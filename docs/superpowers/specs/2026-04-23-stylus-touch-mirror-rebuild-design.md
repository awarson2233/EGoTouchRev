# Stylus Touch-Mirror Rebuild Design

> 日期：2026-04-23
> 状态：Draft for review
> 目标仓库：`D:\source\repos\EGoTouchRev-rebuild`

## 1. 设计结论

本次重构选择 **A. Full Touch-Mirror Pipeline**，并按用户确认的边界执行：

1. `StylusPipeline` 的整体职责要与 `TouchPipeline` 对齐。
2. stylus 全链路围绕一个统一的大帧 `HeatmapFrame` 加工。
3. 删除 `StylusFrameState` 这一层，不再保留 frame-scoped scratch owner。
4. 保留现有手写笔算法主体，但删除 `PenStateMachine` 及其派生出的 fast-ink / lift / animation / committed-frame 语义。
5. `BtPressBuffer` 允许作为 pipeline 外部输入源存在，但进入单帧处理后要先快照进大帧，再由各 phase 只读大帧。
6. stylus packet 不再由 solver 组装；改为像 touch 一样，由 `VhfReporter` 根据 `frame.stylus` 最终状态后置组包。
7. diagnostics 不再使用集中式 `StylusDiagnosticsWriter + StylusDiagnostics` 汇总；改成 touch 风格的分散式 debug-only 数据与 pipeline cache。
8. Release 构建中收窄 `StylusFrameData`，大量过程镜像和调试字段迁入 `#if EGOTOUCH_DIAG`。

这不是“兼容式精简”，而是一次 **破坏式、单分支、大重构**。目标不是让旧 stylus 架构更干净，而是让它在结构、数据流、文件职责、诊断模式上都变成 touch 同类实现。

---

## 2. 为什么不选其他方案

### 2.1 不选 B. Touch-Like Surface, Stylus-Like Internals

这个方案只是把主入口伪装成 `Process(frame)` 顺排，但内部仍保留 `StylusFrameState` 或等价 scratch 聚合层。这样会保留当前 stylus 特有的隐藏上下文模型，导致：

1. `StylusPipeline` 表面和 `TouchPipeline` 相似，真实心智模型却不同。
2. phase 模块仍在读写 scratch，而不是读写统一帧，后续维护会继续被“第二套帧模型”反噬。
3. packet、diag、lifecycle 这些旧职责很容易继续挂回 orchestrator。

该方案降低了首轮改动难度，但会把本次重构降格为“再一次整理旧架构”，不符合目标。

### 2.2 不选 C. Mega-Module Collapse

该方案会把很多 stylus 模块压成少数几个大文件。虽然文件数减少，但会偏离 `TouchPipeline` 当前的模块粒度和风格：

1. touch 是“单 orchestrator + 多 phase 模块”，不是“单 orchestrator + 三个大 stage”。
2. stylus 如果压成巨型 stage 文件，会形成新的不可维护热点。
3. 这种结构会让后续 cross-review 无法直接按 touch 的经验类比。

因此最终设计坚持 **single orchestrator + explicit phase modules**。

---

## 3. 当前问题

当前 stylus 实现与 touch 的核心差异，不仅是模块名称不同，而是架构基因不同：

1. `StylusPipeline` 通过 `StylusFrameState` 驱动多分支状态流，而 `TouchPipeline` 直接顺排读写 `HeatmapFrame`。
2. stylus 把 packet、commit、terminal、reuse、animation、diag 汇总都留在 solver 内部；touch 只负责产出 `frame.contacts` 和若干 debug cache。
3. stylus 有较多 release 期过程镜像字段，`StylusFrameData` 承担了“最终输出 + 中间过程 + 对齐老协议 + 调试镜像”四种角色；touch 则把 frame 内常驻结构收敛到输出和必要运行态。
4. touch 的 diagnostics 是分散式的：debug frame 字段和 pipeline cache 贴着 phase 落地；stylus 的 diagnostics 则集中在一个 writer 和一个大 diag struct 上。
5. `TouchTracker` 已经读取 `frame.stylus` 参与 pen-touch suppression / AFT，但 stylus 当前输出结构过于依赖 anim state 和其他旧生命周期字段。

如果仅做“精简式重构”，这些深层差异仍会存在。

---

## 4. 目标

### 4.1 一级目标

把 stylus 重写为与 `TouchPipeline` 同类的 solver：

1. `StylusPipeline::Process(HeatmapFrame& frame)` 是唯一大帧入口。
2. `StylusPipeline` 只做阶段顺排、config 汇总、少量 debug cache 访问。
3. 所有算法/逻辑由 phase 模块接管。
4. 所有 phase 围绕 `HeatmapFrame` 和 `frame.stylus` 工作。

### 4.2 二级目标

1. 删除 `StylusFrameState.hpp`
2. 删除 `PenStateMachine.hpp`
3. 删除 `StylusDiagnosticsWriter.hpp`
4. 删除 solver 内 packet build 责任
5. 删除 committed-frame / reuse / terminal-commit 这类旧输出语义
6. 将 stylus release 数据结构收窄到“最终输出 + touch interop 必要信号”

### 4.3 明确保留

1. slave overlay 解析算法
2. TX1/TX2 peak / projection / coordinate 求解算法
3. signal analysis / recheck / overlap 判定主体
4. BT pressure 映射算法
5. CoorReviser / LinearFilter / edge correction 等坐标后处理算法
6. touch suppression 所需的 stylus interop 信号

---

## 5. 明确不做

1. 不保留旧 `StylusFrameData` ABI 兼容层
2. 不保留 packetRoute / packet / commit / finalize 兼容语义
3. 不保留 `animState`、fast-lift、sustain-output、reuse-committed-frame 等旧状态机输出
4. 不引入新的总控型中间层，如 `StylusEngine`、`StylusCoordinator`
5. 不把触摸和手写笔合并成一个共享 solver，只做结构对齐，不做算法合并

---

## 6. 目标架构

### 6.1 目标入口

目标态的 `StylusPipeline` 应与 `TouchPipeline` 处于同一抽象层级：

```cpp
class StylusPipeline {
public:
    bool Process(HeatmapFrame& frame);

    std::vector<ConfigParam> GetConfigSchema() const;
    void SaveConfig(std::ostream& out) const;
    void LoadConfig(const std::string& key, const std::string& value);

    void SetBtMcuPressure(uint16_t pressure);

#if EGOTOUCH_DIAG
    StylusDebugPeaks GetPeaks() const;
    StylusDebugProjection GetTx1Projection() const;
    StylusDebugProjection GetTx2Projection() const;
#endif

    StylusFrameParser      m_frameParser;
    CommonModeFilter       m_cmf;
    GridPeakDetector       m_peakDet;
    CoordinateSolver       m_coordSolver;
    StylusSignalAnalyzer   m_signalAnalyzer;
    PressureSolver         m_pressure;
    StylusOutputGate       m_outputGate;
    StylusCoordinateFilter m_coordFilter;
    CoorReviser            m_coorReviser;
    EdgeLiftCorrector      m_edgeComp;
};
```

核心点：

1. 只保留 module members，不再保留 `OutputState` / `StylusFrameState` / centralized diagnostics writer。
2. `Process()` 中每一段都直接读写 `frame`。
3. packet 组装和 HID route 不再属于 solver。

### 6.2 目标 phase 分层

建议采用与 touch 同风格的六阶段组织：

1. **Phase 1: Frame Parsing**
   - `StylusFrameParser`
   - 从 `rawPtr/slaveSuffix` 解析 slave frame
   - 回填 `frame.stylus.input`、`frame.stylus.runtime.parseStatus`

2. **Phase 2: Signal Conditioning**
   - `CommonModeFilter`
   - 对 TX1/TX2 grid 做 conditioning
   - debug-only 时保存 pre/post-CMF 投影或摘要

3. **Phase 3: Feature Extraction**
   - `GridPeakDetector`
   - 产出 TX1/TX2 peak、projection、anchor、peak signal

4. **Phase 4: Solve & Analysis**
   - `CoordinateSolver`
   - `StylusSignalAnalyzer`
   - `PressureSolver`
   - 输出 global coor、tilt 输入、signal metrics、pressure snapshot

5. **Phase 5: Output Filtering & Validity**
   - `StylusOutputGate`
   - `StylusCoordinateFilter`
   - `CoorReviser`
   - `EdgeLiftCorrector`
   - 完成坐标合法化、抖动处理、coordinate revise、edge 修正、tip/in-range 决策

6. **Phase 6: Debug Cache & Final Mirrors**
   - pipeline 自身写 thread-safe debug cache
   - 不再生成 stylus packet

这里最关键的变化是：原先由状态机驱动的生命周期决策，将缩成一个 **无历史状态机语义的输出 gate**。它只负责本帧输出合法化和必要的轻量跨帧滤波，不再承担 fast ink / release staging / committed reuse。

---

## 7. 大帧模型

### 7.1 总体原则

重构后所有阶段都围绕 `HeatmapFrame` 工作。`HeatmapFrame` 仍然是 touch + stylus 共用大帧，但 stylus 要在其中形成更清晰的“输入 / 输出 / interop / debug”分区。

### 7.2 `StylusFrameData` 新边界

重构后建议把 `StylusFrameData` 拆成四个职责层次：

```cpp
struct StylusFrameData {
    StylusInputSnapshot input;
    StylusOutputState   output;
    StylusTouchInterop  interop;
#if EGOTOUCH_DIAG
    StylusDebugFrame    debug;
#endif
};
```

#### `StylusInputSnapshot`

保存每帧进入 solver 时已经冻结的输入摘要：

1. slave 是否有效
2. status / checksum / tx1/tx2 block valid
3. 最新 BT pressure sample 的快照

它的目的不是提供第二套 scratch，而是让下游 phase 都能从统一帧读取“该帧已知输入”。

#### `StylusOutputState`

只保留 release 真正需要的最终输出：

1. `inRange`
2. `tipDown`
3. `pressure`
4. `StylusSolvePoint point`
5. 必要的 `status` / `valid` / final confidence

这里不再保留 `StylusPacket`。

#### `StylusTouchInterop`

只保留 touch suppression / AFT 真正需要的字段：

1. `signalX`
2. `signalY`
3. `maxRawPeak`
4. `recheckPassed`
5. `recheckThreshold`
6. `recheckThresholdMulti`
7. `touchSuppressActive`
8. `touchSuppressFrames`
9. `touchNullLike`

`TouchTracker` 后续只能依赖这一组稳定 interop 字段，而不能再依赖 `animState`、fast-lift 或 packet 相关镜像。

#### `StylusDebugFrame`

全部在 `#if EGOTOUCH_DIAG` 下存在。内容按 phase 分段：

1. `parse`
2. `conditioning`
3. `extract`
4. `solve`
5. `post`

这样才能真正实现“diag 分散在模块中”，而不是末端二次汇总。

### 7.3 应移除或降级到 debug-only 的旧字段

以下字段不应继续作为 release 常驻字段：

1. `packet`
2. `packetRoute`
3. `animState`
4. `asaMode`
5. `dataType`
6. `processResult`
7. `validJudgmentPassed`
8. `modeExitRelease`
9. `noPressInkActive`
10. `sustainOutput`
11. `fastLiftOutput`
12. `hpp3RatioWarnCountX/Y`
13. `hpp3SignalAvgX/Y`
14. `StylusDiagnostics diag`

其中少数如果仍对调试有价值，应迁入 `StylusDebugFrame`。

---

## 8. `HeatmapFrame` 需要的新语义

### 8.1 允许保留的共有成员

以下结构仍继续保留：

1. `rawPtr / rawLen`
2. `masterSuffix / slaveSuffix`
3. `heatmapMatrix`
4. `contacts`
5. `stylus`
6. `timestamp`
7. `masterWasRead`

### 8.2 建议新增的 stylus debug-only frame 成员

为了与 touch 诊断风格对齐，建议在 `HeatmapFrame` 的 debug 区域补充 stylus 可视化数据，而不是只塞一个大 diag struct：

1. `stylusTx1Peaks`
2. `stylusTx2Peaks`
3. `stylusTx1Projection`
4. `stylusTx2Projection`
5. `stylusStageMap` 或等价阶段标记

这些字段是否直接挂在 `HeatmapFrame`，还是由 `StylusPipeline` 单独缓存，两者都可以；原则是 **按 phase 拆、按用途取，不集中汇总成一个总诊断对象**。

---

## 9. 删除状态机后的新输出语义

### 9.1 删除内容

完整删除以下概念：

1. `PenStateMachine`
2. `ResetFallback`
3. committed-frame reuse
4. terminal commit
5. release keep-previous-coordinate
6. fast down / fast lift / animation state

### 9.2 替代语义

用一个更薄的 `StylusOutputGate` 替代状态机：

1. 输入：坐标有效性、signal metrics、BT pressure、recheck 结果
2. 输出：`inRange`、`tipDown`、`pressure`、本帧是否 suppress、是否保持触摸干扰抑制
3. 可以有轻量跨帧滤波状态
4. 不允许重新长成事件状态机

它的职责是“本帧输出合法化”，不是“抽象一整套笔生命周期”。

### 9.3 手感保持策略

既然保留主要算法而删除状态机，本次重构的手感边界应明确为：

1. 坐标求解质量尽量不变
2. pressure mapping 尽量不变
3. touch suppression 信号输入尽量不变
4. 允许失去旧状态机驱动的特殊落笔/抬笔行为

这是一个用户已接受的行为变化，而不是回归。

---

## 10. BT Pressure 外部源设计

BT pressure 不并入 runtime 主帧采集链，仍允许作为外部源存在。

但为了满足“大帧加工”的要求，进入 `Process(frame)` 后必须先做一步统一快照：

1. `StylusPipeline` 读取最新 BT sample
2. 立即写入 `frame.stylus.input.btSample`
3. 后续所有 phase 只从 `frame` 读取 BT 输入，不直接访问 `BtPressBuffer`

因此：

1. `BtPressBuffer` 可以继续存在
2. 它不再是 phase 模块的共享逻辑对象
3. 它只是 pipeline 外部输入适配器

---

## 11. VHF / Packet 责任重划分

### 11.1 Solver 侧变化

solver 不再产出：

1. `StylusPacket`
2. `StylusPacketRoute`
3. invalid-zero-state packet
4. parse-failure packet

solver 只产出最终 stylus output state。

### 11.2 `VhfReporter` 侧变化

`VhfReporter` 改成像 touch 一样，根据 `frame.stylus.output` 直接组包：

1. solver 输出 `inRange/tipDown/pressure/x/y/tilt`
2. reporter 决定具体 HID bytes
3. parse failure / invalid frame 是否发零态包，也改为 reporter 侧策略

这样 packet 协议和 solver 算法彻底解耦。

### 11.3 影响

这会把当前 `PacketBuilder.hpp` 从 stylus solver 目录中移除，或者至少移出 solver 主链，改为 `VhfReporter` 私有 helper。

---

## 12. Touch Interop 设计

当前 `TouchTracker` 会读取 `frame.stylus` 做 suppression / AFT，因此 stylus 重构必须同步定义一组稳定 interop contract。

新的 contract 原则：

1. touch 只能依赖 `StylusTouchInterop`
2. 不再依赖 `animState`
3. 不再依赖 packet 语义
4. 不再依赖 committed-frame/reuse 概念

`TouchTracker` 如果需要“笔是否活跃”的判断，应使用：

1. `output.inRange`
2. `output.tipDown`
3. `interop.signalX/signalY/maxRawPeak`
4. `interop.recheckPassed`

这让 cross-pipeline 耦合变得显式且稳定。

---

## 13. Diagnostics 模式

### 13.1 目标模式

采用 touch 风格的分散式 diagnostics：

1. 各 phase 自己写自己负责的 debug-only 字段
2. `StylusPipeline` 只维护 thread-safe cache 访问器
3. 不再有全链路总 writer

### 13.2 数据组织

建议按 touch 的模式同时保留两种 debug 视图：

1. **frame-attached debug fields**
   - 供 shared frame / IPC / 调试面板使用
2. **pipeline caches**
   - 供 UI 按需拉取最近一次投影、峰值、阶段摘要

### 13.3 Release 成本

除了必要 interop 字段，所有重调试信息都应进入 `#if EGOTOUCH_DIAG`：

1. TX1/TX2 投影
2. raw anchor
3. revise delta
4. filter internals
5. parser 原始头和 checksum breakdown

这与用户要求的“减少 release 开销”一致。

---

## 14. 文件结构调整

### 14.1 应删除的文件

1. `EGoTouchService/Solvers/StylusSolver/StylusFrameState.hpp`
2. `EGoTouchService/Solvers/StylusSolver/PenStateMachine.hpp`
3. `EGoTouchService/Solvers/StylusSolver/StylusDiagnosticsWriter.hpp`

### 14.2 应迁移职责的文件

1. `PacketBuilder.hpp`
   - 从 solver 责任迁出到 `VhfReporter` 侧，或作为 reporter 私有 helper
2. `BtPressBuffer.hpp`
   - 保留为 pipeline ingress helper，但不再让 phase 读它

### 14.3 应重命名或重构的文件

建议按 touch 风格收敛文件命名：

1. `StylusInputParser.hpp` -> `StylusFrameParser.hpp`
2. `LinearFilter.hpp` + `NoiseGate.hpp` 的 solver 输出侧职责整理为 `StylusCoordinateFilter` / `StylusOutputGate`
3. `EdgeCoorPost` 归并到 edge correction 模块，而不是零散挂在 pipeline 尾部

原则不是机械改名，而是让文件职责更像 touch：名字对应 phase，phase 对应责任。

---

## 15. `StylusPipeline::Process()` 目标形态

目标态入口应接近下面这种结构：

```cpp
bool StylusPipeline::Process(HeatmapFrame& frame) {
    SnapshotBtPressure(frame);

    m_frameParser.Process(frame);
    if (!frame.stylus.input.slaveValid) {
        ResetStylusOutputs(frame);
        UpdateDebugCaches(frame);
        return true;
    }

    m_cmf.Process(frame);
    m_peakDet.Process(frame);
    m_coordSolver.Process(frame);
    m_signalAnalyzer.Process(frame);
    m_pressure.Process(frame);
    m_outputGate.Process(frame);
    m_coordFilter.Process(frame);
    m_coorReviser.Process(frame);
    m_edgeComp.Process(frame);

    UpdateDebugCaches(frame);
    return true;
}
```

关键要求：

1. 不再创建 `state`
2. 不再 commit / finalize
3. 不再 build packet
4. 不再写集中式 diag
5. 不再手工拼大量阶段输入输出

---

## 16. 测试策略

### 16.1 测试重心变化

由于 packet 责任移出 solver，测试要分成两层：

1. **Stylus solver tests**
   - 验证 `frame.stylus` 的最终输出和 interop 字段
2. **VhfReporter stylus packet tests**
   - 验证最终 stylus output 到 HID packet 的映射

### 16.2 solver 侧测试

需要覆盖：

1. parser 有效/无效帧
2. tx1/tx2 peak 与坐标求解
3. pressure sample 映射
4. recheck / overlap / touch suppression 输入
5. coordinate revise / filter / edge correction
6. 删除状态机后的 tipDown / inRange 决策

### 16.3 cross-pipeline 测试

新增或重写一组测试，确认 `TouchTracker` 只依赖新的 stylus interop contract，而不再依赖 `animState` 等已删除字段。

---

## 17. 实施方式

用户已经明确要求“大重构、最终整体架构与 touch 相同”，因此实施方式采用：

1. 单分支破坏式重构
2. 不保留兼容 wrapper
3. 不做 dual-path runtime 切换
4. 分阶段提交，但只在最终全链路可运行后合入

这意味着实现过程可以短暂破坏编译，但最终交付必须一次性收敛。

---

## 18. 风险

### 18.1 高风险

1. `StylusFrameData` 结构收窄会影响 shared-memory / IPC / UI 调试视图
2. `VhfReporter` 需要接管更多 stylus 协议细节
3. 删除状态机会改变现有落笔/抬笔边缘行为

### 18.2 可控风险

1. 大部分核心求解算法可复用
2. touch 已提供了清晰的目标样板
3. touch suppression 现有依赖点已知，可定向重构

---

## 19. 验收标准

重构完成后，应满足以下标准：

1. `StylusPipeline` 的主入口结构与 `TouchPipeline` 同级别
2. 仓库中不存在 `StylusFrameState.hpp`
3. 仓库中不存在 `PenStateMachine.hpp`
4. stylus solver 不再组 packet
5. `VhfReporter` 根据 `frame.stylus` 直接组 stylus packet
6. diagnostics 为 debug-only 分散式结构
7. `TouchTracker` 不再依赖 stylus state machine 字段
8. `StylusFrameData` release 期字段明显收窄

---

## 20. 本设计与现有计划文档的关系

现有 [stylus_pipeline_process_simplification_plan.md](../../stylus_pipeline_process_simplification_plan.md) 的目标是：

1. 保留 stylus 现有语义
2. 用 `StylusFrameState` 精简 orchestrator
3. 继续在 solver 内保留 output / packet / diag 收口

本设计与其方向不同，属于新的主方案。后续如果执行本设计，应把旧计划文档视为历史中间态，而不是继续扩展它。

---

## 21. 下一步

如果本规格确认无误，下一步不是直接写代码，而是生成一份正式 implementation plan，逐项列出：

1. 要删除和改造的文件
2. 数据结构重写顺序
3. `VhfReporter` 重构顺序
4. `TouchTracker` interop contract 改造顺序
5. 测试拆分和验证命令

这份 implementation plan 会单独保存到 `docs/superpowers/plans/`。
