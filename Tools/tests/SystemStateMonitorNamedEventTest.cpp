#include "SystemStateMonitor.h"

#include <windows.h>

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace {

bool ExpectSequence(const std::vector<Host::SystemStateEventType>& expected,
                    const std::vector<Host::SystemStateEventType>& actual) {
    if (actual.size() < expected.size()) {
        return false;
    }

    for (std::size_t i = 0; i < expected.size(); ++i) {
        if (expected[i] != actual[i]) {
            return false;
        }
    }

    return true;
}

} // namespace

int main() {
    using namespace std::chrono_literals;

    Host::SystemStateMonitor monitor;

    std::mutex mu;
    std::condition_variable cv;
    std::vector<Host::SystemStateEventType> observed;

    const bool started = monitor.Start([&](const Host::SystemStateEvent& event) {
        {
            std::lock_guard<std::mutex> lock(mu);
            observed.push_back(event.type);
        }
        cv.notify_all();
    });

    if (!started) {
        std::cerr << "[TEST] SystemStateMonitor start failed.\n";
        return 1;
    }

    for (HANDLE event_handle : monitor.m_events) {
        if (event_handle != nullptr && event_handle != INVALID_HANDLE_VALUE) {
            ResetEvent(event_handle);
        }
    }

    // Give worker thread a short warm-up window before injecting events.
    std::this_thread::sleep_for(80ms);

    const std::vector<std::pair<std::size_t, Host::SystemStateEventType>> script = {
        {1, Host::SystemStateEventType::DisplayOff},
        {0, Host::SystemStateEventType::DisplayOn},
        {3, Host::SystemStateEventType::DisplayOff},
        {2, Host::SystemStateEventType::DisplayOn},
        {5, Host::SystemStateEventType::LidOff},
        {4, Host::SystemStateEventType::LidOn},
        {6, Host::SystemStateEventType::Shutdown},
        {7, Host::SystemStateEventType::ResumeAutomatic},
    };

    for (const auto& step : script) {
        const std::size_t index = step.first;
        if (index >= monitor.m_events.size()) {
            monitor.Stop();
            return 2;
        }
        HANDLE event_handle = monitor.m_events[index];
        if (event_handle == nullptr || event_handle == INVALID_HANDLE_VALUE) {
            monitor.Stop();
            std::cerr << "[TEST] Invalid event handle for index=" << index << "\n";
            return 2;
        }
        if (!SetEvent(event_handle)) {
            monitor.Stop();
            std::cerr << "[TEST] SetEvent failed for index=" << index << ", gle=" << GetLastError() << "\n";
            return 2;
        }
        std::this_thread::sleep_for(20ms);
    }

    const std::vector<Host::SystemStateEventType> expected = {
        Host::SystemStateEventType::DisplayOff,
        Host::SystemStateEventType::DisplayOn,
        Host::SystemStateEventType::DisplayOff,
        Host::SystemStateEventType::DisplayOn,
        Host::SystemStateEventType::LidOff,
        Host::SystemStateEventType::LidOn,
        Host::SystemStateEventType::Shutdown,
        Host::SystemStateEventType::ResumeAutomatic,
    };

    {
        std::unique_lock<std::mutex> lock(mu);
        const bool received_all = cv.wait_for(lock, 2s, [&] {
            return observed.size() >= expected.size();
        });
        if (!received_all) {
            monitor.Stop();
            std::cerr << "[TEST] Timeout waiting for detector events. observed=" << observed.size() << "\n";
            return 3;
        }
    }

    monitor.Stop();

    if (!ExpectSequence(expected, observed)) {
        std::cerr << "[TEST] Event sequence mismatch.\n";
        for (std::size_t i = 0; i < observed.size(); ++i) {
            std::cerr << "  observed[" << i << "]=" << Host::ToString(observed[i]) << "\n";
        }
        return 4;
    }

    std::cout << "[TEST] SystemStateMonitor event test passed. observed=" << observed.size() << "\n";
    return 0;
}
