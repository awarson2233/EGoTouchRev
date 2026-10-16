// 基线状态机（原 Baseline/BLSM.c，原厂 src/Alg/Baseline/BaselineStateMachine.c）。

#include <algorithm>
#include <cstdint>
#include <cstdlib>

#include "Alg/Baseline/Baseline.h"
#include "Alg/Baseline/BaselineInternal.h"
#include "Alg/SelfData/SelfData.h"
#include "Alg/Touch/TouchData.h"
#include "Alg/TouchThold/TouchThold.h"
#include "App/Afe.h"
#include "App/Scenario.h"
#include "App/Sys.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Common/Assert.h"
#include "Common/Bss.h"
#include "Prmt/Prmt.h"
#include "WorkAround/ToeSynaBl/ToeSynaBl.h"

namespace Solvers::Tsa::Core::Baseline {

namespace {

constexpr uint32_t kSysResetMask = 0x7;           // systemFlags 的复位位
constexpr uint32_t kSysResetMask2 = 0x800000;
constexpr uint32_t kAfeRawUnstable = 0x40000;     // TsaStatic::afeStatus
constexpr uint32_t kPersistNotEdge = 0x200;       // PrevTouchUnit::persistFlags
constexpr uint32_t kShbCapture = 0x1;             // TsaPrpt::shbFlags
constexpr uint32_t kShbCaptureLong = 0x2;
constexpr uint64_t kShbStateMs = 5000;
constexpr int16_t kToeSuspectSig = 0x514;         // 基线可疑时极值在 ±1300 以内改看 ToeSyna 的属性
constexpr int16_t kRawUnstableStep = 0x11;

/// 本帧是否按「侧边相邻列」的小门限处理（0x6bb7f020）。
uint8_t& NextEdgeSmallThold() { return Slot<uint8_t>(Bss().blsmNextEdgeSmallThresh); }

bool ToeUseSpecialThold() { return TouchThold::ToeUseSpecialThold(); }
bool IsGame() { return Scenario::IsGame(); }
bool SafeBaselineApplied() { return static_cast<char>(Sys::IsSafeBaselineApplied()) != 0; }

/// 列 col 是侧边相邻的列（第 1 列或倒数第 2 列）。
bool IsNextToEdge(uint8_t col) { return col == 1 || col == PrmtFlash().dim1Len - 2; }

// BLSM_ProcessUseNextSideThold @ 0x6ba4cf41
bool UseNextSideThold() { return ToeUseSpecialThold() && IsNextToEdge(Prpt().dif.minDim1); }

// BLSM_GetNoTouchBlStep @ 0x6ba4cf9c
// 无触摸时的门限与步长：侧边相邻列用侧边的一组。IDLE 扫描时步长改按极值的 1/16，不超过门限的 1/4。
void NoTouchStep(int16_t& thold, int16_t& step) {
    const DynamicPrmt& dyn = PrmtDynamic();
    uint8_t s = dyn.stepNoTouch;
    if (NextEdgeSmallThold() != 0 || UseNextSideThold()) {
        thold = dyn.sideSmallThold;
        s = dyn.stepNoTouchSide;
    }
    if (Static().afeIdle == 0) {
        step = s;
        return;
    }
    const TsaPrpt& prpt = Prpt();
    const int peak = std::max<int>(prpt.dif.maxSig, -prpt.dif.minSig);
    step = static_cast<int16_t>(static_cast<int16_t>(peak) / 16);
    if (thold / 4 < step) step = static_cast<int16_t>(thold / 4);
}

// BLSM_SaveNormalBl @ 0x6ba4d0c3
void SaveNormalBl() {
    if (Static().wakeUpExceptionLatch != 0 || PrevTouches().exceptionFlags == 0x2000) return;
    CopyChecked(Buffers().blNormal, 0x2648, Buffers().bl, PrmtFlash().sensorNum * size_t{2});
}

// BLSM_Reset @ 0x6ba4cf23（SS_Reset 被裁掉）
void ResetAll() {
    Iir::Reset();
    Self::Reset();
}

// BLSM_GetProperty @ 0x6ba4d523
// 复位请求 > 刷新请求 > BLReset 触发 > 噪声 > 防抖 > 按极值判有无触摸。
void GetProperty(bool resetReq, bool refreshReq) {
    const int16_t maxSig = MaxSig();
    TsaStatic& stat = Static();
    TsaPrpt& prpt = Prpt();
    const DynamicPrmt& dyn = PrmtDynamic();
    uint32_t flags = 0;
    prpt.prevBlPropertyState = prpt.blPropertyState;
    prpt.blPropertyState = 0;
    NextEdgeSmallThold() = 0;
    if ((stat.systemFlags & kSysResetMask) != 0) stat.blResetRefTime = stat.curTimeMs;
    if ((stat.systemFlags & kSysResetMask2) != 0) stat.blResetRefTime = stat.curTimeMs;
    if (resetReq) {
        stat.blResetLatched = 1;
        flags = kPropReset;
        if ((stat.systemFlags & 5) != 0) {
            prpt.blPropertyState = 1;
        } else {
            prpt.blPropertyState = (stat.systemFlags & 8) != 0 ? 2 : 3;
        }
    } else if (refreshReq) {
        flags = kPropRefresh;
    } else if (Reset::Process(false)) {
        flags = kPropReset;
    } else if (ResetInDebounce()) {
        flags = kPropDebounce;
    } else {
        if (ToeUseSpecialThold() && IsNextToEdge(prpt.dif.maxDim1)) NextEdgeSmallThold() = 1;
        const bool edge = NextEdgeSmallThold() != 0;
        flags = kPropTouched;
        if (maxSig < dyn.noiseThold && !Afe::IsCoreTouchDetected() &&
            (!edge || maxSig < dyn.sideNoiseThold)) {
            flags = (dyn.smallThold <= maxSig || (edge && dyn.sideSmallThold <= maxSig)) ? kPropSmallSig : 0;
        }
    }
    prpt.blPropertyFlags = flags;
}

// BLSM_UpdateStage @ 0x6ba4c6f1
// 属性里的复位 / 噪声 / 防抖 / 负信号 / 刷新直接决定阶段；否则按当前阶段与有无触摸、阶段计时推进。
// 换阶段时阶段帧数从 1、阶段计时从一个帧间隔重新算起。
void UpdateStage() {
    TsaPrpt& prpt = Prpt();
    const TsaStatic& stat = Static();
    const uint32_t flags = prpt.blPropertyFlags;
    prpt.prevBlStage = prpt.blStage;
    if ((flags & kPropReset) != 0) {
        prpt.blStage = kStageReset;
        return;
    }
    if ((flags & kPropDebounce) != 0) {
        prpt.blStage = kStageHold;
        return;
    }
    if ((flags & kPropNegative) != 0) {
        prpt.blStage = kStageNegative;
        return;
    }
    if ((flags & kPropRefresh) != 0) {
        prpt.blStage = kStageHold;
        return;
    }
    uint32_t next = prpt.blStage;
    ++prpt.blStageFrames;
    prpt.blStageTime = stat.curTimeMs + (prpt.blStageTime - stat.prevTimeMs);
    const auto restart = [&](uint32_t stage) {
        prpt.blStageFrames = 1;
        prpt.blStageTime = Interval<uint64_t>();
        next = stage;
    };
    const bool touched = (flags & kPropTouched) != 0;
    const bool timeout = static_cast<uint16_t>(PrmtDynamic().stageTimeoutMs) <= prpt.blStageTime;
    switch (prpt.blStage) {
    case kStageNoTouch:
        if (touched) {
            restart(kStageTouched);
        } else if ((flags & kPropSmallSig) != 0) {
            restart(kStageSmallSig);
        }
        break;
    case kStageReset:
    case kStageNoisyReset:
    case kStageHold:
        restart(kStageNoTouch);
        break;
    case kStageSmallSig:
    case kStage8:
    case kStageNegative:
        if (touched) {
            restart(kStageTouched);
        } else if (timeout) {
            restart(kStageNoTouch);
        }
        break;
    case kStageTouched: {
        if (!touched) restart(Static().singleTouchLatch != 0 ? kStageSmallSig : kStageLiftOff);
        const PrevTouchTable& prev = PrevTouches();
        Static().singleTouchLatch = prev.count == 1 && (prev.unit[0].persistFlags & kPersistNotEdge) == 0;
        break;
    }
    case kStageLiftOff:
        if (touched) {
            restart(kStageTouched);
        } else if (timeout) {
            restart(kStageAfterTouch);
        }
        break;
    case kStageAfterTouch: {
        const int16_t thold = NextEdgeSmallThold() != 0 ? PrmtDynamic().sideSmallThold : PrmtDynamic().smallThold;
        if (touched) {
            restart(kStageTouched);
        } else if (timeout || (prpt.dif.maxSig < thold && -prpt.dif.minSig < thold)) {
            restart(kStageNoTouch);
        }
        break;
    }
    default:
        prpt.blStageFrames = 1;
        prpt.blStageTime = Interval<uint64_t>();
        break;
    }
    prpt.blStage = next;
}

// BLSM_ProcessStage @ 0x6ba4d170
void ProcessStage() {
    TsaStatic& stat = Static();
    TsaPrpt& prpt = Prpt();
    const DynamicPrmt& dyn = PrmtDynamic();
    stat.blUpdatedThisFrame = 0;
    stat.resetDiffRequested = 0;
    stat.blIirUpdated = 0;
    switch (prpt.blStage) {
    case kStageNoTouch: {
        int16_t thold = dyn.smallThold;
        int16_t step = 0;
        NoTouchStep(thold, step);
        Iir::Update(false, thold, step);
        stat.blUpdatedThisFrame = 1;
        SaveNormalBl();
        break;
    }
    case kStageReset:
    case kStageNoisyReset:
        stat.resetDiffRequested = 1;
        ResetAll();
        break;
    case kStageHold:
        prpt.blStage = prpt.prevBlStage == kStageReset || prpt.prevBlStage == kStageNoisyReset || prpt.prevBlStage == kStageHold
                           ? static_cast<uint32_t>(kStageNoTouch)
                           : prpt.prevBlStage;
        break;
    case kStageAfterTouch:
    case kStage8: {
        const bool side = NextEdgeSmallThold() != 0 || UseNextSideThold();
        Iir::Update(false, side ? dyn.clampAfterTouchSide : dyn.clampAfterTouch, dyn.stepAfterTouch);
        stat.blUpdatedThisFrame = 1;
        break;
    }
    case kStageNegative: {
        // 负信号的 1/16 为步长，不超过小门限的 1/4
        auto step = static_cast<int16_t>(-prpt.dif.minSig / 16);
        if (dyn.smallThold / 4 < step) step = static_cast<int16_t>(dyn.smallThold / 4);
        Iir::Update(true, static_cast<int16_t>(-prpt.dif.minSig), step);
        stat.blUpdatedThisFrame = 1;
        break;
    }
    default:
        break;
    }
    if (stat.resetDiffRequested == 0) {
        Iir::CalcDiff();
    } else {
        Iir::ResetDiff();
    }
}

// BLSM_ShbIsRawChanges @ 0x6ba4c382
// 抓取的 raw 与当前 raw 有一格相差超过 finger 门限的一半。
// 结果只看有没有，按 16 格一块无分支地判（可向量化），有一块命中就返回。
bool ShbIsRawChanged() {
    const TsaBufferTable& buf = Buffers();
    const int n = PrmtFlash().sensorNum;
    const int limit = PrmtFlash().fingerThold / 2;
    const int16_t* capture = buf.rawCapture;
    const int16_t* raw = buf.raw;
    constexpr int kChunk = 16;
    for (int base = 0; base < n; base += kChunk) {
        const int end = std::min(base + kChunk, n);
        bool hit = false;
        for (int i = base; i < end; ++i) hit |= limit < std::abs(static_cast<int16_t>(capture[i] - raw[i]));
        if (hit) return true;
    }
    return false;
}

// BLSM_ShbCaptureBl @ 0x6ba4c422
void ShbCaptureBl() {
    CopyChecked(Buffers().rawCapture, GridBufSize() * size_t{2}, Buffers().preCmfRaw, PrmtFlash().sensorNum * size_t{2});
    Prpt().shbStateStartTime = Static().curTimeMs;
}

// BLSM_ShbProcess @ 0x6ba4c4cc
// 基线复位或 raw 变化时重新开始；之后每 5 s 抓一次 CMF 前的 raw，第二段起同时置长期标志。
void ShbProcess() {
    TsaPrpt& prpt = Prpt();
    if (IsReset()) prpt.shbState = 0;
    if (ShbIsRawChanged()) prpt.shbState = 0;
    const bool due = prpt.shbStateStartTime + kShbStateMs <= Static().curTimeMs;
    if (prpt.shbState == 1) {
        if (due) {
            prpt.shbState = 2;
            prpt.shbFlags |= kShbCapture;
        }
    } else if (prpt.shbState == 2) {
        if (due) prpt.shbFlags |= kShbCapture | kShbCaptureLong;
    } else {
        prpt.shbState = 1;
        prpt.shbFlags |= kShbCapture;
    }
    if ((prpt.shbFlags & kShbCapture) != 0) ShbCaptureBl();
}

}  // namespace

// BLSM_Init @ 0x6ba4c220（BLIIR_Init 为空）
void Init(bool clearResetLatch) {
    Reset::Init(clearResetLatch);
    Recal::Init();
    TsaPrpt& prpt = Prpt();
    prpt.blStage = kStageNoTouch;
    prpt.blStageFrames = 1;
    prpt.blStageTime = 0;
}

// BLSM_PreProcess @ 0x6ba4c36b
void PreProcess() { Prpt().shbFlags = 0; }

// BLSM_Process @ 0x6ba4c645
void Process(bool hwReset, bool refresh) {
    Recal::Process();
    const TsaStatic& stat = Static();
    GetProperty(hwReset || stat.blHwResetRequest != 0, refresh || stat.blRefreshRequest != 0);
    UpdateStage();
    ProcessStage();
    ShbProcess();
}

// BLSM_UpdateForRawUnstable @ 0x6ba4c27b
// 非游戏场景下 raw 不稳定时，每格按 0x11 的步长把基线拉向 CMF 前的 raw；
// 用安全基线时 raw 高出基线 finger 门限以上的格不动。
void UpdateForRawUnstable() {
    if (IsGame() || (Static().afeStatus & kAfeRawUnstable) == 0) return;
    const TsaBufferTable& buf = Buffers();
    const auto* src = reinterpret_cast<const uint16_t*>(buf.preCmfRaw);
    auto* bl = reinterpret_cast<uint16_t*>(buf.bl);
    const int limit = SafeBaselineApplied() ? PrmtFlash().fingerThold : INT32_MAX;
    const uint16_t n = PrmtFlash().sensorNum;
    for (uint16_t i = 0; i < n; ++i) {
        const int d = src[i] - bl[i];
        if (d >= limit) continue;
        if (d >= kRawUnstableStep) {
            bl[i] = static_cast<uint16_t>(bl[i] + kRawUnstableStep);
        } else if (d < -(kRawUnstableStep - 1)) {
            bl[i] = static_cast<uint16_t>(bl[i] - kRawUnstableStep);
        }
    }
}

// BLSM_IsReset @ 0x6ba4d794
bool IsReset() { return Prpt().blStage == kStageReset; }

// BLSM_IsTouched @ 0x6ba4d7ad
bool IsTouched() { return Prpt().blStage == kStageTouched; }

// BLSM_IsOKToIDLE @ 0x6ba4d7fd
bool IsOkToIdle() {
    const uint32_t stage = Prpt().blStage;
    if (PrmtFlash().fastNoObjExit == 0) return stage == kStageNoTouch;
    return stage == kStageNoTouch || stage == kStageLiftOff || stage == kStageAfterTouch || stage == kStageSmallSig;
}

// BLSM_IsBaselineOK @ 0x6ba4d7c6
bool IsBaselineOk() {
    const uint32_t flags = Prpt().blPropertyFlags;
    return flags == 0 || flags == kPropDebounce;
}

// BLSM_GetMaxSig @ 0x6ba4d41d
// 正向极值；不用安全基线时，基线可疑的 Toe 机型在极值不大时改看 difCore.maxSig，再交给 ToeSyna 的属性。
int16_t MaxSig() {
    int16_t v = Prpt().dif.maxSig;
    if (SafeBaselineApplied()) return v;
    if (ToeUseSpecialThold() && !IsGame() && Static().blSuspect != 0 && v < kToeSuspectSig) v = Prpt().difCore.maxSig;
    return ToeSynaBl::PropertyMaxSig(v);
}

// BLSM_GetMinSig @ 0x6ba4d4a0
int16_t MinSig() {
    int16_t v = Prpt().dif.minSig;
    if (SafeBaselineApplied()) return v;
    if (ToeUseSpecialThold() && !IsGame() && Static().blSuspect != 0 && -kToeSuspectSig < v) v = Prpt().difCore.minSig;
    return ToeSynaBl::PropertyMinSig(v);
}

}  // namespace Solvers::Tsa::Core::Baseline
