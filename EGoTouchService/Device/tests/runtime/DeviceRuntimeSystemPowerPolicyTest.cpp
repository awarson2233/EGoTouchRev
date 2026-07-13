#include "runtime/DeviceRuntime.h"

#include <chrono>
#include <iostream>
#include <thread>

namespace {

RuntimePolicyEvent MakePolicyEvent(RuntimePolicyEvent::Type type) {
    RuntimePolicyEvent event{};
    event.type = type;
    event.timestamp = std::chrono::system_clock::now();
    return event;
}

bool WaitForState(DeviceRuntime& runtime,
                  workerState expected,
                  std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (runtime.GetSnapshot().state == expected) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return runtime.GetSnapshot().state == expected;
}

DeviceRuntime MakeRuntimeWithMissingHardware() {
    return DeviceRuntime(
        L"\\\\?\\EGoTouchMissingMaster",
        L"\\\\?\\EGoTouchMissingSlave",
        L"\\\\?\\EGoTouchMissingInterrupt");
}

bool RunResumeAfterSystemSuspendForcesReadyFromRecoverTest() {
    using namespace std::chrono_literals;

    auto runtime = MakeRuntimeWithMissingHardware();
    runtime.SetAutoMode(true);

    if (!runtime.Start()) {
        std::cerr << "[TEST] Failed to start runtime.\n";
        return false;
    }

    if (!WaitForState(runtime, workerState::recover, 2s)) {
        const auto snapshot = runtime.GetSnapshot();
        runtime.Stop();
        std::cerr << "[TEST] Runtime did not enter recover with missing hardware; state="
                  << ToString(snapshot.state) << "\n";
        return false;
    }

    runtime.SetAutoMode(false);
    runtime.IngestPolicyEvent(MakePolicyEvent(RuntimePolicyEvent::Type::Suspend));
    runtime.IngestPolicyEvent(MakePolicyEvent(RuntimePolicyEvent::Type::ResumeAutomatic));

    if (!WaitForState(runtime, workerState::ready, 2s)) {
        const auto snapshot = runtime.GetSnapshot();
        runtime.Stop();
        std::cerr << "[TEST] Resume after system suspend did not force ready; state="
                  << ToString(snapshot.state) << "\n";
        return false;
    }

    runtime.Stop();
    return true;
}

bool RunResumeWithoutSystemSuspendKeepsRecoverStateTest() {
    using namespace std::chrono_literals;

    auto runtime = MakeRuntimeWithMissingHardware();
    runtime.SetAutoMode(true);

    if (!runtime.Start()) {
        std::cerr << "[TEST] Failed to start runtime.\n";
        return false;
    }

    if (!WaitForState(runtime, workerState::recover, 2s)) {
        const auto snapshot = runtime.GetSnapshot();
        runtime.Stop();
        std::cerr << "[TEST] Runtime did not enter recover with missing hardware; state="
                  << ToString(snapshot.state) << "\n";
        return false;
    }

    runtime.SetAutoMode(false);
    runtime.IngestPolicyEvent(MakePolicyEvent(RuntimePolicyEvent::Type::ResumeAutomatic));
    std::this_thread::sleep_for(250ms);

    const auto snapshot = runtime.GetSnapshot();
    runtime.Stop();

    if (snapshot.state != workerState::recover) {
        std::cerr << "[TEST] Resume without system suspend unexpectedly changed state to "
                  << ToString(snapshot.state) << "\n";
        return false;
    }

    return true;
}

bool RunRestartAfterWorkerSelfExitTest() {
    using namespace std::chrono_literals;

    auto runtime = MakeRuntimeWithMissingHardware();
    runtime.SetAutoMode(true);

    if (!runtime.Start()) {
        std::cerr << "[TEST] Failed to start runtime before self-exit restart test.\n";
        return false;
    }

    if (!WaitForState(runtime, workerState::recover, 2s)) {
        const auto snapshot = runtime.GetSnapshot();
        runtime.Stop();
        std::cerr << "[TEST] Runtime did not enter recover before self-exit restart test; state="
                  << ToString(snapshot.state) << "\n";
        return false;
    }

    runtime.IngestPolicyEvent(MakePolicyEvent(RuntimePolicyEvent::Type::Shutdown));
    if (!WaitForState(runtime, workerState::quit, 2s)) {
        const auto snapshot = runtime.GetSnapshot();
        runtime.Stop();
        std::cerr << "[TEST] Runtime did not self-exit after Shutdown event; state="
                  << ToString(snapshot.state) << " running=" << runtime.IsRunning() << "\n";
        return false;
    }

    if (!runtime.Start()) {
        std::cerr << "[TEST] Restart failed after worker self-exit left joinable thread.\n";
        return false;
    }

    if (!WaitForState(runtime, workerState::recover, 2s)) {
        const auto snapshot = runtime.GetSnapshot();
        runtime.Stop();
        std::cerr << "[TEST] Restarted runtime did not re-enter recover; state="
                  << ToString(snapshot.state) << "\n";
        return false;
    }

    runtime.Stop();
    return true;
}

bool RunPenAfeRestartPlanOrderingTest() {
    RuntimePenState state{};
    state.hasConnection = true;
    state.connected = true;
    state.hasStylusId = true;
    state.stylusId = 7;

    const auto plan = BuildPenAfeCommandPlan(state);
    if (plan.count != 2 ||
        plan.commands[0].type != AFE_Command::InitStylus ||
        plan.commands[0].param != 0 ||
        plan.commands[1].type != AFE_Command::SetStylusId ||
        plan.commands[1].param != 7) {
        std::cerr << "[TEST] Pen AFE restart plan has incorrect command ordering.\n";
        return false;
    }

    state.connected = false;
    if (BuildPenAfeCommandPlan(state).count != 0) {
        std::cerr << "[TEST] Disconnected pen unexpectedly produced restart commands.\n";
        return false;
    }
    return true;
}

bool RunStoppedPenIngressDoesNotQueueAfeCommandsTest() {
    auto runtime = MakeRuntimeWithMissingHardware();
    runtime.Stop();

    if (runtime.SubmitExternalAfeCommand(AFE_Command::ClearStatus, 0)) {
        std::cerr << "[TEST] Stopped runtime accepted an external AFE command.\n";
        return false;
    }

    command staleCommand{AFE_Command::ClearStatus, 0};
    runtime.SubmitCommand(staleCommand, CommandSource::SystemPolicy, "stale-before-restart");
    if (runtime.GetSnapshot().queue_depth != 1) {
        std::cerr << "[TEST] Failed to seed stale command queue for restart test.\n";
        return false;
    }

    Himax::Pen::PenEvent connection{};
    connection.code = Himax::Pen::PenUsbEventCode::PenConnStatus;
    connection.semantic.hasConnection = true;
    connection.semantic.connected = true;
    runtime.IngestPenEvent(connection);

    Himax::Pen::PenEvent penType{};
    penType.code = Himax::Pen::PenUsbEventCode::PenTypeInfo;
    penType.semantic.hasStylusId = true;
    penType.semantic.stylusId = 9;
    runtime.IngestPenEvent(penType);

    const auto stoppedSnapshot = runtime.GetSnapshot();
    const auto penState = runtime.GetPenStateSnapshot();
    if (stoppedSnapshot.queue_depth != 1 ||
        !penState.hasConnection || !penState.connected ||
        !penState.hasStylusId || penState.stylusId != 9) {
        std::cerr << "[TEST] Stopped pen ingress did not isolate AFE work from state updates.\n";
        return false;
    }

    runtime.SetAutoMode(false);
    if (!runtime.Start()) {
        std::cerr << "[TEST] Runtime failed to restart while testing stale queue cleanup.\n";
        return false;
    }
    const auto restartedSnapshot = runtime.GetSnapshot();
    runtime.Stop();

    if (restartedSnapshot.queue_depth != 0) {
        std::cerr << "[TEST] Runtime restart retained stale AFE commands.\n";
        return false;
    }
    return true;
}

} // namespace

int main() {
    if (!RunResumeAfterSystemSuspendForcesReadyFromRecoverTest()) {
        return 1;
    }

    if (!RunResumeWithoutSystemSuspendKeepsRecoverStateTest()) {
        return 2;
    }

    if (!RunRestartAfterWorkerSelfExitTest()) {
        return 3;
    }

    if (!RunPenAfeRestartPlanOrderingTest()) {
        return 4;
    }

    if (!RunStoppedPenIngressDoesNotQueueAfeCommandsTest()) {
        return 5;
    }

    std::cout << "[TEST] DeviceRuntime system power policy tests passed.\n";
    return 0;
}
