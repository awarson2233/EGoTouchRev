#include "App/TsaOut.h"

#include "Alg/Touch/PrevTouch.h"
#include "Alg/Touch/Touch.h"
#include "Alg/Touch/TouchData.h"
#include "App/Report.h"
#include "App/Scenario.h"
#include "App/Sys.h"
#include "App/TsaStatic.h"
#include "Extension/Thenar/Thenar.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::Out {

namespace {

constexpr uint32_t kVolPress = 0x4;              // TouchUnit::volatileFlags：刚按下
constexpr uint32_t kVolSuppressed = 0x8000000;
constexpr uint32_t kVol2Suppressed = 0x4000;     // TouchUnit::volatileFlags2
constexpr uint32_t kPersistReported = 0x8;       // TouchUnit::persistFlags
constexpr uint32_t kPersistRemoved = 0x10000;
constexpr uint32_t kPersistTimed = 0x20000000;   // PrevTouchUnit::persistFlags：已记过首报时间
constexpr uint32_t kNewExtHwResetDone = 0x200000;

bool XIsDim1() {
    const FlashPrmt& fl = PrmtFlash();
    return (fl.dim1Axis ^ fl.displayXAxis) == fl.displaySwapXy;
}

uint16_t Dim1Pos(uint8_t idx, uint16_t scale) {
    const FlashPrmt& fl = PrmtFlash();
    const uint16_t v = Dim1PosInternal(Touches().unit[idx].rptPos.dim1, scale);
    if (fl.dim1Flip == 0) return v;
    return static_cast<uint16_t>(static_cast<uint16_t>(fl.dim1Res * scale) - v - 1);
}

uint16_t Dim2Pos(uint8_t idx, uint16_t scale) {
    const FlashPrmt& fl = PrmtFlash();
    const uint16_t v = Dim2PosInternal(Touches().unit[idx].rptPos.dim2, scale);
    if (fl.dim2Flip == 0) return v;
    return static_cast<uint16_t>(static_cast<uint16_t>(fl.dim2Res * scale) - v - 1);
}

// TSAOut_TouchXPosInternal @ 0x6ba69f4d / TSAOut_TouchYPosInternal @ 0x6ba6a0c7
uint16_t TouchXPosInternal(uint8_t idx, uint16_t scale) { return XIsDim1() ? Dim1Pos(idx, scale) : Dim2Pos(idx, scale); }
uint16_t TouchYPosInternal(uint8_t idx, uint16_t scale) { return XIsDim1() ? Dim2Pos(idx, scale) : Dim1Pos(idx, scale); }

// TSAOut_Display2Pixel @ 0x6ba69c56
int Display2Pixel(int v) {
    const auto scale = static_cast<int>(PrmtFlashConst().dim2DisplayScale);
    return (v * 0x10000 + scale / 2) / scale;
}

// TSAOut_EdgeScale @ 0x6ba69c9e：从 edge 起按 (256 - edge) / 256 压缩
int EdgeScale(int edge, int pos) { return edge + (0x100 - edge) * pos / 256; }

// TSAOut_PostProcessBefUDFPUnit @ 0x6ba6a6f3
void PostProcessBefUdfpUnit(uint8_t idx) {
    const TouchUnit& u = Touches().unit[idx];
    if ((u.persistFlags & kPersistReported) == 0 || (u.volatileFlags & kVolSuppressed) != 0) Touch::UnitRemove(idx);
}

// TSAOut_PostProcessUnit @ 0x6ba6a3ef
// 报出的触点 X 限制在 [xMin, xMax]，计数并记下报点时间；第一次报点时在上一帧表里记时间。
void PostProcessUnit(uint8_t idx, uint8_t& pressCount) {
    TouchTable& tt = Touches();
    const TouchUnit& u = tt.unit[idx];
    if ((u.persistFlags & kPersistReported) == 0 || (u.volatileFlags & kVolSuppressed) != 0 ||
        (u.volatileFlags2 & kVol2Suppressed) != 0 || (u.persistFlags & kPersistRemoved) != 0) {
        Touch::UnitRemove(idx);
        return;
    }
    TsaStatic& s = Static();
    if (DisplayXPos(idx) < s.xMin) {
        Report::SetXPos(idx, s.xMin);
    } else if (static_cast<uint16_t>(s.xMax) < DisplayXPos(idx)) {
        Report::SetXPos(idx, static_cast<uint16_t>(s.xMax));
    }
    ++tt.reportCount;
    ++tt.field006;
    if ((u.volatileFlags & kVolPress) != 0) ++pressCount;
    s.lastReportTime = static_cast<int64_t>(s.curTimeMs);
    const uint8_t prevIdx = PrevTouch::GetIdxBasedOnId(u.id);
    if (prevIdx < kMaxTouchNum) {
        PrevTouchUnit& p = PrevTouches().unit[prevIdx];
        if ((p.persistFlags & kPersistTimed) == 0) {
            p.field458 = s.curTimeMs;
            p.persistFlags |= kPersistTimed;
        }
    }
}

}  // namespace

// TSAOut_PostProcess @ 0x6ba6a77a（UDFP 事件、侧边触摸被裁掉）
// 合上智能皮套以外的盖板（field003）且不在游戏场景时，本帧不报点。
void PostProcess() {
    TouchTable& tt = Touches();
    uint8_t pressCount = 0;
    tt.reportCount = 0;
    tt.field006 = 0;
    if (tt.liftOffCount != 0 && (Static().featureFlags & kFeatSmartCover) == 0 && !Scenario::IsGame()) {
        tt.count = 0;
    } else {
        for (int k = static_cast<int8_t>(tt.count) - 1; k >= 0; --k) PostProcessBefUdfpUnit(static_cast<uint8_t>(k));
    }
    for (int k = static_cast<int8_t>(tt.count) - 1; k >= 0; --k) PostProcessUnit(static_cast<uint8_t>(k), pressCount);
    PrevTouches().field006 = tt.field006;
    TsaStatic& s = Static();
    s.pressCount = pressCount;
    s.extFlags = tt.extFlags;
    s.newExtFlags = s.extFlags & ~s.prevExtFlags;
    if (Sys::IsHwResetPrev() && !Sys::IsHwReset()) s.newExtFlags |= kNewExtHwResetDone;
    Thenar::UpdateExternalInfo();
}

// TSAOut_DisplayXPos @ 0x6ba6a241 / TSAOut_DisplayYPos @ 0x6ba6a293
uint16_t DisplayXPos(uint8_t idx) { return TouchXPosInternal(idx, 1); }
uint16_t DisplayYPos(uint8_t idx) { return TouchYPosInternal(idx, 1); }
// TSAOut_RptXPos @ 0x6ba6a264 / TSAOut_RptYPos @ 0x6ba6a2b6
uint16_t RptXPos(uint8_t idx) { return TouchXPosInternal(idx, PrmtFlash().outputResScale); }
uint16_t RptYPos(uint8_t idx) { return TouchYPosInternal(idx, PrmtFlash().outputResScale); }

// TSAOut_Dim1PosInternal @ 0x6ba69a50（折叠屏重映射被裁掉）
uint16_t Dim1PosInternal(uint16_t pos, uint16_t scale) {
    const FlashPrmt& fl = PrmtFlash();
    int v = static_cast<int>((pos * PrmtFlashConst().dim1DisplayScale * scale + 0x8000) >> 16) - scale * fl.dim1ClipLow;
    if (v < 0) v = 0;
    const int hi = scale * fl.dim1Res - 1;
    if (hi < v) v = hi;
    return static_cast<uint16_t>(v);
}

// TSAOut_Dim2PosInternal @ 0x6ba69ccb
// 两端裁剪为负（向外延伸）时，边缘一格内的坐标先按 EdgeScale 压缩。
uint16_t Dim2PosInternal(uint16_t pos, uint16_t scale) {
    const FlashPrmt& fl = PrmtFlash();
    int p = pos;
    if (pos < 0x100) {
        if (fl.dim2ClipLow < 0) p = EdgeScale(Display2Pixel(fl.dim2ClipLow - fl.dim2EdgeLow), pos);
    } else if ((fl.dim2Len - 1) * 0x100 < p && fl.dim2ClipHigh < 0) {
        const int edge = Display2Pixel(fl.dim2ClipHigh - fl.dim2EdgeHigh);
        p = fl.dim2Len * 0x100 - EdgeScale(edge, fl.dim2Len * 0x100 - p);
    }
    // 四舍五入后按有符号除以 0x10000（向零取整）
    const auto prod = static_cast<int>(scale * PrmtFlashConst().dim2DisplayScale * static_cast<uint32_t>(p));
    int rounded = prod + 0x8000;
    if (rounded < 0) rounded = prod + 0x17fff;
    int v = (rounded >> 16) - scale * fl.dim2ClipLow;
    if (v < 0) v = 0;
    if (scale * fl.dim2Res <= v) v = scale * fl.dim2Res - 1;
    return static_cast<uint16_t>(v);
}

// TSAOut_Pixel2DisplayDim1 @ 0x6ba69b0b / TSAOut_Pixel2DisplayDim2 @ 0x6ba69e95
uint16_t Pixel2DisplayDim1(uint16_t pos) { return Dim1PosInternal(pos, 1); }
uint16_t Pixel2DisplayDim2(uint16_t pos) { return Dim2PosInternal(pos, 1); }

// TSAOut_RevertDim1Pos @ 0x6ba69b2f
uint32_t RevertDim1Pos(uint16_t pos) {
    const FlashConstPrmt& fc = PrmtFlashConst();
    int p = PrmtFlash().dim1ClipLow + pos;
    if (p < 0) p = 0;
    uint32_t v = (static_cast<uint32_t>(p) * 0x10000 + (fc.dim1DisplayScale >> 1)) / fc.dim1DisplayScale;
    if (fc.dim1LenQ8 <= v) v = fc.dim1LenQ8 - 1u;
    return v;
}

// TSAOut_RevertDim2Pos @ 0x6ba69eb9
uint32_t RevertDim2Pos(uint16_t pos) {
    const FlashConstPrmt& fc = PrmtFlashConst();
    int p = PrmtFlash().dim2ClipLow + pos;
    if (p < 0) p = 0;
    uint32_t v = (static_cast<uint32_t>(p) * 0x10000 + (fc.dim2DisplayScale >> 1)) / fc.dim2DisplayScale;
    if (fc.dim2LenQ8 <= v) v = fc.dim2LenQ8 - 1u;
    return v;
}

// TSAOut_IntToExtDist @ 0x6ba69bfb
uint32_t IntToExtDist(int16_t dist) {
    const auto a = static_cast<int16_t>(dist < 0 ? -dist : dist);
    const uint32_t v = (static_cast<uint32_t>(a * static_cast<int>(PrmtFlashConst().dim1DisplayScale)) + 0x8000u) >> 16;
    return dist < 0 ? 0u - v : v;
}

// TSAOut_ExtToIntDist @ 0x6ba69bc3
uint64_t ExtToIntDist(uint16_t dist) { return (static_cast<uint64_t>(dist) << 16) / PrmtFlashConst().dim1DisplayScale; }

// TSAOut_Axis @ 0x6ba6a2e5
uint16_t Axis(int16_t axis) { return static_cast<uint16_t>(IntToExtDist(axis)); }

// TSAOut_Angle @ 0x6ba6a305
int8_t Angle(int8_t angle) {
    const FlashPrmt& fl = PrmtFlash();
    if (fl.dim1Axis != fl.displayXAxis) return static_cast<int8_t>(-angle);
    return static_cast<int8_t>(angle < 1 ? angle + 90 : angle - 90);
}

}  // namespace Solvers::Tsa::Core::Out
