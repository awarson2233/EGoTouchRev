#include "Extension/HandGesture/HandGesture.h"

#include <algorithm>
#include <array>
#include <cstdlib>

#include "Alg/CTD/Ctd.h"
#include "Alg/IDTracking/IdTracking.h"
#include "Alg/Peak/Peak.h"
#include "Alg/Peak/PrevPeak.h"
#include "Alg/Touch/PrevTouch.h"
#include "Alg/Touch/Touch.h"
#include "Alg/Touch/TouchData.h"
#include "Alg/Touch/TouchGeometry.h"
#include "Alg/TouchEvent/TouchEvent.h"
#include "Alg/TouchZone/TouchZone.h"
#include "App/DataSwitch.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Common/Assert.h"
#include "Common/Memory.h"
#include "Common/TsaMath.h"
#include "Prmt/Prmt.h"
#include "Prmt/TsaPrmt.h"
#include "Rejection/GripFilter/GripFilter.h"

namespace Solvers::Tsa::Core::HandGesture {

namespace {

constexpr uint32_t kArmed = 0x1;    // 握持基线已复位，等待处理
constexpr uint32_t kTwoHand = 0x2;  // 双手握持

constexpr uint64_t kDifWindowMs = 999;       // 基线复位后多久内算手势 Dif
constexpr uint64_t kTwoHandLiftMs = 100;     // 标记触点抬起后多久内有远处触点算双手
constexpr uint16_t kTwoHandSqDistMm = 399;
constexpr uint64_t kTwoHandHoldMs = 4000;    // 双手握持至少保持这么久
constexpr uint16_t kMinSqDistMm = 400;       // 关联握持的距离范围（mm²）
constexpr uint16_t kMaxSqDistMm = 0x1900;
constexpr int kAngleTolerance = 0x1f;
constexpr int16_t kTimeNone = 0x7fff;
constexpr int16_t kReportedLongMs = 5000;    // 报点超过这么久的格不算手势区域
constexpr uint16_t kDifCells = 0x1324;       // Dif 备份的容量
constexpr uint32_t kHandGripFlags = 0x17f;   // 手势数据集上 TZ 用的握持 / 遍历模式

constexpr uint32_t kPersistWithdrawable = 0x20000000;  // TouchUnit::persistFlags
constexpr uint32_t kGripHandRelated = 0x8000000;       // TouchUnit::gripFlags

/// 原 handGesture（0x6bc86ca0）中仍被读取的部分。
struct State {
    uint32_t flags;
    int16_t maxDif;      // 本帧手势 Dif 的最大值
    int16_t prevMaxDif;  // 上一帧的
    Point gripPos;       // 抬起的关联握持触点的位置与倾角
    int8_t gripAngle;
    uint64_t resetTime;    // 握持基线复位的时间
    uint64_t twoHandTime;  // 最近一次判为双手的时间
};
State g_state;

TouchUnit& Cur(uint8_t idx) { return Touches().unit[idx]; }
// 原代码不检查历史下标：为 kNoTouch 时照样按下标读写，落在历史表之后的内存上。
PrevTouchUnit& Prev(uint8_t idx) { return PrevTouches().unit[idx]; }
uint64_t Now() { return Static().curTimeMs; }

// HandGesture_GetWidth @ 0x6baeedbc：两侧各算 width + 1 列
uint8_t EdgeWidth() { return (Static().featureFlags & kFeatSideTouch) != 0 ? 3 : 2; }

// HandGesture_GetPkThold @ 0x6baef045
int16_t PeakThold() { return static_cast<int16_t>(PrmtDynamic().sigThold * 3 / 4); }

/// 对两侧边缘各 width + 1 列、落在手势区域里的格执行 fn(cell)。
template <class Fn>
void ForEachEdgeCell(Fn fn) {
    const FlashPrmt& fl = PrmtFlash();
    const uint8_t width = EdgeWidth();
    const auto visit = [&](uint8_t col) {
        for (uint8_t row = 0; row < fl.dim2Len; ++row) {
            const auto cell = static_cast<uint16_t>(col + row * fl.dim1Len);
            if (0 < HandBufs().area[cell]) fn(cell);
        }
    };
    for (uint8_t col = 0; col <= width; ++col) visit(col);
    for (auto col = static_cast<uint8_t>(fl.dim1Len - width - 1); col <= fl.dim1Len - 1; ++col) visit(col);
}

// HandGesture_AngleFormat1 @ 0x6baeeabd：折到 [-90, 90]
int16_t FoldAngle(int16_t a) {
    if (90 < a) return static_cast<int16_t>(a - 180);
    if (a < -90) return static_cast<int16_t>(a + 180);
    return a;
}

// HandGesture_IsAngleCorelated @ 0x6baeeb33：连线方向与椭圆倾角（转 90°）相差不到 31°
bool IsAngleCorrelated(int16_t lineAngle, int16_t ellipseAngle) {
    const int16_t a = FoldAngle(lineAngle);
    const int16_t b = FoldAngle(static_cast<int16_t>(ellipseAngle < 1 ? ellipseAngle + 90 : ellipseAngle - 90));
    return std::abs(a - b) < kAngleTolerance;
}

int16_t LineAngle(Point from, Point to) {
    return static_cast<int16_t>(-static_cast<int16_t>(Math::ATan2(static_cast<int16_t>(to.dim1 - from.dim1),
                                                                  static_cast<int16_t>(to.dim2 - from.dim2))));
}

// HandGesture_IsDistInValid @ 0x6baee0e5
bool IsDistInvalid(Point a, Point b) {
    const uint16_t d = Touch::GetSqDistMm(a.dim1, a.dim2, b.dim1, b.dim2);
    return d < kMinSqDistMm || kMaxSqDistMm < d;
}

// HandGesture_ResetRecord @ 0x6baede69
void ResetRecord() {
    HandBuffers& hb = HandBufs();
    std::fill_n(hb.tzPrev, GridBufSize(), int8_t{0});
    std::fill_n(hb.time, PrmtFlash().sensorNum, kTimeNone);
}

// HandGesture_BaselineReset @ 0x6baeddbd（随后按新基线算的整屏 Dif 在用到之前总会被 DifProcess 重算，略去）
void BaselineReset() { std::copy_n(Buffers().raw, PrmtFlash().sensorNum, HandBufs().baseline); }

// HandGesture_TimeStep @ 0x6baeea17：各格时间按帧间隔累加（饱和）
void TimeStep() {
    const int dt = Static().frameIntervalMs;
    int16_t* time = HandBufs().time;
    for (uint16_t i = 0; i < PrmtFlash().sensorNum; ++i) {
        if (time[i] != kTimeNone) time[i] = static_cast<int16_t>(std::clamp(dt + time[i], -0x8000, 0x7fff));
    }
}

// HandGesture_2HandDetection @ 0x6baef641
// 上一帧报了两个以上的点，或可撤下的触点离刚抬起的标记触点够远时判为双手；有握持触点且已过 4 s 时解除。
void TwoHandDetection() {
    const TsaStatic& stat = Static();
    bool twoHand = 1 < PrevTouches().field006;
    bool gripping = false;
    for (uint8_t k = 0; k < Touches().count; ++k) {
        const TouchUnit& u = Cur(k);
        if ((u.persistFlags & kPersistWithdrawable) == 0) {
            if (u.gripStage == 2 || u.gripStage == 1) gripping = true;
        } else {
            const uint16_t d = Touch::GetSqDistMm(u.pos.dim1, u.pos.dim2, stat.markedLiftOffPos.dim1, stat.markedLiftOffPos.dim2);
            if (Now() - stat.markedLiftOffTime < kTwoHandLiftMs && kTwoHandSqDistMm < d) twoHand = true;
        }
    }
    if (twoHand) {
        g_state.flags |= kTwoHand;
        g_state.twoHandTime = Now();
        ResetRecord();
    } else if (gripping && kTwoHandHoldMs < Now() - g_state.twoHandTime) {
        g_state.flags &= ~kTwoHand;
    }
}

// HandGesture_SetHandArea @ 0x6baee147：手势区域取历史触点 prevIdx 对应区域的格
void SetHandArea(uint8_t prevIdx) {
    int8_t* area = HandBufs().area;
    std::fill_n(area, GridBufSize(), int8_t{0});
    const uint8_t zone = Cur(Prev(prevIdx).curIdx).zoneIdx;
    const uint8_t* tzZone = Buffers().tzZone;
    for (uint16_t i = 0; i < PrmtFlash().sensorNum; ++i) {
        if (tzZone[i] == zone) area[i] = 1;
    }
}

// HandGesture_IsGripRelated @ 0x6baee25c
// 历史触点 prevIdx 在 curIdx 椭圆长轴方向上、距离适中时标记它，它在握持中时算关联。
bool IsGripRelated(uint8_t curIdx, uint8_t prevIdx) {
    const TouchUnit& u = Cur(curIdx);
    const PrevTouchUnit& p = Prev(prevIdx);
    const bool correlated = IsAngleCorrelated(LineAngle(u.pos, p.pos), u.angle);
    const bool invalid = IsDistInvalid(p.pos, u.pos);
    if (!correlated || invalid) return false;
    Cur(p.curIdx).gripFlags |= kGripHandRelated;
    return GripFilter::IsGrip(p.gripStage);
}

// HandGesture_IsRelatedGripDetected @ 0x6baee4eb
// 可撤下、较圆且够大的触点，找一个与它关联的握持触点，把那个触点的区域记为手势区域。
bool IsRelatedGripDetected(uint8_t curIdx, uint8_t prevIdx) {
    const TouchUnit& u = Cur(curIdx);
    if ((Prev(prevIdx).persistFlags & kPersistWithdrawable) == 0 || u.majorMm * 4u < u.minorMm * 5u || u.minorMm <= 7) return false;
    for (uint8_t j = 0; j < PrevTouches().count; ++j) {
        if (u.id != Prev(j).id && IsGripRelated(curIdx, j)) {
            SetHandArea(j);
            return true;
        }
    }
    return false;
}

// HandGesture_CheckRelatedGrip @ 0x6baee667：信号和超过轨迹最大值 3/4 时重判关联握持，记下位置与倾角
void CheckRelatedGrip(uint8_t curIdx, uint8_t prevIdx) {
    const TouchUnit& u = Cur(curIdx);
    PrevTouchUnit& p = Prev(prevIdx);
    if (u.sigSum * 4 <= p.maxSigSum * 3) return;
    if (!IsRelatedGripDetected(curIdx, prevIdx)) {
        p.relatedGrip = 0;
        return;
    }
    p.relatedGrip = 1;
    p.relatedGripPos = u.pos;
    p.relatedGripAngle = u.angle;
}

// HandGesture_IsOKToResetBaseline @ 0x6baee815：有关联握持的触点抬起
bool IsOkToResetBaseline() {
    for (uint8_t k = 0; k < Touches().count; ++k) {
        const uint8_t prevIdx = Cur(k).prevIdx;
        CheckRelatedGrip(k, prevIdx);
        const PrevTouchUnit& p = Prev(prevIdx);
        if (Cur(k).event == TouchEvent::kLiftOff && p.relatedGrip != 0) {
            g_state.gripPos = p.relatedGripPos;
            g_state.gripAngle = p.relatedGripAngle;
            return true;
        }
    }
    return false;
}

// HandGesture_DifUnit @ 0x6baeede7 / HandGesture_DifProcess @ 0x6baeef00：按手势基线算两侧边缘的 Dif
void DifProcess() {
    HandBuffers& hb = HandBufs();
    std::fill_n(hb.dif, GridBufSize(), int16_t{0});
    g_state.maxDif = INT16_MIN;
    const int16_t* raw = Buffers().raw;
    const bool reverted = Static().signalReverted != 0;
    ForEachEdgeCell([&](uint16_t cell) {
        const auto d = static_cast<int16_t>(reverted ? hb.baseline[cell] - raw[cell] : raw[cell] - hb.baseline[cell]);
        hb.dif[cell] = d;
        g_state.maxDif = std::max(g_state.maxDif, d);
    });
}

/// 手势处理期间临时改动、处理完恢复的门限与 TZ 参数。
struct Thresholds {
    int16_t noiseThold, peakThold, touchThold, field2D6, fingerTouchThold, touchDownThold;
    uint8_t field2DC, tholdSwitching;
    int16_t field2DE;
    uint8_t field2E0;
    int16_t gripModeSigThold;
    uint8_t sigMult, sigMultNormal;
    uint32_t gripFlags;
};

Thresholds SaveThresholds() {
    const TsaPrpt& pr = Prpt();
    const FlashPrmt& fl = PrmtFlash();
    return {pr.noiseThold,       pr.peakThold, pr.touchThold,     pr.field2D6, pr.fingerTouchThold, pr.touchDownThold,
            pr.field2DC,         pr.tholdSwitching, pr.field2DE,   pr.field2E0, fl.gripModeSigThold, fl.sigMult,
            fl.sigMultNormal,    fl.gripFlags};
}

void RestoreThresholds(const Thresholds& t) {
    TsaPrpt& pr = Prpt();
    FlashPrmt& fl = PrmtFlash();
    pr.noiseThold = t.noiseThold;
    pr.peakThold = t.peakThold;
    pr.touchThold = t.touchThold;
    pr.field2D6 = t.field2D6;
    pr.fingerTouchThold = t.fingerTouchThold;
    pr.touchDownThold = t.touchDownThold;
    pr.field2DC = t.field2DC;
    pr.tholdSwitching = t.tholdSwitching;
    pr.field2DE = t.field2DE;
    pr.field2E0 = t.field2E0;
    fl.gripModeSigThold = t.gripModeSigThold;
    fl.sigMult = t.sigMult;
    fl.sigMultNormal = t.sigMultNormal;
    fl.gripFlags = t.gripFlags;
}

// HandGesture_TouchProcess @ 0x6baef30b（含 HandGesture_CopyDiff）
// 临时把 Dif 换成手势 Dif，在手势数据集上跑峰检测、TZ、坐标、ID 跟踪与事件。
void TouchProcess() {
    const Thresholds saved = SaveThresholds();
    FlashPrmt& fl = PrmtFlash();
    int16_t* dif = Buffers().dif;
    const uint16_t n = fl.sensorNum;
    TsaAssert(n <= kDifCells);
    std::array<int16_t, kDifCells> difBackup{};
    std::copy_n(dif, n, difBackup.begin());
    std::fill_n(dif, n, int16_t{0});
    ForEachEdgeCell([&](uint16_t cell) { dif[cell] = HandBufs().dif[cell]; });

    Prmt::Process();
    Touch::Clean();
    Peak::DetectInRange(PeakThold(), 0, static_cast<uint8_t>(fl.dim2Len - 1), 0, static_cast<uint8_t>(fl.dim1Len - 1));
    Peak::SortInSigAscending();
    fl.gripModeSigThold = static_cast<int16_t>(PrmtDynamic().sigThold / 2);
    fl.sigMult = fl.sigMultSD;
    fl.sigMultNormal = fl.sigMultSD;
    fl.gripFlags = kHandGripFlags;
    TouchZone::Process();
    Ctd::EcProcess();
    IdTracking::Process(false);
    Touch::AssignPreIdxBasedOnId();
    PrevTouch::AssignCurIdxBasedOnId();
    TouchEvent::Process();
    PrevTouch::PostProcess();
    PrevPeak::Process();

    std::copy_n(difBackup.begin(), n, dif);
    RestoreThresholds(saved);
}

// HandGesture_CheckValidity @ 0x6baeec0f：手势数据集上只有一个触点，且它与抬起的握持触点方向一致、距离适中
bool IsValid() {
    const TouchTable& tt = Touches();
    if (tt.count != 1) return false;
    const Point pos = tt.unit[0].pos;
    return IsAngleCorrelated(LineAngle(g_state.gripPos, pos), g_state.gripAngle) && !IsDistInvalid(pos, g_state.gripPos);
}

// HandGesture_PostProcess @ 0x6baeed20（含 TZFlagProcess / TimeRecord）
// 有效时去掉报点已久的格，记下区域图，区域里的格时间清零。
void PostProcess() {
    if (!IsValid()) return;
    HandBuffers& hb = HandBufs();
    const int16_t* timeRpt = Buffers().timeRpt;
    const uint16_t n = PrmtFlash().sensorNum;
    for (uint16_t i = 0; i < n; ++i) {
        if (timeRpt[i] < kReportedLongMs) hb.tz[i] = 0;
    }
    std::copy_n(hb.tz, GridBufSize(), hb.tzPrev);
    for (uint16_t i = 0; i < n; ++i) {
        if (hb.tz[i] != 0) hb.time[i] = 0;
    }
}

// HandGesture_GripDetection @ 0x6baef865
// 关联握持触点抬起时复位手势基线；之后 1 s 内手势 Dif 的峰值开始回落且仍高于峰门限时，在手势数据集上
// 跑一遍触点处理。没跑时清空手势数据集。
void GripDetection() {
    std::fill_n(HandBufs().tz, GridBufSize(), int8_t{0});
    if (IsOkToResetBaseline()) {
        g_state.flags |= kArmed;
        g_state.resetTime = Now();
        BaselineReset();
    }
    DataSwitch::ToHand();
    bool processed = false;
    if ((g_state.flags & kArmed) == 0 || kDifWindowMs < Now() - g_state.resetTime) {
        g_state.maxDif = INT16_MIN;
    } else {
        DifProcess();
        if (g_state.maxDif < g_state.prevMaxDif && PeakThold() < g_state.maxDif) {
            processed = true;
            g_state.flags &= ~kArmed;
            TouchProcess();
        }
    }
    if (processed) {
        PostProcess();
    } else {
        Touch::Clean();
        Peak::Clean();
        PrevTouch::PostProcess();
        PrevPeak::Process();
    }
    DataSwitch::ToGrid();
}

}  // namespace

// HandGesture_Malloc @ 0x6baedc30
bool Malloc() {
    const uint32_t cells = GridBufSize();
    HandBuffers& hb = HandBufs();
    hb.baseline = static_cast<int16_t*>(Mem::Allocate(cells * 2));
    if (hb.baseline == nullptr) return false;
    hb.dif = static_cast<int16_t*>(Mem::Allocate(cells * 2));
    if (hb.dif == nullptr) return false;
    hb.area = static_cast<int8_t*>(Mem::Allocate(cells));
    if (hb.area == nullptr) return false;
    hb.tz = static_cast<int8_t*>(Mem::Allocate(cells));
    if (hb.tz == nullptr) return false;
    hb.tzPrev = static_cast<int8_t*>(Mem::Allocate(cells));
    if (hb.tzPrev == nullptr) return false;
    hb.time = static_cast<int16_t*>(Mem::Allocate(cells * 2));
    return hb.time != nullptr;
}

// HandGesture_Free @ 0x6baedd66
void Free() {
    HandBuffers& hb = HandBufs();
    Mem::Release(hb.baseline);
    Mem::Release(hb.dif);
    Mem::Release(hb.area);
    Mem::Release(hb.tz);
    Mem::Release(hb.tzPrev);
    Mem::Release(hb.time);
}

// HandGesture_Init @ 0x6baedf1b
void Init() {
    g_state = {};
    std::fill_n(HandBufs().area, GridBufSize(), int8_t{0});
    std::fill_n(HandBufs().tz, GridBufSize(), int8_t{0});
    ResetRecord();
    BaselineReset();
}

// HandGesture_Process @ 0x6baef9fc
void Process() {
    TimeStep();
    TwoHandDetection();
    GripDetection();
    g_state.prevMaxDif = g_state.maxDif;
}

// HandGesture_Is2HandDetected @ 0x6baee0ce
bool Is2HandDetected() { return (g_state.flags & kTwoHand) != 0; }

}  // namespace Solvers::Tsa::Core::HandGesture
