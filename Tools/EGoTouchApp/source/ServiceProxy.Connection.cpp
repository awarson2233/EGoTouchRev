#include "ServiceProxy.h"
#include "Logger.h"

namespace App {

namespace {

ServiceWorkerState DecodeServiceWorkerState(int8_t value) noexcept {
    switch (value) {
    case -2: return ServiceWorkerState::Suspend;
    case -1: return ServiceWorkerState::Quit;
    case 0: return ServiceWorkerState::Ready;
    case 1: return ServiceWorkerState::Streaming;
    case 2: return ServiceWorkerState::Recover;
    default: return ServiceWorkerState::Unknown;
    }
}

} // namespace

ServiceProxy::ServiceProxy()
    : m_dvrBuffer(std::make_unique<RingBuffer<Dvr::DvrFrameSlot, kDvrCapacity>>()),
      m_dvrDynamicDebugBuffer(std::make_unique<RingBuffer<Dvr::DvrDynamicDebugFrameSlot, kDvrCapacity>>()) {
    // TouchPipeline is self-contained — no processor registration needed.
    InitConfigSchema();
    RefreshConfigSnapshot();
}

ServiceProxy::~ServiceProxy() {
    // Join any in-flight DVR export before tearing down resources
    if (m_dvrThread.joinable()) m_dvrThread.join();
    Disconnect();
}

ServiceRuntimeStatus ServiceProxy::GetServiceRuntimeStatus() const {
    std::lock_guard<std::mutex> lk(m_serviceRuntimeMutex);
    return m_serviceRuntimeStatus;
}

std::vector<ServiceRuntimeTransition> ServiceProxy::GetServiceRuntimeTransitions() const {
    std::lock_guard<std::mutex> lk(m_serviceRuntimeMutex);
    return m_serviceRuntimeTransitions;
}

void ServiceProxy::UpdateServiceRuntimeStatusFromSharedFrame(
    const Ipc::SharedFrameData& frame,
    uint64_t frameId,
    uint64_t appReceiveEpochUs) {
    const ServiceWorkerState nextState = DecodeServiceWorkerState(frame.workerState);

    std::lock_guard<std::mutex> lk(m_serviceRuntimeMutex);
    const ServiceWorkerState prevState = m_serviceRuntimeStatus.workerState;
    const bool hadPrevious = m_serviceRuntimeStatus.hasFrame;

    m_serviceRuntimeStatus.connected = true;
    m_serviceRuntimeStatus.hasFrame = true;
    m_serviceRuntimeStatus.workerState = nextState;
    m_serviceRuntimeStatus.streaming = frame.streaming;
    m_serviceRuntimeStatus.vhfEnabled = frame.vhfEnabled;
    m_serviceRuntimeStatus.vhfDeviceOpen = frame.vhfDeviceOpen;
    m_serviceRuntimeStatus.vhfTranspose = frame.vhfTranspose;
    m_serviceRuntimeStatus.frameId = frameId;
    m_serviceRuntimeStatus.serviceTimestamp = frame.timestamp;
    m_serviceRuntimeStatus.appReceiveEpochUs = appReceiveEpochUs;

    if (hadPrevious && prevState != nextState) {
        if (m_serviceRuntimeTransitions.size() >= kServiceRuntimeTransitionCapacity) {
            m_serviceRuntimeTransitions.erase(m_serviceRuntimeTransitions.begin());
        }
        m_serviceRuntimeTransitions.push_back(ServiceRuntimeTransition{
            .from = prevState,
            .to = nextState,
            .frameId = frameId,
            .serviceTimestamp = frame.timestamp,
            .appReceiveEpochUs = appReceiveEpochUs,
        });
    }
}

void ServiceProxy::UpdateServiceRuntimeStatusFromWire(
    const Ipc::RuntimeStatusWire& wire,
    uint64_t appReceiveEpochUs) {
    const ServiceWorkerState nextState = DecodeServiceWorkerState(wire.workerState);

    std::lock_guard<std::mutex> lk(m_serviceRuntimeMutex);
    const ServiceWorkerState prevState = m_serviceRuntimeStatus.workerState;
    const bool hadPrevious = m_serviceRuntimeStatus.hasFrame;

    m_serviceRuntimeStatus.connected = true;
    m_serviceRuntimeStatus.hasFrame = true;
    m_serviceRuntimeStatus.workerState = nextState;
    m_serviceRuntimeStatus.streaming = (wire.flags & Ipc::kRuntimeStatusStreaming) != 0;
    m_serviceRuntimeStatus.vhfEnabled = (wire.flags & Ipc::kRuntimeStatusVhfEnabled) != 0;
    m_serviceRuntimeStatus.vhfDeviceOpen = (wire.flags & Ipc::kRuntimeStatusVhfDeviceOpen) != 0;
    m_serviceRuntimeStatus.vhfTranspose = (wire.flags & Ipc::kRuntimeStatusVhfTranspose) != 0;
    m_serviceRuntimeStatus.recoverCount = wire.recoverCount;
    m_serviceRuntimeStatus.queueDepth = wire.queueDepth;
    m_serviceRuntimeStatus.lastCommandId = wire.lastCommandId;
    m_serviceRuntimeStatus.appReceiveEpochUs = appReceiveEpochUs;

    size_t noteLen = wire.lastNoteUtf8Len;
    if (noteLen > sizeof(wire.lastNoteUtf8)) {
        noteLen = sizeof(wire.lastNoteUtf8);
    }
    m_serviceRuntimeStatus.lastNote.assign(wire.lastNoteUtf8, wire.lastNoteUtf8 + noteLen);

    if (hadPrevious && prevState != nextState) {
        if (m_serviceRuntimeTransitions.size() >= kServiceRuntimeTransitionCapacity) {
            m_serviceRuntimeTransitions.erase(m_serviceRuntimeTransitions.begin());
        }
        m_serviceRuntimeTransitions.push_back(ServiceRuntimeTransition{
            .from = prevState,
            .to = nextState,
            .frameId = m_serviceRuntimeStatus.frameId,
            .serviceTimestamp = m_serviceRuntimeStatus.serviceTimestamp,
            .appReceiveEpochUs = appReceiveEpochUs,
        });
    }
}

void ServiceProxy::ResetServiceRuntimeStatus() {
    std::lock_guard<std::mutex> lk(m_serviceRuntimeMutex);
    m_serviceRuntimeStatus = ServiceRuntimeStatus{};
    m_serviceRuntimeTransitions.clear();
}

bool ServiceProxy::Connect() {
    std::lock_guard<std::mutex> lk(m_connectionMutex);
    if (m_client.IsConnected()) {
        return true;
    }
    if (m_pollThread.joinable()) {
        LOG_WARN("App", __func__, "IPC", "Connect requested while polling thread is still active; cleaning up stale connection state.");
        DisconnectLocked();
    }

    if (!m_frameReader.Open(kSharedMemName)) {
        LOG_ERROR("App", __func__, "IPC", "Failed to open shared memory (Service not running?).");
        DisconnectLocked();
        return false;
    }
    if (!m_client.Connect(3000)) {
        LOG_ERROR("App", __func__, "IPC", "Pipe connection failed.");
        DisconnectLocked();
        return false;
    }
    auto resp = m_client.EnterDebugMode(kSharedMemName);
    if (!resp.success) {
        LOG_ERROR("App", __func__, "IPC", "EnterDebugMode rejected.");
        DisconnectLocked();
        return false;
    }

    if (!SynchronizeConfigFromServiceForEditing()) {
        LOG_WARN("App", __func__, "Config", "Config v3 synchronization failed; parameter adjustment is disabled until retry.");
    }

    if (!RefreshDynamicDebugSchema()) {
        LOG_WARN("App", __func__, "IPC", "Dynamic debug schema unavailable; UI/export dynamic fields will be empty.");
    }
    m_logEvent = OpenEventW(SYNCHRONIZE, FALSE, Ipc::kLogReadyEventName);
    if (!m_logEvent) {
        LOG_WARN("App", __func__, "IPC", "OpenEvent failed for LogReadyEvent: {}", GetLastError());
    }
    m_penEvent = OpenEventW(SYNCHRONIZE, FALSE, Ipc::kPenReadyEventName);
    if (!m_penEvent) {
        LOG_WARN("App", __func__, "IPC", "OpenEvent failed for PenReadyEvent: {}", GetLastError());
    }
    m_pollStopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!m_pollStopEvent) {
        LOG_WARN("App", __func__, "IPC", "CreateEvent failed for PollStopEvent: {}", GetLastError());
        DisconnectLocked();
        return false;
    }

    m_polling.store(true);
    m_pollThread = std::thread(&ServiceProxy::PollLoop, this);

    LOG_INFO("App", __func__, "IPC", "Connected to EGoTouchService.");
    return true;
}

void ServiceProxy::DisconnectLocked() {
    m_polling.store(false);
    if (m_pollStopEvent) {
        SetEvent(m_pollStopEvent);
    }
    if (m_pollThread.joinable()) m_pollThread.join();
    if (m_pollStopEvent) {
        CloseHandle(m_pollStopEvent);
        m_pollStopEvent = nullptr;
    }
    if (m_logEvent) {
        CloseHandle(m_logEvent);
        m_logEvent = nullptr;
    }
    if (m_penEvent) {
        CloseHandle(m_penEvent);
        m_penEvent = nullptr;
    }

    if (m_client.IsConnected()) {
        m_client.ExitDebugMode();
        m_client.Disconnect();
    }
    m_frameReader.Close();
    ClearDynamicDebugState();
    ResetServiceRuntimeStatus();
    {
        std::lock_guard<std::mutex> lk(m_penMutex);
        m_penStatus = PenBridgeStatus{};
        m_penIdentityStatus = PenIdentityStatus{};
    }
    m_fps.store(0);
    m_slaveFps.store(0);
    SetConfigServiceSyncState(
        ConfigServiceSyncState::OfflineFallback,
        "Service is disconnected; config adjustment is disabled until current Service values are synchronized.");
}

void ServiceProxy::Disconnect() {
    std::lock_guard<std::mutex> lk(m_connectionMutex);
    DisconnectLocked();
    LOG_INFO("App", __func__, "IPC", "Disconnected.");
}

bool ServiceProxy::TryConnect() {
    return Connect();
}

bool ServiceProxy::IsConnected() const {
    std::lock_guard<std::mutex> lk(m_connectionMutex);
    return m_client.IsConnected();
}

bool ServiceProxy::SwitchAfeMode(uint8_t afeCmd, uint8_t param) {
    if (!IsLiveControlAllowed()) return false;
    auto resp = m_client.SendAfeCommand(afeCmd, param);
    return resp.success;
}

bool ServiceProxy::StartRemoteRuntime() {
    if (!IsLiveControlAllowed()) return false;
    const auto resp = m_client.StartRuntime();
    if (!resp.success) {
        LOG_WARN("App", __func__, "IPC", "StartRuntime request failed.");
    }
    return resp.success;
}

bool ServiceProxy::StopRemoteRuntime() {
    if (!IsLiveControlAllowed()) return false;
    const auto resp = m_client.StopRuntime();
    if (!resp.success) {
        LOG_WARN("App", __func__, "IPC", "StopRuntime request failed.");
    }
    return resp.success;
}

bool ServiceProxy::SetPenPressureMode(uint8_t mode) {
    if (!IsLiveControlAllowed()) return false;
    Ipc::IpcRequest req{};
    req.command = Ipc::IpcCommand::SetPenPressureMode;
    req.param[0] = mode == 0 ? 0 : 1;
    req.paramLen = 1;
    const auto resp = m_client.Send(req);
    if (!resp.success) {
        LOG_WARN("App", __func__, "IPC", "SetPenPressureMode request failed.");
    }
    return resp.success;
}

// ── VHF control ──
bool ServiceProxy::SetVhfEnabled(bool enabled) {
    if (!IsLiveControlAllowed()) return false;
    Ipc::IpcRequest req{};
    req.command = Ipc::IpcCommand::SetVhfEnabled;
    req.param[0] = enabled ? 1 : 0; req.paramLen = 1;
    bool ok = m_client.Send(req).success;
    if (ok) m_vhfEnabled.store(enabled);
    return ok;
}

bool ServiceProxy::SetVhfTranspose(bool enabled) {
    if (!IsLiveControlAllowed()) return false;
    Ipc::IpcRequest req{};
    req.command = Ipc::IpcCommand::SetVhfTranspose;
    req.param[0] = enabled ? 1 : 0; req.paramLen = 1;
    bool ok = m_client.Send(req).success;
    if (ok) m_vhfTranspose.store(enabled);
    return ok;
}

bool ServiceProxy::TriggerQueryHardwareVersion() {
    if (!IsLiveControlAllowed()) return false;
    Ipc::IpcRequest req{};
    req.command = Ipc::IpcCommand::TriggerQueryHardwareVersion;
    req.paramLen = 0;
    const auto resp = m_client.Send(req);
    return resp.success;
}

bool ServiceProxy::TriggerQueryPenStatus() {
    if (!IsLiveControlAllowed()) return false;
    Ipc::IpcRequest req{};
    req.command = Ipc::IpcCommand::TriggerQueryPenStatus;
    req.paramLen = 0;
    const auto resp = m_client.Send(req);
    return resp.success;
}

bool ServiceProxy::TriggerQueryPenInfo() {
    if (!IsLiveControlAllowed()) return false;
    Ipc::IpcRequest req{};
    req.command = Ipc::IpcCommand::TriggerQueryPenInfo;
    req.paramLen = 0;
    const auto resp = m_client.Send(req);
    return resp.success;
}

bool ServiceProxy::TriggerSendScanMode(uint8_t freq1, uint8_t freq2, uint8_t mode) {
    if (!IsLiveControlAllowed()) return false;
    const auto req = Ipc::MakeTriggerSendScanModeRequest(freq1, freq2, mode);
    const auto resp = m_client.Send(req);
    return resp.success;
}

bool ServiceProxy::TriggerSendFactoryInitParams() {
    if (!IsLiveControlAllowed()) return false;
    const auto req = Ipc::MakeTriggerSendFactoryInitParamsRequest();
    const auto resp = m_client.Send(req);
    return resp.success;
}

bool ServiceProxy::TriggerSendPairInfoSet(uint8_t value) {
    if (!IsLiveControlAllowed()) return false;
    Ipc::IpcRequest req{};
    req.command = Ipc::IpcCommand::TriggerSendPairInfoSet;
    req.param[0] = value;
    req.paramLen = 1;
    const auto resp = m_client.Send(req);
    return resp.success;
}

} // namespace App

