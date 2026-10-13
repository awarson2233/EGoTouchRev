// ============================================================================
// Function: FIException_CheckChargerNoiseInRxLines @ 6ba59429
// ============================================================================

void FIException_CheckChargerNoiseInRxLines(void)

{
                    /* 0x19429  590  FIException_CheckChargerNoiseInRxLines */
  return;
}


// ============================================================================
// Function: FIException_IsChargerNoiseDetected @ 6ba5942f
// ============================================================================

undefined8 FIException_IsChargerNoiseDetected(void)

{
                    /* 0x1942f  594  FIException_IsChargerNoiseDetected */
  return 0;
}


// ============================================================================
// Function: FIException_CheckPanelSD @ 6ba593bc
// ============================================================================

void FIException_CheckPanelSD(void)

{
                    /* 0x193bc  591  FIException_CheckPanelSD */
  return;
}


// ============================================================================
// Function: FIException_Init @ 6ba593c2
// ============================================================================

void FIException_Init(void)

{
                    /* 0x193c2  593  FIException_Init */
  *(undefined2 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x42 = 0;
  *(undefined8 *)((longlong)&(*_refptr_g_tsaStaticPtr)[1].llField_0x48 + 2) = 0;
  *(undefined4 *)((longlong)&(*_refptr_g_tsaStaticPtr)[1].llField_0x50 + 2) = 0;
  *(undefined1 *)((longlong)&(*_refptr_g_tsaStaticPtr)[1].llField_0x50 + 6) = 0;
  *(undefined4 *)((longlong)&(*_refptr_g_tsaStaticPtr)[1].llField_0x58 + 2) = 0;
  return;
}


// ============================================================================
// Function: FIException_ProcessTouchZone @ 6ba59bdb
// ============================================================================

void FIException_ProcessTouchZone(longlong param_1)

{
  char cVar1;
  
                    /* 0x19bdb  596  FIException_ProcessTouchZone */
  *(undefined1 *)((longlong)&(*_refptr_g_tsaStaticPtr)[1].llField_0x50 + 6) = 0;
  FIException_GetRatio(param_1);
  if (*(int *)((longlong)&(*_refptr_g_tsaStaticPtr)[1].llField_0x58 + 2) == 0x100) {
    *(uint *)(param_1 + 0x350) = *(uint *)(param_1 + 0x350) | 0x100;
    *(undefined4 *)(param_1 + 0x354) = 0x100;
    *(undefined1 *)(param_1 + 0x348) = 1;
  }
  else {
    cVar1 = FIException_CheckAbnormalTouchZoneBaseline(param_1);
    if (cVar1 == '\0') {
      cVar1 = FIException_CheckWetFinger(param_1);
      if (cVar1 != '\0') {
        *(undefined1 *)((longlong)&(*_refptr_g_tsaStaticPtr)[1].llField_0x50 + 6) = 1;
        *(uint *)(param_1 + 0x350) = *(uint *)(param_1 + 0x350) | 0x80;
        *(undefined4 *)(param_1 + 0x354) = 0x80;
      }
    }
    else {
      *(uint *)(param_1 + 0x350) = *(uint *)(param_1 + 0x350) | 0x200;
      *(undefined4 *)(param_1 + 0x354) = 0x200;
      *(undefined1 *)(param_1 + 0x34c) = 1;
    }
    FIException_DebounceTouchZone(param_1);
  }
  FIException_Recording(*(undefined4 *)(param_1 + 0x350));
  return;
}


// ============================================================================
// Function: FIException_Recording @ 6ba5943a
// ============================================================================

void FIException_Recording(uint param_1)

{
  if ((*(uint *)((longlong)&(*_refptr_g_tsaStaticPtr)[1].llField_0x58 + 2) < param_1) &&
     (param_1 != 2)) {
    *(uint *)((longlong)&(*_refptr_g_tsaStaticPtr)[1].llField_0x58 + 2) = param_1;
  }
  return;
}


// ============================================================================
// Function: FIException_DebounceTouchZone @ 6ba59471
// ============================================================================

void FIException_DebounceTouchZone(longlong param_1)

{
  bool bVar1;
  int local_10;
  int local_c;
  
  local_c = 0;
  local_10 = *(int *)(param_1 + 0x350);
  if (*(char *)(param_1 + 0x324) != '\x14') {
    local_c = *(int *)((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)*(byte *)(param_1 + 0x324) * 0x478 + 0x1ec);
  }
  if (((*_refptr_g_prevTouchesPtr)->nTouchCount & 0xc0U) == 0) {
    if ((*_refptr_g_prevTouchesPtr)->nTouchCount == 8) {
      if ((local_10 == 0) && (local_c == 0)) {
        local_10 = 0;
      }
      else if ((*(char *)((longlong)&(*_refptr_g_tsaStaticPtr)[1].llField_0x50 + 6) == '\0') ||
              (2 < (byte)(*_refptr_g_prevTouchesPtr)->field_0x5)) {
        local_10 = 8;
      }
      else {
        (*_refptr_g_prevTouchesPtr)->nTouchCount = local_10;
      }
    }
    else {
      bVar1 = local_10 == 2;
      local_10 = *(int *)(param_1 + 0x350);
      if (bVar1) {
        if ((local_c == 0x40) || (local_c == 0x80)) {
          local_10 = 0x80;
        }
        else if (local_c == 8) {
          local_10 = 8;
        }
      }
    }
  }
  else if ((local_10 == 0) && (local_c == 0)) {
    local_10 = 0;
  }
  else {
    local_10 = (*_refptr_g_prevTouchesPtr)->nTouchCount;
  }
  if ((4 < (byte)(*_refptr_g_prevTouchesPtr)->field_0x5) && (local_10 != 0)) {
    local_10 = 8;
  }
  *(int *)(param_1 + 0x350) = local_10;
  return;
}


// ============================================================================
// Function: FIException_CheckAbnormalTouchZoneBaseline @ 6ba595ee
// ============================================================================

undefined1 FIException_CheckAbnormalTouchZoneBaseline(longlong param_1)

{
  undefined1 local_9;
  
  local_9 = 0;
  if ((*(uchar *)(param_1 + 0x2b2) == (*_refptr_g_peaksPtr)->bPeakCount) &&
     (200 < *(ushort *)(param_1 + 0x33c))) {
    local_9 = 1;
  }
  return local_9;
}


// ============================================================================
// Function: FIException_CheckWetFinger @ 6ba59919
// ============================================================================

undefined8 FIException_CheckWetFinger(longlong param_1)

{
  byte bVar1;
  byte bVar2;
  byte bVar3;
  short sVar4;
  short sVar5;
  char cVar6;
  undefined8 uVar7;
  uint local_14;
  uint local_10;
  ushort local_a;
  
  local_a = 0;
  bVar1 = *(byte *)(param_1 + 0x324);
  if (bVar1 < 0x14) {
    cVar6 = TSAStatic_IsProximityEnabled();
    if (cVar6 == '\0') {
      PrevTouch_HistoryQueuePush
                (bVar1,*(undefined2 *)
                        ((longlong)*_refptr_g_prevTouchesPtr +
                        (longlong)(int)(uint)bVar1 * 0x478 + 0x166),
                 *(undefined2 *)
                  ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x168)
                );
      if (_refptr_g_tsaPrpt->nSPeakPositive < (short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax)
      {
        uVar7 = 0;
      }
      else {
        memset_s(g_reached,*(undefined2 *)_refptr_g_gridBufSize,0,
                 *(undefined2 *)_refptr_g_gridBufSize);
        sVar4 = _refptr_g_tsaPrpt->nSPeakPositive;
        bVar2 = *(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x5d);
        sVar5 = _refptr_g_tsaPrpt->nSPeakPositive;
        bVar3 = *(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x5e);
        for (local_10 = (uint)*(char *)((longlong)*_refptr_g_prevTouchesPtr +
                                       (longlong)(int)(uint)bVar1 * 0x478 + 0x474);
            (int)*(char *)((longlong)*_refptr_g_prevTouchesPtr +
                          (longlong)(int)(uint)bVar1 * 0x478 + 0x475) != local_10;
            local_10 = (int)(local_10 + 1) % 0x10) {
          local_a = FIException_GetWaterSize
                              (bVar1,local_10 & 0xff,local_a,
                               (int)(short)((int)((int)sVar4 * (uint)bVar2) / 100),
                               (int)(short)((int)((int)sVar5 * (uint)bVar3) / 100));
        }
        *(ushort *)(param_1 + 0x10) = local_a;
        local_14 = 0;
        if (*(short *)(param_1 + 0x2ee) != 0) {
          local_14 = (((uint)*(ushort *)(param_1 + 0x2ec) * 4 + (uint)*(ushort *)(param_1 + 0x2ec))
                     * 2) / (uint)*(ushort *)(param_1 + 0x2ee);
        }
        if (((local_a < *(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x5c)) ||
            (local_14 < 0x14)) || ((*(uint *)(param_1 + 4) & 0x20) != 0)) {
          uVar7 = 0;
        }
        else {
          uVar7 = 1;
        }
      }
    }
    else {
      uVar7 = 0;
    }
  }
  else {
    uVar7 = 0;
  }
  return uVar7;
}


// ============================================================================
// Function: FIException_GetRatio @ 6ba59bbf
// ============================================================================

void FIException_GetRatio(undefined8 param_1)

{
  TZ_GetExceptionRatio(param_1);
  return;
}


// ============================================================================
// Function: FIException_GetWaterSize @ 6ba59733
// ============================================================================

short FIException_GetWaterSize(byte param_1,byte param_2,short param_3,short param_4,short param_5)

{
  byte bVar1;
  byte bVar2;
  short sVar3;
  bool bVar4;
  char cVar5;
  short sVar6;
  short sVar7;
  short sVar8;
  short local_res18;
  short local_c;
  short local_a;
  
  bVar1 = *(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_2 + (longlong)(int)(uint)param_1 * 0x478 + 0x476);
  bVar2 = *(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_2 + (longlong)(int)(uint)param_1 * 0x478 + 0x486);
  bVar4 = false;
  local_c = -1;
  local_res18 = param_3;
  do {
    if (1 < local_c) {
      return local_res18;
    }
    for (local_a = -1; local_a < 2; local_a = local_a + 1) {
      sVar6 = local_a + (ushort)bVar1;
      sVar7 = local_c + (ushort)bVar2;
      if (((((byte)sVar6 < _refptr_g_tsaPrmtFlash->bCols) &&
           ((byte)sVar7 < _refptr_g_tsaPrmtFlash->bRows)) &&
          (sVar8 = sVar6 + (ushort)_refptr_g_tsaPrmtFlash->bCols * sVar7,
          *(char *)(sVar8 + g_reached) == '\0')) &&
         (((sVar3 = *(short *)(*(longlong *)_refptr_g_tsaBufDif + (longlong)sVar8 * 2),
           sVar3 <= param_5 && (param_4 <= sVar3)) &&
          (cVar5 = FIException_IsAroundSmall((int)sVar6,(int)sVar7,1,(int)param_5), cVar5 != '\0')))
         ) {
        local_res18 = local_res18 + 1;
        *(undefined1 *)(sVar8 + g_reached) = 1;
        bVar4 = true;
        break;
      }
    }
    if (bVar4) {
      return local_res18;
    }
    local_c = local_c + 1;
  } while( true );
}


// ============================================================================
// Function: FIException_IsAroundSmall @ 6ba59639
// ============================================================================

undefined8 FIException_IsAroundSmall(short param_1,short param_2,int param_3,short param_4)

{
  short local_c;
  short local_a;
  
  local_c = -(short)param_3;
  do {
    if (param_3 < local_c) {
      return 1;
    }
    for (local_a = -(short)param_3; local_a <= param_3; local_a = local_a + 1) {
      if ((((byte)(local_a + param_1) < _refptr_g_tsaPrmtFlash->bCols) &&
          ((byte)(local_c + param_2) < _refptr_g_tsaPrmtFlash->bRows)) &&
         (param_4 < *(short *)((ulonglong)
                               (ushort)(local_a + param_1 +
                                       (ushort)_refptr_g_tsaPrmtFlash->bCols * (local_c + param_2))
                               * 2 + *(longlong *)_refptr_g_tsaBufDif))) {
        return 0;
      }
    }
    local_c = local_c + 1;
  } while( true );
}


