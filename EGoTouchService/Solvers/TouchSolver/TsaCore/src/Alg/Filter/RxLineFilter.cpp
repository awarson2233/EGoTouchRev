// 扫描线串扰过滤（原 Signal/RxLineFilter.c）：噪声较大时，同一扫描线上另有明显更强的峰的弱触点判为串扰去掉。

#include "Alg/Filter/RxLineFilter.h"

#include <cstdlib>

#include "Alg/Peak/PeakData.h"
#include "Alg/Touch/PrevTouch.h"
#include "Alg/Touch/Touch.h"
#include "Alg/Touch/TouchData.h"
#include "App/TsaStatic.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::RxLineFilter {

namespace {

constexpr uint32_t kPersistReported = 0x8;  // PrevTouchUnit::persistFlags

/// 峰所在的扫描线：dim1Axis 为 0 时是行，否则是列。
int8_t LineOf(const PeakEntry& pk) { return static_cast<int8_t>(PrmtFlash().dim1Axis == 0 ? pk.dim2 : pk.dim1); }

// RxLineFilter_IsZ8Failed @ 0x6ba60446
// 扫描线相近的其他触点里 Z8 最强者超过本触点 Z8（负值按 0）的 ratio/128 倍。
bool IsZ8Failed(uint8_t idx, uint8_t ratio) {
    const TouchTable& tt = Touches();
    const PeakTable& peaks = Peaks();
    const uint8_t own = tt.unit[idx].peakIdx;
    const int8_t line = LineOf(peaks.entry[own]);
    int32_t ownZ8 = peaks.entry[own].z8;
    int32_t best = ownZ8;
    uint8_t bestIdx = own;
    for (uint8_t t = 0; t < tt.count; ++t) {
        if (t == idx) continue;
        const uint8_t other = tt.unit[t].peakIdx;
        if (std::abs(line - LineOf(peaks.entry[other])) < PrmtFlash().rxLineMaxDist && best < peaks.entry[other].z8) {
            best = peaks.entry[other].z8;
            bestIdx = other;
        }
    }
    if (bestIdx == own) return false;
    if (ownZ8 < 0) ownZ8 = 0;
    return static_cast<uint32_t>(ownZ8 << 7) < static_cast<uint32_t>(best) * ratio;
}

// RxLineFilter_IsPossibleFalseTouch @ 0x6ba6069c（SS_TouchValid 被裁掉，恒为否）
// 串扰嫌疑的触点：重度噪声下、或尚无 ID、或已报过点但峰值减半个峰门限后低于 hiException.crosstalkPeakFloor。
bool IsPossibleFalseTouch(uint8_t idx, uint8_t ratio) {
    if (!IsZ8Failed(idx, ratio)) return false;
    bool r = Static().noiseLevel == 3;
    const TouchUnit& u = Touches().unit[idx];
    if (u.id == 0) return true;
    const uint8_t prevIdx = PrevTouch::GetIdxBasedOnId(u.id);
    if (prevIdx != kNoTouch && (PrevTouches().unit[prevIdx].persistFlags & kPersistReported) != 0 &&
        Peaks().entry[u.peakIdx].sig - PrmtDynamic().peakThold / 2 < Static().hiException.crosstalkPeakFloor) {
        r = true;
    }
    return r;
}

}  // namespace

// RxLineFilter_Process @ 0x6ba60827
void Process() {
    if (Static().noiseLevel < 2) return;
    const uint8_t ratio = PrmtFlash().rxLineZ8Ratio;
    for (auto k = static_cast<int8_t>(Touches().count - 1); k >= 0; --k) {
        if (IsPossibleFalseTouch(static_cast<uint8_t>(k), ratio)) Touch::UnitRemove(static_cast<uint8_t>(k));
    }
}

}  // namespace Solvers::Tsa::Core::RxLineFilter
