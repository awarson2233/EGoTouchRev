#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class DeviceRuntime;

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

/// 服务运行模式
enum class ServiceMode {
    Full,       ///< 触摸 + 手写笔（PenEventBridge + PenPressureReader + StylusPipeline 全部启用）
    TouchOnly,  ///< 仅触摸（跳过手写笔模块 / StylusPipeline）
};

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

    struct ServiceConfigState {
        ServiceMode mode = ServiceMode::Full;
        bool autoMode = true;
        bool stylusVhfEnabled = true;
    };

    struct ReloadServiceConfigResult {
        uint8_t changedFields = 0;
        uint8_t appliedFields = 0;
        uint8_t restartRequiredFields = 0;
    };

    ServiceConfigState m_configState{};
    ServiceMode m_runtimeMode = ServiceMode::Full;

    std::unique_ptr<DeviceRuntime> m_deviceRuntime;
    std::unique_ptr<Impl> m_impl;

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

    ServiceConfigState ParseServiceConfig(const std::string& configPath) const;
    void ApplyServiceConfigToRuntime(const ServiceConfigState& config);
    ReloadServiceConfigResult HandleReloadServiceConfig(const ServiceConfigState& reloadedConfig);
    void BuildDefaultPipeline(const std::string& configPath);

    bool StartRuntimeAndPipeline(const std::string& configPath);
    void StartSystemStateMonitor();
    void StartIpcSubsystem();
    void StartPenSubsystem();

    void StopIpcSubsystem();
    void StopPenSubsystem();
    void StopSystemStateMonitor();
    void StopRuntimeSubsystem();

    void HandleIpcEnterDebugMode(Ipc::IpcResponse& resp);
    void HandleIpcExitDebugMode(Ipc::IpcResponse& resp);
    void HandleIpcReloadConfig(Ipc::IpcResponse& resp);
    void HandleIpcSaveConfig(Ipc::IpcResponse& resp);
    void HandleIpcGetLogs(Ipc::IpcResponse& resp);
    void HandleIpcGetPenBridgeStatus(Ipc::IpcResponse& resp);
    void HandleIpcGetDebugSchema(const Ipc::IpcRequest& req, Ipc::IpcResponse& resp);
    void HandleIpcGetDebugSnapshot(Ipc::IpcResponse& resp);

    Ipc::IpcResponse HandleIpcCommand(const Ipc::IpcRequest& req);
};

} // namespace Service

