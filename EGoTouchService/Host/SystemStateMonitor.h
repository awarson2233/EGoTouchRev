#pragma once

#include "SystemStateEvent.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <functional>
#include <thread>
#include <windows.h>

namespace Host {

class SystemStateMonitor {
public:
    using EventCallback = std::function<void(const SystemStateEvent&)>;

    static constexpr std::size_t kEventCount = 8;

    SystemStateMonitor();
    ~SystemStateMonitor();

    SystemStateMonitor(const SystemStateMonitor&) = delete;
    SystemStateMonitor& operator=(const SystemStateMonitor&) = delete;
    SystemStateMonitor(SystemStateMonitor&&) = delete;
    SystemStateMonitor& operator=(SystemStateMonitor&&) = delete;

    bool Start(EventCallback callback);
    void Stop();

    bool IsRunning() const noexcept;

    static const std::array<const wchar_t*, kEventCount>& NamedEventList() noexcept;

private:
    bool OpenOrCreateEvents();
    void CloseEvents() noexcept;
    void WorkerLoop();
    static SystemStateEvent BuildEvent(std::size_t index);

private:
    std::array<HANDLE, kEventCount> m_events{};
    HANDLE m_stopEvent = nullptr;
    EventCallback m_callback;
    std::thread m_worker;
    std::atomic<bool> m_running{false};
};

} // namespace Host

