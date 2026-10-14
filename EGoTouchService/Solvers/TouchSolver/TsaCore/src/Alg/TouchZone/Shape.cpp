// 形状判定（TZ_Shape.c、TZ_DFS.c、TZ_ShapeDebounce.c）：大物体、手指、大手指、多指。

#include <algorithm>

#include "Alg/Debounce/Debounce.h"
#include "Alg/Peak/PeakData.h"
#include "Alg/Touch/PrevTouch.h"
#include "Alg/Touch/TouchData.h"
#include "Alg/TouchZone/SubTouchZone.h"
#include "Alg/TouchZone/TouchZone.h"
#include "Alg/TouchZone/TouchZoneInternal.h"
#include "App/TsaBuffer.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"
#include "Tool/PosnList/PosnSpecial.h"

namespace Solvers::Tsa::Core::TouchZone {

namespace {

constexpr uint16_t kLargeMajorMm = 0x14;    // 历史触点是大手指时，长轴达到它仍按大手指
constexpr uint32_t kLoExceptionMin = 9;     // 区域异常字不小于它时不判大物体
constexpr uint8_t kLoShapeRatio = 5;
constexpr uint16_t kStableAge = 5;          // 形状消抖：历史触点存在的帧数
constexpr uint8_t kMaxMfPeakCount = 4;
constexpr uint16_t kMfMaxCellCount = 10;
constexpr uint16_t kFingerMaxCellCount = 9;
constexpr uint32_t kFfException = 8;        // 区域异常字超过它时按大手指
constexpr int kCentralSensorKind = 6;       // 区域图里的核心格

const PeakEntry& PeakAt(uint8_t idx) { return Peaks().entry[idx]; }

// ── 两峰中点（TZ_DFS.c）──────────────────────────────────────────────────

// TZ_GetZ9 @ 0x6ba829c0：(dim1, dim2) 周围 3×3 的 Dif 和，有一格出屏时为 0。
int Z9At(uint8_t dim1, uint8_t dim2) {
    const FlashPrmt& fl = PrmtFlash();
    const int16_t* dif = Buffers().dif;
    int sum = 0;
    for (int r = dim2 - 1; r <= dim2 + 1; ++r) {
        if (r < 0 || r >= fl.dim2Len) return 0;
        for (int c = dim1 - 1; c <= dim1 + 1; ++c) {
            if (c < 0 || c >= fl.dim1Len) return 0;
            sum += dif[GridIndex(static_cast<uint8_t>(c), static_cast<uint8_t>(r))];
        }
    }
    return sum;
}

// TZ_GetCentInfo @ 0x6ba82be4
// 中点附近一格：计入 Dif 和与格数；周围 3×3 有信号时清 kZoneCentNoSignal 并更新 maxCentZ9。
void AddCentCell(TouchZoneUnit& z, uint8_t dim1, uint8_t dim2, uint8_t& n, int& sum) {
    const int z9 = Z9At(dim1, dim2);
    if (z9 != 0) {
        z.flags &= ~kZoneCentNoSignal;
        z.maxCentZ9 = std::max(z.maxCentZ9, z9);
    }
    sum += Buffers().dif[GridIndex(dim1, dim2)];
    ++n;
}

// TZ_GetAveCentSig @ 0x6ba82b1e
// 两峰中点的平均 Dif：坐标和为奇数的方向上，中点落在两格之间，取两格（两个方向都是则取 2×2）。
int16_t AveCentSig(TouchZoneUnit& z, uint8_t dim1, uint8_t dim2, uint8_t sumDim1, uint8_t sumDim2) {
    uint8_t n = 0;
    int sum = 0;
    const bool oddDim1 = (sumDim1 & 1) != 0;
    const bool oddDim2 = (sumDim2 & 1) != 0;
    AddCentCell(z, dim1, dim2, n, sum);
    if (oddDim1) AddCentCell(z, static_cast<uint8_t>(dim1 + 1), dim2, n, sum);
    if (oddDim2) AddCentCell(z, dim1, static_cast<uint8_t>(dim2 + 1), n, sum);
    if (oddDim1 && oddDim2) AddCentCell(z, static_cast<uint8_t>(dim1 + 1), static_cast<uint8_t>(dim2 + 1), n, sum);
    return static_cast<int16_t>(sum / n);
}

// TZ_UpdateAveCentSig @ 0x6ba82c8d
// 两峰同列（行）时改取两峰之间各格 Dif 的最小值（两峰相邻时为 0x7fff）。
int16_t UpdateAveCentSig(uint8_t dim1A, uint8_t dim2A, uint8_t dim1B, uint8_t dim2B, int16_t ave) {
    const int16_t* dif = Buffers().dif;
    const uint8_t cols = PrmtFlash().dim1Len;
    if (dim1A == dim1B) {
        const uint8_t lo = std::min(dim2A, dim2B);
        const uint8_t hi = std::max(dim2A, dim2B);
        int16_t res = INT16_MAX;
        for (uint8_t r = lo + 1; r < hi; ++r) res = std::min(res, dif[static_cast<uint16_t>(dim1A + r * cols)]);
        return res;
    }
    if (dim2A == dim2B) {
        const uint8_t lo = std::min(dim1A, dim1B);
        const uint8_t hi = std::max(dim1A, dim1B);
        int16_t res = INT16_MAX;
        for (uint8_t c = lo + 1; c < hi; ++c) res = std::min(res, dif[static_cast<uint16_t>(c + dim2A * cols)]);
        return res;
    }
    return ave;
}

// ── 形状消抖（TZ_ShapeDebounce.c）──────────────────────────────────────

/// 峰对应的历史触点（峰没有 ID 时为 kNoTouch）。有 ID 却找不到历史触点时记一次断言。
uint8_t PrevIdxOfPeak(uint8_t peak) {
    const uint8_t id = PeakAt(peak).id;
    if (id == 0) return kNoTouch;
    const uint8_t idx = PrevTouch::GetIdxBasedOnId(id);
    TsaAssert(idx != kNoTouch);
    return idx;
}

// TZ_GetPrevShape @ 0x6ba8a030：峰对应的历史触点报过点时它的区域形状，否则 kShapeNone。
uint32_t GetPrevShape(uint8_t peak) {
    const uint8_t idx = PrevIdxOfPeak(peak);
    if (idx >= kMaxTouchNum) return kShapeNone;
    const PrevTouchUnit& u = PrevTouches().unit[idx];
    return (u.persistFlags & 8) != 0 ? static_cast<uint32_t>(u.zoneShape) : kShapeNone;
}

// TZ_GetPrevAge @ 0x6ba8a186
uint16_t GetPrevAge(uint8_t peak) {
    const uint8_t idx = PrevIdxOfPeak(peak);
    return idx < kMaxTouchNum ? PrevTouches().unit[idx].age : 0;
}

// ── 判定 ─────────────────────────────────────────────────────────────────

const SubTouchZone::Unit& LargestSubZone() { return SubTouchZone::UnitAt(SubTouchZone::Global().maxCellIdx); }

// TZ_IsLO @ 0x6ba83474
// 大物体：区域无严重异常，椭圆面积（mm²，按区域或最大子区域）超过消抖的大物体门限，且最大子区域的长轴不超过
// 大物体尺寸或格数相对椭圆偏多。（SS 的大物体判定在本机型恒为否。）
bool IsLargeObject(const TouchZoneUnit& z) {
    uint16_t area;
    if ((PrmtFlash().gripFlags & kGripLoByZoneAxis) == 0) {
        const SubTouchZone::Unit& u = LargestSubZone();
        area = static_cast<uint16_t>(u.minorMm * u.majorMm);
    } else {
        area = static_cast<uint16_t>(z.minorMm * z.majorMm);
    }
    if (z.exceptionType >= kLoExceptionMin) return false;
    if (Debounce::LargeObjectSize() >= area) return false;
    return LargestSubZone().majorMm <= PrmtFlash().largeObjectSizeMm || kLoShapeRatio < z.shapeRatio;
}

bool IsAbsFfMinorAxis(uint8_t minorMm) { return PrmtFlash().absFfMinorMm < minorMm; }

// TZ_IsAbsFatFinger @ 0x6ba8ceaf：短轴、两峰中点信号比或中点 Z9 任一超过绝对大手指门限。
bool IsAbsFatFinger(const TouchZoneUnit& z) {
    const FlashPrmt& fl = PrmtFlash();
    return IsAbsFfMinorAxis(static_cast<uint8_t>(z.minorMm)) || fl.absFfCentToMinPkRatio < z.centToMinPkRatio ||
           fl.absFfZ9 < z.maxCentZ9;
}

// TZ_GetShapeBaseOnCentToMinPkRatio @ 0x6ba8cf22
// 中点下凹明显（比值小）为多指；带区域异常时门限减半。
uint32_t ShapeByCentToMinPkRatio(const TouchZoneUnit& z) {
    const FlashPrmt& fl = PrmtFlash();
    const uint8_t r = z.centToMinPkRatio;
    if (z.exceptionType != 0) return r < (fl.mfCentToMinPkRatio >> 1) ? kShapeMultiFinger : kShapeFatFinger;
    if (fl.absFfCentToMinPkRatio < r) return kShapeFatFinger;
    if (r < fl.mfCentToMinPkRatioLow || r < fl.mfCentToMinPkRatio) return kShapeMultiFinger;
    return kShapeFatFinger;
}

// TZ_JudgeFFOrMF @ 0x6ba8d12d
// 多峰区域：严重异常、短轴过大、质心落在核心格、或长轴不大且不贴边时为大手指，否则按两峰中点信号比判定。
void JudgeFfOrMf(TouchZoneUnit& z) {
    const FlashPrmt& fl = PrmtFlash();
    GetDfsPrpt(z);
    uint32_t shape = kShapeNone;
    if (kFfException < z.exceptionType) shape = kShapeFatFinger;
    // TZ_GetShapeBaseOnMinorAxis @ 0x6ba8cfd5
    if (shape == kShapeNone && fl.absFfMinorMm < z.minorMm) shape = kShapeFatFinger;
    // TZ_GetShapeBaseOnCentralSensor @ 0x6ba8d053
    if (shape == kShapeNone &&
        CellKind(GridIndex(static_cast<uint8_t>(z.centroidDim1 >> 8), static_cast<uint8_t>(z.centroidDim2 >> 8))) == kCentralSensorKind) {
        shape = kShapeFatFinger;
    }
    // TZ_GetShapeBaseOnMajorAxis @ 0x6ba8d011
    if (shape == kShapeNone && z.majorMm < fl.ffMaxMajorMm && (z.flags & kZoneOnBorder) == 0) shape = kShapeFatFinger;
    if (shape == kShapeNone) shape = ShapeByCentToMinPkRatio(z);
    z.shape = shape;
}

// TZ_DFSProcess @ 0x6ba832d9
// 全频噪声或最强峰不到手指门限时为大手指；否则先按峰对应的历史形状消抖，消抖不出结论再按几何判定。
void DfsProcess(TouchZoneUnit& z) {
    if (TSAStatic_IsAllFreqNoisy() != 0) {
        z.shape = kShapeFatFinger;
        return;
    }
    if (PrevTouches().count < Peaks().count) PeakFilter(z);
    if (PeakAt(z.maxSigPeakIdx).sig < PrmtFlash().fingerThold) {
        z.shape = kShapeFatFinger;
        return;
    }
    const uint32_t db = GetDbShape(z);
    if (db == kShapeNone) {
        JudgeFfOrMf(z);
    } else {
        z.shape = IsAbsFatFinger(z) ? kShapeFatFinger : db;
    }
}

// TZ_MultiFatFingerCheck @ 0x6ba836a1
// 多指区域其实是一个大手指（kZoneMultiFatFinger）：覆盖到的历史触点带标志 2 且不在握持，最大子区域足够大且
// 拆出多个有效子区域，或覆盖到的历史触点是大手指且又覆盖了别的触点、峰太多、长轴比历史触点长出一半以上。
void MultiFatFingerCheck(TouchZoneUnit& z) {
    if (z.shape != kShapeMultiFinger) return;
    const uint8_t prevIdx = PrevTouch::GetIdxBasedOnId(z.mergedId);
    if ((z.flags & kZonePrevTouchFlag2) != 0 && prevIdx != kNoTouch &&
        static_cast<uint8_t>(GripFilter_IsGrip(PrevTouches().unit[prevIdx].gripStage)) == 0) {
        z.flags |= kZoneMultiFatFinger;
    }
    if (kMfMaxCellCount < SubTouchZone::Global().maxCellCount && 1 < SubTouchZone::GetNumAftFilter()) z.flags |= kZoneMultiFatFinger;
    if ((z.flags & kZoneMergedFatFinger) == 0) return;
    if ((z.flags & kZoneIdMerged) != 0) z.flags |= kZoneMultiFatFinger;
    if (kMaxMfPeakCount < z.peakCount ||
        (prevIdx != kNoTouch && PrevTouches().unit[prevIdx].majorMm * 3u < z.majorMm * 2u)) {
        z.flags |= kZoneMultiFatFinger;
    }
}

/// 多峰区域的形状：坏传感器、握持拆分、强制拆分、覆盖两个历史触点、历史大手指、虚拟峰、正负信号交替、
/// 子区域明显分开，依次判定，都不是时走 DFS。
uint32_t MultiPeakShape(TouchZoneUnit& z) {
    const uint32_t f = z.flags;
    if ((f & kZoneBrokenSensor) != 0) return kShapeFatFinger;
    if ((f & kZoneGripSeparated) != 0) return kShapeMultiFinger;
    if ((f & kZoneForceSeparate) != 0) return kShapeFatFinger;
    if ((f & kZoneIdMerged) != 0 && (f & kZoneOuterEdgePeak) == 0 && !IsAbsFfMinorAxis(static_cast<uint8_t>(z.minorMm))) {
        return kShapeMultiFinger;
    }
    if (kLargeMajorMm <= z.majorMm && z.prevIdx != kNoTouch &&
        PrevTouches().unit[z.prevIdx].zoneShape == static_cast<int32_t>(kShapeFatFinger) && (f & kZoneIdMerged) == 0 &&
        z.maxSig * 3 < PrmtDynamic().touchThold * 2) {
        return kShapeFatFinger;
    }
    if ((f & kZoneVirtualPeak) != 0) return kShapeMultiFinger;
    if (2 <= PosnSpecial::GetPosMarker() && PeakAt(z.maxSigPeakIdx).sig / 2 < PosnSpecial::Global().zone[2].maxSig) {
        return kShapeMultiFinger;
    }
    if (SubTouchZone::IsMultiFinger()) return kShapeMultiFinger;
    DfsProcess(z);
    return z.shape;
}

}  // namespace

// TZ_GetAutoCentToMinPkRatio @ 0x6ba82e3c
uint8_t CentToMinPkRatio(TouchZoneUnit& z, uint8_t peakA, uint8_t peakB) {
    if (peakA == kNoPeak || peakB == kNoPeak) {
        TsaAssert(false);
        return 0;
    }
    if (peakA == peakB) return 0;
    const PeakEntry& a = PeakAt(peakA);
    const PeakEntry& b = PeakAt(peakB);
    const auto sumDim1 = static_cast<uint8_t>(b.dim1 + a.dim1);
    const auto sumDim2 = static_cast<uint8_t>(b.dim2 + a.dim2);
    z.flags |= kZoneCentNoSignal;
    z.maxCentZ9 = 0;
    int16_t cent = AveCentSig(z, sumDim1 >> 1, sumDim2 >> 1, sumDim1, sumDim2);
    cent = UpdateAveCentSig(a.dim1, a.dim2, b.dim1, b.dim2, cent);
    const int16_t minSig = std::min(a.sig, b.sig);
    if (cent < minSig && minSig != 0) return static_cast<uint8_t>((cent << 6) / minSig);
    return 0x40;
}

// TZ_GetDFSPrpt @ 0x6ba830c5
// 每峰平均信号相对触点门限（HTI 有效时相对最强峰）的比值、参考信号 × 椭圆面积相对信号和的比值，
// 以及最近两峰的中点信号比。
void GetDfsPrpt(TouchZoneUnit& z) {
    int32_t avg = z.peakCount != 0 ? z.sigSum / z.peakCount : 0;
    avg = static_cast<int32_t>(static_cast<uint32_t>(avg) << 4);
    int16_t ref;
    if ((z.flags & kZoneHtiValid) == 0) {
        ref = PrmtDynamic().touchThold;
        avg /= ref;
    } else {
        ref = PeakAt(z.maxSigPeakIdx).sig;
        if (ref != 0) avg /= ref;
    }
    int32_t area = static_cast<int32_t>(static_cast<uint32_t>(ref) * z.minorMm * z.majorMm);
    if (z.sigSum != 0) area /= z.sigSum;
    z.avgSigRatio = static_cast<uint8_t>(avg < 0x100 ? avg : 0xff);
    z.areaToSigRatio = area < 0x100 ? area : 0xff;
    uint8_t a = kNoPeak;
    uint8_t b = kNoPeak;
    Get2ClosestPeaks(z, a, b);
    z.centToMinPkRatio = CentToMinPkRatio(z, a, b);
}

// TZ_GetDBShape @ 0x6ba8a24f
// 形状消抖：各峰都对应已报点的手指且峰数等于历史触点数，或只有手指与多指且历史够久时为多指；
// 只有一个峰对应已报点的手指或大手指（其余峰是新的）且历史够久时为大手指；否则不定。
uint32_t GetDbShape(const TouchZoneUnit& z) {
    uint8_t none = 0;
    uint8_t finger = 0;
    uint8_t multi = 0;
    uint8_t fat = 0;
    uint16_t maxAge = 0;
    for (uint8_t k = 0; k < z.peakCount; ++k) {
        const uint8_t peak = z.peakIdx[k];
        const uint32_t shape = GetPrevShape(peak);
        maxAge = std::max(maxAge, GetPrevAge(peak));
        switch (shape) {
            case kShapeNone: ++none; break;
            case kShapeFinger: ++finger; break;
            case kShapeMultiFinger: ++multi; break;
            case kShapeFatFinger: ++fat; break;
            default: break;
        }
    }
    if (z.peakCount == finger && z.peakCount == PrevTouches().count) return kShapeMultiFinger;
    if (multi != 0 && finger + multi == z.peakCount && kStableAge <= maxAge) return kShapeMultiFinger;
    if (none + 1 == z.peakCount && fat + finger == 1 && kStableAge <= maxAge) return kShapeFatFinger;
    return kShapeNone;
}

// TZ_GetType @ 0x6ba837f8
// 单峰区域按历史形状消抖（没有历史时按最大子区域的格数分手指 / 大手指），多峰区域见 MultiPeakShape。
void GetType(TouchZoneUnit& z) {
    if (IsLargeObject(z)) {
        z.shape = kShapeLargeObject;
        return;
    }
    if ((PrmtFlash().gripFlags & kGripForceFinger) != 0 || ((z.flags & kZoneSeparateRegret) != 0 && (z.flags & kZoneIdMerged) == 0)) {
        z.shape = kShapeFinger;
        return;
    }
    if (z.peakCount < 2) {
        const uint32_t prev = GetPrevShape(z.peakIdx[0]);
        if (prev == kShapeFatFinger || (z.flags & kZonePrevTouchFlag2) != 0) {
            z.shape = kShapeFatFinger;
        } else if (prev == kShapeMultiFinger) {
            z.shape = kShapeMultiFinger;
        } else {
            // TZ_JudgeFFOrNF @ 0x6ba8d1ed
            z.shape = kFingerMaxCellCount < SubTouchZone::Global().maxCellCount ? kShapeFatFinger : kShapeFinger;
        }
    } else {
        z.shape = MultiPeakShape(z);
    }
    MultiFatFingerCheck(z);
}

}  // namespace Solvers::Tsa::Core::TouchZone
