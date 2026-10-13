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
  int iVar1;
  undefined8 uVar2;
  ushort local_16;
  ushort local_14;
  ushort local_12;
  undefined4 local_10;
  undefined2 local_c;
  undefined2 local_a;
  
                    /* 0x1719b  547  ER_IsTouchNeedMoveInCorrection */
  local_a = *(undefined2 *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170);
  local_c = *(undefined2 *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
  g_flagMoveInCorrectedButDisabled = 0;
  local_10 = TSA_GetDimEdge(local_a,local_c);
  ER_GetPrmt(local_10,&local_14,&local_16);
  local_12 = Touch_GetDistToLogicEdge(local_10,local_a,local_c);
  if (local_14 == 0) {
    g_flagMoveInCorrectedButDisabled = 1;
    uVar2 = 0;
  }
  else {
    iVar1 = TPSensor_IsNoMoveInCorrection(local_a,local_c);
    if (iVar1 == 0) {
      if (((local_14 < local_12) && (local_12 < local_16)) &&
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
  char cVar1;
  byte bVar2;
  short sVar3;
  undefined1 local_68 [16];
  ushort local_58;
  ushort local_56;
  ushort local_48;
  ushort local_46;
  int local_2c;
  int local_28;
  ushort local_24;
  ushort local_22;
  ushort local_20;
  ushort local_1e;
  ushort local_1c;
  ushort local_1a;
  int local_18;
  ushort local_14;
  ushort local_12;
  ushort local_10;
  ushort local_e;
  int local_c;
  
                    /* 0x17355  540  ER_BottomMoveInCorrection */
  if (((*(char *)(_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 9) == '\0') ||
      (0x1d < *(ushort *)
               ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x188)))
     || (*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1be
                   ) < 1000)) {
    local_c = 0;
  }
  else {
    local_c = 0;
    if (*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_2 * 0x478 + 0x4e4) == 1) {
      cVar1 = PrevTouch_HistoryBufferGetNum(param_2);
      if (cVar1 != '\0') {
        cVar1 = PrevTouch_HistoryBufferGetNum(param_2);
        PrevTouch_HistoryBufferGet(param_2,cVar1 + -1,&local_48);
        local_e = local_48;
        local_10 = local_46;
        local_12 = *(ushort *)
                    ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x170);
        local_14 = *(ushort *)
                    ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x172);
        local_18 = (uint)local_46 - (uint)local_14;
        cVar1 = ER_IsInBottomZone(local_48,local_46);
        if (((cVar1 != '\0') && (0x32 < local_18)) &&
           ((-0x100 < (int)((uint)local_12 - (uint)local_e) &&
            ((int)((uint)local_12 - (uint)local_e) < 0x100)))) {
          local_c = 1;
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
                 &local_58);
      local_1a = local_58;
      local_1c = local_56;
      cVar1 = PrevTouch_HistoryBufferGetNum(param_2);
      PrevTouch_HistoryBufferGet(param_2,cVar1 + -1,&local_58);
      local_1e = local_58;
      local_20 = local_56;
      local_22 = *(ushort *)
                  ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170)
      ;
      local_24 = *(ushort *)
                  ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172)
      ;
      local_28 = (uint)local_56 - (uint)local_24;
      local_2c = (uint)local_1c - (uint)local_24;
      cVar1 = ER_IsInBottomZone(local_1a,local_1c);
      if ((((((cVar1 != '\0') && (0x32 < local_28)) && (100 < local_2c)) &&
           ((-0x100 < (int)((uint)local_1a - (uint)local_1e) &&
            ((int)((uint)local_1a - (uint)local_1e) < 0x100)))) &&
          ((-0x100 < (int)((uint)local_1e - (uint)local_22) &&
           (((int)((uint)local_1e - (uint)local_22) < 0x100 &&
            (-0x100 < (int)((uint)local_22 - (uint)local_1a))))))) &&
         ((int)((uint)local_22 - (uint)local_1a) < 0x100)) {
        local_c = 1;
      }
    }
    if (local_c != 0) {
      do {
        bVar2 = PrevTouch_HistoryBufferGetNum(param_2);
        if ((ushort)bVar2 <=
            *(ushort *)
             ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4e4))
        break;
        cVar1 = PrevTouch_HistoryBufferPop(param_2,local_68);
      } while (cVar1 != '\0');
    }
  }
  return local_c;
}


// ============================================================================
// Function: ER_MoveInCheck @ 6ba57a07
// ============================================================================

char ER_MoveInCheck(byte param_1,undefined1 param_2)

{
  char cVar1;
  byte bVar2;
  undefined2 local_38;
  undefined2 local_36;
  uint local_28;
  uint local_24;
  uint local_20;
  uint local_1c;
  undefined2 local_16;
  undefined2 local_14;
  undefined2 local_12;
  undefined2 local_10;
  undefined2 local_e;
  undefined2 local_c;
  char local_9;
  
                    /* 0x17a07  549  ER_MoveInCheck */
  cVar1 = PrevTouch_HistoryBufferGetNum(param_2);
  if ((cVar1 != '\0') && (bVar2 = PrevTouch_HistoryBufferGetNum(param_2), 1 < bVar2)) {
    local_9 = '\x01';
    cVar1 = PrevTouch_HistoryBufferGetNum(param_2);
    PrevTouch_HistoryBufferGet(param_2,cVar1 + -2,&local_38);
    local_c = local_38;
    local_e = local_36;
    cVar1 = PrevTouch_HistoryBufferGetNum(param_2);
    PrevTouch_HistoryBufferGet(param_2,cVar1 + -1,&local_38);
    local_10 = local_38;
    local_12 = local_36;
    local_14 = *(undefined2 *)
                ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170);
    local_16 = *(undefined2 *)
                ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
    local_1c = Touch_GetSQDistPixel(local_c,local_e,local_38,local_36);
    local_20 = Touch_GetSQDistPixel(local_10,local_12,local_14,local_16);
    local_24 = Touch_GetSQDistPixel(local_c,local_e,local_14,local_16);
    local_28 = Touch_GetSQDistPixelToEdge(local_c,local_e,local_10,local_12);
    if (local_1c < 0x9c4) {
      local_9 = '\0';
    }
    if (local_20 < 0x9c4) {
      local_9 = '\0';
    }
    if (local_24 < 10000) {
      local_9 = '\0';
    }
    if (local_24 < local_28) {
      local_9 = '\0';
    }
    if (local_9 == '\x01') {
      ER_MoveInCorrection(param_2,param_1);
    }
    return local_9;
  }
  return '\0';
}


// ============================================================================
// Function: ER_Init @ 6ba56f76
// ============================================================================

void ER_Init(void)

{
  undefined2 uVar1;
  undefined4 uVar2;
  undefined1 local_c [2];
  undefined1 local_a [2];
  
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
  ER_GetPrmt(uVar2,local_a,local_c);
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
  char cVar1;
  byte bVar2;
  short local_38;
  short local_36;
  longlong local_30;
  int local_24;
  short local_20;
  short local_1e;
  short local_1c;
  short local_1a;
  short local_18;
  short local_16;
  short local_14;
  short local_12;
  longlong local_10;
  
                    /* 0x17786  550  ER_MoveInCorrection */
  cVar1 = PrevTouch_HistoryBufferGetNum(param_1);
  if ((cVar1 != '\0') && (bVar2 = PrevTouch_HistoryBufferGetNum(param_1), 1 < bVar2)) {
    while (bVar2 = PrevTouch_HistoryBufferGetNum(param_1), 2 < bVar2) {
      PrevTouch_HistoryBufferPop(param_1,&local_38);
      local_16 = local_38;
      local_18 = local_36;
    }
    PrevTouch_HistoryBufferGet(param_1,0,&local_38);
    local_16 = local_38;
    local_18 = local_36;
    local_10 = local_30;
    PrevTouch_HistoryBufferGet(param_1,1,&local_38);
    local_1a = local_38;
    local_1c = local_36;
    local_1e = local_38 - local_16;
    local_20 = local_36 - local_18;
    local_24 = (int)local_30 - (int)local_10;
    local_12 = local_16;
    local_14 = local_18;
    while (((local_1e != 0 && (local_20 != 0)) &&
           (cVar1 = PrevTouch_IsHistoryBufferFull(param_1), cVar1 == '\0'))) {
      local_12 = local_12 - local_1e;
      local_14 = local_14 - local_20;
      local_30 = local_10 - local_24;
      if ((int)local_12 < (int)(uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2a)) {
        local_12 = *(short *)(_refptr_g_tsaPrmtFlashConst + 0x2a);
      }
      if ((int)local_14 < (int)(uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2e)) {
        local_14 = *(short *)(_refptr_g_tsaPrmtFlashConst + 0x2e);
      }
      if ((int)(*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2c) - 1) < (int)local_12) {
        local_12 = *(short *)(_refptr_g_tsaPrmtFlashConst + 0x2c) + -1;
      }
      if ((int)(*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x30) - 1) < (int)local_14) {
        local_14 = *(short *)(_refptr_g_tsaPrmtFlashConst + 0x30) + -1;
      }
      local_38 = local_12;
      local_36 = local_14;
      local_10 = local_30;
      PrevTouch_HistoryBufferInsert(param_1,0,&local_38);
      if ((int)local_12 == (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2a)) {
        return;
      }
      if ((int)local_14 == (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2e)) {
        return;
      }
      if ((int)local_12 == *(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2c) - 1) {
        return;
      }
      if ((int)local_14 == *(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x30) - 1) {
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


