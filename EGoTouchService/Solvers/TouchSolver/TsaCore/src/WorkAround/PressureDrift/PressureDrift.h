#pragma once
// 按压漂移（原 Baseline/PressureDrift.c）：屏幕被整行按压时，一行上的 Dif 普遍抬高而平缓，
// 峰检测据此把这类峰当作漂移而不是触点（TsaStatic::featureFlags2 bit5 开启时）。

#include <cstdint>

namespace Solvers::Tsa::Core::PressureDrift {

/// 峰 (dim1, dim2) 所在行判为按压漂移。
[[nodiscard]] bool Detect(uint8_t dim1, uint8_t dim2);

}  // namespace Solvers::Tsa::Core::PressureDrift
