#pragma once
// 子触摸区域（原 Detection/SubTZ.c，原厂 src/Alg/TouchZone/SubTouchZone）：TZ 得到一个区域后，以区域内
// 每个尚未被覆盖的峰为种子再做一遍区域生长，拆出若干子区域，各自统计信号矩、椭圆轴与包含的峰。
// TZ 用子区域判定多指（IsMultiFinger）与握持拆分，并按偏移直接读子区域表。

#include <cstddef>
#include <cstdint>

#include "Common/Image.h"

namespace Solvers::Tsa::Core {
struct TouchZoneUnit;
}

namespace Solvers::Tsa::Core::SubTouchZone {

inline constexpr uint8_t kMaxUnitNum = 10;

/// Unit::flags：Filter 判为无效的原因
inline constexpr uint32_t kWeakPeak = 0x1;  // 最大信号不超过区域最强峰的一半
inline constexpr uint32_t kWeakSum = 0x2;   // 信号和不超过 Z9 最大的子区域的 1/8

/// 一个子区域。矩按格子坐标 (dim1, dim2) 以信号加权累计。
struct Unit {
    int16_t maxSig;          // 达到遍历门限的格子中的最大信号
    uint8_t peakIdx;         // 区域内峰值最大的峰
    uint8_t peakCount;       // 区域内的峰数
    uint16_t cellCount;      // 达到遍历门限的格子数
    uint16_t centroidDim1;   // Q8.8
    uint16_t centroidDim2;
    uint8_t reserved00A[0x2];
    int32_t sigSum;          // 零阶矩
    int32_t momentDim1;      // 一阶矩
    int32_t momentDim2;
    int64_t momentDim1Dim2;  // 二阶矩
    int64_t momentDim1Sq;
    int64_t momentDim2Sq;
    uint16_t majorMm;        // 椭圆长轴（mm）
    uint16_t minorMm;
    uint16_t majorAxis;      // 椭圆长轴（格，Q8.8）
    uint16_t minorAxis;
    int8_t angle;            // 长轴方向（度）
    uint8_t minDim1;         // 外接框
    uint8_t maxDim1;
    uint8_t minDim2;
    uint8_t maxDim2;
    uint8_t gripRegion;      // 覆盖到的格子里最大的握持区域编号
    uint8_t reserved03E[0x2];
    uint32_t flags;          // kWeak*
    uint8_t reserved044[0x4];
};
static_assert(sizeof(Unit) == 0x48);
static_assert(offsetof(Unit, sigSum) == 0x0C);
static_assert(offsetof(Unit, momentDim1Dim2) == 0x18);
static_assert(offsetof(Unit, majorMm) == 0x30);
static_assert(offsetof(Unit, angle) == 0x38);
static_assert(offsetof(Unit, gripRegion) == 0x3D);
static_assert(offsetof(Unit, flags) == 0x40);

/// g_subTZ：表头 + 子区域。
struct Table {
    uint8_t count;
    uint8_t numAftFilter;        // Filter 后仍有效的子区域数
    uint8_t maxZ9Idx;            // 最大峰 Z9 最大的子区域
    uint8_t maxCentToMinPkRatio; // 其余有效子区域相对 maxZ9Idx 的质心到最小峰信号比的最大值
    uint16_t maxCellCount;
    uint16_t maxCellIdx;         // 格子最多的子区域
    Unit unit[kMaxUnitNum];
};
static_assert(sizeof(Table) == 0x2D8);
static_assert(offsetof(Table, unit) == 0x08);

[[nodiscard]] inline Table& Global() noexcept { return Image::At<Table>(0x6bbc8680); }
/// 第 i 个子区域。原代码不检查下标是否超过 kMaxUnitNum，这里按地址取，越界时的内存行为与原代码一致。
[[nodiscard]] inline Unit& UnitAt(uint32_t i) noexcept { return Image::At<Unit>(0x6bbc8688 + i * sizeof(Unit)); }
/// g_pkAccessedFlagSubTZ：已归入某个子区域的峰（峰表下标的位图），TZ 每帧清零。
[[nodiscard]] inline uint32_t& PeakAccessed() noexcept { return Image::At<uint32_t>(0x6bbc8958); }

/// 对一个 TZ 区域拆分子区域，结果写入 Global()。
void Process(TouchZoneUnit& zone);
/// 有效子区域多于一个，且它们与最强子区域之间信号明显下凹，或与上一帧触点数相同且有子区域足够大。
[[nodiscard]] bool IsMultiFinger();
[[nodiscard]] uint8_t GetNumAftFilter();

}  // namespace Solvers::Tsa::Core::SubTouchZone
