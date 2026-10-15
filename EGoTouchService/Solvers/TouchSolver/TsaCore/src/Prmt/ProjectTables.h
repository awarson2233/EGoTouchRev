#pragma once
// 机型参数表的装入（Prmt/ProjectTables.cpp）。

namespace Solvers::Tsa::Core::Prmt {

/// 把 W273AS2700 的 FlashPrmt、CMF PCA、ASA 参数表装入 .bss 里各自的槽。
void LoadProjectTables();

}  // namespace Solvers::Tsa::Core::Prmt
