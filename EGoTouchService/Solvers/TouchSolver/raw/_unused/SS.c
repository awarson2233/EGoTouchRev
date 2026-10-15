// ============================================================================
// Function: SS_IsRawValid @ 6baff0da
// ============================================================================

undefined1 SS_IsRawValid(void)

{
                    /* 0xbf0da  1199  SS_IsRawValid */
  return g_ssRawValid;
}


// ============================================================================
// Function: SS_CopyRaw @ 6baff201
// ============================================================================

void SS_CopyRaw(ushort *param_1,ushort param_2)

{
  ushort uVar1;
  int iVar2;
  int iVar3;
  ushort *local_res8;
  int *local_18;
  int local_c;
  
                    /* 0xbf201  1190  SS_CopyRaw */
  if ((param_1 == (ushort *)0x0) ||
     (param_2 < (ushort)((ushort)_refptr_g_tsaPrmtFlash->bRows +
                        (ushort)_refptr_g_tsaPrmtFlash->bCols))) {
    g_ssRawValid = 0;
    if ((_refptr_g_tsaPrmtDynamic[0x24] != '\0') && (*_refptr_g_tsaLogLoglevel != '\0')) {
      StrPrintf_Process(0,"\nSelf Signal Null Pointer");
    }
  }
  else {
    g_ssRawValid = 1;
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,&DAT_6bb58b1a);
    }
    if (*(short *)&_refptr_g_tsaPrmtFlash->field_0x5b8 == 0) {
      iVar2 = 1;
    }
    else {
      iVar2 = (int)(48000 / (longlong)(int)*(short *)&_refptr_g_tsaPrmtFlash->field_0x5b8);
    }
    local_18 = (int *)_refptr_g_tsaBufSSRaw;
    local_res8 = param_1;
    for (local_c = (uint)_refptr_g_tsaPrmtFlash->bRows + (uint)_refptr_g_tsaPrmtFlash->bCols;
        local_c != 0; local_c = local_c + -1) {
      uVar1 = *local_res8;
      *(uint *)(_refptr_g_tsaBufSSPreUnifiedRaw +
               (longlong)
               (int)(((uint)_refptr_g_tsaPrmtFlash->bRows + (uint)_refptr_g_tsaPrmtFlash->bCols) -
                    local_c) * 4) = (uint)uVar1;
      iVar3 = (uint)uVar1 * iVar2;
      if (iVar3 < 0) {
        iVar3 = iVar3 + 0xf;
      }
      *local_18 = iVar3 >> 4;
      local_res8 = local_res8 + 1;
      local_18 = local_18 + 1;
    }
  }
  return;
}


// ============================================================================
// Function: SS_CheckDirtyByMutual @ 6baffb6d
// ============================================================================

void SS_CheckDirtyByMutual(void)

{
  int iVar1;
  int iVar2;
  byte local_1e;
  byte local_1d [13];
  uint local_10;
  int local_c;
  
                    /* 0xbfb6d  1189  SS_CheckDirtyByMutual */
  SS_GetBaselineIdxRange(local_1d,&local_1e);
  local_c = 0;
  _refptr_g_tsaPrpt[1].field_0x78 = 0;
  _refptr_g_tsaPrpt[1].field_0x79 = 0;
  for (local_10 = (uint)local_1d[0]; (int)local_10 < (int)(uint)local_1e; local_10 = local_10 + 1) {
    iVar1 = *(int *)(_refptr_g_tsaBufSelf + ((longlong)(int)local_10 + 0x3c0) * 4);
    iVar2 = *(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)local_10 * 4);
    if (((600 < iVar1) && (iVar2 < 0x79)) || ((iVar1 < 0x259 && (0x78 < iVar2)))) {
      local_c = local_c + 1;
    }
    if (600 < iVar1) {
      if ((iVar2 < -0x77) ||
         (((iVar2 < 0x79 &&
           (((int)(local_10 - 1) < (int)(uint)local_1d[0] ||
            (*(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)(local_10 - 1) * 4) < 0x79)))) &&
          (((int)(uint)local_1e <= (int)(local_10 + 1) ||
           (*(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)(local_10 + 1) * 4) < 0x79)))))) {
        _refptr_g_tsaPrpt[1].field_0x78 = _refptr_g_tsaPrpt[1].field_0x78 + '\x01';
      }
      if ((((0x78 < iVar2) || ((int)(local_10 - 1) < (int)(uint)local_1d[0])) ||
          (0x78 < *(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)(local_10 - 1) * 4))) ||
         (((int)(uint)local_1e <= (int)(local_10 + 1) ||
          (0x78 < *(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)(local_10 + 1) * 4))))) {
        _refptr_g_tsaPrpt[1].field_0x79 = _refptr_g_tsaPrpt[1].field_0x79 + '\x01';
      }
    }
  }
  if (((int)((uint)local_1e - (uint)local_1d[0]) < 1) ||
     (local_c <= (int)(((uint)local_1e - (uint)local_1d[0]) * 2) / 3)) {
    _refptr_g_tsaPrpt[1].field_0x7a = 0;
  }
  else {
    _refptr_g_tsaPrpt[1].field_0x7a = 1;
  }
  return;
}


// ============================================================================
// Function: SS_Process @ 6bb00e4e
// ============================================================================

void SS_Process(void)

{
  char cVar1;
  longlong lVar2;
  
                    /* 0xc0e4e  1202  SS_Process */
  if (_refptr_g_tsaPrmtDynamic[0x24] != '\0') {
    cVar1 = SS_IsRawValid();
    if (cVar1 != '\0') {
      if (_refptr_g_tsaPrmtDynamic[0x2a] == '\0') {
        SS_UpdateStage();
        lVar2 = AFE_GetFrame();
        if ((*(longlong *)(lVar2 + 0x40) == 0) ||
           ((*(uint *)(*(longlong *)(lVar2 + 0x40) + 0x1c) & 0x10) == 0)) {
          SS_UpdateBaseline();
        }
        SS_CalculateDif();
        SS_CalculateIIRDif();
        SS_CMFProcess();
        SS_GetProperty();
      }
      else {
        SS_CopyDifFromRaw();
        SS_GetProperty();
        SS_UpdateStage();
      }
    }
  }
  return;
}


// ============================================================================
// Function: SS_ChargerNoiseFilterProcess @ 6bb018a2
// ============================================================================

void SS_ChargerNoiseFilterProcess(void)

{
  char cVar1;
  longlong lVar2;
  undefined8 uVar3;
  undefined4 uVar5;
  undefined4 *puVar4;
  undefined4 local_5c;
  undefined4 local_58;
  undefined4 local_54;
  undefined4 local_50;
  int local_4c;
  undefined4 local_48;
  short local_42;
  undefined8 local_40;
  undefined8 local_38;
  short local_2a;
  errno_t local_28;
  byte local_21;
  longlong local_20;
  uint local_18;
  uint local_14;
  uint local_10;
  uchar local_b;
  uchar local_a;
  byte local_9;
  
                    /* 0xc18a2  1187  SS_ChargerNoiseFilterProcess */
  cVar1 = SS_IsChargerNoiseFilterEnabled();
  if (cVar1 != '\0') {
    local_28 = memcpy_s(*(void **)_refptr_g_tsaBufSSChargerNFilterDif,
                        (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,
                        *(void **)_refptr_g_tsaBufDif,
                        (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
    if (local_28 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/SS/SS_Process.c",0x3ac,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
    if (local_28 == 0) {
      local_9 = _refptr_g_tsaPrmtFlash->bCols;
      local_a = _refptr_g_tsaPrmtFlash->bRows;
      local_b = _refptr_g_tsaPrmtFlash->bCols;
      local_10 = 0;
      local_14 = (uint)_refptr_g_tsaPrmtFlash->bCols;
      local_18 = 1;
      local_58 = 0;
      local_5c = 0;
      local_2a = *(short *)&_refptr_g_tsaPrpt[1].field_0x14 / 2;
      if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass != '\0') {
        local_9 = _refptr_g_tsaPrmtFlash->bRows;
        local_a = _refptr_g_tsaPrmtFlash->bCols;
        local_10 = (uint)_refptr_g_tsaPrmtFlash->bCols;
        local_14 = 0;
        local_b = '\x01';
        local_18 = (uint)_refptr_g_tsaPrmtFlash->bCols;
      }
      local_20 = SS_DiffRelatCal(local_9,local_10);
      cVar1 = SS_IsChargerNoiseFilterNeeded(local_20);
      if (cVar1 != '\0') {
        if (*_refptr_g_tsaLogLoglevel != '\0') {
          StrPrintf_Process(0,"\nSS_DiffRelatCal is %d, chargerNoiseThold is %d\n",local_20,
                            (int)local_2a);
        }
        local_38 = 0xc00;
        local_40 = 0x600;
        if (local_20 < 0xc01) {
          if (local_20 < 0x600) {
            local_20 = 0x600;
          }
        }
        else {
          local_20 = 0xc00;
        }
        puVar4 = &local_5c;
        SS_ChargerNoiseFilterSjSumCal((int)local_2a,local_a,local_14,&local_58,puVar4);
        for (local_21 = 0; local_21 < local_9; local_21 = local_21 + 1) {
          local_42 = (ushort)local_21 * (short)local_18;
          uVar3 = CONCAT44((int)((ulonglong)puVar4 >> 0x20),(int)local_2a);
          local_48 = SS_ChargerNoiseFilterSigSumCal(local_42,local_b,local_a,local_14,uVar3);
          uVar5 = (undefined4)((ulonglong)uVar3 >> 0x20);
          local_4c = *(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)(local_21 + local_10) * 4);
          lVar2 = local_4c * local_20;
          if (lVar2 < 0) {
            lVar2 = lVar2 + 0x3ff;
          }
          local_50 = (undefined4)(lVar2 >> 10);
          local_54 = SS_ChargerNoiseFilterKiCal(local_48,local_58,lVar2 >> 10 & 0xffffffff,local_5c)
          ;
          local_42 = (ushort)local_21 * (short)local_18;
          puVar4 = (undefined4 *)CONCAT44(uVar5,local_54);
          SS_FilterChargerNoise(local_42,local_a,local_14,local_b,puVar4);
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: SS_ChargerNoiseFilterSwitchBuffer @ 6bb01b92
// ============================================================================

bool SS_ChargerNoiseFilterSwitchBuffer(void)

{
  char cVar1;
  
                    /* 0xc1b92  1188  SS_ChargerNoiseFilterSwitchBuffer */
  cVar1 = SS_IsChargerNoiseFilterEnabled();
  if (cVar1 != '\0') {
    g_tsaDifBufPtrSsChargerNFilterBackup = *(undefined8 *)_refptr_g_tsaBufDif;
    *(undefined8 *)_refptr_g_tsaBufDif = *(undefined8 *)_refptr_g_tsaBufSSChargerNFilterDif;
  }
  return cVar1 != '\0';
}


// ============================================================================
// Function: SS_ChargerNoiseFilterPostProcess @ 6bb01bdf
// ============================================================================

void SS_ChargerNoiseFilterPostProcess(void)

{
  char cVar1;
  
                    /* 0xc1bdf  1186  SS_ChargerNoiseFilterPostProcess */
  cVar1 = SS_IsChargerNoiseFilterEnabled();
  if ((cVar1 != '\0') && (g_tsaDifBufPtrSsChargerNFilterBackup != 0)) {
    *(longlong *)_refptr_g_tsaBufDif = g_tsaDifBufPtrSsChargerNFilterBackup;
  }
  return;
}


// ============================================================================
// Function: SS_Reset @ 6baff430
// ============================================================================

void SS_Reset(void)

{
  int iVar1;
  errno_t eVar2;
  
                    /* 0xbf430  1203  SS_Reset */
  iVar1 = memset_s(_refptr_g_tsaBufSS,4,0,4);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SS/SS_Process.c",0x97,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(_refptr_g_tsaBufSSDif,0x280,0,
                   (longlong)
                   (int)((uint)_refptr_g_tsaPrmtFlash->bRows + (uint)_refptr_g_tsaPrmtFlash->bCols)
                   << 2);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SS/SS_Process.c",0x9a,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(_refptr_g_tsaBufSSPreCMFDif,0x280,0,
                   (longlong)
                   (int)((uint)_refptr_g_tsaPrmtFlash->bRows + (uint)_refptr_g_tsaPrmtFlash->bCols)
                   << 2);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SS/SS_Process.c",0x9d,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(_refptr_g_tsaBufSSIIRDif,0x280,0,
                   (longlong)
                   (int)((uint)_refptr_g_tsaPrmtFlash->bRows + (uint)_refptr_g_tsaPrmtFlash->bCols)
                   << 2);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SS/SS_Process.c",0xa0,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(_refptr_g_tsaBufSsDifForBl,0x280,0,
                   (longlong)
                   (int)((uint)_refptr_g_tsaPrmtFlash->bRows + (uint)_refptr_g_tsaPrmtFlash->bCols)
                   << 2);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SS/SS_Process.c",0xa3,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  eVar2 = memcpy_s(_refptr_g_tsaBufSSBaseline,0x280,_refptr_g_tsaBufSSRaw,
                   (longlong)
                   (int)((uint)_refptr_g_tsaPrmtFlash->bRows + (uint)_refptr_g_tsaPrmtFlash->bCols)
                   << 2);
  if (eVar2 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SS/SS_Process.c",0xa6,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  g_ssCaptureStatus = 0;
  *(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x48 = 3;
  return;
}


// ============================================================================
// Function: SS_GetBaselineIdxRange @ 6baff6fd
// ============================================================================

void SS_GetBaselineIdxRange(uchar *param_1,uchar *param_2)

{
  char cVar1;
  
  cVar1 = SS_IsDim1ValidForBaseline();
  if (cVar1 == '\0') {
    *param_1 = _refptr_g_tsaPrmtFlash->bCols;
  }
  else {
    *param_1 = '\0';
  }
  cVar1 = SS_IsDim2ValidForBaseline();
  if (cVar1 == '\0') {
    *param_2 = _refptr_g_tsaPrmtFlash->bCols;
  }
  else {
    *param_2 = _refptr_g_tsaPrmtFlash->bCols + _refptr_g_tsaPrmtFlash->bRows;
  }
  return;
}


// ============================================================================
// Function: SS_GetProperty @ 6baffa6f
// ============================================================================

void SS_GetProperty(void)

{
  char local_a;
  char local_9;
  
  *(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x4c = 0x80000000;
  *(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x50 = 0x7fffffff;
  *(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x54 = 0x80000000;
  *(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x58 = 0x7fffffff;
  *(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x6c = 0x80000000;
  *(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x70 = 0x7fffffff;
  *(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x74 = 0x80000000;
  SS_GetBaselineIdxRange(&local_9,&local_a);
  SS_GetSSDifPorperty(local_9,local_a);
  local_9 = local_9 + '\x01';
  local_a = local_a + -1;
  SS_GetSSPreCMFDifProperty(local_9,local_a);
  SS_GetSSIIRDifProperty(local_9,local_a);
  SS_GetSSDifForBlProperty(local_9,local_a);
  return;
}


// ============================================================================
// Function: SS_UpdateStage @ 6bafffb6
// ============================================================================

void SS_UpdateStage(void)

{
  int iVar1;
  
  if ((*_refptr_g_tsaStaticPtr)[1].field_0x2 == '\0') {
    if ((*_refptr_g_tsaStaticPtr)->bBaselineUpdatedThisFrame == '\0') {
      iVar1 = SS_IsDirtyTouch();
      if (iVar1 == 0) {
        *(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x48 = 5;
      }
      else {
        *(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x48 = 3;
      }
    }
    else {
      *(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x48 = 3;
    }
  }
  else {
    *(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x48 = 2;
  }
  return;
}


// ============================================================================
// Function: SS_UpdateBaseline @ 6bb0021c
// ============================================================================

void SS_UpdateBaseline(void)

{
  uint uVar1;
  ushort local_a;
  
  uVar1 = *(uint *)&_refptr_g_tsaPrpt[1].field_0x48;
  if (uVar1 == 2) {
    g_ssCaptureStatus = 0;
    return;
  }
  if (uVar1 < 3) {
    if (uVar1 == 0) {
      for (local_a = (ushort)g_ssIdxStart; local_a < g_ssIdxEnd; local_a = local_a + 1) {
        *(undefined4 *)(_refptr_g_tsaBufSSBaseline + (longlong)(int)(uint)local_a * 4) =
             *(undefined4 *)(_refptr_g_tsaBufSSRaw + (longlong)(int)(uint)local_a * 4);
      }
      g_ssCaptureStatus = 0;
      return;
    }
  }
  else {
    if (uVar1 == 3) {
      SS_UpdateBaselineNoTouchProcess();
      return;
    }
    if (uVar1 == 5) {
      g_ssCaptureStatus = 0;
      return;
    }
  }
  g_ssCaptureStatus = 0;
  return;
}


// ============================================================================
// Function: SS_CalculateDif @ 6bb0033f
// ============================================================================

void SS_CalculateDif(void)

{
  int iVar1;
  ushort local_a;
  
  for (local_a = (ushort)g_ssIdxStart; local_a < g_ssIdxEnd; local_a = local_a + 1) {
    if (_refptr_g_tsaPrmtDynamic[0x30] == '\0') {
      iVar1 = *(int *)(_refptr_g_tsaBufSSBaseline + (longlong)(int)(uint)local_a * 4) -
              *(int *)(_refptr_g_tsaBufSSRaw + (longlong)(int)(uint)local_a * 4);
    }
    else {
      iVar1 = *(int *)(_refptr_g_tsaBufSSRaw + (longlong)(int)(uint)local_a * 4) -
              *(int *)(_refptr_g_tsaBufSSBaseline + (longlong)(int)(uint)local_a * 4);
    }
    *(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)(uint)local_a * 4) = iVar1;
  }
  return;
}


// ============================================================================
// Function: SS_CopyDifFromRaw @ 6bb003f0
// ============================================================================

void SS_CopyDifFromRaw(void)

{
  ushort local_a;
  
  for (local_a = (ushort)g_ssIdxStart; local_a < g_ssIdxEnd; local_a = local_a + 1) {
    *(undefined4 *)(_refptr_g_tsaBufSSDif + (longlong)(int)(uint)local_a * 4) =
         *(undefined4 *)(_refptr_g_tsaBufSSRaw + (longlong)(int)(uint)local_a * 4);
    *(undefined4 *)(_refptr_g_tsaBufSSPreCMFDif + (longlong)(int)(uint)local_a * 4) =
         *(undefined4 *)(_refptr_g_tsaBufSSRaw + (longlong)(int)(uint)local_a * 4);
  }
  return;
}


// ============================================================================
// Function: SS_CalculateIIRDif @ 6bb0046f
// ============================================================================

void SS_CalculateIIRDif(void)

{
  char cVar1;
  int iVar2;
  byte local_1a;
  byte local_19 [15];
  ushort local_a;
  
  for (local_a = (ushort)g_ssIdxStart; local_a < g_ssIdxEnd; local_a = local_a + 1) {
    *(undefined4 *)(_refptr_g_tsaBufSsDifForBl + (longlong)(int)(uint)local_a * 4) =
         *(undefined4 *)(_refptr_g_tsaBufSSDif + (longlong)(int)(uint)local_a * 4);
  }
  SS_GetBaselineIdxRange(local_19,&local_1a);
  SS_CMFProcessRange(_refptr_g_tsaBufSsDifForBl + (longlong)(int)(uint)local_19[0] * 4,
                     0xa0 - (uint)local_19[0],(uint)local_1a - (uint)local_19[0],
                     (int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x28));
  local_a = (ushort)local_19[0];
  while (local_a = local_a + 1, (int)(uint)local_a < (int)(local_1a - 1)) {
    *(int *)(_refptr_g_tsaBufSsDifForBl + (longlong)(int)(uint)local_a * 4) =
         (*(int *)(_refptr_g_tsaBufSsDifForBl + (longlong)(int)(local_a + 1) * 4) +
         *(int *)(_refptr_g_tsaBufSsDifForBl + (longlong)(int)(uint)local_a * 4)) / 2;
  }
  cVar1 = TSAStatic_IsAllFreqNoisy();
  if (cVar1 != '\0') {
    for (local_a = (ushort)g_ssIdxStart; local_a < g_ssIdxEnd; local_a = local_a + 1) {
      iVar2 = *(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)(uint)local_a * 4) *
              (uint)*(byte *)(_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 3) +
              *(int *)(_refptr_g_tsaBufSSIIRDif + (longlong)(int)(uint)local_a * 4) *
              (0x100 - (uint)*(byte *)(_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 3));
      if (iVar2 < 0) {
        iVar2 = iVar2 + 0xff;
      }
      *(int *)(_refptr_g_tsaBufSSIIRDif + (longlong)(int)(uint)local_a * 4) =
           (int)(short)((uint)iVar2 >> 8);
      *(undefined4 *)(_refptr_g_tsaBufSSDif + (longlong)(int)(uint)local_a * 4) =
           *(undefined4 *)(_refptr_g_tsaBufSSIIRDif + (longlong)(int)(uint)local_a * 4);
    }
  }
  return;
}


// ============================================================================
// Function: SS_CMFProcess @ 6bb00da4
// ============================================================================

void SS_CMFProcess(void)

{
  char cVar1;
  errno_t eVar2;
  
  eVar2 = memcpy_s(_refptr_g_tsaBufSSPreCMFDif,0x280,_refptr_g_tsaBufSSDif,
                   (longlong)
                   (int)((uint)_refptr_g_tsaPrmtFlash->bRows + (uint)_refptr_g_tsaPrmtFlash->bCols)
                   << 2);
  if (eVar2 == 0) {
    cVar1 = SS_IsDim2Valid();
    if (cVar1 != '\0') {
      SS_CMFProcessDim2();
    }
    cVar1 = SS_IsDim1Valid();
    if (cVar1 != '\0') {
      SS_CMFProcessDim1();
    }
  }
  else {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SS/SS_Process.c",0x2ac,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: SS_DiffRelatCal @ 6bb00ed5
// ============================================================================

longlong SS_DiffRelatCal(byte param_1,int param_2)

{
  int iVar1;
  longlong lVar2;
  byte local_29;
  int local_28;
  int local_24;
  longlong local_20;
  longlong local_18;
  longlong local_10;
  
                    /* 0xc0ed5  1192  SS_DiffRelatCal */
  local_10 = 0;
  local_18 = 0;
  local_20 = 0;
  local_24 = 0;
  local_28 = 0;
  for (local_29 = 0; local_29 < param_1; local_29 = local_29 + 1) {
    iVar1 = *(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)((uint)local_29 + param_2) * 4);
    local_18 = local_18 +
               *(int *)(_refptr_g_tsaBufSelf + (longlong)(int)((uint)local_29 + param_2) * 4) *
               iVar1;
    local_20 = local_20 + iVar1 * iVar1;
    local_24 = local_24 +
               *(int *)(_refptr_g_tsaBufSelf + (longlong)(int)((uint)local_29 + param_2) * 4);
    local_28 = local_28 + iVar1;
  }
  lVar2 = (ulonglong)param_1 * local_20 - (longlong)local_28 * (longlong)local_28;
  if (lVar2 < 0) {
    lVar2 = lVar2 + 0x3ff;
  }
  if (lVar2 >> 10 != 0) {
    local_10 = (longlong)((ulonglong)param_1 * local_18 - (longlong)local_28 * (longlong)local_24) /
               (lVar2 >> 10);
  }
  return local_10;
}


// ============================================================================
// Function: SS_IsChargerNoiseFilterEnabled @ 6bb0114b
// ============================================================================

undefined8 SS_IsChargerNoiseFilterEnabled(void)

{
  char cVar1;
  
  cVar1 = SS_IsDim2Valid();
  if ((((cVar1 != '\0') && (cVar1 = SS_IsDim1Valid(), cVar1 != '\0')) &&
      (_refptr_g_tsaPrmtDynamic[0x38] != '\0')) &&
     ((cVar1 = TSAStatic_IsChargerPluggedNoDebounce(), cVar1 != '\0' ||
      ((*_refptr_g_tsaStaticPtr)->bField_0x111 == '\x03')))) {
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: SS_IsChargerNoiseFilterNeeded @ 6bb011a4
// ============================================================================

undefined8 SS_IsChargerNoiseFilterNeeded(longlong param_1)

{
  short sVar1;
  uint uVar2;
  int iVar3;
  longlong lVar4;
  longlong lVar5;
  int local_c;
  
  lVar5 = g_ssDiffRelatK;
  if ((short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax < _refptr_g_tsaPrpt->nSPeakPositive) {
    sVar1 = *(short *)&_refptr_g_tsaPrpt[1].field_0x14;
    for (local_c = 0; iVar3 = NotchCorrect_GetNotchNum(), local_c < iVar3; local_c = local_c + 1) {
      uVar2 = (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                             (longlong)local_c * 3 + 0x105) +
              (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                             (longlong)local_c * 3 + 0x106) * (uint)_refptr_g_tsaPrmtFlash->bCols &
              0xffff;
      if ((*(short *)&_refptr_g_tsaPrpt[1].field_0x16 <
           *(short *)(*(longlong *)_refptr_g_tsaBufDif + (longlong)(int)uVar2 * 2)) ||
         ((int)*(short *)&_refptr_g_tsaPrpt[1].field_0x16 <
          -(int)*(short *)(*(longlong *)_refptr_g_tsaBufDif + (longlong)(int)uVar2 * 2))) {
        return 0;
      }
    }
    if (((((((int)sVar1 < *(int *)_refptr_g_tsaBufSSDif) ||
           ((int)sVar1 < -*(int *)_refptr_g_tsaBufSSDif)) ||
          ((int)sVar1 <
           *(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)(_refptr_g_tsaPrmtFlash->bCols - 1) * 4))
          ) || ((int)sVar1 <
                -*(int *)(_refptr_g_tsaBufSSDif +
                         (longlong)(int)(_refptr_g_tsaPrmtFlash->bCols - 1) * 4))) &&
        ((((int)sVar1 <
           *(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)(uint)_refptr_g_tsaPrmtFlash->bCols * 4)
          || ((int)sVar1 <
              -*(int *)(_refptr_g_tsaBufSSDif +
                       (longlong)(int)(uint)_refptr_g_tsaPrmtFlash->bCols * 4))) ||
         (((int)sVar1 <
           *(int *)(_refptr_g_tsaBufSSDif +
                   (longlong)
                   (int)((uint)_refptr_g_tsaPrmtFlash->bRows + (uint)_refptr_g_tsaPrmtFlash->bCols +
                        -1) * 4) ||
          ((int)sVar1 <
           -*(int *)(_refptr_g_tsaBufSSDif +
                    (longlong)
                    (int)((uint)_refptr_g_tsaPrmtFlash->bRows + (uint)_refptr_g_tsaPrmtFlash->bCols
                         + -1) * 4))))))) && ((*_refptr_g_tsaStaticPtr)->bField_0x111 != '\x03')) {
      return 0;
    }
    lVar5 = g_ssDiffRelatK * 0xe;
    if (lVar5 < 0) {
      lVar5 = lVar5 + 0xf;
    }
    lVar4 = param_1 * 2;
    if (lVar4 < 0) {
      lVar4 = lVar4 + 0xf;
    }
    lVar5 = (lVar5 >> 4) + (lVar4 >> 4);
    if ((param_1 < (g_ssDiffRelatK * 7) / 5) && ((g_ssDiffRelatK * 3) / 5 < param_1)) {
      g_ssDiffRelatK = lVar5;
      return 1;
    }
    if (((param_1 < (g_ssDiffRelatK * 8) / 5) && ((g_ssDiffRelatK * 2) / 5 < param_1)) &&
       ((*_refptr_g_tsaStaticPtr)->bField_0x111 == '\x03')) {
      g_ssDiffRelatK = lVar5;
      return 1;
    }
  }
  g_ssDiffRelatK = lVar5;
  return 0;
}


// ============================================================================
// Function: SS_ChargerNoiseFilterSjSumCal @ 6bb01589
// ============================================================================

void SS_ChargerNoiseFilterSjSumCal
               (short param_1,byte param_2,int param_3,int *param_4,undefined4 *param_5)

{
  byte local_d;
  int local_c;
  
  local_c = 1;
  for (local_d = 0; local_d < param_2; local_d = local_d + 1) {
    if ((int)param_1 <
        *(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)((uint)local_d + param_3) * 4)) {
      *param_4 = *param_4 +
                 *(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)((uint)local_d + param_3) * 4);
    }
    if (local_c < *(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)((uint)local_d + param_3) * 4)) {
      local_c = *(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)((uint)local_d + param_3) * 4);
    }
    else if (local_c < -*(int *)(_refptr_g_tsaBufSSDif +
                                (longlong)(int)((uint)local_d + param_3) * 4)) {
      local_c = -*(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)((uint)local_d + param_3) * 4);
    }
  }
  if ((*_refptr_g_tsaStaticPtr)->bField_0x111 == '\x03') {
    *param_5 = (int)(0x1f400 / (longlong)local_c);
  }
  else {
    *param_5 = (int)(0x9600 / (longlong)local_c);
  }
  return;
}


// ============================================================================
// Function: SS_ChargerNoiseFilterSigSumCal @ 6bb016b9
// ============================================================================

int SS_ChargerNoiseFilterSigSumCal
              (ushort param_1,byte param_2,byte param_3,int param_4,short param_5)

{
  ushort local_res8;
  byte local_d;
  int local_c;
  
  local_c = 0;
  local_res8 = param_1;
  for (local_d = 0; local_d < param_3; local_d = local_d + 1) {
    if ((int)param_5 <
        *(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)((uint)local_d + param_4) * 4)) {
      local_c = local_c + *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)local_res8 * 2);
    }
    local_res8 = local_res8 + param_2;
  }
  return local_c;
}


// ============================================================================
// Function: SS_ChargerNoiseFilterKiCal @ 6bb01741
// ============================================================================

int SS_ChargerNoiseFilterKiCal(int param_1,int param_2,int param_3,int param_4)

{
  int local_res10;
  int local_c;
  
  local_res10 = param_2;
  if (param_2 == 0) {
    local_res10 = 1;
  }
  local_c = ((param_1 - param_3) * 0x40) / local_res10;
  if (param_4 < local_c) {
    local_c = param_4;
  }
  else if (local_c < -param_4) {
    local_c = -param_4;
  }
  return local_c;
}


// ============================================================================
// Function: SS_FilterChargerNoise @ 6bb0179f
// ============================================================================

void SS_FilterChargerNoise(ushort param_1,byte param_2,int param_3,byte param_4,int param_5)

{
  short sVar1;
  int iVar2;
  ushort local_res8;
  byte local_9;
  
  sVar1 = *(short *)&_refptr_g_tsaPrpt[1].field_0x14;
  local_res8 = param_1;
  for (local_9 = 0; local_9 < param_2; local_9 = local_9 + 1) {
    if ((int)(sVar1 / 2) <
        *(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)((uint)local_9 + param_3) * 4)) {
      iVar2 = *(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)((uint)local_9 + param_3) * 4) *
              param_5;
      if (iVar2 < 0) {
        iVar2 = iVar2 + 0x3f;
      }
      *(short *)(*(longlong *)_refptr_g_tsaBufSSChargerNFilterDif + (ulonglong)local_res8 * 2) =
           *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)local_res8 * 2) -
           (short)(((iVar2 >> 6) * 4) / 5);
    }
    local_res8 = local_res8 + param_4;
  }
  return;
}


// ============================================================================
// Function: SS_IsDim1Valid @ 6baff040
// ============================================================================

byte SS_IsDim1Valid(void)

{
                    /* 0xbf040  1195  SS_IsDim1Valid */
  return _refptr_g_tsaPrmtDynamic[0x24] & 1;
}


// ============================================================================
// Function: SS_IsDim2Valid @ 6baff054
// ============================================================================

byte SS_IsDim2Valid(void)

{
                    /* 0xbf054  1196  SS_IsDim2Valid */
  return _refptr_g_tsaPrmtDynamic[0x24] & 2;
}


// ============================================================================
// Function: SS_IsValidForCMF @ 6baff1f3
// ============================================================================

undefined8 SS_IsValidForCMF(void)

{
                    /* 0xbf1f3  1201  SS_IsValidForCMF */
  return 0;
}


// ============================================================================
// Function: SS_DiffCMFProcessDimUnit @ 6bb00fef
// ============================================================================

ulonglong SS_DiffCMFProcessDimUnit
                    (int param_1,byte param_2,byte param_3,byte param_4,longlong param_5)

{
  ulonglong uVar1;
  ushort local_18;
  byte local_16;
  byte local_15;
  uint local_10;
  byte local_9;
  
                    /* 0xc0fef  1191  SS_DiffCMFProcessDimUnit */
  if (param_1 == 1) {
    local_10 = (uint)param_2;
    local_15 = 1;
    local_18 = (ushort)param_2;
    local_9 = param_2;
  }
  else {
    local_10 = 0;
    local_18 = 1;
    local_15 = param_2;
    local_9 = param_3;
  }
  param_5 = *(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)(param_4 + local_10) * 4) * param_5;
  if (param_5 < 0) {
    param_5 = param_5 + 0x3ff;
  }
  uVar1 = ((longlong)*(int *)(_refptr_g_tsaBufSelf + (longlong)(int)(param_4 + local_10) * 4) -
          (param_5 >> 10)) / (longlong)(ulonglong)local_9;
  local_18 = param_4 * local_18;
  for (local_16 = 0; local_16 < local_9; local_16 = local_16 + 1) {
    *(short *)((ulonglong)local_18 * 2 + *(longlong *)_refptr_g_tsaBufDif) =
         *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)local_18 * 2) - (short)uVar1;
    *(undefined2 *)((ulonglong)local_18 * 2 + *(longlong *)_refptr_g_tsaBufAftSSCMFDif) =
         *(undefined2 *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)local_18 * 2);
    local_18 = local_18 + local_15;
  }
  return uVar1 & 0xffffffff;
}


// ============================================================================
// Function: SS_IsDim1ValidForBaseline @ 6baff0e7
// ============================================================================

undefined8 SS_IsDim1ValidForBaseline(void)

{
  char cVar1;
  undefined8 uVar2;
  
  cVar1 = SS_IsDim1Valid();
  if ((cVar1 == '\0') || ((_refptr_g_tsaPrmtDynamic[0x25] & 1) == 0)) {
    uVar2 = 0;
  }
  else {
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: SS_IsDim2ValidForBaseline @ 6baff11f
// ============================================================================

undefined8 SS_IsDim2ValidForBaseline(void)

{
  char cVar1;
  undefined8 uVar2;
  
  cVar1 = SS_IsDim2Valid();
  if ((cVar1 == '\0') || ((_refptr_g_tsaPrmtDynamic[0x25] & 2) == 0)) {
    uVar2 = 0;
  }
  else {
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: SS_GetSSDifPorperty @ 6baff770
// ============================================================================

void SS_GetSSDifPorperty(byte param_1,byte param_2)

{
  int iVar1;
  ushort local_a;
  
  for (local_a = (ushort)param_1; local_a < param_2; local_a = local_a + 1) {
    iVar1 = *(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)(uint)local_a * 4);
    if (*(int *)&_refptr_g_tsaPrpt[1].field_0x4c < iVar1) {
      *(int *)&_refptr_g_tsaPrpt[1].field_0x4c = iVar1;
    }
    if (iVar1 < *(int *)&_refptr_g_tsaPrpt[1].field_0x50) {
      *(int *)&_refptr_g_tsaPrpt[1].field_0x50 = iVar1;
    }
  }
  return;
}


// ============================================================================
// Function: SS_GetSSPreCMFDifProperty @ 6baff7fd
// ============================================================================

void SS_GetSSPreCMFDifProperty(byte param_1,byte param_2)

{
  int iVar1;
  ushort local_a;
  
  *(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x5c = 0;
  *(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x60 = 0;
  *(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x64 = 0;
  *(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x68 = 0;
  for (local_a = (ushort)param_1; local_a < param_2; local_a = local_a + 1) {
    iVar1 = *(int *)(_refptr_g_tsaBufSSPreCMFDif + (longlong)(int)(uint)local_a * 4);
    if (*(int *)&_refptr_g_tsaPrpt[1].field_0x54 < iVar1) {
      *(int *)&_refptr_g_tsaPrpt[1].field_0x54 = iVar1;
    }
    if (iVar1 < *(int *)&_refptr_g_tsaPrpt[1].field_0x58) {
      *(int *)&_refptr_g_tsaPrpt[1].field_0x58 = iVar1;
    }
    if ((int)(uint)*(ushort *)(_refptr_g_tsaPrmtDynamic + 0x2c) < iVar1) {
      *(int *)&_refptr_g_tsaPrpt[1].field_0x5c = *(int *)&_refptr_g_tsaPrpt[1].field_0x5c + iVar1;
      *(int *)&_refptr_g_tsaPrpt[1].field_0x64 = *(int *)&_refptr_g_tsaPrpt[1].field_0x64 + 1;
    }
    else if (iVar1 < (int)-(uint)*(ushort *)(_refptr_g_tsaPrmtDynamic + 0x2c)) {
      *(int *)&_refptr_g_tsaPrpt[1].field_0x60 = *(int *)&_refptr_g_tsaPrpt[1].field_0x60 + iVar1;
      *(int *)&_refptr_g_tsaPrpt[1].field_0x68 = *(int *)&_refptr_g_tsaPrpt[1].field_0x68 + 1;
    }
  }
  return;
}


// ============================================================================
// Function: SS_GetSSIIRDifProperty @ 6baff977
// ============================================================================

void SS_GetSSIIRDifProperty(byte param_1,byte param_2)

{
  int iVar1;
  ushort local_a;
  
  for (local_a = (ushort)param_1; local_a < param_2; local_a = local_a + 1) {
    iVar1 = *(int *)(_refptr_g_tsaBufSSIIRDif + (longlong)(int)(uint)local_a * 4);
    if (*(int *)&_refptr_g_tsaPrpt[1].field_0x6c < iVar1) {
      *(int *)&_refptr_g_tsaPrpt[1].field_0x6c = iVar1;
    }
    if (iVar1 < *(int *)&_refptr_g_tsaPrpt[1].field_0x70) {
      *(int *)&_refptr_g_tsaPrpt[1].field_0x70 = iVar1;
    }
  }
  return;
}


// ============================================================================
// Function: SS_GetSSDifForBlProperty @ 6baffa04
// ============================================================================

void SS_GetSSDifForBlProperty(byte param_1,byte param_2)

{
  ushort local_a;
  
  for (local_a = (ushort)param_1; local_a < param_2; local_a = local_a + 1) {
    if (*(int *)&_refptr_g_tsaPrpt[1].field_0x74 <
        *(int *)(_refptr_g_tsaBufSsDifForBl + (longlong)(int)(uint)local_a * 4)) {
      *(int *)&_refptr_g_tsaPrpt[1].field_0x74 =
           *(int *)(_refptr_g_tsaBufSsDifForBl + (longlong)(int)(uint)local_a * 4);
    }
  }
  return;
}


// ============================================================================
// Function: SS_IsDirtyTouch @ 6baffd75
// ============================================================================

undefined8 SS_IsDirtyTouch(void)

{
  undefined8 uVar1;
  int local_10;
  int local_c;
  
  if ((ulonglong)
      ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
      *(longlong *)&(*_refptr_g_prevTouchesPtr)[1].field_0x14) < 0x1389) {
    if ((*_refptr_g_prevTouchesPtr)->field_0x3 == '\0') {
      if ((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 2 <
          _refptr_g_tsaPrpt->nSPeakPositive) {
        uVar1 = 0;
      }
      else if (*(ushort *)&_refptr_g_tsaPrpt->field_0xd8 < 0x15) {
        if (*(short *)&_refptr_g_tsaPrpt->field_0x8 == 0) {
          local_10 = 0;
          for (local_c = 0; local_c < (int)(uint)(*_refptr_g_prevTouchesPtr)->bPrevTouchCount;
              local_c = local_c + 1) {
            if (((((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)local_c * 0x478 + 0x1f8) & 8) != 0) &&
                 ((0x2ff < *(ushort *)
                            ((longlong)*_refptr_g_prevTouchesPtr + (longlong)local_c * 0x478 + 0x22c
                            ) ||
                  (0xff < *(ushort *)
                           ((longlong)*_refptr_g_prevTouchesPtr + (longlong)local_c * 0x478 + 0x22e)
                  )))) && ((0xff < *(ushort *)
                                    ((longlong)*_refptr_g_prevTouchesPtr +
                                    (longlong)local_c * 0x478 + 0x22c) ||
                           (0x2ff < *(ushort *)
                                     ((longlong)*_refptr_g_prevTouchesPtr +
                                     (longlong)local_c * 0x478 + 0x22e))))) &&
               ((0x1ff < *(ushort *)
                          ((longlong)*_refptr_g_prevTouchesPtr + (longlong)local_c * 0x478 + 0x22c)
                || (0x1ff < *(ushort *)
                             ((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)local_c * 0x478 + 0x22e))))) {
              local_10 = local_10 + 1;
            }
          }
          if (local_10 < 1) {
            uVar1 = 1;
          }
          else {
            uVar1 = 0;
          }
        }
        else {
          uVar1 = 0;
        }
      }
      else {
        uVar1 = 0;
      }
    }
    else {
      uVar1 = 0;
    }
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: SS_UpdateBaselineNoTouchProcess @ 6bb00064
// ============================================================================

void SS_UpdateBaselineNoTouchProcess(void)

{
  char cVar1;
  ushort local_a;
  
  if (g_ssCaptureStatus == 1) {
    if ((void *)((longlong)g_ssCaptureTimer + 100) <= (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr) {
      for (local_a = (ushort)g_ssIdxStart; local_a < g_ssIdxEnd; local_a = local_a + 1) {
        *(undefined4 *)(_refptr_g_tsaBufSSRawCapture + (longlong)(int)(uint)local_a * 4) =
             *(undefined4 *)(_refptr_g_tsaBufSSRaw + (longlong)(int)(uint)local_a * 4);
      }
      g_ssCaptureStatus = 2;
      g_ssCaptureTimer = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    }
  }
  else if (g_ssCaptureStatus == 0) {
    g_ssCaptureStatus = 1;
    g_ssCaptureTimer = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  }
  else if (g_ssCaptureStatus == 2) {
    if (((void *)((longlong)g_ssCaptureTimer + 100) <= (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr)
       && (cVar1 = SS_KeepBaseLine(), cVar1 == '\0')) {
      for (local_a = (ushort)g_ssIdxStart; local_a < g_ssIdxEnd; local_a = local_a + 1) {
        *(undefined4 *)(_refptr_g_tsaBufSSBaseline + (longlong)(int)(uint)local_a * 4) =
             *(undefined4 *)(_refptr_g_tsaBufSSRawCapture + (longlong)(int)(uint)local_a * 4);
        *(undefined4 *)(_refptr_g_tsaBufSSRawCapture + (longlong)(int)(uint)local_a * 4) =
             *(undefined4 *)(_refptr_g_tsaBufSSRaw + (longlong)(int)(uint)local_a * 4);
      }
      g_ssCaptureTimer = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    }
  }
  else {
    g_ssCaptureStatus = 0;
  }
  return;
}


// ============================================================================
// Function: SS_CMFProcessRange @ 6bb002da
// ============================================================================

undefined8 SS_CMFProcessRange(longlong param_1,int param_2,int param_3,undefined4 param_4)

{
  undefined8 uVar1;
  undefined1 local_d;
  undefined4 local_c;
  
  if ((param_1 == 0) || (param_2 < param_3)) {
    uVar1 = 0;
  }
  else {
    local_c = 200;
    uVar1 = CMF_ProcessCore(param_1,param_2,param_3,param_4,200,&local_d);
  }
  return uVar1;
}


// ============================================================================
// Function: SS_CMFProcessDim2 @ 6bb00803
// ============================================================================

void SS_CMFProcessDim2(void)

{
  longlong lVar1;
  int iVar2;
  undefined1 local_1d8 [95];
  undefined1 local_179;
  undefined1 local_178 [335];
  byte local_29;
  longlong local_28;
  longlong local_20;
  int local_14;
  int local_10;
  int local_c;
  
  SS_CMFBufferProcess(local_178,0x50);
  local_20 = AFE_GetFrame();
  lVar1 = AFE_GetFrame();
  local_28 = *(longlong *)(lVar1 + 0x40);
  if (((*(longlong *)(local_20 + 0x10) == 0) && (local_28 != 0)) &&
     ((*(uint *)(local_28 + 0x1c) & 0x10) != 0)) {
    CMF_ProcessCoreWeight
              (_refptr_g_tsaBufSSDif + (longlong)(int)(uint)_refptr_g_tsaPrmtFlash->bCols * 4,0,
               _refptr_g_tsaPrmtFlash->bRows,0x7fffffff,200,&local_179);
  }
  else {
    local_10 = *(int *)(_refptr_g_tsaBufSelf +
                       ((longlong)(int)(uint)_refptr_g_tsaPrmtFlash->bCols + 0x3c0) * 4);
    for (local_c = 0; local_c < (int)(uint)_refptr_g_tsaPrmtFlash->bRows; local_c = local_c + 1) {
      if (local_10 <
          *(int *)(_refptr_g_tsaBufSelf +
                  ((longlong)(int)((uint)_refptr_g_tsaPrmtFlash->bCols + local_c) + 0x3c0) * 4)) {
        local_10 = *(int *)(_refptr_g_tsaBufSelf +
                           ((longlong)(int)((uint)_refptr_g_tsaPrmtFlash->bCols + local_c) + 0x3c0)
                           * 4);
      }
    }
    if (local_10 == 0) {
      local_10 = 1;
    }
    for (local_c = 0; local_c < (int)(uint)_refptr_g_tsaPrmtFlash->bRows; local_c = local_c + 1) {
      iVar2 = 0xff - (*(int *)(_refptr_g_tsaBufSelf +
                              ((longlong)(int)((uint)_refptr_g_tsaPrmtFlash->bCols + local_c) +
                              0x3c0) * 4) * 0xff) / local_10;
      local_14 = (iVar2 * iVar2 * iVar2) / 0xfe01;
      if (local_14 < 10) {
        local_14 = 10;
      }
      else if (0xff < local_14) {
        local_14 = 0xff;
      }
      local_1d8[local_c] = (char)local_14;
    }
    if (_refptr_g_tsaPrmtFlash->field_0x80e == '\0') {
      CMF_ProcessCoreWeight
                (_refptr_g_tsaBufSSDif + (longlong)(int)(uint)_refptr_g_tsaPrmtFlash->bCols * 4,
                 local_1d8,_refptr_g_tsaPrmtFlash->bRows,0x7fffffff,200,&local_179);
    }
    else {
      local_29 = _refptr_g_tsaPrmtFlash->bRows >> 1;
      CMF_SSProcessCoreWeightHalfDim
                (_refptr_g_tsaBufSSDif + (longlong)(int)(uint)_refptr_g_tsaPrmtFlash->bCols * 4,
                 local_1d8,local_29,0x7fffffff,200,&local_179);
      CMF_SSProcessCoreWeightHalfDim
                (_refptr_g_tsaBufSSDif +
                 (longlong)(int)((uint)local_29 + (uint)_refptr_g_tsaPrmtFlash->bCols) * 4,
                 local_1d8 + local_29,local_29,0x7fffffff,200,&local_179);
    }
  }
  return;
}


// ============================================================================
// Function: SS_CMFProcessDim1 @ 6bb00bab
// ============================================================================

void SS_CMFProcessDim1(void)

{
  short sVar1;
  byte bVar2;
  int local_14;
  int local_10;
  int local_c;
  
  if (_refptr_g_tsaPrmtDynamic[0x31] == '\x01') {
    local_c = 0;
    local_10 = 0;
    for (local_14 = 0; local_14 < (int)(uint)_refptr_g_tsaPrmtFlash->bCols; local_14 = local_14 + 1)
    {
      local_10 = local_10 + *(int *)(_refptr_g_tsaBufSSDif + (longlong)local_14 * 4);
    }
    for (local_14 = 0; local_14 < (int)(uint)_refptr_g_tsaPrmtFlash->bRows; local_14 = local_14 + 1)
    {
      local_c = local_c + *(int *)(_refptr_g_tsaBufSSDif +
                                  (longlong)(int)((uint)_refptr_g_tsaPrmtFlash->bCols + local_14) *
                                  4);
    }
    sVar1 = *(short *)(_refptr_g_tsaPrmtDynamic + 0x34);
    bVar2 = _refptr_g_tsaPrmtFlash->bCols;
    for (local_14 = 0; local_14 < (int)(uint)_refptr_g_tsaPrmtFlash->bCols; local_14 = local_14 + 1)
    {
      *(int *)(_refptr_g_tsaBufSSDif + (longlong)local_14 * 4) =
           *(int *)(_refptr_g_tsaBufSSDif + (longlong)local_14 * 4) +
           ((sVar1 * local_c) / 100 - local_10) / (int)(uint)bVar2;
    }
    SS_CMFProcessRange(_refptr_g_tsaBufSSDif,0xa0,_refptr_g_tsaPrmtFlash->bCols,200);
  }
  else if (_refptr_g_tsaPrmtFlash->field_0x80e == '\0') {
    SS_CMFProcessRange(_refptr_g_tsaBufSSDif,0xa0,_refptr_g_tsaPrmtFlash->bCols,0x7fffffff);
  }
  else {
    bVar2 = _refptr_g_tsaPrmtFlash->bCols >> 1;
    SS_CMFProcessRange(_refptr_g_tsaBufSSDif,0xa0,bVar2,0x7fffffff);
    SS_CMFProcessRange(_refptr_g_tsaBufSSDif + (longlong)(int)(uint)bVar2 * 4,0xa0 - (uint)bVar2,
                       bVar2,0x7fffffff);
  }
  return;
}


// ============================================================================
// Function: SS_TouchLength @ 6bb0208b
// ============================================================================

byte SS_TouchLength(longlong param_1,byte param_2,int param_3)

{
  byte local_c;
  byte local_b;
  byte local_a;
  byte local_9;
  
                    /* 0xc208b  1205  SS_TouchLength */
  local_9 = 0;
  local_a = 0xff;
  local_b = 0;
  for (local_c = 0; local_c < param_2; local_c = local_c + 1) {
    if (param_3 < *(int *)(param_1 + (ulonglong)local_c * 4)) {
      if (local_a == 0xff) {
        local_a = local_c;
        local_b = local_b + 1;
      }
      else {
        local_b = local_b + 1;
      }
    }
    else if ((param_3 / 2 < *(int *)(param_1 + (ulonglong)local_c * 4)) && (local_a != 0xff)) {
      local_b = local_b + 1;
    }
    else if ((local_a != 0xff) && (local_9 < local_b)) {
      local_9 = local_b;
    }
  }
  if (local_9 < local_b) {
    local_9 = local_b;
  }
  return local_9;
}


// ============================================================================
// Function: SS_TouchValid @ 6bb01fe3
// ============================================================================

void SS_TouchValid(byte param_1)

{
  uint uVar1;
  
                    /* 0xc1fe3  1206  SS_TouchValid */
  uVar1 = (uint)*(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x23c);
  SS_IsDifValid((*_refptr_g_peaksPtr)[(longlong)(int)uVar1 * 0x14 + 0x15].bPeakCount,
                (*_refptr_g_peaksPtr)[(longlong)(int)uVar1 * 0x14 + 0x16].bPeakCount);
  return;
}


// ============================================================================
// Function: SS_KeepBaseLine @ 6bb00041
// ============================================================================

bool SS_KeepBaseLine(void)

{
  char cVar1;
  
  cVar1 = ToeSyna_KeepSSBaseline();
  return cVar1 != '\0';
}


// ============================================================================
// Function: SS_CMFBufferProcess @ 6bb0067a
// ============================================================================

void SS_CMFBufferProcess(longlong param_1,undefined1 param_2)

{
  int iVar1;
  uint local_c;
  
  if (_refptr_g_tsaPrmtDynamic[0x32] == '\x02') {
    for (local_c = 0; (int)local_c < (int)(_refptr_g_tsaPrmtFlash->bRows - 1); local_c = local_c + 1
        ) {
      *(int *)((longlong)(int)local_c * 4 + param_1) =
           *(int *)(_refptr_g_tsaBufSSDif +
                   (longlong)(int)(_refptr_g_tsaPrmtFlash->bCols + local_c) * 4) -
           *(int *)(_refptr_g_tsaBufSSDif +
                   (longlong)(int)(local_c + _refptr_g_tsaPrmtFlash->bCols + 1) * 4);
      if ((local_c & 1) != 0) {
        *(int *)(param_1 + (longlong)(int)local_c * 4) =
             -*(int *)(param_1 + (longlong)(int)local_c * 4);
      }
    }
    iVar1 = SS_CMFProcessRange(param_1,param_2,_refptr_g_tsaPrmtFlash->bRows - 1,0x7fffffff);
    for (local_c = 1; (int)local_c < (int)(uint)_refptr_g_tsaPrmtFlash->bRows; local_c = local_c + 2
        ) {
      *(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)(_refptr_g_tsaPrmtFlash->bCols + local_c) * 4)
           = *(int *)(_refptr_g_tsaBufSSDif +
                     (longlong)(int)(_refptr_g_tsaPrmtFlash->bCols + local_c) * 4) + iVar1;
    }
  }
  return;
}


// ============================================================================
// Function: SS_IsDifValid @ 6bb01d00
// ============================================================================

undefined8 SS_IsDifValid(undefined4 param_1,undefined4 param_2)

{
  char cVar1;
  int iVar2;
  undefined8 uVar3;
  
  cVar1 = SS_IsDim1ValidForTouch();
  if ((cVar1 == '\0') && (cVar1 = SS_IsDim2ValidForTouch(), cVar1 == '\0')) {
    return 1;
  }
  cVar1 = SS_IsDim2ValidForTouch();
  if (cVar1 == '\0') {
    uVar3 = SS_IsDifValidOnDim(1,param_1);
  }
  else {
    cVar1 = SS_IsDim1ValidForTouch();
    if (cVar1 == '\0') {
      uVar3 = SS_IsDifValidOnDim(2,param_2);
    }
    else {
      iVar2 = SS_IsDifValidOnDim(1,param_1);
      if ((iVar2 == 0) && (iVar2 = SS_IsDifValidOnDim(2,param_2), iVar2 == 0)) {
        return 0;
      }
      uVar3 = 1;
    }
  }
  return uVar3;
}


// ============================================================================
// Function: SS_IsDim1ValidForTouch @ 6baff157
// ============================================================================

undefined8 SS_IsDim1ValidForTouch(void)

{
  char cVar1;
  undefined8 uVar2;
  
  cVar1 = SS_IsDim1Valid();
  if ((cVar1 == '\0') || ((_refptr_g_tsaPrmtDynamic[0x26] & 1) == 0)) {
    uVar2 = 0;
  }
  else {
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: SS_IsDim2ValidForTouch @ 6baff18f
// ============================================================================

undefined8 SS_IsDim2ValidForTouch(void)

{
  char cVar1;
  undefined8 uVar2;
  
  cVar1 = SS_IsDim2Valid();
  if ((cVar1 == '\0') || ((_refptr_g_tsaPrmtDynamic[0x26] & 2) == 0)) {
    uVar2 = 0;
  }
  else {
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: SS_IsDifValidOnDim @ 6bb01c13
// ============================================================================

undefined4 SS_IsDifValidOnDim(int param_1,int param_2)

{
  byte bVar1;
  int local_20;
  int local_18;
  int local_14;
  int local_10;
  uint local_c;
  
  local_c = 0;
  if (param_1 == 1) {
    bVar1 = _refptr_g_tsaPrmtFlash->bCols;
  }
  else {
    local_c = (uint)_refptr_g_tsaPrmtFlash->bCols;
    bVar1 = _refptr_g_tsaPrmtFlash->bRows;
  }
  local_10 = bVar1 - 1;
  local_14 = param_2 + -1;
  if (local_14 < 0) {
    local_14 = 0;
  }
  local_18 = param_2 + 1;
  if (local_10 < local_18) {
    local_18 = local_10;
  }
  local_20 = local_14;
  while( true ) {
    if (local_18 <= local_20) {
      return 0;
    }
    if (199 < *(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)(local_20 + local_c + 1) * 4) +
              *(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)(local_c + local_20) * 4)) break;
    local_20 = local_20 + 1;
  }
  return 1;
}


// ============================================================================
// Function: SS_IsValidForBaseline @ 6baff1c7
// ============================================================================

undefined8 SS_IsValidForBaseline(void)

{
  char cVar1;
  
                    /* 0xbf1c7  1200  SS_IsValidForBaseline */
  cVar1 = SS_IsDim1ValidForBaseline();
  if ((cVar1 == '\0') && (cVar1 = SS_IsDim2ValidForBaseline(), cVar1 == '\0')) {
    return 0;
  }
  return 1;
}


// ============================================================================
// Function: SS_IsNoSignal @ 6bb01f41
// ============================================================================

undefined8 SS_IsNoSignal(void)

{
  byte local_16;
  byte local_15 [5];
  uint local_10;
  uint local_c;
  
                    /* 0xc1f41  1197  SS_IsNoSignal */
  SS_GetBaselineIdxRange(local_15,&local_16);
  local_c = (uint)local_15[0];
  while( true ) {
    if ((int)(uint)local_16 <= (int)local_c) {
      return 1;
    }
    local_10 = (uint)*(ushort *)(_refptr_g_tsaPrmtDynamic + 0x2c);
    if ((local_15[0] == local_c) || (local_16 - 1 == local_c)) {
      local_10 = (local_10 * 3) / 2;
    }
    if ((int)local_10 < *(int *)(_refptr_g_tsaBufSsDifForBl + (longlong)(int)local_c * 4)) break;
    local_c = local_c + 1;
  }
  return 0;
}


// ============================================================================
// Function: SS_IsNotMatchWithMutual @ 6ba4977c
// ============================================================================

undefined8 SS_IsNotMatchWithMutual(char param_1)

{
  int iVar1;
  bool bVar2;
  undefined8 uVar3;
  longlong lVar4;
  byte local_33;
  byte local_32;
  int local_30;
  int local_2c;
  longlong local_28;
  longlong local_20;
  longlong local_18;
  byte local_9;
  
                    /* 0x977c  1198  SS_IsNotMatchWithMutual */
  local_9 = 0;
  local_18 = 0;
  local_20 = 0;
  local_28 = 0;
  local_2c = 0;
  local_30 = 0;
  bVar2 = false;
  local_32 = _refptr_g_tsaPrmtFlash->bRows;
  local_33 = _refptr_g_tsaPrmtFlash->bCols;
  if (param_1 != '\0') {
    local_32 = _refptr_g_tsaPrmtFlash->bCols;
    local_33 = 0;
    if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) != 0) {
      local_32 = local_32 - 1;
      local_9 = 1;
    }
  }
  for (; local_9 < local_32; local_9 = local_9 + 1) {
    if (*(int *)(_refptr_g_tsaBufSelf +
                ((longlong)(int)((uint)local_33 + (uint)local_9) + 0x3c0) * 4) != 0) {
      bVar2 = true;
    }
    iVar1 = *(int *)(_refptr_g_tsaBufSSPreCMFDif +
                    (longlong)(int)((uint)local_33 + (uint)local_9) * 4);
    local_20 = local_20 +
               *(int *)(_refptr_g_tsaBufSelfForBlRst +
                       ((longlong)(int)((uint)local_33 + (uint)local_9) + 0x3c0) * 4) * iVar1;
    local_28 = local_28 + iVar1 * iVar1;
    local_2c = local_2c +
               *(int *)(_refptr_g_tsaBufSelfForBlRst +
                       ((longlong)(int)((uint)local_33 + (uint)local_9) + 0x3c0) * 4);
    local_30 = local_30 + iVar1;
  }
  lVar4 = (ulonglong)local_32 * local_28 - (longlong)local_30 * (longlong)local_30;
  if (lVar4 < 0) {
    lVar4 = lVar4 + 0x3ff;
  }
  if (lVar4 >> 10 != 0) {
    local_18 = (longlong)((ulonglong)local_32 * local_20 - (longlong)local_30 * (longlong)local_2c)
               / (lVar4 >> 10);
  }
  if (((bVar2) && (local_18 < -0x533)) && (-0x19000 < local_18)) {
    uVar3 = 1;
  }
  else {
    uVar3 = 0;
  }
  return uVar3;
}


