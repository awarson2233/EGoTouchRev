#include "TSACoreInternal.h"

// ============================================================================
// Function: RxLineFilter_Process @ 6ba60827
// ============================================================================

void RxLineFilter_Process(void)

{
  undefined1 uVar1;
  char cVar2;
  char local_9;
  
                    /* 0x20827  1174  RxLineFilter_Process */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&_rdata,"RxLineFilter_Process");
  }
  if (1 < (*_refptr_g_tsaStaticPtr)->bField_0x111) {
    uVar1 = _refptr_g_tsaPrmtFlash[1].field_0x64;
    local_9 = **(char **)_refptr_g_touchesPtr;
    while (local_9 = local_9 + -1, -1 < local_9) {
      cVar2 = RxLineFilter_IsPossibleFalseTouch(local_9,uVar1);
      if (cVar2 != '\0') {
        Touch_UnitRemove(local_9);
      }
    }
  }
  return;
}


// ============================================================================
// Function: RxLineFilter_Init @ 6ba60440
// ============================================================================

void RxLineFilter_Init(void)

{
                    /* 0x20440  1173  RxLineFilter_Init */
  return;
}


// ============================================================================
// Function: RxLineFilter_IsPossibleFalseTouch @ 6ba6069c
// ============================================================================

undefined1 RxLineFilter_IsPossibleFalseTouch(byte param_1,undefined1 param_2)

{
  char cVar1;
  byte bVar2;
  undefined1 local_9;
  
  local_9 = 0;
  cVar1 = RxLineFilter_IsZ8Failed(param_1,param_2);
  if (cVar1 == '\0') {
    local_9 = 0;
  }
  else {
    cVar1 = SS_TouchValid(param_1);
    if ((cVar1 == '\0') && ((*_refptr_g_tsaStaticPtr)->bField_0x111 == '\x03')) {
      local_9 = 1;
    }
    if (*(char *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x154)
        == '\0') {
      local_9 = 1;
    }
    else {
      bVar2 = PrevTouch_GetIdxBasedOnID
                        (*(undefined1 *)
                          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x154));
      if (((bVar2 != 0x14) &&
          ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)bVar2 * 0x478 + 0x1f8) & 8) != 0)) &&
         ((int)*(short *)(*_refptr_g_peaksPtr +
                         (longlong)
                         (int)(uint)*(byte *)((longlong)(int)(uint)param_1 * 0x168 +
                                              *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 +
                         0x18) - (int)(*(short *)(_refptr_g_tsaPrmtDynamic + 0xc) / 2) <
          (int)*(short *)&(*_refptr_g_tsaStaticPtr)[1].field_0x38)) {
        local_9 = 1;
      }
    }
  }
  return local_9;
}


// ============================================================================
// Function: RxLineFilter_IsZ8Failed @ 6ba60446
// ============================================================================

undefined8 RxLineFilter_IsZ8Failed(byte param_1,byte param_2)

{
  byte bVar1;
  byte bVar2;
  undefined8 uVar3;
  uint uVar4;
  uchar local_17;
  byte local_16;
  uchar local_15;
  int local_14;
  int local_10;
  byte local_9;
  
  bVar1 = *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x23c
                   );
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    local_17 = (*_refptr_g_peaksPtr)[(longlong)(int)(uint)bVar1 * 0x14 + 0x16].bPeakCount;
  }
  else {
    local_17 = (*_refptr_g_peaksPtr)[(longlong)(int)(uint)bVar1 * 0x14 + 0x15].bPeakCount;
  }
  local_14 = *(int *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)bVar1 * 0x14 + 0x1c);
  local_10 = local_14;
  local_9 = bVar1;
  for (local_16 = 0; local_16 < **(byte **)_refptr_g_touchesPtr; local_16 = local_16 + 1) {
    if (local_16 != param_1) {
      bVar2 = *(byte *)((longlong)(int)(uint)local_16 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x23c);
      if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
        local_15 = (*_refptr_g_peaksPtr)[(longlong)(int)(uint)bVar2 * 0x14 + 0x16].bPeakCount;
      }
      else {
        local_15 = (*_refptr_g_peaksPtr)[(longlong)(int)(uint)bVar2 * 0x14 + 0x15].bPeakCount;
      }
      uVar4 = (int)(char)local_17 - (int)(char)local_15 >> 0x1f;
      if (((int)(((int)(char)local_17 - (int)(char)local_15 ^ uVar4) - uVar4) <
           (int)(uint)(byte)_refptr_g_tsaPrmtFlash[1].field_0x65) &&
         (local_10 < *(int *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)bVar2 * 0x14 + 0x1c))) {
        local_10 = *(int *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)bVar2 * 0x14 + 0x1c);
        local_9 = bVar2;
      }
    }
  }
  if (local_9 == bVar1) {
    uVar3 = 0;
  }
  else {
    if (local_14 < 0) {
      local_14 = 0;
    }
    if ((uint)(local_14 << 7) < local_10 * (uint)param_2) {
      uVar3 = 1;
    }
    else {
      uVar3 = 0;
    }
  }
  return uVar3;
}


