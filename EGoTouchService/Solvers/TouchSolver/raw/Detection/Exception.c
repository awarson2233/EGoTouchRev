#include "TSACoreInternal.h"

// ============================================================================
// Function: Exception_CheckUnderWater @ 6ba589c1
// ============================================================================

void Exception_CheckUnderWater(void)

{
  char cVar1;
  
                    /* 0x189c1  577  Exception_CheckUnderWater */
  if ((((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 1) == 0) &&
     (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 4) != 0)) {
    Exception_GetWaterPrpt(1);
    cVar1 = TSA_IsPossibleUnderWater();
    if (cVar1 != '\0') {
      *(undefined4 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x32 = 0x2000;
      *(undefined4 *)((longlong)&(*_refptr_g_tsaStaticPtr)[1].llField_0x58 + 2) = 0x2000;
    }
  }
  else {
    Exception_GetWaterPrpt(0);
    cVar1 = TSA_IsPossibleUnderWater();
    if (cVar1 != '\0') {
      cVar1 = TSAStatic_IsSignalReverted();
      if (cVar1 == '\0') {
        *(undefined4 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x32 = 0x2000;
        *(undefined4 *)((longlong)&(*_refptr_g_tsaStaticPtr)[1].llField_0x58 + 2) = 0x2000;
      }
      else {
        *(undefined4 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x32 = 0x4000;
        *(undefined4 *)((longlong)&(*_refptr_g_tsaStaticPtr)[1].llField_0x58 + 2) = 0x4000;
      }
    }
  }
  return;
}


// ============================================================================
// Function: Exception_Process @ 6ba58f25
// ============================================================================

void Exception_Process(void)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x28]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x30];
  undefined *puVar1;
  TSAFlashPrmt *pTVar2;
  char cVar3;
  
                    /* 0x18f25  584  Exception_Process */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&_rdata,"Exception_Process");
  }
  (*(undefined8 *)(_stk + 0x20)) = 0;
  (*(ulonglong * *)(_stk + 0x18)) = (ulonglong *)0x0;
  (*(undefined8 *)(_stk + 0x10)) = 0;
  (*(int * *)(_stk + 0x8)) = (int *)0x0;
  Exception_InitVariable(&(*(undefined8 *)(_stk + 0x20)),&(*(ulonglong * *)(_stk + 0x18)),&(*(undefined8 *)(_stk + 0x10)),&(*(int * *)(_stk + 0x8)));
  if ((*_refptr_g_prevTouchesPtr)->nTouchCount == 0x100) {
    cVar3 = AFE_IsManualFreqHopping();
    if (cVar3 != '\0') {
      cVar3 = TSAStatic_IsFreqShiftDone();
      if (cVar3 == '\0') goto LAB_6ba58fc8;
    }
    (*_refptr_g_prevTouchesPtr)->nTouchCount = 0;
  }
LAB_6ba58fc8:
  if (((*_refptr_g_prevTouchesPtr)->nTouchCount == 0x2000) && (*(*(int * *)(_stk + 0x8)) == 0x4000)) {
    (*_refptr_g_prevTouchesPtr)->nTouchCount = 0;
  }
  Exception_UpdatePrevException((*(undefined8 *)(_stk + 0x20)),(*(undefined8 *)(_stk + 0x10)),(*(int * *)(_stk + 0x8)));
  if ((*(int *)(*(longlong *)_refptr_g_touchesPtr + 0x20) == 0x40) ||
     (*(int *)(*(longlong *)_refptr_g_touchesPtr + 0x20) == 0x80)) {
    (*_refptr_g_tsaStaticPtr)->bField_0x110 = '\x01';
  }
  else {
    (*_refptr_g_tsaStaticPtr)->bField_0x110 = '\0';
  }
  if (*(int *)(*(longlong *)_refptr_g_touchesPtr + 0x20) == 0x100) {
    (*_refptr_g_tsaStaticPtr)->bField_0x111 = '\x02';
    *(*(ulonglong * *)(_stk + 0x18)) = 0;
  }
  else if (*(int *)(*(longlong *)_refptr_g_touchesPtr + 0x20) == 0x8000) {
    (*_refptr_g_tsaStaticPtr)->bField_0x111 = '\x03';
    *(*(ulonglong * *)(_stk + 0x18)) = 0;
  }
  else {
    *(*(ulonglong * *)(_stk + 0x18)) = (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal + *(*(ulonglong * *)(_stk + 0x18));
    if (0x52 < *(*(ulonglong * *)(_stk + 0x18))) {
      (*_refptr_g_tsaStaticPtr)->bField_0x111 = '\0';
      *(*(ulonglong * *)(_stk + 0x18)) = 0x53;
    }
  }
  pTVar2 = _refptr_g_tsaPrmtFlash;
  puVar1 = _refptr_g_tsaPrmtDynamic;
  if ((*(int *)(*(longlong *)_refptr_g_touchesPtr + 0x20) == 0) ||
     (*(int *)(*(longlong *)_refptr_g_touchesPtr + 0x20) == 0x400)) {
    *(double *)(_refptr_g_tsaPrmtDynamic + 0x80) = _refptr_g_tsaPrmtFlash[1].pDim1PitchMap[7];
    *(undefined4 *)(puVar1 + 0x88) = *(undefined4 *)(pTVar2[1].pDim1PitchMap + 8);
  }
  else {
    _refptr_g_tsaPrmtDynamic[0x80] = _refptr_g_tsaPrmtDynamic[0x80] & 0xfb;
  }
  *(*(int * *)(_stk + 0x8)) = 0;
  if (*(int *)(*(longlong *)_refptr_g_touchesPtr + 0x20) != (*_refptr_g_prevTouchesPtr)->nTouchCount
     ) {
    (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
         (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 0x80;
  }
  return;
}


// ============================================================================
// Function: Exception_CheckChargerNoiseInRxLines @ 6ba59214
// ============================================================================

void Exception_CheckChargerNoiseInRxLines(void)

{
                    /* 0x19214  575  Exception_CheckChargerNoiseInRxLines */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&_rdata,"Exception_CheckChargerNoiseInRxLines");
  }
  if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3) {
    FIException_CheckChargerNoiseInRxLines();
  }
  if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode != 3) {
    HIException_CheckChargerNoiseInRxLines();
  }
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"\nnoiseLevel = %d",(*_refptr_g_tsaStaticPtr)->bField_0x111);
  }
  return;
}


// ============================================================================
// Function: Exception_IsChargerNoiseDetected @ 6ba592a7
// ============================================================================

undefined8 Exception_IsChargerNoiseDetected(void)

{
  undefined8 uVar1;
  
                    /* 0x192a7  582  Exception_IsChargerNoiseDetected */
  if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3) {
    uVar1 = FIException_IsChargerNoiseDetected();
  }
  else if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3) {
    uVar1 = 0;
  }
  else {
    uVar1 = HIException_IsChargerNoiseDetected();
  }
  return uVar1;
}


// ============================================================================
// Function: Exception_CheckPanelSD @ 6ba59325
// ============================================================================

void Exception_CheckPanelSD(void)

{
                    /* 0x19325  576  Exception_CheckPanelSD */
  if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3) {
    FIException_CheckPanelSD();
  }
  if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode != 3) {
    HIException_CheckPanelSD();
  }
  return;
}


// ============================================================================
// Function: Exception_GetWaterPrpt @ 6ba5882e
// ============================================================================

void Exception_GetWaterPrpt(char param_1)

{
  byte bVar1;
  byte bVar2;
  short sVar3;
  short local_10;
  short local_e;
  ushort local_c;
  byte local_a;
  byte local_9;
  
                    /* 0x1882e  580  Exception_GetWaterPrpt */
  bVar1 = _refptr_g_tsaPrmtFlash->bCols;
  bVar2 = _refptr_g_tsaPrmtFlash->bRows;
  local_c = 0;
  local_e = 0;
  local_10 = 0;
  if (param_1 == '\0') {
    BLIIR_CalcDiffCommon
              (*(short **)_refptr_g_tsaBufDif,*(short **)_refptr_g_tsaBufRaw,
               *(short **)_refptr_g_tsaBufBl);
  }
  else {
    BLIIR_CalcDiffCommon
              (*(short **)_refptr_g_tsaBufDif,*(short **)_refptr_g_tsaBufBl,
               *(short **)_refptr_g_tsaBufBlNormal);
  }
  for (local_a = 0; local_a < bVar2; local_a = local_a + 1) {
    for (local_9 = 0; local_9 < bVar1; local_9 = local_9 + 1) {
      sVar3 = *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)local_c * 2);
      if ((*(short *)(_refptr_g_tsaPrmtDynamic + 0x22) <= sVar3) &&
         (sVar3 <= *(short *)(_refptr_g_tsaPrmtDynamic + 0x20))) {
        local_e = local_e + 1;
      }
      if (((int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x22) <= -(int)sVar3) &&
         (-(int)sVar3 <= (int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x20))) {
        local_10 = local_10 + 1;
      }
      local_c = local_c + 1;
    }
  }
  *(short *)&_refptr_g_tsaPrpt->field_0xbc = local_e;
  *(short *)&_refptr_g_tsaPrpt->field_0xbe = local_10;
  return;
}


// ============================================================================
// Function: Exception_InitVariable @ 6ba58aaa
// ============================================================================

void Exception_InitVariable(longlong *param_1,longlong *param_2,longlong *param_3,longlong *param_4)

{
  if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3) {
    *param_1 = (longlong)&(*_refptr_g_tsaStaticPtr)[1].field_0x42;
    *param_2 = (longlong)((longlong)&(*_refptr_g_tsaStaticPtr)[1].llField_0x48 + 2);
    *param_3 = (longlong)((longlong)&(*_refptr_g_tsaStaticPtr)[1].llField_0x50 + 2);
    *param_4 = (longlong)((longlong)&(*_refptr_g_tsaStaticPtr)[1].llField_0x58 + 2);
  }
  if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode != 3) {
    *param_1 = (longlong)&(*_refptr_g_tsaStaticPtr)[1].field_0x1a;
    *param_2 = (longlong)&(*_refptr_g_tsaStaticPtr)[1].field_0x22;
    *param_3 = (longlong)&(*_refptr_g_tsaStaticPtr)[1].field_0x2a;
    *param_4 = (longlong)&(*_refptr_g_tsaStaticPtr)[1].field_0x32;
  }
  return;
}


// ============================================================================
// Function: Exception_UpdatePrevException @ 6ba58c5c
// ============================================================================

void Exception_UpdatePrevException(undefined2 *param_1,int *param_2,int *param_3)

{
  char cVar1;
  
  if ((*param_3 == 0x100) || ((*_refptr_g_prevTouchesPtr)->nTouchCount == 0x100)) {
    *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x20) = 0x100;
    *param_2 = 0x100;
  }
  else if (*param_3 == 0x8000) {
    *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x20) = 0x8000;
    *param_2 = 0x8000;
  }
  else if ((*param_3 == 0x2000) || ((*_refptr_g_prevTouchesPtr)->nTouchCount == 0x2000)) {
    *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x20) = 0x2000;
    *param_2 = 0x2000;
  }
  else if (*param_3 == 0x4000) {
    *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x20) = 0x4000;
    *param_2 = 0x4000;
  }
  else {
    cVar1 = BLReset_Status();
    if ((cVar1 == '\x01') && (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3)) {
      *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x20) = 0x200;
      *param_2 = 0x200;
    }
    else if ((*_refptr_g_tsaStaticPtr)->bJustExitedSpecialCtrlMode == '\0') {
      if (((*_refptr_g_prevTouchesPtr)->nTouchCount & 0x1300U) == 0) {
        if (*param_3 == (*_refptr_g_prevTouchesPtr)->nTouchCount) {
          *(int *)(*(longlong *)_refptr_g_touchesPtr + 0x20) =
               (*_refptr_g_prevTouchesPtr)->nTouchCount;
          *param_1 = 0;
          *param_2 = *param_3;
        }
        else if ((((*_refptr_g_prevTouchesPtr)->nTouchCount == 0) ||
                 ((*_refptr_g_prevTouchesPtr)->bPrevTouchCount != '\0')) &&
                ((*_refptr_g_prevTouchesPtr)->nTouchCount != 0)) {
          if ((*param_3 == 0x80) && ((*_refptr_g_prevTouchesPtr)->nTouchCount == 0x40)) {
            *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x20) = 0x80;
            *param_1 = 0;
            *param_2 = *param_3;
          }
          else {
            *(int *)(*(longlong *)_refptr_g_touchesPtr + 0x20) =
                 (*_refptr_g_prevTouchesPtr)->nTouchCount;
            *param_1 = 0;
            *param_2 = *param_3;
          }
        }
        else {
          Exception_DebounceProcess(param_1,param_2,param_3);
          *param_2 = *param_3;
        }
      }
      else {
        *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x20) = 0;
        *param_1 = 0;
        *param_2 = *param_3;
      }
    }
    else {
      *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x20) =
           *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x20) | 0x1000;
      *param_2 = 0x1000;
    }
  }
  return;
}


// ============================================================================
// Function: Exception_DebounceProcess @ 6ba58b9e
// ============================================================================

void Exception_DebounceProcess(ushort *param_1,int *param_2,int *param_3)

{
  if (*param_2 == *param_3) {
    *param_1 = *param_1 + 1;
    if (((*param_3 == 0) && (299 < *param_1)) || ((*param_3 != 0 && (9 < *param_1)))) {
      *(int *)(*(longlong *)_refptr_g_touchesPtr + 0x20) = *param_3;
      *param_1 = 0;
    }
    else {
      *(int *)(*(longlong *)_refptr_g_touchesPtr + 0x20) = (*_refptr_g_prevTouchesPtr)->nTouchCount;
    }
  }
  else {
    *(int *)(*(longlong *)_refptr_g_touchesPtr + 0x20) = (*_refptr_g_prevTouchesPtr)->nTouchCount;
    *param_1 = 0;
  }
  return;
}


// ============================================================================
// Function: Exception_Init @ 6ba587f7
// ============================================================================

void Exception_Init(void)

{
                    /* 0x187f7  581  Exception_Init */
  if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3) {
    FIException_Init();
  }
  if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode != 3) {
    HIException_Init();
  }
  return;
}


// ============================================================================
// Function: Exception_GetDetectedChargerNoise @ 6ba592e6
// ============================================================================

undefined8 Exception_GetDetectedChargerNoise(void)

{
  undefined8 uVar1;
  
                    /* 0x192e6  579  Exception_GetDetectedChargerNoise */
  if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3) {
    uVar1 = 0;
  }
  else if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3) {
    uVar1 = 0;
  }
  else {
    uVar1 = HIException_GetDetectedChargerNoise();
  }
  return uVar1;
}


// ============================================================================
// Function: Exception_ProcessTouchZone @ 6ba591d1
// ============================================================================

void Exception_ProcessTouchZone(undefined8 param_1)

{
                    /* 0x191d1  585  Exception_ProcessTouchZone */
  if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3) {
    FIException_ProcessTouchZone(param_1);
  }
  if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode != 3) {
    HIException_ProcessTouchZone(param_1);
  }
  return;
}


// ============================================================================
// Function: Exception_Free @ 6ba587e3
// ============================================================================

void Exception_Free(void)

{
                    /* 0x187e3  578  Exception_Free */
  FIException_Free();
  return;
}


// ============================================================================
// Function: Exception_Malloc @ 6ba587c0
// ============================================================================

bool Exception_Malloc(void)

{
  char cVar1;
  
                    /* 0x187c0  583  Exception_Malloc */
  cVar1 = FIException_Malloc();
  return cVar1 != '\0';
}


