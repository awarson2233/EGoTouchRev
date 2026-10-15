#include "Extension/Thenar/Thenar.h"

#include <algorithm>

#include "Alg/Touch/TouchData.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Extension/Thenar/ThenarData.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::Thenar {

namespace {

constexpr uint16_t kDifSize = 0x1324;  // 扫描时 Dif 下标的上限
constexpr uint8_t kMaxPoints = 20;
constexpr int16_t kFallingGradient = -1000;  // 沿边梯度低于它：信号开始回落
constexpr int16_t kRisingGradient = 1000;    // 回落中梯度高于它：另起一段
constexpr uint16_t kShortLength = 6;         // 短于它的段算短段
constexpr uint64_t kGestureConfirmMs = 800;
constexpr uint64_t kGestureHoldMs = 100;

constexpr uint32_t kSideLeft = 4;   // ThenarInfo::side
constexpr uint32_t kSideRight = 8;
constexpr uint32_t kStateStableReported = 0x1;  // TouchTable::stateFlags
constexpr uint32_t kExtLeftGrip = 0x2;          // TouchTable::extFlags
constexpr uint32_t kExtRightGrip = 0x4;
constexpr uint32_t kExtLeftPalm = 0x8;
constexpr uint32_t kExtRightPalm = 0x10;

constexpr uint32_t kEnterMax = 0x25;       // 侧握计数：判在这一侧时每帧 +12（另一侧已离开时），否则 -1
constexpr uint32_t kEnterDecayMax = 0x24;
constexpr uint32_t kEnterStep = 12;
constexpr uint32_t kEnterReported = 0x23;  // 超过它置 extFlags
constexpr uint32_t kExitMax = 0x10;        // 离开计数：不在这一侧时每帧 +1
constexpr uint32_t kExitOther = 0xf;       // 另一侧离开超过它才累加
constexpr uint32_t kExitReset = 0xc;       // 离开超过它时侧握计数清零

// 0x6bbaa8d0 / 0x6bbaa8d8：左（0）/ 右（1）两侧的侧握与离开计数
uint32_t g_sideEnter[2];
uint32_t g_sideExit[2];

/// 沿侧边扫描的坐标系：x 跨边（0 与 xLen-1 是两条侧边），y 沿边。下标 = y·yStep + x·xStep。
struct EdgeScan {
    uint8_t xStep;
    uint8_t yStep;
    uint8_t xLen;
    uint8_t yLen;
};

EdgeScan Scan() {
    const FlashPrmt& fl = PrmtFlash();
    if (fl.dim1Axis == fl.displayXAxis) return {1, fl.dim1Len, fl.dim1Len, fl.dim2Len};
    return {fl.dim1Len, 1, fl.dim2Len, fl.dim1Len};
}

// Thenar_FeatureGradient @ 0x6bb3032e：沿边方向的 Sobel 梯度（y-1 一行减 y+1 一行）
int16_t Gradient(const int16_t* dif, int16_t x, int16_t y, const EdgeScan& g) {
    static constexpr int16_t kSobel[3][3] = {{1, 2, 1}, {0, 0, 0}, {-1, -2, -1}};  // [dy][dx]
    int16_t sum = 0;
    for (int16_t dx = 0; dx < 3; ++dx) {
        for (int16_t dy = 0; dy < 3; ++dy) {
            const auto xx = static_cast<int16_t>(x + dx - 1);
            const auto yy = static_cast<int16_t>(y + dy - 1);
            const auto idx = static_cast<uint16_t>(yy * g.yStep + g.xStep * xx);
            if (0 <= xx && xx < g.xLen && 0 <= yy && yy < g.yLen && idx < kDifSize) {
                sum = static_cast<int16_t>(sum + dif[idx] * kSobel[dy][dx]);
            }
        }
    }
    return sum;
}

// Thenar_FeatureUpdateEdgeWidthX @ 0x6bb30495：从 (x, y) 沿下标 ±xStep 两个方向数超过触摸门限的格
// （按下标走，到行尾会折进相邻一行）
uint16_t EdgeWidth(const int16_t* dif, int16_t x, int16_t y, const EdgeScan& g) {
    const int16_t thold = Prpt().touchThold;
    const auto at = static_cast<int16_t>(g.yStep * y + x * g.xStep);
    int16_t width = 0;
    auto c = at;
    for (int16_t e = 0; e < g.xLen; ++e) {
        c = static_cast<int16_t>(c + g.xStep);
        if (kDifSize <= c || dif[c] <= thold) break;
        ++width;
    }
    c = at;
    for (int16_t e = 0; e < g.xLen; ++e) {
        c = static_cast<int16_t>(c - g.xStep);
        if (c < 0 || dif[c] <= thold) break;
        ++width;
    }
    return static_cast<uint16_t>(width);
}

// Thenar_FeatureExtractionUnit @ 0x6bb306a0
// 沿一条侧边从远端往回扫，信号超过门限时开一段；段内梯度先跌破 -1000 再升过 1000 时另起一段。
// 段的长度 / 宽度 / 峰值在每多一格时更新（一格就结束的段只记起点）。
void ExtractEdge(const int16_t* dif, int16_t x, const EdgeScan& g) {
    enum class State { kOutside, kRising, kFalling };
    ThenarInfo& info = Touches().thenar;
    auto idx = static_cast<uint16_t>(g.yStep * (g.yLen - 1) + x * g.xStep);
    uint8_t n = info.pointCount;
    if (kDifSize <= idx || kMaxPoints <= n) return;
    const auto thold = static_cast<int16_t>(PrmtDynamic().sideNoiseThold / 2);
    const uint8_t onFirstEdge = x != g.xLen - 1;
    State state = State::kOutside;
    int16_t length = 0;
    uint16_t width = 0;
    int16_t peak = 0;
    const auto update = [&](uint16_t w) {
        ThenarPoint& p = info.point[n - 1];
        p.length = static_cast<uint16_t>(length);
        p.edgeWidth = w;
        p.peak = peak;
    };
    const auto close = [&] {
        state = State::kOutside;
        update(width);
        length = 0;
        width = 0;
        peak = 0;
    };
    const auto open = [&](int16_t y, int16_t v) {
        state = State::kRising;
        ++n;
        length = 1;
        width = EdgeWidth(dif, x, y, g);
        peak = v;
        info.point[n - 1].start = static_cast<uint16_t>(y);
        info.point[n - 1].onFirstEdge = onFirstEdge;
    };
    const auto extend = [&](int16_t y, int16_t v) {
        ++length;
        width = std::max(EdgeWidth(dif, x, y, g), width);
        peak = std::max(peak, v);
        update(width);
    };
    for (int16_t y = static_cast<int16_t>(g.yLen - 1); 0 <= y; --y, idx = static_cast<uint16_t>(idx - g.yStep)) {
        if (kMaxPoints <= n) {
            --n;
            break;
        }
        const int16_t v = dif[idx];
        switch (state) {
            case State::kOutside:
                if (thold < v) open(y, v);
                break;
            case State::kRising:
                if (v < thold) {
                    close();
                } else {
                    extend(y, v);
                    state = Gradient(dif, x, y, g) < kFallingGradient ? State::kFalling : State::kRising;
                }
                break;
            case State::kFalling:
                if (v < thold) {
                    close();
                } else if (Gradient(dif, x, y, g) <= kRisingGradient) {
                    extend(y, v);
                } else {
                    update(width);
                    open(y, v);
                }
                break;
        }
    }
    info.pointCount = n;
}

// Thenar_FeatureExtraction @ 0x6bb30c47（含 Touch_ThenarFeatureReset）：扫两条侧边
void ExtractFeatures() {
    ThenarInfo& info = Touches().thenar;
    info.pointCount = 0;
    ThenarPoint& p0 = info.point[0];
    p0.peak = 0;
    p0.edgeWidth = 0;
    p0.start = 0;
    p0.length = 0;
    const EdgeScan g = Scan();
    const int16_t* dif = Buffers().dif;
    for (int16_t x = 0; x < g.xLen; x = static_cast<int16_t>(x + g.xLen - 1)) ExtractEdge(dif, x, g);
}

/// 一条侧边上够强的段的统计。
struct EdgeFeature {
    uint16_t maxLength = 0;
    uint16_t maxIdx = 0;
    uint8_t shortCount = 0;
    uint16_t minGap = 0xffff;    // 之后各段起点与第一段末端的最小间距
    uint16_t firstIdx = 0xffff;  // 第一段
};

// Thenar_UpdateDetectFeature @ 0x6bb30f99：只看峰值不低于侧边噪声门限 5/6 的段
void DetectFeatures(EdgeFeature& first, EdgeFeature& last) {
    const ThenarInfo& info = Touches().thenar;
    const auto floor = static_cast<int16_t>(PrmtDynamic().sideNoiseThold * 5 / 6);
    for (uint16_t i = 0; i < info.pointCount; ++i) {
        const ThenarPoint& p = info.point[i];
        if (p.peak < floor) continue;
        EdgeFeature& f = p.onFirstEdge != 0 ? first : last;
        if (f.maxLength < p.length) {
            f.maxLength = p.length;
            f.maxIdx = i;
        }
        if (p.length < kShortLength) ++f.shortCount;
        if (f.firstIdx == 0xffff) {
            f.firstIdx = i;
        } else {
            const ThenarPoint& head = info.point[f.firstIdx];
            const auto gap = static_cast<uint16_t>(static_cast<int16_t>(head.start) - static_cast<int16_t>(head.length) -
                                                   static_cast<int16_t>(p.start));
            f.minGap = std::min(f.minGap, gap);
        }
    }
}

// Thenar_ProcessGensture @ 0x6bb2fefc
// 判出侧边、没有稳定报点的触点且信号够强时记为手掌手势；手势持续超过 800 ms 时确认，
// 确认在中断后 100 ms 内保持。
void ProcessGesture(int16_t peak) {
    TouchTable& tt = Touches();
    ThenarInfo& info = tt.thenar;
    const ThenarInfo& prev = PrevTouches().thenar;
    const uint64_t now = Static().curTimeMs;
    if (info.detected != 0 && (tt.stateFlags & kStateStableReported) == 0 &&
        static_cast<int16_t>(PrmtFlash().peakFilterReferenceThreshold) / 3 * 2 < peak) {
        info.gesture = 1;
    }
    if (info.gesture != 0) {
        if (prev.gesture == 0) {
            info.gestureStartTime = now;
        } else if (now - prev.gestureStartTime <= kGestureConfirmMs) {
            info.gestureStartTime = prev.gestureStartTime;
        } else {
            info.gestureConfirmed = 1;
        }
    }
    if (info.gestureConfirmed != 0) {
        info.gestureConfirmTime = now;
    } else if (prev.gestureConfirmed != 0 && now - prev.gestureConfirmTime < kGestureHoldMs) {
        info.gestureConfirmed = 1;
        info.gestureConfirmTime = prev.gestureConfirmTime;
    }
}

// Thenar_ProcessInfoByDifTraditional @ 0x6bb31392
// 一侧短段多、另一侧少，或一侧段间距小、另一侧大，或一侧最长段够大物体尺寸时判出手掌侧。
void Detect() {
    EdgeFeature first;
    EdgeFeature last;
    ExtractFeatures();
    DetectFeatures(first, last);
    const FlashPrmt& fl = PrmtFlash();
    const auto largeLength = static_cast<uint16_t>((fl.largeObjectSizeMm << 4) / fl.sensorPitchX16);
    uint32_t side = 0;
    if (3 <= first.shortCount && last.shortCount <= 2) {
        side = kSideRight;
    } else if (3 <= last.shortCount && first.shortCount <= 2) {
        side = kSideLeft;
    } else if (first.minGap < 4 && 4 < last.minGap) {
        side = kSideRight;
    } else if (last.minGap < 4 && 4 < first.minGap) {
        side = kSideLeft;
    } else if (last.maxLength < first.maxLength && largeLength <= first.maxLength) {
        side = kSideLeft;
    } else if (first.maxLength < last.maxLength && largeLength <= last.maxLength) {
        side = kSideRight;
    }
    ThenarInfo& info = Touches().thenar;
    if (side != 0) {
        info.detected = 1;
        info.side = side;
    }
    const EdgeFeature& longer = first.maxLength < last.maxLength ? last : first;
    ProcessGesture(info.point[longer.maxIdx].peak);
}

// Thenar_EventUpdateUnit @ 0x6bb30d38
void UpdateSideCount(uint32_t side, int self, int other) {
    uint32_t& enter = g_sideEnter[self];
    uint32_t& exit = g_sideExit[self];
    if (Touches().thenar.side == side) {
        if (enter < kEnterMax && kExitOther < g_sideExit[other]) enter += kEnterStep;
        exit = 0;
    } else {
        if (enter != 0 && enter < kEnterDecayMax) --enter;
        if (exit < kExitMax) ++exit;
    }
    if (kExitReset < exit) enter = 0;
}

// Thenar_EventUpdate @ 0x6bb30eb3
void UpdateEvents() {
    TouchTable& tt = Touches();
    UpdateSideCount(kSideLeft, 0, 1);
    if (kEnterReported < g_sideEnter[0]) {
        tt.extFlags |= kExtLeftGrip;
        if (tt.thenar.gestureConfirmed != 0) tt.extFlags |= kExtLeftPalm;
    }
    UpdateSideCount(kSideRight, 1, 0);
    if (kEnterReported < g_sideEnter[1]) {
        tt.extFlags |= kExtRightGrip;
        if (tt.thenar.gestureConfirmed != 0) tt.extFlags |= kExtRightPalm;
    }
}

}  // namespace

// Thenar_Init @ 0x6bb2fce0
void Init() {
    for (int i = 0; i < 2; ++i) {
        g_sideEnter[i] = 0xf;
        g_sideExit[i] = 0xf;
    }
}

// Thenar_ProcessInfoByDif @ 0x6bb315e8
void ProcessInfoByDif() {
    Detect();
    UpdateEvents();
    CopyInfo(PrevTouches().thenar, Touches().thenar);
}

// Thenar_UpdateExternalInfo @ 0x6bb2fe59
void UpdateExternalInfo() {
    const ThenarInfo& info = Touches().thenar;
    ThenarExternal().side = info.detected != 0 ? info.side : 0;
}

// Thenar_UnitCopy @ 0x6bb2fd40
void CopyInfo(ThenarInfo& dst, const ThenarInfo& src) {
    dst.detected = src.detected;
    dst.gesture = src.gesture;
    dst.gestureConfirmed = src.gestureConfirmed;
    dst.side = src.side;
    dst.gestureStartTime = src.gestureStartTime;
    std::copy(std::begin(src.point), std::end(src.point), std::begin(dst.point));
    dst.pointCount = src.pointCount;
}

}  // namespace Solvers::Tsa::Core::Thenar
