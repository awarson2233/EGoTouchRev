#include "Common/Memory.h"

#include <cstdlib>

#include "Legacy/RawApi.h"

namespace Solvers::Tsa::Core::Mem {

namespace {
constexpr uint32_t kMaxAllocSize = 0xffff;
}

// TSA_MallocAndInit @ 0x6baa3430
void* Allocate(uint32_t size) {
    if (size == 0 || kMaxAllocSize < size) return nullptr;
    void* p = std::calloc(1, size);
    if (p != nullptr) TSAState_OnAlloc(p, size);
    return p;
}

// TSA_Free @ 0x6baa34c4
void Free(void* p) {
    if (p == nullptr) return;
    TSAState_OnFree(p);
    std::free(p);
}

}  // namespace Solvers::Tsa::Core::Mem

// raw 的调用方
extern "C" {
void* TSA_MallocAndInit(uint32_t size) { return Solvers::Tsa::Core::Mem::Allocate(size); }
void TSA_Free(int64_t* slot) { Solvers::Tsa::Core::Mem::Release(*reinterpret_cast<void**>(slot)); }
}
