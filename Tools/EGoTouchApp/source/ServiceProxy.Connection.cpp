#include "ServiceProxyInternal.h"
#include "Logger.h"
#include <chrono>

namespace App {

ServiceProxy::ServiceProxy()
    : m_dvrBuffer(std::make_unique<RingBuffer<Dvr::DvrFrameSlot, kDvrCapacity>>()) {
    // TouchPipeline is self-contained — no processor registration needed.
    LoadConfig();
}

ServiceProxy::~ServiceProxy() {
    // Join any in-flight DVR export before tearing down resources
    if (m_dvrThread.joinable()) m_dvrThread.join();
    Disconnect();
}

bool ServiceProxy::Connect() {
    // 1. Open shared memory (Service owns the Global\\ mapping)
    if (!m_frameReader.Open(kSharedMemName)) {
        LOG_ERROR("App", __func__, "IPC", "Failed to open shared memory (Service not running?).");
        return false;
    }
    // 2. Connect pipe to Service
    if (!m_client.Connect(3000)) {
        LOG_ERROR("App", __func__, "IPC", "Pipe connection failed.");
        m_frameReader.Close();
        return false;
    }
    // 4. Tell Service to enter debug mode
    auto resp = m_client.EnterDebugMode(kSharedMemName);
    if (!resp.success) {
        LOG_ERROR("App", __func__, "IPC", "EnterDebugMode rejected.");
        m_client.Disconnect();
        m_frameReader.Close();
        return false;
    }

    LoadConfig();

    // 4.1 Pull dynamic debug schema (best effort)
    if (!RefreshDynamicDebugSchema()) {
        LOG_WARN("App", __func__, "IPC", "Dynamic debug schema unavailable; UI/export dynamic fields will be empty.");
    }
    if (!m_logEvent) {
        m_logEvent = OpenEventW(SYNCHRONIZE, FALSE, Ipc::kLogReadyEventName);
        if (!m_logEvent) {
            LOG_WARN("App", __func__, "IPC", "OpenEvent failed for LogReadyEvent: {}", GetLastError());
        }
    }
    if (!m_penEvent) {
        m_penEvent = OpenEventW(SYNCHRONIZE, FALSE, Ipc::kPenReadyEventName);
        if (!m_penEvent) {
            LOG_WARN("App", __func__, "IPC", "OpenEvent failed for PenReadyEvent: {}", GetLastError());
        }
    }
    if (!m_pollStopEvent) {
        m_pollStopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!m_pollStopEvent) {
            LOG_WARN("App", __func__, "IPC", "CreateEvent failed for PollStopEvent: {}", GetLastError());
        }
    }
    // 5. Start polling thread
    m_polling.store(true);
    m_pollThread = std::thread(&ServiceProxy::PollLoop, this);

    LOG_INFO("App", __func__, "IPC", "Connected to EGoTouchService.");
    return true;
}

void ServiceProxy::Disconnect() {
    // Stop polling
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

    // Tell Service to exit debug mode
    if (m_client.IsConnected()) {
        m_client.ExitDebugMode();
        m_client.Disconnect();
    }
    m_frameReader.Close();
    m_fps.store(0);
    m_slaveFps.store(0);
    LOG_INFO("App", __func__, "IPC", "Disconnected.");
}

bool ServiceProxy::TryConnect() {
    if (IsConnected()) return true;
    return Connect();
}

bool ServiceProxy::IsConnected() const {
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

} // namespace App
