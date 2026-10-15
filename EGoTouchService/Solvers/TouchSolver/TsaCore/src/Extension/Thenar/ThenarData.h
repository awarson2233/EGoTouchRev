#pragma once
// 大鱼际 / 手掌区域（Thenar）的数据：触点表头 +0x40 处的检测结果，以及对外报告的那一份。

#include <cstddef>
#include <cstdint>

#include "Common/Image.h"

namespace Solvers::Tsa::Core {

/// 沿一条侧边扫出来的一段信号（Dif 超过侧边噪声门限一半的连续格）。
struct ThenarPoint {
    uint16_t start;       // 段的起点：沿边坐标（从远端往回扫，是段内最大的那个）
    uint16_t length;      // 格数
    uint16_t edgeWidth;   // 段内各格向屏幕内侧（含跨行折回）超过触摸门限的最大格数
    uint8_t onFirstEdge;  // 1：下标 0 的那条边；0：最后一条边
    uint8_t reserved07;
    int16_t peak;  // 段内最大 Dif
};
static_assert(sizeof(ThenarPoint) == 10);

/// 触点表头 +0x40 处的大鱼际信息。
struct ThenarInfo {
    uint8_t detected;          // 判出了手掌在哪一侧
    uint8_t reserved01[0x3];
    uint8_t gesture;           // 手掌信号够强
    uint8_t gestureConfirmed;  // 手掌信号持续超过 800 ms
    uint8_t reserved06[0x2];
    uint32_t side;  // 4 / 8：手掌在哪一侧
    uint8_t reserved0C[0xc];
    uint64_t gestureStartTime;    // 手掌信号开始的时间（ms）
    uint64_t gestureConfirmTime;  // 确认的时间（ms）；历史表里这一项不随帧拷贝
    ThenarPoint point[20];
    uint8_t pointCount;
    uint8_t reservedF1[0x7];
};
static_assert(sizeof(ThenarInfo) == 0xF8);
static_assert(offsetof(ThenarInfo, side) == 0x08);
static_assert(offsetof(ThenarInfo, gestureStartTime) == 0x18);
static_assert(offsetof(ThenarInfo, point) == 0x28);
static_assert(offsetof(ThenarInfo, pointCount) == 0xF0);

/// 0x6bb4f2a0：Thenar 对外报告的手掌侧（未检出时为 0），以及 β 握持的累计次数。
struct ThenarExternalInfo {
    uint8_t reserved00[0x4];
    uint32_t side;  // 同 ThenarInfo::side
    uint8_t reserved08[0x8];
    uint32_t betaGripLeftCount;
    uint32_t betaGripRightCount;
};
static_assert(sizeof(ThenarExternalInfo) == 0x18);
static_assert(offsetof(ThenarExternalInfo, side) == 0x04);
static_assert(offsetof(ThenarExternalInfo, betaGripLeftCount) == 0x10);

[[nodiscard]] inline ThenarExternalInfo& ThenarExternal() noexcept { return Image::At<ThenarExternalInfo>(0x6bb4f2a0); }

}  // namespace Solvers::Tsa::Core
