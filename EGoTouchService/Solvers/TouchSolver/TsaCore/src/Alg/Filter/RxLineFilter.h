#pragma once
// 扫描线串扰过滤（原 Signal/RxLineFilter.c）。

namespace Solvers::Tsa::Core::RxLineFilter {

/// 噪声等级达到 2 时，去掉同一扫描线上另有明显更强的峰的弱触点。
void Process();

}  // namespace Solvers::Tsa::Core::RxLineFilter
