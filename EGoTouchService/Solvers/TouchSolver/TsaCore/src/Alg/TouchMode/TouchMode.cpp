#include "Alg/TouchMode/TouchMode.h"

#include <array>
#include <cstdlib>

#include "Alg/Peak/Peak.h"
#include "Alg/Peak/PeakData.h"
#include "Alg/Sensor/Sensor.h"
#include "Alg/Touch/Touch.h"
#include "Alg/Touch/TouchData.h"
#include "App/TsaStatic.h"
#include "Common/Assert.h"
#include "Common/Image.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::TouchMode {

namespace {

enum Mode : uint32_t { kNoObj, kFinger, kGlove, kStylus, kModeNum };

constexpr int32_t kNotDetected = 1;
constexpr int32_t kDetected = 2;
constexpr int32_t kLevelNormal = 1;
constexpr int32_t kLevelConfirmed = 2;

constexpr uint8_t kZTholdFinger = 0x2;      // DynamicPrmt::zTholdFlags：允许手指模式确认
constexpr uint8_t kZTholdGlove = 0x4;
constexpr uint8_t kZTholdStylus = 0x8;
constexpr uint8_t kStylusFirst = 0x8;       // DynamicPrmt::touchModeFlags
constexpr uint32_t kFeatStylus = 0x1000;    // TsaStatic::featureFlags

constexpr uint32_t kSuppressFinger = 0x1;   // TouchUnit::suppressFlags
constexpr uint32_t kSuppressByMode = 0x2;   // 当前模式下不报
constexpr uint32_t kSuppressStylus = 0x4;   // 手套模式未开时一律抑制
constexpr uint32_t kSuppressNotStylus = 0x8;// 不是笔
constexpr uint32_t kVolatileWeakFinger = 0x800;  // TouchUnit::volatileFlags
constexpr uint32_t kPersistGlove = 0x4;          // TouchUnit::persistFlags：手套触点
constexpr uint32_t kPersistNoGlove = 0x80000;
constexpr uint32_t kEdgeFlagsSide = 0xf000;      // TouchUnit::edgeFlags：贴侧边
constexpr uint8_t kStylusMaxSpan = 3;
constexpr uint8_t kStrongFingerFrames = 2;
constexpr uint8_t kGloveFilterRatio = 0x40;

/// 一个模式的检测结果，与 TsaPrpt 里 touchMode / Level / Detect 三个字段一一对应。
struct ModeUnit {
    uint32_t mode;
    int32_t level;
    int32_t detect;
};

using DetectFn = void (*)(int32_t& detect, int32_t& level);
using ProcessFn = void (*)(int32_t detect, int32_t level);
struct Handler {
    DetectFn detect;
    ProcessFn process;
};

// SWTimer.c：按帧计数的软件定时器。
struct SwTimer {
    uint8_t running;
    uint8_t reserved01[0x3];
    int32_t start;

    // SWTimer_Start @ 0x6bb427d0
    void Start(int32_t now) {
        if (running != 0) return;
        running = 1;
        start = now;
    }
    // SWTimer_Stop @ 0x6bb42802
    void Stop() { running = 0; }
    // SWTimer_IsExpired @ 0x6bb42843
    [[nodiscard]] bool IsExpired(int32_t now, uint32_t frames) const {
        return running != 0 && static_cast<uint32_t>(now - start) >= frames;
    }
};
static_assert(sizeof(SwTimer) == 0x8);

/// g_tmSwitchTimer：模式切换的防抖计时。
SwTimer& SwitchTimer() { return Image::At<SwTimer>(0x6bbc8660); }
/// g_tmSwitchDebounce：[上一帧模式][新模式] 的切换防抖帧数。
using DebounceTable = std::array<std::array<uint16_t, kModeNum>, kModeNum>;
DebounceTable& SwitchDebounce() { return Image::At<DebounceTable>(0x6bb4f200); }
/// g_tmSwitchPriority：[上一帧模式] 下依次尝试的模式。
using PriorityTable = std::array<std::array<uint32_t, kModeNum>, kModeNum>;
PriorityTable& SwitchPriority() { return Image::At<PriorityTable>(0x6bb4f240); }

TouchTable& Cur() { return Touches(); }
bool StylusOff() { return (Static().featureFlags & kFeatStylus) == 0; }
bool GloveOn() { return (PrmtDynamic().zTholdFlags & kZTholdGlove) != 0 && (Static().featureFlags & kFeatGloveMode) != 0; }
bool StylusOn() { return (PrmtDynamic().zTholdFlags & kZTholdStylus) != 0 && (Static().featureFlags & kFeatKnuckle) != 0; }
bool CoverOn() { return (Static().featureFlags & kFeatSmartCover) != 0 || Static().field3F1 != 0; }

void SuppressAll(uint32_t flag) {
    TouchTable& t = Cur();
    for (uint8_t i = 0; i < t.count; ++i) t.unit[i].suppressFlags |= flag;
}

// ── 笔（指关节）过滤 ─────────────────────────────────────────────────────

// TouchMode_StylusIsClearanceAreaSatisfied @ 0x6ba79930
// 峰周围 (2r+1)² 方框的外圈上没有属于别的区域的格子（stylus 模式下 r 为 2，否则 3）。
bool IsClearanceAreaSatisfied(uint8_t idx) {
    const PeakEntry& pk = Peaks().entry[Cur().unit[idx].peakIdx];
    const FlashPrmt& fl = PrmtFlash();
    const int r = PrevTouches().field00C == 0 || StylusOff() ? 3 : 2;
    bool ok = true;
    for (int row = pk.dim2 - r; row <= pk.dim2 + r; ++row) {
        if (static_cast<uint8_t>(row) >= fl.dim2Len) continue;
        for (int col = pk.dim1 - r; col <= pk.dim1 + r; ++col) {
            if (static_cast<uint8_t>(col) >= fl.dim1Len) continue;
            if (std::abs(col - pk.dim1) != r && std::abs(row - pk.dim2) != r) continue;
            if (Sensor::IsInDifferentZones(static_cast<uint8_t>(col), static_cast<uint8_t>(row), pk.dim1, pk.dim2)) ok = false;
        }
    }
    return ok;
}

// TouchMode_GetStylusNum @ 0x6ba79c23
// 像笔的触点数：区域不超过 2×2、信号和不超过 3 倍手套退出门限、周围干净；不像的标记 kSuppressNotStylus。
uint8_t GetStylusNum() {
    TouchTable& t = Cur();
    uint8_t n = 0;
    for (uint8_t i = 0; i < t.count; ++i) {
        TouchUnit& u = t.unit[i];
        if (u.dim1Span >= kStylusMaxSpan || u.dim2Span >= kStylusMaxSpan || PrmtFlash().gloveExitThold * 3 < u.maxSigSum ||
            !IsClearanceAreaSatisfied(i)) {
            u.suppressFlags |= kSuppressNotStylus;
        } else if ((u.suppressFlags & kSuppressNotStylus) == 0) {
            ++n;
        }
    }
    return n;
}

bool IsStylusCandidate(const TouchUnit& u) { return (u.suppressFlags & kSuppressNotStylus) == 0 && (u.edgeFlags & kEdgeFlagsSide) == 0; }

// TouchMode_StylusGetCoreIndex @ 0x6ba79b0f
uint8_t StylusGetCoreIndex() {
    const TouchTable& t = Cur();
    for (uint8_t i = 0; i < t.count; ++i) {
        if (IsStylusCandidate(t.unit[i])) return i;
    }
    return kNoTouch;
}

// TouchMode_StylusFilterProcess @ 0x6ba79e26
// 像笔的触点多于一个时只留一个：最强峰所在的那个，否则离最强峰最远的那个（贴侧边的先排除）。
uint8_t StylusFilterProcess() {
    uint8_t n = GetStylusNum();
    if (n <= 1) return n;
    TouchTable& t = Cur();
    const auto top = static_cast<uint8_t>(Peak::GetIdxBySig(Peaks().maxSig));
    uint8_t keep = 0;
    uint8_t farthest = 0;
    bool topFound = false;
    uint16_t maxDist = 0;
    int32_t maxSigSum = INT32_MIN;
    for (uint8_t i = 0; i < t.count; ++i) {
        TouchUnit& u = t.unit[i];
        if ((u.suppressFlags & kSuppressNotStylus) != 0) continue;
        if ((u.edgeFlags & kEdgeFlagsSide) != 0) {
            u.suppressFlags |= kSuppressNotStylus;
            if (--n == 1) break;
            continue;
        }
        if (maxSigSum < u.maxSigSum) {
            maxSigSum = u.maxSigSum;
            keep = i;
        }
        if (u.peakIdx == top) topFound = true;
        const auto dist = static_cast<uint16_t>(Peak::GetSquareDistance(top, u.peakIdx));
        if (maxDist < dist) {
            farthest = i;
            maxDist = dist;
        }
    }
    if (n <= 1) return n;
    if (!topFound) keep = farthest;
    for (uint8_t i = 0; i < t.count; ++i) {
        if (i == keep) continue;
        t.unit[i].suppressFlags |= kSuppressNotStylus;
        --n;
    }
    return n;
}

// TouchMode_StylusFilter @ 0x6ba7a0ed
void StylusFilter() {
    const uint8_t n = StylusFilterProcess();
    if (StylusGetCoreIndex() != kNoTouch) Cur().field00C = n;
}

// ── 各模式的 Detect / Process ────────────────────────────────────────────

// TouchMode_NoObjDetect @ 0x6ba790ae：触点都被判为无效时检出。
void NoObjDetect(int32_t& detect, int32_t& level) {
    detect = kNotDetected;
    level = kLevelConfirmed;
    if (Cur().count == Cur().field00D) detect = kDetected;
}

// TouchMode_FingerDetect @ 0x6ba7911f
// 有达到峰门限的非弱触点时检出；全局峰 Zn 连续 3 帧超过 3 倍手套退出门限时确认手指并解除手套锁定。
void FingerDetect(int32_t& detect, int32_t& level) {
    detect = kNotDetected;
    level = kLevelNormal;
    if ((PrmtDynamic().zTholdFlags & kZTholdFinger) == 0 || !StylusOff()) return;
    const TouchTable& t = Cur();
    for (uint8_t i = 0; i < t.count; ++i) {
        const TouchUnit& u = t.unit[i];
        if ((u.volatileFlags & kVolatileWeakFinger) == 0 && PrmtDynamic().peakThold <= Peaks().entry[u.peakIdx].sig) {
            detect = kDetected;
            break;
        }
    }
    TsaPrpt& pr = Prpt();
    uint8_t& frames = Static().strongFingerFrames;
    if (pr.touchMode != kFinger) frames = 0;
    if (pr.touchMode != kFinger || pr.touchModeLevel != kLevelConfirmed) {
        frames = PrmtFlash().gloveExitThold * 3 < Peaks().maxZn ? static_cast<uint8_t>(frames + 1) : 0;
    }
    if (kStrongFingerFrames < frames) {
        level = kLevelConfirmed;
        pr.touchModeLatched = 0;
    }
}

// TouchMode_FingerProcess @ 0x6ba79318：手指模式确认（或未锁定手套）时，弱触点不报（信号失衡时除外）。
void FingerProcess(int32_t, int32_t level) {
    if (level != kLevelConfirmed && Prpt().touchModeLatched != 0) return;
    TouchTable& t = Cur();
    for (uint8_t i = 0; i < t.count; ++i) {
        TouchUnit& u = t.unit[i];
        if ((u.volatileFlags & kVolatileWeakFinger) != 0 && Static().fullIncellSignalDisparity == 0) u.suppressFlags |= kSuppressByMode;
    }
}

// TouchMode_GloveDetect @ 0x6ba793fe
// 手套 / 指关节 / 皮套开启时，有有效触点即检出。皮套，或已锁定手套 / 指关节时直接锁定并确认；否则出现
// 手套触点（且不是从手指模式来）时锁定、确认并让它补报，stylus 模式下有触点即锁定确认。
void GloveDetect(int32_t& detect, int32_t& level) {
    detect = kNotDetected;
    level = kLevelNormal;
    const bool glove = GloveOn();
    const bool stylus = StylusOn();
    if (!glove && !stylus && !CoverOn()) return;
    TouchTable& t = Cur();
    TsaPrpt& pr = Prpt();
    if (t.count != t.field00D) detect = kDetected;
    if ((pr.touchModeLatched != 0 && (glove || stylus)) || CoverOn()) {
        pr.touchModeLatched = 1;
        level = kLevelConfirmed;
        return;
    }
    for (uint8_t i = 0; i < t.count; ++i) {
        const TouchUnit& u = t.unit[i];
        if (glove && (u.persistFlags & kPersistGlove) != 0 && pr.touchMode != kFinger && (u.persistFlags & kPersistNoGlove) == 0) {
            pr.touchModeLatched = 1;
            level = kLevelConfirmed;
            Touch::CreatePostReport(i);
            return;
        }
        if (!StylusOff()) {
            pr.touchModeLatched = 1;
            level = kLevelConfirmed;
            return;
        }
    }
}

// TouchMode_GloveFilter @ 0x6ba796d1：手套模式未确认时，握持比偏大的非手指触点不报。
void GloveFilter() {
    TouchTable& t = Cur();
    for (uint8_t i = 0; i < t.count; ++i) {
        TouchUnit& u = t.unit[i];
        if ((u.suppressFlags & kSuppressFinger) == 0 && kGloveFilterRatio < u.field088) u.suppressFlags |= kSuppressByMode;
    }
}

// TouchMode_GloveProcess @ 0x6ba797a7
// 手套与皮套都没开（只是指关节）时全部按 kSuppressStylus 抑制；手套模式未确认时全部不报，确认后过滤。
void GloveProcess(int32_t, int32_t level) {
    if (!StylusOff()) return;
    if (!GloveOn() && !CoverOn()) {
        // TouchMode_StylusSuppression @ 0x6ba79ba6
        SuppressAll(kSuppressStylus);
    } else if (level == kLevelNormal) {
        SuppressAll(kSuppressByMode);
    } else {
        GloveFilter();
    }
}

// TouchMode_StylusDetect @ 0x6ba798b8
void StylusDetect(int32_t& detect, int32_t& level) {
    detect = kNotDetected;
    level = kLevelNormal;
}

void NoProcess(int32_t, int32_t) {}

/// g_tmHandlerTable：按模式编号。
constexpr std::array<Handler, kModeNum> kHandlers = {{
    {NoObjDetect, NoProcess},
    {FingerDetect, FingerProcess},
    {GloveDetect, GloveProcess},
    {StylusDetect, NoProcess},
}};

// TouchMode_SwitchPriorityProcess @ 0x6ba7900b
// 指关节模式开启时，按 touchModeFlags 决定无物体 / 手指两种状态下先试指关节还是先试手指。
void SwitchPriorityProcess() {
    if (!StylusOn()) return;
    const bool stylusFirst = (PrmtDynamic().touchModeFlags & kStylusFirst) != 0;
    PriorityTable& pri = SwitchPriority();
    pri[kFinger][0] = stylusFirst ? kStylus : kFinger;
    pri[kFinger][1] = stylusFirst ? kFinger : kStylus;
    pri[kNoObj][1] = pri[kFinger][0];
    pri[kNoObj][2] = pri[kFinger][1];
}

// TouchMode_NewModeProcess @ 0x6ba78c16
// 检出的模式与上一帧不同。上一帧模式未检出（detect < 2）时先执行新模式的 Process，再按防抖表计时（上一帧
// 级别为普通且新模式不是无物体时不防抖），未到时退回上一帧模式。上一帧 detect 为 4 时，没有触点才退回。
void NewModeProcess(ModeUnit& next, const ModeUnit& prev, const Handler& h) {
    SwTimer& timer = SwitchTimer();
    if (static_cast<uint32_t>(prev.detect) < 2) {
        h.process(next.detect, next.level);
        const uint16_t frames = prev.level == kLevelNormal && next.mode != kNoObj ? 0 : SwitchDebounce()[prev.mode][next.mode];
        const int32_t now = Static().frameCounter;
        timer.Start(now);
        if (timer.IsExpired(now, frames)) {
            timer.Stop();
            return;
        }
        if (prev.detect == 0) {
            next.detect = Prpt().prevTouchModeDetect;
            next.level = Prpt().prevTouchModeLevel;
        } else {
            next.detect = prev.detect;
            next.level = prev.level;
        }
        next.mode = prev.mode;
    } else if (prev.detect == 4) {
        h.process(next.detect, next.level);
        if (Cur().liftOffCount + Cur().count == 0) next = prev;
        timer.Stop();
    }
}

}  // namespace

// TouchMode_Init @ 0x6ba78b60
void Init() {
    TsaPrpt& pr = Prpt();
    pr.touchMode = kNoObj;
    pr.touchModeDetect = 0;
    pr.touchModeLevel = 0;
    pr.touchModeLatched = 0;
    SwitchTimer().Stop();
    DebounceTable& db = SwitchDebounce();
    db[kNoObj][kFinger] = 0;
    db[kFinger][kNoObj] = PrmtFlash().field044 == 0 ? 0x3c : 0;
    db[kNoObj][kGlove] = 0x14;
    db[kGlove][kNoObj] = 200;
    db[kGlove][kFinger] = 0;
    Static().strongFingerFrames = 0;
}

// TouchMode_Process @ 0x6ba78dde
void Process() {
    TsaPrpt& pr = Prpt();
    pr.prevTouchMode = pr.touchMode;
    pr.prevTouchModeLevel = pr.touchModeLevel;
    pr.prevTouchModeDetect = pr.touchModeDetect;
    ModeUnit prev{pr.prevTouchMode, 0, 0};
    TsaAssert(prev.mode < kModeNum);
    SwitchPriorityProcess();
    if ((PrmtDynamic().zTholdFlags & kZTholdStylus) != 0) StylusFilter();
    // 没有模式检出时沿用上一帧的模式（检测结果取本帧对它的检测）
    ModeUnit result{};
    bool found = false;
    for (uint32_t i = 0; i < kModeNum && !found; ++i) {
        ModeUnit next{SwitchPriority()[prev.mode][i], 0, 0};
        TsaAssert(next.mode < kModeNum);
        const Handler& h = kHandlers[next.mode];
        h.detect(next.detect, next.level);
        if (next.mode == prev.mode) {
            prev.detect = next.detect;
            prev.level = next.level;
        }
        if (next.detect != kDetected) continue;
        if (next.mode == prev.mode) {
            h.process(kDetected, next.level);
            SwitchTimer().Stop();
        } else {
            NewModeProcess(next, prev, h);
        }
        result = next;
        found = true;
    }
    if (!found) result = prev;
    pr.touchMode = result.mode;
    pr.touchModeDetect = result.detect;
    pr.touchModeLevel = result.level;
}

}  // namespace Solvers::Tsa::Core::TouchMode

// raw TSA 还在调用的入口
