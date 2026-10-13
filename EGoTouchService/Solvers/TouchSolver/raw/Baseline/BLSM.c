#include "TSACoreInternal.h"

// ============================================================================
// Function: BLSM_PreProcess @ 6ba4c36b
// ============================================================================

void BLSM_PreProcess(void)

{
                    /* 0xc36b  379  BLSM_PreProcess */
  _refptr_g_tsaPrpt->dwShbFlags = 0;
  return;
}


// ============================================================================
// Function: BLSM_UpdateForRawUnstable @ 6ba4c27b
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

void BLSM_UpdateForRawUnstable(void)

{
  char cConditionResult;
  int nRawMinusBaseline;
  int nUpdateLimit;
  uint dwRemainingElements;
  ushort *pwBaselineCursor;
  ushort *pwPreCmfRawCursor;
  
                    /* 0xc27b  381  BLSM_UpdateForRawUnstable */
  cConditionResult = TSA_IsGameScenario();
  if ((cConditionResult == '\0') &&
     ((*(uint *)&(*_refptr_g_tsaStaticPtr)->field_0xd8 & 0x40000) != 0)) {
    pwPreCmfRawCursor = *(ushort **)_refptr_g_tsaBufPreCMFRaw;
    pwBaselineCursor = *(ushort **)_refptr_g_tsaBufBl;
    dwRemainingElements = (uint)_refptr_g_tsaPrmtFlash->wBufElementCount;
    nUpdateLimit = 0x7fffffff;
    cConditionResult = TSAStatic_IsSafeBaselineApplied();
    if (cConditionResult != '\0') {
      nUpdateLimit = (int)(short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax;
    }
    do {
      nRawMinusBaseline = (uint)*pwPreCmfRawCursor - (uint)*pwBaselineCursor;
      if (nRawMinusBaseline < nUpdateLimit) {
        if (nRawMinusBaseline < 0x11) {
          if (nRawMinusBaseline < -0x10) {
            *pwBaselineCursor = *pwBaselineCursor - 0x11;
          }
        }
        else {
          *pwBaselineCursor = *pwBaselineCursor + 0x11;
        }
      }
      pwPreCmfRawCursor = pwPreCmfRawCursor + 1;
      pwBaselineCursor = pwBaselineCursor + 1;
      dwRemainingElements = dwRemainingElements - 1;
    } while (dwRemainingElements != 0);
  }
  return;
}


// ============================================================================
// Function: BLSM_Process @ 6ba4c645
// ============================================================================

/* Top-level baseline state-machine tick: recalibration, property selection, stage update, stage
   execution, and SHB handling. */

void BLSM_Process(bool bHardwareReset,bool bForceBaselineProperty)

{
  bool bBaselinePropertyRefresh;
  bool bBaselineResetProperty;
  
                    /* Export ordinal 380; public BLSM entry from TSA_MSProcess. */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,"\n%s\n","BLSM_Process");
  }
  BLRecal_Process();
                    /* Force parameter or static latch requests baseline property refresh. */
  if ((bForceBaselineProperty) || ((*_refptr_g_tsaStaticPtr)[1].field_0x2 != '\0')) {
    bBaselinePropertyRefresh = true;
  }
  else {
    bBaselinePropertyRefresh = false;
  }
                    /* Hardware reset parameter or static latch requests baseline reset property. */
  if ((bHardwareReset) || ((*_refptr_g_tsaStaticPtr)[1].field_0x1 != '\0')) {
    bBaselineResetProperty = true;
  }
  else {
    bBaselineResetProperty = false;
  }
  BLSM_GetProperty(bBaselineResetProperty,bBaselinePropertyRefresh);
  BLSM_UpdateStage();
  BLSM_ProcessStage();
  BLSM_ShbProcess();
  return;
}


// ============================================================================
// Function: BLSM_IsReset @ 6ba4d794
// ============================================================================

undefined4 BLSM_IsReset(void)

{
                    /* 0xd794  377  BLSM_IsReset */
  return CONCAT31((int3)(_refptr_g_tsaPrpt->dwBaselineStage >> 8),
                  _refptr_g_tsaPrpt->dwBaselineStage == 1);
}


// ============================================================================
// Function: BLSM_ShbProcess @ 6ba4c4cc
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* Runs the SHB capture state machine, resetting on baseline/raw changes and scheduling capture
   flags by elapsed frame time. */

void BLSM_ShbProcess(void)

{
  char cVar1;
  int iVar2;
  
  cVar1 = BLSM_IsReset();
  if (cVar1 != '\0') {
    _refptr_g_tsaPrpt->dwShbState = 0;
  }
  iVar2 = BLSM_ShbIsRawChanges();
  if (iVar2 != 0) {
    _refptr_g_tsaPrpt->dwShbState = 0;
  }
  if (_refptr_g_tsaPrpt->dwShbState == 1) {
    if ((void *)(_refptr_g_tsaPrpt->qwShbStateStartTime + 5000) <=
        (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr) {
      _refptr_g_tsaPrpt->dwShbState = 2;
      _refptr_g_tsaPrpt->dwShbFlags = _refptr_g_tsaPrpt->dwShbFlags | 1;
    }
  }
  else if (_refptr_g_tsaPrpt->dwShbState == 2) {
    if ((void *)(_refptr_g_tsaPrpt->qwShbStateStartTime + 5000) <=
        (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr) {
      _refptr_g_tsaPrpt->dwShbFlags = _refptr_g_tsaPrpt->dwShbFlags | 1;
      _refptr_g_tsaPrpt->dwShbFlags = _refptr_g_tsaPrpt->dwShbFlags | 2;
    }
  }
  else {
    _refptr_g_tsaPrpt->dwShbState = 1;
    _refptr_g_tsaPrpt->dwShbFlags = _refptr_g_tsaPrpt->dwShbFlags | 1;
  }
  if ((_refptr_g_tsaPrpt->dwShbFlags & 1) != 0) {
    BLSM_ShbCaptureBl();
  }
  return;
}


// ============================================================================
// Function: BLSM_UpdateStage @ 6ba4c6f1
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* Advances baseline stage and updates per-stage timing/reference state from the current baseline
   property flags. */

void BLSM_UpdateStage(void)

{
  short local_e;
  uint local_c;
  
  local_e = (short)DAT_6bcab6c6;
  g_tsaPrpt.dwPrevBaselineStage = g_tsaPrpt.dwBaselineStage;
  if ((g_tsaPrpt.dwBaselinePropertyFlags & 0x10) == 0) {
    if ((g_tsaPrpt.dwBaselinePropertyFlags & 0x200) == 0) {
      if ((g_tsaPrpt.dwBaselinePropertyFlags & 0x100) == 0) {
        if ((g_tsaPrpt.dwBaselinePropertyFlags & 0x80) == 0) {
          if ((g_tsaPrpt.dwBaselinePropertyFlags & 0x40) == 0) {
            local_c = g_tsaPrpt.dwBaselineStage;
            g_tsaPrpt.nBaselineStageFrameCount = g_tsaPrpt.nBaselineStageFrameCount + 1;
            g_tsaPrpt.qwBaselineStageReference =
                 (longlong)g_tsaStaticPtr->pFrameBufferPtr +
                 (g_tsaPrpt.qwBaselineStageReference - g_tsaStaticPtr->llField_0xe8);
            switch(g_tsaPrpt.dwBaselineStage) {
            case 0:
              if ((g_tsaPrpt.dwBaselinePropertyFlags & 2) == 0) {
                if ((g_tsaPrpt.dwBaselinePropertyFlags & 1) != 0) {
                  g_tsaPrpt.nBaselineStageFrameCount = 1;
                  g_tsaPrpt.qwBaselineStageReference =
                       (ulonglong)g_tsaStaticPtr->nBaselineStageInitialSignal;
                  local_c = 4;
                }
              }
              else {
                g_tsaPrpt.nBaselineStageFrameCount = 1;
                g_tsaPrpt.qwBaselineStageReference =
                     (ulonglong)g_tsaStaticPtr->nBaselineStageInitialSignal;
                local_c = 5;
              }
              break;
            case 1:
            case 2:
              g_tsaPrpt.nBaselineStageFrameCount = 1;
              g_tsaPrpt.qwBaselineStageReference =
                   (ulonglong)g_tsaStaticPtr->nBaselineStageInitialSignal;
              local_c = 0;
              break;
            case 3:
              g_tsaPrpt.nBaselineStageFrameCount = 1;
              g_tsaPrpt.qwBaselineStageReference =
                   (ulonglong)g_tsaStaticPtr->nBaselineStageInitialSignal;
              local_c = 0;
              break;
            case 4:
              if ((g_tsaPrpt.dwBaselinePropertyFlags & 2) == 0) {
                if ((ushort)(DAT_6bcab6d2 >> 16) <= g_tsaPrpt.qwBaselineStageReference) {
                  g_tsaPrpt.nBaselineStageFrameCount = 1;
                  g_tsaPrpt.qwBaselineStageReference =
                       (ulonglong)g_tsaStaticPtr->nBaselineStageInitialSignal;
                  local_c = 0;
                }
              }
              else {
                g_tsaPrpt.nBaselineStageFrameCount = 1;
                g_tsaPrpt.qwBaselineStageReference =
                     (ulonglong)g_tsaStaticPtr->nBaselineStageInitialSignal;
                local_c = 5;
              }
              break;
            case 5:
              /* bSingleTouchBaselineLatch @ TSAStatic+0x3ec */
              if ((g_tsaPrpt.dwBaselinePropertyFlags & 2) == 0) {
                if (*((uint8_t *)g_tsaStaticPtr + 0x3ec) == '\0') {
                  g_tsaPrpt.nBaselineStageFrameCount = 1;
                  g_tsaPrpt.qwBaselineStageReference =
                       (ulonglong)g_tsaStaticPtr->nBaselineStageInitialSignal;
                  local_c = 6;
                }
                else {
                  g_tsaPrpt.nBaselineStageFrameCount = 1;
                  g_tsaPrpt.qwBaselineStageReference =
                       (ulonglong)g_tsaStaticPtr->nBaselineStageInitialSignal;
                  local_c = 4;
                }
              }
              if ((g_pPrevTouches->bPrevTouchCount == '\x01') &&
                 ((*(uint *)(g_pPrevTouches + 0xe) & 0x200) == 0)) {
                *((uint8_t *)g_tsaStaticPtr + 0x3ec) = '\x01';
              }
              else {
                *((uint8_t *)g_tsaStaticPtr + 0x3ec) = '\0';
              }
              break;
            case 6:
              if ((g_tsaPrpt.dwBaselinePropertyFlags & 2) == 0) {
                if ((ushort)(DAT_6bcab6d2 >> 16) <= g_tsaPrpt.qwBaselineStageReference) {
                  g_tsaPrpt.nBaselineStageFrameCount = 1;
                  g_tsaPrpt.qwBaselineStageReference =
                       (ulonglong)g_tsaStaticPtr->nBaselineStageInitialSignal;
                  local_c = 7;
                }
              }
              else {
                g_tsaPrpt.nBaselineStageFrameCount = 1;
                g_tsaPrpt.qwBaselineStageReference =
                     (ulonglong)g_tsaStaticPtr->nBaselineStageInitialSignal;
                local_c = 5;
              }
              break;
            case 7:
              if (g_BLSMNextEdgeSmallThresh != '\0') {
                local_e = _DAT_6bcab6c8;
              }
              if ((g_tsaPrpt.dwBaselinePropertyFlags & 2) == 0) {
                if (g_tsaPrpt.qwBaselineStageReference < (ushort)(DAT_6bcab6d2 >> 16)) {
                  if ((g_tsaPrpt.nSPeakPositive < local_e) &&
                     (-(int)g_tsaPrpt.nSPeakNegative < (int)local_e)) {
                    g_tsaPrpt.nBaselineStageFrameCount = 1;
                    g_tsaPrpt.qwBaselineStageReference =
                         (ulonglong)g_tsaStaticPtr->nBaselineStageInitialSignal;
                    local_c = 0;
                  }
                }
                else {
                  g_tsaPrpt.nBaselineStageFrameCount = 1;
                  g_tsaPrpt.qwBaselineStageReference =
                       (ulonglong)g_tsaStaticPtr->nBaselineStageInitialSignal;
                  local_c = 0;
                }
              }
              else {
                g_tsaPrpt.nBaselineStageFrameCount = 1;
                g_tsaPrpt.qwBaselineStageReference =
                     (ulonglong)g_tsaStaticPtr->nBaselineStageInitialSignal;
                local_c = 5;
              }
              break;
            case 8:
            case 9:
              if ((g_tsaPrpt.dwBaselinePropertyFlags & 2) == 0) {
                if ((ushort)(DAT_6bcab6d2 >> 16) <= g_tsaPrpt.qwBaselineStageReference) {
                  g_tsaPrpt.nBaselineStageFrameCount = 1;
                  g_tsaPrpt.qwBaselineStageReference =
                       (ulonglong)g_tsaStaticPtr->nBaselineStageInitialSignal;
                  local_c = 0;
                }
              }
              else {
                g_tsaPrpt.nBaselineStageFrameCount = 1;
                g_tsaPrpt.qwBaselineStageReference =
                     (ulonglong)g_tsaStaticPtr->nBaselineStageInitialSignal;
                local_c = 5;
              }
              break;
            default:
              g_tsaPrpt.nBaselineStageFrameCount = 1;
              g_tsaPrpt.qwBaselineStageReference =
                   (ulonglong)g_tsaStaticPtr->nBaselineStageInitialSignal;
            }
            g_tsaPrpt.dwBaselineStage = local_c;
          }
          else {
            g_tsaPrpt.dwBaselineStage = 3;
          }
        }
        else {
          g_tsaPrpt.dwBaselineStage = 9;
        }
      }
      else {
        g_tsaPrpt.dwBaselineStage = 3;
      }
    }
    else {
      g_tsaPrpt.dwBaselineStage = 2;
    }
  }
  else {
    g_tsaPrpt.dwBaselineStage = 1;
  }
  return;
}


// ============================================================================
// Function: BLSM_ProcessStage @ 6ba4d170
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* Dispatches the current baseline stage handler, then either recalculates or resets the IIR diff
   state. */

void BLSM_ProcessStage(void)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0xc]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x10];
  bool bVar1;
  short sVar2;
  int iVar3;
  
  (*(short *)(_stk + 0x6)) = DAT_6bcab6c6;
  (*_refptr_g_tsaStaticPtr)->bBaselineUpdatedThisFrame = '\0';
  (*_refptr_g_tsaStaticPtr)->bResetDiffRequested = '\0';
  (*_refptr_g_tsaStaticPtr)[1].bField_0x00 = '\0';
  switch(_refptr_g_tsaPrpt->dwBaselineStage) {
  case 0:
    BLSM_GetNoTouchBlStep(&(*(short *)(_stk + 0x6)),(ushort *)&(*(short *)(_stk + 0x4)));
    BLIIR_Update(0,(*(short *)(_stk + 0x6)),(*(short *)(_stk + 0x4)));
    (*_refptr_g_tsaStaticPtr)->bBaselineUpdatedThisFrame = '\x01';
    BLSM_SaveNormalBl();
    break;
  case 1:
    (*_refptr_g_tsaStaticPtr)->bResetDiffRequested = '\x01';
    BLSM_Reset();
    break;
  case 2:
    (*_refptr_g_tsaStaticPtr)->bResetDiffRequested = '\x01';
    BLSM_Reset();
    break;
  case 3:
    if (((_refptr_g_tsaPrpt->dwPrevBaselineStage == 1) ||
         (_refptr_g_tsaPrpt->dwPrevBaselineStage == 2)) ||
        (_refptr_g_tsaPrpt->dwPrevBaselineStage == 3)) {
      _refptr_g_tsaPrpt->dwBaselineStage = 0;
    }
    else {
      _refptr_g_tsaPrpt->dwBaselineStage = _refptr_g_tsaPrpt->dwPrevBaselineStage;
    }
    break;
  case 7:
  case 8:
    if ((g_BLSMNextEdgeSmallThresh != '\0') || (bVar1 = BLSM_ProcessUseNextSideThold(), bVar1)) {
      BLIIR_Update(0,(short)(DAT_6bcab6ca >> 16),(undefined1)DAT_6bcab6d2);
    }
    else {
      BLIIR_Update(0,(short)DAT_6bcab6ca,(undefined1)DAT_6bcab6d2);
    }
    (*_refptr_g_tsaStaticPtr)->bBaselineUpdatedThisFrame = '\x01';
    break;
  case 9:
    iVar3 = -(int)_refptr_g_tsaPrpt->nSPeakNegative;
    if (iVar3 < 0) {
      iVar3 = iVar3 + 0xf;
    }
    (*(short *)(_stk + 0x4)) = (short)(iVar3 >> 4);
    sVar2 = DAT_6bcab6c6;
    if ((short)DAT_6bcab6c6 < 0) {
      sVar2 = DAT_6bcab6c6 + 3;
    }
    if (sVar2 >> 2 < (*(short *)(_stk + 0x4))) {
      (*(short *)(_stk + 0x4)) = DAT_6bcab6c6;
      if ((short)DAT_6bcab6c6 < 0) {
        (*(short *)(_stk + 0x4)) = DAT_6bcab6c6 + 3;
      }
      (*(short *)(_stk + 0x4)) = (*(short *)(_stk + 0x4)) >> 2;
    }
    BLIIR_Update(1,(short)-_refptr_g_tsaPrpt->nSPeakNegative,(*(short *)(_stk + 0x4)));
    (*_refptr_g_tsaStaticPtr)->bBaselineUpdatedThisFrame = '\x01';
  }
  if ((*_refptr_g_tsaStaticPtr)->bResetDiffRequested == '\0') {
    BLIIR_CalcDiff();
  }
  else {
    BLIIR_ResetDiff();
  }
  return;
}

// ============================================================================
// Function: BLSM_GetProperty @ 6ba4d523
// ============================================================================

/* Computes baseline property flags/state from reset conditions, noise/debounce checks, edge
   thresholds, and touch activity. */

void BLSM_GetProperty(bool bResetPropertyRequested,bool bRefreshPropertyRequested)

{
  char cVar1;
  short sVar2;
  uint dwBaselinePropertyFlags;
  
  sVar2 = BLSM_GetMaxSig();
  dwBaselinePropertyFlags = 0;
  _refptr_g_tsaPrpt->dwPrevBaselinePropertyState = _refptr_g_tsaPrpt->dwBaselinePropertyState;
  _refptr_g_tsaPrpt->dwBaselinePropertyState = 0;
  g_BLSMNextEdgeSmallThresh = '\0';
  if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 7) != 0) {
    (*_refptr_g_tsaStaticPtr)->qwBaselineResetReferenceTime =
         (ulonglong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  }
  if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x800000) != 0) {
    (*_refptr_g_tsaStaticPtr)->qwBaselineResetReferenceTime =
         (ulonglong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  }
  if (bResetPropertyRequested) {
    (*_refptr_g_tsaStaticPtr)->bBaselineResetPropertyLatched = '\x01';
    dwBaselinePropertyFlags = 0x10;
    if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 5) == 0) {
      if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 8) == 0) {
        _refptr_g_tsaPrpt->dwBaselinePropertyState = 3;
      }
      else {
        _refptr_g_tsaPrpt->dwBaselinePropertyState = 2;
      }
    }
    else {
      _refptr_g_tsaPrpt->dwBaselinePropertyState = 1;
    }
  }
  else if (bRefreshPropertyRequested) {
    dwBaselinePropertyFlags = 0x40;
  }
  else {
    cVar1 = BLReset_Process(bResetPropertyRequested);
    if (cVar1 == '\0') {
      cVar1 = BLReset_IsBaselineNoisy();
      if (cVar1 == '\0') {
        cVar1 = BLReset_InDebounce();
        if (cVar1 == '\0') {
          cVar1 = TouchThold_ToeUseSpecialThold();
          if ((cVar1 != '\0') &&
             ((_refptr_g_tsaPrpt->bSpecialThreshCol == '\x01' ||
              ((uint)_refptr_g_tsaPrpt->bSpecialThreshCol == _refptr_g_tsaPrmtFlash->bCols - 2)))) {
            g_BLSMNextEdgeSmallThresh = '\x01';
          }
          if (sVar2 < *(short *)(_refptr_g_tsaPrmtDynamic + 0x42)) {
            cVar1 = AFE_IsCoreTouchDetected();
            if ((cVar1 == '\0') &&
               ((g_BLSMNextEdgeSmallThresh == '\0' ||
                (sVar2 < *(short *)(_refptr_g_tsaPrmtDynamic + 0x44))))) {
              if ((*(short *)(_refptr_g_tsaPrmtDynamic + 0x46) <= sVar2) ||
                 ((g_BLSMNextEdgeSmallThresh != '\0' &&
                  (*(short *)(_refptr_g_tsaPrmtDynamic + 0x48) <= sVar2)))) {
                dwBaselinePropertyFlags = 1;
              }
              goto LAB_6ba4d77e;
            }
          }
          dwBaselinePropertyFlags = 2;
        }
        else {
          dwBaselinePropertyFlags = 0x100;
        }
      }
      else {
        dwBaselinePropertyFlags = 0x200;
      }
    }
    else {
      dwBaselinePropertyFlags = 0x10;
    }
  }
LAB_6ba4d77e:
  _refptr_g_tsaPrpt->dwBaselinePropertyFlags = dwBaselinePropertyFlags;
  return;
}


// ============================================================================
// Function: BLSM_ShbIsRawChanges @ 6ba4c382
// ============================================================================

undefined8 BLSM_ShbIsRawChanges(void)

{
  uint uVar1;
  uint local_1c;
  short *local_18;
  short *local_10;
  
  local_1c = (uint)_refptr_g_tsaPrmtFlash->wBufElementCount;
  local_18 = *(short **)_refptr_g_tsaBufRawCapture;
  local_10 = *(short **)_refptr_g_tsaBufRaw;
  do {
    uVar1 = (int)(short)(*local_18 - *local_10) >> 0x1f;
    if ((int)((short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax / 2) <
        (int)(((int)(short)(*local_18 - *local_10) ^ uVar1) - uVar1)) {
      return 1;
    }
    local_1c = local_1c - 1;
    local_18 = local_18 + 1;
    local_10 = local_10 + 1;
  } while (local_1c != 0);
  return 0;
}


// ============================================================================
// Function: BLSM_ShbCaptureBl @ 6ba4c422
// ============================================================================

void BLSM_ShbCaptureBl(void)

{
  errno_t eVar1;
  
  eVar1 = memcpy_s(*(void **)_refptr_g_tsaBufRawCapture,
                   (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,
                   *(void **)_refptr_g_tsaBufPreCMFRaw,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/Baseline/BaselineStateMachine.c",0x7a,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  _refptr_g_tsaPrpt->qwShbStateStartTime = (ulonglong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  return;
}


// ============================================================================
// Function: BLSM_Reset @ 6ba4cf23
// ============================================================================

void BLSM_Reset(void)

{
  BLIIR_Reset();
  Self_Reset();
  SS_Reset();
  return;
}


// ============================================================================
// Function: BLSM_ProcessUseNextSideThold @ 6ba4cf41
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* Returns whether the next-side special threshold path should be used for the current edge column.
    */

bool BLSM_ProcessUseNextSideThold(void)

{
  char cVar1;
  bool bVar2;
  
  cVar1 = TouchThold_ToeUseSpecialThold();
  if ((cVar1 == '\0') ||
     ((_refptr_g_tsaPrpt->bNextSideThreshCol != '\x01' &&
      ((uint)_refptr_g_tsaPrpt->bNextSideThreshCol != _refptr_g_tsaPrmtFlash->bCols - 2)))) {
    bVar2 = false;
  }
  else {
    bVar2 = true;
  }
  return bVar2;
}


// ============================================================================
// Function: BLSM_GetNoTouchBlStep @ 6ba4cf9c
// ============================================================================

void BLSM_GetNoTouchBlStep(short *param_1,ushort *param_2)

{
  bool bVar1;
  ushort uVar2;
  short sVar3;
  int iVar4;
  byte local_9;
  
  local_9 = _refptr_g_tsaPrmtDynamic[0x4f];
  if (g_BLSMNextEdgeSmallThresh == '\0') {
    bVar1 = BLSM_ProcessUseNextSideThold();
    if (bVar1) {
      *param_1 = *(short *)(_refptr_g_tsaPrmtDynamic + 0x48);
      local_9 = _refptr_g_tsaPrmtDynamic[0x50];
    }
  }
  else {
    *param_1 = *(short *)(_refptr_g_tsaPrmtDynamic + 0x48);
    local_9 = _refptr_g_tsaPrmtDynamic[0x50];
  }
  if ((*_refptr_g_tsaStaticPtr)->field_0x3 == '\0') {
    *param_2 = (ushort)local_9;
  }
  else {
    iVar4 = (int)_refptr_g_tsaPrpt->nSPeakPositive;
    if ((int)_refptr_g_tsaPrpt->nSPeakPositive < -(int)_refptr_g_tsaPrpt->nSPeakNegative) {
      iVar4 = -(int)_refptr_g_tsaPrpt->nSPeakNegative;
    }
    *param_2 = (ushort)iVar4;
    uVar2 = *param_2;
    if ((short)uVar2 < 0) {
      uVar2 = uVar2 + 0xf;
    }
    *param_2 = (short)uVar2 >> 4;
    sVar3 = *param_1;
    if (sVar3 < 0) {
      sVar3 = sVar3 + 3;
    }
    if (sVar3 >> 2 < (short)*param_2) {
      sVar3 = *param_1;
      if (sVar3 < 0) {
        sVar3 = sVar3 + 3;
      }
      *param_2 = sVar3 >> 2;
    }
  }
  return;
}


// ============================================================================
// Function: BLSM_SaveNormalBl @ 6ba4d0c3
// ============================================================================

void BLSM_SaveNormalBl(void)

{
  errno_t eVar1;
  
  if (((*_refptr_g_tsaStaticPtr)[1].field_0x3 == '\0') &&
     ((*_refptr_g_prevTouchesPtr)->nTouchCount != 0x2000)) {
    eVar1 = memcpy_s(*(void **)_refptr_g_tsaBufBlNormal,0x2648,*(void **)_refptr_g_tsaBufBl,
                     (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
    if (eVar1 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Alg/Baseline/BaselineStateMachine.c",0x1ad,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
  }
  return;
}


// ============================================================================
// Function: BLSM_GetMaxSig @ 6ba4d41d
// ============================================================================

short BLSM_GetMaxSig(void)

{
  char cVar1;
  short local_a;
  
                    /* 0xd41d  372  BLSM_GetMaxSig */
  local_a = _refptr_g_tsaPrpt->nSPeakPositive;
  cVar1 = TSAStatic_IsSafeBaselineApplied();
  if (cVar1 == '\0') {
    cVar1 = TouchThold_ToeUseSpecialThold();
    if ((((cVar1 != '\0') && (cVar1 = TSA_IsGameScenario(), cVar1 == '\0')) &&
        ((*_refptr_g_tsaStaticPtr)->field_0x3dc != '\0')) && (local_a < 0x514)) {
      local_a = *(short *)&_refptr_g_tsaPrpt->field_0xc0;
    }
    local_a = ToeSyna_PorpertyGetMaxSig((int)local_a);
  }
  return local_a;
}


// ============================================================================
// Function: BLSM_Init @ 6ba4c220
// ============================================================================

void BLSM_Init(undefined1 param_1)

{
                    /* 0xc220  374  BLSM_Init */
  BLIIR_Init();
  BLReset_Init(param_1);
  BLRecal_Init();
  _refptr_g_tsaPrpt->dwBaselineStage = 0;
  _refptr_g_tsaPrpt->nBaselineStageFrameCount = 1;
  _refptr_g_tsaPrpt->qwBaselineStageReference = 0;
  return;
}


// ============================================================================
// Function: BLSM_GetMinSig @ 6ba4d4a0
// ============================================================================

short BLSM_GetMinSig(void)

{
  char cVar1;
  short local_a;
  
                    /* 0xd4a0  373  BLSM_GetMinSig */
  local_a = _refptr_g_tsaPrpt->nSPeakNegative;
  cVar1 = TSAStatic_IsSafeBaselineApplied();
  if (cVar1 == '\0') {
    cVar1 = TouchThold_ToeUseSpecialThold();
    if ((((cVar1 != '\0') && (cVar1 = TSA_IsGameScenario(), cVar1 == '\0')) &&
        ((*_refptr_g_tsaStaticPtr)->field_0x3dc != '\0')) && (-0x514 < local_a)) {
      local_a = *(short *)&_refptr_g_tsaPrpt->field_0xc2;
    }
    local_a = ToeSyna_PorpertyGetMinSig((int)local_a);
  }
  return local_a;
}


// ============================================================================
// Function: BLSM_IsOKToIDLE @ 6ba4d7fd
// ============================================================================

undefined4 BLSM_IsOKToIDLE(void)

{
  undefined4 uVar1;
  
                    /* 0xd7fd  376  BLSM_IsOKToIDLE */
  if (_refptr_g_tsaPrmtFlash->field_0x44 == '\0') {
    uVar1 = CONCAT31((int3)(_refptr_g_tsaPrpt->dwBaselineStage >> 8),
                     _refptr_g_tsaPrpt->dwBaselineStage == 0);
  }
  else if ((((_refptr_g_tsaPrpt->dwBaselineStage == 0) || (_refptr_g_tsaPrpt->dwBaselineStage == 6))
           || (_refptr_g_tsaPrpt->dwBaselineStage == 7)) ||
          (_refptr_g_tsaPrpt->dwBaselineStage == 4)) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: BLSM_IsBaselineOK @ 6ba4d7c6
// ============================================================================

undefined8 BLSM_IsBaselineOK(void)

{
  undefined8 uVar1;
  
                    /* 0xd7c6  375  BLSM_IsBaselineOK */
  if ((_refptr_g_tsaPrpt->dwBaselinePropertyFlags == 0) ||
     (_refptr_g_tsaPrpt->dwBaselinePropertyFlags == 0x100)) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: BLSM_IsTouched @ 6ba4d7ad
// ============================================================================

undefined4 BLSM_IsTouched(void)

{
                    /* 0xd7ad  378  BLSM_IsTouched */
  return CONCAT31((int3)(_refptr_g_tsaPrpt->dwBaselineStage >> 8),
                  _refptr_g_tsaPrpt->dwBaselineStage == 5);
}


