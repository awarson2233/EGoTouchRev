#include "Alg/Peak/Peak.h"

#include <utility>

#include "Alg/Touch/TouchData.h"
#include "Alg/TouchThold/TouchThold.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::Peak {

namespace {

// Peak_Z1Filter @ 0x6ba6848b
// 去掉峰值低于门限的峰（从后往前，删除不影响尚未检查的下标）。
void Z1Filter(int16_t thold) {
    for (auto i = static_cast<int8_t>(Peaks().count - 1); i >= 0; --i) {
        if (Peaks().entry[i].sig < thold) Remove(static_cast<uint8_t>(i));
    }
}

// Peak_Z8Filter @ 0x6ba68502
// 去掉孤立尖峰：八邻域和不到峰值的 1/32。
void Z8Filter() {
    for (auto i = static_cast<int8_t>(Peaks().count - 1); i >= 0; --i) {
        const PeakEntry& e = Peaks().entry[i];
        if (e.z8 < e.sig / 32) Remove(static_cast<uint8_t>(i));
    }
}

// Peak_IsFullIncellInsideSensor @ 0x6ba68944
bool IsFullIncellInsideSensor(uint8_t idx) {
    const PeakEntry& e = Peaks().entry[idx];
    return e.sig < e.z8 / 5;
}

// Peak_IsFullIncellSDDetected @ 0x6ba689c9
// 整屏 incell 信号失衡：最大峰连续 6 帧以上超过 3 倍 finger 门限、并持续 1 s 之后，
// 若最大峰回落到 2 倍 finger 门限以下而全屏信号和仍高，且峰多或峰的八邻域远大于峰值，判为失衡。
bool IsFullIncellSdDetected() {
    const PeakTable& peaks = Peaks();
    if (peaks.count == 0) return false;
    TsaStatic& stat = Static();
    const FlashPrmt& flash = PrmtFlash();
    bool detected = stat.fullIncellSignalDisparity != 0;
    const auto cap = flash.fingerThold;
    if (cap * 3 < peaks.maxSig) {
        if (stat.fullIncellHighPeakFrames < 6) {
            ++stat.fullIncellHighPeakFrames;
        } else {
            stat.fullIncellHighPeakTime = stat.curTimeMs;
        }
    } else {
        stat.fullIncellHighPeakFrames = 0;
    }
    if (stat.curTimeMs - stat.fullIncellHighPeakTime < 1001) return false;
    if (peaks.maxSig < cap * 2) {
        if (flash.peakFilterReferenceThreshold * 2 < Prpt().field0C8) {
            for (uint8_t i = 0; i < peaks.count; ++i) {
                if (peaks.count > 4) detected = true;
                if (IsFullIncellInsideSensor(i) && peaks.maxSig * 3 < peaks.entry[i].sig * 4) detected = true;
            }
        } else {
            detected = false;
        }
    }
    return detected;
}

// Peak_IDTracking @ 0x6ba68167
// 把峰当作临时触点交给 IDT 编号，再把编号写回峰表。
void IdTracking() {
    TouchTable& touches = Touches();
    PeakTable& peaks = Peaks();
    touches.count = 0;
    for (uint8_t i = 0; i < peaks.count && i != kMaxTouchNum; ++i) {
        touches.unit[i].rawPos.dim1 = static_cast<uint16_t>(peaks.entry[i].dim1 * 0x100 + 0x80);
        touches.unit[i].rawPos.dim2 = static_cast<uint16_t>(peaks.entry[i].dim2 * 0x100 + 0x80);
        ++touches.count;
    }
    IDT_Process(1);
    for (uint8_t i = 0; i < touches.count && i != kMaxTouchNum; ++i) peaks.entry[i].id = touches.unit[i].id;
}

}  // namespace

// Peak_Process @ 0x6ba68c4b
// 本机型是 Full Incell：门限封顶为 finger 门限的一半，Z8 过滤，未检出整屏失衡时再按原门限做 Z1 过滤。
void Process() {
    DynamicPrmt& dyn = PrmtDynamic();
    const FlashPrmt& flash = PrmtFlash();
    const int16_t origThold = dyn.sigThold;
    const auto cap = static_cast<int16_t>(flash.fingerThold / 2);
    if (flash.sensorArchitecture == kFullIncell && cap < dyn.sigThold) dyn.sigThold = cap;

    const bool excludeSide = IsToExcludeSideRegion();
    const auto lastCol = static_cast<uint8_t>(flash.dim1Len - (excludeSide ? 2 : 1));
    const auto lastRow = static_cast<uint8_t>(flash.dim2Len - 1);
    DetectInRange(dyn.sigThold, 0, lastRow, excludeSide ? 1 : 0, lastCol);

    if (flash.sensorArchitecture == kFullIncell) {
        Z8Filter();
        TsaStatic& stat = Static();
        if (!IsFullIncellSdDetected()) {
            stat.fullIncellSignalDisparity = 0;
            Z1Filter(origThold);
            dyn.sigThold = origThold;
        } else {
            stat.fullIncellSignalDisparity = 1;
            dyn.znTouchDownThold = dyn.sdZnTholdScaleX16 * dyn.sigThold / 16;
        }
    }
    // Hybrid Incell 时的分支依赖 SignalDisparity_IsOKToProcess，该模块在本版本中被裁掉（恒为假）。

    PeakTable& peaks = Peaks();
    uint8_t minDim2 = flash.dim2Len;
    for (int i = 0; i < peaks.count; ++i) {
        if (peaks.entry[i].dim2 < minDim2) minDim2 = peaks.entry[i].dim2;
    }
    peaks.minPeakDim2 = minDim2;
    // Peak_needUDFPLargeFingerWorkaround 恒为假（UDFPEvent 被裁掉），降序排序的分支不可达。
    SortInSigAscending();
    IdTracking();
}

// Peak_DetectInRange @ 0x6ba685d7
void DetectInRange(int16_t thold, uint8_t firstRow, uint8_t lastRow, uint8_t firstCol, uint8_t lastCol) {
    const FlashPrmt& flash = PrmtFlash();
    const int16_t* dif = Buffers().dif;
    bool toeSpecial = false;
    Clean();
    if (IsToExcludeSideRegion() && TouchThold::ToeUseSpecialThold()) toeSpecial = true;

    for (uint8_t row = firstRow; row <= lastRow; ++row) {
        auto cell = static_cast<uint16_t>(firstCol + row * flash.dim1Len - 1);
        for (uint8_t col = firstCol; col <= lastCol; ++col) {
            ++cell;
            const int16_t sig = dif[cell];
            int16_t cellThold = thold;
            if (toeSpecial && (col == 1 || col == flash.dim1Len - 2 || row == flash.dim2Len - 1)) {
                cellThold = PrmtDynamic().edgePeakThold;
            }
            if (sig < cellThold) continue;

            const bool lastC = lastCol <= col;
            const bool lastR = lastRow <= row;
            const bool firstC = col <= firstCol;
            const bool firstR = row <= firstRow;
            const auto below = static_cast<uint16_t>(cell + flash.dim1Len);
            const bool notBelowRight = (lastR || lastC || dif[static_cast<uint16_t>(below + 1)] <= sig) &&
                                       (lastR || dif[below] <= sig) &&
                                       (lastR || firstC || dif[static_cast<uint16_t>(below - 1)] <= sig) &&
                                       (lastC || dif[cell + 1] <= sig) && (firstC || dif[cell - 1] < sig);
            if (!notBelowRight) continue;
            const auto above = static_cast<uint16_t>(cell - flash.dim1Len);
            if ((firstR || lastC || dif[static_cast<uint16_t>(above + 1)] < sig) && (firstR || dif[above] < sig) &&
                (firstR || firstC || dif[static_cast<uint16_t>(above - 1)] < sig) &&
                ((Static().featureFlags2 & 0x20) == 0 || static_cast<uint8_t>(PressureDrift_Detect(col, row)) == 0)) {
                Insert(col, row);
            }
        }
    }
}

// Peak_SortInSigAscending @ 0x6ba66013
void SortInSigAscending() {
    PeakTable& peaks = Peaks();
    int16_t* dif = Buffers().dif;
    for (int8_t i = 0; i < static_cast<int8_t>(peaks.count) - 1; ++i) {
        for (auto j = static_cast<int8_t>(i + 1); j < peaks.count; ++j) {
            PeakEntry& a = peaks.entry[i];
            PeakEntry& b = peaks.entry[j];
            if (b.sig < a.sig) {
                std::swap(a, b);
            } else if (a.sig == b.sig) {
                ++b.sig;
                ++dif[GridIndex(b.dim1, b.dim2)];
            }
        }
    }
}

// Peak_CalcZn @ 0x6ba67c89
void CalcZn(uint8_t idx) {
    const FlashPrmt& flash = PrmtFlash();
    const int16_t* dif = Buffers().dif;
    const bool crossOnly = (Static().processingState & kProcIdleScan) != 0;
    PeakEntry& e = Peaks().entry[idx];
    const uint8_t col = e.dim1;
    const uint8_t row = e.dim2;
    const auto cell = static_cast<uint16_t>(col + row * flash.dim1Len);
    const int16_t center = dif[cell];
    const auto lastCol = static_cast<uint8_t>(flash.dim1Len - 1);
    const bool hasLeft = col != 0;
    const bool hasRight = col < lastCol;

    // 四个含中心的 2×2 象限和（左上、右上、左下、右下）与十字五点和
    int upLeft = center, upRight = center, downLeft = center, downRight = center, cross = center;
    int z8 = 0;
    uint8_t neighbors = 0;
    if (row < static_cast<uint8_t>(flash.dim2Len - 1)) {
        const auto below = static_cast<uint16_t>(cell + flash.dim1Len);
        if (hasRight) {
            const int16_t s = dif[static_cast<uint16_t>(below + 1)];
            downRight += s;
            z8 += s;
            ++neighbors;
        }
        const int16_t s = dif[below];
        downLeft += s;
        downRight += s;
        z8 += s;
        cross += s;
        ++neighbors;
        if (hasLeft) {
            const int16_t sl = dif[static_cast<uint16_t>(below - 1)];
            downLeft += sl;
            z8 += sl;
            ++neighbors;
        }
    }
    if (hasRight) {
        const int16_t s = dif[static_cast<uint16_t>(cell + 1)];
        upRight += s;
        downRight += s;
        z8 += s;
        if (!crossOnly) cross += s;
        ++neighbors;
    }
    if (hasLeft) {
        const int16_t s = dif[static_cast<uint16_t>(cell - 1)];
        upLeft += s;
        downLeft += s;
        z8 += s;
        if (!crossOnly) cross += s;
        ++neighbors;
    }
    if (row != 0) {
        const auto above = static_cast<uint16_t>(cell - flash.dim1Len);
        if (hasRight) {
            const int16_t s = dif[static_cast<uint16_t>(above + 1)];
            upRight += s;
            z8 += s;
            ++neighbors;
        }
        const int16_t s = dif[above];
        upLeft += s;
        upRight += s;
        z8 += s;
        cross += s;
        ++neighbors;
        if (hasLeft) {
            const int16_t sl = dif[static_cast<uint16_t>(above - 1)];
            upLeft += sl;
            z8 += sl;
            ++neighbors;
        }
    }

    int quadrant = upLeft;
    if (quadrant < upRight) quadrant = upRight;
    if (quadrant < downLeft) quadrant = downLeft;
    if (quadrant < downRight) quadrant = downRight;
    if (!crossOnly) {
        e.zn = quadrant <= cross ? cross : quadrant;
    } else {
        e.zn = cross;
    }
    e.z8 = z8;
    e.neighborCount = neighbors;
}

// Peak_IsToExcludeSideRegion @ 0x6ba685ac
bool IsToExcludeSideRegion() {
    return false;
}

// Peak_needUDFPLargeFingerWorkaround @ 0x6ba68bc6
// 仅 P115 且 UDFPEvent 打开时成立；UDFPEvent 在本版本中被裁掉，恒为假。
bool NeedUdfpLargeFingerWorkaround() {
    return false;
}

}  // namespace Solvers::Tsa::Core::Peak
