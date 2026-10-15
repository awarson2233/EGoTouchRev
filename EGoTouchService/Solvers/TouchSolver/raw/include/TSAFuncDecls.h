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

// ASACalibration_Init @ 0x6baab7b0 (Rejection\ASACalibration.c)
void ASACalibration_Init(void);

// ASAStaticInit @ 0x6bac8481 (Rejection\ASA.c)
void ASAStaticInit(longlong param_1);

// ASA_GetBluetoothConnectStatus @ 0x6bac77ec (Rejection\ASA.c)
undefined4 ASA_GetBluetoothConnectStatus(void);

// ASA_GetDim1Res @ 0x6bac760d (Rejection\ASA.c)
ushort ASA_GetDim1Res(void);

// ASA_GetDim2Res @ 0x6bac761d (Rejection\ASA.c)
ushort ASA_GetDim2Res(void);

// ASA_GetRptInRange @ 0x6bac757f (Rejection\ASA.c)
bool ASA_GetRptInRange(void);

// ASA_GetRptPressure @ 0x6bac767e (Rejection\ASA.c)
ushort ASA_GetRptPressure(void);

// ASA_GetRptXPos @ 0x6bac762e (Rejection\ASA.c)
ushort ASA_GetRptXPos(void);

// ASA_GetRptXTilt @ 0x6bac7656 (Rejection\ASA.c)
short ASA_GetRptXTilt(void);

// ASA_GetRptYPos @ 0x6bac7642 (Rejection\ASA.c)
ushort ASA_GetRptYPos(void);

// ASA_GetRptYTilt @ 0x6bac766a (Rejection\ASA.c)
short ASA_GetRptYTilt(void);

// ASA_InitAFEProject @ 0x6bac4eb9 (Rejection\ASA.c)
undefined8 ASA_InitAFEProject(undefined8 param_1,undefined8 param_2);

// ASA_InitProjectFromTsa @ 0x6bac4e99 (Rejection\ASA.c)
undefined8 ASA_InitProjectFromTsa(undefined8 param_1);

// ASA_IsEnterStylusHPP2 @ 0x6bac6fe8 (Rejection\ASA.c)
bool ASA_IsEnterStylusHPP2(void);

// ASA_IsEnterStylusHPP3 @ 0x6bac701a (Rejection\ASA.c)
undefined8 ASA_IsEnterStylusHPP3(void);

// ASA_LoadProjectPrmt @ 0x6bac4c75 (Rejection\ASA.c)
void ASA_LoadProjectPrmt(undefined8 param_1, void *param_2);

// ASA_LoadProjectPrmtFromTsa @ 0x6bac4c13 (Rejection\ASA.c)
void ASA_LoadProjectPrmtFromTsa(longlong param_1);

// ASA_LoadStylusHWPrmt @ 0x6bac4abc (Rejection\ASA.c)
void ASA_LoadStylusHWPrmt(byte param_1);

// ASA_Reset @ 0x6bac4e3b (Rejection\ASA.c)
undefined8 ASA_Reset(void);

// AdaptiveIIR_Process @ 0x6bafad7d (Filter\AdaptiveIIR.c)
int AdaptiveIIR_Process(int param_1,int param_2,byte param_3,byte param_4,int param_5);

// AdjustByHistory @ 0x6baf540d (Tracking\HotZoneAdjust.c)
void AdjustByHistory(byte param_1,byte param_2);

// AntiTouch_CheckProximityFlag @ 0x6bad6aee (Rejection\AntiTouch.c)
void AntiTouch_CheckProximityFlag(undefined1 param_1);

// AntiTouch_CommonProcess @ 0x6bad7ad0 (Rejection\AntiTouch.c)
void AntiTouch_CommonProcess(void);

// AntiTouch_GetAxisMajorThold @ 0x6bad697c (Rejection\AntiTouch.c)
undefined1 AntiTouch_GetAxisMajorThold(byte param_1);

// AntiTouch_GetEllipseRateThold @ 0x6bad6a5a (Rejection\AntiTouch.c)
undefined2 AntiTouch_GetEllipseRateThold(byte param_1);

// AntiTouch_GetPersistFlag @ 0x6bad6b3d (Rejection\AntiTouch.c)
bool AntiTouch_GetPersistFlag(byte param_1,byte param_2);

// AntiTouch_Init @ 0x6bad6010 (Rejection\AntiTouch.c)
void AntiTouch_Init(void);

// AntiTouch_IsGrip @ 0x6bad6640 (Rejection\AntiTouch.c)
bool AntiTouch_IsGrip(byte param_1);

// AntiTouch_IsToGoBackReport @ 0x6bad6021 (Rejection\AntiTouch.c)
undefined8 AntiTouch_IsToGoBackReport(byte param_1);

// AntiTouch_IsToProtectProximity @ 0x6bad66e6 (Rejection\AntiTouch.c)
bool AntiTouch_IsToProtectProximity(byte param_1);

// AntiTouch_IsTouchCloseToUdfpArea @ 0x6bad689b (Rejection\AntiTouch.c)
undefined8 AntiTouch_IsTouchCloseToUdfpArea(byte param_1);

// AntiTouch_PostReport @ 0x6bad65e6 (Rejection\AntiTouch.c)
void AntiTouch_PostReport(void);

// AntiTouch_PostReportUnit @ 0x6bad6099 (Rejection\AntiTouch.c)
void AntiTouch_PostReportUnit(byte param_1,byte param_2);

// AntiTouch_PreProcess @ 0x6bad67d3 (Rejection\AntiTouch.c)
void AntiTouch_PreProcess(void);

// AntiTouch_Process @ 0x6bad7bbb (Rejection\AntiTouch.c)
void AntiTouch_Process(void);

// AntiTouch_ProcessProximityUnit @ 0x6bad6f59 (Rejection\AntiTouch.c)
void AntiTouch_ProcessProximityUnit(byte param_1,byte param_2);

// AntiTouch_ProcessUnit @ 0x6bad7719 (Rejection\AntiTouch.c)
void AntiTouch_ProcessUnit(byte param_1,byte param_2);

// AntiTouch_Report @ 0x6bad7bf7 (Rejection\AntiTouch.c)
void AntiTouch_Report(void);

// BLIIR_CalcDiff @ 0x6ba46efc (Baseline\BLIIR.c)
void BLIIR_CalcDiff(void);

// BLIIR_CalcDiffCommon @ 0x6ba46f4c (Baseline\BLIIR.c)
void BLIIR_CalcDiffCommon(short *pDifOut,short *pRawGrid,short *pBaselineGrid);

// BLIIR_CalcRawCommon @ 0x6ba4701b (Baseline\BLIIR.c)
void BLIIR_CalcRawCommon(short *param_1,short *param_2,short *param_3);

// BLIIR_DoUpdate @ 0x6ba47141 (Baseline\BLIIR.c)
void BLIIR_DoUpdate(short param_1);

// BLIIR_Init @ 0x6ba46e70 (Baseline\BLIIR.c)
void BLIIR_Init(void);

// BLIIR_Reset @ 0x6ba4721f (Baseline\BLIIR.c)
void BLIIR_Reset(void);

// BLIIR_ResetDiff @ 0x6ba46e76 (Baseline\BLIIR.c)
void BLIIR_ResetDiff(void);

// BLIIR_Update @ 0x6ba470e7 (Baseline\BLIIR.c)
void BLIIR_Update(char param_1,short param_2,short param_3);

// BLRecal_Init @ 0x6ba47430 (Baseline\BLRecal.c)
void BLRecal_Init(void);

// BLRecal_Process @ 0x6ba47458 (Baseline\BLRecal.c)
void BLRecal_Process(void);

// BLReset_BaselineStateMachine @ 0x6ba49f88 (Baseline\BLReset.c)
void BLReset_BaselineStateMachine(byte isTouchProtected);

// BLReset_BaselineStateUpdate @ 0x6ba4a368 (Baseline\BLReset.c)
void BLReset_BaselineStateUpdate(char param_1);

// BLReset_Check @ 0x6ba48939 (Baseline\BLReset.c)
undefined1 BLReset_Check(void);

// BLReset_CheckAbnormal @ 0x6ba481e9 (Baseline\BLReset.c)
bool BLReset_CheckAbnormal(ushort param_1,ushort param_2);

// BLReset_CheckAbnormalPeak @ 0x6ba482f0 (Baseline\BLReset.c)
undefined1 BLReset_CheckAbnormalPeak(byte param_1);

// BLReset_CheckAbnormalSensor @ 0x6ba4825d (Baseline\BLReset.c)
undefined1 BLReset_CheckAbnormalSensor(byte param_1,byte param_2);

// BLReset_CheckAbnormalTouch @ 0x6ba48366 (Baseline\BLReset.c)
undefined1 BLReset_CheckAbnormalTouch(byte param_1);

// BLReset_CheckCleanBaseline @ 0x6ba4a9a3 (Baseline\BLReset.c)
undefined8 BLReset_CheckCleanBaseline(void);

// BLReset_CheckNumOverNoise @ 0x6ba4850e (Baseline\BLReset.c)
undefined8 BLReset_CheckNumOverNoise(void);

// BLReset_CheckTouchTrustability @ 0x6ba4a613 (Baseline\BLReset.c)
void BLReset_CheckTouchTrustability(void);

// BLReset_CocurrentTouchDetect @ 0x6ba4859c (Baseline\BLReset.c)
undefined8 BLReset_CocurrentTouchDetect(void);

// BLReset_FlagDisableBaselineUpdateInit @ 0x6ba47ae6 (Baseline\BLReset.c)
void BLReset_FlagDisableBaselineUpdateInit(void);

// BLReset_GetBaselineCleanState @ 0x6ba4af53 (Baseline\BLReset.c)
char BLReset_GetBaselineCleanState(void);

// BLReset_GetRawChangedThold @ 0x6ba4901e (Baseline\BLReset.c)
short BLReset_GetRawChangedThold(ushort param_1,ushort param_2);

// BLReset_GetTouchToBeProtected2State @ 0x6ba4afd3 (Baseline\BLReset.c)
undefined1 BLReset_GetTouchToBeProtected2State(void);

// BLReset_InDebounce @ 0x6ba47750 (Baseline\BLReset.c)
bool BLReset_InDebounce(void);

// BLReset_Init @ 0x6ba47783 (Baseline\BLReset.c)
void BLReset_Init(char param_1);

// BLReset_IsAbnormalTouchDetected @ 0x6ba486da (Baseline\BLReset.c)
undefined8 BLReset_IsAbnormalTouchDetected(void);

// BLReset_IsAfeRawJump @ 0x6ba4b70a (Baseline\BLReset.c)
bool BLReset_IsAfeRawJump(void);

// BLReset_IsAfeSideTouchToBeProtected @ 0x6ba49ee3 (Baseline\BLReset.c)
undefined8 BLReset_IsAfeSideTouchToBeProtected(void);

// BLReset_IsAllFreqsBaselineInitializedDuringTouchLiftOff @ 0x6ba48b01 (Baseline\BLReset.c)
undefined1 BLReset_IsAllFreqsBaselineInitializedDuringTouchLiftOff(void);

// BLReset_IsAllTouchByAbnormalBaseline @ 0x6ba48425 (Baseline\BLReset.c)
undefined8 BLReset_IsAllTouchByAbnormalBaseline(void);

// BLReset_IsBaselineAbnormal @ 0x6ba4b506 (Baseline\BLReset.c)
undefined1 BLReset_IsBaselineAbnormal(void);

// BLReset_IsBaselineAbnormalType1 @ 0x6ba4b083 (Baseline\BLReset.c)
void BLReset_IsBaselineAbnormalType1(char param_1,char param_2);

// BLReset_IsBaselineAbnormalType2 @ 0x6ba4b295 (Baseline\BLReset.c)
void BLReset_IsBaselineAbnormalType2(int param_1,char param_2,char param_3);

// BLReset_IsBaselineAbnormalType3 @ 0x6ba4b35f (Baseline\BLReset.c)
void BLReset_IsBaselineAbnormalType3(int param_1,char param_2,char param_3);

// BLReset_IsBaselineClean @ 0x6ba47e09 (Baseline\BLReset.c)
bool BLReset_IsBaselineClean(void);

// BLReset_IsBaselineNoisy @ 0x6ba4c0ba (Baseline\BLReset.c)
undefined1 BLReset_IsBaselineNoisy(void);

// BLReset_IsCurBaselineDirty @ 0x6ba4aaee (Baseline\BLReset.c)
bool BLReset_IsCurBaselineDirty(void);

// BLReset_IsCurBaselineDirtyByDiff @ 0x6ba4aa9e (Baseline\BLReset.c)
undefined1 BLReset_IsCurBaselineDirtyByDiff(void);

// BLReset_IsFullPanelOverNoise @ 0x6ba4ac58 (Baseline\BLReset.c)
undefined8 BLReset_IsFullPanelOverNoise(void);

// BLReset_IsLongTimeRawJumpDetected @ 0x6ba48d9e (Baseline\BLReset.c)
undefined1 BLReset_IsLongTimeRawJumpDetected(void);

// BLReset_IsLongTimeSSNegativeSignal @ 0x6ba4995b (Baseline\BLReset.c)
undefined1 BLReset_IsLongTimeSSNegativeSignal(void);

// BLReset_IsLongTimeSSNegativeSignalStable @ 0x6ba49b66 (Baseline\BLReset.c)
undefined1 BLReset_IsLongTimeSSNegativeSignalStable(void);

// BLReset_IsLongTimeSSNoSignal @ 0x6ba49544 (Baseline\BLReset.c)
bool BLReset_IsLongTimeSSNoSignal(void);

// BLReset_IsMutualSSNotMatch @ 0x6ba49950 (Baseline\BLReset.c)
undefined8 BLReset_IsMutualSSNotMatch(void);

// BLReset_IsRawChanged @ 0x6ba490f5 (Baseline\BLReset.c)
undefined8 BLReset_IsRawChanged(void *param_1,char param_2,char param_3,char param_4);

// BLReset_IsRawChangedSinceSSNoSignal @ 0x6ba4950f (Baseline\BLReset.c)
char BLReset_IsRawChangedSinceSSNoSignal(undefined1 param_1);

// BLReset_IsSmartCoverOff @ 0x6ba49d6c (Baseline\BLReset.c)
bool BLReset_IsSmartCoverOff(void);

// BLReset_IsToBeProtectedBySS @ 0x6ba484b2 (Baseline\BLReset.c)
undefined8 BLReset_IsToBeProtectedBySS(void);

// BLReset_IsToDisable @ 0x6ba47afd (Baseline\BLReset.c)
undefined8 BLReset_IsToDisable(void);

// BLReset_IsTouchProtectOnRawJump @ 0x6ba48bb6 (Baseline\BLReset.c)
undefined8 BLReset_IsTouchProtectOnRawJump(void);

// BLReset_IsTouchToBeProtected @ 0x6ba47b08 (Baseline\BLReset.c)
char BLReset_IsTouchToBeProtected(void);

// BLReset_IsTouchToBeProtected2 @ 0x6ba47e9a (Baseline\BLReset.c)
undefined1 BLReset_IsTouchToBeProtected2(char param_1);

// BLReset_IsTriggered @ 0x6ba4bf6d (Baseline\BLReset.c)
char BLReset_IsTriggered(void);

// BLReset_IsTriggered2 @ 0x6ba4bc66 (Baseline\BLReset.c)
bool BLReset_IsTriggered2(void);

// BLReset_IsTriggered2AfeWakeUp @ 0x6ba4b9d4 (Baseline\BLReset.c)
undefined1 BLReset_IsTriggered2AfeWakeUp(void);

// BLReset_IsTriggered2BySS @ 0x6ba4b890 (Baseline\BLReset.c)
bool BLReset_IsTriggered2BySS(void);

// BLReset_Process @ 0x6ba4bf90 (Baseline\BLReset.c)
char BLReset_Process(char param_1);

// BLReset_Status @ 0x6ba4776c (Baseline\BLReset.c)
undefined1 BLReset_Status(void);

// BLReset_UpdateTimer @ 0x6ba47e53 (Baseline\BLReset.c)
longlong BLReset_UpdateTimer(longlong param_1);

// BLSM_GetMaxSig @ 0x6ba4d41d (Baseline\BLSM.c)
short BLSM_GetMaxSig(void);

// BLSM_GetMinSig @ 0x6ba4d4a0 (Baseline\BLSM.c)
short BLSM_GetMinSig(void);

// BLSM_GetNoTouchBlStep @ 0x6ba4cf9c (Baseline\BLSM.c)
void BLSM_GetNoTouchBlStep(short *param_1,ushort *param_2);

// BLSM_GetProperty @ 0x6ba4d523 (Baseline\BLSM.c)
void BLSM_GetProperty(bool bResetPropertyRequested,bool bRefreshPropertyRequested);

// BLSM_Init @ 0x6ba4c220 (Baseline\BLSM.c)
void BLSM_Init(undefined1 param_1);

// BLSM_IsBaselineOK @ 0x6ba4d7c6 (Baseline\BLSM.c)
undefined8 BLSM_IsBaselineOK(void);

// BLSM_IsOKToIDLE @ 0x6ba4d7fd (Baseline\BLSM.c)
undefined4 BLSM_IsOKToIDLE(void);

// BLSM_IsReset @ 0x6ba4d794 (Baseline\BLSM.c)
undefined4 BLSM_IsReset(void);

// BLSM_IsTouched @ 0x6ba4d7ad (Baseline\BLSM.c)
undefined4 BLSM_IsTouched(void);

// BLSM_PreProcess @ 0x6ba4c36b (Baseline\BLSM.c)
void BLSM_PreProcess(void);

// BLSM_Process @ 0x6ba4c645 (Baseline\BLSM.c)
void BLSM_Process(bool bHardwareReset,bool bForceBaselineProperty);

// BLSM_ProcessStage @ 0x6ba4d170 (Baseline\BLSM.c)
void BLSM_ProcessStage(void);

// BLSM_ProcessUseNextSideThold @ 0x6ba4cf41 (Baseline\BLSM.c)
bool BLSM_ProcessUseNextSideThold(void);

// BLSM_Reset @ 0x6ba4cf23 (Baseline\BLSM.c)
void BLSM_Reset(void);

// BLSM_SaveNormalBl @ 0x6ba4d0c3 (Baseline\BLSM.c)
void BLSM_SaveNormalBl(void);

// BLSM_ShbCaptureBl @ 0x6ba4c422 (Baseline\BLSM.c)
void BLSM_ShbCaptureBl(void);

// BLSM_ShbIsRawChanges @ 0x6ba4c382 (Baseline\BLSM.c)
undefined8 BLSM_ShbIsRawChanges(void);

// BLSM_ShbProcess @ 0x6ba4c4cc (Baseline\BLSM.c)
void BLSM_ShbProcess(void);

// BLSM_UpdateForRawUnstable @ 0x6ba4c27b (Baseline\BLSM.c)
void BLSM_UpdateForRawUnstable(void);

// BLSM_UpdateStage @ 0x6ba4c6f1 (Baseline\BLSM.c)
void BLSM_UpdateStage(void);

// BadChannel_CheckType1 @ 0x6bb42f89 (Signal\BadChannel.c)
void BadChannel_CheckType1(void);

// BadChannel_CheckType2 @ 0x6bb43052 (Signal\BadChannel.c)
void BadChannel_CheckType2(void);

// BadChannel_Detect @ 0x6bb43154 (Signal\BadChannel.c)
void BadChannel_Detect(void);

// BadChannel_DetectType1 @ 0x6bb42be5 (Signal\BadChannel.c)
undefined1 BadChannel_DetectType1(byte param_1,byte param_2,ushort param_3,ushort param_4);

// BadChannel_DetectType2 @ 0x6bb42c80 (Signal\BadChannel.c)
undefined1 BadChannel_DetectType2(byte param_1,ushort param_2,ushort param_3,longlong param_4);

// BadChannel_Free @ 0x6bb42b51 (Signal\BadChannel.c)
void BadChannel_Free(void);

// BadChannel_Init @ 0x6bb42b6c (Signal\BadChannel.c)
void BadChannel_Init(void);

// BadChannel_IsDMDDetected @ 0x6bb42e68 (Signal\BadChannel.c)
undefined8 BadChannel_IsDMDDetected(void);

// BadChannel_Malloc @ 0x6bb42b10 (Signal\BadChannel.c)
bool BadChannel_Malloc(void);

// BadChannel_Process @ 0x6bb4325f (Signal\BadChannel.c)
void BadChannel_Process(void);

// BadChannel_SetDebounce @ 0x6bb42f49 (Signal\BadChannel.c)
void BadChannel_SetDebounce(ushort param_1);

// Baseline_Compensation @ 0x6ba46d00 (Baseline\Baseline.c)
void Baseline_Compensation(void);

// Bezier @ 0x6ba6595d (Common\MISC.c)
int Bezier(int param_1,int param_2,int param_3);

// CMF_CopyCommonModeNoise @ 0x6ba4e7ee (Signal\CMF.c)
void CMF_CopyCommonModeNoise(int param_1,byte param_2,byte param_3,byte param_4,undefined2 param_5);

// CMF_EnablePreCMFDiffLog @ 0x6ba4d94e (Signal\CMF.c)
void CMF_EnablePreCMFDiffLog(undefined1 param_1);

// CMF_Free @ 0x6ba4d8a8 (Signal\CMF.c)
void CMF_Free(void);

// CMF_GetCmfMethod @ 0x6ba4d903 (Signal\CMF.c)
uint CMF_GetCmfMethod(void);

// CMF_GetNoiseValue @ 0x6ba4d97a (Signal\CMF.c)
undefined2 CMF_GetNoiseValue(byte param_1,byte param_2);

// CMF_GetNoiseValueFromBuf @ 0x6ba4d960 (Signal\CMF.c)
short CMF_GetNoiseValueFromBuf();

// CMF_Init @ 0x6ba4d8c1 (Signal\CMF.c)
void CMF_Init();

// CMF_InitNoiseData @ 0x6ba4d9e0 (Signal\CMF.c)
void CMF_InitNoiseData(undefined8 param_1,undefined8 param_2);

// CMF_Malloc @ 0x6ba4d880 (Signal\CMF.c)
bool CMF_Malloc(void);

// CMF_NoiseCal @ 0x6ba4e376 (Signal\CMF.c)
void CMF_NoiseCal(undefined4 *param_1,undefined8 param_2,int param_3,int param_4,int param_5);

// CMF_NoiseStatistics @ 0x6ba4deb4 (Signal\CMF.c)
void CMF_NoiseStatistics(void);

// CMF_Process @ 0x6ba4f2c8 (Signal\CMF.c)
void CMF_Process(void);

// CMF_ProcessCore @ 0x6ba4e799 (Signal\CMF.c)
undefined8 CMF_ProcessCore(longlong param_1,undefined8 param_2,undefined4 param_3,undefined4 param_4, undefined4 param_5,undefined8 param_6);

// CMF_ProcessCoreWeight @ 0x6ba4e55b (Signal\CMF.c)
int CMF_ProcessCoreWeight (longlong param_1,undefined8 param_2,int param_3,int param_4,undefined4 param_5, undefined1 *param_6);

// CMF_ProcessDim @ 0x6ba4e8b5 (Signal\CMF.c)
void CMF_ProcessDim(int param_1,short param_2,short param_3,byte param_4,byte param_5);

// CMF_ProcessDimCommonNoise @ 0x6ba4ebf3 (Signal\CMF.c)
void CMF_ProcessDimCommonNoise(int param_1,short param_2,short param_3,byte param_4,byte param_5);

// CMF_ProcessDimCommonOnBuf @ 0x6ba4eb1d (Signal\CMF.c)
void CMF_ProcessDimCommonOnBuf (undefined8 param_1,undefined8 param_2,int param_3,short param_4,short param_5, byte param_6,byte param_7,undefined1 param_8);

// CMF_ProcessDimUnit @ 0x6ba4e2af (Signal\CMF.c)
int CMF_ProcessDimUnit(undefined4 param_1,undefined1 param_2,short param_3,short param_4, undefined8 param_5,undefined1 param_6,undefined1 param_7);

// CMF_ProcessDimUnitOnBuf @ 0x6ba4e0ec (Signal\CMF.c)
undefined4 CMF_ProcessDimUnitOnBuf (longlong param_1,longlong param_2,int param_3,byte param_4,short param_5,short param_6, undefined8 param_7,byte param_8,byte param_9);

// CMF_PushNoiseData @ 0x6ba4da6a (Signal\CMF.c)
void CMF_PushNoiseData(ushort param_1,short param_2);

// CMF_SaveOrderDiff @ 0x6ba4ecaa (Signal\CMF.c)
void CMF_SaveOrderDiff(void);

// CMF_SavePreCmfDif @ 0x6ba4f0c9 (Signal\CMF.c)
void CMF_SavePreCmfDif(void);

// CTD_BasicCalculation @ 0x6ba551e0 (Tracking\CTD.c)
longlong CTD_BasicCalculation(int param_1,int param_2);

// CTD_ECGetFinalOffset @ 0x6ba554f7 (Tracking\CTD.c)
int CTD_ECGetFinalOffset(int param_1,int param_2);

// CTD_ECGetOffset @ 0x6ba553a6 (Tracking\CTD.c)
undefined2 CTD_ECGetOffset(byte param_1,byte param_2);

// CTD_ECInit @ 0x6ba553a0 (Tracking\CTD.c)
void CTD_ECInit(void);

// CTD_ECProcess @ 0x6ba55ca0 (Tracking\CTD.c)
void CTD_ECProcess(void);

// CTD_ECProcessDim1 @ 0x6ba5555b (Tracking\CTD.c)
void CTD_ECProcessDim1(byte param_1);

// CTD_ECProcessDim2 @ 0x6ba558e4 (Tracking\CTD.c)
void CTD_ECProcessDim2(byte param_1);

// CTD_ECProcessUnit @ 0x6ba55c6b (Tracking\CTD.c)
void CTD_ECProcessUnit(undefined1 param_1);

// CTD_ECUpdatePosToEC @ 0x6ba554d8 (Tracking\CTD.c)
void CTD_ECUpdatePosToEC(undefined1 param_1);

// CTD_General @ 0x6ba5523c (Tracking\CTD.c)
void CTD_General(byte param_1,byte param_2,byte param_3,byte param_4,undefined2 *param_5, undefined2 *param_6);

// CaculateInterval @ 0x6bade69c (Common\MISC.c)
void CaculateInterval(void);

// CalculateAppIndex @ 0x6baf61ed (Tracking\HotZoneAdjust.c)
void CalculateAppIndex(longlong param_1);

// CalculateDistance @ 0x6baf4286 (Tracking\HotZoneAdjust.c)
int CalculateDistance(ushort *param_1,ushort param_2,ushort param_3);

// CalculateExpandZone @ 0x6baf480c (Tracking\HotZoneAdjust.c)
void CalculateExpandZone(short *param_1,ushort *param_2);

// CalculateHotZoneIdx @ 0x6baf50ad (Tracking\HotZoneAdjust.c)
void CalculateHotZoneIdx(ushort *param_1);

// CalculateInternalHotZone @ 0x6baf4f8a (Tracking\HotZoneAdjust.c)
void CalculateInternalHotZone(undefined8 param_1,undefined1 param_2);

// CalculateNewPosition @ 0x6baf4eef (Tracking\HotZoneAdjust.c)
void CalculateNewPosition (longlong param_1,ushort param_2,ushort param_3,longlong param_4,short *param_5, short *param_6);

// CalculateSizeIndex @ 0x6baf46dd (Tracking\HotZoneAdjust.c)
ushort CalculateSizeIndex(ushort *param_1);

// CalculateSuccessNum @ 0x6baf6380 (Tracking\HotZoneAdjust.c)
void CalculateSuccessNum(byte param_1,longlong param_2);

// CalculateTargetZone @ 0x6baf4d02 (Tracking\HotZoneAdjust.c)
void CalculateTargetZone(ushort *param_1,longlong param_2,longlong param_3);

// CalculateTargetZoneLen @ 0x6baf4c6c (Tracking\HotZoneAdjust.c)
void CalculateTargetZoneLen (ushort param_1,ushort param_2,ushort param_3,ushort param_4,ushort *param_5, ushort *param_6);

// ClearDetailDescription @ 0x6baf69b5 (Tracking\HotZoneAdjust.c)
void ClearDetailDescription(void);

// ClearDiffNum @ 0x6baf6268 (Tracking\HotZoneAdjust.c)
void ClearDiffNum(longlong param_1);

// ClearDmdDescription @ 0x6baf69e9 (Tracking\HotZoneAdjust.c)
void ClearDmdDescription(void);

// ClearHotZone @ 0x6baf3be8 (Tracking\HotZoneAdjust.c)
void ClearHotZone(void);

// ClearStatisticClickNum @ 0x6baf6305 (Tracking\HotZoneAdjust.c)
void ClearStatisticClickNum(longlong param_1);

// DB_FastClickProcess @ 0x6ba562d4 (Tracking\DB.c)
void DB_FastClickProcess(byte param_1,byte param_2);

// DB_GetNumBySideTouch @ 0x6ba5674c (Tracking\DB.c)
ushort DB_GetNumBySideTouch(byte param_1,ushort param_2);

// DB_GetNumBySignalSum @ 0x6ba565b2 (Tracking\DB.c)
ushort DB_GetNumBySignalSum(byte param_1,byte param_2,ushort param_3);

// DB_LOSize @ 0x6ba55e20 (Tracking\DB.c)
ushort DB_LOSize(void);

// DB_LiftOffNum @ 0x6ba55e79 (Tracking\DB.c)
ushort DB_LiftOffNum(byte param_1);

// DB_TouchDownNum @ 0x6ba5681c (Tracking\DB.c)
ushort DB_TouchDownNum(byte param_1,byte param_2);

// DB_TouchDownTholdCheck @ 0x6ba55f8d (Tracking\DB.c)
bool DB_TouchDownTholdCheck(byte param_1,byte param_2);

// DataSwitch_ToGrid @ 0x6ba906d0 (Core\DataSwitch.c)
void DataSwitch_ToGrid(void);

// DataSwitch_ToHand @ 0x6ba907ce (Core\DataSwitch.c)
void DataSwitch_ToHand(void);

// DetailDescriptionPutStr @ 0x6baf6865 (Tracking\HotZoneAdjust.c)
void DetailDescriptionPutStr(longlong param_1);

// DmdDescriptionPutStr @ 0x6baf690c (Tracking\HotZoneAdjust.c)
void DmdDescriptionPutStr(longlong param_1);

// ER_BottomMoveInCorrection @ 0x6ba57355 (Tracking\ER.c)
int ER_BottomMoveInCorrection(byte param_1,byte param_2);

// ER_GetFlagMoveInCorrectedButDisabled @ 0x6ba570f4 (Tracking\ER.c)
undefined1 ER_GetFlagMoveInCorrectedButDisabled(void);

// ER_GetLimit @ 0x6ba57101 (Tracking\ER.c)
ushort ER_GetLimit(void);

// ER_GetPrmt @ 0x6ba56ec0 (Tracking\ER.c)
void ER_GetPrmt(undefined4 param_1,undefined2 *param_2,undefined2 *param_3);

// ER_Init @ 0x6ba56f76 (Tracking\ER.c)
void ER_Init(void);

// ER_IsInBottomZone @ 0x6ba572bf (Tracking\ER.c)
undefined8 ER_IsInBottomZone(undefined2 param_1,undefined2 param_2);

// ER_IsTouchNeedMoveInCorrection @ 0x6ba5719b (Tracking\ER.c)
undefined8 ER_IsTouchNeedMoveInCorrection(byte param_1);

// ER_MMPrmtToPixelPrmt @ 0x6ba56e50 (Tracking\ER.c)
undefined2 ER_MMPrmtToPixelPrmt(undefined1 param_1,undefined4 param_2);

// ER_MoveInCheck @ 0x6ba57a07 (Tracking\ER.c)
char ER_MoveInCheck(byte param_1,undefined1 param_2);

// ER_MoveInCorrection @ 0x6ba57786 (Tracking\ER.c)
void ER_MoveInCorrection(undefined1 param_1);

// ER_MoveOutCheck @ 0x6ba57bd3 (Tracking\ER.c)
undefined1 ER_MoveOutCheck(byte param_1);

// ER_MoveOutCorrection @ 0x6ba57e7d (Tracking\ER.c)
void ER_MoveOutCorrection(byte param_1,byte param_2);

// ER_Process @ 0x6ba584b3 (Tracking\ER.c)
void ER_Process(void);

// ER_TouchDownProcess @ 0x6ba57ffd (Tracking\ER.c)
void ER_TouchDownProcess(byte param_1,byte param_2);

// Exception_CheckChargerNoiseInRxLines @ 0x6ba59214 (Detection\Exception.c)
void Exception_CheckChargerNoiseInRxLines(void);

// Exception_CheckPanelSD @ 0x6ba59325 (Detection\Exception.c)
void Exception_CheckPanelSD(void);

// Exception_CheckUnderWater @ 0x6ba589c1 (Detection\Exception.c)
void Exception_CheckUnderWater(void);

// Exception_DebounceProcess @ 0x6ba58b9e (Detection\Exception.c)
void Exception_DebounceProcess(ushort *param_1,int *param_2,int *param_3);

// Exception_Free @ 0x6ba587e3 (Detection\Exception.c)
void Exception_Free(void);

// Exception_GetDetectedChargerNoise @ 0x6ba592e6 (Detection\Exception.c)
undefined8 Exception_GetDetectedChargerNoise(void);

// Exception_GetWaterPrpt @ 0x6ba5882e (Detection\Exception.c)
void Exception_GetWaterPrpt(char param_1);

// Exception_Init @ 0x6ba587f7 (Detection\Exception.c)
void Exception_Init(void);

// Exception_InitVariable @ 0x6ba58aaa (Detection\Exception.c)
void Exception_InitVariable(longlong *param_1,longlong *param_2,longlong *param_3,longlong *param_4);

// Exception_IsChargerNoiseDetected @ 0x6ba592a7 (Detection\Exception.c)
undefined8 Exception_IsChargerNoiseDetected(void);

// Exception_Malloc @ 0x6ba587c0 (Detection\Exception.c)
bool Exception_Malloc(void);

// Exception_Process @ 0x6ba58f25 (Detection\Exception.c)
void Exception_Process(void);

// Exception_ProcessTouchZone @ 0x6ba591d1 (Detection\Exception.c)
void Exception_ProcessTouchZone(undefined8 param_1);

// Exception_UpdatePrevException @ 0x6ba58c5c (Detection\Exception.c)
void Exception_UpdatePrevException(undefined2 *param_1,int *param_2,int *param_3);

// ExtToInt @ 0x6baf409d (Tracking\HotZoneAdjust.c)
void ExtToInt(longlong param_1,byte param_2,longlong param_3,byte param_4);

// FIException_CheckAbnormalTouchZoneBaseline @ 0x6ba595ee (Detection\FIException.c)
undefined1 FIException_CheckAbnormalTouchZoneBaseline(longlong param_1);

// FIException_CheckChargerNoiseInRxLines @ 0x6ba59429 (Detection\FIException.c)
void FIException_CheckChargerNoiseInRxLines(void);

// FIException_CheckPanelSD @ 0x6ba593bc (Detection\FIException.c)
void FIException_CheckPanelSD(void);

// FIException_CheckWetFinger @ 0x6ba59919 (Detection\FIException.c)
undefined8 FIException_CheckWetFinger(longlong param_1);

// FIException_DebounceTouchZone @ 0x6ba59471 (Detection\FIException.c)
void FIException_DebounceTouchZone(longlong param_1);

// FIException_Free @ 0x6ba593a1 (Detection\FIException.c)
void FIException_Free(void);

// FIException_GetRatio @ 0x6ba59bbf (Detection\FIException.c)
void FIException_GetRatio(undefined8 param_1);

// FIException_GetWaterSize @ 0x6ba59733 (Detection\FIException.c)
short FIException_GetWaterSize(byte param_1,byte param_2,short param_3,short param_4,short param_5);

// FIException_Init @ 0x6ba593c2 (Detection\FIException.c)
void FIException_Init(void);

// FIException_IsAroundSmall @ 0x6ba59639 (Detection\FIException.c)
undefined8 FIException_IsAroundSmall(short param_1,short param_2,int param_3,short param_4);

// FIException_IsChargerNoiseDetected @ 0x6ba5942f (Detection\FIException.c)
undefined8 FIException_IsChargerNoiseDetected(void);

// FIException_Malloc @ 0x6ba59360 (Detection\FIException.c)
bool FIException_Malloc(void);

// FIException_ProcessTouchZone @ 0x6ba59bdb (Detection\FIException.c)
void FIException_ProcessTouchZone(longlong param_1);

// FIException_Recording @ 0x6ba5943a (Detection\FIException.c)
void FIException_Recording(uint param_1);

// Filter_GetAdaptiveCoef @ 0x6ba5c690 (Filter\Filter.c)
char Filter_GetAdaptiveCoef(byte param_1,byte param_2);

// Filter_GetCoefBasedOnDistance @ 0x6ba5c99c (Filter\Filter.c)
undefined1 Filter_GetCoefBasedOnDistance(undefined1 param_1,byte param_2);

// Filter_GetForwardNum @ 0x6ba5d12a (Filter\Filter.c)
int Filter_GetForwardNum(uint param_1);

// Filter_GetSpeed @ 0x6ba5cf7a (Filter\Filter.c)
int Filter_GetSpeed(undefined1 param_1,short param_2,char param_3);

// Filter_GetTurnNum @ 0x6ba5d0dc (Filter\Filter.c)
int Filter_GetTurnNum(uint param_1);

// Filter_IIRPDims @ 0x6ba5dcd4 (Filter\Filter.c)
void Filter_IIRPDims(byte param_1,byte param_2);

// Filter_IsCurveReturn @ 0x6ba5d178 (Filter\Filter.c)
void Filter_IsCurveReturn(byte param_1,byte param_2);

// Filter_IsHandLift @ 0x6ba5c644 (Filter\Filter.c)
bool Filter_IsHandLift(byte param_1);

// Filter_IsHandLiftFinish @ 0x6ba5dc30 (Filter\Filter.c)
void Filter_IsHandLiftFinish(byte param_1,byte param_2);

// Filter_IsNeedCurve @ 0x6ba5cec2 (Filter\Filter.c)
undefined1 Filter_IsNeedCurve(byte param_1);

// Filter_IsPredictFinish @ 0x6ba5d662 (Filter\Filter.c)
void Filter_IsPredictFinish(byte param_1,byte param_2);

// Filter_IsTouchLiftOff @ 0x6ba5c363 (Filter\Filter.c)
undefined8 Filter_IsTouchLiftOff(byte param_1,byte param_2);

// Filter_IsTouchLiftOffJitterSuppressEnable @ 0x6ba5c5fc (Filter\Filter.c)
undefined1 Filter_IsTouchLiftOffJitterSuppressEnable(byte param_1);

// Filter_JitFilterProcessUnit @ 0x6ba5e994 (Filter\Filter.c)
void Filter_JitFilterProcessUnit(byte param_1,byte param_2);

// Filter_ModeTouchReportedProcessUnit @ 0x6ba5f1c2 (Filter\Filter.c)
void Filter_ModeTouchReportedProcessUnit(byte param_1,byte param_2);

// Filter_NeedHeavyCoef @ 0x6ba5c240 (Filter\Filter.c)
bool Filter_NeedHeavyCoef(byte param_1);

// Filter_ProcessPDim1 @ 0x6ba5cc76 (Filter\Filter.c)
void Filter_ProcessPDim1(byte param_1,byte param_2,undefined1 param_3);

// Filter_ProcessPDim2 @ 0x6ba5cd9c (Filter\Filter.c)
void Filter_ProcessPDim2(byte param_1,byte param_2,undefined1 param_3);

// Filter_ProcessUnit @ 0x6ba5fd96 (Filter\Filter.c)
void Filter_ProcessUnit(byte param_1,undefined1 param_2);

// Filter_ReportClickToFlickDim1OffsetCheck @ 0x6ba5e149 (Filter\Filter.c)
undefined1 Filter_ReportClickToFlickDim1OffsetCheck(byte param_1,byte param_2);

// Filter_ReportClickToFlickDim2OffsetCheck @ 0x6ba5e409 (Filter\Filter.c)
undefined1 Filter_ReportClickToFlickDim2OffsetCheck(byte param_1,byte param_2);

// Filter_ReportClickToFlickProcessUnit @ 0x6ba5e6c3 (Filter\Filter.c)
undefined8 Filter_ReportClickToFlickProcessUnit(byte param_1,byte param_2);

// Filter_ReportedProcessUnit @ 0x6ba5fb41 (Filter\Filter.c)
void Filter_ReportedProcessUnit(byte param_1,byte param_2);

// GenerateAndGetDetailDescription @ 0x6baf6d4a (Tracking\HotZoneAdjust.c)
undefined * GenerateAndGetDetailDescription(void);

// GenerateDetailDescription @ 0x6baf6a1d (Tracking\HotZoneAdjust.c)
void GenerateDetailDescription(void);

// GenerateDmdDescription @ 0x6baf6c49 (Tracking\HotZoneAdjust.c)
void GenerateDmdDescription(void);

// GridIIR_Process @ 0x6ba61098 (Signal\GridIIR.c)
void GridIIR_Process(void);

// GridIIR_ProcessCore @ 0x6ba60fd5 (Signal\GridIIR.c)
void GridIIR_ProcessCore(longlong param_1,longlong param_2,longlong param_3,ushort param_4, undefined1 param_5);

// GridIIR_ProcessCoreRaw @ 0x6ba60f10 (Signal\GridIIR.c)
void GridIIR_ProcessCoreRaw (longlong param_1,longlong param_2,longlong param_3,ushort param_4,undefined1 param_5 );

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

// HardwareAnalyzer_AfeRawJump @ 0x6baf0912 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_AfeRawJump(void);

// HardwareAnalyzer_AfeRawUnstable @ 0x6baf0c82 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_AfeRawUnstable(void);

// HardwareAnalyzer_AfeRawUnstableCoreDetect @ 0x6baf09e9 (Signal\HardwareAnalyzer.c)
undefined8 HardwareAnalyzer_AfeRawUnstableCoreDetect(void);

// HardwareAnalyzer_AfeRawUnstableSideDetect @ 0x6baf0bc1 (Signal\HardwareAnalyzer.c)
undefined8 HardwareAnalyzer_AfeRawUnstableSideDetect(void);

// HardwareAnalyzer_AppendExceptionDescription @ 0x6baf0044 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_AppendExceptionDescription(undefined8 param_1,undefined4 param_2);

// HardwareAnalyzer_AppendExceptionDescription2 @ 0x6baf008b (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_AppendExceptionDescription2(undefined8 param_1,undefined8 param_2);

// HardwareAnalyzer_BackLightShift @ 0x6baf0dd9 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_BackLightShift(void);

// HardwareAnalyzer_BrokenSensorStatByDifZ8 @ 0x6baf2a26 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_BrokenSensorStatByDifZ8(void);

// HardwareAnalyzer_CheckCrossAround @ 0x6baf364b (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_CheckCrossAround(int param_1,int param_2);

// HardwareAnalyzer_CheckException @ 0x6baf0182 (Signal\HardwareAnalyzer.c)
undefined4 HardwareAnalyzer_CheckException(undefined4 param_1);

// HardwareAnalyzer_ClearExceptionDescription @ 0x6baefab0 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_ClearExceptionDescription(void);

// HardwareAnalyzer_CopyBadChannelList @ 0x6baf38c1 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_CopyBadChannelList(void);

// HardwareAnalyzer_DebounceBrokenSensorByDif @ 0x6baf354a (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_DebounceBrokenSensorByDif(int param_1,int param_2);

// HardwareAnalyzer_DebounceBrokenSensorByDifZ8 @ 0x6baf2804 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_DebounceBrokenSensorByDifZ8(char param_1,char param_2);

// HardwareAnalyzer_DetectType3 @ 0x6baf32fa (Signal\HardwareAnalyzer.c)
undefined1 HardwareAnalyzer_DetectType3(byte param_1,byte param_2);

// HardwareAnalyzer_DetectType4 @ 0x6baf37f5 (Signal\HardwareAnalyzer.c)
undefined1 HardwareAnalyzer_DetectType4(byte param_1,byte param_2);

// HardwareAnalyzer_EventAnalyzer @ 0x6baf1e4e (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_EventAnalyzer(void);

// HardwareAnalyzer_Free @ 0x6baf23ed (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_Free(void);

// HardwareAnalyzer_GenerateExceptionDescription @ 0x6baf001a (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_GenerateExceptionDescription(void);

// HardwareAnalyzer_GetCouplingNoiseLevel @ 0x6baf20c4 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_GetCouplingNoiseLevel(void);

// HardwareAnalyzer_GetDetectedException @ 0x6baf0249 (Signal\HardwareAnalyzer.c)
int HardwareAnalyzer_GetDetectedException(void);

// HardwareAnalyzer_GetDetectedNewException @ 0x6baf013d (Signal\HardwareAnalyzer.c)
uint HardwareAnalyzer_GetDetectedNewException(void);

// HardwareAnalyzer_GetExceptionDescription @ 0x6baf0009 (Signal\HardwareAnalyzer.c)
undefined * HardwareAnalyzer_GetExceptionDescription(void);

// HardwareAnalyzer_GetMaxEllipseRateInCore @ 0x6baf1c62 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_GetMaxEllipseRateInCore(void);

// HardwareAnalyzer_GetNumOverNoiseAfterCMF @ 0x6baf101f (Signal\HardwareAnalyzer.c)
short HardwareAnalyzer_GetNumOverNoiseAfterCMF(void);

// HardwareAnalyzer_GetNumOverNoiseAfterCMFNoneSOF @ 0x6baf0ef7 (Signal\HardwareAnalyzer.c)
short HardwareAnalyzer_GetNumOverNoiseAfterCMFNoneSOF(void);

// HardwareAnalyzer_GetRaw2RawPrpt @ 0x6baf10dc (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_GetRaw2RawPrpt(void);

// HardwareAnalyzer_Init @ 0x6baefb27 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_Init(void);

// HardwareAnalyzer_IsAnyExceptionDetected @ 0x6baf016a (Signal\HardwareAnalyzer.c)
bool HardwareAnalyzer_IsAnyExceptionDetected(void);

// HardwareAnalyzer_IsNotOKToEnterIDLEMode @ 0x6baf06ff (Signal\HardwareAnalyzer.c)
undefined8 HardwareAnalyzer_IsNotOKToEnterIDLEMode(void);

// HardwareAnalyzer_IsSafeForCheckDif @ 0x6baf31fc (Signal\HardwareAnalyzer.c)
undefined8 HardwareAnalyzer_IsSafeForCheckDif(void);

// HardwareAnalyzer_IsSensorBrokenByDifZ8 @ 0x6baf258e (Signal\HardwareAnalyzer.c)
undefined1 HardwareAnalyzer_IsSensorBrokenByDifZ8(byte param_1,byte param_2);

// HardwareAnalyzer_IsSensorBrokenByRawRange @ 0x6baf2f76 (Signal\HardwareAnalyzer.c)
undefined8 HardwareAnalyzer_IsSensorBrokenByRawRange(undefined1 param_1,undefined1 param_2);

// HardwareAnalyzer_IsSensorOverRawDeltaLimit @ 0x6baf2cd7 (Signal\HardwareAnalyzer.c)
bool HardwareAnalyzer_IsSensorOverRawDeltaLimit(byte param_1,byte param_2);

// HardwareAnalyzer_IsSensorOverRawLimit @ 0x6baf2c5a (Signal\HardwareAnalyzer.c)
bool HardwareAnalyzer_IsSensorOverRawLimit(byte param_1,byte param_2);

// HardwareAnalyzer_IsSpecifiedExceptionDetected @ 0x6baf00d4 (Signal\HardwareAnalyzer.c)
undefined4 HardwareAnalyzer_IsSpecifiedExceptionDetected(uint param_1,byte param_2);

// HardwareAnalyzer_IsToBypassCurrentFrame @ 0x6baf14ff (Signal\HardwareAnalyzer.c)
undefined1 HardwareAnalyzer_IsToBypassCurrentFrame(void);

// HardwareAnalyzer_LargeDisplayNoise @ 0x6baf0e82 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_LargeDisplayNoise(void);

// HardwareAnalyzer_Malloc @ 0x6baf2350 (Signal\HardwareAnalyzer.c)
undefined8 HardwareAnalyzer_Malloc(void);

// HardwareAnalyzer_PostProcess @ 0x6baf2151 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_PostProcess(void);

// HardwareAnalyzer_PostProcessRaw @ 0x6baf2005 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_PostProcessRaw(void);

// HardwareAnalyzer_PostProcessTouch @ 0x6baf1f82 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_PostProcessTouch(void);

// HardwareAnalyzer_PrintBackground @ 0x6baefe52 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_PrintBackground(void);

// HardwareAnalyzer_Process @ 0x6baf19de (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_Process(void);

// HardwareAnalyzer_ProcessByRaw2Raw @ 0x6baf173a (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_ProcessByRaw2Raw(void);

// HardwareAnalyzer_ProcessDif @ 0x6baf1c4e (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_ProcessDif(void);

// HardwareAnalyzer_ProcessProximity @ 0x6baf1673 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_ProcessProximity(void);

// HardwareAnalyzer_ProcessRFNoise @ 0x6baf15ee (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_ProcessRFNoise(void);

// HardwareAnalyzer_ProcessSideTouch @ 0x6baf16ca (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_ProcessSideTouch(void);

// HardwareAnalyzer_PutStr @ 0x6baefd87 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_PutStr(longlong param_1);

// HardwareAnalyzer_RawLimitProcess @ 0x6baf070a (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_RawLimitProcess(void);

// HardwareAnalyzer_ReportToTSA @ 0x6baf159b (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_ReportToTSA(void);

// HardwareAnalyzer_Reset @ 0x6baf2499 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_Reset(void);

// HardwareAnalyzer_SensorBrokenDifProcess @ 0x6baf3975 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_SensorBrokenDifProcess(void);

// HardwareAnalyzer_SensorBrokenInit @ 0x6baf3755 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_SensorBrokenInit(void);

// HardwareAnalyzer_SensorBrokenPreDifProcess @ 0x6baf3179 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_SensorBrokenPreDifProcess(void);

// HardwareAnalyzer_SensorBrokenRawProcess @ 0x6baf2fbe (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_SensorBrokenRawProcess(void);

// HardwareAnalyzer_SensorBrokenReset @ 0x6baf2420 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_SensorBrokenReset(void);

// HardwareAnalyzer_SetException @ 0x6baf00f6 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_SetException(byte param_1);

// HardwareAnalyzer_SetSoftException @ 0x6baf0124 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_SetSoftException(void);

// HardwareAnalyzer_Z8InfoUpdate @ 0x6baf24b2 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_Z8InfoUpdate (byte param_1,byte param_2,char *param_3,int *param_4,int *param_5);

// HasHotZone @ 0x6baf4008 (Tracking\HotZoneAdjust.c)
undefined8 HasHotZone(void);

// HotZoneAdjustClearAnalyzer @ 0x6baf3fb1 (Tracking\HotZoneAdjust.c)
void HotZoneAdjustClearAnalyzer(void);

// HotZoneAdjustIsFeatureOn @ 0x6baf404a (Tracking\HotZoneAdjust.c)
undefined4 HotZoneAdjustIsFeatureOn(void);

// HotZoneAdjustPoint @ 0x6baf5859 (Tracking\HotZoneAdjust.c)
void HotZoneAdjustPoint(byte param_1,undefined8 param_2,undefined8 param_3);

// HotZoneAdjustProcessAfter @ 0x6baf5f50 (Tracking\HotZoneAdjust.c)
void HotZoneAdjustProcessAfter(byte param_1,byte param_2);

// HotZoneAdjustProcessFirst @ 0x6baf5d93 (Tracking\HotZoneAdjust.c)
void HotZoneAdjustProcessFirst(byte param_1);

// HotZoneAdjustProcessFirstUnit @ 0x6baf5c13 (Tracking\HotZoneAdjust.c)
void HotZoneAdjustProcessFirstUnit(byte param_1,undefined2 *param_2);

// HotZoneAdjustScenario @ 0x6baf406a (Tracking\HotZoneAdjust.c)
undefined8 HotZoneAdjustScenario(void);

// HotZoneAdjust_GenerateAndGetDmdDescription @ 0x6baf6d70 (Tracking\HotZoneAdjust.c)
undefined * HotZoneAdjust_GenerateAndGetDmdDescription(void);

// HotZoneAdjust_Init @ 0x6baf3fdd (Tracking\HotZoneAdjust.c)
void HotZoneAdjust_Init(void);

// HotZoneAdjust_Process @ 0x6baf729e (Tracking\HotZoneAdjust.c)
void HotZoneAdjust_Process(byte param_1,byte param_2);

// HotZoneIsInCore @ 0x6baf51b8 (Tracking\HotZoneAdjust.c)
undefined8 HotZoneIsInCore(longlong param_1);

// HotZoneRecord @ 0x6baf726b (Tracking\HotZoneAdjust.c)
void HotZoneRecord(undefined1 param_1,undefined8 param_2);

// IDT_AccCheck @ 0x6ba619a3 (Tracking\IDT.c)
void IDT_AccCheck(char param_1,longlong param_2,uint param_3);

// IDT_AlwaysMatchProcess @ 0x6ba620ac (Tracking\IDT.c)
void IDT_AlwaysMatchProcess(longlong param_1,char param_2,longlong param_3,byte param_4);

// IDT_GetAcc @ 0x6ba61704 (Tracking\IDT.c)
int IDT_GetAcc(int param_1,int param_2);

// IDT_HM @ 0x6ba61ba2 (Tracking\IDT.c)
void IDT_HM(byte param_1,byte param_2,longlong param_3,longlong param_4);

// IDT_HMProcess @ 0x6ba61323 (Tracking\IDT.c)
void IDT_HMProcess(longlong param_1,uint param_2);

// IDT_Init @ 0x6ba611a0 (Tracking\IDT.c)
void IDT_Init(void);

// IDT_IsEdgeTouch @ 0x6ba622e9 (Tracking\IDT.c)
undefined8 IDT_IsEdgeTouch(byte param_1);

// IDT_IsToIncAccThold @ 0x6ba615c7 (Tracking\IDT.c)
undefined8 IDT_IsToIncAccThold(byte param_1,byte param_2);

// IDT_Map @ 0x6ba6122e (Tracking\IDT.c)
void IDT_Map(longlong param_1,uint param_2);

// IDT_PopulateDistanceMap @ 0x6ba623e4 (Tracking\IDT.c)
void IDT_PopulateDistanceMap(longlong param_1,char param_2);

// IDT_Process @ 0x6ba611a6 (Tracking\IDT.c)
void IDT_Process(uchar bUseStrictAccCheck);

// IIR_Process @ 0x6ba601b0 (Filter\IIR.c)
int IIR_Process(int param_1,int param_2,byte param_3,byte param_4);

// IIR_Process2 @ 0x6ba6021b (Filter\IIR.c)
int IIR_Process2(int param_1,int param_2,byte param_3,byte param_4);

// Image_ClearNeighbor @ 0x6ba64f90 (Detection\Image.c)
void Image_ClearNeighbor(longlong param_1,int param_2,int param_3,int param_4,int param_5, int param_6,int param_7);

// Image_CrossErosion @ 0x6ba65625 (Detection\Image.c)
void Image_CrossErosion(void *param_1,int param_2,int param_3,int param_4,int param_5);

// Image_CrossErosionDim1 @ 0x6ba6540b (Detection\Image.c)
void Image_CrossErosionDim1(longlong param_1,longlong param_2,uint param_3,int param_4);

// Image_CrossErosionDim2 @ 0x6ba65518 (Detection\Image.c)
void Image_CrossErosionDim2(longlong param_1,longlong param_2,uint param_3,int param_4);

// Image_Dilation @ 0x6ba6511d (Detection\Image.c)
void Image_Dilation(void *param_1,int param_2,int param_3,undefined4 param_4,undefined4 param_5);

// Image_Erosion @ 0x6ba652ac (Detection\Image.c)
void Image_Erosion(void *param_1,int param_2,int param_3,undefined4 param_4,undefined4 param_5);

// Image_IsWhiteInNeighbor @ 0x6ba6504f (Detection\Image.c)
undefined8 Image_IsWhiteInNeighbor (longlong param_1,int param_2,int param_3,int param_4,int param_5,int param_6,int param_7);

// IsInAdjustArea @ 0x6baf4e28 (Tracking\HotZoneAdjust.c)
undefined8 IsInAdjustArea(undefined2 param_1,undefined2 param_2,longlong param_3);

// IsInAdjustLine @ 0x6baf4dd6 (Tracking\HotZoneAdjust.c)
undefined8 IsInAdjustLine(ushort param_1,int param_2,ushort param_3);

// IsInHotZone @ 0x6baf4e98 (Tracking\HotZoneAdjust.c)
undefined8 IsInHotZone(ushort param_1,ushort param_2,ushort *param_3);

// IsNeedRecord @ 0x6baf6dae (Tracking\HotZoneAdjust.c)
undefined8 IsNeedRecord(byte param_1,longlong param_2);

// JITFilter @ 0x6ba602c6 (Common\MISC.c)
void JITFilter(int *param_1,int *param_2,int *param_3);

// Misc_SqrtUint32 @ 0x6ba64d8b (Common\MISC.c)
int Misc_SqrtUint32(uint param_1);

// Peak_CalcZn @ 0x6ba67c89 (Detection\Peak.c)
void Peak_CalcZn(byte param_1);

// Peak_Clean @ 0x6ba659b0 (Detection\Peak.c)
void Peak_Clean(void);

// Peak_DetectInRange @ 0x6ba685d7 (Detection\Peak.c)
void Peak_DetectInRange(short param_1,byte param_2,byte param_3,byte param_4,byte param_5);

// Peak_Fill @ 0x6ba65cb8 (Detection\Peak.c)
void Peak_Fill(byte param_1,byte param_2,byte param_3);

// Peak_GetDim1DistToEdge @ 0x6ba67b4d (Detection\Peak.c)
byte Peak_GetDim1DistToEdge(byte param_1);

// Peak_GetDim2DistToEdge @ 0x6ba67bca (Detection\Peak.c)
byte Peak_GetDim2DistToEdge(byte param_1);

// Peak_GetDistToEdge @ 0x6ba67c47 (Detection\Peak.c)
byte Peak_GetDistToEdge(undefined1 param_1);

// Peak_GetDistance @ 0x6ba66bbd (Detection\Peak.c)
byte Peak_GetDistance(byte param_1,byte param_2);

// Peak_GetIdxBySig @ 0x6ba666f3 (Detection\Peak.c)
byte Peak_GetIdxBySig(short param_1);

// Peak_GetMaxPeak @ 0x6ba683f0 (Detection\Peak.c)
int Peak_GetMaxPeak(void);

// Peak_GetSquareDistance @ 0x6ba66c86 (Detection\Peak.c)
int Peak_GetSquareDistance(byte param_1,byte param_2);

// Peak_GetZ9 @ 0x6ba65a72 (Detection\Peak.c)
int Peak_GetZ9(byte param_1);

// Peak_IDTracking @ 0x6ba68167 (Detection\Peak.c)
void Peak_IDTracking(void);

// Peak_Insert @ 0x6ba65e68 (Detection\Peak.c)
uint Peak_Insert(byte param_1,byte param_2);

// Peak_IsFullIncellInsideSensor @ 0x6ba68944 (Detection\Peak.c)
bool Peak_IsFullIncellInsideSensor(byte param_1);

// Peak_IsFullIncellSDDetected @ 0x6ba689c9 (Detection\Peak.c)
uchar Peak_IsFullIncellSDDetected(void);

// Peak_IsInNxNBox @ 0x6ba67a7e (Detection\Peak.c)
undefined1 Peak_IsInNxNBox(undefined1 param_1,undefined1 param_2,undefined1 param_3);

// Peak_IsOnInnerEdgeDim1 @ 0x6ba66868 (Detection\Peak.c)
undefined8 Peak_IsOnInnerEdgeDim1(undefined1 param_1);

// Peak_IsOnInnerEdgeDim1Far @ 0x6ba6679b (Detection\Peak.c)
undefined4 Peak_IsOnInnerEdgeDim1Far(byte param_1);

// Peak_IsOnInnerEdgeDim1Near @ 0x6ba66764 (Detection\Peak.c)
bool Peak_IsOnInnerEdgeDim1Near(byte param_1);

// Peak_IsOnInnerEdgeDim2Far @ 0x6ba6681d (Detection\Peak.c)
undefined4 Peak_IsOnInnerEdgeDim2Far(byte param_1);

// Peak_IsOnInnerEdgeDim2Near @ 0x6ba667e6 (Detection\Peak.c)
bool Peak_IsOnInnerEdgeDim2Near(byte param_1);

// Peak_IsOnLine @ 0x6ba679f2 (Detection\Peak.c)
undefined1 Peak_IsOnLine(int param_1,uchar param_2,byte param_3);

// Peak_IsOnOuterEdge @ 0x6ba66a9d (Detection\Peak.c)
undefined8 Peak_IsOnOuterEdge(undefined1 param_1);

// Peak_IsOnOuterEdgeDim1 @ 0x6ba66a23 (Detection\Peak.c)
undefined8 Peak_IsOnOuterEdgeDim1(undefined1 param_1);

// Peak_IsOnOuterEdgeDim1Far @ 0x6ba66956 (Detection\Peak.c)
undefined4 Peak_IsOnOuterEdgeDim1Far(byte param_1);

// Peak_IsOnOuterEdgeDim1Near @ 0x6ba6691f (Detection\Peak.c)
bool Peak_IsOnOuterEdgeDim1Near(byte param_1);

// Peak_IsOnOuterEdgeDim2 @ 0x6ba66a60 (Detection\Peak.c)
undefined8 Peak_IsOnOuterEdgeDim2(undefined1 param_1);

// Peak_IsOnOuterEdgeDim2Far @ 0x6ba669d8 (Detection\Peak.c)
undefined4 Peak_IsOnOuterEdgeDim2Far(byte param_1);

// Peak_IsOnOuterEdgeDim2Near @ 0x6ba669a1 (Detection\Peak.c)
bool Peak_IsOnOuterEdgeDim2Near(byte param_1);

// Peak_IsToExcludeSideRegion @ 0x6ba685ac (Detection\Peak.c)
undefined8 Peak_IsToExcludeSideRegion(void);

// Peak_IsTouchValid @ 0x6ba65a3e (Detection\Peak.c)
undefined4 Peak_IsTouchValid(byte param_1);

// Peak_Process @ 0x6ba68c4b (Detection\Peak.c)
void Peak_Process(void);

// Peak_Remove @ 0x6ba65b60 (Detection\Peak.c)
void Peak_Remove(byte param_1);

// Peak_SortInSigAscending @ 0x6ba66013 (Detection\Peak.c)
void Peak_SortInSigAscending(void);

// Peak_SortInSigDescending @ 0x6ba66412 (Detection\Peak.c)
void Peak_SortInSigDescending(void);

// Peak_Z1Filter @ 0x6ba6848b (Detection\Peak.c)
void Peak_Z1Filter(short param_1);

// Peak_Z8Filter @ 0x6ba68502 (Detection\Peak.c)
void Peak_Z8Filter(void);

// Peak_needUDFPLargeFingerWorkaround @ 0x6ba68bc6 (Detection\Peak.c)
undefined8 Peak_needUDFPLargeFingerWorkaround(void);

// PolynomialRegression_SetComb @ 0x6baf7460 (Tracking\PolynomialRegression.c)
undefined8 PolynomialRegression_SetComb(longlong param_1,ushort param_2,byte param_3,byte param_4);

// PosnQueue_Init @ 0x6bb41e70 (Tracking\PosnSpecial.c)
void PosnQueue_Init(undefined8 param_1,undefined1 param_2);

// PosnSpecial_AddCellInZone @ 0x6bb421ae (Tracking\PosnSpecial.c)
void PosnSpecial_AddCellInZone(ushort param_1);

// PosnSpecial_Get @ 0x6bb42624 (Tracking\PosnSpecial.c)
undefined2 PosnSpecial_Get(void);

// PosnSpecial_GetNumEdgeSensor @ 0x6bb42466 (Tracking\PosnSpecial.c)
undefined1 PosnSpecial_GetNumEdgeSensor(void);

// PosnSpecial_GetNumSignChange @ 0x6bb42444 (Tracking\PosnSpecial.c)
undefined1 PosnSpecial_GetNumSignChange(void);

// PosnSpecial_GetNumValidSensor @ 0x6bb42455 (Tracking\PosnSpecial.c)
undefined1 PosnSpecial_GetNumValidSensor(void);

// PosnSpecial_GetPosMarker @ 0x6bb423f7 (Tracking\PosnSpecial.c)
char PosnSpecial_GetPosMarker(void);

// PosnSpecial_GetSumEdgeSensor @ 0x6bb42477 (Tracking\PosnSpecial.c)
undefined4 PosnSpecial_GetSumEdgeSensor(void);

// PosnSpecial_Init @ 0x6bb422b6 (Tracking\PosnSpecial.c)
void PosnSpecial_Init(undefined8 param_1,undefined2 param_2);

// PosnSpecial_IsEmpty @ 0x6bb42761 (Tracking\PosnSpecial.c)
undefined1 PosnSpecial_IsEmpty(void);

// PosnSpecial_IsNegEmpty @ 0x6bb42067 (Tracking\PosnSpecial.c)
undefined2 PosnSpecial_IsNegEmpty(void);

// PosnSpecial_IsPosEmpty @ 0x6bb42050 (Tracking\PosnSpecial.c)
undefined2 PosnSpecial_IsPosEmpty(void);

// PosnSpecial_NextZone @ 0x6bb42163 (Tracking\PosnSpecial.c)
void PosnSpecial_NextZone(void);

// PosnSpecial_Put @ 0x6bb42569 (Tracking\PosnSpecial.c)
void PosnSpecial_Put(ushort param_1);

// PosnSpecial_ReBase @ 0x6bb423b7 (Tracking\PosnSpecial.c)
void PosnSpecial_ReBase(void);

// PosnSpecial_Record @ 0x6bb42487 (Tracking\PosnSpecial.c)
void PosnSpecial_Record(short param_1,short param_2,char param_3);

// PosnSpecial_Record2 @ 0x6bb4251d (Tracking\PosnSpecial.c)
void PosnSpecial_Record2(short param_1,short param_2);

// PosnSpecial_ResetZone @ 0x6bb420ba (Tracking\PosnSpecial.c)
void PosnSpecial_ResetZone(byte param_1);

// PosnSpecial_UpdateSign @ 0x6bb42265 (Tracking\PosnSpecial.c)
void PosnSpecial_UpdateSign(void);

// PressureDrift_Detect @ 0x6bb441c9 (Baseline\PressureDrift.c)
undefined8 PressureDrift_Detect(byte param_1,byte param_2);

// PressureDrift_DetectByTimeStamp @ 0x6bb44172 (Baseline\PressureDrift.c)
undefined8 PressureDrift_DetectByTimeStamp(void);

// PressureDrift_GradientCalUnit @ 0x6bb440d0 (Baseline\PressureDrift.c)
int PressureDrift_GradientCalUnit(byte param_1,byte param_2);

// PrevPeak_Init @ 0x6ba68f10 (Detection\PrevPeak.c)
void PrevPeak_Init(void);

// PrevPeak_IsOnOuterEdge @ 0x6ba692c0 (Detection\PrevPeak.c)
undefined8 PrevPeak_IsOnOuterEdge(undefined1 param_1);

// PrevPeak_IsOnOuterEdgeDim1 @ 0x6ba69246 (Detection\PrevPeak.c)
undefined8 PrevPeak_IsOnOuterEdgeDim1(undefined1 param_1);

// PrevPeak_IsOnOuterEdgeDim1Far @ 0x6ba69188 (Detection\PrevPeak.c)
undefined4 PrevPeak_IsOnOuterEdgeDim1Far(byte param_1);

// PrevPeak_IsOnOuterEdgeDim1Near @ 0x6ba69156 (Detection\PrevPeak.c)
bool PrevPeak_IsOnOuterEdgeDim1Near(byte param_1);

// PrevPeak_IsOnOuterEdgeDim2 @ 0x6ba69283 (Detection\PrevPeak.c)
undefined8 PrevPeak_IsOnOuterEdgeDim2(undefined1 param_1);

// PrevPeak_IsOnOuterEdgeDim2Far @ 0x6ba69200 (Detection\PrevPeak.c)
undefined4 PrevPeak_IsOnOuterEdgeDim2Far(byte param_1);

// PrevPeak_IsOnOuterEdgeDim2Near @ 0x6ba691ce (Detection\PrevPeak.c)
bool PrevPeak_IsOnOuterEdgeDim2Near(byte param_1);

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

// PrevTouch_HistoryBufferGetNum @ 0x6ba6e9dc (Tracking\PrevTouch.c)
undefined1 PrevTouch_HistoryBufferGetNum(byte param_1);

// PrevTouch_HistoryBufferInsert @ 0x6ba6ef44 (Tracking\PrevTouch.c)
void PrevTouch_HistoryBufferInsert(byte param_1,byte param_2,undefined8 *param_3);

// PrevTouch_HistoryBufferPop @ 0x6ba6ec0d (Tracking\PrevTouch.c)
undefined8 PrevTouch_HistoryBufferPop(byte param_1,undefined8 *param_2);

// PrevTouch_HistoryBufferPush @ 0x6ba6ea52 (Tracking\PrevTouch.c)
undefined8 PrevTouch_HistoryBufferPush(byte param_1,undefined8 *param_2);

// PrevTouch_HistoryBufferRemove @ 0x6ba6f11a (Tracking\PrevTouch.c)
void PrevTouch_HistoryBufferRemove(byte param_1,byte param_2);

// PrevTouch_HistoryQueuePush @ 0x6ba6e650 (Tracking\PrevTouch.c)
undefined8 PrevTouch_HistoryQueuePush(byte param_1,ushort param_2,ushort param_3);

// PrevTouch_HistoryQueuePush_internal @ 0x6ba6e448 (Tracking\PrevTouch.c)
undefined8 PrevTouch_HistoryQueuePush_internal(byte param_1,ushort param_2,ushort param_3);

// PrevTouch_Init @ 0x6ba6db42 (Tracking\PrevTouch.c)
void PrevTouch_Init(void);

// PrevTouch_IsAnyExceptionDetected @ 0x6ba6e898 (Tracking\PrevTouch.c)
undefined1 PrevTouch_IsAnyExceptionDetected(void);

// PrevTouch_IsHistoryBufferFull @ 0x6ba6f3ef (Tracking\PrevTouch.c)
bool PrevTouch_IsHistoryBufferFull(byte param_1);

// PrevTouch_IsValidId @ 0x6ba6e909 (Tracking\PrevTouch.c)
undefined8 PrevTouch_IsValidId(char param_1);

// PrevTouch_PostProcess @ 0x6ba6e09d (Tracking\PrevTouch.c)
void PrevTouch_PostProcess(void);

// PrevTouch_Release @ 0x6ba6da90 (Tracking\PrevTouch.c)
void PrevTouch_Release(void);

// PrevTouch_RemoveOff @ 0x6ba6de46 (Tracking\PrevTouch.c)
void PrevTouch_RemoveOff(void);

// PrevTouch_UnitCopy @ 0x6ba6d300 (Tracking\PrevTouch.c)
void PrevTouch_UnitCopy(byte param_1,byte param_2);

// PrevTouch_UnitRelease @ 0x6ba6da04 (Tracking\PrevTouch.c)
void PrevTouch_UnitRelease(byte param_1);

// PrevTouch_UnitRemove @ 0x6ba6dcf3 (Tracking\PrevTouch.c)
void PrevTouch_UnitRemove(byte param_1);

// PrevTouch_UnitUpdate @ 0x6ba6d5ed (Tracking\PrevTouch.c)
void PrevTouch_UnitUpdate(byte param_1,byte param_2);

// PrevTouch_UnitUpdateCommon @ 0x6ba6d427 (Tracking\PrevTouch.c)
void PrevTouch_UnitUpdateCommon(byte param_1,byte param_2);

// RFNoise_CopyPrevRaw2RawDif @ 0x6bb4463a (Detection\RFNoise.c)
void RFNoise_CopyPrevRaw2RawDif(void);

// RFNoise_Detect @ 0x6bb44ceb (Detection\RFNoise.c)
undefined1 RFNoise_Detect(void);

// RFNoise_Free @ 0x6bb44483 (Detection\RFNoise.c)
void RFNoise_Free(void);

// RFNoise_GetNoiseValue @ 0x6bb44ca1 (Detection\RFNoise.c)
undefined2 RFNoise_GetNoiseValue(void);

// RFNoise_GetNumNoisyLine @ 0x6bb44cc6 (Detection\RFNoise.c)
undefined1 RFNoise_GetNumNoisyLine(void);

// RFNoise_Init @ 0x6bb444b6 (Detection\RFNoise.c)
void RFNoise_Init(void);

// RFNoise_IsDetected @ 0x6bb44c44 (Detection\RFNoise.c)
undefined1 RFNoise_IsDetected(void);

// RFNoise_IsVariationInRangeUpDown @ 0x6bb449e6 (Detection\RFNoise.c)
undefined8 RFNoise_IsVariationInRangeUpDown(int param_1,int param_2,byte param_3);

// RFNoise_Malloc @ 0x6bb443e0 (Detection\RFNoise.c)
undefined8 RFNoise_Malloc(void);

// RFNoise_Raw2RawProcessDim @ 0x6bb44a73 (Detection\RFNoise.c)
undefined1 RFNoise_Raw2RawProcessDim(int param_1,short *param_2,char *param_3);

// RFNoise_Raw2RawProcessDimUnit @ 0x6bb446d9 (Detection\RFNoise.c)
void RFNoise_Raw2RawProcessDimUnit (int param_1,byte param_2,short *param_3,short *param_4,short *param_5,int *param_6);

// Rawdata_CMF @ 0x6ba69381 (Signal\Rawdata.c)
void Rawdata_CMF(void);

// Rawdata_GetUnNormalizedBaseline @ 0x6ba69937 (Signal\Rawdata.c)
void Rawdata_GetUnNormalizedBaseline(longlong param_1,undefined4 param_2);

// Rawdata_Init @ 0x6ba69300 (Signal\Rawdata.c)
void Rawdata_Init(void);

// Rawdata_IsProcessNeeded @ 0x6ba69306 (Signal\Rawdata.c)
undefined1 Rawdata_IsProcessNeeded(void);

// Rawdata_NormalizeGrid @ 0x6ba6946a (Signal\Rawdata.c)
void Rawdata_NormalizeGrid(void *param_1,uint param_2,void *param_3,uint param_4);

// Rawdata_NormalizeGridUnit @ 0x6ba693f0 (Signal\Rawdata.c)
int Rawdata_NormalizeGridUnit(ushort param_1);

// Rawdata_Process @ 0x6ba69581 (Signal\Rawdata.c)
void Rawdata_Process(void);

// Rawdata_RetrieveBaseline @ 0x6ba696ed (Signal\Rawdata.c)
void Rawdata_RetrieveBaseline (longlong param_1,undefined8 param_2,longlong param_3,undefined8 param_4, longlong param_5, ...);

// RxLineFilter_Init @ 0x6ba60440 (Signal\RxLineFilter.c)
void RxLineFilter_Init(void);

// RxLineFilter_IsPossibleFalseTouch @ 0x6ba6069c (Signal\RxLineFilter.c)
undefined1 RxLineFilter_IsPossibleFalseTouch(byte param_1,undefined1 param_2);

// RxLineFilter_IsZ8Failed @ 0x6ba60446 (Signal\RxLineFilter.c)
undefined8 RxLineFilter_IsZ8Failed(byte param_1,byte param_2);

// RxLineFilter_Process @ 0x6ba60827 (Signal\RxLineFilter.c)
void RxLineFilter_Process(void);

// SWTimer_IsExpired @ 0x6bb42843 (Core\SWTimer.c)
undefined8 SWTimer_IsExpired(int param_1,char *param_2,uint param_3);

// SWTimer_Start @ 0x6bb427d0 (Core\SWTimer.c)
void SWTimer_Start(undefined4 param_1,char *param_2);

// SWTimer_Stop @ 0x6bb42802 (Core\SWTimer.c)
void SWTimer_Stop(undefined1 *param_1);

// SearchNearest @ 0x6baf4fbe (Tracking\HotZoneAdjust.c)
bool SearchNearest(byte param_1,undefined8 param_2,undefined1 param_3,undefined8 param_4);

// SearchNearestDisTwo @ 0x6baf458d (Tracking\HotZoneAdjust.c)
void SearchNearestDisTwo(undefined2 param_1,undefined2 param_2,longlong param_3,byte param_4, ushort *param_5,ushort *param_6,byte *param_7);

// SensorPitchSizeMap @ 0x6bb05fc0 (Common\MISC.c)
int SensorPitchSizeMap(int param_1,double *param_2,ushort param_3);

// SensorPitchSizeMapDim1 @ 0x6bb060b4 (Common\MISC.c)
int SensorPitchSizeMapDim1(int sensorCoord,ushort scale);

// SensorPitchSizeMapDim2 @ 0x6bb060e8 (Common\MISC.c)
int SensorPitchSizeMapDim2(int sensorCoord,ushort scale);

// Sensor_GetDistance @ 0x6ba66b17 (Detection\Sensor.c)
int Sensor_GetDistance(byte param_1,byte param_2,byte param_3,byte param_4);

// Sensor_GetPosType @ 0x6ba67339 (Detection\Sensor.c)
byte Sensor_GetPosType(byte param_1,byte param_2);

// Sensor_GetPosType2 @ 0x6ba67374 (Detection\Sensor.c)
uint Sensor_GetPosType2(ushort param_1);

// Sensor_GetPosType3 @ 0x6ba673c2 (Detection\Sensor.c)
uint Sensor_GetPosType3(byte param_1,byte param_2);

// Sensor_GetSquareDistance @ 0x6ba66b55 (Detection\Sensor.c)
int Sensor_GetSquareDistance(byte param_1,byte param_2,byte param_3,byte param_4);

// Sensor_GetZoneIndex @ 0x6ba672a6 (Detection\Sensor.c)
byte Sensor_GetZoneIndex(byte param_1,byte param_2);

// Sensor_GetZoneIndex2 @ 0x6ba672e1 (Detection\Sensor.c)
undefined1 Sensor_GetZoneIndex2(ushort param_1);

// Sensor_IsInCoreZone @ 0x6ba67540 (Detection\Sensor.c)
undefined8 Sensor_IsInCoreZone(byte param_1,byte param_2,byte param_3);

// Sensor_IsInDifferentTouchZone @ 0x6ba67640 (Detection\Sensor.c)
undefined1 Sensor_IsInDifferentTouchZone (undefined1 param_1,undefined1 param_2,undefined1 param_3,undefined1 param_4);

// Sensor_IsInNxNBox @ 0x6ba67234 (Detection\Sensor.c)
undefined8 Sensor_IsInNxNBox(byte param_1,byte param_2,byte param_3,byte param_4,byte param_5);

// Sensor_IsInSideZone @ 0x6ba675c0 (Detection\Sensor.c)
undefined8 Sensor_IsInSideZone(byte param_1,byte param_2,byte param_3);

// Sensor_IsInZone @ 0x6ba674b8 (Detection\Sensor.c)
undefined8 Sensor_IsInZone(byte param_1,byte param_2,byte param_3);

// Sensor_IsTouchTraversed @ 0x6ba67472 (Detection\Sensor.c)
undefined8 Sensor_IsTouchTraversed(undefined1 param_1,undefined1 param_2);

// Sensor_IsTouchTraversedBefore @ 0x6ba6742c (Detection\Sensor.c)
undefined8 Sensor_IsTouchTraversedBefore(undefined1 param_1,undefined1 param_2);

// SigSumFilter_ReserveTouch @ 0x6ba60977 (Signal\SigSumFilter.c)
void SigSumFilter_ReserveTouch(int param_1);

// SinXDivX @ 0x6ba8573e (Common\MISC.c)
double SinXDivX(double param_1);

// StylusAft_BtStatusPenAttach @ 0x6bb20932 (Rejection\StylusAft.c)
bool StylusAft_BtStatusPenAttach(void);

// StylusAft_BtStatusPenHibernate @ 0x6bb2098f (Rejection\StylusAft.c)
undefined8 StylusAft_BtStatusPenHibernate(void);

// StylusAft_CurrentDomainJudge @ 0x6bb21047 (Rejection\StylusAft.c)
undefined8 StylusAft_CurrentDomainJudge(byte param_1);

// StylusAft_DistBetweenPoints @ 0x6bb20d54 (Rejection\StylusAft.c)
int StylusAft_DistBetweenPoints(int param_1,int param_2,int param_3,int param_4);

// StylusAft_EarlyPalmJudge @ 0x6bb2145c (Rejection\StylusAft.c)
undefined8 StylusAft_EarlyPalmJudge(byte param_1);

// StylusAft_EdgePalmJudge @ 0x6bb214eb (Rejection\StylusAft.c)
undefined8 StylusAft_EdgePalmJudge(byte param_1);

// StylusAft_FanAreaJudge @ 0x6bb20f58 (Rejection\StylusAft.c)
undefined8 StylusAft_FanAreaJudge(byte param_1);

// StylusAft_GetPalmLevel @ 0x6bb209fb (Rejection\StylusAft.c)
undefined8 StylusAft_GetPalmLevel(byte param_1);

// StylusAft_InEdge @ 0x6bb21cc0 (Rejection\StylusAft.c)
undefined1 StylusAft_InEdge(undefined1 param_1);

// StylusAft_InFanArea @ 0x6bb20d9b (Rejection\StylusAft.c)
undefined8 StylusAft_InFanArea(undefined4 param_1,undefined4 param_2);

// StylusAft_Init @ 0x6bb208d0 (Rejection\StylusAft.c)
void StylusAft_Init(void);

// StylusAft_PalmJudge @ 0x6bb20eb3 (Rejection\StylusAft.c)
undefined8 StylusAft_PalmJudge(undefined1 param_1);

// StylusAft_PalmLocationJudge @ 0x6bb20c5e (Rejection\StylusAft.c)
undefined8 StylusAft_PalmLocationJudge(undefined1 param_1,byte param_2);

// StylusAft_PostDomainJudge @ 0x6bb21364 (Rejection\StylusAft.c)
undefined8 StylusAft_PostDomainJudge(byte param_1);

// StylusAft_PostProcessUnit @ 0x6bb223ef (Rejection\StylusAft.c)
void StylusAft_PostProcessUnit(byte param_1);

// StylusAft_ProcessSuppress @ 0x6bb21ac8 (Rejection\StylusAft.c)
void StylusAft_ProcessSuppress(byte param_1,byte param_2);

// StylusAft_ProcessWithdraw @ 0x6bb216da (Rejection\StylusAft.c)
void StylusAft_ProcessWithdraw(byte param_1,byte param_2,undefined1 *param_3);

// StylusAft_Sqrt @ 0x6bb20d02 (Rejection\StylusAft.c)
int StylusAft_Sqrt(uint param_1);

// StylusAft_SuppressBaseContext @ 0x6bb21dfd (Rejection\StylusAft.c)
void StylusAft_SuppressBaseContext(byte param_1,char param_2);

// SubTZ_Centroid @ 0x6ba7d960 (Detection\SubTZ.c)
void SubTZ_Centroid(longlong param_1);

// SubTZ_Filter @ 0x6ba7da00 (Detection\SubTZ.c)
void SubTZ_Filter(longlong param_1);

// SubTZ_GetAxis @ 0x6ba7f1cc (Detection\SubTZ.c)
void SubTZ_GetAxis(longlong param_1);

// SubTZ_GetNumAftFilter @ 0x6ba7ec0c (Detection\SubTZ.c)
undefined1 SubTZ_GetNumAftFilter(void);

// SubTZ_GripBasedTraversal @ 0x6ba7e51f (Detection\SubTZ.c)
void SubTZ_GripBasedTraversal (undefined8 param_1,undefined8 param_2,undefined1 param_3,byte param_4,ushort param_5 );

// SubTZ_IsGrip @ 0x6ba7d9b0 (Detection\SubTZ.c)
undefined8 SubTZ_IsGrip(longlong param_1);

// SubTZ_IsInCurrTZ @ 0x6ba7d35d (Detection\SubTZ.c)
undefined1 SubTZ_IsInCurrTZ(longlong param_1,ushort param_2);

// SubTZ_IsMF @ 0x6ba7ec1d (Detection\SubTZ.c)
bool SubTZ_IsMF(void);

// SubTZ_PeakBasedTraversal @ 0x6ba7d838 (Detection\SubTZ.c)
void SubTZ_PeakBasedTraversal (undefined8 param_1,undefined8 param_2,short param_3,byte param_4,ushort param_5);

// SubTZ_PeakInfoRetrieval @ 0x6ba7dcec (Detection\SubTZ.c)
void SubTZ_PeakInfoRetrieval(longlong param_1,uint *param_2,byte *param_3,char *param_4);

// SubTZ_Process @ 0x6ba7e646 (Detection\SubTZ.c)
void SubTZ_Process(longlong param_1);

// SubTZ_SetExtraFlag @ 0x6ba7d4fe (Detection\SubTZ.c)
void SubTZ_SetExtraFlag(ushort param_1,short param_2,longlong param_3);

// SubTZ_TraversalGripNeighbors @ 0x6ba7e28f (Detection\SubTZ.c)
void SubTZ_TraversalGripNeighbors (longlong param_1,short *param_2,char param_3,byte param_4,byte param_5);

// SubTZ_TraversalNeighbors @ 0x6ba7d5a5 (Detection\SubTZ.c)
void SubTZ_TraversalNeighbors (longlong param_1,short *param_2,short param_3,char param_4,char param_5);

// SubTZ_UnitInit @ 0x6ba7d2e0 (Detection\SubTZ.c)
void SubTZ_UnitInit(undefined2 *param_1);

// SubTZ_UpdateUnit @ 0x6ba7d3cd (Detection\SubTZ.c)
void SubTZ_UpdateUnit(longlong param_1,short param_2,byte param_3,byte param_4);

// SubTz_GetSignalThresh @ 0x6ba7de91 (Detection\SubTZ.c)
short SubTz_GetSignalThresh(short *param_1,undefined8 *param_2);

// TE_AssertProcess @ 0x6ba7872d (Tracking\TE.c)
undefined1 TE_AssertProcess(void);

// TE_GetNextID @ 0x6ba77536 (Tracking\TE.c)
byte TE_GetNextID(void);

// TE_Init @ 0x6ba77530 (Tracking\TE.c)
void TE_Init(void);

// TE_LiftOffProcess @ 0x6ba78511 (Tracking\TE.c)
void TE_LiftOffProcess(byte param_1);

// TE_LiftOffReportProcess @ 0x6ba77d4b (Tracking\TE.c)
byte TE_LiftOffReportProcess(byte param_1);

// TE_MoveProcess @ 0x6ba775fa (Tracking\TE.c)
void TE_MoveProcess(byte param_1,byte param_2);

// TE_Process @ 0x6ba789d9 (Tracking\TE.c)
void TE_Process(void);

// TE_TouchDownDebounce @ 0x6ba7772b (Tracking\TE.c)
void TE_TouchDownDebounce(byte param_1,byte param_2);

// TE_TouchDownProcess @ 0x6ba779f3 (Tracking\TE.c)
void TE_TouchDownProcess(byte param_1);

// TE_WindowsLiftOffProcess @ 0x6ba78033 (Tracking\TE.c)
void TE_WindowsLiftOffProcess(byte param_1);

// TPSensor_AddToBrokenSensorList @ 0x6ba6babf (Signal\TPSensor.c)
bool TPSensor_AddToBrokenSensorList(undefined1 param_1,undefined1 param_2,undefined1 param_3);

// TPSensor_CompensateBrokenSensor @ 0x6ba6c403 (Signal\TPSensor.c)
void TPSensor_CompensateBrokenSensor(void);

// TPSensor_Free @ 0x6ba6ba80 (Signal\TPSensor.c)
void TPSensor_Free(void);

// TPSensor_Init @ 0x6ba6be2e (Signal\TPSensor.c)
void TPSensor_Init(void);

// TPSensor_IsBrokenSensorInTouchZone @ 0x6ba6c46f (Signal\TPSensor.c)
void TPSensor_IsBrokenSensorInTouchZone(longlong param_1);

// TPSensor_IsCompensationNeeded @ 0x6ba6c3bf (Signal\TPSensor.c)
undefined8 TPSensor_IsCompensationNeeded(void);

// TPSensor_IsNoMoveInCorrection @ 0x6ba6c8ef (Signal\TPSensor.c)
char TPSensor_IsNoMoveInCorrection(undefined4 param_1,undefined4 param_2);

// TPSensor_IsSensorButNotNotch @ 0x6ba6bd35 (Signal\TPSensor.c)
undefined8 TPSensor_IsSensorButNotNotch(int param_1,int param_2);

// TPSensor_IsSensorReusedNotch @ 0x6ba6bca9 (Signal\TPSensor.c)
undefined8 TPSensor_IsSensorReusedNotch(int param_1,int param_2);

// TPSensor_IsSensorValid @ 0x6ba6bdc1 (Signal\TPSensor.c)
bool TPSensor_IsSensorValid(int param_1,int param_2);

// TPSensor_Malloc @ 0x6ba6b9b0 (Signal\TPSensor.c)
undefined8 TPSensor_Malloc(void);

// TPSensor_Process @ 0x6ba6c929 (Signal\TPSensor.c)
void TPSensor_Process(void);

// TPSensor_ProcessRaw @ 0x6ba6c910 (Signal\TPSensor.c)
void TPSensor_ProcessRaw(void);

// TPSensor_ProcessSensor @ 0x6ba6c0e5 (Signal\TPSensor.c)
void TPSensor_ProcessSensor(byte *param_1);

// TPSensor_Reset @ 0x6ba6bb6b (Signal\TPSensor.c)
void TPSensor_Reset(void);

// TPSensor_RevertScalingDif @ 0x6ba6c635 (Signal\TPSensor.c)
ulonglong TPSensor_RevertScalingDif(ushort param_1,byte param_2,byte param_3);

// TPSensor_RevertTranslateRaw @ 0x6ba6c7c5 (Signal\TPSensor.c)
void TPSensor_RevertTranslateRaw(longlong param_1,uint param_2);

// TPSensor_ScalingDif @ 0x6ba6c52c (Signal\TPSensor.c)
void TPSensor_ScalingDif(void);

// TPSensor_TranslateRaw @ 0x6ba6c6c7 (Signal\TPSensor.c)
void TPSensor_TranslateRaw(void);

// TPSensor_UpdateBrokenSensorTable @ 0x6ba6bb78 (Signal\TPSensor.c)
void TPSensor_UpdateBrokenSensorTable(void);

// TSABuffer_Free @ 0x6ba8df9f (Core\TSABuffer.c)
void TSABuffer_Free(void);

// TSABuffer_GetNextTime @ 0x6ba8e95e (Core\TSABuffer.c)
int TSABuffer_GetNextTime(int param_1);

// TSABuffer_GripStageProcess @ 0x6ba8ef57 (Core\TSABuffer.c)
void TSABuffer_GripStageProcess(void);

// TSABuffer_GripTimeProcess @ 0x6ba8eea1 (Core\TSABuffer.c)
void TSABuffer_GripTimeProcess(void);

// TSABuffer_Init @ 0x6ba8e183 (Core\TSABuffer.c)
void TSABuffer_Init(void);

// TSABuffer_Malloc @ 0x6ba8d7f6 (Core\TSABuffer.c)
undefined8 TSABuffer_Malloc(void);

// TSABuffer_PostProcess @ 0x6ba8f4c1 (Core\TSABuffer.c)
void TSABuffer_PostProcess(void);

// TSABuffer_PreProcess @ 0x6ba8f37f (Core\TSABuffer.c)
void TSABuffer_PreProcess(void);

// TSABuffer_PrevFrameProcess @ 0x6ba8f540 (Core\TSABuffer.c)
void TSABuffer_PrevFrameProcess(void);

// TSABuffer_TZIDProcess @ 0x6ba8e7ac (Core\TSABuffer.c)
void TSABuffer_TZIDProcess(void);

// TSABuffer_TimeRptProcess @ 0x6ba8e9b0 (Core\TSABuffer.c)
void TSABuffer_TimeRptProcess(void);

// TSABuffer_TzIsMergedHandle @ 0x6ba8ec68 (Core\TSABuffer.c)
void TSABuffer_TzIsMergedHandle(uint param_1);

// TSABuffer_TzIsNotMergedHandle @ 0x6ba8ed8c (Core\TSABuffer.c)
void TSABuffer_TzIsNotMergedHandle(uint param_1);

// TSAConstPrmt_Init @ 0x6bb3bb00 (Prmt\TSAConstPrmt.c)
void TSAConstPrmt_Init(void);

// TSAConstPrmt_Process @ 0x6bb3c698 (Prmt\TSAConstPrmt.c)
void TSAConstPrmt_Process(void);

// TSADynPrmt_CoreProcess @ 0x6bb3d114 (Prmt\TSADynPrmt.c)
void TSADynPrmt_CoreProcess(void);

// TSADynPrmt_GetBL @ 0x6bb3cb4f (Prmt\TSADynPrmt.c)
void TSADynPrmt_GetBL(void);

// TSADynPrmt_GetCMFCoef @ 0x6bb3d0af (Prmt\TSADynPrmt.c)
void TSADynPrmt_GetCMFCoef(void);

// TSADynPrmt_GetHTI @ 0x6bb3cd7f (Prmt\TSADynPrmt.c)
void TSADynPrmt_GetHTI(void);

// TSADynPrmt_GetLTI @ 0x6bb3cd85 (Prmt\TSADynPrmt.c)
void TSADynPrmt_GetLTI(void);

// TSADynPrmt_GetMFLTI @ 0x6bb3cd8b (Prmt\TSADynPrmt.c)
void TSADynPrmt_GetMFLTI(void);

// TSADynPrmt_GetPkThold @ 0x6bb3cd91 (Prmt\TSADynPrmt.c)
void TSADynPrmt_GetPkThold(void);

// TSADynPrmt_GetSig @ 0x6bb3d07d (Prmt\TSADynPrmt.c)
void TSADynPrmt_GetSig(void);

// TSADynPrmt_GetTZThold @ 0x6bb3d018 (Prmt\TSADynPrmt.c)
void TSADynPrmt_GetTZThold(void);

// TSADynPrmt_GetUnderWaterThold @ 0x6bb3d023 (Prmt\TSADynPrmt.c)
void TSADynPrmt_GetUnderWaterThold(void);

// TSADynPrmt_Init @ 0x6bb3c6f2 (Prmt\TSADynPrmt.c)
void TSADynPrmt_Init(void);

// TSADynPrmt_PreProcess @ 0x6bb3d1e7 (Prmt\TSADynPrmt.c)
void TSADynPrmt_PreProcess(void);

// TSADynPrmt_Process @ 0x6bb3d12d (Prmt\TSADynPrmt.c)
void TSADynPrmt_Process(void);

// TSADynPrmt_SSInit @ 0x6bb3c6a0 (Prmt\TSADynPrmt.c)
void TSADynPrmt_SSInit(void);

// TSADynPrmt_SideProcess @ 0x6bb3cf6e (Prmt\TSADynPrmt.c)
void TSADynPrmt_SideProcess(short param_1);

// TSADynPrmt_UpdateBfrTch @ 0x6bb3c960 (Prmt\TSADynPrmt.c)
void TSADynPrmt_UpdateBfrTch(short param_1,short param_2);

// TSADynPrmt_UpdateNoiseTholdBfrTch @ 0x6bb3c849 (Prmt\TSADynPrmt.c)
void TSADynPrmt_UpdateNoiseTholdBfrTch(short param_1,short param_2);

// TSADynPrmt_UpdateStepAftTch @ 0x6bb3ca49 (Prmt\TSADynPrmt.c)
void TSADynPrmt_UpdateStepAftTch(short param_1,short param_2);

// TSAMS_GetDifPtr @ 0x6ba91631 (Core\TSAMS.c)
undefined8 TSAMS_GetDifPtr(void);

// TSAMS_IsOKToEnterIDLEMode @ 0x6ba99ad6 (Core\TSAMS.c)
bool TSAMS_IsOKToEnterIDLEMode(void);

// TSAMS_IsReadyForIDLEMode @ 0x6ba9998b (Core\TSAMS.c)
bool TSAMS_IsReadyForIDLEMode(void);

// TSAMath_ATan2 @ 0x6bb429d4 (Common\TSAMath.c)
int TSAMath_ATan2(ushort param_1,ushort param_2);

// TSAMath_ArcTanOctant @ 0x6bb428a0 (Common\TSAMath.c)
int TSAMath_ArcTanOctant(int param_1,int param_2);

// TSAMem_Free @ 0x6baa0608 (Core\TSABuffer.c)
void TSAMem_Free(void);

// TSAMem_InitProject @ 0x6baa065d (Core\TSABuffer.c)
bool TSAMem_InitProject(void);

// TSAMem_Malloc @ 0x6baa04f0 (Core\TSABuffer.c)
undefined8 TSAMem_Malloc(void);

// TSAOut_Angle @ 0x6ba6a305 (Core\TSAOut.c)
char TSAOut_Angle(char param_1);

// TSAOut_Axis @ 0x6ba6a2e5 (Core\TSAOut.c)
ushort TSAOut_Axis(short param_1);

// TSAOut_Dim1PosInternal @ 0x6ba69a50 (Core\TSAOut.c)
undefined2 TSAOut_Dim1PosInternal(ushort param_1,ushort param_2);

// TSAOut_Dim2PosInternal @ 0x6ba69ccb (Core\TSAOut.c)
undefined2 TSAOut_Dim2PosInternal(ushort param_1,ushort param_2);

// TSAOut_Display2Pixel @ 0x6ba69c56 (Core\TSAOut.c)
int TSAOut_Display2Pixel(int param_1);

// TSAOut_DisplayXPos @ 0x6ba6a241 (Core\TSAOut.c)
ushort TSAOut_DisplayXPos(undefined1 param_1, ...);

// TSAOut_DisplayYPos @ 0x6ba6a293 (Core\TSAOut.c)
ushort TSAOut_DisplayYPos(undefined1 param_1, ...);

// TSAOut_EdgeScale @ 0x6ba69c9e (Core\TSAOut.c)
int TSAOut_EdgeScale(int param_1,int param_2);

// TSAOut_ExtToIntDist @ 0x6ba69bc3 (Core\TSAOut.c)
ulonglong TSAOut_ExtToIntDist(ushort param_1);

// TSAOut_IntToExtDist @ 0x6ba69bfb (Core\TSAOut.c)
uint TSAOut_IntToExtDist(short param_1);

// TSAOut_Pixel2DisplayDim1 @ 0x6ba69b0b (Core\TSAOut.c)
ushort TSAOut_Pixel2DisplayDim1(undefined2 param_1);

// TSAOut_Pixel2DisplayDim2 @ 0x6ba69e95 (Core\TSAOut.c)
ushort TSAOut_Pixel2DisplayDim2(undefined2 param_1);

// TSAOut_PostProcess @ 0x6ba6a77a (Core\TSAOut.c)
void TSAOut_PostProcess(void);

// TSAOut_PostProcessBefUDFPUnit @ 0x6ba6a6f3 (Core\TSAOut.c)
void TSAOut_PostProcessBefUDFPUnit(char param_1);

// TSAOut_PostProcessUnit @ 0x6ba6a3ef (Core\TSAOut.c)
void TSAOut_PostProcessUnit(char param_1,char *param_2);

// TSAOut_ReportExtInfo @ 0x6ba6a6cd (Core\TSAOut.c)
void TSAOut_ReportExtInfo(void);

// TSAOut_RevertDim1Pos @ 0x6ba69b2f (Core\TSAOut.c)
uint TSAOut_RevertDim1Pos(ushort param_1);

// TSAOut_RevertDim2Pos @ 0x6ba69eb9 (Core\TSAOut.c)
uint TSAOut_RevertDim2Pos(ushort param_1);

// TSAOut_RptXPos @ 0x6ba6a264 (Core\TSAOut.c)
ushort TSAOut_RptXPos(undefined1 param_1);

// TSAOut_RptYPos @ 0x6ba6a2b6 (Core\TSAOut.c)
ushort TSAOut_RptYPos(undefined1 param_1);

// TSAOut_TouchXPosInternal @ 0x6ba69f4d (Core\TSAOut.c)
ulonglong TSAOut_TouchXPosInternal(byte param_1,short param_2);

// TSAOut_TouchYPosInternal @ 0x6ba6a0c7 (Core\TSAOut.c)
ulonglong TSAOut_TouchYPosInternal(byte param_1,short param_2);

// TSAPrmt_FlashPrmtLoadProject @ 0x6bb3b759 (Prmt\TSAPrmt.c)
undefined8 TSAPrmt_FlashPrmtLoadProject(undefined8 param_1);

// TSAPrmt_GetProjectName @ 0x6bb3b9df (Prmt\TSAPrmt.c)
TSAFlashPrmt * TSAPrmt_GetProjectName(void);

// TSAPrmt_Init @ 0x6bb3b90d (Prmt\TSAPrmt.c)
bool TSAPrmt_Init(void);

// TSAPrmt_LoadProject @ 0x6bb3b93a (Prmt\TSAPrmt.c)
undefined8 TSAPrmt_LoadProject(longlong param_1);

// TSAPrmt_PreProcess @ 0x6bb3bab7 (Prmt\TSAPrmt.c)
void TSAPrmt_PreProcess(void);

// TSAPrmt_Process @ 0x6bb3bacb (Prmt\TSAPrmt.c)
void TSAPrmt_Process(void);

// TSAPrmt_ReInit @ 0x6bb3b8e5 (Prmt\TSAPrmt.c)
void TSAPrmt_ReInit(void);

// TSAPrmt_Reset @ 0x6bb3b740 (Prmt\TSAPrmt.c)
void TSAPrmt_Reset(void);

// TSAPrmt_Validate @ 0x6bb3b9ec (Prmt\TSAPrmt.c)
void TSAPrmt_Validate(void);

// TSAPrpt_Free @ 0x6baa07ad (Core\TSAPrpt.c)
void TSAPrpt_Free(void);

// TSAPrpt_GetDifPreCMFPrpt @ 0x6baa13be (Core\TSAPrpt.c)
void TSAPrpt_GetDifPreCMFPrpt(void);

// TSAPrpt_Init @ 0x6baa07c8 (Core\TSAPrpt.c)
void TSAPrpt_Init(void);

// TSAPrpt_Malloc @ 0x6baa076c (Core\TSAPrpt.c)
bool TSAPrpt_Malloc(void);

// TSARTPrmt_CopyMember @ 0x6bb3dd22 (Prmt\TSARTPrmt.c)
void TSARTPrmt_CopyMember(undefined4 *param_1,undefined4 *param_2);

// TSARTPrmt_GetAddrOfIndexedKey @ 0x6bb3ddbf (Prmt\TSARTPrmt.c)
longlong TSARTPrmt_GetAddrOfIndexedKey(byte param_1);

// TSARTPrmt_GetKeyIndex @ 0x6bb3dd3c (Prmt\TSARTPrmt.c)
bool TSARTPrmt_GetKeyIndex(char *param_1,byte *param_2);

// TSARTPrmt_Init @ 0x6bb3d3e0 (Prmt\TSARTPrmt.c)
void TSARTPrmt_Init(void);

// TSARTPrmt_Process @ 0x6bb3d933 (Prmt\TSARTPrmt.c)
void TSARTPrmt_Process(void);

// TSARTPrmt_Reset @ 0x6bb3d8fe (Prmt\TSARTPrmt.c)
void TSARTPrmt_Reset(void);

// TSARTPrmt_SetKeyValue @ 0x6bb3de92 (Prmt\TSARTPrmt.c)
void TSARTPrmt_SetKeyValue(undefined8 param_1,undefined4 param_2);

// TSARTPrmt_SetKeyValueWithReturn @ 0x6bb3dde9 (Prmt\TSARTPrmt.c)
undefined8 TSARTPrmt_SetKeyValueWithReturn(longlong param_1,undefined4 param_2);

// TSARTPrmt_SyncWithTTPrmt @ 0x6bb3d210 (Prmt\TSARTPrmt.c)
void TSARTPrmt_SyncWithTTPrmt(void);

// TSARamPrmt_Init @ 0x6bb3d200 (Prmt\TSARamPrmt.c)
void TSARamPrmt_Init(void);

// TSARamPrmt_Process @ 0x6bb3d206 (Prmt\TSARamPrmt.c)
void TSARamPrmt_Process(void);

// TSAStatic_DoesAFERequireToExitIDLEMode @ 0x6baa28bc (Core\TSAStatic.c)
undefined1 TSAStatic_DoesAFERequireToExitIDLEMode(void);

// TSAStatic_DoesAFERequireToExitSideIDLEMode @ 0x6baa298f (Core\TSAStatic.c)
undefined1 TSAStatic_DoesAFERequireToExitSideIDLEMode(void);

// TSAStatic_GetCurrTimeStamp @ 0x6baa2cfe (Core\TSAStatic.c)
void * TSAStatic_GetCurrTimeStamp(void);

// TSAStatic_GetMSTimeStamp @ 0x6baa2d60 (Core\TSAStatic.c)
void * TSAStatic_GetMSTimeStamp(void);

// TSAStatic_GetSigThold @ 0x6baa2f14 (Core\TSAStatic.c)
uint TSAStatic_GetSigThold(int param_1);

// TSAStatic_HTIFilter @ 0x6baa2f56 (Core\TSAStatic.c)
void TSAStatic_HTIFilter(byte param_1);

// TSAStatic_HTIUpdate @ 0x6baa3091 (Core\TSAStatic.c)
void TSAStatic_HTIUpdate(void);

// TSAStatic_Init @ 0x6baa1a42 (Core\TSAStatic.c)
void TSAStatic_Init(void);

// TSAStatic_InitInternalModuleEveryTime @ 0x6baa18b1 (Core\TSAStatic.c)
void TSAStatic_InitInternalModuleEveryTime(void);

// TSAStatic_InitSysModuleEveryTime @ 0x6baa15c6 (Core\TSAStatic.c)
void TSAStatic_InitSysModuleEveryTime(void);

// TSAStatic_IsAFEEnterIdleTimeout @ 0x6baa2742 (Core\TSAStatic.c)
undefined8 TSAStatic_IsAFEEnterIdleTimeout(void);

// TSAStatic_IsAFEEnterSideIdleTimeout @ 0x6baa27ff (Core\TSAStatic.c)
undefined8 TSAStatic_IsAFEEnterSideIdleTimeout(void);

// TSAStatic_IsAFEJustEnterIDLEMode @ 0x6baa26ce (Core\TSAStatic.c)
undefined1 TSAStatic_IsAFEJustEnterIDLEMode(void);

// TSAStatic_IsAFEJustExitIDLEMode @ 0x6baa265a (Core\TSAStatic.c)
undefined1 TSAStatic_IsAFEJustExitIDLEMode(void);

// TSAStatic_IsAFEJustExitSideIDLEMode @ 0x6baa2694 (Core\TSAStatic.c)
undefined1 TSAStatic_IsAFEJustExitSideIDLEMode(void);

// TSAStatic_IsAFTModeEnabled @ 0x6baa2cda (Core\TSAStatic.c)
undefined4 TSAStatic_IsAFTModeEnabled(void);

// TSAStatic_IsAllFreqBecomeNoisy @ 0x6baa260a (Core\TSAStatic.c)
undefined1 TSAStatic_IsAllFreqBecomeNoisy(void);

// TSAStatic_IsAllFreqNoisy @ 0x6baa2590 (Core\TSAStatic.c)
undefined1 TSAStatic_IsAllFreqNoisy(void);

// TSAStatic_IsChargerPlugged @ 0x6baa2434 (Core\TSAStatic.c)
undefined1 TSAStatic_IsChargerPlugged(void);

// TSAStatic_IsChargerPluggedNoDebounce @ 0x6baa24b0 (Core\TSAStatic.c)
bool TSAStatic_IsChargerPluggedNoDebounce(void);

// TSAStatic_IsFreqShiftDone @ 0x6baa23cd (Core\TSAStatic.c)
bool TSAStatic_IsFreqShiftDone(void);

// TSAStatic_IsHWReset @ 0x6baa2345 (Core\TSAStatic.c)
bool TSAStatic_IsHWReset(void);

// TSAStatic_IsHWResetPrev @ 0x6baa2373 (Core\TSAStatic.c)
bool TSAStatic_IsHWResetPrev(void);

// TSAStatic_IsJustOKToEnterIDLEMode @ 0x6baa2a22 (Core\TSAStatic.c)
undefined1 TSAStatic_IsJustOKToEnterIDLEMode(void);

// TSAStatic_IsJustOKToEnterSideIDLEMode @ 0x6baa2a5c (Core\TSAStatic.c)
undefined1 TSAStatic_IsJustOKToEnterSideIDLEMode(void);

// TSAStatic_IsLargeDisplayNoise @ 0x6baa24f9 (Core\TSAStatic.c)
undefined8 TSAStatic_IsLargeDisplayNoise(void);

// TSAStatic_IsOKToEnterIDLEMode @ 0x6baa2a96 (Core\TSAStatic.c)
undefined1 TSAStatic_IsOKToEnterIDLEMode(void);

// TSAStatic_IsOKToEnterSideIDLEMode @ 0x6baa2b49 (Core\TSAStatic.c)
undefined1 TSAStatic_IsOKToEnterSideIDLEMode(void);

// TSAStatic_IsProximityEnabled @ 0x6baa2c95 (Core\TSAStatic.c)
undefined8 TSAStatic_IsProximityEnabled(void);

// TSAStatic_IsSafeBaselineApplied @ 0x6baa30f3 (Core\TSAStatic.c)
undefined8 TSAStatic_IsSafeBaselineApplied(void);

// TSAStatic_IsScreenRotated @ 0x6baa2e39 (Core\TSAStatic.c)
undefined8 TSAStatic_IsScreenRotated(void);

// TSAStatic_IsSignalReverted @ 0x6baa2c81 (Core\TSAStatic.c)
uchar TSAStatic_IsSignalReverted(void);

// TSAStatic_PhoneCallModeDebounce @ 0x6baa20fe (Core\TSAStatic.c)
void TSAStatic_PhoneCallModeDebounce(void);

// TSAStatic_PostProcess @ 0x6baa1f51 (Core\TSAStatic.c)
void TSAStatic_PostProcess(void);

// TSAStatic_PreProcess @ 0x6baa1bb0 (Core\TSAStatic.c)
void TSAStatic_PreProcess(void);

// TSAStatic_ProcessAftTouchEvent @ 0x6baa316a (Core\TSAStatic.c)
void TSAStatic_ProcessAftTouchEvent(void);

// TSAStatic_RestoreExtFeatures @ 0x6baa2d74 (Core\TSAStatic.c)
void TSAStatic_RestoreExtFeatures(TSA_FeatureFlags param_1);

// TSAStatic_SafeDiffCheck @ 0x6baa1de6 (Core\TSAStatic.c)
void TSAStatic_SafeDiffCheck(void);

// TSAStatic_SetLogName @ 0x6baa2e93 (Core\TSAStatic.c)
undefined8 TSAStatic_SetLogName(char *param_1);

// TSAStatic_UpdateCtrlFlags @ 0x6baa21cb (Core\TSAStatic.c)
void TSAStatic_UpdateCtrlFlags(TSA_SystemFlags param_1);

// TSAStatic_UpdateIDLEModeOutput @ 0x6baa2aa7 (Core\TSAStatic.c)
undefined1 TSAStatic_UpdateIDLEModeOutput(void);

// TSAStatic_UpdateSideIDLEModeOutput @ 0x6baa2b5a (Core\TSAStatic.c)
undefined1 TSAStatic_UpdateSideIDLEModeOutput(void);

// TSATTPrmt_Init @ 0x6bb3df90 (Prmt\TSATTPrmt.c)
void TSATTPrmt_Init(void);

// TSATTPrmt_Process @ 0x6bb41b54 (Prmt\TSATTPrmt.c)
void TSATTPrmt_Process(void);

// TSATTPrmt_UpdataFlashPrmtCmf @ 0x6bb410c9 (Prmt\TSATTPrmt.c)
void TSATTPrmt_UpdataFlashPrmtCmf(void);

// TSATTPrmt_UpdataFlashPrmtCtd @ 0x6bb41149 (Prmt\TSATTPrmt.c)
void TSATTPrmt_UpdataFlashPrmtCtd(void);

// TSATTPrmt_UpdataFlashPrmtHiException @ 0x6bb41109 (Prmt\TSATTPrmt.c)
void TSATTPrmt_UpdataFlashPrmtHiException(void);

// TSATTPrmt_UpdataFlashPrmtIdt @ 0x6bb410a6 (Prmt\TSATTPrmt.c)
void TSATTPrmt_UpdataFlashPrmtIdt(void);

// TSATTPrmt_UpdataFlashPrmtProx @ 0x6bb4124b (Prmt\TSATTPrmt.c)
void TSATTPrmt_UpdataFlashPrmtProx(void);

// TSATTPrmt_UpdataFlashPrmtSensorData @ 0x6bb41aa0 (Prmt\TSATTPrmt.c)
void TSATTPrmt_UpdataFlashPrmtSensorData(void);

// TSATTPrmt_UpdataFlashPrmtSideTouch @ 0x6bb413bd (Prmt\TSATTPrmt.c)
void TSATTPrmt_UpdataFlashPrmtSideTouch(void);

// TSATTPrmt_UpdataFlashPrmtSig @ 0x6bb40ec5 (Prmt\TSATTPrmt.c)
void TSATTPrmt_UpdataFlashPrmtSig(void);

// TSATTPrmt_UpdataFlashPrmtSys @ 0x6bb40fe1 (Prmt\TSATTPrmt.c)
void TSATTPrmt_UpdataFlashPrmtSys(void);

// TSATTPrmt_UpdataFlashPrmtTa @ 0x6bb40f51 (Prmt\TSATTPrmt.c)
void TSATTPrmt_UpdataFlashPrmtTa(void);

// TSATTPrmt_UpdataFlashPrmtTm @ 0x6bb40f18 (Prmt\TSATTPrmt.c)
void TSATTPrmt_UpdataFlashPrmtTm(void);

// TSATTPrmt_UpdataFlashPrmtTz @ 0x6bb41032 (Prmt\TSATTPrmt.c)
void TSATTPrmt_UpdataFlashPrmtTz(void);

// TSATTPrmt_UpdateFlashPrmtDisplay @ 0x6bb41ade (Prmt\TSATTPrmt.c)
void TSATTPrmt_UpdateFlashPrmtDisplay(void);

// TSA_ASAAFEStatus @ 0x6ba9c607 (Core\TSA.c)
undefined4 TSA_ASAAFEStatus(void);

// TSA_AppTypeToScenario @ 0x6ba90318 (Core\TSA.c)
void TSA_AppTypeToScenario(void);

// TSA_CoreTouchDim1Center @ 0x6ba8f893 (Core\TSA.c)
ushort TSA_CoreTouchDim1Center(byte param_1);

// TSA_CoreTouchDim2Center @ 0x6ba8f8c3 (Core\TSA.c)
ushort TSA_CoreTouchDim2Center(byte param_1);

// TSA_DelayToIdleModeHPPx @ 0x6ba9e3c6 (Core\TSA.c)
undefined8 TSA_DelayToIdleModeHPPx(void);

// TSA_DimEdgeToSide @ 0x6ba8f5e5 (Core\TSA.c)
undefined8 TSA_DimEdgeToSide(int param_1);

// TSA_DisableScenario @ 0x6ba8fa33 (Core\TSA.c)
void TSA_DisableScenario(uint param_1);

// TSA_EnableEdgePeakFilterWorkaround @ 0x6ba9f33f (Core\TSA.c)
undefined8 TSA_EnableEdgePeakFilterWorkaround(void);

// TSA_EnableGloveFeature @ 0x6ba9eacd (Core\TSA.c)
bool TSA_EnableGloveFeature(void);

// TSA_EnableGripFilterFeature @ 0x6ba9ed08 (Core\TSA.c)
undefined8 TSA_EnableGripFilterFeature(void);

// TSA_EnableScenario @ 0x6ba8fa07 (Core\TSA.c)
void TSA_EnableScenario(uint param_1);

// TSA_Free @ 0x6baa34c4 (Core\TSA.c)
void TSA_Free(longlong *param_1);

// TSA_GetAFEScanFreqIndex @ 0x6ba9f998 (Core\TSA.c)
undefined8 TSA_GetAFEScanFreqIndex(longlong param_1);

// TSA_GetAFEScanRateIndex @ 0x6ba9fa2d (Core\TSA.c)
undefined8 TSA_GetAFEScanRateIndex(longlong param_1);

// TSA_GetASADim1Res @ 0x6ba9d3e0 (Core\TSA.c)
ulonglong TSA_GetASADim1Res(void);

// TSA_GetASADim2Res @ 0x6ba9d41c (Core\TSA.c)
ulonglong TSA_GetASADim2Res(void);

// TSA_GetDifPrptCommon @ 0x6baa1035 (Core\TSA.c)
void TSA_GetDifPrptCommon(void);

// TSA_GetDim1Length @ 0x6ba9addb (Core\TSA.c)
uchar TSA_GetDim1Length(void);

// TSA_GetDim1Res @ 0x6ba9aecf (Core\TSA.c)
undefined2 TSA_GetDim1Res(void);

// TSA_GetDim2Length @ 0x6ba9adec (Core\TSA.c)
uchar TSA_GetDim2Length(void);

// TSA_GetDim2Res @ 0x6ba9aee3 (Core\TSA.c)
undefined2 TSA_GetDim2Res(void);

// TSA_GetDimEdge @ 0x6ba8f6f3 (Core\TSA.c)
undefined4 TSA_GetDimEdge(ushort param_1,ushort param_2);

// TSA_GetFeatures @ 0x6ba9f329 (Core\TSA.c)
TSA_FeatureFlags TSA_GetFeatures(void);

// TSA_GetPrmtName @ 0x6ba9e911 (Core\TSA.c)
char* TSA_GetPrmtName(void);

// TSA_GetScenarioInternal @ 0x6ba8fd76 (Core\TSA.c)
uint TSA_GetScenarioInternal(void);

// TSA_GetSensorData @ 0x6ba8f9be (Core\TSA.c)
longlong TSA_GetSensorData(void);

// TSA_GetSide @ 0x6ba8f7f7 (Core\TSA.c)
undefined4 TSA_GetSide(undefined2 param_1,undefined2 param_2);

// TSA_GetTouchSide @ 0x6ba8f82b (Core\TSA.c)
undefined4 TSA_GetTouchSide(byte param_1);

// TSA_GetXRes @ 0x6ba9aef7 (Core\TSA.c)
ushort TSA_GetXRes(void);

// TSA_GetYLength @ 0x6ba9ae31 (Core\TSA.c)
ushort TSA_GetYLength(void);

// TSA_GetYRes @ 0x6ba9af2b (Core\TSA.c)
ushort TSA_GetYRes(void);

// TSA_InStylusHPP2Mode @ 0x6ba9c70b (Core\TSA.c)
bool TSA_InStylusHPP2Mode(void);

// TSA_InStylusMode @ 0x6ba9c771 (Core\TSA.c)
undefined8 TSA_InStylusMode(void);

// TSA_InitAFEProject_UI @ 0x6ba99dc7 (Core\TSA.c)
undefined8 TSA_InitAFEProject_UI(longlong param_1,longlong param_2,longlong param_3);

// TSA_InitCommonUI @ 0x6ba99ce0 (Core\TSA.c)
void TSA_InitCommonUI(void);

// TSA_InitProject_UI @ 0x6ba99d4e (Core\TSA.c)
undefined8 TSA_InitProject_UI(undefined8 param_1);

// TSA_IsFlickCheckPassed @ 0x6ba9c0e8 (Core\TSA.c)
void TSA_IsFlickCheckPassed(void);

// TSA_IsGameScenario @ 0x6ba8fe7f (Core\TSA.c)
undefined8 TSA_IsGameScenario(void);

// TSA_IsGripOKToIDLE @ 0x6ba9e650 (Core\TSA.c)
char TSA_IsGripOKToIDLE(void);

// TSA_IsHighRateGameScenario @ 0x6ba8feab (Core\TSA.c)
bool TSA_IsHighRateGameScenario(void);

// TSA_IsHighSensitivityMode @ 0x6ba9f685 (Core\TSA.c)
undefined8 TSA_IsHighSensitivityMode(void);

// TSA_IsInProcess @ 0x6ba9f7d7 (Core\TSA.c)
uchar TSA_IsInProcess(void);

// TSA_IsJustEnterTypingScenario @ 0x6ba8fe53 (Core\TSA.c)
undefined8 TSA_IsJustEnterTypingScenario(void);

// TSA_IsNormalRateGameScenario @ 0x6ba8fee1 (Core\TSA.c)
bool TSA_IsNormalRateGameScenario(void);

// TSA_IsOKToEnterIDLEMode @ 0x6ba9e755 (Core\TSA.c)
undefined1 TSA_IsOKToEnterIDLEMode(void);

// TSA_IsOutOfWaterDetected @ 0x6ba9d7fd (Core\TSA.c)
uint TSA_IsOutOfWaterDetected(void);

// TSA_IsPadFeatureEnabled @ 0x6ba9eece (Core\TSA.c)
undefined4 TSA_IsPadFeatureEnabled(void);

// TSA_IsPhoneCallScenario @ 0x6ba8fb38 (Core\TSA.c)
undefined8 TSA_IsPhoneCallScenario(void);

// TSA_IsPossibleProximity @ 0x6ba8fc41 (Core\TSA.c)
bool TSA_IsPossibleProximity(void);

// TSA_IsPossibleUnderWater @ 0x6baa082b (Core\TSA.c)
bool TSA_IsPossibleUnderWater(void);

// TSA_IsReCalibDetected @ 0x6ba9e2d8 (Core\TSA.c)
bool TSA_IsReCalibDetected(void);

// TSA_IsReCalibNeeded @ 0x6ba9e32b (Core\TSA.c)
undefined1 TSA_IsReCalibNeeded(void);

// TSA_IsScreenLockPhoneCallScenario @ 0x6ba8fbfd (Core\TSA.c)
undefined8 TSA_IsScreenLockPhoneCallScenario(void);

// TSA_IsScreenLockScenario @ 0x6ba8fb7c (Core\TSA.c)
undefined8 TSA_IsScreenLockScenario(void);

// TSA_IsToBypassCurrentFrame @ 0x6ba9f720 (Core\TSA.c)
char TSA_IsToBypassCurrentFrame(void);

// TSA_IsTypingScenario @ 0x6ba8fe17 (Core\TSA.c)
bool TSA_IsTypingScenario(void);

// TSA_IsTypingScenarioPrev @ 0x6ba8fe35 (Core\TSA.c)
bool TSA_IsTypingScenarioPrev(void);

// TSA_IsUnderWaterDetected @ 0x6ba9d7e5 (Core\TSA.c)
uint TSA_IsUnderWaterDetected(void);

// TSA_IsVideoScenario @ 0x6ba8fdf7 (Core\TSA.c)
bool TSA_IsVideoScenario(void);

// TSA_IsWindowsPadFeatureEnabled @ 0x6ba9ef4b (Core\TSA.c)
undefined4 TSA_IsWindowsPadFeatureEnabled(void);

// TSA_MSInitCommon @ 0x6ba90830 (Core\TSA.c)
void TSA_MSInitCommon(char param_1);

// TSA_MSInitProject @ 0x6ba90994 (Core\TSA.c)
undefined8 TSA_MSInitProject(undefined8 param_1);

// TSA_MSPeakFilter @ 0x6ba90dbd (Core\TSA.c)
void TSA_MSPeakFilter(void);

// TSA_MSProcess @ 0x6ba90e43 (Core\TSA.c)
void TSA_MSProcess(short *pMutualRawData);

// TSA_MSProcessEnding @ 0x6ba90e1a (Core\TSA.c)
void TSA_MSProcessEnding(void);

// TSA_MSRawDirectionDectect @ 0x6ba90a3d (Core\TSA.c)
void TSA_MSRawDirectionDectect(void);

// TSA_MSReset @ 0x6ba90b16 (Core\TSA.c)
void TSA_MSReset(void);

// TSA_MSTouchPostFilter @ 0x6ba90e14 (Core\TSA.c)
void TSA_MSTouchPostFilter(void);

// TSA_MSTouchPreFilter @ 0x6ba90de8 (Core\TSA.c)
void TSA_MSTouchPreFilter(void);

// TSA_MallocAndInit @ 0x6baa3430 (Core\TSA.c)
void * TSA_MallocAndInit(uint param_1);

// TSA_PitchSizeRemapProcess @ 0x6bb0611c (Core\TSA.c)
void TSA_PitchSizeRemapProcess(void);

// TSA_Processing @ 0x6ba9a443 (Core\TSA.c)
void TSA_Processing(short *pMutualRawData,uint dwCtrlFlags,void *pFrameBuffer);

// TSA_ProcessingAFEFrame @ 0x6ba9a7a7 (Core\TSA.c)
int TSA_ProcessingAFEFrame(AFE_Frame *frame,uint ctrl,ulonglong data);

// TSA_RawCheckFrozen @ 0x6bafca3c (Core\TSA.c)
void TSA_RawCheckFrozen(void);

// TSA_RawCheckGetExceptionDescription @ 0x6bafcafe (Core\TSA.c)
char * TSA_RawCheckGetExceptionDescription(void);

// TSA_RawCheckProcess @ 0x6bafcb0b (Core\TSA.c)
void TSA_RawCheckProcess(void);

// TSA_RawIsFrozen @ 0x6bafca30 (Core\TSA.c)
undefined4 TSA_RawIsFrozen(void);

// TSA_RptASAPressure @ 0x6ba9cb4b (Core\TSA.c)
undefined8 TSA_RptASAPressure(void);

// TSA_RptASAXPos @ 0x6ba9c84f (Core\TSA.c)
undefined8 TSA_RptASAXPos(void);

// TSA_RptASAXTilt @ 0x6ba9cae5 (Core\TSA.c)
undefined8 TSA_RptASAXTilt();

// TSA_RptASAYPos @ 0x6ba9c882 (Core\TSA.c)
undefined8 TSA_RptASAYPos();

// TSA_RptASAYTilt @ 0x6ba9cb18 (Core\TSA.c)
undefined8 TSA_RptASAYTilt(void);

// TSA_RptDiffPtr @ 0x6ba9c5a8 (Core\TSA.c)
undefined8 TSA_RptDiffPtr(void);

// TSA_RptIsLiftOff @ 0x6ba9b1b4 (Core\TSA.c)
uint TSA_RptIsLiftOff(byte param_1);

// TSA_RptIsTouchDown @ 0x6ba9b112 (Core\TSA.c)
uint TSA_RptIsTouchDown(byte param_1);

// TSA_RptLONum @ 0x6ba9b0eb (Core\TSA.c)
undefined1 TSA_RptLONum(void);

// TSA_RptTouchAxisAngle @ 0x6ba9bef5 (Core\TSA.c)
ulonglong TSA_RptTouchAxisAngle(byte param_1);

// TSA_RptTouchAxisMajor @ 0x6ba9bca2 (Core\TSA.c)
ulonglong TSA_RptTouchAxisMajor(byte param_1);

// TSA_RptTouchDim1Center @ 0x6ba9b98a (Core\TSA.c)
ushort TSA_RptTouchDim1Center(byte param_1);

// TSA_RptTouchDim2Center @ 0x6ba9b9bd (Core\TSA.c)
ushort TSA_RptTouchDim2Center(byte param_1);

// TSA_RptTouchEvent @ 0x6ba9b3cc (Core\TSA.c)
undefined4 TSA_RptTouchEvent(byte param_1);

// TSA_RptTouchException @ 0x6ba9b3fa (Core\TSA.c)
undefined4 TSA_RptTouchException(byte param_1);

// TSA_RptTouchID @ 0x6ba9b256 (Core\TSA.c)
int TSA_RptTouchID(byte param_1);

// TSA_RptTouchNum @ 0x6ba9b09c (Core\TSA.c)
uint TSA_RptTouchNum(void);

// TSA_RptTouchPressure @ 0x6ba9bc34 (Core\TSA.c)
uint TSA_RptTouchPressure(byte param_1);

// TSA_RptTouchSuppressFlag @ 0x6ba9c4ba (Core\TSA.c)
undefined4 TSA_RptTouchSuppressFlag(byte param_1);

// TSA_RptTouchXCenter @ 0x6ba9b9f0 (Core\TSA.c)
ushort TSA_RptTouchXCenter(undefined1 param_1);

// TSA_RptTouchXPos @ 0x6ba9b428 (Core\TSA.c)
ulonglong TSA_RptTouchXPos(byte param_1);

// TSA_RptTouchXWidth @ 0x6ba9b4dc (Core\TSA.c)
byte TSA_RptTouchXWidth(byte param_1);

// TSA_RptTouchYCenter @ 0x6ba9ba35 (Core\TSA.c)
ushort TSA_RptTouchYCenter(undefined1 param_1);

// TSA_RptTouchYGripCtd @ 0x6ba9b8a3 (Core\TSA.c)
undefined1 TSA_RptTouchYGripCtd(byte param_1);

// TSA_RptTouchYPos @ 0x6ba9b482 (Core\TSA.c)
ulonglong TSA_RptTouchYPos(byte param_1);

// TSA_SavePreUnifiedRaw @ 0x6ba9a32b (Core\TSA.c)
void TSA_SavePreUnifiedRaw(void *param_1,uint param_2);

// TSA_ScenarioGamePrmtSetting @ 0x6ba8ffe1 (Core\TSA.c)
void TSA_ScenarioGamePrmtSetting(void);

// TSA_ScenarioNormalRateGamePrmtSetting @ 0x6ba90092 (Core\TSA.c)
void TSA_ScenarioNormalRateGamePrmtSetting(void);

// TSA_ScenarioProcess @ 0x6ba904ed (Core\TSA.c)
void TSA_ScenarioProcess(void);

// TSA_ScenarioTypingPrmtSetting @ 0x6ba8ff4d (Core\TSA.c)
void TSA_ScenarioTypingPrmtSetting(void);

// TSA_SetAFECaps @ 0x6ba9f93c (Core\TSA.c)
bool TSA_SetAFECaps(longlong param_1);

// TSA_SetAFEInfo @ 0x6ba9f90e (Core\TSA.c)
bool TSA_SetAFEInfo(longlong param_1);

// TSA_SetDMDType @ 0x6ba9d881 (Core\TSA.c)
int TSA_SetDMDType(void);

// TSA_SetDim1Res @ 0x6ba9af9a (Core\TSA.c)
undefined8 TSA_SetDim1Res(undefined2 param_1);

// TSA_SetDim2Res @ 0x6ba9afd5 (Core\TSA.c)
undefined8 TSA_SetDim2Res(undefined2 param_1);

// TSA_SetDisableBaselineUpdateFlag @ 0x6ba9f852 (Core\TSA.c)
void TSA_SetDisableBaselineUpdateFlag(undefined1 param_1);

// TSA_SetDisableCMFFlag @ 0x6ba9f871 (Core\TSA.c)
void TSA_SetDisableCMFFlag(uchar param_1);

// TSA_SetFeatures @ 0x6ba9f179 (Core\TSA.c)
undefined4 TSA_SetFeatures(TSA_FeatureFlags param_1);

// TSA_SetGloveFeatures @ 0x6ba9ef6b (Core\TSA.c)
undefined8 TSA_SetGloveFeatures(undefined8 param_1,undefined8 param_2,uint *param_3);

// TSA_SetGripFilterFeatures @ 0x6ba9f04b (Core\TSA.c)
undefined8 TSA_SetGripFilterFeatures(uint param_1,uint *param_2, ...);

// TSA_SetResScale @ 0x6ba9af5f (Core\TSA.c)
undefined8 TSA_SetResScale(undefined2 param_1);

// TSA_SetScreenRotateState @ 0x6ba9fea4 (Core\TSA.c)
void TSA_SetScreenRotateState(int param_1);

// TSA_SetSmartCoverFeatures @ 0x6ba9efdb (Core\TSA.c)
undefined8 TSA_SetSmartCoverFeatures(undefined8 param_1,undefined8 param_2,uint *param_3);

// TSA_SetStylusFeatures @ 0x6ba9f095 (Core\TSA.c)
undefined8 TSA_SetStylusFeatures(undefined8 param_1,undefined8 param_2,uint *param_3);

// TSA_SetUnderWaterFeatures @ 0x6ba9f105 (Core\TSA.c)
undefined8 TSA_SetUnderWaterFeatures(undefined8 param_1,undefined8 param_2,uint *param_3);

// TSA_SetVolumeSideEnabledScreenRotate90or270 @ 0x6ba9012b (Core\TSA.c)
void TSA_SetVolumeSideEnabledScreenRotate90or270(void);

// TSA_SetWorkarounds @ 0x6ba9f3b4 (Core\TSA.c)
undefined4 TSA_SetWorkarounds(uint param_1);

// TSA_SetXPos @ 0x6ba6a355 (Core\TSA.c)
void TSA_SetXPos(byte param_1,undefined2 param_2);

// TSA_SetXRes @ 0x6ba9b010 (Core\TSA.c)
void TSA_SetXRes(undefined2 param_1);

// TSA_SetYRes @ 0x6ba9b056 (Core\TSA.c)
void TSA_SetYRes(undefined2 param_1);

// TSA_SideToDimEdge @ 0x6ba8f560 (Core\TSA.c)
undefined8 TSA_SideToDimEdge(int param_1);

// TSA_SideToEdge @ 0x6ba8f66a (Core\TSA.c)
undefined8 TSA_SideToEdge(uint param_1);

// TSA_SideTouchDim1Center @ 0x6ba8f8f3 (Core\TSA.c)
ushort TSA_SideTouchDim1Center(byte param_1);

// TSA_SideTouchDim2Center @ 0x6ba8f923 (Core\TSA.c)
ushort TSA_SideTouchDim2Center(byte param_1);

// TSA_SideTouchPrmtSetting @ 0x6ba90223 (Core\TSA.c)
void TSA_SideTouchPrmtSetting(void);

// TSA_StylusAftProcess @ 0x6bb22b1c (Core\TSA.c)
void TSA_StylusAftProcess(void);

// TSA_UpdateIDLEModeInternally @ 0x6ba9e458 (Core\TSA.c)
bool TSA_UpdateIDLEModeInternally(void);

// TSA_UpdateReportRate @ 0x6ba9044e (Core\TSA.c)
void TSA_UpdateReportRate(void);

// TSA_UpdateSideIDLEModeInternally @ 0x6ba9e663 (Core\TSA.c)
bool TSA_UpdateSideIDLEModeInternally(void);

// TSA_UpdateSysScenario @ 0x6ba8fc9b (Core\TSA.c)
void TSA_UpdateSysScenario(void);

// TS_GetSizeInMM @ 0x6ba7c692 (Tracking\TS.c)
ulonglong TS_GetSizeInMM(short param_1,int param_2);

// TS_Init @ 0x6ba7c000 (Tracking\TS.c)
void TS_Init(void);

// TS_MMToDisplayPixel @ 0x6ba7c436 (Tracking\TS.c)
uint TS_MMToDisplayPixel(byte param_1);

// TS_MMToPixel @ 0x6ba7c416 (Tracking\TS.c)
int TS_MMToPixel(byte param_1);

// TS_PixelToMM @ 0x6ba7c561 (Tracking\TS.c)
int TS_PixelToMM(ushort param_1);

// TS_PixelToMMScale10 @ 0x6ba7c5a7 (Tracking\TS.c)
int TS_PixelToMMScale10(int param_1);

// TS_PostProcess @ 0x6ba7c368 (Tracking\TS.c)
void TS_PostProcess(void);

// TS_Process @ 0x6ba7c2cb (Tracking\TS.c)
void TS_Process(void);

// TS_ProcessUnit @ 0x6ba7c1d7 (Tracking\TS.c)
void TS_ProcessUnit(byte param_1,byte param_2);

// TS_SQMMToSQPixel @ 0x6ba7c4fb (Tracking\TS.c)
ulonglong TS_SQMMToSQPixel(ushort param_1);

// TS_SQPixelToSQMM @ 0x6ba7c4a0 (Tracking\TS.c)
ulonglong TS_SQPixelToSQMM(uint param_1);

// TS_UpdateCurSizeInMM @ 0x6ba7c006 (Tracking\TS.c)
undefined1 TS_UpdateCurSizeInMM(undefined8 param_1,byte param_2);

// TS_UpdateCurSizeInMMForUdfp @ 0x6ba7c0d6 (Tracking\TS.c)
byte TS_UpdateCurSizeInMMForUdfp(undefined8 param_1,byte param_2);

// TZ_AgeProcess @ 0x6ba8c428 (Detection\TZ.c)
void TZ_AgeProcess(void);

// TZ_BeforeSDCompensationPeakInfoRetrieval @ 0x6ba8adec (Detection\TZ.c)
void TZ_BeforeSDCompensationPeakInfoRetrieval(longlong param_1);

// TZ_BiCubicInter @ 0x6ba85a85 (Detection\TZ.c)
float TZ_BiCubicInter(int param_1,int param_2,int param_3,int param_4);

// TZ_BiCubicInter4x4 @ 0x6ba85c95 (Detection\TZ.c)
float TZ_BiCubicInter4x4(int *param_1,float *param_2,longlong param_3);

// TZ_BiCubicInterEdge @ 0x6ba85db3 (Detection\TZ.c)
float TZ_BiCubicInterEdge(int param_1,int param_2,undefined8 param_3,undefined8 param_4);

// TZ_CalcAxisMass @ 0x6ba86978 (Detection\TZ.c)
void TZ_CalcAxisMass(double *param_1,longlong param_2,int param_3);

// TZ_CalcEdgeCount @ 0x6ba8707e (Detection\TZ.c)
void TZ_CalcEdgeCount(longlong param_1,longlong param_2,uint param_3);

// TZ_CalcGripRatio @ 0x6ba7fa1a (Detection\TZ.c)
byte TZ_CalcGripRatio(undefined4 param_1,undefined8 param_2,undefined4 *param_3,undefined2 param_4, undefined2 param_5);

// TZ_CalcGripRatioDim1OnLongAxis @ 0x6ba7f685 (Detection\TZ.c)
byte TZ_CalcGripRatioDim1OnLongAxis (uint param_1,longlong param_2,uint *param_3,ushort param_4,ushort param_5);

// TZ_CalcGripRatioDim1OnShortAxis @ 0x6ba7f7d8 (Detection\TZ.c)
byte TZ_CalcGripRatioDim1OnShortAxis (uint param_1,longlong param_2,uint *param_3,ushort param_4,ushort param_5);

// TZ_CalcHighResDif @ 0x6ba8607c (Detection\TZ.c)
void TZ_CalcHighResDif(longlong param_1,int param_2);

// TZ_CentrodToTouch @ 0x6ba8b9bf (Detection\TZ.c)
void TZ_CentrodToTouch(longlong param_1,byte param_2);

// TZ_CentroidCore @ 0x6ba7f363 (Detection\TZ.c)
void TZ_CentroidCore(longlong param_1);

// TZ_CentroidEdgeFlag @ 0x6ba7f3cb (Detection\TZ.c)
void TZ_CentroidEdgeFlag(longlong param_1,byte param_2);

// TZ_CentroidGripRatio @ 0x6ba7f2f0 (Detection\TZ.c)
uint TZ_CentroidGripRatio(int param_1,uint param_2);

// TZ_CheckFatFingerSD @ 0x6ba8b4f9 (Detection\TZ.c)
undefined8 TZ_CheckFatFingerSD(void);

// TZ_CheckToSeparateZone @ 0x6ba855b7 (Detection\TZ.c)
void TZ_CheckToSeparateZone(longlong param_1);

// TZ_ContourInit @ 0x6ba8590b (Detection\TZ.c)
void TZ_ContourInit(void);

// TZ_CopyEdgeDot @ 0x6ba87246 (Detection\TZ.c)
void TZ_CopyEdgeDot(longlong param_1,longlong param_2);

// TZ_DFSProcess @ 0x6ba832d9 (Detection\TZ.c)
void TZ_DFSProcess(longlong param_1);

// TZ_DilateAndErode @ 0x6ba8bcf4 (Detection\TZ.c)
void TZ_DilateAndErode(void);

// TZ_ExceptionProcess @ 0x6ba812c2 (Detection\TZ.c)
void TZ_ExceptionProcess(longlong param_1);

// TZ_FindContourDot @ 0x6ba8649b (Detection\TZ.c)
void TZ_FindContourDot(short *param_1);

// TZ_FindContourFirstDotDim2 @ 0x6ba86380 (Detection\TZ.c)
int TZ_FindContourFirstDotDim2(longlong param_1,undefined4 param_2,int param_3,int param_4);

// TZ_Free @ 0x6ba85723 (Detection\TZ.c)
void TZ_Free(void);

// TZ_Get2ClosestPeaks @ 0x6ba8146f (Detection\TZ.c)
void TZ_Get2ClosestPeaks(longlong param_1,undefined1 *param_2,undefined1 *param_3);

// TZ_GetAbsRatio @ 0x6ba823ad (Detection\TZ.c)
int TZ_GetAbsRatio(int param_1,int param_2);

// TZ_GetAccessFlagPtr @ 0x6ba8b2a9 (Detection\TZ.c)
undefined8 TZ_GetAccessFlagPtr(void);

// TZ_GetAutoCentToMinPkRatio @ 0x6ba82e3c (Detection\TZ.c)
undefined1 TZ_GetAutoCentToMinPkRatio(longlong param_1,byte param_2,byte param_3);

// TZ_GetAveCentSig @ 0x6ba82b60 (Detection\TZ.c)
undefined2 TZ_GetAveCentSig(undefined8 param_1,char param_2,char param_3,byte param_4,uint param_5);

// TZ_GetAxis @ 0x6ba7f08d (Detection\TZ.c)
void TZ_GetAxis(longlong param_1);

// TZ_GetAxisInternal @ 0x6ba7ec90 (Detection\TZ.c)
void TZ_GetAxisInternal(double param_1,double param_2,double param_3,double param_4,double param_5, double param_6,undefined2 *param_7,undefined2 *param_8,undefined1 *param_9);

// TZ_GetCMFFlag @ 0x6ba82694 (Detection\TZ.c)
void TZ_GetCMFFlag(longlong param_1);

// TZ_GetCentInfo @ 0x6ba82aa5 (Detection\TZ.c)
void TZ_GetCentInfo(longlong param_1,byte param_2,byte param_3,char *param_4,int *param_5);

// TZ_GetCircleRate @ 0x6ba866ad (Detection\TZ.c)
void TZ_GetCircleRate(longlong param_1);

// TZ_GetContour @ 0x6ba87571 (Detection\TZ.c)
void TZ_GetContour(longlong param_1);

// TZ_GetDBShape @ 0x6ba8a24f (Detection\TZ.c)
undefined4 TZ_GetDBShape(longlong param_1);

// TZ_GetDFSPrpt @ 0x6ba830c5 (Detection\TZ.c)
void TZ_GetDFSPrpt(longlong param_1);

// TZ_GetDifHR @ 0x6ba85fac (Detection\TZ.c)
int TZ_GetDifHR(longlong param_1,int param_2,int param_3);

// TZ_GetDistanceMapPtr @ 0x6ba8ce17 (Detection\TZ.c)
undefined * TZ_GetDistanceMapPtr(void);

// TZ_GetERInernal @ 0x6ba86b64 (Detection\TZ.c)
undefined2 TZ_GetERInernal(longlong param_1,int param_2);

// TZ_GetEdgeDotType @ 0x6ba8700e (Detection\TZ.c)
uint TZ_GetEdgeDotType(int param_1,int param_2);

// TZ_GetEdgeSigThold @ 0x6ba87e84 (Detection\TZ.c)
short TZ_GetEdgeSigThold(short param_1,short param_2);

// TZ_GetEdgeTouchedFlag @ 0x6ba81544 (Detection\TZ.c)
void TZ_GetEdgeTouchedFlag(longlong param_1);

// TZ_GetEdgeWidth @ 0x6ba875ca (Detection\TZ.c)
void TZ_GetEdgeWidth(longlong param_1,short param_2);

// TZ_GetEllipseRate @ 0x6ba86fe1 (Detection\TZ.c)
undefined2 TZ_GetEllipseRate(longlong param_1);

// TZ_GetEllipseRateMirror @ 0x6ba8749f (Detection\TZ.c)
undefined2 TZ_GetEllipseRateMirror(longlong param_1);

// TZ_GetExceptionRatio @ 0x6ba8244c (Detection\TZ.c)
void TZ_GetExceptionRatio(longlong param_1);

// TZ_GetGripRatio @ 0x6ba81f2f (Detection\TZ.c)
void TZ_GetGripRatio(longlong param_1);

// TZ_GetHTIValidFlag @ 0x6ba81626 (Detection\TZ.c)
void TZ_GetHTIValidFlag(longlong param_1);

// TZ_GetId @ 0x6ba802e3 (Detection\TZ.c)
uchar TZ_GetId(longlong param_1,uchar param_2);

// TZ_GetMaxEdgeType @ 0x6ba87437 (Detection\TZ.c)
uint TZ_GetMaxEdgeType(longlong param_1,int param_2);

// TZ_GetMaxMinPkIdx @ 0x6ba82207 (Detection\TZ.c)
void TZ_GetMaxMinPkIdx(longlong param_1);

// TZ_GetMaxNotGripTime @ 0x6ba84020 (Detection\TZ.c)
void TZ_GetMaxNotGripTime(longlong param_1);

// TZ_GetMinEllipseRate @ 0x6ba7fda8 (Detection\TZ.c)
undefined2 TZ_GetMinEllipseRate(byte param_1);

// TZ_GetMinPeakGripTime @ 0x6ba841d4 (Detection\TZ.c)
void TZ_GetMinPeakGripTime(longlong param_1);

// TZ_GetMove @ 0x6ba84fba (Detection\TZ.c)
void TZ_GetMove(longlong param_1);

// TZ_GetOutEdgeSumRatio @ 0x6ba84e4c (Detection\TZ.c)
void TZ_GetOutEdgeSumRatio(longlong param_1);

// TZ_GetPeakGripFlag @ 0x6ba84485 (Detection\TZ.c)
void TZ_GetPeakGripFlag(longlong param_1);

// TZ_GetPrevAge @ 0x6ba8a186 (Detection\TZ.c)
undefined2 TZ_GetPrevAge(byte param_1);

// TZ_GetPrevShape @ 0x6ba8a030 (Detection\TZ.c)
undefined4 TZ_GetPrevShape(byte param_1);

// TZ_GetPrevTouch @ 0x6ba8ab23 (Detection\TZ.c)
void TZ_GetPrevTouch(longlong param_1);

// TZ_GetProperty @ 0x6ba8b126 (Detection\TZ.c)
void TZ_GetProperty(longlong param_1);

// TZ_GetRatio @ 0x6ba823fc (Detection\TZ.c)
int TZ_GetRatio(int param_1,int param_2);

// TZ_GetSearchRange @ 0x6ba846aa (Detection\TZ.c)
void TZ_GetSearchRange(longlong param_1,char param_2,char *param_3,undefined1 *param_4,char *param_5 ,undefined1 *param_6);

// TZ_GetShapeBaseOnCentToMinPkRatio @ 0x6ba8cf22 (Detection\TZ.c)
undefined4 TZ_GetShapeBaseOnCentToMinPkRatio(longlong param_1);

// TZ_GetShapeBaseOnCentralSensor @ 0x6ba8d053 (Detection\TZ.c)
undefined4 TZ_GetShapeBaseOnCentralSensor(longlong param_1);

// TZ_GetShapeBaseOnMajorAxis @ 0x6ba8d011 (Detection\TZ.c)
undefined4 TZ_GetShapeBaseOnMajorAxis(ushort param_1,char param_2);

// TZ_GetShapeBaseOnMinorAxis @ 0x6ba8cfd5 (Detection\TZ.c)
undefined4 TZ_GetShapeBaseOnMinorAxis(ushort param_1, ...);

// TZ_GetShapeFlag @ 0x6ba81fdb (Detection\TZ.c)
void TZ_GetShapeFlag(longlong param_1);

// TZ_GetShapeRatio @ 0x6ba833cb (Detection\TZ.c)
void TZ_GetShapeRatio(longlong param_1);

// TZ_GetSigThold @ 0x6ba8a90b (Detection\TZ.c)
short TZ_GetSigThold(undefined8 param_1,byte param_2);

// TZ_GetSigTholdCommon @ 0x6ba8a7e4 (Detection\TZ.c)
undefined2 TZ_GetSigTholdCommon(undefined8 param_1,byte param_2);

// TZ_GetSigTholdForAFT @ 0x6ba8a51f (Detection\TZ.c)
undefined2 TZ_GetSigTholdForAFT(byte param_1);

// TZ_GetSigTholdForCharger @ 0x6ba8a5d3 (Detection\TZ.c)
short TZ_GetSigTholdForCharger(byte param_1);

// TZ_GetSigTholdForMergePeak @ 0x6ba83afc (Detection\TZ.c)
ushort TZ_GetSigTholdForMergePeak();

// TZ_GetSigTholdForSideTouch @ 0x6ba8a480 (Detection\TZ.c)
undefined2 TZ_GetSigTholdForSideTouch(byte param_1);

// TZ_GetSigTholdForVPA @ 0x6ba8d519 (Detection\TZ.c)
undefined2 TZ_GetSigTholdForVPA();

// TZ_GetSurroundingStatus @ 0x6ba819d9 (Detection\TZ.c)
undefined8 TZ_GetSurroundingStatus(longlong param_1);

// TZ_GetSurroundingStatusFlag @ 0x6ba81efd (Detection\TZ.c)
void TZ_GetSurroundingStatusFlag(longlong param_1);

// TZ_GetTouchZoneRange @ 0x6ba8b2b9 (Detection\TZ.c)
void TZ_GetTouchZoneRange(uint *param_1,uint *param_2,uint *param_3,uint *param_4);

// TZ_GetType @ 0x6ba837f8 (Detection\TZ.c)
void TZ_GetType(longlong param_1);

// TZ_GetTzEdgeWidthDim1 @ 0x6ba7fd24 (Detection\TZ.c)
int TZ_GetTzEdgeWidthDim1(longlong param_1);

// TZ_GetTzEdgeWidthDim2 @ 0x6ba7fd66 (Detection\TZ.c)
int TZ_GetTzEdgeWidthDim2(longlong param_1);

// TZ_GetZ9 @ 0x6ba829c0 (Detection\TZ.c)
int TZ_GetZ9(byte param_1,byte param_2);

// TZ_GetZValue @ 0x6ba7fe48 (Detection\TZ.c)
uint TZ_GetZValue(int param_1);

// TZ_GripLevelProcess @ 0x6ba80e91 (Detection\TZ.c)
void TZ_GripLevelProcess(longlong param_1,byte param_2,byte param_3);

// TZ_Init @ 0x6ba8aa70 (Detection\TZ.c)
void TZ_Init(void);

// TZ_InsertVirtualPeak @ 0x6ba8d460 (Detection\TZ.c)
bool TZ_InsertVirtualPeak(longlong param_1,undefined1 param_2,undefined1 param_3);

// TZ_Interpolate @ 0x6ba8613f (Detection\TZ.c)
void TZ_Interpolate(short *param_1);

// TZ_IsAbnormalShape @ 0x6ba852d3 (Detection\TZ.c)
undefined8 TZ_IsAbnormalShape(longlong param_1,longlong param_2);

// TZ_IsAbsFFMinorAxis @ 0x6ba8ce81 (Detection\TZ.c)
bool TZ_IsAbsFFMinorAxis(byte param_1);

// TZ_IsAbsFFRatio @ 0x6ba8ce57 (Detection\TZ.c)
bool TZ_IsAbsFFRatio(byte param_1);

// TZ_IsAbsFFZ9 @ 0x6ba8ce30 (Detection\TZ.c)
bool TZ_IsAbsFFZ9(int param_1);

// TZ_IsAbsFatFinger @ 0x6ba8ceaf (Detection\TZ.c)
undefined8 TZ_IsAbsFatFinger(longlong param_1);

// TZ_IsClearSignalGapAlongDim1 @ 0x6ba81663 (Detection\TZ.c)
bool TZ_IsClearSignalGapAlongDim1(byte param_1,byte param_2,byte param_3,byte param_4,char param_5);

// TZ_IsClearSignalGapAlongDim2 @ 0x6ba8181e (Detection\TZ.c)
bool TZ_IsClearSignalGapAlongDim2(byte param_1,byte param_2,byte param_3,byte param_4,char param_5);

// TZ_IsEdgeMerged @ 0x6ba88122 (Detection\TZ.c)
undefined8 TZ_IsEdgeMerged(longlong param_1,byte param_2,byte param_3);

// TZ_IsGripMergeAllowed @ 0x6ba85566 (Detection\TZ.c)
undefined8 TZ_IsGripMergeAllowed(longlong param_1);

// TZ_IsGripWall @ 0x6ba87cc5 (Detection\TZ.c)
undefined4 TZ_IsGripWall(byte param_1,int param_2,ushort param_3,ushort param_4,short param_5);

// TZ_IsIDMerged @ 0x6ba87ef9 (Detection\TZ.c)
undefined8 TZ_IsIDMerged(longlong param_1,byte param_2,byte param_3);

// TZ_IsLO @ 0x6ba83474 (Detection\TZ.c)
undefined8 TZ_IsLO(longlong param_1);

// TZ_IsLineJump @ 0x6ba83d7d (Detection\TZ.c)
undefined8 TZ_IsLineJump(longlong param_1);

// TZ_IsOKToInsertGripPeak @ 0x6ba850e8 (Detection\TZ.c)
undefined8 TZ_IsOKToInsertGripPeak(longlong param_1);

// TZ_IsPeakInGrip @ 0x6ba843c8 (Detection\TZ.c)
undefined8 TZ_IsPeakInGrip(byte param_1);

// TZ_IsPeakNotInGrip @ 0x6ba8437e (Detection\TZ.c)
undefined8 TZ_IsPeakNotInGrip(longlong param_1,undefined1 param_2);

// TZ_IsPoorEllipseRate @ 0x6ba83ea4 (Detection\TZ.c)
undefined8 TZ_IsPoorEllipseRate(longlong param_1);

// TZ_IsPossibleGripMerge @ 0x6ba83fcd (Detection\TZ.c)
undefined8 TZ_IsPossibleGripMerge(undefined8 param_1);

// TZ_IsRptTouchMerged @ 0x6ba83b24 (Detection\TZ.c)
undefined8 TZ_IsRptTouchMerged(longlong param_1);

// TZ_IsSD @ 0x6ba8a67e (Detection\TZ.c)
undefined8 TZ_IsSD(longlong param_1);

// TZ_IsTimeOnIntervalOK @ 0x6ba8513a (Detection\TZ.c)
bool TZ_IsTimeOnIntervalOK(longlong param_1);

// TZ_IsToBeRptTouchMerged @ 0x6ba83b90 (Detection\TZ.c)
undefined8 TZ_IsToBeRptTouchMerged(longlong param_1);

// TZ_IsToSeparateZone @ 0x6ba851a2 (Detection\TZ.c)
undefined8 TZ_IsToSeparateZone(longlong param_1);

// TZ_IsTouchMerged @ 0x6ba83d3e (Detection\TZ.c)
undefined8 TZ_IsTouchMerged(undefined8 param_1);

// TZ_IsVPHighPriority @ 0x6ba848db (Detection\TZ.c)
undefined8 TZ_IsVPHighPriority(ushort param_1,ushort param_2,ushort param_3);

// TZ_IsValid @ 0x6ba8aaca (Detection\TZ.c)
undefined8 TZ_IsValid(longlong param_1);

// TZ_IsVirtualPeakAllowed @ 0x6ba8d230 (Detection\TZ.c)
undefined1 TZ_IsVirtualPeakAllowed(longlong param_1);

// TZ_JudgeFFOrMF @ 0x6ba8d12d (Detection\TZ.c)
void TZ_JudgeFFOrMF(longlong param_1);

// TZ_JudgeFFOrNF @ 0x6ba8d1ed (Detection\TZ.c)
undefined4 TZ_JudgeFFOrNF();

// TZ_LogDbShape @ 0x6ba832d0 (Detection\TZ.c)
void TZ_LogDbShape();

// TZ_MFProcess @ 0x6ba80fbd (Detection\TZ.c)
void TZ_MFProcess(longlong param_1);

// TZ_Malloc @ 0x6ba856e0 (Detection\TZ.c)
bool TZ_Malloc(void);

// TZ_MirrorEdgeDot @ 0x6ba872cc (Detection\TZ.c)
void TZ_MirrorEdgeDot(longlong param_1,short *param_2,int param_3);

// TZ_MovePeakNotInGripToEnd @ 0x6ba851f9 (Detection\TZ.c)
void TZ_MovePeakNotInGripToEnd(longlong param_1);

// TZ_MultiFatFingerCheck @ 0x6ba836a1 (Detection\TZ.c)
void TZ_MultiFatFingerCheck(longlong param_1);

// TZ_NormalTraversal @ 0x6ba8baec (Detection\TZ.c)
void TZ_NormalTraversal(longlong param_1,byte param_2,undefined1 param_3);

// TZ_PeakBasedProcess @ 0x6ba8bffd (Detection\TZ.c)
void TZ_PeakBasedProcess(void);

// TZ_PeakBasedTraversal @ 0x6ba89162 (Detection\TZ.c)
void TZ_PeakBasedTraversal(longlong param_1,short param_2,byte param_3,byte param_4);

// TZ_PeakBasedTraversalSimple @ 0x6ba89b60 (Detection\TZ.c)
void TZ_PeakBasedTraversalSimple(undefined8 param_1,short param_2,byte param_3,byte param_4);

// TZ_PeakFilter @ 0x6ba8a009 (Detection\TZ.c)
void TZ_PeakFilter(undefined8 param_1);

// TZ_PeakFilterOnce @ 0x6ba89cc0 (Detection\TZ.c)
undefined8 TZ_PeakFilterOnce(void *param_1);

// TZ_PeakInfoRetrieval @ 0x6ba8acb6 (Detection\TZ.c)
void TZ_PeakInfoRetrieval(undefined8 param_1);

// TZ_PeakRegister @ 0x6ba82339 (Detection\TZ.c)
void TZ_PeakRegister(longlong param_1,undefined1 param_2);

// TZ_Process @ 0x6ba8c9f0 (Detection\TZ.c)
void TZ_Process(void);

// TZ_PushEdgeDot @ 0x6ba8641c (Detection\TZ.c)
void TZ_PushEdgeDot(longlong param_1,undefined2 param_2,undefined2 param_3);

// TZ_RegistForShapeFF @ 0x6ba811e3 (Detection\TZ.c)
void TZ_RegistForShapeFF(longlong param_1);

// TZ_RegisterBasedOnSubTZ @ 0x6ba8096a (Detection\TZ.c)
void TZ_RegisterBasedOnSubTZ(longlong param_1,char param_2);

// TZ_RegisterTouch @ 0x6ba8039b (Detection\TZ.c)
void TZ_RegisterTouch(undefined2 *param_1);

// TZ_RemoveEdgeDot @ 0x6ba87167 (Detection\TZ.c)
void TZ_RemoveEdgeDot(longlong param_1,short *param_2,byte param_3);

// TZ_SSLoProcess @ 0x6ba8be08 (Detection\TZ.c)
void TZ_SSLoProcess(void);

// TZ_SearchMergePeak @ 0x6ba84a90 (Detection\TZ.c)
char TZ_SearchMergePeak(longlong param_1,short *param_2);

// TZ_SearchNxN @ 0x6ba8d53e (Detection\TZ.c)
undefined1 TZ_SearchNxN(char param_1,char param_2,short param_3,short *param_4);

// TZ_SeparateZoneRegretProcess @ 0x6ba85492 (Detection\TZ.c)
void TZ_SeparateZoneRegretProcess(longlong param_1,longlong param_2);

// TZ_ShowContourInIDE @ 0x6ba862eb (Detection\TZ.c)
void TZ_ShowContourInIDE(longlong param_1);

// TZ_SimpleTraversal @ 0x6ba8b88f (Detection\TZ.c)
void TZ_SimpleTraversal(longlong param_1,byte param_2);

// TZ_StylusXYCorrection @ 0x6ba7fc89 (Detection\TZ.c)
int TZ_StylusXYCorrection(ushort param_1);

// TZ_Transform @ 0x6ba8ca70 (Detection\TZ.c)
void TZ_Transform(longlong param_1,char param_2,char param_3,char param_4);

// TZ_TraversalNeighbor @ 0x6ba88709 (Detection\TZ.c)
void TZ_TraversalNeighbor (longlong param_1,ushort param_2,longlong param_3,short param_4,undefined8 param_5, short param_6,undefined2 param_7);

// TZ_TraversalNeighborSimple @ 0x6ba89752 (Detection\TZ.c)
void TZ_TraversalNeighborSimple(longlong param_1,ushort param_2,longlong param_3,short param_4);

// TZ_TryExtraMergePeak @ 0x6ba84dea (Detection\TZ.c)
void TZ_TryExtraMergePeak(longlong param_1,undefined1 param_2,undefined1 param_3);

// TZ_TryInsertVPA @ 0x6ba8d6c4 (Detection\TZ.c)
undefined1 TZ_TryInsertVPA(longlong param_1);

// TZ_TryMergePeak @ 0x6ba84d81 (Detection\TZ.c)
undefined1 TZ_TryMergePeak(undefined8 param_1);

// TZ_UnitInit @ 0x6ba7f620 (Detection\TZ.c)
void TZ_UnitInit(undefined8 param_1);

// TZ_UnitMaxMinInit @ 0x6ba87d7e (Detection\TZ.c)
void TZ_UnitMaxMinInit(undefined2 *param_1,undefined2 param_2);

// TZ_UnitProcess @ 0x6ba8b236 (Detection\TZ.c)
void TZ_UnitProcess(undefined8 param_1);

// TZ_UnitToTouch @ 0x6ba8134b (Detection\TZ.c)
void TZ_UnitToTouch(longlong param_1);

// TZ_UnitToTouchNormal @ 0x6ba8126f (Detection\TZ.c)
void TZ_UnitToTouchNormal(longlong param_1);

// TZ_UpdateAveCentSig @ 0x6ba82c8d (Detection\TZ.c)
short TZ_UpdateAveCentSig(byte param_1,byte param_2,byte param_3,byte param_4,short param_5);

// TZ_UpdateDimMaxMin @ 0x6ba8781b (Detection\TZ.c)
void TZ_UpdateDimMaxMin(longlong param_1,byte param_2,byte param_3);

// TZ_UpdateEdgeInfo @ 0x6ba87985 (Detection\TZ.c)
void TZ_UpdateEdgeInfo(longlong param_1,short param_2,byte param_3,byte param_4,uint param_5);

// TZ_UpdateGripFlag @ 0x6ba8b852 (Detection\TZ.c)
void TZ_UpdateGripFlag(undefined8 param_1);

// TZ_UpdateGripFlagDim @ 0x6ba8b70a (Detection\TZ.c)
void TZ_UpdateGripFlagDim(longlong param_1,byte param_2);

// TZ_UpdateGripRatio @ 0x6ba7faa8 (Detection\TZ.c)
void TZ_UpdateGripRatio(byte param_1,longlong param_2);

// TZ_UpdateInsideSensor @ 0x6ba8019d (Detection\TZ.c)
void TZ_UpdateInsideSensor(longlong param_1,byte *param_2,byte *param_3);

// TZ_UpdateMValue @ 0x6ba87890 (Detection\TZ.c)
void TZ_UpdateMValue(longlong param_1,short param_2,byte param_3,byte param_4);

// TZ_UpdatePeakTzAge @ 0x6ba8c9ad (Detection\TZ.c)
void TZ_UpdatePeakTzAge(void);

// TZ_UpdatePeakTzAgeUnit @ 0x6ba8c665 (Detection\TZ.c)
void TZ_UpdatePeakTzAgeUnit(byte param_1);

// TZ_UpdateTouchFlag @ 0x6ba7fe99 (Detection\TZ.c)
void TZ_UpdateTouchFlag(longlong param_1,byte param_2);

// TZ_UpdateWeightedSumSimple @ 0x6ba896a4 (Detection\TZ.c)
void TZ_UpdateWeightedSumSimple(longlong param_1,short param_2,byte param_3,byte param_4);

// TZ_VPInit @ 0x6ba8d320 (Detection\TZ.c)
void TZ_VPInit(void);

// TZ_VPProcess @ 0x6ba8d348 (Detection\TZ.c)
void TZ_VPProcess(longlong param_1);

// TZ_VirtualPeakInfoRetrieval @ 0x6ba8af94 (Detection\TZ.c)
void TZ_VirtualPeakInfoRetrieval(void);

// ToeSynaBl_BackLightShiftCheckTouch @ 0x6bb45d0d (Baseline\ToeSynaBl.c)
undefined8 ToeSynaBl_BackLightShiftCheckTouch(void);

// ToeSynaBl_BackLightShiftProcess @ 0x6bb463d8 (Baseline\ToeSynaBl.c)
void ToeSynaBl_BackLightShiftProcess(void);

// ToeSynaBl_BackLightShiftUnit @ 0x6bb45dd1 (Baseline\ToeSynaBl.c)
undefined8 ToeSynaBl_BackLightShiftUnit(ushort param_1);

// ToeSynaBl_CheckNumOverNoise @ 0x6bb45640 (Baseline\ToeSynaBl.c)
undefined8 ToeSynaBl_CheckNumOverNoise(void);

// ToeSynaBl_ClothRiskProcess @ 0x6bb464e6 (Baseline\ToeSynaBl.c)
void ToeSynaBl_ClothRiskProcess(void);

// ToeSynaBl_Free @ 0x6bb46137 (Baseline\ToeSynaBl.c)
void ToeSynaBl_Free(void);

// ToeSynaBl_GetSidePrpt @ 0x6bb471b0 (Baseline\ToeSynaBl.c)
void ToeSynaBl_GetSidePrpt(void);

// ToeSynaBl_GripRegionAbnormal @ 0x6bb474ef (Baseline\ToeSynaBl.c)
undefined8 ToeSynaBl_GripRegionAbnormal(char param_1);

// ToeSynaBl_IsAbnormalTouchShape @ 0x6bb4681a (Baseline\ToeSynaBl.c)
bool ToeSynaBl_IsAbnormalTouchShape(char param_1);

// ToeSynaBl_IsBaselineAbnormal @ 0x6bb4782f (Baseline\ToeSynaBl.c)
void ToeSynaBl_IsBaselineAbnormal(char param_1,char param_2,undefined1 *param_3);

// ToeSynaBl_IsBaselineAbnormalAftScreenOn @ 0x6bb465b4 (Baseline\ToeSynaBl.c)
undefined8 ToeSynaBl_IsBaselineAbnormalAftScreenOn(void);

// ToeSynaBl_IsBaselineAbnormalSSProtectedByDim1 @ 0x6bb45c29 (Baseline\ToeSynaBl.c)
undefined8 ToeSynaBl_IsBaselineAbnormalSSProtectedByDim1(void);

// ToeSynaBl_IsColumnRawChanged @ 0x6bb46bc3 (Baseline\ToeSynaBl.c)
undefined4 ToeSynaBl_IsColumnRawChanged(byte param_1);

// ToeSynaBl_IsLongTimeSSNoSignal @ 0x6bb46228 (Baseline\ToeSynaBl.c)
bool ToeSynaBl_IsLongTimeSSNoSignal(void);

// ToeSynaBl_IsNextSideDifNormal @ 0x6bb46c9c (Baseline\ToeSynaBl.c)
uint ToeSynaBl_IsNextSideDifNormal(byte param_1);

// ToeSynaBl_IsRawChangedSinceBackLightShift @ 0x6bb46030 (Baseline\ToeSynaBl.c)
char ToeSynaBl_IsRawChangedSinceBackLightShift(undefined1 param_1);

// ToeSynaBl_IsRawChangedSinceSSNoSignal @ 0x6bb46062 (Baseline\ToeSynaBl.c)
char ToeSynaBl_IsRawChangedSinceSSNoSignal(undefined1 param_1);

// ToeSynaBl_IsSSNotMatchWithMutual @ 0x6bb46a77 (Baseline\ToeSynaBl.c)
bool ToeSynaBl_IsSSNotMatchWithMutual(char param_1);

// ToeSynaBl_IsSideTouchToBeProtected @ 0x6bb4577d (Baseline\ToeSynaBl.c)
undefined8 ToeSynaBl_IsSideTouchToBeProtected(void);

// ToeSynaBl_IsTouchToBeProtected2 @ 0x6bb45675 (Baseline\ToeSynaBl.c)
undefined1 ToeSynaBl_IsTouchToBeProtected2(undefined1 param_1,undefined1 param_2);

// ToeSynaBl_IsWaterDetected @ 0x6bb45a50 (Baseline\ToeSynaBl.c)
undefined8 ToeSynaBl_IsWaterDetected(void);

// ToeSynaBl_IsWaterDetectedZ8Check @ 0x6bb45950 (Baseline\ToeSynaBl.c)
undefined1 ToeSynaBl_IsWaterDetectedZ8Check(void);

// ToeSynaBl_Malloc @ 0x6bb46094 (Baseline\ToeSynaBl.c)
undefined8 ToeSynaBl_Malloc(void);

// ToeSynaBl_PrptSideRawProtected @ 0x6bb46f9f (Baseline\ToeSynaBl.c)
undefined1 ToeSynaBl_PrptSideRawProtected(uint param_1,uint param_2);

// ToeSynaBl_PrptSideRawProtected2 @ 0x6bb4701b (Baseline\ToeSynaBl.c)
undefined1 ToeSynaBl_PrptSideRawProtected2(int param_1,byte param_2,uint param_3,uint param_4);

// ToeSynaBl_SSIsDim2NoSignal @ 0x6bb4616a (Baseline\ToeSynaBl.c)
undefined8 ToeSynaBl_SSIsDim2NoSignal(void);

// ToeSynaBl_SideTouchCheckSS @ 0x6bb456af (Baseline\ToeSynaBl.c)
undefined8 ToeSynaBl_SideTouchCheckSS(ushort param_1,short param_2);

// ToeSyna_KeepSSBaseline @ 0x6bb474dd (Baseline\ToeSyna.c)
bool ToeSyna_KeepSSBaseline(void);

// ToeSyna_PorpertyGetMaxSig @ 0x6bb47483 (Baseline\ToeSyna.c)
undefined2 ToeSyna_PorpertyGetMaxSig(undefined2 param_1);

// ToeSyna_PorpertyGetMinSig @ 0x6bb47412 (Baseline\ToeSyna.c)
undefined2 ToeSyna_PorpertyGetMinSig(undefined2 param_1);

// TouchAction_Init @ 0x6ba77220 (Tracking\TouchAction.c)
void TouchAction_Init(void);

// TouchAction_IsFlickCheckPassed @ 0x6ba773d9 (Tracking\TouchAction.c)
bool TouchAction_IsFlickCheckPassed(void);

// TouchAction_PostProcess @ 0x6ba773eb (Tracking\TouchAction.c)
void TouchAction_PostProcess(void);

// TouchAction_Process @ 0x6ba7733c (Tracking\TouchAction.c)
void TouchAction_Process(void);

// TouchAction_ProcessUnit @ 0x6ba77226 (Tracking\TouchAction.c)
void TouchAction_ProcessUnit(byte param_1,byte param_2);

// TouchMode_FingerDetect @ 0x6ba7911f (Filter\TouchMode.c)
void TouchMode_FingerDetect(undefined4 *param_1,undefined4 *param_2);

// TouchMode_FingerProcess @ 0x6ba79318 (Filter\TouchMode.c)
void TouchMode_FingerProcess(undefined8 param_1,int param_2);

// TouchMode_GetStylusNum @ 0x6ba79c23 (Filter\TouchMode.c)
char TouchMode_GetStylusNum(void);

// TouchMode_GloveDetect @ 0x6ba793fe (Filter\TouchMode.c)
void TouchMode_GloveDetect(undefined4 *param_1,undefined4 *param_2);

// TouchMode_GloveFilter @ 0x6ba796d1 (Filter\TouchMode.c)
void TouchMode_GloveFilter(void);

// TouchMode_GloveProcess @ 0x6ba797a7 (Filter\TouchMode.c)
void TouchMode_GloveProcess(undefined8 param_1,int param_2);

// TouchMode_Init @ 0x6ba78b60 (Filter\TouchMode.c)
void TouchMode_Init(void);

// TouchMode_NewModeProcess @ 0x6ba78c16 (Filter\TouchMode.c)
void TouchMode_NewModeProcess(uint *param_1,uint *param_2,longlong param_3);

// TouchMode_NoObjDetect @ 0x6ba790ae (Filter\TouchMode.c)
void TouchMode_NoObjDetect(undefined4 *param_1,undefined4 *param_2);

// TouchMode_NoObjProcess @ 0x6ba79113 (Filter\TouchMode.c)
void TouchMode_NoObjProcess(void);

// TouchMode_Process @ 0x6ba78dde (Filter\TouchMode.c)
void TouchMode_Process(void);

// TouchMode_StylusDetect @ 0x6ba798b8 (Filter\TouchMode.c)
void TouchMode_StylusDetect(undefined4 *param_1,undefined4 *param_2);

// TouchMode_StylusFilter @ 0x6ba7a0ed (Filter\TouchMode.c)
void TouchMode_StylusFilter(void);

// TouchMode_StylusFilterProcess @ 0x6ba79e26 (Filter\TouchMode.c)
byte TouchMode_StylusFilterProcess(void);

// TouchMode_StylusGetCoreIndex @ 0x6ba79b0f (Filter\TouchMode.c)
byte TouchMode_StylusGetCoreIndex(void);

// TouchMode_StylusIsClearanceAreaSatisfied @ 0x6ba79930 (Filter\TouchMode.c)
undefined1 TouchMode_StylusIsClearanceAreaSatisfied(byte param_1);

// TouchMode_StylusProcess @ 0x6ba7991a (Filter\TouchMode.c)
void TouchMode_StylusProcess(void);

// TouchMode_StylusSuppression @ 0x6ba79ba6 (Filter\TouchMode.c)
void TouchMode_StylusSuppression(void);

// TouchMode_SwitchPriorityProcess @ 0x6ba7900b (Filter\TouchMode.c)
void TouchMode_SwitchPriorityProcess(void);

// TouchReport_GoBackMultiStep @ 0x6ba7a79b (Rejection\TouchReport.c)
void TouchReport_GoBackMultiStep(byte param_1,byte param_2);

// TouchReport_GoBackOneStep @ 0x6ba7a134 (Rejection\TouchReport.c)
void TouchReport_GoBackOneStep(byte param_1,byte param_2);

// TouchReport_GoBackReport @ 0x6ba7ae1d (Rejection\TouchReport.c)
void TouchReport_GoBackReport(byte param_1,undefined1 param_2);

// TouchReport_Init @ 0x6ba7a120 (Rejection\TouchReport.c)
void TouchReport_Init(void);

// TouchReport_IsAllTouchBack @ 0x6ba7a3d6 (Rejection\TouchReport.c)
undefined1 TouchReport_IsAllTouchBack(void);

// TouchReport_IsAnyTouchCancelByPosture @ 0x6ba7a487 (Rejection\TouchReport.c)
undefined8 TouchReport_IsAnyTouchCancelByPosture(void);

// TouchReport_IsHistoryBufferApplied @ 0x6ba7b373 (Rejection\TouchReport.c)
undefined1 TouchReport_IsHistoryBufferApplied (byte param_1,undefined1 param_2,undefined2 *param_3,undefined2 *param_4);

// TouchReport_IsMultStepBackEnable @ 0x6ba7a3b0 (Rejection\TouchReport.c)
bool TouchReport_IsMultStepBackEnable(void);

// TouchReport_IsReadyToCancel @ 0x6ba7a577 (Rejection\TouchReport.c)
undefined8 TouchReport_IsReadyToCancel(undefined8 param_1,byte param_2);

// TouchReport_PostureAntiTouch @ 0x6ba7ae86 (Rejection\TouchReport.c)
undefined8 TouchReport_PostureAntiTouch(byte param_1,byte param_2);

// TouchReport_Process @ 0x6ba7be73 (Rejection\TouchReport.c)
void TouchReport_Process(void);

// TouchReport_ProcessUnit @ 0x6ba7b86e (Rejection\TouchReport.c)
void TouchReport_ProcessUnit(byte param_1,byte param_2);

// TouchReport_ToBeReported @ 0x6ba7afbc (Rejection\TouchReport.c)
bool TouchReport_ToBeReported(byte param_1,byte param_2);

// TouchReport_UpdateStartReportPoint @ 0x6ba7b657 (Rejection\TouchReport.c)
void TouchReport_UpdateStartReportPoint(byte param_1,byte param_2);

// TouchThold_BasedOnExperience @ 0x6ba7cb32 (Tracking\TouchThold.c)
short TouchThold_BasedOnExperience(short param_1);

// TouchThold_GetNoiseThold @ 0x6ba7ca48 (Tracking\TouchThold.c)
short TouchThold_GetNoiseThold(short param_1,short param_2);

// TouchThold_GetNoisyTouchNum @ 0x6ba7c896 (Tracking\TouchThold.c)
char TouchThold_GetNoisyTouchNum(void);

// TouchThold_GetPeakThold @ 0x6ba7cee9 (Tracking\TouchThold.c)
short TouchThold_GetPeakThold(short param_1,short param_2);

// TouchThold_GetTholdBasedOnSNR @ 0x6ba7c776 (Tracking\TouchThold.c)
int TouchThold_GetTholdBasedOnSNR(short param_1);

// TouchThold_GetTholdBasedOnTouchMode @ 0x6ba7c7a5 (Tracking\TouchThold.c)
int TouchThold_GetTholdBasedOnTouchMode(ushort param_1,short param_2);

// TouchThold_GetTouchDownThold @ 0x6ba7cca5 (Tracking\TouchThold.c)
short TouchThold_GetTouchDownThold(short param_1);

// TouchThold_Init @ 0x6ba7c922 (Tracking\TouchThold.c)
void TouchThold_Init(void);

// TouchThold_IsSwitching @ 0x6ba7c90e (Tracking\TouchThold.c)
undefined1 TouchThold_IsSwitching(void);

// TouchThold_Process @ 0x6ba7d0ee (Tracking\TouchThold.c)
void TouchThold_Process(short param_1,short param_2,short *param_3,short *param_4, undefined2 *param_5);

// TouchThold_SOFInSmartCoverMode @ 0x6ba7ceb3 (Tracking\TouchThold.c)
undefined8 TouchThold_SOFInSmartCoverMode(void);

// TouchThold_ToeUseSpecialThold @ 0x6ba7cd8f (Tracking\TouchThold.c)
undefined8 TouchThold_ToeUseSpecialThold(void);

// TouchThold_UseHighTholdAtSensitivityMode @ 0x6ba7ce03 (Tracking\TouchThold.c)
undefined8 TouchThold_UseHighTholdAtSensitivityMode(void);

// TouchZone_GetInnerCornerSigRatio @ 0x6ba883a8 (Detection\TouchZone.c)
int TouchZone_GetInnerCornerSigRatio(short param_1,short param_2,short param_3);

// TouchZone_InnerCornerHandler @ 0x6ba88419 (Detection\TouchZone.c)
void TouchZone_InnerCornerHandler(longlong param_1,byte param_2,byte param_3);

// Touch_AssignPreIdxBasedOnID @ 0x6ba73fc7 (Tracking\Touch.c)
void Touch_AssignPreIdxBasedOnID(void);

// Touch_CheckTrustabilityUnit @ 0x6ba718d4 (Tracking\Touch.c)
void Touch_CheckTrustabilityUnit(byte param_1,byte param_2);

// Touch_Clean @ 0x6ba6f49c (Tracking\Touch.c)
void Touch_Clean(void);

// Touch_CreatePostReport @ 0x6ba76a03 (Tracking\Touch.c)
void Touch_CreatePostReport(byte param_1, byte param_2);

// Touch_GetDim1EdgeDist @ 0x6ba72a59 (Tracking\Touch.c)
uint Touch_GetDim1EdgeDist(ushort param_1);

// Touch_GetDim2EdgeDist @ 0x6ba72ab9 (Tracking\Touch.c)
uint Touch_GetDim2EdgeDist(ushort param_1);

// Touch_GetDistToLogicEdge @ 0x6ba73009 (Tracking\Touch.c)
short Touch_GetDistToLogicEdge(int param_1,short param_2,short param_3);

// Touch_GetFltSQDistPixelFromPrevPos @ 0x6ba73cc1 (Tracking\Touch.c)
void Touch_GetFltSQDistPixelFromPrevPos(byte param_1,byte param_2);

// Touch_GetFormattedTimeInterval @ 0x6ba76329 (Tracking\Touch.c)
short Touch_GetFormattedTimeInterval(short param_1);

// Touch_GetIdxBasedOnPreIdx @ 0x6ba74433 (Tracking\Touch.c)
undefined1 Touch_GetIdxBasedOnPreIdx(byte param_1);

// Touch_GetMergedInfo @ 0x6ba74b56 (Tracking\Touch.c)
void Touch_GetMergedInfo(byte param_1);

// Touch_GetPeakSumPosSig @ 0x6ba6f641 (Tracking\Touch.c)
void Touch_GetPeakSumPosSig(byte param_1,byte param_2);

// Touch_GetPostReportLength @ 0x6ba76958 (Tracking\Touch.c)
uint Touch_GetPostReportLength(int param_1,int param_2,int param_3,int param_4, ...);

// Touch_GetRptSQDistDisplayPixelFromPrevPos @ 0x6ba73ba1 (Tracking\Touch.c)
void Touch_GetRptSQDistDisplayPixelFromPrevPos(byte param_1,byte param_2);

// Touch_GetRptSQDistMMFromPrevPos @ 0x6ba73ac1 (Tracking\Touch.c)
void Touch_GetRptSQDistMMFromPrevPos(byte param_1,byte param_2);

// Touch_GetRptSQDistMMFromStartPos @ 0x6ba739e1 (Tracking\Touch.c)
void Touch_GetRptSQDistMMFromStartPos(byte param_1,byte param_2);

// Touch_GetSQAccMMFromPrevPos @ 0x6ba736aa (Tracking\Touch.c)
void Touch_GetSQAccMMFromPrevPos(byte param_1,byte param_2);

// Touch_GetSQDistMM @ 0x6ba734be (Tracking\Touch.c)
undefined2 Touch_GetSQDistMM(undefined2 param_1,undefined2 param_2,undefined2 param_3,undefined2 param_4);

// Touch_GetSQDistMMFromPrevPos @ 0x6ba735c9 (Tracking\Touch.c)
void Touch_GetSQDistMMFromPrevPos(byte param_1,byte param_2);

// Touch_GetSQDistMMFromStartPos @ 0x6ba738ff (Tracking\Touch.c)
void Touch_GetSQDistMMFromStartPos(byte param_1,byte param_2);

// Touch_GetSQDistMMInBetween @ 0x6ba73515 (Tracking\Touch.c)
ushort Touch_GetSQDistMMInBetween(byte param_1,byte param_2);

// Touch_GetSQDistPixel @ 0x6ba73447 (Tracking\Touch.c)
uint Touch_GetSQDistPixel(ushort param_1,ushort param_2,ushort param_3,ushort param_4);

// Touch_GetSQDistPixelFromPrevPos @ 0x6ba7381e (Tracking\Touch.c)
void Touch_GetSQDistPixelFromPrevPos(byte param_1,byte param_2);

// Touch_GetSQDistPixelToEdge @ 0x6ba7329a (Tracking\Touch.c)
int Touch_GetSQDistPixelToEdge(short param_1,short param_2,short param_3,short param_4);

// Touch_GetTimeOn @ 0x6ba6f517 (Tracking\Touch.c)
int Touch_GetTimeOn(byte param_1);

// Touch_GetTimeRpt @ 0x6ba6f5ac (Tracking\Touch.c)
int Touch_GetTimeRpt(byte param_1);

// Touch_IIRDataProcess @ 0x6ba7474f (Tracking\Touch.c)
void Touch_IIRDataProcess(byte param_1,byte param_2);

// Touch_IIRProcess @ 0x6ba746ed (Tracking\Touch.c)
int Touch_IIRProcess(int param_1,int param_2);

// Touch_Init @ 0x6ba6f503 (Tracking\Touch.c)
void Touch_Init(void);

// Touch_IsAbnormalShape @ 0x6ba74dfe (Tracking\Touch.c)
undefined8 Touch_IsAbnormalShape(byte param_1);

// Touch_IsAbnormalShape2 @ 0x6ba7504e (Tracking\Touch.c)
undefined8 Touch_IsAbnormalShape2(byte param_1);

// Touch_IsAbnormalShape3 @ 0x6ba7513e (Tracking\Touch.c)
undefined8 Touch_IsAbnormalShape3(byte param_1);

// Touch_IsAbnormalShape4 @ 0x6ba7532f (Tracking\Touch.c)
undefined8 Touch_IsAbnormalShape4(byte param_1);

// Touch_IsBetaGrip @ 0x6ba76d53 (Tracking\Touch.c)
undefined8 Touch_IsBetaGrip(byte param_1,byte param_2);

// Touch_IsCoreOffInSide @ 0x6ba715a8 (Tracking\Touch.c)
undefined1 Touch_IsCoreOffInSide(undefined1 param_1, ...);

// Touch_IsException @ 0x6ba7416a (Tracking\Touch.c)
undefined1 Touch_IsException(byte param_1,byte param_2);

// Touch_IsFarAwayFromCornerCompareToStartPoint @ 0x6ba74aef (Tracking\Touch.c)
bool Touch_IsFarAwayFromCornerCompareToStartPoint(byte param_1, ...);

// Touch_IsFullyLiftOff @ 0x6ba717cf (Tracking\Touch.c)
bool Touch_IsFullyLiftOff(byte param_1);

// Touch_IsGhostByMinSigOff @ 0x6ba7160b (Tracking\Touch.c)
undefined8 Touch_IsGhostByMinSigOff(byte param_1);

// Touch_IsGhostByMinSigOff2 @ 0x6ba7173d (Tracking\Touch.c)
bool Touch_IsGhostByMinSigOff2(byte param_1);

// Touch_IsGrip @ 0x6ba742be (Tracking\Touch.c)
char Touch_IsGrip(undefined1 param_1);

// Touch_IsInAbnormalShapeDetectArea @ 0x6ba74c6f (Tracking\Touch.c)
undefined8 Touch_IsInAbnormalShapeDetectArea(byte param_1);

// Touch_IsLiftOffDueToMerged @ 0x6ba73da0 (Tracking\Touch.c)
undefined8 Touch_IsLiftOffDueToMerged(undefined1 param_1,byte param_2);

// Touch_IsOff @ 0x6ba74ab5 (Tracking\Touch.c)
char Touch_IsOff(byte param_1);

// Touch_IsOffEvent @ 0x6ba74a8e (Tracking\Touch.c)
undefined8 Touch_IsOffEvent(int param_1);

// Touch_IsPeakOnInnerDim1Edge @ 0x6ba70f56 (Tracking\Touch.c)
undefined8 Touch_IsPeakOnInnerDim1Edge(byte param_1);

// Touch_IsSideCoreInSameZone @ 0x6ba71415 (Tracking\Touch.c)
undefined8 Touch_IsSideCoreInSameZone(byte param_1,byte param_2);

// Touch_IsSideOverCore @ 0x6ba714db (Tracking\Touch.c)
undefined1 Touch_IsSideOverCore(byte param_1,byte param_2);

// Touch_IsTouchDownDueToSeperated @ 0x6ba73e7a (Tracking\Touch.c)
undefined8 Touch_IsTouchDownDueToSeperated(byte param_1);

// Touch_KeepPrevTouchWithExceptionExcluded @ 0x6ba766b9 (Tracking\Touch.c)
void Touch_KeepPrevTouchWithExceptionExcluded(void);

// Touch_NeedNextEdgeLowThresh @ 0x6ba6fe70 (Tracking\Touch.c)
undefined1 Touch_NeedNextEdgeLowThresh(byte param_1, ...);

// Touch_NeedXYFilter @ 0x6ba742dc (Tracking\Touch.c)
bool Touch_NeedXYFilter(byte param_1,byte param_2);

// Touch_ProcessAftTouchEvent @ 0x6ba75af5 (Tracking\Touch.c)
void Touch_ProcessAftTouchEvent(void);

// Touch_ProcessAftTouchEventUnit @ 0x6ba7542a (Tracking\Touch.c)
void Touch_ProcessAftTouchEventUnit(byte param_1);

// Touch_ProcessBetaGrip @ 0x6ba76ee8 (Tracking\Touch.c)
void Touch_ProcessBetaGrip(void);

// Touch_ProcessExtInfo @ 0x6ba7713d (Tracking\Touch.c)
void Touch_ProcessExtInfo(void);

// Touch_SetEdgeDist @ 0x6ba72b19 (Tracking\Touch.c)
void Touch_SetEdgeDist(byte param_1);

// Touch_SetEdgeFlag @ 0x6ba72d1c (Tracking\Touch.c)
void Touch_SetEdgeFlag(byte param_1);

// Touch_SetPersistFlag @ 0x6ba72776 (Tracking\Touch.c)
void Touch_SetPersistFlag(byte param_1,byte param_2);

// Touch_SetPersistFlagMoveOverClickSuppressRange @ 0x6ba723e9 (Tracking\Touch.c)
void Touch_SetPersistFlagMoveOverClickSuppressRange(byte param_1,byte param_2);

// Touch_SetPersistFlagMoveOverFlickDistance @ 0x6ba72647 (Tracking\Touch.c)
void Touch_SetPersistFlagMoveOverFlickDistance(byte param_1,byte param_2);

// Touch_SetPersistFlagMoveOverMaxClickSuppressRange @ 0x6ba72518 (Tracking\Touch.c)
void Touch_SetPersistFlagMoveOverMaxClickSuppressRange(byte param_1,byte param_2);

// Touch_SetRegion @ 0x6ba72bce (Tracking\Touch.c)
void Touch_SetRegion(byte param_1);

// Touch_SetSuppressFlag @ 0x6ba72ffb (Tracking\Touch.c)
void Touch_SetSuppressFlag(byte param_1, ...);

// Touch_SetTrustableTag @ 0x6ba71d77 (Tracking\Touch.c)
void Touch_SetTrustableTag(byte param_1);

// Touch_SetVolatileFlag @ 0x6ba70fdd (Tracking\Touch.c)
void Touch_SetVolatileFlag(byte param_1,byte param_2);

// Touch_SetVolatileFlagFalseFlickDetected @ 0x6ba71f3a (Tracking\Touch.c)
void Touch_SetVolatileFlagFalseFlickDetected(byte param_1,byte param_2);

// Touch_SetVolatileFlagFingerZFailed @ 0x6ba700e1 (Tracking\Touch.c)
void Touch_SetVolatileFlagFingerZFailed(byte param_1, ...);

// Touch_SetVolatileFlagLiftOffZFailed @ 0x6ba70356 (Tracking\Touch.c)
void Touch_SetVolatileFlagLiftOffZFailed(byte param_1,byte param_2,char param_3);

// Touch_SetVolatileFlagSigSum @ 0x6ba6f7af (Tracking\Touch.c)
void Touch_SetVolatileFlagSigSum(byte param_1,byte param_2);

// Touch_SetVolatileFlagStableTouch @ 0x6ba70775 (Tracking\Touch.c)
void Touch_SetVolatileFlagStableTouch(byte param_1);

// Touch_SetVolatileFlagTouchAxisAbnormal @ 0x6ba6fc6a (Tracking\Touch.c)
void Touch_SetVolatileFlagTouchAxisAbnormal(byte param_1,byte param_2);

// Touch_SetVolatileFlagTouchDownZFailed @ 0x6ba701c3 (Tracking\Touch.c)
void Touch_SetVolatileFlagTouchDownZFailed(byte param_1,undefined8 param_2,char param_3);

// Touch_ThenarFeatureReset @ 0x6bb30bf0 (Tracking\Touch.c)
void Touch_ThenarFeatureReset(void);

// Touch_ToeUseSpecialZ4TholdEdge @ 0x6ba6fe1a (Tracking\Touch.c)
undefined8 Touch_ToeUseSpecialZ4TholdEdge(void);

// Touch_UnitCopy @ 0x6ba74462 (Tracking\Touch.c)
void Touch_UnitCopy(byte param_1,byte param_2);

// Touch_UnitRemove @ 0x6ba763f4 (Tracking\Touch.c)
void Touch_UnitRemove(byte param_1);

// Touch_UnitUpdate @ 0x6ba760da (Tracking\Touch.c)
void Touch_UnitUpdate(byte param_1,byte param_2);

// Touch_UnitUpdateMaxMin @ 0x6ba75d1a (Tracking\Touch.c)
void Touch_UnitUpdateMaxMin(byte param_1,byte param_2);

// Touch_UnitUpdateSqDistMMFromStartPosMax @ 0x6ba75c2e (Tracking\Touch.c)
void Touch_UnitUpdateSqDistMMFromStartPosMax(byte param_1,byte param_2);

// Touch_UpdateGlobalState @ 0x6ba75b75 (Tracking\Touch.c)
void Touch_UpdateGlobalState(byte param_1,byte param_2);

// Touch_UpdateHandEnum @ 0x6ba70df0 (Tracking\Touch.c)
void Touch_UpdateHandEnum(byte param_1,byte param_2);

// Touch_UpdateStableProperty @ 0x6ba708d8 (Tracking\Touch.c)
void Touch_UpdateStableProperty(byte param_1,byte param_2);

// Touch_ValidComparison @ 0x6ba7435e (Tracking\Touch.c)
undefined8 Touch_ValidComparison(byte param_1,byte param_2);

// UpdateEndTime @ 0x6baf635d (Tracking\HotZoneAdjust.c)
void UpdateEndTime(longlong param_1);

// UpdateHotZonePosition @ 0x6baf522e (Tracking\HotZoneAdjust.c)
undefined8 UpdateHotZonePosition(byte param_1,undefined8 *param_2);

// UpdateStartTime @ 0x6baf633b (Tracking\HotZoneAdjust.c)
void UpdateStartTime(undefined8 *param_1);

// Thenar_ProcessInfoByDif @ 0x6bb315e8 (Rejection\Thenar.c)
void Thenar_ProcessInfoByDif(void);
// Thenar_UnitCopy @ 0x6bb2fd40 (Rejection\Thenar.c)
void Thenar_UnitCopy(undefined1 *param_1,undefined1 *param_2);
// Thenar_EventUpdate @ 0x6bb30eb3 (Rejection\Thenar.c)
void Thenar_EventUpdate(void);
// Thenar_ProcessInfoByDifTraditional @ 0x6bb31392 (Rejection\Thenar.c)
void Thenar_ProcessInfoByDifTraditional(void);
// Thenar_UpdateExternalInfo @ 0x6bb2fe59 (Rejection\Thenar.c)
bool Thenar_UpdateExternalInfo(void);
// Thenar_EventUpdateUnit @ 0x6bb30d38 (Rejection\Thenar.c)
void Thenar_EventUpdateUnit(int param_1,uint param_2,uint param_3);
// Thenar_ProcessGensture @ 0x6bb2fefc (Rejection\Thenar.c)
void Thenar_ProcessGensture(short param_1);
// Thenar_SideHandDetectByCoreTouch @ 0x6bb300a5 (Rejection\Thenar.c)
void Thenar_SideHandDetectByCoreTouch(byte param_1);
// Thenar_FeatureExtraction @ 0x6bb30c47 (Rejection\Thenar.c)
void Thenar_FeatureExtraction(undefined8 param_1,undefined2 param_2);
// Thenar_UpdateDetectFeature @ 0x6bb30f99 (Rejection\Thenar.c)
undefined8 * Thenar_UpdateDetectFeature(undefined8 *param_1);
// Thenar_Init @ 0x6bb2fce0 (Rejection\Thenar.c)
void Thenar_Init(void);
// Thenar_FeatureExtractionUnit @ 0x6bb306a0 (Rejection\Thenar.c)
void Thenar_FeatureExtractionUnit(longlong param_1,ushort param_2,short param_3,ushort param_4,undefined2 param_5);
// Thenar_FeatureGradient @ 0x6bb3032e (Rejection\Thenar.c)
short Thenar_FeatureGradient(longlong param_1,ushort param_2,undefined4 param_3,ushort param_4, undefined8 param_5);
// Thenar_FeatureUpdateEdgeWidthX @ 0x6bb30495 (Rejection\Thenar.c)
short Thenar_FeatureUpdateEdgeWidthX(longlong param_1,ushort param_2,undefined4 param_3,ushort param_4,byte param_5);
// Thenar_FeatureUnitPostUpdate @ 0x6bb305f5 (Rejection\Thenar.c)
void Thenar_FeatureUnitPostUpdate(byte param_1,undefined2 param_2,undefined2 param_3,undefined2 param_4);
// Self_Process @ 0x6ba6aea8 (Signal\Self.c)
void Self_Process(void);
// Self_ProcessDim @ 0x6ba6abb4 (Signal\Self.c)
void Self_ProcessDim(int param_1);
// Self_GetProperty @ 0x6ba6ae85 (Signal\Self.c)
void Self_GetProperty(void);
// Self_ProcessDimUnit @ 0x6ba6a984 (Signal\Self.c)
void Self_ProcessDimUnit(ushort param_1,uint param_2,int param_3,uint param_4);
// Self_GetPropertyDim @ 0x6ba6ac88 (Signal\Self.c)
void Self_GetPropertyDim(int param_1);
// Self_Reset @ 0x6ba6a920 (Signal\Self.c)
void Self_Reset(void);

// --- ASA 侧被手指管线读写的那一层（Rejection\ASAHostBridge.c） ---
// TSA_ASAProcess @ 0x6ba99e8d (Core\TSA.c)
void TSA_ASAProcess(uint param_1);
// TSA_RptASAInRange @ 0x6ba9c7e9 (Core\TSA.c)
undefined8 TSA_RptASAInRange(void);
// TSA_SetASABluetoothConnectStatus @ 0x6ba9cd02 (Core\TSA.c)
void TSA_SetASABluetoothConnectStatus(undefined4 param_1);
// TSA_SetASABluetoothPenStatus @ 0x6ba9cc2d (Core\TSA.c)
void TSA_SetASABluetoothPenStatus(undefined1 param_1);
// TSA_SetASABluetoothPressure @ 0x6ba9cc65 (Core\TSA.c)
void TSA_SetASABluetoothPressure(undefined2 param_1,undefined2 param_2,undefined2 param_3,undefined2 param_4);
// ASA_SetBluetoothStatus @ 0x6bac72d3 (Rejection\ASA.c)
void ASA_SetBluetoothStatus(uint param_1);
// ASA_SetBluetoothPenStatus @ 0x6bac7135 (Rejection\ASA.c)
void ASA_SetBluetoothPenStatus(undefined1 param_1);
// ASA_SetBluetoothPressure @ 0x6bac714d (Rejection\ASA.c)
void ASA_SetBluetoothPressure(ushort param_1,ushort param_2,ushort param_3,ushort param_4);
// ASA_IsHpp3TouchEnableFeatureEnabled @ 0x6bac8a70 (Rejection\ASA.c)
bool ASA_IsHpp3TouchEnableFeatureEnabled(void);
// ASA_IsTouchNull @ 0x6bac704c (Rejection\ASA.c)
undefined8 ASA_IsTouchNull(longlong param_1);
// ASA_NeedRecordTrigger @ 0x6bac7780 (Rejection\ASA.c)
bool ASA_NeedRecordTrigger(void);
// ValidJudgment @ 0x6bac4efc (Rejection\ASA.c)
bool ValidJudgment(stylusFrame *pStylusFrame);
// GetRealtime @ 0x6bac8400 (Rejection\ASA.c)
ulonglong GetRealtime(void);
// ASAStaticCounterClean @ 0x6bac8414 (Rejection\ASA.c)
void ASAStaticCounterClean(void);
// ASAStaticPreProcess @ 0x6bac89f0 (Rejection\ASA.c)
void ASAStaticPreProcess(void);
// ASAStaticPostProcess @ 0x6bac8a04 (Rejection\ASA.c)
void ASAStaticPostProcess(void);
// ASAStaticStatusPreProcess @ 0x6bac883d (Rejection\ASA.c)
void ASAStaticStatusPreProcess(uint auxStatusFlags);
// EnterActiveStylusMode @ 0x6bac858b (Rejection\ASA.c)
void EnterActiveStylusMode(void);
// ExitActiveStylusMode @ 0x6bac87b1 (Rejection\ASA.c)
void ExitActiveStylusMode(void);
// ASAOutClean @ 0x6bac30f0 (Rejection\ASA.c)
void ASAOutClean(void);
// HPP3_BtStatusProcess @ 0x6bab778a (Rejection\HPP3.c)
void HPP3_BtStatusProcess(void);
// BtPressureInit @ 0x6babf350 (Rejection\HPP3.c)
void BtPressureInit(void);
// HardwareAnalyzer_ASA_Process @ 0x6baf1bc2 (Signal\HardwareAnalyzer.c)
void HardwareAnalyzer_ASA_Process(void);
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
