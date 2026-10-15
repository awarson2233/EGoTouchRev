#pragma once
// 共模噪声滤波（原 Signal/CMF.c，原厂 src/Alg/CMF/CMF.c）：沿每条扫描线把 Dif 分段聚类，取最可信的一簇的均值作为
// 该线的公共噪声，从整线减去，并记入 commonModeNoise。2D（PCA）滤波只在 cmfMethod 为 1 时启用，本机型的
// 参数下不会出现，已裁掉。

#include <cstdint>

namespace Solvers::Tsa::Core::Cmf {

[[nodiscard]] bool Malloc();
void Free();
void Init();
void Process();
/// 1 为 2D（PCA）滤波；智能皮套且 cmfMethodSmartCover 不为 2 时取皮套的设置。
[[nodiscard]] uint32_t GetMethod();
/// 本帧该格减去的公共噪声，越界为 0。
[[nodiscard]] int16_t GetNoiseValue(uint8_t dim1, uint8_t dim2);
/// 对任意缓冲按扫描线做一遍 CMF（dim 1 按行、2 按列），不记录噪声。
void ProcessDimCommonOnBuf(int16_t* dst, const int16_t* src, int dim, int16_t thold, int16_t grad, uint8_t cols,
                           uint8_t rows);

}  // namespace Solvers::Tsa::Core::Cmf
