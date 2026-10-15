#include "Rejection/TouchReport/TouchReport.h"

#include "Alg/Baseline/Baseline.h"
#include "Alg/Filter/Filter.h"
#include "Alg/HotZoneAdjust/HotZoneAdjust.h"
#include "Alg/Touch/PrevTouch.h"
#include "Alg/Touch/Touch.h"
#include "Alg/Touch/TouchData.h"
#include "App/TsaStatic.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::TouchReport {

namespace {

constexpr uint32_t kMultiStepBack = 0x2;            // FlashPrmt::touchReportFlags

constexpr uint32_t kFeatHideByMode8 = 0x1000;       // TsaStatic::featureFlags：按 suppressFlags 0x8 / 0x20 不报
constexpr uint32_t kFeatHideByMode20 = 0x2000;

constexpr uint32_t kStateBlResetDebounce = 0x8;     // TouchTable::stateFlags：基线复位消抖期间不报新点

constexpr uint32_t kPersistReported = 0x8;          // TouchUnit::persistFlags
constexpr uint32_t kPersistHistoryReplay = 0x80;
constexpr uint32_t kPersistRemoved = 0x10000;
constexpr uint32_t kPersistFastClick = 0x40000;
constexpr uint32_t kPersistReportedOnce = 0x100000;
constexpr uint32_t kPersistReplayedLift = 0x200000;
constexpr uint32_t kPersistSteppedBack = 0x800000;
constexpr uint32_t kPersistWithdrawable = 0x20000000;
constexpr uint32_t kPersist3PostureCancel = 0x1;    // TouchUnit::persistFlags3：被姿态判为误触
constexpr uint32_t kVolatileLiftOff = 0x2000;       // TouchUnit::volatileFlags
constexpr uint32_t kVolatileEdgeTouch = 0x20000;
constexpr uint32_t kVolatileHidden = 0x20000000;
constexpr uint32_t kVolatile2Hold = 0x1;            // TouchUnit::volatileFlags2
constexpr uint32_t kSuppressWeak = 0x2;             // TouchUnit::suppressFlags
constexpr uint32_t kSuppressStylus = 0x4;
constexpr uint32_t kSuppressNotStylus = 0x8;
constexpr uint32_t kSuppressMode20 = 0x20;
constexpr int32_t kExcRegisterOnly = 0x40;          // TouchUnit::exceptionType：只登记、不报点

constexpr uint32_t kEventTouchDown = 0x2;           // TouchUnit::event
constexpr uint32_t kEventMove = 0x4;
constexpr uint32_t kEventHold = 0x10;
constexpr uint32_t kEventNoReport = 0x21;
constexpr uint32_t kEventLiftMask = 0x30;
constexpr uint32_t kEventNoHistory = 0x38;

constexpr uint8_t kStepBackSteps = 6;
constexpr uint8_t kStepBackDone = 7;
constexpr int16_t kStepBackSmallMove = 100;

TouchUnit& Cur(uint8_t idx) { return Touches().unit[idx]; }
PrevTouchUnit& Prev(uint8_t idx) { return PrevTouches().unit[idx]; }

// TouchReport_IsMultStepBackEnable @ 0x6ba7a3b0
bool MultiStepBackEnabled() { return (PrmtFlash().touchReportFlags & kMultiStepBack) != 0; }

/// 回退的目标：未报过点时回到按下消抖的起点（尚未判为移动时）或消抖结束的位置，报过点时回到首报位置。
Point BackTarget(const TouchUnit& u, const PrevTouchUnit& p) {
    if ((u.persistFlags & kPersistReported) != 0) return p.firstRptPos;
    return u.event < kEventTouchDown ? p.startPos : p.tdDebouncePos;
}

// TouchReport_IsAllTouchBack @ 0x6ba7a3d6：可撤下的触点都已退完（且至少有一个）
bool IsAllTouchBack() {
    bool any = false;
    for (uint8_t k = 0; k < Touches().count; ++k) {
        const uint8_t prevIdx = PrevTouch::GetIdxBasedOnCurIdx(k);
        if (kMaxTouchNum <= prevIdx || (Prev(prevIdx).persistFlags & kPersistWithdrawable) == 0) continue;
        if (Prev(prevIdx).stepBackCount < kStepBackDone) return false;
        any = true;
    }
    return any;
}

// TouchReport_IsAnyTouchCancelByPosture @ 0x6ba7a487：有被姿态判为误触、正在回退的触点
bool IsAnyTouchCancelByPosture() {
    if (!MultiStepBackEnabled()) return false;
    for (uint8_t k = 0; k < Touches().count; ++k) {
        const uint8_t prevIdx = PrevTouch::GetIdxBasedOnCurIdx(k);
        if (kMaxTouchNum <= prevIdx) continue;
        const PrevTouchUnit& p = Prev(prevIdx);
        if (((Cur(k).persistFlags & kPersistSteppedBack) != 0 || (p.persistFlags & kPersistSteppedBack) != 0) &&
            (p.persistFlags3 & kPersist3PostureCancel) != 0) {
            return true;
        }
    }
    return false;
}

// TouchReport_GoBackOneStep @ 0x6ba7a134
void GoBackOneStep(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& u = Cur(curIdx);
    u.persistFlags |= kPersistSteppedBack;
    u.pos = BackTarget(u, Prev(prevIdx));
}

// TouchReport_IsReadyToCancel @ 0x6ba7a577（区域切换时间来自被裁掉的折叠屏模块，恒为 0，那两个判据不成立）
// 上一帧没有报点，或只报了一个点且回退量不大，或所有可撤下的触点都已退完时，直接退到目标。
bool IsReadyToCancel(uint8_t prevIdx) {
    if (IsAnyTouchCancelByPosture()) return true;
    const uint8_t prevReports = PrevTouches().reportCount;
    if (prevReports == 0) return true;
    const SPoint d = Prev(prevIdx).stepBackDelta;
    if (prevReports == 1 && -kStepBackSmallMove < d.dim1 && d.dim1 < kStepBackSmallMove && -kStepBackSmallMove < d.dim2 &&
        d.dim2 < kStepBackSmallMove) {
        return true;
    }
    return IsAllTouchBack();
}

// TouchReport_GoBackMultiStep @ 0x6ba7a79b：第一次时记下目标与位移，之后每帧退 1/6，退完或可以直接取消时到目标
void GoBackMultiStep(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& u = Cur(curIdx);
    PrevTouchUnit& p = Prev(prevIdx);
    if (p.stepBackOrigin.dim1 == 0 && p.stepBackOrigin.dim2 == 0) {
        p.stepBackOrigin = BackTarget(u, p);
        p.stepBackDelta.dim1 = static_cast<int16_t>(static_cast<int16_t>(u.pos.dim1) - static_cast<int16_t>(p.stepBackOrigin.dim1));
        p.stepBackDelta.dim2 = static_cast<int16_t>(static_cast<int16_t>(u.pos.dim2) - static_cast<int16_t>(p.stepBackOrigin.dim2));
    }
    if (IsReadyToCancel(prevIdx)) {
        u.pos = p.stepBackOrigin;
        u.persistFlags |= kPersistSteppedBack;
        p.stepBackCount = kStepBackDone;
        return;
    }
    if (kStepBackDone <= p.stepBackCount) return;
    const int remain = kStepBackSteps - p.stepBackCount;
    u.pos.dim1 = static_cast<uint16_t>(static_cast<int16_t>(p.stepBackOrigin.dim1) + remain * p.stepBackDelta.dim1 / kStepBackSteps);
    u.pos.dim2 = static_cast<uint16_t>(static_cast<int16_t>(p.stepBackOrigin.dim2) + remain * p.stepBackDelta.dim2 / kStepBackSteps);
    ++p.stepBackCount;
}

// TouchReport_PostureAntiTouch @ 0x6ba7ae86（传感器姿态判据被裁掉，恒不成立）
// 姿态误触时回退坐标；不可撤下的触点直接移除。返回 true 表示本帧不报。
bool PostureAntiTouch(uint8_t curIdx, uint8_t prevIdx) {
    const PrevTouchUnit& p = Prev(prevIdx);
    if (!IsAnyTouchCancelByPosture() && (p.persistFlags3 & kPersist3PostureCancel) == 0) return false;
    TouchUnit& u = Cur(curIdx);
    u.persistFlags3 |= kPersist3PostureCancel;
    GoBack(curIdx, prevIdx);
    if ((p.persistFlags & kPersistWithdrawable) != 0) return false;
    u.persistFlags |= kPersistRemoved;
    return true;
}

// TouchReport_ToBeReported @ 0x6ba7afbc
bool ToBeReported(uint8_t curIdx, uint8_t prevIdx) {
    TouchTable& tt = Touches();
    TouchUnit& u = tt.unit[curIdx];
    const PrevTouchUnit& p = Prev(prevIdx);
    const uint32_t feat = Static().featureFlags;
    const bool smartCover = (feat & kFeatSmartCover) != 0;
    bool report = !PostureAntiTouch(curIdx, prevIdx);
    if ((tt.stateFlags & kStateBlResetDebounce) != 0 && Baseline::ResetInDebounce() &&
        (!smartCover || (u.volatileFlags & kVolatileEdgeTouch) != 0)) {
        report = false;
    }
    if ((u.volatileFlags & kVolatileHidden) != 0) report = false;
    if (u.exceptionType == kExcRegisterOnly && !smartCover) report = false;
    if ((u.volatileFlags2 & kVolatile2Hold) != 0 && u.event != kEventHold) report = false;
    // 还没报过点的（快速点击且非异常的除外）：信号弱或判为笔时不报
    if ((p.persistFlags & kPersistReported) == 0 &&
        ((p.persistFlags & kPersistFastClick) == 0 || Touch::IsException(curIdx, prevIdx))) {
        if ((u.suppressFlags & kSuppressWeak) != 0 && u.signalScale < static_cast<int16_t>(PrmtFlash().peakFilterReferenceThreshold) / 2 &&
            !smartCover) {
            report = false;
        }
        if ((u.suppressFlags & kSuppressStylus) != 0) report = false;
    }
    if ((u.suppressFlags & kSuppressNotStylus) != 0 && (feat & kFeatHideByMode8) != 0) report = false;
    if ((u.suppressFlags & kSuppressMode20) != 0 && (feat & kFeatHideByMode20) != 0) report = false;
    if ((u.event & kEventNoReport) != 0) report = false;
    if ((u.volatileFlags & kVolatileLiftOff) != 0) {
        if (u.exceptionType != kExcRegisterOnly) ++tt.liftOffCount;
        report = false;
    }
    return report;
}

// TouchReport_IsHistoryBufferApplied @ 0x6ba7b373
// 记下本帧位置；要求回放时取出最早的历史位置作为本帧坐标（原坐标经 orig 带回），返回是否回放。
bool ApplyHistory(uint8_t curIdx, uint8_t prevIdx, Point& orig) {
    TouchUnit& u = Cur(curIdx);
    if ((u.event & kEventNoHistory) == 0) {
        HistoryEntry e{};
        e.pos = {static_cast<int16_t>(u.pos.dim1), static_cast<int16_t>(u.pos.dim2)};
        e.time = u.timestamp;
        PrevTouch::HistoryBufferPush(prevIdx, e);
    }
    if ((u.persistFlags & kPersistHistoryReplay) == 0) return false;
    HistoryEntry e{};
    if (!PrevTouch::HistoryBufferPop(prevIdx, e)) return false;
    orig = u.pos;
    u.pos = {static_cast<uint16_t>(e.pos.dim1), static_cast<uint16_t>(e.pos.dim2)};
    if ((u.persistFlags & kPersistReported) == 0) u.event = kEventTouchDown;
    if ((u.event & kEventLiftMask) != 0) {
        u.persistFlags |= kPersistReplayedLift;
        u.event = kEventMove;
    }
    return true;
}

// TouchReport_UpdateStartReportPoint @ 0x6ba7b657：第一次报点的位置
void UpdateStartReportPoint(uint8_t curIdx, uint8_t prevIdx) {
    const Point rpt = Cur(curIdx).rptPos;
    PrevTouchUnit& p = Prev(prevIdx);
    p.firstRptPos = rpt;
    p.falseFlickPos = rpt;
    p.falseFlickOffset = {static_cast<int16_t>(rpt.dim1), static_cast<int16_t>(rpt.dim2)};
}

// TouchReport_ProcessUnit @ 0x6ba7b86e
void ProcessUnit(uint8_t curIdx, uint8_t prevIdx) {
    TouchTable& tt = Touches();
    TouchUnit& u = tt.unit[curIdx];
    Point orig{};
    const bool report = ToBeReported(curIdx, prevIdx);
    const bool replayed = ApplyHistory(curIdx, prevIdx, orig);
    if (report || replayed) {
        ++tt.reportCount;
        u.persistFlags |= kPersistReported | kPersistReportedOnce;
    } else {
        u.persistFlags &= ~kPersistReported;
    }
    Filter::ProcessUnit(curIdx, prevIdx);
    if (MultiStepBackEnabled() && IsAnyTouchCancelByPosture()) {
        u.rptPos = Prev(prevIdx).stepBackOrigin;
        u.persistFlags |= kPersistSteppedBack;
    }
    if (replayed) u.pos = orig;
    const bool reported = (u.persistFlags & kPersistReported) != 0;
    if (reported && (Prev(prevIdx).persistFlags & kPersistReported) == 0) UpdateStartReportPoint(curIdx, prevIdx);
    if (reported) {
        Touch::GetRptSqDistMmFromStartPos(curIdx, prevIdx);
        Touch::GetRptSqDistMmFromPrevPos(curIdx, prevIdx);
        Touch::GetRptSqDistDisplayPixelFromPrevPos(curIdx, prevIdx);
    }
    Touch::GetFltSqDistPixelFromPrevPos(curIdx, prevIdx);
    HotZoneAdjust::Process(curIdx, prevIdx);
}

}  // namespace

// TouchReport_Init @ 0x6ba7a120
void Init() { HotZoneAdjust::Init(); }

// TouchReport_Process @ 0x6ba7be73：所有可撤下的触点都已退完时，报点坐标停在回退目标
void Process() {
    TouchTable& tt = Touches();
    tt.reportCount = 0;
    for (uint8_t k = 0; k < tt.count; ++k) ProcessUnit(k, PrevTouch::GetIdxBasedOnCurIdx(k));
    if (!MultiStepBackEnabled() || !IsAllTouchBack()) return;
    for (uint8_t k = 0; k < tt.count; ++k) {
        const uint8_t prevIdx = PrevTouch::GetIdxBasedOnCurIdx(k);
        if (prevIdx < kMaxTouchNum) tt.unit[k].rptPos = Prev(prevIdx).stepBackOrigin;
    }
}

// TouchReport_GoBackReport @ 0x6ba7ae1d
void GoBack(uint8_t curIdx, uint8_t prevIdx) {
    if (MultiStepBackEnabled() && (Cur(curIdx).persistFlags3 & kPersist3PostureCancel) != 0) {
        GoBackMultiStep(curIdx, prevIdx);
    } else {
        GoBackOneStep(curIdx, prevIdx);
    }
}

}  // namespace Solvers::Tsa::Core::TouchReport

// raw 的调用方（AntiTouch）
extern "C" void TouchReport_GoBackReport(uint8_t curIdx, uint8_t prevIdx) {
    Solvers::Tsa::Core::TouchReport::GoBack(curIdx, prevIdx);
}
