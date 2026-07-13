#pragma once

#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>

namespace Service {

// Linearizes public Start/Stop calls without holding the lifecycle mutex while
// subsystem operations or callbacks run. Stop waits for an in-flight Start to
// publish all stages, then performs one complete coordinator teardown.
class ServiceLifecycleStateMachine {
public:
    template <typename StartOperation>
    bool RunStart(StartOperation&& operation) {
        std::shared_ptr<StartAttempt> attempt;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            while (m_state == State::Stopping) {
                m_stateChanged.wait(lock);
            }
            if (m_state == State::Running) {
                return true;
            }
            if (m_state == State::Starting || m_state == State::StartingStopPending) {
                attempt = m_startAttempt;
                ++m_startWaiters;
                attempt->completed.wait(lock, [&] { return attempt->done; });
                --m_startWaiters;
                return attempt->succeeded;
            }

            attempt = std::make_shared<StartAttempt>();
            m_startAttempt = attempt;
            m_state = State::Starting;
        }

        bool started = false;
        try {
            started = operation();
        } catch (...) {
            CompleteStart(attempt, false);
            return false;
        }
        CompleteStart(attempt, started);
        return started;
    }

    template <typename StopOperation>
    void RunStop(StopOperation&& operation) noexcept {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            while (m_state == State::Starting || m_state == State::StartingStopPending) {
                m_state = State::StartingStopPending;
                const auto attempt = m_startAttempt;
                attempt->completed.wait(lock, [&] { return attempt->done; });
            }
            if (m_state == State::Stopping) {
                m_stateChanged.wait(lock, [&] { return m_state == State::Stopped; });
                return;
            }
            if (m_state == State::Stopped) {
                return;
            }
            m_state = State::Stopping;
        }

        try {
            operation();
        } catch (...) {
            // The public lifecycle boundary remains stopped even if a host-like
            // test double throws. Production rollback already isolates stages.
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_state = State::Stopped;
        }
        m_stateChanged.notify_all();
    }

    bool HasPendingStop() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_state == State::StartingStopPending;
    }

    bool HasWaitingStart() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_startWaiters != 0;
    }

private:
    enum class State {
        Stopped,
        Starting,
        StartingStopPending,
        Running,
        Stopping,
    };

    struct StartAttempt {
        std::condition_variable completed;
        bool done = false;
        bool succeeded = false;
    };

    void CompleteStart(const std::shared_ptr<StartAttempt>& attempt, bool succeeded) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            attempt->succeeded = succeeded;
            attempt->done = true;
            m_state = succeeded ? State::Running : State::Stopped;
        }
        attempt->completed.notify_all();
        m_stateChanged.notify_all();
    }

    mutable std::mutex m_mutex;
    std::condition_variable m_stateChanged;
    std::shared_ptr<StartAttempt> m_startAttempt;
    std::size_t m_startWaiters = 0;
    State m_state = State::Stopped;
};

// Defines the only supported subsystem publication and teardown order.
// HostLike supplies idempotent lifecycle operations that return false when a
// startup stage cannot become ready.
class ServiceLifecycleCoordinator {
public:
    template <typename HostLike>
    static bool Start(HostLike& host) noexcept {
        Stages stages{};
        try {
            stages.runtime = true;
            if (!host.StartRuntimeAndPipeline()) {
                Rollback(host, stages);
                return false;
            }

            stages.ipc = true;
            if (!host.StartIpcSubsystem()) {
                Rollback(host, stages);
                return false;
            }

            stages.pen = true;
            if (!host.StartPenSubsystem()) {
                Rollback(host, stages);
                return false;
            }

            stages.monitor = true;
            if (!host.StartSystemStateMonitor()) {
                Rollback(host, stages);
                return false;
            }
            return true;
        } catch (...) {
            Rollback(host, stages);
            return false;
        }
    }

    template <typename HostLike>
    static void Stop(HostLike& host) noexcept {
        Rollback(host, Stages{true, true, true, true});
    }

private:
    struct Stages {
        bool runtime = false;
        bool ipc = false;
        bool pen = false;
        bool monitor = false;
    };

    template <typename Operation>
    static void InvokeNoexcept(Operation&& operation) noexcept {
        try {
            operation();
        } catch (...) {
            // Continue rollback so later resources are never stranded.
        }
    }

    template <typename HostLike>
    static void Rollback(HostLike& host, const Stages& stages) noexcept {
        if (stages.monitor) {
            InvokeNoexcept([&] { host.StopSystemStateMonitor(); });
        }
        if (stages.ipc) {
            // Gate and join handlers before any Pen object can be destroyed.
            InvokeNoexcept([&] { host.StopIpcServer(); });
        }
        if (stages.pen || stages.ipc) {
            // IPC setup owns the Pen notification handle before Pen objects are
            // published. Always pass through the idempotent Pen teardown so an
            // IPC readiness failure cannot strand that handle.
            InvokeNoexcept([&] { host.StopPenSubsystem(); });
        }
        if (stages.ipc) {
            InvokeNoexcept([&] { host.CloseIpcResources(); });
        }
        if (stages.runtime) {
            InvokeNoexcept([&] { host.StopRuntimeSubsystem(); });
        }
    }
};

} // namespace Service
