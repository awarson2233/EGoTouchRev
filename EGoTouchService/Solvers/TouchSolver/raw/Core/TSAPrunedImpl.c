#include "TSACoreInternal.h"

// 被剪枝模块中仍需写镜像全局的少数函数。TSAPrunedStubs.h 先于 TSABssMemoryImage.h
// 被包含，static inline 桩里用不了镜像宏，所以放在这里。

// ============================================================================
// Function: CMFRemap_Init @ 6ba503b6
// ============================================================================

// W273AS2700：flash[0x6cc]=1、flash[0x6cd]=0、flash[0x80e]=0 → CMFRemap_IsNeedRemap()
// 恒为 0，Enter/Exit/MapBuffer 全被门控跳过。这里只保留不需要 remap 时原函数留下的
// 全局状态；映射表与索引表的生成（CMFRemap_Mapping / GenerateIndexTable）随模块剪掉。
void CMFRemap_Init(void)

{
  byte bVar1;
  byte bVar2;

  g_cmfRemapNumRxCombine = (uint)*((uint8_t *)_refptr_g_tsaPrmtFlash + 0x6cc);
  bVar2 = _refptr_g_tsaPrmtFlash->bRows;
  bVar1 = _refptr_g_tsaPrmtFlash->bCols;
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass != '\0') {
    bVar2 = _refptr_g_tsaPrmtFlash->bCols;
    bVar1 = _refptr_g_tsaPrmtFlash->bRows;
  }
  g_cmfRemapTxNum = (ushort)bVar1;
  g_cmfRemapRxNum = (ushort)bVar2;
  g_cmfRemapMapRxNum = g_cmfRemapRxNum * (short)g_cmfRemapNumRxCombine;
  g_cmfRemapMapTxNum = g_cmfRemapTxNum;
  g_cmfRemapIsReshape = 1;
  return;
}

// ============================================================================
// Function: SideTouch_Malloc @ 6bb06220
// ============================================================================

bool SideTouch_Malloc(void)

{
                    /* 0xc6220  1391  SideTouch_Malloc */
  *(void **)_refptr_g_tsaBufTzSide = TSA_MallocAndInit(g_gridBufSize);
  return *(void **)_refptr_g_tsaBufTzSide != (void *)0x0;
}
