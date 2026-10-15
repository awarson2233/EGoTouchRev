#include "App/Scenario.h"

#include "Alg/Touch/PrevTouch.h"
#include "Alg/Touch/TouchData.h"
#include "App/Afe.h"
#include "App/Sys.h"
#include "App/TsaStatic.h"
#include "Prmt/Prmt.h"
#include "Prmt/TsaPrmt.h"

namespace Solvers::Tsa::Core::Scenario {

namespace {

constexpr uint32_t kSysGloveOff = 0x80000;       // TsaStatic::systemFlags：宿主关闭手套检测
constexpr uint32_t kSysPhoneCall = 0x400000;
constexpr uint32_t kSysScreenLock = 0x800000;
constexpr uint32_t kFeatProximity = 0x4000;      // TsaStatic::featureFlags
constexpr uint32_t kFeatExtMask = 0x4813;
constexpr uint32_t kFeatUnsupported = 0x8;
constexpr uint32_t kProxScreenLockMask = 0x180;  // FlashPrmt::proxConfig：锁屏时也做接近检测
constexpr uint32_t kVolumeSideGesture = 0x4000;  // FlashPrmt::volumeGestureSet

/// scenarioMask 的位（与 FlashPrmt 的各 *Scenarios 相与）。
constexpr uint32_t kMaskScreenLock = 0x1;
constexpr uint32_t kMaskPhoneCall = 0x2;
constexpr uint32_t kMaskScreenLockPhoneCall = 0x4;
constexpr uint32_t kMaskPossibleProximity = 0x8;

constexpr uint8_t kGloveZTholdFlag = 0x4;        // DynamicPrmt::zTholdFlags：支持手套模式
constexpr uint32_t kTouchModeGlove = 2;
constexpr int32_t kTouchModeConfirmed = 2;
constexpr uint16_t kStrongTapGloveLimit = 5;
constexpr int kRateStepHz = 60;
constexpr uint16_t kMaxReportRateHz = 360;

void Set(const char* key, int32_t value) { RtPrmt::SetKeyValue(key, value); }

uint32_t Gate() { return Static().aftGateFlags; }

bool IsHighRateGame() { return !IsTyping() && (Gate() & kGame) != 0; }
bool IsNormalRateGame() { return !IsTyping() && (Gate() & kNormalRateGame) != 0; }

bool IsScreenLockPhoneCall() {
    const uint32_t sys = Static().systemFlags;
    return (sys & kSysPhoneCall) != 0 && (sys & kSysScreenLock) != 0;
}

bool IsPossibleProximity() {
    return (Static().featureFlags & kFeatProximity) != 0 && !IsPhoneCall() && !IsScreenLock() && !IsScreenLockPhoneCall();
}

// TSA_AppTypeToScenario @ 0x6ba90318：宿主的应用类型换成场景位（保留输入场景位）
void AppTypeToScenario() {
    TsaStatic& s = Static();
    if (s.appScenarioValid == 0) return;
    s.aftGateFlags &= kTyping;
    switch (s.appType) {
    case 8: s.aftGateFlags |= kVideo; break;
    case 4: s.aftGateFlags |= kInput; break;
    case 9: s.aftGateFlags |= kGame; break;
    case 999: s.aftGateFlags |= kSpecialApp; break;
    default: s.aftGateFlags |= kNormal; break;
    }
}

// TSA_ScenarioTypingPrmtSetting @ 0x6ba8ff4d
void TypingSetting() {
    const FlashPrmt& fl = PrmtFlash();
    Set("dbEnterIDLE", fl.dbEnterIdleTyping);
    if (PrevTouch::IsAnyExceptionDetected()) return;
    Set("sigMult", fl.sigMultNormal);
    Set("sigMultSD", fl.sigMultNormal);
    Set("accThold", fl.accThold >> 1);
}

// TSA_ScenarioGamePrmtSetting @ 0x6ba8ffe1 / TSA_ScenarioNormalRateGamePrmtSetting @ 0x6ba90092
void GameSetting(bool highRate) {
    const FlashPrmt& fl = PrmtFlash();
    const DynamicPrmt& dyn = PrmtDynamic();
    if (!PrevTouch::IsAnyExceptionDetected()) Set("sigRatioToDirectReport", 0x10);
    Set("dbEnterIDLE", fl.dbEnterIdleGame);
    Set("maxClickSuppressDistance", dyn.gameMaxClickSuppressDistance);
    Set("clickDuration", dyn.gameClickDuration);
    Set("clickSuppressDistance", dyn.gameClickSuppressDistance);
    if (highRate) Afe::SetMaxScanRate(fl.gameMaxScanRate);
}

// TSA_UpdateReportRate @ 0x6ba9044e：宿主指定的报点率档位
void UpdateReportRate() {
    const TsaStatic& s = Static();
    if (s.appScenarioValid == 0) return;
    if (s.reportRateLevel == 0) {
        Afe::SetMaxScanRate(PrmtFlash().maxScanRate);
    } else if (s.reportRateLevel < 7) {
        Afe::SetMaxScanRate(static_cast<uint16_t>(s.reportRateLevel * kRateStepHz));
    } else {
        Afe::SetMaxScanRate(kMaxReportRateHz);
    }
}

// TSA_EnableGloveFeature @ 0x6ba9eacd
void EnableGlove() {
    if ((PrmtDynamic().zTholdFlags & kGloveZTholdFlag) != 0) Static().featureFlags |= kFeatGloveMode;
}

// TSA_UpdateSysScenario @ 0x6ba8fc9b
void UpdateScenarioMask() {
    uint32_t m = 0;
    if (IsScreenLockPhoneCall()) m = kMaskScreenLockPhoneCall;
    else if (IsPhoneCall()) m = kMaskPhoneCall;
    else if (IsScreenLock()) m = kMaskScreenLock;
    else if (IsPossibleProximity()) m = kMaskPossibleProximity;
    Static().scenarioMask = m;
}

// TSA_SetVolumeSideEnabledScreenRotate90or270 @ 0x6ba9012b：横屏 270° 时左右两侧对调
void SetVolumeSideEnabled() {
    const int16_t sides = static_cast<int16_t>(PrmtFlash().volumeSideEnabled2);
    if (sides == 0 || sides == 3) {
        Set("volumeSideEnabled", sides);
        return;
    }
    const bool rot270 = Static().screenRotation == 3;
    if ((sides & 1) != 0) Set("volumeSideEnabled", rot270 ? 2 : 1);
    if ((sides & 2) != 0) Set("volumeSideEnabled", rot270 ? 1 : 2);
}

// TSA_SideTouchPrmtSetting @ 0x6ba90223：非通话、横屏时用横屏的音量手势区域
void SideTouchSetting() {
    const FlashPrmt& fl = PrmtFlash();
    const TsaStatic& s = Static();
    if ((s.systemFlags & kSysPhoneCall) != 0 || (fl.volumeGestureSet & kVolumeSideGesture) == 0) return;
    if (s.screenRotation != 1 && s.screenRotation != 3) return;
    Set("volumeActiveAreaStart", fl.volumeActiveAreaStart2);
    Set("volumeActiveAreaEnd", fl.volumeActiveAreaEnd2);
    Set("volumeGreenAreaStart", fl.volumeGreenAreaStart2);
    Set("volumeGreenAreaEnd", fl.volumeGreenAreaEnd2);
    SetVolumeSideEnabled();
}

/// 合并一个外部特性位：请求与初值相同时沿用当前的开关。
void KeepIfUnchanged(uint32_t& requested, uint32_t bit) {
    if (((requested ^ Static().extFeatureFlagsInit) & bit) == 0) requested = (requested & ~bit) | (Static().featureFlags & bit);
}

}  // namespace

// TSA_IsTypingScenario @ 0x6ba8fe17
bool IsTyping() { return (Gate() & kTyping) != 0; }

// TSA_IsJustEnterTypingScenario @ 0x6ba8fe53
bool IsJustEnterTyping() { return IsTyping() && (Static().prevAftGateFlags & kTyping) == 0; }

// TSA_IsVideoScenario @ 0x6ba8fdf7
bool IsVideo() { return (Gate() & kVideo) != 0; }

// TSA_IsGameScenario @ 0x6ba8fe7f
bool IsGame() { return IsHighRateGame() || IsNormalRateGame(); }

// TSA_IsPhoneCallScenario @ 0x6ba8fb38
bool IsPhoneCall() {
    const uint32_t sys = Static().systemFlags;
    return (sys & kSysPhoneCall) != 0 && (sys & kSysScreenLock) == 0;
}

// TSA_IsScreenLockScenario @ 0x6ba8fb7c：锁屏且不在通话；未开接近时还要求机型锁屏不做接近
bool IsScreenLock() {
    const uint32_t sys = Static().systemFlags;
    if ((sys & kSysPhoneCall) != 0 || (sys & kSysScreenLock) == 0) return false;
    return (Static().featureFlags & kFeatProximity) != 0 || (PrmtFlash().proxConfig & kProxScreenLockMask) == 0;
}

// TSA_ScenarioProcess @ 0x6ba904ed
// 场景优先级：输入 > 高帧率游戏 > 0x10 > 普通帧率游戏。手套：宿主未关闭、强按次数未满、且查温度或 HIMA
// 机型时，手套模式已在跟踪但未确认时降加速度门限并在单指时开手套，否则不超过两指时开手套。
void Process() {
    const FlashPrmt& fl = PrmtFlash();
    TsaStatic& s = Static();
    Afe::SetMaxScanRate(fl.maxScanRate);
    AppTypeToScenario();
    Set("dbEnterIDLE", fl.dbEnterIdleNormal);
    const uint32_t gate = s.aftGateFlags;
    if ((gate & kTyping) != 0) {
        TypingSetting();
    } else if ((gate & kGame) != 0) {
        GameSetting(true);
    } else if ((gate & 0x10) == 0 && (gate & kNormalRateGame) != 0) {
        GameSetting(false);
    }
    UpdateReportRate();
    const bool gloveAllowed = (s.systemFlags & kSysGloveOff) == 0 && s.strongTapCount < kStrongTapGloveLimit &&
                              ((s.aftGateFlags & kTempCheck) != 0 || IsProject("HIMA"));
    if (gloveAllowed) {
        Set("distToEnterGloveMode", fl.distToEnterGloveMode);
        const TsaPrpt& p = Prpt();
        const uint8_t prevCount = PrevTouches().count;
        if (p.touchMode == kTouchModeGlove && p.touchModeLevel != kTouchModeConfirmed) {
            Set("accThold", fl.accThold / 3);
            if (prevCount < 2) EnableGlove();
        } else if (prevCount < 3) {
            EnableGlove();
        }
    } else {
        Sys::RestoreExtFeatures(kFeatGloveMode);
    }
    UpdateScenarioMask();
    SideTouchSetting();
}

// TSA_EnableScenario @ 0x6ba8fa07 / TSA_DisableScenario @ 0x6ba8fa33
void Enable(uint32_t mask) { Static().aftGateFlags |= mask; }
void Disable(uint32_t mask) { Static().aftGateFlags &= ~mask; }

// TSA_SetFeatures @ 0x6ba9f179（握持过滤的开关在原代码里随即被覆盖，不起作用）
uint32_t SetFeatures(uint32_t features) {
    TsaStatic& s = Static();
    uint32_t fixed = features & ~kFeatExtMask;
    uint32_t ext = features & kFeatExtMask;
    const uint32_t extRequested = ext;
    uint32_t unsupported = 0;
    KeepIfUnchanged(ext, kFeatGloveMode);
    KeepIfUnchanged(ext, kFeatSmartCover);
    if ((features & kFeatUnsupported) != 0) {
        unsupported = 1;
        fixed &= ~kFeatUnsupported;
    }
    KeepIfUnchanged(ext, kFeatKnuckle);
    KeepIfUnchanged(ext, 0x800);
    s.featureFlags = fixed | ext | PrmtFlash().featureFlagsForce;
    s.extFeatureFlagsInit = extRequested;
    return unsupported;
}

// TSA_GetFeatures @ 0x6ba9f329
uint32_t GetFeatures() { return Static().featureFlags; }

// TSA_SetScreenRotateState @ 0x6ba9fea4
void SetScreenRotateState(int state) {
    if (IsProject("P167") || IsProject("W157")) state = state == 0 ? 3 : state - 1;
    Static().screenRotation = state;
}

}  // namespace Solvers::Tsa::Core::Scenario

// 宿主 API
extern "C" {
namespace Sc = Solvers::Tsa::Core::Scenario;
uint32_t TSA_SetFeatures(uint32_t features) { return Sc::SetFeatures(features); }
uint32_t TSA_GetFeatures(void) { return Sc::GetFeatures(); }
void TSA_EnableScenario(uint32_t mask) { Sc::Enable(mask); }
void TSA_DisableScenario(uint32_t mask) { Sc::Disable(mask); }
void TSA_SetScreenRotateState(int state) { Sc::SetScreenRotateState(state); }
}  // extern "C"
