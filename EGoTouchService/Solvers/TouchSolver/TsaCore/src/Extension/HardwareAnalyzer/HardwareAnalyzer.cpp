#include "App/Property.h"
#include "App/Sys.h"
#include "Extension/HardwareAnalyzer/HardwareAnalyzer.h"

#include <cstdlib>
#include <cstring>

#include "Alg/Baseline/Baseline.h"
#include "Alg/Cmf/Cmf.h"
#include "Alg/TpSensor/TpSensor.h"
#include "Alg/Touch/TouchData.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Common/Assert.h"
#include "Extension/HardwareAnalyzer/HardwareAnalyzerInternal.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"
#include "WorkAround/BadChannel/BadChannel.h"
#include "WorkAround/RfNoise/RfNoise.h"
#include "WorkAround/ToeSynaBl/ToeSynaBl.h"

namespace Solvers::Tsa::Core::HardwareAnalyzer {

namespace {

constexpr uint32_t kSysDifBufPath = 0x10000;        // TsaStatic::systemFlags
constexpr uint32_t kFeat2BadChannel = 0x08;          // TsaStatic::featureFlags2
constexpr uint32_t kFeat2RfNoise = 0x10;
constexpr uint32_t kSpecialHwException = 0x10;       // TsaStatic::specialControlFlags：本帧有异常要上报
constexpr uint32_t kSpecialCtrlException = 0x400;
constexpr uint32_t kTouchExcHw = 0x400;              // TouchTable::exceptionFlags
constexpr uint32_t kTouchExcRecal = 0x1000;
constexpr uint32_t kTouchExcBaselineReset = 0x2000;
constexpr uint32_t kTouchStateBadEllipse = 0x40;     // TouchTable::stateFlags：核心触点椭圆度持续偏大
constexpr uint32_t kAfeRawJump = 0x20000;            // TsaStatic::afeStatus
constexpr uint32_t kAfeRawUnstable = 0x40000;
constexpr uint32_t kPersistReported = 0x8;           // TouchUnit::persistFlags
constexpr uint32_t kPersistSmallTouch = 0x20000000;
constexpr uint32_t kCleanBaselineDone = 0x10;        // TsaStatic::cleanState
constexpr uint8_t kMaxLineLen = 0x50;

/// 上报顺序：同一帧有多个新异常时只报最靠前的一个。
constexpr uint8_t kReportOrder[kExceptionNum] = {
    6, 7, 1, 2, 0x10, 0xe, 9, 4, 10, 5, 0xb, 8, 0xc, 0xd, 0xf, 0x11,
    0x12, 0x13, 3, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
};
/// g_numRepeatToReport：各异常连续检出几次才上报（下标为异常码 - 1）。
constexpr uint8_t kRepeatToReport[kExceptionNum] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
};

uint32_t& TouchExceptionFlags() { return Touches().exceptionFlags; }

// HardwareAnalyzer_ClearExceptionDescription @ 0x6baefab0
void ClearExceptionDescription() {
    State& st = St();
    ClearChecked(st.description, sizeof(st.description), sizeof(st.description));
    st.descriptionLen = 0;
}

// HardwareAnalyzer_CheckException @ 0x6baf0182
// 连续检出够次数才返回异常码，并重新计数。
int CheckException(uint8_t code) {
    const auto idx = static_cast<uint8_t>(code - 1);
    if (idx >= kExceptionNum) {
        TsaAssert(false);
        return 0;
    }
    uint8_t& n = Cnt().repeat[idx];
    ++n;
    if (n < kRepeatToReport[idx]) return 0;
    n = 0;
    return code;
}

// HardwareAnalyzer_GetNumOverNoiseAfterCMF @ 0x6baf101f（含 ..NoneSOF @ 0x6baf0ef7）
// 2D 滤波时整屏按噪声门限（至少 200）计；否则只在 Hybrid Incell 下计，两端列的门限放大到 4/3。挖孔格不计。
int16_t GetNumOverNoiseAfterCmf() {
    const FlashPrmt& fl = PrmtFlash();
    const int16_t* aft = Buffers().raw2RawDifAftCmf;
    const uint8_t* flags = TpSensor::Flags();
    const int16_t noise = Prpt().noiseThold;
    int16_t n = 0;
    if (Cmf::GetMethod() == 1) {
        const int16_t thold = noise < 200 ? int16_t{200} : noise;
        for (uint32_t i = 0; i < fl.sensorNum; ++i) {
            if ((flags[i] & TpSensor::kFlagNotch) == 0 && thold < std::abs(aft[i])) ++n;
        }
        return n;
    }
    if (fl.sensorArchitecture != 2) return 0;
    uint32_t i = 0;
    for (uint32_t r = 0; r < fl.dim2Len; ++r) {
        for (uint32_t c = 0; c < fl.dim1Len; ++c, ++i) {
            const int thold = c == 0 || c == fl.dim1Len - 1u ? noise * 4 / 3 : noise;
            if (thold < std::abs(aft[i]) && (flags[i] & TpSensor::kFlagNotch) == 0) ++n;
        }
    }
    return n;
}

// HardwareAnalyzer_ProcessRFNoise @ 0x6baf15ee
void ProcessRfNoise() {
    if ((Static().featureFlags2 & kFeat2RfNoise) != 0 && RfNoise::Detect()) SetException(kExcRfNoise);
}

// HardwareAnalyzer_ProcessByRaw2Raw @ 0x6baf173a
// 正负 Dif 之和相差不到 4 倍（整屏同向平移不算）时，超噪声的格过半判为 raw2raw 噪声。
void ProcessByRaw2Raw() {
    const uint32_t prevEx = PrevTouches().exceptionFlags;
    if (prevEx == 0x4000 || prevEx == 0x2000) return;
    const Raw2RawPrpt& r = Prpt().raw2Raw;
    const uint32_t n = PrmtFlash().sensorNum;
    const bool balanced = r.posSum < r.negSum * -4 && -r.negSum < r.posSum * 4;
    if (n <= r.overBigNoiseCount * 100u) SetException(kExcBigNoise);
    if (n >> 2 < r.overHtiCount) SetException(kExcRaw2RawNoise);
    if (Sys::IsChargerPlugged() == 0 || FullIncell()) {
        if (n >> 1 < r.overLtiCount) SetException(kExcRaw2RawNoise);
        const uint16_t over = FullIncell() ? r.overNoiseCount : r.overNoiseAfterCmf;
        if (n >> 1 < over && balanced) SetException(kExcRaw2RawNoise);
    } else if (n >> 1 < r.overLtiCount) {
        SetException(kExcRaw2RawNoise);
    }
}

// HardwareAnalyzer_RawLimitProcess @ 0x6baf070a
// raw 的跨度超过 2 倍范围、或最大 / 最小值超出中心 ± 1.5 倍范围。
void RawLimitProcess() {
    const FlashPrmt& fl = PrmtFlash();
    const TsaPrpt& prpt = Prpt();
    const uint16_t range = fl.recalRawRange;
    const uint32_t center = fl.recalRawCenter;
    const uint32_t half = (range * 3u / 2) & 0xffff;
    if (static_cast<uint16_t>(range * 2) < static_cast<int>(prpt.rawMax - prpt.rawMin)) SetException(kExcRawLimit);
    if (center + half < prpt.rawMax) SetException(kExcRawLimit);
    if (static_cast<int>(prpt.rawMin) < static_cast<int>(center - half)) SetException(kExcRawLimit);
}

// HardwareAnalyzer_PostProcessRaw @ 0x6baf2005
void PostProcessRaw() {
    const TsaStatic& stat = Static();
    if ((stat.systemFlags & kSysDifBufPath) == 0 && (Sys::IsChargerPlugged() == 0 || FullIncell())) RawLimitProcess();
    if (stat.rawJumpDetected != 0) SetException(kExcRaw2RawNoise);
    if (PrmtFlash().fingerThold < static_cast<uint16_t>(Property::MaxCmfNoise()) && FullIncell()) {
        SetException(kExcDisplayNoise);
    }
}

// HardwareAnalyzer_GetMaxEllipseRateInCore @ 0x6baf1c62
// 报过点、不贴边、非多指拆分的触点的最大椭圆度；超过 0x14 持续 25 ms 后置 kTouchStateBadEllipse。
void GetMaxEllipseRateInCore() {
    TouchTable& tt = Touches();
    tt.maxCoreEllipseRate = 0;
    tt.coreTouchCount = 0;
    for (int k = 0; k < tt.count; ++k) {
        const TouchUnit& u = tt.unit[k];
        if (u.field06E != 0 || u.field06F != 0 || u.zoneShape == 3 || (u.persistFlags & kPersistReported) == 0) continue;
        if (tt.maxCoreEllipseRate < u.ellipseRate) tt.maxCoreEllipseRate = u.ellipseRate;
        ++tt.coreTouchCount;
    }
    int64_t& timer = Cnt().ellipseRateTimerMs;
    if (tt.maxCoreEllipseRate < 0x15) {
        timer = 0;
    } else if (timer < 0x19) {
        timer += Static().frameIntervalMs;
    } else {
        tt.stateFlags |= kTouchStateBadEllipse;
    }
}

// HardwareAnalyzer_PostProcessTouch @ 0x6baf1f82
void PostProcessTouch() {
    if (Prpt().blPropertyState == 9) SetException(kExcAbnormalShape);
    GetMaxEllipseRateInCore();
    if (FullIncell() && static_cast<uint8_t>(Sys::IsAftModeEnabled()) == 0 &&
        (Touches().stateFlags & kTouchStateBadEllipse) != 0) {
        SetException(kExcAbnormalShape);
    }
}

// HardwareAnalyzer_AfeRawJump @ 0x6baf0912
void AfeRawJump() {
    const TsaStatic& stat = Static();
    if ((stat.afeStatus & kAfeRawJump) == 0 || (stat.prevAfeStatus & kAfeRawJump) != 0) return;
    ++St().afeRawJumpCount;
    if (stat.cleanState == kCleanBaselineDone) SetException(kExcAfeRawJump);
}

// HardwareAnalyzer_AfeRawUnstableCoreDetect @ 0x6baf09e9
// raw 不稳定期间连续 5 帧有小而圆的核心触点。
bool AfeRawUnstableCoreDetect() {
    const TouchTable& tt = Touches();
    bool found = false;
    for (int k = 0; k < tt.count; ++k) {
        if ((tt.unit[k].persistFlags & kPersistSmallTouch) != 0 && tt.unit[k].ellipseRate < 0xc) {
            found = true;
            break;
        }
    }
    State& st = St();
    if (st.coreUnstableFrames < 5) {
        st.coreUnstableFrames = found ? static_cast<uint8_t>(st.coreUnstableFrames + 1) : 0;
        return false;
    }
    SetException(kExcAfeRawUnstable);
    return true;
}

// HardwareAnalyzer_AfeRawUnstableSideDetect @ 0x6baf0bc1（侧边触摸被裁掉，计数总被清零）
void AfeRawUnstableSideDetect() {
    State& st = St();
    if (st.sideUnstableFrames < 5) {
        st.sideUnstableFrames = 0;
        return;
    }
    SetException(kExcAfeRawUnstable);
}

// HardwareAnalyzer_AfeRawUnstable @ 0x6baf0c82
// 初始化 65 s 之后才出现的 raw 不稳定直接报；之后每帧看核心触点、再看侧边触点。
void AfeRawUnstable() {
    const TsaStatic& stat = Static();
    State& st = St();
    if ((stat.afeStatus & kAfeRawUnstable) != 0 && (stat.prevAfeStatus & kAfeRawUnstable) == 0) {
        ++st.afeRawUnstableCount;
        st.coreUnstableFrames = 0;
        st.sideUnstableFrames = 0;
        if (stat.msInitTime + 65000 < stat.curTimeMs) {
            SetException(kExcAfeRawUnstable);
            return;
        }
    }
    if ((stat.afeStatus & kAfeRawUnstable) == 0) {
        st.coreUnstableFrames = 0;
        st.sideUnstableFrames = 0;
    } else if (!AfeRawUnstableCoreDetect()) {
        AfeRawUnstableSideDetect();
    }
}

// HardwareAnalyzer_BackLightShift @ 0x6baf0dd9
void BackLightShift() {
    const TsaPrpt& prpt = Prpt();
    if (ToeSynaBl::BackLightShiftUnit(0x41) && PrmtFlash().sensorNum >> 2 < prpt.field184 &&
        PrmtDynamic().smallThold < prpt.dif.maxSig) {
        SetException(kExcBackLightShift);
    }
}

// HardwareAnalyzer_GetCouplingNoiseLevel @ 0x6baf20c4
void GetCouplingNoiseLevel() {
    int64_t& timer = Cnt().couplingNoiseTimerMs;
    if (Static().noiseLevel != 3) {
        timer = 0;
    } else if (timer < 2) {
        timer += Static().frameIntervalMs;
    } else {
        SetException(kExcCouplingNoise);
    }
}

// HardwareAnalyzer_LargeDisplayNoise @ 0x6baf0e82
void LargeDisplayNoise() {
    if (Static().allFreqNoisy != 0 && Sys::IsChargerPlugged() == 0) SetException(kExcDisplayNoise);
}

// PostProcess 里的环境温度异常：手套 / 指关节模式锁定、且按 zTholdFlags 启用时（智能皮套除外）。
// 只对 aftGateFlags bit3 或 HIMA 参数查。
void CheckEnvTemp() {
    const TsaStatic& stat = Static();
    if ((stat.aftGateFlags & 8) == 0 && std::strncmp(TSA_GetPrmtName(), "HIMA", 4) != 0) return;
    const uint8_t z = PrmtDynamic().zTholdFlags;
    const bool mode = ((z & 4) != 0 && (stat.featureFlags & kFeatGloveMode) != 0) ||
                      ((z & 8) != 0 && (stat.featureFlags & kFeatKnuckle) != 0);
    if (Prpt().touchModeLatched != 0 && mode && (stat.featureFlags & kFeatSmartCover) == 0) {
        SetException(kExcAbnormalBaseline);
    }
}

}  // namespace

void SetException(uint8_t code) { St().cur |= 1u << (code & 31); }

// HardwareAnalyzer_SetSoftException @ 0x6baf0124
void SetSoftException() { SetException(kExcSoft); }

// HardwareAnalyzer_Malloc @ 0x6baf2350
bool Malloc() {
    Counters& c = Cnt();
    c.difDebounce = static_cast<uint8_t*>(TSA_MallocAndInit(GridBufSize()));
    if (c.difDebounce == nullptr) return false;
    c.difState = static_cast<uint8_t*>(TSA_MallocAndInit(GridBufSize()));
    if (c.difState == nullptr) return false;
    c.difChecked = static_cast<uint8_t*>(TSA_MallocAndInit(GridBufSize()));
    return c.difChecked != nullptr;
}

// HardwareAnalyzer_Free @ 0x6baf23ed
void Free() {
    Counters& c = Cnt();
    TSA_Free(reinterpret_cast<int64_t*>(&c.difDebounce));
    TSA_Free(reinterpret_cast<int64_t*>(&c.difState));
    TSA_Free(reinterpret_cast<int64_t*>(&c.difChecked));
}

// HardwareAnalyzer_Init @ 0x6baefb27
void Init() {
    State& st = St();
    st.cur = 0;
    st.prev = 0;
    st.afeRawJumpCount = 0;
    st.afeRawUnstableCount = 0;
    st.coreUnstableFrames = 0;
    st.sideUnstableFrames = 0;
    ClearExceptionDescription();
    TsaPrpt& prpt = Prpt();
    constexpr size_t kBrokenBytes = offsetof(TsaPrpt, brokenSensor) + sizeof(TsaPrpt::brokenSensor) - offsetof(TsaPrpt, brokenSensorNum);
    ClearChecked(&prpt.brokenSensorNum, kBrokenBytes, kBrokenBytes);
    TsaBufferTable& buf = Buffers();
    const size_t gridBytes = GridBufSize() * size_t{2};
    ClearChecked(buf.raw2RawDif, gridBytes, gridBytes);
    ClearChecked(&prpt.raw2Raw, sizeof(Raw2RawPrpt), sizeof(Raw2RawPrpt));
    ClearChecked(buf.raw2RawDifAftCmf, gridBytes, gridBytes);
    const uint32_t f2 = Static().featureFlags2;
    if ((f2 & kFeat2RfNoise) != 0) RfNoise::Init();
    SensorBrokenInit();
    if ((f2 & kFeat2BadChannel) != 0) BadChannel::Init();
}

// HardwareAnalyzer_Reset @ 0x6baf2499
void Reset() {
    TpSensor::Reset();
    SensorBrokenReset();
}

// HardwareAnalyzer_GetRaw2RawPrpt @ 0x6baf10dc
// raw2raw Dif（raw - 上一帧 raw）按扫描线分组（FlashPrmt::field62C 是各线所属的组）统计极值与超门限的格数。
void GetRaw2RawPrpt() {
    const FlashPrmt& fl = PrmtFlash();
    const FlashConstPrmt& fc = PrmtFlashConst();
    TsaPrpt& prpt = Prpt();
    const int16_t noise = prpt.noiseThold;
    const int16_t ref = fl.peakFilterReferenceThreshold;
    const int16_t finger = fl.fingerThold;
    const auto bigNoise = static_cast<int16_t>(ref * 3);
    const uint8_t lineLen = fc.field024;
    const uint8_t lineNum = fc.field025;
    const uint8_t groupNum = fc.field026;
    TsaAssert(lineLen <= kMaxLineLen);
    RfNoise::SavePrevRaw2RawDif();
    TsaBufferTable& buf = Buffers();
    Baseline::CalcDiffCommon(buf.raw2RawDif, buf.raw, buf.preHwRaw);
    Cmf::ProcessDimCommonOnBuf(buf.raw2RawDifAftCmf, buf.raw2RawDif, fl.dim1Axis == 0 ? 2 : 1, 0x7fff,
                               PrmtDynamic().cmfThold.grad, fl.dim1Len, fl.dim2Len);
    Raw2RawPrpt r{};
    r.maxDif = INT16_MIN;
    r.minDif = INT16_MAX;
    int16_t total = 0;
    uint8_t maxPercent = 0;
    for (uint32_t g = 0; g < groupNum; ++g) {
        int over = 0;
        int lines = 0;
        for (uint32_t line = 0; line < lineNum; ++line) {
            if (fl.field62C[line] != g) continue;
            ++lines;
            for (uint32_t k = 0; k < lineLen; ++k) {
                const uint32_t i = fl.dim1Axis == 0 ? line + k * fl.dim1Len : k + line * fl.dim1Len;
                const int16_t v = buf.raw2RawDif[i];
                if (r.maxDif < v) r.maxDif = v;
                if (v < r.minDif) r.minDif = v;
                const int a = std::abs(v);
                if (a <= noise) continue;
                ++over;
                if (v < 1) {
                    r.negSum += v;
                } else {
                    r.posSum += v;
                }
                if (finger < a) {
                    ++r.overLtiCount;
                    if (ref < a) {
                        ++r.overHtiCount;
                        if (bigNoise < a) ++r.overBigNoiseCount;
                    }
                }
            }
        }
        total = static_cast<int16_t>(total + over);
        int cells = lineLen * lines;
        if (cells == 0) cells = 1;
        const int percent = over * 100 / cells;
        if (maxPercent < percent) maxPercent = static_cast<uint8_t>(percent);
    }
    const int16_t afterCmf = GetNumOverNoiseAfterCmf();
    Raw2RawPrpt& out = prpt.raw2Raw;
    out.maxDif = r.maxDif;
    out.minDif = r.minDif;
    out.overNoiseCount = static_cast<uint16_t>(total);
    out.overBigNoiseCount = r.overBigNoiseCount;
    out.overNoiseAfterCmf = static_cast<uint16_t>(afterCmf);
    out.overHtiCount = r.overHtiCount;
    out.overLtiCount = r.overLtiCount;
    out.posSum = r.posSum;
    out.negSum = r.negSum;
    out.maxOverNoisePercent = maxPercent;
}

// HardwareAnalyzer_Process @ 0x6baf19de（接近检测、侧边触摸被裁掉）
void Process() {
    TsaPrpt& prpt = Prpt();
    std::memcpy(&prpt.prevRaw2Raw, &prpt.raw2Raw, sizeof(Raw2RawPrpt));
    State& st = St();
    st.prev = st.cur;
    st.cur = 0;
    ClearExceptionDescription();
    GetRaw2RawPrpt();
    if (Sys::IsHwReset()) {
        Init();
    } else if ((Static().systemFlags & kSysDifBufPath) == 0) {
        SensorBrokenRawProcess();
        SensorBrokenPreDifProcess();
        ProcessRfNoise();
        ProcessByRaw2Raw();
    }
    const TsaBufferTable& buf = Buffers();
    CopyChecked(buf.preHwRaw, GridBufSize() * size_t{2}, buf.raw, PrmtFlash().sensorNum * size_t{2});
    ReportToTsa();
    TsaStatic& stat = Static();
    if (IsDetected(st.cur, kExcRfNoise)) {
        ++stat.rfNoiseFrames;
        if (stat.maxRfNoiseFrames < stat.rfNoiseFrames) stat.maxRfNoiseFrames = stat.rfNoiseFrames;
    } else {
        stat.rfNoiseFrames = 0;
    }
}

// HardwareAnalyzer_ProcessDif @ 0x6baf1c4e
void ProcessDif() { SensorBrokenDifProcess(); }

// HardwareAnalyzer_PostProcess @ 0x6baf2151（安全基线、EventAnalyzer 被裁掉）
void PostProcess() {
    if (!Sys::IsHwReset()) {
        PostProcessRaw();
        const uint32_t ex = TouchExceptionFlags();
        if ((ex & kTouchExcBaselineReset) != 0) SetException(kExcBaselineReset);
        if ((ex & kTouchExcRecal) != 0 && 0x32 < Static().recalFrames) SetException(kExcRecal);
        if ((Static().specialControlFlags & kSpecialCtrlException) != 0) SetException(kExcSpecialControl);
        CheckEnvTemp();
        PostProcessTouch();
        AfeRawJump();
        AfeRawUnstable();
        BackLightShift();
        GetCouplingNoiseLevel();
        LargeDisplayNoise();
    }
    ReportToTsa();
}

// HardwareAnalyzer_GetDetectedException @ 0x6baf0249
int GetDetectedException() {
    const State& st = St();
    const uint32_t fresh = st.cur & ~st.prev;
    int code = 0;
    for (const uint8_t c : kReportOrder) {
        if (IsDetected(fresh, c)) {
            code = CheckException(c);
            break;
        }
    }
    if (code != 0) Static().specialControlFlags |= kSpecialHwException;
    return code;
}

// HardwareAnalyzer_ReportToTSA @ 0x6baf159b
void ReportToTsa() {
    if (GetDetectedException() != 0) {
        TouchExceptionFlags() |= kTouchExcHw;
    } else {
        TouchExceptionFlags() &= ~kTouchExcHw;
    }
}

// HardwareAnalyzer_IsToBypassCurrentFrame @ 0x6baf14ff
bool IsToBypassCurrentFrame() {
    const uint32_t cur = St().cur;
    bool bypass = false;
    if (FullIncell() && IsDetected(cur, kExcRaw2RawNoise) && !IsDetected(cur, kExcRfNoise)) bypass = true;
    if (IsDetected(cur, kExcBigNoise)) bypass = true;
    if ((Static().featureFlags2 & kFeat2RfNoise) != 0 && RfNoise::IsDetected()) bypass = true;
    return bypass;
}

// HardwareAnalyzer_GetExceptionDescription @ 0x6baf0009
const char* GetExceptionDescription() { return St().description; }

}  // namespace Solvers::Tsa::Core::HardwareAnalyzer

// raw 的 TSA / TSABuffer / AFE / ASA 等还在调用的入口
extern "C" {
namespace HA = Solvers::Tsa::Core::HardwareAnalyzer;
uint64_t HardwareAnalyzer_Malloc(void) { return HA::Malloc() ? 1 : 0; }
void HardwareAnalyzer_Free(void) { HA::Free(); }
void HardwareAnalyzer_Init(void) { HA::Init(); }
void HardwareAnalyzer_Reset(void) { HA::Reset(); }
void HardwareAnalyzer_Process(void) { HA::Process(); }
void HardwareAnalyzer_ProcessDif(void) { HA::ProcessDif(); }
void HardwareAnalyzer_PostProcess(void) { HA::PostProcess(); }
void HardwareAnalyzer_GetRaw2RawPrpt(void) { HA::GetRaw2RawPrpt(); }
void HardwareAnalyzer_ReportToTSA(void) { HA::ReportToTsa(); }
void HardwareAnalyzer_SetSoftException(void) { HA::SetSoftException(); }
int HardwareAnalyzer_GetDetectedException(void) { return HA::GetDetectedException(); }
// 描述串靠 StrPrintf 拼写，StrPrintf 被裁掉后为空操作。
void HardwareAnalyzer_GenerateExceptionDescription(void) {}
uint8_t HardwareAnalyzer_IsToBypassCurrentFrame(void) { return HA::IsToBypassCurrentFrame() ? 1 : 0; }
const char* HardwareAnalyzer_GetExceptionDescription(void) { return HA::GetExceptionDescription(); }
uint64_t HardwareAnalyzer_IsNotOKToEnterIDLEMode(void) { return 0; }
}  // extern "C"
