// ============================================================================
// Function: BLReset_InDebounce @ 6ba47750
// ============================================================================

bool BLReset_InDebounce(void)

{
                    /* 0x7750  358  BLReset_InDebounce */
  return (*_refptr_g_tsaStaticPtr)->bBaselineResetInDebounce != '\0';
}


// ============================================================================
// Function: BLReset_Process @ 6ba4bf90
// ============================================================================

char BLReset_Process(char param_1)

{
  char cVar1;
  char local_9;
  
                    /* 0xbf90  369  BLReset_Process */
  local_9 = '\0';
  *(undefined4 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 = 0;
  if (param_1 != '\0') {
    *(undefined2 *)&_refptr_g_tsaPrpt->field_0x286 = 0;
  }
  cVar1 = BLReset_IsToDisable();
  if (cVar1 == '\0') {
    (*_refptr_g_tsaStaticPtr)->bBaselineResetCheckEnabled = '\x01';
  }
  else {
    (*_refptr_g_tsaStaticPtr)->bBaselineResetCheckEnabled = '\0';
  }
  if (param_1 == '\0') {
    if ((*_refptr_g_tsaStaticPtr)->bBaselineResetCheckEnabled == '\0') goto LAB_6ba4c020;
    cVar1 = BLReset_IsTriggered();
    if (cVar1 == '\0') goto LAB_6ba4c020;
  }
  local_9 = '\x01';
LAB_6ba4c020:
  if (*(ushort *)&_refptr_g_tsaPrpt->field_0x284 < *(ushort *)(_refptr_g_tsaPrmtDynamic + 0x58)) {
    *(short *)&_refptr_g_tsaPrpt->field_0x284 = *(short *)&_refptr_g_tsaPrpt->field_0x284 + 1;
  }
  if (*(ushort *)&_refptr_g_tsaPrpt->field_0x286 < *(ushort *)(_refptr_g_tsaPrmtDynamic + 0x5a)) {
    *(short *)&_refptr_g_tsaPrpt->field_0x286 = *(short *)&_refptr_g_tsaPrpt->field_0x286 + 1;
  }
  if (local_9 != '\0') {
    *(undefined2 *)&_refptr_g_tsaPrpt->field_0x286 = 0;
  }
  return local_9;
}


// ============================================================================
// Function: BLReset_IsBaselineNoisy @ 6ba4c0ba
// ============================================================================

undefined1 BLReset_IsBaselineNoisy(void)

{
  char cVar1;
  undefined1 local_9;
  
                    /* 0xc0ba  362  BLReset_IsBaselineNoisy */
  if (((_refptr_g_tsaPrpt->dwPrevBaselinePropertyState != 6) &&
      (_refptr_g_tsaPrpt->dwPrevBaselinePropertyState != 3)) &&
     (_refptr_g_tsaPrpt->dwPrevBaselinePropertyState != 0)) {
    cVar1 = AFE_IsAlwaysNoisy();
    if (cVar1 != '\0') {
      *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x300 =
           (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
    }
  }
  if (*(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x300 == 0) {
    local_9 = 0;
  }
  else if (*(ushort *)&_refptr_g_tsaPrpt->field_0x4 < 6) {
    local_9 = 1;
    (*_refptr_g_tsaStaticPtr)->field_0x2f9 = 1;
    *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x300 = 0;
  }
  else if (*(ulonglong *)&(*_refptr_g_tsaStaticPtr)->field_0x300 < 0xa7) {
    local_9 = 1;
    *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x300 =
         (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal +
         *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x300;
    _refptr_g_tsaPrpt->dwBaselinePropertyState = 6;
  }
  else {
    local_9 = 0;
    *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x300 = 0;
  }
  return local_9;
}


// ============================================================================
// Function: BLReset_Status @ 6ba4776c
// ============================================================================

undefined1 BLReset_Status(void)

{
                    /* 0x776c  370  BLReset_Status */
  return (*_refptr_g_tsaStaticPtr)->field_0x2f8;
}


// ============================================================================
// Function: BLReset_FlagDisableBaselineUpdateInit @ 6ba47ae6
// ============================================================================

void BLReset_FlagDisableBaselineUpdateInit(void)

{
                    /* 0x7ae6  357  BLReset_FlagDisableBaselineUpdateInit */
  (*_refptr_g_tsaStaticPtr)[1].field_0x2 = 0;
  return;
}


// ============================================================================
// Function: BLReset_IsToDisable @ 6ba47afd
// ============================================================================

undefined8 BLReset_IsToDisable(void)

{
  return 0;
}


// ============================================================================
// Function: BLReset_IsTriggered @ 6ba4bf6d
// ============================================================================

void BLReset_IsTriggered(void)

{
  char cVar1;
  
  cVar1 = TSAStatic_IsSafeBaselineApplied();
  if (cVar1 == '\0') {
    BLReset_IsTriggered2();
  }
  else {
    SafeBaseline_IsResetTriggered();
  }
  return;
}


// ============================================================================
// Function: BLReset_Init @ 6ba47783
// ============================================================================

void BLReset_Init(char param_1)

{
  int iVar1;
  
                    /* 0x7783  359  BLReset_Init */
  *(undefined2 *)&_refptr_g_tsaPrpt->field_0x284 = 0;
  *(undefined2 *)&_refptr_g_tsaPrpt->field_0x286 = 0;
  _refptr_g_tsaPrpt->field_0x29c = 0;
  _refptr_g_tsaPrpt->dwBaselinePropertyState = 0;
  _refptr_g_tsaPrpt->dwPrevBaselinePropertyState = 0;
  (*_refptr_g_tsaStaticPtr)->bBaselineResetCheckEnabled = '\x01';
  (*_refptr_g_tsaStaticPtr)->bBaselineResetInDebounce = '\0';
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x2f0 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x310 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x330 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x340 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x350 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x360 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x368 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x370 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x378 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x380 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x388 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x390 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x3a8 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x3b0 = 0;
  *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3b8 = 0;
  *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3bc = 0;
  *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3c0 = 0;
  *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3c4 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x3d0 = 0;
  *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3d8 = 0;
  (*_refptr_g_tsaStaticPtr)->field_0x2f8 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x300 = 0;
  (*_refptr_g_tsaStaticPtr)->field_0x2f9 = 0;
  (*_refptr_g_tsaStaticPtr)->field_0x30a = 0;
  (*_refptr_g_tsaStaticPtr)->field_0x318 = 0;
  (*_refptr_g_tsaStaticPtr)->bBaselineUpdatedThisFrame = '\0';
  (*_refptr_g_tsaStaticPtr)[1].field_0x1 = 0;
  (*_refptr_g_tsaStaticPtr)[1].field_0x3 = 0;
  *(undefined4 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 = 0;
  (*_refptr_g_tsaStaticPtr)->field_0x3ea = 0;
  iVar1 = memset_s(*(undefined8 *)_refptr_g_tsaBufCleanBaseline,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2,0,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/Baseline/BaselineReset.c",0x72,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  if (param_1 != '\0') {
    (*_refptr_g_tsaStaticPtr)->bBaselineResetPropertyLatched = '\0';
  }
  return;
}


// ============================================================================
// Function: BLReset_CheckNumOverNoise @ 6ba4850e
// ============================================================================

undefined8 BLReset_CheckNumOverNoise(void)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0x850e  356  BLReset_CheckNumOverNoise */
  if (*(ushort *)&_refptr_g_tsaPrpt->field_0x4 < 5) {
    uVar2 = 1;
  }
  else if ((_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 2) &&
          (*(ushort *)&_refptr_g_tsaPrpt->field_0x8 < 3)) {
    uVar2 = 1;
  }
  else {
    cVar1 = TSAStatic_IsSafeBaselineApplied();
    if ((cVar1 == '\0') && (cVar1 = ToeSynaBl_CheckNumOverNoise(), cVar1 != '\0')) {
      return 1;
    }
    *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 =
         *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 | 0x800;
    uVar2 = 0;
  }
  return uVar2;
}


// ============================================================================
// Function: BLReset_IsTriggered2 @ 6ba4bc66
// ============================================================================

bool BLReset_IsTriggered2(void)

{
  char cVar1;
  char cVar2;
  bool local_9;
  
  cVar1 = BLReset_IsLongTimeRawJumpDetected();
  if (cVar1 != '\0') {
    _refptr_g_tsaPrpt->dwBaselinePropertyState = 7;
  }
  cVar2 = BLReset_IsTriggered2BySS();
  local_9 = cVar2 != '\0' || cVar1 != '\0';
  cVar1 = AFE_IsFrequentlyHopping();
  if ((cVar1 != '\0') &&
     ((*_refptr_g_tsaStaticPtr)->pFrameBufferPtr <
      (void *)(*(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x80 + 15000))) {
    (*_refptr_g_tsaStaticPtr)->field_0x3ea = 0;
    _refptr_g_tsaPrpt->dwBaselinePropertyState = 0xd;
    (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
         (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 1;
  }
  cVar1 = BLReset_IsTriggered2AfeWakeUp();
  if (cVar1 != '\0') {
    local_9 = true;
  }
  cVar1 = BLReset_IsSmartCoverOff();
  if (cVar1 != '\0') {
    local_9 = true;
    (*_refptr_g_tsaStaticPtr)->field_0x3ea = 0;
    _refptr_g_tsaPrpt->dwBaselinePropertyState = 0xc;
    (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
         (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 1;
  }
  cVar1 = SensorData_AntiTouchStageTimeout();
  if (cVar1 != '\0') {
    local_9 = true;
    _refptr_g_tsaPrpt->dwBaselinePropertyState = 0x17;
    (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
         (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 1;
  }
  if ((_refptr_g_tsaPrmtFlash->field_0x6b != '\0') &&
     (cVar1 = BLReset_IsBaselineAbnormal(), cVar1 != '\0')) {
    local_9 = true;
    _refptr_g_tsaPrpt->dwBaselinePropertyState = 0xf;
    (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
         (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 1;
  }
  cVar1 = BLReset_IsCurBaselineDirty();
  if (cVar1 != '\0') {
    local_9 = true;
    (*_refptr_g_tsaStaticPtr)->field_0x3ea = 0;
    _refptr_g_tsaPrpt->dwBaselinePropertyState = 0x13;
    (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
         (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 1;
  }
  cVar1 = ToeSynaBl_IsBaselineAbnormalAftScreenOn();
  if (cVar1 != '\0') {
    local_9 = true;
    (*_refptr_g_tsaStaticPtr)->field_0x3ea = 0;
    _refptr_g_tsaPrpt->dwBaselinePropertyState = 0x12;
    (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
         (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 1;
  }
  cVar1 = BLReset_IsAfeRawJump();
  if (cVar1 != '\0') {
    local_9 = true;
    (*_refptr_g_tsaStaticPtr)->field_0x3ea = 0;
    _refptr_g_tsaPrpt->dwBaselinePropertyState = 0x10;
    (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
         (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 1;
  }
  cVar1 = NotchProtect_IsNotchBaselineDirty();
  if (cVar1 != '\0') {
    local_9 = true;
    _refptr_g_tsaPrpt->dwBaselinePropertyState = 0x16;
    (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
         (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 1;
  }
  BLReset_CheckTouchTrustability();
  BLReset_BaselineStateUpdate(local_9);
  (*_refptr_g_tsaStaticPtr)->field_0x2f8 = local_9;
  return local_9;
}


// ============================================================================
// Function: BLReset_CheckAbnormalTouch @ 6ba48366
// ============================================================================

void BLReset_CheckAbnormalTouch(byte param_1)

{
                    /* 0x8366  355  BLReset_CheckAbnormalTouch */
  BLReset_CheckAbnormalPeak
            (*(undefined1 *)
              ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x23c));
  return;
}


// ============================================================================
// Function: BLReset_IsTouchToBeProtected2 @ 6ba47e9a
// ============================================================================

undefined1 BLReset_IsTouchToBeProtected2(char param_1)

{
  byte bVar1;
  byte bVar2;
  bool bVar3;
  ushort local_10;
  ushort local_e;
  byte local_b;
  undefined1 local_9;
  
                    /* 0x7e9a  368  BLReset_IsTouchToBeProtected2 */
  local_9 = 0;
  local_b = _refptr_g_tsaPrmtFlash->bCols - 1;
  local_e = 0;
  bVar3 = ((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) != 0;
  if (bVar3) {
    local_b = _refptr_g_tsaPrmtFlash->bCols - 2;
  }
  if ((*_refptr_g_prevTouchesPtr)->bPrevTouchCount < 6) {
    for (local_10 = 0; local_10 < (*_refptr_g_prevTouchesPtr)->bPrevTouchCount;
        local_10 = local_10 + 1) {
      bVar1 = (*_refptr_g_peaksPtr)
              [(longlong)
               (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                   (longlong)(int)(uint)local_10 * 0x478 + 0x23c) * 0x14 + 0x15].
              bPeakCount;
      bVar2 = (*_refptr_g_peaksPtr)
              [(longlong)
               (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                   (longlong)(int)(uint)local_10 * 0x478 + 0x23c) * 0x14 + 0x16].
              bPeakCount;
      if (((((((bVar3 < bVar1) && (bVar1 < local_b)) && (bVar2 != 0)) &&
            ((bVar2 < (byte)(_refptr_g_tsaPrmtFlash->bRows - 1) &&
             ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                        (longlong)(int)(uint)local_10 * 0x478 + 0x1f8) & 0x20000000) != 0)))) &&
           ((*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)local_10 * 0x478 + 0x1a0) < 0xd &&
            (((*(short *)(_refptr_g_tsaPrmtDynamic + 0xc) / 3) * 4 <
              (int)*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)local_10 * 0x478 + 0x1ba) &&
             (-(int)_refptr_g_tsaPrpt->nSPeakNegative <
              *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                        (longlong)(int)(uint)local_10 * 0x478 + 0x1be) * 2)))))) &&
          ((0 < *(int *)(_refptr_g_tsaBufSSPreCMFDif +
                        (longlong)(int)((uint)_refptr_g_tsaPrmtFlash->bCols + (uint)bVar2) * 4) ||
           (0 < *(int *)(_refptr_g_tsaBufSSPreCMFDif + (longlong)(int)(uint)bVar1 * 4))))) &&
         ((0 < *(int *)(_refptr_g_tsaBufSelf +
                       ((longlong)(int)((uint)_refptr_g_tsaPrmtFlash->bCols + (uint)bVar2) + 800) *
                       4) + 100 &&
          (0 < *(int *)(_refptr_g_tsaBufSelf + ((longlong)(int)(uint)bVar1 + 800) * 4) + 100)))) {
        local_e = local_e + 1;
      }
    }
    if ((param_1 == '\0') || (1 < local_e)) {
      if (local_e != 0) {
        local_9 = 1;
        *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 =
             *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 | 1;
      }
    }
    else {
      local_9 = 0;
    }
  }
  else {
    local_9 = 0;
  }
  return local_9;
}


// ============================================================================
// Function: BLReset_IsLongTimeRawJumpDetected @ 6ba48d9e
// ============================================================================

/* WARNING: Removing unreachable block (ram,0x6ba48de4) */
/* WARNING: Removing unreachable block (ram,0x6ba48e15) */

undefined1 BLReset_IsLongTimeRawJumpDetected(void)

{
  char cVar1;
  undefined1 local_9;
  
                    /* 0x8d9e  364  BLReset_IsLongTimeRawJumpDetected */
  local_9 = 0;
  if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3) {
    if (((uint)((int)((uint)_refptr_g_tsaPrmtFlash->wBufElementCount * 2 +
                     (uint)_refptr_g_tsaPrmtFlash->wBufElementCount) >> 2) <
         (uint)*(ushort *)&_refptr_g_tsaPrpt->field_0x4) ||
       ((uint)((int)((uint)_refptr_g_tsaPrmtFlash->wBufElementCount * 2 +
                    (uint)_refptr_g_tsaPrmtFlash->wBufElementCount) >> 2) <
        (uint)*(ushort *)&_refptr_g_tsaPrpt->field_0x20)) {
      (*_refptr_g_tsaStaticPtr)->field_0x30a = 1;
    }
    if ((0x5a < (byte)_refptr_g_tsaPrpt->field_0x18) || (0x5a < (byte)_refptr_g_tsaPrpt->field_0x34)
       ) {
      (*_refptr_g_tsaStaticPtr)->field_0x30a = 1;
    }
  }
  if (((*_refptr_g_tsaStaticPtr)->field_0x30a == '\0') ||
     (cVar1 = AFE_IsAlwaysNoisy(), cVar1 != '\0')) {
    *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x310 = 0;
    (*_refptr_g_tsaStaticPtr)->field_0x30a = 0;
    (*_refptr_g_tsaStaticPtr)->field_0x318 = 0;
    local_9 = 0;
  }
  else {
    if ((*(ushort *)&_refptr_g_tsaPrpt->field_0x4 < 0x28) &&
       (_refptr_g_tsaPrmtFlash->wBufElementCount >> 1 < *(ushort *)&_refptr_g_tsaPrpt->field_0x162))
    {
      cVar1 = BLReset_IsTouchProtectOnRawJump();
      if (cVar1 == '\0') {
        if (*(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x310 < 0x54) {
          *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x310 =
               (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal +
               *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x310;
        }
        else {
          local_9 = 1;
          *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x310 = 0;
          (*_refptr_g_tsaStaticPtr)->field_0x30a = 0;
        }
      }
    }
    else if (*(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x310 < -0x29) {
      *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x310 = 0;
      (*_refptr_g_tsaStaticPtr)->field_0x30a = 0;
    }
    else {
      *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x310 =
           *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x310 -
           (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
    }
    (*_refptr_g_tsaStaticPtr)->field_0x318 = local_9;
  }
  return local_9;
}


// ============================================================================
// Function: BLReset_IsSmartCoverOff @ 6ba49d6c
// ============================================================================

bool BLReset_IsSmartCoverOff(void)

{
  bool bVar1;
  char cVar2;
  
                    /* 0x9d6c  366  BLReset_IsSmartCoverOff */
  if ((((*_refptr_g_tsaStaticPtr)->dwFeatureFlagsPrev & 2) == 0) &&
     (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) != 0)) {
    stopCheckTimeStamp_11212 = (void *)0x0;
    *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x368 = 0;
  }
  if ((((*_refptr_g_tsaStaticPtr)->dwFeatureFlagsPrev & 2) != 0) &&
     (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) == 0)) {
    stopCheckTimeStamp_11212 = (void *)((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr + 0xfa)
    ;
    *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x368 = 0;
  }
  if ((*_refptr_g_tsaStaticPtr)->pFrameBufferPtr < stopCheckTimeStamp_11212) {
    cVar2 = BLReset_CheckNumOverNoise();
    if (cVar2 == '\0') {
      *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x368 = 0;
    }
    else {
      *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x368 =
           (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal +
           *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x368;
    }
  }
  bVar1 = 0x7c < *(ulonglong *)&(*_refptr_g_tsaStaticPtr)->field_0x368;
  if (bVar1) {
    *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x368 = 0;
    stopCheckTimeStamp_11212 = (void *)0x0;
  }
  return bVar1;
}


// ============================================================================
// Function: BLReset_BaselineStateUpdate @ 6ba4a368
// ============================================================================

void BLReset_BaselineStateUpdate(char param_1)

{
  bool bVar1;
  ushort local_e;
  byte local_b;
  undefined1 local_9;
  
  if (((*(uint *)&(*_refptr_g_tsaStaticPtr)->field_0xd8 & 0x40000) == 0) &&
     (((*_refptr_g_tsaStaticPtr)->nField_0xdc & 0x40000U) != 0)) {
    bVar1 = true;
  }
  else {
    bVar1 = false;
  }
  if ((((*_refptr_g_tsaStaticPtr)->pFrameBufferPtr ==
        *(void **)&(*_refptr_g_tsaStaticPtr)->field_0x80) || (param_1 != '\0')) || (bVar1)) {
    *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3d8 = 0;
    *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x3e0 = 0;
    *(undefined2 *)&(*_refptr_g_tsaStaticPtr)->field_0x3e8 = 0;
    (*_refptr_g_tsaStaticPtr)->field_0x3dc = 0;
  }
  else if (*(int *)&(*_refptr_g_tsaStaticPtr)->field_0x3d8 != 0x10) {
    local_9 = 0;
    local_b = _refptr_g_tsaPrmtFlash->bCols - 1;
    bVar1 = ((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) != 0;
    if (bVar1) {
      local_b = _refptr_g_tsaPrmtFlash->bCols - 2;
    }
    for (local_e = 0; local_e < (*_refptr_g_prevTouchesPtr)->bPrevTouchCount; local_e = local_e + 1)
    {
      if ((((bVar1 < (*_refptr_g_peaksPtr)
                     [(longlong)
                      (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                          (longlong)(int)(uint)local_e * 0x478 + 0x23c) * 0x14 +
                      0x15].bPeakCount) &&
           ((*_refptr_g_peaksPtr)
            [(longlong)
             (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                 (longlong)(int)(uint)local_e * 0x478 + 0x23c) * 0x14 + 0x15].
            bPeakCount < local_b)) &&
          (((*_refptr_g_peaksPtr)
            [(longlong)
             (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                 (longlong)(int)(uint)local_e * 0x478 + 0x23c) * 0x14 + 0x16].
            bPeakCount != 0 &&
           (((*_refptr_g_peaksPtr)
             [(longlong)
              (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                  (longlong)(int)(uint)local_e * 0x478 + 0x23c) * 0x14 + 0x16].
             bPeakCount < (byte)(_refptr_g_tsaPrmtFlash->bRows - 1) &&
            ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)local_e * 0x478 + 0x1f8) & 0x20000000) != 0)))))) &&
         (*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)local_e * 0x478 + 0x1a0) < 0xc)) {
        local_9 = 1;
        break;
      }
    }
    BLReset_BaselineStateMachine(local_9);
  }
  return;
}


// ============================================================================
// Function: BLReset_CheckTouchTrustability @ 6ba4a613
// ============================================================================

void BLReset_CheckTouchTrustability(void)

{
  uchar uVar1;
  bool bVar2;
  ushort local_c;
  byte local_a;
  
  local_a = _refptr_g_tsaPrmtFlash->bCols - 1;
  uVar1 = _refptr_g_tsaPrmtFlash->bRows;
  bVar2 = ((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) != 0;
  if (bVar2) {
    local_a = _refptr_g_tsaPrmtFlash->bCols - 2;
  }
  for (local_c = 0; local_c < (*_refptr_g_prevTouchesPtr)->bPrevTouchCount; local_c = local_c + 1) {
    if ((((bVar2 < (*_refptr_g_peaksPtr)
                   [(longlong)
                    (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                        (longlong)(int)(uint)local_c * 0x478 + 0x23c) * 0x14 + 0x15]
                   .bPeakCount) &&
         ((*_refptr_g_peaksPtr)
          [(longlong)
           (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                               (longlong)(int)(uint)local_c * 0x478 + 0x23c) * 0x14 + 0x15].
          bPeakCount < local_a)) &&
        ((*_refptr_g_peaksPtr)
         [(longlong)
          (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                              (longlong)(int)(uint)local_c * 0x478 + 0x23c) * 0x14 + 0x16].
         bPeakCount != 0)) &&
       ((*_refptr_g_peaksPtr)
        [(longlong)
         (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)local_c * 0x478 + 0x23c) * 0x14 + 0x16].bPeakCount
        < (byte)(uVar1 - 1))) {
      if ((((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)local_c * 0x478 + 0x1f8) & 0x20000000) == 0) ||
          (0xc < *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                           (longlong)(int)(uint)local_c * 0x478 + 0x1a0))) ||
         (*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)local_c * 0x478 + 0x1be) * 2 <=
          -(int)_refptr_g_tsaPrpt->nSPeakNegative)) {
        if ((*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)local_c * 0x478 + 0x4ec) < 0xf) &&
           (*(char *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)local_c * 0x478 + 0x4ec) != '\0')) {
          *(char *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)local_c * 0x478 + 0x4ec) =
               *(char *)((longlong)*_refptr_g_prevTouchesPtr +
                        (longlong)(int)(uint)local_c * 0x478 + 0x4ec) + -1;
        }
      }
      else if (*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                        (longlong)(int)(uint)local_c * 0x478 + 0x4ec) < 0xf) {
        *(char *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_c * 0x478 + 0x4ec
                 ) = *(char *)((longlong)*_refptr_g_prevTouchesPtr +
                              (longlong)(int)(uint)local_c * 0x478 + 0x4ec) + '\x01';
      }
    }
    else if ((*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)local_c * 0x478 + 0x4ec) < 0xf) &&
            (*(char *)((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)local_c * 0x478 + 0x4ec) != '\0')) {
      *(char *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_c * 0x478 + 0x4ec)
           = *(char *)((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)local_c * 0x478 + 0x4ec) + -1;
    }
  }
  return;
}


// ============================================================================
// Function: BLReset_IsCurBaselineDirty @ 6ba4aaee
// ============================================================================

bool BLReset_IsCurBaselineDirty(void)

{
  TSAStatic *pTVar1;
  bool bVar2;
  char cVar3;
  undefined8 uVar4;
  char local_1b;
  
  bVar2 = true;
  if (((_refptr_g_tsaPrmtFlash->field_0x6a != '\0') &&
      (cVar3 = BLReset_CheckNumOverNoise(), cVar3 != '\0')) &&
     (cVar3 = TSA_IsGameScenario(), cVar3 == '\0')) {
    local_1b = '\0';
    if ((void *)(*(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x80 + 310000) <=
        (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr) {
      local_1b = BLReset_IsTouchToBeProtected();
    }
    if ((((*(uint *)&(*_refptr_g_tsaStaticPtr)->field_0xd8 & 0x100000) == 0) && (local_1b == '\0'))
       && ((cVar3 = BLReset_IsCurBaselineDirtyByDiff(), cVar3 != '\0' &&
           (cVar3 = BLReset_CheckCleanBaseline(), cVar3 != '\0')))) {
      bVar2 = false;
    }
    if (bVar2) {
      *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x388 = 0;
    }
    else {
      *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 =
           *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 | 0x2000;
      pTVar1 = *_refptr_g_tsaStaticPtr;
      uVar4 = BLReset_UpdateTimer(*(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x388);
      *(undefined8 *)&pTVar1->field_0x388 = uVar4;
    }
    bVar2 = 300 < *(ulonglong *)&(*_refptr_g_tsaStaticPtr)->field_0x388;
    if (bVar2) {
      *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x388 = 0;
    }
    return bVar2;
  }
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x388 = 0;
  return false;
}


// ============================================================================
// Function: BLReset_IsBaselineAbnormal @ 6ba4b506
// ============================================================================

undefined1 BLReset_IsBaselineAbnormal(void)

{
  byte bVar1;
  char cVar2;
  undefined1 local_14;
  undefined1 local_13;
  char local_12;
  char local_11;
  undefined1 local_10;
  char local_f;
  char local_e;
  char local_d;
  uint local_c;
  
  local_14 = 0;
  bVar1 = TSA_IsGameScenario();
  local_c = (uint)bVar1;
  local_d = BLReset_IsAfeSideTouchToBeProtected();
  local_e = '\0';
  local_f = BLReset_CheckNumOverNoise();
  local_10 = BLReset_GetTouchToBeProtected2State();
  local_11 = ToeSynaBl_IsSideTouchToBeProtected();
  BLReset_IsBaselineAbnormalType1(local_f,local_10);
  BLReset_IsBaselineAbnormalType2(local_c,local_f,local_10);
  BLReset_IsBaselineAbnormalType3(local_c,local_f,local_10);
  cVar2 = ToeSynaBl_IsBaselineAbnormalSSProtectedByDim1();
  if (cVar2 != '\0') {
    *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x370 = 0;
  }
  if (local_11 != '\0') {
    *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x370 = 0;
    *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x380 = 0;
    *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3b8 = 0;
  }
  if ((local_d != '\0') || (local_e != '\0')) {
    *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x370 = 0;
    *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x378 = 0;
    *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x380 = 0;
  }
  if (((300 < *(ulonglong *)&(*_refptr_g_tsaStaticPtr)->field_0x370) ||
      (300 < *(ulonglong *)&(*_refptr_g_tsaStaticPtr)->field_0x378)) ||
     (300 < *(ulonglong *)&(*_refptr_g_tsaStaticPtr)->field_0x380)) {
    *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x370 = 0;
    *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x378 = 0;
    *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x380 = 0;
    local_14 = 1;
  }
  local_12 = BLReset_GetBaselineCleanState();
  if (((local_12 == '\0') && (local_11 == '\0')) &&
     ((local_d == '\0' && ((local_e == '\0' && (local_f != '\0')))))) {
    local_13 = 0;
  }
  else {
    local_13 = 1;
  }
  ToeSynaBl_IsBaselineAbnormal(local_13,local_10,&local_14);
  return local_14;
}


// ============================================================================
// Function: BLReset_IsAfeRawJump @ 6ba4b70a
// ============================================================================

bool BLReset_IsAfeRawJump(void)

{
  bool bVar1;
  char cVar2;
  
  cVar2 = TSA_IsGameScenario();
  if (cVar2 != '\0') {
    return false;
  }
  if (0 < *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x390) {
    *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 =
         *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 | 0x400;
    cVar2 = BLReset_CheckNumOverNoise();
    if ((cVar2 == '\0') && (cVar2 = BLReset_IsTouchToBeProtected2(0), cVar2 != '\0')) {
      *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x390 = 1;
    }
    else {
      *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x390 =
           (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal +
           *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x390;
    }
  }
  if (((*(uint *)&(*_refptr_g_tsaStaticPtr)->field_0xd8 & 0x20000) != 0) &&
     (*(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x390 == 0)) {
    *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x390 = 1;
  }
  if (((*(uint *)&(*_refptr_g_tsaStaticPtr)->field_0xd8 & 0x20000) == 0) &&
     (*(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x390 == -1)) {
    *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x390 = 0;
  }
  bVar1 = 0 < *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x390;
  if (bVar1) {
    *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x390 = 0xffffffffffffffff;
  }
  return bVar1;
}


// ============================================================================
// Function: BLReset_IsTriggered2BySS @ 6ba4b890
// ============================================================================

bool BLReset_IsTriggered2BySS(void)

{
  char cVar1;
  bool local_9;
  
  cVar1 = BLReset_IsLongTimeSSNoSignal();
  local_9 = cVar1 != '\0';
  if (local_9) {
    _refptr_g_tsaPrpt->dwBaselinePropertyState = 10;
    (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
         (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 1;
  }
  cVar1 = BLReset_IsBaselineClean();
  if (cVar1 == '\0') {
    cVar1 = BLReset_IsMutualSSNotMatch();
    if (cVar1 != '\0') {
      local_9 = true;
      _refptr_g_tsaPrpt->dwBaselinePropertyState = 0xe;
      (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
           (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 1;
    }
  }
  cVar1 = BLReset_IsBaselineClean();
  if (cVar1 == '\0') {
    cVar1 = BLReset_IsLongTimeSSNegativeSignal();
    if (cVar1 != '\0') {
      local_9 = true;
      _refptr_g_tsaPrpt->dwBaselinePropertyState = 0xb;
      (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
           (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 1;
    }
  }
  if (_refptr_g_tsaPrmtFlash->field_0x6c != '\0') {
    cVar1 = BLReset_IsBaselineClean();
    if (cVar1 != '\0') {
      cVar1 = BLReset_IsLongTimeSSNegativeSignalStable();
      if (cVar1 != '\0') {
        local_9 = true;
        _refptr_g_tsaPrpt->dwBaselinePropertyState = 0x15;
        (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
             (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 1;
      }
    }
  }
  return local_9;
}


// ============================================================================
// Function: BLReset_IsTriggered2AfeWakeUp @ 6ba4b9d4
// ============================================================================

undefined1 BLReset_IsTriggered2AfeWakeUp(void)

{
  char cVar1;
  undefined1 local_9;
  
  local_9 = 0;
  cVar1 = BLReset_IsAllFreqsBaselineInitializedDuringTouchLiftOff();
  if (cVar1 == '\0') {
    if (((_refptr_g_tsaPrmtFlash->field_0x6b == '\0') && (cVar1 = BLReset_Check(), cVar1 != '\0'))
       || (cVar1 = BLReset_IsAbnormalTouchDetected(), cVar1 != '\0')) {
      (*_refptr_g_tsaStaticPtr)->bBaselineResetInDebounce =
           (*_refptr_g_tsaStaticPtr)->bBaselineResetInDebounce + '\x01';
      *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x2f0 =
           (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal +
           *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x2f0;
      if (0xa0 < *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x2f0) {
        local_9 = 1;
        (*_refptr_g_tsaStaticPtr)->field_0x3ea = 0;
        (*_refptr_g_tsaStaticPtr)->bBaselineResetInDebounce = '\0';
        *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x2f0 = 0;
        _refptr_g_tsaPrpt->dwBaselinePropertyState = 5;
        (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
             (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 1;
        if ((*_refptr_g_prevTouchesPtr)->nTouchCount == 0x2000) {
          (*_refptr_g_tsaStaticPtr)[1].field_0x3 = 1;
          _refptr_g_tsaPrpt->dwBaselinePropertyState = 8;
        }
        if ((*(uint *)(*_refptr_g_prevTouchesPtr + 1) & 0x10) != 0) {
          _refptr_g_tsaPrpt->dwBaselinePropertyState = 9;
        }
      }
    }
    else if ((*_refptr_g_tsaStaticPtr)->field_0x2f8 == '\0') {
      if ((*_refptr_g_tsaStaticPtr)->bBaselineResetInDebounce < 2) {
        (*_refptr_g_tsaStaticPtr)->bBaselineResetInDebounce = '\0';
        *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x2f0 = 0;
      }
      else {
        (*_refptr_g_tsaStaticPtr)->bBaselineResetInDebounce =
             (*_refptr_g_tsaStaticPtr)->bBaselineResetInDebounce + 0xff;
        *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x2f0 =
             *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x2f0 -
             (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
      }
    }
    else {
      *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x2f0 = 0;
      (*_refptr_g_tsaStaticPtr)->bBaselineResetInDebounce = '\0';
    }
  }
  else {
    local_9 = 1;
    (*_refptr_g_tsaStaticPtr)->field_0x3ea = 0;
    _refptr_g_tsaPrpt->dwBaselinePropertyState = 4;
  }
  return local_9;
}


// ============================================================================
// Function: BLReset_IsAllFreqsBaselineInitializedDuringTouchLiftOff @ 6ba48b01
// ============================================================================

undefined1 BLReset_IsAllFreqsBaselineInitializedDuringTouchLiftOff(void)

{
  char cVar1;
  undefined1 local_9;
  
                    /* 0x8b01  360  BLReset_IsAllFreqsBaselineInitializedDuringTouchLiftOff */
  local_9 = 0;
  cVar1 = AFE_IsIndividualBaselineForEachFreq();
  if (cVar1 == '\0') {
    local_9 = 0;
  }
  else if (((((*_refptr_g_tsaStaticPtr)->field_0x117 != '\0') &&
            (cVar1 = AFE_IsAlwaysNoisy(), cVar1 == '\0')) &&
           ((int)(*(short *)(_refptr_g_tsaPrmtDynamic + 0xc) / 2) <
            (int)((uint)_refptr_g_tsaPrpt->wBoundaryPositive -
                 *(int *)&_refptr_g_tsaPrmtFlash->field_0x5fc))) &&
          ((byte)_refptr_g_tsaPrpt->field_0x29c < 3)) {
    local_9 = 1;
    _refptr_g_tsaPrpt->field_0x29c = _refptr_g_tsaPrpt->field_0x29c + '\x01';
  }
  return local_9;
}


// ============================================================================
// Function: BLReset_CheckAbnormalPeak @ 6ba482f0
// ============================================================================

void BLReset_CheckAbnormalPeak(byte param_1)

{
  BLReset_CheckAbnormalSensor
            ((*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_1 * 0x14 + 0x15].bPeakCount,
             (*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_1 * 0x14 + 0x16].bPeakCount);
  return;
}


// ============================================================================
// Function: BLReset_IsTouchProtectOnRawJump @ 6ba48bb6
// ============================================================================

undefined8 BLReset_IsTouchProtectOnRawJump(void)

{
  bool bVar1;
  bool bVar2;
  int iVar3;
  undefined8 uVar4;
  uint uVar5;
  int local_10;
  byte local_9;
  
  local_10 = 0;
  iVar3 = (int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold;
  bVar1 = false;
  bVar2 = false;
  for (local_9 = 0; local_9 < (*_refptr_g_prevTouchesPtr)->bPrevTouchCount; local_9 = local_9 + 1) {
    if ((((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)local_9 * 0x478 + 0x1f8) & 8) != 0) &&
        (0x100 < *(ushort *)
                  ((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)local_9 * 0x478 + 0x22c))) &&
       (0x100 < *(ushort *)
                 ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_9 * 0x478 + 0x22e
                 ))) {
      if (iVar3 / 2 <
          (int)*(short *)((longlong)
                          (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                              (longlong)(int)(uint)local_9 * 0x478 + 0x23c) * 0xc +
                          *(longlong *)_refptr_g_prevPeaksPtr + 0xc)) {
        bVar1 = true;
      }
      if ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)local_9 * 0x478 + 0x1f0) & 0x20000) != 0) {
        bVar2 = true;
      }
      local_10 = local_10 + 1;
    }
  }
  if ((((local_10 < 1) || (3 < local_10)) ||
      ((!bVar1 ||
       ((bVar2 || (uVar5 = (int)*(short *)&_refptr_g_tsaPrpt[1].field_0x44 >> 0x1f,
                  iVar3 <= (int)((((int)*(short *)&_refptr_g_tsaPrpt[1].field_0x44 ^ uVar5) - uVar5)
                                * 3))))))) ||
     (uVar5 = (int)*(short *)&_refptr_g_tsaPrpt[1].field_0x46 >> 0x1f,
     iVar3 <= (int)((((int)*(short *)&_refptr_g_tsaPrpt[1].field_0x46 ^ uVar5) - uVar5) * 3))) {
    uVar4 = 0;
  }
  else {
    uVar4 = 1;
  }
  return uVar4;
}


// ============================================================================
// Function: BLReset_BaselineStateMachine @ 6ba49f88
// ============================================================================

void BLReset_BaselineStateMachine(void)

{
  if (*(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x3d8 < 9) {
                    /* WARNING: Could not recover jumptable at 0x6ba49fcf. Too many branches */
                    /* WARNING: Treating indirect jump as call */
    (*(code *)((longlong)&switchD_6ba49fcf::switchdataD_6bb519e0 +
              (longlong)
              (int)(&switchD_6ba49fcf::switchdataD_6bb519e0)
                   [*(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x3d8]))();
    return;
  }
  return;
}


// ============================================================================
// Function: BLReset_IsTouchToBeProtected @ 6ba47b08
// ============================================================================

char BLReset_IsTouchToBeProtected(void)

{
  int iVar1;
  char local_9;
  
                    /* 0x7b08  367  BLReset_IsTouchToBeProtected */
  local_9 = '\0';
  if ((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) != 0) &&
     (iVar1 = SmartCover_GetPrevTouchNumInOpenWindow(), 0 < iVar1)) {
    local_9 = '\x01';
  }
  if (((*_refptr_g_prevTouchesPtr)->bPrevTouchCount == '\x01') &&
     ((*(uint *)&(*_refptr_g_prevTouchesPtr)[0xd].field_0x1c & 0x20000) == 0)) {
    if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3) {
      local_9 = '\x01';
    }
    else if ((((int)*(short *)&_refptr_g_tsaPrpt->field_0x16c <
               *(int *)(_refptr_g_tsaBufSelf +
                       (longlong)(int)(uint)(byte)_refptr_g_tsaPrpt->field_0x170 * 4)) ||
             ((int)*(short *)&_refptr_g_tsaPrpt->field_0x16c <
              *(int *)(_refptr_g_tsaBufSelf +
                      (longlong)
                      (int)((uint)(byte)_refptr_g_tsaPrpt->field_0x171 +
                           (uint)_refptr_g_tsaPrmtFlash->bCols) * 4))) &&
            (((short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax * 3) / 2 <
             (int)*(short *)&_refptr_g_tsaPrpt->field_0x16c)) {
      local_9 = '\x01';
    }
  }
  else if (((*_refptr_g_prevTouchesPtr)->bPrevTouchCount == '\x02') &&
          (((*(uint *)&(*_refptr_g_prevTouchesPtr)[0xd].field_0x1c & 0x20000) == 0 ||
           ((*(uint *)&(*_refptr_g_prevTouchesPtr)[0x2d].field_0x14 & 0x20000) == 0)))) {
    if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3) {
      local_9 = '\x01';
    }
    else if ((((int)(_refptr_g_tsaPrpt->nSPeakPositive / 2) <
               -(int)*(short *)((ulonglong)
                                ((uint)(byte)((ushort)*(undefined2 *)
                                                       &(*_refptr_g_prevTouchesPtr)[10].field_0x8 >>
                                             8) +
                                 (uint)(byte)((ushort)*(undefined2 *)
                                                       &(*_refptr_g_prevTouchesPtr)[0x2a].field_0x2
                                             >> 8) * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) *
                                2 + *(longlong *)_refptr_g_tsaBufDif)) &&
             ((int)(_refptr_g_tsaPrpt->nSPeakPositive / 2) <
              -(int)*(short *)((ulonglong)
                               ((uint)(byte)((ushort)*(undefined2 *)
                                                      (*_refptr_g_prevTouchesPtr + 0x2a) >> 8) +
                                (uint)(byte)((ushort)*(undefined2 *)
                                                      &(*_refptr_g_prevTouchesPtr)[10].field_0xa >>
                                            8) * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
                              *(longlong *)_refptr_g_tsaBufDif))) &&
            (-*(int *)&_refptr_g_tsaPrpt->field_0xb0 < *(int *)&_refptr_g_tsaPrpt->field_0xac)) {
      local_9 = '\x01';
    }
  }
  if (local_9 != '\0') {
    *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 =
         *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 | 8;
  }
  return local_9;
}


// ============================================================================
// Function: BLReset_UpdateTimer @ 6ba47e53
// ============================================================================

longlong BLReset_UpdateTimer(longlong param_1)

{
  longlong local_10;
  
                    /* 0x7e53  371  BLReset_UpdateTimer */
  if (param_1 == 0) {
    local_10 = 1;
  }
  else {
    local_10 = param_1 + (*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
  }
  return local_10;
}


// ============================================================================
// Function: BLReset_CheckCleanBaseline @ 6ba4a9a3
// ============================================================================

undefined8 BLReset_CheckCleanBaseline(void)

{
  short sVar1;
  undefined8 uVar2;
  ushort local_10;
  ushort local_e;
  ushort local_c;
  ushort local_a;
  
  if ((*_refptr_g_tsaStaticPtr)->field_0x3ea == '\0') {
    uVar2 = 0;
  }
  else {
    local_a = 0;
    local_c = 0;
    for (local_e = 0; local_e < _refptr_g_tsaPrmtFlash->bRows; local_e = local_e + 1) {
      for (local_10 = 0; local_10 < _refptr_g_tsaPrmtFlash->bCols; local_10 = local_10 + 1) {
        sVar1 = *(short *)(*(longlong *)_refptr_g_tsaBufPreCMFRaw + (ulonglong)local_a * 2) -
                *(short *)(*(longlong *)_refptr_g_tsaBufCleanBaseline + (ulonglong)local_a * 2);
        if ((100 < sVar1) || (sVar1 < -100)) {
          local_c = local_c + 1;
        }
        local_a = local_a + 1;
      }
    }
    if (local_c < 4) {
      uVar2 = 1;
    }
    else {
      uVar2 = 0;
    }
  }
  return uVar2;
}


// ============================================================================
// Function: BLReset_IsCurBaselineDirtyByDiff @ 6ba4aa9e
// ============================================================================

undefined1 BLReset_IsCurBaselineDirtyByDiff(void)

{
  undefined1 local_9;
  
  local_9 = 0;
  if ((_refptr_g_tsaPrmtFlash->wBufElementCount >> 2 < *(ushort *)&_refptr_g_tsaPrpt->field_0x186)
     && (4 < *(ushort *)&_refptr_g_tsaPrpt->field_0x182)) {
    local_9 = 1;
  }
  return local_9;
}


// ============================================================================
// Function: BLReset_IsAfeSideTouchToBeProtected @ 6ba49ee3
// ============================================================================

undefined8 BLReset_IsAfeSideTouchToBeProtected(void)

{
  undefined8 uVar1;
  
  if (((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) == 0) ||
      (4 < (ushort)(*(short *)(_refptr_g_tsaDifBlockPrpt + 0xbc) +
                   *(short *)(_refptr_g_tsaDifBlockPrpt + 0x14) +
                   *(short *)(_refptr_g_tsaDifBlockPrpt + 0xa0)))) ||
     ((*(uint *)&(*_refptr_g_tsaStaticPtr)->field_0xd8 & 0x80000) == 0)) {
    uVar1 = 0;
  }
  else {
    *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 =
         *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 | 0x1000;
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: BLReset_GetBaselineCleanState @ 6ba4af53
// ============================================================================

char BLReset_GetBaselineCleanState(void)

{
  char cVar1;
  
  if ((*_refptr_g_tsaStaticPtr)->pFrameBufferPtr <
      (void *)(*(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x80 + 310000)) {
    cVar1 = BLReset_IsBaselineClean();
    if (cVar1 == '\0') {
      cVar1 = '\0';
      goto LAB_6ba4af9d;
    }
  }
  cVar1 = '\x01';
LAB_6ba4af9d:
  if (cVar1 != '\0') {
    *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 =
         *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 | 4;
  }
  return cVar1;
}


// ============================================================================
// Function: BLReset_GetTouchToBeProtected2State @ 6ba4afd3
// ============================================================================

undefined1 BLReset_GetTouchToBeProtected2State(void)

{
  char cVar1;
  undefined1 uVar2;
  char local_9;
  
  cVar1 = BLReset_IsTouchToBeProtected2(0);
  if ((cVar1 == '\0') && ((*_refptr_g_tsaStaticPtr)->bField_0x111 != '\x03')) {
    local_9 = '\0';
  }
  else {
    local_9 = '\x01';
  }
  if ((void *)(*(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x80 + 310000) <=
      (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr) {
    if (((local_9 == '\0') && (cVar1 = BLReset_IsTouchToBeProtected(), cVar1 == '\0')) &&
       (cVar1 = BLReset_IsToBeProtectedBySS(), cVar1 == '\0')) {
      local_9 = '\0';
    }
    else {
      local_9 = '\x01';
    }
  }
  uVar2 = ToeSynaBl_IsTouchToBeProtected2(0,local_9);
  return uVar2;
}


// ============================================================================
// Function: BLReset_IsBaselineAbnormalType1 @ 6ba4b083
// ============================================================================

void BLReset_IsBaselineAbnormalType1(char param_1,char param_2)

{
  TSAStatic *pTVar1;
  bool bVar2;
  char cVar3;
  undefined8 uVar4;
  
  bVar2 = false;
  if (param_1 != '\0') {
    cVar3 = BLReset_IsBaselineClean();
    if ((((cVar3 == '\0') &&
         ((((*(short *)&_refptr_g_tsaPrpt->field_0x16c * 3 <
             *(short *)&_refptr_g_tsaPrpt->field_0x16e * -4 ||
            ((uint)*(ushort *)&_refptr_g_tsaPrpt->field_0x180 * 2 +
             (uint)*(ushort *)&_refptr_g_tsaPrpt->field_0x180 <
             (uint)*(ushort *)&_refptr_g_tsaPrpt->field_0x182 * 5)) &&
           (*(int *)&_refptr_g_tsaPrpt->field_0x178 * 3 <
            *(int *)&_refptr_g_tsaPrpt->field_0x17c * -5)) &&
          ((*(short *)&_refptr_g_tsaPrpt->field_0x16c * 2 <
            *(short *)&_refptr_g_tsaPrpt->field_0x16e * -4 &&
           ((int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x46) <
            -(int)*(short *)&_refptr_g_tsaPrpt->field_0x16e)))))) &&
        (2 < *(ushort *)&_refptr_g_tsaPrpt->field_0x182)) && (param_2 == '\0')) {
      if ((void *)(*(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x80 + 310000) <=
          (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr) {
        cVar3 = TSA_IsGameScenario();
        if ((cVar3 != '\0') ||
           (*(ushort *)&_refptr_g_tsaPrpt->field_0x184 <=
            _refptr_g_tsaPrmtFlash->wBufElementCount >> 2)) goto LAB_6ba4b224;
      }
      bVar2 = true;
    }
  }
LAB_6ba4b224:
  if (bVar2) {
    *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 =
         *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 | 0x10;
    pTVar1 = *_refptr_g_tsaStaticPtr;
    uVar4 = BLReset_UpdateTimer(*(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x370);
    *(undefined8 *)&pTVar1->field_0x370 = uVar4;
  }
  else {
    *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x370 = 0;
  }
  return;
}


// ============================================================================
// Function: BLReset_IsBaselineAbnormalType2 @ 6ba4b295
// ============================================================================

void BLReset_IsBaselineAbnormalType2(int param_1,char param_2,char param_3)

{
  TSAStatic *pTVar1;
  bool bVar2;
  char cVar3;
  undefined8 uVar4;
  
  bVar2 = false;
  if ((param_2 != '\0') && (param_1 == 0)) {
    cVar3 = BLReset_IsBaselineClean();
    if ((cVar3 == '\0') && (*(ushort *)&_refptr_g_tsaPrpt->field_0x8 < 5)) {
      cVar3 = BLReset_IsFullPanelOverNoise();
      if ((cVar3 != '\0') && (param_3 == '\0')) {
        bVar2 = true;
      }
    }
  }
  if (bVar2) {
    *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 =
         *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 | 0x20;
    pTVar1 = *_refptr_g_tsaStaticPtr;
    uVar4 = BLReset_UpdateTimer(*(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x378);
    *(undefined8 *)&pTVar1->field_0x378 = uVar4;
  }
  else {
    *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x378 = 0;
  }
  return;
}


// ============================================================================
// Function: BLReset_IsBaselineAbnormalType3 @ 6ba4b35f
// ============================================================================

void BLReset_IsBaselineAbnormalType3(int param_1,char param_2,char param_3)

{
  TSAStatic *pTVar1;
  bool bVar2;
  char cVar3;
  undefined8 uVar4;
  
  bVar2 = false;
  if ((((((param_2 != '\0') && (param_1 == 0)) &&
        ((int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x46) <
         -(int)*(short *)&_refptr_g_tsaPrpt->field_0x18e)) &&
       ((2 < *(ushort *)&_refptr_g_tsaPrpt->field_0x1a2 &&
        (*(ushort *)&_refptr_g_tsaPrpt->field_0x8 < 3)))) &&
      ((*(short *)&_refptr_g_tsaPrpt->field_0x16c * 2 <
        *(short *)&_refptr_g_tsaPrpt->field_0x16e * -3 &&
       ((cVar3 = BLReset_IsTouchToBeProtected(), cVar3 == '\0' && (param_3 == '\0')))))) &&
     (((uint)*(ushort *)&_refptr_g_tsaPrpt->field_0x180 * 4 <
       (uint)*(ushort *)&_refptr_g_tsaPrpt->field_0x182 ||
      (*(int *)&_refptr_g_tsaPrpt->field_0x178 * 4 < -*(int *)&_refptr_g_tsaPrpt->field_0x17c)))) {
    bVar2 = true;
  }
  if ((bVar2) && (cVar3 = ToeSynaBl_IsWaterDetected(), cVar3 == '\0')) {
    *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 =
         *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 | 0x100;
    pTVar1 = *_refptr_g_tsaStaticPtr;
    uVar4 = BLReset_UpdateTimer(*(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x380);
    *(undefined8 *)&pTVar1->field_0x380 = uVar4;
    return;
  }
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x380 = 0;
  *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3b8 = 0;
  return;
}


// ============================================================================
// Function: BLReset_IsBaselineClean @ 6ba47e09
// ============================================================================

bool BLReset_IsBaselineClean(void)

{
  bool bVar1;
  
  bVar1 = *(int *)&(*_refptr_g_tsaStaticPtr)->field_0x3d8 == 0x10;
  if (bVar1) {
    *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 =
         *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 | 2;
  }
  return bVar1;
}


// ============================================================================
// Function: BLReset_IsLongTimeSSNoSignal @ 6ba49544
// ============================================================================

bool BLReset_IsLongTimeSSNoSignal(void)

{
  char cVar1;
  int local_10;
  bool local_9;
  
  local_9 = false;
  cVar1 = SS_IsValidForBaseline();
  if (cVar1 == '\0') {
    (*_refptr_g_tsaStaticPtr)->field_0x328 = 0;
    local_9 = false;
  }
  else {
    cVar1 = SS_IsNoSignal();
    if ((((cVar1 == '\0') || (cVar1 = BLReset_CheckNumOverNoise(), cVar1 == '\0')) ||
        (_refptr_g_tsaPrpt->nSPeakPositive <=
         (short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax / 2)) ||
       (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) != 0)) {
      *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x330 = 0;
      keepFrameNumThesh_11177 = '\0';
    }
    else {
      if ((keepFrameNumThesh_11177 == '\x02') ||
         (*(short *)&_refptr_g_tsaPrpt->field_0xc0 <=
          (short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax / 2)) {
        local_10 = 1000;
        keepFrameNumThesh_11177 = '\x02';
      }
      else {
        cVar1 = TSA_IsGameScenario();
        if (cVar1 == '\0') {
          local_10 = 0x14d;
        }
        else {
          local_10 = 1000;
        }
        keepFrameNumThesh_11177 = '\x01';
      }
      local_9 = *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x330 <= (longlong)local_10;
      if (local_9) {
        *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x330 =
             (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal +
             *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x330;
      }
      else {
        *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x330 = 0;
      }
      local_9 = !local_9;
      if (*(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x330 ==
          (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal) {
        BLReset_IsRawChangedSinceSSNoSignal(1);
      }
      else if (((longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal <
                *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x330) &&
              (cVar1 = BLReset_IsRawChangedSinceSSNoSignal(0), cVar1 != '\0')) {
        *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x330 = 0;
      }
    }
    (*_refptr_g_tsaStaticPtr)->field_0x328 = local_9;
  }
  return local_9;
}


// ============================================================================
// Function: BLReset_IsMutualSSNotMatch @ 6ba49950
// ============================================================================

undefined8 BLReset_IsMutualSSNotMatch(void)

{
  return 0;
}


// ============================================================================
// Function: BLReset_IsLongTimeSSNegativeSignal @ 6ba4995b
// ============================================================================

undefined1 BLReset_IsLongTimeSSNegativeSignal(void)

{
  char cVar1;
  undefined1 local_9;
  
  local_9 = 0;
  cVar1 = TSA_InStylusMode();
  if (cVar1 == '\0') {
    cVar1 = BLReset_CheckNumOverNoise();
    if (cVar1 == '\0') {
      *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x340 = 0;
      (*_refptr_g_tsaStaticPtr)->field_0x338 = 0;
      local_9 = 0;
    }
    else {
      cVar1 = SS_IsValidForBaseline();
      if (cVar1 != '\0') {
        if (((((*(int *)&_refptr_g_tsaPrpt[1].field_0x68 < 10) ||
              (3 < *(int *)&_refptr_g_tsaPrpt[1].field_0x64)) ||
             (-*(int *)&_refptr_g_tsaPrpt[1].field_0x60 <=
              *(int *)&_refptr_g_tsaPrpt[1].field_0x5c * 4)) ||
            (-*(int *)&_refptr_g_tsaPrpt[1].field_0x60 <=
             (short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold * 2)) &&
           (((*(int *)&_refptr_g_tsaPrpt[1].field_0x68 < 10 ||
             (-*(int *)&_refptr_g_tsaPrpt[1].field_0x60 <=
              *(int *)&_refptr_g_tsaPrpt[1].field_0x5c * 5)) ||
            (-*(int *)&_refptr_g_tsaPrpt[1].field_0x60 <=
             (short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold * 4)))) {
          *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x340 = 0;
        }
        else if (*(ulonglong *)&(*_refptr_g_tsaStaticPtr)->field_0x340 < 0xa8) {
          *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x340 =
               (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal +
               *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x340;
        }
        else {
          local_9 = 1;
          *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x340 = 0;
        }
      }
      (*_refptr_g_tsaStaticPtr)->field_0x338 = local_9;
    }
  }
  else {
    *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x340 = 0;
    (*_refptr_g_tsaStaticPtr)->field_0x338 = 0;
    local_9 = 0;
  }
  return local_9;
}


// ============================================================================
// Function: BLReset_IsLongTimeSSNegativeSignalStable @ 6ba49b66
// ============================================================================

undefined1 BLReset_IsLongTimeSSNegativeSignalStable(void)

{
  char cVar1;
  undefined1 local_9;
  
  local_9 = 0;
  if ((((*_refptr_g_tsaStaticPtr)->pFrameBufferPtr <
        (void *)(*(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x80 + 60000)) &&
      (cVar1 = TSA_IsGameScenario(), cVar1 == '\0')) &&
     (cVar1 = BLReset_CheckNumOverNoise(), cVar1 != '\0')) {
    cVar1 = SS_IsValidForBaseline();
    if (cVar1 != '\0') {
      if ((((*(int *)&_refptr_g_tsaPrpt[1].field_0x68 < 10) ||
           (3 < *(int *)&_refptr_g_tsaPrpt[1].field_0x64)) ||
          ((-*(int *)&_refptr_g_tsaPrpt[1].field_0x60 <=
            *(int *)&_refptr_g_tsaPrpt[1].field_0x5c * 4 ||
           (-*(int *)&_refptr_g_tsaPrpt[1].field_0x60 <=
            (short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold * 2)))) &&
         (((*(int *)&_refptr_g_tsaPrpt[1].field_0x68 < 10 ||
           (-*(int *)&_refptr_g_tsaPrpt[1].field_0x60 <=
            *(int *)&_refptr_g_tsaPrpt[1].field_0x5c * 5)) ||
          (-*(int *)&_refptr_g_tsaPrpt[1].field_0x60 <=
           (short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold * 4)))) {
        *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x350 = 0;
      }
      else if (*(ulonglong *)&(*_refptr_g_tsaStaticPtr)->field_0x350 < 0x3e9) {
        *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x350 =
             (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal +
             *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x350;
      }
      else {
        local_9 = 1;
        *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x350 = 0;
      }
    }
    (*_refptr_g_tsaStaticPtr)->field_0x348 = local_9;
    return local_9;
  }
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x350 = 0;
  (*_refptr_g_tsaStaticPtr)->field_0x348 = 0;
  return 0;
}


// ============================================================================
// Function: BLReset_IsAbnormalTouchDetected @ 6ba486da
// ============================================================================

undefined8 BLReset_IsAbnormalTouchDetected(void)

{
  char cVar1;
  undefined8 uVar2;
  
  cVar1 = BLReset_CheckNumOverNoise();
  if (cVar1 == '\0') {
    return 0;
  }
  if ((*_refptr_g_prevTouchesPtr)->field_0x10 != '\0') {
    cVar1 = TSA_IsPadFeatureEnabled();
    if (cVar1 == '\0') {
      if (_refptr_g_tsaPrmtFlash->wBufElementCount >> 1 < *(ushort *)&_refptr_g_tsaPrpt->field_0x186
         ) {
        return 1;
      }
    }
    else if (((uint)_refptr_g_tsaPrmtFlash->wBufElementCount +
             (uint)_refptr_g_tsaPrmtFlash->wBufElementCount) / 3 <
             (uint)*(ushort *)&_refptr_g_tsaPrpt->field_0x186) {
      return 1;
    }
  }
  if ((*_refptr_g_tsaStaticPtr)->field_0xa9 == '\0') {
    if ((*_refptr_g_tsaStaticPtr)->field_0xaa == '\0') {
      uVar2 = 0;
    }
    else if ((byte)(*_refptr_g_prevTouchesPtr)->field_0x10 < 3) {
      if ((uint)*(ushort *)&_refptr_g_tsaPrpt->field_0x1a2 +
          (uint)*(ushort *)&_refptr_g_tsaPrpt->field_0x1a2 * 4 <
          (uint)*(ushort *)&_refptr_g_tsaPrpt->field_0x1a0) {
        uVar2 = 0;
      }
      else if (*(short *)&_refptr_g_tsaPrpt->field_0x18e * -4 <
               (int)*(short *)&_refptr_g_tsaPrpt->field_0x18c) {
        uVar2 = 0;
      }
      else if ((*_refptr_g_prevTouchesPtr)->field_0x10 == '\0') {
        if (_refptr_g_tsaPrmtFlash->wBufElementCount >> 2 <
            *(ushort *)&_refptr_g_tsaPrpt->field_0x186) {
          if (1 < (int)((uint)(*_refptr_g_prevTouchesPtr)->bPrevTouchCount -
                       (uint)(byte)(*_refptr_g_prevTouchesPtr)->field_0xd)) {
            return 1;
          }
          if ((((*_refptr_g_prevTouchesPtr)->field_0x3 != '\0') ||
              ((*_refptr_g_prevTouchesPtr)->field_0x6 == '\0')) ||
             ((*_refptr_g_prevTouchesPtr)->field_0x11 != '\0')) {
            return 1;
          }
        }
        if (_refptr_g_tsaPrmtFlash->wBufElementCount >> 1 <
            *(ushort *)&_refptr_g_tsaPrpt->field_0x186) {
          uVar2 = 1;
        }
        else {
          cVar1 = BLReset_CocurrentTouchDetect();
          if (cVar1 == '\0') {
            uVar2 = 0;
          }
          else {
            uVar2 = 1;
          }
        }
      }
      else {
        uVar2 = 1;
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
// Function: BLReset_Check @ 6ba48939
// ============================================================================

undefined1 BLReset_Check(void)

{
  char cVar1;
  undefined1 local_9;
  
  local_9 = 0;
  cVar1 = BLReset_CheckNumOverNoise();
  if (cVar1 == '\0') goto LAB_6ba48ab7;
  if ((((*(short *)&_refptr_g_tsaPrpt->field_0x16c * 3 <
         *(short *)&_refptr_g_tsaPrpt->field_0x16e * -4) ||
       ((uint)*(ushort *)&_refptr_g_tsaPrpt->field_0x180 * 2 +
        (uint)*(ushort *)&_refptr_g_tsaPrpt->field_0x180 <
        (uint)*(ushort *)&_refptr_g_tsaPrpt->field_0x182 * 4)) &&
      (*(int *)&_refptr_g_tsaPrpt->field_0x178 * 3 < *(int *)&_refptr_g_tsaPrpt->field_0x17c * -4))
     && (((*(short *)&_refptr_g_tsaPrpt->field_0x16c * 2 <
           *(short *)&_refptr_g_tsaPrpt->field_0x16e * -4 &&
          ((int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x46) <
           -(int)*(short *)&_refptr_g_tsaPrpt->field_0x16e)) &&
         (2 < *(ushort *)&_refptr_g_tsaPrpt->field_0x182)))) {
    cVar1 = BLReset_IsTouchToBeProtected();
    if (cVar1 == '\0') {
      cVar1 = BLReset_IsToBeProtectedBySS();
      if (cVar1 == '\0') {
        local_9 = 1;
        goto LAB_6ba48a7a;
      }
    }
    local_9 = 0;
  }
LAB_6ba48a7a:
  if ((*(uint *)(*_refptr_g_prevTouchesPtr + 1) & 0x10) != 0) {
    cVar1 = BLReset_IsTouchToBeProtected();
    if (cVar1 == '\0') {
      cVar1 = BLReset_IsToBeProtectedBySS();
      if (cVar1 == '\0') {
        local_9 = 1;
        goto LAB_6ba48aaa;
      }
    }
    local_9 = 0;
  }
LAB_6ba48aaa:
  cVar1 = BLReset_IsAllTouchByAbnormalBaseline();
  if (cVar1 != '\0') {
    local_9 = 1;
  }
LAB_6ba48ab7:
  if ((*_refptr_g_prevTouchesPtr)->nTouchCount == 0x2000) {
    if ((*_refptr_g_tsaStaticPtr)[1].field_0x3 == '\0') {
      local_9 = 1;
    }
  }
  else {
    (*_refptr_g_tsaStaticPtr)[1].field_0x3 = 0;
  }
  return local_9;
}


// ============================================================================
// Function: BLReset_IsAllTouchByAbnormalBaseline @ 6ba48425
// ============================================================================

undefined8 BLReset_IsAllTouchByAbnormalBaseline(void)

{
  undefined8 uVar1;
  uchar local_a;
  byte local_9;
  
                    /* 0x8425  361  BLReset_IsAllTouchByAbnormalBaseline */
  local_a = '\0';
  for (local_9 = 0; local_9 < (*_refptr_g_prevTouchesPtr)->bPrevTouchCount; local_9 = local_9 + 1) {
    if ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)local_9 * 0x478 + 0x1f0) & 0x400000) != 0) {
      local_a = local_a + '\x01';
    }
  }
  if (((*_refptr_g_prevTouchesPtr)->bPrevTouchCount == local_a) && (local_a != '\0')) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: BLReset_CheckAbnormalSensor @ 6ba4825d
// ============================================================================

undefined1 BLReset_CheckAbnormalSensor(byte param_1,byte param_2)

{
  char cVar1;
  ushort uVar2;
  undefined1 local_9;
  
  local_9 = 0;
  cVar1 = AFE_IsRawUnifiedToTargetValue();
  if (cVar1 != '\0') {
    uVar2 = (ushort)param_1 + (ushort)param_2 * (ushort)_refptr_g_tsaPrmtFlash->bCols;
    cVar1 = BLReset_CheckAbnormal
                      (*(undefined2 *)(*(longlong *)_refptr_g_tsaBufRaw + (ulonglong)uVar2 * 2),
                       *(undefined2 *)(*(longlong *)_refptr_g_tsaBufBl + (ulonglong)uVar2 * 2));
    if (cVar1 != '\0') {
      local_9 = 1;
    }
  }
  return local_9;
}


// ============================================================================
// Function: BLReset_IsToBeProtectedBySS @ 6ba484b2
// ============================================================================

undefined8 BLReset_IsToBeProtectedBySS(void)

{
  char cVar1;
  undefined8 uVar2;
  
  cVar1 = SS_IsValidForBaseline();
  if (cVar1 == '\0') {
    uVar2 = 0;
  }
  else if (((byte)_refptr_g_tsaPrpt[1].field_0x79 < (byte)_refptr_g_tsaPrpt[1].field_0x78) ||
          (_refptr_g_tsaPrpt[1].field_0x7a != '\0')) {
    uVar2 = 0;
  }
  else {
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: BLReset_IsFullPanelOverNoise @ 6ba4ac58
// ============================================================================

undefined8 BLReset_IsFullPanelOverNoise(void)

{
  char cVar1;
  bool bVar2;
  ushort local_1a;
  ushort local_18;
  ushort local_16;
  ushort local_14;
  ushort local_12;
  int local_10;
  byte local_a;
  byte local_9;
  
  if ((_refptr_g_tsaPrmtFlash->bCmfEnabled == '\0') &&
     (_refptr_g_tsaPrmtFlash[2].field_0x6d0 == '\0')) {
    return 0;
  }
  local_9 = 0;
  local_a = 0;
  local_10 = 0;
  local_16 = (ushort)_refptr_g_tsaPrmtFlash->bCols;
  bVar2 = ((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) != 0;
  if (bVar2) {
    local_16 = local_16 - 1;
  }
  local_12 = (ushort)bVar2;
  local_14 = (ushort)bVar2;
  for (local_18 = local_14; local_18 < local_16; local_18 = local_18 + 1) {
    if (600 < *(int *)(_refptr_g_tsaBufSelf + ((longlong)(int)(uint)local_12 + 0x3c0) * 4)) {
      local_9 = local_9 + 1;
    }
    if (*(int *)(_refptr_g_tsaBufSelf + ((longlong)(int)(uint)local_12 + 0x280) * 4) + 400 <
        *(int *)(_refptr_g_tsaBufSelf + ((longlong)(int)(uint)local_12 + 0x3c0) * 4)) {
      local_10 = local_10 + 1;
    }
    local_12 = local_12 + 1;
  }
  local_12 = (ushort)_refptr_g_tsaPrmtFlash->bCols;
  for (local_1a = 0; local_1a < _refptr_g_tsaPrmtFlash->bRows; local_1a = local_1a + 1) {
    if (0x96 < *(int *)(_refptr_g_tsaBufSelf + ((longlong)(int)(uint)local_12 + 0x460) * 4)) {
      local_a = local_a + 1;
    }
    local_12 = local_12 + 1;
  }
  if (2 < local_10) {
    if ((*_refptr_g_tsaStaticPtr)->pFrameBufferPtr <
        (void *)(*(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x80 + 310000)) {
      if (((((uint)_refptr_g_tsaPrmtFlash->bCols + (uint)_refptr_g_tsaPrmtFlash->bCols) / 3 <=
            (uint)local_9) && (_refptr_g_tsaPrmtFlash->bRows >> 1 <= local_a)) ||
         (((byte)((ushort)(((ushort)_refptr_g_tsaPrmtFlash->bCols * 0x11 +
                           (ushort)_refptr_g_tsaPrmtFlash->bCols * 2) * 9) >> 9) <= local_9 &&
          (((uint)_refptr_g_tsaPrmtFlash->bRows * 3) / 5 <= (uint)local_a)))) {
        return 1;
      }
    }
    else {
      cVar1 = TSA_IsGameScenario();
      if (((cVar1 == '\0') &&
          (((uint)_refptr_g_tsaPrmtFlash->bCols + (uint)_refptr_g_tsaPrmtFlash->bCols) / 3 <
           (uint)local_9)) && (((uint)_refptr_g_tsaPrmtFlash->bRows * 3) / 5 < (uint)local_a)) {
        return 1;
      }
    }
  }
  return 0;
}


// ============================================================================
// Function: BLReset_IsRawChangedSinceSSNoSignal @ 6ba4950f
// ============================================================================

void BLReset_IsRawChangedSinceSSNoSignal(undefined1 param_1)

{
  BLReset_IsRawChanged(*(undefined8 *)_refptr_g_tsaBufRawStoredSinceSSNoSignal,param_1,0,0);
  return;
}


// ============================================================================
// Function: BLReset_CocurrentTouchDetect @ 6ba4859c
// ============================================================================

undefined8 BLReset_CocurrentTouchDetect(void)

{
  undefined8 uVar1;
  byte local_d;
  byte local_c;
  byte local_b;
  ushort local_a;
  
  local_a = 0xffff;
  for (local_b = 0; local_b < (*_refptr_g_prevTouchesPtr)->bPrevTouchCount; local_b = local_b + 1) {
    if (*(ushort *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_b * 0x478 + 0x156) <
        local_a) {
      local_a = *(ushort *)
                 ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_b * 0x478 + 0x156
                 );
    }
  }
  local_c = 0;
  for (local_d = 0; local_d < (*_refptr_g_prevTouchesPtr)->bPrevTouchCount; local_d = local_d + 1) {
    if (((int)((uint)*(ushort *)
                      ((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)local_d * 0x478 + 0x156) - (uint)local_a) < 0x32) &&
       (200 < *(ushort *)
               ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_d * 0x478 + 0x156))
       ) {
      local_c = local_c + 1;
    }
  }
  if ((local_c < 3) || ((*_refptr_g_prevTouchesPtr)->bPrevTouchCount != local_c)) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: BLReset_CheckAbnormal @ 6ba481e9
// ============================================================================

bool BLReset_CheckAbnormal(ushort param_1,ushort param_2)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  
  uVar4 = (uint)param_2 - (uint)*(ushort *)&_refptr_g_tsaPrpt->field_0x29a;
  uVar1 = (int)uVar4 >> 0x1f;
  uVar2 = (uint)param_1 - (uint)*(ushort *)&_refptr_g_tsaPrpt->field_0x9a;
  uVar3 = (int)uVar2 >> 0x1f;
  return (int)((uVar2 ^ uVar3) - uVar3) < (int)((uVar4 ^ uVar1) - uVar1);
}


// ============================================================================
// Function: BLReset_IsRawChanged @ 6ba490f5
// ============================================================================

undefined8 BLReset_IsRawChanged(void *param_1,char param_2,char param_3,char param_4)

{
  ushort uVar1;
  undefined8 uVar2;
  uint uVar3;
  bool bVar4;
  undefined1 local_179;
  uint local_178 [82];
  short local_2e;
  uint local_2c;
  ushort local_26;
  errno_t local_24;
  void *local_20;
  ushort local_14;
  ushort local_12;
  ushort local_10;
  ushort local_e;
  ushort local_c;
  ushort local_a;
  
                    /* 0x90f5  365  BLReset_IsRawChanged */
  if (param_4 == '\0') {
    local_20 = *(void **)_refptr_g_tsaBufRaw;
  }
  else {
    local_20 = *(void **)_refptr_g_tsaBufPreCMFRaw;
  }
  if (param_2 == '\0') {
    if (0x50 < (byte)_refptr_g_tsaPrmtFlashConst[0x24]) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Alg/Baseline/BaselineReset.c",0x29a,
                        "TSAConstPrmt(sys, rxNum) <= TSA_MAX_DIM");
      HardwareAnalyzer_SetSoftException();
    }
    local_a = 0;
    local_e = (ushort)(byte)_refptr_g_tsaPrmtFlashConst[0x25];
    bVar4 = ((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) != 0;
    if (bVar4) {
      local_e = local_e - 1;
    }
    local_c = (ushort)bVar4;
    for (local_10 = local_c; local_10 < local_e; local_10 = local_10 + 1) {
      for (local_12 = 0; local_12 < (byte)_refptr_g_tsaPrmtFlashConst[0x24]; local_12 = local_12 + 1
          ) {
        if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\x01') {
          local_26 = local_12 + _refptr_g_tsaPrmtFlash->bCols * local_10;
        }
        else {
          local_26 = local_10 + _refptr_g_tsaPrmtFlash->bCols * local_12;
        }
        local_178[(int)(uint)local_12] =
             (uint)*(ushort *)((longlong)param_1 + (ulonglong)local_26 * 2) -
             (uint)*(ushort *)((longlong)local_20 + (ulonglong)local_26 * 2);
      }
      if ((_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 2) && (param_3 == '\0')) {
        CMF_ProcessCore(local_178,0x50,_refptr_g_tsaPrmtFlashConst[0x24],0x7fffffff,
                        (int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x3e),&local_179);
      }
      for (local_14 = 0; local_14 < (byte)_refptr_g_tsaPrmtFlashConst[0x24]; local_14 = local_14 + 1
          ) {
        if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\x01') {
          uVar1 = local_14 + _refptr_g_tsaPrmtFlash->bCols * local_10;
        }
        else {
          uVar1 = local_10 + _refptr_g_tsaPrmtFlash->bCols * local_14;
        }
        local_2c = (uint)uVar1;
        local_2e = BLReset_GetRawChangedThold(local_14,local_10);
        uVar3 = (int)local_178[(int)(uint)local_14] >> 0x1f;
        if (((int)local_2e < (int)((local_178[(int)(uint)local_14] ^ uVar3) - uVar3)) &&
           ((*(byte *)((longlong)(int)local_2c + *(longlong *)_refptr_g_tpSensorFlag) & 0x20) == 0))
        {
          local_a = local_a + 1;
        }
      }
    }
    if (local_a < 4) {
      uVar2 = 0;
    }
    else {
      uVar2 = 1;
    }
  }
  else {
    local_24 = memcpy_s(param_1,(ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,local_20,
                        (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
    if (local_24 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Alg/Baseline/BaselineReset.c",0x295,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
    uVar2 = 0;
  }
  return uVar2;
}


// ============================================================================
// Function: BLReset_GetRawChangedThold @ 6ba4901e
// ============================================================================

short BLReset_GetRawChangedThold(ushort param_1,ushort param_2)

{
  short local_a;
  
  local_a = *(short *)&_refptr_g_tsaPrpt[1].field_0x14;
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\x01') {
    if ((param_1 == 0) || ((uint)param_1 == (byte)_refptr_g_tsaPrmtFlashConst[0x24] - 1)) {
      local_a = (short)((ulonglong)
                        ((longlong)(*(short *)&_refptr_g_tsaPrpt[1].field_0x14 * 4) * 0x55555556) >>
                       0x20) - (*(short *)&_refptr_g_tsaPrpt[1].field_0x14 >> 0xf);
    }
  }
  else if ((param_2 == 0) || ((uint)param_2 == (byte)_refptr_g_tsaPrmtFlashConst[0x25] - 1)) {
    local_a = (short)((ulonglong)
                      ((longlong)(*(short *)&_refptr_g_tsaPrpt[1].field_0x14 * 4) * 0x55555556) >>
                     0x20) - (*(short *)&_refptr_g_tsaPrpt[1].field_0x14 >> 0xf);
  }
  return local_a;
}


