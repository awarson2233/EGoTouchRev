// GripFilter 主流程：逐触点的属性 → 质量 → 阶段迁移，握持触点的报点标记，以及对外的查询接口。

#include "Rejection/GripFilter/GripFilter.h"

#include "Alg/Baseline/Baseline.h"
#include "Alg/Peak/Peak.h"
#include "Alg/Peak/PeakData.h"
#include "Alg/Touch/PrevTouch.h"
#include "Alg/Touch/Touch.h"
#include "App/Buffer.h"
#include "App/Report.h"
#include "App/Scenario.h"
#include "App/Sys.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Alg/TouchSize/TouchSize.h"
#include "Extension/HandGesture/HandGesture.h"
#include "Rejection/GripFilter/GripFilterInternal.h"

namespace Solvers::Tsa::Core::GripFilter {

namespace {

constexpr uint32_t kFeatGripPost = 0x20;          // TsaStatic::featureFlags：握持后处理（合并、报点、IDLE）
constexpr uint32_t kVolatileGripHeld = 0x8000000;  // TouchUnit::volatileFlags：握持中，暂不报点
constexpr uint32_t kEdgeMergeHandler = 0x2;        // GripEdgePrmt::flags
constexpr uint64_t kIdleAfterGripMs = 500;

constexpr LevelPrmt kLevelPrmt[4] = {{0, 0, 0}, {1, 0, 25}, {2, 30, 20}, {3, 100, 20}};

Context g_ctx;
uint64_t g_inactiveTime = UINT64_MAX;  // 最近一次不是全部绝对握持的时间
bool g_okToIdle;

uint64_t Now() { return Static().curTimeMs; }

// GripFilter_IsMergeHandlerEdgeEnabled @ 0x6bae1763 / GripFilter_IsMergeHandlerEnabled @ 0x6bae18c4
bool IsMergeHandlerEdgeEnabled(uint32_t edge) { return (PrmtFlash().gripEdge[edge].flags & kEdgeMergeHandler) != 0; }
bool IsMergeHandlerEnabled() {
    return IsMergeHandlerEdgeEnabled(0) || IsMergeHandlerEdgeEnabled(1) || IsMergeHandlerEdgeEnabled(2) || IsMergeHandlerEdgeEnabled(3);
}

// GripFilter_IsGripMergeAllowed @ 0x6bae1eda：本帧对外报的点都带握持 / 合并标记
bool IsGripMergeAllowed() {
    const TouchTable& tt = Touches();
    return static_cast<int>(tt.reportedThisFrame) - static_cast<int>(tt.gripReportedThisFrame) < 1;
}

// GripFilter_GetPropertyGlobal @ 0x6bae990d：整帧的属性（触点所在边、形状、多指握持区域、握持合并）
void GetPropertyGlobal() {
    TouchTable& tt = Touches();
    for (uint8_t k = 0; k < tt.count; ++k) {
        TouchUnit& u = tt.unit[k];
        g_ctx.cur = k;
        g_ctx.prev = u.prevIdx;
        u.touchSide = Report::GetTouchSide(k);
        g_ctx.edge = GetEdge();
        GetShapeFlag();
        if (u.age < 4 && IsInMultiGripRegion(k)) u.gripFlags |= kGripInMultiRegion;
        if (!IsGripMergeAllowed()) {
            if ((u.persistFlags4 & kPersist4Merged) != 0 || (u.gripFlags & kGripPeakAbsNotGrip) != 0) {
                u.persistFlags4 |= kPersist4MergeCancelled;
                u.gripFlags &= ~kGripSafe;
            }
            u.persistFlags4 &= ~kPersist4Merged;
            u.gripFlags &= ~kGripPeakAbsNotGrip;
        } else if ((u.persistFlags4 & kPersist4Merged) != 0 && !Touch::IsOff(k) && IsPeakAbsNotGrip(u.peakIdx)) {
            u.gripFlags |= kGripPeakAbsNotGrip;
        }
        if (GetCenterLevel(k) != 0 || u.sigSum < u.gripRegionSigSum * 2) u.gripFlags |= kGripWeakCenter;
        if (!Touch::IsOff(g_ctx.cur) && u.dim1Span == 1 && 2 < u.dim2Span && Peak::IsOnInnerEdgeDim1(u.peakIdx)) {
            u.persistFlags4 |= kPersist4InnerEdgeNarrow;
        }
    }
}

// GripFilter_GetPrmt @ 0x6bae43a3
void GetPrmt() {
    const uint8_t level = GetCenterLevel(g_ctx.cur);
    g_ctx.prmtLevel = level < 4 ? level : 0;
}

// GripFilter_GetQuality @ 0x6bae42bc：基线复位中一律按绝对握持
void GetQuality() {
    if (Baseline::ResetStatus() == 1) {
        g_ctx.quality = kQualityAbsGrip;
    } else if (BasicIsAbsNotGrip()) {
        g_ctx.quality = kQualityAbsNotGrip;
    } else if (BasicIsAbsGrip()) {
        g_ctx.quality = kQualityAbsGrip;
    } else if (BasicIsTempGrip()) {
        g_ctx.quality = kQualityTempGrip;
    } else {
        CurG().gripFlags |= kGripNoQuality;
        g_ctx.quality = kQualityNone;
    }
}

// GripFilter_StageOnQuality @ 0x6bae48e1
int32_t StageOnQuality() {
    switch (g_ctx.quality) {
        case kQualityAbsNotGrip: return kStageSafe;
        case kQualityAbsGrip: return kStageAbsGrip;
        case kQualityTempGrip: return kStageTempGrip;
        default: return kStageNormal;
    }
}

// GripFilter_IsDelayTimeOut @ 0x6bae47ec
bool IsDelayTimeOut() {
    TouchUnit& u = CurG();
    if (u.event < TouchEvent::kTouchDown || u.durationMs < GetDelayTime()) return false;
    u.gripFlags |= kGripDelayTimeOut;
    return true;
}

// GripFilter_IsBackToGrip @ 0x6bae237b
bool IsBackToGrip() {
    if ((Static().featureFlags & kFeatGripPost) != 0 && IsStuckSideTouch()) return true;
    if (IsBackToGripAllowed() && IsBackToGripSatisfied()) return true;
    const TouchUnit& u = CurG();
    if (u.sqAccMmFromPrev < 0x1a || (0x3f < u.edgeDistDim1 && 0x3f < u.edgeDistDim2)) return IsSeparated(g_ctx.cur, g_ctx.prev);
    return true;
}

// GripFilter_BackToGrip @ 0x6bae24a1：按抬起处理，坐标退回上一帧，清掉历史轨迹
void BackToGrip() {
    TouchUnit& u = CurG();
    u.event = TouchEvent::kLiftOff;
    u.pos = PrevG().pos;
    PrevTouch::HistoryBufferClean(g_ctx.prev);
}

// GripFilter_GetStage @ 0x6bae492d：在上一帧的阶段上按本帧质量迁移
void GetStage() {
    int32_t stage = PrevG().gripStage;
    switch (stage) {
        case kStageTbd:
            if (BasicIsAbsNotGrip()) {
                stage = kStageSafe;
            } else {
                stage = StageOnQuality();
                // 还没到延时、仍按着的握持先保持待定
                if (!IsDelayTimeOut() && !Touch::IsOff(g_ctx.cur) && (stage == kStageTempGrip || stage == kStageNormal)) stage = kStageTbd;
            }
            break;
        case kStageAbsGrip:
            if (BasicIsAbsNotGrip()) stage = kStageSafe;
            break;
        case kStageTempGrip:
            stage = BasicIsAbsNotGrip() ? kStageSafe : StageOnQuality();
            break;
        case kStageNormal:
            if (IsBackToGrip()) {
                BackToGrip();
            } else if (BasicIsAbsNotGrip()) {
                stage = kStageSafe;
            } else {
                stage = StageOnQuality();
                if (stage == kStageTempGrip) stage = kStageNormal;
            }
            break;
        case kStageSafe:
            if (IsBackToGrip()) BackToGrip();
            break;
        default: break;
    }
    CurG().gripStage = stage;
}

// GripFilter_BasicProcess @ 0x6bae4ab3
void BasicProcess(uint8_t curIdx, uint8_t prevIdx) {
    if (Scenario::IsJustEnterTyping()) g_ctx.absGripEnterCount = 0;
    if (HandGesture::Is2HandDetected()) g_ctx.absGripEnterCount = 0;
    g_ctx.cur = curIdx;
    g_ctx.prev = prevIdx;
    g_ctx.edge = GetEdge();
    GetPrmt();
    GetProperty();
    GetQuality();
    GetStage();
    UpdateDeadzone();
    TouchTable& tt = Touches();
    TouchUnit& u = tt.unit[curIdx];
    if (u.gripStage == kStageAbsGrip && PrevG().gripStage != kStageAbsGrip && g_ctx.absGripEnterCount != 0xff) ++g_ctx.absGripEnterCount;
    if (u.gripStage == kStageAbsGrip || u.gripStage == kStageTempGrip) ++tt.gripCount;
    if ((u.persistFlags & kPersistReportedOnce) != 0 && IsGrip(Prev(prevIdx).gripStage)) u.persistFlags2 |= kPersist2GripToPost;
}

// GripFilter_Report @ 0x6bae1a5b：握持触点暂不报点；由握持转为报点的补报一次（AFT 模式下由 AntiTouch 处理）
void ReportGrip() {
    TouchTable& tt = Touches();
    for (uint8_t k = 0; k < tt.count; ++k) {
        TouchUnit& u = tt.unit[k];
        if (IsGripTouch(k)) {
            u.volatileFlags |= kVolatileGripHeld;
            continue;
        }
        u.volatileFlags &= ~kVolatileGripHeld;
        if ((u.persistFlags2 & kPersist2GripToPost) != 0 && !Sys::IsAftModeEnabled()) Touch::CreatePostReport(k);
    }
}

// GripFilter_IDLEProcess @ 0x6bae1c75：全部触点绝对握持超过 500 ms 后允许进 IDLE
void IdleProcess() {
    const TouchTable& tt = Touches();
    bool allAbsGrip = PrmtFlash().gripIdleEnabled != 0 && tt.count != 0;
    for (uint8_t k = 0; allAbsGrip && k < tt.count; ++k) {
        if (!IsAbsGrip(k)) allAbsGrip = false;
    }
    if (!allAbsGrip) g_inactiveTime = Now();
    g_okToIdle = kIdleAfterGripMs < Now() - g_inactiveTime;
}

}  // namespace

Context& Ctx() { return g_ctx; }
const LevelPrmt& CurPrmt() { return kLevelPrmt[g_ctx.prmtLevel]; }

// GripFilter_GetEdge @ 0x6bae9854：左 2、右 3、上 0，其余 1
uint32_t GetEdge() {
    const uint32_t side = CurG().touchSide;
    if ((side & kSideLeft) != 0) return 2;
    if ((side & kSideRight) != 0) return 3;
    return (side & kSideTop) != 0 ? 0 : 1;
}

// GripFilter_GetGripLevel @ 0x6baea5a5：区域位里最高的一位（0..3）
uint8_t GetGripLevel(uint8_t regionBits) {
    for (int level = 3; level >= 0; --level) {
        if ((1 << level) <= regionBits) return static_cast<uint8_t>(level);
    }
    return 0;
}

// GripFilter_GetCenterLevel @ 0x6baea5fb：触点所在格的握持区域等级
uint8_t GetCenterLevel(uint8_t curIdx) {
    const Point pos = Cur(curIdx).pos;
    const auto cell = static_cast<uint16_t>((pos.dim1 >> 8) + (pos.dim2 >> 8) * PrmtFlash().dim1Len);
    return GetGripLevel(Buffers().gripRegion[cell]);
}

// GripFilter_Is2HandGame @ 0x6bae1e40
bool Is2HandGame() {
    if (IsPad()) return false;
    if (Scenario::IsTyping() && !Sys::IsScreenRotated()) return false;
    return Scenario::IsGame() || HandGesture::Is2HandDetected();
}

// GripFilter_IsVideoMode @ 0x6bae1e9c
bool IsVideoMode() { return Scenario::IsVideo() && Sys::IsScreenRotated() && !Is2HandGame() && !Scenario::IsTyping(); }

// GripFilter_IsAbsGrip @ 0x6bae1bd9
bool IsAbsGrip(uint8_t curIdx) {
    if (Cur(curIdx).gripStage == kStageAbsGrip) return true;
    return IsGripOrTbdTouch(curIdx) && (200 < Cur(curIdx).age || Touch::IsGhostByMinSigOff2(curIdx));
}

// GripFilter_IsMerged @ 0x6bae83a6：靠边的握持触点被并入了内侧的新触点
// （原代码在这里用历史下标读当前帧的 persistFlags4）
bool IsMerged(uint8_t curIdx, uint8_t prevIdx) {
    const PrevTouchUnit& p = Prev(prevIdx);
    if (Static().htiThold / 2 < PrevPeaks().entry[p.peakIdx].sig) return false;
    if ((Cur(prevIdx).persistFlags4 & kPersist4Merged) != 0 || 0x80 < p.edgeDistDim1) return false;
    if (Cur(curIdx).edgeDistDim1 < static_cast<uint16_t>(TouchSize::MmToPixel(4)) + p.edgeDistDim1) return false;
    if ((p.touchSide & kSideLeft) == 0 && (p.touchSide & kSideRight) == 0) return false;
    if (p.edgeRatioDim1 < PrmtFlash().gripEdge[Report::SideToEdge(p.touchSide)].ratioThold[2]) return false;
    return !Peak::IsOnOuterEdgeDim1(Cur(curIdx).peakIdx) || (Cur(curIdx).persistFlags & kPersistReported) == 0;
}

// GripFilter_Init @ 0x6bae174a
void Init() {
    RegionReset();
    DeadzoneReset();
}

// GripFilter_Process @ 0x6bae1d3c
void Process() {
    GetPropertyGlobal();
    TouchTable& tt = Touches();
    for (uint8_t k = 0; k < tt.count; ++k) {
        if ((tt.unit[k].persistFlags & kPersistMergeHandled) == 0) BasicProcess(k, PrevTouch::GetIdxBasedOnCurIdx(k));
    }
    if ((Static().featureFlags & kFeatGripPost) == 0) return;
    if (IsMergeHandlerEnabled()) Buffer::GripStageProcess();
    ReportGrip();
    IdleProcess();
}

// GripFilter_IsGrip @ 0x6bae193f
bool IsGrip(int32_t stage) { return stage == kStageTbd || stage == kStageAbsGrip || stage == kStageTempGrip; }

// GripFilter_IsGripOrTbd @ 0x6bae1966
bool IsGripOrTbd(int32_t stage) { return stage == kStageTbd || IsGrip(stage); }

// GripFilter_IsGripTouch @ 0x6bae19d1 / GripFilter_IsGripOrTbdTouch @ 0x6bae1995
bool IsGripTouch(uint8_t curIdx) { return IsGrip(Cur(curIdx).gripStage); }
bool IsGripOrTbdTouch(uint8_t curIdx) { return IsGripOrTbd(Cur(curIdx).gripStage); }

// GripFilter_IsPeakAbsGrip @ 0x6baea0be / GripFilter_IsPeakAbsNotGrip @ 0x6baea103
bool IsPeakAbsGrip(uint8_t peakIdx) { return (PeaksGrid().entry[peakIdx].flags & kPeakAbsGrip) != 0; }
bool IsPeakAbsNotGrip(uint8_t peakIdx) { return (PeaksGrid().entry[peakIdx].flags & kPeakAbsNotGrip) != 0; }

// GripFilter_IsToCancelIDMapping @ 0x6bae8858
bool IsToCancelIdMapping(uint8_t curIdx, uint8_t prevIdx) {
    if (IsPeakAbsNotGrip(Cur(curIdx).peakIdx) && IsGrip(Prev(prevIdx).gripStage) && (Cur(curIdx).persistFlags4 & kPersist4Merged) != 0) {
        return true;
    }
    return IsMerged(curIdx, prevIdx);
}

// GripFilter_IsMergeHandlerSensorEnabled @ 0x6bae1820：格在开启握持合并处理的边的两行 / 两列内
bool IsMergeHandlerSensorEnabled(uint8_t dim1, uint8_t dim2) {
    const FlashPrmt& fl = PrmtFlash();
    return (dim2 < 2 && IsMergeHandlerEdgeEnabled(0)) || (fl.dim2Len - 2 <= dim2 && IsMergeHandlerEdgeEnabled(1)) ||
           (dim1 < 2 && IsMergeHandlerEdgeEnabled(2)) || (fl.dim1Len - 2 <= dim1 && IsMergeHandlerEdgeEnabled(3));
}

// GripFilter_IsOKToEnterIDLE @ 0x6bae1bcc
bool IsOkToEnterIdle() { return g_okToIdle; }

// GripFilter_GetTrackingThold @ 0x6bae5d38：finger 按下门限的一半
int16_t GetTrackingThold() { return static_cast<int16_t>(PrmtFlash().fingerThold / 2); }

}  // namespace Solvers::Tsa::Core::GripFilter
