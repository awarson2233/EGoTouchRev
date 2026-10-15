#include "Alg/TouchZone/SubTouchZone.h"

#include <algorithm>
#include <array>

#include "Alg/CTD/Ctd.h"
#include "Alg/Peak/Peak.h"
#include "Alg/Peak/PeakData.h"
#include "Alg/Touch/TouchData.h"
#include "Alg/TouchSize/TouchSize.h"
#include "Alg/TouchZone/TouchZone.h"
#include "Alg/TouchZone/TouchZoneData.h"
#include "App/Sys.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Prmt/Prmt.h"
#include "Tool/PosnList/PosnSpecial.h"
#include "Rejection/GripFilter/GripFilter.h"

namespace Solvers::Tsa::Core::SubTouchZone {

namespace {

constexpr uint8_t kNoPeak = kMaxPeakNum;
constexpr uint16_t kQueueSize = 100;    // 遍历队列的格数
constexpr uint8_t kTagStep = 8;         // 每个区域在区域图里占的编码数
constexpr uint32_t kExcFloating = 0x08;            // 区域异常字：悬浮
constexpr uint32_t kExcIgnoreZoneBound = 0xC8;     // 带这几类异常（悬浮 0x08、0x40、湿手指 0x80）时遍历不限于本 TZ 区域
constexpr uint8_t kGripStage = 1;                  // gripStage 缓冲里的「握持」
constexpr uint16_t kGripMajorMm = 19;              // 贴左右边、长轴超过它的子区域按握持算
constexpr uint16_t kMfCellCount = 10;              // 与上一帧触点数相同时，最大子区域超过这么多格即判多指

struct Offset {
    int8_t dim1;
    int8_t dim2;
};
/// 自身在前，其后是 8 邻域：从右上起顺时针。
constexpr std::array<Offset, 9> kNeighbors = {{{0, 0}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}}};

/// g_tsaBufTemp：遍历读信号的缓冲，每次遍历前指向 Dif。
int16_t*& SigBuf() { return Image::At<int16_t*>(0x6bc2e788); }

// SubTZ_UnitInit @ 0x6ba7d2e0
void InitUnit(Unit& u) {
    u = Unit{};
    u.maxSig = INT16_MIN;
    u.minDim1 = 0xff;
    u.minDim2 = 0xff;
}

// SubTZ_UpdateUnit @ 0x6ba7d3cd
// 把一个格子按信号加权计入各阶矩，并扩展外接框。
void Accumulate(Unit& u, int16_t sig, uint8_t dim1, uint8_t dim2) {
    const int32_t w1 = sig * dim1;
    u.sigSum += sig;
    u.momentDim1 += w1;
    u.momentDim1Dim2 += dim2 * w1;
    u.momentDim1Sq += dim1 * w1;
    u.momentDim2 += sig * dim2;
    u.momentDim2Sq += dim2 * sig * dim2;
    u.minDim1 = std::min(u.minDim1, dim1);
    u.maxDim1 = std::max(u.maxDim1, dim1);
    u.minDim2 = std::min(u.minDim2, dim2);
    u.maxDim2 = std::max(u.maxDim2, dim2);
}

// SubTZ_IsInCurrTZ @ 0x6ba7d35d
// 区域图里该格的编码落在本 TZ 区域的 8 个编码之内。
bool IsInZone(const TouchZoneUnit& zone, uint16_t cell) {
    const int code = Buffers().tz[cell];
    return zone.mapTag <= code && code < zone.mapTag + kTagStep;
}

// SubTZ_SetExtraFlag @ 0x6ba7d4fe
// 互容主流程中：信号超过峰检测门限 3/4 的格子在 tzZone 里记为当前 TZ 区域，并记下覆盖到的最大握持区域。
void MarkExtra(uint16_t cell, int16_t sig, Unit& u) {
    if ((Static().processingState & kProcMsActive) == 0) return;
    TsaBufferTable& buf = Buffers();
    if (PrmtDynamic().sigThold * 3 < sig * 4) buf.tzZone[cell] = TzAccess().tag;
    u.gripRegion = std::max(u.gripRegion, buf.gripRegion[cell]);
}

/// 遍历标记：当前句柄的编码 + 1，带 Dif 的符号。
int8_t VisitMark(uint16_t cell) {
    const int sign = Buffers().dif[cell] < 0 ? -1 : 1;
    return static_cast<int8_t>((CurrentAccess()->tag + 1) * sign);
}

/// 从 (dim1, dim2) 出发做区域生长：每取出一个格子，对它自身和 8 邻域中在屏内且未访问的格子调用
/// visit(cell, dim1, dim2, isSelf)；visit 决定是否计入子区域、是否继续扩展。
template <class Visit>
void Grow(Unit& u, uint8_t dim1, uint8_t dim2, Visit visit) {
    std::array<uint16_t, kQueueSize> queue{};
    PosnSpecial::Init(queue.data(), kQueueSize);
    InitUnit(u);
    PosnSpecial::Put(GridIndex(dim1, dim2));
    const FlashPrmt& fl = PrmtFlash();
    while (!PosnSpecial::IsEmpty()) {
        const uint16_t from = PosnSpecial::Get();
        const auto fromDim1 = static_cast<uint8_t>(from % fl.dim1Len);
        const auto fromDim2 = static_cast<uint8_t>(from / fl.dim1Len);
        const int8_t* map = SubTzAccess().map;
        for (size_t k = 0; k < kNeighbors.size(); ++k) {
            const auto d1 = static_cast<uint8_t>(fromDim1 + kNeighbors[k].dim1);
            const auto d2 = static_cast<uint8_t>(fromDim2 + kNeighbors[k].dim2);
            if (d1 >= fl.dim1Len || d2 >= fl.dim2Len) continue;
            const uint16_t cell = GridIndex(d1, d2);
            if (map[cell] == 0) visit(cell, d1, d2, k == 0);
        }
    }
}

// SubTZ_PeakBasedTraversal @ 0x6ba7d838 / SubTZ_TraversalNeighbors @ 0x6ba7d5a5
// 按信号门限生长：达到门限的格子计入并继续扩展；不到门限的正信号格子只在悬浮相关的情况下计入矩。
// 一般只在本 TZ 区域内走，悬浮大手指或带 kExcIgnoreZoneBound 异常时不限区域。
void GrowByPeak(const TouchZoneUnit& zone, Unit& u, int16_t thold, uint8_t dim1, uint8_t dim2) {
    Grow(u, dim1, dim2, [&](uint16_t cell, uint8_t d1, uint8_t d2, bool isSelf) {
        if (zone.fatFingerSD == 0 && (zone.exceptionType & kExcIgnoreZoneBound) == 0 && !IsInZone(zone, cell)) return;
        const int16_t sig = SigBuf()[cell];
        MarkExtra(cell, sig, u);
        if (sig < thold) {
            if ((zone.fatFingerSD != 0 || (zone.exceptionType & kExcFloating) != 0) && sig > 0) Accumulate(u, sig, d1, d2);
        } else {
            ++u.cellCount;
            u.maxSig = std::max(u.maxSig, sig);
            Accumulate(u, sig, d1, d2);
            if (!isSelf) PosnSpecial::Put(cell);
        }
        SubTzAccess().map[cell] = VisitMark(cell);
    });
}

// SubTZ_GripBasedTraversal @ 0x6ba7e51f / SubTZ_TraversalGripNeighbors @ 0x6ba7e28f
// 按握持阶段生长。种子是绝对握持峰时，走所有握持格；否则只走本 TZ 区域里的非握持格，碰到握持格时
// 交给 TZ 尝试把它并入峰列表。走到的格子都计入并继续扩展，正信号的计入矩。
void GrowByGrip(TouchZoneUnit& zone, Unit& u, bool absGrip, uint8_t dim1, uint8_t dim2) {
    Grow(u, dim1, dim2, [&](uint16_t cell, uint8_t d1, uint8_t d2, bool isSelf) {
        const TsaBufferTable& buf = Buffers();
        if (absGrip) {
            if (buf.gripStage[cell] != kGripStage) return;
        } else {
            if (buf.tzZone[cell] != zone.mapTag) return;
            if (buf.gripStage[cell] == kGripStage) {
                TouchZone::TryExtraMergePeak(zone, d1, d2);
                return;
            }
        }
        const int16_t sig = SigBuf()[cell];
        ++u.cellCount;
        u.maxSig = std::max(u.maxSig, sig);
        if (sig > 0) Accumulate(u, sig, d1, d2);
        SubTzAccess().map[cell] = VisitMark(cell);
        MarkExtra(cell, sig, u);
        if (!isSelf) PosnSpecial::Put(cell);
    });
}

/// 峰值按比例（/128）缩放。
int16_t Scale(uint8_t mult, int16_t sig) { return static_cast<int16_t>(mult * sig / 128); }

bool IsOnEdge(const PeakEntry& p) {
    const FlashPrmt& fl = PrmtFlash();
    return p.dim1 == fl.dim1Len - 1 || p.dim1 == 0 || p.dim2 == fl.dim2Len - 1 || p.dim2 == 0;
}

// SubTz_GetSignalThresh @ 0x6ba7de91
// 生长门限取区域峰值的一定比例：悬浮异常的区域按最弱峰与 sigMultSD；最强峰贴边的区域按最弱峰；其余按最强峰。
// 对应历史触点带 kZonePrevTouchFlag2 时总按最强峰。AFT 模式下直接用区域给的下限，结果不低于该下限。
// 原代码在悬浮大手指或带 kExcIgnoreZoneBound 异常时先看 SD 补偿，SignalDisparity 在本机型上裁掉了（从不补偿），这一支不保留。
int16_t GrowThold(const TouchZoneUnit& zone) {
    int16_t thold = zone.sigThold;
    if (static_cast<uint8_t>(Sys::IsAftModeEnabled()) == 0) {
        const FlashPrmt& fl = PrmtFlash();
        const PeakEntry& strongest = Peaks().entry[zone.maxSigPeakIdx];
        const PeakEntry& weakest = Peaks().entry[zone.minSigPeakIdx];
        if ((zone.flags & kZonePrevTouchFlag2) != 0) {
            thold = Scale(fl.sigMultNormal, strongest.sig);
        } else if (zone.exceptionType == kExcFloating) {
            thold = Scale(fl.sigMultSD, weakest.sig);
        } else if (IsOnEdge(strongest)) {
            thold = Scale(fl.sigMultNormal, weakest.sig);
        } else {
            thold = Scale(fl.sigMultNormal, strongest.sig);
        }
    }
    return std::max(thold, zone.sigThold);
}

// SubTZ_Centroid @ 0x6ba7d960
void Centroid(Unit& u) {
    u.centroidDim1 = static_cast<uint16_t>(Ctd::BasicCalculation(u.momentDim1, u.sigSum));
    u.centroidDim2 = static_cast<uint16_t>(Ctd::BasicCalculation(u.momentDim2, u.sigSum));
}

// SubTZ_GetAxis @ 0x6ba7f1cc
void Axis(Unit& u) {
    TouchZone::AxisFromMoments(u.sigSum, u.momentDim1, u.momentDim2, static_cast<double>(u.momentDim1Dim2),
                               static_cast<double>(u.momentDim1Sq), static_cast<double>(u.momentDim2Sq), &u.majorAxis,
                               &u.minorAxis, &u.angle);
    u.majorMm = static_cast<uint8_t>(TouchSize::PixelToMm(u.majorAxis));
    u.minorMm = static_cast<uint8_t>(TouchSize::PixelToMm(u.minorAxis));
}

struct PeakInfo {
    uint8_t strongest;  // 峰值最大者，没有时为 kNoPeak
    uint8_t count;
};

// SubTZ_PeakInfoRetrieval @ 0x6ba7dcec
// 刚长出的子区域覆盖到的、尚未归入别的子区域的峰：归入本子区域，计数并取峰值最大者。
PeakInfo CollectPeaks() {
    const PeakTable& pk = Peaks();
    const int8_t* map = Buffers().subTz;
    uint32_t& accessed = PeakAccessed();
    PeakInfo info{kNoPeak, 0};
    int16_t best = INT16_MIN;
    for (uint8_t i = 0; i < pk.count; ++i) {
        const uint32_t bit = 1u << i;
        if ((accessed & bit) != 0 || map[GridIndex(pk.entry[i].dim1, pk.entry[i].dim2)] == 0) continue;
        accessed |= bit;
        ++info.count;
        if (best < pk.entry[i].sig) {
            best = pk.entry[i].sig;
            info.strongest = i;
        }
    }
    return info;
}

// SubTZ_IsGrip @ 0x6ba7d9b0
bool IsGrip(const Unit& u) {
    return (u.minDim1 == 0 || u.maxDim1 == PrmtFlash().dim1Len - 1) && u.majorMm > kGripMajorMm;
}

// SubTZ_Filter @ 0x6ba7da00
// 子区域有效：最大信号超过区域最强峰的一半且信号和超过 Z9 最大的子区域的 1/8，或按握持算。
// 无效的记下原因；有效的（除 Z9 最大者本身）求它与 Z9 最大者之间的质心到最小峰信号比，取最大值。
void Filter(TouchZoneUnit& zone) {
    Table& t = Global();
    const int halfPeak = Peaks().entry[zone.maxSigPeakIdx].sig / 2;
    const int32_t eighthSum = UnitAt(t.maxZ9Idx).sigSum / 8;
    uint8_t valid = 0;
    for (uint8_t i = 0; i < t.count; ++i) {
        Unit& u = UnitAt(i);
        if ((halfPeak < u.maxSig && eighthSum < u.sigSum) || IsGrip(u)) {
            ++valid;
            if (i == t.maxZ9Idx) continue;
            const uint8_t ratio = TouchZone::CentToMinPkRatio(zone, u.peakIdx, UnitAt(t.maxZ9Idx).peakIdx);
            t.maxCentToMinPkRatio = std::max(t.maxCentToMinPkRatio, ratio);
        } else {
            if (u.maxSig <= halfPeak) u.flags |= kWeakPeak;
            if (u.sigSum <= eighthSum) u.flags |= kWeakSum;
        }
    }
    t.numAftFilter = valid;
}

}  // namespace

// SubTZ_Process @ 0x6ba7e646
// 以区域内每个尚未被子区域覆盖的峰为种子生长子区域（握持拆分过的区域按握持阶段，否则按信号门限）。
// 非空的子区域求质心、椭圆轴与包含的峰，交给 TZ 做拆分回退，并记下 Z9 最大、格子最多的子区域。
// 遍历期间 g_ptrAccess 切到 SubTZ 的句柄，PosnSpecial 的全局列表先存后还。
void Process(TouchZoneUnit& zone) {
    int32_t maxZ9 = INT32_MIN;
    const PosnSpecial::List savedList = PosnSpecial::Global();
    CurrentAccess() = &SubTzAccess();
    Table& t = Global();
    t = Table{};
    for (uint8_t i = 0; i < zone.peakCount; ++i) {
        const uint8_t seed = zone.peakIdx[i];
        const PeakEntry& peak = Peaks().entry[seed];
        if (Buffers().subTz[GridIndex(peak.dim1, peak.dim2)] != 0) continue;
        SigBuf() = Buffers().dif;
        Unit& u = UnitAt(t.count);
        if ((zone.flags & kZoneGripSeparated) == 0) {
            GrowByPeak(zone, u, GrowThold(zone), peak.dim1, peak.dim2);
        } else {
            GrowByGrip(zone, u, GripFilter::IsPeakAbsGrip(seed), peak.dim1, peak.dim2);
        }
        if (u.cellCount == 0) continue;
        SubTzAccess().tag += kTagStep;
        Centroid(u);
        Axis(u);
        PeakInfo info = CollectPeaks();
        if (info.strongest == kNoPeak) info.strongest = seed;
        u.peakIdx = info.strongest;
        u.peakCount = info.count;
        if (info.strongest != kNoPeak) {
            const int z9 = Peak::GetZ9(info.strongest);
            if (maxZ9 < z9) {
                maxZ9 = z9;
                t.maxZ9Idx = t.count;
            }
        }
        TouchZone::SeparateZoneRegret(zone, u);
        if (t.maxCellCount < u.cellCount) {
            t.maxCellCount = u.cellCount;
            t.maxCellIdx = t.count;
        }
        ++t.count;
    }
    if ((zone.flags & kZoneGripSeparated) == 0) Filter(zone);
    PosnSpecial::Global() = savedList;
    CurrentAccess() = &TzAccess();
}

// SubTZ_IsMF @ 0x6ba7ec1d
bool IsMultiFinger() {
    const Table& t = Global();
    const uint8_t valid = GetNumAftFilter();
    if (valid <= 1) return false;
    return t.maxCentToMinPkRatio < PrmtFlash().mfCentToMinPkRatio ||
           (PrevTouches().count == valid && t.maxCellCount > kMfCellCount);
}

// SubTZ_GetNumAftFilter @ 0x6ba7ec0c
uint8_t GetNumAftFilter() { return Global().numAftFilter; }

}  // namespace Solvers::Tsa::Core::SubTouchZone
