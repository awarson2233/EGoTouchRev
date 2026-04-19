#pragma once

#include "SystemStateEvent.h"

#include <cstddef>
#include <functional>
#include <memory>

namespace Host {

class SystemStateMonitor {
public:
    // Invoked on the monitor worker thread for each signaled named event.
    // Callback exceptions are contained inside the monitor and logged.
    // Callback may call Stop(); Stop() is reentrant-safe for worker-thread calls.
    using EventCallback = std::function<void(const SystemStateEvent&)>;

    static constexpr std::size_t kEventCount = kSystemStateNamedEventCount;

    SystemStateMonitor();
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

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace Host

