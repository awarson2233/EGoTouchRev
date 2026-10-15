#pragma once
// 上一帧峰表（原 src/Alg/Peak/PrevPeakData.c）：帧末从当前峰表拷贝，供下一帧比较。

#include <cstdint>

#include "Alg/Peak/PeakData.h"

namespace Solvers::Tsa::Core::PrevPeak {

void Init();
/// 帧末：当前峰表的个数、最大峰值与每个峰的标志、Zn、峰值、位置拷进上一帧峰表。
void Process();
[[nodiscard]] bool IsOnOuterEdgeDim1Near(uint8_t idx);
[[nodiscard]] bool IsOnOuterEdgeDim1Far(uint8_t idx);
[[nodiscard]] bool IsOnOuterEdgeDim2Near(uint8_t idx);
[[nodiscard]] bool IsOnOuterEdgeDim2Far(uint8_t idx);
[[nodiscard]] bool IsOnOuterEdgeDim1(uint8_t idx);
[[nodiscard]] bool IsOnOuterEdgeDim2(uint8_t idx);
[[nodiscard]] bool IsOnOuterEdge(uint8_t idx);

}  // namespace Solvers::Tsa::Core::PrevPeak
