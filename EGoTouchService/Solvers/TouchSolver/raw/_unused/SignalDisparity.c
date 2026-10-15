// ============================================================================
// Function: SignalDisparity_PostProcess @ 6bb35d33
// ============================================================================

void SignalDisparity_PostProcess(void)

{
                    /* 0xf5d33  1418  SignalDisparity_PostProcess */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&DAT_6bb5ca27,"SignalDisparity_PostProcess");
  }
  if (g_tsaDifBufPtrSdBackup != 0) {
    *(longlong *)_refptr_g_tsaBufDif = g_tsaDifBufPtrSdBackup;
  }
  return;
}


// ============================================================================
// Function: SignalDisparity_Process @ 6bb35d84
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* Current config only calculates signal-disparity scaling; FEAT_SIGNAL_DISPARITY_POST is off, so
   compensation and SD buffer switching are skipped. */

void SignalDisparity_Process(void)

{
  char cConditionResult;
  int nSdScaleByScoring;
  int nSdScaleBySignalRatio;
  
                    /* 0xf5d84  1419  SignalDisparity_Process */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&DAT_6bb5ca27,"SignalDisparity_Process");
  }
  cConditionResult = SS_IsDim1Valid();
  if (cConditionResult != '\0') {
    cConditionResult = SS_IsDim2Valid();
    if (cConditionResult != '\0') {
      nSdScaleByScoring = SignalDisparity_GetSDScalingByScoring();
      nSdScaleBySignalRatio = SignalDisparity_GetSDScalingBySignalRatio();
      if (nSdScaleBySignalRatio < nSdScaleByScoring) {
        *(int *)&(*_refptr_g_tsaStaticPtr)[1].field_0x12 = nSdScaleByScoring;
      }
      else {
        *(int *)&(*_refptr_g_tsaStaticPtr)[1].field_0x12 = nSdScaleBySignalRatio;
      }
      if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SIGNAL_DISPARITY_POST) != 0) {
        SignalDisparity_Compensation();
      }
    }
  }
  return;
}


// ============================================================================
// Function: SignalDisparity_SwitchBuffer @ 6bb35e61
// ============================================================================

undefined4 SignalDisparity_SwitchBuffer(void)

{
  char cVar1;
  undefined4 local_c;
  
                    /* 0xf5e61  1420  SignalDisparity_SwitchBuffer */
  local_c = 0;
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SIGNAL_DISPARITY_POST) != 0) {
    cVar1 = SignalDisparity_IsSignalCompensated();
    if (cVar1 != '\0') {
      cVar1 = SignalDisparity_IsOKToProcess();
      if (cVar1 != '\0') {
        g_tsaDifBufPtrSdBackup = *(undefined8 *)_refptr_g_tsaBufDif;
        *(undefined8 *)_refptr_g_tsaBufDif = *(undefined8 *)_refptr_g_tsaBufSd;
        local_c = 1;
      }
    }
  }
  return local_c;
}


// ============================================================================
// Function: SignalDisparity_IsOKToProcess @ 6bb35e27
// ============================================================================

undefined8 SignalDisparity_IsOKToProcess(void)

{
  undefined8 uVar1;
  
                    /* 0xf5e27  1416  SignalDisparity_IsOKToProcess */
  if (((*_refptr_g_prevTouchesPtr)->field_0xd == '\0') &&
     (((*_refptr_g_prevTouchesPtr)->nTouchCount & 200U) != 0)) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: SignalDisparity_GetSDScalingByScoring @ 6bb355c6
// ============================================================================

undefined4 SignalDisparity_GetSDScalingByScoring(void)

{
  int iVar1;
  int iVar2;
  int local_28;
  int local_24;
  int local_20;
  int local_1c;
  int local_18;
  undefined4 local_14;
  int local_10;
  int local_c;
  
  local_14 = 0;
  local_18 = 0;
  local_1c = 0;
  iVar1 = memset_s(&g_weight,0x400,0,0x400);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Feature/SignalDisparity/SignalDisparity.c",0x3e,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  for (local_10 = 0; local_10 < (int)(uint)_refptr_g_tsaPrmtFlash->bRows; local_10 = local_10 + 1) {
    iVar1 = *(int *)(_refptr_g_tsaBufSSDif +
                    (longlong)(int)((uint)_refptr_g_tsaPrmtFlash->bCols + local_10) * 4);
    if (299 < iVar1) {
      for (local_c = 0; local_c < (int)(uint)_refptr_g_tsaPrmtFlash->bCols; local_c = local_c + 1) {
        if (299 < *(int *)(_refptr_g_tsaBufSSDif + (longlong)local_c * 4)) {
          local_1c = local_1c + 1;
          local_20 = (*(int *)(_refptr_g_tsaBufSSDif + (longlong)local_c * 4) * iVar1) / 0x90;
          if (local_20 == 0) {
            local_20 = 1;
          }
          iVar2 = (int)*(short *)((ulonglong)
                                  ((uint)_refptr_g_tsaPrmtFlash->bCols * local_10 + local_c & 0xffff
                                  ) * 2 + *(longlong *)_refptr_g_tsaBufDif);
          local_24 = (iVar2 * -0x100) / local_20;
          local_28 = (((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold - iVar2) * 0x100
                     ) / local_20;
          if (local_24 < 0) {
            local_24 = 0;
          }
          else if (0xff < local_24) {
            local_24 = 0xff;
          }
          *(int *)(&g_weight + (longlong)local_24 * 4) =
               *(int *)(&g_weight + (longlong)local_24 * 4) + local_20;
          if (local_28 < 0) {
            local_28 = 0;
          }
          else if (0xff < local_28) {
            local_28 = 0xff;
          }
          *(int *)(&g_weight + (longlong)local_28 * 4) =
               *(int *)(&g_weight + (longlong)local_28 * 4) + local_20;
          local_18 = local_18 + local_20;
        }
      }
    }
  }
  if (local_1c != 0) {
    local_14 = SignalDisparity_GetMinCoef(&g_weight,0x100,local_18);
  }
  return local_14;
}


// ============================================================================
// Function: SignalDisparity_GetSDScalingBySignalRatio @ 6bb35844
// ============================================================================

int SignalDisparity_GetSDScalingBySignalRatio(void)

{
  int iVar1;
  longlong local_28;
  longlong local_20;
  int local_18;
  int local_14;
  int local_10;
  int local_c;
  
  local_10 = 0;
  for (local_14 = 0; local_14 < (int)(uint)_refptr_g_tsaPrmtFlash->bCols; local_14 = local_14 + 1) {
    if (299 < *(int *)(_refptr_g_tsaBufSSDif + (longlong)local_14 * 4)) {
      local_10 = local_10 + *(int *)(_refptr_g_tsaBufSSDif + (longlong)local_14 * 4);
    }
  }
  local_20 = 0;
  local_28 = 0;
  for (local_18 = 0; local_18 < (int)(uint)_refptr_g_tsaPrmtFlash->bRows; local_18 = local_18 + 1) {
    iVar1 = *(int *)(_refptr_g_tsaBufSSDif +
                    (longlong)(int)((uint)_refptr_g_tsaPrmtFlash->bCols + local_18) * 4);
    if (299 < iVar1) {
      local_20 = local_20 +
                 iVar1 * *(int *)(_refptr_g_tsaBufSelf +
                                 ((longlong)(int)((uint)_refptr_g_tsaPrmtFlash->bCols + local_18) +
                                 0x280) * 4);
      local_28 = local_28 + iVar1 * iVar1;
    }
  }
  if ((local_28 == 0) || (local_10 == 0)) {
    local_c = 0;
  }
  else {
    local_c = ((((int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x36) << 8) / 100 -
               (int)((local_20 << 8) / local_28)) * 0x90) / local_10;
  }
  if (local_c < 0) {
    local_c = 0;
  }
  else if (10 < local_c) {
    local_c = 10;
  }
  return local_c;
}


// ============================================================================
// Function: SignalDisparity_Compensation @ 6bb35be9
// ============================================================================

void SignalDisparity_Compensation(void)

{
  int iVar1;
  undefined1 local_12;
  undefined1 local_11 [5];
  errno_t local_c;
  
  local_11[0] = 0;
  local_12 = 0;
  *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x16 =
       *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x16 & 0xfffffffe;
  if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x20000) == 0) {
    local_c = memcpy_s(*(void **)_refptr_g_tsaBufSd,(ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,
                       *(void **)_refptr_g_tsaBufDif,
                       (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
    if (local_c != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Feature/SignalDisparity/SignalDisparity.c",0xdb,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
    iVar1 = SignalDisparity_CompensationByScale
                      (*(undefined4 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x12,local_11,&local_12);
    if (iVar1 < (short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold * 2) {
      *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x16 =
           *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x16 | 1;
    }
  }
  return;
}


// ============================================================================
// Function: SignalDisparity_IsSignalCompensated @ 6bb35d1a
// ============================================================================

uint SignalDisparity_IsSignalCompensated(void)

{
                    /* 0xf5d1a  1417  SignalDisparity_IsSignalCompensated */
  return *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x16 & 1;
}


// ============================================================================
// Function: SignalDisparity_Init @ 6bb35510
// ============================================================================

void SignalDisparity_Init(void)

{
                    /* 0xf5510  1415  SignalDisparity_Init */
  *(undefined4 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x16 = 0;
  return;
}


// ============================================================================
// Function: SignalDisparity_GetMinCoef @ 6bb3552a
// ============================================================================

int SignalDisparity_GetMinCoef(longlong param_1,int param_2,int param_3)

{
  longlong local_18;
  int local_c;
  
  local_18 = 0;
  local_c = 0;
  while ((local_c < param_2 &&
         (local_18 = local_18 + *(int *)(param_1 + (longlong)local_c * 4),
         local_18 * 100 <= (longlong)(param_3 * 0x61)))) {
    local_c = local_c + 1;
  }
  local_c = local_c + -1;
  if (local_c < 0) {
    local_c = 0;
  }
  return local_c;
}


// ============================================================================
// Function: SignalDisparity_CompensationByScale @ 6bb359f7
// ============================================================================

int SignalDisparity_CompensationByScale(int param_1,undefined1 *param_2,undefined1 *param_3)

{
  int iVar1;
  byte bVar2;
  int iVar3;
  int local_14;
  uint local_10;
  uint local_c;
  
  local_14 = -0x80000000;
  bVar2 = SignalDisparity_GetExcludedEdge();
  for (local_10 = (uint)bVar2;
      (int)local_10 < (int)((uint)_refptr_g_tsaPrmtFlash->bRows - (uint)bVar2);
      local_10 = local_10 + 1) {
    iVar1 = *(int *)(_refptr_g_tsaBufSSDif +
                    (longlong)(int)(_refptr_g_tsaPrmtFlash->bCols + local_10) * 4);
    if ((0x77 < iVar1) &&
       (599 < *(int *)(_refptr_g_tsaBufSelf +
                      ((longlong)(int)(_refptr_g_tsaPrmtFlash->bCols + local_10) + 0x3c0) * 4))) {
      for (local_c = (uint)bVar2;
          (int)local_c < (int)((uint)_refptr_g_tsaPrmtFlash->bCols - (uint)bVar2);
          local_c = local_c + 1) {
        if ((0x77 < *(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)local_c * 4)) &&
           (599 < *(int *)(_refptr_g_tsaBufSelf + ((longlong)(int)local_c + 0x3c0) * 4))) {
          iVar3 = ((*(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)local_c * 4) * iVar1) / 0x90) *
                  param_1;
          if (iVar3 < 0) {
            iVar3 = iVar3 + 0xff;
          }
          iVar3 = (iVar3 >> 8) +
                  (int)*(short *)((ulonglong)
                                  (_refptr_g_tsaPrmtFlash->bCols * local_10 + local_c & 0xffff) * 2
                                 + *(longlong *)_refptr_g_tsaBufDif);
          *(short *)((ulonglong)(_refptr_g_tsaPrmtFlash->bCols * local_10 + local_c & 0xffff) * 2 +
                    *(longlong *)_refptr_g_tsaBufSd) = (short)iVar3;
          if (local_14 < iVar3) {
            *param_2 = (char)local_c;
            *param_3 = (char)local_10;
            local_14 = iVar3;
          }
        }
      }
    }
  }
  return local_14;
}


// ============================================================================
// Function: SignalDisparity_GetExcludedEdge @ 6bb359cc
// ============================================================================

undefined8 SignalDisparity_GetExcludedEdge(void)

{
  undefined8 uVar1;
  
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) == 0) {
    uVar1 = 0;
  }
  else {
    uVar1 = 2;
  }
  return uVar1;
}


