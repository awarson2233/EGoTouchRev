#include "TSACoreInternal.h"

// ============================================================================
// Function: BLIIR_CalcDiffCommon @ 6ba46f4c
// ============================================================================

void BLIIR_CalcDiffCommon(short *pDifOut,short *pRawGrid,short *pBaselineGrid)

{
  short *pDifCursor;
  short *pRawCursor;
  short *pBaselineCursor;
  uint dwRemainingReverted;
  uint dwRemainingNormal;
  
                    /* 0x6f4c  346  BLIIR_CalcDiffCommon */
  if ((*_refptr_g_tsaStaticPtr)->bField_0x11a == '\0') {
    dwRemainingNormal = (uint)_refptr_g_tsaPrmtFlash->wBufElementCount;
    pDifCursor = pDifOut;
    pRawCursor = pRawGrid;
    pBaselineCursor = pBaselineGrid;
    do {
      *pDifCursor = *pBaselineCursor - *pRawCursor;
      dwRemainingNormal = dwRemainingNormal - 1;
      pDifCursor = pDifCursor + 1;
      pRawCursor = pRawCursor + 1;
      pBaselineCursor = pBaselineCursor + 1;
    } while (dwRemainingNormal != 0);
  }
  else {
    dwRemainingReverted = (uint)_refptr_g_tsaPrmtFlash->wBufElementCount;
    pDifCursor = pDifOut;
    pRawCursor = pRawGrid;
    pBaselineCursor = pBaselineGrid;
    do {
      *pDifCursor = *pRawCursor - *pBaselineCursor;
      dwRemainingReverted = dwRemainingReverted - 1;
      pDifCursor = pDifCursor + 1;
      pRawCursor = pRawCursor + 1;
      pBaselineCursor = pBaselineCursor + 1;
    } while (dwRemainingReverted != 0);
  }
  return;
}


// ============================================================================
// Function: BLIIR_CalcRawCommon @ 6ba4701b
// ============================================================================

void BLIIR_CalcRawCommon(short *param_1,short *param_2,short *param_3)

{
  short *local_res8;
  short *local_res10;
  short *local_res18;
  uint local_10;
  uint local_c;
  
                    /* 0x701b  347  BLIIR_CalcRawCommon */
  if ((*_refptr_g_tsaStaticPtr)->bField_0x11a == '\0') {
    local_c = (uint)_refptr_g_tsaPrmtFlash->wBufElementCount;
    local_res8 = param_1;
    local_res10 = param_2;
    local_res18 = param_3;
    do {
      *local_res8 = *local_res10 - *local_res18;
      local_c = local_c - 1;
      local_res8 = local_res8 + 1;
      local_res10 = local_res10 + 1;
      local_res18 = local_res18 + 1;
    } while (local_c != 0);
  }
  else {
    local_10 = (uint)_refptr_g_tsaPrmtFlash->wBufElementCount;
    local_res8 = param_1;
    local_res10 = param_2;
    local_res18 = param_3;
    do {
      *local_res8 = *local_res18 + *local_res10;
      local_10 = local_10 - 1;
      local_res8 = local_res8 + 1;
      local_res10 = local_res10 + 1;
      local_res18 = local_res18 + 1;
    } while (local_10 != 0);
  }
  return;
}


// ============================================================================
// Function: BLIIR_ResetDiff @ 6ba46e76
// ============================================================================

void BLIIR_ResetDiff(void)

{
  int iVar1;
  
                    /* 0x6e76  351  BLIIR_ResetDiff */
  iVar1 = memset_s(*(undefined8 *)_refptr_g_tsaBufDif,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2,0,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/Baseline/BaselineIIR.c",0x19,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: BLIIR_CalcDiff @ 6ba46efc
// ============================================================================

void BLIIR_CalcDiff(void)

{
                    /* 0x6efc  345  BLIIR_CalcDiff */
  BLIIR_CalcDiffCommon
            (*(short **)_refptr_g_tsaBufDif,*(short **)_refptr_g_tsaBufRaw,
             *(short **)_refptr_g_tsaBufBl);
  return;
}


// ============================================================================
// Function: BLIIR_Update @ 6ba470e7
// ============================================================================

void BLIIR_Update(char param_1,short param_2,short param_3)

{
  short sVar1;
  short sVar2;
  
                    /* 0x70e7  352  BLIIR_Update */
  sVar1 = BLSM_GetMaxSig();
  sVar2 = BLSM_GetMinSig();
  if (((param_1 != '\0') || (-(int)param_2 <= (int)sVar2)) && (sVar1 <= param_2)) {
    BLIIR_DoUpdate((int)param_3);
  }
  return;
}


// ============================================================================
// Function: BLIIR_DoUpdate @ 6ba47141
// ============================================================================

void BLIIR_DoUpdate(short param_1)

{
  uint local_1c;
  ushort *local_18;
  ushort *local_10;
  
                    /* 0x7141  348  BLIIR_DoUpdate */
  if (_refptr_g_tsaPrmtFlash->field_0x69 == '\0') {
    local_10 = *(ushort **)_refptr_g_tsaBufRaw;
  }
  else {
    local_10 = *(ushort **)_refptr_g_tsaBufPreCMFRaw;
  }
  local_18 = *(ushort **)_refptr_g_tsaBufBl;
  local_1c = (uint)_refptr_g_tsaPrmtFlash->wBufElementCount;
  do {
    if ((int)((uint)*local_10 - (uint)*local_18) < (int)param_1) {
      if ((int)((uint)*local_10 - (uint)*local_18) <= -(int)param_1) {
        *local_18 = *local_18 - param_1;
      }
    }
    else {
      *local_18 = *local_18 + param_1;
    }
    local_10 = local_10 + 1;
    local_18 = local_18 + 1;
    local_1c = local_1c - 1;
  } while (local_1c != 0);
  (*_refptr_g_tsaStaticPtr)[1].bField_0x00 = '\x01';
  return;
}


// ============================================================================
// Function: BLIIR_Reset @ 6ba4721f
// ============================================================================

void BLIIR_Reset(void)

{
  char cVar1;
  errno_t eVar2;
  ushort local_a;
  
                    /* 0x721f  350  BLIIR_Reset */
  cVar1 = AFE_IsRawUnifiedToTargetValue();
  if (cVar1 == '\0') {
    eVar2 = memcpy_s(*(void **)_refptr_g_tsaBufBl,0x2648,*(void **)_refptr_g_tsaBufPreCMFRaw,
                     (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
    if (eVar2 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Alg/Baseline/BaselineIIR.c",0x89,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
  }
  else if ((_refptr_g_tsaPrpt->dwBaselinePropertyState == 3) ||
          (cVar1 = AFE_IsAlwaysNoisy(), cVar1 != '\0')) {
    eVar2 = memcpy_s(*(void **)_refptr_g_tsaBufBl,0x2648,*(void **)_refptr_g_tsaBufPreCMFRaw,
                     (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
    if (eVar2 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Alg/Baseline/BaselineIIR.c",0x84,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
  }
  else {
    for (local_a = 0; local_a < _refptr_g_tsaPrmtFlash->wBufElementCount; local_a = local_a + 1) {
      *(short *)((ulonglong)local_a * 2 + *(longlong *)_refptr_g_tsaBufBl) =
           (short)*(undefined4 *)&_refptr_g_tsaPrmtFlash->field_0x5fc;
    }
  }
  cVar1 = Proximity_IsDetected();
  if ((((cVar1 == '\0') || (_refptr_g_tsaPrpt->dwBaselinePropertyState != 5)) &&
      (*(longlong *)_refptr_g_tsaBufProxBL != 0)) &&
     (eVar2 = memcpy_s(*(void **)_refptr_g_tsaBufProxBL,0x2648,*(void **)_refptr_g_tsaBufBl,
                       (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2), eVar2 != 0)) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/Baseline/BaselineIIR.c",0x91,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: BLIIR_Init @ 6ba46e70
// ============================================================================

void BLIIR_Init(void)

{
                    /* 0x6e70  349  BLIIR_Init */
  return;
}


