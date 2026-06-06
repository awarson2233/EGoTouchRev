#pragma once

#include "SystemStateEvent.h"

#include <cstddef>
#include <functional>
#include <memory>

namespace Host {

class SystemStateMonitor {
public:
    // Invoked on the monitor worker thread for coalesced named-event state changes.
    // The Win32 manual-reset named events are level notifications, not counters;
    // repeated SetEvent calls while an event is already signaled may coalesce.
    // Callback exceptions are contained inside the monitor and logged.
    // Callback may call Stop(); Stop() is reentrant-safe for worker-thread calls.
    using EventCallback = std::function<void(const SystemStateEvent&)>;

    static constexpr std::size_t kEventCount = kSystemStateNamedEventCount;

    SystemStateMonitor();
    explicit SystemStateMonitor(const wchar_t* const (&eventNames)[kEventCount]);
    ~SystemStateMonitor();

    SystemStateMonitor(const SystemStateMonitor&) = delete;
    SystemStateMonitor& operator=(const SystemStateMonitor&) = delete;
    SystemStateMonitor(SystemStateMonitor&&) = delete;
    SystemStateMonitor& operator=(SystemStateMonitor&&) = delete;

    bool Start(EventCallback callback);
    void Stop();

    bool IsRunning() const noexcept;

    static const SystemStateNamedEventSpec (&NamedEventSpecs() noexcept)[kEventCount];
    static const wchar_t* const (&NamedEventList() noexcept)[kEventCount];
    static bool SignalNamedEvent(SystemStateNamedEventId id) noexcept;
    static bool SignalNamedEvent(SystemStateNamedEventId id, const wchar_t* const (&eventNames)[kEventCount]) noexcept;

private:
    struct Impl;
    std::shared_ptr<Impl> m_impl;
};

} // namespace Host

