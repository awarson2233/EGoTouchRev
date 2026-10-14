#include "TSACoreInternal.h"

// ============================================================================
// Function: TouchThold_ToeUseSpecialThold @ 6ba7cd8f
// ============================================================================

undefined8 TouchThold_ToeUseSpecialThold(void)

{
  int iVar1;
  char *pcVar2;
  
                    /* 0x3cd8f  2511  TouchThold_ToeUseSpecialThold */
  pcVar2 = (char *)TSA_GetPrmtName();
  iVar1 = strncmp(pcVar2,"P116",4);
  if (iVar1 != 0) {
    pcVar2 = (char *)TSA_GetPrmtName();
    iVar1 = strncmp(pcVar2,"P150",4);
    if (iVar1 != 0) {
      pcVar2 = (char *)TSA_GetPrmtName();
      iVar1 = strncmp(pcVar2,"P186",4);
      if (iVar1 != 0) {
        return 0;
      }
    }
  }
  return 1;
}


// ============================================================================
// Function: TouchThold_IsSwitching @ 6ba7c90e
// ============================================================================

undefined1 TouchThold_IsSwitching(void)

{
                    /* 0x3c90e  2509  TouchThold_IsSwitching */
  return _refptr_g_tsaPrpt[1].field_0x21;
}


// ============================================================================
// Function: TouchThold_Init @ 6ba7c922
// ============================================================================

void TouchThold_Init(void)

{
  undefined2 uVar1;
  
                    /* 0x3c922  2508  TouchThold_Init */
  uVar1 = TouchThold_GetTholdBasedOnSNR
                    ((int)*(short *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x30));
  *(undefined2 *)&_refptr_g_tsaPrpt[1].field_0x16 = uVar1;
  *(ushort *)&_refptr_g_tsaPrpt[1].field_0x18 = _refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax;
  *(ushort *)&_refptr_g_tsaPrpt[1].field_0x1c = _refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax;
  *(ushort *)&_refptr_g_tsaPrpt[1].field_0x1e = _refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax;
  _refptr_g_tsaPrpt[1].field_0x20 = (byte)_refptr_g_tsaPrmtFlash->field_0x38 >> 2;
  *(undefined2 *)&_refptr_g_tsaPrpt[1].field_0x1a = *(undefined2 *)&_refptr_g_tsaPrpt[1].field_0x18;
  *(undefined2 *)&_refptr_g_tsaPrpt[1].field_0x14 = *(undefined2 *)&_refptr_g_tsaPrpt[1].field_0x18;
  _refptr_g_tsaPrpt[1].field_0x24 = 10;
  *(undefined2 *)&_refptr_g_tsaPrpt[1].field_0x22 = *(undefined2 *)&_refptr_g_tsaPrpt[1].field_0x18;
  _refptr_g_tsaPrpt[1].field_0x21 = 0;
  (*_refptr_g_tsaStaticPtr)[1].field_0xd = 0;
  return;
}


// ============================================================================
// Function: TouchThold_GetTholdBasedOnSNR @ 6ba7c776
// ============================================================================

int TouchThold_GetTholdBasedOnSNR(short param_1)

{
  int iVar1;
  
                    /* 0x3c776  2505  TouchThold_GetTholdBasedOnSNR */
  iVar1 = (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x34) * (int)param_1;
  if (iVar1 < 0) {
    iVar1 = iVar1 + 0xf;
  }
  return iVar1 >> 4;
}


// ============================================================================
// Function: TouchThold_Process @ 6ba7d0ee
// ============================================================================

void TouchThold_Process(short param_1,short param_2,short *param_3,short *param_4,
                       undefined2 *param_5)

{
  short sVar1;
  undefined2 uVar2;
  int iVar3;
  short local_a;
  
                    /* 0x3d0ee  2510  TouchThold_Process */
  sVar1 = TouchThold_GetPeakThold((int)param_1,(int)param_2);
  if ((((*_refptr_g_prevTouchesPtr)->bPrevTouchCount == '\x01') &&
      ((*_refptr_g_prevTouchesPtr)->field_0x5 == '\x01')) &&
     ((*(uint *)&(*_refptr_g_prevTouchesPtr)[0xd].field_0x1c & 0x1000) == 0)) {
    iVar3 = sVar1 * 3;
    if (iVar3 < 0) {
      iVar3 = iVar3 + 3;
    }
    *param_3 = (short)(iVar3 >> 2);
  }
  else {
    *param_3 = sVar1;
  }
  sVar1 = TouchThold_BasedOnExperience((int)*param_3);
  if (*param_3 < sVar1) {
    *param_3 = sVar1;
  }
  if (((_refptr_g_tsaPrpt[1].field_0x28 == '\0') ||
      ((((_refptr_g_tsaPrmtDynamic[0x80] & 4) == 0 ||
        (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 1) == 0)) &&
       (((_refptr_g_tsaPrmtDynamic[0x80] & 8) == 0 ||
        (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x10) == 0)))))) &&
     (((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) == 0 &&
       ((*_refptr_g_tsaStaticPtr)[1].field_0x3 == '\0')) &&
      ((*_refptr_g_tsaStaticPtr)->bFullIncellSignalDisparity == '\0')))) {
    local_a = param_1;
  }
  else {
    local_a = param_2;
  }
  sVar1 = TouchThold_GetNoiseThold((int)param_1,(int)param_2);
  if (local_a < sVar1) {
    local_a = sVar1;
  }
  if (local_a < *param_3) {
    local_a = *param_3;
  }
  *param_4 = local_a;
  uVar2 = TouchThold_GetTouchDownThold((int)*param_4);
  *param_5 = uVar2;
  return;
}


// ============================================================================
// Function: TouchThold_GetNoiseThold @ 6ba7ca48
// ============================================================================

short TouchThold_GetNoiseThold(short param_1,short param_2)

{
  byte bVar1;
  char cVar2;
  short sVar3;
  short local_c;
  short local_a;
  
                    /* 0x3ca48  2503  TouchThold_GetNoiseThold */
  bVar1 = TouchThold_GetNoisyTouchNum();
  local_c = *(short *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x2c);
  if (bVar1 == 1) {
    if (local_c < *(short *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x2e)) {
      local_c = *(short *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x2e);
    }
  }
  else if ((1 < bVar1) && (local_c < *(short *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x30))) {
    local_c = *(short *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x30);
  }
  local_a = TouchThold_GetTholdBasedOnSNR((int)local_c);
  sVar3 = TouchThold_GetTholdBasedOnTouchMode((int)param_1,(int)param_2);
  cVar2 = TSAStatic_IsSafeBaselineApplied();
  if (cVar2 != '\0') {
    local_a = MAX((int)local_a,(int)sVar3);
  }
  *(short *)&_refptr_g_tsaPrpt[1].field_0x14 = local_a;
  return local_a;
}


// ============================================================================
// Function: TouchThold_BasedOnExperience @ 6ba7cb32
// ============================================================================

short TouchThold_BasedOnExperience(short param_1)

{
  short sVar1;
  ushort uVar2;
  int iVar3;
  short local_a;
  
                    /* 0x3cb32  2502  TouchThold_BasedOnExperience */
  if ((_refptr_g_tsaPrmtFlash->dwPeakProcessingMode != 3) &&
     (sVar1 = TSA_GetMinSigPrpt(), _refptr_g_tsaPrmtFlash->dwPeakProcessingMode != 3)) {
    if ((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold <
        _refptr_g_tsaPrpt->nSPeakPositive) {
      uVar2 = _refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold;
      if ((short)uVar2 < 0) {
        uVar2 = uVar2 + 3;
      }
    }
    else {
      uVar2 = _refptr_g_tsaPrpt->nSPeakPositive;
      if ((short)uVar2 < 0) {
        uVar2 = uVar2 + 3;
      }
    }
    local_a = (short)uVar2 >> 2;
    if (((*_refptr_g_prevTouchesPtr)->nTouchCount == 0x100) ||
       ((*_refptr_g_tsaStaticPtr)->bField_0x111 == '\x02')) {
      if ((int)local_a < -(int)sVar1) {
        local_a = -sVar1;
      }
      iVar3 = _refptr_g_tsaPrpt->nSPeakPositive * 3;
      if (iVar3 < 0) {
        iVar3 = iVar3 + 3;
      }
      sVar1 = (short)(iVar3 >> 2);
      if (sVar1 < local_a) {
        local_a = sVar1;
      }
    }
    else if (((*_refptr_g_prevTouchesPtr)->nTouchCount & 200U) == 0) {
      sVar1 = (short)(-(int)sVar1 / 2);
      if (local_a < sVar1) {
        local_a = sVar1;
      }
    }
    param_1 = local_a;
  }
  return param_1;
}


// ============================================================================
// Function: TouchThold_GetPeakThold @ 6ba7cee9
// ============================================================================

short TouchThold_GetPeakThold(short param_1,short param_2)

{
  char cVar1;
  short local_a;
  
  local_a = param_1;
  if (((((_refptr_g_tsaPrmtDynamic[0x80] & 4) == 0) ||
       (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 1) == 0)) &&
      (((_refptr_g_tsaPrmtDynamic[0x80] & 8) == 0 ||
       (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x10) == 0)))) &&
     ((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) == 0 &&
      ((*_refptr_g_tsaStaticPtr)[1].field_0x3 == '\0')))) {
    if ((_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3) &&
       ((*_refptr_g_tsaStaticPtr)->bFullIncellSignalDisparity != '\0')) {
      local_a = param_1 / 2;
    }
  }
  else if (((((2 >> ((byte)*(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x2c & 0x1f) & 1U) == 0) ||
            (*(int *)&_refptr_g_tsaPrpt[1].field_0x30 != 2)) ||
           ((_refptr_g_tsaPrmtDynamic[0x80] & 2) == 0)) ||
          (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) != 0)) {
    local_a = param_2;
    if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) == 0) {
      if (((*(int *)&_refptr_g_tsaPrpt[1].field_0x2c == 2) &&
          (*(int *)&_refptr_g_tsaPrpt[1].field_0x30 == 2)) &&
         (cVar1 = TouchThold_UseHighTholdAtSensitivityMode(), cVar1 != '\0')) {
        local_a = 0x168;
      }
    }
    else {
      cVar1 = TouchThold_SOFInSmartCoverMode();
      if ((cVar1 == '\0') && (cVar1 = TouchThold_UseHighTholdAtSensitivityMode(), cVar1 != '\0')) {
        local_a = 0x1a4;
      }
    }
  }
  else if ((_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3) &&
          ((*_refptr_g_tsaStaticPtr)->bFullIncellSignalDisparity != '\0')) {
    local_a = param_1 / 2;
  }
  return local_a;
}


// ============================================================================
// Function: TouchThold_GetTholdBasedOnTouchMode @ 6ba7c7a5
// ============================================================================

int TouchThold_GetTholdBasedOnTouchMode(ushort param_1,short param_2)

{
  int iVar1;
  
                    /* 0x3c7a5  2506  TouchThold_GetTholdBasedOnTouchMode */
  if (((_refptr_g_tsaPrpt[1].field_0x28 == '\0') ||
      ((((_refptr_g_tsaPrmtDynamic[0x80] & 4) == 0 ||
        (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 1) == 0)) &&
       (((_refptr_g_tsaPrmtDynamic[0x80] & 8) == 0 ||
        (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x10) == 0)))))) &&
     (((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) == 0 &&
       ((*_refptr_g_tsaStaticPtr)[1].field_0x3 == '\0')) &&
      ((*_refptr_g_tsaStaticPtr)->bFullIncellSignalDisparity == '\0')))) {
    iVar1 = (uint)param_1 + (uint)(param_1 >> 0xf);
    iVar1 = CONCAT22((short)((uint)iVar1 >> 0x10),(short)iVar1 >> 1);
  }
  else {
    iVar1 = (param_2 * 4) / 5;
  }
  return iVar1;
}


// ============================================================================
// Function: TouchThold_GetNoisyTouchNum @ 6ba7c896
// ============================================================================

char TouchThold_GetNoisyTouchNum(void)

{
  char local_a;
  byte local_9;
  
                    /* 0x3c896  2504  TouchThold_GetNoisyTouchNum */
  local_a = '\0';
  for (local_9 = 0; local_9 < (*_refptr_g_prevTouchesPtr)->bPrevTouchCount; local_9 = local_9 + 1) {
    if ((int)*(short *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x32) <
        *(int *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_9 * 0x478 + 0x1cc)
       ) {
      local_a = local_a + '\x01';
    }
  }
  return local_a;
}


// ============================================================================
// Function: TouchThold_UseHighTholdAtSensitivityMode @ 6ba7ce03
// ============================================================================

undefined8 TouchThold_UseHighTholdAtSensitivityMode(void)

{
  int iVar1;
  char *pcVar2;
  
  pcVar2 = (char *)TSA_GetPrmtName();
  iVar1 = strncmp(pcVar2,"P116",4);
  if (iVar1 != 0) {
    pcVar2 = (char *)TSA_GetPrmtName();
    iVar1 = strncmp(pcVar2,"P150",4);
    if (iVar1 != 0) {
      pcVar2 = (char *)TSA_GetPrmtName();
      iVar1 = strncmp(pcVar2,"P172",4);
      if (iVar1 != 0) {
        pcVar2 = (char *)TSA_GetPrmtName();
        iVar1 = strncmp(pcVar2,"P173",4);
        if (iVar1 != 0) {
          pcVar2 = (char *)TSA_GetPrmtName();
          iVar1 = strncmp(pcVar2,"P186",4);
          if (iVar1 != 0) {
            return 0;
          }
        }
      }
    }
  }
  return 1;
}


// ============================================================================
// Function: TouchThold_SOFInSmartCoverMode @ 6ba7ceb3
// ============================================================================

undefined8 TouchThold_SOFInSmartCoverMode(void)

{
  undefined8 uVar1;
  
  if ((_refptr_g_tsaPrmtFlash->dwCmfMethodSmartCover == 1) ||
     (_refptr_g_tsaPrmtFlash->dwCmfMethodNormal == 1)) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: TouchThold_GetTouchDownThold @ 6ba7cca5
// ============================================================================

short TouchThold_GetTouchDownThold(short param_1)
{
  short local_2;
  short sPeak;
  short sCfg;
                    /* 0x3cca5  TouchThold_GetTouchDownThold（Ghidra 无法反编译，按汇编重建） */
  local_2 = param_1;
  if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3) {
    if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x1000) == 0) {
      sPeak = *(short *)((char *)_refptr_g_tsaPrpt + 0xa0);
      sCfg = *(short *)((char *)_refptr_g_tsaPrmtFlash + 0x5b4);
      if (sPeak <= sCfg) {
        local_2 = sPeak / 3;
      }
      else {
        local_2 = sCfg / 3;
      }
    }
    if (local_2 < param_1) {
      return param_1;
    }
  }
  return local_2;
}


