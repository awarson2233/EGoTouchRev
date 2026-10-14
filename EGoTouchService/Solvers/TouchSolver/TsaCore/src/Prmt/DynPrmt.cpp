#include "Prmt/TsaPrmt.h"

#include <cstring>

#include "Alg/TouchThold/TouchThold.h"
#include "App/TsaStatic.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::DynPrmt {

namespace {

constexpr uint32_t kSysCmfTholdAlt = 0x40000;  // TsaStatic::systemFlags bit18：CMF 用备用门限组

/// 初始化时从机型参数原样复制到动态门限的几段（两边的字段排布相同）。
struct InitBlock {
    size_t dynOffset;
    size_t flashOffset;
    size_t size;
};
constexpr InitBlock kInitBlocks[] = {
    {offsetof(DynamicPrmt, touchThold), offsetof(FlashPrmt, gloveExitThold), 0x24},
    {offsetof(DynamicPrmt, defaultSizeMm), offsetof(FlashPrmt, defaultSizeMm), 0x1},
    {offsetof(DynamicPrmt, defaultTouchAction), offsetof(FlashPrmt, defaultTouchAction), 0x10},
    {offsetof(DynamicPrmt, zTholdFlags), offsetof(FlashPrmt, zTholdFlags), 0xc},
    {offsetof(DynamicPrmt, field024), offsetof(FlashPrmt, field608), 0x1e},  // 含 CMF 门限的常态组
    {offsetof(DynamicPrmt, chargerNoiseTholdPercent), offsetof(FlashPrmt, chargerNoiseTholdPercent), 0xc},
};

bool IdleScan() { return (Static().processingState & kProcIdleScan) != 0; }

// (2 × v) / 3，向零取整（原代码用乘 0x55555556 取高 32 位实现）。
int16_t TwoThirds(int16_t v) { return static_cast<int16_t>(v * 2 / 3); }

// v / divisor（向零取整）限制在 1..255。
uint8_t StepOf(int16_t v, int divisor) {
    int16_t step = static_cast<int16_t>(v / divisor);
    if (step < 1) step = 1;
    else if (step > 0xff) step = 0xff;
    return static_cast<uint8_t>(step);
}

// TSADynPrmt_GetUnderWaterThold @ 0x6bb3d023
void GetUnderWaterThold() {
    DynamicPrmt& dyn = PrmtDynamic();
    const int16_t ref = PrmtFlash().peakFilterReferenceThreshold;
    dyn.underWaterThold = static_cast<int16_t>(ref / 2 * 3);
    dyn.underWaterTholdLow = static_cast<int16_t>(ref / 2);
}

// TSADynPrmt_SSInit @ 0x6bb3c6a0
void SsInit() {
    DynamicPrmt& dyn = PrmtDynamic();
    const int16_t finger = PrmtFlash().fingerThold;
    dyn.ssThold = finger;
    dyn.ssTholdLow = static_cast<int16_t>(finger / 3);
}

// TSADynPrmt_GetPkThold @ 0x6bb3cd91
// 先按 0.7 倍 finger 门限算一组边缘门限，再按 finger 门限算本帧的峰 / 触点 / 按下门限（结果在 TsaPrpt），
// Zn 门限按峰门限与机型给的比例换算。
void GetPkThold() {
    const FlashPrmt& flash = PrmtFlash();
    DynamicPrmt& dyn = PrmtDynamic();
    TsaPrpt& prpt = Prpt();
    int16_t edgePeak = 0, edgeTouch = 0, edgeTouchDown = 0;
    TouchThold::Process(static_cast<int16_t>(flash.fingerThold * 0x38 / 0x50), flash.gloveThold, &edgePeak, &edgeTouch,
                        &edgeTouchDown);
    dyn.edgePeakThold = edgePeak;
    dyn.edgePeakTholdLow = edgeTouch;
    TouchThold::Process(flash.fingerThold, flash.gloveThold, &prpt.peakThold, &prpt.touchThold, &prpt.touchDownThold);
    dyn.sdZnTholdScaleX16 = flash.sdZnTholdScaleX16;
    dyn.touchThold = flash.peakFilterReferenceThreshold;
    dyn.sigThold = prpt.peakThold;
    dyn.peakThold = prpt.touchThold;
    dyn.znTouchDownThold = static_cast<int32_t>(static_cast<uint32_t>(dyn.sdZnTholdScaleX16) * prpt.touchThold) / 16;
    dyn.znFingerThold = prpt.fingerTouchThold * 3;
    dyn.znTouchDownThold2 = static_cast<int32_t>(static_cast<uint32_t>(dyn.sdZnTholdScaleX16) * prpt.touchDownThold) / 16;
}

// TSADynPrmt_UpdateNoiseTholdBfrTch @ 0x6bb3c849
// 触点出现前的小门限：一般取噪声门限的一半；脚趾用特殊门限时取 2/3，处于手套确认态（且无智能皮套）时两个都取 2/3。
void UpdateNoiseTholdBeforeTouch(int16_t noise, int16_t sideNoise) {
    DynamicPrmt& dyn = PrmtDynamic();
    const TsaPrpt& prpt = Prpt();
    if (!TouchThold::ToeUseSpecialThold() || prpt.field2E8 != 2 || prpt.field2EC != 2 ||
        (Static().featureFlags & kFeatSmartCover) != 0) {
        dyn.smallThold = !TouchThold::ToeUseSpecialThold() ? static_cast<int16_t>(noise / 2) : TwoThirds(noise);
        dyn.sideSmallThold = static_cast<int16_t>(sideNoise / 2);
    } else {
        dyn.smallThold = TwoThirds(noise);
        dyn.sideSmallThold = TwoThirds(sideNoise);
    }
}

// TSADynPrmt_UpdateBfrTch @ 0x6bb3c960
// 触点出现前的基线步长：噪声门限 / 256；IDLE 扫描时加倍。
void UpdateBeforeTouch(int16_t noise, int16_t sideNoise) {
    DynamicPrmt& dyn = PrmtDynamic();
    dyn.stepNoTouch = StepOf(noise, 0x100);
    dyn.stepNoTouchSide = StepOf(sideNoise, 0x100);
    dyn.field04E = 1;
    if (IdleScan()) dyn.stepNoTouch = static_cast<uint8_t>(dyn.stepNoTouch * 2);
    dyn.stageTimeoutMs = 0x198;
}

// TSADynPrmt_UpdateStepAftTch @ 0x6bb3ca49
// 触点出现后的钳位与步长：噪声门限 / 64；IDLE 扫描时加倍，超过 127 的取 255。
void UpdateStepAfterTouch(int16_t noise, int16_t sideNoise) {
    DynamicPrmt& dyn = PrmtDynamic();
    dyn.clampAfterTouch = static_cast<int16_t>(noise - 1);
    dyn.clampAfterTouchSide = static_cast<int16_t>(sideNoise - 1);
    dyn.stepAfterTouch = StepOf(noise, 0x40);
    dyn.field051 = 1;
    if (IdleScan()) {
        dyn.stepAfterTouch = static_cast<int8_t>(dyn.stepAfterTouch) < 0 ? 0xff : static_cast<uint8_t>(dyn.stepAfterTouch * 2);
    }
    const uint8_t rate = PrmtFlash().reportRateHz;
    dyn.halfReportRate = static_cast<uint8_t>(rate >> 1);
    dyn.reportRate = rate;
}


// TSADynPrmt_GetBL @ 0x6bb3cb4f
// 本帧基线更新用的噪声门限与步长。
void GetBl() {
    const FlashPrmt& flash = PrmtFlash();
    DynamicPrmt& dyn = PrmtDynamic();
    const TsaPrpt& prpt = Prpt();
    int16_t noise;
    int16_t sideNoise;
    if (TouchThold::UseFingerThold()) {
        noise = prpt.fingerTouchThold;
        sideNoise = static_cast<int16_t>(prpt.fingerTouchThold * 3 / 4);
    } else {
        noise = prpt.touchThold;
        sideNoise = prpt.touchThold;
    }
    if (!IdleScan()) {
        UpdateNoiseTholdBeforeTouch(noise, sideNoise);
    } else {
        noise = flash.sideSigThold;
        dyn.smallThold = TwoThirds(noise);
    }
    dyn.noiseThold = noise;
    dyn.sideNoiseThold = sideNoise;
    UpdateBeforeTouch(noise, sideNoise);
    UpdateStepAfterTouch(noise, sideNoise);
    dyn.resetCounterCap = static_cast<int16_t>(flash.reportRateHz * 200);
    dyn.field05A = dyn.resetCounterCap;
    dyn.refThold = flash.peakFilterReferenceThreshold;
    dyn.negRefThold = static_cast<int16_t>(-dyn.refThold);
}

// TSADynPrmt_SideProcess @ 0x6bb3cf6e
// IDLE 扫描时只按侧边门限设峰门限与 Zn 门限。
void SideProcess(int16_t thold) {
    DynamicPrmt& dyn = PrmtDynamic();
    dyn.sigThold = static_cast<int16_t>(thold * 3 / 4);
    dyn.peakThold = thold;
    dyn.znTouchDownThold = static_cast<int32_t>(static_cast<uint32_t>(dyn.sdZnTholdScaleX16 >> 4) * thold) / 2;
    dyn.znFingerThold = dyn.znTouchDownThold;
    dyn.znTouchDownThold2 = dyn.znTouchDownThold;
    dyn.sigRatioToDirectReport = PrmtFlash().sideSigRatioToDirectReport;
}

}  // namespace

// TSADynPrmt_Init @ 0x6bb3c6f2
void Init() {
    auto* dyn = reinterpret_cast<uint8_t*>(&PrmtDynamic());
    const auto* flash = reinterpret_cast<const uint8_t*>(&PrmtFlash());
    for (const InitBlock& b : kInitBlocks) std::memcpy(dyn + b.dynOffset, flash + b.flashOffset, b.size);
    PrmtDynamic().field05A = 2000;
    PrmtDynamic().resetCounterCap = 2000;
    TouchThold::Init();
    GetUnderWaterThold();
    SsInit();
}

// TSADynPrmt_PreProcess @ 0x6bb3d1e7 → TSADynPrmt_GetCMFCoef @ 0x6bb3d0af
void PreProcess() {
    const FlashPrmt& flash = PrmtFlash();
    PrmtDynamic().cmfThold = (Static().systemFlags & kSysCmfTholdAlt) == 0 ? flash.cmfTholdNormal : flash.cmfTholdAlt;
}

// TSADynPrmt_Process @ 0x6bb3d12d
// 非 IDLE 扫描：TSADynPrmt_CoreProcess（GetSig + GetBL）。GetSig 里的 GetHTI / GetLTI / GetMFLTI / GetTZThold 是空函数。
// IDLE 扫描：按音量键灵敏度把侧边门限打折后只设峰门限。
void Process() {
    if (!IdleScan()) {
        GetPkThold();
        GetUnderWaterThold();
        GetBl();
        return;
    }
    const FlashPrmt& flash = PrmtFlash();
    const int16_t side = flash.sideSigThold;
    if (flash.volumeSensitivityLevel == 1) {
        SideProcess(static_cast<int16_t>(side * 3 / 4));
    } else if (flash.volumeSensitivityLevel == 2) {
        SideProcess(static_cast<int16_t>(side / 2));
    } else {
        SideProcess(side);
    }
}

}  // namespace Solvers::Tsa::Core::DynPrmt
