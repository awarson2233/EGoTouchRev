#pragma once
// ============================================================================
// TSAPrunedStubs.h
// Ghidra Decompiler Primitive Types, Bit-Concatenation Macros, Feature Flags,
// and Inline No-Op / Pass-Through Stubs for Pruned Modules on W273AS2700
// ============================================================================
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cmath>

// ----------------------------------------------------------------------------
// 1. Ghidra Decompiler Primitive Typedefs
// ----------------------------------------------------------------------------
using undefined  = uint8_t;
using undefined1 = uint8_t;
using undefined2 = uint16_t;
using undefined3 = uint32_t;
using undefined4 = uint32_t;
using undefined5 = uint64_t;
using undefined6 = uint64_t;
using undefined7 = uint64_t;
using undefined8 = uint64_t;
using uchar      = uint8_t;
using ushort     = uint16_t;
using uint       = uint32_t;
using ulong      = uint32_t;
using longlong   = int64_t;
using ulonglong  = uint64_t;
using byte       = uint8_t;
using word       = uint16_t;
using dword      = uint32_t;
using qword      = uint64_t;
using int3       = int32_t;
using int5       = int64_t;
using int6       = int64_t;
using int7       = int64_t;
using uint3      = uint32_t;
using uint5      = uint64_t;
using uint6      = uint64_t;
using uint7      = uint64_t;
using float10    = long double;
using unkbyte10  = long double;
using code       = void(*)(void);
using TSA_FeatureFlags = uint32_t;
using TSA_SystemFlags  = uint32_t;
#ifndef _ERRNO_T_DEFINED
#define _ERRNO_T_DEFINED
using errno_t = int;
#endif

// ----------------------------------------------------------------------------
// 2. Ghidra Bit-Concatenation & Truncation Intrinsics + Math Helpers
// ----------------------------------------------------------------------------
#define CONCAT11(a, b) ((((uint16_t)(uint8_t)(a)) << 8)  | ((uint16_t)(uint8_t)(b)))
#define CONCAT12(a, b) ((((uint32_t)(uint8_t)(a)) << 16) | ((uint32_t)(uint16_t)(b)))
#define CONCAT13(a, b) ((((uint32_t)(uint8_t)(a)) << 24) | ((uint32_t)(b) & 0x00FFFFFFu))
#define CONCAT21(a, b) ((((uint32_t)(uint16_t)(a)) << 8) | ((uint32_t)(uint8_t)(b)))
#define CONCAT22(a, b) ((((uint32_t)(uint16_t)(a)) << 16)| ((uint32_t)(uint16_t)(b)))
#define CONCAT31(a, b) ((((uint32_t)(a)) << 8)           | ((uint32_t)(uint8_t)(b)))
#define CONCAT44(a, b) ((((uint64_t)(uint32_t)(a)) << 32)| ((uint64_t)(uint32_t)(b)))
#define CONCAT53(a, b) ((((uint64_t)(a)) << 24)          | ((uint64_t)(b) & 0x00FFFFFFULL))
#define CONCAT62(a, b) ((((uint64_t)(a)) << 16)          | ((uint64_t)(uint16_t)(b)))
#define CONCAT71(a, b) ((((uint64_t)(a)) << 8)           | ((uint64_t)(uint8_t)(b)))
#define SUB81(x, off)  ((uint8_t)(((uint64_t)(x)) >> ((off) * 8)))
#define SUB82(x, off)  ((uint16_t)(((uint64_t)(x)) >> ((off) * 8)))
#define SUB84(x, off)  ((uint32_t)(((uint64_t)(x)) >> ((off) * 8)))
#define SUB41(x, off)  ((uint8_t)(((uint32_t)(x)) >> ((off) * 8)))
#define SUB42(x, off)  ((uint16_t)(((uint32_t)(x)) >> ((off) * 8)))
#define ZEXT12(x)      ((uint16_t)(uint8_t)(x))
#define ZEXT14(x)      ((uint32_t)(uint8_t)(x))
#define ZEXT18(x)      ((uint64_t)(uint8_t)(x))
#define ZEXT24(x)      ((uint32_t)(uint16_t)(x))
#define ZEXT28(x)      ((uint64_t)(uint16_t)(x))
#define ZEXT48(x)      ((uint64_t)(uint32_t)(x))
#define SEXT12(x)      ((int16_t)(int8_t)(x))
#define SEXT14(x)      ((int32_t)(int8_t)(x))
#define SEXT24(x)      ((int32_t)(int16_t)(x))
#define SEXT48(x)      ((int64_t)(int32_t)(x))
#ifndef MIN
#define MIN(a, b)      (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b)      (((a) > (b)) ? (a) : (b))
#endif
#ifndef ABS
#define ABS(x)         (((x) < 0) ? -(x) : (x))
#endif

// ----------------------------------------------------------------------------
// 3. W273AS2700 Feature Flag Constants (dwFeatureFlags = 0x01090121)
// ----------------------------------------------------------------------------
enum TSA_FeatureFlagBits : uint32_t {
    FEAT_GLOVE_MODE            = 0x00000001u,
    FEAT_SMART_COVER           = 0x00000002u,
    FEAT_CHARGER_BIT2          = 0x00000004u,
    FEAT_KNUCKLE               = 0x00000010u,
    FEAT_GAME_MODE_BASE        = 0x00000020u,
    FEAT_STYLUS_AFT            = 0x00000100u,
    FEAT_UNDERWATER_DETECT     = 0x00000800u,
    FEAT_PROXIMITY             = 0x00004000u,
    FEAT_SIDE_TOUCH            = 0x00008000u,
    FEAT_ASA_ENABLED           = 0x00010000u,
    FEAT_SIGNAL_DISPARITY_BASE = 0x00080000u,
    FEAT_SAFE_BASELINE         = 0x00100000u,
    FEAT_AFT_ENABLED           = 0x00200000u,
    FEAT_SIGNAL_DISPARITY_POST = 0x00400000u,
    FEAT_PAD_FEATURE           = 0x00800000u,
    FEAT_WINDOWS_PAD           = 0x01000000u,
    FLAG_SKIP_MS_PROCESS       = 0x00000200u,
    FLAG_USE_DIF_BUF_PATH      = 0x00010000u,
    FLAG_PROCESSING_ACTIVE     = 0x40000000u,
};

// ----------------------------------------------------------------------------
// 4. Huawei SecureC Inline Wrappers & StrPrintf No-Op Stub
// ----------------------------------------------------------------------------
static inline errno_t memset_s(void* dest, size_t destMax, int c, size_t count) {
    if (!dest || count > destMax) return -1;
    std::memset(dest, c, count);
    return 0;
}
static inline errno_t memcpy_s(void* dest, size_t destMax, const void* src, size_t count) {
    if (!dest || !src || count > destMax) return -1;
    std::memcpy(dest, src, count);
    return 0;
}
static inline errno_t memmove_s(void* dest, size_t destMax, const void* src, size_t count) {
    if (!dest || !src || count > destMax) return -1;
    std::memmove(dest, src, count);
    return 0;
}
static inline errno_t strcpy_s(char* dest, size_t destMax, const char* src) {
    if (!dest || !src || destMax == 0) return -1;
    std::strncpy(dest, src, destMax);
    dest[destMax - 1] = '\0';
    return 0;
}
static inline errno_t strncpy_s(char* dest, size_t destMax, const char* src, size_t count) {
    if (!dest || !src || destMax == 0) return -1;
    size_t n = (count < destMax - 1) ? count : (destMax - 1);
    std::strncpy(dest, src, n);
    dest[n] = '\0';
    return 0;
}
static inline void StrPrintf_Process(int, const char*, ...) {}

// ----------------------------------------------------------------------------
// 5. Pruned Module Inline Stubs (162 functions called from active code)
// ----------------------------------------------------------------------------
static inline void AFT_ParameterAdjust(int param_1) {  } // pruned (AFT.c)
static inline void AFT_Process() {  } // pruned (AFT.c)
static inline void AFT_ReinitPalmEdge() {  } // pruned (AFT.c)
static inline void BaliSensorBroken_Process() {  } // pruned (BaliSensorBroken.c)
static inline void BaliSensorBroken_Reset() {  } // pruned (BaliSensorBroken.c)
static inline undefined1 BaliSynaFM_IsFastMoveDetected() { return (undefined1)0; } // pruned (BaliSynaFM.c)
static inline void BaselineQueue_Reset() {  } // pruned (BaselineQueue.c)
static inline void BaselineQueue_SwitchToNormal() {  } // pruned (BaselineQueue.c)
static inline void BaselineQueue_SwitchToTemp() {  } // pruned (BaselineQueue.c)
static inline void BigData_ClearTagData(byte param_1) {  } // pruned (BigData.c)
static inline undefined * BigData_GetReport() { return (undefined *)0; } // pruned (BigData.c)
static inline undefined1 BigData_IsReportReady() { return (undefined1)0; } // pruned (BigData.c)
static inline void BigData_RecordProcess() {  } // pruned (BigData.c)
static inline void BigData_ReportProcess() {  } // pruned (BigData.c)
static inline void BigData_StatProcess() {  } // pruned (BigData.c)
static inline void CMFRemap_Enter(undefined1 *param_1,undefined1 *param_2) {  } // pruned (CMFRemap.c)
static inline void CMFRemap_Exit() {  } // pruned (CMFRemap.c)
static inline void CMFRemap_GenerateIndexTable() {  } // pruned (CMFRemap.c)
static inline void CMFRemap_GenerateIndexTableForDoubleIC(longlong param_1,uint param_2,uint param_3) {  } // pruned (CMFRemap.c)
static inline int CMFRemap_GetRxSlotNum(longlong param_1,uint param_2,uint *param_3,longlong param_4,uint param_5) { return (int)0; } // pruned (CMFRemap.c)
static inline int CMFRemap_GetTxSlotNum(longlong param_1,uint param_2) { return (int)0; } // pruned (CMFRemap.c)
static inline undefined8 CMFRemap_IsNeedRemap() { return (undefined8)0; } // pruned (CMFRemap.c)
static inline undefined8 CMFRemap_IsReshape() { return (undefined8)0; } // pruned (CMFRemap.c)
static inline void CMFRemap_MapBuffer(undefined2 *param_1,undefined8 param_2,longlong param_3,longlong param_4) {  } // pruned (CMFRemap.c)
static inline void CMFRemap_MapBufferBack(undefined2 *param_1,undefined8 param_2,longlong param_3,undefined8 param_4, longlong param_5) {  } // pruned (CMFRemap.c)
static inline int CMFRemap_Mapping(int param_1,int param_2,int param_3,longlong param_4,byte param_5) { return (int)0; } // pruned (CMFRemap.c)
static inline bool DoubleReport_Process() { return (bool)0; } // pruned (DoubleReport.c)
static inline undefined8 DoubleReport_Scenario() { return (undefined8)0; } // pruned (DoubleReport.c)
static inline void EdgePeakFilter_WorkAround() {  } // pruned (EdgePeakFilter.c)
static inline undefined4 EventAnalyzer_GetCurValue() { return (undefined4)0; } // pruned (EventAnalyzer.c)
static inline undefined8 EventAnalyzer_Process(uint param_1,undefined4 param_2,undefined4 param_3,undefined2 param_4,undefined2 param_5, undefined2 param_6,undefined2 param_7,undefined8 param_8,undefined8 param_9, undefined4 param_10,undefined4 param_11) { return (undefined8)0; } // pruned (EventAnalyzer.c)
static inline void Gesture_Process() {  } // pruned (Gesture.c)
static inline void Gesture_Reset() {  } // pruned (Gesture.c)
static inline void HIException_CheckChargerNoiseInRxLines() {  } // pruned (HIException.c)
static inline void HIException_CheckPanelSD() {  } // pruned (HIException.c)
static inline undefined2 HIException_GetDetectedChargerNoise() { return (undefined2)0; } // pruned (HIException.c)
static inline undefined4 HIException_IsChargerNoiseDetected() { return (undefined4)0; } // pruned (HIException.c)
static inline void HIException_ProcessTouchZone(short *param_1) {  } // pruned (HIException.c)
static inline void Matrix_Add(undefined8 param_1,longlong param_2,undefined8 param_3) {  } // pruned (Matrix.c)
static inline undefined8 Matrix_Eig(longlong param_1,undefined8 param_2,undefined8 param_3) { return (undefined8)0; } // pruned (Matrix.c)
static inline undefined8 Matrix_Inv(undefined8 param_1,longlong param_2) { return (undefined8)0; } // pruned (Matrix.c)
static inline void Matrix_Mul(undefined8 param_1,longlong param_2,longlong param_3) {  } // pruned (Matrix.c)
static inline void Matrix_Mul_C(longlong param_1,undefined8 param_2,float param_3) {  } // pruned (Matrix.c)
static inline void Matrix_ReShape(longlong param_1,undefined1 param_2,undefined1 param_3) {  } // pruned (Matrix.c)
static inline void Matrix_Rotate(longlong param_1,longlong param_2) {  } // pruned (Matrix.c)
static inline void Matrix_SymmetricFromUpper(longlong param_1) {  } // pruned (Matrix.c)
static inline byte NotchCorrect_GetNotchNum() { return (byte)0; } // pruned (NotchCorrect.c)
static inline undefined8 NotchCorrect_IsNoMoveInCorrection(int param_1,int param_2) { return (undefined8)0; } // pruned (NotchCorrect.c)
static inline void NotchCorrect_Process() {  } // pruned (NotchCorrect.c)
static inline void NotchCorrect_ProcessRaw() {  } // pruned (NotchCorrect.c)
static inline undefined8 NotchProtect_IsNotchBaselineDirty() { return (undefined8)0; } // pruned (NotchProtect.c)
static inline undefined4 Posture_GetPosture() { return (undefined4)0; } // pruned (Posture.c)
static inline undefined8 Posture_IsHangingPosture() { return (undefined8)0; } // pruned (Posture.c)
static inline void Posture_UpdatePostureType() {  } // pruned (Posture.c)
static inline void Proximity_AppendPowerKeyPressedToScreenOnDescription() {  } // pruned (Proximity.c)
static inline void Proximity_AppendScreenOffTimeOutDescription() {  } // pruned (Proximity.c)
static inline void Proximity_AppendScreenOnTimeOutDescription() {  } // pruned (Proximity.c)
static inline uint Proximity_FilterExtraWakeup(uint param_1) { return param_1; } // pruned (Proximity.c)
static inline undefined2 Proximity_GetNextScanRate() { return (undefined2)0; } // pruned (Proximity.c)
static inline undefined1 Proximity_IsDetected() { return (undefined1)0; } // pruned (Proximity.c)
static inline undefined1 Proximity_IsPowerKeyPressedToScreenOn() { return (undefined1)0; } // pruned (Proximity.c)
static inline undefined1 Proximity_IsScreenOffTimeOut() { return (undefined1)0; } // pruned (Proximity.c)
static inline undefined1 Proximity_IsScreenOnTimeOut() { return (undefined1)0; } // pruned (Proximity.c)
static inline bool Proximity_IsToExitIDLE() { return (bool)0; } // pruned (Proximity.c)
static inline undefined1 Proximity_IsValidTouch(byte param_1) { return (undefined1)0; } // pruned (Proximity.c)
static inline void Proximity_Process() {  } // pruned (Proximity.c)
static inline void Proximity_TouchPostProcess() {  } // pruned (Proximity.c)
static inline undefined8 SafeBaseline_GetLatestBuf() { return (undefined8)0; } // pruned (SafeBaseline.c)
static inline undefined8 SafeBaseline_GetSafeBL2BLDif() { return (undefined8)0; } // pruned (SafeBaseline.c)
static inline undefined8 SafeBaseline_GetSafeBL2BLDifCMF() { return (undefined8)0; } // pruned (SafeBaseline.c)
static inline undefined8 SafeBaseline_GetSafeBL2RawDif() { return (undefined8)0; } // pruned (SafeBaseline.c)
static inline undefined8 SafeBaseline_GetSafeBL2RawDifCMF() { return (undefined8)0; } // pruned (SafeBaseline.c)
static inline undefined8 SafeBaseline_IsAvailable() { return (undefined8)0; } // pruned (SafeBaseline.c)
static inline bool SafeBaseline_IsReset() { return (bool)0; } // pruned (SafeBaseline.c)
static inline bool SafeBaseline_IsResetTriggered() { return (bool)0; } // pruned (SafeBaseline.c)
static inline bool SafeBaseline_IsSignalStable() { return (bool)0; } // pruned (SafeBaseline.c)
static inline void SafeBaseline_Process() {  } // pruned (SafeBaseline.c)
static inline uint SafeBaseline_ScreenOnWithPhoneHeldInHandProcess() { return (uint)0; } // pruned (SafeBaseline.c)
static inline void SafeBaseline_SetBuf(undefined8 param_1) {  } // pruned (SafeBaseline.c)
static inline void Self_Process() {  } // pruned (Self.c)
static inline void Self_Reset() {  } // pruned (Self.c)
static inline undefined8 SensorData_AntiTouchStageTimeout() { return (undefined8)0; } // pruned (SensorData.c)
static inline undefined8 SensorData_IsAntiTouchStage() { return (undefined8)0; } // pruned (SensorData.c)
static inline void SensorGesture_ClearExternally(uint param_1) {  } // pruned (SensorGesture.c)
static inline undefined8 SensorGesture_IsProximityPickUpGestureTimeOut(uint param_1) { return (undefined8)0; } // pruned (SensorGesture.c)
static inline undefined8 SensorGesture_IsProximityPutDownGestureTimeOut(uint param_1) { return (undefined8)0; } // pruned (SensorGesture.c)
static inline void SHBVolumeData_Process() {  } // pruned (SHBVolumeData.c)
static inline void SHBVolumeData_Reset() {  } // pruned (SHBVolumeData.c)
static inline void SideTouch_AppendAbnormalBaselineDescription() {  } // pruned (SideTouch.c)
static inline void SideTouch_AppendVolumeGUIOffTimeOutDescription() {  } // pruned (SideTouch.c)
static inline void SideTouch_AppendVolumeGUIOnTimeOutDescription() {  } // pruned (SideTouch.c)
static inline void SideTouch_GesturePrmtHandler() {  } // pruned (SideTouch.c)
static inline undefined1 SideTouch_IsAbnormalBaselineDetected() { return (undefined1)0; } // pruned (SideTouch.c)
static inline undefined8 SideTouch_IsMotorControlEnabled() { return (undefined8)0; } // pruned (SideTouch.c)
static inline undefined4 SideTouch_IsOKToEnterIDLEMode() { return (undefined4)0; } // pruned (SideTouch.c)
static inline undefined1 SideTouch_IsPrevTouchDetected() { return (undefined1)0; } // pruned (SideTouch.c)
static inline undefined8 SideTouch_IsVolumeControlEnabled() { return (undefined8)0; } // pruned (SideTouch.c)
static inline undefined1 SideTouch_IsVolumeGUIOffTimeOut() { return (undefined1)0; } // pruned (SideTouch.c)
static inline undefined1 SideTouch_IsVolumeGUIOnTimeOut() { return (undefined1)0; } // pruned (SideTouch.c)
static inline void SideTouch_KeepPrevTouchWithExceptionExcluded() {  } // pruned (SideTouch.c)
static inline undefined4 SideTouch_PressureIsVolumeDown() { return (undefined4)0; } // pruned (SideTouch.c)
static inline void SideTouch_Process() {  } // pruned (SideTouch.c)
static inline void SideTouch_Report() {  } // pruned (SideTouch.c)
static inline undefined8 SideTouch_SetRingTaskName(char *param_1) { return (undefined8)0; } // pruned (SideTouch.c)
static inline undefined1 SideTouch_UpdateExternalThenarInfo() { return (undefined1)0; } // pruned (SideTouch.c)
static inline undefined8 SignalDisparity_IsOKToProcess() { return (undefined8)0; } // pruned (SignalDisparity.c)
static inline uint SignalDisparity_IsSignalCompensated() { return (uint)0; } // pruned (SignalDisparity.c)
static inline void SignalDisparity_PostProcess() {  } // pruned (SignalDisparity.c)
static inline void SignalDisparity_Process() {  } // pruned (SignalDisparity.c)
static inline undefined4 SignalDisparity_SwitchBuffer() { return (undefined4)0; } // pruned (SignalDisparity.c)
static inline int SmartCover_GetPrevTouchNumInOpenWindow() { return (int)0; } // pruned (SmartCover.c)
static inline void SmartCover_Process() {  } // pruned (SmartCover.c)
static inline void SS_ChargerNoiseFilterPostProcess() {  } // pruned (SS.c)
static inline void SS_ChargerNoiseFilterProcess() {  } // pruned (SS.c)
static inline bool SS_ChargerNoiseFilterSwitchBuffer() { return (bool)0; } // pruned (SS.c)
static inline void SS_CheckDirtyByMutual() {  } // pruned (SS.c)
static inline void SS_CopyRaw(ushort *param_1,ushort param_2) {  } // pruned (SS.c)
static inline ulonglong SS_DiffCMFProcessDimUnit(int param_1,byte param_2,byte param_3,byte param_4,longlong param_5) { return (ulonglong)0; } // pruned (SS.c)
static inline longlong SS_DiffRelatCal(byte param_1,int param_2) { return (longlong)0; } // pruned (SS.c)
static inline byte SS_IsDim1Valid() { return (byte)0; } // pruned (SS.c)
static inline byte SS_IsDim2Valid() { return (byte)0; } // pruned (SS.c)
static inline undefined8 SS_IsNoSignal() { return (undefined8)0; } // pruned (SS.c)
static inline undefined8 SS_IsNotMatchWithMutual(char param_1) { return (undefined8)0; } // pruned (SS.c)
static inline undefined1 SS_IsRawValid() { return (undefined1)0; } // pruned (SS.c)
static inline undefined8 SS_IsValidForBaseline() { return (undefined8)0; } // pruned (SS.c)
static inline undefined8 SS_IsValidForCMF() { return (undefined8)0; } // pruned (SS.c)
static inline void SS_Process() {  } // pruned (SS.c)
static inline void SS_Reset() {  } // pruned (SS.c)
static inline byte SS_TouchLength(longlong param_1,byte param_2,int param_3) { return (byte)0; } // pruned (SS.c)
static inline void SS_TouchValid(byte param_1) {  } // pruned (SS.c)
static inline void StrPrintf_Backup() {  } // pruned (StrPrintf.c)
static inline void StrPrintf_Retore() {  } // pruned (StrPrintf.c)
static inline void Thenar_ProcessInfoByDif() {  } // pruned (Thenar.c)
static inline void Thenar_UnitCopy(undefined1 *param_1,undefined1 *param_2) {  } // pruned (Thenar.c)
static inline bool Thenar_UpdateExternalInfo() { return (bool)0; } // pruned (Thenar.c)
static inline ushort TSAFold_Dim1PosOutRemap(ushort param_1,ushort param_2) { return param_1; } // pruned (TSAFold.c)
static inline ushort TSAFold_Dim2PosOutRemap(ushort param_1,ushort param_2) { return param_1; } // pruned (TSAFold.c)
static inline undefined4 TSAFold_GetCurrState() { return (undefined4)0; } // pruned (TSAFold.c)
static inline undefined8 TSAFold_GetTimeStampRegionChange() { return (undefined8)0; } // pruned (TSAFold.c)
static inline void TSAFold_InputProcess(undefined8 param_1,uint *param_2,longlong param_3) {  } // pruned (TSAFold.c)
static inline void TSAFold_OutputProcess() {  } // pruned (TSAFold.c)
static inline void TSAFold_PrmtSetRawWithTouch(undefined4 param_1) {  } // pruned (TSAFold.c)
static inline void TSAIDE_CleanPointXY() {  } // pruned (TSAIDE.c)
static inline void TSAIDE_LogPoint(undefined2 param_1,undefined2 param_2,undefined4 param_3) {  } // pruned (TSAIDE.c)
static inline void TSAIDE_LogPreFltGridRpt() {  } // pruned (TSAIDE.c)
static inline void TSAIDE_LogPreFltHandRpt() {  } // pruned (TSAIDE.c)
static inline void TSAIDE_LogSensor(byte param_1,byte param_2,undefined4 param_3) {  } // pruned (TSAIDE.c)
static inline void TSAIDE_Reset() {  } // pruned (TSAIDE.c)
static inline void TSAIDE_SetIndexVolume(byte param_1) {  } // pruned (TSAIDE.c)
static inline void TSALog_Frame() {  } // pruned (TSALog.c)
static inline void TSALog_Input(undefined8 param_1,undefined4 param_2,undefined8 param_3,undefined8 param_4) {  } // pruned (TSALog.c)
static inline void TSALog_Output() {  } // pruned (TSALog.c)
static inline undefined8 TSALog_RtEnable(int param_1,uint param_2,uint param_3) { return (undefined8)0; } // pruned (TSALog.c)
static inline undefined8 UDFPEvent_IsCloseFingerArea(ushort param_1,ushort param_2) { return (undefined8)0; } // pruned (UDFPEvent.c)
static inline undefined8 UDFPEvent_IsEnabled() { return (undefined8)0; } // pruned (UDFPEvent.c)
static inline void UDFPEvent_Process() {  } // pruned (UDFPEvent.c)
static inline ulonglong VirtualKey_GetAction(byte param_1) { return (ulonglong)0; } // pruned (VirtualKey.c)
static inline undefined2 VirtualKey_GetCode(byte param_1) { return (undefined2)0; } // pruned (VirtualKey.c)
static inline undefined1 VirtualKey_GetEventNum() { return (undefined1)0; } // pruned (VirtualKey.c)
