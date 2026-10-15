#pragma once
// 定点三角函数、整数开方与插值核。

#include <cstdint>

namespace Solvers::Tsa::Core::Math {

/// 向量 (x, y) 的方向角，单位度，范围 (-180, 180]，向零取整。零向量返回 180。
/// 内部以 256 份为 180° 计算（八分圆内 64 级）。
[[nodiscard]] int ATan2(int16_t y, int16_t x) noexcept;

/// 八分圆内的反正切：0 ≤ minor ≤ major，返回 atan(minor / major)，0..64 对应 0°..45°。
[[nodiscard]] int ArcTanOctant(int major, int minor) noexcept;

/// 整数平方根，向下取整（逐位试商，结果不超过 0xffff）。
[[nodiscard]] int SqrtUint32(uint32_t v) noexcept;

/// 三次卷积插值核（a = −1）：|x| ≤ 1 时 x³ − 2x² + 1，1 < |x| ≤ 2 时 −x³ + 5x² − 8x + 4，其余为 0。
[[nodiscard]] double CubicKernel(double x) noexcept;

}  // namespace Solvers::Tsa::Core::Math
