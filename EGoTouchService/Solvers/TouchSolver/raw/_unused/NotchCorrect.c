// ============================================================================
// Function: NotchCorrect_ProcessRaw @ 6ba6d0b2
// ============================================================================

void NotchCorrect_ProcessRaw(void)

{
  int iVar1;
  int local_c;
  
                    /* 0x2d0b2  947  NotchCorrect_ProcessRaw */
  iVar1 = NotchCorrect_GetNotchNum();
  for (local_c = 0; local_c < iVar1; local_c = local_c + 1) {
    NotchCorrect_ProcessRawSensor
              ((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + (longlong)local_c * 3 + 0x105);
  }
  return;
}


// ============================================================================
// Function: NotchCorrect_Process @ 6ba6d109
// ============================================================================

void NotchCorrect_Process(void)

{
  errno_t eVar1;
  int iVar2;
  int local_c;
  
                    /* 0x2d109  946  NotchCorrect_Process */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&_rdata,"NotchCorrect_Process");
  }
  eVar1 = memcpy_s(_refptr_g_tsaBufPreNotchCorrectDif,0x2648,*(void **)_refptr_g_tsaBufDif,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/TPSensor/TPSensor_NotchCorrect.c",0x100,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar2 = NotchCorrect_GetNotchNum();
  for (local_c = 0; local_c < iVar2; local_c = local_c + 1) {
    if (*(char *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + (longlong)local_c * 3 + 0x107)
        < '\0') {
      NotchCorrect_ProcessSensor
                ((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + (longlong)local_c * 3 + 0x105,1
                );
    }
  }
  for (local_c = 0; local_c < iVar2; local_c = local_c + 1) {
    if (-1 < *(char *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                      (longlong)local_c * 3 + 0x107)) {
      NotchCorrect_ProcessSensor
                ((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + (longlong)local_c * 3 + 0x105,0
                );
    }
  }
  eVar1 = memcpy_s(_refptr_g_tsaBufAftNotchCorrectDif,0x2648,*(void **)_refptr_g_tsaBufDif,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/TPSensor/TPSensor_NotchCorrect.c",0x117,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: NotchCorrect_GetNotchNum @ 6ba6c950
// ============================================================================

byte NotchCorrect_GetNotchNum(void)

{
  byte bVar1;
  
                    /* 0x2c950  942  NotchCorrect_GetNotchNum */
  bVar1 = *(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x104);
  if (0xc < bVar1) {
    bVar1 = 0xc;
  }
  return bVar1;
}


// ============================================================================
// Function: NotchCorrect_ProcessRawSensor @ 6ba6cf82
// ============================================================================

void NotchCorrect_ProcessRawSensor(byte *param_1)

{
  byte bVar1;
  byte bVar2;
  int iVar3;
  int local_18;
  int local_14;
  int local_10;
  int local_c;
  
  local_14 = 0;
  local_18 = 0;
  if (param_1 != (byte *)0x0) {
    for (local_c = -1; local_c < 2; local_c = local_c + 1) {
      for (local_10 = -1; local_10 < 2; local_10 = local_10 + 1) {
        if ((local_c != 0) || (local_10 != 0)) {
          bVar1 = *param_1;
          bVar2 = param_1[1];
          iVar3 = TPSensor_IsSensorButNotNotch(local_c + (uint)bVar1,local_10 + (uint)bVar2);
          if (iVar3 != 0) {
            local_14 = local_14 +
                       (uint)*(ushort *)
                              ((ulonglong)
                               ((uint)_refptr_g_tsaPrmtFlash->bCols * (local_10 + (uint)bVar2) +
                                local_c + (uint)bVar1 & 0xffff) * 2 +
                              *(longlong *)_refptr_g_tsaBufRaw);
            local_18 = local_18 + 1;
          }
        }
      }
    }
    if (local_18 != 0) {
      *(short *)((ulonglong)
                 ((uint)*param_1 + (uint)param_1[1] * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)
                 * 2 + *(longlong *)_refptr_g_tsaBufRaw) = (short)(local_14 / local_18);
    }
  }
  return;
}


// ============================================================================
// Function: NotchCorrect_ProcessSensor @ 6ba6cc56
// ============================================================================

void NotchCorrect_ProcessSensor(byte *param_1,undefined1 param_2)

{
  short sVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int local_20;
  int local_1c;
  int local_18;
  short local_12;
  int local_10;
  int local_c;
  
  local_c = 0;
  local_10 = 0;
  local_12 = -0x8000;
  if (param_1 != (byte *)0x0) {
    for (local_18 = -1; local_18 < 2; local_18 = local_18 + 1) {
      for (local_1c = -1; local_1c < 2; local_1c = local_1c + 1) {
        if ((local_18 != 0) || (local_1c != 0)) {
          iVar2 = local_18 + (uint)*param_1;
          iVar3 = local_1c + (uint)param_1[1];
          iVar4 = (uint)*param_1 + local_18 * 2;
          iVar5 = (uint)param_1[1] + local_1c * 2;
          iVar6 = NotchCorrect_IsNeedSkipComp(iVar2,iVar3,iVar4,iVar5,param_2);
          if (iVar6 == 0) {
            local_20 = 0x10;
            if ((local_18 != 0) && (local_1c != 0)) {
              local_20 = 8;
            }
            local_20 = (*(short *)((ulonglong)
                                   ((uint)_refptr_g_tsaPrmtFlash->bCols * iVar3 + iVar2 & 0xffff) *
                                   2 + *(longlong *)_refptr_g_tsaBufDif) * 0x14 -
                       (int)*(short *)((ulonglong)
                                       ((uint)_refptr_g_tsaPrmtFlash->bCols * iVar5 + iVar4 & 0xffff
                                       ) * 2 + *(longlong *)_refptr_g_tsaBufDif) *
                       (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x103)) *
                       local_20;
            if (local_20 < 0) {
              local_20 = local_20 + 0x7f;
            }
            local_c = local_c + (local_20 >> 7);
            local_10 = local_10 + 1;
            if (local_12 <
                *(short *)((ulonglong)((uint)_refptr_g_tsaPrmtFlash->bCols * iVar3 + iVar2 & 0xffff)
                           * 2 + *(longlong *)_refptr_g_tsaBufDif)) {
              local_12 = *(short *)((ulonglong)
                                    ((uint)_refptr_g_tsaPrmtFlash->bCols * iVar3 + iVar2 & 0xffff) *
                                    2 + *(longlong *)_refptr_g_tsaBufDif);
            }
          }
        }
      }
    }
    if (local_10 != 0) {
      local_c = (uint)(param_1[2] & 0x7f) * (local_c / local_10);
      if (local_c < 0) {
        local_c = local_c + 0xf;
      }
      local_c = local_c >> 4;
      sVar1 = (short)((local_12 * 3) / 2);
      if (sVar1 < local_c) {
        local_c = (int)sVar1;
      }
      if (((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold * 6) / 5 < local_c) {
        local_c = ((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold * 6) / 5;
      }
      if (local_c < 0) {
        local_c = 0;
      }
      *(short *)((ulonglong)
                 ((uint)*param_1 + (uint)param_1[1] * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)
                 * 2 + *(longlong *)_refptr_g_tsaBufDif) = (short)local_c;
    }
  }
  return;
}


// ============================================================================
// Function: NotchCorrect_IsNeedSkipComp @ 6ba6cbb5
// ============================================================================

undefined8
NotchCorrect_IsNeedSkipComp
          (undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,char param_5)

{
  int iVar1;
  
                    /* 0x2cbb5  944  NotchCorrect_IsNeedSkipComp */
  if (param_5 == '\0') {
    iVar1 = TPSensor_IsSensorButNotNotch(param_1,param_2);
    if (((iVar1 == 0) && (iVar1 = TPSensor_IsSensorReusedNotch(param_1,param_2), iVar1 == 0)) ||
       ((iVar1 = TPSensor_IsSensorButNotNotch(param_3,param_4), iVar1 == 0 &&
        (iVar1 = TPSensor_IsSensorReusedNotch(param_3,param_4), iVar1 == 0)))) {
      return 1;
    }
  }
  else {
    iVar1 = TPSensor_IsSensorButNotNotch(param_1,param_2);
    if ((iVar1 == 0) || (iVar1 = TPSensor_IsSensorButNotNotch(param_3,param_4), iVar1 == 0)) {
      return 1;
    }
  }
  return 0;
}


// ============================================================================
// Function: NotchCorrect_Init @ 6ba6c982
// ============================================================================

void NotchCorrect_Init(void)

{
  int iVar1;
  uint uVar2;
  int local_10;
  int local_c;
  
                    /* 0x2c982  943  NotchCorrect_Init */
  for (local_c = 0; local_c < (int)(uint)_refptr_g_tsaPrmtFlash->wBufElementCount;
      local_c = local_c + 1) {
    *(byte *)(*(longlong *)_refptr_g_tpSensorFlag + (longlong)local_c) =
         *(byte *)((longlong)local_c + *(longlong *)_refptr_g_tpSensorFlag) & 0xdf;
    *(byte *)(*(longlong *)_refptr_g_tpSensorFlag + (longlong)local_c) =
         *(byte *)((longlong)local_c + *(longlong *)_refptr_g_tpSensorFlag) & 0xbf;
  }
  iVar1 = NotchCorrect_GetNotchNum();
  for (local_10 = 0; local_10 < iVar1; local_10 = local_10 + 1) {
    uVar2 = (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                           (longlong)local_10 * 3 + 0x105) +
            (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                           (longlong)local_10 * 3 + 0x106) * (uint)_refptr_g_tsaPrmtFlash->bCols &
            0xffff;
    *(byte *)(*(longlong *)_refptr_g_tpSensorFlag + (longlong)(int)uVar2) =
         *(byte *)((longlong)(int)uVar2 + *(longlong *)_refptr_g_tpSensorFlag) | 0x20;
    if (*(char *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + (longlong)local_10 * 3 + 0x107
                 ) < '\0') {
      *(byte *)(*(longlong *)_refptr_g_tpSensorFlag + (longlong)(int)uVar2) =
           *(byte *)((longlong)(int)uVar2 + *(longlong *)_refptr_g_tpSensorFlag) | 0x40;
    }
  }
  return;
}


// ============================================================================
// Function: NotchCorrect_IsNoMoveInCorrection @ 6ba6cb15
// ============================================================================

undefined8 NotchCorrect_IsNoMoveInCorrection(int param_1,int param_2)

{
  ushort uVar1;
  
                    /* 0x2cb15  945  NotchCorrect_IsNoMoveInCorrection */
  uVar1 = TS_MMToPixel(*(undefined1 *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x129));
  if (((((int)(uint)uVar1 <= param_1) &&
       (uVar1 = TS_MMToPixel(*(undefined1 *)
                              ((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x12a)),
       param_1 < (int)(uint)uVar1)) &&
      (uVar1 = TS_MMToPixel(*(undefined1 *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 299)
                           ), (int)(uint)uVar1 <= param_2)) &&
     (uVar1 = TS_MMToPixel(*(undefined1 *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 300))
     , param_2 < (int)(uint)uVar1)) {
    return 1;
  }
  return 0;
}


