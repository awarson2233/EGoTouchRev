# 非 Solver 模块代码质量审查清单与多 Agent 整改计划

> 更新日期：2026-04-20  
> 范围：`EGoTouchService/source`、`EGoTouchService/Host`、`EGoTouchService/include`、`EGoTouchService/Device`、`Tools`  
> 明确排除：`EGoTouchService/Solvers/**`

---

## 1. 结论摘要

本轮审查确认，非 Solver 代码的主要问题不在于单点 bug，而在于**边界失真、职责混杂、状态语义不清、测试分层不足、文档缺失**。

如果目标是“真正提高质量”，不建议继续采用局部修补、轻度包装、只改命名/注释的折中路线。  
**本计划采用效果优先方案**：直接重建关键 ownership、接口边界和状态模型。

核心方向：

1. **Service 侧统一配置主权与运行时编排主权**。
2. **Device 层收缩为设备采集/协议适配层，不再直接暴露求解链与上层输出细节**。
3. **Tools 侧改造成 thin client，不再本地持有一套服务端内部状态模型**。
4. **测试体系按 unit / integration / benchmark 重新分层**。
5. **跨进程契约与状态机补足正式文档**。

---

## 2. 审查清单

### S1 High

| ID | 模块 | 关键文件 | 问题 | 建议采用的强方案 |
|---|---|---|---|---|
| NS-S1-01 | Service/Host | `EGoTouchService/source/ServiceHost.cpp`、`EGoTouchService/include/ServiceHost.h` | `ServiceHost` 已成为 orchestration god object，混合配置、生命周期、IPC、debug schema、pen 子系统与运行时控制 | 直接拆成 `ServiceConfigStore`、`ServiceCommandDispatcher`、`DebugSchemaRegistry`，`ServiceHost` 只保留生命周期协调职责 |
| NS-S1-02 | Device | `EGoTouchService/Device/runtime/DeviceRuntime.h`、`EGoTouchService/Device/runtime/DeviceRuntime.cpp` | `DeviceRuntime` 同时承担设备采集、状态机、Host 电源策略、solver 调度、VHF 输出、调试回调 | 把 `DeviceRuntime` 限缩为采集/状态执行层；将 solver orchestration 与输出编排上移到服务编排层 |
| NS-S1-03 | Tools | `Tools/EGoTouchApp/include/ServiceProxy.h`、`Tools/EGoTouchApp/source/DiagnosticsWorkbench.*` | `ServiceProxy` 和 `DiagnosticsWorkbench` 同时承担 UI、配置 ownership、运行时控制、导入导出、副作用逻辑 | 改造成 thin client：UI 只消费 DTO/view-model，command 通过明确 API 发送，render 层不直接改 domain object |

### S2 Medium

| ID | 模块 | 关键文件 | 问题 | 建议采用的强方案 |
|---|---|---|---|---|
| NS-S2-01 | Service/Host | `EGoTouchService/source/ServiceShell.cpp`、`EGoTouchService/Host/SystemStateEvent.h`、`EGoTouchService/Host/SystemStateMonitor.cpp` | `ServiceShell` 已直接编码 Host 事件语义；多个 named event 被折叠为同一领域语义，归一化责任不清 | 让 `ServiceShell` 只转发原始 Win32 事件；在 Host/Service 内引入单一 power-event translator，统一 authoritative event model |
| NS-S2-02 | Device | `EGoTouchService/Device/penevt/PenEventBridge.cpp`、`EGoTouchService/Device/btmcu/PenUsbTypes.h` | BT MCU 握手流程主要依赖 sleep、ACK 映射和布尔标志；`PenSessionState` 未真正接管流程 | 用正式状态机替代 `sleep + flag`，让 `PenSessionState` 成为唯一的握手推进模型 |
| NS-S2-03 | Device | `EGoTouchService/Device/common/DeviceError.h`、`runtime/DeviceRuntime.h`、`vhf/VhfReporter.h`、`penevt/PenEventBridge.h` | 错误语义不统一，底层有 `std::expected`，上层仍混用 `bool`、`void` 和多套结果类型 | 定义统一的设备层 public result 语义，停止扩散裸 `bool` 接口 |
| NS-S2-04 | Tools | `Tools/EGoTouchApp/source/ServiceProxy.cpp`、`ServiceProxy.Config.cpp`、`DiagnosticsWorkbench.*`、`Tools/tests/RawdataBenchmarkTest.cpp` | ProgramData 路径、config 迁移、legacy key 映射重复实现 | 统一成单一 `RuntimePaths` / `ConfigMigration` 契约，由 Service 持有 canonical owner |
| NS-S2-05 | Tools/tests | `Tools/tests/*` | unit / integration / benchmark 混在一起，且存在直接 include 私有实现头、依赖 named event 与 `sleep_for` 的测试 | 直接重组测试目标：拆分测试目录与 target，benchmark 迁出 `tests/` |

### S3 Low

| ID | 模块 | 关键文件 | 问题 | 建议采用的强方案 |
|---|---|---|---|---|
| NS-S3-01 | Service | `EGoTouchService/include/ServiceHost.h` | 看似公共头，实则暴露大量内部运行时/调试细节，目录语义与 ownership 不一致 | 如果不是稳定公共 API，移回 internal/source 布局；否则拆出窄接口头 |
| NS-S3-02 | Service | `EGoTouchService/source/ServiceHost.cpp` | 存在不真正驱动行为的状态，如 `m_debugMode` | 删除死状态，或升级为真正的 session state |
| NS-S3-03 | 全局 | `EGoTouchService/Device`、`EGoTouchService/source`、`Tools` | 缺少模块级文档、跨进程契约文档、状态机文档 | 为 Service lifecycle、system-state、Device state machine、Tools DVR/config contract 补文档 |

---

## 3. 推荐整改路线：直接采用效果更好的方案

本项目当前最有效的路线不是“小步修补”，而是**先冻结关键契约，再并行重构三个主边界**。

### 3.1 必须先统一的三个契约

这三个契约如果不先定下来，后续多 agent 并发会相互打架：

1. **配置主权契约**
   - 最终 owner：`Service`
   - `Tools` 不再直接写 `config.ini`
   - 只允许通过 command / patch / DTO 请求配置修改

2. **系统状态事件契约**
   - `ServiceShell` 只产生原始系统事件
   - `Host/Service` 内部负责归一化为统一领域事件
   - 不再扩散重复的 named event 语义别名

3. **Device 边界契约**
   - `Device` 只处理设备会话、采集、协议适配、错误语义
   - solver 调用、VHF 输出编排、debug frame 编排上移到服务层协调
   - 停止在 public header 暴露 solver/VHF/Host 细节

> 这一步建议由一个架构 agent 独占完成，不建议并发。

---

## 4. 多 Agent 并发执行规划

### 4.1 阶段划分

#### Phase 0：契约冻结（单 agent，不能并发）

| 任务 | 目标输出 | 原因 |
|---|---|---|
| 定义配置主权模型 | config command/DTO、owner 边界、立即生效/重启生效语义 | 同时影响 Service 与 Tools |
| 定义 system-state event 归一化模型 | 原始事件到领域事件映射表 | 同时影响 `ServiceShell`、`SystemStateMonitor`、`ServiceHost` |
| 定义 Device 公共边界 | 允许暴露的接口、统一错误模型、状态机边界 | 同时影响 `DeviceRuntime`、`ServiceHost`、`Tools` |

#### Phase 1：主干并行重构（可 3~4 agent 并发）

| 并发任务 | 目标文件/目录 | 是否可并发 | 依赖 |
|---|---|---|---|
| Workstream A：拆 `ServiceHost` | `EGoTouchService/source`、`EGoTouchService/include`、`EGoTouchService/Host` | 是 | 依赖 Phase 0 |
| Workstream B：重建 `DeviceRuntime` 边界与 pen 状态机 | `EGoTouchService/Device/runtime`、`Device/penevt`、`Device/vhf`、`Device/himax` | 是 | 依赖 Phase 0 |
| Workstream C：把 App 改成 thin client | `Tools/EGoTouchApp` | 是 | 依赖 Phase 0 |
| Workstream D：先行拆测试与 benchmark 结构 | `Tools/tests`、相关 CMake target | 部分可并发 | 最好在 A/B/C 接口趋稳后完成最终落地 |

#### Phase 2：收口与验证（可 2~3 agent 并发）

| 并发任务 | 目标文件/目录 | 是否可并发 | 依赖 |
|---|---|---|---|
| Workstream E：补文档与契约说明 | `docs/`、必要头文件注释 | 是 | 依赖 Phase 0，可在 A/B/C 期间同步推进；最终版依赖 A/B/C |
| Workstream F：修正测试适配层 | `Tools/tests`、可能涉及 `Common/` 和 target | 是 | 依赖 A/B/C 主要接口稳定 |
| Workstream G：目录与 public header 清理 | `include/`、internal/source 布局 | 是 | 依赖 A/B/C 已确定归属 |

#### Phase 3：总体验收（单 agent 或主会话串行）

| 任务 | 说明 |
|---|---|
| 集成验证 | 验证 Service 启停、配置修改、system-state、Device 运行态、Tools 连接 |
| 定向代码审查 | 对 `Service/Host`、`Device`、`Tools` 再做一次深审 |
| 回归测试 | 重新执行 unit / integration / benchmark |

---

## 5. 推荐的多 Agent 分工

### Agent 0：架构冻结 Agent（必须先做）

**职责**
- 产出三个 canonical contract：配置主权、system-state event、Device 边界
- 明确每个 workstream 的目标接口与禁止跨层访问项

**建议输出**
- 一页接口/ownership 说明
- 变更边界清单
- 后续 agent 的禁止事项

### Agent 1：Service/Host 重构 Agent

**负责目录**
- `EGoTouchService/source`
- `EGoTouchService/include`
- `EGoTouchService/Host`

**目标**
- `ServiceHost` 降级为 lifecycle coordinator
- 抽出 config store、command dispatcher、debug schema registry
- `ServiceShell` 不再直接知道 Host 领域事件枚举

### Agent 2：Device 重构 Agent

**负责目录**
- `EGoTouchService/Device/runtime`
- `EGoTouchService/Device/penevt`
- `EGoTouchService/Device/vhf`
- `EGoTouchService/Device/himax`

**目标**
- `DeviceRuntime` 收缩为设备运行态执行层
- 用正式状态机接管 pen 握手
- 统一错误语义
- 移除 public header 中的上层泄漏

### Agent 3：Tools 重构 Agent

**负责目录**
- `Tools/EGoTouchApp`
- 必要时触及少量 `Common/IPC` 契约

**目标**
- `ServiceProxy` 不再持有本地 canonical config ownership
- `DiagnosticsWorkbench` 拆分 render 与 controller/use-case
- 所有副作用逻辑退出 UI render 路径

### Agent 4：测试/文档 Agent

**负责目录**
- `Tools/tests`
- `docs/`
- 相关 CMake 配置

**目标**
- unit / integration / benchmark 分层
- benchmark 移出 `tests/`
- 补齐契约文档与测试说明

> **最佳并发度建议：4 个实现 agent + 1 个架构 agent。**  
> 不建议一开始就 5 个实现 agent 全并发，因为在契约未冻结前，返工成本会显著高于并发收益。

---

## 6. 哪些任务适合并发，哪些不适合

### 明确适合并发

1. **`ServiceHost` 拆分** 与 **`DeviceRuntime` 边界重建**
   - 前提：Phase 0 已冻结 Device 与 Service 契约
2. **Tools thin client 改造** 与 **Service 侧 command/DTO 收口**
   - 前提：配置主权已统一到 Service
3. **测试分层重组** 与 **文档补充**
   - 前提：核心接口已基本稳定
4. **目录/public header 清理** 与 **最终深审**
   - 前提：主要实现已落地

### 不适合直接并发

1. **配置主权设计**
   - Service 和 Tools 都依赖它，必须单点拍板
2. **system-state authoritative event model**
   - `ServiceShell`、`Host`、`ServiceHost` 都受影响，不宜多头设计
3. **Device public boundary 定义**
   - 一旦多个 agent 同时修改边界，很容易产生新的跨层耦合

---

## 7. 建议的执行顺序

### Step 1：先冻结契约
- 配置主权
- system-state authoritative event
- Device public boundary

### Step 2：三路并发主改造
- Agent 1：Service/Host
- Agent 2：Device
- Agent 3：Tools

### Step 3：两路并发收口
- Agent 4：测试分层与 benchmark 清理
- Agent 5：文档、public header 与目录结构清理

### Step 4：集中验收
- 集成验证
- 深度复审
- 回归测试

---

## 8. 验收标准

### Service/Host
- `ServiceHost` 不再同时持有配置解析、命令分发、debug schema 构建三类职责
- `ServiceShell` 不直接编码 Host 领域事件
- `desired state` / `active state` 语义明确区分

### Device
- `DeviceRuntime` public header 不再直接暴露 solver/VHF/Host 细节
- pen 握手流程由显式状态机驱动
- 设备层 public API 不再新增裸 `bool` 结果语义

### Tools
- UI render 路径不再直接触发副作用业务流程
- `ServiceProxy` 不再直接写 canonical config 文件
- 配置与运行态查询通过 DTO / command 边界完成

### Tests / Docs
- unit / integration / benchmark 边界清晰
- `Tools/tests` 不再承载 benchmark 工具职责
- `docs/` 中有 service lifecycle、device state machine、tools contract 三类说明

---

## 9. 当前建议：下一步应该做什么

如果要正式进入整改，建议直接按下面顺序发起：

1. **先做 Phase 0 契约冻结**
2. **随后并发启动 3 个实现 agent**：Service/Host、Device、Tools
3. **接口稳定后，再并发启动测试/文档 agent**
4. **最后统一做深审和集成验证**

这条路线不是折中方案，但从效果与返工成本比来看，是当前最优解。