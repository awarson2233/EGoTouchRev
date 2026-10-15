#pragma once
// 二值图形态学（原厂 TSA_Misc.c 的 Image_* 函数，CMF 用）。图按行存放，dim1Len 列 × dim2Len 行，
// 0 为空、非 0 为有；结果原地写回，判断一律按处理前的图。

#include <cstdint>

namespace Solvers::Tsa::Core::Morphology {

/// 膨胀：空格周围 (2·radius1+1) × (2·radius2+1) 内有非空格时置 1。
void Dilation(int16_t* img, int dim1Len, int dim2Len, int radius1, int radius2);
/// 腐蚀：每个空格把周围 (2·radius1+1) × (2·radius2+1) 清 0。
void Erosion(int16_t* img, int dim1Len, int dim2Len, int radius1, int radius2);
/// 十字腐蚀：alongDim1 时，上下都空的空格把左右两格清 0；alongDim2 时，左右都空的空格把上下两格清 0。
void CrossErosion(int16_t* img, int dim1Len, int dim2Len, bool alongDim1, bool alongDim2);

}  // namespace Solvers::Tsa::Core::Morphology
