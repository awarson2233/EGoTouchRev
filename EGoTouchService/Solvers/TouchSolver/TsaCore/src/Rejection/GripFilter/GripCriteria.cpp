// GripFilter 的判据：本帧的绝对 / 临时握持、边缘比例门限、长按、异常与被抑制的点击。
// 边缘比例（edgeRatioDim1 / edgeRatioDim2 及其历史最小、最大与 IIR 值）是触点在边缘一列 / 一行上的信号占比，
// 触点贴着 Dim1 的边（IsOnDim1）时看 Dim1 方向的那一个。

#include <algorithm>
#include <cstdlib>

#include "Alg/Peak/Peak.h"
#include "Alg/Peak/PeakData.h"
#include "Alg/Touch/Touch.h"
#include "Alg/TouchSize/TouchSize.h"
#include "App/Report.h"
#include "App/Scenario.h"
#include "App/Sys.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Rejection/GripFilter/GripFilterInternal.h"

namespace Solvers::Tsa::Core::GripFilter {

namespace {

uint64_t Now() { return Static().curTimeMs; }

// GripFilter_IsNegSigTouch @ 0x6bae38c7：负信号触点（小、刚按下且仍按着）
bool IsNegSigTouch() {
    const TouchUnit& u = CurG();
    return PrmtDynamic().skipZ8CheckOnInnerEdge != 0 && u.hiSkipZ8 != 0 && u.maxMajorMm < 0xf && u.durationMs < 1000 &&
           !Touch::IsOff(Ctx().cur);
}

// GripFilter_IsGlobalSuppressedClick @ 0x6bae82c8
bool IsGlobalSuppressedClick() {
    const TouchUnit& u = CurG();
    return (u.gripFlags & kGripNewTouchNearby) != 0 && (u.gripFlags & kGripHigherPriority) != 0 &&
           (u.persistFlags2 & kPersist2HeavyRegion) != 0 && IsSignalOK(Ctx().cur, 1, 1);
}

}  // namespace

// GripFilter_BasicIsAbsNotGrip @ 0x6bae4262
bool BasicIsAbsNotGrip() { return (CurG().gripFlags & kGripSafe) != 0; }

// GripFilter_BasicIsAbsGrip @ 0x6bae3fc1
bool BasicIsAbsGrip() {
    if (BasicIsAbsGripPrmt() || IsSuppressedGlobally() || IsSuppressedGlobally2()) return true;
    const uint8_t cur = Ctx().cur;
    const TouchUnit& u = Cur(cur);
    if ((u.gripFlags & kGripMulti) != 0) return true;
    if ((u.persistFlags4 & kPersist4Merged) != 0 && (Peaks().entry[u.peakIdx].flags & kPeakAbsGrip) != 0) return true;
    if ((u.persistFlags4 & kPersist4MergeCancelled) != 0 || IsCornerGripClick() || BasicIsAbsGripAbnormal()) return true;
    if (IsLongPress() && (u.persistFlags & kPersistWithdrawable) == 0 && !Is2HandGame()) {
        if ((IsOnDim2() || IsNearbyCorner(cur)) && (u.gripFlags & kGripDelayTimeOut) != 0) return true;
        if (IsNearbyCorner(cur) && Sys::IsScreenRotated() && (u.gripFlags & kGripNewTouchNearby) != 0) return true;
    }
    return IsNegSigTouchBypass();
}

// GripFilter_BasicIsTempGrip @ 0x6bae39aa
bool BasicIsTempGrip() {
    const uint8_t cur = Ctx().cur;
    const TouchUnit& u = Cur(cur);
    if (Touch::IsGhostByMinSigOff(cur) || IsGlobalSuppressedClick() || IsRatioOK(0xd)) return true;
    if (!IsLongPressAllowed() && IsLongPress()) return true;
    if ((u.gripFlags & kGripOppositeSide) != 0 && IsRatioOK(0x11)) return true;
    if ((u.persistFlags2 & kPersist2HeavyCorner) != 0 && u.signalScale * 3 < u.maxRefPeakSig * 2) return true;
    if ((u.persistFlags2 & kPersist2HeavyCornerFirstFrame) != 0 && 10 <= u.maxSqDistMmFromStart) return true;
    if (IsRatioOK(IsLongPress() ? 0xe : 0xc)) return true;
    if ((u.gripFlags & kGripElongated) != 0 && IsRatioOK(5)) return true;
    return BasicIsTempGripAbnormal() || IsNegSigTouch();
}

// GripFilter_BasicIsTempGripAbnormal @ 0x6bae3618
// 信号异常或异常抬起的大触点、角落里的重压、跨行多的触点；或按下处刚抬起不久而信号偏弱。
bool BasicIsTempGripAbnormal() {
    if (!IsToCheckAbnormal()) return false;
    const uint8_t cur = Ctx().cur;
    TouchUnit& u = Cur(cur);
    const bool signal = IsSignalOK(cur, 0x10, 0);
    const bool liftOff = IsAbnormalLiftOff();
    if (liftOff || signal) {
        if (0x18 < u.maxMajorMm) {
            u.gripFlags |= kGripAbnormal;
            return true;
        }
        if ((u.persistFlags4 & (kPersist4CloseGrip | kPersist4InnerEdgeNarrow)) != 0) return true;
        if (0xf < u.maxMajorMm && IsHeavyRegionTouched(cur)) return true;
        if (4 < u.dim2Span) return true;
    }
    const PrevTouchUnit& p = PrevG();
    return p.timeOffAtTouchDown < 0x32 && u.signalScale < p.minSigOffAtTouchDown * 2;
}

// GripFilter_IsRatioOK @ 0x6baecdc6：按判据种类比较边缘比例与门限
bool IsRatioOK(uint32_t kind) {
    const TouchUnit& u = CurG();
    const PrevTouchUnit& p = PrevG();
    const GripEdgePrmt& e = EdgePrmt();
    switch (kind) {
        case 1:
        case 2:
        case 6:
        case 7: {
            const uint16_t t = GetEdgeClickRatioThold(kind == 1 ? 2 : 3);
            return t <= (IsOnDim1() ? p.minEdgeRatioDim1 : p.minEdgeRatioDim2);
        }
        case 3:
        case 0xb: return e.ratioThold[2] <= (IsOnDim1() ? p.startEdgeRatio.dim1 : p.startEdgeRatio.dim2);
        case 4:
            if (Sys::IsScreenRotated() && (u.gripFlags & kGripHigherPriority) != 0 && IsNearbyCorner(Ctx().cur)) return true;
            if (IsOnDim1()) return e.ratioThold[1] <= p.maxEdgeRatioDim1 && 0x10 <= p.minEdgeRatioDim1;
            if (!Sys::IsScreenRotated()) return e.ratioThold[1] <= p.minEdgeRatioDim2;
            return e.ratioThold[1] <= p.maxEdgeRatioDim2 && (e.ratioThold[1] >> 1) <= p.minEdgeRatioDim2;
        case 5:
        case 0xc: {
            const uint16_t t = GetEdgeClickRatioThold(0);
            if (IsOnDim1()) return t <= p.minEdgeRatioDim1 && u.maxEdgeDistDim1 <= 0x100;
            return t <= p.minEdgeRatioDim2 && u.maxEdgeDistDim2 <= 0x100;
        }
        case 9: return (u.persistFlags & kPersistWithdrawable) == 0 && e.ratioThold[0] <= p.minEdgeRatioDim1;
        case 10: {
            uint16_t t = e.ratioThold[0];
            if ((u.persistFlags & kPersistWithdrawable) != 0) t += e.ratioThold[0] >> 2;
            return t <= p.iirEdgeRatioDim1;
        }
        case 0xd: return IsOnDim1() && 0x29 <= Now() - p.strongZnTime && e.ratioThold[0] <= u.edgeRatioDim1;
        case 0xe: {
            const uint16_t t = GetEdgeClickRatioThold(0);
            if (IsOnDim1()) return t <= p.iirEdgeRatioDim1;
            return t <= (Sys::IsScreenRotated() ? p.iirEdgeRatioDim2 : p.minEdgeRatioDim2);
        }
        case 0xf: return u.edgeRatioDim1 <= 8 && u.edgeRatioDim2 <= 8;
        case 0x11: return e.ratioThold[2] <= (IsOnDim1() ? p.iirEdgeRatioDim1 : p.iirEdgeRatioDim2);
        default: return true;
    }
}

// GripFilter_IsLongPressAllowed @ 0x6bae28e0
bool IsLongPressAllowed() {
    const TouchUnit& u = CurG();
    if ((u.gripFlags & kGripPalmShape) != 0) return false;
    if (IsRatioOK(6) && (u.gripFlags & kGripElongated) != 0) return false;
    if (u.minTimeHand < 5000 && IsRatioOK(0xb)) return false;
    if (IsRatioOK(0xe) || (u.gripFlags & kGripGrab) != 0) return false;
    if (!IsCornerLongPressAllowed() || !IsVideoModeLongPressAllowed() || !IsScreenRotatedStateLongPressAllowed()) return false;
    if (BasicIsTempGripAbnormal()) return false;
    return (u.gripFlags & kGripAbnormal) == 0;
}

// GripFilter_IsLongPress @ 0x6bae675f：按住超过 300 ms（视频模式或角落里消抖较久时 200 ms）
bool IsLongPress() {
    const TouchUnit& u = CurG();
    if (!IsVideoMode() && (u.gripFlags & kGripCornerDebounced) == 0) return 299 < u.durationMs;
    return 199 < u.durationMs;
}

// GripFilter_IsSuppressedGlobally2 @ 0x6bae2a9d：附近有新触点、边缘比例够，横屏时优先或靠着可撤下的触点
bool IsSuppressedGlobally2() {
    const uint32_t f = CurG().gripFlags;
    if ((f & kGripNewTouchNearby) != 0 && IsRatioOK(2)) {
        if (Sys::IsScreenRotated() && (f & kGripHigherPriority) != 0) return true;
        if ((f & kGripNearWithdrawable) != 0) return true;
    }
    if ((f & kGripNearWithdrawable) != 0 && 0x19 <= CurG().maxMajorMm) return true;
    return Sys::IsScreenRotated() && (f & kGripNearWithdrawable) != 0 && TouchesSide().reportCount != 0 && IsRatioOK(2);
}

// GripFilter_IsSuppressedGlobally @ 0x6bae3211
bool IsSuppressedGlobally() { return IsSuppressedByHandGesture() || IsSuppressedByCoreTouch(); }

// GripFilter_BasicIsAbsGripPrmt @ 0x6bae3c2f：握持区域里形状 IIR 的长轴超过本等级的门限
bool BasicIsAbsGripPrmt() {
    const LevelPrmt& prmt = CurPrmt();
    if (prmt.absGripEnabled == 0) return false;
    uint16_t thold = GetAbsThold(prmt.absTholdPercent);
    if (IsVideoMode()) thold = 0x14;
    const TouchUnit& u = CurG();
    return (u.shapeIirValid != 0 || Touch::IsOff(Ctx().cur)) && thold < u.iirMajorMm;
}

// GripFilter_BasicIsAbsGripAbnormal @ 0x6bae3d07
bool BasicIsAbsGripAbnormal() {
    const uint8_t cur = Ctx().cur;
    const TouchUnit& u = Cur(cur);
    if ((u.persistFlags2 & kPersist2MergedOrSplit) != 0 && IsToCheckAbnormal() && (u.gripFlags & kGripIdMappingCancelled) == 0) return true;
    if ((u.gripFlags & kGripRawJump) != 0) return true;
    if ((u.gripFlags & kGripCloseInTime) != 0 && 1 < GetCenterLevel(cur)) return true;
    if (BasicIsTempGripAbnormal() && IsLongPress()) return true;
    if ((u.gripFlags & kGripNoQuality) == 0 && IsLongPress()) return true;
    return BasicIsTempGripAbnormal() && 5 <= u.age && 0x19 <= u.majorMm;
}

// GripFilter_IsNegSigTouchBypass @ 0x6bae3f02：负信号触点已经变大或按久了
bool IsNegSigTouchBypass() {
    const TouchUnit& u = CurG();
    return PrmtDynamic().skipZ8CheckOnInnerEdge != 0 && u.hiSkipZ8 != 0 && (0xf <= u.maxMajorMm || 1000 <= u.durationMs);
}

// GripFilter_IsCornerGripClick @ 0x6bae7060
bool IsCornerGripClick() {
    const TouchUnit& u = CurG();
    return (u.persistFlags2 & kPersist2HeavyCornerFirstFrame) != 0 && (u.persistFlags2 & kPersist2HeavyCornerOff) != 0 &&
           (u.gripFlags & kGripNewTouchNearby) != 0 && (u.gripFlags & kGripHigherPriority) != 0 && 6 <= u.maxSqDistMmFromStart;
}

// GripFilter_IsNearbyCorner @ 0x6baea708：两个方向的边缘比例都不为 0 且和不小于 8
bool IsNearbyCorner(uint8_t curIdx) {
    const TouchUnit& u = Cur(curIdx);
    return u.edgeRatioDim1 != 0 && u.edgeRatioDim2 != 0 && 8 <= u.edgeRatioDim1 + u.edgeRatioDim2;
}

// GripFilter_IsOnDim2 @ 0x6baec7e8：贴着上 / 下边
bool IsOnDim2() { return (CurG().touchSide & (kSideTop | kSideBottom)) != 0; }

// GripFilter_IsBackToGripAllowed @ 0x6bae1f52
bool IsBackToGripAllowed() { return (CurG().gripFlags & (kGripPeakAbsNotGrip | kGripMergedGrip)) != 0; }

// GripFilter_IsBackToGripSatisfied @ 0x6bae1fd5：区域信号掉到上一帧的 1/3 以下且峰在最外圈，或贴边
bool IsBackToGripSatisfied() {
    const TouchUnit& u = CurG();
    if (u.zoneSignalScale < PrevG().signalScale / 3 && Peak::IsOnOuterEdge(u.peakIdx)) return true;
    return u.edgeDistDim1 < 0x20 || u.edgeDistDim2 < 0x20;
}

// GripFilter_IsStuckSideTouch @ 0x6bae223a：贴边且所在格握持超过 500 ms，或落在动态死区里
bool IsStuckSideTouch() {
    if (!IsToCheckStuckSideTouch()) return false;
    const TouchUnit& u = CurG();
    const auto cell = static_cast<uint16_t>((u.pos.dim1 >> 8) + (u.pos.dim2 >> 8) * PrmtFlash().dim1Len);
    if (500 < Buffers().gripTime[cell] && (u.edgeDistDim1 < 0x20 || u.edgeDistDim2 < 0x20)) return true;
    return IsInDynamicDeadZone(0x20);
}

// GripFilter_IsSeparated @ 0x6bae864d：握持触点从边缘向内分出来
bool IsSeparated(uint8_t curIdx, uint8_t prevIdx) {
    const TouchUnit& u = Cur(curIdx);
    if ((u.gripFlags & kGripIdMappingCancelled) == 0 && (CurG().gripFlags & kGripPeakAbsNotGrip) == 0) return false;
    if (0x100 < u.edgeDistDim1) return false;
    const PrevTouchUnit& p = Prev(prevIdx);
    if (p.edgeDistDim1 < u.edgeDistDim1 + static_cast<uint16_t>(TouchSize::MmToPixel(4))) return false;
    if ((u.touchSide & (kSideLeft | kSideRight)) == 0) return false;
    return PrmtFlash().gripEdge[Report::SideToEdge(p.touchSide)].ratioThold[2] <= u.edgeRatioDim1;
}

// GripFilter_GetDelayTime @ 0x6bae4796：判握持前等待的时间（ms）
uint16_t GetDelayTime() {
    if (Report::IsWindowsPadFeatureEnabled()) return 0x32;
    if (IsToApplyMaxDelay()) return 600;
    if (!IsToApplyDelay() && IsToBypassDelay()) return 0;
    return CurPrmt().delayMs;
}

// GripFilter_BasicIsFlick @ 0x6bae81d7：轻扫；不允许轻扫时记下 kGripFlickBlocked
bool BasicIsFlick() {
    if (IsFlickDetected() && !IsFlickAllowed()) CurG().gripFlags |= kGripFlickBlocked;
    return IsFlickAllowed() && IsFlickDetected();
}


// GripFilter_IsOnDim1 @ 0x6baec770：贴着左 / 右边
bool IsOnDim1() { return (CurG().touchSide & (kSideLeft | kSideRight)) != 0; }

namespace {

// GripFilter_GetClickGripRatio @ 0x6baec860：点击的边缘比例门限（竖屏贴上 / 下边且 mode 为 1 时不判）
uint16_t GetClickGripRatio() {
    if (!Sys::IsScreenRotated() && IsOnDim2() && EdgePrmt().mode == 1) return 0xff;
    return EdgePrmt().ratioThold[0];
}

// GripFilter_IsEnableWeakenCornerClickSupress @ 0x6baec9fe / GripFilter_WeakenCornerClickSupress @ 0x6baeca55
bool IsEnableWeakenCornerClickSupress() { return PrmtFlash().gripWeakenCornerClick != 0 && CurG().durationMs <= 299; }
void WeakenCornerClickSupress(uint8_t& level) {
    const TouchUnit& u = CurG();
    if (1 < u.edgeWidthDim1 && 1 < u.edgeWidthDim2) ++level;
}

// GripFilter_GetRatioTholdLevelExtra @ 0x6baec8e8：长按与视频模式至少用等级 1
void GetRatioTholdLevelExtra(uint8_t& level) {
    const TouchUnit& u = CurG();
    if (IsLongPress()) {
        if (level == 0) level = 1;
        if (level < 2 && Sys::IsScreenRotated() && !Scenario::IsGame() && IsOnDim2() && 0xe < u.maxMajorMm && 0x1f < u.iirAxisRatioX16) {
            level = 2;
        }
    }
    if (IsVideoMode() && level == 0 && (IsSignalOK(Ctx().cur, 1, false) || IsOnDim2())) level = 1;
}

// GripFilter_IsCoreTouchChangeInTime @ 0x6bae2c51：本触点按下消抖开始的时间与最近一次对外按下 / 抬起相差不到 ms
// （原代码只取时间的低 32 位）
bool IsCoreTouchChangeInTime(uint16_t ms) {
    const auto start = static_cast<uint32_t>(PrevG().tdDebounceTime);
    const auto within = [&](uint64_t time) {
        const auto t = static_cast<uint32_t>(time);
        const auto hi = static_cast<int32_t>(std::max(start, t));
        const auto lo = static_cast<int32_t>(std::min(start, t));
        return static_cast<uint64_t>(static_cast<int64_t>(hi) - lo) < ms;
    };
    const TouchTable& tt = Touches();
    return within(tt.lastTouchDownTime) || within(tt.lastLiftOffTime);
}

// GripFilter_GetSafeTouchThold @ 0x6bae5d10
int16_t GetSafeTouchThold() { return static_cast<int16_t>(PrmtDynamic().touchThold * 2 / 5); }

// GripFilter_IsToApplyMaxDelayOnHeavyCorner @ 0x6bae440b
bool IsToApplyMaxDelayOnHeavyCorner() {
    const TouchUnit& u = CurG();
    if ((u.persistFlags4 & kPersist4HeavyCornerTouched) == 0) return false;
    return 0xe < u.maxMajorMm || u.edgeDistDim1 < 0x181 || u.edgeDistDim2 < 0x181 || 0x10 < u.maxGripRatio;
}

// GripFilter_GetSafeDistToEdgeThold @ 0x6bae5d57 / GripFilter_IsSideTouch @ 0x6bae5d8e
uint16_t GetSafeDistToEdgeThold() { return (Static().featureFlags & kFeatSideTouch) != 0 ? 0x1c0 : 0x100; }
bool IsSideTouch() { return (Static().featureFlags & kFeatSideTouch) != 0 && IsRatioOK(9); }

}  // namespace

// GripFilter_GetEdgeClickRatioThold @ 0x6baecad0：按触点大小、形状、角落与长按把门限等级往上调
uint16_t GetEdgeClickRatioThold(uint8_t level) {
    const uint8_t cur = Ctx().cur;
    const TouchUnit& u = Cur(cur);
    uint8_t lv = level;
    if (0x11 < u.maxMajorMm) {
        lv = 0x2f < u.iirAxisRatioX16 ? level + 2 : level + 1;
        if (0x18 < u.maxMajorMm) ++lv;
        if ((u.persistFlags2 & kPersist2BothEdges) != 0 && IsHeavyRegionTouched(cur)) ++lv;
        if (IsLongPress()) ++lv;
    }
    if (IsHeavyRegionTouched(cur) && (u.persistFlags2 & kPersist2HeavyCornerFirstFrame) != 0 && PrmtFlash().gripDevice != kDevicePad) {
        if (IsEnableWeakenCornerClickSupress()) {
            WeakenCornerClickSupress(lv);
        } else {
            lv += 0x20 < u.maxInnerCornerSigRatio ? 2 : 1;
            if (0x30 < u.maxInnerCornerSigRatio) ++lv;
        }
    }
    GetRatioTholdLevelExtra(lv);
    uint16_t thold;
    switch (lv) {
        case 0: thold = GetClickGripRatio(); break;
        case 1: thold = EdgePrmt().ratioThold[1]; break;
        case 2: thold = EdgePrmt().ratioThold[2]; break;
        default: thold = 0x10; break;
    }
    return 0xfe < thold ? 0xffff : thold;
}

// GripFilter_IsCornerLongPressAllowed @ 0x6bae25b3
bool IsCornerLongPressAllowed() {
    const uint8_t cur = Ctx().cur;
    const TouchUnit& u = Cur(cur);
    if (9 < u.maxSqDistMmFromStart && ((u.gripFlags & kGripInCornerFirstFrame) != 0 || IsNearbyCorner(cur))) return false;
    if (5 <= u.maxSqDistMmFromStart && (u.persistFlags2 & kPersist2HeavyCornerFirstFrame) != 0) return false;
    return Sys::IsScreenRotated() || Is2HandGame() || Scenario::IsTyping() || (u.gripFlags & kGripCornerDebounced) == 0;
}

// GripFilter_IsVideoModeLongPressAllowed @ 0x6bae2720
bool IsVideoModeLongPressAllowed() {
    if (!IsVideoMode() || !IsNearbyCorner(Ctx().cur)) return true;
    const TouchUnit& u = CurG();
    return (u.persistFlags2 & kPersist2HeavyCorner) == 0 && u.maxMajorMm <= 0xe && (u.gripFlags & kGripNewTouchNearby) == 0;
}

// GripFilter_IsScreenRotatedStateLongPressAllowed @ 0x6bae280c
bool IsScreenRotatedStateLongPressAllowed() {
    if (!Sys::IsScreenRotated() || !(IsOnDim2() || IsNearbyCorner(Ctx().cur)) || Is2HandGame() || !IsRatioOK(6)) return true;
    const TouchUnit& u = CurG();
    return u.maxMajorMm <= 0xe && (u.gripFlags & kGripNewTouchNearby) == 0;
}

// GripFilter_IsAbnormalLiftOff @ 0x6bae340f：按下处刚抬起不久，或那里抬起时的最小信号偏强
bool IsAbnormalLiftOff() {
    const PrevTouchUnit& p = PrevG();
    if (p.timeOffAtTouchDown < 0x32 && 0x32 < p.timeRptAtTouchDown - p.timeOffAtTouchDown) return true;
    return PrmtDynamic().sigThold * 3 < p.minSigOffAtTouchDown * 4;
}

// GripFilter_IsToCheckAbnormal @ 0x6bae3513：握持区域里不到 3 列宽、峰在边缘或已无效的触点
bool IsToCheckAbnormal() {
    const uint8_t cur = Ctx().cur;
    if (GetCenterLevel(cur) == 0 || 3 <= CurG().dim1Span) return false;
    const uint8_t peak = CurG().peakIdx;
    return !Peak::IsTouchValid(cur) || Peak::IsOnOuterEdgeDim1(peak) || Peak::IsOnInnerEdgeDim1(peak);
}

// GripFilter_IsSignalOK @ 0x6baed803：按判据种类看信号是否偏弱（0x11 反过来看是否够强）
bool IsSignalOK(uint8_t curIdx, uint32_t kind, bool checkState) {
    const TouchUnit& u = Cur(curIdx);
    if (checkState && (u.event < TouchEvent::kTouchDown || u.age < 3 || (u.volatileFlags & kVolatileSigSumSpread) != 0)) return false;
    const int sig = u.signalScale;
    const int hti = static_cast<int16_t>(Static().htiThold);
    const auto weakToRef = [&] { return sig < u.maxRefPeakSig * 2 / 3 || sig < PrmtDynamic().touchThold * 2 / 5; };
    switch (kind) {
        case 1: return weakToRef();
        case 8: return u.zoneMaxDim2 == PrmtFlash().dim2Len - 1 || sig < hti * 2 / 3;
        case 0x10: return sig < hti * 2 / 3 || weakToRef();
        case 0x11: return hti * 2 / 5 <= sig && u.maxRefPeakSig / 2 <= sig;
        default: return true;
    }
}

// GripFilter_IsSuppressedByHandGesture @ 0x6bae2dd7：屏内触点刚按下 / 抬起时的边缘触点，或与握持手势关联的触点
bool IsSuppressedByHandGesture() {
    const uint8_t cur = Ctx().cur;
    const TouchUnit& u = Cur(cur);
    const uint16_t window = PrmtFlash().gripCoreTouchChangeMs;
    if (u.minTimeHand < 5000) {
        if (std::abs(u.minTimeHand - Touch::GetTimeOn(cur)) < window) return true;
        if (IsCoreTouchChangeInTime(window) && ((u.gripFlags & kGripHandRelated) != 0 || IsRatioOK(0xb) || IsSignalOK(cur, 1, true))) {
            return true;
        }
    }
    if ((u.gripFlags & kGripHandRelated) == 0) return false;
    return (u.persistFlags4 & kPersist4HeavyCornerTouched) != 0 || (IsPad() && IsRatioOK(0xb));
}

// GripFilter_IsSuppressedByCoreTouch @ 0x6bae2fb7
bool IsSuppressedByCoreTouch() {
    if (Is2HandGame()) return false;
    const uint8_t cur = Ctx().cur;
    const TouchUnit& u = Cur(cur);
    if (IsCoreTouchChangeInTime(PrmtFlash().gripCoreTouchChangeMs)) {
        if ((u.gripFlags & kGripCloseInPalm) != 0) return true;
        if ((u.gripFlags & kGripHandRelated) != 0 && IsRatioOK(0xb) && ((u.persistFlags4 & kPersist4HeavyCornerTouched) != 0 || IsPad())) {
            return true;
        }
        if ((Sys::IsScreenRotated() || IsPad()) && (u.gripFlags & kGripNearWithdrawable) != 0 && IsRatioOK(0xb)) return true;
        if ((u.gripFlags & kGripNearWithdrawable) != 0) {
            if (IsSignalOK(cur, 1, true) && IsRatioOK(0xb)) return true;
            if (IsCoreTouchChangeInTime(0x32) && !Scenario::IsTyping() && (u.persistFlags4 & kPersist4HeavyCornerTouched) != 0) return true;
        }
    }
    return IsCoreTouchChangeInTime(100) && IsRatioOK(0xb);
}

// GripFilter_GetAbsThold @ 0x6bae65e3
uint16_t GetAbsThold(uint16_t percent) { return static_cast<uint16_t>(percent + (percent >> 2)); }

// GripFilter_IsToCheckStuckSideTouch @ 0x6bae2113：大触点、区域信号掉得厉害，或贴边的单个触点
bool IsToCheckStuckSideTouch() {
    const TouchUnit& u = CurG();
    if (0x1a <= u.iirMajorMm || u.zoneSignalScale < PrevG().signalScale / 3) return true;
    return u.maxEdgeDistDim1 < 0x200 && !Is2HandGame() && !Scenario::IsTyping() && 1 < PrevTouches().outCount;
}

// GripFilter_IsToApplyMaxDelay @ 0x6bae4536
bool IsToApplyMaxDelay() {
    const TouchUnit& u = CurG();
    if ((u.persistFlags2 & kPersist2IdleWeak) != 0 || (u.gripFlags & (kGripCloseInTime | kGripInCorner)) != 0) return true;
    if (IsToApplyMaxDelayOnHeavyCorner() || IsRatioOK(4) || u.minTimeHand < 5000) return true;
    if (!IsLongPressAllowed() || IsAbnormalLiftOff()) return true;
    return u.signalScale < GetSafeTouchThold();
}

// GripFilter_IsToApplyDelay @ 0x6bae4695
bool IsToApplyDelay() {
    const uint8_t cur = Ctx().cur;
    return IsNearbyCorner(cur) || IsRatioOK(2) || IsSignalOK(cur, 0x10, true) || 0xf <= CurG().maxMajorMm;
}

// GripFilter_IsToBypassDelay @ 0x6bae4740
bool IsToBypassDelay() {
    if (Scenario::IsTyping() && Ctx().absGripEnterCount == 0) return true;
    return Ctx().quality == kQualityAbsNotGrip || Ctx().quality == kQualityNone;
}

// GripFilter_IsFlickAllowed @ 0x6bae7dbd
bool IsFlickAllowed() {
    const TouchUnit& u = CurG();
    if ((u.persistFlags2 & kPersist2HeavyCornerFirstFrame) != 0 && (u.volatileFlags & kVolatileMultiGripRegion) != 0) return false;
    if (u.edgeDistDim1 < GetSafeDistToEdgeThold() && !Sys::IsScreenRotated() && (u.gripFlags & kGripAboveGrip) != 0) return false;
    if ((u.gripFlags & kGripMulti) != 0 && (1000 < u.durationMs || (u.persistFlags2 & kPersist2HeavyCornerFirstFrame) != 0)) return false;
    if (IsSideTouch() || IsInDynamicDeadZone(0x20)) return false;
    if ((u.gripFlags & kGripNearWithdrawable) != 0 && 0xf <= u.maxMajorMm) return false;
    if (!IsVideoMode()) return true;
    if (IsRatioOK(6) && (u.gripFlags & kGripNewTouchNearby) != 0) return false;
    return (u.gripFlags & (kGripFlickBlocked | kGripMulti)) == 0;
}

// GripFilter_IsFlickDetected @ 0x6bae80e0：从首次报点起移动超过门限（与 3/4 长轴取大）
bool IsFlickDetected() {
    const uint32_t size = CurG().maxMajorMm * 3u >> 2;
    const uint32_t thold = std::max<uint32_t>(PrmtFlash().gripFlickDistMm, size);
    return thold * thold < PrevG().rptSqDistMmFromFirstRpt;
}

}  // namespace Solvers::Tsa::Core::GripFilter
