#include "App/Afe.h"
#include "App/Sys.h"

#include <algorithm>
#include <cstring>

#include "Alg/Touch/TouchData.h"
#include "Alg/Touch/TouchGeometry.h"
#include "App/Report.h"
#include "App/Tsa.h"
#include "App/TsaStatic.h"
#include "Common/Bss.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::Sys {

namespace {

constexpr int64_t kNone = -1;
constexpr uint32_t kFeatSafeBaseline = 0x40;      // TsaStatic::featureFlags
constexpr uint32_t kFeatProximity = 0x4000;
constexpr uint32_t kFeatNoIdleTimeout = 0x8000;
constexpr uint32_t kFeatExtMask = 0x4813;         // 宿主可开关的特性位
constexpr uint32_t kNewExtHwResetDone = 0x200000;  // TsaStatic::newExtFlags
constexpr uint64_t kChargerHoldMs = 2000;
constexpr uint64_t kPhoneCallHoldMs = 1200;
constexpr uint64_t kAfeIdleExitHoldMs = 100;
constexpr uint64_t kIdleTimeoutMs = 4000;

// 一次性初始化与 IDLE 超时的时间戳（函数内静态量）
uint8_t g_initOnce = 1;   // flagOnceOnly_10264
uint8_t g_htiFirst = 1;   // flagOnceOnly_10424
uint8_t& InitOnce() { return g_initOnce; }
uint8_t& HtiFirst() { return g_htiFirst; }
uint64_t& IdleTimeoutStamp() { return Slot<uint64_t>(Bss().idleTimeoutStamp); }
uint64_t& SideIdleTimeoutStamp() { return Slot<uint64_t>(Bss().sideIdleTimeoutStamp); }

/// t != -1 且 now ∈ [t, t + span)。
bool Within(int64_t t, uint64_t now, uint64_t span) {
    return t != kNone && static_cast<uint64_t>(t) <= now && now < static_cast<uint64_t>(t) + span;
}

// TSAStatic_PhoneCallModeDebounce @ 0x6baa20fe
void PhoneCallModeDebounce() {
    TsaStatic& s = Static();
    bool inCall = false;
    if ((s.systemFlags & kSysPhoneCall) != 0) {
        inCall = true;
        s.phoneCallTime = static_cast<int64_t>(s.curTimeMs);
    } else if (Within(s.phoneCallTime, s.curTimeMs, kPhoneCallHoldMs) && IsProximityEnabled()) {
        inCall = true;
    }
    if (inCall) s.systemFlags |= kSysPhoneCall;
}

// TSAStatic_IsLargeDisplayNoise @ 0x6baa24f9
bool IsLargeDisplayNoise() {
    const FlashPrmt& fl = PrmtFlash();
    const TsaPrpt& prpt = Prpt();
    const TsaStatic& s = Static();
    const bool noisy = (fl.peakFilterReferenceThreshold * 2 < prpt.preCmfMaxAbsDif && fl.sensorNum >> 2 < prpt.preCmfOverCount) ||
                       s.prevAllFreqNoisy != 0;
    if (!noisy) return false;
    return (s.systemFlags & kSysChargerNoise) != 0 || Afe::IsAllFreqNoisy();
}

bool IsJustOkToEnterIdle() { return Static().idleReady != 0 && Static().prevIdleReady == 0; }
bool IsJustOkToEnterSideIdle() { return Static().sideIdleReady != 0 && Static().prevSideIdleReady == 0; }

// TSAStatic_UpdateIDLEModeOutput @ 0x6baa2aa7
// idleReady 持续 idleEnterDelayMs 之后才对外请求进 IDLE。
bool UpdateIdleModeOutput() {
    TsaStatic& s = Static();
    if (s.idleReady == 0) {
        s.idleReadyTime = kNone;
        return false;
    }
    if (IsJustOkToEnterIdle()) {
        s.idleReadyTime = static_cast<int64_t>(s.curTimeMs);
        return false;
    }
    return s.idleReadyTime != kNone && static_cast<uint64_t>(s.idleReadyTime) + PrmtFlash().idleEnterDelayMs < s.curTimeMs;
}

// TSAStatic_UpdateSideIDLEModeOutput @ 0x6baa2b5a
bool UpdateSideIdleModeOutput() {
    TsaStatic& s = Static();
    if (s.sideIdleReady == 0) {
        s.sideIdleReadyTime = kNone;
        return false;
    }
    if (IsJustOkToEnterSideIdle()) {
        s.sideIdleReadyTime = static_cast<int64_t>(s.curTimeMs);
        return false;
    }
    if (s.sideIdleReadyTime == kNone) {
        return false;
    }
    if (static_cast<uint64_t>(s.sideIdleReadyTime) + PrmtFlash().idleEnterDelayMs < s.curTimeMs) return true;
    return false;
}

// TSAStatic_HTIFilter @ 0x6baa2f56
// 第一个持续够 10 帧的可信触点直接取它的信号刻度，之后上升直接跟、下降按 15/16 IIR。
void HtiFilter(uint8_t idx) {
    const TouchUnit& u = Touches().unit[idx];
    TsaStatic& s = Static();
    const auto v = static_cast<int16_t>(std::min<int>(u.signalScale, PrmtFlash().peakFilterReferenceThreshold));
    if (HtiFirst() != 0) {
        if (9 < u.maxMajorMm) {
            s.htiThold = v;
            HtiFirst() = 0;
        }
        return;
    }
    if (s.htiThold < v) {
        s.htiThold = v;
    } else if (9 < u.maxMajorMm) {
        s.htiThold = static_cast<int16_t>((v + s.htiThold * 15) / 16);
    }
}

// TSAStatic_HTIUpdate @ 0x6baa3091
void HtiUpdate() {
    const TouchTable& tt = Touches();
    for (uint8_t k = 0; k < tt.count; ++k) {
        if (tt.unit[k].trustTag == 1) HtiFilter(k);
    }
}

// TSAStatic_InitSysModuleEveryTime @ 0x6baa15c6
void InitSysModule() {
    TsaStatic& s = Static();
    const FlashPrmt& fl = PrmtFlash();
    s.prevTimeMs = 0;
    s.frameCounter = 0;
    s.noiseLevel = 0;
    s.featureFlags = fl.featureFlagsForce | fl.featureFlagsDefault;
    s.prevFeatureFlags = s.featureFlags;
    s.extFeatureFlagsInit = s.featureFlags & kFeatExtMask;
    s.featureFlags2 = fl.featureFlags2Force | fl.featureFlags2Default;
    s.rfNoiseFrames = 0;
    s.maxRfNoiseFrames = 0;
    s.strongTapCount = 0;
    s.prevSystemFlags = 0;
    s.prevAfeStatus = 0;
    s.resetJustCleared = 0;
    s.noisyResetAfterReset = 0;
    s.signalReverted = 0;
    s.cmfBlocked = 0;
    s.newExtFlags = 0;
    s.prevExtFlags = 0;
    s.fullIncellSignalDisparity = 0;
    s.envTemp = 0;
    s.screenRotation = 0;
    s.frameDone = 0;
}

// TSAStatic_InitInternalModuleEveryTime @ 0x6baa18b1
void InitInternalModule() {
    TsaStatic& s = Static();
    s.pressCount = 0;
    s.allFreqNoisy = 0;
    s.okToIdle = 0;
    s.afeIdle = 0;
    s.afeSideIdle = 0;
    s.idleReady = 0;
    s.sideIdleReady = 0;
    s.noisyBlReset = 0;
    s.afeIdleExitTime = kNone;
    s.idleReadyTime = kNone;
    s.afeSideIdleExitTime = kNone;
    s.sideIdleReadyTime = kNone;
    s.idleDebounceStart = kNone;
    s.proxScreenOffTime = kNone;
    s.chargerTime = kNone;
    s.phoneCallTime = kNone;
    s.lastReportTime = kNone;
    s.msInitTime = static_cast<uint64_t>(kNone);
    s.fullIncellHighPeakTime = 0;
    s.autoCalibrationStartTime = 0;
    s.markedLiftOffTime = 0;
    s.fullIncellHighPeakFrames = 0;
    s.resetHoldFrames = 0;
    s.trustedTouchCount = 0;
    s.suspectTouchCount = 0;
    s.safeDif = 0;
    s.unsafeDif = 0;
}

}  // namespace

// TSAStatic_Init @ 0x6baa1a42（日志名写入恒成功）
void Init() {
    InitSysModule();
    InitInternalModule();
    TsaStatic& s = Static();
    std::strncpy(s.logName, "TSA_DUMP", sizeof(s.logName) - 1);
    s.logName[0x31] = '\0';
    if (InitOnce() != 0) {
        s.scenarioMask = 0;
        s.xMin = 0;
        s.xMax = static_cast<int16_t>(Report::XRes() - 1);
        s.yMin = 0;
        s.yMax = static_cast<int16_t>(Report::YRes() - 1);
        s.htiThold = PrmtFlash().peakFilterReferenceThreshold;
        s.proximityOnTime = kNone;
        s.proximityOffTime = kNone;
        s.aftGateFlags = 1;
        s.prevAftGateFlags = 1;
    }
    InitOnce() = 0;
}

// TSAStatic_PreProcess @ 0x6baa1bb0
void PreProcess() {
    TsaStatic& s = Static();
    s.prevPressCount = s.pressCount;
    s.prevAllFreqNoisy = s.allFreqNoisy;
    s.prevAfeIdle = s.afeIdle;
    s.prevIdleReady = s.idleReady;
    s.prevAfeSideIdle = s.afeSideIdle;
    s.prevSideIdleReady = s.sideIdleReady;
    s.afeIdle = Afe::IsInIdle() ? 1 : 0;
    s.afeSideIdle = Afe::IsInSideIdle();
    s.prevNoisyBlReset = s.noisyBlReset;
    s.signalReverted = s.wakeUpExceptionLatch;
    if (!Afe::IsProximityScreenOff()) {
        s.proxScreenOffTime = kNone;
    } else if (s.proxScreenOffTime == kNone) {
        s.proxScreenOffTime = static_cast<int64_t>(s.curTimeMs);
    }
    if (!IsProximityEnabled()) {
        if (s.proximityOnTime != kNone) s.proximityOffTime = static_cast<int64_t>(s.curTimeMs);
        s.proximityOnTime = kNone;
    } else {
        if (s.proximityOnTime == kNone) s.proximityOnTime = static_cast<int64_t>(s.curTimeMs);
        s.proximityOffTime = kNone;
    }
    s.frameIntervalMs = Touch::GetFormattedTimeInterval(static_cast<int16_t>(static_cast<int16_t>(s.curTimeMs) - static_cast<int16_t>(s.prevTimeMs)));
    s.afeStatus = Afe::FrameStatus();
}

// TSAStatic_PostProcess @ 0x6baa1f51
void PostProcess() {
    TsaStatic& s = Static();
    s.allFreqNoisy = IsAllFreqNoisy() ? 1 : 0;
    s.okToIdle = Tsa::IsOkToEnterIdle() ? 1 : 0;
    s.idleReady = Tsa::UpdateIdleMode() ? 1 : 0;
    s.sideIdleReady = Tsa::UpdateSideIdleMode() ? 1 : 0;
    s.idleRequest = UpdateIdleModeOutput() ? 1 : 0;
    s.sideIdleRequest = UpdateSideIdleModeOutput() ? 1 : 0;
    s.noisyBlReset = s.blNoisyReset;
    if (s.resetJustCleared != 0 && s.blNoisyReset != 0) {
        s.noisyResetAfterReset = 1;
        s.resetJustCleared = 0;
    } else {
        s.noisyResetAfterReset = 0;
    }
    s.blNoisyReset = 0;
    s.blHwResetRequest = 0;
    s.prevSystemFlags = s.systemFlags;
    s.prevTimeMs = s.curTimeMs;
    s.prevFeatureFlags = s.featureFlags;
    s.prevAfeStatus = s.afeStatus;
    s.prevExtFlags = s.extFlags;
    s.prevAftGateFlags = s.aftGateFlags;
}

// TSAStatic_UpdateCtrlFlags @ 0x6baa21cb
// 复位请求（bit 0/2/3）之后复位位再保持 3 帧；基线复位锁存期间不保持。
void UpdateCtrlFlags(uint32_t flags) {
    TsaStatic& s = Static();
    if ((flags & 8) != 0) s.autoCalibrationStartTime = s.curTimeMs;
    if ((flags & 4) != 0) {
        flags &= ~8u;
        s.lastReportTime = static_cast<int64_t>(s.curTimeMs);
    }
    if ((flags & 0xd) != 0) {
        s.resetHoldFrames = 3;
        s.blResetLatched = 0;
    }
    uint32_t next = flags;
    if (s.resetHoldFrames != 0 || s.blResetLatched == 0) {
        if (s.resetHoldFrames != 0) --s.resetHoldFrames;
        next = (flags & ~0xdu) | (s.systemFlags & 0xd) | (flags & 0xd);
    }
    if ((s.systemFlags & 0xd) == 0 || (next & 0xd) != 0) {
        s.resetFlagsCleared = 0;
    } else {
        s.resetFlagsCleared = 1;
        s.resetJustCleared = 1;
    }
    s.systemFlags = next;
    PhoneCallModeDebounce();
}

// TSAStatic_RestoreExtFeatures @ 0x6baa2d74
void RestoreExtFeatures(uint32_t mask) {
    TsaStatic& s = Static();
    const uint32_t m = mask & kFeatExtMask;
    s.featureFlags = (s.featureFlags & ~m) | (s.extFeatureFlagsInit & m);
}

// TSAStatic_IsHWReset @ 0x6baa2345
bool IsHwReset() { return (Static().systemFlags & kSysResetMask) != 0; }
// TSAStatic_IsHWResetPrev @ 0x6baa2373
bool IsHwResetPrev() { return (Static().prevSystemFlags & kSysResetMask) != 0; }

// TSAStatic_GetCurrTimeStamp @ 0x6baa2cfe：没有时间戳时按帧数推算
uint64_t CurrTimeStamp() {
    const TsaStatic& s = Static();
    if (s.curTimeMs != 0) return s.curTimeMs;
    return static_cast<uint64_t>(static_cast<int64_t>(Touch::GetFormattedTimeInterval(0)) * static_cast<uint64_t>(static_cast<uint32_t>(s.frameCounter)));
}

// TSAStatic_IsProximityEnabled @ 0x6baa2c95
bool IsProximityEnabled() {
    const TsaStatic& s = Static();
    return (PrmtFlash().proximityScenarios & s.scenarioMask) != 0 && (s.featureFlags & kFeatProximity) != 0;
}

// TSAStatic_IsAFTModeEnabled @ 0x6baa2cda
bool IsAftModeEnabled() { return (PrmtFlash().antiTouchScenarios & Static().scenarioMask) != 0; }

// TSAStatic_IsSafeBaselineApplied @ 0x6baa30f3（SafeBaseline 被裁掉，恒不可用）
bool IsSafeBaselineApplied() { return false; }

// TSAStatic_IsSignalReverted @ 0x6baa2c81
bool IsSignalReverted() { return Static().signalReverted != 0; }

// TSAStatic_IsFreqShiftDone @ 0x6baa23cd
bool IsFreqShiftDone() {
    const uint32_t flags = Static().systemFlags;
    return Afe::IsFreqShiftDone() || (flags & kSysFreqShiftDone) != 0;
}

// TSAStatic_IsScreenRotated @ 0x6baa2e39：90° / 270°
bool IsScreenRotated() {
    const TsaStatic& s = Static();
    return (s.systemFlags & kSysScreenRotated) != 0 || s.screenRotation == 1 || s.screenRotation == 3;
}

// TSAStatic_IsChargerPluggedNoDebounce @ 0x6baa24b0
bool IsChargerPluggedNoDebounce() {
    TsaStatic& s = Static();
    const bool plugged = (s.systemFlags & (kSysCharger | kSysCharger2)) != 0;
    if (plugged) s.chargerTime = static_cast<int64_t>(s.curTimeMs);
    return plugged;
}

// TSAStatic_IsChargerPlugged @ 0x6baa2434
bool IsChargerPlugged() {
    if (IsChargerPluggedNoDebounce()) return true;
    const TsaStatic& s = Static();
    return Within(s.chargerTime, s.curTimeMs, kChargerHoldMs);
}

// TSAStatic_IsAllFreqNoisy @ 0x6baa2590
bool IsAllFreqNoisy() {
    TsaStatic& s = Static();
    if (!IsChargerPlugged()) {
        s.allFreqNoisy = IsLargeDisplayNoise() ? 1 : 0;
    } else {
        if ((s.systemFlags & kSysChargerNoise) != 0) s.allFreqNoisy = 1;
        if (Afe::IsAllFreqNoisy()) s.allFreqNoisy = 1;
    }
    return s.allFreqNoisy != 0;
}

// TSAStatic_IsAllFreqBecomeNoisy @ 0x6baa260a
bool IsAllFreqBecomeNoisy() {
    TsaStatic& s = Static();
    s.allFreqNoisy = IsAllFreqNoisy() ? 1 : 0;
    return s.allFreqNoisy != 0 && s.prevAllFreqNoisy == 0;
}

// TSAStatic_IsAFEJustEnterIDLEMode @ 0x6baa26ce
bool IsAfeJustEnterIdle() { return Static().afeIdle != 0 && Static().prevAfeIdle == 0; }
// TSAStatic_IsAFEJustExitIDLEMode @ 0x6baa265a
bool IsAfeJustExitIdle() { return Static().afeIdle == 0 && Static().prevAfeIdle != 0; }
// TSAStatic_IsAFEJustExitSideIDLEMode @ 0x6baa2694
bool IsAfeJustExitSideIdle() { return Static().afeSideIdle == 0 && Static().prevAfeSideIdle != 0; }

// TSAStatic_IsAFEEnterIdleTimeout @ 0x6baa2742
// idleReady 之后 idleEnterDelayMs + 4 s 仍未进 IDLE，记一次超时（本帧返回真）。
bool IsAfeEnterIdleTimeout() {
    const TsaStatic& s = Static();
    if ((s.featureFlags & kFeatNoIdleTimeout) != 0) return false;
    if (s.idleReadyTime != kNone && !Afe::IsInIdle() &&
        static_cast<uint64_t>(s.idleReadyTime) + PrmtFlash().idleEnterDelayMs + kIdleTimeoutMs < s.curTimeMs) {
        IdleTimeoutStamp() = s.curTimeMs;
    }
    return s.curTimeMs == IdleTimeoutStamp();
}

// TSAStatic_IsAFEEnterSideIdleTimeout @ 0x6baa27ff
bool IsAfeEnterSideIdleTimeout() {
    const TsaStatic& s = Static();
    if ((s.featureFlags & kFeatNoIdleTimeout) != 0) return false;
    if (s.sideIdleReadyTime != kNone && !Afe::IsInSideIdle() &&
        static_cast<uint64_t>(s.sideIdleReadyTime) + PrmtFlash().idleEnterDelayMs + kIdleTimeoutMs < s.curTimeMs) {
        SideIdleTimeoutStamp() = s.curTimeMs;
    }
    return s.curTimeMs == SideIdleTimeoutStamp();
}

// TSAStatic_DoesAFERequireToExitIDLEMode @ 0x6baa28bc：AFE 刚退出 IDLE 后 100 ms 内
bool DoesAfeRequireToExitIdle() {
    TsaStatic& s = Static();
    if (IsAfeJustExitIdle()) {
        s.afeIdleExitTime = static_cast<int64_t>(s.curTimeMs);
        return true;
    }
    if (!Within(s.afeIdleExitTime, s.curTimeMs, kAfeIdleExitHoldMs)) return false;
    return true;
}

// TSAStatic_DoesAFERequireToExitSideIDLEMode @ 0x6baa298f
bool DoesAfeRequireToExitSideIdle() {
    TsaStatic& s = Static();
    if (IsAfeJustExitSideIdle()) {
        s.afeSideIdleExitTime = static_cast<int64_t>(s.curTimeMs);
        return true;
    }
    return Within(s.afeSideIdleExitTime, s.curTimeMs, kAfeIdleExitHoldMs);
}

// TSAStatic_IsOKToEnterIDLEMode @ 0x6baa2a96
bool IsOkToEnterIdle() { return Static().idleRequest != 0; }
// TSAStatic_IsOKToEnterSideIDLEMode @ 0x6baa2b49
bool IsOkToEnterSideIdle() { return Static().sideIdleRequest != 0; }

// TSAStatic_GetSigThold @ 0x6baa2f14
uint32_t GetSigThold(int mode) {
    if (mode == 1) return static_cast<uint32_t>(Static().htiThold * 2 / 5);
    return static_cast<uint16_t>(PrmtDynamic().sigThold);
}

// TSAStatic_SafeDiffCheck @ 0x6baa1de6
// raw 稳定、本帧没有报点、且曾确认过可信触点时 Dif 可信；否则看上一帧的异常形状触点多不多。
void SafeDiffCheck() {
    TsaStatic& s = Static();
    if (s.resetDiffRequested == 0 && !Afe::IsRawUnstable() && !Afe::IsRawJump()) {
        if (s.trustedTouchCount != 0 && Touches().reportedThisFrame == 0 && Prpt().msDifInner.count < 0x32) {
            s.safeDif = 1;
            s.unsafeDif = 0;
        }
    } else {
        s.trustedTouchCount = 0;
        s.suspectTouchCount = 0;
        s.safeDif = 0;
    }
    if (s.safeDif != 0) return;
    const PrevTouchTable& prev = PrevTouches();
    if (3 < static_cast<int>(prev.count) - prev.gripCount && prev.abnormalShapeCount != 0) s.unsafeDif = 1;
    if (2 < prev.abnormalShapeCount) s.unsafeDif = 1;
    if ((s.featureFlags & kFeatSmartCover) == 0 && prev.abnormalShape3Count != 0) s.unsafeDif = 1;
}

// TSAStatic_ProcessAftTouchEvent @ 0x6baa316a
void ProcessAftTouchEvent() {
    HtiUpdate();
    SafeDiffCheck();
}

}  // namespace Solvers::Tsa::Core::Sys
