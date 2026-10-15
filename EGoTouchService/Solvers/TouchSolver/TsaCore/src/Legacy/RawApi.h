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



// Alg/Exception/Exception.cpp（被裁掉的充电噪声检测，恒为 0）
uint64_t Exception_GetDetectedChargerNoise(void);


// Rejection/GripFilter.c


}  // extern "C"
