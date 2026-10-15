#include "App/Report.h"

#include "Alg/Touch/TouchData.h"
#include "App/Tsa.h"
#include "App/TsaBuffer.h"
#include "App/TsaOut.h"
#include "App/TsaStatic.h"
#include "Extension/HardwareAnalyzer/HardwareAnalyzer.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"
#include "Prmt/TsaPrmt.h"

namespace Solvers::Tsa::Core::Report {

namespace {

// AFT（报点重排）被裁掉，报点接口只走触点表。
constexpr uint32_t kFeatAltEvents = 0x1000000;  // 按下 / 抬起用另一组事件位
constexpr uint32_t kFeatPad = 0x800000;
constexpr uint32_t kFeatWindowsPad = 0x1000000;

constexpr uint32_t kEventDown = 0x2;
constexpr uint32_t kEventDownAlt = 0x1e;
constexpr uint32_t kEventRptOff = 0x20;
constexpr uint32_t kEventRptOffAlt = 0x40;

constexpr uint32_t kTouchUnderWater = 0x2000;  // TouchTable::exceptionFlags
constexpr uint32_t kTouchOutOfWater = 0x4000;

constexpr uint32_t kEdgeDim1Near = 1, kEdgeDim1Far = 2, kEdgeDim2Near = 4, kEdgeDim2Far = 8;

const TouchUnit& Unit(uint8_t idx) { return Touches().unit[idx]; }

void SetDim1Res(uint16_t res) {
    PrmtFlash().dim1Res = res;
    ConstPrmt::Init();
}

void SetDim2Res(uint16_t res) {
    PrmtFlash().dim2Res = res;
    ConstPrmt::Init();
}

/// 到两条边的距离里取近的那条（距离按 16 位截断后比较）。
struct Nearest {
    int nearDist, farDist;
    uint16_t dist;
};
Nearest NearestOf(uint16_t pos, uint16_t nearEdge, uint16_t farEdge) {
    const int toNear = pos - nearEdge;
    const int toFar = farEdge - pos;
    return {toNear, toFar, static_cast<uint16_t>(toNear <= toFar ? toNear : toFar)};
}

}  // namespace

// TSA_RptTouchNum @ 0x6ba9b2a5
uint8_t TouchNum() { return Touches().count; }

// TSA_RptTouchID @ 0x6ba9b2f2
int TouchId(uint8_t idx) { return Unit(idx).id - 1; }

// TSA_RptTouchXPos @ 0x6ba9b37a / TSA_RptTouchYPos
uint16_t TouchXPos(uint8_t idx) { return Out::RptXPos(idx); }
uint16_t TouchYPos(uint8_t idx) { return Out::RptYPos(idx); }

// TSA_RptTouchPressure
uint16_t TouchPressure(uint8_t idx) { return Unit(idx).pressure; }

// TSA_RptIsTouchDown / TSA_RptIsLiftOff
uint32_t IsTouchDown(uint8_t idx) {
    return Unit(idx).event & ((Static().featureFlags & kFeatAltEvents) != 0 ? kEventDownAlt : kEventDown);
}
uint32_t IsLiftOff(uint8_t idx) {
    return Unit(idx).event & ((Static().featureFlags & kFeatAltEvents) != 0 ? kEventRptOffAlt : kEventRptOff);
}

// TSA_RptLONum
uint8_t LiftOffNum() { return Touches().liftOffCount; }

// TSA_RptTouchXCenter / YCenter / YGripCtd
uint16_t TouchXCenter(uint8_t idx) {
    const Point& p = Unit(idx).pos;
    return (IsDim1DisplayX() ? p.dim1 : p.dim2) >> 8;
}
uint16_t TouchYCenter(uint8_t idx) {
    const Point& p = Unit(idx).pos;
    return (IsDim1DisplayX() ? p.dim2 : p.dim1) >> 8;
}
uint8_t TouchYGripRatio(uint8_t idx) {
    const TouchUnit& u = Unit(idx);
    return IsDim1DisplayX() ? u.gripRatioDim2 : u.gripRatioDim1;
}

// TSA_SetXPos @ 0x6ba9bb2e
void SetXPos(uint8_t idx, uint16_t x) {
    Point& p = Touches().unit[idx].rptPos;
    if (IsDim1DisplayX()) {
        p.dim1 = static_cast<uint16_t>(Out::RevertDim1Pos(x));
    } else {
        p.dim2 = static_cast<uint16_t>(Out::RevertDim2Pos(x));
    }
}

// TSA_CoreTouchDim1Center 等
uint16_t CoreTouchDim1Center(uint8_t idx) { return TouchesGrid().unit[idx].pos.dim1 >> 8; }
uint16_t CoreTouchDim2Center(uint8_t idx) { return TouchesGrid().unit[idx].pos.dim2 >> 8; }
uint16_t SideTouchDim1Center(uint8_t idx) { return TouchesSide().unit[idx].pos.dim1 >> 8; }
uint16_t SideTouchDim2Center(uint8_t idx) { return TouchesSide().unit[idx].pos.dim2 >> 8; }

// TSA_GetXRes / TSA_GetYRes
uint16_t XRes() { return IsDim1DisplayX() ? PrmtFlash().dim1Res : PrmtFlash().dim2Res; }
uint16_t YRes() { return IsDim1DisplayX() ? PrmtFlash().dim2Res : PrmtFlash().dim1Res; }

// TSA_SetXRes / TSA_SetYRes（TSA_SetDim1Res / TSA_SetDim2Res）
void SetXRes(uint16_t res) { IsDim1DisplayX() ? SetDim1Res(res) : SetDim2Res(res); }
void SetYRes(uint16_t res) { IsDim1DisplayX() ? SetDim2Res(res) : SetDim1Res(res); }

// TSA_SetResScale
void SetResScale(uint16_t scale) {
    PrmtFlash().outputResScale = scale;
    ConstPrmt::Init();
}

// TSA_GetYLength：Dim1 是列方向
uint8_t YLength() { return IsDim1DisplayX() ? PrmtFlash().dim2Len : PrmtFlash().dim1Len; }

// TSA_GetDimEdge @ 0x6ba9fb4a
// 两个方向各取近的一侧，Dim2 方向更近时报 Dim2 的边，否则报 Dim1 的边（相等时偏向近端）。
uint32_t GetDimEdge(uint16_t dim1, uint16_t dim2) {
    const FlashConstPrmt& c = PrmtFlashConst();
    const Nearest d1 = NearestOf(dim1, c.dim1NearEdge, c.dim1FarEdge);
    const Nearest d2 = NearestOf(dim2, c.dim2NearEdge, c.dim2FarEdge);
    if (d2.dist < d1.dist) return d2.farDist < d2.nearDist ? kEdgeDim2Far : kEdgeDim2Near;
    return d1.farDist < d1.nearDist ? kEdgeDim1Far : kEdgeDim1Near;
}

// TSA_DimEdgeToSide / TSA_SideToDimEdge：两个方向的映射互为逆，且相同
uint32_t DimEdgeToSide(uint32_t dimEdge) {
    switch (dimEdge) {
    case kEdgeDim1Near:
    case kEdgeDim1Far:
        return PrmtFlash().dim1OnLongAxis != 0 ? dimEdge : dimEdge << 2;
    case kEdgeDim2Near:
    case kEdgeDim2Far:
        return PrmtFlash().dim1OnLongAxis != 0 ? dimEdge : dimEdge >> 2;
    default:
        return 0;
    }
}
uint32_t SideToDimEdge(uint32_t side) { return DimEdgeToSide(side); }

// TSA_SideToEdge @ 0x6ba9fd96
uint8_t SideToEdge(uint32_t side) {
    if ((side & 4) != 0) return 2;
    if ((side & 8) != 0) return 3;
    if ((side & 1) != 0) return 0;
    if ((side & 2) != 0) return 1;
    HardwareAnalyzer::SetSoftException();
    return 0;
}

// TSA_GetSide / TSA_GetTouchSide
uint32_t GetSide(uint16_t dim1, uint16_t dim2) { return DimEdgeToSide(GetDimEdge(dim1, dim2)); }
uint32_t GetTouchSide(uint8_t idx) {
    const Point& p = Unit(idx).pos;
    return GetSide(p.dim1, p.dim2);
}

// TSA_IsUnderWaterDetected / TSA_IsOutOfWaterDetected
bool IsUnderWaterDetected() { return (Touches().exceptionFlags & kTouchUnderWater) != 0; }
bool IsOutOfWaterDetected() { return (Touches().exceptionFlags & kTouchOutOfWater) != 0; }

// TSA_IsPossibleUnderWater @ 0x6ba9e2d9
bool IsPossibleUnderWater() {
    const int eighth = PrmtFlash().sensorNum >> 3;
    return eighth * 7 < static_cast<uint16_t>(Prpt().msUnderWaterCells);
}

// TSA_IsPadFeatureEnabled / TSA_IsWindowsPadFeatureEnabled
bool IsPadFeatureEnabled() { return (Static().featureFlags & kFeatPad) != 0; }
bool IsWindowsPadFeatureEnabled() { return (Static().featureFlags & kFeatWindowsPad) != 0; }

}  // namespace Solvers::Tsa::Core::Report

// 宿主与 raw 的调用方
extern "C" {
namespace R = Solvers::Tsa::Core::Report;
uint32_t TSA_RptTouchNum(void) { return R::TouchNum(); }
int TSA_RptTouchID(uint8_t idx) { return R::TouchId(idx); }
uint64_t TSA_RptTouchXPos(uint8_t idx) { return R::TouchXPos(idx); }
uint64_t TSA_RptTouchYPos(uint8_t idx) { return R::TouchYPos(idx); }
uint32_t TSA_RptTouchPressure(uint8_t idx) { return R::TouchPressure(idx); }
uint32_t TSA_RptIsTouchDown(uint8_t idx) { return R::IsTouchDown(idx); }
uint32_t TSA_RptIsLiftOff(uint8_t idx) { return R::IsLiftOff(idx); }
uint64_t TSA_RptDiffPtr(void) { return reinterpret_cast<uint64_t>(Solvers::Tsa::Core::Buffers().dif); }
uint8_t TSA_RptLONum(void) { return R::LiftOffNum(); }
uint16_t TSA_RptTouchXCenter(uint8_t idx) { return R::TouchXCenter(idx); }
uint16_t TSA_RptTouchYCenter(uint8_t idx) { return R::TouchYCenter(idx); }
uint8_t TSA_RptTouchYGripCtd(uint8_t idx) { return R::TouchYGripRatio(idx); }
uint16_t TSA_GetXRes(void) { return R::XRes(); }
void TSA_SetXRes(uint16_t res) { R::SetXRes(res); }
void TSA_SetYRes(uint16_t res) { R::SetYRes(res); }
uint64_t TSA_SetResScale(uint16_t scale) {
    R::SetResScale(scale);
    return 0;
}
uint16_t TSA_GetYLength(void) { return R::YLength(); }
const char* TSA_GetPrmtName(void) { return Solvers::Tsa::Core::ProjectName(); }
uint32_t TSA_GetSide(uint16_t dim1, uint16_t dim2) { return R::GetSide(dim1, dim2); }
uint32_t TSA_GetTouchSide(uint8_t idx) { return R::GetTouchSide(idx); }
uint64_t TSA_SideToEdge(uint32_t side) { return R::SideToEdge(side); }
uint32_t TSA_IsWindowsPadFeatureEnabled(void) { return R::IsWindowsPadFeatureEnabled() ? 1 : 0; }
}  // extern "C"
