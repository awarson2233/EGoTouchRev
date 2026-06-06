# IPC 通信协议文档

> EGoTouchRev Service ↔ App 进程间通信协议 | 版本: 2 | 日期: 2026-06-05

---

## 1. 概述

EGoTouchService (Windows Service, Session 0) 与 EGoTouchApp (桌面应用, User Session) 之间通过 **Named Pipe** 进行命令/响应通信，通过 **Shared Memory** 进行实时帧数据推送。

```
┌─────────────────────┐                    ┌─────────────────────┐
│   EGoTouchApp       │                    │   EGoTouchService   │
│   (User Session)    │                    │   (Session 0)       │
│                     │                    │                     │
│  IpcPipeClient ─────┼──── Named Pipe ────┼── IpcPipeServer     │
│       │             │  \\.\pipe\          │       │             │
│       │             │  EGoTouchControl    │       │             │
│       ▼             │                    │       ▼             │
│  SharedFrameReader  │◄── Shared Memory ──│  SharedFrameWriter  │
│                     │  Global\            │                     │
│                     │  EGoTouchSharedFrame│                     │
│  ConfigDirtyFlag ───┼── Shared Memory ───┼── ConfigDirtyFlag   │
│  (Writer)           │  Global\            │  (Reader)           │
│                     │  EGoTouchConfigDirty│                     │
└─────────────────────┘                    └─────────────────────┘
```

---

## 2. 传输层

### 2.1 Named Pipe

| 属性 | 值 |
|------|-----|
| Pipe 名称 | `\\.\pipe\EGoTouchControl` |
| 方向 | 双向 (App 发请求, Service 发响应) |
| 并发 | 单连接 (单 App 实例) |
| 安全 | Admin-only (`D:P(A;;GA;;;SY)(A;;GA;;;BA)`) |
| App 连接超时 | 5 秒 |
| 重连策略 | App 侧主动重连 |

### 2.2 消息帧格式

**请求帧** (`IpcRequest`, 260 bytes):

```
┌──────────────┬──────────────┬─────────────────────────────────┐
│ command      │ paramLen     │ param[256]                       │
│ uint8_t      │ uint16_t     │ uint8_t[]                        │
│ 1 byte       │ 2 bytes      │ 256 bytes                        │
└──────────────┴──────────────┴─────────────────────────────────┘
```

**响应帧** (`IpcResponse`, 4100 bytes):

```
┌──────────────┬──────────────┬──────────────┬──────────────────┐
│ status       │ success      │ dataLen      │ data[4096]        │
│ uint8_t      │ bool (1B)    │ uint16_t     │ uint8_t[]         │
│ 1 byte       │ 1 byte       │ 2 bytes      │ 4096 bytes        │
└──────────────┴──────────────┴──────────────┴──────────────────┘
```

### 2.3 协议版本

| 版本 | 说明 |
|------|------|
| v1 | 初始版本 |
| v2 | 引入 `GetConfigSnapshot` / `ApplyConfigPatch` / `PersistConfig` 作为 canonical config 控制路径 |

---

## 3. 命令参考

### 3.1 系统命令

#### Ping (0x00)

检测 Service 是否存活。

| 方向 | 字段 | 值 |
|------|------|-----|
| Req | command | `0x00` |
| Req | paramLen | `0` |
| Res | success | `true` |

---

#### EnterDebugMode (0x01)

App 请求 Service 开始推送调试帧到共享内存。

| 方向 | 字段 | 值 |
|------|------|-----|
| Req | command | `0x01` |
| Req | param | 共享内存名称 (UTF-16 wchar_t[]) |
| Req | paramLen | 名称字节数 |
| Res | success | `true` / `false` |
| Res | status (失败) | `InvalidState` (DeviceRuntime 未就绪) |

> :warning: 仅在 `_DEBUG` 构建中可用。Release 中直接返回失败。

---

#### ExitDebugMode (0x02)

App 请求 Service 停止推送调试帧。

| 方向 | 字段 | 值 |
|------|------|-----|
| Req | command | `0x02` |
| Req | paramLen | `0` |
| Res | success | `true` |

---

### 3.2 运行时控制

#### StartRuntime (0x0A)

启动 DeviceRuntime (硬件数据采集 + Pipeline 处理)。

| 方向 | 字段 | 值 |
|------|------|-----|
| Req | command | `0x0A` |
| Req | paramLen | `0` |
| Res | success | `true` (已启动或成功启动) |

> 幂等: 重复调用返回 success=true。

---

#### StopRuntime (0x0B)

停止 DeviceRuntime。

| 方向 | 字段 | 值 |
|------|------|-----|
| Req | command | `0x0B` |
| Req | paramLen | `0` |
| Res | success | `true` (已停止或成功停止) |

> 幂等: 重复调用返回 success=true。

---

### 3.3 AFE 命令

#### AfeCommand (0x14)

发送 AFE (Analog Front-End) 控制命令。

| 方向 | 字段 | 值 |
|------|------|-----|
| Req | command | `0x14` |
| Req | param[0] | AFE 命令码 (`uint8_t`) |
| Req | param[1] | 命令参数 (`uint8_t`) |
| Req | paramLen | `2` |
| Res | success | `true` / `false` |
| Res | status (失败) | `InvalidRequest` (paramLen < 2) |

---

### 3.4 VHF 控制

#### SetVhfEnabled (0x1E)

启用/禁用 VHF (Virtual HID Framework) 设备。

| 方向 | 字段 | 值 |
|------|------|-----|
| Req | command | `0x1E` |
| Req | param[0] | `0` = 禁用, 非零 = 启用 |
| Req | paramLen | `1` |
| Res | success | `true` / `false` |
| Res | status (失败) | `InvalidRequest` (paramLen < 1) / `InvalidState` (DeviceRuntime 未就绪) |

---

#### SetVhfTranspose (0x1F)

启用/禁用 VHF 坐标转置。

| 方向 | 字段 | 值 |
|------|------|-----|
| Req | command | `0x1F` |
| Req | param[0] | `0` = 禁用, 非零 = 启用 |
| Req | paramLen | `1` |
| Res | success | `true` / `false` |
| Res | status (失败) | `InvalidRequest` / `InvalidState` |

---

### 3.5 配置命令

#### GetConfigSnapshot (0x2A)

获取 Service 当前配置快照 (期望值 + 实际生效值)。

| 方向 | 字段 | 值 |
|------|------|-----|
| Req | command | `0x2A` |
| Req | paramLen | `0` |
| Res | success | `true` / `false` |
| Res | data | `ConfigSnapshotWire` (10 bytes) |
| Res | dataLen | `sizeof(ConfigSnapshotWire)` = 10 |
| Res | status (失败) | `InvalidState` (DeviceRuntime 未就绪) |

**ConfigSnapshotWire** 布局 (10 bytes):

```
Offset  Size  Field               Type      说明
------  ----  -----               ----      ----
0       2     wireVersion         uint16_t  协议版本 (2)
2       1     definedFields       uint8_t   有效字段位掩码
3       1     desiredMode         uint8_t   期望模式 (见 ServiceModeWire)
4       1     activeMode          uint8_t   实际生效模式
5       1     autoMode            uint8_t   自动模式 (0/1)
6       1     stylusVhfEnabled    uint8_t   Stylus VHF 启用 (0/1)
7       1     penButtonMode       uint8_t   笔按键模式 (见 PenButtonModeWire)
8       1     penButtonRoute      uint8_t   笔按键路由 (见 PenButtonRouteWire)
```

**definedFields** 位掩码:

| Bit | 字段 | 说明 |
|-----|------|------|
| 0 | Mode | 模式字段有效 |
| 1 | AutoMode | 自动模式字段有效 |
| 2 | StylusVhfEnabled | Stylus VHF 字段有效 |
| 3 | PenButtonMode | 笔按键模式字段有效 |
| 4 | PenButtonRoute | 笔按键路由字段有效 |

**ServiceModeWire**:

| 值 | 含义 |
|----|------|
| 0 | Full (完整模式: Touch + Stylus) |
| 1 | TouchOnly (仅 Touch) |

**PenButtonModeWire**:

| 值 | 含义 |
|----|------|
| 0 | OemCustom (OEM 自定义按键码) |
| 1 | NativeBarrel (原生笔杆按键) |
| 2 | NativeEraser (原生橡皮擦) |

**PenButtonRouteWire**:

| 值 | 含义 |
|----|------|
| 0 | VhfOnly (仅 VHF 注入) |
| 1 | Win32Only (仅 Win32 虚拟笔 API) |
| 2 | VhfAndWin32 (双路由, 诊断用) |

> :bulb: `desiredMode` 与 `activeMode` 的区别: 当调用 `ApplyConfigPatch` 设置 mode=Full 时, `desiredMode=Full`。但由于 mode 变更需要重启, 当前运行时可能仍是 TouchOnly, 此时 `activeMode=TouchOnly`。

---

#### ApplyConfigPatch (0x2B)

应用增量配置变更 (不需要重启的部分即时生效, 需要重启的字段标记为 restartRequired)。

| 方向 | 字段 | 值 |
|------|------|-----|
| Req | command | `0x2B` |
| Req | param | `ApplyConfigPatchRequestWire` (8 bytes) |
| Req | paramLen | `sizeof(ApplyConfigPatchRequestWire)` = 8 |
| Res | success | `true` / `false` |
| Res | data | `ConfigMutationResultWire` (6 bytes) |
| Res | dataLen | `sizeof(ConfigMutationResultWire)` = 6 |
| Res | status (失败) | `InvalidRequest` / `InvalidState` |

**ApplyConfigPatchRequestWire** 布局 (8 bytes):

```
Offset  Size  Field               Type      说明
------  ----  -----               ----      ----
0       2     wireVersion         uint16_t  协议版本
2       1     fieldMask           uint8_t   要变更的字段位掩码
3       1     desiredMode         uint8_t   期望模式
4       1     autoMode            uint8_t   自动模式 (0/1)
5       1     stylusVhfEnabled    uint8_t   Stylus VHF (0/1)
6       1     penButtonMode       uint8_t   笔按键模式
7       1     penButtonRoute      uint8_t   笔按键路由
```

> `fieldMask` 指定哪些字段参与变更。仅 `fieldMask` 中置位的字段被处理, 其余字段保留当前值。

**ConfigMutationResultWire** 布局 (6 bytes):

```
Offset  Size  Field                Type      说明
------  ----  -----                ----      ----
0       2     wireVersion          uint16_t  协议版本
2       1     changedFields        uint8_t   实际发生变更的字段位掩码
3       1     appliedFields        uint8_t   即时生效的字段位掩码
4       1     restartRequiredFields uint8_t  需要重启才能生效的字段位掩码
5       1     _reserved0           uint8_t   保留
```

> :warning: `restartRequiredFields` 非零表示某些变更需要在**下次 Service 重启**时才会生效。当前 mode 变更属于此类。

---

#### PersistConfig (0x2C)

将当前配置持久化到 `config.ini` 文件。

| 方向 | 字段 | 值 |
|------|------|-----|
| Req | command | `0x2C` |
| Req | paramLen | `0` |
| Res | success | `true` / `false` |
| Res | data | `PersistConfigResponseWire` (4 bytes) |
| Res | dataLen | `sizeof(PersistConfigResponseWire)` = 4 |
| Res | status (失败) | `InvalidState` / `InternalError` |

**PersistConfigResponseWire** 布局 (4 bytes):

```
Offset  Size  Field            Type      说明
------  ----  -----            ----      ----
0       2     wireVersion      uint16_t  协议版本
2       1     persistedFields  uint8_t   已持久化的字段位掩码
3       1     _reserved0       uint8_t   保留
```

> 持久化行为: 原子写入 (写入 `.tmp` → `MoveFileExW`), 文件备份 (`.bak`)。

---

#### ReloadConfig (0x28)

从 `config.ini` 重新加载 Pipeline 配置, 并重新解析 Service 配置以检测变更。

| 方向 | 字段 | 值 |
|------|------|-----|
| Req | command | `0x28` |
| Req | paramLen | `0` |
| Res | success | `true` / `false` |
| Res | data | `ReloadConfigSummaryWire` (3 bytes) |
| Res | dataLen | `sizeof(ReloadConfigSummaryWire)` = 3 |
| Res | status (失败) | `NotFound` (config.ini 不存在) |

**ReloadConfigSummaryWire** 布局 (3 bytes):

```
Offset  Size  Field                 Type
------  ----  -----                 ----
0       1     changedFields         uint8_t  变更字段位掩码
1       1     appliedFields         uint8_t  已应用字段位掩码
2       1     restartRequiredFields uint8_t  需重启字段位掩码
```

---

#### SaveConfig (0x29)

`PersistConfig` 的旧别名, 行为完全一致。

---

### 3.6 日志

#### GetLogs (0x32)

拉取 Service 端最近累积的日志行。

| 方向 | 字段 | 值 |
|------|------|-----|
| Req | command | `0x32` |
| Req | paramLen | `0` |
| Res | success | `true` |
| Res | data | UTF-8 文本行 (换行符分隔) |
| Res | dataLen | 实际日志字节数 (最大 4096) |

> 每次调用消费并清空缓冲区 (DrainNewLines 语义)。

---

### 3.7 诊断命令

#### GetDebugSchema (0x3D)

分页获取调试字段元数据 (字段名/类型/UI 分组等)。

| 方向 | 字段 | 值 |
|------|------|-----|
| Req | command | `0x3D` |
| Req | param | `DebugSchemaRequest` (4 bytes) |
| Req | paramLen | `sizeof(DebugSchemaRequest)` = 4 |
| Res | success | `true` |
| Res | data | `DebugSchemaResponseHeader` + N×`DebugFieldSchemaWire` |
| Res | dataLen | 实际数据长度 |

**DebugSchemaRequest** 布局 (4 bytes):

```
Offset  Size  Field    Type      说明
------  ----  -----    ----      ----
0       2     offset   uint16_t  起始索引
2       2     limit    uint16_t  最大返回数 (0 = 自动)
```

**DebugSchemaResponseHeader** 布局 (12 bytes):

```
Offset  Size  Field          Type      说明
------  ----  -----          ----      ----
0       2     schemaVersion  uint16_t  Schema 版本 (变更时递增)
2       2     totalFields    uint16_t  总字段数
4       2     returnedFields uint16_t  本次返回字段数
6       2     recordSize     uint16_t  单条记录大小 (96)
8       4     schemaHash     uint32_t  描述内容 hash
```

**DebugFieldSchemaWire** 布局 (96 bytes, 每条记录):

```
Offset  Size  Field          Type       说明
------  ----  -----          ----       ----
0       2     fieldId        uint16_t   字段 ID
2       1     valueType      uint8_t    值类型 (见 DebugValueType)
3       1     sourceKind     uint8_t    数据来源 (见 DebugSourceKind)
4       2     sourceIndex    int16_t    来源索引
6       1     uiOrder        uint8_t    UI 显示顺序
7       1     dvrTarget      uint8_t    DVR 目标
8       1     dvrPositionMode uint8_t   DVR 定位模式
9       1     _reserved0     uint8_t    保留
10      2     dvrIndex       int16_t    DVR 索引
12      2     _reserved1     int16_t    保留
14      32    key[32]        char[]     键名
46      48    displayName[48] char[]    显示名称
94      16    unit[16]       char[]     单位
110     24    uiGroup[24]    char[]     UI 分组
134     32    dvrColumnName[32] char[]  DVR 列名
166     32    dvrAnchor[32]  char[]     DVR 锚点
```

**DebugValueType**:

| 值 | 类型 |
|----|------|
| 0 | UInt32 |
| 1 | Int32 |
| 2 | Float32 |
| 3 | Bool |

**DebugSourceKind**:

| 值 | 说明 |
|----|------|
| 0 | MasterSuffixWord — Master 硬件后缀字 |
| 1 | SlaveSuffixWord — Slave 硬件后缀字 |
| 2 | StylusField — Stylus Pipeline 字段 |
| 3 | PenBridgeField — BT MCU 笔桥字段 |
| 4 | DerivedField — 派生计算字段 |

---

#### GetDebugSnapshot (0x3E)

获取一帧调试数据的快照值。

| 方向 | 字段 | 值 |
|------|------|-----|
| Req | command | `0x3E` |
| Req | paramLen | `0` |
| Res | success | `true` |
| Res | data | `DebugSnapshotHeader` + N×`DebugSnapshotValueWire` |
| Res | dataLen | 实际数据长度 |

**DebugSnapshotHeader** 布局 (8 bytes):

```
Offset  Size  Field          Type      说明
------  ----  -----          ----      ----
0       2     schemaVersion  uint16_t  Schema 版本 (必须与 GetDebugSchema 一致)
2       2     fieldCount     uint16_t  字段数
4       2     recordSize     uint16_t  单条记录大小 (16)
6       2     _reserved0     uint16_t  保留
```

**DebugSnapshotValueWire** 布局 (16 bytes, 每条记录):

```
Offset  Size  Field       Type      说明
------  ----  -----       ----      ----
0       2     fieldId     uint16_t  字段 ID (对应 schema)
2       1     valueType   uint8_t   值类型
3       1     flags       uint8_t   bit0: 值是否有效
4       4     _reserved0  uint32_t  保留
8       8     rawValue    uint64_t  原始值 (union 存储)
```

---

### 3.8 Pen Bridge 命令

#### GetPenBridgeStatus (0x3C)

查询 BT MCU 笔桥和压力传感器状态。

| 方向 | 字段 | 值 |
|------|------|-----|
| Req | command | `0x3C` |
| Req | paramLen | `0` |
| Res | success | `true` |
| Res | data | 24 bytes 二进制布局 |
| Res | dataLen | `24` |

**PenBridgeStatus** 布局 (24 bytes):

```
Offset  Size  Field         Type      说明
------  ----  -----         ----      ----
0       1     evtRunning    uint8_t   事件通道运行中 (0/1)
1       1     pressRunning  uint8_t   压力通道运行中 (0/1)
2       1     reportType    uint8_t   报告类型
3       1     freq1         uint8_t   频率通道 1
4       1     freq2         uint8_t   频率通道 2
5       8     press[4]      uint16_t[4] 缩放后压力值 ×4
13      1     pressureMode  uint8_t   压力范围模式 (0=4096, 1=16382/4)
14      2     pressureMax   uint16_t  最大压力值
16      8     rawPress[4]   uint16_t[4] 原始压力值 ×4
```

---

#### GetPenIdentityStatus (0x41)

查询当前手写笔身份状态。该命令不改变 `GetPenBridgeStatus` 的 24 bytes 固定布局。

| 方向 | 字段 | 值 |
|------|------|-----|
| Req | command | `0x41` |
| Req | paramLen | `0` |
| Res | success | `true` |
| Res | data | `PenIdentityStatusWire` |
| Res | dataLen | `140` |
| Res | status (失败) | `InvalidState` (DeviceRuntime 未就绪) |

**PenIdentityStatusWire** 布局 (140 bytes):

```
Offset  Size  Field                   Type       说明
------  ----  -----                   ----       ----
0       2     wireVersion             uint16_t   IPC 协议版本
2       1     flags                   uint8_t    bit0=stylusId 有效, bit1=modelId 有效, bit2=hardwareVersion 有效, bit3=connected
3       1     stylusId                uint8_t    PenTypeInfo payload[0]
4       4     penModuleModelId        uint32_t   PenModule 小端 modelId
8       2     hardwareVersionUtf8Len  uint16_t   UTF-8 字节长度, 不含 NUL
10      2     _reserved0              uint16_t   保留
12      128   hardwareVersionUtf8     char[128]  UTF-8 validated 字符串缓冲, 额外 NUL 仅供 C string 便利
```

字符串约束：`hardwareVersionUtf8Len` 是字节数，不是字符数；服务端按 UTF-8 边界截断，不会截断到多字节字符中间。

---

#### SetPenPressureMode (0x3F)

设置笔压力读取范围模式。

| 方向 | 字段 | 值 |
|------|------|-----|
| Req | command | `0x3F` |
| Req | param[0] | `0` = Raw12Bit4096, `1` = Raw14Bit16382 |
| Req | paramLen | `1` |
| Res | success | `true` / `false` |
| Res | status (失败) | `InvalidState` (PenPressureReader 未就绪) |

---

#### SetMasterParserOnly (0x40)

启用/禁用仅 Master Parser 模式 (调试用, 关闭下游 Pipeline 模块)。

| 方向 | 字段 | 值 |
|------|------|-----|
| Req | command | `0x40` |
| Req | param[0] | `0` = 正常, `1` = 仅 Master Parser |
| Req | paramLen | `1` |
| Res | success | `true` / `false` |
| Res | status (失败) | `InvalidState` (DeviceRuntime 未就绪) |

---

### 3.9 状态码参考

| 值 | 枚举 | 含义 |
|----|------|------|
| 0 | `Ok` | 成功 |
| 1 | `UnsupportedCommand` | 不支持的命令 |
| 2 | `InvalidRequest` | 请求参数无效 |
| 3 | `InvalidState` | 当前状态不允许此操作 (如 Runtime 未启动) |
| 4 | `NotFound` | 请求的资源不存在 |
| 5 | `PermissionDenied` | 权限不足 |
| 6 | `InternalError` | 内部错误 |

---

## 4. 共享内存 (实时帧推送)

### 4.1 SharedFrameBuffer

| 属性 | 值 |
|------|-----|
| 名称 | `Global\EGoTouchSharedFrame` |
| 布局 | `SharedTripleBuffer` (三缓冲) |
| ABI 版本 | 5 |
| 帧就绪事件 | `Global\EGoTouchFrameReady` (Windows Event) |
| 热图尺寸 | 40×60 (rows × cols) |
| 最大触控点数 | 10 |
| 最大 Peak 数 | 30 |

### 4.2 三缓冲读取协议

```
Writer (Service):                    Reader (App):
  1. 选取下一个 slot                  1. 读取 readyIdx
  2. slotSequence = odd (dirty)      2. 读取 slotFrameIds[readyIdx]
  3. 写入 SharedFrameData            3. 读取 slotSequences[readyIdx] → S1
  4. slotFrameIds[slot] = frameId    4. 拷贝 SharedFrameData
  5. slotSequences[slot] = even      5. 读取 slotSequences[readyIdx] → S2
  6. readyIdx = slot                 6. 读取 slotFrameIds[readyIdx]
  7. frameId++                       7. 若 S1 == S2 (均为偶数) 且
  8. SetEvent(frameReady)               slotFrameIds 与步骤2一致
                                      → 数据有效，使用拷贝
                                     否则 → 丢弃，等待下一次
```

### 4.3 SharedFrameData 关键字段

| 分类 | 字段 | 类型 | 说明 |
|------|------|------|------|
| **运行时状态** | workerState | int8_t | 工作线程状态 |
| | streaming | bool | 是否正在推送帧 |
| | lastFrameProcessUs | int64_t | 上帧处理耗时 (μs) |
| | avgFrameProcessUs | int64_t | 平均帧处理耗时 (μs) |
| | acquisitionFps | int32_t | Master FPS |
| | slaveAcquisitionFps | int32_t | Slave FPS |
| | vhfEnabled | bool | VHF 设备是否启用 |
| | vhfDeviceOpen | bool | VHF 设备是否打开 |
| **触控** | heatmapMatrix[40][60] | int16_t | 热图数据矩阵 |
| | contactCount | uint8_t | 触控点数 |
| | contacts[10] | SharedContact | 触控点详情 (id/x/y/state/area/sizeMm 等) |
| | peaks[30] | SharedPeak | 峰值点列表 |
| | touchPackets[2] | SharedTouchPacket | HID 触控报文 |
| **笔 (Stylus)** | stylusPoint | SharedStylusSolvePoint | 笔求解坐标 (x/y/pressure/tilt/confidence) |
| | stylusPacket | SharedStylusPacket | 笔 HID 报文 |
| | stylusStatus | uint32_t | 笔状态寄存器 |
| | stylusPressure | uint16_t | 笔压力值 |
| | stylusRawGrid | SharedStylusRawGrid | 笔原始网格 (TX1/TX2) |
| | diag | SharedStylusDiagnostics | 笔诊断详情 (100+ 字段) |
| **诊断** | masterSuffix | MasterSuffixView | Master 硬件后缀视图 |
| | slaveSuffix | SlaveSuffixView | Slave 硬件后缀视图 |

### 4.4 辅助事件

| 事件名 | 用途 |
|--------|------|
| `Global\EGoTouchLogReady` | Service 通知 App 有新日志 |
| `Global\EGoTouchPenStatusReady` | Service 通知 App 笔状态更新 |

---

## 5. ConfigDirtyFlag (共享内存)

| 属性 | 值 |
|------|-----|
| 名称 | `Global\EGoTouchConfigDirty` |
| 大小 | 4 bytes (`std::atomic<uint32_t>`) |
| 安全 | Admin-only |

**协议**:

```
App (Writer):                       Service (Reader):
  1. 写入 config.ini                每帧调用 CheckAndClear()
  2. m_flag->store(1, release)      m_flag->exchange(0, acq_rel)
                                    若返回 1 → 触发 ReloadConfig
```

> 仅支持 "脏/不脏" 布尔信号。重构后将由 IPC `ApplyConfigPatch` 替代此机制。

---

## 6. 典型交互序列

### 6.1 App 启动 → 连接 Service

```
App                          Service
 │                              │
 │── Connect(5s timeout) ──────→│
 │←── Pipe connected ──────────│
 │                              │
 │── Ping ─────────────────────→│
 │←── success=true ────────────│
 │                              │
 │── GetConfigSnapshot ────────→│
 │←── ConfigSnapshotWire ──────│
 │                              │
 │── EnterDebugMode ───────────→│
 │←── success=true ────────────│
 │                              │
 │  ◄══ SharedFrame 推送开始 ══│
```

### 6.2 用户修改配置

```
App                          Service
 │                              │
 │── ApplyConfigPatch ─────────→│
 │   fieldMask=PenButtonMode    │
 │   penButtonMode=NativeBarrel │
 │←── ConfigMutationResult ────│
 │   changed=PenButtonMode      │
 │   applied=PenButtonMode      │
 │                              │
 │── PersistConfig ────────────→│
 │←── PersistConfigResponse ───│
 │   persistedFields=PenBtnMode │
```

### 6.3 拉取诊断快照

```
App                          Service
 │                              │
 │── GetDebugSchema ───────────→│
 │   offset=0, limit=0          │
 │←── SchemaResponse ──────────│
 │   totalFields=42             │
 │   schemaVersion=5            │
 │   42×DebugFieldSchemaWire    │
 │                              │
 │── GetDebugSnapshot ─────────→│
 │←── SnapshotHeader ──────────│
 │   42×SnapshotValueWire       │
 │                              │
 │── GetPenBridgeStatus ───────→│
 │←── 24 bytes PenBridgeStatus  │
 │                              │
 │── GetLogs ──────────────────→│
 │←── "2026-06-05 10:30:01..."  │
```

---

## 7. 重构后协议 (目标态)

重构完成后, 以下变更生效:

### 7.1 移除的命令

| 命令 | 替代 |
|------|------|
| `SaveConfig` (0x29) | `PersistConfig` (0x2C) |
| `ReloadConfig` (0x28) | 重新加载 YAML 的新实现 |

### 7.2 新增的 TLV 格式

固定布局 `ConfigSnapshotWire` / `ApplyConfigPatchRequestWire` 将替换为:

```
ConfigSnapshot TLV:
  [uint16_t] version
  [uint16_t] entryCount
  重复 N 次:
    [uint16_t] keyId       (ConfigKeyId enum)
    [uint8_t]  valueType   (0=bool, 1=int32, 2=float32, 3=enum8, 4=string)
    [uint16_t] valueLen
    [N bytes]  valuePayload

ConfigPatch TLV:
  同 Snapshot, 但仅包含变更的键
```

### 7.3 ConfigKeyId 分区

| 范围 | 子系统 |
|------|--------|
| 0x0000-0x00FF | Service 配置 |
| 0x0100-0x01FF | Touch Pipeline |
| 0x0200-0x02FF | Stylus Pipeline |
| 0x0300-0x03FF | DVR Runtime |

未知 keyId → 安全跳过 (向前兼容)。

---

## 8. 错误恢复

| 场景 | 行为 |
|------|------|
| Pipe 断开 | App 侧检测 `IsConnected()==false`, UI 显示断连状态 |
| Service 重启 | App 重新 Connect + Ping, 拉取最新 ConfigSnapshot |
| 请求超时 | `IpcPipeClient::Send()` 在 Pipe 层阻塞; 无应用层超时 |
| 并发请求 | `IpcPipeClient` 持有 `std::mutex`, 串行化所有 Send 调用 |
| 共享内存写入冲突 | Triple-buffer seqlock 保证读者不会读到半写数据 |
