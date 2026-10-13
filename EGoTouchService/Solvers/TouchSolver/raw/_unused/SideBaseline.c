// ============================================================================
// Function: SideBaseline_ResetInit @ 6bb0cf86
// ============================================================================

void SideBaseline_ResetInit(void)

{
  int iVar1;
  
                    /* 0xccf86  1308  SideBaseline_ResetInit */
  iVar1 = memset_s(&g_safeBaselineResetLeft,0x2c,0,0x2c);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SideTouch/SideTouch_BaselineReset.c",0x35,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(&g_safeBaselineResetRight,0x2c,0,0x2c);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SideTouch/SideTouch_BaselineReset.c",0x37,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  g_resetPtr = &g_safeBaselineResetLeft;
  return;
}


// ============================================================================
// Function: SideBaseline_IsResetTriggered @ 6bb0d777
// ============================================================================

bool SideBaseline_IsResetTriggered(void)

{
  int iVar1;
  bool bVar2;
  
                    /* 0xcd777  1303  SideBaseline_IsResetTriggered */
  if (*(int *)_refptr_g_baselineSide == 4) {
    g_resetPtr = &g_safeBaselineResetLeft;
  }
  else {
    g_resetPtr = &g_safeBaselineResetRight;
  }
  iVar1 = SideBaseline_ResetCheck();
  bVar2 = iVar1 != 0;
  if (bVar2) {
    (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
         (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 0x2000;
  }
  (*_refptr_g_tsaStaticPtr)->field_0x2f8 = bVar2;
  return bVar2;
}


// ============================================================================
// Function: SideBaseline_ResetInDebounce @ 6bb0d055
// ============================================================================

undefined4 SideBaseline_ResetInDebounce(void)

{
                    /* 0xcd055  1307  SideBaseline_ResetInDebounce */
  return CONCAT31((int3)((uint)*(int *)(g_resetPtr + 0x20) >> 8),*(int *)(g_resetPtr + 0x20) != 0);
}


// ============================================================================
// Function: SideBaseline_ResetCheck @ 6bb0d5ab
// ============================================================================

bool SideBaseline_ResetCheck(void)

{
  longlong lVar1;
  char cVar2;
  undefined4 uVar3;
  
                    /* 0xcd5ab  1306  SideBaseline_ResetCheck */
  *(undefined4 *)(g_resetPtr + 0x20) = 0;
  *(undefined4 *)(g_resetPtr + 0x28) = 0;
  cVar2 = SafeBaseline_IsSignalStable();
  if (cVar2 != '\0') {
    cVar2 = BLSM_IsTouched();
    lVar1 = g_resetPtr;
    if (cVar2 == '\0') {
      uVar3 = SafeBaseline_JudgeCurrentSideBaseline(*(undefined4 *)_refptr_g_baselineSide);
      *(undefined4 *)(lVar1 + 0x28) = uVar3;
      if (*(int *)(g_resetPtr + 0x28) != 1) {
        if (((*_refptr_g_prevTouchesPtr)->field_0xf == (*_refptr_g_prevTouchesPtr)->field_0xe) &&
           ((*_refptr_g_prevTouchesPtr)->field_0xf != '\0')) {
          *(uint *)(g_resetPtr + 0x20) = *(uint *)(g_resetPtr + 0x20) | 1;
        }
        cVar2 = SideBaseline_IsTouchToBeProtected();
        if ((cVar2 == '\0') || (*(int *)(g_resetPtr + 0x28) == -1)) {
          cVar2 = SideTouch_CheckAbnormalMaxMin((int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x46));
          if (cVar2 != '\0') {
            *(uint *)(g_resetPtr + 0x20) = *(uint *)(g_resetPtr + 0x20) | 2;
          }
          cVar2 = SideBaseline_IsVeryNegative();
          if (cVar2 != '\0') {
            *(uint *)(g_resetPtr + 0x20) = *(uint *)(g_resetPtr + 0x20) | 0x20;
          }
        }
      }
      if ((*(short *)(*(longlong *)_refptr_g_baselineData + 0x1058) <
           *(short *)(_refptr_g_tsaPrmtDynamic + 0x42)) &&
         (*(char *)(*(longlong *)_refptr_g_baselineStatic + 0x106) == '\0')) {
        cVar2 = SideTouch_CheckAbnormalMaxMin((int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x46));
        if (cVar2 != '\0') {
          *(uint *)(g_resetPtr + 0x20) = *(uint *)(g_resetPtr + 0x20) | 0x100;
        }
      }
    }
  }
  SideBaseline_ResetStatistics();
  return *(int *)(g_resetPtr + 0x24) != 0;
}


// ============================================================================
// Function: SideBaseline_ResetStatistics @ 6bb0d23e
// ============================================================================

void SideBaseline_ResetStatistics(void)

{
  byte local_9;
  
                    /* 0xcd23e  1309  SideBaseline_ResetStatistics */
  *(undefined4 *)(g_resetPtr + 0x24) = 0;
  for (local_9 = 0; local_9 < 10; local_9 = local_9 + 1) {
    if (((((*_refptr_g_tsaStaticPtr)->pFrameBufferPtr <
           (void *)((ulonglong)
                    *(uint *)(&g_timeScreenOnTholdSide + (longlong)(int)(uint)local_9 * 4) +
                   *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x80)) ||
         (*(int *)(g_resetPtr + 0x28) == -1)) &&
        (((&g_pushAbnormalTholdSide)[(int)(uint)local_9] == '\0' ||
         (((byte)_refptr_g_safeBaseline[0x21] < (byte)(&g_pushAbnormalTholdSide)[(int)(uint)local_9]
          || (*(int *)(g_resetPtr + 0x28) == -1)))))) &&
       ((*(byte *)(g_resetPtr + (int)(uint)local_9) <
         (byte)(&g_triggerAbnormalTholdSide)[(int)(uint)local_9] ||
        (*(int *)(g_resetPtr + 0x28) == -1)))) {
      SideBaseline_ResetStatisticsSub(local_9);
    }
  }
  return;
}


// ============================================================================
// Function: SideBaseline_IsVeryNegative @ 6bb0d4f7
// ============================================================================

undefined8 SideBaseline_IsVeryNegative(void)

{
  undefined8 uVar1;
  
                    /* 0xcd4f7  1305  SideBaseline_IsVeryNegative */
  if ((*(short *)(*(longlong *)_refptr_g_baselineData + 0x1058) * 4 <
       *(short *)(*(longlong *)_refptr_g_baselineData + 0x105a) * -2) &&
     ((int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x42) <
      -(int)*(short *)(*(longlong *)_refptr_g_baselineData + 0x105a))) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: SideBaseline_IsTouchToBeProtected @ 6bb0d567
// ============================================================================

bool SideBaseline_IsTouchToBeProtected(void)

{
                    /* 0xcd567  1304  SideBaseline_IsTouchToBeProtected */
  return 0 < (int)((uint)(byte)(*_refptr_g_prevTouchesPtr)->field_0xf -
                  (uint)(byte)(*_refptr_g_prevTouchesPtr)->field_0xe);
}


// ============================================================================
// Function: SideBaseline_ResetStatisticsSub @ 6bb0d06d
// ============================================================================

void SideBaseline_ResetStatisticsSub(byte param_1)

{
  char cVar1;
  short local_c;
  ushort local_a;
  
                    /* 0xcd06d  1310  SideBaseline_ResetStatisticsSub */
  if ((1 << (param_1 & 0x1f) & *(uint *)(g_resetPtr + 0x20)) == 0) {
    local_c = (*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
    if (local_c < 0) {
      local_c = 0;
    }
    if ((int)local_c < (int)(uint)*(ushort *)(g_resetPtr + 10 + (longlong)(int)(uint)param_1 * 2)) {
      if (*(int *)(g_resetPtr + 0x20) == 0) {
        *(short *)(g_resetPtr + 10 + (longlong)(int)(uint)param_1 * 2) =
             *(short *)(g_resetPtr + 10 + (longlong)(int)(uint)param_1 * 2) - local_c;
      }
    }
    else {
      *(undefined2 *)(g_resetPtr + 10 + (longlong)(int)(uint)param_1 * 2) = 0;
    }
  }
  else {
    *(short *)(g_resetPtr + 10 + (longlong)(int)(uint)param_1 * 2) =
         *(short *)(g_resetPtr + 10 + (longlong)(int)(uint)param_1 * 2) +
         (*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
    local_a = *(ushort *)(&g_timeAbnormalTholdSide + (longlong)(int)(uint)param_1 * 2);
    cVar1 = SafeBaseline_IsSafeDiffChecked();
    if (cVar1 == '\0') {
      local_a = 0xfa;
    }
    if (local_a <= *(ushort *)(g_resetPtr + 10 + (longlong)(int)(uint)param_1 * 2)) {
      *(undefined2 *)(g_resetPtr + 10 + (longlong)(int)(uint)param_1 * 2) = 0;
      *(char *)(g_resetPtr + (int)(uint)param_1) =
           *(char *)(g_resetPtr + (int)(uint)param_1) + '\x01';
      *(uint *)(g_resetPtr + 0x24) = 1 << (param_1 & 0x1f) | *(uint *)(g_resetPtr + 0x24);
    }
  }
  return;
}


