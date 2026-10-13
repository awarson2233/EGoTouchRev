#include "ServiceShell.h"
#include "SystemStateMonitor.h"
#include "ServiceHost.h"
#include "Logger.h"

#include <powersetting.h>
#include <powrprof.h>

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

    // 控制台模式没有 SCM 句柄，改用回调式注册
    HPOWERNOTIFY hConsoleDisplayNotify = nullptr;
    HPOWERNOTIFY hConsoleLidNotify = nullptr;
    HPOWERNOTIFY hConsoleSuspendResume = nullptr;
    DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS consoleSubscribe{};
};

namespace {

// GUID_CONSOLE_DISPLAY_STATE: 0=off, 1=on, 2=dimmed
constexpr GUID kDisplayGuid =
    {0x6fe69556, 0x704a, 0x47a0, {0x8f, 0x24, 0xc2, 0x8d, 0x93, 0x6f, 0xda, 0x47}};
// GUID_LIDSWITCH_STATE_CHANGE
constexpr GUID kLidGuid =
    {0xba3e0f4d, 0xb817, 0x4094, {0xa2, 0xd1, 0xd5, 0x63, 0x79, 0xe6, 0xa0, 0xf3}};
// GUID_SYSTEM_AWAYMODE (away mode / connected standby)
constexpr GUID kAwayGuid =
    {0x98a7f580, 0x01f7, 0x48aa, {0x9c, 0x0f, 0x44, 0x35, 0x2c, 0x29, 0xe5, 0xc0}};

void SignalEvent(Host::SystemStateNamedEventId id) {
    (void)Host::SystemStateMonitor::SignalNamedEvent(id);
}

// 服务模式（SERVICE_CONTROL_POWEREVENT）与控制台模式（注册回调）共用的电源事件翻译
void DispatchPowerEvent(const char* source, DWORD evtType, const POWERBROADCAST_SETTING* pbs) {
    switch (evtType) {
    case PBT_APMSUSPEND:
        LOG_NOTICE("Service", "Power", source, "PBT_APMSUSPEND");
        SignalEvent(Host::SystemStateNamedEventId::PbtApmSuspend);
        return;
    case PBT_APMRESUMEAUTOMATIC:
        LOG_NOTICE("Service", "Power", source, "PBT_APMRESUMEAUTOMATIC");
        SignalEvent(Host::SystemStateNamedEventId::PbtApmResumeAutomatic);
        return;
    case PBT_APMRESUMESUSPEND:
        LOG_NOTICE("Service", "Power", source, "PBT_APMRESUMESUSPEND");
        SignalEvent(Host::SystemStateNamedEventId::PbtApmResumeSuspend);
        return;
    case PBT_POWERSETTINGCHANGE:
        break;
    default:
        return;
    }

    if (pbs == nullptr || pbs->DataLength < sizeof(DWORD)) return;
    const DWORD state = *reinterpret_cast<const DWORD*>(pbs->Data);
    if (pbs->PowerSetting == kDisplayGuid) {
        LOG_NOTICE("Service", "Power", source, "GUID_CONSOLE_DISPLAY_STATE = {} (0=off, 1=on, 2=dimmed)", state);
        SignalEvent(state >= 1 ? Host::SystemStateNamedEventId::MonitorConsoleDisplayOn
                               : Host::SystemStateNamedEventId::MonitorConsoleDisplayOff);
    } else if (pbs->PowerSetting == kLidGuid) {
        LOG_NOTICE("Service", "Power", source, "GUID_LIDSWITCH_STATE = {} (1=open, 0=closed)", state);
        SignalEvent(state == 1 ? Host::SystemStateNamedEventId::MonitorLidOn
                               : Host::SystemStateNamedEventId::MonitorLidOff);
    }
}

ULONG CALLBACK ConsolePowerCallback(PVOID, ULONG type, PVOID setting) {
    DispatchPowerEvent("Console", type, static_cast<const POWERBROADCAST_SETTING*>(setting));
    return ERROR_SUCCESS;
}

} // namespace

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

    switch (ctrl) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
    case SERVICE_CONTROL_PRESHUTDOWN:
        LOG_INFO("Service", __func__, "Stopping", "Received stop/shutdown control code={}.", ctrl);
        s->SignalShutdownTransportAndStop();
        s->ReportStatus(SERVICE_STOP_PENDING, 5000);
        return NO_ERROR;

    case SERVICE_CONTROL_POWEREVENT:
        DispatchPowerEvent("Service", evtType, static_cast<const POWERBROADCAST_SETTING*>(evtData));
        return NO_ERROR;

    case SERVICE_CONTROL_INTERROGATE:
        return NO_ERROR;
    default:
        return ERROR_CALL_NOT_IMPLEMENTED;
    }
}

// ─── 控制台模式 ──────────────────────────────

#if EGOTOUCH_SERVICE_ENABLE_IPC
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
#endif

void ServiceShell::SignalShutdownTransportAndStop() noexcept {
    Host::SystemStateMonitor::SignalNamedEvent(Host::SystemStateNamedEventId::MonitorShutDown);
    if (m_impl != nullptr && m_impl->stopEvent != nullptr) {
        SetEvent(m_impl->stopEvent);
    }
}

#if EGOTOUCH_SERVICE_ENABLE_IPC
void ServiceShell::RunAsConsole() {
    m_impl->stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

    SetConsoleCtrlHandler(&ServiceShell::ConsoleCtrlHandler, TRUE);

    LOG_INFO("Service", __func__, "Boot", "Starting modules (console mode)...");
    if (!m_impl->host.Start()) {
        LOG_ERROR("Service", __func__, "Boot", "ServiceHost::Start() failed.");
        return;
    }

    RegisterConsolePowerNotifications();
    LOG_INFO("Service", __func__, "Running", "Service running in console mode. Press Ctrl+C to stop.");
    WaitForStop();
    UnregisterConsolePowerNotifications();
    m_impl->host.Stop();
    LOG_INFO("Service", __func__, "Stopped", "Console mode stopped.");
}
#endif

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

    m_impl->hDisplayNotify = RegisterPowerSettingNotification(
        m_impl->statusHandle, &kDisplayGuid, DEVICE_NOTIFY_SERVICE_HANDLE);
    m_impl->hLidNotify = RegisterPowerSettingNotification(
        m_impl->statusHandle, &kLidGuid, DEVICE_NOTIFY_SERVICE_HANDLE);
    m_impl->hSuspendNotify = RegisterPowerSettingNotification(
        m_impl->statusHandle, &kAwayGuid, DEVICE_NOTIFY_SERVICE_HANDLE);

    LOG_NOTICE("Service", __func__, "Power", "Registered PBT notifications (display={}, lid={}, away={}).", m_impl->hDisplayNotify != nullptr, m_impl->hLidNotify != nullptr, m_impl->hSuspendNotify != nullptr);
}

void ServiceShell::RegisterConsolePowerNotifications() {
    m_impl->consoleSubscribe.Callback = &ConsolePowerCallback;
    m_impl->consoleSubscribe.Context = nullptr;
    HANDLE recipient = &m_impl->consoleSubscribe;

    const DWORD displayErr = PowerSettingRegisterNotification(
        &kDisplayGuid, DEVICE_NOTIFY_CALLBACK, recipient, &m_impl->hConsoleDisplayNotify);
    const DWORD lidErr = PowerSettingRegisterNotification(
        &kLidGuid, DEVICE_NOTIFY_CALLBACK, recipient, &m_impl->hConsoleLidNotify);
    m_impl->hConsoleSuspendResume = RegisterSuspendResumeNotification(recipient, DEVICE_NOTIFY_CALLBACK);

    LOG_NOTICE("Service", __func__, "Power",
               "Registered console power callbacks (display err={}, lid err={}, suspend/resume={}).",
               displayErr, lidErr, m_impl->hConsoleSuspendResume != nullptr);
}

void ServiceShell::UnregisterConsolePowerNotifications() {
    if (m_impl->hConsoleDisplayNotify) {
        PowerSettingUnregisterNotification(m_impl->hConsoleDisplayNotify);
        m_impl->hConsoleDisplayNotify = nullptr;
    }
    if (m_impl->hConsoleLidNotify) {
        PowerSettingUnregisterNotification(m_impl->hConsoleLidNotify);
        m_impl->hConsoleLidNotify = nullptr;
    }
    if (m_impl->hConsoleSuspendResume) {
        UnregisterSuspendResumeNotification(m_impl->hConsoleSuspendResume);
        m_impl->hConsoleSuspendResume = nullptr;
    }
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

