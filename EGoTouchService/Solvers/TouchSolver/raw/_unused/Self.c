// ============================================================================
// Function: Self_Process @ 6ba6aea8
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

void Self_Process(void)

{
                    /* 0x2aea8  1255  Self_Process */
  Self_ProcessDim(1);
  Self_ProcessDim(2);
  Self_GetProperty();
  return;
}


// ============================================================================
// Function: Self_ProcessDim @ 6ba6abb4
// ============================================================================

void Self_ProcessDim(int param_1)

{
  byte local_10;
  uchar local_f;
  ushort local_e;
  byte local_b;
  uchar local_a;
  byte local_9;
  
                    /* 0x2abb4  1256  Self_ProcessDim */
  if (param_1 == 1) {
    local_9 = _refptr_g_tsaPrmtFlash->bRows;
    local_a = _refptr_g_tsaPrmtFlash->bCols;
    local_f = '\x01';
    local_10 = _refptr_g_tsaPrmtFlash->bCols;
    local_e = (ushort)_refptr_g_tsaPrmtFlash->bCols;
  }
  else {
    local_9 = _refptr_g_tsaPrmtFlash->bCols;
    local_a = _refptr_g_tsaPrmtFlash->bRows;
    local_f = _refptr_g_tsaPrmtFlash->bCols;
    local_10 = 0;
    local_e = 1;
  }
  for (local_b = 0; local_b < local_9; local_b = local_b + 1) {
    Self_ProcessDimUnit(local_b * local_e,local_f,local_a,(uint)local_b + (uint)local_10);
  }
  return;
}


// ============================================================================
// Function: Self_GetProperty @ 6ba6ae85
// ============================================================================

void Self_GetProperty(void)

{
                    /* 0x2ae85  1253  Self_GetProperty */
  Self_GetPropertyDim(1);
  Self_GetPropertyDim(2);
  return;
}


// ============================================================================
// Function: Self_ProcessDimUnit @ 6ba6a984
// ============================================================================

void Self_ProcessDimUnit(ushort param_1,uint param_2,int param_3,uint param_4)

{
  int iVar1;
  int iVar2;
  int iVar3;
  short *local_38;
  short local_2a;
  int local_28;
  int local_24;
  int local_20;
  int local_1c;
  int local_18;
  int local_14;
  int local_10;
  int local_c;
  
                    /* 0x2a984  1257  Self_ProcessDimUnit */
  local_10 = 0;
  local_14 = 0;
  local_18 = 0;
  local_1c = 0;
  local_20 = 0;
  local_24 = 0;
  local_28 = 0;
  local_2a = 0;
  local_38 = (short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)param_1 * 2);
  local_c = param_3;
  while (iVar1 = local_c + -1, local_c != 0) {
    iVar2 = (int)*local_38;
    local_38 = local_38 + param_2;
    iVar3 = iVar2;
    if (iVar2 <= local_28) {
      iVar3 = local_28;
    }
    local_28 = iVar3;
    if (iVar3 <= -iVar2) {
      local_28 = -iVar2;
    }
    local_c = iVar1;
    if (iVar2 < 0x4b) {
      if (iVar2 < 0) {
        if (iVar2 < -0x4a) {
          local_10 = local_10 + iVar2;
          if ((iVar1 != param_3 + -1) && (iVar1 != 0)) {
            local_18 = local_18 + iVar2;
          }
          local_20 = local_20 + iVar2;
        }
        else {
          local_10 = local_10 + iVar2;
        }
      }
      else {
        local_14 = local_14 + iVar2;
      }
    }
    else {
      local_14 = local_14 + iVar2;
      if ((iVar1 != param_3 + -1) && (iVar1 != 0)) {
        local_2a = local_2a + 1;
        local_1c = local_1c + iVar2;
      }
      local_24 = local_24 + iVar2;
    }
  }
  *(int *)(_refptr_g_tsaBufSelf + (ulonglong)param_4 * 4) = local_14 + local_10;
  *(short *)(_refptr_g_tsaBufSelf + ((ulonglong)param_4 + 0x3c0) * 2) = local_2a;
  *(int *)(_refptr_g_tsaBufSelf + ((ulonglong)param_4 + 0x280) * 4) = local_24 + local_20;
  *(int *)(_refptr_g_tsaBufSelf + ((ulonglong)param_4 + 800) * 4) = local_1c + local_18;
  *(int *)(_refptr_g_tsaBufSelf + ((ulonglong)param_4 + 0x3c0) * 4) = local_24 - local_20;
  *(int *)(_refptr_g_tsaBufSelf + ((ulonglong)param_4 + 0x460) * 4) = local_1c - local_18;
  *(int *)(_refptr_g_tsaBufSelf + ((ulonglong)param_4 + 0x140) * 4) = local_14;
  *(int *)(_refptr_g_tsaBufSelf + ((ulonglong)param_4 + 0xa0) * 4) = local_10;
  *(short *)(_refptr_g_tsaBufSelf + ((ulonglong)param_4 + 0x460) * 2) = (short)local_28;
  return;
}


// ============================================================================
// Function: Self_GetPropertyDim @ 6ba6ac88
// ============================================================================

void Self_GetPropertyDim(int param_1)

{
  int iVar1;
  int *local_18;
  byte local_b;
  byte local_a;
  byte local_9;
  
                    /* 0x2ac88  1254  Self_GetPropertyDim */
  if (param_1 == 1) {
    local_b = 0;
    local_9 = _refptr_g_tsaPrmtFlash->bCols;
    local_18 = (int *)&_refptr_g_tsaPrpt->field_0x22c;
  }
  else {
    local_b = _refptr_g_tsaPrmtFlash->bCols;
    local_9 = _refptr_g_tsaPrmtFlash->bRows;
    local_18 = (int *)&_refptr_g_tsaPrpt->field_0x250;
  }
  *local_18 = -0x80000000;
  local_18[2] = 0x7fffffff;
  local_18[4] = -0x80000000;
  local_18[6] = 0x7fffffff;
  *(undefined1 *)(local_18 + 1) = 0;
  *(undefined1 *)(local_18 + 3) = 0;
  *(undefined1 *)(local_18 + 7) = 0;
  *(undefined1 *)(local_18 + 5) = 0;
  local_18[8] = 0;
  for (local_a = local_b; (uint)local_a < (uint)local_9 + (uint)local_b; local_a = local_a + 1) {
    iVar1 = *(int *)(_refptr_g_tsaBufSelf + (longlong)(int)(uint)local_a * 4);
    if (*local_18 < iVar1) {
      *local_18 = iVar1;
      *(byte *)(local_18 + 1) = local_a - local_b;
    }
    else if (iVar1 < local_18[2]) {
      local_18[2] = iVar1;
      *(byte *)(local_18 + 3) = local_a - local_b;
    }
    if (local_18[4] < *(int *)(_refptr_g_tsaBufSelf + ((longlong)(int)(uint)local_a + 0x140) * 4)) {
      local_18[4] = *(int *)(_refptr_g_tsaBufSelf + ((longlong)(int)(uint)local_a + 0x140) * 4);
      *(byte *)(local_18 + 5) = local_a - local_b;
    }
    if (*(int *)(_refptr_g_tsaBufSelf + ((longlong)(int)(uint)local_a + 0xa0) * 4) < local_18[6]) {
      local_18[6] = *(int *)(_refptr_g_tsaBufSelf + ((longlong)(int)(uint)local_a + 0xa0) * 4);
      *(byte *)(local_18 + 7) = local_a - local_b;
    }
    local_18[8] = local_18[8] +
                  *(int *)(_refptr_g_tsaBufSelf + ((longlong)(int)(uint)local_a + 0x280) * 4);
  }
  return;
}


// ============================================================================
// Function: Self_Reset @ 6ba6a920
// ============================================================================

void Self_Reset(void)

{
  int iVar1;
  
                    /* 0x2a920  1258  Self_Reset */
  iVar1 = memset_s(_refptr_g_tsaBufSelf,0x1b80,0,0x1b80);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/SelfData/SelfData.c",0xe,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


