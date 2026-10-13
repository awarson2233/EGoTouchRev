// ============================================================================
// Function: Posture_UpdatePostureType @ 6baf81fb
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void Posture_UpdatePostureType(void)

{
  longlong lVar1;
  int iVar2;
  undefined4 uVar3;
  undefined8 local_38;
  int local_30;
  short local_1e;
  undefined4 local_1c;
  
                    /* 0xb81fb  1038  Posture_UpdatePostureType */
  if ((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6c4 & 1) != 0) {
    if ((*_refptr_g_tsaStaticPtr)->field_0x3 != '\0') {
      *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x138) = 0;
    }
    if (DAT_6bba3f50 == (*_refptr_g_tsaStaticPtr)->llField_0xe8) {
      if ((DAT_6bb4f9ca + 1) % 0x28 == (uint)DAT_6bb4f9c8) {
        Posture_DeQuene(&g_posturePointQueue);
      }
      DAT_6bba3f48 = Posture_GetBranchType
                               ((int)g_lastPosturePointFrame,(int)_DAT_6bba3f42,(int)_DAT_6bba3f44,0
                               );
      if ((DAT_6bba3f48 == 6) && (0xc00 < _DAT_6bba3f44)) {
        DAT_6bba3f48 = 3;
      }
      local_38 = CONCAT26(uRam000000006bba3f46,
                          CONCAT24(_DAT_6bba3f44,CONCAT22(_DAT_6bba3f42,g_lastPosturePointFrame)));
      local_30 = DAT_6bba3f48;
      Posture_EnQuene(&g_posturePointQueue,&local_38);
      if ((DAT_6bb4f9ca + 1) % 0x28 == (uint)DAT_6bb4f9c8) {
        iVar2 = Posture_VarQueue(g_posturePointQueue);
        if (iVar2 == 0) {
          Posture_InitQuene(&g_posturePointQueue);
          *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x138) = 0;
        }
        else {
          local_1c = Posture_ModeType(g_posturePointQueue);
          local_1e = *(short *)(g_posturePointQueue + (ulonglong)DAT_6bb4f9ca * 0xc);
          lVar1 = *(longlong *)_refptr_g_touchesPtr;
          uVar3 = Posture_TypePortraitFix(local_1c,(int)local_1e);
          *(undefined4 *)(lVar1 + 0x138) = uVar3;
        }
      }
      else {
        *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x138) = 0;
      }
    }
    else {
      *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x138) =
           *(undefined4 *)&(*_refptr_g_prevTouchesPtr)[8].field_0x18;
    }
  }
  return;
}


// ============================================================================
// Function: Posture_IsHangingPosture @ 6baf8438
// ============================================================================

undefined8 Posture_IsHangingPosture(void)

{
  undefined8 uVar1;
  
                    /* 0xb8438  1037  Posture_IsHangingPosture */
  if (((((*(int *)(*(longlong *)_refptr_g_touchesPtr + 0x138) == 6) ||
        (*(int *)(*(longlong *)_refptr_g_touchesPtr + 0x138) == 7)) ||
       (*(int *)(*(longlong *)_refptr_g_touchesPtr + 0x138) == 8)) ||
      ((*(int *)(*(longlong *)_refptr_g_touchesPtr + 0x138) == 1 ||
       (*(int *)(*(longlong *)_refptr_g_touchesPtr + 0x138) == 2)))) &&
     (0x5dc < *(short *)&_refptr_g_tsaPrpt->field_0x140)) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: Posture_DeQuene @ 6baf7770
// ============================================================================

void Posture_DeQuene(longlong param_1)

{
  uint uVar1;
  
  if (*(short *)(param_1 + 10) != *(short *)(param_1 + 8)) {
    uVar1 = *(ushort *)(param_1 + 8) + 1;
    *(short *)(param_1 + 8) = (short)uVar1 + (short)(uVar1 / 0x28) * -0x28;
  }
  return;
}


// ============================================================================
// Function: Posture_EnQuene @ 6baf77c8
// ============================================================================

void Posture_EnQuene(longlong *param_1,undefined8 *param_2)

{
  undefined8 *puVar1;
  uint uVar2;
  
  if ((*(ushort *)((longlong)param_1 + 10) + 1) % 0x28 != (uint)*(ushort *)(param_1 + 1)) {
    puVar1 = (undefined8 *)((ulonglong)*(ushort *)((longlong)param_1 + 10) * 0xc + *param_1);
    *puVar1 = *param_2;
    *(undefined4 *)(puVar1 + 1) = *(undefined4 *)(param_2 + 1);
    uVar2 = *(ushort *)((longlong)param_1 + 10) + 1;
    *(short *)((longlong)param_1 + 10) = (short)uVar2 + (short)(uVar2 / 0x28) * -0x28;
  }
  return;
}


// ============================================================================
// Function: Posture_InitQuene @ 6baf787c
// ============================================================================

void Posture_InitQuene(longlong param_1)

{
  *(undefined2 *)(param_1 + 8) = 0;
  *(undefined2 *)(param_1 + 10) = 0;
  return;
}


// ============================================================================
// Function: Posture_VarQueue @ 6baf789a
// ============================================================================

bool Posture_VarQueue(longlong param_1)

{
  longlong lVar1;
  int local_40;
  ushort local_3a;
  longlong local_38;
  longlong local_30;
  longlong local_28;
  longlong local_20;
  longlong local_18;
  longlong local_10;
  
  local_10 = 0;
  local_18 = 0;
  local_20 = 0;
  local_28 = 0;
  local_30 = 0;
  local_38 = 0;
  for (local_3a = 0; local_3a < 0x28; local_3a = local_3a + 1) {
    local_10 = local_10 + *(short *)(param_1 + (ulonglong)local_3a * 0xc);
    local_18 = local_18 + *(short *)(param_1 + (ulonglong)local_3a * 0xc + 2);
    local_20 = local_20 + *(short *)(param_1 + (ulonglong)local_3a * 0xc + 4);
  }
  local_10 = local_10 / 0x28;
  local_18 = local_18 / 0x28;
  local_20 = local_20 / 0x28;
  for (local_40 = 0; local_40 < 0x28; local_40 = local_40 + 1) {
    if (local_10 < *(short *)(param_1 + (longlong)local_40 * 0xc)) {
      lVar1 = *(short *)(param_1 + (longlong)local_40 * 0xc) - local_10;
    }
    else {
      lVar1 = local_10 - *(short *)(param_1 + (longlong)local_40 * 0xc);
    }
    local_28 = local_28 + lVar1;
    if (local_18 < *(short *)(param_1 + (longlong)local_40 * 0xc + 2)) {
      lVar1 = *(short *)(param_1 + (longlong)local_40 * 0xc + 2) - local_18;
    }
    else {
      lVar1 = local_18 - *(short *)(param_1 + (longlong)local_40 * 0xc + 2);
    }
    local_30 = local_30 + lVar1;
    if (local_20 < *(short *)(param_1 + (longlong)local_40 * 0xc + 4)) {
      lVar1 = *(short *)(param_1 + (longlong)local_40 * 0xc + 4) - local_20;
    }
    else {
      lVar1 = local_20 - *(short *)(param_1 + (longlong)local_40 * 0xc + 4);
    }
    local_38 = local_38 + lVar1;
  }
  if (local_30 <= local_28) {
    local_30 = local_28;
  }
  if (local_38 <= local_30) {
    local_38 = local_30;
  }
  return local_38 < 0xc8000;
}


// ============================================================================
// Function: Posture_GetBranchType @ 6baf7be3
// ============================================================================

ulonglong Posture_GetBranchType(short param_1,short param_2,short param_3,uint param_4)

{
  longlong lVar1;
  bool bVar2;
  ulonglong uVar3;
  short local_a;
  
  if (*(longlong *)_refptr_g_tsaPrmtFlashPosture == 0) {
    uVar3 = 0;
  }
  else {
    lVar1 = *(longlong *)_refptr_g_tsaPrmtFlashPosture;
    if (*(int *)(lVar1 + (ulonglong)param_4 * 0x18 + 4) == 1) {
      local_a = param_1;
    }
    else if (*(int *)(lVar1 + (ulonglong)param_4 * 0x18 + 4) == 2) {
      local_a = param_2;
    }
    else if (*(int *)(lVar1 + (ulonglong)param_4 * 0x18 + 4) == 3) {
      local_a = param_3;
    }
    else {
      local_a = 0;
    }
    if (*(int *)(lVar1 + (ulonglong)param_4 * 0x18 + 8) == 3) {
      bVar2 = local_a <= *(short *)(lVar1 + (ulonglong)param_4 * 0x18 + 0xc);
    }
    else if (*(int *)(lVar1 + (ulonglong)param_4 * 0x18 + 8) == 4) {
      bVar2 = local_a < *(short *)(lVar1 + (ulonglong)param_4 * 0x18 + 0xc);
    }
    else if (*(int *)(lVar1 + (ulonglong)param_4 * 0x18 + 8) == 1) {
      bVar2 = *(short *)(lVar1 + (ulonglong)param_4 * 0x18 + 0xc) <= local_a;
    }
    else if (*(int *)(lVar1 + (ulonglong)param_4 * 0x18 + 8) == 2) {
      bVar2 = *(short *)(lVar1 + (ulonglong)param_4 * 0x18 + 0xc) < local_a;
    }
    else {
      bVar2 = false;
    }
    if (bVar2) {
      if (*(int *)(lVar1 + (ulonglong)param_4 * 0x18 + 0x14) == 0) {
        if (*(short *)(lVar1 + (ulonglong)param_4 * 0x18 + 0xe) < 0) {
          uVar3 = 0;
        }
        else {
          uVar3 = Posture_GetBranchType
                            ((int)param_1,(int)param_2,(int)param_3,
                             (int)*(short *)(lVar1 + (ulonglong)param_4 * 0x18 + 0xe));
        }
      }
      else {
        uVar3 = (ulonglong)*(uint *)(lVar1 + (ulonglong)param_4 * 0x18 + 0x14);
      }
    }
    else if (*(short *)(lVar1 + (ulonglong)param_4 * 0x18 + 0x10) < 0) {
      uVar3 = 0;
    }
    else {
      uVar3 = Posture_GetBranchType
                        ((int)param_1,(int)param_2,(int)param_3,
                         (int)*(short *)(lVar1 + (ulonglong)param_4 * 0x18 + 0x10));
    }
  }
  return uVar3;
}


// ============================================================================
// Function: Posture_ModeType @ 6baf7f0a
// ============================================================================

uint Posture_ModeType(longlong param_1)

{
  longlong lVar1;
  uint *puVar2;
  uint local_68 [15];
  uint local_2c;
  uint local_28;
  int local_24;
  uint local_20;
  uint local_1c;
  
  puVar2 = local_68;
  for (lVar1 = 7; lVar1 != 0; lVar1 = lVar1 + -1) {
    puVar2[0] = 0;
    puVar2[1] = 0;
    puVar2 = puVar2 + 2;
  }
  local_1c = 0;
  for (local_24 = 0; local_24 < 0x28; local_24 = local_24 + 1) {
    for (local_28 = 0; local_28 < 0xe; local_28 = local_28 + 1) {
      if (*(uint *)(param_1 + (longlong)local_24 * 0xc + 8) == local_28) {
        local_68[local_28] = local_68[local_28] + 1;
      }
    }
  }
  for (local_2c = 0; local_2c < 0xe; local_2c = local_2c + 1) {
    if (local_68[local_1c] < local_68[local_2c]) {
      local_1c = local_2c;
    }
    else {
    }
  }
  if (local_68[local_1c] < 0x15) {
    local_20 = 0;
  }
  else {
    local_20 = local_1c;
  }
  return local_20;
}


// ============================================================================
// Function: Posture_TypePortraitFix @ 6baf7fe8
// ============================================================================

int Posture_TypePortraitFix(int param_1,short param_2)

{
  if (param_1 == 5) {
    if ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 0) {
      if (param_2 < 1) {
        param_1 = 2;
      }
      else {
        param_1 = 1;
      }
    }
    else {
      param_1 = 7;
    }
  }
  else if (param_1 == 7) {
    if (((*_refptr_g_tsaStaticPtr)->nField_0x264 == 0) ||
       ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 2)) {
      return 8;
    }
  }
  else {
    if (param_1 == 0xd) {
      if (((*_refptr_g_tsaStaticPtr)->nField_0x264 != 0) &&
         ((*_refptr_g_tsaStaticPtr)->nField_0x264 != 2)) {
        return 7;
      }
      return 8;
    }
    if ((param_1 == 1) || (param_1 == 2)) {
      if (((*_refptr_g_tsaStaticPtr)->nField_0x264 == 1) ||
         ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 3)) {
        return 0xb;
      }
    }
    else if (param_1 == 9) {
      if (((*_refptr_g_tsaStaticPtr)->nField_0x264 == 1) ||
         ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 3)) {
        return 7;
      }
    }
    else if ((param_1 == 6) || (param_1 == 0xc)) {
      if (((*_refptr_g_tsaStaticPtr)->nField_0x264 == 1) ||
         ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 3)) {
        return 10;
      }
    }
    else if ((param_1 == 8) &&
            (((*_refptr_g_tsaStaticPtr)->nField_0x264 == 1 ||
             ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 3)))) {
      return 7;
    }
  }
  return param_1;
}


// ============================================================================
// Function: Posture_GetPosture @ 6baf84c7
// ============================================================================

undefined4 Posture_GetPosture(void)

{
                    /* 0xb84c7  1036  Posture_GetPosture */
  return *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x138);
}


