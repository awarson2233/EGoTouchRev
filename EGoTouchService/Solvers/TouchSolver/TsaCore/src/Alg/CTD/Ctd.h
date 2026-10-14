#pragma once
// 质心（原 CTD 模块）：在 Dif 上求加权质心，以及挨着阵列边缘的触点的质心边缘修正（EC）——
// 边缘外没有传感器，质心会被拉向阵列内侧，按触点尺寸与边缘信号比把坐标往边缘推回去。

#include <cstdint>

namespace Solvers::Tsa::Core::Ctd {

/// 每帧的边缘修正：pos 从 rawPos 出发，修正后同步到 rptPos，并更新到边缘的距离。
void EcProcess();
/// 以 (dim1, dim2) 为中心、dim1Span × dim2Span 窗口内正信号的加权质心（Q8.8，取格中心）。
void General(uint8_t dim1, uint8_t dim2, uint8_t dim1Span, uint8_t dim2Span, uint16_t* centerDim1, uint16_t* centerDim2);
/// sum × 256 / weight + 128（格中心），负数取 0；weight 为 0 时为 0。
[[nodiscard]] int64_t BasicCalculation(int32_t weightedSum, int32_t weight);

}  // namespace Solvers::Tsa::Core::Ctd
