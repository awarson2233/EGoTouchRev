#include "TSACoreInternal.h"

// ============================================================================
// Function: TPSensor_ProcessRaw @ 6ba6c910
// ============================================================================

void TPSensor_ProcessRaw(void)

{
                    /* 0x2c910  1472  TPSensor_ProcessRaw */
  TPSensor_TranslateRaw();
  NotchCorrect_ProcessRaw();
  return;
}


// ============================================================================
// Function: TPSensor_Process @ 6ba6c929
// ============================================================================

void TPSensor_Process(void)

{
                    /* 0x2c929  1471  TPSensor_Process */
  TPSensor_CompensateBrokenSensor();
  NotchCorrect_Process();
  TPSensor_ScalingDif();
  return;
}


// ============================================================================
// Function: TPSensor_TranslateRaw @ 6ba6c6c7
// ============================================================================

void TPSensor_TranslateRaw(void)

{
  byte bVar1;
  ushort uVar2;
  ushort uVar3;
  ushort local_a;
  
                    /* 0x2c6c7  1476  TPSensor_TranslateRaw */
  bVar1 = *(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x12d);
  for (local_a = 0; local_a < bVar1; local_a = local_a + 1) {
    uVar2 = (ushort)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                             (longlong)(int)(uint)local_a * 6 + 0x12e);
    uVar3 = (ushort)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                             (longlong)(int)(uint)local_a * 6 + 0x12f);
    *(short *)(*(longlong *)_refptr_g_tsaBufRaw +
              (ulonglong)(ushort)(uVar2 + _refptr_g_tsaPrmtFlash->bCols * uVar3) * 2) =
         *(short *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                   (longlong)(int)(uint)local_a * 6 + 0x132) +
         *(short *)((ulonglong)(ushort)(uVar2 + _refptr_g_tsaPrmtFlash->bCols * uVar3) * 2 +
                   *(longlong *)_refptr_g_tsaBufRaw);
  }
  return;
}


// ============================================================================
// Function: TPSensor_CompensateBrokenSensor @ 6ba6c403
// ============================================================================

void TPSensor_CompensateBrokenSensor(void)

{
  int iVar1;
  int local_c;
  
  iVar1 = TPSensor_IsCompensationNeeded();
  if ((iVar1 != 0) && (g_tpSensorBrokenList < 100)) {
    TPSensor_UpdateBrokenSensorTable();
    for (local_c = 0; local_c < (int)(uint)g_tpSensorBrokenList; local_c = local_c + 1) {
      TPSensor_ProcessSensor(&DAT_6bb80e41 + (longlong)local_c * 3);
    }
  }
  return;
}


// ============================================================================
// Function: TPSensor_ScalingDif @ 6ba6c52c
// ============================================================================

void TPSensor_ScalingDif(void)

{
  byte bVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  int local_c;
  
  bVar1 = *(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x12d);
  for (local_c = 0; local_c < (int)(uint)bVar1; local_c = local_c + 1) {
    uVar2 = (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                           (longlong)local_c * 6 + 0x12e);
    uVar3 = (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                           (longlong)local_c * 6 + 0x12f);
    iVar4 = (uint)*(ushort *)
                   ((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                   (longlong)local_c * 6 + 0x130) *
            (int)*(short *)((ulonglong)(_refptr_g_tsaPrmtFlash->bCols * uVar3 + uVar2 & 0xffff) * 2
                           + *(longlong *)_refptr_g_tsaBufDif);
    if (iVar4 < 0) {
      iVar4 = iVar4 + 0x3ff;
    }
    *(short *)(*(longlong *)_refptr_g_tsaBufDif +
              (ulonglong)(_refptr_g_tsaPrmtFlash->bCols * uVar3 + uVar2 & 0xffff) * 2) =
         (short)(iVar4 >> 10);
  }
  return;
}


// ============================================================================
// Function: TPSensor_Reset @ 6ba6bb6b
// ============================================================================

void TPSensor_Reset(void)

{
                    /* 0x2bb6b  1473  TPSensor_Reset */
  g_tpSensorBrokenList = 0;
  return;
}


// ============================================================================
// Function: TPSensor_UpdateBrokenSensorTable @ 6ba6bb78
// ============================================================================

void TPSensor_UpdateBrokenSensorTable(void)

{
  longlong lVar1;
  int local_10;
  ushort local_a;
  
  for (local_a = 0; local_a < _refptr_g_tsaPrmtFlash->wBufElementCount; local_a = local_a + 1) {
    *(byte *)((ulonglong)local_a + g_tpSensorFlag) =
         *(byte *)((ulonglong)local_a + g_tpSensorFlag) & 0xf0;
  }
  for (local_10 = 0; local_10 < (int)(uint)g_tpSensorBrokenList; local_10 = local_10 + 1) {
    lVar1 = (longlong)local_10 * 3;
    *(byte *)(g_tpSensorFlag +
             (ulonglong)
             ((uint)(byte)(&DAT_6bb80e41)[lVar1] +
              (uint)(byte)(&DAT_6bb80e42)[lVar1] * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)) =
         (&DAT_6bb80e43)[(longlong)local_10 * 3] & 0xf |
         *(byte *)((ulonglong)
                   ((uint)(byte)(&DAT_6bb80e41)[lVar1] +
                    (uint)(byte)(&DAT_6bb80e42)[lVar1] * (uint)_refptr_g_tsaPrmtFlash->bCols &
                   0xffff) + g_tpSensorFlag);
  }
  return;
}


// ============================================================================
// Function: TPSensor_ProcessSensor @ 6ba6c0e5
// ============================================================================

void TPSensor_ProcessSensor(byte *param_1)

{
  short sVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
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
          iVar6 = local_18 + (uint)*param_1;
          iVar2 = local_1c + (uint)param_1[1];
          iVar3 = (uint)*param_1 + local_18 * 2;
          iVar4 = (uint)param_1[1] + local_1c * 2;
          iVar5 = TPSensor_IsSensorValid(iVar6,iVar2);
          if ((iVar5 != 0) && (iVar5 = TPSensor_IsSensorValid(iVar3,iVar4), iVar5 != 0)) {
            iVar3 = (*(short *)((ulonglong)
                                ((uint)_refptr_g_tsaPrmtFlash->bCols * iVar2 + iVar6 & 0xffff) * 2 +
                               *(longlong *)_refptr_g_tsaBufDif) * 2 -
                    (int)*(short *)((ulonglong)
                                    ((uint)_refptr_g_tsaPrmtFlash->bCols * iVar4 + iVar3 & 0xffff) *
                                    2 + *(longlong *)_refptr_g_tsaBufDif)) * 0x100;
            if (iVar3 < 0) {
              iVar3 = iVar3 + 0x7f;
            }
            local_c = local_c + (iVar3 >> 7);
            local_10 = local_10 + 1;
            if (local_12 <
                *(short *)((ulonglong)((uint)_refptr_g_tsaPrmtFlash->bCols * iVar2 + iVar6 & 0xffff)
                           * 2 + *(longlong *)_refptr_g_tsaBufDif)) {
              local_12 = *(short *)((ulonglong)
                                    ((uint)_refptr_g_tsaPrmtFlash->bCols * iVar2 + iVar6 & 0xffff) *
                                    2 + *(longlong *)_refptr_g_tsaBufDif);
            }
          }
        }
      }
    }
    if (local_10 == 0) {
      *(undefined2 *)
       ((ulonglong)
        ((uint)*param_1 + (uint)param_1[1] * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
       *(longlong *)_refptr_g_tsaBufDif) = 0;
    }
    else {
      iVar6 = (local_c / local_10) * 8;
      if (local_c / local_10 << 3 < 0) {
        iVar6 = iVar6 + 0xf;
      }
      local_c = iVar6 >> 4;
      iVar6 = local_12 * 5;
      if (iVar6 < 0) {
        iVar6 = iVar6 + 3;
      }
      sVar1 = (short)(iVar6 >> 2);
      if (sVar1 < local_c) {
        local_c = (int)sVar1;
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
// Function: TPSensor_IsCompensationNeeded @ 6ba6c3bf
// ============================================================================

undefined8 TPSensor_IsCompensationNeeded(void)

{
  undefined8 uVar1;
  
  if (*(char *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x1a6) == '\0') {
    uVar1 = 0;
  }
  else if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x80000) == 0) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: TPSensor_Init @ 6ba6be2e
// ============================================================================

void TPSensor_Init(void)

{
  byte bVar1;
  TSAFlashPrmt *pTVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  int local_10;
  int local_c;
  
                    /* 0x2be2e  1464  TPSensor_Init */
  iVar3 = memset_s(g_tpSensorDim1Offset,*(undefined2 *)_refptr_g_gridBufSize,0,
                   _refptr_g_tsaPrmtFlash->wBufElementCount);
  if (iVar3 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/TPSensor/TPSensor.c",0xaa,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar3 = memset_s(g_tpSensorDim2Offset,*(undefined2 *)_refptr_g_gridBufSize,0,
                   _refptr_g_tsaPrmtFlash->wBufElementCount);
  if (iVar3 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/TPSensor/TPSensor.c",0xac,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  bVar1 = *(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x6c);
  for (local_10 = 0; pTVar2 = _refptr_g_tsaPrmtFlash, local_10 < (int)(uint)bVar1;
      local_10 = local_10 + 1) {
    uVar4 = (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                           (longlong)local_10 * 5 + 0x6d);
    uVar5 = (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                           (longlong)local_10 * 5 + 0x6e);
    if ((*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + (longlong)local_10 * 5 + 0x6f
                  ) & 1) != 0) {
      *(undefined1 *)
       (g_tpSensorDim1Offset + (ulonglong)(_refptr_g_tsaPrmtFlash->bCols * uVar5 + uVar4 & 0xffff))
           = *(undefined1 *)
              ((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + (longlong)local_10 * 5 + 0x70);
      *(undefined1 *)
       (g_tpSensorDim2Offset + (ulonglong)(_refptr_g_tsaPrmtFlash->bCols * uVar5 + uVar4 & 0xffff))
           = *(undefined1 *)((longlong)pTVar2[1].pDim1PitchMap + (longlong)local_10 * 5 + 0x71);
    }
  }
  NotchCorrect_Init();
  for (local_c = 0; local_c < (int)(uint)_refptr_g_tsaPrmtFlash->wBufElementCount;
      local_c = local_c + 1) {
    *(undefined2 *)(g_tpSensorDifScale + (longlong)local_c * 2) = 0x400;
  }
  bVar1 = *(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x12d);
  for (local_10 = 0; local_10 < (int)(uint)bVar1; local_10 = local_10 + 1) {
    *(undefined2 *)
     (g_tpSensorDifScale +
     (ulonglong)
     ((uint)_refptr_g_tsaPrmtFlash->bCols *
      (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                     (longlong)local_10 * 6 + 0x12f) +
      (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                     (longlong)local_10 * 6 + 0x12e) & 0xffff) * 2) =
         *(undefined2 *)
          ((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + (longlong)local_10 * 6 + 0x130);
  }
  return;
}


// ============================================================================
// Function: TPSensor_IsSensorValid @ 6ba6bdc1
// ============================================================================

bool TPSensor_IsSensorValid(int param_1,int param_2)

{
  int iVar1;
  bool bVar2;
  
                    /* 0x2bdc1  1469  TPSensor_IsSensorValid */
  iVar1 = TPSensor_IsSensorButNotNotch(param_1,param_2);
  if (iVar1 == 0) {
    bVar2 = false;
  }
  else {
    bVar2 = (*(byte *)((int)((uint)_refptr_g_tsaPrmtFlash->bCols * param_2 + param_1 & 0xffff) +
                      g_tpSensorFlag) & 0xf) == 0;
  }
  return bVar2;
}


// ============================================================================
// Function: TPSensor_IsNoMoveInCorrection @ 6ba6c8ef
// ============================================================================

char TPSensor_IsNoMoveInCorrection(undefined4 param_1,undefined4 param_2)

{
                    /* 0x2c8ef  1466  TPSensor_IsNoMoveInCorrection */
  return (char)NotchCorrect_IsNoMoveInCorrection(param_1,param_2);
}


// ============================================================================
// Function: TPSensor_IsSensorButNotNotch @ 6ba6bd35
// ============================================================================

undefined8 TPSensor_IsSensorButNotNotch(int param_1,int param_2)

{
  undefined8 uVar1;
  
                    /* 0x2bd35  1467  TPSensor_IsSensorButNotNotch */
  if (((byte)param_1 < _refptr_g_tsaPrmtFlash->bCols) &&
     ((byte)param_2 < _refptr_g_tsaPrmtFlash->bRows)) {
    if ((*(byte *)((int)((uint)_refptr_g_tsaPrmtFlash->bCols * param_2 + param_1 & 0xffff) +
                  g_tpSensorFlag) & 0x20) == 0) {
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


// ============================================================================
// Function: TPSensor_AddToBrokenSensorList @ 6ba6babf
// ============================================================================

bool TPSensor_AddToBrokenSensorList(undefined1 param_1,undefined1 param_2,undefined1 param_3)

{
  bool bVar1;
  
                    /* 0x2babf  1462  TPSensor_AddToBrokenSensorList */
  bVar1 = g_tpSensorBrokenList < 100;
  if (bVar1) {
    (&DAT_6bb80e41)[(longlong)(int)(uint)g_tpSensorBrokenList * 3] = param_1;
    (&DAT_6bb80e42)[(longlong)(int)(uint)g_tpSensorBrokenList * 3] = param_2;
    (&DAT_6bb80e43)[(longlong)(int)(uint)g_tpSensorBrokenList * 3] = param_3;
    g_tpSensorBrokenList = g_tpSensorBrokenList + 1;
  }
  return bVar1;
}


// ============================================================================
// Function: TPSensor_IsSensorReusedNotch @ 6ba6bca9
// ============================================================================

undefined8 TPSensor_IsSensorReusedNotch(int param_1,int param_2)

{
  undefined8 uVar1;
  
                    /* 0x2bca9  1468  TPSensor_IsSensorReusedNotch */
  if (((byte)param_1 < _refptr_g_tsaPrmtFlash->bCols) &&
     ((byte)param_2 < _refptr_g_tsaPrmtFlash->bRows)) {
    if ((*(byte *)((int)((uint)_refptr_g_tsaPrmtFlash->bCols * param_2 + param_1 & 0xffff) +
                  g_tpSensorFlag) & 0x40) == 0) {
      uVar1 = 0;
    }
    else {
      uVar1 = 1;
    }
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: TPSensor_IsBrokenSensorInTouchZone @ 6ba6c46f
// ============================================================================

void TPSensor_IsBrokenSensorInTouchZone(longlong param_1)

{
  char cVar1;
  int iVar2;
  byte local_9;
  
                    /* 0x2c46f  1465  TPSensor_IsBrokenSensorInTouchZone */
  if ((param_1 != 0) && (iVar2 = TPSensor_IsCompensationNeeded(), iVar2 != 0)) {
    for (local_9 = 0; local_9 < g_tpSensorBrokenList; local_9 = local_9 + 1) {
      cVar1 = Sensor_IsInZone((&DAT_6bb80e41)[(longlong)(int)(uint)local_9 * 3],
                              (&DAT_6bb80e42)[(longlong)(int)(uint)local_9 * 3],
                              *(undefined1 *)(param_1 + 0x35e));
      if (cVar1 != '\0') {
        *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x10000;
        return;
      }
    }
  }
  return;
}


// ============================================================================
// Function: TPSensor_Free @ 6ba6ba80
// ============================================================================

void TPSensor_Free(void)

{
                    /* 0x2ba80  1463  TPSensor_Free */
  TSA_Free(&g_tpSensorFlag);
  TSA_Free(&g_tpSensorDim1Offset);
  TSA_Free(&g_tpSensorDim2Offset);
  TSA_Free(&g_tpSensorDifScale);
  return;
}


// ============================================================================
// Function: TPSensor_Malloc @ 6ba6b9b0
// ============================================================================

undefined8 TPSensor_Malloc(void)

{
  undefined8 uVar1;
  
                    /* 0x2b9b0  1470  TPSensor_Malloc */
  g_tpSensorFlag = TSA_MallocAndInit(*(undefined2 *)_refptr_g_gridBufSize);
  if (g_tpSensorFlag == 0) {
    uVar1 = 0;
  }
  else {
    g_tpSensorDim1Offset = TSA_MallocAndInit(*(undefined2 *)_refptr_g_gridBufSize);
    if (g_tpSensorDim1Offset == 0) {
      uVar1 = 0;
    }
    else {
      g_tpSensorDim2Offset = TSA_MallocAndInit(*(undefined2 *)_refptr_g_gridBufSize);
      if (g_tpSensorDim2Offset == 0) {
        uVar1 = 0;
      }
      else {
        g_tpSensorDifScale = TSA_MallocAndInit((uint)*(ushort *)_refptr_g_gridBufSize * 2);
        if (g_tpSensorDifScale == 0) {
          uVar1 = 0;
        }
        else {
          uVar1 = 1;
        }
      }
    }
  }
  return uVar1;
}


// ============================================================================
// Function: TPSensor_RevertScalingDif @ 6ba6c635
// ============================================================================

ulonglong TPSensor_RevertScalingDif(ushort param_1,byte param_2,byte param_3)

{
  ulonglong uVar1;
  
                    /* 0x2c635  1474  TPSensor_RevertScalingDif */
  if (*(short *)((ulonglong)
                 ((uint)param_2 + (uint)param_3 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2
                + g_tpSensorDifScale) == 0) {
    uVar1 = (ulonglong)param_1;
  }
  else {
    uVar1 = (longlong)((int)(short)param_1 << 10) /
            (longlong)
            (int)(uint)*(ushort *)
                        ((ulonglong)
                         ((uint)param_2 + (uint)param_3 * (uint)_refptr_g_tsaPrmtFlash->bCols &
                         0xffff) * 2 + g_tpSensorDifScale) & 0xffffffff;
  }
  return uVar1;
}


// ============================================================================
// Function: TPSensor_RevertTranslateRaw @ 6ba6c7c5
// ============================================================================

void TPSensor_RevertTranslateRaw(longlong param_1,uint param_2)

{
  byte bVar1;
  ushort uVar2;
  ushort uVar3;
  ushort local_a;
  
                    /* 0x2c7c5  1475  TPSensor_RevertTranslateRaw */
  if (param_1 != 0) {
    bVar1 = *(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x12d);
    for (local_a = 0; local_a < bVar1; local_a = local_a + 1) {
      uVar2 = (ushort)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                               (longlong)(int)(uint)local_a * 6 + 0x12e);
      uVar3 = (ushort)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                               (longlong)(int)(uint)local_a * 6 + 0x12f);
      if ((ushort)(uVar2 + _refptr_g_tsaPrmtFlash->bCols * uVar3) < param_2) {
        *(short *)((ulonglong)(ushort)(uVar2 + _refptr_g_tsaPrmtFlash->bCols * uVar3) * 2 + param_1)
             = *(short *)(param_1 +
                         (ulonglong)(ushort)(uVar2 + _refptr_g_tsaPrmtFlash->bCols * uVar3) * 2) -
               *(short *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                         (longlong)(int)(uint)local_a * 6 + 0x132);
      }
    }
  }
  return;
}


