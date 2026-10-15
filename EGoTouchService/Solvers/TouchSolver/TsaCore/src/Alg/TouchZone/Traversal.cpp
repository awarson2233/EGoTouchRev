// 区域生长（TZ_PeakBasedTraversal.c）：从峰出发按队列逐格访问 8 邻域，把格子标进区域图并累计各项统计。

#include <algorithm>
#include <array>
#include <cstdlib>

#include "Alg/Peak/Peak.h"
#include "Alg/Peak/PeakData.h"
#include "Alg/Touch/PrevTouch.h"
#include "Alg/Touch/TouchData.h"
#include "Alg/TpSensor/TpSensor.h"
#include "Alg/TouchZone/InnerCorner.h"
#include "Alg/TouchZone/TouchZoneInternal.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Prmt/Prmt.h"
#include "Tool/PosnList/PosnSpecial.h"

namespace Solvers::Tsa::Core::TouchZone {

namespace {

constexpr uint16_t kQueueSize = 100;
constexpr int kGripAgeGap = 0xc9;          // 贴边格与起点峰的握持年龄相差到这么多就不并入

/// 区域图里的格子类别（编码 = tag + kind）。
enum CellKind : int {
    kKindStrong = 1,       // 达到门限，继续扩展
    kKindWeak = 2,         // 未达门限的边界格
    kKindIsolated = 3,     // 取出时邻域里边缘格少于 2 个、自身够强
    kKindEdge = 4,         // 取出时信号低于全局峰值一半、但达到边缘门限
    kKindCore = 5,         // 取出时信号不低于全局峰值一半
    kKindInner = 6,        // 取出时 4 个正向邻居都超过门限
};

/// UpdateEdgeInfo 的 mode 位。
constexpr uint32_t kEdgeMaxSig = 0x1;       // 记边上的最大信号
constexpr uint32_t kNearEdgeMaxSig = 0x2;   // 记次边上的最大信号
constexpr uint32_t kEdgeExtent = 0x4;       // 记边 / 次边上的格子范围

struct Offset {
    int8_t dim1;
    int8_t dim2;
};
/// 自身在前，其后是 8 邻域：从右上起顺时针。奇数下标是上下左右四个正向邻居。
constexpr std::array<Offset, 9> kNeighbors = {{{0, 0}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}}};

// TZ_UnitMaxMinInit @ 0x6ba87d7e
void UnitMaxMinInit(TouchZoneUnit& z, int16_t thold) {
    z.minDim1 = 0xff;
    z.minDim2 = 0xff;
    z.edgeMinDim1 = 0x7f;
    z.edgeMinDim2 = 0x7f;
    z.nearEdgeMinDim1 = 0x7f;
    z.nearEdgeMinDim2 = 0x7f;
    z.maxDim1 = 0;
    z.maxDim2 = 0;
    z.edgeMaxDim1 = INT8_MIN;
    z.edgeMaxDim2 = INT8_MIN;
    z.nearEdgeMaxDim1 = INT8_MIN;
    z.nearEdgeMaxDim2 = INT8_MIN;
    z.edgeMaxSigDim1 = INT16_MIN;
    z.nearEdgeMaxSigDim1 = INT16_MIN;
    z.edgeMaxSigDim2 = INT16_MIN;
    z.nearEdgeMaxSigDim2 = INT16_MIN;
    z.sigThold = thold;
    z.mergedId = 0;
    z.mergedId2 = 0;
    z.minTimeHand = INT16_MAX;
    z.minGripTime = INT16_MAX;
    z.maxMinSigOff = INT16_MIN;
    z.minSentinel = INT16_MAX;
    z.minTimeRpt = INT16_MAX;
}

// TZ_UpdateDimMaxMin @ 0x6ba8781b
void UpdateDimMaxMin(TouchZoneUnit& z, uint8_t d1, uint8_t d2) {
    z.minDim1 = std::min(z.minDim1, d1);
    z.maxDim1 = std::max(z.maxDim1, d1);
    z.minDim2 = std::min(z.minDim2, d2);
    z.maxDim2 = std::max(z.maxDim2, d2);
}

// TZ_UpdateMValue @ 0x6ba87890
// 椭圆用的各阶矩（权重是信号，或 AFT / 带符号模式下的 |信号|）。
void UpdateMValue(TouchZoneUnit& z, int16_t w, uint8_t d1, uint8_t d2) {
    const int32_t w1 = w * d1;
    z.moment0 += w;
    z.momentDim1 += w1;
    z.momentDim1Dim2 += d2 * w1;
    z.momentDim1Sq += d1 * w1;
    z.momentDim2 += w * d2;
    z.momentDim2Sq += d2 * w * d2;
}

void ExtendRange(int8_t& lo, int8_t& hi, uint8_t v) {
    if (v < lo) lo = static_cast<int8_t>(v);
    if (hi < v) hi = static_cast<int8_t>(v);
}

// TZ_UpdateEdgeInfo @ 0x6ba87985
// 格子在逻辑边或次边上时累计该边的信号和，并按 mode 记最大信号与格子范围。
void UpdateEdgeInfo(TouchZoneUnit& z, int16_t sig, uint8_t d1, uint8_t d2, uint32_t mode) {
    const FlashPrmt& fl = PrmtFlash();
    if (d1 == fl.dim1LogicFirst || d1 == fl.dim1LogicLast) {
        z.edgeSumDim1 += sig;
        if ((mode & kEdgeMaxSig) != 0) z.edgeMaxSigDim1 = std::max(z.edgeMaxSigDim1, sig);
        if ((mode & kEdgeExtent) != 0) {
            if (d2 < z.edgeMinDim2) {
                z.edgeMinDim2 = static_cast<int8_t>(d2);
                z.edgeMinDim2Col = d1;
            }
            if (z.edgeMaxDim2 < d2) {
                z.edgeMaxDim2 = static_cast<int8_t>(d2);
                z.edgeMaxDim2Col = d1;
            }
        }
    } else if (d1 == fl.dim1LogicFirst + 1 || d1 == fl.dim1LogicLast - 1) {
        z.nearEdgeSumDim1 += sig;
        if ((mode & kNearEdgeMaxSig) != 0) z.nearEdgeMaxSigDim1 = std::max(z.nearEdgeMaxSigDim1, sig);
        if ((mode & kEdgeExtent) != 0) ExtendRange(z.nearEdgeMinDim2, z.nearEdgeMaxDim2, d2);
    }
    if (d2 == fl.dim2LogicFirst || d2 == fl.dim2LogicLast) {
        z.edgeSumDim2 += sig;
        if ((mode & kEdgeMaxSig) != 0) z.edgeMaxSigDim2 = std::max(z.edgeMaxSigDim2, sig);
        if ((mode & kEdgeExtent) != 0) {
            if (d1 < z.edgeMinDim1) {
                z.edgeMinDim1 = static_cast<int8_t>(d1);
                z.edgeMinDim1Row = d2;
            }
            if (z.edgeMaxDim1 < d1) {
                z.edgeMaxDim1 = static_cast<int8_t>(d1);
                z.edgeMaxDim1Row = d2;
            }
        }
    } else if (d2 == fl.dim2LogicFirst + 1 || d2 == fl.dim2LogicLast - 1) {
        z.nearEdgeSumDim2 += sig;
        if ((mode & kNearEdgeMaxSig) != 0) z.nearEdgeMaxSigDim2 = std::max(z.nearEdgeMaxSigDim2, sig);
        if ((mode & kEdgeExtent) != 0) ExtendRange(z.nearEdgeMinDim1, z.nearEdgeMaxDim1, d1);
    }
}

// TZ_IsGripWall @ 0x6ba87cc5
// 握持年龄模式下的首 / 末列：负信号，或与起点峰的握持年龄相差太大（两者都已有年龄）的格子不并入。
bool IsGripWall(uint8_t d1, uint16_t cell, uint16_t seedAge, int16_t sig) {
    const FlashPrmt& fl = PrmtFlash();
    if ((fl.gripFlags & kGripAge) == 0 || (d1 != 0 && d1 != fl.dim1Len - 1)) return false;
    const uint16_t age = Buffers().tzAge[cell];
    const bool farApart = seedAge != 0 && age != 0 && std::abs(seedAge - age) >= kGripAgeGap;
    return farApart || sig < 0;
}

// TZ_IsIDMerged @ 0x6ba87ef9
// 格子在上一帧属于某个触点：记下第一个这样的 ID，并按它的历史触点置标志；遇到第二个不同的 ID 时返回 true。
bool IsIdMerged(TouchZoneUnit& z, uint8_t d1, uint8_t d2) {
    const int8_t id = Buffers().tzId[GridIndex(d1, d2)];
    if (id == 0) return false;
    const PrevTouchTable& prev = PrevTouches();
    if (id == prev.lastReportedId) z.flags |= kZoneMergedLastReported;
    const uint8_t prevIdx = PrevTouch::GetIdxBasedOnId(static_cast<uint8_t>(id));
    if (prevIdx != kNoTouch) {
        if (prev.unit[prevIdx].zoneShape == static_cast<int32_t>(kShapeFatFinger)) z.flags |= kZoneMergedFatFinger;
        if ((prev.unit[prevIdx].persistFlags3 & 2) != 0) z.flags |= kZonePrevTouchFlag2;
    }
    if (z.mergedId == 0) {
        z.mergedId = static_cast<uint8_t>(id);
    } else if (id != static_cast<int8_t>(z.mergedId)) {
        z.mergedId2 = static_cast<uint8_t>(id);
        return true;
    }
    return false;
}

bool OnBorder(uint8_t d1, uint8_t d2) {
    const FlashPrmt& fl = PrmtFlash();
    return d1 == fl.dim1Len - 1 || d1 == 0 || d2 == fl.dim2Len - 1 || d2 == 0;
}

// TZ_IsEdgeMerged @ 0x6ba88122
// 起点峰不在屏幕边、而这个屏幕边上的格子上一帧不属于任何区域，且比起点峰早进入握持超过 gripTimeInterval。
bool IsEdgeMerged(const TouchZoneUnit& z, uint8_t d1, uint8_t d2) {
    const PeakEntry& seed = Peaks().entry[z.seedPeakIdx];
    if (OnBorder(seed.dim1, seed.dim2) || !OnBorder(d1, d2)) return false;
    const TsaBufferTable& buf = Buffers();
    const uint16_t cell = GridIndex(d1, d2);
    if (buf.tzZonePrev[cell] != 0) return false;
    return PrmtFlash().gripTimeInterval < buf.gripTime[cell] - buf.gripTime[GridIndex(seed.dim1, seed.dim2)];
}

// TZ_GetEdgeSigThold @ 0x6ba87e84
// 取出格子时判「边缘」的门限：全局峰值的 1/4 与起点峰值的一半中较小者，不低于生长门限。
int16_t EdgeSigThold(int16_t seedSig, int16_t thold) {
    auto t = static_cast<int16_t>(Prpt().dif.maxSig / 4);
    t = std::min(t, static_cast<int16_t>(seedSig / 2));
    return std::max(t, thold);
}

// TZ_GetEdgeWidth @ 0x6ba875ca
// 从贴边格的范围两端沿所在行（列）往外延伸，直到信号低于 thold，得到贴边部分的宽度。
void GetEdgeWidth(TouchZoneUnit& z, int16_t thold) {
    const FlashPrmt& fl = PrmtFlash();
    const int16_t* dif = Buffers().dif;
    const int cols = fl.dim1Len;
    const int rows = fl.dim2Len;
    if (z.edgeMaxDim1 == INT8_MIN || z.edgeMinDim1 == INT8_MAX) {
        z.edgeWidthDim1 = 0;
    } else {
        int hi = z.edgeMaxDim1;
        for (int c = hi; c < cols && thold <= dif[static_cast<uint16_t>(cols * z.edgeMaxDim1Row + c)]; ++c) hi = c;
        int lo = z.edgeMinDim1;
        for (int c = lo; c >= 0 && thold <= dif[static_cast<uint16_t>(cols * z.edgeMinDim1Row + c)]; --c) lo = c;
        z.edgeWidthDim1 = static_cast<uint8_t>(hi - lo + 1);
    }
    if (z.edgeMaxDim2 == INT8_MIN || z.edgeMinDim2 == INT8_MAX) {
        z.edgeWidthDim2 = 0;
    } else {
        // 列号按有符号字节读
        const auto colHi = static_cast<uint16_t>(static_cast<int8_t>(z.edgeMaxDim2Col));
        const auto colLo = static_cast<uint16_t>(static_cast<int8_t>(z.edgeMinDim2Col));
        int hi = z.edgeMaxDim2;
        for (int r = hi; r < rows && thold <= dif[static_cast<uint16_t>(colHi + r * cols)]; ++r) hi = r;
        int lo = z.edgeMinDim2;
        for (int r = lo; r >= 0 && thold <= dif[static_cast<uint16_t>(colLo + r * cols)]; --r) lo = r;
        z.edgeWidthDim2 = static_cast<uint8_t>(hi - lo + 1);
    }
    z.edgeWidthSum = static_cast<uint8_t>(z.edgeWidthDim1 + z.edgeWidthDim2);
}

/// 达到门限的格子：互容主流程中记下握持区域与按下时间的极值。
void RecordStrongCell(TouchZoneUnit& z, uint16_t cell) {
    if ((Static().processingState & kProcMsActive) == 0) return;
    const TsaBufferTable& buf = Buffers();
    z.gripRegion = std::max(z.gripRegion, buf.gripRegion[cell]);
    z.maxMinSigOff = std::max(z.maxMinSigOff, buf.minSigOff[cell]);
    z.minTimeRpt = std::min(z.minTimeRpt, buf.timeRpt[cell]);
}

struct TraversalCtx {
    int8_t* map;
    int16_t thold;
    int16_t halfPeak;          // 全局峰值的一半：PosnSpecial 记有效格子的门限
    uint16_t seedAge;
    int16_t centroidOffDim1;
    int16_t centroidOffDim2;
};

// TZ_TraversalNeighbor @ 0x6ba88709
// 访问 from 自身与 8 邻域里在屏内的格子。
//   - 未访问的：互容主流程中先记 tzZone、上一帧触点合并、握持；握持墙不并入。
//     达到门限（或是握持格）的标为 kKindStrong 并入队继续扩展，否则标为 kKindWeak；
//     两种都计入信号和与矩，正信号还计入质心。
//   - 已访问的：在别的区域里时置 kZoneTouchesOther。
void TraversalNeighbor(TouchZoneUnit& z, uint16_t from, const TraversalCtx& ctx) {
    const FlashPrmt& fl = PrmtFlash();
    TsaBufferTable& buf = Buffers();
    const uint32_t cols = fl.dim1Len;
    const uint32_t rows = fl.dim2Len;
    const bool ms = (Static().processingState & kProcMsActive) != 0;
    bool signedSig = (fl.gripFlags & kGripSignedSig) != 0;
    const auto fromDim1 = static_cast<uint8_t>(from % cols);
    const auto fromDim2 = static_cast<uint8_t>(from / cols);
    for (size_t k = 0; k < kNeighbors.size(); ++k) {
        const auto d1 = static_cast<uint8_t>(fromDim1 + kNeighbors[k].dim1);
        const auto d2 = static_cast<uint8_t>(fromDim2 + kNeighbors[k].dim2);
        const bool axial = (k & 1) != 0;
        if (d1 >= cols || d2 >= rows) continue;
        const auto cell = static_cast<uint16_t>(d1 + cols * d2);
        const int16_t sig = buf.dif[cell];
        ZoneSig()[cell] = sig;
        if (ms) {
            signedSig = !(d1 < cols - 2 && 1 < d1);
            if (k != 0) PosnSpecial::Record2(sig, ctx.thold);
        }
        const int16_t level = signedSig ? sig : Abs16(sig);
        if (ctx.map[cell] != 0) {
            if (k != 0 && ctx.thold <= level) PosnSpecial::Record(sig, ctx.halfPeak, axial);
            const int code = std::abs(static_cast<int>(ctx.map[cell]));
            const int tag = TzAccess().tag;
            if (code != 0 && (tag + 8 <= code || code < tag)) z.flags |= kZoneTouchesOther;
            continue;
        }
        if (ms) {
            if (PrmtDynamic().sigThold * 3 < sig * 4 || ctx.thold <= level) {
                buf.tzZone[cell] = TzAccess().tag;
                if (IsIdMerged(z, d1, d2)) z.flags |= kZoneIdMerged;
                if ((Static().featureFlags & 0x20) != 0) {
                    if (IsEdgeMerged(z, d1, d2)) z.flags |= kZoneEdgeMerged;
                    if (buf.gripStage[cell] == 1) {
                        z.flags |= kZoneCoversGrip;
                        z.minGripTime = std::min(z.minGripTime, buf.gripTime[cell]);
                    }
                }
            }
            z.minTimeHand = std::min(z.minTimeHand, TimeHand()[cell]);
        }
        if (IsGripWall(d1, cell, ctx.seedAge, sig)) continue;
        const bool gripCell = ms && buf.gripStage[cell] == 1;
        if (level < ctx.thold && !gripCell) {
            ctx.map[cell] = MapCode(kKindWeak, sig);
            if (sig > 0) UpdateEdgeInfo(z, sig, d1, d2, 0);
            ++LineWeakCount()[d1];
            ++LineWeakCount()[cols + d2];
        } else {
            ctx.map[cell] = MapCode(kKindStrong, sig);
            RecordStrongCell(z, cell);
            if (sig < 1) {
                ++z.negCellCount;
            } else {
                ++z.cellCount;
                UpdateEdgeInfo(z, sig, d1, d2, kEdgeMaxSig | kNearEdgeMaxSig | kEdgeExtent);
            }
            UpdateDimMaxMin(z, d1, d2);
            if (k != 0) {
                PosnSpecial::Record(sig, ctx.halfPeak, axial);
                PosnSpecial::Put(cell);
            }
        }
        z.sigSumAll += sig;
        LineSigSum()[d1] += sig;
        LineSigSum()[cols + d2] += sig;
        if (level > 0) UpdateMValue(z, level, d1, d2);
        if (sig < 1) {
            if (sig < z.minSig) {
                z.minSig = sig;
                z.minSigDim1 = d1;
                z.minSigDim2 = d2;
            }
            z.negSigSum += sig;
        } else {
            z.maxSig = std::max(z.maxSig, sig);
            if (buf.gripRegion[cell] != 0) z.gripRegionSigSum += sig;
            const int w1 = std::max(sig - ctx.centroidOffDim1, 0);
            const int w2 = std::max(sig - ctx.centroidOffDim2, 0);
            z.centroidWeightDim1 += w1;
            z.centroidWeightDim2 += w2;
            z.sigSum += sig;
            z.centroidMomentDim1 += (TpSensor::Dim1Offset()[cell] + d1 * 0x80) * w1 / 128;
            z.centroidMomentDim2 += (TpSensor::Dim2Offset()[cell] + d2 * 0x80) * w2 / 128;
            InnerCorner::Process(z, d1, d2);
        }
    }
}

// TZ_UpdateWeightedSumSimple @ 0x6ba896a4
void UpdateWeightedSumSimple(TouchZoneUnit& z, int16_t sig, uint8_t d1, uint8_t d2) {
    z.sigSum += sig;
    z.centroidWeightDim1 += sig;
    z.centroidWeightDim2 += sig;
    z.centroidMomentDim1 += sig * d1;
    z.centroidMomentDim2 += sig * d2;
}

// TZ_TraversalNeighborSimple @ 0x6ba89752
// 简化版：按 |信号| 比门限，不看握持与上一帧，质心直接按格坐标加权。
void TraversalNeighborSimple(TouchZoneUnit& z, uint16_t from, int8_t* map, int16_t thold) {
    const FlashPrmt& fl = PrmtFlash();
    const int16_t* dif = Buffers().dif;
    const uint32_t cols = fl.dim1Len;
    const uint32_t rows = fl.dim2Len;
    const auto fromDim1 = static_cast<uint8_t>(from % cols);
    const auto fromDim2 = static_cast<uint8_t>(from / cols);
    for (size_t k = 0; k < kNeighbors.size(); ++k) {
        const auto d1 = static_cast<uint8_t>(fromDim1 + kNeighbors[k].dim1);
        const auto d2 = static_cast<uint8_t>(fromDim2 + kNeighbors[k].dim2);
        if (d1 >= cols || d2 >= rows) continue;
        const auto cell = static_cast<uint16_t>(d1 + cols * d2);
        const int16_t sig = dif[cell];
        const int16_t level = Abs16(sig);
        if (map[cell] != 0) continue;
        if (level < thold) {
            map[cell] = MapCode(kKindWeak, sig);
            if (sig > 0) UpdateEdgeInfo(z, sig, d1, d2, 0);
        } else {
            map[cell] = MapCode(kKindStrong, sig);
            RecordStrongCell(z, cell);
            if (sig < 1) {
                ++z.negCellCount;
            } else {
                ++z.cellCount;
                UpdateEdgeInfo(z, sig, d1, d2, kEdgeMaxSig);
                UpdateDimMaxMin(z, d1, d2);
            }
            if (k != 0) PosnSpecial::Put(cell);
        }
        z.sigSumAll += sig;
        if (level > 0) UpdateMValue(z, level, d1, d2);
        if (sig > 0) UpdateWeightedSumSimple(z, sig, d1, d2);
    }
}

// TZ_PeakFilterOnce @ 0x6ba89cc0
// 区域有 3 个以上的峰时，找一对相距不到 3 格的峰：把它们单独当作一个两峰区域求 DFS 属性，
// 中点信号比与平均信号比都高（两峰之间没有明显的谷）时去掉后一个峰。返回是否去掉了一个。
// 试算用一份区域副本，各次试算之间不复位。
bool PeakFilterOnce(TouchZoneUnit& z) {
    if (z.peakCount <= 2) return false;
    PeakTable& pk = Peaks();
    TouchZoneUnit trial = z;
    for (int i = 0; i < z.peakCount - 1; ++i) {
        const uint8_t a = z.peakIdx[i];
        for (int j = i + 1; j < z.peakCount; ++j) {
            const uint8_t b = z.peakIdx[j];
            if (Peak::GetDistance(a, b) >= 3) continue;
            trial.peakCount = 2;
            trial.peakIdx[0] = a;
            trial.peakIdx[1] = b;
            trial.minSigPeakIdx = pk.entry[a].sig < pk.entry[b].sig ? a : b;
            GetDfsPrpt(trial);
            if (PrmtFlash().peakFilterCentToMinPkRatio < trial.centToMinPkRatio && 0x30 < trial.avgSigRatio) {
                if (pk.entry[a].id == 0 && pk.entry[b].id != kNoPeak) pk.entry[a].id = pk.entry[b].id;
                for (; j < z.peakCount - 1; ++j) z.peakIdx[j] = z.peakIdx[j + 1];
                --z.peakCount;
                return true;
            }
        }
    }
    return false;
}

}  // namespace

// TZ_PeakBasedTraversal @ 0x6ba89162
// 从 (dim1, dim2) 的峰出发生长区域。每取出一个格子先访问它的邻域，再按 PosnSpecial 对邻域的统计
// 给它定类别：4 个正向邻居都有效为 kKindInner；邻域里边缘格少于 2 个、自身达到门限且比边缘格之和
// 的一半强为 kKindIsolated；否则按信号与全局峰值一半、边缘门限的关系为 kKindCore / kKindEdge（不够边缘
// 门限时保留 kKindStrong）。结束后求贴边宽度。
void PeakBasedTraversal(TouchZoneUnit& z, int16_t thold, uint8_t dim1, uint8_t dim2, bool centroidSigOffset) {
    const FlashPrmt& fl = PrmtFlash();
    const TsaBufferTable& buf = Buffers();
    std::fill_n(LineSigSum(), kLineBufBytes / sizeof(int32_t), 0);
    std::fill_n(LineWeakCount(), kLineBufBytes / sizeof(int32_t), 0);
    std::array<uint16_t, kQueueSize> queue{};
    PosnSpecial::Init(queue.data(), kQueueSize);
    UnitMaxMinInit(z, thold);
    ClearChecked(ZoneSig(), GridBufSize() * sizeof(int16_t), fl.sensorNum * sizeof(int16_t));
    const uint16_t seed = GridIndex(dim1, dim2);
    TraversalCtx ctx{};
    ctx.map = TzAccess().map;
    ctx.thold = thold;
    ctx.halfPeak = static_cast<int16_t>(Prpt().dif.maxSig / 2);
    ctx.seedAge = buf.tzAge[seed];
    if (centroidSigOffset) {
        ctx.centroidOffDim1 = fl.centroidSigOffsetDim1;
        ctx.centroidOffDim2 = fl.centroidSigOffsetDim2;
    }
    const int16_t edgeThold = EdgeSigThold(buf.dif[seed], thold);
    PosnSpecial::Put(seed);
    while (!PosnSpecial::IsEmpty()) {
        PosnSpecial::ReBase();
        const uint16_t cell = PosnSpecial::Get();
        TraversalNeighbor(z, cell, ctx);
        const int16_t sig = buf.dif[cell];
        if (PosnSpecial::GetNumSignChange() > 2) ++z.signChangeCells;
        if (PosnSpecial::GetNumValidSensor() == 4) {
            ctx.map[cell] = MapCode(kKindInner, sig);
        } else if (PosnSpecial::GetNumEdgeSensor() < 2 && thold <= sig && PosnSpecial::GetSumEdgeSensor() < sig * 2) {
            ctx.map[cell] = MapCode(kKindIsolated, sig);
            ++z.isolatedCells;
        } else if (ctx.halfPeak <= sig) {
            ctx.map[cell] = MapCode(kKindCore, sig);
        } else if (edgeThold <= sig) {
            ctx.map[cell] = MapCode(kKindEdge, sig);
        }
    }
    GetEdgeWidth(z, static_cast<int16_t>(fl.aftEdgeWidthThold));
}

// TZ_PeakBasedTraversalSimple @ 0x6ba89b60
// 取出的格子一律标为 kKindCore。
void PeakBasedTraversalSimple(TouchZoneUnit& z, int16_t thold, uint8_t dim1, uint8_t dim2) {
    int8_t* map = TzAccess().map;
    std::array<uint16_t, kQueueSize> queue{};
    PosnSpecial::Init(queue.data(), kQueueSize);
    UnitMaxMinInit(z, thold);
    PosnSpecial::Put(GridIndex(dim1, dim2));
    while (!PosnSpecial::IsEmpty()) {
        PosnSpecial::ReBase();
        const uint16_t cell = PosnSpecial::Get();
        TraversalNeighborSimple(z, cell, map, thold);
        map[cell] = MapCode(kKindCore, Buffers().dif[cell]);
    }
}

// TZ_PeakFilter @ 0x6ba8a009
void PeakFilter(TouchZoneUnit& z) {
    while (PeakFilterOnce(z)) {
    }
}

}  // namespace Solvers::Tsa::Core::TouchZone
