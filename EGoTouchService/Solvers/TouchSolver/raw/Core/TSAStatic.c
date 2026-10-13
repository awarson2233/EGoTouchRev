// ============================================================================
// Function: TSAStatic_PreProcess @ 6baa1bb0
// ============================================================================

void TSAStatic_PreProcess(void)

{
  TSAStatic *pTVar1;
  undefined1 uVar2;
  char cVar3;
  short sVar4;
  
                    /* 0x61bb0  1709  TSAStatic_PreProcess */
  g_tsaStaticPtr->nField_0x28c = 0;
  g_tsaStaticPtr->field_0x1 = g_tsaStaticPtr->bField_0x00;
  g_tsaStaticPtr->field_0xe = g_tsaStaticPtr->field_0xd;
  g_tsaStaticPtr->field_0x4 = g_tsaStaticPtr->field_0x3;
  g_tsaStaticPtr->field_0x8 = g_tsaStaticPtr->field_0x7;
  g_tsaStaticPtr->field_0x6 = g_tsaStaticPtr->field_0x5;
  g_tsaStaticPtr->field_0xb = g_tsaStaticPtr->field_0xa;
  pTVar1 = g_tsaStaticPtr;
  uVar2 = AFE_IsInIDLEMode();
  pTVar1->field_0x3 = uVar2;
  pTVar1 = g_tsaStaticPtr;
  uVar2 = AFE_IsInSideIDLEMode();
  pTVar1->field_0x5 = uVar2;
  g_tsaStaticPtr->field_0x10 = g_tsaStaticPtr->field_0xf;
  g_tsaStaticPtr->dwSpecialControlFlags = 0;
  *(undefined4 *)&g_tsaStaticPtr->field_0x9c = 0;
  g_tsaStaticPtr->bField_0x11a = g_tsaStaticPtr[1].field_0x3;
  cVar3 = AFE_IsInProximityScreenOffState();
  if (cVar3 == '\0') {
    *(undefined8 *)&g_tsaStaticPtr->field_0x40 = 0xffffffffffffffff;
  }
  else if (*(longlong *)&g_tsaStaticPtr->field_0x40 == -1) {
    *(void **)&g_tsaStaticPtr->field_0x40 = g_tsaStaticPtr->pFrameBufferPtr;
  }
  cVar3 = TSAStatic_IsProximityEnabled();
  if (cVar3 == '\0') {
    if (g_tsaStaticPtr->llField_0x50 != -1) {
      g_tsaStaticPtr->llField_0x58 = (longlong)g_tsaStaticPtr->pFrameBufferPtr;
    }
    g_tsaStaticPtr->llField_0x50 = -1;
    g_tsaStaticPtr->llField_0x48 = -1;
  }
  else {
    if (g_tsaStaticPtr->llField_0x50 == -1) {
      g_tsaStaticPtr->llField_0x50 = (longlong)g_tsaStaticPtr->pFrameBufferPtr;
    }
    g_tsaStaticPtr->llField_0x58 = -1;
  }
  pTVar1 = g_tsaStaticPtr;
  sVar4 = Touch_GetFormattedTimeInterval
                    ((int)(short)((short)g_tsaStaticPtr->pFrameBufferPtr -
                                 (short)g_tsaStaticPtr->llField_0xe8));
  pTVar1->nBaselineStageInitialSignal = sVar4;
  *(undefined4 *)&g_tsaStaticPtr->field_0xd8 =
       *(undefined4 *)(*(longlong *)(_refptr_g_thpAFEStruct + 0x10) + 0x34);
  return;
}


// ============================================================================
// Function: TSAStatic_PostProcess @ 6baa1f51
// ============================================================================

void TSAStatic_PostProcess(void)

{
  TSAStatic *pTVar1;
  undefined1 uVar2;
  
                    /* 0x61f51  1708  TSAStatic_PostProcess */
  pTVar1 = g_tsaStaticPtr;
  uVar2 = TSAStatic_IsAllFreqNoisy();
  pTVar1->field_0xd = uVar2;
  pTVar1 = g_tsaStaticPtr;
  uVar2 = TSA_IsOKToEnterIDLEMode();
  pTVar1->field_0x2 = uVar2;
  pTVar1 = g_tsaStaticPtr;
  uVar2 = TSA_UpdateIDLEModeInternally();
  pTVar1->field_0x7 = uVar2;
  pTVar1 = g_tsaStaticPtr;
  uVar2 = TSA_UpdateSideIDLEModeInternally();
  pTVar1->field_0xa = uVar2;
  pTVar1 = g_tsaStaticPtr;
  uVar2 = TSAStatic_UpdateIDLEModeOutput();
  pTVar1->field_0x9 = uVar2;
  pTVar1 = g_tsaStaticPtr;
  uVar2 = TSAStatic_UpdateSideIDLEModeOutput();
  pTVar1->field_0xc = uVar2;
  g_tsaStaticPtr->field_0xf = g_tsaStaticPtr->field_0x2f9;
  if ((g_tsaStaticPtr->bField_0x118 == '\0') || (g_tsaStaticPtr->field_0x2f9 == '\0')) {
    g_tsaStaticPtr->bField_0x119 = '\0';
  }
  else {
    g_tsaStaticPtr->bField_0x119 = '\x01';
    g_tsaStaticPtr->bField_0x118 = '\0';
  }
  g_tsaStaticPtr->field_0x2f9 = 0;
  g_tsaStaticPtr[1].field_0x1 = 0;
  g_tsaStaticPtr->nField_0xc0 = g_tsaStaticPtr->dwSystemFlags;
  g_tsaStaticPtr->llField_0xe8 = (longlong)g_tsaStaticPtr->pFrameBufferPtr;
  g_tsaStaticPtr->llField_0xd0 = *(longlong *)&g_tsaStaticPtr->field_0xc8;
  g_tsaStaticPtr->dwFeatureFlagsPrev = g_tsaStaticPtr->dwFeatureFlags;
  g_tsaStaticPtr->nField_0xdc = *(int *)&g_tsaStaticPtr->field_0xd8;
  g_tsaStaticPtr->nField_0x278 = *(int *)&g_tsaStaticPtr->field_0x27c;
  g_tsaStaticPtr->dwField_0x158 = g_tsaStaticPtr->dwStylusAftGateFlags;
  return;
}


// ============================================================================
// Function: TSAStatic_UpdateCtrlFlags @ 6baa21cb
// ============================================================================

void TSAStatic_UpdateCtrlFlags(TSA_SystemFlags param_1)

{
  TSA_SystemFlags local_res8;
  TSA_SystemFlags local_c;
  
                    /* 0x621cb  1715  TSAStatic_UpdateCtrlFlags */
  if ((param_1 & 8) != 0) {
    g_tsaStaticPtr->llAutoCalibrationStartTime = (longlong)g_tsaStaticPtr->pFrameBufferPtr;
  }
  local_res8 = param_1;
  if ((param_1 & 4) != 0) {
    local_res8 = param_1 & 0xfffffff7;
    *(void **)&g_tsaStaticPtr->field_0x70 = g_tsaStaticPtr->pFrameBufferPtr;
  }
  if ((local_res8 & 0xd) != 0) {
    g_tsaStaticPtr->field_0xa1 = 3;
    g_tsaStaticPtr->bBaselineResetPropertyLatched = '\0';
  }
  if ((g_tsaStaticPtr->field_0xa1 == '\0') &&
     (g_tsaStaticPtr->bBaselineResetPropertyLatched != '\0')) {
    local_c = local_res8;
  }
  else {
    if (g_tsaStaticPtr->field_0xa1 != '\0') {
      g_tsaStaticPtr->field_0xa1 = g_tsaStaticPtr->field_0xa1 + -1;
    }
    local_c = local_res8 & 0xfffffff2 | g_tsaStaticPtr->dwSystemFlags & 0xd | local_res8 & 0xd;
  }
  if (((g_tsaStaticPtr->dwSystemFlags & 0xd) == 0) || ((local_c & 0xd) != 0)) {
    g_tsaStaticPtr->field_0x117 = 0;
  }
  else {
    g_tsaStaticPtr->field_0x117 = 1;
    g_tsaStaticPtr->bField_0x118 = '\x01';
  }
  g_tsaStaticPtr->dwSystemFlags = local_c;
  TSAStatic_PhoneCallModeDebounce();
  return;
}


// ============================================================================
// Function: TSAStatic_IsHWReset @ 6baa2345
// ============================================================================

bool TSAStatic_IsHWReset(void)

{
                    /* 0x62345  1695  TSAStatic_IsHWReset */
  return (g_tsaStaticPtr->dwSystemFlags & 0xf) != 0;
}


// ============================================================================
// Function: TSAStatic_GetMSTimeStamp @ 6baa2d60
// ============================================================================

void * TSAStatic_GetMSTimeStamp(void)

{
                    /* 0x62d60  1677  TSAStatic_GetMSTimeStamp */
  return g_tsaStaticPtr->pFrameBufferPtr;
}


// ============================================================================
// Function: TSAStatic_IsHWResetPrev @ 6baa2373
// ============================================================================

bool TSAStatic_IsHWResetPrev(void)

{
                    /* 0x62373  1696  TSAStatic_IsHWResetPrev */
  return (g_tsaStaticPtr->nField_0xc0 & 0xfU) != 0;
}


// ============================================================================
// Function: TSAStatic_IsProximityEnabled @ 6baa2c95
// ============================================================================

undefined8 TSAStatic_IsProximityEnabled(void)

{
  undefined8 uVar1;
  
                    /* 0x62c95  1704  TSAStatic_IsProximityEnabled */
  if (((*(uint *)&_refptr_g_tsaPrmtFlash->field_0x64 & g_tsaStaticPtr->dwAftRuntimeModeMask) == 0)
     || ((g_tsaStaticPtr->dwFeatureFlags & 0x4000) == 0)) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: TSAStatic_IsAllFreqNoisy @ 6baa2590
// ============================================================================

undefined1 TSAStatic_IsAllFreqNoisy(void)

{
  char cVar1;
  
                    /* 0x62590  1690  TSAStatic_IsAllFreqNoisy */
  cVar1 = TSAStatic_IsChargerPlugged();
  if (cVar1 == '\0') {
    cVar1 = TSAStatic_IsLargeDisplayNoise();
    if (cVar1 == '\0') {
      g_tsaStaticPtr->field_0xd = 0;
    }
    else {
      g_tsaStaticPtr->field_0xd = 1;
    }
  }
  else {
    if ((g_tsaStaticPtr->dwSystemFlags & 0x20000) != 0) {
      g_tsaStaticPtr->field_0xd = 1;
    }
    cVar1 = AFE_IsAllFreqNoisy();
    if (cVar1 != '\0') {
      g_tsaStaticPtr->field_0xd = 1;
    }
  }
  return g_tsaStaticPtr->field_0xd;
}


// ============================================================================
// Function: TSAStatic_UpdateIDLEModeOutput @ 6baa2aa7
// ============================================================================

undefined1 TSAStatic_UpdateIDLEModeOutput(void)

{
  char cVar1;
  undefined1 local_9;
  
                    /* 0x62aa7  1717  TSAStatic_UpdateIDLEModeOutput */
  local_9 = 0;
  if (g_tsaStaticPtr->field_0x7 == '\0') {
    *(undefined8 *)&g_tsaStaticPtr->field_0x28 = 0xffffffffffffffff;
  }
  else {
    cVar1 = TSAStatic_IsJustOKToEnterIDLEMode();
    if (cVar1 == '\0') {
      if ((*(longlong *)&g_tsaStaticPtr->field_0x28 != -1) &&
         ((void *)((ulonglong)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x3a +
                  *(longlong *)&g_tsaStaticPtr->field_0x28) < g_tsaStaticPtr->pFrameBufferPtr)) {
        local_9 = 1;
      }
    }
    else {
      *(void **)&g_tsaStaticPtr->field_0x28 = g_tsaStaticPtr->pFrameBufferPtr;
      local_9 = 0;
    }
  }
  return local_9;
}


// ============================================================================
// Function: TSAStatic_UpdateSideIDLEModeOutput @ 6baa2b5a
// ============================================================================

undefined1 TSAStatic_UpdateSideIDLEModeOutput(void)

{
  char cVar1;
  undefined1 local_9;
  
                    /* 0x62b5a  1718  TSAStatic_UpdateSideIDLEModeOutput */
  local_9 = 0;
  if (g_tsaStaticPtr->field_0xa == '\0') {
    *(undefined8 *)&g_tsaStaticPtr->field_0x30 = 0xffffffffffffffff;
    *(uint *)&g_tsaStaticPtr->field_0x9c = *(uint *)&g_tsaStaticPtr->field_0x9c | 2;
  }
  else {
    cVar1 = TSAStatic_IsJustOKToEnterSideIDLEMode();
    if (cVar1 == '\0') {
      if (*(longlong *)&g_tsaStaticPtr->field_0x30 == -1) {
        *(uint *)&g_tsaStaticPtr->field_0x9c = *(uint *)&g_tsaStaticPtr->field_0x9c | 0x20000;
      }
      else if ((void *)((ulonglong)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x3a +
                       *(longlong *)&g_tsaStaticPtr->field_0x30) < g_tsaStaticPtr->pFrameBufferPtr)
      {
        local_9 = 1;
      }
      else {
        *(uint *)&g_tsaStaticPtr->field_0x9c = *(uint *)&g_tsaStaticPtr->field_0x9c | 8;
      }
    }
    else {
      *(void **)&g_tsaStaticPtr->field_0x30 = g_tsaStaticPtr->pFrameBufferPtr;
      local_9 = 0;
      *(uint *)&g_tsaStaticPtr->field_0x9c = *(uint *)&g_tsaStaticPtr->field_0x9c | 4;
    }
  }
  return local_9;
}


// ============================================================================
// Function: TSAStatic_PhoneCallModeDebounce @ 6baa20fe
// ============================================================================

void TSAStatic_PhoneCallModeDebounce(void)

{
  bool bVar1;
  char cVar2;
  
  bVar1 = false;
  if ((g_tsaStaticPtr->dwSystemFlags & 0x400000) == 0) {
    if (((*(longlong *)&g_tsaStaticPtr->field_0x68 != -1) &&
        (*(void **)&g_tsaStaticPtr->field_0x68 <= g_tsaStaticPtr->pFrameBufferPtr)) &&
       (g_tsaStaticPtr->pFrameBufferPtr < (void *)(*(longlong *)&g_tsaStaticPtr->field_0x68 + 0x4b0)
       )) {
      cVar2 = TSAStatic_IsProximityEnabled();
      if (cVar2 != '\0') {
        bVar1 = true;
      }
    }
  }
  else {
    bVar1 = true;
    *(void **)&g_tsaStaticPtr->field_0x68 = g_tsaStaticPtr->pFrameBufferPtr;
  }
  if (bVar1) {
    g_tsaStaticPtr->dwSystemFlags = g_tsaStaticPtr->dwSystemFlags | 0x400000;
  }
  return;
}


// ============================================================================
// Function: TSAStatic_GetCurrTimeStamp @ 6baa2cfe
// ============================================================================

void * TSAStatic_GetCurrTimeStamp(void)

{
  short sVar1;
  void *pvVar2;
  
                    /* 0x62cfe  1676  TSAStatic_GetCurrTimeStamp */
  if (g_tsaStaticPtr->pFrameBufferPtr == (void *)0x0) {
    sVar1 = Touch_GetFormattedTimeInterval(0);
    pvVar2 = (void *)((longlong)sVar1 * (ulonglong)(uint)g_tsaStaticPtr->nFrameCounter);
  }
  else {
    pvVar2 = g_tsaStaticPtr->pFrameBufferPtr;
  }
  return pvVar2;
}


// ============================================================================
// Function: TSAStatic_IsSafeBaselineApplied @ 6baa30f3
// ============================================================================

undefined8 TSAStatic_IsSafeBaselineApplied(void)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0x630f3  1705  TSAStatic_IsSafeBaselineApplied */
  cVar1 = SafeBaseline_IsAvailable();
  if ((cVar1 == '\0') || ((g_tsaStaticPtr->dwFeatureFlags & FEAT_SAFE_BASELINE) == 0)) {
    uVar2 = 0;
  }
  else {
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: TSAStatic_IsSignalReverted @ 6baa2c81
// ============================================================================

uchar TSAStatic_IsSignalReverted(void)

{
                    /* 0x62c81  1707  TSAStatic_IsSignalReverted */
  return g_tsaStaticPtr->bField_0x11a;
}


// ============================================================================
// Function: TSAStatic_IsFreqShiftDone @ 6baa23cd
// ============================================================================

bool TSAStatic_IsFreqShiftDone(void)

{
  TSA_SystemFlags TVar1;
  char cVar2;
  bool local_9;
  
                    /* 0x623cd  1694  TSAStatic_IsFreqShiftDone */
  TVar1 = g_tsaStaticPtr->dwSystemFlags;
  cVar2 = AFE_IsFreqShiftDone();
  local_9 = cVar2 != '\0' || (TVar1 & 0x10) != 0;
  return local_9;
}


// ============================================================================
// Function: TSAStatic_IsAllFreqBecomeNoisy @ 6baa260a
// ============================================================================

undefined1 TSAStatic_IsAllFreqBecomeNoisy(void)

{
  TSAStatic *pTVar1;
  undefined1 uVar2;
  undefined1 local_19;
  
                    /* 0x6260a  1689  TSAStatic_IsAllFreqBecomeNoisy */
  pTVar1 = g_tsaStaticPtr;
  uVar2 = TSAStatic_IsAllFreqNoisy();
  pTVar1->field_0xd = uVar2;
  if ((g_tsaStaticPtr->field_0xd == '\0') || (g_tsaStaticPtr->field_0xe != '\0')) {
    local_19 = 0;
  }
  else {
    local_19 = 1;
  }
  return local_19;
}


// ============================================================================
// Function: TSAStatic_ProcessAftTouchEvent @ 6baa316a
// ============================================================================

void TSAStatic_ProcessAftTouchEvent(void)

{
                    /* 0x6316a  1710  TSAStatic_ProcessAftTouchEvent */
  TSAStatic_HTIUpdate();
  TSAStatic_SafeDiffCheck();
  return;
}


// ============================================================================
// Function: TSAStatic_IsAFTModeEnabled @ 6baa2cda
// ============================================================================

undefined4 TSAStatic_IsAFTModeEnabled(void)

{
  uint uVar1;
  
                    /* 0x62cda  1688  TSAStatic_IsAFTModeEnabled */
  uVar1 = _refptr_g_tsaPrmtFlash->dwAftModeFeatureMask & g_tsaStaticPtr->dwAftRuntimeModeMask;
  return CONCAT31((int3)(uVar1 >> 8),uVar1 != 0);
}


// ============================================================================
// Function: TSAStatic_IsAFEJustExitIDLEMode @ 6baa265a
// ============================================================================

undefined1 TSAStatic_IsAFEJustExitIDLEMode(void)

{
  undefined1 local_9;
  
                    /* 0x6265a  1686  TSAStatic_IsAFEJustExitIDLEMode */
  if ((g_tsaStaticPtr->field_0x3 == '\0') && (g_tsaStaticPtr->field_0x4 != '\0')) {
    local_9 = 1;
  }
  else {
    local_9 = 0;
  }
  return local_9;
}


// ============================================================================
// Function: TSAStatic_IsAFEJustEnterIDLEMode @ 6baa26ce
// ============================================================================

undefined1 TSAStatic_IsAFEJustEnterIDLEMode(void)

{
  undefined1 local_9;
  
                    /* 0x626ce  1684  TSAStatic_IsAFEJustEnterIDLEMode */
  if ((g_tsaStaticPtr->field_0x3 == '\0') || (g_tsaStaticPtr->field_0x4 != '\0')) {
    local_9 = 0;
  }
  else {
    local_9 = 1;
  }
  return local_9;
}


// ============================================================================
// Function: TSAStatic_IsAFEEnterIdleTimeout @ 6baa2742
// ============================================================================

undefined8 TSAStatic_IsAFEEnterIdleTimeout(void)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0x62742  1682  TSAStatic_IsAFEEnterIdleTimeout */
  if ((g_tsaStaticPtr->dwFeatureFlags & 0x8000) == 0) {
    if (*(longlong *)&g_tsaStaticPtr->field_0x28 != -1) {
      cVar1 = AFE_IsInIDLEMode();
      if ((cVar1 == '\0') &&
         ((void *)((ulonglong)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x3a +
                   *(longlong *)&g_tsaStaticPtr->field_0x28 + 4000) <
          g_tsaStaticPtr->pFrameBufferPtr)) {
        lastTimeOutStamp_10344 = g_tsaStaticPtr->pFrameBufferPtr;
      }
    }
    if (g_tsaStaticPtr->pFrameBufferPtr == lastTimeOutStamp_10344) {
      uVar2 = 1;
    }
    else {
      uVar2 = 0;
    }
  }
  else {
    uVar2 = 0;
  }
  return uVar2;
}


// ============================================================================
// Function: TSAStatic_DoesAFERequireToExitIDLEMode @ 6baa28bc
// ============================================================================

undefined1 TSAStatic_DoesAFERequireToExitIDLEMode(void)

{
  char cVar1;
  undefined1 local_9;
  
                    /* 0x628bc  1673  TSAStatic_DoesAFERequireToExitIDLEMode */
  local_9 = 0;
  cVar1 = TSAStatic_IsAFEJustExitIDLEMode();
  if (cVar1 == '\0') {
    if (((*(longlong *)&g_tsaStaticPtr->field_0x18 != -1) &&
        (*(void **)&g_tsaStaticPtr->field_0x18 <= g_tsaStaticPtr->pFrameBufferPtr)) &&
       (g_tsaStaticPtr->pFrameBufferPtr < (void *)(*(longlong *)&g_tsaStaticPtr->field_0x18 + 100)))
    {
      local_9 = 1;
      *(uint *)&g_tsaStaticPtr->field_0x9c = *(uint *)&g_tsaStaticPtr->field_0x9c | 0x1000000;
    }
  }
  else {
    *(void **)&g_tsaStaticPtr->field_0x18 = g_tsaStaticPtr->pFrameBufferPtr;
    local_9 = 1;
    *(uint *)&g_tsaStaticPtr->field_0x9c = *(uint *)&g_tsaStaticPtr->field_0x9c | 0x800000;
  }
  return local_9;
}


// ============================================================================
// Function: TSAStatic_IsAFEEnterSideIdleTimeout @ 6baa27ff
// ============================================================================

undefined8 TSAStatic_IsAFEEnterSideIdleTimeout(void)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0x627ff  1683  TSAStatic_IsAFEEnterSideIdleTimeout */
  if ((g_tsaStaticPtr->dwFeatureFlags & 0x8000) == 0) {
    if (*(longlong *)&g_tsaStaticPtr->field_0x30 != -1) {
      cVar1 = AFE_IsInSideIDLEMode();
      if ((cVar1 == '\0') &&
         ((void *)((ulonglong)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x3a +
                   *(longlong *)&g_tsaStaticPtr->field_0x30 + 4000) <
          g_tsaStaticPtr->pFrameBufferPtr)) {
        lastTimeOutStamp_10349 = g_tsaStaticPtr->pFrameBufferPtr;
      }
    }
    if (g_tsaStaticPtr->pFrameBufferPtr == lastTimeOutStamp_10349) {
      uVar2 = 1;
    }
    else {
      uVar2 = 0;
    }
  }
  else {
    uVar2 = 0;
  }
  return uVar2;
}


// ============================================================================
// Function: TSAStatic_DoesAFERequireToExitSideIDLEMode @ 6baa298f
// ============================================================================

undefined1 TSAStatic_DoesAFERequireToExitSideIDLEMode(void)

{
  char cVar1;
  undefined1 local_9;
  
                    /* 0x6298f  1674  TSAStatic_DoesAFERequireToExitSideIDLEMode */
  local_9 = 0;
  cVar1 = TSAStatic_IsAFEJustExitSideIDLEMode();
  if (cVar1 == '\0') {
    if (((*(longlong *)&g_tsaStaticPtr->field_0x20 != -1) &&
        (*(void **)&g_tsaStaticPtr->field_0x20 <= g_tsaStaticPtr->pFrameBufferPtr)) &&
       (g_tsaStaticPtr->pFrameBufferPtr < (void *)(*(longlong *)&g_tsaStaticPtr->field_0x20 + 100)))
    {
      local_9 = 1;
    }
  }
  else {
    *(void **)&g_tsaStaticPtr->field_0x20 = g_tsaStaticPtr->pFrameBufferPtr;
    local_9 = 1;
  }
  return local_9;
}


// ============================================================================
// Function: TSAStatic_IsOKToEnterIDLEMode @ 6baa2a96
// ============================================================================

undefined1 TSAStatic_IsOKToEnterIDLEMode(void)

{
                    /* 0x62a96  1702  TSAStatic_IsOKToEnterIDLEMode */
  return g_tsaStaticPtr->field_0x9;
}


// ============================================================================
// Function: TSAStatic_IsOKToEnterSideIDLEMode @ 6baa2b49
// ============================================================================

undefined1 TSAStatic_IsOKToEnterSideIDLEMode(void)

{
                    /* 0x62b49  1703  TSAStatic_IsOKToEnterSideIDLEMode */
  return g_tsaStaticPtr->field_0xc;
}


// ============================================================================
// Function: TSAStatic_IsChargerPlugged @ 6baa2434
// ============================================================================

undefined1 TSAStatic_IsChargerPlugged(void)

{
  char cVar1;
  undefined1 local_9;
  
                    /* 0x62434  1691  TSAStatic_IsChargerPlugged */
  local_9 = 0;
  cVar1 = TSAStatic_IsChargerPluggedNoDebounce();
  if (cVar1 == '\0') {
    if (((*(longlong *)&g_tsaStaticPtr->field_0x60 != -1) &&
        (*(void **)&g_tsaStaticPtr->field_0x60 <= g_tsaStaticPtr->pFrameBufferPtr)) &&
       (g_tsaStaticPtr->pFrameBufferPtr < (void *)(*(longlong *)&g_tsaStaticPtr->field_0x60 + 2000))
       ) {
      local_9 = 1;
    }
  }
  else {
    local_9 = 1;
  }
  return local_9;
}


// ============================================================================
// Function: TSAStatic_IsLargeDisplayNoise @ 6baa24f9
// ============================================================================

undefined8 TSAStatic_IsLargeDisplayNoise(void)

{
  char cVar1;
  
                    /* 0x624f9  1701  TSAStatic_IsLargeDisplayNoise */
  if ((((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold * 2 <
        (int)*(short *)&_refptr_g_tsaPrpt->field_0x160) &&
      (_refptr_g_tsaPrmtFlash->wBufElementCount >> 2 < *(ushort *)&_refptr_g_tsaPrpt->field_0x162))
     || (g_tsaStaticPtr->field_0xe != '\0')) {
    if ((g_tsaStaticPtr->dwSystemFlags & 0x20000) != 0) {
      return 1;
    }
    cVar1 = AFE_IsAllFreqNoisy();
    if (cVar1 != '\0') {
      return 1;
    }
  }
  return 0;
}


// ============================================================================
// Function: TSAStatic_IsJustOKToEnterIDLEMode @ 6baa2a22
// ============================================================================

undefined1 TSAStatic_IsJustOKToEnterIDLEMode(void)

{
  undefined1 local_9;
  
                    /* 0x62a22  1697  TSAStatic_IsJustOKToEnterIDLEMode */
  if ((g_tsaStaticPtr->field_0x7 == '\0') || (g_tsaStaticPtr->field_0x8 != '\0')) {
    local_9 = 0;
  }
  else {
    local_9 = 1;
  }
  return local_9;
}


// ============================================================================
// Function: TSAStatic_IsJustOKToEnterSideIDLEMode @ 6baa2a5c
// ============================================================================

undefined1 TSAStatic_IsJustOKToEnterSideIDLEMode(void)

{
  undefined1 local_9;
  
                    /* 0x62a5c  1698  TSAStatic_IsJustOKToEnterSideIDLEMode */
  if ((g_tsaStaticPtr->field_0xa == '\0') || (g_tsaStaticPtr->field_0xb != '\0')) {
    local_9 = 0;
  }
  else {
    local_9 = 1;
  }
  return local_9;
}


// ============================================================================
// Function: TSAStatic_IsCouplingNoiseEstimateNeeded @ 6ba5b769
// ============================================================================

bool TSAStatic_IsCouplingNoiseEstimateNeeded(void)

{
  char cVar1;
  int iVar2;
  bool local_9;
  
  cVar1 = TSAStatic_IsCouplingNoiseEstimateApplied();
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    iVar2 = *(int *)&_refptr_g_tsaPrpt->field_0x234;
  }
  else {
    iVar2 = *(int *)&_refptr_g_tsaPrpt->field_0x258;
  }
  local_9 = 999 < -iVar2 && cVar1 != '\0';
  if ((*_refptr_g_prevTouchesPtr)->bPrevTouchCount == '\0') {
    local_9 = false;
  }
  return local_9;
}


// ============================================================================
// Function: TSAStatic_IsChargerPluggedNoDebounce @ 6baa24b0
// ============================================================================

bool TSAStatic_IsChargerPluggedNoDebounce(void)

{
  bool bVar1;
  
                    /* 0x624b0  1692  TSAStatic_IsChargerPluggedNoDebounce */
  bVar1 = (g_tsaStaticPtr->dwSystemFlags & 0x200020) != 0;
  if (bVar1) {
    *(void **)&g_tsaStaticPtr->field_0x60 = g_tsaStaticPtr->pFrameBufferPtr;
  }
  return bVar1;
}


// ============================================================================
// Function: TSAStatic_SafeDiffCheck @ 6baa1de6
// ============================================================================

void TSAStatic_SafeDiffCheck(void)

{
  char cVar1;
  
                    /* 0x61de6  1712  TSAStatic_SafeDiffCheck */
  if (g_tsaStaticPtr->bResetDiffRequested == '\0') {
    cVar1 = AFE_IsRawUnstable();
    if (cVar1 == '\0') {
      cVar1 = AFE_IsRawJump();
      if (cVar1 == '\0') {
        if (((*(int *)&g_tsaStaticPtr->field_0xac != 0) &&
            (*(char *)(*(longlong *)_refptr_g_touchesPtr + 8) == '\0')) &&
           (*(ushort *)&_refptr_g_tsaPrpt->field_0x1c4 < 0x32)) {
          g_tsaStaticPtr->field_0xa9 = 1;
          g_tsaStaticPtr->field_0xaa = 0;
        }
        goto LAB_6baa1e97;
      }
    }
  }
  *(undefined4 *)&g_tsaStaticPtr->field_0xac = 0;
  *(undefined4 *)&g_tsaStaticPtr->field_0xb0 = 0;
  g_tsaStaticPtr->field_0xa9 = 0;
LAB_6baa1e97:
  if (g_tsaStaticPtr->field_0xa9 == '\0') {
    if ((3 < (int)((uint)(*_refptr_g_prevTouchesPtr)->bPrevTouchCount -
                  (uint)(byte)(*_refptr_g_prevTouchesPtr)->field_0xd)) &&
       ((*_refptr_g_prevTouchesPtr)->field_0x10 != '\0')) {
      g_tsaStaticPtr->field_0xaa = 1;
    }
    if (2 < (byte)(*_refptr_g_prevTouchesPtr)->field_0x10) {
      g_tsaStaticPtr->field_0xaa = 1;
    }
    if (((g_tsaStaticPtr->dwFeatureFlags & FEAT_SMART_COVER) == 0) &&
       ((*_refptr_g_prevTouchesPtr)->field_0x12 != '\0')) {
      g_tsaStaticPtr->field_0xaa = 1;
    }
  }
  return;
}


// ============================================================================
// Function: TSAStatic_HTIUpdate @ 6baa3091
// ============================================================================

void TSAStatic_HTIUpdate(void)

{
  byte local_9;
  
                    /* 0x63091  1680  TSAStatic_HTIUpdate */
  for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
    if (*(int *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x2a0)
        == 1) {
      TSAStatic_HTIFilter(local_9);
    }
  }
  return;
}


// ============================================================================
// Function: TSAStatic_Init @ 6baa1a42
// ============================================================================

void TSAStatic_Init(void)

{
  TSAStatic *pTVar1;
  short sVar2;
  int iVar3;
  
                    /* 0x61a42  1681  TSAStatic_Init */
  TSAStatic_InitSysModuleEveryTime();
  TSAStatic_InitInternalModuleEveryTime();
  iVar3 = TSAStatic_SetLogName("TSA_DUMP");
  if (iVar3 != 1) {
    if (flagOnceOnly_10264 != '\0') {
      g_tsaStaticPtr->pSensorDataCallback = _refptr_TSA_GetSensorData;
      g_tsaStaticPtr->llField_0x148 = 0;
      g_tsaStaticPtr->dwAftRuntimeModeMask = 0;
      g_tsaStaticPtr->bField_0x11b = '\x01';
      g_tsaStaticPtr->wField_0x2e0 = 0;
      pTVar1 = g_tsaStaticPtr;
      sVar2 = TSA_GetXRes();
      pTVar1->nXResMax = sVar2 + -1;
      g_tsaStaticPtr->wField_0x2e4 = 0;
      pTVar1 = g_tsaStaticPtr;
      sVar2 = TSA_GetYRes();
      pTVar1->nYResMax = sVar2 + -1;
      g_tsaStaticPtr->nField_0x26c = 0;
      g_tsaStaticPtr->wField_0x292 = _refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold;
      g_tsaStaticPtr->llField_0x50 = -1;
      g_tsaStaticPtr->llField_0x58 = -1;
      g_tsaStaticPtr->llField_0x48 = -1;
      g_tsaStaticPtr->dwStylusAftGateFlags = 1;
      g_tsaStaticPtr->dwField_0x158 = 1;
      g_tsaStaticPtr->nField_0x268 = 0;
    }
    flagOnceOnly_10264 = '\0';
  }
  return;
}


// ============================================================================
// Function: TSAStatic_IsAFEJustExitSideIDLEMode @ 6baa2694
// ============================================================================

undefined1 TSAStatic_IsAFEJustExitSideIDLEMode(void)

{
  undefined1 local_9;
  
                    /* 0x62694  1687  TSAStatic_IsAFEJustExitSideIDLEMode */
  if ((g_tsaStaticPtr->field_0x5 == '\0') && (g_tsaStaticPtr->field_0x6 != '\0')) {
    local_9 = 1;
  }
  else {
    local_9 = 0;
  }
  return local_9;
}


// ============================================================================
// Function: TSAStatic_IsCouplingNoiseEstimateApplied @ 6baa312c
// ============================================================================

undefined8 TSAStatic_IsCouplingNoiseEstimateApplied(void)

{
  char cVar1;
  
                    /* 0x6312c  1693  TSAStatic_IsCouplingNoiseEstimateApplied */
  if (((_refptr_g_tsaPrmtDynamic[0x92] != '\0') && (cVar1 = TSA_IsGameScenario(), cVar1 == '\0')) &&
     (cVar1 = TSAStatic_IsChargerPlugged(), cVar1 == '\0')) {
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: TSAStatic_HTIFilter @ 6baa2f56
// ============================================================================

void TSAStatic_HTIFilter(byte param_1)

{
  ushort uVar1;
  int iVar2;
  
                    /* 0x62f56  1679  TSAStatic_HTIFilter */
  uVar1 = MIN((int)*(short *)((longlong)(int)(uint)param_1 * 0x168 +
                              *(longlong *)_refptr_g_touchesPtr + 0x1ba),
              (int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold);
  if (flagOnceOnly_10424 == '\0') {
    if ((short)g_tsaStaticPtr->wField_0x292 < (short)uVar1) {
      g_tsaStaticPtr->wField_0x292 = uVar1;
    }
    else if (9 < *(ushort *)
                  ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x228)
            ) {
      iVar2 = (int)(short)uVar1 + (short)g_tsaStaticPtr->wField_0x292 * 0xf;
      if (iVar2 < 0) {
        iVar2 = iVar2 + 0xf;
      }
      g_tsaStaticPtr->wField_0x292 = (ushort)(iVar2 >> 4);
    }
  }
  else if (9 < *(ushort *)
                ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x228))
  {
    g_tsaStaticPtr->wField_0x292 = uVar1;
    flagOnceOnly_10424 = '\0';
  }
  return;
}


// ============================================================================
// Function: TSAStatic_InitSysModuleEveryTime @ 6baa15c6
// ============================================================================

void TSAStatic_InitSysModuleEveryTime(void)

{
  g_tsaStaticPtr->llField_0xe8 = 0;
  g_tsaStaticPtr->nFrameCounter = 0;
  g_tsaStaticPtr->bField_0x110 = '\0';
  g_tsaStaticPtr->bField_0x111 = '\0';
  g_tsaStaticPtr->bField_0x116 = '\0';
  g_tsaStaticPtr->dwFeatureFlags = _refptr_g_tsaPrmtFlash->dwFeatureFlagsBase1;
  g_tsaStaticPtr->dwFeatureFlags =
       _refptr_g_tsaPrmtFlash->dwFeatureFlagsBase1_or | g_tsaStaticPtr->dwFeatureFlags;
  g_tsaStaticPtr->dwFeatureFlagsPrev = g_tsaStaticPtr->dwFeatureFlags;
  g_tsaStaticPtr->dwFeatureFlagsMasked = g_tsaStaticPtr->dwFeatureFlags & 0x4813;
  g_tsaStaticPtr->dwFeatureFlags2 = _refptr_g_tsaPrmtFlash->dwFeatureFlagsBase2;
  g_tsaStaticPtr->dwFeatureFlags2 =
       _refptr_g_tsaPrmtFlash->dwFeatureFlagsBase2_or | g_tsaStaticPtr->dwFeatureFlags2;
  g_tsaStaticPtr->wField_0x11e = 0;
  g_tsaStaticPtr->wField_0x120 = 0;
  g_tsaStaticPtr->wField_0x122 = 0;
  g_tsaStaticPtr->nField_0xc0 = 0;
  g_tsaStaticPtr->llField_0xd0 = 0;
  g_tsaStaticPtr->nField_0xdc = 0;
  g_tsaStaticPtr->nField_0x124 = 0;
  g_tsaStaticPtr->nField_0x128 = 0;
  g_tsaStaticPtr->nField_0x12c = 0;
  g_tsaStaticPtr->nField_0x130 = 0;
  g_tsaStaticPtr->nField_0x134 = 0;
  g_tsaStaticPtr->nField_0x138 = 0;
  g_tsaStaticPtr->bField_0x118 = '\0';
  g_tsaStaticPtr->bField_0x119 = '\0';
  g_tsaStaticPtr->bField_0x11a = '\0';
  g_tsaStaticPtr->bCmfProcessingBlocked = '\0';
  g_tsaStaticPtr->nField_0x280 = 0;
  g_tsaStaticPtr->nField_0x278 = 0;
  g_tsaStaticPtr->nField_0x284 = 4;
  g_tsaStaticPtr->bFullIncellSignalDisparity = '\0';
  g_tsaStaticPtr->nField_0x288 = 0;
  g_tsaStaticPtr->nField_0x28c = 0;
  g_tsaStaticPtr->nField_0x264 = 0;
  g_tsaStaticPtr->nField_0x270 = 0;
  g_tsaStaticPtr->bField_0x295 = '\0';
  g_tsaStaticPtr->bField_0x290 = '\0';
  g_tsaStaticPtr->nField_0x298 = 0;
  g_tsaStaticPtr->nField_0x29c = 0;
  g_tsaStaticPtr->nField_0x2a0 = 0;
  g_tsaStaticPtr->bFrameDoneFlag = '\0';
  return;
}


// ============================================================================
// Function: TSAStatic_InitInternalModuleEveryTime @ 6baa18b1
// ============================================================================

void TSAStatic_InitInternalModuleEveryTime(void)

{
  g_tsaStaticPtr->bField_0x00 = '\0';
  g_tsaStaticPtr->field_0xd = 0;
  g_tsaStaticPtr->field_0x2 = 0;
  g_tsaStaticPtr->field_0x3 = 0;
  g_tsaStaticPtr->field_0x5 = 0;
  g_tsaStaticPtr->field_0x7 = 0;
  g_tsaStaticPtr->field_0xa = 0;
  g_tsaStaticPtr->field_0xf = 0;
  *(undefined8 *)&g_tsaStaticPtr->field_0x18 = 0xffffffffffffffff;
  *(undefined8 *)&g_tsaStaticPtr->field_0x28 = 0xffffffffffffffff;
  *(undefined8 *)&g_tsaStaticPtr->field_0x20 = 0xffffffffffffffff;
  *(undefined8 *)&g_tsaStaticPtr->field_0x30 = 0xffffffffffffffff;
  *(undefined8 *)&g_tsaStaticPtr->field_0x38 = 0xffffffffffffffff;
  *(undefined8 *)&g_tsaStaticPtr->field_0x40 = 0xffffffffffffffff;
  *(undefined8 *)&g_tsaStaticPtr->field_0x60 = 0xffffffffffffffff;
  *(undefined8 *)&g_tsaStaticPtr->field_0x68 = 0xffffffffffffffff;
  *(undefined8 *)&g_tsaStaticPtr->field_0x70 = 0xffffffffffffffff;
  *(undefined8 *)&g_tsaStaticPtr->field_0x80 = 0xffffffffffffffff;
  *(undefined8 *)&g_tsaStaticPtr->field_0x78 = 0;
  g_tsaStaticPtr->llAutoCalibrationStartTime = 0;
  *(undefined8 *)&g_tsaStaticPtr->field_0x90 = 0;
  g_tsaStaticPtr->field_0xa0 = 0;
  g_tsaStaticPtr->field_0xa1 = 0;
  g_tsaStaticPtr->field_0xa8 = 0;
  *(undefined4 *)&g_tsaStaticPtr->field_0xac = 0;
  *(undefined4 *)&g_tsaStaticPtr->field_0xb0 = 0;
  g_tsaStaticPtr->field_0xa9 = 0;
  g_tsaStaticPtr->field_0xaa = 0;
  return;
}


// ============================================================================
// Function: TSAStatic_SetLogName @ 6baa2e93
// ============================================================================

undefined8 TSAStatic_SetLogName(char *param_1)

{
  errno_t eVar1;
  
                    /* 0x62e93  1714  TSAStatic_SetLogName */
  eVar1 = strncpy_s(&g_tsaStaticPtr->field_0x2a4,0x32,param_1,0x31);
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_Static.c",
                      0x310,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  g_tsaStaticPtr->field_0x2d5 = 0;
  return 0;
}


// ============================================================================
// Function: TSAStatic_IsScreenRotated @ 6baa2e39
// ============================================================================

undefined8 TSAStatic_IsScreenRotated(void)

{
  undefined8 uVar1;
  
                    /* 0x62e39  1706  TSAStatic_IsScreenRotated */
  if ((g_tsaStaticPtr->dwSystemFlags & 0x80) == 0) {
    if (g_tsaStaticPtr->nField_0x264 == 1) {
      uVar1 = 1;
    }
    else if (g_tsaStaticPtr->nField_0x264 == 3) {
      uVar1 = 1;
    }
    else {
      uVar1 = 0;
    }
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: TSAStatic_RestoreExtFeatures @ 6baa2d74
// ============================================================================

void TSAStatic_RestoreExtFeatures(TSA_FeatureFlags param_1)

{
                    /* 0x62d74  1711  TSAStatic_RestoreExtFeatures */
  g_tsaStaticPtr->dwFeatureFlags =
       g_tsaStaticPtr->dwFeatureFlags & ~(param_1 & 0x4813) |
       g_tsaStaticPtr->dwFeatureFlagsMasked & param_1 & 0x4813;
  return;
}


// ============================================================================
// Function: TSAStatic_GetSigThold @ 6baa2f14
// ============================================================================

uint TSAStatic_GetSigThold(int param_1)

{
  uint uVar1;
  
                    /* 0x62f14  1678  TSAStatic_GetSigThold */
  if (param_1 == 1) {
    uVar1 = ((short)g_tsaStaticPtr->wField_0x292 * 2) / 5;
  }
  else {
    uVar1 = (uint)*(ushort *)(_refptr_g_tsaPrmtDynamic + 8);
  }
  return uVar1;
}


