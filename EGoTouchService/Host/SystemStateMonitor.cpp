#include "SystemStateMonitor.h"
#include "Logger.h"

#include <array>
#include <utility>

namespace Host {

namespace {

constexpr std::array<const wchar_t*, SystemStateMonitor::kEventCount> kNamedEvents = {
    L"Global\\MonitorPowerOnEvent",
    L"Global\\MonitorPowerOffEvent",
    L"Global\\MonitorConsoleDisplayOnEvent",
    L"Global\\MonitorConsoleDisplayOffEvent",
    L"Global\\MonitorLidOnEvent",
    L"Global\\MonitorLidOffEvent",
    L"Global\\MonitorShutDownEvent",
    L"Global\\PBT_APMRESUMEAUTOMATIC",
};

SECURITY_ATTRIBUTES BuildPermissiveGlobalSa(SECURITY_DESCRIPTOR& sd) {
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = FALSE;
    sa.lpSecurityDescriptor = nullptr;

    if (InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION) != 0 &&
        SetSecurityDescriptorDacl(&sd, TRUE, nullptr, FALSE) != 0) {
        sa.lpSecurityDescriptor = &sd;
    }

    return sa;
}

HANDLE OpenOrCreateNamedEvent(const wchar_t* name) {
    HANDLE handle = OpenEventW(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, name);
    if (handle != nullptr) {
        return handle;
    }

    SECURITY_DESCRIPTOR sd{};
    SECURITY_ATTRIBUTES sa = BuildPermissiveGlobalSa(sd);
    LPSECURITY_ATTRIBUTES sa_ptr = sa.lpSecurityDescriptor != nullptr ? &sa : nullptr;

    return CreateEventW(sa_ptr, TRUE, FALSE, name);
}

} // namespace

const char* ToString(SystemStateEventType type) noexcept {
    switch (type) {
    case SystemStateEventType::DisplayOn:
        return "DisplayOn";
    case SystemStateEventType::DisplayOff:
        return "DisplayOff";
    case SystemStateEventType::LidOn:
        return "LidOn";
    case SystemStateEventType::LidOff:
        return "LidOff";
    case SystemStateEventType::Shutdown:
        return "Shutdown";
    case SystemStateEventType::ResumeAutomatic:
        return "ResumeAutomatic";
    default:
        return "Unknown";
    }
}

SystemStateMonitor::SystemStateMonitor() {
    m_events.fill(nullptr);
}

SystemStateMonitor::~SystemStateMonitor() {
    Stop();
}

bool SystemStateMonitor::Start(EventCallback callback) {
    if (m_running.exchange(true, std::memory_order_acq_rel)) {
        return false;
    }

    m_callback = std::move(callback);
    m_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (m_stopEvent == nullptr) {
        m_running.store(false, std::memory_order_release);
        return false;
    }

    if (!OpenOrCreateEvents()) {
        Stop();
        return false;
    }

    m_worker = std::thread(&SystemStateMonitor::WorkerLoop, this);
    return true;
}

void SystemStateMonitor::Stop() {
    m_running.store(false, std::memory_order_release);

    if (m_stopEvent != nullptr) {
        SetEvent(m_stopEvent);
    }

    if (m_worker.joinable()) {
        m_worker.join();
    }

    CloseEvents();
    m_callback = nullptr;
}

bool SystemStateMonitor::IsRunning() const noexcept {
    return m_running.load(std::memory_order_acquire);
}

const std::array<const wchar_t*, SystemStateMonitor::kEventCount>& SystemStateMonitor::NamedEventList() noexcept {
    return kNamedEvents;
}

bool SystemStateMonitor::OpenOrCreateEvents() {
    for (std::size_t i = 0; i < kEventCount; ++i) {
        m_events[i] = OpenOrCreateNamedEvent(kNamedEvents[i]);
        if (m_events[i] == nullptr || m_events[i] == INVALID_HANDLE_VALUE) {
            return false;
        }
    }

    return true;
}

void SystemStateMonitor::CloseEvents() noexcept {
    for (HANDLE& event_handle : m_events) {
        if (event_handle != nullptr && event_handle != INVALID_HANDLE_VALUE) {
            CloseHandle(event_handle);
        }
        event_handle = nullptr;
    }

    if (m_stopEvent != nullptr && m_stopEvent != INVALID_HANDLE_VALUE) {
        CloseHandle(m_stopEvent);
        m_stopEvent = nullptr;
    }
}

void SystemStateMonitor::WorkerLoop() {
    std::array<HANDLE, kEventCount + 1> wait_handles{};
    wait_handles[0] = m_stopEvent;
    for (std::size_t i = 0; i < kEventCount; ++i) {
        wait_handles[i + 1] = m_events[i];
    }

    while (m_running.load(std::memory_order_acquire)) {
        DWORD wait_result = WaitForMultipleObjects(
            static_cast<DWORD>(wait_handles.size()),
            wait_handles.data(),
            FALSE,
            INFINITE);

        if (wait_result == WAIT_OBJECT_0) {
            LOG_INFO("Host", __func__, "Stop", "Stop event signaled, exiting monitor loop.");
            break;
        }

        if (wait_result >= WAIT_OBJECT_0 + 1 && wait_result < WAIT_OBJECT_0 + 1 + kEventCount) {
            const std::size_t event_index = static_cast<std::size_t>(wait_result - WAIT_OBJECT_0 - 1);
            SystemStateEvent event = BuildEvent(event_index);

            LOG_INFO("Host", __func__, "Signal", "Named event[{}] signaled → type={}",  event_index, ToString(event.type));

            if (m_callback) {
                m_callback(event);
            }

            HANDLE event_handle = m_events[event_index];
            if (event_handle != nullptr && event_handle != INVALID_HANDLE_VALUE) {
                ResetEvent(event_handle);
            }

            continue;
        }

        // WAIT_FAILED/WAIT_ABANDONED are treated as hard stop for this monitor instance.
        LOG_WARN("Host", __func__, "Error", "WaitForMultipleObjects returned unexpected result: {}",  wait_result);
        break;
    }

    m_running.store(false, std::memory_order_release);
}

SystemStateEvent SystemStateMonitor::BuildEvent(std::size_t index) {
    SystemStateEvent event{};
    event.source = SystemStateEventSource::ThpServiceNamedEvent;
    event.timestamp = std::chrono::system_clock::now();
    event.raw_index = static_cast<std::uint32_t>(index);
    event.raw_name = index < kEventCount ? kNamedEvents[index] : L"";

    switch (index) {
    case 0:
    case 2:
        event.type = SystemStateEventType::DisplayOn;
        break;
    case 1:
    case 3:
        event.type = SystemStateEventType::DisplayOff;
        break;
    case 4:
        event.type = SystemStateEventType::LidOn;
        break;
    case 5:
        event.type = SystemStateEventType::LidOff;
        break;
    case 6:
        event.type = SystemStateEventType::Shutdown;
        break;
    case 7:
        event.type = SystemStateEventType::ResumeAutomatic;
        break;
    default:
        event.type = SystemStateEventType::Unknown;
        break;
    }

    return event;
}

} // namespace Host
