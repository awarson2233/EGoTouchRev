// GripFilter 的属性检测：形状、抓握、多指握持、附近的握持 / 新触点、快速移动、角落与手掌区域、安全触点。

#include <algorithm>
#include <cstdlib>

#include "Alg/Peak/Peak.h"
#include "Alg/Peak/PeakData.h"
#include "Alg/Touch/Touch.h"
#include "App/Report.h"
#include "App/Scenario.h"
#include "App/Sys.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Extension/HandGesture/HandGesture.h"
#include "Rejection/GripFilter/GripFilterInternal.h"

namespace Solvers::Tsa::Core::GripFilter {

namespace {

constexpr uint32_t kVolatileNoFastMove = 0x40000;  // TouchUnit::volatileFlags
constexpr int32_t kZoneMultiFinger = 3;                 // TouchUnit::zoneShape

// GripFilter_GetCtdDistToEdge @ 0x6bae61b9 / GripFilter_GetScreenRotatedCtdDistToEdge @ 0x6bae6220：
// 到长边 / 短边的距离（折叠屏的长短边与 Dim 相反）
uint16_t CtdDistToEdge(uint8_t curIdx) {
    const TouchUnit& u = Cur(curIdx);
    return PrmtFlash().gripDevice == kDeviceFold ? u.edgeDistDim2 : u.edgeDistDim1;
}
uint16_t ScreenRotatedCtdDistToEdge(uint8_t curIdx) {
    const TouchUnit& u = Cur(curIdx);
    return PrmtFlash().gripDevice == kDeviceFold ? u.edgeDistDim1 : u.edgeDistDim2;
}

// GripFilter_IsScreenRotatedMultiGripEnabled @ 0x6bae7ae6
bool IsScreenRotatedMultiGripEnabled() { return Sys::IsScreenRotated() && !Is2HandGame(); }

// GripFilter_GrapDetect @ 0x6bae71df：四个以上触点在 250 ms 内先后按下、都不到 500 ms、信号够强且都在握持区域里
void GrapDetect() {
    TouchTable& tt = Touches();
    uint32_t minDuration = UINT32_MAX;
    for (uint8_t k = 0; k < tt.count; ++k) minDuration = std::min(minDuration, tt.unit[k].durationMs);
    uint8_t n = 0;
    const int refThold = static_cast<int16_t>(PrmtFlash().peakFilterReferenceThreshold) * 3;
    for (uint8_t k = 0; k < tt.count; ++k) {
        const TouchUnit& u = tt.unit[k];
        if (!Touch::IsOff(k) && u.durationMs - minDuration < 250 && u.durationMs < 500 && refThold < u.signalScale * 10 &&
            GetCenterLevel(k) != 0) {
            ++n;
        }
    }
    if (n <= 3 || tt.count != n) return;
    for (uint8_t k = 0; k < tt.count; ++k) tt.unit[k].gripFlags |= kGripGrab;
}

// GripFilter_AboveGripDetect @ 0x6bae698c
void AboveGripDetect() {
    const uint8_t cur = Ctx().cur;
    TouchUnit& u = Cur(cur);
    for (uint8_t k = 0; k < Touches().count; ++k) {
        if (k == cur || (Cur(k).gripFlags & kGripInMultiRegion) == 0 || (u.gripFlags & kGripInMultiRegion) == 0) continue;
        if (GetCoreDim2Distance(cur, k) < 6 && Cur(k).pos.dim2 < u.pos.dim2) u.gripFlags |= kGripAboveGrip;
    }
}

// GripFilter_CloseGripDetect @ 0x6bae6e5b
void CloseGripDetect() {
    TouchUnit& u = CurG();
    if (!IsInPalmRegion(Ctx().cur)) u.gripFlags |= kGripOutOfPalm;
    if ((u.gripFlags & kGripOutOfPalm) != 0) {
        u.gripFlags &= ~(kGripCloseInTime | kGripCloseInPalm);
        return;
    }
    if (TouchesSide().count != 0 && Peak::IsToExcludeSideRegion()) {
        CloseGripOnSide();
    } else {
        CloseGripOnCore();
    }
}

// GripFilter_MultiGripDetect @ 0x6bae7d43
void MultiGripDetect() {
    const uint32_t f = CurG().gripFlags;
    if ((f & kGripMulti) == 0 && (f & kGripInMultiRegion) != 0) CheckMultiGrip();
}

// GripFilter_NewTouchDetect @ 0x6bae8ab3
void NewTouchDetect() {
    const TouchTable& tt = Touches();
    const uint8_t cur = Ctx().cur;
    TouchUnit& u = Cur(cur);
    const int maxInterval = 2 < tt.count ? 300 : 200;
    for (uint8_t k = 0; k < tt.count; ++k) {
        if (k == cur) continue;
        if (!Touch::IsGhostByMinSigOff(k) && Touch::GetSqDistMmInBetween(cur, k) < 0x1c39) {
            if (IsTouchIntervalInTime(cur, k, false, maxInterval)) u.gripFlags |= kGripNewTouchNearby;
            if (IsHigherPriorityTouch(cur, k)) u.gripFlags |= kGripHigherPriority;
            if ((Cur(k).persistFlags & kPersistWithdrawable) != 0 && (u.gripFlags & kGripHigherPriority) != 0) {
                u.gripFlags |= kGripNearWithdrawable;
            }
        }
        if (!Touch::IsOff(k)) u.gripFlags |= kGripHasOther;
    }
}

// GripFilter_FastMoveDetect @ 0x6bae65ff
void FastMoveDetect() {
    TouchUnit& u = CurG();
    if (0xffff < u.maxSqDistPxFromPrev && (u.volatileFlags & kVolatileNoFastMove) == 0 && u.maxPeakZn / 2 < Peaks().entry[u.peakIdx].zn) {
        u.gripFlags |= kGripFastMove;
    }
}

// GripFilter_HeavyGripRegionDetect @ 0x6bae8d98
void HeavyGripRegionDetect() {
    const uint8_t cur = Ctx().cur;
    TouchUnit& u = Cur(cur);
    if (IsTouchInCornerRegion(cur)) {
        u.gripFlags |= kGripInCorner;
        if (static_cast<int16_t>(u.age) == 1) u.gripFlags |= kGripInCornerFirstFrame;
        if ((u.event & TouchEvent::kTouchDownDebounce) != 0 && 4 < PrevG().debounceFrames) u.gripFlags |= kGripCornerDebounced;
    }
    if (IsTouchInHeavyRegion(cur)) {
        u.persistFlags2 |= kPersist2HeavyRegion;
        if (IsTouchInCornerRegion(cur)) {
            u.persistFlags2 |= kPersist2HeavyCorner;
            if (static_cast<int16_t>(u.age) == 1) u.persistFlags2 |= kPersist2HeavyCornerFirstFrame;
            if (Touch::IsOff(cur)) u.persistFlags2 |= kPersist2HeavyCornerOff;
        }
    }
    if (IsHeavyCornerTouched(cur)) u.persistFlags4 |= kPersist4HeavyCornerTouched;
}

// GripFilter_SafeTouchDetect @ 0x6bae891d
void SafeTouchDetect() {
    TouchUnit& u = CurG();
    bool safe;
    if (u.field059 == 0 && 1 < u.age) {
        safe = true;
    } else {
        safe = BasicIsFlick() || (u.gripFlags & kGripPeakAbsNotGrip) != 0 || IsInExcludedRegion(Ctx().cur) ||
               ((u.persistFlags & kPersistWithdrawable) != 0 && IsRatioOK(0xf));
    }
    if (safe) u.gripFlags |= kGripSafe;
}

// GripFilter_GetSideTouchOnTime @ 0x6bae6425：侧边触点所在格自按下以来的时间
int GetSideTouchOnTime(uint8_t sideIdx) {
    const Point pos = TouchesSide().unit[sideIdx].pos;
    return Buffers().timeDown[static_cast<uint16_t>((pos.dim1 >> 8) + (pos.dim2 >> 8) * PrmtFlash().dim1Len)];
}

// GripFilter_IsOnSameSide @ 0x6baea8bc / GripFilter_IsOnSameShortSide @ 0x6baeaac1：
// 两个触点在屏幕同一半边（alongDim1 时按 Dim1 分左右，否则按 Dim2 分上下）
bool IsOnSameSide(uint8_t a, uint8_t b, bool alongDim1) {
    const FlashConstPrmt& fc = PrmtFlashConst();
    const uint16_t half = (alongDim1 ? fc.dim1LenQ8 : fc.dim2LenQ8) >> 1;
    const uint16_t pa = alongDim1 ? Cur(a).pos.dim1 : Cur(a).pos.dim2;
    const uint16_t pb = alongDim1 ? Cur(b).pos.dim1 : Cur(b).pos.dim2;
    return (pa < half && pb < half) || (half < pa && half < pb);
}
bool IsOnSameShortSide(uint8_t a, uint8_t b) { return IsOnSameSide(a, b, false); }

// GripFilter_IsOnOppositeSide @ 0x6bae717f：左右两边，横屏时也算上下两边
bool IsOnOppositeSide(uint32_t a, uint32_t b) {
    if ((a == kSideLeft && b == kSideRight) || (a == kSideRight && b == kSideLeft)) return true;
    return Sys::IsScreenRotated() && ((a == kSideTop && b == kSideBottom) || (a == kSideBottom && b == kSideTop));
}

// GripFilter_IsHigherPriorityTouchPad @ 0x6baeabbe：平板上，同一半边里离边更近（或另一个可撤下）的触点优先
bool IsHigherPriorityTouchPad(uint8_t a, uint8_t b) {
    const int32_t rotation = Static().screenRotation;
    const bool landscape = rotation == 1 || rotation == 3;
    if (!IsOnSameSide(a, b, !landscape)) return false;
    if ((Cur(b).persistFlags & kPersistWithdrawable) != 0) return true;
    const bool closer = landscape ? Cur(a).maxEdgeDistDim2 < Cur(b).maxEdgeDistDim2 : Cur(a).maxEdgeDistDim1 < Cur(b).maxEdgeDistDim1;
    return closer && IsOnDim2();
}

// GripFilter_IsHigherPriorityTouchNormal @ 0x6baead61
bool IsHigherPriorityTouchNormal(uint8_t a, uint8_t b) {
    if (Scenario::IsTyping() && Ctx().absGripEnterCount == 0) return false;
    if (!Sys::IsScreenRotated()) {
        if (HandGesture::Is2HandDetected()) return false;
        if ((Cur(b).persistFlags & kPersistWithdrawable) != 0) return true;
        return Cur(a).edgeDistDim1 < Cur(b).edgeDistDim1 && IsOnDim1();
    }
    if (!IsOnSameShortSide(a, b)) return false;
    if ((Cur(b).persistFlags & kPersistWithdrawable) != 0) return true;
    return Cur(a).maxEdgeDistDim2 < Cur(b).maxEdgeDistDim2 && IsOnDim2();
}

// GripFilter_GetCoreDim1Distance @ 0x6bae5fc9：两个仍按着、Dim2 相距不到 2 格的触点沿 Dim1 的间距，否则 100
int GetCoreDim1Distance(uint8_t a, uint8_t b) {
    const TouchUnit& ua = Cur(a);
    const TouchUnit& ub = Cur(b);
    const int d = GetDim2Distance(ua.field03C, ua.field03D, ub.field03C, ub.field03D);
    const int dim2 = static_cast<int>(ua.pos.dim2) - static_cast<int>(ub.pos.dim2);
    if (-0x200 < dim2 && dim2 < 0x200 && !Touch::IsOff(a) && !Touch::IsOff(b)) return d;
    return 100;
}

// GripFilter_GetCoreDistance @ 0x6bae6171：沿长边方向的间距
int GetCoreDistance(uint8_t a, uint8_t b) {
    return PrmtFlash().gripDevice == kDeviceFold ? GetCoreDim1Distance(a, b) : GetCoreDim2Distance(a, b);
}

uint32_t RptSide(const TouchUnit& u) { return Report::GetSide(u.rptPos.dim1, u.rptPos.dim2); }

// GripFilterCheckMultiGripUnit @ 0x6bae73f3：另一个多指握持区域里、按下时间相近的触点紧挨着或在对边
void CheckMultiGripUnit(uint8_t k) {
    const uint8_t cur = Ctx().cur;
    if (k == cur || Touch::IsGhostByMinSigOff(k) || Touch::IsOff(k)) return;
    const TouchUnit& other = Cur(k);
    if ((other.gripFlags & kGripInMultiRegion) == 0 || !IsTouchIntervalInTime(cur, k, false, 250)) return;
    if ((other.persistFlags & kPersistWithdrawable) != 0 || !IsRatioOK(3)) return;
    TouchUnit& u = Cur(cur);
    const uint32_t side = RptSide(u);
    const uint32_t otherSide = RptSide(other);
    if (GetCoreDistance(cur, k) < 6) {
        u.gripFlags |= kGripMulti;
    } else if (IsOnOppositeSide(side, otherSide)) {
        u.gripFlags |= IsVideoMode() ? kGripMulti : kGripOppositeSide;
    }
}

}  // namespace

// GripFilter_GetProperty @ 0x6bae9747
void GetProperty() {
    GrapDetect();
    AboveGripDetect();
    CloseGripDetect();
    MultiGripDetect();
    NewTouchDetect();
    FastMoveDetect();
    HeavyGripRegionDetect();
    SafeTouchDetect();
}

// GripFilter_GetShapeFlag @ 0x6bae9201：刚拆成多指的触点清掉形状位；手掌区域里的长条形状
void GetShapeFlag() {
    const uint8_t cur = Ctx().cur;
    TouchUnit& u = Cur(cur);
    PrevTouchUnit& p = PrevG();
    constexpr uint32_t kShapeBits = kGripPalmShape | kGripElongated | kGripPalmShape2;
    if (u.zoneShape == kZoneMultiFinger && p.zoneShape != kZoneMultiFinger) {
        u.gripFlags &= ~kShapeBits;
        p.gripFlags &= ~kShapeBits;
    }
    if (IsInPalmRegion(cur) && (u.shapeIirValid != 0 || Touch::IsOff(cur))) {
        if (0x20 < u.iirAxisRatioX16 && 0xc < u.iirMajorMm) u.gripFlags |= kGripPalmShape;
        if (0xe < u.iirField066) u.gripFlags |= kGripPalmShape2;
    }
    if (0x2f < u.iirAxisRatioX16 && 0xe < u.iirMajorMm) u.gripFlags |= kGripElongated;
}

// GripFilter_IsInMultiGripRegion @ 0x6bae977e
bool IsInMultiGripRegion(uint8_t curIdx) {
    const TouchUnit& u = Cur(curIdx);
    if ((u.volatileFlags & kVolatileMultiGripRegion) != 0) return true;
    if (CtdDistToEdge(curIdx) < 0x101 && IsRatioOK(3)) return true;
    if (!IsScreenRotatedMultiGripEnabled()) return false;
    return ScreenRotatedCtdDistToEdge(curIdx) < (u.majorMm < 0x10 ? 0x101 : 0x181);
}

// GripFilter_IsInPalmRegion @ 0x6bae696e
bool IsInPalmRegion(uint8_t curIdx) { return IsTouchInHeavyRegion(curIdx); }

// GripFilter_GetCoreDim2Distance @ 0x6bae5e21：两个仍按着、Dim1 相距不到 2 格的触点沿 Dim2 的间距，否则 100
int GetCoreDim2Distance(uint8_t a, uint8_t b) {
    const TouchUnit& ua = Cur(a);
    const TouchUnit& ub = Cur(b);
    const int d = GetDim2Distance(ua.field03E, ua.field03F, ub.field03E, ub.field03F);
    const int dim1 = static_cast<int>(ua.pos.dim1) - static_cast<int>(ub.pos.dim1);
    if (-0x200 < dim1 && dim1 < 0x200 && !Touch::IsOff(a) && !Touch::IsOff(b)) return d;
    return 100;
}

// GripFilter_CloseGripOnCore @ 0x6bae6b13：屏幕网格上沿 Dim2 紧挨着另一个触点
void CloseGripOnCore() {
    const uint8_t cur = Ctx().cur;
    TouchUnit& u = Cur(cur);
    for (uint8_t k = 0; k < Touches().count; ++k) {
        if (k == cur || !IsSignalOK(cur, 8, 1) || 6 <= GetCoreDim2Distance(cur, k)) continue;
        u.persistFlags4 |= kPersist4CloseGrip;
        if (IsInPalmRegion(k)) u.gripFlags |= kGripCloseInPalm;
        if (IsTouchIntervalInTime(cur, k, false, 200)) u.gripFlags |= kGripCloseInTime;
    }
}

// GripFilter_CloseGripOnSide @ 0x6bae6cf9：侧边触点表里沿 Dim2 紧挨着一个触点
void CloseGripOnSide() {
    const uint8_t cur = Ctx().cur;
    TouchUnit& u = Cur(cur);
    for (uint8_t k = 0; k < TouchesSide().count; ++k) {
        if (6 <= GetSideDim2Distance(cur, k) || !IsSignalOK(cur, 8, 1)) continue;
        u.gripFlags |= kGripCloseInPalm;
        if (IsTouchIntervalInTime(cur, k, true, 200)) u.gripFlags |= kGripCloseInTime;
    }
}

// GripFilter_CheckMultiGrip @ 0x6bae7d07
void CheckMultiGrip() {
    if (TouchesSide().count != 0 && Peak::IsToExcludeSideRegion()) {
        CheckMultiGripOnSide();
    } else {
        CheckMultiGripOnCore();
    }
    CheckCornerMultiGrip();
    CheckScreenRotatedMultiGrip();
}

// GripFilter_CheckMultiGripOnCore @ 0x6bae7701
void CheckMultiGripOnCore() {
    for (uint8_t k = 0; k < Touches().count; ++k) CheckMultiGripUnit(k);
}

// GripFilter_CheckMultiGripOnSide @ 0x6bae773c：按下时间相近的侧边触点紧挨着或在对边
void CheckMultiGripOnSide() {
    const uint8_t cur = Ctx().cur;
    TouchUnit& u = Cur(cur);
    for (uint8_t k = 0; k < TouchesSide().count; ++k) {
        if (Touch::IsSideCoreInSameZone(cur, k) || !IsTouchIntervalInTime(cur, k, true, 250)) continue;
        const uint32_t side = RptSide(u);
        const uint32_t otherSide = RptSide(TouchesSide().unit[k]);
        if (GetSideDim2Distance(cur, k) < 6 || IsOnOppositeSide(side, otherSide)) u.gripFlags |= kGripOppositeSide;
    }
}

// GripFilter_CheckCornerMultiGrip @ 0x6bae7964：两个触点都按在重握持的角上
void CheckCornerMultiGrip() {
    const uint8_t cur = Ctx().cur;
    TouchUnit& u = Cur(cur);
    for (uint8_t k = 0; k < Touches().count; ++k) {
        const TouchUnit& other = Cur(k);
        if (k == cur || (other.persistFlags & kPersistWithdrawable) != 0 || (other.persistFlags2 & kPersist2HeavyCorner) == 0 ||
            (u.persistFlags2 & kPersist2HeavyCorner) == 0) {
            continue;
        }
        if (IsTouchIntervalInTime(cur, k, false, 250) || !Touch::IsFullyLiftOff(cur)) u.gripFlags |= kGripMulti;
    }
}

// GripFilter_CheckScreenRotatedMultiGrip @ 0x6bae7b19：横屏时同一短边上已是多指握持的触点，
// 或视频模式下沿 Dim1 紧挨着的角落新触点
void CheckScreenRotatedMultiGrip() {
    if (!IsScreenRotatedMultiGripEnabled()) return;
    const uint8_t cur = Ctx().cur;
    TouchUnit& u = Cur(cur);
    for (uint8_t k = 0; k < Touches().count; ++k) {
        if (k == cur) continue;
        const TouchUnit& other = Cur(k);
        const bool sameShortSide = (other.gripFlags & kGripMulti) != 0 && IsOnSameShortSide(cur, k);
        const bool cornerNearby = (other.gripFlags & kGripInCornerFirstFrame) != 0 && GetCoreDim1Distance(cur, k) <= 5 && IsVideoMode();
        if ((other.persistFlags & kPersistWithdrawable) != 0 || (!sameShortSide && !cornerNearby)) continue;
        if (IsTouchIntervalInTime(cur, k, false, 250) || !Touch::IsFullyLiftOff(cur)) u.gripFlags |= kGripMulti;
    }
}

// GripFilter_IsTouchIntervalInTime @ 0x6bae6536：两个仍按着的触点按下时间相差不到 maxMs
bool IsTouchIntervalInTime(uint8_t curIdx, uint8_t otherIdx, bool otherOnSide, int maxMs) {
    if (Touch::IsOff(otherIdx) || Touch::IsOff(curIdx)) return false;
    const int on = Touch::GetTimeOn(curIdx);
    int otherOn = Touch::GetTimeOn(otherIdx);
    if (otherOnSide) otherOn = GetSideTouchOnTime(otherIdx);
    return std::abs(on - otherOn) < maxMs;
}

// GripFilter_IsHigherPriorityTouch @ 0x6baeaf03
bool IsHigherPriorityTouch(uint8_t curIdx, uint8_t otherIdx) {
    return IsPad() ? IsHigherPriorityTouchPad(curIdx, otherIdx) : IsHigherPriorityTouchNormal(curIdx, otherIdx);
}

// GripFilter_GetDim2Distance @ 0x6bae5dd6：两段 [start, end] 之间的空隙，重叠时为 0
int GetDim2Distance(uint8_t aStart, uint8_t aEnd, uint8_t bStart, uint8_t bEnd) {
    if (aEnd < bStart) return bStart - aEnd;
    if (bEnd < aStart) return aStart - bEnd;
    return 0;
}

// GripFilter_GetSideDim2Distance @ 0x6bae6287：屏内触点与侧边触点沿 Dim2 的间距（同一区域、已抬起或 Dim1 相距 2 格以上时 100）
int GetSideDim2Distance(uint8_t coreIdx, uint8_t sideIdx) {
    const TouchUnit& u = Cur(coreIdx);
    const TouchUnit& s = TouchesSide().unit[sideIdx];
    const int d = GetDim2Distance(u.field03E, u.field03F, s.field03E, s.field03F);
    if (Touch::IsSideCoreInSameZone(coreIdx, sideIdx) || Touch::IsOff(coreIdx)) return 100;
    const int dim1 = static_cast<int>(s.pos.dim1) - static_cast<int>(TouchesGrid().unit[coreIdx].pos.dim1);
    return -0x200 < dim1 && dim1 < 0x200 ? d : 100;
}

}  // namespace Solvers::Tsa::Core::GripFilter
