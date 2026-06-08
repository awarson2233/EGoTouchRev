# EGoTouch 驱动服务 — 架构文档

> 最后更新：2026-05-30  
> 状态：历史架构记录；Config 章节已被 [ipc_protocol.md](api/ipc_protocol.md) 与 [config_framework_api.md](api/config_framework_api.md) 取代。

---

## 1. 当前架构总览

```text
EGoTouchService.exe
  ServiceShell
    -> ServiceHost
       -> DeviceRuntime
       -> Host::SystemStateMonitor
       -> IpcPipeServer
       -> PenEventBridge / PenPressureReader (full 模式)
       -> SharedFrameWriter (Debug)
       -> DVRCore (DVR2 binary/csv/runtime-config IO)

EGoTouchApp.exe
  ServiceProxy
    -> IpcPipeClient
    -> SharedFrameReader
    -> DVRCore (DVR2 import/export + CSV export)
    -> 本地 Touch/Stylus 配置镜像（UI 编辑用）
```

核心边界：

1. **配置权威语义在 Service**（snapshot/patch/persist）
2. **system-state 归一化在 Host::SystemStateMonitor**
3. **DeviceRuntime 以门面接口（Facade）暴露，不再泄漏内部 pipeline/VHF 对象**
4. **DVR2 文件读写能力已从 App 抽离到 `DVRCore` 静态库**

---

## 2. 配置链路（Service-owned + thin-client 语义）

### 2.1 当前 Config v3 配置命令

`IpcCommand` 已包含：

| 命令 | 值 | 当前角色 |
|---|---:|---|
| `ReloadConfig` - `ApplyConfigTlvChunk` | `40`-`45` | legacy config tombstone，connected IPC 返回 `UnsupportedCommand` |
| `GetConfigCatalogV3` | `46` | 分页读取 Service v3 catalog |
| `GetConfigSnapshotV3` | `47` | 分页读取 Service v3 snapshot |
| `ApplyConfigPatchV3` | `48` | 通过 static `ConfigKeyId` + TLV patch 修改配置 |
| `PersistConfigV3` | `49` | 按 `UserOverride` policy 写入 `overrides.yaml` |

旧 fixed config ABI (`ConfigSnapshotWire` / `ApplyConfigPatchRequestWire` / `PersistConfigResponseWire`) 不再是 connected config 主路径。

### 2.2 desired / active 语义

当前快照明确区分：

- `desiredMode`：配置目标模式
- `activeMode`：当前 runtime 实际模式

App 侧 `ServiceProxy` 已分别维护镜像（desired/active），不再把两者混成单一 `mode` 语义。

### 2.3 当前 connected/fallback 行为

App connected mode 只走 Config v3 catalog/snapshot/patch/persist；本地 binder/YAML fallback 仅用于 Service 不可用或离线预览。`ServiceProxy::SaveConfig()` 当前只是调用 v3 `ApplyConfigStoreGlobally()` 的兼容入口。

---

## 3. IPC / Debug / Shared-memory 契约现状

### 3.1 Pipe 协议

- `kIpcProtocolVersion` 当前为 `2`。
- `IpcResponse` 已包含 `IpcStatusCode`，同时保留 `success` 兼容字段。
- 传输层仍是固定结构体整包读写（非变长 framing）。
- `ReloadConfig` / `SaveConfig` 等 legacy config commands 仅保留 command value tombstone，不再是 supported connected IPC。

当前 `IpcStatusCode`：

| 状态 | 值 |
|---|---:|
| `Ok` | `0` |
| `UnsupportedCommand` | `1` |
| `InvalidRequest` | `2` |
| `InvalidState` | `3` |
| `NotFound` | `4` |
| `PermissionDenied` | `5` |
| `InternalError` | `6` |

### 3.2 Debug / runtime control IPC

除配置命令外，当前协议还包含以下已落地命令：

| 命令 | 值 | 当前角色 |
|---|---:|---|
| `EnterDebugMode` | `1` | App 请求 Service 打开 shared memory 并推送 debug frame |
| `ExitDebugMode` | `2` | 停止 debug frame 推送 |
| `StartRuntime` | `10` | 启动 runtime（幂等） |
| `StopRuntime` | `11` | 停止 runtime（幂等） |
| `AfeCommand` | `20` | 外部 AFE 命令入口 |
| `SetVhfEnabled` | `30` | VHF 输出开关 |
| `SetVhfTranspose` | `31` | VHF transpose 开关 |
| `GetLogs` | `50` | 读取 Service 日志 |
| `GetPenBridgeStatus` | `60` | 查询 BT MCU pressure bridge 状态 |
| `GetDebugSchema` | `61` | 读取 dynamic debug schema |
| `GetDebugSnapshot` | `62` | 读取 dynamic debug value snapshot |
| `SetPenPressureMode` | `63` | 切换 pen pressure 映射模式 |
| `SetMasterParserOnly` | `64` | 切换 service-side master-parser-only 模式 |

### 3.3 Shared-memory ABI

`SharedFrameBuffer.h` 已具备 Common-owned ABI 头与版本字段：

- `SharedFrameAbiHeader`
- `kSharedFrameAbiVersion`（当前 `3`）
- `SharedTripleBuffer`

ABI header 当前记录：

- `abiVersion`
- `totalSize`
- `headerSize`
- `capabilities`
- `slotCount`
- `reserved`

Shared frame 仍是 POD 稳定布局方向，包含 touch/stylus packet、contact、diagnostics mirror、dynamic debug snapshot 等固定布局数据。

---

## 4. System-state 语义归一化 ownership

### 4.1 分层职责

- `ServiceShell`：接收 SCM / Power 原始事件，并发出 transport named event。
- `Host::SystemStateMonitor`：监听 named event，执行 normalization + dedupe。
- `ServiceHost`：把 `Host::SystemStateEvent` 翻译为 `RuntimePolicyEvent`，再下发到 `DeviceRuntime`。
- `DeviceRuntime`：只消费 policy event（Display/Lid/Shutdown/Resume），不再作为 normalized owner。

### 4.2 canonical/legacy transport 现状

`SystemStateEvent.h` 中已标注：

- canonical：`MonitorConsoleDisplayOn/Off`、`MonitorLidOn/Off`、`MonitorShutDown`、`PBT_APMRESUMEAUTOMATIC`
- legacy alias：`MonitorPowerOn/Off`

`SystemStateMonitor` 在同批事件中优先 canonical transport，并对 normalized type 做去重。

---

## 5. DeviceRuntime 门面隔离接缝

`DeviceRuntime` 当前公开接口集中在：

- lifecycle：`Start/Stop/RequestStart/RequestStop`
- runtime state：`IsShutdownRequested/IsRunning/IsSuspended`
- service policy：`SetAutoMode/IsAutoMode`、`SetStylusVhfEnabled/IsStylusVhfEnabled`、`ApplyServicePolicy`
- pen button policy：`SetPenButtonMode/GetPenButtonMode`、`SetPenButtonRoute/GetPenButtonRoute`
- policy ingress：`IngestPolicyEvent`
- command ingress：`SubmitExternalAfeCommand`、`SubmitCommand`
- 配置门面（Facade）：LoadPipelineConfig/LoadStylusPipelineConfig、SavePipelineConfig/SaveStylusPipelineConfig
- VHF / 解析器门面（Facade）：SetVhfEnabled、SetVhfTransposeEnabled、SetMasterParserOnlyMode
- BT MCU pressure ingress：`IngestBtMcuPressure`、`IngestBtMcuPressurePacket`
- pen event ingress：`IngestPenEvent`
- snapshot/history：`GetSnapshot/GetHistory/ClearHistory`
- debug frame callback：`SetFramePushCallback`（`_DEBUG` only）

已不再公开：

- `GetPipeline()`
- `GetStylusPipeline()`
- `GetVhfReporter()`
- `IngestSystemEvent(const Host::SystemStateEvent&)`

这条隔离接缝已使 `ServiceHost` 与 runtime 内部对象解耦；Service 侧只能通过 runtime 门面接口访问 pipeline、VHF、policy、command 与 debug frame 相关能力。

---

## 6. TouchPipeline 当前处理链路

当前 `TouchPipeline` 仍是线性 solver orchestration，核心模块按成员暴露用于配置与诊断：

```text
HeatmapFrame
  -> MasterFrameParser
  -> BaselineSubtraction
  -> CMFProcessor
  -> MacroZoneDetector
  -> PeakDetector
  -> TouchClassifier
  -> ContactExtractor / ZoneExpander
  -> EdgeCompensator / EdgeRejector
  -> StylusTouchSuppressor
  -> TouchTracker
  -> CoordinateFilter
  -> Touch packet / diagnostics outputs
```

### 6.1 Baseline reacquire / dynamic baseline

当前 `TouchPipeline` 已暴露：

- `RequestBaselineReacquire(int frames = 8)`

该入口调用 `BaselineSubtraction::RequestReacquireFrames()`，用于在 runtime policy / recovery 场景中请求后续若干帧重新采集 baseline。

`BaselineSubtraction` 当前包含：

- baseline Q8 内部累积
- snapshot reacquire
- touch freeze threshold
- release hold frames
- positive / negative / noise drift deadband
- acquisition alpha / max-step
- settle frames after reacquire
- optional noise tracking

### 6.2 GridIIR 状态

`GridIIRProcessor.hpp` 和对应测试已从当前工作区删除；当前 touch recovery / baseline reacquire 不再依赖 GridIIR 作为主流程阶段。

---

## 7. DVRCore / DVR2 文件能力

### 7.1 模块边界

当前 `CMakeLists.txt` 已把 DVR 文件能力抽离为独立静态库：

```text
Common/DVRCore
  include/
    DvrBinaryIO.h
    DvrCsvExport.h
    DvrFormat.h
    DvrFrameSlot.h
    DvrTypes.h
  source/
    DvrBinaryReader.cpp
    DvrBinaryWriter.cpp
    DvrCsvExport.cpp
```

链接关系：

| Target | DVRCore 使用方式 |
|---|---|
| `EGoTouchApp` | DVR2 import/export、CSV export、playback |
| `EGoTouchService` | 共享 DVR 类型与 IO 能力 |
| tests | DVR2 round-trip / ServiceProxy config merge 等测试 |

### 7.2 Frame slot 与 zero-copy DVR

`DvrFrameSlot` 当前是 fixed-size POD frame slot，用于 diagnostics DVR recording 的 ring-buffer 与文件写入路径，避免在录制热路径中依赖 heap-allocating frame 容器。

### 7.3 DVR2 runtime config snapshot

DVR2 IO 接口已支持随 recording 写入/读取 runtime config snapshot：

- `WriteDvrBinaryFile(..., const DvrRuntimeConfigSnapshot* runtimeConfig, ...)`
- `ReadDvrBinaryFile(..., DvrRuntimeConfigSnapshot* outRuntimeConfig)`

`DvrRuntimeConfigSnapshot` 当前由两部分组成：

- `fields`：runtime config schema / field definition
- `values`：录制时对应字段值

该能力用于让 DVR2 replay / CSV export 基于文件内嵌 schema 还原当时 runtime 配置，而不是依赖当前 writer 的字段集合。

---

## 8. 已知限制与下一阶段 follow-ups

1. **real service-process e2e 仍未纳入默认 gate**
   真实 Windows service-process e2e 只应在 dedicated disposable service/hardware harness 上 opt-in；当前本地 gate 使用 device-free runtime/packaging/App fake v3 IPC 测试。

2. **legacy 配置命令仍需保留 tombstone**
   `ReloadConfig/SaveConfig` 等 command value 保持 ABI 稳定，但 connected IPC 返回 unsupported。

3. **IPC 状态码一致性仍需收口**  
   新命令已使用 `IpcStatusCode`，部分旧命令路径仍以 `success` 为兼容字段。

4. **`SetAutoAfeSync` 仍未落地**  
   文档中不再作为当前可用能力；保留为后续语义收口项。

5. **Debug 帧推送仍受构建配置约束**  
   `EnterDebugMode` 在 Release 下返回不支持，后续如需产品化需单独决策。

6. **DVR2 runtime config 已落地，但仍需保持 schema-driven 读取语义**  
   replay / export 侧应优先使用文件内嵌 schema，不应假定当前代码中的 writer 字段集合等于旧文件字段集合。
