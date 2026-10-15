#include "Alg/Filter/Filter.h"

#include <algorithm>
#include <cstdlib>

#include "Alg/Filter/IirFilter.h"
#include "Alg/Peak/PeakData.h"
#include "Alg/Touch/PrevTouch.h"
#include "Alg/Touch/Touch.h"
#include "Alg/Touch/TouchData.h"
#include "App/Scenario.h"
#include "App/TsaOut.h"
#include "App/TsaStatic.h"
#include "Common/Bss.h"
#include "Common/TsaMath.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::Filter {

namespace {

constexpr uint32_t kFeatClickSuppress = 0x100;   // TsaStatic::featureFlags：点击抑制
constexpr uint32_t kFeatStylus = 0x1000;
constexpr uint32_t kPersistReported = 0x8;         // persistFlags：报过点
constexpr uint32_t kPersistHeavyCoef = 0x10;       // IIR 用重系数组（多指区域或全局异常时置位，此后一直保持）
constexpr uint32_t kPersistPostReport = 0x40;      // PrevTouchUnit::persistFlags：报点后续帧
constexpr uint32_t kPersistClickLocked = 0x800;    // 坐标锁在按下点（点击抑制）
constexpr uint32_t kPersistMovedOver = 0x8000;     // 离第一个报点位置超过点击抑制距离
constexpr uint32_t kPersistClickToSlide = 0x1000;  // 转为滑动
constexpr uint32_t kPersistRestarted = 0x20000;    // 本帧重新开始报点
constexpr uint32_t kVolatileFalseFlick = 0x100000; // Touch：本帧位移只是形状变化
constexpr uint8_t kIirShift = 5;                   // IIR 权重的满量程 32
constexpr uint8_t kCurveCoefMin = 8;               // 折返跟踪期间的最小 IIR 权重
constexpr int kEdgeJumpQ8 = 0x31;                  // 坐标贴屏幕边时，位移超过它直接取当前值
constexpr uint8_t kLiftOffMinHistory = 30;
constexpr uint8_t kLiftOffMaxCount = 11;
constexpr uint16_t kLiftOffZDropPercent = 0x1f;    // Z 值相对上一帧下降超过 30%
constexpr uint32_t kLiftOffMinDurationMs = 0x12d;
constexpr uint8_t kHandLiftSigSumRatio = 0x1c;     // sigSum 与上一帧之比（×32）低于它算手在抬起
constexpr uint8_t kCurveReturnMaxFrames = 5;
constexpr int kCurveMargin = 0x180;                // 离屏幕边超过 1.5 格才识别折返
constexpr uint32_t kCurveWindowMs = 0x50;          // 往回看的时间窗
constexpr uint32_t kCurveMinTimeMs = 7;
constexpr uint32_t kCurveFarPath = 0x98;
constexpr uint32_t kCurveMinPath = 0x80;
constexpr int kSpeedFrames = 6;
constexpr uint8_t kPredictMaxFrames = 12;
constexpr uint8_t kPostReportCoefMax = 0x20;

/// 调试观测量（只写）：滤波前、IIR 后的显示坐标，抬起检测的 Z 值降幅。
uint32_t& PosPreFilterDim1() { return Slot<uint32_t>(Bss().xPosPreFilter); }
uint32_t& PosPreFilterDim2() { return Slot<uint32_t>(Bss().yPosPreFilter); }
uint32_t& PosAftIirDim1() { return Slot<uint32_t>(Bss().xPosAftIirFilter); }
uint32_t& PosAftIirDim2() { return Slot<uint32_t>(Bss().yPosAftIirFilter); }
uint32_t& ZValPrevToCurRatio() { return Slot<uint32_t>(Bss().zValPrevToCurRatio); }

TouchUnit& Cur(uint8_t idx) { return Touches().unit[idx]; }
PrevTouchUnit& Prev(uint8_t idx) { return PrevTouches().unit[idx]; }

bool StylusOff() { return (Static().featureFlags & kFeatStylus) == 0; }
bool ClickSuppressOn() { return (Static().featureFlags & kFeatClickSuppress) != 0 && StylusOff(); }

/// 贴到屏幕边（Q8.8 坐标为 0 或最后一个值）。
bool OnScreenEdge(uint16_t pos, uint16_t lenQ8) { return pos == lenQ8 - 1 || pos == 0; }

/// 一个像素对应的 Q8.8 坐标数（dim1Res 为 0 时为 fallback）。
uint32_t Q8PerPixel(uint32_t fallback) {
    const uint16_t res = PrmtFlash().dim1Res;
    return res == 0 ? fallback : PrmtFlashConst().dim1LenQ8 / res;
}

HistoryEntry History(uint8_t prevIdx, int pos) {
    HistoryEntry e;
    PrevTouch::HistoryBufferGet(prevIdx, static_cast<uint8_t>(pos), e);
    return e;
}

// ── 抗抖动 ───────────────────────────────────────────────────────────────

struct JitResult {
    int dim1;
    int dim2;
};

/// 一个方向上向当前值跟进后剩下的滞后量：按半径占总位移的比例分配，但不少于 maxStep（总位移更小时取全部）。
int JitLag(uint32_t delta, int radius, int maxStep, int dist) {
    int lag = static_cast<int>(radius * delta) / dist;
    if (std::abs(lag) < maxStep) {
        const int absDelta = std::abs(static_cast<int>(delta));
        lag = absDelta < maxStep ? static_cast<int>(delta) : static_cast<int>(maxStep * delta) / absDelta;
    }
    return lag;
}

// JITFilter @ 0x6ba602c6（MISC.c）
// 与上一帧的距离不超过 radius 时保持上一帧坐标，否则停在距当前值约 radius 的地方。
JitResult JitFilter(int curDim1, int curDim2, int prevDim1, int prevDim2, int radius, int maxStep) {
    const auto d1 = static_cast<uint32_t>(curDim1 - prevDim1);
    const auto d2 = static_cast<uint32_t>(curDim2 - prevDim2);
    const uint32_t sq = d2 * d2 + d1 * d1;
    maxStep = std::min(maxStep, radius);
    const int dist = Math::SqrtUint32(sq);
    if (static_cast<uint32_t>(radius * radius) >= sq || dist == 0) return {prevDim1, prevDim2};
    return {curDim1 - JitLag(d1, radius, maxStep, dist), curDim2 - JitLag(d2, radius, maxStep, dist)};
}

// ── 自适应 IIR 的系数 ────────────────────────────────────────────────────

// Filter_NeedHeavyCoef @ 0x6ba5c240：多指拆分出的触点或有全局异常时改用重系数组（一旦置位一直保持）。
bool NeedHeavyCoef(uint8_t c) {
    TouchUnit& cur = Cur(c);
    if (cur.zoneShape == 3) cur.persistFlags |= kPersistHeavyCoef;
    if (Touches().exceptionFlags != 0) cur.persistFlags |= kPersistHeavyCoef;
    return (cur.persistFlags & kPersistHeavyCoef) != 0;
}

// Filter_IsHandLift @ 0x6ba5c644
bool IsHandLift(uint8_t c) { return Cur(c).sigSumRatio < kHandLiftSigSumRatio; }

// Filter_GetAdaptiveCoef @ 0x6ba5c690
// AIIR 按与上一帧 IIR 坐标的距离取权重；折返跟踪期间不低于 8；与上一帧的权重相差超过 1 时每帧只挪 1。
uint8_t GetAdaptiveCoef(uint8_t c, uint8_t p) {
    TouchUnit& cur = Cur(c);
    PrevTouchUnit& prev = Prev(p);
    const auto d1 = static_cast<uint16_t>(std::abs(cur.pos.dim1 - prev.iirPos.dim1));
    const auto d2 = static_cast<uint16_t>(std::abs(cur.pos.dim2 - prev.iirPos.dim2));
    const auto dist = static_cast<uint16_t>(Math::SqrtUint32(static_cast<uint32_t>(d2) * d2 + static_cast<uint32_t>(d1) * d1));
    uint8_t coef = Aiir::GetCoef(dist);
    if (prev.curveTracking == 1) {
        cur.curveTracking = 1;
        coef = std::max(coef, kCurveCoefMin);
    } else if ((cur.persistFlags & kPersistMovedOver) != 0 && !IsHandLift(c)) {
        cur.curveTracking = 1;
    }
    if (prev.prevIirCoef == 0) prev.prevIirCoef = coef;
    if (prev.prevIirCoef + 1 < coef) {
        prev.prevIirCoef = --coef;
    } else if (coef + 1 < prev.prevIirCoef) {
        prev.prevIirCoef = ++coef;
    } else {
        coef = prev.prevIirCoef;
    }
    return coef;
}

// Filter_GetCoefBasedOnDistance @ 0x6ba5c99c
// 选一组 AIIR 参数（普通 / 重 / stylus）写进 RamPrmt，再求权重。报点后续帧改用逐帧翻倍的权重（到 32 为止）。
uint8_t GetCoefBasedOnDistance(uint8_t c, uint8_t p) {
    const FlashPrmt& fl = PrmtFlash();
    const FlashConstPrmt& fc = PrmtFlashConst();
    const int set = !StylusOff() ? 2 : NeedHeavyCoef(c) ? 1 : 0;
    PrmtRam().aiir = fl.aiir[set];
    PrmtRamConst().aiirScaledCoefToDeltaRatio = fc.aiirCoefToDeltaRatio[set];
    uint8_t coef = GetAdaptiveCoef(c, p);
    PrevTouchUnit& prev = Prev(p);
    if ((prev.persistFlags & kPersistPostReport) != 0) {
        if (prev.postReportIirCoef == 0) {
            prev.postReportIirCoef = 2;
        } else if (prev.postReportIirCoef < kPostReportCoefMax) {
            prev.postReportIirCoef = static_cast<uint8_t>(prev.postReportIirCoef * 2);
        }
        coef = prev.postReportIirCoef;
    }
    return coef;
}

// ── 折返外推 ─────────────────────────────────────────────────────────────

// Filter_IsNeedCurve @ 0x6ba5cec2：坐标离屏幕边都超过 1.5 格。
bool IsNeedCurve(uint8_t c) {
    const Point& pos = Cur(c).pos;
    const FlashConstPrmt& fc = PrmtFlashConst();
    return kCurveMargin < pos.dim1 && pos.dim1 < fc.dim1LenQ8 - kCurveMargin && kCurveMargin < pos.dim2 &&
           pos.dim2 < fc.dim2LenQ8 - kCurveMargin;
}

// Filter_GetSpeed @ 0x6ba5cf7a
// 轨迹最后 6 段沿折返方向的平均速度（每段坐标差的绝对值），符号取这 6 段的总位移方向。
int GetSpeed(uint8_t p, int16_t last, bool alongDim1) {
    const int first = last - kSpeedFrames;
    if (first < 0) return 0;
    auto coord = [&](const HistoryEntry& e) -> int {
        return alongDim1 ? static_cast<uint16_t>(e.pos.dim1) : static_cast<uint16_t>(e.pos.dim2);
    };
    int sum = 0;
    for (int i = first; i < last; ++i) sum += std::abs(coord(History(p, i + 1)) - coord(History(p, i)));
    int speed = sum / (last - first);
    if (coord(History(p, last)) - coord(History(p, first)) < 0) speed = -speed;
    return speed;
}

// Filter_GetTurnNum @ 0x6ba5d0dc / Filter_GetForwardNum @ 0x6ba5d12a
// 速度越快转向越快：转向帧数 660 / |v|（到 12 为止）、前冲帧数 60 / |v|（到 2 为止）；|v| ≤ 4 时都为 0。
uint8_t FramesBySpeed(int speed, int scale, int maxFrames) {
    if (-4 <= speed && speed <= 4) return 0;
    return static_cast<uint8_t>(std::min(scale / std::abs(speed), maxFrames));
}

// Filter_IsCurveReturn @ 0x6ba5d178
// 往回看约 80 ms 的轨迹：先走够一段路（> 0x98）、之后又过了几毫秒的那个点，若比轨迹终点离起点更远，
// 说明轨迹折返了。折返基本沿一个轴时开始外推，按速度算转向与前冲的帧数。
void IsCurveReturn(uint8_t c, uint8_t p) {
    if (!IsNeedCurve(c)) return;
    PrevTouchUnit& prev = Prev(p);
    const uint8_t n = prev.historyCount;
    if (n == 0 || Cur(c).curveTracking != 1) return;
    uint32_t path = 0;
    uint32_t elapsed = 0;
    int16_t farIdx = -1;
    int16_t timeIdx = -1;
    for (uint8_t k = n;;) {
        if (--k == 0) break;
        const HistoryEntry a = History(p, k);
        const HistoryEntry b = History(p, k - 1);
        const auto dx = static_cast<int16_t>(a.pos.dim1 - b.pos.dim1);
        const auto dy = static_cast<int16_t>(a.pos.dim2 - b.pos.dim2);
        elapsed += static_cast<uint32_t>(static_cast<int>(a.time) - static_cast<int>(b.time));
        path += static_cast<uint32_t>(Math::SqrtUint32(static_cast<uint32_t>(dy * dy + dx * dx)));
        if (farIdx < 0 && kCurveFarPath < path) farIdx = static_cast<int16_t>(k - 1);
        if (timeIdx < 0 && kCurveMinTimeMs < elapsed) timeIdx = static_cast<int16_t>(k - 1);
        if (elapsed >= kCurveWindowMs) break;
    }
    if (elapsed < kCurveWindowMs || path <= kCurveMinPath || farIdx < 0 || timeIdx < 0 || timeIdx <= farIdx) return;
    const HistoryEntry from = History(p, farIdx);
    const HistoryEntry end = History(p, n - 1);
    const HistoryEntry mid = History(p, timeIdx);
    const auto ex = static_cast<int16_t>(end.pos.dim1 - from.pos.dim1);
    const auto ey = static_cast<int16_t>(end.pos.dim2 - from.pos.dim2);
    const auto mx = static_cast<int16_t>(mid.pos.dim1 - from.pos.dim1);
    const auto my = static_cast<int16_t>(mid.pos.dim2 - from.pos.dim2);
    const auto endDist = static_cast<uint32_t>(Math::SqrtUint32(static_cast<uint32_t>(ey * ey + ex * ex)));
    const auto midDist = static_cast<uint32_t>(Math::SqrtUint32(static_cast<uint32_t>(my * my + mx * mx)));
    if (endDist >= midDist) return;
    prev.curveReturnFrames = 1;
    if (std::abs(ey) * 5 < std::abs(ex) && std::abs(my) * 5 < std::abs(mx)) {
        prev.predictFrames = 1;
        prev.predictAlongDim1 = 1;
    } else if (std::abs(ex) * 5 < std::abs(ey) && std::abs(mx) * 5 < std::abs(my)) {
        prev.predictFrames = 1;
        prev.predictAlongDim1 = 0;
    }
    if (PrmtFlash().noCurvePredict == 0 && prev.predictFrames != 0) {
        const int speed = GetSpeed(p, static_cast<int16_t>(n - 1), prev.predictAlongDim1 != 0);
        prev.predictTurnFrames = FramesBySpeed(speed, 0x294, kPredictMaxFrames);
        prev.predictForwardFrames = FramesBySpeed(speed, 0x3c, 2);
    }
}

// Filter_IsPredictFinish @ 0x6ba5d662
// 外推期间：按轨迹最后一段位移推算 predictPos，转向帧数用完后结束。noCurvePredict 时不外推，
// 改为在 12 帧内按「先减小、再置 0、再恢复」的节奏调整抗抖动半径。
void IsPredictFinish(uint8_t c, uint8_t p) {
    TouchUnit& cur = Cur(c);
    PrevTouchUnit& prev = Prev(p);
    const uint32_t radius = PrmtFlashConst().jitRadius;
    if (PrmtFlash().noCurvePredict == 0) {
        if (prev.predictFrames < prev.predictTurnFrames) {
            cur.jitRadius = radius;
            const uint8_t n = prev.historyCount;
            if (1 < n) {
                const HistoryEntry a = History(p, n - 1);
                const HistoryEntry b = History(p, n - 2);
                prev.predictPos.dim1 = static_cast<int16_t>(prev.jitPos.dim1 + a.pos.dim1 - b.pos.dim1);
                prev.predictPos.dim2 = static_cast<int16_t>(prev.jitPos.dim2 + a.pos.dim2 - b.pos.dim2);
            }
            ++prev.predictFrames;
        } else {
            prev.predictFrames = 0;
            cur.jitRadius = radius;
            prev.predictPos = {0, 0};
        }
        return;
    }
    if (prev.predictFrames >= kPredictMaxFrames) {
        prev.predictFrames = 0;
        cur.jitRadius = radius;
        return;
    }
    const uint32_t step = Q8PerPixel(1);
    if (prev.predictFrames < 3) {
        cur.jitRadius = step - step * prev.predictFrames * 2 + radius;
    } else if (prev.predictFrames < 6) {
        cur.jitRadius = 0;
    } else {
        cur.jitRadius = radius - step * (kPredictMaxFrames - prev.predictFrames);
    }
    ++prev.predictFrames;
}

// Filter_IsHandLiftFinish @ 0x6ba5dc30
void IsHandLiftFinish(uint8_t c, uint8_t p) {
    const uint8_t frames = Prev(p).curveReturnFrames;
    Cur(c).curveReturnFrames = frames < kCurveReturnMaxFrames ? static_cast<uint8_t>(frames + 1) : 0;
}

// Filter_IsTouchLiftOff @ 0x6ba5c363
// 轨迹够长、触点已移出点击范围且按下够久时，Z 值比上一帧骤降 30% 以上算抬起中（最多记 11 次）。
bool IsTouchLiftOff(uint8_t c, uint8_t p) {
    TouchUnit& cur = Cur(c);
    const PrevTouchUnit& prev = Prev(p);
    uint16_t drop = 0;
    cur.liftOffJitter = 0;
    if (prev.historyCount < kLiftOffMinHistory || PrmtFlash().liftOffJitterSuppress == 0) return false;
    if (cur.pressure < prev.pressure) {
        drop = static_cast<uint16_t>(static_cast<uint32_t>(prev.pressure) * 100 / cur.pressure - 100);
        ZValPrevToCurRatio() = drop;
    }
    cur.liftOffJitterCount = prev.liftOffJitterCount;
    if (cur.liftOffJitterCount >= kLiftOffMaxCount) return false;
    if (drop < kLiftOffZDropPercent || cur.durationMs < kLiftOffMinDurationMs || (cur.persistFlags & kPersistMovedOver) == 0) return false;
    cur.liftOffJitter = 1;
    ++cur.liftOffJitterCount;
    return true;
}

// Filter_IsTouchLiftOffJitterSuppressEnable @ 0x6ba5c5fc
bool IsLiftOffJitterSuppressed(uint8_t c) { return PrmtFlash().liftOffJitterSuppress != 0 && Cur(c).liftOffJitter != 0; }

// ── IIR ──────────────────────────────────────────────────────────────────

/// Filter_ProcessPDim1 @ 0x6ba5cc76 / Dim2 @ 0x6ba5cd9c：带 8 位定点余数的一阶 IIR。
void IirDim(uint16_t pos, uint16_t prevPos, uint8_t prevRem, uint8_t coef, uint16_t& out, uint8_t& rem) {
    const int v = Iir::Process(pos << 8, prevPos * 0x100 + prevRem, coef, kIirShift);
    out = static_cast<uint16_t>(v / 256);
    rem = static_cast<uint8_t>(v);
}

// Filter_IIRPDims @ 0x6ba5dcd4
// 折返外推期间手在抬起、或抬起骤降时权重取 1（几乎不跟当前值）。贴屏幕边且跳动较大的方向直接取当前值。
void IirPDims(uint8_t c, uint8_t p) {
    TouchUnit& cur = Cur(c);
    PrevTouchUnit& prev = Prev(p);
    const FlashConstPrmt& fc = PrmtFlashConst();
    uint8_t coef = GetCoefBasedOnDistance(c, p);
    if (prev.predictFrames == 0) IsCurveReturn(c, p);
    if (prev.predictFrames != 0) IsPredictFinish(c, p);
    if ((prev.curveReturnFrames != 0 || prev.predictFrames != 0) && IsHandLift(c) &&
        !Scenario::IsGame()) {
        IsHandLiftFinish(c, p);
        coef = 1;
        cur.jitRadius = fc.jitRadius;
    }
    if (IsTouchLiftOff(c, p)) {
        coef = 1;
        cur.jitRadius = fc.jitRadius;
    }
    const auto jump1 = static_cast<uint16_t>(std::abs(cur.pos.dim1 - prev.pos.dim1));
    const auto jump2 = static_cast<uint16_t>(std::abs(cur.pos.dim2 - prev.pos.dim2));
    if (OnScreenEdge(cur.pos.dim1, fc.dim1LenQ8) && kEdgeJumpQ8 < jump1) {
        cur.iirPos.dim1 = cur.pos.dim1;
    } else {
        IirDim(cur.pos.dim1, prev.iirPos.dim1, prev.iirRemDim1, coef, cur.iirPos.dim1, cur.iirRemDim1);
    }
    if (OnScreenEdge(cur.pos.dim2, fc.dim2LenQ8) && kEdgeJumpQ8 < jump2) {
        cur.iirPos.dim2 = cur.pos.dim2;
    } else {
        IirDim(cur.pos.dim2, prev.iirPos.dim2, prev.iirRemDim2, coef, cur.iirPos.dim2, cur.iirRemDim2);
    }
    const uint16_t scale = PrmtFlash().outputResScale;
    PosAftIirDim1() = Out::Dim1PosInternal(cur.iirPos.dim1, scale);
    PosAftIirDim2() = Out::Dim2PosInternal(cur.iirPos.dim2, scale);
}

// ── 点击抑制 ─────────────────────────────────────────────────────────────

/// 坐标锁在上一帧的 IIR 坐标（按下点），记下与当前坐标的偏移。
void LockClick(uint8_t c, uint8_t p) {
    TouchUnit& cur = Cur(c);
    PrevTouchUnit& prev = Prev(p);
    cur.iirPos = prev.iirPos;
    cur.persistFlags |= kPersistClickLocked;
    prev.clickToFlickOffset.dim1 = static_cast<int16_t>(cur.pos.dim1 - cur.iirPos.dim1);
    prev.clickToFlickOffset.dim2 = static_cast<int16_t>(cur.pos.dim2 - cur.iirPos.dim2);
}

// Filter_ReportClickToFlickDim1OffsetCheck @ 0x6ba5e149 / Dim2 @ 0x6ba5e409
// 每帧最多收回 min(IIR 位移 / 4, clickToFlickStep) 的偏移；偏移收回到不足一步时返回 true。
bool RecoverClickOffset(uint16_t& iir, uint16_t prevIir, int16_t& offset) {
    const auto stepLimit = static_cast<int16_t>(Out::ExtToIntDist(PrmtDynamic().clickToFlickStep));
    const int moved = std::abs(static_cast<int>(static_cast<uint32_t>(iir) - prevIir));
    const auto step = static_cast<int16_t>(std::min(moved / 4, static_cast<int>(stepLimit)));
    if (step <= offset) {
        iir = static_cast<uint16_t>(iir + step);
        offset = static_cast<int16_t>(offset - step);
        return false;
    }
    if (-step < offset) return true;
    iir = iir < step ? 0 : static_cast<uint16_t>(iir - step);
    offset = static_cast<int16_t>(offset + step);
    return false;
}

// Filter_ReportClickToFlickProcessUnit @ 0x6ba5e6c3
// 点击锁定后转为滑动：扣掉锁定期间的偏移做 IIR，再逐帧把偏移收回；两个方向都收回后解除锁定。
bool ReportClickToFlick(uint8_t c, uint8_t p) {
    TouchUnit& cur = Cur(c);
    PrevTouchUnit& prev = Prev(p);
    const Point saved = cur.pos;
    const SPoint off = prev.clickToFlickOffset;
    cur.pos.dim1 = cur.pos.dim1 < off.dim1 ? 0 : static_cast<uint16_t>(cur.pos.dim1 - off.dim1);
    cur.pos.dim2 = cur.pos.dim2 < off.dim2 ? 0 : static_cast<uint16_t>(cur.pos.dim2 - off.dim2);
    IirPDims(c, p);
    cur.pos = saved;
    const bool done1 = RecoverClickOffset(cur.iirPos.dim1, prev.iirPos.dim1, prev.clickToFlickOffset.dim1);
    const bool done2 = RecoverClickOffset(cur.iirPos.dim2, prev.iirPos.dim2, prev.clickToFlickOffset.dim2);
    return done1 && done2;
}

// Filter_JitFilterProcessUnit @ 0x6ba5e994
// 抗抖动半径平时取 jitRadius，外推期间取本帧调整过的半径；抬起骤降时再按 jitRadius 重做一次。贴屏幕边的
// 方向不做；游戏场景每帧最多再向上一帧挪 1。外推未结束时按进度在 predictPos 与滤波结果之间插值。
void JitFilterProcessUnit(uint8_t c, uint8_t p) {
    TouchUnit& cur = Cur(c);
    const PrevTouchUnit& prev = Prev(p);
    const FlashConstPrmt& fc = PrmtFlashConst();
    const int maxStep = PrmtFlash().dim1Res == 0 ? 2 : static_cast<int>(Q8PerPixel(1) + 1);
    const int radius = prev.predictFrames == 0 ? fc.jitRadius : static_cast<int>(cur.jitRadius);
    JitResult out = JitFilter(cur.iirPos.dim1, cur.iirPos.dim2, prev.jitPos.dim1, prev.jitPos.dim2, radius, maxStep);
    if (IsLiftOffJitterSuppressed(c)) {
        out = JitFilter(cur.iirPos.dim1, cur.iirPos.dim2, prev.jitPos.dim1, prev.jitPos.dim2, fc.jitRadius, maxStep);
    }
    const bool game = Scenario::IsGame();
    auto settle = [&](uint16_t iir, uint16_t lenQ8, int filtered, uint16_t prevJit, uint16_t& jit) {
        if (OnScreenEdge(iir, lenQ8)) {
            jit = iir;
            return;
        }
        jit = static_cast<uint16_t>(filtered);
        if (!game) return;
        if (prevJit < jit) --jit;
        if (jit < prevJit) ++jit;
    };
    settle(cur.iirPos.dim1, fc.dim1LenQ8, out.dim1, prev.jitPos.dim1, cur.jitPos.dim1);
    settle(cur.iirPos.dim2, fc.dim2LenQ8, out.dim2, prev.jitPos.dim2, cur.jitPos.dim2);
    if (PrmtFlash().noCurvePredict == 0 && prev.predictForwardFrames < prev.predictFrames - 1) {
        const int w = (prev.predictFrames - 1) * 0x100 / prev.predictTurnFrames;
        cur.jitPos.dim1 = static_cast<uint16_t>(((0x100 - w) * prev.predictPos.dim1 + static_cast<int>(cur.jitPos.dim1 * static_cast<uint32_t>(w))) / 256);
        cur.jitPos.dim2 = static_cast<uint16_t>(((0x100 - w) * prev.predictPos.dim2 + static_cast<int>(cur.jitPos.dim2 * static_cast<uint32_t>(w))) / 256);
    }
}

// Filter_ModeTouchReportedProcessUnit @ 0x6ba5f1c2
// 点击抑制开启时，弱且未移出点击范围的点击、或只是形状变化（false flick）的触点锁在按下点；已锁定的
// 点击转为滑动后收回偏移；其余触点（需要时）做 IIR。最后做抗抖动。
void ModeTouchReportedProcessUnit(uint8_t c, uint8_t p) {
    TouchUnit& cur = Cur(c);
    const PrevTouchUnit& prev = Prev(p);
    const bool weakClick = cur.znRatio < 0x10 && cur.peakSigRatio < 0x14 &&
                           Peaks().entry[cur.peakIdx].sig < PrmtDynamic().touchThold / 2 && prev.action == TouchAction::kClick &&
                           (prev.persistFlags & kPersistMovedOver) == 0;
    const bool falseFlick =
        (cur.persistFlags & (kPersistMovedOver | kPersistClickToSlide)) == 0 && (cur.volatileFlags & kVolatileFalseFlick) != 0;
    const bool lockable = (cur.action & TouchAction::kClick) != 0 && (cur.persistFlags & kPersistMovedOver) == 0;
    if ((weakClick || falseFlick || lockable) && ClickSuppressOn()) {
        LockClick(c, p);
    } else if ((cur.persistFlags & kPersistClickLocked) != 0) {
        if (ReportClickToFlick(c, p)) cur.persistFlags &= ~kPersistClickLocked;
    } else if (Touch::NeedXyFilter(c, p)) {
        IirPDims(c, p);
    } else {
        cur.iirPos = cur.pos;
    }
    JitFilterProcessUnit(c, p);
}

// Filter_ReportedProcessUnit @ 0x6ba5fb41
// 历史触点未报过点、或本帧重新开始报点时不滤波，否则按模式滤波。
void ReportedProcessUnit(uint8_t c, uint8_t p) {
    TouchUnit& cur = Cur(c);
    const PrevTouchUnit& prev = Prev(p);
    if ((prev.persistFlags & kPersistReported) == 0 ||
        ((cur.persistFlags & kPersistRestarted) != 0 && (prev.persistFlags & kPersistRestarted) == 0)) {
        cur.jitRadius = PrmtFlashConst().jitRadius;
        cur.iirPos = cur.pos;
        cur.jitPos = cur.pos;
        cur.iirRemDim1 = 0;
        cur.iirRemDim2 = 0;
        return;
    }
    ModeTouchReportedProcessUnit(c, p);
}

}  // namespace

// Filter_ProcessUnit @ 0x6ba5fd96
void ProcessUnit(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& cur = Cur(curIdx);
    const uint16_t scale = PrmtFlash().outputResScale;
    PosPreFilterDim1() = Out::Dim1PosInternal(cur.pos.dim1, scale);
    PosPreFilterDim2() = Out::Dim2PosInternal(cur.pos.dim2, scale);
    if ((cur.persistFlags & kPersistReported) == 0) {
        cur.iirPos = cur.pos;
        cur.jitPos = cur.pos;
    } else {
        ReportedProcessUnit(curIdx, prevIdx);
    }
    cur.rptPos = cur.jitPos;
}

}  // namespace Solvers::Tsa::Core::Filter

// raw TouchReport 还在调用的入口
