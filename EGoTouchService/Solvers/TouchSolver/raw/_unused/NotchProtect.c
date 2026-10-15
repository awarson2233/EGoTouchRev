// ============================================================================
// Function: NotchProtect_IsNotchBaselineDirty @ 6bb43d4e
// ============================================================================

undefined8 NotchProtect_IsNotchBaselineDirty(void)

{
  TSAStatic *pTVar1;
  bool bVar2;
  char cVar3;
  int iVar4;
  char *_Str1;
  undefined8 uVar5;
  bool bVar6;
  ushort local_2a;
  ushort local_28;
  ushort local_26;
  byte local_21;
  int local_20;
  int local_1c;
  
                    /* 0x103d4e  948  NotchProtect_IsNotchBaselineDirty */
  local_1c = 900;
  local_20 = 500;
  local_21 = 1;
  _Str1 = (char *)TSA_GetPrmtName();
  iVar4 = strncmp(_Str1,"P150",4);
  bVar6 = iVar4 == 0;
  if (bVar6) {
    local_1c = 0x708;
    local_20 = 300;
    local_21 = 4;
  }
  if ((((_refptr_g_tsaPrmtFlash[2].field_0x6d8 == '\0') ||
       (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) != 0)) ||
      ((*(int *)&_refptr_g_tsaPrpt[1].field_0x2c == 2 &&
       (*(int *)&_refptr_g_tsaPrpt[1].field_0x30 == 2)))) ||
     (((cVar3 = TSA_IsGameScenario(), cVar3 != '\0' ||
       (cVar3 = NotchProtect_IsTouchProtected(), cVar3 != '\0')) ||
      (2 < *(ushort *)&_refptr_g_tsaPrpt->field_0x8)))) {
    *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x3d0 = 0;
    return 0;
  }
  bVar2 = true;
  local_26 = _refptr_g_tsaPrmtFlash->bCols + 5;
  local_28 = 3000;
  if ((*_refptr_g_tsaStaticPtr)->field_0x3dc != '\0') {
    local_28 = 0x14d;
  }
  for (local_2a = 5;
      (int)(uint)local_2a < (int)((uint)_refptr_g_tsaPrmtFlash->bRows - (uint)local_21);
      local_2a = local_2a + 1) {
    if ((local_1c < *(int *)(_refptr_g_tsaBufSelf + ((longlong)(int)(uint)local_26 + 0x460) * 4)) &&
       ((!bVar6 ||
        ((bVar6 && ((int)((uint)*(ushort *)
                                 (_refptr_g_tsaBufSelf + ((longlong)(int)(uint)local_26 + 0x3c0) * 2
                                 ) * local_20) <
                    *(int *)(_refptr_g_tsaBufSelf + ((longlong)(int)(uint)local_26 + 800) * 4)))))))
    {
      bVar2 = false;
    }
    local_26 = local_26 + 1;
  }
  if (((bVar2) &&
      ((ushort)(byte)_refptr_g_tsaPrmtFlash[2].field_0x6db <
       *(ushort *)
        (_refptr_g_tsaBufSelf + ((longlong)(int)(uint)_refptr_g_tsaPrmtFlash->bCols + 0x3c0) * 2)))
     && ((((ushort)(byte)_refptr_g_tsaPrmtFlash[2].field_0x6dc <
           *(ushort *)
            (_refptr_g_tsaBufSelf + ((longlong)(int)(_refptr_g_tsaPrmtFlash->bCols + 1) + 0x3c0) * 2
            ) && ((((int)*(short *)&_refptr_g_tsaPrpt->field_0xc0 <
                    (int)(uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6de &&
                   (_refptr_g_tsaPrpt->nSPeakPositive < 1000)) &&
                  ((int)(uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e0 <
                   (int)*(short *)&_refptr_g_tsaPrpt->field_0xc0)))) &&
         (local_20 < _refptr_g_tsaPrpt->nSPeakPositive)))) {
    if (*(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x3d0 == 0) {
      *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x3d0 = 1;
    }
    else {
      pTVar1 = *_refptr_g_tsaStaticPtr;
      uVar5 = BLReset_UpdateTimer(*(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x3d0);
      *(undefined8 *)&pTVar1->field_0x3d0 = uVar5;
    }
  }
  else {
    *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x3d0 = 0;
  }
  if ((ulonglong)local_28 < *(ulonglong *)&(*_refptr_g_tsaStaticPtr)->field_0x3d0) {
    *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x3d0 = 0;
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: NotchProtect_IsTouchProtected @ 6bb43c30
// ============================================================================

undefined1 NotchProtect_IsTouchProtected(void)

{
  ushort uVar1;
  ushort uVar2;
  ushort local_c;
  undefined1 local_9;
  
                    /* 0x103c30  949  NotchProtect_IsTouchProtected */
  local_9 = 0;
  for (local_c = 0; local_c < (*_refptr_g_prevTouchesPtr)->bPrevTouchCount; local_c = local_c + 1) {
    uVar1 = *(ushort *)
             ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_c * 0x478 + 0x496) >>
            8;
    uVar2 = *(ushort *)
             ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_c * 0x478 + 0x498) >>
            8;
    if ((((1 < uVar1) && ((int)(uint)uVar1 < (int)(_refptr_g_tsaPrmtFlash->bCols - 2))) &&
        (1 < uVar2)) && ((int)(uint)uVar2 < (int)(_refptr_g_tsaPrmtFlash->bRows - 2))) {
      local_9 = 1;
    }
    if ((uVar1 <= (ushort)((byte)_refptr_g_tsaPrmtFlash[2].field_0x6d9 + 1)) &&
       (uVar2 <= (ushort)((byte)_refptr_g_tsaPrmtFlash[2].field_0x6da + 1))) {
      local_9 = 0;
    }
  }
  return local_9;
}


