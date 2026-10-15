// 基线复位判定（原 Baseline/BLReset.c，原厂 src/Alg/Baseline/BaselineReset.c）。
//
// 每帧由 BaselineStateMachine 的 GetProperty 调 Process：依次检查长时间 raw 跳变、唤醒后的异常触点、
// 智能皮套打开、基线异常、基线被污染、AFE raw 跳变等，任一成立即复位基线并记下原因（TsaPrpt::blPropertyState）。
// 同时维护「干净基线」状态机：无触摸稳定一段时间后把当前基线存进 cleanBaseline，作为判脏的参照。
//
// SS（自容）、安全基线、智能皮套、接近检测、NotchProtect 在本机型被裁掉，依赖它们的判定恒不成立，
// 这里只保留它们对状态的写入。

#include <algorithm>
#include <cstdlib>

#include "Alg/Baseline/Baseline.h"
#include "Alg/Baseline/BaselineInternal.h"
#include "Alg/Peak/PeakData.h"
#include "Alg/SelfData/SelfData.h"
#include "Alg/Touch/TouchData.h"
#include "Alg/TpSensor/TpSensor.h"
#include "App/Afe.h"
#include "App/Sys.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Common/Assert.h"
#include "Common/Image.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"
#include "WorkAround/ToeSynaBl/ToeSynaBl.h"

namespace Solvers::Tsa::Core::Baseline {

namespace {

/// TsaPrpt::blPropertyState：基线复位的原因
enum ResetReason : uint32_t {
    kReasonAllFreqInit = 4,
    kReasonWakeUp = 5,
    kReasonNoisy = 6,
    kReasonRawJump = 7,
    kReasonWakeUpException = 8,
    kReasonWakeUpState = 9,
    kReasonSmartCoverOff = 0xc,
    kReasonFreqHopping = 0xd,
    kReasonAbnormal = 0xf,
    kReasonAfeRawJump = 0x10,
    kReasonToeAftScreenOn = 0x12,
    kReasonDirty = 0x13,
};

/// TsaStatic::blDebugFlags：各判定的命中位
enum DebugFlag : uint32_t {
    kDbgProtected2 = 0x1,
    kDbgClean = 0x2,
    kDbgCleanState = 0x4,
    kDbgProtected = 0x8,
    kDbgAbnormal1 = 0x10,
    kDbgAbnormal2 = 0x20,
    kDbgAbnormal3 = 0x100,
    kDbgAfeRawJump = 0x400,
    kDbgNumOverNoise = 0x800,
    kDbgAfeSideTouch = 0x1000,
    kDbgDirty = 0x2000,
};

constexpr uint32_t kCtrlBlReset = 0x1;           // TsaStatic::specialControlFlags
constexpr uint32_t kAfeRawJump = 0x20000;         // TsaStatic::afeStatus
constexpr uint32_t kAfeRawUnstable = 0x40000;
constexpr uint32_t kAfeSideTouch = 0x80000;
constexpr uint32_t kAfeNoDirtyCheck = 0x100000;
constexpr uint32_t kPersistReported = 0x8;        // PrevTouchUnit::persistFlags
constexpr uint32_t kPersistSmallTouch = 0x20000000;
constexpr uint32_t kVolatileEdgeTouch = 0x20000;  // PrevTouchUnit::volatileFlags
constexpr uint32_t kVolatileAbnormalBl = 0x400000;
constexpr uint32_t kExceptionTouchMode = 0x2000;  // PrevTouchTable::exceptionFlags
constexpr uint32_t kStateFlagWakeUp = 0x10;       // PrevTouchTable::stateFlags
constexpr uint32_t kCleanDone = 0x10;             // TsaStatic::cleanState
constexpr uint64_t kAfterInitMs = 310000;         // 互容初始化 310 s 之后才看触点保护
constexpr uint64_t kTimerLimit = 300;
constexpr uint8_t kMaxTrustFrames = 0xf;
constexpr size_t kBlBytes = 0x2648;
constexpr uint8_t kMaxCmfLen = 0x50;              // TSA_MAX_DIM

void Debug(uint32_t bit) { Static().blDebugFlags |= bit; }
bool IsGame() { return static_cast<char>(TSA_IsGameScenario()) != 0; }
bool SideTouch() { return (Static().featureFlags & kFeatSideTouch) != 0; }
bool AfterInit(uint64_t ms) { return Static().msInitTime + ms <= Static().curTimeMs; }

/// 带帧间隔累加的复位计时：超过 limit 即成立并清零。
bool Expired(uint64_t& timer, uint64_t limit) {
    const bool r = limit < timer;
    if (r) timer = 0;
    return r;
}

/// 历史触点对应的峰不贴边（侧边触摸时两侧各让出一列）。
struct InnerPeak {
    uint8_t lastCol;
    uint8_t firstCol;
    InnerPeak() {
        const FlashPrmt& fl = PrmtFlash();
        firstCol = SideTouch() ? 1 : 0;
        lastCol = static_cast<uint8_t>(fl.dim1Len - 1 - firstCol);
    }
    bool operator()(const PrevTouchUnit& u) const {
        const PeakEntry& pk = Peaks().entry[u.peakIdx];
        return firstCol < pk.dim1 && pk.dim1 < lastCol && pk.dim2 != 0 && pk.dim2 < static_cast<uint8_t>(PrmtFlash().dim2Len - 1);
    }
};

bool SafeBaselineApplied() { return static_cast<char>(Sys::IsSafeBaselineApplied()) != 0; }

// BLReset_CheckAbnormal @ 0x6ba481e9
// 基线偏离基线参考值比 raw 偏离 raw 参考值更远。
bool CheckAbnormal(uint16_t raw, uint16_t bl) {
    const TsaPrpt& prpt = Prpt();
    return std::abs(static_cast<int>(raw - prpt.rawAvg)) < std::abs(static_cast<int>(bl - prpt.blAvg));
}

// BLReset_IsBaselineClean @ 0x6ba47e09
bool IsBaselineClean() {
    const bool r = Static().cleanState == kCleanDone;
    if (r) Debug(kDbgClean);
    return r;
}

// BLReset_IsTouchToBeProtected @ 0x6ba47b08（智能皮套被裁掉）
// 恰有一个（或两个）不贴边的历史触点，且全屏 incell，或自容 / 交叉位置的 Dif 也印证有触摸。
bool IsTouchToBeProtected() {
    const PrevTouchTable& prev = PrevTouches();
    const TsaPrpt& prpt = Prpt();
    const FlashPrmt& fl = PrmtFlash();
    const bool fullIncell = fl.sensorArchitecture == 3;
    bool r = false;
    if (prev.count == 1 && (prev.unit[0].volatileFlags & kVolatileEdgeTouch) == 0) {
        const SelfData& self = SelfBuf();
        r = fullIncell || ((prpt.field16C < self.sum[prpt.field170] || prpt.field16C < self.sum[prpt.field171 + fl.dim1Len]) &&
                           fl.fingerThold * 3 / 2 < prpt.field16C);
    } else if (prev.count == 2 &&
               ((prev.unit[0].volatileFlags & kVolatileEdgeTouch) == 0 || (prev.unit[1].volatileFlags & kVolatileEdgeTouch) == 0)) {
        // 两个触点交叉位置（一个的列、另一个的行）上的 Dif 都是明显的负值
        const Point& p0 = prev.unit[0].rptPos;
        const Point& p1 = prev.unit[1].rptPos;
        const int16_t* dif = Buffers().dif;
        const int half = prpt.dif.maxSig / 2;
        r = fullIncell || (half < -dif[GridIndex(static_cast<uint8_t>(p0.dim1 >> 8), static_cast<uint8_t>(p1.dim2 >> 8))] &&
                           half < -dif[GridIndex(static_cast<uint8_t>(p1.dim1 >> 8), static_cast<uint8_t>(p0.dim2 >> 8))] &&
                           -prpt.dif.negSum < prpt.dif.posSum);
    }
    if (r) Debug(kDbgProtected);
    return r;
}

// BLReset_CheckCleanBaseline @ 0x6ba4a9a3
// CMF 前的 raw 与干净基线相差超过 100 的格不到 4 个。
bool CheckCleanBaseline() {
    if (Static().cleanBaselineValid == 0) return false;
    const TsaBufferTable& buf = Buffers();
    const FlashPrmt& fl = PrmtFlash();
    uint16_t far = 0;
    uint16_t i = 0;
    for (uint16_t r = 0; r < fl.dim2Len; ++r) {
        for (uint16_t c = 0; c < fl.dim1Len; ++c, ++i) {
            const auto d = static_cast<int16_t>(buf.preCmfRaw[i] - buf.cleanBaseline[i]);
            if (100 < d || d < -100) ++far;
        }
    }
    return far < 4;
}

// BLReset_IsCurBaselineDirtyByDiff @ 0x6ba4aa9e
bool IsCurBaselineDirtyByDiff() {
    const TsaPrpt& prpt = Prpt();
    return PrmtFlash().sensorNum >> 2 < prpt.field186 && 4 < prpt.field182;
}

// BLReset_IsCurBaselineDirty @ 0x6ba4aaee
// 超噪声格少、当前基线与 Dif 都显示被污染而 raw 与干净基线一致时累计，超过 300 ms 复位。
bool IsCurBaselineDirty() {
    TsaStatic& stat = Static();
    if (PrmtFlash().blDirtyCheck == 0 || !Reset::CheckNumOverNoise() || IsGame()) {
        stat.dirtyTimer = 0;
        return false;
    }
    const bool protect = AfterInit(kAfterInitMs) && IsTouchToBeProtected();
    if ((stat.afeStatus & kAfeNoDirtyCheck) == 0 && !protect && IsCurBaselineDirtyByDiff() && CheckCleanBaseline()) {
        Debug(kDbgDirty);
        stat.dirtyTimer = UpdateTimer(stat.dirtyTimer);
    } else {
        stat.dirtyTimer = 0;
    }
    return Expired(stat.dirtyTimer, kTimerLimit);
}

// BLReset_IsTouchProtectOnRawJump @ 0x6ba48bb6
// 1..3 个报过点、离边缘超过一格的历史触点，其中有强峰且都不贴边，且上下偏移都不大。
bool IsTouchProtectOnRawJump() {
    const PrevTouchTable& prev = PrevTouches();
    const TsaPrpt& prpt = Prpt();
    const int ref = PrmtFlash().peakFilterReferenceThreshold;
    int n = 0;
    bool strong = false;
    bool edge = false;
    for (uint8_t i = 0; i < prev.count; ++i) {
        const PrevTouchUnit& u = prev.unit[i];
        if ((u.persistFlags & kPersistReported) == 0 || u.edgeDistDim1 <= 0x100 || u.edgeDistDim2 <= 0x100) continue;
        if (ref / 2 < PrevPeaks().entry[u.peakIdx].sig) strong = true;
        if ((u.volatileFlags & kVolatileEdgeTouch) != 0) edge = true;
        ++n;
    }
    return 1 <= n && n <= 3 && strong && !edge && std::abs(prpt.cmfNoiseMin) * 3 < ref && std::abs(prpt.cmfNoiseMax) * 3 < ref;
}

// BLReset_IsLongTimeRawJumpDetected @ 0x6ba48d9e
// 全屏 incell 时超噪声格过多即锁存 raw 跳变；锁存期间（非常噪声）超噪声格回落而大面积 raw 仍偏离、又没有要保护的触点时
// 累计超过 0x54 ms 即复位，否则按帧间隔回退，回退到 -0x2a 以下解除锁存。
bool IsLongTimeRawJumpDetected() {
    TsaStatic& stat = Static();
    const TsaPrpt& prpt = Prpt();
    const FlashPrmt& fl = PrmtFlash();
    if (fl.sensorArchitecture == 3) {
        const uint32_t limit = static_cast<uint32_t>(fl.sensorNum * 3) >> 2;
        if (limit < prpt.raw2Raw.overNoiseCount || limit < prpt.prevRaw2Raw.overNoiseCount) stat.rawJumpLatched = 1;
        if (0x5a < prpt.raw2Raw.maxOverNoisePercent || 0x5a < prpt.prevRaw2Raw.maxOverNoisePercent) stat.rawJumpLatched = 1;
    }
    if (stat.rawJumpLatched == 0 || Afe::IsAlwaysNoisy()) {
        stat.rawJumpTimer = 0;
        stat.rawJumpLatched = 0;
        stat.rawJumpDetected = 0;
        return false;
    }
    bool r = false;
    if (prpt.raw2Raw.overNoiseCount < 0x28 && fl.sensorNum >> 1 < prpt.preCmfOverCount) {
        if (!IsTouchProtectOnRawJump()) {
            if (stat.rawJumpTimer < 0x54) {
                AddInterval(stat.rawJumpTimer);
            } else {
                r = true;
                stat.rawJumpTimer = 0;
                stat.rawJumpLatched = 0;
            }
        }
    } else if (stat.rawJumpTimer < -0x29) {
        stat.rawJumpTimer = 0;
        stat.rawJumpLatched = 0;
    } else {
        SubInterval(stat.rawJumpTimer);
    }
    stat.rawJumpDetected = r;
    return r;
}

// BLReset_IsLongTimeSSNoSignal @ 0x6ba49544（SS 被裁掉：自容对基线无效）
void IsLongTimeSsNoSignal() { Static().ssNoSignal = 0; }

// BLReset_IsLongTimeSSNegativeSignal @ 0x6ba4995b（SS 被裁掉：判定恒不成立）
void IsLongTimeSsNegativeSignal() {
    TsaStatic& stat = Static();
    if (static_cast<char>(TSA_InStylusMode()) != 0 || !Reset::CheckNumOverNoise()) stat.ssNegSignalTimer = 0;
    stat.ssNegSignal = 0;
}

// BLReset_IsLongTimeSSNegativeSignalStable @ 0x6ba49b66（SS 被裁掉：判定恒不成立）
void IsLongTimeSsNegativeSignalStable() {
    TsaStatic& stat = Static();
    if (!(stat.curTimeMs < stat.msInitTime + 60000 && !IsGame() && Reset::CheckNumOverNoise())) stat.ssNegSignalStableTimer = 0;
    stat.ssNegSignalStable = 0;
}

// BLReset_IsTriggered2BySS @ 0x6ba4b890
// 自容的几项判定都依赖被裁掉的 SS，恒不触发；IsBaselineClean 仍按原顺序调用（它会记调试位）。
bool IsTriggered2BySs() {
    IsLongTimeSsNoSignal();
    IsBaselineClean();  // 之后的 IsMutualSSNotMatch 恒为 0
    if (!IsBaselineClean()) IsLongTimeSsNegativeSignal();
    if (PrmtFlash().ssNegSignalStableCheck != 0 && IsBaselineClean()) IsLongTimeSsNegativeSignalStable();
    return false;
}

// BLReset_IsAllTouchByAbnormalBaseline @ 0x6ba48425
bool IsAllTouchByAbnormalBaseline() {
    const PrevTouchTable& prev = PrevTouches();
    uint8_t n = 0;
    for (uint8_t i = 0; i < prev.count; ++i) {
        if ((prev.unit[i].volatileFlags & kVolatileAbnormalBl) != 0) ++n;
    }
    return prev.count == n && n != 0;
}

// BLReset_Check @ 0x6ba48939（IsToBeProtectedBySS 恒为 0）
// 负向统计明显大于正向（基线被按下时抬高）、或上一帧有唤醒标志，且没有要保护的触点；或全部触点都在异常基线上。
// 触摸模式异常（0x2000）出现后第一次也算。
bool Check() {
    const TsaPrpt& prpt = Prpt();
    bool r = false;
    if (Reset::CheckNumOverNoise()) {
        if ((prpt.field16C * 3 < prpt.field16E * -4 || prpt.field180 * 3u < prpt.field182 * 4u) &&
            prpt.field178 * 3 < prpt.field17C * -4 && prpt.field16C * 2 < prpt.field16E * -4 &&
            PrmtDynamic().smallThold < -prpt.field16E && 2 < prpt.field182) {
            r = !IsTouchToBeProtected();
        }
        if ((PrevTouches().stateFlags & kStateFlagWakeUp) != 0) r = !IsTouchToBeProtected();
        if (IsAllTouchByAbnormalBaseline()) r = true;
    }
    if (PrevTouches().exceptionFlags == kExceptionTouchMode) {
        if (Static().field3F1 == 0) r = true;
    } else {
        Static().field3F1 = 0;
    }
    return r;
}

// BLReset_CocurrentTouchDetect @ 0x6ba4859c
// 三个以上触点同时按下（年龄都超过 200 帧，彼此相差不到 50 帧）。
bool ConcurrentTouchDetect() {
    const PrevTouchTable& prev = PrevTouches();
    uint16_t minAge = 0xffff;
    for (uint8_t i = 0; i < prev.count; ++i) minAge = std::min(minAge, prev.unit[i].age);
    uint8_t n = 0;
    for (uint8_t i = 0; i < prev.count; ++i) {
        const uint16_t age = prev.unit[i].age;
        if (age - minAge < 0x32 && 200 < age) ++n;
    }
    return 3 <= n && prev.count == n;
}

// BLReset_IsAbnormalTouchDetected @ 0x6ba486da
bool IsAbnormalTouchDetected() {
    if (!Reset::CheckNumOverNoise()) return false;
    const PrevTouchTable& prev = PrevTouches();
    const TsaPrpt& prpt = Prpt();
    const TsaStatic& stat = Static();
    const uint16_t n = PrmtFlash().sensorNum;
    if (prev.abnormalShapeCount != 0) {
        if (static_cast<char>(TSA_IsPadFeatureEnabled()) == 0) {
            if (n >> 1 < prpt.field186) return true;
        } else if ((n + n) / 3u < prpt.field186) {
            return true;
        }
    }
    if (stat.safeDif != 0 || stat.unsafeDif == 0) return false;
    if (3 <= prev.abnormalShapeCount) return true;
    if (prpt.field1A2 * 5u < prpt.field1A0) return false;
    if (prpt.field18E * -4 < prpt.field18C) return false;
    if (prev.abnormalShapeCount != 0) return true;
    if (n >> 2 < prpt.field186) {
        if (1 < prev.count - prev.field00D) return true;
        if (prev.field003 != 0 || prev.field006 == 0 || prev.abnormalShape2Count != 0) return true;
    }
    return n >> 1 < prpt.field186 || ConcurrentTouchDetect();
}

// BLReset_IsAllFreqsBaselineInitializedDuringTouchLiftOff @ 0x6ba48b01
// 各频点独立基线时，复位位刚清掉、非常噪声且 raw 上边界高出目标值半个峰门限以上，最多 3 次。
bool IsAllFreqsBaselineInitializedDuringTouchLiftOff() {
    if (!Afe::IsIndividualBaselineForEachFreq()) return false;
    TsaPrpt& prpt = Prpt();
    if (Static().resetFlagsCleared == 0 || Afe::IsAlwaysNoisy() ||
        static_cast<int>(static_cast<uint32_t>(prpt.boundaryPositive) - static_cast<uint32_t>(PrmtFlash().field5FC)) <=
            PrmtDynamic().peakThold / 2 ||
        3 <= prpt.allFreqInitCount) {
        return false;
    }
    ++prpt.allFreqInitCount;
    return true;
}

// BLReset_IsTriggered2AfeWakeUp @ 0x6ba4b9d4
// 唤醒后检查（或基线异常检查关闭时的 Check）连续成立累计超过 0xa0 ms 即复位；不成立时防抖逐帧回退。
bool IsTriggered2AfeWakeUp() {
    TsaStatic& stat = Static();
    TsaPrpt& prpt = Prpt();
    if (IsAllFreqsBaselineInitializedDuringTouchLiftOff()) {
        stat.cleanBaselineValid = 0;
        prpt.blPropertyState = kReasonAllFreqInit;
        return true;
    }
    if ((PrmtFlash().blAbnormalCheck == 0 && Check()) || IsAbnormalTouchDetected()) {
        ++stat.blResetDebounce;
        AddInterval(stat.blWakeUpTimer);
        if (stat.blWakeUpTimer <= 0xa0) return false;
        stat.cleanBaselineValid = 0;
        stat.blResetDebounce = 0;
        stat.blWakeUpTimer = 0;
        prpt.blPropertyState = kReasonWakeUp;
        stat.specialControlFlags |= kCtrlBlReset;
        const PrevTouchTable& prev = PrevTouches();
        if (prev.exceptionFlags == kExceptionTouchMode) {
            stat.field3F1 = 1;
            prpt.blPropertyState = kReasonWakeUpException;
        }
        if ((prev.stateFlags & kStateFlagWakeUp) != 0) prpt.blPropertyState = kReasonWakeUpState;
        return true;
    }
    if (stat.blResetStatus != 0 || stat.blResetDebounce < 2) {
        stat.blResetDebounce = 0;
        stat.blWakeUpTimer = 0;
    } else {
        --stat.blResetDebounce;
        SubInterval(stat.blWakeUpTimer);
    }
    return false;
}

// BLReset_IsSmartCoverOff @ 0x6ba49d6c
// 打开智能皮套后 250 ms 内超噪声格少的累计时间超过 0x7c ms 即复位。
bool IsSmartCoverOff() {
    TsaStatic& stat = Static();
    uint64_t& stopTime = Image::At<uint64_t>(0x6bb7f018);
    const bool was = (stat.prevFeatureFlags & kFeatSmartCover) != 0;
    const bool is = (stat.featureFlags & kFeatSmartCover) != 0;
    if (!was && is) {
        stopTime = 0;
        stat.smartCoverOffTimer = 0;
    }
    if (was && !is) {
        stopTime = stat.curTimeMs + 0xfa;
        stat.smartCoverOffTimer = 0;
    }
    if (stat.curTimeMs < stopTime) {
        if (Reset::CheckNumOverNoise()) {
            AddInterval(stat.smartCoverOffTimer);
        } else {
            stat.smartCoverOffTimer = 0;
        }
    }
    const bool r = 0x7c < stat.smartCoverOffTimer;
    if (r) {
        stat.smartCoverOffTimer = 0;
        stopTime = 0;
    }
    return r;
}

// BLReset_IsAfeSideTouchToBeProtected @ 0x6ba49ee3
// 侧边触摸时，AFE 报侧边有触摸而 Dif 分块统计里几乎没有信号。
bool IsAfeSideTouchToBeProtected() {
    struct DifBlockPrpt {  // g_tsaDifBlockPrpt（0x6bc4a980）里用到的三项
        uint8_t reserved000[0x14];
        int16_t field014;
        uint8_t reserved016[0x8a];
        int16_t field0A0;
        uint8_t reserved0A2[0x1a];
        int16_t field0BC;
    };
    const auto& blk = Image::At<DifBlockPrpt>(0x6bc4a980);
    if (!SideTouch() || 4 < static_cast<uint16_t>(blk.field0BC + blk.field014 + blk.field0A0) ||
        (Static().afeStatus & kAfeSideTouch) == 0) {
        return false;
    }
    Debug(kDbgAfeSideTouch);
    return true;
}

// BLReset_GetBaselineCleanState @ 0x6ba4af53
bool GetBaselineCleanState() {
    const bool r = AfterInit(kAfterInitMs) || IsBaselineClean();
    if (r) Debug(kDbgCleanState);
    return r;
}

// BLReset_GetTouchToBeProtected2State @ 0x6ba4afd3（IsToBeProtectedBySS 恒为 0）
bool GetTouchToBeProtected2State() {
    bool p = Reset::IsTouchToBeProtected2(false) || Static().noiseLevel == 3;
    if (AfterInit(kAfterInitMs)) p = p || IsTouchToBeProtected();
    return ToeSynaBl::IsTouchToBeProtected2(false, p);
}

/// BLReset_IsBaselineAbnormalType1..3 的公共结尾：成立时记调试位并累计，否则清零。
void Accumulate(bool hit, uint32_t bit, uint64_t& timer) {
    if (hit) {
        Debug(bit);
        timer = UpdateTimer(timer);
    } else {
        timer = 0;
    }
}

// BLReset_IsBaselineAbnormalType1 @ 0x6ba4b083
// 负向统计明显大于正向，且（初始化 310 s 内，或非游戏场景下 field184 超过四分之一屏）。
void IsBaselineAbnormalType1(bool overNoise, bool protected2) {
    const TsaPrpt& prpt = Prpt();
    bool hit = overNoise && !IsBaselineClean() &&
               (prpt.field16C * 3 < prpt.field16E * -4 || prpt.field180 * 3u < prpt.field182 * 5u) &&
               prpt.field178 * 3 < prpt.field17C * -5 && prpt.field16C * 2 < prpt.field16E * -4 &&
               PrmtDynamic().smallThold < -prpt.field16E && 2 < prpt.field182 && !protected2;
    if (hit && AfterInit(kAfterInitMs)) hit = !IsGame() && PrmtFlash().sensorNum >> 2 < prpt.field184;
    Accumulate(hit, kDbgAbnormal1, Static().abnormalType1Timer);
}

// BLReset_IsFullPanelOverNoise @ 0x6ba4ac58
// 自容的几段统计里，多数列与多数行都超过门限。
bool IsFullPanelOverNoise() {
    const FlashPrmt& fl = PrmtFlash();
    if (fl.cmfEnabled == 0 && fl.toeSynaBl == 0) return false;
    const SelfData& self = SelfBuf();
    uint8_t cols = 0;
    uint8_t rows = 0;
    int rising = 0;
    const bool side = SideTouch();
    const uint16_t end = static_cast<uint16_t>(fl.dim1Len - (side ? 1 : 0));
    for (uint16_t c = side ? 1 : 0; c < end; ++c) {
        if (600 < self.strongAbsSum[c]) ++cols;
        if (self.strongSum[c] + 400 < self.strongAbsSum[c]) ++rising;
    }
    for (uint16_t r = 0; r < fl.dim2Len; ++r) {
        if (0x96 < self.innerStrongAbsSum[fl.dim1Len + r]) ++rows;
    }
    if (rising <= 2) return false;
    if (!AfterInit(kAfterInitMs)) {
        return (fl.dim1Len * 2u / 3 <= cols && fl.dim2Len >> 1 <= rows) ||
               (static_cast<uint8_t>(static_cast<uint16_t>((fl.dim1Len * 0x11 + fl.dim1Len * 2) * 9) >> 9) <= cols &&
                fl.dim2Len * 3u / 5 <= rows);
    }
    return !IsGame() && fl.dim1Len * 2u / 3 < cols && fl.dim2Len * 3u / 5 < rows;
}

// BLReset_IsBaselineAbnormalType2 @ 0x6ba4b295
void IsBaselineAbnormalType2(bool game, bool overNoise, bool protected2) {
    const bool hit = overNoise && !game && !IsBaselineClean() && Prpt().raw2Raw.overNoiseAfterCmf < 5 && IsFullPanelOverNoise() && !protected2;
    Accumulate(hit, kDbgAbnormal2, Static().abnormalType2Timer);
}

// BLReset_IsBaselineAbnormalType3 @ 0x6ba4b35f
void IsBaselineAbnormalType3(bool game, bool overNoise, bool protected2) {
    const TsaPrpt& prpt = Prpt();
    TsaStatic& stat = Static();
    const bool hit = overNoise && !game && PrmtDynamic().smallThold < -prpt.field18E && 2 < prpt.field1A2 &&
                     prpt.raw2Raw.overNoiseAfterCmf < 3 && prpt.field16C * 2 < prpt.field16E * -3 && !IsTouchToBeProtected() && !protected2 &&
                     (prpt.field180 * 4u < prpt.field182 || prpt.field178 * 4 < -prpt.field17C);
    if (hit && !ToeSynaBl::IsWaterDetected()) {
        Debug(kDbgAbnormal3);
        stat.abnormalType3Timer = UpdateTimer(stat.abnormalType3Timer);
        return;
    }
    stat.abnormalType3Timer = 0;
    stat.toeWaterTimer = 0;
}

// BLReset_IsBaselineAbnormal @ 0x6ba4b506（ToeSynaBl 的侧边 / SS 保护依赖被裁掉的 SS，恒不成立）
// 三类异常任一累计超过 300 ms 即成立；AFE 报侧边触摸时三者都清零。
bool IsBaselineAbnormal() {
    TsaStatic& stat = Static();
    bool abnormal = false;
    const bool game = IsGame();
    const bool afeSide = IsAfeSideTouchToBeProtected();
    const bool overNoise = Reset::CheckNumOverNoise();
    const bool protected2 = GetTouchToBeProtected2State();
    IsBaselineAbnormalType1(overNoise, protected2);
    IsBaselineAbnormalType2(game, overNoise, protected2);
    IsBaselineAbnormalType3(game, overNoise, protected2);
    if (afeSide) {
        stat.abnormalType1Timer = 0;
        stat.abnormalType2Timer = 0;
        stat.abnormalType3Timer = 0;
    }
    if (kTimerLimit < stat.abnormalType1Timer || kTimerLimit < stat.abnormalType2Timer || kTimerLimit < stat.abnormalType3Timer) {
        stat.abnormalType1Timer = 0;
        stat.abnormalType2Timer = 0;
        stat.abnormalType3Timer = 0;
        abnormal = true;
    }
    const bool clean = GetBaselineCleanState();
    const bool skipShape = clean || afeSide || !overNoise;
    ToeSynaBl::IsBaselineAbnormal(skipShape, protected2, abnormal);
    return abnormal;
}

// BLReset_IsAfeRawJump @ 0x6ba4b70a
// AFE 报 raw 跳变（afeStatus bit17）后下一帧复位；之后计时器停在 -1，直到 AFE 清掉标志。
bool IsAfeRawJump() {
    if (IsGame()) return false;
    TsaStatic& stat = Static();
    if (0 < stat.afeRawJumpTimer) {
        Debug(kDbgAfeRawJump);
        if (!Reset::CheckNumOverNoise() && Reset::IsTouchToBeProtected2(false)) {
            stat.afeRawJumpTimer = 1;
        } else {
            AddInterval(stat.afeRawJumpTimer);
        }
    }
    const bool jump = (stat.afeStatus & kAfeRawJump) != 0;
    if (jump && stat.afeRawJumpTimer == 0) stat.afeRawJumpTimer = 1;
    if (!jump && stat.afeRawJumpTimer == -1) stat.afeRawJumpTimer = 0;
    const bool r = 0 < stat.afeRawJumpTimer;
    if (r) stat.afeRawJumpTimer = -1;
    return r;
}

// BLReset_CheckTouchTrustability @ 0x6ba4a613
// 不贴边的小触点（标记 0x20000000、椭圆度不超过 12、峰值不小于负向极值的一半）可信帧数加一，其余减一。
void CheckTouchTrustability() {
    PrevTouchTable& prev = PrevTouches();
    const InnerPeak inner;
    const int negPeak = -Prpt().dif.minSig;
    for (uint16_t i = 0; i < prev.count; ++i) {
        PrevTouchUnit& u = prev.unit[i];
        const bool trusted = inner(u) && (u.persistFlags & kPersistSmallTouch) != 0 && u.ellipseRate <= 0xc && negPeak < u.maxPeakSig * 2;
        if (trusted) {
            if (u.blTrustFrames < kMaxTrustFrames) ++u.blTrustFrames;
        } else if (u.blTrustFrames < kMaxTrustFrames && u.blTrustFrames != 0) {
            --u.blTrustFrames;
        }
    }
}

// BLReset_BaselineStateMachine @ 0x6ba49f88
// 干净基线：开机 6 帧后等出现不贴边的小触点（1 → 2），触点离开后（4）计时，超噪声格少于 2 时进入确认（8），
// 确认 0x65 ms 都没有触点即把当前基线存为干净基线（0x10）。中途超噪声格变多则判基线可疑并回到 1。
void BaselineStateMachine(bool touched) {
    TsaStatic& stat = Static();
    const uint16_t overNoise = Prpt().dif.count;
    switch (stat.cleanState) {
    case 0:
        if (stat.cleanFrames < 6) {
            ++stat.cleanFrames;
        } else {
            stat.cleanState = 1;
            stat.cleanFrames = 0;
        }
        break;
    case 1:
        if (touched) stat.cleanState = 2;
        break;
    case 2:
        if (!touched) {
            stat.cleanState = 4;
            stat.cleanTimer = 0;
        }
        break;
    case 4:
        if (touched) {
            stat.cleanState = 2;
            stat.cleanTimer = 0;
        } else if (overNoise < 0x1f && stat.cleanTimer < 0x14e) {
            if (overNoise < 2) {
                stat.cleanState = 8;
                stat.cleanTimer = 0;
            } else {
                AddInterval(stat.cleanTimer);
            }
        } else {
            stat.blSuspect = 1;
            stat.cleanState = 1;
            stat.cleanTimer = 0;
        }
        break;
    case 8:
        if (touched) {
            stat.cleanState = 2;
            stat.cleanTimer = 0;
        } else if (stat.cleanTimer < 0x65) {
            if (overNoise < 2) {
                AddInterval(stat.cleanTimer);
            } else {
                stat.blSuspect = 1;
                stat.cleanState = 1;
            }
        } else {
            stat.blSuspect = 0;
            stat.cleanState = kCleanDone;
            CopyChecked(Buffers().cleanBaseline, kBlBytes, Buffers().bl, PrmtFlash().sensorNum * size_t{2});
            stat.cleanBaselineValid = 1;
        }
        break;
    default:
        break;
    }
}

// BLReset_BaselineStateUpdate @ 0x6ba4a368
// 互容初始化当帧、本帧复位或 raw 不稳定刚结束时重新开始；否则按有无不贴边的小触点推进状态机。
void BaselineStateUpdate(bool reset) {
    TsaStatic& stat = Static();
    const bool unstableEnded = (stat.afeStatus & kAfeRawUnstable) == 0 && (stat.prevAfeStatus & kAfeRawUnstable) != 0;
    if (stat.curTimeMs == stat.msInitTime || reset || unstableEnded) {
        stat.cleanState = 0;
        stat.cleanTimer = 0;
        stat.cleanFrames = 0;
        stat.blSuspect = 0;
        return;
    }
    if (stat.cleanState == kCleanDone) return;
    const PrevTouchTable& prev = PrevTouches();
    const InnerPeak inner;
    bool touched = false;
    for (uint16_t i = 0; i < prev.count; ++i) {
        const PrevTouchUnit& u = prev.unit[i];
        if (inner(u) && (u.persistFlags & kPersistSmallTouch) != 0 && u.ellipseRate < 0xc) {
            touched = true;
            break;
        }
    }
    BaselineStateMachine(touched);
}

/// 复位原因记下后同时请求特殊控制。
void Trigger(uint32_t reason, bool clearClean) {
    if (clearClean) Static().cleanBaselineValid = 0;
    Prpt().blPropertyState = reason;
    Static().specialControlFlags |= kCtrlBlReset;
}

// BLReset_IsTriggered2 @ 0x6ba4bc66（SensorData、NotchProtect 被裁掉）
bool IsTriggered2() {
    TsaStatic& stat = Static();
    const bool rawJump = IsLongTimeRawJumpDetected();
    if (rawJump) Prpt().blPropertyState = kReasonRawJump;
    bool r = IsTriggered2BySs() || rawJump;
    if (Afe::IsFrequentlyHopping() && stat.curTimeMs < stat.msInitTime + 15000) Trigger(kReasonFreqHopping, true);
    if (IsTriggered2AfeWakeUp()) r = true;
    if (IsSmartCoverOff()) {
        r = true;
        Trigger(kReasonSmartCoverOff, true);
    }
    if (PrmtFlash().blAbnormalCheck != 0 && IsBaselineAbnormal()) {
        r = true;
        Trigger(kReasonAbnormal, false);
    }
    if (IsCurBaselineDirty()) {
        r = true;
        Trigger(kReasonDirty, true);
    }
    if (ToeSynaBl::IsBaselineAbnormalAftScreenOn()) {
        r = true;
        Trigger(kReasonToeAftScreenOn, true);
    }
    if (IsAfeRawJump()) {
        r = true;
        Trigger(kReasonAfeRawJump, true);
    }
    CheckTouchTrustability();
    BaselineStateUpdate(r);
    stat.blResetStatus = r;
    return r;
}

// BLReset_GetRawChangedThold @ 0x6ba4901e
// 按噪声门限；扫描线两端的格放宽到 4/3。
int16_t RawChangedThold(uint16_t k, uint16_t line) {
    const int16_t t = Prpt().noiseThold;
    const FlashConstPrmt& c = PrmtFlashConst();
    const bool edge = PrmtFlash().dim1Axis == 1 ? (k == 0 || k == c.field024 - 1) : (line == 0 || line == c.field025 - 1);
    return edge ? static_cast<int16_t>(t * 4 / 3) : t;
}

}  // namespace

namespace Reset {

// BLReset_Init @ 0x6ba47783
void Init(bool clearLatch) {
    TsaPrpt& prpt = Prpt();
    TsaStatic& stat = Static();
    prpt.blResetFrames = 0;
    prpt.blSinceResetFrames = 0;
    prpt.allFreqInitCount = 0;
    prpt.blPropertyState = 0;
    prpt.prevBlPropertyState = 0;
    stat.blResetCheckEnabled = 1;
    stat.blResetDebounce = 0;
    stat.blWakeUpTimer = 0;
    stat.rawJumpTimer = 0;
    stat.ssNoSignalTimer = 0;
    stat.ssNegSignalTimer = 0;
    stat.ssNegSignalStableTimer = 0;
    stat.field360 = 0;
    stat.smartCoverOffTimer = 0;
    stat.abnormalType1Timer = 0;
    stat.abnormalType2Timer = 0;
    stat.abnormalType3Timer = 0;
    stat.dirtyTimer = 0;
    stat.afeRawJumpTimer = 0;
    stat.toeSsNotMatchTimer = 0;
    stat.toeAbnormalShapeTimer = 0;
    stat.toeWaterTimer = 0;
    stat.toeClothRiskTimer = 0;
    stat.toeBackLightShiftTimer = 0;
    stat.toeSsNoSignalTimer = 0;
    stat.field3D0 = 0;
    stat.cleanState = 0;
    stat.blResetStatus = 0;
    stat.blNoisyTimer = 0;
    stat.blNoisyReset = 0;
    stat.rawJumpLatched = 0;
    stat.rawJumpDetected = 0;
    stat.blUpdatedThisFrame = 0;
    stat.blHwResetRequest = 0;
    stat.field3F1 = 0;
    stat.blDebugFlags = 0;
    stat.cleanBaselineValid = 0;
    const size_t bytes = PrmtFlash().sensorNum * size_t{2};
    ClearChecked(Buffers().cleanBaseline, bytes, bytes);
    if (clearLatch) stat.blResetLatched = 0;
}

// BLReset_Process @ 0x6ba4bf90（BLReset_IsToDisable 恒为 0，安全基线下的 SafeBaseline_IsResetTriggered 恒为 0）
bool Process(bool reset) {
    TsaStatic& stat = Static();
    TsaPrpt& prpt = Prpt();
    const DynamicPrmt& dyn = PrmtDynamic();
    stat.blDebugFlags = 0;
    if (reset) prpt.blSinceResetFrames = 0;
    stat.blResetCheckEnabled = 1;
    const bool r = reset || (!SafeBaselineApplied() && IsTriggered2());
    if (prpt.blResetFrames < static_cast<uint16_t>(dyn.resetCounterCap)) ++prpt.blResetFrames;
    if (prpt.blSinceResetFrames < static_cast<uint16_t>(dyn.field05A)) ++prpt.blSinceResetFrames;
    if (r) prpt.blSinceResetFrames = 0;
    return r;
}

// BLReset_IsBaselineNoisy @ 0x6ba4c0ba
// 上一帧因噪声以外的原因复位、且常噪声时开始计时；超噪声格少于 6 时立即放行，否则在 0xa7 ms 内保持「噪声」。
bool IsBaselineNoisy() {
    TsaStatic& stat = Static();
    TsaPrpt& prpt = Prpt();
    const uint32_t prev = prpt.prevBlPropertyState;
    if (prev != kReasonNoisy && prev != 3 && prev != 0 && Afe::IsAlwaysNoisy()) stat.blNoisyTimer = Interval<uint64_t>();
    if (stat.blNoisyTimer == 0) return false;
    if (prpt.raw2Raw.overNoiseCount < 6) {
        stat.blNoisyReset = 1;
        stat.blNoisyTimer = 0;
        return true;
    }
    if (stat.blNoisyTimer < 0xa7) {
        AddInterval(stat.blNoisyTimer);
        prpt.blPropertyState = kReasonNoisy;
        return true;
    }
    stat.blNoisyTimer = 0;
    return false;
}

// BLReset_CheckNumOverNoise @ 0x6ba4850e
// 超噪声的格少（Hybrid Incell 另看 overNoiseAfterCmf，Toe 机型另看 overLtiCount）。不成立时记调试位。
bool CheckNumOverNoise() {
    const TsaPrpt& prpt = Prpt();
    if (prpt.raw2Raw.overNoiseCount < 5) return true;
    if (PrmtFlash().sensorArchitecture == 2 && prpt.raw2Raw.overNoiseAfterCmf < 3) return true;
    if (!SafeBaselineApplied() && ToeSynaBl::CheckNumOverNoise()) return true;
    Debug(kDbgNumOverNoise);
    return false;
}

// BLReset_IsTouchToBeProtected2 @ 0x6ba47e9a
// 历史触点不到 6 个时，不贴边、小而稳定、峰值可观且自容也有信号的触点个数（needTwo 时至少 2 个）。
bool IsTouchToBeProtected2(bool needTwo) {
    const PrevTouchTable& prev = PrevTouches();
    if (6 <= prev.count) return false;
    const TsaPrpt& prpt = Prpt();
    const uint8_t cols = PrmtFlash().dim1Len;
    const int32_t* ss = SsPreCmfDif();
    const SelfData& self = SelfBuf();
    const int peakLimit = PrmtDynamic().peakThold / 3 * 4;
    const InnerPeak inner;
    uint16_t n = 0;
    for (uint16_t i = 0; i < prev.count; ++i) {
        const PrevTouchUnit& u = prev.unit[i];
        if (!inner(u) || (u.persistFlags & kPersistSmallTouch) == 0 || 0xd <= u.ellipseRate || u.signalScale <= peakLimit ||
            u.maxPeakSig * 2 <= -prpt.dif.minSig) {
            continue;
        }
        const PeakEntry& pk = Peaks().entry[u.peakIdx];
        if ((0 < ss[cols + pk.dim2] || 0 < ss[pk.dim1]) && 0 < self.innerStrongSum[cols + pk.dim2] + 100 &&
            0 < self.innerStrongSum[pk.dim1] + 100) {
            ++n;
        }
    }
    if (needTwo && n < 2) return false;
    if (n == 0) return false;
    Debug(kDbgProtected2);
    return true;
}

// BLReset_IsRawChanged @ 0x6ba490f5（调用方都比 CMF 前的 raw，且不做 CMF）
// 按扫描线逐格比较，变化超过 RawChangedThold 的非坏点格达到 4 个即判为变化。
bool IsRawChanged(int16_t* stored, bool store) {
    const TsaBufferTable& buf = Buffers();
    const auto* cur = reinterpret_cast<const uint16_t*>(buf.preCmfRaw);
    if (store) {
        CopyChecked(stored, GridBufSize() * size_t{2}, cur, PrmtFlash().sensorNum * size_t{2});
        return false;
    }
    const FlashPrmt& fl = PrmtFlash();
    const FlashConstPrmt& c = PrmtFlashConst();
    TsaAssert(c.field024 <= kMaxCmfLen);
    const uint8_t* sensorFlags = TpSensor::Flags();
    const uint16_t first = SideTouch() ? 1 : 0;
    const uint16_t lines = static_cast<uint16_t>(c.field025 - first);
    uint16_t changed = 0;
    for (uint16_t line = first; line < lines; ++line) {
        for (uint16_t k = 0; k < c.field024; ++k) {
            const auto idx = static_cast<uint16_t>(fl.dim1Axis == 1 ? k + fl.dim1Len * line : line + fl.dim1Len * k);
            const int d = reinterpret_cast<const uint16_t*>(stored)[idx] - cur[idx];
            if (RawChangedThold(k, line) < std::abs(d) && (sensorFlags[idx] & TpSensor::kFlagNotch) == 0) ++changed;
        }
    }
    return 4 <= changed;
}

}  // namespace Reset

// BLReset_InDebounce @ 0x6ba47750
bool ResetInDebounce() { return Static().blResetDebounce != 0; }

// BLReset_Status @ 0x6ba4776c
uint8_t ResetStatus() { return Static().blResetStatus; }

// BLReset_FlagDisableBaselineUpdateInit @ 0x6ba47ae6
void ClearRefreshRequest() { Static().blRefreshRequest = 0; }

// BLReset_CheckAbnormalTouch @ 0x6ba48366
bool CheckAbnormalTouch(uint8_t touchIdx) {
    if (!Afe::IsRawUnifiedToTargetValue()) return false;
    const PeakEntry& pk = Peaks().entry[Touches().unit[touchIdx].peakIdx];
    const uint16_t i = GridIndex(pk.dim1, pk.dim2);
    const TsaBufferTable& buf = Buffers();
    return CheckAbnormal(static_cast<uint16_t>(buf.raw[i]), static_cast<uint16_t>(buf.bl[i]));
}

}  // namespace Solvers::Tsa::Core::Baseline

// raw 的 TSA / TSAMS / TouchReport / GripFilter 还在调用的入口
extern "C" {
namespace B = Solvers::Tsa::Core::Baseline;
bool BLReset_InDebounce(void) { return B::ResetInDebounce(); }
uint8_t BLReset_Status(void) { return B::ResetStatus(); }
void BLReset_FlagDisableBaselineUpdateInit(void) { B::ClearRefreshRequest(); }
}  // extern "C"