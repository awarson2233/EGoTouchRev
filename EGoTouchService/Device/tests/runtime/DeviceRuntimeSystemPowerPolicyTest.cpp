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

bool WaitForCommandGate(DeviceRuntime& runtime,
                        bool expected,
                        std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (runtime.IsAcceptingExternalAfeCommands() == expected) {
            return true;
        }
        std::this_thread::yield();
    }
    return runtime.IsAcceptingExternalAfeCommands() == expected;
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
    const auto restartResult = runtime.RequestStart();
    if (restartResult != DeviceRuntime::StartRequestResult::Started) {
        runtime.Stop();
        std::cerr << "[TEST] RequestStart did not restart across worker self-stop; result="
                  << static_cast<int>(restartResult) << "\n";
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

bool RunRequestStartLinearizesWithExplicitStopTest() {
    using namespace std::chrono_literals;

    auto runtime = std::make_unique<DeviceRuntime>(
        L"\\\\?\\EGoTouchMissingMaster",
        L"\\\\?\\EGoTouchMissingSlave",
        L"\\\\?\\EGoTouchMissingInterrupt");
    runtime->SetAutoMode(true);
    if (!runtime->Start() || !WaitForState(*runtime, workerState::recover, 2s)) {
        runtime->Stop();
        std::cerr << "[TEST] Failed to prepare runtime for concurrent Stop/RequestStart.\n";
        return false;
    }

    std::barrier startBarrier(2);
    std::thread stopper([&]() {
        startBarrier.arrive_and_wait();
        (void)runtime->RequestStop();
    });
    startBarrier.arrive_and_wait();

    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (std::chrono::steady_clock::now() < deadline &&
           runtime->GetSnapshot().state != workerState::quit &&
           runtime->IsRunning()) {
        std::this_thread::yield();
    }

    const auto startResult = runtime->RequestStart();
    stopper.join();
    const bool restarted =
        startResult == DeviceRuntime::StartRequestResult::Started &&
        WaitForState(*runtime, workerState::recover, 2s);
    runtime->Stop();
    if (!restarted) {
        std::cerr << "[TEST] RequestStart did not linearize after concurrent Stop; result="
                  << static_cast<int>(startResult) << "\n";
        return false;
    }
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

bool RunRepeatedConnectedEventPreservesReplayedStylusIdTest() {
    auto chip = std::make_unique<Himax::Chip>(
        L"\\\\?\\EGoTouchMissingMaster",
        L"\\\\?\\EGoTouchMissingSlave",
        L"\\\\?\\EGoTouchMissingInterrupt");
    chip->m_connState.store(Himax::ConnectionState::Connected, std::memory_order_release);

    (void)chip->SendAfeCommand(command{AFE_Command::InitStylus, 0});
    (void)chip->SendAfeCommand(command{AFE_Command::SetStylusId, 7});
    if (const auto duplicateConnected = BuildPenConnectionAfeCommand(false, true)) {
        (void)chip->SendAfeCommand(*duplicateConnected);
    }

    const auto snapshot = chip->m_afe.GetStylusStateSnapshot();
    chip->m_connState.store(Himax::ConnectionState::Unconnected, std::memory_order_release);
    if (!snapshot.connected || snapshot.pen_id != 7) {
        std::cerr << "[TEST] Duplicate connected event reset replayed stylus ID.\n";
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

bool RunAcceptedCommandsObservedAcrossShutdownAndStopTest() {
    using namespace std::chrono_literals;
    constexpr int kSubmitters = 8;

    for (const bool policyShutdown : {false, true}) {
        auto runtime = std::make_unique<DeviceRuntime>(
            L"\\\\?\\EGoTouchMissingMaster",
            L"\\\\?\\EGoTouchMissingSlave",
            L"\\\\?\\EGoTouchMissingInterrupt");
        runtime->SetAutoMode(false);
        if (!runtime->Start()) {
            std::cerr << "[TEST] Failed to start command cancellation runtime.\n";
            return false;
        }
        runtime->ClearHistory();

        std::atomic<int> accepted{
            runtime->SubmitExternalAfeCommand(AFE_Command::ClearStatus, 0) ? 1 : 0};
        std::barrier stopBarrier(kSubmitters + 2);
        std::vector<std::thread> submitters;
        submitters.reserve(kSubmitters);
        for (int i = 0; i < kSubmitters; ++i) {
            submitters.emplace_back([&]() {
                stopBarrier.arrive_and_wait();
                if (runtime->SubmitExternalAfeCommand(AFE_Command::ClearStatus, 0)) {
                    accepted.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }
        std::thread stopper([&]() {
            stopBarrier.arrive_and_wait();
            if (policyShutdown) {
                runtime->IngestPolicyEvent(
                    MakePolicyEvent(RuntimePolicyEvent::Type::Shutdown));
            } else {
                (void)runtime->RequestStop();
            }
        });

        stopBarrier.arrive_and_wait();
        for (auto& submitter : submitters) {
            submitter.join();
        }
        stopper.join();

        if (policyShutdown) {
            const auto deadline = std::chrono::steady_clock::now() + 2s;
            while (std::chrono::steady_clock::now() < deadline && runtime->IsRunning()) {
                std::this_thread::sleep_for(5ms);
            }
            runtime->Stop();
        }

        const auto history = runtime->GetHistory(100);
        const auto acceptedCount = static_cast<std::size_t>(
            accepted.load(std::memory_order_acquire));
        const bool gateClosed =
            !runtime->SubmitExternalAfeCommand(AFE_Command::ClearStatus, 0);
        if (!gateClosed || history.size() != acceptedCount) {
            std::cerr << "[TEST] Accepted command was silently lost across "
                      << (policyShutdown ? "Shutdown" : "Stop")
                      << ": accepted=" << acceptedCount
                      << " observed=" << history.size() << "\n";
            return false;
        }
    }
    return true;
}

bool RunPublishedShutdownIsNotLostByStartTest() {
    using namespace std::chrono_literals;
    constexpr int kIterations = 12;

    auto runtime = std::make_unique<DeviceRuntime>(
        L"\\\\?\\EGoTouchMissingMaster",
        L"\\\\?\\EGoTouchMissingSlave",
        L"\\\\?\\EGoTouchMissingInterrupt");
    runtime->SetAutoMode(false);

    for (int iteration = 0; iteration < kIterations; ++iteration) {
        runtime->Stop();
        std::barrier startBarrier(3);
        std::atomic<bool> startDone{false};
        std::atomic<bool> startOk{false};

        std::thread starter([&]() {
            startBarrier.arrive_and_wait();
            startOk.store(runtime->Start(), std::memory_order_release);
            startDone.store(true, std::memory_order_release);
        });
        std::thread shutdownPublisher([&]() {
            startBarrier.arrive_and_wait();
            while (!runtime->IsRunning() &&
                   !startDone.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            if (runtime->IsRunning()) {
                runtime->IngestPolicyEvent(
                    MakePolicyEvent(RuntimePolicyEvent::Type::Shutdown));
            }
        });

        startBarrier.arrive_and_wait();
        starter.join();
        shutdownPublisher.join();
        if (!startOk.load(std::memory_order_acquire)) {
            std::cerr << "[TEST] Concurrent Start failed before Shutdown publication.\n";
            return false;
        }

        const auto deadline = std::chrono::steady_clock::now() + 2s;
        while (std::chrono::steady_clock::now() < deadline && runtime->IsRunning()) {
            std::this_thread::sleep_for(5ms);
        }
        const bool shutdownObserved = !runtime->IsRunning() &&
            runtime->GetSnapshot().state == workerState::quit;
        runtime->Stop();
        if (!shutdownObserved) {
            std::cerr << "[TEST] Shutdown published after running was lost during Start.\n";
            return false;
        }
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

bool RunWorkerSelfStopRejectsWorkerRestartTest() {
    auto runtime = std::make_unique<DeviceRuntime>(
        L"\\\\?\\EGoTouchMissingMaster",
        L"\\\\?\\EGoTouchMissingSlave",
        L"\\\\?\\EGoTouchMissingInterrupt");
    runtime->SetAutoMode(false);

    std::barrier hookCompleted(2);
    std::atomic<int> startResult{-1};
    runtime->SetWorkerHookForTesting([&]() {
        runtime->Stop();
        startResult.store(
            static_cast<int>(runtime->RequestStart()),
            std::memory_order_release);
        hookCompleted.arrive_and_wait();
    });

    if (!runtime->Start()) {
        std::cerr << "[TEST] Failed to start runtime for worker self-stop test.\n";
        return false;
    }
    hookCompleted.arrive_and_wait();
    const auto result = static_cast<DeviceRuntime::StartRequestResult>(
        startResult.load(std::memory_order_acquire));
    runtime->Stop();

    if (result != DeviceRuntime::StartRequestResult::Failed) {
        std::cerr << "[TEST] Worker RequestStart was not rejected after self-stop.\n";
        return false;
    }
    return true;
}

bool RunExternalStopDoesNotDeadlockWorkerRestartTest() {
    using namespace std::chrono_literals;

    auto runtime = std::make_unique<DeviceRuntime>(
        L"\\\\?\\EGoTouchMissingMaster",
        L"\\\\?\\EGoTouchMissingSlave",
        L"\\\\?\\EGoTouchMissingInterrupt");
    runtime->SetAutoMode(false);

    std::barrier hookEntered(2);
    std::barrier releaseHook(2);
    std::atomic<int> startResult{-1};
    runtime->SetWorkerHookForTesting([&]() {
        hookEntered.arrive_and_wait();
        releaseHook.arrive_and_wait();
        startResult.store(
            static_cast<int>(runtime->RequestStart()),
            std::memory_order_release);
    });

    if (!runtime->Start()) {
        std::cerr << "[TEST] Failed to start runtime for external stop test.\n";
        return false;
    }
    hookEntered.arrive_and_wait();

    std::thread stopper([&]() { (void)runtime->RequestStop(); });
    const bool gateClosed = WaitForCommandGate(*runtime, false, 2s);
    releaseHook.arrive_and_wait();
    stopper.join();

    const auto result = static_cast<DeviceRuntime::StartRequestResult>(
        startResult.load(std::memory_order_acquire));
    if (!gateClosed || result != DeviceRuntime::StartRequestResult::Failed ||
        runtime->IsRunning()) {
        std::cerr << "[TEST] Worker RequestStart did not fail cleanly during external Stop.\n";
        return false;
    }
    return true;
}

bool RunExplicitStopRemainsTerminalAfterSleepEventsTest() {
    using namespace std::chrono_literals;

    auto runtime = std::make_unique<DeviceRuntime>(
        L"\\\\?\\EGoTouchMissingMaster",
        L"\\\\?\\EGoTouchMissingSlave",
        L"\\\\?\\EGoTouchMissingInterrupt");
    runtime->SetAutoMode(false);

    std::barrier hookEntered(2);
    std::barrier releaseHook(2);
    runtime->SetWorkerHookForTesting([&]() {
        hookEntered.arrive_and_wait();
        releaseHook.arrive_and_wait();
    });

    if (!runtime->Start()) {
        std::cerr << "[TEST] Failed to start runtime for terminal Stop test.\n";
        return false;
    }
    hookEntered.arrive_and_wait();

    std::thread stopper([&]() { runtime->Stop(); });
    const bool gateClosed = WaitForCommandGate(*runtime, false, 2s);
    runtime->IngestPolicyEvent(MakePolicyEvent(RuntimePolicyEvent::Type::Suspend));
    runtime->IngestPolicyEvent(MakePolicyEvent(RuntimePolicyEvent::Type::LidOff));
    releaseHook.arrive_and_wait();
    stopper.join();

    if (!gateClosed || runtime->IsRunning() ||
        runtime->GetSnapshot().state != workerState::quit) {
        std::cerr << "[TEST] Sleep event downgraded an explicit Stop request.\n";
        return false;
    }
    return true;
}

bool RunPolicyShutdownRemainsTerminalAfterSleepEventsTest() {
    using namespace std::chrono_literals;

    auto runtime = std::make_unique<DeviceRuntime>(
        L"\\\\?\\EGoTouchMissingMaster",
        L"\\\\?\\EGoTouchMissingSlave",
        L"\\\\?\\EGoTouchMissingInterrupt");
    runtime->SetAutoMode(false);

    std::barrier hookEntered(2);
    std::barrier releaseHook(2);
    runtime->SetWorkerHookForTesting([&]() {
        hookEntered.arrive_and_wait();
        releaseHook.arrive_and_wait();
    });

    if (!runtime->Start()) {
        std::cerr << "[TEST] Failed to start runtime for terminal Shutdown test.\n";
        return false;
    }
    hookEntered.arrive_and_wait();

    runtime->IngestPolicyEvent(MakePolicyEvent(RuntimePolicyEvent::Type::Shutdown));
    const bool gateClosed = WaitForCommandGate(*runtime, false, 2s);
    runtime->IngestPolicyEvent(MakePolicyEvent(RuntimePolicyEvent::Type::Suspend));
    runtime->IngestPolicyEvent(MakePolicyEvent(RuntimePolicyEvent::Type::LidOff));
    releaseHook.arrive_and_wait();

    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (std::chrono::steady_clock::now() < deadline && runtime->IsRunning()) {
        std::this_thread::yield();
    }
    const bool terminated = !runtime->IsRunning() &&
        runtime->GetSnapshot().state == workerState::quit;
    runtime->Stop();

    if (!gateClosed || !terminated) {
        std::cerr << "[TEST] Sleep event downgraded a policy Shutdown request.\n";
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

    if (!RunRequestStartLinearizesWithExplicitStopTest()) {
        return 5;
    }

    if (!RunPenAfeRestartPlanOrderingTest()) {
        return 6;
    }

    if (!RunPenReplayCoversResumeAndRecoverInitCyclesTest()) {
        return 7;
    }

    if (!RunAfeStylusSnapshotSynchronizationTest()) {
        return 8;
    }

    if (!RunRepeatedConnectedEventPreservesReplayedStylusIdTest()) {
        return 9;
    }

    if (!RunConcurrentStartAndSubmitLinearizationTest()) {
        return 10;
    }

    if (!RunAcceptedCommandsObservedAcrossShutdownAndStopTest()) {
        return 11;
    }

    if (!RunPublishedShutdownIsNotLostByStartTest()) {
        return 12;
    }

    if (!RunStoppedPenIngressDoesNotQueueAfeCommandsTest()) {
        return 13;
    }

    if (!RunWorkerSelfStopRejectsWorkerRestartTest()) {
        return 14;
    }

    if (!RunExternalStopDoesNotDeadlockWorkerRestartTest()) {
        return 15;
    }

    if (!RunExplicitStopRemainsTerminalAfterSleepEventsTest()) {
        return 16;
    }

    if (!RunPolicyShutdownRemainsTerminalAfterSleepEventsTest()) {
        return 17;
    }

    std::cout << "[TEST] DeviceRuntime system power policy tests passed.\n";
    return 0;
}
