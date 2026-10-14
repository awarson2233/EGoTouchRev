#pragma once
// TZ 的触摸区域单元与区域图访问句柄（Detection/TZ.c 的数据）。

#include <cstddef>
#include <cstdint>

#include "Common/Image.h"

namespace Solvers::Tsa::Core {

/// TZ 的一个触摸区域（TZ_Process 逐个区域填写）。只列已改写的模块用到的字段。
struct TouchZoneUnit {
    int16_t sigThold;  // 区域生长门限；SubTZ 以它为下限
    uint8_t reserved002[0x2];
    uint32_t flags;  // kZone*
    uint8_t reserved008[0x8];
    uint16_t waterCells;  // FIException：沿历史轨迹累计的水膜格数
    uint8_t reserved012[0x2A0];
    uint8_t peakCount;  // 区域内的峰数
    uint8_t reserved2B3[0x3];
    uint8_t maxSigPeakIdx;  // 区域内峰值最大的峰
    uint8_t minSigPeakIdx;  // 区域内峰值最小的峰
    uint8_t reserved2B8[0x1];
    uint8_t peakIdx[20];  // 区域内的峰（峰表下标）
    uint8_t reserved2CD[0x1F];
    uint16_t majorMm;  // 椭圆长轴（mm）
    uint16_t minorMm;
    uint8_t reserved2F0[0x34];
    uint8_t prevIdx;  // 对应的历史触点，kNoTouch 表示没有
    uint8_t reserved325[0x17];
    uint16_t exceptionRatio[6];  // TZ_GetExceptionRatio：[0] 为二阶矩相对零阶矩的离散度
    uint8_t noiseException;  // 区域异常因全局噪声（0x100）而置位
    uint8_t reserved349[0x2];
    uint8_t fatFingerSD;  // TZ_CheckFatFingerSD：本帧有悬浮状态下的大手指
    uint8_t abnormalBaseline;  // 区域判为基线异常（0x200）
    uint8_t reserved34D[0x3];
    uint32_t exceptionType;  // 区域异常字（消抖后）
    uint32_t detectedException;  // 本帧检出的区域异常（消抖前）
    uint8_t reserved358[0x6];
    uint8_t mapTag;  // 本区域在区域图里的编码基数（ZoneMapAccess::tag）
    uint8_t reserved35F[0xD];
    uint8_t innerCornerSigRatio;  // 峰在内角格时，边缘格相对它的信号比（×16）
    uint8_t reserved36D[0x3];
};
static_assert(sizeof(TouchZoneUnit) == 0x370);
static_assert(offsetof(TouchZoneUnit, sigThold) == 0x000);
static_assert(offsetof(TouchZoneUnit, flags) == 0x004);
static_assert(offsetof(TouchZoneUnit, waterCells) == 0x010);
static_assert(offsetof(TouchZoneUnit, peakCount) == 0x2B2);
static_assert(offsetof(TouchZoneUnit, maxSigPeakIdx) == 0x2B6);
static_assert(offsetof(TouchZoneUnit, minSigPeakIdx) == 0x2B7);
static_assert(offsetof(TouchZoneUnit, peakIdx) == 0x2B9);
static_assert(offsetof(TouchZoneUnit, majorMm) == 0x2EC);
static_assert(offsetof(TouchZoneUnit, minorMm) == 0x2EE);
static_assert(offsetof(TouchZoneUnit, prevIdx) == 0x324);
static_assert(offsetof(TouchZoneUnit, exceptionRatio) == 0x33C);
static_assert(offsetof(TouchZoneUnit, noiseException) == 0x348);
static_assert(offsetof(TouchZoneUnit, fatFingerSD) == 0x34B);
static_assert(offsetof(TouchZoneUnit, abnormalBaseline) == 0x34C);
static_assert(offsetof(TouchZoneUnit, exceptionType) == 0x350);
static_assert(offsetof(TouchZoneUnit, detectedException) == 0x354);
static_assert(offsetof(TouchZoneUnit, mapTag) == 0x35E);
static_assert(offsetof(TouchZoneUnit, innerCornerSigRatio) == 0x36C);

/// TouchZoneUnit::flags
inline constexpr uint32_t kZonePrevTouchFlag2 = 0x00000002u;   // 对应的历史触点 +0x200 字段带 bit1
inline constexpr uint32_t kZoneGripSeparated = 0x01000000u;   // 握持峰已移到峰列表末尾，SubTZ 按握持阶段拆分

/// 区域图的访问句柄（g_tzAccess / g_subTZAccess）：TZ 与 SubTZ 各一份，g_ptrAccess 指向正在遍历的那份。
/// 遍历到的格子在 map 里记为 (tag + k) × Dif 符号，k 是 1~7 的格子类别；每完成一个区域 tag 加 8。
struct ZoneMapAccess {
    int8_t* map;
    uint8_t reserved008;
    uint8_t tag;
};
static_assert(sizeof(ZoneMapAccess) == 0x10);
static_assert(offsetof(ZoneMapAccess, tag) == 0x09);

[[nodiscard]] inline ZoneMapAccess& TzAccess() noexcept { return Image::At<ZoneMapAccess>(0x6bbc8960); }
[[nodiscard]] inline ZoneMapAccess& SubTzAccess() noexcept { return Image::At<ZoneMapAccess>(0x6bbc8970); }
/// g_ptrAccess
[[nodiscard]] inline ZoneMapAccess*& CurrentAccess() noexcept { return Image::At<ZoneMapAccess*>(0x6bb80fa0); }

}  // namespace Solvers::Tsa::Core
