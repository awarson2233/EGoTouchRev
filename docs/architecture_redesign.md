# EGoTouch 驱动服务 — 架构文档 (v3)

> **最后更新**: 2026-04-01  
> **状态**: 文档已与当前代码实现对齐（非规划稿）

---

## 一、架构演进概述

### 1.1 v3 更新目标（已完成 ✅）

本次 v3 不是再次重构代码，而是对架构文档进行“**代码对齐修订**”，解决 v2 中的若干描述漂移问题。

| # | v2 文档问题 | v3 修正结果 |
|---|-----------|------------|
| D1 | Shared Memory 名称写为 `Local\EGoTouchSharedFrame` | ✅ 已修正为 `Global\EGoTouchSharedFrame` |
| D2 | Touch Pipeline 仍写 9 段且包含 `StylusProcessor` | ✅ 已修正为当前 Service 实际注册的 8 段 |
| D3 | “第二阶段规划”口径偏重未来态 | ✅ 改为“当前实现 + 风险清单 + 优先级” |
| D4 | 配置链路描述不完整 | ✅ 明确记录 App 本地写配置 + Service Reload 的现状 |
| D5 | 调试帧通道在 Release 可用性未说明 | ✅ 明确 `_DEBUG` 分支行为与风险 |

### 1.2 当前实际层次结构

```text
EGoTouchService.exe                     ← 生产服务进程
  ServiceEntry.cpp                      — wmain: --install/--uninstall/--console
  ├── ServiceShell                      — SCM 控制码与电源事件桥接
  │     └── ServiceHost                 — 模块加载器
  │           ├── DeviceRuntime         — 状态机 + 帧采集 Worker + 命令队列
  │           │     ├── Himax::Chip     — I2C/USB 硬件独占
  │           │     ├── FramePipeline   — Touch 算法管线 (8 stages)
  │           │     ├── StylusPipeline  — Stylus 独立管线
  │           │     └── VhfReporter     — HID VHF 注入
  │           ├── SystemStateMonitor    — 亮灭屏/Lid/Resume/Shutdown 事件
  │           ├── PenEventBridge        — BT MCU 事件通道 (col00)
  │           ├── PenPressureReader     — BT MCU 压力通道 (col01)
  │           ├── IpcPipeServer         — Named Pipe IPC 命令通道
  │           └── SharedFrameWriter     — Shared Memory 帧推送（Debug）

EGoTouchApp.exe                         ← 调试诊断上位机 (Tools/)
  ApplicationEntry.cpp                  — ImGui DX11 主循环
  ├── ServiceProxy                      — Pipe + SharedMem 客户端代理
  │     ├── IpcPipeClient               — 命令通道客户端
  │     ├── SharedFrameReader           — 帧数据读取
  │     ├── FramePipeline (local copy)  — GUI 参数镜像
  │     └── DVR RingBuffer              — 本地回放缓存
  └── DiagnosticsWorkbench              — 调试 UI 面板

BtMcuTestTool.exe                       ← BT MCU 协议验证工具
```

### 1.3 当前设计亮点

**核心已落地能力：**

1. `ServiceShell + ServiceHost + DeviceRuntime` 分层明确，服务主链稳定。
2. 触摸和手写笔处理链独立，便于分开调试与禁用。
3. App 通过 `ServiceProxy` 代理通信，不再直接访问硬件。
4. 系统电源事件已闭环接入到 Runtime 状态机。
5. 支持 `full / touch_only` 运行模式切换。

---

## 二、当前模块详解

### 2.1 服务端三层架构

```mermaid
flowchart TD
    subgraph SHELL["ServiceShell（最外层壳）"]
        SCM["SCM 注册 / 控制台回退<br/>SvcMain() / RunAsConsole()"]
    end

    SCM -->|"Start() / Stop()"| HOST

    subgraph HOST["ServiceHost（模块加载器）"]
        SSM["SystemStateMonitor<br/>NamedEvent 监听"]
        IPC["IpcPipeServer<br/>Named Pipe 命令"]
        PE["PenEventBridge<br/>BT MCU 事件通道"]
        PP["PenPressureReader<br/>BT MCU 压力通道"]
        SFW["SharedFrameWriter<br/>Debug 帧推送"]

        subgraph DR["DeviceRuntime（设备维护 + 处理）"]
            CQ["命令队列 + Worker"]
            CHIP["Himax::Chip"]
            TP["FramePipeline<br/>Touch 8-stage"]
            SP["StylusPipeline"]
            VHF["VhfReporter<br/>HID 注入"]
        end
    end

    SSM -->|"IngestSystemEvent()"| CQ
    IPC -->|"SubmitCommand / SetVhf / ReloadConfig"| CQ
    PE -->|"InitStylus / DisconnectStylus"| CQ
    PP -->|"SetBtMcuPressure"| SP
    CQ -->|"AFE 命令"| CHIP
    CHIP -->|"GetFrame"| TP
    CHIP -->|"rawData"| SP
    TP -->|"TouchPacket"| VHF
    SP -->|"StylusPacket"| VHF
    DR -->|"FramePushCallback"| SFW
```

### 2.2 IPC 通信协议

**命令通道**: `\\.\pipe\EGoTouchControl`

| 命令 | 方向 | 用途 |
|------|------|------|
| `Ping` | App→Svc | 连接探活 |
| `EnterDebugMode` / `ExitDebugMode` | App→Svc | 控制调试帧推送 |
| `StartRuntime` / `StopRuntime` | App→Svc | 远程启停 Runtime |
| `AfeCommand` | App→Svc | AFE 控制命令 |
| `SetVhfEnabled` / `SetVhfTranspose` | App→Svc | VHF 输出控制 |
| `SetAutoAfeSync` | App→Svc | 当前为 placeholder（返回成功） |
| `ReloadConfig` / `SaveConfig` | App→Svc | 配置读取/保存 |
| `GetLogs` | App→Svc | 拉取 Service 端日志 |
| `GetPenBridgeStatus` | App→Svc | 查询 Pen 通道运行状态与压力数据 |

**帧通道**: Shared Memory `Global\EGoTouchSharedFrame`

### 2.3 DeviceRuntime 状态机

```mermaid
stateDiagram-v2
    [*] --> quit

    quit --> ready : Start()
    ready --> streaming : Chip::Init 成功
    ready --> recover : Chip::Init 失败

    streaming --> recover : 连续读帧失败达到阈值
    streaming --> suspend : ScreenOff/LidOff
    streaming --> quit : Shutdown/Stop

    recover --> streaming : 恢复成功
    recover --> suspend : 超过恢复上限
    recover --> quit : Shutdown/Stop

    suspend --> ready : DisplayOn/LidOn/Resume
    suspend --> quit : Shutdown/Stop

    quit --> [*]
```

**状态说明：**

| 状态 | Worker 行为 |
|------|------------|
| `ready` | auto mode 下尝试 `Chip::Init()` |
| `streaming` | `GetFrame()` + Touch/Stylus 处理 + VHF 上报 |
| `recover` | `check_bus()` + `Init()` 重试 |
| `suspend` | `HoldReset()` 后低功耗等待唤醒事件 |
| `quit` | 退出线程并释放运行态 |

### 2.4 线程汇总

| 线程 | 所属 | 职责 | 创建时机 |
|------|------|------|---------|
| `DeviceRuntime::WorkerMain` | DeviceRuntime | 状态机 + 采帧 + Pipeline + VHF | `DeviceRuntime::Start()` |
| `SystemStateMonitor::WorkerLoop` | Host | 监听 NamedEvent 并回调 | `ServiceHost::Start()` |
| `IpcPipeServer::ServerLoop` | Common | IPC 连接与命令分发 | `ServiceHost::Start()` |
| `PenEventBridge` 线程 | Device/penevt | BT 事件通道读取与处理 | `mode=full` 时 |
| `PenPressureReader` 线程 | Device/penpress | BT 压力通道读取 | `mode=full` 时 |
| `ServiceProxy::PollLoop` | App | 轮询 SharedMem 帧数据 | App 连接后 |
| `ServiceProxy::DiscoveryLoop` | App | 自动发现并重连 Service | App 启动后 |

### 2.5 Engine Pipeline 处理链（当前实现）

```text
Touch Pipeline (Service 侧注册顺序，共 8 段):
  1. MasterFrameParser
  2. BaselineSubtraction
  3. CMFProcessor
  4. GridIIRProcessor
  5. FeatureExtractor
  6. TouchTracker
  7. CoordinateFilter
  8. TouchGestureStateMachine

Stylus Pipeline:
  StylusPipeline
    -> GridPeakDetector
    -> CoordinateSolver
    -> CoorPostProcessor
    -> StylusPacket
```

---

## 三、目录结构（当前）

```text
EGoTouchRev-rebuild/
├── CMakeLists.txt
├── Common/
│   ├── include/
│   │   ├── Logger.h / GuiLogSink.h
│   │   ├── IpcProtocol.h / IpcPipeServer.h / IpcPipeClient.h
│   │   ├── SharedFrameBuffer.h
│   │   └── ConfigSync.h
│   └── source/
│
├── EGoTouchService/
│   ├── include/                      ← ServiceShell.h / ServiceHost.h
│   ├── source/                       ← ServiceEntry.cpp / ServiceShell.cpp / ServiceHost.cpp
│   ├── Device/
│   │   ├── runtime/                  ← DeviceRuntime
│   │   ├── himax/                    ← HimaxChip / HimaxAfe / Protocol / Registers
│   │   ├── vhf/                      ← VhfReporter
│   │   ├── btmcu/                    ← BT HID transport/channel
│   │   ├── penevt/                   ← PenEventBridge
│   │   └── penpress/                 ← PenPressureReader
│   ├── Engine/
│   │   ├── Preprocessing/
│   │   ├── TouchSolver/
│   │   ├── Reporting/
│   │   └── StylusSolver/
│   └── Host/                         ← SystemStateMonitor
│
├── Tools/
│   ├── EGoTouchApp/
│   ├── BtMcuTestTool/
│   └── tests/
│
├── scripts/
│   ├── install_service.bat
│   └── uninstall_service.bat
│
└── docs/
    └── architecture_redesign.md
```

---

## 四、当前实现与架构风险

### 4.1 风险清单

| # | 问题 | 影响 | 优先级 |
|---|------|------|--------|
| R1 | **配置链路双写**：App 直接写 `config.ini`，Service 也支持 `SaveConfig` | 配置主权分散，后续权限与审计复杂 | 🔴 P0 |
| R2 | **StylusPipeline 配置闭环不完整** | UI 改动可能未实际作用于 Service 运行态 | 🔴 P0 |
| R3 | **ConfigDirtyFlag 未在 Service 主循环消费** | `SetDirty` 机制价值不足，易误导 | 🟠 P1 |
| R4 | **Common 依赖 EngineTypes（反向耦合）** | 公共层复用与拆分难度上升 | 🟠 P1 |
| R5 | **调试帧通道受 `_DEBUG` 限制** | Release 下 App 调试能力不确定 | 🟡 P2 |
| R6 | **`SetAutoAfeSync` 是 placeholder** | UI 显示成功但行为不变 | 🟡 P2 |

### 4.2 详细说明

1. `ServiceProxy::SaveConfig()` 当前直接写 `C:/ProgramData/EGoTouchRev/config.ini`，再通知 `ReloadConfig`。
2. Service 侧 `BuildDefaultPipeline/ReloadConfig` 遍历的是 touch processors，StylusPipeline 配置未显式闭环。
3. `ConfigSync.h` 有 `CheckAndClear()`，但未见 Service 周期消费逻辑。
4. `SharedFrameBuffer` 直接包含 `EngineTypes`，使 Common 变成“准业务层”。
5. `EnterDebugMode` 的共享帧推送绑定 `_DEBUG` 分支，需明确产品策略。
6. `SetAutoAfeSync` 当前仅 `resp.success = true`，未落地 runtime 行为。

---

## 五、实施优先级建议

| 优先级 | 任务 | 涉及模块 |
|--------|------|---------|
| 🔴 P0 | 统一配置主权到 Service（App 不直接写文件） | `ServiceProxy`, `ServiceHost`, IPC |
| 🔴 P0 | 补齐 StylusPipeline 配置加载/保存 | `ServiceHost`, `DeviceRuntime`, `StylusPipeline` |
| 🟠 P1 | 明确并实现 ConfigDirtyFlag 消费路径 | `ServiceHost` 或 `DeviceRuntime` |
| 🟠 P1 | 拆分共享契约，降低 Common/Engine 耦合 | `Common`, `Engine`, 新 `DataContracts` |
| 🟡 P2 | 明确 Release 调试策略（启用或明确禁用） | `ServiceHost`, `ServiceProxy`, 文档/UI |
| 🟡 P2 | 清理 placeholder IPC 命令返回语义 | `ServiceHost`, `IpcProtocol`, App UI |

---

## 六、构建目标 (CMake)

| Target | 类型 | 用途 |
|--------|------|------|
| `EGoTouchService` | EXE | 生产服务（Shell + Host + Device + Engine） |
| `EGoTouchApp` | EXE | 调试诊断 GUI（ImGui DX11 + ServiceProxy） |
| `BtMcuTestTool` | EXE | BT MCU 协议验证 |
| `HostSystemStateMonitorTest` | EXE | Host 事件监听测试 |
| `EngineRawdataBenchmarkTest` | EXE | Engine 性能基准 |

**依赖图（按 CMake 当前配置）：**

```mermaid
graph LR
    Service[EGoTouchService] --> Device
    Service --> Host
    Service --> Common

    App[EGoTouchApp] --> Device
    App --> Engine
    App --> Host
    App --> Common

    Device --> Common
    Device --> Host
    Device --> Engine

    Engine --> Common
    Host --> Common
```

---

## 七、核心接口速查

### DeviceRuntime

```cpp
class DeviceRuntime {
    bool Start();
    void Stop();
    void SetAutoMode(bool enabled);
    void SetTouchOnlyMode(bool enabled);
    void SetStylusVhfEnabled(bool enabled);

    uint64_t SubmitCommand(command cmd, CommandSource src, const char* reason);
    void IngestSystemEvent(const Host::SystemStateEvent& ev);

    Engine::FramePipeline& GetPipeline();
    Engine::StylusPipeline& GetStylusPipeline();
    VhfReporter& GetVhfReporter();
};
```

### ServiceHost

```cpp
class ServiceHost {
    bool Start();   // Parse config -> Runtime -> Monitor -> IPC -> Pen(full)
    void Stop();    // IPC -> Pen -> Monitor -> Runtime
};
```

### ServiceProxy (App 端)

```cpp
class ServiceProxy {
    bool Connect();
    void Disconnect();
    bool GetLatestFrame(Engine::HeatmapFrame& out);

    bool SwitchAfeMode(uint8_t cmd, uint8_t param);
    bool SetVhfEnabled(bool enabled);
    bool SetVhfTranspose(bool enabled);

    void SaveConfig();
    void LoadConfig();
    PenBridgeStatus GetPenBridgeStatus() const;
};
```

