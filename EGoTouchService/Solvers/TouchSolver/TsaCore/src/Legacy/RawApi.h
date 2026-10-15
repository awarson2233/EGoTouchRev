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

// Rejection/HandGesture.c
uint64_t HandGesture_Malloc(void);
void HandGesture_Free(void);

// Alg/Exception/Exception.cpp（被裁掉的充电噪声检测，恒为 0）
uint64_t Exception_GetDetectedChargerNoise(void);

// Common/TSAState.c
void TSAState_OnAlloc(void* ptr, uint32_t size);
void TSAState_OnFree(void* ptr);
// Common/MISC.c（传感器间距映射）
int SensorPitchSizeMapDim1(int sensorCoord, uint16_t scale);
int SensorPitchSizeMapDim2(int sensorCoord, uint16_t scale);
// Rejection/ASA.c、ASAHostBridge.c
uint16_t ASA_GetDim1Res(void);
uint16_t ASA_GetDim2Res(void);
bool ASA_GetRptInRange(void);
uint16_t ASA_GetRptPressure(void);
uint16_t ASA_GetRptXPos(void);
int16_t ASA_GetRptXTilt(void);
uint16_t ASA_GetRptYPos(void);
int16_t ASA_GetRptYTilt(void);
uint64_t ASA_InitAFEProject(uint64_t caps, uint64_t flashAsa);
uint64_t ASA_InitProjectFromTsa(uint64_t flash);
bool ASA_IsEnterStylusHPP2(void);
uint64_t ASA_IsEnterStylusHPP3(void);
void TSA_ASAProcess(uint32_t ctrl);
// Rejection/StylusAft.c
void StylusAft_Init(void);
void StylusAft_PostProcessUnit(uint8_t idx);
// Rejection/TouchReport.c
void TouchReport_Init(void);
void TouchReport_Process(void);
// Rejection/HandGesture.c
void HandGesture_Init(void);
void HandGesture_Process(void);
// Rejection/AntiTouch.c
void AntiTouch_Init(void);
void AntiTouch_Process(void);
void AntiTouch_Report(void);

// Rejection/GripFilter.c
void GripFilter_Init(void);
uint8_t GripFilter_IsOKToEnterIDLE(void);
void GripFilter_Process(void);
void GripFilter_RegionProcess(void);
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
void Thenar_Init(void);
void Thenar_ProcessInfoByDif(void);
void Thenar_UnitCopy(uint8_t* dst, uint8_t* src);
bool Thenar_UpdateExternalInfo(void);

}  // extern "C"
