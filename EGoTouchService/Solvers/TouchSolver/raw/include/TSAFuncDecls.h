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


// DataSwitch_ToGrid @ 0x6ba906d0 (Core\DataSwitch.c)
void DataSwitch_ToGrid(void);

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



// GripFilterCheckMultiGripUnit @ 0x6bae73f3 (Rejection\GripFilter.c)
void GripFilterCheckMultiGripUnit(byte param_1);

// GripFilter_AboveGripDetect @ 0x6bae698c (Rejection\GripFilter.c)
void GripFilter_AboveGripDetect(void);

// GripFilter_BackToGrip @ 0x6bae24a1 (Rejection\GripFilter.c)
void GripFilter_BackToGrip(void);

// GripFilter_BasicIsAbsGrip @ 0x6bae3fc1 (Rejection\GripFilter.c)
bool GripFilter_BasicIsAbsGrip(void);

// GripFilter_BasicIsAbsGripAbnormal @ 0x6bae3d07 (Rejection\GripFilter.c)
undefined8 GripFilter_BasicIsAbsGripAbnormal(void);

// GripFilter_BasicIsAbsGripPrmt @ 0x6bae3c2f (Rejection\GripFilter.c)
undefined8 GripFilter_BasicIsAbsGripPrmt(void);

// GripFilter_BasicIsAbsNotGrip @ 0x6bae4262 (Rejection\GripFilter.c)
bool GripFilter_BasicIsAbsNotGrip(void);

// GripFilter_BasicIsFlick @ 0x6bae81d7 (Rejection\GripFilter.c)
undefined8 GripFilter_BasicIsFlick(void);

// GripFilter_BasicIsTempGrip @ 0x6bae39aa (Rejection\GripFilter.c)
undefined8 GripFilter_BasicIsTempGrip(void);

// GripFilter_BasicIsTempGripAbnormal @ 0x6bae3618 (Rejection\GripFilter.c)
undefined8 GripFilter_BasicIsTempGripAbnormal(void);

// GripFilter_BasicProcess @ 0x6bae4ab3 (Rejection\GripFilter.c)
void GripFilter_BasicProcess(byte param_1,byte param_2);

// GripFilter_CheckCornerMultiGrip @ 0x6bae7964 (Rejection\GripFilter.c)
void GripFilter_CheckCornerMultiGrip(void);

// GripFilter_CheckMultiGrip @ 0x6bae7d07 (Rejection\GripFilter.c)
void GripFilter_CheckMultiGrip(void);

// GripFilter_CheckMultiGripOnCore @ 0x6bae7701 (Rejection\GripFilter.c)
void GripFilter_CheckMultiGripOnCore(void);

// GripFilter_CheckMultiGripOnSide @ 0x6bae773c (Rejection\GripFilter.c)
void GripFilter_CheckMultiGripOnSide(void);

// GripFilter_CheckScreenRotatedMultiGrip @ 0x6bae7b19 (Rejection\GripFilter.c)
void GripFilter_CheckScreenRotatedMultiGrip(void);

// GripFilter_ClearDeadZoneFlag @ 0x6bae5565 (Rejection\GripFilter.c)
void GripFilter_ClearDeadZoneFlag(void);

// GripFilter_CloseGripDetect @ 0x6bae6e5b (Rejection\GripFilter.c)
void GripFilter_CloseGripDetect(void);

// GripFilter_CloseGripOnCore @ 0x6bae6b13 (Rejection\GripFilter.c)
void GripFilter_CloseGripOnCore(void);

// GripFilter_CloseGripOnSide @ 0x6bae6cf9 (Rejection\GripFilter.c)
void GripFilter_CloseGripOnSide(void);

// GripFilter_DeadZoneInit @ 0x6bae596d (Rejection\GripFilter.c)
void GripFilter_DeadZoneInit(void);

// GripFilter_DeadzoneReset @ 0x6bae4ce0 (Rejection\GripFilter.c)
void GripFilter_DeadzoneReset(void);

// GripFilter_FastMoveDetect @ 0x6bae65ff (Rejection\GripFilter.c)
void GripFilter_FastMoveDetect(void);

// GripFilter_GetAbsThold @ 0x6bae65e3 (Rejection\GripFilter.c)
int GripFilter_GetAbsThold(ushort param_1);

// GripFilter_GetCenterLevel @ 0x6baea5fb (Rejection\GripFilter.c)
byte GripFilter_GetCenterLevel(byte param_1);

// GripFilter_GetClickGripRatio @ 0x6baec860 (Rejection\GripFilter.c)
undefined1 GripFilter_GetClickGripRatio(void);

// GripFilter_GetCoreDim1Distance @ 0x6bae5fc9 (Rejection\GripFilter.c)
undefined4 GripFilter_GetCoreDim1Distance(byte param_1,byte param_2);

// GripFilter_GetCoreDim2Distance @ 0x6bae5e21 (Rejection\GripFilter.c)
undefined4 GripFilter_GetCoreDim2Distance(byte param_1,byte param_2);

// GripFilter_GetCoreDistance @ 0x6bae6171 (Rejection\GripFilter.c)
int GripFilter_GetCoreDistance(undefined1 param_1,undefined1 param_2);

// GripFilter_GetCtdDistToEdge @ 0x6bae61b9 (Rejection\GripFilter.c)
undefined2 GripFilter_GetCtdDistToEdge(byte param_1);

// GripFilter_GetDelayTime @ 0x6bae4796 (Rejection\GripFilter.c)
undefined2 GripFilter_GetDelayTime(void);

// GripFilter_GetDim2Distance @ 0x6bae5dd6 (Rejection\GripFilter.c)
int GripFilter_GetDim2Distance(byte param_1,byte param_2,byte param_3,byte param_4);

// GripFilter_GetEdge @ 0x6bae9854 (Rejection\GripFilter.c)
undefined8 GripFilter_GetEdge(void);

// GripFilter_GetEdgeClickRatioThold @ 0x6baecad0 (Rejection\GripFilter.c)
ushort GripFilter_GetEdgeClickRatioThold(char param_1);

// GripFilter_GetEdgeWidthEnd @ 0x6bae4d95 (Rejection\GripFilter.c)
char GripFilter_GetEdgeWidthEnd(ushort param_1,ushort param_2,byte param_3);

// GripFilter_GetEdgeWidthStart @ 0x6bae4d44 (Rejection\GripFilter.c)
char GripFilter_GetEdgeWidthStart(ushort param_1,ushort param_2);

// GripFilter_GetGripLevel @ 0x6baea5a5 (Rejection\GripFilter.c)
byte GripFilter_GetGripLevel(byte param_1);

// GripFilter_GetPrmt @ 0x6bae43a3 (Rejection\GripFilter.c)
void GripFilter_GetPrmt(void);

// GripFilter_GetProperty @ 0x6bae9747 (Rejection\GripFilter.c)
void GripFilter_GetProperty(void);

// GripFilter_GetPropertyGlobal @ 0x6bae990d (Rejection\GripFilter.c)
void GripFilter_GetPropertyGlobal(void);

// GripFilter_GetQuality @ 0x6bae42bc (Rejection\GripFilter.c)
void GripFilter_GetQuality(void);

// GripFilter_GetRatioTholdLevelExtra @ 0x6baec8e8 (Rejection\GripFilter.c)
void GripFilter_GetRatioTholdLevelExtra(byte *param_1);

// GripFilter_GetSafeDistToEdgeThold @ 0x6bae5d57 (Rejection\GripFilter.c)
undefined2 GripFilter_GetSafeDistToEdgeThold(void);

// GripFilter_GetSafeTouchThold @ 0x6bae5d10 (Rejection\GripFilter.c)
int GripFilter_GetSafeTouchThold(void);

// GripFilter_GetScreenRotatedCtdDistToEdge @ 0x6bae6220 (Rejection\GripFilter.c)
undefined2 GripFilter_GetScreenRotatedCtdDistToEdge(byte param_1);

// GripFilter_GetShapeFlag @ 0x6bae9201 (Rejection\GripFilter.c)
void GripFilter_GetShapeFlag(void);

// GripFilter_GetSideDim2Distance @ 0x6bae6287 (Rejection\GripFilter.c)
undefined4 GripFilter_GetSideDim2Distance(byte param_1,byte param_2);

// GripFilter_GetSideTouchOnTime @ 0x6bae6425 (Rejection\GripFilter.c)
int GripFilter_GetSideTouchOnTime(byte param_1);

// GripFilter_GetStage @ 0x6bae492d (Rejection\GripFilter.c)
void GripFilter_GetStage(void);

// GripFilter_GetTouchedLevel @ 0x6baea6a0 (Rejection\GripFilter.c)
byte GripFilter_GetTouchedLevel(byte param_1);

// GripFilter_GetTrackingThold @ 0x6bae5d38 (Rejection\GripFilter.c)
undefined4 GripFilter_GetTrackingThold(void);

// GripFilter_GrapDetect @ 0x6bae71df (Rejection\GripFilter.c)
void GripFilter_GrapDetect(void);

// GripFilter_HeavyGripRegionDetect @ 0x6bae8d98 (Rejection\GripFilter.c)
void GripFilter_HeavyGripRegionDetect(void);

// GripFilter_IDLEProcess @ 0x6bae1c75 (Rejection\GripFilter.c)
void GripFilter_IDLEProcess(void);

// GripFilter_Init @ 0x6bae174a (Rejection\GripFilter.c)
void GripFilter_Init(void);

// GripFilter_Is2HandGame @ 0x6bae1e40 (Rejection\GripFilter.c)
undefined8 GripFilter_Is2HandGame(void);

// GripFilter_IsAbnormalLiftOff @ 0x6bae340f (Rejection\GripFilter.c)
undefined8 GripFilter_IsAbnormalLiftOff(void);

// GripFilter_IsAbsGrip @ 0x6bae1bd9 (Rejection\GripFilter.c)
undefined8 GripFilter_IsAbsGrip(byte param_1);

// GripFilter_IsBackToGrip @ 0x6bae237b (Rejection\GripFilter.c)
undefined8 GripFilter_IsBackToGrip(void);

// GripFilter_IsBackToGripAllowed @ 0x6bae1f52 (Rejection\GripFilter.c)
undefined8 GripFilter_IsBackToGripAllowed(void);

// GripFilter_IsBackToGripSatisfied @ 0x6bae1fd5 (Rejection\GripFilter.c)
undefined8 GripFilter_IsBackToGripSatisfied(void);

// GripFilter_IsCoreTouchChangeInTime @ 0x6bae2c51 (Rejection\GripFilter.c)
undefined8 GripFilter_IsCoreTouchChangeInTime(ushort param_1);

// GripFilter_IsCornerGripClick @ 0x6bae7060 (Rejection\GripFilter.c)
undefined8 GripFilter_IsCornerGripClick(void);

// GripFilter_IsCornerLongPressAllowed @ 0x6bae25b3 (Rejection\GripFilter.c)
undefined8 GripFilter_IsCornerLongPressAllowed(void);

// GripFilter_IsDelayTimeOut @ 0x6bae47ec (Rejection\GripFilter.c)
undefined8 GripFilter_IsDelayTimeOut(void);

// GripFilter_IsEnableWeakenCornerClickSupress @ 0x6baec9fe (Rejection\GripFilter.c)
undefined8 GripFilter_IsEnableWeakenCornerClickSupress(void);

// GripFilter_IsFlickAllowed @ 0x6bae7dbd (Rejection\GripFilter.c)
undefined8 GripFilter_IsFlickAllowed(void);

// GripFilter_IsFlickDetected @ 0x6bae80e0 (Rejection\GripFilter.c)
bool GripFilter_IsFlickDetected(void);

// GripFilter_IsGlobalSuppressedClick @ 0x6bae82c8 (Rejection\GripFilter.c)
undefined8 GripFilter_IsGlobalSuppressedClick(void);

// GripFilter_IsGrip @ 0x6bae193f (Rejection\GripFilter.c)
undefined8 GripFilter_IsGrip(int param_1);

// GripFilter_IsGripCriteriaDeadzone @ 0x6bae4df8 (Rejection\GripFilter.c)
undefined8 GripFilter_IsGripCriteriaDeadzone(byte param_1);

// GripFilter_IsGripMergeAllowed @ 0x6bae1eda (Rejection\GripFilter.c)
bool GripFilter_IsGripMergeAllowed(void);

// GripFilter_IsGripOrTbd @ 0x6bae1966 (Rejection\GripFilter.c)
undefined8 GripFilter_IsGripOrTbd(int param_1);

// GripFilter_IsGripOrTbdTouch @ 0x6bae1995 (Rejection\GripFilter.c)
char GripFilter_IsGripOrTbdTouch(byte param_1);

// GripFilter_IsGripTouch @ 0x6bae19d1 (Rejection\GripFilter.c)
bool GripFilter_IsGripTouch(byte param_1);

// GripFilter_IsHeavyCornerTouched @ 0x6baea7ca (Rejection\GripFilter.c)
undefined8 GripFilter_IsHeavyCornerTouched(undefined1 param_1);

// GripFilter_IsHeavyRegionTouched @ 0x6baea6e5 (Rejection\GripFilter.c)
bool GripFilter_IsHeavyRegionTouched(undefined1 param_1);

// GripFilter_IsHigherPriorityTouch @ 0x6baeaf03 (Rejection\GripFilter.c)
char GripFilter_IsHigherPriorityTouch(undefined1 param_1,undefined1 param_2);

// GripFilter_IsHigherPriorityTouchNormal @ 0x6baead61 (Rejection\GripFilter.c)
undefined8 GripFilter_IsHigherPriorityTouchNormal(byte param_1,byte param_2);

// GripFilter_IsHigherPriorityTouchPad @ 0x6baeabbe (Rejection\GripFilter.c)
undefined8 GripFilter_IsHigherPriorityTouchPad(byte param_1,byte param_2);

// GripFilter_IsInDynamicDeadZone @ 0x6bae59ae (Rejection\GripFilter.c)
undefined8 GripFilter_IsInDynamicDeadZone(ushort param_1);

// GripFilter_IsInExcludedRegion @ 0x6bae9f40 (Rejection\GripFilter.c)
undefined8 GripFilter_IsInExcludedRegion(byte param_1);

// GripFilter_IsInMultiGripRegion @ 0x6bae977e (Rejection\GripFilter.c)
undefined8 GripFilter_IsInMultiGripRegion(byte param_1);

// GripFilter_IsInPalmRegion @ 0x6bae696e (Rejection\GripFilter.c)
char GripFilter_IsInPalmRegion(undefined1 param_1);

// GripFilter_IsLongPress @ 0x6bae675f (Rejection\GripFilter.c)
undefined4 GripFilter_IsLongPress(void);

// GripFilter_IsLongPressAllowed @ 0x6bae28e0 (Rejection\GripFilter.c)
undefined8 GripFilter_IsLongPressAllowed(void);

// GripFilter_IsMergeHandlerEdgeEnabled @ 0x6bae1763 (Rejection\GripFilter.c)
bool GripFilter_IsMergeHandlerEdgeEnabled(uint param_1);

// GripFilter_IsMergeHandlerEnabled @ 0x6bae18c4 (Rejection\GripFilter.c)
undefined8 GripFilter_IsMergeHandlerEnabled(void);

// GripFilter_IsMergeHandlerSensorEnabled @ 0x6bae1820 (Rejection\GripFilter.c)
undefined1 GripFilter_IsMergeHandlerSensorEnabled(byte param_1,byte param_2);

// GripFilter_IsMerged @ 0x6bae83a6 (Rejection\GripFilter.c)
undefined8 GripFilter_IsMerged(byte param_1,byte param_2);

// GripFilter_IsNearbyCorner @ 0x6baea708 (Rejection\GripFilter.c)
undefined8 GripFilter_IsNearbyCorner(byte param_1);

// GripFilter_IsNegSigTouch @ 0x6bae38c7 (Rejection\GripFilter.c)
undefined8 GripFilter_IsNegSigTouch(void);

// GripFilter_IsNegSigTouchBypass @ 0x6bae3f02 (Rejection\GripFilter.c)
undefined8 GripFilter_IsNegSigTouchBypass(void);

// GripFilter_IsOKToEnterIDLE @ 0x6bae1bcc (Rejection\GripFilter.c)
undefined1 GripFilter_IsOKToEnterIDLE(void);

// GripFilter_IsOnDim1 @ 0x6baec770 (Rejection\GripFilter.c)
undefined8 GripFilter_IsOnDim1(void);

// GripFilter_IsOnDim2 @ 0x6baec7e8 (Rejection\GripFilter.c)
undefined8 GripFilter_IsOnDim2(void);

// GripFilter_IsOnOppositeSide @ 0x6bae717f (Rejection\GripFilter.c)
undefined8 GripFilter_IsOnOppositeSide(int param_1,int param_2);

// GripFilter_IsOnSameShortSide @ 0x6baeaac1 (Rejection\GripFilter.c)
undefined8 GripFilter_IsOnSameShortSide(byte param_1,byte param_2);

// GripFilter_IsOnSameSide @ 0x6baea8bc (Rejection\GripFilter.c)
undefined8 GripFilter_IsOnSameSide(byte param_1,byte param_2,int param_3);

// GripFilter_IsPad @ 0x6bae1e1c (Rejection\GripFilter.c)
bool GripFilter_IsPad(void);

// GripFilter_IsPeakAbsGrip @ 0x6baea0be (Rejection\GripFilter.c)
bool GripFilter_IsPeakAbsGrip(byte param_1);

// GripFilter_IsPeakAbsNotGrip @ 0x6baea103 (Rejection\GripFilter.c)
bool GripFilter_IsPeakAbsNotGrip(byte param_1);

// GripFilter_IsRatioOK @ 0x6baecdc6 (Rejection\GripFilter.c)
undefined8 GripFilter_IsRatioOK(undefined4 param_1);

// GripFilter_IsSafeTouch @ 0x6bae42a9 (Rejection\GripFilter.c)
char GripFilter_IsSafeTouch(void);

// GripFilter_IsScreenRotatedMultiGripEnabled @ 0x6bae7ae6 (Rejection\GripFilter.c)
undefined8 GripFilter_IsScreenRotatedMultiGripEnabled(void);

// GripFilter_IsScreenRotatedStateLongPressAllowed @ 0x6bae280c (Rejection\GripFilter.c)
undefined8 GripFilter_IsScreenRotatedStateLongPressAllowed(void);

// GripFilter_IsSensorInCornerRegion @ 0x6baea1f4 (Rejection\GripFilter.c)
undefined8 GripFilter_IsSensorInCornerRegion(byte param_1,byte param_2);

// GripFilter_IsSeparated @ 0x6bae864d (Rejection\GripFilter.c)
undefined8 GripFilter_IsSeparated(byte param_1,byte param_2);

// GripFilter_IsSideTouch @ 0x6bae5d8e (Rejection\GripFilter.c)
undefined8 GripFilter_IsSideTouch(void);

// GripFilter_IsSignalOK @ 0x6baed803 (Rejection\GripFilter.c)
undefined8 GripFilter_IsSignalOK(byte param_1,uint param_2,char param_3);

// GripFilter_IsStuckSideTouch @ 0x6bae223a (Rejection\GripFilter.c)
undefined8 GripFilter_IsStuckSideTouch(void);

// GripFilter_IsSuppressedByCoreTouch @ 0x6bae2fb7 (Rejection\GripFilter.c)
undefined8 GripFilter_IsSuppressedByCoreTouch(void);

// GripFilter_IsSuppressedByHandGesture @ 0x6bae2dd7 (Rejection\GripFilter.c)
undefined8 GripFilter_IsSuppressedByHandGesture(void);

// GripFilter_IsSuppressedGlobally @ 0x6bae3211 (Rejection\GripFilter.c)
undefined8 GripFilter_IsSuppressedGlobally(void);

// GripFilter_IsSuppressedGlobally2 @ 0x6bae2a9d (Rejection\GripFilter.c)
undefined8 GripFilter_IsSuppressedGlobally2(void);

// GripFilter_IsToApplyDelay @ 0x6bae4695 (Rejection\GripFilter.c)
undefined8 GripFilter_IsToApplyDelay(void);

// GripFilter_IsToApplyMaxDelay @ 0x6bae4536 (Rejection\GripFilter.c)
undefined8 GripFilter_IsToApplyMaxDelay(void);

// GripFilter_IsToApplyMaxDelayOnHeavyCorner @ 0x6bae440b (Rejection\GripFilter.c)
undefined8 GripFilter_IsToApplyMaxDelayOnHeavyCorner(void);

// GripFilter_IsToBypassDelay @ 0x6bae4740 (Rejection\GripFilter.c)
undefined8 GripFilter_IsToBypassDelay(void);

// GripFilter_IsToCancelIDMapping @ 0x6bae8858 (Rejection\GripFilter.c)
bool GripFilter_IsToCancelIDMapping(byte param_1,byte param_2);

// GripFilter_IsToCheckAbnormal @ 0x6bae3513 (Rejection\GripFilter.c)
undefined8 GripFilter_IsToCheckAbnormal(void);

// GripFilter_IsToCheckStuckSideTouch @ 0x6bae2113 (Rejection\GripFilter.c)
undefined8 GripFilter_IsToCheckStuckSideTouch(void);

// GripFilter_IsTouchInCornerRegion @ 0x6baea327 (Rejection\GripFilter.c)
char GripFilter_IsTouchInCornerRegion(byte param_1);

// GripFilter_IsTouchInHeavyRegion @ 0x6baea454 (Rejection\GripFilter.c)
bool GripFilter_IsTouchInHeavyRegion(byte param_1);

// GripFilter_IsTouchIntervalInTime @ 0x6bae6536 (Rejection\GripFilter.c)
undefined8 GripFilter_IsTouchIntervalInTime(undefined1 param_1,undefined1 param_2,char param_3,int param_4);

// GripFilter_IsTouchPosInCornerRegion @ 0x6baea2b0 (Rejection\GripFilter.c)
undefined8 GripFilter_IsTouchPosInCornerRegion(byte param_1);

// GripFilter_IsVideoMode @ 0x6bae1e9c (Rejection\GripFilter.c)
undefined8 GripFilter_IsVideoMode(void);

// GripFilter_IsVideoModeLongPressAllowed @ 0x6bae2720 (Rejection\GripFilter.c)
undefined8 GripFilter_IsVideoModeLongPressAllowed(void);

// GripFilter_MultiGripDetect @ 0x6bae7d43 (Rejection\GripFilter.c)
void GripFilter_MultiGripDetect(void);

// GripFilter_NewTouchDetect @ 0x6bae8ab3 (Rejection\GripFilter.c)
void GripFilter_NewTouchDetect(void);

// GripFilter_Process @ 0x6bae1d3c (Rejection\GripFilter.c)
void GripFilter_Process(void);

// GripFilter_RegionInit @ 0x6bae9f0d (Rejection\GripFilter.c)
void GripFilter_RegionInit(void);

// GripFilter_RegionProcess @ 0x6baec71c (Rejection\GripFilter.c)
void GripFilter_RegionProcess(void);

// GripFilter_RegionReset @ 0x6bae9e90 (Rejection\GripFilter.c)
void GripFilter_RegionReset(void);

// GripFilter_RegionSet @ 0x6baebad8 (Rejection\GripFilter.c)
void GripFilter_RegionSet(uint param_1,undefined4 param_2,byte param_3);

// GripFilter_RegionSetBottomLeftCorner @ 0x6baeb21d (Rejection\GripFilter.c)
void GripFilter_RegionSetBottomLeftCorner(byte param_1);

// GripFilter_RegionSetBottomRightCorner @ 0x6baeb391 (Rejection\GripFilter.c)
void GripFilter_RegionSetBottomRightCorner(byte param_1);

// GripFilter_RegionSetByHandGesture @ 0x6baec611 (Rejection\GripFilter.c)
void GripFilter_RegionSetByHandGesture(void);

// GripFilter_RegionSetByScreenRotation @ 0x6baebde9 (Rejection\GripFilter.c)
void GripFilter_RegionSetByScreenRotation(void);

// GripFilter_RegionSetByScreenRotationFold @ 0x6baec3e4 (Rejection\GripFilter.c)
void GripFilter_RegionSetByScreenRotationFold(void);

// GripFilter_RegionSetByScreenRotationPad @ 0x6baec121 (Rejection\GripFilter.c)
void GripFilter_RegionSetByScreenRotationPad(void);

// GripFilter_RegionSetPalm @ 0x6baeb512 (Rejection\GripFilter.c)
void GripFilter_RegionSetPalm(uint param_1,byte param_2,byte param_3);

// GripFilter_RegionSetTopLeftCorner @ 0x6baeaf42 (Rejection\GripFilter.c)
void GripFilter_RegionSetTopLeftCorner(byte param_1);

// GripFilter_RegionSetTopRightCorner @ 0x6baeb0a9 (Rejection\GripFilter.c)
void GripFilter_RegionSetTopRightCorner(byte param_1);

// GripFilter_Report @ 0x6bae1a5b (Rejection\GripFilter.c)
void GripFilter_Report(void);

// GripFilter_SafeTouchDetect @ 0x6bae891d (Rejection\GripFilter.c)
void GripFilter_SafeTouchDetect(void);

// GripFilter_SetDeadZoneFlag @ 0x6bae5021 (Rejection\GripFilter.c)
void GripFilter_SetDeadZoneFlag(void);

// GripFilter_StageOnQuality @ 0x6bae48e1 (Rejection\GripFilter.c)
undefined8 GripFilter_StageOnQuality(void);

// GripFilter_UpdateDeadzone @ 0x6bae5981 (Rejection\GripFilter.c)
void GripFilter_UpdateDeadzone(void);

// GripFilter_WeakenCornerClickSupress @ 0x6baeca55 (Rejection\GripFilter.c)
void GripFilter_WeakenCornerClickSupress(char *param_1);

// HandGesture_2HandDetection @ 0x6baef641 (Rejection\HandGesture.c)
void HandGesture_2HandDetection(void);

// HandGesture_AngleFormat1 @ 0x6baeeabd (Rejection\HandGesture.c)
short HandGesture_AngleFormat1(short param_1);

// HandGesture_AngleFormat2 @ 0x6baeeaf5 (Rejection\HandGesture.c)
short HandGesture_AngleFormat2(short param_1);

// HandGesture_BaselineReset @ 0x6baeddbd (Rejection\HandGesture.c)
void HandGesture_BaselineReset(void);

// HandGesture_CheckRelatedGrip @ 0x6baee667 (Rejection\HandGesture.c)
void HandGesture_CheckRelatedGrip(byte param_1,byte param_2);

// HandGesture_CheckValidity @ 0x6baeec0f (Rejection\HandGesture.c)
void HandGesture_CheckValidity(void);

// HandGesture_CopyDiff @ 0x6baef0bb (Rejection\HandGesture.c)
void HandGesture_CopyDiff(void);

// HandGesture_DifProcess @ 0x6baeef00 (Rejection\HandGesture.c)
void HandGesture_DifProcess(void);

// HandGesture_DifUnit @ 0x6baeede7 (Rejection\HandGesture.c)
void HandGesture_DifUnit(byte param_1,byte param_2);

// HandGesture_Free @ 0x6baedd66 (Rejection\HandGesture.c)
void HandGesture_Free(void);

// HandGesture_GetPkThold @ 0x6baef045 (Rejection\HandGesture.c)
int HandGesture_GetPkThold(void);

// HandGesture_GetWidth @ 0x6baeedbc (Rejection\HandGesture.c)
undefined8 HandGesture_GetWidth(void);

// HandGesture_GripDetection @ 0x6baef865 (Rejection\HandGesture.c)
void HandGesture_GripDetection(void);

// HandGesture_Init @ 0x6baedf1b (Rejection\HandGesture.c)
void HandGesture_Init();

// HandGesture_Is2HandDetected @ 0x6baee0ce (Rejection\HandGesture.c)
bool HandGesture_Is2HandDetected(void);

// HandGesture_IsAngleCorelated @ 0x6baeeb33 (Rejection\HandGesture.c)
bool HandGesture_IsAngleCorelated(short param_1,short param_2);

// HandGesture_IsAngleValid @ 0x6baeeb8b (Rejection\HandGesture.c)
char HandGesture_IsAngleValid(void);

// HandGesture_IsDistInValid @ 0x6baee0e5 (Rejection\HandGesture.c)
undefined8 HandGesture_IsDistInValid (undefined2 param_1,undefined2 param_2,undefined2 param_3,undefined2 param_4);

// HandGesture_IsGripRelated @ 0x6baee25c (Rejection\HandGesture.c)
undefined8 HandGesture_IsGripRelated(byte param_1,byte param_2);

// HandGesture_IsOKToCalcDiff @ 0x6baee96e (Rejection\HandGesture.c)
undefined8 HandGesture_IsOKToCalcDiff(void);

// HandGesture_IsOKToProcessTouch @ 0x6baef06a (Rejection\HandGesture.c)
undefined8 HandGesture_IsOKToProcessTouch(void);

// HandGesture_IsOKToResetBaseline @ 0x6baee815 (Rejection\HandGesture.c)
undefined8 HandGesture_IsOKToResetBaseline(void);

// HandGesture_IsRelatedGripDetected @ 0x6baee4eb (Rejection\HandGesture.c)
undefined8 HandGesture_IsRelatedGripDetected(byte param_1,byte param_2);

// HandGesture_Malloc @ 0x6baedc30 (Rejection\HandGesture.c)
undefined8 HandGesture_Malloc(void);

// HandGesture_PostProcess @ 0x6baeed20 (Rejection\HandGesture.c)
void HandGesture_PostProcess(void);

// HandGesture_Process @ 0x6baef9fc (Rejection\HandGesture.c)
void HandGesture_Process(void);

// HandGesture_ResetRecord @ 0x6baede69 (Rejection\HandGesture.c)
void HandGesture_ResetRecord(void);

// HandGesture_SetHandArea @ 0x6baee147 (Rejection\HandGesture.c)
void HandGesture_SetHandArea(byte param_1);

// HandGesture_TZFlagProcess @ 0x6baeecc0 (Rejection\HandGesture.c)
void HandGesture_TZFlagProcess(void);

// HandGesture_TimeRecord @ 0x6baee9ba (Rejection\HandGesture.c)
void HandGesture_TimeRecord(void);

// HandGesture_TimeStep @ 0x6baeea17 (Rejection\HandGesture.c)
void HandGesture_TimeStep(void);

// HandGesture_TouchProcess @ 0x6baef30b (Rejection\HandGesture.c)
void HandGesture_TouchProcess(void);



















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

// PrevTouch_AssignCurIdxBasedOnID @ 0x6ba6dbc9 (Tracking\PrevTouch.c)
void PrevTouch_AssignCurIdxBasedOnID(void);

// PrevTouch_GetIdxBasedOnCurIdx @ 0x6ba6dcc4 (Tracking\PrevTouch.c)
undefined1 PrevTouch_GetIdxBasedOnCurIdx(byte param_1);

// PrevTouch_GetIdxBasedOnID @ 0x6ba6db56 (Tracking\PrevTouch.c)
byte PrevTouch_GetIdxBasedOnID(char param_1);

// PrevTouch_HistoryBufferClean @ 0x6ba6e9b0 (Tracking\PrevTouch.c)
void PrevTouch_HistoryBufferClean(byte param_1);

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























// TSAMath_ATan2 @ 0x6bb429d4 (Common\TSAMath.c)
int TSAMath_ATan2(ushort param_1,ushort param_2);



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

// TSAStatic_IsScreenRotated @ 0x6baa2e39 (Core\TSAStatic.c)
undefined8 TSAStatic_IsScreenRotated(void);

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

// TSA_Free @ 0x6baa34c4 (Core\TSA.c)
void TSA_Free(longlong *param_1);

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




// TSA_IsJustEnterTypingScenario @ 0x6ba8fe53 (Core\TSA.c)
undefined8 TSA_IsJustEnterTypingScenario(void);


// TSA_IsOKToEnterIDLEMode @ 0x6ba9e755 (Core\TSA.c)
undefined1 TSA_IsOKToEnterIDLEMode(void);







// TSA_IsReCalibNeeded @ 0x6ba9e32b (Core\TSA.c)
undefined1 TSA_IsReCalibNeeded(void);




// TSA_IsTypingScenario @ 0x6ba8fe17 (Core\TSA.c)
bool TSA_IsTypingScenario(void);



// TSA_IsVideoScenario @ 0x6ba8fdf7 (Core\TSA.c)
bool TSA_IsVideoScenario(void);

// TSA_IsWindowsPadFeatureEnabled @ 0x6ba9ef4b (Core\TSA.c)
undefined4 TSA_IsWindowsPadFeatureEnabled(void);







// TSA_MSReset @ 0x6ba90b16 (Core\TSA.c)
void TSA_MSReset(void);



// TSA_MallocAndInit @ 0x6baa3430 (Core\TSA.c)
void * TSA_MallocAndInit(uint param_1);



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




// Touch_AssignPreIdxBasedOnID @ 0x6ba73fc7 (Tracking\Touch.c)
void Touch_AssignPreIdxBasedOnID(void);


// Touch_Clean @ 0x6ba6f49c (Tracking\Touch.c)
void Touch_Clean(void);

// Touch_CreatePostReport @ 0x6ba76a03 (Tracking\Touch.c)
void Touch_CreatePostReport(byte param_1, byte param_2);



// Touch_GetDistToLogicEdge @ 0x6ba73009 (Tracking\Touch.c)
short Touch_GetDistToLogicEdge(int param_1,short param_2,short param_3);


// Touch_GetFormattedTimeInterval @ 0x6ba76329 (Tracking\Touch.c)
short Touch_GetFormattedTimeInterval(short param_1);









// Touch_GetSQDistMM @ 0x6ba734be (Tracking\Touch.c)
undefined2 Touch_GetSQDistMM(undefined2 param_1,undefined2 param_2,undefined2 param_3,undefined2 param_4);



// Touch_GetSQDistMMInBetween @ 0x6ba73515 (Tracking\Touch.c)
ushort Touch_GetSQDistMMInBetween(byte param_1,byte param_2);

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
