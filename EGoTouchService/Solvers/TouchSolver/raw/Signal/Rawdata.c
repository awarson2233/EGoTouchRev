#include "TSACoreInternal.h"

// ============================================================================
// Function: Rawdata_CMF @ 6ba69381
// ============================================================================

void Rawdata_CMF(void)

{
  char cVar1;
  
                    /* 0x29381  1150  Rawdata_CMF */
  cVar1 = TSAStatic_IsHWReset();
  if (cVar1 == '\0') {
    BLIIR_CalcDiffCommon
              (*(short **)_refptr_g_tsaBufDif,*(short **)_refptr_g_tsaBufRaw,
               *(short **)_refptr_g_tsaBufBl);
    CMF_Process();
    BLIIR_CalcRawCommon(*(undefined8 *)_refptr_g_tsaBufRaw,*(undefined8 *)_refptr_g_tsaBufBl,
                        *(undefined8 *)_refptr_g_tsaBufDif);
  }
  return;
}


// ============================================================================
// Function: Rawdata_Process @ 6ba69581
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

void Rawdata_Process(void)

{
  char cProcessNeeded;
  uint dwScaleSignMask;
  ushort wGridIndex;
  byte bRow;
  byte bCol;
  
                    /* 0x29581  1156  Rawdata_Process */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&DAT_6bb5258f,"Rawdata_Process");
  }
  cProcessNeeded = Rawdata_IsProcessNeeded();
  if (cProcessNeeded != '\0') {
    if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & FLAG_USE_DIF_BUF_PATH) == 0) {
      Rawdata_NormalizeGrid
                (*(undefined8 *)_refptr_g_tsaBufRawGrid,_refptr_g_tsaPrmtFlash->wBufElementCount,
                 *(undefined8 *)_refptr_g_tsaBufRawGrid,*(undefined2 *)_refptr_g_gridBufSize);
    }
    else {
      wGridIndex = 0;
      for (bRow = 0; bRow < _refptr_g_tsaPrmtFlash->bRows; bRow = bRow + 1) {
        for (bCol = 0; bCol < _refptr_g_tsaPrmtFlash->bCols; bCol = bCol + 1) {
          dwScaleSignMask = (int)*(uint *)(_refptr_g_tsaPrmtFlashConst + 0x38) >> 0x1f;
          *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)wGridIndex * 2) =
               (short)((int)(((*(uint *)(_refptr_g_tsaPrmtFlashConst + 0x38) ^ dwScaleSignMask) -
                             dwScaleSignMask) *
                            (int)*(short *)(*(longlong *)_refptr_g_tsaBufDif +
                                           (ulonglong)wGridIndex * 2)) / 0x3fff);
          wGridIndex = wGridIndex + 1;
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: Rawdata_IsProcessNeeded @ 6ba69306
// ============================================================================

undefined1 Rawdata_IsProcessNeeded(void)

{
  undefined1 local_9;
  
                    /* 0x29306  1153  Rawdata_IsProcessNeeded */
  local_9 = 1;
  if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & FLAG_USE_DIF_BUF_PATH) == 0) {
    if (((*(int *)(_refptr_g_tsaPrmtFlashConst + 0x38) == 0x3fff) &&
        (*(int *)(_refptr_g_tsaPrmtFlashConst + 0x3c) == 0)) &&
       (_refptr_g_tsaPrmtFlash->bRawdataForceNormalize == '\0')) {
      local_9 = 0;
    }
  }
  else if (*(int *)(_refptr_g_tsaPrmtFlashConst + 0x38) == 0x3fff) {
    local_9 = 0;
  }
  return local_9;
}


// ============================================================================
// Function: Rawdata_NormalizeGrid @ 6ba6946a
// ============================================================================

void Rawdata_NormalizeGrid(void *param_1,uint param_2,void *param_3,uint param_4)

{
  char cVar1;
  undefined2 uVar2;
  errno_t eVar3;
  byte local_1c;
  byte local_1b;
  ushort local_1a;
  
                    /* 0x2946a  1154  Rawdata_NormalizeGrid */
  cVar1 = Rawdata_IsProcessNeeded();
  if (cVar1 == '\0') {
    if ((param_1 != param_3) &&
       (eVar3 = memcpy_s(param_3,(ulonglong)param_4 * 2,param_1,(ulonglong)param_2 * 2), eVar3 != 0)
       ) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Alg/Rawdata/Rawdata.c",0x5a,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
  }
  else {
    local_1a = 0;
    for (local_1b = 0; local_1b < _refptr_g_tsaPrmtFlash->bRows; local_1b = local_1b + 1) {
      for (local_1c = 0; local_1c < _refptr_g_tsaPrmtFlash->bCols; local_1c = local_1c + 1) {
        uVar2 = Rawdata_NormalizeGridUnit
                          (*(undefined2 *)((longlong)param_1 + (ulonglong)local_1a * 2));
        *(undefined2 *)((ulonglong)local_1a * 2 + (longlong)param_3) = uVar2;
        local_1a = local_1a + 1;
      }
    }
  }
  return;
}


// ============================================================================
// Function: Rawdata_NormalizeGridUnit @ 6ba693f0
// ============================================================================

int Rawdata_NormalizeGridUnit(ushort param_1)

{
  uint local_c;
  
                    /* 0x293f0  1155  Rawdata_NormalizeGridUnit */
  if (_refptr_g_tsaPrmtFlash->bRawdataForceNormalize == '\0') {
    local_c = (uint)param_1;
  }
  else {
    local_c = (uint)(short)param_1;
  }
  return (int)(*(int *)(_refptr_g_tsaPrmtFlashConst + 0x38) * local_c) / 0x3fff +
         *(int *)(_refptr_g_tsaPrmtFlashConst + 0x3c);
}


// ============================================================================
// Function: Rawdata_Init @ 6ba69300
// ============================================================================

void Rawdata_Init(void)

{
                    /* 0x29300  1152  Rawdata_Init */
  return;
}


// ============================================================================
// Function: Rawdata_RetrieveBaseline @ 6ba696ed
// ============================================================================

void Rawdata_RetrieveBaseline
               (longlong param_1,undefined8 param_2,longlong param_3,undefined8 param_4,
               longlong param_5, ...)

{
  char cVar1;
  ushort uVar2;
  short sVar3;
  int local_10;
  byte local_a;
  byte local_9;
  
                    /* 0x296ed  1157  Rawdata_RetrieveBaseline */
  if ((param_1 != 0) && (param_3 != 0)) {
    if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & FLAG_USE_DIF_BUF_PATH) == 0) {
      for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bCols; local_9 = local_9 + 1) {
        for (local_a = 0; local_a < _refptr_g_tsaPrmtFlash->bRows; local_a = local_a + 1) {
          uVar2 = (ushort)local_9 + (ushort)local_a * (ushort)_refptr_g_tsaPrmtFlash->bCols;
          sVar3 = *(short *)(param_3 + (ulonglong)uVar2 * 2);
          local_10 = (int)sVar3;
          if ((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) == 0) ||
             ((local_9 != 0 && ((uint)local_9 != _refptr_g_tsaPrmtFlash->bCols - 1)))) {
            sVar3 = TPSensor_RevertScalingDif((int)sVar3,local_9,local_a);
            local_10 = (int)sVar3;
            if (param_5 != 0) {
              sVar3 = CMF_GetNoiseValueFromBuf(param_5,local_9,local_a);
              local_10 = local_10 + sVar3;
            }
          }
          cVar1 = Rawdata_IsProcessNeeded();
          if (cVar1 != '\0') {
            local_10 = (local_10 * 0x3fff) / *(int *)(_refptr_g_tsaPrmtFlashConst + 0x38);
          }
          if ((*_refptr_g_tsaStaticPtr)->bField_0x11a == '\0') {
            sVar3 = (short)local_10 + *(short *)(param_1 + (ulonglong)uVar2 * 2);
          }
          else {
            sVar3 = *(short *)(param_1 + (ulonglong)uVar2 * 2) - (short)local_10;
          }
          *(short *)(param_1 + (ulonglong)uVar2 * 2) = sVar3;
        }
      }
    }
    else {
      for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bCols; local_9 = local_9 + 1) {
        for (local_a = 0; local_a < _refptr_g_tsaPrmtFlash->bRows; local_a = local_a + 1) {
          *(undefined2 *)
           (param_1 +
           (ulonglong)
           (ushort)((ushort)local_9 + (ushort)local_a * (ushort)_refptr_g_tsaPrmtFlash->bCols) * 2)
               = 0;
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: Rawdata_GetUnNormalizedBaseline @ 6ba69937
// ============================================================================

void Rawdata_GetUnNormalizedBaseline(longlong param_1,undefined4 param_2)

{
  char cVar1;
  ushort local_c;
  byte local_a;
  byte local_9;
  
                    /* 0x29937  1151  Rawdata_GetUnNormalizedBaseline */
  cVar1 = Rawdata_IsProcessNeeded();
  if (cVar1 == '\0') {
    TPSensor_RevertTranslateRaw(param_1,param_2);
  }
  else {
    if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & FLAG_USE_DIF_BUF_PATH) == 0) {
      local_c = 0;
      for (local_a = 0; local_a < _refptr_g_tsaPrmtFlash->bRows; local_a = local_a + 1) {
        for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bCols; local_9 = local_9 + 1) {
          *(short *)(param_1 + (ulonglong)local_c * 2) =
               (short)((int)(((uint)*(ushort *)(param_1 + (ulonglong)local_c * 2) -
                             *(int *)(_refptr_g_tsaPrmtFlashConst + 0x3c)) * 0x3fff) /
                      *(int *)(_refptr_g_tsaPrmtFlashConst + 0x38));
          local_c = local_c + 1;
        }
      }
    }
    TPSensor_RevertTranslateRaw(param_1,param_2);
  }
  return;
}


