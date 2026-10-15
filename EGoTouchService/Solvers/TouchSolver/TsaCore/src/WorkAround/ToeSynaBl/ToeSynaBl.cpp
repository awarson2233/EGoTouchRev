#include "App/Sys.h"
#include "WorkAround/ToeSynaBl/ToeSynaBl.h"

#include <cstddef>
#include <cstdlib>
#include <cstring>

#include "Alg/Baseline/BaselineInternal.h"
#include "Alg/Peak/PeakData.h"
#include "Alg/SelfData/SelfData.h"
#include "Alg/Touch/TouchData.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Common/Assert.h"
#include "Common/Image.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::ToeSynaBl {

namespace B = Baseline;

namespace {

/// FlashPrmt::toeSynaBlFlags
constexpr uint32_t kFlagBackLight = 0x1;
constexpr uint32_t kFlagWater = 0x2;
constexpr uint32_t kFlagShape = 0x8;

/// IsNextSideDifNormal 的结果位
constexpr uint32_t kSideSig = 0x01;       // 有格超过 0xf9
constexpr uint32_t kSideStrong = 0x02;    // 有格超过强信号门限
constexpr uint32_t kSideNegative = 0x08;  // 有格低于负门限
constexpr uint32_t kSideRising = 0x10;    // 端点弱而近端有格超过 300

constexpr uint32_t kPersistSmallTouch = 0x20000000;  // PrevTouchUnit::persistFlags
constexpr int32_t kStrongSelf = 8001;                // 自容强信号
constexpr uint64_t kTimerLimit = 300;

/// 0x6bbbf590 起的模块状态。
struct State {
    int16_t* rawSinceBackLightShift;  // 背光偏移开始时的 CMF 前 raw
    int16_t* rawSinceClothRisk;       // 只分配，不使用
    int16_t* rawSinceDim2SsNoSignal;  // Dim2 自容无信号开始时的 CMF 前 raw
    uint8_t excludeSide;              // 两侧列排除在基线判定之外
};
static_assert(offsetof(State, excludeSide) == 0x18);

State& S() { return Image::At<State>(0x6bbbf590); }

bool Enabled() { return PrmtFlash().toeSynaBl != 0; }
bool Flag(uint32_t bit) { return (PrmtFlash().toeSynaBlFlags & bit) != 0; }
bool SideTouch() { return (Static().featureFlags & kFeatSideTouch) != 0; }
bool IsGame() { return static_cast<char>(TSA_IsGameScenario()) != 0; }
void Debug(uint32_t bit) { Static().blDebugFlags |= bit; }
int16_t DifAt(uint8_t col, uint8_t row) { return Buffers().dif[GridIndex(col, row)]; }
/// 这一列的 Dif 和强（不小于 8001 且超过 SS Dif 的 10 倍）。
bool StrongSelf(int col) { return kStrongSelf <= SelfForBlReset().sum[col] && SsDifForBl()[col] * 10 < SelfForBlReset().sum[col]; }

// ToeSynaBl_IsColumnRawChanged @ 0x6bb46bc3
// 这一列的 CMF 前 raw 与抓取时相比有格变化超过 200。
bool IsColumnRawChanged(uint8_t col) {
    const TsaBufferTable& buf = Buffers();
    const auto* cur = reinterpret_cast<const uint16_t*>(buf.preCmfRaw);
    const auto* cap = reinterpret_cast<const uint16_t*>(buf.rawCaptureNextSide);
    for (uint8_t row = 0; row < PrmtFlash().dim2Len; ++row) {
        const uint16_t i = GridIndex(col, row);
        if (200 < std::abs(cur[i] - cap[i])) return true;
    }
    return false;
}

// ToeSynaBl_IsNextSideDifNormal @ 0x6bb46c9c
// 按 Dim2 分三段（两端各 6 行门限高些）看这一列的 Dif，返回 kSide* 位。
uint32_t IsNextSideDifNormal(uint8_t col) {
    const uint8_t rows = PrmtFlash().dim2Len;
    const uint8_t last = static_cast<uint8_t>(rows - 1);
    uint32_t r = 0;
    if (0xf9 < DifAt(col, 0) || 0xf9 < DifAt(col, last)) r = kSideSig;
    const auto scan = [&](uint8_t from, int to, int16_t strong, int16_t negative) {
        for (uint8_t row = from; row < to; ++row) {
            const int16_t v = DifAt(col, row);
            if (strong < v) r |= kSideStrong;
            if (0xf9 < v) r |= kSideSig;
            if (v < negative) r |= kSideNegative;
        }
    };
    scan(1, 7, 0x5dc, -0x4b0);
    scan(7, rows - 7, 0x3b6, -900);
    scan(static_cast<uint8_t>(rows - 7), rows - 1, 0x5dc, -0x4b0);
    if (DifAt(col, 0) < 300) {
        for (uint8_t row = 1; row < 7; ++row) {
            if (300 < DifAt(col, row)) {
                r |= kSideRising;
                break;
            }
        }
    }
    if (DifAt(col, last) < 300) {
        for (auto row = static_cast<uint8_t>(rows - 7); row < rows - 1; ++row) {
            if (300 < DifAt(col, row)) return r | kSideRising;
        }
    }
    return r;
}

// ToeSynaBl_PrptSideRawProtected @ 0x6bb46f9f
// 两侧都没有信号，或有强信号 / 近端上升，或有负信号。
bool PrptSideRawProtected(uint32_t a, uint32_t b) {
    if ((a & kSideSig) == 0 && (b & kSideSig) == 0) return true;
    if (((a | b) & (kSideStrong | kSideRising)) != 0) return true;
    return ((a | b) & kSideNegative) != 0;
}

// ToeSynaBl_PrptSideRawProtected2 @ 0x6bb4701b
// 两侧都没有信号；或一侧有强信号 / 有信号而自容不强（SS Dif 也不明显为负）。
bool PrptSideRawProtected2(int cols, uint8_t side, uint32_t a, uint32_t b) {
    const int far = cols - 1 - side;
    const bool strongNear = StrongSelf(side);
    const bool strongFar = StrongSelf(far);
    const int32_t* ss = SsDifForBl();
    if ((a & kSideSig) == 0 && (b & kSideSig) == 0) return true;
    if ((a & kSideStrong) != 0 && !strongNear) return true;
    if ((b & kSideStrong) != 0 && !strongFar) return true;
    if ((a & kSideSig) != 0 && -3000 < ss[side] && !strongNear) return true;
    return (b & kSideSig) != 0 && -3000 < ss[far] && !strongFar;
}

// ToeSynaBl_BackLightShiftCheckTouch @ 0x6bb45d0d
// 有要保护的触点，或游戏场景下有持续可信的触点。
bool BackLightShiftCheckTouch() {
    if (B::Reset::IsTouchToBeProtected2(false)) return true;
    const PrevTouchTable& prev = PrevTouches();
    if (IsGame() && prev.count < 6) {
        for (uint16_t i = 0; i < prev.count; ++i) {
            if (0xe < prev.unit[i].blTrustFrames) {
                Debug(0x1);
                return true;
            }
        }
    }
    return false;
}

bool IsRawChangedSinceBackLightShift(bool store) { return B::Reset::IsRawChanged(S().rawSinceBackLightShift, store); }
bool IsRawChangedSinceSsNoSignal(bool store) { return B::Reset::IsRawChanged(S().rawSinceDim2SsNoSignal, store); }

// ToeSynaBl_BackLightShiftProcess @ 0x6bb463d8
// 背光偏移开始时存下 raw；之后 raw 不变时累计，变化即清零。
void BackLightShiftProcess() {
    TsaStatic& stat = Static();
    if (!BackLightShiftUnit(0x50)) {
        stat.toeBackLightShiftTimer = 0;
    } else if (stat.toeBackLightShiftTimer == 0) {
        stat.toeBackLightShiftTimer = 1;
        IsRawChangedSinceBackLightShift(true);
    } else if (!IsRawChangedSinceBackLightShift(false)) {
        Debug(0x20000);
        stat.toeBackLightShiftTimer = B::UpdateTimer(stat.toeBackLightShiftTimer);
    } else {
        Debug(0x40000);
        stat.toeBackLightShiftTimer = 0;
    }
}

// ToeSynaBl_ClothRiskProcess @ 0x6bb464e6
// 计时器只会回落（原代码没有累加的地方），实际恒为 0。
void ClothRiskProcess() {
    TsaStatic& stat = Static();
    if (Prpt().field16E < -799) {
        stat.toeClothRiskTimer = 0;
        return;
    }
    Debug(0x10000);
    if (stat.frameIntervalMs < stat.toeClothRiskTimer) {
        B::SubInterval(stat.toeClothRiskTimer);
    } else {
        stat.toeClothRiskTimer = 0;
    }
}

// ToeSynaBl_SSIsDim2NoSignal @ 0x6bb4616a
// Dim2 方向的自容 Dif 都不超过门限（两端放宽到 2.5 倍低门限）。
bool SsIsDim2NoSignal() {
    const FlashPrmt& fl = PrmtFlash();
    const auto end = static_cast<uint8_t>(fl.dim2Len + fl.dim1Len);
    const auto low = static_cast<uint16_t>(PrmtDynamic().ssTholdLow);
    for (uint8_t i = fl.dim1Len; i < end; ++i) {
        const int thold = i == fl.dim1Len || i == end - 1 ? static_cast<int>(low * 10u) >> 2 : low * 2;
        if (thold < SsDifForBl()[i]) return false;
    }
    return true;
}

// ToeSynaBl_IsLongTimeSSNoSignal @ 0x6bb46228
// Dim2 自容无信号而互容有明显信号时累计；首帧存下 raw，此后 raw 变化即重新计时。
bool IsLongTimeSsNoSignal() {
    TsaStatic& stat = Static();
    bool r = false;
    if (!SsIsDim2NoSignal() || !B::Reset::CheckNumOverNoise() || Prpt().difInner.maxSig <= PrmtFlash().fingerThold / 2 ||
        (stat.featureFlags & kFeatSmartCover) != 0) {
        stat.toeSsNoSignalTimer = 0;
    } else {
        const int dt = stat.frameIntervalMs;
        if (stat.toeSsNoSignalTimer < 0x12d) {
            B::AddInterval(stat.toeSsNoSignalTimer);
        } else {
            stat.toeSsNoSignalTimer = 0;
            r = true;
        }
        if (stat.toeSsNoSignalTimer == dt) {
            IsRawChangedSinceSsNoSignal(true);
        } else if (dt < stat.toeSsNoSignalTimer) {
            if (IsRawChangedSinceSsNoSignal(false)) {
                stat.toeSsNoSignalTimer = 0;
            } else {
                Debug(0x200000);
            }
        }
    }
    stat.ssNoSignal = r;
    return r;
}

// ToeSynaBl_IsAbnormalTouchShape @ 0x6bb4681a（IsBaselineAbnormalSSProtectedByDim1 依赖被裁掉的 SS，恒为 0）
// 有长条（field066 超过 16）的小触点贴在第 0 / 1 列或最后两列，累计超过 300 ms。
bool IsAbnormalTouchShape(bool skip) {
    TsaStatic& stat = Static();
    if (!Enabled() || !Flag(kFlagShape) || skip) {
        stat.toeAbnormalShapeTimer = 0;
        return false;
    }
    const PrevTouchTable& prev = PrevTouches();
    const int lastCols = PrmtFlash().dim1Len - 3;
    bool abnormal = false;
    for (uint16_t i = 0; i < prev.count; ++i) {
        const PrevTouchUnit& u = prev.unit[i];
        const uint8_t dim1 = Peaks().entry[u.peakIdx].dim1;
        if (0x10 < u.field066 && (u.persistFlags & kPersistSmallTouch) != 0 && (dim1 < 2 || lastCols < dim1)) abnormal = true;
    }
    if (abnormal) {
        Debug(0x40);
        stat.toeAbnormalShapeTimer = B::UpdateTimer(stat.toeAbnormalShapeTimer);
    } else {
        stat.toeAbnormalShapeTimer = 0;
    }
    const bool r = kTimerLimit < stat.toeAbnormalShapeTimer;
    if (r) stat.toeAbnormalShapeTimer = 0;
    return r;
}

// ToeSynaBl_GripRegionAbnormal @ 0x6bb474ef
// 侧边触摸时，两侧第 1、2 列自容强而不受保护、中间列的自容第 7 段都平静，持续 2 s 即判异常。
bool GripRegionAbnormal(bool protected2) {
    TsaStatic& stat = Static();
    const FlashPrmt& fl = PrmtFlash();
    if (!SideTouch() || fl.dim1Len < 4 || IsGame() || !B::Reset::CheckNumOverNoise() || protected2) {
        stat.toeGripRegionTime = stat.curTimeMs;
        return false;
    }
    const int cols = fl.dim1Len;
    bool abnormal = false;
    for (uint16_t c = 1; c < 3; ++c) {
        const uint32_t a = IsNextSideDifNormal(static_cast<uint8_t>(c));
        const uint32_t b = IsNextSideDifNormal(static_cast<uint8_t>(cols - c - 1));
        const bool p1 = PrptSideRawProtected(a, b);
        const bool p2 = PrptSideRawProtected2(cols, static_cast<uint8_t>(c), a, b);
        const int far = cols - 1 - c;
        if (StrongSelf(c)) {
            abnormal = true;
        } else if (0x4b0 < SelfForBlReset().maxAbs[c]) {
            abnormal = false;
            break;
        }
        if (StrongSelf(far)) {
            abnormal = true;
        } else if (0x4b0 < SelfForBlReset().maxAbs[far]) {
            abnormal = false;
            break;
        }
        if (p1 && p2) {
            abnormal = false;
            break;
        }
    }
    for (uint16_t c = 3; c < cols - 3; ++c) {
        const int16_t v = SelfForBlReset().maxAbs[c];
        if (400 < v || v < -400) {
            abnormal = false;
            break;
        }
    }
    if (!abnormal) {
        stat.toeGripRegionTime = stat.curTimeMs;
        return false;
    }
    if (2000 < stat.curTimeMs - stat.toeGripRegionTime) {
        stat.toeGripRegionTime = stat.curTimeMs;
        return true;
    }
    Debug(0x100000);
    return false;
}

// ToeSynaBl_IsWaterDetectedZ8Check @ 0x6bb45950
// field172 / field173 所指的格不贴边，且其 3×3 邻域里有 Dif 高于 -150 的格。
bool IsWaterDetectedZ8Check() {
    const TsaPrpt& prpt = Prpt();
    const FlashPrmt& fl = PrmtFlash();
    const uint16_t d1 = prpt.field172;
    const uint16_t d2 = prpt.field173;
    if (d1 == 0 || d2 == 0 || fl.dim1Len - 1 <= d1 || fl.dim2Len - 1 <= d2) return false;
    for (int a = -1; a < 2; ++a) {
        for (int b = -1; b < 2; ++b) {
            const auto i = static_cast<uint16_t>(fl.dim1Len * (b + d2) + d1 + a);
            if (-0x96 < Buffers().dif[i]) return true;
        }
    }
    return false;
}

}  // namespace

// ToeSynaBl_Malloc @ 0x6bb46094
bool Malloc() {
    const uint32_t bytes = GridBufSize() * 2u;
    State& s = S();
    s.rawSinceClothRisk = static_cast<int16_t*>(TSA_MallocAndInit(bytes));
    if (s.rawSinceClothRisk == nullptr) return false;
    s.rawSinceBackLightShift = static_cast<int16_t*>(TSA_MallocAndInit(bytes));
    if (s.rawSinceBackLightShift == nullptr) return false;
    s.rawSinceDim2SsNoSignal = static_cast<int16_t*>(TSA_MallocAndInit(bytes));
    return s.rawSinceDim2SsNoSignal != nullptr;
}

// ToeSynaBl_Free @ 0x6bb46137
void Free() {
    State& s = S();
    TSA_Free(reinterpret_cast<int64_t*>(&s.rawSinceClothRisk));
    TSA_Free(reinterpret_cast<int64_t*>(&s.rawSinceBackLightShift));
    TSA_Free(reinterpret_cast<int64_t*>(&s.rawSinceDim2SsNoSignal));
}

// ToeSynaBl_GetSidePrpt @ 0x6bb471b0
// 两侧列 raw 有变化时重新抓取；侧边 raw 受保护的时间离现在超过 1 s（或 2 s）即排除侧边。
void GetSidePrpt() {
    if (static_cast<char>(Sys::IsSafeBaselineApplied()) != 0 || !Enabled()) return;
    TsaStatic& stat = Static();
    const uint8_t cols = PrmtFlash().dim1Len;
    const uint8_t side = SideTouch() ? 1 : 0;
    const auto far = static_cast<uint8_t>(cols - side - 1);
    if (IsColumnRawChanged(side) || IsColumnRawChanged(far)) {
        CopyChecked(Buffers().rawCaptureNextSide, GridBufSize() * size_t{2}, Buffers().preCmfRaw,
                    PrmtFlash().sensorNum * size_t{2});
        stat.toeSideRawTime = stat.curTimeMs;
        stat.toeSideRawTime2 = stat.curTimeMs;
    }
    const uint32_t a = IsNextSideDifNormal(side);
    const uint32_t b = IsNextSideDifNormal(far);
    if (PrptSideRawProtected(a, b)) stat.toeSideRawTime = stat.curTimeMs;
    if (PrptSideRawProtected2(cols, side, a, b)) stat.toeSideRawTime2 = stat.curTimeMs;
    S().excludeSide = !(stat.curTimeMs - stat.toeSideRawTime < 1001 && stat.curTimeMs - stat.toeSideRawTime2 < 2001);
}

// ToeSyna_PorpertyGetMaxSig @ 0x6bb47483
int16_t PropertyMaxSig(int16_t v) {
    if (S().excludeSide == 0) return v;
    return SideTouch() ? Prpt().difInner.maxSig : Prpt().difCore.maxSig;
}

// ToeSyna_PorpertyGetMinSig @ 0x6bb47412
int16_t PropertyMinSig(int16_t v) {
    if (static_cast<char>(Sys::IsSafeBaselineApplied()) != 0 || S().excludeSide == 0) return v;
    return SideTouch() ? Prpt().difInner.minSig : Prpt().difCore.minSig;
}

// ToeSynaBl_BackLightShiftUnit @ 0x6bb45dd1
// 第 2..倒数第 3 列逐列沿 Dim2 看 Dif：相邻两行跳变超过 0xaa，或间隔 5 行的升 / 降格数都不到 percent%，
// 这一列计为平缓。平缓的列不到 5 列时判为背光偏移。
bool BackLightShiftUnit(uint16_t percent) {
    if (!Flag(kFlagBackLight) || BackLightShiftCheckTouch()) return false;
    const FlashPrmt& fl = PrmtFlash();
    const int limit = static_cast<int>(static_cast<uint32_t>(fl.dim2Len - 9) * percent) / 100;
    uint16_t flat = 0;
    for (uint16_t col = 2; col < fl.dim1Len - 2; ++col) {
        uint16_t n = 0;
        int up = 0;
        int down = 0;
        int16_t prev = DifAt(static_cast<uint8_t>(col), 2);
        for (uint16_t row = 3; row < fl.dim2Len - 2; ++row) {
            ++n;
            const int16_t v = Buffers().dif[static_cast<uint16_t>(col + fl.dim1Len * row)];
            if (0xaa < v - prev || v - prev < -0xaa) {
                ++flat;
                break;
            }
            if (4 < n) {
                const int g = v - DifAt(static_cast<uint8_t>(col), static_cast<uint8_t>(row - 5));
                if (0x1f <= g) {
                    ++up;
                } else if (g < -0x1e) {
                    ++down;
                }
            }
            prev = v;
        }
        if (up < limit && down < limit) ++flat;
    }
    return flat < 5;
}

// ToeSynaBl_CheckNumOverNoise @ 0x6bb45640
bool CheckNumOverNoise() { return Enabled() && Prpt().raw2Raw.overLtiCount <= 4; }

// ToeSynaBl_IsBaselineAbnormalAftScreenOn @ 0x6bb465b4
// 亮屏后自容的列、行统计都偏高、超噪声格少时看背光偏移，持续 0x14e ms 即复位。
bool IsBaselineAbnormalAftScreenOn() {
    if (IsGame()) return false;
    TsaStatic& stat = Static();
    const FlashPrmt& fl = PrmtFlash();
    bool active = false;
    if (Enabled() && Flag(kFlagBackLight)) {
        const SelfData& self = SelfForBlReset();
        uint16_t cols = 0;
        uint16_t rows = 0;
        for (uint16_t c = 0; c < fl.dim1Len; ++c) {
            if (600 < self.strongAbsSum[c]) ++cols;
        }
        for (uint16_t r = 0; r < fl.dim2Len; ++r) {
            if (0x96 < self.innerStrongAbsSum[fl.dim1Len + r]) ++rows;
        }
        const TsaPrpt& prpt = Prpt();
        active = fl.dim1Len * 2u / 3 < cols && fl.dim2Len >> 1 < rows && 3 < prpt.field180 && 3 < prpt.field182 &&
                 B::Reset::CheckNumOverNoise();
    }
    if (active) {
        BackLightShiftProcess();
        ClothRiskProcess();
    } else {
        stat.toeBackLightShiftTimer = 0;
        stat.toeClothRiskTimer = 0;
    }
    if (0x1388 < stat.toeClothRiskTimer) {
        stat.toeClothRiskTimer = 0;
        return true;
    }
    if (stat.toeBackLightShiftTimer < 0x14e) return false;
    stat.toeBackLightShiftTimer = 0;
    return true;
}

// ToeSynaBl_IsBaselineAbnormal @ 0x6bb4782f（IsSSNotMatchWithMutual 依赖被裁掉的 SS，恒不成立）
void IsBaselineAbnormal(bool skipShape, bool protected2, bool& abnormal) {
    if (SideTouch() && IsAbnormalTouchShape(skipShape)) abnormal = true;
    Static().toeSsNotMatchTimer = 0;
    if (GripRegionAbnormal(protected2)) abnormal = true;
    if (IsLongTimeSsNoSignal() && Static().curTimeMs < Static().msInitTime + 60000) {
        const char* name = TSA_GetPrmtName();
        if (std::strncmp(name, "P150", 4) == 0 || std::strncmp(name, "P186", 4) == 0 || std::strncmp(name, "P17392", 6) == 0 ||
            std::strncmp(name, "P17292", 6) == 0) {
            abnormal = true;
        }
    }
}

// ToeSynaBl_IsTouchToBeProtected2 @ 0x6bb45675
bool IsTouchToBeProtected2(bool needTwo, bool fallback) {
    return Enabled() ? B::Reset::IsTouchToBeProtected2(needTwo) : fallback;
}

// ToeSynaBl_IsWaterDetected @ 0x6bb45a50
// 自容第 6 段比第 4 段高出 0x4b0 的列里，邻近 SS Dif 平静的列是不平静的 3 倍以上，且 Z8 检查成立，持续超过 0x50 ms。
bool IsWaterDetected() {
    if (!Enabled() || !Flag(kFlagWater)) return false;
    TsaStatic& stat = Static();
    const FlashPrmt& fl = PrmtFlash();
    const SelfData& self = SelfForBlReset();
    const int32_t* ss = SsDifForBl();
    uint8_t active = 0;
    uint8_t quiet = 0;
    for (uint16_t c = 2; c < fl.dim1Len - 2; ++c) {
        if (self.strongAbsSum[c] <= self.strongSum[c] + 0x4b0) continue;
        bool near = false;
        for (int k = -1; k < 2; ++k) {
            const int32_t v = ss[c + k];
            if (v < -400 || 400 < v) {
                near = true;
                break;
            }
        }
        if (near) {
            ++active;
        } else {
            ++quiet;
        }
    }
    if (active * 3u < quiet && IsWaterDetectedZ8Check()) {
        stat.toeWaterTimer = B::UpdateTimer(stat.toeWaterTimer);
    } else {
        stat.toeWaterTimer = 0;
    }
    if (stat.toeWaterTimer < 0x51) return false;
    stat.toeWaterTimer = 0;
    return true;
}

}  // namespace Solvers::Tsa::Core::ToeSynaBl

// raw 的 TSA / TSABuffer / HardwareAnalyzer 还在调用的入口
extern "C" {
namespace T = Solvers::Tsa::Core::ToeSynaBl;
uint64_t ToeSynaBl_Malloc(void) { return T::Malloc() ? 1 : 0; }
void ToeSynaBl_Free(void) { T::Free(); }
void ToeSynaBl_GetSidePrpt(void) { T::GetSidePrpt(); }
uint64_t ToeSynaBl_BackLightShiftUnit(uint16_t percent) { return T::BackLightShiftUnit(percent) ? 1 : 0; }
}  // extern "C"