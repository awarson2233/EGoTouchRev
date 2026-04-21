#include "SystemStateMonitor.h"
#include "Logger.h"

#include <array>
#include <atomic>
#include <exception>
#include <thread>
#include <utility>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <sddl.h>

namespace Host {

struct SystemStateMonitor::Impl {
    HANDLE events[kEventCount]{};
    HANDLE stopEvent = nullptr;
    EventCallback callback;
    std::thread worker;
    std::atomic<bool> running{false};
};

namespace {

struct NamedEventNameListHolder {
    const wchar_t* names[SystemStateMonitor::kEventCount]{};

    constexpr NamedEventNameListHolder() {
        for (std::size_t i = 0; i < SystemStateMonitor::kEventCount; ++i) {
            names[i] = kSystemStateNamedEventSpecs[i].name;
        }
    }
};

inline constexpr NamedEventNameListHolder kNamedEventNameListHolder{};

HANDLE OpenOrCreateNamedEvent(const wchar_t* name) {
    HANDLE handle = OpenEventW(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, name);
    if (handle != nullptr) {
        return handle;
    }

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = FALSE;
    sa.lpSecurityDescriptor = nullptr;

    // Secure SDDL:
    // D: DACL
    // (A;;GA;;;SY) -> Allow Generic All for SYSTEM
    // (A;;GA;;;BA) -> Allow Generic All for Built-in Administrators
    // (A;;GRGW;;;BU) -> Allow Generic Read/Write for Built-in Users
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            L"D:(A;;GA;;;SY)(A;;GA;;;BA)(A;;GRGW;;;BU)",
            SDDL_REVISION_1,
            &sa.lpSecurityDescriptor,
            nullptr)) {
        // Fallback to default security descriptor if conversion fails
        sa.lpSecurityDescriptor = nullptr;
    }

    HANDLE new_handle = CreateEventW(sa.lpSecurityDescriptor ? &sa : nullptr, TRUE, FALSE, name);

    if (sa.lpSecurityDescriptor != nullptr) {
        LocalFree(sa.lpSecurityDescriptor);
    }

    return new_handle;
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

SystemStateMonitor::SystemStateMonitor()
    : m_impl(std::make_unique<Impl>()) {}

SystemStateMonitor::~SystemStateMonitor() {
    Stop();
}

namespace {

SystemStateEvent BuildEvent(std::size_t index) {
    SystemStateEvent event{};
    event.source = SystemStateEventSource::ThpServiceNamedEvent;
    event.timestamp = std::chrono::system_clock::now();
    event.raw_index = static_cast<std::uint32_t>(index);

    const SystemStateNamedEventSpec* spec = TryGetNamedEventSpec(index);
    if (spec != nullptr) {
        event.raw_name = spec->name;
        event.type = spec->type;
    } else {
        event.raw_name = L"";
        event.type = SystemStateEventType::Unknown;
    }

    return event;
}

template <typename ImplT>
bool OpenOrCreateEvents(ImplT& impl) {
    for (std::size_t i = 0; i < SystemStateMonitor::kEventCount; ++i) {
        const SystemStateNamedEventSpec& spec = kSystemStateNamedEventSpecs[i];
        impl.events[i] = OpenOrCreateNamedEvent(spec.name);
        if (impl.events[i] == nullptr || impl.events[i] == INVALID_HANDLE_VALUE) {
            return false;
        }
    }

    return true;
}

template <typename ImplT>
void CloseEvents(ImplT& impl) noexcept {
    for (HANDLE& event_handle : impl.events) {
        if (event_handle != nullptr && event_handle != INVALID_HANDLE_VALUE) {
            CloseHandle(event_handle);
        }
        event_handle = nullptr;
    }

    if (impl.stopEvent != nullptr && impl.stopEvent != INVALID_HANDLE_VALUE) {
        CloseHandle(impl.stopEvent);
        impl.stopEvent = nullptr;
    }
}

template <typename ImplT>
void WorkerLoop(ImplT& impl) {
    std::array<HANDLE, SystemStateMonitor::kEventCount + 1> wait_handles{};
    wait_handles[0] = impl.stopEvent;
    for (std::size_t i = 0; i < SystemStateMonitor::kEventCount; ++i) {
        wait_handles[i + 1] = impl.events[i];
    }

    while (impl.running.load(std::memory_order_acquire)) {
        const DWORD wait_result = WaitForMultipleObjects(
            static_cast<DWORD>(wait_handles.size()),
            wait_handles.data(),
            FALSE,
            INFINITE);

        if (wait_result == WAIT_OBJECT_0) {
            LOG_INFO("Host", __func__, "Stop", "Stop event signaled, exiting monitor loop.");
            break;
        }

        if (wait_result >= WAIT_OBJECT_0 + 1 &&
            wait_result < WAIT_OBJECT_0 + 1 + SystemStateMonitor::kEventCount) {
            const std::size_t event_index = static_cast<std::size_t>(wait_result - WAIT_OBJECT_0 - 1);
            SystemStateEvent event = BuildEvent(event_index);

            LOG_INFO("Host", __func__, "Signal", "Named event[{}] signaled → type={}", event_index, ToString(event.type));

            if (impl.callback) {
                try {
                    impl.callback(event);
                } catch (const std::exception& ex) {
                    LOG_ERROR("Host", __func__, "Callback", "SystemStateMonitor callback threw std::exception: {}", ex.what());
                } catch (...) {
                    LOG_ERROR("Host", __func__, "Callback", "SystemStateMonitor callback threw unknown exception.");
                }
            }

            HANDLE event_handle = impl.events[event_index];
            if (event_handle != nullptr && event_handle != INVALID_HANDLE_VALUE) {
                ResetEvent(event_handle);
            }

            continue;
        }

        // WAIT_FAILED/WAIT_ABANDONED are treated as hard stop for this monitor instance.
        LOG_WARN("Host", __func__, "Error", "WaitForMultipleObjects returned unexpected result: {}", wait_result);
        break;
    }

    impl.running.store(false, std::memory_order_release);
}

} // namespace

bool SystemStateMonitor::Start(EventCallback callback) {
    Impl& impl = *m_impl;

    if (impl.running.load(std::memory_order_acquire)) {
        return false;
    }

    if (impl.worker.joinable()) {
        if (impl.worker.get_id() == std::this_thread::get_id()) {
            LOG_WARN("Host", __func__, "Thread", "Start() called from worker thread while previous worker is joinable.");
            return false;
        }

        impl.worker.join();
        CloseEvents(impl);
        impl.callback = nullptr;
    }

    if (impl.running.exchange(true, std::memory_order_acq_rel)) {
        return false;
    }

    impl.callback = std::move(callback);
    impl.stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (impl.stopEvent == nullptr) {
        impl.running.store(false, std::memory_order_release);
        return false;
    }

    if (!OpenOrCreateEvents(impl)) {
        Stop();
        return false;
    }

    impl.worker = std::thread([this]() {
        WorkerLoop(*m_impl);
    });
    return true;
}

void SystemStateMonitor::Stop() {
    if (!m_impl) {
        return;
    }

    Impl& impl = *m_impl;
    impl.running.store(false, std::memory_order_release);

    if (impl.stopEvent != nullptr) {
        SetEvent(impl.stopEvent);
    }

    if (impl.worker.joinable()) {
        if (impl.worker.get_id() == std::this_thread::get_id()) {
            LOG_INFO("Host", __func__, "Thread", "Stop() called from worker thread; deferring join to external caller.");
            return;
        }

        impl.worker.join();
    }

    CloseEvents(impl);
    impl.callback = nullptr;
}

bool SystemStateMonitor::IsRunning() const noexcept {
    return m_impl && m_impl->running.load(std::memory_order_acquire);
}

const SystemStateNamedEventSpec (&SystemStateMonitor::NamedEventSpecs() noexcept)[SystemStateMonitor::kEventCount] {
    return kSystemStateNamedEventSpecs;
}

const wchar_t* const (&SystemStateMonitor::NamedEventList() noexcept)[SystemStateMonitor::kEventCount] {
    return kNamedEventNameListHolder.names;
}

bool SystemStateMonitor::SignalNamedEvent(SystemStateNamedEventId id) noexcept {
    const SystemStateNamedEventSpec* spec = TryGetNamedEventSpec(id);
    if (spec == nullptr || spec->name == nullptr || spec->name[0] == L'\0') {
        return false;
    }

    HANDLE event_handle = OpenEventW(EVENT_MODIFY_STATE, FALSE, spec->name);
    if (event_handle == nullptr || event_handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    const BOOL set_result = SetEvent(event_handle);
    CloseHandle(event_handle);
    return set_result != FALSE;
}

} // namespace Host
