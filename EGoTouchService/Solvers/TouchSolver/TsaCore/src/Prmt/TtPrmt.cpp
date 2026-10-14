#include "Prmt/TsaPrmt.h"

#include <array>

#include "Prmt/Prmt.h"
#include "Prmt/TuningField.h"

namespace Solvers::Tsa::Core::TtPrmt {

namespace {

using Tuning::Bounds;
using Tuning::EntrySpec;
using Tuning::FieldRef;

#define FLASH(member) TSA_PRMT_FIELD(FlashPrmt, member)

struct TtSpec : EntrySpec {
    bool writeBack = true;  // 每帧把当前值写回机型参数
};

// g_tsaTTPrmt 的各项（TSATTPrmt_Init 填表、TSATTPrmt_UpdataFlashPrmt* 写回）。说明文字是原厂给调参工具的原文。
// 质心边缘修正的四项（18..21）按显示方向的上下左右排列，表里写的是 Dim1 为显示 X 方向时对应的字段。
constexpr std::array<TtSpec, kTtPrmtNum> kSpecs = {{
    /*   0 */ {{FLASH(gloveThold), Bounds::kAroundValue, 0, 0, "touch threshold for glove mode;\ntuning based on LTI of smart-cover/thick-glove in device floating situation;\nNOTE: if noise bigger than glove threshold, then glove performance will be sacrificed to ensure no ghost touch;"}},
    /*   1 */ {{FLASH(fingerThold), Bounds::kAroundValue, 0, 0, "touch threshold for finger mode;\ntuning based on LTI of 4mm finger without film and 5mm finger with film in device floating situation;"}},
    /*   2 */ {{FLASH(gloveExitThold), Bounds::kDerived, 0, 0, "touch threshold to exit glove mode or confirm finger mode;\nthe higher threshold is the better glove performance but might lead to too sensitive issue for finger;"}},
    /*   3 */ {{FLASH(gloveFilterSides), Bounds::kFixed, 15, 0, "define sides to apply glove filter which will filter glove touch with >4 grip ratio;\nNOTE: TOP_SIDE=1,BOTTOM_SIDE=2,LEFT_SIDE=4,RIGHT_SIDE=8;\ntuning based on performance of status-bar drop down and navigation-bar drop up with finger and glove;"}},
    /*   4 */ {{FLASH(distToEnterGloveMode), Bounds::kFixed, 30, 0, "distance threshold in mm to activate glove mode;\nthe less threshold is the easier to enter glove mode and the higher possibility for ghost touch;"}},
    /*   5 */ {{FLASH(clickDuration), Bounds::kObsolete, 0, 0, "obsolete;"}},
    /*   6 */ {{FLASH(clickSuppressDistance), Bounds::kFixed, 100, 0, "non-axis-change related moving distance(pixel) threshold to determine click or flick action, which will be applied to determine whether to apply click suppression or not;\nthe more threshold is the better click performance and poorer flick performance will be;"}},
    /*   7 */ {{FLASH(maxClickSuppressDistance), Bounds::kFixed, 150, 0, "moving distance threshold(pixel) to determine whether to apply click suppression or not;\nthe more threshold is the better click performance and poorer flick performance will be;"}},
    /*   8 */ {{FLASH(flickDistance), Bounds::kFixed, 120, 0, "non-axis-change related moving distance(pixel) threshold to determine default action of next touch, which will be applied to determine whether to apply click suppression or not by default for next touch;\nthe more threshold is the better click performance and poorer flick performance will be;"}},
    /*   9 */ {{FLASH(field909), Bounds::kObsolete, 0, 0, "obsolete;"}},
    /*  10 */ {{FLASH(sensorArchitecture), Bounds::kFixed, 3, 1, "Touch panel sensor architecture, could be OnCell(1), Hybrid Incell(2), Full Incell(3);"}},
    /*  11 */ {{FLASH(antiTouchScenarios), Bounds::kFixed, INT32_MAX, 0, "specific in which scenarios that anti-touch feature will be enabled;"}},
    /*  12 */ {{FLASH(proximityScenarios), Bounds::kFixed, INT32_MAX, 0, "specific in which scenarios that proximity feature will be enabled;"}},
    /*  13 */ {{FLASH(sigMultNormal), Bounds::kFixed, 96, 48, "128 scale up ratio to peak signal to determine touch zone traversal threshold for device grounding situation;\nthe more threshold is the better multi-finger performance and poorer fat-finger performance will be;"}},
    /*  14 */ {{FLASH(sigMultSD), Bounds::kFixed, 96, 16, "128 scale up ratio to peak signal to determine touch zone traversal threshold in case device floating detected;\nthe more threshold is the better multi-finger performance and poorer fat-finger performance will be;"}},
    /*  15 */ {{FLASH(accThold), Bounds::kFixed, 24, 8, "acceleration threshold to determine whether to do line connection between 2 touches;\nthe more threshold is the better line drawing performance and poorer 2 closed finger tapping performance will be; "}},
    /*  16 */ {{FLASH(chargerNoiseTholdPercent), Bounds::kFixed, 50, 10, "percentage to HTI threshold to determine charger noise threshold for hybrid incell;"}},
    /*  17 */ {{FLASH(field942), Bounds::kObsolete, 0, 0, "obsolete;"}},
    /*  18 */ {{FLASH(ctdEdge[0].segment[0].lnHighIdx), Bounds::kFixed, 255, 16, "threshold for centroid top edge correction\n;the less threshold is, the easier to reach edge"}},
    /*  19 */ {{FLASH(ctdEdge[1].segment[0].lnHighIdx), Bounds::kFixed, 255, 16, "threshold for centroid bottom edge correction\n;the less threshold is, the easier to reach edge"}},
    /*  20 */ {{FLASH(ctdEdge[2].segment[0].lnHighIdx), Bounds::kFixed, 255, 16, "threshold for centroid left edge correction\n;the less threshold is, the easier to reach edge"}},
    /*  21 */ {{FLASH(ctdEdge[3].segment[0].lnHighIdx), Bounds::kFixed, 255, 16, "threshold for centroid right edge correction\n;the less threshold is, the easier to reach edge"}},
    /*  22 */ {{FLASH(largeObjectSizeMm), Bounds::kFixed, 40, 28, "threshold of size in mm to determine larger object;"}},
    /*  23 */ {{FLASH(cmfTholdAlt.amp), Bounds::kFixed, 32767, 0, "common mode noise threshold for sensors\' signal;"}},
    /*  24 */ {{FLASH(cmfTholdAlt.grad), Bounds::kFixed, 1000, 0, "common mode noise threshold for delta signal of neighbouring sensor;"}},
    /*  25 */ {{FLASH(aftEdgeWidthThold), Bounds::kDerived, 0, 0, "edge width threshold of ewx/ewy for AFT"}},
    /*  26 */ {{FLASH(proxConfig), Bounds::kFixed, INT32_MAX, 0, "proximity feature configuration;"}},
    /*  27 */ {{FLASH(proxAreaY), Bounds::kDerived, 0, 5, "define proximity detection area along Y axis;"}},
    /*  28 */ {{FLASH(proxTriggerThold), Bounds::kFixed, 5000, 200, "signal threshold to trigger proximity event;\nthe bigger threshold is the better to avoid false trigger but the poorer to trigger proximity;"}},
    /*  29 */ {{FLASH(proxLiftOffTholdMin), Bounds::kFixed, 5000, 200, "min threshold for proximity to lift off;\nthe bigger threshold is the better to exit proximity but the poorer to avoid false proximity exit;"}},
    /*  30 */ {{FLASH(proxLiftOffTholdMax), Bounds::kFixed, 10000, 500, "max threshold for proximity to lift off;\nthe bigger threshold is the better to exit proximity but the poorer to avoid false proximity exit;"}},
    /*  31 */ {{FLASH(proxBaselineSpeed), Bounds::kFixed, 100, 5, "baseline update speed for proximity;\nthe bigger threshold is the slower to update proximity baseline;"}},
    /*  32 */ {{FLASH(proxBaselineThold), Bounds::kFixed, 5000, 50, "baseline update threshold for proximity;"}},
    /*  33 */ {{FLASH(proxLiftOffRatio), Bounds::kFixed, 16, 8, "ratio to proximity trigger threshold for min proximity lift off threshold;\nthe bigger threshold is the better to avoid trigger but the poorer for proximity debounce;"}},
    /*  34 */ {{FLASH(proxTzRatio), Bounds::kFixed, 16, 8, "ratio to proximity trigger threshold for touch zone traversal threshold;\nthe bigger threshold is the better to avoid false trigger but the poorer to trigger proximity;"}},
    /*  35 */ {{FLASH(callScanRateProxOff), Bounds::kFixed, 120, 60, "scan rate in phone call mode with proximity off;\neither 120 or 60 is selectable to trade off power consumption and touch performance;"}},
    /*  36 */ {{FLASH(callScanRateProxOn), Bounds::kFixed, 120, 15, "scan rate in phone call mode with proximity on;\navailable option [120, 60, 40, 30, 15] to trade off power consumption and proximity performance;"}},
    /*  37 */ {{FLASH(lockScanRateProxOff), Bounds::kFixed, 120, 60, "scan rate in screen lock mode with proximity off;\neither 120 or 60 is selectable to trade off power consumption and touch performance;"}},
    /*  38 */ {{FLASH(lockScanRateProxOn), Bounds::kFixed, 120, 15, "scan rate in screen lock mode with proximity on;\navailable option [120, 60, 40, 30, 15] to trade off power consumption and proximity performance;"}},
    /*  39 */ {{FLASH(proxTimeAutoScreenOff), Bounds::kFixed, INT32_MAX, 0, "in phone call mode, if screen is turned off automatically by either pressing power key,\nor system over proxTimeAutoScreenOff ms since last touch event and without proximity detected,\nthe screen off event will be identify as a proximity event,\nso that algorithm can detect proximity off once phone is away from face;"}, false},
    /*  40 */ {{FLASH(volumeGestureSet), Bounds::kFixed, INT32_MAX, 0, "volume gesture set"}},
    /*  41 */ {{FLASH(volumeFlickUp), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeFlickUp"}},
    /*  42 */ {{FLASH(volumeFlickDown), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeFlickDown"}},
    /*  43 */ {{FLASH(volumeStepUp), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeStepUp"}},
    /*  44 */ {{FLASH(volumeStepDown), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeStepDown"}},
    /*  45 */ {{FLASH(volumeActiveAreaStart), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeActiveAreaStart"}},
    /*  46 */ {{FLASH(volumeActiveAreaEnd), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeActiveAreaEnd"}},
    /*  47 */ {{FLASH(volumeGreenAreaStart), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeGreenAreaStart"}},
    /*  48 */ {{FLASH(volumeGreenAreaEnd), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeGreenAreaEnd"}},
    /*  49 */ {{FLASH(volumeSideEnabled), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeSideEnabled"}},
    /*  50 */ {{FLASH(volumeSensitivityLevel), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeSensitivityLevel"}},
    /*  51 */ {{FLASH(volumeActiveAreaStart2), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeActiveAreaStart2"}},
    /*  52 */ {{FLASH(volumeActiveAreaEnd2), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeActiveAreaEnd2"}},
    /*  53 */ {{FLASH(volumeGreenAreaStart2), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeGreenAreaStart2"}},
    /*  54 */ {{FLASH(volumeGreenAreaEnd2), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeGreenAreaEnd2"}},
    /*  55 */ {{FLASH(volumeSideEnabled2), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeSideEnabled2"}},
    /*  56 */ {{FLASH(volumeActiveAreaStart3), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeActiveAreaStart3"}},
    /*  57 */ {{FLASH(volumeActiveAreaEnd3), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeActiveAreaEnd3"}},
    /*  58 */ {{FLASH(volumeGreenAreaStart3), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeGreenAreaStart3"}},
    /*  59 */ {{FLASH(volumeGreenAreaEnd3), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeGreenAreaEnd3"}},
    /*  60 */ {{FLASH(volumeGreenAreaToMid3), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeGreenAreaToMid3"}},
    /*  61 */ {{FLASH(volumeSideEnabled3), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeSideEnabled3"}},
    /*  62 */ {{FLASH(volumeTouchMin), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeTouchMin"}},
    /*  63 */ {{FLASH(volumeTouchMax), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeTouchMax"}},
    /*  64 */ {{FLASH(volumeTimeFlick), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeTimeFlick"}},
    /*  65 */ {{FLASH(volumeTimeOnMin), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeTimeOnMin"}},
    /*  66 */ {{FLASH(volumeTimeOffMin), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeTimeOffMin"}},
    /*  67 */ {{FLASH(volumeTimeOffMax), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeTimeOffMax"}},
    /*  68 */ {{FLASH(volumeTimeGui), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeTimeGUI"}},
    /*  69 */ {{FLASH(volumeTimeToAntiTrigger), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeTimeToAntiTrigger"}},
    /*  70 */ {{FLASH(volumeForceAreaStart), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeForceAreaStart"}},
    /*  71 */ {{FLASH(volumeForceAreaEnd), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeForceAreaEnd"}},
    /*  72 */ {{FLASH(volumeForceAreaGap), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeForceAreaGap"}},
    /*  73 */ {{FLASH(volumeForceAreaGapCenter), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeForceAreaGapCenter"}},
    /*  74 */ {{FLASH(volumeForceAreaDebounce), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeForceAreaDebounce"}},
    /*  75 */ {{FLASH(volumeForceMoveMax), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeForceMoveMax"}},
    /*  76 */ {{FLASH(volumeForceTouchMax), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeForceTouchMax"}},
    /*  77 */ {{FLASH(volumeForceShbReportYPosDebounce), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeForceSHBReportYPosDebounce"}},
    /*  78 */ {{FLASH(volumeMappingTimeDelay), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeMappingTimeDelay"}},
    /*  79 */ {{FLASH(volumeMuteMinMove), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeMuteMinMove"}},
    /*  80 */ {{FLASH(volumeMuteMaxTime), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeMuteMaxTime"}},
    /*  81 */ {{FLASH(volumeGuiOnTimeOut), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeGUIOnTimeOut"}},
    /*  82 */ {{FLASH(volumeGuiOffTimeOut), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeGUIOffTimeOut"}},
    /*  83 */ {{FLASH(volumeKeyDebounceTime), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeKeyDebounceTime"}},
    /*  84 */ {{FLASH(powerAreaStart), Bounds::kFixed, INT32_MAX, INT32_MIN, "powerAreaStart"}},
    /*  85 */ {{FLASH(powerAreaEnd), Bounds::kFixed, INT32_MAX, INT32_MIN, "powerAreaEnd"}},
    /*  86 */ {{FLASH(powerTouchMax), Bounds::kFixed, INT32_MAX, INT32_MIN, "powerTouchMax"}},
    /*  87 */ {{FLASH(powerMoveMax), Bounds::kFixed, INT32_MAX, INT32_MIN, "powerMoveMax"}},
    /*  88 */ {{FLASH(powerTimeOnMin), Bounds::kFixed, INT32_MAX, INT32_MIN, "powerTimeOnMin"}},
    /*  89 */ {{FLASH(powerTimeOnMax), Bounds::kFixed, INT32_MAX, INT32_MIN, "powerTimeOnMax"}},
    /*  90 */ {{FLASH(powerTimeOffMin), Bounds::kFixed, INT32_MAX, INT32_MIN, "powerTimeOffMin"}},
    /*  91 */ {{FLASH(powerTimeOffMax), Bounds::kFixed, INT32_MAX, INT32_MIN, "powerTimeOffMax"}},
    /*  92 */ {{FLASH(powerGestureSet), Bounds::kFixed, INT32_MAX, 0, "power gesture set"}},
    /*  93 */ {{FLASH(motorVolumeOnWaveform), Bounds::kFixed, INT32_MAX, 0, "motorVolumeOnWaveform"}},
    /*  94 */ {{FLASH(motorVolumeOffWaveform), Bounds::kFixed, INT32_MAX, 0, "motorVolumeOffWaveform"}},
    /*  95 */ {{FLASH(sideTouchXCoreStart), Bounds::kFixed, INT32_MAX, INT32_MIN, "sideTouchXCoreStart"}},
    /*  96 */ {{FLASH(sideTouchXCoreEnd), Bounds::kFixed, INT32_MAX, INT32_MIN, "sideTouchXCoreEnd"}},
    /*  97 */ {{FLASH(sideTouchMinGripRatio), Bounds::kFixed, INT32_MAX, INT32_MIN, "sideTouchMinGripRatio"}},
    /*  98 */ {{FLASH(sideTouchMidGripRatio), Bounds::kFixed, INT32_MAX, INT32_MIN, "sideTouchMidGripRatio"}},
    /*  99 */ {{FLASH(sideTouchMaxGripRatio), Bounds::kFixed, INT32_MAX, INT32_MIN, "sideTouchMaxGripRatio"}},
    /* 100 */ {{FLASH(sideTouchConfig), Bounds::kFixed, INT32_MAX, 0, "side touch config"}},
    /* 101 */ {{FLASH(sensorEnabled), Bounds::kFixed, INT32_MAX, INT32_MIN, "sensorEnabled"}},
    /* 102 */ {{FLASH(sensorRate), Bounds::kFixed, INT32_MAX, INT32_MIN, "sensorRate"}},
    /* 103 */ {{FLASH(dim1ClipLow), Bounds::kFixed, 127, -128, "dim1ClipLow parameter"}},
    /* 104 */ {{FLASH(dim1ClipHigh), Bounds::kFixed, 127, -128, "dim1ClipHigh parameter"}},
    /* 105 */ {{FLASH(dim2ClipLow), Bounds::kFixed, 127, -128, "dim2ClipLow parameter"}},
    /* 106 */ {{FLASH(dim2ClipHigh), Bounds::kFixed, 127, -128, "dim2ClipHigh parameter"}},
}};

#undef FLASH

constexpr size_t kGloveExitThold = 2;
constexpr size_t kCtdFirst = 18;
constexpr size_t kAftEdgeWidthThold = 25;
constexpr size_t kProxAreaY = 27;

// 第 k 项对应的机型参数字段。Dim1 不是显示 X 方向时，质心边缘修正的上下两项与左右两项对调。
FieldRef FieldOf(size_t k) {
    if (k >= kCtdFirst && k < kCtdFirst + 4 && !IsDim1DisplayX()) {
        return kSpecs[kCtdFirst + (k - kCtdFirst + 2) % 4].field;
    }
    return kSpecs[k].field;
}

}  // namespace

// TSATTPrmt_Init @ 0x6bb3df90
void Init() {
    const FlashPrmt& flash = PrmtFlash();
    TtPrmtTable& table = PrmtTuning();
    for (size_t k = 0; k < kTtPrmtNum; ++k) Tuning::Fill(table[k], kSpecs[k], Tuning::Read(flash, FieldOf(k)));
    // 上下限取决于其它机型参数的几项
    const int finger = flash.fingerThold;
    table[kGloveExitThold].maxValue = finger * 2;
    table[kGloveExitThold].minValue = finger * 3 / 2;
    table[kAftEdgeWidthThold].maxValue = finger * 3 / 2;
    table[kProxAreaY].maxValue = flash.dim1Len - 2;
}

// TSATTPrmt_Process @ 0x6bb41b54
// 各项的当前值写回机型参数（原代码按模块分成 TSATTPrmt_UpdataFlashPrmtSig / Tm / Ta / ... 十三个函数，写的字段互不重叠）。
void Process() {
    FlashPrmt& flash = PrmtFlash();
    const TtPrmtTable& table = PrmtTuning();
    for (size_t k = 0; k < kTtPrmtNum; ++k) {
        if (kSpecs[k].writeBack) Tuning::Write(flash, FieldOf(k), table[k].value);
    }
}

}  // namespace Solvers::Tsa::Core::TtPrmt
