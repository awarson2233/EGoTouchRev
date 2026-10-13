#include "TSACoreInternal.h"

// ============================================================================
// Function: ER_Process @ 6ba584b3
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

void ER_Process(void)

{
  byte bPrevTouchIndex;
  char cMoveOutDetected;
  byte bCurTouchIndex;
  
                    /* 0x184b3  553  ER_Process */
  for (bCurTouchIndex = 0; bCurTouchIndex < **(byte **)_refptr_g_touchesPtr;
      bCurTouchIndex = bCurTouchIndex + 1) {
    bPrevTouchIndex = PrevTouch_GetIdxBasedOnCurIdx(bCurTouchIndex);
    if ((*(uint *)((longlong)(int)(uint)bCurTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                  0x21c) & 2) == 0) {
      if (((*(uint *)((longlong)(int)(uint)bCurTouchIndex * 0x168 +
                      *(longlong *)_refptr_g_touchesPtr + 0x21c) & 0x20) != 0) &&
         ((*(uint *)((longlong)(int)(uint)bCurTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr
                    + 0x1f8) & 0x400) != 0)) {
        *(uint *)((longlong)(int)(uint)bCurTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                 0x1f8) =
             *(uint *)((longlong)(int)(uint)bCurTouchIndex * 0x168 +
                       *(longlong *)_refptr_g_touchesPtr + 0x1f8) & 0xfffffbff;
        *(uint *)((longlong)(int)(uint)bCurTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                 0x1f0) =
             *(uint *)((longlong)(int)(uint)bCurTouchIndex * 0x168 +
                       *(longlong *)_refptr_g_touchesPtr + 0x1f0) | 0x20;
        *(uint *)((longlong)(int)(uint)bCurTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                 0x1f8) =
             *(uint *)((longlong)(int)(uint)bCurTouchIndex * 0x168 +
                       *(longlong *)_refptr_g_touchesPtr + 0x1f8) | 0x80;
      }
    }
    else {
      ER_TouchDownProcess(bCurTouchIndex,bPrevTouchIndex);
    }
    if ((((*(int *)((longlong)(int)(uint)bCurTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr
                   + 0x21c) == 8) ||
         (*(int *)((longlong)(int)(uint)bCurTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                  0x21c) == 0x20)) &&
        ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)bPrevTouchIndex * 0x478 + 0x1f8) & 8) != 0)) &&
       (cMoveOutDetected = ER_MoveOutCheck(bPrevTouchIndex), cMoveOutDetected != '\0')) {
      *(uint *)((longlong)(int)(uint)bCurTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr +
               0x1f0) =
           *(uint *)((longlong)(int)(uint)bCurTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr
                    + 0x1f0) | 0x40;
      ER_MoveOutCorrection(bPrevTouchIndex,bCurTouchIndex);
      if (*(int *)((longlong)(int)(uint)bCurTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                  0x21c) == 0x20) {
        *(undefined4 *)
         ((longlong)(int)(uint)bCurTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c) =
             0x10;
      }
    }
  }
  return;
}


// ============================================================================
// Function: ER_MoveOutCheck @ 6ba57bd3
// ============================================================================

undefined1 ER_MoveOutCheck(byte param_1)

{
  short sVar1;
  undefined1 local_9;
  
                    /* 0x17bd3  551  ER_MoveOutCheck */
  local_9 = 0;
  sVar1 = *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_1 * 0x478 + 0x4d8) +
          *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_1 * 0x478 + 0x162) +
          *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_1 * 0x478 + 0x4d4);
  if ((int)(uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2a) < (int)sVar1) {
    if ((int)(*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2c) - 1) <= (int)sVar1) {
      *(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x204)
           = *(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_1 * 0x478 + 0x204) | 0x20;
      local_9 = 1;
    }
  }
  else {
    *(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x204) =
         *(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_1 * 0x478 + 0x204) | 0x10;
    local_9 = 1;
  }
  sVar1 = *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_1 * 0x478 + 0x4da) +
          *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_1 * 0x478 + 0x164) +
          *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_1 * 0x478 + 0x4d6);
  if ((int)(uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2a) < (int)sVar1) {
    if ((int)(*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x30) - 1) <= (int)sVar1) {
      *(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x204)
           = *(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_1 * 0x478 + 0x204) | 0x80;
      local_9 = 1;
    }
  }
  else {
    *(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x204) =
         *(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_1 * 0x478 + 0x204) | 0x40;
    local_9 = 1;
  }
  return local_9;
}


// ============================================================================
// Function: ER_MoveOutCorrection @ 6ba57e7d
// ============================================================================

void ER_MoveOutCorrection(byte param_1,byte param_2)

{
                    /* 0x17e7d  552  ER_MoveOutCorrection */
  if ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x204)
      & 0x10) != 0) {
    *(undefined2 *)
     ((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170) =
         *(undefined2 *)(_refptr_g_tsaPrmtFlashConst + 0x2a);
  }
  if ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x204)
      & 0x20) != 0) {
    *(short *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170) =
         *(short *)(_refptr_g_tsaPrmtFlashConst + 0x2c) + -1;
  }
  if ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x204)
      & 0x40) != 0) {
    *(undefined2 *)
     ((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172) =
         *(undefined2 *)(_refptr_g_tsaPrmtFlashConst + 0x2e);
  }
  if ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x204)
      & 0x80) != 0) {
    *(short *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172) =
         *(short *)(_refptr_g_tsaPrmtFlashConst + 0x30) + -1;
  }
  return;
}


// ============================================================================
// Function: ER_TouchDownProcess @ 6ba57ffd
// ============================================================================

void ER_TouchDownProcess(byte param_1,byte param_2)

{
  char cVar1;
  
                    /* 0x17ffd  554  ER_TouchDownProcess */
  if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) &
      0x400) == 0) {
    cVar1 = ER_BottomMoveInCorrection(param_1,param_2);
    if (cVar1 == '\0') {
      cVar1 = ER_IsTouchNeedMoveInCorrection(param_1);
      if (cVar1 != '\0') {
        if (*(ushort *)
             ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4e4) <
            2) {
          *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8
                   ) = *(uint *)((longlong)(int)(uint)param_1 * 0x168 +
                                 *(longlong *)_refptr_g_touchesPtr + 0x1f8) | 0x400;
          *(undefined4 *)
           ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c) = 1;
          *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_2 * 0x478 + 0x4e4) =
               *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                         (longlong)(int)(uint)param_2 * 0x478 + 0x4e4) + 1;
          if (*(ushort *)
               ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x238) <
              2) {
            *(undefined2 *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x238) = 2;
          }
        }
        else {
          cVar1 = ER_MoveInCheck(param_1,param_2);
          if (cVar1 != '\0') {
            *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1f0) =
                 *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x1f0) | 0x20;
            *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1f8) =
                 *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x1f8) | 0x80;
          }
        }
      }
    }
    else {
      *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) =
           *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1f0) | 0x20;
      *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) =
           *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1f8) | 0x80;
    }
  }
  else {
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
         & 0xfffffbff;
    cVar1 = ER_BottomMoveInCorrection(param_1,param_2);
    if (cVar1 == '\0') {
      cVar1 = ER_MoveInCheck(param_1,param_2);
      if (cVar1 != '\0') {
        *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0)
             = *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x1f0) | 0x20;
        *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
             = *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x1f8) | 0x80;
      }
    }
    else {
      *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) =
           *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1f0) | 0x20;
      *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) =
           *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1f8) | 0x80;
    }
  }
  return;
}


// ============================================================================
// Function: ER_IsTouchNeedMoveInCorrection @ 6ba5719b
// ============================================================================

undefined8 ER_IsTouchNeedMoveInCorrection(byte param_1)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x16]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x20];
  int iVar1;
  undefined8 uVar2;
  
                    /* 0x1719b  547  ER_IsTouchNeedMoveInCorrection */
  (*(undefined2 *)(_stk + 0x16)) = *(undefined2 *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170);
  (*(undefined2 *)(_stk + 0x14)) = *(undefined2 *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
  g_flagMoveInCorrectedButDisabled = 0;
  (*(undefined4 *)(_stk + 0x10)) = TSA_GetDimEdge((*(undefined2 *)(_stk + 0x16)),(*(undefined2 *)(_stk + 0x14)));
  ER_GetPrmt((*(undefined4 *)(_stk + 0x10)),&(*(ushort *)(_stk + 0xc)),&(*(ushort *)(_stk + 0xa)));
  (*(ushort *)(_stk + 0xe)) = Touch_GetDistToLogicEdge((*(undefined4 *)(_stk + 0x10)),(*(undefined2 *)(_stk + 0x16)),(*(undefined2 *)(_stk + 0x14)));
  if ((*(ushort *)(_stk + 0xc)) == 0) {
    g_flagMoveInCorrectedButDisabled = 1;
    uVar2 = 0;
  }
  else {
    iVar1 = TPSensor_IsNoMoveInCorrection((*(undefined2 *)(_stk + 0x16)),(*(undefined2 *)(_stk + 0x14)));
    if (iVar1 == 0) {
      if ((((*(ushort *)(_stk + 0xc)) < (*(ushort *)(_stk + 0xe))) && ((*(ushort *)(_stk + 0xe)) < (*(ushort *)(_stk + 0xa)))) &&
         (*(ushort *)
           ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x188) < 0x1e
         )) {
        uVar2 = 1;
      }
      else {
        uVar2 = 0;
      }
    }
    else {
      uVar2 = 0;
    }
  }
  return uVar2;
}


// ============================================================================
// Function: ER_BottomMoveInCorrection @ 6ba57355
// ============================================================================

int ER_BottomMoveInCorrection(byte param_1,byte param_2)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x68]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x70];
  char cVar1;
  byte bVar2;
  short sVar3;
  
                    /* 0x17355  540  ER_BottomMoveInCorrection */
  if (((*(char *)(_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 9) == '\0') ||
      (0x1d < *(ushort *)
               ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x188)))
     || (*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1be
                   ) < 1000)) {
    (*(int *)(_stk + 0x64)) = 0;
  }
  else {
    (*(int *)(_stk + 0x64)) = 0;
    if (*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_2 * 0x478 + 0x4e4) == 1) {
      cVar1 = PrevTouch_HistoryBufferGetNum(param_2);
      if (cVar1 != '\0') {
        cVar1 = PrevTouch_HistoryBufferGetNum(param_2);
        PrevTouch_HistoryBufferGet(param_2,cVar1 + -1,&(*(ushort *)(_stk + 0x28)));
        (*(ushort *)(_stk + 0x62)) = (*(ushort *)(_stk + 0x28));
        (*(ushort *)(_stk + 0x60)) = (*(ushort *)(_stk + 0x2a));
        (*(ushort *)(_stk + 0x5e)) = *(ushort *)
                    ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x170);
        (*(ushort *)(_stk + 0x5c)) = *(ushort *)
                    ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x172);
        (*(int *)(_stk + 0x58)) = (uint)(*(ushort *)(_stk + 0x2a)) - (uint)(*(ushort *)(_stk + 0x5c));
        cVar1 = ER_IsInBottomZone((*(ushort *)(_stk + 0x28)),(*(ushort *)(_stk + 0x2a)));
        if (((cVar1 != '\0') && (0x32 < (*(int *)(_stk + 0x58)))) &&
           ((-0x100 < (int)((uint)(*(ushort *)(_stk + 0x5e)) - (uint)(*(ushort *)(_stk + 0x62))) &&
            ((int)((uint)(*(ushort *)(_stk + 0x5e)) - (uint)(*(ushort *)(_stk + 0x62))) < 0x100)))) {
          (*(int *)(_stk + 0x64)) = 1;
        }
      }
    }
    else if (((1 < *(ushort *)
                    ((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_2 * 0x478 + 0x4e4)) &&
             (*(ushort *)
               ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4e4)
              < 4)) &&
            (bVar2 = PrevTouch_HistoryBufferGetNum(param_2),
            *(ushort *)
             ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4e4) <=
            (ushort)bVar2)) {
      sVar3 = PrevTouch_HistoryBufferGetNum(param_2);
      PrevTouch_HistoryBufferGet
                (param_2,sVar3 - *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                                           (longlong)(int)(uint)param_2 * 0x478 + 0x4e4) & 0xff,
                 &(*(ushort *)(_stk + 0x18)));
      (*(ushort *)(_stk + 0x56)) = (*(ushort *)(_stk + 0x18));
      (*(ushort *)(_stk + 0x54)) = (*(ushort *)(_stk + 0x1a));
      cVar1 = PrevTouch_HistoryBufferGetNum(param_2);
      PrevTouch_HistoryBufferGet(param_2,cVar1 + -1,&(*(ushort *)(_stk + 0x18)));
      (*(ushort *)(_stk + 0x52)) = (*(ushort *)(_stk + 0x18));
      (*(ushort *)(_stk + 0x50)) = (*(ushort *)(_stk + 0x1a));
      (*(ushort *)(_stk + 0x4e)) = *(ushort *)
                  ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170)
      ;
      (*(ushort *)(_stk + 0x4c)) = *(ushort *)
                  ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172)
      ;
      (*(int *)(_stk + 0x48)) = (uint)(*(ushort *)(_stk + 0x1a)) - (uint)(*(ushort *)(_stk + 0x4c));
      (*(int *)(_stk + 0x44)) = (uint)(*(ushort *)(_stk + 0x54)) - (uint)(*(ushort *)(_stk + 0x4c));
      cVar1 = ER_IsInBottomZone((*(ushort *)(_stk + 0x56)),(*(ushort *)(_stk + 0x54)));
      if ((((((cVar1 != '\0') && (0x32 < (*(int *)(_stk + 0x48)))) && (100 < (*(int *)(_stk + 0x44)))) &&
           ((-0x100 < (int)((uint)(*(ushort *)(_stk + 0x56)) - (uint)(*(ushort *)(_stk + 0x52))) &&
            ((int)((uint)(*(ushort *)(_stk + 0x56)) - (uint)(*(ushort *)(_stk + 0x52))) < 0x100)))) &&
          ((-0x100 < (int)((uint)(*(ushort *)(_stk + 0x52)) - (uint)(*(ushort *)(_stk + 0x4e))) &&
           (((int)((uint)(*(ushort *)(_stk + 0x52)) - (uint)(*(ushort *)(_stk + 0x4e))) < 0x100 &&
            (-0x100 < (int)((uint)(*(ushort *)(_stk + 0x4e)) - (uint)(*(ushort *)(_stk + 0x56))))))))) &&
         ((int)((uint)(*(ushort *)(_stk + 0x4e)) - (uint)(*(ushort *)(_stk + 0x56))) < 0x100)) {
        (*(int *)(_stk + 0x64)) = 1;
      }
    }
    if ((*(int *)(_stk + 0x64)) != 0) {
      do {
        bVar2 = PrevTouch_HistoryBufferGetNum(param_2);
        if ((ushort)bVar2 <=
            *(ushort *)
             ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4e4))
        break;
        cVar1 = PrevTouch_HistoryBufferPop(param_2,((undefined1 *)(_stk + 0x8)));
      } while (cVar1 != '\0');
    }
  }
  return (*(int *)(_stk + 0x64));
}


// ============================================================================
// Function: ER_MoveInCheck @ 6ba57a07
// ============================================================================

char ER_MoveInCheck(byte param_1,undefined1 param_2)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x38]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x40];
  char cVar1;
  byte bVar2;
  
                    /* 0x17a07  549  ER_MoveInCheck */
  cVar1 = PrevTouch_HistoryBufferGetNum(param_2);
  if ((cVar1 != '\0') && (bVar2 = PrevTouch_HistoryBufferGetNum(param_2), 1 < bVar2)) {
    (*(char *)(_stk + 0x37)) = '\x01';
    cVar1 = PrevTouch_HistoryBufferGetNum(param_2);
    PrevTouch_HistoryBufferGet(param_2,cVar1 + -2,&(*(undefined2 *)(_stk + 0x8)));
    (*(undefined2 *)(_stk + 0x34)) = (*(undefined2 *)(_stk + 0x8));
    (*(undefined2 *)(_stk + 0x32)) = (*(undefined2 *)(_stk + 0xa));
    cVar1 = PrevTouch_HistoryBufferGetNum(param_2);
    PrevTouch_HistoryBufferGet(param_2,cVar1 + -1,&(*(undefined2 *)(_stk + 0x8)));
    (*(undefined2 *)(_stk + 0x30)) = (*(undefined2 *)(_stk + 0x8));
    (*(undefined2 *)(_stk + 0x2e)) = (*(undefined2 *)(_stk + 0xa));
    (*(undefined2 *)(_stk + 0x2c)) = *(undefined2 *)
                ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170);
    (*(undefined2 *)(_stk + 0x2a)) = *(undefined2 *)
                ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
    (*(uint *)(_stk + 0x24)) = Touch_GetSQDistPixel((*(undefined2 *)(_stk + 0x34)),(*(undefined2 *)(_stk + 0x32)),(*(undefined2 *)(_stk + 0x8)),(*(undefined2 *)(_stk + 0xa)));
    (*(uint *)(_stk + 0x20)) = Touch_GetSQDistPixel((*(undefined2 *)(_stk + 0x30)),(*(undefined2 *)(_stk + 0x2e)),(*(undefined2 *)(_stk + 0x2c)),(*(undefined2 *)(_stk + 0x2a)));
    (*(uint *)(_stk + 0x1c)) = Touch_GetSQDistPixel((*(undefined2 *)(_stk + 0x34)),(*(undefined2 *)(_stk + 0x32)),(*(undefined2 *)(_stk + 0x2c)),(*(undefined2 *)(_stk + 0x2a)));
    (*(uint *)(_stk + 0x18)) = Touch_GetSQDistPixelToEdge((*(undefined2 *)(_stk + 0x34)),(*(undefined2 *)(_stk + 0x32)),(*(undefined2 *)(_stk + 0x30)),(*(undefined2 *)(_stk + 0x2e)));
    if ((*(uint *)(_stk + 0x24)) < 0x9c4) {
      (*(char *)(_stk + 0x37)) = '\0';
    }
    if ((*(uint *)(_stk + 0x20)) < 0x9c4) {
      (*(char *)(_stk + 0x37)) = '\0';
    }
    if ((*(uint *)(_stk + 0x1c)) < 10000) {
      (*(char *)(_stk + 0x37)) = '\0';
    }
    if ((*(uint *)(_stk + 0x1c)) < (*(uint *)(_stk + 0x18))) {
      (*(char *)(_stk + 0x37)) = '\0';
    }
    if ((*(char *)(_stk + 0x37)) == '\x01') {
      ER_MoveInCorrection(param_2);
    }
    return (*(char *)(_stk + 0x37));
  }
  return '\0';
}


// ============================================================================
// Function: ER_Init @ 6ba56f76
// ============================================================================

void ER_Init(void)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0xc]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x10];
  undefined2 uVar1;
  undefined4 uVar2;
  
                    /* 0x16f76  544  ER_Init */
  uVar1 = ER_MMPrmtToPixelPrmt
                    (*(undefined2 *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x44),1);
  *(undefined2 *)(_refptr_g_tsaPrmtFlashConst + 0x40) = uVar1;
  uVar1 = ER_MMPrmtToPixelPrmt
                    (*(undefined2 *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x46),1);
  *(undefined2 *)(_refptr_g_tsaPrmtFlashConst + 0x42) = uVar1;
  uVar1 = ER_MMPrmtToPixelPrmt
                    (*(undefined2 *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x4a),2);
  *(undefined2 *)(_refptr_g_tsaPrmtFlashConst + 0x46) = uVar1;
  uVar1 = ER_MMPrmtToPixelPrmt
                    (*(undefined2 *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x4c),2);
  *(undefined2 *)(_refptr_g_tsaPrmtFlashConst + 0x48) = uVar1;
  uVar1 = ER_MMPrmtToPixelPrmt(*(undefined2 *)(_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 10),4);
  *(undefined2 *)(_refptr_g_tsaPrmtFlashConst + 0x4c) = uVar1;
  uVar1 = ER_MMPrmtToPixelPrmt
                    (*(undefined2 *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x52),4);
  *(undefined2 *)(_refptr_g_tsaPrmtFlashConst + 0x4e) = uVar1;
  uVar1 = ER_MMPrmtToPixelPrmt
                    (*(undefined2 *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x56),8);
  *(undefined2 *)(_refptr_g_tsaPrmtFlashConst + 0x52) = uVar1;
  uVar1 = ER_MMPrmtToPixelPrmt(*(undefined2 *)(_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0xb),8);
  *(undefined2 *)(_refptr_g_tsaPrmtFlashConst + 0x54) = uVar1;
  uVar2 = TSA_SideToDimEdge(1);
  ER_GetPrmt(uVar2,((undefined1 *)(_stk + 0x6)),((undefined1 *)(_stk + 0x4)));
  return;
}


// ============================================================================
// Function: ER_GetPrmt @ 6ba56ec0
// ============================================================================

void ER_GetPrmt(undefined4 param_1,undefined2 *param_2,undefined2 *param_3)

{
  byte bVar1;
  undefined4 uVar2;
  
                    /* 0x16ec0  543  ER_GetPrmt */
  uVar2 = TSA_DimEdgeToSide(param_1);
  bVar1 = TSA_SideToEdge(uVar2);
  *param_2 = *(undefined2 *)(_refptr_g_tsaPrmtFlashConst + (longlong)(int)(uint)bVar1 * 6 + 0x40);
  *param_3 = *(undefined2 *)(_refptr_g_tsaPrmtFlashConst + (longlong)(int)(uint)bVar1 * 6 + 0x42);
  g_erLimit = *(undefined2 *)
               ((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
               (longlong)(int)(uint)bVar1 * 6 + 0x44);
  return;
}


// ============================================================================
// Function: ER_IsInBottomZone @ 6ba572bf
// ============================================================================

undefined8 ER_IsInBottomZone(undefined2 param_1,undefined2 param_2)

{
  ushort uVar1;
  ushort uVar2;
  undefined8 uVar3;
  
                    /* 0x172bf  545  ER_IsInBottomZone */
  uVar1 = TSAOut_Pixel2DisplayDim1(param_1);
  uVar2 = TSAOut_Pixel2DisplayDim2(param_2);
  if ((((int)(uint)uVar2 < (int)(*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x9a - 0x36)) ||
      ((uint)uVar1 < *(ushort *)&_refptr_g_tsaPrmtFlash->field_0x94 + 100)) ||
     ((int)(*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x96 - 100) <= (int)(uint)uVar1)) {
    uVar3 = 0;
  }
  else {
    uVar3 = 1;
  }
  return uVar3;
}


// ============================================================================
// Function: ER_MoveInCorrection @ 6ba57786
// ============================================================================

void ER_MoveInCorrection(undefined1 param_1)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x38]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x40];
  char cVar1;
  byte bVar2;
  
                    /* 0x17786  550  ER_MoveInCorrection */
  cVar1 = PrevTouch_HistoryBufferGetNum(param_1);
  if ((cVar1 != '\0') && (bVar2 = PrevTouch_HistoryBufferGetNum(param_1), 1 < bVar2)) {
    while (bVar2 = PrevTouch_HistoryBufferGetNum(param_1), 2 < bVar2) {
      PrevTouch_HistoryBufferPop(param_1,&(*(short *)(_stk + 0x8)));
      (*(short *)(_stk + 0x2a)) = (*(short *)(_stk + 0x8));
      (*(short *)(_stk + 0x28)) = (*(short *)(_stk + 0xa));
    }
    PrevTouch_HistoryBufferGet(param_1,0,&(*(short *)(_stk + 0x8)));
    (*(short *)(_stk + 0x2a)) = (*(short *)(_stk + 0x8));
    (*(short *)(_stk + 0x28)) = (*(short *)(_stk + 0xa));
    (*(longlong *)(_stk + 0x30)) = (*(longlong *)(_stk + 0x10));
    PrevTouch_HistoryBufferGet(param_1,1,&(*(short *)(_stk + 0x8)));
    (*(short *)(_stk + 0x26)) = (*(short *)(_stk + 0x8));
    (*(short *)(_stk + 0x24)) = (*(short *)(_stk + 0xa));
    (*(short *)(_stk + 0x22)) = (*(short *)(_stk + 0x8)) - (*(short *)(_stk + 0x2a));
    (*(short *)(_stk + 0x20)) = (*(short *)(_stk + 0xa)) - (*(short *)(_stk + 0x28));
    (*(int *)(_stk + 0x1c)) = (int)(*(longlong *)(_stk + 0x10)) - (int)(*(longlong *)(_stk + 0x30));
    (*(short *)(_stk + 0x2e)) = (*(short *)(_stk + 0x2a));
    (*(short *)(_stk + 0x2c)) = (*(short *)(_stk + 0x28));
    while ((((*(short *)(_stk + 0x22)) != 0 && ((*(short *)(_stk + 0x20)) != 0)) &&
           (cVar1 = PrevTouch_IsHistoryBufferFull(param_1), cVar1 == '\0'))) {
      (*(short *)(_stk + 0x2e)) = (*(short *)(_stk + 0x2e)) - (*(short *)(_stk + 0x22));
      (*(short *)(_stk + 0x2c)) = (*(short *)(_stk + 0x2c)) - (*(short *)(_stk + 0x20));
      (*(longlong *)(_stk + 0x10)) = (*(longlong *)(_stk + 0x30)) - (*(int *)(_stk + 0x1c));
      if ((int)(*(short *)(_stk + 0x2e)) < (int)(uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2a)) {
        (*(short *)(_stk + 0x2e)) = *(short *)(_refptr_g_tsaPrmtFlashConst + 0x2a);
      }
      if ((int)(*(short *)(_stk + 0x2c)) < (int)(uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2e)) {
        (*(short *)(_stk + 0x2c)) = *(short *)(_refptr_g_tsaPrmtFlashConst + 0x2e);
      }
      if ((int)(*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2c) - 1) < (int)(*(short *)(_stk + 0x2e))) {
        (*(short *)(_stk + 0x2e)) = *(short *)(_refptr_g_tsaPrmtFlashConst + 0x2c) + -1;
      }
      if ((int)(*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x30) - 1) < (int)(*(short *)(_stk + 0x2c))) {
        (*(short *)(_stk + 0x2c)) = *(short *)(_refptr_g_tsaPrmtFlashConst + 0x30) + -1;
      }
      (*(short *)(_stk + 0x8)) = (*(short *)(_stk + 0x2e));
      (*(short *)(_stk + 0xa)) = (*(short *)(_stk + 0x2c));
      (*(longlong *)(_stk + 0x30)) = (*(longlong *)(_stk + 0x10));
      PrevTouch_HistoryBufferInsert(param_1,0,&(*(short *)(_stk + 0x8)));
      if ((int)(*(short *)(_stk + 0x2e)) == (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2a)) {
        return;
      }
      if ((int)(*(short *)(_stk + 0x2c)) == (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2e)) {
        return;
      }
      if ((int)(*(short *)(_stk + 0x2e)) == *(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2c) - 1) {
        return;
      }
      if ((int)(*(short *)(_stk + 0x2c)) == *(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x30) - 1) {
        return;
      }
    }
  }
  return;
}


// ============================================================================
// Function: ER_MMPrmtToPixelPrmt @ 6ba56e50
// ============================================================================

undefined2 ER_MMPrmtToPixelPrmt(undefined1 param_1,undefined4 param_2)

{
  short sVar1;
  ushort uVar2;
  int iVar3;
  undefined2 local_a;
  
                    /* 0x16e50  548  ER_MMPrmtToPixelPrmt */
  uVar2 = TS_MMToDisplayPixel(param_1);
  sVar1 = (uVar2 >> 4) - 5;
  iVar3 = TSA_SideToDimEdge(param_2);
  if ((iVar3 == 1) || (iVar3 == 2)) {
    local_a = TSAOut_RevertDim1Pos(sVar1);
  }
  else {
    local_a = TSAOut_RevertDim2Pos(sVar1);
  }
  return local_a;
}


// ============================================================================
// Function: ER_GetFlagMoveInCorrectedButDisabled @ 6ba570f4
// ============================================================================

undefined1 ER_GetFlagMoveInCorrectedButDisabled(void)

{
                    /* 0x170f4  541  ER_GetFlagMoveInCorrectedButDisabled */
  return g_flagMoveInCorrectedButDisabled;
}


// ============================================================================
// Function: ER_GetLimit @ 6ba57101
// ============================================================================

ushort ER_GetLimit(void)

{
  ushort uVar1;
  
                    /* 0x17101  542  ER_GetLimit */
  uVar1 = TS_MMToDisplayPixel(g_erLimit & 0xff);
  return uVar1 >> 4;
}


