#pragma once
// 按格查询：两格间的距离，以及某格在触摸区域图（TZ 缓冲区）里的归属。
//
// 区域图每格一个 int8：0 表示不属于任何触摸区域；否则绝对值是“区域编码”，同一区域的格
// 编码落在 [区域基值, 区域基值 + 8) 内，低 3 位是格在区域里的位置类型，(编码 - 16) / 8 是区号。

#include <cstdint>

namespace Solvers::Tsa::Core::Sensor {

/// 区号的“无”。
inline constexpr uint8_t kNoZone = 0xff;

/// 曼哈顿距离 |Δdim1| + |Δdim2|。
[[nodiscard]] int Distance(uint8_t dim1A, uint8_t dim2A, uint8_t dim1B, uint8_t dim2B) noexcept;
/// 距离平方 Δdim1² + Δdim2²。
[[nodiscard]] int SquareDistance(uint8_t dim1A, uint8_t dim2A, uint8_t dim1B, uint8_t dim2B) noexcept;
/// (dim1, dim2) 落在以 (centerDim1, centerDim2) 为中心、边长 boxSize 的方框内。
[[nodiscard]] bool IsInBox(uint8_t dim1, uint8_t dim2, uint8_t centerDim1, uint8_t centerDim2, uint8_t boxSize) noexcept;

/// 本帧区域图里该格的位置类型（0..7，0 为不在任何区域）；cell = dim1 + dim2 × 列数。
[[nodiscard]] uint8_t PosType(uint16_t cell) noexcept;
[[nodiscard]] uint8_t PosType(uint8_t dim1, uint8_t dim2) noexcept;
/// 本帧区域图里该格的区号，不在任何区域时为 kNoZone。
[[nodiscard]] uint8_t ZoneIndex(uint16_t cell) noexcept;
[[nodiscard]] uint8_t ZoneIndex(uint8_t dim1, uint8_t dim2) noexcept;

/// 该格被触摸区域覆盖（位置类型不是 0、2、7），分别按本帧与上一帧的区域图。
[[nodiscard]] bool IsTraversed(uint8_t dim1, uint8_t dim2) noexcept;
[[nodiscard]] bool IsTraversedBefore(uint8_t dim1, uint8_t dim2) noexcept;

/// 该格属于以 zoneBase 为基值的区域，分别查本帧、Grid 与 Side 的区域图。
[[nodiscard]] bool IsInZone(uint8_t dim1, uint8_t dim2, uint8_t zoneBase) noexcept;
[[nodiscard]] bool IsInCoreZone(uint8_t dim1, uint8_t dim2, uint8_t zoneBase) noexcept;
[[nodiscard]] bool IsInSideZone(uint8_t dim1, uint8_t dim2, uint8_t zoneBase) noexcept;

/// 两格都属于某个区域，且区号不同。
[[nodiscard]] bool IsInDifferentZones(uint8_t dim1A, uint8_t dim2A, uint8_t dim1B, uint8_t dim2B) noexcept;

}  // namespace Solvers::Tsa::Core::Sensor
