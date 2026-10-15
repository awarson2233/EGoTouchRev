#include "Alg/Touch/Touch.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "Alg/Baseline/Baseline.h"
#include "Alg/Peak/Peak.h"
#include "Alg/Peak/PrevPeak.h"
#include "Alg/Sensor/Sensor.h"
#include "Alg/Touch/PrevTouch.h"
#include "Alg/TouchSize/TouchSize.h"
#include "App/Afe.h"
#include "App/Report.h"
#include "App/Sys.h"
#include "App/TsaBuffer.h"
#include "App/TsaOut.h"
#include "App/TsaStatic.h"
#include "Common/Assert.h"
#include "Extension/HardwareAnalyzer/HardwareAnalyzer.h"
#include "Extension/Thenar/Thenar.h"
#include "Extension/Thenar/ThenarData.h"
#include "Prmt/Prmt.h"
#include "Rejection/AntiTouch/AntiTouch.h"
#include "Rejection/GripFilter/GripFilter.h"

namespace Solvers::Tsa::Core::Touch {

namespace {

TouchUnit& Cur(uint8_t idx) { return Touches().unit[idx]; }
// 原代码不检查历史下标：为 kNoTouch 时照样按下标读写，落在历史表之后的内存上。
PrevTouchUnit& Prev(uint8_t idx) { return PrevTouches().unit[idx]; }
const PeakEntry& PeakOf(const TouchUnit& unit) { return Peaks().entry[unit.peakIdx]; }

/// raw 函数返回值被原代码截断成 char 再判断。
bool ByteTrue(uint64_t value) { return static_cast<uint8_t>(value) != 0; }
/// raw 函数返回值被原代码截断成 int 再判断。
bool IntTrue(uint64_t value) { return static_cast<uint32_t>(value) != 0; }

/// x·3/4，向零取整；乘法按 32 位回绕。
int ThreeQuarters(int x) { return static_cast<int>(static_cast<uint32_t>(x) * 3u) / 4; }

/// (num·32) / den，封顶 255。
uint8_t Ratio32(int num, int den) {
    const int ratio = (num << 5) / den;
    return static_cast<uint8_t>(ratio < 0x100 ? ratio : 0xff);
}

uint16_t CellIndex(const PeakEntry& peak) { return GridIndex(peak.dim1, peak.dim2); }
uint16_t CellIndex(Point pos) { return GridIndex(static_cast<uint8_t>(pos.dim1 >> 8), static_cast<uint8_t>(pos.dim2 >> 8)); }

// ── 距离 ──────────────────────────────────────────────────────────────

// Touch_GetSQDistMMFromPrevPos @ 0x6ba735c9
void GetSqDistMmFromPrevPos(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& cur = Cur(curIdx);
    const PrevTouchUnit& prev = Prev(prevIdx);
    cur.sqDistMmFromPrev = GetSqDistMm(cur.pos.dim1, cur.pos.dim2, prev.pos.dim1, prev.pos.dim2);
}

// Touch_GetSQDistPixelFromPrevPos @ 0x6ba7381e
void GetSqDistPixelFromPrevPos(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& cur = Cur(curIdx);
    const PrevTouchUnit& prev = Prev(prevIdx);
    cur.sqDistPxFromPrev = GetSqDistPixel(cur.pos.dim1, cur.pos.dim2, prev.pos.dim1, prev.pos.dim2);
}

// Touch_GetSQDistMMFromStartPos @ 0x6ba738ff
void GetSqDistMmFromStartPos(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& cur = Cur(curIdx);
    const PrevTouchUnit& prev = Prev(prevIdx);
    cur.sqDistMmFromStart = GetSqDistMm(cur.pos.dim1, cur.pos.dim2, prev.startPos.dim1, prev.startPos.dim2);
}

// Touch_GetSQAccMMFromPrevPos @ 0x6ba736aa
// 本帧位移减去上一帧位移，即加速度。
void GetSqAccMmFromPrevPos(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& cur = Cur(curIdx);
    const PrevTouchUnit& prev = Prev(prevIdx);
    const auto acc1 = static_cast<int16_t>(cur.pos.dim1 - prev.pos.dim1 - prev.posDelta.dim1);
    const auto acc2 = static_cast<int16_t>(cur.pos.dim2 - prev.pos.dim2 - prev.posDelta.dim2);
    const auto sq1 = static_cast<uint32_t>(acc1 * acc1);
    uint32_t sum = static_cast<uint32_t>(acc2 * acc2) + sq1;
    if (sum < sq1) sum = 0xffffffffu;
    cur.sqAccMmFromPrev = static_cast<uint16_t>(TouchSize::SqPixelToSqMm(sum));
}

// ── UnitUpdate 中的标志 ──────────────────────────────────────────────

// Touch_SetEdgeFlag @ 0x6ba72d1c
void SetEdgeFlag(uint8_t curIdx) {
    TouchUnit& cur = Cur(curIdx);
    const uint8_t peak = cur.peakIdx;
    if (Peak::IsOnInnerEdgeDim1Near(peak)) cur.edgeFlags |= 0x100;
    else if (Peak::IsOnInnerEdgeDim1Far(peak)) cur.edgeFlags |= 0x200;
    if (Peak::IsOnInnerEdgeDim2Near(peak)) cur.edgeFlags |= 0x400;
    else if (Peak::IsOnInnerEdgeDim2Far(peak)) cur.edgeFlags |= 0x800;
    if (Peak::IsOnOuterEdgeDim1Near(peak)) cur.edgeFlags |= 0x1000;
    else if (Peak::IsOnOuterEdgeDim1Far(peak)) cur.edgeFlags |= 0x2000;
    if (Peak::IsOnOuterEdgeDim2Near(peak)) cur.edgeFlags |= 0x4000;
    else if (Peak::IsOnOuterEdgeDim2Far(peak)) cur.edgeFlags |= 0x8000;
}

// Touch_SetRegion @ 0x6ba72bce
// 显示坐标所在象限：0 左上、1 左下、2 右上、3 右下（以显示 X / Y 的中线划分）。
void SetRegion(uint8_t curIdx) {
    const uint16_t x = Out::DisplayXPos(curIdx);
    const uint16_t y = Out::DisplayYPos(curIdx);
    const FlashPrmt& flash = PrmtFlash();
    const uint16_t xRes = IsDim1DisplayX() ? flash.dim1Res : flash.dim2Res;
    const uint16_t yRes = IsDim1DisplayX() ? flash.dim2Res : flash.dim1Res;
    const bool left = x < (xRes >> 1);
    const bool top = y < (yRes >> 1);
    Cur(curIdx).quadrant = left ? (top ? 0u : 1u) : (top ? 2u : 3u);
}

// Touch_SetVolatileFlagSigSum @ 0x6ba6f7af
// 区域信号和、峰 Zn、峰值与上一帧之比（×32，封顶 255；上一帧为 0 或峰已不在时记 255）。
void SetVolatileFlagSigSum(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& cur = Cur(curIdx);
    const PrevTouchUnit& prev = Prev(prevIdx);
    cur.sigSumRatio = prev.sigSum == 0 ? uint8_t{0xff} : Ratio32(cur.sigSum, prev.sigSum);

    const PrevPeakTable& prevPeaks = PrevPeaks();
    if (prev.peakIdx < prevPeaks.count) {
        const PrevPeakEntry& prevPeak = prevPeaks.entry[prev.peakIdx];
        const PeakEntry& peak = PeakOf(cur);
        cur.znRatio = prevPeak.zn == 0 ? uint8_t{0xff} : Ratio32(peak.zn, prevPeak.zn);
        cur.peakSigRatio = prevPeak.sig == 0 ? uint8_t{0xff} : Ratio32(peak.sig, prevPeak.sig);
    } else {
        cur.znRatio = 0xff;
        cur.peakSigRatio = 0xff;
    }
    if (cur.sigSumRatio < 0x18) cur.volatileFlags |= 2;
    if (cur.sigSumRatio > 0x28) cur.volatileFlags |= 1;
}

// Touch_SetVolatileFlagTouchAxisAbnormal @ 0x6ba6fc6a
void SetVolatileFlagTouchAxisAbnormal(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& cur = Cur(curIdx);
    if (cur.znRatio >= 0x18) return;
    cur.volatileFlags |= 0x40000;
    if (Prev(prevIdx).majorMm * 4u < cur.majorMm * 3u && cur.majorMm > 0x19 &&
        PeakOf(cur).sig * 2 < PrmtDynamic().peakThold * 3) {
        cur.volatileFlags2 |= 0x40;
    }
}

// Touch_IsPeakOnInnerDim1Edge @ 0x6ba70f56
// 注意：原代码把触点下标直接当峰下标用。
bool IsPeakOnInnerDim1Edge(uint8_t idx) {
    const uint8_t dim1 = Peaks().entry[idx].dim1;
    return dim1 == 1 || dim1 == PrmtFlash().dim1Len - 2;
}

// Touch_ToeUseSpecialZ4TholdEdge @ 0x6ba6fe1a
bool ToeUseSpecialZ4TholdEdge() {
    return IsProject("B206") || IsProject("B20Y");
}

// Touch_NeedNextEdgeLowThresh @ 0x6ba6fe70
// 峰在次边缘一圈（或机型指定的远端边缘）时，按下 / 抬起门限打 3/4 折。
bool NeedNextEdgeLowThresh(uint8_t curIdx) {
    const PeakEntry& peak = PeakOf(Cur(curIdx));
    const FlashPrmt& flash = PrmtFlash();
    const DynamicPrmt& dyn = PrmtDynamic();
    bool low = false;
    if ((IsProject("P116") || IsProject("P150")) &&
        (peak.dim1 == 1 || peak.dim1 == flash.dim1Len - 2 || peak.dim2 == flash.dim2Len - 1)) {
        low = true;
    } else if (ToeUseSpecialZ4TholdEdge()) {
        low = true;
    }
    if (dyn.lowTholdOnDim1FarEdge != 0 && peak.dim1 == flash.dim1Len - 1) low = true;
    if (dyn.lowTholdOnDim2FarEdge != 0 && peak.dim2 == flash.dim2Len - 1) low = true;
    return low;
}

// Touch_SetVolatileFlagFingerZFailed @ 0x6ba700e1
void SetVolatileFlagFingerZFailed(uint8_t curIdx) {
    int thold = PrmtDynamic().znFingerThold;
    if ((Static().featureFlags & kFeatSmartCover) != 0) thold /= 2;
    TouchUnit& cur = Cur(curIdx);
    if (PeakOf(cur).zn < thold) cur.volatileFlags |= 0x800;
}

// Touch_SetVolatileFlagTouchDownZFailed @ 0x6ba701c3
void SetVolatileFlagTouchDownZFailed(uint8_t curIdx, bool nextEdgeLow) {
    const DynamicPrmt& dyn = PrmtDynamic();
    TouchUnit& cur = Cur(curIdx);
    int thold = dyn.znTouchDownThold;
    if (nextEdgeLow) thold = ThreeQuarters(thold);
    if (PeakOf(cur).zn < thold) cur.volatileFlags |= 0x400;
    int thold2 = dyn.znTouchDownThold2;
    if (nextEdgeLow) thold2 = ThreeQuarters(thold2);
    if (PeakOf(cur).zn < thold2) cur.volatileFlags |= 0x40000000;
}

// Touch_SetVolatileFlagLiftOffZFailed @ 0x6ba70356
void SetVolatileFlagLiftOffZFailed(uint8_t curIdx, uint8_t prevIdx, bool nextEdgeLow) {
    const DynamicPrmt& dyn = PrmtDynamic();
    const TsaPrpt& prpt = Prpt();
    TouchUnit& cur = Cur(curIdx);
    const PrevTouchUnit& prev = Prev(prevIdx);
    const PeakEntry& peak = PeakOf(cur);
    const bool regionThold = ((2 >> (static_cast<uint8_t>(prpt.touchMode) & 0x1f)) & 1) != 0 && prpt.touchModeLevel == 2 &&
                             (dyn.zTholdFlags & 2) != 0;
    if (!regionThold) {
        // 按住超过 60 帧且移动过的触点不判
        if (prev.age >= 0x3d && (prev.persistFlags & 4) != 0) return;
        auto sigThold = static_cast<int16_t>(ThreeQuarters(dyn.peakThold));
        int znThold = ThreeQuarters(dyn.znTouchDownThold);
        if (prev.maxPeakSig / 2 < sigThold) sigThold = static_cast<int16_t>(prev.maxPeakSig / 2);
        if (prev.maxPeakZn / 2 < znThold) znThold = prev.maxPeakZn / 2;
        if (peak.zn < znThold || peak.sig < sigThold) cur.volatileFlags2 |= 1;
    } else {
        int znThold = ThreeQuarters(dyn.znTouchDownThold);
        auto sigThold = static_cast<int16_t>(ThreeQuarters(dyn.peakThold));
        if (nextEdgeLow) {
            znThold = ThreeQuarters(znThold);
            sigThold = static_cast<int16_t>(ThreeQuarters(dyn.edgePeakTholdLow));
        }
        if (peak.zn < znThold || peak.sig < sigThold) cur.volatileFlags2 |= 1;
    }
}

// Touch_SetVolatileFlagStableTouch @ 0x6ba70775
void SetVolatileFlagStableTouch(uint8_t curIdx) {
    const DynamicPrmt& dyn = PrmtDynamic();
    const TsaStatic& stat = Static();
    const bool glove = (dyn.zTholdFlags & 4) != 0 && (stat.featureFlags & kFeatGloveMode) != 0;
    const bool knuckle = (dyn.zTholdFlags & 8) != 0 && (stat.featureFlags & kFeatKnuckle) != 0;
    const bool fingerThold = (Prpt().touchModeLatched == 0 || (!glove && !knuckle)) &&
                             (stat.featureFlags & kFeatSmartCover) == 0 && stat.field3F1 == 0;
    const int thold = fingerThold ? dyn.znFingerThold : dyn.znTouchDownThold;
    TouchUnit& cur = Cur(curIdx);
    if (static_cast<int>(static_cast<uint32_t>(thold) * 3u) < PeakOf(cur).zn) cur.volatileFlags |= 0x1000;
}

// Touch_UpdateStableProperty @ 0x6ba708d8
// 稳定锚点：按下前 10 帧跟随当前位置；之后只有移动超过 5 mm 才更新。
void UpdateStableProperty(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& cur = Cur(curIdx);
    const PrevTouchUnit& prev = Prev(prevIdx);
    const uint8_t reportedCount = Touches().reportedCount;
    if (prev.age < 0xb) {
        cur.stablePos = cur.pos;
        cur.stableMajorMm = cur.majorMm;
        return;
    }
    const uint16_t sqDist = GetSqDistMm(cur.pos.dim1, cur.pos.dim2, prev.stablePos.dim1, prev.stablePos.dim2);
    if (sqDist < 0x19) {
        cur.stablePos = prev.stablePos;
        cur.stableMajorMm = prev.stableMajorMm;
        cur.stableReportedCount = reportedCount;
        return;
    }
    const int majorDiff = cur.majorMm - prev.stableMajorMm;
    const auto absMajorDiff = static_cast<uint16_t>(majorDiff < 0 ? -majorDiff : majorDiff);
    const auto sqMajorDiff = static_cast<uint16_t>(static_cast<uint32_t>(absMajorDiff) * absMajorDiff);
    if (static_cast<uint16_t>(sqDist >> 1) <= sqMajorDiff && cur.majorMm > 0x14 &&
        prev.exceptionType == 8 && reportedCount == 1 && prev.stableReportedCount == reportedCount) {
        cur.volatileFlags |= 0x200000;
    }
    cur.stablePos = cur.pos;
    cur.stableMajorMm = cur.majorMm;
    cur.stableReportedCount = reportedCount;
}

// Touch_GetPeakSumPosSig @ 0x6ba6f641
void GetPeakSumPosSig(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& cur = Cur(curIdx);
    const PrevTouchUnit& prev = Prev(prevIdx);
    if (prev.maxSigSum < cur.sigSum) {
        cur.maxSigSum = cur.sigSum;
        cur.maxSigSumAngle = cur.angle;
    } else {
        cur.maxSigSum = prev.maxSigSum;
        cur.maxSigSumAngle = prev.maxSigSumAngle;
    }
}

// Touch_UpdateHandEnum @ 0x6ba70df0
// 信号和最大那一帧的倾角在 (15°, 75°) 为 4，在 (-75°, -15°) 为 8，否则沿用上一帧。
void UpdateHandEnum(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& cur = Cur(curIdx);
    const auto angle = [&cur] { return static_cast<int8_t>(Out::Angle(static_cast<char>(cur.maxSigSumAngle))); };
    if (angle() > 15 && angle() < 75) {
        cur.handEnum = 4;
        return;
    }
    if (angle() < -15 && angle() > -75) {
        cur.handEnum = 8;
        return;
    }
    cur.handEnum = Prev(prevIdx).handEnum;
}

// Touch_SetVolatileFlag @ 0x6ba70fdd
void SetVolatileFlag(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& cur = Cur(curIdx);
    const PrevTouchUnit& prev = Prev(prevIdx);
    TsaStatic& stat = Static();

    SetVolatileFlagSigSum(curIdx, prevIdx);
    SetVolatileFlagTouchAxisAbnormal(curIdx, prevIdx);
    if (cur.peakSigRatio < 0x18) cur.volatileFlags |= 0x80000;
    if (cur.sqDistMmFromPrev > 100) cur.volatileFlags |= 0x80;
    if (cur.field068 > 7 && cur.field06A > 7) cur.persistFlags2 |= 8;
    if (cur.field070 != 0 && cur.field071 != 0) cur.volatileFlags |= 0x4000;

    // 峰的 Z8 不到中心值的 1/32：孤立尖峰。机型要求时内边缘上的峰不判。
    const bool checkZ8 = PrmtDynamic().skipZ8CheckOnInnerEdge == 0 || !IsPeakOnInnerDim1Edge(curIdx) || cur.field164 == 0;
    if (checkZ8) {
        const PeakEntry& peak = PeakOf(cur);
        if (peak.z8 < peak.sig / 32) {
            cur.volatileFlags |= 0x80000000;
            stat.specialControlFlags |= 0x40;
        }
    }

    const bool nextEdgeLow = NeedNextEdgeLowThresh(curIdx);
    SetVolatileFlagFingerZFailed(curIdx);
    SetVolatileFlagTouchDownZFailed(curIdx, nextEdgeLow);
    SetVolatileFlagLiftOffZFailed(curIdx, prevIdx, nextEdgeLow);
    SetVolatileFlagStableTouch(curIdx);
    UpdateStableProperty(curIdx, prevIdx);

    if (prev.age == 0 && (stat.systemFlags & 0x10) != 0) cur.volatileFlags |= 0x800000;
    if ((prev.persistFlags & 0x4000) != 0 || (cur.volatileFlags & 0x800000) != 0) {
        if (Baseline::CheckAbnormalTouch(curIdx)) cur.volatileFlags |= 0x400000;
    }
    if (prev.age == 0 && cur.exceptionType != 0) stat.specialControlFlags |= 0x80;
    GetPeakSumPosSig(curIdx, prevIdx);
    UpdateHandEnum(curIdx, prevIdx);
}

// Touch_SetVolatileFlagFalseFlickDetected @ 0x6ba71f3a
// 位移明显而形状变化更大：认为是形变造成的假滑动，记下位置并累计偏移。
void SetVolatileFlagFalseFlickDetected(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& cur = Cur(curIdx);
    PrevTouchUnit& prev = Prev(prevIdx);
    const PeakEntry& peak = PeakOf(cur);
    const FlashPrmt& flash = PrmtFlash();

    bool wideMargin = false;
    if ((Static().systemFlags & 0x20) != 0 && (IsProject("W169") || IsProject("W189"))) {
        wideMargin = peak.dim1 < 2 || flash.dim1Len - 2 <= peak.dim1 || peak.dim2 < 2 || flash.dim2Len - 2 <= peak.dim2;
    }
    const uint32_t minDist = static_cast<uint16_t>(TouchSize::MmToPixel(wideMargin ? 2 : 1));
    if ((minDist * minDist >> 1) >= cur.sqDistPxFromPrev) return;

    const auto majorDiff = static_cast<uint32_t>(std::abs(cur.majorMm - prev.majorMm));
    const auto minorDiff = static_cast<uint32_t>(std::abs(cur.minorMm - prev.minorMm));
    const auto shapeSqPx = static_cast<uint32_t>(TouchSize::SqMmToSqPixel(static_cast<uint16_t>(minorDiff * minorDiff + majorDiff * majorDiff)));
    if (cur.sqDistPxFromPrev < shapeSqPx >> 2) {
        prev.falseFlickPos = cur.pos;
        prev.falseFlickOffset.dim1 = static_cast<int16_t>(cur.pos.dim1 - prev.pos.dim1 + prev.falseFlickOffset.dim1);
        prev.falseFlickOffset.dim2 = static_cast<int16_t>(cur.pos.dim2 - prev.pos.dim2 + prev.falseFlickOffset.dim2);
        cur.volatileFlags |= 0x100000;
    }
}

/// 当前位置到 (dim1, dim2) 的距离超过参数给出的外部像素距离时，在 persistFlags 记 flag。
void SetPersistFlagMoveOver(uint8_t curIdx, uint16_t dim1, uint16_t dim2, uint8_t extDist, uint32_t flag) {
    TouchUnit& cur = Cur(curIdx);
    const uint32_t sqDist = GetSqDistPixel(cur.pos.dim1, cur.pos.dim2, dim1, dim2);
    const uint32_t dist = static_cast<uint16_t>(Out::ExtToIntDist(extDist));
    if (dist * dist < sqDist) cur.persistFlags |= flag;
}

// Touch_SetPersistFlag @ 0x6ba72776
void SetPersistFlag(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& cur = Cur(curIdx);
    const PrevTouchUnit& prev = Prev(prevIdx);
    const DynamicPrmt& dyn = PrmtDynamic();

    if ((cur.volatileFlags & 4) != 0) cur.persistFlags |= 1;
    if (static_cast<int>(static_cast<uint32_t>(dyn.distToEnterGloveMode) * dyn.distToEnterGloveMode) <= cur.sqDistMmFromStart) {
        cur.persistFlags |= 4;
    }
    if ((cur.volatileFlags & 0x1000) != 0) cur.persistFlags |= 0x200;
    if ((prev.persistFlags & 8) != 0 && prev.field0A8 != 6) {
        SetVolatileFlagFalseFlickDetected(curIdx, prevIdx);
        // Touch_SetPersistFlagMoveOverClickSuppressRange @ 0x6ba723e9
        SetPersistFlagMoveOver(curIdx, prev.falseFlickPos.dim1, prev.falseFlickPos.dim2, dyn.clickSuppressDistance, 0x100);
        // Touch_SetPersistFlagMoveOverMaxClickSuppressRange @ 0x6ba72518
        SetPersistFlagMoveOver(curIdx, prev.firstRptPos.dim1, prev.firstRptPos.dim2, dyn.maxClickSuppressDistance, 0x8000);
        // Touch_SetPersistFlagMoveOverFlickDistance @ 0x6ba72647（原代码拿累计偏移当坐标用）
        SetPersistFlagMoveOver(curIdx, static_cast<uint16_t>(prev.falseFlickOffset.dim1),
                               static_cast<uint16_t>(prev.falseFlickOffset.dim2), dyn.flickDistance, 0x1000);
    }
    if ((cur.volatileFlags & 0x800000) != 0) cur.persistFlags |= 0x4000;
    if ((prev.volatileFlags & 0x400000) != 0 && prev.debounceFrames == 10) cur.persistFlags |= 0x2000;
    if (cur.sqDistMmFromPrev > 0x10 || cur.exceptionType != 0 ||
        PrmtFlash().fingerThold * 2 < cur.maxPeakSig || Touches().count > 1) {
        cur.persistFlags |= 0x80000;
    }
    if (prev.age == 0 && Afe::IsRawJump()) cur.gripFlags |= 0x10000000;

    cur.persistFlags |= prev.persistFlags;
    cur.persistFlags2 |= prev.persistFlags2;
    cur.persistFlags3 |= prev.persistFlags3;
    cur.gripFlags |= prev.gripFlags;
}

// Touch_UnitUpdateMaxMin @ 0x6ba75d1a
void UnitUpdateMaxMin(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& cur = Cur(curIdx);
    const PrevTouchUnit& prev = Prev(prevIdx);
    const PeakEntry& peak = PeakOf(cur);
    const auto refThold = PrmtFlash().peakFilterReferenceThreshold;
    const auto refSig = static_cast<int16_t>(std::min<int>(Peaks().maxSig, refThold));

    cur.maxField072 = std::max(cur.field072, prev.maxField072);
    cur.maxField070 = std::max(cur.field070, prev.maxField070);
    cur.maxField071 = std::max(cur.field071, prev.maxField071);
    cur.maxField058 = std::max(cur.field058, prev.maxField058);
    cur.maxField078 = std::max(cur.field078, prev.maxField078);
    cur.maxEdgeDistDim1 = std::max(cur.edgeDistDim1, prev.maxEdgeDistDim1);
    cur.maxEdgeDistDim2 = std::max(cur.edgeDistDim2, prev.maxEdgeDistDim2);
    cur.maxSqDistPxFromPrev = std::max(cur.sqDistPxFromPrev, prev.maxSqDistPxFromPrev);
    cur.maxPeakSig = std::max(peak.sig, prev.maxPeakSig);
    cur.maxRefPeakSig = std::max(refSig, prev.maxRefPeakSig);
    // 原代码按无符号比较
    cur.maxPeakZn = static_cast<int32_t>(std::max(static_cast<uint32_t>(peak.zn), static_cast<uint32_t>(prev.maxPeakZn)));
    if (prev.signalScale < cur.zoneSignalScale) {
        cur.signalScale = std::min(cur.zoneSignalScale, refThold);
    } else {
        cur.signalScale = prev.signalScale;
    }
    if (prev.age == 0) {
        cur.minField058 = cur.field058;
        cur.minEdgeDistDim1 = cur.edgeDistDim1;
        cur.minEdgeDistDim2 = cur.edgeDistDim2;
    } else {
        cur.minField058 = std::min(cur.field058, prev.minField058);
        cur.minEdgeDistDim1 = std::min(cur.edgeDistDim1, prev.minEdgeDistDim1);
        cur.minEdgeDistDim2 = std::min(cur.edgeDistDim2, prev.minEdgeDistDim2);
    }
}

// Touch_IsFarAwayFromCornerCompareToStartPoint @ 0x6ba74aef
bool IsFarAwayFromCornerCompareToStartPoint(uint8_t curIdx) {
    const TouchUnit& cur = Cur(curIdx);
    return cur.maxSigSum / 2 <= cur.sigSum;
}

// Touch_UnitUpdateSqDistMMFromStartPosMax @ 0x6ba75c2e
void UnitUpdateSqDistMmFromStartPosMax(uint8_t curIdx, uint8_t prevIdx) {
    const bool update = IsFarAwayFromCornerCompareToStartPoint(curIdx);
    TouchUnit& cur = Cur(curIdx);
    const PrevTouchUnit& prev = Prev(prevIdx);
    cur.maxSqDistMmFromStart =
        update ? std::max(cur.sqDistMmFromStart, prev.maxSqDistMmFromStart) : prev.maxSqDistMmFromStart;
}

// Touch_UpdateGlobalState @ 0x6ba75b75
void UpdateGlobalState(uint8_t curIdx, uint8_t prevIdx) {
    TouchTable& touches = Touches();
    const TouchUnit& cur = touches.unit[curIdx];
    if ((cur.volatileFlags & 4) != 0 && (Prev(prevIdx).persistFlags & 8) != 0) touches.stateFlags |= 1;
    if ((cur.volatileFlags & 0x2000) != 0) touches.lastLargeObjectTime = Static().curTimeMs;
}

// ── ProcessAftTouchEvent 中的判定 ────────────────────────────────────

// Touch_CheckTrustabilityUnit @ 0x6ba718d4
// 报点中的触点每帧累计 trustFrames；出现大面积、强信号、快速移动、异常等情形时记为可疑（0x20000）。
void CheckTrustabilityUnit(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& cur = Cur(curIdx);
    const uint32_t systemFlags = Static().systemFlags;
    if ((cur.persistFlags2 & 0x100000) != 0 || (systemFlags & 0x400000) != 0 || (systemFlags & 0x800000) != 0) {
        cur.persistFlags2 |= 0x40000;
        return;
    }
    if ((cur.volatileFlags2 & 0x1000) == 0 || (cur.persistFlags & 8) == 0) return;

    ++cur.trustFrames;
    if (cur.majorMm > 0x19) cur.persistFlags2 |= 0x20000;
    if (cur.smoothEllipseRate > 0xe && cur.field0FE > 4) cur.persistFlags2 |= 0x20000;
    if (cur.sqAccMmFromPrev > 0x19 && cur.sqDistMmFromPrev > 0x19) cur.persistFlags2 |= 0x20000;
    if (cur.exceptionType != 0) cur.persistFlags2 |= 0x20000;
    if (PrevTouches().reportedThisFrame > 1) cur.persistFlags2 |= 0x20000;
    if (cur.event == TouchEvent::kLiftOff && Prev(prevIdx).event > 2 &&
        cur.maxPeakSig < PrmtFlash().peakFilterReferenceThreshold * 2 / 5) {
        cur.persistFlags2 |= 0x20000;
    }
}

// Touch_SetTrustableTag @ 0x6ba71d77
// 触点抬起时给出一次结论：1 可信（报点超过 2 帧且未记可疑），2 可疑。
void SetTrustableTag(uint8_t curIdx) {
    TouchUnit& cur = Cur(curIdx);
    cur.trustTag = 0;
    if (cur.event != TouchEvent::kLiftOff || (cur.persistFlags2 & 0x100000) != 0 || (cur.persistFlags2 & 0x40000) != 0) {
        return;
    }
    cur.persistFlags2 |= 0x100000;
    if ((cur.persistFlags2 & 0x20000) == 0) {
        if (cur.trustFrames > 2) {
            cur.trustTag = 1;
            ++Static().trustedTouchCount;
        }
    } else {
        cur.trustTag = 2;
        ++Static().suspectTouchCount;
    }
}

// Touch_IsInAbnormalShapeDetectArea @ 0x6ba74c6f
bool IsInAbnormalShapeDetectArea(uint8_t curIdx) {
    const TouchUnit& cur = Cur(curIdx);
    const uint8_t margin = (Static().featureFlags & kFeatSideTouch) != 0 ? 3 : 2;
    if (static_cast<uint16_t>(margin * 0x100) < cur.maxEdgeDistDim1 && 0x200 < cur.maxEdgeDistDim2) return true;
    if (cur.dim1Span < static_cast<uint8_t>(margin * 2) || cur.dim2Span < 4) {
        if (!GripFilter::IsGripOrTbd(Prev(cur.prevIdx).gripStage)) return true;
        return (cur.gripFlags & 0x10000000) != 0;
    }
    return true;
}

// Touch_IsAbnormalShape @ 0x6ba74dfe
bool IsAbnormalShape(uint8_t curIdx) {
    if (IsOff(curIdx)) return false;
    const TouchUnit& cur = Cur(curIdx);
    if (PrevTouches().field006 != 0 && cur.exceptionType == 0x40 && GetTimeRpt(curIdx) < 0x7fff) return false;
    if (!IsInAbnormalShapeDetectArea(curIdx)) return false;
    if ((cur.volatileFlags & 0x2000000) != 0) return true;
    if (cur.field066 > 0xe && cur.majorMm > 0x18) return true;
    if (cur.majorMm > 0x1e) return true;
    if (cur.curMajorMm > 0x27 && cur.dim1Span > 4 && cur.dim2Span > 4 && cur.majorMm > 0x13) return true;
    if ((cur.volatileFlags2 & 0x100) != 0) return true;
    return (cur.volatileFlags & 0x1000000) != 0;
}

// Touch_IsAbnormalShape2 @ 0x6ba7504e
// 细长（长轴超过短轴 3 倍）且区域信号弱。
bool IsAbnormalShape2(uint8_t curIdx) {
    if (IsOff(curIdx)) return false;
    const TouchUnit& cur = Cur(curIdx);
    return cur.majorMm >= 0x19 && cur.majorMm > cur.minorMm * 3u && cur.zoneSignalScale < PrmtDynamic().touchThold / 2;
}

// Touch_IsAbnormalShape3 @ 0x6ba7513e
bool IsAbnormalShape3(uint8_t curIdx) {
    if (IsOff(curIdx)) return false;
    const TouchUnit& cur = Cur(curIdx);
    if (cur.exceptionType == 0x40 && GetTimeRpt(curIdx) < 0x7fff) return false;
    const auto refThold = PrmtFlash().peakFilterReferenceThreshold;
    if (cur.exceptionType == 0x80 && cur.signalScale > refThold / 2) return false;
    if (!IsInAbnormalShapeDetectArea(curIdx)) return false;
    if (cur.majorMm > 0x31 || (cur.volatileFlags & 0x2000) != 0) return true;
    return PrmtFlash().sensorNum / 3 < Prpt().msDif.bigCount && cur.signalScale < refThold / 3 && cur.majorMm > 0x27;
}

// Touch_IsAbnormalShape4 @ 0x6ba7532f
bool IsAbnormalShape4(uint8_t curIdx) {
    if (IsOff(curIdx)) return false;
    const TouchUnit& cur = Cur(curIdx);
    return cur.curMajorMm >= 0x1b && cur.majorMm >= 0x1b && cur.minorMm >= 0x14 && cur.dim1Span >= 6 && cur.dim2Span >= 6;
}

// Touch_IsLiftOffDueToMerged @ 0x6ba73da0
// 抬起的触点所在格仍被其它触点的区域覆盖：被合并。
bool IsLiftOffDueToMerged(uint8_t curIdx, uint8_t prevIdx) {
    const PrevPeakEntry& prevPeak = PrevPeaks().entry[Prev(prevIdx).peakIdx];
    return IsOff(curIdx) && Sensor::IsTraversed(prevPeak.dim1, prevPeak.dim2);
}

// Touch_GetMergedInfo @ 0x6ba74b56
void GetMergedInfo(uint8_t curIdx) {
    TouchTable& touches = Touches();
    const Point pos = touches.unit[curIdx].pos;
    for (uint8_t i = 0; i < touches.count; ++i) {
        if (i != curIdx &&
            Sensor::IsInZone(static_cast<uint8_t>(pos.dim1 >> 8), static_cast<uint8_t>(pos.dim2 >> 8), touches.unit[i].zoneIdx)) {
            touches.unit[i].gripFlags |= 0x100000;
        }
    }
}

// Touch_IsTouchDownDueToSeperated @ 0x6ba73e7a
// 刚按下的触点所在格早已有信号：从别的触点里分离出来。
bool IsTouchDownDueToSeperated(uint8_t curIdx) {
    const PeakEntry& peak = PeakOf(Cur(curIdx));
    return !IsOff(curIdx) && Cur(curIdx).age == 1 &&
           Static().frameIntervalMs < Buffers().timeDown[CellIndex(peak)] &&
           Sensor::IsTraversedBefore(peak.dim1, peak.dim2);
}

// Touch_IsSideOverCore @ 0x6ba714db
bool IsSideOverCore(uint8_t curIdx, uint8_t prevIdx) {
    bool over = false;
    if (Prev(prevIdx).age != 0) return over;
    for (uint8_t side = 0; side < TouchesSide().count; ++side) {
        if (IsSideCoreInSameZone(curIdx, side) && TouchesSide().unit[side].durationMs > 0x50 &&
            Cur(curIdx).edgeDistDim1 < 0x101) {
            over = true;
        }
    }
    return over;
}

// Touch_IsCoreOffInSide @ 0x6ba715a8
bool IsCoreOffInSide(uint8_t curIdx) {
    bool inSide = false;
    if (!IsOff(curIdx)) return inSide;
    for (uint8_t side = 0; side < TouchesSide().count; ++side) {
        if (IsSideCoreInSameZone(curIdx, side)) inSide = true;
    }
    return inSide;
}

// Touch_IIRDataProcess @ 0x6ba7474f
// 形状参数的 7/8 IIR；刚按下或刚变成多指拆分区域时直接取当前值。
void IirDataProcess(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& cur = Cur(curIdx);
    const PrevTouchUnit& prev = Prev(prevIdx);
    cur.shapeIirValid = 1;
    if (IsProject("P116")) {
        if (cur.field068 > 100) cur.field068 = 100;
        if (cur.field06A > 100) cur.field06A = 100;
    }
    if (cur.event < 3 || (cur.zoneShape == 3 && prev.zoneShape != 3)) {
        cur.iirMajorMm = cur.majorMm;
        cur.iirMinorMm = cur.minorMm;
        cur.iirMajorPx = cur.majorPx;
        cur.iirMinorPx = cur.minorPx;
        cur.iirAngle = cur.angle;
        cur.iirField066 = cur.field066;
        cur.iirField068 = cur.field068;
        cur.iirField06A = cur.field06A;
    } else {
        cur.iirMajorMm = static_cast<uint16_t>(IirProcess(prev.iirMajorMm, cur.majorMm));
        cur.iirMinorMm = static_cast<uint16_t>(IirProcess(prev.iirMinorMm, cur.minorMm));
        cur.iirMajorPx = static_cast<uint16_t>(IirProcess(prev.iirMajorPx, cur.majorPx));
        cur.iirMinorPx = static_cast<uint16_t>(IirProcess(prev.iirMinorPx, cur.minorPx));
        cur.iirAngle = static_cast<int8_t>(IirProcess(prev.iirAngle, cur.angle));
        cur.iirField066 = static_cast<int16_t>(IirProcess(prev.iirField066, cur.field066));
        cur.iirField068 = static_cast<uint16_t>(IirProcess(prev.iirField068, cur.field068));
        cur.iirField06A = static_cast<uint16_t>(IirProcess(prev.iirField06A, cur.field06A));
    }
    if (cur.iirMinorPx == 0) cur.iirMinorPx = 1;
    cur.iirAxisRatioX16 = static_cast<uint8_t>((static_cast<uint32_t>(cur.iirMajorPx) << 4) / cur.iirMinorPx);
}

// Touch_ProcessAftTouchEventUnit @ 0x6ba7542a
void ProcessAftTouchEventUnit(uint8_t curIdx) {
    TouchTable& touches = Touches();
    TouchUnit& cur = touches.unit[curIdx];
    const uint8_t prevIdx = cur.prevIdx;

    CheckTrustabilityUnit(curIdx, prevIdx);
    SetTrustableTag(curIdx);
    if (IsAbnormalShape(curIdx) || (cur.persistFlags3 & 4) != 0) ++touches.abnormalShapeCount;
    if (IsAbnormalShape2(curIdx)) ++touches.abnormalShape2Count;
    if (IsAbnormalShape3(curIdx)) {
        ++touches.abnormalShape3Count;
        cur.persistFlags3 |= 4;
    }
    if (IsAbnormalShape4(curIdx)) ++touches.abnormalShape4Count;

    if ((cur.volatileFlags2 & 0x1000) != 0) {
        if (!IsOff(curIdx)) {
            ++touches.reportedThisFrame;
            touches.lastReportedId = cur.id;
            if ((cur.persistFlags4 & 1) != 0 || (cur.gripFlags & 0x200000) != 0) ++touches.gripReportedThisFrame;
            if (cur.event < 3) touches.lastTouchDownTime = Static().curTimeMs;
        } else {
            touches.lastLiftOffTime = Static().curTimeMs;
        }
    }

    if (IsLiftOffDueToMerged(curIdx, prevIdx)) {
        cur.persistFlags2 |= 0x1000000;
        if (GripFilter::IsGripOrTbdTouch(curIdx) && PrevPeak::IsOnOuterEdge(cur.peakIdx)) {
            GetMergedInfo(curIdx);
        }
    }
    if (IsTouchDownDueToSeperated(curIdx)) cur.persistFlags2 |= 0x2000000;
    if ((cur.gripFlags & 0x100000) == 0 && (cur.volatileFlags2 & 0x2000) != 0) cur.gripFlags |= 0x100000;
    if (IsSideOverCore(curIdx, prevIdx)) cur.persistFlags2 |= 0x400000;
    if (IsCoreOffInSide(curIdx)) cur.persistFlags2 |= 0x80000000;
    IirDataProcess(curIdx, prevIdx);

    // 长轴的轨迹最大值；互容主流程中刚按下的触点直接取当前值。
    const uint16_t prevMax = Prev(prevIdx).maxMajorMm;
    const bool msActive = (Static().processingState & kProcMsActive) != 0;
    if (prevMax < cur.curMajorMm || (msActive && cur.event == TouchEvent::kTouchDown)) {
        cur.maxMajorMm = cur.curMajorMm;
    } else {
        cur.maxMajorMm = prevMax;
    }
}

// Touch_IsBetaGrip @ 0x6ba76d53
// 屏幕左右边缘（显示 X 在 9 像素内）、底部 15 mm 以内的稳定强触点：左侧 4，右侧 8，否则 0。
int IsBetaGrip(uint8_t curIdx, uint8_t prevIdx) {
    const bool charger = Sys::IsChargerPlugged() != 0;
    const TouchUnit& cur = Cur(curIdx);
    const int16_t peakSig = PeakOf(cur).sig;
    const auto refThold = PrmtFlash().peakFilterReferenceThreshold;
    const uint32_t event = cur.event;
    const bool exception = IsException(curIdx, prevIdx);
    bool beta = !exception && event == TouchEvent::kMove && refThold / 2 <= peakSig && !charger;

    if (Out::DisplayXPos(curIdx) > 8) {
        const uint16_t x = Out::DisplayXPos(curIdx);
        const uint16_t xRes = Report::XRes();
        if (x < xRes - 9) beta = false;
    }
    const uint16_t y = Out::DisplayYPos(curIdx);
    const uint16_t yRes = Report::YRes();
    const auto bottomBand = static_cast<uint16_t>(TouchSize::MmToDisplayPixel(15));
    if (y < yRes - 1 - bottomBand) beta = false;
    if ((Prev(prevIdx).persistFlags & 0x1000000) != 0) beta = false;

    if (!beta) return 0;
    return Out::DisplayXPos(curIdx) < 9 ? 4 : 8;
}

}  // namespace

// ── 表维护 ────────────────────────────────────────────────────────────

// Touch_Init @ 0x6ba6f503
void Init() {
    Clean();
}

// Touch_Clean @ 0x6ba6f49c
void Clean() {
    ZeroFill(Touches());
}

// Touch_AssignPreIdxBasedOnID @ 0x6ba73fc7
void AssignPreIdxBasedOnId() {
    TouchTable& cur = Touches();
    const PrevTouchTable& prev = PrevTouches();
    for (uint8_t i = 0; i < cur.count; ++i) {
        uint8_t j = 0;
        while (j < prev.count && prev.unit[j].id != cur.unit[i].id) ++j;
        cur.unit[i].prevIdx = j < prev.count ? j : kNoTouch;
    }
}

// Touch_GetIdxBasedOnPreIdx @ 0x6ba74433
uint8_t GetIdxBasedOnPreIdx(uint8_t prevIdx) {
    return Prev(prevIdx).curIdx;
}

// Touch_KeepPrevTouchWithExceptionExcluded @ 0x6ba766b9
void KeepPrevTouchWithExceptionExcluded() {
    TouchTable& cur = Touches();
    PrevTouchTable& prev = PrevTouches();
    cur.liftOffCount = prev.liftOffCount;
    cur.field004 = prev.field004;
    cur.extFlags = prev.extFlags;
    cur.newExtFlags = prev.newExtFlags;
    cur.reportedCount = prev.reportedCount;
    cur.field002 = prev.field002;
    cur.maxCoreEllipseRate = prev.maxCoreEllipseRate;
    cur.reportCount = prev.reportCount;
    cur.coreTouchCount = prev.coreTouchCount;
    cur.field00C = prev.field00C;
    cur.field00D = prev.field00D;
    cur.field00E = prev.field00E;
    cur.field00F = prev.field00F;
    cur.abnormalShapeCount = prev.abnormalShapeCount;
    cur.abnormalShape2Count = prev.abnormalShape2Count;
    cur.abnormalShape3Count = prev.abnormalShape3Count;
    cur.abnormalShape4Count = prev.abnormalShape4Count;
    HardwareAnalyzer::ReportToTsa();
    prev.exceptionFlags = cur.exceptionFlags & ~0x400u;
    cur.stateFlags = prev.stateFlags;
    cur.lastLargeObjectTime = prev.lastLargeObjectTime;
    cur.field138 = prev.field138;
    Thenar::CopyInfo(cur.thenar, prev.thenar);
    for (uint8_t i = 0; i < prev.count; ++i) UnitCopy(i, i);
    cur.count = prev.count;
}

// Touch_UnitCopy @ 0x6ba74462
void UnitCopy(uint8_t curIdx, uint8_t prevIdx) {
    TsaAssert(prevIdx < kMaxTouchNum);
    TsaAssert(curIdx < kMaxTouchNum);
    if (prevIdx >= kMaxTouchNum) prevIdx = kMaxTouchNum - 1;
    if (curIdx >= kMaxTouchNum) curIdx = kMaxTouchNum - 1;
    TouchUnit& cur = Cur(curIdx);
    const int32_t maxSigSum = cur.maxSigSum;
    const uint8_t sigSumRatio = cur.sigSumRatio;
    const uint8_t znRatio = cur.znRatio;
    const uint8_t peakSigRatio = cur.peakSigRatio;
    const uint32_t persistFlags3 = cur.persistFlags3;
    std::memcpy(&cur, &Prev(prevIdx), kTouchCommonSize);
    cur.maxSigSum = maxSigSum;
    cur.sigSumRatio = sigSumRatio;
    cur.znRatio = znRatio;
    cur.peakSigRatio = peakSigRatio;
    cur.persistFlags3 = persistFlags3;
}

// Touch_UnitRemove @ 0x6ba763f4
void UnitRemove(uint8_t curIdx) {
    TouchTable& touches = Touches();
    TsaAssert(touches.count <= kMaxTouchNum);
    if (touches.count > kMaxTouchNum) touches.count = kMaxTouchNum;
    if (curIdx >= touches.count) return;
    for (int i = curIdx; i < touches.count - 1; ++i) touches.unit[i] = touches.unit[i + 1];
    --touches.count;
}

// Touch_UnitUpdate @ 0x6ba760da
void UnitUpdate(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& cur = Cur(curIdx);
    const PrevTouchUnit& prev = Prev(prevIdx);
    cur.smoothEllipseRate = static_cast<int16_t>((prev.smoothEllipseRate * 7 + cur.ellipseRate) / 8);
    cur.trustFrames = prev.trustFrames;
    cur.prevDt = prev.dt;
    cur.dt = GetFormattedTimeInterval(
        static_cast<int16_t>(static_cast<int16_t>(cur.timestamp) - static_cast<int16_t>(prev.timestamp)));
    GetSqDistMmFromPrevPos(curIdx, prevIdx);
    GetSqDistPixelFromPrevPos(curIdx, prevIdx);
    GetSqDistMmFromStartPos(curIdx, prevIdx);
    GetSqAccMmFromPrevPos(curIdx, prevIdx);
    SetEdgeFlag(curIdx);
    SetRegion(curIdx);
    SetVolatileFlag(curIdx, prevIdx);
    SetPersistFlag(curIdx, prevIdx);
    // Touch_SetSuppressFlag 在本版本中为空函数
    UnitUpdateMaxMin(curIdx, prevIdx);
    UnitUpdateSqDistMmFromStartPosMax(curIdx, prevIdx);
    UpdateGlobalState(curIdx, prevIdx);
}

// Touch_SetEdgeDist @ 0x6ba72b19
void SetEdgeDist(uint8_t curIdx) {
    TouchUnit& cur = Cur(curIdx);
    cur.edgeDistDim1 = static_cast<uint16_t>(GetDim1EdgeDist(cur.pos.dim1));
    cur.edgeDistDim2 = static_cast<uint16_t>(GetDim2EdgeDist(cur.pos.dim2));
}

// Touch_ProcessAftTouchEvent @ 0x6ba75af5
void ProcessAftTouchEvent() {
    TouchTable& touches = Touches();
    touches.reportedThisFrame = 0;
    touches.gripReportedThisFrame = 0;
    touches.lastTouchDownTime = PrevTouches().lastTouchDownTime;
    touches.lastLiftOffTime = PrevTouches().lastLiftOffTime;
    for (uint8_t i = 0; i < touches.count; ++i) ProcessAftTouchEventUnit(i);
}

// Touch_ProcessBetaGrip @ 0x6ba76ee8
void ProcessBetaGrip() {
    for (uint8_t i = 0; i < Touches().count; ++i) {
        const uint8_t prevIdx = PrevTouch::GetIdxBasedOnCurIdx(i);
        const int side = IsBetaGrip(i, prevIdx);
        PrevTouchUnit& prev = Prev(prevIdx);
        if (side == 4 || side == 8) {
            ++prev.betaGripFrames;
            uint32_t& count = side == 4 ? ThenarExternal().betaGripLeftCount : ThenarExternal().betaGripRightCount;
            if (prev.betaGripFrames > 1 && count < 0xfffffffeu) {
                Cur(i).persistFlags |= 0x1000000;
                ++count;
            }
        } else {
            prev.betaGripFrames = 0;
        }
    }
}

// Touch_ProcessExtInfo @ 0x6ba7713d
// 接近检测、拿起 / 放下手势与姿态模块在本版本中被裁掉：不置 0x80，拿起 0x100 与放下 0x200 恒置位。
void ProcessExtInfo() {
    TouchTable& touches = Touches();
    if (Report::IsUnderWaterDetected()) touches.extFlags |= 0x20;
    if (Report::IsOutOfWaterDetected()) touches.extFlags |= 0x40;
    touches.extFlags |= 0x100;
    touches.extFlags |= 0x200;
    AntiTouch::Report();
}

// Touch_CreatePostReport @ 0x6ba76a03
void CreatePostReport(uint8_t curIdx) {
    const uint8_t prevIdx = PrevTouch::GetIdxBasedOnCurIdx(curIdx);
    const PrevTouchUnit& prev = Prev(prevIdx);
    if ((prev.persistFlags & 0x20020000) != 0) return;

    TouchUnit& cur = Cur(curIdx);
    cur.persistFlags |= 0x20000;
    const Point start = (cur.persistFlags & 8) != 0 ? prev.firstRptPos : prev.startPos;
    const uint32_t startDim1 = start.dim1;
    const uint32_t startDim2 = start.dim2;
    const uint32_t endDim1 = cur.pos.dim1;
    const uint32_t endDim2 = cur.pos.dim2;
    const auto length = static_cast<uint8_t>(GetPostReportLength(static_cast<int>(startDim1), static_cast<int>(startDim2),
                                                                 static_cast<int>(endDim1), static_cast<int>(endDim2)));
    const int step1 = static_cast<int>(endDim1 - startDim1) / (length - 1);
    const int step2 = static_cast<int>(endDim2 - startDim2) / (length - 1);
    const auto span = static_cast<int32_t>(static_cast<uint32_t>(cur.timestamp) - static_cast<uint32_t>(prev.timestamp));

    PrevTouch::HistoryBufferClean(prevIdx);
    HistoryEntry entry{};
    for (uint8_t k = 0; k < length - 1; ++k) {
        entry.pos.dim1 = static_cast<int16_t>(k * static_cast<int16_t>(step1) + static_cast<int16_t>(startDim1));
        entry.pos.dim2 = static_cast<int16_t>(k * static_cast<int16_t>(step2) + static_cast<int16_t>(startDim2));
        entry.time = cur.timestamp - static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(k * static_cast<uint32_t>(span))));
        PrevTouch::HistoryBufferPush(prevIdx, entry);
    }
    cur.persistFlags |= 0x80;
}

// ── 报点距离 ──────────────────────────────────────────────────────────

// Touch_GetRptSQDistMMFromStartPos @ 0x6ba739e1
void GetRptSqDistMmFromStartPos(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& cur = Cur(curIdx);
    const PrevTouchUnit& prev = Prev(prevIdx);
    cur.rptSqDistMmFromFirstRpt = GetSqDistMm(cur.rptPos.dim1, cur.rptPos.dim2, prev.firstRptPos.dim1, prev.firstRptPos.dim2);
}

// Touch_GetRptSQDistMMFromPrevPos @ 0x6ba73ac1
void GetRptSqDistMmFromPrevPos(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& cur = Cur(curIdx);
    const PrevTouchUnit& prev = Prev(prevIdx);
    cur.rptSqDistMmFromPrev = GetSqDistMm(cur.rptPos.dim1, cur.rptPos.dim2, prev.rptPos.dim1, prev.rptPos.dim2);
}

// Touch_GetRptSQDistDisplayPixelFromPrevPos @ 0x6ba73ba1
void GetRptSqDistDisplayPixelFromPrevPos(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& cur = Cur(curIdx);
    const PrevTouchUnit& prev = Prev(prevIdx);
    const uint32_t curDim1 = Out::Pixel2DisplayDim1(cur.rptPos.dim1);
    const uint32_t prevDim1 = Out::Pixel2DisplayDim1(prev.rptPos.dim1);
    const uint32_t curDim2 = Out::Pixel2DisplayDim2(cur.rptPos.dim2);
    const uint32_t prevDim2 = Out::Pixel2DisplayDim2(prev.rptPos.dim2);
    cur.rptSqDistDisplayPxFromPrev =
        (curDim2 - prevDim2) * (curDim2 - prevDim2) + (curDim1 - prevDim1) * (curDim1 - prevDim1);
}

// Touch_GetFltSQDistPixelFromPrevPos @ 0x6ba73cc1
void GetFltSqDistPixelFromPrevPos(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& cur = Cur(curIdx);
    const PrevTouchUnit& prev = Prev(prevIdx);
    cur.fltSqDistPxFromPrev = GetSqDistPixel(cur.rptPos.dim1, cur.rptPos.dim2, prev.rptPos.dim1, prev.rptPos.dim2);
}

// Touch_GetSQDistMMInBetween @ 0x6ba73515
uint16_t GetSqDistMmInBetween(uint8_t idxA, uint8_t idxB) {
    const Point a = Cur(idxA).pos;
    const Point b = Cur(idxB).pos;
    return GetSqDistMm(a.dim1, a.dim2, b.dim1, b.dim2);
}

// ── 查询 ──────────────────────────────────────────────────────────────

// Touch_IsOffEvent @ 0x6ba74a8e
bool IsOffEvent(uint32_t event) {
    return event == TouchEvent::kLiftOff || event == TouchEvent::kLiftOffDebounce || event == TouchEvent::kLiftOffHold;
}

// Touch_IsOff @ 0x6ba74ab5
bool IsOff(uint8_t curIdx) {
    return IsOffEvent(Cur(curIdx).event);
}

// Touch_IsException @ 0x6ba7416a
bool IsException(uint8_t curIdx, uint8_t prevIdx) {
    return (curIdx != kNoTouch && Cur(curIdx).exceptionType != 0) ||
           (prevIdx != kNoTouch && Prev(prevIdx).exceptionType != 0) || Touches().exceptionFlags != 0 ||
           PrevTouches().exceptionFlags != 0;
}

// Touch_IsGrip @ 0x6ba742be
bool IsGrip(uint8_t curIdx) {
    return GripFilter::IsGripTouch(curIdx);
}

// Touch_IsGhostByMinSigOff @ 0x6ba7160b
// 信号刻度不到所在格抬起后最小信号的两倍：可能是残影。
bool IsGhostByMinSigOff(uint8_t curIdx) {
    if (!Peak::IsTouchValid(curIdx)) return false;
    const TouchUnit& cur = Cur(curIdx);
    return cur.signalScale < Buffers().minSigOff[CellIndex(PeakOf(cur))] * 2;
}

// Touch_IsGhostByMinSigOff2 @ 0x6ba7173d
bool IsGhostByMinSigOff2(uint8_t curIdx) {
    const TouchUnit& cur = Cur(curIdx);
    return cur.signalScale < Prev(cur.prevIdx).minSigOffAtTouchDown * 2;
}

// Touch_IsFullyLiftOff @ 0x6ba717cf
bool IsFullyLiftOff(uint8_t curIdx) {
    return Buffers().minSigOff[CellIndex(PeakOf(Cur(curIdx)))] <= PrmtDynamic().sigThold / 2;
}

// Touch_IsSideCoreInSameZone @ 0x6ba71415
bool IsSideCoreInSameZone(uint8_t coreIdx, uint8_t sideIdx) {
    {
        const uint8_t zone = TouchesGrid().unit[coreIdx].zoneIdx;
        const auto dim2 = static_cast<uint8_t>(Report::SideTouchDim2Center(sideIdx));
        const auto dim1 = static_cast<uint8_t>(Report::SideTouchDim1Center(sideIdx));
        if (Sensor::IsInCoreZone(dim1, dim2, zone)) return true;
    }
    const uint8_t zone = TouchesSide().unit[sideIdx].zoneIdx;
    const auto dim2 = static_cast<uint8_t>(Report::CoreTouchDim2Center(coreIdx));
    const auto dim1 = static_cast<uint8_t>(Report::CoreTouchDim1Center(coreIdx));
    return Sensor::IsInSideZone(dim1, dim2, zone);
}

// Touch_NeedXYFilter @ 0x6ba742dc
bool NeedXyFilter(uint8_t curIdx, uint8_t prevIdx) {
    return (Prev(prevIdx).persistFlags & 0x40) != 0 ||
           (Cur(curIdx).event & (TouchEvent::kMove | TouchEvent::kLiftOffHold)) != 0;
}

// Touch_ValidComparison @ 0x6ba7435e
bool ValidComparison(uint8_t curIdx, uint8_t prevIdx) {
    const TouchUnit& cur = Cur(curIdx);
    const PrevTouchUnit& prev = Prev(prevIdx);
    return (cur.volatileFlags & 4) != 0 && (prev.volatileFlags & 4) != 0 && cur.zoneShape == prev.zoneShape;
}

// Touch_GetTimeOn @ 0x6ba6f517
int GetTimeOn(uint8_t curIdx) {
    return Buffers().timeDown[CellIndex(Cur(curIdx).pos)];
}

// Touch_GetTimeRpt @ 0x6ba6f5ac
int GetTimeRpt(uint8_t curIdx) {
    return Buffers().timeRpt[CellIndex(Cur(curIdx).pos)];
}

}  // namespace Solvers::Tsa::Core::Touch
