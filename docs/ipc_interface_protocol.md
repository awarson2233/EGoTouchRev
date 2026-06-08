# EGoTouch IPC 接口协议说明（historical Phase 1-3）

> 最后更新：2026-04-20  
> 状态：历史记录；当前 Config v3 契约见 [api/ipc_protocol.md](api/ipc_protocol.md) 与 [api/config_framework_api.md](api/config_framework_api.md)。

---

## 1. 概述

当前 App 与 Service 之间有两条 IPC 路径：

1. **Named Pipe 命令通道**（控制/查询）
2. **Shared Memory + Event 帧通道**（实时调试帧）

对应代码：

- 协议定义：`Common/include/IpcProtocol.h`
- Pipe 客户端：`Common/source/IpcPipeClient.cpp`
- Pipe 服务端处理：`EGoTouchService/source/ServiceHost.cpp`
- 共享帧 ABI：`Common/include/SharedFrameBuffer.h`
- 共享帧读写：`Common/source/SharedFrameBuffer.cpp`

---

## 2. 通道与资源名

### 2.1 Pipe 通道

- 名称：`\\.\pipe\EGoTouchControl`
- 方向：App ⇄ Service
- 用途：命令/响应

### 2.2 Shared Memory 帧通道

- 映射名：`Global\EGoTouchSharedFrame`
- 事件名：`Global\EGoTouchFrameReady`
- 方向：Service → App
- 用途：调试帧实时推送（低延迟）

### 2.3 其他 IPC 相关全局事件

- `Global\EGoTouchLogReady`
- `Global\EGoTouchPenStatusReady`

---

## 3. Pipe 报文结构

`IpcRequest` / `IpcResponse` 为固定结构体整包读写：

```cpp
struct IpcRequest {
    IpcCommand command;
    uint16_t   paramLen = 0;
    uint8_t    param[256]{};
};

struct IpcResponse {
    IpcStatusCode status = IpcStatusCode::InternalError;
    bool     success = false;
    uint16_t dataLen = 0;
    uint8_t  data[4096]{};
};
```

`IpcStatusCode` 当前定义：

- `Ok`
- `UnsupportedCommand`
- `InvalidRequest`
- `InvalidState`
- `NotFound`
- `PermissionDenied`
- `InternalError`

说明：

- 目前仍是固定包，不是变长 framing。
- `paramLen/dataLen` 是有效载荷长度标记，底层仍整包 `WriteFile/ReadFile`。

---

## 4. IpcCommand（当前实现）

| 命令 | 值 | 说明 |
|---|---:|---|
| `Ping` | 0 | 连通性检测 |
| `EnterDebugMode` | 1 | 开启调试帧推送（仅 Debug build） |
| `ExitDebugMode` | 2 | 关闭调试帧推送 |
| `StartRuntime` | 10 | 启动 runtime（幂等） |
| `StopRuntime` | 11 | 停止 runtime（幂等） |
| `AfeCommand` | 20 | AFE 命令转发 |
| `SetVhfEnabled` | 30 | VHF 开关 |
| `SetVhfTranspose` | 31 | VHF transpose 开关 |
| `ReloadConfig` | 40 | legacy config tombstone，unsupported |
| `SaveConfig` | 41 | legacy config tombstone，unsupported |
| `GetConfigSnapshot` | 42 | legacy config tombstone，unsupported |
| `ApplyConfigPatch` | 43 | legacy config tombstone，unsupported |
| `PersistConfig` | 44 | legacy config tombstone，unsupported |
| `ApplyConfigTlvChunk` | 45 | legacy config tombstone，unsupported |
| `GetConfigCatalogV3` | 46 | Config v3 catalog 分页读取 |
| `GetConfigSnapshotV3` | 47 | Config v3 snapshot 分页读取 |
| `ApplyConfigPatchV3` | 48 | Config v3 TLV patch |
| `PersistConfigV3` | 49 | Config v3 `UserOverride` persist |
| `GetLogs` | 50 | 拉取日志 |
| `GetPenBridgeStatus` | 60 | 查询 PenBridge 状态 |
| `GetDebugSchema` | 61 | 获取动态调试字段 schema |
| `GetDebugSnapshot` | 62 | 获取动态调试字段值 |
| `SetPenPressureMode` | 63 | 设置笔压力读取范围 |
| `SetMasterParserOnly` | 64 | 调试用，仅运行 Master Parser |
| `GetPenIdentityStatus` | 65 | 查询当前手写笔 ID / modelId / UTF-8 hardwareVersion |

---

## 5. 配置相关协议（superseded）

以下 fixed config ABI 是历史 Phase 1-3 记录，不是当前 connected config 主路径。当前 Service connected mode 使用 Config v3 command `46`-`49`；legacy config command `40`-`45` 仅保留 ABI tombstone 并返回 unsupported。

### 5.1 `GetConfigCatalogV3` (`46`) / `GetConfigSnapshotV3` (`47`)

分页返回 Config v3 catalog/snapshot payload，条目使用 static `ConfigKeyId` 绑定 YAML path。schema/current value/strategy metadata 由 `ConfigRuntime` 构建。

### 5.2 `ApplyConfigPatchV3` (`48`)

请求携带 base schema/snapshot version 和 TLV patch payload。语义结果包括 `Ok`、`NoChanges`、`VersionMismatch`、`Rejected`、`PersistFailed`；格式错误请求仍走 IPC failure。

### 5.3 `PersistConfigV3` (`49`)

请求：无  
响应：`PersistConfigV3ResponseWire`

语义：Service 按 catalog 中 `ConfigPersistPolicy::UserOverride` 写 `overrides.yaml`；`default.yaml` 是 factory default 输入，legacy `config.ini` 不是当前目标格式。

### 5.4 legacy command tombstone

- `ReloadConfig` (`40`) / `SaveConfig` (`41`) / `GetConfigSnapshot` (`42`) / `ApplyConfigPatch` (`43`) / `PersistConfig` (`44`) / `ApplyConfigTlvChunk` (`45`)：保留 command value，connected IPC 返回 `UnsupportedCommand`。

---

## 6. 其他命令要点

### 6.1 `EnterDebugMode`

- 请求参数：共享内存名（`wchar_t[]`）
- 现状：服务端当前并不按请求名切换映射；仅检查既有 `SharedFrameWriter`。
- Release 构建下返回 `UnsupportedCommand`。

### 6.2 `AfeCommand`

- 请求参数：2 字节（`AFE_Command` + `param`）
- 行为：进入 `DeviceRuntime` 命令队列。

### 6.3 `GetPenBridgeStatus`

返回固定 24 字节：

- `evtRunning`
- `pressRunning`
- `reportType`
- `freq1/freq2`
- `press0..press3`（little-endian `uint16_t`）
- `pressureMode`
- `pressureMax`
- `rawPress0..rawPress3`（little-endian `uint16_t`）

### 6.4 `GetPenIdentityStatus`

返回固定 140 字节 `PenIdentityStatusWire`：

- `wireVersion`
- `flags`：bit0=`stylusId` 有效，bit1=`penModuleModelId` 有效，bit2=`hardwareVersionUtf8` 有效，bit3=`connected`
- `stylusId`：`PenTypeInfo payload[0]`
- `penModuleModelId`：`PenModule` 小端 modelId
- `hardwareVersionUtf8Len`：UTF-8 字节长度，不含 NUL
- `hardwareVersionUtf8[128]`：UTF-8 validated 字符串，服务端按字符边界安全截断

### 6.5 `GetDebugSchema` / `GetDebugSnapshot`

- `GetDebugSchema`：返回 schema header + records
- `GetDebugSnapshot`：返回 snapshot header + values
- snapshot 中 `fieldId/valueType/flags/rawValue` 与 schema 配套解析

---

## 7. Shared Memory ABI（当前实现）

### 7.1 ABI 头

`SharedFrameBuffer.h` 已定义稳定 ABI 头：

- `SharedFrameAbiHeader`
- `kSharedFrameAbiVersion`（当前为 `1`）
- `capabilities/reserved`

### 7.2 Triple buffer 发布模型

顶层：`SharedTripleBuffer`

- `readyIdx`
- `frameId/slaveFrameId/masterFrameId`
- `slots[3]` (`SharedFrameData`)

发布语义：

1. Writer 写入当前 slot。
2. `readyIdx.store(..., release)` 发布。
3. Reader 读取 `readyIdx` 对应 slot。

### 7.3 `SharedFrameData` 方向

当前已是 Common-owned POD 共享结构，包含：

- runtime 状态字段
- heatmap / contacts / packets
- stylus 数据与 diagnostics mirror
- structured suffix (`MasterSuffixView` / `SlaveSuffixView`)

---

## 8. 当前已知兼容边界

1. Pipe 协议仍是固定结构体整包，尚未引入独立 framing/version header。
2. `success + status` 并存；新路径已使用 `status`，旧路径仍存在兼容语义。
3. 配置 canonical 路径已落地为 Config v3；`ReloadConfig/SaveConfig` 仅保留 legacy tombstone。

---

## 9. 下一阶段 follow-ups（延期项）

1. **保持 legacy tombstone ABI 稳定**：`ReloadConfig/SaveConfig` 等旧 command value 不再恢复为主路径。
2. **统一状态码语义**：将剩余老命令补齐一致的 `IpcStatusCode` 赋值策略。  
3. **明确 EnterDebugMode 请求参数语义**：要么真正按请求名工作，要么收敛为无参命令。
