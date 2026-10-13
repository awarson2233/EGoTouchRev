#include "TSACoreInternal.h"

// ============================================================================
// Function: TSAPrpt_GetDifPreCMFPrpt @ 6baa13be
// ============================================================================

void TSAPrpt_GetDifPreCMFPrpt(void)

{
  short sVar1;
  int iVar2;
  byte local_22;
  byte local_21;
  short *local_20;
  byte local_14;
  byte local_13;
  byte local_12;
  byte local_11;
  short local_10;
  short local_e;
  short local_c;
  short local_a;
  
                    /* 0x613be  1656  TSAPrpt_GetDifPreCMFPrpt */
  local_a = 0;
  local_c = 0;
  local_e = 0;
  local_10 = 0;
  local_11 = 0;
  local_12 = 0;
  local_13 = 0;
  local_14 = 0;
  local_20 = *(short **)_refptr_g_tsaBufDif;
  for (local_21 = 0; local_21 < _refptr_g_tsaPrmtFlash->bRows; local_21 = local_21 + 1) {
    for (local_22 = 0; local_22 < _refptr_g_tsaPrmtFlash->bCols; local_22 = local_22 + 1) {
      sVar1 = *local_20;
      if (sVar1 < DAT_6bc4a910) {
        if (sVar1 <= -DAT_6bc4a910) {
          local_c = local_c + 1;
        }
      }
      else {
        local_a = local_a + 1;
      }
      if (local_e < sVar1) {
        local_11 = local_22;
        local_12 = local_21;
        local_e = sVar1;
      }
      else if (sVar1 < local_10) {
        local_13 = local_22;
        local_14 = local_21;
        local_10 = sVar1;
      }
      local_20 = local_20 + 1;
    }
  }
  g_tsaPrpt._354_2_ = local_a + local_c;
  iVar2 = (int)local_e;
  if ((int)local_e <= -(int)local_10) {
    iVar2 = -(int)local_10;
  }
  g_tsaPrpt._352_2_ = (short)iVar2;
  g_tsaPrpt._356_2_ = local_e;
  g_tsaPrpt._360_1_ = local_11;
  g_tsaPrpt._361_1_ = local_12;
  g_tsaPrpt._358_2_ = local_10;
  g_tsaPrpt._362_1_ = local_13;
  g_tsaPrpt._363_1_ = local_14;
  return;
}


// ============================================================================
// Function: TSAPrpt_Init @ 6baa07c8
// ============================================================================

void TSAPrpt_Init(void)

{
                    /* 0x607c8  1657  TSAPrpt_Init */
  TSA_InitBlockIdx();
  return;
}


// ============================================================================
// Function: TSAPrpt_Free @ 6baa07ad
// ============================================================================

void TSAPrpt_Free(void)

{
                    /* 0x607ad  1655  TSAPrpt_Free */
  TSA_Free(&g_tsaBlockIdx);
  return;
}


// ============================================================================
// Function: TSAPrpt_Malloc @ 6baa076c
// ============================================================================

bool TSAPrpt_Malloc(void)

{
                    /* 0x6076c  1658  TSAPrpt_Malloc */
  g_tsaBlockIdx = TSA_MallocAndInit(*(undefined2 *)_refptr_g_gridBufSize);
  return g_tsaBlockIdx != 0;
}


