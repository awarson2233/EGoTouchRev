#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>

#include <deque>
#include <expected>
#include <mutex>
#include <optional>

#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <functional>
#include "PenButtonConfig.h"
#include "btmcu/PenUsbTypes.h"
#include "win32/SyntheticPenButtonInjector.h"

namespace Config {
class ConfigStore;
struct ValidationResult;
}

#include "himax/HimaxChip.h"
#include "himax/PanelPowerWatcher.h"
#include "MasterFrameParser.h"
#include "TouchSolver/TsaCore/TsaCoreSolver.h"
#include "vhf/VhfReporter.h"
#include "StylusSolver/StylusPipeline.h"

// --------------- 静态队列（零内存分配） ---------------
template <typename T, std::size_t N>
class StaticQueue {
public:
    bool push(T&& item) noexcept {
        if (m_count >= N) return false;
        m_data[m_tail] = std::move(item);
        m_tail = (m_tail + 1) % N;
        ++m_count;
        return true;
    }

    bool pop(T& outItem) noexcept {
        if (m_count == 0) return false;
        outItem = std::move(m_data[m_head]);
        m_head = (m_head + 1) % N;
        --m_count;
        return true;
    }

    void clear() noexcept {
        m_head = 0;
        m_tail = 0;
        m_count = 0;
    }

    std::size_t size() const noexcept { return m_count; }
    bool empty() const noexcept { return m_count == 0; }

private:
    std::array<T, N> m_data{};
    std::size_t m_head = 0;
    std::size_t m_tail = 0;
    std::size_t m_count = 0;
};

// --------------- 按键与状态动作辅助类型 ---------------
struct PenStateUpdateResult {
    bool stateChanged = false;
    bool applyToPipeline = true;
    bool stylusIdChanged = false;
    uint8_t nextStylusId = 0;
};

struct PenButtonAction {
    enum class Type {
        Barrel,
        Eraser
    };
    Type type;
    bool pressed = false;
    uint8_t rawPayload = 0;
};

// --------------- 基础类型 ---------------

enum class result { error, timeout };
using ThreadResult = std::expected<void, result>;

/// Worker 的阶段。只有 worker 线程会改它（Start 之前与 Stop join 之后除外）；
/// 外部线程通过 PowerInputs 表达意图，由 worker 计算目标并完成迁移。
enum class workerState {
    suspend  = -2,   // 待机：链路已静默（取消读帧、关中断通道），等待亮屏
    quit     = -1,   // 终止：worker 线程退出
    ready    =  0,   // 建链：等面板上电 → 检查总线 → Chip::Init
    streaming,       // 取帧
    recover,         // 建链失败后的退避重试（与 ready 同一段逻辑，单独列出便于观察）
    hold,            // 灭屏宽限：停止取帧但保留链路，宽限内亮屏且面板没断电则直接继续取帧
};

/// 宿主电源输入（电平）。外部线程只写这里，worker 每轮读一次快照、算目标阶段。
struct PowerInputs {
    bool displayOn = true;
    bool lidOpen = true;
    bool systemSuspended = false;
    bool shutdown = false;
    /// 面板电源真值（SPB 驱动的 ACPI bit0）。Windows 的显示事件只是宿主的意图，TDDI 的触摸部分
    /// 跟着这一位上下电；内屏单独断电时没有任何 Windows 事件，只能从这里看到。
    bool panelOn = true;
    /// 面板电源位每掉一次 +1（灭屏宽限期靠它判断面板有没有断过电）。
    uint64_t panelDrops = 0;
    /// 每个可能让面板 / SPB 链路断电的事件（灭屏、合盖、系统休眠、系统唤醒、面板断电）+1。
    /// worker 只在「链路自上次证明以来没有经历过这类事件」时才继续取帧。
    uint64_t downEpoch = 0;
    /// 任一输入变化 +1，用于唤醒等待中的 worker。
    uint64_t revision = 0;
    std::chrono::steady_clock::time_point displayOffAt{};
    std::chrono::steady_clock::time_point displayOnAt{};
};

/// 由 PowerInputs 推出的目标。
enum class PowerTarget : uint8_t {
    Quit,
    Standby,
    Hold,      // 只因灭屏，且还在宽限期内
    Active,
};

const char* ToString(PowerTarget t) noexcept;

const char* ToString(workerState s) noexcept;

enum class CommandSource : uint8_t {
    External = 0,
    SystemPolicy,
};

const char* ToString(CommandSource s) noexcept;

class RuntimePolicyEvent {
public:
    enum class Type : uint8_t {
        Unknown = 0,
        DisplayOn,
        DisplayOff,
        LidOn,
        LidOff,
        Suspend,
        Shutdown,
        ResumeAutomatic,
    };

    enum class Source : uint8_t {
        HostSystemState = 0,
    };

    Type type = Type::Unknown;
    Source source = Source::HostSystemState;
    std::chrono::system_clock::time_point timestamp{};
    uint32_t rawIndex = 0;
};

const char* ToString(RuntimePolicyEvent::Type type) noexcept;

// --------------- 审计日志 ---------------

#if defined(NDEBUG)
inline constexpr std::size_t kMaxHistoryItems = 128;
#else
inline constexpr std::size_t kMaxHistoryItems = 512;
#endif

struct HistoryEntry {
    std::chrono::system_clock::time_point timestamp{};
    uint64_t command_id = 0;
    std::array<char, 32> command_name{};
    CommandSource source = CommandSource::External;
    bool success = false;
    std::array<char, 32> detail{};
};

struct RuntimeSnapshot {
    workerState state = workerState::quit;
    bool stylus_connected = false;
    uint8_t recover_count = 0;
    std::size_t queue_depth = 0;
    uint64_t last_command_id = 0;
    std::string last_note;
};

struct RuntimePenState {
    uint16_t factoryStatusFlags = 0;

    bool hasConnection = false;
    bool connected = false;

    bool hasPairStatus = false;
    uint8_t pairStatus = 0;

    bool hasStylusId = false;
    uint8_t stylusId = 0;

    Solvers::StylusProtocolHint protocolHint = Solvers::StylusProtocolHint::Auto;
    bool protocolHintFromPenModule = false;
    uint32_t penRevision = 0;
    uint32_t pipelineRevision = 0;

    bool hasPenModuleModelId = false;
    uint32_t penModuleModelId = 0;
    Himax::Pen::PenModuleModel penModuleModel = Himax::Pen::PenModuleModel::Unknown;

    bool hasSerialNumber = false;
    std::string serialNumber;

    bool hasHardwareVersion = false;
    std::string hardwareVersion;

    bool hasFirmwareVersion = false;
    std::string firmwareVersion;

    bool hasCurrentMode = false;
    Himax::Pen::PenCurrentMode currentMode = Himax::Pen::PenCurrentMode::Unknown;
    uint8_t currentModeRaw = 0;

    bool hasEraserToggle = false;
    uint8_t eraserToggle = 0;

    bool hasCurrentFunc = false;
    uint8_t currentFunc = 0;
};

struct PenAfeCommandPlan {
    std::array<command, 2> commands{};
    std::size_t count = 0;
};

/// Build the AFE state reconstruction required after a full runtime restart.
/// Commands are ordered by hardware dependency: InitStylus before SetStylusId.
PenAfeCommandPlan BuildPenAfeCommandPlan(const RuntimePenState& state) noexcept;
std::optional<command> BuildPenConnectionAfeCommand(
    bool connectionChanged, bool connected) noexcept;

struct PenAfeReplayState {
    uint64_t generation = 0;
    bool pending = false;

    void BeginInitCycle() noexcept {
        ++generation;
        if (generation == 0) {
            ++generation;
        }
        pending = true;
    }

    void CompleteInitCycle() noexcept { pending = false; }

    bool IsCurrent(uint64_t commandGeneration) const noexcept {
        return commandGeneration != 0 &&
               commandGeneration == generation &&
               !pending;
    }
};

// --------------- DeviceRuntime ---------------

struct DeviceRuntimePenStateTestAccess;

class DeviceRuntime {
public:
    enum class StartRequestResult : uint8_t {
        Started = 0,
        AlreadyRunning,
        Failed,
    };

    enum class StopRequestResult : uint8_t {
        Stopped = 0,
        AlreadyStopped,
    };

    DeviceRuntime(const std::wstring& master,
                  const std::wstring& slave,
                  const std::wstring& interrupt);
    ~DeviceRuntime();
    DeviceRuntime(const DeviceRuntime&) = delete;
    DeviceRuntime& operator=(const DeviceRuntime&) = delete;

    bool Start();
    void Stop();
    StartRequestResult RequestStart();
    StopRequestResult RequestStop();
    bool IsShutdownRequested() const;
    bool IsRunning() const { return m_running.load(); }
    bool IsSuspended() const { return m_state.load() == workerState::suspend; }
    PowerInputs GetPowerInputs() const;

    // Auto/Manual 模式
    void SetAutoMode(bool enabled) { m_autoMode.store(enabled); }
    bool IsAutoMode() const { return m_autoMode.load(); }


    // 单独的 Stylus VHF 输出开关
    void SetStylusVhfEnabled(bool v) { m_stylusVhfEnabled.store(v); }
    bool IsStylusVhfEnabled() const { return m_stylusVhfEnabled.load(); }
    void ApplyServicePolicy(bool autoMode, bool stylusVhfEnabled,
                            PenButtonMode penButtonMode = PenButtonMode::OemCustom,
                            PenButtonRoute penButtonRoute = PenButtonRoute::VhfOnly,
                            bool penButtonRouteExplicit = false);
#if EGOTOUCH_SERVICE_ENABLE_IPC
    Config::ValidationResult ValidateConfigStore(const Config::ConfigStore& store) const;
    void ApplyConfigStore(const Config::ConfigStore& store);
    void ApplyPipelineConfig(const Config::ConfigStore& store) {
        ApplyConfigStore(store);
    }
#endif

    void SetPenButtonMode(PenButtonMode m) { m_penButtonMode.store(m, std::memory_order_release); }
    PenButtonMode GetPenButtonMode() const { return m_penButtonMode.load(std::memory_order_acquire); }
    void SetPenButtonRoute(PenButtonRoute r, bool explicitRoute = true) {
        m_penButtonRoute.store(r, std::memory_order_release);
        m_penButtonRouteExplicit.store(explicitRoute, std::memory_order_release);
    }
    PenButtonRoute GetPenButtonRoute() const { return m_penButtonRoute.load(std::memory_order_acquire); }

    // Pipeline/VHF façade methods for Phase 0 contract freeze.
    void SetVhfEnabled(bool enabled);
    bool IsVhfEnabled() const;
    bool IsVhfDeviceOpen() const;
    void SetVhfTransposeEnabled(bool enabled);
    bool IsVhfTransposeEnabled() const;
    void SetMasterParserOnlyMode(bool enabled);

    /// 注入 BT MCU 压感值（由 PenBridge 线程写入，StylusPipeline 帧内读取）
    void IngestBtMcuPressure(uint16_t p);
    void IngestBtMcuPressurePacket(const std::array<uint16_t, 4>& pressure,
                                   const std::array<uint16_t, 4>& rawPressure,
                                   uint8_t freq1,
                                   uint8_t freq2);

    // Frame push callback for IPC (called after pipeline+VHF in worker loop)
    using FramePushCallback = std::function<void(const Solvers::HeatmapFrame&)>;
#ifdef _DEBUG
    void SetFramePushCallback(FramePushCallback cb);
#endif

    void IngestPolicyEvent(const RuntimePolicyEvent& ev);
    bool SubmitExternalAfeCommand(AFE_Command type, uint8_t param);
    uint64_t SubmitCommand(command cmd, CommandSource src,
                           const char* reason = "");

    RuntimeSnapshot GetSnapshot() const;
    RuntimePenState GetPenStateSnapshot() const;
    /// 第一次建链时从显示驱动 OTP 读到的面板型号；还没建链或读失败为空
    std::optional<Himax::PanelIdentity> GetPanelIdentity() const { return m_chip.GetPanelIdentity(); }
    PenAfeReplayState GetPenAfeReplayStateSnapshot() const;
    std::vector<HistoryEntry> GetHistory(std::size_t n = 200) const;
    void ClearHistory();

    /// Deterministic one-shot hook invoked on the actual worker thread.
    void SetWorkerHookForTesting(std::function<void()> hook);
    bool IsAcceptingExternalAfeCommands() const;

    /// MCU 事件 ingress（runtime 内部完成状态/AFE 命令分派）
    void IngestPenEvent(const Himax::Pen::PenEvent& ev);

    /// 主机电源转换（Suspend / DisplayOff / LidOff）时注销笔连接。
    /// MCU 在休眠期间不会补发 PenConnStatus，若不主动注销，m_penState.connected
    /// 会保持陈旧 true，恢复期 ReplayPenStateAfterChipInit() 会据此向 AFE 重放
    /// InitStylus，与尚未重新握手的 MCU 侧状态不一致。
    /// @return true 表示连接状态确实发生了变化。
    bool NotifyPenLinkDown();

    /// 笔事件通道重新打开时调用：连接状态与笔身份回到服务冷启动时的"未知"。
    /// NotifyPenLinkDown 写下的"已断开"只能由 MCU 的 PenConnStatus 解除，而 MCU
    /// 只应答亮屏时的 0x7101；亮屏那一刻通道若正在重连，这次应答就丢了。新通道上
    /// 的连接状态以 MCU 重新上报为准，在那之前按冷启动处理（笔管线按帧自动识别、
    /// 芯片初始化后不重放 InitStylus）。不下发 AFE 命令。
    void NotifyPenLinkReset();

private:
    friend struct DeviceRuntimePenStateTestAccess;
    StartRequestResult StartStateMachine();
    ThreadResult WorkerMain();
    void HandlePenButtonStatusCode(uint8_t statusCode,
                                   uint8_t rawEventPayload,
                                   const char* source);
    void ApplyPenStateToStylusPipeline();
    void UpdatePenState(std::function<void(RuntimePenState&, PenStateUpdateResult&)> updateFn);
    void SubmitPenAfeCommandLocked(command cmd, const char* reason);
    void BeginPenReplayInitCycle();
    void ReplayPenStateAfterChipInit();
    void DispatchPenButtonAction(const PenButtonAction& action, const char* source);

    // ── Worker 状态机 ──
    // 每轮：读输入快照 → ComputeTarget → 按目标与当前阶段做一步。阶段迁移只走
    // TransitionTo，离开 / 进入取帧的动作集中在 LeaveStreaming / EnterStreaming。
    PowerTarget ComputeTarget(const PowerInputs& in,
                              std::chrono::steady_clock::time_point now) const;
    void StepStandby(const PowerInputs& in);
    void StepHold(const PowerInputs& in);
    void StepActive(const PowerInputs& in);
    void StepBringUp(const PowerInputs& in);
    void OnStreaming();
    bool OnQuit();

    void TransitionTo(workerState next, const char* reason);
    void EnterStandby(const char* reason);
    void BeginBringUp(const char* reason, workerState phase);
    void EnterStreaming(const PowerInputs& in);
    void LeaveStreaming(const char* reason);
    void ExecuteSolverAfeActions(const Solvers::Tsa::AfeActionList& actions);
    /// 等到输入变化或超时。返回 true 表示输入变了。
    bool WaitForInputs(std::chrono::milliseconds timeout);
    void UpdateInputs(const char* why, const std::function<bool(PowerInputs&)>& mutate,
                      bool interruptFrameRead);
    /// 记一次面板电源读数（监听线程的中断通知、worker 的轮询都走这里）。读不到时不改电平。
    void NotePanelPower(std::optional<bool> panelOn, const char* source);

    struct QueuedCommand {
        uint64_t id = 0;
        uint64_t penGeneration = 0;
        command cmd{};
        CommandSource source = CommandSource::External;
        std::chrono::steady_clock::time_point enqueued_at{};
        std::array<char, 32> reason{};
    };

    QueuedCommand MakeQueuedCommand(command cmd, CommandSource src,
                                    const char* reason,
                                    uint64_t penGeneration = 0);
    uint64_t EnqueueCommand(command cmd, CommandSource src,
                            const char* reason, uint64_t penGeneration = 0);
    bool ShouldExecuteCommand(const QueuedCommand& qc);
    bool ExecuteCommand(const QueuedCommand& qc);
    bool DrainCommands();
    void CancelQueuedCommandsLocked(const char* detail);
    void RecordHistory(const QueuedCommand& qc,
                       bool ok, const std::string& det);

    void SetState(workerState newState);
    std::atomic<workerState> m_state{workerState::quit};

    // 外部输入（受 m_mu 保护）与 worker 私有的链路记账
    PowerInputs m_inputs{};
    std::condition_variable m_inputsCv;
    uint64_t m_seenRevision = 0;
    struct LinkState {
        uint64_t provenEpoch = 0;        // 上次证明链路完好时的 downEpoch
        uint64_t holdPanelDrops = 0;     // 进入灭屏宽限期时的 panelDrops
        int bringUpAttempts = 0;         // 连续建链失败次数
        std::chrono::steady_clock::time_point nextAttemptAt{};
        std::chrono::steady_clock::time_point busWaitSince{};   // 开始等总线就绪的时刻
        // 最近一次记入日志的探测结果，只在变化时再记
        bool probeLogged = false;
        bool lastIoOk = false;
        uint8_t lastBusMaster = 0;
        uint8_t lastBusSlave = 0;
        int lastPanel = -1;              // -1 读不到，0 断电，1 上电
    } m_link{};

    // 状态机心跳：worker 每 30 s 记一行，证明自己活着、停在哪、为什么
    struct Heartbeat {
        std::chrono::steady_clock::time_point next{};
        uint32_t frames = 0;         // 取到的帧（含 IDLE 帧）
        uint32_t idleFrames = 0;     // 其中 IC 自报 IDLE 的帧
        uint32_t noData = 0;         // 没有可用数据的轮次（无效帧 / 读被取消）
        uint32_t sos = 0;            // 芯片层判 SOS、整体重启链路的次数
        int64_t maxReadMs = 0;
    } m_hb{};

    void NoteLinkProbe(const Himax::Chip::BusProbe& probe, std::optional<bool> panel, const char* where);
    void MaybeHeartbeat(PowerTarget target, const PowerInputs& in);
    std::string DescribeInputs(const PowerInputs& in) const;

    RuntimePenState m_penState{};
    mutable std::mutex m_penStateMu;
    mutable std::mutex m_penIngressMu;
    bool m_acceptPenAfeCommands = false;
    PenAfeReplayState m_penReplay{};
    std::atomic<bool> m_autoMode{false};
    std::atomic<bool> m_stylusVhfEnabled{true};
    std::atomic<bool> m_masterParserOnly{false};
    std::atomic<PenButtonMode> m_penButtonMode{PenButtonMode::OemCustom};
    std::atomic<PenButtonRoute> m_penButtonRoute{PenButtonRoute::VhfOnly};
    std::atomic<bool> m_penButtonRouteExplicit{false};
    SyntheticPenButtonInjector m_synthPenButton;
    Himax::Chip m_chip;
    Himax::PanelPowerWatcher m_panelWatcher;
    Solvers::MasterFrameParser m_frameParser;
    Solvers::Tsa::TsaCoreSolver m_tsaCoreSolver;
    Solvers::StylusPipeline m_stylusPipeline;
    mutable std::mutex m_pipelineMu;
    VhfReporter m_vhfReporter;
    uint8_t m_recoverCount = 0;


    mutable std::mutex m_mu;
    StaticQueue<QueuedCommand, 16> m_cmdQueue{};
    bool m_acceptExternalAfeCommands = false;

    mutable std::mutex m_workerHookMu;
    std::function<void()> m_workerHookForTesting;

    std::array<HistoryEntry, kMaxHistoryItems> m_history{};
    size_t m_historyWriteIdx = 0;
    size_t m_historyCount = 0;
    uint64_t m_lastCmdId = 0;
    std::string m_lastNote;
    std::atomic<uint64_t> m_nextCmdId{1};
#ifdef _DEBUG
    mutable std::mutex m_framePushCbMu;
    FramePushCallback m_framePushCb;
#endif

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stopped{false};
    mutable std::mutex m_lifecycleMu;
    std::condition_variable m_lifecycleCv;
    bool m_stopInProgress = false;
    bool m_selfStopDetached = false;
    std::thread::id m_workerThreadId{};
    std::thread m_thread;
};