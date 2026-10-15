// 握持区域的合并与拆分（TZ_GripMerge.c）：手掌边缘（握持格）与手指连成一个区域时，在握持格上补一个虚拟峰，
// 并把不在握持格上的峰移到峰列表末尾，让 SubTZ 按握持阶段把区域拆开。

#include <algorithm>
#include <cstdlib>
#include <initializer_list>
#include <utility>

#include "Alg/Peak/Peak.h"
#include "Alg/Peak/PeakData.h"
#include "Alg/Touch/PrevTouch.h"
#include "Alg/Touch/TouchData.h"
#include "Alg/Touch/TouchGeometry.h"
#include "Alg/TouchSize/TouchSize.h"
#include "Alg/TouchZone/SubTouchZone.h"
#include "Alg/TouchZone/TouchZone.h"
#include "Alg/TouchZone/TouchZoneInternal.h"
#include "App/Report.h"
#include "App/Sys.h"
#include "App/TsaBuffer.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::TouchZone {

namespace {

constexpr int16_t kLongHandTime = 5000;       // 手掌按下够久（ms）
constexpr uint32_t kPrevReported = 0x20000000; // PrevTouchUnit::persistFlags：已报点
constexpr uint32_t kPrevGripMergeHold = 0x200; // PrevTouchUnit::gripFlags
constexpr uint8_t kSideDim1Near = 4;           // TSA_GetSide：Dim1 方向的两侧
constexpr uint8_t kSideDim1Far = 8;

PeakEntry& PeakAt(uint8_t idx) { return Peaks().entry[idx]; }
const PrevTouchUnit& PrevAt(uint8_t idx) { return PrevTouches().unit[idx]; }
uint16_t PeakCell(const PeakEntry& p) { return GridIndex(p.dim1, p.dim2); }
uint16_t MmToPixel(uint8_t mm) { return static_cast<uint16_t>(TouchSize::MmToPixel(mm)); }

// ── 峰的握持属性 ─────────────────────────────────────────────────────────

// TZ_IsPeakInGrip @ 0x6ba843c8：Grid 峰表里同一下标的峰落在握持格上，且该格允许合并处理。
bool IsPeakInGrip(uint8_t peak) {
    const PeakEntry& p = PeaksGrid().entry[peak];
    return Buffers().gripStage[GridIndex(p.dim1, p.dim2)] == 1 && GripFilter_IsMergeHandlerSensorEnabled(p.dim1, p.dim2) != 0;
}

// TZ_IsPeakNotInGrip @ 0x6ba8437e：不在外边缘的峰，手掌又已按下够久。
bool IsPeakNotInGrip(const TouchZoneUnit& z, uint8_t peak) { return !Peak::IsOnOuterEdge(peak) && kLongHandTime <= z.minTimeHand; }

// TZ_GetMinPeakGripTime @ 0x6ba841d4
void GetMinPeakGripTime(TouchZoneUnit& z) {
    const TsaBufferTable& buf = Buffers();
    int16_t minTime = INT16_MAX;
    int16_t maxTimeOff = INT16_MIN;
    for (uint8_t k = 0; k < z.peakCount; ++k) {
        const uint16_t cell = PeakCell(PeakAt(z.peakIdx[k]));
        minTime = std::min(minTime, buf.gripTime[cell]);
        maxTimeOff = std::max(maxTimeOff, buf.gripTimeOff[cell]);
    }
    z.minPeakGripTime = minTime;
    z.maxPeakGripTimeOff = maxTimeOff;
}

// TZ_GetMaxNotGripTime @ 0x6ba84020
void GetMaxNotGripTime(TouchZoneUnit& z) {
    int16_t maxTime = 0;
    for (uint8_t k = 0; k < z.peakCount; ++k) {
        const PeakEntry& p = PeakAt(z.peakIdx[k]);
        if ((p.flags & kPeakNotInGrip) != 0) maxTime = std::max(maxTime, Buffers().gripTime[PeakCell(p)]);
    }
    z.maxNotGripTime = maxTime;
}

// ── 合并时机 ─────────────────────────────────────────────────────────────

// TZ_IsGripMergeAllowed @ 0x6ba85566：上一帧没有报点，或只报了一个点且本区域覆盖到它。
bool IsGripMergeAllowed(const TouchZoneUnit& z) {
    const uint8_t reported = PrevTouches().reportedThisFrame;
    if (reported >= 2) return false;
    return reported != 1 || (z.flags & kZoneMergedLastReported) != 0;
}

// TZ_GetOutEdgeSumRatio @ 0x6ba84e4c：质心所在一侧的边上信号和相对历史触点（%）。
void GetOutEdgeSumRatio(TouchZoneUnit& z) {
    const PrevTouchUnit& prev = PrevAt(z.prevIdx);
    const uint32_t side = Report::GetSide(z.centroidDim1, z.centroidDim2);
    const bool dim1 = side == kSideDim1Near || side == kSideDim1Far;
    const int32_t ref = dim1 ? prev.zoneEdgeSumDim1 : prev.zoneEdgeSumDim2;
    const int32_t cur = dim1 ? z.edgeSumDim1 : z.edgeSumDim2;
    z.outEdgeSumRatio = ref == 0 ? 100 : cur * 100 / ref;
}

// TZ_GetMove @ 0x6ba84fba：质心相对历史触点位置（扣掉其帧间位移）的偏移。
void GetMove(TouchZoneUnit& z) {
    if (z.prevIdx >= kMaxTouchNum) {
        z.moveDim1 = 0;
        z.moveDim2 = 0;
        return;
    }
    const PrevTouchUnit& prev = PrevAt(z.prevIdx);
    z.moveDim1 = static_cast<uint16_t>(std::abs(prev.posDelta.dim1 - prev.pos.dim1 + z.centroidDim1));
    z.moveDim2 = static_cast<uint16_t>(std::abs(prev.posDelta.dim2 - prev.pos.dim2 + z.centroidDim2));
}

// TZ_IsRptTouchMerged @ 0x6ba83b24：覆盖到两个历史触点，其中对应的那个已报点或区域里有从未报点的格子。
bool IsRptTouchMerged(const TouchZoneUnit& z) {
    if ((z.flags & kZoneIdMerged) == 0) return false;
    return (PrevAt(z.prevIdx).persistFlags & kPrevReported) != 0 || z.minTimeRpt == 0;
}

// TZ_IsToBeRptTouchMerged @ 0x6ba83b90
// 覆盖到的两个历史触点，久的一个已存在满一个握持间隔，新的一个不到 21 ms 且离 Dim1 边不近。
bool IsToBeRptTouchMerged(const TouchZoneUnit& z) {
    if ((z.flags & kZoneIdMerged) == 0 || z.mergedId == 0 || z.mergedId2 == 0) return false;
    const uint8_t a = PrevTouch::GetIdxBasedOnId(z.mergedId);
    if (a == kNoTouch) return false;
    const uint8_t b = PrevTouch::GetIdxBasedOnId(z.mergedId2);
    if (b == kNoTouch) return false;
    const bool aOlder = PrevAt(b).durationMs < PrevAt(a).durationMs;
    const PrevTouchUnit& older = PrevAt(aOlder ? a : b);
    const PrevTouchUnit& newer = PrevAt(aOlder ? b : a);
    if (older.durationMs < PrmtFlash().gripTimeInterval) return false;
    return newer.durationMs < 0x15 && 0x100 <= newer.minEdgeDistDim1;
}

// TZ_IsLineJump @ 0x6ba83d7d：沿 Dim1 突然跳出一段且边上信号变强，或历史触点刚出现就带合并保持标志。
bool IsLineJump(const TouchZoneUnit& z) {
    if (MmToPixel(12) <= z.moveDim2) return false;
    if (MmToPixel(4) <= z.moveDim1 && 0x56 <= z.outEdgeSumRatio) return true;
    if (MmToPixel(3) <= z.moveDim1 && 0x65 <= z.outEdgeSumRatio && 0x1389 <= z.maxPeakGripTimeOff) return true;
    const PrevTouchUnit& prev = PrevAt(z.prevIdx);
    return (prev.gripFlags & kPrevGripMergeHold) != 0 && prev.age == 1;
}

// TZ_IsPoorEllipseRate @ 0x6ba83ea4：轮廓离椭圆较远、几乎不沿 Dim2 移动、贴 Dim1 边的部分窄。
bool IsPoorEllipseRate(const TouchZoneUnit& z) {
    if (z.cellCount <= 3 || z.ellipseRate <= 0x14 || z.ellipseRateMirror <= 0x14) return false;
    if (MmToPixel(4) <= z.moveDim2 || PrmtFlash().maxNotGripTime < z.maxNotGripTime) return false;
    if (z.maxPeakGripTimeOff <= kLongHandTime || static_cast<int32_t>(PrevAt(z.prevIdx).gripFlags) < 0) return false;
    if (z.outEdgeSumRatio <= 0x55) return false;
    return static_cast<int8_t>(z.edgeWidthDim2) < 6 || z.nearEdgeMaxDim2 - z.nearEdgeMinDim2 < 3;
}

// TZ_IsPossibleGripMerge @ 0x6ba83fcd（TZ_IsTouchMerged @ 0x6ba83d3e）
bool IsPossibleGripMerge(const TouchZoneUnit& z) {
    return IsRptTouchMerged(z) || IsToBeRptTouchMerged(z) || IsLineJump(z) || IsPoorEllipseRate(z);
}

// TZ_IsOKToInsertGripPeak @ 0x6ba850e8：区域覆盖到握持格却没有峰在握持格上。
bool IsOkToInsertGripPeak(const TouchZoneUnit& z) {
    const uint32_t f = z.flags;
    return (f & kZonePeakInGrip) == 0 && (f & kZonePeakNotInGrip) != 0 && (f & kZoneCoversGrip) != 0 && IsPossibleGripMerge(z);
}

// TZ_IsTimeOnIntervalOK @ 0x6ba8513a：握持格比非握持峰早进入握持超过一个间隔（覆盖到上一帧最后报点时减半）。
bool IsTimeOnIntervalOk(const TouchZoneUnit& z) {
    uint16_t interval = PrmtFlash().gripTimeInterval;
    if ((z.flags & kZoneMergedLastReported) != 0) interval >>= 1;
    return interval < z.minGripTime - z.maxNotGripTime;
}

// TZ_IsToSeparateZone @ 0x6ba851a2
bool IsToSeparateZone(const TouchZoneUnit& z) {
    return IsTimeOnIntervalOk(z) && kLongHandTime <= z.minTimeHand && IsPossibleGripMerge(z);
}

// ── 补虚拟峰 ─────────────────────────────────────────────────────────────

struct MergeCandidate {
    int16_t sig;
    uint8_t dim1;
    uint8_t dim2;
};

// TZ_IsVPHighPriority @ 0x6ba848db
// 已选的格子 best 是否比新格子 cand 更适合：先看哪个与质心同在本帧区域，再看上一帧区域，都一样时比 Dif。
bool IsVpHighPriority(uint16_t center, uint16_t best, uint16_t cand) {
    for (const uint8_t* zone : {Buffers().tzZone, Buffers().tzZonePrev}) {
        const bool bestSame = zone[best] == zone[center];
        const bool candSame = zone[cand] == zone[center];
        if (bestSame != candSame) return bestSame;
        if (bestSame) break;
    }
    const int16_t* dif = Buffers().dif;
    return dif[cand] <= dif[best];
}

// TZ_SearchMergePeak @ 0x6ba84a90（TZ_GetSearchRange @ 0x6ba8d544）
// 区域没有峰在握持格上、但有不算握持的峰时，在质心周围 ±gripPeakEdgeDist（贴边一侧只往里找）
// 的握持格里找 Dif 达到跟踪门限的格子，取最合适的一个。
bool SearchMergePeak(const TouchZoneUnit& z, MergeCandidate& best) {
    if ((z.flags & kZonePeakInGrip) != 0 || (z.flags & kZonePeakNotInGrip) == 0) return false;
    const FlashPrmt& fl = PrmtFlash();
    const TsaBufferTable& buf = Buffers();
    const auto cd1 = static_cast<uint8_t>(z.centroidDim1 >> 8);
    const auto cd2 = static_cast<uint8_t>(z.centroidDim2 >> 8);
    const uint16_t center = GridIndex(cd1, cd2);
    const auto thold = static_cast<int16_t>(GripFilter_GetTrackingThold());
    const int r = fl.gripPeakEdgeDist;
    int lo1 = -r, hi1 = r, lo2 = -r, hi2 = r;
    if (z.minDim1 < 2) {
        hi1 = 1;
    } else if (fl.dim1Len - 2 <= z.maxDim1) {
        lo1 = -1;
    }
    if (z.minDim2 < 2) {
        hi2 = 1;
    } else if (fl.dim2Len - 2 <= z.maxDim2) {
        lo2 = -1;
    }
    bool found = false;
    for (int o1 = lo1; o1 <= hi1; ++o1) {
        for (int o2 = lo2; o2 <= hi2; ++o2) {
            const auto d1 = static_cast<uint8_t>(cd1 + o1);
            const auto d2 = static_cast<uint8_t>(cd2 + o2);
            if (d1 >= fl.dim1Len || d2 >= fl.dim2Len) continue;
            const uint16_t cell = GridIndex(d1, d2);
            if (buf.dif[cell] < thold || buf.gripStage[cell] != 1) continue;
            if (found && IsVpHighPriority(center, GridIndex(best.dim1, best.dim2), cell)) continue;
            found = true;
            best = {buf.dif[cell], d1, d2};
        }
    }
    return found;
}

// TZ_TryMergePeak @ 0x6ba84d81
void TryMergePeak(TouchZoneUnit& z) {
    MergeCandidate best{};
    if (SearchMergePeak(z, best)) InsertVirtualPeak(z, best.dim1, best.dim2);
}

// TZ_MovePeakNotInGripToEnd @ 0x6ba851f9：第一个不算握持的峰与最后一个峰对调。
void MovePeakNotInGripToEnd(TouchZoneUnit& z) {
    for (uint8_t k = 0; k < z.peakCount; ++k) {
        if ((PeakAt(z.peakIdx[k]).flags & kPeakNotInGrip) != 0) {
            std::swap(z.peakIdx[k], z.peakIdx[z.peakCount - 1]);
            return;
        }
    }
}

// TZ_IsAbnormalShape @ 0x6ba852d3
// 拆出的子区域：长轴超过 14 mm 时离历史触点太近（< 5 mm）或过扁；峰在 Dim1 内边缘时只有一列且较长，
// 或不到两列且峰值不到门限。
bool IsAbnormalShape(const TouchZoneUnit& z, const SubTouchZone::Unit& u) {
    if (0xe < u.majorMm) {
        uint16_t sqDist = 0;
        if (z.prevIdx < kMaxTouchNum) {
            const PrevTouchUnit& prev = PrevAt(z.prevIdx);
            sqDist = Touch::GetSqDistMm(z.centroidDim1, z.centroidDim2, prev.pos.dim1, prev.pos.dim2);
        }
        if (sqDist < 0x19) return true;
        if (u.minorMm * 3u < u.majorMm) return true;
    }
    if (!Peak::IsOnInnerEdgeDim1(u.peakIdx)) return false;
    if (u.maxDim1 == u.minDim1 && 2 < u.maxDim2 - u.minDim2) return true;
    return u.maxDim1 - u.minDim1 < 2 && PeakAt(u.peakIdx).sig < static_cast<int16_t>(Sys::GetSigThold(1));
}

}  // namespace

// TZ_GetPeakGripFlag @ 0x6ba84485
// 离边较远或不在握持格上（手掌按下够久）的峰记为非握持，在握持格上的记为握持；另记有无内部的非握持峰。
void GetPeakGripFlag(TouchZoneUnit& z) {
    GetMinPeakGripTime(z);
    for (uint8_t k = 0; k < z.peakCount; ++k) {
        const uint8_t peak = z.peakIdx[k];
        PeakEntry& p = PeakAt(peak);
        if (PrmtFlash().gripPeakEdgeDist < Peak::GetDistToEdge(peak)) {
            z.flags |= kZonePeakNotInGrip;
            p.flags |= kPeakNotInGrip;
        } else if (IsPeakInGrip(peak)) {
            z.flags |= kZonePeakInGrip;
            p.flags |= kPeakInGrip;
        } else if (IsPeakNotInGrip(z, peak)) {
            z.flags |= kZonePeakNotInGrip;
            p.flags |= kPeakNotInGrip;
        }
        if (!IsPeakInGrip(peak) && !Peak::IsOnOuterEdge(peak)) z.flags |= kZoneInnerNonGripPeak;
    }
    GetMaxNotGripTime(z);
}

// TZ_CheckToSeparateZone @ 0x6ba855b7
// 区域有对应的历史触点时：需要时在握持格上补虚拟峰；有握持峰也有非握持峰、且时机合适（或历史触点上一帧
// 已拆分）时，把非握持峰移到末尾并置 kZoneGripSeparated。
void CheckToSeparateZone(TouchZoneUnit& z) {
    if (!IsGripMergeAllowed(z) || z.prevIdx == kNoTouch) return;
    GetOutEdgeSumRatio(z);
    GetMove(z);
    GetPeakGripFlag(z);
    bool force = false;
    if ((PrevAt(z.prevIdx).persistFlags4 & 1) != 0 && (z.flags & kZonePeakNotInGrip) != 0) {
        z.flags |= kZoneForceSeparate;
        force = true;
    }
    if (force || IsOkToInsertGripPeak(z)) TryMergePeak(z);
    GetPeakGripFlag(z);
    if ((z.flags & kZonePeakInGrip) != 0 && (z.flags & kZonePeakNotInGrip) != 0 && (force || IsToSeparateZone(z))) {
        MovePeakNotInGripToEnd(z);
        z.flags |= kZoneGripSeparated;
    }
}

// TZ_SeparateZoneRegretProcess @ 0x6ba85492
void SeparateZoneRegret(TouchZoneUnit& z, const SubTouchZone::Unit& u) {
    if ((z.flags & kZoneGripSeparated) == 0 || (z.flags & kZoneForceSeparate) != 0) return;
    if ((PrevAt(z.prevIdx).persistFlags & kPrevReported) != 0) return;
    if (!GripFilter_IsPeakAbsNotGrip(u.peakIdx)) return;
    if (IsAbnormalShape(z, u) || 1 < u.peakCount) {
        z.flags |= kZoneSeparateRegret;
        z.flags &= ~kZoneGripSeparated;
    }
}

// TZ_TryExtraMergePeak @ 0x6ba84dea
void TryExtraMergePeak(TouchZoneUnit& z, uint8_t dim1, uint8_t dim2) {
    if ((z.flags & kZoneExtraMergeTried) != 0) return;
    z.flags |= kZoneExtraMergeTried;
    InsertVirtualPeak(z, dim1, dim2);
    GetPeakGripFlag(z);
}

}  // namespace Solvers::Tsa::Core::TouchZone
