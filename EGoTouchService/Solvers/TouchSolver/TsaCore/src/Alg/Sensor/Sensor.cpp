#include "Alg/Sensor/Sensor.h"

#include <cstdlib>

#include "Common/Image.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::Sensor {

namespace {

// 区域图：本帧、上一帧、Grid（核心区）、Side（侧边区）
const int8_t* ZoneMap() noexcept { return Image::At<const int8_t*>(0x6bb81318); }
const int8_t* PrevZoneMap() noexcept { return Image::At<const int8_t*>(0x6bb81390); }
const int8_t* CoreZoneMap() noexcept { return Image::At<const int8_t*>(0x6bb81360); }
const int8_t* SideZoneMap() noexcept { return Image::At<const int8_t*>(0x6bbaa0e0); }

uint16_t Cell(uint8_t dim1, uint8_t dim2) noexcept { return static_cast<uint16_t>(dim1 + dim2 * PrmtFlash().dim1Len); }

/// 格的区域编码：绝对值按 int8 截断（-128 仍为 -128）。
int8_t Code(int8_t v) noexcept { return static_cast<int8_t>(v < 0 ? -v : v); }

/// 位置类型：绝对值按 int 算（-128 为 128）后取低 3 位。
uint8_t TypeOf(int8_t v) noexcept { return static_cast<uint8_t>(std::abs(static_cast<int>(v)) & 7); }

/// 位置类型 0（不在区域）、2、7 以外的格算被覆盖。
bool IsCovered(uint8_t type) noexcept { return type != 0 && type != 7 && type != 2; }

/// 编码落在 [zoneBase, zoneBase + 8) 内；zoneBase 为 0 表示没有区域。
bool IsInRange(int code, uint8_t zoneBase) noexcept { return zoneBase != 0 && zoneBase <= code && code < zoneBase + 8; }

}  // namespace

// Sensor_GetDistance @ 0x6ba66b17
int Distance(uint8_t dim1A, uint8_t dim2A, uint8_t dim1B, uint8_t dim2B) noexcept {
    return std::abs(dim2A - dim2B) + std::abs(dim1A - dim1B);
}

// Sensor_GetSquareDistance @ 0x6ba66b55
int SquareDistance(uint8_t dim1A, uint8_t dim2A, uint8_t dim1B, uint8_t dim2B) noexcept {
    const int d1 = dim1A - dim1B;
    const int d2 = dim2A - dim2B;
    return d2 * d2 + d1 * d1;
}

// Sensor_IsInNxNBox @ 0x6ba67234
// 半边长按 int 算后截成 8 位：boxSize 为 0 时半边长是 255。
bool IsInBox(uint8_t dim1, uint8_t dim2, uint8_t centerDim1, uint8_t centerDim2, uint8_t boxSize) noexcept {
    const int half = ((boxSize - 1) >> 1) & 0xff;
    return std::abs(dim1 - centerDim1) <= half && std::abs(dim2 - centerDim2) <= half;
}

// Sensor_GetPosType2 @ 0x6ba67374
uint8_t PosType(uint16_t cell) noexcept { return TypeOf(ZoneMap()[cell]); }

// Sensor_GetPosType @ 0x6ba67339
uint8_t PosType(uint8_t dim1, uint8_t dim2) noexcept { return PosType(Cell(dim1, dim2)); }

// Sensor_GetZoneIndex2 @ 0x6ba672e1
uint8_t ZoneIndex(uint16_t cell) noexcept {
    const int code = Code(ZoneMap()[cell]);
    return code == 0 ? kNoZone : static_cast<uint8_t>((code - 16) / 8);
}

// Sensor_GetZoneIndex @ 0x6ba672a6
uint8_t ZoneIndex(uint8_t dim1, uint8_t dim2) noexcept { return ZoneIndex(Cell(dim1, dim2)); }

// Sensor_IsTouchTraversed @ 0x6ba67472
bool IsTraversed(uint8_t dim1, uint8_t dim2) noexcept { return IsCovered(PosType(dim1, dim2)); }

// Sensor_IsTouchTraversedBefore @ 0x6ba6742c / Sensor_GetPosType3 @ 0x6ba673c2
bool IsTraversedBefore(uint8_t dim1, uint8_t dim2) noexcept { return IsCovered(TypeOf(PrevZoneMap()[Cell(dim1, dim2)])); }

// Sensor_IsInZone @ 0x6ba674b8
bool IsInZone(uint8_t dim1, uint8_t dim2, uint8_t zoneBase) noexcept { return IsInRange(Code(ZoneMap()[Cell(dim1, dim2)]), zoneBase); }

// Sensor_IsInCoreZone @ 0x6ba67540
// Grid 与 Side 的区域图直接按有符号值比较，不取绝对值。
bool IsInCoreZone(uint8_t dim1, uint8_t dim2, uint8_t zoneBase) noexcept { return IsInRange(CoreZoneMap()[Cell(dim1, dim2)], zoneBase); }

// Sensor_IsInSideZone @ 0x6ba675c0
bool IsInSideZone(uint8_t dim1, uint8_t dim2, uint8_t zoneBase) noexcept { return IsInRange(SideZoneMap()[Cell(dim1, dim2)], zoneBase); }

// Sensor_IsInDifferentTouchZone @ 0x6ba67640
bool IsInDifferentZones(uint8_t dim1A, uint8_t dim2A, uint8_t dim1B, uint8_t dim2B) noexcept {
    const uint8_t a = ZoneIndex(dim1A, dim2A);
    const uint8_t b = ZoneIndex(dim1B, dim2B);
    return a != kNoZone && b != kNoZone && a != b;
}

}  // namespace Solvers::Tsa::Core::Sensor

// raw 的 TZ / TouchMode / AntiTouch / TPSensor 还在调用的入口
extern "C" {
namespace S = Solvers::Tsa::Core::Sensor;
uint8_t Sensor_GetZoneIndex2(uint16_t cell) { return S::ZoneIndex(cell); }
uint32_t Sensor_GetPosType2(uint16_t cell) { return S::PosType(cell); }
uint8_t Sensor_GetPosType(uint8_t dim1, uint8_t dim2) { return S::PosType(dim1, dim2); }
uint8_t Sensor_GetZoneIndex(uint8_t dim1, uint8_t dim2) { return S::ZoneIndex(dim1, dim2); }
uint64_t Sensor_IsInZone(uint8_t dim1, uint8_t dim2, uint8_t zoneBase) { return S::IsInZone(dim1, dim2, zoneBase) ? 1 : 0; }
uint8_t Sensor_IsInDifferentTouchZone(uint8_t dim1A, uint8_t dim2A, uint8_t dim1B, uint8_t dim2B) {
    return S::IsInDifferentZones(dim1A, dim2A, dim1B, dim2B) ? 1 : 0;
}
}  // extern "C"
