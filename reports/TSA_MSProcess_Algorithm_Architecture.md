# TSA_MSProcess 算法架构报告

## 目标平台: W273AS2700 (高坤 Himax CSOT) — 手指互容传感流水线

**二进制文件**: `TSACore.dll` @ `0x6ba90e43`
**传感器**: 60 列 × 40 行 (2400 个互容节点)
**日期**: 2026-05-26

---

## 1. 流水线总览

```
入口 → raw→dif/BLSM → CMF回退 → GridIIR → 峰值检测(模式3) → 触控跟踪 → 后处理 → 上报
```

当前激活的算法链路是一条贯穿 6 个主要阶段的单通道线性流水线。共有 5 个功能分支在编译期或运行期被禁用（详见 §9）。

---

## 2. 阶段一：数据采集与原始数据预处理

### 2.1 入口与标志位设置
- 在 `g_tsaStaticPtr->dwProcessingState` 中置位 `PROC_MS_ACTIVE`
- `DataSwitch_ToGrid()` — 将数据复用器切换至网格（互容）模式
- `AFE_GetFrame()` — 获取 AFE（模拟前端）帧缓冲
- `SS_CopyRaw()` — 按 `bRows`×`bCols` 维度复制原始数据

### 2.2 提前退出保护
当 `pMutualRawData == NULL` 或原始数据未通过 `SS_IsRawValid()` 校验时：
- 检查 HW 复位状态 → 需要时调用 `TSA_MSReset()`
- 若侧帧数据有效，尝试 `SS_Process()` 恢复
- 最终落入 `TSA_MSProcessEnding()` 退出

### 2.3 预处理序列
```
TSAIDE_CleanPointXY → HardwareAnalyzer_Reset → [Proximity_Process] → Touch_Clean
```
- **Proximity_Process**: 仅在非 HW 复位且非旁路当前帧时运行
- 保存上一帧缓冲区：`g_tsaBufPreDif ← g_tsaBufDif`，`g_tsaBufPreRaw ← g_tsaBufRaw`
- `TSA_RawCheckProcess()` — 校验原始数据完整性

### 2.4 原始数据路由
由 `dwSystemFlags` 中的 `FLAG_USE_DIF_BUF_PATH` 控制：

| 标志位状态 | 数据目的地 | 附加动作 |
|---|---|---|
| **清零（正常）** | `g_tsaBufRaw` | 调用 `TPSensor_ProcessRaw()` |
| **置位** | `g_tsaBufDif` | （后续跳过 BLSM+CMF+IIR） |

随后：`Rawdata_Process()` → `HardwareAnalyzer_Process()`

---

## 3. 阶段二：信号处理链

### 3.1 BLIIR_CalcDiffCommon — 差分信号计算
**地址**: `0x6ba46f4c`

对所有 `wBufElementCount` 个元素执行逐元素减法：
- **正常模式** (`bField_0x11a == 0`): `dif[i] = baseline[i] - raw[i]`
- **反转模式** (`bField_0x11a != 0`): `dif[i] = raw[i] - baseline[i]`

这是关键的"raw→dif"转换。后续所有算法均基于差分缓冲区 `g_tsaBufDif` 运行。

### 3.2 BLSM（基线管理）
**地址**: `0x6ba4c645`

```
BLRecal_Process → BLSM_GetProperty(reset, refresh) → BLSM_UpdateStage → BLSM_ProcessStage → BLSM_ShbProcess
```

- **BLSM_ProcessStage**: 由 `g_tsaPrpt+0x278` 处的报告计数器驱动的状态机。当计数器 < 10 时，通过跳转表分发到各阶段处理函数。当计数器 ≥ 10 时，评估标志位决定重新计算差分（`BLIIR_CalcDiff`）或重置差分（`BLIIR_ResetDiff`）。
- **BLSM_ShbProcess**: 智能主机基线 — 在空闲期间自适应更新基线
- **复位/刷新触发**: HW 复位强制基线复位；功能标志 `dwFeatureFlags2` bit 0 强制属性刷新
- `BLSM_UpdateForRawUnstable()` 在 BLSM 之前调用，处理原始数据不稳定的情况

### 3.3 CMF（共模滤波器）— 回退路径
**地址**: `0x6ba4f2c8`
**当前配置**: `bCmfEnabled=0`（Rawdata_CMF 已跳过），`bCmfForceDim1Pass=1`

由于 `bCmfEnabled=0`，CMF 作为 **BLSM 之后的回退滤波器** 运行，而非 BLSM 之前的滤波器：

```
CMF_SaveOrderDiff → CMF_SavePreCmfDif → CMF_ProcessDim(1) → [Baseline_Compensation] → CMFRemap_Exit
```

**算法细节**：
- **方法**: 0（非 `CMF_Filter2D`）— 使用基于维度的处理方式
- **维度**: `CMF_ProcessDim(1)` — 仅处理 **dim1（行方向）**，因为 `bCmfForceDim1Pass=1` 强制走此路径，无论 remap 状态如何
- **逐行处理** (`CMF_ProcessDimUnit`): 计算每行的共模信号，从差分数据中减去
- **自容校验** (`SS_IsValidForCMF`): 每行在应用 CMF 前通过自容数据进行校验；无效行不做 CMF 修正
- **游戏场景**: 当 `g_tsaPrmtDynamic[0x40]` 且 `TSA_IsGameScenario()` 时，运行 `CMF_ProcessDimCommonNoise(1, 0x4B0, 200, cols, rows)` — 游戏使用场景的额外噪声抑制
- **边缘处理**: 对首/末 2 行应用特殊阈值（200）
- 输出保存至 `g_tsaBufAftDim1CMFDif` 和 `g_tsaBufAftCMFDif`

### 3.4 GridIIR（无限脉冲响应滤波器）
**地址**: `0x6ba61098`

条件式时序滤波器 — 仅在检测到噪声时激活：

```
if (IsAllFreqBecomeNoisy || IsAllFreqNoisy):
    GridIIR_ProcessCore(dif, dif, history, count, alpha)
else:
    memcpy(history, dif)  // 直通模式
```

- **GridIIR_ProcessCore**: 逐元素 `IIR_Process2(input, history, alpha, 8)` — 8 位精度 IIR，alpha 参数来自 `g_tsaPrmtFlash[1].pDim1PitchMap + 3`
- 未检测到噪声时，仅将差分数据复制到历史缓冲区（直通模式），保证零延迟

### 3.5 HardwareAnalyzer_ProcessDif
**地址**: `0x6baf1c4e`

最小化实现 — 仅运行 `HardwareAnalyzer_SensorBrokenDifProcess()` 进行差分数据的传感器完整性检查。

### 3.6 其他信号路径函数
- **TPSensor_Process**: 对处理后的差分数据进行触摸面板传感器处理
- **TSA_GetPrpt**: 从原始和基线缓冲区更新报告属性
- **Self_Process**: 自容处理（独立于互容）
- **ToeSynaBl_GetSidePrpt**: 侧边/边缘报告属性
- **SS_CheckDirtyByMutual**: 通过互容数据检测脏污/污染

---

## 4. 阶段三：充电器噪声检查点

```
TSA_MSRawDirectionDectect → Exception_CheckChargerNoiseInRxLines
```

当检测到充电器噪声且存在上一帧触摸点时：
- 设置 `bKeepPreviousTouches = true`
- 运行 `Exception_Process()` 进行噪声缓解
- 进入提前退出：保留上一帧触摸状态，跳过峰值检测和触控跟踪

当 `TSA_IsToBypassCurrentFrame()` 为 true 时，同样保留上一帧触摸点。

---

## 5. 阶段四：峰值检测（模式 3）

**地址**: `0x6ba68c4b`
**模式**: `dwPeakProcessingMode = 3`

### 5.1 阈值钳制
```
nMode3ThresholdCap = wPeakMode3ThresholdMax / 2
if (mode == 3 AND nMode3ThresholdCap < dynamic_threshold):
    dynamic_threshold = nMode3ThresholdCap
```
此举将峰值检测的动态阈值上限于 `wPeakMode3ThresholdMax` 的一半，防止弱信号条件下漏检触摸。

### 5.2 Peak_DetectInRange — 3×3 局部最大值检测
**地址**: `0x6ba685d7`

经典的 3×3 邻域局部最大值扫描，遍历差分网格：
- 对于每个 `dif[i] >= threshold` 的网格单元：
  - 检查全部 8 个邻居；单元必须严格大于（或在边界处等于）所有邻居
  - 若 `Peak_IsToExcludeSideRegion()` 且 `TouchThold_ToeUseSpecialThold()`：边缘列（col 1, col N-2, row N-1）使用来自 `g_tsaPrmtDynamic + 10` 的特殊边缘阈值
  - 防漂移检查：若 `dwFeatureFlags2 & 0x20`，调用 `PressureDrift_Detect()` 抑制压力漂移导致的伪峰
- 有效峰值通过 `Peak_Insert(col, row)` 插入

### 5.3 Peak_Z8Filter — 邻域抑制滤波
**地址**: `0x6ba68502`

移除 `peak_signal < sum_of_8_neighbors / 32` 的峰值：
- 反向遍历所有已检测峰值
- 对每个峰值检查 `peak.signal_z1 < peak.signal_z8 >> 5`
- 移除相对于其 8 邻域总和而言过弱的峰值

### 5.4 全内嵌信号差异检查
Z8 滤波后，检查 `Peak_IsFullIncellSDDetected()`：

| 条件 | 动作 |
|---|---|
| **无全内嵌 SD** | 设置 `bFullIncellSignalDisparity = 0`，运行 `Peak_Z1Filter(originalThreshold)`，恢复原始阈值 |
| **检测到全内嵌 SD** | 设置 `bFullIncellSignalDisparity = 1`，调整峰值阈值为 `(g_tsaPrmtDynamic[0x1c] * originalThreshold) >> 4` |

### 5.5 Peak_Z1Filter — 二次阈值滤波
**地址**: `0x6ba6848b`

简单的阈值复查：移除所有 `signal < param_1` 的峰值。

### 5.6 峰值后处理
- 查找最小峰值行位置
- **排序顺序**: `Peak_needUDFPLargeFingerWorkaround()` 决定按信号升序或降序排列
- `Peak_IDTracking()` — 基于空间邻近性分配跟踪 ID

### 5.7 缓冲区切换重检
初次峰值检测后，`SS_ChargerNoiseFilterSwitchBuffer()` 和 `SignalDisparity_SwitchBuffer()` 可能触发缓冲区切换。若任一触发，`Peak_Process()` 会在切换后的缓冲区上再次运行。

### 5.8 TSA_MSPeakFilter
**地址**: `0x6ba90dbd`

条件执行：若 `dwFeatureFlags2 & 1`，运行 `EdgePeakFilter_WorkAround()` 处理边缘相关的峰值伪影。

---

## 6. 阶段五：触控跟踪链

### 6.1 SignalDisparity_Process
**地址**: `0x6bb35d84`

计算信号差异缩放因子（非补偿）：
- 校验 dim1 和 dim2 自容数据
- 计算两个缩放估计值：`SignalDisparity_GetSDScalingByScoring()` 和 `SignalDisparity_GetSDScalingBySignalRatio()`
- 取两者中的 **最大值** 作为全局 SD 缩放因子
- 由于 `FEAT_SIGNAL_DISPARITY_POST=0`，补偿步骤被跳过 — 仅计算缩放因子

### 6.2 TZ_Process — 触摸区域构建
**地址**: `0x6ba8c9f0`

将检测到的峰值转换为触摸区域：
- `TZ_PeakBasedProcess()` — 主要的峰值→区域转换
- 若握持标志 `0x200` 置位：前置 `TZ_UpdatePeakTzAge()`，后置 `TZ_AgeProcess()`
- 处理前将触摸计数重置为 0

### 6.3 TSA_MSTouchPreFilter
**地址**: `0x6ba90de8`

坐标计算前的两个预滤波器：
- `RxLineFilter_Process()` — 按 RX 线特性过滤触摸数据
- `SigSumFilter_ReserveTouch(param)` — 信号总和过滤，参数来自 `g_tsaPrmtFlash[1].field_0x74`

### 6.4 CTD_ECProcess — 坐标变换与边缘补偿
**地址**: `0x6ba55ca0`

逐触摸循环（触摸结构体大小: 0x168 字节）：
1. 保存原始坐标至备份字段（偏移 0x162/0x164 ← 0x15e/0x160）
2. `CTD_ECProcessUnit(touchIdx)` — 核心坐标变换与边缘补偿
3. 保存补偿后坐标（偏移 0x170/0x172 ← 0x162/0x164）
4. `Touch_SetEdgeDist(touchIdx)` — 计算边缘距离度量

### 6.5 IDT_Process — ID 跟踪
**地址**: `0x6ba611a6`

三阶段跟踪，`bUseStrictAccCheck=0`：
1. **IDT_HMProcess**: 在 20 字节临时映射上运行匈牙利匹配算法 — 当前触摸与上一帧触摸 ID 的最优分配
2. **IDT_AccCheck**: 加速度一致性检查 — 验证匹配对是否具有物理上合理的运动轨迹
3. **IDT_Map**: 将当前触摸索引映射到持久 ID

### 6.6 双向索引分配
- `Touch_AssignPreIdxBasedOnID()` — 当前→上一帧索引映射
- `PrevTouch_AssignCurIdxBasedOnID()` — 上一帧→当前索引映射

### 6.7 TS_Process — 触摸状态机
**地址**: `0x6ba7c2cb`

逐触摸状态处理：
- 跳过标志位 `0x20` 置位的触摸（新创建）
- `TS_ProcessUnit(prevIdx, curIdx)` — 每个触摸的状态转换逻辑

### 6.8 TE_Process — 触摸事件处理
**地址**: `0x6ba789d9`

两遍事件生成：

**第一遍 — 抬起检测**（上一帧有但当前无匹配的触摸）:
- 若 `TSA_IsWindowsPadFeatureEnabled()`: 使用 `TE_WindowsLiftOffProcess()`（符合 Windows 精确触控板规范）
- 否则: 使用 `TE_LiftOffProcess()`（标准）

**第二遍 — 当前触摸事件**:
- **新触摸** (`field_0x154 == 0`): `TE_TouchDownProcess()` — 按下事件
- **已有触摸** 带防抖标志: `TE_TouchDownDebounce()` — 防抖处理
- **已有触摸** 无防抖标志: `TE_MoveProcess()` — 移动/更新事件

**事件后处理**:
- `TE_AssertProcess()` — 校验事件一致性；失败时可能触发 `Touch_Clean()` + `PrevTouch_Release()`
- 若 `PROC_MS_ACTIVE`: `Touch_ProcessAftTouchEvent()` + `TSAStatic_ProcessAftTouchEvent()`

### 6.9 ER_Process — 边缘拒绝
**地址**: `0x6ba584b3`

逐触摸边缘处理：
- **新触摸** (标志 0x2): `ER_TouchDownProcess()` — 校验边缘处的触摸按下
- **边缘移出检测**: 若触摸处于状态 8 或 0x20 且上一帧触摸有边缘标志 0x8，检查 `ER_MoveOutCheck()` → `ER_MoveOutCorrection()` 处理触摸移出传感器边缘的情况
- **边缘标志清理**: 清除标志 0x400，为边缘过渡触摸设置 0x20/0x80

---

## 7. 阶段六：后处理

### 7.1 触摸动作流水线
```
TouchAction_Process → TSA_MSTouchPostFilter → TS_PostProcess → TouchAction_PostProcess
```

### 7.2 Exception_Process
处理运行时异常（噪声、干扰、环境变化）。

### 7.3 Gesture_Process
**地址**: `0x6bae16db`

基于 `g_tsaPrmtDynamic[0x80] & 8` 和 `FEAT_STYLUS (0x10)` 进行路由：
- **两者均置位**: `Gesture_StylusProcess()` — 触控笔手势识别
- **否则**: `Gesture_NormalProcess()` — 手指手势识别（点击、滑动等）

### 7.4 GripFilter_Process
**地址**: `0x6bae1d3c`

两级握持抑制：
1. **逐触摸**: 对每个非合并触摸执行 `GripFilter_BasicProcess()`
2. **全局** (当 `FEAT_GRIP (0x20)` 启用时):
   - `GripFilter_IsMergeHandlerEnabled()` → `TSABuffer_GripStageProcess()` — 合并处理
   - `GripFilter_Report()` — 握持报告生成
   - `GripFilter_IDLEProcess()` — 空闲状态握持处理

### 7.5 TouchMode_Process
**地址**: `0x6ba78dde`

触摸模式状态机：
1. 将上一帧模式/结果/数据从 `g_tsaPrpt+0x2e8` 复制到 `g_tsaPrpt+0x2f4`
2. `TouchMode_SwitchPriorityProcess()` — 构建优先级表
3. 若触控笔位已置位: `TouchMode_StylusFilter()`
4. 遍历优先级表（最多 4 个模式）；对每个候选模式：
   - 从 `g_tmHandlerTable[mode]` 查找处理函数对
   - 调用检查处理函数 → 返回结果码
   - 若 `result == 2`（提交）: 同模式调用提交处理函数 + 停止计时器；新模式调用 `TouchMode_NewModeProcess()`
   - 将选定模式提交至 `g_tsaPrpt+0x2e8`

### 7.6 信号差异与充电器噪声后处理
- `SS_ChargerNoiseFilterPostProcess()` — 帧后充电器噪声清理
- `TSAIDE_LogPreFltGridRpt()` — 诊断日志记录

---

## 8. 阶段七：上报

```
AntiTouch_Process → Touch_ProcessBetaGrip → TouchReport_Process → Touch_ProcessExtInfo
→ HardwareAnalyzer_PostProcess → HandGesture_Process → BigData_RecordProcess
→ BigData_StatProcess → PrevTouch_PostProcess → PrevPeak_Process → TSABuffer_PostProcess
```

### 8.1 TouchReport_Process
**地址**: `0x6ba7be73`

- 通过 `TouchReport_ProcessUnit(curIdx, prevIdx)` 构建逐触摸上报单元
- **多步回退**: 若启用且所有触摸均在"回退"，恢复上一帧坐标以实现平滑的撤回行为

### 8.2 其他上报功能
- **AntiTouch_Process**: 误触排除
- **Touch_ProcessBetaGrip**: Beta 握持抑制
- **Touch_ProcessExtInfo**: 扩展触摸信息（压力、面积等）
- **HardwareAnalyzer_PostProcess**: 帧后硬件分析
- **HandGesture_Process**: 手掌手势检测
- **BigData_RecordProcess/StatProcess**: 遥测记录与统计

---

## 9. 已禁用的分支

| 分支 | 控制标志 | 原因 |
|---|---|---|
| **Rawdata_CMF** | `bCmfEnabled = 0` | CMF 作为 BLSM 之后的回退运行，而非 BLSM 之前的原始数据 CMF |
| **SafeBaseline** | `FEAT_SAFE_BASELINE = 0` | 手持亮屏基线保护已禁用 |
| **UnderWater** | `FEAT_UNDERWATER_DETECT = 0` | 水下检测已禁用 |
| **SignalDisparity_Post** | `FEAT_SIGNAL_DISPARITY_POST = 0` | 帧后信号差异补偿已禁用；仅计算缩放因子 |
| **AFT** | (不在当前激活路径中) | 高级滤波触摸未接入当前流水线 |

---

## 10. 关键参数配置

### 传感器与缓冲区
| 参数 | 值 | 来源 |
|---|---|---|
| 列数 (dim1) | 60 (0x3C) | `bCols`, flash+0x1e |
| 行数 (dim2) | 40 (0x28) | `bRows`, flash+0x1d |
| 网格元素数 | 2400 (0x0960) | `wBufElementCount` |
| 网格缓冲区大小 | 0x1324 字节 | `g_gridBufSize` |

### 算法开关
| 参数 | 值 | 效果 |
|---|---|---|
| `dwPeakProcessingMode` | 3 | Z8 滤波 + Z1 回退 + 阈值钳制 |
| `bCmfEnabled` | 0 (运行时) | CMF 在 BLSM 之后回退执行 |
| `bCmfForceDim1Pass` | 1 | 仅 dim1 方向 CMF 处理 |
| `bCmfProcessingBlocked` | 运行时检查 | 被阻塞时可跳过 CMF |
| `bFullIncellSignalDisparity` | 运行时 | 控制走 Z1 还是 SD 阈值路径 |

### Flash 表间距图
| 参数 | 值 |
|---|---|
| Dim1 间距图 (flash+0xa0) | 活跃表，首值: 0, 0.984, 1.969, 2.953... |
| Dim2 间距图 (flash+0x320) | 标记/禁用表，首值为 100.0 |

---

## 11. 数据流总图

```
AFE_GetFrame()
  │
  ▼
SS_CopyRaw() ──► g_tsaUnnormalizedRawBuf
  │
  ▼
[FLAG_USE_DIF_BUF_PATH?]
  │
  ├─ NO ──► g_tsaBufRaw ──► TPSensor_ProcessRaw()
  │
  └─ YES ─► g_tsaBufDif
  │
  ▼
BLIIR_CalcDiffCommon(g_tsaBufDif, g_tsaBufRaw, g_tsaBufBl)
  │  dif = baseline - raw
  ▼
BLSM_Process() ──► 更新 g_tsaBufBl
  │
  ▼
CMF_Process() ──► g_tsaBufDif (dim1 共模已移除)
  │
  ▼
GridIIR_Process() ──► g_tsaBufDif (有噪声时做时序 IIR)
  │
  ▼
Peak_Process() ──► g_peaksPtr (局部最大值, Z8/Z1 滤波后)
  │
  ▼
TZ_Process() ──► g_touchesPtr (从峰值构建触摸区域)
  │
  ▼
CTD_ECProcess() ──► 补偿后坐标
  │
  ▼
IDT_Process() ──► 持久触摸 ID
  │
  ▼
TS_Process() + TE_Process() ──► 触摸状态 + 事件
  │
  ▼
[后处理: Gesture, Grip, TouchMode, Exception]
  │
  ▼
TouchReport_Process() ──► 最终输出报告
```

---

## 12. 关键设计特征

1. **单通道流水线**: 无迭代求精 — 每阶段每帧仅处理一次
2. **噪声门控 IIR**: GridIIR 在清洁环境下为直通模式，最小化延迟
3. **两级峰值滤波**: Z8（邻域比值）后 Z1（绝对阈值），实现鲁棒的峰值校验
4. **充电器噪声优先**: 充电器噪声检测可跳过整个峰值/跟踪流水线并保留上一帧触摸
5. **仅 dim1 CMF**: 仅移除行方向共模；列方向 CMF 已禁用
6. **基线状态机**: BLSM 具有多阶段状态机（< 10 个阶段），实现自适应基线管理
7. **缓冲区切换韧性**: 若充电器噪声滤波或信号差异触发缓冲区切换，峰值检测可重新运行
8. **全流程边缘感知**: 每个阶段均有特殊阈值和排除逻辑（峰值检测、CMF、CTD、ER）

---

## 13. 手指管线共享大帧结构体

手指管线各算法阶段通过以下核心数据结构交换触摸数据。这些结构体是整条流水线的"共享内存"，每个阶段读取并可能修改其中字段。

### 13.1 当前帧触摸结构体 (TouchEntry)
**总大小**: `0x168` (360 字节) / 每触摸点
**数组指针**: `g_touchesPtr`
**数组头**: 偏移 0x00 为 `bTouchCount`（当前帧触摸点数）

| 偏移 | 大小 | 类型 | 字段用途 | 读/写阶段 |
|---|---:|---:|---|---|
| `0x154` | 1 | byte | 新触摸标志 (0 = 新按下) | TE_Process (读) |
| `0x15e` | 2 | short | dim1 原始坐标 | CTD_ECProcess (读→备份) |
| `0x160` | 2 | short | dim2 原始坐标 | CTD_ECProcess (读→备份) |
| `0x162` | 2 | short | dim1 备份坐标 (可被历史缓冲覆写) | CTD_ECProcess (写), TouchReport (写) |
| `0x164` | 2 | short | dim2 备份坐标 (可被历史缓冲覆写) | CTD_ECProcess (写), TouchReport (写) |
| `0x170` | 2 | short | dim1 补偿后坐标 (最终输出) | CTD_ECProcess (写), TouchReport (读/回退时覆写) |
| `0x172` | 2 | short | dim2 补偿后坐标 (最终输出) | CTD_ECProcess (写), TouchReport (读/回退时覆写) |
| `0x194` | 1 | byte | 触摸面积 (mm) | TS_ProcessUnit (写) |
| `0x195` | 1 | byte | UDFP 触摸面积 (mm) | TS_ProcessUnit (写) |
| `0x1ec` | 4 | uint | 次要标志位 | TouchReport (bit0x40 检测) |
| `0x1f0` | 4 | uint | 状态标志位集 | ER_Process (bit0x20, bit0x40), TouchReport (bit0x8000000) |
| `0x1f8` | 4 | uint | 主状态/上报标志位 | ER_Process (bit0x400, bit0x80), TouchReport (bit0x8=已上报, bit0x100000, bit0x800000=姿势取消) |
| `0x21c` | 4 | uint/int | 触摸状态码 (2|8|0x10|0x20) | TS_Process (跳过0x20), ER_Process, TE_Process |
| `0x290` | 4 | uint | 上报状态 (1|2|4|0x20) | TouchReport (写) |

### 13.2 上一帧触摸结构体 (PrevTouchEntry)
**总大小**: `0x478` (1144 字节) / 每触摸点
**数组指针**: `g_prevTouchesPtr`
**数组头**: 偏移 0x00 为 `bPrevTouchCount`，偏移 0x20 为 `nTouchCount`

| 偏移 | 大小 | 类型 | 字段用途 | 读/写阶段 |
|---|---:|---:|---|---|
| `0x194` | 1 | byte | 上一帧触摸面积 (mm) | TS_ProcessUnit (读) |
| `0x1f0` | 4 | uint | 状态标志位集 | TouchReport (bit0x8000000 检测) |
| `0x1f8` | 4 | uint | 主状态标志位 (bit0x8=曾被上报) | ER_Process (bit0x8), TouchReport (bit0x8) |
| `0x21c` | 4 | uint | 上一帧触摸状态码 | TE_Process (bit0x1=防抖) |
| `0x594` | 2 | short | 上一帧 dim1 坐标 | TouchReport (读, 回退时恢复) |
| `0x596` | 2 | short | 上一帧 dim2 坐标 | TouchReport (读, 回退时恢复) |

### 13.3 触摸数组头
**当前帧数组** (`g_touchesPtr`):

| 偏移 | 大小 | 类型 | 含义 |
|---|---:|---:|---|
| `0x00` | 1 | byte | `bTouchCount` — 当前帧触摸点数 |
| `0x05` | 1 | byte | 已上报触摸计数 (TouchReport 递增) |
| `0x0b` | 1 | byte | 含标志位 0x40 的触摸计数 (TouchReport 递增) |
| → `0x168 * N` | — | TouchEntry[N] | 触摸条目数组 |

**上一帧数组** (`g_prevTouchesPtr`):

| 偏移 | 大小 | 类型 | 含义 |
|---|---:|---:|---|
| `0x00` | 1 | byte | `bPrevTouchCount` — 上一帧触摸点数 |
| `0x20` | 4 | int | `nTouchCount` — 计数 |
| → `0x478 * N` | — | PrevTouchEntry[N] | 上一帧触摸条目数组 |

### 13.4 峰值检测结构体 (PeakArray / PeakEntry)
**数组指针**: `g_peaksPtr`
**头大小**: 22 字节 (`0x00`–`0x15`)
**每峰值步长**: `0x14` (20 字节)

**数组头**:

| 偏移 | 大小 | 类型 | 含义 |
|---|---:|---:|---|
| `0x00` | 1 | byte | `bPeakCount` — 检测到的峰值总数 |
| `0x0c` | 1 | byte | `bMinPeakRow` — 所有峰值中的最小行位置 (Peak_Process 写入) |

**每峰值条目** (以条目 N 的绝对偏移 = `0x16 + N * 0x14`):

| 相对 | 绝对 (N=0) | 大小 | 类型 | 字段用途 |
|---:|---:|---:|---|---|
| `0x00` | `0x16` | 1 | byte | 峰值行位置 (row) |
| `0x01` | `0x17` | 1 | byte | 峰值列位置 (col) |
| `0x02` | `0x18` | 2 | short | `signal_z1` — 峰值信号值 (Z1Filter/Z8Filter 使用) |
| `0x06` | `0x1c` | 4 | int | `signal_z8` — 8 邻域信号和 (Z8Filter 使用) |

### 13.5 网格数据缓冲区
所有缓冲区大小为 `wBufElementCount * 2 = 2400 * 2 = 4800` 字节 (0x12C0)，类型为 `short[]`。

| 缓冲区 | 用途 | 主要读者/写者 |
|---|---|---|
| `g_tsaBufRaw` | 当前帧原始互容数据 | Rawdata_Process, BLIIR_CalcDiffCommon |
| `g_tsaBufDif` | 差分数据 (raw − baseline) | BLIIR_CalcDiffCommon(写), CMF_Process, GridIIR_Process, Peak_DetectInRange(读) |
| `g_tsaBufBl` | 基线数据 | BLSM_Process(更新), BLIIR_CalcDiffCommon(读) |
| `g_tsaBufPreRaw` | 上一帧原始数据 | 帧间比较 |
| `g_tsaBufPreDif` | 上一帧差分数据 | 旁路帧时回退使用 |
| `g_tsaBufPreCMFRaw` | CMF 执行前的原始数据快照 | 诊断/比较 |
| `g_tsaBufPreCMFDif` | CMF 执行前的差分数据快照 | CMF_Process(保存) |
| `g_tsaBufGridIIRDif` | GridIIR 历史/状态缓冲 | GridIIR_ProcessCore(读/写) |
| `g_tsaBufAftCMFDif` | CMF 处理后的差分数据 | CMF_Process(写) |
| `g_tsaBufAftDim1CMFDif` | dim1 CMF 处理后的差分数据 | CMF_Process(写) |

### 13.6 TSAStatic — 全局状态结构体
**总大小**: 744 字节 (0x2E8)

**手指管线关键字段**:

| 偏移 | 大小 | 类型 | 含义 |
|---|---:|---:|---|
| `0xbc` | 4 | uint | `dwSystemFlags` — 系统标志 (含 `FLAG_USE_DIF_BUF_PATH`) |
| `0xfc` | 4 | uint | `dwFeatureFlags` — 功能开关 (SafeBaseline, UnderWater, SignalDisparity_Post 等) |
| `0x100` | 4 | uint | `dwFeatureFlagsPrev` — 上一帧功能标志 |
| `0x104` | 4 | uint | `dwFeatureFlagsMasked` — 掩码后的功能标志 |
| `0x108` | 4 | uint | `dwFeatureFlags2` — 扩展功能开关 (EdgePeakFilter, PressureDrift 等) |
| `0x10c` | 4 | uint | `dwProcessingState` — 处理状态 (含 `PROC_MS_ACTIVE`) |
| `0x114` | 1 | byte | `bFullIncellSignalDisparity` — 全内嵌 SD 标志 (Peak_Process 写入) |
| `0x11a` | 1 | byte | `bField_0x11a` — 差分方向控制 (BLIIR_CalcDiffCommon 读取) |
| `0x11c` | 1 | byte | `bCmfProcessingBlocked` — CMF 阻塞标志 (CMF_Process 检查) |
| `0x274` | 4 | uint | `dwAftRuntimeModeMask` — AFT 运行时模式掩码 |
| `0x294` | 1 | byte | `bFrameDoneFlag` — 帧完成标志 |

### 13.7 TSAFlashPrmt — Flash 参数表（手指相关字段）
**总大小**: 2112 字节 (0x840)，运行时通过 `g_tsaPrmtFlash` 访问

| 偏移 | 大小 | 类型 | 含义 |
|---|---:|---:|---|
| `0x18` | 1 | byte | `bCmfEnabled` — CMF 使能 (flash=1, 运行时=0) |
| `0x1d` | 1 | byte | `bRows` — 传感器行数 (0x3C = 60) |
| `0x1e` | 1 | byte | `bCols` — 传感器列数 (0x28 = 40) |
| `0x22` | 2 | ushort | `wBufElementCount` — 缓冲元素数 (0x0960 = 2400) |
| `0xa0` | 648 | double[81] | `pDim1PitchMap` — dim1 间距补偿查找表 |
| `0x320` | 648 | double[81] | `pDim2PitchMap` — dim2 间距补偿查找表 (标记值 100.0 = 禁用) |

### 13.8 结构体关系与数据流

```
g_tsaBufRaw (short[2400])                    g_peaksPtr (PeakArray)
  │ BLIIR_CalcDiffCommon                       │ Peak_Process
  ▼                                             ▼
g_tsaBufDif (short[2400])                    PeakEntry[0..N]  (步长 0x14)
  │ CMF_Process → GridIIR_Process              │ TZ_Process
  ▼                                             ▼
g_tsaBufDif (已滤波)                         g_touchesPtr (TouchArray)
  │ Peak_DetectInRange                          │ CTD_ECProcess → IDT → TS → TE
  ▼                                             ▼
g_peaksPtr                                   TouchEntry[0..N] (步长 0x168)
                                                │ TouchReport_Process
                                                ▼
                                             g_prevTouchesPtr (PrevTouchArray)
                                               PrevTouchEntry[0..N] (步长 0x478)
```

---

## 14. 信号处理链算法深度分析

### 14.1 BLIIR_CalcDiffCommon — 差分信号计算

#### 核心公式

```
dif[i] = baseline[i] - raw[i]    (正常模式, bField_0x11a == 0)
dif[i] = raw[i] - baseline[i]    (反转模式, bField_0x11a != 0)
```

#### 设计思想

这是整个手指管线最基础的操作——将绝对原始值转换为相对差分值。其核心设计只有一个要点：**差分方向可切换**。

正常模式下 `dif = baseline − raw`，含义是"基线比原始值高多少"，即手指靠近时差分值**增大**（手指吸收电场，原始值下降，baseline − raw 变大）。这是容式触摸的标准约定。

反转模式存在的原因是配合后续 `BLSM_ProcessStage` 中的跳转表逻辑。当基线管理状态机判定需要"重置差分"时，会翻转此方向标志。这不是常态运行路径，而是基线复位/重校准期间的过渡策略。

算法本身无滤波、无平滑——仅做逐元素整数减法。所有滤波职责由下游 CMF 和 GridIIR 承担。这种"减法先行、滤波后置"的架构使得每个阶段的输入/输出语义单一明确。

---

### 14.2 BLSM — 基线管理系统（完整状态更新机制）

#### 14.2.0 执行顺序与调用链

```
每帧 TSA_MSProcess 调用:
  BLSM_Process(bHardwareReset, bForceBaselineProperty)
    ├── BLRecal_Process()                    ← 重校准边界检测
    ├── BLSM_GetProperty(reset, refresh)     ← 信号量计算 → 属性标志
    ├── BLSM_UpdateStage()                   ← 属性 → 阶段号
    ├── BLSM_ProcessStage()                  ← 阶段执行（跳转表）
    │     ├── [stage 0] BLIIR_UpdateBl(...)  ← 正常更新
    │     ├── [stage 1,2] BLSM_BlFullReset   ← 全复位
    │     ├── [stage 3] 阶段重定向
    │     ├── [stage 4-6] NOP
    │     ├── [stage 7,8] BLIIR_UpdateBl(...) ← 边缘更新
    │     └── [stage 9] BLIIR_UpdateBl(..., knee=1) ← 膝点恢复
    │     └── 最终: BLIIR_CalcDiff 或 BLIIR_ResetDiff
    └── BLSM_ShbProcess()                    ← 慢通道基线快照
```

---

#### 14.2.1 信号量体系

BLSM 使用两大信号量来计算属性并决定状态。这两个值由 `TSA_GetPrpt()` 在每帧 BLSM 之前扫描整个差分网格 (2400 元素) 统计得出：

##### (A) 全局最大正/负信号

```
nSPeakPositive = max(dif[i])  ∀ i ∈ [0, wBufElementCount)  — 正向偏移最大格点
nSPeakNegative = min(dif[i])  ∀ i ∈ [0, wBufElementCount)  — 负向偏移最大格点 (保留符号)
```

这两个值是 BLSM 所有判断的核心输入。存储于 `g_tsaPrpt`：

| 字段 | 偏移 | 类型 | 含义 |
|---|---|---|---|
| `nSPeakPositive` | `g_tsaPrpt + 0xa4` | short | 全网格最大正差分 |
| `nSPeakNegative` | `g_tsaPrpt + 0xa6` | short | 全网格最大负差分（有符号，通常为负） |
| `wBoundaryPositive` | `g_tsaPrpt + 0x9e` | ushort | 正向超出阈值的边界计数 |
| `wBoundaryNegative` | `g_tsaPrpt + 0xa0` | ushort | 负向超出阈值的边界计数 |

**BLSM_GetMaxSig()** 返回有效最大正信号:
```
maxSig = nSPeakPositive
if (SafeBaseline未应用 AND 特殊toe阈值 AND 非游戏 AND field_0x3dc):
    if (maxSig < 0x514):
        maxSig = field_0xc0     ← 替代信号源
    maxSig = ToeSyna_PorpertyGetMaxSig(maxSig)   ← 边缘场景覆盖
return maxSig
```

**BLSM_GetMinSig()** 返回有效最大负信号:
```
minSig = nSPeakNegative  (负值)
if (SafeBaseline未应用 AND 特殊toe阈值 AND 非游戏 AND field_0x3dc):
    if (minSig > -0x514):
        minSig = field_0xc2    ← 替代信号源
    minSig = ToeSyna_PorpertyGetMinSig(minSig)   ← 边缘场景覆盖
return minSig
```

##### (B) 基线步长参数

`BLSM_GetNoTouchBlStep(stepThld, maxStep)` 输出两个参数，控制基线每步调整的幅度：

**正常模式** (`field_0x3 == 0`):
```
stepThld = g_tsaPrmtDynamic[0x48] 或 [0x4f]  (取决于边缘/特殊阈值状态)
maxStep  = g_tsaPrmtDynamic[0x50] 或 [0x4f]  (字节, 上限值)
```

**特殊模式** (`field_0x3 != 0`):
```
maxStep  = max(|nSPeakPositive|, |nSPeakNegative|) >> 4   ← 信号峰值的 1/16
maxStep  = CLAMP(maxStep, 0, stepThld >> 2)               ← 不超过 stepThld/4
```

---

#### 14.2.2 属性判定: BLSM_GetProperty → dwBaselinePropertyFlags

这是状态机的"传感器层"。按优先级检查各种条件，输出一个标志位集合：

```
输入: bResetPropertyRequested, bRefreshPropertyRequested
      全局信号: nSPeakPositive, nSPeakNegative
      噪声状态: BLReset_IsBaselineNoisy(), BLReset_InDebounce()
      边界状态: g_tsaPrmtDynamic[0x42]/[0x44]/[0x46]/[0x48]

判定流程（优先级从上到下）:

1. IF bResetPropertyRequested:
      bBaselineResetPropertyLatched = 1
      dwBaselinePropertyFlags = 0x10 (强制复位)
      IF systemFlags & 5:  propertyState = 1
      ELIF systemFlags & 8: propertyState = 2
      ELSE:                 propertyState = 3
      → 返回

2. ELIF bRefreshPropertyRequested:
      dwBaselinePropertyFlags = 0x40 (强制刷新)
      → 返回

3. ELSE (正常属性判定):
   a) IF BLReset_Process() = true:
         dwBaselinePropertyFlags = 0x10
         → 返回

   b) IF BLReset_IsBaselineNoisy() = true:
         dwBaselinePropertyFlags = 0x200
         propertyState = 6
         → 返回

   c) IF BLReset_InDebounce() = true:
         dwBaselinePropertyFlags = 0x100
         → 返回

   d) 边缘检测:
         IF TouchThold_ToeUseSpecialThold() AND 特殊列号:
             g_BLSMNextEdgeSmallThresh = 1

   e) 信号范围判定:
         maxSig = BLSM_GetMaxSig()
         IF maxSig < g_tsaPrmtDynamic[0x42]:            ← 信号在低阈值内
             IF NOT AFE_IsCoreTouchDetected() AND (无边缘 OR maxSig < [0x44]):
                 IF maxSig >= [0x46] OR (边缘 AND maxSig >= [0x48]):
                     dwBaselinePropertyFlags = 0x01      ← 正常更新
                 → 返回
         dwBaselinePropertyFlags = 0x02                 ← 有触摸/信号过大
```

**标志位汇总与优先级**:
```
0x10 > 0x40 > 0x200 > 0x100 > 0x80 > 0x01 > 0x02
 复位   刷新   噪声    防抖   阶段9  正常   信号过大
```

---

#### 14.2.3 阶段选择: BLSM_UpdateStage → dwBaselineStage

由 `dwBaselinePropertyFlags` 到具体阶段号 (0–9) 的映射：

```
dwBaselineStage = 
  0x10  (复位)  → 1    ← 最高优先: 全复位
  0x200 (噪声)  → 2    ← 噪声模式
  0x100 (防抖)  → 3    ← 防抖/暂停
  0x80  (阶段9) → 9    ← 膝点恢复
  0x40  (刷新)  → 3    ← 强制刷新走防抖
  其他           → 当前阶段不变; 帧计数+1; 若≥10则归零
```

当阶段不变时（正常状态），执行当前阶段的事件处理器：
- **阶段 < 10**: 通过跳转表调用阶段处理函数
- **阶段 ≥ 10**: 重置 `nBaselineStageFrameCount = 1`，阶段归零

---

#### 14.2.4 阶段执行引擎: BLSM_ProcessStage（跳转表展开）

每帧开始时三个清零:
```
bBaselineUpdatedThisFrame = 0
bResetDiffRequested = 0
bField_0x00 = 0
```

然后通过跳转表 `switchdataD_6bb51a9c[10]` 分发到对应阶段:

##### 阶段 0 — 正常更新（最频繁路径）

```
1. BLSM_GetNoTouchBlStep(&stepThld, &maxStep)
   → 从动态参数获取步长和上限

2. BLIIR_Update(stage=0, maxSignal=stepThld, stepSize=maxStep)
   → 门控检查: 仅当 maxSig ≤ stepThld AND -stepThld ≤ minSig 时执行
   → 执行 BLIIR_DoUpdate(maxStep)
   
3. bBaselineUpdatedThisFrame = 1
4. BLSM_SaveNormalBl()   ← 保存正常基线快照
```

##### 阶段 1, 2 — 全复位

```
1. bResetDiffRequested = 1        ← 通知下游重置差分
2. BLSM_BlFullResetRequest()
   → BLIIR_Reset()                ← 重置基线为 raw 或目标值
   → Self_Reset()                 ← 重置自容
   → SS_Reset()                   ← 重置自容处理
```

##### 阶段 3 — 阶段重定向

```
IF field_0x27c ∈ {1, 2, 3}:
    dwBaselineStage = 0           ← 重定向到正常更新
ELSE:
    dwBaselineStage = field_0x27c ← 重定向到指定阶段
```

##### 阶段 4, 5, 6 — 空操作

```
NOP (不做任何基线修改, 仅保持当前 baseline 不变)
```

##### 阶段 7, 8 — 边缘优化更新

```
IF (全局边缘标志 OR BLSM_ProcessUseNextSideThold()):
    stepThld = g_tsaPrmtFlash[0x4c]   ← 宽阈值 (边缘场景)
ELSE:
    stepThld = g_tsaPrmtFlash[0x4a]   ← 窄阈值
maxStep = g_tsaPrmtFlash[0x52]        ← 步长 (字节)

BLIIR_Update(stage=0, maxSignal=stepThld, stepSize=maxStep)
bBaselineUpdatedThisFrame = 1
```

与阶段 0 的区别：使用 flash 表中的固定阈值（而非 `dynamic` 动态参数），且边缘场景有独立的宽阈值路径。这使边缘基线更新能适应边缘列信号偏弱的物理特性。

##### 阶段 9 — 膝点恢复模式

```
kneeThreshold = abs(g_tsaPrpt->field_0xa2) >> 4    ← 信号偏移量的 1/16
baseThreshold  = g_tsaPrmtFlash[0x46] >> 2          ← flash 阈值的 1/4

IF (baseThreshold < kneeThreshold):
    kneeThreshold = baseThreshold                  ← 取较小值

BLIIR_Update(stage=1, maxSignal=-field_0xa2, stepSize=kneeThreshold)
                                                    ↑ stage=1 触发无条件模式
bBaselineUpdatedThisFrame = 1
```

**膝点模式的特殊之处**: `stage=1` 传给 `BLIIR_Update`，使其跳过门控检查（见下方公式），无条件执行基线更新。这用于处理信号因温度/环境突变而产生的快速单方向偏移——"膝点"（knee point）指信号变化曲线上的拐点。

##### 阶段执行后统一收尾

```
IF bResetDiffRequested:
    BLIIR_ResetDiff()   ← memset(dif, 0, 4800)
ELSE:
    BLIIR_CalcDiff()    ← dif = baseline − raw (重新计算全网格差分)
```

---

#### 14.2.5 核心更新公式: BLIIR_Update → BLIIR_DoUpdate

##### 门控逻辑 (`BLIIR_Update`)

```
输入: stage (0=正常, 1=膝点), maxSignal, stepSize

maxSig = BLSM_GetMaxSig()   ← 全网格最大正差分
minSig = BLSM_GetMinSig()   ← 全网格最大负差分 (负值)

IF stage == 1 (膝点模式):
    → 直接执行 BLIIR_DoUpdate(stepSize)   [无条件]
ELSE (stage == 0, 正常/边缘模式):
    IF (-maxSignal ≤ minSig) AND (maxSig ≤ maxSignal):
        → 执行 BLIIR_DoUpdate(stepSize)   [通过门控]
    ELSE:
        → 跳过更新                          [门控拦截]
```

**门控的物理含义**: 只有当全网格的差分信号完全落在 `[−maxSignal, +maxSignal]` 区间内时，才允许更新基线。如果有人触摸屏幕，局部信号会远超此区间，门控拦截更新，防止触摸信号"污染"基线。

##### 逐元素更新公式 (`BLIIR_DoUpdate`)

```
输入: stepSize (步长参数)

对每个网格元素 i ∈ [0, wBufElementCount):
    rawVal = (field_0x69 == 0) ? g_tsaBufRaw[i] : g_tsaBufPreCMFRaw[i]
    blVal  = g_tsaBufBl[i]
    
    diff = rawVal − blVal

    IF   diff ≥ +stepSize:    g_tsaBufBl[i] += stepSize
    ELIF diff ≤ −stepSize:    g_tsaBufBl[i] −= stepSize
    ELSE:                     无变化 (死区)
```

**公式解读**:
- 这是一个**对称死区跟踪器**。基线以每步 `±stepSize` 的固定步长追逐原始信号
- 死区 `(−stepSize, +stepSize)` 内的信号波动被完全忽略——这防止基线被微小噪声拖着来回振荡
- 当原始信号持续偏离基线超过死区时，基线以每帧一步的速率追赶。对于缓慢的环境漂移（温度），步长足够小以平滑跟踪；对于触摸信号，门控层在更早阶段就阻止了更新
- `field_0x69` 控制使用哪个 raw 数据源：为 0 时直接使用 `g_tsaBufRaw`，为 1 时使用 CMF 前的快照 `g_tsaBufPreCMFRaw`

##### 基线复位公式 (`BLIIR_Reset`)

```
IF NOT AFE_IsRawUnifiedToTargetValue():
    g_tsaBufBl = memcpy(g_tsaBufPreCMFRaw)              ← 用 CMF 前原始值替换
ELIF propertyState == 3 OR AFE_IsAlwaysNoisy():
    g_tsaBufBl = memcpy(g_tsaBufPreCMFRaw)              ← 同上
ELSE:
    g_tsaBufBl[i] = g_tsaPrmtFlash->field_0x5fc  ∀i    ← 用固定目标值填充
```

---

#### 14.2.6 属性判定子函数详解

##### BLReset_Process — 复位触发检测

```
1. 清零 field_0x6
2. IF bResetPropertyRequested: 清零 field_0x286
3. 更新 bBaselineResetCheckEnabled (基于 BLReset_IsToDisable)
4. IF NOT bBaselineResetCheckEnabled: → 不触发
5. IF BLReset_IsTriggered(): → 触发 (返回 true)
6. 更新 field_0x284/field_0x286 计数器 (帧计数, 上限来自 dynamic[0x58]/[0x5a])
7. IF 触发: 清零 field_0x286
8. 返回触发状态
```

##### BLReset_IsBaselineNoisy — 噪声持续性判断

这是一个带滞后的噪声状态追踪器：

```
状态变量: field_0x300 (计时器), field_0x2f9 (标志)

IF prevPropertyState NOT IN {6, 3, 0} AND AFE_IsAlwaysNoisy():
    field_0x300 = nBaselineStageInitialSignal   ← 初始化计时器

IF field_0x300 == 0:
    返回 false  (无噪声)
ELIF field_0x4 < 6:                             ← 帧计数不足
    field_0x2f9 = 1
    field_0x300 = 0
    返回 true   (噪声确认)
ELIF field_0x300 < 0xa7 (167 帧):               ← 计时未到
    field_0x300 += nBaselineStageInitialSignal
    propertyState = 6
    返回 true   (噪声持续)
ELSE:
    field_0x300 = 0
    返回 false  (超时, 噪声结束)
```

**设计思想**: 这不是简单的"检测到噪声→立即进入噪声模式"，而是要求噪声在 **6–167 帧的时间窗口内**持续存在。`field_0x300` 作为计时器在窗口内递增，超出 167 帧后噪声状态自动退出——防止无限期停留在噪声模式。`AFE_IsAlwaysNoisy()` 是 AFE 层的硬件噪声检测。

---

#### 14.2.7 BLRecal_Process — 重校准边界检测（完整条件）

基于帧计数器 `bRecalFrameCount` 的分段逻辑：

**段 1 (帧 0–4)**: 静默期，仅递增计数器

**段 2 (帧 5–14)**: 累积边界命中证据
```
先决条件: nSPeakPositive < wPeakMode3ThresholdMax
          AND -nSPeakNegative < wPeakMode3ThresholdMax

三个独立命中条件 (满足任一 → bRecalBoundaryHitCount++):
  条件 A (双向失衡):
    (wBoundaryPositive − wBoundaryNegative) > field_0x4a

  条件 B (正向溢出):
    wBoundaryPositive > (field_0x48 + field_0x4a/2)

  条件 C (负向溢出):
    wBoundaryNegative > (field_0x48 − field_0x4a/2)
```

**段 3 (帧 ≥ 15)**: 决策
```
IF bRecalBoundaryHitCount > 2 AND NOT AFE_IsAutoCalibration():
    bJustExitedSpecialCtrlMode = 1
    dwSpecialControlFlags |= 2     ← 触发特殊控制模式
bRecalBoundaryHitCount = 0        ← 重置计数器
```

**自动校准超时退出**:
```
IF AFE_IsAutoCalibration() AND (当前帧时间 − llAutoCalibrationStartTime) > 300000ms:
    IF AFE_IsRecalRequested() AND bBaselineUpdatedThisFrame AND nBaselineStageFrameCount > 2:
        bJustExitedSpecialCtrlMode = 1
```

---

#### 14.2.8 BLSM_ShbProcess — 慢通道基线捕获

```
状态机 (状态保存于 g_tsaPrpt->dwShbState):

IF BLSM_IsReset() OR BLSM_ShbIsRawChanges():
    dwShbState = 0                ← 复位或原始数据变化→重置

状态 0 → 1: 初始化
    dwShbState = 1
    dwShbFlags |= 1               ← 请求捕获

状态 1 → 2: 等待 5000ms 后
    dwShbState = 2
    dwShbFlags |= 1

状态 2: 维持, 5000ms 后
    dwShbFlags |= 1 | 2

IF dwShbFlags & 1:
    BLSM_ShbCaptureBl()           ← g_tsaBufRawCapture = memcpy(g_tsaBufPreCMFRaw)
    qwShbStateStartTime = 当前帧时间戳
```

---

#### 14.2.9 完整状态变迁图

```
                    ┌──────────────────────────────────────────┐
                    │         BLSM_GetProperty (每帧)           │
                    │   信号量: nSPeakPositive, nSPeakNegative  │
                    │   输入: reset?, refresh?, noise?, debounce│
                    └──────────────┬───────────────────────────┘
                                   │
                                   ▼
                    ┌──────────────────────────┐
                    │  dwBaselinePropertyFlags  │
                    │  0x10/0x40/0x200/0x100/  │
                    │  0x80/0x01/0x02          │
                    └──────────────┬───────────┘
                                   │
                                   ▼  BLSM_UpdateStage
                    ┌──────────────────────────┐
                    │    dwBaselineStage        │
                    │    0 ─────────── 正常更新  │
                    │    1,2 ───────── 全复位    │
                    │    3 ─────────── 重定向    │
                    │    4,5,6 ─────── 空操作    │
                    │    7,8 ───────── 边缘更新  │
                    │    9 ─────────── 膝点恢复  │
                    └──────────────┬───────────┘
                                   │
                                   ▼  BLSM_ProcessStage (跳转表)
                    ┌──────────────────────────┐
                    │    执行阶段处理函数        │
                    │    stage 0: BLIIR_Update  │
                    │           → BLIIR_DoUpdate│
                    │    stage 9: BLIIR_Update  │
                    │           (无条件模式)     │
                    └──────────────┬───────────┘
                                   │
                                   ▼
                    ┌──────────────────────────┐
                    │  收尾: CalcDiff/ResetDiff │
                    │  更新: g_tsaBufDif 全网格  │
                    └──────────────────────────┘
```

---

#### 14.2.10 核心设计原则总结

1. **双层门控**: 第一层门控是 `BLSM_GetProperty` 的语义层（判断"应该更新基线吗"），第二层门控是 `BLIIR_Update` 的信号层（判断"现在可以安全更新吗"）。两层独立运作，保证即使属性层判断正确，信号层也会在触摸存在时阻止更新。

2. **死区跟踪而非平滑**: 基线更新使用固定步长 `±stepSize` 的死区跟踪，而非 IIR 平滑。这意味着基线不会在无触摸时做无谓的渐变——它只在原始值偏离超过死区时才迈出一步。这比 IIR 更节能，也更不容易被累积误差漂移。

3. **步长自适应**: 正常模式下步长来自动态参数表，膝点模式下步长与信号偏移量成正比 (`|signal| >> 4`)，但受限于 `stepThld >> 2`。这确保大幅度偏移时步长不会过大。

4. **阶段 4–6 空操作**: 这些阶段存在但不做任何基线修改。它们在 BLSM 状态机中充当"等待"或"观察"间隔——让系统在特殊事件（如复位、噪声）后有足够的时间稳定后再恢复正常的基线更新。

5. **快慢双通道**: BLSM 主循环是逐帧"快通道"（死区跟踪），SHB 是秒级"慢通道"（原始值快照用于长期参考）。两者的时间常数相差约 3 个数量级，分别应对快速干扰和缓慢漂移。

---

### 14.3 CMF — 共模滤波器

#### 架构总览

```
CMF_SaveOrderDiff        ← 计算一阶/二阶差分的行/列统计
CMF_SavePreCmfDif        ← 保存 CMF 前差分快照
CMF_ProcessDim(1)        ← 仅 dim1 (行方向) 处理
  ├── SS_DiffRelatCal    ← 自容校验: 计算相对校准值
  ├── CMF_ProcessDimUnit ← 逐行/列 CMF 核心
  │     ├── CMF_ProcessDimUnitOnBuf  ← 收集行/列数据
  │     └── CMF_ProcessCore          ← 共模计算
  │           └── CMF_ProcessCoreWeight  ← 加权共模 + 噪声校准
  └── CMF_ProcessDimCommonNoise → 游戏场景额外噪声抑制
Baseline_Compensation    ← 基线漂移补偿
```

#### 14.3.1 CMF_SaveOrderDiff — 一阶/二阶差分预计算

沿处理维度（dim1 强制模式下为行方向）计算相邻元素的一阶差分和二阶差分：

```
一阶差分: order1[i] = dif[i] − dif[i−1]
二阶差分: order2[i] = order1[i+1] − order1[i]
```

结果存入 `g_tsaBufOneOrderDiffRx` 和 `g_tsaBufTwoOrderDiffRx`。这是为后续共模计算准备的梯度特征。

**设计思想**：差分阶次是共模滤波的"特征工程"。一阶差分反映信号沿处理维度的变化率，二阶差分反映变化率的加速度。这两组特征用于区分真正的触摸信号（局部尖锐）和共模噪声（全局平滑）。噪声在一阶/二阶差分上表现出的统计特性与触摸信号不同，这是 CMF 能够分离两者的数学基础。

#### 14.3.2 CMF_ProcessDim — 逐行共模处理

核心流程（dim1 模式）：
1. 对每行通过自容校验 (`SS_IsValidForCMF`) 决定是否应用 CMF
2. 对有效行：`SS_DiffRelatCal` 计算相对校准值 → `SS_DiffCMFProcessDimUnit` 计算自容修正
3. 对每行：`CMF_ProcessDimUnit` 计算该行的共模值并从差分数据中减去
4. 边缘行（首/末 2 行）使用特殊阈值 200 替代标准阈值
5. 跟踪全局最小/最大共模值至 `g_tsaPrpt+0x300/0x302`

**设计思想**：CMF 假设共模噪声在整行（或整列）上均匀叠加，而触摸信号是局部现象。通过计算每行的"平均"偏差（共模值），将其从该行所有元素中减去，可以在不损伤触摸信号的前提下消除系统性偏移。自容校验 (`SS_IsValidForCMF`) 是关键的安全网——当自容数据不足以判定某行时，跳过该行的 CMF 处理，宁可保留噪声也不误伤信号。

#### 14.3.3 CMF_ProcessCoreWeight — 加权共模计算

```
CMF_NoiseCal(input, 0, count, noiseThreshold, ...)  ← 噪声特征提取
if (FEAT_SMART_COVER && smartCoverFlag):
    CMF_NoiseCal(input, 0, count, adjustedThreshold, ...)  ← 皮套模式调整
for each element:
    input[i] -= noiseValue                           ← 全局共模减法
```

**设计思想**：`CMF_NoiseCal` 的核心任务是从一行（或一列）的信号中提取共模成分。它分析差分数据沿该维度的统计分布，找出表征"整体偏移"的标量。然后简单地将此标量从该行每个元素中减去。皮套模式 (`FEAT_SMART_COVER`) 下会用调整后的阈值重新计算，以适应皮套带来的电容基线变化。

#### 14.3.4 Baseline_Compensation — 基线漂移补偿

当 CMF 处理后的报告属性 (`g_tsaPrpt[1].field_0x44/0x46`) 指示系统性偏移超过阈值 4 且不在特殊控制模式时：

```
if (positive_offset > 4 || negative_offset < -4):
    compensation = ±5  (方向由偏移符号决定)
    for all baseline elements:
        baseline[i] += compensation  (或 -= ，由 bField_0x11a 决定)
    set dwSpecialControlFlags |= 0x100
```

**设计思想**：CMF 处理差分数据，但如果基线本身存在系统性漂移（例如温度导致的缓慢偏移），仅靠 CMF 逐帧修正差分是不够的。`Baseline_Compensation` 直接修正基线值——每次 ±5 的小步长调整——使基线跟踪环境变化，从根源上减少 CMF 的负担。±5 的步长是精心选择的：足够小以避免过调振荡，又足够大以在实际时间尺度上跟踪温度漂移。

---

### 14.4 GridIIR — 时序 IIR 滤波器

#### 核心公式

```
output[i] = IIR_Process2(input[i], history[i], alpha, 8)

IIR_Process2 内部:
  result = (alpha * input + (256 - alpha) * history) / 256
  result += sign(input - result)   ← 舍入修正
```

#### 激活条件

```
if (IsAllFreqBecomeNoisy || IsAllFreqNoisy):
    执行逐元素 IIR
else:
    memcpy(history, input)  ← 直通
```

#### 设计思想

GridIIR 是一个**一阶低通 IIR 滤波器**，精度为 8 位（分母 = 2^8 = 256），由单字节参数 `alpha` 控制时间常数。

核心设计决策是**噪声门控激活**：

- **无噪声时完全直通**：这是 W273AS2700 的正常运行状态。直通意味着零延迟——触摸响应不会被滤波器拖慢。对消费电子设备而言，触控延迟是用户体验的核心指标，直通模式确保在理想条件下达到最佳响应。
- **有噪声时逐元素 IIR**：只有当全局噪声检测器（`TSAStatic_IsAllFreqBecomeNoisy` / `TSAStatic_IsAllFreqNoisy`）确认存在全频噪声时才激活。激活后，每个网格元素独立执行 `y[n] = (α·x[n] + (256−α)·y[n−1]) / 256`。

公式中的 `sign(input − result)` 项是**舍入修正**：整除截断可能导致系统性偏置，加入此修正确保滤波输出在统计上无偏。

alpha 参数来自 `g_tsaPrmtFlash[1].pDim1PitchMap + 3`，即 dim1 间距图的第 4 字节。这意味着 alpha 是一个全局常量（不随坐标变化），但其位置嵌入在 pitch map 中，暗示可能存在按列变化的 IIR 参数的扩展能力。

---

### 14.5 Peak Detection — 两级峰值检测

#### 架构总览

```
Peak_DetectInRange        ← 3×3 局部最大值扫描
Peak_Z8Filter             ← 邻域比值滤波
Peak_IsFullIncellSDDetected  ← 全内嵌 SD 判断
  ├── [无 SD] → Peak_Z1Filter + 恢复阈值
  └── [有 SD] → 调整阈值
Peak_Sort                 ← 排序
Peak_IDTracking           ← ID 分配
```

#### 14.5.1 Peak_DetectInRange — 局部最大值扫描

对每个网格位置 `(col, row)` 检查三个条件：
1. `dif[i] >= threshold` — 信号强度足够
2. 8 邻域内无更大值 — 是局部最大值
3. 压力漂移检测通过（若 `dwFeatureFlags2 & 0x20`）

边缘列 (col=1, col=N−2, row=M−1) 在侧边区域排除 + 特殊 toe 阈值激活时使用 `g_tsaPrmtDynamic + 10` 的独立阈值。

**设计思想**：这是经典的 3×3 非极大值抑制（NMS）算法，但有三处针对触摸屏的定制：
- **双阈值体系**：普通区域和边缘区域使用不同阈值。边缘列因传感器物理构造通常信号更弱，降低阈值防止边缘漏检。
- **压力漂移抑制**：当触摸持续时间长、压力稳定时，峰值信号强度会因物理效应而缓慢下降。`PressureDrift_Detect` 能识别这种模式并避免将漂移导致的弱信号峰值滤除。
- **网格遍历方向无关**：不同于图像处理中的先行后列遍历，这里遍历顺序无关紧要——每个峰值判断独立于之前的结果。

#### 14.5.2 Peak_Z8Filter — 邻域比值滤波

```
对每个峰值:
    if signal_z1 < signal_z8 / 32:
        移除
```

`signal_z8` 是 8 个相邻格点的信号之和。比值 `signal_z1 / signal_z8` < `1/32` 意味着峰值信号相对于其周边环境太弱。

**设计思想**：Z8 滤波器回答的问题是"这个峰值有多突出？"。一个真正的触摸信号应该在其 3×3 邻域内占据主导地位。Z8 阈值 `1/32` 意味着如果峰值信号不到其 8 邻居总和的 3.125%，则判定为噪声伪影。

这与简单的 `signal > 绝对阈值` 完全不同——**相对阈值**对信号强度的整体缩放（如增益变化、温度漂移）具有不变性。当全屏信号因环境变化整体抬升或降低时，绝对阈值可能失效，但 Z8 比值依然有效。

#### 14.5.3 Peak_Z1Filter — 绝对阈值二次滤波

```
对每个峰值:
    if signal < original_threshold:
        移除
```

**设计思想**：Z1 是"最后一道防线"。在 Z8 相对滤波之后，对幸存的峰值再做一次绝对阈值检查。这次使用的是**未经 Mode 3 钳制的原始动态阈值**（`nOriginalPeakThreshold`）。Z8 解决了"相对突出"问题，Z1 解决了"绝对强度"问题——两者互补。

#### 14.5.4 Mode 3 阈值钳制

```
nMode3ThresholdCap = wPeakMode3ThresholdMax / 2
if mode == 3 and nMode3ThresholdCap < dynamic_threshold:
    dynamic_threshold = nMode3ThresholdCap
```

**设计思想**：动态阈值由 `TSAPrmt_Process` 根据当前帧信号统计自适应调整。但在弱信号条件下（如手指在边缘、干手指），动态阈值可能飙升到不合理的高位，导致漏检。Mode 3 将阈值上限钳制在 `wPeakMode3ThresholdMax / 2`，确保在最差条件下阈值也不会完全阻止触摸检测。`/ 2` 是一个经验常数，在灵敏度和误触抑制之间取得平衡。

#### 14.5.5 Full-Incell SD 处理

当检测到全内嵌信号差异时：
- Z1 跳过（因为全屏信号差异会使 Z1 误杀真实触摸）
- 改用 `(g_tsaPrmtDynamic[0x1c] * threshold) >> 4` 作为替代阈值
- 设置 `bFullIncellSignalDisparity = 1` 通知下游

**设计思想**：SD (Signal Disparity) 是整屏信号异常偏移的标志。此时 Z1 的绝对阈值过滤逻辑会失效——因为所有信号都被系统性抬升或压低。检测到 SD 后，改用基于缩放因子的替代阈值，确保即使在 SD 状态下也能正确检测峰值。

---

### 14.6 信号处理链协同工作——整体设计哲学

```
raw ──→ dif = baseline − raw           [空间分离: 绝对值→相对值]
  dif ──→ BLSM 管理 baseline            [慢通道: 基线自适应跟踪环境]
  dif ──→ CMF 移除行共模               [空间滤波: 利用触摸的局部性]
  dif ──→ GridIIR 时序平滑 (若噪声)    [时间滤波: 噪声门控, 零延迟]
  dif ──→ Peak 检测局部最大值           [特征提取: Z8相对+Z1绝对]
```

五个算法协同实现了从"原始电容值"到"触摸候选点"的完整转换。它们的设计遵循三个核心原则：

1. **分工明确，接口单一**：每个阶段只做一件事。BLIIR 只做减法，BLSM 只管理基线，CMF 只移除共模，GridIIR 只做时序平滑，Peak 只找最大值。这种"管道过滤器"架构使每个算法可独立调优和替换。

2. **正常路径零开销**：GridIIR 在无噪声时直通，CMF 中无效行跳过处理，Peak 中 Mode 2 分支不激活。每个阶段都尽可能在理想条件下不做多余计算，确保最佳触摸延迟。

3. **多重安全网**：Z8 和 Z1 双级滤波互补，充电器噪声检查可在任何阶段提前退出，全内嵌 SD 检测切换阈值策略，BLRecal 的多条件滞后判断——系统在每个层级都设有"安全网"，任何单点异常都不会导致整个流水线崩溃。

---

## 15. 屏幕边缘手指修正机制分析

屏幕边缘是容式触摸屏的"弱信号区"——边缘电极的电场分布不对称，导致手指在边缘时信号更弱、坐标更容易畸变、更容易意外离开。原厂算法在流水线的**五个不同层级**部署了边缘修正机制，从信号检测到坐标输出形成完整闭环。

### 15.0 边缘判定基础

#### 面板 Toe 特性检测

`TouchThold_ToeUseSpecialThold()` 通过项目名判断是否启用特殊 toe 阈值：
```
if projectName in {"P116", "P150", "P186"}: return true
else: return false
```
"Toe"（趾端）指面板的物理边缘区域。这些特定华为面板型号的边缘电场特性需要特殊处理。

#### 边缘距离计算

`Touch_SetEdgeDist` 计算每个触摸点到最近边缘的距离，存储于触摸结构体中：

```
touch->dim1EdgeDist (0x22c) = min(compensatedX − leftBoundary, rightBoundary − compensatedX)
touch->dim2EdgeDist (0x22e) = min(compensatedY − topBoundary,  bottomBoundary − compensatedY)
```

边界值来自 `g_tsaPrmtFlashConst`:
| 字段 | 偏移 | 含义 |
|---|---|---|
| `leftBoundary` | `+0x2a` | dim1 左边界 |
| `rightBoundary` | `+0x2c` | dim1 右边界 |
| `topBoundary` | `+0x2e` | dim2 上边界 |
| `bottomBoundary` | `+0x30` | dim2 下边界 |

---

### 15.1 第一层：峰值检测 — 边缘阈值差异化

这是信号进入触摸候选之前的**第一道防线**，位于流水线的 Peak Detection 阶段。

**W273AS2700 配置**: `Peak_IsToExcludeSideRegion()` 硬编码返回 `0`——侧边区域**不排除**，即边缘列和内部列同样参与峰值检测。

当 toe 面板 + 特殊阈值启用时，在 `Peak_DetectInRange` 的 3×3 扫描中：

```
对每个网格位置 (col, row):
    if (toe面板 AND 处于边缘列):
        effectiveThreshold = g_tsaPrmtDynamic[10]   ← toe专用阈值
    else:
        effectiveThreshold = g_tsaPrmtDynamic[8]    ← 标准动态阈值
    
    if dif[col,row] >= effectiveThreshold AND 是局部最大值:
        Peak_Insert(col, row)
```

**设计意图**: 边缘列的电场强度通常弱于中央列。独立的 toe 阈值允许边缘使用更低的检测门槛，防止边缘手指因信号弱而被漏检。这不是坐标修正，而是确保边缘触摸**能被检测到**的前提保障。

---

### 15.2 第二层：BLSM 基线 — 边缘优化更新

位于信号处理链的 BLSM 阶段 7-8：

```
BLSM_ProcessStage: stages 7,8

IF (全局边缘标志 OR BLSM_ProcessUseNextSideThold()):
    使用 flash[0x4c] 作为阈值   ← 宽阈值（边缘场景）
ELSE:
    使用 flash[0x4a] 作为阈值   ← 窄阈值

BLIIR_Update(stage=0, threshold, flash[0x52])
```

`BLSM_ProcessUseNextSideThold()` 的条件链：
```
TouchThold_ToeUseSpecialThold()  ← P116/P150/P186 面板
AND (g_tsaPrpt->bNextSideThreshCol == 1  OR  bNextSideThreshCol == bCols - 2)
```

即仅在**面板支持 toe** 且**当前活动列为边缘列**时启用宽阈值。

**设计意图**: 边缘列的电场波动天然大于中央列。使用更宽的基线更新阈值防止正常的边缘信号起伏被误判为"触摸"而冻结基线更新。这确保边缘区域的基线仍能在触摸间隙得到更新，避免长期累积漂移。

在 `BLSM_GetProperty` 中，边缘小阈值标志 `g_BLSMNextEdgeSmallThresh` 还会影响属性判定的信号范围条件，进一步调整边缘列的基线行为。

---

### 15.3 第三层：CMF — 边缘行特殊阈值

在 `CMF_ProcessDim` 逐行处理中：

```
对每行 row:
    if (特殊toe阈值启用 AND (row < 2 OR row >= totalRows - 2)):
        effectiveThreshold = 200
    else:
        effectiveThreshold = g_tsaPrmtDynamic[0x3c]   ← 标准CMF阈值
    
    CMF_ProcessDimUnit(dim=1, row, effectiveThreshold, ...)
```

**设计意图**: 最边缘的 2 行（上下各 2 行）的共模特性与内部行显著不同——边缘行的传感器电极只有一侧有邻居，导致其共模波动幅度更大。使用固定高阈值 200 防止 CMF 在这几行过度修正。200 是一个远大于正常阈值的常数，意味着边缘行的共模修正几乎被抑制。

---

### 15.4 第四层：CTD_EC — 坐标边缘补偿（核心算法）

这是边缘修正中最数学化的一层，直接修改触摸点的输出坐标。它补偿的是**传感器边缘电场畸变**导致的坐标非线性。

#### 15.4.1 触发条件

`CTD_ECUpdatePosToEC` 先调用 `Touch_SetEdgeDist` 更新边缘距离，然后 `CTD_ECProcessDim1` 和 `CTD_ECProcessDim2` 分别处理 dim1 (X) 和 dim2 (Y)。

每个维度的补偿由 `field_0x204` 标志位控制：

| 标志 | 含义 | 补偿方向 |
|---|---|---|
| `bit 0 (1)` | dim1 近边（左边缘） | 从左边界向内补偿 |
| `bit 1 (2)` | dim1 远边（右边缘） | 从右边界向内补偿 |
| `bit 2 (4)` | dim2 近边（上边缘） | 从上边界向内补偿 |
| `bit 3 (8)` | dim2 远边（下边缘） | 从下边界向内补偿 |

每帧先清除 `0x100`/`0x200` 标志（`field_0x1f0`），仅在补偿实际改变了坐标时才重新设置。

#### 15.4.2 补偿参数加载

从 `g_tsaPrmtFlash[1]` 的子区域加载 16 字节运行时参数表到 `g_tsaPrmtRam`：

| 补偿方向 | Flash 源字段 |
|---|---|
| dim1 近边 | `flash[1].field_0x00..0x10` |
| dim1 远边 | `flash[1].field_0x11..0x21` |
| dim2 近边 | `flash[1].field_0x22..0x32` |
| dim2 远边 | `flash[1].field_0x33..0x43` |

参数表结构（`g_tsaPrmtRam`, 每 4 字节一个分段）:
```
[0]: count (有效分段数 - 1)
每分段 [4*i .. 4*i+3]:
    [0]: 未使用
    [1]: sizeThreshold  — 触摸尺寸阈值 (byte)
    [2]: offsetBase     — LUT 基索引 (byte)
    [3]: offsetNext     — LUT 下一索引 (byte)
```

#### 15.4.3 补偿强度计算 — CTD_ECGetOffset

输入: `edgeDist`（触摸点边缘距离，来自 `field_0x196`/`0x197`）、`touchSize`（触摸面积，来自 `field_0x194`）

```
1. 按触摸尺寸查分段:
   在 g_tsaPrmtRam 中找到第一个 sizeThreshold >= touchSize 的分段

2. 分段线性插值:
   查 256 项 LUT 表 g_ctd256Ln:
   baseVal = g_ctd256Ln[offsetBase]     ← 查表 (16-bit 值)
   nextVal = g_ctd256Ln[offsetNext]     ← 查表

   IF nextVal == baseVal:
       offset = 0
   ELSE:
       offset = ((g_ctd256Ln[edgeDist] - baseVal) * 256) / (nextVal - baseVal)
       offset = CLAMP(offset, 0, 255)
```

`g_ctd256Ln` 是一个 256 项的 16-bit 查找表，编码了边缘距离到补偿强度之间的非线性映射曲线。通过分段参数，算法根据触摸尺寸选择不同的曲线段，实现了**尺寸相关的边缘补偿**——大手指和小手指在边缘受到的电场畸变程度不同，需要不同强度的补偿。

#### 15.4.4 坐标修正公式 — CTD_ECGetFinalOffset

这是将补偿强度施加到坐标上的**二次混合函数**：

```
输入: rawPos  — 相对边界的原始位置（近边模式: rawPos = touchX - leftBoundary; 远边模式: rawPos = rightBoundary - touchX）
      offset — 补偿强度 (0..255)

计算:
    t = rawPos - 256           // t < 0 表示还在边缘影响区内
    IF t > 0 AND t < 64:       // rawPos ∈ (256, 320)，即距离边界 0~64 单位
        // 二次混合: 边缘补偿权重的平滑衰减
        weight_raw    = 4 * t                       // 原始位置权重 (随距离线性增长)
        weight_offset = 256 - 4 * t                 // 补偿权重 (随距离线性衰减)
        result = (rawPos * t * 4 + weight_offset * offset) / 256
    ELSE:
        result = rawPos          // 超出补偿范围，直通
```

**混合函数解析**:

这是两个二次项的线性组合。展开来看（令 `t = rawPos - 256`, `c = offset`）:

```
result = [4·rawPos·t + (256 − 4t)·c] / 256
       = [4·rawPos·(rawPos−256) + (256 − 4(rawPos−256))·c] / 256
       = [4·rawPos² − 1024·rawPos + (1280 − 4·rawPos)·c] / 256
```

当 `rawPos = 256` 时，`t = 0`：
- `weight_raw = 0`, `weight_offset = 256`
- `result = (0 + 256·c) / 256 = c` → 全补偿

当 `rawPos = 320` 时，`t = 64`：
- `weight_raw = 256`, `weight_offset = 0`
- `result = (320·64·4 + 0·c) / 256 = 320` → 无补偿

在 `[256, 320]` 区间内平滑过渡，距离边界越远，补偿越弱，到 64 单位后完全退出。

**最终坐标输出** (以 dim1 近边模式为例):
```
compensatedX = rawPos_compensated + leftBoundary
if compensatedX != originalX:
    field_0x1f0 |= 0x100     ← 标记此维度已补偿
```

#### 15.4.5 远边对称性

远边模式（右/下边缘）的处理完全对称，只是方向反转：
```
rawPos = rightBoundary - touchX     ← 翻转方向
compensated = CTD_ECGetFinalOffset(rawPos, 256 - offset)
finalX = rightBoundary - compensated  ← 翻转回来
```

---

### 15.5 第五层：ER (Edge Rejection) — 边缘触摸状态机

这是位于 TS/TE 之后的**后处理层**，不修改信号或坐标本身，而是验证边缘触摸的合法性和状态转换。

#### 15.5.1 新触摸边缘校验 — ER_TouchDownProcess

每个新检测到的触摸（标志 `0x02`）在边缘处需经过额外验证：

```
1. ER_BottomMoveInCorrection(touchIdx, prevIdx)
   → 底部边缘特定修正检查

2. IF NOT ER_IsTouchNeedMoveInCorrection(touchIdx):
      跳过（不需要边缘修正）
   
3. IF prevTouch->moveInFrameCount < 2:
      前2帧：标记为边缘暂缓触摸
      field_0x1f8 |= 0x400     ← 边缘暂缓标志
      field_0x21c = 1           ← 状态 = 等待确认
      prevTouch->moveInFrameCount++
      IF touch->edgeZoneRadius < 2:
          touch->edgeZoneRadius = 2
   
4. ELIF ER_MoveInCheck(touchIdx, prevIdx):
      第3帧起：校验是否为合法边缘移入
      校验通过 → field_0x1f0 |= 0x20; field_0x1f8 |= 0x80
      校验不通过 → 继续暂缓

5. 若已有 0x400 标志（之前已被暂缓）：
      清除 0x400，重新评估底部移入和常规移入
```

**ER_IsTouchNeedMoveInCorrection** 的判定条件：
```
edge = GetDimEdge(compensatedX, compensatedY)
GetPrmt(edge, &minDist, &maxDist)
IF minDist == 0: 跳过（此边缘类型不需要修正）
IF TPSensor_IsNoMoveInCorrection(): 跳过
IF minDist < distToEdge < maxDist AND touchAge < 30帧:
    return true  ← 需要边缘移入修正
```

**ER_MoveInCheck** 的多条件轨迹验证：
```
从历史缓冲取前2帧和当前帧的坐标

historyDist_2to1 = SQDist(帧-2, 帧-1)      ← 前两帧间的移动量
historyDist_1to0 = SQDist(帧-1, 当前)       ← 最近帧间移动量
totalDist = SQDist(帧-2, 当前)              ← 总位移量
edgeMoveDist = SQDistToEdge(帧-2, 帧-1)     ← 朝边缘方向的移动分量

条件1: historyDist_2to1 >= 2500 (50²)      ← 前两帧有足够移动
条件2: historyDist_1to0 >= 2500             ← 最近帧也有足够移动
条件3: totalDist >= 10000 (100²)            ← 总位移足够大
条件4: totalDist >= edgeMoveDist             ← 总位移朝向边缘方向

全部满足 → 合法边缘移入，执行 ER_MoveInCorrection
任一不满足 → 拒绝（可能是噪声/误触）
```

#### 15.5.2 边缘移出检测 — ER_MoveOutCheck

检测已跟踪的触摸是否正在移出屏幕：

```
对 dim1 (X轴):
    totalX = prevMoveDistX + prevCompensatedX + prevAccumDistX
    
    IF totalX < leftBoundary:
        prevTouch->field_0x204 |= 0x10    ← 左边缘移出
        detected = true
    ELIF totalX >= rightBoundary - 1:
        prevTouch->field_0x204 |= 0x20    ← 右边缘移出
        detected = true

对 dim2 (Y轴):  同上，标志 0x40 (上) / 0x80 (下)
```

#### 15.5.3 移出坐标钳制 — ER_MoveOutCorrection

当移出被确认后，强制钳制输出坐标：

```
IF leftMoveOut (0x10):   compensatedX = leftBoundary
IF rightMoveOut (0x20):  compensatedX = rightBoundary - 1
IF topMoveOut (0x40):    compensatedY = topBoundary
IF bottomMoveOut (0x80): compensatedY = bottomBoundary - 1
```

坐标被钳制到边界上，防止报告超出屏幕范围的坐标。

#### 15.5.4 ER_Process 中的状态清理

```
对每个触摸:
    IF 状态 == 8 OR 0x20 AND 上一帧有边缘标志(0x08):
        IF ER_MoveOutCheck 检测到移出:
            field_0x1f0 |= 0x40          ← 标记移出
            ER_MoveOutCorrection         ← 钳制坐标
            IF 状态 == 0x20:
                field_0x21c = 0x10       ← 状态降级
```

---

### 15.6 五层修正协同全景

```
┌─────────────────────────────────────────────────────┐
│  层级        位置              修正内容              │
├─────────────────────────────────────────────────────┤
│  ① Peak      信号→候选点      边缘独立阈值          │
│              确保边缘触摸可被检测                    │
├─────────────────────────────────────────────────────┤
│  ② BLSM      信号处理         边缘宽阈值基线更新    │
│              防止边缘波动误冻结基线                  │
├─────────────────────────────────────────────────────┤
│  ③ CMF       信号处理         边缘行共模抑制减弱    │
│              避免边缘行过度修正                      │
├─────────────────────────────────────────────────────┤
│  ④ CTD_EC    坐标变换         二次混合边缘补偿      │
│              修正电场畸变导致的坐标非线性             │
├─────────────────────────────────────────────────────┤
│  ⑤ ER        触控跟踪         边缘触摸合法性验证    │
│              防误触/移出钳制/移入验证                 │
└─────────────────────────────────────────────────────┘
```

**设计哲学**: 五层修正形成了一个**纵深防御体系**。前三层（Peak/BLSM/CMF）确保边缘信号能以正确的形态进入检测流程——属于"信号层面"的修正。第四层（CTD_EC）在坐标变换时补偿物理畸变——属于"几何层面"的修正。第五层（ER）在触摸跟踪时验证边缘事件的合法性——属于"语义层面"的修正。三个层面互不重叠、缺一不可，共同保证边缘手指在全流程中得到正确处理。

**W273AS2700 的关键差异**: `Peak_IsToExcludeSideRegion()` 返回 `0`（不排除边缘），同时面板名称不匹配 P116/P150/P186 导致 `TouchThold_ToeUseSpecialThold()` 也返回 `0`。这意味着**第①②③层在 W273AS2700 上实际被大幅削减**——边缘专用阈值、宽基线步长、CMF 边缘阈值 200 均不生效。该面板主要依赖第④层（CTD_EC 坐标补偿）和第⑤层（ER 边缘验证）来校正边缘触摸行为。
