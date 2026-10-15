#include "Alg/HotZoneAdjust/HotZoneAdjust.h"

#include <array>
#include <cstddef>
#include <cstring>
#include <utility>

#include "Alg/Touch/TouchData.h"
#include "App/Scenario.h"
#include "App/Sys.h"
#include "App/TsaOut.h"
#include "App/TsaStatic.h"
#include "Common/Bss.h"
#include "Common/TsaMath.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::HotZoneAdjust {

namespace {

constexpr uint32_t kFeatureHotZoneAdjust = 0x400000;  // TsaStatic::featureFlags
constexpr uint32_t kPersistReported = 0x8;            // persistFlags：报过点
constexpr uint32_t kPersistRestarted = 0x20000;       // persistFlags：本帧重新开始报点
constexpr uint32_t kPersist3Adjusted = 0x8;           // persistFlags3：做过热区调整
constexpr uint8_t kRectNum = 50;
constexpr uint8_t kAppNum = 6;        // 统计分组：0 为全部，1..5 为 g_appList 里的应用
constexpr uint8_t kUnknownApp = 6;
constexpr uint16_t kRecordNearDist = 100;  // IsNeedRecord：按下点、抬起点离热区的容差（外部单位）
constexpr uint16_t kRecordStillDist = 10;  // 按下点与抬起点的最大偏差（外部单位）

/// 一个热区：Dim1 / Dim2 的首末坐标，以及所在格与尺寸档。
struct Rect {
    uint16_t dim1Min;
    uint16_t dim2Min;
    uint16_t dim1Max;
    uint16_t dim2Max;
    uint16_t dim1Idx;
    uint16_t dim2Idx;
    uint16_t sizeIdx;
};
static_assert(sizeof(Rect) == 0xE);

/// 一个分组的点击统计；Diff 为本日志周期的增量。
struct Stat {
    int32_t total;
    int32_t origin;    // 原始坐标就在热区里
    int32_t optimize;  // 原始坐标或调整后的坐标在热区里
    int32_t totalDiff;
    int32_t originDiff;
    int32_t optimizeDiff;
};
static_assert(sizeof(Stat) == 0x18);

constexpr size_t kDescriptionSize = 0x1f5;

/// g_hotZoneAnalyzer
struct Analyzer {
    uint64_t startTime;
    uint64_t endTime;
    char detail[kDescriptionSize];
    uint8_t reserved205;
    uint16_t detailLen;
    char dmd[kDescriptionSize];
    uint8_t reserved3FD;
    uint16_t dmdLen;
    const char* appName;
    uint8_t reserved408[0x8];
    uint8_t appIdx;
    uint8_t reserved411;
    Rect touchZone[41];  // 按触点 ID：按下时匹配到的热区（内部坐标）
    Stat stat[kAppNum];
};
static_assert(sizeof(Analyzer) == 0x6E0);
static_assert(offsetof(Analyzer, detailLen) == 0x206);
static_assert(offsetof(Analyzer, dmd) == 0x208);
static_assert(offsetof(Analyzer, dmdLen) == 0x3FE);
static_assert(offsetof(Analyzer, appName) == 0x400);
static_assert(offsetof(Analyzer, appIdx) == 0x410);
static_assert(offsetof(Analyzer, touchZone) == 0x412);
static_assert(offsetof(Analyzer, stat) == 0x650);

using RectTable = std::array<Rect, kRectNum>;

Analyzer& TheAnalyzer() { return Slot<Analyzer>(Bss().hotZoneAnalyzer); }
/// 上层下发的热区（显示坐标），全 0 的项为表尾。
RectTable& HotZones() { return Slot<RectTable>(Bss().hotZoneRect); }
/// g_appList：统计分组对应的应用包名。
constexpr std::array<const char*, kAppNum> kAppList{
    "all", "com.tencent.mm", "com.sina.weibo", "com.taobao.taobao", "com.ss.android.article.news", "com.baidu.searchbox",
};

TouchUnit& Cur(uint8_t idx) { return Touches().unit[idx]; }
// 原代码不检查历史下标：为 kNoTouch 时照样按下标读写，落在历史表之后的内存上。
PrevTouchUnit& Prev(uint8_t idx) { return PrevTouches().unit[idx]; }

uint16_t ExtToIntDist(uint16_t dist) { return static_cast<uint16_t>(Out::ExtToIntDist(dist)); }

bool IsEmpty(const Rect& r) { return r.dim1Min == 0 && r.dim1Max == 0 && r.dim2Min == 0 && r.dim2Max == 0; }

bool IsInRect(uint16_t dim1, uint16_t dim2, const Rect& r) {
    return dim1 >= r.dim1Min && dim1 <= r.dim1Max && dim2 >= r.dim2Min && dim2 <= r.dim2Max;
}

// ClearHotZone @ 0x6baf3be8
void ClearHotZones() { HotZones() = {}; }

// HotZoneAdjustScenario @ 0x6baf406a
bool IsScenarioAllowed() { return !Scenario::IsTyping() && static_cast<char>(Sys::IsScreenRotated()) == 0; }

// HotZoneIsInCore @ 0x6baf51b8
// 所在格不在网格的最外一圈。
bool IsInCore(const Rect& r) {
    const FlashPrmt& fl = PrmtFlash();
    return r.dim1Idx != 0 && r.dim1Idx < fl.hotZoneGridDim1 - 1 && r.dim2Idx != 0 && r.dim2Idx < fl.hotZoneGridDim2 - 1;
}

// ExtToInt @ 0x6baf409d / CalculateInternalHotZone @ 0x6baf4f8a
// 热区表换成内部坐标（到表尾为止）。
RectTable ToInternal() {
    RectTable out{};
    const RectTable& zones = HotZones();
    for (uint8_t i = 0; i < kRectNum && !IsEmpty(zones[i]); ++i) {
        out[i].dim1Min = static_cast<uint16_t>(Out::RevertDim1Pos(zones[i].dim1Min));
        out[i].dim1Max = static_cast<uint16_t>(Out::RevertDim1Pos(zones[i].dim1Max));
        out[i].dim2Min = static_cast<uint16_t>(Out::RevertDim2Pos(zones[i].dim2Min));
        out[i].dim2Max = static_cast<uint16_t>(Out::RevertDim2Pos(zones[i].dim2Max));
    }
    return out;
}

// CalculateDistance @ 0x6baf4286
// 点到矩形的距离，在矩形内为 0。
uint16_t DistanceTo(const Rect& r, uint16_t dim1, uint16_t dim2) {
    uint32_t d1 = 0;
    if (dim1 < r.dim1Min) d1 = r.dim1Min - dim1;
    else if (dim1 > r.dim1Max) d1 = dim1 - r.dim1Max;
    uint32_t d2 = 0;
    if (dim2 < r.dim2Min) d2 = r.dim2Min - dim2;
    else if (dim2 > r.dim2Max) d2 = dim2 - r.dim2Max;
    return static_cast<uint16_t>(Math::SqrtUint32(d2 * d2 + d1 * d1));
}

// SearchNearest @ 0x6baf4fbe / SearchNearestDisTwo @ 0x6baf458d
// 找离触点最近的热区；最近与次近的距离差超过 hotZoneMinGap 时才算找到。
bool SearchNearest(uint8_t curIdx, const RectTable& zones, uint8_t& nearest) {
    const Point pos = Cur(curIdx).rptPos;
    uint16_t best = 0xffff;
    uint16_t second = 0xffff;
    for (uint8_t i = 0; i < kRectNum && !IsEmpty(zones[i]); ++i) {
        const uint16_t d = DistanceTo(zones[i], pos.dim1, pos.dim2);
        if (d < best) {
            second = best;
            best = d;
            nearest = i;
        } else if (d < second) {
            second = d;
        }
    }
    return ExtToIntDist(PrmtFlash().hotZoneMinGap) < second - best;
}

// CalculateSizeIndex @ 0x6baf46dd
// 与热区面积（外部单位）最接近的尺寸档。
uint16_t SizeIndex(const Rect& r) {
    const auto halfDim1 = static_cast<int16_t>(Out::IntToExtDist(static_cast<int16_t>((r.dim1Max - r.dim1Min) / 2)));
    const auto halfDim2 = static_cast<int16_t>(Out::IntToExtDist(static_cast<int16_t>((r.dim2Max - r.dim2Min) / 2)));
    const auto area = static_cast<uint32_t>(halfDim2 * halfDim1 * 4);
    const FlashPrmt& fl = PrmtFlash();
    uint32_t bestDiff = UINT32_MAX;
    uint16_t best = 0;
    for (uint16_t k = 0; k < fl.hotZoneSizeNum; ++k) {
        const uint32_t sq = static_cast<uint32_t>(fl.hotZoneSize[k]) * fl.hotZoneSize[k];
        const uint32_t diff = sq < area ? area - sq : sq - area;
        if (diff < bestDiff) {
            best = k;
            bestDiff = diff;
        }
    }
    return best;
}

/// 区间中点：首端加上半长（原代码按 int 取半再截成 int16）。
uint16_t Center(uint16_t lo, uint16_t hi) { return static_cast<uint16_t>(static_cast<int16_t>((hi - lo) / 2) + lo); }

// CalculateHotZoneIdx @ 0x6baf50ad
// 热区中心落在 hotZoneGridDim1 × hotZoneGridDim2 网格的哪一格，以及尺寸档。
void SetGridAndSize(Rect& r) {
    const FlashPrmt& fl = PrmtFlash();
    const FlashConstPrmt& fc = PrmtFlashConst();
    r.dim1Idx = static_cast<uint16_t>(Center(r.dim1Min, r.dim1Max) / (fc.dim1LenQ8 / fl.hotZoneGridDim1));
    r.dim2Idx = static_cast<uint16_t>(Center(r.dim2Min, r.dim2Max) / (fc.dim2LenQ8 / fl.hotZoneGridDim2));
    r.sizeIdx = SizeIndex(r);
}

enum class ZoneResult : uint8_t { kFound, kNoHotZone, kTooClose, kInCore };

// UpdateHotZonePosition @ 0x6baf522e
// 为刚报点的触点找热区，找到时记到它的 ID 下。
ZoneResult FindZone(uint8_t curIdx, Rect& zone) {
    Analyzer& an = TheAnalyzer();
    const uint8_t id = Cur(curIdx).id;
    an.touchZone[id] = {};
    const RectTable internal = ToInternal();
    if (IsEmpty(HotZones()[0])) return ZoneResult::kNoHotZone;
    uint8_t nearest = 0;
    if (!SearchNearest(curIdx, internal, nearest)) return ZoneResult::kTooClose;
    zone = internal[nearest];
    SetGridAndSize(zone);
    if (PrmtFlash().hotZoneAdjustCore == 0 && IsInCore(zone)) return ZoneResult::kInCore;
    an.touchZone[id] = zone;
    return ZoneResult::kFound;
}

/// 一维的区间：中心与长度。
struct Span {
    int32_t center;
    uint16_t len;
};

/// 热区向四周扩展后的范围：落在其中的点会被拉进热区。
struct Expanded {
    uint16_t dim1Lo;  // 中心到两端的距离
    uint16_t dim2Lo;
    uint16_t dim1Hi;
    uint16_t dim2Hi;
    Span dim1;
    Span dim2;
};

// CalculateExpandZone @ 0x6baf480c
// 扩展量按尺寸档在 hotZoneExpandMin 与 hotZoneExpandMax 间插值，再按所在列 / 行在两端间插值，
// 加上尺寸档边长的一半；不小于热区自身的半长。
Expanded Expand(const Rect& r) {
    const FlashPrmt& fl = PrmtFlash();
    const auto halfDim1 = static_cast<uint16_t>(static_cast<uint16_t>(r.dim1Max - r.dim1Min) >> 1);
    const auto halfDim2 = static_cast<uint16_t>(static_cast<uint16_t>(r.dim2Max - r.dim2Min) >> 1);
    const uint16_t sizeHalf = ExtToIntDist(fl.hotZoneSize[r.sizeIdx]) >> 1;
    const int sizeSteps = fl.hotZoneSizeNum - 1;
    std::array<uint16_t, 4> edge{};
    for (int k = 0; k < 4; ++k) {
        const uint16_t large = ExtToIntDist(fl.hotZoneExpandMax[k]);
        const uint16_t small = ExtToIntDist(fl.hotZoneExpandMin[k]);
        edge[k] = static_cast<uint16_t>(small - static_cast<int16_t>(static_cast<int>(r.sizeIdx * static_cast<uint32_t>(small - large)) / sizeSteps));
    }
    const auto interp = [](uint16_t from, uint16_t to, uint16_t idx, int steps) {
        return static_cast<int16_t>(static_cast<int>(idx * static_cast<uint32_t>(to - from)) / steps);
    };
    const int16_t step1 = interp(edge[0], edge[1], r.dim1Idx, fl.hotZoneGridDim1 - 1);
    const int16_t step2 = interp(edge[2], edge[3], r.dim2Idx, fl.hotZoneGridDim2 - 1);
    const auto atLeast = [](uint16_t v, uint16_t min) { return v <= min ? min : v; };
    Expanded e;
    e.dim1Lo = atLeast(static_cast<uint16_t>(edge[0] + step1 + sizeHalf), halfDim1);
    e.dim1Hi = atLeast(static_cast<uint16_t>(edge[1] - step1 + sizeHalf), halfDim1);
    e.dim2Lo = atLeast(static_cast<uint16_t>(edge[2] + step2 + sizeHalf), halfDim2);
    e.dim2Hi = atLeast(static_cast<uint16_t>(edge[3] - step2 + sizeHalf), halfDim2);
    e.dim1.len = static_cast<uint16_t>(e.dim1Lo + e.dim1Hi);
    e.dim2.len = static_cast<uint16_t>(e.dim2Lo + e.dim2Hi);
    e.dim1.center = static_cast<int32_t>(static_cast<uint16_t>(halfDim1 + r.dim1Min) + (e.dim1.len >> 1) - static_cast<uint32_t>(e.dim1Lo));
    e.dim2.center = static_cast<int32_t>(static_cast<uint16_t>(halfDim2 + r.dim2Min) + (e.dim2.len >> 1) - static_cast<uint32_t>(e.dim2Lo));
    return e;
}

// CalculateTargetZone @ 0x6baf4d02 / CalculateTargetZoneLen @ 0x6baf4c6c
// 目标区间：以热区中心为中心、长宽比与扩展范围相同、不超出热区。
std::pair<Span, Span> Target(const Rect& r, const Expanded& e) {
    const auto len1 = static_cast<uint16_t>(r.dim1Max - r.dim1Min);
    const auto len2 = static_cast<uint16_t>(r.dim2Max - r.dim2Min);
    const uint16_t a = e.dim1.len != 0 ? e.dim1.len : 1;
    const uint16_t b = e.dim2.len != 0 ? e.dim2.len : 1;
    const auto fit1 = static_cast<uint16_t>(static_cast<int>(a * static_cast<uint32_t>(len2)) / b);
    const auto fit2 = static_cast<uint16_t>(static_cast<int>(b * static_cast<uint32_t>(len1)) / a);
    Span t1{static_cast<int32_t>(r.dim1Min + static_cast<uint32_t>(len1 >> 1)), fit1 <= len1 ? fit1 : len1};
    Span t2{static_cast<int32_t>(r.dim2Min + static_cast<uint32_t>(len2 >> 1)), fit2 <= len2 ? fit2 : len2};
    return {t1, t2};
}

// IsInAdjustLine @ 0x6baf4dd6
bool IsInSpan(uint16_t v, const Span& s) {
    const uint32_t half = s.len >> 1;
    return static_cast<int32_t>(s.center - half) < v && v < static_cast<int32_t>(s.center + half);
}

// CalculateNewPosition @ 0x6baf4eef
// 把扩展范围内的位置按比例映射到目标区间。
uint16_t MapInto(uint16_t v, const Span& from, const Span& to) {
    return static_cast<uint16_t>(static_cast<int16_t>(to.center) +
                                 static_cast<int16_t>(static_cast<int>((v - static_cast<uint32_t>(from.center)) * to.len) / static_cast<int>(from.len)));
}

// HotZoneAdjustProcessFirstUnit @ 0x6baf5c13 / HotZoneAdjustPoint @ 0x6baf5859
void AdjustFirst(uint8_t curIdx, const Rect& zone) {
    const Expanded e = Expand(zone);
    const auto [t1, t2] = Target(zone, e);
    TouchUnit& cur = Cur(curIdx);
    const Point pos = cur.rptPos;
    if (!IsInSpan(pos.dim1, e.dim1) || !IsInSpan(pos.dim2, e.dim2)) return;
    const uint16_t dim1 = MapInto(pos.dim1, e.dim1, t1);
    const uint16_t dim2 = MapInto(pos.dim2, e.dim2, t2);
    cur.hotZoneShiftDim1 = static_cast<int32_t>(static_cast<uint32_t>(dim1) - pos.dim1);
    cur.hotZoneShiftDim2 = static_cast<int32_t>(static_cast<uint32_t>(dim2) - pos.dim2);
    cur.rptPos = {dim1, dim2};
    cur.persistFlags3 |= kPersist3Adjusted;
}

/// 沿用上一帧的平移量，平移后不出 [0, lenQ8 - 1]。
int32_t ClampedShift(uint16_t pos, int32_t prevShift, uint16_t lenQ8) {
    if (prevShift + pos < 1) return -static_cast<int32_t>(pos);
    if (static_cast<int32_t>(pos + static_cast<uint32_t>(prevShift)) < lenQ8 - 1) return prevShift;
    return lenQ8 - 1 - pos;
}

// AdjustByHistory @ 0x6baf540d
void AdjustByHistory(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& cur = Cur(curIdx);
    const PrevTouchUnit& prev = Prev(prevIdx);
    const FlashConstPrmt& fc = PrmtFlashConst();
    cur.hotZoneShiftDim1 = ClampedShift(cur.rptPos.dim1, prev.hotZoneShiftDim1, fc.dim1LenQ8);
    cur.rptPos.dim1 = static_cast<uint16_t>(cur.rptPos.dim1 + static_cast<int16_t>(cur.hotZoneShiftDim1));
    cur.hotZoneShiftDim2 = ClampedShift(cur.rptPos.dim2, prev.hotZoneShiftDim2, fc.dim2LenQ8);
    cur.rptPos.dim2 = static_cast<uint16_t>(cur.rptPos.dim2 + static_cast<int16_t>(cur.hotZoneShiftDim2));
}

// IsNeedRecord @ 0x6baf6dae
// 抬起的触点值得统计：按下时找到过热区，按下点（falseFlickOffset，这里当位置用）与抬起点（jitPos）都在热区附近，
// 且两者相差不超过 kRecordStillDist（即一次点击）。
bool IsNeedRecord(uint8_t prevIdx) {
    const PrevTouchUnit& prev = Prev(prevIdx);
    const Rect& z = TheAnalyzer().touchZone[prev.id];
    if (IsEmpty(z)) return false;
    if (PrmtFlash().hotZoneAdjustCore == 0 && IsInCore(z)) return false;
    const int down1 = prev.falseFlickOffset.dim1;
    const int down2 = prev.falseFlickOffset.dim2;
    const int near = ExtToIntDist(kRecordNearDist);
    if (z.dim1Min > down1 + near || down1 > z.dim1Max + near || z.dim2Min > down2 + near || down2 > z.dim2Max + near) return false;
    const int up1 = prev.jitPos.dim1;
    const int up2 = prev.jitPos.dim2;
    if (up1 + near < z.dim1Min || z.dim1Max + near < up1 || up2 + near < z.dim2Min || z.dim2Max + near < up2) return false;
    const int still = ExtToIntDist(kRecordStillDist);
    return up1 <= still + down1 && up2 <= still + down2 && down1 <= still + up1 && down2 <= still + up2;
}

// HotZoneRecord @ 0x6baf726b：CalculateAppIndex @ 0x6baf61ed + CalculateSuccessNum @ 0x6baf6380
void Record(uint8_t prevIdx) {
    Analyzer& an = TheAnalyzer();
    an.appIdx = kUnknownApp;
    for (uint8_t k = 1; k < kAppNum; ++k) {
        if (an.appName != nullptr && std::strcmp(an.appName, kAppList[k]) == 0) an.appIdx = k;
    }
    const uint8_t app = an.appIdx;
    const PrevTouchUnit& prev = Prev(prevIdx);
    const Rect& z = an.touchZone[prev.id];
    const auto count = [&](int32_t Stat::*field, int32_t Stat::*diff) {
        ++(an.stat[0].*field);
        ++(an.stat[0].*diff);
        if (app < kAppNum) {
            ++(an.stat[app].*field);
            ++(an.stat[app].*diff);
        }
    };
    count(&Stat::total, &Stat::totalDiff);
    const bool originIn = IsInRect(prev.jitPos.dim1, prev.jitPos.dim2, z) &&
                          IsInRect(static_cast<uint16_t>(prev.falseFlickOffset.dim1), static_cast<uint16_t>(prev.falseFlickOffset.dim2), z);
    if (originIn) {
        count(&Stat::origin, &Stat::originDiff);
        count(&Stat::optimize, &Stat::optimizeDiff);
    } else if ((prev.persistFlags3 & kPersist3Adjusted) != 0 && IsInRect(prev.rptPos.dim1, prev.rptPos.dim2, z)) {
        count(&Stat::optimize, &Stat::optimizeDiff);
    }
}

// GenerateAndGetDetailDescription @ 0x6baf6d4a：描述串清空（StrPrintf 被裁掉），本周期的增量清零。
void ResetDetail() {
    Analyzer& an = TheAnalyzer();
    std::memset(an.detail, 0, sizeof an.detail);
    an.detailLen = 0;
    for (Stat& s : an.stat) {
        s.totalDiff = 0;
        s.originDiff = 0;
        s.optimizeDiff = 0;
    }
}

}  // namespace

// HotZoneAdjust_Init @ 0x6baf3fdd
void Init() {
    Analyzer& an = TheAnalyzer();
    std::memset(&an, 0, sizeof an);
    an.startTime = Static().curTimeMs;
}

// HotZoneAdjust_Process @ 0x6baf729e
// 刚报点（或重新开始报点）时找热区并调整，之后沿用平移量；报点的触点抬起时统计并清空热区表。
void Process(uint8_t curIdx, uint8_t prevIdx) {
    if ((Static().featureFlags & kFeatureHotZoneAdjust) == 0) return;
    if (!IsScenarioAllowed()) {
        ClearHotZones();
        return;
    }
    const TouchUnit& cur = Cur(curIdx);
    const PrevTouchUnit& prev = Prev(prevIdx);
    if ((cur.persistFlags & kPersistReported) != 0) {
        const bool first = (prev.persistFlags & kPersistReported) == 0 ||
                           ((cur.persistFlags & kPersistRestarted) != 0 && (prev.persistFlags & kPersistRestarted) == 0);
        if (first) {
            Rect zone{};
            if (FindZone(curIdx, zone) == ZoneResult::kFound) AdjustFirst(curIdx, zone);
        } else if ((prev.persistFlags3 & kPersist3Adjusted) != 0) {
            AdjustByHistory(curIdx, prevIdx);
            Cur(curIdx).persistFlags3 |= kPersist3Adjusted;
        }
    }
    if ((cur.persistFlags & kPersistReported) == 0 && (prev.persistFlags & kPersistReported) != 0) {
        if (IsNeedRecord(prevIdx)) Record(prevIdx);
        ResetDetail();
        ClearHotZones();
    }
}

// HotZoneAdjust_GenerateAndGetDmdDescription @ 0x6baf6d70
const char* GenerateAndGetDmdDescription() {
    Analyzer& an = TheAnalyzer();
    an.endTime = Static().curTimeMs;
    std::memset(an.dmd, 0, sizeof an.dmd);
    an.dmdLen = 0;
    std::memset(an.stat, 0, sizeof an.stat);
    an.startTime = Static().curTimeMs;
    return an.dmd;
}

}  // namespace Solvers::Tsa::Core::HotZoneAdjust
