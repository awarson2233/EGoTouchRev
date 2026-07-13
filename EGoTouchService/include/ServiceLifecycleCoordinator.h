#pragma once

namespace Service {

// Defines the only supported subsystem publication and teardown order.
// HostLike supplies the concrete lifecycle operations; production ServiceHost
// and the fake integration harness both execute this exact sequence.
class ServiceLifecycleCoordinator {
public:
    template <typename HostLike>
    static bool Start(HostLike& host) {
        if (!host.StartRuntimeAndPipeline()) {
            return false;
        }
        host.StartIpcSubsystem();
        host.StartPenSubsystem();
        host.StartSystemStateMonitor();
        return true;
    }

    template <typename HostLike>
    static void Stop(HostLike& host) {
        host.StopSystemStateMonitor();
        host.StopIpcServer();
        host.StopPenSubsystem();
        host.CloseIpcResources();
        host.StopRuntimeSubsystem();
    }
};

} // namespace Service
