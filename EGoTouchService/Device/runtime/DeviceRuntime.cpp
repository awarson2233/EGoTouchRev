#include "runtime/DeviceRuntime.h"
#include "Logger.h"
#include "SolverTypes.h"


#include <chrono>
#include <cstdio>
#include <format>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

namespace {
constexpr std::size_t kMaxHistoryItems = 512;
constexpr std::chrono::milliseconds kEventDebounce{400};
constexpr std::chrono::milliseconds kDisplayOffSuspendDelay{2000};

uint8_t PayloadByteOrZero(const Himax::Pen::PenEvent &ev) noexcept {
  return ev.payload.empty() ? 0 : ev.payload[0];
}

struct PenButtonRoutePlan {
  bool vhf = false;
  bool win32 = false;
};

PenButtonRoutePlan DefaultPenButtonRouteForMode(PenButtonMode mode) noexcept {
  switch (mode) {
  case PenButtonMode::OemCustom:
    return {.vhf = true, .win32 = false};
  case PenButtonMode::NativeBarrel:
  case PenButtonMode::NativeEraser:
    return {.vhf = false, .win32 = true};
  default:
    return {.vhf = true, .win32 = false};
  }
}

PenButtonRoutePlan ResolvePenButtonRoute(PenButtonMode mode,
                                         PenButtonRoute route,
                                         bool routeExplicit) noexcept {
  if (!routeExplicit && route == PenButtonRoute::VhfOnly) {
    return DefaultPenButtonRouteForMode(mode);
  }

  switch (route) {
  case PenButtonRoute::VhfOnly:
    return {.vhf = true, .win32 = false};
  case PenButtonRoute::Win32Only:
    return {.vhf = false, .win32 = true};
  case PenButtonRoute::VhfAndWin32:
    return {.vhf = true, .win32 = true};
  default:
    return DefaultPenButtonRouteForMode(mode);
  }
}

const char *FactoryStatusEventName(Himax::Pen::PenUsbEventCode code) noexcept {
  using EC = Himax::Pen::PenUsbEventCode;
  switch (code) {
  case EC::PenAcStatus:
    return "PenAcStatus";
  case EC::PenConnStatus:
    return "PenConnStatus";
  case EC::PenCurStatus:
    return "PenCurStatus";
  case EC::PenTypeInfo:
    return "PenTypeInfo";
  case EC::PenRotateAngle:
    return "PenRotateAngle";
  case EC::PenTouchMode:
    return "PenTouchMode";
  case EC::PenGlobalPreventMode:
    return "PenGlobalPreventMode";
  case EC::PenHolster:
    return "PenHolster";
  default:
    return "Unknown";
  }
}
} // namespace

// --------------- ToString helpers ---------------

const char *ToString(workerState s) noexcept {
  switch (s) {
  case workerState::suspend:
    return "suspend";
  case workerState::quit:
    return "quit";
  case workerState::ready:
    return "ready";
  case workerState::streaming:
    return "streaming";
  case workerState::recover:
    return "recover";
  default:
    return "unknown";
  }
}

const char *ToString(CommandSource s) noexcept {
  switch (s) {
  case CommandSource::External:
    return "External";
  case CommandSource::SystemPolicy:
    return "SystemPolicy";
  default:
    return "Unknown";
  }
}

const char *ToString(RuntimePolicyEvent::Type type) noexcept {
  switch (type) {
  case RuntimePolicyEvent::Type::DisplayOn:
    return "DisplayOn";
  case RuntimePolicyEvent::Type::DisplayOff:
    return "DisplayOff";
  case RuntimePolicyEvent::Type::LidOn:
    return "LidOn";
  case RuntimePolicyEvent::Type::LidOff:
    return "LidOff";
  case RuntimePolicyEvent::Type::Suspend:
    return "Suspend";
  case RuntimePolicyEvent::Type::Shutdown:
    return "Shutdown";
  case RuntimePolicyEvent::Type::ResumeAutomatic:
    return "ResumeAutomatic";
  default:
    return "Unknown";
  }
}

// --------------- Lifecycle ---------------

DeviceRuntime::DeviceRuntime(const std::wstring &master,
                             const std::wstring &slave,
                             const std::wstring &interrupt)
    : m_chip(master, slave, interrupt) {
  m_vhfReporter.SetStylusPacketSensorRows(
      m_stylusPipeline.GetPacketSensorRows());
  m_vhfReporter.SetStylusPacketSensorCols(
      m_stylusPipeline.GetPacketSensorCols());
  m_vhfReporter.SetStylusPacketEmitWhenInvalid(
      m_stylusPipeline.GetEmitPacketWhenInvalid());
}

DeviceRuntime::~DeviceRuntime() { Stop(); }

bool DeviceRuntime::Start() {
  if (m_running.exchange(true))
    return false;
  m_stopReason.store(
      StopReason::None); // ← critical: clear stop reason for restart
  SetState(workerState::ready);
  m_needSuspendDeinit.store(false, std::memory_order_release);
  m_systemSuspendObserved.store(false, std::memory_order_release);
  m_recoverCount = 0;
  {
    std::lock_guard<std::mutex> lk(m_mu);
    m_displayOffSuspendPending = false;
  }
  m_lastNote = "Runtime started";
  m_thread = std::thread(&DeviceRuntime::WorkerMain, this);
  LOG_INFO("Runtime", __func__, "ready", "Worker thread launched.");
  return true;
}

void DeviceRuntime::Stop() {
  m_stopReason.store(StopReason::Shutdown);
  m_chip.CancelPendingFrameRead();
  if (m_thread.joinable())
    m_thread.join();
  m_running.store(false);
  SetState(workerState::quit);
  m_lastNote = "Runtime stopped";
}

DeviceRuntime::StartRequestResult DeviceRuntime::RequestStart() {
  if (IsRunning()) {
    return StartRequestResult::AlreadyRunning;
  }
  return Start() ? StartRequestResult::Started : StartRequestResult::Failed;
}

DeviceRuntime::StopRequestResult DeviceRuntime::RequestStop() {
  if (!IsRunning()) {
    return StopRequestResult::AlreadyStopped;
  }
  Stop();
  return StopRequestResult::Stopped;
}

void DeviceRuntime::ApplyServicePolicy(bool autoMode, bool stylusVhfEnabled,
                                       PenButtonMode penButtonMode,
                                       PenButtonRoute penButtonRoute,
                                       bool penButtonRouteExplicit) {
  SetAutoMode(autoMode);
  SetStylusVhfEnabled(stylusVhfEnabled);
  SetPenButtonMode(penButtonMode);
  SetPenButtonRoute(penButtonRoute, penButtonRouteExplicit);
  LOG_INFO(
      "Runtime", __func__, "Policy",
      "Applied: autoMode={} stylusVhfEnabled={} penBtnMode={} penBtnRoute={} penBtnRouteExplicit={}",
      autoMode, stylusVhfEnabled, static_cast<int>(penButtonMode),
      static_cast<int>(penButtonRoute), penButtonRouteExplicit);
}

bool DeviceRuntime::IsShutdownRequested() const {
  return m_stopReason.load() == StopReason::Shutdown;
}

#ifdef _DEBUG
void DeviceRuntime::SetFramePushCallback(DeviceRuntime::FramePushCallback cb) {
  std::lock_guard<std::mutex> lk(m_framePushCbMu);
  m_framePushCb = std::move(cb);
}
#endif

void DeviceRuntime::LoadPipelineConfig(const std::string &key,
                                       const std::string &value) {
  std::lock_guard<std::mutex> lk(m_pipelineMu);
  m_touchPipeline.LoadConfig(key, value);
}

void DeviceRuntime::LoadStylusPipelineConfig(const std::string &key,
                                             const std::string &value) {
  std::lock_guard<std::mutex> lk(m_pipelineMu);
  m_stylusPipeline.LoadConfig(key, value);
  m_vhfReporter.SetStylusPacketSensorRows(
      m_stylusPipeline.GetPacketSensorRows());
  m_vhfReporter.SetStylusPacketSensorCols(
      m_stylusPipeline.GetPacketSensorCols());
  m_vhfReporter.SetStylusPacketEmitWhenInvalid(
      m_stylusPipeline.GetEmitPacketWhenInvalid());
}

void DeviceRuntime::SavePipelineConfig(std::ostream &out) const {
  std::lock_guard<std::mutex> lk(m_pipelineMu);
  m_touchPipeline.SaveConfig(out);
}

void DeviceRuntime::SaveStylusPipelineConfig(std::ostream &out) const {
  std::lock_guard<std::mutex> lk(m_pipelineMu);
  m_stylusPipeline.SaveConfig(out);
}

void DeviceRuntime::SetVhfEnabled(bool enabled) {
  m_vhfReporter.SetEnabled(enabled);
}

bool DeviceRuntime::IsVhfEnabled() const {
  return m_vhfReporter.IsEnabled();
}

bool DeviceRuntime::IsVhfDeviceOpen() const {
  return m_vhfReporter.IsDeviceOpen();
}

void DeviceRuntime::SetVhfTransposeEnabled(bool enabled) {
  m_vhfReporter.SetTransposeEnabled(enabled);
}

bool DeviceRuntime::IsVhfTransposeEnabled() const {
  return m_vhfReporter.IsTransposeEnabled();
}

void DeviceRuntime::SetMasterParserOnlyMode(bool enabled) {
  const bool wasEnabled =
      m_masterParserOnly.exchange(enabled, std::memory_order_acq_rel);
  if (!wasEnabled && enabled) {
    m_vhfReporter.FlushTouchAllUp();
  }
}

void DeviceRuntime::IngestBtMcuPressure(uint16_t p) {
  std::lock_guard<std::mutex> lk(m_pipelineMu);
  m_stylusPipeline.SetBtMcuPressure(p);
}

void DeviceRuntime::IngestBtMcuPressurePacket(
    const std::array<uint16_t, 4> &pressure,
    const std::array<uint16_t, 4> &rawPressure, uint8_t freq1, uint8_t freq2) {
  std::lock_guard<std::mutex> lk(m_pipelineMu);
  m_stylusPipeline.SetBtMcuPressurePacket(pressure, rawPressure, freq1, freq2);
}

// --------------- 命令注入 ---------------

bool DeviceRuntime::SubmitExternalAfeCommand(AFE_Command type, uint8_t param) {
  if (!m_running.load(std::memory_order_acquire)) {
    return false;
  }

  command cmd{};
  cmd.type = type;
  cmd.param = param;
  SubmitCommand(cmd, CommandSource::External, "IPC AFE");
  return true;
}

uint64_t DeviceRuntime::SubmitCommand(command cmd, CommandSource src,
                                      const char *reason) {
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

void DeviceRuntime::IngestPolicyEvent(const RuntimePolicyEvent &ev) {
  using EventType = RuntimePolicyEvent::Type;

  const auto now = std::chrono::steady_clock::now();
  const bool isWakeEvent = ev.type == EventType::DisplayOn ||
                           ev.type == EventType::LidOn ||
                           ev.type == EventType::ResumeAutomatic;
  {
    std::lock_guard<std::mutex> lk(m_mu);
    const int key = static_cast<int>(ev.type);
    auto it = m_lastEventByType.find(key);
    if (it != m_lastEventByType.end() && now - it->second < kEventDebounce &&
        !(isWakeEvent && m_displayOffSuspendPending)) {
      return;
    }
    m_lastEventByType[key] = now;
  }

  switch (ev.type) {
  case EventType::DisplayOff: {
    std::lock_guard<std::mutex> lk(m_mu);
    m_displayOffSuspendPending = true;
    m_displayOffSuspendDeadline = now + kDisplayOffSuspendDelay;
  }
    LOG_INFO("Runtime", __func__, "Policy",
             "DisplayOff pending; suspend delayed by {} ms.",
             kDisplayOffSuspendDelay.count());
    m_chip.CancelPendingFrameRead();
    break;
  case EventType::LidOff: {
    std::lock_guard<std::mutex> lk(m_mu);
    m_displayOffSuspendPending = false;
  }
    LOG_INFO("Runtime", __func__, "Policy",
             "Sleep event ({}), requesting suspend.", ToString(ev.type));
    m_chip.CancelPendingFrameRead();
    m_stopReason.store(StopReason::ScreenOff);
    break;
  case EventType::Suspend: {
    {
      std::lock_guard<std::mutex> lk(m_mu);
      m_displayOffSuspendPending = false;
    }
    m_systemSuspendObserved.store(true, std::memory_order_release);
    LOG_INFO("Runtime", __func__, "Policy",
             "System suspend event, requesting immediate suspend.");
    m_chip.CancelPendingFrameRead();
    m_stopReason.store(StopReason::ScreenOff);
    break;
  }
  case EventType::DisplayOn:
  case EventType::LidOn:
  case EventType::ResumeAutomatic: {
    bool cancelledDisplayOff = false;
    {
      std::lock_guard<std::mutex> lk(m_mu);
      cancelledDisplayOff = m_displayOffSuspendPending;
      m_displayOffSuspendPending = false;
    }

    const bool resumedAfterSystemSuspend =
        m_systemSuspendObserved.exchange(false, std::memory_order_acq_rel);
    StopReason expected = StopReason::ScreenOff;
    const bool clearedScreenOff =
        m_stopReason.compare_exchange_strong(expected, StopReason::None);
    m_needSuspendDeinit.store(false, std::memory_order_release);

    LOG_INFO("Runtime", __func__, "Policy",
             "Wake event ({}), attempting resume.", ToString(ev.type));
    if (cancelledDisplayOff) {
      LOG_INFO("Runtime", __func__, "Policy",
               "Wake event cancelled pending DisplayOff suspend.");
    }
    if (clearedScreenOff) {
      LOG_INFO("Runtime", __func__, "Policy",
               "Wake event cleared pending ScreenOff stop reason.");
    }

    const workerState state = m_state.load(std::memory_order_acquire);
    if (IsRunning() &&
        (cancelledDisplayOff || clearedScreenOff || resumedAfterSystemSuspend ||
         state == workerState::suspend)) {
      std::lock_guard<std::mutex> lk(m_pipelineMu);
      m_touchPipeline.RequestBaselineReacquire();
    }
    if (state == workerState::suspend ||
        (resumedAfterSystemSuspend && IsRunning())) {
      m_chip.CancelPendingFrameRead();
      SetState(workerState::ready);
      LOG_INFO("Runtime", __func__, "Policy", "{}",
               resumedAfterSystemSuspend
                   ? "Resumed after system suspend -> ready for reinitialization."
                   : "Resumed from suspend -> ready (zero-cost wakeup).");
    } else if (IsRunning()) {
      LOG_INFO("Runtime", __func__, "Policy",
               "Runtime already active; wake event does not restart worker.");
    } else {
      LOG_INFO("Runtime", __func__, "Policy",
               "Wake event ignored because runtime is stopped.");
    }
  } break;
  case EventType::Shutdown:
    LOG_INFO("Runtime", __func__, "Policy",
             "Shutdown event, requesting termination.");
    m_stopReason.store(StopReason::Shutdown);
    break;
  default:
    break;
  }
}

// --------------- Pipe 查询 ---------------

RuntimeSnapshot DeviceRuntime::GetSnapshot() const {
  std::lock_guard<std::mutex> lk(m_mu);
  RuntimeSnapshot s;
  s.state = m_state.load();
  s.stylus_connected = m_chip.IsStylusConnected();
  s.recover_count = m_recoverCount;
  s.queue_depth = m_cmdQueue.size();
  s.last_command_id = m_lastCmdId;
  s.last_note = m_lastNote;
  return s;
}

std::vector<HistoryEntry> DeviceRuntime::GetHistory(std::size_t n) const {
  std::lock_guard<std::mutex> lk(m_mu);
  if (n >= m_history.size())
    return m_history;
  return {m_history.end() - static_cast<ptrdiff_t>(n), m_history.end()};
}

void DeviceRuntime::ClearHistory() {
  std::lock_guard<std::mutex> lk(m_mu);
  m_history.clear();
}

// --------------- 审计日志 ---------------

void DeviceRuntime::RecordHistory(const QueuedCommand &qc, bool ok,
                                  const std::string &det) {
  HistoryEntry e;
  e.timestamp = std::chrono::system_clock::now();
  e.command_id = qc.id;
  e.command_name = qc.reason;
  e.source = qc.source;
  e.success = ok;
  e.detail = det;
  m_history.push_back(std::move(e));
  if (m_history.size() > kMaxHistoryItems)
    m_history.erase(
        m_history.begin(),
        m_history.begin() +
            static_cast<ptrdiff_t>(m_history.size() - kMaxHistoryItems));
}

// --------------- 命令执行 ---------------

bool DeviceRuntime::DrainCommands() {
  while (true) {
    QueuedCommand qc{};
    {
      std::lock_guard<std::mutex> lk(m_mu);
      if (m_cmdQueue.empty()) {
        return true;
      }
      qc = m_cmdQueue.front();
      m_cmdQueue.pop_front();
      m_lastCmdId = qc.id;
    }

    const bool ok = static_cast<bool>(m_chip.SendAfeCommand(qc.cmd));
    {
      std::lock_guard<std::mutex> lk(m_mu);
      RecordHistory(qc, ok, ok ? "OK" : "afe_sendCommand failed");
    }
    if (!ok) {
      LOG_WARN("Runtime", __func__, "CmdExec",
               "Command '{}' (type={}) failed — skipping (non-fatal).",
               qc.reason, static_cast<int>(qc.cmd.type));
      // 不再触发 recover: AFE 命令失败不代表 bus 挂了
      continue;
    }
  }
}

// ----------- Worker 核心循环 -----------

ThreadResult DeviceRuntime::WorkerMain() {
  while (true) {
    bool displayOffSuspendDue = false;
    {
      std::lock_guard<std::mutex> lk(m_mu);
      if (m_displayOffSuspendPending &&
          std::chrono::steady_clock::now() >= m_displayOffSuspendDeadline &&
          m_stopReason.load(std::memory_order_acquire) !=
              StopReason::Shutdown) {
        m_displayOffSuspendPending = false;
        SetState(workerState::suspend);
        m_needSuspendDeinit.store(true, std::memory_order_release);
        m_cmdQueue.clear();
        displayOffSuspendDue = true;
      }
    }
    if (displayOffSuspendDue) {
      LOG_INFO("Runtime", __func__, "Policy",
               "DisplayOff remained active for {} ms; entering suspend.",
               kDisplayOffSuspendDelay.count());
      continue;
    }

    // ── 检查停止请求，根据 StopReason 分流到 suspend 或 quit ──
    auto reason =
        m_stopReason.exchange(StopReason::None, std::memory_order_acq_rel);
    if (reason != StopReason::None) {
      if (reason == StopReason::ScreenOff) {
        LOG_INFO("Runtime", __func__, "StopReq",
                 "StopReason::ScreenOff consumed -> suspend");
        SetState(workerState::suspend);
        m_needSuspendDeinit.store(true, std::memory_order_release);
      } else {
        LOG_INFO("Runtime", __func__, "StopReq",
                 "StopReason::Shutdown consumed -> quit");
        SetState(workerState::quit);
      }
      std::lock_guard<std::mutex> lk(m_mu);
      m_cmdQueue.clear();
    }

    DrainCommands();

    auto curState = m_state.load(std::memory_order_acquire);
    switch (curState) {
    case workerState::ready:
      OnReady();
      break;
    case workerState::streaming:
      OnStreaming();
      break;
    case workerState::recover:
      OnRecover();
      break;
    case workerState::suspend:
      OnSuspend();
      break;
    case workerState::quit:
      if (OnQuit()) {
        m_running.store(false); // allow restart via Start()
        LOG_INFO("Runtime", __func__, "quit",
                 "Worker exited, m_running=false.");
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
    {
      std::lock_guard<std::mutex> lk(m_pipelineMu);
      m_touchPipeline.RequestBaselineReacquire();
    }
    SetState(workerState::streaming);
  } else {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}

void DeviceRuntime::OnStreaming() {
  bool displayOffSuspendPending = false;
  {
    std::lock_guard<std::mutex> lk(m_mu);
    displayOffSuspendPending = m_displayOffSuspendPending;
  }
  if (displayOffSuspendPending) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return;
  }

  auto res = m_chip.GetFrame();
  if (res == std::unexpected(ChipError::Timeout))
    return;
  if (!res) {
    // 连续 N 帧非Timeout失败才进入 recover；
    // 某些 AFE 命令可能导致一两帧 bus 暂时失响应，不算致命。
    m_consecutiveFrameErrors++;
    if (m_consecutiveFrameErrors >= kMaxConsecutiveFrameErrors) {
      LOG_ERROR("Runtime", __func__, "Streaming",
                "{} consecutive GetFrame failures -> recover.",
                m_consecutiveFrameErrors);
      m_consecutiveFrameErrors = 0;
      SetState(workerState::recover);
    } else {
      LOG_WARN("Runtime", __func__, "Streaming",
               "GetFrame failed ({}/{}), retrying...", m_consecutiveFrameErrors,
               kMaxConsecutiveFrameErrors);
    }
    return;
  }
  m_consecutiveFrameErrors = 0; // 成功读帧重置计数

  const auto &rawData = m_chip.GetFrameBuffer();

  // 0. Build frame (zero-copy from Chip)
  Solvers::HeatmapFrame touchFrame;
  touchFrame.rawPtr = rawData.data();
  touchFrame.rawLen = Frame::kTotalFrameSize;
#if EGOTOUCH_DIAG
  // Debug/App 模式: 拷贝完整帧数据供 IPC 帧推送
  touchFrame.rawData.assign(rawData.begin(),
                            rawData.begin() + Frame::kTotalFrameSize);
#endif
  touchFrame.masterWasRead = m_chip.GetLastMasterWasRead();
  touchFrame.timestamp = m_chip.GetLastFrameTimestamp();

  const bool stylusVhfEnabled =
      m_stylusVhfEnabled.load(std::memory_order_relaxed);
  bool dispatchTouch = false;
  {
    std::lock_guard<std::mutex> lk(m_pipelineMu);

    // 3. Stylus pipeline — reads rawPtr, writes frame.stylus
    m_stylusPipeline.Process(touchFrame);

    // 4. Touch pipeline — reads frame, writes contacts/packets
    if (m_masterParserOnly.load(std::memory_order_relaxed)) {
      m_touchPipeline.ProcessMasterParserOnly(touchFrame);
    } else {
      m_touchPipeline.Process(touchFrame);
      dispatchTouch = true;
    }
  }

  // VHF dispatch can block on device I/O; keep it outside m_pipelineMu.
  // touchFrame owns processed stylus/contact vectors, and rawPtr references the
  // current chip frame buffer until the next GetFrame() on this worker thread.
  m_vhfReporter.DispatchStylus(touchFrame, stylusVhfEnabled);
  if (dispatchTouch) {
    m_vhfReporter.DispatchTouch(touchFrame);
  }

#ifdef _DEBUG
  // 5. Debug frame push (IPC visualization)
  // Hold callback mutex through invocation so disable() waits for in-flight
  // callback to finish.
  std::lock_guard<std::mutex> lk(m_framePushCbMu);
  if (m_framePushCb) {
    m_framePushCb(touchFrame);
  }
#endif
}

// --------------- MCU 事件路由 ---------------

void DeviceRuntime::HandlePenButtonStatusCode(uint8_t statusCode,
                                              uint8_t rawEventPayload,
                                              const char *source) {
  const auto mode = GetPenButtonMode();
  const auto route = GetPenButtonRoute();
  const auto plan = ResolvePenButtonRoute(
      mode, route, m_penButtonRouteExplicit.load(std::memory_order_acquire));

  bool vhfQueued = false;
  bool win32Ok = false;
  bool win32Attempted = false;

  if (plan.vhf) {
    switch (mode) {
    case PenButtonMode::OemCustom:
    case PenButtonMode::NativeBarrel:
      m_vhfReporter.SetBarrelButtonState(true);
      vhfQueued = true;
      break;
    case PenButtonMode::NativeEraser:
      // Status-code events are edge-like button gestures, not an eraser state
      // lifetime. Avoid latching VHF inverted/eraser bits; persistent VHF eraser
      // state is driven only by explicit EraserToggle events.
      break;
    }
  }

  if (plan.win32) {
    win32Attempted = true;
    switch (mode) {
    case PenButtonMode::OemCustom:
    case PenButtonMode::NativeBarrel:
      win32Ok = m_synthPenButton.InjectWinF22Shortcut();
      break;
    case PenButtonMode::NativeEraser: {
      POINT pt{};
      GetCursorPos(&pt);
      win32Ok = m_synthPenButton.InjectEraserPulse(pt);
      break;
    }
    }
  }

  LOG_INFO("Runtime", __func__, "MCU",
           "{}: statusCode={} raw={} mode={} route={} vhf={} win32={} win32_ok={}",
           source, statusCode, rawEventPayload, ToString(mode), ToString(route),
           vhfQueued ? 1 : 0, win32Attempted ? 1 : 0,
           win32Ok ? 1 : 0);
}

void DeviceRuntime::IngestPenEvent(const Himax::Pen::PenEvent &ev) {
  using EC = Himax::Pen::PenUsbEventCode;
  const uint8_t payload0 = PayloadByteOrZero(ev);

  if (Himax::Pen::FactoryStatusFlagsAffected(ev.code)) {
    uint16_t oldFlags = 0;
    uint16_t newFlags = 0;
    {
      std::lock_guard<std::mutex> lk(m_penStateMu);
      oldFlags = m_penState.factoryStatusFlags;
      newFlags =
          Himax::Pen::ApplyFactoryStatusFlagUpdate(oldFlags, ev.code, payload0);
      m_penState.factoryStatusFlags = newFlags;
    }
    if (oldFlags != newFlags) {
      LOG_INFO("Runtime", __func__, "MCU",
               "{} flags: 0x{:04X} -> 0x{:04X} (payload=0x{:02X})",
               FactoryStatusEventName(ev.code), oldFlags, newFlags, payload0);
    }
  }

  switch (ev.code) {

  case EC::PenConnStatus: {
    bool connected = false;
    {
      std::lock_guard<std::mutex> lk(m_penStateMu);
      m_penState.hasConnection = ev.semantic.hasConnection;
      m_penState.connected =
          ev.semantic.hasConnection ? ev.semantic.connected : false;
      connected = m_penState.hasConnection && m_penState.connected;
    }

    LOG_INFO("Runtime", __func__, "MCU", "PenConnStatus: {}.",
             connected ? "connected" : "disconnected");

    command cmd{};
    if (connected) {
      cmd.type = AFE_Command::InitStylus;
      cmd.param = 0;
      SubmitCommand(cmd, CommandSource::SystemPolicy, "PenConnStatus->Init");
    } else {
      cmd.type = AFE_Command::DisconnectStylus;
      cmd.param = 0;
      SubmitCommand(cmd, CommandSource::SystemPolicy,
                    "PenConnStatus->Disconnect");
    }
    break;
  }

  case EC::PenFreqJump: {
    std::string hexDump;
    for (size_t i = 0; i < ev.payload.size(); ++i) {
      char buf[8];
      snprintf(buf, sizeof(buf), "%02X ", ev.payload[i]);
      hexDump += buf;
    }
    LOG_INFO("Runtime", __func__, "MCU",
             "PenFreqJump ignored (payload[{}]: {})", ev.payload.size(),
             hexDump);
    break;
  }

  case EC::PenTypeInfo: {
    uint8_t penType = 0;
    {
      std::lock_guard<std::mutex> lk(m_penStateMu);
      m_penState.hasStylusId = ev.semantic.hasStylusId;
      m_penState.stylusId = ev.semantic.hasStylusId ? ev.semantic.stylusId : 0;
      penType = m_penState.stylusId;
    }

    LOG_INFO("Runtime", __func__, "MCU", "PenTypeInfo: pen_type={}.", penType);

    command cmd{};
    cmd.type = AFE_Command::SetStylusId;
    cmd.param = penType;
    SubmitCommand(cmd, CommandSource::SystemPolicy, "PenTypeInfo->SetStylusId");
    break;
  }

  case EC::PenCurStatus: {
    uint8_t modeRaw = 0;
    Himax::Pen::PenCurrentMode mode = Himax::Pen::PenCurrentMode::Unknown;
    {
      std::lock_guard<std::mutex> lk(m_penStateMu);
      m_penState.hasCurrentMode = ev.semantic.hasCurrentMode;
      m_penState.currentModeRaw =
          ev.semantic.hasCurrentMode ? ev.semantic.currentModeRaw : 0;
      m_penState.currentMode = ev.semantic.hasCurrentMode
                                   ? ev.semantic.currentMode
                                   : Himax::Pen::PenCurrentMode::Unknown;
      modeRaw = m_penState.currentModeRaw;
      mode = m_penState.currentMode;
    }

    LOG_INFO("Runtime", __func__, "MCU", "PenCurStatus: mode={} ({}).", modeRaw,
             Himax::Pen::ToString(mode));
    break;
  }

  case EC::PenCurrentFunc: {
    uint8_t func = 0;
    {
      std::lock_guard<std::mutex> lk(m_penStateMu);
      m_penState.hasCurrentFunc = ev.semantic.hasCurrentFunc;
      m_penState.currentFunc =
          ev.semantic.hasCurrentFunc ? ev.semantic.currentFunc : payload0;
      func = m_penState.currentFunc;
    }

    if (func == 1) {
      HandlePenButtonStatusCode(3, func, "PenCurrentFunc");
    } else {
      LOG_INFO("Runtime", __func__, "MCU",
               "PenCurrentFunc: func={} ignored by factory gate.", func);
    }
    break;
  }

  case EC::PenAcStatus:
  case EC::PenRotateAngle:
  case EC::PenTouchMode:
  case EC::PenGlobalPreventMode:
  case EC::PenHolster:
    LOG_INFO("Runtime", __func__, "MCU", "{}: payload=0x{:02X} flags updated.",
             FactoryStatusEventName(ev.code), payload0);
    break;

  case EC::PenGlobalAnnotation:
    HandlePenButtonStatusCode(4, payload0, "PenGlobalAnnotation");
    break;

  case EC::EraserToggle: {
    uint8_t eraserState = 0;
    {
      std::lock_guard<std::mutex> lk(m_penStateMu);
      m_penState.hasEraserToggle = ev.semantic.hasEraserToggle;
      m_penState.eraserToggle =
          ev.semantic.hasEraserToggle ? ev.semantic.eraserToggle : 0;
      eraserState = m_penState.eraserToggle;
    }

    const auto mode = GetPenButtonMode();
    const auto route = GetPenButtonRoute();
    const auto plan = ResolvePenButtonRoute(
        mode, route, m_penButtonRouteExplicit.load(std::memory_order_acquire));
    bool vhfQueued = false;
    bool win32Attempted = false;
    bool win32Ok = false;

    if (plan.vhf && (mode == PenButtonMode::OemCustom ||
                     mode == PenButtonMode::NativeEraser)) {
      m_vhfReporter.SetEraserState(eraserState);
      vhfQueued = true;
    }

    if (plan.win32 && eraserState != 0 && mode == PenButtonMode::NativeEraser) {
      POINT pt{};
      GetCursorPos(&pt);
      win32Attempted = true;
      win32Ok = m_synthPenButton.InjectEraserPulse(pt);
    }

    LOG_INFO("Runtime", __func__, "MCU",
             "EraserToggle: state={} mode={} route={} vhf={} win32={} win32_ok={}",
             eraserState, ToString(mode), ToString(route), vhfQueued ? 1 : 0,
             win32Attempted ? 1 : 0, win32Ok ? 1 : 0);
    break;
  }

  default:
    LOG_INFO("Runtime", __func__, "MCU", "MCU event 0x{:02X} received.",
             static_cast<uint8_t>(ev.code));
    break;
  }
}

bool DeviceRuntime::OnQuit() {
  if (m_autoMode.load()) {
    if (auto res = m_chip.Deinit(false); !res) {
      LOG_WARN("Runtime", __func__, "quit", "Chip deinit failed during quit.");
    }
  }
  return true; // signal WorkerMain to return
}

void DeviceRuntime::OnSuspend() {
  // 首次进入 suspend 时执行 HoldReset（拉低 reset，关闭中断通道）
  if (m_needSuspendDeinit.exchange(false, std::memory_order_acq_rel)) {
    m_chip.HoldReset();
    LOG_INFO("Runtime", __func__, "suspend",
             "Entered suspend, chip reset held low. Waiting for wake event.");
  }
  // 低功耗等待，每 100ms 检查一次状态变更
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void DeviceRuntime::OnRecover() {
  m_recoverCount++;

  // 最大重试 30 次（500ms 间隔 ≈ 15 秒恢复窗口）
  if (m_recoverCount > 30) {
    LOG_ERROR("Runtime", __func__, "Recover",
              "Exceeded 30 recovery attempts, entering suspend.");
    m_recoverCount = 0;
    m_needSuspendDeinit.store(true, std::memory_order_release);
    SetState(workerState::suspend);
    return;
  }

  // 等待 500ms 再重试，给硬件从休眠/灭屏恢复的时间
  // 期间每 50ms 检查一次 stop 请求以保持响应性
  for (int i = 0; i < 10; ++i) {
    if (m_stopReason.load(std::memory_order_relaxed) != StopReason::None)
      return;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  LOG_INFO("Runtime", __func__, "Recover", "Recovery attempt {}/30...",
           m_recoverCount);

  if (auto res = m_chip.check_bus(); !res)
    return;
  if (auto res = m_chip.Init(); !res)
    return;

  LOG_INFO("Runtime", __func__, "Recover",
           "Recovery succeeded after {} attempts.", m_recoverCount);
  {
    std::lock_guard<std::mutex> lk(m_pipelineMu);
    m_touchPipeline.RequestBaselineReacquire();
  }
  SetState(workerState::streaming);
  m_recoverCount = 0;
}

void DeviceRuntime::SetState(workerState newState) {
  workerState old = m_state.exchange(newState, std::memory_order_acq_rel);
  if (old != newState) {
    LOG_INFO("Runtime", __func__, "StateTransition", "State changed: {} -> {}",
             ToString(old), ToString(newState));
  }
}
