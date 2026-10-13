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
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x58]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x60];
  ushort uVar1;
  ushort uVar2;
  char cVar3;
  int iVar4;
  int iVar5;
  
  (*(uint *)(_stk + 0x28)) = (uint)*(ushort *)
                    ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x166);
  uVar1 = *(ushort *)
           ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x168);
  (*(uint *)(_stk + 0x2c)) = (uint)uVar1;
  (*(uint *)(_stk + 0x30)) = (uint)*(ushort *)
                    ((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_2 * 0x478 + 0x16c);
  uVar2 = *(ushort *)
           ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x16e);
  (*(uint *)(_stk + 0x34)) = (uint)uVar2;
  if (*(short *)&_refptr_g_tsaPrmtFlash->field_0x5a6 == 0) {
    (*(int *)(_stk + 0x3c)) = 2;
  }
  else {
    (*(int *)(_stk + 0x3c)) = (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x20) /
                (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a6 + 1;
  }
  if (*(char *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5ad)
      == '\0') {
    (*(uint *)(_stk + 0x38)) = (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x12);
    (*(ulonglong *)(_stk + 0x8)) = (ulonglong)CONCAT24(uVar1,(*(uint *)(_stk + 0x28)));
    (*(ulonglong *)(_stk + 0x10)) = (ulonglong)CONCAT24(uVar2,(*(uint *)(_stk + 0x30)));
    (*(undefined8 *)(_stk + 0x18)) = CONCAT44((*(int *)(_stk + 0x3c)),(*(uint *)(_stk + 0x38)));
    JITFilter(&(*(ulonglong *)(_stk + 0x8)),((undefined2 *)(_stk + 0x50)),((undefined2 *)(_stk + 0x4c)));
  }
  else {
    (*(uint *)(_stk + 0x38)) = *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x27c);
    (*(ulonglong *)(_stk + 0x8)) = (ulonglong)CONCAT24(uVar1,(*(uint *)(_stk + 0x28)));
    (*(ulonglong *)(_stk + 0x10)) = (ulonglong)CONCAT24(uVar2,(*(uint *)(_stk + 0x30)));
    (*(undefined8 *)(_stk + 0x18)) = CONCAT44((*(int *)(_stk + 0x3c)),(*(uint *)(_stk + 0x38)));
    JITFilter(&(*(ulonglong *)(_stk + 0x8)),((undefined2 *)(_stk + 0x50)),((undefined2 *)(_stk + 0x4c)));
  }
  cVar3 = Filter_IsTouchLiftOffJitterSuppressEnable(param_1);
  if (cVar3 != '\0') {
    (*(uint *)(_stk + 0x38)) = (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x12);
    (*(ulonglong *)(_stk + 0x8)) = CONCAT44((*(uint *)(_stk + 0x2c)),(*(uint *)(_stk + 0x28)));
    (*(ulonglong *)(_stk + 0x10)) = CONCAT44((*(uint *)(_stk + 0x34)),(*(uint *)(_stk + 0x30)));
    (*(undefined8 *)(_stk + 0x18)) = CONCAT44((*(int *)(_stk + 0x3c)),(*(uint *)(_stk + 0x38)));
    JITFilter(&(*(ulonglong *)(_stk + 0x8)),((undefined2 *)(_stk + 0x50)),((undefined2 *)(_stk + 0x4c)));
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
         ((undefined2 *)(_stk + 0x50))[0];
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
         ((undefined2 *)(_stk + 0x4c))[0];
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
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x98]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0xa0];
  PrevTouchArray *pPVar1;
  char cVar2;
  undefined1 uVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  
                    /* 0x1d178  600  Filter_IsCurveReturn */
  cVar2 = Filter_IsNeedCurve(param_1);
  if (((cVar2 != '\0') &&
      ((*(byte *)(_stk + 0x7a)) = *(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                           (longlong)(int)(uint)param_2 * 0x478 + 0x288), (*(byte *)(_stk + 0x7a)) != 0)) &&
     (*(char *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x279) ==
      '\x01')) {
    (*(uint *)(_stk + 0x84)) = 0;
    (*(short *)(_stk + 0x82)) = -1;
    (*(short *)(_stk + 0x80)) = -1;
    (*(uint *)(_stk + 0x7c)) = 0;
    (*(byte *)(_stk + 0x7b)) = (*(byte *)(_stk + 0x7a));
    do {
      (*(byte *)(_stk + 0x7b)) = (*(byte *)(_stk + 0x7b)) - 1;
      if ((*(byte *)(_stk + 0x7b)) == 0) break;
      PrevTouch_HistoryBufferGet(param_2,(*(byte *)(_stk + 0x7b)),&(*(short *)(_stk + 0x18)));
      PrevTouch_HistoryBufferGet(param_2,(*(byte *)(_stk + 0x7b)) - 1,&(*(short *)(_stk + 0x8)));
      (*(short *)(_stk + 0x78)) = (*(short *)(_stk + 0x18)) - (*(short *)(_stk + 0x8));
      (*(short *)(_stk + 0x76)) = (*(short *)(_stk + 0x1a)) - (*(short *)(_stk + 0xa));
      (*(uint *)(_stk + 0x7c)) = (*(uint *)(_stk + 0x7c)) + ((int)(*(undefined8 *)(_stk + 0x20)) - (int)(*(undefined8 *)(_stk + 0x10)));
      iVar4 = Misc_SqrtUint32((int)(*(short *)(_stk + 0x76)) * (int)(*(short *)(_stk + 0x76)) + (int)(*(short *)(_stk + 0x78)) * (int)(*(short *)(_stk + 0x78)));
      (*(uint *)(_stk + 0x84)) = (*(uint *)(_stk + 0x84)) + iVar4;
      if (((*(short *)(_stk + 0x82)) < 0) && (0x98 < (*(uint *)(_stk + 0x84)))) {
        (*(short *)(_stk + 0x82)) = (*(byte *)(_stk + 0x7b)) - 1;
      }
      if (((*(short *)(_stk + 0x80)) < 0) && (7 < (*(uint *)(_stk + 0x7c)))) {
        (*(short *)(_stk + 0x80)) = (*(byte *)(_stk + 0x7b)) - 1;
      }
    } while ((*(uint *)(_stk + 0x7c)) < 0x50);
    if (((((0x4f < (*(uint *)(_stk + 0x7c))) && (0x80 < (*(uint *)(_stk + 0x84)))) && (-1 < (*(short *)(_stk + 0x82)))) &&
        ((-1 < (*(short *)(_stk + 0x80)) && ((*(short *)(_stk + 0x82)) < (*(short *)(_stk + 0x80)))))) && (-1 < (*(short *)(_stk + 0x82)))) {
      PrevTouch_HistoryBufferGet(param_2,(char)(*(short *)(_stk + 0x82)),&(*(short *)(_stk + 0x48)));
      PrevTouch_HistoryBufferGet(param_2,(*(byte *)(_stk + 0x7a)) - 1,&(*(short *)(_stk + 0x38)));
      (*(short *)(_stk + 0x74)) = (*(short *)(_stk + 0x38)) - (*(short *)(_stk + 0x48));
      (*(short *)(_stk + 0x72)) = (*(short *)(_stk + 0x3a)) - (*(short *)(_stk + 0x4a));
      (*(uint *)(_stk + 0x6c)) = Misc_SqrtUint32((int)(*(short *)(_stk + 0x72)) * (int)(*(short *)(_stk + 0x72)) + (int)(*(short *)(_stk + 0x74)) * (int)(*(short *)(_stk + 0x74)));
      PrevTouch_HistoryBufferGet(param_2,(char)(*(short *)(_stk + 0x80)),&(*(short *)(_stk + 0x28)));
      (*(short *)(_stk + 0x6a)) = (*(short *)(_stk + 0x28)) - (*(short *)(_stk + 0x48));
      (*(short *)(_stk + 0x68)) = (*(short *)(_stk + 0x2a)) - (*(short *)(_stk + 0x4a));
      (*(uint *)(_stk + 0x64)) = Misc_SqrtUint32((int)(*(short *)(_stk + 0x68)) * (int)(*(short *)(_stk + 0x68)) + (int)(*(short *)(_stk + 0x6a)) * (int)(*(short *)(_stk + 0x6a)));
      if ((*(uint *)(_stk + 0x6c)) < (*(uint *)(_stk + 0x64))) {
        *(undefined1 *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x280) = 1;
        uVar5 = (int)(*(short *)(_stk + 0x74)) >> 0x1f;
        uVar6 = (int)(*(short *)(_stk + 0x72)) >> 0x1f;
        if (((int)((((int)(*(short *)(_stk + 0x72)) ^ uVar6) - uVar6) * 5) < (int)(((int)(*(short *)(_stk + 0x74)) ^ uVar5) - uVar5))
           && (uVar5 = (int)(*(short *)(_stk + 0x6a)) >> 0x1f, uVar6 = (int)(*(short *)(_stk + 0x68)) >> 0x1f,
              (int)((((int)(*(short *)(_stk + 0x68)) ^ uVar6) - uVar6) * 5) < (int)(((int)(*(short *)(_stk + 0x6a)) ^ uVar5) - uVar5))
           ) {
          *(undefined1 *)
           ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5ad) = 1;
          *(undefined1 *)
           ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5ac) = 1;
        }
        else {
          uVar5 = (int)(*(short *)(_stk + 0x72)) >> 0x1f;
          uVar6 = (int)(*(short *)(_stk + 0x74)) >> 0x1f;
          if (((int)((((int)(*(short *)(_stk + 0x74)) ^ uVar6) - uVar6) * 5) < (int)(((int)(*(short *)(_stk + 0x72)) ^ uVar5) - uVar5)
              ) && (uVar5 = (int)(*(short *)(_stk + 0x68)) >> 0x1f, uVar6 = (int)(*(short *)(_stk + 0x6a)) >> 0x1f,
                   (int)((((int)(*(short *)(_stk + 0x6a)) ^ uVar6) - uVar6) * 5) <
                   (int)(((int)(*(short *)(_stk + 0x68)) ^ uVar5) - uVar5))) {
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
          (*(undefined4 *)(_stk + 0x60)) = Filter_GetSpeed(param_2,(int)(short)((*(byte *)(_stk + 0x7a)) - 1),
                                     *(undefined1 *)
                                      ((longlong)*_refptr_g_prevTouchesPtr +
                                      (longlong)(int)(uint)param_2 * 0x478 + 0x5ac));
          pPVar1 = *_refptr_g_prevTouchesPtr;
          uVar3 = Filter_GetTurnNum((*(undefined4 *)(_stk + 0x60)));
          *(undefined1 *)((longlong)pPVar1 + (longlong)(int)(uint)param_2 * 0x478 + 0x5ae) = uVar3;
          pPVar1 = *_refptr_g_prevTouchesPtr;
          uVar3 = Filter_GetForwardNum((*(undefined4 *)(_stk + 0x60)));
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
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x48]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x50];
  uint uVar1;
  uint uVar2;
  
  if (*(char *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwAftModeFeatureMask + 3) == '\0') {
    if (*(byte *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5ad
                 ) <
        *(byte *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5ae
                 )) {
      *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x27c) =
           (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x12);
      (*(byte *)(_stk + 0x37)) = *(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                          (longlong)(int)(uint)param_2 * 0x478 + 0x288);
      if (1 < (*(byte *)(_stk + 0x37))) {
        PrevTouch_HistoryBufferGet(param_2,(*(byte *)(_stk + 0x37)) - 1,&(*(short *)(_stk + 0x18)));
        PrevTouch_HistoryBufferGet(param_2,(*(byte *)(_stk + 0x37)) - 2,&(*(short *)(_stk + 0x8)));
        *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_2 * 0x478 + 0x5b0) =
             (*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                        (longlong)(int)(uint)param_2 * 0x478 + 0x16c) + (*(short *)(_stk + 0x18))) - (*(short *)(_stk + 0x8));
        *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_2 * 0x478 + 0x5b2) =
             (*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                        (longlong)(int)(uint)param_2 * 0x478 + 0x16e) + (*(short *)(_stk + 0x1a))) - (*(short *)(_stk + 0xa));
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
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x48]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x50];
  uint uVar1;
  
  (*(short *)(_stk + 0x34)) = param_2 + -6;
  if ((*(short *)(_stk + 0x34)) < 0) {
    (*(int *)(_stk + 0x38)) = 0;
  }
  else {
    (*(int *)(_stk + 0x40)) = 0;
    (*(short *)(_stk + 0x36)) = param_2;
    for ((*(uint *)(_stk + 0x3c)) = (uint)(*(short *)(_stk + 0x34)); (int)(*(uint *)(_stk + 0x3c)) < (int)(*(short *)(_stk + 0x36)); (*(uint *)(_stk + 0x3c)) = (*(uint *)(_stk + 0x3c)) + 1) {
      PrevTouch_HistoryBufferGet(param_1,(*(uint *)(_stk + 0x3c)) & 0xff,&(*(ushort *)(_stk + 0x18)));
      PrevTouch_HistoryBufferGet(param_1,(*(uint *)(_stk + 0x3c)) + 1 & 0xff,&(*(ushort *)(_stk + 0x8)));
      if (param_3 == '\0') {
        uVar1 = (int)((uint)(*(ushort *)(_stk + 0xa)) - (uint)(*(ushort *)(_stk + 0x1a))) >> 0x1f;
        (*(int *)(_stk + 0x44)) = ((uint)(*(ushort *)(_stk + 0xa)) - (uint)(*(ushort *)(_stk + 0x1a)) ^ uVar1) - uVar1;
      }
      else {
        uVar1 = (int)((uint)(*(ushort *)(_stk + 0x8)) - (uint)(*(ushort *)(_stk + 0x18))) >> 0x1f;
        (*(int *)(_stk + 0x44)) = ((uint)(*(ushort *)(_stk + 0x8)) - (uint)(*(ushort *)(_stk + 0x18)) ^ uVar1) - uVar1;
      }
      (*(int *)(_stk + 0x40)) = (*(int *)(_stk + 0x40)) + (*(int *)(_stk + 0x44));
    }
    (*(int *)(_stk + 0x38)) = (*(int *)(_stk + 0x40)) / ((int)(*(short *)(_stk + 0x36)) - (int)(*(short *)(_stk + 0x34)));
    PrevTouch_HistoryBufferGet(param_1,(char)(*(short *)(_stk + 0x34)),&(*(ushort *)(_stk + 0x18)));
    PrevTouch_HistoryBufferGet(param_1,(char)(*(short *)(_stk + 0x36)),&(*(ushort *)(_stk + 0x8)));
    if (param_3 == '\0') {
      (*(int *)(_stk + 0x44)) = (uint)(*(ushort *)(_stk + 0xa)) - (uint)(*(ushort *)(_stk + 0x1a));
    }
    else {
      (*(int *)(_stk + 0x44)) = (uint)(*(ushort *)(_stk + 0x8)) - (uint)(*(ushort *)(_stk + 0x18));
    }
    if ((*(int *)(_stk + 0x44)) < 0) {
      (*(int *)(_stk + 0x38)) = -(*(int *)(_stk + 0x38));
    }
  }
  return (*(int *)(_stk + 0x38));
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


