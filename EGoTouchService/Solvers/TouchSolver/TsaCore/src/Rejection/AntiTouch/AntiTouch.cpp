#include "Rejection/AntiTouch/AntiTouch.h"

#include "Alg/Baseline/Baseline.h"
#include "Alg/Peak/PeakData.h"
#include "Alg/Sensor/Sensor.h"
#include "Alg/Touch/PrevTouch.h"
#include "Alg/Touch/Touch.h"
#include "Alg/Touch/TouchData.h"
#include "App/Report.h"
#include "App/Scenario.h"
#include "App/Sys.h"
#include "App/TsaOut.h"
#include "App/TsaStatic.h"
#include "Prmt/Prmt.h"
#include "Rejection/TouchReport/TouchReport.h"

// 接近传感器与屏下指纹（UDFP）模块在本版本中被裁掉：接近恒未检出，UDFP 恒未开启，相关判据都不成立。

namespace Solvers::Tsa::Core::AntiTouch {

namespace {

constexpr uint32_t kStateAbnormal = 0x20;         // TouchTable::stateFlags：有大面积 / 异常触点（500 ms 内保持）
constexpr uint32_t kStateProximity = 0x80;        // 屏幕上部有像脸 / 耳朵的触点
constexpr uint32_t kStateProtectProximity = 0x100;
constexpr uint32_t kExtProximity = 0x1000000;     // TouchTable::extFlags
constexpr uint32_t kExtProximityGone = 0x2000000;
constexpr uint32_t kVolatileSuspect = 0x4000000;  // TouchUnit::volatileFlags：可疑触点
constexpr uint32_t kVolatileHeld = 0x8000000;     // AFT：暂不报点
constexpr uint32_t kPersistUnstable = 0x20000;    // TouchUnit::persistFlags
constexpr uint32_t kPersistReportedOnce = 0x100000;
constexpr uint32_t kPersistProtectProximity = 0x400000;
constexpr uint32_t kPersistWithdrawable = 0x20000000;
constexpr uint32_t kGripMergeHandled = 0x200000;  // TouchUnit::gripFlags
constexpr int32_t kZoneMultiFinger = 3;           // TouchUnit::zoneShape
constexpr uint32_t kSysUdfp = 0x800000;           // TsaStatic::systemFlags

constexpr uint8_t kFramesLatched = 10;     // 可疑 / 正常帧数到这里就不再变化
constexpr uint64_t kAbnormalHoldMs = 500;
constexpr uint64_t kStayLongMs = 1500;     // 按下后停留超过它且几乎没动算可疑
constexpr uint16_t kStayMaxSqDistMm = 100;
constexpr uint64_t kSettleMs = 0x33;       // AFT：按下超过它才补报
constexpr uint64_t kGripStayMs = 200;
constexpr uint16_t kEdgeDistGrip = 0x100;  // 离边缘不到一格算握持

uint64_t g_abnormalTime = UINT64_MAX;  // 最近一次出现大面积触点的时间（ms）
uint8_t g_gripCount;                    // 本帧算握持的触点数
bool g_proximityReported;               // 上一次报的是 kExtProximity

TouchUnit& Cur(uint8_t idx) { return Touches().unit[idx]; }
// 原代码不检查历史下标：为 kNoTouch 时照样按下标读写，落在历史表之后的内存上。
PrevTouchUnit& Prev(uint8_t idx) { return PrevTouches().unit[idx]; }

bool Dim1IsDisplayX() { return PrmtFlash().dim1Axis == PrmtFlash().displayXAxis; }
uint64_t SinceTouchDown(uint8_t curIdx, uint8_t prevIdx) { return Cur(curIdx).timestamp - Prev(prevIdx).tdDebounceTime; }
bool InProximityArea(uint8_t curIdx) { return static_cast<uint8_t>(Report::TouchYCenter(curIdx)) <= PrmtFlash().proxAreaY; }
bool WideGrip(const TouchUnit& u) { return 8 < u.gripRatioDim1 || 8 < u.gripRatioDim2; }

// AntiTouch_CheckProximityFlag @ 0x6bad6aee
void CheckProximity(uint8_t curIdx) {
    if (InProximityArea(curIdx)) Touches().stateFlags |= kStateProximity;
}

// AntiTouch_IsGrip @ 0x6bad6640：握持，或贴着显示的左右边
bool IsGrip(uint8_t curIdx) {
    const TouchUnit& u = Cur(curIdx);
    const bool grip = Touch::IsGrip(curIdx);
    return (Dim1IsDisplayX() ? u.edgeDistDim1 : u.edgeDistDim2) < kEdgeDistGrip || grip;
}

// AntiTouch_GetAxisMajorThold @ 0x6bad697c
uint8_t AxisMajorThold(const TouchUnit& u) {
    if (u.gripRatioDim1 < 9 && u.gripRatioDim2 < 9 && u.smoothEllipseRate < 15) return 30;
    return u.age < 5 ? 30 : 25;
}

// AntiTouch_GetEllipseRateThold @ 0x6bad6a5a：握持比大且在屏幕上半部时放宽
int16_t EllipseRateThold(uint8_t curIdx) {
    if (WideGrip(Cur(curIdx)) && static_cast<uint8_t>(Report::TouchYCenter(curIdx)) <= Report::YLength() >> 1) return 10;
    return 15;
}

// AntiTouch_IsToProtectProximity @ 0x6bad66e6：接近区内沿显示 Y 方向握持比大的不保护
bool IsToProtectProximity(uint8_t curIdx) {
    const TouchUnit& u = Cur(curIdx);
    bool protect = !IsGrip(curIdx) && TouchEvent::kTouchDownDebounce < u.event;
    if (InProximityArea(curIdx) && 0x10 < (Dim1IsDisplayX() ? u.gripRatioDim2 : u.gripRatioDim1)) protect = false;
    return protect;
}

// AntiTouch_IsToGoBackReport @ 0x6bad6021
bool IsToGoBackReport(uint8_t prevIdx) {
    const PrevTouchUnit& p = Prev(prevIdx);
    return kFramesLatched <= p.antiTouchSuspectFrames && p.antiTouchClearFrames < kFramesLatched;
}

// AntiTouch_ProcessProximityUnit @ 0x6bad6f59
void ProcessProximityUnit(uint8_t curIdx, uint8_t prevIdx) {
    TouchTable& tt = Touches();
    TouchUnit& u = tt.unit[curIdx];
    const FlashPrmt& fl = PrmtFlash();
    if (40 < u.majorMm && static_cast<int16_t>(fl.peakFilterReferenceThreshold) / 2 < Peaks().entry[u.peakIdx].sig &&
        !IsGrip(curIdx)) {
        CheckProximity(curIdx);
    }
    if (kStayLongMs < SinceTouchDown(curIdx, prevIdx) && u.sqDistMmFromStart < kStayMaxSqDistMm) {
        u.volatileFlags |= kVolatileSuspect;
        if (9 < u.smoothEllipseRate && !IsGrip(curIdx)) CheckProximity(curIdx);
    }
    if ((u.gripFlags & kGripMergeHandled) == 0 && u.majorMm < 9 && 9 < u.smoothEllipseRate) {
        u.volatileFlags |= kVolatileSuspect;
        if (InProximityArea(curIdx) && !IsGrip(curIdx)) tt.stateFlags |= kStateProximity;
    }
    if (IsToProtectProximity(curIdx)) u.persistFlags |= kPersistProtectProximity;
    if ((u.persistFlags & kPersistProtectProximity) != 0) tt.stateFlags |= kStateProtectProximity;
    if (20 < u.majorMm && 15 < Report::TouchYGripRatio(curIdx) && 15 < u.smoothEllipseRate &&
        kGripStayMs < SinceTouchDown(curIdx, prevIdx)) {
        CheckProximity(curIdx);
    }
    if (u.majorMm <= 20 || u.dim2Span <= 4 || u.smoothEllipseRate <= 9 || !InProximityArea(curIdx)) return;
    // 开启接近直报时只算不贴左右边的
    if (fl.antiTouchProxMode != 0) {
        const auto x = static_cast<uint8_t>(Report::TouchXCenter(curIdx));
        if (x <= 1 || fl.dim1Len - 3 < x) return;
    }
    u.volatileFlags |= kVolatileSuspect;
    tt.stateFlags |= kStateProximity;
}

// AntiTouch_GetPersistFlag @ 0x6bad6b3d：本帧是否可疑（不像正常手指）
bool IsSuspect(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& u = Cur(curIdx);
    bool suspect = false;
    if (kStayLongMs < SinceTouchDown(curIdx, prevIdx) && u.sqDistMmFromStart < kStayMaxSqDistMm) {
        suspect = (Static().systemFlags & kSysUdfp) == 0;
        if (suspect) u.volatileFlags |= kVolatileSuspect;
        if (9 < u.smoothEllipseRate && !IsGrip(curIdx)) CheckProximity(curIdx);
    }
    const int nonGrip = Touches().count - g_gripCount;
    if (1 < nonGrip && EllipseRateThold(curIdx) <= u.smoothEllipseRate && u.zoneShape != kZoneMultiFinger) {
        u.volatileFlags |= kVolatileSuspect;
        suspect = true;
        CheckProximity(curIdx);
    }
    if (3 < nonGrip) {
        u.volatileFlags |= kVolatileSuspect;
        suspect = true;
        CheckProximity(curIdx);
    }
    const uint32_t major = u.majorMm;
    if (AxisMajorThold(u) < major) {
        u.volatileFlags |= kVolatileSuspect;
        suspect = true;
    }
    // 长短轴比超过 2.5 可疑，超过 3 计入
    if (u.minorMm * 5u < major * 2 && 15 < major) {
        u.volatileFlags |= kVolatileSuspect;
        if (u.minorMm * 6u < major * 2) suspect = true;
    }
    return suspect;
}

// AntiTouch_ProcessUnit @ 0x6bad7719
// 可疑帧数逐帧累加，不可疑时回落；不可疑且像正常手指（椭圆度小、在屏幕上部三分之一以下）的帧数也累加。
void ProcessUnit(uint8_t curIdx, uint8_t prevIdx) {
    TouchTable& tt = Touches();
    TouchUnit& u = tt.unit[curIdx];
    const FlashPrmt& fl = PrmtFlash();
    const uint16_t yRes = Dim1IsDisplayX() ? fl.dim2Res : fl.dim1Res;
    if (50 < u.majorMm && !IsGrip(curIdx)) {
        tt.stateFlags |= kStateAbnormal;
        CheckProximity(curIdx);
        g_abnormalTime = Sys::CurrTimeStamp();
    }
    ProcessProximityUnit(curIdx, prevIdx);
    if ((u.gripFlags & kGripMergeHandled) == 0 && WideGrip(u) && 15 < u.smoothEllipseRate) u.volatileFlags |= kVolatileSuspect;
    PrevTouchUnit& p = Prev(prevIdx);
    if (IsSuspect(curIdx, prevIdx)) {
        if (p.antiTouchSuspectFrames < kFramesLatched) ++p.antiTouchSuspectFrames;
        return;
    }
    if (p.antiTouchSuspectFrames != 0 && p.antiTouchSuspectFrames < kFramesLatched) --p.antiTouchSuspectFrames;
    if (kFramesLatched <= p.antiTouchClearFrames) return;
    if (u.smoothEllipseRate < 10 && yRes < Out::DisplayYPos(curIdx) * 3u) {
        ++p.antiTouchClearFrames;
    } else {
        p.antiTouchClearFrames = 0;
    }
}

// AntiTouch_PostReportUnit @ 0x6bad6099
// 长期可疑的触点回退坐标；可撤下 / 不稳定的不处理，其余先暂不报点，按下够久（或刚抬起且在传感器区域外）
// 且不再可疑、不是握持时放开，报过点的补一次报点。
void PostReportUnit(uint8_t curIdx, uint8_t prevIdx) {
    if (IsToGoBackReport(prevIdx)) TouchReport::GoBack(curIdx, prevIdx);
    TouchUnit& u = Cur(curIdx);
    if ((u.persistFlags & (kPersistUnstable | kPersistWithdrawable)) != 0) return;
    u.volatileFlags |= kVolatileHeld;
    const PrevTouchUnit& p = Prev(prevIdx);
    const bool lifting = u.event == TouchEvent::kLiftOff || u.event == TouchEvent::kLiftOffHold;
    const bool outsideSensor = lifting && Sensor::ZoneIndex(static_cast<uint8_t>(p.pos.dim1 >> 8), static_cast<uint8_t>(p.pos.dim2 >> 8)) == 0xff;
    if (!outsideSensor && SinceTouchDown(curIdx, prevIdx) < kSettleMs) return;
    if ((Touches().stateFlags & kStateAbnormal) != 0 || p.antiTouchSuspectFrames != 0 || (u.volatileFlags & kVolatileSuspect) != 0 ||
        Touch::IsGrip(curIdx)) {
        return;
    }
    u.volatileFlags &= ~kVolatileHeld;
    if ((u.persistFlags & kPersistReportedOnce) != 0) Touch::CreatePostReport(curIdx);
}

}  // namespace

// AntiTouch_Init @ 0x6bad6010
void Init() { g_abnormalTime = UINT64_MAX; }

// AntiTouch_Process @ 0x6bad7bbb（含 AntiTouch_CommonProcess / PreProcess / PostReport）
void Process() {
    TouchTable& tt = Touches();
    g_gripCount = 0;
    for (uint8_t k = 0; k < tt.count; ++k) {
        if (IsGrip(k)) ++g_gripCount;
    }
    if (g_abnormalTime != UINT64_MAX && Sys::CurrTimeStamp() - g_abnormalTime < kAbnormalHoldMs) tt.stateFlags |= kStateAbnormal;
    for (uint8_t k = 0; k < tt.count; ++k) ProcessUnit(k, PrevTouch::GetIdxBasedOnCurIdx(k));
    if (!Sys::IsAftModeEnabled()) return;
    if (Baseline::IsReset()) {
        Touch::Clean();
        PrevTouch::Release();
        Init();
        return;
    }
    for (uint8_t k = 0; k < tt.count; ++k) PostReportUnit(k, PrevTouch::GetIdxBasedOnCurIdx(k));
}

// AntiTouch_Report @ 0x6bad7bf7
// 开启接近直报且不在锁屏场景时按本帧接近状态报；否则有触点抬起时报 kExtProximity，之后第一帧没有抬起时报解除。
void Report() {
    TouchTable& tt = Touches();
    if (PrmtFlash().antiTouchProxMode != 0 && !Scenario::IsScreenLock()) {
        tt.extFlags |= (tt.stateFlags & kStateProximity) != 0 ? kExtProximity : kExtProximityGone;
        return;
    }
    if (g_proximityReported && tt.liftOffCount == 0) {
        tt.extFlags |= kExtProximityGone;
        g_proximityReported = false;
    }
    if (tt.liftOffCount != 0) {
        tt.extFlags |= kExtProximity;
        g_proximityReported = true;
    }
}

}  // namespace Solvers::Tsa::Core::AntiTouch
