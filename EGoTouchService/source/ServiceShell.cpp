#include "ServiceShell.h"
#include "SystemStateMonitor.h"
#include "ServiceHost.h"
#include "Logger.h"

#include <string_view>
#include <powersetting.h>

namespace Service {

struct ServiceShell::Impl {
    SERVICE_STATUS_HANDLE statusHandle = nullptr;
    SERVICE_STATUS status{};
    HANDLE stopEvent = nullptr;
    ServiceHost host;

    // PBT power setting notification handles
    HPOWERNOTIFY hDisplayNotify = nullptr;
    HPOWERNOTIFY hLidNotify = nullptr;
    HPOWERNOTIFY hSuspendNotify = nullptr;
};

static ServiceShell s_instance;

ServiceShell::ServiceShell()
    : m_impl(std::make_unique<Impl>()) {}

ServiceShell::~ServiceShell() = default;

ServiceShell* ServiceShell::Instance() {
    return &s_instance;
}

// ─── SCM 模式 ────────────────────────────────

void WINAPI ServiceShell::SvcMain(DWORD argc, LPWSTR* argv) {
    auto* s = Instance();

    s->m_impl->statusHandle = RegisterServiceCtrlHandlerExW(
        kServiceName, SvcCtrlHandlerEx, s);
    if (!s->m_impl->statusHandle) {
        LOG_ERROR("Service", __func__, "Boot", "RegisterServiceCtrlHandlerExW failed.");
        return;
    }

    s->ReportStatus(SERVICE_START_PENDING, 3000);
    s->m_impl->stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

    LOG_INFO("Service", __func__, "Boot", "Starting modules...");
    if (!s->m_impl->host.Start()) {
        LOG_ERROR("Service", __func__, "Boot", "ServiceHost::Start() failed.");
        s->ReportStatus(SERVICE_STOPPED);
        return;
    }

    s->RegisterPowerNotifications();

    s->ReportStatus(SERVICE_RUNNING);
    LOG_INFO("Service", __func__, "Running", "Service is running. Waiting for stop signal...");
    s->WaitForStop();
    s->UnregisterPowerNotifications();
    s->m_impl->host.Stop();
    s->ReportStatus(SERVICE_STOPPED);
    LOG_INFO("Service", __func__, "Stopped", "Service stopped.");
}

DWORD WINAPI ServiceShell::SvcCtrlHandlerEx(
        DWORD ctrl, DWORD evtType, LPVOID evtData, LPVOID ctx) {
    auto* s = static_cast<ServiceShell*>(ctx);

    auto signalEvent = [](Host::SystemStateNamedEventId id) {
        return Host::SystemStateMonitor::SignalNamedEvent(id);
    };

    switch (ctrl) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
    case SERVICE_CONTROL_PRESHUTDOWN:
        LOG_INFO("Service", __func__, "Stopping", "Received stop/shutdown control code={}.", ctrl);
        s->SignalShutdownTransportAndStop();
        s->ReportStatus(SERVICE_STOP_PENDING, 5000);
        return NO_ERROR;

    case SERVICE_CONTROL_POWEREVENT: {
        // Handle PBT_APMRESUMEAUTOMATIC (system resumed from sleep)
        if (evtType == PBT_APMRESUMEAUTOMATIC) {
            LOG_INFO("Service", __func__, "Power", "PBT_APMRESUMEAUTOMATIC");
            signalEvent(Host::SystemStateNamedEventId::PbtApmResumeAutomatic);
            return NO_ERROR;
        }

        if (evtType != PBT_POWERSETTINGCHANGE || !evtData)
            return NO_ERROR;
        auto* pbs = static_cast<POWERBROADCAST_SETTING*>(evtData);

        // GUID_CONSOLE_DISPLAY_STATE: 0=off, 1=on, 2=dimmed
        static const GUID kDisplayGuid =
            {0x6fe69556, 0x704a, 0x47a0,
             {0x8f, 0x24, 0xc2, 0x8d, 0x93, 0x6f, 0xda, 0x47}};
        // GUID_LIDSWITCH_STATE_CHANGE
        static const GUID kLidGuid =
            {0xba3e0f4d, 0xb817, 0x4094,
             {0xa2, 0xd1, 0xd5, 0x63, 0x79, 0xe6, 0xa0, 0xf3}};

        if (pbs->PowerSetting == kDisplayGuid && pbs->DataLength >= 4) {
            DWORD state = *reinterpret_cast<DWORD*>(pbs->Data);
            LOG_INFO("Service", __func__, "Power", "GUID_CONSOLE_DISPLAY_STATE = {}", state);
            if (state >= 1) {
                signalEvent(Host::SystemStateNamedEventId::MonitorPowerOn);
                signalEvent(Host::SystemStateNamedEventId::MonitorConsoleDisplayOn);
            } else {
                signalEvent(Host::SystemStateNamedEventId::MonitorPowerOff);
                signalEvent(Host::SystemStateNamedEventId::MonitorConsoleDisplayOff);
            }
        }
        else if (pbs->PowerSetting == kLidGuid && pbs->DataLength >= 4) {
            DWORD state = *reinterpret_cast<DWORD*>(pbs->Data);
            LOG_INFO("Service", __func__, "Power", "GUID_LIDSWITCH_STATE = {} (1=open, 0=closed)", state);
            if (state == 1) {
                signalEvent(Host::SystemStateNamedEventId::MonitorLidOn);
            } else {
                signalEvent(Host::SystemStateNamedEventId::MonitorLidOff);
            }
        }
        return NO_ERROR;
    }

    case SERVICE_CONTROL_INTERROGATE:
        return NO_ERROR;
    default:
        return ERROR_CALL_NOT_IMPLEMENTED;
    }
}

// ─── 控制台模式 ──────────────────────────────

BOOL WINAPI ServiceShell::ConsoleCtrlHandler(DWORD ctrlType) {
    switch (ctrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        Instance()->SignalShutdownTransportAndStop();
        return TRUE;
    default:
        return FALSE;
    }
}

void ServiceShell::SignalShutdownTransportAndStop() noexcept {
    Host::SystemStateMonitor::SignalNamedEvent(Host::SystemStateNamedEventId::MonitorShutDown);
    if (m_impl != nullptr && m_impl->stopEvent != nullptr) {
        SetEvent(m_impl->stopEvent);
    }
}

void ServiceShell::RunAsConsole() {
    m_impl->stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

    SetConsoleCtrlHandler(&ServiceShell::ConsoleCtrlHandler, TRUE);

    LOG_INFO("Service", __func__, "Boot", "Starting modules (console mode)...");
    if (!m_impl->host.Start()) {
        LOG_ERROR("Service", __func__, "Boot", "ServiceHost::Start() failed.");
        return;
    }

    LOG_INFO("Service", __func__, "Running", "Service running in console mode. Press Ctrl+C to stop.");
    WaitForStop();
    m_impl->host.Stop();
    LOG_INFO("Service", __func__, "Stopped", "Console mode stopped.");
}

// ─── 辅助 ────────────────────────────────────

void ServiceShell::ReportStatus(DWORD state, DWORD waitHint) {
    if (!m_impl->statusHandle) return;

    m_impl->status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_impl->status.dwCurrentState = state;
    m_impl->status.dwWin32ExitCode = NO_ERROR;
    m_impl->status.dwWaitHint = waitHint;

    if (state == SERVICE_START_PENDING) {
        m_impl->status.dwControlsAccepted = 0;
    } else {
        m_impl->status.dwControlsAccepted =
            SERVICE_ACCEPT_STOP |
            SERVICE_ACCEPT_SHUTDOWN |
            SERVICE_ACCEPT_PRESHUTDOWN |
            SERVICE_ACCEPT_POWEREVENT;
    }

    static DWORD checkPoint = 1;
    if (state == SERVICE_RUNNING || state == SERVICE_STOPPED) {
        m_impl->status.dwCheckPoint = 0;
    } else {
        m_impl->status.dwCheckPoint = checkPoint++;
    }

    SetServiceStatus(m_impl->statusHandle, &m_impl->status);
}

void ServiceShell::WaitForStop() {
    if (m_impl->stopEvent) {
        WaitForSingleObject(m_impl->stopEvent, INFINITE);
        CloseHandle(m_impl->stopEvent);
        m_impl->stopEvent = nullptr;
    }
}

void ServiceShell::RegisterPowerNotifications() {
    if (!m_impl->statusHandle) return;

    // GUID_CONSOLE_DISPLAY_STATE
    static const GUID kDisplayGuid =
        {0x6fe69556, 0x704a, 0x47a0,
         {0x8f, 0x24, 0xc2, 0x8d, 0x93, 0x6f, 0xda, 0x47}};
    // GUID_LIDSWITCH_STATE_CHANGE
    static const GUID kLidGuid =
        {0xba3e0f4d, 0xb817, 0x4094,
         {0xa2, 0xd1, 0xd5, 0x63, 0x79, 0xe6, 0xa0, 0xf3}};
    // GUID_SYSTEM_AWAYMODE (away mode / connected standby)
    static const GUID kAwayGuid =
        {0x98a7f580, 0x01f7, 0x48aa,
         {0x9c, 0x0f, 0x44, 0x35, 0x2c, 0x29, 0xe5, 0xc0}};

    m_impl->hDisplayNotify = RegisterPowerSettingNotification(
        m_impl->statusHandle, &kDisplayGuid, DEVICE_NOTIFY_SERVICE_HANDLE);
    m_impl->hLidNotify = RegisterPowerSettingNotification(
        m_impl->statusHandle, &kLidGuid, DEVICE_NOTIFY_SERVICE_HANDLE);
    m_impl->hSuspendNotify = RegisterPowerSettingNotification(
        m_impl->statusHandle, &kAwayGuid, DEVICE_NOTIFY_SERVICE_HANDLE);

    LOG_INFO("Service", __func__, "Power", "Registered PBT notifications (display={}, lid={}, away={}).", m_impl->hDisplayNotify != nullptr, m_impl->hLidNotify != nullptr, m_impl->hSuspendNotify != nullptr);
}

void ServiceShell::UnregisterPowerNotifications() {
    if (m_impl->hDisplayNotify) {
        UnregisterPowerSettingNotification(m_impl->hDisplayNotify);
        m_impl->hDisplayNotify = nullptr;
    }
    if (m_impl->hLidNotify) {
        UnregisterPowerSettingNotification(m_impl->hLidNotify);
        m_impl->hLidNotify = nullptr;
    }
    if (m_impl->hSuspendNotify) {
        UnregisterPowerSettingNotification(m_impl->hSuspendNotify);
        m_impl->hSuspendNotify = nullptr;
    }
}

} // namespace Service

