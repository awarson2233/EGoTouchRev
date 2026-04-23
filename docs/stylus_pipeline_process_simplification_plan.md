# StylusPipeline 精简重构实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` (recommended) or `superpowers:executing-plans` to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.
>
> 日期：2026-04-23  
> 范围：`EGoTouchService/Solvers/StylusSolver/**`  
> 目标文件：`EGoTouchService/Solvers/StylusSolver/StylusPipeline.cpp`

**Goal:** 让 `StylusPipeline::Process()` 退化为和 `TouchPipeline::Process()` 同级别的线性编排函数，只保留阶段顺序调用、config 体系和上位机通信相关职责，不再持有热路径中的中间函数、`switch` 判定和大批局部阶段变量。

**Architecture:** 引入一个仅表示“当前帧临时工作区”的 `StylusFrameState`，由各个 stylus 模块通过统一的 `Process(StylusFrameState& state)` 接口顺序处理。跨帧持久状态仍保留在模块成员中，单帧中间结果统一放在 `StylusFrameState` 中显式传递，不做 `StylusPipeline` 共享隐式变量，也不再引入 `StylusEngine`、`StylusLifecycleReset`、`StylusConfigRegistry` 一类中间层。

**Tech Stack:** C++20、现有 header-only stylus 模块、`HeatmapFrame` / `StylusFrameData`、`StylusPipelineFastInkTest`、`StylusPipelineModulesTest`

---

## 1. 目标边界

### 1.1 本次必须做到

1. `StylusPipeline::Process()` 只保留阶段顺排，不再直接拼装复杂阶段输入输出。
2. 所有热路径模块都以统一的 `Process(state)` 形式暴露给 pipeline。
3. 消灭 `Process()` 中的局部 `switch(parseRes.frameClass)`。
4. 消灭 `Process()` 中的局部 `finalizeFrame` / `commitInvalid` lambda。
5. 消灭 `Process()` 中的阶段性大批局部变量，统一收敛到 `StylusFrameState`。
6. 保留 `StylusPipeline` 的 config 接口：`GetConfigSchema` / `SaveConfig` / `LoadConfig` 继续留在 pipeline，保持和 `TouchPipeline` 同级别的职责形态。

### 1.2 本次明确不做

1. 不引入新的总控类，例如 `StylusEngine`、`StylusLifecycleReset`、`StylusPostProcessStage`。
2. 不把本帧状态做成 `StylusPipeline` 的共享成员变量，也不做 thread-local / 全局隐式传输。
3. 不把 config 系统整体搬出 `StylusPipeline`。
4. 不做与本目标无关的算法语义重写，例如 pressure 公式、tilt 公式、packet 协议变化。

---

## 2. 当前热点与必须清理的内容

### 2.1 当前 `Process()` 中的三类问题

1. **控制流杂质**
   - `finalizeFrame` lambda
   - `commitInvalid` lambda
   - `switch(parseRes.frameClass)`
   - 多处 `return finalizeFrame()`

2. **阶段中间变量过多**
   - 解析阶段：`rawData`、`parseRes`
   - 信号阶段：`gridData`、`preCmfTx1Grid`、`tx1Analysis`、`tx2Analysis`、`peak`、`tx2Peak`
   - 坐标阶段：`rawCoor`、`tx2Coor`
   - 状态阶段：`btSample`、`recheckCtxBeforeState`、`recheckPassed`、`stateOutput`、`pressureStage`、`evidence`、`penUpdate`
   - 后处理阶段：`postCoor`、`finalCoor`、`snappedX`、`snappedY`
   - 诊断阶段：`diagCtx`

3. **pipeline 绕过模块统一入口**
   - `StylusInputParser.Parse(...)`
   - `CommonModeFilter.Apply(...)`
   - `GridPeakDetector.AnalyzePeakAndProjection(...)`
   - `CoordinateSolver.Solve(...)`
   - `CoorReviser.Revise(...)`
   - `CoorPostProcessor.StepIIR(...)`
   - `CoorPostProcessor.StepJitter(...)`
   - `PacketBuilder.BuildPacket(...)`

### 2.2 当前局部变量的去向清单

| 当前局部变量 / 控制块 | 目标去向 | 说明 |
|---|---|---|
| `finalizeFrame` | `StylusFrameCommitter` | 由 committer 统一负责最后一次 `GetLastResult` / diag 回填 / 返回值语义 |
| `commitInvalid` | `StylusInputParser` + `StylusFrameCommitter` + `PacketBuilder` | parser 负责给出终止流向，committer 负责 invalid commit，packet builder 负责 packet kind |
| `switch(parseRes.frameClass)` | `StylusInputParser::Process(state)` | parser 直接写入 `state.flow` |
| `rawData` / `parseRes` | `StylusFrameState::parse` | 只在 parser 内部和 state 中存在 |
| `gridData` / `preCmfTx1Grid` | `StylusFrameState::tx1/tx2` | pipeline 不再直接复制 grid |
| `tx1Analysis` / `tx2Analysis` / `peak` / `tx2Peak` | `GridPeakDetector::Process(state)` | detector 内部计算并写回 state |
| `rawCoor` / `tx2Coor` | `CoordinateSolver::Process(state)` | solver 负责 local->global 与 tx1/tx2 point 回填 |
| `metrics` / `recheckCtx*` | `StylusSignalAnalyzer::Process(state)` | signal 模块统一产出 |
| `btSample` / `stateOutput` / `pressureStage` / `evidence` / `penUpdate` | `StylusStateController::Process(state, ...)` | pipeline 不再拼状态机输入 |
| `postCoor` / `finalCoor` / `snappedX/Y` | `CoorPostProcessor::Process(state, ...)` | 后处理链收口 |
| `diagCtx` | `StylusDiagnosticsWriter::Process(state, ...)` | 诊断写入模块自己从 state 取数 |

---

## 3. 目标形态

### 3.1 `StylusPipeline::Process()` 的目标形态

最终应接近下面这种长度和复杂度：

```cpp
bool StylusPipeline::Process(HeatmapFrame& frame) {
    StylusFrameState state(frame, m_sensorRows, m_sensorCols, m_anchorCenterOffset);

    SyncPipelineConfig();
    m_output.BeginFrame(state);
    m_diagnostics.Reset();

    m_inputParser.Process(state, m_enableSlaveChecksum);
    if (state.flow.terminal) {
        return m_output.Finalize(state, m_packets, m_diagnostics);
    }

    m_cmfFilter.Process(state);
    m_peakDetector.Process(state);
    m_coordSolver.Process(state, m_pitchMapEnabled, m_pitchTableDim1, m_pitchTableDim2);
    m_signalAnalyzer.Process(state, m_recheckThBase, m_recheckThMulti);
    m_penState.Process(state, m_btPressBuf, m_pressureSolver, m_penStateMachine, m_output, m_noiseGate);
    if (state.flow.terminal) {
        return m_output.Finalize(state, m_packets, m_diagnostics);
    }

    m_postProcessor.Process(state, m_linearFilter, m_coorReviser, m_noiseGate,
                            m_coordSolver, m_signalRatioTracker, m_output);
    m_diagnostics.Process(state, m_penStateMachine, m_linearFilter,
                          m_coorReviser, m_cmfFilter, m_signalRatioTracker);
    m_packets.Process(state.stylus, state.flow.packetKind, state.stylus.packet);
    m_output.CommitFinal(state);
    return m_output.Finalize(state, m_packets, m_diagnostics);
}
```

### 3.2 `StylusFrameState` 命名与定位

- 类型名使用 `StylusFrameState`
- 局部变量名使用 `state`
- 语义：当前这一帧的临时工作区
- 不是跨帧状态 owner
- 不是全局 context
- 不是 pipeline 隐式共享成员

### 3.3 数据流示意

```mermaid
flowchart LR
    A[StylusInputParser] --> B[CommonModeFilter]
    B --> C[GridPeakDetector]
    C --> D[CoordinateSolver]
    D --> E[StylusSignalAnalyzer]
    E --> F[StylusStateController]
    F --> G[CoorPostProcessor]
    G --> H[StylusDiagnosticsWriter]
    H --> I[PacketBuilder]
    I --> J[StylusFrameCommitter]
```

---

## 4. 文件结构与职责冻结

### 4.1 新增文件

- Create: `EGoTouchService/Solvers/StylusSolver/StylusFrameState.hpp`

职责：
- 定义 `StylusFrameState`
- 定义终止流控字段，例如 `terminal`、`reuseCommittedFrame`、`packetKind`
- 定义 TX1/TX2 / signal / lifecycle / output 这些按阶段聚合的单帧临时数据

### 4.2 继续保留在 `StylusPipeline` 的职责

- `Process()` 中的顺序编排
- `SetBtMcuPressure()`
- `GetLastResult()`
- `GetDebugCoord()`
- `GetFilterMode()` / `SetFilterMode()`
- `GetConfigSchema()` / `SaveConfig()` / `LoadConfig()`

### 4.3 必须下沉到模块的职责

- 解析终止分支
- invalid packet / invalid commit 语义
- TX1/TX2 分析结果拼装
- recheck 计算
- pressure gate / pressure stage / pen evidence 组装
- linear + revise + iir + jitter + exit snap 串联
- diagnostics context 拼装

---

## 5. 实施任务

### Task 1: 建立 `StylusFrameState`，切断 `Process()` 的局部变量膨胀

**Files:**
- Create: `EGoTouchService/Solvers/StylusSolver/StylusFrameState.hpp`
- Modify: `EGoTouchService/Solvers/StylusSolver/StylusPipeline.cpp`
- Modify: `EGoTouchService/Solvers/StylusSolver/StylusPipeline.h`
- Test: `Tools/tests/StylusPipelineModulesTest.cpp`

- [ ] 新建 `StylusFrameState`，按阶段组织字段，而不是做一个平铺巨型 struct。
- [ ] 在 `StylusFrameState` 中保留对 `HeatmapFrame&` 和 `StylusFrameData&` 的引用，避免 pipeline 中重复写 `auto& stylus = frame.stylus`。
- [ ] 增加 `flow` 子结构，至少包含：`terminal`、`clearCommitted`、`resetPost`、`resetNoise`、`pipelineStage`、`packetKind`、`reuseCommittedFrame`。
- [ ] 在 `StylusPipeline::Process()` 中只保留一个本地对象：`StylusFrameState state{...};`
- [ ] 删除 `finalizeFrame` lambda。
- [ ] 删除 `commitInvalid` lambda。
- [ ] 保留 `SyncPipelineConfig()` 一类的轻量初始化，负责同步 `sensorRows/sensorCols` 到相关模块。
- [ ] 静态检查：确认 `StylusPipeline.cpp` 中不再出现 `const auto finalizeFrame =` 和 `const auto commitInvalid =`。

### Task 2: 让 parser 接管 `switch` 与无效帧终止流

**Files:**
- Modify: `EGoTouchService/Solvers/StylusSolver/StylusInputParser.hpp`
- Modify: `EGoTouchService/Solvers/StylusSolver/StylusFrameCommitter.hpp`
- Modify: `EGoTouchService/Solvers/StylusSolver/PacketBuilder.hpp`
- Modify: `EGoTouchService/Solvers/StylusSolver/StylusPipeline.cpp`
- Test: `Tools/tests/StylusPipelineModulesTest.cpp`

- [ ] 为 `StylusInputParser` 新增 `Process(StylusFrameState& state, bool enableSlaveChecksum)`。
- [ ] 在 parser 内部完成 raw-length 检查、checksum、`frameClass` 判定、`slaveValid/status/checksum/slaveWordOffset` 回填。
- [ ] parser 不再只返回 `ParseResult`；它还必须负责设置 `state.flow`。
- [ ] `ShortFrame` / `ParseFail` / `NoSignal` / `Tx1Missing` 的 packet kind 与 reset 策略在 parser 阶段直接写入 `state.flow`。
- [ ] `StylusPipeline::Process()` 中删除 `switch(parseRes.frameClass)`。
- [ ] `StylusFrameCommitter` 新增统一的终止收口接口，用来处理 invalid commit 与最后返回值。
- [ ] `PacketBuilder` 统一走 `Process(...)` 接口，pipeline 不再直接调用 `BuildPacket` / `BuildParseFailurePacket`。
- [ ] 静态检查：确认 `StylusPipeline.cpp` 不再出现 `switch (parseRes.frameClass)`。

### Task 3: 把信号提取链收拢成真正的模块顺排

**Files:**
- Modify: `EGoTouchService/Solvers/StylusSolver/CommonModeFilter.hpp`
- Modify: `EGoTouchService/Solvers/StylusSolver/GridPeakDetector.hpp`
- Modify: `EGoTouchService/Solvers/StylusSolver/CoordinateSolver.hpp`
- Modify: `EGoTouchService/Solvers/StylusSolver/StylusSignalAnalyzer.hpp`
- Modify: `EGoTouchService/Solvers/StylusSolver/StylusPipeline.cpp`
- Test: `Tools/tests/StylusPipelineModulesTest.cpp`
- Test: `Tools/tests/StylusPipelineFastInkTest.cpp`

- [ ] `CommonModeFilter` 增加 `Process(state)`，由模块自己处理 TX1/TX2 grid，而不是 pipeline 直接 `Apply(...)`。
- [ ] 处理 pre-CMF TX1 composite 需求，避免 pipeline 保留 `preCmfTx1Grid` 这种整块临时副本。
- [ ] `GridPeakDetector` 增加 `Process(state)`，产出 TX1/TX2 `analysis`、`peakSignal`，并在 TX1 peak 无效时直接终止本帧。
- [ ] `CoordinateSolver` 增加 `Process(state, ...)`，把 `Solve + pitch map + local to global` 一次收口。
- [ ] `CoordinateSolver` 在处理 TX1/TX2 时，直接回填 `stylus.point.tx1X/tx1Y/tx2X/tx2Y`。
- [ ] `StylusSignalAnalyzer` 增加 `Process(state, baseThreshold, multiThreshold)`，统一产出 `metrics` 与 `recheck`，并直接回填 `stylus.signalX/signalY/maxRawPeak/peakTx1/peakTx2/recheck*`。
- [ ] `StylusPipeline.cpp` 中删除 `gridData`、`tx1Analysis`、`tx2Analysis`、`peak`、`tx1PeakSignal`、`tx2PeakSignal`、`metrics` 这些散落局部变量。

### Task 4: 把状态机与 pressure 热路径整体收进 `StylusStateController`

**Files:**
- Modify: `EGoTouchService/Solvers/StylusSolver/StylusStateController.hpp`
- Modify: `EGoTouchService/Solvers/StylusSolver/PressureSolver.hpp`
- Modify: `EGoTouchService/Solvers/StylusSolver/PenStateMachine.hpp`
- Modify: `EGoTouchService/Solvers/StylusSolver/NoiseGate.hpp`
- Modify: `EGoTouchService/Solvers/StylusSolver/StylusPipeline.cpp`
- Test: `Tools/tests/StylusPipelineFastInkTest.cpp`
- Test: `Tools/tests/StylusPipelineModulesTest.cpp`

- [ ] `StylusStateController::Process(...)` 接管 `btSample` 获取、`previouslyWriting` 判断、pressure gate、pressure solve、`PenFrameEvidence` 拼装、`PenStateMachine` 调用。
- [ ] `StylusStateController` 直接写回 `state.lifecycle`，包括 `pressureResult`、`penUpdate`、`mappedPressure/outputPressure/tipSwitchActive`。
- [ ] `suspiciousDrop + ReuseCommittedFrame` 判断下沉到 `StylusStateController` 与 `StylusFrameCommitter` 的协作接口，不再放在 pipeline。
- [ ] `JustLeftRange()` 触发的 `ClosePressureGate()` 与 `NoiseGate.Reset()` 也下沉到这一阶段。
- [ ] `NoiseGate` 的职责收窄：只保留 coordinate jump 和 exit-edge snap；recheck 逻辑不再由 pipeline 直接驱动。
- [ ] `StylusPipeline.cpp` 中删除 `btSample`、`recheckCtxBeforeState`、`recheckPassed`、`stateOutput`、`pressureStage`、`evidence`、`penUpdate` 局部变量。

### Task 5: 收口后处理链，消灭 `postCoor/finalCoor/diagCtx`

**Files:**
- Modify: `EGoTouchService/Solvers/StylusSolver/CoorReviser.hpp`
- Modify: `EGoTouchService/Solvers/StylusSolver/CoorPostProcessor.hpp`
- Modify: `EGoTouchService/Solvers/StylusSolver/NoiseGate.hpp`
- Modify: `EGoTouchService/Solvers/StylusSolver/StylusDiagnosticsWriter.hpp`
- Modify: `EGoTouchService/Solvers/StylusSolver/StylusFrameCommitter.hpp`
- Modify: `EGoTouchService/Solvers/StylusSolver/PacketBuilder.hpp`
- Modify: `EGoTouchService/Solvers/StylusSolver/StylusPipeline.cpp`
- Test: `Tools/tests/StylusPipelineFastInkTest.cpp`

- [ ] `CoorPostProcessor` 新增真正的 `Process(state, ...)`，接管 `LinearFilter -> optional TX2 revise -> IIR -> Jitter -> final coordinate resolve -> exit snap`。
- [ ] `CoorReviser` 继续保留 tilt/revise 算法，但通过 `Process(state, ...)` 参与，不再由 pipeline 手动准备 `tiltX/tiltY/tx2Coor`。
- [ ] `StylusDiagnosticsWriter` 新增 `Process(state, ...)`，直接从 `state` 和相关模块取值，消灭 `StylusPipeline.cpp` 中的 `diagCtx`。
- [ ] `StylusFrameCommitter` 新增 `Finalize(...)` 或等价接口，统一处理最终结果回写、diag 注入与返回值语义。
- [ ] `StylusPipeline.cpp` 中删除 `postCoor`、`tx2Coor`、`finalCoor`、`diagCtx`、`snappedX`、`snappedY` 局部变量。
- [ ] `StylusPipeline.cpp` 热路径收缩为“创建 state + 模块顺排 + terminal 检查 + final finalize”。

### Task 6: 保持 config 责任在 pipeline，但按模块顺序重排

**Files:**
- Modify: `EGoTouchService/Solvers/StylusSolver/StylusPipeline.cpp`
- Modify: `EGoTouchService/Solvers/StylusSolver/StylusPipeline.h`
- Test: `Tools/tests/StylusPipelineModulesTest.cpp`

- [ ] `GetConfigSchema()` 继续留在 `StylusPipeline`，但按解析、信号、状态、后处理、输出的顺序分段整理。
- [ ] `SaveConfig()` / `LoadConfig()` 继续留在 `StylusPipeline`，不单独抽 `StylusConfigRegistry`。
- [ ] 将 config 字段与模块实际 ownership 对齐，避免因为热路径下沉导致配置项漂移。
- [ ] 静态检查：确认没有新增 `StylusEngine`、`StylusLifecycleReset`、`StylusPostProcessStage`、`StylusConfigRegistry` 文件。

---

## 6. 逐文件改动清单

| 文件 | 改动级别 | 主要改动 |
|---|---|---|
| `EGoTouchService/Solvers/StylusSolver/StylusFrameState.hpp` | 新增 | 定义单帧状态与 flow |
| `EGoTouchService/Solvers/StylusSolver/StylusPipeline.cpp` | 高 | 热路径缩减为模块顺排 |
| `EGoTouchService/Solvers/StylusSolver/StylusPipeline.h` | 中 | 引入新 state 相关声明与轻量同步接口 |
| `EGoTouchService/Solvers/StylusSolver/StylusInputParser.hpp` | 高 | 接管无效帧终止流 |
| `EGoTouchService/Solvers/StylusSolver/CommonModeFilter.hpp` | 中 | 增加 `Process(state)` |
| `EGoTouchService/Solvers/StylusSolver/GridPeakDetector.hpp` | 高 | 接管 TX1/TX2 分析结果 |
| `EGoTouchService/Solvers/StylusSolver/CoordinateSolver.hpp` | 高 | 接管 global coordinate 变换 |
| `EGoTouchService/Solvers/StylusSolver/StylusSignalAnalyzer.hpp` | 高 | 接管 metrics + recheck |
| `EGoTouchService/Solvers/StylusSolver/StylusStateController.hpp` | 高 | 接管 pressure/state/pen update 热路径 |
| `EGoTouchService/Solvers/StylusSolver/PressureSolver.hpp` | 中 | 配合 state controller 改接口 |
| `EGoTouchService/Solvers/StylusSolver/PenStateMachine.hpp` | 中 | 配合新的单帧输入组织 |
| `EGoTouchService/Solvers/StylusSolver/NoiseGate.hpp` | 中 | 收窄为 jump + exit snap |
| `EGoTouchService/Solvers/StylusSolver/CoorReviser.hpp` | 中 | 参与统一后处理接口 |
| `EGoTouchService/Solvers/StylusSolver/CoorPostProcessor.hpp` | 高 | 接管后处理收口 |
| `EGoTouchService/Solvers/StylusSolver/StylusDiagnosticsWriter.hpp` | 高 | 消灭 `diagCtx` |
| `EGoTouchService/Solvers/StylusSolver/StylusFrameCommitter.hpp` | 高 | 接管 finalize 与终止收口 |
| `EGoTouchService/Solvers/StylusSolver/PacketBuilder.hpp` | 中 | 统一 `Process(...)` 出口 |

---

## 7. 验收清单

- [ ] `StylusPipeline.cpp` 中不再出现局部 `switch(parseRes.frameClass)`
- [ ] `StylusPipeline.cpp` 中不再出现 `finalizeFrame` / `commitInvalid` lambda
- [ ] `StylusPipeline.cpp` 中不再直接调用 `Apply` / `AnalyzePeakAndProjection` / `Solve` / `Revise` / `StepIIR` / `StepJitter` / `BuildPacket`
- [ ] `StylusPipeline::Process()` 中的局部变量数量显著下降，保留的局部变量原则上不超过 `state` 和极少量只读别名
- [ ] 所有热路径模块都具备面向 pipeline 的 `Process(StylusFrameState& state)` 入口
- [ ] 没有新增 `StylusEngine` / registry / lifecycle wrapper 等中间层
- [ ] config 仍保留在 `StylusPipeline`
- [ ] `StylusPipelineFastInkTest` 语义保持通过
- [ ] `StylusPipelineModulesTest` 能覆盖新接口与 flow 语义

---

## 8. 建议验证方式

> 说明：当前环境中不建议由 agent 直接长时间跑 `cmake` / `ninja`。以下命令作为实现后的建议验证路径，优先由用户侧或单独验证回合执行。

1. 编译指定测试目标
   - `cmake --build build --target StylusPipelineFastInkTest`
   - `cmake --build build --target StylusPipelineModulesTest`

2. 运行指定测试
   - `ctest --test-dir build --output-on-failure -R StylusPipelineFastInkTest`
   - `ctest --test-dir build --output-on-failure -R StylusPipelineModulesTest`

3. 静态回归检查
   - 搜索 `StylusPipeline.cpp`，确认不再出现 `switch (parseRes.frameClass)`、`finalizeFrame`、`commitInvalid`
   - 搜索 stylus 模块头，确认都已提供统一的 `Process(StylusFrameState& state)` 入口

---

## 9. 自检

### 9.1 覆盖性

- 已覆盖：`switch` 消除
- 已覆盖：中间变量收敛
- 已覆盖：模块统一 `Process(state)` 接口
- 已覆盖：`StylusPipeline` 只保留顺排 + config + IPC 职责
- 已覆盖：不使用隐式共享状态
- 已覆盖：不新增中间总控层

### 9.2 Placeholder 检查

- 文档中未使用 `TBD` / `TODO` / “后续补充” 一类占位表达
- 每个任务都给出了具体文件路径和改动方向

### 9.3 一致性

- 全文统一使用 `StylusFrameState`
- 全文统一要求显式 `Process(StylusFrameState& state)` 传递
- 全文统一保留 config 在 `StylusPipeline`

---

## 10. 实施进度 / 并发拆分 / 当前剩余项

> 说明：本节只记录当前仓库中已经能直接看到的真实进展。这里的“已完成”仅表示模块接口或测试文件已经落地，不表示 `StylusPipeline.cpp` 主线已经完成切换。

### 10.1 当前已落地的模块下沉项

以下项已经能在仓库中直接看到代码落地：

1. `StylusFrameState.hpp` 已存在，并且已经拆出 `flow / parse / tx1 / tx2 / signal / lifecycle / output` 等单帧子状态。
2. `StylusInputParser.hpp` 已新增 `Process(StylusFrameState& state, bool enableSlaveChecksum)`，并且会把 parse 结果与 `frameClass -> flow.packetRoute/terminal/reset*` 写回 `state`。
3. `CommonModeFilter.hpp` 已新增 `Process(state)`，并缓存 pre-CMF TX1 grid，开始把 `preCmfTx1Grid` 这类临时数据往模块内部收。
4. `GridPeakDetector.hpp` 已新增 `Process(state)`，并在 TX1 peak 无效时直接写 `state.flow.terminal` 与 invalid route。
5. `CoordinateSolver.hpp` 已新增 `Process(state, ...)`，会把 TX1/TX2 的 local/global coordinate 与 `stylus.point.tx1X/tx1Y/tx2X/tx2Y` 回填到 state/stylus。
6. `StylusSignalAnalyzer.hpp` 已新增 `Process(state, ...)`，已经可以把 metrics/recheck 结果统一写回 `state.signal` 与 `stylus`。
7. `PressureSolver.hpp` 已新增 `Process(state, ...)` 与 `ApplyStageToState(...)`，pressure stage 结果已经可以直接回填 `state.lifecycle`。
8. `PenStateMachine.hpp` 已支持 `Process(state, evidence)` / `ApplyUpdateToState(...)`，motion/output 已经可以回填到 `state.lifecycle`。
9. `StylusStateController.hpp` 已新增 `Process(state, config, pressureSolver, penStateMachine)`，并能处理 gate、pressure、evidence、pen update 这一整段 state 写回。
10. `NoiseGate.hpp` 已支持 `Process(state)`、`ProcessExitSnap(state, ctx)` 与 `StylusExitSnapContext`，exit snap 所需上下文已经有模块侧承载。
11. `CoorPostProcessor.hpp` 已新增 `Process(state, ...)`，并能串起 linear filter、coor reviser、IIR、jitter、final coordinate resolve、exit snap。
12. `StylusDiagnosticsWriter.hpp` 已新增 `Process(state, ...)` 和 `BuildContext(state, ...)`，`diagCtx` 的模块收口接口已经存在。
13. `PacketBuilder.hpp` 已新增面向 `StylusFrameState` / `StylusPacketRoute` 的 `Build(...)` / `Process(...)` 入口，route 已能直接驱动 packet 输出。
14. `StylusFrameCommitter.hpp` 已新增 `BeginFrame(state)`、`FinalizeTerminal(...)`、`FinalizeFinal(...)` 等辅助接口，terminal/final output 收口接口已经存在。
15. `Tools/tests/StylusPipelineModulesTest.cpp` 已存在，当前已覆盖 parser、packet builder、committer 的一部分模块级行为；`Tools/tests/StylusPipelineFastInkTest.cpp` 也仍在仓库内。

### 10.2 `StylusPipeline.cpp` 当前仍待删除的中间变量、控制流块、局部 lambda

当前 `EGoTouchService/Solvers/StylusSolver/StylusPipeline.cpp` 已经完成主线切换，下面这些旧主线杂质已被清掉：

1. 已删除局部 lambda：
   - `finalizeFrame`
   - `commitInvalid`
2. 已删除 parser 后本地 `switch (parseRes.frameClass)`，统一改成 `state.flow.terminal / packetRoute / reset*` 驱动。
3. 已删除旧顶层别名和大批阶段局部变量：
   - `auto& stylus`
   - `rawData`
   - `parseRes`
   - `gridData`
   - `preCmfTx1Grid`
   - `tx1Analysis`
   - `peak`
   - `tx1PeakSignal`
   - `tx2PeakSignal`
   - `tx2Analysis`
   - `tx2Peak`
   - `metrics`
   - `btSample`
   - `recheckCtxBeforeState`
   - `recheckPassed`
   - `stateOutput`
   - `isEdge`
   - `edgeSignals`
   - `pressureStage`
   - `evidence`
   - `penUpdate`
   - `currentlyWriting`
   - `recheckCtx`
   - `postCoor`
   - `tx2Coor`
   - `tiltX`
   - `tiltY`
   - `finalCoor`
   - `snappedX`
   - `snappedY`
   - `diagCtx`
4. 主线已切到以下 state 接口顺排：
   - `m_inputParser.Process(state, ...)`
   - `m_cmfFilter.Process(state)`
   - `m_peakDetector.Process(state)`
   - `m_coordSolver.Process(state, ...)`
   - `m_noiseGate.ProcessJump(state)` / `m_noiseGate.Process(state)`
   - `m_signalAnalyzer.Process(state, ...)`
   - `m_penState.Process(state, ...)`
   - `m_postProcessor.Process(state, ...)`
   - `m_diagnostics.Process(state, ...)`
   - `m_packets.Process(state)`
   - `m_output.FinalizeTerminal* / FinalizeFinal*`

当前仍保留在主线中的显式编排项只剩 orchestration 级内容：

1. 终止帧时对 `post/noise` 模块的 reset 协调。
2. `m_emitPacketWhenInvalid` 的配置分支。
3. `m_edgeCoorPost.Apply(...)` 这一条独立后处理调用。
4. 将 `state.output.finalCoor` 镜像回 `stylus.point` 后再生成 packet 的收口顺序。

### 10.3 当前并发拆分的 3 条工作线

#### A. 主线 orchestrator

目标：只改 `StylusPipeline.cpp` 主线编排，把“模块已支持 state 接口”的中间态切换成真正的顺排 orchestrator。

当前状态：已完成。

本轮已落地：

1. 用 `StylusFrameState state(...)` 替代了原来的 `auto& stylus` + 大批阶段局部变量。
2. parser 终止、TX1 peak invalid、坐标 invalid、noise invalid、reuse committed 已统一改成 `state.flow` 驱动。
3. `finalizeFrame` / `commitInvalid` lambda 已删除。
4. `switch(parseRes.frameClass)` 与多处 `return finalizeFrame()` 已被模块级 `Process(state)` / `Finalize*` 接口替换。
5. 最终 packet/commit/diag 路径已经改成统一 output tail。

#### B. Tail 模块

目标：把状态机之后的“尾段输出链”收口到模块接口里，保证主线只保留顺序调用。

当前状态：主体已完成，仍有 1 个轻尾巴。

本轮已落地：

1. `CoorPostProcessor`、`NoiseGate`、`StylusDiagnosticsWriter`、`PacketBuilder`、`StylusFrameCommitter` 的 state 接口已经真正接入主线。
2. `postCoor/finalCoor/diagCtx/snappedX/snappedY` 的主线手工拼装已删除。
3. 尾段已经收敛成：
   - `m_postProcessor.Process(state, ...)`
   - `m_diagnostics.Process(state, ...)`
   - `m_packets.Process(state)`
   - `m_output.FinalizeFinalWithDiagnostics(state, ...)`

当前仍显式留在主线中的 tail 只有：

1. `m_edgeCoorPost.Apply(...)` 这一条 panel-edge 收尾步骤。

#### C. 测试

目标：把模块级接口变化和主线切换分别落到可维护的测试层次。

当前状态：已完成本轮目标验证。

本轮已落地：

1. `Tools/tests/StylusPipelineModulesTest.cpp` 已补到覆盖 state-based parser、`StylusPacketRoute`、`FinalizeTerminal`、`FinalizeFinal`。
2. `Tools/tests/StylusPipelineFastInkTest.cpp` 保持现有语义不变，并通过了主线切换后的回归运行。
3. 当前已经完成的 focused verification：
   - `StylusPipelineModulesTest.exe`
   - `StylusPipelineFastInkTest.exe`

### 10.4 当前剩余项总表

从当前仓库可见状态看，本次“主线切到 `StylusFrameState` 顺排 orchestrator”已经完成。下一阶段如果继续收缩，剩余项主要是锦上添花型，而不是本轮 blocker：

1. 如果要进一步逼近 `TouchPipeline` 的极简形态，可以继续把 `m_edgeCoorPost.Apply(...)` 下沉成 tail 模块的一部分。
2. 如果要让 pipeline 连状态镜像赋值都更少，可以继续把 `stylus.pressure/tipSwitch/noPressInk/sustain/fastLift` 这类 frame mirror 写回再下沉一步。
3. 如果后面还想让 diagnostics 完全不依赖主线提供任何额外上下文，可以考虑把 `signalRatio`、`linearFilterMode` 等缓存再并入 state。
