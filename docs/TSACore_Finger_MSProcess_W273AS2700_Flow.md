# TSACore 手指触摸算法流 — W273AS2700 当前启用路径

> 目标：记录 `TSACore.dll` 中 `TSA_MSProcess` 的手指触摸主流水线。  
> 范围：只按 `tsaprmt-table-reader` 导出的 Gaokun Himax CSOT / `W273AS2700` 表参数判断当前实际启用路径。

## 1. 帧级入口

`TSA_MSProcess` 是手指 mutual-sensing 主算法入口，但不是整帧最外层入口。整帧入口关系如下：

```text
TSA_Processing @ 6ba9a443
  ├─ TSAFold_InputProcess
  ├─ AFE_PreProcess
  ├─ TSAStatic_UpdateCtrlFlags
  ├─ BLSM_PreProcess
  ├─ TSA_MSProcess @ 6ba90e43        // 手指 mutual-sensing 主流水线
  ├─ Proximity_TouchPostProcess
  ├─ SideTouch_Process
  ├─ TSA_ASAProcess                  // stylus/ASA，MS 之后的 sibling pipeline
  ├─ TSAOut_PostProcess
  ├─ AFE_Process
  └─ TSA_PitchSizeRemapProcess       // 对最终 touch 坐标做 pitch-size remap
```

## 2. 当前表参数对手指路径的关键影响

`flash` 表关键字段：

```text
bCols / bRows             = 0x3c / 0x28 = 60 x 40
wBufElementCount          = 0x0960 = 2400
dwPeakProcessingMode      = 3
bCmfForceDim1Pass         = 1
bCmfEnabled               = 0
wPeakFilterReferenceThold = 0x0bb8 = 3000
wPeakMode3ThresholdMax    = 0x0258 = 600
dwCmfMethodNormal         = 0
dwCmfMethodSmartCover     = 2
dwAftModeFeatureMask      = 0
Dim1 pitch map            = active
Dim2 pitch map            = sentinel / no-op
```

由这些参数得到当前手指路径：

```text
启用：
- raw check / raw process
- SS_Process
- baseline diff: BLIIR_CalcDiffCommon
- BLSM_Process
- fallback CMF_Process
- CMF_ProcessDim(1)，即 Dim1 一维 CMF
- GridIIR_Process
- Peak_Process mode 3
- finger final Dim1 pitch remap

关闭或当前不展开：
- Rawdata_CMF，因 bCmfEnabled = 0
- CMF_Filter2D / CMF PCA，因 dwCmfMethodNormal = 0
- SafeBaseline，feature flag off
- UnderWater detect，feature flag off
- SignalDisparity_PostProcess，feature flag off
- AFT_Process，feature flag off / mask = 0
- EdgePeakFilter_WorkAround，dwFeatureFlags2 bit0 off
- Dim2 pitch remap，map sentinel 使其等价 no-op
```

## 3. `TSA_MSProcess` 主流程

```text
TSA_MSProcess @ 6ba90e43
  ├─ 标记 PROC_MS_ACTIVE
  ├─ DataSwitch_ToGrid
  ├─ AFE_GetFrame
  ├─ SS_CopyRaw(frame + 0x18, bRows + bCols)
  ├─ raw pointer / SS raw validity check
  │   └─ invalid: optional TSA_MSReset / SS_Process / TSA_MSProcessEnding
  └─ normal path
      ├─ TSAIDE_CleanPointXY
      ├─ HardwareAnalyzer_Reset
      ├─ optional Proximity_Process
      ├─ Touch_Clean
      ├─ 保存上一帧 dif/raw
      ├─ TSA_RawCheckProcess
      ├─ copy input raw -> g_tsaBufRaw 或 g_tsaBufDif
      ├─ Rawdata_Process
      ├─ HardwareAnalyzer_Process
      ├─ TSAPrmt_PreProcess
      ├─ SS_Process
      ├─ copy g_tsaBufRaw -> g_tsaBufPreCMFRaw
      ├─ BLIIR_CalcDiffCommon
      ├─ TSAPrpt_GetDifPreCMFPrpt
      ├─ BLSM_UpdateForRawUnstable
      ├─ BLSM_Process
      ├─ CMF_Process
      │   └─ CMF_ProcessDim(1)
      ├─ GridIIR_Process
      ├─ HardwareAnalyzer_ProcessDif
      ├─ TPSensor_Process
      ├─ TSA_GetPrpt
      ├─ Self_Process
      ├─ ToeSynaBl_GetSidePrpt
      ├─ SS_CheckDirtyByMutual
      ├─ TSA_MSRawDirectionDectect
      ├─ Exception_CheckChargerNoiseInRxLines
      ├─ SS_ChargerNoiseFilterProcess
      ├─ TSAPrmt_Process
      ├─ SignalDisparity_Process
      ├─ Peak_Process
      ├─ optional SS_ChargerNoiseFilterSwitchBuffer -> re-Peak_Process
      ├─ TSA_MSPeakFilter
      ├─ Exception_CheckPanelSD
      ├─ GripFilter_RegionProcess
      ├─ TSABuffer_PrevFrameProcess
      ├─ TZ_Process
      ├─ TSA_MSTouchPreFilter
      ├─ CTD_ECProcess
      ├─ IDT_Process(0)
      ├─ Touch_AssignPreIdxBasedOnID
      ├─ PrevTouch_AssignCurIdxBasedOnID
      ├─ TS_Process
      ├─ TE_Process
      ├─ TSABuffer_PreProcess
      ├─ ER_Process
      ├─ TouchAction_Process
      ├─ TSA_MSTouchPostFilter
      ├─ TS_PostProcess
      ├─ TouchAction_PostProcess
      ├─ Exception_Process
      ├─ Gesture_Process
      ├─ GripFilter_Process
      ├─ TouchMode_Process
      ├─ SS_ChargerNoiseFilterPostProcess
      ├─ AntiTouch_Process
      ├─ Touch_ProcessBetaGrip
      ├─ TouchReport_Process
      ├─ Touch_ProcessExtInfo
      ├─ HardwareAnalyzer_PostProcess
      ├─ HandGesture_Process
      ├─ BigData_RecordProcess
      ├─ BigData_StatProcess
      ├─ PrevTouch_PostProcess
      ├─ PrevPeak_Process
      ├─ TSABuffer_PostProcess
      └─ TSA_MSProcessEnding
```

## 4. 当前启用路径的核心数据流

```text
pMutualRawData
  -> g_tsaBufRaw
  -> Rawdata_Process
  -> SS_Process
  -> BLIIR_CalcDiffCommon(g_tsaBufDif, g_tsaBufRaw, g_tsaBufBl)
  -> BLSM_Process 更新 baseline 状态
  -> CMF_Process / CMF_ProcessDim(1) 抑制 common-mode noise
  -> GridIIR_Process 视噪声状态做 dif IIR
  -> Peak_Process mode 3 提取候选峰
  -> TZ / CTD / IDT / TS / TE 生成 touch objects
  -> TouchReport_Process 生成最终上报
  -> TSA_PitchSizeRemapProcess 做 Dim1 pitch remap
```

## 5. 重点子算法定位

| 函数 | 地址 | 当前路径中的角色 |
|---|---:|---|
| `Rawdata_Process` | `0x6ba69581` | 对输入 raw grid 做归一化或 scale，准备后续 raw/dif 计算。 |
| `SS_Process` | `0x6bb00e4e` | 处理 self-sensing/side-channel 原始数据，生成 SS dif / property。 |
| `BLIIR_CalcDiffCommon` | `0x6ba46f4c` | 用 baseline 与 raw 逐点相减，生成 mutual dif buffer。 |
| `BLSM_Process` | `0x6ba4c645` | baseline state machine，驱动 baseline recal/update/stage/shb 处理。 |
| `CMF_Process` | `0x6ba4f2c8` | 当前 fallback CMF；因表参数选择 Dim1 一维 CMF。 |
| `GridIIR_Process` | `0x6ba61098` | 全频噪声状态下对 dif grid 做 IIR 平滑或复制旁路。 |
| `Peak_Process` | `0x6ba68c4b` | 当前 mode 3；Z8 filter + Z1 fallback + peak 排序/ID tracking。 |

## 6. 当前配置下的一句话总结

`W273AS2700` 的手指路径是：`raw -> Rawdata_Process -> SS_Process -> baseline diff -> BLSM -> fallback Dim1 CMF -> GridIIR -> Peak mode 3 -> touch tracking/report -> Dim1 pitch remap`。其中 2D CMF/PCA、Rawdata_CMF、SafeBaseline、UnderWater、SignalDisparity post-compensation、AFT 都不是当前主路径。 
