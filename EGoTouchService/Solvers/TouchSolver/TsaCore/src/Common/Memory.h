#pragma once
// 模块缓冲的分配（原 TSA_MallocAndInit / TSA_Free）：清零分配，并登记到 TSAState 以便宿主转储。

#include <cstdint>

namespace Solvers::Tsa::Core::Mem {

/// 清零分配 size 字节；0 或超过 64 KB 时返回空。
[[nodiscard]] void* Allocate(uint32_t size);
/// 释放并登记；p 为空时什么也不做。
void Free(void* p);

/// 释放后把指针置空。
template <class T>
void Release(T*& p) {
    Free(p);
    p = nullptr;
}

}  // namespace Solvers::Tsa::Core::Mem
