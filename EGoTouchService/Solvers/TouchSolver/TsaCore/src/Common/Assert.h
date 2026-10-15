#pragma once
// 原代码的 TSA_ASSERT：条件不成立时打印断言并记一次软件异常（HardwareAnalyzer 的异常码，
// 会经 TSA_SetDMDType 报给宿主）。打印在本移植里裁掉了，只保留异常码。

#include <cstddef>
#include <cstring>

namespace Solvers::Tsa::Core {

namespace HardwareAnalyzer {
void SetSoftException();
}  // namespace HardwareAnalyzer

inline void TsaAssert(bool condition) noexcept {
    if (!condition) HardwareAnalyzer::SetSoftException();
}

/// 原代码的 memset_s：长度超过容量时不写并记一次断言。
inline void ClearChecked(void* p, size_t capacity, size_t n) noexcept {
    TsaAssert(n <= capacity);
    if (n <= capacity) std::memset(p, 0, n);
}

/// 原代码的 memcpy_s：长度超过容量时不拷并记一次断言。
inline void CopyChecked(void* dst, size_t capacity, const void* src, size_t n) noexcept {
    TsaAssert(n <= capacity);
    if (n <= capacity) std::memcpy(dst, src, n);
}

}  // namespace Solvers::Tsa::Core
