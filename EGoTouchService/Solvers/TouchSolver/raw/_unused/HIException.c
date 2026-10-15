// ============================================================================
// Function: HIException_CheckChargerNoiseInRxLines @ 6ba5b9d1
// ============================================================================

void HIException_CheckChargerNoiseInRxLines(void)

{
  char cVar1;
  uint uVar2;
  undefined4 local_28;
  undefined2 local_24;
  undefined4 local_18;
  undefined2 local_14;
  byte local_12;
  byte local_11;
  byte local_10;
  undefined1 local_f;
  short local_e;
  short local_c;
  char local_9;
  
                    /* 0x1b9d1  798  HIException_CheckChargerNoiseInRxLines */
  local_9 = '\0';
  local_e = 0;
  cVar1 = TSAStatic_IsChargerPlugged();
  if (((cVar1 != '\0') || (cVar1 = AFE_IsManualFreqHopping(), cVar1 != '\0')) ||
     (cVar1 = TSAStatic_IsCouplingNoiseEstimateNeeded(), cVar1 != '\0')) {
    HIException_GetChargerNoiseThold();
    if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
      local_f = _refptr_g_tsaPrpt->field_0x238;
    }
    else {
      local_f = _refptr_g_tsaPrpt->field_0x25c;
    }
    local_10 = HIException_GetMinDifRxInSpecificTxLine(local_f);
    local_11 = HIException_GetMinDifTxInSpecificRxLine(local_10);
    local_12 = HIException_GetMaxDifTxInSpecificRxLine(local_10);
    if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\x01') {
      uVar2 = (uint)local_10 + (uint)local_11 * (uint)_refptr_g_tsaPrmtFlash->bCols;
    }
    else {
      uVar2 = (uint)local_11 + (uint)local_10 * (uint)_refptr_g_tsaPrmtFlash->bCols;
    }
    if ((int)*(short *)&(*_refptr_g_tsaStaticPtr)[1].field_0x36 <
        -(int)*(short *)((ulonglong)(uVar2 & 0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif)) {
      if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\x01') {
        uVar2 = (uint)local_10 + (uint)local_12 * (uint)_refptr_g_tsaPrmtFlash->bCols;
      }
      else {
        uVar2 = (uint)local_12 + (uint)local_10 * (uint)_refptr_g_tsaPrmtFlash->bCols;
      }
      if (*(short *)(_refptr_g_tsaPrmtDynamic + 0xc) <
          *(short *)((ulonglong)(uVar2 & 0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif)) {
        if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\x01') {
          uVar2 = (uint)local_10 + (uint)local_11 * (uint)_refptr_g_tsaPrmtFlash->bCols;
        }
        else {
          uVar2 = (uint)local_11 + (uint)local_10 * (uint)_refptr_g_tsaPrmtFlash->bCols;
        }
        local_c = (short)(-(int)*(short *)((ulonglong)(uVar2 & 0xffff) * 2 +
                                          *(longlong *)_refptr_g_tsaBufDif) / 2);
        if (*(short *)(_refptr_g_tsaPrmtDynamic + 0xc) < local_c) {
          local_c = *(short *)(_refptr_g_tsaPrmtDynamic + 0xc);
        }
        if (local_c < *(short *)&_refptr_g_tsaPrpt[1].field_0x14) {
          local_c = *(short *)&_refptr_g_tsaPrpt[1].field_0x14;
        }
        HIException_ChargerGetRxLinePeakNum((int)local_c,local_10,local_11,&local_18);
        cVar1 = TSAStatic_IsChargerPluggedNoDebounce();
        if (cVar1 == '\0') {
          local_28 = local_18;
          local_24 = local_14;
          cVar1 = HIException_CheckRxLinePeaks(&local_28);
          if ((cVar1 != '\0') && (((*_refptr_g_prevTouchesPtr)->nTouchCount & 0xc0U) == 0)) {
            local_9 = 1;
            local_9 = HIException_CheckRaw2RawNoiseInSpecificRxLine(local_10);
          }
        }
        else if (((char)local_18 == '\0') || (local_18._1_1_ == '\0')) {
          local_9 = '\0';
        }
        else {
          local_9 = '\x01';
        }
        if (local_9 == '\x01') {
          cVar1 = TSAStatic_IsChargerPlugged();
          if (cVar1 == '\0') {
            cVar1 = TSAStatic_IsCouplingNoiseEstimateNeeded();
            if (cVar1 != '\0') {
              (*_refptr_g_tsaStaticPtr)->bField_0x111 = '\x03';
              *(undefined4 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x32 = 0x8000;
              *(short *)&(*_refptr_g_tsaStaticPtr)[1].field_0x38 = -local_e;
            }
          }
          else {
            (*_refptr_g_tsaStaticPtr)->bField_0x111 = '\x02';
            *(undefined4 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x32 = 0x100;
            *(short *)&(*_refptr_g_tsaStaticPtr)[1].field_0x38 = -local_e;
          }
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: HIException_IsChargerNoiseDetected @ 6ba5bd3a
// ============================================================================

undefined4 HIException_IsChargerNoiseDetected(void)

{
                    /* 0x1bd3a  802  HIException_IsChargerNoiseDetected */
  return CONCAT31((int3)((uint)*(int *)&(*_refptr_g_tsaStaticPtr)[1].field_0x32 >> 8),
                  *(int *)&(*_refptr_g_tsaStaticPtr)[1].field_0x32 == 0x100);
}


// ============================================================================
// Function: HIException_CheckPanelSD @ 6ba5a3ff
// ============================================================================

void HIException_CheckPanelSD(void)

{
  char cVar1;
  
                    /* 0x1a3ff  799  HIException_CheckPanelSD */
  cVar1 = HIException_IsNegSigSatisfySDCriteria
                    (_refptr_g_tsaPrpt->bNextSideThreshCol,_refptr_g_tsaPrpt->field_0xa7);
  (*_refptr_g_tsaStaticPtr)->field_0x113 = cVar1 != '\0';
  return;
}


// ============================================================================
// Function: HIException_ChargerGetRxLinePeakNum @ 6ba5ab84
// ============================================================================

void HIException_ChargerGetRxLinePeakNum
               (short param_1,byte param_2,undefined8 param_3,char *param_4)

{
  byte bVar1;
  int iVar2;
  byte bVar3;
  byte local_15;
  undefined4 local_14;
  int local_10;
  byte local_c;
  byte local_b;
  short local_a;
  
  local_14 = 0;
  local_15 = 0;
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    local_c = _refptr_g_tsaPrmtFlash->bCols;
    if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) != 0) {
      local_15 = 1;
    }
  }
  else {
    local_c = _refptr_g_tsaPrmtFlash->bRows;
  }
  param_4[4] = '\0';
  param_4[5] = '\0';
  *param_4 = '\0';
  param_4[1] = '\0';
  param_4[3] = '\0';
  param_4[2] = '\0';
  for (local_b = local_15; (int)(uint)local_b < (int)((uint)local_c - (uint)local_15);
      local_b = local_b + 1) {
    if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
      local_a = *(short *)((ulonglong)
                           ((uint)local_b + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols &
                           0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif);
      local_10 = *(int *)(_refptr_g_tsaBufSelf + (longlong)(int)(uint)local_b * 4);
    }
    else {
      local_a = *(short *)((ulonglong)
                           ((uint)param_2 + (uint)local_b * (uint)_refptr_g_tsaPrmtFlash->bCols &
                           0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif);
      local_10 = *(int *)(_refptr_g_tsaBufSelf +
                         (longlong)(int)((uint)local_b + (uint)_refptr_g_tsaPrmtFlash->bCols) * 4);
    }
    if (-1 < local_a * local_10) {
      iVar2 = HIException_GetNextPeakState(local_14,(int)local_a,(int)param_1,param_2,local_b);
      if (iVar2 == 2) {
        param_4[1] = param_4[1] + '\x01';
        local_14 = 2;
        bVar1 = HIException_ChargerGetTxWidth(param_2,local_b);
        bVar3 = param_4[3];
        if ((byte)param_4[3] < bVar1) {
          bVar3 = bVar1;
        }
        param_4[3] = bVar3;
      }
      else if (iVar2 == 1) {
        *param_4 = *param_4 + '\x01';
        *(short *)(param_4 + 4) = local_a;
        local_14 = 1;
        bVar1 = HIException_ChargerGetTxWidth(param_2,local_b);
        bVar3 = param_4[2];
        if ((byte)param_4[2] < bVar1) {
          bVar3 = bVar1;
        }
        param_4[2] = bVar3;
      }
    }
  }
  return;
}


// ============================================================================
// Function: HIException_GetMinDifRxInSpecificTxLine @ 6ba5b16d
// ============================================================================

byte HIException_GetMinDifRxInSpecificTxLine(byte param_1)

{
  byte local_f;
  short local_e;
  byte local_c;
  byte local_b;
  short local_a;
  
  local_e = 0x7fff;
  local_f = 0;
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    local_c = _refptr_g_tsaPrmtFlash->bRows;
  }
  else {
    local_c = _refptr_g_tsaPrmtFlash->bCols;
  }
  for (local_b = 0; local_b < local_c; local_b = local_b + 1) {
    if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
      local_a = *(short *)((ulonglong)
                           ((uint)param_1 + (uint)local_b * (uint)_refptr_g_tsaPrmtFlash->bCols &
                           0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif);
    }
    else {
      local_a = *(short *)((ulonglong)
                           ((uint)local_b + (uint)param_1 * (uint)_refptr_g_tsaPrmtFlash->bCols &
                           0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif);
    }
    if (local_a < local_e) {
      local_e = local_a;
      local_f = local_b;
    }
  }
  return local_f;
}


// ============================================================================
// Function: HIException_GetMaxDifTxInSpecificRxLine @ 6ba5b26f
// ============================================================================

byte HIException_GetMaxDifTxInSpecificRxLine(byte param_1)

{
  byte local_f;
  short local_e;
  byte local_c;
  byte local_b;
  short local_a;
  
  local_e = -0x8000;
  local_f = 0;
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    local_c = _refptr_g_tsaPrmtFlash->bCols;
  }
  else {
    local_c = _refptr_g_tsaPrmtFlash->bRows;
  }
  for (local_b = 0; local_b < local_c; local_b = local_b + 1) {
    if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
      local_a = *(short *)((ulonglong)
                           ((uint)local_b + (uint)param_1 * (uint)_refptr_g_tsaPrmtFlash->bCols &
                           0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif);
    }
    else {
      local_a = *(short *)((ulonglong)
                           ((uint)param_1 + (uint)local_b * (uint)_refptr_g_tsaPrmtFlash->bCols &
                           0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif);
    }
    if (local_e < local_a) {
      local_e = local_a;
      local_f = local_b;
    }
  }
  return local_f;
}


// ============================================================================
// Function: HIException_GetMinDifTxInSpecificRxLine @ 6ba5b371
// ============================================================================

byte HIException_GetMinDifTxInSpecificRxLine(byte param_1)

{
  byte local_f;
  short local_e;
  byte local_c;
  byte local_b;
  short local_a;
  
  local_e = 0x7fff;
  local_f = 0;
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    local_c = _refptr_g_tsaPrmtFlash->bCols;
  }
  else {
    local_c = _refptr_g_tsaPrmtFlash->bRows;
  }
  for (local_b = 0; local_b < local_c; local_b = local_b + 1) {
    if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
      local_a = *(short *)((ulonglong)
                           ((uint)local_b + (uint)param_1 * (uint)_refptr_g_tsaPrmtFlash->bCols &
                           0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif);
    }
    else {
      local_a = *(short *)((ulonglong)
                           ((uint)param_1 + (uint)local_b * (uint)_refptr_g_tsaPrmtFlash->bCols &
                           0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif);
    }
    if (local_a < local_e) {
      local_e = local_a;
      local_f = local_b;
    }
  }
  return local_f;
}


// ============================================================================
// Function: HIException_CheckRaw2RawNoiseInSpecificRxLine @ 6ba5b5b3
// ============================================================================

undefined8 HIException_CheckRaw2RawNoiseInSpecificRxLine(byte param_1)

{
  ulonglong uVar1;
  char cVar2;
  byte local_25;
  int local_24;
  short local_1e;
  ushort local_1c;
  byte local_19;
  int local_18;
  int local_14;
  undefined4 local_10;
  byte local_9;
  
  local_25 = 0;
  local_10 = 0;
  local_14 = 0;
  local_18 = 0;
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    local_19 = _refptr_g_tsaPrmtFlash->bCols;
  }
  else {
    local_19 = _refptr_g_tsaPrmtFlash->bRows;
  }
  for (local_9 = 0; local_9 < local_19; local_9 = local_9 + 1) {
    if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
      local_1c = *(ushort *)
                  ((ulonglong)
                   ((uint)local_9 + (uint)param_1 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) *
                   2 + *(longlong *)_refptr_g_tsaBufRaw2rawDifAftCMF);
    }
    else {
      local_1c = *(ushort *)
                  ((ulonglong)
                   ((uint)param_1 + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) *
                   2 + *(longlong *)_refptr_g_tsaBufRaw2rawDifAftCMF);
    }
    local_1e = (local_1c ^ (short)local_1c >> 0x1f) - ((short)local_1c >> 0xf);
    local_18 = local_18 + local_1e;
    if (local_14 < local_1e) {
      local_14 = (int)local_1e;
    }
    local_10 = HIException_UpdatePeakState(local_10,&local_25,(int)(short)local_1c);
  }
  if (local_14 == 0) {
    local_14 = 1;
  }
  if (((*(short *)&_refptr_g_tsaPrpt[1].field_0x14 < local_14) &&
      (uVar1 = (longlong)(local_18 * 100) / (longlong)(int)(uint)local_19,
      -10 < (int)((longlong)((ulonglong)(uint)((int)uVar1 >> 0x1f) << 0x20 | uVar1 & 0xffffffff) /
                 (longlong)local_14) + -0x40)) &&
     (uVar1 = (longlong)(local_18 * 100) / (longlong)(int)(uint)local_19,
     (int)((longlong)((ulonglong)(uint)((int)uVar1 >> 0x1f) << 0x20 | uVar1 & 0xffffffff) /
          (longlong)local_14) + -0x40 < 10)) {
    local_24 = 1;
  }
  else {
    local_24 = 0;
  }
  if ((local_25 < 6) &&
     ((local_25 < 3 ||
      ((local_24 == 0 && (cVar2 = TSAStatic_IsCouplingNoiseEstimateApplied(), cVar2 == '\0')))))) {
    return 0;
  }
  return 1;
}


// ============================================================================
// Function: HIException_GetChargerNoiseThold @ 6ba5b7df
// ============================================================================

void HIException_GetChargerNoiseThold(void)

{
  char cVar1;
  
  *(short *)&(*_refptr_g_tsaStaticPtr)[1].field_0x36 =
       (short)(((int)*(short *)_refptr_g_tsaPrmtDynamic *
               (int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x8c)) / 100);
  if (*(short *)&(*_refptr_g_tsaStaticPtr)[1].field_0x36 <
      *(short *)&_refptr_g_tsaPrpt[1].field_0x14) {
    *(undefined2 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x36 =
         *(undefined2 *)&_refptr_g_tsaPrpt[1].field_0x14;
  }
  cVar1 = TSAStatic_IsCouplingNoiseEstimateNeeded();
  if (cVar1 != '\0') {
    *(short *)&(*_refptr_g_tsaStaticPtr)[1].field_0x3a =
         (short)(((int)*(short *)_refptr_g_tsaPrmtDynamic *
                 (int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x94)) / 100);
    if (*(short *)&(*_refptr_g_tsaStaticPtr)[1].field_0x36 <
        *(short *)&(*_refptr_g_tsaStaticPtr)[1].field_0x3a) {
      *(undefined2 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x36 =
           *(undefined2 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x3a;
    }
  }
  return;
}


// ============================================================================
// Function: HIException_CheckRxLinePeaks @ 6ba5b916
// ============================================================================

undefined8 HIException_CheckRxLinePeaks(byte *param_1)

{
  bool bVar1;
  bool bVar2;
  char cVar3;
  undefined8 uVar4;
  
  if ((((param_1[3] < param_1[2]) || (*param_1 == 0)) || (param_1[1] == 0)) ||
     ((uint)param_1[1] + (uint)*param_1 < 3)) {
    bVar1 = false;
  }
  else {
    bVar1 = true;
  }
  if (((1 < *param_1) && (1 < param_1[1])) &&
     ((5 < (byte)_refptr_g_tsaPrpt[1].field_0x78 && (4 < (uint)param_1[1] + (uint)*param_1)))) {
    cVar3 = TSAStatic_IsCouplingNoiseEstimateNeeded();
    if (cVar3 != '\0') {
      bVar2 = true;
      goto LAB_6ba5b9af;
    }
  }
  bVar2 = false;
LAB_6ba5b9af:
  if ((bVar1) || (bVar2)) {
    uVar4 = 1;
  }
  else {
    uVar4 = 0;
  }
  return uVar4;
}


// ============================================================================
// Function: HIException_IsNegSigSatisfySDCriteria @ 6ba5a243
// ============================================================================

undefined8 HIException_IsNegSigSatisfySDCriteria(byte param_1,byte param_2)

{
  bool bVar1;
  bool bVar2;
  undefined8 uVar3;
  byte local_c;
  byte local_b;
  
  bVar1 = false;
  bVar2 = false;
  local_c = 0;
  if (-(int)_refptr_g_tsaPrpt->nSPeakNegative < (int)_refptr_g_tsaPrpt->nSPeakPositive) {
    for (local_b = 0; local_b < (*_refptr_g_peaksPtr)->bPeakCount; local_b = local_b + 1) {
      if ((-2 < (int)((uint)(*_refptr_g_peaksPtr)[(longlong)(int)(uint)local_b * 0x14 + 0x15].
                            bPeakCount - (uint)param_1)) &&
         ((int)((uint)(*_refptr_g_peaksPtr)[(longlong)(int)(uint)local_b * 0x14 + 0x15].bPeakCount -
               (uint)param_1) < 2)) {
        bVar1 = true;
        local_c = local_b;
        break;
      }
    }
    for (local_b = 0; local_b < (*_refptr_g_peaksPtr)->bPeakCount; local_b = local_b + 1) {
      if (((local_b != local_c) &&
          (-2 < (int)((uint)(*_refptr_g_peaksPtr)[(longlong)(int)(uint)local_b * 0x14 + 0x16].
                            bPeakCount - (uint)param_2))) &&
         ((int)((uint)(*_refptr_g_peaksPtr)[(longlong)(int)(uint)local_b * 0x14 + 0x16].bPeakCount -
               (uint)param_2) < 2)) {
        bVar2 = true;
        break;
      }
    }
  }
  if ((bVar1) && (bVar2)) {
    uVar3 = 1;
  }
  else {
    uVar3 = 0;
  }
  return uVar3;
}


// ============================================================================
// Function: HIException_ChargerGetTxWidth @ 6ba5a6d3
// ============================================================================

char HIException_ChargerGetTxWidth(byte param_1,byte param_2)

{
  short sVar1;
  uint uVar2;
  byte local_d;
  short local_c;
  char local_a;
  byte local_9;
  
  local_a = '\0';
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\x01') {
    uVar2 = (uint)param_1 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols;
  }
  else {
    uVar2 = (uint)param_2 + (uint)param_1 * (uint)_refptr_g_tsaPrmtFlash->bCols;
  }
  sVar1 = *(short *)((ulonglong)(uVar2 & 0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif);
  local_c = sVar1 / 2;
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    local_d = _refptr_g_tsaPrmtFlash->bRows;
  }
  else {
    local_d = _refptr_g_tsaPrmtFlash->bCols;
  }
  if (sVar1 < 1) {
    if (-(int)local_c < (int)*(short *)&(*_refptr_g_tsaStaticPtr)[1].field_0x36) {
      local_c = -*(short *)&(*_refptr_g_tsaStaticPtr)[1].field_0x36;
    }
    for (local_9 = 0; local_9 < local_d; local_9 = local_9 + 1) {
      if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\x01') {
        uVar2 = (uint)local_9 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols;
      }
      else {
        uVar2 = (uint)param_2 + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols;
      }
      if (*(short *)((ulonglong)(uVar2 & 0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif) < local_c)
      {
        local_a = local_a + '\x01';
      }
    }
  }
  else {
    if (local_c < *(short *)&(*_refptr_g_tsaStaticPtr)[1].field_0x36) {
      local_c = *(short *)&(*_refptr_g_tsaStaticPtr)[1].field_0x36;
    }
    for (local_9 = 0; local_9 < local_d; local_9 = local_9 + 1) {
      if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\x01') {
        uVar2 = (uint)local_9 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols;
      }
      else {
        uVar2 = (uint)param_2 + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols;
      }
      if (local_c < *(short *)((ulonglong)(uVar2 & 0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif))
      {
        local_a = local_a + '\x01';
      }
    }
  }
  return local_a;
}


// ============================================================================
// Function: HIException_GetNextPeakState @ 6ba5aa93
// ============================================================================

undefined4
HIException_GetNextPeakState(int param_1,short param_2,short param_3,char param_4,char param_5)

{
  char cVar1;
  undefined4 local_c;
  
  local_c = 0;
  if (param_1 == 0) {
    if (param_3 < param_2) {
      local_c = 2;
    }
    else if ((int)param_3 < -(int)param_2) {
      cVar1 = HIException_ChargerCheckWholeTx(param_4,param_5);
      if (cVar1 != '\0') {
        cVar1 = HIException_ChargerCheckNeighbourTx((int)param_4,(int)param_5);
        if (cVar1 != '\0') {
          local_c = 1;
        }
      }
    }
  }
  else if (param_1 == 1) {
    if (param_3 < param_2) {
      local_c = 2;
    }
  }
  else if ((int)param_3 < -(int)param_2) {
    cVar1 = HIException_ChargerCheckWholeTx(param_4,param_5);
    if (cVar1 != '\0') {
      cVar1 = HIException_ChargerCheckNeighbourTx((int)param_4,(int)param_5);
      if (cVar1 != '\0') {
        local_c = 1;
      }
    }
  }
  return local_c;
}


// ============================================================================
// Function: HIException_UpdatePeakState @ 6ba5b473
// ============================================================================

int HIException_UpdatePeakState(int param_1,char *param_2,short param_3)

{
  int local_res8;
  
  local_res8 = param_1;
  if (param_1 == 1) {
    if ((int)*(short *)&_refptr_g_tsaPrpt[1].field_0x14 < -(int)param_3) {
      local_res8 = 2;
      *param_2 = *param_2 + '\x01';
    }
    else if (param_3 < *(short *)&_refptr_g_tsaPrpt[1].field_0x14) {
      local_res8 = 0;
    }
  }
  else if (param_1 == 0) {
    if (*(short *)&_refptr_g_tsaPrpt[1].field_0x14 < param_3) {
      local_res8 = 1;
      *param_2 = *param_2 + '\x01';
    }
    else if ((int)*(short *)&_refptr_g_tsaPrpt[1].field_0x14 < -(int)param_3) {
      local_res8 = 2;
      *param_2 = *param_2 + '\x01';
    }
  }
  else if (param_1 == 2) {
    if (*(short *)&_refptr_g_tsaPrpt[1].field_0x14 < param_3) {
      local_res8 = 1;
      *param_2 = *param_2 + '\x01';
    }
    else if (-(int)param_3 < (int)*(short *)&_refptr_g_tsaPrpt[1].field_0x14) {
      local_res8 = 0;
    }
  }
  return local_res8;
}


// ============================================================================
// Function: HIException_Init @ 6ba5bf39
// ============================================================================

void HIException_Init(void)

{
                    /* 0x1bf39  801  HIException_Init */
  *(undefined2 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x1a = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x22 = 0;
  *(undefined4 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x2a = 0;
  (*_refptr_g_tsaStaticPtr)[1].field_0x2e = 0;
  *(undefined4 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x32 = 0;
  return;
}


// ============================================================================
// Function: HIException_ChargerCheckWholeTx @ 6ba5a4f5
// ============================================================================

undefined1 HIException_ChargerCheckWholeTx(byte param_1,byte param_2)

{
  short local_c;
  byte local_9;
  
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    local_c = (short)(-(int)*(short *)((ulonglong)
                                       ((uint)param_2 +
                                        (uint)param_1 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff
                                       ) * 2 + *(longlong *)_refptr_g_tsaBufDif) / 2);
    if (local_c < *(short *)&(*_refptr_g_tsaStaticPtr)[1].field_0x36) {
      local_c = *(short *)&(*_refptr_g_tsaStaticPtr)[1].field_0x36;
    }
    for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bRows; local_9 = local_9 + 1) {
      if (local_c < *(short *)((ulonglong)
                               ((uint)param_2 + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols
                               & 0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif)) {
        return 0;
      }
    }
  }
  else {
    local_c = (short)(-(int)*(short *)((ulonglong)
                                       ((uint)param_1 +
                                        (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff
                                       ) * 2 + *(longlong *)_refptr_g_tsaBufDif) / 2);
    if (local_c < *(short *)&(*_refptr_g_tsaStaticPtr)[1].field_0x36) {
      local_c = *(short *)&(*_refptr_g_tsaStaticPtr)[1].field_0x36;
    }
    for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bCols; local_9 = local_9 + 1) {
      if (local_c < *(short *)((ulonglong)
                               ((uint)local_9 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols
                               & 0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif)) {
        return 0;
      }
    }
  }
  return 1;
}


// ============================================================================
// Function: HIException_ChargerCheckNeighbourTx @ 6ba5a93f
// ============================================================================

undefined1 HIException_ChargerCheckNeighbourTx(char param_1,byte param_2)

{
  bool bVar1;
  short local_c;
  byte local_9;
  
  local_c = 0;
  local_9 = param_1 - 1;
  while( true ) {
    if (param_1 + 1 < (int)(char)local_9) {
      return 1;
    }
    bVar1 = false;
    if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
      if ((param_2 < _refptr_g_tsaPrmtFlash->bCols) && (local_9 < _refptr_g_tsaPrmtFlash->bRows)) {
        local_c = *(short *)((ulonglong)
                             (ushort)((short)(char)param_2 +
                                     (short)(char)local_9 * (ushort)_refptr_g_tsaPrmtFlash->bCols) *
                             2 + *(longlong *)_refptr_g_tsaBufDif);
        bVar1 = true;
      }
    }
    else if ((local_9 < _refptr_g_tsaPrmtFlash->bCols) && (param_2 < _refptr_g_tsaPrmtFlash->bRows))
    {
      local_c = *(short *)((ulonglong)
                           (ushort)((short)(char)local_9 +
                                   (short)(char)param_2 * (ushort)_refptr_g_tsaPrmtFlash->bCols) * 2
                          + *(longlong *)_refptr_g_tsaBufDif);
      bVar1 = true;
    }
    if ((bVar1) && (-1 < local_c)) break;
    local_9 = local_9 + 1;
  }
  return 0;
}


// ============================================================================
// Function: HIException_GetDetectedChargerNoise @ 6ba5bd58
// ============================================================================

undefined2 HIException_GetDetectedChargerNoise(void)

{
                    /* 0x1bd58  800  HIException_GetDetectedChargerNoise */
  return *(undefined2 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x38;
}


// ============================================================================
// Function: HIException_ProcessTouchZone @ 6ba5bfeb
// ============================================================================

void HIException_ProcessTouchZone(short *param_1)

{
  char cVar1;
  uint uVar2;
  
                    /* 0x1bfeb  803  HIException_ProcessTouchZone */
  (*_refptr_g_tsaStaticPtr)[1].field_0x2e = 0;
  HIException_GetRatio(param_1);
  HIException_GetMaxPeakNumInTouchZone(param_1);
  if (*(int *)&(*_refptr_g_tsaStaticPtr)[1].field_0x32 == 0x100) {
    *(uint *)(param_1 + 0x1a8) = *(uint *)(param_1 + 0x1a8) | 0x100;
    param_1[0x1aa] = 0x100;
    param_1[0x1ab] = 0;
    *(undefined1 *)(param_1 + 0x1a4) = 1;
    HIException_Recording(*(undefined4 *)(param_1 + 0x1a8));
  }
  else if (*(int *)&(*_refptr_g_tsaStaticPtr)[1].field_0x32 == 0x8000) {
    *(uint *)(param_1 + 0x1a8) = *(uint *)(param_1 + 0x1a8) | 0x8000;
    param_1[0x1aa] = -0x8000;
    param_1[0x1ab] = 0;
    HIException_Recording(*(undefined4 *)(param_1 + 0x1a8));
  }
  else {
    cVar1 = TSAStatic_IsChargerPlugged();
    if (cVar1 == '\0') {
      if ((0x19 < (ushort)param_1[0x19e]) && ((int)param_1[0x16] < -(int)*param_1)) {
        cVar1 = HIException_CheckSDBasedOnMultiSignSensor(param_1);
        if (cVar1 == '\0') {
          cVar1 = HIException_CheckWater(param_1);
          if (cVar1 == '\0') {
            uVar2 = HIException_SDOrWetFinger(param_1);
            *(uint *)(param_1 + 0x1a8) = *(uint *)(param_1 + 0x1a8) | uVar2;
            *(uint *)(param_1 + 0x1aa) = uVar2;
          }
          else {
            (*_refptr_g_tsaStaticPtr)[1].field_0x2e = 1;
            *(uint *)(param_1 + 0x1a8) = *(uint *)(param_1 + 0x1a8) | 0x40;
            param_1[0x1aa] = 0x40;
            param_1[0x1ab] = 0;
          }
        }
        else {
          (*_refptr_g_tsaStaticPtr)[1].field_0x2e = 1;
          cVar1 = HIException_CheckWater(param_1);
          if (cVar1 == '\0') {
            *(uint *)(param_1 + 0x1a8) = *(uint *)(param_1 + 0x1a8) | 0x80;
            param_1[0x1aa] = 0x80;
            param_1[0x1ab] = 0;
          }
          else {
            *(uint *)(param_1 + 0x1a8) = *(uint *)(param_1 + 0x1a8) | 0x40;
            param_1[0x1aa] = 0x40;
            param_1[0x1ab] = 0;
          }
        }
      }
      HIException_DebounceTouchZone(param_1);
      HIException_Recording(*(undefined4 *)(param_1 + 0x1a8));
    }
    else {
      HIException_Recording(*(undefined4 *)(param_1 + 0x1a8));
    }
  }
  return;
}


// ============================================================================
// Function: HIException_CheckWater @ 6ba59e68
// ============================================================================

undefined1 HIException_CheckWater(undefined8 param_1)

{
  char cVar1;
  undefined1 local_9;
  
  local_9 = 0;
  cVar1 = HIException_CheckSmallWater(param_1);
  if (cVar1 == '\0') {
    cVar1 = HIException_CheckNormalOrBigWater(param_1);
    if (cVar1 != '\0') {
      local_9 = 1;
    }
  }
  else {
    local_9 = 1;
  }
  return local_9;
}


// ============================================================================
// Function: HIException_SDOrWetFinger @ 6ba5a0c6
// ============================================================================

undefined8 HIException_SDOrWetFinger(longlong param_1)

{
  short sVar1;
  char cVar2;
  undefined8 uVar3;
  uint local_c;
  
  cVar2 = PosnSpecial_GetPosMarker();
  sVar1 = *(short *)(*_refptr_g_peaksPtr +
                    (longlong)(int)(uint)*(byte *)(param_1 + 0x2b6) * 0x14 + 0x18);
  local_c = 0;
  if (*(char *)(param_1 + 0x324) != '\x14') {
    local_c = *(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)*(byte *)(param_1 + 0x324) * 0x478 + 0x1f0);
  }
  if ((local_c & 0x200000) == 0) {
    if (cVar2 == '\x01') {
      if ((*(short *)(param_1 + 0x35a) == 0) || (*(ushort *)(param_1 + 0xc) < 5)) {
        cVar2 = HIException_CheckTouchZoneSD(param_1);
        if (cVar2 == '\0') {
          if ((*(short *)(_refptr_g_tsaPrmtDynamic + 0x90) < sVar1) ||
             (200 < *(ushort *)(param_1 + 0x340))) {
            uVar3 = 0x80;
          }
          else {
            uVar3 = 2;
          }
        }
        else {
          uVar3 = 8;
        }
      }
      else {
        uVar3 = 0x80;
      }
    }
    else if ((sVar1 / 2 < *(short *)(_refptr_g_list + 0x38)) &&
            (sVar1 / 2 < *(short *)(_refptr_g_list + 0x20))) {
      uVar3 = 8;
    }
    else {
      uVar3 = 0x80;
    }
  }
  else {
    (*_refptr_g_tsaStaticPtr)[1].field_0x2e = 1;
    uVar3 = 0x80;
  }
  return uVar3;
}


// ============================================================================
// Function: HIException_GetMaxPeakNumInTouchZone @ 6ba5b0af
// ============================================================================

void HIException_GetMaxPeakNumInTouchZone(longlong param_1)

{
  byte bVar1;
  byte local_9;
  
  local_9 = 0;
  bVar1 = HIException_GetMaxPeakNumInTouchZoneDim(param_1,1);
  if (bVar1 != 0) {
    local_9 = bVar1;
  }
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    *(byte *)(param_1 + 0x35d) = local_9 - 1;
  }
  else {
    *(byte *)(param_1 + 0x35c) = local_9 - 1;
  }
  bVar1 = HIException_GetMaxPeakNumInTouchZoneDim(param_1,2);
  if (local_9 < bVar1) {
    local_9 = bVar1;
  }
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    *(byte *)(param_1 + 0x35c) = local_9 - 1;
  }
  else {
    *(byte *)(param_1 + 0x35d) = local_9 - 1;
  }
  return;
}


// ============================================================================
// Function: HIException_Recording @ 6ba5bd6f
// ============================================================================

void HIException_Recording(uint param_1)

{
  if ((*(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x32 < param_1) && (param_1 != 2)) {
    *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x32 = param_1;
  }
  return;
}


// ============================================================================
// Function: HIException_DebounceTouchZone @ 6ba5bda6
// ============================================================================

void HIException_DebounceTouchZone(longlong param_1)

{
  int iVar1;
  int local_10;
  int local_c;
  
  local_c = 0;
  iVar1 = *(int *)(param_1 + 0x350);
  local_10 = *(int *)(param_1 + 0x350);
  if (*(char *)(param_1 + 0x324) != '\x14') {
    local_c = *(int *)((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)*(byte *)(param_1 + 0x324) * 0x478 + 0x1ec);
  }
  if (((*_refptr_g_prevTouchesPtr)->nTouchCount & 0xc0U) == 0) {
    if ((*_refptr_g_prevTouchesPtr)->nTouchCount == 8) {
      if (((*_refptr_g_tsaStaticPtr)[1].field_0x2e == '\0') ||
         (2 < (byte)(*_refptr_g_prevTouchesPtr)->field_0x5)) {
        local_10 = 8;
      }
      else {
        (*_refptr_g_prevTouchesPtr)->nTouchCount = iVar1;
        local_10 = iVar1;
      }
    }
    else if (iVar1 == 2) {
      if ((local_c == 0x40) || (local_c == 0x80)) {
        local_10 = 0x80;
      }
      else if (local_c == 8) {
        local_10 = 8;
      }
    }
  }
  else if ((iVar1 == 0) && (local_c == 0)) {
    local_10 = 0;
  }
  else {
    local_10 = (*_refptr_g_prevTouchesPtr)->nTouchCount;
  }
  if ((4 < (byte)(*_refptr_g_prevTouchesPtr)->field_0x5) && (local_10 != 0)) {
    local_10 = 8;
  }
  if ((local_10 == 0x40) || (local_10 == 0x80)) {
    iVar1 = HIException_WaterOrWetFinger(param_1);
    if (iVar1 == 0x40) {
      local_10 = 0x40;
    }
    else {
      local_10 = 0x80;
    }
  }
  *(int *)(param_1 + 0x350) = local_10;
  return;
}


// ============================================================================
// Function: HIException_GetRatio @ 6ba5bfa0
// ============================================================================

void HIException_GetRatio(undefined8 param_1)

{
  TZ_GetExceptionRatio(param_1);
  return;
}


// ============================================================================
// Function: HIException_CheckSDBasedOnMultiSignSensor @ 6ba5bfbc
// ============================================================================

bool HIException_CheckSDBasedOnMultiSignSensor(longlong param_1)

{
  return 4 < *(ushort *)(param_1 + 0x358);
}


// ============================================================================
// Function: HIException_CheckNormalOrBigWater @ 6ba59d00
// ============================================================================

undefined1 HIException_CheckNormalOrBigWater(longlong param_1)

{
  undefined1 local_9;
  
  local_9 = 0;
  if ((*(ushort *)(param_1 + 0x344) < 0x96) && (*(ushort *)(param_1 + 0x346) < 0x96)) {
    local_9 = 1;
  }
  return local_9;
}


// ============================================================================
// Function: HIException_CheckSmallWater @ 6ba59d40
// ============================================================================

bool HIException_CheckSmallWater(longlong param_1)

{
  ushort uVar1;
  bool local_9;
  
  local_9 = false;
  if ((((uint)*(ushort *)(param_1 + 0xc) + (uint)*(ushort *)(param_1 + 0xe) < 8) &&
      (-(int)*(short *)(param_1 + 0x2c) <
       (int)((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 2))) &&
     (*(short *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)*(byte *)(param_1 + 0x2b6) * 0x14 + 0x18
                ) < (short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 2)) {
    local_9 = *(short *)(param_1 + 0x358) != 0;
    uVar1 = _refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold;
    if ((short)uVar1 < 0) {
      uVar1 = uVar1 + 3;
    }
    if (*(short *)(*_refptr_g_peaksPtr +
                  (longlong)(int)(uint)*(byte *)(param_1 + 0x2b6) * 0x14 + 0x18) < (short)uVar1 >> 2
       ) {
      local_9 = true;
    }
  }
  return local_9;
}


// ============================================================================
// Function: HIException_CheckTouchZoneSD @ 6ba5a456
// ============================================================================

undefined1 HIException_CheckTouchZoneSD(longlong param_1)

{
  char cVar1;
  undefined1 local_9;
  
  local_9 = 1;
  if ((*(byte *)(param_1 + 0x2b2) < *(byte *)(param_1 + 0x35c)) ||
     (*(byte *)(param_1 + 0x2b2) < *(byte *)(param_1 + 0x35d))) {
    local_9 = 0;
  }
  cVar1 = HIException_IsNegSigSatisfySDCriteria
                    (*(undefined1 *)(param_1 + 0x30),*(undefined1 *)(param_1 + 0x31));
  if (cVar1 == '\0') {
    local_9 = 0;
  }
  else {
    *(undefined1 *)(param_1 + 0x34a) = 1;
  }
  *(undefined1 *)(param_1 + 0x349) = (*_refptr_g_tsaStaticPtr)->field_0x113;
  return local_9;
}


// ============================================================================
// Function: HIException_GetMaxPeakNumInTouchZoneDim @ 6ba5afd0
// ============================================================================

byte HIException_GetMaxPeakNumInTouchZoneDim(longlong param_1,int param_2)

{
  byte local_10;
  byte local_f;
  byte local_e;
  byte local_d;
  byte local_c;
  byte local_b;
  byte local_a;
  byte local_9;
  
  local_c = 0;
  local_d = 0;
  local_e = 0;
  if (param_2 == 1) {
    local_9 = *(byte *)(param_1 + 0x35);
    local_a = *(byte *)(param_1 + 0x36);
  }
  else {
    local_9 = *(byte *)(param_1 + 0x33);
    local_a = *(byte *)(param_1 + 0x34);
  }
  for (local_b = local_9; local_b <= local_a; local_b = local_b + 1) {
    HIException_GetPeakNumInTouchZoneLine(param_1,param_2,local_b,&local_f,&local_10);
    if (local_c < local_f) {
      local_c = local_f;
    }
    if (local_d < local_10) {
      local_d = local_10;
    }
    if ((uint)local_e < (uint)local_10 + (uint)local_f) {
      local_e = local_10 + local_f;
    }
  }
  return local_e;
}


// ============================================================================
// Function: HIException_WaterOrWetFinger @ 6ba59f73
// ============================================================================

undefined4 HIException_WaterOrWetFinger(longlong param_1)

{
  char cVar1;
  undefined4 local_c;
  
  local_c = 0x80;
  if (0x19 < *(ushort *)(param_1 + 0x33c)) {
    if (((*_refptr_g_prevTouchesPtr)->nTouchCount == 0x40) && ((*(uint *)(param_1 + 4) & 0x20) != 0)
       ) {
      local_c = 0x40;
    }
    else {
      if ((*(int *)(*_refptr_g_peaksPtr +
                   (longlong)(int)(uint)*(byte *)(param_1 + 0x2b6) * 0x14 + 0x1c) <
           (int)*(short *)(*_refptr_g_peaksPtr +
                          (longlong)(int)(uint)*(byte *)(param_1 + 0x2b6) * 0x14 + 0x18)) &&
         (cVar1 = HIException_IsInnerEdgeNegSigExist(param_1), cVar1 == '\0')) {
        return 0x40;
      }
      if (*(short *)(*_refptr_g_peaksPtr +
                    (longlong)(int)(uint)*(byte *)(param_1 + 0x2b8) * 0x14 + 0x18) * 4 <
          _refptr_g_tsaPrpt->nSPeakPositive * 2) {
        local_c = 0x40;
      }
      else {
        cVar1 = HIException_CheckWater(param_1);
        if (cVar1 != '\0') {
          local_c = 0x40;
        }
      }
    }
  }
  return local_c;
}


// ============================================================================
// Function: HIException_GetPeakNumInTouchZoneLine @ 6ba5add6
// ============================================================================

void HIException_GetPeakNumInTouchZoneLine
               (short *param_1,int param_2,byte param_3,char *param_4,char *param_5)

{
  short sVar1;
  short sVar2;
  uint uVar3;
  int iVar4;
  byte local_f;
  byte local_e;
  byte local_d;
  int local_c;
  
  local_c = 0;
  sVar1 = *param_1;
  if (param_2 == 1) {
    local_d = *(byte *)((longlong)param_1 + 0x33);
    local_e = *(byte *)(param_1 + 0x1a);
  }
  else {
    local_d = *(byte *)((longlong)param_1 + 0x35);
    local_e = *(byte *)(param_1 + 0x1b);
  }
  *param_4 = '\0';
  *param_5 = '\0';
  for (local_f = local_d; local_f <= local_e; local_f = local_f + 1) {
    if (param_2 == 1) {
      uVar3 = (uint)local_f + (uint)param_3 * (uint)_refptr_g_tsaPrmtFlash->bCols;
    }
    else {
      uVar3 = (uint)param_3 + (uint)local_f * (uint)_refptr_g_tsaPrmtFlash->bCols;
    }
    uVar3 = uVar3 & 0xffff;
    if (*(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)uVar3 * 2) < 0) {
      iVar4 = -1;
    }
    else {
      iVar4 = 1;
    }
    if (2 < (int)(*(char *)((ulonglong)uVar3 + *(longlong *)_refptr_g_tzAccess) * iVar4 -
                 (uint)*(byte *)(*(longlong *)_refptr_g_ptrAccess + 9))) {
      sVar2 = *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)uVar3 * 2);
      if (local_c == 0) {
        if (sVar1 < sVar2) {
          *param_5 = *param_5 + '\x01';
          local_c = 2;
        }
        else if ((int)sVar1 < -(int)sVar2) {
          *param_4 = *param_4 + '\x01';
          local_c = 1;
        }
      }
      else if (local_c == 1) {
        if (sVar1 < sVar2) {
          *param_5 = *param_5 + '\x01';
          local_c = 2;
        }
      }
      else if ((int)sVar1 < -(int)sVar2) {
        *param_4 = *param_4 + '\x01';
        local_c = 1;
      }
    }
  }
  return;
}


// ============================================================================
// Function: HIException_IsInnerEdgeNegSigExist @ 6ba59ea6
// ============================================================================

undefined8 HIException_IsInnerEdgeNegSigExist(longlong param_1)

{
  undefined8 uVar1;
  
  if (_refptr_g_tsaPrmtDynamic[0x96] == '\0') {
    uVar1 = 0;
  }
  else if (((*_refptr_g_peaksPtr)[(longlong)(int)(uint)*(byte *)(param_1 + 0x2b6) * 0x14 + 0x15].
            bPeakCount == '\x01') ||
          ((uint)(*_refptr_g_peaksPtr)
                 [(longlong)(int)(uint)*(byte *)(param_1 + 0x2b6) * 0x14 + 0x15].bPeakCount ==
           _refptr_g_tsaPrmtFlash->bCols - 2)) {
    *(undefined1 *)(param_1 + 0x36d) = 1;
    uVar1 = 1;
  }
  else {
    *(undefined1 *)(param_1 + 0x36d) = 0;
    uVar1 = 0;
  }
  return uVar1;
}


