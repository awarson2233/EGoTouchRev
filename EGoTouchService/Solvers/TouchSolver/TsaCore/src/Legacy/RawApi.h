#pragma once
// ══════════════════════════════════════════════════════════════════════
// 过渡期：已改写的代码调用的、尚未改写的 raw 函数。
//
// 签名与 raw/include/TSAFuncDecls.h 的声明逐一对应（Ghidra 的 undefinedN / byte / ushort 写成同宽的
// 定宽整数，返回类型保持原样，因为 raw 侧按它编译）。调用方按 raw 的方式使用返回值：raw 把返回值截断
// 成 char 再判断的，这里同样截断。某个模块改写完成后，从这里删掉它的条目。
// ══════════════════════════════════════════════════════════════════════

#include <cstdint>

extern "C" {

// Common/TSABssMemoryImage.c
void TSACoreRaw_LoadProjectTables(void);

// Common/MISC.c
int Misc_SqrtUint32(uint32_t value);
double SinXDivX(double x);

// Core/AFE.c
uint64_t AFE_Malloc(void);
uint8_t AFE_IsAllFreqNoisy(void);
char AFE_IsFreqShiftDone(void);
bool AFE_IsInIDLEMode(void);
uint32_t AFE_IsInProximityScreenOffState(void);
uint32_t AFE_IsInSideIDLEMode(void);
uint32_t AFE_IsRawUnstable(void);
uint32_t AFE_IsRawJump(void);
uint16_t AFE_GetColNum(void);
uint16_t AFE_GetRowNum(void);
uint32_t AFE_IsManualFreqHopping(void);
bool AFE_IsRawUnifiedToTargetValue(void);
bool AFE_IsAlwaysNoisy(void);
uint32_t AFE_IsAutoCalibration(void);
bool AFE_IsRecalRequested(void);
uint64_t AFE_IsCoreTouchDetected(void);
uint8_t AFE_IsFrequentlyHopping(void);
bool AFE_IsIndividualBaselineForEachFreq(void);

// Core/TSAPrpt.c
bool TSAPrpt_Malloc(void);
void TSAPrpt_Free(void);

// Rejection/HandGesture.c
uint64_t HandGesture_Malloc(void);
void HandGesture_Free(void);

// Core/TSA.c
uint16_t TSA_GetXRes(void);
uint16_t TSA_GetYRes(void);
uint32_t TSA_IsUnderWaterDetected(void);
uint32_t TSA_IsOutOfWaterDetected(void);
char* TSA_GetPrmtName(void);
int TSA_GetMaxCMFNoiseValue(void);
int16_t TSA_GetMinSigPrpt(void);
uint64_t TSA_IsGameScenario(void);
uint32_t TSA_IsWindowsPadFeatureEnabled(void);
bool TSA_IsTypingScenario(void);
void TSA_ScenarioProcess(void);
uint16_t TSA_SideTouchDim1Center(uint8_t sideIdx);
uint16_t TSA_SideTouchDim2Center(uint8_t sideIdx);
uint16_t TSA_CoreTouchDim1Center(uint8_t idx);
uint16_t TSA_CoreTouchDim2Center(uint8_t idx);
uint32_t TSA_GetDimEdge(uint16_t dim1, uint16_t dim2);
uint64_t TSA_DimEdgeToSide(int dimEdge);
uint64_t TSA_SideToEdge(uint32_t side);
uint64_t TSA_SideToDimEdge(int side);
bool TSA_IsPossibleUnderWater(void);
void* TSA_MallocAndInit(uint32_t size);
void TSA_Free(int64_t* ptr);
uint32_t TSA_GetSide(uint16_t dim1, uint16_t dim2);
uint64_t TSA_InStylusMode(void);
uint32_t TSA_IsPadFeatureEnabled(void);
char TSA_IsGripOKToIDLE(void);
uint8_t TSA_IsOKToEnterIDLEMode(void);
bool TSA_UpdateIDLEModeInternally(void);
bool TSA_UpdateSideIDLEModeInternally(void);
int64_t TSA_GetSensorData(void);
void TSA_SetXPos(uint8_t idx, uint16_t x);




// Rejection/AntiTouch.c
void AntiTouch_Report(void);

// Rejection/GripFilter.c
char GripFilter_IsGripOrTbdTouch(uint8_t idx);
bool GripFilter_IsGripTouch(uint8_t idx);
uint64_t GripFilter_IsGripOrTbd(int32_t gripStage);
bool GripFilter_IsPeakAbsGrip(uint8_t peakIdx);
bool GripFilter_IsToCancelIDMapping(uint8_t curIdx, uint8_t prevIdx);
uint64_t GripFilter_IsGrip(int32_t gripStage);
bool GripFilter_IsPeakAbsNotGrip(uint8_t peakIdx);
uint32_t GripFilter_GetTrackingThold(void);
uint8_t GripFilter_IsMergeHandlerSensorEnabled(uint8_t dim1, uint8_t dim2);

// Rejection/Thenar.c
void Thenar_UnitCopy(uint8_t* dst, uint8_t* src);
bool Thenar_UpdateExternalInfo(void);

}  // extern "C"
