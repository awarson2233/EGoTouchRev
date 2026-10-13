// ============================================================================
// Function: TouchZone_InnerCornerHandler @ 6ba88419
// ============================================================================

void TouchZone_InnerCornerHandler(longlong param_1,byte param_2,byte param_3)

{
  short sVar1;
  undefined1 uVar2;
  short local_c;
  short local_a;
  
                    /* 0x48419  2513  TouchZone_InnerCornerHandler */
  sVar1 = *(short *)((ulonglong)
                     ((uint)param_2 + (uint)param_3 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)
                     * 2 + *(longlong *)_refptr_g_tsaBufDif);
  local_a = 0;
  local_c = 0;
  if ((param_2 == 1) && (param_3 == 1)) {
    local_a = *(short *)(*(longlong *)_refptr_g_tsaBufDif + 2);
    local_c = *(short *)((ulonglong)_refptr_g_tsaPrmtFlash->bCols * 2 +
                        *(longlong *)_refptr_g_tsaBufDif);
  }
  else if (((uint)param_2 == _refptr_g_tsaPrmtFlash->bCols - 2) && (param_3 == 1)) {
    local_a = *(short *)((ulonglong)param_2 * 2 + *(longlong *)_refptr_g_tsaBufDif);
    local_c = *(short *)((ulonglong)((uint)param_2 + (uint)_refptr_g_tsaPrmtFlash->bCols + 1) * 2 +
                        *(longlong *)_refptr_g_tsaBufDif);
  }
  else if ((param_2 == 1) && ((uint)param_3 == _refptr_g_tsaPrmtFlash->bRows - 2)) {
    local_a = *(short *)((ulonglong)
                         ((param_3 + 1) * (uint)_refptr_g_tsaPrmtFlash->bCols + 1 & 0xffff) * 2 +
                        *(longlong *)_refptr_g_tsaBufDif);
    local_c = *(short *)((ulonglong)((uint)param_3 * (uint)_refptr_g_tsaPrmtFlash->bCols) * 2 +
                        *(longlong *)_refptr_g_tsaBufDif);
  }
  else if (((uint)param_2 == _refptr_g_tsaPrmtFlash->bCols - 2) &&
          ((uint)param_3 == _refptr_g_tsaPrmtFlash->bRows - 2)) {
    local_a = *(short *)((ulonglong)
                         ((uint)param_2 + (param_3 + 1) * (uint)_refptr_g_tsaPrmtFlash->bCols &
                         0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif);
    local_c = *(short *)((ulonglong)
                         ((uint)param_2 + (uint)param_3 * (uint)_refptr_g_tsaPrmtFlash->bCols + 1 &
                         0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif);
  }
  if (((0 < local_a) && (0 < local_c)) && (0 < sVar1)) {
    uVar2 = TouchZone_GetInnerCornerSigRatio((int)sVar1,(int)local_a,(int)local_c);
    *(undefined1 *)(param_1 + 0x36c) = uVar2;
  }
  return;
}


// ============================================================================
// Function: TouchZone_GetInnerCornerSigRatio @ 6ba883a8
// ============================================================================

int TouchZone_GetInnerCornerSigRatio(short param_1,short param_2,short param_3)

{
  int local_10;
  short local_a;
  
                    /* 0x483a8  2512  TouchZone_GetInnerCornerSigRatio */
  if (param_1 == 0) {
    local_10 = 0;
  }
  else {
    local_a = param_2;
    if (param_3 < param_2) {
      local_a = param_3;
    }
    local_10 = ((int)local_a << 4) / (int)param_1;
    if (0xff < local_10) {
      local_10 = 0xff;
    }
  }
  return local_10;
}


