# EGoTouch 驱动服务 — 架构文档（Phase 4 收口版）

> 最后更新：2026-04-20  
> 状态：按 Phase 1-3 已落地代码对齐（仅记录当前行为）

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

EGoTouchApp.exe
  ServiceProxy
    -> IpcPipeClient
    -> SharedFrameReader
    -> 本地 Touch/Stylus 配置镜像（UI 编辑用）
```

核心边界：

1. **配置权威语义在 Service**（snapshot/patch/persist）
2. **system-state 归一化在 Host::SystemStateMonitor**
3. **DeviceRuntime 以 façade 暴露，不再泄漏内部 pipeline/VHF 对象**

---

## 2. 配置链路（Service-owned + thin-client 语义）

### 2.1 已落地的 canonical 命令

`IpcCommand` 已包含：

- `GetConfigSnapshot` (`42`)
- `ApplyConfigPatch` (`43`)
- `PersistConfig` (`44`)

`ServiceHost` 已实现对应处理，并返回 typed wire：

- `ConfigSnapshotWire`
- `ConfigMutationResultWire`
- `PersistConfigResponseWire`

### 2.2 desired / active 语义

当前快照明确区分：

- `desiredMode`：配置目标模式
- `activeMode`：当前 runtime 实际模式

App 侧 `ServiceProxy` 已分别维护镜像（desired/active），不再把两者混成单一 `mode` 语义。

### 2.3 当前兼容行为（仍存在）

为兼容既有 pipeline 配置流程，App 端 `SaveConfig()` 在已连接模式下仍会：

1. 先走 `ApplyConfigPatch + PersistConfig` 更新 `[Service]`
2. 本地合并写入 touch/stylus pipeline section
3. 调用 `ReloadConfig` 触发服务重载 pipeline

因此当前是“Service 负责 `[Service]` 语义 + pipeline section 兼容重载”的混合阶段。

---

## 3. System-state 语义归一化 ownership

### 3.1 分层职责

- `ServiceShell`：接收 SCM / Power 原始事件，并发出 transport named event。
- `Host::SystemStateMonitor`：监听 named event，执行 normalization + dedupe。
- `ServiceHost`：把 `Host::SystemStateEvent` 翻译为 `RuntimePolicyEvent`，再下发到 `DeviceRuntime`。
- `DeviceRuntime`：只消费 policy event（Display/Lid/Shutdown/Resume），不再作为 normalized owner。

### 3.2 canonical/legacy transport 现状

`SystemStateEvent.h` 中已标注：

- canonical：`MonitorConsoleDisplayOn/Off`、`MonitorLidOn/Off`、`MonitorShutDown`、`PBT_APMRESUMEAUTOMATIC`
- legacy alias：`MonitorPowerOn/Off`

`SystemStateMonitor` 在同批事件中优先 canonical transport，并对 normalized type 做去重。

---

## 4. DeviceRuntime façade seam（已收口）

`DeviceRuntime` 当前公开接口集中在：

- lifecycle（`Start/Stop/RequestStart/RequestStop`）
- policy ingress（`IngestPolicyEvent`）
- command ingress（`SubmitExternalAfeCommand`）
- 配置 façade（load/save pipeline config、VHF 控制）

已不再公开：

- `GetPipeline()`
- `GetStylusPipeline()`
- `GetVhfReporter()`
- `IngestSystemEvent(const Host::SystemStateEvent&)`

这条 seam 已使 `ServiceHost` 与 runtime 内部对象解耦。

---

## 5. IPC / Shared-memory 契约现状

### 5.1 Pipe 协议

- `IpcResponse` 已包含 `IpcStatusCode`，同时保留 `success` 兼容字段。
- 传输层仍是固定结构体整包读写（非变长 framing）。

### 5.2 Shared-memory ABI

`SharedFrameBuffer.h` 已具备 Common-owned ABI 头与版本字段：

- `SharedFrameAbiHeader`
- `kSharedFrameAbiVersion`（当前 `1`）
- triple-buffer `SharedTripleBuffer`

Shared frame 仍是 POD 稳定布局方向，包含 touch/stylus/suffix 与 diagnostics mirror。

---

## 6. 已知限制与下一阶段 follow-ups

1. **配置链路最终收口未完成**  
   App 仍会本地写 pipeline section 并触发 `ReloadConfig`；下一阶段应继续压缩该兼容路径。

2. **legacy 配置命令仍在主流程中出现**  
   `ReloadConfig/SaveConfig` 仍可用；下一阶段需继续降级并最终退役主控制角色。

3. **IPC 状态码一致性仍需收口**  
   新命令已使用 `IpcStatusCode`，部分旧命令路径仍以 `success` 为主。

4. **`SetAutoAfeSync` 仍未落地**  
   文档中不再作为当前可用能力；保留为后续语义收口项。

5. **Debug 帧推送仍受构建配置约束**  
   `EnterDebugMode` 在 Release 下返回不支持，后续如需产品化需单独决策。

---

## 7. 结论

截至 Phase 1-3，三条主线已达成可验证落地：

1. 配置 canonical 路径已在 Service 建立（并支持 desired/active 区分）。
2. Host 已成为 system-state normalization owner。
3. DeviceRuntime 已收口为 façade seam。

Phase 4 文档收口后，后续工作重点是“兼容路径退役与语义一致性收尾”，不是再次改写主契约。