#include "TSACoreInternal.h"

// ============================================================================
// Function: AntiTouch_Process @ 6bad7bbb
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* Runs common anti-touch processing. Current config has AFT feature off, so the AFT-mode
   post-report/reset branch is normally inactive. */

void AntiTouch_Process(void)

{
  char cAftModeOrResetState;
  
                    /* 0x97bbb  338  AntiTouch_Process */
  AntiTouch_CommonProcess();
  cAftModeOrResetState = TSAStatic_IsAFTModeEnabled();
  if (cAftModeOrResetState != '\0') {
    cAftModeOrResetState = BLSM_IsReset();
    if (cAftModeOrResetState == '\0') {
      AntiTouch_PostReport();
    }
    else {
      Touch_Clean();
      PrevTouch_Release();
      AntiTouch_Init();
    }
  }
  return;
}


// ============================================================================
// Function: AntiTouch_Report @ 6bad7bf7
// ============================================================================

void AntiTouch_Report(void)

{
  char cVar1;
  
                    /* 0x97bf7  339  AntiTouch_Report */
  if ((_refptr_g_tsaPrmtFlash[2].field_0x6c2 != '\0') &&
     (cVar1 = TSA_IsScreenLockScenario(), cVar1 == '\0')) {
    if ((*(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) & 0x80) != 0) {
      *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
           *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) | 0x1000000;
      return;
    }
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) | 0x2000000;
    return;
  }
  if ((g_antiTouchReportFlag == '\x01') &&
     (*(char *)(*(longlong *)_refptr_g_touchesPtr + 3) == '\0')) {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) | 0x2000000;
    g_antiTouchReportFlag = '\0';
  }
  if (*(char *)(*(longlong *)_refptr_g_touchesPtr + 3) != '\0') {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) | 0x1000000;
    g_antiTouchReportFlag = '\x01';
  }
  return;
}


// ============================================================================
// Function: AntiTouch_Init @ 6bad6010
// ============================================================================

void AntiTouch_Init(void)

{
                    /* 0x96010  336  AntiTouch_Init */
  g_antiTouchAbnormalTimeStamp = 0xffffffffffffffff;
  return;
}


// ============================================================================
// Function: AntiTouch_PostReport @ 6bad65e6
// ============================================================================

void AntiTouch_PostReport(void)

{
  undefined1 uVar1;
  byte local_9;
  
  for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
    uVar1 = PrevTouch_GetIdxBasedOnCurIdx(local_9);
    AntiTouch_PostReportUnit(local_9,uVar1);
  }
  return;
}


// ============================================================================
// Function: AntiTouch_CommonProcess @ 6bad7ad0
// ============================================================================

void AntiTouch_CommonProcess(void)

{
  char cVar1;
  undefined1 uVar2;
  longlong lVar3;
  byte local_9;
  
  AntiTouch_PreProcess();
  if (g_antiTouchAbnormalTimeStamp != -1) {
    lVar3 = TSAStatic_GetCurrTimeStamp();
    if ((ulonglong)(lVar3 - g_antiTouchAbnormalTimeStamp) < 500) {
      *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) =
           *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) | 0x20;
    }
  }
  cVar1 = Proximity_IsDetected();
  if (cVar1 != '\0') {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) | 0x20;
  }
  for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
    uVar2 = PrevTouch_GetIdxBasedOnCurIdx(local_9);
    AntiTouch_ProcessUnit(local_9,uVar2);
  }
  return;
}


// ============================================================================
// Function: AntiTouch_PostReportUnit @ 6bad6099
// ============================================================================

void AntiTouch_PostReportUnit(byte param_1,byte param_2)

{
  byte bVar1;
  bool bVar2;
  char cVar3;
  int iVar4;
  
  cVar3 = AntiTouch_IsToGoBackReport(param_2);
  if (cVar3 != '\0') {
    TouchReport_GoBackReport(param_1,param_2);
  }
  bVar2 = false;
  if (((*_refptr_g_tsaStaticPtr)->nField_0x284 == 0) &&
     (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x800000) != 0)) {
    iVar4 = UDFPEvent_IsCloseFingerArea
                      (*(undefined2 *)
                        ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x170),*(undefined2 *)
                                ((longlong)(int)(uint)param_1 * 0x168 +
                                 *(longlong *)_refptr_g_touchesPtr + 0x172));
    if (iVar4 != 0) {
      bVar2 = true;
    }
  }
  if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) &
      0x20000000) != 0) {
    bVar2 = true;
  }
  if (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
       & 0x20000) != 0) || (bVar2)) goto LAB_6bad64e8;
  *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) =
       *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) |
       0x8000000;
  if ((*(int *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c) ==
       0x20) ||
     (*(int *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c) ==
      0x10)) {
    cVar3 = Sensor_GetZoneIndex((char)((ushort)*(undefined2 *)
                                                ((longlong)*_refptr_g_prevTouchesPtr +
                                                (longlong)(int)(uint)param_2 * 0x478 + 0x162) >> 8),
                                (char)((ushort)*(undefined2 *)
                                                ((longlong)*_refptr_g_prevTouchesPtr +
                                                (longlong)(int)(uint)param_2 * 0x478 + 0x164) >> 8))
    ;
    if (cVar3 != -1) goto LAB_6bad62c8;
  }
  else {
LAB_6bad62c8:
    iVar4 = UDFPEvent_IsEnabled();
    if (iVar4 != 0) {
      iVar4 = UDFPEvent_IsCloseFingerArea
                        (*(undefined2 *)
                          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x170),
                         *(undefined2 *)
                          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x172));
      if ((iVar4 != 0) &&
         (10 < (ulonglong)
               (*(longlong *)
                 ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x148)
               - *(longlong *)
                  ((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_2 * 0x478 + 0x4b8)))) goto LAB_6bad63d9;
    }
    if ((ulonglong)
        (*(longlong *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x148) -
        *(longlong *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4b8)) <
        0x33) goto LAB_6bad64e8;
  }
LAB_6bad63d9:
  if ((((*(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) & 0x20) == 0) &&
      (*(char *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4e6)
       == '\0')) &&
     ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) &
      0x4000000) == 0)) {
    cVar3 = Touch_IsGrip(param_1);
    if (cVar3 == '\0') {
      *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) =
           *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1f0) & 0xf7ffffff;
      if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1f8) & 0x100000) != 0) {
        Touch_CreatePostReport(param_1,5);
      }
    }
  }
LAB_6bad64e8:
  if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c) &
      2) != 0) {
    if (g_antiTouchMaxTouchPkIdx < 0x14) {
      bVar1 = *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x23c);
      if (*(short *)(*_refptr_g_peaksPtr +
                    (longlong)(int)(uint)g_antiTouchMaxTouchPkIdx * 0x14 + 0x18) <
          *(short *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)bVar1 * 0x14 + 0x18)) {
        g_antiTouchMaxTouchPkIdx = bVar1;
      }
    }
    else {
      g_antiTouchMaxTouchPkIdx =
           *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x23c);
    }
  }
  return;
}


// ============================================================================
// Function: AntiTouch_PreProcess @ 6bad67d3
// ============================================================================

void AntiTouch_PreProcess(void)

{
  char cVar1;
  byte local_9;
  
  g_antiTouchNumGrip = '\0';
  g_antiTouchIirEllipseRateMax = 0;
  g_antiTouchMaxTouchPkIdx = 0x14;
  for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
    cVar1 = AntiTouch_IsGrip(local_9);
    if (cVar1 != '\0') {
      g_antiTouchNumGrip = g_antiTouchNumGrip + '\x01';
    }
    if (g_antiTouchIirEllipseRateMax <
        *(short *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1a2)
       ) {
      g_antiTouchIirEllipseRateMax =
           *(short *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1a2);
    }
  }
  return;
}


// ============================================================================
// Function: AntiTouch_ProcessUnit @ 6bad7719
// ============================================================================

void AntiTouch_ProcessUnit(byte param_1,byte param_2)

{
  char cVar1;
  ushort uVar2;
  ushort local_a;
  
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    local_a = *(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a8;
  }
  else {
    local_a = *(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a6;
  }
  if ((0x32 < *(ushort *)
               ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x188))
     && (cVar1 = AntiTouch_IsGrip(param_1), cVar1 == '\0')) {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) | 0x20;
    AntiTouch_CheckProximityFlag(param_1);
    g_antiTouchAbnormalTimeStamp = TSAStatic_GetCurrTimeStamp();
  }
  AntiTouch_ProcessProximityUnit(param_1,param_2);
  if (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214)
       & 0x200000) == 0) &&
     (((8 < *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x196) ||
       (8 < *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x197))) &&
      (0xf < *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x1a2))))) {
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0)
         | 0x4000000;
  }
  cVar1 = AntiTouch_GetPersistFlag(param_1,param_2);
  if (cVar1 == '\0') {
    if ((*(char *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_2 * 0x478 + 0x4e6) != '\0') &&
       (*(byte *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4e6
                 ) < 10)) {
      *(char *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4e6)
           = *(char *)((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_2 * 0x478 + 0x4e6) + -1;
    }
    if (*(byte *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4e7
                 ) < 10) {
      if ((*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1a2) < 10) &&
         (uVar2 = TSAOut_DisplayYPos(param_1), (uint)local_a < (uint)uVar2 + (uint)uVar2 * 2)) {
        *(char *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4e7
                 ) = *(char *)((longlong)*_refptr_g_prevTouchesPtr +
                              (longlong)(int)(uint)param_2 * 0x478 + 0x4e7) + '\x01';
        return;
      }
      *(undefined1 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4e7) = 0;
    }
  }
  else if (*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_2 * 0x478 + 0x4e6) < 10) {
    *(char *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4e6) =
         *(char *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_2 * 0x478 + 0x4e6) + '\x01';
  }
  return;
}


// ============================================================================
// Function: AntiTouch_IsToGoBackReport @ 6bad6021
// ============================================================================

undefined8 AntiTouch_IsToGoBackReport(byte param_1)

{
  char cVar1;
  undefined8 uVar2;
  
  cVar1 = Proximity_IsDetected();
  if ((cVar1 == '\0') &&
     ((*(byte *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x4e6)
       < 10 || (9 < *(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)param_1 * 0x478 + 0x4e7))))) {
    uVar2 = 0;
  }
  else {
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: AntiTouch_IsGrip @ 6bad6640
// ============================================================================

bool AntiTouch_IsGrip(byte param_1)

{
  char cVar1;
  bool local_9;
  
  cVar1 = Touch_IsGrip(param_1);
  local_9 = cVar1 != '\0';
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    if (*(ushort *)
         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22c) < 0x100)
    {
      local_9 = true;
    }
  }
  else if (*(ushort *)
            ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22e) <
           0x100) {
    local_9 = true;
  }
  return local_9;
}


// ============================================================================
// Function: AntiTouch_CheckProximityFlag @ 6bad6aee
// ============================================================================

void AntiTouch_CheckProximityFlag(undefined1 param_1)

{
  byte bVar1;
  
  bVar1 = TSA_RptTouchYCenter(param_1);
  if (bVar1 <= (byte)_refptr_g_tsaPrmtFlash[2].field_0x60c) {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) | 0x80;
  }
  return;
}


// ============================================================================
// Function: AntiTouch_GetPersistFlag @ 6bad6b3d
// ============================================================================

bool AntiTouch_GetPersistFlag(byte param_1,byte param_2)

{
  short sVar1;
  ushort uVar2;
  ushort uVar3;
  char cVar4;
  byte bVar5;
  short sVar6;
  bool local_19;
  
  local_19 = false;
  if ((0x5dc < (ulonglong)
               (*(longlong *)
                 ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x148)
               - *(longlong *)
                  ((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_2 * 0x478 + 0x4b8))) &&
     (*(ushort *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x24c)
      < 100)) {
    local_19 = ((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x800000) == 0;
    if (local_19) {
      *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) =
           *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1f0) | 0x4000000;
    }
    if (9 < *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x1a2)) {
      cVar4 = AntiTouch_IsGrip(param_1);
      if (cVar4 == '\0') {
        AntiTouch_CheckProximityFlag(param_1);
      }
    }
  }
  if (1 < (int)((uint)**(byte **)_refptr_g_touchesPtr - (uint)g_antiTouchNumGrip)) {
    sVar1 = *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x1a2);
    sVar6 = AntiTouch_GetEllipseRateThold(param_1);
    if ((sVar6 <= sVar1) &&
       (*(int *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1e0)
        != 3)) {
      *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) =
           *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1f0) | 0x4000000;
      local_19 = true;
      AntiTouch_CheckProximityFlag(param_1);
    }
  }
  if (3 < (int)((uint)**(byte **)_refptr_g_touchesPtr - (uint)g_antiTouchNumGrip)) {
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0)
         | 0x4000000;
    local_19 = true;
    AntiTouch_CheckProximityFlag(param_1);
  }
  uVar2 = *(ushort *)
           ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x188);
  bVar5 = AntiTouch_GetAxisMajorThold(param_1);
  if (bVar5 < uVar2) {
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0)
         | 0x4000000;
    local_19 = true;
  }
  uVar3 = *(ushort *)
           ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x18a);
  if ((((uint)uVar3 * 4 + (uint)uVar3 < (uint)uVar2 + (uint)uVar2) && (0xf < uVar2)) &&
     (*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) =
           *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1f0) | 0x4000000,
     uVar3 = *(ushort *)
              ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x18a),
     ((uint)uVar3 * 2 + (uint)uVar3) * 2 < (uint)uVar2 + (uint)uVar2)) {
    local_19 = true;
  }
  return local_19;
}


// ============================================================================
// Function: AntiTouch_ProcessProximityUnit @ 6bad6f59
// ============================================================================

void AntiTouch_ProcessProximityUnit(byte param_1,byte param_2)

{
  char cVar1;
  byte bVar2;
  
  if ((0x28 < *(ushort *)
               ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x188))
     && ((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 2 <
         *(short *)(*_refptr_g_peaksPtr +
                   (longlong)
                   (int)(uint)*(byte *)((longlong)(int)(uint)param_1 * 0x168 +
                                        *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x18)))
  {
    cVar1 = AntiTouch_IsGrip(param_1);
    if (cVar1 == '\0') {
      bVar2 = TSA_RptTouchYCenter(param_1);
      if (bVar2 <= (byte)_refptr_g_tsaPrmtFlash[2].field_0x60c) {
        *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) =
             *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) | 0x80;
      }
    }
  }
  if ((0x5dc < (ulonglong)
               (*(longlong *)
                 ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x148)
               - *(longlong *)
                  ((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_2 * 0x478 + 0x4b8))) &&
     (*(ushort *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x24c)
      < 100)) {
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0)
         | 0x4000000;
    if (9 < *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x1a2)) {
      cVar1 = AntiTouch_IsGrip(param_1);
      if (cVar1 == '\0') {
        bVar2 = TSA_RptTouchYCenter(param_1);
        if (bVar2 <= (byte)_refptr_g_tsaPrmtFlash[2].field_0x60c) {
          *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) =
               *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) | 0x80;
        }
      }
    }
  }
  if ((((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214)
        & 0x200000) == 0) &&
      (*(ushort *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x188)
       < 9)) &&
     (9 < *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1a2))) {
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0)
         | 0x4000000;
    bVar2 = TSA_RptTouchYCenter(param_1);
    if (bVar2 <= (byte)_refptr_g_tsaPrmtFlash[2].field_0x60c) {
      cVar1 = AntiTouch_IsGrip(param_1);
      if (cVar1 == '\0') {
        *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) =
             *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) | 0x80;
      }
    }
  }
  cVar1 = Proximity_IsDetected();
  if (cVar1 != '\0') {
    cVar1 = Proximity_IsValidTouch(param_1);
    if (cVar1 != '\0') {
      *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) =
           *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) | 0x80;
    }
  }
  cVar1 = AntiTouch_IsToProtectProximity(param_1);
  if (cVar1 != '\0') {
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
         | 0x400000;
  }
  if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) &
      0x400000) != 0) {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) | 0x100;
  }
  if (0x14 < *(ushort *)
              ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x188)) {
    bVar2 = TSA_RptTouchYGripCtd(param_1);
    if (((0xf < bVar2) &&
        (0xf < *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x1a2))) &&
       (200 < (ulonglong)
              (*(longlong *)
                ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x148) -
              *(longlong *)
               ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4b8))
       )) {
      bVar2 = TSA_RptTouchYCenter(param_1);
      if (bVar2 <= (byte)_refptr_g_tsaPrmtFlash[2].field_0x60c) {
        *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) =
             *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) | 0x80;
      }
    }
  }
  if (_refptr_g_tsaPrmtFlash[2].field_0x6c2 == '\0') {
    if (((0x14 < *(ushort *)
                  ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x188)
         ) && (4 < *(byte *)((longlong)(int)(uint)param_1 * 0x168 +
                             *(longlong *)_refptr_g_touchesPtr + 0x1ad))) &&
       (9 < *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x1a2))) {
      bVar2 = TSA_RptTouchYCenter(param_1);
      if (bVar2 <= (byte)_refptr_g_tsaPrmtFlash[2].field_0x60c) {
        *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0)
             = *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x1f0) | 0x4000000;
        *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) =
             *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) | 0x80;
      }
    }
  }
  else if (((0x14 < *(ushort *)
                     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x188)) &&
           (4 < *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x1ad))) &&
          (9 < *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x1a2))) {
    bVar2 = TSA_RptTouchYCenter(param_1);
    if (bVar2 <= (byte)_refptr_g_tsaPrmtFlash[2].field_0x60c) {
      bVar2 = TSA_RptTouchXCenter(param_1);
      if (1 < bVar2) {
        bVar2 = TSA_RptTouchXCenter(param_1);
        if ((int)(uint)bVar2 <= (int)(_refptr_g_tsaPrmtFlash->bCols - 3)) {
          *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0
                   ) = *(uint *)((longlong)(int)(uint)param_1 * 0x168 +
                                 *(longlong *)_refptr_g_touchesPtr + 0x1f0) | 0x4000000;
          *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) =
               *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) | 0x80;
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: AntiTouch_GetAxisMajorThold @ 6bad697c
// ============================================================================

undefined1 AntiTouch_GetAxisMajorThold(byte param_1)

{
  char cVar1;
  undefined1 local_9;
  
  if (((*(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x196)
        < 9) && (*(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x197) < 9)) &&
     (*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1a2) <
      0xf)) {
    local_9 = 0x1e;
  }
  else if (*(ushort *)
            ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x156) < 5)
  {
    local_9 = 0x1e;
  }
  else {
    local_9 = 0x19;
  }
  cVar1 = AntiTouch_IsTouchCloseToUdfpArea(param_1);
  if (cVar1 != '\0') {
    local_9 = 0x28;
  }
  return local_9;
}


// ============================================================================
// Function: AntiTouch_GetEllipseRateThold @ 6bad6a5a
// ============================================================================

undefined2 AntiTouch_GetEllipseRateThold(byte param_1)

{
  byte bVar1;
  byte bVar2;
  
  if ((8 < *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x196)) ||
     (8 < *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x197
                   ))) {
    bVar1 = TSA_RptTouchYCenter(param_1);
    bVar2 = TSA_GetYLength();
    if (bVar1 <= bVar2 >> 1) {
      return 10;
    }
  }
  return 0xf;
}


// ============================================================================
// Function: AntiTouch_IsToProtectProximity @ 6bad66e6
// ============================================================================

bool AntiTouch_IsToProtectProximity(byte param_1)

{
  uint uVar1;
  char cVar2;
  byte bVar3;
  bool local_9;
  
  uVar1 = *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c
                   );
  cVar2 = AntiTouch_IsGrip(param_1);
  local_9 = cVar2 == '\0' && 1 < uVar1;
  bVar3 = TSA_RptTouchYCenter(param_1);
  if (bVar3 <= (byte)_refptr_g_tsaPrmtFlash[2].field_0x60c) {
    if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
      if (0x10 < *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x197)) {
        local_9 = false;
      }
    }
    else if (0x10 < *(byte *)((longlong)(int)(uint)param_1 * 0x168 +
                              *(longlong *)_refptr_g_touchesPtr + 0x196)) {
      local_9 = false;
    }
  }
  return local_9;
}


// ============================================================================
// Function: AntiTouch_IsTouchCloseToUdfpArea @ 6bad689b
// ============================================================================

undefined8 AntiTouch_IsTouchCloseToUdfpArea(byte param_1)

{
  int iVar1;
  
  if ((((((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x800000) != 0) &&
       (iVar1 = UDFPEvent_IsEnabled(), iVar1 != 0)) &&
      (_refptr_g_tsaPrmtFlash[2].field_0x6c1 != '\0')) &&
     ((*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1a2)
       < 0xf && (iVar1 = UDFPEvent_IsCloseFingerArea
                                   (*(undefined2 *)
                                     ((longlong)(int)(uint)param_1 * 0x168 +
                                      *(longlong *)_refptr_g_touchesPtr + 0x170),
                                    *(undefined2 *)
                                     ((longlong)(int)(uint)param_1 * 0x168 +
                                      *(longlong *)_refptr_g_touchesPtr + 0x172)), iVar1 != 0)))) {
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: AntiTouch_GetMaxTouchPeakIdx @ 6bad6633
// ============================================================================

undefined1 AntiTouch_GetMaxTouchPeakIdx(void)

{
                    /* 0x96633  335  AntiTouch_GetMaxTouchPeakIdx */
  return g_antiTouchMaxTouchPkIdx;
}


// ============================================================================
// Function: AntiTouch_IsRptDebounce @ 6bad7b8f
// ============================================================================

undefined8 AntiTouch_IsRptDebounce(void)

{
  char cVar1;
  
                    /* 0x97b8f  337  AntiTouch_IsRptDebounce */
  cVar1 = TSAStatic_IsAFTModeEnabled();
  if ((cVar1 != '\0') && (cVar1 = BLSM_IsReset(), cVar1 == '\0')) {
    return 1;
  }
  return 0;
}


