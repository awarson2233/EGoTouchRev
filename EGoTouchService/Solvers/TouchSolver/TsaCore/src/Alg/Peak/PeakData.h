#pragma once
// 峰表（Alg/Peak/PeakData.c、PrevPeakData.c 的数据）。

#include <cstddef>
#include <cstdint>

#include "Common/Bss.h"
#include "Common/Types.h"

namespace Solvers::Tsa::Core {

/// 峰表里的一个峰。
struct PeakEntry {
    uint32_t flags;
    uint8_t id;
    uint8_t dim1;  // 所在列
    uint8_t dim2;  // 所在行
    uint8_t neighborCount;
    int16_t sig;  // 峰中心的 Dif（Z1）
    uint8_t reserved00A[0x2];
    int32_t z8;  // 合法邻居的信号和，不含中心
    int32_t zn;  // 含中心的 2×2 象限和与十字五点和中的最大值
};
static_assert(sizeof(PeakEntry) == 0x014);
static_assert(offsetof(PeakEntry, flags) == 0x000);
static_assert(offsetof(PeakEntry, id) == 0x004);
static_assert(offsetof(PeakEntry, dim1) == 0x005);
static_assert(offsetof(PeakEntry, dim2) == 0x006);
static_assert(offsetof(PeakEntry, neighborCount) == 0x007);
static_assert(offsetof(PeakEntry, sig) == 0x008);
static_assert(offsetof(PeakEntry, z8) == 0x00C);
static_assert(offsetof(PeakEntry, zn) == 0x010);

/// 当前帧峰表（g_peaksGrid / g_peaksHand / g_peaksSide，经 g_peaksPtr 选用）。
struct PeakTable {
    uint8_t count;
    uint8_t innerCount;
    int16_t maxSig;
    int16_t minSig;
    uint8_t reserved006[0x2];
    int32_t maxZn;
    uint8_t minPeakDim2;
    uint8_t reserved00D[0x3];
    PeakEntry entry[kMaxPeakNum];
};
static_assert(sizeof(PeakTable) == 0x1A0);
static_assert(offsetof(PeakTable, count) == 0x000);
static_assert(offsetof(PeakTable, innerCount) == 0x001);
static_assert(offsetof(PeakTable, maxSig) == 0x002);
static_assert(offsetof(PeakTable, minSig) == 0x004);
static_assert(offsetof(PeakTable, maxZn) == 0x008);
static_assert(offsetof(PeakTable, minPeakDim2) == 0x00C);
static_assert(offsetof(PeakTable, entry) == 0x010);

/// 上一帧峰表里的一个峰。
struct PrevPeakEntry {
    uint32_t flags;
    int32_t zn;
    int16_t sig;
    uint8_t dim1;
    uint8_t dim2;
};
static_assert(sizeof(PrevPeakEntry) == 0x00C);
static_assert(offsetof(PrevPeakEntry, flags) == 0x000);
static_assert(offsetof(PrevPeakEntry, zn) == 0x004);
static_assert(offsetof(PrevPeakEntry, sig) == 0x008);
static_assert(offsetof(PrevPeakEntry, dim1) == 0x00A);
static_assert(offsetof(PrevPeakEntry, dim2) == 0x00B);

/// 上一帧峰表（g_prevPeaksGrid / Hand / Side，经 g_prevPeaksPtr 选用）。
struct PrevPeakTable {
    uint8_t count;
    uint8_t reserved001[0x1];
    int16_t maxSig;
    PrevPeakEntry entry[kMaxPeakNum];
};
static_assert(sizeof(PrevPeakTable) == 0x0F4);
static_assert(offsetof(PrevPeakTable, count) == 0x000);
static_assert(offsetof(PrevPeakTable, maxSig) == 0x002);
static_assert(offsetof(PrevPeakTable, entry) == 0x004);

/// 当前在用的峰表（g_peaksPtr 所指）。
extern PeakTable* g_peaks;  // 当前数据集（App/DataSwitch.cpp）
[[nodiscard]] inline PeakTable& Peaks() noexcept { return *g_peaks; }
/// g_peaksGrid：Grid 的峰表（TZ 在上面标握持峰）。
[[nodiscard]] inline PeakTable& PeaksGrid() noexcept { return Slot<PeakTable>(Bss().peaksGrid); }
/// 上一帧峰表（g_prevPeaksPtr 所指）。
extern PrevPeakTable* g_prevPeaks;
[[nodiscard]] inline PrevPeakTable& PrevPeaks() noexcept { return *g_prevPeaks; }

}  // namespace Solvers::Tsa::Core
