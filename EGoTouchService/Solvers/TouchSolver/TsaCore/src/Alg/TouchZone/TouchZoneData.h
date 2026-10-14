#pragma once
// TZ 的触摸区域单元（Detection/TZ.c 的数据）。

#include <cstddef>
#include <cstdint>

namespace Solvers::Tsa::Core {

/// TZ 的一个触摸区域（TZ_Process 逐个区域填写）。只列已改写的模块用到的字段。
struct TouchZoneUnit {
    uint8_t reserved000[0x4];
    uint32_t flags;
    uint8_t reserved008[0x8];
    uint16_t waterCells;  // FIException：沿历史轨迹累计的水膜格数
    uint8_t reserved012[0x2A0];
    uint8_t peakCount;  // 区域内的峰数
    uint8_t reserved2B3[0x39];
    uint16_t majorMm;  // 椭圆长轴（mm）
    uint16_t minorMm;
    uint8_t reserved2F0[0x34];
    uint8_t prevIdx;  // 对应的历史触点，kNoTouch 表示没有
    uint8_t reserved325[0x17];
    uint16_t exceptionRatio[6];  // TZ_GetExceptionRatio：[0] 为二阶矩相对零阶矩的离散度
    uint8_t noiseException;  // 区域异常因全局噪声（0x100）而置位
    uint8_t reserved349[0x3];
    uint8_t abnormalBaseline;  // 区域判为基线异常（0x200）
    uint8_t reserved34D[0x3];
    uint32_t exceptionType;  // 区域异常字（消抖后）
    uint32_t detectedException;  // 本帧检出的区域异常（消抖前）
    uint8_t reserved358[0x14];
    uint8_t innerCornerSigRatio;  // 峰在内角格时，边缘格相对它的信号比（×16）
    uint8_t reserved36D[0x3];
};
static_assert(sizeof(TouchZoneUnit) == 0x370);
static_assert(offsetof(TouchZoneUnit, flags) == 0x004);
static_assert(offsetof(TouchZoneUnit, waterCells) == 0x010);
static_assert(offsetof(TouchZoneUnit, peakCount) == 0x2B2);
static_assert(offsetof(TouchZoneUnit, majorMm) == 0x2EC);
static_assert(offsetof(TouchZoneUnit, minorMm) == 0x2EE);
static_assert(offsetof(TouchZoneUnit, prevIdx) == 0x324);
static_assert(offsetof(TouchZoneUnit, exceptionRatio) == 0x33C);
static_assert(offsetof(TouchZoneUnit, noiseException) == 0x348);
static_assert(offsetof(TouchZoneUnit, abnormalBaseline) == 0x34C);
static_assert(offsetof(TouchZoneUnit, exceptionType) == 0x350);
static_assert(offsetof(TouchZoneUnit, detectedException) == 0x354);
static_assert(offsetof(TouchZoneUnit, innerCornerSigRatio) == 0x36C);

}  // namespace Solvers::Tsa::Core