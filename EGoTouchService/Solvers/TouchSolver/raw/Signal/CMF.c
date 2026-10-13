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
  char cVar1;
  undefined1 local_25;
  short local_24;
  short local_22;
  undefined4 local_20;
  int local_1c;
  undefined8 local_18;
  uint local_10;
  byte local_a;
  byte local_9;
  
                    /* 0xe8b5  481  CMF_ProcessDim */
  *(undefined2 *)&_refptr_g_tsaPrpt[1].field_0x44 = 0x7fff;
  *(undefined2 *)&_refptr_g_tsaPrpt[1].field_0x46 = 0x8000;
  local_a = param_4;
  if (param_1 == 1) {
    local_a = param_5;
  }
  local_10 = 0;
  if (param_1 == 1) {
    local_10 = (uint)param_4;
  }
  local_18 = 0;
  cVar1 = SS_IsValidForCMF(param_1);
  if (cVar1 != '\0') {
    local_18 = SS_DiffRelatCal(local_a,local_10);
  }
  local_1c = 0;
  cVar1 = TouchThold_ToeUseSpecialThold();
  if (cVar1 != '\0') {
    local_1c = 1;
  }
  for (local_9 = 0; local_9 < local_a; local_9 = local_9 + 1) {
    local_20 = 0;
    cVar1 = SS_IsValidForCMF(param_1);
    if (cVar1 != '\0') {
      local_20 = SS_DiffCMFProcessDimUnit(param_1,param_4,param_5,local_9,local_18);
    }
    if ((local_1c == 0) || ((1 < local_9 && ((int)(uint)local_9 < (int)(local_a - 2))))) {
      local_22 = param_2;
    }
    else {
      local_22 = 200;
    }
    local_24 = CMF_ProcessDimUnit(param_1,local_9,(int)local_22,(int)param_3,&local_25,param_4,
                                  param_5);
    local_24 = local_24 + (short)local_20;
    CMF_CopyCommonModeNoise(param_1,local_9,param_4,param_5,(int)local_24);
    if (local_24 < *(short *)&_refptr_g_tsaPrpt[1].field_0x44) {
      *(short *)&_refptr_g_tsaPrpt[1].field_0x44 = local_24;
    }
    if (*(short *)&_refptr_g_tsaPrpt[1].field_0x46 < local_24) {
      *(short *)&_refptr_g_tsaPrpt[1].field_0x46 = local_24;
    }
  }
  return;
}


// ============================================================================
// Function: CMF_ProcessDimCommonNoise @ 6ba4ebf3
// ============================================================================

void CMF_ProcessDimCommonNoise(int param_1,short param_2,short param_3,byte param_4,byte param_5)

{
  short sVar1;
  undefined1 local_11;
  int local_10;
  byte local_b;
  byte local_a;
  byte local_9;
  
                    /* 0xebf3  483  CMF_ProcessDimCommonNoise */
  local_b = 0;
  local_a = param_4;
  if (param_1 == 1) {
    local_b = param_4;
    local_a = param_5;
  }
  for (local_9 = 0; local_9 < local_a; local_9 = local_9 + 1) {
    sVar1 = CMF_ProcessDimUnit(param_1,local_9,(int)param_2,(int)param_3,&local_11,param_4,param_5);
    local_10 = (int)sVar1;
    *(short *)(_refptr_g_tsaBufSelf + ((longlong)(int)((uint)local_b + (uint)local_9) + 0xb40) * 2)
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
  longlong lVar1;
  longlong *plVar2;
  undefined1 local_4d38 [9808];
  undefined1 local_26e8 [9808];
  longlong local_98 [5];
  ushort local_70;
  ushort local_6e;
  ushort local_6c;
  ushort local_6a;
  undefined1 *local_68;
  undefined1 *local_60;
  ushort local_2c;
  ushort local_2a;
  FILE *local_28;
  int local_20;
  int local_1c;
  undefined8 uStack_18;
  
  uStack_18 = 0x6ba4f0d5;
  if (g_preCMFDiffLogEnabled != '\0') {
    plVar2 = local_98;
    for (lVar1 = 0xc; lVar1 != 0; lVar1 = lVar1 + -1) {
      *plVar2 = 0;
      plVar2 = plVar2 + 1;
    }
    local_70 = (ushort)_refptr_g_tsaPrmtFlash->bCols;
    local_6e = (ushort)_refptr_g_tsaPrmtFlash->bRows;
    local_60 = local_26e8;
    local_68 = local_4d38;
    local_98[2] = *(longlong *)_refptr_g_tsaBufDif;
    CMFRemap_InitMapIdxBySlotTable
              (&_refptr_g_tsaPrmtFlash->field_0x67c,_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass,
               _refptr_g_tsaPrmtFlash->field_0x6cc,local_98);
    local_28 = fopen("preCMFDiff.csv","a");
    for (local_1c = 0; local_1c < (int)(uint)local_6a; local_1c = local_1c + 1) {
      for (local_20 = 0; local_20 < (int)(uint)local_6c; local_20 = local_20 + 1) {
        if (local_20 != 0) {
          fputc(0x2c,local_28);
        }
        local_2a = local_6c * (short)local_1c + (short)local_20;
        local_2c = *(ushort *)(local_60 + (ulonglong)local_2a * 2);
        fprintf(local_28,"%d",
                (ulonglong)(uint)(int)*(short *)(local_98[2] + (ulonglong)local_2c * 2));
      }
      fputc(10,local_28);
    }
    fclose(local_28);
  }
  return;
}


// ============================================================================
// Function: CMF_Filter2D @ 6ba54009
// ============================================================================

void CMF_Filter2D(void)

{
  longlong lVar1;
  undefined **ppuVar2;
  undefined8 local_68;
  undefined8 local_60;
  undefined *local_58 [4];
  undefined *local_38;
  undefined8 local_30;
  undefined2 local_1a;
  
                    /* 0x14009  467  CMF_Filter2D */
  ppuVar2 = local_58;
  for (lVar1 = 6; lVar1 != 0; lVar1 = lVar1 + -1) {
    *ppuVar2 = (undefined *)0x0;
    ppuVar2 = ppuVar2 + 1;
  }
  local_1a = *(undefined2 *)(_refptr_g_tsaPrmtFlashCmfPca + 0xce8);
  local_58[1] = _refptr_g_tsaBufCmfDraftNeighborDistance;
  local_58[0] = _refptr_g_tsaBufCmfDraftWeight;
  local_58[3] = _refptr_g_tsaBufCmfNeighborDistance;
  local_58[2] = _refptr_g_tsaBufCmfWeight;
  local_38 = _refptr_g_tsaBufAftPcaFilterDif;
  local_30 = *(undefined8 *)_refptr_g_tsaBufCommonModeNoise;
  local_68 = *(undefined8 *)_refptr_g_tsaBufDif;
  local_60 = *(undefined8 *)_refptr_g_tsaBufDif;
  CMF_Filter2DCommonInternal(&local_68,local_1a,local_58,2);
  return;
}


// ============================================================================
// Function: CMF_Init2DFilter @ 6ba51699
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void CMF_Init2DFilter(void)

{
  byte bVar1;
  undefined1 local_a8 [16];
  undefined1 local_98;
  byte local_97;
  undefined8 local_88;
  int local_48 [5];
  undefined1 local_34 [4];
  undefined *local_30;
  uint local_28;
  int local_24;
  uint local_20;
  uint local_1c;
  ushort local_16;
  ushort local_14;
  byte local_12;
  byte local_11;
  int local_10;
  byte local_9;
  
                    /* 0x11699  475  CMF_Init2DFilter */
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    bVar1 = _refptr_g_tsaPrmtFlash->bRows;
  }
  else {
    bVar1 = _refptr_g_tsaPrmtFlash->bCols;
  }
  local_14 = (ushort)bVar1;
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    bVar1 = _refptr_g_tsaPrmtFlash->bCols;
  }
  else {
    bVar1 = _refptr_g_tsaPrmtFlash->bRows;
  }
  local_16 = (ushort)bVar1;
  local_48[0] = 0;
  local_48[1] = 0;
  local_1c = CMFRemap_GetRxSlotNum
                       (&_refptr_g_tsaPrmtFlash->field_0x67c,local_14,local_34,local_48,2);
  local_20 = CMFRemap_GetTxSlotNum(&_refptr_g_tsaPrmtFlash->field_0x62c,local_16);
  local_97 = *_refptr_g_tsaPrmtFlashCmfPca;
  for (local_9 = 0; local_9 < local_20; local_9 = local_9 + 1) {
    local_10 = 0;
    for (local_11 = 0; local_11 < local_1c; local_11 = local_11 + 1) {
      local_24 = local_48[(int)(uint)local_11];
      local_28 = (uint)local_97 * ((uint)local_14 * (uint)local_9 + local_10);
      *(undefined **)
       (&g_pcaCoef + ((longlong)(int)(uint)local_11 + (longlong)(int)(uint)local_9 * 2) * 8) =
           _refptr_g_tsaPrmtFlashCmfPca + (ulonglong)local_28 * 4 + 4;
      local_88 = *(undefined8 *)
                  (&g_pcaCoef +
                  ((longlong)(int)(uint)local_11 + (longlong)(int)(uint)local_9 * 2) * 8);
      local_98 = (undefined1)local_24;
      MtLsm_CalcMatrixB(&g_pcaMatrixB +
                        (longlong)(int)(uint)local_11 * 800 + (longlong)(int)(uint)local_9 * 0x640,
                        &g_pcaCoefSqSum +
                        (longlong)(int)(uint)local_11 * 100 + (longlong)(int)(uint)local_9 * 200,400
                        ,local_a8);
      local_10 = local_10 + local_24;
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
  for (local_12 = 1; (int)(uint)local_12 < (int)(local_14 - 1); local_12 = local_12 + 1) {
    local_30 = &g_cmfNeighborCoefArray + (longlong)(int)((local_12 - 1) * 9) * 2;
    CMF_GetNeighborCoef(local_30,local_12 - 1,local_12,local_12 + 1);
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
  int iVar1;
  undefined1 local_b;
  byte local_a;
  byte local_9;
  
                    /* 0xeb1d  484  CMF_ProcessDimCommonOnBuf */
  iVar1 = CMF_GetCmfMethod();
  if (iVar1 == 1) {
    CMF_Filter2DCommon(param_1,param_2,param_8);
  }
  else {
    local_a = param_6;
    if (param_3 == 1) {
      local_a = param_7;
    }
    for (local_9 = 0; local_9 < local_a; local_9 = local_9 + 1) {
      CMF_ProcessDimUnitOnBuf
                (param_1,param_2,param_3,local_9,(int)param_4,(int)param_5,&local_b,param_6,param_7)
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
  char cVar1;
  uint uVar2;
  longlong lVar3;
  undefined8 *puVar4;
  bool bVar5;
  undefined8 in_stack_ffffffffffff65e8;
  undefined4 uVar7;
  undefined8 uVar6;
  undefined8 local_9a08 [10];
  undefined8 local_99b8 [10];
  undefined1 local_9968 [9808];
  undefined1 local_7318 [9808];
  undefined2 local_4cc8 [4904];
  undefined1 local_2678 [9808];
  int local_28;
  errno_t local_24;
  int local_20;
  uint local_1c;
  undefined8 uStack_18;
  
  uVar7 = (undefined4)((ulonglong)in_stack_ffffffffffff65e8 >> 0x20);
  uStack_18 = 0x6ba53ae5;
  memset(local_4cc8,0,0x2648);
  local_24 = memcpy_s(local_9968,0x2648,(void *)param_1[1],
                      (ulonglong)DAT_6bb800aa * (ulonglong)DAT_6bb800a8 * 2);
  if (local_24 != 0) {
    uVar7 = 0;
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/CMF/CMF_Pca.c",
                      0x2e0,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  if (param_3 == (longlong *)0x0) {
    DAT_6bb800c0 = local_2678;
    _DAT_6bb800c8 = 0;
    DAT_6bb800a0 = local_7318;
    DAT_6bb80098 = *param_1;
  }
  else {
    DAT_6bb800c0 = (undefined1 *)*param_3;
    if (DAT_6bb800c0 == (undefined1 *)0x0) {
      DAT_6bb800c0 = local_2678;
    }
    _DAT_6bb800c8 = param_3[1];
    DAT_6bb800a0 = (undefined1 *)param_3[5];
    DAT_6bb80098 = param_3[4];
    if (DAT_6bb80098 == 0) {
      DAT_6bb80098 = *param_1;
    }
  }
  _DAT_6bb800d0 = &_refptr_g_tsaPrmtFlash->field_0x67c;
  _DAT_6bb80090 = local_9968;
  puVar4 = local_99b8;
  for (lVar3 = 10; lVar3 != 0; lVar3 = lVar3 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  if (param_4 == 1) {
    uVar7 = 0;
    CMF_InitWeightByNeighbor(param_2,&g_param,0,0,0,0);
  }
  else if (param_4 == 2) {
    puVar4 = local_99b8;
    CMF_InitWeightByNeighbor(param_2,&g_param,1,0,local_99b8,0x50);
    uVar7 = (undefined4)((ulonglong)puVar4 >> 0x20);
    local_1c = (uint)DAT_6bb800aa * (uint)DAT_6bb800a8;
    while (uVar2 = local_1c - 1, bVar5 = local_1c != 0, local_1c = uVar2, bVar5) {
      local_4cc8[uVar2] = *(undefined2 *)(DAT_6bb800c0 + (ulonglong)uVar2 * 2);
    }
  }
  uVar6 = CONCAT44(uVar7,0x1324);
  local_28 = CMF_SubtractPreDif(local_99b8,0x50,local_4cc8,local_9968,uVar6);
  uVar7 = (undefined4)((ulonglong)uVar6 >> 0x20);
  if (local_28 != 0) {
    if (param_3 != (longlong *)0x0) {
      DAT_6bb800c0 = (undefined1 *)param_3[2];
      if (DAT_6bb800c0 == (undefined1 *)0x0) {
        DAT_6bb800c0 = local_2678;
      }
      _DAT_6bb800c8 = param_3[3];
    }
    puVar4 = local_9a08;
    for (lVar3 = 10; lVar3 != 0; lVar3 = lVar3 + -1) {
      *puVar4 = 0;
      puVar4 = puVar4 + 1;
    }
    puVar4 = local_9a08;
    CMF_InitWeightByNeighbor(param_2,&g_param,0,local_99b8,puVar4,0x50);
    uVar7 = (undefined4)((ulonglong)puVar4 >> 0x20);
  }
  _DAT_6bb80090 = local_9968;
  DAT_6bb80084 = param_4 == 0;
  CMF_Filter2DCore(&g_param);
  CMF_AddPreDif(local_99b8,0x50,local_4cc8,DAT_6bb80098,CONCAT44(uVar7,0x1324));
  local_20 = (int)((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 3);
  if (param_4 == 2) {
    uVar2 = CMF_GetAverage(DAT_6bb800a0,(uint)DAT_6bb800a8 * (uint)DAT_6bb800aa);
    local_20 = (int)((((int)uVar2 >> 0x1f ^ uVar2) - ((int)uVar2 >> 0x1f)) * 0x19) / 100;
    if (_refptr_g_tsaPrmtFlash[2].field_0x7b4 != '\0') {
      cVar1 = BaliSynaFM_IsFastMoveDetected();
      if (cVar1 != '\0') {
        local_20 = 600;
      }
    }
  }
  _DAT_6bb80090 = (undefined1 *)DAT_6bb80098;
  DAT_6bb80098 = *param_1;
  CMF_FilterOnTx((int)(short)local_20,&g_param);
  return;
}


// ============================================================================
// Function: CMF_GetNeighborCoef @ 6ba511ea
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void CMF_GetNeighborCoef(longlong param_1,byte param_2,undefined1 param_3,undefined1 param_4)

{
  uint uVar1;
  double dVar2;
  undefined1 local_278 [112];
  float local_208 [25];
  byte local_1a3;
  undefined1 local_198 [112];
  float local_128 [25];
  byte local_c3;
  byte local_b8 [16];
  undefined4 local_a8 [25];
  byte local_43;
  int local_34;
  uint local_30;
  uint local_2c;
  uint local_28;
  uint local_24;
  uint local_20;
  uint local_1c;
  
  local_30 = (uint)(byte)*_refptr_g_tsaPrmtFlashCmfPca;
  local_b8[0] = param_2;
  local_b8[1] = param_3;
  local_b8[2] = param_4;
  if ((double)*(float *)(_refptr_g_tsaPrmtFlashCmfPca + ((ulonglong)(local_30 - 1) + 800) * 4 + 4)
      <= _DAT_6bb51d50) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/CMF/CMF_Pca.c",
                      0x4a,"TSAPrmtCmfPca(cmfPca, cmfPcaConstrain)[pcaDegree - 1] > 1e-12");
    HardwareAnalyzer_SetSoftException();
  }
  Matrix_ReShape(local_a8,3,local_30 & 0xff);
  for (local_1c = 0; local_1c < 3; local_1c = local_1c + 1) {
    local_34 = local_b8[local_1c] * local_30;
    for (local_20 = 0; local_20 < local_30; local_20 = local_20 + 1) {
      local_a8[local_43 * local_1c + local_20] =
           *(undefined4 *)(_refptr_g_tsaPrmtFlashCmfPca + (ulonglong)(local_34 + local_20) * 4 + 4);
    }
  }
  Matrix_Init(local_128,local_30 & 0xff,local_30 & 0xff,0);
  for (local_24 = 0; local_24 < local_30; local_24 = local_24 + 1) {
    uVar1 = (local_c3 + 1) * local_24;
    dVar2 = sqrt((double)(*(float *)(_refptr_g_tsaPrmtFlashCmfPca +
                                    ((ulonglong)local_24 + 800) * 4 + 4) * DAT_6bb51d58));
    local_128[uVar1] = (float)(1.0 / dVar2);
  }
  Matrix_Mul(local_a8,local_a8,local_128);
  Matrix_Rotate(local_198,local_a8);
  Matrix_Mul(local_a8,local_a8,local_198);
  Matrix_Eig(local_a8,local_208,local_278);
  Matrix_Rotate(local_208,local_208);
  for (local_28 = 0; local_28 < 3; local_28 = local_28 + 1) {
    local_208[(int)((uint)local_1a3 + (uint)local_1a3)] =
         local_208[1] * local_208[(int)(local_1a3 + 2)] -
         local_208[(int)(local_1a3 + 1)] * local_208[2];
    local_208[(int)((uint)local_1a3 * 2 + 1)] =
         local_208[(int)(uint)local_1a3] * local_208[2] -
         local_208[0] * local_208[(int)(local_1a3 + 2)];
    local_208[(int)((local_1a3 + 1) * 2)] =
         local_208[0] * local_208[(int)(local_1a3 + 1)] -
         local_208[(int)(uint)local_1a3] * local_208[1];
  }
  for (local_2c = 0; local_2c < 9; local_2c = local_2c + 1) {
    *(short *)((ulonglong)local_2c * 2 + param_1) = (short)(int)(local_208[local_2c] * DAT_6bb51d5c)
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
  undefined4 uVar1;
  int local_418 [258];
  byte local_f;
  byte local_e;
  byte local_d;
  ushort local_c;
  ushort local_a;
  
                    /* 0xe0ec  486  CMF_ProcessDimUnitOnBuf */
  local_e = param_9;
  if (param_3 == 1) {
    local_a = (ushort)param_8 * (ushort)param_4;
    local_d = 1;
    local_e = param_8;
  }
  else {
    local_a = (ushort)param_4;
    local_d = param_8;
  }
  local_c = local_a;
  for (local_f = 0; local_f < local_e; local_f = local_f + 1) {
    local_418[(int)(uint)local_f] = (int)*(short *)(param_2 + (ulonglong)local_c * 2);
    local_c = local_c + local_d;
  }
  uVar1 = CMF_ProcessCore(local_418,0x100,local_e,(int)param_5,(int)param_6,param_7);
  local_c = local_a;
  for (local_f = 0; local_f < local_e; local_f = local_f + 1) {
    *(short *)((ulonglong)local_c * 2 + param_1) = (short)local_418[(int)(uint)local_f];
    local_c = local_c + local_d;
  }
  return uVar1;
}


// ============================================================================
// Function: CMF_Filter2DCommon @ 6ba53f96
// ============================================================================

void CMF_Filter2DCommon(undefined8 param_1,undefined8 param_2,char param_3)

{
  undefined8 local_28;
  undefined8 local_20;
  uint local_c;
  
                    /* 0x13f96  468  CMF_Filter2DCommon */
  local_c = (uint)(param_3 != '\0');
  local_28 = param_1;
  local_20 = param_2;
  CMF_Filter2DCommonInternal
            (&local_28,*(undefined2 *)(_refptr_g_tsaPrmtFlashCmfPca + 0xce8),0,local_c);
  return;
}


// ============================================================================
// Function: CMF_Filter2DCore @ 6ba51d77
// ============================================================================

/* WARNING: Function: ___chkstk_ms replaced with injection: alloca_probe */

void CMF_Filter2DCore(undefined4 *param_1)

{
  longlong lVar1;
  undefined8 *puVar2;
  undefined8 in_stack_ffffffffffff8c48;
  undefined4 uVar3;
  undefined1 local_739c [4];
  undefined8 local_7398;
  undefined1 *local_7390;
  undefined1 *local_7388;
  undefined1 *local_7380;
  undefined1 *local_7378;
  undefined2 local_7370;
  undefined8 local_7368;
  byte local_7360;
  undefined1 local_735f;
  undefined8 local_7358 [6];
  undefined1 local_7328 [16];
  undefined1 local_7318 [9808];
  undefined1 local_4cc8 [9808];
  undefined1 local_2678 [9812];
  uint local_24;
  byte local_1e;
  byte local_1d;
  uint local_1c;
  undefined8 uStack_18;
  
                    /* 0x11d77  469  CMF_Filter2DCore */
  uVar3 = (undefined4)((ulonglong)in_stack_ffffffffffff8c48 >> 0x20);
  uStack_18 = 0x6ba51d83;
  CMFRemap_MapBuffer(local_2678,0x1324,*(undefined8 *)(param_1 + 4),param_1);
  if (*(char *)(param_1 + 1) == '\0') {
    CMFRemap_MapBuffer(local_4cc8,0x1324,*(undefined8 *)(param_1 + 0x10),param_1);
  }
  puVar2 = &local_7398;
  for (lVar1 = 0xf; lVar1 != 0; lVar1 = lVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  local_735f = *(undefined1 *)(param_1 + 1);
  local_7398._0_4_ = *param_1;
  local_7398._4_4_ = *(undefined4 *)((longlong)param_1 + 6);
  local_7368 = *(undefined8 *)(param_1 + 0x14);
  local_7360 = CMFRemap_GetRxSlotNum
                         (local_7368,*(undefined2 *)(param_1 + 0xb),local_739c,local_7328,
                          CONCAT44(uVar3,2));
  for (local_1c = 0; local_1c < *(ushort *)((longlong)param_1 + 0x2e); local_1c = local_1c + 1) {
    local_1e = (&_refptr_g_tsaPrmtFlash->field_0x62c)[local_1c];
    for (local_1d = 0; local_1d < local_7360; local_1d = local_1d + 1) {
      (&local_7398)[(longlong)(int)(uint)local_1d + 8] =
           &g_pcaMatrixB +
           (longlong)(int)(uint)local_1d * 800 + (longlong)(int)(uint)local_1e * 0x640;
      (&local_7398)[(longlong)(int)(uint)local_1d + 0xc] =
           *(undefined8 *)
            (&g_pcaCoef + ((longlong)(int)(uint)local_1d + (longlong)(int)(uint)local_1e * 2) * 8);
      (&local_7398)[(longlong)(int)(uint)local_1d + 10] =
           &g_pcaCoefSqSum +
           (longlong)(int)(uint)local_1d * 100 + (longlong)(int)(uint)local_1e * 200;
    }
    local_24 = *(ushort *)(param_1 + 0xb) * local_1c;
    local_7380 = local_2678 + (ulonglong)local_24 * 2;
    local_7378 = local_7318 + (ulonglong)local_24 * 2;
    local_7390 = local_2678 + (ulonglong)local_24 * 2;
    local_7388 = local_4cc8 + (ulonglong)local_24 * 2;
    local_7370 = *(undefined2 *)(param_1 + 0xb);
    CMF_Filter1DCore(&local_7398);
  }
  CMFRemap_MapBufferBack
            (*(undefined8 *)(param_1 + 6),
             *(short *)((longlong)param_1 + 0x2a) * *(short *)(param_1 + 10),local_2678,0x1324,
             param_1);
  if (*(longlong *)(param_1 + 8) != 0) {
    CMFRemap_MapBufferBack
              (*(undefined8 *)(param_1 + 8),
               *(short *)((longlong)param_1 + 0x2a) * *(short *)(param_1 + 10),local_7318,0x1324,
               param_1);
  }
  return;
}


// ============================================================================
// Function: CMF_InitWeightByNeighbor @ 6ba529b7
// ============================================================================

/* WARNING: Function: ___chkstk_ms replaced with injection: alloca_probe */
/* WARNING: Removing unreachable block (ram,0x6ba52eef) */

void CMF_InitWeightByNeighbor
               (ushort param_1,longlong param_2,int param_3,longlong param_4,longlong param_5)

{
  uint uVar1;
  uint uVar2;
  double dVar3;
  int local_27f8 [80];
  short local_26b8 [4903];
  ushort local_6a;
  int local_68;
  ushort local_62;
  ushort local_60;
  ushort local_5e;
  int local_5c;
  int local_58;
  int local_54;
  int local_50;
  int local_4c;
  int local_48;
  byte local_42;
  byte local_41;
  undefined2 *local_40;
  byte local_32;
  byte local_31;
  int local_30;
  byte local_29;
  undefined *local_28;
  ushort local_1e;
  int local_1c;
  byte local_15;
  int local_14;
  undefined2 *local_10;
  
  local_10 = (undefined2 *)0x6ba529c2;
  CMFRemap_MapBuffer(local_26b8,0x1324,*(undefined8 *)(param_2 + 0x10),param_2);
  local_40 = *(undefined2 **)(param_2 + 0x40);
  local_41 = (byte)*(undefined2 *)(param_2 + 0x2c);
  local_42 = (byte)*(undefined2 *)(param_2 + 0x2e);
  local_48 = (uint)param_1 * (uint)param_1;
  local_4c = (int)(short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax;
  local_14 = (uint)local_41 * (uint)local_42;
  local_10 = local_40;
  while (local_14 = local_14 + -1, local_14 != 0) {
    *local_10 = 1;
    local_10 = local_10 + 1;
  }
  local_14 = 0;
  for (local_15 = 0; local_15 < local_42; local_15 = local_15 + 1) {
    if ((param_4 == 0) || (*(char *)(param_4 + (ulonglong)local_15) != '\0')) {
      local_1c = 0;
      if (param_5 != 0) {
        *(undefined1 *)(param_5 + (ulonglong)local_15) = 1;
      }
      local_1e = (ushort)local_41 * (ushort)local_15 + 1;
      local_28 = &g_cmfNeighborCoefArray;
      for (local_29 = 1; (int)(uint)local_29 < (int)(local_41 - 1); local_29 = local_29 + 1) {
        local_50 = (int)local_26b8[(int)(local_1e - 1)];
        local_54 = (int)local_26b8[(int)(uint)local_1e];
        local_1e = local_1e + 1;
        local_58 = (int)local_26b8[(int)(uint)local_1e];
        local_5c = CMF_GetDistance(local_50,local_54,local_58,local_28);
        local_27f8[(int)(uint)local_29] = local_5c;
        local_28 = local_28 + 0x12;
      }
      local_30 = 1;
      local_1e = (ushort)local_41 * (ushort)local_15;
      for (local_31 = 1; local_1e = local_1e + 1, (int)(uint)local_31 < (int)(local_41 - 1);
          local_31 = local_31 + 1) {
        local_5e = *(ushort *)(*(longlong *)(param_2 + 0x38) + (ulonglong)local_1e * 2);
        if (local_48 < local_27f8[(int)(uint)local_31]) {
          local_30 = 0;
          local_40[local_5e] = 0;
          local_1c = local_1c + 1;
        }
        else if ((param_3 != 0) &&
                (uVar1 = (uint)*(short *)(*(longlong *)_refptr_g_tsaBufPreDif +
                                         (ulonglong)local_5e * 2), uVar2 = (int)uVar1 >> 0x1f,
                local_4c < (int)((uVar1 ^ uVar2) - uVar2))) {
          local_40[local_5e] = 0;
          local_1c = local_1c + 1;
        }
      }
      if ((param_5 != 0) && (((int)(local_41 - 5) <= local_1c || (local_30 != 0)))) {
        *(undefined1 *)(param_5 + (ulonglong)local_15) = 0;
      }
      local_27f8[0] = local_27f8[1];
      local_27f8[(int)(local_41 - 1)] = local_27f8[(int)(local_41 - 2)];
      local_40[*(ushort *)
                (*(longlong *)(param_2 + 0x38) +
                (longlong)(int)((uint)local_41 * (uint)local_15) * 2)] =
           local_40[*(ushort *)
                     (*(longlong *)(param_2 + 0x38) +
                     (longlong)(int)((uint)local_41 * (uint)local_15 + 1) * 2)];
      local_60 = *(ushort *)
                  (*(longlong *)(param_2 + 0x38) +
                  (longlong)(int)((uint)local_41 * (uint)local_15 + (local_41 - 1)) * 2);
      local_62 = *(ushort *)
                  (*(longlong *)(param_2 + 0x38) +
                  (longlong)(int)((uint)local_41 * (uint)local_15 + (local_41 - 2)) * 2);
      local_40[local_60] = local_40[local_62];
      if ((int)((uint)local_41 * 2 + (uint)local_41) >> 2 < local_1c) {
        local_68 = Misc_FindKthSmallest(local_27f8,local_41,local_41 >> 2);
        dVar3 = sqrt((double)local_68);
        local_68 = ((uint)param_1 + (int)dVar3) * ((uint)param_1 + (int)dVar3);
        for (local_32 = 0; local_32 < local_41; local_32 = local_32 + 1) {
          local_1e = (ushort)local_41 * (ushort)local_15 + (ushort)local_32;
          local_6a = *(ushort *)(*(longlong *)(param_2 + 0x38) + (ulonglong)local_1e * 2);
          local_40[local_6a] = (ushort)(local_27f8[(int)(uint)local_32] <= local_68);
        }
      }
      if (*(longlong *)(param_2 + 0x48) != 0) {
        CMF_UpdateNeighborDistance
                  (*(undefined8 *)(param_2 + 0x48),local_27f8,local_41,local_15,param_2);
      }
    }
  }
  CMF_CrossErosionWeight(local_40,*(undefined2 *)(param_2 + 0x28),*(undefined2 *)(param_2 + 0x2a));
  CMF_SetNotchToZero(local_40,(uint)*(ushort *)(param_2 + 0x2a) * (uint)*(ushort *)(param_2 + 0x28))
  ;
  CMF_ConditionClearCmfToPreDif(param_5,local_42,param_2,local_40);
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

undefined4 CMF_SubtractPreDif(longlong param_1,undefined8 param_2,longlong param_3,longlong param_4)

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

void CMF_AddPreDif(longlong param_1,undefined8 param_2,longlong param_3,longlong param_4)

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
  lVar5 = (longlong)local_48 + lVar2;
  for (local_19 = 0; local_19 < uVar4; local_19 = local_19 + 1) {
    local_48[3] = local_19 * local_28 + param_1 * local_24;
    local_48[2] = local_19 * local_28 + param_2 * local_24;
    *(int *)(lVar5 + (longlong)(int)(uint)local_19 * 4) =
         (int)*(short *)(*(longlong *)(param_3 + 0x10) + (ulonglong)(uint)local_48[3] * 2) -
         (int)*(short *)(*(longlong *)(param_3 + 0x18) + (ulonglong)(uint)local_48[2] * 2);
  }
  local_38 = lVar5;
  local_20 = uVar4;
  *(undefined8 *)((longlong)auStack_70 + lVar2) = 0x6ba53433;
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
  undefined4 uVar1;
  longlong lVar2;
  longlong *plVar3;
  longlong local_88 [2];
  undefined1 local_78;
  byte local_77;
  undefined1 local_76;
  undefined *local_70;
  undefined8 local_68;
  undefined8 local_60;
  undefined1 local_58;
  undefined8 local_50;
  byte local_27;
  byte local_26;
  undefined1 local_25;
  int local_24;
  byte local_1d;
  uint local_1c;
  
  plVar3 = local_88;
  for (lVar2 = 0xb; lVar2 != 0; lVar2 = lVar2 + -1) {
    *plVar3 = 0;
    plVar3 = plVar3 + 1;
  }
  local_76 = _refptr_g_tsaPrmtFlashCmfPca[1];
  local_70 = _refptr_g_tsaPrmtFlashCmfPca + 0xc84;
  local_77 = *_refptr_g_tsaPrmtFlashCmfPca;
  local_1c = 0;
  local_1d = 0;
  do {
    if (*(byte *)(param_1 + 0x38) <= local_1d) {
      return 1;
    }
    local_24 = 0;
    local_25 = *(undefined1 *)(param_1 + 0x39);
    if (*(char *)(param_1 + 0x39) == '\0') {
      for (local_26 = (byte)local_1c;
          (uint)local_26 < local_1c + *(int *)(param_1 + ((longlong)(int)(uint)local_1d + 0x1c) * 4)
          ; local_26 = local_26 + 1) {
        local_24 = local_24 + *(short *)(*(longlong *)(param_1 + 0x10) + (ulonglong)local_26 * 2);
      }
      if (local_24 == *(int *)(param_1 + ((longlong)(int)(uint)local_1d + 0x1c) * 4)) {
        local_25 = 1;
      }
      else if (local_24 < (int)(local_77 + 3)) {
        uVar1 = *(undefined4 *)(param_1 + ((longlong)(int)(uint)local_1d + 0x1c) * 4);
        for (local_27 = 0; local_27 < (byte)uVar1; local_27 = local_27 + 1) {
          *(undefined2 *)(*(longlong *)(param_1 + 0x20) + (ulonglong)(local_1c + local_27) * 2) = 0;
        }
        return 0;
      }
    }
    local_88[0] = *(longlong *)(param_1 + 8) + (ulonglong)local_1c * 2;
    local_88[1] = *(longlong *)(param_1 + 0x10) + (ulonglong)local_1c * 2;
    local_68 = *(undefined8 *)(param_1 + ((longlong)(int)(uint)local_1d + 0xc) * 8);
    local_78 = (undefined1)*(undefined4 *)(param_1 + ((longlong)(int)(uint)local_1d + 0x1c) * 4);
    local_58 = local_25;
    local_50 = *(undefined8 *)(param_1 + ((longlong)(int)(uint)local_1d + 8) * 8);
    local_60 = *(undefined8 *)(param_1 + ((longlong)(int)(uint)local_1d + 10) * 8);
    MtDiff_MsLinearCorrectionFilterCore
              (*(longlong *)(param_1 + 0x18) + (ulonglong)local_1c * 2,
               (ulonglong)local_1c * 2 + *(longlong *)(param_1 + 0x20),0x100,local_88);
    local_1c = local_1c + *(int *)(param_1 + ((longlong)(int)(uint)local_1d + 0x1c) * 4);
    local_1d = local_1d + 1;
  } while( true );
}


// ============================================================================
// Function: CMF_Filter1DByWholeLine @ 6ba51c74
// ============================================================================

void CMF_Filter1DByWholeLine(int *param_1)

{
  longlong lVar1;
  undefined8 *puVar2;
  undefined8 local_78 [2];
  undefined1 local_68;
  undefined1 local_67;
  undefined1 local_66;
  undefined *local_60;
  undefined *local_58;
  undefined1 local_48;
  
  if (*param_1 == 1) {
    puVar2 = local_78;
    for (lVar1 = 0xb; lVar1 != 0; lVar1 = lVar1 + -1) {
      *puVar2 = 0;
      puVar2 = puVar2 + 1;
    }
    local_48 = 0;
    local_78[0] = *(undefined8 *)(param_1 + 2);
    local_78[1] = *(undefined8 *)(param_1 + 4);
    local_58 = _refptr_g_tsaPrmtFlashCmfPca + 0xcec;
    local_66 = 0;
    local_60 = _refptr_g_tsaPrmtFlashCmfPca + 0xc84;
    local_67 = _refptr_g_tsaPrmtFlashCmfPca[0xceb];
    local_68 = (undefined1)(short)param_1[10];
    MtDiff_MsLinearCorrectionFilterCore
              (*(undefined8 *)(param_1 + 6),*(undefined8 *)(param_1 + 8),0x100,local_78);
  }
  return;
}


// ============================================================================
// Function: CMF_Ransac @ 6ba53106
// ============================================================================

/* WARNING: Function: ___chkstk_ms replaced with injection: alloca_probe */

void CMF_Ransac(longlong param_1,uint param_2,int param_3)

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
  puVar4 = (undefined4 *)((longlong)&local_48 + lVar1);
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
  *(undefined8 *)((longlong)auStack_70 + lVar1) = 0x6ba532aa;
  Misc_FindKthSmallest(puVar4,uVar2,uVar3 >> 1,param_2);
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


