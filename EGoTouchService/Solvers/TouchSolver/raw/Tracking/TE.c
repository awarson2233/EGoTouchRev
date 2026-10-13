#include "TSACoreInternal.h"

// ============================================================================
// Function: TE_Process @ 6ba789d9
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* Touch event stage. Current config has WindowsPad enabled, so lift-off events use
   TE_WindowsLiftOffProcess when a previous touch has no current match. */

void TE_Process(void)

{
  byte bPrevTouchIndex;
  char cMappedTouchIndex;
  byte bPrevTouchIndexIter;
  byte bCurTouchIndex;
  byte bCurTouchCount;
  
                    /* 0x389d9  1461  TE_Process */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&DAT_6bb529f6,"TE_Process");
  }
  bCurTouchCount = **(byte **)_refptr_g_touchesPtr;
  for (bPrevTouchIndexIter = 0; bPrevTouchIndexIter < (*_refptr_g_prevTouchesPtr)->bPrevTouchCount;
      bPrevTouchIndexIter = bPrevTouchIndexIter + 1) {
    cMappedTouchIndex = Touch_GetIdxBasedOnPreIdx(bPrevTouchIndexIter);
    if (cMappedTouchIndex == '\x14') {
      cMappedTouchIndex = TSA_IsWindowsPadFeatureEnabled();
      if (cMappedTouchIndex == '\0') {
        TE_LiftOffProcess(bPrevTouchIndexIter);
      }
      else {
        TE_WindowsLiftOffProcess(bPrevTouchIndexIter);
      }
    }
  }
  for (bCurTouchIndex = 0; bCurTouchIndex < bCurTouchCount; bCurTouchIndex = bCurTouchIndex + 1) {
    if (*(char *)((longlong)(int)(uint)bCurTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                 0x154) == '\0') {
      TE_TouchDownProcess(bCurTouchIndex);
    }
    else {
      bPrevTouchIndex = PrevTouch_GetIdxBasedOnCurIdx(bCurTouchIndex);
      if ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)bPrevTouchIndex * 0x478 + 0x21c) & 1) == 0) {
        TE_MoveProcess(bCurTouchIndex,bPrevTouchIndex);
      }
      else {
        TE_TouchDownDebounce(bCurTouchIndex,bPrevTouchIndex);
      }
    }
  }
  cMappedTouchIndex = TE_AssertProcess();
  if (cMappedTouchIndex != '\0') {
    Touch_Clean();
    PrevTouch_Release();
  }
  if (((*_refptr_g_tsaStaticPtr)->dwProcessingState & PROC_MS_ACTIVE) != 0) {
    Touch_ProcessAftTouchEvent();
    TSAStatic_ProcessAftTouchEvent();
  }
  return;
}


// ============================================================================
// Function: TE_MoveProcess @ 6ba775fa
// ============================================================================

void TE_MoveProcess(byte param_1,byte param_2)

{
  Touch_UnitUpdate(param_1,param_2);
  *(undefined2 *)
   ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4e4) = 0;
  *(undefined4 *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c)
       = 4;
  *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x156) =
       *(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x156
                 ) + 1;
  *(int *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x158) =
       (int)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
       (int)*(undefined8 *)
             ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4b8);
  PrevTouch_UnitUpdate(param_2,param_1);
  return;
}


// ============================================================================
// Function: TE_TouchDownDebounce @ 6ba7772b
// ============================================================================

void TE_TouchDownDebounce(byte param_1,byte param_2)

{
  ushort uVar1;
  
  *(undefined2 *)
   ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4b0) =
       *(undefined2 *)
        ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162);
  *(undefined2 *)
   ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4b2) =
       *(undefined2 *)
        ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164);
  *(void **)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4b8) =
       (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  Touch_UnitUpdate(param_1,param_2);
  uVar1 = DB_TouchDownNum(param_2,param_1);
  if (*(ushort *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4e4) < uVar1)
  {
    *(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4e4) =
         *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_2 * 0x478 + 0x4e4) + 1;
    *(undefined4 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c) = 1;
  }
  else {
    *(undefined4 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c) = 2;
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1fc) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1fc)
         | 0x400;
  }
  *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x156) =
       *(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x156
                 ) + 1;
  *(int *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x158) =
       (int)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
       (int)*(undefined8 *)
             ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4b8);
  PrevTouch_UnitUpdate(param_2,param_1);
  return;
}


// ============================================================================
// Function: TE_TouchDownProcess @ 6ba779f3
// ============================================================================

void TE_TouchDownProcess(byte param_1)

{
  byte bVar1;
  char cVar2;
  
  cVar2 = TE_GetNextID();
  if ((cVar2 != '\0') &&
     (*(char *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x154) =
           cVar2, (*_refptr_g_prevTouchesPtr)->bPrevTouchCount < 0x14)) {
    bVar1 = (*_refptr_g_prevTouchesPtr)->bPrevTouchCount;
    PrevTouch_UnitCopy(bVar1,param_1);
    *(undefined2 *)
     ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x496) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162);
    *(undefined2 *)
     ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x498) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164);
    *(undefined4 *)
     ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x4a0) =
         *(undefined4 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1c4);
    *(undefined2 *)
     ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x514) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1a8);
    *(undefined2 *)
     ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x516) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1aa);
    *(undefined2 *)
     ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x1a2) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1a0);
    *(byte *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x4f2) =
         param_1;
    *(undefined2 *)
     ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x156) = 0;
    *(void **)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x4c0) =
         (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    *(undefined1 *)
     ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x58c) = 0;
    *(undefined1 *)
     ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x58d) = 0;
    *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x287) =
         bVar1;
    TE_TouchDownDebounce(param_1,bVar1);
    (*_refptr_g_prevTouchesPtr)->bPrevTouchCount =
         (*_refptr_g_prevTouchesPtr)->bPrevTouchCount + '\x01';
  }
  return;
}


// ============================================================================
// Function: TE_WindowsLiftOffProcess @ 6ba78033
// ============================================================================

void TE_WindowsLiftOffProcess(byte param_1)

{
  char cVar1;
  short sVar2;
  byte local_9;
  
  local_9 = 0x14;
  if ((*(int *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x21c)
       != 0x40) &&
     (*(int *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x21c)
      != 8)) {
    *(undefined2 *)
     ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x4e4) = 0;
  }
  sVar2 = DB_LiftOffNum(param_1);
  if ((int)(uint)*(ushort *)
                  ((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_1 * 0x478 + 0x4e4) < (int)((ushort)(sVar2 + 1U) - 1)) {
    *(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x4e4) =
         *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_1 * 0x478 + 0x4e4) + 1;
    if (**(byte **)_refptr_g_touchesPtr < 0x14) {
      local_9 = **(byte **)_refptr_g_touchesPtr;
      Touch_UnitCopy(local_9,param_1);
      *(byte *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x287) =
           param_1;
      *(undefined4 *)
       ((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c) = 8;
      *(short *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x156) =
           *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)param_1 * 0x478 + 0x156) + 1;
      *(byte *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x4f2)
           = local_9;
      **(char **)_refptr_g_touchesPtr = **(char **)_refptr_g_touchesPtr + '\x01';
    }
  }
  else if (((uint)*(ushort *)
                   ((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_1 * 0x478 + 0x4e4) == (ushort)(sVar2 + 1U) - 1) ||
          (*(int *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_1 * 0x478 + 0x21c) == 0x10)) {
    *(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x4e4) =
         *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_1 * 0x478 + 0x4e4) + 1;
    if (**(byte **)_refptr_g_touchesPtr < 0x14) {
      local_9 = **(byte **)_refptr_g_touchesPtr;
      Touch_UnitCopy(local_9,param_1);
      if ((((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_1 * 0x478 + 0x1f8) & 0x40000) == 0) ||
          (*(int *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_1 * 0x478 + 0x21c) != 1)) ||
         (cVar1 = Touch_IsException(local_9,param_1), cVar1 != '\0')) {
        *(undefined4 *)
         ((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c) = 0x40;
      }
      else {
        *(undefined4 *)
         ((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c) = 0x10;
        *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_1 * 0x478 + 0x4e4) =
             *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_1 * 0x478 + 0x4e4) + -1;
      }
      *(byte *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x287) =
           param_1;
      *(short *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x156) =
           *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)param_1 * 0x478 + 0x156) + 1;
      *(byte *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x4f2)
           = local_9;
      **(char **)_refptr_g_touchesPtr = **(char **)_refptr_g_touchesPtr + '\x01';
    }
  }
  else {
    local_9 = TE_LiftOffReportProcess(param_1);
  }
  if (local_9 != 0x14) {
    PrevTouch_UnitUpdate(param_1,local_9);
  }
  return;
}


// ============================================================================
// Function: TE_LiftOffProcess @ 6ba78511
// ============================================================================

void TE_LiftOffProcess(byte param_1)

{
  ushort uVar1;
  byte local_9;
  
  local_9 = 0x14;
  if (*(int *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x21c)
      != 8) {
    *(undefined2 *)
     ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x4e4) = 0;
  }
  uVar1 = DB_LiftOffNum(param_1);
  if (*(ushort *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x4e4) < uVar1)
  {
    *(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x4e4) =
         *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_1 * 0x478 + 0x4e4) + 1;
    if (**(byte **)_refptr_g_touchesPtr < 0x14) {
      local_9 = **(byte **)_refptr_g_touchesPtr;
      Touch_UnitCopy(local_9,param_1);
      *(byte *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x287) =
           param_1;
      *(undefined4 *)
       ((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c) = 8;
      *(short *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x156) =
           *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)param_1 * 0x478 + 0x156) + 1;
      *(byte *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x4f2)
           = local_9;
      **(char **)_refptr_g_touchesPtr = **(char **)_refptr_g_touchesPtr + '\x01';
    }
  }
  else {
    local_9 = TE_LiftOffReportProcess(param_1);
  }
  if (local_9 != 0x14) {
    PrevTouch_UnitUpdate(param_1,local_9);
  }
  return;
}


// ============================================================================
// Function: TE_AssertProcess @ 6ba7872d
// ============================================================================

undefined1 TE_AssertProcess(void)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x28]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x30];
  char cVar1;
  
  (*(undefined1 *)(_stk + 0x25)) = 0;
  if ((*_refptr_g_prevTouchesPtr)->bPrevTouchCount != **(uchar **)_refptr_g_touchesPtr) {
    cVar1 = TSALog_RtEnable(1,1,1);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\nTouch/PrevTouch number not match...\n");
    }
    (*(undefined1 *)(_stk + 0x25)) = 1;
  }
  (*(int *)(_stk + 0x20)) = memset_s(((char *)(_stk + 0x8)),0x14,0,0x14);
  if ((*(int *)(_stk + 0x20)) != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/TouchEvent/TE_Process.c",0x131,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  for ((*(byte *)(_stk + 0x26)) = 0; (*(byte *)(_stk + 0x26)) < (*_refptr_g_prevTouchesPtr)->bPrevTouchCount; (*(byte *)(_stk + 0x26)) = (*(byte *)(_stk + 0x26)) + 1) {
    (*(byte *)(_stk + 0x27)) = Touch_GetIdxBasedOnPreIdx((*(byte *)(_stk + 0x26)));
    if ((*(byte *)(_stk + 0x27)) < 0x14) {
      ((char *)(_stk + 0x8))[(int)(uint)(*(byte *)(_stk + 0x27))] = '\x01';
    }
    else {
      cVar1 = TSALog_RtEnable(1,1,1);
      if (cVar1 != '\0') {
        StrPrintf_Process(0,"\nPrevTouch buffer overflow...\n");
      }
      (*(undefined1 *)(_stk + 0x25)) = 1;
    }
  }
  for ((*(byte *)(_stk + 0x24)) = 0; (*(byte *)(_stk + 0x24)) < (*_refptr_g_prevTouchesPtr)->bPrevTouchCount; (*(byte *)(_stk + 0x24)) = (*(byte *)(_stk + 0x24)) + 1) {
    if (((char *)(_stk + 0x8))[(int)(uint)(*(byte *)(_stk + 0x24))] != '\x01') {
      cVar1 = TSALog_RtEnable(1,1,1);
      if (cVar1 != '\0') {
        StrPrintf_Process(0,"\npreIdx exception...\n");
      }
      (*(undefined1 *)(_stk + 0x25)) = 1;
    }
  }
  (*(int *)(_stk + 0x20)) = memset_s(((char *)(_stk + 0x8)),0x14,0,0x14);
  if ((*(int *)(_stk + 0x20)) != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/TouchEvent/TE_Process.c",0x144,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  for ((*(byte *)(_stk + 0x27)) = 0; (*(byte *)(_stk + 0x27)) < **(byte **)_refptr_g_touchesPtr; (*(byte *)(_stk + 0x27)) = (*(byte *)(_stk + 0x27)) + 1) {
    (*(byte *)(_stk + 0x26)) = PrevTouch_GetIdxBasedOnCurIdx((*(byte *)(_stk + 0x27)));
    if ((*(byte *)(_stk + 0x26)) < 0x14) {
      ((char *)(_stk + 0x8))[(int)(uint)(*(byte *)(_stk + 0x26))] = '\x01';
    }
    else {
      cVar1 = TSALog_RtEnable(1,1,1);
      if (cVar1 != '\0') {
        StrPrintf_Process(0,"\nTouch buffer overflow...\n");
      }
      (*(undefined1 *)(_stk + 0x25)) = 1;
    }
  }
  for ((*(byte *)(_stk + 0x24)) = 0; (*(byte *)(_stk + 0x24)) < **(byte **)_refptr_g_touchesPtr; (*(byte *)(_stk + 0x24)) = (*(byte *)(_stk + 0x24)) + 1) {
    if (((char *)(_stk + 0x8))[(int)(uint)(*(byte *)(_stk + 0x24))] != '\x01') {
      cVar1 = TSALog_RtEnable(1,1,1);
      if (cVar1 != '\0') {
        StrPrintf_Process(0,"\ncurIdx exception...\n");
      }
      (*(undefined1 *)(_stk + 0x25)) = 1;
    }
  }
  return (*(undefined1 *)(_stk + 0x25));
}


// ============================================================================
// Function: TE_GetNextID @ 6ba77536
// ============================================================================

byte TE_GetNextID(void)

{
  char cVar1;
  bool bVar2;
  byte local_c;
  byte local_b;
  char local_9;
  
  local_c = 1;
  do {
    if (0x14 < local_c) {
      return 0;
    }
    local_b = local_c + prevOccupiedID_10025;
    if (0x14 < local_b) {
      local_b = local_b - 0x13;
    }
    cVar1 = PrevTouch_IsValidId(local_b);
    bVar2 = cVar1 == '\0';
    for (local_9 = '\0'; (int)local_9 < (int)(uint)**(byte **)_refptr_g_touchesPtr;
        local_9 = local_9 + '\x01') {
      if (*(byte *)((longlong)(int)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x154) ==
          local_b) {
        bVar2 = true;
        break;
      }
    }
    if (!bVar2) {
      return local_b;
    }
    local_c = local_c + 1;
  } while( true );
}


// ============================================================================
// Function: TE_LiftOffReportProcess @ 6ba77d4b
// ============================================================================

byte TE_LiftOffReportProcess(byte param_1)

{
  byte bVar1;
  char cVar2;
  
  if (**(byte **)_refptr_g_touchesPtr < 0x14) {
    bVar1 = **(byte **)_refptr_g_touchesPtr;
    Touch_UnitCopy(bVar1,param_1);
    if ((((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_1 * 0x478 + 0x1f8) & 0x40000) == 0) ||
        (*(int *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x21c
                 ) != 1)) || (cVar2 = Touch_IsException(bVar1,param_1), cVar2 != '\0')) {
      *(undefined4 *)
       ((longlong)(int)(uint)bVar1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c) = 0x20;
    }
    else {
      *(undefined4 *)
       ((longlong)(int)(uint)bVar1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c) = 0x10;
    }
    cVar2 = TSA_IsWindowsPadFeatureEnabled();
    if (cVar2 != '\0') {
      *(undefined4 *)
       ((longlong)(int)(uint)bVar1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c) = 0x20;
    }
    *(byte *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x4f2) =
         bVar1;
    *(byte *)((longlong)(int)(uint)bVar1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x287) =
         param_1;
    **(char **)_refptr_g_touchesPtr = **(char **)_refptr_g_touchesPtr + '\x01';
    if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0xc00000) == 0) {
      if ((((short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax * 2 <
            (int)*(short *)((longlong)(int)(uint)bVar1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                           0x1be)) &&
          ((*(uint *)((longlong)(int)(uint)bVar1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8
                     ) & 0x80) == 0)) &&
         (*(ushort *)
           ((longlong)(int)(uint)bVar1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x228) < 0x15))
      {
        if ((*_refptr_g_tsaStaticPtr)->wField_0x122 < 5) {
          (*_refptr_g_tsaStaticPtr)->wField_0x122 = (*_refptr_g_tsaStaticPtr)->wField_0x122 + 1;
        }
      }
      else if ((*_refptr_g_tsaStaticPtr)->wField_0x122 < 5) {
        (*_refptr_g_tsaStaticPtr)->wField_0x122 = 0;
      }
    }
    else if ((*_refptr_g_tsaStaticPtr)->wField_0x122 < 5) {
      (*_refptr_g_tsaStaticPtr)->wField_0x122 = 0;
    }
  }
  else {
    bVar1 = 0x14;
  }
  return bVar1;
}


// ============================================================================
// Function: TE_Init @ 6ba77530
// ============================================================================

void TE_Init(void)

{
                    /* 0x37530  1460  TE_Init */
  return;
}


