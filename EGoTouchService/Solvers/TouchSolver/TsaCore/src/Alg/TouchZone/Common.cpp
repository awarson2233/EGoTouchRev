// 区域属性与登记成触点（TZ_Common.c，以及质心 / 椭圆轴）。

#include <algorithm>
#include <cmath>
#include <numbers>

#include "Alg/CTD/Ctd.h"
#include "Alg/Cmf/Cmf.h"
#include "Alg/Peak/Peak.h"
#include "Alg/Peak/PeakData.h"
#include "Alg/Touch/TouchData.h"
#include "Alg/TouchSize/TouchSize.h"
#include "Alg/TouchThold/TouchThold.h"
#include "Alg/TouchZone/SubTouchZone.h"
#include "Alg/TouchZone/TouchZone.h"
#include "Alg/TouchZone/TouchZoneInternal.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::TouchZone {

namespace {

constexpr uint32_t kFeatStylus = 0x1000;
constexpr int kMaxRatio = 2000;
constexpr uint8_t kLargeGripRatio = 0x13;   // 握持比超过它算过宽
constexpr int kGapCellCount = 6;            // 外侧一列（行）未达门限的格数超过它才看信号间隙

const PeakEntry& PeakAt(uint8_t idx) { return Peaks().entry[idx]; }
TouchUnit& TouchAt(uint8_t idx) { return Touches().unit[idx]; }

// ── 质心与握持比 ─────────────────────────────────────────────────────────

// TZ_CentroidGripRatio @ 0x6ba7f2f0
// 边上信号和相对次边的比值（×16，无符号），封顶 0xffff；次边为 0 而边不为 0 时为 0xffff。
uint16_t EdgeToNearEdgeRatio(int32_t edge, int32_t nearEdge) {
    if (edge == 0 || nearEdge == 0) return edge != 0 && nearEdge == 0 ? 0xffff : 0;
    const uint32_t r = (static_cast<uint32_t>(edge) << 4) / static_cast<uint32_t>(nearEdge);
    return static_cast<uint16_t>(std::min<uint32_t>(r, 0xffff));
}

/// 外接框靠近屏幕的某个角（两个方向上离边的格数之和不到 2）。
bool NearCorner(const TouchZoneUnit& z) {
    const FlashPrmt& fl = PrmtFlash();
    const int cols = fl.dim1Len;
    const int rows = fl.dim2Len;
    return z.minDim2 + z.minDim1 < 2 || (rows - 1 - z.maxDim2) + z.minDim1 < 2 || z.minDim2 + (cols - 1 - z.maxDim1) < 2 ||
           (rows - z.maxDim2) + (cols - 2 - z.maxDim1) < 2;
}

/// 外接框贴到 sides 选中的屏幕边时，取对应方向的握持比中的最大值，并在 hitSides 里记下贴到的边。
/// sides / hitSides 的位：1、2 是 Dim2 的首末边，4、8 是 Dim1 的首末边（Dim1 沿长边时两者对调）。
struct SideRatio {
    bool selected;
    bool atEdge;
    uint32_t bit;
    uint16_t ratio;
};
uint8_t MaxSideRatio(const SideRatio (&s)[4], uint32_t* hitSides) {
    uint8_t res = 0;
    for (size_t i = 0; i < 4; ++i) {
        if (!s[i].selected || !s[i].atEdge) continue;
        if (s[i].ratio != 0) *hitSides |= s[i].bit;
        // 第一项只在比值非 0 时取，其余各项与已取的比较
        if (i == 0) {
            if (s[i].ratio != 0) res = static_cast<uint8_t>(s[i].ratio);
        } else if (res < s[i].ratio) {
            res = static_cast<uint8_t>(s[i].ratio);
        }
    }
    return res;
}

// TZ_CalcGripRatio @ 0x6ba7fa1a（Dim1OnShortAxis @ 0x6ba7f7d8、Dim1OnLongAxis @ 0x6ba7f685）
// 靠近屏幕角的区域在 Dim1 沿短边时四边都算。
uint8_t CalcGripRatio(uint32_t sides, const TouchZoneUnit& z, uint32_t* hitSides, uint16_t ratioDim1, uint16_t ratioDim2) {
    const FlashPrmt& fl = PrmtFlash();
    *hitSides = 0;
    if (fl.dim1OnLongAxis == 0) {
        const bool corner = NearCorner(z);
        const SideRatio s[4] = {{(sides & 4) != 0 || corner, z.minDim1 == 0, 4, ratioDim1},
                                {(sides & 8) != 0 || corner, z.maxDim1 == fl.dim1Len - 1, 8, ratioDim1},
                                {(sides & 1) != 0 || corner, z.minDim2 == 0, 1, ratioDim2},
                                {(sides & 2) != 0 || corner, z.maxDim2 == fl.dim2Len - 1, 2, ratioDim2}};
        return MaxSideRatio(s, hitSides);
    }
    const SideRatio s[4] = {{(sides & 4) != 0, z.minDim2 == 0, 4, ratioDim2},
                            {(sides & 8) != 0, z.maxDim2 == fl.dim2Len - 1, 8, ratioDim2},
                            {(sides & 1) != 0, z.minDim1 == 0, 1, ratioDim1},
                            {(sides & 2) != 0, z.maxDim1 == fl.dim1Len - 1, 2, ratioDim1}};
    return MaxSideRatio(s, hitSides);
}

// TZ_UpdateGripRatio @ 0x6ba7faa8
// field088：按手套过滤的边算的握持比；field058：边上最大信号不超过最强峰一半的方向不算，按左右（系统标志
// 0x80 时四边）算的握持比，贴到的边记在 field084。
void UpdateGripRatio(uint8_t idx, const TouchZoneUnit& z) {
    TouchUnit& t = TouchAt(idx);
    uint32_t hit = 0;
    t.field088 = CalcGripRatio(PrmtDynamic().gloveFilterSides, z, &hit, z.gripRatioDim1, z.gripRatioDim2);
    const int16_t halfPeak = static_cast<int16_t>(PeakAt(z.maxSigPeakIdx).sig / 2);
    const uint16_t r1 = z.edgeMaxSigDim1 <= halfPeak ? 0 : z.gripRatioDim1;
    const uint16_t r2 = z.edgeMaxSigDim2 <= halfPeak ? 0 : z.gripRatioDim2;
    const uint32_t sides = (Static().systemFlags & 0x80) != 0 ? 0xf : 0xc;
    t.field058 = CalcGripRatio(sides, z, &hit, r1, r2);
    t.field084 = hit;
}

// TZ_CentroidEdgeFlag @ 0x6ba7f3cb
// 外接框贴到逻辑边、质心离该边不到 1.5 格（Q8.8 下 [首边, 首边 + 2) 或 (末边 − 1, …]）时记边缘标志。
void CentroidEdgeFlag(const TouchZoneUnit& z, uint8_t idx) {
    const FlashPrmt& fl = PrmtFlash();
    uint32_t& flags = TouchAt(idx).edgeFlags;
    if (z.maxDim1 == fl.dim1LogicLast && (fl.dim1LogicLast - 1) * 0x100 < z.centroidDim1) flags |= 2;
    if (z.minDim1 == fl.dim1LogicFirst && z.centroidDim1 < (fl.dim1LogicFirst + 2) * 0x100) flags |= 1;
    if (z.maxDim2 == fl.dim2LogicLast && (fl.dim2LogicLast - 1) * 0x100 < z.centroidDim2) flags |= 8;
    if (z.minDim2 == fl.dim2LogicFirst && z.centroidDim2 < (fl.dim2LogicFirst + 2) * 0x100) flags |= 4;
}

// ── 登记成触点 ───────────────────────────────────────────────────────────

// TZ_StylusXYCorrection @ 0x6ba7fc89
// stylus 模式下按格内位置做正弦修正：frac + ⌊−14·sin(2π·frac / 255)⌋（向零取整）。
uint16_t StylusXyCorrection(uint16_t pos) {
    constexpr float kTwoPi = 2.0f * std::numbers::pi_v<float>;
    const int frac = pos & 0xff;
    const double s = std::sin(static_cast<double>((kTwoPi * static_cast<float>(frac)) / 255.0f));
    return static_cast<uint16_t>(frac + static_cast<int>(s * -14.0) + (pos >> 8) * 0x100);
}

// TZ_GetTzEdgeWidthDim1 @ 0x6ba7fd24 / Dim2 @ 0x6ba7fd66：贴边格的跨度，没有贴边格时为 0。
int EdgeSpan(int8_t lo, int8_t hi) {
    if (hi == INT8_MIN || lo == INT8_MAX) return 0;
    return static_cast<uint8_t>(hi) - static_cast<uint8_t>(lo) + 1;
}

// TZ_GetZValue @ 0x6ba7fe48
uint16_t ZValue(int32_t sigSum) {
    const uint32_t v = static_cast<uint32_t>(PrmtFlashConst().field000) * static_cast<uint32_t>(sigSum) >> 12;
    return static_cast<uint16_t>(std::clamp<uint32_t>(v, 1, 0xffff));
}

// TZ_UpdateTouchFlag @ 0x6ba7fe99
// 区域标志翻译成触点标志；几类异常形状还记到表头 stateFlags 与 TsaStatic::specialControlFlags。
void UpdateTouchFlag(const TouchZoneUnit& z, uint8_t idx) {
    TouchTable& touches = Touches();
    TouchUnit& t = touches.unit[idx];
    uint32_t& special = Static().specialControlFlags;
    const uint32_t f = z.flags;
    if ((f & kZoneOnBorder) == 0) t.volatileFlags |= 4;
    if ((f & kZoneNearBorder) == 0) t.volatileFlags2 |= 0x1000;
    if ((f & kZoneBrokenSensor) != 0) t.volatileFlags |= 0x10000000;
    if ((f & kZoneLargeShape) != 0) {
        t.volatileFlags |= 0x20000;
        touches.stateFlags |= 8;
    }
    if ((f & kZoneIsolated) != 0) {
        t.volatileFlags |= 0x2000000;
        touches.stateFlags |= 0x10;
        special |= 0x800;
    }
    if ((f & kZoneCmfWeak) != 0) {
        t.volatileFlags |= 0x1000000;
        special |= 0x20;
    }
    if ((f & kZoneCmfSideNoise) != 0) {
        t.volatileFlags |= 0x20000000;
        special |= 0x20;
    }
    if (z.shape == kShapeLargeObject) {
        t.volatileFlags |= 0x2000;
        special |= 0x800;
    }
    if ((f & kZoneEdgeMerged) != 0) t.volatileFlags2 |= 0x2000;
    if ((f & kZoneGripSeparated) != 0) t.persistFlags4 |= 1;
    if ((f & kZoneSeparateRegret) != 0) t.gripFlags |= 0x400;
    if ((f & kZoneInnerNonGripPeak) != 0 && (f & kZoneGripSeparated) == 0) t.gripFlags |= 0x80000000;
    if ((f & kZoneMultiFatFinger) != 0) t.persistFlags3 |= 2;
}

// TZ_UpdateInsideSensor @ 0x6ba8019d
// 质心所在格在区域核心（类别 6），或区域短轴超过 3 个传感器间距时记下该格，否则记为屏外。
void UpdateInsideSensor(const TouchZoneUnit& z, uint8_t* dim1, uint8_t* dim2) {
    const FlashPrmt& fl = PrmtFlash();
    const auto d1 = static_cast<uint8_t>(z.centroidDim1 >> 8);
    const auto d2 = static_cast<uint8_t>(z.centroidDim2 >> 8);
    if (CellKind(GridIndex(d1, d2)) == 6 || fl.sensorPitchX16 * 3u < z.minorMm * 16u) {
        *dim1 = d1;
        *dim2 = d2;
    } else {
        *dim1 = fl.dim1Len;
        *dim2 = fl.dim2Len;
    }
}

// TZ_GetId @ 0x6ba802e3：区域里第一个有 ID 的峰的 ID，没有时保持 cur。
uint8_t GetId(const TouchZoneUnit& z, uint8_t cur) {
    for (uint8_t k = 0; k < z.peakCount; ++k) {
        const uint8_t id = PeakAt(z.peakIdx[k]).id;
        if (id != 0) return id;
    }
    return cur;
}

// TZ_RegisterTouch @ 0x6ba8039b
// 区域的质心（夹在屏内）、外接框、椭圆、握持、形状与异常等写进当前帧触点表的下一个单元。
void RegisterTouch(TouchZoneUnit& z) {
    TouchTable& touches = Touches();
    if (touches.count >= kMaxTouchNum) return;
    const uint8_t idx = touches.count;
    const FlashConstPrmt& fc = PrmtFlashConst();
    if (fc.dim1LenQ8 - 1 < z.centroidDim1) z.centroidDim1 = static_cast<uint16_t>(fc.dim1LenQ8 - 1);
    if (fc.dim2LenQ8 - 1 < z.centroidDim2) z.centroidDim2 = static_cast<uint16_t>(fc.dim2LenQ8 - 1);
    TouchUnit& t = touches.unit[idx];
    t.timestamp = Static().curTimeMs;
    t.rawPos = {z.centroidDim1, z.centroidDim2};
    if ((Static().featureFlags & kFeatStylus) != 0) {
        t.rawPos = {StylusXyCorrection(z.centroidDim1), StylusXyCorrection(z.centroidDim2)};
    }
    t.rptPos = {z.centroidDim1, z.centroidDim2};
    t.pos = {z.centroidDim1, z.centroidDim2};
    t.field03E = z.minDim2;
    t.field03F = z.maxDim2;
    t.field03C = z.minDim1;
    t.field03D = z.maxDim1;
    t.dim1Span = static_cast<uint8_t>(z.maxDim1 - z.minDim1 + 1);
    t.dim2Span = static_cast<uint8_t>(z.maxDim2 - z.minDim2 + 1);
    t.field070 = z.edgeWidthDim1;
    t.field071 = z.edgeWidthDim2;
    t.field072 = z.edgeWidthSum;
    t.field06E = static_cast<int8_t>(EdgeSpan(z.edgeMinDim1, z.edgeMaxDim1));
    t.field06F = static_cast<int8_t>(EdgeSpan(z.edgeMinDim2, z.edgeMaxDim2));
    t.field0FE = z.cellCount;
    t.waterCells = z.waterCells;
    t.field068 = z.gripRatioRawDim1;
    t.field06A = z.gripRatioRawDim2;
    t.field078 = z.innerCornerSigRatio;
    t.field059 = z.gripRegion;
    t.maxMinSigOff = z.maxMinSigOff;
    t.field05C = z.field338;
    t.field056 = z.gripRatioDim1;
    t.field057 = z.gripRatioDim2;
    UpdateGripRatio(idx, z);
    t.curMajorMm = z.curMajorMm;
    t.majorMm = z.majorMm;
    t.minorMm = z.minorMm;
    t.majorPx = z.majorAxis;
    t.minorPx = z.minorAxis;
    t.angle = z.angle;
    t.z9 = z.ellipseRate;
    t.field064 = z.ellipseRateMirror;
    t.field066 = std::min(t.field064, t.z9);
    t.zoneShape = static_cast<int32_t>(z.shape);
    t.sigSum = z.sigSum;
    t.field090 = z.gripRegionSigSum;
    t.field098 = z.sigSumAll;
    t.field01C = ZValue(t.sigSum);
    t.field14A = static_cast<uint16_t>(z.sigThold);
    t.zoneSignalScale = z.maxSig;
    t.peakIdx = z.maxSigPeakIdx;
    t.sdcPeakIdx = z.sdcMaxPeakIdx;
    t.zoneEdgeSumDim1 = z.edgeSumDim1;
    t.zoneEdgeSumDim2 = z.edgeSumDim2;
    t.exceptionType = static_cast<int32_t>(z.exceptionType);
    t.field164 = z.field36D;
    CentroidEdgeFlag(z, idx);
    UpdateTouchFlag(z, idx);
    UpdateInsideSensor(z, &t.insideDim1, &t.insideDim2);
    t.field0A8 = 1;
    t.zoneIdx = TzAccess().tag;
    t.field05E = z.minTimeHand;
    t.id = GetId(z, t.id);
    ++touches.count;
    ++touches.reportedCount;
}

/// 子区域的质心、椭圆与信号写回区域，然后登记成触点。
void RegisterSubZone(TouchZoneUnit& z, const SubTouchZone::Unit& u) {
    z.sigSum = u.sigSum;
    z.cellCount = u.cellCount;
    z.angle = u.angle;
    z.majorMm = u.majorMm;
    z.minorMm = u.minorMm;
    z.majorAxis = u.majorAxis;
    z.minorAxis = u.minorAxis;
    z.centroidDim1 = u.centroidDim1;
    z.centroidDim2 = u.centroidDim2;
    z.gripRegion = u.gripRegion;
    if (u.peakIdx != kNoPeak) z.maxSigPeakIdx = u.peakIdx;
    RegisterTouch(z);
}

// TZ_RegisterBasedOnSubTZ @ 0x6ba8096a
// all 为 false 时只登记最强（Z9 最大）的子区域，否则登记 Filter 后仍有效的每个子区域。
void RegisterBasedOnSubZone(TouchZoneUnit& z, bool all) {
    const SubTouchZone::Table& t = SubTouchZone::Global();
    if (!all) {
        RegisterSubZone(z, SubTouchZone::UnitAt(t.maxZ9Idx));
        return;
    }
    for (uint8_t i = 0; i < t.count; ++i) {
        const SubTouchZone::Unit& u = SubTouchZone::UnitAt(i);
        if ((u.flags & (SubTouchZone::kWeakPeak | SubTouchZone::kWeakSum)) == 0) RegisterSubZone(z, u);
    }
}

// TZ_GripLevelProcess @ 0x6ba80e91：(dim1, dim2) 落在哪个子区域的外接框里，就取它的握持区域。
void GripLevelProcess(TouchZoneUnit& z, uint8_t d1, uint8_t d2) {
    const SubTouchZone::Table& t = SubTouchZone::Global();
    for (uint8_t i = 0; i < t.count; ++i) {
        const SubTouchZone::Unit& u = SubTouchZone::UnitAt(i);
        if (u.minDim1 <= d1 && d1 <= u.maxDim1 && u.minDim2 <= d2 && d2 <= u.maxDim2) z.gripRegion = u.gripRegion;
    }
}

// TZ_MFProcess @ 0x6ba80fbd
// 多指区域按峰拆成多个触点：格数与信号和平摊到各峰，椭圆取默认尺寸的圆，质心取峰周围 3×3。
// 区域其实是大手指、握持已拆分或悬浮大手指时，改按子区域登记。
void MfProcess(TouchZoneUnit& z) {
    if ((z.flags & kZoneMultiFatFinger) != 0 || (z.flags & kZoneGripSeparated) != 0 || z.fatFingerSD != 0) {
        RegisterBasedOnSubZone(z, true);
        return;
    }
    const uint8_t sizeMm = PrmtDynamic().defaultSizeMm;
    z.cellCount = static_cast<uint16_t>(z.cellCount / z.peakCount);
    z.sigSum = z.sigSum / z.peakCount;
    z.angle = 0;
    z.majorMm = sizeMm;
    z.minorMm = sizeMm;
    z.majorAxis = static_cast<uint16_t>(TouchSize::MmToPixel(sizeMm));
    z.minorAxis = z.majorAxis;
    for (uint8_t k = 0; k < z.peakCount; ++k) {
        const uint8_t peak = z.peakIdx[k];
        const PeakEntry& p = PeakAt(peak);
        Ctd::General(p.dim1, p.dim2, 3, 3, &z.centroidDim1, &z.centroidDim2);
        z.maxSigPeakIdx = peak;
        GripLevelProcess(z, p.dim1, p.dim2);
        RegisterTouch(z);
    }
}

// TZ_RegistForShapeFF @ 0x6ba811e3
// 大手指：Full Incell 下椭圆度差、尺寸达到大物体或悬浮大手指时按最强子区域登记，否则直接登记。
void RegisterFatFinger(TouchZoneUnit& z) {
    const FlashPrmt& fl = PrmtFlash();
    if ((fl.sensorArchitecture == 3 && 0xe < z.ellipseRate) || fl.largeObjectSizeMm <= z.majorMm || z.fatFingerSD != 0) {
        RegisterBasedOnSubZone(z, false);
    } else {
        RegisterTouch(z);
    }
}

// TZ_UnitToTouchNormal @ 0x6ba8126f
void UnitToTouchNormal(TouchZoneUnit& z) {
    switch (z.shape) {
        case kShapeMultiFinger: MfProcess(z); break;
        case kShapeFatFinger: RegisterFatFinger(z); break;
        default: RegisterTouch(z); break;
    }
}

// TZ_ExceptionProcess @ 0x6ba812c2
// 带区域异常时：0x2 / 0x8 照常处理（单独的 0x2 清掉）；0x80 / 0x100 / 0x200 / 0x8000 按最强子区域登记；
// 0x40 直接登记；其余不登记。
void ExceptionProcess(TouchZoneUnit& z) {
    if ((z.exceptionType & 0xa) != 0) {
        if (z.exceptionType == 2) z.exceptionType = 0;
        UnitToTouchNormal(z);
    } else if ((z.exceptionType & 0x8380) != 0) {
        RegisterBasedOnSubZone(z, false);
    } else if ((z.exceptionType & 0x40) != 0) {
        RegisterTouch(z);
    }
}

// ── 区域属性 ─────────────────────────────────────────────────────────────

// TZ_IsClearSignalGapAlongDim1 @ 0x6ba81663
// 沿 Dim1 从 dim1Lo 到 dim1Hi，比较 inner 行与 outer 行的 |Dif|：outer 处处不超过 inner 的 1/8，
// 且 inner 的起伏不超过其最大值的 1/4。
bool IsClearGapAlongDim1(uint8_t dim1Lo, uint8_t dim1Hi, uint8_t innerDim2, uint8_t outerDim2) {
    const int16_t* dif = Buffers().dif;
    int16_t hi = INT16_MIN;
    int16_t lo = INT16_MAX;
    for (uint8_t c = dim1Lo; c <= dim1Hi; ++c) {
        const int16_t inner = Abs16(dif[GridIndex(c, innerDim2)]);
        const int16_t outer = Abs16(dif[GridIndex(c, outerDim2)]);
        if (inner / 8 < outer) return false;
        hi = std::max(hi, inner);
        lo = std::min(lo, inner);
    }
    return hi - lo <= hi / 4;
}

// TZ_IsClearSignalGapAlongDim2 @ 0x6ba8181e
bool IsClearGapAlongDim2(uint8_t innerDim1, uint8_t outerDim1, uint8_t dim2Lo, uint8_t dim2Hi) {
    const int16_t* dif = Buffers().dif;
    int16_t hi = INT16_MIN;
    int16_t lo = INT16_MAX;
    for (uint8_t r = dim2Lo; r <= dim2Hi; ++r) {
        const int16_t inner = Abs16(dif[GridIndex(innerDim1, r)]);
        const int16_t outer = Abs16(dif[GridIndex(outerDim1, r)]);
        if (inner / 8 < outer) return false;
        hi = std::max(hi, inner);
        lo = std::min(lo, inner);
    }
    return hi - lo <= hi / 4;
}

// TZ_GetSurroundingStatus @ 0x6ba819d9
// 外接框在某个方向的两侧（屏外算作有间隙）都与外侧一列（行）之间有清晰的信号间隙。
// 外侧一列（行）要有足够多未达门限的格子才看。
bool IsIsolated(const TouchZoneUnit& z) {
    const FlashPrmt& fl = PrmtFlash();
    const int cols = fl.dim1Len;
    const int rows = fl.dim2Len;
    const int32_t* weak = LineWeakCount();
    const auto right = static_cast<uint8_t>(z.maxDim1 + 1);
    const auto left = static_cast<uint8_t>(z.minDim1 - 1);
    const auto below = static_cast<uint8_t>(z.maxDim2 + 1);
    const auto above = static_cast<uint8_t>(z.minDim2 - 1);
    auto rightGap = [&] { return kGapCellCount < weak[z.maxDim1 + 1] && IsClearGapAlongDim2(z.maxDim1, right, z.minDim2, z.maxDim2); };
    auto leftGap = [&] { return kGapCellCount < weak[z.minDim1 - 1] && IsClearGapAlongDim2(z.minDim1, left, z.minDim2, z.maxDim2); };
    auto belowGap = [&] { return kGapCellCount < weak[z.maxDim2 + cols + 1] && IsClearGapAlongDim1(z.minDim1, z.maxDim1, z.maxDim2, below); };
    auto aboveGap = [&] { return kGapCellCount < weak[z.minDim2 + cols - 1] && IsClearGapAlongDim1(z.minDim1, z.maxDim1, z.minDim2, above); };
    if (right < cols && rightGap() && (cols <= left || leftGap())) return true;
    if (left < cols && leftGap() && (cols <= right || rightGap())) return true;
    if (below < rows && belowGap() && (rows <= above || aboveGap())) return true;
    if (above < rows && aboveGap() && (rows <= below || belowGap())) return true;
    return false;
}

// TZ_GetAbsRatio @ 0x6ba823ad：|a·256 / b|，封顶 2000；b 为 0 时 2000。
int AbsRatio(int32_t a, int32_t b) {
    if (b == 0) return kMaxRatio;
    const int32_t r = static_cast<int32_t>(static_cast<uint32_t>(a) << 8) / b;
    return std::min(r < 0 ? -r : r, kMaxRatio);
}

// TZ_GetRatio @ 0x6ba823fc：max(a, 0)·256 / b，封顶 2000；b 为 0 时 2000。
int Ratio(int32_t a, int32_t b) {
    if (b == 0) return kMaxRatio;
    const int32_t r = static_cast<int32_t>(static_cast<uint32_t>(std::max(a, 0)) << 8) / b;
    return std::min(r, kMaxRatio);
}

}  // namespace

// TZ_GetAxisInternal @ 0x6ba7ec90
void AxisFromMoments(double m0, double m1Dim1, double m1Dim2, double m2Dim1Dim2, double m2Dim1, double m2Dim2,
                     uint16_t* majorAxis, uint16_t* minorAxis, int8_t* angle) {
    const double n = m0 == 0.0 ? 1.0 : m0;
    const double c1 = m1Dim1 / n;
    const double c2 = m1Dim2 / n;
    const double var1 = m2Dim1 / n - c1 * c1;
    const double var2 = m2Dim2 / n - c2 * c2;
    const double cov = m2Dim1Dim2 / n - c1 * c2;
    const double d = std::sqrt(cov * 4.0 * cov + (var1 - var2) * (var1 - var2));
    const double major = std::sqrt(std::max((var1 + var2 + d) * 6.0, 0.0));
    const double minor = std::sqrt(std::max(((var1 + var2) - d) * 6.0, 0.0));
    const double e = m2Dim1Dim2 * n - m1Dim1 * m1Dim2;
    const double theta = std::atan2(e + e, -(((m2Dim2 - m2Dim1) * n - m1Dim2 * m1Dim2) + m1Dim1 * m1Dim1));
    *majorAxis = major >= 0.5 ? static_cast<uint16_t>(static_cast<int>(major * 256.0)) : 0x80;
    *minorAxis = minor >= 0.5 ? static_cast<uint16_t>(static_cast<int>(minor * 256.0)) : 0x80;
    *angle = static_cast<int8_t>(static_cast<int>((theta * 57.29577957855229) / 2.0));
}

// TZ_GetAxis @ 0x6ba7f08d
void GetAxis(TouchZoneUnit& z) {
    AxisFromMoments(z.moment0, z.momentDim1, z.momentDim2, static_cast<double>(z.momentDim1Dim2), static_cast<double>(z.momentDim1Sq),
                    static_cast<double>(z.momentDim2Sq), &z.majorAxis, &z.minorAxis, &z.angle);
    z.majorMm = static_cast<uint8_t>(TouchSize::PixelToMm(z.majorAxis));
    z.minorMm = static_cast<uint8_t>(TouchSize::PixelToMm(z.minorAxis));
}

// TZ_CentroidCore @ 0x6ba7f363
void CentroidCore(TouchZoneUnit& z) {
    z.centroidDim1 = static_cast<uint16_t>(Ctd::BasicCalculation(z.centroidMomentDim1, z.centroidWeightDim1));
    z.centroidDim2 = static_cast<uint16_t>(Ctd::BasicCalculation(z.centroidMomentDim2, z.centroidWeightDim2));
}

// TZ_GetEdgeTouchedFlag @ 0x6ba81544
void GetEdgeTouchedFlag(TouchZoneUnit& z) {
    const FlashPrmt& fl = PrmtFlash();
    if (z.minDim1 == 0 || z.maxDim1 == fl.dim1Len - 1 || z.minDim2 == 0 || z.maxDim2 == fl.dim2Len - 1) z.flags |= kZoneOnBorder;
    if (z.minDim1 < 2 || fl.dim1Len - 2 <= z.maxDim1 || z.minDim2 < 2 || fl.dim2Len - 2 <= z.maxDim2) z.flags |= kZoneNearBorder;
}

// TZ_GetHTIValidFlag @ 0x6ba81626
void GetHtiValidFlag(TouchZoneUnit& z) {
    if (static_cast<uint16_t>(z.peakCount * 4) < z.cellCount) z.flags |= kZoneHtiValid;
}

// TZ_GetGripRatio @ 0x6ba81f2f
void GetGripRatio(TouchZoneUnit& z) {
    z.gripRatioRawDim1 = EdgeToNearEdgeRatio(z.edgeSumDim1, z.nearEdgeSumDim1);
    z.gripRatioDim1 = static_cast<uint8_t>(z.gripRatioRawDim1 > 0xfe ? 0xff : z.gripRatioRawDim1);
    z.gripRatioRawDim2 = EdgeToNearEdgeRatio(z.edgeSumDim2, z.nearEdgeSumDim2);
    z.gripRatioDim2 = static_cast<uint8_t>(z.gripRatioRawDim2 > 0xfe ? 0xff : z.gripRatioRawDim2);
}

// TZ_GetShapeRatio @ 0x6ba833cb：格数相对椭圆面积（按传感器间距换算成格）的比值（×16）。
void GetShapeRatio(TouchZoneUnit& z) {
    const FlashPrmt& fl = PrmtFlash();
    const uint32_t area = static_cast<uint32_t>(z.minorMm) * z.majorMm * 0x100 / fl.sensorPitchX16 / fl.sensorPitch2X16;
    z.shapeRatio = area == 0 ? 0xff : static_cast<uint8_t>((static_cast<uint64_t>(z.cellCount) << 4) / area);
}

// TZ_GetShapeFlag @ 0x6ba81fdb
// 最大子区域过长或过扁、贴边部分过宽，或握持比过大时置 kZoneLargeShape。
void GetShapeFlag(TouchZoneUnit& z) {
    const FlashPrmt& fl = PrmtFlash();
    const SubTouchZone::Unit& u = SubTouchZone::UnitAt(SubTouchZone::Global().maxCellIdx);
    const uint8_t largeMm = fl.largeObjectSizeMm;
    if ((largeMm * 16 - largeMm) / 10 < u.majorMm) z.flags |= kZoneLargeShape;
    if (u.minorMm * 3u < u.majorMm && 0xf < u.majorMm) z.flags |= kZoneLargeShape;
    const auto maxSpan = static_cast<uint16_t>((static_cast<uint32_t>(largeMm) << 4) / fl.sensorPitchX16 - 1);
    const auto span1 = static_cast<uint16_t>(EdgeSpan(z.edgeMinDim1, z.edgeMaxDim1));
    const auto span2 = static_cast<uint16_t>(EdgeSpan(z.edgeMinDim2, z.edgeMaxDim2));
    if (maxSpan <= span1) z.flags |= kZoneLargeShape;
    if (maxSpan <= span2) z.flags |= kZoneLargeShape;
    if (kLargeGripRatio < z.gripRatioDim1) z.flags |= kZoneLargeShape;
    if (kLargeGripRatio < z.gripRatioDim2) z.flags |= kZoneLargeShape;
}

// TZ_GetSurroundingStatusFlag @ 0x6ba81efd
void GetSurroundingStatusFlag(TouchZoneUnit& z) {
    if (IsIsolated(z)) z.flags |= kZoneIsolated;
}

// TZ_GetCMFFlag @ 0x6ba82694
// 最强峰加上其所在格的共模噪声仍不到峰门限（脚趾机型的边缘格用低门限）时置 kZoneCmfWeak；
// 区域孤立或过大时，再看外接框外侧（沿 CMF 方向）的共模噪声是否足以解释区域信号。
void GetCmfFlag(TouchZoneUnit& z) {
    const FlashPrmt& fl = PrmtFlash();
    const PeakEntry& p = PeakAt(z.maxSigPeakIdx);
    const int noise = Cmf::GetNoiseValue(p.dim1, p.dim2);
    const int total = noise + p.sig;
    int thold = PrmtDynamic().peakThold;
    if (TouchThold::ToeUseSpecialThold() && (p.dim1 == 1 || p.dim1 == fl.dim1Len - 2 || p.dim2 == fl.dim2Len - 1)) {
        thold = PrmtDynamic().edgePeakTholdLow;
    }
    if (total < thold && Cmf::GetMethod() != 1) z.flags |= kZoneCmfWeak;
    if ((z.flags & (kZoneIsolated | kZoneLargeShape)) == 0) return;
    int side[2];
    if (fl.dim1Axis == 0) {
        side[0] = Cmf::GetNoiseValue(static_cast<uint8_t>(z.minDim1 - 1), z.minDim2);
        side[1] = Cmf::GetNoiseValue(static_cast<uint8_t>(z.maxDim1 + 1), z.maxDim2);
    } else {
        side[0] = Cmf::GetNoiseValue(z.minDim1, static_cast<uint8_t>(z.minDim2 - 1));
        side[1] = Cmf::GetNoiseValue(z.maxDim1, static_cast<uint8_t>(z.maxDim2 + 1));
    }
    for (const int v : side) {
        if (fl.peakFilterReferenceThreshold < v * 2 && noise * 2 <= v && total < v * 2) z.flags |= kZoneCmfSideNoise;
    }
}

// TZ_GetMaxMinPkIdx @ 0x6ba82207
void GetMaxMinPkIdx(TouchZoneUnit& z) {
    int16_t minSig = INT16_MAX;
    int16_t maxSig = INT16_MIN;
    int32_t maxZ = INT32_MIN;
    for (uint8_t k = 0; k < z.peakCount; ++k) {
        const uint8_t idx = z.peakIdx[k];
        const PeakEntry& p = PeakAt(idx);
        if (p.sig < minSig) {
            z.minSigPeakIdx = idx;
            minSig = p.sig;
        }
        if (maxSig < p.sig) {
            z.maxSigPeakIdx = idx;
            maxSig = p.sig;
        }
        if (maxZ < p.z8 + p.sig) {
            z.maxZPeakIdx = idx;
            maxZ = p.z8 + p.sig;
        }
    }
    z.sdcMaxPeakIdx = z.maxSigPeakIdx;
}

// TZ_PeakRegister @ 0x6ba82339
void PeakRegister(TouchZoneUnit& z, uint8_t peak) {
    if (z.peakCount >= kMaxZonePeakNum) return;
    z.peakIdx[z.peakCount++] = peak;
    if (Peak::IsOnOuterEdge(peak)) z.flags |= kZoneOuterEdgePeak;
}

// TZ_Get2ClosestPeaks @ 0x6ba8146f：区域里距离最近的一对峰（并列时取先找到的）。
void Get2ClosestPeaks(const TouchZoneUnit& z, uint8_t& a, uint8_t& b) {
    uint8_t best = 0xff;
    for (int i = 0; i < z.peakCount - 1; ++i) {
        for (int j = i + 1; j < z.peakCount; ++j) {
            const uint8_t d = Peak::GetDistance(z.peakIdx[j], z.peakIdx[i]);
            if (d < best) {
                a = z.peakIdx[i];
                b = z.peakIdx[j];
                best = d;
            }
        }
    }
}

// TZ_UnitToTouch @ 0x6ba8134b
void UnitToTouch(TouchZoneUnit& z) {
    ++Touches().field002;
    z.curMajorMm = z.majorMm;
    if ((PrmtFlash().gripFlags & kGripRegisterDirect) != 0) {
        RegisterTouch(z);
    } else if (z.exceptionType == 0 || (z.flags & kZoneGripSeparated) != 0) {
        UnitToTouchNormal(z);
    } else {
        ExceptionProcess(z);
    }
}

// TZ_GetExceptionRatio @ 0x6ba8244c
// [0] 负信号和 / 正信号和，[1] 最小信号 / 最强峰值，[2] 最小信号 / 最强峰 Z8，[3] 正信号和 / 参考门限，
// [4] [5] 峰值 + Z8 最大的峰所在列、行的区域信号和 / 它的峰值。
void GetExceptionRatio(TouchZoneUnit& z) {
    const PeakEntry& strongest = PeakAt(z.maxSigPeakIdx);
    z.exceptionRatio[0] = static_cast<uint16_t>(AbsRatio(z.negSigSum, z.sigSum));
    z.exceptionRatio[1] = static_cast<uint16_t>(AbsRatio(z.minSig, strongest.sig));
    z.exceptionRatio[2] = static_cast<uint16_t>(AbsRatio(z.minSig, strongest.z8));
    z.exceptionRatio[3] = static_cast<uint16_t>(AbsRatio(z.sigSum, PrmtFlash().peakFilterReferenceThreshold));
    const PeakEntry& top = PeakAt(z.maxZPeakIdx);
    z.exceptionRatio[4] = static_cast<uint16_t>(Ratio(LineSigSum()[top.dim1], top.sig));
    z.exceptionRatio[5] = static_cast<uint16_t>(Ratio(LineSigSum()[top.dim2 + PrmtFlash().dim1Len], top.sig));
}

}  // namespace Solvers::Tsa::Core::TouchZone
