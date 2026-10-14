#pragma once
// 原代码的 TSA_ASSERT：条件不成立时打印断言并记一次软件异常（HardwareAnalyzer 的异常码，
// 会经 TSA_SetDMDType 报给宿主）。打印在本移植里裁掉了，只保留异常码。

extern "C" void HardwareAnalyzer_SetSoftException(void);

namespace Solvers::Tsa::Core {

inline void TsaAssert(bool condition) noexcept {
    if (!condition) HardwareAnalyzer_SetSoftException();
}

}  // namespace Solvers::Tsa::Core
