# Phase 0 Contract Freeze

> 日期：2026-04-20  
> 状态：冻结基线 + Phase 1-3 已落地状态（Phase 4 文档收口）

---

## 1. 冻结目标（保持不变）

Phase 0 冻结的四条主契约仍然有效：

1. 配置主权收敛到 Service。
2. system-state 归一化语义主权收敛到 Host（SystemStateMonitor）。
3. DeviceRuntime 公共边界收敛为 façade，不暴露内部对象引用。
4. IPC / shared-memory 走稳定、可版本化契约。

---

## 2. Phase 1-3 实际落地状态（as-built）

### 2.1 配置主权（Service-owned）

**已落地：**
- `IpcProtocol.h` 新增 canonical 配置命令：
  - `GetConfigSnapshot`
  - `ApplyConfigPatch`
  - `PersistConfig`
- `ServiceHost` 已实现上述命令处理，`[Service]` 字段由 Service 解释并持久化。
- 快照中已明确区分：
  - `desiredMode`
  - `activeMode`
  - `autoMode`
  - `stylusVhfEnabled`

**当前兼容现状（仍存在）：**
- App 端 `ServiceProxy::SaveConfig()` 在已连接模式下，仍会本地合并并写入 pipeline section（兼容路径），随后调用 `ReloadConfig` 让 Service 重载。
- `ReloadConfig` / `SaveConfig` 仍保留并可用，其中 `SaveConfig` 已是 `PersistConfig` 别名。

### 2.2 Host normalization ownership

**已落地：**
- `ServiceShell` 负责 SCM/Power 原始事件转 transport named event。
- `Host::SystemStateMonitor` 负责：
  - named event → normalized event 映射
  - canonical/legacy alias 归并
  - 去重（display/lid/shutdown）
- `ServiceHost` 将 `Host::SystemStateEvent` 翻译为 `RuntimePolicyEvent` 后下发到 runtime。

### 2.3 DeviceRuntime façade seam

**已落地：**
- `DeviceRuntime` 公共边界已改为 façade 风格：
  - lifecycle (`Start/Stop/RequestStart/RequestStop`)
  - typed policy ingress (`IngestPolicyEvent`)
  - typed command ingress (`SubmitExternalAfeCommand`)
  - 配置 façade 方法（pipeline/stylus/vhf）
- `DeviceRuntime` 不再暴露：
  - `GetPipeline()`
  - `GetStylusPipeline()`
  - `GetVhfReporter()`
- `DeviceRuntime` 不再公开 `Host::SystemStateEvent` 作为接口类型。

### 2.4 IPC / Shared-memory 契约

**已落地：**
- `IpcResponse` 已包含 `IpcStatusCode`（`Ok/InvalidRequest/...`）。
- 配置控制命令已使用 typed wire：
  - `ConfigSnapshotWire`
  - `ApplyConfigPatchRequestWire`
  - `ConfigMutationResultWire`
  - `PersistConfigResponseWire`
- `SharedFrameBuffer` 已固化为 Common-owned POD ABI：
  - `SharedFrameAbiHeader`
  - triple-buffer (`SharedTripleBuffer`)
  - `kSharedFrameAbiVersion`

---

## 3. 仍延期项（下一阶段 follow-ups）

1. **彻底移除 App 直写 config 文件的兼容路径**  
   当前 App 仍会写 pipeline section 并触发 `ReloadConfig`；下一阶段应收敛为纯 Service 侧配置提交/持久化。

2. **退役 legacy 配置命令主路径**  
   `ReloadConfig` / `SaveConfig` 仍在用；下一阶段应降为仅兼容入口，最终退出主控制路径。

3. **IPC status 一致性收口**  
   `IpcStatusCode` 已引入，但部分老命令处理仍主要依赖 `success`；下一阶段应统一状态码填充语义。

4. **runtime 侧策略防抖边界复核**  
   Host 已承担归一化与主去重；runtime 仍有事件防抖保护。下一阶段可按“唯一 owner”原则继续收口。

---

## 4. 本文档用途

本文档继续作为 Phase 0 契约基线；以上“已落地/延期项”用于 Phase 4 文档收口，反映当前代码真实状态，不代表新增规划。