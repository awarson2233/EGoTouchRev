#include "TSACoreInternal.h"

// ============================================================================
// Function: TZ_Process @ 6ba8c9f0
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* Converts current peaks into touch zones and optional age processing when grip flag 0x200 is set.
    */

void TZ_Process(void)

{
                    /* 0x4c9f0  2439  TZ_Process */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&_rdata,"TZ_Process");
  }
  if ((_refptr_g_tsaPrmtFlash->dwGripFlags & 0x200) != 0) {
    TZ_UpdatePeakTzAge();
  }
  **(undefined1 **)_refptr_g_touchesPtr = 0;
  TZ_PeakBasedProcess();
  if ((_refptr_g_tsaPrmtFlash->dwGripFlags & 0x200) != 0) {
    TZ_AgeProcess();
  }
  return;
}


// ============================================================================
// Function: TZ_PeakBasedProcess @ 6ba8bffd
// ============================================================================

void TZ_PeakBasedProcess(void)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x388]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x390];
  
                    /* 0x4bffd  2432  TZ_PeakBasedProcess */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&_rdata,"TZ_PeakBasedProcess");
  }
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x100000) != 0) {
    TZ_SSLoProcess();
  }
  memset(((undefined1 *)(_stk + 0x8)),0,0x370);
  (*(int *)(_stk + 0x380)) = memset_s(*(undefined8 *)_refptr_g_tsaBufTz,0x1324,0,
                      _refptr_g_tsaPrmtFlash->wBufElementCount);
  if ((*(int *)(_stk + 0x380)) != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/TouchZone/TZ_StateMachine.c",0x21f,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  if (((*_refptr_g_tsaStaticPtr)->dwProcessingState & PROC_MS_ACTIVE) != 0) {
    (*(int *)(_stk + 0x380)) = memset_s(*(undefined8 *)_refptr_g_tsaBufSubTZ,0x1324,0,
                        _refptr_g_tsaPrmtFlash->wBufElementCount);
    if ((*(int *)(_stk + 0x380)) != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Alg/TouchZone/TZ_StateMachine.c",0x222,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
    (*(int *)(_stk + 0x380)) = memset_s(*(undefined8 *)_refptr_g_tsaBufTzZone,*(undefined2 *)_refptr_g_gridBufSize,0
                        ,_refptr_g_tsaPrmtFlash->wBufElementCount);
    if ((*(int *)(_stk + 0x380)) != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Alg/TouchZone/TZ_StateMachine.c",0x225,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
  }
  _refptr_g_tzAccess[9] = 0x10;
  *(undefined8 *)_refptr_g_tzAccess = *(undefined8 *)_refptr_g_tsaBufTz;
  *(undefined **)_refptr_g_ptrAccess = _refptr_g_tzAccess;
  *(undefined8 *)_refptr_g_subTZAccess = *(undefined8 *)_refptr_g_tsaBufSubTZ;
  _refptr_g_subTZAccess[9] = 0x10;
  g_pkAccessedFlag = 0;
  *(undefined4 *)_refptr_g_pkAccessedFlagSubTZ = 0;
  g_pkBeforeSDCAccessedFlag = 0;
  (*(undefined4 *)(_stk + 0x37c)) = TZ_CheckFatFingerSD();
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"isFatFingerSD = %d",(*(undefined4 *)(_stk + 0x37c)));
  }
  for ((*(byte *)(_stk + 0x387)) = 0; (*(byte *)(_stk + 0x387)) < (*_refptr_g_peaksPtr)->bPeakCount; (*(byte *)(_stk + 0x387)) = (*(byte *)(_stk + 0x387)) + 1) {
    (*(ushort *)(_stk + 0x37a)) = (ushort)(*_refptr_g_peaksPtr)[(longlong)(int)(uint)(*(byte *)(_stk + 0x387)) * 0x14 + 0x15].bPeakCount
               + (ushort)(*_refptr_g_peaksPtr)[(longlong)(int)(uint)(*(byte *)(_stk + 0x387)) * 0x14 + 0x16].
                         bPeakCount * (ushort)_refptr_g_tsaPrmtFlash->bCols;
    if ((*(char *)((ulonglong)(*(ushort *)(_stk + 0x37a)) + *(longlong *)_refptr_g_tsaBufTz) == '\0') &&
       ((((*_refptr_g_tsaStaticPtr)->dwProcessingState & PROC_MS_ACTIVE) == 0 ||
        (*(char *)((ulonglong)(*(ushort *)(_stk + 0x37a)) + *(longlong *)_refptr_g_tsaBufSubTZ) == '\0')))) {
      if ((_refptr_g_tsaPrmtFlash->dwGripFlags & 0x100) == 0) {
        if ((_refptr_g_tsaPrmtFlash->dwGripFlags & 0x80) == 0) {
          TZ_NormalTraversal(((undefined1 *)(_stk + 0x8)),(*(byte *)(_stk + 0x387)),(*(undefined4 *)(_stk + 0x37c)));
        }
        else {
          TZ_CentrodToTouch(((undefined1 *)(_stk + 0x8)),(*(byte *)(_stk + 0x387)));
        }
      }
      else {
        TZ_SimpleTraversal(((undefined1 *)(_stk + 0x8)),(*(byte *)(_stk + 0x387)));
      }
    }
  }
  if ((_refptr_g_tsaPrmtFlash->dwGripFlags & 0x200) != 0) {
    TZ_UpdateGripFlag(*(undefined8 *)_refptr_g_tsaBufTz);
  }
  TZ_DilateAndErode();
  return;
}


// ============================================================================
// Function: TZ_AgeProcess @ 6ba8c428
// ============================================================================

void TZ_AgeProcess(void)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x168]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x170];
  char cVar1;
  uint uVar2;
  longlong lVar3;
  undefined8 *puVar4;
  
                    /* 0x4c428  2356  TZ_AgeProcess */
  puVar4 = ((undefined8 *)(_stk + 0x108));
  for (lVar3 = 7; lVar3 != 0; lVar3 = lVar3 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  *(undefined4 *)puVar4 = 0;
  if (*(longlong *)&_refptr_g_tsaPrpt[1].field_0xc == -1) {
    *(void **)&_refptr_g_tsaPrpt[1].field_0xc = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  }
  (*(int *)(_stk + 0x14c)) = (int)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
             (int)*(undefined8 *)&_refptr_g_tsaPrpt[1].field_0xc;
  (*(uint *)(_stk + 0x154)) = (uint)_refptr_g_tsaPrmtFlash->wBufElementCount;
  while ((*(uint *)(_stk + 0x154)) = (*(uint *)(_stk + 0x154)) - 1, -1 < (int)(*(uint *)(_stk + 0x154))) {
    uVar2 = Sensor_GetPosType2((*(uint *)(_stk + 0x154)) & 0xffff);
    if (uVar2 < 3) {
      *(undefined2 *)(*(longlong *)_refptr_g_tsaBufTzAge + (longlong)(int)(*(uint *)(_stk + 0x154)) * 2) = 0;
    }
    else {
      if (*(ushort *)(*(longlong *)_refptr_g_tsaBufTzAge + (longlong)(int)(*(uint *)(_stk + 0x154)) * 2) < 2) {
        (*(int *)(_stk + 0x150)) = 2;
      }
      else {
        (*(int *)(_stk + 0x150)) = (*(int *)(_stk + 0x14c)) +
                   (uint)*(ushort *)
                          (*(longlong *)_refptr_g_tsaBufTzAge + (longlong)(int)(*(uint *)(_stk + 0x154)) * 2);
        if (0xffff < (*(int *)(_stk + 0x150))) {
          (*(int *)(_stk + 0x150)) = 0xffff;
        }
      }
      cVar1 = Sensor_GetZoneIndex2((*(uint *)(_stk + 0x154)) & 0xffff);
      (*(int *)(_stk + 0x148)) = (int)cVar1;
      if (((-1 < (*(int *)(_stk + 0x148))) && ((*(int *)(_stk + 0x148)) < 0x1e)) &&
         ((int)(uint)*(ushort *)((longlong)((undefined8 *)(_stk + 0x108)) + (longlong)(*(int *)(_stk + 0x148)) * 2) < (*(int *)(_stk + 0x150)))) {
        *(short *)((longlong)((undefined8 *)(_stk + 0x108)) + (longlong)(*(int *)(_stk + 0x148)) * 2) = (short)(*(int *)(_stk + 0x150));
      }
      *(short *)(*(longlong *)_refptr_g_tsaBufTzAge + (longlong)(int)(*(uint *)(_stk + 0x154)) * 2) = (short)(*(int *)(_stk + 0x150))
      ;
    }
  }
  (*(uint *)(_stk + 0x154)) = (uint)_refptr_g_tsaPrmtFlash->wBufElementCount;
  while ((*(uint *)(_stk + 0x154)) = (*(uint *)(_stk + 0x154)) - 1, -1 < (int)(*(uint *)(_stk + 0x154))) {
    cVar1 = Sensor_GetZoneIndex2((*(uint *)(_stk + 0x154)) & 0xffff);
    (*(int *)(_stk + 0x144)) = (int)cVar1;
    if (((-1 < (*(int *)(_stk + 0x144))) && ((*(int *)(_stk + 0x144)) < 0x1e)) &&
       (*(short *)(*(longlong *)_refptr_g_tsaBufTzAge + (longlong)(int)(*(uint *)(_stk + 0x154)) * 2) != 0)) {
      *(undefined2 *)((longlong)(int)(*(uint *)(_stk + 0x154)) * 2 + *(longlong *)_refptr_g_tsaBufTzAge) =
           *(undefined2 *)((longlong)((undefined8 *)(_stk + 0x108)) + (longlong)(*(int *)(_stk + 0x144)) * 2);
    }
  }
  *(void **)&_refptr_g_tsaPrpt[1].field_0xc = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  return;
}


// ============================================================================
// Function: TZ_UpdatePeakTzAge @ 6ba8c9ad
// ============================================================================

void TZ_UpdatePeakTzAge(void)

{
  byte local_9;
  
  for (local_9 = 0; (local_9 < **(byte **)_refptr_g_touchesPtr && (local_9 != 0x14));
      local_9 = local_9 + 1) {
    TZ_UpdatePeakTzAgeUnit(local_9);
  }
  return;
}


// ============================================================================
// Function: TZ_CheckFatFingerSD @ 6ba8b4f9
// ============================================================================

undefined8 TZ_CheckFatFingerSD(void)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x30]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x30];
  char cVar1;
  int iVar2;
  int iVar3;
  
                    /* 0x4b4f9  2361  TZ_CheckFatFingerSD */
  cVar1 = SS_IsDim1Valid();
  if ((cVar1 == '\0') || (cVar1 = SS_IsDim2Valid(), cVar1 == '\0')) {
    return 0;
  }
  if (*(int *)&(*_refptr_g_tsaStaticPtr)[1].field_0x12 < 0) {
    return 0;
  }
  cVar1 = SignalDisparity_IsSignalCompensated();
  if (cVar1 == '\0') {
    return 0;
  }
  if ((*_refptr_g_peaksPtr)->bPeakCount == '\0') {
    return 0;
  }
  cVar1 = TSAStatic_IsChargerPlugged();
  if (cVar1 != '\0') {
    return 0;
  }
  if (1 < (byte)(*_refptr_g_prevTouchesPtr)->field_0x5) {
    return 0;
  }
  if ((7 < (uint)(*_refptr_g_prevTouchesPtr)->nTouchCount) &&
     ((*_refptr_g_prevTouchesPtr)->nTouchCount != 0x8000)) {
    return 0;
  }
  TZ_GetTouchZoneRange(((int *)(_stk + 0xc)),&(*(int *)(_stk + 0x8)),&(*(int *)(_stk + 0x4)),&(*(int *)(_stk + 0x0)));
  for ((*(byte *)(_stk + 0x27)) = 1; (*(byte *)(_stk + 0x27)) < (*_refptr_g_peaksPtr)->bPeakCount; (*(byte *)(_stk + 0x27)) = (*(byte *)(_stk + 0x27)) + 1) {
    if (((((*(int *)(_stk + 0x8)) <
           (int)(uint)(*_refptr_g_peaksPtr)[(longlong)(int)(uint)(*(byte *)(_stk + 0x27)) * 0x14 + 0x15].bPeakCount)
         || ((int)(uint)(*_refptr_g_peaksPtr)[(longlong)(int)(uint)(*(byte *)(_stk + 0x27)) * 0x14 + 0x15].bPeakCount
             < ((int *)(_stk + 0xc))[0])) ||
        ((*(int *)(_stk + 0x0)) <
         (int)(uint)(*_refptr_g_peaksPtr)[(longlong)(int)(uint)(*(byte *)(_stk + 0x27)) * 0x14 + 0x16].bPeakCount))
       || ((int)(uint)(*_refptr_g_peaksPtr)[(longlong)(int)(uint)(*(byte *)(_stk + 0x27)) * 0x14 + 0x16].bPeakCount <
           (*(int *)(_stk + 0x4)))) {
      return 0;
    }
  }
  iVar2 = ((*(int *)(_stk + 0x8)) - ((int *)(_stk + 0xc))[0]) + 1;
  if ((iVar2 < 4) || (10 < iVar2)) {
    return 0;
  }
  iVar3 = ((*(int *)(_stk + 0x0)) - (*(int *)(_stk + 0x4))) + 1;
  if ((3 < iVar3) && (iVar3 < 0xb)) {
    if ((0xf < iVar2 * iVar3) && (iVar2 * iVar3 < 0x52)) {
      return 1;
    }
    return 0;
  }
  return 0;
}


// ============================================================================
// Function: TZ_UpdateGripFlag @ 6ba8b852
// ============================================================================

void TZ_UpdateGripFlag(undefined8 param_1)

{
                    /* 0x4b852  2453  TZ_UpdateGripFlag */
  TZ_UpdateGripFlagDim(param_1,0);
  TZ_UpdateGripFlagDim(param_1,_refptr_g_tsaPrmtFlash->bCols + 0xff);
  return;
}


// ============================================================================
// Function: TZ_SimpleTraversal @ 6ba8b88f
// ============================================================================

void TZ_SimpleTraversal(longlong param_1,byte param_2)

{
  short sVar1;
  
  sVar1 = TZ_GetSigThold(param_1,param_2);
  TZ_UnitInit(param_1);
  *(uchar *)(param_1 + 0x325) =
       (*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_2 * 0x14 + 0x14].bPeakCount;
  TZ_PeakBasedTraversalSimple
            (param_1,(int)sVar1,
             (*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_2 * 0x14 + 0x15].bPeakCount,
             (*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_2 * 0x14 + 0x16].bPeakCount);
  *(undefined *)(param_1 + 0x35e) = _refptr_g_tzAccess[9];
  TZ_CentroidCore(param_1);
  TZ_GetAxis(param_1);
  TZ_GetGripRatio(param_1);
  TZ_GetEdgeTouchedFlag(param_1);
  TZ_UnitToTouch(param_1);
  _refptr_g_tzAccess[9] = _refptr_g_tzAccess[9] + '\b';
  return;
}


// ============================================================================
// Function: TZ_CentrodToTouch @ 6ba8b9bf
// ============================================================================

void TZ_CentrodToTouch(longlong param_1,byte param_2)

{
  TZ_UnitInit(param_1);
  CTD_General((*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_2 * 0x14 + 0x15].bPeakCount,
              (*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_2 * 0x14 + 0x16].bPeakCount,3,3,
              param_1 + 0x2ae,param_1 + 0x2b0);
  *(int *)(param_1 + 0x18) =
       *(int *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)param_2 * 0x14 + 0x1c) +
       (int)*(short *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)param_2 * 0x14 + 0x18);
  *(undefined1 *)(param_1 + 0x334) = 0x10;
  *(undefined1 *)(param_1 + 0x335) = 0x10;
  TZ_UnitToTouch(param_1);
  return;
}


// ============================================================================
// Function: TZ_NormalTraversal @ 6ba8baec
// ============================================================================

void TZ_NormalTraversal(longlong param_1,byte param_2,undefined1 param_3)

{
  uint uVar1;
  short sVar2;
  
  sVar2 = TZ_GetSigThold(param_1,param_2);
  TZ_UnitInit(param_1);
  *(uchar *)(param_1 + 0x325) =
       (*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_2 * 0x14 + 0x14].bPeakCount;
  *(undefined1 *)(param_1 + 0x34b) = param_3;
  *(byte *)(param_1 + 0x2b3) = param_2;
  uVar1 = _refptr_g_tsaPrmtFlash->dwGripFlags;
  if (((((*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_2 * 0x14 + 0x15].bPeakCount < 2) ||
       ((uint)_refptr_g_tsaPrmtFlash->bCols <=
        (*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_2 * 0x14 + 0x15].bPeakCount + 2)) ||
      ((*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_2 * 0x14 + 0x16].bPeakCount < 2)) ||
     ((uint)_refptr_g_tsaPrmtFlash->bRows <=
      (*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_2 * 0x14 + 0x16].bPeakCount + 2)) {
    _refptr_g_tsaPrmtFlash->dwGripFlags = _refptr_g_tsaPrmtFlash->dwGripFlags & 0xfffffbff;
  }
  TZ_PeakBasedTraversal
            (param_1,(int)sVar2,
             (*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_2 * 0x14 + 0x15].bPeakCount,
             (*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_2 * 0x14 + 0x16].bPeakCount);
  _refptr_g_tsaPrmtFlash->dwGripFlags = uVar1;
  *(undefined *)(param_1 + 0x35e) = _refptr_g_tzAccess[9];
  TZ_UnitProcess(param_1);
  _refptr_g_tzAccess[9] = _refptr_g_tzAccess[9] + '\b';
  return;
}


// ============================================================================
// Function: TZ_DilateAndErode @ 6ba8bcf4
// ============================================================================

void TZ_DilateAndErode(void)

{
  ushort uVar1;
  errno_t eVar2;
  void *pvVar3;
  
  uVar1 = _refptr_g_tsaPrmtFlash->wBufElementCount;
  pvVar3 = (void *)TZ_GetAccessFlagPtr();
  eVar2 = memcpy_s(_refptr_g_tsaFlagAfterDilate,0x1324,pvVar3,(ulonglong)uVar1);
  if (eVar2 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/TouchZone/TZ_StateMachine.c",0x1f0,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  TZ_Transform(_refptr_g_tsaFlagAfterDilate,0x14,0,1);
  uVar1 = _refptr_g_tsaPrmtFlash->wBufElementCount;
  pvVar3 = (void *)TZ_GetAccessFlagPtr();
  eVar2 = memcpy_s(_refptr_g_tsaFlagAfterErode,0x1324,pvVar3,(ulonglong)uVar1);
  if (eVar2 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/TouchZone/TZ_StateMachine.c",499,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  TZ_Transform(_refptr_g_tsaFlagAfterErode,0x14,1,1);
  return;
}


// ============================================================================
// Function: TZ_SSLoProcess @ 6ba8be08
// ============================================================================

void TZ_SSLoProcess(void)

{
  byte bVar1;
  byte bVar2;
  byte bVar3;
  byte bVar4;
  
                    /* 0x4be08  2441  TZ_SSLoProcess */
  bVar1 = SS_TouchLength(_refptr_g_tsaBufSSDif,_refptr_g_tsaPrmtFlash->bCols,600);
  bVar2 = SS_TouchLength(_refptr_g_tsaBufSSDif +
                         (longlong)(int)(uint)_refptr_g_tsaPrmtFlash->bCols * 4,
                         _refptr_g_tsaPrmtFlash->bRows,600);
  bVar3 = SS_TouchLength(_refptr_g_tsaBufSSDif,_refptr_g_tsaPrmtFlash->bCols,400);
  bVar4 = SS_TouchLength(_refptr_g_tsaBufSSDif +
                         (longlong)(int)(uint)_refptr_g_tsaPrmtFlash->bCols * 4,
                         _refptr_g_tsaPrmtFlash->bRows,400);
  if (((bVar1 < 0xe) || (bVar2 < 0xe)) || ((uint)bVar2 + (uint)bVar1 < 0x22)) {
    *(undefined4 *)_refptr_g_isSSLo = 0;
  }
  else {
    *(undefined4 *)_refptr_g_isSSLo = 1;
  }
  if ((7 < (*_refptr_g_peaksPtr)->bPeakCount) && ((0x19 < bVar1 || (0x19 < bVar2)))) {
    *(undefined4 *)_refptr_g_isSSLo = 1;
  }
  if (((bVar3 < 0xd) || (bVar4 < 0xd)) || ((uint)bVar4 + (uint)bVar3 < 0x1c)) {
    *(undefined4 *)_refptr_g_isSSLoSD = 0;
  }
  else {
    *(undefined4 *)_refptr_g_isSSLoSD = 1;
  }
  if ((7 < (*_refptr_g_peaksPtr)->bPeakCount) && ((0x19 < bVar3 || (0x19 < bVar4)))) {
    *(undefined4 *)_refptr_g_isSSLoSD = 1;
  }
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,
                      "\nTZ_SSLoProcess count1=%d,count2=%d,ssLo=%d,count1SD=%d,count2SD=%d,ssLoSD=%d"
                      ,bVar1,bVar2,*(undefined4 *)_refptr_g_isSSLo,bVar3,bVar4,
                      *(undefined4 *)_refptr_g_isSSLoSD);
  }
  return;
}


// ============================================================================
// Function: TZ_UpdatePeakTzAgeUnit @ 6ba8c665
// ============================================================================

void TZ_UpdatePeakTzAgeUnit(byte param_1)

{
  byte bVar1;
  byte bVar2;
  byte bVar3;
  byte bVar4;
  short sVar5;
  int iVar6;
  uint uVar7;
  
  bVar1 = (*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_1 * 0x14 + 0x15].bPeakCount;
  bVar2 = (*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_1 * 0x14 + 0x16].bPeakCount;
  bVar3 = PrevTouch_GetIdxBasedOnID
                    ((*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_1 * 0x14 + 0x14].bPeakCount);
  iVar6 = *(short *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)param_1 * 0x14 + 0x18) * 3;
  if (iVar6 < 0) {
    iVar6 = iVar6 + 3;
  }
  sVar5 = (short)(iVar6 >> 2);
  if (bVar3 == 0x14) {
    if ((bVar1 < 2) || ((int)(_refptr_g_tsaPrmtFlash->bCols - 3) < (int)(uint)bVar1)) {
      if (bVar1 == 1) {
        if (sVar5 < *(short *)((ulonglong)
                               ((uint)bVar2 * (uint)_refptr_g_tsaPrmtFlash->bCols + 2 & 0xffff) * 2
                              + *(longlong *)_refptr_g_tsaBufDif)) {
          *(undefined2 *)
           ((ulonglong)((uint)bVar2 * (uint)_refptr_g_tsaPrmtFlash->bCols + 1 & 0xffff) * 2 +
           *(longlong *)_refptr_g_tsaBufTzAge) = 1;
        }
      }
      else if (((uint)bVar1 == _refptr_g_tsaPrmtFlash->bCols - 2) &&
              (sVar5 < *(short *)((ulonglong)
                                  (((uint)bVar1 + (uint)bVar2 * (uint)_refptr_g_tsaPrmtFlash->bCols)
                                   - 1 & 0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif))) {
        *(undefined2 *)
         ((ulonglong)((uint)bVar1 + (uint)bVar2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2
         + *(longlong *)_refptr_g_tsaBufTzAge) = 1;
      }
    }
    else {
      *(undefined2 *)
       ((ulonglong)((uint)bVar1 + (uint)bVar2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
       *(longlong *)_refptr_g_tsaBufTzAge) = 1;
    }
  }
  else {
    uVar7 = Sensor_GetPosType(bVar1,bVar2);
    if (uVar7 < 3) {
      bVar4 = (byte)((ushort)*(undefined2 *)
                              ((longlong)*_refptr_g_prevTouchesPtr +
                              (longlong)(int)(uint)bVar3 * 0x478 + 0x15e) >> 8);
      if ((bVar4 != 0) && ((int)(uint)bVar4 < (int)(_refptr_g_tsaPrmtFlash->bCols - 1))) {
        *(undefined2 *)
         (*(longlong *)_refptr_g_tsaBufTzAge +
         (ulonglong)((uint)bVar1 + (uint)bVar2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2)
             = *(undefined2 *)
                ((ulonglong)
                 ((uint)bVar4 +
                  (uint)(byte)((ushort)*(undefined2 *)
                                        ((longlong)*_refptr_g_prevTouchesPtr +
                                        (longlong)(int)(uint)bVar3 * 0x478 + 0x160) >> 8) *
                  (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
                *(longlong *)_refptr_g_tsaBufTzAge);
      }
    }
  }
  return;
}


// ============================================================================
// Function: TZ_Init @ 6ba8aa70
// ============================================================================

void TZ_Init(void)

{
                    /* 0x4aa70  2404  TZ_Init */
  TZ_VPInit();
  TZ_ContourInit();
  *(undefined8 *)&_refptr_g_tsaPrpt[1].field_0xc = 0xffffffffffffffff;
  _refptr_g_tzAccess[9] = 0x10;
  *(undefined8 *)_refptr_g_tzAccess = *(undefined8 *)_refptr_g_tsaBufTz;
  *(undefined **)_refptr_g_ptrAccess = _refptr_g_tzAccess;
  return;
}


// ============================================================================
// Function: TZ_GetTouchZoneRange @ 6ba8b2b9
// ============================================================================

void TZ_GetTouchZoneRange(uint *param_1,uint *param_2,uint *param_3,uint *param_4)

{
  byte bVar1;
  byte bVar2;
  
                    /* 0x4b2b9  2400  TZ_GetTouchZoneRange */
  bVar1 = (*_refptr_g_peaksPtr)[0x15].bPeakCount;
  bVar2 = (*_refptr_g_peaksPtr)[0x16].bPeakCount;
  *param_1 = (uint)bVar1;
  while ((0 < (int)*param_1 &&
         ((200 < *(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)(*param_1 - 1) * 4) ||
          (600 < *(int *)(_refptr_g_tsaBufSelf + ((longlong)(int)(*param_1 - 1) + 0x280) * 4)))))) {
    *param_1 = *param_1 - 1;
  }
  *param_2 = (uint)bVar1;
  while (((int)*param_2 < (int)(_refptr_g_tsaPrmtFlash->bCols - 1) &&
         ((200 < *(int *)(_refptr_g_tsaBufSSDif + (longlong)(int)(*param_2 + 1) * 4) ||
          (600 < *(int *)(_refptr_g_tsaBufSelf + ((longlong)(int)(*param_2 + 1) + 0x280) * 4)))))) {
    *param_2 = *param_2 + 1;
  }
  *param_3 = (uint)bVar2;
  while ((0 < (int)*param_3 &&
         ((200 < *(int *)(_refptr_g_tsaBufSSDif +
                         (longlong)(int)(*param_3 + (uint)_refptr_g_tsaPrmtFlash->bCols + -1) * 4)
          || (600 < *(int *)(_refptr_g_tsaBufSelf +
                            ((longlong)(int)(*param_3 + (uint)_refptr_g_tsaPrmtFlash->bCols + -1) +
                            0x280) * 4)))))) {
    *param_3 = *param_3 - 1;
  }
  *param_4 = (uint)bVar2;
  while (((int)*param_4 < (int)(_refptr_g_tsaPrmtFlash->bRows - 1) &&
         ((200 < *(int *)(_refptr_g_tsaBufSSDif +
                         (longlong)(int)(*param_4 + (uint)_refptr_g_tsaPrmtFlash->bCols + 1) * 4) ||
          (600 < *(int *)(_refptr_g_tsaBufSelf +
                         ((longlong)(int)(*param_4 + (uint)_refptr_g_tsaPrmtFlash->bCols + 1) +
                         0x280) * 4)))))) {
    *param_4 = *param_4 + 1;
  }
  return;
}


// ============================================================================
// Function: TZ_UpdateGripFlagDim @ 6ba8b70a
// ============================================================================

void TZ_UpdateGripFlagDim(longlong param_1,byte param_2)

{
  ushort uVar1;
  ushort uVar2;
  ushort uVar3;
  short sVar4;
  char cVar5;
  short local_c;
  byte local_9;
  
  uVar1 = _refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax;
  local_c = -0x8000;
  for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bRows; local_9 = local_9 + 1) {
    uVar3 = (ushort)param_2 + (ushort)local_9 * (ushort)_refptr_g_tsaPrmtFlash->bCols;
    uVar2 = *(ushort *)
             ((ulonglong)
              ((uint)param_2 + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
             *(longlong *)_refptr_g_tsaBufDif);
    sVar4 = (uVar2 ^ (short)uVar2 >> 0x1f) - ((short)uVar2 >> 0xf);
    if (*(char *)(param_1 + (ulonglong)uVar3) == '\0') {
      if ((short)uVar1 < sVar4) {
        if (*(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)uVar3 * 2) < 0) {
          cVar5 = -1;
        }
        else {
          cVar5 = '\x01';
        }
        *(char *)((ulonglong)uVar3 + param_1) =
             (*(char *)(*(longlong *)_refptr_g_ptrAccess + 9) + '\a') * cVar5;
      }
      else if ((short)uVar1 < local_c) {
        _refptr_g_tzAccess[9] = _refptr_g_tzAccess[9] + '\b';
      }
    }
    local_c = sVar4;
  }
  return;
}


// ============================================================================
// Function: TZ_GetAxis @ 6ba7f08d
// ============================================================================

void TZ_GetAxis(longlong param_1)

{
  byte bVar1;
  
                    /* 0x3f08d  2369  TZ_GetAxis */
  TZ_GetAxisInternal((double)*(int *)(param_1 + 0x2f8),(double)*(int *)(param_1 + 0x2fc),
                     (double)*(int *)(param_1 + 0x300),(double)*(longlong *)(param_1 + 0x308),
                     (double)*(longlong *)(param_1 + 0x310),(double)*(longlong *)(param_1 + 0x318),
                     param_1 + 0x2f0,param_1 + 0x2f2,param_1 + 0x2f4);
  bVar1 = TS_PixelToMM(*(undefined2 *)(param_1 + 0x2f0));
  *(ushort *)(param_1 + 0x2ec) = (ushort)bVar1;
  bVar1 = TS_PixelToMM(*(undefined2 *)(param_1 + 0x2f2));
  *(ushort *)(param_1 + 0x2ee) = (ushort)bVar1;
  return;
}


// ============================================================================
// Function: TZ_CentroidCore @ 6ba7f363
// ============================================================================

void TZ_CentroidCore(longlong param_1)

{
  undefined2 uVar1;
  undefined2 uVar2;
  
                    /* 0x3f363  2358  TZ_CentroidCore */
  uVar1 = CTD_BasicCalculation(*(undefined4 *)(param_1 + 0x2c4),*(undefined4 *)(param_1 + 0x1c));
  uVar2 = CTD_BasicCalculation(*(undefined4 *)(param_1 + 0x2c8),*(undefined4 *)(param_1 + 0x20));
  *(undefined2 *)(param_1 + 0x2ae) = uVar1;
  *(undefined2 *)(param_1 + 0x2b0) = uVar2;
  return;
}


// ============================================================================
// Function: TZ_UnitInit @ 6ba7f620
// ============================================================================

void TZ_UnitInit(undefined8 param_1)

{
  int iVar1;
  
                    /* 0x3f620  2449  TZ_UnitInit */
  iVar1 = memset_s(param_1,0x370,0,0x370);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/TouchZone/TZ_Common.c",0x2c,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: TZ_UnitToTouch @ 6ba8134b
// ============================================================================

void TZ_UnitToTouch(longlong param_1)

{
                    /* 0x4134b  2452  TZ_UnitToTouch */
  *(char *)(*(longlong *)_refptr_g_touchesPtr + 2) =
       *(char *)(*(longlong *)_refptr_g_touchesPtr + 2) + '\x01';
  *(undefined2 *)(param_1 + 0x2ea) = *(undefined2 *)(param_1 + 0x2ec);
  if ((_refptr_g_tsaPrmtFlash->dwGripFlags & 8) == 0) {
    if ((*(int *)(param_1 + 0x350) == 0) || ((*(uint *)(param_1 + 4) & 0x1000000) != 0)) {
      TZ_UnitToTouchNormal(param_1);
    }
    else {
      TZ_ExceptionProcess(param_1);
    }
  }
  else {
    TZ_RegisterTouch(param_1);
  }
  return;
}


// ============================================================================
// Function: TZ_GetEdgeTouchedFlag @ 6ba81544
// ============================================================================

void TZ_GetEdgeTouchedFlag(longlong param_1)

{
                    /* 0x41544  2377  TZ_GetEdgeTouchedFlag */
  if ((((*(char *)(param_1 + 0x33) == '\0') ||
       ((uint)*(byte *)(param_1 + 0x34) == _refptr_g_tsaPrmtFlash->bCols - 1)) ||
      (*(char *)(param_1 + 0x35) == '\0')) ||
     ((uint)*(byte *)(param_1 + 0x36) == _refptr_g_tsaPrmtFlash->bRows - 1)) {
    *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x20;
  }
  if (((*(byte *)(param_1 + 0x33) < 2) ||
      ((int)(_refptr_g_tsaPrmtFlash->bCols - 2) <= (int)(uint)*(byte *)(param_1 + 0x34))) ||
     ((*(byte *)(param_1 + 0x35) < 2 ||
      ((int)(_refptr_g_tsaPrmtFlash->bRows - 2) <= (int)(uint)*(byte *)(param_1 + 0x36))))) {
    *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x80000;
  }
  return;
}


// ============================================================================
// Function: TZ_GetGripRatio @ 6ba81f2f
// ============================================================================

void TZ_GetGripRatio(longlong param_1)

{
  ushort uVar1;
  
                    /* 0x41f2f  2380  TZ_GetGripRatio */
  uVar1 = TZ_CentroidGripRatio(*(undefined4 *)(param_1 + 0x2cc),*(undefined4 *)(param_1 + 0x2d0));
  *(ushort *)(param_1 + 0x326) = uVar1;
  if (0xfe < uVar1) {
    uVar1 = 0xffff;
  }
  *(char *)(param_1 + 0x334) = (char)uVar1;
  uVar1 = TZ_CentroidGripRatio(*(undefined4 *)(param_1 + 0x2d4),*(undefined4 *)(param_1 + 0x2d8));
  *(ushort *)(param_1 + 0x328) = uVar1;
  if (0xfe < uVar1) {
    uVar1 = 0xffff;
  }
  *(char *)(param_1 + 0x335) = (char)uVar1;
  return;
}


// ============================================================================
// Function: TZ_PeakBasedTraversalSimple @ 6ba89b60
// ============================================================================

void TZ_PeakBasedTraversalSimple(undefined8 param_1,short param_2,byte param_3,byte param_4)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0xe8]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0xf0];
  char cVar1;
  
                    /* 0x49b60  2434  TZ_PeakBasedTraversalSimple */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&_rdata,"TZ_PeakBasedTraversalSimple");
  }
  (*(longlong *)(_stk + 0xe0)) = *(longlong *)_refptr_g_tzAccess;
  PosnSpecial_Init(((undefined1 *)(_stk + 0x8)),100);
  TZ_UnitMaxMinInit(param_1,(int)param_2);
  PosnSpecial_Put((ushort)param_3 + (ushort)param_4 * (ushort)_refptr_g_tsaPrmtFlash->bCols);
  while( true ) {
    cVar1 = PosnSpecial_IsEmpty();
    if (cVar1 != '\0') break;
    PosnSpecial_ReBase();
    (*(ushort *)(_stk + 0xde)) = PosnSpecial_Get();
    TZ_TraversalNeighborSimple(param_1,(*(ushort *)(_stk + 0xde)),(*(longlong *)(_stk + 0xe0)),(int)param_2);
    if (*(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)(*(ushort *)(_stk + 0xde)) * 2) < 0) {
      cVar1 = -1;
    }
    else {
      cVar1 = '\x01';
    }
    *(char *)((ulonglong)(*(ushort *)(_stk + 0xde)) + (*(longlong *)(_stk + 0xe0))) =
         (*(char *)(*(longlong *)_refptr_g_ptrAccess + 9) + '\x05') * cVar1;
  }
  return;
}


// ============================================================================
// Function: TZ_GetSigThold @ 6ba8a90b
// ============================================================================

short TZ_GetSigThold(undefined8 param_1,byte param_2)

{
  short sVar1;
  char cVar2;
  short local_a;
  
                    /* 0x4a90b  2398  TZ_GetSigThold */
  sVar1 = *(short *)&_refptr_g_tsaPrpt[1].field_0x14;
  if ((_refptr_g_tsaPrmtFlash->dwGripFlags & 4) == 0) {
    if (((*_refptr_g_tsaStaticPtr)->dwProcessingState & 0x10) == 0) {
      cVar2 = TSAStatic_IsAFTModeEnabled();
      if (cVar2 == '\0') {
        if (((*_refptr_g_prevTouchesPtr)->nTouchCount != 0x100) &&
           ((*_refptr_g_tsaStaticPtr)->bField_0x111 != '\x02')) {
          cVar2 = TSAStatic_IsChargerPlugged();
          if (cVar2 == '\0') {
            local_a = TZ_GetSigTholdCommon(param_1,param_2);
            goto LAB_6ba8a9f1;
          }
        }
        local_a = TZ_GetSigTholdForCharger(param_2);
      }
      else {
        local_a = TZ_GetSigTholdForAFT(param_2);
      }
    }
    else {
      local_a = TZ_GetSigTholdForSideTouch(param_2);
    }
  }
  else {
    local_a = *(short *)&_refptr_g_tsaPrmtFlash->field_0x81c;
  }
LAB_6ba8a9f1:
  if (local_a < sVar1) {
    local_a = sVar1;
  }
  if (*(short *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)param_2 * 0x14 + 0x18) < local_a) {
    local_a = *(short *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)param_2 * 0x14 + 0x18);
  }
  return local_a;
}


// ============================================================================
// Function: TZ_PeakBasedTraversal @ 6ba89162
// ============================================================================

void TZ_PeakBasedTraversal(longlong param_1,short param_2,byte param_3,byte param_4)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0xe8]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0xf0];
  byte bVar1;
  char cVar2;
  int iVar3;
  undefined8 in_stack_fffffffffffffef8;
  undefined4 uVar4;
  
                    /* 0x49162  2433  TZ_PeakBasedTraversal */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&_rdata,"TZ_PeakBasedTraversal");
  }
  (*(longlong *)(_stk + 0xe0)) = *(longlong *)_refptr_g_tzAccess;
  (*(int *)(_stk + 0xdc)) = memset_s(_refptr_g_tsaBufSelf + 0x1400,0x280,0,0x280);
  if ((*(int *)(_stk + 0xdc)) != 0) {
    in_stack_fffffffffffffef8 = 0;
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/TouchZone/TZ_PeakBasedTraversal.c",0x563,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (*(int *)(_stk + 0xdc)) = memset_s(_refptr_g_tsaBufSelf + 0x1900,0x280,0,0x280);
  if ((*(int *)(_stk + 0xdc)) != 0) {
    in_stack_fffffffffffffef8 = 0;
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/TouchZone/TZ_PeakBasedTraversal.c",0x566,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  PosnSpecial_Init(((undefined1 *)(_stk + 0x8)),100);
  TZ_UnitMaxMinInit(param_1,(int)param_2);
  memset_s(g_tzBuffer,(ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,0,
           (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  (*(short *)(_stk + 0xda)) = *(short *)((ulonglong)
                        ((uint)param_3 + (uint)param_4 * (uint)_refptr_g_tsaPrmtFlash->bCols &
                        0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif);
  (*(undefined2 *)(_stk + 0xd8)) = *(undefined2 *)
              ((ulonglong)
               ((uint)param_3 + (uint)param_4 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
              *(longlong *)_refptr_g_tsaBufTzAge);
  (*(short *)(_stk + 0xd6)) = TZ_GetEdgeSigThold((int)(*(short *)(_stk + 0xda)),(int)param_2);
  (*(short *)(_stk + 0xd4)) = _refptr_g_tsaPrpt->nSPeakPositive / 2;
  PosnSpecial_Put((ushort)param_3 + (ushort)param_4 * (ushort)_refptr_g_tsaPrmtFlash->bCols);
LAB_6ba89671:
  do {
    while( true ) {
      uVar4 = (undefined4)((ulonglong)in_stack_fffffffffffffef8 >> 0x20);
      cVar2 = PosnSpecial_IsEmpty();
      if (cVar2 != '\0') {
        TZ_GetEdgeWidth(param_1,(int)(short)*(undefined4 *)&_refptr_g_tsaPrmtFlash->field_0x838);
        return;
      }
      PosnSpecial_ReBase();
      (*(ushort *)(_stk + 0xd2)) = PosnSpecial_Get();
      in_stack_fffffffffffffef8 = CONCAT44(uVar4,(int)(*(short *)(_stk + 0xd6)));
      TZ_TraversalNeighbor
                (param_1,(*(ushort *)(_stk + 0xd2)),(*(longlong *)(_stk + 0xe0)),(int)param_2,in_stack_fffffffffffffef8,(int)(*(short *)(_stk + 0xd4)),
                 (*(undefined2 *)(_stk + 0xd8)));
      (*(short *)(_stk + 0xd0)) = *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)(*(ushort *)(_stk + 0xd2)) * 2);
      bVar1 = PosnSpecial_GetNumSignChange();
      if (2 < bVar1) {
        *(short *)(param_1 + 0x358) = *(short *)(param_1 + 0x358) + 1;
      }
      cVar2 = PosnSpecial_GetNumValidSensor();
      if (cVar2 != '\x04') break;
      if (*(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)(*(ushort *)(_stk + 0xd2)) * 2) < 0) {
        cVar2 = -1;
      }
      else {
        cVar2 = '\x01';
      }
      *(char *)((ulonglong)(*(ushort *)(_stk + 0xd2)) + (*(longlong *)(_stk + 0xe0))) =
           (*(char *)(*(longlong *)_refptr_g_ptrAccess + 9) + '\x06') * cVar2;
    }
    bVar1 = PosnSpecial_GetNumEdgeSensor();
    if ((bVar1 < 2) && (param_2 <= (*(short *)(_stk + 0xd0)))) {
      iVar3 = PosnSpecial_GetSumEdgeSensor();
      if (iVar3 < (*(short *)(_stk + 0xd0)) * 2) {
        if (*(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)(*(ushort *)(_stk + 0xd2)) * 2) < 0) {
          cVar2 = -1;
        }
        else {
          cVar2 = '\x01';
        }
        *(char *)((ulonglong)(*(ushort *)(_stk + 0xd2)) + (*(longlong *)(_stk + 0xe0))) =
             (*(char *)(*(longlong *)_refptr_g_ptrAccess + 9) + '\x03') * cVar2;
        *(short *)(param_1 + 0x35a) = *(short *)(param_1 + 0x35a) + 1;
        goto LAB_6ba89671;
      }
    }
    if ((*(short *)(_stk + 0xd0)) < (*(short *)(_stk + 0xd4))) {
      if ((*(short *)(_stk + 0xd6)) <= (*(short *)(_stk + 0xd0))) {
        if (*(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)(*(ushort *)(_stk + 0xd2)) * 2) < 0) {
          cVar2 = -1;
        }
        else {
          cVar2 = '\x01';
        }
        *(char *)((ulonglong)(*(ushort *)(_stk + 0xd2)) + (*(longlong *)(_stk + 0xe0))) =
             (*(char *)(*(longlong *)_refptr_g_ptrAccess + 9) + '\x04') * cVar2;
      }
    }
    else {
      if (*(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)(*(ushort *)(_stk + 0xd2)) * 2) < 0) {
        cVar2 = -1;
      }
      else {
        cVar2 = '\x01';
      }
      *(char *)((ulonglong)(*(ushort *)(_stk + 0xd2)) + (*(longlong *)(_stk + 0xe0))) =
           (*(char *)(*(longlong *)_refptr_g_ptrAccess + 9) + '\x05') * cVar2;
    }
  } while( true );
}


// ============================================================================
// Function: TZ_UnitProcess @ 6ba8b236
// ============================================================================

void TZ_UnitProcess(undefined8 param_1)

{
  char cVar1;
  
                    /* 0x4b236  2451  TZ_UnitProcess */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&_rdata,"TZ_UnitProcess");
  }
  TZ_PeakInfoRetrieval(param_1);
  TZ_BeforeSDCompensationPeakInfoRetrieval(param_1);
  cVar1 = TZ_IsValid(param_1);
  if (cVar1 != '\0') {
    TZ_GetPrevTouch(param_1);
    TZ_GetProperty(param_1);
    TZ_UnitToTouch(param_1);
  }
  return;
}


// ============================================================================
// Function: TZ_GetAccessFlagPtr @ 6ba8b2a9
// ============================================================================

undefined8 TZ_GetAccessFlagPtr(void)

{
                    /* 0x4b2a9  2367  TZ_GetAccessFlagPtr */
  return *(undefined8 *)_refptr_g_tsaBufTz;
}


// ============================================================================
// Function: TZ_Transform @ 6ba8ca70
// ============================================================================

void TZ_Transform(longlong param_1,char param_2,char param_3,char param_4)

{
  bool bVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  uint local_10;
  uint local_c;
  
                    /* 0x4ca70  2445  TZ_Transform */
  local_c = (uint)_refptr_g_tsaPrmtFlash->bCols;
  while (uVar4 = local_c, local_c = uVar4 - 1, -1 < (int)local_c) {
    uVar5 = (uint)_refptr_g_tsaPrmtFlash->bRows;
    while (local_10 = uVar5 - 1, -1 < (int)local_10) {
      uVar2 = _refptr_g_tsaPrmtFlash->bCols * local_10 + local_c & 0xffff;
      if (param_3 == '\0') {
        bVar1 = param_2 <= *(char *)(param_1 + (ulonglong)uVar2);
      }
      else {
        bVar1 = *(char *)(param_1 + (ulonglong)uVar2) < param_2;
      }
      if (bVar1) {
        _refptr_g_tsaDistanceMapBuf[uVar2] = 0;
        uVar5 = local_10;
      }
      else {
        _refptr_g_tsaDistanceMapBuf[uVar2] =
             _refptr_g_tsaPrmtFlash->bRows + _refptr_g_tsaPrmtFlash->bCols;
        uVar3 = _refptr_g_tsaPrmtFlash->bCols * local_10 + local_c + 1 & 0xffff;
        if (((int)uVar4 < (int)(uint)_refptr_g_tsaPrmtFlash->bCols) &&
           ((char)_refptr_g_tsaDistanceMapBuf[uVar3] + 1 <
            (int)(char)_refptr_g_tsaDistanceMapBuf[uVar2])) {
          _refptr_g_tsaDistanceMapBuf[uVar2] = _refptr_g_tsaDistanceMapBuf[uVar3] + '\x01';
        }
        uVar3 = _refptr_g_tsaPrmtFlash->bCols * uVar5 + local_c & 0xffff;
        bVar1 = (int)uVar5 < (int)(uint)_refptr_g_tsaPrmtFlash->bRows;
        uVar5 = local_10;
        if ((bVar1) &&
           ((char)_refptr_g_tsaDistanceMapBuf[uVar3] + 1 <
            (int)(char)_refptr_g_tsaDistanceMapBuf[uVar2])) {
          _refptr_g_tsaDistanceMapBuf[uVar2] = _refptr_g_tsaDistanceMapBuf[uVar3] + '\x01';
        }
      }
    }
  }
  for (local_c = 0; (int)local_c < (int)(uint)_refptr_g_tsaPrmtFlash->bCols; local_c = local_c + 1)
  {
    for (local_10 = 0; (int)local_10 < (int)(uint)_refptr_g_tsaPrmtFlash->bRows;
        local_10 = local_10 + 1) {
      uVar4 = _refptr_g_tsaPrmtFlash->bCols * local_10 + local_c & 0xffff;
      uVar5 = (_refptr_g_tsaPrmtFlash->bCols * local_10 + local_c) - 1 & 0xffff;
      if ((0 < (int)local_c) &&
         ((char)_refptr_g_tsaDistanceMapBuf[uVar5] + 1 <
          (int)(char)_refptr_g_tsaDistanceMapBuf[uVar4])) {
        _refptr_g_tsaDistanceMapBuf[uVar4] = _refptr_g_tsaDistanceMapBuf[uVar5] + '\x01';
      }
      uVar5 = (uint)_refptr_g_tsaPrmtFlash->bCols * (local_10 + -1) + local_c & 0xffff;
      if ((0 < (int)local_10) &&
         ((char)_refptr_g_tsaDistanceMapBuf[uVar5] + 1 <
          (int)(char)_refptr_g_tsaDistanceMapBuf[uVar4])) {
        _refptr_g_tsaDistanceMapBuf[uVar4] = _refptr_g_tsaDistanceMapBuf[uVar5] + '\x01';
      }
      if (param_4 < (char)_refptr_g_tsaDistanceMapBuf[uVar4]) {
        *(bool *)(param_1 + (ulonglong)uVar4) = param_3 != '\0';
      }
      else {
        *(bool *)((ulonglong)uVar4 + param_1) = param_3 == '\0';
      }
    }
  }
  return;
}


// ============================================================================
// Function: TZ_ContourInit @ 6ba8590b
// ============================================================================

void TZ_ContourInit(void)

{
  double dVar1;
  float fVar2;
  int local_c;
  
                    /* 0x4590b  2363  TZ_ContourInit */
  for (local_c = 0; local_c < 5; local_c = local_c + 1) {
    fVar2 = (float)local_c / DAT_6bb52ef0;
    dVar1 = (double)SinXDivX((double)(fVar2 + DAT_6bb52ef4));
    *(float *)(&g_afu + (longlong)local_c * 0x10) = (float)dVar1;
    dVar1 = (double)SinXDivX((double)fVar2);
    *(float *)(&DAT_6bc13984 + (longlong)local_c * 0x10) = (float)dVar1;
    dVar1 = (double)SinXDivX((double)(DAT_6bb52ef4 - fVar2));
    *(float *)(&DAT_6bc13988 + (longlong)local_c * 0x10) = (float)dVar1;
    dVar1 = (double)SinXDivX((double)(DAT_6bb52ee0 - fVar2));
    *(float *)(&DAT_6bc1398c + (longlong)local_c * 0x10) = (float)dVar1;
  }
  return;
}


// ============================================================================
// Function: TZ_VPInit @ 6ba8d320
// ============================================================================

void TZ_VPInit(void)

{
                    /* 0x4d320  2454  TZ_VPInit */
  (*_refptr_g_tsaStaticPtr)[1].field_0xe = 0;
  (*_refptr_g_tsaStaticPtr)[1].field_0xf = 0;
  return;
}


// ============================================================================
// Function: TZ_GetAxisInternal @ 6ba7ec90
// ============================================================================

void TZ_GetAxisInternal(double param_1,double param_2,double param_3,double param_4,double param_5,
                       double param_6,undefined2 *param_7,undefined2 *param_8,undefined1 *param_9)

{
  double dVar1;
  double dVar2;
  double dVar3;
  double dVar4;
  double local_res8;
  double local_10;
  
                    /* 0x3ec90  2370  TZ_GetAxisInternal */
  local_res8 = param_1;
  if (param_1 == 0.0) {
    local_res8 = 1.0;
  }
  dVar2 = param_2 / local_res8;
  dVar3 = param_3 / local_res8;
  dVar1 = param_5 / local_res8 - dVar2 * dVar2;
  dVar4 = param_6 / local_res8 - dVar3 * dVar3;
  dVar2 = param_4 / local_res8 - dVar2 * dVar3;
  dVar2 = sqrt(dVar2 * 4.0 * dVar2 + (dVar1 - dVar4) * (dVar1 - dVar4));
  local_10 = (dVar1 + dVar4 + dVar2) * 6.0;
  if (local_10 < 0.0) {
    local_10 = 0.0;
  }
  dVar3 = sqrt(local_10);
  local_10 = ((dVar1 + dVar4) - dVar2) * 6.0;
  if (local_10 < 0.0) {
    local_10 = 0.0;
  }
  dVar2 = sqrt(local_10);
  dVar1 = param_4 * local_res8 - param_2 * param_3;
  dVar1 = atan2(dVar1 + dVar1,
                -(((param_6 - param_5) * local_res8 - param_3 * param_3) + param_2 * param_2));
  if (0.5 <= dVar3) {
    *param_7 = (short)(int)(dVar3 * 256.0);
  }
  else {
    *param_7 = 0x80;
  }
  if (0.5 <= dVar2) {
    *param_8 = (short)(int)(dVar2 * 256.0);
  }
  else {
    *param_8 = 0x80;
  }
  *param_9 = (char)(int)((dVar1 * 57.29577957855229) / 2.0);
  return;
}


// ============================================================================
// Function: TZ_RegisterTouch @ 6ba8039b
// ============================================================================

void TZ_RegisterTouch(undefined2 *param_1)

{
  byte bVar1;
  undefined1 uVar2;
  undefined2 uVar3;
  longlong lVar4;
  
  if (**(byte **)_refptr_g_touchesPtr < 0x14) {
    bVar1 = **(byte **)_refptr_g_touchesPtr;
    if ((int)(*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x20) - 1) <
        (int)(uint)(ushort)param_1[0x157]) {
      param_1[0x157] = *(short *)(_refptr_g_tsaPrmtFlashConst + 0x20) + -1;
    }
    if ((int)(*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x22) - 1) <
        (int)(uint)(ushort)param_1[0x158]) {
      param_1[0x158] = *(short *)(_refptr_g_tsaPrmtFlashConst + 0x22) + -1;
    }
    lVar4 = *(longlong *)_refptr_g_touchesPtr + (longlong)(int)(uint)bVar1 * 0x168 + 0x140;
    *(void **)(lVar4 + 8) = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    *(undefined2 *)(lVar4 + 0x1e) = param_1[0x157];
    *(undefined2 *)(lVar4 + 0x20) = param_1[0x158];
    if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x1000) != 0) {
      uVar3 = TZ_StylusXYCorrection(param_1[0x157]);
      *(undefined2 *)(lVar4 + 0x1e) = uVar3;
      uVar3 = TZ_StylusXYCorrection(param_1[0x158]);
      *(undefined2 *)(lVar4 + 0x20) = uVar3;
    }
    *(undefined2 *)(lVar4 + 0x30) = param_1[0x157];
    *(undefined2 *)(lVar4 + 0x32) = param_1[0x158];
    *(undefined2 *)(lVar4 + 0x22) = param_1[0x157];
    *(undefined2 *)(lVar4 + 0x24) = param_1[0x158];
    *(undefined1 *)(lVar4 + 0x3e) = *(undefined1 *)((longlong)param_1 + 0x35);
    *(undefined1 *)(lVar4 + 0x3f) = *(undefined1 *)(param_1 + 0x1b);
    *(undefined1 *)(lVar4 + 0x3c) = *(undefined1 *)((longlong)param_1 + 0x33);
    *(undefined1 *)(lVar4 + 0x3d) = *(undefined1 *)(param_1 + 0x1a);
    *(char *)(lVar4 + 0x6c) =
         (*(char *)(param_1 + 0x1a) - *(char *)((longlong)param_1 + 0x33)) + '\x01';
    *(char *)(lVar4 + 0x6d) =
         (*(char *)(param_1 + 0x1b) - *(char *)((longlong)param_1 + 0x35)) + '\x01';
    *(undefined1 *)(lVar4 + 0x70) = *(undefined1 *)((longlong)param_1 + 0x43);
    *(undefined1 *)(lVar4 + 0x71) = *(undefined1 *)(param_1 + 0x22);
    *(undefined1 *)(lVar4 + 0x72) = *(undefined1 *)((longlong)param_1 + 0x45);
    uVar2 = TZ_GetTzEdgeWidthDim1(param_1);
    *(undefined1 *)(lVar4 + 0x6e) = uVar2;
    uVar2 = TZ_GetTzEdgeWidthDim2(param_1);
    *(undefined1 *)(lVar4 + 0x6f) = uVar2;
    *(undefined2 *)(lVar4 + 0xfe) = param_1[6];
    *(undefined2 *)(lVar4 + 0x148) = param_1[8];
    *(undefined2 *)(lVar4 + 0x68) = param_1[0x193];
    *(undefined2 *)(lVar4 + 0x6a) = param_1[0x194];
    *(undefined1 *)(lVar4 + 0x78) = *(undefined1 *)(param_1 + 0x1b6);
    *(undefined1 *)(lVar4 + 0x59) = *(undefined1 *)(param_1 + 0x195);
    *(undefined2 *)(lVar4 + 0x5a) = param_1[0x19b];
    *(undefined2 *)(lVar4 + 0x5c) = param_1[0x19c];
    *(undefined1 *)(lVar4 + 0x56) = *(undefined1 *)(param_1 + 0x19a);
    *(undefined1 *)(lVar4 + 0x57) = *(undefined1 *)((longlong)param_1 + 0x335);
    TZ_UpdateGripRatio(bVar1,param_1);
    *(undefined2 *)(lVar4 + 0xea) = param_1[0x175];
    *(undefined2 *)(lVar4 + 0x48) = param_1[0x176];
    *(undefined2 *)(lVar4 + 0x4a) = param_1[0x177];
    *(undefined2 *)(lVar4 + 0x4c) = param_1[0x178];
    *(undefined2 *)(lVar4 + 0x4e) = param_1[0x179];
    *(undefined1 *)(lVar4 + 0x52) = *(undefined1 *)(param_1 + 0x17a);
    *(undefined2 *)(lVar4 + 0x60) = param_1[0x155];
    *(undefined2 *)(lVar4 + 100) = param_1[0x156];
    uVar3 = TZ_GetMinEllipseRate(bVar1);
    *(undefined2 *)(lVar4 + 0x66) = uVar3;
    *(undefined4 *)(lVar4 + 0xa0) = *(undefined4 *)(param_1 + 4);
    *(undefined4 *)(lVar4 + 0x8c) = *(undefined4 *)(param_1 + 0xc);
    *(undefined4 *)(lVar4 + 0x90) = *(undefined4 *)(param_1 + 0x12);
    *(undefined4 *)(lVar4 + 0x98) = *(undefined4 *)(param_1 + 10);
    uVar3 = TZ_GetZValue(*(undefined4 *)(lVar4 + 0x8c));
    *(undefined2 *)(lVar4 + 0x1c) = uVar3;
    *(undefined2 *)(lVar4 + 0x14a) = *param_1;
    *(undefined2 *)(lVar4 + 0x15e) = param_1[0x17];
    *(undefined1 *)(lVar4 + 0xfc) = *(undefined1 *)(param_1 + 0x15b);
    *(undefined1 *)(lVar4 + 0xfd) = *(undefined1 *)((longlong)param_1 + 0x2b5);
    *(undefined4 *)(lVar4 + 0x130) = *(undefined4 *)(param_1 + 0x166);
    *(undefined4 *)(lVar4 + 0x134) = *(undefined4 *)(param_1 + 0x16a);
    *(undefined4 *)(lVar4 + 0xac) = *(undefined4 *)(param_1 + 0x1a8);
    *(undefined1 *)(lVar4 + 0x164) = *(undefined1 *)((longlong)param_1 + 0x36d);
    TZ_CentroidEdgeFlag(param_1,bVar1);
    TZ_UpdateTouchFlag(param_1,bVar1);
    TZ_UpdateInsideSensor(param_1,lVar4 + 0x145,lVar4 + 0x146);
    *(undefined4 *)(lVar4 + 0xa8) = 1;
    *(undefined1 *)(lVar4 + 0x14c) = DAT_6bbc8969;
    *(undefined2 *)(lVar4 + 0x5e) = param_1[0x1b1];
    uVar2 = TZ_GetId(param_1,*(undefined1 *)(lVar4 + 0x14));
    *(undefined1 *)(lVar4 + 0x14) = uVar2;
    **(char **)_refptr_g_touchesPtr = **(char **)_refptr_g_touchesPtr + '\x01';
    *(char *)(*(longlong *)_refptr_g_touchesPtr + 1) =
         *(char *)(*(longlong *)_refptr_g_touchesPtr + 1) + '\x01';
  }
  return;
}


// ============================================================================
// Function: TZ_UnitToTouchNormal @ 6ba8126f
// ============================================================================

void TZ_UnitToTouchNormal(longlong param_1)

{
  int iVar1;
  
  iVar1 = *(int *)(param_1 + 8);
  if (iVar1 == 3) {
    TZ_MFProcess(param_1);
  }
  else if (iVar1 == 5) {
    TZ_RegisterTouch(param_1);
  }
  else if (iVar1 == 2) {
    TZ_RegistForShapeFF(param_1);
  }
  else {
    TZ_RegisterTouch(param_1);
  }
  return;
}


// ============================================================================
// Function: TZ_ExceptionProcess @ 6ba812c2
// ============================================================================

void TZ_ExceptionProcess(longlong param_1)

{
  if ((*(uint *)(param_1 + 0x350) & 10) == 0) {
    if ((*(uint *)(param_1 + 0x350) & 0x8380) == 0) {
      if ((*(uint *)(param_1 + 0x350) & 0x40) != 0) {
        TZ_RegisterTouch(param_1);
      }
    }
    else {
      TZ_RegisterBasedOnSubTZ(param_1,0);
    }
  }
  else {
    if (*(int *)(param_1 + 0x350) == 2) {
      *(undefined4 *)(param_1 + 0x350) = 0;
    }
    TZ_UnitToTouchNormal(param_1);
  }
  return;
}


// ============================================================================
// Function: TZ_CentroidGripRatio @ 6ba7f2f0
// ============================================================================

uint TZ_CentroidGripRatio(int param_1,uint param_2)

{
  uint local_c;
  
                    /* 0x3f2f0  2360  TZ_CentroidGripRatio */
  if ((param_2 == 0) || (param_1 == 0)) {
    if ((param_2 == 0) && (param_1 == 0)) {
      local_c = 0;
    }
    else if (param_2 == 0) {
      local_c = 0xffff;
    }
    else {
      local_c = 0;
    }
  }
  else {
    local_c = (uint)(param_1 << 4) / param_2;
  }
  if (0xffff < local_c) {
    local_c = 0xffff;
  }
  return local_c;
}


// ============================================================================
// Function: TZ_UnitMaxMinInit @ 6ba87d7e
// ============================================================================

void TZ_UnitMaxMinInit(undefined2 *param_1,undefined2 param_2)

{
                    /* 0x47d7e  2450  TZ_UnitMaxMinInit */
  *(undefined1 *)((longlong)param_1 + 0x33) = 0xff;
  *(undefined1 *)((longlong)param_1 + 0x35) = 0xff;
  *(undefined1 *)((longlong)param_1 + 0x37) = 0x7f;
  *(undefined1 *)((longlong)param_1 + 0x39) = 0x7f;
  *(undefined1 *)((longlong)param_1 + 0x3b) = 0x7f;
  *(undefined1 *)((longlong)param_1 + 0x3d) = 0x7f;
  *(undefined1 *)(param_1 + 0x1a) = 0;
  *(undefined1 *)(param_1 + 0x1b) = 0;
  *(undefined1 *)(param_1 + 0x1c) = 0x80;
  *(undefined1 *)(param_1 + 0x1d) = 0x80;
  *(undefined1 *)(param_1 + 0x1e) = 0x80;
  *(undefined1 *)(param_1 + 0x1f) = 0x80;
  param_1[0x16e] = 0x8000;
  param_1[0x16f] = 0x8000;
  param_1[0x170] = 0x8000;
  param_1[0x171] = 0x8000;
  *param_1 = param_2;
  *(undefined1 *)((longlong)param_1 + 0x35f) = 0;
  *(undefined1 *)(param_1 + 0x1b0) = 0;
  param_1[0x1b1] = 0x7fff;
  param_1[0x1b2] = 0x7fff;
  param_1[0x19b] = 0x8000;
  param_1[0x19c] = 0x7fff;
  param_1[0x19d] = 0x7fff;
  return;
}


// ============================================================================
// Function: TZ_TraversalNeighborSimple @ 6ba89752
// ============================================================================

void TZ_TraversalNeighborSimple(longlong param_1,ushort param_2,longlong param_3,short param_4)

{
  char cVar1;
  char local_38 [16];
  char local_28 [10];
  short local_1e;
  ushort local_1c;
  ushort local_1a;
  byte local_18;
  byte local_17;
  char local_16;
  char local_15;
  uint local_14;
  uint local_10;
  uint local_c;
  
  local_28[0] = '\0';
  local_28[1] = 1;
  local_28[2] = 1;
  local_28[3] = 1;
  local_28[4] = 0;
  local_28[5] = 0xff;
  local_28[6] = 0xff;
  local_28[7] = 0xff;
  local_28[8] = 0;
  local_38[0] = '\0';
  local_38[1] = 0xff;
  local_38[2] = 0;
  local_38[3] = 1;
  local_38[4] = 1;
  local_38[5] = 1;
  local_38[6] = 0;
  local_38[7] = 0xff;
  local_38[8] = 0xff;
  local_10 = (uint)_refptr_g_tsaPrmtFlash->bCols;
  local_14 = (uint)_refptr_g_tsaPrmtFlash->bRows;
  local_15 = (char)((ulonglong)param_2 %
                   (ulonglong)(longlong)(int)(uint)_refptr_g_tsaPrmtFlash->bCols);
  local_16 = (char)((ulonglong)param_2 /
                   (ulonglong)(longlong)(int)(uint)_refptr_g_tsaPrmtFlash->bCols);
  for (local_c = 0; local_c < 9; local_c = local_c + 1) {
    local_17 = local_15 + local_28[local_c];
    local_18 = local_16 + local_38[local_c];
    if (((int)(uint)local_17 < (int)local_10) && ((int)(uint)local_18 < (int)local_14)) {
      local_1a = (ushort)local_17 + (short)local_10 * (ushort)local_18;
      local_1c = *(ushort *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)local_1a * 2);
      local_1e = (local_1c ^ (short)local_1c >> 0x1f) - ((short)local_1c >> 0xf);
      if (*(char *)(param_3 + (ulonglong)local_1a) == '\0') {
        if (local_1e < param_4) {
          if (*(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)local_1a * 2) < 0) {
            cVar1 = -1;
          }
          else {
            cVar1 = '\x01';
          }
          *(char *)((ulonglong)local_1a + param_3) =
               (*(char *)(*(longlong *)_refptr_g_ptrAccess + 9) + '\x02') * cVar1;
          if (0 < (short)local_1c) {
            TZ_UpdateEdgeInfo(param_1,(int)(short)local_1c,local_17,local_18,0);
          }
        }
        else {
          if (*(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)local_1a * 2) < 0) {
            cVar1 = -1;
          }
          else {
            cVar1 = '\x01';
          }
          *(char *)((ulonglong)local_1a + param_3) =
               (*(char *)(*(longlong *)_refptr_g_ptrAccess + 9) + '\x01') * cVar1;
          if (((*_refptr_g_tsaStaticPtr)->dwProcessingState & PROC_MS_ACTIVE) != 0) {
            if (*(byte *)(param_1 + 0x32a) <
                *(byte *)((ulonglong)local_1a + *(longlong *)_refptr_g_tsaBufGripRegion)) {
              *(undefined1 *)(param_1 + 0x32a) =
                   *(undefined1 *)((ulonglong)local_1a + *(longlong *)_refptr_g_tsaBufGripRegion);
            }
            if (*(short *)(param_1 + 0x336) <
                *(short *)(g_tsaBufMinSigOff + (ulonglong)local_1a * 2)) {
              *(undefined2 *)(param_1 + 0x336) =
                   *(undefined2 *)(g_tsaBufMinSigOff + (ulonglong)local_1a * 2);
            }
            if (*(short *)(g_tsaBufTimeRpt + (ulonglong)local_1a * 2) < *(short *)(param_1 + 0x33a))
            {
              *(undefined2 *)(param_1 + 0x33a) =
                   *(undefined2 *)(g_tsaBufTimeRpt + (ulonglong)local_1a * 2);
            }
          }
          if ((short)local_1c < 1) {
            *(short *)(param_1 + 0xe) = *(short *)(param_1 + 0xe) + 1;
          }
          else {
            *(short *)(param_1 + 0xc) = *(short *)(param_1 + 0xc) + 1;
            TZ_UpdateEdgeInfo(param_1,(int)(short)local_1c,local_17,local_18,1);
            TZ_UpdateDimMaxMin(param_1,local_17,local_18);
          }
          if (local_c != 0) {
            PosnSpecial_Put(local_1a);
          }
        }
        *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + (int)(short)local_1c;
        if (0 < local_1e) {
          TZ_UpdateMValue(param_1,(int)local_1e,local_17,local_18);
        }
        if (0 < (short)local_1c) {
          TZ_UpdateWeightedSumSimple(param_1,(int)(short)local_1c,local_17,local_18);
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: TZ_GetSigTholdForSideTouch @ 6ba8a480
// ============================================================================

undefined2 TZ_GetSigTholdForSideTouch(byte param_1)

{
  ushort uVar1;
  uint local_c;
  
  if ((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold <
      *(short *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)param_1 * 0x14 + 0x18)) {
    uVar1 = _refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold;
  }
  else {
    uVar1 = *(ushort *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)param_1 * 0x14 + 0x18);
  }
  local_c = (uint)(short)uVar1;
  return (short)(local_c >> 1);
}


// ============================================================================
// Function: TZ_GetSigTholdForAFT @ 6ba8a51f
// ============================================================================

undefined2 TZ_GetSigTholdForAFT(byte param_1)

{
  ushort uVar1;
  int iVar2;
  uint local_10;
  undefined2 local_a;
  
  if ((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold <
      *(short *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)param_1 * 0x14 + 0x18)) {
    uVar1 = _refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold;
  }
  else {
    uVar1 = *(ushort *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)param_1 * 0x14 + 0x18);
  }
  local_10 = (uint)(short)uVar1;
  iVar2 = Peak_needUDFPLargeFingerWorkaround();
  if (iVar2 == 0) {
    local_a = (undefined2)(local_10 >> 2);
  }
  else {
    local_a = (undefined2)(local_10 >> 1);
  }
  return local_a;
}


// ============================================================================
// Function: TZ_GetSigTholdForCharger @ 6ba8a5d3
// ============================================================================

short TZ_GetSigTholdForCharger(byte param_1)

{
  short local_a;
  
  if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3) {
    local_a = (short)((uint)(byte)_refptr_g_tsaPrmtFlash->field_0x81f *
                      (int)*(short *)(*_refptr_g_peaksPtr +
                                     (longlong)(int)(uint)param_1 * 0x14 + 0x18) >> 7);
  }
  else {
    local_a = *(short *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)param_1 * 0x14 + 0x18) / 2;
  }
  return local_a;
}


// ============================================================================
// Function: TZ_GetSigTholdCommon @ 6ba8a7e4
// ============================================================================

undefined2 TZ_GetSigTholdCommon(undefined8 param_1,byte param_2)

{
  ushort uVar1;
  char cVar2;
  int local_10;
  undefined2 local_a;
  
  if ((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold <
      *(short *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)param_2 * 0x14 + 0x18)) {
    uVar1 = _refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold;
  }
  else {
    uVar1 = *(ushort *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)param_2 * 0x14 + 0x18);
  }
  local_10 = (int)(short)uVar1;
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x1000) == 0) {
    cVar2 = Peak_IsOnOuterEdge(param_2);
    if ((cVar2 != '\0') && (cVar2 = TZ_IsSD(param_1), cVar2 == '\0')) {
      return (short)((uint)(byte)_refptr_g_tsaPrmtFlash->field_0x81f * local_10 >> 7);
    }
    local_a = (undefined2)((uint)(byte)_refptr_g_tsaPrmtFlash->field_0x81e * local_10 >> 7);
  }
  else {
    local_a = (undefined2)((uint)(byte)_refptr_g_tsaPrmtFlash->field_0x81f * local_10 >> 7);
  }
  return local_a;
}


// ============================================================================
// Function: TZ_GetEdgeWidth @ 6ba875ca
// ============================================================================

void TZ_GetEdgeWidth(longlong param_1,short param_2)

{
  int local_1c;
  int local_18;
  int local_14;
  int local_10;
  int local_c;
  
                    /* 0x475ca  2378  TZ_GetEdgeWidth */
  local_c = (int)*(char *)(param_1 + 0x37);
  local_1c = (int)*(char *)(param_1 + 0x38);
  local_14 = (int)*(char *)(param_1 + 0x39);
  local_18 = (int)*(char *)(param_1 + 0x3a);
  if ((local_1c == -0x80) || (local_c == 0x7f)) {
    *(undefined1 *)(param_1 + 0x43) = 0;
  }
  else {
    local_10 = local_1c;
    while ((local_1c < (int)(uint)_refptr_g_tsaPrmtFlash->bCols &&
           (param_2 <=
            *(short *)((ulonglong)
                       ((uint)_refptr_g_tsaPrmtFlash->bCols * (uint)*(byte *)(param_1 + 0x40) +
                        local_1c & 0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif)))) {
      local_10 = local_1c;
      local_1c = local_1c + 1;
    }
    local_1c = local_c;
    while ((-1 < local_1c &&
           (param_2 <=
            *(short *)((ulonglong)
                       ((uint)_refptr_g_tsaPrmtFlash->bCols * (uint)*(byte *)(param_1 + 0x3f) +
                        local_1c & 0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif)))) {
      local_c = local_1c;
      local_1c = local_1c + -1;
    }
    *(char *)(param_1 + 0x43) = ((char)local_10 - (char)local_c) + '\x01';
  }
  if ((local_18 == -0x80) || (local_14 == 0x7f)) {
    *(undefined1 *)(param_1 + 0x44) = 0;
  }
  else {
    local_1c = local_18;
    while ((local_1c < (int)(uint)_refptr_g_tsaPrmtFlash->bRows &&
           (param_2 <=
            *(short *)((ulonglong)
                       ((uint)(ushort)(short)*(char *)(param_1 + 0x42) +
                        local_1c * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
                      *(longlong *)_refptr_g_tsaBufDif)))) {
      local_18 = local_1c;
      local_1c = local_1c + 1;
    }
    local_1c = local_14;
    while ((-1 < local_1c &&
           (param_2 <=
            *(short *)((ulonglong)
                       ((uint)(ushort)(short)*(char *)(param_1 + 0x41) +
                        local_1c * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
                      *(longlong *)_refptr_g_tsaBufDif)))) {
      local_14 = local_1c;
      local_1c = local_1c + -1;
    }
    *(char *)(param_1 + 0x44) = ((char)local_18 - (char)local_14) + '\x01';
  }
  *(char *)(param_1 + 0x45) = *(char *)(param_1 + 0x43) + *(char *)(param_1 + 0x44);
  return;
}


// ============================================================================
// Function: TZ_GetEdgeSigThold @ 6ba87e84
// ============================================================================

short TZ_GetEdgeSigThold(short param_1,short param_2)

{
  short local_a;
  
  local_a = _refptr_g_tsaPrpt->nSPeakPositive;
  if (local_a < 0) {
    local_a = local_a + 3;
  }
  local_a = local_a >> 2;
  if (param_1 / 2 < local_a) {
    local_a = param_1 / 2;
  }
  if (local_a < param_2) {
    local_a = param_2;
  }
  return local_a;
}


// ============================================================================
// Function: TZ_TraversalNeighbor @ 6ba88709
// ============================================================================

void TZ_TraversalNeighbor
               (longlong param_1,ushort param_2,longlong param_3,short param_4,undefined8 param_5,
               short param_6,undefined2 param_7)

{
  char cVar1;
  uint uVar2;
  int iVar3;
  char local_68 [16];
  char local_58 [24];
  int local_40;
  int local_3c;
  ushort local_38;
  ushort local_36;
  ushort local_34;
  byte local_31;
  byte local_30;
  byte local_2f;
  char local_2e;
  char local_2d;
  uint local_2c;
  uint local_28;
  int local_24;
  int local_20;
  char local_19;
  uint local_18;
  int local_14;
  int local_10;
  uint local_c;
  
  local_58[0] = '\0';
  local_58[1] = 1;
  local_58[2] = 1;
  local_58[3] = 1;
  local_58[4] = 0;
  local_58[5] = 0xff;
  local_58[6] = 0xff;
  local_58[7] = 0xff;
  local_58[8] = 0;
  local_68[0] = '\0';
  local_68[1] = 0xff;
  local_68[2] = 0;
  local_68[3] = 1;
  local_68[4] = 1;
  local_68[5] = 1;
  local_68[6] = 0;
  local_68[7] = 0xff;
  local_68[8] = 0xff;
  local_28 = (uint)_refptr_g_tsaPrmtFlash->bCols;
  local_2c = (uint)_refptr_g_tsaPrmtFlash->bRows;
  local_c = _refptr_g_tsaPrmtFlash->dwGripFlags & 0x40;
  local_2d = (char)((ulonglong)param_2 %
                   (ulonglong)(longlong)(int)(uint)_refptr_g_tsaPrmtFlash->bCols);
  local_2e = (char)((ulonglong)param_2 /
                   (ulonglong)(longlong)(int)(uint)_refptr_g_tsaPrmtFlash->bCols);
  local_10 = 0;
  local_14 = 0;
  if ((_refptr_g_tsaPrmtFlash->dwGripFlags & 0x400) != 0) {
    local_10 = (int)*(short *)&_refptr_g_tsaPrmtFlash[1].field_0x44;
    local_14 = (int)*(short *)&_refptr_g_tsaPrmtFlash[1].field_0x46;
  }
  for (local_18 = 0; local_18 < 9; local_18 = local_18 + 1) {
    local_2f = local_2d + local_58[local_18];
    local_30 = local_2e + local_68[local_18];
    local_31 = (byte)local_18 & 1;
    if (((int)(uint)local_2f < (int)local_28) && ((int)(uint)local_30 < (int)local_2c)) {
      local_34 = (ushort)local_2f + (short)local_28 * (ushort)local_30;
      local_36 = *(ushort *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)local_34 * 2);
      *(ushort *)((ulonglong)local_34 * 2 + g_tzBuffer) = local_36;
      if (((*_refptr_g_tsaStaticPtr)->dwProcessingState & PROC_MS_ACTIVE) != 0) {
        if (((int)(uint)local_2f < (int)(_refptr_g_tsaPrmtFlash->bCols - 2)) && (1 < local_2f)) {
          local_c = 0;
        }
        else {
          local_c = 1;
        }
        if (local_18 != 0) {
          PosnSpecial_Record2((int)(short)local_36,(int)param_4);
        }
      }
      if (local_c == 0) {
        local_38 = (local_36 ^ (short)local_36 >> 0xf) - ((short)local_36 >> 0xf);
      }
      else {
        local_38 = local_36;
      }
      if (*(char *)(param_3 + (ulonglong)local_34) == '\0') {
        if (((*_refptr_g_tsaStaticPtr)->dwProcessingState & PROC_MS_ACTIVE) != 0) {
          if ((*(short *)(_refptr_g_tsaPrmtDynamic + 8) * 3 < (short)local_36 * 4) ||
             (param_4 <= (short)local_38)) {
            *(undefined *)(*(longlong *)_refptr_g_tsaBufTzZone + (ulonglong)local_34) =
                 _refptr_g_tzAccess[9];
            cVar1 = TZ_IsIDMerged(param_1,local_2f,local_30);
            if (cVar1 != '\0') {
              *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x200000;
            }
            if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x20) != 0) {
              cVar1 = TZ_IsEdgeMerged(param_1,local_2f,local_30);
              if (cVar1 != '\0') {
                *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x100000;
              }
              if ((*(char *)((ulonglong)local_34 + *(longlong *)_refptr_g_tsaBufGripStage) == '\x01'
                  ) && (*(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x2000000,
                       *(short *)(*(longlong *)_refptr_g_tsaBufGripTime + (ulonglong)local_34 * 2) <
                       *(short *)(param_1 + 0x364))) {
                *(undefined2 *)(param_1 + 0x364) =
                     *(undefined2 *)
                      (*(longlong *)_refptr_g_tsaBufGripTime + (ulonglong)local_34 * 2);
              }
            }
          }
          if (*(short *)(*(longlong *)_refptr_g_tsaBufTimeHand + (ulonglong)local_34 * 2) <
              *(short *)(param_1 + 0x362)) {
            *(undefined2 *)(param_1 + 0x362) =
                 *(undefined2 *)(*(longlong *)_refptr_g_tsaBufTimeHand + (ulonglong)local_34 * 2);
          }
        }
        iVar3 = TZ_IsGripWall(local_2f,local_28,local_34,param_7,(int)(short)local_36);
        if (iVar3 == 0) {
          local_19 = '\0';
          if ((((*_refptr_g_tsaStaticPtr)->dwProcessingState & PROC_MS_ACTIVE) != 0) &&
             (*(char *)((ulonglong)local_34 + *(longlong *)_refptr_g_tsaBufGripStage) == '\x01')) {
            local_19 = '\x01';
          }
          if (((short)local_38 < param_4) && (local_19 == '\0')) {
            if (*(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)local_34 * 2) < 0) {
              cVar1 = -1;
            }
            else {
              cVar1 = '\x01';
            }
            *(char *)((ulonglong)local_34 + param_3) =
                 (*(char *)(*(longlong *)_refptr_g_ptrAccess + 9) + '\x02') * cVar1;
            if (0 < (short)local_36) {
              TZ_UpdateEdgeInfo(param_1,(int)(short)local_36,local_2f,local_30,0);
            }
            *(int *)(_refptr_g_tsaBufSelf + ((longlong)(int)(uint)local_2f + 0x640) * 4) =
                 *(int *)(_refptr_g_tsaBufSelf + ((longlong)(int)(uint)local_2f + 0x640) * 4) + 1;
            *(int *)(_refptr_g_tsaBufSelf + ((longlong)(int)(local_28 + local_30) + 0x640) * 4) =
                 *(int *)(_refptr_g_tsaBufSelf + ((longlong)(int)(local_28 + local_30) + 0x640) * 4)
                 + 1;
          }
          else {
            if (*(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)local_34 * 2) < 0) {
              cVar1 = -1;
            }
            else {
              cVar1 = '\x01';
            }
            *(char *)((ulonglong)local_34 + param_3) =
                 (*(char *)(*(longlong *)_refptr_g_ptrAccess + 9) + '\x01') * cVar1;
            if (((*_refptr_g_tsaStaticPtr)->dwProcessingState & PROC_MS_ACTIVE) != 0) {
              if (*(byte *)(param_1 + 0x32a) <
                  *(byte *)((ulonglong)local_34 + *(longlong *)_refptr_g_tsaBufGripRegion)) {
                *(undefined1 *)(param_1 + 0x32a) =
                     *(undefined1 *)((ulonglong)local_34 + *(longlong *)_refptr_g_tsaBufGripRegion);
              }
              if (*(short *)(param_1 + 0x336) <
                  *(short *)(g_tsaBufMinSigOff + (ulonglong)local_34 * 2)) {
                *(undefined2 *)(param_1 + 0x336) =
                     *(undefined2 *)(g_tsaBufMinSigOff + (ulonglong)local_34 * 2);
              }
              if (*(short *)(g_tsaBufTimeRpt + (ulonglong)local_34 * 2) <
                  *(short *)(param_1 + 0x33a)) {
                *(undefined2 *)(param_1 + 0x33a) =
                     *(undefined2 *)(g_tsaBufTimeRpt + (ulonglong)local_34 * 2);
              }
            }
            if ((short)local_36 < 1) {
              *(short *)(param_1 + 0xe) = *(short *)(param_1 + 0xe) + 1;
            }
            else {
              *(short *)(param_1 + 0xc) = *(short *)(param_1 + 0xc) + 1;
              TZ_UpdateEdgeInfo(param_1,(int)(short)local_36,local_2f,local_30,7);
            }
            TZ_UpdateDimMaxMin(param_1,local_2f,local_30);
            if (local_18 != 0) {
              PosnSpecial_Record((int)(short)local_36,(int)param_6,local_31);
              PosnSpecial_Put(local_34);
            }
          }
          *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + (int)(short)local_36;
          *(int *)(_refptr_g_tsaBufSelf + ((longlong)(int)(uint)local_2f + 0x500) * 4) =
               *(int *)(_refptr_g_tsaBufSelf + ((longlong)(int)(uint)local_2f + 0x500) * 4) +
               (int)(short)local_36;
          *(int *)(_refptr_g_tsaBufSelf + ((longlong)(int)(local_30 + local_28) + 0x500) * 4) =
               *(int *)(_refptr_g_tsaBufSelf + ((longlong)(int)(local_30 + local_28) + 0x500) * 4) +
               (int)(short)local_36;
          if (0 < (short)local_38) {
            TZ_UpdateMValue(param_1,(int)(short)local_38,local_2f,local_30);
          }
          if ((short)local_36 < 1) {
            if ((short)local_36 < *(short *)(param_1 + 0x2c)) {
              *(ushort *)(param_1 + 0x2c) = local_36;
              *(byte *)(param_1 + 0x30) = local_2f;
              *(byte *)(param_1 + 0x31) = local_30;
            }
            *(int *)(param_1 + 0x28) = *(int *)(param_1 + 0x28) + (int)(short)local_36;
          }
          else {
            if (*(short *)(param_1 + 0x2e) < (short)local_36) {
              *(ushort *)(param_1 + 0x2e) = local_36;
            }
            if (*(char *)((ulonglong)local_34 + *(longlong *)_refptr_g_tsaBufGripRegion) != '\0') {
              *(int *)(param_1 + 0x24) = *(int *)(param_1 + 0x24) + (int)(short)local_36;
            }
            local_20 = (short)local_36 - local_10;
            local_24 = (short)local_36 - local_14;
            if (local_20 < 0) {
              local_20 = 0;
            }
            if (local_24 < 0) {
              local_24 = 0;
            }
            *(int *)(param_1 + 0x1c) = *(int *)(param_1 + 0x1c) + local_20;
            *(int *)(param_1 + 0x20) = *(int *)(param_1 + 0x20) + local_24;
            *(int *)(param_1 + 0x18) = *(int *)(param_1 + 0x18) + (int)(short)local_36;
            iVar3 = ((int)*(char *)((ulonglong)
                                    ((uint)local_2f +
                                     (uint)local_30 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)
                                   + *(longlong *)_refptr_g_tpSensorDim1Offset) +
                    (uint)local_2f * 0x80) * local_20;
            if (iVar3 < 0) {
              iVar3 = iVar3 + 0x7f;
            }
            *(int *)(param_1 + 0x2c4) = *(int *)(param_1 + 0x2c4) + (iVar3 >> 7);
            local_40 = ((int)*(char *)((ulonglong)
                                       ((uint)local_2f +
                                        (uint)local_30 * (uint)_refptr_g_tsaPrmtFlash->bCols &
                                       0xffff) + *(longlong *)_refptr_g_tpSensorDim2Offset) +
                       (uint)local_30 * 0x80) * local_24;
            if (local_40 < 0) {
              local_40 = local_40 + 0x7f;
            }
            local_40 = local_40 >> 7;
            *(int *)(param_1 + 0x2c8) = *(int *)(param_1 + 0x2c8) + local_40;
            TouchZone_InnerCornerHandler(param_1,local_2f,local_30);
          }
        }
      }
      else {
        if ((local_18 != 0) && (param_4 <= (short)local_38)) {
          PosnSpecial_Record((int)(short)local_36,(int)param_6,local_31);
        }
        uVar2 = (int)*(char *)((ulonglong)local_34 + *(longlong *)_refptr_g_tsaBufTz) >> 0x1f;
        local_3c = ((int)*(char *)((ulonglong)local_34 + *(longlong *)_refptr_g_tsaBufTz) ^ uVar2) -
                   uVar2;
        if ((local_3c != 0) &&
           (((int)((byte)_refptr_g_tzAccess[9] + 8) <= local_3c ||
            (local_3c < (int)(uint)(byte)_refptr_g_tzAccess[9])))) {
          *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x4000;
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: TZ_IsValid @ 6ba8aaca
// ============================================================================

undefined8 TZ_IsValid(longlong param_1)

{
  undefined8 uVar1;
  
                    /* 0x4aaca  2425  TZ_IsValid */
  if (*(char *)(param_1 + 0x2b2) == '\0') {
    uVar1 = 0;
  }
  else if (((*_refptr_g_tsaStaticPtr)->dwProcessingState & PROC_MS_ACTIVE) == 0) {
    uVar1 = 1;
  }
  else if (*(char *)(param_1 + 0x2b4) == '\0') {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: TZ_GetPrevTouch @ 6ba8ab23
// ============================================================================

void TZ_GetPrevTouch(longlong param_1)

{
  byte bVar1;
  undefined1 uVar2;
  byte local_9;
  
                    /* 0x4ab23  2390  TZ_GetPrevTouch */
  *(undefined1 *)(param_1 + 0x324) = 0x14;
  if ((*_refptr_g_peaksPtr)[(longlong)(int)(uint)*(byte *)(param_1 + 0x2b6) * 0x14 + 0x14].
      bPeakCount == '\0') {
    for (local_9 = 0; local_9 < *(byte *)(param_1 + 0x2b2); local_9 = local_9 + 1) {
      bVar1 = *(byte *)(param_1 + 0x2b9 + (longlong)(int)(uint)local_9);
      if ((*_refptr_g_peaksPtr)[(longlong)(int)(uint)bVar1 * 0x14 + 0x14].bPeakCount != '\0') {
        uVar2 = PrevTouch_GetIdxBasedOnID
                          ((*_refptr_g_peaksPtr)[(longlong)(int)(uint)bVar1 * 0x14 + 0x14].
                           bPeakCount);
        *(undefined1 *)(param_1 + 0x324) = uVar2;
        break;
      }
    }
  }
  else {
    uVar2 = PrevTouch_GetIdxBasedOnID
                      ((*_refptr_g_peaksPtr)
                       [(longlong)(int)(uint)*(byte *)(param_1 + 0x2b6) * 0x14 + 0x14].bPeakCount);
    *(undefined1 *)(param_1 + 0x324) = uVar2;
  }
  if ((*(char *)(param_1 + 0x324) == '\x14') && (*(char *)(param_1 + 0x35f) != '\0')) {
    uVar2 = PrevTouch_GetIdxBasedOnID(*(undefined1 *)(param_1 + 0x35f));
    *(undefined1 *)(param_1 + 0x324) = uVar2;
  }
  return;
}


// ============================================================================
// Function: TZ_PeakInfoRetrieval @ 6ba8acb6
// ============================================================================

void TZ_PeakInfoRetrieval(undefined8 param_1)

{
  longlong lVar1;
  byte local_9;
  
                    /* 0x4acb6  2437  TZ_PeakInfoRetrieval */
  lVar1 = *(longlong *)_refptr_g_tzAccess;
  for (local_9 = 0; local_9 < (*_refptr_g_peaksPtr)->bPeakCount; local_9 = local_9 + 1) {
    if ((g_pkAccessedFlag & 1 << (local_9 & 0x1f)) == 0) {
      if (*(char *)(lVar1 + (ulonglong)
                            (ushort)((ushort)(*_refptr_g_peaksPtr)
                                             [(longlong)(int)(uint)local_9 * 0x14 + 0x15].bPeakCount
                                    + (ushort)(*_refptr_g_peaksPtr)
                                              [(longlong)(int)(uint)local_9 * 0x14 + 0x16].
                                              bPeakCount * (ushort)_refptr_g_tsaPrmtFlash->bCols))
          != '\0') {
        g_pkAccessedFlag = 1 << (local_9 & 0x1f) | g_pkAccessedFlag;
        TZ_PeakRegister(param_1,local_9);
      }
    }
  }
  TZ_GetMaxMinPkIdx(param_1);
  return;
}


// ============================================================================
// Function: TZ_BeforeSDCompensationPeakInfoRetrieval @ 6ba8adec
// ============================================================================

void TZ_BeforeSDCompensationPeakInfoRetrieval(longlong param_1)

{
  longlong lVar1;
  short local_c;
  byte local_9;
  
                    /* 0x4adec  2357  TZ_BeforeSDCompensationPeakInfoRetrieval */
  local_c = -0x8000;
  lVar1 = *(longlong *)_refptr_g_tzAccess;
  *(undefined1 *)(param_1 + 0x2b4) = 0;
  for (local_9 = 0; local_9 < (byte)*_refptr_g_peaksBeforeSwitchDifBuffer; local_9 = local_9 + 1) {
    if (((g_pkBeforeSDCAccessedFlag & 1 << (local_9 & 0x1f)) == 0) &&
       (*(char *)(lVar1 + (ulonglong)
                          (ushort)((ushort)(byte)_refptr_g_peaksBeforeSwitchDifBuffer
                                                 [(longlong)(int)(uint)local_9 * 0x14 + 0x15] +
                                  (ushort)(byte)_refptr_g_peaksBeforeSwitchDifBuffer
                                                [(longlong)(int)(uint)local_9 * 0x14 + 0x16] *
                                  (ushort)_refptr_g_tsaPrmtFlash->bCols)) != '\0')) {
      g_pkBeforeSDCAccessedFlag = 1 << (local_9 & 0x1f) | g_pkBeforeSDCAccessedFlag;
      *(char *)(param_1 + 0x2b4) = *(char *)(param_1 + 0x2b4) + '\x01';
      if (local_c < *(short *)(_refptr_g_peaksBeforeSwitchDifBuffer +
                              (longlong)(int)(uint)local_9 * 0x14 + 0x18)) {
        local_c = *(short *)(_refptr_g_peaksBeforeSwitchDifBuffer +
                            (longlong)(int)(uint)local_9 * 0x14 + 0x18);
        *(byte *)(param_1 + 0x2b5) = local_9;
      }
    }
  }
  return;
}


// ============================================================================
// Function: TZ_GetProperty @ 6ba8b126
// ============================================================================

void TZ_GetProperty(longlong param_1)

{
                    /* 0x4b126  2391  TZ_GetProperty */
  TPSensor_IsBrokenSensorInTouchZone(param_1);
  TZ_CentroidCore(param_1);
  TZ_GetEdgeTouchedFlag(param_1);
  TZ_GetHTIValidFlag(param_1);
  TZ_GetAxis(param_1);
  TZ_GetShapeRatio(param_1);
  TZ_GetGripRatio(param_1);
  TZ_GetContour(param_1);
  if (((*_refptr_g_tsaStaticPtr)->dwProcessingState & PROC_MS_ACTIVE) != 0) {
    if ((_refptr_g_tsaPrmtFlash->dwGripFlags & 0x20) == 0) {
      Exception_ProcessTouchZone(param_1);
    }
    if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x20) != 0) {
      TZ_CheckToSeparateZone(param_1);
    }
    SubTZ_Process(param_1);
    TZ_GetShapeFlag(param_1);
    TZ_GetSurroundingStatusFlag(param_1);
    TZ_GetCMFFlag(param_1);
    if ((*(uint *)(param_1 + 4) & 0x1000000) == 0) {
      TZ_VPProcess(param_1);
      if ((*(uint *)(param_1 + 4) & 0x40) != 0) {
        TZ_VirtualPeakInfoRetrieval();
      }
    }
  }
  TZ_GetType(param_1);
  return;
}


// ============================================================================
// Function: TZ_GetDistanceMapPtr @ 6ba8ce17
// ============================================================================

undefined * TZ_GetDistanceMapPtr(void)

{
                    /* 0x4ce17  2376  TZ_GetDistanceMapPtr */
  return _refptr_g_tsaDistanceMapBuf;
}


// ============================================================================
// Function: TZ_CentroidEdgeFlag @ 6ba7f3cb
// ============================================================================

void TZ_CentroidEdgeFlag(longlong param_1,byte param_2)

{
                    /* 0x3f3cb  2359  TZ_CentroidEdgeFlag */
  if ((*(char *)(param_1 + 0x34) == _refptr_g_tsaPrmtFlash->field_0x91) &&
     ((int)(((byte)_refptr_g_tsaPrmtFlash->field_0x91 - 1) * 0x100) <
      (int)(uint)*(ushort *)(param_1 + 0x2ae))) {
    *(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x204) =
         *(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x204)
         | 2;
  }
  if ((*(char *)(param_1 + 0x33) == _refptr_g_tsaPrmtFlash->field_0x90) &&
     ((uint)*(ushort *)(param_1 + 0x2ae) < ((byte)_refptr_g_tsaPrmtFlash->field_0x90 + 2) * 0x100))
  {
    *(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x204) =
         *(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x204)
         | 1;
  }
  if ((*(char *)(param_1 + 0x36) == _refptr_g_tsaPrmtFlash->field_0x93) &&
     ((int)(((byte)_refptr_g_tsaPrmtFlash->field_0x93 - 1) * 0x100) <
      (int)(uint)*(ushort *)(param_1 + 0x2b0))) {
    *(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x204) =
         *(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x204)
         | 8;
  }
  if ((*(char *)(param_1 + 0x35) == _refptr_g_tsaPrmtFlash->field_0x92) &&
     ((uint)*(ushort *)(param_1 + 0x2b0) < ((byte)_refptr_g_tsaPrmtFlash->field_0x92 + 2) * 0x100))
  {
    *(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x204) =
         *(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x204)
         | 4;
  }
  return;
}


// ============================================================================
// Function: TZ_UpdateGripRatio @ 6ba7faa8
// ============================================================================

void TZ_UpdateGripRatio(byte param_1,longlong param_2)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x24]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x30];
  longlong lVar1;
  undefined1 uVar2;
  
  lVar1 = *(longlong *)_refptr_g_touchesPtr;
  uVar2 = TZ_CalcGripRatio(*(undefined4 *)(_refptr_g_tsaPrmtDynamic + 0x84),param_2,&(*(undefined4 *)(_stk + 0xc)),
                           *(undefined1 *)(param_2 + 0x334),*(undefined1 *)(param_2 + 0x335));
  *(undefined1 *)((longlong)(int)(uint)param_1 * 0x168 + lVar1 + 0x1c8) = uVar2;
  (*(ushort *)(_stk + 0x16)) = (ushort)*(byte *)(param_2 + 0x334);
  (*(ushort *)(_stk + 0x14)) = (ushort)*(byte *)(param_2 + 0x335);
  if (*(short *)(param_2 + 0x2dc) <=
      *(short *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)*(byte *)(param_2 + 0x2b6) * 0x14 + 0x18
                ) / 2) {
    (*(ushort *)(_stk + 0x16)) = 0;
  }
  if (*(short *)(param_2 + 0x2e0) <=
      *(short *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)*(byte *)(param_2 + 0x2b6) * 0x14 + 0x18
                ) / 2) {
    (*(ushort *)(_stk + 0x14)) = 0;
  }
  (*(undefined4 *)(_stk + 0x10)) = 0xc;
  if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x80) != 0) {
    (*(undefined4 *)(_stk + 0x10)) = 0xf;
  }
  lVar1 = *(longlong *)_refptr_g_touchesPtr;
  uVar2 = TZ_CalcGripRatio((*(undefined4 *)(_stk + 0x10)),param_2,&(*(undefined4 *)(_stk + 0xc)),(*(ushort *)(_stk + 0x16)),(*(ushort *)(_stk + 0x14)));
  *(undefined1 *)((longlong)(int)(uint)param_1 * 0x168 + lVar1 + 0x198) = uVar2;
  *(undefined4 *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1c4)
       = (*(undefined4 *)(_stk + 0xc));
  return;
}


// ============================================================================
// Function: TZ_StylusXYCorrection @ 6ba7fc89
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int TZ_StylusXYCorrection(ushort param_1)

{
  double dVar1;
  
  dVar1 = sin((double)((DAT_6bb52d48 * (float)(param_1 & 0xff)) / _DAT_6bb52d4c));
  return (param_1 & 0xff) + (int)(dVar1 * -14.0) + (uint)(param_1 >> 8) * 0x100;
}


// ============================================================================
// Function: TZ_GetTzEdgeWidthDim1 @ 6ba7fd24
// ============================================================================

int TZ_GetTzEdgeWidthDim1(longlong param_1)

{
  int iVar1;
  
  if ((*(char *)(param_1 + 0x38) == -0x80) || (*(char *)(param_1 + 0x37) == '\x7f')) {
    iVar1 = 0;
  }
  else {
    iVar1 = ((uint)*(byte *)(param_1 + 0x38) - (uint)*(byte *)(param_1 + 0x37)) + 1;
  }
  return iVar1;
}


// ============================================================================
// Function: TZ_GetTzEdgeWidthDim2 @ 6ba7fd66
// ============================================================================

int TZ_GetTzEdgeWidthDim2(longlong param_1)

{
  int iVar1;
  
  if ((*(char *)(param_1 + 0x3a) == -0x80) || (*(char *)(param_1 + 0x39) == '\x7f')) {
    iVar1 = 0;
  }
  else {
    iVar1 = ((uint)*(byte *)(param_1 + 0x3a) - (uint)*(byte *)(param_1 + 0x39)) + 1;
  }
  return iVar1;
}


// ============================================================================
// Function: TZ_GetMinEllipseRate @ 6ba7fda8
// ============================================================================

undefined2 TZ_GetMinEllipseRate(byte param_1)

{
  undefined2 uVar1;
  
  if (*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1a4) <
      *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1a0))
  {
    uVar1 = *(undefined2 *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1a4);
  }
  else {
    uVar1 = *(undefined2 *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1a0);
  }
  return uVar1;
}


// ============================================================================
// Function: TZ_GetZValue @ 6ba7fe48
// ============================================================================

uint TZ_GetZValue(int param_1)

{
  uint local_c;
  
  local_c = (uint)*(ushort *)_refptr_g_tsaPrmtFlashConst * param_1 >> 0xc;
  if (local_c == 0) {
    local_c = 1;
  }
  if (0xffff < local_c) {
    local_c = 0xffff;
  }
  return local_c;
}


// ============================================================================
// Function: TZ_UpdateTouchFlag @ 6ba7fe99
// ============================================================================

void TZ_UpdateTouchFlag(longlong param_1,byte param_2)

{
  uint uVar1;
  longlong lVar2;
  
  lVar2 = *(longlong *)_refptr_g_touchesPtr + (longlong)(int)(uint)param_2 * 0x168 + 0x140;
  uVar1 = *(uint *)(param_1 + 4);
  if ((uVar1 & 0x20) == 0) {
    *(uint *)(lVar2 + 0xb0) = *(uint *)(lVar2 + 0xb0) | 4;
  }
  if ((uVar1 & 0x80000) == 0) {
    *(uint *)(lVar2 + 0xb4) = *(uint *)(lVar2 + 0xb4) | 0x1000;
  }
  if ((uVar1 & 0x10000) != 0) {
    *(uint *)(lVar2 + 0xb0) = *(uint *)(lVar2 + 0xb0) | 0x10000000;
  }
  if ((uVar1 & 0x1000) != 0) {
    *(uint *)(lVar2 + 0xb0) = *(uint *)(lVar2 + 0xb0) | 0x20000;
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) | 8;
  }
  if ((uVar1 & 0x8000) != 0) {
    *(uint *)(lVar2 + 0xb0) = *(uint *)(lVar2 + 0xb0) | 0x2000000;
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) | 0x10;
    (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
         (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 0x800;
  }
  if ((uVar1 & 0x20000) != 0) {
    *(uint *)(lVar2 + 0xb0) = *(uint *)(lVar2 + 0xb0) | 0x1000000;
    (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
         (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 0x20;
  }
  if ((uVar1 & 0x40000) != 0) {
    *(uint *)(lVar2 + 0xb0) = *(uint *)(lVar2 + 0xb0) | 0x20000000;
    (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
         (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 0x20;
  }
  if (*(int *)(param_1 + 8) == 5) {
    *(uint *)(lVar2 + 0xb0) = *(uint *)(lVar2 + 0xb0) | 0x2000;
    (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
         (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 0x800;
  }
  if ((uVar1 & 0x100000) != 0) {
    *(uint *)(lVar2 + 0xb4) = *(uint *)(lVar2 + 0xb4) | 0x2000;
  }
  if ((uVar1 & 0x1000000) != 0) {
    *(uint *)(lVar2 + 0xd8) = *(uint *)(lVar2 + 0xd8) | 1;
  }
  if ((uVar1 & 0x8000000) != 0) {
    *(uint *)(lVar2 + 0xd4) = *(uint *)(lVar2 + 0xd4) | 0x400;
  }
  if (((uVar1 & 0x20000000) != 0) && ((uVar1 & 0x1000000) == 0)) {
    *(uint *)(lVar2 + 0xd4) = *(uint *)(lVar2 + 0xd4) | 0x80000000;
  }
  if ((uVar1 & 0x800) != 0) {
    *(uint *)(lVar2 + 0xc0) = *(uint *)(lVar2 + 0xc0) | 2;
  }
  return;
}


// ============================================================================
// Function: TZ_UpdateInsideSensor @ 6ba8019d
// ============================================================================

void TZ_UpdateInsideSensor(longlong param_1,byte *param_2,byte *param_3)

{
  byte bVar1;
  byte bVar2;
  int iVar3;
  
  bVar1 = (byte)((ushort)*(undefined2 *)(param_1 + 0x2ae) >> 8);
  bVar2 = (byte)((ushort)*(undefined2 *)(param_1 + 0x2b0) >> 8);
  if (*(short *)((ulonglong)
                 ((uint)bVar1 + (uint)bVar2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
                *(longlong *)_refptr_g_tsaBufDif) < 0) {
    iVar3 = -1;
  }
  else {
    iVar3 = 1;
  }
  if ((*(char *)((ulonglong)
                 ((uint)bVar1 + (uint)bVar2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) +
                g_tzAccess) * iVar3 - (uint)*(byte *)(g_ptrAccess + 9) == 6) ||
     ((uint)(byte)_refptr_g_tsaPrmtFlash->field_0x1b * 2 +
      (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x1b < (uint)*(ushort *)(param_1 + 0x2ee) * 0x10)) {
    TSAIDE_LogSensor(bVar1,bVar2,1);
    *param_2 = bVar1;
    *param_3 = bVar2;
  }
  else {
    *param_2 = _refptr_g_tsaPrmtFlash->bCols;
    *param_3 = _refptr_g_tsaPrmtFlash->bRows;
  }
  return;
}


// ============================================================================
// Function: TZ_GetId @ 6ba802e3
// ============================================================================

uchar TZ_GetId(longlong param_1,uchar param_2)

{
  byte bVar1;
  byte local_a;
  
  local_a = 0;
  while( true ) {
    if (*(byte *)(param_1 + 0x2b2) <= local_a) {
      return param_2;
    }
    bVar1 = *(byte *)(param_1 + 0x2b9 + (longlong)(int)(uint)local_a);
    if ((*_refptr_g_peaksPtr)[(longlong)(int)(uint)bVar1 * 0x14 + 0x14].bPeakCount != '\0') break;
    local_a = local_a + 1;
  }
  return (*_refptr_g_peaksPtr)[(longlong)(int)(uint)bVar1 * 0x14 + 0x14].bPeakCount;
}


// ============================================================================
// Function: TZ_MFProcess @ 6ba80fbd
// ============================================================================

void TZ_MFProcess(longlong param_1)

{
  uchar uVar1;
  uchar uVar2;
  undefined2 uVar3;
  byte local_9;
  
  if ((((*(uint *)(param_1 + 4) & 0x800) == 0) && ((*(uint *)(param_1 + 4) & 0x1000000) == 0)) &&
     (*(char *)(param_1 + 0x34b) == '\0')) {
    *(short *)(param_1 + 0xc) =
         (short)((ulonglong)*(ushort *)(param_1 + 0xc) /
                (ulonglong)(longlong)(int)(uint)*(byte *)(param_1 + 0x2b2));
    *(int *)(param_1 + 0x18) = *(int *)(param_1 + 0x18) / (int)(uint)*(byte *)(param_1 + 0x2b2);
    *(undefined1 *)(param_1 + 0x2f4) = 0;
    *(ushort *)(param_1 + 0x2ec) = (ushort)(byte)_refptr_g_tsaPrmtDynamic[0x6c];
    *(undefined2 *)(param_1 + 0x2ee) = *(undefined2 *)(param_1 + 0x2ec);
    uVar3 = TS_MMToPixel(_refptr_g_tsaPrmtDynamic[0x6c]);
    *(undefined2 *)(param_1 + 0x2f0) = uVar3;
    *(undefined2 *)(param_1 + 0x2f2) = *(undefined2 *)(param_1 + 0x2f0);
    for (local_9 = 0; local_9 < *(byte *)(param_1 + 0x2b2); local_9 = local_9 + 1) {
      uVar1 = (*_refptr_g_peaksPtr)
              [(longlong)(int)(uint)*(byte *)(param_1 + 0x2b9 + (longlong)(int)(uint)local_9) * 0x14
               + 0x15].bPeakCount;
      uVar2 = (*_refptr_g_peaksPtr)
              [(longlong)(int)(uint)*(byte *)(param_1 + 0x2b9 + (longlong)(int)(uint)local_9) * 0x14
               + 0x16].bPeakCount;
      CTD_General(uVar1,uVar2,3,3,param_1 + 0x2ae,param_1 + 0x2b0);
      *(undefined1 *)(param_1 + 0x2b6) =
           *(undefined1 *)(param_1 + 0x2b9 + (longlong)(int)(uint)local_9);
      TZ_GripLevelProcess(param_1,uVar1,uVar2);
      TZ_RegisterTouch(param_1);
    }
  }
  else {
    TZ_RegisterBasedOnSubTZ(param_1,1);
  }
  return;
}


// ============================================================================
// Function: TZ_RegistForShapeFF @ 6ba811e3
// ============================================================================

void TZ_RegistForShapeFF(longlong param_1)

{
                    /* 0x411e3  2440  TZ_RegistForShapeFF */
  if (((_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3) && (0xe < *(short *)(param_1 + 0x2aa)))
     || ((ushort)(byte)_refptr_g_tsaPrmtFlash->field_0x836 <= *(ushort *)(param_1 + 0x2ec))) {
    TZ_RegisterBasedOnSubTZ(param_1,0);
  }
  else if (*(char *)(param_1 + 0x34b) == '\0') {
    TZ_RegisterTouch(param_1);
  }
  else {
    TZ_RegisterBasedOnSubTZ(param_1,0);
  }
  return;
}


// ============================================================================
// Function: TZ_RegisterBasedOnSubTZ @ 6ba8096a
// ============================================================================

void TZ_RegisterBasedOnSubTZ(longlong param_1,char param_2)

{
  byte local_9;
  
  if (param_2 == '\0') {
    *(undefined4 *)(param_1 + 0x18) =
         *(undefined4 *)
          (_refptr_g_subTZ + (longlong)(int)(uint)(byte)_refptr_g_subTZ[2] * 0x48 + 0x14);
    *(undefined2 *)(param_1 + 0xc) =
         *(undefined2 *)
          (_refptr_g_subTZ + (longlong)(int)(uint)(byte)_refptr_g_subTZ[2] * 0x48 + 0xc);
    *(undefined *)(param_1 + 0x2f4) =
         _refptr_g_subTZ[(longlong)(int)(uint)(byte)_refptr_g_subTZ[2] * 0x48 + 0x40];
    *(undefined2 *)(param_1 + 0x2ec) =
         *(undefined2 *)
          (_refptr_g_subTZ + (longlong)(int)(uint)(byte)_refptr_g_subTZ[2] * 0x48 + 0x38);
    *(undefined2 *)(param_1 + 0x2ee) =
         *(undefined2 *)
          (_refptr_g_subTZ + (longlong)(int)(uint)(byte)_refptr_g_subTZ[2] * 0x48 + 0x3a);
    *(undefined2 *)(param_1 + 0x2f0) =
         *(undefined2 *)
          (_refptr_g_subTZ + (longlong)(int)(uint)(byte)_refptr_g_subTZ[2] * 0x48 + 0x3c);
    *(undefined2 *)(param_1 + 0x2f2) =
         *(undefined2 *)
          (_refptr_g_subTZ + (longlong)(int)(uint)(byte)_refptr_g_subTZ[2] * 0x48 + 0x3e);
    *(undefined2 *)(param_1 + 0x2ae) =
         *(undefined2 *)
          (_refptr_g_subTZ + (longlong)(int)(uint)(byte)_refptr_g_subTZ[2] * 0x48 + 0xe);
    *(undefined2 *)(param_1 + 0x2b0) =
         *(undefined2 *)
          (_refptr_g_subTZ + (longlong)(int)(uint)(byte)_refptr_g_subTZ[2] * 0x48 + 0x10);
    *(undefined *)(param_1 + 0x32a) =
         _refptr_g_subTZ[(longlong)(int)(uint)(byte)_refptr_g_subTZ[2] * 0x48 + 0x45];
    if (_refptr_g_subTZ[(longlong)(int)(uint)(byte)_refptr_g_subTZ[2] * 0x48 + 10] != '\x14') {
      *(undefined *)(param_1 + 0x2b6) =
           _refptr_g_subTZ[(longlong)(int)(uint)(byte)_refptr_g_subTZ[2] * 0x48 + 10];
    }
    TZ_RegisterTouch(param_1);
  }
  else {
    for (local_9 = 0; local_9 < (byte)*_refptr_g_subTZ; local_9 = local_9 + 1) {
      if ((*(uint *)(_refptr_g_subTZ + (longlong)(int)(uint)local_9 * 0x48 + 0x48) & 3) == 0) {
        *(undefined4 *)(param_1 + 0x18) =
             *(undefined4 *)(_refptr_g_subTZ + (longlong)(int)(uint)local_9 * 0x48 + 0x14);
        *(undefined2 *)(param_1 + 0xc) =
             *(undefined2 *)(_refptr_g_subTZ + (longlong)(int)(uint)local_9 * 0x48 + 0xc);
        *(undefined *)(param_1 + 0x2f4) =
             _refptr_g_subTZ[(longlong)(int)(uint)local_9 * 0x48 + 0x40];
        *(undefined2 *)(param_1 + 0x2ec) =
             *(undefined2 *)(_refptr_g_subTZ + (longlong)(int)(uint)local_9 * 0x48 + 0x38);
        *(undefined2 *)(param_1 + 0x2ee) =
             *(undefined2 *)(_refptr_g_subTZ + (longlong)(int)(uint)local_9 * 0x48 + 0x3a);
        *(undefined2 *)(param_1 + 0x2f0) =
             *(undefined2 *)(_refptr_g_subTZ + (longlong)(int)(uint)local_9 * 0x48 + 0x3c);
        *(undefined2 *)(param_1 + 0x2f2) =
             *(undefined2 *)(_refptr_g_subTZ + (longlong)(int)(uint)local_9 * 0x48 + 0x3e);
        *(undefined2 *)(param_1 + 0x2ae) =
             *(undefined2 *)(_refptr_g_subTZ + (longlong)(int)(uint)local_9 * 0x48 + 0xe);
        *(undefined2 *)(param_1 + 0x2b0) =
             *(undefined2 *)(_refptr_g_subTZ + (longlong)(int)(uint)local_9 * 0x48 + 0x10);
        *(undefined *)(param_1 + 0x32a) =
             _refptr_g_subTZ[(longlong)(int)(uint)local_9 * 0x48 + 0x45];
        if (_refptr_g_subTZ[(longlong)(int)(uint)local_9 * 0x48 + 10] != '\x14') {
          *(undefined *)(param_1 + 0x2b6) =
               _refptr_g_subTZ[(longlong)(int)(uint)local_9 * 0x48 + 10];
        }
        TZ_RegisterTouch(param_1);
      }
    }
  }
  return;
}


// ============================================================================
// Function: TZ_UpdateDimMaxMin @ 6ba8781b
// ============================================================================

void TZ_UpdateDimMaxMin(longlong param_1,byte param_2,byte param_3)

{
  if (param_2 < *(byte *)(param_1 + 0x33)) {
    *(byte *)(param_1 + 0x33) = param_2;
  }
  if (*(byte *)(param_1 + 0x34) < param_2) {
    *(byte *)(param_1 + 0x34) = param_2;
  }
  if (param_3 < *(byte *)(param_1 + 0x35)) {
    *(byte *)(param_1 + 0x35) = param_3;
  }
  if (*(byte *)(param_1 + 0x36) < param_3) {
    *(byte *)(param_1 + 0x36) = param_3;
  }
  return;
}


// ============================================================================
// Function: TZ_UpdateMValue @ 6ba87890
// ============================================================================

void TZ_UpdateMValue(longlong param_1,short param_2,byte param_3,byte param_4)

{
  int iVar1;
  
  *(int *)(param_1 + 0x2f8) = *(int *)(param_1 + 0x2f8) + (int)param_2;
  iVar1 = (int)param_2 * (uint)param_3;
  *(int *)(param_1 + 0x2fc) = *(int *)(param_1 + 0x2fc) + iVar1;
  *(longlong *)(param_1 + 0x308) =
       *(longlong *)(param_1 + 0x308) + (longlong)(int)((uint)param_4 * iVar1);
  *(longlong *)(param_1 + 0x310) =
       *(longlong *)(param_1 + 0x310) + (longlong)(int)((uint)param_3 * iVar1);
  *(uint *)(param_1 + 0x300) = *(int *)(param_1 + 0x300) + (int)param_2 * (uint)param_4;
  *(longlong *)(param_1 + 0x318) =
       *(longlong *)(param_1 + 0x318) +
       (longlong)(int)((uint)param_4 * (int)param_2 * (uint)param_4);
  return;
}


// ============================================================================
// Function: TZ_UpdateEdgeInfo @ 6ba87985
// ============================================================================

void TZ_UpdateEdgeInfo(longlong param_1,short param_2,byte param_3,byte param_4,uint param_5)

{
  byte bVar1;
  byte bVar2;
  
  bVar1 = _refptr_g_tsaPrmtFlash->field_0x92;
  bVar2 = _refptr_g_tsaPrmtFlash->field_0x93;
  if ((param_3 == _refptr_g_tsaPrmtFlash->field_0x90) ||
     (param_3 == _refptr_g_tsaPrmtFlash->field_0x91)) {
    *(int *)(param_1 + 0x2cc) = *(int *)(param_1 + 0x2cc) + (int)param_2;
    if (((param_5 & 1) != 0) && (*(short *)(param_1 + 0x2dc) < param_2)) {
      *(short *)(param_1 + 0x2dc) = param_2;
    }
    if ((param_5 & 4) != 0) {
      if ((int)(uint)param_4 < (int)*(char *)(param_1 + 0x39)) {
        *(byte *)(param_1 + 0x39) = param_4;
        *(byte *)(param_1 + 0x41) = param_3;
      }
      if ((int)*(char *)(param_1 + 0x3a) < (int)(uint)param_4) {
        *(byte *)(param_1 + 0x3a) = param_4;
        *(byte *)(param_1 + 0x42) = param_3;
      }
    }
  }
  else if (((uint)param_3 == (byte)_refptr_g_tsaPrmtFlash->field_0x90 + 1) ||
          ((uint)param_3 == (byte)_refptr_g_tsaPrmtFlash->field_0x91 - 1)) {
    *(int *)(param_1 + 0x2d0) = *(int *)(param_1 + 0x2d0) + (int)param_2;
    if (((param_5 & 2) != 0) && (*(short *)(param_1 + 0x2de) < param_2)) {
      *(short *)(param_1 + 0x2de) = param_2;
    }
    if ((param_5 & 4) != 0) {
      if ((int)(uint)param_4 < (int)*(char *)(param_1 + 0x3d)) {
        *(byte *)(param_1 + 0x3d) = param_4;
      }
      if ((int)*(char *)(param_1 + 0x3e) < (int)(uint)param_4) {
        *(byte *)(param_1 + 0x3e) = param_4;
      }
    }
  }
  if ((param_4 == bVar1) || (param_4 == bVar2)) {
    *(int *)(param_1 + 0x2d4) = *(int *)(param_1 + 0x2d4) + (int)param_2;
    if (((param_5 & 1) != 0) && (*(short *)(param_1 + 0x2e0) < param_2)) {
      *(short *)(param_1 + 0x2e0) = param_2;
    }
    if ((param_5 & 4) != 0) {
      if ((int)(uint)param_3 < (int)*(char *)(param_1 + 0x37)) {
        *(byte *)(param_1 + 0x37) = param_3;
        *(byte *)(param_1 + 0x3f) = param_4;
      }
      if ((int)*(char *)(param_1 + 0x38) < (int)(uint)param_3) {
        *(byte *)(param_1 + 0x38) = param_3;
        *(byte *)(param_1 + 0x40) = param_4;
      }
    }
  }
  else if (((uint)param_4 == bVar1 + 1) || ((uint)param_4 == bVar2 - 1)) {
    *(int *)(param_1 + 0x2d8) = *(int *)(param_1 + 0x2d8) + (int)param_2;
    if (((param_5 & 2) != 0) && (*(short *)(param_1 + 0x2e2) < param_2)) {
      *(short *)(param_1 + 0x2e2) = param_2;
    }
    if ((param_5 & 4) != 0) {
      if ((int)(uint)param_3 < (int)*(char *)(param_1 + 0x3b)) {
        *(byte *)(param_1 + 0x3b) = param_3;
      }
      if ((int)*(char *)(param_1 + 0x3c) < (int)(uint)param_3) {
        *(byte *)(param_1 + 0x3c) = param_3;
      }
    }
  }
  return;
}


// ============================================================================
// Function: TZ_UpdateWeightedSumSimple @ 6ba896a4
// ============================================================================

void TZ_UpdateWeightedSumSimple(longlong param_1,short param_2,byte param_3,byte param_4)

{
  *(int *)(param_1 + 0x18) = *(int *)(param_1 + 0x18) + (int)param_2;
  *(int *)(param_1 + 0x1c) = *(int *)(param_1 + 0x1c) + (int)param_2;
  *(int *)(param_1 + 0x20) = *(int *)(param_1 + 0x20) + (int)param_2;
  *(uint *)(param_1 + 0x2c4) = *(int *)(param_1 + 0x2c4) + (int)param_2 * (uint)param_3;
  *(uint *)(param_1 + 0x2c8) = *(int *)(param_1 + 0x2c8) + (int)param_2 * (uint)param_4;
  return;
}


// ============================================================================
// Function: TZ_IsSD @ 6ba8a67e
// ============================================================================

undefined8 TZ_IsSD(longlong param_1)

{
  undefined8 uVar1;
  
                    /* 0x4a67e  2420  TZ_IsSD */
  if ((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold * 2 <
      _refptr_g_tsaPrpt->nSPeakPositive * 3) {
    uVar1 = 0;
  }
  else if ((*_refptr_g_prevTouchesPtr)->nTouchCount == 8) {
    uVar1 = 1;
  }
  else if ((((*(char *)(param_1 + 0x324) == '\x14') ||
            (*(ushort *)
              ((longlong)*_refptr_g_prevTouchesPtr +
              (longlong)(int)(uint)*(byte *)(param_1 + 0x324) * 0x478 + 0x228) < 0x14)) ||
           (*(int *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)*(byte *)(param_1 + 0x324) * 0x478 + 0x1e0) != 2)) ||
          (((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)*(byte *)(param_1 + 0x324) * 0x478 + 500) & 0x1000) == 0
           || (*(short *)_refptr_g_tsaPrmtDynamic * 2 <=
               *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                         (longlong)(int)(uint)*(byte *)(param_1 + 0x324) * 0x478 + 0x1ba) * 3)))) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: TZ_IsGripWall @ 6ba87cc5
// ============================================================================

undefined4 TZ_IsGripWall(byte param_1,int param_2,ushort param_3,ushort param_4,short param_5)

{
  ushort uVar1;
  uint uVar2;
  uint uVar3;
  undefined4 local_c;
  
  local_c = 0;
  if (((_refptr_g_tsaPrmtFlash->dwGripFlags & 0x200) != 0) &&
     ((param_1 == 0 || ((uint)param_1 == param_2 - 1U)))) {
    uVar1 = *(ushort *)(*(longlong *)_refptr_g_tsaBufTzAge + (ulonglong)param_3 * 2);
    if ((param_4 == 0) ||
       ((uVar1 == 0 ||
        (uVar2 = (uint)param_4 - (uint)uVar1, uVar3 = (int)uVar2 >> 0x1f,
        (int)((uVar2 ^ uVar3) - uVar3) < 0xc9)))) {
      local_c = 0;
    }
    else {
      local_c = 1;
    }
    if (param_5 < 0) {
      local_c = 1;
    }
  }
  return local_c;
}


// ============================================================================
// Function: TZ_IsIDMerged @ 6ba87ef9
// ============================================================================

undefined8 TZ_IsIDMerged(longlong param_1,byte param_2,byte param_3)

{
  byte bVar1;
  
  if (*(char *)((ulonglong)
                ((uint)param_2 + (uint)param_3 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) +
               *(longlong *)_refptr_g_tsaBufTzID) != '\0') {
    if (*(char *)((ulonglong)
                  ((uint)param_2 + (uint)param_3 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) +
                 *(longlong *)_refptr_g_tsaBufTzID) == (*_refptr_g_prevTouchesPtr)->field_0x9) {
      *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x40000000;
    }
    bVar1 = PrevTouch_GetIdxBasedOnID
                      (*(undefined1 *)
                        ((ulonglong)
                         ((uint)param_2 + (uint)param_3 * (uint)_refptr_g_tsaPrmtFlash->bCols &
                         0xffff) + *(longlong *)_refptr_g_tsaBufTzID));
    if (bVar1 != 0x14) {
      if (*(int *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x1e0)
          == 2) {
        *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x80000000;
      }
      if ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)bVar1 * 0x478 + 0x200) & 2) != 0) {
        *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 2;
      }
    }
    if (*(char *)(param_1 + 0x35f) == '\0') {
      *(undefined1 *)(param_1 + 0x35f) =
           *(undefined1 *)
            ((ulonglong)
             ((uint)param_2 + (uint)param_3 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) +
            *(longlong *)_refptr_g_tsaBufTzID);
    }
    else if (*(char *)((ulonglong)
                       ((uint)param_2 + (uint)param_3 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff
                       ) + *(longlong *)_refptr_g_tsaBufTzID) != *(char *)(param_1 + 0x35f)) {
      *(undefined1 *)(param_1 + 0x360) =
           *(undefined1 *)
            ((ulonglong)
             ((uint)param_2 + (uint)param_3 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) +
            *(longlong *)_refptr_g_tsaBufTzID);
      return 1;
    }
  }
  return 0;
}


// ============================================================================
// Function: TZ_IsEdgeMerged @ 6ba88122
// ============================================================================

undefined8 TZ_IsEdgeMerged(longlong param_1,byte param_2,byte param_3)

{
  ushort uVar1;
  
  if (((((uint)(*_refptr_g_peaksPtr)[(longlong)(int)(uint)*(byte *)(param_1 + 0x2b3) * 0x14 + 0x15].
               bPeakCount != _refptr_g_tsaPrmtFlash->bCols - 1) &&
       ((*_refptr_g_peaksPtr)[(longlong)(int)(uint)*(byte *)(param_1 + 0x2b3) * 0x14 + 0x15].
        bPeakCount != '\0')) &&
      ((uint)(*_refptr_g_peaksPtr)[(longlong)(int)(uint)*(byte *)(param_1 + 0x2b3) * 0x14 + 0x16].
             bPeakCount != _refptr_g_tsaPrmtFlash->bRows - 1)) &&
     ((*_refptr_g_peaksPtr)[(longlong)(int)(uint)*(byte *)(param_1 + 0x2b3) * 0x14 + 0x16].
      bPeakCount != '\0')) {
    uVar1 = (ushort)param_2 + (ushort)param_3 * (ushort)_refptr_g_tsaPrmtFlash->bCols;
    if (((((uint)param_2 == _refptr_g_tsaPrmtFlash->bCols - 1) || (param_2 == 0)) ||
        (((uint)param_3 == _refptr_g_tsaPrmtFlash->bRows - 1 || (param_3 == 0)))) &&
       ((*(char *)((ulonglong)uVar1 + *(longlong *)_refptr_g_tsaBufTzZonePrev) == '\0' &&
        ((int)(uint)*(ushort *)&_refptr_g_tsaPrmtFlash[1].field_0x7a <
         (int)*(short *)(*(longlong *)_refptr_g_tsaBufGripTime + (ulonglong)uVar1 * 2) -
         (int)*(short *)(*(longlong *)_refptr_g_tsaBufGripTime +
                        (ulonglong)
                        (ushort)((ushort)(*_refptr_g_peaksPtr)
                                         [(longlong)(int)(uint)*(byte *)(param_1 + 0x2b3) * 0x14 +
                                          0x15].bPeakCount +
                                (ushort)(*_refptr_g_peaksPtr)
                                        [(longlong)(int)(uint)*(byte *)(param_1 + 0x2b3) * 0x14 +
                                         0x16].bPeakCount * (ushort)_refptr_g_tsaPrmtFlash->bCols) *
                        2))))) {
      return 1;
    }
  }
  return 0;
}


// ============================================================================
// Function: TZ_GetMaxMinPkIdx @ 6ba82207
// ============================================================================

void TZ_GetMaxMinPkIdx(longlong param_1)

{
  byte bVar1;
  short sVar2;
  int iVar3;
  int local_14;
  short local_e;
  short local_c;
  byte local_9;
  
                    /* 0x42207  2382  TZ_GetMaxMinPkIdx */
  local_c = 0x7fff;
  local_e = -0x8000;
  local_14 = -0x80000000;
  for (local_9 = 0; local_9 < *(byte *)(param_1 + 0x2b2); local_9 = local_9 + 1) {
    bVar1 = *(byte *)(param_1 + 0x2b9 + (longlong)(int)(uint)local_9);
    sVar2 = *(short *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)bVar1 * 0x14 + 0x18);
    iVar3 = *(int *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)bVar1 * 0x14 + 0x1c);
    if (sVar2 < local_c) {
      *(byte *)(param_1 + 0x2b7) = bVar1;
      local_c = sVar2;
    }
    if (local_e < sVar2) {
      *(byte *)(param_1 + 0x2b6) = bVar1;
      local_e = sVar2;
    }
    if (local_14 < iVar3 + sVar2) {
      *(byte *)(param_1 + 0x2b8) = bVar1;
      local_14 = iVar3 + sVar2;
    }
  }
  *(undefined1 *)(param_1 + 0x2b5) = *(undefined1 *)(param_1 + 0x2b6);
  return;
}


// ============================================================================
// Function: TZ_PeakRegister @ 6ba82339
// ============================================================================

void TZ_PeakRegister(longlong param_1,undefined1 param_2)

{
  byte bVar1;
  char cVar2;
  
                    /* 0x42339  2438  TZ_PeakRegister */
  if (*(byte *)(param_1 + 0x2b2) < 10) {
    bVar1 = *(byte *)(param_1 + 0x2b2);
    *(byte *)(param_1 + 0x2b2) = bVar1 + 1;
    *(undefined1 *)(param_1 + 0x2b9 + (longlong)(int)(uint)bVar1) = param_2;
    cVar2 = Peak_IsOnOuterEdge(param_2);
    if (cVar2 != '\0') {
      *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 1;
    }
  }
  return;
}


// ============================================================================
// Function: TZ_GetHTIValidFlag @ 6ba81626
// ============================================================================

void TZ_GetHTIValidFlag(longlong param_1)

{
                    /* 0x41626  2381  TZ_GetHTIValidFlag */
  if ((ushort)((ushort)*(byte *)(param_1 + 0x2b2) * 4) < *(ushort *)(param_1 + 0xc)) {
    *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x100;
  }
  return;
}


// ============================================================================
// Function: TZ_GetSurroundingStatusFlag @ 6ba81efd
// ============================================================================

void TZ_GetSurroundingStatusFlag(longlong param_1)

{
  char cVar1;
  
                    /* 0x41efd  2399  TZ_GetSurroundingStatusFlag */
  cVar1 = TZ_GetSurroundingStatus(param_1);
  if (cVar1 == '\x01') {
    *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x8000;
  }
  return;
}


// ============================================================================
// Function: TZ_GetShapeFlag @ 6ba81fdb
// ============================================================================

void TZ_GetShapeFlag(longlong param_1)

{
  ushort uVar1;
  ushort uVar2;
  ushort local_c;
  ushort local_a;
  
                    /* 0x41fdb  2396  TZ_GetShapeFlag */
  uVar1 = *(ushort *)
           (_refptr_g_subTZ + (longlong)(int)(uint)*(ushort *)(_refptr_g_subTZ + 6) * 0x48 + 0x38);
  uVar2 = *(ushort *)
           (_refptr_g_subTZ + (longlong)(int)(uint)*(ushort *)(_refptr_g_subTZ + 6) * 0x48 + 0x3a);
  if ((int)((uint)(byte)_refptr_g_tsaPrmtFlash->field_0x836 * 0x10 -
           (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x836) / 10 < (int)(uint)uVar1) {
    *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x1000;
  }
  if (((uint)uVar2 * 2 + (uint)uVar2 < (uint)uVar1) && (0xf < uVar1)) {
    *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x1000;
  }
  uVar1 = (short)(((ulonglong)(byte)_refptr_g_tsaPrmtFlash->field_0x836 << 4) /
                 (ulonglong)(longlong)(int)(uint)(byte)_refptr_g_tsaPrmtFlash->field_0x1b) - 1;
  if ((*(char *)(param_1 + 0x38) == -0x80) || (*(char *)(param_1 + 0x37) == '\x7f')) {
    local_a = 0;
  }
  else {
    local_a = ((short)*(char *)(param_1 + 0x38) - (short)*(char *)(param_1 + 0x37)) + 1;
  }
  if (uVar1 <= local_a) {
    *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x1000;
  }
  if ((*(char *)(param_1 + 0x3a) == -0x80) || (*(char *)(param_1 + 0x39) == '\x7f')) {
    local_c = 0;
  }
  else {
    local_c = ((short)*(char *)(param_1 + 0x3a) - (short)*(char *)(param_1 + 0x39)) + 1;
  }
  if (uVar1 <= local_c) {
    *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x1000;
  }
  if (0x13 < *(byte *)(param_1 + 0x334)) {
    *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x1000;
  }
  if (0x13 < *(byte *)(param_1 + 0x335)) {
    *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x1000;
  }
  return;
}


// ============================================================================
// Function: TZ_GetCMFFlag @ 6ba82694
// ============================================================================

void TZ_GetCMFFlag(longlong param_1)

{
  char cVar1;
  short sVar2;
  short sVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int local_14;
  int local_10;
  int local_c;
  
                    /* 0x42694  2371  TZ_GetCMFFlag */
  sVar2 = CMF_GetNoiseValue((*_refptr_g_peaksPtr)
                            [(longlong)(int)(uint)*(byte *)(param_1 + 0x2b6) * 0x14 + 0x15].
                            bPeakCount,
                            (*_refptr_g_peaksPtr)
                            [(longlong)(int)(uint)*(byte *)(param_1 + 0x2b6) * 0x14 + 0x16].
                            bPeakCount);
  iVar4 = (int)sVar2;
  iVar5 = iVar4 + *(short *)(*_refptr_g_peaksPtr +
                            (longlong)(int)(uint)*(byte *)(param_1 + 0x2b6) * 0x14 + 0x18);
  local_c = (int)*(short *)(_refptr_g_tsaPrmtDynamic + 0xc);
  cVar1 = TouchThold_ToeUseSpecialThold();
  if ((cVar1 != '\0') &&
     ((((*_refptr_g_peaksPtr)[(longlong)(int)(uint)*(byte *)(param_1 + 0x2b6) * 0x14 + 0x15].
        bPeakCount == '\x01' ||
       ((uint)(*_refptr_g_peaksPtr)[(longlong)(int)(uint)*(byte *)(param_1 + 0x2b6) * 0x14 + 0x15].
              bPeakCount == _refptr_g_tsaPrmtFlash->bCols - 2)) ||
      ((uint)(*_refptr_g_peaksPtr)[(longlong)(int)(uint)*(byte *)(param_1 + 0x2b6) * 0x14 + 0x16].
             bPeakCount == _refptr_g_tsaPrmtFlash->bRows - 1)))) {
    local_c = (int)*(short *)(_refptr_g_tsaPrmtDynamic + 0xe);
  }
  if (iVar5 < local_c) {
    iVar6 = CMF_GetCmfMethod();
    if (iVar6 != 1) {
      *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x20000;
    }
  }
  if ((*(uint *)(param_1 + 4) & 0x9000) != 0) {
    if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
      sVar2 = CMF_GetNoiseValue(*(char *)(param_1 + 0x33) + -1,*(undefined1 *)(param_1 + 0x35));
      sVar3 = CMF_GetNoiseValue(*(char *)(param_1 + 0x34) + '\x01',*(undefined1 *)(param_1 + 0x36));
    }
    else {
      sVar2 = CMF_GetNoiseValue(*(undefined1 *)(param_1 + 0x33),*(char *)(param_1 + 0x35) + -1);
      sVar3 = CMF_GetNoiseValue(*(undefined1 *)(param_1 + 0x34),*(char *)(param_1 + 0x36) + '\x01');
    }
    local_10 = (int)sVar2;
    local_14 = (int)sVar3;
    if ((((int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold < local_10 * 2) &&
        (iVar4 * 2 <= local_10)) && (iVar5 < local_10 * 2)) {
      *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x40000;
    }
    if ((((int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold < local_14 * 2) &&
        (iVar4 * 2 <= local_14)) && (iVar5 < local_14 * 2)) {
      *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x40000;
    }
  }
  return;
}


// ============================================================================
// Function: TZ_GetShapeRatio @ 6ba833cb
// ============================================================================

void TZ_GetShapeRatio(longlong param_1)

{
  uint uVar1;
  undefined1 local_c;
  
                    /* 0x433cb  2397  TZ_GetShapeRatio */
  uVar1 = (((uint)*(ushort *)(param_1 + 0x2ee) * (uint)*(ushort *)(param_1 + 0x2ec) * 0x100) /
          (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x1b) /
          (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x1c;
  if (uVar1 == 0) {
    local_c = 0xff;
  }
  else {
    local_c = (undefined1)(((ulonglong)*(ushort *)(param_1 + 0xc) << 4) / (ulonglong)uVar1);
  }
  *(undefined1 *)(param_1 + 0x2f5) = local_c;
  return;
}


// ============================================================================
// Function: TZ_GetType @ 6ba837f8
// ============================================================================

void TZ_GetType(longlong param_1)

{
  char cVar1;
  byte bVar2;
  int iVar3;
  undefined4 uVar4;
  
                    /* 0x437f8  2401  TZ_GetType */
  cVar1 = TZ_IsLO(param_1);
  if (cVar1 == '\0') {
    if (((_refptr_g_tsaPrmtFlash->dwGripFlags & 1) == 0) &&
       (((*(uint *)(param_1 + 4) & 0x8000000) == 0 || ((*(uint *)(param_1 + 4) & 0x200000) != 0))))
    {
      if (*(byte *)(param_1 + 0x2b2) < 2) {
        iVar3 = TZ_GetPrevShape(*(undefined1 *)(param_1 + 0x2b9));
        if ((iVar3 == 2) || ((*(uint *)(param_1 + 4) & 2) != 0)) {
          *(undefined4 *)(param_1 + 8) = 2;
        }
        else if (iVar3 == 3) {
          *(undefined4 *)(param_1 + 8) = 3;
        }
        else {
          uVar4 = TZ_JudgeFFOrNF(param_1);
          *(undefined4 *)(param_1 + 8) = uVar4;
        }
      }
      else if ((*(uint *)(param_1 + 4) & 0x10000) == 0) {
        if ((*(uint *)(param_1 + 4) & 0x1000000) == 0) {
          if ((*(uint *)(param_1 + 4) & 0x10000000) == 0) {
            if ((((*(uint *)(param_1 + 4) & 0x200000) == 0) || ((*(uint *)(param_1 + 4) & 1) != 0))
               || (cVar1 = TZ_IsAbsFFMinorAxis((char)*(undefined2 *)(param_1 + 0x2ee)),
                  cVar1 != '\0')) {
              if (((*(ushort *)(param_1 + 0x2ec) < 0x14) || (*(char *)(param_1 + 0x324) == '\x14'))
                 || ((*(int *)((longlong)*_refptr_g_prevTouchesPtr +
                              (longlong)(int)(uint)*(byte *)(param_1 + 0x324) * 0x478 + 0x1e0) != 2
                     || (((*(uint *)(param_1 + 4) & 0x200000) != 0 ||
                         (*(short *)_refptr_g_tsaPrmtDynamic * 2 <= *(short *)(param_1 + 0x2e) * 3))
                        )))) {
                if ((*(uint *)(param_1 + 4) & 0x40) == 0) {
                  bVar2 = PosnSpecial_GetPosMarker();
                  if ((bVar2 < 2) ||
                     (*(short *)(_refptr_g_list + 0x38) <=
                      *(short *)(*_refptr_g_peaksPtr +
                                (longlong)(int)(uint)*(byte *)(param_1 + 0x2b6) * 0x14 + 0x18) / 2))
                  {
                    cVar1 = SubTZ_IsMF();
                    if (cVar1 == '\0') {
                      TZ_DFSProcess(param_1);
                    }
                    else {
                      *(undefined4 *)(param_1 + 8) = 3;
                    }
                  }
                  else {
                    *(undefined4 *)(param_1 + 8) = 3;
                  }
                }
                else {
                  *(undefined4 *)(param_1 + 8) = 3;
                }
              }
              else {
                *(undefined4 *)(param_1 + 8) = 2;
              }
            }
            else {
              *(undefined4 *)(param_1 + 8) = 3;
            }
          }
          else {
            *(undefined4 *)(param_1 + 8) = 2;
          }
        }
        else {
          *(undefined4 *)(param_1 + 8) = 3;
        }
      }
      else {
        *(undefined4 *)(param_1 + 8) = 2;
      }
      TZ_MultiFatFingerCheck(param_1);
    }
    else {
      *(undefined4 *)(param_1 + 8) = 1;
    }
  }
  else {
    *(undefined4 *)(param_1 + 8) = 5;
  }
  return;
}


// ============================================================================
// Function: TZ_CheckToSeparateZone @ 6ba855b7
// ============================================================================

void TZ_CheckToSeparateZone(longlong param_1)

{
  bool bVar1;
  char cVar2;
  
                    /* 0x455b7  2362  TZ_CheckToSeparateZone */
  cVar2 = TZ_IsGripMergeAllowed(param_1);
  if ((cVar2 != '\0') && (*(char *)(param_1 + 0x324) != '\x14')) {
    TZ_GetOutEdgeSumRatio(param_1);
    TZ_GetMove(param_1);
    TZ_GetPeakGripFlag(param_1);
    bVar1 = false;
    if (((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)*(byte *)(param_1 + 0x324) * 0x478 + 0x218) & 1) != 0) &&
       ((*(uint *)(param_1 + 4) & 0x800000) != 0)) {
      *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x10000000;
      bVar1 = true;
    }
    if ((bVar1) || (cVar2 = TZ_IsOKToInsertGripPeak(param_1), cVar2 != '\0')) {
      TZ_TryMergePeak(param_1);
    }
    TZ_GetPeakGripFlag(param_1);
    if ((((*(uint *)(param_1 + 4) & 0x400000) != 0) && ((*(uint *)(param_1 + 4) & 0x800000) != 0))
       && ((bVar1 || (cVar2 = TZ_IsToSeparateZone(param_1), cVar2 != '\0')))) {
      TZ_MovePeakNotInGripToEnd(param_1);
      *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x1000000;
    }
  }
  return;
}


// ============================================================================
// Function: TZ_GetContour @ 6ba87571
// ============================================================================

void TZ_GetContour(longlong param_1)

{
  undefined2 uVar1;
  
                    /* 0x47571  2373  TZ_GetContour */
  TZ_Interpolate(param_1);
  TZ_FindContourDot(param_1);
  TZ_GetCircleRate(param_1);
  uVar1 = TZ_GetEllipseRate(param_1);
  *(undefined2 *)(param_1 + 0x2aa) = uVar1;
  uVar1 = TZ_GetEllipseRateMirror(param_1);
  *(undefined2 *)(param_1 + 0x2ac) = uVar1;
  return;
}


// ============================================================================
// Function: TZ_VirtualPeakInfoRetrieval @ 6ba8af94
// ============================================================================

void TZ_VirtualPeakInfoRetrieval(void)

{
  longlong lVar1;
  ushort uVar2;
  char cVar3;
  byte local_9;
  
                    /* 0x4af94  2456  TZ_VirtualPeakInfoRetrieval */
  lVar1 = *(longlong *)_refptr_g_tzAccess;
  for (local_9 = 0; local_9 < (*_refptr_g_peaksPtr)->bPeakCount; local_9 = local_9 + 1) {
    if (((*(uint *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)local_9 * 0x14 + 0x10) & 0x100) != 0)
       && ((g_pkAccessedFlag & 1 << (local_9 & 0x1f)) == 0)) {
      uVar2 = (ushort)(*_refptr_g_peaksPtr)[(longlong)(int)(uint)local_9 * 0x14 + 0x15].bPeakCount +
              (ushort)(*_refptr_g_peaksPtr)[(longlong)(int)(uint)local_9 * 0x14 + 0x16].bPeakCount *
              (ushort)_refptr_g_tsaPrmtFlash->bCols;
      if (*(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)uVar2 * 2) < 0) {
        cVar3 = -1;
      }
      else {
        cVar3 = '\x01';
      }
      *(char *)((ulonglong)uVar2 + lVar1) =
           (*(char *)(*(longlong *)_refptr_g_ptrAccess + 9) + '\x01') * cVar3;
      g_pkAccessedFlag = 1 << (local_9 & 0x1f) | g_pkAccessedFlag;
    }
  }
  return;
}


// ============================================================================
// Function: TZ_VPProcess @ 6ba8d348
// ============================================================================

void TZ_VPProcess(longlong param_1)

{
  char cVar1;
  int iVar2;
  byte local_9;
  
                    /* 0x4d348  2455  TZ_VPProcess */
  local_9 = 0;
  if (*(char *)(param_1 + 0x324) != '\x14') {
    local_9 = (byte)*(undefined2 *)
                     ((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)*(byte *)(param_1 + 0x324) * 0x478 + 0x188);
  }
  if ((*_refptr_g_peaksPtr)->bPeakCount < (*_refptr_g_prevTouchesPtr)->bPrevTouchCount) {
    iVar2 = TZ_GetDBShape(param_1);
    if ((((iVar2 == 3) &&
         (*(int *)&_refptr_g_tsaPrpt[1].field_0x2c == *(int *)&_refptr_g_tsaPrpt[1].field_0x38)) &&
        (*(int *)(param_1 + 0x350) == 0)) &&
       ((((*_refptr_g_prevTouchesPtr)->nTouchCount == 0 && ((*(uint *)(param_1 + 4) & 0x20) == 0))
        && ((local_9 < 10 && ((_refptr_g_tsaPrmtFlash->dwGripFlags & 2) == 0)))))) {
      cVar1 = TZ_IsVirtualPeakAllowed(param_1);
      if (cVar1 != '\0') {
        TZ_TryInsertVPA(param_1);
      }
    }
  }
  return;
}


// ============================================================================
// Function: TZ_CalcGripRatio @ 6ba7fa1a
// ============================================================================

byte TZ_CalcGripRatio(undefined4 param_1,undefined8 param_2,undefined4 *param_3,undefined2 param_4, undefined2 param_5)

{
  *param_3 = 0;
  if (_refptr_g_tsaPrmtFlash->field_0x1a == '\0') {
    return TZ_CalcGripRatioDim1OnShortAxis(param_1,param_2,param_3,param_4,param_5);
  }
  else {
    return TZ_CalcGripRatioDim1OnLongAxis(param_1,param_2,param_3,param_4,param_5);
  }
}


// ============================================================================
// Function: TZ_GripLevelProcess @ 6ba80e91
// ============================================================================

void TZ_GripLevelProcess(longlong param_1,byte param_2,byte param_3)

{
  byte local_9;
  
  for (local_9 = 0; local_9 < (byte)*_refptr_g_subTZ; local_9 = local_9 + 1) {
    if (((((byte)_refptr_g_subTZ[(longlong)(int)(uint)local_9 * 0x48 + 0x41] <= param_2) &&
         (param_2 <= (byte)_refptr_g_subTZ[(longlong)(int)(uint)local_9 * 0x48 + 0x42])) &&
        ((byte)_refptr_g_subTZ[(longlong)(int)(uint)local_9 * 0x48 + 0x43] <= param_3)) &&
       (param_3 <= (byte)_refptr_g_subTZ[(longlong)(int)(uint)local_9 * 0x48 + 0x44])) {
      *(undefined *)(param_1 + 0x32a) = _refptr_g_subTZ[(longlong)(int)(uint)local_9 * 0x48 + 0x45];
    }
  }
  return;
}


// ============================================================================
// Function: TZ_SeparateZoneRegretProcess @ 6ba85492
// ============================================================================

void TZ_SeparateZoneRegretProcess(longlong param_1,longlong param_2)

{
  char cVar1;
  
                    /* 0x45492  2442  TZ_SeparateZoneRegretProcess */
  if ((((*(uint *)(param_1 + 4) & 0x1000000) != 0) && ((*(uint *)(param_1 + 4) & 0x10000000) == 0))
     && ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)*(byte *)(param_1 + 0x324) * 0x478 + 0x1f8) & 0x20000000) ==
         0)) {
    cVar1 = GripFilter_IsPeakAbsNotGrip(*(undefined1 *)(param_2 + 2));
    if (cVar1 != '\0') {
      cVar1 = TZ_IsAbnormalShape(param_1,param_2);
      if ((cVar1 != '\0') || (1 < *(byte *)(param_2 + 3))) {
        *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x8000000;
        *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) & 0xfeffffff;
      }
    }
  }
  return;
}


// ============================================================================
// Function: TZ_GetSurroundingStatus @ 6ba819d9
// ============================================================================

undefined8 TZ_GetSurroundingStatus(longlong param_1)

{
  char cVar1;
  
  if ((((byte)(*(char *)(param_1 + 0x34) + 1U) < _refptr_g_tsaPrmtFlash->bCols) &&
      (6 < *(int *)(_refptr_g_tsaBufSelf +
                   ((longlong)(int)(*(byte *)(param_1 + 0x34) + 1) + 0x640) * 4))) &&
     (cVar1 = TZ_IsClearSignalGapAlongDim2
                        (*(undefined1 *)(param_1 + 0x34),*(char *)(param_1 + 0x34) + '\x01',
                         *(undefined1 *)(param_1 + 0x35),*(undefined1 *)(param_1 + 0x36),0),
     cVar1 != '\0')) {
    if (_refptr_g_tsaPrmtFlash->bCols <= (byte)(*(char *)(param_1 + 0x33) - 1U)) {
      return 1;
    }
    if ((6 < *(int *)(_refptr_g_tsaBufSelf +
                     ((longlong)(int)(*(byte *)(param_1 + 0x33) - 1) + 0x640) * 4)) &&
       (cVar1 = TZ_IsClearSignalGapAlongDim2
                          (*(char *)(param_1 + 0x33) + -1,*(undefined1 *)(param_1 + 0x33),
                           *(undefined1 *)(param_1 + 0x35),*(undefined1 *)(param_1 + 0x36),1),
       cVar1 != '\0')) {
      return 1;
    }
  }
  if ((((byte)(*(char *)(param_1 + 0x33) - 1U) < _refptr_g_tsaPrmtFlash->bCols) &&
      (6 < *(int *)(_refptr_g_tsaBufSelf +
                   ((longlong)(int)(*(byte *)(param_1 + 0x33) - 1) + 0x640) * 4))) &&
     (cVar1 = TZ_IsClearSignalGapAlongDim2
                        (*(char *)(param_1 + 0x33) + -1,*(undefined1 *)(param_1 + 0x33),
                         *(undefined1 *)(param_1 + 0x35),*(undefined1 *)(param_1 + 0x36),1),
     cVar1 != '\0')) {
    if (_refptr_g_tsaPrmtFlash->bCols <= (byte)(*(char *)(param_1 + 0x34) + 1U)) {
      return 1;
    }
    if ((6 < *(int *)(_refptr_g_tsaBufSelf +
                     ((longlong)(int)(*(byte *)(param_1 + 0x34) + 1) + 0x640) * 4)) &&
       (cVar1 = TZ_IsClearSignalGapAlongDim2
                          (*(undefined1 *)(param_1 + 0x34),*(char *)(param_1 + 0x34) + '\x01',
                           *(undefined1 *)(param_1 + 0x35),*(undefined1 *)(param_1 + 0x36),0),
       cVar1 != '\0')) {
      return 1;
    }
  }
  if ((((byte)(*(char *)(param_1 + 0x36) + 1U) < _refptr_g_tsaPrmtFlash->bRows) &&
      (6 < *(int *)(_refptr_g_tsaBufSelf +
                   ((longlong)
                    (int)((uint)*(byte *)(param_1 + 0x36) + (uint)_refptr_g_tsaPrmtFlash->bCols + 1)
                   + 0x640) * 4))) &&
     (cVar1 = TZ_IsClearSignalGapAlongDim1
                        (*(undefined1 *)(param_1 + 0x33),*(undefined1 *)(param_1 + 0x34),
                         *(undefined1 *)(param_1 + 0x36),*(char *)(param_1 + 0x36) + '\x01',0),
     cVar1 != '\0')) {
    if (_refptr_g_tsaPrmtFlash->bRows <= (byte)(*(char *)(param_1 + 0x35) - 1U)) {
      return 1;
    }
    if ((6 < *(int *)(_refptr_g_tsaBufSelf +
                     ((longlong)
                      (int)((uint)*(byte *)(param_1 + 0x35) + (uint)_refptr_g_tsaPrmtFlash->bCols +
                           -1) + 0x640) * 4)) &&
       (cVar1 = TZ_IsClearSignalGapAlongDim1
                          (*(undefined1 *)(param_1 + 0x33),*(undefined1 *)(param_1 + 0x34),
                           *(char *)(param_1 + 0x35) + -1,*(undefined1 *)(param_1 + 0x35),1),
       cVar1 != '\0')) {
      return 1;
    }
  }
  if ((((byte)(*(char *)(param_1 + 0x35) - 1U) < _refptr_g_tsaPrmtFlash->bRows) &&
      (6 < *(int *)(_refptr_g_tsaBufSelf +
                   ((longlong)
                    (int)((uint)*(byte *)(param_1 + 0x35) + (uint)_refptr_g_tsaPrmtFlash->bCols + -1
                         ) + 0x640) * 4))) &&
     (cVar1 = TZ_IsClearSignalGapAlongDim1
                        (*(undefined1 *)(param_1 + 0x33),*(undefined1 *)(param_1 + 0x34),
                         *(char *)(param_1 + 0x35) + -1,*(undefined1 *)(param_1 + 0x35),1),
     cVar1 != '\0')) {
    if (_refptr_g_tsaPrmtFlash->bRows <= (byte)(*(char *)(param_1 + 0x36) + 1U)) {
      return 1;
    }
    if ((6 < *(int *)(_refptr_g_tsaBufSelf +
                     ((longlong)
                      (int)((uint)*(byte *)(param_1 + 0x36) + (uint)_refptr_g_tsaPrmtFlash->bCols +
                           1) + 0x640) * 4)) &&
       (cVar1 = TZ_IsClearSignalGapAlongDim1
                          (*(undefined1 *)(param_1 + 0x33),*(undefined1 *)(param_1 + 0x34),
                           *(undefined1 *)(param_1 + 0x36),*(char *)(param_1 + 0x36) + '\x01',0),
       cVar1 != '\0')) {
      return 1;
    }
  }
  return 0;
}


// ============================================================================
// Function: TZ_DFSProcess @ 6ba832d9
// ============================================================================

void TZ_DFSProcess(longlong param_1)

{
  char cVar1;
  int iVar2;
  
                    /* 0x432d9  2364  TZ_DFSProcess */
  cVar1 = TSAStatic_IsAllFreqNoisy();
  if (cVar1 == '\0') {
    if ((*_refptr_g_prevTouchesPtr)->bPrevTouchCount < (*_refptr_g_peaksPtr)->bPeakCount) {
      TZ_PeakFilter(param_1);
    }
    if (*(short *)(*_refptr_g_peaksPtr +
                  (longlong)(int)(uint)*(byte *)(param_1 + 0x2b6) * 0x14 + 0x18) <
        (short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax) {
      *(undefined4 *)(param_1 + 8) = 2;
    }
    else {
      iVar2 = TZ_GetDBShape(param_1);
      if (iVar2 == 0) {
        TZ_JudgeFFOrMF(param_1);
      }
      else {
        cVar1 = TZ_IsAbsFatFinger(param_1);
        if (cVar1 == '\0') {
          *(int *)(param_1 + 8) = iVar2;
          TZ_LogDbShape(iVar2);
        }
        else {
          *(undefined4 *)(param_1 + 8) = 2;
        }
      }
    }
  }
  else {
    *(undefined4 *)(param_1 + 8) = 2;
  }
  return;
}


// ============================================================================
// Function: TZ_IsLO @ 6ba83474
// ============================================================================

undefined8 TZ_IsLO(longlong param_1)

{
  ushort uVar1;
  undefined8 uVar2;
  undefined4 uVar3;
  ushort local_a;
  
                    /* 0x43474  2412  TZ_IsLO */
  if ((_refptr_g_tsaPrmtFlash->dwGripFlags & 0x10) == 0) {
    local_a = *(short *)(_refptr_g_subTZ +
                        (longlong)(int)(uint)*(ushort *)(_refptr_g_subTZ + 6) * 0x48 + 0x3a) *
              *(short *)(_refptr_g_subTZ +
                        (longlong)(int)(uint)*(ushort *)(_refptr_g_subTZ + 6) * 0x48 + 0x38);
  }
  else {
    local_a = *(short *)(param_1 + 0x2ee) * *(short *)(param_1 + 0x2ec);
  }
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    if ((*(int *)_refptr_g_isSSLoSD == 0) && (*(int *)_refptr_g_isSSLo == 0)) {
      uVar3 = 0;
    }
    else {
      uVar3 = 1;
    }
    StrPrintf_Process(0,"TZ_IsLO ssIsLo=%d, exception=%d, area=%d, shapeRatio=%d",uVar3,
                      *(undefined4 *)(param_1 + 0x350),local_a,*(undefined1 *)(param_1 + 0x2f5));
  }
  if (((*(uint *)(param_1 + 0x350) < 9) || (*(int *)_refptr_g_isSSLoSD != 0)) ||
     (*(int *)_refptr_g_isSSLo != 0)) {
    if ((((*(int *)_refptr_g_isSSLoSD == 0) || (*(uint *)(param_1 + 0x350) < 8)) &&
        ((*(int *)_refptr_g_isSSLo == 0 || (7 < *(uint *)(param_1 + 0x350))))) ||
       ((local_a < 0x2bd && (((*_refptr_g_peaksPtr)->bPeakCount < 6 || (local_a < 0x1f5)))))) {
      uVar1 = DB_LOSize();
      if (uVar1 < local_a) {
        if (*(ushort *)
             (_refptr_g_subTZ + (longlong)(int)(uint)*(ushort *)(_refptr_g_subTZ + 6) * 0x48 + 0x38)
            <= (ushort)(byte)_refptr_g_tsaPrmtFlash->field_0x836) {
          return 1;
        }
        if (5 < *(byte *)(param_1 + 0x2f5)) {
          return 1;
        }
      }
      uVar2 = 0;
    }
    else {
      uVar2 = 1;
    }
  }
  else {
    uVar2 = 0;
  }
  return uVar2;
}


// ============================================================================
// Function: TZ_MultiFatFingerCheck @ 6ba836a1
// ============================================================================

void TZ_MultiFatFingerCheck(longlong param_1)

{
  ushort uVar1;
  byte bVar2;
  char cVar3;
  byte bVar4;
  
                    /* 0x436a1  2431  TZ_MultiFatFingerCheck */
  if (*(int *)(param_1 + 8) == 3) {
    bVar2 = PrevTouch_GetIdxBasedOnID(*(undefined1 *)(param_1 + 0x35f));
    if (((*(uint *)(param_1 + 4) & 2) != 0) && (bVar2 != 0x14)) {
      cVar3 = GripFilter_IsGrip(*(undefined4 *)
                                 ((longlong)*_refptr_g_prevTouchesPtr +
                                 (longlong)(int)(uint)bVar2 * 0x478 + 0x210));
      if (cVar3 == '\0') {
        *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x800;
      }
    }
    if (10 < *(ushort *)(_refptr_g_subTZ + 4)) {
      bVar4 = SubTZ_GetNumAftFilter();
      if (1 < bVar4) {
        *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x800;
      }
    }
    if (*(int *)(param_1 + 4) < 0) {
      if ((*(uint *)(param_1 + 4) & 0x200000) != 0) {
        *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x800;
      }
      if ((4 < *(byte *)(param_1 + 0x2b2)) ||
         ((bVar2 != 0x14 &&
          (uVar1 = *(ushort *)
                    ((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)bVar2 * 0x478 + 0x188),
          (uint)uVar1 * 2 + (uint)uVar1 <
          (uint)*(ushort *)(param_1 + 0x2ec) + (uint)*(ushort *)(param_1 + 0x2ec))))) {
        *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x800;
      }
    }
  }
  return;
}


// ============================================================================
// Function: TZ_GetPrevShape @ 6ba8a030
// ============================================================================

undefined4 TZ_GetPrevShape(byte param_1)

{
  byte bVar1;
  undefined4 local_c;
  
                    /* 0x4a030  2389  TZ_GetPrevShape */
  local_c = 0;
  if ((*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_1 * 0x14 + 0x14].bPeakCount != '\0') {
    bVar1 = PrevTouch_GetIdxBasedOnID
                      ((*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_1 * 0x14 + 0x14].bPeakCount)
    ;
    if (bVar1 == 0x14) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Alg/TouchZone/TZ_ShapeDebounce.c",0x13,
                        "tchIdx != TOUCH_INVALID_IDX");
      HardwareAnalyzer_SetSoftException();
    }
    if ((bVar1 < 0x14) &&
       ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x1f8)
        & 8) != 0)) {
      local_c = *(undefined4 *)
                 ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x1e0);
    }
  }
  return local_c;
}


// ============================================================================
// Function: TZ_IsAbsFFMinorAxis @ 6ba8ce81
// ============================================================================

bool TZ_IsAbsFFMinorAxis(byte param_1)

{
                    /* 0x4ce81  2407  TZ_IsAbsFFMinorAxis */
  return *(ushort *)&_refptr_g_tsaPrmtFlash->field_0x82e < (ushort)param_1;
}


// ============================================================================
// Function: TZ_JudgeFFOrNF @ 6ba8d1ed
// ============================================================================

undefined4 TZ_JudgeFFOrNF()

{
  undefined4 local_c;
  
                    /* 0x4d1ed  2428  TZ_JudgeFFOrNF */
  local_c = 1;
  if (9 < *(ushort *)(_refptr_g_subTZ + 4)) {
    local_c = 2;
  }
  return local_c;
}


// ============================================================================
// Function: TZ_GetPeakGripFlag @ 6ba84485
// ============================================================================

void TZ_GetPeakGripFlag(longlong param_1)

{
  byte bVar1;
  byte bVar2;
  char cVar3;
  byte local_9;
  
                    /* 0x44485  2387  TZ_GetPeakGripFlag */
  TZ_GetMinPeakGripTime(param_1);
  for (local_9 = 0; local_9 < *(byte *)(param_1 + 0x2b2); local_9 = local_9 + 1) {
    bVar1 = *(byte *)(param_1 + 0x2b9 + (longlong)(int)(uint)local_9);
    bVar2 = Peak_GetDistToEdge(bVar1);
    if ((byte)_refptr_g_tsaPrmtFlash[1].field_0x79 < bVar2) {
      *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x800000;
      *(uint *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)bVar1 * 0x14 + 0x10) =
           *(uint *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)bVar1 * 0x14 + 0x10) | 0x800;
    }
    else {
      cVar3 = TZ_IsPeakInGrip(bVar1);
      if (cVar3 == '\0') {
        cVar3 = TZ_IsPeakNotInGrip(param_1,bVar1);
        if (cVar3 != '\0') {
          *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x800000;
          *(uint *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)bVar1 * 0x14 + 0x10) =
               *(uint *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)bVar1 * 0x14 + 0x10) | 0x800;
        }
      }
      else {
        *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x400000;
        *(uint *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)bVar1 * 0x14 + 0x10) =
             *(uint *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)bVar1 * 0x14 + 0x10) | 0x400;
      }
    }
    cVar3 = TZ_IsPeakInGrip(bVar1);
    if (cVar3 == '\0') {
      cVar3 = Peak_IsOnOuterEdge(bVar1);
      if (cVar3 == '\0') {
        *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x20000000;
      }
    }
  }
  TZ_GetMaxNotGripTime(param_1);
  return;
}


// ============================================================================
// Function: TZ_TryMergePeak @ 6ba84d81
// ============================================================================

undefined1 TZ_TryMergePeak(undefined8 param_1)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x18]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x20];
  char cVar1;
  
                    /* 0x44d81  2448  TZ_TryMergePeak */
  (*(char *)(_stk + 0x17)) = '\0';
  (*(undefined1 *)(_stk + 0xa)) = 0;
  (*(undefined1 *)(_stk + 0xb)) = 0;
  (*(undefined2 *)(_stk + 0x8)) = 0;
  cVar1 = TZ_SearchMergePeak(param_1,&(*(undefined2 *)(_stk + 0x8)));
  if (cVar1 != '\0') {
    (*(char *)(_stk + 0x17)) = '\x01';
  }
  (*(undefined1 *)(_stk + 0x16)) = 0;
  if ((*(char *)(_stk + 0x17)) != '\0') {
    (*(undefined1 *)(_stk + 0x16)) = TZ_InsertVirtualPeak(param_1,(*(undefined1 *)(_stk + 0xa)),(*(undefined1 *)(_stk + 0xb)));
  }
  return (*(undefined1 *)(_stk + 0x16));
}


// ============================================================================
// Function: TZ_GetOutEdgeSumRatio @ 6ba84e4c
// ============================================================================

void TZ_GetOutEdgeSumRatio(longlong param_1)

{
  int iVar1;
  int local_c;
  
                    /* 0x44e4c  2386  TZ_GetOutEdgeSumRatio */
  iVar1 = TSA_GetSide(*(undefined2 *)(param_1 + 0x2ae),*(undefined2 *)(param_1 + 0x2b0));
  if ((iVar1 == 4) || (iVar1 == 8)) {
    if (*(int *)((longlong)*_refptr_g_prevTouchesPtr +
                (longlong)(int)(uint)*(byte *)(param_1 + 0x324) * 0x478 + 0x270) == 0) {
      local_c = 100;
    }
    else {
      local_c = (*(int *)(param_1 + 0x2cc) * 100) /
                *(int *)((longlong)*_refptr_g_prevTouchesPtr +
                        (longlong)(int)(uint)*(byte *)(param_1 + 0x324) * 0x478 + 0x270);
    }
  }
  else if (*(int *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)*(byte *)(param_1 + 0x324) * 0x478 + 0x274) == 0) {
    local_c = 100;
  }
  else {
    local_c = (*(int *)(param_1 + 0x2d4) * 100) /
              *(int *)((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)*(byte *)(param_1 + 0x324) * 0x478 + 0x274);
  }
  *(int *)(param_1 + 0x32c) = local_c;
  return;
}


// ============================================================================
// Function: TZ_GetMove @ 6ba84fba
// ============================================================================

void TZ_GetMove(longlong param_1)

{
  byte bVar1;
  ushort uVar2;
  int iVar3;
  
                    /* 0x44fba  2385  TZ_GetMove */
  bVar1 = *(byte *)(param_1 + 0x324);
  if (bVar1 < 0x14) {
    iVar3 = ((int)*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                            (longlong)(int)(uint)bVar1 * 0x478 + 0x4d4) -
            (uint)*(ushort *)
                   ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x162
                   )) + (uint)*(ushort *)(param_1 + 0x2ae);
    uVar2 = (ushort)(iVar3 >> 0x1f);
    *(ushort *)(param_1 + 0x330) = ((ushort)iVar3 ^ uVar2) - uVar2;
    iVar3 = ((int)*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                            (longlong)(int)(uint)bVar1 * 0x478 + 0x4d6) -
            (uint)*(ushort *)
                   ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x164
                   )) + (uint)*(ushort *)(param_1 + 0x2b0);
    uVar2 = (ushort)(iVar3 >> 0x1f);
    *(ushort *)(param_1 + 0x332) = ((ushort)iVar3 ^ uVar2) - uVar2;
  }
  else {
    *(undefined2 *)(param_1 + 0x330) = 0;
    *(undefined2 *)(param_1 + 0x332) = 0;
  }
  return;
}


// ============================================================================
// Function: TZ_IsOKToInsertGripPeak @ 6ba850e8
// ============================================================================

undefined8 TZ_IsOKToInsertGripPeak(longlong param_1)

{
  undefined8 uVar1;
  
                    /* 0x450e8  2414  TZ_IsOKToInsertGripPeak */
  if ((((*(uint *)(param_1 + 4) & 0x400000) == 0) && ((*(uint *)(param_1 + 4) & 0x800000) != 0)) &&
     ((*(uint *)(param_1 + 4) & 0x2000000) != 0)) {
    uVar1 = TZ_IsPossibleGripMerge(param_1);
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: TZ_IsToSeparateZone @ 6ba851a2
// ============================================================================

undefined8 TZ_IsToSeparateZone(longlong param_1)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0x451a2  2423  TZ_IsToSeparateZone */
  cVar1 = TZ_IsTimeOnIntervalOK(param_1);
  if (cVar1 == '\0') {
    uVar2 = 0;
  }
  else if (*(short *)(param_1 + 0x362) < 5000) {
    uVar2 = 0;
  }
  else {
    cVar1 = TZ_IsPossibleGripMerge(param_1);
    if (cVar1 == '\0') {
      uVar2 = 0;
    }
    else {
      uVar2 = 1;
    }
  }
  return uVar2;
}


// ============================================================================
// Function: TZ_MovePeakNotInGripToEnd @ 6ba851f9
// ============================================================================

void TZ_MovePeakNotInGripToEnd(longlong param_1)

{
  byte bVar1;
  undefined1 uVar2;
  byte local_9;
  
                    /* 0x451f9  2430  TZ_MovePeakNotInGripToEnd */
  local_9 = 0;
  while( true ) {
    if (*(byte *)(param_1 + 0x2b2) <= local_9) {
      return;
    }
    bVar1 = *(byte *)(param_1 + 0x2b9 + (longlong)(int)(uint)local_9);
    if ((*(uint *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)bVar1 * 0x14 + 0x10) & 0x800) != 0)
    break;
    local_9 = local_9 + 1;
  }
  uVar2 = *(undefined1 *)(param_1 + 0x2b9 + (longlong)(int)(*(byte *)(param_1 + 0x2b2) - 1));
  *(byte *)(param_1 + 0x2b9 + (longlong)(int)(*(byte *)(param_1 + 0x2b2) - 1)) = bVar1;
  *(undefined1 *)(param_1 + 0x2b9 + (longlong)(int)(uint)local_9) = uVar2;
  return;
}


// ============================================================================
// Function: TZ_IsGripMergeAllowed @ 6ba85566
// ============================================================================

undefined8 TZ_IsGripMergeAllowed(longlong param_1)

{
  undefined8 uVar1;
  
                    /* 0x45566  2411  TZ_IsGripMergeAllowed */
  if ((byte)(*_refptr_g_prevTouchesPtr)->field_0x8 < 2) {
    if (((*_refptr_g_prevTouchesPtr)->field_0x8 == '\x01') &&
       ((*(uint *)(param_1 + 4) & 0x40000000) == 0)) {
      uVar1 = 0;
    }
    else {
      uVar1 = 1;
    }
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: TZ_Interpolate @ 6ba8613f
// ============================================================================

void TZ_Interpolate(short *param_1)

{
  short sVar1;
  int local_18;
  int local_14;
  int local_10;
  int local_c;
  
  sVar1 = *param_1;
  local_c = *(byte *)((longlong)param_1 + 0x33) - 1;
  local_10 = *(byte *)(param_1 + 0x1a) + 1;
  local_14 = *(byte *)((longlong)param_1 + 0x35) - 1;
  local_18 = *(byte *)(param_1 + 0x1b) + 1;
  memset_s(&g_tzBufferHighRes,0x32000,0,
           (longlong)(int)((uint)_refptr_g_tsaPrmtFlash->bRows << 2) *
           (ulonglong)_refptr_g_tsaPrmtFlash->bCols * 8);
  memset_s(&g_tzBufferBin,0x19000,0,
           (uint)_refptr_g_tsaPrmtFlash->bRows * 4 * (uint)_refptr_g_tsaPrmtFlash->bCols * 4);
  if (local_c < 0) {
    local_c = 0;
  }
  if ((int)(_refptr_g_tsaPrmtFlash->bCols - 1) < local_10) {
    local_10 = _refptr_g_tsaPrmtFlash->bCols - 1;
  }
  if (local_14 < 0) {
    local_14 = 0;
  }
  if ((int)(_refptr_g_tsaPrmtFlash->bRows - 1) < local_18) {
    local_18 = _refptr_g_tsaPrmtFlash->bRows - 1;
  }
  param_1[0x23] = (short)(local_c << 2);
  param_1[0x24] = (short)(local_10 << 2);
  param_1[0x25] = (short)(local_14 << 2);
  param_1[0x26] = (short)(local_18 << 2);
  TZ_CalcHighResDif(param_1,(int)sVar1);
  return;
}


// ============================================================================
// Function: TZ_FindContourDot @ 6ba8649b
// ============================================================================

void TZ_FindContourDot(short *param_1)

{
  int iVar1;
  int local_78 [17];
  int local_34;
  int local_30;
  int local_2c;
  uint local_28;
  uint local_24;
  int local_20;
  uint local_1c;
  int local_18;
  uint local_14;
  int local_10;
  int local_c;
  
  local_20 = (int)*param_1;
  local_24 = (uint)*(byte *)(param_1 + 0x15b);
  local_c = (uint)(*_refptr_g_peaksPtr)[(longlong)(int)local_24 * 0x14 + 0x15].bPeakCount << 2;
  local_10 = TZ_FindContourFirstDotDim2
                       (param_1,(ulonglong)
                                (*_refptr_g_peaksPtr)[(longlong)(int)local_24 * 0x14 + 0x15].
                                bPeakCount << 2,local_20,local_24);
  local_78[0] = 0;
  local_78[1] = 0xffffffff;
  local_78[2] = 0xffffffff;
  local_78[3] = 0xffffffff;
  local_78[4] = 0xffffffff;
  local_78[5] = 0;
  local_78[6] = 0xffffffff;
  local_78[7] = 1;
  local_78[8] = 0;
  local_78[9] = 1;
  local_78[10] = 1;
  local_78[0xb] = 1;
  local_78[0xc] = 1;
  local_78[0xd] = 0;
  local_78[0xe] = 1;
  local_78[0xf] = 0xffffffff;
  param_1[0x153] = 0;
  TZ_PushEdgeDot(param_1,local_c,local_10);
  local_14 = 0;
  do {
    local_18 = 0;
    for (local_1c = 0; local_1c < 8; local_1c = local_1c + 1) {
      local_28 = local_14 + local_1c + 1 & 7;
      local_2c = local_78[(ulonglong)local_28 * 2];
      local_30 = local_78[(ulonglong)local_28 * 2 + 1];
      local_34 = local_78[(ulonglong)local_28 * 2] + local_c;
      local_78[0x10] = local_78[(ulonglong)local_28 * 2 + 1] + local_10;
      if ((param_1[0x27] == local_34) && (param_1[0x28] == local_78[0x10])) break;
      iVar1 = TZ_GetDifHR(param_1,local_34,local_78[0x10]);
      if (local_20 < iVar1) {
        local_18 = 1;
        local_c = local_34;
        local_10 = local_78[0x10];
        local_14 = local_28 + 4 & 7;
        TZ_PushEdgeDot(param_1,local_34,local_78[0x10]);
        break;
      }
    }
    if (local_18 == 0) {
      if (((*_refptr_g_tsaStaticPtr)->dwProcessingState & PROC_MS_ACTIVE) != 0) {
        TZ_ShowContourInIDE(param_1);
      }
      return;
    }
  } while( true );
}


// ============================================================================
// Function: TZ_GetCircleRate @ 6ba866ad
// ============================================================================

void TZ_GetCircleRate(longlong param_1)

{
  double dVar1;
  double dVar2;
  double local_30;
  double local_28;
  double local_20;
  double local_18;
  int local_c;
  
  local_18 = 0.0;
  local_20 = 0.0;
  for (local_c = 0; local_c < *(short *)(param_1 + 0x2a6); local_c = local_c + 1) {
    local_18 = local_18 + (double)(int)*(short *)(param_1 + 0xe + ((longlong)local_c + 0x10) * 4);
    local_20 = local_20 + (double)(int)*(short *)(param_1 + 0x50 + (longlong)local_c * 4);
  }
  if (*(short *)(param_1 + 0x2a6) != 0) {
    local_18 = local_18 / (double)(int)*(short *)(param_1 + 0x2a6);
    local_20 = local_20 / (double)(int)*(short *)(param_1 + 0x2a6);
  }
  local_28 = 0.0;
  local_30 = 0.0;
  for (local_c = 0; local_c < *(short *)(param_1 + 0x2a6); local_c = local_c + 1) {
    dVar1 = (double)(int)*(short *)(param_1 + 0xe + ((longlong)local_c + 0x10) * 4) - local_18;
    dVar2 = (double)(int)*(short *)(param_1 + 0x50 + (longlong)local_c * 4) - local_20;
    dVar2 = dVar1 * dVar1 + dVar2 * dVar2;
    dVar1 = sqrt(dVar2);
    local_28 = local_28 + dVar1;
    local_30 = local_30 + dVar2;
  }
  if (*(short *)(param_1 + 0x2a6) == 0) {
    *(undefined2 *)(param_1 + 0x2a8) = 0;
  }
  else {
    dVar1 = sqrt(((double)(int)*(short *)(param_1 + 0x2a6) * local_30) / (local_28 * local_28) - 1.0
                );
    *(short *)(param_1 + 0x2a8) = (short)(int)(dVar1 * 100.0);
  }
  return;
}


// ============================================================================
// Function: TZ_GetEllipseRate @ 6ba86fe1
// ============================================================================

undefined2 TZ_GetEllipseRate(longlong param_1)

{
  return TZ_GetERInernal(param_1 + 0x4e,(int)*(short *)(param_1 + 0x2a6));
}


// ============================================================================
// Function: TZ_GetEllipseRateMirror @ 6ba8749f
// ============================================================================

undefined2 TZ_GetEllipseRateMirror(longlong param_1)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x2a]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x30];
  
  ((int *)(_stk + 0x8))[0] = 0;
  ((int *)(_stk + 0x8))[1] = 0;
  ((int *)(_stk + 0x8))[2] = 0;
  ((int *)(_stk + 0x8))[3] = 0;
  TZ_CalcEdgeCount(param_1,((int *)(_stk + 0x8)),4);
  (*(uint *)(_stk + 0x20)) = TZ_GetMaxEdgeType(((int *)(_stk + 0x8)),4);
  (*(undefined2 *)(_stk + 0x26)) = *(undefined2 *)(param_1 + 0x2aa);
  if (((int *)(_stk + 0x8))[(*(uint *)(_stk + 0x20))] != 0) {
    TZ_CopyEdgeDot(&edgeDot_11595,param_1);
    (*(short *)(_stk + 0x6)) = *(short *)(param_1 + 0x2a6);
    TZ_RemoveEdgeDot(&edgeDot_11595,&(*(short *)(_stk + 0x6)),(*(uint *)(_stk + 0x20)));
    TZ_MirrorEdgeDot(&edgeDot_11595,&(*(short *)(_stk + 0x6)),(*(uint *)(_stk + 0x20)));
    (*(undefined2 *)(_stk + 0x26)) = TZ_GetERInernal(&edgeDot_11595,(int)(*(short *)(_stk + 0x6)));
  }
  return (*(undefined2 *)(_stk + 0x26));
}


// ============================================================================
// Function: TZ_GetDBShape @ 6ba8a24f
// ============================================================================

undefined4 TZ_GetDBShape(longlong param_1)

{
  undefined1 uVar1;
  ushort uVar2;
  int iVar3;
  ushort local_16;
  undefined4 local_14;
  byte local_d;
  byte local_c;
  byte local_b;
  byte local_a;
  byte local_9;
  
                    /* 0x4a24f  2374  TZ_GetDBShape */
  local_a = 0;
  local_b = 0;
  local_c = 0;
  local_d = 0;
  local_14 = 0;
  local_16 = 0;
  for (local_9 = 0; local_9 < *(byte *)(param_1 + 0x2b2); local_9 = local_9 + 1) {
    uVar1 = *(undefined1 *)(param_1 + 0x2b9 + (longlong)(int)(uint)local_9);
    iVar3 = TZ_GetPrevShape(uVar1);
    uVar2 = TZ_GetPrevAge(uVar1);
    if (local_16 < uVar2) {
      local_16 = uVar2;
    }
    if (iVar3 == 0) {
      local_d = local_d + 1;
    }
    else if (iVar3 == 1) {
      local_a = local_a + 1;
    }
    else if (iVar3 == 3) {
      local_b = local_b + 1;
    }
    else if (iVar3 == 2) {
      local_c = local_c + 1;
    }
  }
  if ((*(byte *)(param_1 + 0x2b2) == local_a) &&
     (*(uchar *)(param_1 + 0x2b2) == (*_refptr_g_prevTouchesPtr)->bPrevTouchCount)) {
    local_14 = 3;
  }
  else if ((local_b == 0) ||
          (((uint)local_a + (uint)local_b != (uint)*(byte *)(param_1 + 0x2b2) || (local_16 < 5)))) {
    if ((local_d + 1 == (uint)*(byte *)(param_1 + 0x2b2)) &&
       (((uint)local_c + (uint)local_a == 1 && (4 < local_16)))) {
      local_14 = 2;
    }
  }
  else {
    local_14 = 3;
  }
  return local_14;
}


// ============================================================================
// Function: TZ_IsVirtualPeakAllowed @ 6ba8d230
// ============================================================================

undefined1 TZ_IsVirtualPeakAllowed(longlong param_1)

{
                    /* 0x4d230  2426  TZ_IsVirtualPeakAllowed */
  if ((uint)*(ushort *)(param_1 + 0x2ee) *
      (uint)(byte)((*(char *)(param_1 + 0x2b2) + '\x01') * '\b' - 4) <
      (uint)*(ushort *)(param_1 + 0x2ec) << 3) {
    (*_refptr_g_tsaStaticPtr)[1].field_0xe = 1;
    (*_refptr_g_tsaStaticPtr)[1].field_0xf = 0;
  }
  else {
    (*_refptr_g_tsaStaticPtr)[1].field_0xf = (*_refptr_g_tsaStaticPtr)[1].field_0xf + '\x01';
    if (4 < (byte)(*_refptr_g_tsaStaticPtr)[1].field_0xf) {
      (*_refptr_g_tsaStaticPtr)[1].field_0xe = 0;
      (*_refptr_g_tsaStaticPtr)[1].field_0xf = 5;
    }
  }
  return (*_refptr_g_tsaStaticPtr)[1].field_0xe;
}


// ============================================================================
// Function: TZ_TryInsertVPA @ 6ba8d6c4
// ============================================================================

undefined1 TZ_TryInsertVPA(longlong param_1)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x18]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x20];
  char cVar1;
  
                    /* 0x4d6c4  2447  TZ_TryInsertVPA */
  (*(undefined1 *)(_stk + 0xa)) = 0;
  (*(undefined1 *)(_stk + 0xb)) = 0;
  (*(undefined2 *)(_stk + 0x8)) = 0;
  (*(short *)(_stk + 0x12)) = TZ_GetSigTholdForVPA(param_1);
  (*(char *)(_stk + 0x17)) = '\0';
  for ((*(byte *)(_stk + 0x16)) = 0; (*(byte *)(_stk + 0x16)) < *(byte *)(param_1 + 0x2b2); (*(byte *)(_stk + 0x16)) = (*(byte *)(_stk + 0x16)) + 1) {
    (*(char *)(_stk + 0x11)) = *(char *)(param_1 + 0x2b9 + (longlong)(int)(uint)(*(byte *)(_stk + 0x16)));
    (*(uchar *)(_stk + 0x10)) = (*_refptr_g_peaksPtr)[(longlong)(int)(*(char *)(_stk + 0x11)) * 0x14 + 0x15].bPeakCount;
    (*(uchar *)(_stk + 0xf)) = (*_refptr_g_peaksPtr)[(longlong)(int)(*(char *)(_stk + 0x11)) * 0x14 + 0x16].bPeakCount;
    cVar1 = TZ_SearchNxN((*(uchar *)(_stk + 0x10)),(*(uchar *)(_stk + 0xf)),(int)(*(short *)(_stk + 0x12)),&(*(undefined2 *)(_stk + 0x8)));
    if (cVar1 != '\0') {
      (*(char *)(_stk + 0x17)) = '\x01';
    }
  }
  (*(undefined1 *)(_stk + 0x15)) = 0;
  if ((*(char *)(_stk + 0x17)) != '\0') {
    (*(undefined1 *)(_stk + 0x15)) = TZ_InsertVirtualPeak(param_1,(*(undefined1 *)(_stk + 0xa)),(*(undefined1 *)(_stk + 0xb)));
  }
  return (*(undefined1 *)(_stk + 0x15));
}


// ============================================================================
// Function: TZ_CalcGripRatioDim1OnLongAxis @ 6ba7f685
// ============================================================================

byte TZ_CalcGripRatioDim1OnLongAxis
               (uint param_1,longlong param_2,uint *param_3,ushort param_4,ushort param_5)

{
  byte local_9;
  
  local_9 = 0;
  if ((((param_1 & 4) != 0) && (*(char *)(param_2 + 0x35) == '\0')) && (param_5 != 0)) {
    *param_3 = *param_3 | 4;
    local_9 = (byte)param_5;
  }
  if (((param_1 & 8) != 0) && ((uint)*(byte *)(param_2 + 0x36) == _refptr_g_tsaPrmtFlash->bRows - 1)
     ) {
    if (param_5 != 0) {
      *param_3 = *param_3 | 8;
    }
    if (local_9 < param_5) {
      local_9 = (byte)param_5;
    }
  }
  if (((param_1 & 1) != 0) && (*(char *)(param_2 + 0x33) == '\0')) {
    if (param_4 != 0) {
      *param_3 = *param_3 | 1;
    }
    if (local_9 < param_4) {
      local_9 = (byte)param_4;
    }
  }
  if (((param_1 & 2) != 0) && ((uint)*(byte *)(param_2 + 0x34) == _refptr_g_tsaPrmtFlash->bCols - 1)
     ) {
    if (param_4 != 0) {
      *param_3 = *param_3 | 2;
    }
    if (local_9 < param_4) {
      local_9 = (byte)param_4;
    }
  }
  return local_9;
}


// ============================================================================
// Function: TZ_CalcGripRatioDim1OnShortAxis @ 6ba7f7d8
// ============================================================================

byte TZ_CalcGripRatioDim1OnShortAxis
               (uint param_1,longlong param_2,uint *param_3,ushort param_4,ushort param_5)

{
  bool bVar1;
  byte local_9;
  
  local_9 = 0;
  if (((((uint)*(byte *)(param_2 + 0x35) + (uint)*(byte *)(param_2 + 0x33) < 2) ||
       ((int)(((_refptr_g_tsaPrmtFlash->bRows - 1) - (uint)*(byte *)(param_2 + 0x36)) +
             (uint)*(byte *)(param_2 + 0x33)) < 2)) ||
      ((int)((uint)*(byte *)(param_2 + 0x35) +
            ((_refptr_g_tsaPrmtFlash->bCols - 1) - (uint)*(byte *)(param_2 + 0x34))) < 2)) ||
     ((int)(((uint)_refptr_g_tsaPrmtFlash->bRows - (uint)*(byte *)(param_2 + 0x36)) +
           ((_refptr_g_tsaPrmtFlash->bCols - 2) - (uint)*(byte *)(param_2 + 0x34))) < 2)) {
    bVar1 = true;
  }
  else {
    bVar1 = false;
  }
  if ((((param_1 & 4) != 0) || (bVar1)) && ((*(char *)(param_2 + 0x33) == '\0' && (param_4 != 0))))
  {
    *param_3 = *param_3 | 4;
    local_9 = (byte)param_4;
  }
  if ((((param_1 & 8) != 0) || (bVar1)) &&
     ((uint)*(byte *)(param_2 + 0x34) == _refptr_g_tsaPrmtFlash->bCols - 1)) {
    if (param_4 != 0) {
      *param_3 = *param_3 | 8;
    }
    if (local_9 < param_4) {
      local_9 = (byte)param_4;
    }
  }
  if ((((param_1 & 1) != 0) || (bVar1)) && (*(char *)(param_2 + 0x35) == '\0')) {
    if (param_5 != 0) {
      *param_3 = *param_3 | 1;
    }
    if (local_9 < param_5) {
      local_9 = (byte)param_5;
    }
  }
  if ((((param_1 & 2) != 0) || (bVar1)) &&
     ((uint)*(byte *)(param_2 + 0x36) == _refptr_g_tsaPrmtFlash->bRows - 1)) {
    if (param_5 != 0) {
      *param_3 = *param_3 | 2;
    }
    if (local_9 < param_5) {
      local_9 = (byte)param_5;
    }
  }
  return local_9;
}


// ============================================================================
// Function: TZ_GetAutoCentToMinPkRatio @ 6ba82e3c
// ============================================================================

undefined1 TZ_GetAutoCentToMinPkRatio(longlong param_1,byte param_2,byte param_3)

{
  uchar uVar1;
  uchar uVar2;
  uchar uVar3;
  uchar uVar4;
  byte bVar5;
  byte bVar6;
  short sVar7;
  undefined1 local_b;
  short local_a;
  
                    /* 0x42e3c  2368  TZ_GetAutoCentToMinPkRatio */
  if ((param_2 == 0x14) || (param_3 == 0x14)) {
    if ((param_2 == 0x14) || (param_3 == 0x14)) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Alg/TouchZone/TZ_DFS.c",0x80,
                        "!((pk0Idx == PEAK_INVALID_IDX) || (pk1Idx == PEAK_INVALID_IDX))");
      HardwareAnalyzer_SetSoftException();
    }
    local_b = 0;
  }
  else if (param_2 == param_3) {
    local_b = 0;
  }
  else {
    uVar1 = (*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_2 * 0x14 + 0x15].bPeakCount;
    uVar2 = (*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_2 * 0x14 + 0x16].bPeakCount;
    uVar3 = (*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_3 * 0x14 + 0x15].bPeakCount;
    uVar4 = (*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_3 * 0x14 + 0x16].bPeakCount;
    bVar5 = uVar3 + uVar1;
    bVar6 = uVar4 + uVar2;
    *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 4;
    *(undefined4 *)(param_1 + 0x2e4) = 0;
    sVar7 = TZ_GetAveCentSig(param_1,bVar5 >> 1,bVar6 >> 1,bVar5,bVar6);
    sVar7 = TZ_UpdateAveCentSig(uVar1,uVar2,uVar3,uVar4,(int)sVar7);
    local_a = *(short *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)param_2 * 0x14 + 0x18);
    if (*(short *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)param_3 * 0x14 + 0x18) < local_a) {
      local_a = *(short *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)param_3 * 0x14 + 0x18);
    }
    if ((sVar7 < local_a) && (local_a != 0)) {
      local_b = (undefined1)(((int)sVar7 << 6) / (int)local_a);
    }
    else {
      local_b = 0x40;
    }
  }
  return local_b;
}


// ============================================================================
// Function: TZ_IsAbnormalShape @ 6ba852d3
// ============================================================================

undefined8 TZ_IsAbnormalShape(longlong param_1,longlong param_2)

{
  byte bVar1;
  short sVar2;
  char cVar3;
  short sVar4;
  ushort local_1a;
  
                    /* 0x452d3  2406  TZ_IsAbnormalShape */
  bVar1 = *(byte *)(param_1 + 0x324);
  if (0xe < *(ushort *)(param_2 + 0x30)) {
    local_1a = 0;
    if (bVar1 < 0x14) {
      local_1a = Touch_GetSQDistMM(*(undefined2 *)(param_1 + 0x2ae),*(undefined2 *)(param_1 + 0x2b0)
                                   ,*(undefined2 *)
                                     ((longlong)*_refptr_g_prevTouchesPtr +
                                     (longlong)(int)(uint)bVar1 * 0x478 + 0x162),
                                   *(undefined2 *)
                                    ((longlong)*_refptr_g_prevTouchesPtr +
                                    (longlong)(int)(uint)bVar1 * 0x478 + 0x164));
    }
    if (local_1a < 0x19) {
      return 1;
    }
    if ((uint)*(ushort *)(param_2 + 0x32) * 2 + (uint)*(ushort *)(param_2 + 0x32) <
        (uint)*(ushort *)(param_2 + 0x30)) {
      return 1;
    }
  }
  cVar3 = Peak_IsOnInnerEdgeDim1(*(undefined1 *)(param_2 + 2));
  if (cVar3 != '\0') {
    if ((*(char *)(param_2 + 0x3a) == *(char *)(param_2 + 0x39)) &&
       (2 < (int)((uint)*(byte *)(param_2 + 0x3c) - (uint)*(byte *)(param_2 + 0x3b)))) {
      return 1;
    }
    if (((int)((uint)*(byte *)(param_2 + 0x3a) - (uint)*(byte *)(param_2 + 0x39)) < 2) &&
       (sVar2 = *(short *)(*_refptr_g_peaksPtr +
                          (longlong)(int)(uint)*(byte *)(param_2 + 2) * 0x14 + 0x18),
       sVar4 = TSAStatic_GetSigThold(1), sVar2 < sVar4)) {
      return 1;
    }
  }
  return 0;
}


// ============================================================================
// Function: TZ_IsClearSignalGapAlongDim1 @ 6ba81663
// ============================================================================

bool TZ_IsClearSignalGapAlongDim1(byte param_1,byte param_2,byte param_3,byte param_4,char param_5)

{
  ushort uVar1;
  short sVar2;
  short local_12;
  short local_10;
  short local_e;
  short local_c;
  byte local_9;
  
  local_10 = -0x8000;
  local_12 = 0x7fff;
  local_9 = param_1;
  while( true ) {
    if (param_2 < local_9) {
      sVar2 = local_10;
      if (local_10 < 0) {
        sVar2 = local_10 + 3;
      }
      return (int)local_10 - (int)local_12 <= (int)(sVar2 >> 2);
    }
    if (param_5 == '\0') {
      uVar1 = *(ushort *)
               ((ulonglong)
                ((uint)local_9 + (uint)param_3 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
               *(longlong *)_refptr_g_tsaBufDif);
      local_c = (uVar1 ^ (short)uVar1 >> 0x1f) - ((short)uVar1 >> 0xf);
      uVar1 = *(ushort *)
               ((ulonglong)
                ((uint)local_9 + (uint)param_4 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
               *(longlong *)_refptr_g_tsaBufDif);
      local_e = (uVar1 ^ (short)uVar1 >> 0x1f) - ((short)uVar1 >> 0xf);
    }
    else {
      uVar1 = *(ushort *)
               ((ulonglong)
                ((uint)local_9 + (uint)param_4 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
               *(longlong *)_refptr_g_tsaBufDif);
      local_c = (uVar1 ^ (short)uVar1 >> 0x1f) - ((short)uVar1 >> 0xf);
      uVar1 = *(ushort *)
               ((ulonglong)
                ((uint)local_9 + (uint)param_3 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
               *(longlong *)_refptr_g_tsaBufDif);
      local_e = (uVar1 ^ (short)uVar1 >> 0x1f) - ((short)uVar1 >> 0xf);
    }
    sVar2 = local_c;
    if (local_c < 0) {
      sVar2 = local_c + 7;
    }
    if (sVar2 >> 3 < local_e) break;
    if (local_10 < local_c) {
      local_10 = local_c;
    }
    if (local_c < local_12) {
      local_12 = local_c;
    }
    local_9 = local_9 + 1;
  }
  return false;
}


// ============================================================================
// Function: TZ_IsClearSignalGapAlongDim2 @ 6ba8181e
// ============================================================================

bool TZ_IsClearSignalGapAlongDim2(byte param_1,byte param_2,byte param_3,byte param_4,char param_5)

{
  ushort uVar1;
  short sVar2;
  short local_12;
  short local_10;
  short local_e;
  short local_c;
  byte local_9;
  
  local_10 = -0x8000;
  local_12 = 0x7fff;
  local_9 = param_3;
  while( true ) {
    if (param_4 < local_9) {
      sVar2 = local_10;
      if (local_10 < 0) {
        sVar2 = local_10 + 3;
      }
      return (int)local_10 - (int)local_12 <= (int)(sVar2 >> 2);
    }
    if (param_5 == '\0') {
      uVar1 = *(ushort *)
               ((ulonglong)
                ((uint)param_2 + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
               *(longlong *)_refptr_g_tsaBufDif);
      local_e = (uVar1 ^ (short)uVar1 >> 0x1f) - ((short)uVar1 >> 0xf);
      uVar1 = *(ushort *)
               ((ulonglong)
                ((uint)param_1 + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
               *(longlong *)_refptr_g_tsaBufDif);
      local_c = (uVar1 ^ (short)uVar1 >> 0x1f) - ((short)uVar1 >> 0xf);
    }
    else {
      uVar1 = *(ushort *)
               ((ulonglong)
                ((uint)param_2 + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
               *(longlong *)_refptr_g_tsaBufDif);
      local_c = (uVar1 ^ (short)uVar1 >> 0x1f) - ((short)uVar1 >> 0xf);
      uVar1 = *(ushort *)
               ((ulonglong)
                ((uint)param_1 + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
               *(longlong *)_refptr_g_tsaBufDif);
      local_e = (uVar1 ^ (short)uVar1 >> 0x1f) - ((short)uVar1 >> 0xf);
    }
    sVar2 = local_c;
    if (local_c < 0) {
      sVar2 = local_c + 7;
    }
    if (sVar2 >> 3 < local_e) break;
    if (local_10 < local_c) {
      local_10 = local_c;
    }
    if (local_c < local_12) {
      local_12 = local_c;
    }
    local_9 = local_9 + 1;
  }
  return false;
}


// ============================================================================
// Function: TZ_LogDbShape @ 6ba832d0
// ============================================================================

void TZ_LogDbShape()

{
  return;
}


// ============================================================================
// Function: TZ_PeakFilter @ 6ba8a009
// ============================================================================

void TZ_PeakFilter(undefined8 param_1)

{
  char cVar1;
  
  do {
    cVar1 = TZ_PeakFilterOnce(param_1);
  } while (cVar1 != '\0');
  return;
}


// ============================================================================
// Function: TZ_IsAbsFatFinger @ 6ba8ceaf
// ============================================================================

undefined8 TZ_IsAbsFatFinger(longlong param_1)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0x4ceaf  2410  TZ_IsAbsFatFinger */
  cVar1 = TZ_IsAbsFFMinorAxis((char)*(undefined2 *)(param_1 + 0x2ee));
  if (cVar1 == '\0') {
    cVar1 = TZ_IsAbsFFRatio(*(undefined1 *)(param_1 + 0x2e8));
    if (cVar1 == '\0') {
      cVar1 = TZ_IsAbsFFZ9(*(undefined4 *)(param_1 + 0x2e4));
      if (cVar1 == '\0') {
        uVar2 = 0;
      }
      else {
        uVar2 = 1;
      }
    }
    else {
      uVar2 = 1;
    }
  }
  else {
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: TZ_JudgeFFOrMF @ 6ba8d12d
// ============================================================================

void TZ_JudgeFFOrMF(longlong param_1)

{
  int local_c;
  
                    /* 0x4d12d  2427  TZ_JudgeFFOrMF */
  local_c = 0;
  TZ_GetDFSPrpt(param_1);
  if (8 < *(uint *)(param_1 + 0x350)) {
    local_c = 2;
  }
  if (local_c == 0) {
    local_c = TZ_GetShapeBaseOnMinorAxis
                        (*(undefined2 *)(param_1 + 0x2ee),*(uint *)(param_1 + 4) & 0x20);
  }
  if (local_c == 0) {
    local_c = TZ_GetShapeBaseOnCentralSensor(param_1);
  }
  if (local_c == 0) {
    local_c = TZ_GetShapeBaseOnMajorAxis
                        (*(undefined2 *)(param_1 + 0x2ec),*(uint *)(param_1 + 4) & 0x20);
  }
  if (local_c == 0) {
    local_c = TZ_GetShapeBaseOnCentToMinPkRatio(param_1);
  }
  *(int *)(param_1 + 8) = local_c;
  return;
}


// ============================================================================
// Function: TZ_GetMaxNotGripTime @ 6ba84020
// ============================================================================

void TZ_GetMaxNotGripTime(longlong param_1)

{
  byte bVar1;
  byte local_b;
  short local_a;
  
                    /* 0x44020  2383  TZ_GetMaxNotGripTime */
  local_a = 0;
  for (local_b = 0; local_b < *(byte *)(param_1 + 0x2b2); local_b = local_b + 1) {
    bVar1 = *(byte *)(param_1 + 0x2b9 + (longlong)(int)(uint)local_b);
    if (((*(uint *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)bVar1 * 0x14 + 0x10) & 0x800) != 0)
       && (local_a < *(short *)((ulonglong)
                                ((uint)(*_refptr_g_peaksPtr)
                                       [(longlong)(int)(uint)bVar1 * 0x14 + 0x15].bPeakCount +
                                 (uint)(*_refptr_g_peaksPtr)
                                       [(longlong)(int)(uint)bVar1 * 0x14 + 0x16].bPeakCount *
                                 (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
                               *(longlong *)_refptr_g_tsaBufGripTime))) {
      local_a = *(short *)((ulonglong)
                           ((uint)(*_refptr_g_peaksPtr)[(longlong)(int)(uint)bVar1 * 0x14 + 0x15].
                                  bPeakCount +
                            (uint)(*_refptr_g_peaksPtr)[(longlong)(int)(uint)bVar1 * 0x14 + 0x16].
                                  bPeakCount * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
                          *(longlong *)_refptr_g_tsaBufGripTime);
    }
  }
  *(short *)(param_1 + 0x366) = local_a;
  return;
}


// ============================================================================
// Function: TZ_GetMinPeakGripTime @ 6ba841d4
// ============================================================================

void TZ_GetMinPeakGripTime(longlong param_1)

{
  byte bVar1;
  byte local_d;
  short local_c;
  short local_a;
  
                    /* 0x441d4  2384  TZ_GetMinPeakGripTime */
  local_a = 0x7fff;
  local_c = -0x8000;
  for (local_d = 0; local_d < *(byte *)(param_1 + 0x2b2); local_d = local_d + 1) {
    bVar1 = *(byte *)(param_1 + 0x2b9 + (longlong)(int)(uint)local_d);
    local_a = MIN((int)local_a,
                  (int)*(short *)((ulonglong)
                                  ((uint)(*_refptr_g_peaksPtr)
                                         [(longlong)(int)(uint)bVar1 * 0x14 + 0x15].bPeakCount +
                                   (uint)(*_refptr_g_peaksPtr)
                                         [(longlong)(int)(uint)bVar1 * 0x14 + 0x16].bPeakCount *
                                   (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
                                 *(longlong *)_refptr_g_tsaBufGripTime));
    local_c = MAX((int)local_c,
                  (int)*(short *)((ulonglong)
                                  ((uint)(*_refptr_g_peaksPtr)
                                         [(longlong)(int)(uint)bVar1 * 0x14 + 0x15].bPeakCount +
                                   (uint)(*_refptr_g_peaksPtr)
                                         [(longlong)(int)(uint)bVar1 * 0x14 + 0x16].bPeakCount *
                                   (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
                                 *(longlong *)_refptr_g_tsaBufGripTimeOff));
  }
  *(short *)(param_1 + 0x368) = local_a;
  *(short *)(param_1 + 0x36a) = local_c;
  return;
}


// ============================================================================
// Function: TZ_IsPeakNotInGrip @ 6ba8437e
// ============================================================================

undefined8 TZ_IsPeakNotInGrip(longlong param_1,undefined1 param_2)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0x4437e  2416  TZ_IsPeakNotInGrip */
  cVar1 = Peak_IsOnOuterEdge(param_2);
  if (cVar1 == '\0') {
    if (*(short *)(param_1 + 0x362) < 5000) {
      uVar2 = 0;
    }
    else {
      uVar2 = 1;
    }
  }
  else {
    uVar2 = 0;
  }
  return uVar2;
}


// ============================================================================
// Function: TZ_IsPeakInGrip @ 6ba843c8
// ============================================================================

undefined8 TZ_IsPeakInGrip(byte param_1)

{
  char cVar1;
  
                    /* 0x443c8  2415  TZ_IsPeakInGrip */
  if ((*(char *)((ulonglong)
                 ((uint)(byte)_refptr_g_peaksGrid[(longlong)(int)(uint)param_1 * 0x14 + 0x15] +
                  (uint)(byte)_refptr_g_peaksGrid[(longlong)(int)(uint)param_1 * 0x14 + 0x16] *
                  (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) +
                *(longlong *)_refptr_g_tsaBufGripStage) == '\x01') &&
     (cVar1 = GripFilter_IsMergeHandlerSensorEnabled
                        (_refptr_g_peaksGrid[(longlong)(int)(uint)param_1 * 0x14 + 0x15],
                         _refptr_g_peaksGrid[(longlong)(int)(uint)param_1 * 0x14 + 0x16]),
     cVar1 != '\0')) {
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: TZ_SearchMergePeak @ 6ba84a90
// ============================================================================

char TZ_SearchMergePeak(longlong param_1,short *param_2)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x1e]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x20];
  char cVar1;
  
  if ((*(uint *)(param_1 + 4) & 0x400000) == 0) {
    if (((*(uint *)(param_1 + 4) & 0x400000) == 0) && ((*(uint *)(param_1 + 4) & 0x800000) == 0)) {
      (*(char *)(_stk + 0x17)) = '\0';
    }
    else {
      (*(char *)(_stk + 0x17)) = '\0';
      (*(byte *)(_stk + 0x11)) = (byte)((ushort)*(undefined2 *)(param_1 + 0x2ae) >> 8);
      (*(byte *)(_stk + 0x10)) = (byte)((ushort)*(undefined2 *)(param_1 + 0x2b0) >> 8);
      (*(ushort *)(_stk + 0xe)) = (ushort)(*(byte *)(_stk + 0x11)) + (ushort)(*(byte *)(_stk + 0x10)) * (ushort)_refptr_g_tsaPrmtFlash->bCols;
      (*(char *)(_stk + 0xd)) = (*(uint *)(param_1 + 4) & 0x400000) == 0;
      (*(short *)(_stk + 0x14)) = TZ_GetSigTholdForMergePeak(param_1);
      if ((*(char *)(_stk + 0xd)) != '\0') {
        (*(short *)(_stk + 0x14)) = GripFilter_GetTrackingThold();
      }
      TZ_GetSearchRange(param_1,(*(char *)(_stk + 0xd)),&(*(char *)(_stk + 0x5)),&(*(char *)(_stk + 0x4)),&(*(char *)(_stk + 0x3)),&(*(char *)(_stk + 0x2)));
      for ((*(char *)(_stk + 0x13)) = (*(char *)(_stk + 0x5)); (*(char *)(_stk + 0x13)) <= (*(char *)(_stk + 0x4)); (*(char *)(_stk + 0x13)) = (*(char *)(_stk + 0x13)) + '\x01') {
        for ((*(char *)(_stk + 0x12)) = (*(char *)(_stk + 0x3)); (*(char *)(_stk + 0x12)) <= (*(char *)(_stk + 0x2)); (*(char *)(_stk + 0x12)) = (*(char *)(_stk + 0x12)) + '\x01') {
          (*(byte *)(_stk + 0xc)) = (*(byte *)(_stk + 0x11)) + (*(char *)(_stk + 0x13));
          (*(byte *)(_stk + 0xb)) = (*(byte *)(_stk + 0x10)) + (*(char *)(_stk + 0x12));
          if ((((*(byte *)(_stk + 0xc)) < _refptr_g_tsaPrmtFlash->bCols) &&
              ((*(byte *)(_stk + 0xb)) < _refptr_g_tsaPrmtFlash->bRows)) &&
             ((*(ushort *)(_stk + 0x8)) = (short)(char)(*(byte *)(_stk + 0xc)) +
                         (short)(char)(*(byte *)(_stk + 0xb)) * (ushort)_refptr_g_tsaPrmtFlash->bCols,
             (*(short *)(_stk + 0x14)) <= *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)(*(ushort *)(_stk + 0x8)) * 2))) {
            if ((*(char *)(_stk + 0xd)) == '\0') {
              if (((*(char *)((ulonglong)(*(ushort *)(_stk + 0x8)) + *(longlong *)_refptr_g_tsaBufGripStage) !=
                    '\x01') &&
                  (*(char *)((ulonglong)(*(ushort *)(_stk + 0x8)) + *(longlong *)_refptr_g_tsaBufTzZone) ==
                   *(char *)((ulonglong)(*(ushort *)(_stk + 0xe)) + *(longlong *)_refptr_g_tsaBufTzZone))) &&
                 (*param_2 < *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)(*(ushort *)(_stk + 0x8)) * 2))
                 ) goto LAB_6ba84d11;
            }
            else if ((*(char *)((ulonglong)(*(ushort *)(_stk + 0x8)) + *(longlong *)_refptr_g_tsaBufGripStage) ==
                      '\x01') &&
                    (((*(short *)(_stk + 0x6)) = (ushort)*(byte *)(param_2 + 1) +
                                 (ushort)*(byte *)((longlong)param_2 + 3) *
                                 (ushort)_refptr_g_tsaPrmtFlash->bCols, (*(char *)(_stk + 0x17)) == '\0' ||
                     (cVar1 = TZ_IsVPHighPriority((*(ushort *)(_stk + 0xe)),(*(short *)(_stk + 0x6)),(*(ushort *)(_stk + 0x8))), cVar1 == '\0')))) {
LAB_6ba84d11:
              (*(char *)(_stk + 0x17)) = '\x01';
              *param_2 = *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)(*(ushort *)(_stk + 0x8)) * 2);
              *(byte *)(param_2 + 1) = (*(byte *)(_stk + 0xc));
              *(byte *)((longlong)param_2 + 3) = (*(byte *)(_stk + 0xb));
            }
          }
        }
      }
    }
  }
  else {
    (*(char *)(_stk + 0x17)) = '\0';
  }
  return (*(char *)(_stk + 0x17));
}


// ============================================================================
// Function: TZ_InsertVirtualPeak @ 6ba8d460
// ============================================================================

bool TZ_InsertVirtualPeak(longlong param_1,undefined1 param_2,undefined1 param_3)

{
  byte bVar1;
  
                    /* 0x4d460  2405  TZ_InsertVirtualPeak */
  bVar1 = Peak_Insert(param_2,param_3);
  if (bVar1 != 0x14) {
    *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x40;
    *(uint *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)bVar1 * 0x14 + 0x10) =
         *(uint *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)bVar1 * 0x14 + 0x10) | 0x100;
    TZ_PeakRegister(param_1,bVar1);
  }
  return bVar1 != 0x14;
}


// ============================================================================
// Function: TZ_IsPossibleGripMerge @ 6ba83fcd
// ============================================================================

undefined8 TZ_IsPossibleGripMerge(undefined8 param_1)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0x43fcd  2418  TZ_IsPossibleGripMerge */
  cVar1 = TZ_IsTouchMerged(param_1);
  if (cVar1 == '\0') {
    cVar1 = TZ_IsLineJump(param_1);
    if (cVar1 == '\0') {
      cVar1 = TZ_IsPoorEllipseRate(param_1);
      if (cVar1 == '\0') {
        uVar2 = 0;
      }
      else {
        uVar2 = 1;
      }
    }
    else {
      uVar2 = 1;
    }
  }
  else {
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: TZ_IsTimeOnIntervalOK @ 6ba8513a
// ============================================================================

bool TZ_IsTimeOnIntervalOK(longlong param_1)

{
  ushort local_a;
  
                    /* 0x4513a  2421  TZ_IsTimeOnIntervalOK */
  local_a = *(ushort *)&_refptr_g_tsaPrmtFlash[1].field_0x7a;
  if ((*(uint *)(param_1 + 4) & 0x40000000) != 0) {
    local_a = local_a >> 1;
  }
  return (int)(uint)local_a < (int)*(short *)(param_1 + 0x364) - (int)*(short *)(param_1 + 0x366);
}


// ============================================================================
// Function: TZ_CalcHighResDif @ 6ba8607c
// ============================================================================

void TZ_CalcHighResDif(longlong param_1,int param_2)

{
  int iVar1;
  int iVar2;
  uint local_10;
  int local_c;
  
  for (local_c = (int)*(short *)(param_1 + 0x46); local_c <= *(short *)(param_1 + 0x48);
      local_c = local_c + 1) {
    for (local_10 = (uint)*(short *)(param_1 + 0x4a);
        (int)local_10 <= (int)*(short *)(param_1 + 0x4c); local_10 = local_10 + 1) {
      iVar1 = local_c + (local_10 & 0xffff) * (uint)_refptr_g_tsaPrmtFlash->bCols * 4;
      iVar2 = TZ_GetDifHR(param_1,local_c,local_10);
      *(short *)(&g_tzBufferHighRes + (longlong)iVar1 * 2) = (short)iVar2;
      if (param_2 < iVar2) {
        (&g_tzBufferBin)[iVar1] = 1;
      }
    }
  }
  return;
}


// ============================================================================
// Function: TZ_GetDifHR @ 6ba85fac
// ============================================================================

int TZ_GetDifHR(longlong param_1,int param_2,int param_3)

{
  int iVar1;
  int iVar2;
  float fVar3;
  
  if ((param_2 < *(short *)(param_1 + 0x46)) || (*(short *)(param_1 + 0x48) < param_2)) {
    iVar2 = 0;
  }
  else if ((param_3 < *(short *)(param_1 + 0x4a)) || (*(short *)(param_1 + 0x4c) < param_3)) {
    iVar2 = 0;
  }
  else {
    iVar2 = param_2;
    if (param_2 < 0) {
      iVar2 = param_2 + 3;
    }
    iVar1 = param_3;
    if (param_3 < 0) {
      iVar1 = param_3 + 3;
    }
    fVar3 = (float)TZ_BiCubicInter(iVar2 >> 2,iVar1 >> 2,param_2 + (iVar2 >> 2) * -4,
                                   param_3 + (iVar1 >> 2) * -4);
    iVar2 = (int)fVar3;
  }
  return iVar2;
}


// ============================================================================
// Function: TZ_ShowContourInIDE @ 6ba862eb
// ============================================================================

void TZ_ShowContourInIDE(longlong param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int local_c;
  
  for (local_c = 0; local_c < *(short *)(param_1 + 0x2a6); local_c = local_c + 1) {
    iVar1 = (int)*(short *)(param_1 + 0xe + ((longlong)local_c + 0x10) * 4);
    iVar2 = iVar1 * 0x100;
    if (iVar1 << 8 < 0) {
      iVar2 = iVar2 + 3;
    }
    iVar3 = (int)*(short *)(param_1 + 0x50 + (longlong)local_c * 4);
    iVar1 = iVar3 * 0x100;
    if (iVar3 << 8 < 0) {
      iVar1 = iVar1 + 3;
    }
    TSAIDE_LogPoint((iVar2 >> 2) + 0x80U & 0xffff,(iVar1 >> 2) + 0x80U & 0xffff,0);
  }
  return;
}


// ============================================================================
// Function: TZ_FindContourFirstDotDim2 @ 6ba86380
// ============================================================================

int TZ_FindContourFirstDotDim2(longlong param_1,undefined4 param_2,int param_3,int param_4)

{
  int iVar1;
  int local_c;
  
  local_c = (uint)(*_refptr_g_peaksPtr)[(longlong)param_4 * 0x14 + 0x16].bPeakCount << 2;
  while( true ) {
    if (local_c < *(short *)(param_1 + 0x4a)) {
      return local_c;
    }
    iVar1 = TZ_GetDifHR(param_1,param_2,local_c);
    if (iVar1 < param_3) break;
    if (*(short *)(param_1 + 0x4a) == local_c) {
      return local_c;
    }
    local_c = local_c + -1;
  }
  return local_c + 1;
}


// ============================================================================
// Function: TZ_PushEdgeDot @ 6ba8641c
// ============================================================================

void TZ_PushEdgeDot(longlong param_1,undefined2 param_2,undefined2 param_3)

{
  if (*(short *)(param_1 + 0x2a6) < 0x96) {
    *(undefined2 *)(param_1 + 0xe + ((longlong)(int)*(short *)(param_1 + 0x2a6) + 0x10) * 4) =
         param_2;
    *(undefined2 *)(param_1 + 0x50 + (longlong)(int)*(short *)(param_1 + 0x2a6) * 4) = param_3;
    *(short *)(param_1 + 0x2a6) = *(short *)(param_1 + 0x2a6) + 1;
  }
  return;
}


// ============================================================================
// Function: TZ_GetERInernal @ 6ba86b64
// ============================================================================

undefined2 TZ_GetERInernal(longlong param_1,int param_2)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0xad]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0xb0];
  uint uVar1;
  double dVar2;
  
  TZ_CalcAxisMass(&(*(double *)(_stk + 0x8)),param_1,param_2);
  if ((*(double *)(_stk + 0x8)) == 0.0) {
    (*(double *)(_stk + 0x8)) = 1.0;
  }
  (*(double *)(_stk + 0x88)) = (*(double *)(_stk + 0x10)) / (*(double *)(_stk + 0x8));
  (*(double *)(_stk + 0x80)) = (*(double *)(_stk + 0x18)) / (*(double *)(_stk + 0x8));
  TZ_GetAxisInternal((*(double *)(_stk + 0x8)),(*(double *)(_stk + 0x10)),(*(double *)(_stk + 0x18)),(*(undefined8 *)(_stk + 0x20)),(*(undefined8 *)(_stk + 0x28)),(*(undefined8 *)(_stk + 0x30)),&(*(ushort *)(_stk + 0x6)),&(*(ushort *)(_stk + 0x4)),
                     &(*(char *)(_stk + 0x3)));
  uVar1 = (uint)(*(ushort *)(_stk + 0x6));
  (*(ushort *)(_stk + 0x6)) = (ushort)(int)((double)uVar1 / 2.449489742783178);
  (*(ushort *)(_stk + 0x4)) = (ushort)(int)((double)(*(ushort *)(_stk + 0x4)) / 2.449489742783178);
  (*(double *)(_stk + 0x78)) = ((double)(int)(*(char *)(_stk + 0x3)) * 3.14159265) / 180.0;
  (*(double *)(_stk + 0xa0)) = 0.0;
  (*(double *)(_stk + 0x98)) = 0.0;
  if ((*(ushort *)(_stk + 0x4)) == 0) {
    (*(ushort *)(_stk + 0x4)) = 1;
  }
  (*(double *)(_stk + 0x70)) = (double)((int)((double)uVar1 / 2.449489742783178) & 0xffff) / (double)(*(ushort *)(_stk + 0x4));
  (*(double *)(_stk + 0x68)) = cos((*(double *)(_stk + 0x78)));
  (*(double *)(_stk + 0x60)) = sin((*(double *)(_stk + 0x78)));
  for ((*(int *)(_stk + 0x94)) = 0; (*(int *)(_stk + 0x94)) < param_2; (*(int *)(_stk + 0x94)) = (*(int *)(_stk + 0x94)) + 1) {
    (*(double *)(_stk + 0x58)) = (double)(int)*(short *)(param_1 + (longlong)(*(int *)(_stk + 0x94)) * 4) - (*(double *)(_stk + 0x88));
    (*(double *)(_stk + 0x50)) = (double)(int)*(short *)(param_1 + (longlong)(*(int *)(_stk + 0x94)) * 4 + 2) - (*(double *)(_stk + 0x80));
    (*(double *)(_stk + 0x48)) = (*(double *)(_stk + 0x58)) * (*(double *)(_stk + 0x68)) + (*(double *)(_stk + 0x50)) * (*(double *)(_stk + 0x60));
    (*(double *)(_stk + 0x40)) = (-(*(double *)(_stk + 0x58)) * (*(double *)(_stk + 0x60)) + (*(double *)(_stk + 0x50)) * (*(double *)(_stk + 0x68))) * (*(double *)(_stk + 0x70));
    (*(double *)(_stk + 0x38)) = (*(double *)(_stk + 0x48)) * (*(double *)(_stk + 0x48)) + (*(double *)(_stk + 0x40)) * (*(double *)(_stk + 0x40));
    (*(double *)(_stk + 0x98)) = (*(double *)(_stk + 0x98)) + (*(double *)(_stk + 0x38));
    dVar2 = sqrt((*(double *)(_stk + 0x38)));
    (*(double *)(_stk + 0xa0)) = (*(double *)(_stk + 0xa0)) + dVar2;
  }
  (*(undefined2 *)(_stk + 0x92)) = 0;
  if ((param_2 != 0) && ((*(double *)(_stk + 0xa0)) != 0.0)) {
    dVar2 = sqrt(((double)param_2 * (*(double *)(_stk + 0x98))) / ((*(double *)(_stk + 0xa0)) * (*(double *)(_stk + 0xa0))) - 1.0);
    (*(undefined2 *)(_stk + 0x92)) = (undefined2)(int)(dVar2 * 100.0);
  }
  return (*(undefined2 *)(_stk + 0x92));
}


// ============================================================================
// Function: TZ_CalcEdgeCount @ 6ba8707e
// ============================================================================

void TZ_CalcEdgeCount(longlong param_1,longlong param_2,uint param_3)

{
  uint uVar1;
  int *piVar2;
  int local_10;
  uint local_c;
  
  for (local_c = 0; local_c < param_3; local_c = local_c + 1) {
    *(undefined4 *)(param_2 + (ulonglong)local_c * 4) = 0;
  }
  for (local_10 = 0; local_10 < *(short *)(param_1 + 0x2a6); local_10 = local_10 + 1) {
    uVar1 = TZ_GetEdgeDotType((int)*(short *)(param_1 + 0xe + ((longlong)local_10 + 0x10) * 4),
                              (int)*(short *)(param_1 + 0x50 + (longlong)local_10 * 4));
    for (local_c = 0; local_c < param_3; local_c = local_c + 1) {
      if ((1 << ((byte)local_c & 0x1f) & uVar1) != 0) {
        piVar2 = (int *)(param_2 + (ulonglong)local_c * 4);
        *piVar2 = *piVar2 + 1;
      }
    }
  }
  return;
}


// ============================================================================
// Function: TZ_RemoveEdgeDot @ 6ba87167
// ============================================================================

void TZ_RemoveEdgeDot(longlong param_1,short *param_2,byte param_3)

{
  short sVar1;
  short sVar2;
  uint uVar3;
  int local_10;
  uint local_c;
  
  local_c = 0;
  for (local_10 = 0; local_10 < *param_2; local_10 = local_10 + 1) {
    sVar1 = *(short *)(param_1 + (longlong)local_10 * 4);
    sVar2 = *(short *)(param_1 + (longlong)local_10 * 4 + 2);
    uVar3 = TZ_GetEdgeDotType((int)sVar1,(int)sVar2);
    if ((1 << (param_3 & 0x1f) & uVar3) == 0) {
      *(short *)(param_1 + (ulonglong)local_c * 4) = sVar1;
      *(short *)(param_1 + (ulonglong)local_c * 4 + 2) = sVar2;
      local_c = local_c + 1;
    }
  }
  *param_2 = (short)local_c;
  return;
}


// ============================================================================
// Function: TZ_CopyEdgeDot @ 6ba87246
// ============================================================================

void TZ_CopyEdgeDot(longlong param_1,longlong param_2)

{
  int local_c;
  
  for (local_c = 0; local_c < *(short *)(param_2 + 0x2a6); local_c = local_c + 1) {
    *(undefined2 *)((longlong)local_c * 4 + param_1) =
         *(undefined2 *)(param_2 + 0xe + ((longlong)local_c + 0x10) * 4);
    *(undefined2 *)((longlong)local_c * 4 + param_1 + 2) =
         *(undefined2 *)(param_2 + 0x50 + (longlong)local_c * 4);
  }
  return;
}


// ============================================================================
// Function: TZ_MirrorEdgeDot @ 6ba872cc
// ============================================================================

void TZ_MirrorEdgeDot(longlong param_1,short *param_2,int param_3)

{
  short sVar1;
  short sVar2;
  int local_10;
  uint local_c;
  
  local_10 = 0;
  for (local_c = (uint)*param_2; (local_10 < *param_2 && (local_c < 0x96)); local_c = local_c + 1) {
    sVar1 = *(short *)(param_1 + (longlong)local_10 * 4);
    sVar2 = *(short *)(param_1 + (longlong)local_10 * 4 + 2);
    if (param_3 == 1) {
      sVar1 = (_refptr_g_tsaPrmtFlash->bCols - 1) * 8 - sVar1;
    }
    else if (param_3 == 0) {
      sVar1 = 2 - sVar1;
    }
    else if (param_3 == 2) {
      sVar2 = 2 - sVar2;
    }
    else if (param_3 == 3) {
      sVar2 = (_refptr_g_tsaPrmtFlash->bRows - 1) * 8 - sVar2;
    }
    *(short *)(param_1 + (ulonglong)local_c * 4) = sVar1;
    *(short *)(param_1 + (ulonglong)local_c * 4 + 2) = sVar2;
    local_10 = local_10 + 1;
  }
  *param_2 = (short)local_c;
  return;
}


// ============================================================================
// Function: TZ_GetMaxEdgeType @ 6ba87437
// ============================================================================

uint TZ_GetMaxEdgeType(longlong param_1,int param_2)

{
  uint local_10;
  uint local_c;
  
  local_c = 0;
  for (local_10 = 0; (int)local_10 < param_2; local_10 = local_10 + 1) {
    if (*(uint *)(param_1 + (ulonglong)local_c * 4) <
        *(uint *)(param_1 + (longlong)(int)local_10 * 4)) {
      local_c = local_10;
    }
  }
  return local_c;
}


// ============================================================================
// Function: TZ_GetPrevAge @ 6ba8a186
// ============================================================================

undefined2 TZ_GetPrevAge(byte param_1)

{
  byte bVar1;
  undefined2 local_a;
  
                    /* 0x4a186  2388  TZ_GetPrevAge */
  local_a = 0;
  if ((*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_1 * 0x14 + 0x14].bPeakCount == '\0') {
    local_a = 0;
  }
  else {
    bVar1 = PrevTouch_GetIdxBasedOnID
                      ((*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_1 * 0x14 + 0x14].bPeakCount)
    ;
    if (bVar1 == 0x14) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Alg/TouchZone/TZ_ShapeDebounce.c",0x3b,
                        "tchIdx != TOUCH_INVALID_IDX");
      HardwareAnalyzer_SetSoftException();
    }
    if (bVar1 < 0x14) {
      local_a = *(undefined2 *)
                 ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x156);
    }
  }
  return local_a;
}


// ============================================================================
// Function: TZ_GetSigTholdForVPA @ 6ba8d519
// ============================================================================

undefined2 TZ_GetSigTholdForVPA()

{
  return *(undefined2 *)(_refptr_g_tsaPrmtDynamic + 8);
}


// ============================================================================
// Function: TZ_SearchNxN @ 6ba8d53e
// ============================================================================

undefined1 TZ_SearchNxN(char param_1,char param_2,short param_3,short *param_4)

{
  byte bVar1;
  byte bVar2;
  char cVar3;
  ushort uVar4;
  byte local_b;
  byte local_a;
  undefined1 local_9;
  
  local_9 = 0;
  for (local_a = 0; local_a < 5; local_a = local_a + 1) {
    for (local_b = 0; local_b < 5; local_b = local_b + 1) {
      if ((((local_a == 0) || (local_a == 4)) || (local_b == 0)) || (local_b == 4)) {
        bVar1 = (param_1 + local_a) - 2;
        bVar2 = (param_2 + local_b) - 2;
        if (((bVar1 < _refptr_g_tsaPrmtFlash->bCols) && (bVar2 < _refptr_g_tsaPrmtFlash->bRows)) &&
           ((uVar4 = (short)(char)bVar1 + (short)(char)bVar2 * (ushort)_refptr_g_tsaPrmtFlash->bCols
            , param_3 <= *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)uVar4 * 2) &&
            ((*param_4 < *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)uVar4 * 2) &&
             (cVar3 = Peak_IsInNxNBox((int)(char)bVar1,(int)(char)bVar2,3), cVar3 == '\0')))))) {
          local_9 = 1;
          *param_4 = *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)uVar4 * 2);
          *(byte *)(param_4 + 1) = bVar1;
          *(byte *)((longlong)param_4 + 3) = bVar2;
        }
      }
    }
  }
  return local_9;
}


// ============================================================================
// Function: TZ_GetExceptionRatio @ 6ba8244c
// ============================================================================

void TZ_GetExceptionRatio(longlong param_1)

{
  byte bVar1;
  undefined2 uVar2;
  
                    /* 0x4244c  2379  TZ_GetExceptionRatio */
  uVar2 = TZ_GetAbsRatio(*(undefined4 *)(param_1 + 0x28),*(undefined4 *)(param_1 + 0x18));
  *(undefined2 *)(param_1 + 0x33c) = uVar2;
  uVar2 = TZ_GetAbsRatio((int)*(short *)(param_1 + 0x2c),
                         (int)*(short *)(*_refptr_g_peaksPtr +
                                        (longlong)(int)(uint)*(byte *)(param_1 + 0x2b6) * 0x14 +
                                        0x18));
  *(undefined2 *)(param_1 + 0x33e) = uVar2;
  uVar2 = TZ_GetAbsRatio((int)*(short *)(param_1 + 0x2c),
                         *(undefined4 *)
                          (*_refptr_g_peaksPtr +
                          (longlong)(int)(uint)*(byte *)(param_1 + 0x2b6) * 0x14 + 0x1c));
  *(undefined2 *)(param_1 + 0x340) = uVar2;
  uVar2 = TZ_GetAbsRatio(*(undefined4 *)(param_1 + 0x18),
                         (int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold);
  *(undefined2 *)(param_1 + 0x342) = uVar2;
  bVar1 = (*_refptr_g_peaksPtr)[(longlong)(int)(uint)*(byte *)(param_1 + 0x2b8) * 0x14 + 0x16].
          bPeakCount;
  uVar2 = TZ_GetRatio(*(undefined4 *)
                       (_refptr_g_tsaBufSelf +
                       ((longlong)
                        (int)(uint)(*_refptr_g_peaksPtr)
                                   [(longlong)(int)(uint)*(byte *)(param_1 + 0x2b8) * 0x14 + 0x15].
                                   bPeakCount + 0x500) * 4),
                      (int)*(short *)(*_refptr_g_peaksPtr +
                                     (longlong)(int)(uint)*(byte *)(param_1 + 0x2b8) * 0x14 + 0x18))
  ;
  *(undefined2 *)(param_1 + 0x344) = uVar2;
  uVar2 = TZ_GetRatio(*(undefined4 *)
                       (_refptr_g_tsaBufSelf +
                       ((longlong)(int)((uint)bVar1 + (uint)_refptr_g_tsaPrmtFlash->bCols) + 0x500)
                       * 4),(int)*(short *)(*_refptr_g_peaksPtr +
                                           (longlong)(int)(uint)*(byte *)(param_1 + 0x2b8) * 0x14 +
                                           0x18));
  *(undefined2 *)(param_1 + 0x346) = uVar2;
  return;
}


// ============================================================================
// Function: TZ_GetAveCentSig @ 6ba82b60
// ============================================================================

undefined2 TZ_GetAveCentSig(undefined8 param_1,char param_2,char param_3,byte param_4,uint param_5)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x10]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x10];
  
  ((byte *)(_stk + 0x5))[0] = 0;
  (*(int *)(_stk + 0x0)) = 0;
  TZ_GetCentInfo(param_1,param_2,param_3,((byte *)(_stk + 0x5)),&(*(int *)(_stk + 0x0)));
  if ((param_4 & 1) != 0) {
    TZ_GetCentInfo(param_1,param_2 + '\x01',param_3,((byte *)(_stk + 0x5)),&(*(int *)(_stk + 0x0)));
  }
  if ((param_5 & 1) != 0) {
    TZ_GetCentInfo(param_1,param_2,param_3 + '\x01',((byte *)(_stk + 0x5)),&(*(int *)(_stk + 0x0)));
  }
  if (((param_5 & 1) != 0) && ((param_4 & 1) != 0)) {
    TZ_GetCentInfo(param_1,param_2 + '\x01',param_3 + '\x01',((byte *)(_stk + 0x5)),&(*(int *)(_stk + 0x0)));
  }
  if (((byte *)(_stk + 0x5))[0] == 0) {
    ((byte *)(_stk + 0x5))[0] = 1;
  }
  return (short)((*(int *)(_stk + 0x0)) / (int)(uint)((byte *)(_stk + 0x5))[0]);
}


// ============================================================================
// Function: TZ_UpdateAveCentSig @ 6ba82c8d
// ============================================================================

short TZ_UpdateAveCentSig(byte param_1,byte param_2,byte param_3,byte param_4,short param_5)

{
  short local_1c;
  ushort local_10;
  ushort local_e;
  byte local_b;
  byte local_a;
  byte local_9;
  
  local_1c = param_5;
  if (param_1 == param_3) {
    if (param_4 < param_2) {
      local_b = param_2;
      local_a = param_4;
    }
    else {
      local_b = param_4;
      local_a = param_2;
    }
    local_1c = 0x7fff;
    local_e = (ushort)param_1 + (ushort)local_a * (ushort)_refptr_g_tsaPrmtFlash->bCols;
    while (local_9 = local_a + 1, local_9 < local_b) {
      local_e = local_e + _refptr_g_tsaPrmtFlash->bCols;
      if (*(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)local_e * 2) < local_1c) {
        local_1c = *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)local_e * 2);
      }
      local_a = local_9;
    }
  }
  else if (param_2 == param_4) {
    if (param_3 < param_1) {
      local_b = param_1;
      local_a = param_3;
    }
    else {
      local_b = param_3;
      local_a = param_1;
    }
    local_1c = 0x7fff;
    local_10 = (ushort)local_a + (ushort)param_2 * (ushort)_refptr_g_tsaPrmtFlash->bCols;
    while (local_9 = local_a + 1, local_9 < local_b) {
      local_10 = local_10 + 1;
      if (*(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)local_10 * 2) < local_1c) {
        local_1c = *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)local_10 * 2);
      }
      local_a = local_9;
    }
  }
  return local_1c;
}


// ============================================================================
// Function: TZ_TryExtraMergePeak @ 6ba84dea
// ============================================================================

void TZ_TryExtraMergePeak(longlong param_1,undefined1 param_2,undefined1 param_3)

{
                    /* 0x44dea  2446  TZ_TryExtraMergePeak */
  if ((*(uint *)(param_1 + 4) & 0x4000000) == 0) {
    *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x4000000;
    TZ_InsertVirtualPeak(param_1,param_2,param_3);
    TZ_GetPeakGripFlag(param_1);
  }
  return;
}


// ============================================================================
// Function: TZ_PeakFilterOnce @ 6ba89cc0
// ============================================================================

undefined8 TZ_PeakFilterOnce(void *param_1)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x388]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x390];
  byte bVar1;
  
                    /* 0x49cc0  2436  TZ_PeakFilterOnce */
  if (2 < *(byte *)((longlong)param_1 + 0x2b2)) {
    memcpy(((undefined1 *)(_stk + 0x8)),param_1,0x370);
    for ((*(byte *)(_stk + 0x387)) = 0; (int)(uint)(*(byte *)(_stk + 0x387)) < (int)(*(byte *)((longlong)param_1 + 0x2b2) - 1);
        (*(byte *)(_stk + 0x387)) = (*(byte *)(_stk + 0x387)) + 1) {
      (*(byte *)(_stk + 0x385)) = *(byte *)((longlong)param_1 + (longlong)(int)(uint)(*(byte *)(_stk + 0x387)) + 0x2b9);
      (*(byte *)(_stk + 0x386)) = (*(byte *)(_stk + 0x387));
      while ((*(byte *)(_stk + 0x386)) = (*(byte *)(_stk + 0x386)) + 1, (*(byte *)(_stk + 0x386)) < *(byte *)((longlong)param_1 + 0x2b2)) {
        (*(byte *)(_stk + 0x384)) = *(byte *)((longlong)param_1 + (longlong)(int)(uint)(*(byte *)(_stk + 0x386)) + 0x2b9);
        bVar1 = Peak_GetDistance((*(byte *)(_stk + 0x385)),(*(byte *)(_stk + 0x384)));
        if (bVar1 < 3) {
          (*(undefined1 *)(_stk + 0x2ba)) = 2;
          (*(byte *)(_stk + 0x2c1)) = (*(byte *)(_stk + 0x385));
          (*(byte *)(_stk + 0x2c2)) = (*(byte *)(_stk + 0x384));
          if (*(short *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)(*(byte *)(_stk + 0x385)) * 0x14 + 0x18) <
              *(short *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)(*(byte *)(_stk + 0x384)) * 0x14 + 0x18)) {
            (*(byte *)(_stk + 0x2bf)) = (*(byte *)(_stk + 0x385));
          }
          else {
            (*(byte *)(_stk + 0x2bf)) = (*(byte *)(_stk + 0x384));
          }
          TZ_GetDFSPrpt(((undefined1 *)(_stk + 0x8)));
          if (((byte)_refptr_g_tsaPrmtFlash->field_0x832 < (*(byte *)(_stk + 0x2f0))) && (0x30 < (*(byte *)(_stk + 0x3a)))) {
            if (((*_refptr_g_peaksPtr)[(longlong)(int)(uint)(*(byte *)(_stk + 0x385)) * 0x14 + 0x14].bPeakCount ==
                 '\0') &&
               ((*_refptr_g_peaksPtr)[(longlong)(int)(uint)(*(byte *)(_stk + 0x384)) * 0x14 + 0x14].bPeakCount !=
                '\x14')) {
              (*_refptr_g_peaksPtr)[(longlong)(int)(uint)(*(byte *)(_stk + 0x385)) * 0x14 + 0x14].bPeakCount =
                   (*_refptr_g_peaksPtr)[(longlong)(int)(uint)(*(byte *)(_stk + 0x384)) * 0x14 + 0x14].bPeakCount;
            }
            for (; (int)(uint)(*(byte *)(_stk + 0x386)) < (int)(*(byte *)((longlong)param_1 + 0x2b2) - 1);
                (*(byte *)(_stk + 0x386)) = (*(byte *)(_stk + 0x386)) + 1) {
              *(undefined1 *)((longlong)param_1 + (longlong)(int)(uint)(*(byte *)(_stk + 0x386)) + 0x2b9) =
                   *(undefined1 *)((longlong)param_1 + (longlong)(int)((*(byte *)(_stk + 0x386)) + 1) + 0x2b9);
            }
            *(char *)((longlong)param_1 + 0x2b2) = *(char *)((longlong)param_1 + 0x2b2) + -1;
            return 1;
          }
        }
      }
    }
  }
  return 0;
}


// ============================================================================
// Function: TZ_IsAbsFFZ9 @ 6ba8ce30
// ============================================================================

bool TZ_IsAbsFFZ9(int param_1)

{
                    /* 0x4ce30  2409  TZ_IsAbsFFZ9 */
  return *(int *)&_refptr_g_tsaPrmtFlash->field_0x828 < param_1;
}


// ============================================================================
// Function: TZ_IsAbsFFRatio @ 6ba8ce57
// ============================================================================

bool TZ_IsAbsFFRatio(byte param_1)

{
                    /* 0x4ce57  2408  TZ_IsAbsFFRatio */
  return (byte)_refptr_g_tsaPrmtFlash->field_0x830 < param_1;
}


// ============================================================================
// Function: TZ_GetDFSPrpt @ 6ba830c5
// ============================================================================

void TZ_GetDFSPrpt(longlong param_1)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x12]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x20];
  short sVar1;
  undefined1 uVar2;
  
                    /* 0x430c5  2375  TZ_GetDFSPrpt */
  (*(int *)(_stk + 0x14)) = 0;
  if (*(char *)(param_1 + 0x2b2) != '\0') {
    (*(int *)(_stk + 0x14)) = *(int *)(param_1 + 0x18) / (int)(uint)*(byte *)(param_1 + 0x2b2);
  }
  (*(int *)(_stk + 0x14)) = (*(int *)(_stk + 0x14)) << 4;
  if ((*(uint *)(param_1 + 4) & 0x100) == 0) {
    (*(int *)(_stk + 0x14)) = (*(int *)(_stk + 0x14)) / (int)*(short *)_refptr_g_tsaPrmtDynamic;
    sVar1 = *(short *)_refptr_g_tsaPrmtDynamic;
  }
  else {
    if (*(short *)(*_refptr_g_peaksPtr +
                  (longlong)(int)(uint)*(byte *)(param_1 + 0x2b6) * 0x14 + 0x18) != 0) {
      (*(int *)(_stk + 0x14)) = (*(int *)(_stk + 0x14)) / (int)*(short *)(*_refptr_g_peaksPtr +
                                         (longlong)(int)(uint)*(byte *)(param_1 + 0x2b6) * 0x14 +
                                         0x18);
    }
    sVar1 = *(short *)(*_refptr_g_peaksPtr +
                      (longlong)(int)(uint)*(byte *)(param_1 + 0x2b6) * 0x14 + 0x18);
  }
  (*(int *)(_stk + 0x10)) = (int)sVar1 * (uint)*(ushort *)(param_1 + 0x2ee) * (uint)*(ushort *)(param_1 + 0x2ec);
  if (*(int *)(param_1 + 0x18) != 0) {
    (*(int *)(_stk + 0x10)) = (*(int *)(_stk + 0x10)) / *(int *)(param_1 + 0x18);
  }
  if ((*(int *)(_stk + 0x14)) < 0x100) {
    *(char *)(param_1 + 0x32) = (char)(*(int *)(_stk + 0x14));
  }
  else {
    *(undefined1 *)(param_1 + 0x32) = 0xff;
  }
  if ((*(int *)(_stk + 0x10)) < 0x100) {
    *(int *)(param_1 + 800) = (*(int *)(_stk + 0x10));
  }
  else {
    *(undefined4 *)(param_1 + 800) = 0xff;
  }
  TZ_Get2ClosestPeaks(param_1,&(*(undefined1 *)(_stk + 0xf)),&(*(undefined1 *)(_stk + 0xe)));
  uVar2 = TZ_GetAutoCentToMinPkRatio(param_1,(*(undefined1 *)(_stk + 0xf)),(*(undefined1 *)(_stk + 0xe)));
  *(undefined1 *)(param_1 + 0x2e8) = uVar2;
  return;
}


// ============================================================================
// Function: TZ_GetShapeBaseOnCentToMinPkRatio @ 6ba8cf22
// ============================================================================

undefined4 TZ_GetShapeBaseOnCentToMinPkRatio(longlong param_1)

{
  byte bVar1;
  undefined4 local_c;
  
                    /* 0x4cf22  2392  TZ_GetShapeBaseOnCentToMinPkRatio */
  bVar1 = *(byte *)(param_1 + 0x2e8);
  if (*(int *)(param_1 + 0x350) == 0) {
    if ((byte)_refptr_g_tsaPrmtFlash->field_0x830 < bVar1) {
      local_c = 2;
    }
    else if (bVar1 < (byte)_refptr_g_tsaPrmtFlash->field_0x831) {
      local_c = 3;
    }
    else if (bVar1 < (byte)_refptr_g_tsaPrmtFlash->field_0x833) {
      local_c = 3;
    }
    else {
      local_c = 2;
    }
  }
  else if (bVar1 < (byte)_refptr_g_tsaPrmtFlash->field_0x833 >> 1) {
    local_c = 3;
  }
  else {
    local_c = 2;
  }
  return local_c;
}


// ============================================================================
// Function: TZ_GetShapeBaseOnMinorAxis @ 6ba8cfd5
// ============================================================================

undefined4 TZ_GetShapeBaseOnMinorAxis(ushort param_1, ...)

{
  undefined4 local_c;
  
                    /* 0x4cfd5  2395  TZ_GetShapeBaseOnMinorAxis */
  local_c = 0;
  if (*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x82e < param_1) {
    local_c = 2;
  }
  return local_c;
}


// ============================================================================
// Function: TZ_GetShapeBaseOnMajorAxis @ 6ba8d011
// ============================================================================

undefined4 TZ_GetShapeBaseOnMajorAxis(ushort param_1,char param_2)

{
  undefined4 local_c;
  
                    /* 0x4d011  2394  TZ_GetShapeBaseOnMajorAxis */
  local_c = 0;
  if ((param_1 < *(ushort *)&_refptr_g_tsaPrmtFlash->field_0x82c) && (param_2 == '\0')) {
    local_c = 2;
  }
  return local_c;
}


// ============================================================================
// Function: TZ_GetShapeBaseOnCentralSensor @ 6ba8d053
// ============================================================================

undefined4 TZ_GetShapeBaseOnCentralSensor(longlong param_1)

{
  byte bVar1;
  byte bVar2;
  int iVar3;
  undefined4 local_c;
  
                    /* 0x4d053  2393  TZ_GetShapeBaseOnCentralSensor */
  local_c = 0;
  bVar1 = (byte)((ushort)*(undefined2 *)(param_1 + 0x2ae) >> 8);
  bVar2 = (byte)((ushort)*(undefined2 *)(param_1 + 0x2b0) >> 8);
  if (*(short *)((ulonglong)
                 ((uint)bVar1 + (uint)bVar2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
                *(longlong *)_refptr_g_tsaBufDif) < 0) {
    iVar3 = -1;
  }
  else {
    iVar3 = 1;
  }
  if (*(char *)((ulonglong)
                ((uint)bVar1 + (uint)bVar2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) +
               *(longlong *)_refptr_g_tzAccess) * iVar3 -
      (uint)*(byte *)(*(longlong *)_refptr_g_ptrAccess + 9) == 6) {
    local_c = 2;
  }
  return local_c;
}


// ============================================================================
// Function: TZ_GetSigTholdForMergePeak @ 6ba83afc
// ============================================================================

ushort TZ_GetSigTholdForMergePeak()

{
  return _refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax;
}


// ============================================================================
// Function: TZ_GetSearchRange @ 6ba846aa
// ============================================================================

void TZ_GetSearchRange(longlong param_1,char param_2,char *param_3,undefined1 *param_4,char *param_5
                      ,undefined1 *param_6)

{
  *param_3 = -_refptr_g_tsaPrmtFlash[1].field_0x79;
  *param_4 = _refptr_g_tsaPrmtFlash[1].field_0x79;
  *param_5 = -_refptr_g_tsaPrmtFlash[1].field_0x79;
  *param_6 = _refptr_g_tsaPrmtFlash[1].field_0x79;
  if (param_2 == '\0') {
    if ((int)(uint)*(byte *)(param_1 + 0x34) < (int)(_refptr_g_tsaPrmtFlash->bCols - 2)) {
      if (*(byte *)(param_1 + 0x33) < 2) {
        *param_3 = -1;
        *param_4 = _refptr_g_tsaPrmtFlash[1].field_0x79;
      }
    }
    else {
      *param_3 = -_refptr_g_tsaPrmtFlash[1].field_0x79;
      *param_4 = 1;
    }
    if ((int)(uint)*(byte *)(param_1 + 0x36) < (int)(_refptr_g_tsaPrmtFlash->bCols - 2)) {
      if (*(byte *)(param_1 + 0x35) < 2) {
        *param_5 = -1;
        *param_6 = _refptr_g_tsaPrmtFlash[1].field_0x79;
      }
    }
    else {
      *param_5 = -_refptr_g_tsaPrmtFlash[1].field_0x79;
      *param_6 = 1;
    }
  }
  else {
    if (*(byte *)(param_1 + 0x33) < 2) {
      *param_3 = -_refptr_g_tsaPrmtFlash[1].field_0x79;
      *param_4 = 1;
    }
    else if ((int)(_refptr_g_tsaPrmtFlash->bCols - 2) <= (int)(uint)*(byte *)(param_1 + 0x34)) {
      *param_3 = -1;
      *param_4 = _refptr_g_tsaPrmtFlash[1].field_0x79;
    }
    if (*(byte *)(param_1 + 0x35) < 2) {
      *param_5 = -_refptr_g_tsaPrmtFlash[1].field_0x79;
      *param_6 = 1;
    }
    else if ((int)(_refptr_g_tsaPrmtFlash->bRows - 2) <= (int)(uint)*(byte *)(param_1 + 0x36)) {
      *param_5 = -1;
      *param_6 = _refptr_g_tsaPrmtFlash[1].field_0x79;
    }
  }
  return;
}


// ============================================================================
// Function: TZ_IsVPHighPriority @ 6ba848db
// ============================================================================

undefined8 TZ_IsVPHighPriority(ushort param_1,ushort param_2,ushort param_3)

{
  undefined8 uVar1;
  char local_9;
  
  if (*(char *)((ulonglong)param_2 + *(longlong *)_refptr_g_tsaBufTzZone) ==
      *(char *)((ulonglong)param_1 + *(longlong *)_refptr_g_tsaBufTzZone)) {
    if (*(char *)((ulonglong)param_3 + *(longlong *)_refptr_g_tsaBufTzZone) ==
        *(char *)((ulonglong)param_1 + *(longlong *)_refptr_g_tsaBufTzZone)) {
      local_9 = '\0';
    }
    else {
      local_9 = '\x01';
    }
  }
  else if (*(char *)((ulonglong)param_3 + *(longlong *)_refptr_g_tsaBufTzZone) ==
           *(char *)((ulonglong)param_1 + *(longlong *)_refptr_g_tsaBufTzZone)) {
    local_9 = -1;
  }
  else if (*(char *)((ulonglong)param_2 + *(longlong *)_refptr_g_tsaBufTzZonePrev) ==
           *(char *)((ulonglong)param_1 + *(longlong *)_refptr_g_tsaBufTzZonePrev)) {
    if (*(char *)((ulonglong)param_3 + *(longlong *)_refptr_g_tsaBufTzZonePrev) ==
        *(char *)((ulonglong)param_1 + *(longlong *)_refptr_g_tsaBufTzZonePrev)) {
      local_9 = '\0';
    }
    else {
      local_9 = '\x01';
    }
  }
  else if (*(char *)((ulonglong)param_3 + *(longlong *)_refptr_g_tsaBufTzZonePrev) ==
           *(char *)((ulonglong)param_1 + *(longlong *)_refptr_g_tsaBufTzZonePrev)) {
    local_9 = -1;
  }
  else {
    local_9 = '\0';
  }
  if (local_9 == '\x01') {
    uVar1 = 1;
  }
  else if (local_9 == -1) {
    uVar1 = 0;
  }
  else if (*(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)param_2 * 2) <
           *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)param_3 * 2)) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: TZ_IsTouchMerged @ 6ba83d3e
// ============================================================================

undefined8 TZ_IsTouchMerged(undefined8 param_1)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0x43d3e  2424  TZ_IsTouchMerged */
  cVar1 = TZ_IsRptTouchMerged(param_1);
  if (cVar1 == '\0') {
    cVar1 = TZ_IsToBeRptTouchMerged(param_1);
    if (cVar1 == '\0') {
      uVar2 = 0;
    }
    else {
      uVar2 = 1;
    }
  }
  else {
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: TZ_IsLineJump @ 6ba83d7d
// ============================================================================

undefined8 TZ_IsLineJump(longlong param_1)

{
  ushort uVar1;
  ushort uVar2;
  undefined8 uVar3;
  
                    /* 0x43d7d  2413  TZ_IsLineJump */
  uVar1 = *(ushort *)(param_1 + 0x332);
  uVar2 = TS_MMToPixel(0xc);
  if (uVar1 < uVar2) {
    uVar1 = *(ushort *)(param_1 + 0x330);
    uVar2 = TS_MMToPixel(4);
    if ((uVar1 < uVar2) || (*(int *)(param_1 + 0x32c) < 0x56)) {
      uVar1 = *(ushort *)(param_1 + 0x330);
      uVar2 = TS_MMToPixel(3);
      if ((uVar1 < uVar2) ||
         ((*(int *)(param_1 + 0x32c) < 0x65 || (*(short *)(param_1 + 0x36a) < 0x1389)))) {
        if (((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)*(byte *)(param_1 + 0x324) * 0x478 + 0x214) & 0x200) ==
             0) || (*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                              (longlong)(int)(uint)*(byte *)(param_1 + 0x324) * 0x478 + 0x156) != 1)
           ) {
          uVar3 = 0;
        }
        else {
          uVar3 = 1;
        }
      }
      else {
        uVar3 = 1;
      }
    }
    else {
      uVar3 = 1;
    }
  }
  else {
    uVar3 = 0;
  }
  return uVar3;
}


// ============================================================================
// Function: TZ_IsPoorEllipseRate @ 6ba83ea4
// ============================================================================

undefined8 TZ_IsPoorEllipseRate(longlong param_1)

{
  ushort uVar1;
  ushort uVar2;
  
                    /* 0x43ea4  2417  TZ_IsPoorEllipseRate */
  if (((((3 < *(ushort *)(param_1 + 0xc)) && (0x14 < *(short *)(param_1 + 0x2aa))) &&
       (0x14 < *(short *)(param_1 + 0x2ac))) &&
      ((uVar1 = *(ushort *)(param_1 + 0x332), uVar2 = TS_MMToPixel(4), uVar1 < uVar2 &&
       ((int)*(short *)(param_1 + 0x366) <=
        (int)(uint)*(ushort *)&_refptr_g_tsaPrmtFlash[1].field_0x7c)))) &&
     ((5000 < *(short *)(param_1 + 0x36a) &&
      ((-1 < *(int *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)*(byte *)(param_1 + 0x324) * 0x478 + 0x214) &&
       (0x55 < *(int *)(param_1 + 0x32c))))))) {
    if (*(char *)(param_1 + 0x44) < '\x06') {
      return 1;
    }
    if ((int)*(char *)(param_1 + 0x3e) - (int)*(char *)(param_1 + 0x3d) < 3) {
      return 1;
    }
  }
  return 0;
}


// ============================================================================
// Function: TZ_BiCubicInter @ 6ba85a85
// ============================================================================

float TZ_BiCubicInter(int param_1,int param_2,int param_3,int param_4)

{
  int iVar1;
  int iVar2;
  longlong lVar3;
  int local_1c;
  short *local_18;
  float local_c;
  
  lVar3 = (longlong)param_3 * 0x10;
  iVar1 = param_1 + -1;
  iVar2 = param_2 + -1;
  if ((((iVar1 < 0) || ((int)(uint)_refptr_g_tsaPrmtFlash->bCols < param_1 + 3)) || (iVar2 < 0)) ||
     ((int)(uint)_refptr_g_tsaPrmtFlash->bRows < param_2 + 3)) {
    local_c = (float)TZ_BiCubicInterEdge(iVar1,iVar2,&g_afu + lVar3,
                                         &g_afu + (longlong)param_4 * 0x10);
  }
  else {
    local_c = DAT_6bb52ef8;
    local_18 = (short *)((ulonglong)((uint)_refptr_g_tsaPrmtFlash->bCols * iVar2 + iVar1 & 0xffff) *
                         2 + g_tzBuffer);
    for (local_1c = 0; local_1c < 4; local_1c = local_1c + 1) {
      local_c = local_c + (*(float *)(&DAT_6bc1398c + lVar3) * (float)(int)local_18[3] +
                          *(float *)(&g_afu + lVar3) * (float)(int)*local_18 +
                          *(float *)(&DAT_6bc13984 + lVar3) * (float)(int)local_18[1] +
                          *(float *)(&DAT_6bc13988 + lVar3) * (float)(int)local_18[2]) *
                          *(float *)(&g_afu + (longlong)param_4 * 0x10 + (longlong)local_1c * 4);
      local_18 = local_18 + _refptr_g_tsaPrmtFlash->bCols;
    }
  }
  return local_c;
}


// ============================================================================
// Function: TZ_CalcAxisMass @ 6ba86978
// ============================================================================

void TZ_CalcAxisMass(double *param_1,longlong param_2,int param_3)

{
  int iVar1;
  int iVar2;
  double local_40;
  double local_38;
  double local_30;
  double local_28;
  double local_20;
  double local_18;
  int local_c;
  
  local_18 = 0.0;
  local_20 = 0.0;
  local_28 = 0.0;
  local_30 = 0.0;
  local_38 = 0.0;
  local_40 = 0.0;
  for (local_c = 0; local_c < param_3; local_c = local_c + 1) {
    iVar1 = (int)*(short *)(param_2 + (longlong)local_c * 4);
    iVar2 = (int)*(short *)(param_2 + (longlong)local_c * 4 + 2);
    local_18 = local_18 + 1.0;
    local_20 = local_20 + (double)iVar1;
    local_28 = local_28 + (double)iVar2;
    local_30 = local_30 + (double)(iVar1 * iVar2);
    local_38 = local_38 + (double)(iVar1 * iVar1);
    local_40 = local_40 + (double)(iVar2 * iVar2);
  }
  if (local_18 == 0.0) {
    local_18 = 1.0;
  }
  *param_1 = local_18;
  param_1[1] = local_20;
  param_1[2] = local_28;
  param_1[3] = local_30;
  param_1[4] = local_38;
  param_1[5] = local_40;
  return;
}


// ============================================================================
// Function: TZ_GetEdgeDotType @ 6ba8700e
// ============================================================================

uint TZ_GetEdgeDotType(int param_1,int param_2)

{
  uint local_c;
  
  local_c = 0;
  if (param_1 < (int)((_refptr_g_tsaPrmtFlash->bCols - 1) * 4)) {
    if (param_1 < 2) {
      local_c = 1;
    }
  }
  else {
    local_c = 2;
  }
  if (param_2 < (int)((_refptr_g_tsaPrmtFlash->bRows - 1) * 4)) {
    if (param_2 < 2) {
      local_c = local_c | 4;
    }
  }
  else {
    local_c = local_c | 8;
  }
  return local_c;
}


// ============================================================================
// Function: TZ_GetAbsRatio @ 6ba823ad
// ============================================================================

int TZ_GetAbsRatio(int param_1,int param_2)

{
  uint uVar1;
  uint uVar2;
  int local_c;
  
  if (param_2 == 0) {
    local_c = 2000;
  }
  else {
    uVar2 = (param_1 << 8) / param_2;
    uVar1 = (int)uVar2 >> 0x1f;
    local_c = (uVar2 ^ uVar1) - uVar1;
    if (2000 < local_c) {
      local_c = 2000;
    }
  }
  return local_c;
}


// ============================================================================
// Function: TZ_GetRatio @ 6ba823fc
// ============================================================================

int TZ_GetRatio(int param_1,int param_2)

{
  int local_res8;
  int local_c;
  
  local_res8 = param_1;
  if (param_1 < 0) {
    local_res8 = 0;
  }
  if (param_2 == 0) {
    local_c = 2000;
  }
  else {
    local_c = (local_res8 << 8) / param_2;
    if (2000 < local_c) {
      local_c = 2000;
    }
  }
  return local_c;
}


// ============================================================================
// Function: TZ_GetCentInfo @ 6ba82aa5
// ============================================================================

void TZ_GetCentInfo(longlong param_1,byte param_2,byte param_3,char *param_4,int *param_5)

{
  int iVar1;
  
  iVar1 = TZ_GetZ9(param_2,param_3);
  if ((iVar1 != 0) &&
     (*(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) & 0xfffffffb,
     *(int *)(param_1 + 0x2e4) < iVar1)) {
    *(int *)(param_1 + 0x2e4) = iVar1;
  }
  *param_5 = *param_5 +
             (int)*(short *)((ulonglong)
                             ((uint)param_2 + (uint)param_3 * (uint)_refptr_g_tsaPrmtFlash->bCols &
                             0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif);
  *param_4 = *param_4 + '\x01';
  return;
}


// ============================================================================
// Function: TZ_Get2ClosestPeaks @ 6ba8146f
// ============================================================================

void TZ_Get2ClosestPeaks(longlong param_1,undefined1 *param_2,undefined1 *param_3)

{
  undefined1 uVar1;
  undefined1 uVar2;
  byte bVar3;
  byte local_b;
  byte local_a;
  byte local_9;
  
                    /* 0x4146f  2366  TZ_Get2ClosestPeaks */
  local_b = 0xff;
  for (local_9 = 0; (int)(uint)local_9 < (int)(*(byte *)(param_1 + 0x2b2) - 1);
      local_9 = local_9 + 1) {
    uVar1 = *(undefined1 *)(param_1 + 0x2b9 + (longlong)(int)(uint)local_9);
    local_a = local_9;
    while (local_a = local_a + 1, local_a < *(byte *)(param_1 + 0x2b2)) {
      uVar2 = *(undefined1 *)(param_1 + 0x2b9 + (longlong)(int)(uint)local_a);
      bVar3 = Peak_GetDistance(uVar2,uVar1);
      if (bVar3 < local_b) {
        *param_2 = uVar1;
        *param_3 = uVar2;
        local_b = bVar3;
      }
    }
  }
  return;
}


// ============================================================================
// Function: TZ_IsRptTouchMerged @ 6ba83b24
// ============================================================================

undefined8 TZ_IsRptTouchMerged(longlong param_1)

{
  undefined8 uVar1;
  
                    /* 0x43b24  2419  TZ_IsRptTouchMerged */
  if (((*(uint *)(param_1 + 4) & 0x200000) == 0) ||
     (((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                 (longlong)(int)(uint)*(byte *)(param_1 + 0x324) * 0x478 + 0x1f8) & 0x20000000) == 0
      && (*(short *)(param_1 + 0x33a) != 0)))) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: TZ_IsToBeRptTouchMerged @ 6ba83b90
// ============================================================================

undefined8 TZ_IsToBeRptTouchMerged(longlong param_1)

{
  byte bVar1;
  undefined8 uVar2;
  byte local_a;
  byte local_9;
  
                    /* 0x43b90  2422  TZ_IsToBeRptTouchMerged */
  if ((((*(uint *)(param_1 + 4) & 0x200000) == 0) || (*(char *)(param_1 + 0x35f) == '\0')) ||
     (*(char *)(param_1 + 0x360) == '\0')) {
    uVar2 = 0;
  }
  else {
    bVar1 = PrevTouch_GetIdxBasedOnID(*(undefined1 *)(param_1 + 0x35f));
    if (bVar1 == 0x14) {
      uVar2 = 0;
    }
    else {
      local_9 = PrevTouch_GetIdxBasedOnID(*(undefined1 *)(param_1 + 0x360));
      if (local_9 == 0x14) {
        uVar2 = 0;
      }
      else {
        if (*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)local_9 * 0x478 + 0x158) <
            *(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)bVar1 * 0x478 + 0x158)) {
          local_a = local_9;
          local_9 = bVar1;
        }
        else {
          local_a = bVar1;
        }
        if (*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)local_9 * 0x478 + 0x158) <
            (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[1].field_0x7a) {
          uVar2 = 0;
        }
        else if (*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                          (longlong)(int)(uint)local_a * 0x478 + 0x158) < 0x15) {
          if (*(ushort *)
               ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_a * 0x478 + 0x230)
              < 0x100) {
            uVar2 = 0;
          }
          else {
            uVar2 = 1;
          }
        }
        else {
          uVar2 = 0;
        }
      }
    }
  }
  return uVar2;
}


// ============================================================================
// Function: TZ_BiCubicInterEdge @ 6ba85db3
// ============================================================================

undefined4 TZ_BiCubicInterEdge(int param_1,int param_2,undefined8 param_3,undefined8 param_4)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x68]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x70];
  undefined4 uVar1;
  
  (*(int *)(_stk + 0x5c)) = 0;
  (*(int *)(_stk + 0x58)) = 4;
  (*(int *)(_stk + 0x54)) = 0;
  (*(int *)(_stk + 0x50)) = 4;
  if (param_1 < 0) {
    (*(int *)(_stk + 0x5c)) = 1;
  }
  else if ((int)(_refptr_g_tsaPrmtFlash->bCols - 4) < param_1) {
    (*(int *)(_stk + 0x58)) = 3;
  }
  if (param_2 < 0) {
    (*(int *)(_stk + 0x54)) = 1;
  }
  else if ((int)(_refptr_g_tsaPrmtFlash->bRows - 4) < param_2) {
    (*(int *)(_stk + 0x50)) = 3;
  }
  for ((*(int *)(_stk + 0x60)) = (*(int *)(_stk + 0x54)); (*(int *)(_stk + 0x60)) < (*(int *)(_stk + 0x50)); (*(int *)(_stk + 0x60)) = (*(int *)(_stk + 0x60)) + 1) {
    for ((*(int *)(_stk + 0x64)) = (*(int *)(_stk + 0x5c)); (*(int *)(_stk + 0x64)) < (*(int *)(_stk + 0x58)); (*(int *)(_stk + 0x64)) = (*(int *)(_stk + 0x64)) + 1) {
      ((int *)(_stk + 0x8))[(*(int *)(_stk + 0x60)) * 4 + (*(int *)(_stk + 0x64))] =
           (int)*(short *)((ulonglong)
                           ((uint)_refptr_g_tsaPrmtFlash->bCols * ((*(int *)(_stk + 0x60)) + param_2) +
                            (*(int *)(_stk + 0x64)) + param_1 & 0xffff) * 2 + g_tzBuffer);
    }
  }
  if ((*(int *)(_stk + 0x5c)) != 0) {
    for ((*(int *)(_stk + 0x60)) = 0; (*(int *)(_stk + 0x60)) < 4; (*(int *)(_stk + 0x60)) = (*(int *)(_stk + 0x60)) + 1) {
      ((int *)(_stk + 0x8))[(*(int *)(_stk + 0x60)) * 4] = ((int *)(_stk + 0x8))[(*(int *)(_stk + 0x60)) * 4 + 1];
    }
  }
  if ((*(int *)(_stk + 0x58)) != 4) {
    for ((*(int *)(_stk + 0x60)) = 0; (*(int *)(_stk + 0x60)) < 4; (*(int *)(_stk + 0x60)) = (*(int *)(_stk + 0x60)) + 1) {
      ((int *)(_stk + 0x8))[(*(int *)(_stk + 0x60)) * 4 + 3] = ((int *)(_stk + 0x8))[(*(int *)(_stk + 0x60)) * 4 + 2];
    }
  }
  if ((*(int *)(_stk + 0x54)) != 0) {
    for ((*(int *)(_stk + 0x64)) = 0; (*(int *)(_stk + 0x64)) < 4; (*(int *)(_stk + 0x64)) = (*(int *)(_stk + 0x64)) + 1) {
      ((int *)(_stk + 0x8))[(*(int *)(_stk + 0x64))] = ((int *)(_stk + 0x8))[(*(int *)(_stk + 0x64)) + 4];
    }
  }
  if ((*(int *)(_stk + 0x50)) != 4) {
    for ((*(int *)(_stk + 0x64)) = 0; (*(int *)(_stk + 0x64)) < 4; (*(int *)(_stk + 0x64)) = (*(int *)(_stk + 0x64)) + 1) {
      ((int *)(_stk + 0x8))[(*(int *)(_stk + 0x64)) + 0xc] = ((int *)(_stk + 0x8))[(*(int *)(_stk + 0x64)) + 8];
    }
  }
  uVar1 = TZ_BiCubicInter4x4(((int *)(_stk + 0x8)),param_3,param_4);
  return uVar1;
}


// ============================================================================
// Function: TZ_GetZ9 @ 6ba829c0
// ============================================================================

int TZ_GetZ9(byte param_1,byte param_2)

{
  int local_10;
  byte local_a;
  byte local_9;
  
                    /* 0x429c0  2402  TZ_GetZ9 */
  local_10 = 0;
  local_a = param_2 - 1;
  while( true ) {
    if ((int)(param_2 + 1) < (int)(char)local_a) {
      return local_10;
    }
    if (_refptr_g_tsaPrmtFlash->bRows <= local_a) break;
    for (local_9 = param_1 - 1; (int)(char)local_9 <= (int)(param_1 + 1); local_9 = local_9 + 1) {
      if (_refptr_g_tsaPrmtFlash->bCols <= local_9) {
        return 0;
      }
      local_10 = local_10 +
                 *(short *)(*(longlong *)_refptr_g_tsaBufDif +
                           (longlong)
                           (int)((int)(char)local_9 +
                                (uint)(ushort)((ushort)_refptr_g_tsaPrmtFlash->bCols *
                                              (short)(char)local_a)) * 2);
    }
    local_a = local_a + 1;
  }
  return 0;
}


// ============================================================================
// Function: TZ_BiCubicInter4x4 @ 6ba85c95
// ============================================================================

float TZ_BiCubicInter4x4(int *param_1,float *param_2,longlong param_3)

{
  int *local_18;
  float local_10;
  int local_c;
  
  local_10 = DAT_6bb52ef8;
  local_18 = param_1;
  for (local_c = 0; local_c < 4; local_c = local_c + 1) {
    local_10 = local_10 +
               (param_2[3] * (float)local_18[3] +
               *param_2 * (float)*local_18 + param_2[1] * (float)local_18[1] +
               param_2[2] * (float)local_18[2]) * *(float *)(param_3 + (longlong)local_c * 4);
    local_18 = local_18 + 4;
  }
  return local_10;
}


// ============================================================================
// Function: TZ_Free @ 6ba85723
// ============================================================================

void TZ_Free(void)

{
                    /* 0x45723  2365  TZ_Free */
  TSA_Free(&g_tzBuffer);
  return;
}


// ============================================================================
// Function: TZ_Malloc @ 6ba856e0
// ============================================================================

bool TZ_Malloc(void)

{
                    /* 0x456e0  2429  TZ_Malloc */
  g_tzBuffer = TSA_MallocAndInit((uint)*(ushort *)_refptr_g_gridBufSize * 2);
  return g_tzBuffer != 0;
}


