#include "TSACoreInternal.h"

// 算法状态的全貌，给回放比对用：镜像（按原 VA 寻址）加上 TSA_MallocAndInit 分出去的缓冲。
// 原 DLL 没有这一层；这里不参与任何算法，只登记缓冲并把两类内存报给宿主。

#define kTsaStateHeapMax 512

typedef struct TSACoreRaw_StateBlock {
  const uint8_t *data;
  uint64_t size;
  uint64_t va;    // 镜像块是原 VA；堆缓冲填 0，由调用方按「哪个镜像槽位指向它」定名
} TSACoreRaw_StateBlock;

static struct {
  void *ptr;
  uint32_t size;
} s_heapBlocks[kTsaStateHeapMax];
static uint32_t s_heapBlockCount;
static uint32_t s_heapBlockDropped;

void TSAState_OnAlloc(void *ptr,uint32_t size)
{
  if (s_heapBlockCount == kTsaStateHeapMax) {
    s_heapBlockDropped = s_heapBlockDropped + 1;
    return;
  }
  s_heapBlocks[s_heapBlockCount].ptr = ptr;
  s_heapBlocks[s_heapBlockCount].size = size;
  s_heapBlockCount = s_heapBlockCount + 1;
}

void TSAState_OnFree(void *ptr)
{
  uint32_t i;
  uint32_t j;

  for (i = 0; i < s_heapBlockCount; i = i + 1) {
    if (s_heapBlocks[i].ptr == ptr) {
      // 保持分配次序
      for (j = i + 1; j < s_heapBlockCount; j = j + 1) {
        s_heapBlocks[j - 1] = s_heapBlocks[j];
      }
      s_heapBlockCount = s_heapBlockCount - 1;
      return;
    }
  }
}

// 把全部状态块写进 out（镜像在前，堆缓冲按分配次序），返回块数；capacity 不够时只返回需要的块数。
// 登记表溢出时返回 (size_t)-1：那样的快照不完整，不能拿来比。
size_t TSACoreRaw_StateBlocks(TSACoreRaw_StateBlock *out,size_t capacity)
{
  size_t count;
  uint32_t i;

  if (s_heapBlockDropped != 0) {
    return (size_t)-1;
  }
  count = (size_t)s_heapBlockCount + 1;
  if (out == NULL || capacity < count) {
    return count;
  }
  out[0].data = g_tsaMemoryImage;
  out[0].size = kTsaMemoryImageSize;
  out[0].va = kTsaMemoryImageBaseVA;
  for (i = 0; i < s_heapBlockCount; i = i + 1) {
    out[i + 1].data = (const uint8_t *)s_heapBlocks[i].ptr;
    out[i + 1].size = s_heapBlocks[i].size;
    out[i + 1].va = 0;
  }
  return count;
}
