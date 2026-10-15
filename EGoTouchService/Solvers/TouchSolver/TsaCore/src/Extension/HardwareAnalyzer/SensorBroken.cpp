// 坏点检测（原厂 src/Extension/HardwareAnalyzer/HardwareAnalyzer_SensorBroken.c）：
// - raw：raw 的最大 / 最小格超出 raw 上下限且与相邻格之差越限；BadChannel 另查；
// - DifZ8：用特殊门限的两格 Dif 远大于周围 8 格（且 raw 偏离周围均值），持续约 0x683 ms 确认；
// - Dif（只在 Full Incell、基线与公共噪声都干净时）：强信号格的十字邻格若孤立地凹陷（Type3）、
//   或格本身信号落在可疑区间（Type4），防抖 5 帧后记为坏点并报给 TPSensor。

#include <cstdlib>
#include <cstring>

#include "Alg/Touch/TouchData.h"
#include "Alg/TpSensor/TpSensor.h"
#include "App/Sys.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Common/Assert.h"
#include "Extension/HardwareAnalyzer/HardwareAnalyzerInternal.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"
#include "WorkAround/BadChannel/BadChannel.h"

namespace Solvers::Tsa::Core::HardwareAnalyzer {

namespace {

constexpr uint32_t kFeat2BadChannel = 0x08;        // TsaStatic::featureFlags2
constexpr uint32_t kFeat2BaliSensorBroken = 0x80;  // BaliSensorBroken（被裁掉）接管坏点检测
constexpr uint8_t kBrokenByDif = 4;                // TPSensor 坏点类型
constexpr int8_t kTypeNone = -1;
constexpr int64_t kZ8ConfirmMs = 0x683;
constexpr int64_t kZ8Confirmed = 5000;
constexpr uint8_t kMaxZ8Candidates = 5;
constexpr uint32_t kBlPropDirty = 0x198;           // TsaPrpt::blPropertyFlags 里基线不干净的位

/// g_stepDelta：十字方向的四个邻格。
constexpr int kCrossDim1[4] = {-1, 1, 0, 0};
constexpr int kCrossDim2[4] = {0, 0, -1, 1};

uint16_t Cell(uint32_t dim1, uint32_t dim2) { return static_cast<uint16_t>(dim1 + dim2 * PrmtFlash().dim1Len); }

// HardwareAnalyzer_IsSensorOverRawLimit @ 0x6baf2c5a
bool IsSensorOverRawLimit(uint8_t dim1, uint8_t dim2) {
    const FlashConstPrmt& fc = PrmtFlashConst();
    const auto v = static_cast<uint16_t>(Buffers().raw[Cell(dim1, dim2)]);
    return v < fc.field05C || fc.field05A < v;
}

// HardwareAnalyzer_IsSensorOverRawDeltaLimit @ 0x6baf2cd7
// 与四个邻格的 raw 之差有一个超出 (field060, field05E) 即为越限。
bool IsSensorOverRawDeltaLimit(uint8_t dim1, uint8_t dim2) {
    const FlashPrmt& fl = PrmtFlash();
    const FlashConstPrmt& fc = PrmtFlashConst();
    const int16_t* raw = Buffers().raw;
    const auto hi = static_cast<int16_t>(fc.field05E);
    const auto lo = static_cast<int16_t>(fc.field060);
    const uint32_t cell = dim1 + dim2 * static_cast<uint32_t>(fl.dim1Len);
    const int16_t v = raw[static_cast<uint16_t>(cell)];
    bool over = false;
    const auto check = [&](uint32_t neighbor) {
        const auto d = static_cast<int16_t>(v - raw[static_cast<uint16_t>(neighbor)]);
        if (hi < d || d < lo) over = true;
    };
    if (static_cast<uint8_t>(dim1 - 1) < fl.dim1Len && dim2 < fl.dim2Len) check(cell - 1);
    if (static_cast<uint8_t>(dim1 + 1) < fl.dim1Len && dim2 < fl.dim2Len) check(cell + 1);
    if (dim1 < fl.dim1Len && static_cast<uint8_t>(dim2 - 1) < fl.dim2Len) check(dim1 + (dim2 - 1u) * fl.dim1Len);
    if (dim1 < fl.dim1Len && static_cast<uint8_t>(dim2 + 1) < fl.dim2Len) check(dim1 + (dim2 + 1u) * fl.dim1Len);
    return over;
}

// HardwareAnalyzer_IsSensorBrokenByRawRange @ 0x6baf2f76
bool IsSensorBrokenByRawRange(uint8_t dim1, uint8_t dim2) {
    return IsSensorOverRawLimit(dim1, dim2) && IsSensorOverRawDeltaLimit(dim1, dim2);
}

/// HardwareAnalyzer_Z8InfoUpdate 的累加量。
struct Z8Info {
    uint8_t count = 0;
    uint32_t rawSum = 0;
    int difAbsSum = 0;
};

// HardwareAnalyzer_Z8InfoUpdate @ 0x6baf24b2
void Z8InfoUpdate(uint8_t dim1, uint8_t dim2, Z8Info& z) {
    const FlashPrmt& fl = PrmtFlash();
    if (dim1 >= fl.dim1Len || dim2 >= fl.dim2Len) return;
    const uint16_t i = Cell(dim1, dim2);
    ++z.count;
    z.rawSum += static_cast<uint16_t>(Buffers().raw[i]);
    z.difAbsSum += std::abs(Buffers().dif[i]);
}

// HardwareAnalyzer_IsSensorBrokenByDifZ8 @ 0x6baf258e
// |Dif| 超过 peakFilterReferenceThreshold 且超过周围 8 格 |Dif| 之和的 10 倍；raw 已统一到目标值时，
// 还要求 raw 偏离周围 raw 均值超过 |Dif| 的一半。
bool IsSensorBrokenByDifZ8(uint8_t dim1, uint8_t dim2) {
    const uint16_t i = Cell(dim1, dim2);
    const int a = std::abs(Buffers().dif[i]);
    if (a <= PrmtFlash().peakFilterReferenceThreshold) return false;
    Z8Info z;
    for (const int d1 : {-1, 0, 1}) {
        for (const int d2 : {-1, 0, 1}) {
            if (d1 != 0 || d2 != 0) Z8InfoUpdate(static_cast<uint8_t>(dim1 + d1), static_cast<uint8_t>(dim2 + d2), z);
        }
    }
    const uint32_t rawAvg = z.count == 0 ? 0 : z.rawSum / z.count;
    if (a <= z.difAbsSum * 10) return false;
    if (!AFE_IsRawUnifiedToTargetValue()) return true;
    const uint32_t d = static_cast<uint16_t>(Buffers().raw[i]) - rawAvg;
    return a / 2 < std::abs(static_cast<int>(d));
}

// HardwareAnalyzer_DebounceBrokenSensorByDifZ8 @ 0x6baf2804
// 已在候选表里的格计时加两帧的时长，到 0x683 ms 置为已确认；不在表里的加入（最多 5 个）。
void DebounceBrokenSensorByDifZ8(uint8_t dim1, uint8_t dim2) {
    TsaPrpt& prpt = Prpt();
    const int16_t step = Static().frameIntervalMs;
    bool found = false;
    for (uint8_t k = 0; k < prpt.brokenSensorNum; ++k) {
        BrokenSensorZ8& s = prpt.brokenSensor[k];
        if (s.dim1 != dim1 || s.dim2 != dim2) continue;
        found = true;
        if (s.timerMs < kZ8ConfirmMs) {
            s.timerMs += step * 2;
        } else if (s.timerMs != kZ8Confirmed) {
            s.timerMs = kZ8Confirmed;
            prpt.brokenByDifZ8 = 1;
        }
    }
    if (found || prpt.brokenSensorNum >= kMaxZ8Candidates) return;
    BrokenSensorZ8& s = prpt.brokenSensor[prpt.brokenSensorNum];
    s.timerMs = step * 2;
    s.dim1 = dim1;
    s.dim2 = dim2;
    ++prpt.brokenSensorNum;
}

// HardwareAnalyzer_BrokenSensorStatByDifZ8 @ 0x6baf2a26
// 查两个特殊门限格；未确认的候选计时逐帧递减，到 0 移出候选表。
void BrokenSensorStatByDifZ8() {
    TsaPrpt& prpt = Prpt();
    prpt.brokenByDifZ8 = 0;
    if (IsSensorBrokenByDifZ8(prpt.dif.maxDim1, prpt.dif.maxDim2)) {
        DebounceBrokenSensorByDifZ8(prpt.dif.maxDim1, prpt.dif.maxDim2);
    }
    if (IsSensorBrokenByDifZ8(prpt.dif.minDim1, prpt.dif.minDim2)) {
        DebounceBrokenSensorByDifZ8(prpt.dif.minDim1, prpt.dif.minDim2);
    }
    for (auto k = static_cast<int8_t>(prpt.brokenSensorNum - 1); k >= 0; --k) {
        int64_t& t = prpt.brokenSensor[k].timerMs;
        if (0 < t && t != kZ8Confirmed) t -= Static().frameIntervalMs;
        if (t >= 1) continue;
        for (int j = k; j < prpt.brokenSensorNum - 1; ++j) {
            std::memcpy(&prpt.brokenSensor[j], &prpt.brokenSensor[j + 1], sizeof(BrokenSensorZ8));
        }
        --prpt.brokenSensorNum;
    }
}

// HardwareAnalyzer_IsSafeForCheckDif @ 0x6baf31fc
bool IsSafeForCheckDif() {
    if (PrmtFlash().difBrokenCheck == 0 || !FullIncell()) return false;
    const uint32_t ex = Touches().exceptionFlags;
    if (ex != 0 && ex != 0x400) return false;
    const TsaPrpt& prpt = Prpt();
    const int limit = PrmtFlash().peakFilterReferenceThreshold / 20;
    if (limit < std::abs(prpt.cmfNoiseMin) || limit < std::abs(prpt.cmfNoiseMax)) return false;
    return (prpt.blPropertyFlags & kBlPropDirty) == 0;
}

// HardwareAnalyzer_DebounceBrokenSensorByDif @ 0x6baf354a
// 判为坏点（type 1）连续 5 次后置坏点，判为正常（type 0）连续 5 次后解除。
void DebounceBrokenSensorByDif(uint16_t i, int8_t type) {
    Counters& c = Cnt();
    uint8_t& deb = c.difDebounce[i];
    uint8_t& state = c.difState[i];
    const bool keep = state == 0 ? type == 0 : type == 1;
    if (keep) {
        deb = 0;
        return;
    }
    ++deb;
    if (4 < static_cast<int8_t>(deb)) {
        state = state == 0 ? 1 : 0;
        deb = 0;
    }
}

// HardwareAnalyzer_DetectType3 @ 0x6baf32fa
// 本格信号弱（|Dif| × 5 不超过参考门限），十字邻格至少两个明显更强，且 5×5 内没有明显负信号。
int8_t DetectType3(uint8_t dim1, uint8_t dim2) {
    const int16_t* dif = Buffers().dif;
    const int a = std::abs(dif[Cell(dim1, dim2)]);
    const int ref = PrmtFlash().peakFilterReferenceThreshold;
    if (ref < a * 5) return kTypeNone;
    int strong = 0;
    for (int k = 0; k < 4; ++k) {
        const int n1 = kCrossDim1[k] + dim1;
        const int n2 = kCrossDim2[k] + dim2;
        if (TpSensor::IsSensorValid(n1, n2) && a + ref / 2 < dif[Cell(n1, n2)]) ++strong;
    }
    if (strong < 2 || ref * strong <= a * 20) return kTypeNone;
    for (int d1 = -2; d1 < 3; ++d1) {
        for (int d2 = -2; d2 < 3; ++d2) {
            if (d1 == 0 && d2 == 0) continue;
            if (TpSensor::IsSensorValid(d1 + dim1, d2 + dim2) && dif[Cell(d1 + dim1, d2 + dim2)] * 15 < -ref) return kTypeNone;
        }
    }
    return 1;
}

// HardwareAnalyzer_DetectType4 @ 0x6baf37f5
// |Dif| 不超过 ref/2 时不判；(ref/2, 6ref/5] 判为正常；超过 2ref 判为坏点；其间不判。
int8_t DetectType4(uint8_t dim1, uint8_t dim2) {
    const int a = std::abs(Buffers().dif[Cell(dim1, dim2)]);
    const int ref = PrmtFlash().peakFilterReferenceThreshold;
    if (a * 5 <= ref || a * 2 <= ref) return kTypeNone;
    if (a * 5 <= ref * 6) return 0;
    return ref * 2 < a ? 1 : kTypeNone;
}

// HardwareAnalyzer_CheckCrossAround @ 0x6baf364b
void CheckCrossAround(uint8_t dim1, uint8_t dim2) {
    const FlashPrmt& fl = PrmtFlash();
    uint8_t* checked = Cnt().difChecked;
    for (int k = 0; k < 4; ++k) {
        const auto n1 = static_cast<uint32_t>(dim1 + kCrossDim1[k]);
        const auto n2 = static_cast<uint32_t>(dim2 + kCrossDim2[k]);
        if (static_cast<uint8_t>(n1) >= fl.dim1Len || static_cast<uint8_t>(n2) >= fl.dim2Len) continue;
        const auto i = static_cast<uint16_t>(fl.dim1Len * static_cast<int16_t>(n2) + static_cast<int16_t>(n1));
        if (checked[i] != 0) continue;
        checked[i] = 1;
        const int8_t type = DetectType3(static_cast<uint8_t>(n1), static_cast<uint8_t>(n2));
        if (type != kTypeNone) DebounceBrokenSensorByDif(i, type);
    }
}

// HardwareAnalyzer_CopyBadChannelList @ 0x6baf38c1
// 坏点报给 TPSensor；某行报失败（坏点表满）时跳过该行余下的格。
void CopyBadChannelList() {
    const FlashPrmt& fl = PrmtFlash();
    const uint8_t* state = Cnt().difState;
    for (uint8_t r = 0; r < fl.dim2Len; ++r) {
        for (uint8_t c = 0; c < fl.dim1Len; ++c) {
            if (state[Cell(c, r)] != 1) continue;
            Prpt().brokenByDif = 1;
            if (!TpSensor::AddToBrokenSensorList(c, r, kBrokenByDif)) break;
        }
    }
}

}  // namespace

// HardwareAnalyzer_SensorBrokenReset @ 0x6baf2420
void SensorBrokenReset() { ClearChecked(Cnt().difChecked, GridBufSize(), GridBufSize()); }

// HardwareAnalyzer_SensorBrokenInit @ 0x6baf3755
// 按 Dif 的坏点状态只在第一次初始化时清零（同时清 TPSensor 的坏点类型）。
void SensorBrokenInit() {
    SensorBrokenReset();
    Counters& c = Cnt();
    if (c.initOnce == 0) {
        c.initOnce = 1;
        uint8_t* flags = TpSensor::Flags();
        for (int i = 0; i < PrmtFlash().sensorNum; ++i) {
            c.difDebounce[i] = 0;
            c.difState[i] = 0;
            flags[i] &= static_cast<uint8_t>(~TpSensor::kFlagBrokenMask);
        }
    }
    Prpt().brokenByDif = 0;
}

// HardwareAnalyzer_SensorBrokenRawProcess @ 0x6baf2fbe（BaliSensorBroken 被裁掉）
// CMF 后超噪声的格少、且（Full Incell 或未插充电器）时，查 raw 最大 / 最小的格。
void SensorBrokenRawProcess() {
    const FlashPrmt& fl = PrmtFlash();
    const TsaPrpt& prpt = Prpt();
    const uint32_t f2 = Static().featureFlags2;
    if (prpt.raw2Raw.overNoiseAfterCmf < 5 && (FullIncell() || !Sys::IsChargerPluggedNoDebounce()) &&
        fl.rawUpperLimit != 0 && (f2 & kFeat2BaliSensorBroken) == 0 && fl.rawLowerLimit != 0) {
        if (IsSensorBrokenByRawRange(prpt.boundaryPositiveDim1, prpt.boundaryPositiveDim2)) SetException(kExcSensorBrokenByRaw);
        if (IsSensorBrokenByRawRange(prpt.boundaryNegativeDim1, prpt.boundaryNegativeDim2)) SetException(kExcSensorBrokenByRaw);
    }
    if ((f2 & kFeat2BadChannel) != 0) {
        if (BadChannel::IsDmdDetected()) SetException(kExcSensorBrokenByRaw);
        BadChannel::Process();
    }
}

// HardwareAnalyzer_SensorBrokenPreDifProcess @ 0x6baf3179
void SensorBrokenPreDifProcess() {
    if ((Static().featureFlags2 & kFeat2BaliSensorBroken) != 0) return;
    BrokenSensorStatByDifZ8();
    if (Prpt().brokenByDifZ8 != 0) SetException(kExcSensorBrokenByDif);
    if (Prpt().brokenByDif != 0) SetException(kExcSensorBrokenByDif);
}

// HardwareAnalyzer_SensorBrokenDifProcess @ 0x6baf3975
// 只看 Dif 不低于 max(ref/2, 90% 正向极值) 的有效格（且负向极值不太深）的十字邻格。
void SensorBrokenDifProcess() {
    TsaPrpt& prpt = Prpt();
    prpt.brokenByDif = 0;
    if (!IsSafeForCheckDif()) return;
    const FlashPrmt& fl = PrmtFlash();
    const int16_t* dif = Buffers().dif;
    const int ref = fl.peakFilterReferenceThreshold;
    int thold = ref / 2;
    const int peak = prpt.dif.maxSig * 0x5a / 100;
    if (thold < peak) thold = peak;
    for (uint8_t c = 0; c < fl.dim1Len; ++c) {
        for (uint8_t r = 0; r < fl.dim2Len; ++r) {
            if (-ref <= prpt.dif.minSig * 8 && thold <= dif[Cell(c, r)] && TpSensor::IsSensorValid(c, r)) {
                CheckCrossAround(c, r);
            }
        }
    }
    for (uint8_t c = 0; c < fl.dim1Len; ++c) {
        for (uint8_t r = 0; r < fl.dim2Len; ++r) {
            const int8_t type = DetectType4(c, r);
            if (type != kTypeNone) DebounceBrokenSensorByDif(static_cast<uint16_t>(c + r * fl.dim1Len), type);
        }
    }
    CopyBadChannelList();
}

}  // namespace Solvers::Tsa::Core::HardwareAnalyzer
