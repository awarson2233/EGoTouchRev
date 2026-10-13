// ============================================================================
// Function: SafeBaseline_SetBuf @ 6bb02f38
// ============================================================================

void SafeBaseline_SetBuf(undefined8 param_1)

{
  undefined8 local_38;
  undefined8 local_30;
  undefined8 local_28;
  undefined8 local_20;
  undefined8 local_18;
  
                    /* 0xc2f38  1246  SafeBaseline_SetBuf */
  local_38 = 0;
  local_30 = 0;
  local_28 = 0;
  local_20 = 0;
  local_18 = 0;
  SafeBaseline_InitItem(&local_38);
  local_18 = param_1;
  BaselineQueue_Push(&local_38);
  return;
}


// ============================================================================
// Function: SafeBaseline_Process @ 6bb046be
// ============================================================================

void SafeBaseline_Process(void)

{
  char cVar1;
  
                    /* 0xc46be  1242  SafeBaseline_Process */
  DAT_6bc8d04c = DAT_6bc8d048;
  DAT_6bc8d048 = DAT_6bc8d044;
  DAT_6bc8d044 = 0;
  SafeBaseline_CheckTouch();
  SafeBaseline_CheckTrustability();
  SafeBaseline_CheckSafeDiff();
  cVar1 = SafeBaseline_IsOKToUpdate();
  if (cVar1 != '\0') {
    cVar1 = SafeBaseline_IsValid();
    if (cVar1 == '\0') {
      cVar1 = SafeBaseline_IsOKToReset();
      if (cVar1 != '\0') {
        SafeBaseline_Push();
        DAT_6bc8d044 = DAT_6bc8d044 | 3;
      }
    }
    else {
      cVar1 = SafeBaseline_IsOKToPush();
      if (cVar1 != '\0') {
        SafeBaseline_Push();
        DAT_6bc8d044 = DAT_6bc8d044 | 1;
      }
    }
  }
  if (_refptr_g_tsaPrpt->dwPrevBaselinePropertyState == 7) {
    SafeBaseline_Reset();
  }
  cVar1 = SafeBaseline_IsToDoReset();
  if (cVar1 != '\0') {
    SafeBaseline_Reset();
  }
  return;
}


// ============================================================================
// Function: SafeBaseline_ScreenOnWithPhoneHeldInHandProcess @ 6bb04998
// ============================================================================

/* WARNING: Function: ___chkstk_ms replaced with injection: alloca_probe */

uint SafeBaseline_ScreenOnWithPhoneHeldInHandProcess(void)

{
  uchar uVar1;
  ushort uVar2;
  void *_Dst;
  longlong lVar3;
  char cVar4;
  uint uVar5;
  void *_Src;
  undefined8 auStack_a0 [5];
  undefined8 auStack_78 [2];
  short asStack_68 [2];
  errno_t local_64;
  void *local_60;
  longlong local_58;
  longlong local_50;
  longlong local_48;
  ushort local_3c;
  byte local_3a;
  byte local_39;
  
                    /* 0xc4998  1245  SafeBaseline_ScreenOnWithPhoneHeldInHandProcess */
  auStack_a0[0] = 0x6bb049b1;
  cVar4 = TSAStatic_IsHWResetPrev();
  if (cVar4 != '\0') {
    auStack_a0[0] = 0x6bb049ba;
    cVar4 = TSAStatic_IsHWReset();
    if (cVar4 == '\0') goto LAB_6bb049e9;
  }
  if (_refptr_g_tsaPrpt->dwPrevBaselinePropertyState != 0x10) {
    return _refptr_g_tsaPrpt->dwPrevBaselinePropertyState;
  }
  if ((DAT_6bc8d050 & 4) == 0) {
    return 0;
  }
LAB_6bb049e9:
  auStack_a0[0] = 0x6bb049f4;
  cVar4 = SafeBaseline_IsToReplaceCurrentBaseline();
  if (cVar4 == '\0') {
    uVar5 = 0;
  }
  else {
    local_3c = 0;
    auStack_a0[0] = 0x6bb04a0d;
    local_48 = BaselineQueue_GetLastestItem();
    local_50 = *(undefined8 *)(local_48 + 0x20);
    auStack_a0[0] = 0x6bb04a3a;
    local_50 = TSAFold_GridZoomInRemap(0,local_50,2,0);
    DAT_6bc8d050 = DAT_6bc8d050 | 4;
    uVar2 = *(ushort *)_refptr_g_gridBufSize;
    local_58 = (ulonglong)uVar2 - 1;
    auStack_a0[0] = 0x6bb04a9e;
    lVar3 = -((ulonglong)uVar2 * 2 + 0x10 & 0xfffffffffffffff0);
    _Src = (void *)((longlong)asStack_68 + lVar3);
    for (local_3a = 0; local_3a < _refptr_g_tsaPrmtFlash->bRows; local_3a = local_3a + 1) {
      for (local_39 = 0; local_39 < _refptr_g_tsaPrmtFlash->bCols; local_39 = local_39 + 1) {
        *(short *)((longlong)_Src + (longlong)(int)(uint)local_3c * 2) =
             (*(short *)(local_50 + (ulonglong)local_3c * 2) -
             *(short *)(g_tsaBufSafeBL2BLDif + (ulonglong)local_3c * 2)) +
             *(short *)(g_tsaBufSafeBL2BLDifCMF + (ulonglong)local_3c * 2);
        local_3c = local_3c + 1;
      }
    }
    _Dst = *(void **)_refptr_g_tsaBufBl;
    local_60 = _Src;
    *(undefined8 *)((longlong)auStack_a0 + lVar3) = 0x6bb04b85;
    local_64 = memcpy_s(_Dst,0x2648,_Src,(ulonglong)uVar2 * 2);
    if (local_64 != 0) {
      *(char **)((longlong)auStack_78 + lVar3) = "rc == EOK";
      *(undefined8 *)((longlong)auStack_a0 + lVar3) = 0x6bb04bb8;
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/SafeBaseline/SafeBaseline.c",0x3ec);
      *(undefined8 *)((longlong)auStack_a0 + lVar3) = 0x6bb04bbd;
      HardwareAnalyzer_SetSoftException();
    }
    if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) != 0) {
      for (local_3a = 0; local_3a < _refptr_g_tsaPrmtFlash->bRows; local_3a = local_3a + 1) {
        *(undefined2 *)(_refptr_g_sideData + (longlong)(int)(uint)local_3a * 0x30 + 0x12) =
             *(undefined2 *)
              ((longlong)local_60 +
              (longlong)(int)((uint)local_3a * (uint)_refptr_g_tsaPrmtFlash->bCols) * 2);
      }
      uVar1 = _refptr_g_tsaPrmtFlash->bCols;
      for (local_3a = 0; local_3a < _refptr_g_tsaPrmtFlash->bRows; local_3a = local_3a + 1) {
        *(undefined2 *)(_refptr_g_sideData + (longlong)(int)(uint)local_3a * 0x30 + 0x109a) =
             *(undefined2 *)
              ((longlong)local_60 +
              (longlong)
              (int)((uint)(byte)(uVar1 - 1) + (uint)local_3a * (uint)_refptr_g_tsaPrmtFlash->bCols &
                   0xffff) * 2);
      }
    }
    uVar5 = 1;
  }
  return uVar5;
}


// ============================================================================
// Function: SafeBaseline_InitItem @ 6bb02eaa
// ============================================================================

void SafeBaseline_InitItem(undefined8 *param_1)

{
  undefined2 uVar1;
  undefined4 uVar2;
  
                    /* 0xc2eaa  1228  SafeBaseline_InitItem */
  *param_1 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  *(int *)(param_1 + 2) = (*_refptr_g_tsaStaticPtr)->nField_0x26c;
  *(int *)((longlong)param_1 + 0x14) = (*_refptr_g_tsaStaticPtr)->nField_0x270;
  param_1[1] = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  uVar2 = TSAFold_GetCurrState();
  *(undefined4 *)((longlong)param_1 + 0x1c) = uVar2;
  uVar1 = AFE_GetScanFreq();
  *(undefined2 *)(param_1 + 3) = uVar1;
  return;
}


// ============================================================================
// Function: SafeBaseline_IsSignalStable @ 6bb02c88
// ============================================================================

bool SafeBaseline_IsSignalStable(void)

{
                    /* 0xc2c88  1236  SafeBaseline_IsSignalStable */
  return 4 < DAT_6bc8d062;
}


// ============================================================================
// Function: SafeBaseline_IsReset @ 6bb02cb6
// ============================================================================

bool SafeBaseline_IsReset(void)

{
                    /* 0xc2cb6  1231  SafeBaseline_IsReset */
  return (DAT_6bc8d044 & 2) != 0;
}


// ============================================================================
// Function: SafeBaseline_IsToDoReset @ 6bb02cce
// ============================================================================

bool SafeBaseline_IsToDoReset(void)

{
                    /* 0xc2cce  1237  SafeBaseline_IsToDoReset */
  return (DAT_6bc8d04c & 2) != 0;
}


// ============================================================================
// Function: SafeBaseline_Push @ 6bb02f9e
// ============================================================================

void SafeBaseline_Push(void)

{
  undefined8 local_38;
  undefined8 local_30;
  undefined8 local_28;
  undefined8 local_20;
  undefined8 local_18;
  
  local_38 = 0;
  local_30 = 0;
  local_28 = 0;
  local_20 = 0;
  local_18 = 0;
  SafeBaseline_InitItem(&local_38);
  local_18 = *(undefined8 *)_refptr_g_tsaBufBl;
  BaselineQueue_Push(&local_38);
  DAT_6bc8d054 = 0;
  DAT_6bc8d061 = DAT_6bc8d061 + '\x01';
  return;
}


// ============================================================================
// Function: SafeBaseline_IsOKToPush @ 6bb03020
// ============================================================================

undefined8 SafeBaseline_IsOKToPush(void)

{
  char cVar1;
  undefined8 uVar2;
  
  cVar1 = BaselineQueue_IsFull();
  if (cVar1 == '\0') {
    uVar2 = 1;
  }
  else {
    cVar1 = SafeBaseline_IsTrustableScreenOnSource();
    if (cVar1 == '\0') {
      uVar2 = 0;
    }
    else if (DAT_6bc8d061 < 3) {
      uVar2 = 1;
    }
    else {
      uVar2 = 0;
    }
  }
  return uVar2;
}


// ============================================================================
// Function: SafeBaseline_CheckTrustability @ 6bb03069
// ============================================================================

void SafeBaseline_CheckTrustability(void)

{
  byte local_9;
  
  if (1 < *(byte *)(*(longlong *)_refptr_g_touchesPtr + 8)) {
    g_safeBaseline = g_safeBaseline | 1;
  }
  for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
    if (*(int *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x2a0)
        == 1) {
      DAT_6bc8d054 = DAT_6bc8d054 + 1;
      DAT_6bc8d058 = DAT_6bc8d058 + 1;
    }
    else if (*(int *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x2a0) == 2) {
      DAT_6bc8d05c = DAT_6bc8d05c + 1;
    }
  }
  return;
}


// ============================================================================
// Function: SafeBaseline_IsOKToUpdate @ 6bb031c3
// ============================================================================

undefined8 SafeBaseline_IsOKToUpdate(void)

{
  char cVar1;
  undefined8 uVar2;
  
  cVar1 = SafeBaseline_IsJustOKToEnterIDLE();
  if ((((cVar1 == '\0') || (DAT_6bc8d05c != 0)) || (DAT_6bc8d054 < 5)) ||
     ((g_safeBaseline != 0 ||
      (_refptr_g_tsaPrmtFlash->wBufElementCount >> 3 <= *(ushort *)&_refptr_g_tsaPrpt->field_0x118))
     )) {
    uVar2 = 0;
  }
  else {
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: SafeBaseline_IsValid @ 6bb03ab5
// ============================================================================

undefined1 SafeBaseline_IsValid(void)

{
  char cVar1;
  ushort uVar2;
  byte local_1a;
  
  local_1a = 0;
  while( true ) {
    uVar2 = BaselineQueue_Count();
    if (uVar2 <= local_1a) {
      return 1;
    }
    cVar1 = SafeBaseline_IsUnitValid(local_1a);
    if (cVar1 == '\0') break;
    local_1a = local_1a + 1;
  }
  return 0;
}


// ============================================================================
// Function: SafeBaseline_Reset @ 6bb043c9
// ============================================================================

void SafeBaseline_Reset(void)

{
  BaselineQueue_Reset();
  DAT_6bc8d061 = 0;
  return;
}


// ============================================================================
// Function: SafeBaseline_CheckTouch @ 6bb043e7
// ============================================================================

void SafeBaseline_CheckTouch(void)

{
  char cVar1;
  byte local_9;
  
                    /* 0xc43e7  1219  SafeBaseline_CheckTouch */
  cVar1 = SafeBaseline_IsSignalStable();
  if (cVar1 != '\0') {
    for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
      cVar1 = Peak_IsTouchValid(local_9);
      if (cVar1 != '\0') {
        *(char *)(*(longlong *)_refptr_g_touchesPtr + 0xf) =
             *(char *)(*(longlong *)_refptr_g_touchesPtr + 0xf) + '\x01';
        cVar1 = SafeBaseline_CheckAbnormalTouch(local_9);
        if ((cVar1 != '\0') ||
           ((*(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x1f0) & 0x1000000) != 0)) {
          *(char *)(*(longlong *)_refptr_g_touchesPtr + 0xe) =
               *(char *)(*(longlong *)_refptr_g_touchesPtr + 0xe) + '\x01';
          *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 500)
               = *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 500) | 0x100;
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: SafeBaseline_CheckSafeDiff @ 6bb044ef
// ============================================================================

void SafeBaseline_CheckSafeDiff(void)

{
  if (((DAT_6bc8d058 != 0) && (*(char *)(*(longlong *)_refptr_g_touchesPtr + 8) == '\0')) &&
     (*(ushort *)&_refptr_g_tsaPrpt->field_0x1c4 < 0x32)) {
    DAT_6bc8d050 = DAT_6bc8d050 | 2;
  }
  return;
}


// ============================================================================
// Function: SafeBaseline_IsOKToReset @ 6bb0467b
// ============================================================================

undefined8 SafeBaseline_IsOKToReset(void)

{
  char cVar1;
  undefined8 uVar2;
  
  cVar1 = SafeBaseline_IsSafeRawChecked();
  if (cVar1 == '\0') {
    cVar1 = SafeBaseline_IsNewBaselineTrustableEnough();
    if (cVar1 == '\0') {
      cVar1 = SafeBaseline_IsOldBaselineTrustableEnough();
      if (cVar1 == '\0') {
        uVar2 = 1;
      }
      else {
        uVar2 = 0;
      }
    }
    else {
      uVar2 = 1;
    }
  }
  else {
    uVar2 = 0;
  }
  return uVar2;
}


// ============================================================================
// Function: SafeBaseline_IsToReplaceCurrentBaseline @ 6bb04828
// ============================================================================

undefined8 SafeBaseline_IsToReplaceCurrentBaseline(void)

{
  char cVar1;
  longlong lVar2;
  undefined8 uVar3;
  
  if ((_refptr_g_tsaPrmtFlash[2].field_0x6c3 != '\0') &&
     (cVar1 = SafeBaseline_IsDoubleChecked(), cVar1 != '\0')) {
    lVar2 = BaselineQueue_GetLastestItem();
    uVar3 = TSAFold_GridZoomInRemap(0,*(undefined8 *)(lVar2 + 0x20),2,0);
    SafeBaseline_BufferComparison
              (*(undefined8 *)_refptr_g_tsaBufBl,uVar3,g_tsaBufSafeBL2BLDif,g_tsaBufSafeBL2BLDifCMF,
               (int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x46),
               (int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x42),&DAT_6bc8d0dc,1);
    if ((((4 < DAT_6bc8d146) && ((4 < DAT_6bc8d144 && (-(int)DAT_6bc8d13e < (int)DAT_6bc8d13c)))) ||
        ((4 < DAT_6bc8d152 && ((4 < DAT_6bc8d150 && (-(int)DAT_6bc8d14a < (int)DAT_6bc8d148)))))) &&
       (DAT_6bc8d120 < 5)) {
      return 1;
    }
  }
  return 0;
}


// ============================================================================
// Function: SafeBaseline_CheckSideTouch @ 6bb04de1
// ============================================================================

void SafeBaseline_CheckSideTouch(void)

{
  char cVar1;
  byte local_9;
  
                    /* 0xc4de1  1218  SafeBaseline_CheckSideTouch */
  cVar1 = SafeBaseline_IsSignalStable();
  if (cVar1 != '\0') {
    for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
      cVar1 = Peak_IsTouchValid(local_9);
      if (cVar1 != '\0') {
        *(char *)(*(longlong *)_refptr_g_touchesPtr + 0xf) =
             *(char *)(*(longlong *)_refptr_g_touchesPtr + 0xf) + '\x01';
        cVar1 = SafeBaseline_CheckAbnormalSensor
                          ((*_refptr_g_peaksPtr)
                           [(longlong)
                            (int)(uint)*(byte *)((longlong)(int)(uint)local_9 * 0x168 +
                                                 *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 +
                            0x15].bPeakCount,
                           (*_refptr_g_peaksPtr)
                           [(longlong)
                            (int)(uint)*(byte *)((longlong)(int)(uint)local_9 * 0x168 +
                                                 *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 +
                            0x16].bPeakCount);
        if (cVar1 != '\0') {
          *(char *)(*(longlong *)_refptr_g_touchesPtr + 0xe) =
               *(char *)(*(longlong *)_refptr_g_touchesPtr + 0xe) + '\x01';
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: SafeBaseline_IsPushed @ 6bb02c9e
// ============================================================================

bool SafeBaseline_IsPushed(void)

{
                    /* 0xc2c9e  1230  SafeBaseline_IsPushed */
  return (DAT_6bc8d044 & 1) != 0;
}


// ============================================================================
// Function: SafeBaseline_IsAvailable @ 6bb02ce6
// ============================================================================

undefined8 SafeBaseline_IsAvailable(void)

{
  char cVar1;
  ushort uVar2;
  int iVar3;
  longlong lVar4;
  undefined8 uVar5;
  byte local_19;
  
                    /* 0xc2ce6  1229  SafeBaseline_IsAvailable */
  cVar1 = TSAFold_IsSupported();
  if (cVar1 != '\0') {
    for (local_19 = 0; uVar2 = BaselineQueue_Count(), local_19 < uVar2; local_19 = local_19 + 1) {
      lVar4 = BaselineQueue_GetIndexedItem(local_19);
      iVar3 = TSAFold_GetCurrState();
      if ((iVar3 == 0) && (*(int *)(lVar4 + 0x1c) == 1)) {
        return 0;
      }
    }
  }
  uVar5 = BaselineQueue_Count();
  return uVar5;
}


// ============================================================================
// Function: SafeBaseline_Init @ 6bb02b65
// ============================================================================

void SafeBaseline_Init(char param_1)

{
  int iVar1;
  byte local_9;
  
                    /* 0xc2b65  1227  SafeBaseline_Init */
  iVar1 = memset_s(&g_safeBaseline,0x114,0,0x114);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SafeBaseline/SafeBaseline.c",0x7a,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  if (param_1 != '\0') {
    for (local_9 = 0; local_9 < 5; local_9 = local_9 + 1) {
      SafeBaseline_SafetyUnitsInit(&g_blSafetyUnits + (longlong)(int)(uint)local_9 * 0x28);
    }
    BaselineQueue_Init(&g_blSafetyUnits,5,&g_blSafetyTempUnits,5);
  }
  SafeBaseline_ResetInit();
  return;
}


// ============================================================================
// Function: SafeBaseline_IsTrustableScreenOnSource @ 6bb02f93
// ============================================================================

undefined8 SafeBaseline_IsTrustableScreenOnSource(void)

{
  return 1;
}


// ============================================================================
// Function: SafeBaseline_IsJustOKToEnterIDLE @ 6bb0315e
// ============================================================================

undefined8 SafeBaseline_IsJustOKToEnterIDLE(void)

{
  char cVar1;
  undefined8 uVar2;
  
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) == 0) {
    if ((_refptr_g_tsaPrpt->dwBaselineStage == 0) && (_refptr_g_tsaPrpt->dwPrevBaselineStage != 0))
    {
      return 1;
    }
  }
  else {
    cVar1 = TSA_IsGripOKToIDLE();
    if (cVar1 == '\0') {
      uVar2 = SideTouch_IsJustOKToEnterIDLEMode();
      return uVar2;
    }
  }
  return 0;
}


// ============================================================================
// Function: SafeBaseline_IsUnitValid @ 6bb039bd
// ============================================================================

bool SafeBaseline_IsUnitValid(undefined1 param_1)

{
  int iVar1;
  char cVar2;
  int iVar3;
  undefined1 local_a8 [92];
  short local_4c;
  undefined8 local_28;
  longlong local_20;
  
  local_20 = BaselineQueue_GetIndexedItem(param_1);
  local_28 = *(undefined8 *)(local_20 + 0x20);
  cVar2 = TSAFold_IsSupported();
  if (((cVar2 != '\0') && (*(int *)(local_20 + 0x1c) != 0)) &&
     (iVar1 = *(int *)(local_20 + 0x1c), iVar3 = TSAFold_GetCurrState(), iVar1 != iVar3)) {
    return false;
  }
  local_28 = TSAFold_GridZoomInRemap(0,local_28,2,0);
  SafeBaseline_BufferComparison
            (*(undefined8 *)_refptr_g_tsaBufBl,local_28,0,0,
             (int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x46),
             (int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x42),local_a8,0);
  return local_4c == 0;
}


// ============================================================================
// Function: SafeBaseline_CheckAbnormalTouch @ 6bb03d1c
// ============================================================================

void SafeBaseline_CheckAbnormalTouch(byte param_1)

{
  SafeBaseline_CheckAbnormalPeak
            (*(undefined1 *)
              ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x23d));
  return;
}


// ============================================================================
// Function: SafeBaseline_IsSafeRawChecked @ 6bb04542
// ============================================================================

bool SafeBaseline_IsSafeRawChecked(void)

{
                    /* 0xc4542  1235  SafeBaseline_IsSafeRawChecked */
  return (DAT_6bc8d050 & 1) != 0;
}


// ============================================================================
// Function: SafeBaseline_IsNewBaselineTrustableEnough @ 6bb0461d
// ============================================================================

bool SafeBaseline_IsNewBaselineTrustableEnough(void)

{
  char cVar1;
  
  BaselineQueue_SwitchToTemp();
  cVar1 = SafeBaseline_IsValid();
  if (cVar1 == '\0') {
    BaselineQueue_Reset();
  }
  else {
    cVar1 = SafeBaseline_IsOKToPush();
    if (cVar1 != '\0') {
      SafeBaseline_Push();
    }
  }
  cVar1 = SafeBaseline_IsTrustableEnoughCommon();
  BaselineQueue_SwitchToNormal();
  return cVar1 != '\0';
}


// ============================================================================
// Function: SafeBaseline_IsOldBaselineTrustableEnough @ 6bb04668
// ============================================================================

void SafeBaseline_IsOldBaselineTrustableEnough(void)

{
  SafeBaseline_IsTrustableEnoughCommon();
  return;
}


// ============================================================================
// Function: SafeBaseline_BufferComparison @ 6bb036da
// ============================================================================

void SafeBaseline_BufferComparison
               (short *param_1,short *param_2,void *param_3,void *param_4,short param_5,
               short param_6,longlong param_7,undefined1 param_8)

{
  errno_t eVar1;
  
  eVar1 = memcpy_s(g_tsaBufStack,(ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2,
                   *(void **)_refptr_g_tsaBufDif,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SafeBaseline/SafeBaseline.c",0x1d2,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  BLIIR_CalcDiffCommon(*(short **)_refptr_g_tsaBufDif,param_1,param_2);
  TPSensor_Process();
  if (param_7 != 0) {
    SafeBaseline_GetPrpt(*(undefined8 *)_refptr_g_tsaBufDif,(int)param_5,(int)param_6,param_7);
  }
  if (param_3 != (void *)0x0) {
    eVar1 = memcpy_s(param_3,(ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2,
                     *(void **)_refptr_g_tsaBufDif,
                     (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
    if (eVar1 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/SafeBaseline/SafeBaseline.c",0x1e0,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
  }
  CMF_ProcessDimCommon
            (1,15000,500,_refptr_g_tsaPrmtFlash->bCols,_refptr_g_tsaPrmtFlash->bRows,param_8);
  if (param_7 != 0) {
    SafeBaseline_GetPrpt
              (*(undefined8 *)_refptr_g_tsaBufDif,(int)param_5,(int)param_6,param_7 + 0x3c);
  }
  if (param_4 != (void *)0x0) {
    eVar1 = memcpy_s(param_4,(ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2,
                     *(void **)_refptr_g_tsaBufDif,
                     (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
    if (eVar1 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/SafeBaseline/SafeBaseline.c",0x1ed,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
  }
  eVar1 = memcpy_s(*(void **)_refptr_g_tsaBufDif,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2,g_tsaBufStack,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SafeBaseline/SafeBaseline.c",500,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: SafeBaseline_IsDoubleChecked @ 6bb047b2
// ============================================================================

bool SafeBaseline_IsDoubleChecked(void)

{
  ushort uVar1;
  longlong *plVar2;
  byte local_22;
  char local_21;
  longlong local_20;
  
  local_20 = 0;
  local_21 = '\0';
  local_22 = 0;
  while( true ) {
    uVar1 = BaselineQueue_Count();
    if (uVar1 <= local_22) break;
    plVar2 = (longlong *)BaselineQueue_GetIndexedItem(local_22);
    if (*plVar2 != local_20) {
      local_21 = local_21 + '\x01';
      plVar2 = (longlong *)BaselineQueue_GetIndexedItem(local_22);
      local_20 = *plVar2;
    }
    local_22 = local_22 + 1;
  }
  return local_21 != '\0';
}


// ============================================================================
// Function: SafeBaseline_CheckAbnormalSensor @ 6bb03c24
// ============================================================================

undefined8 SafeBaseline_CheckAbnormalSensor(byte param_1,byte param_2)

{
  char cVar1;
  short sVar2;
  undefined8 uVar3;
  
                    /* 0xc3c24  1215  SafeBaseline_CheckAbnormalSensor */
  if ((param_1 < _refptr_g_tsaPrmtFlash->bCols) && (param_2 < _refptr_g_tsaPrmtFlash->bRows)) {
    sVar2 = BaselineQueue_Count();
    if (sVar2 == 0) {
      uVar3 = 0;
    }
    else {
      cVar1 = SafeBaseline_IsAbnormalSensor
                        ((ushort)param_1 + (ushort)param_2 * (ushort)_refptr_g_tsaPrmtFlash->bCols);
      if (cVar1 == '\0') {
        uVar3 = 0;
      }
      else {
        uVar3 = 1;
      }
    }
  }
  else {
    uVar3 = 0;
  }
  return uVar3;
}


// ============================================================================
// Function: SafeBaseline_SafetyUnitsInit @ 6bb022b1
// ============================================================================

void SafeBaseline_SafetyUnitsInit(undefined8 *param_1)

{
  int iVar1;
  
                    /* 0xc22b1  1244  SafeBaseline_SafetyUnitsInit */
  *param_1 = 0;
  param_1[1] = 0;
  *(undefined4 *)(param_1 + 2) = 0;
  *(undefined4 *)((longlong)param_1 + 0x14) = 0;
  *(undefined2 *)(param_1 + 3) = 0;
  *(undefined4 *)((longlong)param_1 + 0x1c) = 0;
  iVar1 = memset_s(param_1[4],(ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,0,
                   (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SafeBaseline/BaselineQueue/BaselineQueue.c",0x49,
                      "rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: SafeBaseline_ResetInit @ 6bb04f10
// ============================================================================

void SafeBaseline_ResetInit(void)

{
  int iVar1;
  
                    /* 0xc4f10  1243  SafeBaseline_ResetInit */
  iVar1 = memset_s(&g_safeBaselineReset,0x34,0,0x34);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SafeBaseline/SafeBaselineReset.c",0x1c,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: SafeBaseline_CheckAbnormalPeak @ 6bb03cac
// ============================================================================

void SafeBaseline_CheckAbnormalPeak(byte param_1)

{
  SafeBaseline_CheckAbnormalSensor
            (_refptr_g_peaksBeforeSwitchDifBuffer[(longlong)(int)(uint)param_1 * 0x14 + 0x15],
             _refptr_g_peaksBeforeSwitchDifBuffer[(longlong)(int)(uint)param_1 * 0x14 + 0x16]);
  return;
}


// ============================================================================
// Function: SafeBaseline_IsTrustableEnoughCommon @ 6bb04588
// ============================================================================

undefined8 SafeBaseline_IsTrustableEnoughCommon(void)

{
  ushort uVar1;
  longlong lVar2;
  undefined8 uVar3;
  byte local_22;
  byte local_21;
  longlong local_20;
  
  uVar1 = BaselineQueue_Count();
  if (uVar1 < 2) {
    uVar3 = 0;
  }
  else {
    local_20 = 0;
    local_21 = 0;
    local_22 = 0;
    while( true ) {
      uVar1 = BaselineQueue_Count();
      if (uVar1 <= local_22) break;
      lVar2 = BaselineQueue_GetIndexedItem(local_22);
      if (*(longlong *)(lVar2 + 8) != local_20) {
        local_21 = local_21 + 1;
        lVar2 = BaselineQueue_GetIndexedItem(local_22);
        local_20 = *(longlong *)(lVar2 + 8);
      }
      local_22 = local_22 + 1;
    }
    if (local_21 < 2) {
      uVar3 = 0;
    }
    else {
      uVar3 = 1;
    }
  }
  return uVar3;
}


// ============================================================================
// Function: SafeBaseline_GetPrpt @ 6bb03629
// ============================================================================

void SafeBaseline_GetPrpt(undefined8 param_1,short param_2,short param_3,undefined8 param_4)

{
  byte local_a;
  byte local_9;
  
  SafeBaseline_ClrPrpts(param_4);
  for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bRows; local_9 = local_9 + 1) {
    for (local_a = 0; local_a < _refptr_g_tsaPrmtFlash->bCols; local_a = local_a + 1) {
      if ((local_a < _refptr_g_tsaPrmtFlash->bCols) && (local_9 < _refptr_g_tsaPrmtFlash->bRows)) {
        SafeBaseline_GetPrptSensor(local_a,local_9,(int)param_2,(int)param_3,param_4);
      }
    }
  }
  return;
}


// ============================================================================
// Function: SafeBaseline_IsAbnormalSensor @ 6bb03b5c
// ============================================================================

undefined8 SafeBaseline_IsAbnormalSensor(ushort param_1)

{
  uint uVar1;
  uint uVar2;
  undefined8 uVar3;
  uint uVar4;
  uint uVar5;
  
  if (DAT_6bc8d0c0 < DAT_6bc8d084) {
    uVar1 = (uint)*(short *)(g_tsaBufSafeBL2BLDifCMF + (ulonglong)param_1 * 2);
    uVar4 = (int)uVar1 >> 0x1f;
    uVar2 = (uint)*(short *)(g_tsaBufSafeBL2RawDifCMF + (ulonglong)param_1 * 2);
    uVar5 = (int)uVar2 >> 0x1f;
    if ((int)((uVar2 ^ uVar5) - uVar5) < (int)((uVar1 ^ uVar4) - uVar4)) {
      uVar3 = 1;
    }
    else {
      uVar3 = 0;
    }
  }
  else {
    uVar1 = (uint)*(short *)(g_tsaBufSafeBL2BLDif + (ulonglong)param_1 * 2);
    uVar4 = (int)uVar1 >> 0x1f;
    uVar2 = (uint)*(short *)(g_tsaBufSafeBL2RawDif + (ulonglong)param_1 * 2);
    uVar5 = (int)uVar2 >> 0x1f;
    if ((int)((uVar2 ^ uVar5) - uVar5) < (int)((uVar1 ^ uVar4) - uVar4)) {
      uVar3 = 1;
    }
    else {
      uVar3 = 0;
    }
  }
  return uVar3;
}


// ============================================================================
// Function: SafeBaseline_IsResetTriggered @ 6bb0592b
// ============================================================================

bool SafeBaseline_IsResetTriggered(void)

{
  char cVar1;
  char cVar2;
  int iVar3;
  bool local_9;
  
                    /* 0xc592b  1233  SafeBaseline_IsResetTriggered */
  cVar1 = BLReset_IsLongTimeRawJumpDetected();
  if (cVar1 != '\0') {
    _refptr_g_tsaPrpt->dwBaselinePropertyState = 7;
  }
  cVar2 = BLReset_IsSmartCoverOff();
  if (cVar2 != '\0') {
    _refptr_g_tsaPrpt->dwBaselinePropertyState = 0xc;
  }
  local_9 = cVar2 != '\0' || cVar1 != '\0';
  cVar1 = BLReset_IsAllFreqsBaselineInitializedDuringTouchLiftOff();
  if (cVar1 != '\0') {
    local_9 = true;
    _refptr_g_tsaPrpt->dwBaselinePropertyState = 4;
  }
  cVar1 = SensorData_AntiTouchStageTimeout();
  if (cVar1 != '\0') {
    local_9 = true;
    _refptr_g_tsaPrpt->dwBaselinePropertyState = 0x17;
  }
  cVar1 = AFE_IsRawJump();
  if (((cVar1 != '\0') &&
      (_refptr_g_tsaPrmtFlash->wBufElementCount >> 2 < *(ushort *)&_refptr_g_tsaPrpt->field_0x4)) &&
     ((*_refptr_g_prevTouchesPtr)->field_0x6 == '\0')) {
    local_9 = true;
    _refptr_g_tsaPrpt->dwBaselinePropertyState = 0x10;
  }
  iVar3 = SafeBaseline_ResetCheck();
  if (iVar3 != 0) {
    local_9 = true;
    _refptr_g_tsaPrpt->dwBaselinePropertyState = 0x14;
    if ((*_refptr_g_prevTouchesPtr)->nTouchCount == 0x2000) {
      (*_refptr_g_tsaStaticPtr)[1].field_0x3 = 1;
      _refptr_g_tsaPrpt->dwBaselinePropertyState = 8;
    }
  }
  if (local_9 != false) {
    (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
         (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 1;
  }
  (*_refptr_g_tsaStaticPtr)->field_0x2f8 = local_9;
  return local_9;
}


// ============================================================================
// Function: SafeBaseline_GetLatestBuf @ 6bb02d52
// ============================================================================

undefined8 SafeBaseline_GetLatestBuf(void)

{
  short sVar1;
  longlong lVar2;
  undefined8 uVar3;
  
                    /* 0xc2d52  1222  SafeBaseline_GetLatestBuf */
  sVar1 = BaselineQueue_Count();
  if (sVar1 == 0) {
    lVar2 = BaselineQueue_Pop();
    uVar3 = *(undefined8 *)(lVar2 + 0x20);
  }
  else {
    lVar2 = BaselineQueue_GetLastestItem();
    uVar3 = *(undefined8 *)(lVar2 + 0x20);
  }
  return uVar3;
}


// ============================================================================
// Function: SafeBaseline_GetSafeBL2RawDif @ 6bb02860
// ============================================================================

undefined8 SafeBaseline_GetSafeBL2RawDif(void)

{
                    /* 0xc2860  1225  SafeBaseline_GetSafeBL2RawDif */
  return g_tsaBufSafeBL2RawDif;
}


// ============================================================================
// Function: SafeBaseline_GetSafeBL2RawDifCMF @ 6bb0286d
// ============================================================================

undefined8 SafeBaseline_GetSafeBL2RawDifCMF(void)

{
                    /* 0xc286d  1226  SafeBaseline_GetSafeBL2RawDifCMF */
  return g_tsaBufSafeBL2RawDifCMF;
}


// ============================================================================
// Function: SafeBaseline_GetSafeBL2BLDif @ 6bb0287a
// ============================================================================

undefined8 SafeBaseline_GetSafeBL2BLDif(void)

{
                    /* 0xc287a  1223  SafeBaseline_GetSafeBL2BLDif */
  return g_tsaBufSafeBL2BLDif;
}


// ============================================================================
// Function: SafeBaseline_GetSafeBL2BLDifCMF @ 6bb02887
// ============================================================================

undefined8 SafeBaseline_GetSafeBL2BLDifCMF(void)

{
                    /* 0xc2887  1224  SafeBaseline_GetSafeBL2BLDifCMF */
  return g_tsaBufSafeBL2BLDifCMF;
}


// ============================================================================
// Function: SafeBaseline_ClrPrpts @ 6bb03263
// ============================================================================

void SafeBaseline_ClrPrpts(longlong param_1)

{
  SafeBaseline_ClrPrpt(param_1);
  SafeBaseline_ClrPrpt(param_1 + 0xc);
  SafeBaseline_ClrPrpt(param_1 + 0x18);
  SafeBaseline_ClrPrpt(param_1 + 0x24);
  SafeBaseline_ClrPrpt(param_1 + 0x30);
  return;
}


// ============================================================================
// Function: SafeBaseline_GetPrptSensor @ 6bb03576
// ============================================================================

void SafeBaseline_GetPrptSensor
               (byte param_1,byte param_2,short param_3,short param_4,undefined8 param_5)

{
  ushort uVar1;
  short sVar2;
  
  uVar1 = *(ushort *)
           ((ulonglong)
            ((uint)param_1 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
           *(longlong *)_refptr_g_tsaBufDif);
  sVar2 = (uVar1 ^ (short)uVar1 >> 0x1f) - ((short)uVar1 >> 0xf);
  if (param_3 < sVar2) {
    if (param_4 < sVar2) {
      SafeBaseline_GetPrptCommon(param_1,param_2,param_5,1);
    }
    else {
      SafeBaseline_GetPrptCommon(param_1,param_2,param_5,0);
    }
  }
  return;
}


// ============================================================================
// Function: SafeBaseline_ResetCheck @ 6bb0583b
// ============================================================================

bool SafeBaseline_ResetCheck(void)

{
  char cVar1;
  
  SafeBaseline_StableStateProcess();
  DAT_6bc8d180 = 0;
  DAT_6bc8d18c = 0;
  cVar1 = SafeBaseline_IsSignalStable();
  if (cVar1 != '\0') {
    DAT_6bc8d18c = SafeBaseline_JudgeCurrentBaseline();
    if (DAT_6bc8d18c != 1) {
      SafeBaseline_CheckWithState();
    }
    SafeBaseline_CheckWithSignal();
  }
  cVar1 = BLReset_IsAllTouchByAbnormalBaseline();
  if (cVar1 != '\0') {
    DAT_6bc8d180 = DAT_6bc8d180 | 8;
  }
  if ((*_refptr_g_prevTouchesPtr)->nTouchCount == 0x2000) {
    if ((*_refptr_g_tsaStaticPtr)[1].field_0x3 == '\0') {
      DAT_6bc8d180 = DAT_6bc8d180 | 4;
    }
  }
  else {
    (*_refptr_g_tsaStaticPtr)[1].field_0x3 = 0;
  }
  SafeBaseline_ResetStatistics();
  return DAT_6bc8d184 != 0;
}


// ============================================================================
// Function: SafeBaseline_IsSafeDiffChecked @ 6bb04565
// ============================================================================

bool SafeBaseline_IsSafeDiffChecked(void)

{
                    /* 0xc4565  1234  SafeBaseline_IsSafeDiffChecked */
  return (DAT_6bc8d050 & 2) != 0;
}


// ============================================================================
// Function: SafeBaseline_ClrPrpt @ 6bb03232
// ============================================================================

void SafeBaseline_ClrPrpt(undefined2 *param_1)

{
  param_1[4] = 0;
  param_1[5] = 0;
  param_1[1] = 0x7fff;
  *param_1 = 0x8000;
  return;
}


// ============================================================================
// Function: SafeBaseline_GetPrptCommon @ 6bb0338e
// ============================================================================

void SafeBaseline_GetPrptCommon(byte param_1,byte param_2,longlong param_3,undefined1 param_4)

{
  short sVar1;
  char cVar2;
  
  sVar1 = *(short *)((ulonglong)
                     ((uint)param_1 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)
                     * 2 + *(longlong *)_refptr_g_tsaBufDif);
  if (((((uint)param_1 == _refptr_g_tsaPrmtFlash->bCols - 1) || (param_1 == 0)) ||
      ((uint)param_2 == _refptr_g_tsaPrmtFlash->bRows - 1)) || (param_2 == 0)) {
    if (param_1 == 0) {
      if (((uint)param_2 != _refptr_g_tsaPrmtFlash->bRows - 1) && (param_2 != 0)) {
        SafeBaseline_SetPrpt(param_3 + 0x24,(int)sVar1,param_4);
        SafeBaseline_SetPrpt(param_3 + 0x18,(int)sVar1,param_4);
      }
    }
    else if ((uint)param_1 == _refptr_g_tsaPrmtFlash->bCols - 1) {
      if (((uint)param_2 != _refptr_g_tsaPrmtFlash->bRows - 1) && (param_2 != 0)) {
        SafeBaseline_SetPrpt(param_3 + 0x30,(int)sVar1,param_4);
        SafeBaseline_SetPrpt(param_3 + 0x18,(int)sVar1,param_4);
      }
    }
    else {
      SafeBaseline_SetPrpt(param_3 + 0x18,(int)sVar1,param_4);
    }
  }
  else {
    SafeBaseline_SetPrpt(param_3 + 0xc,(int)sVar1,param_4);
    SafeBaseline_SetPrpt(param_3 + 0x18,(int)sVar1,param_4);
    cVar2 = SafeBaseline_IsInCenter(param_1,param_2);
    if (cVar2 != '\0') {
      SafeBaseline_SetPrpt(param_3,(int)sVar1,param_4);
    }
  }
  return;
}


// ============================================================================
// Function: SafeBaseline_IsToSyncResetForSideArea @ 6bb04f88
// ============================================================================

undefined8 SafeBaseline_IsToSyncResetForSideArea(void)

{
  undefined8 uVar1;
  
                    /* 0xc4f88  1238  SafeBaseline_IsToSyncResetForSideArea */
  if ((_refptr_g_tsaPrpt->dwBaselinePropertyState == 0) ||
     (_refptr_g_tsaPrpt->dwBaselinePropertyState == 0x14)) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: SafeBaseline_StableStateProcess @ 6bb02c3d
// ============================================================================

void SafeBaseline_StableStateProcess(void)

{
  char cVar1;
  
                    /* 0xc2c3d  1247  SafeBaseline_StableStateProcess */
  cVar1 = BLReset_CheckNumOverNoise();
  if (cVar1 == '\0') {
    DAT_6bc8d062 = 0;
  }
  else if (DAT_6bc8d062 < 5) {
    DAT_6bc8d062 = DAT_6bc8d062 + 1;
  }
  return;
}


// ============================================================================
// Function: SafeBaseline_JudgeCurrentBaseline @ 6bb041a2
// ============================================================================

undefined4 SafeBaseline_JudgeCurrentBaseline(void)

{
  char cVar1;
  short sVar2;
  longlong lVar3;
  undefined8 uVar4;
  
                    /* 0xc41a2  1239  SafeBaseline_JudgeCurrentBaseline */
  sVar2 = BaselineQueue_Count();
  if (sVar2 != 0) {
    lVar3 = BaselineQueue_GetLastestItem();
    uVar4 = TSAFold_GridZoomInRemap(0,*(undefined8 *)(lVar3 + 0x20),2,0);
    SafeBaseline_BufferComparison
              (*(undefined8 *)_refptr_g_tsaBufBl,uVar4,g_tsaBufSafeBL2BLDif,g_tsaBufSafeBL2BLDifCMF,
               (int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x46),
               (int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x42),&DAT_6bc8d0dc,0);
    SafeBaseline_BufferComparison
              (*(undefined8 *)_refptr_g_tsaBufRaw,uVar4,g_tsaBufSafeBL2RawDif,
               g_tsaBufSafeBL2RawDifCMF,(int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x46),
               (int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x42),&DAT_6bc8d064,0);
    if ((DAT_6bc8d0c0 == 0) &&
       ((cVar1 = SafeBaseline_IsAnyBaselineBeforeCurrentScreenOn(), cVar1 != '\0' ||
        ((DAT_6bc8d050 & 2) != 0)))) {
      DAT_6bc8d050 = DAT_6bc8d050 | 1;
    }
    if (DAT_6bc8d0c0 == 0) {
      return 0xffffffff;
    }
    if (DAT_6bc8d138 == 0) {
      return 1;
    }
    cVar1 = SafeBaseline_CheckAbnormalMax(0);
    if (((cVar1 != '\0') || (cVar1 = SafeBaseline_CheckAbnormalMin(0), cVar1 != '\0')) &&
       (((uint)DAT_6bc8d0c0 * 2 + (uint)DAT_6bc8d0c0 <= (uint)DAT_6bc8d138 ||
        ((uint)DAT_6bc8d0c2 * 2 + (uint)DAT_6bc8d0c2 <= (uint)DAT_6bc8d13a)))) {
      return 0xffffffff;
    }
  }
  return 0;
}


// ============================================================================
// Function: SafeBaseline_ResetStatistics @ 6bb0524e
// ============================================================================

void SafeBaseline_ResetStatistics(void)

{
  char cVar1;
  byte local_9;
  
  DAT_6bc8d184 = 0;
  DAT_6bc8d190 = 0;
  DAT_6bc8d188 = 0;
  for (local_9 = 0; local_9 < 10; local_9 = local_9 + 1) {
    cVar1 = SafeBaseline_ResetIsAllowed(local_9);
    if (cVar1 != '\0') {
      DAT_6bc8d188 = DAT_6bc8d188 | 1 << (local_9 & 0x1f);
      SafeBaseline_ResetStatisticsUnit(local_9);
    }
  }
  return;
}


// ============================================================================
// Function: SafeBaseline_CheckWithState @ 6bb053eb
// ============================================================================

void SafeBaseline_CheckWithState(void)

{
  char cVar1;
  
  if (((DAT_6bc8d18c == -1) &&
      ((*_refptr_g_prevTouchesPtr)->field_0xf == (*_refptr_g_prevTouchesPtr)->field_0xe)) &&
     ((*_refptr_g_prevTouchesPtr)->field_0xf != '\0')) {
    DAT_6bc8d180 = DAT_6bc8d180 | 1;
  }
  cVar1 = SafeBaseline_IsSafeDiffChecked();
  if ((cVar1 == '\0') &&
     (((*_refptr_g_prevTouchesPtr)->field_0x10 != '\0' ||
      ((*_refptr_g_prevTouchesPtr)->field_0x13 != '\0')))) {
    DAT_6bc8d180 = DAT_6bc8d180 | 0x40;
  }
  if (((_refptr_g_tsaPrmtFlash->wBufElementCount >> 2 < *(ushort *)&_refptr_g_tsaPrpt->field_0x1e4)
      && (((*_refptr_g_prevTouchesPtr)->field_0xe != '\0' ||
          ((*_refptr_g_prevTouchesPtr)->field_0xf == '\0')))) &&
     (6 < (byte)_refptr_g_peaksBeforeSwitchDifBuffer[1])) {
    DAT_6bc8d180 = DAT_6bc8d180 | 0x10;
  }
  cVar1 = SafeBaseline_IsTouchToBeProtected();
  if ((cVar1 != '\0') && (DAT_6bc8d18c != -1)) {
    return;
  }
  cVar1 = SafeBaseline_IsSafeRawChecked();
  if ((cVar1 == '\0') ||
     (cVar1 = SafeBaseline_CheckAbnormalMax((int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x46)),
     cVar1 == '\0')) {
    cVar1 = SafeBaseline_CheckGhostMax(0);
    if (cVar1 != '\0') {
      DAT_6bc8d180 = DAT_6bc8d180 | 2;
    }
  }
  else {
    DAT_6bc8d180 = DAT_6bc8d180 | 2;
  }
  cVar1 = SafeBaseline_IsVeryNoisy();
  if (cVar1 != '\0') {
    DAT_6bc8d180 = DAT_6bc8d180 | 0x10;
  }
  cVar1 = SafeBaseline_IsVeryNegative();
  if (cVar1 != '\0') {
    DAT_6bc8d180 = DAT_6bc8d180 | 0x20;
  }
  if ((((*_refptr_g_prevTouchesPtr)->field_0xe != '\0') ||
      ((*_refptr_g_prevTouchesPtr)->field_0xf == '\0')) &&
     (4 < (byte)_refptr_g_peaksBeforeSwitchDifBuffer[1])) {
    DAT_6bc8d180 = DAT_6bc8d180 | 0x10;
  }
  return;
}


// ============================================================================
// Function: SafeBaseline_CheckWithSignal @ 6bb05615
// ============================================================================

void SafeBaseline_CheckWithSignal(void)

{
  short sVar1;
  char cVar2;
  
  sVar1 = *(short *)&_refptr_g_tsaPrpt->field_0x1cc;
  if ((((int)sVar1 < ((short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax * 3) / 2) &&
      (_refptr_g_tsaPrmtFlash->wBufElementCount >> 3 < *(ushort *)&_refptr_g_tsaPrpt->field_0x1e4))
     && ((*_refptr_g_prevTouchesPtr)->field_0xf == (*_refptr_g_prevTouchesPtr)->field_0x10)) {
    DAT_6bc8d180 = DAT_6bc8d180 | 0x80;
  }
  cVar2 = SafeBaseline_CheckGhostMaxInSilent();
  if (cVar2 != '\0') {
    DAT_6bc8d180 = DAT_6bc8d180 | 0x200;
  }
  if ((_refptr_g_tsaPrmtFlash->wBufElementCount >> 2 < *(ushort *)&_refptr_g_tsaPrpt->field_0x1e4)
     && ((sVar1 < (short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 2 ||
         (cVar2 = SafeBaseline_CheckAbnormalMax(0), cVar2 != '\0')))) {
    DAT_6bc8d180 = DAT_6bc8d180 | 0x80;
  }
  if ((_refptr_g_tsaPrmtFlash->wBufElementCount / 3 < *(ushort *)&_refptr_g_tsaPrpt->field_0x1e6) &&
     (((sVar1 < *(short *)(_refptr_g_tsaPrmtDynamic + 0x42) ||
       ((*_refptr_g_prevTouchesPtr)->field_0x3 != '\0')) ||
      (((*_refptr_g_prevTouchesPtr)->field_0x6 == '\0' ||
       ((*_refptr_g_prevTouchesPtr)->field_0x11 != '\0')))))) {
    DAT_6bc8d180 = DAT_6bc8d180 | 0x80;
  }
  if ((sVar1 < *(short *)(_refptr_g_tsaPrmtDynamic + 0x42)) &&
     ((*_refptr_g_tsaStaticPtr)[1].bField_0x00 == '\0')) {
    cVar2 = SafeBaseline_CheckAbnormalMax((int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x46));
    if ((cVar2 == '\0') &&
       (cVar2 = SafeBaseline_CheckAbnormalMin((int)-*(short *)(_refptr_g_tsaPrmtDynamic + 0x46)),
       cVar2 == '\0')) {
      return;
    }
    DAT_6bc8d180 = DAT_6bc8d180 | 0x100;
  }
  return;
}


// ============================================================================
// Function: SafeBaseline_SetPrpt @ 6bb032c2
// ============================================================================

void SafeBaseline_SetPrpt(short *param_1,short param_2,char param_3)

{
  if (param_3 != '\0') {
    param_1[5] = param_1[5] + 1;
  }
  param_1[4] = param_1[4] + 1;
  if (param_2 < param_1[1]) {
    param_1[1] = param_2;
  }
  if (*param_1 < param_2) {
    *param_1 = param_2;
  }
  return;
}


// ============================================================================
// Function: SafeBaseline_IsInCenter @ 6bb03336
// ============================================================================

undefined8 SafeBaseline_IsInCenter(byte param_1,byte param_2)

{
  undefined8 uVar1;
  
  if ((((param_1 < 4) || ((int)(_refptr_g_tsaPrmtFlash->bCols - 4) <= (int)(uint)param_1)) ||
      (param_2 < 4)) || ((int)(_refptr_g_tsaPrmtFlash->bRows - 4) <= (int)(uint)param_2)) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: SafeBaseline_CheckAbnormalMax @ 6bb03e0e
// ============================================================================

bool SafeBaseline_CheckAbnormalMax(short param_1)

{
  char cVar1;
  bool local_9;
  
                    /* 0xc3e0e  1212  SafeBaseline_CheckAbnormalMax */
  local_9 = false;
  if (param_1 <= *(short *)&_refptr_g_tsaPrpt->field_0x1cc) {
    cVar1 = SafeBaseline_CheckAbnormalSensor
                      (_refptr_g_tsaPrpt->field_0x1d0,_refptr_g_tsaPrpt->field_0x1d1);
    local_9 = cVar1 != '\0';
    cVar1 = SafeBaseline_IsOKToCheckBefCMFDifMax();
    if ((cVar1 != '\0') &&
       (cVar1 = SafeBaseline_CheckAbnormalSensor
                          (_refptr_g_tsaPrpt->field_0x168,_refptr_g_tsaPrpt->field_0x169),
       cVar1 != '\0')) {
      local_9 = true;
    }
  }
  return local_9;
}


// ============================================================================
// Function: SafeBaseline_CheckAbnormalMin @ 6bb03ea9
// ============================================================================

undefined1 SafeBaseline_CheckAbnormalMin(short param_1)

{
  char cVar1;
  undefined1 local_9;
  
                    /* 0xc3ea9  1214  SafeBaseline_CheckAbnormalMin */
  local_9 = 0;
  if (*(short *)&_refptr_g_tsaPrpt->field_0x1ce <= param_1) {
    cVar1 = SafeBaseline_CheckAbnormalSensor
                      (_refptr_g_tsaPrpt->field_0x1d2,_refptr_g_tsaPrpt->field_0x1d3);
    if (cVar1 != '\0') {
      local_9 = 1;
    }
  }
  return local_9;
}


// ============================================================================
// Function: SafeBaseline_IsAnyBaselineBeforeCurrentScreenOn @ 6bb0413e
// ============================================================================

undefined8 SafeBaseline_IsAnyBaselineBeforeCurrentScreenOn(void)

{
  ushort uVar1;
  longlong lVar2;
  byte local_19;
  
  local_19 = 0;
  while( true ) {
    uVar1 = BaselineQueue_Count();
    if (uVar1 <= local_19) {
      return 0;
    }
    lVar2 = BaselineQueue_GetIndexedItem(local_19);
    if (*(ulonglong *)(lVar2 + 8) < *(ulonglong *)&(*_refptr_g_tsaStaticPtr)->field_0x80) break;
    local_19 = local_19 + 1;
  }
  return 1;
}


// ============================================================================
// Function: SafeBaseline_ResetStatisticsUnit @ 6bb04fbd
// ============================================================================

void SafeBaseline_ResetStatisticsUnit(byte param_1)

{
  char cVar1;
  short local_c;
  ushort local_a;
  
  if ((1 << (param_1 & 0x1f) & DAT_6bc8d180) == 0) {
    local_c = (*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
    if (local_c < 0) {
      local_c = 0;
    }
    if ((int)local_c < (int)(uint)*(ushort *)(&DAT_6bc8d16a + (longlong)(int)(uint)param_1 * 2)) {
      if (DAT_6bc8d180 == 0) {
        *(short *)(&DAT_6bc8d16a + (longlong)(int)(uint)param_1 * 2) =
             *(short *)(&DAT_6bc8d16a + (longlong)(int)(uint)param_1 * 2) - local_c;
      }
    }
    else {
      *(undefined2 *)(&DAT_6bc8d16a + (longlong)(int)(uint)param_1 * 2) = 0;
    }
  }
  else {
    DAT_6bc8d190 = 1;
    *(short *)(&DAT_6bc8d16a + (longlong)(int)(uint)param_1 * 2) =
         *(short *)(&DAT_6bc8d16a + (longlong)(int)(uint)param_1 * 2) +
         (*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
    local_a = *(ushort *)(&g_timeAbnormalThold + (longlong)(int)(uint)param_1 * 2);
    cVar1 = SafeBaseline_IsSafeDiffChecked();
    if (cVar1 == '\0') {
      local_a = 0xfa;
    }
    if (local_a <= *(ushort *)(&DAT_6bc8d16a + (longlong)(int)(uint)param_1 * 2)) {
      *(undefined2 *)(&DAT_6bc8d16a + (longlong)(int)(uint)param_1 * 2) = 0;
      (&g_safeBaselineReset)[(int)(uint)param_1] =
           (&g_safeBaselineReset)[(int)(uint)param_1] + '\x01';
      DAT_6bc8d184 = DAT_6bc8d184 | 1 << (param_1 & 0x1f);
    }
  }
  return;
}


// ============================================================================
// Function: SafeBaseline_ResetIsAllowed @ 6bb05179
// ============================================================================

undefined8 SafeBaseline_ResetIsAllowed(byte param_1)

{
  char cVar1;
  undefined8 uVar2;
  
  cVar1 = SafeBaseline_IsSafeRawChecked();
  if (cVar1 == '\0') {
    if (((*_refptr_g_tsaStaticPtr)->pFrameBufferPtr <
         (void *)((ulonglong)*(uint *)(&g_timeScreenOnThold + (longlong)(int)(uint)param_1 * 4) +
                 *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x80)) &&
       ((((&g_pushAbnormalThold)[(int)(uint)param_1] == '\0' ||
         ((byte)_refptr_g_safeBaseline[0x21] < (byte)(&g_pushAbnormalThold)[(int)(uint)param_1])) &&
        ((byte)(&g_safeBaselineReset)[(int)(uint)param_1] <
         (byte)(&g_triggerAbnormalThold)[(int)(uint)param_1])))) {
      uVar2 = 1;
    }
    else {
      uVar2 = 0;
    }
  }
  else {
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: SafeBaseline_CheckGhostMax @ 6bb0409c
// ============================================================================

bool SafeBaseline_CheckGhostMax(short param_1)

{
  char cVar1;
  char cVar2;
  bool local_9;
  
                    /* 0xc409c  1216  SafeBaseline_CheckGhostMax */
  cVar1 = SafeBaseline_CheckAbnormalMax((int)param_1);
  cVar2 = SafeBaseline_CheckGhostSensorByMinSigOff
                    (_refptr_g_tsaPrpt->field_0x1d0,_refptr_g_tsaPrpt->field_0x1d1);
  local_9 = cVar2 != '\0' && cVar1 != '\0';
  return local_9;
}


// ============================================================================
// Function: SafeBaseline_IsVeryNegative @ 6bb052db
// ============================================================================

undefined8 SafeBaseline_IsVeryNegative(void)

{
  if ((int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x42) <
      -(int)*(short *)&_refptr_g_tsaPrpt->field_0x1ce) {
    if (*(short *)&_refptr_g_tsaPrpt->field_0x1cc * 2 <
        -(int)*(short *)&_refptr_g_tsaPrpt->field_0x1ce) {
      return 1;
    }
    if (*(int *)&_refptr_g_tsaPrpt->field_0x1d8 * 2 < -*(int *)&_refptr_g_tsaPrpt->field_0x1dc) {
      return 1;
    }
  }
  return 0;
}


// ============================================================================
// Function: SafeBaseline_IsVeryNoisy @ 6bb05363
// ============================================================================

undefined8 SafeBaseline_IsVeryNoisy(void)

{
  undefined8 uVar1;
  
  if ((*(ushort *)&_refptr_g_tsaPrpt->field_0x1e4 < 0x33) ||
     (*(short *)(_refptr_g_tsaPrmtDynamic + 0x42) <= *(short *)&_refptr_g_tsaPrpt->field_0x1cc)) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: SafeBaseline_IsTouchToBeProtected @ 6bb053a7
// ============================================================================

bool SafeBaseline_IsTouchToBeProtected(void)

{
  return 0 < (int)((uint)(byte)(*_refptr_g_prevTouchesPtr)->field_0xf -
                  (uint)(byte)(*_refptr_g_prevTouchesPtr)->field_0xe);
}


// ============================================================================
// Function: SafeBaseline_CheckGhostMaxInSilent @ 6bb0403a
// ============================================================================

undefined8 SafeBaseline_CheckGhostMaxInSilent(void)

{
  undefined1 uVar1;
  undefined1 uVar2;
  char cVar3;
  
                    /* 0xc403a  1217  SafeBaseline_CheckGhostMaxInSilent */
  uVar1 = _refptr_g_tsaPrpt->field_0x1d0;
  uVar2 = _refptr_g_tsaPrpt->field_0x1d1;
  cVar3 = SafeBaseline_CheckGhostSensorByMinSigOff(uVar1,uVar2);
  if ((cVar3 != '\0') &&
     (cVar3 = SafeBaseline_CheckGhostSensorBySilentTouchNum(uVar1,uVar2), cVar3 != '\0')) {
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: SafeBaseline_JudgeCurrentSideBaseline @ 6bb04ce9
// ============================================================================

undefined4 SafeBaseline_JudgeCurrentSideBaseline(int param_1)

{
  short sVar1;
  undefined2 *local_18;
  undefined *local_10;
  
                    /* 0xc4ce9  1240  SafeBaseline_JudgeCurrentSideBaseline */
  sVar1 = BaselineQueue_Count();
  if (sVar1 != 0) {
    if (param_1 == 4) {
      local_10 = &DAT_6bc8d0c4;
      local_18 = &DAT_6bc8d13c;
    }
    else {
      local_10 = &DAT_6bc8d0d0;
      local_18 = &DAT_6bc8d148;
    }
    if (*(short *)(local_10 + 8) == 0) {
      return 0xffffffff;
    }
    if (local_18[4] == 0) {
      return 1;
    }
    if (((uint)*(ushort *)(local_10 + 8) * 2 + (uint)*(ushort *)(local_10 + 8) <=
         (uint)(ushort)local_18[4]) ||
       ((uint)*(ushort *)(local_10 + 10) * 2 + (uint)*(ushort *)(local_10 + 10) <=
        (uint)(ushort)local_18[5])) {
      return 0xffffffff;
    }
  }
  return 0;
}


// ============================================================================
// Function: SafeBaseline_IsOKToCheckBefCMFDifMax @ 6bb03d5d
// ============================================================================

undefined8 SafeBaseline_IsOKToCheckBefCMFDifMax(void)

{
  undefined8 uVar1;
  
  if ((((*(short *)_refptr_g_tsaPrpt < (short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax / 2) &&
       (-(int)(short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax / 2 <
        (int)*(short *)&_refptr_g_tsaPrpt->field_0x2)) &&
      (*(ushort *)&_refptr_g_tsaPrpt->field_0x4 < _refptr_g_tsaPrmtFlash->wBufElementCount >> 2)) &&
     ((short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax / 2 <
      *(short *)&_refptr_g_tsaPrpt->field_0x1cc)) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: SafeBaseline_CheckGhostSensorByMinSigOff @ 6bb03f0a
// ============================================================================

undefined8 SafeBaseline_CheckGhostSensorByMinSigOff(byte param_1,byte param_2)

{
  undefined8 uVar1;
  
  if (*(short *)_refptr_g_tsaPrmtDynamic / 3 <
      *(short *)((ulonglong)
                 ((uint)param_1 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2
                + g_tsaBufMaxSigDown)) {
    uVar1 = 0;
  }
  else if ((int)*(short *)((ulonglong)
                           ((uint)param_1 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols &
                           0xffff) * 2 + g_tsaBufMaxSigDown) <
           *(short *)((ulonglong)
                      ((uint)param_1 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)
                      * 2 + g_tsaBufMinSigOff) * 2) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: SafeBaseline_CheckGhostSensorBySilentTouchNum @ 6bb03fee
// ============================================================================

bool SafeBaseline_CheckGhostSensorBySilentTouchNum(byte param_1,byte param_2)

{
  return 2 < *(byte *)((ulonglong)
                       ((uint)param_1 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff
                       ) + g_tsaBufSilentTouchNum);
}


