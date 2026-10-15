#pragma once
// 每帧的全局属性统计（原 src/App/TSA_Prpt.c 与 TSA_MSProc.c 中求属性的部分）：raw 的极值与内圈平均，
// 以及 Dif 按区块（内部、四边、角）统计后组合成几组 DifPrpt，结果在 TsaPrpt。

#include <cstdint>

namespace Solvers::Tsa::Core::Property {

[[nodiscard]] bool Malloc();
void Free();
/// 给每格标上所属区块。
void Init();
/// raw / baseline 的属性（不走 Dif 缓冲路径时），再求 Dif 的属性。
void Get(const uint16_t* raw, const uint16_t* bl);
/// CMF 之前 Dif 的极值与超门限格数。
void GetDifPreCmf();
/// 负向极值：featureFlags 0x8000 时取内部区块的。
[[nodiscard]] int16_t MinSig();
/// CMF 公共噪声上下限的最大绝对值。
[[nodiscard]] int MaxCmfNoise();

}  // namespace Solvers::Tsa::Core::Property
