# EGoTouchService 跨团队接口协定

> 生成日期：2026-04-19  
> 依据：`docs/EGoTouchService_模块层级-优先度审查清单.md` 与当前代码实现  
> 目标：在多团队并发开发前，先冻结模块边界、输入输出、线程/生命周期约束与变更规则，降低交叉返工

---

## 1. 适用范围

本文覆盖以下团队边界：

1. **Device Runtime 团队**：`EGoTouchService/Device/runtime`
2. **Himax Core 团队**：`EGoTouchService/Device/himax`
3. **BT Pen I/O 团队**：`EGoTouchService/Device/penevt`、`EGoTouchService/Device/btmcu`、`EGoTouchService/Device/penpress`
4. **VHF 输出团队**：`EGoTouchService/Device/vhf`
5. **Device API / 公共边界团队**：`EGoTouchService/Device` 公共头
6. **Host 事件团队**：`EGoTouchService/Host`
7. **Service 壳层团队**：`EGoTouchService/include/ServiceShell.h`、`EGoTouchService/source/ServiceShell.cpp`、`EGoTouchService/source/ServiceEntry.cpp`
8. **Service 编排 / IPC 团队**：`EGoTouchService/include/ServiceHost.h`、`EGoTouchService/source/ServiceHost.cpp`
9. **App / IPC 消费方**：`Tools/EGoTouchApp` + `Common/include/IpcProtocol.h` + `Common/include/SharedFrameBuffer.h`

---

## 重构状态快照（2026-04）

> 本节用于给后续团队快速判断“哪些已经落地、哪些仍在收尾”。

### 0.1 已完成波次（以当前代码为准）

以下波次已完成并进入稳定协作阶段：

1. Host event contract + callback safety
2. Himax runtime facade + Himax split first pass
3. Pen semantic contract + Runtime pen state first pass
4. Service config/IPC semantics
5. Debug schema single-source + version/hash policy
6. ServiceHost decomposition first pass
7. VHF split first pass
8. Header-boundary cleanup first pass

### 0.2 当前已生效的强制契约

1. **系统事件契约**：跨模块仅通过 `SystemStateEvent` 语义对象协作；`raw_*` 字段仅用于诊断。
2. **回调安全契约**：Host/Pen 回调按“工作线程同步回调、短耗时、不可阻塞”执行。
3. **Runtime-Himax 边界契约**：Runtime 通过 facade 访问 Himax 能力，不再以内部成员布局作为稳定接口。
4. **配置与 IPC 契约**：`IpcProtocol.h` 与 `SharedFrameBuffer.h` 是唯一真源；配置项需声明生效级别（即时/重启）。
5. **Debug schema 契约**：schema/snapshot 由单一字段定义驱动；`schemaVersion + schemaHash` 共同标识兼容性。
6. **头文件边界契约**：公共头优先暴露稳定语义类型，平台/实现细节继续向 `.cpp` 下沉。

### 0.3 仍需收尾的工作（非推测）

1. **first pass 后续收口**：Himax、ServiceHost、VHF、Header-boundary 四条线仍有 second pass 收口空间。
2. **配置热更新语义闭环**：`ReloadConfig` 在 `mode/auto_mode` 上仍需与运行态效果严格对齐。
3. **IPC 幂等/失败语义补齐**：`StartRuntime` 等命令的重复调用语义需在协议注释与实现行为保持一致。
4. **文档与回归联动**：跨团队契约变更仍需同步更新本文与对应 smoke/integration 用例。

### 0.4 建议下一步（实操）

1. 先做 second pass 的“接口不变、实现解耦”类重构，避免重新打开跨团队协议面。
2. 对 `ReloadConfig`、`StartRuntime`、debug 命令补最小回归矩阵并在 PR 中强制附带。
3. 对公共头继续做“平台类型外泄”清理，但保持向后兼容 include 入口。
4. 每次协议字段变更都在本文维护“变更摘要 + 兼容性结论”。

---

## 2. 总体分层与 ownership

### 2.1 分层关系

- **ServiceShell / ServiceEntry**：SCM、Console、进程入口、电源事件注册、服务安装/卸载。
- **ServiceHost**：唯一模块装配者与生命周期编排者；负责 IPC、配置加载、debug schema、模块接线。
- **Host/SystemStateMonitor**：系统事件采集与语义化，不做设备策略。
- **DeviceRuntime**：唯一运行状态机拥有者；负责命令排队、采帧编排、pipeline 调度。
- **Himax**：唯一硬件采帧/AFE/协议/HAL 拥有者。
- **Pen I/O**：唯一 BT MCU HID 发现、握手、ACK、事件/压感解码拥有者。
- **VHF**：唯一 HID 输出与设备写入拥有者。
- **App/IPC**：唯一跨进程控制与 debug 数据消费方。

### 2.2 Ownership 原则

1. **Shell 不直接调用 Runtime 业务策略**。
2. **ServiceHost 是唯一模块接线点**。
3. **Runtime 不直接依赖 Himax 内部字段布局**。
4. **Pen I/O 向上只输出语义事件/压感，不暴露 HID 报文细节。**
5. **VHF 对外只接受语义化 touch/stylus 输入。**
6. **公共头只暴露稳定契约，不暴露内部实现细节。**

---

## 3. 团队间接口协定

## 3.1 Shell 团队 ↔ Host 事件团队

### 边界
- Shell 负责把 SCM / Windows power broadcast 转成命名事件。
- Host 团队负责把命名事件转成 `Host::SystemStateEvent`。

### 当前锚点
- `EGoTouchService/source/ServiceShell.cpp:53`
- `EGoTouchService/Host/SystemStateMonitor.h:33`
- `EGoTouchService/Host/SystemStateMonitor.cpp:12`
- `EGoTouchService/Host/SystemStateMonitor.cpp:200`
- `EGoTouchService/Host/SystemStateEvent.h:8`

### 输入 / 输出
- Shell 输出：命名事件 signal
- Host 输出：`Host::SystemStateEvent`

### 必须冻结的契约
1. `SystemStateEvent` 是跨模块传递的**唯一系统事件对象**。
2. `raw_index` / `raw_name` **仅用于诊断**，不得作为业务分支稳定键。
3. 命名事件与语义事件映射必须保持一一受控。

### 当前脆弱点
- `ServiceShell.cpp` 通过 `NamedEventList()[idx]` 发送事件，依赖数组顺序。
- `SystemStateMonitor.cpp:207` 用 `switch(index)` 解释语义，形成隐式顺序协议。

### 契约文案
> Shell 仅负责把 OS/SCM 事件转换为 Host 事件源，不得直接调用 Runtime 策略接口。  
> Host 团队维护事件名到 `SystemStateEventType` 的映射表，并对该映射的版本变化负责。  
> 任何事件新增、改名、改序都必须同步更新映射表与回归测试。

---

## 3.2 Host 事件团队 ↔ Runtime 团队

### 边界
- Host 只做系统事件转译与回调调度。
- Runtime 只消费语义事件并决定 suspend/resume/shutdown 策略。

### 当前锚点
- `EGoTouchService/Host/SystemStateEvent.h:22`
- `EGoTouchService/Host/SystemStateMonitor.h:16`
- `EGoTouchService/Host/SystemStateMonitor.cpp:155`
- `EGoTouchService/Device/runtime/DeviceRuntime.h:114`
- `EGoTouchService/Device/runtime/DeviceRuntime.cpp:194`

### 输入 / 输出
- Host 输入：命名事件句柄被触发
- Host 输出：`const Host::SystemStateEvent&`
- Runtime 输入：`IngestSystemEvent(const Host::SystemStateEvent&)`
- Runtime 输出：状态迁移（ready/suspend/quit/recover）

### 线程契约
1. `SystemStateMonitor::EventCallback` 在 **monitor worker 线程同步执行**。
2. 回调实现必须短小、不可长时间阻塞。
3. 回调中不得形成对 `SystemStateMonitor::Stop()` 的自锁/自 join 风险。

### 行为契约
1. Runtime 只依据 `SystemStateEventType` 做策略判断。
2. 同类 display/lid 事件去抖属于 Runtime 内部策略，不是 Host 传输层保证。
3. Host 不承诺“不会重复发同语义事件”；Runtime 负责容忍重复。

### 契约文案
> Host 团队只负责“采集 + 语义化 + 回调”，不负责设备恢复/暂停策略。  
> Runtime 团队不得依赖 Windows 原始广播结构或命名事件序号，只依赖 `SystemStateEvent`。  
> 回调在线程模型上采用“同步、可重入、短耗时”契约。

---

## 3.3 ServiceHost 团队 ↔ Runtime 团队

### 边界
- `ServiceHost` 是 Runtime 的唯一上层装配者。
- Runtime 对外暴露稳定控制面，不暴露内部状态机细节。

### 当前锚点
- `EGoTouchService/include/ServiceHost.h:35`
- `EGoTouchService/source/ServiceHost.cpp:234`
- `EGoTouchService/Device/runtime/DeviceRuntime.h:83`
- `EGoTouchService/Device/runtime/DeviceRuntime.h:115`

### Runtime 对外稳定接口
- `Start()`
- `Stop()`
- `IngestSystemEvent(...)`
- `SubmitCommand(...)`
- `OnPenEvent(...)`
- `SetBtMcuPressure(...)`
- `GetPipeline()` / `GetStylusPipeline()` / `GetVhfReporter()`

### 生命周期契约
1. `ServiceHost::Start()` 先创建 Runtime，再创建 Monitor，再创建 IPC，再创建 Pen 模块。见 `EGoTouchService/source/ServiceHost.cpp:239-333`。
2. `ServiceHost::Stop()` 必须先停 Pen，再停 Monitor，再停 Runtime，避免回调访问已析构对象。见 `EGoTouchService/source/ServiceHost.cpp:338-380`。
3. `Start/Stop` 必须保持幂等或显式失败语义；不得隐式重复初始化底层设备。

### 配置契约
1. `ServiceHost` 负责读取 `[Service]` 与 pipeline 配置。
2. Runtime 只接收结果配置，不直接读配置文件。
3. `mode`、`auto_mode`、`stylus_vhf_enabled` 必须明确区分：
   - **配置态**：保存在 ini 中
   - **运行态**：当前进程内实际生效状态

### 契约文案
> ServiceHost 是唯一的 Runtime 装配与控制入口。  
> Runtime 对外只提供运行控制与数据注入接口，不暴露内部 worker 状态机与硬件细节。  
> 生命周期顺序由 ServiceHost 统一保证，其他模块不得绕过 ServiceHost 直接串联 Runtime。

---

## 3.4 Runtime 团队 ↔ Himax Core 团队

### 边界
- Runtime 负责状态机与调度。
- Himax 负责设备句柄、协议、帧采集、AFE 命令执行。

### 当前锚点
- `EGoTouchService/Device/runtime/DeviceRuntime.cpp:181`
- `EGoTouchService/Device/runtime/DeviceRuntime.cpp:264`
- `EGoTouchService/Device/runtime/DeviceRuntime.cpp:274`
- `EGoTouchService/Device/himax/HimaxChip.h:69`

### 当前事实接口
- `Chip::Init()`
- `Chip::Deinit()`
- `Chip::HoldReset()`
- `Chip::GetFrame()`
- `m_afe.SendCommand(...)`（当前为泄漏接口）

### 当前脆弱点
- Runtime 直接访问：
  - `m_chip.m_afe`
  - `m_chip.back_data`
  - `m_chip.m_lastMasterWasRead`
- 这意味着 Runtime 与 Himax 共享内部布局，而不是共享稳定 facade。

### 必须冻结的契约
1. Runtime 不得读取 Himax 内部 public 字段。
2. Himax 必须提供显式的“帧结果 / 状态快照 / 命令返回”接口。
3. Himax 错误语义必须通过 `ChipError` 或等价稳定错误通道暴露。
4. 未实现的 AFE 能力不得伪装为成功。

### 契约文案
> Runtime 与 Himax 的唯一交互方式是稳定公开接口；Runtime 不得访问 Himax 内部缓存与控制器成员。  
> Himax 模块对外提供生命周期、采帧、AFE 控制、错误语义四类能力。  
> 帧元数据必须通过显式返回结构暴露，而不是依赖上层读取内部成员。

---

## 3.5 Runtime 团队 ↔ BT Pen I/O 团队

### 边界
- Pen I/O 团队负责 HID 发现、握手、ACK、报文解码。
- Runtime 团队负责消费“已解码”的笔事件与压感值。

### 当前锚点
- `EGoTouchService/Device/penevt/PenEventBridge.h:33`
- `EGoTouchService/Device/penevt/PenEventBridge.cpp:199`
- `EGoTouchService/Device/penpress/PenPressureReader.h:34`
- `EGoTouchService/source/ServiceHost.cpp:313-327`
- `EGoTouchService/Device/runtime/DeviceRuntime.h:105`
- `EGoTouchService/Device/runtime/DeviceRuntime.h:123`

### Pen I/O 输出接口
- `PenEventBridge::SetEventCallback(PenEventCallback)`
- `PenPressureReader::SetPressureCallback(PressureCallback)`
- `PenPressureReader::GetPressureStats()`

### 线程契约
1. `PenEventCallback` 从事件读取线程触发。头文件已注明“不得长时间阻塞”。见 `PenEventBridge.h:33`。
2. `PressureCallback` 从压感读取线程触发，也必须无阻塞。
3. Runtime 若需耗时处理，必须转发到自己的队列/状态机，而不是在回调线程里做复杂逻辑。

### 必须冻结的契约
1. Pen I/O 对上层输出的稳定对象应是**结构化笔事件**，而不是长期依赖 `payload[0]` 之类裸字节。
2. 至少 `PenConnStatus`、`PenTypeInfo`、`PenCurStatus` 应有稳定字段定义。
3. 压感回调语义必须明确：当前是四通道 max 值，不是原始全量通道。

### 契约文案
> Pen I/O 模块对外只交付语义化 `PenEvent` 与 `PressureSample`，HID 报文字节布局不属于跨团队稳定契约。  
> Runtime 不得依赖握手时序、ACK 表、magic byte 细节。  
> 回调在线程上为“工作线程同步回调”，消费方必须保证无阻塞。

---

## 3.6 Runtime 团队 ↔ VHF 团队

### 边界
- Runtime 负责准备语义帧数据。
- VHF 负责将 touch/stylus 语义结果编码并写入系统 HID。

### 当前锚点
- `EGoTouchService/Device/runtime/DeviceRuntime.cpp:281-289`
- `EGoTouchService/Device/vhf/VhfReporter.h:27-54`

### 输入 / 输出
- Runtime 输入到 VHF：
  - `Solvers::HeatmapFrame&`
  - `Solvers::StylusPacket`
- VHF 输出：
  - 设备写入
  - 可能修改 `frame.touchPackets`

### 必须冻结的契约
1. VHF 输入是语义化 touch/stylus 数据，不是底层硬件帧。
2. `DispatchTouch(...)` 若会修改 `HeatmapFrame`，必须在接口文档中明示副作用。
3. VHF 应提供清晰的设备状态语义：open / write fail / reconnecting，而不能只体现在日志。
4. `eraser`、transpose 等外部可控状态必须有稳定字段语义。

### 契约文案
> Runtime 只负责把语义结果交给 VHF，VHF 独占 HID 报文字节布局与设备重连策略。  
> VHF 的写入失败语义必须通过接口可观测，而不是仅靠日志。  
> Touch/stylus 数据编码规则由 VHF 模块独占维护，上游不得内嵌复制。

---

## 3.7 Device API 团队 ↔ 各 Device 子模块团队

### 边界
- `Device.h` 与 Device 公共头定义稳定的公共类型边界。
- 子模块内部实现不得反向依赖聚合头。

### 当前锚点
- `EGoTouchService/Device/Device.h:2`
- `EGoTouchService/Device/himax/HimaxProtocol.h:14`
- `EGoTouchService/Device/vhf/VhfReporter.h:11`
- `EGoTouchService/Device/penevt/PenEventBridge.h:5`

### 必须冻结的契约
1. `Device.h` 仅供外层兼容 include，不得成为内部模块通用依赖入口。
2. 公共头应尽量只暴露平台无关类型。
3. `HANDLE/windows.h` 等 Win32 细节原则上下沉到 `.cpp`。
4. 领域对象应采用语义化命名；例如 `command` 应收敛为更明确的领域名称。

### 契约文案
> `Device.h` 是兼容聚合头，不是内部模块协作边界。  
> 内部模块必须 include 自己真正依赖的最小头文件集合。  
> 公共声明层禁止扩大平台细节外泄面。

---

## 3.8 ServiceHost 团队 ↔ IPC / App 团队

### 边界
- `IpcProtocol.h` 是命名管道线协议唯一真源。
- `SharedFrameBuffer.h` 是 debug 帧共享内存布局唯一真源。
- `ServiceHost` 是服务端 IPC 命令唯一入口。

### 当前锚点
- `Common/include/IpcProtocol.h:13`
- `Common/include/SharedFrameBuffer.h:78`
- `EGoTouchService/include/ServiceHost.h:99`
- `EGoTouchService/source/ServiceHost.cpp:690`
- `Tools/EGoTouchApp/source/ServiceProxy.cpp:5`

### IPC 契约
#### 命令面
- `Ping`
- `EnterDebugMode` / `ExitDebugMode`
- `StartRuntime` / `StopRuntime`
- `AfeCommand`
- `SetVhfEnabled` / `SetVhfTranspose`
- `ReloadConfig` / `SaveConfig`
- `GetLogs`
- `GetPenBridgeStatus`
- `GetDebugSchema` / `GetDebugSnapshot`

#### 共享内存面
- `SharedFrameData`
- `SharedTripleBuffer`
- `kSharedFrameName`
- `kFrameReadyEventName`

### 必须冻结的契约
1. `IpcProtocol.h` 任何字段改动都视为协议变更。
2. `SharedFrameData` 字段布局改动都视为 App/Service 协同变更。
3. `EnterDebugMode` / `ExitDebugMode` 的 release / debug 语义必须明确写在协议里。
4. `ReloadConfig` 必须声明每个配置项是：
   - 立即生效
   - 仅下次启动生效
5. `GetDebugSchema` 的 `schemaVersion` 与 `schemaHash` 必须形成稳定版本策略，不得长期固定 version 而只变 hash。

### 当前脆弱点
- `EnterDebugMode` 注释写“请求携带共享内存名”，但当前实现并未使用该参数。见 `IpcProtocol.h:15-17` 与 `ServiceHost.cpp:699-721`。
- `ReloadConfig` 当前只明确即时应用了 `stylus_vhf_enabled`，而 `mode/auto_mode` 的即时生效语义并未完整兑现。见 `ServiceHost.cpp:763-796`。
- `StartRuntime` 在服务启动后再次调用时的幂等/失败语义未明确。见 `ServiceHost.cpp:750-760`。

### 契约文案
> `IpcProtocol.h` 是线协议单一真源；`SharedFrameBuffer.h` 是共享内存布局单一真源。  
> App 与 Service 均不得在协议文件之外额外定义“隐式字段语义”。  
> 所有 IPC 命令必须有明确的幂等性、失败语义和版本兼容策略。  
> Debug schema 的字段顺序、`fieldId`、`valueType` 一经发布即视为契约。

---

## 4. 必须先冻结的全局协定

## 4.1 系统事件协定
涉及团队：Shell、Host、Runtime、ServiceHost

冻结项：
1. 语义事件枚举
2. raw 诊断字段是否保留
3. 回调线程模型
4. 重复事件是否允许
5. 停止顺序与 Stop 重入语义

## 4.2 Pen 运行状态协定
涉及团队：Runtime、Himax、Pen I/O、VHF

冻结项：
1. `connected / hover / eraser / writing` 的统一状态模型
2. `PenCurStatus` 的正式语义
3. 压感值的来源、单位与刷新语义
4. VHF 对笔状态字段的消费方式

## 4.3 配置与运行态协定
涉及团队：ServiceHost、Runtime、App/IPC

冻结项：
1. `mode`
2. `auto_mode`
3. `stylus_vhf_enabled`
4. `debugMode`
5. 配置变更是“即时生效”还是“重启生效”

## 4.4 Debug schema 协定
涉及团队：ServiceHost、App/IPC

冻结项：
1. `fieldId`
2. `valueType`
3. `sourceKind`
4. `schemaVersion` + `schemaHash` 策略
5. snapshot 中无值字段的 flags 语义

## 4.5 公共头边界协定
涉及团队：Device API、Himax、Pen I/O、VHF、ServiceHost

冻结项：
1. 哪些类型允许出现在 public header
2. 哪些 Win32 细节必须下沉
3. 聚合头的使用范围
4. 类型命名是否具有领域语义

---

## 5. 变更规则

### 5.1 必须走跨团队评审的变更
以下任一变更都必须走跨团队评审：

1. `SystemStateEvent` 字段增删改
2. 命名事件名或顺序变更
3. Runtime ↔ Himax 的公开方法签名变更
4. `PenEvent` / `PenPressureStats` 结构变更
5. `IpcProtocol.h` 任一 wire struct 或 command 变更
6. `SharedFrameData` 布局变更
7. `DebugFieldSchemaWire` / `DebugSnapshotValueWire` 语义变更
8. 公共头新增 Win32 类型泄漏

### 5.2 必须同步更新的文档/测试
变更发生时，至少同步更新：

- 本文档
- 对应协议注释
- 相关 smoke test / 集成测试
- App 侧兼容处理（如涉及 IPC/shared memory）

---

## 6. 并发开发建议顺序

### Wave 1：先冻结协议，再拆实现
1. 系统事件协定
2. Pen 运行状态协定
3. 配置与运行态协定
4. Debug schema 协定

### Wave 2：按边界拆模块
1. Runtime ↔ Himax
2. Pen I/O ↔ Runtime
3. Runtime ↔ VHF
4. ServiceHost ↔ IPC/App

### Wave 3：最后做公共头收口
1. Device 公共边界净化
2. include 层 API 收口
3. 文档与测试补齐

---

## 7. 当前最需要先定稿的五项接口

1. **`SystemStateEvent` 的正式合同**  
   否则 Host / Shell / Runtime 会继续通过隐式索引耦合。

2. **Runtime ↔ Himax 的 facade 合同**  
   否则 Runtime 无法和 Himax 团队真正并行。

3. **`PenEvent` 结构化合同**  
   否则 Pen I/O、Runtime、VHF 会持续围绕 payload 裸字节反复对齐。

4. **`ReloadConfig` 生效级别合同**  
   否则 App、ServiceHost、Runtime 对 mode/auto_mode 的期望会继续分叉。

5. **Debug schema 版本合同**  
   否则 App 与 Service 的 debug 功能会长期依赖弱约定。

---

## 8. 附：关键锚点文件

- `EGoTouchService/Host/SystemStateEvent.h`
- `EGoTouchService/Host/SystemStateMonitor.h`
- `EGoTouchService/Host/SystemStateMonitor.cpp`
- `EGoTouchService/source/ServiceShell.cpp`
- `EGoTouchService/source/ServiceEntry.cpp`
- `EGoTouchService/include/ServiceHost.h`
- `EGoTouchService/source/ServiceHost.cpp`
- `EGoTouchService/Device/runtime/DeviceRuntime.h`
- `EGoTouchService/Device/runtime/DeviceRuntime.cpp`
- `EGoTouchService/Device/penevt/PenEventBridge.h`
- `EGoTouchService/Device/penpress/PenPressureReader.h`
- `EGoTouchService/Device/vhf/VhfReporter.h`
- `EGoTouchService/Device/Device.h`
- `Common/include/IpcProtocol.h`
- `Common/include/SharedFrameBuffer.h`
- `Tools/EGoTouchApp/source/ServiceProxy.cpp`

---

## 9. 结论

这份接口协定的核心目的不是把实现细节写死，而是先冻结**跨团队稳定面**：

- 谁拥有什么
- 谁只能依赖什么
- 哪些字段/回调/协议是稳定合同
- 哪些地方现在仍是隐式耦合，不能直接并行开发

在进入并发重构前，建议先以本文为 baseline 做一次接口评审，先确认第 7 节的五项合同，再开始分团队实施。