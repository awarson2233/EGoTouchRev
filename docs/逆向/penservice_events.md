# PenService.dll 事件码表 (HID col00 完整映射)

基于 Ghidra 对 `PenService.dll` 中 `GetInterruptPipeMsg` 及分发表 (`180014130`) 的完整逆向，共识别出 21 个事件码。这些事件主要由 MCU 发送，用于触发操作系统层面的 UI（如配对、电量悬浮窗）或业务逻辑。

| 事件码(Hex) | 偏移 | 底层处理函数 | 回调注册函数 (`RegisterCallback...`) | 含义 / 触发功能 |
|------------|-----|------------|-----------------------------------|---------------|
| `0x00` | 0x120 | FUN_18000a220 | `UpdatePenModule` | 笔模块信息更新 |
| `0x01` | 0x128 | FUN_18000a280 | `UpdatePenSerialNo` | 获取/上报笔序列号 (含日志 "HandlePenSerialNumber") |
| `0x02` | 0x130 | FUN_18000a460 | `UpdatePenHardwareVersion` | 获取/上报硬件版本 (含日志 "HandlePenHardwareVersion") |
| `0x03` | 0x138 | FUN_18000a640 | `UpdatePenFirmwareVersion` | 获取/上报固件版本 (含日志 "HandlePenFirmwareVersion") |
| `0x08` | 0x108 | FUN_18000a820 | `UpdateBatteryVolume` | 笔电量百分比更新 |
| `0x09` | 0x118 | FUN_18000a900 | `UpdatePenChargingStatus` | 充电状态更新 (输出 "Charging Status: ") |
| `0x12` | 0x110 | FUN_18000a890 | `UpdatePenConnectStatus` | 笔连接状态（会额外开启防抖动相关检测线程） |
| **`0x15`** | **0x140** | **FUN_18000aae0** | **`NewPenConnectRequest`** | **发起新笔配对请求UI (收到后会回发 0x2E01 ACK)** |
| `0x16` | 0x148 | FUN_18000ab60 | `NewPenConnectResult` | 新笔配对结果通知 |
| `0x24` | 0x150 | FUN_18000aba0 | `UpdatePenKeySupport` | 查询笔按键功能支持 |
| `0x25` | 0x158 | FUN_18000ac20 | `UpdatePenKeyFuncSet` | 笔按键功能设置 |
| `0x26` | 0x160 | FUN_18000ac60 | `UpdatePenKeyFuncGet` | 笔按键功能读取 |
| `0x27` | 0x168 | FUN_18000aca0 | `TransferPenMode` (推测) | 笔模式切换相关 |
| **`0x28`** | **0x170** | **FUN_18000ace0** | **`PenTopBatteryWindow`** | **呼出顶部电量悬浮窗 (纯通知)** |
| **`0x29`** | **0x178** | **FUN_18000ad10** | **`PenCloseConnectWindow`** | **关闭配对/连接窗口提示 (纯通知)** |
| `0x2A` | 0x180 | FUN_18000ad40 | `PenDeviationReminder` | 笔尖偏差校准提醒 |
| `0x2C` | 0x188 | FUN_18000ad70 | `PenFirstBatAfterConn` | 连接后首次电量播报 |
| `0x2D` | 0x190 | FUN_18000abe0 | `PenOskPrevensionMode` | 软键盘防误触模式开关通知 |
| `0x2F` | 0x1B0 | FUN_18000ae20 | `PenCurrentFunc` | 笔当前激活功能反馈 |
| `0x67` | 0x198 | FUN_18000ada0 | `PenUpdateProgress` (推测) | OTA 固件更新进度 |
| `0x68` | 0x1A0 | FUN_18000ade0 | `SysLang` | 系统语言环境请求/反馈 |

### 逆向发现汇总
通过遍历比对 `PenService.dll` 内部映射表（共 21 项），我们发现不仅 `0x15`, `0x28`, `0x29` 在此列，包括常用的电量(`0x08`)、固件版本(`0x03`)、充电状态(`0x09`) 等管理型指令全部由该组件接管。

> **注意：** `THP_Service.dll` 控制的 `0x71`~`0x7F` 更侧重于数据/书写状态及频率匹配逻辑；而 `PenService.dll` 则更像是一个专职的“辅助/UI交互后台”，这些 `0x00`-`0x68` 事件码属于外围生态范畴。
