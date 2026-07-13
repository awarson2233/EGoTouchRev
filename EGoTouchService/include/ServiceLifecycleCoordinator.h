#pragma once

namespace Service {

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
        if (stages.pen) {
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
