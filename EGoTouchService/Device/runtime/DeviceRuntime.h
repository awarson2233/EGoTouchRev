#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>

#include <deque>
#include <expected>
#include <mutex>

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
#include "TouchSolver/TouchPipeline.h"
#include "vhf/VhfReporter.h"
#include "StylusSolver/StylusPipeline.h"

namespace Config {
class ConfigStore;
}

// --------------- 基础类型 ---------------

enum class result { error, timeout };
using ThreadResult = std::expected<void, result>;

enum class workerState {
    suspend  = -2,   // 屏幕关闭/合盖 → worker 暂停（线程保留，等待唤醒）
    quit     = -1,   // 服务终止/系统关机 → worker 线程退出
    ready    =  0,   // 初始就绪
    streaming,       // 正在采帧
    recover,         // 恢复中
};

/// 停止原因（替代原来的 m_stopReq + m_shutdownReq 双 flag）
enum class StopReason : uint8_t {
    None = 0,
    ScreenOff,   // DisplayOff / LidOff → 进入 suspend
    Shutdown,    // 系统关机 / Stop() → 进入 quit
};

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

struct HistoryEntry {
    std::chrono::system_clock::time_point timestamp{};
    uint64_t command_id = 0;
    std::string command_name;
    CommandSource source = CommandSource::External;
    bool success = false;
    std::string detail;
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

    bool hasStylusId = false;
    uint8_t stylusId = 0;

    Solvers::StylusProtocolHint protocolHint = Solvers::StylusProtocolHint::Auto;
    bool protocolHintFromPenModule = false;
    uint32_t penRevision = 0;

    bool hasPenModuleModelId = false;
    uint32_t penModuleModelId = 0;
    Himax::Pen::PenModuleModel penModuleModel = Himax::Pen::PenModuleModel::Unknown;

    bool hasHardwareVersion = false;
    std::string hardwareVersion;

    bool hasCurrentMode = false;
    Himax::Pen::PenCurrentMode currentMode = Himax::Pen::PenCurrentMode::Unknown;
    uint8_t currentModeRaw = 0;

    bool hasEraserToggle = false;
    uint8_t eraserToggle = 0;

    bool hasCurrentFunc = false;
    uint8_t currentFunc = 0;
};

// --------------- DeviceRuntime ---------------

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
    Config::ValidationResult ValidateConfigStore(const Config::ConfigStore& store) const;
    void ApplyConfigStore(const Config::ConfigStore& store);
    void ApplyPipelineConfig(const Config::ConfigStore& store);

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
    std::vector<HistoryEntry> GetHistory(std::size_t n = 200) const;
    void ClearHistory();

    /// MCU 事件 ingress（runtime 内部完成状态/AFE 命令分派）
    void IngestPenEvent(const Himax::Pen::PenEvent& ev);

private:
    ThreadResult WorkerMain();
    void HandlePenButtonStatusCode(uint8_t statusCode,
                                   uint8_t rawEventPayload,
                                   const char* source);
    void ApplyPenStateToStylusPipeline();

    // ── Worker 状态处理（每个状态一个入口，Worker 只做调度） ──
    void OnReady();              // ready → 尝试 auto init
    void OnStreaming();          // streaming → 采帧 + 处理
    void OnRecover();            // recover → 重试恢复
    void OnSuspend();            // suspend → 屏幕关闭，暂停等待唤醒
    bool OnQuit();               // quit → 清理并退出

    struct QueuedCommand {
        uint64_t id = 0;
        command cmd{};
        CommandSource source = CommandSource::External;
        std::chrono::steady_clock::time_point enqueued_at{};
        std::string reason;
    };

    bool DrainCommands();
    void RecordHistory(const QueuedCommand& qc,
                       bool ok, const std::string& det);

    void SetState(workerState newState);
    std::atomic<workerState> m_state{workerState::quit};
    std::atomic<StopReason> m_stopReason{StopReason::None};

    RuntimePenState m_penState{};
    mutable std::mutex m_penStateMu;
    std::atomic<bool> m_autoMode{false};
    std::atomic<bool> m_stylusVhfEnabled{true};
    std::atomic<bool> m_masterParserOnly{false};
    std::atomic<PenButtonMode> m_penButtonMode{PenButtonMode::OemCustom};
    std::atomic<PenButtonRoute> m_penButtonRoute{PenButtonRoute::VhfOnly};
    std::atomic<bool> m_penButtonRouteExplicit{false};
    SyntheticPenButtonInjector m_synthPenButton;
    Himax::Chip m_chip;
    Solvers::TouchPipeline m_touchPipeline;
    Solvers::StylusPipeline m_stylusPipeline;
    mutable std::mutex m_pipelineMu;
    VhfReporter m_vhfReporter;
    uint8_t m_recoverCount = 0;
    std::atomic<bool> m_needSuspendDeinit{false};

    // GetFrame 连续非Timeout失败计数（容忍 AFE 命令后的短暂 bus 异常）
    static constexpr int kMaxConsecutiveFrameErrors = 3;
    int m_consecutiveFrameErrors = 0;

    mutable std::mutex m_mu;
    std::deque<QueuedCommand> m_cmdQueue;
    bool m_displayOffSuspendPending = false;
    std::chrono::steady_clock::time_point m_displayOffSuspendDeadline{};
    std::atomic<bool> m_systemSuspendObserved{false};

    std::vector<HistoryEntry> m_history;
    std::unordered_map<int, std::chrono::steady_clock::time_point>
        m_lastEventByType;
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