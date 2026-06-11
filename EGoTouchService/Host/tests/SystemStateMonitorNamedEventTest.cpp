#include "SystemStateMonitor.h"

#include <windows.h>

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

constexpr auto kWorkerWarmup = std::chrono::milliseconds(80);
constexpr auto kCallbackTimeout = std::chrono::seconds(2);
constexpr auto kNoExtraCallbackWindow = std::chrono::milliseconds(200);

struct TestEventNames {
    std::array<std::wstring, Host::SystemStateMonitor::kEventCount> storage{};
    const wchar_t* names[Host::SystemStateMonitor::kEventCount]{};

    explicit TestEventNames(const wchar_t* scenario) {
        const DWORD process_id = GetCurrentProcessId();
        const DWORD thread_id = GetCurrentThreadId();
        for (std::size_t i = 0; i < Host::SystemStateMonitor::kEventCount; ++i) {
            std::wostringstream stream;
            stream << L"Local\\EGoTouchSystemStateMonitorTest_"
                   << process_id << L"_" << thread_id << L"_"
                   << scenario << L"_" << i;
            storage[i] = stream.str();
            names[i] = storage[i].c_str();
        }
    }

    const wchar_t* const (&List() const noexcept)[Host::SystemStateMonitor::kEventCount] {
        return names;
    }
};

void ResetNamedEventBestEffort(const wchar_t* event_name) {
    HANDLE event_handle = OpenEventW(EVENT_MODIFY_STATE, FALSE, event_name);
    if (event_handle == nullptr) {
        return;
    }
    ResetEvent(event_handle);
    CloseHandle(event_handle);
}

void ResetAllNamedEventsBestEffort(const wchar_t* const (&named_events)[Host::SystemStateMonitor::kEventCount]) {
    for (const wchar_t* event_name : named_events) {
        ResetNamedEventBestEffort(event_name);
    }
}

struct TestEventHandles {
    std::vector<HANDLE> handles;

    ~TestEventHandles() {
        Close();
    }

    TestEventHandles(const TestEventHandles&) = delete;
    TestEventHandles& operator=(const TestEventHandles&) = delete;

    TestEventHandles() = default;

    bool OpenAll(const wchar_t* const (&named_events)[Host::SystemStateMonitor::kEventCount]) {
        Close();
        handles.reserve(Host::SystemStateMonitor::kEventCount);

        for (const wchar_t* event_name : named_events) {
            HANDLE event_handle = CreateEventW(nullptr, TRUE, FALSE, event_name);
            if (event_handle == nullptr) {
                std::wcerr << L"[TEST] CreateEventW failed for " << event_name
                           << L" error=" << GetLastError() << L"\n";
                Close();
                return false;
            }
            handles.push_back(event_handle);
        }

        return handles.size() == Host::SystemStateMonitor::kEventCount;
    }

    void Close() noexcept {
        for (HANDLE handle : handles) {
            if (handle != nullptr && handle != INVALID_HANDLE_VALUE) {
                CloseHandle(handle);
            }
        }
        handles.clear();
    }

    bool Set(Host::SystemStateNamedEventId id) const noexcept {
        const std::size_t index = Host::ToIndex(id);
        if (index >= handles.size() || handles[index] == nullptr) {
            return false;
        }
        return SetEvent(handles[index]) != FALSE;
    }

    bool Reset(Host::SystemStateNamedEventId id) const noexcept {
        const std::size_t index = Host::ToIndex(id);
        if (index >= handles.size() || handles[index] == nullptr) {
            return false;
        }
        return ResetEvent(handles[index]) != FALSE;
    }
};

struct ObservedEvent {
    Host::SystemStateEventType type = Host::SystemStateEventType::Unknown;
    std::uint32_t raw_index = 0;
};

class EventRecorder {
public:
    void Record(const Host::SystemStateEvent& event) {
        {
            std::lock_guard<std::mutex> lock(mu_);
            observed_.push_back({event.type, event.raw_index});
        }
        cv_.notify_all();
    }

    bool WaitForCount(std::size_t expected_count, std::chrono::milliseconds timeout = kCallbackTimeout) {
        std::unique_lock<std::mutex> lock(mu_);
        return cv_.wait_for(lock, timeout, [&] {
            return observed_.size() >= expected_count;
        });
    }

    std::vector<ObservedEvent> Snapshot() const {
        std::lock_guard<std::mutex> lock(mu_);
        return observed_;
    }

    std::size_t Size() const {
        std::lock_guard<std::mutex> lock(mu_);
        return observed_.size();
    }

private:
    mutable std::mutex mu_;
    std::condition_variable cv_;
    std::vector<ObservedEvent> observed_;
};

void DumpObserved(const std::vector<ObservedEvent>& observed) {
    for (std::size_t i = 0; i < observed.size(); ++i) {
        std::cerr << "  observed[" << i << "]=" << Host::ToString(observed[i].type)
                  << " raw_index=" << observed[i].raw_index << "\n";
    }
}

bool ExpectTypes(const std::vector<Host::SystemStateEventType>& expected,
                 const std::vector<ObservedEvent>& actual,
                 const char* scenario) {
    if (actual.size() != expected.size()) {
        std::cerr << "[TEST] " << scenario << " count mismatch. expected=" << expected.size()
                  << " actual=" << actual.size() << "\n";
        DumpObserved(actual);
        return false;
    }

    for (std::size_t i = 0; i < expected.size(); ++i) {
        if (expected[i] != actual[i].type) {
            std::cerr << "[TEST] " << scenario << " type mismatch at index " << i
                      << ". expected=" << Host::ToString(expected[i])
                      << " actual=" << Host::ToString(actual[i].type) << "\n";
            DumpObserved(actual);
            return false;
        }
    }

    return true;
}

bool SignalAndWaitForCount(Host::SystemStateMonitor& monitor,
                           EventRecorder& recorder,
                           const wchar_t* const (&event_names)[Host::SystemStateMonitor::kEventCount],
                           Host::SystemStateNamedEventId id,
                           std::size_t expected_count,
                           const char* scenario) {
    if (!Host::SystemStateMonitor::SignalNamedEvent(id, event_names)) {
        monitor.Stop();
        std::cerr << "[TEST] Failed to signal named event in " << scenario << ". id="
                  << static_cast<int>(id) << "\n";
        return false;
    }

    if (!recorder.WaitForCount(expected_count)) {
        monitor.Stop();
        std::cerr << "[TEST] Timeout waiting for callback count " << expected_count
                  << " in " << scenario << ". observed=" << recorder.Size() << "\n";
        return false;
    }

    return true;
}

bool SignalAndExpectNoAdditionalCallback(Host::SystemStateMonitor& monitor,
                                         EventRecorder& recorder,
                                         const wchar_t* const (&event_names)[Host::SystemStateMonitor::kEventCount],
                                         Host::SystemStateNamedEventId id,
                                         std::size_t stable_count,
                                         const char* scenario) {
    if (!Host::SystemStateMonitor::SignalNamedEvent(id, event_names)) {
        monitor.Stop();
        std::cerr << "[TEST] Failed to signal duplicate named event in " << scenario << ". id="
                  << static_cast<int>(id) << "\n";
        return false;
    }

    if (recorder.WaitForCount(stable_count + 1, kNoExtraCallbackWindow)) {
        monitor.Stop();
        std::cerr << "[TEST] Unexpected duplicate callback in " << scenario
                  << ". observed=" << recorder.Size() << "\n";
        return false;
    }

    return true;
}

bool RunNamedEventInfrastructureTest() {
    if (Host::SystemStateMonitor::kEventCount != Host::kSystemStateNamedEventCount) {
        std::cerr << "[TEST] kEventCount does not match kSystemStateNamedEventCount.\n";
        return false;
    }

    const auto& specs = Host::SystemStateMonitor::NamedEventSpecs();
    const auto& names = Host::SystemStateMonitor::NamedEventList();
    for (std::size_t i = 0; i < Host::SystemStateMonitor::kEventCount; ++i) {
        if (specs[i].name == nullptr || specs[i].name[0] == L'\0' || names[i] == nullptr) {
            std::cerr << "[TEST] Empty named event spec at index " << i << ".\n";
            return false;
        }
        if (specs[i].name != names[i]) {
            std::cerr << "[TEST] NamedEventSpecs/NamedEventList mismatch at index " << i << ".\n";
            return false;
        }
    }

    TestEventNames event_names(L"Infrastructure");
    TestEventHandles event_handles;
    if (!event_handles.OpenAll(event_names.List())) {
        std::cerr << "[TEST] Failed to create/open all named events in infrastructure test.\n";
        return false;
    }

    ResetAllNamedEventsBestEffort(event_names.List());
    if (!event_handles.Set(Host::SystemStateNamedEventId::MonitorConsoleDisplayOn) ||
        WaitForSingleObject(event_handles.handles[Host::ToIndex(Host::SystemStateNamedEventId::MonitorConsoleDisplayOn)], 0) != WAIT_OBJECT_0 ||
        !event_handles.Reset(Host::SystemStateNamedEventId::MonitorConsoleDisplayOn) ||
        WaitForSingleObject(event_handles.handles[Host::ToIndex(Host::SystemStateNamedEventId::MonitorConsoleDisplayOn)], 0) == WAIT_OBJECT_0) {
        std::cerr << "[TEST] Named event SetEvent/ResetEvent sanity check failed.\n";
        ResetAllNamedEventsBestEffort(event_names.List());
        return false;
    }

    ResetAllNamedEventsBestEffort(event_names.List());
    return true;
}

bool RunNamedEventSequenceTest() {
    using namespace std::chrono_literals;

    TestEventNames event_names(L"Sequence");
    ResetAllNamedEventsBestEffort(event_names.List());

    Host::SystemStateMonitor monitor(event_names.List());
    EventRecorder recorder;

    const bool started = monitor.Start([&](const Host::SystemStateEvent& event) {
        recorder.Record(event);
    });

    if (!started) {
        std::cerr << "[TEST] SystemStateMonitor start failed in sequence test.\n";
        return false;
    }

    std::this_thread::sleep_for(kWorkerWarmup);

    const std::vector<std::pair<Host::SystemStateNamedEventId, Host::SystemStateEventType>> script = {
        {Host::SystemStateNamedEventId::MonitorConsoleDisplayOff, Host::SystemStateEventType::DisplayOff},
        {Host::SystemStateNamedEventId::MonitorConsoleDisplayOn, Host::SystemStateEventType::DisplayOn},
        {Host::SystemStateNamedEventId::MonitorLidOff, Host::SystemStateEventType::LidOff},
        {Host::SystemStateNamedEventId::PbtApmSuspend, Host::SystemStateEventType::Suspend},
        {Host::SystemStateNamedEventId::PbtApmResumeAutomatic, Host::SystemStateEventType::ResumeAutomatic},
        {Host::SystemStateNamedEventId::PbtApmResumeSuspend, Host::SystemStateEventType::ResumeAutomatic},
    };

    for (std::size_t i = 0; i < script.size(); ++i) {
        if (!SignalAndWaitForCount(monitor, recorder, event_names.List(), script[i].first, i + 1, "sequence test")) {
            return false;
        }
        std::this_thread::sleep_for(30ms);
    }

    monitor.Stop();
    ResetAllNamedEventsBestEffort(event_names.List());

    std::vector<Host::SystemStateEventType> expected;
    expected.reserve(script.size());
    for (const auto& step : script) {
        expected.push_back(step.second);
    }

    return ExpectTypes(expected, recorder.Snapshot(), "sequence test");
}



bool RunStateNormalizationDedupTest() {
    using namespace std::chrono_literals;

    TestEventNames event_names(L"StateNormalizationDedup");
    ResetAllNamedEventsBestEffort(event_names.List());

    Host::SystemStateMonitor monitor(event_names.List());
    EventRecorder recorder;

    const bool started = monitor.Start([&](const Host::SystemStateEvent& event) {
        recorder.Record(event);
    });

    if (!started) {
        std::cerr << "[TEST] SystemStateMonitor start failed in state normalization test.\n";
        return false;
    }

    std::this_thread::sleep_for(kWorkerWarmup);

    if (!SignalAndWaitForCount(monitor, recorder, event_names.List(), Host::SystemStateNamedEventId::MonitorConsoleDisplayOn, 1, "state normalization test") ||
        !SignalAndWaitForCount(monitor, recorder, event_names.List(), Host::SystemStateNamedEventId::MonitorConsoleDisplayOff, 2, "DisplayOff transition test") ||
        !SignalAndWaitForCount(monitor, recorder, event_names.List(), Host::SystemStateNamedEventId::MonitorLidOn, 3, "LidOn transition test") ||
        !SignalAndExpectNoAdditionalCallback(monitor, recorder, event_names.List(), Host::SystemStateNamedEventId::MonitorLidOn, 3, "duplicate LidOn test") ||
        !SignalAndWaitForCount(monitor, recorder, event_names.List(), Host::SystemStateNamedEventId::MonitorShutDown, 4, "Shutdown transition test") ||
        !SignalAndExpectNoAdditionalCallback(monitor, recorder, event_names.List(), Host::SystemStateNamedEventId::MonitorShutDown, 4, "duplicate Shutdown test") ||
        !SignalAndWaitForCount(monitor, recorder, event_names.List(), Host::SystemStateNamedEventId::PbtApmResumeAutomatic, 5, "first ResumeAutomatic test") ||
        !SignalAndWaitForCount(monitor, recorder, event_names.List(), Host::SystemStateNamedEventId::PbtApmResumeSuspend, 6, "second ResumeAutomatic test")) {
        return false;
    }

    monitor.Stop();
    ResetAllNamedEventsBestEffort(event_names.List());

    const std::vector<Host::SystemStateEventType> expected = {
        Host::SystemStateEventType::DisplayOn,
        Host::SystemStateEventType::DisplayOff,
        Host::SystemStateEventType::LidOn,
        Host::SystemStateEventType::Shutdown,
        Host::SystemStateEventType::ResumeAutomatic,
        Host::SystemStateEventType::ResumeAutomatic,
    };

    return ExpectTypes(expected, recorder.Snapshot(), "state normalization test");
}

bool RunDisplayBurstCoalescingTest() {
    TestEventNames event_names(L"DisplayBurstCoalescing");
    ResetAllNamedEventsBestEffort(event_names.List());

    TestEventHandles event_handles;
    if (!event_handles.OpenAll(event_names.List())) {
        std::cerr << "[TEST] Failed to create named events in display burst coalescing test.\n";
        return false;
    }

    if (!event_handles.Set(Host::SystemStateNamedEventId::MonitorConsoleDisplayOff) ||
        !event_handles.Set(Host::SystemStateNamedEventId::MonitorConsoleDisplayOn)) {
        std::cerr << "[TEST] Failed to pre-signal display burst events.\n";
        ResetAllNamedEventsBestEffort(event_names.List());
        return false;
    }

    Host::SystemStateMonitor monitor(event_names.List());
    EventRecorder recorder;

    const bool started = monitor.Start([&](const Host::SystemStateEvent& event) {
        recorder.Record(event);
    });

    if (!started) {
        std::cerr << "[TEST] SystemStateMonitor start failed in display burst coalescing test.\n";
        ResetAllNamedEventsBestEffort(event_names.List());
        return false;
    }

    if (!recorder.WaitForCount(1)) {
        monitor.Stop();
        ResetAllNamedEventsBestEffort(event_names.List());
        std::cerr << "[TEST] Timeout waiting for display burst event.\n";
        return false;
    }

    if (recorder.WaitForCount(2, kNoExtraCallbackWindow)) {
        monitor.Stop();
        ResetAllNamedEventsBestEffort(event_names.List());
        std::cerr << "[TEST] Display burst emitted more than one event.\n";
        DumpObserved(recorder.Snapshot());
        return false;
    }

    monitor.Stop();
    ResetAllNamedEventsBestEffort(event_names.List());

    return ExpectTypes({Host::SystemStateEventType::DisplayOn}, recorder.Snapshot(), "display burst coalescing test");
}

bool RunCallbackExceptionContainmentTest() {
    using namespace std::chrono_literals;

    TestEventNames event_names(L"CallbackExceptionContainment");
    ResetAllNamedEventsBestEffort(event_names.List());

    Host::SystemStateMonitor monitor(event_names.List());

    std::atomic<int> callback_count{0};
    std::mutex mu;
    std::condition_variable cv;

    const bool started = monitor.Start([&](const Host::SystemStateEvent&) {
        const int n = callback_count.fetch_add(1, std::memory_order_acq_rel) + 1;
        if (n == 1) {
            throw std::runtime_error("intentional callback failure");
        }

        cv.notify_all();
    });

    if (!started) {
        std::cerr << "[TEST] SystemStateMonitor start failed in exception containment test.\n";
        return false;
    }

    std::this_thread::sleep_for(kWorkerWarmup);

    if (!Host::SystemStateMonitor::SignalNamedEvent(Host::SystemStateNamedEventId::MonitorConsoleDisplayOn, event_names.List())) {
        monitor.Stop();
        std::cerr << "[TEST] Failed first signal in exception containment test.\n";
        return false;
    }

    std::this_thread::sleep_for(30ms);

    if (!Host::SystemStateMonitor::SignalNamedEvent(Host::SystemStateNamedEventId::MonitorLidOn, event_names.List())) {
        monitor.Stop();
        std::cerr << "[TEST] Failed second signal in exception containment test.\n";
        return false;
    }

    {
        std::unique_lock<std::mutex> lock(mu);
        const bool received_after_throw = cv.wait_for(lock, kCallbackTimeout, [&] {
            return callback_count.load(std::memory_order_acquire) >= 2;
        });
        if (!received_after_throw) {
            monitor.Stop();
            std::cerr << "[TEST] Monitor did not process callback after exception. count="
                      << callback_count.load(std::memory_order_acquire) << "\n";
            return false;
        }
    }

    monitor.Stop();
    ResetAllNamedEventsBestEffort(event_names.List());
    return true;
}

bool RunCallbackReentrantStopTest() {
    using namespace std::chrono_literals;

    TestEventNames event_names(L"CallbackReentrantStop");
    ResetAllNamedEventsBestEffort(event_names.List());

    Host::SystemStateMonitor monitor(event_names.List());

    std::atomic<int> callback_count{0};
    std::mutex mu;
    std::condition_variable cv;

    const bool started = monitor.Start([&](const Host::SystemStateEvent&) {
        callback_count.fetch_add(1, std::memory_order_acq_rel);
        monitor.Stop();
        cv.notify_all();
    });

    if (!started) {
        std::cerr << "[TEST] SystemStateMonitor start failed in reentrant Stop test.\n";
        return false;
    }

    std::this_thread::sleep_for(kWorkerWarmup);

    if (!Host::SystemStateMonitor::SignalNamedEvent(Host::SystemStateNamedEventId::MonitorConsoleDisplayOn, event_names.List())) {
        monitor.Stop();
        std::cerr << "[TEST] Failed signal in reentrant Stop test.\n";
        return false;
    }

    {
        std::unique_lock<std::mutex> lock(mu);
        const bool callback_observed = cv.wait_for(lock, kCallbackTimeout, [&] {
            return callback_count.load(std::memory_order_acquire) >= 1;
        });
        if (!callback_observed) {
            monitor.Stop();
            std::cerr << "[TEST] Timeout waiting for callback in reentrant Stop test.\n";
            return false;
        }
    }

    monitor.Stop();
    ResetAllNamedEventsBestEffort(event_names.List());

    if (monitor.IsRunning()) {
        std::cerr << "[TEST] Monitor still running after external Stop following reentrant Stop.\n";
        return false;
    }

    if (!monitor.Start([&](const Host::SystemStateEvent&) {
            callback_count.fetch_add(1, std::memory_order_acq_rel);
            cv.notify_all();
        })) {
        std::cerr << "[TEST] Monitor restart failed after reentrant Stop cleanup.\n";
        return false;
    }

    std::this_thread::sleep_for(kWorkerWarmup);
    const int count_before_restart_signal = callback_count.load(std::memory_order_acquire);
    if (!Host::SystemStateMonitor::SignalNamedEvent(Host::SystemStateNamedEventId::MonitorLidOn, event_names.List())) {
        monitor.Stop();
        std::cerr << "[TEST] Failed signal after reentrant Stop restart.\n";
        return false;
    }

    {
        std::unique_lock<std::mutex> lock(mu);
        const bool callback_observed = cv.wait_for(lock, kCallbackTimeout, [&] {
            return callback_count.load(std::memory_order_acquire) > count_before_restart_signal;
        });
        if (!callback_observed) {
            monitor.Stop();
            std::cerr << "[TEST] Timeout waiting for callback after reentrant Stop restart.\n";
            return false;
        }
    }

    monitor.Stop();
    ResetAllNamedEventsBestEffort(event_names.List());
    return true;
}

bool RunCallbackStopThenDestructorTest() {
    using namespace std::chrono_literals;

    TestEventNames event_names(L"CallbackStopThenDestructor");
    ResetAllNamedEventsBestEffort(event_names.List());

    std::atomic<int> callback_count{0};
    std::mutex mu;
    std::condition_variable cv;

    {
        Host::SystemStateMonitor monitor(event_names.List());
        const bool started = monitor.Start([&](const Host::SystemStateEvent&) {
            callback_count.fetch_add(1, std::memory_order_acq_rel);
            monitor.Stop();
            cv.notify_all();
        });

        if (!started) {
            std::cerr << "[TEST] SystemStateMonitor start failed in reentrant Stop destructor test.\n";
            return false;
        }

        std::this_thread::sleep_for(kWorkerWarmup);
        if (!Host::SystemStateMonitor::SignalNamedEvent(Host::SystemStateNamedEventId::MonitorConsoleDisplayOff, event_names.List())) {
            monitor.Stop();
            std::cerr << "[TEST] Failed signal in reentrant Stop destructor test.\n";
            return false;
        }

        std::unique_lock<std::mutex> lock(mu);
        const bool callback_observed = cv.wait_for(lock, kCallbackTimeout, [&] {
            return callback_count.load(std::memory_order_acquire) >= 1;
        });
        if (!callback_observed) {
            monitor.Stop();
            std::cerr << "[TEST] Timeout waiting for callback in reentrant Stop destructor test.\n";
            return false;
        }
    }

    ResetAllNamedEventsBestEffort(event_names.List());
    return true;
}

} // namespace

int main() {
    if (!RunNamedEventInfrastructureTest()) {
        return 1;
    }

    if (!RunNamedEventSequenceTest()) {
        return 2;
    }

    if (!RunStateNormalizationDedupTest()) {
        return 3;
    }

    if (!RunDisplayBurstCoalescingTest()) {
        return 4;
    }

    if (!RunCallbackExceptionContainmentTest()) {
        return 5;
    }

    if (!RunCallbackReentrantStopTest()) {
        return 6;
    }

    if (!RunCallbackStopThenDestructorTest()) {
        return 7;
    }

    std::cout << "[TEST] SystemStateMonitor named-event tests passed.\n";
    return 0;
}
