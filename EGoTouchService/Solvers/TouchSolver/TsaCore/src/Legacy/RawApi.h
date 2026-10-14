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

// Core/AFE.c
uint32_t AFE_IsRawJump(void);
uint16_t AFE_GetColNum(void);
uint16_t AFE_GetRowNum(void);

// Core/TSA.c
uint16_t TSA_GetXRes(void);
uint16_t TSA_GetYRes(void);
uint32_t TSA_IsUnderWaterDetected(void);
uint32_t TSA_IsOutOfWaterDetected(void);
char* TSA_GetPrmtName(void);
int16_t TSA_GetMinSigPrpt(void);
uint64_t TSA_IsGameScenario(void);
uint32_t TSA_IsWindowsPadFeatureEnabled(void);
void TSA_ScenarioProcess(void);
uint16_t TSA_SideTouchDim1Center(uint8_t sideIdx);
uint16_t TSA_SideTouchDim2Center(uint8_t sideIdx);
uint16_t TSA_CoreTouchDim1Center(uint8_t idx);
uint16_t TSA_CoreTouchDim2Center(uint8_t idx);
uint32_t TSA_GetDimEdge(uint16_t dim1, uint16_t dim2);
uint64_t TSA_DimEdgeToSide(int dimEdge);
uint64_t TSA_SideToEdge(uint32_t side);
uint64_t TSA_SideToDimEdge(int side);

// Core/TSAOut.c
uint16_t TSAOut_DisplayXPos(uint8_t idx, ...);
uint16_t TSAOut_DisplayYPos(uint8_t idx, ...);
uint16_t TSAOut_Pixel2DisplayDim1(uint16_t pos);
uint16_t TSAOut_Pixel2DisplayDim2(uint16_t pos);
uint32_t TSAOut_RevertDim1Pos(uint16_t pos);
uint32_t TSAOut_RevertDim2Pos(uint16_t pos);
uint64_t TSAOut_ExtToIntDist(uint16_t dist);
char TSAOut_Angle(char angle);

// Core/TSAStatic.c
uint8_t TSAStatic_IsChargerPlugged(void);
uint64_t TSAStatic_IsSafeBaselineApplied(void);
void TSAStatic_ProcessAftTouchEvent(void);
uint8_t TSAStatic_IsAFEJustEnterIDLEMode(void);
uint8_t TSAStatic_IsAFEJustExitIDLEMode(void);

// Baseline/BLReset.c
uint8_t BLReset_CheckAbnormalTouch(uint8_t idx);

// Baseline/PressureDrift.c
uint64_t PressureDrift_Detect(uint8_t dim1, uint8_t dim2);

// Detection/Sensor.c
int Sensor_GetDistance(uint8_t dim1A, uint8_t dim2A, uint8_t dim1B, uint8_t dim2B);
int Sensor_GetSquareDistance(uint8_t dim1A, uint8_t dim2A, uint8_t dim1B, uint8_t dim2B);
uint64_t Sensor_IsInNxNBox(uint8_t dim1, uint8_t dim2, uint8_t centerDim1, uint8_t centerDim2, uint8_t boxSize);
uint64_t Sensor_IsTouchTraversed(uint8_t dim1, uint8_t dim2);
uint64_t Sensor_IsTouchTraversedBefore(uint8_t dim1, uint8_t dim2);
uint64_t Sensor_IsInZone(uint8_t dim1, uint8_t dim2, uint8_t zoneIdx);
uint64_t Sensor_IsInCoreZone(uint8_t dim1, uint8_t dim2, uint8_t zoneIdx);
uint64_t Sensor_IsInSideZone(uint8_t dim1, uint8_t dim2, uint8_t zoneIdx);

// Rejection/AntiTouch.c
void AntiTouch_Report(void);

// Rejection/GripFilter.c
char GripFilter_IsGripOrTbdTouch(uint8_t idx);
bool GripFilter_IsGripTouch(uint8_t idx);
uint64_t GripFilter_IsGripOrTbd(int32_t gripStage);
bool GripFilter_IsPeakAbsGrip(uint8_t peakIdx);
bool GripFilter_IsToCancelIDMapping(uint8_t curIdx, uint8_t prevIdx);

// Rejection/Thenar.c
void Thenar_UnitCopy(uint8_t* dst, uint8_t* src);

// Signal/CMF.c
int16_t CMF_GetNoiseValue(uint8_t dim1, uint8_t dim2);

// Signal/TPSensor.c
char TPSensor_IsNoMoveInCorrection(uint32_t dim1, uint32_t dim2);

// Signal/HardwareAnalyzer.c
void HardwareAnalyzer_ReportToTSA(void);

}  // extern "C"
