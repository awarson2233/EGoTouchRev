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


