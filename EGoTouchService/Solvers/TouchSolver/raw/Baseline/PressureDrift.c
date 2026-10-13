// ============================================================================
// Function: PressureDrift_Detect @ 6bb441c9
// ============================================================================

undefined8 PressureDrift_Detect(byte param_1,byte param_2)

{
  short sVar1;
  char cVar2;
  short sVar3;
  int iVar4;
  undefined8 uVar5;
  byte local_11;
  int local_10;
  int local_c;
  
                    /* 0x1041c9  1039  PressureDrift_Detect */
  local_c = 0;
  local_10 = 0;
  sVar1 = *(short *)((ulonglong)
                     ((uint)param_1 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)
                     * 2 + *(longlong *)_refptr_g_tsaBufDif);
  iVar4 = (short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax * 3;
  if (iVar4 < 0) {
    iVar4 = iVar4 + 3;
  }
  sVar3 = (short)(iVar4 >> 2);
  if ((*_refptr_g_prevTouchesPtr)->bPrevTouchCount < 3) {
    cVar2 = PressureDrift_DetectByTimeStamp();
    if (cVar2 == '\0') {
      return 0;
    }
  }
  else {
    g_pressureDriftDebounce = 0;
  }
  if ((sVar3 < sVar1) || (sVar1 < sVar3 / 2)) {
    uVar5 = 0;
  }
  else {
    for (local_11 = 1; (int)(uint)local_11 < (int)(_refptr_g_tsaPrmtFlash->bCols - 1);
        local_11 = local_11 + 1) {
      iVar4 = PressureDrift_GradientCalUnit(local_11,param_2);
      if ((short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax / 3 < iVar4) {
        return 0;
      }
      local_c = local_c + iVar4;
      if (0 < *(short *)((ulonglong)
                         ((uint)local_11 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols &
                         0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif)) {
        local_10 = local_10 +
                   *(short *)((ulonglong)
                              ((uint)local_11 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols
                              & 0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif);
      }
    }
    if ((local_10 < (sVar1 * 9) / 2) || (sVar1 * 6 < local_c)) {
      uVar5 = 0;
    }
    else {
      g_lastPressureDriftDetectedTimeStamp = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
      uVar5 = 1;
    }
  }
  return uVar5;
}


// ============================================================================
// Function: PressureDrift_GradientCalUnit @ 6bb440d0
// ============================================================================

int PressureDrift_GradientCalUnit(byte param_1,byte param_2)

{
  int local_28 [6];
  byte local_d;
  uint local_c;
  
  local_c = 0;
  local_28[0] = -1;
  local_28[1] = 0;
  local_28[2] = 1;
  for (local_d = 0; local_d < 3; local_d = local_d + 1) {
    local_c = local_c + local_28[(int)(uint)local_d] *
                        (int)*(short *)((ulonglong)
                                        (((uint)_refptr_g_tsaPrmtFlash->bCols * (uint)param_2 +
                                         (uint)local_d + (uint)param_1) - 1 & 0xffff) * 2 +
                                       *(longlong *)_refptr_g_tsaBufDif);
  }
  return ((int)local_c >> 0x1f ^ local_c) - ((int)local_c >> 0x1f);
}


// ============================================================================
// Function: PressureDrift_DetectByTimeStamp @ 6bb44172
// ============================================================================

undefined8 PressureDrift_DetectByTimeStamp(void)

{
  undefined8 uVar1;
  
  if ((ulonglong)
      ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - g_lastPressureDriftDetectedTimeStamp)
      < 0x19) {
    g_pressureDriftDebounce = g_pressureDriftDebounce + 1;
    if (g_pressureDriftDebounce < 0x33) {
      uVar1 = 1;
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


