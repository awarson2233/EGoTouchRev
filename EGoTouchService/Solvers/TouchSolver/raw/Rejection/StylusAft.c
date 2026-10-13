#include "TSACoreInternal.h"

// ============================================================================
// Function: StylusAft_PostProcessUnit @ 6bb223ef
// ============================================================================

void StylusAft_PostProcessUnit(byte param_1)

{
  char cVar1;
  undefined1 uVar2;
  char local_a;
  byte local_9;
  
                    /* 0xe23ef  1448  StylusAft_PostProcessUnit */
  local_9 = PrevTouch_GetIdxBasedOnCurIdx(param_1);
  local_a = '\0';
  cVar1 = ASA_GetBluetoothConnectStatus();
  if (cVar1 != '\0') {
    if ((*(ushort *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x156) < 0xf1)
       || (*(char *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)local_9 * 0x478 + 0x58d) != '\0')) {
      if ((*_refptr_g_tsaStaticPtr)->bField_0x111 < 3) {
        if (*(char *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)local_9 * 0x478 + 0x58c) == '\0') {
          if ((((*_refptr_g_tsaStaticPtr)->bField_0x111 < 2) ||
              (((*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags & 0x20) != 0)) &&
             (cVar1 = StylusAft_PalmLocationJudge(param_1,0x32), cVar1 != '\0')) {
            StylusAft_ProcessWithdraw(param_1,local_9,&local_a);
          }
        }
        else {
          *(undefined1 *)
           ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_9 * 0x478 + 0x58d) =
               0x28;
        }
        if (local_a != '\x01') {
          StylusAft_ProcessSuppress(param_1,local_9);
        }
        if (*(char *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)local_9 * 0x478 + 0x58d) != '\0') {
          *(char *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)local_9 * 0x478 + 0x58d) =
               *(char *)((longlong)*_refptr_g_prevTouchesPtr +
                        (longlong)(int)(uint)local_9 * 0x478 + 0x58d) + -1;
          *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208
                   ) = *(uint *)((longlong)(int)(uint)param_1 * 0x168 +
                                 *(longlong *)_refptr_g_touchesPtr + 0x208) | 0x40;
        }
        if ((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x1000000) != 0) &&
           (cVar1 = StylusAft_InEdge(param_1), cVar1 == '\0')) {
          cVar1 = StylusAft_BtStatusPenAttach();
          if ((cVar1 == '\0') && (cVar1 = StylusAft_BtStatusPenHibernate(), cVar1 == '\0')) {
            uVar2 = 1;
          }
          else {
            uVar2 = 0;
          }
          StylusAft_SuppressBaseContext(param_1,uVar2);
        }
        if ((((*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags & 0x20) != 0) &&
           (*(ushort *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x156) < 3)
           ) {
          if (*_refptr_g_tsaLogLoglevel != '\0') {
            StrPrintf_Process(0,"\nStylusAFT: Id=%d, age : %d, (3debounce Suppressed)\n",param_1,
                              *(undefined2 *)
                               ((longlong)(int)(uint)param_1 * 0x168 +
                                *(longlong *)_refptr_g_touchesPtr + 0x156));
          }
          *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208
                   ) = *(uint *)((longlong)(int)(uint)param_1 * 0x168 +
                                 *(longlong *)_refptr_g_touchesPtr + 0x208) | 0x40;
        }
        if ((g_lastStylusTimeStamp < (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr) &&
           ((ulonglong)
            ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - (longlong)g_lastStylusTimeStamp)
            < 200)) {
          *(undefined1 *)
           ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_9 * 0x478 + 0x58c) = 1;
          *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208
                   ) = *(uint *)((longlong)(int)(uint)param_1 * 0x168 +
                                 *(longlong *)_refptr_g_touchesPtr + 0x208) | 0x40;
        }
        if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x208) & 0x40) != 0) {
          *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8
                   ) = *(uint *)((longlong)(int)(uint)param_1 * 0x168 +
                                 *(longlong *)_refptr_g_touchesPtr + 0x1f8) & 0xfffffff7;
        }
        if (*_refptr_g_tsaLogLoglevel != '\0') {
          StrPrintf_Process(0,
                            "\nStylusAFT: Id=%d, withdrawFlag=%d, suppressNum=%d, maxAxisMajor=%d, ellipseRate=%d"
                            ,param_1,*(undefined1 *)
                                      ((longlong)*_refptr_g_prevTouchesPtr +
                                      (longlong)(int)(uint)local_9 * 0x478 + 0x58c),
                            *(undefined1 *)
                             ((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)local_9 * 0x478 + 0x58d),
                            *(undefined2 *)
                             ((longlong)(int)(uint)param_1 * 0x168 +
                              *(longlong *)_refptr_g_touchesPtr + 0x228),
                            (int)*(short *)((longlong)(int)(uint)param_1 * 0x168 +
                                            *(longlong *)_refptr_g_touchesPtr + 0x1a0));
        }
        if (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x208) & 0x40) != 0) && (*_refptr_g_tsaLogLoglevel != '\0')) {
          StrPrintf_Process(0,"(Suppressed)");
        }
        if (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x208) & 0x80) != 0) && (*_refptr_g_tsaLogLoglevel != '\0')) {
          StrPrintf_Process(0,"(Withdrawn)");
        }
      }
      else {
        *(undefined1 *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_9 * 0x478 + 0x58d) = 0;
        *(undefined1 *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_9 * 0x478 + 0x58c) = 0;
        *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208)
             = *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x208) & 0xffffffbf;
        if (*_refptr_g_tsaLogLoglevel != '\0') {
          StrPrintf_Process(0,"\nStylusAFT: Id=%d, (noise level %d is over limit, stylusAFT closed)"
                            ,param_1,(*_refptr_g_tsaStaticPtr)->bField_0x111);
        }
      }
    }
    else if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"\nStylusAFT: Id=%d, Age: %d out of range, Unsuppressed",param_1,
                        *(undefined2 *)
                         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x156));
    }
  }
  return;
}


// ============================================================================
// Function: StylusAft_BtStatusPenAttach @ 6bb20932
// ============================================================================

bool StylusAft_BtStatusPenAttach(void)

{
                    /* 0xe0932  1434  StylusAft_BtStatusPenAttach */
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"\nStylusAFT: StylusAft_BtStatusPenAttach g_btPenStatus=%d\n",
                      *_refptr_g_btPenStatus);
  }
  return (*_refptr_g_btPenStatus & 2) != 0;
}


// ============================================================================
// Function: StylusAft_BtStatusPenHibernate @ 6bb2098f
// ============================================================================

undefined8 StylusAft_BtStatusPenHibernate(void)

{
  undefined8 uVar1;
  
                    /* 0xe098f  1435  StylusAft_BtStatusPenHibernate */
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"\nStylusAFT: StylusAft_BtStatusPenHibernate g_btPenStatus=%d\n",
                      *_refptr_g_btPenStatus);
  }
  if (((*_refptr_g_btPenStatus & 1) == 0) && (*(int *)(_refptr_g_asaHpp3Freq + 4) == 1)) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: StylusAft_PalmLocationJudge @ 6bb20c5e
// ============================================================================

undefined8 StylusAft_PalmLocationJudge(undefined1 param_1,byte param_2)

{
  ushort uVar1;
  
                    /* 0xe0c5e  1446  StylusAft_PalmLocationJudge */
  uVar1 = TSAOut_DisplayXPos(param_1);
  if ((((param_2 < uVar1) &&
       (uVar1 = TSAOut_DisplayXPos(param_1),
       (int)(uint)uVar1 <
       (int)((uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a6 - (uint)param_2))) &&
      (uVar1 = TSAOut_DisplayYPos(param_1), param_2 < uVar1)) &&
     (uVar1 = TSAOut_DisplayYPos(param_1),
     (int)(uint)uVar1 < (int)((uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a8 - (uint)param_2)
     )) {
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: StylusAft_ProcessWithdraw @ 6bb216da
// ============================================================================

void StylusAft_ProcessWithdraw(byte param_1,byte param_2,undefined1 *param_3)

{
  undefined1 uVar1;
  char cVar2;
  
                    /* 0xe16da  1450  StylusAft_ProcessWithdraw */
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    uVar1 = TSA_RptLONum();
    StrPrintf_Process(0,
                      "\nStylusAFT: Id=%d, TouchPrpt(curIdx, persistFlag)= %ld, PrevTouchPrpt(preIdx, persistFlag)= %ld, TouchPrpt(curIdx, volatileFlag)= %ld, PrevTouchPrpt(preIdx, volatileFlag)= %ld\n, TSA_RptLONum()=%d"
                      ,param_1,*(undefined4 *)
                                ((longlong)(int)(uint)param_1 * 0x168 +
                                 *(longlong *)_refptr_g_touchesPtr + 0x1f8),
                      *(undefined4 *)
                       ((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_2 * 0x478 + 0x1f8),
                      *(undefined4 *)
                       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x1f0),*(undefined4 *)
                               ((longlong)*_refptr_g_prevTouchesPtr +
                               (longlong)(int)(uint)param_2 * 0x478 + 0x1f0),uVar1);
  }
  if (((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x1f8
                 ) & 0x20000000) == 0) && (cVar2 = TSA_RptLONum(), cVar2 == '\0')) {
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      uVar1 = StylusAft_GetPalmLevel(param_1);
      StrPrintf_Process(0,"\nStylusAFT: Id=%d, PalmLevel:%d, (withdrawn return)\n",param_1,uVar1);
    }
  }
  else {
    if ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_2 * 0x478 + 0x1f0) & 0x8000000) != 0) {
      if (*_refptr_g_tsaLogLoglevel != '\0') {
        uVar1 = StylusAft_GetPalmLevel(param_1);
        StrPrintf_Process(0,"\nStylusAFT: PalmLevel:%d, (volatile withdrawn)\n",param_1,uVar1);
      }
      *(undefined1 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x58c) = 1;
      *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) =
           *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1f0) & 0xf7ffffff;
    }
    cVar2 = StylusAft_PalmJudge(param_1);
    if (cVar2 == '\0') {
      cVar2 = StylusAft_FanAreaJudge(param_1);
      if (cVar2 == '\0') {
        cVar2 = StylusAft_CurrentDomainJudge(param_1);
        if (cVar2 == '\0') {
          cVar2 = StylusAft_EarlyPalmJudge(param_1);
          if (cVar2 == '\0') {
            cVar2 = StylusAft_EdgePalmJudge(param_1);
            if (cVar2 == '\0') {
              if (*_refptr_g_tsaLogLoglevel != '\0') {
                StrPrintf_Process(0,"\nStylusAFT withdraw else\n");
              }
            }
            else {
              *(undefined1 *)
               ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x58c)
                   = 1;
            }
          }
          else {
            *(undefined1 *)
             ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x58c) =
                 1;
          }
        }
        else {
          *(undefined1 *)
           ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x58c) = 1;
        }
      }
      else {
        *(undefined1 *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x58c) = 1;
      }
    }
    else {
      *(undefined1 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x58c) = 1;
    }
    if (*(char *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x58c
                 ) != '\0') {
      *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208) =
           *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x208) | 0x80;
      *param_3 = 1;
    }
  }
  return;
}


// ============================================================================
// Function: StylusAft_ProcessSuppress @ 6bb21ac8
// ============================================================================

void StylusAft_ProcessSuppress(byte param_1,byte param_2)

{
  char cVar1;
  
                    /* 0xe1ac8  1449  StylusAft_ProcessSuppress */
  if ((0x96 < *(ushort *)
               ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x24e))
     && (800 < (ulonglong)
               ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - g_lastStylusTimeStamp))) {
    *(byte *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x58d) =
         *(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_2 * 0x478 + 0x58d) >> 1;
  }
  cVar1 = StylusAft_PalmJudge(param_1);
  if (cVar1 == '\0') {
    cVar1 = StylusAft_FanAreaJudge(param_1);
    if (cVar1 == '\0') {
      cVar1 = StylusAft_CurrentDomainJudge(param_1);
      if (cVar1 == '\0') {
        cVar1 = StylusAft_PostDomainJudge(param_1);
        if (cVar1 == '\0') {
          cVar1 = StylusAft_EarlyPalmJudge(param_1);
          if (cVar1 == '\0') {
            cVar1 = StylusAft_EdgePalmJudge(param_1);
            if (cVar1 != '\0') {
              *(undefined1 *)
               ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x58d)
                   = 0x28;
            }
          }
          else {
            *(undefined1 *)
             ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x58d) =
                 0x28;
          }
        }
        else {
          *(undefined1 *)
           ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x58d) =
               0x28;
        }
      }
      else {
        *(undefined1 *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x58d) = 0x28
        ;
      }
    }
    else {
      *(undefined1 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x58d) = 0x28;
    }
  }
  else {
    *(undefined1 *)
     ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x58d) = 100;
  }
  return;
}


// ============================================================================
// Function: StylusAft_InEdge @ 6bb21cc0
// ============================================================================

undefined1 StylusAft_InEdge(undefined1 param_1)

{
  ushort uVar1;
  int iVar2;
  ulonglong uVar3;
  
                    /* 0xe1cc0  1442  StylusAft_InEdge */
  uVar1 = TSAOut_DisplayXPos(param_1);
  uVar3 = (ulonglong)((uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a6 * 3);
  if ((uint)(uVar3 / (ulonglong)(longlong)(int)(uint)_refptr_g_tsaPrmtFlash->bCols) < (uint)uVar1) {
    uVar1 = TSAOut_DisplayXPos(param_1,uVar3 % (ulonglong)
                                               (longlong)(int)(uint)_refptr_g_tsaPrmtFlash->bCols);
    iVar2 = (_refptr_g_tsaPrmtFlash->bCols - 3) *
            (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a6;
    if ((int)(uint)uVar1 < iVar2 / (int)(uint)_refptr_g_tsaPrmtFlash->bCols) {
      uVar1 = TSAOut_DisplayYPos(param_1,(longlong)iVar2 %
                                         (longlong)(int)(uint)_refptr_g_tsaPrmtFlash->bCols &
                                         0xffffffff);
      uVar3 = (ulonglong)((uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a8 * 3);
      if (((uint)(uVar3 / (ulonglong)(longlong)(int)(uint)_refptr_g_tsaPrmtFlash->bRows) <
           (uint)uVar1) &&
         (uVar1 = TSAOut_DisplayYPos(param_1,uVar3 % (ulonglong)
                                                     (longlong)
                                                     (int)(uint)_refptr_g_tsaPrmtFlash->bRows),
         (int)(uint)uVar1 <
         (int)((_refptr_g_tsaPrmtFlash->bRows - 3) *
              (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a8) /
         (int)(uint)_refptr_g_tsaPrmtFlash->bRows)) {
        return 0;
      }
    }
  }
  return 1;
}


// ============================================================================
// Function: StylusAft_SuppressBaseContext @ 6bb21dfd
// ============================================================================

void StylusAft_SuppressBaseContext(byte param_1,char param_2)

{
  short local_a;
  
                    /* 0xe1dfd  1452  StylusAft_SuppressBaseContext */
  if (param_2 == '\0') {
    local_a = 10;
  }
  else {
    local_a = 5;
    if (*(ushort *)
         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x156) < 2) {
      *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208) =
           *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x208) | 0x40;
      if (*_refptr_g_tsaLogLoglevel == '\0') {
        return;
      }
      StrPrintf_Process(0,"\nStylusAFT: Id=%d, age : %d, ellipseRate: %d (3debounce Suppressed)\n",
                        param_1,*(undefined2 *)
                                 ((longlong)(int)(uint)param_1 * 0x168 +
                                  *(longlong *)_refptr_g_touchesPtr + 0x156),
                        (int)*(short *)((longlong)(int)(uint)param_1 * 0x168 +
                                        *(longlong *)_refptr_g_touchesPtr + 0x1a0));
      return;
    }
  }
  if (((int)*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x1a0) < local_a + 2) ||
     (2 < *(ushort *)
           ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x156))) {
    if (((int)*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x1a0) < local_a + 4) ||
       (3 < *(ushort *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x156))) {
      if (((int)*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x1a0) < local_a + 6) ||
         (4 < *(ushort *)
               ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x156)))
      {
        if ((local_a + 8 <=
             (int)*(short *)((longlong)(int)(uint)param_1 * 0x168 +
                             *(longlong *)_refptr_g_touchesPtr + 0x1a0)) &&
           ((*(ushort *)
              ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x156) < 6
            && (*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x208) =
                     *(uint *)((longlong)(int)(uint)param_1 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x208) | 0x40,
               *_refptr_g_tsaLogLoglevel != '\0')))) {
          StrPrintf_Process(0,
                            "\nStylusAFT: Id=%d, age : %d, ellipseRate: %d (3debounce Suppressed)\n"
                            ,param_1,*(undefined2 *)
                                      ((longlong)(int)(uint)param_1 * 0x168 +
                                       *(longlong *)_refptr_g_touchesPtr + 0x156),
                            (int)*(short *)((longlong)(int)(uint)param_1 * 0x168 +
                                            *(longlong *)_refptr_g_touchesPtr + 0x1a0));
        }
      }
      else {
        *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208)
             = *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x208) | 0x40;
        if (*_refptr_g_tsaLogLoglevel != '\0') {
          StrPrintf_Process(0,
                            "\nStylusAFT: Id=%d, age : %d, ellipseRate: %d (3debounce Suppressed)\n"
                            ,param_1,*(undefined2 *)
                                      ((longlong)(int)(uint)param_1 * 0x168 +
                                       *(longlong *)_refptr_g_touchesPtr + 0x156),
                            (int)*(short *)((longlong)(int)(uint)param_1 * 0x168 +
                                            *(longlong *)_refptr_g_touchesPtr + 0x1a0));
        }
      }
    }
    else {
      *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208) =
           *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x208) | 0x40;
      if (*_refptr_g_tsaLogLoglevel != '\0') {
        StrPrintf_Process(0,"\nStylusAFT: Id=%d, age : %d, ellipseRate: %d (3debounce Suppressed)\n"
                          ,param_1,*(undefined2 *)
                                    ((longlong)(int)(uint)param_1 * 0x168 +
                                     *(longlong *)_refptr_g_touchesPtr + 0x156),
                          (int)*(short *)((longlong)(int)(uint)param_1 * 0x168 +
                                          *(longlong *)_refptr_g_touchesPtr + 0x1a0));
      }
    }
  }
  else {
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208)
         | 0x40;
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"\nStylusAFT: Id=%d, age : %d, ellipseRate: %d (3debounce Suppressed)\n",
                        param_1,*(undefined2 *)
                                 ((longlong)(int)(uint)param_1 * 0x168 +
                                  *(longlong *)_refptr_g_touchesPtr + 0x156),
                        (int)*(short *)((longlong)(int)(uint)param_1 * 0x168 +
                                        *(longlong *)_refptr_g_touchesPtr + 0x1a0));
    }
  }
  return;
}


// ============================================================================
// Function: StylusAft_GetPalmLevel @ 6bb209fb
// ============================================================================

undefined8 StylusAft_GetPalmLevel(byte param_1)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0xe09fb  1441  StylusAft_GetPalmLevel */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x1000000) != 0) {
    cVar1 = StylusAft_BtStatusPenAttach();
    if (((cVar1 == '\0') && (cVar1 = StylusAft_BtStatusPenHibernate(), cVar1 == '\0')) &&
       (0x14 < *(ushort *)
                ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x228)))
    {
      return 3;
    }
    if (0x1e < *(ushort *)
                ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x228))
    {
      return 3;
    }
  }
  if (((*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags & 0x20) == 0) {
    if (*(ushort *)
         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x228) < 0x27)
    {
      uVar2 = 0;
    }
    else {
      uVar2 = 3;
    }
  }
  else if (((int)((int)*(short *)((longlong)(int)(uint)param_1 * 0x168 +
                                  *(longlong *)_refptr_g_touchesPtr + 0x1a0) +
                 (uint)*(ushort *)
                        ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x228)) < 0x29) ||
          ((*(ushort *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x228) <
            0x1a && (*(short *)((longlong)(int)(uint)param_1 * 0x168 +
                                *(longlong *)_refptr_g_touchesPtr + 0x1a0) < 0x1a)))) {
    if ((*(ushort *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x228) < 0x17)
       || (*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1a0) < 0x13)) {
      if ((*(ushort *)
            ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x228) <
           0x1a) &&
         (*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1a0) < 0x10)) {
        uVar2 = 0;
      }
      else {
        uVar2 = 1;
      }
    }
    else {
      uVar2 = 2;
    }
  }
  else {
    uVar2 = 3;
  }
  return uVar2;
}


// ============================================================================
// Function: StylusAft_PalmJudge @ 6bb20eb3
// ============================================================================

undefined8 StylusAft_PalmJudge(undefined1 param_1)

{
  byte bVar1;
  undefined1 uVar2;
  ushort uVar3;
  undefined8 uVar4;
  
                    /* 0xe0eb3  1445  StylusAft_PalmJudge */
  bVar1 = StylusAft_GetPalmLevel(param_1);
  if (bVar1 < 3) {
    uVar4 = 0;
  }
  else {
    g_lastPalmTimeStamp = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    uVar3 = TSAOut_DisplayXPos(param_1);
    g_lastPalmX = (uint)uVar3;
    uVar3 = TSAOut_DisplayYPos(param_1);
    g_lastPalmY = (uint)uVar3;
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      uVar2 = StylusAft_GetPalmLevel(param_1);
      StrPrintf_Process(0,"\nStylusAFT: Id=%d, PalmLevel:%d, (PalmJudge Suppressed)\n",param_1,uVar2
                       );
    }
    uVar4 = 1;
  }
  return uVar4;
}


// ============================================================================
// Function: StylusAft_FanAreaJudge @ 6bb20f58
// ============================================================================

undefined8 StylusAft_FanAreaJudge(byte param_1)

{
  char cVar1;
  undefined1 uVar2;
  undefined2 uVar3;
  undefined2 uVar4;
  
                    /* 0xe0f58  1440  StylusAft_FanAreaJudge */
  if ((*(ushort *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x24e)
       < 0x97) &&
     ((ulonglong)((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - g_lastStylusTimeStamp) <
      800)) {
    uVar3 = TSAOut_DisplayYPos(param_1);
    uVar4 = TSAOut_DisplayXPos(param_1);
    cVar1 = StylusAft_InFanArea(uVar4,uVar3);
    if ((cVar1 != '\0') && (cVar1 = StylusAft_GetPalmLevel(param_1), cVar1 != '\0')) {
      if (*_refptr_g_tsaLogLoglevel != '\0') {
        uVar2 = StylusAft_GetPalmLevel(param_1);
        StrPrintf_Process(0,"\nStylusAFT: Id=%d, PalmLevel:%d, (FanAreaJudge Suppressed)\n",param_1,
                          uVar2);
      }
      return 1;
    }
  }
  return 0;
}


// ============================================================================
// Function: StylusAft_CurrentDomainJudge @ 6bb21047
// ============================================================================

undefined8 StylusAft_CurrentDomainJudge(byte param_1)

{
  char cVar1;
  byte bVar2;
  undefined1 uVar3;
  undefined2 uVar4;
  undefined2 uVar5;
  undefined2 uVar6;
  undefined2 uVar7;
  int iVar8;
  uint uVar9;
  uint uVar10;
  byte local_29;
  
                    /* 0xe1047  1436  StylusAft_CurrentDomainJudge */
  if (*(ushort *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x24e)
      < 0x97) {
    for (local_29 = 0; local_29 < **(byte **)_refptr_g_touchesPtr; local_29 = local_29 + 1) {
      if (local_29 != param_1) {
        uVar4 = TSAOut_DisplayYPos(local_29);
        uVar5 = TSAOut_DisplayXPos(local_29);
        uVar6 = TSAOut_DisplayYPos(param_1);
        uVar7 = TSAOut_DisplayXPos(param_1);
        iVar8 = StylusAft_DistBetweenPoints(uVar7,uVar6,uVar5,uVar4);
        if ((iVar8 <= (int)((uint)*(ushort *)
                                   ((longlong)(int)(uint)local_29 * 0x168 +
                                    *(longlong *)_refptr_g_touchesPtr + 0x228) * 0x10)) &&
           (cVar1 = StylusAft_PalmLocationJudge(local_29,200), cVar1 != '\0')) {
          bVar2 = StylusAft_GetPalmLevel(local_29);
          if (2 < bVar2) {
            if (*_refptr_g_tsaLogLoglevel != '\0') {
              uVar3 = StylusAft_GetPalmLevel(param_1);
              StrPrintf_Process(0,
                                "\nStylusAFT: Id=%d, PalmLevel:%d, (CurrentDomainJudge1 Suppressed)\n"
                                ,local_29,uVar3);
            }
            return 1;
          }
          uVar9 = (uint)*(ushort *)
                         ((longlong)(int)(uint)local_29 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                         + 0x156) -
                  (uint)*(ushort *)
                         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x156);
          uVar10 = (int)uVar9 >> 0x1f;
          if (100 < (int)((uVar9 ^ uVar10) - uVar10)) {
            return 0;
          }
          if ((*(char *)((longlong)(int)(uint)local_29 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x287) != '\x14') &&
             (*(char *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)
                       (int)(uint)*(byte *)((longlong)(int)(uint)local_29 * 0x168 +
                                            *(longlong *)_refptr_g_touchesPtr + 0x287) * 0x478 +
                       0x58d) != '\0')) {
            if (*_refptr_g_tsaLogLoglevel != '\0') {
              uVar3 = StylusAft_GetPalmLevel(param_1);
              StrPrintf_Process(0,
                                "\nStylusAFT: Id=%d, PalmLevel:%d, (CurrentDomainJudge2 Suppressed)\n"
                                ,local_29,uVar3);
            }
            return 1;
          }
          if ((*(int *)((longlong)(int)(uint)local_29 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x210) == 1) ||
             (*(int *)((longlong)(int)(uint)local_29 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x210) == 2)) {
            if (*_refptr_g_tsaLogLoglevel != '\0') {
              uVar3 = StylusAft_GetPalmLevel(param_1);
              StrPrintf_Process(0,
                                "\nStylusAFT: Id=%d, PalmLevel:%d, (CurrentDomainJudge3 grip Suppressed)\n"
                                ,local_29,uVar3);
            }
            return 1;
          }
        }
      }
    }
  }
  return 0;
}


// ============================================================================
// Function: StylusAft_EarlyPalmJudge @ 6bb2145c
// ============================================================================

undefined8 StylusAft_EarlyPalmJudge(byte param_1)

{
  byte bVar1;
  undefined1 uVar2;
  
                    /* 0xe145c  1438  StylusAft_EarlyPalmJudge */
  if ((*(ushort *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x156)
       < 0x18) && (bVar1 = StylusAft_GetPalmLevel(param_1), 1 < bVar1)) {
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      uVar2 = StylusAft_GetPalmLevel(param_1);
      StrPrintf_Process(0,"\nStylusAFT: Id=%d, PalmLevel:%d, (EarlyPalmJudge Suppressed)\n",param_1,
                        uVar2);
    }
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: StylusAft_EdgePalmJudge @ 6bb214eb
// ============================================================================

undefined8 StylusAft_EdgePalmJudge(byte param_1)

{
  byte bVar1;
  undefined1 uVar2;
  char cVar3;
  
                    /* 0xe14eb  1439  StylusAft_EdgePalmJudge */
  if (0x96 < *(ushort *)
              ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x24e)) {
    return 0;
  }
  if (((*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags & 0x20) != 0) {
    if ((*(ushort *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22c) < 700)
       || (*(ushort *)
            ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22e) < 700
          )) {
      bVar1 = StylusAft_GetPalmLevel(param_1);
      if (1 < bVar1) {
        if (*_refptr_g_tsaLogLoglevel != '\0') {
          uVar2 = StylusAft_GetPalmLevel(param_1);
          StrPrintf_Process(0,"\nStylusAFT: Id=%d, PalmLevel:%d, (EdgePalmJudge1 Suppressed)\n",
                            param_1,uVar2);
        }
        return 1;
      }
      cVar3 = StylusAft_GetPalmLevel(param_1);
      if ((cVar3 != '\0') &&
         (0x18 < *(ushort *)
                  ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x156)
         )) {
        if (*_refptr_g_tsaLogLoglevel != '\0') {
          uVar2 = StylusAft_GetPalmLevel(param_1);
          StrPrintf_Process(0,"\nStylusAFT: Id=%d, PalmLevel:%d, (EdgePalmJudge2 Suppressed)\n",
                            param_1,uVar2);
        }
        return 1;
      }
    }
    else if (0x13 < *(short *)((longlong)(int)(uint)param_1 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x1a0)) {
      if (*_refptr_g_tsaLogLoglevel != '\0') {
        uVar2 = StylusAft_GetPalmLevel(param_1);
        StrPrintf_Process(0,"\nStylusAFT: Id=%d, PalmLevel:%d, (EdgePalmJudge3 Suppressed)\n",
                          param_1,uVar2);
      }
      return 1;
    }
  }
  return 0;
}


// ============================================================================
// Function: StylusAft_PostDomainJudge @ 6bb21364
// ============================================================================

undefined8 StylusAft_PostDomainJudge(byte param_1)

{
  ushort uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined1 uVar4;
  undefined2 uVar5;
  undefined2 uVar6;
  int iVar7;
  
  uVar3 = g_lastPalmY;
                    /* 0xe1364  1447  StylusAft_PostDomainJudge */
  uVar2 = g_lastPalmX;
  if ((ulonglong)((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - g_lastPalmTimeStamp) < 200)
  {
    uVar5 = TSAOut_DisplayYPos(param_1);
    uVar6 = TSAOut_DisplayXPos(param_1);
    iVar7 = StylusAft_DistBetweenPoints(uVar6,uVar5,uVar2,uVar3);
    uVar1 = *(ushort *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x228);
    if (iVar7 < (int)((uint)uVar1 * 4 + (uint)uVar1)) {
      if (*_refptr_g_tsaLogLoglevel != '\0') {
        uVar4 = StylusAft_GetPalmLevel(param_1);
        StrPrintf_Process(0,"\nStylusAFT: Id=%d, PalmLevel:%d, (PostDomainJudge Suppressed)\n",
                          param_1,uVar4);
      }
      return 1;
    }
  }
  return 0;
}


// ============================================================================
// Function: StylusAft_InFanArea @ 6bb20d9b
// ============================================================================

undefined8 StylusAft_InFanArea(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  uint uVar6;
  uint local_10;
  uint local_c;
  
  iVar4 = g_lastTiltY;
                    /* 0xe0d9b  1443  StylusAft_InFanArea */
  iVar3 = g_lastTiltX;
  iVar5 = g_lastTiltX * 6;
  iVar1 = g_lastTiltY * 6;
  iVar2 = StylusAft_Sqrt(iVar1 * iVar1 + iVar5 * iVar5);
  local_c = iVar2 * 7;
  if (local_c < 800) {
    local_c = 800;
  }
  iVar3 = g_lastStylusX + iVar3 * 0x1e;
  iVar4 = g_lastStylusY + iVar4 * 0x1e;
  iVar5 = StylusAft_Sqrt(iVar1 * iVar1 + iVar5 * iVar5);
  local_10 = iVar5 * 5;
  if (local_10 < 0x236) {
    local_10 = 0x236;
  }
  uVar6 = StylusAft_DistBetweenPoints(param_1,param_2,g_lastStylusX,g_lastStylusY);
  if ((uVar6 < local_c) &&
     (uVar6 = StylusAft_DistBetweenPoints(param_1,param_2,iVar3,iVar4), uVar6 < local_10)) {
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: StylusAft_DistBetweenPoints @ 6bb20d54
// ============================================================================

int StylusAft_DistBetweenPoints(int param_1,int param_2,int param_3,int param_4)

{
                    /* 0xe0d54  1437  StylusAft_DistBetweenPoints */
  return StylusAft_Sqrt((param_4 - param_2) * (param_4 - param_2) +
                 (param_3 - param_1) * (param_3 - param_1));
}


// ============================================================================
// Function: StylusAft_Sqrt @ 6bb20d02
// ============================================================================

int StylusAft_Sqrt(uint param_1)

{
  int local_10;
  uint local_c;
  
                    /* 0xe0d02  1451  StylusAft_Sqrt */
  local_10 = 0;
  for (local_c = 0x8000; param_1 < local_c; local_c = local_c >> 1) {
  }
  for (; local_c != 0; local_c = local_c >> 1) {
    local_10 = local_10 + local_c;
    if (param_1 < (uint)(local_10 * local_10)) {
      local_10 = local_10 - local_c;
    }
  }
  return local_10;
}


// ============================================================================
// Function: StylusAft_Init @ 6bb208d0
// ============================================================================

void StylusAft_Init(void)

{
                    /* 0xe08d0  1444  StylusAft_Init */
  g_lastStylusTimeStamp = 0;
  g_lastStylusX = 0;
  g_lastStylusY = 0;
  g_lastTiltX = 0;
  g_lastTiltY = 0;
  g_lastPalmTimeStamp = 0;
  g_lastPalmX = 0;
  g_lastPalmY = 0;
  *_refptr_g_btPenStatus = 0;
  return;
}


