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

// Core/AFE.c
uint32_t AFE_IsRawJump(void);

// Core/TSA.c
uint16_t TSA_GetXRes(void);
uint16_t TSA_GetYRes(void);
uint32_t TSA_IsUnderWaterDetected(void);
uint32_t TSA_IsOutOfWaterDetected(void);
char* TSA_GetPrmtName(void);
uint16_t TSA_SideTouchDim1Center(uint8_t sideIdx);
uint16_t TSA_SideTouchDim2Center(uint8_t sideIdx);
uint16_t TSA_CoreTouchDim1Center(uint8_t idx);
uint16_t TSA_CoreTouchDim2Center(uint8_t idx);

// Core/TSAOut.c
uint16_t TSAOut_DisplayXPos(uint8_t idx, ...);
uint16_t TSAOut_DisplayYPos(uint8_t idx, ...);
uint16_t TSAOut_Pixel2DisplayDim1(uint16_t pos);
uint16_t TSAOut_Pixel2DisplayDim2(uint16_t pos);
uint64_t TSAOut_ExtToIntDist(uint16_t dist);
char TSAOut_Angle(char angle);

// Core/TSAStatic.c
uint8_t TSAStatic_IsChargerPlugged(void);
uint8_t TSAStatic_IsAFEJustEnterIDLEMode(void);
uint8_t TSAStatic_IsAFEJustExitIDLEMode(void);

// Baseline/BLReset.c
uint8_t BLReset_CheckAbnormalTouch(uint8_t idx);

// Detection/Peak.c
bool Peak_IsOnInnerEdgeDim1Near(uint8_t peakIdx);
uint32_t Peak_IsOnInnerEdgeDim1Far(uint8_t peakIdx);
bool Peak_IsOnInnerEdgeDim2Near(uint8_t peakIdx);
uint32_t Peak_IsOnInnerEdgeDim2Far(uint8_t peakIdx);
bool Peak_IsOnOuterEdgeDim1Near(uint8_t peakIdx);
uint32_t Peak_IsOnOuterEdgeDim1Far(uint8_t peakIdx);
bool Peak_IsOnOuterEdgeDim2Near(uint8_t peakIdx);
uint32_t Peak_IsOnOuterEdgeDim2Far(uint8_t peakIdx);
uint32_t Peak_IsTouchValid(uint8_t idx);

// Detection/PrevPeak.c
uint64_t PrevPeak_IsOnOuterEdge(uint8_t peakIdx);

// Detection/Sensor.c
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

// Rejection/Thenar.c
void Thenar_UnitCopy(uint8_t* dst, uint8_t* src);

// Signal/HardwareAnalyzer.c
void HardwareAnalyzer_ReportToTSA(void);

// Tracking/TS.c
uint64_t TS_SQPixelToSQMM(uint32_t sqPixel);
uint32_t TS_MMToDisplayPixel(uint8_t mm);
int32_t TS_MMToPixel(uint8_t mm);
uint64_t TS_SQMMToSQPixel(uint16_t sqMm);

}  // extern "C"
