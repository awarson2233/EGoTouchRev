#pragma once
// ============================================================================
// TSAFuncDecls.h
// Unified Forward Declarations for all active TSACore functions
// Total functions: 2009
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

// AIIR_GetCoef @ 0x6ba60040 (Filter\AIIR.c)
char AIIR_GetCoef(ushort param_1);





















// AdjustByHistory @ 0x6baf540d (Tracking\HotZoneAdjust.c)
void AdjustByHistory(byte param_1,byte param_2);



















// BLIIR_CalcDiffCommon @ 0x6ba46f4c (Baseline\BLIIR.c)
void BLIIR_CalcDiffCommon(short *pDifOut,short *pRawGrid,short *pBaselineGrid);
























// BLReset_InDebounce @ 0x6ba47750 (Baseline\BLReset.c)
bool BLReset_InDebounce(void);


































// BLReset_Status @ 0x6ba4776c (Baseline\BLReset.c)
undefined1 BLReset_Status(void);






// BLSM_Init @ 0x6ba4c220 (Baseline\BLSM.c)
void BLSM_Init(undefined1 param_1);

// BLSM_IsBaselineOK @ 0x6ba4d7c6 (Baseline\BLSM.c)
undefined8 BLSM_IsBaselineOK(void);

// BLSM_IsOKToIDLE @ 0x6ba4d7fd (Baseline\BLSM.c)
undefined4 BLSM_IsOKToIDLE(void);

// BLSM_IsReset @ 0x6ba4d794 (Baseline\BLSM.c)
undefined4 BLSM_IsReset(void);


// BLSM_PreProcess @ 0x6ba4c36b (Baseline\BLSM.c)
void BLSM_PreProcess(void);

// BLSM_Process @ 0x6ba4c645 (Baseline\BLSM.c)
void BLSM_Process(bool bHardwareReset,bool bForceBaselineProperty);








// BLSM_UpdateForRawUnstable @ 0x6ba4c27b (Baseline\BLSM.c)
void BLSM_UpdateForRawUnstable(void);













// Baseline_Compensation @ 0x6ba46d00 (Baseline\Baseline.c)
void Baseline_Compensation(void);





// CMF_GetCmfMethod @ 0x6ba4d903 (Signal\CMF.c)
uint CMF_GetCmfMethod(void);








// CMF_Process @ 0x6ba4f2c8 (Signal\CMF.c)
void CMF_Process(void);





// CMF_ProcessDimCommonOnBuf @ 0x6ba4eb1d (Signal\CMF.c)
void CMF_ProcessDimCommonOnBuf (undefined8 param_1,undefined8 param_2,int param_3,short param_4,short param_5, byte param_6,byte param_7,undefined1 param_8);






// CTD_BasicCalculation @ 0x6ba551e0 (Tracking\CTD.c)
longlong CTD_BasicCalculation(int param_1,int param_2);




// CTD_ECProcess @ 0x6ba55ca0 (Tracking\CTD.c)
void CTD_ECProcess(void);





// CTD_General @ 0x6ba5523c (Tracking\CTD.c)
void CTD_General(byte param_1,byte param_2,byte param_3,byte param_4,undefined2 *param_5, undefined2 *param_6);




















// DB_LOSize @ 0x6ba55e20 (Tracking\DB.c)
ushort DB_LOSize(void);

// DB_LiftOffNum @ 0x6ba55e79 (Tracking\DB.c)
ushort DB_LiftOffNum(byte param_1);

// DB_TouchDownNum @ 0x6ba5681c (Tracking\DB.c)
ushort DB_TouchDownNum(byte param_1,byte param_2);



// DataSwitch_ToHand @ 0x6ba907ce (Core\DataSwitch.c)
void DataSwitch_ToHand(void);





// ER_GetLimit @ 0x6ba57101 (Tracking\ER.c)
ushort ER_GetLimit(void);


// ER_Init @ 0x6ba56f76 (Tracking\ER.c)
void ER_Init(void);








// ER_Process @ 0x6ba584b3 (Tracking\ER.c)
void ER_Process(void);


// Exception_CheckChargerNoiseInRxLines @ 0x6ba59214 (Detection\Exception.c)
void Exception_CheckChargerNoiseInRxLines(void);

// Exception_CheckPanelSD @ 0x6ba59325 (Detection\Exception.c)
void Exception_CheckPanelSD(void);




// Exception_GetDetectedChargerNoise @ 0x6ba592e6 (Detection\Exception.c)
undefined8 Exception_GetDetectedChargerNoise(void);




// Exception_IsChargerNoiseDetected @ 0x6ba592a7 (Detection\Exception.c)
undefined8 Exception_IsChargerNoiseDetected(void);


// Exception_Process @ 0x6ba58f25 (Detection\Exception.c)
void Exception_Process(void);

// Exception_ProcessTouchZone @ 0x6ba591d1 (Detection\Exception.c)
void Exception_ProcessTouchZone(undefined8 param_1);



































// Filter_ProcessUnit @ 0x6ba5fd96 (Filter\Filter.c)
void Filter_ProcessUnit(byte param_1,undefined1 param_2);








// GridIIR_Process @ 0x6ba61098 (Signal\GridIIR.c)
void GridIIR_Process(void);











































































































































































































// HardwareAnalyzer_GenerateExceptionDescription @ 0x6baf001a (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_GenerateExceptionDescription(void);


// HardwareAnalyzer_GetDetectedException @ 0x6baf0249 (Signal\HardwareAnalyzer.c)
int HardwareAnalyzer_GetDetectedException(void);






// HardwareAnalyzer_GetRaw2RawPrpt @ 0x6baf10dc (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_GetRaw2RawPrpt(void);










// HardwareAnalyzer_IsToBypassCurrentFrame @ 0x6baf14ff (Signal\HardwareAnalyzer.c)
undefined1 HardwareAnalyzer_IsToBypassCurrentFrame(void);







// HardwareAnalyzer_Process @ 0x6baf19de (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_Process(void);
















// HardwareAnalyzer_SetSoftException @ 0x6baf0124 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_SetSoftException(void);




















// IDT_Init @ 0x6ba611a0 (Tracking\IDT.c)
void IDT_Init(void);





// IDT_Process @ 0x6ba611a6 (Tracking\IDT.c)
void IDT_Process(uchar bUseStrictAccCheck);

// IIR_Process @ 0x6ba601b0 (Filter\IIR.c)
int IIR_Process(int param_1,int param_2,byte param_3,byte param_4);

// IIR_Process2 @ 0x6ba6021b (Filter\IIR.c)
int IIR_Process2(int param_1,int param_2,byte param_3,byte param_4);











// IsNeedRecord @ 0x6baf6dae (Tracking\HotZoneAdjust.c)
undefined8 IsNeedRecord(byte param_1,longlong param_2);




// Peak_Clean @ 0x6ba659b0 (Detection\Peak.c)
void Peak_Clean(void);

// Peak_DetectInRange @ 0x6ba685d7 (Detection\Peak.c)
void Peak_DetectInRange(short param_1,byte param_2,byte param_3,byte param_4,byte param_5);




// Peak_GetDistToEdge @ 0x6ba67c47 (Detection\Peak.c)
byte Peak_GetDistToEdge(undefined1 param_1);

// Peak_GetDistance @ 0x6ba66bbd (Detection\Peak.c)
byte Peak_GetDistance(byte param_1,byte param_2);

// Peak_GetIdxBySig @ 0x6ba666f3 (Detection\Peak.c)
byte Peak_GetIdxBySig(short param_1);


// Peak_GetSquareDistance @ 0x6ba66c86 (Detection\Peak.c)
int Peak_GetSquareDistance(byte param_1,byte param_2);

// Peak_GetZ9 @ 0x6ba65a72 (Detection\Peak.c)
int Peak_GetZ9(byte param_1);


// Peak_Insert @ 0x6ba65e68 (Detection\Peak.c)
uint Peak_Insert(byte param_1,byte param_2);



// Peak_IsInNxNBox @ 0x6ba67a7e (Detection\Peak.c)
undefined1 Peak_IsInNxNBox(undefined1 param_1,undefined1 param_2,undefined1 param_3);

// Peak_IsOnInnerEdgeDim1 @ 0x6ba66868 (Detection\Peak.c)
undefined8 Peak_IsOnInnerEdgeDim1(undefined1 param_1);






// Peak_IsOnOuterEdge @ 0x6ba66a9d (Detection\Peak.c)
undefined8 Peak_IsOnOuterEdge(undefined1 param_1);

// Peak_IsOnOuterEdgeDim1 @ 0x6ba66a23 (Detection\Peak.c)
undefined8 Peak_IsOnOuterEdgeDim1(undefined1 param_1);






// Peak_IsToExcludeSideRegion @ 0x6ba685ac (Detection\Peak.c)
undefined8 Peak_IsToExcludeSideRegion(void);

// Peak_IsTouchValid @ 0x6ba65a3e (Detection\Peak.c)
undefined4 Peak_IsTouchValid(byte param_1);

// Peak_Process @ 0x6ba68c4b (Detection\Peak.c)
void Peak_Process(void);


// Peak_SortInSigAscending @ 0x6ba66013 (Detection\Peak.c)
void Peak_SortInSigAscending(void);




// Peak_needUDFPLargeFingerWorkaround @ 0x6ba68bc6 (Detection\Peak.c)
undefined8 Peak_needUDFPLargeFingerWorkaround(void);




// PosnSpecial_Get @ 0x6bb42624 (Tracking\PosnSpecial.c)
undefined2 PosnSpecial_Get(void);




// PosnSpecial_GetPosMarker @ 0x6bb423f7 (Tracking\PosnSpecial.c)
char PosnSpecial_GetPosMarker(void);


// PosnSpecial_Init @ 0x6bb422b6 (Tracking\PosnSpecial.c)
void PosnSpecial_Init(undefined8 param_1,undefined2 param_2);





// PosnSpecial_Put @ 0x6bb42569 (Tracking\PosnSpecial.c)
void PosnSpecial_Put(ushort param_1);

















// PrevPeak_Process @ 0x6ba68f77 (Detection\PrevPeak.c)
void PrevPeak_Process(void);



// PrevTouch_GetIdxBasedOnID @ 0x6ba6db56 (Tracking\PrevTouch.c)
byte PrevTouch_GetIdxBasedOnID(char param_1);


// PrevTouch_HistoryBufferGet @ 0x6ba6ee1d (Tracking\PrevTouch.c)
undefined8 PrevTouch_HistoryBufferGet(byte param_1,byte param_2,undefined8 *param_3);


// PrevTouch_HistoryBufferInsert @ 0x6ba6ef44 (Tracking\PrevTouch.c)
void PrevTouch_HistoryBufferInsert(byte param_1,byte param_2,undefined8 *param_3);

// PrevTouch_HistoryBufferPop @ 0x6ba6ec0d (Tracking\PrevTouch.c)
undefined8 PrevTouch_HistoryBufferPop(byte param_1,undefined8 *param_2);

// PrevTouch_HistoryBufferPush @ 0x6ba6ea52 (Tracking\PrevTouch.c)
undefined8 PrevTouch_HistoryBufferPush(byte param_1,undefined8 *param_2);


// PrevTouch_HistoryQueuePush @ 0x6ba6e650 (Tracking\PrevTouch.c)
undefined8 PrevTouch_HistoryQueuePush(byte param_1,ushort param_2,ushort param_3);





// PrevTouch_IsValidId @ 0x6ba6e909 (Tracking\PrevTouch.c)
undefined8 PrevTouch_IsValidId(char param_1);

// PrevTouch_PostProcess @ 0x6ba6e09d (Tracking\PrevTouch.c)
void PrevTouch_PostProcess(void);

// PrevTouch_Release @ 0x6ba6da90 (Tracking\PrevTouch.c)
void PrevTouch_Release(void);


// PrevTouch_UnitCopy @ 0x6ba6d300 (Tracking\PrevTouch.c)
void PrevTouch_UnitCopy(byte param_1,byte param_2);



// PrevTouch_UnitUpdate @ 0x6ba6d5ed (Tracking\PrevTouch.c)
void PrevTouch_UnitUpdate(byte param_1,byte param_2);













// Rawdata_CMF @ 0x6ba69381 (Signal\Rawdata.c)
void Rawdata_CMF(void);






// Rawdata_Process @ 0x6ba69581 (Signal\Rawdata.c)
void Rawdata_Process(void);









// SearchNearest @ 0x6baf4fbe (Tracking\HotZoneAdjust.c)
bool SearchNearest(byte param_1,undefined8 param_2,undefined1 param_3,undefined8 param_4);



















// SigSumFilter_ReserveTouch @ 0x6ba60977 (Signal\SigSumFilter.c)
void SigSumFilter_ReserveTouch(int param_1);












































// TE_Process @ 0x6ba789d9 (Tracking\TE.c)
void TE_Process(void);


























// TSABuffer_GripStageProcess @ 0x6ba8ef57 (Core\TSABuffer.c)
void TSABuffer_GripStageProcess(void);




// TSABuffer_PostProcess @ 0x6ba8f4c1 (Core\TSABuffer.c)
void TSABuffer_PostProcess(void);

// TSABuffer_PreProcess @ 0x6ba8f37f (Core\TSABuffer.c)
void TSABuffer_PreProcess(void);

// TSABuffer_PrevFrameProcess @ 0x6ba8f540 (Core\TSABuffer.c)
void TSABuffer_PrevFrameProcess(void);





// TSAConstPrmt_Init @ 0x6bb3bb00 (Prmt\TSAConstPrmt.c)
void TSAConstPrmt_Init(void);


























// TSAMem_InitProject @ 0x6baa065d (Core\TSABuffer.c)
bool TSAMem_InitProject(void);


// TSAOut_Angle @ 0x6ba6a305 (Core\TSAOut.c)
char TSAOut_Angle(char param_1);

// TSAOut_Axis @ 0x6ba6a2e5 (Core\TSAOut.c)
ushort TSAOut_Axis(short param_1);

// TSAOut_Dim1PosInternal @ 0x6ba69a50 (Core\TSAOut.c)
undefined2 TSAOut_Dim1PosInternal(ushort param_1,ushort param_2);

// TSAOut_Dim2PosInternal @ 0x6ba69ccb (Core\TSAOut.c)
undefined2 TSAOut_Dim2PosInternal(ushort param_1,ushort param_2);


// TSAOut_DisplayXPos @ 0x6ba6a241 (Core\TSAOut.c)
ushort TSAOut_DisplayXPos(undefined1 param_1, ...);



// TSAOut_ExtToIntDist @ 0x6ba69bc3 (Core\TSAOut.c)
ulonglong TSAOut_ExtToIntDist(ushort param_1);

// TSAOut_IntToExtDist @ 0x6ba69bfb (Core\TSAOut.c)
uint TSAOut_IntToExtDist(short param_1);



// TSAOut_PostProcess @ 0x6ba6a77a (Core\TSAOut.c)
void TSAOut_PostProcess(void);




// TSAOut_RevertDim1Pos @ 0x6ba69b2f (Core\TSAOut.c)
uint TSAOut_RevertDim1Pos(ushort param_1);


// TSAOut_RptXPos @ 0x6ba6a264 (Core\TSAOut.c)
ushort TSAOut_RptXPos(undefined1 param_1);








// TSAPrmt_PreProcess @ 0x6bb3bab7 (Prmt\TSAPrmt.c)
void TSAPrmt_PreProcess(void);

// TSAPrmt_Process @ 0x6bb3bacb (Prmt\TSAPrmt.c)
void TSAPrmt_Process(void);





// TSAPrpt_GetDifPreCMFPrpt @ 0x6baa13be (Core\TSAPrpt.c)
void TSAPrpt_GetDifPreCMFPrpt(void);









// TSARTPrmt_SetKeyValue @ 0x6bb3de92 (Prmt\TSARTPrmt.c)
void TSARTPrmt_SetKeyValue(undefined8 param_1,undefined4 param_2);












// TSAStatic_Init @ 0x6baa1a42 (Core\TSAStatic.c)
void TSAStatic_Init(void);








// TSAStatic_IsAFTModeEnabled @ 0x6baa2cda (Core\TSAStatic.c)
undefined4 TSAStatic_IsAFTModeEnabled(void);

// TSAStatic_IsAllFreqBecomeNoisy @ 0x6baa260a (Core\TSAStatic.c)
undefined1 TSAStatic_IsAllFreqBecomeNoisy(void);

// TSAStatic_IsAllFreqNoisy @ 0x6baa2590 (Core\TSAStatic.c)
undefined1 TSAStatic_IsAllFreqNoisy(void);

// TSAStatic_IsChargerPlugged @ 0x6baa2434 (Core\TSAStatic.c)
undefined1 TSAStatic_IsChargerPlugged(void);



// TSAStatic_IsHWReset @ 0x6baa2345 (Core\TSAStatic.c)
bool TSAStatic_IsHWReset(void);








// TSAStatic_IsSafeBaselineApplied @ 0x6baa30f3 (Core\TSAStatic.c)
undefined8 TSAStatic_IsSafeBaselineApplied(void);


// TSAStatic_IsSignalReverted @ 0x6baa2c81 (Core\TSAStatic.c)
uchar TSAStatic_IsSignalReverted(void);



// TSAStatic_PreProcess @ 0x6baa1bb0 (Core\TSAStatic.c)
void TSAStatic_PreProcess(void);



// TSAStatic_SafeDiffCheck @ 0x6baa1de6 (Core\TSAStatic.c)
void TSAStatic_SafeDiffCheck(void);


























// TSA_DisableScenario @ 0x6ba8fa33 (Core\TSA.c)
void TSA_DisableScenario(uint param_1);



// TSA_EnableScenario @ 0x6ba8fa07 (Core\TSA.c)
void TSA_EnableScenario(uint param_1);


// TSA_GetAFEScanFreqIndex @ 0x6ba9f998 (Core\TSA.c)
undefined8 TSA_GetAFEScanFreqIndex(longlong param_1);

// TSA_GetAFEScanRateIndex @ 0x6ba9fa2d (Core\TSA.c)
undefined8 TSA_GetAFEScanRateIndex(longlong param_1);









// TSA_GetFeatures @ 0x6ba9f329 (Core\TSA.c)
TSA_FeatureFlags TSA_GetFeatures(void);

// TSA_GetPrmtName @ 0x6ba9e911 (Core\TSA.c)
char* TSA_GetPrmtName(void);


// TSA_GetSide @ 0x6ba8f7f7 (Core\TSA.c)
undefined4 TSA_GetSide(undefined2 param_1,undefined2 param_2);

// TSA_GetTouchSide @ 0x6ba8f82b (Core\TSA.c)
undefined4 TSA_GetTouchSide(byte param_1);

// TSA_GetXRes @ 0x6ba9aef7 (Core\TSA.c)
ushort TSA_GetXRes(void);

// TSA_GetYLength @ 0x6ba9ae31 (Core\TSA.c)
ushort TSA_GetYLength(void);



// TSA_InStylusMode @ 0x6ba9c771 (Core\TSA.c)
undefined8 TSA_InStylusMode(void);

// TSA_InitAFEProject_UI @ 0x6ba99dc7 (Core\TSA.c)
undefined8 TSA_InitAFEProject_UI(longlong param_1,longlong param_2,longlong param_3);



// TSA_IsGameScenario @ 0x6ba8fe7f (Core\TSA.c)
undefined8 TSA_IsGameScenario(void);






// TSA_IsOKToEnterIDLEMode @ 0x6ba9e755 (Core\TSA.c)
undefined1 TSA_IsOKToEnterIDLEMode(void);







// TSA_IsReCalibNeeded @ 0x6ba9e32b (Core\TSA.c)
undefined1 TSA_IsReCalibNeeded(void);








// TSA_IsWindowsPadFeatureEnabled @ 0x6ba9ef4b (Core\TSA.c)
undefined4 TSA_IsWindowsPadFeatureEnabled(void);







// TSA_MSReset @ 0x6ba90b16 (Core\TSA.c)
void TSA_MSReset(void);






// TSA_ProcessingAFEFrame @ 0x6ba9a7a7 (Core\TSA.c)
int TSA_ProcessingAFEFrame(AFE_Frame *frame,uint ctrl,ulonglong data);










// TSA_RptDiffPtr @ 0x6ba9c5a8 (Core\TSA.c)
undefined8 TSA_RptDiffPtr(void);

// TSA_RptIsLiftOff @ 0x6ba9b1b4 (Core\TSA.c)
uint TSA_RptIsLiftOff(byte param_1);

// TSA_RptIsTouchDown @ 0x6ba9b112 (Core\TSA.c)
uint TSA_RptIsTouchDown(byte param_1);




// TSA_RptTouchID @ 0x6ba9b256 (Core\TSA.c)
int TSA_RptTouchID(byte param_1);

// TSA_RptTouchNum @ 0x6ba9b09c (Core\TSA.c)
uint TSA_RptTouchNum(void);

// TSA_RptTouchPressure @ 0x6ba9bc34 (Core\TSA.c)
uint TSA_RptTouchPressure(byte param_1);


// TSA_RptTouchXPos @ 0x6ba9b428 (Core\TSA.c)
ulonglong TSA_RptTouchXPos(byte param_1);



// TSA_RptTouchYPos @ 0x6ba9b482 (Core\TSA.c)
ulonglong TSA_RptTouchYPos(byte param_1);











// TSA_SetFeatures @ 0x6ba9f179 (Core\TSA.c)
undefined4 TSA_SetFeatures(TSA_FeatureFlags param_1);



// TSA_SetResScale @ 0x6ba9af5f (Core\TSA.c)
undefined8 TSA_SetResScale(undefined2 param_1);

// TSA_SetScreenRotateState @ 0x6ba9fea4 (Core\TSA.c)
void TSA_SetScreenRotateState(int param_1);






// TSA_SetXRes @ 0x6ba9b010 (Core\TSA.c)
void TSA_SetXRes(undefined2 param_1);

// TSA_SetYRes @ 0x6ba9b056 (Core\TSA.c)
void TSA_SetYRes(undefined2 param_1);


// TSA_SideToEdge @ 0x6ba8f66a (Core\TSA.c)
undefined8 TSA_SideToEdge(uint param_1);











// TS_MMToDisplayPixel @ 0x6ba7c436 (Tracking\TS.c)
uint TS_MMToDisplayPixel(byte param_1);

// TS_MMToPixel @ 0x6ba7c416 (Tracking\TS.c)
int TS_MMToPixel(byte param_1);

// TS_PixelToMM @ 0x6ba7c561 (Tracking\TS.c)
int TS_PixelToMM(ushort param_1);


// TS_PostProcess @ 0x6ba7c368 (Tracking\TS.c)
void TS_PostProcess(void);

// TS_Process @ 0x6ba7c2cb (Tracking\TS.c)
void TS_Process(void);











































































































































// TZ_Process @ 0x6ba8c9f0 (Detection\TZ.c)
void TZ_Process(void);







































































// TouchAction_PostProcess @ 0x6ba773eb (Tracking\TouchAction.c)
void TouchAction_PostProcess(void);













// TouchMode_Process @ 0x6ba78dde (Filter\TouchMode.c)
void TouchMode_Process(void);






























// TouchThold_Init @ 0x6ba7c922 (Tracking\TouchThold.c)
void TouchThold_Init(void);

// TouchThold_IsSwitching @ 0x6ba7c90e (Tracking\TouchThold.c)
undefined1 TouchThold_IsSwitching(void);

// TouchThold_Process @ 0x6ba7d0ee (Tracking\TouchThold.c)
void TouchThold_Process(short param_1,short param_2,short *param_3,short *param_4, undefined2 *param_5);


// TouchThold_ToeUseSpecialThold @ 0x6ba7cd8f (Tracking\TouchThold.c)
undefined8 TouchThold_ToeUseSpecialThold(void);






// Touch_Clean @ 0x6ba6f49c (Tracking\Touch.c)
void Touch_Clean(void);

// Touch_CreatePostReport @ 0x6ba76a03 (Tracking\Touch.c)
void Touch_CreatePostReport(byte param_1, byte param_2);



// Touch_GetDistToLogicEdge @ 0x6ba73009 (Tracking\Touch.c)
short Touch_GetDistToLogicEdge(int param_1,short param_2,short param_3);


// Touch_GetFormattedTimeInterval @ 0x6ba76329 (Tracking\Touch.c)
short Touch_GetFormattedTimeInterval(short param_1);













// Touch_GetSQDistPixel @ 0x6ba73447 (Tracking\Touch.c)
uint Touch_GetSQDistPixel(ushort param_1,ushort param_2,ushort param_3,ushort param_4);


// Touch_GetSQDistPixelToEdge @ 0x6ba7329a (Tracking\Touch.c)
int Touch_GetSQDistPixelToEdge(short param_1,short param_2,short param_3,short param_4);

// Touch_GetTimeOn @ 0x6ba6f517 (Tracking\Touch.c)
int Touch_GetTimeOn(byte param_1);











// Touch_IsException @ 0x6ba7416a (Tracking\Touch.c)
undefined1 Touch_IsException(byte param_1,byte param_2);


// Touch_IsFullyLiftOff @ 0x6ba717cf (Tracking\Touch.c)
bool Touch_IsFullyLiftOff(byte param_1);

// Touch_IsGhostByMinSigOff @ 0x6ba7160b (Tracking\Touch.c)
undefined8 Touch_IsGhostByMinSigOff(byte param_1);

// Touch_IsGhostByMinSigOff2 @ 0x6ba7173d (Tracking\Touch.c)
bool Touch_IsGhostByMinSigOff2(byte param_1);

// Touch_IsGrip @ 0x6ba742be (Tracking\Touch.c)
char Touch_IsGrip(undefined1 param_1);



// Touch_IsOff @ 0x6ba74ab5 (Tracking\Touch.c)
char Touch_IsOff(byte param_1);

// Touch_IsOffEvent @ 0x6ba74a8e (Tracking\Touch.c)
undefined8 Touch_IsOffEvent(int param_1);


// Touch_IsSideCoreInSameZone @ 0x6ba71415 (Tracking\Touch.c)
undefined8 Touch_IsSideCoreInSameZone(byte param_1,byte param_2);



// Touch_KeepPrevTouchWithExceptionExcluded @ 0x6ba766b9 (Tracking\Touch.c)
void Touch_KeepPrevTouchWithExceptionExcluded(void);


// Touch_NeedXYFilter @ 0x6ba742dc (Tracking\Touch.c)
bool Touch_NeedXYFilter(byte param_1,byte param_2);

// Touch_ProcessAftTouchEvent @ 0x6ba75af5 (Tracking\Touch.c)
void Touch_ProcessAftTouchEvent(void);


// Touch_ProcessBetaGrip @ 0x6ba76ee8 (Tracking\Touch.c)
void Touch_ProcessBetaGrip(void);

// Touch_ProcessExtInfo @ 0x6ba7713d (Tracking\Touch.c)
void Touch_ProcessExtInfo(void);

// Touch_SetEdgeDist @ 0x6ba72b19 (Tracking\Touch.c)
void Touch_SetEdgeDist(byte param_1);



















// Touch_UnitCopy @ 0x6ba74462 (Tracking\Touch.c)
void Touch_UnitCopy(byte param_1,byte param_2);

// Touch_UnitRemove @ 0x6ba763f4 (Tracking\Touch.c)
void Touch_UnitRemove(byte param_1);

// Touch_UnitUpdate @ 0x6ba760da (Tracking\Touch.c)
void Touch_UnitUpdate(byte param_1,byte param_2);






// Touch_ValidComparison @ 0x6ba7435e (Tracking\Touch.c)
undefined8 Touch_ValidComparison(byte param_1,byte param_2);




// Self_Reset @ 0x6ba6a920 (Signal\Self.c)
void Self_Reset(void);

// --- ASA 侧被手指管线读写的那一层（Rejection\ASAHostBridge.c） ---
// TSA_RptASAInRange @ 0x6ba9c7e9 (Core\TSA.c)
undefined8 TSA_RptASAInRange(void);
// TSA_SetASABluetoothConnectStatus @ 0x6ba9cd02 (Core\TSA.c)
void TSA_SetASABluetoothConnectStatus(undefined4 param_1);
// TSA_SetASABluetoothPenStatus @ 0x6ba9cc2d (Core\TSA.c)
void TSA_SetASABluetoothPenStatus(undefined1 param_1);
// TSA_SetASABluetoothPressure @ 0x6ba9cc65 (Core\TSA.c)
void TSA_SetASABluetoothPressure(undefined2 param_1,undefined2 param_2,undefined2 param_3,undefined2 param_4);
// 宿主桥：每帧在 TSA_ProcessingAFEFrame 之前送入 StylusSolver 解出的笔报告
void TSAHost_SetStylusReport(uint dwStatusFlags,ushort wXPos,ushort wYPos,short nXTilt,short nYTilt,ushort wPressure);

// 已改写成 C++（TsaCore/src），raw 仍在调用的入口
// App/Afe.cpp
void AFE_Init(void);
void AFE_SetCaps(void *caps);
void AFE_SetFrame(void *frame);
void AFE_SetInfo(void *info);
void AFE_SetMaxScanRate(ushort rate);
AFE_Frame * AFE_GetFrame(void);
void AFE_PreProcess(void);
void AFE_Process(void);
undefined8 AFE_GetNextScanFreqIndex(byte *index);
undefined8 AFE_GetNextScanRateIndex(byte *index,char *doubleReport);
void AFE_AutoCheckProcessInternal(void *data,ushort size,uint ctrl,void *timeMs);
undefined1 AFE_IsInAutoCheckProcess(void);
void AFE_ClearAutoCheckFlag(void);
undefined1 AFE_AutoCheckToEnterIDLE(void);
uint AFE_IsAutoCalibration(void);
bool AFE_IsIndividualBaselineForEachFreq(void);
// App/Property.cpp
void TSA_GetPrpt(short *pRawGrid,short *pBaselineGrid);
short TSA_GetMinSigPrpt(void);
int TSA_GetMaxCMFNoiseValue(void);

#ifdef __cplusplus
}
#endif
