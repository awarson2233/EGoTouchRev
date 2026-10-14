#pragma once
// 峰（局部极大值）的检测与峰表维护（原 src/Alg/Peak/PeakData.c 与峰检测流程）。
//   - Peak.cpp：每帧在 Dif 上找峰、计算 Z8 / Zn、过滤弱峰、排序并交给 IDT 做初步编号；
//   - PeakData.cpp：峰表的增删与各模块用到的查询。

#include <cstdint>

#include "Alg/Peak/PeakData.h"

namespace Solvers::Tsa::Core::Peak {

/// 每帧的峰检测流程（TSA 主流程调用）。
void Process();
/// 在 [firstRow, lastRow] × [firstCol, lastCol] 内找不低于门限的局部极大值，逐个插入峰表。
/// 平台上只认最左上的一格：与下方、右方相邻格相等也算峰，与上方、左方相等则不算。
void DetectInRange(int16_t thold, uint8_t firstRow, uint8_t lastRow, uint8_t firstCol, uint8_t lastCol);
/// 峰值升序排列；峰值相同时把后一个峰（以及它在 Dif 里的值）加 1，使峰值两两不同。
void SortInSigAscending();
/// 计算峰的 Z8（八邻域和）、Zn（含中心的 2×2 象限和与十字五点和中的最大值）与邻居数。
void CalcZn(uint8_t idx);
[[nodiscard]] bool IsToExcludeSideRegion();
[[nodiscard]] bool NeedUdfpLargeFingerWorkaround();

void Clean();
/// 插入 (dim1, dim2) 处的峰，返回其下标。峰表已满时替换峰值最小且比新峰弱的一个，都比新峰强则返回 kMaxPeakNum。
uint32_t Insert(uint8_t dim1, uint8_t dim2);
void Fill(uint8_t idx, uint8_t dim1, uint8_t dim2);
void Remove(uint8_t idx);

/// 触点的事件处于按下 / 移动阶段（TouchEvent 不超过 kMove）。
[[nodiscard]] bool IsTouchValid(uint8_t touchIdx);
[[nodiscard]] bool IsOnInnerEdgeDim1Near(uint8_t idx);
[[nodiscard]] bool IsOnInnerEdgeDim1Far(uint8_t idx);
[[nodiscard]] bool IsOnInnerEdgeDim2Near(uint8_t idx);
[[nodiscard]] bool IsOnInnerEdgeDim2Far(uint8_t idx);
[[nodiscard]] bool IsOnInnerEdgeDim1(uint8_t idx);
[[nodiscard]] bool IsOnOuterEdgeDim1Near(uint8_t idx);
[[nodiscard]] bool IsOnOuterEdgeDim1Far(uint8_t idx);
[[nodiscard]] bool IsOnOuterEdgeDim2Near(uint8_t idx);
[[nodiscard]] bool IsOnOuterEdgeDim2Far(uint8_t idx);
[[nodiscard]] bool IsOnOuterEdgeDim1(uint8_t idx);
[[nodiscard]] bool IsOnOuterEdgeDim2(uint8_t idx);
[[nodiscard]] bool IsOnOuterEdge(uint8_t idx);
/// 是否有峰落在以 (dim1, dim2) 为中心、边长 boxSize 的方框内。
[[nodiscard]] bool IsInNxNBox(uint8_t dim1, uint8_t dim2, uint8_t boxSize);

[[nodiscard]] uint8_t GetIdxBySig(int16_t sig);
[[nodiscard]] int GetZ9(uint8_t idx);
[[nodiscard]] uint8_t GetDim1DistToEdge(uint8_t idx);
[[nodiscard]] uint8_t GetDim2DistToEdge(uint8_t idx);
[[nodiscard]] uint8_t GetDistToEdge(uint8_t idx);
[[nodiscard]] uint8_t GetDistance(uint8_t idxA, uint8_t idxB);
[[nodiscard]] int GetSquareDistance(uint8_t idxA, uint8_t idxB);

}  // namespace Solvers::Tsa::Core::Peak
