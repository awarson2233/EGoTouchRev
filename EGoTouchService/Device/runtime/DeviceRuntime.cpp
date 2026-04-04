#include "runtime/DeviceRuntime.h"
#include "EngineTypes.h"
#include "Logger.h"


#include <format>

namespace {
constexpr std::size_t kMaxHistoryItems = 512;
constexpr std::chrono::milliseconds kEventDebounce{400};
} // namespace

// --------------- ToString helpers ---------------

const char* ToString(workerState s) noexcept {
    switch (s) {
    case workerState::suspend:   return "suspend";
    case workerState::quit:      return "quit";
    case workerState::ready:     return "ready";
    case workerState::streaming: return "streaming";
    case workerState::recover:   return "recover";
    default:                     return "unknown";
    }
}

const char* ToString(CommandSource s) noexcept {
    switch (s) {
    case CommandSource::External:     return "External";
    case CommandSource::SystemPolicy: return "SystemPolicy";
    default:                          return "Unknown";
    }
}

// --------------- Lifecycle ---------------

DeviceRuntime::DeviceRuntime(
        const std::wstring& master,
        const std::wstring& slave,
        const std::wstring& interrupt)
    : m_chip(master, slave, interrupt) {}

DeviceRuntime::~DeviceRuntime() { Stop(); }

bool DeviceRuntime::Start() {
    if (m_running.exchange(true)) return false;
    m_stopReason.store(StopReason::None);  // ← critical: clear stop reason for restart
    SetState(workerState::ready);
    m_needSuspendDeinit = false;
    m_recoverCount = 0;
    m_lastNote = "Runtime started";
    m_thread = std::thread(&DeviceRuntime::WorkerMain, this);
    LOG_INFO("Runtime", __func__, "ready", "Worker thread launched.");
    return true;
}

void DeviceRuntime::Stop() {
    m_stopReason.store(StopReason::Shutdown);
    if (m_thread.joinable()) m_thread.join();
    m_running.store(false);
    SetState(workerState::quit);
    m_lastNote = "Runtime stopped";
}

bool DeviceRuntime::IsShutdownRequested() const {
    return m_stopReason.load() == StopReason::Shutdown;
}

// --------------- 命令注入 ---------------

uint64_t DeviceRuntime::SubmitCommand(
        command cmd, CommandSource src, const char* reason) {
    QueuedCommand qc{};
    qc.id = m_nextCmdId.fetch_add(1);
    qc.cmd = cmd;
    qc.source = src;
    qc.enqueued_at = std::chrono::steady_clock::now();
    qc.reason = reason ? reason : "";
    {
        std::lock_guard<std::mutex> lk(m_mu);
        m_cmdQueue.push_back(std::move(qc));
    }
    return qc.id;
}

void DeviceRuntime::IngestSystemEvent(
        const Host::SystemStateEvent& ev) {
    const auto now = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lk(m_mu);
        int key = static_cast<int>(ev.type);
        auto it = m_lastEventByType.find(key);
        if (it != m_lastEventByType.end() &&
            now - it->second < kEventDebounce) return;
        m_lastEventByType[key] = now;
    }
    using ET = Host::SystemStateEventType;
    switch (ev.type) {
    case ET::DisplayOff:
    case ET::LidOff:
        LOG_INFO("Runtime", __func__, "Policy", "Sleep event ({}), requesting suspend.", Host::ToString(ev.type));
        m_chip.CancelPendingFrameRead();  // abort any blocking GetFrame NOW
        m_stopReason.store(StopReason::ScreenOff);
        break;
    case ET::DisplayOn:
    case ET::LidOn:
    case ET::ResumeAutomatic:
        LOG_INFO("Runtime", __func__, "Policy", "Wake event ({}), attempting resume.", Host::ToString(ev.type));
        // suspend 状态下直接恢复，无需重建线程
        if (m_state.load() == workerState::suspend) {
            SetState(workerState::ready);
            LOG_INFO("Runtime", __func__, "Policy", "Resumed from suspend -> ready (zero-cost wakeup).");
        } else {
            Stop();
            Start();
        }
        break;
    case ET::Shutdown:
        LOG_INFO("Runtime", __func__, "Policy", "Shutdown event, requesting termination.");
        m_stopReason.store(StopReason::Shutdown);
        break;
    default: break;
    }
}

// --------------- Pipe 查询 ---------------

RuntimeSnapshot DeviceRuntime::GetSnapshot() const {
    std::lock_guard<std::mutex> lk(m_mu);
    RuntimeSnapshot s;
    s.state = m_state.load();
    s.stylus_connected = m_chip.m_afe.GetStylusState().connected;
    s.recover_count = m_recoverCount;
    s.queue_depth = m_cmdQueue.size();
    s.last_command_id = m_lastCmdId;
    s.last_note = m_lastNote;
    return s;
}

std::vector<HistoryEntry> DeviceRuntime::GetHistory(
        std::size_t n) const {
    std::lock_guard<std::mutex> lk(m_mu);
    if (n >= m_history.size()) return m_history;
    return {m_history.end() - static_cast<ptrdiff_t>(n),
            m_history.end()};
}

void DeviceRuntime::ClearHistory() {
    std::lock_guard<std::mutex> lk(m_mu);
    m_history.clear();
}



// --------------- 审计日志 ---------------

void DeviceRuntime::RecordHistory(
        const QueuedCommand& qc, bool ok,
        const std::string& det) {
    HistoryEntry e;
    e.timestamp = std::chrono::system_clock::now();
    e.command_id = qc.id;
    e.command_name = qc.reason;
    e.source = qc.source;
    e.success = ok;
    e.detail = det;
    m_history.push_back(std::move(e));
    if (m_history.size() > kMaxHistoryItems)
        m_history.erase(m_history.begin(),
            m_history.begin() +
            static_cast<ptrdiff_t>(
                m_history.size() - kMaxHistoryItems));
}

// --------------- 命令执行 ---------------

bool DeviceRuntime::DrainCommands() {
    std::lock_guard<std::mutex> lk(m_mu);
    while (!m_cmdQueue.empty()) {
        auto qc = m_cmdQueue.front();
        m_cmdQueue.pop_front();
        m_lastCmdId = qc.id;
        if (auto r = m_chip.m_afe.SendCommand(qc.cmd); !r) {
            RecordHistory(qc, false, "afe_sendCommand failed");
            LOG_WARN("Runtime", __func__, "CmdExec", "Command '{}' (type={}) failed — skipping (non-fatal).", qc.reason, static_cast<int>(qc.cmd.type));
            // 不再触发 recover: AFE 命令失败不代表 bus 挂了
            continue;
        }
        RecordHistory(qc, true, "OK");
    }
    return true;
}

// ----------- Worker 核心循环 -----------

ThreadResult DeviceRuntime::WorkerMain() {
    while (true) {
        // ── 检查停止请求，根据 StopReason 分流到 suspend 或 quit ──
        auto reason = m_stopReason.exchange(StopReason::None,
                                            std::memory_order_acq_rel);
        if (reason != StopReason::None) {
            if (reason == StopReason::ScreenOff) {
                LOG_INFO("Runtime", __func__, "StopReq", "StopReason::ScreenOff consumed -> suspend");
                SetState(workerState::suspend);
                m_needSuspendDeinit = true;   // 延迟到 OnSuspend 首次进入时执行
            } else {
                LOG_INFO("Runtime", __func__, "StopReq", "StopReason::Shutdown consumed -> quit");
                SetState(workerState::quit);
            }
            std::lock_guard<std::mutex> lk(m_mu);
            m_cmdQueue.clear();
        }

        DrainCommands();

        auto curState = m_state.load(std::memory_order_acquire);
        switch (curState) {
        case workerState::ready:     OnReady();     break;
        case workerState::streaming: OnStreaming();  break;
        case workerState::recover:   OnRecover();   break;
        case workerState::suspend:   OnSuspend();   break;
        case workerState::quit:
            if (OnQuit()) {
                m_running.store(false);  // allow restart via Start()
                LOG_INFO("Runtime", __func__, "quit", "Worker exited, m_running=false.");
                return ThreadResult();
            }
            break;
        }
    }
    return ThreadResult();
}

// ----------- 状态处理 -----------

void DeviceRuntime::OnReady() {
    if (m_autoMode.load()) {
        if (auto r = m_chip.Init(); !r) {
            SetState(workerState::recover);
            return;
        }
        SetState(workerState::streaming);
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void DeviceRuntime::OnStreaming() {
    auto res = m_chip.GetFrame();
    if (res == std::unexpected(ChipError::Timeout)) return;
    if (!res) {
        // 连续 N 帧非Timeout失败才进入 recover；
        // AFE 命令（如 EnableFreqShift）可能导致一两帧 bus 暂时失响应，不算致命。
        m_consecutiveFrameErrors++;
        if (m_consecutiveFrameErrors >= kMaxConsecutiveFrameErrors) {
            LOG_ERROR("Runtime", __func__, "Streaming", "{} consecutive GetFrame failures -> recover.", m_consecutiveFrameErrors);
            m_consecutiveFrameErrors = 0;
            SetState(workerState::recover);
        } else {
            LOG_WARN("Runtime", __func__, "Streaming", "GetFrame failed ({}/{}), retrying...", m_consecutiveFrameErrors, kMaxConsecutiveFrameErrors);
        }
        return;
    }
    m_consecutiveFrameErrors = 0;  // 成功读帧重置计数

    const bool touchOnly = m_touchOnly.load(std::memory_order_relaxed);

    // 0. BT MCU 心跳注入（每帧，原厂 ASA_SetBluetoothFreq 等价）
    if (!touchOnly && m_btFreqProvider) {
        auto [f1, f2] = m_btFreqProvider();
        m_chip.m_afe.UpdateBtHeartbeat(f1, f2);
    }

    // 1. 手写笔频率跟踪（仅 Full 模式）
    if (!touchOnly) {
        if (m_chip.m_afe.ProcessStylusStatus()) {
            SubmitCommand({AFE_Command::ForceToFreqPoint, m_chip.m_afe.GetStylusState().switchTargetIdx},
                          CommandSource::SystemPolicy, "Stylus Freq Sync Requested");
        }
    }

    const auto& rawData = m_chip.back_data;

    // 2. Stylus pipeline（仅 Full 模式）
    // rawData 为 master+slave 复合帧；pipeline 只需 slave frame（339字节，偏移 5063）
    Engine::StylusPacket stylusPacket{};
    if (!touchOnly) {
        static constexpr size_t kMasterBytes = 5063;
        static constexpr size_t kSlaveBytes  = 339;
        if (rawData.size() >= kMasterBytes + kSlaveBytes) {
            m_stylusPipeline.Process(
                std::span<const uint8_t>(
                    rawData.data() + kMasterBytes, kSlaveBytes),
                stylusPacket);
        }
        if (m_stylusVhfEnabled.load(std::memory_order_relaxed)) {
            m_vhfReporter.DispatchStylus(stylusPacket);
        }
    }

    // 3. Touch pipeline (always active)
    Engine::HeatmapFrame touchFrame;
    touchFrame.rawData.assign(rawData.begin(), rawData.end());
    touchFrame.masterWasRead = m_chip.m_lastMasterWasRead;  // 传递 master 读取状态给帧写入器
    m_touchPipeline.Execute(touchFrame);
    m_vhfReporter.DispatchTouch(touchFrame);

    // 4. Merge results for UI push
    if (m_framePushCb) {
        if (!touchOnly) {
            touchFrame.stylus = m_stylusPipeline.GetLastResult();
            touchFrame.stylus.packet = stylusPacket;
        }
        m_framePushCb(touchFrame);
    }
}

bool DeviceRuntime::OnQuit() {
    if (m_autoMode.load()) {
        (void)m_chip.Deinit(false);
    }
    return true;  // signal WorkerMain to return
}

void DeviceRuntime::OnSuspend() {
    // 首次进入 suspend 时执行 HoldReset（拉低 reset，关闭中断通道）
    if (m_needSuspendDeinit) {
        m_chip.HoldReset();
        m_needSuspendDeinit = false;
        LOG_INFO("Runtime", __func__, "suspend", "Entered suspend, chip reset held low. Waiting for wake event.");
    }
    // 低功耗等待，每 100ms 检查一次状态变更
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void DeviceRuntime::OnRecover() {
    m_recoverCount++;

    // 最大重试 30 次（500ms 间隔 ≈ 15 秒恢复窗口）
    if (m_recoverCount > 30) {
        LOG_ERROR("Runtime", __func__, "Recover", "Exceeded 30 recovery attempts, entering suspend.");
        m_recoverCount = 0;
        m_needSuspendDeinit = true;
        SetState(workerState::suspend);
        return;
    }

    // 等待 500ms 再重试，给硬件从休眠/灭屏恢复的时间
    // 期间每 50ms 检查一次 stop 请求以保持响应性
    for (int i = 0; i < 10; ++i) {
        if (m_stopReason.load(std::memory_order_relaxed) != StopReason::None) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    LOG_INFO("Runtime", __func__, "Recover", "Recovery attempt {}/30...", m_recoverCount);

    if (auto res = m_chip.check_bus(); !res) return;
    if (auto res = m_chip.Init(); !res) return;

    LOG_INFO("Runtime", __func__, "Recover", "Recovery succeeded after {} attempts.", m_recoverCount);
    SetState(workerState::streaming);
    m_recoverCount = 0;
}

void DeviceRuntime::SetState(workerState newState) {
    workerState old = m_state.exchange(newState, std::memory_order_acq_rel);
    if (old != newState) {
        LOG_INFO("Runtime", __func__, "StateTransition", "State changed: {} -> {}", ToString(old), ToString(newState));
    }
}