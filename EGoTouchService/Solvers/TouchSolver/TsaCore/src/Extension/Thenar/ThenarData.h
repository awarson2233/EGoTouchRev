#pragma once
// 大鱼际 / 手掌区域（Thenar）的数据。Thenar 模块尚未改写，这里只给出触点表头里那一块的布局，
// 字段含义待改写 Thenar 时确定。

#include <cstddef>
#include <cstdint>

#include "Common/Image.h"

namespace Solvers::Tsa::Core {

/// 区域轮廓上的一个点：5 个 16 位量。
struct ThenarPoint {
    uint16_t value[5];
};
static_assert(sizeof(ThenarPoint) == 10);

/// 触点表头 +0x40 处的大鱼际信息（Thenar_UnitCopy 拷贝的范围到 pointCount 为止）。
struct ThenarInfo {
    uint8_t active;
    uint8_t field01;
    uint8_t field02[4];
    uint8_t reserved06[0x2];
    uint32_t side;  // 4 / 8：手掌在哪一侧
    uint8_t reserved0C[0x4];
    uint16_t field10;
    uint8_t reserved12[0x6];
    uint64_t field18;
    uint8_t reserved20[0x8];
    ThenarPoint point[20];
    uint8_t pointCount;
    uint8_t reservedF1[0x7];
};
static_assert(sizeof(ThenarInfo) == 0xF8);
static_assert(offsetof(ThenarInfo, side) == 0x08);
static_assert(offsetof(ThenarInfo, point) == 0x28);
static_assert(offsetof(ThenarInfo, pointCount) == 0xF0);

/// 0x6bb4f2a0：Thenar 对外报告的大鱼际信息（Thenar_UpdateExternalInfo），以及 β 握持的累计次数。
struct ThenarExternalInfo {
    uint8_t detected;
    uint8_t field01;
    uint8_t reserved02[0x2];
    uint32_t side;  // 同 ThenarInfo::side
    uint8_t reserved08[0x4];
    uint16_t field0C;
    uint8_t reserved0E[0x2];
    uint32_t betaGripLeftCount;
    uint32_t betaGripRightCount;
};
static_assert(sizeof(ThenarExternalInfo) == 0x18);
static_assert(offsetof(ThenarExternalInfo, side) == 0x04);
static_assert(offsetof(ThenarExternalInfo, betaGripLeftCount) == 0x10);

[[nodiscard]] inline ThenarExternalInfo& ThenarExternal() noexcept { return Image::At<ThenarExternalInfo>(0x6bb4f2a0); }

}  // namespace Solvers::Tsa::Core
