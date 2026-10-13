#include "TSACoreInternal.h"

// ============================================================================
// Function: Filter_ProcessUnit @ 6ba5fd96
// ============================================================================

void Filter_ProcessUnit(byte param_1,undefined1 param_2)

{
  ushort uVar1;
  
                    /* 0x1fd96  602  Filter_ProcessUnit */
  uVar1 = TSAOut_Dim1PosInternal
                    (*(undefined2 *)
                      ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x162),*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a0);
  g_xPosPreFilter = (uint)uVar1;
  uVar1 = TSAOut_Dim2PosInternal
                    (*(undefined2 *)
                      ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x164),*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a0);
  g_yPosPreFilter = (uint)uVar1;
  if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) &
      8) == 0) {
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x166) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162);
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x168) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164);
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16c) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162);
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16e) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164);
  }
  else {
    Filter_ReportedProcessUnit(param_1,param_2);
  }
  *(undefined2 *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170)
       = *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16c);
  *(undefined2 *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172)
       = *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16e);
  return;
}


// ============================================================================
// Function: Filter_ReportedProcessUnit @ 6ba5fb41
// ============================================================================

void Filter_ReportedProcessUnit(byte param_1,byte param_2)

{
  if (((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x1f8
                 ) & 8) == 0) ||
     (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
       & 0x20000) != 0 &&
      ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x1f8
                 ) & 0x20000) == 0)))) {
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x27c) =
         (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x12);
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x166) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162);
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x168) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164);
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16c) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162);
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16e) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164);
    *(undefined1 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16a) = 0;
    *(undefined1 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16b) = 0;
  }
  else {
    Filter_ModeTouchReportedProcessUnit(param_1,param_2);
  }
  return;
}


// ============================================================================
// Function: Filter_ModeTouchReportedProcessUnit @ 6ba5f1c2
// ============================================================================

void Filter_ModeTouchReportedProcessUnit(byte param_1,byte param_2)

{
  char cVar1;
  
  if ((((*(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1dd)
         < 0x10) &&
       (*(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1de)
        < 0x14)) &&
      (*(short *)(*_refptr_g_peaksPtr +
                 (longlong)
                 (int)(uint)*(byte *)((longlong)(int)(uint)param_1 * 0x168 +
                                      *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x18) <
       *(short *)_refptr_g_tsaPrmtDynamic / 2)) &&
     (((*(int *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x224)
        == 1 && ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                           (longlong)(int)(uint)param_2 * 0x478 + 0x1f8) & 0x8000) == 0)) &&
      ((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x100) != 0 &&
       (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x1000) == 0)))))) {
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x166) =
         *(undefined2 *)
          ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x166);
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x168) =
         *(undefined2 *)
          ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x168);
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
         | 0x800;
    *(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4ee) =
         *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162
                   ) -
         *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x166
                   );
    *(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4f0) =
         *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164
                   ) -
         *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x168
                   );
  }
  else if ((((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x1f8) & 0x9000) == 0) &&
           ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x1f0) & 0x100000) != 0)) &&
          ((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x100) != 0 &&
           (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x1000) == 0)))) {
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x166) =
         *(undefined2 *)
          ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x166);
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x168) =
         *(undefined2 *)
          ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x168);
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
         | 0x800;
    *(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4ee) =
         *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162
                   ) -
         *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x166
                   );
    *(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4f0) =
         *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164
                   ) -
         *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x168
                   );
  }
  else if (((((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x224) & 1) == 0) ||
            ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x1f8) & 0x8000) != 0)) ||
           (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x100) == 0)) ||
          (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x1000) != 0)) {
    if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
        & 0x800) == 0) {
      cVar1 = Touch_NeedXYFilter(param_1,param_2);
      if (cVar1 == '\0') {
        *(undefined2 *)
         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x166) =
             *(undefined2 *)
              ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162);
        *(undefined2 *)
         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x168) =
             *(undefined2 *)
              ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164);
      }
      else {
        Filter_IIRPDims(param_1,param_2);
      }
    }
    else {
      cVar1 = Filter_ReportClickToFlickProcessUnit(param_1,param_2);
      if (cVar1 != '\0') {
        *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
             = *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x1f8) & 0xfffff7ff;
      }
    }
  }
  else {
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x166) =
         *(undefined2 *)
          ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x166);
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x168) =
         *(undefined2 *)
          ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x168);
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
         | 0x800;
    *(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4ee) =
         *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162
                   ) -
         *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x166
                   );
    *(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4f0) =
         *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164
                   ) -
         *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x168
                   );
  }
  Filter_JitFilterProcessUnit(param_1,param_2);
  return;
}


// ============================================================================
// Function: Filter_IIRPDims @ 6ba5dcd4
// ============================================================================

void Filter_IIRPDims(byte param_1,byte param_2)

{
  ushort uVar1;
  char cVar2;
  ushort uVar3;
  int iVar4;
  int iVar5;
  undefined1 local_9;
  
                    /* 0x1dcd4  599  Filter_IIRPDims */
  local_9 = Filter_GetCoefBasedOnDistance(param_1,param_2);
  if (*(char *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5ad)
      == '\0') {
    Filter_IsCurveReturn(param_1,param_2);
  }
  if (*(char *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5ad)
      != '\0') {
    Filter_IsPredictFinish(param_1,param_2);
  }
  if ((*(char *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x280)
       != '\0') ||
     (*(char *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5ad)
      != '\0')) {
    cVar2 = Filter_IsHandLift(param_1);
    if (cVar2 != '\0') {
      cVar2 = TSA_IsGameScenario();
      if (cVar2 == '\0') {
        Filter_IsHandLiftFinish(param_1,param_2);
        local_9 = 1;
        *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x27c)
             = (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x12);
      }
    }
  }
  cVar2 = Filter_IsTouchLiftOff(param_1,param_2);
  if (cVar2 != '\0') {
    local_9 = 1;
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x27c) =
         (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x12);
  }
  iVar4 = (uint)*(ushort *)
                 ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162)
          - (uint)*(ushort *)
                   ((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_2 * 0x478 + 0x162);
  uVar3 = (ushort)(iVar4 >> 0x1f);
  iVar5 = (uint)*(ushort *)
                 ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164)
          - (uint)*(ushort *)
                   ((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_2 * 0x478 + 0x164);
  uVar1 = (ushort)(iVar5 >> 0x1f);
  if ((((uint)*(ushort *)
               ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162) ==
        *(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x20) - 1) ||
      (*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162)
       == 0)) && (0x31 < (ushort)(((ushort)iVar4 ^ uVar3) - uVar3))) {
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x166) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162);
  }
  else {
    Filter_ProcessPDim1(param_1,param_2,local_9);
  }
  if ((((uint)*(ushort *)
               ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164) ==
        *(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x22) - 1) ||
      (*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164)
       == 0)) && (0x31 < (ushort)(((ushort)iVar5 ^ uVar1) - uVar1))) {
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x168) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164);
  }
  else {
    Filter_ProcessPDim2(param_1,param_2,local_9);
  }
  uVar3 = TSAOut_Dim1PosInternal
                    (*(undefined2 *)
                      ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x166),*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a0);
  g_xPosAftIirFilter = (uint)uVar3;
  uVar3 = TSAOut_Dim2PosInternal
                    (*(undefined2 *)
                      ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x168),*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a0);
  g_yPosAftIirFilter = (uint)uVar3;
  return;
}


// ============================================================================
// Function: Filter_ReportClickToFlickProcessUnit @ 6ba5e6c3
// ============================================================================

undefined8 Filter_ReportClickToFlickProcessUnit(byte param_1,byte param_2)

{
  undefined2 uVar1;
  undefined2 uVar2;
  char cVar3;
  char cVar4;
  undefined8 uVar5;
  
  uVar1 = *(undefined2 *)
           ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162);
  uVar2 = *(undefined2 *)
           ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164);
  if ((int)(uint)*(ushort *)
                  ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162)
      < (int)*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_2 * 0x478 + 0x4ee)) {
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162) = 0;
  }
  else {
    *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162) =
         *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162
                   ) -
         *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_2 * 0x478 + 0x4ee);
  }
  if ((int)(uint)*(ushort *)
                  ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164)
      < (int)*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_2 * 0x478 + 0x4f0)) {
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164) = 0;
  }
  else {
    *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164) =
         *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164
                   ) -
         *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_2 * 0x478 + 0x4f0);
  }
  Filter_IIRPDims(param_1,param_2);
  *(undefined2 *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162)
       = uVar1;
  *(undefined2 *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164)
       = uVar2;
  cVar3 = Filter_ReportClickToFlickDim1OffsetCheck(param_1,param_2);
  cVar4 = Filter_ReportClickToFlickDim2OffsetCheck(param_1,param_2);
  if ((cVar3 == '\0') || (cVar4 == '\0')) {
    uVar5 = 0;
  }
  else {
    uVar5 = 1;
  }
  return uVar5;
}


// ============================================================================
// Function: Filter_JitFilterProcessUnit @ 6ba5e994
// ============================================================================

void Filter_JitFilterProcessUnit(byte param_1,byte param_2)

{
  ushort uVar1;
  ushort uVar2;
  char cVar3;
  int iVar4;
  int iVar5;
  ulonglong local_58;
  ulonglong local_50;
  undefined8 local_48;
  uint local_38;
  uint uStack_34;
  uint local_30;
  uint uStack_2c;
  uint local_28;
  int iStack_24;
  undefined2 local_14 [2];
  undefined2 local_10 [4];
  
  local_38 = (uint)*(ushort *)
                    ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x166);
  uVar1 = *(ushort *)
           ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x168);
  uStack_34 = (uint)uVar1;
  local_30 = (uint)*(ushort *)
                    ((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_2 * 0x478 + 0x16c);
  uVar2 = *(ushort *)
           ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x16e);
  uStack_2c = (uint)uVar2;
  if (*(short *)&_refptr_g_tsaPrmtFlash->field_0x5a6 == 0) {
    iStack_24 = 2;
  }
  else {
    iStack_24 = (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x20) /
                (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a6 + 1;
  }
  if (*(char *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5ad)
      == '\0') {
    local_28 = (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x12);
    local_58 = (ulonglong)CONCAT24(uVar1,local_38);
    local_50 = (ulonglong)CONCAT24(uVar2,local_30);
    local_48 = CONCAT44(iStack_24,local_28);
    JITFilter(&local_58,local_10,local_14);
  }
  else {
    local_28 = *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x27c);
    local_58 = (ulonglong)CONCAT24(uVar1,local_38);
    local_50 = (ulonglong)CONCAT24(uVar2,local_30);
    local_48 = CONCAT44(iStack_24,local_28);
    JITFilter(&local_58,local_10,local_14);
  }
  cVar3 = Filter_IsTouchLiftOffJitterSuppressEnable(param_1);
  if (cVar3 != '\0') {
    local_28 = (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x12);
    local_58 = CONCAT44(uStack_34,local_38);
    local_50 = CONCAT44(uStack_2c,local_30);
    local_48 = CONCAT44(iStack_24,local_28);
    JITFilter(&local_58,local_10,local_14);
  }
  if (((uint)*(ushort *)
              ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x166) ==
       *(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x20) - 1) ||
     (*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x166)
      == 0)) {
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16c) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x166);
  }
  else {
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16c) =
         local_10[0];
    cVar3 = TSA_IsGameScenario();
    if (cVar3 != '\0') {
      if (*(ushort *)
           ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x16c) <
          *(ushort *)
           ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16c)) {
        *(short *)(*(longlong *)_refptr_g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x16c)
             = *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x16c) + -1;
      }
      if (*(ushort *)
           ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16c) <
          *(ushort *)
           ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x16c)) {
        *(short *)(*(longlong *)_refptr_g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x16c)
             = *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x16c) + 1;
      }
    }
  }
  if (((uint)*(ushort *)
              ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x168) ==
       *(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x22) - 1) ||
     (*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x168)
      == 0)) {
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16e) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x168);
  }
  else {
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16e) =
         local_14[0];
    cVar3 = TSA_IsGameScenario();
    if (cVar3 != '\0') {
      if (*(ushort *)
           ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x16e) <
          *(ushort *)
           ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16e)) {
        *(short *)(*(longlong *)_refptr_g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x16e)
             = *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x16e) + -1;
      }
      if (*(ushort *)
           ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16e) <
          *(ushort *)
           ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x16e)) {
        *(short *)(*(longlong *)_refptr_g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x16e)
             = *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x16e) + 1;
      }
    }
  }
  if ((*(char *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwAftModeFeatureMask + 3) == '\0') &&
     ((int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                          (longlong)(int)(uint)param_2 * 0x478 + 0x5af) <
      (int)(*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)param_2 * 0x478 + 0x5ad) - 1))) {
    iVar4 = (int)((*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                            (longlong)(int)(uint)param_2 * 0x478 + 0x5ad) - 1) * 0x100) /
            (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                (longlong)(int)(uint)param_2 * 0x478 + 0x5ae);
    iVar5 = (0x100 - iVar4) *
            (int)*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                           (longlong)(int)(uint)param_2 * 0x478 + 0x5b0) +
            (uint)*(ushort *)
                   ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16c
                   ) * iVar4;
    if (iVar5 < 0) {
      iVar5 = iVar5 + 0xff;
    }
    *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16c) =
         (short)((uint)iVar5 >> 8);
    iVar5 = (0x100 - iVar4) *
            (int)*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                           (longlong)(int)(uint)param_2 * 0x478 + 0x5b2) +
            (uint)*(ushort *)
                   ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16e
                   ) * iVar4;
    if (iVar5 < 0) {
      iVar5 = iVar5 + 0xff;
    }
    *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16e) =
         (short)((uint)iVar5 >> 8);
  }
  return;
}


// ============================================================================
// Function: Filter_IsTouchLiftOff @ 6ba5c363
// ============================================================================

undefined8 Filter_IsTouchLiftOff(byte param_1,byte param_2)

{
  undefined8 uVar1;
  ushort local_a;
  
  local_a = 0;
  *(undefined1 *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x282)
       = 0;
  if (*(byte *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x288)
      < 0x1e) {
    uVar1 = 0;
  }
  else if (*(char *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwAftModeFeatureMask + 2) == '\0') {
    uVar1 = 0;
  }
  else {
    if (*(ushort *)
         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x15c) <
        *(ushort *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x15c)) {
      local_a = (short)((ulonglong)
                        ((uint)*(ushort *)
                                ((longlong)*_refptr_g_prevTouchesPtr +
                                (longlong)(int)(uint)param_2 * 0x478 + 0x15c) * 100) /
                       (ulonglong)
                       (longlong)
                       (int)(uint)*(ushort *)
                                   ((longlong)(int)(uint)param_1 * 0x168 +
                                    *(longlong *)_refptr_g_touchesPtr + 0x15c)) - 100;
      g_zValPrevToCurRatio = (uint)local_a;
    }
    *(undefined1 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x283) =
         *(undefined1 *)
          ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x283);
    if (*(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x283)
        < 0xb) {
      if (((local_a < 0x1f) ||
          (*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x158) < 0x12d)) ||
         ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1f8) & 0x8000) == 0)) {
        uVar1 = 0;
      }
      else {
        *(undefined1 *)
         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x282) = 1;
        *(char *)(*(longlong *)_refptr_g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x283)
             = *(char *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x283) + '\x01';
        uVar1 = 1;
      }
    }
    else {
      uVar1 = 0;
    }
  }
  return uVar1;
}


// ============================================================================
// Function: Filter_IsHandLift @ 6ba5c644
// ============================================================================

bool Filter_IsHandLift(byte param_1)

{
                    /* 0x1c644  601  Filter_IsHandLift */
  return *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1dc)
         < 0x1c;
}


// ============================================================================
// Function: Filter_GetCoefBasedOnDistance @ 6ba5c99c
// ============================================================================

undefined1 Filter_GetCoefBasedOnDistance(undefined1 param_1,byte param_2)

{
  char cVar1;
  undefined1 local_9;
  
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x1000) == 0) {
    cVar1 = Filter_NeedHeavyCoef(param_1);
    if (cVar1 == '\0') {
      *(undefined2 *)(_refptr_g_tsaPrmtRam + 0x16) =
           *(undefined2 *)&_refptr_g_tsaPrmtFlash[1].field_0x4e;
      *(short *)(_refptr_g_tsaPrmtRam + 0x18) = (short)_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase1
      ;
      _refptr_g_tsaPrmtRam[0x1a] =
           *(undefined1 *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase1 + 2);
      _refptr_g_tsaPrmtRam[0x1b] =
           *(undefined1 *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase1 + 3);
      *(undefined2 *)(_refptr_g_tsaPrmtRamConst + 2) =
           *(undefined2 *)(_refptr_g_tsaPrmtFlashConst + 0xc);
    }
    else {
      *(short *)(_refptr_g_tsaPrmtRam + 0x16) =
           (short)_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase1_or;
      *(undefined2 *)(_refptr_g_tsaPrmtRam + 0x18) =
           *(undefined2 *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase1_or + 2);
      _refptr_g_tsaPrmtRam[0x1a] = (char)_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase2;
      _refptr_g_tsaPrmtRam[0x1b] =
           *(undefined1 *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase2 + 1);
      *(undefined2 *)(_refptr_g_tsaPrmtRamConst + 2) =
           *(undefined2 *)(_refptr_g_tsaPrmtFlashConst + 0xe);
    }
  }
  else {
    *(undefined2 *)(_refptr_g_tsaPrmtRam + 0x16) =
         *(undefined2 *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase2 + 2);
    *(short *)(_refptr_g_tsaPrmtRam + 0x18) =
         (short)_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase2_or;
    _refptr_g_tsaPrmtRam[0x1a] =
         *(undefined1 *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase2_or + 2);
    _refptr_g_tsaPrmtRam[0x1b] =
         *(undefined1 *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase2_or + 3);
    *(undefined2 *)(_refptr_g_tsaPrmtRamConst + 2) =
         *(undefined2 *)(_refptr_g_tsaPrmtFlashConst + 0x10);
  }
  local_9 = Filter_GetAdaptiveCoef(param_1,param_2);
  if ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x1f8)
      & 0x40) != 0) {
    if (*(char *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4f3
                 ) == '\0') {
      *(undefined1 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4f3) = 2;
    }
    else if (*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_2 * 0x478 + 0x4f3) < 0x20) {
      *(char *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4f3)
           = *(char *)((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_2 * 0x478 + 0x4f3) * '\x02';
    }
    local_9 = *(undefined1 *)
               ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4f3);
  }
  return local_9;
}


// ============================================================================
// Function: Filter_ProcessPDim1 @ 6ba5cc76
// ============================================================================

void Filter_ProcessPDim1(byte param_1,byte param_2,undefined1 param_3)

{
  int iVar1;
  int iVar2;
  
  iVar1 = IIR_Process((ulonglong)
                      *(ushort *)
                       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x162) << 8,
                      (uint)*(ushort *)
                             ((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)param_2 * 0x478 + 0x166) * 0x100 +
                      (uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                     (longlong)(int)(uint)param_2 * 0x478 + 0x16a),param_3,5);
  iVar2 = iVar1;
  if (iVar1 < 0) {
    iVar2 = iVar1 + 0xff;
  }
  *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x166) =
       (short)((uint)iVar2 >> 8);
  *(char *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16a) =
       (char)iVar1;
  return;
}


// ============================================================================
// Function: Filter_ProcessPDim2 @ 6ba5cd9c
// ============================================================================

void Filter_ProcessPDim2(byte param_1,byte param_2,undefined1 param_3)

{
  int iVar1;
  int iVar2;
  
  iVar1 = IIR_Process((ulonglong)
                      *(ushort *)
                       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x164) << 8,
                      (uint)*(ushort *)
                             ((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)param_2 * 0x478 + 0x168) * 0x100 +
                      (uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                     (longlong)(int)(uint)param_2 * 0x478 + 0x16b),param_3,5);
  iVar2 = iVar1;
  if (iVar1 < 0) {
    iVar2 = iVar1 + 0xff;
  }
  *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x168) =
       (short)((uint)iVar2 >> 8);
  *(char *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16b) =
       (char)iVar1;
  return;
}


// ============================================================================
// Function: Filter_IsCurveReturn @ 6ba5d178
// ============================================================================

void Filter_IsCurveReturn(byte param_1,byte param_2)

{
  PrevTouchArray *pPVar1;
  char cVar2;
  undefined1 uVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  short local_98;
  short local_96;
  undefined8 local_90;
  short local_88;
  short local_86;
  undefined8 local_80;
  short local_78;
  short local_76;
  short local_68;
  short local_66;
  short local_58;
  short local_56;
  undefined4 local_40;
  uint local_3c;
  short local_38;
  short local_36;
  uint local_34;
  short local_2e;
  short local_2c;
  short local_2a;
  short local_28;
  byte local_26;
  byte local_25;
  uint local_24;
  short local_20;
  short local_1e;
  uint local_1c;
  
                    /* 0x1d178  600  Filter_IsCurveReturn */
  cVar2 = Filter_IsNeedCurve(param_1);
  if (((cVar2 != '\0') &&
      (local_26 = *(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                           (longlong)(int)(uint)param_2 * 0x478 + 0x288), local_26 != 0)) &&
     (*(char *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x279) ==
      '\x01')) {
    local_1c = 0;
    local_1e = -1;
    local_20 = -1;
    local_24 = 0;
    local_25 = local_26;
    do {
      local_25 = local_25 - 1;
      if (local_25 == 0) break;
      PrevTouch_HistoryBufferGet(param_2,local_25,&local_88);
      PrevTouch_HistoryBufferGet(param_2,local_25 - 1,&local_98);
      local_28 = local_88 - local_98;
      local_2a = local_86 - local_96;
      local_24 = local_24 + ((int)local_80 - (int)local_90);
      iVar4 = Misc_SqrtUint32((int)local_2a * (int)local_2a + (int)local_28 * (int)local_28);
      local_1c = local_1c + iVar4;
      if ((local_1e < 0) && (0x98 < local_1c)) {
        local_1e = local_25 - 1;
      }
      if ((local_20 < 0) && (7 < local_24)) {
        local_20 = local_25 - 1;
      }
    } while (local_24 < 0x50);
    if (((((0x4f < local_24) && (0x80 < local_1c)) && (-1 < local_1e)) &&
        ((-1 < local_20 && (local_1e < local_20)))) && (-1 < local_1e)) {
      PrevTouch_HistoryBufferGet(param_2,(char)local_1e,&local_58);
      PrevTouch_HistoryBufferGet(param_2,local_26 - 1,&local_68);
      local_2c = local_68 - local_58;
      local_2e = local_66 - local_56;
      local_34 = Misc_SqrtUint32((int)local_2e * (int)local_2e + (int)local_2c * (int)local_2c);
      PrevTouch_HistoryBufferGet(param_2,(char)local_20,&local_78);
      local_36 = local_78 - local_58;
      local_38 = local_76 - local_56;
      local_3c = Misc_SqrtUint32((int)local_38 * (int)local_38 + (int)local_36 * (int)local_36);
      if (local_34 < local_3c) {
        *(undefined1 *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x280) = 1;
        uVar5 = (int)local_2c >> 0x1f;
        uVar6 = (int)local_2e >> 0x1f;
        if (((int)((((int)local_2e ^ uVar6) - uVar6) * 5) < (int)(((int)local_2c ^ uVar5) - uVar5))
           && (uVar5 = (int)local_36 >> 0x1f, uVar6 = (int)local_38 >> 0x1f,
              (int)((((int)local_38 ^ uVar6) - uVar6) * 5) < (int)(((int)local_36 ^ uVar5) - uVar5))
           ) {
          *(undefined1 *)
           ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5ad) = 1;
          *(undefined1 *)
           ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5ac) = 1;
        }
        else {
          uVar5 = (int)local_2e >> 0x1f;
          uVar6 = (int)local_2c >> 0x1f;
          if (((int)((((int)local_2c ^ uVar6) - uVar6) * 5) < (int)(((int)local_2e ^ uVar5) - uVar5)
              ) && (uVar5 = (int)local_38 >> 0x1f, uVar6 = (int)local_36 >> 0x1f,
                   (int)((((int)local_36 ^ uVar6) - uVar6) * 5) <
                   (int)(((int)local_38 ^ uVar5) - uVar5))) {
            *(undefined1 *)
             ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5ad) =
                 1;
            *(undefined1 *)
             ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5ac) =
                 0;
          }
        }
        if ((*(char *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwAftModeFeatureMask + 3) == '\0') &&
           (*(char *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)param_2 * 0x478 + 0x5ad) != '\0')) {
          local_40 = Filter_GetSpeed(param_2,(int)(short)(local_26 - 1),
                                     *(undefined1 *)
                                      ((longlong)*_refptr_g_prevTouchesPtr +
                                      (longlong)(int)(uint)param_2 * 0x478 + 0x5ac));
          pPVar1 = *_refptr_g_prevTouchesPtr;
          uVar3 = Filter_GetTurnNum(local_40);
          *(undefined1 *)((longlong)pPVar1 + (longlong)(int)(uint)param_2 * 0x478 + 0x5ae) = uVar3;
          pPVar1 = *_refptr_g_prevTouchesPtr;
          uVar3 = Filter_GetForwardNum(local_40);
          *(undefined1 *)((longlong)pPVar1 + (longlong)(int)(uint)param_2 * 0x478 + 0x5af) = uVar3;
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: Filter_IsPredictFinish @ 6ba5d662
// ============================================================================

void Filter_IsPredictFinish(byte param_1,byte param_2)

{
  uint uVar1;
  uint uVar2;
  short local_48;
  short local_46;
  short local_38;
  short local_36;
  byte local_19;
  
  if (*(char *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwAftModeFeatureMask + 3) == '\0') {
    if (*(byte *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5ad
                 ) <
        *(byte *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5ae
                 )) {
      *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x27c) =
           (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x12);
      local_19 = *(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                          (longlong)(int)(uint)param_2 * 0x478 + 0x288);
      if (1 < local_19) {
        PrevTouch_HistoryBufferGet(param_2,local_19 - 1,&local_38);
        PrevTouch_HistoryBufferGet(param_2,local_19 - 2,&local_48);
        *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_2 * 0x478 + 0x5b0) =
             (*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                        (longlong)(int)(uint)param_2 * 0x478 + 0x16c) + local_38) - local_48;
        *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_2 * 0x478 + 0x5b2) =
             (*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                        (longlong)(int)(uint)param_2 * 0x478 + 0x16e) + local_36) - local_46;
      }
      *(char *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5ad)
           = *(char *)((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_2 * 0x478 + 0x5ad) + '\x01';
    }
    else {
      *(undefined1 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5ad) = 0;
      *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x27c) =
           (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x12);
      *(undefined2 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5b0) = 0;
      *(undefined2 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5b2) = 0;
    }
  }
  else if (*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_2 * 0x478 + 0x5ad) < 0xc) {
    if (*(byte *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5ad
                 ) < 3) {
      if (*(short *)&_refptr_g_tsaPrmtFlash->field_0x5a6 == 0) {
        uVar2 = 1;
      }
      else {
        uVar2 = (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x20) /
                (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a6;
      }
      if (*(short *)&_refptr_g_tsaPrmtFlash->field_0x5a6 == 0) {
        uVar1 = 1;
      }
      else {
        uVar1 = (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x20) /
                (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a6;
      }
      *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x27c) =
           uVar2 + uVar1 * *(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                    (longlong)(int)(uint)param_2 * 0x478 + 0x5ad) * -2 +
           (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x12);
    }
    else if (*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_2 * 0x478 + 0x5ad) < 6) {
      *(undefined4 *)
       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x27c) = 0;
    }
    else {
      if (*(short *)&_refptr_g_tsaPrmtFlash->field_0x5a6 == 0) {
        uVar2 = 1;
      }
      else {
        uVar2 = (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x20) /
                (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a6;
      }
      *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x27c) =
           (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x12) -
           uVar2 * (0xc - (uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                         (longlong)(int)(uint)param_2 * 0x478 + 0x5ad));
    }
    *(char *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5ad) =
         *(char *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_2 * 0x478 + 0x5ad) + '\x01';
  }
  else {
    *(undefined1 *)
     ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5ad) = 0;
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x27c) =
         (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x12);
  }
  return;
}


// ============================================================================
// Function: Filter_IsHandLiftFinish @ 6ba5dc30
// ============================================================================

void Filter_IsHandLiftFinish(byte param_1,byte param_2)

{
  if (*(byte *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x280)
      < 5) {
    *(char *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x280) =
         *(char *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_2 * 0x478 + 0x280) + '\x01';
  }
  else {
    *(undefined1 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x280) = 0;
  }
  return;
}


// ============================================================================
// Function: Filter_ReportClickToFlickDim1OffsetCheck @ 6ba5e149
// ============================================================================

undefined1 Filter_ReportClickToFlickDim1OffsetCheck(byte param_1,byte param_2)

{
  short sVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  undefined1 local_9;
  
  local_9 = 0;
  sVar1 = TSAOut_ExtToIntDist(_refptr_g_tsaPrmtDynamic[0x7e]);
  uVar5 = (uint)*(ushort *)
                 ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x166)
          - (uint)*(ushort *)
                   ((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_2 * 0x478 + 0x166);
  uVar2 = (int)uVar5 >> 0x1f;
  iVar3 = (uVar2 ^ uVar5) - uVar2;
  if (iVar3 < 0) {
    iVar3 = iVar3 + 3;
  }
  iVar4 = iVar3 >> 2;
  if ((int)sVar1 <= iVar3 >> 2) {
    iVar4 = (int)sVar1;
  }
  sVar1 = (short)iVar4;
  if (*(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4ee)
      < iVar4) {
    if (-iVar4 < (int)*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                                (longlong)(int)(uint)param_2 * 0x478 + 0x4ee)) {
      local_9 = 1;
    }
    else {
      if ((int)(uint)*(ushort *)
                      ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x166) < iVar4) {
        *(undefined2 *)
         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x166) = 0;
      }
      else {
        *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x166)
             = *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x166) - sVar1;
      }
      *(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4ee)
           = sVar1 + *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                               (longlong)(int)(uint)param_2 * 0x478 + 0x4ee);
    }
  }
  else {
    *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x166) =
         sVar1 + *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                           + 0x166);
    *(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4ee) =
         *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_2 * 0x478 + 0x4ee) - sVar1;
  }
  return local_9;
}


// ============================================================================
// Function: Filter_ReportClickToFlickDim2OffsetCheck @ 6ba5e409
// ============================================================================

undefined1 Filter_ReportClickToFlickDim2OffsetCheck(byte param_1,byte param_2)

{
  short sVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  undefined1 local_9;
  
  local_9 = 0;
  sVar1 = TSAOut_ExtToIntDist(_refptr_g_tsaPrmtDynamic[0x7e]);
  uVar5 = (uint)*(ushort *)
                 ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x168)
          - (uint)*(ushort *)
                   ((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_2 * 0x478 + 0x168);
  uVar2 = (int)uVar5 >> 0x1f;
  iVar3 = (uVar2 ^ uVar5) - uVar2;
  if (iVar3 < 0) {
    iVar3 = iVar3 + 3;
  }
  iVar4 = iVar3 >> 2;
  if ((int)sVar1 <= iVar3 >> 2) {
    iVar4 = (int)sVar1;
  }
  sVar1 = (short)iVar4;
  if (*(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4f0)
      < iVar4) {
    if (-iVar4 < (int)*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                                (longlong)(int)(uint)param_2 * 0x478 + 0x4f0)) {
      local_9 = 1;
    }
    else {
      if ((int)(uint)*(ushort *)
                      ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x168) < iVar4) {
        *(undefined2 *)
         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x168) = 0;
      }
      else {
        *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x168)
             = *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x168) - sVar1;
      }
      *(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4f0)
           = sVar1 + *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                               (longlong)(int)(uint)param_2 * 0x478 + 0x4f0);
    }
  }
  else {
    *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x168) =
         sVar1 + *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                           + 0x168);
    *(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4f0) =
         *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_2 * 0x478 + 0x4f0) - sVar1;
  }
  return local_9;
}


// ============================================================================
// Function: Filter_IsTouchLiftOffJitterSuppressEnable @ 6ba5c5fc
// ============================================================================

undefined1 Filter_IsTouchLiftOffJitterSuppressEnable(byte param_1)

{
  undefined1 uVar1;
  
  if (*(char *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwAftModeFeatureMask + 2) == '\0') {
    uVar1 = 0;
  }
  else {
    uVar1 = *(undefined1 *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x282);
  }
  return uVar1;
}


// ============================================================================
// Function: Filter_NeedHeavyCoef @ 6ba5c240
// ============================================================================

bool Filter_NeedHeavyCoef(byte param_1)

{
  bool bVar1;
  bool local_9;
  
  bVar1 = *(int *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1e0)
          == 3;
  if (bVar1) {
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
         | 0x10;
  }
  local_9 = *(int *)(*(longlong *)_refptr_g_touchesPtr + 0x20) != 0;
  if (local_9) {
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
         | 0x10;
  }
  local_9 = local_9 || bVar1;
  if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) &
      0x10) != 0) {
    local_9 = true;
  }
  return local_9;
}


// ============================================================================
// Function: Filter_GetAdaptiveCoef @ 6ba5c690
// ============================================================================

char Filter_GetAdaptiveCoef(byte param_1,byte param_2)

{
  char cVar1;
  ushort uVar2;
  ushort uVar3;
  undefined2 uVar4;
  int iVar5;
  byte local_9;
  
  iVar5 = (uint)*(ushort *)
                 ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162)
          - (uint)*(ushort *)
                   ((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_2 * 0x478 + 0x166);
  uVar2 = (ushort)(iVar5 >> 0x1f);
  uVar2 = ((ushort)iVar5 ^ uVar2) - uVar2;
  iVar5 = (uint)*(ushort *)
                 ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164)
          - (uint)*(ushort *)
                   ((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_2 * 0x478 + 0x168);
  uVar3 = (ushort)(iVar5 >> 0x1f);
  uVar3 = ((ushort)iVar5 ^ uVar3) - uVar3;
  uVar4 = Misc_SqrtUint32((uint)uVar3 * (uint)uVar3 + (uint)uVar2 * (uint)uVar2);
  local_9 = AIIR_GetCoef(uVar4);
  if (*(char *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x279)
      == '\x01') {
    *(undefined1 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x279) = 1;
    if (local_9 < 8) {
      local_9 = 8;
    }
  }
  else if (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x1f8) & 0x8000) != 0) && (cVar1 = Filter_IsHandLift(param_1), cVar1 == '\0'))
  {
    *(undefined1 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x279) = 1;
  }
  if (*(char *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4f4)
      == '\0') {
    *(byte *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4f4) =
         local_9;
  }
  if (*(byte *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4f4)
      + 1 < (uint)local_9) {
    local_9 = local_9 - 1;
    *(byte *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4f4) =
         local_9;
  }
  else if (local_9 + 1 <
           (uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                          (longlong)(int)(uint)param_2 * 0x478 + 0x4f4)) {
    local_9 = local_9 + 1;
    *(byte *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4f4) =
         local_9;
  }
  else {
    local_9 = *(char *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_2 * 0x478 + 0x4f4);
  }
  return local_9;
}


// ============================================================================
// Function: Filter_IsNeedCurve @ 6ba5cec2
// ============================================================================

undefined1 Filter_IsNeedCurve(byte param_1)

{
  ushort uVar1;
  ushort uVar2;
  undefined1 local_9;
  
  local_9 = 0;
  uVar1 = *(ushort *)
           ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162);
  uVar2 = *(ushort *)
           ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164);
  if ((((0x180 < uVar1) &&
       ((int)(uint)uVar1 < (int)(*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x20) - 0x180))) &&
      (0x180 < uVar2)) &&
     ((int)(uint)uVar2 < (int)(*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x22) - 0x180))) {
    local_9 = 1;
  }
  return local_9;
}


// ============================================================================
// Function: Filter_GetSpeed @ 6ba5cf7a
// ============================================================================

int Filter_GetSpeed(undefined1 param_1,short param_2,char param_3)

{
  uint uVar1;
  ushort local_48;
  ushort local_46;
  ushort local_38;
  ushort local_36;
  short local_1c;
  short local_1a;
  int local_18;
  uint local_14;
  int local_10;
  int local_c;
  
  local_1c = param_2 + -6;
  if (local_1c < 0) {
    local_18 = 0;
  }
  else {
    local_10 = 0;
    local_1a = param_2;
    for (local_14 = (uint)local_1c; (int)local_14 < (int)local_1a; local_14 = local_14 + 1) {
      PrevTouch_HistoryBufferGet(param_1,local_14 & 0xff,&local_38);
      PrevTouch_HistoryBufferGet(param_1,local_14 + 1 & 0xff,&local_48);
      if (param_3 == '\0') {
        uVar1 = (int)((uint)local_46 - (uint)local_36) >> 0x1f;
        local_c = ((uint)local_46 - (uint)local_36 ^ uVar1) - uVar1;
      }
      else {
        uVar1 = (int)((uint)local_48 - (uint)local_38) >> 0x1f;
        local_c = ((uint)local_48 - (uint)local_38 ^ uVar1) - uVar1;
      }
      local_10 = local_10 + local_c;
    }
    local_18 = local_10 / ((int)local_1a - (int)local_1c);
    PrevTouch_HistoryBufferGet(param_1,(char)local_1c,&local_38);
    PrevTouch_HistoryBufferGet(param_1,(char)local_1a,&local_48);
    if (param_3 == '\0') {
      local_c = (uint)local_46 - (uint)local_36;
    }
    else {
      local_c = (uint)local_48 - (uint)local_38;
    }
    if (local_c < 0) {
      local_18 = -local_18;
    }
  }
  return local_18;
}


// ============================================================================
// Function: Filter_GetTurnNum @ 6ba5d0dc
// ============================================================================

int Filter_GetTurnNum(uint param_1)

{
  int local_c;
  
  if (((int)param_1 < -4) || (4 < (int)param_1)) {
    local_c = (int)(0x294 / (longlong)
                            (int)(((int)param_1 >> 0x1f ^ param_1) - ((int)param_1 >> 0x1f)));
    if (0xc < local_c) {
      local_c = 0xc;
    }
  }
  else {
    local_c = 0;
  }
  return local_c;
}


// ============================================================================
// Function: Filter_GetForwardNum @ 6ba5d12a
// ============================================================================

int Filter_GetForwardNum(uint param_1)

{
  int local_c;
  
  if (((int)param_1 < -4) || (4 < (int)param_1)) {
    local_c = (int)(0x3c / (longlong)
                           (int)(((int)param_1 >> 0x1f ^ param_1) - ((int)param_1 >> 0x1f)));
    if (2 < local_c) {
      local_c = 2;
    }
  }
  else {
    local_c = 0;
  }
  return local_c;
}


