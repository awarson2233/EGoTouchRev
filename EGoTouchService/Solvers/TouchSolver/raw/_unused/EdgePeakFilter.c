// ============================================================================
// Function: EdgePeakFilter_WorkAround @ 6bb438f6
// ============================================================================

void EdgePeakFilter_WorkAround(void)

{
  char cVar1;
  short sVar2;
  uchar local_9;
  
                    /* 0x1038f6  559  EdgePeakFilter_WorkAround */
  if (_refptr_g_tsaPrmtFlash->field_0x1a == '\0') {
    sVar2 = EdgePeakFilter_GetMaxSig(2,0);
    if (sVar2 < 0) {
      sVar2 = sVar2 + 7;
    }
    local_9 = (*_refptr_g_peaksPtr)->bPeakCount;
    while (local_9 = local_9 + 0xff, -1 < (char)local_9) {
      cVar1 = Peak_IsOnLine(2,0,local_9);
      if ((cVar1 != '\0') &&
         (*(short *)(*_refptr_g_peaksPtr + (longlong)(int)(char)local_9 * 0x14 + 0x18) <
          (short)((sVar2 >> 3) * 5))) {
        Peak_Remove(local_9);
      }
    }
    sVar2 = EdgePeakFilter_GetMaxSig(2,_refptr_g_tsaPrmtFlash->bCols + 0xff);
    if (sVar2 < 0) {
      sVar2 = sVar2 + 7;
    }
    local_9 = (*_refptr_g_peaksPtr)->bPeakCount;
    while (local_9 = local_9 + 0xff, -1 < (char)local_9) {
      cVar1 = Peak_IsOnLine(2,_refptr_g_tsaPrmtFlash->bCols + 0xff,local_9);
      if ((cVar1 != '\0') &&
         (*(short *)(*_refptr_g_peaksPtr + (longlong)(int)(char)local_9 * 0x14 + 0x18) <
          (short)((sVar2 >> 3) * 5))) {
        Peak_Remove(local_9);
      }
    }
  }
  else {
    sVar2 = EdgePeakFilter_GetMaxSig(1,0);
    if (sVar2 < 0) {
      sVar2 = sVar2 + 7;
    }
    local_9 = (*_refptr_g_peaksPtr)->bPeakCount;
    while (local_9 = local_9 + 0xff, -1 < (char)local_9) {
      cVar1 = Peak_IsOnLine(1,0,local_9);
      if ((cVar1 != '\0') &&
         (*(short *)(*_refptr_g_peaksPtr + (longlong)(int)(char)local_9 * 0x14 + 0x18) <
          (short)((sVar2 >> 3) * 5))) {
        Peak_Remove(local_9);
      }
    }
    sVar2 = EdgePeakFilter_GetMaxSig(1,_refptr_g_tsaPrmtFlash->bRows + 0xff);
    if (sVar2 < 0) {
      sVar2 = sVar2 + 7;
    }
    local_9 = (*_refptr_g_peaksPtr)->bPeakCount;
    while (local_9 = local_9 + 0xff, -1 < (char)local_9) {
      cVar1 = Peak_IsOnLine(1,_refptr_g_tsaPrmtFlash->bRows + 0xff,local_9);
      if ((cVar1 != '\0') &&
         (*(short *)(*_refptr_g_peaksPtr + (longlong)(int)(char)local_9 * 0x14 + 0x18) <
          (short)((sVar2 >> 3) * 5))) {
        Peak_Remove(local_9);
      }
    }
  }
  return;
}


// ============================================================================
// Function: EdgePeakFilter_GetMaxSig @ 6bb43820
// ============================================================================

short EdgePeakFilter_GetMaxSig(int param_1,byte param_2)

{
  short sVar1;
  short local_10;
  byte local_e;
  byte local_d;
  ushort local_c;
  ushort local_a;
  
  local_10 = -0x8000;
  if (param_1 == 1) {
    local_a = (ushort)_refptr_g_tsaPrmtFlash->bCols * (ushort)param_2;
    local_d = 1;
    local_e = _refptr_g_tsaPrmtFlash->bCols;
  }
  else {
    local_a = (ushort)param_2;
    local_d = _refptr_g_tsaPrmtFlash->bCols;
    local_e = _refptr_g_tsaPrmtFlash->bRows;
  }
  for (local_c = 0; local_c < local_e; local_c = local_c + 1) {
    sVar1 = *(short *)(*(longlong *)_refptr_g_tsaBufDif +
                      (ulonglong)(ushort)(local_a + local_d * local_c) * 2);
    if (local_10 < sVar1) {
      local_10 = sVar1;
    }
  }
  return local_10;
}


