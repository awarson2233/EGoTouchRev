#include "ServiceConfigCore.h"
#include "TestAssert.h"

#include <string>
#include <vector>

namespace {

struct FakeRuntime {
    bool startResult = true;
    std::vector<std::string>* calls = nullptr;

    void ApplyPolicy(const Service::ServiceConfigState&) { calls->push_back("runtime.applyPolicy"); }
    void BuildPipeline() { calls->push_back("runtime.buildPipeline"); }
    bool Start() { calls->push_back("runtime.start"); return startResult; }
    void Stop() { calls->push_back("runtime.stop"); }
};

struct FakeMonitor {
    bool startResult = true;
    std::vector<std::string>* calls = nullptr;
    bool started = false;

    void Start() {
        calls->push_back("monitor.start");
        started = startResult;
        if (!startResult) calls->push_back("monitor.reset");
    }
    void Stop() {
        if (started) calls->push_back("monitor.stop");
        started = false;
    }
};

struct FakeIpc {
    std::vector<std::string>* calls = nullptr;
    bool started = false;

    void Start() { calls->push_back("ipc.start"); started = true; }
    void Stop() {
        if (started) calls->push_back("ipc.stop");
        started = false;
    }
};

struct FakePen {
    std::vector<std::string>* calls = nullptr;
    bool eventStarted = false;
    bool pressureStarted = false;

    void Start(Service::ServiceMode mode) {
        if (mode != Service::ServiceMode::Full) {
            calls->push_back("pen.skip");
            return;
        }
        calls->push_back("penEvent.start");
        eventStarted = true;
        calls->push_back("penPressure.start");
        pressureStarted = true;
    }
    void Stop() {
        if (pressureStarted) calls->push_back("penPressure.stop");
        if (eventStarted) calls->push_back("penEvent.stop");
        pressureStarted = false;
        eventStarted = false;
    }
};

struct FakeServiceHostHarness {
    Service::ServiceConfigState config{};
    std::vector<std::string> calls;
    FakeRuntime runtime{true, &calls};
    FakeMonitor monitor{true, &calls};
    FakeIpc ipc{&calls};
    FakePen pen{&calls};
    bool started = false;

    bool Start() {
        runtime.ApplyPolicy(config);
        runtime.BuildPipeline();
        if (!runtime.Start()) return false;
        monitor.Start();
        ipc.Start();
        pen.Start(config.mode);
        started = true;
        return true;
    }

    void Stop() {
        ipc.Stop();
        pen.Stop();
        monitor.Stop();
        if (started) runtime.Stop();
        started = false;
    }
};

bool FullModeStartsAndStopsInServiceOrder() {
    FakeServiceHostHarness host;
    REQUIRE_TRUE(host.Start());
    host.Stop();

    const std::vector<std::string> expected{
        "runtime.applyPolicy",
        "runtime.buildPipeline",
        "runtime.start",
        "monitor.start",
        "ipc.start",
        "penEvent.start",
        "penPressure.start",
        "ipc.stop",
        "penPressure.stop",
        "penEvent.stop",
        "monitor.stop",
        "runtime.stop",
    };
    REQUIRE_TRUE(host.calls == expected);
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
        "monitor.start",
        "ipc.start",
        "pen.skip",
        "ipc.stop",
        "monitor.stop",
        "runtime.stop",
    };
    REQUIRE_TRUE(host.calls == expected);
    return true;
}

bool RuntimeStartFailureBlocksLaterSubsystems() {
    FakeServiceHostHarness host;
    host.runtime.startResult = false;
    REQUIRE_TRUE(!host.Start());

    const std::vector<std::string> expected{
        "runtime.applyPolicy",
        "runtime.buildPipeline",
        "runtime.start",
    };
    REQUIRE_TRUE(host.calls == expected);
    return true;
}

bool MonitorFailureDoesNotFailServiceStart() {
    FakeServiceHostHarness host;
    host.monitor.startResult = false;
    REQUIRE_TRUE(host.Start());

    const std::vector<std::string> expectedPrefix{
        "runtime.applyPolicy",
        "runtime.buildPipeline",
        "runtime.start",
        "monitor.start",
        "monitor.reset",
        "ipc.start",
        "penEvent.start",
        "penPressure.start",
    };
    REQUIRE_TRUE(host.calls == expectedPrefix);
    return true;
}

} // namespace

int main() {
    int failures = 0;
    failures += RunTest(&FullModeStartsAndStopsInServiceOrder, "FullModeStartsAndStopsInServiceOrder");
    failures += RunTest(&TouchOnlySkipsPenButKeepsCoreSubsystems, "TouchOnlySkipsPenButKeepsCoreSubsystems");
    failures += RunTest(&RuntimeStartFailureBlocksLaterSubsystems, "RuntimeStartFailureBlocksLaterSubsystems");
    failures += RunTest(&MonitorFailureDoesNotFailServiceStart, "MonitorFailureDoesNotFailServiceStart");
    return failures == 0 ? 0 : 1;
}
