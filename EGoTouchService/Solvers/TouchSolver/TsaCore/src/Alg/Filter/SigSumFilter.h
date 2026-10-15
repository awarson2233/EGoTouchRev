#pragma once
// 按信号量保留触点（原 Signal/SigSumFilter.c）。

namespace Solvers::Tsa::Core::SigSumFilter {

/// 只保留 sigSumAll 最大的 keep 个触点；keep 为 0 时不处理。
void ReserveTouch(int keep);

}  // namespace Solvers::Tsa::Core::SigSumFilter
