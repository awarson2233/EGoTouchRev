# BTMCU USB 协议表

基于以下来源整理：
- `THP_Service.dll` 的 Ghidra MCP 逆向结果
- `EGoTouchService/Device/penevt/PenEventBridge.cpp`
- 当前桥接层已落地的 exact factory capture

> 置信度标记
> - **Confirmed**: 已被反汇编/反编译或当前代码中的 exact capture 明确证实
> - **Likely**: 高概率成立，但还缺少抓包或更深层调用链确认
> - **Open**: 仍待确认

## 1. 报文骨架

### 1.1 Host → MCU 发送帧（Confirmed）
所有已确认的主机发包最终都走 `BtPen_SendPacket @ 0x18000f500`，其行为是：
- 拷贝 8 字节 header
- 强制 `byte[6] = 0x11`
- 从 `byte[8]` 开始追加 payload
- 调用 `WriteFile(handle, buf, payload_len + 8, ...)`

已确认的发送帧布局：

| 偏移 | 含义 | 备注 |
|---|---|---|
| `byte[0]` | `0x07` | 固定 |
| `byte[1]` | 子类型/标志 | 见命令表，已观测到 `0x00 / 0x01` |
| `byte[2]` | `0x02` | 固定 |
| `byte[3]` | `0x00` | 目前观测固定为 0 |
| `byte[4]` | 命令低字节 | 已观测均为 `0x01` |
| `byte[5]` | 命令高字节 | 如 `0x71 / 0x77 / 0x7D / 0x7E / 0x80` |
| `byte[6]` | `0x11` | `BtPen_SendPacket` 强制覆盖 |
| `byte[7]` | 类型字节 | 已观测到 `0x00 / 0x01 / 0x20` |
| `byte[8..]` | payload | 可空 |

### 1.2 MCU → Host 接收帧（部分 Confirmed）
`AsynchReadThreadProc @ 0x18000e1d0` 持续 `ReadFile(..., 0x40)`，并把 64 字节包推进环形队列。

在 `USB_AsynchProcThreadProc @ 0x18000cf40` 中，已确认：
- 事件码来自 `packet[5]`
- 多数事件的首个有效载荷字节来自 `packet[8]`
- 接收线程会校验 `packet[4] == 0x01`

其余接收头字段语义仍待抓包与更深层验证。

---

## 2. Host → MCU 命令表

| 命令 | 方向 | 头部示例 | Payload | 触发点 | 结论 |
|---|---|---|---|---|---|
| `0x7101` | Host → MCU | `07 00 02 00 01 71 11 00` | 无 | `BtPen_CheckPenStatus @ 0x18000e830` | **Confirmed** |
| `0x7701` | Host → MCU | `07 00 02 00 01 77 11 00` | 无 | `BtPen_CheckMcuStatus @ 0x18000e8c0` | **Confirmed** |
| `0x8001` | Host → MCU | `07 01 02 00 01 80 11 20` | 1 字节 ACK code | `BtPen_SendEventAck @ 0x180011d20` | **Confirmed** |
| `0x7E01` | Host → MCU | `07 01 02 00 01 7E 11 01` | 1 字节 | `BtPen_GetReportInfo @ 0x18000e960`，type=4 | **Confirmed** |
| `0x7D01` | Host → MCU | `07 01 02 00 01 7D 11 20` | 32 字节 | `BtPen_HandleInitParamEvent @ 0x18000f660`，type=2 | **Confirmed** |

### 2.1 `0x8001` ACK 包（Confirmed）
`BtPen_SendEventAck` 的 payload 只有 1 字节，即 ACK code。

示例：
```text
07 01 02 00 01 80 11 20 <ack_code>
```

### 2.2 `0x7E01` Match-Info 响应（Confirmed）
`BtPen_GetReportInfo(type=4)` 会发送 1 字节 payload，来源于 `param_1[1]`。

示例：
```text
07 01 02 00 01 7E 11 01 <value>
```

### 2.3 `0x7D01` Init-Param 响应（Confirmed）
`BtPen_HandleInitParamEvent` 会把一段 32 字节 ASCII/CSV 参数串转换成 32 字节二进制，然后发出：

```text
07 01 02 00 01 7D 11 20 <32-byte payload>
```

---

## 3. MCU → Host 事件表

| 事件码 (`packet[5]`) | 名称/日志 | Host 处理 | ACK code | 结论 |
|---|---|---|---:|---|
| `0x03` | `USBD_SW_VERSION` | 记录日志 | 无 | **Confirmed** |
| `0x08` | `BATTERY_STATUS` | 记录日志 | 无 | **Confirmed** |
| `0x09` | `CHARGING_STATUS` | 记录日志 | 无 | **Confirmed** |
| `0x10` | `DEV_CONNECT` | 记录日志 | 无 | **Confirmed** |
| `0x12` | `DEV_PAIR_STATUS` | 更新状态 | 无 | **Confirmed** |
| `0x21` | `PEN_DOCK_STATUS` | 记录日志 | 无 | **Confirmed** |
| `0x23` | `PEN_UPDATE_STATUS` | 记录日志 | 无 | **Confirmed** |
| `0x27` | `PEN_KEY_FUNC_GET` | 记录日志 | 无 | **Confirmed** |
| `0x2C` | `PEN_BATTERY_AFTER_CONN` | 记录日志 | 无 | **Confirmed** |
| `0x2E` | `PEN_PAIR_DETECT_ACK` | 记录日志 | 无 | **Confirmed** |
| `0x2F` | `PEN_CURRENT_FUNC` | 更新当前功能 | `0x0B` | **Confirmed** |
| `0x70` | `PEN_AC_STATUS` | 更新状态位 | `0x00` | **Confirmed** |
| `0x71` | `PEN_CONN_STATUS` | 更新连接状态 | `0x01` | **Confirmed** |
| `0x72` | `PEN_CUR_STATUS` | 更新当前模式 | `0x02` | **Confirmed** |
| `0x73` | `PEN_TYPE_INFO` | 更新笔类型/ID | `0x0D` | **Confirmed** |
| `0x74` | `PEN_ROATE_ANGLE` | 更新旋转角 | `0x03` | **Confirmed** |
| `0x75` | `PEN_TOUCH_MODE` | 更新 touch mode | `0x04` | **Confirmed** |
| `0x76` | `PEN_GLOBAL_PREVENT_MODE` | 更新 prevent mode | `0x05` | **Confirmed** |
| `0x77` | 未命名事件 | 仅回 ACK | `0x06` | **Confirmed** |
| `0x78` | `PEN_HOLSTER` | 更新 holster | `0x07` | **Confirmed** |
| `0x79` | `PEN_FREQ_JUMP` | 记录/通知 | `0x08` | **Confirmed** |
| `0x7B` | `PEN_REP_PARAM` | 仅记录/ACK | `0x0A` | **Confirmed** |
| `0x7C` | `PEN_GLOBAL_ANNOTATION` | 更新注解相关状态 | `0x0C` | **Confirmed** |
| `0x7F` | `ERASER_TOGGLE` | 更新 eraser toggle | `0x09` | **Confirmed** |

---

## 4. ACK 对照表

| 事件码 | ACK code |
|---|---:|
| `0x70` | `0x00` |
| `0x71` | `0x01` |
| `0x72` | `0x02` |
| `0x74` | `0x03` |
| `0x75` | `0x04` |
| `0x76` | `0x05` |
| `0x77` | `0x06` |
| `0x78` | `0x07` |
| `0x79` | `0x08` |
| `0x7F` | `0x09` |
| `0x7B` | `0x0A` |
| `0x2F` | `0x0B` |
| `0x7C` | `0x0C` |
| `0x73` | `0x0D` |

---

## 5. 0x7B / 0x7D01 专题

### 5.1 `0x7B` 本身（Confirmed）
在 `USB_AsynchProcThreadProc` 的 `case 0x7B` 中，原厂行为只有两步：
1. 记录日志 `"[USB]PEN_REP_PARAM"`
2. 发送 `BtPen_SendEventAck(0x0A)`

**原厂并不会在这个 switch case 里直接发送 `0x7D01`。**

### 5.2 `0x7D01` 的真实触发路径（Confirmed）
`0x7D01` 来自另一条路径：

```text
ServiceInterface[+0xA8]
  -> BtPen_GetReportInfo(type=2)
    -> BtPen_HandleInitParamEvent(...)
      -> BtPen_SendPacket(cmd=0x7D01, payload_len=0x20)
```

也就是说：
- `0x7B` 是 MCU 上报事件
- `0x7D01` 是 Host 侧稍后通过另一路 service callback 主动发回的初始化参数
- 两者在时序上相关，但**不是同一个 switch 分支里的直接同步响应**

### 5.3 当前项目中唯一已确认的 exact payload（Confirmed）
当前代码在 `PenEventBridge.cpp:376-398` 固定发送了一份 exact factory capture：

```text
Header:
07 01 02 00 01 7D 11 20

Payload:
33 33 33 33 E7 02 12 04 58 02 8D 20 0F 01 02 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
```

这份 payload 是目前最可靠的 `0x7D01` 基线。

### 5.4 用 Ghidra 中的 type-3 编码规则反推 CSV token（Confirmed）
`BtPen_HandleInitParamEvent` 的 token 编码规则是：

| token 长度 | 输出字节 |
|---|---|
| `1` | `[hex(d0)]` |
| `2` | `[hex(d1), hex(d0)]` |
| `3` | `[hex(d1d2), hex(d0)]` |
| `4` | `[hex(d2d3), hex(d0d1)]` |

按该规则反推，`PenEventBridge.cpp:385-393` 的 exact payload 对应 token 序列应为：

```text
3333,3333,2e7,412,258,208d,10f,2,
```

这与 `PenEventBridge.cpp` 里旧注释中的：

```text
3333,3333,2e7,412,258,411a,0f,1,
```

**并不一致**。因此在重新抓到更强证据前，应优先相信 exact payload，而不是旧注释字符串。

---

## 6. 当前项目代码对应点

| 位置 | 说明 |
|---|---|
| `PenEventBridge.cpp:171-179` | 当前桥接层的 ACK 映射表 |
| `PenEventBridge.cpp:187-197` | 当前桥接层 `0x8001` ACK 发送 |
| `PenEventBridge.cpp:231-243` | 当前桥接层把首次 `0x7B` 作为触发点，发送 `0x7D01` |
| `PenEventBridge.cpp:300-367` | 当前握手序列：`0x7101 -> 0x7701 -> 0x7701` |
| `PenEventBridge.cpp:376-398` | 当前唯一已确认的 `0x7D01` exact factory payload |

---

## 7. 仍待深挖的问题

1. 接收帧 `byte[0..3] / byte[6] / byte[7]` 的精确定义仍未最终命名。
2. `0x77` 事件目前只看到 ACK，没有看见业务语义。
3. 需要更多抓包来确认：`0x7B` 的 event payload 本身是否携带可用于选择 `0x7D01` 模板的信息。
4. `PenEventBridge.cpp:171` 中的 `0x6F -> 0x0B` 不是原厂 Ghidra 事件表中的确认项；原厂确认的是 `0x2F -> 0x0B`。
