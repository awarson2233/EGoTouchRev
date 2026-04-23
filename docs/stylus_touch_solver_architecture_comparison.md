# StylusSolver 与 TouchSolver 架构对比

依据的主入口与核心状态文件：

- `EGoTouchService/Solvers/StylusSolver/StylusPipeline.h`
- `EGoTouchService/Solvers/StylusSolver/StylusPipeline.cpp`
- `EGoTouchService/Solvers/StylusSolver/StylusFrameState.hpp`
- `EGoTouchService/Solvers/TouchSolver/TouchPipeline.h`
- `EGoTouchService/Solvers/TouchSolver/TouchPipeline.cpp`

## 总体结论

两者都属于单入口、成员模块串行编排的 pipeline 架构，但核心关注点不同：

- **StylusSolver**：更偏向“状态驱动的单目标笔输入管线”，强调帧分类、生命周期控制、压力门控、历史结果复用与最终提交语义。
- **TouchSolver**：更偏向“多目标触点检测与跟踪管线”，强调信号预处理、区域分割、contact 生成、轨迹跟踪和手势输出。

---

## StylusSolver 架构图

```mermaid
flowchart TD
    A[HeatmapFrame] --> B[StylusPipeline::Process]

    B --> S[构造 StylusFrameState]
    S --> P1[StylusInputParser<br/>解析 slave frame / 分类帧状态]

    P1 --> D1{terminal?}
    D1 -- yes --> T1[重置 Post / Noise<br/>ResetFallback / 终止态镜像]
    T1 --> O1[OutputState.CommitTerminal<br/>FinalizeTerminalWithDiagnostics]
    O1 --> R1[StylusFrameData 输出]

    D1 -- no --> C1[CommonModeFilter]
    C1 --> C2[GridPeakDetector]
    C2 --> C3[CoordinateSolver]
    C3 --> C4[NoiseGate.ProcessJump]

    C4 --> S1[读取 BT pressure sample<br/>判断 previouslyWriting]
    S1 --> S2[StylusSignalAnalyzer<br/>首次分析]
    S2 --> S3[NoiseGate.Process]
    S3 --> S4[StylusStateController<br/>+ PressureSolver<br/>+ PenStateMachine]

    S4 --> D2{reusedCommittedFrame?}
    D2 -- yes --> O2[OutputState.ReuseCommittedFrame<br/>FinalizeWithDiagnostics]
    O2 --> R1

    D2 -- no --> S5[StylusSignalAnalyzer<br/>按 currentlyWriting 二次分析]
    S5 --> D3{enableCoorReviser?}
    D3 -- yes --> S6[SignalRatioTracker]
    D3 -- no --> P2[跳过]

    S6 --> P3[CoorPostProcessor]
    P2 --> P3

    P3 --> P4[LinearFilter]
    P3 --> P5[CoorReviser]
    P3 --> P6[NoiseGate / OutputState 协同]

    P4 --> P7[StylusDiagnosticsWriter]
    P5 --> P7
    P6 --> P7

    P7 --> O3[EdgeCoorPost<br/>OutputState.CommitFinal]
    O3 --> R1
```

### StylusSolver 结构特点

1. **以 `StylusFrameState` 为核心中间态**  
   将整条链路拆为 `flow`、`parse`、`tx1/tx2`、`signal`、`lifecycle`、`output` 等子状态域，模块围绕它读写。

2. **前段先做帧分类和路由决策**  
   `StylusInputParser` 会先区分 `Valid`、`ShortFrame`、`NoSignal`、`ParseFail`、`Tx1Missing`，并直接驱动 terminal / reset / packetRoute。

3. **输出有提交语义**  
   `OutputState` 负责 `BeginFrame`、`CommitFinal`、`CommitTerminal`、`ReuseCommittedFrame`、`Finalize`，支持历史有效结果复用。

4. **核心在 pen lifecycle，而不是 contact extraction**  
   中段重点是 `StylusSignalAnalyzer`、`StylusStateController`、`PressureSolver`、`PenStateMachine` 这一组状态控制模块。

---

## TouchSolver 架构图

```mermaid
flowchart TD
    A[HeatmapFrame] --> B[TouchPipeline::Process]

    B --> P1[MasterFrameParser]
    P1 --> D1{当前有 finger?<br/>或已有 live track/state?}

    D1 -- no --> I1[ResetIdleOutputs]
    I1 --> R1[清空 contacts / touchPackets / diag cache]

    D1 -- yes --> C1[BaselineSubtraction]
    C1 --> C2[CMFProcessor]
    C2 --> C3[GridIIRProcessor]

    C3 --> F1[MacroZoneDetector<br/>连通域]
    F1 --> F2[PalmRejector<br/>去除大面积/细长区]
    F2 --> F3[PeakDetector<br/>局部峰值]
    F3 --> F4[MicroZoneSegmenter<br/>细分 zone]

    F4 --> Z1[ZoneExpander<br/>从 peak 扩张为 contact]
    Z1 --> Z2[EdgeCompensator]
    Z2 --> Z3[TouchSizeCalculator]
    Z3 --> Z4[EdgeRejector]

    Z4 --> D2[更新 cached counts<br/>/ diag zones / peaks]
    D2 --> T1[TouchTracker]
    T1 --> T2[CoordinateFilter]
    T2 --> G1[TouchGestureStateMachine]

    G1 --> R2[frame.contacts / touchPackets / gesture 输出]
```

### TouchSolver 结构特点

1. **典型分阶段线性流水线**  
   `TouchPipeline` 明确分成 Parsing、Conditioning、Feature Extraction、Zone & Contact、Tracking、Gesture 六个阶段。

2. **直接围绕 `HeatmapFrame` 和 `contacts` 工作**  
   没有像 Stylus 那样单独抽出大型帧级状态对象，而是在 `frame` 上逐步加工中间结果和输出结果。

3. **中心问题是多触点生成与跟踪**  
   主要模块链路是 MacroZone → PalmReject → Peak → MicroZone → ZoneExpand → Track。

4. **诊断缓存职责更靠近处理中段**  
   pipeline 内直接维护 peaks / touchZones / zoneEdge 等缓存，并通过 mutex / atomic 提供线程安全访问。

---

## 并排对比图

```mermaid
flowchart LR
    subgraph StylusSolver
        S1[InputParser]
        S2[CMF + PeakDetector]
        S3[CoordinateSolver]
        S4[SignalAnalyzer]
        S5[StateController<br/>+ PressureSolver<br/>+ PenStateMachine]
        S6[PostProcessor<br/>+ LinearFilter<br/>+ CoorReviser]
        S7[OutputState / Diagnostics]
        S1 --> S2 --> S3 --> S4 --> S5 --> S6 --> S7
    end

    subgraph TouchSolver
        T1[MasterFrameParser]
        T2[Baseline + CMF + GridIIR]
        T3[MacroZone + PalmReject + PeakDetector + MicroZoneSeg]
        T4[ZoneExpander + EdgeComp + TouchSize + EdgeReject]
        T5[TouchTracker + CoordinateFilter]
        T6[GestureStateMachine + Diag Cache]
        T1 --> T2 --> T3 --> T4 --> T5 --> T6
    end
```

---

## 关键架构差异

### 1. 数据模型不同

- **StylusSolver**：`StylusFrameState` 是核心中间模型。
- **TouchSolver**：以 `HeatmapFrame` / `contacts` 为中心进行就地加工。

### 2. 处理目标不同

- **StylusSolver**：单笔尖、单结果、连续性优先。
- **TouchSolver**：多触点、多区域、多目标关联优先。

### 3. 控制方式不同

- **StylusSolver**：更强的状态机与分支控制，包含 terminal、reuseCommittedFrame、pressure gate、release 处理。
- **TouchSolver**：更强的线性 phase 控制，绝大多数路径沿固定顺序推进。

### 4. 输出语义不同

- **StylusSolver**：输出带有 commit / reuse / terminal 语义。
- **TouchSolver**：输出更接近“本帧 contact 检测 + 轨迹跟踪 + gesture 状态”。

### 5. 诊断职责位置不同

- **StylusSolver**：诊断更偏后处理末端汇总。
- **TouchSolver**：诊断更偏处理中段缓存和可视化支持。

---

## 一句话总结

- **StylusSolver** 是一个以状态机和输出连续性为核心的单笔输入控制管线。
- **TouchSolver** 是一个以区域分割、峰值提取、contact 生成和多目标跟踪为核心的多点触控检测管线。
