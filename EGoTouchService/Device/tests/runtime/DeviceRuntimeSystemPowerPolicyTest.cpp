#include "runtime/DeviceRuntime.h"

#include <atomic>
#include <barrier>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

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

bool RunSuspendResumeSchedulesFreshPenReplayTest() {
    using namespace std::chrono_literals;

    auto runtime = std::make_unique<DeviceRuntime>(
        L"\\\\?\\EGoTouchMissingMaster",
        L"\\\\?\\EGoTouchMissingSlave",
        L"\\\\?\\EGoTouchMissingInterrupt");
    runtime->SetAutoMode(false);
    if (!runtime->Start()) {
        std::cerr << "[TEST] Failed to start manual runtime for replay test.\n";
        return false;
    }

    const auto beforeSuspend = runtime->GetPenAfeReplayStateSnapshot();
    runtime->IngestPolicyEvent(MakePolicyEvent(RuntimePolicyEvent::Type::Suspend));
    if (!WaitForState(*runtime, workerState::suspend, 2s)) {
        runtime->Stop();
        std::cerr << "[TEST] Runtime did not enter suspend before replay test.\n";
        return false;
    }

    runtime->SetAutoMode(true);
    runtime->IngestPolicyEvent(MakePolicyEvent(RuntimePolicyEvent::Type::ResumeAutomatic));
    if (!WaitForState(*runtime, workerState::recover, 2s)) {
        runtime->Stop();
        std::cerr << "[TEST] Resumed runtime did not attempt chip reinitialization.\n";
        return false;
    }

    const auto afterResume = runtime->GetPenAfeReplayStateSnapshot();
    runtime->Stop();
    if (beforeSuspend.pending || !afterResume.pending ||
        afterResume.generation == beforeSuspend.generation) {
        std::cerr << "[TEST] Suspend/resume did not schedule a fresh pen replay generation.\n";
        return false;
    }
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

bool RunPenReplayCoversResumeAndRecoverInitCyclesTest() {
    PenAfeReplayState replay{};

    replay.BeginInitCycle(); // initial start
    const uint64_t initialGeneration = replay.generation;
    if (!replay.pending || replay.IsCurrent(initialGeneration)) {
        std::cerr << "[TEST] Initial init cycle did not suppress in-flight pen commands.\n";
        return false;
    }
    replay.CompleteInitCycle();
    if (!replay.IsCurrent(initialGeneration)) {
        std::cerr << "[TEST] Initial replay generation did not become current.\n";
        return false;
    }

    replay.BeginInitCycle(); // suspend/resume reinitialization
    const uint64_t resumeGeneration = replay.generation;
    if (resumeGeneration == initialGeneration ||
        replay.IsCurrent(initialGeneration) || replay.IsCurrent(resumeGeneration)) {
        std::cerr << "[TEST] Resume init cycle did not invalidate pre-reset commands.\n";
        return false;
    }
    replay.CompleteInitCycle();
    if (!replay.IsCurrent(resumeGeneration)) {
        std::cerr << "[TEST] Resume replay generation did not become current.\n";
        return false;
    }

    replay.BeginInitCycle(); // recover reinitialization
    const uint64_t recoverGeneration = replay.generation;
    if (recoverGeneration == resumeGeneration || replay.IsCurrent(resumeGeneration)) {
        std::cerr << "[TEST] Recover init cycle did not invalidate pre-reset commands.\n";
        return false;
    }
    replay.CompleteInitCycle();
    if (!replay.IsCurrent(recoverGeneration)) {
        std::cerr << "[TEST] Recover replay generation did not become current.\n";
        return false;
    }
    return true;
}

bool RunAfeStylusSnapshotSynchronizationTest() {
    auto chip = std::make_unique<Himax::Chip>(
        L"\\\\?\\EGoTouchMissingMaster",
        L"\\\\?\\EGoTouchMissingSlave",
        L"\\\\?\\EGoTouchMissingInterrupt");
    chip->m_connState.store(Himax::ConnectionState::Connected, std::memory_order_release);

    std::barrier startBarrier(3);
    std::atomic<bool> stateConsistent{true};
    std::thread writer([&]() {
        startBarrier.arrive_and_wait();
        for (int i = 0; i < 128; ++i) {
            (void)chip->SendAfeCommand(command{AFE_Command::InitStylus, 5});
            (void)chip->SendAfeCommand(command{AFE_Command::DisconnectStylus, 0});
        }
    });
    std::thread reader([&]() {
        startBarrier.arrive_and_wait();
        for (int i = 0; i < 1024; ++i) {
            const auto snapshot = chip->m_afe.GetStylusStateSnapshot();
            if ((snapshot.connected && snapshot.pen_id != 5) ||
                (!snapshot.connected && snapshot.pen_id != 0)) {
                stateConsistent.store(false, std::memory_order_release);
                break;
            }
        }
    });

    startBarrier.arrive_and_wait();
    writer.join();
    reader.join();

    (void)chip->SendAfeCommand(command{AFE_Command::InitStylus, 11});
    const auto finalSnapshot = chip->m_afe.GetStylusStateSnapshot();
    chip->m_connState.store(Himax::ConnectionState::Unconnected, std::memory_order_release);
    if (!stateConsistent.load(std::memory_order_acquire) ||
        !finalSnapshot.connected || finalSnapshot.pen_id != 11 ||
        !chip->IsStylusConnected()) {
        std::cerr << "[TEST] AFE stylus snapshot was not synchronized.\n";
        return false;
    }
    return true;
}

bool RunConcurrentStartAndSubmitLinearizationTest() {
    using namespace std::chrono_literals;
    constexpr int kIterations = 12;
    constexpr int kSubmitters = 12;
    std::size_t totalAccepted = 0;

    auto runtime = std::make_unique<DeviceRuntime>(
        L"\\\\?\\EGoTouchMissingMaster",
        L"\\\\?\\EGoTouchMissingSlave",
        L"\\\\?\\EGoTouchMissingInterrupt");
    runtime->SetAutoMode(false);

    for (int iteration = 0; iteration < kIterations; ++iteration) {
        runtime->Stop();
        runtime->ClearHistory();

        std::barrier startBarrier(kSubmitters + 2);
        std::atomic<int> accepted{0};
        std::atomic<bool> startOk{false};
        std::vector<std::thread> submitters;
        submitters.reserve(kSubmitters);
        for (int i = 0; i < kSubmitters; ++i) {
            submitters.emplace_back([&]() {
                startBarrier.arrive_and_wait();
                for (int attempt = 0; attempt < 2; ++attempt) {
                    if (runtime->SubmitExternalAfeCommand(AFE_Command::ClearStatus, 0)) {
                        accepted.fetch_add(1, std::memory_order_relaxed);
                    }
                    std::this_thread::yield();
                }
            });
        }
        std::thread starter([&]() {
            startBarrier.arrive_and_wait();
            startOk.store(runtime->Start(), std::memory_order_release);
        });

        startBarrier.arrive_and_wait();
        for (auto& submitter : submitters) {
            submitter.join();
        }
        starter.join();
        if (!startOk.load(std::memory_order_acquire)) {
            std::cerr << "[TEST] Concurrent runtime Start failed.\n";
            return false;
        }

        const int acceptedCount = accepted.load(std::memory_order_acquire);
        totalAccepted += static_cast<std::size_t>(acceptedCount);
        const auto deadline = std::chrono::steady_clock::now() + 2s;
        while (std::chrono::steady_clock::now() < deadline &&
               runtime->GetHistory(100).size() < static_cast<std::size_t>(acceptedCount)) {
            std::this_thread::sleep_for(5ms);
        }
        const auto executedCount = runtime->GetHistory(100).size();
        runtime->Stop();

        if (executedCount != static_cast<std::size_t>(acceptedCount)) {
            std::cerr << "[TEST] Start/submit boundary lost an accepted command: accepted="
                      << acceptedCount << " executed=" << executedCount << "\n";
            return false;
        }
    }

    if (totalAccepted == 0) {
        std::cerr << "[TEST] Start/submit concurrency test did not exercise accepted commands.\n";
        return false;
    }
    return true;
}

bool RunStoppedPenIngressDoesNotQueueAfeCommandsTest() {
    auto runtime = std::make_unique<DeviceRuntime>(
        L"\\\\?\\EGoTouchMissingMaster",
        L"\\\\?\\EGoTouchMissingSlave",
        L"\\\\?\\EGoTouchMissingInterrupt");
    runtime->Stop();

    if (runtime->SubmitExternalAfeCommand(AFE_Command::ClearStatus, 0)) {
        std::cerr << "[TEST] Stopped runtime accepted an external AFE command.\n";
        return false;
    }

    command staleCommand{AFE_Command::ClearStatus, 0};
    runtime->SubmitCommand(staleCommand, CommandSource::SystemPolicy, "stale-before-restart");
    if (runtime->GetSnapshot().queue_depth != 1) {
        std::cerr << "[TEST] Failed to seed stale command queue for restart test.\n";
        return false;
    }

    Himax::Pen::PenEvent connection{};
    connection.code = Himax::Pen::PenUsbEventCode::PenConnStatus;
    connection.semantic.hasConnection = true;
    connection.semantic.connected = true;
    runtime->IngestPenEvent(connection);

    Himax::Pen::PenEvent penType{};
    penType.code = Himax::Pen::PenUsbEventCode::PenTypeInfo;
    penType.semantic.hasStylusId = true;
    penType.semantic.stylusId = 9;
    runtime->IngestPenEvent(penType);

    const auto stoppedSnapshot = runtime->GetSnapshot();
    const auto penState = runtime->GetPenStateSnapshot();
    if (stoppedSnapshot.queue_depth != 1 ||
        !penState.hasConnection || !penState.connected ||
        !penState.hasStylusId || penState.stylusId != 9) {
        std::cerr << "[TEST] Stopped pen ingress did not isolate AFE work from state updates.\n";
        return false;
    }

    runtime->SetAutoMode(false);
    if (!runtime->Start()) {
        std::cerr << "[TEST] Runtime failed to restart while testing stale queue cleanup.\n";
        return false;
    }
    const auto restartedSnapshot = runtime->GetSnapshot();
    runtime->Stop();

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

    if (!RunSuspendResumeSchedulesFreshPenReplayTest()) {
        return 2;
    }

    if (!RunResumeWithoutSystemSuspendKeepsRecoverStateTest()) {
        return 3;
    }

    if (!RunRestartAfterWorkerSelfExitTest()) {
        return 4;
    }

    if (!RunPenAfeRestartPlanOrderingTest()) {
        return 5;
    }

    if (!RunPenReplayCoversResumeAndRecoverInitCyclesTest()) {
        return 6;
    }

    if (!RunAfeStylusSnapshotSynchronizationTest()) {
        return 7;
    }

    if (!RunConcurrentStartAndSubmitLinearizationTest()) {
        return 8;
    }

    if (!RunStoppedPenIngressDoesNotQueueAfeCommandsTest()) {
        return 9;
    }

    std::cout << "[TEST] DeviceRuntime system power policy tests passed.\n";
    return 0;
}
