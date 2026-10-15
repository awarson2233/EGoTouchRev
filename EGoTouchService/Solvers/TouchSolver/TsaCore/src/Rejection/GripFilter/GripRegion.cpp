// GripFilter 的握持区域图与死区。
// 区域图（g_tsaBufGripRegion）每格一个字节，bit n 表示该格属于等级 n 的握持区域（GetGripLevel 取最高位）。
// 死区按四条边各记 0x50 格：握持触点沿边缘覆盖的范围，边缘信号消失后清除。

#include <algorithm>
#include <array>
#include <cstring>
#include <utility>

#include "Alg/Peak/PeakData.h"
#include "App/Scenario.h"
#include "App/Sys.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Extension/HandGesture/HandGesture.h"
#include "Prmt/TsaPrmt.h"
#include "Rejection/GripFilter/GripFilterInternal.h"

namespace Solvers::Tsa::Core::GripFilter {

namespace {

// RegionSet 的区域掩码
constexpr uint32_t kRow = 0x1;             // 第 offset 行
constexpr uint32_t kRowFromEnd = 0x2;      // 倒数第 offset 行
constexpr uint32_t kCol = 0x4;             // 第 offset 列
constexpr uint32_t kColFromEnd = 0x8;      // 倒数第 offset 列
constexpr uint32_t kTopLeft = 0x10;        // 四个角各 3 格
constexpr uint32_t kTopRight = 0x20;
constexpr uint32_t kBottomLeft = 0x40;
constexpr uint32_t kBottomRight = 0x80;
constexpr uint32_t kPalmColTop = 0x100;    // 手掌区域：沿某列 / 某行从一端起 10 格
constexpr uint32_t kPalmColEndTop = 0x200;
constexpr uint32_t kPalmColBottom = 0x400;
constexpr uint32_t kPalmColEndBottom = 0x800;
constexpr uint32_t kPalmRowLeft = 0x1000;
constexpr uint32_t kPalmRowRight = 0x2000;
constexpr uint32_t kPalmRowEndLeft = 0x4000;
constexpr uint32_t kPalmRowEndRight = 0x8000;
constexpr uint8_t kPalmLength = 10;
constexpr uint8_t kHandGestureBit = 0x8;   // 握持手势的区域按等级 3 标

constexpr size_t kDeadZoneLen = 0x50;
constexpr uint8_t kDeadZoneTop = 0x1;      // FlashPrmt::gripDeadzoneEdges
constexpr uint8_t kDeadZoneBottom = 0x2;
constexpr uint8_t kDeadZoneLeft = 0x4;
constexpr uint8_t kDeadZoneRight = 0x8;

using DeadZone = std::array<uint8_t, 4 * kDeadZoneLen>;  // 上、下、左、右各 kDeadZoneLen 格，首尾相接
DeadZone g_deadZone;  // 0x6bba33c0

void Mark(int cell, uint8_t bit) { Buffers().gripRegion[static_cast<uint16_t>(cell)] |= bit; }

// GripFilter_RegionSetTopLeftCorner @ 0x6baeaf42 等四个：角上的格及其两个邻格
void SetCorners(uint32_t mask, uint8_t bit) {
    const int cols = PrmtFlash().dim1Len;
    const int lastCol = static_cast<uint8_t>(cols - 1);
    const int lastRow = static_cast<uint8_t>(PrmtFlash().dim2Len - 1);
    if ((mask & kTopLeft) != 0) {
        Mark(0, bit);
        Mark(1, bit);
        Mark(cols, bit);
    }
    if ((mask & kTopRight) != 0) {
        Mark(lastCol, bit);
        Mark(lastCol - 1, bit);
        Mark(lastCol + cols, bit);
    }
    if ((mask & kBottomLeft) != 0) {
        Mark(lastRow * cols, bit);
        Mark(lastRow * cols + 1, bit);
        Mark((lastRow - 1) * cols, bit);
    }
    if ((mask & kBottomRight) != 0) {
        Mark(lastCol + lastRow * cols, bit);
        Mark(lastCol + lastRow * cols - 1, bit);
        Mark(lastCol + (lastRow - 1) * cols, bit);
    }
}

// GripFilter_RegionSetPalm @ 0x6baeb512
void SetPalm(uint32_t mask, uint8_t bit, uint8_t offset) {
    const FlashPrmt& fl = PrmtFlash();
    const int cols = fl.dim1Len;
    const int rows = fl.dim2Len;
    const int colFromEnd = static_cast<uint8_t>(cols - offset - 1);
    const int rowFromEnd = static_cast<uint8_t>(rows - offset - 1);
    for (int i = 0; i < kPalmLength; ++i) {
        if ((mask & kPalmColTop) != 0) Mark(offset + i * cols, bit);
        if ((mask & kPalmColEndTop) != 0) Mark(colFromEnd + i * cols, bit);
        if ((mask & kPalmColBottom) != 0) Mark(offset + (rows - 1 - i) * cols, bit);
        if ((mask & kPalmColEndBottom) != 0) Mark(colFromEnd + (rows - 1 - i) * cols, bit);
        if ((mask & kPalmRowLeft) != 0) Mark(i + offset * cols, bit);
        if ((mask & kPalmRowEndLeft) != 0) Mark(i + rowFromEnd * cols, bit);
        if ((mask & kPalmRowRight) != 0) Mark(cols - i + offset * cols - 1, bit);
        if ((mask & kPalmRowEndRight) != 0) Mark(cols - i + rowFromEnd * cols - 1, bit);
    }
}

// GripFilter_RegionSet @ 0x6baebad8：把掩码选中的行 / 列 / 角 / 手掌区域标为等级 level
void RegionSet(uint32_t mask, uint8_t level, uint8_t offset) {
    const FlashPrmt& fl = PrmtFlash();
    const auto bit = static_cast<uint8_t>(1u << (level & 0x1f));
    const int cols = fl.dim1Len;
    for (int c = 0; c < cols; ++c) {
        if ((mask & kRow) != 0) Mark(c + offset * cols, bit);
        if ((mask & kRowFromEnd) != 0) Mark(c + static_cast<uint8_t>(fl.dim2Len - offset - 1) * cols, bit);
    }
    for (int r = 0; r < fl.dim2Len; ++r) {
        if ((mask & kCol) != 0) Mark(offset + r * cols, bit);
        if ((mask & kColFromEnd) != 0) Mark(static_cast<uint8_t>(cols - offset - 1) + r * cols, bit);
    }
    SetCorners(mask, bit);
    SetPalm(mask, bit, offset);
}

/// 横屏时手握一侧的两个角（另一侧在双手游戏外也标）。
void SetRotatedCorners(uint32_t gripSide, uint32_t otherSide, bool other) {
    RegionSet(gripSide, 3, 0);
    if (other) RegionSet(otherSide, 3, 0);
}

// GripFilter_RegionSetByScreenRotation @ 0x6baebde9：手机
void RegionSetByScreenRotation() {
    const TsaStatic& stat = Static();
    RegionSet(kCol | kColFromEnd, 2, 0);
    RegionSet(kCol | kColFromEnd, 1, 1);
    const bool other = !Is2HandGame() && !IsPad();
    switch (stat.screenRotation) {
        case 1: SetRotatedCorners(kTopLeft | kBottomLeft, kTopRight | kBottomRight, other); break;
        case 3: SetRotatedCorners(kTopRight | kBottomRight, kTopLeft | kBottomLeft, other); break;
        case 2:
            RegionSet(kTopLeft, 3, 0);
            RegionSet(kTopRight, 3, 0);
            break;
        case 0:
            RegionSet(kBottomLeft, 3, 0);
            RegionSet(kBottomRight, 3, 0);
            RegionSet(kPalmColBottom, 3, 0);
            RegionSet(kPalmColEndBottom, 3, 0);
            break;
        default: break;
    }
    const bool sideTouch = (stat.featureFlags & kFeatSideTouch) != 0;
    if (stat.screenRotation == 0 || stat.screenRotation == 2) {
        RegionSet(kRow | kRowFromEnd, 1, 0);
        if (sideTouch) {
            const uint8_t level = Scenario::IsTyping() ? 2 : 3;
            RegionSet(kPalmColBottom, level, 1);
            RegionSet(kPalmColEndBottom, level, 1);
        }
    } else {
        RegionSet(kRow | kRowFromEnd, 2, 0);
        if (std::strncmp(ProjectName(), "P212", 4) == 0) RegionSet(kRow | kRowFromEnd, 2, 1);
        if (sideTouch) RegionSet(kCol | kColFromEnd, 2, 1);
    }
}

// GripFilter_RegionSetByScreenRotationPad @ 0x6baec121：平板（只在机型为平板时调用，另一侧的角从不标）
void RegionSetByScreenRotationPad() {
    switch (Static().screenRotation) {
        case 1:
        case 3:
            RegionSet(kRow | kRowFromEnd, 2, 0);
            RegionSet(kRow | kRowFromEnd, 1, 1);
            RegionSet(kCol | kColFromEnd, 1, 0);
            if (Static().screenRotation == 1) {
                RegionSet(kTopLeft | kBottomLeft | kPalmRowLeft | kPalmRowEndLeft, 3, 0);
            } else {
                RegionSet(kTopRight | kBottomRight | kPalmRowRight | kPalmRowEndRight, 3, 0);
            }
            break;
        default:
            RegionSet(kRow | kRowFromEnd, 1, 0);
            RegionSet(kCol | kColFromEnd, 2, 0);
            RegionSet(kCol | kColFromEnd, 1, 1);
            if (Static().screenRotation == 2) {
                RegionSet(kTopLeft | kTopRight | kPalmColTop | kPalmColEndTop, 3, 0);
            } else if (Static().screenRotation == 0) {
                RegionSet(kBottomLeft | kBottomRight | kPalmColBottom | kPalmColEndBottom, 3, 0);
            }
            break;
    }
}

// GripFilter_RegionSetByScreenRotationFold @ 0x6baec3e4：折叠屏
void RegionSetByScreenRotationFold() {
    const int32_t rotation = Static().screenRotation;
    RegionSet(kRow | kRowFromEnd, 2, 0);
    RegionSet(kRow | kRowFromEnd, 1, 1);
    const bool other = !Is2HandGame();
    switch (rotation) {
        case 1: SetRotatedCorners(kBottomLeft | kBottomRight, kTopLeft | kTopRight, other); break;
        case 3: SetRotatedCorners(kTopLeft | kTopRight, kBottomLeft | kBottomRight, other); break;
        case 2: RegionSet(kTopLeft | kBottomLeft, 3, 0); break;
        case 0: RegionSet(kTopRight | kBottomRight | kPalmRowEndRight | kPalmRowRight, 3, 0); break;
        default: break;
    }
    RegionSet(kCol | kColFromEnd, rotation == 0 || rotation == 2 ? 1 : 2, 0);
}

// GripFilter_RegionSetByHandGesture @ 0x6baec611
void RegionSetByHandGesture() {
    const int8_t* tzPrev = HandBufs().tzPrev;
    uint8_t* region = Buffers().gripRegion;
    for (uint16_t i = 0; i < PrmtFlash().sensorNum; ++i) {
        if (tzPrev[i] != 0) region[i] |= kHandGestureBit;
    }
}

// GripFilter_SetDeadZoneFlag @ 0x6bae5021：握持（或满足死区判据）的触点在边缘时，把它沿边覆盖的格记入死区
void SetDeadZoneFlag() {
    const uint8_t curIdx = Ctx().cur;
    if (!IsGripOrTbdTouch(curIdx) && !IsGripCriteriaDeadzone(curIdx)) return;
    const FlashPrmt& fl = PrmtFlash();
    const TouchUnit& u = Cur(curIdx);
    const uint8_t edges = fl.gripDeadzoneEdges;
    const auto mark = [](size_t edge, uint16_t pos, uint8_t width, uint8_t len) {
        const uint8_t end = GetEdgeWidthEnd(pos, width, len);
        for (uint8_t i = GetEdgeWidthStart(pos, width); i <= end; ++i) g_deadZone[edge * kDeadZoneLen + i] = 1;
    };
    if ((edges & kDeadZoneTop) != 0 && u.rawPos.dim2 <= 0xff) {
        mark(0, u.rawPos.dim1, u.field070, fl.dim1Len);
    } else if ((edges & kDeadZoneBottom) != 0 && (fl.dim2Len - 1) * 0x100 < u.rawPos.dim2) {
        mark(1, u.rawPos.dim1, u.field070, fl.dim1Len);
    } else if ((edges & kDeadZoneLeft) != 0 && u.rawPos.dim1 <= 0xff) {
        mark(2, u.rawPos.dim2, u.field071, fl.dim2Len);
    } else if ((edges & kDeadZoneRight) != 0 && (fl.dim1Len - 1) * 0x100 < u.rawPos.dim1) {
        mark(3, u.rawPos.dim2, u.field071, fl.dim2Len);
    }
}

// GripFilter_ClearDeadZoneFlag @ 0x6bae5565：边缘那一行 / 列的 Dif 不超过参考门限 1/6 的格清出死区；没有触点时全清
void ClearDeadZoneFlag() {
    const FlashPrmt& fl = PrmtFlash();
    const int thold = static_cast<int16_t>(fl.peakFilterReferenceThreshold) / 6;
    const int16_t* dif = Buffers().dif;
    const uint8_t cols = fl.dim1Len;
    const uint8_t rows = fl.dim2Len;
    DeadZone active{};
    if ((fl.gripDeadzoneEdges & kDeadZoneTop) != 0) {
        for (uint8_t c = 0; c < cols; ++c) active[c] = thold < dif[c];
    }
    if ((fl.gripDeadzoneEdges & kDeadZoneBottom) != 0) {
        for (uint8_t c = 0; c < cols; ++c) active[kDeadZoneLen + c] = thold < dif[static_cast<uint16_t>(c + (rows - 1) * cols)];
    }
    if ((fl.gripDeadzoneEdges & kDeadZoneLeft) != 0) {
        for (uint8_t r = 0; r < rows; ++r) active[2 * kDeadZoneLen + r] = thold < dif[cols * r];
    }
    if ((fl.gripDeadzoneEdges & kDeadZoneRight) != 0) {
        for (uint8_t r = 0; r < rows; ++r) active[3 * kDeadZoneLen + r] = thold < dif[static_cast<uint16_t>(cols + r * cols - 1)];
    }
    if (Touches().count == 0) active = {};
    for (size_t i = 0; i < active.size(); ++i) {
        if (active[i] == 0) g_deadZone[i] = 0;
    }
}

}  // namespace

// GripFilter_GetEdgeWidthStart @ 0x6bae4d44 / GripFilter_GetEdgeWidthEnd @ 0x6bae4d95：
// 以 pos（Q8.8）所在格为中心、宽 width 的范围，夹在 [0, len - 1] 内
uint8_t GetEdgeWidthStart(uint16_t pos, uint8_t width) {
    const int center = pos >> 8;
    const int half = width >> 1;
    return static_cast<uint8_t>(center < half ? 0 : center - half);
}
uint8_t GetEdgeWidthEnd(uint16_t pos, uint8_t width, uint8_t len) {
    const int end = (pos >> 8) + (width >> 1);
    return static_cast<uint8_t>(len - 1 < end ? len - 1 : end);
}

// GripFilter_IsGripCriteriaDeadzone @ 0x6bae4df8：在开启死区的边上，且沿边跨度大或形状 IIR 的长轴大
bool IsGripCriteriaDeadzone(uint8_t curIdx) {
    const FlashPrmt& fl = PrmtFlash();
    const TouchUnit& u = Cur(curIdx);
    const bool bigShape = 0x1a <= CurG().iirMajorMm;
    if ((fl.gripDeadzoneEdges & (kDeadZoneLeft | kDeadZoneRight)) != 0) {
        const bool inner = 0xff < u.rawPos.dim1 && u.rawPos.dim1 <= (fl.dim1Len - 1) * 0x100;
        if (!inner && (fl.dim2Len >> 2 < u.field071 || bigShape)) return true;
    }
    if ((fl.gripDeadzoneEdges & (kDeadZoneTop | kDeadZoneBottom)) == 0) return false;
    const bool inner = 0xff < u.rawPos.dim2 && u.rawPos.dim2 <= (fl.dim2Len - 1) * 0x100;
    return !inner && (fl.dim1Len >> 1 < u.field070 || bigShape);
}

// GripFilter_IsInDynamicDeadZone @ 0x6bae59ae：贴着开启死区的边时，沿边那一格及两侧邻格在任一条边的死区里
bool IsInDynamicDeadZone(uint16_t distThold) {
    const FlashPrmt& fl = PrmtFlash();
    if (fl.gripDeadzoneEdges == 0) return false;
    const TouchUnit& u = CurG();
    const auto hit = [](int center, int len) {
        const int lo = center == 0 ? 0 : center - 1;
        const int hi = center < len - 1 ? center + 1 : len - 1;
        for (size_t e = 0; e < 4; ++e) {
            const uint8_t* zone = &g_deadZone[e * kDeadZoneLen];
            if (zone[center] != 0 || zone[lo] != 0 || zone[hi] != 0) return true;
        }
        return false;
    };
    if ((fl.gripDeadzoneEdges & (kDeadZoneLeft | kDeadZoneRight)) != 0 && u.edgeDistDim1 < distThold &&
        hit(u.rawPos.dim2 >> 8, fl.dim2Len)) {
        return true;
    }
    return (fl.gripDeadzoneEdges & (kDeadZoneTop | kDeadZoneBottom)) != 0 && u.edgeDistDim2 < distThold &&
           hit(u.rawPos.dim1 >> 8, fl.dim1Len);
}

namespace {

// GripFilter_IsSensorInCornerRegion @ 0x6baea1f4：角上的格，及其沿 Dim2 的一个、沿 Dim1 的一个邻格
bool IsSensorInCornerRegion(int dim1, int dim2) {
    const int cols = PrmtFlash().dim1Len;
    const int rows = PrmtFlash().dim2Len;
    if ((dim1 == 0 || dim1 == cols - 1) && (dim2 == 0 || dim2 == 1 || dim2 == rows - 2 || dim2 == rows - 1)) return true;
    return (dim1 == 1 || dim1 == cols - 2) && (dim2 == 0 || dim2 == rows - 1);
}

// GripFilter_IsTouchPosInCornerRegion @ 0x6baea2b0：两个方向都离边不到 1.5 格
bool IsTouchPosInCornerRegion(uint8_t curIdx) {
    const TouchUnit& u = Cur(curIdx);
    return u.edgeDistDim1 * 2 < 0x300 && u.edgeDistDim2 * 2 < 0x300;
}

/// 判区域用的格：靠角时用峰所在的格，否则用坐标所在的格。
std::pair<int, int> RegionCell(uint8_t curIdx) {
    const TouchUnit& u = Cur(curIdx);
    if (IsTouchPosInCornerRegion(curIdx)) {
        const PeakEntry& pk = Peaks().entry[u.peakIdx];
        return {pk.dim1, pk.dim2};
    }
    return {u.pos.dim1 >> 8, u.pos.dim2 >> 8};
}

}  // namespace

// GripFilter_IsTouchInCornerRegion @ 0x6baea327
bool IsTouchInCornerRegion(uint8_t curIdx) {
    const auto [dim1, dim2] = RegionCell(curIdx);
    return IsSensorInCornerRegion(dim1, dim2);
}

// GripFilter_IsTouchInHeavyRegion @ 0x6baea454：所在格属于等级 3 的握持区域
bool IsTouchInHeavyRegion(uint8_t curIdx) {
    const auto [dim1, dim2] = RegionCell(curIdx);
    return 7 < Buffers().gripRegion[static_cast<uint16_t>(dim1 + dim2 * PrmtFlash().dim1Len)];
}

// GripFilter_IsHeavyRegionTouched @ 0x6baea6e5 / GripFilter_GetTouchedLevel @ 0x6baea6a0：触点区域碰到的最高握持等级为 3
bool IsHeavyRegionTouched(uint8_t curIdx) { return GetGripLevel(Cur(curIdx).field059) == 3; }

// GripFilter_IsHeavyCornerTouched @ 0x6baea7ca
bool IsHeavyCornerTouched(uint8_t curIdx) { return IsHeavyRegionTouched(curIdx) && IsNearbyCorner(curIdx); }

// GripFilter_IsInExcludedRegion @ 0x6bae9f40：离边足够远的小触点，或离边很远的触点不判握持
bool IsInExcludedRegion(uint8_t curIdx) {
    uint16_t dim1Thold = 0x200;
    uint16_t dim2Thold = 0x200;
    if (!Sys::IsScreenRotated()) {
        if ((Static().featureFlags & kFeatSideTouch) != 0) dim1Thold = 0x300;
        dim2Thold = 0x100;
    }
    if (IsVideoMode() && IsNearbyCorner(Ctx().cur)) return false;
    const TouchUnit& u = Cur(curIdx);
    if (u.majorMm < 0x14 && dim1Thold < u.edgeDistDim1 && dim2Thold < u.edgeDistDim2 && IsRatioOK(0xf)) return true;
    return 0x400 < u.edgeDistDim1 && 0x300 < u.edgeDistDim2;
}

// GripFilter_RegionReset @ 0x6bae9e90
void RegionReset() { std::fill_n(Buffers().gripRegion, PrmtFlash().sensorNum, uint8_t{0}); }

// GripFilter_DeadzoneReset @ 0x6bae4ce0
void DeadzoneReset() { g_deadZone = {}; }

// GripFilter_UpdateDeadzone @ 0x6bae5981
void UpdateDeadzone() {
    if (PrmtFlash().gripDeadzoneEdges == 0) return;
    SetDeadZoneFlag();
    ClearDeadZoneFlag();
}

// GripFilter_RegionProcess @ 0x6baec71c
void RegionProcess() {
    RegionReset();
    switch (PrmtFlash().gripDevice) {
        case kDevicePad: RegionSetByScreenRotationPad(); break;
        case kDeviceFold: RegionSetByScreenRotationFold(); break;
        default: RegionSetByScreenRotation(); break;
    }
    RegionSetByHandGesture();
}

}  // namespace Solvers::Tsa::Core::GripFilter
