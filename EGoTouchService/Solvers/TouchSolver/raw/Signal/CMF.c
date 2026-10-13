#include "TSACoreInternal.h"

// ============================================================================
// Function: CMF_Process @ 6ba4f2c8
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* Current config CMF path: bCmfEnabled=0 fallback enters CMF_Process; method=0,
   bCmfForceDim1Pass=1, so it uses CMF_ProcessDim(1) rather than CMF_Filter2D. */

void CMF_Process(void)

{
  char cIsGameScenario;
  int nCmfMethod;
  undefined8 in_stack_ffffffffffffffd8;
  byte bCmfRows;
  byte bCmfCols;
  errno_t copyStatus;
  undefined4 dwRowsUpper;
  undefined8 qwPackedCmfRows;
  
                    /* 0xf2c8  478  CMF_Process */
  dwRowsUpper = (undefined4)((ulonglong)in_stack_ffffffffffffffd8 >> 0x20);
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&DAT_6bb51b3b,"CMF_Process");
  }
  bCmfCols = _refptr_g_tsaPrmtFlash->bCols;
  bCmfRows = _refptr_g_tsaPrmtFlash->bRows;
  copyStatus = memcpy_s(*(void **)_refptr_g_tsaBufPreCMFDif,
                        (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,
                        *(void **)_refptr_g_tsaBufDif,
                        (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  if (copyStatus != 0) {
    dwRowsUpper = 0;
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/CMF/CMF.c",0x25b,
                      "rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  CMF_SaveOrderDiff();
  CMF_SavePreCmfDif();
  if ((*_refptr_g_tsaStaticPtr)->bCmfProcessingBlocked == '\0') {
    nCmfMethod = CMF_GetCmfMethod();
    if (nCmfMethod == 1) {
      CMF_Filter2D((int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x3c),
                   (int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x3e));
    }
    else {
      CMFRemap_Enter(&bCmfCols,&bCmfRows);
      copyStatus = memset_s(*(undefined8 *)_refptr_g_tsaBufCommonModeNoise,
                            (ulonglong)*(ushort *)_refptr_g_gridMapBufSize * 2,0,
                            (longlong)(int)((uint)bCmfRows * (uint)bCmfCols) * 2);
      if (copyStatus != 0) {
        dwRowsUpper = 0;
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/CMF/CMF.c",
                          0x26f,"rc == EOK");
        HardwareAnalyzer_SetSoftException();
      }
      if ((_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') &&
         (nCmfMethod = CMFRemap_IsNeedRemap(), nCmfMethod == 0)) {
        qwPackedCmfRows = CONCAT44(dwRowsUpper,(uint)bCmfRows);
        CMF_ProcessDim(2,(int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x3c),
                       (int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x3e),bCmfCols,qwPackedCmfRows);
        dwRowsUpper = (undefined4)((ulonglong)qwPackedCmfRows >> 0x20);
      }
      else {
        qwPackedCmfRows = CONCAT44(dwRowsUpper,(uint)bCmfRows);
        CMF_ProcessDim(1,(int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x3c),
                       (int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x3e),bCmfCols,qwPackedCmfRows);
        dwRowsUpper = (undefined4)((ulonglong)qwPackedCmfRows >> 0x20);
      }
      if ((*_refptr_g_tsaStaticPtr)[1].field_0x2 == '\0') {
        Baseline_Compensation();
      }
      CMFRemap_Exit();
      if ((_refptr_g_tsaPrmtDynamic[0x40] != '\0') &&
         (cIsGameScenario = TSA_IsGameScenario(), cIsGameScenario != '\0')) {
        CMF_ProcessDimCommonNoise
                  (1,0x4b0,200,_refptr_g_tsaPrmtFlash->bCols,
                   CONCAT44(dwRowsUpper,(uint)_refptr_g_tsaPrmtFlash->bRows));
      }
      copyStatus = memcpy_s(_refptr_g_tsaBufAftDim1CMFDif,0x2648,*(void **)_refptr_g_tsaBufDif,
                            (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
      if (copyStatus != 0) {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/CMF/CMF.c",
                          0x284,"rc == EOK");
        HardwareAnalyzer_SetSoftException();
      }
    }
    copyStatus = memcpy_s(_refptr_g_tsaBufAftCMFDif,0x2648,*(void **)_refptr_g_tsaBufDif,
                          (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
    if (copyStatus != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/CMF/CMF.c",0x289
                        ,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
    return;
  }
  return;
}


// ============================================================================
// Function: CMF_Init @ 6ba4d8c1
// ============================================================================

void CMF_Init(void)

{
                    /* 0xd8c1  474  CMF_Init */
  CMFRemap_Init();
  if ((_refptr_g_tsaPrmtFlash->dwCmfMethodNormal == 1) ||
     (_refptr_g_tsaPrmtFlash->dwCmfMethodSmartCover == 1)) {
    CMF_Init2DFilter();
  }
  return;
}


// ============================================================================
// Function: CMF_GetCmfMethod @ 6ba4d903
// ============================================================================

uint CMF_GetCmfMethod(void)

{
  uint uVar1;
  
                    /* 0xd903  471  CMF_GetCmfMethod */
  if ((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) == 0) ||
     (_refptr_g_tsaPrmtFlash->dwCmfMethodSmartCover == 2)) {
    uVar1 = _refptr_g_tsaPrmtFlash->dwCmfMethodNormal;
  }
  else {
    uVar1 = _refptr_g_tsaPrmtFlash->dwCmfMethodSmartCover;
  }
  return uVar1;
}


// ============================================================================
// Function: CMF_ProcessDim @ 6ba4e8b5
// ============================================================================

void CMF_ProcessDim(int param_1,short param_2,short param_3,byte param_4,byte param_5)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x25]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x30];
  char cVar1;
  
                    /* 0xe8b5  481  CMF_ProcessDim */
  *(undefined2 *)&_refptr_g_tsaPrpt[1].field_0x44 = 0x7fff;
  *(undefined2 *)&_refptr_g_tsaPrpt[1].field_0x46 = 0x8000;
  (*(byte *)(_stk + 0x26)) = param_4;
  if (param_1 == 1) {
    (*(byte *)(_stk + 0x26)) = param_5;
  }
  (*(uint *)(_stk + 0x20)) = 0;
  if (param_1 == 1) {
    (*(uint *)(_stk + 0x20)) = (uint)param_4;
  }
  (*(undefined8 *)(_stk + 0x18)) = 0;
  cVar1 = SS_IsValidForCMF(param_1);
  if (cVar1 != '\0') {
    (*(undefined8 *)(_stk + 0x18)) = SS_DiffRelatCal((*(byte *)(_stk + 0x26)),(*(uint *)(_stk + 0x20)));
  }
  (*(int *)(_stk + 0x14)) = 0;
  cVar1 = TouchThold_ToeUseSpecialThold();
  if (cVar1 != '\0') {
    (*(int *)(_stk + 0x14)) = 1;
  }
  for ((*(byte *)(_stk + 0x27)) = 0; (*(byte *)(_stk + 0x27)) < (*(byte *)(_stk + 0x26)); (*(byte *)(_stk + 0x27)) = (*(byte *)(_stk + 0x27)) + 1) {
    (*(undefined4 *)(_stk + 0x10)) = 0;
    cVar1 = SS_IsValidForCMF(param_1);
    if (cVar1 != '\0') {
      (*(undefined4 *)(_stk + 0x10)) = SS_DiffCMFProcessDimUnit(param_1,param_4,param_5,(*(byte *)(_stk + 0x27)),(*(undefined8 *)(_stk + 0x18)));
    }
    if (((*(int *)(_stk + 0x14)) == 0) || ((1 < (*(byte *)(_stk + 0x27)) && ((int)(uint)(*(byte *)(_stk + 0x27)) < (int)((*(byte *)(_stk + 0x26)) - 2))))) {
      (*(short *)(_stk + 0xe)) = param_2;
    }
    else {
      (*(short *)(_stk + 0xe)) = 200;
    }
    (*(short *)(_stk + 0xc)) = CMF_ProcessDimUnit(param_1,(*(byte *)(_stk + 0x27)),(int)(*(short *)(_stk + 0xe)),(int)param_3,&(*(undefined1 *)(_stk + 0xb)),param_4,
                                  param_5);
    (*(short *)(_stk + 0xc)) = (*(short *)(_stk + 0xc)) + (short)(*(undefined4 *)(_stk + 0x10));
    CMF_CopyCommonModeNoise(param_1,(*(byte *)(_stk + 0x27)),param_4,param_5,(int)(*(short *)(_stk + 0xc)));
    if ((*(short *)(_stk + 0xc)) < *(short *)&_refptr_g_tsaPrpt[1].field_0x44) {
      *(short *)&_refptr_g_tsaPrpt[1].field_0x44 = (*(short *)(_stk + 0xc));
    }
    if (*(short *)&_refptr_g_tsaPrpt[1].field_0x46 < (*(short *)(_stk + 0xc))) {
      *(short *)&_refptr_g_tsaPrpt[1].field_0x46 = (*(short *)(_stk + 0xc));
    }
  }
  return;
}


// ============================================================================
// Function: CMF_ProcessDimCommonNoise @ 6ba4ebf3
// ============================================================================

void CMF_ProcessDimCommonNoise(int param_1,short param_2,short param_3,byte param_4,byte param_5)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x11]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x20];
  short sVar1;
  
                    /* 0xebf3  483  CMF_ProcessDimCommonNoise */
  (*(byte *)(_stk + 0x15)) = 0;
  (*(byte *)(_stk + 0x16)) = param_4;
  if (param_1 == 1) {
    (*(byte *)(_stk + 0x15)) = param_4;
    (*(byte *)(_stk + 0x16)) = param_5;
  }
  for ((*(byte *)(_stk + 0x17)) = 0; (*(byte *)(_stk + 0x17)) < (*(byte *)(_stk + 0x16)); (*(byte *)(_stk + 0x17)) = (*(byte *)(_stk + 0x17)) + 1) {
    sVar1 = CMF_ProcessDimUnit(param_1,(*(byte *)(_stk + 0x17)),(int)param_2,(int)param_3,&(*(undefined1 *)(_stk + 0xf)),param_4,param_5);
    (*(int *)(_stk + 0x10)) = (int)sVar1;
    *(short *)(_refptr_g_tsaBufSelf + ((longlong)(int)((uint)(*(byte *)(_stk + 0x15)) + (uint)(*(byte *)(_stk + 0x17))) + 0xb40) * 2)
         = sVar1;
  }
  return;
}


// ============================================================================
// Function: CMF_SaveOrderDiff @ 6ba4ecaa
// ============================================================================

void CMF_SaveOrderDiff(void)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  byte local_c;
  byte local_b;
  byte local_a;
  byte local_9;
  
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    local_b = _refptr_g_tsaPrmtFlash->bRows;
    local_c = _refptr_g_tsaPrmtFlash->bCols;
  }
  else {
    local_b = _refptr_g_tsaPrmtFlash->bCols;
    local_c = _refptr_g_tsaPrmtFlash->bRows;
  }
  for (local_a = 0; local_a < local_c; local_a = local_a + 1) {
    if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\x01') {
      uVar1 = (uint)_refptr_g_tsaPrmtFlash->bCols * (uint)local_a;
    }
    else {
      uVar1 = (uint)local_a;
    }
    *(undefined2 *)(_refptr_g_tsaBufOneOrderDiffRx + (longlong)(int)uVar1 * 2) = 0;
    for (local_9 = 1; local_9 < local_b; local_9 = local_9 + 1) {
      if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\x01') {
        uVar1 = (uint)local_9 + (uint)local_a * (uint)_refptr_g_tsaPrmtFlash->bCols;
      }
      else {
        uVar1 = (uint)local_a + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols;
      }
      if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\x01') {
        uVar2 = (uint)local_9 + (uint)local_a * (uint)_refptr_g_tsaPrmtFlash->bCols;
      }
      else {
        uVar2 = (uint)local_a + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols;
      }
      if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\x01') {
        uVar3 = ((uint)local_9 + (uint)local_a * (uint)_refptr_g_tsaPrmtFlash->bCols) - 1;
      }
      else {
        uVar3 = (uint)local_a + (local_9 - 1) * (uint)_refptr_g_tsaPrmtFlash->bCols;
      }
      *(short *)(_refptr_g_tsaBufOneOrderDiffRx + (longlong)(int)(uVar1 & 0xffff) * 2) =
           *(short *)((ulonglong)(uVar2 & 0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif) -
           *(short *)((ulonglong)(uVar3 & 0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif);
    }
  }
  for (local_a = 0; local_a < local_c; local_a = local_a + 1) {
    if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\x01') {
      uVar1 = (uint)local_a;
    }
    else {
      uVar1 = (uint)_refptr_g_tsaPrmtFlash->bCols * (uint)local_a;
    }
    *(undefined2 *)(_refptr_g_tsaBufTwoOrderDiffRx + (longlong)(int)uVar1 * 2) = 0;
    if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\x01') {
      uVar1 = (uint)local_a + (local_b - 1) * (uint)_refptr_g_tsaPrmtFlash->bCols;
    }
    else {
      uVar1 = ((uint)local_b + (uint)local_a * (uint)_refptr_g_tsaPrmtFlash->bCols) - 1;
    }
    *(undefined2 *)(_refptr_g_tsaBufTwoOrderDiffRx + (longlong)(int)(uVar1 & 0xffff) * 2) = 0;
    for (local_9 = 1; (int)(uint)local_9 < (int)(local_b - 1); local_9 = local_9 + 1) {
      if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\x01') {
        uVar1 = (uint)local_9 + (uint)local_a * (uint)_refptr_g_tsaPrmtFlash->bCols;
      }
      else {
        uVar1 = (uint)local_a + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols;
      }
      if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\x01') {
        uVar2 = (uint)local_9 + (uint)local_a * (uint)_refptr_g_tsaPrmtFlash->bCols + 1;
      }
      else {
        uVar2 = (uint)local_a + (local_9 + 1) * (uint)_refptr_g_tsaPrmtFlash->bCols;
      }
      if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\x01') {
        uVar3 = (uint)local_9 + (uint)local_a * (uint)_refptr_g_tsaPrmtFlash->bCols;
      }
      else {
        uVar3 = (uint)local_a + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols;
      }
      *(short *)(_refptr_g_tsaBufTwoOrderDiffRx + (longlong)(int)(uVar1 & 0xffff) * 2) =
           *(short *)(_refptr_g_tsaBufOneOrderDiffRx + (longlong)(int)(uVar2 & 0xffff) * 2) -
           *(short *)(_refptr_g_tsaBufOneOrderDiffRx + (longlong)(int)(uVar3 & 0xffff) * 2);
    }
  }
  return;
}


// ============================================================================
// Function: CMF_SavePreCmfDif @ 6ba4f0c9
// ============================================================================

/* WARNING: Function: ___chkstk_ms replaced with injection: alloca_probe */

void CMF_SavePreCmfDif(void)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x4d38]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x4d40];
  longlong lVar1;
  longlong *plVar2;
  
  (*(undefined8 *)(_stk + 0x4d28)) = 0x6ba4f0d5;
  if (g_preCMFDiffLogEnabled != '\0') {
    plVar2 = ((longlong *)(_stk + 0x4ca8));
    for (lVar1 = 0xc; lVar1 != 0; lVar1 = lVar1 + -1) {
      *plVar2 = 0;
      plVar2 = plVar2 + 1;
    }
    (*(ushort *)(_stk + 0x4cd0)) = (ushort)_refptr_g_tsaPrmtFlash->bCols;
    (*(ushort *)(_stk + 0x4cd2)) = (ushort)_refptr_g_tsaPrmtFlash->bRows;
    (*(undefined1 * *)(_stk + 0x4ce0)) = ((undefined1 *)(_stk + 0x2658));
    (*(undefined1 * *)(_stk + 0x4cd8)) = ((undefined1 *)(_stk + 0x8));
    ((longlong *)(_stk + 0x4ca8))[2] = *(longlong *)_refptr_g_tsaBufDif;
    CMFRemap_InitMapIdxBySlotTable
              (&_refptr_g_tsaPrmtFlash->field_0x67c,_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass,
               _refptr_g_tsaPrmtFlash->field_0x6cc,((longlong *)(_stk + 0x4ca8)));
    (*(FILE * *)(_stk + 0x4d18)) = fopen("preCMFDiff.csv","a");
    for ((*(int *)(_stk + 0x4d24)) = 0; (*(int *)(_stk + 0x4d24)) < (int)(uint)(*(ushort *)(_stk + 0x4cd6)); (*(int *)(_stk + 0x4d24)) = (*(int *)(_stk + 0x4d24)) + 1) {
      for ((*(int *)(_stk + 0x4d20)) = 0; (*(int *)(_stk + 0x4d20)) < (int)(uint)(*(ushort *)(_stk + 0x4cd4)); (*(int *)(_stk + 0x4d20)) = (*(int *)(_stk + 0x4d20)) + 1) {
        if ((*(int *)(_stk + 0x4d20)) != 0) {
          fputc(0x2c,(*(FILE * *)(_stk + 0x4d18)));
        }
        (*(ushort *)(_stk + 0x4d16)) = (*(ushort *)(_stk + 0x4cd4)) * (short)(*(int *)(_stk + 0x4d24)) + (short)(*(int *)(_stk + 0x4d20));
        (*(ushort *)(_stk + 0x4d14)) = *(ushort *)((*(undefined1 * *)(_stk + 0x4ce0)) + (ulonglong)(*(ushort *)(_stk + 0x4d16)) * 2);
        fprintf((*(FILE * *)(_stk + 0x4d18)),"%d",
                (ulonglong)(uint)(int)*(short *)(((longlong *)(_stk + 0x4ca8))[2] + (ulonglong)(*(ushort *)(_stk + 0x4d14)) * 2));
      }
      fputc(10,(*(FILE * *)(_stk + 0x4d18)));
    }
    fclose((*(FILE * *)(_stk + 0x4d18)));
  }
  return;
}


// ============================================================================
// Function: CMF_Filter2D @ 6ba54009
// ============================================================================

void CMF_Filter2D(int param_1, int param_2)
{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x68]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x70];
  (void)param_1; (void)param_2;
  longlong lVar1;
  undefined **ppuVar2;
  
                    /* 0x14009  467  CMF_Filter2D */
  ppuVar2 = ((undefined * *)(_stk + 0x18));
  for (lVar1 = 6; lVar1 != 0; lVar1 = lVar1 + -1) {
    *ppuVar2 = (undefined *)0x0;
    ppuVar2 = ppuVar2 + 1;
  }
  (*(undefined2 *)(_stk + 0x56)) = *(undefined2 *)(_refptr_g_tsaPrmtFlashCmfPca + 0xce8);
  ((undefined * *)(_stk + 0x18))[1] = _refptr_g_tsaBufCmfDraftNeighborDistance;
  ((undefined * *)(_stk + 0x18))[0] = _refptr_g_tsaBufCmfDraftWeight;
  ((undefined * *)(_stk + 0x18))[3] = _refptr_g_tsaBufCmfNeighborDistance;
  ((undefined * *)(_stk + 0x18))[2] = _refptr_g_tsaBufCmfWeight;
  (*(undefined * *)(_stk + 0x38)) = _refptr_g_tsaBufAftPcaFilterDif;
  (*(undefined8 *)(_stk + 0x40)) = *(undefined8 *)_refptr_g_tsaBufCommonModeNoise;
  (*(undefined8 *)(_stk + 0x8)) = *(undefined8 *)_refptr_g_tsaBufDif;
  (*(undefined8 *)(_stk + 0x10)) = *(undefined8 *)_refptr_g_tsaBufDif;
  CMF_Filter2DCommonInternal(&(*(undefined8 *)(_stk + 0x8)),(*(undefined2 *)(_stk + 0x56)),((undefined * *)(_stk + 0x18)),2);
  return;
}


// ============================================================================
// Function: CMF_Init2DFilter @ 6ba51699
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void CMF_Init2DFilter(void)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0xa8]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0xb0];
  byte bVar1;
  
                    /* 0x11699  475  CMF_Init2DFilter */
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    bVar1 = _refptr_g_tsaPrmtFlash->bRows;
  }
  else {
    bVar1 = _refptr_g_tsaPrmtFlash->bCols;
  }
  (*(ushort *)(_stk + 0x9c)) = (ushort)bVar1;
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    bVar1 = _refptr_g_tsaPrmtFlash->bCols;
  }
  else {
    bVar1 = _refptr_g_tsaPrmtFlash->bRows;
  }
  (*(ushort *)(_stk + 0x9a)) = (ushort)bVar1;
  ((int *)(_stk + 0x68))[0] = 0;
  ((int *)(_stk + 0x68))[1] = 0;
  (*(uint *)(_stk + 0x94)) = CMFRemap_GetRxSlotNum
                       (&_refptr_g_tsaPrmtFlash->field_0x67c,(*(ushort *)(_stk + 0x9c)),((undefined1 *)(_stk + 0x7c)),((int *)(_stk + 0x68)),2);
  (*(uint *)(_stk + 0x90)) = CMFRemap_GetTxSlotNum(&_refptr_g_tsaPrmtFlash->field_0x62c,(*(ushort *)(_stk + 0x9a)));
  (*(byte *)(_stk + 0x19)) = *_refptr_g_tsaPrmtFlashCmfPca;
  for ((*(byte *)(_stk + 0xa7)) = 0; (*(byte *)(_stk + 0xa7)) < (*(uint *)(_stk + 0x90)); (*(byte *)(_stk + 0xa7)) = (*(byte *)(_stk + 0xa7)) + 1) {
    (*(int *)(_stk + 0xa0)) = 0;
    for ((*(byte *)(_stk + 0x9f)) = 0; (*(byte *)(_stk + 0x9f)) < (*(uint *)(_stk + 0x94)); (*(byte *)(_stk + 0x9f)) = (*(byte *)(_stk + 0x9f)) + 1) {
      (*(int *)(_stk + 0x8c)) = ((int *)(_stk + 0x68))[(int)(uint)(*(byte *)(_stk + 0x9f))];
      (*(uint *)(_stk + 0x88)) = (uint)(*(byte *)(_stk + 0x19)) * ((uint)(*(ushort *)(_stk + 0x9c)) * (uint)(*(byte *)(_stk + 0xa7)) + (*(int *)(_stk + 0xa0)));
      *(undefined **)
       (&g_pcaCoef + ((longlong)(int)(uint)(*(byte *)(_stk + 0x9f)) + (longlong)(int)(uint)(*(byte *)(_stk + 0xa7)) * 2) * 8) =
           _refptr_g_tsaPrmtFlashCmfPca + (ulonglong)(*(uint *)(_stk + 0x88)) * 4 + 4;
      (*(undefined8 *)(_stk + 0x28)) = *(undefined8 *)
                  (&g_pcaCoef +
                  ((longlong)(int)(uint)(*(byte *)(_stk + 0x9f)) + (longlong)(int)(uint)(*(byte *)(_stk + 0xa7)) * 2) * 8);
      (*(undefined1 *)(_stk + 0x18)) = (undefined1)(*(int *)(_stk + 0x8c));
      MtLsm_CalcMatrixB(&g_pcaMatrixB +
                        (longlong)(int)(uint)(*(byte *)(_stk + 0x9f)) * 800 + (longlong)(int)(uint)(*(byte *)(_stk + 0xa7)) * 0x640,
                        &g_pcaCoefSqSum +
                        (longlong)(int)(uint)(*(byte *)(_stk + 0x9f)) * 100 + (longlong)(int)(uint)(*(byte *)(_stk + 0xa7)) * 200,400
                        ,((undefined1 *)(_stk + 0x8)));
      (*(int *)(_stk + 0xa0)) = (*(int *)(_stk + 0xa0)) + (*(int *)(_stk + 0x8c));
    }
  }
  _g_param = 1;
  DAT_6bb800a8 = (ushort)_refptr_g_tsaPrmtFlash->bCols;
  DAT_6bb800aa = (ushort)_refptr_g_tsaPrmtFlash->bRows;
  _DAT_6bb800b8 = g_difIdx;
  _DAT_6bb800b0 = g_mapIdx;
  CMFRemap_InitMapIdxBySlotTable
            (&_refptr_g_tsaPrmtFlash->field_0x67c,_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass,
             _refptr_g_tsaPrmtFlash->field_0x6cc,&g_param);
  for ((*(byte *)(_stk + 0x9e)) = 1; (int)(uint)(*(byte *)(_stk + 0x9e)) < (int)((*(ushort *)(_stk + 0x9c)) - 1); (*(byte *)(_stk + 0x9e)) = (*(byte *)(_stk + 0x9e)) + 1) {
    (*(undefined * *)(_stk + 0x80)) = &g_cmfNeighborCoefArray + (longlong)(int)(((*(byte *)(_stk + 0x9e)) - 1) * 9) * 2;
    CMF_GetNeighborCoef((*(undefined * *)(_stk + 0x80)),(*(byte *)(_stk + 0x9e)) - 1,(*(byte *)(_stk + 0x9e)),(*(byte *)(_stk + 0x9e)) + 1);
  }
  return;
}


// ============================================================================
// Function: CMF_ProcessDimCommonOnBuf @ 6ba4eb1d
// ============================================================================

void CMF_ProcessDimCommonOnBuf
               (undefined8 param_1,undefined8 param_2,int param_3,short param_4,short param_5,
               byte param_6,byte param_7,undefined1 param_8)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0xb]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x10];
  int iVar1;
  
                    /* 0xeb1d  484  CMF_ProcessDimCommonOnBuf */
  iVar1 = CMF_GetCmfMethod();
  if (iVar1 == 1) {
    CMF_Filter2DCommon(param_1,param_2,param_8);
  }
  else {
    (*(byte *)(_stk + 0x6)) = param_6;
    if (param_3 == 1) {
      (*(byte *)(_stk + 0x6)) = param_7;
    }
    for ((*(byte *)(_stk + 0x7)) = 0; (*(byte *)(_stk + 0x7)) < (*(byte *)(_stk + 0x6)); (*(byte *)(_stk + 0x7)) = (*(byte *)(_stk + 0x7)) + 1) {
      CMF_ProcessDimUnitOnBuf
                (param_1,param_2,param_3,(*(byte *)(_stk + 0x7)),(int)param_4,(int)param_5,&(*(undefined1 *)(_stk + 0x5)),param_6,param_7)
      ;
    }
  }
  return;
}


// ============================================================================
// Function: CMF_ProcessDimUnit @ 6ba4e2af
// ============================================================================

int CMF_ProcessDimUnit(undefined4 param_1,undefined1 param_2,short param_3,short param_4,
                      undefined8 param_5,undefined1 param_6,undefined1 param_7)

{
  short sVar1;
  
                    /* 0xe2af  485  CMF_ProcessDimUnit */
  sVar1 = CMF_ProcessDimUnitOnBuf
                    (*(undefined8 *)_refptr_g_tsaBufDif,*(undefined8 *)_refptr_g_tsaBufDif,param_1,
                     param_2,(int)param_3,(int)param_4,param_5,param_6,param_7);
  if ((int)*(short *)&_refptr_g_tsaPrpt[1].field_0x14 < (int)sVar1) {
    (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
         (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 8;
  }
  return (int)sVar1;
}


// ============================================================================
// Function: CMF_CopyCommonModeNoise @ 6ba4e7ee
// ============================================================================

void CMF_CopyCommonModeNoise(int param_1,byte param_2,byte param_3,byte param_4,undefined2 param_5)

{
  byte local_a;
  byte local_9;
  
  if (param_1 == 1) {
    for (local_9 = 0; local_9 < param_3; local_9 = local_9 + 1) {
      *(undefined2 *)
       ((longlong)(int)((uint)param_3 * (uint)param_2 + (uint)local_9) * 2 +
       *(longlong *)_refptr_g_tsaBufCommonModeNoise) = param_5;
    }
  }
  else {
    for (local_a = 0; local_a < param_4; local_a = local_a + 1) {
      *(undefined2 *)
       ((longlong)(int)((uint)param_3 * (uint)local_a + (uint)param_2) * 2 +
       *(longlong *)_refptr_g_tsaBufCommonModeNoise) = param_5;
    }
  }
  return;
}


// ============================================================================
// Function: CMF_Filter2DCommonInternal @ 6ba53ad9
// ============================================================================

/* WARNING: Function: ___chkstk_ms replaced with injection: alloca_probe */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void CMF_Filter2DCommonInternal(longlong *param_1,undefined2 param_2,longlong *param_3,int param_4)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x9a08]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x9a10];
  char cVar1;
  uint uVar2;
  longlong lVar3;
  undefined8 *puVar4;
  bool bVar5;
  undefined8 in_stack_ffffffffffff65e8;
  undefined4 uVar7;
  undefined8 uVar6;
  
  uVar7 = (undefined4)((ulonglong)in_stack_ffffffffffff65e8 >> 0x20);
  (*(undefined8 *)(_stk + 0x99f8)) = 0x6ba53ae5;
  memset(((undefined2 *)(_stk + 0x4d48)),0,0x2648);
  (*(errno_t *)(_stk + 0x99ec)) = memcpy_s(((undefined1 *)(_stk + 0xa8)),0x2648,(void *)param_1[1],
                      (ulonglong)DAT_6bb800aa * (ulonglong)DAT_6bb800a8 * 2);
  if ((*(errno_t *)(_stk + 0x99ec)) != 0) {
    uVar7 = 0;
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/CMF/CMF_Pca.c",
                      0x2e0,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  if (param_3 == (longlong *)0x0) {
    DAT_6bb800c0 = ((undefined1 *)(_stk + 0x7398));
    _DAT_6bb800c8 = 0;
    DAT_6bb800a0 = ((undefined1 *)(_stk + 0x26f8));
    DAT_6bb80098 = *param_1;
  }
  else {
    DAT_6bb800c0 = (undefined1 *)*param_3;
    if (DAT_6bb800c0 == (undefined1 *)0x0) {
      DAT_6bb800c0 = ((undefined1 *)(_stk + 0x7398));
    }
    _DAT_6bb800c8 = param_3[1];
    DAT_6bb800a0 = (undefined1 *)param_3[5];
    DAT_6bb80098 = param_3[4];
    if (DAT_6bb80098 == 0) {
      DAT_6bb80098 = *param_1;
    }
  }
  _DAT_6bb800d0 = &_refptr_g_tsaPrmtFlash->field_0x67c;
  _DAT_6bb80090 = ((undefined1 *)(_stk + 0xa8));
  puVar4 = ((undefined8 *)(_stk + 0x58));
  for (lVar3 = 10; lVar3 != 0; lVar3 = lVar3 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  if (param_4 == 1) {
    uVar7 = 0;
    CMF_InitWeightByNeighbor(param_2,&g_param,0,0,0,0);
  }
  else if (param_4 == 2) {
    puVar4 = ((undefined8 *)(_stk + 0x58));
    CMF_InitWeightByNeighbor(param_2,&g_param,1,0,((undefined8 *)(_stk + 0x58)),0x50);
    uVar7 = (undefined4)((ulonglong)puVar4 >> 0x20);
    (*(uint *)(_stk + 0x99f4)) = (uint)DAT_6bb800aa * (uint)DAT_6bb800a8;
    while (uVar2 = (*(uint *)(_stk + 0x99f4)) - 1, bVar5 = (*(uint *)(_stk + 0x99f4)) != 0, (*(uint *)(_stk + 0x99f4)) = uVar2, bVar5) {
      ((undefined2 *)(_stk + 0x4d48))[uVar2] = *(undefined2 *)(DAT_6bb800c0 + (ulonglong)uVar2 * 2);
    }
  }
  uVar6 = CONCAT44(uVar7,0x1324);
  (*(int *)(_stk + 0x99e8)) = CMF_SubtractPreDif(((undefined8 *)(_stk + 0x58)),0x50,((undefined2 *)(_stk + 0x4d48)),((undefined1 *)(_stk + 0xa8)),uVar6);
  uVar7 = (undefined4)((ulonglong)uVar6 >> 0x20);
  if ((*(int *)(_stk + 0x99e8)) != 0) {
    if (param_3 != (longlong *)0x0) {
      DAT_6bb800c0 = (undefined1 *)param_3[2];
      if (DAT_6bb800c0 == (undefined1 *)0x0) {
        DAT_6bb800c0 = ((undefined1 *)(_stk + 0x7398));
      }
      _DAT_6bb800c8 = param_3[3];
    }
    puVar4 = ((undefined8 *)(_stk + 0x8));
    for (lVar3 = 10; lVar3 != 0; lVar3 = lVar3 + -1) {
      *puVar4 = 0;
      puVar4 = puVar4 + 1;
    }
    puVar4 = ((undefined8 *)(_stk + 0x8));
    CMF_InitWeightByNeighbor(param_2,&g_param,0,((undefined8 *)(_stk + 0x58)),puVar4,0x50);
    uVar7 = (undefined4)((ulonglong)puVar4 >> 0x20);
  }
  _DAT_6bb80090 = ((undefined1 *)(_stk + 0xa8));
  DAT_6bb80084 = param_4 == 0;
  CMF_Filter2DCore(&g_param);
  CMF_AddPreDif(((undefined8 *)(_stk + 0x58)),0x50,((undefined2 *)(_stk + 0x4d48)),DAT_6bb80098,CONCAT44(uVar7,0x1324));
  (*(int *)(_stk + 0x99f0)) = (int)((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 3);
  if (param_4 == 2) {
    uVar2 = CMF_GetAverage(DAT_6bb800a0,(uint)DAT_6bb800a8 * (uint)DAT_6bb800aa);
    (*(int *)(_stk + 0x99f0)) = (int)((((int)uVar2 >> 0x1f ^ uVar2) - ((int)uVar2 >> 0x1f)) * 0x19) / 100;
    if (_refptr_g_tsaPrmtFlash[2].field_0x7b4 != '\0') {
      cVar1 = BaliSynaFM_IsFastMoveDetected();
      if (cVar1 != '\0') {
        (*(int *)(_stk + 0x99f0)) = 600;
      }
    }
  }
  _DAT_6bb80090 = (undefined1 *)DAT_6bb80098;
  DAT_6bb80098 = *param_1;
  CMF_FilterOnTx((int)(short)(*(int *)(_stk + 0x99f0)),&g_param);
  return;
}


// ============================================================================
// Function: CMF_GetNeighborCoef @ 6ba511ea
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void CMF_GetNeighborCoef(longlong param_1,byte param_2,undefined1 param_3,undefined1 param_4)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x278]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x280];
  uint uVar1;
  double dVar2;
  
  (*(uint *)(_stk + 0x250)) = (uint)(byte)*_refptr_g_tsaPrmtFlashCmfPca;
  ((byte *)(_stk + 0x1c8))[0] = param_2;
  ((byte *)(_stk + 0x1c8))[1] = param_3;
  ((byte *)(_stk + 0x1c8))[2] = param_4;
  if ((double)*(float *)(_refptr_g_tsaPrmtFlashCmfPca + ((ulonglong)((*(uint *)(_stk + 0x250)) - 1) + 800) * 4 + 4)
      <= _DAT_6bb51d50) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/CMF/CMF_Pca.c",
                      0x4a,"TSAPrmtCmfPca(cmfPca, cmfPcaConstrain)[pcaDegree - 1] > 1e-12");
    HardwareAnalyzer_SetSoftException();
  }
  Matrix_ReShape(((undefined4 *)(_stk + 0x1d8)),3,(*(uint *)(_stk + 0x250)) & 0xff);
  for ((*(uint *)(_stk + 0x264)) = 0; (*(uint *)(_stk + 0x264)) < 3; (*(uint *)(_stk + 0x264)) = (*(uint *)(_stk + 0x264)) + 1) {
    (*(int *)(_stk + 0x24c)) = ((byte *)(_stk + 0x1c8))[(*(uint *)(_stk + 0x264))] * (*(uint *)(_stk + 0x250));
    for ((*(uint *)(_stk + 0x260)) = 0; (*(uint *)(_stk + 0x260)) < (*(uint *)(_stk + 0x250)); (*(uint *)(_stk + 0x260)) = (*(uint *)(_stk + 0x260)) + 1) {
      ((undefined4 *)(_stk + 0x1d8))[(*(byte *)(_stk + 0x23d)) * (*(uint *)(_stk + 0x264)) + (*(uint *)(_stk + 0x260))] =
           *(undefined4 *)(_refptr_g_tsaPrmtFlashCmfPca + (ulonglong)((*(int *)(_stk + 0x24c)) + (*(uint *)(_stk + 0x260))) * 4 + 4);
    }
  }
  Matrix_Init(((float *)(_stk + 0x158)),(*(uint *)(_stk + 0x250)) & 0xff,(*(uint *)(_stk + 0x250)) & 0xff,0);
  for ((*(uint *)(_stk + 0x25c)) = 0; (*(uint *)(_stk + 0x25c)) < (*(uint *)(_stk + 0x250)); (*(uint *)(_stk + 0x25c)) = (*(uint *)(_stk + 0x25c)) + 1) {
    uVar1 = ((*(byte *)(_stk + 0x1bd)) + 1) * (*(uint *)(_stk + 0x25c));
    dVar2 = sqrt((double)(*(float *)(_refptr_g_tsaPrmtFlashCmfPca +
                                    ((ulonglong)(*(uint *)(_stk + 0x25c)) + 800) * 4 + 4) * DAT_6bb51d58));
    ((float *)(_stk + 0x158))[uVar1] = (float)(1.0 / dVar2);
  }
  Matrix_Mul(((undefined4 *)(_stk + 0x1d8)),((undefined4 *)(_stk + 0x1d8)),((float *)(_stk + 0x158)));
  Matrix_Rotate(((undefined1 *)(_stk + 0xe8)),((undefined4 *)(_stk + 0x1d8)));
  Matrix_Mul(((undefined4 *)(_stk + 0x1d8)),((undefined4 *)(_stk + 0x1d8)),((undefined1 *)(_stk + 0xe8)));
  Matrix_Eig(((undefined4 *)(_stk + 0x1d8)),((float *)(_stk + 0x78)),((undefined1 *)(_stk + 0x8)));
  Matrix_Rotate(((float *)(_stk + 0x78)),((float *)(_stk + 0x78)));
  for ((*(uint *)(_stk + 0x258)) = 0; (*(uint *)(_stk + 0x258)) < 3; (*(uint *)(_stk + 0x258)) = (*(uint *)(_stk + 0x258)) + 1) {
    ((float *)(_stk + 0x78))[(int)((uint)(*(byte *)(_stk + 0xdd)) + (uint)(*(byte *)(_stk + 0xdd)))] =
         ((float *)(_stk + 0x78))[1] * ((float *)(_stk + 0x78))[(int)((*(byte *)(_stk + 0xdd)) + 2)] -
         ((float *)(_stk + 0x78))[(int)((*(byte *)(_stk + 0xdd)) + 1)] * ((float *)(_stk + 0x78))[2];
    ((float *)(_stk + 0x78))[(int)((uint)(*(byte *)(_stk + 0xdd)) * 2 + 1)] =
         ((float *)(_stk + 0x78))[(int)(uint)(*(byte *)(_stk + 0xdd))] * ((float *)(_stk + 0x78))[2] -
         ((float *)(_stk + 0x78))[0] * ((float *)(_stk + 0x78))[(int)((*(byte *)(_stk + 0xdd)) + 2)];
    ((float *)(_stk + 0x78))[(int)(((*(byte *)(_stk + 0xdd)) + 1) * 2)] =
         ((float *)(_stk + 0x78))[0] * ((float *)(_stk + 0x78))[(int)((*(byte *)(_stk + 0xdd)) + 1)] -
         ((float *)(_stk + 0x78))[(int)(uint)(*(byte *)(_stk + 0xdd))] * ((float *)(_stk + 0x78))[1];
  }
  for ((*(uint *)(_stk + 0x254)) = 0; (*(uint *)(_stk + 0x254)) < 9; (*(uint *)(_stk + 0x254)) = (*(uint *)(_stk + 0x254)) + 1) {
    *(short *)((ulonglong)(*(uint *)(_stk + 0x254)) * 2 + param_1) = (short)(int)(((float *)(_stk + 0x78))[(*(uint *)(_stk + 0x254))] * DAT_6bb51d5c)
    ;
  }
  return;
}


// ============================================================================
// Function: CMF_ProcessDimUnitOnBuf @ 6ba4e0ec
// ============================================================================

undefined4
CMF_ProcessDimUnitOnBuf
          (longlong param_1,longlong param_2,int param_3,byte param_4,short param_5,short param_6,
          undefined8 param_7,byte param_8,byte param_9)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x418]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x420];
  undefined4 uVar1;
  
                    /* 0xe0ec  486  CMF_ProcessDimUnitOnBuf */
  (*(byte *)(_stk + 0x412)) = param_9;
  if (param_3 == 1) {
    (*(ushort *)(_stk + 0x416)) = (ushort)param_8 * (ushort)param_4;
    (*(byte *)(_stk + 0x413)) = 1;
    (*(byte *)(_stk + 0x412)) = param_8;
  }
  else {
    (*(ushort *)(_stk + 0x416)) = (ushort)param_4;
    (*(byte *)(_stk + 0x413)) = param_8;
  }
  (*(ushort *)(_stk + 0x414)) = (*(ushort *)(_stk + 0x416));
  for ((*(byte *)(_stk + 0x411)) = 0; (*(byte *)(_stk + 0x411)) < (*(byte *)(_stk + 0x412)); (*(byte *)(_stk + 0x411)) = (*(byte *)(_stk + 0x411)) + 1) {
    ((int *)(_stk + 0x8))[(int)(uint)(*(byte *)(_stk + 0x411))] = (int)*(short *)(param_2 + (ulonglong)(*(ushort *)(_stk + 0x414)) * 2);
    (*(ushort *)(_stk + 0x414)) = (*(ushort *)(_stk + 0x414)) + (*(byte *)(_stk + 0x413));
  }
  uVar1 = CMF_ProcessCore(((int *)(_stk + 0x8)),0x100,(*(byte *)(_stk + 0x412)),(int)param_5,(int)param_6,param_7);
  (*(ushort *)(_stk + 0x414)) = (*(ushort *)(_stk + 0x416));
  for ((*(byte *)(_stk + 0x411)) = 0; (*(byte *)(_stk + 0x411)) < (*(byte *)(_stk + 0x412)); (*(byte *)(_stk + 0x411)) = (*(byte *)(_stk + 0x411)) + 1) {
    *(short *)((ulonglong)(*(ushort *)(_stk + 0x414)) * 2 + param_1) = (short)((int *)(_stk + 0x8))[(int)(uint)(*(byte *)(_stk + 0x411))];
    (*(ushort *)(_stk + 0x414)) = (*(ushort *)(_stk + 0x414)) + (*(byte *)(_stk + 0x413));
  }
  return uVar1;
}


// ============================================================================
// Function: CMF_Filter2DCommon @ 6ba53f96
// ============================================================================

void CMF_Filter2DCommon(undefined8 param_1,undefined8 param_2,char param_3)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x28]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x30];
  
                    /* 0x13f96  468  CMF_Filter2DCommon */
  (*(uint *)(_stk + 0x24)) = (uint)(param_3 != '\0');
  (*(undefined8 *)(_stk + 0x8)) = param_1;
  (*(undefined8 *)(_stk + 0x10)) = param_2;
  CMF_Filter2DCommonInternal
            (&(*(undefined8 *)(_stk + 0x8)),*(undefined2 *)(_refptr_g_tsaPrmtFlashCmfPca + 0xce8),0,(*(uint *)(_stk + 0x24)));
  return;
}


// ============================================================================
// Function: CMF_Filter2DCore @ 6ba51d77
// ============================================================================

/* WARNING: Function: ___chkstk_ms replaced with injection: alloca_probe */

void CMF_Filter2DCore(undefined4 *param_1)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x739c]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x73a0];
  longlong lVar1;
  undefined8 *puVar2;
  undefined8 in_stack_ffffffffffff8c48;
  undefined4 uVar3;
  
                    /* 0x11d77  469  CMF_Filter2DCore */
  uVar3 = (undefined4)((ulonglong)in_stack_ffffffffffff8c48 >> 0x20);
  (*(undefined8 *)(_stk + 0x7388)) = 0x6ba51d83;
  CMFRemap_MapBuffer(((undefined1 *)(_stk + 0x4d28)),0x1324,*(undefined8 *)(param_1 + 4),param_1);
  if (*(char *)(param_1 + 1) == '\0') {
    CMFRemap_MapBuffer(((undefined1 *)(_stk + 0x26d8)),0x1324,*(undefined8 *)(param_1 + 0x10),param_1);
  }
  puVar2 = &(*(undefined8 *)(_stk + 0x8));
  for (lVar1 = 0xf; lVar1 != 0; lVar1 = lVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  (*(undefined1 *)(_stk + 0x41)) = *(undefined1 *)(param_1 + 1);
  *(uint32_t*)&(*(undefined8 *)(_stk + 0x8)) = *param_1;
  *((uint32_t*)&(*(undefined8 *)(_stk + 0x8)) + 1) = *(undefined4 *)((longlong)param_1 + 6);
  (*(undefined8 *)(_stk + 0x38)) = *(undefined8 *)(param_1 + 0x14);
  (*(byte *)(_stk + 0x40)) = CMFRemap_GetRxSlotNum
                         ((*(undefined8 *)(_stk + 0x38)),*(undefined2 *)(param_1 + 0xb),((undefined1 *)(_stk + 0x4)),((undefined1 *)(_stk + 0x78)),
                          CONCAT44(uVar3,2));
  for ((*(uint *)(_stk + 0x7384)) = 0; (*(uint *)(_stk + 0x7384)) < *(ushort *)((longlong)param_1 + 0x2e); (*(uint *)(_stk + 0x7384)) = (*(uint *)(_stk + 0x7384)) + 1) {
    (*(byte *)(_stk + 0x7382)) = (&_refptr_g_tsaPrmtFlash->field_0x62c)[(*(uint *)(_stk + 0x7384))];
    for ((*(byte *)(_stk + 0x7383)) = 0; (*(byte *)(_stk + 0x7383)) < (*(byte *)(_stk + 0x40)); (*(byte *)(_stk + 0x7383)) = (*(byte *)(_stk + 0x7383)) + 1) {
      (&(*(undefined8 *)(_stk + 0x8)))[(longlong)(int)(uint)(*(byte *)(_stk + 0x7383)) + 8] =
           &g_pcaMatrixB +
           (longlong)(int)(uint)(*(byte *)(_stk + 0x7383)) * 800 + (longlong)(int)(uint)(*(byte *)(_stk + 0x7382)) * 0x640;
      (&(*(undefined8 *)(_stk + 0x8)))[(longlong)(int)(uint)(*(byte *)(_stk + 0x7383)) + 0xc] =
           *(undefined8 *)
            (&g_pcaCoef + ((longlong)(int)(uint)(*(byte *)(_stk + 0x7383)) + (longlong)(int)(uint)(*(byte *)(_stk + 0x7382)) * 2) * 8);
      (&(*(undefined8 *)(_stk + 0x8)))[(longlong)(int)(uint)(*(byte *)(_stk + 0x7383)) + 10] =
           &g_pcaCoefSqSum +
           (longlong)(int)(uint)(*(byte *)(_stk + 0x7383)) * 100 + (longlong)(int)(uint)(*(byte *)(_stk + 0x7382)) * 200;
    }
    (*(uint *)(_stk + 0x737c)) = *(ushort *)(param_1 + 0xb) * (*(uint *)(_stk + 0x7384));
    (*(undefined1 * *)(_stk + 0x20)) = ((undefined1 *)(_stk + 0x4d28)) + (ulonglong)(*(uint *)(_stk + 0x737c)) * 2;
    (*(undefined1 * *)(_stk + 0x28)) = ((undefined1 *)(_stk + 0x88)) + (ulonglong)(*(uint *)(_stk + 0x737c)) * 2;
    (*(undefined1 * *)(_stk + 0x10)) = ((undefined1 *)(_stk + 0x4d28)) + (ulonglong)(*(uint *)(_stk + 0x737c)) * 2;
    (*(undefined1 * *)(_stk + 0x18)) = ((undefined1 *)(_stk + 0x26d8)) + (ulonglong)(*(uint *)(_stk + 0x737c)) * 2;
    (*(undefined2 *)(_stk + 0x30)) = *(undefined2 *)(param_1 + 0xb);
    CMF_Filter1DCore(&(*(undefined8 *)(_stk + 0x8)));
  }
  CMFRemap_MapBufferBack
            (*(undefined8 *)(param_1 + 6),
             *(short *)((longlong)param_1 + 0x2a) * *(short *)(param_1 + 10),((undefined1 *)(_stk + 0x4d28)),0x1324,
             param_1);
  if (*(longlong *)(param_1 + 8) != 0) {
    CMFRemap_MapBufferBack
              (*(undefined8 *)(param_1 + 8),
               *(short *)((longlong)param_1 + 0x2a) * *(short *)(param_1 + 10),((undefined1 *)(_stk + 0x88)),0x1324,
               param_1);
  }
  return;
}


// ============================================================================
// Function: CMF_InitWeightByNeighbor @ 6ba529b7
// ============================================================================

/* WARNING: Function: ___chkstk_ms replaced with injection: alloca_probe */
/* WARNING: Removing unreachable block (ram,0x6ba52eef) */

void CMF_InitWeightByNeighbor(ushort param_1,longlong param_2,int param_3,longlong param_4,longlong param_5, ...)
{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x27f8]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x2800];
  uint uVar1;
  uint uVar2;
  double dVar3;
  
  (*(undefined2 * *)(_stk + 0x27f0)) = (undefined2 *)0x6ba529c2;
  CMFRemap_MapBuffer(((short *)(_stk + 0x148)),0x1324,*(undefined8 *)(param_2 + 0x10),param_2);
  (*(undefined2 * *)(_stk + 0x27c0)) = *(undefined2 **)(param_2 + 0x40);
  (*(byte *)(_stk + 0x27bf)) = (byte)*(undefined2 *)(param_2 + 0x2c);
  (*(byte *)(_stk + 0x27be)) = (byte)*(undefined2 *)(param_2 + 0x2e);
  (*(int *)(_stk + 0x27b8)) = (uint)param_1 * (uint)param_1;
  (*(int *)(_stk + 0x27b4)) = (int)(short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax;
  (*(int *)(_stk + 0x27ec)) = (uint)(*(byte *)(_stk + 0x27bf)) * (uint)(*(byte *)(_stk + 0x27be));
  (*(undefined2 * *)(_stk + 0x27f0)) = (*(undefined2 * *)(_stk + 0x27c0));
  while ((*(int *)(_stk + 0x27ec)) = (*(int *)(_stk + 0x27ec)) + -1, (*(int *)(_stk + 0x27ec)) != 0) {
    *(*(undefined2 * *)(_stk + 0x27f0)) = 1;
    (*(undefined2 * *)(_stk + 0x27f0)) = (*(undefined2 * *)(_stk + 0x27f0)) + 1;
  }
  (*(int *)(_stk + 0x27ec)) = 0;
  for ((*(byte *)(_stk + 0x27eb)) = 0; (*(byte *)(_stk + 0x27eb)) < (*(byte *)(_stk + 0x27be)); (*(byte *)(_stk + 0x27eb)) = (*(byte *)(_stk + 0x27eb)) + 1) {
    if ((param_4 == 0) || (*(char *)(param_4 + (ulonglong)(*(byte *)(_stk + 0x27eb))) != '\0')) {
      (*(int *)(_stk + 0x27e4)) = 0;
      if (param_5 != 0) {
        *(undefined1 *)(param_5 + (ulonglong)(*(byte *)(_stk + 0x27eb))) = 1;
      }
      (*(ushort *)(_stk + 0x27e2)) = (ushort)(*(byte *)(_stk + 0x27bf)) * (ushort)(*(byte *)(_stk + 0x27eb)) + 1;
      (*(undefined * *)(_stk + 0x27d8)) = &g_cmfNeighborCoefArray;
      for ((*(byte *)(_stk + 0x27d7)) = 1; (int)(uint)(*(byte *)(_stk + 0x27d7)) < (int)((*(byte *)(_stk + 0x27bf)) - 1); (*(byte *)(_stk + 0x27d7)) = (*(byte *)(_stk + 0x27d7)) + 1) {
        (*(int *)(_stk + 0x27b0)) = (int)((short *)(_stk + 0x148))[(int)((*(ushort *)(_stk + 0x27e2)) - 1)];
        (*(int *)(_stk + 0x27ac)) = (int)((short *)(_stk + 0x148))[(int)(uint)(*(ushort *)(_stk + 0x27e2))];
        (*(ushort *)(_stk + 0x27e2)) = (*(ushort *)(_stk + 0x27e2)) + 1;
        (*(int *)(_stk + 0x27a8)) = (int)((short *)(_stk + 0x148))[(int)(uint)(*(ushort *)(_stk + 0x27e2))];
        (*(int *)(_stk + 0x27a4)) = CMF_GetDistance((*(int *)(_stk + 0x27b0)),(*(int *)(_stk + 0x27ac)),(*(int *)(_stk + 0x27a8)),(*(undefined * *)(_stk + 0x27d8)));
        ((int *)(_stk + 0x8))[(int)(uint)(*(byte *)(_stk + 0x27d7))] = (*(int *)(_stk + 0x27a4));
        (*(undefined * *)(_stk + 0x27d8)) = (*(undefined * *)(_stk + 0x27d8)) + 0x12;
      }
      (*(int *)(_stk + 0x27d0)) = 1;
      (*(ushort *)(_stk + 0x27e2)) = (ushort)(*(byte *)(_stk + 0x27bf)) * (ushort)(*(byte *)(_stk + 0x27eb));
      for ((*(byte *)(_stk + 0x27cf)) = 1; (*(ushort *)(_stk + 0x27e2)) = (*(ushort *)(_stk + 0x27e2)) + 1, (int)(uint)(*(byte *)(_stk + 0x27cf)) < (int)((*(byte *)(_stk + 0x27bf)) - 1);
          (*(byte *)(_stk + 0x27cf)) = (*(byte *)(_stk + 0x27cf)) + 1) {
        (*(ushort *)(_stk + 0x27a2)) = *(ushort *)(*(longlong *)(param_2 + 0x38) + (ulonglong)(*(ushort *)(_stk + 0x27e2)) * 2);
        if ((*(int *)(_stk + 0x27b8)) < ((int *)(_stk + 0x8))[(int)(uint)(*(byte *)(_stk + 0x27cf))]) {
          (*(int *)(_stk + 0x27d0)) = 0;
          (*(undefined2 * *)(_stk + 0x27c0))[(*(ushort *)(_stk + 0x27a2))] = 0;
          (*(int *)(_stk + 0x27e4)) = (*(int *)(_stk + 0x27e4)) + 1;
        }
        else if ((param_3 != 0) &&
                (uVar1 = (uint)*(short *)(*(longlong *)_refptr_g_tsaBufPreDif +
                                         (ulonglong)(*(ushort *)(_stk + 0x27a2)) * 2), uVar2 = (int)uVar1 >> 0x1f,
                (*(int *)(_stk + 0x27b4)) < (int)((uVar1 ^ uVar2) - uVar2))) {
          (*(undefined2 * *)(_stk + 0x27c0))[(*(ushort *)(_stk + 0x27a2))] = 0;
          (*(int *)(_stk + 0x27e4)) = (*(int *)(_stk + 0x27e4)) + 1;
        }
      }
      if ((param_5 != 0) && (((int)((*(byte *)(_stk + 0x27bf)) - 5) <= (*(int *)(_stk + 0x27e4)) || ((*(int *)(_stk + 0x27d0)) != 0)))) {
        *(undefined1 *)(param_5 + (ulonglong)(*(byte *)(_stk + 0x27eb))) = 0;
      }
      ((int *)(_stk + 0x8))[0] = ((int *)(_stk + 0x8))[1];
      ((int *)(_stk + 0x8))[(int)((*(byte *)(_stk + 0x27bf)) - 1)] = ((int *)(_stk + 0x8))[(int)((*(byte *)(_stk + 0x27bf)) - 2)];
      (*(undefined2 * *)(_stk + 0x27c0))[*(ushort *)
                (*(longlong *)(param_2 + 0x38) +
                (longlong)(int)((uint)(*(byte *)(_stk + 0x27bf)) * (uint)(*(byte *)(_stk + 0x27eb))) * 2)] =
           (*(undefined2 * *)(_stk + 0x27c0))[*(ushort *)
                     (*(longlong *)(param_2 + 0x38) +
                     (longlong)(int)((uint)(*(byte *)(_stk + 0x27bf)) * (uint)(*(byte *)(_stk + 0x27eb)) + 1) * 2)];
      (*(ushort *)(_stk + 0x27a0)) = *(ushort *)
                  (*(longlong *)(param_2 + 0x38) +
                  (longlong)(int)((uint)(*(byte *)(_stk + 0x27bf)) * (uint)(*(byte *)(_stk + 0x27eb)) + ((*(byte *)(_stk + 0x27bf)) - 1)) * 2);
      (*(ushort *)(_stk + 0x279e)) = *(ushort *)
                  (*(longlong *)(param_2 + 0x38) +
                  (longlong)(int)((uint)(*(byte *)(_stk + 0x27bf)) * (uint)(*(byte *)(_stk + 0x27eb)) + ((*(byte *)(_stk + 0x27bf)) - 2)) * 2);
      (*(undefined2 * *)(_stk + 0x27c0))[(*(ushort *)(_stk + 0x27a0))] = (*(undefined2 * *)(_stk + 0x27c0))[(*(ushort *)(_stk + 0x279e))];
      if ((int)((uint)(*(byte *)(_stk + 0x27bf)) * 2 + (uint)(*(byte *)(_stk + 0x27bf))) >> 2 < (*(int *)(_stk + 0x27e4))) {
        (*(int *)(_stk + 0x2798)) = Misc_FindKthSmallest(((int *)(_stk + 0x8)),(*(byte *)(_stk + 0x27bf)),(*(byte *)(_stk + 0x27bf)) >> 2);
        dVar3 = sqrt((double)(*(int *)(_stk + 0x2798)));
        (*(int *)(_stk + 0x2798)) = ((uint)param_1 + (int)dVar3) * ((uint)param_1 + (int)dVar3);
        for ((*(byte *)(_stk + 0x27ce)) = 0; (*(byte *)(_stk + 0x27ce)) < (*(byte *)(_stk + 0x27bf)); (*(byte *)(_stk + 0x27ce)) = (*(byte *)(_stk + 0x27ce)) + 1) {
          (*(ushort *)(_stk + 0x27e2)) = (ushort)(*(byte *)(_stk + 0x27bf)) * (ushort)(*(byte *)(_stk + 0x27eb)) + (ushort)(*(byte *)(_stk + 0x27ce));
          (*(ushort *)(_stk + 0x2796)) = *(ushort *)(*(longlong *)(param_2 + 0x38) + (ulonglong)(*(ushort *)(_stk + 0x27e2)) * 2);
          (*(undefined2 * *)(_stk + 0x27c0))[(*(ushort *)(_stk + 0x2796))] = (ushort)(((int *)(_stk + 0x8))[(int)(uint)(*(byte *)(_stk + 0x27ce))] <= (*(int *)(_stk + 0x2798)));
        }
      }
      if (*(longlong *)(param_2 + 0x48) != 0) {
        CMF_UpdateNeighborDistance
                  (*(undefined8 *)(param_2 + 0x48),((int *)(_stk + 0x8)),(*(byte *)(_stk + 0x27bf)),(*(byte *)(_stk + 0x27eb)),param_2);
      }
    }
  }
  CMF_CrossErosionWeight((*(undefined2 * *)(_stk + 0x27c0)),*(undefined2 *)(param_2 + 0x28),*(undefined2 *)(param_2 + 0x2a));
  CMF_SetNotchToZero((*(undefined2 * *)(_stk + 0x27c0)),(uint)*(ushort *)(param_2 + 0x2a) * (uint)*(ushort *)(param_2 + 0x28))
  ;
  CMF_ConditionClearCmfToPreDif(param_5,(*(byte *)(_stk + 0x27be)),param_2,(*(undefined2 * *)(_stk + 0x27c0)));
  return;
}


// ============================================================================
// Function: CMF_FilterOnTx @ 6ba534d8
// ============================================================================

void CMF_FilterOnTx(short param_1,longlong param_2)

{
  ushort uVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  uint uVar8;
  int local_18;
  int local_14;
  uint local_10;
  uint local_c;
  
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    uVar1 = *(ushort *)(param_2 + 0x2a);
  }
  else {
    uVar1 = *(ushort *)(param_2 + 0x28);
  }
  uVar2 = (uint)uVar1;
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    uVar1 = *(ushort *)(param_2 + 0x28);
  }
  else {
    uVar1 = *(ushort *)(param_2 + 0x2a);
  }
  uVar3 = (uint)uVar1;
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    uVar4 = (uint)*(ushort *)(param_2 + 0x28);
  }
  else {
    uVar4 = 1;
  }
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    uVar5 = 1;
  }
  else {
    uVar5 = (uint)*(ushort *)(param_2 + 0x28);
  }
  for (local_c = 0; local_c < uVar2; local_c = local_c + 1) {
    local_14 = 0;
    local_18 = 0;
    for (local_10 = 0; local_10 < uVar3; local_10 = local_10 + 1) {
      uVar7 = local_10 * uVar5 + local_c * uVar4;
      uVar6 = (uint)*(short *)(*(longlong *)(param_2 + 0x10) + (ulonglong)uVar7 * 2);
      if (((*(char *)(param_2 + 4) != '\0') ||
          (*(short *)(*(longlong *)(param_2 + 0x40) + (ulonglong)uVar7 * 2) != 0)) &&
         (uVar8 = (int)uVar6 >> 0x1f, (int)((uVar8 ^ uVar6) - uVar8) < (int)param_1)) {
        local_14 = local_14 + *(short *)(*(longlong *)(param_2 + 0x10) + (ulonglong)uVar7 * 2);
        local_18 = local_18 + 1;
      }
    }
    if (local_18 < 4) {
      local_14 = 0;
      local_18 = 0;
      for (local_10 = 0; local_10 < uVar3; local_10 = local_10 + 1) {
        uVar7 = local_10 * uVar5 + local_c * uVar4;
        uVar6 = (uint)*(short *)(*(longlong *)(param_2 + 0x10) + (ulonglong)uVar7 * 2);
        uVar8 = (int)uVar6 >> 0x1f;
        if ((int)((uVar8 ^ uVar6) - uVar8) < (int)param_1) {
          local_14 = local_14 + *(short *)(*(longlong *)(param_2 + 0x10) + (ulonglong)uVar7 * 2);
          local_18 = local_18 + 1;
        }
      }
    }
    if (local_18 != 0) {
      local_14 = local_14 / local_18;
    }
    for (local_10 = 0; local_10 < uVar3; local_10 = local_10 + 1) {
      uVar7 = local_10 * uVar5 + local_c * uVar4;
      *(short *)((ulonglong)uVar7 * 2 + *(longlong *)(param_2 + 0x20)) =
           (short)local_14 + *(short *)(*(longlong *)(param_2 + 0x20) + (ulonglong)uVar7 * 2);
      *(short *)((ulonglong)uVar7 * 2 + *(longlong *)(param_2 + 0x18)) =
           *(short *)(*(longlong *)(param_2 + 0x10) + (ulonglong)uVar7 * 2) - (short)local_14;
    }
  }
  CMF_FilterOnTxOfRx(1,2,param_2);
  CMF_FilterOnTxOfRx(0,1,param_2);
  CMF_FilterOnTxOfRx(uVar2 - 1 & 0xff,uVar2 - 2 & 0xff,param_2);
  return;
}


// ============================================================================
// Function: CMF_GetAverage @ 6ba537c3
// ============================================================================

ulonglong CMF_GetAverage(short *param_1,int param_2)

{
  ulonglong uVar1;
  uint uVar2;
  int local_1c;
  short *local_18;
  int local_c;
  
  if (param_2 < 1) {
    uVar1 = 0;
  }
  else {
    local_c = 0;
    local_1c = param_2;
    local_18 = param_1;
    do {
      uVar2 = (int)*local_18 >> 0x1f;
      local_c = local_c + (((int)*local_18 ^ uVar2) - uVar2);
      local_1c = local_1c + -1;
      local_18 = local_18 + 1;
    } while (local_1c != 0);
    uVar1 = (longlong)local_c / (longlong)param_2 & 0xffffffff;
  }
  return uVar1;
}


// ============================================================================
// Function: CMF_SubtractPreDif @ 6ba53823
// ============================================================================

undefined4 CMF_SubtractPreDif(longlong param_1,undefined8 param_2,longlong param_3,longlong param_4, ...)

{
  ushort uVar1;
  byte bVar2;
  byte bVar3;
  ushort uVar4;
  byte local_e;
  byte local_d;
  undefined4 local_c;
  
  uVar1 = DAT_6bb800a8;
  bVar3 = (byte)DAT_6bb800a8;
  bVar2 = (byte)DAT_6bb800aa;
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass != '\0') {
    bVar2 = bVar3;
  }
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass != '\0') {
    bVar3 = (byte)DAT_6bb800aa;
  }
  local_c = 0;
  for (local_d = 0; local_d < bVar3; local_d = local_d + 1) {
    if (*(char *)(param_1 + (ulonglong)local_d) != '\0') {
      for (local_e = 0; local_e < bVar2; local_e = local_e + 1) {
        if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
          uVar4 = (uVar1 & 0xff) * (ushort)local_e + (ushort)local_d;
        }
        else {
          uVar4 = (uVar1 & 0xff) * (ushort)local_d + (ushort)local_e;
        }
        if (*(short *)(param_3 + (ulonglong)uVar4 * 2) == 0) {
          *(short *)((ulonglong)uVar4 * 2 + param_4) =
               *(short *)(param_4 + (ulonglong)uVar4 * 2) -
               *(short *)(*(longlong *)_refptr_g_tsaBufPreDif + (ulonglong)uVar4 * 2);
          local_c = 1;
        }
      }
    }
  }
  return local_c;
}


// ============================================================================
// Function: CMF_AddPreDif @ 6ba53988
// ============================================================================

void CMF_AddPreDif(longlong param_1,undefined8 param_2,longlong param_3,longlong param_4, ...)

{
  ushort uVar1;
  byte bVar2;
  byte bVar3;
  ushort uVar4;
  byte local_a;
  byte local_9;
  
  uVar1 = DAT_6bb800a8;
  bVar3 = (byte)DAT_6bb800a8;
  bVar2 = (byte)DAT_6bb800aa;
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass != '\0') {
    bVar2 = bVar3;
  }
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass != '\0') {
    bVar3 = (byte)DAT_6bb800aa;
  }
  for (local_9 = 0; local_9 < bVar3; local_9 = local_9 + 1) {
    if (*(char *)(param_1 + (ulonglong)local_9) != '\0') {
      for (local_a = 0; local_a < bVar2; local_a = local_a + 1) {
        if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
          uVar4 = (uVar1 & 0xff) * (ushort)local_a + (ushort)local_9;
        }
        else {
          uVar4 = (uVar1 & 0xff) * (ushort)local_9 + (ushort)local_a;
        }
        if (*(short *)(param_3 + (ulonglong)uVar4 * 2) == 0) {
          *(short *)((ulonglong)uVar4 * 2 + param_4) =
               *(short *)(*(longlong *)_refptr_g_tsaBufPreDif + (ulonglong)uVar4 * 2) +
               *(short *)(param_4 + (ulonglong)uVar4 * 2);
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: CMF_GetNoiseValue @ 6ba4d97a
// ============================================================================

undefined2 CMF_GetNoiseValue(byte param_1,byte param_2)

{
  undefined2 uVar1;
  
                    /* 0xd97a  472  CMF_GetNoiseValue */
  if ((param_1 < _refptr_g_tsaPrmtFlash->bCols) && (param_2 < _refptr_g_tsaPrmtFlash->bRows)) {
    uVar1 = *(undefined2 *)
             ((ulonglong)
              ((uint)param_1 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
             *(longlong *)_refptr_g_tsaBufCommonModeNoise);
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: CMF_ProcessCore @ 6ba4e799
// ============================================================================

undefined8
CMF_ProcessCore(longlong param_1,undefined8 param_2,undefined4 param_3,undefined4 param_4,
               undefined4 param_5,undefined8 param_6)

{
  undefined8 uVar1;
  
                    /* 0xe799  479  CMF_ProcessCore */
  if (param_1 == 0) {
    uVar1 = 0;
  }
  else {
    uVar1 = CMF_ProcessCoreWeight(param_1,0,param_3,param_4,param_5,param_6);
  }
  return uVar1;
}


// ============================================================================
// Function: CMF_ProcessCoreWeight @ 6ba4e55b
// ============================================================================

int CMF_ProcessCoreWeight
              (longlong param_1,undefined8 param_2,int param_3,int param_4,undefined4 param_5,
              undefined1 *param_6)

{
  int iVar1;
  byte local_9;
  
                    /* 0xe55b  480  CMF_ProcessCoreWeight */
  if (param_1 == 0) {
    iVar1 = 0;
  }
  else {
    CMF_NoiseCal(param_1,param_2,param_3,param_4,param_5);
    if ((((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) != 0) &&
         (_refptr_g_tsaPrmtFlash->field_0x6d != '\0')) &&
        (iVar1 = (DAT_6bb7f128 * 3) / 2, 0 < iVar1)) && (iVar1 < param_4)) {
      CMF_NoiseCal(param_1,param_2,param_3,iVar1,param_5);
    }
    for (local_9 = 0; (int)(uint)local_9 < param_3; local_9 = local_9 + 1) {
      *(int *)(param_1 + (ulonglong)local_9 * 4) =
           *(int *)(param_1 + (ulonglong)local_9 * 4) - DAT_6bb7f128;
    }
    if (param_6 != (undefined1 *)0x0) {
      *param_6 = DAT_6bb7f124;
    }
    iVar1 = DAT_6bb7f128;
  }
  return iVar1;
}


// ============================================================================
// Function: CMF_SSProcessCoreWeightHalfDim @ 6ba4e685
// ============================================================================

int CMF_SSProcessCoreWeightHalfDim
              (longlong param_1,undefined8 param_2,uint param_3,undefined4 param_4,
              undefined4 param_5,undefined1 *param_6)

{
  byte bVar1;
  int iVar2;
  byte local_9;
  
                    /* 0xe685  487  CMF_SSProcessCoreWeightHalfDim */
  if (param_1 == 0) {
    iVar2 = 0;
  }
  else {
    CMF_NoiseCal(param_1,param_2,param_3,param_4,param_5);
    bVar1 = SS_TouchLength(param_1,param_3 & 0xff,600);
    if ((600 < DAT_6bb7f128) && ((int)(param_3 * 2) / 3 < (int)(uint)bVar1)) {
      DAT_6bb7f124 = 0;
      DAT_6bb7f128 = 0;
    }
    for (local_9 = 0; (int)(uint)local_9 < (int)param_3; local_9 = local_9 + 1) {
      *(int *)(param_1 + (ulonglong)local_9 * 4) =
           *(int *)(param_1 + (ulonglong)local_9 * 4) - DAT_6bb7f128;
    }
    if (param_6 != (undefined1 *)0x0) {
      *param_6 = DAT_6bb7f124;
    }
    iVar2 = DAT_6bb7f128;
  }
  return iVar2;
}


// ============================================================================
// Function: CMF_ProcessDimCommon @ 6ba4ea9c
// ============================================================================

void CMF_ProcessDimCommon
               (undefined4 param_1,short param_2,short param_3,undefined1 param_4,undefined1 param_5
               ,undefined1 param_6)

{
                    /* 0xea9c  482  CMF_ProcessDimCommon */
  CMF_ProcessDimCommonOnBuf
            (*(undefined8 *)_refptr_g_tsaBufDif,*(undefined8 *)_refptr_g_tsaBufDif,param_1,
             (int)param_2,(int)param_3,param_4,param_5,param_6);
  return;
}


// ============================================================================
// Function: CMF_Filter1DCore @ 6ba51d3a
// ============================================================================

void CMF_Filter1DCore(undefined8 param_1)

{
  char cVar1;
  
                    /* 0x11d3a  466  CMF_Filter1DCore */
  cVar1 = CMF_Filter1DBySubSlot(param_1);
  if ((cVar1 == '\0') && (_refptr_g_tsaPrmtFlashCmfPca[0xcea] != '\0')) {
    CMF_Filter1DByWholeLine(param_1);
  }
  return;
}


// ============================================================================
// Function: CMF_GetDistance @ 6ba52613
// ============================================================================

int CMF_GetDistance(int param_1,int param_2,int param_3,longlong param_4)

{
  int iVar1;
  int iVar2;
  
  iVar1 = *(short *)(param_4 + 10) * param_3 +
          *(short *)(param_4 + 6) * param_1 + *(short *)(param_4 + 8) * param_2;
  if (iVar1 < 0) {
    iVar1 = iVar1 + 0x3ff;
  }
  iVar2 = *(short *)(param_4 + 0x10) * param_3 +
          *(short *)(param_4 + 0xc) * param_1 + *(short *)(param_4 + 0xe) * param_2;
  if (iVar2 < 0) {
    iVar2 = iVar2 + 0x3ff;
  }
  return (iVar2 >> 10) * (iVar2 >> 10) + (iVar1 >> 10) * (iVar1 >> 10);
}


// ============================================================================
// Function: CMF_SetNotchToZero @ 6ba526d5
// ============================================================================

void CMF_SetNotchToZero(longlong param_1,uint param_2)

{
  int iVar1;
  uint uVar2;
  int local_c;
  
  iVar1 = NotchCorrect_GetNotchNum();
  for (local_c = 0; local_c < iVar1; local_c = local_c + 1) {
    uVar2 = (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                           (longlong)local_c * 3 + 0x105) +
            (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                           (longlong)local_c * 3 + 0x106) * (uint)_refptr_g_tsaPrmtFlash->bCols &
            0xffff;
    if (uVar2 < param_2) {
      *(undefined2 *)(param_1 + (ulonglong)uVar2 * 2) = 0;
    }
  }
  return;
}


// ============================================================================
// Function: CMF_ConditionClearCmfToPreDif @ 6ba52790
// ============================================================================

void CMF_ConditionClearCmfToPreDif(longlong param_1,byte param_2,longlong param_3,short *param_4)

{
  short *local_res20;
  byte local_11;
  int local_10;
  uint local_c;
  
  if (param_1 != 0) {
    local_c = (uint)*(ushort *)(param_3 + 0x2a) * (uint)*(ushort *)(param_3 + 0x28);
    local_res20 = param_4;
    local_10 = (uint)*(ushort *)(param_3 + 0x2a) * (uint)*(ushort *)(param_3 + 0x28);
    while (local_10 != 0) {
      local_c = local_c - (int)*local_res20;
      local_res20 = local_res20 + 1;
      local_10 = local_10 + -1;
    }
    if (((uint)((int)((uint)DAT_6bb800a8 * (uint)DAT_6bb800aa) / 6) < local_c) ||
       (_refptr_g_tsaPrpt->nSPeakPositive < (short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax))
    {
      for (local_11 = 0; local_11 < param_2; local_11 = local_11 + 1) {
        *(undefined1 *)(param_1 + (ulonglong)local_11) = 0;
      }
    }
  }
  return;
}


// ============================================================================
// Function: CMF_UpdateNeighborDistance @ 6ba5288d
// ============================================================================

void CMF_UpdateNeighborDistance
               (longlong param_1,longlong param_2,byte param_3,byte param_4,longlong param_5)

{
  ushort uVar1;
  double dVar2;
  byte local_19;
  
  for (local_19 = 0; local_19 < param_3; local_19 = local_19 + 1) {
    uVar1 = *(ushort *)
             (*(longlong *)(param_5 + 0x38) +
             (ulonglong)(ushort)(*(short *)(param_5 + 0x2c) * (ushort)param_4 + (ushort)local_19) *
             2);
    dVar2 = sqrt((double)*(int *)(param_2 + (ulonglong)local_19 * 4));
    *(short *)((ulonglong)uVar1 * 2 + param_1) = (short)(int)dVar2;
  }
  return;
}


// ============================================================================
// Function: CMF_CrossErosionWeight @ 6ba52948
// ============================================================================

void CMF_CrossErosionWeight(undefined8 param_1,undefined4 param_2,undefined4 param_3)

{
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    Image_CrossErosion(param_1,param_2,param_3,1,0);
  }
  else {
    Image_CrossErosion(param_1,param_2,param_3,0,1);
  }
  return;
}


// ============================================================================
// Function: CMF_FilterOnTxOfRx @ 6ba532b5
// ============================================================================

/* WARNING: Function: ___chkstk_ms replaced with injection: alloca_probe */

void CMF_FilterOnTxOfRx(byte param_1,byte param_2,longlong param_3)

{
  ushort uVar1;
  longlong lVar2;
  short sVar3;
  uint uVar4;
  longlong lVar5;
  undefined8 auStack_70 [5];
  int local_48 [4];
  longlong local_38;
  longlong local_30;
  uint local_28;
  uint local_24;
  uint local_20;
  byte local_19;
  
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    uVar1 = *(ushort *)(param_3 + 0x28);
  }
  else {
    uVar1 = *(ushort *)(param_3 + 0x2a);
  }
  uVar4 = (uint)uVar1;
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    local_24 = (uint)*(ushort *)(param_3 + 0x28);
  }
  else {
    local_24 = 1;
  }
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    local_28 = 1;
  }
  else {
    local_28 = (uint)*(ushort *)(param_3 + 0x28);
  }
  local_30 = (ulonglong)uVar4 - 1;
  auStack_70[0] = 0x6ba53382;
  lVar2 = -((ulonglong)uVar4 * 4 + 0x12 & 0xfffffffffffffff0);
  /* 原为 alloca：VLA 位于栈帧下方（Ghidra 写作 local_48 + 负偏移） */
  lVar5 = (longlong)__builtin_alloca((size_t)-lVar2);
  for (local_19 = 0; local_19 < uVar4; local_19 = local_19 + 1) {
    local_48[3] = local_19 * local_28 + param_1 * local_24;
    local_48[2] = local_19 * local_28 + param_2 * local_24;
    *(int *)(lVar5 + (longlong)(int)(uint)local_19 * 4) =
         (int)*(short *)(*(longlong *)(param_3 + 0x10) + (ulonglong)(uint)local_48[3] * 2) -
         (int)*(short *)(*(longlong *)(param_3 + 0x18) + (ulonglong)(uint)local_48[2] * 2);
  }
  local_38 = lVar5;
  local_20 = uVar4;
  sVar3 = CMF_Ransac(lVar5,uVar4,200,uVar4);
  for (local_19 = 0; local_19 < local_20; local_19 = local_19 + 1) {
    uVar4 = local_19 * local_28 + param_1 * local_24;
    *(short *)((ulonglong)uVar4 * 2 + *(longlong *)(param_3 + 0x20)) =
         sVar3 + *(short *)(*(longlong *)(param_3 + 0x20) + (ulonglong)uVar4 * 2);
    *(short *)((ulonglong)uVar4 * 2 + *(longlong *)(param_3 + 0x18)) =
         *(short *)(*(longlong *)(param_3 + 0x10) + (ulonglong)uVar4 * 2) - sVar3;
  }
  return;
}


// ============================================================================
// Function: CMF_NoiseCal @ 6ba4e376
// ============================================================================

void CMF_NoiseCal(undefined4 *param_1,undefined8 param_2,int param_3,int param_4,int param_5)

{
  int aiStack_418 [258];
  short local_10;
  short local_e;
  short local_c;
  byte local_9;
  
  local_c = (short)*param_1;
  for (local_9 = 1; (int)(uint)local_9 < param_3; local_9 = local_9 + 1) {
    local_e = (short)param_1[local_9];
    local_10 = local_e - local_c;
    if (param_4 < local_e) {
      aiStack_418[(int)(uint)local_9] = 4;
    }
    else if ((int)local_e < -param_4) {
      aiStack_418[(int)(uint)local_9] = 5;
    }
    else if (param_5 < local_10) {
      aiStack_418[(int)(uint)local_9] = 2;
    }
    else if ((int)local_10 < -param_5) {
      aiStack_418[(int)(uint)local_9] = 3;
    }
    else {
      aiStack_418[(int)(uint)local_9] = 1;
    }
    local_c = local_e;
  }
  CMF_InitNoiseData(param_1,param_2);
  for (local_9 = 1; (int)(uint)local_9 < param_3 + -1; local_9 = local_9 + 1) {
    if ((aiStack_418[(int)(uint)local_9] == 1) && (aiStack_418[(int)(local_9 + 1)] == 1)) {
      CMF_PushNoiseData(local_9,(int)(short)param_5);
    }
  }
  CMF_NoiseStatistics();
  return;
}


// ============================================================================
// Function: CMF_Filter1DBySubSlot @ 6ba51a1e
// ============================================================================

undefined8 CMF_Filter1DBySubSlot(longlong param_1)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x88]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x90];
  undefined4 uVar1;
  longlong lVar2;
  longlong *plVar3;
  
  plVar3 = ((longlong *)(_stk + 0x8));
  for (lVar2 = 0xb; lVar2 != 0; lVar2 = lVar2 + -1) {
    *plVar3 = 0;
    plVar3 = plVar3 + 1;
  }
  (*(undefined1 *)(_stk + 0x1a)) = _refptr_g_tsaPrmtFlashCmfPca[1];
  (*(undefined * *)(_stk + 0x20)) = _refptr_g_tsaPrmtFlashCmfPca + 0xc84;
  (*(byte *)(_stk + 0x19)) = *_refptr_g_tsaPrmtFlashCmfPca;
  (*(uint *)(_stk + 0x74)) = 0;
  (*(byte *)(_stk + 0x73)) = 0;
  do {
    if (*(byte *)(param_1 + 0x38) <= (*(byte *)(_stk + 0x73))) {
      return 1;
    }
    (*(int *)(_stk + 0x6c)) = 0;
    (*(undefined1 *)(_stk + 0x6b)) = *(undefined1 *)(param_1 + 0x39);
    if (*(char *)(param_1 + 0x39) == '\0') {
      for ((*(byte *)(_stk + 0x6a)) = (byte)(*(uint *)(_stk + 0x74));
          (uint)(*(byte *)(_stk + 0x6a)) < (*(uint *)(_stk + 0x74)) + *(int *)(param_1 + ((longlong)(int)(uint)(*(byte *)(_stk + 0x73)) + 0x1c) * 4)
          ; (*(byte *)(_stk + 0x6a)) = (*(byte *)(_stk + 0x6a)) + 1) {
        (*(int *)(_stk + 0x6c)) = (*(int *)(_stk + 0x6c)) + *(short *)(*(longlong *)(param_1 + 0x10) + (ulonglong)(*(byte *)(_stk + 0x6a)) * 2);
      }
      if ((*(int *)(_stk + 0x6c)) == *(int *)(param_1 + ((longlong)(int)(uint)(*(byte *)(_stk + 0x73)) + 0x1c) * 4)) {
        (*(undefined1 *)(_stk + 0x6b)) = 1;
      }
      else if ((*(int *)(_stk + 0x6c)) < (int)((*(byte *)(_stk + 0x19)) + 3)) {
        uVar1 = *(undefined4 *)(param_1 + ((longlong)(int)(uint)(*(byte *)(_stk + 0x73)) + 0x1c) * 4);
        for ((*(byte *)(_stk + 0x69)) = 0; (*(byte *)(_stk + 0x69)) < (byte)uVar1; (*(byte *)(_stk + 0x69)) = (*(byte *)(_stk + 0x69)) + 1) {
          *(undefined2 *)(*(longlong *)(param_1 + 0x20) + (ulonglong)((*(uint *)(_stk + 0x74)) + (*(byte *)(_stk + 0x69))) * 2) = 0;
        }
        return 0;
      }
    }
    ((longlong *)(_stk + 0x8))[0] = *(longlong *)(param_1 + 8) + (ulonglong)(*(uint *)(_stk + 0x74)) * 2;
    ((longlong *)(_stk + 0x8))[1] = *(longlong *)(param_1 + 0x10) + (ulonglong)(*(uint *)(_stk + 0x74)) * 2;
    (*(undefined8 *)(_stk + 0x28)) = *(undefined8 *)(param_1 + ((longlong)(int)(uint)(*(byte *)(_stk + 0x73)) + 0xc) * 8);
    (*(undefined1 *)(_stk + 0x18)) = (undefined1)*(undefined4 *)(param_1 + ((longlong)(int)(uint)(*(byte *)(_stk + 0x73)) + 0x1c) * 4);
    (*(undefined1 *)(_stk + 0x38)) = (*(undefined1 *)(_stk + 0x6b));
    (*(undefined8 *)(_stk + 0x40)) = *(undefined8 *)(param_1 + ((longlong)(int)(uint)(*(byte *)(_stk + 0x73)) + 8) * 8);
    (*(undefined8 *)(_stk + 0x30)) = *(undefined8 *)(param_1 + ((longlong)(int)(uint)(*(byte *)(_stk + 0x73)) + 10) * 8);
    MtDiff_MsLinearCorrectionFilterCore
              (*(longlong *)(param_1 + 0x18) + (ulonglong)(*(uint *)(_stk + 0x74)) * 2,
               (ulonglong)(*(uint *)(_stk + 0x74)) * 2 + *(longlong *)(param_1 + 0x20),0x100,((longlong *)(_stk + 0x8)));
    (*(uint *)(_stk + 0x74)) = (*(uint *)(_stk + 0x74)) + *(int *)(param_1 + ((longlong)(int)(uint)(*(byte *)(_stk + 0x73)) + 0x1c) * 4);
    (*(byte *)(_stk + 0x73)) = (*(byte *)(_stk + 0x73)) + 1;
  } while( true );
}


// ============================================================================
// Function: CMF_Filter1DByWholeLine @ 6ba51c74
// ============================================================================

void CMF_Filter1DByWholeLine(int *param_1)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x78]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x80];
  longlong lVar1;
  undefined8 *puVar2;
  
  if (*param_1 == 1) {
    puVar2 = ((undefined8 *)(_stk + 0x8));
    for (lVar1 = 0xb; lVar1 != 0; lVar1 = lVar1 + -1) {
      *puVar2 = 0;
      puVar2 = puVar2 + 1;
    }
    (*(undefined1 *)(_stk + 0x38)) = 0;
    ((undefined8 *)(_stk + 0x8))[0] = *(undefined8 *)(param_1 + 2);
    ((undefined8 *)(_stk + 0x8))[1] = *(undefined8 *)(param_1 + 4);
    (*(undefined * *)(_stk + 0x28)) = _refptr_g_tsaPrmtFlashCmfPca + 0xcec;
    (*(undefined1 *)(_stk + 0x1a)) = 0;
    (*(undefined * *)(_stk + 0x20)) = _refptr_g_tsaPrmtFlashCmfPca + 0xc84;
    (*(undefined1 *)(_stk + 0x19)) = _refptr_g_tsaPrmtFlashCmfPca[0xceb];
    (*(undefined1 *)(_stk + 0x18)) = (undefined1)(short)param_1[10];
    MtDiff_MsLinearCorrectionFilterCore
              (*(undefined8 *)(param_1 + 6),*(undefined8 *)(param_1 + 8),0x100,((undefined8 *)(_stk + 0x8)));
  }
  return;
}


// ============================================================================
// Function: CMF_Ransac @ 6ba53106
// ============================================================================

/* WARNING: Function: ___chkstk_ms replaced with injection: alloca_probe */

short CMF_Ransac(longlong param_1,uint param_2,int param_3, ...)

{
  longlong lVar1;
  uint uVar2;
  uint uVar3;
  undefined4 *puVar4;
  undefined8 auStack_70 [5];
  undefined4 *local_48;
  longlong local_40;
  int local_38;
  uint local_34;
  uint local_30;
  uint local_2c;
  uint local_28;
  uint local_24;
  int local_20;
  uint local_1c;
  
  local_1c = 0;
  local_20 = 0;
  for (local_24 = 0; local_24 < param_2; local_24 = local_24 + 1) {
    local_38 = *(int *)(param_1 + (ulonglong)local_24 * 4);
    local_28 = 0;
    for (local_2c = 0; local_2c < param_2; local_2c = local_2c + 1) {
      uVar2 = *(int *)(param_1 + (ulonglong)local_2c * 4) - local_38;
      uVar3 = (int)uVar2 >> 0x1f;
      if ((int)((uVar2 ^ uVar3) - uVar3) < param_3) {
        local_28 = local_28 + 1;
      }
    }
    if (local_1c < local_28) {
      local_1c = local_28;
      local_20 = local_38;
    }
    if ((param_2 * 2) / 3 < local_28) break;
  }
  local_30 = 0;
  local_40 = (ulonglong)param_2 - 1;
  auStack_70[0] = 0x6ba53214;
  lVar1 = -((ulonglong)param_2 * 4 + 0x12 & 0xfffffffffffffff0);
  /* 原为 alloca：VLA 位于栈帧下方（Ghidra 写作 &local_48 + 负偏移） */
  puVar4 = (undefined4 *)__builtin_alloca((size_t)-lVar1);
  *puVar4 = 0;
  for (local_34 = 0; uVar2 = local_30, local_34 < param_2; local_34 = local_34 + 1) {
    uVar2 = *(int *)(param_1 + (ulonglong)local_34 * 4) - local_20;
    uVar3 = (int)uVar2 >> 0x1f;
    if ((int)((uVar2 ^ uVar3) - uVar3) < param_3) {
      puVar4[local_30] = *(undefined4 *)(param_1 + (ulonglong)local_34 * 4);
      local_30 = local_30 + 1;
    }
  }
  uVar3 = local_30 + 1;
  local_48 = puVar4;
  return (short)Misc_FindKthSmallest(puVar4,uVar2,uVar3 >> 1,param_2);
}


// ============================================================================
// Function: CMF_InitNoiseData @ 6ba4d9e0
// ============================================================================

void CMF_InitNoiseData(undefined8 param_1,undefined8 param_2)

{
  int iVar1;
  
  iVar1 = memset_s(&g_cmfNoise,0xc0,0,0xc0);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/CMF/CMF.c",0x83,
                      "rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  DAT_6bb7f130 = param_1;
  DAT_6bb7f138 = param_2;
  g_cmfNoise = 0xffff;
  return;
}


// ============================================================================
// Function: CMF_PushNoiseData @ 6ba4da6a
// ============================================================================

void CMF_PushNoiseData(ushort param_1,short param_2)

{
  int iVar1;
  bool bVar2;
  short sVar3;
  byte local_e;
  uint local_c;
  
  iVar1 = *(int *)(DAT_6bb7f130 + (ulonglong)param_1 * 4);
  local_c = 1;
  if (DAT_6bb7f138 != 0) {
    local_c = (uint)*(byte *)((ulonglong)param_1 + DAT_6bb7f138);
  }
  if ((uint)param_1 == g_cmfNoise + 1) {
    *(uint *)(&DAT_6bb7f084 + (longlong)(int)(uint)DAT_6bb7f082 * 0x10) =
         *(int *)(&DAT_6bb7f084 + (longlong)(int)(uint)DAT_6bb7f082 * 0x10) + iVar1 * local_c;
    (&DAT_6bb7f088)[(longlong)(int)(uint)DAT_6bb7f082 * 0x10] =
         (&DAT_6bb7f088)[(longlong)(int)(uint)DAT_6bb7f082 * 0x10] + '\x01';
    *(int *)(&DAT_6bb7f08c + (longlong)(int)(uint)DAT_6bb7f082 * 0x10) = iVar1;
    *(uint *)(&DAT_6bb7f090 + (longlong)(int)(uint)DAT_6bb7f082 * 0x10) =
         *(int *)(&DAT_6bb7f090 + (longlong)(int)(uint)DAT_6bb7f082 * 0x10) + local_c;
  }
  else {
    bVar2 = false;
    for (local_e = 0; local_e < DAT_6bb7f083; local_e = local_e + 1) {
      sVar3 = (short)iVar1 -
              (short)*(undefined4 *)(&DAT_6bb7f08c + (longlong)(int)(uint)local_e * 0x10);
      if ((sVar3 < param_2) && (-(int)param_2 < (int)sVar3)) {
        DAT_6bb7f082 = local_e;
        *(uint *)(&DAT_6bb7f084 + (longlong)(int)(uint)local_e * 0x10) =
             *(int *)(&DAT_6bb7f084 + (longlong)(int)(uint)local_e * 0x10) + iVar1 * local_c;
        (&DAT_6bb7f088)[(longlong)(int)(uint)local_e * 0x10] =
             (&DAT_6bb7f088)[(longlong)(int)(uint)local_e * 0x10] + '\x01';
        *(int *)(&DAT_6bb7f08c + (longlong)(int)(uint)local_e * 0x10) = iVar1;
        *(uint *)(&DAT_6bb7f090 + (longlong)(int)(uint)DAT_6bb7f082 * 0x10) =
             *(int *)(&DAT_6bb7f090 + (longlong)(int)(uint)DAT_6bb7f082 * 0x10) + local_c;
        bVar2 = true;
        break;
      }
    }
    if (!bVar2) {
      if (DAT_6bb7f083 < 10) {
        DAT_6bb7f082 = DAT_6bb7f083;
        *(uint *)(&DAT_6bb7f084 + (longlong)(int)(uint)DAT_6bb7f083 * 0x10) = iVar1 * local_c;
        (&DAT_6bb7f088)[(longlong)(int)(uint)DAT_6bb7f082 * 0x10] = 1;
        *(int *)(&DAT_6bb7f08c + (longlong)(int)(uint)DAT_6bb7f082 * 0x10) = iVar1;
        *(uint *)(&DAT_6bb7f090 + (longlong)(int)(uint)DAT_6bb7f082 * 0x10) = local_c;
        DAT_6bb7f083 = DAT_6bb7f083 + 1;
      }
      else {
        *(uint *)(&DAT_6bb7f084 + (longlong)(int)(uint)DAT_6bb7f082 * 0x10) =
             *(int *)(&DAT_6bb7f084 + (longlong)(int)(uint)DAT_6bb7f082 * 0x10) + iVar1 * local_c;
        (&DAT_6bb7f088)[(longlong)(int)(uint)DAT_6bb7f082 * 0x10] =
             (&DAT_6bb7f088)[(longlong)(int)(uint)DAT_6bb7f082 * 0x10] + '\x01';
        *(int *)(&DAT_6bb7f08c + (longlong)(int)(uint)DAT_6bb7f082 * 0x10) = iVar1;
        *(uint *)(&DAT_6bb7f090 + (longlong)(int)(uint)DAT_6bb7f082 * 0x10) =
             *(int *)(&DAT_6bb7f090 + (longlong)(int)(uint)DAT_6bb7f082 * 0x10) + local_c;
      }
    }
  }
  g_cmfNoise = param_1;
  return;
}


// ============================================================================
// Function: CMF_NoiseStatistics @ 6ba4deb4
// ============================================================================

void CMF_NoiseStatistics(void)

{
  byte bVar1;
  short sVar2;
  uint uVar3;
  uint uVar4;
  int local_18;
  short local_10;
  short local_c;
  byte local_9;
  
  local_c = 0x7fff;
  bVar1 = 0;
  local_10 = 0x7fff;
  DAT_6bb7f124 = 0;
  if (DAT_6bb7f138 == 0) {
    for (local_9 = 0; local_9 < DAT_6bb7f083; local_9 = local_9 + 1) {
      sVar2 = (short)(*(int *)(&DAT_6bb7f084 + (longlong)(int)(uint)local_9 * 0x10) /
                     (int)(uint)(byte)(&DAT_6bb7f088)[(longlong)(int)(uint)local_9 * 0x10]);
      if (sVar2 < local_c) {
        bVar1 = (&DAT_6bb7f088)[(longlong)(int)(uint)local_9 * 0x10];
        local_c = sVar2;
      }
      uVar3 = (int)local_10 >> 0x1f;
      uVar4 = (int)sVar2 >> 0x1f;
      if ((int)(((int)sVar2 ^ uVar4) - uVar4) < (int)(((int)local_10 ^ uVar3) - uVar3)) {
        DAT_6bb7f124 = (&DAT_6bb7f088)[(longlong)(int)(uint)local_9 * 0x10];
        local_10 = sVar2;
      }
    }
    if (DAT_6bb7f083 == 0) {
      DAT_6bb7f128 = 0;
      DAT_6bb7f124 = 0;
    }
    else if ((DAT_6bb7f124 < bVar1) || (0 < local_c)) {
      DAT_6bb7f128 = (int)local_c;
      DAT_6bb7f124 = bVar1;
    }
    else {
      DAT_6bb7f128 = (int)local_10;
    }
  }
  else {
    local_18 = -0x80000000;
    DAT_6bb7f128 = 0;
    DAT_6bb7f124 = 0;
    for (local_9 = 0; local_9 < DAT_6bb7f083; local_9 = local_9 + 1) {
      if (local_18 < *(int *)(&DAT_6bb7f090 + (longlong)(int)(uint)local_9 * 0x10)) {
        DAT_6bb7f128 = (int)(short)(*(int *)(&DAT_6bb7f084 + (longlong)(int)(uint)local_9 * 0x10) /
                                   *(int *)(&DAT_6bb7f090 + (longlong)(int)(uint)local_9 * 0x10));
        DAT_6bb7f124 = (&DAT_6bb7f088)[(longlong)(int)(uint)local_9 * 0x10];
        local_18 = *(int *)(&DAT_6bb7f090 + (longlong)(int)(uint)local_9 * 0x10);
      }
    }
  }
  return;
}


// ============================================================================
// Function: CMF_InitWeight @ 6ba5213d
// ============================================================================

void CMF_InitWeight(short param_1,short param_2,longlong param_3)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  short local_428 [256];
  short local_228 [256];
  uint local_28;
  byte local_23;
  byte local_22;
  byte local_21;
  longlong local_20;
  longlong local_18;
  uint local_10;
  byte local_a;
  byte local_9;
  
                    /* 0x1213d  476  CMF_InitWeight */
  local_18 = *(longlong *)(param_3 + 0x10);
  local_20 = *(longlong *)(param_3 + 0x40);
  local_21 = (byte)*(undefined2 *)(param_3 + 0x28);
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    local_22 = (byte)*(undefined2 *)(param_3 + 0x2a);
  }
  else {
    local_22 = (byte)*(undefined2 *)(param_3 + 0x28);
  }
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    local_23 = (byte)*(undefined2 *)(param_3 + 0x28);
  }
  else {
    local_23 = (byte)*(undefined2 *)(param_3 + 0x2a);
  }
  local_10 = (uint)local_23 * (uint)local_22;
  while (uVar1 = local_10 - 1, local_10 != 0) {
    *(undefined2 *)(local_20 + (ulonglong)uVar1 * 2) = 1;
    local_10 = uVar1;
  }
  for (local_a = 0; local_a < local_23; local_a = local_a + 1) {
    local_228[0] = 0;
    for (local_9 = 1; local_9 < local_22; local_9 = local_9 + 1) {
      if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
        iVar3 = (uint)local_21 * (uint)local_9 + (uint)local_a;
      }
      else {
        iVar3 = (uint)local_21 * (uint)local_a + (uint)local_9;
      }
      if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
        iVar2 = (uint)local_21 * (local_9 - 1) + (uint)local_a;
      }
      else {
        iVar2 = (uint)local_21 * (uint)local_a + (local_9 - 1);
      }
      local_228[(int)(uint)local_9] =
           *(short *)((longlong)iVar3 * 2 + local_18) - *(short *)((longlong)iVar2 * 2 + local_18);
    }
    if (local_22 != 0) {
      local_428[(int)(local_22 - 1)] = 0;
      local_428[0] = local_428[(int)(local_22 - 1)];
      for (local_9 = 1; (int)(uint)local_9 < (int)(local_22 - 1); local_9 = local_9 + 1) {
        local_428[(int)(uint)local_9] =
             local_228[(int)(local_9 + 1)] - local_228[(int)(uint)local_9];
      }
      for (local_9 = 0; local_9 < local_22; local_9 = local_9 + 1) {
        if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
          iVar3 = (uint)local_21 * (uint)local_9 + (uint)local_a;
        }
        else {
          iVar3 = (uint)local_21 * (uint)local_a + (uint)local_9;
        }
        local_28 = (uint)*(short *)((longlong)iVar3 * 2 + local_18);
        if (((int)param_1 < (int)(((int)local_28 >> 0x1f ^ local_28) - ((int)local_28 >> 0x1f))) ||
           (uVar4 = (int)local_428[(int)(uint)local_9] >> 0x1f,
           (int)param_2 < (int)(((int)local_428[(int)(uint)local_9] ^ uVar4) - uVar4))) {
          if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
            iVar3 = (uint)local_21 * (uint)local_9 + (uint)local_a;
          }
          else {
            iVar3 = (uint)local_21 * (uint)local_a + (uint)local_9;
          }
          *(undefined2 *)((longlong)iVar3 * 2 + local_20) = 0;
        }
      }
    }
  }
  local_10 = uVar1;
  Image_Erosion(local_20,*(undefined2 *)(param_3 + 0x28),*(undefined2 *)(param_3 + 0x2a),1,1);
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    Image_Dilation(local_20,*(undefined2 *)(param_3 + 0x28),*(undefined2 *)(param_3 + 0x2a),0,1);
  }
  else {
    Image_Dilation(local_20,*(undefined2 *)(param_3 + 0x28),*(undefined2 *)(param_3 + 0x2a),1,0);
  }
  return;
}


// ============================================================================
// Function: CMF_Free @ 6ba4d8a8
// ============================================================================

void CMF_Free(void)

{
                    /* 0xd8a8  470  CMF_Free */
  CMFRemap_Free();
  CMFPca_Free();
  return;
}


// ============================================================================
// Function: CMF_Malloc @ 6ba4d880
// ============================================================================

bool CMF_Malloc(void)

{
  char cVar1;
  
                    /* 0xd880  477  CMF_Malloc */
  CMFPca_Malloc();
  cVar1 = CMFRemap_Malloc();
  return cVar1 != '\0';
}


// ============================================================================
// Function: CMF_EnablePreCMFDiffLog @ 6ba4d94e
// ============================================================================

void CMF_EnablePreCMFDiffLog(undefined1 param_1)

{
                    /* 0xd94e  465  CMF_EnablePreCMFDiffLog */
  g_preCMFDiffLogEnabled = param_1;
  return;
}


// ============================================================================
// Function: CMF_GetNoiseValueFromBuf @ 6ba4d960
// ============================================================================

short CMF_GetNoiseValueFromBuf()

{
                    /* 0xd960  473  CMF_GetNoiseValueFromBuf */
  return 0;
}


