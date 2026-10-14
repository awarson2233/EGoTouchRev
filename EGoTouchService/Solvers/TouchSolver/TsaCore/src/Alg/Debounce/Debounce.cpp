#include "Alg/Debounce/Debounce.h"

#include <algorithm>

#include "Alg/Peak/PeakData.h"
#include "Alg/Touch/Touch.h"
#include "Alg/Touch/TouchData.h"
#include "Alg/TouchThold/TouchThold.h"
#include "App/TsaStatic.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::Debounce {

namespace {

constexpr int16_t kNoiseHigh = 500;              // 共模噪声超出 (kNoiseLow, kNoiseHigh] 时按 noisyTouchDownDebounce 防抖
constexpr int16_t kNoiseLow = -400;

TouchUnit& Cur(uint8_t idx) { return Touches().unit[idx]; }
// 原代码不检查历史下标：为 kNoTouch 时照样按下标读写，落在历史表之后的内存上。
PrevTouchUnit& Prev(uint8_t idx) { return PrevTouches().unit[idx]; }
const PeakEntry& PeakOf(const TouchUnit& unit) { return Peaks().entry[unit.peakIdx]; }

// DB_TouchDownTholdCheck @ 0x6ba55f8d
// 峰值是否达到按下门限（动态峰门限）。IDLE 扫描且非游戏场景下，上一帧已抬高过门限、或上一帧没过门限且本帧仍低于门限时
// 门限乘 1.5 并记 persistFlags2 bit1；脚趾特殊门限的机型在边缘列 / 末行改用边缘门限。没过门限记 volatileFlags2 bit5。
bool TouchDownTholdCheck(uint8_t prevIdx, uint8_t curIdx) {
    const DynamicPrmt& dyn = PrmtDynamic();
    const FlashPrmt& flash = PrmtFlash();
    TouchUnit& cur = Cur(curIdx);
    const PrevTouchUnit& prev = Prev(prevIdx);
    int16_t thold = dyn.peakThold;
    if ((Static().processingState & kProcIdleScan) != 0) {
        if (static_cast<char>(TSA_IsGameScenario()) == 0 &&
            ((prev.persistFlags2 & 2) != 0 || (PeakOf(cur).sig < thold && (prev.volatileFlags2 & 0x20) != 0))) {
            thold = static_cast<int16_t>(thold * 3 / 2);
            cur.persistFlags2 |= 2;
        }
    }
    if (TouchThold::ToeUseSpecialThold()) {
        const PeakEntry& peak = PeakOf(cur);
        if (peak.dim1 == 1 || peak.dim1 == flash.dim1Len - 2 || peak.dim2 == flash.dim2Len - 1) thold = dyn.edgePeakTholdLow;
    }
    const int16_t sig = PeakOf(cur).sig;
    if (sig < thold) cur.volatileFlags2 |= 0x20;
    return sig >= thold;
}

// DB_FastClickProcess @ 0x6ba562d4
// 按下防抖未满 4 帧、离起点不足 31 mm² 且形状小时，触点或历史里带 volatileFlags bit1 / bit0 且长轴小于 16 mm 的标为快速点击；
// 防抖超过 5 帧后取消。
void FastClickProcess(uint8_t prevIdx, uint8_t curIdx) {
    TouchUnit& cur = Cur(curIdx);
    const PrevTouchUnit& prev = Prev(prevIdx);
    if (prev.debounceFrames < 4 && cur.sqDistMmFromStart < 0x1f) {
        if (prev.field066 < 0xb && cur.field066 < 0xb &&
            ((cur.volatileFlags & 0x800) == 0 || (prev.volatileFlags & 0x800) == 0)) {
            if ((cur.volatileFlags & 2) != 0 && cur.majorMm < 0x10) cur.persistFlags |= kPersistFastClick;
            if ((cur.volatileFlags & 1) != 0 && prev.majorMm < 0x10) cur.persistFlags |= kPersistFastClick;
        }
    } else if (prev.debounceFrames > 5) {
        cur.persistFlags &= ~kPersistFastClick;
    }
}

// DB_GetNumBySignalSum @ 0x6ba565b2
// 峰 Zn 不到直接上报门限（znFingerThold × sigRatioToDirectReport / 16）时至少防抖 1 帧；
// 能与历史比较且 sigSum 增长超过上一帧的 1/4 时至少 3 帧。
uint16_t GetNumBySignalSum(uint8_t prevIdx, uint8_t curIdx, uint16_t num) {
    const DynamicPrmt& dyn = PrmtDynamic();
    const TouchUnit& cur = Cur(curIdx);
    const PrevTouchUnit& prev = Prev(prevIdx);
    const auto directReportZn =
        static_cast<int32_t>(static_cast<uint32_t>(dyn.sigRatioToDirectReport) * static_cast<uint32_t>(dyn.znFingerThold)) / 16;
    if (PeakOf(cur).zn >= directReportZn) return num;
    if (prev.age == 0 || !Touch::ValidComparison(curIdx, prevIdx)) return num == 0 ? 1 : num;
    if (prev.sigSum / 4 < cur.sigSum - prev.sigSum && num < 3) return 3;
    return num;
}

// DB_GetNumBySideTouch @ 0x6ba5674c
// 长轴大（IDLE 扫描时超过 29 mm，否则超过 14 mm）时至少 2 帧；非 IDLE 扫描下没有 volatileFlags bit2 时至少 1 帧。
uint16_t GetNumBySideTouch(uint8_t curIdx, uint16_t num) {
    const TouchUnit& cur = Cur(curIdx);
    uint16_t result = num;
    if ((Static().processingState & kProcIdleScan) == 0) {
        if (cur.majorMm > 0xe && num < 2) result = 2;
        if ((cur.volatileFlags & 4) == 0 && result == 0) result = 1;
    } else if (cur.majorMm > 0x1d && num < 2) {
        result = 2;
    }
    return result;
}

}  // namespace

// DB_TouchDownNum @ 0x6ba5681c
uint16_t TouchDownNum(uint8_t prevIdx, uint8_t curIdx) {
    const TsaStatic& stat = Static();
    const FlashPrmt& flash = PrmtFlash();
    TouchUnit& cur = Cur(curIdx);
    PrevTouchUnit& prev = Prev(prevIdx);

    uint16_t num = 0;
    if (Peaks().count - PrevPeaks().count > 1) num = 1;  // 本帧比上一帧多出不止一个峰
    if ((cur.exceptionType & 0x80) != 0) num = std::max<uint16_t>(num, 2);
    if ((cur.exceptionType & 0x8) != 0 && num == 0) num = 1;
    if ((cur.exceptionType & 0x200) != 0) num = cur.field098 < 1 ? kNoTouchDown : std::max<uint16_t>(num, 3);
    if (stat.noiseLevel != 0) num = std::max<uint16_t>(num, 2);
    if (cur.field164 != 0) num = std::max<uint16_t>(num, 3);
    // 否决条件。TouchDownTholdCheck 有副作用，只在 exceptionType bit6 未置位时调用。
    if ((cur.exceptionType & 0x40) != 0 || !TouchDownTholdCheck(prevIdx, curIdx) || (cur.volatileFlags & 0x400) != 0 ||
        static_cast<int32_t>(cur.volatileFlags) < 0 || (cur.volatileFlags & 0x40000000) != 0 ||
        (cur.volatileFlags & 0x1000000) != 0 || (cur.volatileFlags & 0x2000000) != 0) {
        num = kNoTouchDown;
    }
    num = GetNumBySignalSum(prevIdx, curIdx, num);
    num = GetNumBySideTouch(curIdx, num);
    if (cur.zoneShape == 2) num = std::max<uint16_t>(num, 3);
    if (cur.zoneShape == 3) num = std::max<uint16_t>(num, 3);
    if ((cur.volatileFlags & 0x800) != 0) num = std::max<uint16_t>(num, 2);
    if ((cur.persistFlags & 0x2000) != 0 || (cur.volatileFlags & 0x400000) != 0) num = kNoTouchDown;

    if (num == kNoTouchDown) {
        cur.volatileFlags |= 0x10;
    } else if ((prev.volatileFlags & 0x10) != 0) {
        prev.debounceFrames = 0;  // 上一帧被否决过，防抖从头计
    }
    FastClickProcess(prevIdx, curIdx);
    num = std::max(num, prev.field0F8);
    if (stat.frameIntervalMs > 0xe) num = static_cast<uint16_t>((num + 1) / 2);
    if (flash.noisyTouchDownDebounce != 0) {
        const auto dim1 = static_cast<uint8_t>(cur.rptPos.dim1 >> 8);
        const auto dim2 = static_cast<uint8_t>(cur.rptPos.dim2 >> 8);
        if (CMF_GetNoiseValue(dim1, dim2) > kNoiseHigh || CMF_GetNoiseValue(dim1, dim2) < kNoiseLow) {
            num = std::max<uint16_t>(num, flash.noisyTouchDownDebounce);
        }
    }
    return num;
}

// DB_LiftOffNum @ 0x6ba55e79
// 报过点且没有 persistFlags bit7 的触点才做抬起防抖：多指拆分区域 3 帧，volatileFlags bit28 或 噪声等级大于 2 时至少 2 帧。
uint16_t LiftOffNum(uint8_t prevIdx) {
    const PrevTouchUnit& prev = Prev(prevIdx);
    if ((prev.persistFlags & 0x8) == 0 || (prev.persistFlags & 0x80) != 0) return 0;
    uint16_t num = 0;
    if (prev.zoneShape == 3) num = 3;
    if ((prev.volatileFlags & 0x10000000) != 0 && num < 2) num = 2;
    if (Static().noiseLevel > 2 && num < 2) num = 2;
    return num;
}

// DB_LOSize @ 0x6ba55e20
uint16_t LargeObjectSize() {
    const uint8_t sizeMm = PrmtFlash().largeObjectSizeMm;
    auto area = static_cast<uint16_t>(sizeMm * sizeMm);
    if (PrevTouches().field003 != 0) area = static_cast<uint16_t>(static_cast<uint16_t>(area * 3) >> 2);
    return area;
}

}  // namespace Solvers::Tsa::Core::Debounce

namespace DB = Solvers::Tsa::Core::Debounce;

extern "C" {
uint16_t DB_TouchDownNum(uint8_t prevIdx, uint8_t curIdx) { return DB::TouchDownNum(prevIdx, curIdx); }
uint16_t DB_LiftOffNum(uint8_t prevIdx) { return DB::LiftOffNum(prevIdx); }
uint16_t DB_LOSize(void) { return DB::LargeObjectSize(); }
}  // extern "C"
