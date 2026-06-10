#pragma once

#include "ConfigRuntime.h"
#include "ServiceConfigCore.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class DeviceRuntime;

namespace Config {
class ConfigStore;
}

namespace Himax::Pen {
struct PenPressureStats;
}

namespace Ipc {
struct DebugFieldSchemaWire;
struct IpcRequest;
struct IpcResponse;
}

namespace Solvers {
struct HeatmapFrame;
}

namespace Service {

/// 模块加载器：负责创建、连接、启停所有子模块。
/// 不知道 SCM 的存在，可以独立测试。
class ServiceHost {
public:
    ServiceHost();
    ~ServiceHost();

    ServiceHost(const ServiceHost&) = delete;
    ServiceHost& operator=(const ServiceHost&) = delete;

    /// 按依赖顺序启动所有模块
    bool Start();

    /// 逆序停止所有模块
    void Stop();

    ServiceMode GetMode() const { return m_runtimeMode; }

private:
    struct Impl;

    ServiceConfigState m_configState{};
    ServiceMode m_runtimeMode = ServiceMode::Full;
    ConfigRuntime m_configRuntime;

    std::unique_ptr<DeviceRuntime> m_deviceRuntime;
    std::unique_ptr<Impl> m_impl;

#if EGOTOUCH_SERVICE_ENABLE_IPC
    static void CopyCString(char* dst, size_t dstSize, std::string_view src);
    static uint32_t HashDebugSchema(const std::vector<Ipc::DebugFieldSchemaWire>& defs);
    static uint16_t DeriveDebugSchemaVersion(uint32_t schemaHash);
    static uint64_t EncodeDebugValue(const Solvers::HeatmapFrame& frame,
                                     const Ipc::DebugFieldSchemaWire& def,
                                     bool& valid);
    static uint64_t EncodePenValue(const Himax::Pen::PenPressureStats& s,
                                   bool evtRunning,
                                   bool pressRunning,
                                   int16_t sourceIndex,
                                   bool& valid);
    void BuildDebugSchema();
#endif

    bool InitializeConfigStores(const std::string& configPath);
    void ApplyServiceConfigToRuntime(const ServiceConfigState& config);
    ReloadServiceConfigResult HandleReloadServiceConfig(const ServiceConfigState& reloadedConfig);
    bool ValidateStartupConfig(const Config::ConfigStore& store) const;
    bool StartRuntimeAndPipeline();
    void StartSystemStateMonitor();
#if EGOTOUCH_SERVICE_ENABLE_IPC
    void StartIpcSubsystem();
#endif
    void StartPenSubsystem();

#if EGOTOUCH_SERVICE_ENABLE_IPC
    void StopIpcSubsystem();
#endif
    void StopPenSubsystem();
    void StopSystemStateMonitor();
    void StopRuntimeSubsystem();

#if EGOTOUCH_SERVICE_ENABLE_IPC
    void HandleIpcEnterDebugMode(Ipc::IpcResponse& resp);
    void HandleIpcExitDebugMode(Ipc::IpcResponse& resp);
    void HandleIpcGetConfigCatalogV3(const Ipc::IpcRequest& req, Ipc::IpcResponse& resp);
    void HandleIpcGetConfigV3Snapshot(const Ipc::IpcRequest& req, Ipc::IpcResponse& resp);
    void HandleIpcConfigV3ApplyPatch(const Ipc::IpcRequest& req, Ipc::IpcResponse& resp);
    void HandleIpcConfigV3Persist(Ipc::IpcResponse& resp);
    void HandleIpcGetLogs(Ipc::IpcResponse& resp);
    void HandleIpcGetPenBridgeStatus(Ipc::IpcResponse& resp);
    void HandleIpcGetPenIdentityStatus(Ipc::IpcResponse& resp);
    void HandleIpcGetDebugSchema(const Ipc::IpcRequest& req, Ipc::IpcResponse& resp);
    void HandleIpcGetDebugSnapshot(Ipc::IpcResponse& resp);

    Ipc::IpcResponse HandleIpcCommand(const Ipc::IpcRequest& req);
#endif
};

} // namespace Service

