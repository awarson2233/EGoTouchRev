#include "ServiceConfigCore.h"
#include "ServiceLifecycleCoordinator.h"
#include "SystemStateEvent.h"
#include "TestAssert.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

using namespace std::chrono_literals;

class CallLog {
public:
    void Add(std::string call) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_calls.push_back(std::move(call));
        }
        m_cv.notify_all();
    }

    std::vector<std::string> Snapshot() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_calls;
    }

    bool WaitFor(const std::string& call) {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_cv.wait_for(lock, 2s, [&] {
            return std::find(m_calls.begin(), m_calls.end(), call) != m_calls.end();
        });
    }

private:
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::vector<std::string> m_calls;
};

class StageBarrier {
public:
    void ArriveAndWait() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_arrived = true;
        m_cv.notify_all();
        m_cv.wait(lock, [&] { return m_released; });
    }

    bool WaitUntilArrived() {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_cv.wait_for(lock, 2s, [&] { return m_arrived; });
    }

    void Release() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_released = true;
        }
        m_cv.notify_all();
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_arrived = false;
    bool m_released = false;
};

bool Contains(const std::vector<std::string>& calls, const std::string& call) {
    return std::find(calls.begin(), calls.end(), call) != calls.end();
}

std::size_t IndexOf(const std::vector<std::string>& calls, const std::string& call) {
    const auto it = std::find(calls.begin(), calls.end(), call);
    return it == calls.end() ? calls.size() : static_cast<std::size_t>(it - calls.begin());
}

bool WaitForPendingStop(Service::ServiceLifecycleStateMachine& lifecycle) {
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (std::chrono::steady_clock::now() < deadline) {
        if (lifecycle.HasPendingStop()) {
            return true;
        }
        std::this_thread::yield();
    }
    return false;
}

struct FakeRuntime {
    bool startResult = true;
    CallLog* calls = nullptr;

    void ApplyPolicy(const Service::ServiceConfigState&) { calls->Add("runtime.applyPolicy"); }
    void BuildPipeline() { calls->Add("runtime.buildPipeline"); }
    bool Start() { calls->Add("runtime.start"); return startResult; }
    void HandlePolicyEvent() { calls->Add("runtime.policyEvent"); }
    void Stop() { calls->Add("runtime.stop"); }
};

class FakeMonitor {
public:
    using Callback = std::function<void(Host::SystemStateEventType)>;

    explicit FakeMonitor(CallLog* calls) : m_calls(calls) {}
    ~FakeMonitor() {
        ReleaseCallback();
        if (m_callbackThread.joinable()) m_callbackThread.join();
    }

    bool startResult = true;
    StageBarrier* readyBarrier = nullptr;

    bool Start(Callback callback) {
        m_calls->Add("monitor.start");
        m_callback = std::move(callback);
        m_started = startResult;
        if (!startResult) {
            m_calls->Add("monitor.reset");
            return false;
        }
        if (readyBarrier) {
            m_calls->Add("monitor.ready.wait");
            readyBarrier->ArriveAndWait();
            m_calls->Add("monitor.ready");
        }
        return true;
    }

    void Emit(Host::SystemStateEventType event) {
        if (m_started && m_callback) m_callback(event);
    }

    void BeginInFlightCallback(Host::SystemStateEventType event) {
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            m_blockInsideCallback = true;
            m_releaseCallback = false;
        }
        m_callbackThread = std::thread([this, event] {
            if (m_callback) m_callback(event);
        });
    }

    void WaitInsideCallbackIfRequested() {
        std::unique_lock<std::mutex> lock(m_callbackMutex);
        if (!m_blockInsideCallback) return;
        m_calls->Add("monitor.callback.enter");
        m_callbackCv.wait(lock, [&] { return m_releaseCallback; });
        m_calls->Add("monitor.callback.exit");
        m_blockInsideCallback = false;
    }

    void ReleaseCallback() {
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            m_releaseCallback = true;
        }
        m_callbackCv.notify_all();
    }

    void Stop() {
        if (!m_started) return;
        m_calls->Add("monitor.stop.begin");
        if (m_callbackThread.joinable()) m_callbackThread.join();
        m_started = false;
        m_calls->Add("monitor.stop");
    }

private:
    CallLog* m_calls;
    Callback m_callback;
    bool m_started = false;
    std::thread m_callbackThread;
    std::mutex m_callbackMutex;
    std::condition_variable m_callbackCv;
    bool m_blockInsideCallback = false;
    bool m_releaseCallback = false;
};

class FakeIpc {
public:
    FakeIpc(CallLog* calls, bool* penNotifyHandleOpen)
        : m_calls(calls), m_penNotifyHandleOpen(penNotifyHandleOpen) {}
    ~FakeIpc() {
        ReleaseHandler();
        if (m_handlerThread.joinable()) m_handlerThread.join();
    }

    bool resourcesStartResult = true;
    bool serverStartResult = true;
    bool throwAfterResources = false;
    StageBarrier* readyBarrier = nullptr;

    bool Start() {
        m_calls->Add("ipc.resources.start");
        if (!resourcesStartResult) {
            m_calls->Add("ipc.resources.start.fail");
            return false;
        }
        m_resourcesOpen = true;
        *m_penNotifyHandleOpen = true;
        m_calls->Add("pen.notify.handle.create");
        if (throwAfterResources) {
            m_calls->Add("ipc.resources.start.throw");
            throw std::runtime_error("injected IPC startup failure");
        }
        m_calls->Add("ipc.server.start");
        if (!serverStartResult) {
            m_calls->Add("ipc.server.start.fail");
            return false;
        }
        m_serverStarted = true;
        if (readyBarrier) {
            m_calls->Add("ipc.ready.wait");
            readyBarrier->ArriveAndWait();
            m_calls->Add("ipc.ready");
        }
        return true;
    }

    void BeginInFlightHandler() {
        m_handlerThread = std::thread([this] {
            m_calls->Add("ipc.handler.enter");
            {
                std::unique_lock<std::mutex> lock(m_handlerMutex);
                m_handlerCv.wait(lock, [&] { return m_releaseHandler; });
            }
            m_calls->Add("ipc.handler.exit");
        });
    }

    void ReleaseHandler() {
        {
            std::lock_guard<std::mutex> lock(m_handlerMutex);
            m_releaseHandler = true;
        }
        m_handlerCv.notify_all();
    }

    void StopServer() {
        if (!m_serverStarted) return;
        m_calls->Add("ipc.server.stop.begin");
        if (m_handlerThread.joinable()) m_handlerThread.join();
        m_serverStarted = false;
        m_calls->Add("ipc.server.stop");
    }

    void CloseResources() {
        if (!m_resourcesOpen) return;
        m_resourcesOpen = false;
        m_calls->Add("ipc.resources.close");
    }

    bool HasOpenResources() const { return m_resourcesOpen; }

private:
    CallLog* m_calls;
    bool* m_penNotifyHandleOpen;
    bool m_resourcesOpen = false;
    bool m_serverStarted = false;
    std::thread m_handlerThread;
    std::mutex m_handlerMutex;
    std::condition_variable m_handlerCv;
    bool m_releaseHandler = false;
};

struct FakePen {
    CallLog* calls = nullptr;
    bool startResult = true;
    bool eventStarted = false;
    bool pressureStarted = false;
    bool notifyAttached = false;
    StageBarrier* beforePublishBarrier = nullptr;

    bool Start(Service::ServiceMode mode) {
        if (mode != Service::ServiceMode::Full) {
            calls->Add("pen.skip");
            return true;
        }
        calls->Add("pen.notify.attach");
        notifyAttached = true;
        calls->Add("penEvent.start");
        eventStarted = true;
        calls->Add("penPressure.start");
        if (!startResult) {
            calls->Add("penPressure.start.fail");
            return false;
        }
        pressureStarted = true;
        if (beforePublishBarrier) {
            calls->Add("pen.publish.wait");
            beforePublishBarrier->ArriveAndWait();
            calls->Add("pen.publish");
        }
        return true;
    }

    void SendQueryPenStatus() {
        if (eventStarted) calls->Add("penEvent.query.0x7101");
    }

    void Stop() {
        if (notifyAttached) calls->Add("pen.notify.detach");
        notifyAttached = false;
        if (pressureStarted) calls->Add("penPressure.stop");
        if (eventStarted) calls->Add("penEvent.stop");
        pressureStarted = false;
        eventStarted = false;
    }
};

struct FakeServiceHostHarness {
    Service::ServiceLifecycleStateMachine lifecycle;
    Service::ServiceConfigState config{};
    CallLog calls;
    FakeRuntime runtime{true, &calls};
    FakeMonitor monitor{&calls};
    bool penNotifyHandleOpen = false;
    FakeIpc ipc{&calls, &penNotifyHandleOpen};
    FakePen pen{&calls};
    bool started = false;

    bool StartRuntimeAndPipeline() {
        runtime.ApplyPolicy(config);
        runtime.BuildPipeline();
        started = runtime.Start();
        if (!started) {
            runtime.Stop();
        }
        return started;
    }

    bool StartIpcSubsystem() { return ipc.Start(); }
    bool StartPenSubsystem() { return pen.Start(config.mode); }
    bool StartSystemStateMonitor() {
        return monitor.Start([this](Host::SystemStateEventType event) {
            runtime.HandlePolicyEvent();
            if (config.mode == Service::ServiceMode::Full &&
                Host::IsPenStatusWakeEvent(event)) {
                pen.SendQueryPenStatus();
            }
            monitor.WaitInsideCallbackIfRequested();
        });
    }

    void StopSystemStateMonitor() { monitor.Stop(); }
    void StopIpcServer() { ipc.StopServer(); }
    void StopPenSubsystem() {
        pen.Stop();
        if (penNotifyHandleOpen) {
            penNotifyHandleOpen = false;
            calls.Add("pen.notify.handle.close");
        }
    }
    void CloseIpcResources() { ipc.CloseResources(); }
    void StopRuntimeSubsystem() {
        if (started) runtime.Stop();
        started = false;
    }

    bool Start() {
        return lifecycle.RunStart([this] {
            return Service::ServiceLifecycleCoordinator::Start(*this);
        });
    }
    void Stop() {
        lifecycle.RunStop([this] {
            Service::ServiceLifecycleCoordinator::Stop(*this);
        });
    }
};

bool FullModeStartsAndStopsInRaceFreeOrder() {
    FakeServiceHostHarness host;
    REQUIRE_TRUE(host.Start());
    host.Stop();
    host.Stop();

    const std::vector<std::string> expected{
        "runtime.applyPolicy",
        "runtime.buildPipeline",
        "runtime.start",
        "ipc.resources.start",
        "pen.notify.handle.create",
        "ipc.server.start",
        "pen.notify.attach",
        "penEvent.start",
        "penPressure.start",
        "monitor.start",
        "monitor.stop.begin",
        "monitor.stop",
        "ipc.server.stop.begin",
        "ipc.server.stop",
        "pen.notify.detach",
        "penPressure.stop",
        "penEvent.stop",
        "pen.notify.handle.close",
        "ipc.resources.close",
        "runtime.stop",
    };
    REQUIRE_TRUE(host.calls.Snapshot() == expected);
    return true;
}

bool TouchOnlySkipsPenButKeepsCoreSubsystems() {
    FakeServiceHostHarness host;
    host.config.mode = Service::ServiceMode::TouchOnly;
    REQUIRE_TRUE(host.Start());
    host.Stop();

    const std::vector<std::string> expected{
        "runtime.applyPolicy",
        "runtime.buildPipeline",
        "runtime.start",
        "ipc.resources.start",
        "pen.notify.handle.create",
        "ipc.server.start",
        "pen.skip",
        "monitor.start",
        "monitor.stop.begin",
        "monitor.stop",
        "ipc.server.stop.begin",
        "ipc.server.stop",
        "pen.notify.handle.close",
        "ipc.resources.close",
        "runtime.stop",
    };
    REQUIRE_TRUE(host.calls.Snapshot() == expected);
    return true;
}

bool RuntimeStartFailureRollsBackRuntimeObject() {
    FakeServiceHostHarness host;
    host.runtime.startResult = false;
    REQUIRE_TRUE(!host.Start());

    const std::vector<std::string> expected{
        "runtime.applyPolicy",
        "runtime.buildPipeline",
        "runtime.start",
        "runtime.stop",
    };
    REQUIRE_TRUE(host.calls.Snapshot() == expected);
    return true;
}

bool IpcResourceFailureRollsBackRuntime() {
    FakeServiceHostHarness host;
    host.ipc.resourcesStartResult = false;
    REQUIRE_TRUE(!host.Start());

    const std::vector<std::string> expected{
        "runtime.applyPolicy",
        "runtime.buildPipeline",
        "runtime.start",
        "ipc.resources.start",
        "ipc.resources.start.fail",
        "runtime.stop",
    };
    REQUIRE_TRUE(host.calls.Snapshot() == expected);
    return true;
}

bool IpcServerFailureClosesResourcesAndRuntime() {
    FakeServiceHostHarness host;
    host.ipc.serverStartResult = false;
    REQUIRE_TRUE(!host.Start());

    const std::vector<std::string> expected{
        "runtime.applyPolicy",
        "runtime.buildPipeline",
        "runtime.start",
        "ipc.resources.start",
        "pen.notify.handle.create",
        "ipc.server.start",
        "ipc.server.start.fail",
        "pen.notify.handle.close",
        "ipc.resources.close",
        "runtime.stop",
    };
    REQUIRE_TRUE(host.calls.Snapshot() == expected);
    return true;
}

bool IpcStartupExceptionStillClosesResourcesAndRuntime() {
    FakeServiceHostHarness host;
    host.ipc.throwAfterResources = true;
    REQUIRE_TRUE(!host.Start());

    const std::vector<std::string> expected{
        "runtime.applyPolicy",
        "runtime.buildPipeline",
        "runtime.start",
        "ipc.resources.start",
        "pen.notify.handle.create",
        "ipc.resources.start.throw",
        "pen.notify.handle.close",
        "ipc.resources.close",
        "runtime.stop",
    };
    REQUIRE_TRUE(host.calls.Snapshot() == expected);
    return true;
}

bool IpcReadinessFailureCanRetryWithoutLeakingResources() {
    FakeServiceHostHarness host;
    host.ipc.serverStartResult = false;

    REQUIRE_TRUE(!host.Start());
    REQUIRE_TRUE(!host.penNotifyHandleOpen);
    REQUIRE_TRUE(!host.ipc.HasOpenResources());

    REQUIRE_TRUE(!host.Start());
    REQUIRE_TRUE(!host.penNotifyHandleOpen);
    REQUIRE_TRUE(!host.ipc.HasOpenResources());

    host.ipc.serverStartResult = true;
    REQUIRE_TRUE(host.Start());
    REQUIRE_TRUE(host.penNotifyHandleOpen);
    REQUIRE_TRUE(host.ipc.HasOpenResources());
    host.Stop();

    REQUIRE_TRUE(!host.penNotifyHandleOpen);
    REQUIRE_TRUE(!host.ipc.HasOpenResources());
    const auto calls = host.calls.Snapshot();
    REQUIRE_TRUE(std::count(calls.begin(), calls.end(), "pen.notify.handle.create") == 3);
    REQUIRE_TRUE(std::count(calls.begin(), calls.end(), "pen.notify.handle.close") == 3);
    REQUIRE_TRUE(std::count(calls.begin(), calls.end(), "ipc.resources.close") == 3);
    REQUIRE_TRUE(std::count(calls.begin(), calls.end(), "ipc.server.start.fail") == 2);
    return true;
}

bool PenFailureGatesIpcThenRollsBackPartialPen() {
    FakeServiceHostHarness host;
    host.pen.startResult = false;
    REQUIRE_TRUE(!host.Start());

    const std::vector<std::string> expected{
        "runtime.applyPolicy",
        "runtime.buildPipeline",
        "runtime.start",
        "ipc.resources.start",
        "pen.notify.handle.create",
        "ipc.server.start",
        "pen.notify.attach",
        "penEvent.start",
        "penPressure.start",
        "penPressure.start.fail",
        "ipc.server.stop.begin",
        "ipc.server.stop",
        "pen.notify.detach",
        "penEvent.stop",
        "pen.notify.handle.close",
        "ipc.resources.close",
        "runtime.stop",
    };
    REQUIRE_TRUE(host.calls.Snapshot() == expected);
    return true;
}

bool MonitorFailureRollsBackAllCompletedStages() {
    FakeServiceHostHarness host;
    host.monitor.startResult = false;
    REQUIRE_TRUE(!host.Start());

    const std::vector<std::string> expected{
        "runtime.applyPolicy",
        "runtime.buildPipeline",
        "runtime.start",
        "ipc.resources.start",
        "pen.notify.handle.create",
        "ipc.server.start",
        "pen.notify.attach",
        "penEvent.start",
        "penPressure.start",
        "monitor.start",
        "monitor.reset",
        "ipc.server.stop.begin",
        "ipc.server.stop",
        "pen.notify.detach",
        "penPressure.stop",
        "penEvent.stop",
        "pen.notify.handle.close",
        "ipc.resources.close",
        "runtime.stop",
    };
    REQUIRE_TRUE(host.calls.Snapshot() == expected);
    return true;
}

bool WakeCallbacksIssuePenStatusQueryOnlyInFullMode() {
    FakeServiceHostHarness fullHost;
    REQUIRE_TRUE(fullHost.Start());
    fullHost.monitor.Emit(Host::SystemStateEventType::DisplayOn);
    fullHost.monitor.Emit(Host::SystemStateEventType::LidOn);
    fullHost.monitor.Emit(Host::SystemStateEventType::ResumeAutomatic);
    fullHost.monitor.Emit(Host::SystemStateEventType::Unknown);

    const auto fullCalls = fullHost.calls.Snapshot();
    REQUIRE_TRUE(std::count(fullCalls.begin(), fullCalls.end(),
                            "penEvent.query.0x7101") == 3);

    FakeServiceHostHarness touchOnlyHost;
    touchOnlyHost.config.mode = Service::ServiceMode::TouchOnly;
    REQUIRE_TRUE(touchOnlyHost.Start());
    touchOnlyHost.monitor.Emit(Host::SystemStateEventType::DisplayOn);
    REQUIRE_TRUE(!Contains(touchOnlyHost.calls.Snapshot(), "penEvent.query.0x7101"));
    return true;
}

bool StopWaitsForPenLocalProducersToPublish() {
    FakeServiceHostHarness host;
    StageBarrier beforePublish;
    host.pen.beforePublishBarrier = &beforePublish;

    bool startResult = false;
    std::thread startThread([&] { startResult = host.Start(); });
    const bool reachedPublishBarrier = beforePublish.WaitUntilArrived();

    std::thread stopThread([&] { host.Stop(); });
    const bool stopPending = WaitForPendingStop(host.lifecycle);
    const auto blockedCalls = host.calls.Snapshot();

    beforePublish.Release();
    startThread.join();
    stopThread.join();

    REQUIRE_TRUE(reachedPublishBarrier);
    REQUIRE_TRUE(stopPending);
    REQUIRE_TRUE(!Contains(blockedCalls, "monitor.start"));
    REQUIRE_TRUE(!Contains(blockedCalls, "ipc.server.stop.begin"));
    REQUIRE_TRUE(!Contains(blockedCalls, "pen.notify.detach"));
    REQUIRE_TRUE(startResult);

    const auto calls = host.calls.Snapshot();
    REQUIRE_TRUE(IndexOf(calls, "pen.publish.wait") < IndexOf(calls, "pen.publish"));
    REQUIRE_TRUE(IndexOf(calls, "pen.publish") < IndexOf(calls, "monitor.start"));
    REQUIRE_TRUE(IndexOf(calls, "monitor.start") < IndexOf(calls, "monitor.stop.begin"));
    REQUIRE_TRUE(IndexOf(calls, "penEvent.stop") < IndexOf(calls, "pen.notify.handle.close"));
    REQUIRE_TRUE(!host.pen.eventStarted && !host.pen.pressureStarted);
    REQUIRE_TRUE(!host.penNotifyHandleOpen && !host.ipc.HasOpenResources());
    REQUIRE_TRUE(!host.started);
    return true;
}

bool StopWaitsForIpcAndMonitorStartupStages() {
    {
        FakeServiceHostHarness host;
        StageBarrier ipcReady;
        host.ipc.readyBarrier = &ipcReady;

        bool startResult = false;
        std::thread startThread([&] { startResult = host.Start(); });
        const bool reachedIpcBarrier = ipcReady.WaitUntilArrived();
        std::thread stopThread([&] { host.Stop(); });
        const bool stopPending = WaitForPendingStop(host.lifecycle);
        const auto blockedCalls = host.calls.Snapshot();

        ipcReady.Release();
        startThread.join();
        stopThread.join();

        REQUIRE_TRUE(reachedIpcBarrier);
        REQUIRE_TRUE(stopPending);
        REQUIRE_TRUE(!Contains(blockedCalls, "pen.notify.attach"));
        REQUIRE_TRUE(!Contains(blockedCalls, "ipc.server.stop.begin"));
        REQUIRE_TRUE(startResult);
        REQUIRE_TRUE(!host.penNotifyHandleOpen && !host.ipc.HasOpenResources());
        REQUIRE_TRUE(!host.started);
    }

    {
        FakeServiceHostHarness host;
        StageBarrier monitorReady;
        host.monitor.readyBarrier = &monitorReady;

        bool startResult = false;
        std::thread startThread([&] { startResult = host.Start(); });
        const bool reachedMonitorBarrier = monitorReady.WaitUntilArrived();
        std::thread stopThread([&] { host.Stop(); });
        const bool stopPending = WaitForPendingStop(host.lifecycle);
        const auto blockedCalls = host.calls.Snapshot();

        monitorReady.Release();
        startThread.join();
        stopThread.join();

        REQUIRE_TRUE(reachedMonitorBarrier);
        REQUIRE_TRUE(stopPending);
        REQUIRE_TRUE(Contains(blockedCalls, "penPressure.start"));
        REQUIRE_TRUE(!Contains(blockedCalls, "monitor.stop.begin"));
        REQUIRE_TRUE(!Contains(blockedCalls, "pen.notify.detach"));
        REQUIRE_TRUE(startResult);
        REQUIRE_TRUE(!host.penNotifyHandleOpen && !host.ipc.HasOpenResources());
        REQUIRE_TRUE(!host.started);
    }
    return true;
}

bool ConcurrentStartsAndStopsAreIdempotent() {
    FakeServiceHostHarness host;
    StageBarrier beforePublish;
    host.pen.beforePublishBarrier = &beforePublish;

    bool firstStartResult = false;
    bool secondStartResult = false;
    std::thread firstStart([&] { firstStartResult = host.Start(); });
    const bool reachedPublishBarrier = beforePublish.WaitUntilArrived();
    std::thread secondStart([&] { secondStartResult = host.Start(); });

    beforePublish.Release();
    firstStart.join();
    secondStart.join();

    REQUIRE_TRUE(reachedPublishBarrier);
    REQUIRE_TRUE(firstStartResult && secondStartResult);
    auto calls = host.calls.Snapshot();
    REQUIRE_TRUE(std::count(calls.begin(), calls.end(), "runtime.start") == 1);
    REQUIRE_TRUE(std::count(calls.begin(), calls.end(), "ipc.server.start") == 1);
    REQUIRE_TRUE(std::count(calls.begin(), calls.end(), "penEvent.start") == 1);
    REQUIRE_TRUE(std::count(calls.begin(), calls.end(), "monitor.start") == 1);

    host.monitor.BeginInFlightCallback(Host::SystemStateEventType::Unknown);
    const bool callbackEntered = host.calls.WaitFor("monitor.callback.enter");
    std::thread firstStop([&] { host.Stop(); });
    const bool stopStarted = host.calls.WaitFor("monitor.stop.begin");
    std::thread secondStop([&] { host.Stop(); });

    host.monitor.ReleaseCallback();
    firstStop.join();
    secondStop.join();

    REQUIRE_TRUE(callbackEntered);
    REQUIRE_TRUE(stopStarted);
    calls = host.calls.Snapshot();
    REQUIRE_TRUE(std::count(calls.begin(), calls.end(), "monitor.stop.begin") == 1);
    REQUIRE_TRUE(std::count(calls.begin(), calls.end(), "ipc.server.stop.begin") == 1);
    REQUIRE_TRUE(std::count(calls.begin(), calls.end(), "penEvent.stop") == 1);
    REQUIRE_TRUE(std::count(calls.begin(), calls.end(), "runtime.stop") == 1);
    REQUIRE_TRUE(!host.penNotifyHandleOpen && !host.ipc.HasOpenResources());
    REQUIRE_TRUE(!host.started);
    return true;
}

bool StopWaitsForMonitorAndIpcBeforePenTeardown() {
    FakeServiceHostHarness host;
    REQUIRE_TRUE(host.Start());

    host.monitor.BeginInFlightCallback(Host::SystemStateEventType::ResumeAutomatic);
    REQUIRE_TRUE(host.calls.WaitFor("monitor.callback.enter"));
    REQUIRE_TRUE(host.calls.WaitFor("penEvent.query.0x7101"));
    host.ipc.BeginInFlightHandler();
    REQUIRE_TRUE(host.calls.WaitFor("ipc.handler.enter"));

    std::thread stopThread([&] { host.Stop(); });
    REQUIRE_TRUE(host.calls.WaitFor("monitor.stop.begin"));
    auto calls = host.calls.Snapshot();
    REQUIRE_TRUE(!Contains(calls, "ipc.server.stop.begin"));
    REQUIRE_TRUE(!Contains(calls, "pen.notify.detach"));

    host.monitor.ReleaseCallback();
    REQUIRE_TRUE(host.calls.WaitFor("ipc.server.stop.begin"));
    calls = host.calls.Snapshot();
    REQUIRE_TRUE(!Contains(calls, "pen.notify.detach"));

    host.ipc.ReleaseHandler();
    stopThread.join();
    calls = host.calls.Snapshot();

    REQUIRE_TRUE(IndexOf(calls, "monitor.callback.exit") < IndexOf(calls, "monitor.stop"));
    REQUIRE_TRUE(IndexOf(calls, "monitor.stop") < IndexOf(calls, "ipc.server.stop.begin"));
    REQUIRE_TRUE(IndexOf(calls, "ipc.handler.exit") < IndexOf(calls, "ipc.server.stop"));
    REQUIRE_TRUE(IndexOf(calls, "ipc.server.stop") < IndexOf(calls, "pen.notify.detach"));
    REQUIRE_TRUE(IndexOf(calls, "penEvent.stop") < IndexOf(calls, "pen.notify.handle.close"));
    REQUIRE_TRUE(IndexOf(calls, "pen.notify.handle.close") < IndexOf(calls, "ipc.resources.close"));
    REQUIRE_TRUE(IndexOf(calls, "ipc.resources.close") < IndexOf(calls, "runtime.stop"));
    return true;
}

} // namespace

int main() {
    int failures = 0;
    failures += RunTest(&FullModeStartsAndStopsInRaceFreeOrder, "FullModeStartsAndStopsInRaceFreeOrder");
    failures += RunTest(&TouchOnlySkipsPenButKeepsCoreSubsystems, "TouchOnlySkipsPenButKeepsCoreSubsystems");
    failures += RunTest(&RuntimeStartFailureRollsBackRuntimeObject, "RuntimeStartFailureRollsBackRuntimeObject");
    failures += RunTest(&IpcResourceFailureRollsBackRuntime, "IpcResourceFailureRollsBackRuntime");
    failures += RunTest(&IpcServerFailureClosesResourcesAndRuntime, "IpcServerFailureClosesResourcesAndRuntime");
    failures += RunTest(&IpcStartupExceptionStillClosesResourcesAndRuntime, "IpcStartupExceptionStillClosesResourcesAndRuntime");
    failures += RunTest(&IpcReadinessFailureCanRetryWithoutLeakingResources, "IpcReadinessFailureCanRetryWithoutLeakingResources");
    failures += RunTest(&PenFailureGatesIpcThenRollsBackPartialPen, "PenFailureGatesIpcThenRollsBackPartialPen");
    failures += RunTest(&MonitorFailureRollsBackAllCompletedStages, "MonitorFailureRollsBackAllCompletedStages");
    failures += RunTest(&WakeCallbacksIssuePenStatusQueryOnlyInFullMode, "WakeCallbacksIssuePenStatusQueryOnlyInFullMode");
    failures += RunTest(&StopWaitsForPenLocalProducersToPublish, "StopWaitsForPenLocalProducersToPublish");
    failures += RunTest(&StopWaitsForIpcAndMonitorStartupStages, "StopWaitsForIpcAndMonitorStartupStages");
    failures += RunTest(&ConcurrentStartsAndStopsAreIdempotent, "ConcurrentStartsAndStopsAreIdempotent");
    failures += RunTest(&StopWaitsForMonitorAndIpcBeforePenTeardown, "StopWaitsForMonitorAndIpcBeforePenTeardown");
    return failures == 0 ? 0 : 1;
}
