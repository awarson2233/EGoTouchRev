#include "TSACoreInternal.h"

// ============================================================================
// Function: Peak_Process @ 6ba68c4b
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* Current config peak path: dwPeakProcessingMode=3 clamps dynamic threshold with
   wPeakMode3ThresholdMax/2, runs Peak_Z8Filter, then Z1 fallback unless full-incell SD is detected.
    */

void Peak_Process(void)

{
  char cConditionResult;
  short nMode3ThresholdCap;
  short nOriginalPeakThreshold;
  int nWorkValue;
  int nPeakIndex;
  byte bMinPeakRow;
  char cLastRowIndex;
  char cLastColIndex;
  bool bExcludeSideRegion;
  uchar bSensorRows;
  
                    /* 0x28c4b  992  Peak_Process */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&DAT_6bb524ce,"Peak_Process");
  }
  nOriginalPeakThreshold = *(short *)(_refptr_g_tsaPrmtDynamic + 8);
  nMode3ThresholdCap = (short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax / 2;
  if ((_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3) &&
     (nMode3ThresholdCap < *(short *)(_refptr_g_tsaPrmtDynamic + 8))) {
    *(short *)(_refptr_g_tsaPrmtDynamic + 8) = nMode3ThresholdCap;
  }
  cConditionResult = Peak_IsToExcludeSideRegion();
  if (cConditionResult == '\0') {
    cLastColIndex = _refptr_g_tsaPrmtFlash->bCols + 0xff;
    bSensorRows = _refptr_g_tsaPrmtFlash->bRows;
  }
  else {
    cLastColIndex = _refptr_g_tsaPrmtFlash->bCols + 0xfe;
    bSensorRows = _refptr_g_tsaPrmtFlash->bRows;
  }
  cLastRowIndex = bSensorRows + 0xff;
  bExcludeSideRegion = cConditionResult != '\0';
  Peak_DetectInRange((int)*(short *)(_refptr_g_tsaPrmtDynamic + 8),0,cLastRowIndex,
                     bExcludeSideRegion,cLastColIndex);
  if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3) {
    Peak_Z8Filter();
    cConditionResult = Peak_IsFullIncellSDDetected();
    if (cConditionResult == '\0') {
      (*_refptr_g_tsaStaticPtr)->bFullIncellSignalDisparity = '\0';
      Peak_Z1Filter((int)nOriginalPeakThreshold);
      *(short *)(_refptr_g_tsaPrmtDynamic + 8) = nOriginalPeakThreshold;
    }
    else {
      (*_refptr_g_tsaStaticPtr)->bFullIncellSignalDisparity = '\x01';
      nWorkValue = (uint)(byte)_refptr_g_tsaPrmtDynamic[0x1c] *
                   (int)*(short *)(_refptr_g_tsaPrmtDynamic + 8);
      if (nWorkValue < 0) {
        nWorkValue = nWorkValue + 0xf;
      }
      *(int *)(_refptr_g_tsaPrmtDynamic + 0x10) = nWorkValue >> 4;
    }
  }
  if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 2) {
    cConditionResult = SignalDisparity_IsOKToProcess();
    if ((cConditionResult != '\0') && (2 < (*_refptr_g_peaksPtr)->bPeakCount)) {
      nOriginalPeakThreshold = Peak_GetMaxPeak();
      Peak_Z1Filter((int)(nOriginalPeakThreshold / 3));
    }
  }
  bMinPeakRow = _refptr_g_tsaPrmtFlash->bRows;
  for (nPeakIndex = 0; nPeakIndex < (int)(uint)(*_refptr_g_peaksPtr)->bPeakCount;
      nPeakIndex = nPeakIndex + 1) {
    if ((*_refptr_g_peaksPtr)[(longlong)nPeakIndex * 0x14 + 0x16].bPeakCount < bMinPeakRow) {
      bMinPeakRow = (*_refptr_g_peaksPtr)[(longlong)nPeakIndex * 0x14 + 0x16].bPeakCount;
    }
  }
  (*_refptr_g_peaksPtr)[0xc].bPeakCount = bMinPeakRow;
  nWorkValue = Peak_needUDFPLargeFingerWorkaround();
  if (nWorkValue == 0) {
    Peak_SortInSigAscending();
  }
  else {
    Peak_SortInSigDescending();
  }
  Peak_IDTracking();
  return;
}


// ============================================================================
// Function: Peak_SortInSigAscending @ 6ba66013
// ============================================================================

void Peak_SortInSigAscending(void)

{
  undefined1 local_28 [24];
  errno_t local_10;
  char local_a;
  char local_9;
  
                    /* 0x26013  995  Peak_SortInSigAscending */
  for (local_9 = '\0'; local_a = local_9, (int)local_9 < (char)g_pPeaks->bPeakCount + -1;
      local_9 = local_9 + '\x01') {
    while (local_a = local_a + '\x01', (int)local_a < (int)(uint)g_pPeaks->bPeakCount) {
      if (*(short *)(g_pPeaks + (longlong)(int)local_a * 0x14 + 0x18) <
          *(short *)(g_pPeaks + (longlong)(int)local_9 * 0x14 + 0x18)) {
        local_10 = memcpy_s(local_28,0x14,g_pPeaks + (longlong)(int)local_9 * 0x14 + 0x10,0x14);
        if (local_10 != 0) {
          StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                            "../../src/Alg/Peak/PeakData.c",0x89,"rc == EOK");
          HardwareAnalyzer_SetSoftException();
        }
        local_10 = memcpy_s(g_pPeaks + (longlong)(int)local_9 * 0x14 + 0x10,0x14,
                            g_pPeaks + (longlong)(int)local_a * 0x14 + 0x10,0x14);
        if (local_10 != 0) {
          StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                            "../../src/Alg/Peak/PeakData.c",0x8b,"rc == EOK");
          HardwareAnalyzer_SetSoftException();
        }
        local_10 = memcpy_s(g_pPeaks + (longlong)(int)local_a * 0x14 + 0x10,0x14,local_28,0x14);
        if (local_10 != 0) {
          StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                            "../../src/Alg/Peak/PeakData.c",0x8d,"rc == EOK");
          HardwareAnalyzer_SetSoftException();
        }
      }
      else if (*(short *)(g_pPeaks + (longlong)(int)local_9 * 0x14 + 0x18) ==
               *(short *)(g_pPeaks + (longlong)(int)local_a * 0x14 + 0x18)) {
        *(short *)(g_pPeaks + (longlong)(int)local_a * 0x14 + 0x18) =
             *(short *)(g_pPeaks + (longlong)(int)local_a * 0x14 + 0x18) + 1;
        *(short *)(*(longlong *)_refptr_g_tsaBufDif +
                  (ulonglong)
                  ((uint)g_pPeaks[(longlong)(int)local_a * 0x14 + 0x15].bPeakCount +
                   (uint)g_pPeaks[(longlong)(int)local_a * 0x14 + 0x16].bPeakCount *
                   (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2) =
             *(short *)((ulonglong)
                        ((uint)g_pPeaks[(longlong)(int)local_a * 0x14 + 0x15].bPeakCount +
                         (uint)g_pPeaks[(longlong)(int)local_a * 0x14 + 0x16].bPeakCount *
                         (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
                       *(longlong *)_refptr_g_tsaBufDif) + 1;
      }
    }
  }
  return;
}


// ============================================================================
// Function: Peak_SortInSigDescending @ 6ba66412
// ============================================================================

void Peak_SortInSigDescending(void)

{
  undefined1 local_28 [24];
  errno_t local_10;
  char local_a;
  char local_9;
  
                    /* 0x26412  996  Peak_SortInSigDescending */
  for (local_9 = '\0'; local_a = local_9, (int)local_9 < (char)g_pPeaks->bPeakCount + -1;
      local_9 = local_9 + '\x01') {
    while (local_a = local_a + '\x01', (int)local_a < (int)(uint)g_pPeaks->bPeakCount) {
      if (*(short *)(g_pPeaks + (longlong)(int)local_9 * 0x14 + 0x18) <
          *(short *)(g_pPeaks + (longlong)(int)local_a * 0x14 + 0x18)) {
        local_10 = memcpy_s(local_28,0x14,g_pPeaks + (longlong)(int)local_9 * 0x14 + 0x10,0x14);
        if (local_10 != 0) {
          StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                            "../../src/Alg/Peak/PeakData.c",0xa2,"rc == EOK");
          HardwareAnalyzer_SetSoftException();
        }
        local_10 = memcpy_s(g_pPeaks + (longlong)(int)local_9 * 0x14 + 0x10,0x14,
                            g_pPeaks + (longlong)(int)local_a * 0x14 + 0x10,0x14);
        if (local_10 != 0) {
          StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                            "../../src/Alg/Peak/PeakData.c",0xa4,"rc == EOK");
          HardwareAnalyzer_SetSoftException();
        }
        local_10 = memcpy_s(g_pPeaks + (longlong)(int)local_a * 0x14 + 0x10,0x14,local_28,0x14);
        if (local_10 != 0) {
          StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                            "../../src/Alg/Peak/PeakData.c",0xa6,"rc == EOK");
          HardwareAnalyzer_SetSoftException();
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: Peak_IDTracking @ 6ba68167
// ============================================================================

void Peak_IDTracking(void)

{
  byte local_a;
  byte local_9;
  
                    /* 0x28167  968  Peak_IDTracking */
  **(undefined1 **)_refptr_g_touchesPtr = 0;
  local_9 = 0;
  for (local_a = 0; (local_9 < g_pPeaks->bPeakCount && (local_a != 0x14)); local_a = local_a + 1) {
    *(ushort *)((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x15e) =
         (ushort)g_pPeaks[(longlong)(int)(uint)local_9 * 0x14 + 0x15].bPeakCount * 0x100 + 0x80;
    *(ushort *)((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x160) =
         (ushort)g_pPeaks[(longlong)(int)(uint)local_9 * 0x14 + 0x16].bPeakCount * 0x100 + 0x80;
    **(char **)_refptr_g_touchesPtr = **(char **)_refptr_g_touchesPtr + '\x01';
    local_9 = local_9 + 1;
  }
  IDT_Process('\x01');
  for (local_a = 0; (local_a < **(byte **)_refptr_g_touchesPtr && (local_a != 0x14));
      local_a = local_a + 1) {
    g_pPeaks[(longlong)(int)(uint)local_a * 0x14 + 0x14].bPeakCount =
         *(uchar *)((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x154
                   );
  }
  return;
}


// ============================================================================
// Function: Peak_GetMaxPeak @ 6ba683f0
// ============================================================================

int Peak_GetMaxPeak(void)

{
  int local_10;
  uchar local_9;
  
  local_10 = -0x80000000;
  local_9 = (*_refptr_g_peaksPtr)->bPeakCount;
  while (local_9 = local_9 + 0xff, -1 < (char)local_9) {
    if (local_10 < *(short *)(*_refptr_g_peaksPtr + (longlong)(int)(char)local_9 * 0x14 + 0x18)) {
      local_10 = (int)*(short *)(*_refptr_g_peaksPtr + (longlong)(int)(char)local_9 * 0x14 + 0x18);
    }
  }
  return local_10;
}


// ============================================================================
// Function: Peak_Z1Filter @ 6ba6848b
// ============================================================================

void Peak_Z1Filter(short param_1)

{
  uchar local_9;
  
  local_9 = (*_refptr_g_peaksPtr)->bPeakCount;
  while (local_9 = local_9 + 0xff, -1 < (char)local_9) {
    if (*(short *)(*_refptr_g_peaksPtr + (longlong)(int)(char)local_9 * 0x14 + 0x18) < param_1) {
      Peak_Remove(local_9);
    }
  }
  return;
}


// ============================================================================
// Function: Peak_Z8Filter @ 6ba68502
// ============================================================================

void Peak_Z8Filter(void)

{
  short sVar1;
  uchar local_9;
  
  local_9 = (*_refptr_g_peaksPtr)->bPeakCount;
  while (local_9 = local_9 + 0xff, -1 < (char)local_9) {
    sVar1 = *(short *)(*_refptr_g_peaksPtr + (longlong)(int)(char)local_9 * 0x14 + 0x18);
    if (sVar1 < 0) {
      sVar1 = sVar1 + 0x1f;
    }
    if (*(int *)(*_refptr_g_peaksPtr + (longlong)(int)(char)local_9 * 0x14 + 0x1c) <
        (int)(sVar1 >> 5)) {
      Peak_Remove(local_9);
    }
  }
  return;
}


// ============================================================================
// Function: Peak_IsToExcludeSideRegion @ 6ba685ac
// ============================================================================

undefined8 Peak_IsToExcludeSideRegion(void)

{
                    /* 0x285ac  989  Peak_IsToExcludeSideRegion */
  return 0;
}


// ============================================================================
// Function: Peak_DetectInRange @ 6ba685d7
// ============================================================================

void Peak_DetectInRange(short param_1,byte param_2,byte param_3,byte param_4,byte param_5)

{
  short sVar1;
  bool bVar2;
  bool bVar3;
  bool bVar4;
  char cVar5;
  ushort uVar6;
  bool bVar7;
  bool bVar8;
  short local_14;
  byte local_12;
  byte local_11;
  ushort local_a;
  
                    /* 0x285d7  954  Peak_DetectInRange */
  bVar4 = false;
  Peak_Clean();
  cVar5 = Peak_IsToExcludeSideRegion();
  if ((cVar5 != '\0') && (cVar5 = TouchThold_ToeUseSpecialThold(), cVar5 != '\0')) {
    bVar4 = true;
  }
  for (local_11 = param_2; local_11 <= param_3; local_11 = local_11 + 1) {
    local_a = ((ushort)param_4 + (ushort)local_11 * (ushort)_refptr_g_tsaPrmtFlash->bCols) - 1;
    for (local_12 = param_4; local_12 <= param_5; local_12 = local_12 + 1) {
      local_a = local_a + 1;
      sVar1 = *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)local_a * 2);
      local_14 = param_1;
      if ((bVar4) &&
         (((local_12 == 1 || ((uint)local_12 == _refptr_g_tsaPrmtFlash->bCols - 2)) ||
          ((uint)local_11 == _refptr_g_tsaPrmtFlash->bRows - 1)))) {
        local_14 = *(short *)(_refptr_g_tsaPrmtDynamic + 10);
      }
      if (local_14 <= sVar1) {
        bVar7 = param_5 <= local_12;
        bVar8 = param_3 <= local_11;
        bVar2 = local_12 <= param_4;
        bVar3 = local_11 <= param_2;
        uVar6 = local_a + _refptr_g_tsaPrmtFlash->bCols;
        if ((((((bVar8) || (bVar7)) ||
              (*(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)(ushort)(uVar6 + 1) * 2) <=
               sVar1)) &&
             ((bVar8 || (*(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)uVar6 * 2) <=
                         sVar1)))) &&
            (((bVar8 || ((bVar2 || (*(short *)(*(longlong *)_refptr_g_tsaBufDif +
                                              (ulonglong)(ushort)(uVar6 - 1) * 2) <= sVar1)))) &&
             ((bVar7 || (*(short *)(*(longlong *)_refptr_g_tsaBufDif + ((ulonglong)local_a + 1) * 2)
                         <= sVar1)))))) &&
           ((bVar2 || (*(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)local_a * 2 + -2) <
                       sVar1)))) {
          uVar6 = local_a - _refptr_g_tsaPrmtFlash->bCols;
          if ((((bVar3) ||
               ((bVar7 || (*(short *)(*(longlong *)_refptr_g_tsaBufDif +
                                     (ulonglong)(ushort)(uVar6 + 1) * 2) < sVar1)))) &&
              ((bVar3 || (*(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)uVar6 * 2) <
                          sVar1)))) &&
             ((((bVar3 || (bVar2)) ||
               (*(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)(ushort)(uVar6 - 1) * 2) <
                sVar1)) &&
              ((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags2 & 0x20) == 0 ||
               (cVar5 = PressureDrift_Detect(local_12,local_11), cVar5 == '\0')))))) {
            Peak_Insert(local_12,local_11);
          }
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: Peak_IsFullIncellSDDetected @ 6ba689c9
// ============================================================================

uchar Peak_IsFullIncellSDDetected(void)

{
  char cVar1;
  uchar local_a;
  byte local_9;
  
  if ((*_refptr_g_peaksPtr)->bPeakCount == '\0') {
    local_a = '\0';
  }
  else {
    local_a = (*_refptr_g_tsaStaticPtr)->bFullIncellSignalDisparity;
    if ((short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax * 3 <
        (int)*(short *)(*_refptr_g_peaksPtr + 2)) {
      if ((byte)(*_refptr_g_tsaStaticPtr)->field_0xa0 < 6) {
        (*_refptr_g_tsaStaticPtr)->field_0xa0 = (*_refptr_g_tsaStaticPtr)->field_0xa0 + '\x01';
      }
      else {
        *(void **)&(*_refptr_g_tsaStaticPtr)->field_0x78 =
             (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
      }
    }
    else {
      (*_refptr_g_tsaStaticPtr)->field_0xa0 = 0;
    }
    if ((ulonglong)
        ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
        *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x78) < 0x3e9) {
      local_a = '\0';
    }
    else if ((int)*(short *)(*_refptr_g_peaksPtr + 2) <
             (short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax * 2) {
      if ((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold * 2 <
          *(int *)&_refptr_g_tsaPrpt->field_0xc8) {
        for (local_9 = 0; local_9 < (*_refptr_g_peaksPtr)->bPeakCount; local_9 = local_9 + 1) {
          if (4 < (*_refptr_g_peaksPtr)->bPeakCount) {
            local_a = '\x01';
          }
          cVar1 = Peak_IsFullIncellInsideSensor(local_9);
          if ((cVar1 != '\0') &&
             (*(short *)(*_refptr_g_peaksPtr + 2) * 3 <
              *(short *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)local_9 * 0x14 + 0x18) * 4)) {
            local_a = '\x01';
          }
        }
      }
      else {
        local_a = '\0';
      }
    }
  }
  return local_a;
}


// ============================================================================
// Function: Peak_needUDFPLargeFingerWorkaround @ 6ba68bc6
// ============================================================================

undefined8 Peak_needUDFPLargeFingerWorkaround(void)

{
  int iVar1;
  char *_Str1;
  
                    /* 0x28bc6  997  Peak_needUDFPLargeFingerWorkaround */
  _Str1 = (char *)TSA_GetPrmtName();
  iVar1 = strncmp(_Str1,"P115",4);
  if ((((iVar1 == 0) && (iVar1 = UDFPEvent_IsEnabled(), iVar1 != 0)) &&
      (0x30d4 < *(int *)(*_refptr_g_peaksPtr + 8))) &&
     ((uint)_refptr_g_tsaPrmtFlash->bRows * 2 + (uint)_refptr_g_tsaPrmtFlash->bRows <
      (uint)(*_refptr_g_peaksPtr)[0xc].bPeakCount * 4)) {
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: Peak_Remove @ 6ba65b60
// ============================================================================

void Peak_Remove(byte param_1)

{
  errno_t eVar1;
  byte local_9;
  
                    /* 0x25b60  993  Peak_Remove */
  if (0x14 < g_pPeaks->bPeakCount) {
    if (0x14 < g_pPeaks->bPeakCount) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/Peak/PeakData.c"
                        ,0x33,"PeaksPrpt(num) <= TSA_MAX_PEAK_NUM");
      HardwareAnalyzer_SetSoftException();
    }
    g_pPeaks->bPeakCount = '\x14';
  }
  if (param_1 < g_pPeaks->bPeakCount) {
    for (local_9 = param_1; (int)(uint)local_9 < (int)(g_pPeaks->bPeakCount - 1);
        local_9 = local_9 + 1) {
      eVar1 = memcpy_s(g_pPeaks + (longlong)(int)(uint)local_9 * 0x14 + 0x10,0x14,
                       g_pPeaks + (longlong)(int)(local_9 + 1) * 0x14 + 0x10,0x14);
      if (eVar1 != 0) {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                          "../../src/Alg/Peak/PeakData.c",0x3c,"rc == EOK");
        HardwareAnalyzer_SetSoftException();
      }
    }
    g_pPeaks->bPeakCount = g_pPeaks->bPeakCount + 0xff;
  }
  return;
}


// ============================================================================
// Function: Peak_Clean @ 6ba659b0
// ============================================================================

void Peak_Clean(void)

{
  int iVar1;
  
                    /* 0x259b0  953  Peak_Clean */
  iVar1 = memset_s(g_pPeaks,0x1a0,0,0x1a0);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/Peak/PeakData.c",
                      0x19,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  *(undefined2 *)(g_pPeaks + 2) = 0x8000;
  *(undefined2 *)(g_pPeaks + 4) = 0x7fff;
  *(undefined4 *)(g_pPeaks + 8) = 0x80000000;
  return;
}


// ============================================================================
// Function: Peak_Insert @ 6ba65e68
// ============================================================================

uint Peak_Insert(byte param_1,byte param_2)

{
  char cVar1;
  uint local_10;
  uint local_c;
  
                    /* 0x25e68  969  Peak_Insert */
  if (g_pPeaks->bPeakCount < 0x14) {
    Peak_Fill(g_pPeaks->bPeakCount,param_1,param_2);
    cVar1 = Peak_IsOnOuterEdge(g_pPeaks->bPeakCount);
    if (cVar1 == '\0') {
      g_pPeaks[1].bPeakCount = g_pPeaks[1].bPeakCount + '\x01';
    }
    g_pPeaks->bPeakCount = g_pPeaks->bPeakCount + '\x01';
    local_10 = g_pPeaks->bPeakCount - 1;
  }
  else {
    local_10 = 0;
    cVar1 = TSALog_RtEnable(1,1,1);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\nPeak Buffer Overflow!!!");
    }
    for (local_c = 1; local_c < 0x14; local_c = local_c + 1) {
      if (*(short *)(g_pPeaks + (ulonglong)local_c * 0x14 + 0x18) <
          *(short *)(g_pPeaks + (ulonglong)local_10 * 0x14 + 0x18)) {
        local_10 = local_c;
      }
    }
    if (*(short *)(g_pPeaks + (ulonglong)local_10 * 0x14 + 0x18) <
        *(short *)((ulonglong)
                   ((uint)param_1 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) *
                   2 + *(longlong *)_refptr_g_tsaBufDif)) {
      Peak_Fill(local_10 & 0xff,param_1,param_2);
    }
    else {
      local_10 = 0x14;
    }
  }
  return local_10;
}


// ============================================================================
// Function: Peak_IsFullIncellInsideSensor @ 6ba68944
// ============================================================================

bool Peak_IsFullIncellInsideSensor(byte param_1)

{
  return (int)*(short *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)param_1 * 0x14 + 0x18) <
         *(int *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)param_1 * 0x14 + 0x1c) / 5;
}


// ============================================================================
// Function: Peak_IsOnLine @ 6ba679f2
// ============================================================================

undefined1 Peak_IsOnLine(int param_1,uchar param_2,byte param_3)

{
  undefined1 local_9;
  
                    /* 0x279f2  980  Peak_IsOnLine */
  local_9 = 0;
  if (((param_1 == 1) &&
      (g_pPeaks[(longlong)(int)(uint)param_3 * 0x14 + 0x16].bPeakCount == param_2)) ||
     ((param_1 == 2 && (g_pPeaks[(longlong)(int)(uint)param_3 * 0x14 + 0x15].bPeakCount == param_2))
     )) {
    local_9 = 1;
  }
  return local_9;
}


// ============================================================================
// Function: Peak_IsOnInnerEdgeDim1 @ 6ba66868
// ============================================================================

undefined8 Peak_IsOnInnerEdgeDim1(undefined1 param_1)

{
  char cVar1;
  
                    /* 0x26868  974  Peak_IsOnInnerEdgeDim1 */
  cVar1 = Peak_IsOnInnerEdgeDim1Near(param_1);
  if ((cVar1 == '\0') && (cVar1 = Peak_IsOnInnerEdgeDim1Far(param_1), cVar1 == '\0')) {
    return 0;
  }
  return 1;
}


// ============================================================================
// Function: Peak_IsTouchValid @ 6ba65a3e
// ============================================================================

undefined4 Peak_IsTouchValid(byte param_1)

{
  uint uVar1;
  
                    /* 0x25a3e  990  Peak_IsTouchValid */
  uVar1 = *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c
                   );
  return CONCAT31((int3)(uVar1 >> 8),uVar1 < 5);
}


// ============================================================================
// Function: Peak_Fill @ 6ba65cb8
// ============================================================================

void Peak_Fill(byte param_1,byte param_2,byte param_3)

{
  short sVar1;
  
                    /* 0x25cb8  955  Peak_Fill */
  sVar1 = *(short *)((ulonglong)
                     ((uint)param_2 + (uint)param_3 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)
                     * 2 + *(longlong *)_refptr_g_tsaBufDif);
  *(short *)(g_pPeaks + (longlong)(int)(uint)param_1 * 0x14 + 0x18) = sVar1;
  g_pPeaks[(longlong)(int)(uint)param_1 * 0x14 + 0x15].bPeakCount = param_2;
  g_pPeaks[(longlong)(int)(uint)param_1 * 0x14 + 0x16].bPeakCount = param_3;
  Peak_CalcZn(param_1);
  if (*(short *)(g_pPeaks + 2) < sVar1) {
    *(short *)(g_pPeaks + 2) = sVar1;
  }
  if (sVar1 < *(short *)(g_pPeaks + 4)) {
    *(short *)(g_pPeaks + 4) = sVar1;
  }
  if (*(int *)(g_pPeaks + 8) < *(int *)(g_pPeaks + (longlong)(int)(uint)param_1 * 0x14 + 0x20)) {
    *(undefined4 *)(g_pPeaks + 8) =
         *(undefined4 *)(g_pPeaks + (longlong)(int)(uint)param_1 * 0x14 + 0x20);
  }
  return;
}


// ============================================================================
// Function: Peak_IsOnOuterEdge @ 6ba66a9d
// ============================================================================

undefined8 Peak_IsOnOuterEdge(undefined1 param_1)

{
  char cVar1;
  
                    /* 0x26a9d  982  Peak_IsOnOuterEdge */
  cVar1 = Peak_IsOnOuterEdgeDim1(param_1);
  if ((cVar1 == '\0') && (cVar1 = Peak_IsOnOuterEdgeDim2(param_1), cVar1 == '\0')) {
    return 0;
  }
  return 1;
}


// ============================================================================
// Function: Peak_GetIdxBySig @ 6ba666f3
// ============================================================================

byte Peak_GetIdxBySig(short param_1)

{
  byte local_9;
  
                    /* 0x266f3  963  Peak_GetIdxBySig */
  local_9 = 0;
  while( true ) {
    if (g_pPeaks->bPeakCount <= local_9) {
      return 0x14;
    }
    if (*(short *)(g_pPeaks + (longlong)(int)(uint)local_9 * 0x14 + 0x18) == param_1) break;
    local_9 = local_9 + 1;
  }
  return local_9;
}


// ============================================================================
// Function: Peak_GetSquareDistance @ 6ba66c86
// ============================================================================

int Peak_GetSquareDistance(byte param_1,byte param_2)

{
                    /* 0x26c86  966  Peak_GetSquareDistance */
  return Sensor_GetSquareDistance
            (g_pPeaks[(longlong)(int)(uint)param_1 * 0x14 + 0x15].bPeakCount,
             g_pPeaks[(longlong)(int)(uint)param_1 * 0x14 + 0x16].bPeakCount,
             g_pPeaks[(longlong)(int)(uint)param_2 * 0x14 + 0x15].bPeakCount,
             g_pPeaks[(longlong)(int)(uint)param_2 * 0x14 + 0x16].bPeakCount);
}


// ============================================================================
// Function: Peak_IsOnInnerEdgeDim1Near @ 6ba66764
// ============================================================================

bool Peak_IsOnInnerEdgeDim1Near(byte param_1)

{
                    /* 0x26764  976  Peak_IsOnInnerEdgeDim1Near */
  return g_pPeaks[(longlong)(int)(uint)param_1 * 0x14 + 0x15].bPeakCount == '\x01';
}


// ============================================================================
// Function: Peak_IsOnInnerEdgeDim1Far @ 6ba6679b
// ============================================================================

undefined4 Peak_IsOnInnerEdgeDim1Far(byte param_1)

{
  uint uVar1;
  
                    /* 0x2679b  975  Peak_IsOnInnerEdgeDim1Far */
  uVar1 = _refptr_g_tsaPrmtFlash->bCols - 2;
  return CONCAT31((int3)(uVar1 >> 8),
                  g_pPeaks[(longlong)(int)(uint)param_1 * 0x14 + 0x15].bPeakCount == uVar1);
}


// ============================================================================
// Function: Peak_IsOnOuterEdgeDim1 @ 6ba66a23
// ============================================================================

undefined8 Peak_IsOnOuterEdgeDim1(undefined1 param_1)

{
  char cVar1;
  
                    /* 0x26a23  983  Peak_IsOnOuterEdgeDim1 */
  cVar1 = Peak_IsOnOuterEdgeDim1Near(param_1);
  if ((cVar1 == '\0') && (cVar1 = Peak_IsOnOuterEdgeDim1Far(param_1), cVar1 == '\0')) {
    return 0;
  }
  return 1;
}


// ============================================================================
// Function: Peak_CalcZn @ 6ba67c89
// ============================================================================

void Peak_CalcZn(byte param_1)

{
  byte bVar1;
  byte bVar2;
  short sVar3;
  uchar uVar4;
  byte bVar5;
  ushort uVar6;
  ushort uVar7;
  bool bVar8;
  int local_38;
  int local_34;
  int local_30;
  int local_2c;
  uchar local_15;
  int local_14;
  int local_10;
  int local_c;
  
                    /* 0x27c89  952  Peak_CalcZn */
  local_14 = 0;
  local_15 = '\0';
  bVar1 = g_pPeaks[(longlong)(int)(uint)param_1 * 0x14 + 0x15].bPeakCount;
  bVar2 = g_pPeaks[(longlong)(int)(uint)param_1 * 0x14 + 0x16].bPeakCount;
  uVar6 = (ushort)bVar1 + (ushort)bVar2 * (ushort)_refptr_g_tsaPrmtFlash->bCols;
  sVar3 = *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)uVar6 * 2);
  bVar5 = _refptr_g_tsaPrmtFlash->bCols - 1;
  local_38 = (int)sVar3;
  local_34 = (int)sVar3;
  local_30 = (int)sVar3;
  local_2c = (int)sVar3;
  local_10 = (int)sVar3;
  if (bVar2 < (byte)(_refptr_g_tsaPrmtFlash->bRows - 1)) {
    uVar7 = uVar6 + _refptr_g_tsaPrmtFlash->bCols;
    bVar8 = bVar1 < bVar5;
    if (bVar8) {
      sVar3 = *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)(ushort)(uVar7 + 1) * 2);
      local_2c = sVar3 + local_2c;
      local_14 = (int)sVar3;
    }
    sVar3 = *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)uVar7 * 2);
    local_30 = sVar3 + local_30;
    local_2c = sVar3 + local_2c;
    local_14 = local_14 + sVar3;
    local_10 = local_10 + sVar3;
    local_15 = bVar8 + '\x01';
    if (bVar1 != 0) {
      sVar3 = *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)(ushort)(uVar7 - 1) * 2);
      local_30 = sVar3 + local_30;
      local_14 = local_14 + sVar3;
      local_15 = bVar8 + '\x02';
    }
  }
  if (bVar1 < bVar5) {
    sVar3 = *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)(ushort)(uVar6 + 1) * 2);
    local_34 = sVar3 + local_34;
    local_2c = sVar3 + local_2c;
    local_14 = local_14 + sVar3;
    if (((*_refptr_g_tsaStaticPtr)->dwProcessingState & 0x10) == 0) {
      local_10 = local_10 + sVar3;
    }
    local_15 = local_15 + '\x01';
  }
  if (bVar1 != 0) {
    sVar3 = *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)(ushort)(uVar6 - 1) * 2);
    local_38 = sVar3 + local_38;
    local_30 = sVar3 + local_30;
    local_14 = local_14 + sVar3;
    if (((*_refptr_g_tsaStaticPtr)->dwProcessingState & 0x10) == 0) {
      local_10 = local_10 + sVar3;
    }
    local_15 = local_15 + '\x01';
  }
  uVar4 = local_15;
  if (bVar2 != 0) {
    uVar6 = uVar6 - _refptr_g_tsaPrmtFlash->bCols;
    if (bVar1 < bVar5) {
      sVar3 = *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)(ushort)(uVar6 + 1) * 2);
      local_34 = sVar3 + local_34;
      local_14 = local_14 + sVar3;
      local_15 = local_15 + '\x01';
    }
    sVar3 = *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)uVar6 * 2);
    local_38 = sVar3 + local_38;
    local_34 = sVar3 + local_34;
    local_14 = local_14 + sVar3;
    local_10 = local_10 + sVar3;
    uVar4 = local_15 + '\x01';
    if (bVar1 != 0) {
      sVar3 = *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)(ushort)(uVar6 - 1) * 2);
      local_38 = sVar3 + local_38;
      local_14 = local_14 + sVar3;
      uVar4 = local_15 + '\x02';
    }
  }
  local_15 = uVar4;
  local_c = local_38;
  if (local_38 < local_34) {
    local_c = local_34;
  }
  if (local_c < local_30) {
    local_c = local_30;
  }
  if (local_c < local_2c) {
    local_c = local_2c;
  }
  if (((*_refptr_g_tsaStaticPtr)->dwProcessingState & 0x10) == 0) {
    if (local_c <= local_10) {
      local_c = local_10;
    }
    *(int *)(g_pPeaks + (longlong)(int)(uint)param_1 * 0x14 + 0x20) = local_c;
  }
  else {
    *(int *)(g_pPeaks + (longlong)(int)(uint)param_1 * 0x14 + 0x20) = local_10;
  }
  *(int *)(g_pPeaks + (longlong)(int)(uint)param_1 * 0x14 + 0x1c) = local_14;
  g_pPeaks[(longlong)(int)(uint)param_1 * 0x14 + 0x17].bPeakCount = local_15;
  return;
}


// ============================================================================
// Function: Peak_IsOnOuterEdgeDim2 @ 6ba66a60
// ============================================================================

undefined8 Peak_IsOnOuterEdgeDim2(undefined1 param_1)

{
  char cVar1;
  
                    /* 0x26a60  986  Peak_IsOnOuterEdgeDim2 */
  cVar1 = Peak_IsOnOuterEdgeDim2Near(param_1);
  if ((cVar1 == '\0') && (cVar1 = Peak_IsOnOuterEdgeDim2Far(param_1), cVar1 == '\0')) {
    return 0;
  }
  return 1;
}


// ============================================================================
// Function: Peak_IsOnInnerEdgeDim2Near @ 6ba667e6
// ============================================================================

bool Peak_IsOnInnerEdgeDim2Near(byte param_1)

{
                    /* 0x267e6  979  Peak_IsOnInnerEdgeDim2Near */
  return g_pPeaks[(longlong)(int)(uint)param_1 * 0x14 + 0x16].bPeakCount == '\x01';
}


// ============================================================================
// Function: Peak_IsOnInnerEdgeDim2Far @ 6ba6681d
// ============================================================================

undefined4 Peak_IsOnInnerEdgeDim2Far(byte param_1)

{
  uint uVar1;
  
                    /* 0x2681d  978  Peak_IsOnInnerEdgeDim2Far */
  uVar1 = _refptr_g_tsaPrmtFlash->bRows - 2;
  return CONCAT31((int3)(uVar1 >> 8),
                  g_pPeaks[(longlong)(int)(uint)param_1 * 0x14 + 0x16].bPeakCount == uVar1);
}


// ============================================================================
// Function: Peak_IsOnOuterEdgeDim1Near @ 6ba6691f
// ============================================================================

bool Peak_IsOnOuterEdgeDim1Near(byte param_1)

{
                    /* 0x2691f  985  Peak_IsOnOuterEdgeDim1Near */
  return g_pPeaks[(longlong)(int)(uint)param_1 * 0x14 + 0x15].bPeakCount == '\0';
}


// ============================================================================
// Function: Peak_IsOnOuterEdgeDim1Far @ 6ba66956
// ============================================================================

undefined4 Peak_IsOnOuterEdgeDim1Far(byte param_1)

{
  uint uVar1;
  
                    /* 0x26956  984  Peak_IsOnOuterEdgeDim1Far */
  uVar1 = _refptr_g_tsaPrmtFlash->bCols - 1;
  return CONCAT31((int3)(uVar1 >> 8),
                  g_pPeaks[(longlong)(int)(uint)param_1 * 0x14 + 0x15].bPeakCount == uVar1);
}


// ============================================================================
// Function: Peak_IsOnOuterEdgeDim2Near @ 6ba669a1
// ============================================================================

bool Peak_IsOnOuterEdgeDim2Near(byte param_1)

{
                    /* 0x269a1  988  Peak_IsOnOuterEdgeDim2Near */
  return g_pPeaks[(longlong)(int)(uint)param_1 * 0x14 + 0x16].bPeakCount == '\0';
}


// ============================================================================
// Function: Peak_IsOnOuterEdgeDim2Far @ 6ba669d8
// ============================================================================

undefined4 Peak_IsOnOuterEdgeDim2Far(byte param_1)

{
  uint uVar1;
  
                    /* 0x269d8  987  Peak_IsOnOuterEdgeDim2Far */
  uVar1 = _refptr_g_tsaPrmtFlash->bRows - 1;
  return CONCAT31((int3)(uVar1 >> 8),
                  g_pPeaks[(longlong)(int)(uint)param_1 * 0x14 + 0x16].bPeakCount == uVar1);
}


// ============================================================================
// Function: Peak_GetZ9 @ 6ba65a72
// ============================================================================

int Peak_GetZ9(byte param_1)

{
                    /* 0x25a72  967  Peak_GetZ9 */
  return *(int *)(g_pPeaks + (longlong)(int)(uint)param_1 * 0x14 + 0x1c) +
         (int)*(short *)(g_pPeaks + (longlong)(int)(uint)param_1 * 0x14 + 0x18);
}


// ============================================================================
// Function: Peak_GetDistToEdge @ 6ba67c47
// ============================================================================

byte Peak_GetDistToEdge(undefined1 param_1)

{
  byte bVar1;
  byte bVar2;
  
                    /* 0x27c47  960  Peak_GetDistToEdge */
  bVar1 = Peak_GetDim1DistToEdge(param_1);
  bVar2 = Peak_GetDim2DistToEdge(param_1);
  if (bVar1 <= bVar2) {
    bVar2 = bVar1;
  }
  return bVar2;
}


// ============================================================================
// Function: Peak_GetDim1DistToEdge @ 6ba67b4d
// ============================================================================

byte Peak_GetDim1DistToEdge(byte param_1)

{
  byte local_9;
  
                    /* 0x27b4d  958  Peak_GetDim1DistToEdge */
  local_9 = g_pPeaks[(longlong)(int)(uint)param_1 * 0x14 + 0x15].bPeakCount;
  if ((int)((_refptr_g_tsaPrmtFlash->bCols - 1) - (uint)local_9) < (int)(uint)local_9) {
    local_9 = (_refptr_g_tsaPrmtFlash->bCols - local_9) - 1;
  }
  return local_9;
}


// ============================================================================
// Function: Peak_GetDim2DistToEdge @ 6ba67bca
// ============================================================================

byte Peak_GetDim2DistToEdge(byte param_1)

{
  byte local_9;
  
                    /* 0x27bca  959  Peak_GetDim2DistToEdge */
  local_9 = g_pPeaks[(longlong)(int)(uint)param_1 * 0x14 + 0x16].bPeakCount;
  if ((int)((_refptr_g_tsaPrmtFlash->bRows - 1) - (uint)local_9) < (int)(uint)local_9) {
    local_9 = (_refptr_g_tsaPrmtFlash->bRows - local_9) - 1;
  }
  return local_9;
}


// ============================================================================
// Function: Peak_IsInNxNBox @ 6ba67a7e
// ============================================================================

undefined1 Peak_IsInNxNBox(undefined1 param_1,undefined1 param_2,undefined1 param_3)

{
  char cVar1;
  byte local_a;
  
                    /* 0x27a7e  970  Peak_IsInNxNBox */
  local_a = 0;
  while( true ) {
    if (g_pPeaks->bPeakCount <= local_a) {
      return 0;
    }
    cVar1 = Sensor_IsInNxNBox(g_pPeaks[(longlong)(int)(uint)local_a * 0x14 + 0x15].bPeakCount,
                              g_pPeaks[(longlong)(int)(uint)local_a * 0x14 + 0x16].bPeakCount,
                              param_1,param_2,param_3);
    if (cVar1 != '\0') break;
    local_a = local_a + 1;
  }
  return 1;
}


// ============================================================================
// Function: Peak_GetDistance @ 6ba66bbd
// ============================================================================

byte Peak_GetDistance(byte param_1,byte param_2)

{
                    /* 0x26bbd  961  Peak_GetDistance */
  return (byte)Sensor_GetDistance(g_pPeaks[(longlong)(int)(uint)param_1 * 0x14 + 0x15].bPeakCount,
                                 g_pPeaks[(longlong)(int)(uint)param_1 * 0x14 + 0x16].bPeakCount,
                                 g_pPeaks[(longlong)(int)(uint)param_2 * 0x14 + 0x15].bPeakCount,
                                 g_pPeaks[(longlong)(int)(uint)param_2 * 0x14 + 0x16].bPeakCount);
}


