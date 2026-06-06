# EGoTouch IPC 接口协议说明（Phase 1-3 as-built）

> 最后更新：2026-04-20  
> 状态：按当前已落地代码对齐（非规划稿）

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
| `ReloadConfig` | 40 | 兼容命令：重载配置 |
| `SaveConfig` | 41 | 兼容别名：转发到 `PersistConfig` |
| `GetConfigSnapshot` | 42 | 获取 Service authoritative 配置快照 |
| `ApplyConfigPatch` | 43 | 提交配置修改意图 |
| `PersistConfig` | 44 | 持久化 canonical 配置 |
| `GetLogs` | 50 | 拉取日志 |
| `GetPenBridgeStatus` | 60 | 查询 PenBridge 状态 |
| `GetDebugSchema` | 61 | 获取动态调试字段 schema |
| `GetDebugSnapshot` | 62 | 获取动态调试字段值 |
| `SetPenPressureMode` | 63 | 设置笔压力读取范围 |
| `SetMasterParserOnly` | 64 | 调试用，仅运行 Master Parser |
| `GetPenIdentityStatus` | 65 | 查询当前手写笔 ID / modelId / UTF-8 hardwareVersion |

---

## 5. 配置相关协议（Phase 1-3 已落地）

### 5.1 `GetConfigSnapshot` (`42`)

返回 `ConfigSnapshotWire`，关键字段：

- `desiredMode`（配置目标）
- `activeMode`（当前运行）
- `autoMode`
- `stylusVhfEnabled`
- `definedFields`
- `wireVersion`

### 5.2 `ApplyConfigPatch` (`43`)

请求：`ApplyConfigPatchRequestWire`  
响应：`ConfigMutationResultWire`

语义：

- `changedFields`
- `appliedFields`
- `restartRequiredFields`

### 5.3 `PersistConfig` (`44`)

请求：无  
响应：`PersistConfigResponseWire`

语义：由 Service 将当前 canonical 配置写入 `C:/ProgramData/EGoTouchRev/config.ini`。

### 5.4 兼容命令

- `ReloadConfig` (`40`)：返回 `ReloadConfigSummaryWire`（3 字节）。
- `SaveConfig` (`41`)：服务端内部转发到 `PersistConfig`。

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
3. 配置 canonical 路径已落地，但 `ReloadConfig/SaveConfig` 仍保留兼容。

---

## 9. 下一阶段 follow-ups（延期项）

1. **退役 legacy 配置主路径**：逐步退出 `ReloadConfig/SaveConfig` 的主控制角色。  
2. **统一状态码语义**：将剩余老命令补齐一致的 `IpcStatusCode` 赋值策略。  
3. **明确 EnterDebugMode 请求参数语义**：要么真正按请求名工作，要么收敛为无参命令。