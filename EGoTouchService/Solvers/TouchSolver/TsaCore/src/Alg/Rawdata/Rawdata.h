#pragma once
// 原始数据的归一化（原 Signal/Rawdata.c，原厂 src/Alg/Rawdata/Rawdata.c）：raw 按 FlashConstPrmt 的比例
// field038 / 0x3fff 与偏移 field03C 线性换算；走 Dif 缓冲路径（systemFlags bit16）时只缩放 Dif。

#include <cstdint>

namespace Solvers::Tsa::Core::Rawdata {

[[nodiscard]] bool IsProcessNeeded();
void Process();
/// 在 Dif 域做一次 CMF：由 raw 与基线算出 Dif，CMF 后再还原 raw（硬件复位时跳过）。
void Cmf();
/// 由 Dif 还原基线：bl += Dif（逆缩放、逆归一化后；信号反向时减）。走 Dif 缓冲路径时基线清零。
void RetrieveBaseline(int16_t* bl, const int16_t* dif);
/// 把归一化后的基线换回原始刻度，再交给 TPSensor 逆平移。
void GetUnNormalizedBaseline(int16_t* bl, uint32_t n);

}  // namespace Solvers::Tsa::Core::Rawdata
