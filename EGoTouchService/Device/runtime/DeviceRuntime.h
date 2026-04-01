#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>

#include <deque>
#include <expected>
#include <mutex>

#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <functional>

#include "Device.h"
#include "himax/HimaxChip.h"
#include "SystemStateEvent.h"
#include "FramePipeline.h"
#include "vhf/VhfReporter.h"
#include "StylusSolver/StylusPipeline.h"

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

// --------------- DeviceRuntime ---------------

class DeviceRuntime {
public:
    DeviceRuntime(const std::wstring& master,
                  const std::wstring& slave,
                  const std::wstring& interrupt);
    ~DeviceRuntime();
    DeviceRuntime(const DeviceRuntime&) = delete;
    DeviceRuntime& operator=(const DeviceRuntime&) = delete;

    bool Start();
    void Stop();
    bool IsShutdownRequested() const;
    bool IsRunning() const { return m_running.load(); }
    bool IsSuspended() const { return m_state.load() == workerState::suspend; }

    // Auto/Manual 模式
    void SetAutoMode(bool enabled) { m_autoMode.store(enabled); }
    bool IsAutoMode() const { return m_autoMode.load(); }

    // Touch-Only 模式（跳过 StylusPipeline / ProcessStylusStatus）
    void SetTouchOnlyMode(bool v) { m_touchOnly.store(v); }
    bool IsTouchOnlyMode() const { return m_touchOnly.load(); }

    // 单独的 Stylus VHF 输出开关
    void SetStylusVhfEnabled(bool v) { m_stylusVhfEnabled.store(v); }
    bool IsStylusVhfEnabled() const { return m_stylusVhfEnabled.load(); }

    // Pipeline / VHF 配置 — 仅在 Start() 前调用
    Engine::FramePipeline& GetTouchPipeline() { return m_touchPipeline; }
    // Legacy alias
    Engine::FramePipeline& GetPipeline() { return m_touchPipeline; }
    Engine::StylusPipeline& GetStylusPipeline() { return m_stylusPipeline; }
    VhfReporter& GetVhfReporter() { return m_vhfReporter; }

    /// 注入 BT MCU 压感值（由 PenBridge 线程写入，StylusPipeline 帧内读取）
    void SetBtMcuPressure(uint16_t p) { m_stylusPipeline.SetBtMcuPressure(p); }

    /// BT 频率提供者：每帧调用获取最新 BT MCU 频率 (freq1, freq2)
    using BtFreqProvider = std::function<std::pair<uint8_t, uint8_t>()>;
    void SetBtFreqProvider(BtFreqProvider fn) { m_btFreqProvider = std::move(fn); }

    // Frame push callback for IPC (called after pipeline+VHF in worker loop)
    using FramePushCallback = std::function<void(const Engine::HeatmapFrame&)>;
    void SetFramePushCallback(FramePushCallback cb) { m_framePushCb = std::move(cb); }

    void IngestSystemEvent(const Host::SystemStateEvent& ev);
    uint64_t SubmitCommand(command cmd, CommandSource src,
                           const char* reason = "");

    RuntimeSnapshot GetSnapshot() const;
    std::vector<HistoryEntry> GetHistory(std::size_t n = 200) const;
    void ClearHistory();

private:
    ThreadResult WorkerMain();

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

    std::atomic<workerState> m_state{workerState::quit};
    std::atomic<StopReason> m_stopReason{StopReason::None};
    std::atomic<bool> m_autoMode{false};
    std::atomic<bool> m_touchOnly{false};
    std::atomic<bool> m_stylusVhfEnabled{true};
    Himax::Chip m_chip;
    Engine::FramePipeline m_touchPipeline;
    Engine::StylusPipeline m_stylusPipeline;
    VhfReporter m_vhfReporter;
    uint8_t m_recoverCount = 0;
    bool m_needSuspendDeinit = false;  // suspend 首次进入时执行 Deinit

    // GetFrame 连续非Timeout失败计数（容忍 AFE 命令后的短暂 bus 异常）
    static constexpr int kMaxConsecutiveFrameErrors = 3;
    int m_consecutiveFrameErrors = 0;

    mutable std::mutex m_mu;
    std::deque<QueuedCommand> m_cmdQueue;

    std::vector<HistoryEntry> m_history;
    std::unordered_map<int, std::chrono::steady_clock::time_point>
        m_lastEventByType;
    uint64_t m_lastCmdId = 0;
    std::string m_lastNote;
    std::atomic<uint64_t> m_nextCmdId{1};
    FramePushCallback m_framePushCb;
    BtFreqProvider m_btFreqProvider;

    std::atomic<bool> m_running{false};
    std::thread m_thread;
};