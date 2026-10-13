#include "TSACoreInternal.h"

// ============================================================================
// Function: TouchReport_Process @ 6ba7be73
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* Builds final touch report units, then restores previous coordinates for multi-step-back cases
   when all touches are backing out. */

void TouchReport_Process(void)

{
  undefined1 bPrevTouchIndex;
  char cReportCondition;
  byte bPrevTouchIndexForBackstep;
  byte bTouchIndex;
  
                    /* 0x3be73  2501  TouchReport_Process */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&_rdata,"TouchReport_Process");
  }
  *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 5) = 0;
  *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0xb) = 0;
  *(undefined4 *)_refptr_g_zValPrevToCurRatio = 0;
  for (bTouchIndex = 0; bTouchIndex < **(byte **)_refptr_g_touchesPtr; bTouchIndex = bTouchIndex + 1
      ) {
    bPrevTouchIndex = PrevTouch_GetIdxBasedOnCurIdx(bTouchIndex);
    TouchReport_ProcessUnit(bTouchIndex,bPrevTouchIndex);
  }
  cReportCondition = TouchReport_IsMultStepBackEnable();
  if ((cReportCondition != '\0') &&
     (cReportCondition = TouchReport_IsAllTouchBack(), cReportCondition != '\0')) {
    for (bTouchIndex = 0; bTouchIndex < **(byte **)_refptr_g_touchesPtr;
        bTouchIndex = bTouchIndex + 1) {
      bPrevTouchIndexForBackstep = PrevTouch_GetIdxBasedOnCurIdx(bTouchIndex);
      if (bPrevTouchIndexForBackstep < 0x14) {
        *(undefined2 *)
         ((longlong)(int)(uint)bTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170) =
             *(undefined2 *)
              ((longlong)*_refptr_g_prevTouchesPtr +
              (longlong)(int)(uint)bPrevTouchIndexForBackstep * 0x478 + 0x594);
        *(undefined2 *)
         ((longlong)(int)(uint)bTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172) =
             *(undefined2 *)
              ((longlong)*_refptr_g_prevTouchesPtr +
              (longlong)(int)(uint)bPrevTouchIndexForBackstep * 0x478 + 0x596);
      }
    }
  }
  return;
}


// ============================================================================
// Function: TouchReport_IsMultStepBackEnable @ 6ba7a3b0
// ============================================================================

bool TouchReport_IsMultStepBackEnable(void)

{
  return (*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6cc & 2) != 0;
}


// ============================================================================
// Function: TouchReport_IsAllTouchBack @ 6ba7a3d6
// ============================================================================

undefined1 TouchReport_IsAllTouchBack(void)

{
  byte bVar1;
  undefined1 local_a;
  byte local_9;
  
  local_a = 0;
  local_9 = 0;
  do {
    if (**(byte **)_refptr_g_touchesPtr <= local_9) {
      return local_a;
    }
    bVar1 = PrevTouch_GetIdxBasedOnCurIdx(local_9);
    if ((bVar1 < 0x14) &&
       ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x1f8)
        & 0x20000000) != 0)) {
      if (*(byte *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x58e
                   ) < 7) {
        return 0;
      }
      local_a = 1;
    }
    local_9 = local_9 + 1;
  } while( true );
}


// ============================================================================
// Function: TouchReport_ProcessUnit @ 6ba7b86e
// ============================================================================

void TouchReport_ProcessUnit(byte param_1,byte param_2)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0xe]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x10];
  char cVar1;
  
  (*(undefined2 *)(_stk + 0x4)) = 0;
  (*(undefined2 *)(_stk + 0x2)) = 0;
  (*(char *)(_stk + 0x7)) = TouchReport_ToBeReported(param_1,param_2);
  (*(char *)(_stk + 0x6)) = TouchReport_IsHistoryBufferApplied(param_1,param_2,&(*(undefined2 *)(_stk + 0x4)),&(*(undefined2 *)(_stk + 0x2)));
  if (((*(char *)(_stk + 0x7)) == '\0') && ((*(char *)(_stk + 0x6)) == '\0')) {
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
         & 0xfffffff7;
  }
  else {
    *(char *)(*(longlong *)_refptr_g_touchesPtr + 5) =
         *(char *)(*(longlong *)_refptr_g_touchesPtr + 5) + '\x01';
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
         | 8;
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
         | 0x100000;
  }
  if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1ec) &
      0x40) != 0) {
    *(char *)(*(longlong *)_refptr_g_touchesPtr + 0xb) =
         *(char *)(*(longlong *)_refptr_g_touchesPtr + 0xb) + '\x01';
  }
  Filter_ProcessUnit(param_1,param_2);
  cVar1 = TouchReport_IsMultStepBackEnable();
  if (cVar1 != '\0') {
    cVar1 = TouchReport_IsAnyTouchCancelByPosture();
    if (cVar1 != '\0') {
      *(undefined2 *)
       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170) =
           *(undefined2 *)
            ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x594);
      *(undefined2 *)
       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172) =
           *(undefined2 *)
            ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x596);
      *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) =
           *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1f8) | 0x800000;
    }
  }
  if ((*(char *)(_stk + 0x6)) != '\0') {
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162) = (*(undefined2 *)(_stk + 0x4));
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164) = (*(undefined2 *)(_stk + 0x2));
  }
  if (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
       & 8) != 0) &&
     ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x1f8)
      & 8) == 0)) {
    TouchReport_UpdateStartReportPoint(param_1,param_2);
  }
  if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) &
      8) != 0) {
    Touch_GetRptSQDistMMFromStartPos(param_1,param_2);
    Touch_GetRptSQDistMMFromPrevPos(param_1,param_2);
    Touch_GetRptSQDistDisplayPixelFromPrevPos(param_1,param_2);
  }
  if (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
       & 8) == 0) &&
     ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x1f8)
      & 8) == 0)) {
    *(undefined4 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x290) = 1;
  }
  else if (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x1f8) & 8) == 0) ||
          ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)param_2 * 0x478 + 0x1f8) & 8) != 0)) {
    if (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8
                   ) & 8) == 0) ||
       ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_2 * 0x478 + 0x1f8) & 8) == 0)) {
      *(undefined4 *)
       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x290) = 0x20;
    }
    else if (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x1f0) & 0x8000000) == 0) &&
            ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_2 * 0x478 + 0x1f0) & 0x8000000) != 0)) {
      *(undefined4 *)
       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x290) = 2;
    }
    else {
      *(undefined4 *)
       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x290) = 4;
    }
  }
  else {
    *(undefined4 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x290) = 2;
  }
  Touch_GetFltSQDistPixelFromPrevPos(param_1,param_2);
  HotZoneAdjust_Process(param_1,param_2);
  return;
}


// ============================================================================
// Function: TouchReport_IsAnyTouchCancelByPosture @ 6ba7a487
// ============================================================================

undefined8 TouchReport_IsAnyTouchCancelByPosture(void)

{
  char cVar1;
  byte bVar2;
  byte local_9;
  
  cVar1 = TouchReport_IsMultStepBackEnable();
  if (cVar1 != '\0') {
    for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
      bVar2 = PrevTouch_GetIdxBasedOnCurIdx(local_9);
      if ((bVar2 < 0x14) &&
         ((((*(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x1f8) & 0x800000) != 0 ||
           ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)bVar2 * 0x478 + 0x1f8) & 0x800000) != 0)) &&
          ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)bVar2 * 0x478 + 0x200) & 1) != 0)))) {
        return 1;
      }
    }
  }
  return 0;
}


// ============================================================================
// Function: TouchReport_ToBeReported @ 6ba7afbc
// ============================================================================

bool TouchReport_ToBeReported(byte param_1,byte param_2)

{
  char cVar1;
  bool local_9;
  
  cVar1 = TouchReport_PostureAntiTouch(param_1,param_2);
  local_9 = cVar1 == '\0';
  if ((((*(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) & 8) != 0) &&
      (cVar1 = BLReset_InDebounce(), cVar1 != '\0')) &&
     ((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) == 0 ||
      ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0)
       & 0x20000) != 0)))) {
    local_9 = false;
  }
  if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) &
      0x20000000) != 0) {
    local_9 = false;
  }
  if ((*(int *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1ec) ==
       0x40) && (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) == 0)) {
    local_9 = false;
  }
  if (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 500) &
       1) != 0) &&
     (*(int *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c) !=
      0x10)) {
    local_9 = false;
  }
  if (((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x1f8
                 ) & 8) == 0) &&
     (((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x1f8
                 ) & 0x40000) == 0 || (cVar1 = Touch_IsException(param_1,param_2), cVar1 != '\0'))))
  {
    if (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208
                   ) & 2) != 0) &&
       ((*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1ba
                   ) < (short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 2 &&
        (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) == 0)))) {
      local_9 = false;
    }
    if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208)
        & 4) != 0) {
      local_9 = false;
    }
  }
  if (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208)
       & 8) != 0) && (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x1000) != 0)) {
    local_9 = false;
  }
  if (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208)
       & 0x20) != 0) && (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x2000) != 0)) {
    local_9 = false;
  }
  if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c) &
      0x21) != 0) {
    local_9 = false;
  }
  if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) &
      0x2000) != 0) {
    if (*(int *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1ec)
        != 0x40) {
      *(char *)(*(longlong *)_refptr_g_touchesPtr + 3) =
           *(char *)(*(longlong *)_refptr_g_touchesPtr + 3) + '\x01';
    }
    local_9 = false;
  }
  return local_9;
}


// ============================================================================
// Function: TouchReport_IsHistoryBufferApplied @ 6ba7b373
// ============================================================================

undefined1
TouchReport_IsHistoryBufferApplied
          (byte param_1,undefined1 param_2,undefined2 *param_3,undefined2 *param_4)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x38]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x40];
  char cVar1;
  
  (*(undefined1 *)(_stk + 0x37)) = 0;
  if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c) &
      0x38) == 0) {
    (*(undefined2 *)(_stk + 0x18)) = *(undefined2 *)
                ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162);
    (*(undefined2 *)(_stk + 0x1a)) = *(undefined2 *)
                ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164);
    (*(undefined8 *)(_stk + 0x20)) = *(undefined8 *)
                ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x148);
    PrevTouch_HistoryBufferPush(param_2,&(*(undefined2 *)(_stk + 0x18)));
  }
  if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) &
      0x80) != 0) {
    (*(undefined8 *)(_stk + 0x8)) = 0;
    (*(undefined8 *)(_stk + 0x10)) = 0;
    cVar1 = PrevTouch_HistoryBufferPop(param_2,&(*(undefined8 *)(_stk + 0x8)));
    if (cVar1 != '\0') {
      *param_3 = *(undefined2 *)
                  ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162)
      ;
      *param_4 = *(undefined2 *)
                  ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164)
      ;
      *(undefined2 *)
       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162) =
           (undefined2)(*(undefined8 *)(_stk + 0x8));
      *(undefined2 *)
       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164) =
           (undefined2)((*(undefined8 *)(_stk + 0x8)) >> 16);
      if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1f8) & 8) == 0) {
        *(undefined4 *)
         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c) = 2;
      }
      if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x21c) & 0x30) != 0) {
        *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
             = *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x1f8) | 0x200000;
        *(undefined4 *)
         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c) = 4;
      }
      (*(undefined1 *)(_stk + 0x37)) = 1;
    }
  }
  return (*(undefined1 *)(_stk + 0x37));
}


// ============================================================================
// Function: TouchReport_UpdateStartReportPoint @ 6ba7b657
// ============================================================================

void TouchReport_UpdateStartReportPoint(byte param_1,byte param_2)

{
  *(undefined2 *)
   ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x49a) =
       *(undefined2 *)
        ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170);
  *(undefined2 *)
   ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x49c) =
       *(undefined2 *)
        ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
  *(undefined2 *)
   ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x49e) =
       *(undefined2 *)
        ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x156);
  *(undefined2 *)
   ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4a4) =
       *(undefined2 *)
        ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170);
  *(undefined2 *)
   ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4a6) =
       *(undefined2 *)
        ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
  *(undefined2 *)
   ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4ac) =
       *(undefined2 *)
        ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170);
  *(undefined2 *)
   ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4ae) =
       *(undefined2 *)
        ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
  return;
}


// ============================================================================
// Function: TouchReport_Init @ 6ba7a120
// ============================================================================

void TouchReport_Init(void)

{
                    /* 0x3a120  2500  TouchReport_Init */
  HotZoneAdjust_Init();
  return;
}


// ============================================================================
// Function: TouchReport_PostureAntiTouch @ 6ba7ae86
// ============================================================================

undefined8 TouchReport_PostureAntiTouch(byte param_1,byte param_2)

{
  char cVar1;
  
  cVar1 = SensorData_IsAntiTouchStage();
  if (((cVar1 != '\0') || (cVar1 = TouchReport_IsAnyTouchCancelByPosture(), cVar1 != '\0')) ||
     ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x200)
      & 1) != 0)) {
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x200) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x200)
         | 1;
    TouchReport_GoBackReport(param_1,param_2);
    if ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_2 * 0x478 + 0x1f8) & 0x20000000) == 0) {
      *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) =
           *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1f8) | 0x10000;
      return 1;
    }
  }
  return 0;
}


// ============================================================================
// Function: TouchReport_GoBackReport @ 6ba7ae1d
// ============================================================================

void TouchReport_GoBackReport(byte param_1,undefined1 param_2)

{
  char cVar1;
  
                    /* 0x3ae1d  2499  TouchReport_GoBackReport */
  cVar1 = TouchReport_IsMultStepBackEnable();
  if ((cVar1 == '\0') ||
     ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x200) &
      1) == 0)) {
    TouchReport_GoBackOneStep(param_1,param_2);
  }
  else {
    TouchReport_GoBackMultiStep(param_1,param_2);
  }
  return;
}


// ============================================================================
// Function: TouchReport_GoBackOneStep @ 6ba7a134
// ============================================================================

void TouchReport_GoBackOneStep(byte param_1,byte param_2)

{
  *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) =
       *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) |
       0x800000;
  if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) &
      8) == 0) {
    if (*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c)
        < 2) {
      *(undefined2 *)
       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162) =
           *(undefined2 *)
            ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x496);
      *(undefined2 *)
       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164) =
           *(undefined2 *)
            ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x498);
    }
    else {
      *(undefined2 *)
       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162) =
           *(undefined2 *)
            ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4b0);
      *(undefined2 *)
       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164) =
           *(undefined2 *)
            ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4b2);
    }
  }
  else {
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162) =
         *(undefined2 *)
          ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x49a);
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164) =
         *(undefined2 *)
          ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x49c);
  }
  return;
}


// ============================================================================
// Function: TouchReport_GoBackMultiStep @ 6ba7a79b
// ============================================================================

void TouchReport_GoBackMultiStep(byte param_1,byte param_2)

{
  char cVar1;
  int iVar2;
  
  if ((*(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x594
                 ) == 0) &&
     (*(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x596)
      == 0)) {
    if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
        & 8) == 0) {
      if (*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c
                   ) < 2) {
        *(undefined2 *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x594) =
             *(undefined2 *)
              ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x496);
        *(undefined2 *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x596) =
             *(undefined2 *)
              ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x498);
      }
      else {
        *(undefined2 *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x594) =
             *(undefined2 *)
              ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4b0);
        *(undefined2 *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x596) =
             *(undefined2 *)
              ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4b2);
      }
    }
    else {
      *(undefined2 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x594) =
           *(undefined2 *)
            ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x49a);
      *(undefined2 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x596) =
           *(undefined2 *)
            ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x49c);
    }
    *(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x590) =
         *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162
                   ) -
         *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_2 * 0x478 + 0x594);
    *(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x592) =
         *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164
                   ) -
         *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_2 * 0x478 + 0x596);
  }
  cVar1 = TouchReport_IsReadyToCancel(param_1,param_2);
  if (cVar1 == '\0') {
    if (*(byte *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x58e
                 ) < 7) {
      iVar2 = (6 - (uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                  (longlong)(int)(uint)param_2 * 0x478 + 0x58e)) *
              (int)*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)param_2 * 0x478 + 0x590);
      *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162) =
           *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)param_2 * 0x478 + 0x594) +
           ((short)((ulonglong)((longlong)iVar2 * 0x2aaaaaab) >> 0x20) - (short)(iVar2 >> 0x1f));
      iVar2 = (6 - (uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                  (longlong)(int)(uint)param_2 * 0x478 + 0x58e)) *
              (int)*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)param_2 * 0x478 + 0x592);
      *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164) =
           *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)param_2 * 0x478 + 0x596) +
           ((short)((ulonglong)((longlong)iVar2 * 0x2aaaaaab) >> 0x20) - (short)(iVar2 >> 0x1f));
      *(char *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x58e)
           = *(char *)((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_2 * 0x478 + 0x58e) + '\x01';
    }
  }
  else {
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162) =
         *(undefined2 *)
          ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x594);
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164) =
         *(undefined2 *)
          ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x596);
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
         | 0x800000;
    *(undefined1 *)
     ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x58e) = 7;
  }
  return;
}


// ============================================================================
// Function: TouchReport_IsReadyToCancel @ 6ba7a577
// ============================================================================

undefined8 TouchReport_IsReadyToCancel(undefined8 param_1,byte param_2)

{
  char cVar1;
  ulonglong uVar2;
  undefined8 uVar3;
  
  uVar2 = TSAFold_GetTimeStampRegionChange();
  if ((uVar2 != 0) && ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - uVar2 < 100)) {
    return 1;
  }
  if ((((int)((uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a0 *
             (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a6) <=
        (int)(uint)*(ushort *)
                    ((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_2 * 0x478 + 0x162)) ||
      ((int)((uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a0 *
            (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a8) <=
       (int)(uint)*(ushort *)
                   ((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_2 * 0x478 + 0x164))) &&
     (*(ulonglong *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4b8) < uVar2)
     ) {
    return 1;
  }
  cVar1 = TouchReport_IsAnyTouchCancelByPosture();
  if (cVar1 == '\0') {
    if (((*_refptr_g_prevTouchesPtr)->field_0x5 == '\0') ||
       ((((*_refptr_g_prevTouchesPtr)->field_0x5 == '\x01' &&
         (-100 < *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                           (longlong)(int)(uint)param_2 * 0x478 + 0x590))) &&
        ((*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_2 * 0x478 + 0x590) < 100 &&
         ((-100 < *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                            (longlong)(int)(uint)param_2 * 0x478 + 0x592) &&
          (*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)param_2 * 0x478 + 0x592) < 100)))))))) {
      uVar3 = 1;
    }
    else {
      cVar1 = TouchReport_IsAllTouchBack();
      if (cVar1 == '\0') {
        uVar3 = 0;
      }
      else {
        uVar3 = 1;
      }
    }
  }
  else {
    uVar3 = 1;
  }
  return uVar3;
}


