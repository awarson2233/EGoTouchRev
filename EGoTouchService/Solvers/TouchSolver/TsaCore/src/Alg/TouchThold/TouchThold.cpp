#include "Alg/TouchThold/TouchThold.h"

#include <algorithm>

#include "Alg/Touch/TouchData.h"
#include "App/Sys.h"
#include "App/TsaStatic.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::TouchThold {

namespace {

constexpr int16_t kSensitivityPeakThold = 0x168;  // 灵敏度机型在手套确认态的峰门限
constexpr int16_t kSmartCoverPeakThold = 0x1a4;   // 灵敏度机型在智能皮套（非 SOF）下的峰门限

/// 手套 / 指关节模式生效：动态门限允许该模式（zTholdFlags bit2 / bit3）且系统已开启。
bool GloveOrKnuckleActive() {
    const uint8_t flags = PrmtDynamic().zTholdFlags;
    const uint32_t feature = Static().featureFlags;
    return ((flags & 4) != 0 && (feature & kFeatGloveMode) != 0) || ((flags & 8) != 0 && (feature & kFeatKnuckle) != 0);
}

bool SmartCover() { return (Static().featureFlags & kFeatSmartCover) != 0; }

/// 整屏 incell 且信号失衡时，峰门限减半。
bool HalvePeakThold() { return PrmtFlash().sensorArchitecture == kFullIncell && Static().fullIncellSignalDisparity != 0; }

// TouchThold_UseHighTholdAtSensitivityMode @ 0x6ba7ce03
bool UseHighTholdAtSensitivityMode() {
    return IsProject("P116") || IsProject("P150") || IsProject("P172") || IsProject("P173") || IsProject("P186");
}

// TouchThold_SOFInSmartCoverMode @ 0x6ba7ceb3
bool SofInSmartCoverMode() {
    const FlashPrmt& flash = PrmtFlash();
    return flash.cmfMethodSmartCover == 1 || flash.cmfMethodNormal == 1;
}

// TouchThold_GetTholdBasedOnSNR @ 0x6ba7c776
int GetTholdBasedOnSnr(int16_t thold) { return PrmtFlash().snrScaleX16 * thold / 16; }

// TouchThold_GetTholdBasedOnTouchMode @ 0x6ba7c7a5
int16_t GetTholdBasedOnTouchMode(int16_t fingerThold, int16_t gloveThold) {
    if (UseFingerThold()) return static_cast<int16_t>(fingerThold / 2);
    return static_cast<int16_t>(gloveThold * 4 / 5);
}

// TouchThold_GetNoisyTouchNum @ 0x6ba7c896
// sigSum 超过机型门限的历史触点个数。
uint8_t GetNoisyTouchNum() {
    const PrevTouchTable& prev = PrevTouches();
    const int16_t limit = PrmtFlash().noisyTouchSigSum;
    uint8_t num = 0;
    for (uint8_t i = 0; i < prev.count; ++i) {
        if (limit < prev.unit[i].sigSum) ++num;
    }
    return num;
}

// TouchThold_GetNoiseThold @ 0x6ba7ca48
// 噪声触点越多取越高的机型噪声门限，按 SNR 换算；SafeBaseline 生效时不低于按触点模式算的门限。
int16_t GetNoiseThold(int16_t fingerThold, int16_t gloveThold) {
    const FlashPrmt& flash = PrmtFlash();
    const uint8_t noisy = GetNoisyTouchNum();
    int16_t base = flash.noiseTholdQuiet;
    if (noisy == 1) {
        base = std::max(base, flash.noiseTholdOneNoisy);
    } else if (noisy > 1) {
        base = std::max(base, flash.noiseTholdMultiNoisy);
    }
    auto noise = static_cast<int16_t>(GetTholdBasedOnSnr(base));
    const int16_t byTouchMode = GetTholdBasedOnTouchMode(fingerThold, gloveThold);
    if (static_cast<char>(Sys::IsSafeBaselineApplied()) != 0) noise = std::max(noise, byTouchMode);
    Prpt().noiseThold = noise;
    return noise;
}

// TouchThold_BasedOnExperience @ 0x6ba7cb32
// 非整屏 incell：取 min(参考门限, 正向极值) / 4，再按异常状态用负向极值与正向极值钳位。
int16_t BasedOnExperience(int16_t thold) {
    const FlashPrmt& flash = PrmtFlash();
    if (flash.sensorArchitecture == kFullIncell) return thold;
    const int16_t minSig = TSA_GetMinSigPrpt();
    if (flash.sensorArchitecture == kFullIncell) return thold;
    const TsaPrpt& prpt = Prpt();
    const uint32_t exceptions = PrevTouches().exceptionFlags;
    auto result = static_cast<int16_t>(std::min(flash.peakFilterReferenceThreshold, prpt.sigPeakPositive) / 4);
    if (exceptions == 0x100 || Static().noiseLevel == 2) {
        if (result < -minSig) result = static_cast<int16_t>(-minSig);
        const auto cap = static_cast<int16_t>(prpt.sigPeakPositive * 3 / 4);
        result = std::min(result, cap);
    } else if ((exceptions & 0xc8) == 0) {
        result = std::max(result, static_cast<int16_t>(-minSig / 2));
    }
    return result;
}

// TouchThold_GetPeakThold @ 0x6ba7cee9
// 手指模式取 finger 门限（信号失衡时减半）；手套 / 指关节 / 智能皮套 / field3F1 下取手套门限，
// 但 touchMode 为 1、touchModeLevel 为 2 且动态门限允许（zTholdFlags bit1）时仍按手指处理。
int16_t GetPeakThold(int16_t fingerThold, int16_t gloveThold) {
    const TsaPrpt& prpt = Prpt();
    if (!GloveOrKnuckleActive() && !SmartCover() && Static().field3F1 == 0) {
        return HalvePeakThold() ? static_cast<int16_t>(fingerThold / 2) : fingerThold;
    }
    // 原代码为 (2 >> touchMode) & 1，移位量按 x86 取低 5 位
    if ((prpt.touchMode & 0x1f) == 1 && prpt.touchModeLevel == 2 && (PrmtDynamic().zTholdFlags & 2) != 0 && !SmartCover()) {
        return HalvePeakThold() ? static_cast<int16_t>(fingerThold / 2) : fingerThold;
    }
    if (!SmartCover()) {
        if (prpt.touchMode == 2 && prpt.touchModeLevel == 2 && UseHighTholdAtSensitivityMode()) return kSensitivityPeakThold;
    } else if (!SofInSmartCoverMode() && UseHighTholdAtSensitivityMode()) {
        return kSmartCoverPeakThold;
    }
    return gloveThold;
}

// TouchThold_GetTouchDownThold @ 0x6ba7cca5
// 整屏 incell：按下门限取 min(正向极值, 参考门限) / 3（featureFlags bit12 时不取），且不低于触点门限。
int16_t GetTouchDownThold(int16_t touchThold) {
    const FlashPrmt& flash = PrmtFlash();
    if (flash.sensorArchitecture != kFullIncell) return touchThold;
    int16_t result = touchThold;
    if ((Static().featureFlags & 0x1000) == 0) {
        result = static_cast<int16_t>(std::min(Prpt().sigPeakPositive, flash.peakFilterReferenceThreshold) / 3);
    }
    return std::max(result, touchThold);
}

}  // namespace

// TouchThold_ToeUseSpecialThold @ 0x6ba7cd8f
bool ToeUseSpecialThold() { return IsProject("P116") || IsProject("P150") || IsProject("P186"); }

// TouchThold_IsSwitching @ 0x6ba7c90e
bool IsSwitching() { return Prpt().tholdSwitching != 0; }

bool UseFingerThold() {
    const TsaStatic& stat = Static();
    return (Prpt().touchModeLatched == 0 || !GloveOrKnuckleActive()) && !SmartCover() && stat.field3F1 == 0 &&
           stat.fullIncellSignalDisparity == 0;
}

// TouchThold_Init @ 0x6ba7c922
void Init() {
    const FlashPrmt& flash = PrmtFlash();
    TsaPrpt& prpt = Prpt();
    prpt.peakThold = static_cast<int16_t>(GetTholdBasedOnSnr(flash.noiseTholdMultiNoisy));
    prpt.touchThold = flash.fingerThold;
    prpt.fingerTouchThold = flash.fingerThold;
    prpt.touchDownThold = flash.fingerThold;
    prpt.field2DC = static_cast<uint8_t>(flash.reportRateHz >> 2);
    prpt.field2D6 = prpt.touchThold;
    prpt.noiseThold = prpt.touchThold;
    prpt.field2E0 = 10;
    prpt.field2DE = prpt.touchThold;
    prpt.tholdSwitching = 0;
    Static().field3FB = 0;
}

// TouchThold_Process @ 0x6ba7d0ee
// 峰门限：历史表只有一个触点、field005 为 1 且它的 volatileFlags bit12 未置位时取 3/4，再不低于经验门限。
// 触点门限：finger 或手套门限，不低于噪声门限与峰门限。
void Process(int16_t fingerThold, int16_t gloveThold, int16_t* peakThold, int16_t* touchThold, int16_t* touchDownThold) {
    const int16_t peak = GetPeakThold(fingerThold, gloveThold);
    const PrevTouchTable& prev = PrevTouches();
    if (prev.count == 1 && prev.field005 == 1 && (prev.unit[0].volatileFlags & 0x1000) == 0) {
        *peakThold = static_cast<int16_t>(peak * 3 / 4);
    } else {
        *peakThold = peak;
    }
    const int16_t experience = BasedOnExperience(*peakThold);
    if (*peakThold < experience) *peakThold = experience;

    int16_t touch = UseFingerThold() ? fingerThold : gloveThold;
    const int16_t noise = GetNoiseThold(fingerThold, gloveThold);
    touch = std::max(touch, noise);
    touch = std::max(touch, *peakThold);
    *touchThold = touch;
    *touchDownThold = GetTouchDownThold(*touchThold);
}

}  // namespace Solvers::Tsa::Core::TouchThold

namespace T = Solvers::Tsa::Core::TouchThold;

extern "C" {
void TouchThold_Init(void) { T::Init(); }
void TouchThold_Process(int16_t fingerThold, int16_t gloveThold, int16_t* peakThold, int16_t* touchThold,
                        int16_t* touchDownThold) {
    T::Process(fingerThold, gloveThold, peakThold, touchThold, touchDownThold);
}
uint8_t TouchThold_IsSwitching(void) { return Solvers::Tsa::Core::Prpt().tholdSwitching; }
uint64_t TouchThold_ToeUseSpecialThold(void) { return T::ToeUseSpecialThold(); }
}  // extern "C"
