#pragma once
// 机型参数表的装入（Prmt/ProjectTables.cpp）。

namespace Solvers::Tsa::Core::Prmt {

/// 把 W273AS2700 的 FlashPrmt、CMF PCA、ASA 参数表写回镜像里的原位置。
void LoadProjectTables();

}  // namespace Solvers::Tsa::Core::Prmt
