#include "runtime/DeviceRuntime.h"
#include "Logger.h"
#include "SolverTypes.h"
#include "config/ConfigBinder.h"
#include "config/SchemaValidator.h"


#include <algorithm>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

namespace {
// 只灭屏（不合盖、不休眠）时保留链路的宽限期。宽限内亮屏且面板没断过电就直接继续取帧。
constexpr std::chrono::milliseconds kDisplayOffGrace{2000};
// 宽限期内读 ACPI 面板电源位的间隔
constexpr std::chrono::milliseconds kHoldPanelPollInterval{100};
// 宿主认为屏亮着、面板电源位却是断的时，等它回来的轮询间隔（原厂 WaitNotify 也是约 200 ms 查一次）
constexpr std::chrono::milliseconds kPanelPollInterval{200};
// 总线没就绪（面板 / 触摸部分没上电）时的探测间隔
constexpr std::chrono::milliseconds kBusPollInterval{500};
// 状态机心跳间隔
constexpr std::chrono::seconds kHeartbeatInterval{30};
// 连续建链失败这么多次后，连设备句柄一起重开
constexpr int kReopenAfterFailures = 5;
// 待机 / 手动模式下的空等上限（输入变化会立刻唤醒）
constexpr std::chrono::milliseconds kStandbyWait{1000};
constexpr std::chrono::milliseconds kManualModeWait{50};

// thp_afe_screen_on 在总线没准备好时先等 1.5 s 再重试；这里由短到长退避、封顶 2 s，不设次数上限
std::chrono::milliseconds BringUpBackoff(int failures) noexcept {
  static constexpr std::array<int, 5> kStepsMs{200, 500, 1000, 1500, 2000};
  const int idx = std::clamp(failures, 1, static_cast<int>(kStepsMs.size())) - 1;
  return std::chrono::milliseconds{kStepsMs[static_cast<std::size_t>(idx)]};
}

const char *StandbyReason(const PowerInputs &in) noexcept {
  if (in.systemSuspended) return "system suspended";
  if (!in.lidOpen) return "lid closed";
  if (!in.panelOn) return "panel power off";
  return "display off past grace period";
}

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

Solvers::StylusProtocolHint ResolveProtocolHintFromStylusId(uint8_t) noexcept {
  // PenTypeInfo is not a reliable HPP2/HPP3 discriminator.  Protocol selection
  // is driven by PenModule ModelId when available; otherwise the stylus pipeline
  // stays in Auto and resolves from packet shape.
  return Solvers::StylusProtocolHint::Auto;
}

void ResetPenTransientState(RuntimePenState &state) noexcept {
  state.hasCurrentMode = false;
  state.currentMode = Himax::Pen::PenCurrentMode::Unknown;
  state.currentModeRaw = 0;
  state.hasEraserToggle = false;
  state.eraserToggle = 0;
  state.hasCurrentFunc = false;
  state.currentFunc = 0;
}

bool ClearPenIdentityState(RuntimePenState &state) noexcept {
  const bool changed =
      state.hasStylusId || state.stylusId != 0 ||
      state.protocolHint != Solvers::StylusProtocolHint::Auto ||
      state.protocolHintFromPenModule || state.hasPenModuleModelId ||
      state.penModuleModelId != 0 ||
      state.penModuleModel != Himax::Pen::PenModuleModel::Unknown ||
      state.hasSerialNumber || !state.serialNumber.empty() ||
      state.hasHardwareVersion || !state.hardwareVersion.empty() ||
      state.hasFirmwareVersion || !state.firmwareVersion.empty();

  state.hasStylusId = false;
  state.stylusId = 0;
  state.protocolHint = Solvers::StylusProtocolHint::Auto;
  state.protocolHintFromPenModule = false;
  state.hasPenModuleModelId = false;
  state.penModuleModelId = 0;
  state.penModuleModel = Himax::Pen::PenModuleModel::Unknown;
  state.hasSerialNumber = false;
  state.serialNumber.clear();
  state.hasHardwareVersion = false;
  state.hardwareVersion.clear();
  state.hasFirmwareVersion = false;
  state.firmwareVersion.clear();
  return changed;
}

Solvers::StylusProtocolHint ResolveProtocolHintFromPenModule(
    Himax::Pen::PenModuleProtocolHint hint) noexcept {
  switch (hint) {
  case Himax::Pen::PenModuleProtocolHint::Hpp2:
    return Solvers::StylusProtocolHint::Hpp2;
  case Himax::Pen::PenModuleProtocolHint::Hpp3:
    return Solvers::StylusProtocolHint::Hpp3;
  default:
    return Solvers::StylusProtocolHint::Auto;
  }
}

const char *ToString(Solvers::StylusProtocolHint hint) noexcept {
  switch (hint) {
  case Solvers::StylusProtocolHint::Auto:
    return "Auto";
  case Solvers::StylusProtocolHint::Hpp2:
    return "Hpp2";
  case Solvers::StylusProtocolHint::Hpp3:
    return "Hpp3";
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
  case workerState::hold:
    return "hold";
  default:
    return "unknown";
  }
}

const char *ToString(PowerTarget t) noexcept {
  switch (t) {
  case PowerTarget::Quit:
    return "Quit";
  case PowerTarget::Standby:
    return "Standby";
  case PowerTarget::Hold:
    return "Hold";
  case PowerTarget::Active:
    return "Active";
  default:
    return "Unknown";
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

PenAfeCommandPlan BuildPenAfeCommandPlan(const RuntimePenState& state) noexcept {
  PenAfeCommandPlan plan{};
  if (!state.hasConnection || !state.connected) {
    return plan;
  }

  plan.commands[plan.count++] = command{AFE_Command::InitStylus, 0};
  if (state.hasStylusId) {
    plan.commands[plan.count++] =
        command{AFE_Command::SetStylusId, state.stylusId};
  }
  return plan;
}

std::optional<command> BuildPenConnectionAfeCommand(
    bool connectionChanged, bool connected) noexcept {
  if (!connectionChanged) {
    return std::nullopt;
  }
  return command{
      connected ? AFE_Command::InitStylus : AFE_Command::DisconnectStylus,
      0};
}

// --------------- Lifecycle ---------------

DeviceRuntime::DeviceRuntime(const std::wstring &master,
                             const std::wstring &slave,
                             const std::wstring &interrupt)
    : m_chip(master, slave), m_panelWatcher(interrupt) {
  m_vhfReporter.SetStylusPacketSensorRows(
      m_stylusPipeline.GetPacketSensorRows());
  m_vhfReporter.SetStylusPacketSensorCols(
      m_stylusPipeline.GetPacketSensorCols());
  m_vhfReporter.SetStylusPacketEmitWhenInvalid(
      m_stylusPipeline.GetEmitPacketWhenInvalid());
}

DeviceRuntime::~DeviceRuntime() { Stop(); }

bool DeviceRuntime::Start() {
  return StartStateMachine() == StartRequestResult::Started;
}

DeviceRuntime::StartRequestResult DeviceRuntime::StartStateMachine() {
  std::unique_lock<std::mutex> lifecycleLock(m_lifecycleMu);
  const auto caller = std::this_thread::get_id();
  if (m_workerThreadId == caller ||
      (m_thread.joinable() && m_thread.get_id() == caller)) {
    LOG_WARN("Runtime", __func__, "Thread",
             "Worker-thread start request rejected to avoid lifecycle self-deadlock.");
    return StartRequestResult::Failed;
  }

  m_lifecycleCv.wait(lifecycleLock, [this]() {
    return !m_stopInProgress && !m_selfStopDetached;
  });

  if (m_running.load(std::memory_order_acquire)) {
    bool lifecycleAccepting = false;
    bool shutdownRequested = false;
    {
      std::lock_guard<std::mutex> lk(m_mu);
      lifecycleAccepting = m_acceptExternalAfeCommands;
      shutdownRequested = m_inputs.shutdown;
    }
    const bool workerIsQuitting =
        !lifecycleAccepting ||
        m_state.load(std::memory_order_acquire) == workerState::quit ||
        shutdownRequested;
    if (!workerIsQuitting) {
      return StartRequestResult::AlreadyRunning;
    }
    if (!m_thread.joinable() ||
        m_thread.get_id() == std::this_thread::get_id()) {
      return StartRequestResult::Failed;
    }

    m_lifecycleCv.wait(lifecycleLock, [this]() {
      return !m_running.load(std::memory_order_acquire) &&
             m_workerThreadId == std::thread::id{};
    });
  }

  if (m_thread.joinable()) {
    if (m_thread.get_id() == std::this_thread::get_id()) {
      LOG_WARN("Runtime", __func__, "Thread",
               "Start() called from worker thread while previous worker is joinable.");
      return StartRequestResult::Failed;
    }
    m_thread.join();
  }

  // Initialize every new-generation state before publishing running/accepting.
  // IngestPolicyEvent uses m_lifecycleMu, so events linearized after publication
  // cannot be overwritten by startup resets.
  // 显示 / 合盖 / 休眠是宿主电平，跨越 Stop/Start 保留；只有终止请求属于上一代 worker。
  SetState(workerState::ready);
  m_link = LinkState{};

  {
    std::lock_guard<std::mutex> penIngressLock(m_penIngressMu);
    m_acceptPenAfeCommands = true;
    if (m_autoMode.load(std::memory_order_acquire)) {
      m_penReplay.BeginInitCycle();
    } else {
      if (m_penReplay.generation == 0) {
        m_penReplay.BeginInitCycle();
      }
      m_penReplay.CompleteInitCycle();
    }

    std::lock_guard<std::mutex> lk(m_mu);
    CancelQueuedCommandsLocked("cancelled: superseded by runtime start");
    m_inputs.shutdown = false;
    ++m_inputs.revision;
    m_seenRevision = m_inputs.revision;
    m_recoverCount = 0;
    m_lastNote = "Runtime started";
    m_acceptExternalAfeCommands = true;
    m_stopped.store(false, std::memory_order_release);
    m_running.store(true, std::memory_order_release);
  }

  m_thread = std::thread(&DeviceRuntime::WorkerMain, this);
  LOG_INFO("Runtime", __func__, "ready", "Worker thread launched.");
  return StartRequestResult::Started;
}

void DeviceRuntime::Stop() {
  {
    std::unique_lock<std::mutex> lifecycleLock(m_lifecycleMu);
    if (m_selfStopDetached && m_workerThreadId == std::this_thread::get_id()) {
      return;
    }

    if (m_stopInProgress) {
      if ((m_thread.joinable() &&
           m_thread.get_id() == std::this_thread::get_id()) ||
          m_workerThreadId == std::this_thread::get_id()) {
        return;
      }
      m_lifecycleCv.wait(lifecycleLock,
                         [this]() { return !m_stopInProgress; });
      if (!m_running.load(std::memory_order_acquire) &&
          !m_thread.joinable()) {
        return;
      }
    }

    {
      // Close both accepting gates before cancelling queued work. Pen state
      // snapshots remain writable while stopped.
      std::lock_guard<std::mutex> penIngressLock(m_penIngressMu);
      m_acceptPenAfeCommands = false;
      m_penReplay.CompleteInitCycle();

      std::lock_guard<std::mutex> lk(m_mu);
      m_acceptExternalAfeCommands = false;
      m_stopped.store(true, std::memory_order_release);
      CancelQueuedCommandsLocked("cancelled: runtime stop");
      m_inputs.shutdown = true;
      ++m_inputs.revision;
    }
    m_inputsCv.notify_all();

    if (!m_running.load(std::memory_order_acquire) && !m_thread.joinable() &&
        !m_selfStopDetached) {
      SetState(workerState::quit);
      std::lock_guard<std::mutex> lk(m_mu);
      m_lastNote = "Runtime stopped";
      return;
    }

    m_stopInProgress = true;
  }

  m_chip.CancelPendingFrameRead();

  std::unique_lock<std::mutex> lifecycleLock(m_lifecycleMu);
  if (m_thread.joinable()) {
    if (m_thread.get_id() == std::this_thread::get_id()) {
      m_selfStopDetached = true;
      m_thread.detach();
      m_stopInProgress = false;
      LOG_INFO("Runtime", __func__, "Thread",
               "Stop() called from worker thread; worker detached and will exit asynchronously.");
      m_lifecycleCv.notify_all();
      return;
    }

    // WorkerMain clears m_workerThreadId under m_lifecycleMu as its final
    // owner-state access. Do not hold this mutex while joining, or the worker
    // cannot publish completion and Stop() can deadlock.
    lifecycleLock.unlock();
    m_thread.join();
    lifecycleLock.lock();
  } else if (m_selfStopDetached) {
    // A detached self-stop worker may set m_running=false before it finishes
    // lifecycle cleanup. Wait for the final cleanup marker, not just !running,
    // so Stop()/~DeviceRuntime cannot return while the detached worker can
    // still access owner fields.
    m_lifecycleCv.wait(lifecycleLock, [this]() {
      return !m_selfStopDetached &&
             m_workerThreadId == std::thread::id{};
    });
  }
  m_running.store(false, std::memory_order_release);
  SetState(workerState::quit);
  {
    std::lock_guard<std::mutex> lk(m_mu);
    m_lastNote = "Runtime stopped";
  }
  m_stopInProgress = false;
  lifecycleLock.unlock();
  m_lifecycleCv.notify_all();
}

DeviceRuntime::StartRequestResult DeviceRuntime::RequestStart() {
  return StartStateMachine();
}

DeviceRuntime::StopRequestResult DeviceRuntime::RequestStop() {
  const bool wasRunning = IsRunning();
  Stop();
  return wasRunning ? StopRequestResult::Stopped
                    : StopRequestResult::AlreadyStopped;
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

#if EGOTOUCH_SERVICE_ENABLE_IPC
Config::ValidationResult DeviceRuntime::ValidateConfigStore(
    const Config::ConfigStore &store) const {
  std::lock_guard<std::mutex> lk(m_pipelineMu);

  // Build schema from a fresh pipeline instance so validation remains read-only
  // and cannot accidentally strip setters from the runtime apply path.
  Solvers::StylusPipeline stylusPipeline;
  Config::ConfigBinder binder;
  stylusPipeline.registerBindings(binder);
  return Config::SchemaValidator::validate(store, binder);
}

void DeviceRuntime::ApplyConfigStore(const Config::ConfigStore& store) {
  std::lock_guard<std::mutex> lk(m_pipelineMu);
  m_stylusPipeline.applyConfig(store);
  m_vhfReporter.SetStylusPacketSensorRows(m_stylusPipeline.GetPacketSensorRows());
  m_vhfReporter.SetStylusPacketSensorCols(m_stylusPipeline.GetPacketSensorCols());
  m_vhfReporter.SetStylusPacketEmitWhenInvalid(m_stylusPipeline.GetEmitPacketWhenInvalid());
  LOG_INFO("Runtime", __func__, "Config", "Applied ConfigStore to the stylus pipeline.");
}

#endif

bool DeviceRuntime::IsShutdownRequested() const {
  std::lock_guard<std::mutex> lk(m_mu);
  return m_inputs.shutdown;
}

PowerInputs DeviceRuntime::GetPowerInputs() const {
  std::lock_guard<std::mutex> lk(m_mu);
  return m_inputs;
}

#ifdef _DEBUG
void DeviceRuntime::SetFramePushCallback(DeviceRuntime::FramePushCallback cb) {
  std::lock_guard<std::mutex> lk(m_framePushCbMu);
  m_framePushCb = std::move(cb);
}
#endif

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
    std::lock_guard<std::mutex> lk(m_pipelineMu);
    m_vhfReporter.FlushTouchAllUp();
  }
}

void DeviceRuntime::IngestBtMcuPressure(uint16_t p) {
  std::lock_guard<std::mutex> lk(m_pipelineMu);
  m_stylusPipeline.SetBtMcuPressure(p);
  m_tsaCoreSolver.OnBtPressure();
}

void DeviceRuntime::IngestBtMcuPressurePacket(
    const std::array<uint16_t, 4> &pressure,
    const std::array<uint16_t, 4> &rawPressure, uint8_t freq1, uint8_t freq2) {
  std::lock_guard<std::mutex> lk(m_pipelineMu);
  m_stylusPipeline.SetBtMcuPressurePacket(pressure, rawPressure, freq1, freq2);
  // ApDaemon 收到压力包时同样通知 TSACore：它据此判断蓝牙笔是否还活着
  m_tsaCoreSolver.OnBtPressure();
}

void DeviceRuntime::ApplyPenStateToStylusPipeline() {
  Solvers::StylusPenSession session{};
  {
    std::lock_guard<std::mutex> lk(m_penStateMu);
    session.hasConnectionState = m_penState.hasConnection;
    session.connected = m_penState.connected;
    session.hasStylusId = m_penState.hasStylusId;
    session.stylusId = m_penState.stylusId;
    session.protocolHint = m_penState.protocolHint;
    session.revision = m_penState.pipelineRevision;
  }

  std::lock_guard<std::mutex> lk(m_pipelineMu);
  m_stylusPipeline.ApplyPenSession(session);
}

void DeviceRuntime::UpdatePenState(std::function<void(RuntimePenState&, PenStateUpdateResult&)> updateFn) {
  PenStateUpdateResult result{};
  bool applyToPipeline = false;
  {
    std::lock_guard<std::mutex> lk(m_penStateMu);
    updateFn(m_penState, result);

    const bool changed = result.stateChanged || result.stylusIdChanged;
    if (changed) {
      ++m_penState.penRevision;
      if (result.applyToPipeline) {
        ++m_penState.pipelineRevision;
        applyToPipeline = true;
      }
    }
  }

  if (applyToPipeline) {
    ApplyPenStateToStylusPipeline();
  }

  if (result.stylusIdChanged) {
    command cmd{};
    cmd.type = AFE_Command::SetStylusId;
    cmd.param = result.nextStylusId;
    SubmitPenAfeCommandLocked(cmd, "PenIdentityChange->SetStylusId");
  }
}

void DeviceRuntime::SubmitPenAfeCommandLocked(command cmd, const char* reason) {
  if (!m_acceptPenAfeCommands || m_penReplay.pending) {
    return;
  }
  EnqueueCommand(cmd, CommandSource::SystemPolicy, reason,
                 m_penReplay.generation);
}

void DeviceRuntime::BeginPenReplayInitCycle() {
  std::lock_guard<std::mutex> penIngressLock(m_penIngressMu);
  if (m_acceptPenAfeCommands && !m_penReplay.pending) {
    m_penReplay.BeginInitCycle();
  }
}

void DeviceRuntime::ReplayPenStateAfterChipInit() {
  std::lock_guard<std::mutex> penIngressLock(m_penIngressMu);
  if (!m_acceptPenAfeCommands || !m_penReplay.pending) {
    return;
  }

  RuntimePenState snapshot{};
  {
    std::lock_guard<std::mutex> penStateLock(m_penStateMu);
    snapshot = m_penState;
  }

  const auto plan = BuildPenAfeCommandPlan(snapshot);
  for (std::size_t i = 0; i < plan.count; ++i) {
    const char* reason = plan.commands[i].type == AFE_Command::InitStylus
        ? "ChipInitReplay->InitStylus"
        : "ChipInitReplay->SetStylusId";
    ExecuteCommand(MakeQueuedCommand(
        plan.commands[i], CommandSource::SystemPolicy, reason));
  }
  m_penReplay.CompleteInitCycle();
}

void DeviceRuntime::DispatchPenButtonAction(const PenButtonAction& action, const char* source) {
  const auto mode = GetPenButtonMode();
  const auto route = GetPenButtonRoute();
  const auto plan = ResolvePenButtonRoute(
      mode, route, m_penButtonRouteExplicit.load(std::memory_order_acquire));

  bool vhfQueued = false;
  bool win32Attempted = false;
  bool win32Ok = false;

  // 1. VHF route
  if (plan.vhf) {
    if (action.type == PenButtonAction::Type::Barrel) {
      if (mode == PenButtonMode::OemCustom || mode == PenButtonMode::NativeBarrel) {
        m_vhfReporter.SetBarrelButtonState(action.pressed);
        vhfQueued = true;
      }
    } else if (action.type == PenButtonAction::Type::Eraser) {
      if (mode == PenButtonMode::OemCustom || mode == PenButtonMode::NativeEraser) {
        m_vhfReporter.SetEraserState(action.pressed ? 1 : 0);
        vhfQueued = true;
      }
    }
  }

  // 2. Win32 route
  if (plan.win32) {
    win32Attempted = true;
    if (action.type == PenButtonAction::Type::Barrel) {
      if (action.pressed && (mode == PenButtonMode::OemCustom || mode == PenButtonMode::NativeBarrel)) {
        win32Ok = m_synthPenButton.InjectWinF22Shortcut();
      }
    } else if (action.type == PenButtonAction::Type::Eraser) {
      if (mode == PenButtonMode::NativeEraser) {
        POINT pt{};
        GetCursorPos(&pt);
        if (action.pressed) {
          win32Ok = m_synthPenButton.InjectEraserPulse(pt);
        }
      }
    }
  }

  LOG_INFO("Runtime", __func__, "MCU",
           "{}: action={} pressed={} mode={} route={} vhf={} win32={} win32_ok={}",
           source,
           action.type == PenButtonAction::Type::Barrel ? "Barrel" : "Eraser",
           action.pressed, ToString(mode), ToString(route),
           vhfQueued ? 1 : 0, win32Attempted ? 1 : 0, win32Ok ? 1 : 0);
}

// --------------- 命令注入 ---------------

bool DeviceRuntime::SubmitExternalAfeCommand(AFE_Command type, uint8_t param) {
  command cmd{};
  cmd.type = type;
  cmd.param = param;
  auto qc = MakeQueuedCommand(cmd, CommandSource::External, "IPC AFE");

  std::lock_guard<std::mutex> lk(m_mu);
  // Recheck while holding the queue lock used by Stop() to clear pending work.
  // This closes the check-then-enqueue race at the stopped boundary.
  if (!m_acceptExternalAfeCommands ||
      !m_running.load(std::memory_order_acquire) ||
      m_stopped.load(std::memory_order_acquire)) {
    return false;
  }
  if (!m_cmdQueue.push(std::move(qc))) {
    LOG_ERROR("Runtime", __func__, "Queue", "Command queue overflow! Command dropped.");
    return false;
  }
  return true;
}

DeviceRuntime::QueuedCommand DeviceRuntime::MakeQueuedCommand(
    command cmd, CommandSource src, const char* reason,
    uint64_t penGeneration) {
  QueuedCommand qc{};
  qc.id = m_nextCmdId.fetch_add(1, std::memory_order_relaxed);
  qc.penGeneration = penGeneration;
  qc.cmd = cmd;
  qc.source = src;
  qc.enqueued_at = std::chrono::steady_clock::now();
  if (reason) {
    std::strncpy(qc.reason.data(), reason, qc.reason.size() - 1);
  }
  return qc;
}

uint64_t DeviceRuntime::EnqueueCommand(command cmd, CommandSource src,
                                       const char* reason,
                                       uint64_t penGeneration) {
  auto qc = MakeQueuedCommand(cmd, src, reason, penGeneration);
  const uint64_t commandId = qc.id;
  {
    std::lock_guard<std::mutex> lk(m_mu);
    if (!m_cmdQueue.push(std::move(qc))) {
      LOG_ERROR("Runtime", __func__, "Queue", "Command queue overflow! Command dropped.");
    }
  }
  // IC 在 IDLE 时一次取帧会阻塞约 3 s，命令不等它：取消这次读，worker 回到循环顶上先执行命令
  m_chip.CancelPendingFrameRead();
  return commandId;
}

uint64_t DeviceRuntime::SubmitCommand(command cmd, CommandSource src,
                                      const char *reason) {
  return EnqueueCommand(cmd, src, reason);
}

void DeviceRuntime::IngestPolicyEvent(const RuntimePolicyEvent &ev) {
  // Establish a generation boundary with Start/Stop. Events that acquire this
  // lock after running is published cannot be overwritten by startup resets.
  std::unique_lock<std::mutex> lifecycleLock(m_lifecycleMu);
  using EventType = RuntimePolicyEvent::Type;

  if (ev.type != EventType::Shutdown) {
    bool acceptingCommands = false;
    bool shutdownRequested = false;
    {
      std::lock_guard<std::mutex> lk(m_mu);
      acceptingCommands = m_acceptExternalAfeCommands;
      shutdownRequested = m_inputs.shutdown;
    }
    if (m_stopInProgress || !acceptingCommands || shutdownRequested) {
      LOG_INFO("Runtime", __func__, "Policy",
               "Ignoring non-terminal event ({}) while termination is in progress.",
               ToString(ev.type));
      return;
    }
  }

  // 事件只改电平，不碰 worker 状态：重复事件幂等，乱序到达时以最后一次为准，
  // 什么时候静默、什么时候重新建链由 worker 根据电平决定。
  const auto now = std::chrono::steady_clock::now();
  switch (ev.type) {
  case EventType::DisplayOn:
    UpdateInputs("DisplayOn", [&](PowerInputs &in) {
      if (in.displayOn) return false;
      in.displayOn = true;
      in.displayOnAt = now;
      return true;
    }, false);
    break;
  case EventType::DisplayOff:
    UpdateInputs("DisplayOff", [&](PowerInputs &in) {
      ++in.downEpoch;
      if (in.displayOn) {
        in.displayOn = false;
        in.displayOffAt = now;
      }
      return true;
    }, true);
    break;
  case EventType::LidOn:
    UpdateInputs("LidOn", [&](PowerInputs &in) {
      if (in.lidOpen) return false;
      in.lidOpen = true;
      return true;
    }, false);
    break;
  case EventType::LidOff:
    UpdateInputs("LidOff", [&](PowerInputs &in) {
      ++in.downEpoch;
      in.lidOpen = false;
      return true;
    }, true);
    break;
  case EventType::Suspend:
    UpdateInputs("Suspend", [&](PowerInputs &in) {
      ++in.downEpoch;
      in.systemSuspended = true;
      return true;
    }, true);
    break;
  case EventType::ResumeAutomatic:
    // 系统唤醒只说明「休眠结束」：屏幕是否亮由显示电平决定，链路一律要重新证明
    UpdateInputs("ResumeAutomatic", [&](PowerInputs &in) {
      ++in.downEpoch;
      in.systemSuspended = false;
      return true;
    }, true);
    break;
  case EventType::Shutdown: {
    {
      std::lock_guard<std::mutex> penIngressLock(m_penIngressMu);
      m_acceptPenAfeCommands = false;
      m_penReplay.CompleteInitCycle();

      std::lock_guard<std::mutex> lk(m_mu);
      m_acceptExternalAfeCommands = false;
      CancelQueuedCommandsLocked("cancelled: shutdown requested");
    }
    UpdateInputs("Shutdown", [&](PowerInputs &in) {
      if (in.shutdown) return false;
      in.shutdown = true;
      return true;
    }, true);
    break;
  }
  default:
    break;
  }
}

void DeviceRuntime::UpdateInputs(const char *why,
                                 const std::function<bool(PowerInputs &)> &mutate,
                                 bool interruptFrameRead) {
  PowerInputs snapshot{};
  bool changed = false;
  {
    std::lock_guard<std::mutex> lk(m_mu);
    changed = mutate(m_inputs);
    if (changed) {
      ++m_inputs.revision;
    }
    snapshot = m_inputs;
  }
  LOG_NOTICE("PowerSM", "IngestPolicyEvent", "input", "{}: {}{}", why, DescribeInputs(snapshot),
             changed ? "" : " (unchanged)");
  if (!changed) {
    return;
  }
  m_inputsCv.notify_all();
  if (interruptFrameRead) {
    // 把可能阻塞在读帧里的 worker 叫醒，让它尽快按新电平行动
    m_chip.CancelPendingFrameRead();
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

RuntimePenState DeviceRuntime::GetPenStateSnapshot() const {
  std::lock_guard<std::mutex> lk(m_penStateMu);
  return m_penState;
}

PenAfeReplayState DeviceRuntime::GetPenAfeReplayStateSnapshot() const {
  std::lock_guard<std::mutex> penIngressLock(m_penIngressMu);
  return m_penReplay;
}

std::vector<HistoryEntry> DeviceRuntime::GetHistory(std::size_t n) const {
  std::lock_guard<std::mutex> lk(m_mu);
  std::vector<HistoryEntry> result;
  if (m_historyCount == 0) {
    return result;
  }
  const size_t count = std::min(n, m_historyCount);
  result.reserve(count);

  size_t startIdx = (m_historyWriteIdx + kMaxHistoryItems - count) % kMaxHistoryItems;
  for (size_t i = 0; i < count; ++i) {
    result.push_back(m_history[(startIdx + i) % kMaxHistoryItems]);
  }
  return result;
}

void DeviceRuntime::ClearHistory() {
  std::lock_guard<std::mutex> lk(m_mu);
  m_historyWriteIdx = 0;
  m_historyCount = 0;
  m_history.fill(HistoryEntry{});
}

void DeviceRuntime::SetWorkerHookForTesting(std::function<void()> hook) {
  std::lock_guard<std::mutex> lock(m_workerHookMu);
  m_workerHookForTesting = std::move(hook);
}

bool DeviceRuntime::IsAcceptingExternalAfeCommands() const {
  std::lock_guard<std::mutex> lk(m_mu);
  return m_acceptExternalAfeCommands;
}

// --------------- 审计日志 ---------------

void DeviceRuntime::CancelQueuedCommandsLocked(const char* detail) {
  QueuedCommand qc{};
  while (m_cmdQueue.pop(qc)) {
    m_lastCmdId = qc.id;
    RecordHistory(qc, false, detail ? detail : "cancelled");
  }
}

void DeviceRuntime::RecordHistory(const QueuedCommand &qc, bool ok,
                                  const std::string &det) {
  HistoryEntry e;
  e.timestamp = std::chrono::system_clock::now();
  e.command_id = qc.id;
  e.command_name = qc.reason;
  e.source = qc.source;
  e.success = ok;
  std::memset(e.detail.data(), 0, e.detail.size());
  std::strncpy(e.detail.data(), det.c_str(), e.detail.size() - 1);

  m_history[m_historyWriteIdx] = std::move(e);
  m_historyWriteIdx = (m_historyWriteIdx + 1) % kMaxHistoryItems;
  if (m_historyCount < kMaxHistoryItems) {
    m_historyCount++;
  }
}

// --------------- 命令执行 ---------------

bool DeviceRuntime::ShouldExecuteCommand(const QueuedCommand& qc) {
  if (qc.penGeneration == 0) {
    return true;
  }

  std::lock_guard<std::mutex> penIngressLock(m_penIngressMu);
  return m_acceptPenAfeCommands && m_penReplay.IsCurrent(qc.penGeneration);
}

bool DeviceRuntime::ExecuteCommand(const QueuedCommand& qc) {
  const bool ok = static_cast<bool>(m_chip.SendAfeCommand(qc.cmd));
  {
    std::lock_guard<std::mutex> lk(m_mu);
    m_lastCmdId = qc.id;
    RecordHistory(qc, ok, ok ? "OK" : "afe_sendCommand failed");
  }
  if (!ok) {
    LOG_WARN("Runtime", __func__, "CmdExec",
             "Command '{}' (type={}) failed — skipping (non-fatal).",
             qc.reason.data(), static_cast<int>(qc.cmd.type));
  }
  return ok;
}

bool DeviceRuntime::DrainCommands() {
  while (true) {
    QueuedCommand qc{};
    {
      std::lock_guard<std::mutex> lk(m_mu);
      if (!m_cmdQueue.pop(qc)) {
        return true;
      }
    }
    if (!ShouldExecuteCommand(qc)) {
      LOG_INFO("Runtime", __func__, "Queue",
               "Dropped stale pen command '{}' (generation={}).",
               qc.reason.data(), qc.penGeneration);
      continue;
    }
    ExecuteCommand(qc);
  }
}

// ----------- Worker 核心循环 -----------

ThreadResult DeviceRuntime::WorkerMain() {
  {
    std::lock_guard<std::mutex> lifecycleLock(m_lifecycleMu);
    m_workerThreadId = std::this_thread::get_id();
  }

  std::function<void()> workerHook;
  {
    std::lock_guard<std::mutex> hookLock(m_workerHookMu);
    workerHook = std::move(m_workerHookForTesting);
  }
  if (workerHook) {
    workerHook();
  }

  // 面板电源通道跟着 worker 的生命周期走
  m_panelWatcher.Start([this](bool panelOn, uint8_t) { NotePanelPower(panelOn, "interrupt"); });

  while (true) {
    PowerInputs in{};
    {
      std::lock_guard<std::mutex> lk(m_mu);
      in = m_inputs;
      m_seenRevision = in.revision;
    }
    const PowerTarget target = ComputeTarget(in, std::chrono::steady_clock::now());
    MaybeHeartbeat(target, in);

    if (target == PowerTarget::Quit) {
      m_panelWatcher.Stop();
      {
        std::lock_guard<std::mutex> penIngressLock(m_penIngressMu);
        m_acceptPenAfeCommands = false;
        m_penReplay.CompleteInitCycle();
      }
      m_stopped.store(true, std::memory_order_release);
      {
        std::lock_guard<std::mutex> lk(m_mu);
        CancelQueuedCommandsLocked("cancelled: shutdown consumed");
      }
      OnQuit();
      LOG_INFO("Runtime", __func__, "quit", "Worker exited, m_running=false.");
      {
        std::lock_guard<std::mutex> lifecycleLock(m_lifecycleMu);
        m_workerThreadId = std::thread::id{};
        m_selfStopDetached = false;
        m_running.store(false, std::memory_order_release); // allow restart via Start()
        m_lifecycleCv.notify_all();
      }
      return ThreadResult();
    }

    DrainCommands();

    switch (target) {
    case PowerTarget::Standby:
      if (m_state.load(std::memory_order_acquire) != workerState::suspend) {
        EnterStandby(StandbyReason(in));
      }
      StepStandby(in);
      break;
    case PowerTarget::Hold:
      StepHold(in);
      break;
    case PowerTarget::Active:
      StepActive(in);
      break;
    default:
      break;
    }
  }
}

PowerTarget DeviceRuntime::ComputeTarget(
    const PowerInputs &in, std::chrono::steady_clock::time_point now) const {
  if (in.shutdown) return PowerTarget::Quit;
  if (in.systemSuspended || !in.lidOpen || !in.panelOn) return PowerTarget::Standby;
  if (!in.displayOn) {
    return now - in.displayOffAt < kDisplayOffGrace ? PowerTarget::Hold
                                                    : PowerTarget::Standby;
  }
  return PowerTarget::Active;
}

bool DeviceRuntime::WaitForInputs(std::chrono::milliseconds timeout) {
  std::unique_lock<std::mutex> lk(m_mu);
  return m_inputsCv.wait_for(lk, timeout, [this]() {
    return m_inputs.revision != m_seenRevision;
  });
}

// ----------- 阶段处理 -----------

void DeviceRuntime::StepStandby(const PowerInputs &in) {
  if (!in.systemSuspended && in.lidOpen && in.displayOn) {
    // 只差面板电源位：宿主认为屏是亮的，等它回来。中断通知是主路，这里的轮询保证漏掉通知也能恢复；
    // 读不到时按已上电处理，交给建链阶段的总线探测去判断。
    NotePanelPower(m_chip.ReadPanelPower().value_or(true), "standby poll");
    (void)WaitForInputs(kPanelPollInterval);
    return;
  }
  // 其余的待机不碰硬件（读 ACPI 也会让 SPB 控制器保持唤醒），只等宿主电平变化
  (void)WaitForInputs(kStandbyWait);
}

void DeviceRuntime::StepHold(const PowerInputs &in) {
  switch (m_state.load(std::memory_order_acquire)) {
  case workerState::streaming:
    LeaveStreaming("display off");
    m_link.holdPanelDrops = in.panelDrops;
    TransitionTo(workerState::hold, "display off; link kept during grace period");
    return;
  case workerState::hold:
    // 宽限期内盯着面板电源：一旦断电（或读不到）就不再保留链路，直接待机，亮屏后重新建链
    NotePanelPower(m_chip.ReadPanelPower().value_or(false), "hold poll");
    (void)WaitForInputs(kHoldPanelPollInterval);
    return;
  default:
    // 没有在取帧（建链中 / 待机）：灭屏时不建链，等宽限期结束进入待机或重新亮屏
    (void)WaitForInputs(kHoldPanelPollInterval);
    return;
  }
}

void DeviceRuntime::StepActive(const PowerInputs &in) {
  switch (m_state.load(std::memory_order_acquire)) {
  case workerState::streaming:
    if (in.downEpoch != m_link.provenEpoch) {
      // 取帧期间收到过可能断电的事件（例如被合并掉的灭屏 / 系统唤醒），链路要重新证明
      BeginBringUp("power event while streaming", workerState::ready);
      return;
    }
    OnStreaming();
    return;
  case workerState::hold:
    if (in.panelDrops != m_link.holdPanelDrops) {
      BeginBringUp("panel lost power during display-off grace", workerState::ready);
      return;
    }
    m_link.provenEpoch = in.downEpoch;
    TransitionTo(workerState::streaming, "display back on within grace; panel stayed powered");
    return;
  case workerState::suspend:
    BeginBringUp("host reports panel on", workerState::ready);
    return;
  default:
    StepBringUp(in);
    return;
  }
}

void DeviceRuntime::StepBringUp(const PowerInputs &in) {
  if (!m_autoMode.load(std::memory_order_acquire)) {
    // 手动模式：不碰芯片，由外部命令驱动
    (void)WaitForInputs(kManualModeWait);
    return;
  }

  const auto now = std::chrono::steady_clock::now();
  if (now < m_link.nextAttemptAt) {
    (void)WaitForInputs(std::chrono::duration_cast<std::chrono::milliseconds>(
        m_link.nextAttemptAt - now) + std::chrono::milliseconds{1});
    return;
  }

  // 1. 总线就绪（himax_thp_drv!checkBusReady）：寄存器 0x13 两路都非零。面板 / 触摸部分
  //    没上电时 IOCTL 照样成功、读到 0；这时做 Init 只会在 hx_set_N_frame 失败，还要白拉一次 reset。
  //    所以只探测、不建链，等它上电。
  const auto probe = m_chip.ProbeBus();
  const auto panel = m_chip.ReadPanelPower();
  NoteLinkProbe(probe, panel, "bring-up");
  NotePanelPower(panel, "bring-up");
  if (probe.ioOk && !probe.Ready()) {
    if (m_link.busWaitSince == std::chrono::steady_clock::time_point{}) {
      m_link.busWaitSince = now;
    }
    (void)WaitForInputs(kBusPollInterval);
    return;
  }
  m_link.busWaitSince = {};

  // 2. 从静默点重新建链；连续失败到一定次数时连设备句柄一起重开（对应原厂整只守护重启）
  m_chip.Quiesce();
  if (m_link.bringUpAttempts > 0 && m_link.bringUpAttempts % kReopenAfterFailures == 0) {
    (void)m_chip.ReopenDevices();
  }
  ChipResult<> res{};
  const char *failStep = "";
  if (!probe.ioOk) {
    res = std::unexpected(ChipError::CommunicationError);
    failStep = "bus_ioctl";
  } else {
    BeginPenReplayInitCycle();
    res = m_chip.Init();
    failStep = m_chip.LastInitFailStep();
  }
  if (!res) {
    ++m_link.bringUpAttempts;
    const auto delay = BringUpBackoff(m_link.bringUpAttempts);
    m_link.nextAttemptAt = std::chrono::steady_clock::now() + delay;
    {
      std::lock_guard<std::mutex> lk(m_mu);
      m_recoverCount = static_cast<uint8_t>(std::min(m_link.bringUpAttempts, 255));
    }
    LOG_NOTICE("PowerSM", __func__, "ready", "Bring-up attempt {} failed at '{}' ({}, err={}); retrying in {} ms.",
               m_link.bringUpAttempts, failStep, ToString(res.error()), probe.nativeError, delay.count());
    if (m_state.load(std::memory_order_acquire) != workerState::recover) {
      TransitionTo(workerState::recover, "bring-up failed; backing off");
    }
    return;
  }

  EnterStreaming(in);
}

// ----------- 迁移与进入 / 离开动作 -----------

void DeviceRuntime::TransitionTo(workerState next, const char *reason) {
  const workerState prev = m_state.exchange(next, std::memory_order_acq_rel);
  if (prev == next) {
    return;
  }
  PowerInputs in{};
  {
    std::lock_guard<std::mutex> lk(m_mu);
    in = m_inputs;
    m_lastNote = std::string(ToString(next)) + ": " + reason;
  }
  LOG_NOTICE("PowerSM", __func__, ToString(next), "{} -> {} ({}) | {} proven={} attempts={}",
             ToString(prev), ToString(next), reason, DescribeInputs(in), m_link.provenEpoch,
             m_link.bringUpAttempts);
}

void DeviceRuntime::EnterStandby(const char *reason) {
  const workerState phase = m_state.load(std::memory_order_acquire);
  if (phase == workerState::streaming || phase == workerState::hold) {
    LeaveStreaming(reason);
  }
  // 对齐 thp_afe_screen_off：只停止取帧，不发总线命令、不拉 reset
  m_chip.Quiesce();
  m_link.bringUpAttempts = 0;
  m_link.nextAttemptAt = {};
  m_link.busWaitSince = {};
  {
    std::lock_guard<std::mutex> lk(m_mu);
    m_recoverCount = 0;
    CancelQueuedCommandsLocked("cancelled: standby");
  }
  TransitionTo(workerState::suspend, reason);
}

void DeviceRuntime::BeginBringUp(const char *reason, workerState phase) {
  const workerState prev = m_state.load(std::memory_order_acquire);
  if (prev == workerState::streaming || prev == workerState::hold) {
    LeaveStreaming(reason);
  }
  m_link.bringUpAttempts = 0;
  m_link.nextAttemptAt = {};
  m_link.busWaitSince = {};
  {
    // 重新建链后手写笔状态会整体重放，链路断开前排队的命令作废
    std::lock_guard<std::mutex> lk(m_mu);
    CancelQueuedCommandsLocked("cancelled: link re-initialization");
  }
  TransitionTo(phase, reason);
}

void DeviceRuntime::EnterStreaming(const PowerInputs &in) {
  ReplayPenStateAfterChipInit();
  {
    // 芯片刚重新初始化：闭环与输出记账归零（TSACore 的基线由芯片校准完成位触发重建）
    std::lock_guard<std::mutex> lk(m_pipelineMu);
    // 面板机型在第一次建链时从 OTP 读出；读不出或不认识时按缺省机型（原厂此时打不开 AFE）
    const auto panel = m_chip.GetPanelIdentity();
    const std::string_view projectId = Solvers::Tsa::ResolvePanelProjectId(panel ? panel->projectId : "");
    if (!panel || panel->projectId != projectId) {
      LOG_WARN("PowerSM", __func__, "panel", "Panel project id unavailable ({}), using {}.",
               panel ? panel->projectId : std::string("OTP read failed"), projectId);
    }
    if (projectId != m_tsaCoreSolver.PanelProjectId()) {
      if (m_tsaCoreSolver.SetPanelProjectId(projectId)) {
        LOG_INFO("PowerSM", __func__, "panel", "TSACore panel profile: {}.", projectId);
      } else {
        LOG_WARN("PowerSM", __func__, "panel", "Panel changed to {} after TSACore loaded {}; keeping it.",
                 projectId, m_tsaCoreSolver.PanelProjectId());
      }
    }
    m_stylusPipeline.SetPanelProjectId(projectId);
    m_tsaCoreSolver.OnAfeStarted();
  }
  m_link.provenEpoch = in.downEpoch;
  m_link.bringUpAttempts = 0;
  m_link.nextAttemptAt = {};
  m_link.busWaitSince = {};
  {
    std::lock_guard<std::mutex> lk(m_mu);
    m_recoverCount = 0;
  }
  TransitionTo(workerState::streaming, "chip initialized");
}

void DeviceRuntime::LeaveStreaming(const char *reason) {
  {
    std::lock_guard<std::mutex> lk(m_pipelineMu);
    m_tsaCoreSolver.OnAfeStopped();
  }
  // 不再有帧来驱动抬起：在场的手指与笔由这里统一收尾
  m_vhfReporter.FlushAllUp();
  LOG_NOTICE("PowerSM", __func__, "streaming", "Frame streaming stopped ({}); HID contacts released.", reason);
}

void DeviceRuntime::OnStreaming() {
  // IDLE 由 TSACore 请求；parser-only 模式下不解算，改由芯片按无输入帧计数自己进
  m_chip.SetAutoIdleEnabled(m_masterParserOnly.load(std::memory_order_relaxed));

  // 取帧不分 Normal / IDLE：IC 在 IDLE 里慢慢出帧，这一步就跟着慢；IDLE 帧照常进解算器，
  // TSACore 从帧尾状态位自己判断 IDLE（与 ApDaemon 一致，控制字里没有 IDLE 位）。
  const auto readStart = std::chrono::steady_clock::now();
  const auto read = m_chip.GetFrame();
  const auto readMs = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - readStart).count();
  m_hb.maxReadMs = std::max<int64_t>(m_hb.maxReadMs, readMs);
  if (read == Himax::Chip::FrameRead::NoData) {
    ++m_hb.noData;
    return;
  }
  if (read == Himax::Chip::FrameRead::Sos) {
    // 对应 ApDaemon 的 ProcessSos → ThpBase::Reset（afe Stop + afe Start）
    ++m_hb.sos;
    LOG_NOTICE("PowerSM", __func__, "streaming", "AFE SOS after {} ms: {}; restarting the link.", readMs,
               m_chip.LastSosReason());
    {
      // DriverThp::DoSos：重启前给 TSACore 置复位位，重启后的第一帧带上
      std::lock_guard<std::mutex> lk(m_pipelineMu);
      m_tsaCoreSolver.OnAfeSos();
    }
    BeginBringUp("AFE SOS", workerState::recover);
    return;
  }
  ++m_hb.frames;
  if (m_chip.GetAfeMode() == THP_AFE_MODE::Idle) {
    ++m_hb.idleFrames;
  }

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
  Solvers::Tsa::AfeActionList afeActions;
  {
    std::lock_guard<std::mutex> lk(m_pipelineMu);

    // 3. Stylus pipeline — reads rawPtr, writes frame.stylus
    m_stylusPipeline.Process(touchFrame);

    // 4. 热力图与 suffix 只在读到 master 的帧上解析（供诊断、IPC 与 DVR）。
    // 手指由 TSACore 解算：原版每一帧都调用它，跳过 master 的帧只跑笔相关的手指侧逻辑
    // （StylusAft 的抑制计数按调用次数走）。parser-only 模式只解析、不解算。
    if (touchFrame.masterWasRead) {
      m_frameParser.Process(touchFrame);
    }
    if (!m_masterParserOnly.load(std::memory_order_relaxed)) {
      m_tsaCoreSolver.SetStylusConnected(m_chip.IsStylusConnected());
      dispatchTouch = m_tsaCoreSolver.Process(touchFrame);
      afeActions = m_tsaCoreSolver.PendingAfeActions();
    }

    // Serialized re-check: if parser-only was enabled between pipeline
    // processing and this check, suppress touch dispatch. The lock
    // guarantees that SetMasterParserOnlyMode's FlushTouchAllUp()
    // either hasn't happened yet (touch→all-up = correct HID order)
    // or already happened (dispatch suppressed = correct).
    if (dispatchTouch && m_masterParserOnly.load(std::memory_order_relaxed)) {
      dispatchTouch = false;
    }
  }

  // VHF dispatch can block on device I/O; keep it outside m_pipelineMu.
  // touchFrame owns processed stylus/contact vectors, and rawPtr references the
  // current chip frame buffer until the next GetFrame() on this worker thread.
  m_vhfReporter.DispatchStylus(touchFrame, stylusVhfEnabled);
  if (dispatchTouch) {
    m_vhfReporter.DispatchTouch(touchFrame);
  }

  // 5. TSACore → AFE 闭环：本帧要下发的命令在 worker 上同步执行，结果回报给解算器
  ExecuteSolverAfeActions(afeActions);

#ifdef _DEBUG
  // 6. 逐帧回调（IPC 可视化、EGoTouchRecorder 录制），只在 Debug 构建里有。持锁调用，
  // 清除回调时会等正在进行的那一次结束。
  std::lock_guard<std::mutex> lk(m_framePushCbMu);
  if (m_framePushCb) {
    m_framePushCb(touchFrame);
  }
#endif
}

void DeviceRuntime::ExecuteSolverAfeActions(
    const Solvers::Tsa::AfeActionList &actions) {
  using Solvers::Tsa::AfeActionType;
  for (const auto &action : actions) {
    command cmd{};
    cmd.param = action.param;
    const char *reason = "";
    switch (action.type) {
    case AfeActionType::ClearStatus:
      cmd.type = AFE_Command::ClearStatus;
      reason = "TSACore->ClearStatus";
      break;
    case AfeActionType::ForceToFreqPoint:
      cmd.type = AFE_Command::ForceToFreqPoint;
      reason = "TSACore->ForceToFreqPoint";
      break;
    case AfeActionType::ForceToScanRate:
      cmd.type = AFE_Command::ForceToScanRate;
      reason = "TSACore->ForceToScanRate";
      break;
    case AfeActionType::EnterIdle:
      cmd.type = AFE_Command::EnterIdle;
      reason = "TSACore->EnterIdle";
      break;
    case AfeActionType::StartCalibration:
      cmd.type = AFE_Command::StartCalibration;
      reason = "TSACore->StartCalibration";
      break;
    }
    const bool ok = ExecuteCommand(
        MakeQueuedCommand(cmd, CommandSource::SystemPolicy, reason));
    std::lock_guard<std::mutex> lk(m_pipelineMu);
    m_tsaCoreSolver.OnAfeActionExecuted(action, ok);
  }
}

// --------------- MCU 事件路由 ---------------

void DeviceRuntime::HandlePenButtonStatusCode(uint8_t statusCode,
                                              uint8_t rawEventPayload,
                                              const char *source) {
  // statusCode 3 and 4 represent a barrel button action in this context
  PenButtonAction action{PenButtonAction::Type::Barrel, true, rawEventPayload};
  DispatchPenButtonAction(action, source);
}

bool DeviceRuntime::NotifyPenLinkDown() {
  // Mirror the PenConnStatus(disconnected) ingress path so the AFE, the stylus
  // pipeline and the replay plan all observe the same disconnected state.
  std::lock_guard<std::mutex> penIngressLock(m_penIngressMu);

  bool connectionChanged = false;
  UpdatePenState([&](RuntimePenState &state, PenStateUpdateResult &res) {
    const bool hadConnection = state.hasConnection;
    const bool oldConnected = state.connected;

    state.hasConnection = true;
    state.connected = false;

    connectionChanged =
        hadConnection != state.hasConnection || oldConnected != state.connected;
    res.stateChanged = connectionChanged;
    if (connectionChanged) {
      ResetPenTransientState(state);
    }

    const bool pipelineIdentityChanged =
        state.hasStylusId ||
        state.protocolHint != Solvers::StylusProtocolHint::Auto;
    res.stateChanged = ClearPenIdentityState(state) || res.stateChanged;
    res.applyToPipeline = connectionChanged || pipelineIdentityChanged;
  });

  if (const auto cmd = BuildPenConnectionAfeCommand(connectionChanged, false)) {
    SubmitPenAfeCommandLocked(*cmd, "HostPowerTransition->Disconnect");
  }

  LOG_INFO("Runtime", __func__, "Policy",
           "Pen link marked down by host power transition (connectionChanged={}).",
           connectionChanged);
  return connectionChanged;
}

void DeviceRuntime::NotifyPenLinkReset() {
  std::lock_guard<std::mutex> penIngressLock(m_penIngressMu);

  bool wasKnown = false;
  bool wasConnected = false;
  UpdatePenState([&](RuntimePenState &state, PenStateUpdateResult &res) {
    wasKnown = state.hasConnection;
    wasConnected = state.connected;

    state.hasConnection = false;
    state.connected = false;
    if (wasKnown) {
      ResetPenTransientState(state);
    }

    const bool pipelineIdentityChanged =
        state.hasStylusId ||
        state.protocolHint != Solvers::StylusProtocolHint::Auto;
    res.stateChanged = ClearPenIdentityState(state) || wasKnown;
    res.applyToPipeline = wasKnown || pipelineIdentityChanged;
  });

  if (wasKnown) {
    LOG_NOTICE("Runtime", __func__, "MCU",
               "Pen event channel reopened; connection state reset to unknown (was {}).",
               wasConnected ? "connected" : "disconnected");
  }
}

void DeviceRuntime::IngestPenEvent(const Himax::Pen::PenEvent &ev) {
  // Keep state mutation and any derived AFE submission on the same side of a
  // Stop/Start boundary. Stop still permits state updates after releasing this lock.
  std::lock_guard<std::mutex> penIngressLock(m_penIngressMu);
  using EC = Himax::Pen::PenUsbEventCode;
  const uint8_t payload0 = PayloadByteOrZero(ev);

  if (Himax::Pen::FactoryStatusFlagsAffected(ev.code)) {
    std::lock_guard<std::mutex> lk(m_penStateMu);
    m_penState.factoryStatusFlags = Himax::Pen::ApplyFactoryStatusFlagUpdate(
        m_penState.factoryStatusFlags, ev.code, payload0);
    // ApDaemon 的 DriverThp::GetStatus：状态字交给 TSACore（屏幕旋转、笔吸附 / 休眠、
    // 连接、防误触模式）
    m_tsaCoreSolver.SetPenStatusWord(m_penState.factoryStatusFlags);
  }

  switch (ev.code) {

  case EC::DevPairStatus: {
    if (!ev.semantic.hasPairStatus) {
      LOG_WARN("Runtime", __func__, "MCU",
               "DevPairStatus ignored because no valid pair status semantic was present.");
      break;
    }

    UpdatePenState([&](RuntimePenState& state, PenStateUpdateResult& res) {
      res.applyToPipeline = false;
      res.stateChanged = !state.hasPairStatus ||
                         state.pairStatus != ev.semantic.pairStatus;
      state.hasPairStatus = true;
      state.pairStatus = ev.semantic.pairStatus;
    });
    break;
  }

  case EC::PenModule: {
    if (!ev.semantic.hasPenModuleModelId) {
      LOG_WARN("Runtime", __func__, "MCU",
               "PenModule ignored because no valid ModelId semantic was present.");
      break;
    }

    UpdatePenState([&](RuntimePenState& state, PenStateUpdateResult& res) {
      const bool oldHasStylusId = state.hasStylusId;
      const uint8_t oldStylusId = state.stylusId;
      const auto oldProtocolHint = state.protocolHint;
      const Himax::Pen::PenModuleModelInfo modelInfo{
          ev.semantic.penModuleModelId,
          ev.semantic.penModuleModel,
          ev.semantic.hasPenModuleProtocolHint
              ? ev.semantic.penModuleProtocolHint
              : Himax::Pen::PenModuleProtocolHint::Auto,
          Himax::Pen::ToString(ev.semantic.penModuleModel)};

      const bool hasModuleHint =
          modelInfo.protocolHint != Himax::Pen::PenModuleProtocolHint::Auto;
      Solvers::StylusProtocolHint nextProtocolHint = hasModuleHint
          ? ResolveProtocolHintFromPenModule(modelInfo.protocolHint)
          : Solvers::StylusProtocolHint::Auto;
      if (!hasModuleHint && state.hasStylusId) {
        nextProtocolHint = ResolveProtocolHintFromStylusId(state.stylusId);
      }

      res.stateChanged = !state.hasPenModuleModelId ||
                       state.penModuleModelId != modelInfo.modelId ||
                       state.penModuleModel != modelInfo.model ||
                       state.protocolHintFromPenModule != hasModuleHint ||
                       state.protocolHint != nextProtocolHint;

      if (const auto derivedStylusId =
              Himax::Pen::TryResolveStylusIdFromPenModule(modelInfo.model);
          derivedStylusId && !state.hasStylusId) {
        state.hasStylusId = true;
        state.stylusId = *derivedStylusId;
        res.stylusIdChanged = true;
        res.nextStylusId = *derivedStylusId;
      }

      state.hasPenModuleModelId = true;
      state.penModuleModelId = modelInfo.modelId;
      state.penModuleModel = modelInfo.model;
      state.protocolHintFromPenModule = hasModuleHint;
      state.protocolHint = nextProtocolHint;
      res.applyToPipeline = oldHasStylusId != state.hasStylusId ||
                            oldStylusId != state.stylusId ||
                            oldProtocolHint != state.protocolHint;
    });
    break;
  }

  case EC::PenSerialNumber: {
    if (!ev.semantic.hasSerialNumber) {
      LOG_WARN("Runtime", __func__, "MCU",
               "PenSerialNumber ignored because no valid serial semantic was present.");
      break;
    }

    UpdatePenState([&](RuntimePenState& state, PenStateUpdateResult& res) {
      res.applyToPipeline = false;
      res.stateChanged = !state.hasSerialNumber ||
                         state.serialNumber != ev.semantic.serialNumber;
      state.hasSerialNumber = true;
      state.serialNumber = ev.semantic.serialNumber;
    });
    break;
  }

  case EC::PenHardwareVersion: {
    if (!ev.semantic.hasHardwareVersion) {
      LOG_WARN("Runtime", __func__, "MCU",
               "PenHardwareVersion ignored because no valid version semantic was present.");
      break;
    }

    UpdatePenState([&](RuntimePenState& state, PenStateUpdateResult& res) {
      res.applyToPipeline = false;
      res.stateChanged = !state.hasHardwareVersion ||
                         state.hardwareVersion != ev.semantic.hardwareVersion;
      state.hasHardwareVersion = true;
      state.hardwareVersion = ev.semantic.hardwareVersion;
    });
    break;
  }

  case EC::UsbdSwVersion: {
    if (!ev.semantic.hasFirmwareVersion) {
      LOG_WARN("Runtime", __func__, "MCU",
               "UsbdSwVersion ignored because no valid firmware semantic was present.");
      break;
    }

    UpdatePenState([&](RuntimePenState& state, PenStateUpdateResult& res) {
      const bool oldHasStylusId = state.hasStylusId;
      const uint8_t oldStylusId = state.stylusId;
      const auto oldProtocolHint = state.protocolHint;
      res.applyToPipeline = false;
      res.stateChanged = !state.hasFirmwareVersion ||
                         state.firmwareVersion != ev.semantic.firmwareVersion;
      state.hasFirmwareVersion = true;
      state.firmwareVersion = ev.semantic.firmwareVersion;

      const auto modelInfoFromFirmware =
          Himax::Pen::TryResolvePenModuleModelFromText(state.firmwareVersion);

      if (modelInfoFromFirmware && !state.hasPenModuleModelId) {
        const Himax::Pen::PenModuleModelInfo &modelInfo = *modelInfoFromFirmware;

        const bool hasModuleHint =
            modelInfo.protocolHint != Himax::Pen::PenModuleProtocolHint::Auto;
        Solvers::StylusProtocolHint nextProtocolHint = hasModuleHint
            ? ResolveProtocolHintFromPenModule(modelInfo.protocolHint)
            : Solvers::StylusProtocolHint::Auto;
        if (!hasModuleHint && state.hasStylusId) {
          nextProtocolHint = ResolveProtocolHintFromStylusId(state.stylusId);
        }

        res.stateChanged = res.stateChanged ||
                           !state.hasPenModuleModelId ||
                           state.penModuleModelId != modelInfo.modelId ||
                           state.penModuleModel != modelInfo.model ||
                           state.protocolHintFromPenModule != hasModuleHint ||
                           state.protocolHint != nextProtocolHint;

        if (const auto derivedStylusId =
                Himax::Pen::TryResolveStylusIdFromPenModule(modelInfo.model);
            derivedStylusId && !state.hasStylusId) {
          state.hasStylusId = true;
          state.stylusId = *derivedStylusId;
          res.stylusIdChanged = true;
          res.nextStylusId = *derivedStylusId;
        }

        state.hasPenModuleModelId = true;
        state.penModuleModelId = modelInfo.modelId;
        state.penModuleModel = modelInfo.model;
        state.protocolHintFromPenModule = hasModuleHint;
        state.protocolHint = nextProtocolHint;

        LOG_INFO("Runtime", __func__, "MCU",
                 "Derived PenModule ModelId from USBD_SW_VERSION: model={} id=0x{:06X} protocol={}",
                 modelInfoFromFirmware->name,
                 static_cast<unsigned int>(modelInfoFromFirmware->modelId),
                 Himax::Pen::ToString(modelInfoFromFirmware->protocolHint));
      }
      res.applyToPipeline = oldHasStylusId != state.hasStylusId ||
                            oldStylusId != state.stylusId ||
                            oldProtocolHint != state.protocolHint;
    });
    break;
  }

  case EC::PenConnStatus: {
    bool connected = false;
    bool connectionChanged = false;
    UpdatePenState([&](RuntimePenState& state, PenStateUpdateResult& res) {
      const bool hadConnection = state.hasConnection;
      const bool oldConnected = state.connected;

      state.hasConnection = ev.semantic.hasConnection;
      state.connected =
          ev.semantic.hasConnection ? ev.semantic.connected : false;
      connected = state.hasConnection && state.connected;

      connectionChanged =
          hadConnection != state.hasConnection || oldConnected != state.connected;
      res.stateChanged = connectionChanged;
      if (connectionChanged) {
        ResetPenTransientState(state);
      }

      if (!connected) {
        const bool pipelineIdentityChanged =
            state.hasStylusId ||
            state.protocolHint != Solvers::StylusProtocolHint::Auto;
        res.stateChanged = ClearPenIdentityState(state) || res.stateChanged;
        res.applyToPipeline = connectionChanged || pipelineIdentityChanged;
      } else if (!state.protocolHintFromPenModule && state.hasStylusId) {
        state.protocolHint = ResolveProtocolHintFromStylusId(state.stylusId);
      }
    });

    if (connectionChanged) {
      LOG_NOTICE("Runtime", __func__, "MCU", "PenConnStatus: {}.",
                 connected ? "connected" : "disconnected");
    }
    if (const auto cmd = BuildPenConnectionAfeCommand(
            connectionChanged, connected)) {
      SubmitPenAfeCommandLocked(
          *cmd,
          connected ? "PenConnStatus->Init" : "PenConnStatus->Disconnect");
    }
    break;
  }

  case EC::PenFreqJump: {
    break;
  }

  case EC::PenTypeInfo: {
    const bool hasStylusId = ev.semantic.hasStylusId;
    const uint8_t commandPenType = hasStylusId ? ev.semantic.stylusId : payload0;
    const uint8_t stateStylusId = hasStylusId ? commandPenType : 0;
    const auto fallbackProtocolHint = ResolveProtocolHintFromStylusId(stateStylusId);

    UpdatePenState([&](RuntimePenState& state, PenStateUpdateResult& res) {
      const bool protocolFromPenModule = state.protocolHintFromPenModule;
      res.stateChanged =
          state.hasStylusId != hasStylusId ||
          state.stylusId != stateStylusId ||
          (!protocolFromPenModule && state.protocolHint != fallbackProtocolHint);

      state.hasStylusId = hasStylusId;
      state.stylusId = stateStylusId;
      if (!protocolFromPenModule) {
        state.protocolHint = fallbackProtocolHint;
      }
    });

    command cmd{};
    cmd.type = AFE_Command::SetStylusId;
    cmd.param = commandPenType;
    SubmitPenAfeCommandLocked(cmd, "PenTypeInfo->SetStylusId");
    break;
  }

  case EC::PenCurStatus: {
    UpdatePenState([&](RuntimePenState& state, PenStateUpdateResult& res) {
      state.hasCurrentMode = ev.semantic.hasCurrentMode;
      state.currentModeRaw =
          ev.semantic.hasCurrentMode ? ev.semantic.currentModeRaw : 0;
      state.currentMode = ev.semantic.hasCurrentMode
                               ? ev.semantic.currentMode
                               : Himax::Pen::PenCurrentMode::Unknown;
    });
    break;
  }

  case EC::PenCurrentFunc: {
    uint8_t func = 0;
    UpdatePenState([&](RuntimePenState& state, PenStateUpdateResult& res) {
      state.hasCurrentFunc = ev.semantic.hasCurrentFunc;
      state.currentFunc =
          ev.semantic.hasCurrentFunc ? ev.semantic.currentFunc : payload0;
      func = state.currentFunc;
    });

    if (func == 1) {
      DispatchPenButtonAction({PenButtonAction::Type::Barrel, true, func}, "PenCurrentFunc");
    }
    break;
  }

  case EC::PenAcStatus:
  case EC::PenRotateAngle:
  case EC::PenTouchMode:
  case EC::PenGlobalPreventMode:
  case EC::PenHolster:
    break;

  case EC::PenGlobalAnnotation:
    DispatchPenButtonAction({PenButtonAction::Type::Barrel, true, payload0}, "PenGlobalAnnotation");
    break;

  case EC::EraserToggle: {
    uint8_t eraserState = 0;
    UpdatePenState([&](RuntimePenState& state, PenStateUpdateResult& res) {
      state.hasEraserToggle = ev.semantic.hasEraserToggle;
      state.eraserToggle =
          ev.semantic.hasEraserToggle ? ev.semantic.eraserToggle : 0;
      eraserState = state.eraserToggle;
    });

    DispatchPenButtonAction({PenButtonAction::Type::Eraser, eraserState != 0, eraserState}, "EraserToggle");
    break;
  }

  default:
    break;
  }
}

std::string DeviceRuntime::DescribeInputs(const PowerInputs &in) const {
  return MiniFmt::format("display={} panel={} lid={} suspended={} shutdown={} downEpoch={}",
                         in.displayOn ? "on" : "off", in.panelOn ? "on" : "off",
                         in.lidOpen ? "open" : "closed", in.systemSuspended ? 1 : 0,
                         in.shutdown ? 1 : 0, in.downEpoch);
}

void DeviceRuntime::NoteLinkProbe(const Himax::Chip::BusProbe &probe,
                                  std::optional<bool> panel, const char *where) {
  const int panelLevel = panel ? (*panel ? 1 : 0) : -1;
  if (m_link.probeLogged && m_link.lastIoOk == probe.ioOk &&
      m_link.lastBusMaster == probe.master && m_link.lastBusSlave == probe.slave &&
      m_link.lastPanel == panelLevel) {
    return;
  }
  m_link.probeLogged = true;
  m_link.lastIoOk = probe.ioOk;
  m_link.lastBusMaster = probe.master;
  m_link.lastBusSlave = probe.slave;
  m_link.lastPanel = panelLevel;
  LOG_NOTICE("PowerSM", __func__, where,
             "Link probe: bus {} (0x13 master=0x{:02X} slave=0x{:02X}, err={}), panel ACPI bit0={}.",
             !probe.ioOk ? "ioctl-failed" : (probe.Ready() ? "ready" : "not-ready"),
             probe.master, probe.slave, probe.nativeError,
             panelLevel < 0 ? "unreadable" : (panelLevel ? "1" : "0"));
}

void DeviceRuntime::NotePanelPower(std::optional<bool> panelOn, const char *source) {
  if (!panelOn) {
    return;
  }
  const bool on = *panelOn;
  {
    std::lock_guard<std::mutex> lk(m_mu);
    if (m_inputs.panelOn == on) {
      return;
    }
  }
  const std::string why = MiniFmt::format("Panel{} ({})", on ? "On" : "Off", source);
  // 面板断电时 IC 跟着掉电：把可能阻塞在读帧里的 worker 叫醒，链路要重新证明
  UpdateInputs(why.c_str(), [on](PowerInputs &in) {
    if (in.panelOn == on) return false;
    in.panelOn = on;
    if (!on) {
      ++in.panelDrops;
      ++in.downEpoch;
    }
    return true;
  }, !on);
}

void DeviceRuntime::MaybeHeartbeat(PowerTarget target, const PowerInputs &in) {
  const auto now = std::chrono::steady_clock::now();
  if (now < m_hb.next) {
    return;
  }
  const bool first = m_hb.next == std::chrono::steady_clock::time_point{};
  m_hb.next = now + kHeartbeatInterval;
  if (first) {
    return;
  }
  const workerState phase = m_state.load(std::memory_order_acquire);
  const long long busWaitS =
      m_link.busWaitSince == std::chrono::steady_clock::time_point{}
          ? 0
          : std::chrono::duration_cast<std::chrono::seconds>(now - m_link.busWaitSince).count();
  LOG_NOTICE("PowerSM", "Heartbeat", ToString(phase),
             "target={} | {} | frames={} (idle {}) noData={} sos={} maxRead={}ms afe={} panelIrq={} | attempts={} "
             "busWait={}s lastBus=0x{:02X}/0x{:02X} lastPanel={}",
             ToString(target), DescribeInputs(in), m_hb.frames, m_hb.idleFrames, m_hb.noData, m_hb.sos,
             m_hb.maxReadMs, m_chip.GetAfeMode() == THP_AFE_MODE::Idle ? "idle" : "normal",
             m_panelWatcher.Notifications(), m_link.bringUpAttempts, busWaitS, m_link.lastBusMaster,
             m_link.lastBusSlave, m_link.lastPanel);
  const auto next = m_hb.next;
  m_hb = {};
  m_hb.next = next;
}

bool DeviceRuntime::OnQuit() {
  const workerState prev = m_state.load(std::memory_order_acquire);
  if (prev == workerState::streaming || prev == workerState::hold) {
    LeaveStreaming("shutdown");
  }
  if (m_autoMode.load() &&
      (prev == workerState::streaming || prev == workerState::hold)) {
    // 链路还活着：正常关掉感测
    if (auto res = m_chip.Deinit(false); !res) {
      LOG_WARN("Runtime", __func__, "quit", "Chip deinit failed during quit.");
    }
  } else {
    m_chip.Quiesce();
  }
  TransitionTo(workerState::quit, "shutdown requested");
  return true;
}

void DeviceRuntime::SetState(workerState newState) {
  workerState old = m_state.exchange(newState, std::memory_order_acq_rel);
  if (old != newState) {
    LOG_INFO("Runtime", __func__, "StateTransition", "State changed: {} -> {}",
             ToString(old), ToString(newState));
  }
}
