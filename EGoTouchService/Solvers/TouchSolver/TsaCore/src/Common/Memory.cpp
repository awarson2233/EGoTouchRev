#include "Common/Memory.h"

#include <array>
#include <cstdlib>

#include "../TsaCoreApi.h"
#include "Common/Image.h"

namespace Solvers::Tsa::Core::Mem {

namespace {

constexpr uint32_t kMaxAllocSize = 0xffff;

// 回放比对用的分配登记（原 DLL 没有这一层）：按分配次序记下仍存活的缓冲。登记满了以后再分配的
// 缓冲记不下，那时的快照不完整。
struct HeapBlock {
    void* ptr;
    uint32_t size;
};
constexpr size_t kMaxHeapBlocks = 512;
std::array<HeapBlock, kMaxHeapBlocks> g_blocks{};
size_t g_blockCount = 0;
bool g_overflowed = false;

void Register(void* p, uint32_t size) {
    if (g_blockCount == kMaxHeapBlocks) {
        g_overflowed = true;
        return;
    }
    g_blocks[g_blockCount++] = {p, size};
}

void Unregister(void* p) {
    for (size_t i = 0; i < g_blockCount; ++i) {
        if (g_blocks[i].ptr != p) continue;
        for (size_t j = i + 1; j < g_blockCount; ++j) g_blocks[j - 1] = g_blocks[j];
        --g_blockCount;
        return;
    }
}

}  // namespace

// TSA_MallocAndInit @ 0x6baa3430
void* Allocate(uint32_t size) {
    if (size == 0 || kMaxAllocSize < size) return nullptr;
    void* p = std::calloc(1, size);
    if (p != nullptr) Register(p, size);
    return p;
}

// TSA_Free @ 0x6baa34c4
void Free(void* p) {
    if (p == nullptr) return;
    Unregister(p);
    std::free(p);
}

}  // namespace Solvers::Tsa::Core::Mem

// raw 的调用方
extern "C" {

// 宿主（回放）：镜像在前，堆缓冲按分配次序。
size_t TSACoreRaw_StateBlocks(TSACoreRaw_StateBlock* out, size_t capacity) {
    namespace M = Solvers::Tsa::Core::Mem;
    namespace I = Solvers::Tsa::Core::Image;
    if (M::g_overflowed) return static_cast<size_t>(-1);
    const size_t count = M::g_blockCount + 1;
    if (out == nullptr || capacity < count) return count;
    out[0] = {g_tsaMemoryImage, I::kSize, I::kBaseVa};
    for (size_t i = 0; i < M::g_blockCount; ++i) {
        out[i + 1] = {static_cast<const uint8_t*>(M::g_blocks[i].ptr), M::g_blocks[i].size, 0};
    }
    return count;
}
}  // extern "C"
