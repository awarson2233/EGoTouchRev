#include "ServiceShell.h"
#include "SystemStateMonitor.h"
#include "Logger.h"

#include <string_view>
#include <powersetting.h>

namespace Service {

static ServiceShell s_instance;

ServiceShell* ServiceShell::Instance() {
    return &s_instance;
}

// ─── SCM 模式 ────────────────────────────────

void WINAPI ServiceShell::SvcMain(DWORD argc, LPWSTR* argv) {
    auto* s = Instance();

    s->m_statusHandle = RegisterServiceCtrlHandlerExW(
        kServiceName, SvcCtrlHandlerEx, s);
    if (!s->m_statusHandle) {
        LOG_ERROR("Service", __func__, "Boot", "RegisterServiceCtrlHandlerExW failed.");
        return;
    }

    s->ReportStatus(SERVICE_START_PENDING, 3000);
    s->m_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

    LOG_INFO("Service", __func__, "Boot", "Starting modules...");
    if (!s->m_host.Start()) {
        LOG_ERROR("Service", __func__, "Boot", "ServiceHost::Start() failed.");
        s->ReportStatus(SERVICE_STOPPED);
        return;
    }

    s->RegisterPowerNotifications();

    s->ReportStatus(SERVICE_RUNNING);
    LOG_INFO("Service", __func__, "Running", "Service is running. Waiting for stop signal...");
    s->WaitForStop();
    s->UnregisterPowerNotifications();
    s->m_host.Stop();
    s->ReportStatus(SERVICE_STOPPED);
    LOG_INFO("Service", __func__, "Stopped", "Service stopped.");
}

DWORD WINAPI ServiceShell::SvcCtrlHandlerEx(
        DWORD ctrl, DWORD evtType, LPVOID evtData, LPVOID ctx) {
    auto* s = static_cast<ServiceShell*>(ctx);

    // Helper: signal a named event by index
    auto signalEvent = [](std::size_t idx, std::string_view reason) {
        const auto& names = Host::SystemStateMonitor::NamedEventList();
        if (idx >= names.size()) {
            LOG_WARN("Service", __func__, "Power", "Skip signal for '{}' due to invalid event index={}", reason, idx);
            return;
        }
        HANDLE h = OpenEventW(EVENT_MODIFY_STATE, FALSE, names[idx]);
        if (!h) {
            LOG_WARN("Service", __func__, "Power", "OpenEvent failed for '{}' (index={}, err={})", reason, idx, GetLastError());
            return;
        }
        SetEvent(h);
        CloseHandle(h);
        LOG_INFO("Service", __func__, "Power", "Signaled named event '{}' (index={}).", reason, idx);
    };

    switch (ctrl) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
    case SERVICE_CONTROL_PRESHUTDOWN:
        LOG_INFO("Service", __func__, "Stopping", "Received stop/shutdown control code={}.", ctrl);
        // Signal MonitorShutDownEvent (index 6) so monitor thread sees it
        signalEvent(6, "Shutdown");
        s->ReportStatus(SERVICE_STOP_PENDING, 5000);
        SetEvent(s->m_stopEvent);
        return NO_ERROR;

    case SERVICE_CONTROL_POWEREVENT: {
        // GUID_CONSOLE_DISPLAY_STATE: 0=off, 1=on, 2=dimmed
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

        switch (evtType) {
        case PBT_APMSUSPEND:
            LOG_INFO("Service", __func__, "Power", "PBT_APMSUSPEND -> suspend path");
            signalEvent(1, "PBT_APMSUSPEND");
            return NO_ERROR;
        case PBT_APMRESUMEAUTOMATIC:
            LOG_INFO("Service", __func__, "Power", "PBT_APMRESUMEAUTOMATIC -> wake path");
            signalEvent(7, "PBT_APMRESUMEAUTOMATIC");
            return NO_ERROR;
        case PBT_APMRESUMESUSPEND:
            LOG_INFO("Service", __func__, "Power", "PBT_APMRESUMESUSPEND -> wake path");
            signalEvent(7, "PBT_APMRESUMESUSPEND");
            return NO_ERROR;
        case PBT_APMRESUMECRITICAL:
            LOG_INFO("Service", __func__, "Power", "PBT_APMRESUMECRITICAL -> wake path");
            signalEvent(7, "PBT_APMRESUMECRITICAL");
            return NO_ERROR;
        case PBT_POWERSETTINGCHANGE:
            break;
        default:
            LOG_INFO("Service", __func__, "Power", "Ignored power event ctrl={}, evtType={}", ctrl, evtType);
            return NO_ERROR;
        }

        if (!evtData) {
            LOG_WARN("Service", __func__, "Power", "PBT_POWERSETTINGCHANGE without evtData.");
            return NO_ERROR;
        }

        auto* pbs = static_cast<POWERBROADCAST_SETTING*>(evtData);
        if (pbs->PowerSetting == kDisplayGuid && pbs->DataLength >= 4) {
            DWORD state = *reinterpret_cast<DWORD*>(pbs->Data);
            LOG_INFO("Service", __func__, "Power", "GUID_CONSOLE_DISPLAY_STATE = {}", state);
            if (state >= 1) {
                signalEvent(0, "DisplayOn(Power)");
                signalEvent(2, "DisplayOn(Console)");
            } else {
                signalEvent(1, "DisplayOff(Power)");
                signalEvent(3, "DisplayOff(Console)");
            }
        }
        else if (pbs->PowerSetting == kLidGuid && pbs->DataLength >= 4) {
            DWORD state = *reinterpret_cast<DWORD*>(pbs->Data);
            LOG_INFO("Service", __func__, "Power", "GUID_LIDSWITCH_STATE = {} (1=open, 0=closed)", state);
            if (state == 1) {
                signalEvent(4, "LidOn");
            } else {
                signalEvent(5, "LidOff");
            }
        }
        else if (pbs->PowerSetting == kAwayGuid && pbs->DataLength >= 4) {
            DWORD state = *reinterpret_cast<DWORD*>(pbs->Data);
            LOG_INFO("Service", __func__, "Power", "GUID_SYSTEM_AWAYMODE = {} (1=enter, 0=exit)", state);
            if (state == 0) {
                signalEvent(7, "AwayModeExit");
            } else {
                signalEvent(1, "AwayModeEnter");
            }
        } else {
            LOG_INFO("Service", __func__, "Power", "Unhandled power setting GUID with dataLength={}", pbs->DataLength);
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

void ServiceShell::RunAsConsole() {
    m_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

    SetConsoleCtrlHandler([](DWORD) -> BOOL {
        SetEvent(Instance()->m_stopEvent);
        return TRUE;
    }, TRUE);

    LOG_INFO("Service", __func__, "Boot", "Starting modules (console mode)...");
    if (!m_host.Start()) {
        LOG_ERROR("Service", __func__, "Boot", "ServiceHost::Start() failed.");
        return;
    }

    LOG_INFO("Service", __func__, "Running", "Service running in console mode. Press Ctrl+C to stop.");
    WaitForStop();
    m_host.Stop();
    LOG_INFO("Service", __func__, "Stopped", "Console mode stopped.");
}

// ─── 辅助 ────────────────────────────────────

void ServiceShell::ReportStatus(DWORD state, DWORD waitHint) {
    if (!m_statusHandle) return;

    m_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_status.dwCurrentState = state;
    m_status.dwWin32ExitCode = NO_ERROR;
    m_status.dwWaitHint = waitHint;

    if (state == SERVICE_START_PENDING) {
        m_status.dwControlsAccepted = 0;
    } else {
        m_status.dwControlsAccepted =
            SERVICE_ACCEPT_STOP |
            SERVICE_ACCEPT_SHUTDOWN |
            SERVICE_ACCEPT_PRESHUTDOWN |
            SERVICE_ACCEPT_POWEREVENT;
    }

    static DWORD checkPoint = 1;
    if (state == SERVICE_RUNNING || state == SERVICE_STOPPED) {
        m_status.dwCheckPoint = 0;
    } else {
        m_status.dwCheckPoint = checkPoint++;
    }

    SetServiceStatus(m_statusHandle, &m_status);
}

void ServiceShell::WaitForStop() {
    if (m_stopEvent) {
        WaitForSingleObject(m_stopEvent, INFINITE);
        CloseHandle(m_stopEvent);
        m_stopEvent = nullptr;
    }
}

void ServiceShell::RegisterPowerNotifications() {
    if (!m_statusHandle) return;

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

    m_hDisplayNotify = RegisterPowerSettingNotification(
        m_statusHandle, &kDisplayGuid, DEVICE_NOTIFY_SERVICE_HANDLE);
    m_hLidNotify = RegisterPowerSettingNotification(
        m_statusHandle, &kLidGuid, DEVICE_NOTIFY_SERVICE_HANDLE);
    m_hSuspendNotify = RegisterPowerSettingNotification(
        m_statusHandle, &kAwayGuid, DEVICE_NOTIFY_SERVICE_HANDLE);

    LOG_INFO("Service", __func__, "Power", "Registered PBT notifications (display={}, lid={}, away={}).", m_hDisplayNotify != nullptr, m_hLidNotify != nullptr, m_hSuspendNotify != nullptr);
}

void ServiceShell::UnregisterPowerNotifications() {
    if (m_hDisplayNotify) {
        UnregisterPowerSettingNotification(m_hDisplayNotify);
        m_hDisplayNotify = nullptr;
    }
    if (m_hLidNotify) {
        UnregisterPowerSettingNotification(m_hLidNotify);
        m_hLidNotify = nullptr;
    }
    if (m_hSuspendNotify) {
        UnregisterPowerSettingNotification(m_hSuspendNotify);
        m_hSuspendNotify = nullptr;
    }
}

} // namespace Service

