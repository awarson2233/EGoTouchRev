// ============================================================================
// Function: Misc_GetShiftNum @ 6ba64b9c
// ============================================================================

char Misc_GetShiftNum(ushort *param_1,ushort param_2)

{
  char local_9;
  
                    /* 0x24b9c  922  Misc_GetShiftNum */
  local_9 = '\0';
  if (*param_1 == 0) {
    local_9 = '\0';
  }
  else if (*param_1 < param_2) {
    while (*param_1 < param_2) {
      *param_1 = *param_1 * 2;
      local_9 = local_9 + '\x01';
    }
  }
  else {
    while (param_2 < *param_1) {
      *param_1 = *param_1 >> 1;
      local_9 = local_9 + -1;
    }
  }
  return local_9;
}


// ============================================================================
// Function: Misc_ShiftInINT64S @ 6ba64c33
// ============================================================================

void Misc_ShiftInINT64S(ulonglong *param_1,byte param_2)

{
  ulonglong local_18;
  char local_9;
  
                    /* 0x24c33  925  Misc_ShiftInINT64S */
  local_9 = '\0';
  if ((longlong)*param_1 < 0) {
    local_18 = -*param_1;
  }
  else {
    local_18 = *param_1;
  }
  for (; (ulonglong)(1L << (param_2 & 0x3f)) < local_18; local_18 = local_18 >> 1) {
    local_9 = local_9 + '\x01';
  }
  if ((longlong)*param_1 < 0) {
    *param_1 = -local_18;
  }
  else {
    *param_1 = local_18;
  }
  *(char *)(param_1 + 1) = local_9;
  return;
}


// ============================================================================
// Function: Misc_GetShiftBits @ 6ba64cdb
// ============================================================================

byte Misc_GetShiftBits(ulonglong param_1,byte param_2)

{
  ulonglong local_res8;
  byte local_9;
  
                    /* 0x24cdb  921  Misc_GetShiftBits */
  local_9 = 0;
  local_res8 = param_1;
  if ((longlong)param_1 < 0) {
    local_res8 = -param_1;
  }
  for (; (ulonglong)(1L << (param_2 & 0x3f)) <= local_res8 >> (local_9 & 0x3f);
      local_9 = local_9 + 1) {
  }
  return local_9;
}


// ============================================================================
// Function: Misc_RightShift @ 6ba64d3c
// ============================================================================

ulonglong Misc_RightShift(ulonglong param_1,byte param_2)

{
                    /* 0x24d3c  924  Misc_RightShift */
  if ((longlong)param_1 < 0) {
    param_1 = -(-param_1 >> (param_2 & 0x3f));
  }
  else {
    param_1 = param_1 >> (param_2 & 0x3f);
  }
  return param_1;
}


// ============================================================================
// Function: Misc_SqrtUint32 @ 6ba64d8b
// ============================================================================

int Misc_SqrtUint32(uint param_1)

{
  int local_10;
  uint local_c;
  
                    /* 0x24d8b  927  Misc_SqrtUint32 */
  local_10 = 0;
  for (local_c = 0x8000; param_1 < local_c; local_c = local_c >> 1) {
  }
  for (; local_c != 0; local_c = local_c >> 1) {
    local_10 = local_10 + local_c;
    if (param_1 < (uint)(local_10 * local_10)) {
      local_10 = local_10 - local_c;
    }
  }
  return local_10;
}


// ============================================================================
// Function: Misc_SqrtUint64 @ 6ba64ddd
// ============================================================================

longlong Misc_SqrtUint64(ulonglong param_1)

{
  longlong local_18;
  ulonglong local_10;
  
                    /* 0x24ddd  928  Misc_SqrtUint64 */
  local_18 = 0;
  for (local_10 = 0x80000000; param_1 < local_10; local_10 = local_10 >> 1) {
  }
  for (; local_10 != 0; local_10 = local_10 >> 1) {
    local_18 = local_18 + local_10;
    if (param_1 <= (ulonglong)(local_18 * local_18) && local_18 * local_18 - param_1 != 0) {
      local_18 = local_18 - local_10;
    }
  }
  return local_18;
}


// ============================================================================
// Function: Misc_Sin @ 6ba64e40
// ============================================================================

int Misc_Sin(int param_1)

{
  int local_res8;
  int local_c;
  
                    /* 0x24e40  926  Misc_Sin */
  local_c = 1;
  local_res8 = param_1;
  if (param_1 < 0) {
    local_res8 = -param_1;
    local_c = -1;
  }
  if (0xb4 < local_res8) {
    local_res8 = 0xb4;
  }
  if (0x5a < local_res8) {
    local_res8 = 0xb4 - local_res8;
  }
  return (uint)*(ushort *)(&g_miscSinTable + (longlong)local_res8 * 2) * local_c;
}


// ============================================================================
// Function: Misc_Cos @ 6ba64eaa
// ============================================================================

int Misc_Cos(int param_1)

{
  int local_res8;
  int local_c;
  
                    /* 0x24eaa  919  Misc_Cos */
  local_res8 = param_1;
  if (param_1 < 0) {
    local_res8 = -param_1;
  }
  if (0xb4 < local_res8) {
    local_res8 = 0xb4;
  }
  local_c = 1;
  if (0x5a < local_res8) {
    local_res8 = 0xb4 - local_res8;
    local_c = -1;
  }
  return (uint)*(ushort *)(&g_miscSinTable + (longlong)(0x5a - local_res8) * 2) * local_c;
}


// ============================================================================
// Function: Misc_IsInRect @ 6ba64f1f
// ============================================================================

undefined8 Misc_IsInRect(ushort *param_1,int param_2,int param_3)

{
  undefined8 uVar1;
  
                    /* 0x24f1f  923  Misc_IsInRect */
  if ((int)(uint)*param_1 < param_2) {
    if (param_2 < (int)(uint)param_1[1]) {
      if ((int)(uint)param_1[2] < param_3) {
        if (param_3 < (int)(uint)param_1[3]) {
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
  return uVar1;
}


// ============================================================================
// Function: Misc_FindKthSmallest @ 6ba6572e
// ============================================================================

/* WARNING: Function: ___chkstk_ms replaced with injection: alloca_probe */

undefined4 Misc_FindKthSmallest(longlong param_1,int param_2,int param_3)

{
  bool bVar1;
  undefined4 uVar2;
  undefined4 *puVar3;
  int aiStack_48 [6];
  int local_30;
  int local_2c;
  int local_24;
  int local_20;
  int local_1c;
  
                    /* 0x2572e  920  Misc_FindKthSmallest */
  if ((((param_1 == 0) || (param_2 < 1)) || (param_3 < 1)) || (param_2 <= param_3)) {
    uVar2 = 0;
  }
  else {
    puVar3 = (undefined4 *)
             ((longlong)aiStack_48 - ((longlong)param_2 * 4 + 0x12U & 0xfffffffffffffff0));
    for (local_1c = 0; local_1c < param_3; local_1c = local_1c + 1) {
      puVar3[local_1c] = 0;
    }
    local_24 = 0;
    for (local_20 = 0; local_20 < param_2; local_20 = local_20 + 1) {
      if (local_24 != 0) {
        bVar1 = false;
        local_2c = 0;
LAB_6ba658f1:
        if (local_2c < local_24) {
          if ((int)puVar3[local_2c] <= *(int *)(param_1 + (longlong)local_20 * 4))
          goto code_r0x6ba6586f;
          if (local_24 == param_3) {
            local_30 = local_24 + -1;
          }
          else {
            local_30 = local_24;
          }
          for (; local_2c < local_30; local_30 = local_30 + -1) {
            puVar3[local_30] = puVar3[local_30 + -1];
          }
          puVar3[local_2c] = *(undefined4 *)(param_1 + (longlong)local_20 * 4);
          if (local_24 < param_3) {
            local_24 = local_24 + 1;
          }
          bVar1 = true;
        }
        if ((local_24 < param_3) && (!bVar1)) {
          puVar3[local_24] = *(undefined4 *)(param_1 + (longlong)local_20 * 4);
          local_24 = local_24 + 1;
        }
        goto LAB_6ba65932;
      }
      *puVar3 = *(undefined4 *)(param_1 + (longlong)local_20 * 4);
      local_24 = 1;
LAB_6ba65932:
    }
    uVar2 = puVar3[param_3 + -1];
  }
  return uVar2;
code_r0x6ba6586f:
  local_2c = local_2c + 1;
  goto LAB_6ba658f1;
}


// ============================================================================
// Function: SubTz_GetSignalThresh @ 6ba7de91
// ============================================================================

short SubTz_GetSignalThresh(short *param_1,undefined8 *param_2)

{
  char cVar1;
  int iVar2;
  short local_a;
  
  cVar1 = TSAStatic_IsAFTModeEnabled();
  if (cVar1 == '\0') {
    if ((*(char *)((longlong)param_1 + 0x34b) != '\0') || ((*(uint *)(param_1 + 0x1a8) & 200) != 0))
    {
      cVar1 = SignalDisparity_IsSignalCompensated();
      if ((cVar1 != '\0') &&
         (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SIGNAL_DISPARITY_POST) != 0)) {
        *param_2 = *(undefined8 *)_refptr_g_tsaBufSd;
        local_a = *(short *)((ulonglong)
                             ((uint)(*_refptr_g_peaksPtr)
                                    [(longlong)(int)(uint)*(byte *)(param_1 + 0x15b) * 0x14 + 0x15].
                                    bPeakCount +
                              (uint)(*_refptr_g_peaksPtr)
                                    [(longlong)(int)(uint)*(byte *)(param_1 + 0x15b) * 0x14 + 0x16].
                                    bPeakCount * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
                            *(longlong *)_refptr_g_tsaBufSd) / 2;
        goto LAB_6ba7e26d;
      }
    }
    if ((*(uint *)(param_1 + 2) & 2) == 0) {
      if (*(int *)(param_1 + 0x1a8) == 8) {
        iVar2 = (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x820 *
                (int)*(short *)(*_refptr_g_peaksPtr +
                               (longlong)(int)(uint)*(byte *)((longlong)param_1 + 0x2b7) * 0x14 +
                               0x18);
        if (iVar2 < 0) {
          iVar2 = iVar2 + 0x7f;
        }
        local_a = (short)(iVar2 >> 7);
      }
      else if (((((uint)(*_refptr_g_peaksPtr)
                        [(longlong)(int)(uint)*(byte *)(param_1 + 0x15b) * 0x14 + 0x15].bPeakCount
                  == _refptr_g_tsaPrmtFlash->bCols - 1) ||
                ((*_refptr_g_peaksPtr)
                 [(longlong)(int)(uint)*(byte *)(param_1 + 0x15b) * 0x14 + 0x15].bPeakCount == '\0')
                ) || ((uint)(*_refptr_g_peaksPtr)
                            [(longlong)(int)(uint)*(byte *)(param_1 + 0x15b) * 0x14 + 0x16].
                            bPeakCount == _refptr_g_tsaPrmtFlash->bRows - 1)) ||
              ((*_refptr_g_peaksPtr)[(longlong)(int)(uint)*(byte *)(param_1 + 0x15b) * 0x14 + 0x16].
               bPeakCount == '\0')) {
        iVar2 = (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x81f *
                (int)*(short *)(*_refptr_g_peaksPtr +
                               (longlong)(int)(uint)*(byte *)((longlong)param_1 + 0x2b7) * 0x14 +
                               0x18);
        if (iVar2 < 0) {
          iVar2 = iVar2 + 0x7f;
        }
        local_a = (short)(iVar2 >> 7);
      }
      else {
        iVar2 = (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x81f *
                (int)*(short *)(*_refptr_g_peaksPtr +
                               (longlong)(int)(uint)*(byte *)(param_1 + 0x15b) * 0x14 + 0x18);
        if (iVar2 < 0) {
          iVar2 = iVar2 + 0x7f;
        }
        local_a = (short)(iVar2 >> 7);
      }
    }
    else {
      iVar2 = (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x81f *
              (int)*(short *)(*_refptr_g_peaksPtr +
                             (longlong)(int)(uint)*(byte *)(param_1 + 0x15b) * 0x14 + 0x18);
      if (iVar2 < 0) {
        iVar2 = iVar2 + 0x7f;
      }
      local_a = (short)(iVar2 >> 7);
    }
  }
  else {
    local_a = *param_1;
  }
LAB_6ba7e26d:
  if (local_a < *param_1) {
    local_a = *param_1;
  }
  return local_a;
}


