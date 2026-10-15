#pragma once
// 机型参数表的装入（Prmt/ProjectTables.cpp）。

namespace Solvers::Tsa::Core::Prmt {

/// 按机型名把 FlashPrmt、CMF PCA、ASA 参数表装入 .bss 里各自的槽，返回实际装入的机型名。
/// 带的机型：W273AS2700（GaokunHimaxCSOT）、W273AS1300（GaokunHimaxBOE）、W273AS1310
/// （GaokunHimaxBOE_NewMapping）；其余名字与 TSAPrmt 一样落到缺省的 DFLT000000。
const char* LoadProjectTables(const char* name);

}  // namespace Solvers::Tsa::Core::Prmt
