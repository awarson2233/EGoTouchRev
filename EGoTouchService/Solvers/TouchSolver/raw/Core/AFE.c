#include "TSACoreInternal.h"

// ============================================================================
// Function: AFE_IsInAutoCheckProcess @ 6bb29ce1
// ============================================================================

undefined1 AFE_IsInAutoCheckProcess(void)

{
                    /* 0xe9ce1  35  AFE_IsInAutoCheckProcess */
  return g_flagInAutoCheckProcess;
}


// ============================================================================
// Function: AFE_ClearAutoCheckFlag @ 6bb29d0d
// ============================================================================

void AFE_ClearAutoCheckFlag(void)

{
                    /* 0xe9d0d  6  AFE_ClearAutoCheckFlag */
  g_flagInAutoCheckProcess = 0;
  return;
}


// ============================================================================
// Function: AFE_AutoCheckProcessInternal @ 6bb2c45b
// ============================================================================

void AFE_AutoCheckProcessInternal
               (undefined8 param_1,undefined2 param_2,undefined4 param_3,void *param_4)

{
  char cVar1;
  errno_t eVar2;
  
                    /* 0xec45b  3  AFE_AutoCheckProcessInternal */
  g_afeTestErrorInfo = &_rdata;
  AFE_AutoCheckAFEStruct();
  if (g_flagInAutoCheckProcess == '\0') {
    AFE_AutoCheckInit();
    g_flagInAutoCheckProcess = '\x01';
    (*_refptr_g_tsaStaticPtr)->nFrameCounter = 0;
  }
  TSALog_Input(param_1,param_3,*(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0xc8,param_4);
  TSAStatic_UpdateCtrlFlags(param_3);
  (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr = param_4;
  (*_refptr_g_tsaStaticPtr)->nFrameCounter = (*_refptr_g_tsaStaticPtr)->nFrameCounter + 1;
  cVar1 = TSAStatic_IsHWReset();
  if (cVar1 != '\0') {
    TSA_MSReset();
  }
  AFE_AutoCheckDataPtr(param_1,param_2);
  Rawdata_Process();
  AFE_FreqShiftStatusUpdate();
  AFE_ScanPreProcess();
  if (100 < (uint)(*_refptr_g_tsaStaticPtr)->nFrameCounter) {
    HardwareAnalyzer_GetRaw2RawPrpt();
    AFE_ScanAutoCheck();
  }
  eVar2 = memcpy_s(*(void **)_refptr_g_tsaBufPreHWRaw,
                   (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,*(void **)_refptr_g_tsaBufRaw,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  if (eVar2 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/TSA_AFE/TSA_AFE_Check.c",0x431,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: AFE_PreProcess @ 6bb2cfd8
// ============================================================================

void AFE_PreProcess(void)

{
                    /* 0xecfd8  51  AFE_PreProcess */
  AFE_ScanPreProcess();
  return;
}


// ============================================================================
// Function: AFE_Process @ 6bb2d032
// ============================================================================

void AFE_Process(void)

{
  char cVar1;
  
                    /* 0xed032  52  AFE_Process */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&DAT_6bb5bebf,"AFE_Process");
  }
  cVar1 = AFE_IsValid();
  if (cVar1 != '\0') {
    cVar1 = AFE_IsInAutoCheckProcess();
    if (cVar1 == '\0') {
      cVar1 = TSA_IsOKToEnterIDLEMode();
      if (cVar1 == '\0') {
        AFE_ScanProcess();
      }
    }
  }
  return;
}


// ============================================================================
// Function: AFE_GetFrame @ 6bb2cd26
// ============================================================================

AFE_Frame * AFE_GetFrame(void)

{
                    /* 0xecd26  9  AFE_GetFrame */
  return PTR_AFE_Frame;
}


// ============================================================================
// Function: AFE_IsInIDLEMode @ 6bb2cdc7
// ============================================================================

bool AFE_IsInIDLEMode(void)

{
  bool bVar1;
  
                    /* 0xecdc7  36  AFE_IsInIDLEMode */
  if (PTR_AFE_Frame == (AFE_Frame *)0x0) {
    bVar1 = false;
  }
  else {
    bVar1 = (PTR_AFE_Frame->dwFlags & 1) != 0;
  }
  return bVar1;
}


// ============================================================================
// Function: AFE_IsInSideIDLEMode @ 6bb2cf15
// ============================================================================

undefined4 AFE_IsInSideIDLEMode(void)

{
  uint uVar1;
  undefined4 uVar2;
  
                    /* 0xecf15  38  AFE_IsInSideIDLEMode */
  if (PTR_AFE_Frame == (AFE_Frame *)0x0) {
    uVar2 = 0;
  }
  else {
    uVar1 = PTR_AFE_Frame->dwFlags & 0x8000;
    uVar2 = CONCAT31((int3)(uVar1 >> 8),uVar1 != 0);
  }
  return uVar2;
}


// ============================================================================
// Function: AFE_IsInProximityScreenOffState @ 6bb2cf42
// ============================================================================

undefined4 AFE_IsInProximityScreenOffState(void)

{
  uint uVar1;
  undefined4 uVar2;
  
                    /* 0xecf42  37  AFE_IsInProximityScreenOffState */
  if (PTR_AFE_Frame == (AFE_Frame *)0x0) {
    uVar2 = 0;
  }
  else {
    uVar1 = PTR_AFE_Frame->dwFlags & 0x400;
    uVar2 = CONCAT31((int3)(uVar1 >> 8),uVar1 != 0);
  }
  return uVar2;
}


// ============================================================================
// Function: AFE_AutoCheckInit @ 6bb29d1a
// ============================================================================

void AFE_AutoCheckInit(void)

{
                    /* 0xe9d1a  1  AFE_AutoCheckInit */
  AFE_FreqShiftCheckReset();
  AFE_IDLECheckReset();
  return;
}


// ============================================================================
// Function: AFE_ScanAutoCheck @ 6bb2c027
// ============================================================================

void AFE_ScanAutoCheck(void)

{
  void *pvVar1;
  char cVar2;
  int iVar3;
  uint uVar4;
  void *pvVar5;
  
  pvVar1 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  pvVar5 = (void *)((longlong)pvVar1 - (longlong)g_prevTimeStamp);
  g_prevTimeStamp = pvVar1;
  if (pvVar5 != pvVar1) {
    if (*(longlong *)(_refptr_g_thpAFEStruct + 8) == 0) {
      cVar2 = TSALog_RtEnable(2,0x3e,0x10);
      if (cVar2 != '\0') {
        StrPrintf_Process(0,"\ng_thpAFEStruct.caps is NULL!\n");
      }
      if (g_afeTestLevel != 0) {
        g_afeTestErrorInfo = s_CapsIsNull_6bb5bd97;
      }
    }
    if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x400000) == 0) {
      uVar4 = AFE_FreqShiftAutoCheck();
      if (uVar4 == 0) {
        iVar3 = AFE_IDLEAutoCheck();
        if (iVar3 == 6) {
          AFE_AutoCheckDoneLog();
        }
        else {
          iVar3 = AFE_IDLEAutoCheck();
          if (iVar3 == 10) {
            cVar2 = TSAStatic_IsAFEJustEnterIDLEMode();
            if ((cVar2 == '\0') || (g_idleLoop != 1)) {
              cVar2 = TSAStatic_IsAFEJustExitIDLEMode();
              if (cVar2 == '\0') {
                cVar2 = AFE_IsInIDLEMode();
                if ((cVar2 != '\0') && (cVar2 = TSAStatic_IsAFEJustEnterIDLEMode(), cVar2 == '\0'))
                {
                  AFE_IDLEStableModeAutoCheck(pvVar5);
                }
              }
              else {
                AFE_JustExitIDLEModeAutoCheck();
              }
            }
            else {
              AFE_JustEnterIDLEModeAutoCheck();
            }
          }
          else if (iVar3 == 7) {
            g_numIDLEAbnormalExt = g_numIDLEAbnormalExt + 1;
          }
          AFE_IdleModeAutoCheckLog();
        }
      }
      else {
        AFE_AutoCheckRaw2RawLevel(pvVar1);
        if (uVar4 < 4) {
          AFE_AutoCheckFreqShift(pvVar1,pvVar5);
        }
        AFE_AutoCheckRaw2RawAndFreqShiftLog();
      }
    }
    else {
      iVar3 = AFE_ProximityAutoCheck();
      if (iVar3 == 0xc) {
        AFE_AFTModeProximityAutoCheckUnit();
      }
      else {
        AFE_AFTModeNormalAutoCheckUnit();
      }
    }
  }
  return;
}


// ============================================================================
// Function: AFE_AutoCheckAFEStruct @ 6bb2c220
// ============================================================================

void AFE_AutoCheckAFEStruct(void)

{
  char cVar1;
  
  if (*(longlong *)(_refptr_g_thpAFEStruct + 0x10) == 0) {
    if (g_afeTestLevel != 0) {
      g_afeTestErrorInfo = s_FrameIsNull_6bb5bdba;
    }
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\ng_thpAFEStruct.frame is NULL!\n");
    }
  }
  else if (*(longlong *)(_refptr_g_thpAFEStruct + 8) == 0) {
    if (g_afeTestLevel != 0) {
      g_afeTestErrorInfo = s_CapsIsNull_6bb5bd97;
    }
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\ng_thpAFEStruct.caps is NULL!\n");
    }
  }
  else if (*(longlong *)_refptr_g_thpAFEStruct == 0) {
    if (g_afeTestLevel != 0) {
      g_afeTestErrorInfo = s_InfoIsNull_6bb5bde8;
    }
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\ng_thpAFEStruct.info is NULL!\n");
    }
  }
  return;
}


// ============================================================================
// Function: AFE_AutoCheckDataPtr @ 6bb2c332
// ============================================================================

void AFE_AutoCheckDataPtr(void *param_1,ushort param_2)

{
  errno_t eVar1;
  
  if (param_1 != (void *)0x0) {
    if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & FLAG_USE_DIF_BUF_PATH) == 0) {
      eVar1 = memcpy_s(*(void **)_refptr_g_tsaBufRaw,
                       (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2,param_1,
                       (ulonglong)param_2);
      if (eVar1 != 0) {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                          "../../src/Extension/TSA_AFE/TSA_AFE_Check.c",0x3c5,"rc == EOK");
        HardwareAnalyzer_SetSoftException();
      }
    }
    else {
      eVar1 = memcpy_s(*(void **)_refptr_g_tsaBufDif,
                       (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2,param_1,
                       (ulonglong)param_2);
      if (eVar1 != 0) {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                          "../../src/Extension/TSA_AFE/TSA_AFE_Check.c",0x3c1,"rc == EOK");
        HardwareAnalyzer_SetSoftException();
      }
    }
  }
  return;
}


// ============================================================================
// Function: AFE_FreqShiftStatusUpdate @ 6bb2c441
// ============================================================================

void AFE_FreqShiftStatusUpdate(void)

{
  g_thpAFEStructFrameStatus = *(undefined4 *)(*(longlong *)(_refptr_g_thpAFEStruct + 0x10) + 0x34);
  return;
}


// ============================================================================
// Function: AFE_ScanPreProcess @ 6bb2ee5e
// ============================================================================

void AFE_ScanPreProcess(void)

{
                    /* 0xeee5e  57  AFE_ScanPreProcess */
  AFE_UpdateCurrScanRateUnit();
  AFE_UpdateCurrScanFreqUnit();
  if (g_thpAFEScanStruct == -1) {
    g_thpAFEScanStruct = DAT_6bca3981;
  }
  if (DAT_6bca3a88 == -1) {
    DAT_6bca3a88 = DAT_6bca3a89;
  }
  return;
}


// ============================================================================
// Function: AFE_IsValid @ 6bb2cfec
// ============================================================================

bool AFE_IsValid(void)

{
  bool local_9;
  
                    /* 0xecfec  48  AFE_IsValid */
  local_9 = DAT_6bbaa5c8 != 0 && PTR_AFE_Frame != (AFE_Frame *)0x0;
  if (g_thpAFEStruct == 0) {
    local_9 = false;
  }
  return local_9;
}


// ============================================================================
// Function: AFE_ScanProcess @ 6bb2eec5
// ============================================================================

void AFE_ScanProcess(void)

{
                    /* 0xeeec5  58  AFE_ScanProcess */
  AFE_NextScanFreqIndexProcess();
  AFE_NextScanRateIndexProcess();
  return;
}


// ============================================================================
// Function: AFE_GetGesture @ 6bb2cda4
// ============================================================================

uint AFE_GetGesture(void)

{
  uint uVar1;
  
                    /* 0xecda4  10  AFE_GetGesture */
  if (PTR_AFE_Frame == (AFE_Frame *)0x0) {
    uVar1 = 0;
  }
  else {
    uVar1 = PTR_AFE_Frame->dwGesture;
  }
  return uVar1;
}


// ============================================================================
// Function: AFE_IsManualFreqHopping @ 6bb2cfaf
// ============================================================================

undefined4 AFE_IsManualFreqHopping(void)

{
  undefined4 uVar1;
  
                    /* 0xecfaf  40  AFE_IsManualFreqHopping */
  if (DAT_6bbaa5c8 == 0) {
    uVar1 = 0;
  }
  else {
    uVar1 = CONCAT31((int3)((uint)*(int *)(DAT_6bbaa5c8 + 0x2c) >> 8),
                     *(int *)(DAT_6bbaa5c8 + 0x2c) == 1);
  }
  return uVar1;
}


// ============================================================================
// Function: AFE_AutoCheckToEnterIDLE @ 6bb29cee
// ============================================================================

undefined1 AFE_AutoCheckToEnterIDLE(void)

{
                    /* 0xe9cee  4  AFE_AutoCheckToEnterIDLE */
  return g_autoCheckToEnterIDLE;
}


// ============================================================================
// Function: AFE_IsAllFreqNoisy @ 6bb2e281
// ============================================================================

byte AFE_IsAllFreqNoisy(void)

{
  byte local_9;
  
                    /* 0xee281  28  AFE_IsAllFreqNoisy */
  local_9 = 0;
  if ((*(longlong *)(_refptr_g_thpAFEStruct + 8) != 0) &&
     (*(int *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x2c) == 1)) {
    local_9 = (byte)*(undefined4 *)(&DAT_6bca3a94 + (longlong)(int)(uint)DAT_6bca3a89 * 0xc) & 1;
  }
  return local_9;
}


// ============================================================================
// Function: AFE_GetNextScanRateIndex @ 6bb2d8a1
// ============================================================================

undefined8 AFE_GetNextScanRateIndex(byte *param_1,char *param_2)

{
  ushort uVar1;
  char cVar2;
  undefined2 uVar3;
  int iVar4;
  undefined8 uVar5;
  byte local_9;
  
                    /* 0xed8a1  13  AFE_GetNextScanRateIndex */
  *param_1 = 0;
  if (*(longlong *)(_refptr_g_thpAFEStruct + 8) == 0) {
    *param_1 = DAT_6bca3a89;
    uVar5 = 1;
  }
  else {
    cVar2 = AFE_IsInAutoCheckProcess();
    if (cVar2 == '\0') {
      uVar3 = Proximity_GetNextScanRate();
      iVar4 = AFE_GetTargetScanRateIndex(uVar3,param_1);
      if (iVar4 == 1) {
        if ((_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3) &&
           (*(char *)(*(longlong *)_refptr_g_touchesPtr + 3) != '\0')) {
          AFE_GetTargetScanRateIndex(0x3c,param_1);
        }
        else {
          cVar2 = TSAStatic_IsAllFreqNoisy();
          if (cVar2 == '\0') {
            if (((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) == 0) ||
                (cVar2 = TSAStatic_IsOKToEnterIDLEMode(), cVar2 == '\0')) ||
               (((*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags & 4) != 0)) {
              if (*(int *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x2c) == 1) {
                *param_1 = DAT_6bca3a8a;
              }
              else {
                cVar2 = DoubleReport_Scenario();
                if (cVar2 == '\0') {
                  AFE_GetTargetScanRateIndex(DAT_6bca3a8c,param_1);
                }
                else {
                  *param_2 = '\0';
                  local_9 = 0;
                  AFE_GetTargetScanRateIndex(DAT_6bca3a8c >> 1,&local_9);
                  uVar1 = *(ushort *)
                           (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x20) +
                           (ulonglong)local_9 * 2);
                  if ((uint)uVar1 + (uint)uVar1 == (uint)DAT_6bca3a8c) {
                    *param_2 = '\x01';
                    *param_1 = local_9;
                  }
                  if ((DAT_6bca3a8c == 0x78) &&
                     (AFE_GetTargetScanRateIndex(0x41,&local_9),
                     *(short *)(*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x20) +
                               (ulonglong)local_9 * 2) == 0x41)) {
                    *param_2 = '\x01';
                    *param_1 = local_9;
                  }
                  if (*param_2 == '\0') {
                    AFE_GetTargetScanRateIndex(DAT_6bca3a8c,param_1);
                  }
                }
              }
            }
            else {
              AFE_GetTargetScanRateIndex(0x3c,param_1);
            }
          }
          else {
            AFE_GetTargetScanRateIndex(0x3c,param_1);
          }
        }
      }
    }
    else {
      *param_1 = DAT_6bca3a8a;
    }
    if (*param_1 < *(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x18)) {
      if (*param_1 == DAT_6bca3a89) {
        *param_1 = DAT_6bca3a89;
        uVar5 = 1;
      }
      else {
        uVar5 = 0;
      }
    }
    else {
      *param_1 = DAT_6bca3a89;
      uVar5 = 1;
    }
  }
  return uVar5;
}


// ============================================================================
// Function: AFE_GetScanFreq @ 6bb2d2be
// ============================================================================

undefined2 AFE_GetScanFreq(void)

{
  undefined2 uVar1;
  
                    /* 0xed2be  16  AFE_GetScanFreq */
  if (*(longlong *)(_refptr_g_thpAFEStruct + 0x10) == 0) {
    uVar1 = 0;
  }
  else {
    uVar1 = *(undefined2 *)(*(longlong *)(_refptr_g_thpAFEStruct + 0x10) + 0x30);
  }
  return uVar1;
}


// ============================================================================
// Function: AFE_FreqShiftCheckReset @ 6bb29b90
// ============================================================================

void AFE_FreqShiftCheckReset(void)

{
  g_freqLoop = 0;
  g_fromRate = 0xff;
  g_fromFreq = 0xff;
  g_toRate = 0xff;
  g_toFreq = 0xff;
  g_maxRaw2RawDif = 0x8000;
  g_minRaw2RawDif = 0x7fff;
  g_maxNumRaw2RawOverNoise = 0;
  g_maxRaw2RawDifTimeStamp = 0;
  g_minRaw2RawDifTimeStamp = 0;
  g_maxNumRaw2RawOverNoiseTimeStamp = 0;
  g_maxFreqShiftLatencyTimeStamp = 0;
  g_maxFreqShiftLatency = 0;
  g_flagTraversedDone = 1;
  g_flagInAutoCheckProcess = 0;
  g_prevTimeStamp = 0;
  g_autoCheckToEnterIDLE = 0;
  g_freqShiftedFlagWaitTimes = 0;
  return;
}


// ============================================================================
// Function: AFE_IDLECheckReset @ 6bb29c60
// ============================================================================

void AFE_IDLECheckReset(void)

{
  g_scanFreq = 0;
  g_scanRate = 0;
  g_maxIDLEEnterTimeStamp = 0;
  g_maxIDLEExitTimeStamp = 0;
  g_maxIDLEIntervalTimeStamp = 0;
  g_maxIDLEEnterTime = 0;
  g_maxIDLEExitTime = 0;
  g_maxIDLEIntervalTime = 0;
  g_minIDLEIntervalTime = 0xffffffffffffffff;
  g_numIDLEAbnormalExt = 0;
  g_idleCheckLoop = 0;
  AFE_IDLECheckResetUnit();
  return;
}


// ============================================================================
// Function: AFE_FreqShiftAutoCheck @ 6bb29fef
// ============================================================================

undefined8 AFE_FreqShiftAutoCheck(void)

{
  char cVar1;
  int iVar2;
  undefined8 uVar3;
  
  if (g_freqLoop < 0x33) {
    if (g_flagTraversedDone == '\0') {
      if (((g_thpAFEStructFrameStatus & 4) == 0) && (g_toFreq != -1)) {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                          "../../src/Extension/TSA_AFE/TSA_AFE_Check.c",0x105,
                          "TSA_AFE_AUTO_CHECK_FREQ_SHIFT_PENDING");
        if (g_freqShiftedFlagWaitTimes < 0x14) {
          g_freqShiftedFlagWaitTimes = g_freqShiftedFlagWaitTimes + 1;
        }
        else {
          if (g_afeTestLevel != 0) {
            g_afeTestErrorInfo = s_NoFreqShiftDoneflag_6bb5b206;
          }
          cVar1 = TSALog_RtEnable(2,0x3e,0x10);
          if (cVar1 != '\0') {
            StrPrintf_Process(0,
                              "THP_AFE_STATUS_FREQ_SHIFT_DONE is not set in g_thpAFEStruct.frame->status\n"
                             );
          }
        }
        uVar3 = 4;
      }
      else {
        g_freqShiftedFlagWaitTimes = 0;
        if ((_refptr_g_thpAFEScanStruct[1] == g_fromFreq) &&
           (_refptr_g_thpAFEScanStruct[0x109] == g_fromRate)) {
          if (g_toFreq == -1) {
            g_toFreq = '\0';
            g_toRate = '\0';
          }
          else {
            iVar2 = AFE_FreqShiftSetNextIndex();
            if (iVar2 == 0) {
              return 2;
            }
          }
          if (((g_toRate == g_fromRate) && (g_toFreq == g_fromFreq)) &&
             (iVar2 = AFE_FreqShiftSetNextIndex(), iVar2 == 0)) {
            return 2;
          }
          _refptr_g_thpAFEScanStruct[2] = g_toFreq;
          _refptr_g_thpAFEScanStruct[0x10a] = g_toRate;
        }
        else if ((_refptr_g_thpAFEScanStruct[1] == g_toFreq) &&
                (_refptr_g_thpAFEScanStruct[0x109] == g_toRate)) {
          _refptr_g_thpAFEScanStruct[2] = g_fromFreq;
          _refptr_g_thpAFEScanStruct[0x10a] = g_fromRate;
        }
        else if (g_toFreq != -1) {
          AFE_FreqShiftAutoCheckAbnornalLog();
          return 5;
        }
        uVar3 = 1;
      }
    }
    else {
      iVar2 = AFE_FreqShiftTraversedDoneInitFreqRate();
      if (iVar2 == 0) {
        uVar3 = 3;
      }
      else {
        uVar3 = 1;
      }
    }
  }
  else {
    uVar3 = 0;
  }
  return uVar3;
}


// ============================================================================
// Function: AFE_IDLEAutoCheck @ 6bb2a466
// ============================================================================

int AFE_IDLEAutoCheck(void)

{
  char cVar1;
  int local_c;
  
  local_c = 9;
  if (g_scanRate < *(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x18)) {
    if (g_scanFreq < *(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 10)) {
      _refptr_g_thpAFEScanStruct[2] = g_scanFreq;
      _refptr_g_thpAFEScanStruct[0x10a] = g_scanRate;
      if ((_refptr_g_thpAFEScanStruct[2] == _refptr_g_thpAFEScanStruct[1]) &&
         (_refptr_g_thpAFEScanStruct[0x10a] == _refptr_g_thpAFEScanStruct[0x109])) {
        local_c = AFE_IDLEAutoCheckUnit();
      }
      else {
        local_c = 9;
      }
    }
    if (local_c == 6) {
      cVar1 = AFE_IsInIDLEMode();
      if (cVar1 == '\0') {
        AFE_IDLECheckResetUnit();
        g_timeStampToExitIDLE = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
        g_scanFreq = g_scanFreq + 1;
        if (*(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 10) <= g_scanFreq) {
          g_scanRate = g_scanRate + 1;
          if (*(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x18) <= g_scanRate) {
            return 6;
          }
          g_scanFreq = 0;
        }
        _refptr_g_thpAFEScanStruct[2] = g_scanFreq;
        _refptr_g_thpAFEScanStruct[0x10a] = g_scanRate;
      }
      else {
        g_autoCheckToEnterIDLE = 0;
      }
      local_c = 9;
    }
  }
  else {
    local_c = 6;
  }
  return local_c;
}


// ============================================================================
// Function: AFE_ProximityAutoCheck @ 6bb2a9c8
// ============================================================================

undefined4 AFE_ProximityAutoCheck(void)

{
  undefined4 local_c;
  
  local_c = 0xb;
  if (DAT_6bbaa502 < 100) {
    if (DAT_6bbaa578 < *(short *)_refptr_g_tsaPrpt) {
      DAT_6bbaa578 = *(short *)_refptr_g_tsaPrpt;
      DAT_6bbaa580 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    }
    if (*(short *)&_refptr_g_tsaPrpt->field_0x2 < DAT_6bbaa57a) {
      DAT_6bbaa57a = *(short *)&_refptr_g_tsaPrpt->field_0x2;
      DAT_6bbaa588 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    }
    switch(g_autoCheckProxState) {
    case 0:
      AFE_ProximityForceEnter();
      break;
    case 1:
      AFE_ProximityWaitUntilEnter();
      break;
    case 2:
      AFE_ProximityWaitUntilAfterEnter();
      break;
    case 3:
      AFE_ProximityKeepInEnter();
      break;
    case 4:
      AFE_ProximityForceExit();
      break;
    case 5:
      AFE_ProximityWaitUntilExit();
      break;
    case 6:
      AFE_ProximityKeepInExit();
    }
  }
  else {
    local_c = 0xc;
  }
  return local_c;
}


// ============================================================================
// Function: AFE_AFTModeNormalAutoCheckUnit @ 6bb2aad8
// ============================================================================

void AFE_AFTModeNormalAutoCheckUnit(void)

{
  char cVar1;
  
  StrPrintf_Process(0,&DAT_6bb5b338);
  StrPrintf_Process(0,"\nProximity Auto Check: loop(%d), state(%d)",DAT_6bbaa502,
                    g_autoCheckProxState);
  if (0x15e < DAT_6bbaa558) {
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\ntimeDurationToExitProximity_AFE(%lld) <= 350 -> failed",DAT_6bbaa558);
    }
  }
  if (500 < DAT_6bbaa550) {
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\ntimeDurationToExitProximity(%lld) <= 500 -> failed",DAT_6bbaa550);
    }
  }
  if (0x96 < DAT_6bbaa520) {
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\ntimeDurationToEnterProximity_AFE(%lld) <= 150 -> failed",DAT_6bbaa520);
    }
  }
  if (0xfa < DAT_6bbaa518) {
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\ntimeDurationToEnterProximity(%lld) <= 250 -> failed",DAT_6bbaa518);
    }
  }
  if (0x96 < *(short *)_refptr_g_tsaPrpt) {
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\nmaxRawShift(%d) <= 150 -> failed",(int)*(short *)_refptr_g_tsaPrpt);
    }
  }
  if (*(short *)&_refptr_g_tsaPrpt->field_0x2 < -0x96) {
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\nminRawShift(%d) >= -150 -> failed",
                        (int)*(short *)&_refptr_g_tsaPrpt->field_0x2);
    }
  }
  return;
}


// ============================================================================
// Function: AFE_AFTModeProximityAutoCheckUnit @ 6bb2acbd
// ============================================================================

void AFE_AFTModeProximityAutoCheckUnit(void)

{
  char cVar1;
  
  cVar1 = TSALog_RtEnable(2,0x3e,0x10);
  if (cVar1 != '\0') {
    StrPrintf_Process(0,&DAT_6bb5b338);
    if ((((DAT_6bbaa568 < 0x191) && (DAT_6bbaa560 < 0x259)) && (DAT_6bbaa530 < 0xc9)) &&
       (((DAT_6bbaa528 < 0x12d && (DAT_6bbaa578 < 0xc9)) && (-0xc9 < DAT_6bbaa57a)))) {
      StrPrintf_Process(0,"\nProximity Auto Check Done: PASS!!!");
    }
    else {
      StrPrintf_Process(0,"\nProximity Auto Check Done: FAIL!!!");
    }
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\nmaxTimeDurationToExitProximity(%lld VS 600), AFEPart(%lld) @%lld",
                        DAT_6bbaa560,DAT_6bbaa568,DAT_6bbaa570);
    }
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\nmaxTimeDurationToEnterProximity(%lld VS 300), AFEPart(%lld) @%lld",
                        DAT_6bbaa528,DAT_6bbaa530,DAT_6bbaa538);
    }
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\nmaxRawShift(%d VS 200) @%lld",(int)DAT_6bbaa578,DAT_6bbaa580);
    }
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\ninRawShift(%d VS -200) @%lld",(int)DAT_6bbaa57a,DAT_6bbaa588);
    }
  }
  return;
}


// ============================================================================
// Function: AFE_AutoCheckRaw2RawLevel @ 6bb2ae86
// ============================================================================

void AFE_AutoCheckRaw2RawLevel(undefined8 param_1)

{
  char cVar1;
  
  if ((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 7 < *(short *)_refptr_g_tsaPrpt
     ) {
    if (1 < g_afeTestLevel) {
      g_afeTestErrorInfo = s_MaxRaw2RawDif_exceed_6bb5b5bf;
    }
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\nMaxRaw2RawDif(%d) > %d -> failed",(int)*(short *)_refptr_g_tsaPrpt,
                        (int)((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 7));
    }
  }
  else if ((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 10 <
           *(short *)_refptr_g_tsaPrpt) {
    if (2 < g_afeTestLevel) {
      g_afeTestErrorInfo = s_MaxRaw2RawDif_exceed_6bb5b5bf;
    }
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\nMaxRaw2RawDif(%d) > %d -> failed",(int)*(short *)_refptr_g_tsaPrpt,
                        (int)((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 10));
    }
  }
  if ((int)((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 7) <
      -(int)*(short *)&_refptr_g_tsaPrpt->field_0x2) {
    if (1 < g_afeTestLevel) {
      g_afeTestErrorInfo = s_MinRaw2RawDif_exceed_6bb5b5fa;
    }
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\n-g_minRaw2RawDif(%d) > %d -> failed",
                        -(int)*(short *)&_refptr_g_tsaPrpt->field_0x2,
                        (int)((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 7));
    }
  }
  else if ((int)((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 10) <
           -(int)*(short *)&_refptr_g_tsaPrpt->field_0x2) {
    if (2 < g_afeTestLevel) {
      g_afeTestErrorInfo = s_MinRaw2RawDif_exceed_6bb5b5fa;
    }
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\n-g_minRaw2RawDif(%d) > %d -> failed",
                        -(int)*(short *)&_refptr_g_tsaPrpt->field_0x2,
                        (int)((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 10));
    }
  }
  if (g_maxRaw2RawDif < *(short *)_refptr_g_tsaPrpt) {
    g_maxRaw2RawDif = *(short *)_refptr_g_tsaPrpt;
    g_maxRaw2RawDifTimeStamp = param_1;
  }
  if (*(short *)&_refptr_g_tsaPrpt->field_0x2 < g_minRaw2RawDif) {
    g_minRaw2RawDif = *(short *)&_refptr_g_tsaPrpt->field_0x2;
    g_minRaw2RawDifTimeStamp = param_1;
  }
  if (g_maxNumRaw2RawOverNoise < *(ushort *)&_refptr_g_tsaPrpt->field_0x4) {
    g_maxNumRaw2RawOverNoise = *(ushort *)&_refptr_g_tsaPrpt->field_0x4;
    g_maxNumRaw2RawOverNoiseTimeStamp = param_1;
  }
  return;
}


// ============================================================================
// Function: AFE_AutoCheckFreqShift @ 6bb2b226
// ============================================================================

void AFE_AutoCheckFreqShift(undefined8 param_1,ulonglong param_2)

{
  char cVar1;
  
  if (param_2 < 0x47) {
    if (0x32 < param_2) {
      if (2 < g_afeTestLevel) {
        g_afeTestErrorInfo = s_FreqShiftLatency_exceed_6bb5b635;
      }
      cVar1 = TSALog_RtEnable(2,0x3e,0x10);
      if (cVar1 != '\0') {
        StrPrintf_Process(0,"\nFreqShiftLatency(%lld) > 50 -> failed",param_2);
      }
    }
  }
  else {
    if (1 < g_afeTestLevel) {
      g_afeTestErrorInfo = s_FreqShiftLatency_exceed_6bb5b635;
    }
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\nFreqShiftLatency(%lld) > 70 -> failed",param_2);
    }
  }
  if (g_maxFreqShiftLatency < param_2) {
    g_maxFreqShiftLatencyTimeStamp = param_1;
    g_maxFreqShiftLatency = param_2;
  }
  return;
}


// ============================================================================
// Function: AFE_AutoCheckRaw2RawAndFreqShiftLog @ 6bb2b303
// ============================================================================

void AFE_AutoCheckRaw2RawAndFreqShiftLog(void)

{
  char cVar1;
  
  cVar1 = TSALog_RtEnable(2,0x3e,0x10);
  if (cVar1 != '\0') {
    StrPrintf_Process(0,"\nFreq Shift Auto Checking (%d)",g_freqLoop);
    StrPrintf_Process(0,"\ng_fromRate: %d, g_fromFreq: %d, g_toRate: %d, g_toFreq: %d",g_fromRate,
                      g_fromFreq,g_toRate,g_toFreq);
    StrPrintf_Process(0,"\ncurrRate: %d, currFreq: %d, nextRate: %d, nextFreq: %d",
                      _refptr_g_thpAFEScanStruct[0x109],_refptr_g_thpAFEScanStruct[1],
                      _refptr_g_thpAFEScanStruct[0x10a],_refptr_g_thpAFEScanStruct[2]);
    StrPrintf_Process(0,"\nmaxRaw2RawDif: %d at time: %lld",(int)g_maxRaw2RawDif,
                      g_maxRaw2RawDifTimeStamp);
    StrPrintf_Process(0,"\nminRaw2RawDif: %d at time: %lld",(int)g_minRaw2RawDif,
                      g_minRaw2RawDifTimeStamp);
    StrPrintf_Process(0,"\nmaxNumRaw2RawOverNoise: %d at time: %ld",g_maxNumRaw2RawOverNoise,
                      g_maxNumRaw2RawOverNoiseTimeStamp);
    StrPrintf_Process(0,"\nmaxFreqShiftLatency: %lld at time: %lld",g_maxFreqShiftLatency,
                      g_maxFreqShiftLatencyTimeStamp);
    StrPrintf_Process(0,&DAT_6bb5b338);
  }
  return;
}


// ============================================================================
// Function: AFE_JustEnterIDLEModeAutoCheck @ 6bb2b49d
// ============================================================================

void AFE_JustEnterIDLEModeAutoCheck(void)

{
  char cVar1;
  ulonglong uVar2;
  
  if (g_timeStampToEnterIDLE < (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr) {
    uVar2 = (longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - (longlong)g_timeStampToEnterIDLE;
    if (uVar2 < 0xc9) {
      if (0x32 < uVar2) {
        if (2 < g_afeTestLevel) {
          g_afeTestErrorInfo = s_IDLEEnterTime_exceed_6bb5b7e1;
        }
        cVar1 = TSALog_RtEnable(2,0x3e,0x10);
        if (cVar1 != '\0') {
          StrPrintf_Process(0,"\nIDLEEnterTime(%lld) > 50 -> failed",uVar2);
        }
      }
    }
    else {
      if (1 < g_afeTestLevel) {
        g_afeTestErrorInfo = s_IDLEEnterTime_exceed_6bb5b7e1;
      }
      cVar1 = TSALog_RtEnable(2,0x3e,0x10);
      if (cVar1 != '\0') {
        StrPrintf_Process(0,"\nIDLEEnterTime(%lld) > 200 -> failed",uVar2);
      }
    }
    if (g_maxIDLEEnterTime <
        (ulonglong)
        ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - (longlong)g_timeStampToEnterIDLE)) {
      g_maxIDLEEnterTime =
           (longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - (longlong)g_timeStampToEnterIDLE;
      g_maxIDLEEnterTimeStamp = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    }
  }
  return;
}


// ============================================================================
// Function: AFE_JustExitIDLEModeAutoCheck @ 6bb2b5f9
// ============================================================================

void AFE_JustExitIDLEModeAutoCheck(void)

{
  char cVar1;
  ulonglong uVar2;
  
  if (g_timeStampToExitIDLE < (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr) {
    uVar2 = (longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - (longlong)g_timeStampToExitIDLE;
    if (uVar2 < 0xc9) {
      if (0x32 < uVar2) {
        if (2 < g_afeTestLevel) {
          g_afeTestErrorInfo = s_IDLEExitTime_exceed_6bb5b844;
        }
        cVar1 = TSALog_RtEnable(2,0x3e,0x10);
        if (cVar1 != '\0') {
          StrPrintf_Process(0,"\nIDLEExitTime(%lld) > 50 -> failed",uVar2);
        }
      }
    }
    else {
      if (1 < g_afeTestLevel) {
        g_afeTestErrorInfo = s_IDLEExitTime_exceed_6bb5b844;
      }
      cVar1 = TSALog_RtEnable(2,0x3e,0x10);
      if (cVar1 != '\0') {
        StrPrintf_Process(0,"\nIDLEExitTime(%ld) > 200 -> failed",uVar2);
      }
      cVar1 = TSALog_RtEnable(2,0x3e,0x10);
      if (cVar1 != '\0') {
        StrPrintf_Process(0,"\nTSAStatic(sys, timeStamp)(%lld)",
                          (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr);
      }
      cVar1 = TSALog_RtEnable(2,0x3e,0x10);
      if (cVar1 != '\0') {
        StrPrintf_Process(0,"\ntimeStampToExitIDLE(%lld)",g_timeStampToExitIDLE);
      }
    }
    if (g_maxIDLEExitTime <
        (ulonglong)
        ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - (longlong)g_timeStampToExitIDLE)) {
      g_maxIDLEExitTime =
           (longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - (longlong)g_timeStampToExitIDLE;
      g_maxIDLEExitTimeStamp = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    }
  }
  return;
}


// ============================================================================
// Function: AFE_IDLEStableModeAutoCheck @ 6bb2b7cb
// ============================================================================

void AFE_IDLEStableModeAutoCheck(ulonglong param_1)

{
  char cVar1;
  
  if (param_1 < 0x7d1) {
    if (0x4b0 < param_1) {
      if (2 < g_afeTestLevel) {
        g_afeTestErrorInfo = s_IDLEIntervalTime_exceed_6bb5b8e3;
      }
      cVar1 = TSALog_RtEnable(2,0x3e,0x10);
      if (cVar1 != '\0') {
        StrPrintf_Process(0,"\nIDLEIntervalTime(%lld) > 1200 -> failed",param_1);
      }
    }
  }
  else {
    if (1 < g_afeTestLevel) {
      g_afeTestErrorInfo = s_IDLEIntervalTime_exceed_6bb5b8e3;
    }
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\nIDLEIntervalTime(%lld) > 2000 -> failed",param_1);
    }
  }
  if (g_maxIDLEIntervalTime < param_1) {
    g_maxIDLEIntervalTimeStamp = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    g_maxIDLEIntervalTime = param_1;
  }
  if (param_1 < 500) {
    if (1 < g_afeTestLevel) {
      g_afeTestErrorInfo = s_IDLEIntervalTime_TooShort_6bb5b959;
    }
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\nIDLEIntervalTime(%lld) < 500 -> failed",param_1);
    }
  }
  else if (param_1 < 800) {
    if (2 < g_afeTestLevel) {
      g_afeTestErrorInfo = s_IDLEIntervalTime_TooShort_6bb5b959;
    }
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\nIDLEIntervalTime(%lld) < 800 -> failed",param_1);
    }
  }
  if (param_1 < g_minIDLEIntervalTime) {
    g_minIDLEIntervalTimeStamp = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    g_minIDLEIntervalTime = param_1;
  }
  return;
}


// ============================================================================
// Function: AFE_IdleModeAutoCheckLog @ 6bb2b985
// ============================================================================

void AFE_IdleModeAutoCheckLog(void)

{
  char cVar1;
  undefined8 in_stack_ffffffffffffffe8;
  undefined8 uVar2;
  undefined4 uVar3;
  
  uVar3 = (undefined4)((ulonglong)in_stack_ffffffffffffffe8 >> 0x20);
  cVar1 = TSALog_RtEnable(2,0x3e,0x10);
  if (cVar1 != '\0') {
    StrPrintf_Process(0,"\nFreq Shift Auto Check Done!");
    StrPrintf_Process(0,"\nmaxRaw2RawDif: %d, g_minRaw2RawDif: %d",(int)g_maxRaw2RawDif,
                      (int)g_minRaw2RawDif);
    StrPrintf_Process(0,"\nmaxFreqShiftLatency: %lld ",g_maxFreqShiftLatency);
    if ((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 7 < g_maxRaw2RawDif) {
      uVar2 = g_maxRaw2RawDifTimeStamp;
      StrPrintf_Process(0,"\nmaxRaw2RawDif(%d) > %d -> failed at time: %lld",(int)g_maxRaw2RawDif,
                        (int)((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 10),
                        g_maxRaw2RawDifTimeStamp);
      uVar3 = (undefined4)((ulonglong)uVar2 >> 0x20);
    }
    if ((int)((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 7) <
        -(int)g_minRaw2RawDif) {
      uVar2 = g_minRaw2RawDifTimeStamp;
      StrPrintf_Process(0,"\n-g_minRaw2RawDif(%d) > %d -> failed at time: %lld",
                        -(int)g_minRaw2RawDif,
                        (int)((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 10),
                        g_minRaw2RawDifTimeStamp);
      uVar3 = (undefined4)((ulonglong)uVar2 >> 0x20);
    }
    if (0x46 < g_maxFreqShiftLatency) {
      StrPrintf_Process(0,"\nmaxFreqShiftLatency(%lld) > 70 -> failed at time: %lld",
                        g_maxFreqShiftLatency,g_maxFreqShiftLatencyTimeStamp);
    }
    StrPrintf_Process(0,&DAT_6bb5b338);
    StrPrintf_Process(0,"\nIDLE Auto Checking (%dHz, %dKHz, %d, %d)",
                      *(undefined2 *)
                       (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x20) +
                       (ulonglong)g_scanRate * 2),
                      *(undefined2 *)
                       (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x10) +
                       (ulonglong)g_scanFreq * 2),CONCAT44(uVar3,(uint)g_idleCheckLoop),g_idleLoop);
    StrPrintf_Process(0,"\nmaxIDLEEnterTime: %lld at time: %lld",g_maxIDLEEnterTime,
                      g_maxIDLEEnterTimeStamp);
    StrPrintf_Process(0,"\nmaxIDLEExitTime: %lld at time: %lld",g_maxIDLEExitTime,
                      g_maxIDLEExitTimeStamp);
    StrPrintf_Process(0,"\nmaxIDLEIntervalTime: %lld at time: %lld",g_maxIDLEIntervalTime,
                      g_maxIDLEIntervalTimeStamp);
    StrPrintf_Process(0,"\nminIDLEIntervalTime: %lld at time: %lld",g_minIDLEIntervalTime,
                      g_minIDLEIntervalTimeStamp);
    StrPrintf_Process(0,"\nnumIDLEAbnormalExt: %d ",g_numIDLEAbnormalExt);
    StrPrintf_Process(0,&DAT_6bb5b338);
  }
  return;
}


// ============================================================================
// Function: AFE_AutoCheckDoneLog @ 6bb2bc9d
// ============================================================================

void AFE_AutoCheckDoneLog(void)

{
  char cVar1;
  undefined8 in_stack_ffffffffffffffe8;
  undefined4 uVar3;
  undefined8 uVar2;
  
  uVar3 = (undefined4)((ulonglong)in_stack_ffffffffffffffe8 >> 0x20);
  g_afeTestErrorInfo = &DAT_6bb5bbc2;
  cVar1 = TSALog_RtEnable(2,0x3e,0x10);
  if (cVar1 != '\0') {
    StrPrintf_Process(0,"\nFreq Shift Auto Check Done!");
    uVar2 = CONCAT44(uVar3,(uint)g_maxNumRaw2RawOverNoise);
    StrPrintf_Process(0,"\nmaxRaw2RawDif: %d, g_minRaw2RawDif: %d, g_maxNumRaw2RawOverNoise: %d, ",
                      (int)g_maxRaw2RawDif,(int)g_minRaw2RawDif,uVar2);
    StrPrintf_Process(0,"\nmaxFreqShiftLatency: %lld ",g_maxFreqShiftLatency);
    if ((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 10 < g_maxRaw2RawDif) {
      uVar2 = g_maxRaw2RawDifTimeStamp;
      StrPrintf_Process(0,"\nmaxRaw2RawDif(%d) > %d -> failed at time: %lld",(int)g_maxRaw2RawDif,
                        (int)((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 10),
                        g_maxRaw2RawDifTimeStamp);
    }
    if ((int)((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 10) <
        -(int)g_minRaw2RawDif) {
      uVar2 = g_minRaw2RawDifTimeStamp;
      StrPrintf_Process(0,"\n-g_minRaw2RawDif(%d) > %d -> failed at time: %lld",
                        -(int)g_minRaw2RawDif,
                        (int)((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 10),
                        g_minRaw2RawDifTimeStamp);
    }
    if (0x32 < g_maxFreqShiftLatency) {
      StrPrintf_Process(0,"\nmaxFreqShiftLatency(%lld) > 50 -> failed at time: %lld",
                        g_maxFreqShiftLatency,g_maxFreqShiftLatencyTimeStamp,uVar2);
    }
    StrPrintf_Process(0,"\nIDLE Auto Check Done!");
    if (g_maxIDLEEnterTime < 0x33) {
      StrPrintf_Process(0,"\nmaxIDLEEnterTime: %lld at time: %lld",g_maxIDLEEnterTime,
                        g_maxIDLEEnterTimeStamp,uVar2);
    }
    else {
      StrPrintf_Process(0,"\nmaxIDLEEnterTime(%lld) > 50 -> failed at time: %lld",g_maxIDLEEnterTime
                        ,g_maxIDLEEnterTimeStamp,uVar2);
    }
    if (g_maxIDLEExitTime < 0x33) {
      StrPrintf_Process(0,"\nmaxIDLEExitTime: %lld at time: %ld",g_maxIDLEExitTime,
                        g_maxIDLEExitTimeStamp);
    }
    else {
      StrPrintf_Process(0,"\nmaxIDLEExitTime(%lld) > 50 -> failed at time: %lld",g_maxIDLEExitTime,
                        g_maxIDLEExitTimeStamp);
    }
    if (g_maxIDLEIntervalTime < 0x4b1) {
      StrPrintf_Process(0,"\nmaxIDLEIntervalTime: %lld at time: %lld",g_maxIDLEIntervalTime,
                        g_maxIDLEIntervalTimeStamp);
    }
    else {
      StrPrintf_Process(0,"\nmaxIDLEIntervalTime(%lld) > 1200 -> failed at time: %lld",
                        g_maxIDLEIntervalTime,g_maxIDLEIntervalTimeStamp);
    }
    if (g_minIDLEIntervalTime < 800) {
      StrPrintf_Process(0,"\nminIDLEIntervalTime(%lld) < 800 -> failed at time: %lld",
                        g_minIDLEIntervalTime,g_minIDLEIntervalTimeStamp);
    }
    else {
      StrPrintf_Process(0,"\nminIDLEIntervalTime: %lld at time: %lld",g_minIDLEIntervalTime,
                        g_minIDLEIntervalTimeStamp);
    }
    StrPrintf_Process(0,"\nnumIDLEAbnormalExt: %d ",g_numIDLEAbnormalExt);
    StrPrintf_Process(0,&DAT_6bb5b338);
  }
  return;
}


// ============================================================================
// Function: AFE_UpdateCurrScanFreqUnit @ 6bb2dee2
// ============================================================================

void AFE_UpdateCurrScanFreqUnit(void)

{
  bool bVar1;
  char cVar2;
  uint uVar3;
  uint uVar4;
  byte local_9;
  
  bVar1 = false;
  if (*(longlong *)(_refptr_g_thpAFEStruct + 8) != 0) {
    for (local_9 = 0; local_9 < *(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 10);
        local_9 = local_9 + 1) {
      if (*(short *)(*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x10) +
                    (ulonglong)local_9 * 2) ==
          *(short *)(*(longlong *)(_refptr_g_thpAFEStruct + 0x10) + 0x30)) {
        bVar1 = true;
        break;
      }
    }
    if ((bVar1) && (DAT_6bca3981 != local_9)) {
      DAT_6bca3981 = local_9;
      cVar2 = AFE_IsManualFreqHopping();
      if (cVar2 != '\0') {
        if (local_9 < 10) {
          *(void **)(&DAT_6bca3998 + (longlong)(int)(uint)local_9 * 0x18) =
               (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
          *(undefined8 *)(&DAT_6bca39a0 + (longlong)(int)(uint)local_9 * 0x18) = 0;
        }
        else {
          StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\", value:%d\n",
                            "../../src/Extension/TSA_AFE/TSA_AFE_Scan.c",0x164,
                            "index < THP_MAX_SCAN_FREQ_NUMBER_SUPPORTED",local_9);
          HardwareAnalyzer_SetSoftException();
        }
      }
    }
    if (!bVar1) {
      DAT_6bca3981 = 0;
    }
    uVar3 = DAT_6bca3990 & 0xfffffffb;
    if (DAT_6bca3982 == DAT_6bca3981) {
      uVar4 = DAT_6bca3990 & 1;
      DAT_6bca3990 = uVar3;
      if (uVar4 != 0) {
        DAT_6bca3990 = uVar3 | 4;
      }
      if ((DAT_6bca3990 & 2) != 0) {
        DAT_6bca3990 = DAT_6bca3990 | 4;
      }
      uVar3 = DAT_6bca3990 & 0xfffffffe;
    }
    DAT_6bca3990 = uVar3;
    DAT_6bca3990 = DAT_6bca3990 & 0xfffffffd;
  }
  return;
}


// ============================================================================
// Function: AFE_UpdateCurrScanRateUnit @ 6bb2e106
// ============================================================================

void AFE_UpdateCurrScanRateUnit(void)

{
  bool bVar1;
  char cVar2;
  byte local_9;
  
  bVar1 = false;
  if (*(longlong *)(_refptr_g_thpAFEStruct + 8) != 0) {
    for (local_9 = 0; local_9 < *(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x18);
        local_9 = local_9 + 1) {
      if (*(short *)(*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x20) +
                    (ulonglong)local_9 * 2) ==
          *(short *)(*(longlong *)(_refptr_g_thpAFEStruct + 0x10) + 0x32)) {
        bVar1 = true;
        if (DAT_6bca3a89 != local_9) {
          DAT_6bca3a89 = local_9;
        }
        break;
      }
    }
    cVar2 = AFE_IsInIDLEMode();
    if ((cVar2 == '\0') && (!bVar1)) {
      DAT_6bca3a89 = 0;
    }
  }
  return;
}


// ============================================================================
// Function: AFE_NextScanFreqIndexProcess @ 6bb2e67d
// ============================================================================

void AFE_NextScanFreqIndexProcess(void)

{
  char cVar1;
  int iVar2;
  
  if (*(longlong *)(_refptr_g_thpAFEStruct + 8) != 0) {
    if (DAT_6bca3982 == DAT_6bca3981) {
      cVar1 = AFE_IsManualFreqHopping();
      if ((cVar1 != '\0') && (iVar2 = TSA_DetectedNoiseLevel(), iVar2 != 0)) {
        if (*(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 10) < 0xb) {
          AFE_UpdateScanFreq();
        }
        else {
          StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\", value:%d\n",
                            "../../src/Extension/TSA_AFE/TSA_AFE_Scan.c",0x214,
                            "g_thpAFEStruct.caps->num_scan_freq <= THP_MAX_SCAN_FREQ_NUMBER_SUPPORTED"
                            ,*(undefined1 *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 10));
          HardwareAnalyzer_SetSoftException();
        }
      }
    }
    else if ((DAT_6bca3988 != (void *)0x0) &&
            (2000 < (ulonglong)
                    ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - (longlong)DAT_6bca3988))
            ) {
      if (*_refptr_g_tsaLogLoglevel != '\0') {
        StrPrintf_Process(0,"Switching to next frequency time out.\n");
      }
      DAT_6bca3982 = DAT_6bca3981;
      DAT_6bca3988 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    }
  }
  return;
}


// ============================================================================
// Function: AFE_NextScanRateIndexProcess @ 6bb2ec10
// ============================================================================

void AFE_NextScanRateIndexProcess(void)

{
  byte bVar1;
  char cVar2;
  byte local_a;
  byte local_9;
  
  bVar1 = DAT_6bca3a89;
  if ((*(longlong *)(_refptr_g_thpAFEStruct + 8) != 0) &&
     (cVar2 = AFE_IsManualFreqHopping(), cVar2 != '\0')) {
    if (*(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x18) < 0xb) {
      AFE_UpdateScanRate();
      if (1000 < g_tsaAfeFrameNum) {
        g_tsaAfeFrameNum = 0;
        g_tsaAfeHoppingNum = 0;
      }
      local_a = 0;
      for (local_9 = 0; local_9 < *(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x18);
          local_9 = local_9 + 1) {
        if (*(ushort *)
             (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x20) +
             (ulonglong)local_9 * 2) <
            *(ushort *)
             (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x20) +
             (ulonglong)local_a * 2)) {
          local_a = local_9;
        }
      }
      g_tsaAfeIsFrequentlyHopping = 0;
      g_tsaAfeFrameNum = g_tsaAfeFrameNum + 1;
      if (((bVar1 == local_a) &&
          ((*(uint *)(&DAT_6bca3a94 + (longlong)(int)(uint)bVar1 * 0xc) & 1) != 0)) &&
         (DAT_6bca3982 != DAT_6bca3981)) {
        g_tsaAfeHoppingNum = g_tsaAfeHoppingNum + 1;
      }
      if ((int)((uint)*(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 10) * 2 + 1) <=
          g_tsaAfeHoppingNum) {
        g_tsaAfeIsFrequentlyHopping =
             g_tsaAfeFrameNum <
             (int)((uint)*(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 10) * 4);
        g_tsaAfeHoppingNum = 0;
        g_tsaAfeFrameNum = 0;
      }
    }
    else {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\", value:%d\n",
                        "../../src/Extension/TSA_AFE/TSA_AFE_Scan.c",0x25f,
                        "g_thpAFEStruct.caps->num_scan_rate > THP_MAX_SCAN_RATE_NUMBER_SUPPORTED",
                        *(undefined1 *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x18));
      HardwareAnalyzer_SetSoftException();
    }
  }
  return;
}


// ============================================================================
// Function: AFE_IsAutoCalibration @ 6bb2cd33
// ============================================================================

undefined4 AFE_IsAutoCalibration(void)

{
  undefined4 uVar1;
  
                    /* 0xecd33  30  AFE_IsAutoCalibration */
  if (DAT_6bbaa5c8 == 0) {
    uVar1 = 0;
  }
  else {
    uVar1 = CONCAT31((int3)((uint)*(int *)(DAT_6bbaa5c8 + 0x30) >> 8),
                     *(int *)(DAT_6bbaa5c8 + 0x30) == 2);
  }
  return uVar1;
}


// ============================================================================
// Function: AFE_IsRecalRequested @ 6bb2cf82
// ============================================================================

bool AFE_IsRecalRequested(void)

{
  bool bVar1;
  
                    /* 0xecf82  45  AFE_IsRecalRequested */
  if (PTR_AFE_Frame == (AFE_Frame *)0x0) {
    bVar1 = false;
  }
  else {
    bVar1 = (PTR_AFE_Frame->dwFlags & 0x80) != 0;
  }
  return bVar1;
}


// ============================================================================
// Function: AFE_IsCoreTouchDetected @ 6bb2cdf2
// ============================================================================

undefined8 AFE_IsCoreTouchDetected(void)

{
  undefined8 uVar1;
  
                    /* 0xecdf2  31  AFE_IsCoreTouchDetected */
  if (PTR_AFE_Frame == (AFE_Frame *)0x0) {
    uVar1 = 0;
  }
  else if ((PTR_AFE_Frame->dwFlags & 0x100000) == 0) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: AFE_IsFreqShiftDone @ 6bb2cf6f
// ============================================================================

char AFE_IsFreqShiftDone(void)
{
  return 1;
}


// ============================================================================
// Function: AFE_GetTargetScanRateIndex @ 6bb2d733
// ============================================================================

undefined8 AFE_GetTargetScanRateIndex(ushort param_1,byte *param_2)

{
  undefined8 uVar1;
  ushort local_10;
  ushort local_e;
  byte local_b;
  byte local_a;
  byte local_9;
  
                    /* 0xed733  23  AFE_GetTargetScanRateIndex */
  local_a = 0;
  local_b = 0;
  local_e = 0xffff;
  local_10 = 0;
  if (*(longlong *)(_refptr_g_thpAFEStruct + 8) == 0) {
    *param_2 = 0;
    uVar1 = 1;
  }
  else {
    for (local_9 = 0; local_9 < *(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x18);
        local_9 = local_9 + 1) {
      if ((*(ushort *)
            (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x20) +
            (ulonglong)local_9 * 2) <= param_1) &&
         (local_10 <
          *(ushort *)
           (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x20) + (ulonglong)local_9 * 2
           ))) {
        local_10 = *(ushort *)
                    (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x20) +
                    (ulonglong)local_9 * 2);
        local_b = local_9;
      }
      if (*(ushort *)
           (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x20) + (ulonglong)local_9 * 2
           ) < local_e) {
        local_e = *(ushort *)
                   (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x20) +
                   (ulonglong)local_9 * 2);
        local_a = local_9;
      }
    }
    if (local_10 == 0) {
      *param_2 = local_a;
      uVar1 = 1;
    }
    else if (local_10 == param_1) {
      *param_2 = local_b;
      uVar1 = 0;
    }
    else {
      *param_2 = local_b;
      uVar1 = 1;
    }
  }
  return uVar1;
}


// ============================================================================
// Function: AFE_IDLECheckResetUnit @ 6bb29c34
// ============================================================================

void AFE_IDLECheckResetUnit(void)

{
  g_autoCheckToEnterIDLE = 0;
  g_idleLoop = 0;
  g_timeStampToEnterIDLE = 0;
  g_timeStampToExitIDLE = 0;
  return;
}


// ============================================================================
// Function: AFE_FreqShiftTraversedDoneInitFreqRate @ 6bb29d33
// ============================================================================

undefined8 AFE_FreqShiftTraversedDoneInitFreqRate(void)

{
  if (g_fromFreq == -1) {
    g_fromFreq = 0;
    g_fromRate = 0;
  }
  else {
    g_fromFreq = g_fromFreq + 1;
    if (*(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 10) <= g_fromFreq) {
      g_fromRate = g_fromRate + 1;
      if (*(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x18) <= g_fromRate) {
        g_freqLoop = g_freqLoop + 1;
        g_fromRate = 0xff;
        g_fromFreq = 0xff;
        return 0;
      }
      g_fromFreq = 0;
    }
  }
  _refptr_g_thpAFEScanStruct[2] = g_fromFreq;
  _refptr_g_thpAFEScanStruct[0x10a] = g_fromRate;
  g_flagTraversedDone = 0;
  g_toFreq = 0xff;
  g_toRate = 0xff;
  return 1;
}


// ============================================================================
// Function: AFE_FreqShiftSetNextIndex @ 6bb29e17
// ============================================================================

undefined8 AFE_FreqShiftSetNextIndex(void)

{
  g_toFreq = g_toFreq + 1;
  if (*(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 10) <= g_toFreq) {
    g_toRate = g_toRate + 1;
    if (*(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x18) <= g_toRate) {
      g_toRate = 0xff;
      g_toFreq = 0xff;
      g_flagTraversedDone = 1;
      return 0;
    }
    g_toFreq = 0;
  }
  return 1;
}


// ============================================================================
// Function: AFE_FreqShiftAutoCheckAbnornalLog @ 6bb29e99
// ============================================================================

void AFE_FreqShiftAutoCheckAbnornalLog(void)

{
  char cVar1;
  char *pcVar2;
  
  pcVar2 = "TSA_AFE_AUTO_CHECK_FREQ_SHIFT_ABNORMAL";
  StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                    "../../src/Extension/TSA_AFE/TSA_AFE_Check.c",200,
                    "TSA_AFE_AUTO_CHECK_FREQ_SHIFT_ABNORMAL");
  cVar1 = TSALog_RtEnable(2,0x3e,0x10);
  if (cVar1 != '\0') {
    StrPrintf_Process(0,
                      "scanFreq.currIndex/scanRate.currIndex not match with g_fromFreq/g_fromRate, and also not match with g_toFreq/g_toRate\n"
                     );
  }
  cVar1 = TSALog_RtEnable(2,0x3e,0x10);
  if (cVar1 != '\0') {
    StrPrintf_Process(0,
                      "g_thpAFEScanStruct.scanFreq.currIndex:%d g_thpAFEScanStruct.scanRate.currIndex %d\n"
                      ,_refptr_g_thpAFEScanStruct[1],_refptr_g_thpAFEScanStruct[0x109],pcVar2);
  }
  cVar1 = TSALog_RtEnable(2,0x3e,0x10);
  if (cVar1 != '\0') {
    StrPrintf_Process(0,"g_fromFreq:%d g_fromRate: %d\n",g_fromFreq,g_fromRate);
  }
  cVar1 = TSALog_RtEnable(2,0x3e,0x10);
  if (cVar1 != '\0') {
    StrPrintf_Process(0,"g_toFreq:%d g_toRate: %d\n",g_toFreq,g_toRate);
  }
  AFE_LogCaps();
  if (g_afeTestLevel != 0) {
    g_afeTestErrorInfo = s_ShiftToAbnormalFreq_6bb5b1cb;
  }
  return;
}


// ============================================================================
// Function: AFE_IDLEAutoCheckUnit @ 6bb2a22b
// ============================================================================

undefined8 AFE_IDLEAutoCheckUnit(void)

{
  char cVar1;
  
  if (100 < g_idleCheckLoop) {
    g_autoCheckToEnterIDLE = 0;
    return 6;
  }
  if ((g_autoCheckToEnterIDLE == '\0') && (cVar1 = AFE_IsInIDLEMode(), cVar1 == '\0')) {
    if (g_idleLoop < 100) {
      g_idleLoop = g_idleLoop + 1;
    }
    else {
      g_idleLoop = 0;
      g_idleCheckLoop = g_idleCheckLoop + 1;
      if (100 < g_idleCheckLoop) {
        g_autoCheckToEnterIDLE = 0;
        g_idleLoop = 0;
        return 6;
      }
      g_autoCheckToEnterIDLE = '\x01';
      g_timeStampToEnterIDLE = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    }
  }
  else if ((g_autoCheckToEnterIDLE == '\x01') && (cVar1 = AFE_IsInIDLEMode(), cVar1 != '\0')) {
    if (g_idleLoop < 5) {
      g_idleLoop = g_idleLoop + 1;
    }
    else {
      g_idleLoop = 0;
      g_autoCheckToEnterIDLE = '\0';
      g_timeStampToExitIDLE = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    }
  }
  else {
    if ((g_autoCheckToEnterIDLE == '\x01') && (g_idleLoop != 0)) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/TSA_AFE/TSA_AFE_Check.c",0x137,
                        "TSA_AFE_AUTO_IDLE_CHECK_EXIT_ABNORMAL");
      cVar1 = TSALog_RtEnable(2,0x3e,0x10);
      if (cVar1 != '\0') {
        StrPrintf_Process(0,"Abnormal exit idle mode\n");
      }
      if (g_afeTestLevel != 0) {
        g_afeTestErrorInfo = s_AbnormalExitIdleMode_6bb5b2af;
      }
      return 7;
    }
    if (g_autoCheckToEnterIDLE == '\0') {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/TSA_AFE/TSA_AFE_Check.c",0x13f,
                        "TSA_AFE_AUTO_IDLE_CHECK_EXIT_FAILED");
      cVar1 = TSALog_RtEnable(2,0x3e,0x10);
      if (cVar1 != '\0') {
        StrPrintf_Process(0,"Failed to exit idle mode\n");
      }
      if (g_afeTestLevel != 0) {
        g_afeTestErrorInfo = s_FailedExitIdleMode_6bb5b306;
      }
      return 8;
    }
  }
  return 10;
}


// ============================================================================
// Function: AFE_ProximityForceEnter @ 6bb2a5fb
// ============================================================================

void AFE_ProximityForceEnter(void)

{
  *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
       *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) | 0x80;
  AFE_GetTargetScanRateIndex
            (*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x5fe,
             _refptr_g_thpAFEScanStruct + 0x10a);
  g_autoCheckProxState = 1;
  g_autoCheckProxData = 0;
  DAT_6bbaa508 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  return;
}


// ============================================================================
// Function: AFE_ProximityWaitUntilEnter @ 6bb2a675
// ============================================================================

void AFE_ProximityWaitUntilEnter(void)

{
  *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
       *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) | 0x80;
  if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x1000000) != 0) {
    g_autoCheckProxState = 2;
    DAT_6bbaa510 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    DAT_6bbaa518 = (longlong)DAT_6bbaa510 - DAT_6bbaa508;
  }
  return;
}


// ============================================================================
// Function: AFE_ProximityWaitUntilAfterEnter @ 6bb2a6ee
// ============================================================================

void AFE_ProximityWaitUntilAfterEnter(void)

{
  g_autoCheckProxState = 3;
  DAT_6bbaa520 = (longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
                 (*_refptr_g_tsaStaticPtr)->llField_0xe8;
  DAT_6bbaa518 = DAT_6bbaa520 + DAT_6bbaa518;
  if (DAT_6bbaa528 < DAT_6bbaa518) {
    DAT_6bbaa538 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    DAT_6bbaa528 = DAT_6bbaa518;
    DAT_6bbaa530 = DAT_6bbaa520;
  }
  return;
}


// ============================================================================
// Function: AFE_ProximityKeepInEnter @ 6bb2a78c
// ============================================================================

void AFE_ProximityKeepInEnter(void)

{
  *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
       *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) | 0x80;
  if (g_autoCheckProxData < 10) {
    g_autoCheckProxData = g_autoCheckProxData + 1;
  }
  else {
    g_autoCheckProxState = 4;
  }
  return;
}


// ============================================================================
// Function: AFE_ProximityForceExit @ 6bb2a7d6
// ============================================================================

void AFE_ProximityForceExit(void)

{
  *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
       *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) & 0xffffff7f;
  AFE_GetTargetScanRateIndex
            (*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x5fc,
             _refptr_g_thpAFEScanStruct + 0x10a);
  g_autoCheckProxState = 5;
  DAT_6bbaa501 = 0;
  DAT_6bbaa540 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  return;
}


// ============================================================================
// Function: AFE_ProximityWaitUntilExit @ 6bb2a850
// ============================================================================

void AFE_ProximityWaitUntilExit(void)

{
  *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
       *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) & 0xffffff7f;
  if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x2000000) != 0) {
    g_autoCheckProxState = 6;
    DAT_6bbaa548 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    DAT_6bbaa550 = (longlong)DAT_6bbaa548 - DAT_6bbaa540;
    DAT_6bbaa558 = (longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
                   (*_refptr_g_tsaStaticPtr)->llField_0xe8;
    if (DAT_6bbaa560 < DAT_6bbaa550) {
      DAT_6bbaa570 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
      DAT_6bbaa560 = DAT_6bbaa550;
      DAT_6bbaa568 = DAT_6bbaa558;
    }
  }
  return;
}


// ============================================================================
// Function: AFE_ProximityKeepInExit @ 6bb2a943
// ============================================================================

void AFE_ProximityKeepInExit(void)

{
  *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
       *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) & 0xffffff7f;
  if (DAT_6bbaa501 < 10) {
    DAT_6bbaa501 = DAT_6bbaa501 + 1;
  }
  else {
    if (DAT_6bbaa501 < 10) {
      DAT_6bbaa501 = DAT_6bbaa501 + 1;
    }
    else if (DAT_6bbaa502 < 100) {
      DAT_6bbaa502 = DAT_6bbaa502 + 1;
    }
    g_autoCheckProxState = 0;
  }
  return;
}


// ============================================================================
// Function: AFE_UpdateScanFreq @ 6bb2e2f4
// ============================================================================

void AFE_UpdateScanFreq(void)

{
  char cVar1;
  short sVar2;
  short local_c;
  byte local_a;
  byte local_9;
  
  local_c = 0x7fff;
  sVar2 = Exception_GetDetectedChargerNoise();
  if (*(short *)(&DAT_6bca39a8 + (longlong)(int)(uint)DAT_6bca3981 * 0x18) < sVar2) {
    *(short *)(&DAT_6bca39a8 + (longlong)(int)(uint)DAT_6bca3981 * 0x18) = sVar2;
  }
  if (*(longlong *)(&DAT_6bca39a0 + (longlong)(int)(uint)DAT_6bca3981 * 0x18) == 0) {
    *(void **)(&DAT_6bca39a0 + (longlong)(int)(uint)DAT_6bca3981 * 0x18) =
         (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  }
  local_9 = 0;
  DAT_6bca3982 = 0xff;
  local_a = DAT_6bca3981;
  do {
    local_a = local_a + 1;
    if ((uint)*(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 10) + (uint)DAT_6bca3981 <=
        (uint)local_a) goto LAB_6bb2e47e;
    local_9 = local_a % *(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 10);
    cVar1 = AFE_IsFreqValid(local_9);
  } while (cVar1 == '\0');
  DAT_6bca3982 = local_9;
LAB_6bb2e47e:
  if (DAT_6bca3982 == 0xff) {
    for (local_a = 0; local_a < *(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 10);
        local_a = local_a + 1) {
      if (*(short *)(&DAT_6bca39a8 + (longlong)(int)(uint)local_a * 0x18) < local_c) {
        local_c = *(short *)(&DAT_6bca39a8 + (longlong)(int)(uint)local_a * 0x18);
        local_9 = local_a;
      }
    }
    if (((short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax < local_c) &&
       (cVar1 = TSAStatic_IsChargerPlugged(), cVar1 != '\0')) {
      *(uint *)(&DAT_6bca3a94 + (longlong)(int)(uint)DAT_6bca3a89 * 0xc) =
           *(uint *)(&DAT_6bca3a94 + (longlong)(int)(uint)DAT_6bca3a89 * 0xc) | 1;
    }
    if ((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 3 < sVar2) {
      DAT_6bca3982 = (byte)((ulonglong)(DAT_6bca3981 + 1) %
                           (ulonglong)
                           (longlong)
                           (int)(uint)*(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 10));
    }
    else {
      DAT_6bca3982 = local_9;
    }
  }
  if (DAT_6bca3982 == DAT_6bca3981) {
    DAT_6bca3990 = DAT_6bca3990 | 2;
  }
  else {
    DAT_6bca3990 = DAT_6bca3990 | 1;
    DAT_6bca3988 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  }
  return;
}


// ============================================================================
// Function: AFE_UpdateScanRate @ 6bb2e7c2
// ============================================================================

void AFE_UpdateScanRate(void)

{
  undefined1 uVar1;
  byte bVar2;
  int iVar3;
  uint uVar4;
  ushort local_e;
  ushort local_c;
  byte local_a;
  byte local_9;
  
  bVar2 = DAT_6bca3a89;
  local_c = 0;
  local_e = 0xffff;
  local_9 = DAT_6bca3a89;
  if ((*(uint *)(&DAT_6bca3a94 + (longlong)(int)(uint)DAT_6bca3a89 * 0xc) & 1) != 0) {
    for (local_a = 0; local_a < *(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x18);
        local_a = local_a + 1) {
      if (((*(ushort *)
             (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x20) +
             (ulonglong)local_a * 2) <
            *(ushort *)
             (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x20) +
             (ulonglong)DAT_6bca3a89 * 2)) &&
          (*(ushort *)
            (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x20) +
            (ulonglong)local_a * 2) <= DAT_6bca3a8c)) &&
         (local_c < *(ushort *)
                     (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x20) +
                     (ulonglong)local_a * 2))) {
        local_c = *(ushort *)
                   (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x20) +
                   (ulonglong)local_a * 2);
        local_9 = local_a;
      }
    }
  }
  iVar3 = TSA_DetectedNoiseLevel();
  if (iVar3 == 0) {
    if (**(char **)_refptr_g_touchesPtr != '\0') {
      *(short *)(&DAT_6bca3a98 + (longlong)(int)(uint)bVar2 * 0xc) =
           *(short *)(&DAT_6bca3a98 + (longlong)(int)(uint)bVar2 * 0xc) + 1;
    }
  }
  else {
    *(undefined2 *)(&DAT_6bca3a98 + (longlong)(int)(uint)bVar2 * 0xc) = 0;
  }
  if ((*(ushort *)(&DAT_6bca3a98 + (longlong)(int)(uint)bVar2 * 0xc) < 0xfb) ||
     (*(ushort *)
       (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x20) + (ulonglong)bVar2 * 2) !=
      DAT_6bca3a8c)) {
    if (0xfa < *(ushort *)(&DAT_6bca3a98 + (longlong)(int)(uint)bVar2 * 0xc)) {
      *(undefined2 *)(&DAT_6bca3a98 + (longlong)(int)(uint)bVar2 * 0xc) = 0;
      for (local_a = 0; local_a < *(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x18);
          local_a = local_a + 1) {
        if (((*(ushort *)
               (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x20) +
               (ulonglong)bVar2 * 2) <
              *(ushort *)
               (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x20) +
               (ulonglong)local_a * 2)) &&
            (*(ushort *)
              (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x20) +
              (ulonglong)local_a * 2) <= DAT_6bca3a8c)) &&
           (*(ushort *)
             (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x20) +
             (ulonglong)local_a * 2) < local_e)) {
          local_e = *(ushort *)
                     (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x20) +
                     (ulonglong)local_a * 2);
          local_9 = local_a;
        }
      }
    }
  }
  else {
    *(short *)(&DAT_6bca3a98 + (longlong)(int)(uint)bVar2 * 0xc) =
         *(short *)(&DAT_6bca3a98 + (longlong)(int)(uint)bVar2 * 0xc) + -1;
  }
  uVar1 = DAT_6bca3982;
  DAT_6bca3a8a = local_9;
  if (local_9 != bVar2) {
    uVar4 = DAT_6bca3990 | 1;
    DAT_6bca3990 = uVar4;
    AFE_ScanFreqReset();
    DAT_6bca3982 = uVar1;
    DAT_6bca3990 = uVar4;
    AFE_ScanRateResetUnit(local_9);
  }
  return;
}


// ============================================================================
// Function: AFE_GetMaxNoise @ 6bb2d506
// ============================================================================

undefined2 AFE_GetMaxNoise(byte param_1)

{
  undefined2 uVar1;
  byte local_res8;
  
                    /* 0xed506  11  AFE_GetMaxNoise */
  if (*(longlong *)(_refptr_g_thpAFEStruct + 8) == 0) {
    uVar1 = 0;
  }
  else {
    local_res8 = param_1;
    if (9 < param_1) {
      local_res8 = 0;
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\", value:%d\n",
                        "../../src/Extension/TSA_AFE/TSA_AFE_Scan.c",0x74,
                        "freqIndex < THP_MAX_SCAN_FREQ_NUMBER_SUPPORTED",0);
      HardwareAnalyzer_SetSoftException();
    }
    uVar1 = *(undefined2 *)(&DAT_6bca39a8 + (longlong)(int)(uint)local_res8 * 0x18);
  }
  return uVar1;
}


// ============================================================================
// Function: AFE_IsAlwaysNoisy @ 6bb2d087
// ============================================================================

bool AFE_IsAlwaysNoisy(void)

{
  int iVar1;
  char *pcVar2;
  
                    /* 0xed087  29  AFE_IsAlwaysNoisy */
  pcVar2 = (char *)TSA_GetPrmtName();
  iVar1 = strcmp(pcVar2,"ALPS491000");
  if (iVar1 != 0) {
    pcVar2 = (char *)TSA_GetPrmtName();
    iVar1 = strcmp(pcVar2,"ALPS491600");
    if (iVar1 != 0) {
      return false;
    }
  }
  return (*_refptr_g_tsaStaticPtr)->wField_0x120 != 0;
}


// ============================================================================
// Function: AFE_ScanIsFreqShiftDone @ 6bb2ee46
// ============================================================================

bool AFE_ScanIsFreqShiftDone(void)

{
                    /* 0xeee46  54  AFE_ScanIsFreqShiftDone */
  return (DAT_6bca3990 & 4) != 0;
}


// ============================================================================
// Function: AFE_IsRawJump @ 6bb2cebb
// ============================================================================

undefined4 AFE_IsRawJump(void)

{
  uint uVar1;
  undefined4 uVar2;
  
                    /* 0xecebb  42  AFE_IsRawJump */
  if (PTR_AFE_Frame == (AFE_Frame *)0x0) {
    uVar2 = 0;
  }
  else {
    uVar1 = PTR_AFE_Frame->dwFlags & 0x20000;
    uVar2 = CONCAT31((int3)(uVar1 >> 8),uVar1 != 0);
  }
  return uVar2;
}


// ============================================================================
// Function: AFE_IsRawUnstable @ 6bb2cee8
// ============================================================================

undefined4 AFE_IsRawUnstable(void)

{
  uint uVar1;
  undefined4 uVar2;
  
                    /* 0xecee8  44  AFE_IsRawUnstable */
  if (PTR_AFE_Frame == (AFE_Frame *)0x0) {
    uVar2 = 0;
  }
  else {
    uVar1 = PTR_AFE_Frame->dwFlags & 0x40000;
    uVar2 = CONCAT31((int3)(uVar1 >> 8),uVar1 != 0);
  }
  return uVar2;
}


// ============================================================================
// Function: AFE_SetMaxScanRate @ 6bb2d3ca
// ============================================================================

void AFE_SetMaxScanRate(undefined2 param_1)

{
                    /* 0xed3ca  65  AFE_SetMaxScanRate */
  DAT_6bca3a8c = param_1;
  return;
}


// ============================================================================
// Function: AFE_LogCaps @ 6bb2db8a
// ============================================================================

void AFE_LogCaps(void)

{
  char cVar1;
  byte local_b;
  byte local_a;
  byte local_9;
  
                    /* 0xedb8a  49  AFE_LogCaps */
  if (*(longlong *)(_refptr_g_thpAFEStruct + 8) == 0) {
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"g_thpAFEStruct.caps is Null\n");
    }
  }
  else {
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"g_thpAFEStruct.caps->num_scan_freq = %d\n",
                        *(undefined1 *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 10));
    }
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"g_thpAFEStruct.caps->scan_freq = [");
    }
    for (local_9 = 0; local_9 < *(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 10);
        local_9 = local_9 + 1) {
      cVar1 = TSALog_RtEnable(2,0x3e,0x10);
      if (cVar1 != '\0') {
        StrPrintf_Process(0,&DAT_6bb5c013,
                          *(undefined2 *)
                           (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x10) +
                           (ulonglong)local_9 * 2));
      }
    }
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,&DAT_6bb5c017);
    }
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"g_thpAFEStruct.caps->num_scan_rate = %d\n",
                        *(undefined1 *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x18));
    }
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"g_thpAFEStruct.caps->scan_rate = [");
    }
    for (local_a = 0; local_a < *(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x18);
        local_a = local_a + 1) {
      cVar1 = TSALog_RtEnable(2,0x3e,0x10);
      if (cVar1 != '\0') {
        StrPrintf_Process(0,&DAT_6bb5c013,
                          *(undefined2 *)
                           (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x20) +
                           (ulonglong)local_a * 2));
      }
    }
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,&DAT_6bb5c017);
    }
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"g_thpAFEStruct.caps->stylus_scan_freq_num = %d\n",
                        *(undefined1 *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x44));
    }
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"g_thpAFEStruct.caps->stylus_scan_freq = [");
    }
    for (local_b = 0; local_b < *(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x44);
        local_b = local_b + 1) {
      cVar1 = TSALog_RtEnable(2,0x3e,0x10);
      if (cVar1 != '\0') {
        StrPrintf_Process(0,&DAT_6bb5c013,
                          *(undefined2 *)
                           (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x48) +
                           (ulonglong)local_b * 2));
      }
    }
    cVar1 = TSALog_RtEnable(2,0x3e,0x10);
    if (cVar1 != '\0') {
      StrPrintf_Process(0,&DAT_6bb5c017);
    }
  }
  return;
}


// ============================================================================
// Function: AFE_IsFreqValid @ 6bb2e1ca
// ============================================================================

undefined8 AFE_IsFreqValid(byte param_1)

{
  undefined8 uVar1;
  
  if ((*(longlong *)(&DAT_6bca39a0 + (longlong)(int)(uint)param_1 * 0x18) == 0) ||
     ((ulonglong)*(ushort *)(_refptr_g_tsaPrmtDynamic + 0x8e) <
      (ulonglong)
      (*(longlong *)(&DAT_6bca39a0 + (longlong)(int)(uint)param_1 * 0x18) -
      *(longlong *)(&DAT_6bca3998 + (longlong)(int)(uint)param_1 * 0x18)))) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: AFE_ScanFreqReset @ 6bb2d1e0
// ============================================================================

void AFE_ScanFreqReset(void)

{
  byte local_9;
  
  for (local_9 = 0; local_9 < 10; local_9 = local_9 + 1) {
    *(undefined2 *)(&DAT_6bca39a8 + (longlong)(int)(uint)local_9 * 0x18) = 0;
    *(undefined8 *)(&DAT_6bca3998 + (longlong)(int)(uint)local_9 * 0x18) = 0;
    *(undefined8 *)(&DAT_6bca39a0 + (longlong)(int)(uint)local_9 * 0x18) = 0;
  }
  return;
}


// ============================================================================
// Function: AFE_ScanRateResetUnit @ 6bb2d2ea
// ============================================================================

void AFE_ScanRateResetUnit(byte param_1)

{
  *(undefined2 *)(&DAT_6bca3a90 + (longlong)(int)(uint)param_1 * 0xc) = 0;
  *(undefined4 *)(&DAT_6bca3a94 + (longlong)(int)(uint)param_1 * 0xc) = 0;
  *(undefined2 *)(&DAT_6bca3a98 + (longlong)(int)(uint)param_1 * 0xc) = 0;
  return;
}


// ============================================================================
// Function: AFE_GetNextScanFreqIndex @ 6bb2d650
// ============================================================================

undefined8 AFE_GetNextScanFreqIndex(byte *param_1)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0xed650  12  AFE_GetNextScanFreqIndex */
  *param_1 = 0;
  if (*(longlong *)(_refptr_g_thpAFEStruct + 8) == 0) {
    *param_1 = DAT_6bca3981;
    uVar2 = 1;
  }
  else {
    cVar1 = AFE_IsFreqHopSupported();
    if (cVar1 == '\0') {
      *param_1 = DAT_6bca3981;
      uVar2 = 1;
    }
    else if (DAT_6bca3982 < *(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 10)) {
      if (DAT_6bca3982 == DAT_6bca3981) {
        *param_1 = DAT_6bca3982;
        uVar2 = 1;
      }
      else {
        *param_1 = DAT_6bca3982;
        uVar2 = 0;
      }
    }
    else {
      *param_1 = DAT_6bca3981;
      uVar2 = 1;
    }
  }
  return uVar2;
}


// ============================================================================
// Function: AFE_IsRawUnifiedToTargetValue @ 6bb2d150
// ============================================================================

bool AFE_IsRawUnifiedToTargetValue(void)

{
  int iVar1;
  char *pcVar2;
  
                    /* 0xed150  43  AFE_IsRawUnifiedToTargetValue */
  pcVar2 = (char *)TSA_GetPrmtName();
  iVar1 = strcmp(pcVar2,"ALPS491000");
  if (iVar1 != 0) {
    pcVar2 = (char *)TSA_GetPrmtName();
    iVar1 = strcmp(pcVar2,"ALPS491600");
    if (iVar1 != 0) {
      return false;
    }
  }
  pcVar2 = (char *)AFE_GetVersionInfo();
  iVar1 = strcmp(pcVar2,"ALPS491600_P5007_T91");
  return iVar1 != 0;
}


// ============================================================================
// Function: AFE_IsTouchDetected @ 6bb2ce62
// ============================================================================

undefined8 AFE_IsTouchDetected(void)

{
  char cVar1;
  
                    /* 0xece62  47  AFE_IsTouchDetected */
  cVar1 = AFE_IsCoreTouchDetected();
  if ((cVar1 == '\0') && (cVar1 = AFE_IsSideTouchDetected(), cVar1 == '\0')) {
    return 0;
  }
  return 1;
}


// ============================================================================
// Function: AFE_IsFreqHopSupported @ 6bb2d61a
// ============================================================================

undefined8 AFE_IsFreqHopSupported(void)

{
  char cVar1;
  
  if ((*(int *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x2c) != 1) &&
     (cVar1 = AFE_IsInAutoCheckProcess(), cVar1 == '\0')) {
    return 0;
  }
  return 1;
}


// ============================================================================
// Function: AFE_GetVersionInfo @ 6bb2cd7e
// ============================================================================

char * AFE_GetVersionInfo(void)

{
  char *pcVar1;
  
                    /* 0xecd7e  26  AFE_GetVersionInfo */
  if (g_thpAFEStruct == 0) {
    pcVar1 = "UnKnown";
  }
  else {
    pcVar1 = (char *)(g_thpAFEStruct + 0x40);
  }
  return pcVar1;
}


// ============================================================================
// Function: AFE_IsFrequentlyHopping @ 6bb2eede
// ============================================================================

undefined1 AFE_IsFrequentlyHopping(void)

{
                    /* 0xeeede  34  AFE_IsFrequentlyHopping */
  return g_tsaAfeIsFrequentlyHopping;
}


// ============================================================================
// Function: AFE_IsSideTouchDetected @ 6bb2ce2a
// ============================================================================

undefined8 AFE_IsSideTouchDetected(void)

{
  undefined8 uVar1;
  
                    /* 0xece2a  46  AFE_IsSideTouchDetected */
  if (PTR_AFE_Frame == (AFE_Frame *)0x0) {
    uVar1 = 0;
  }
  else if ((PTR_AFE_Frame->dwFlags & 0x80000) == 0) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: AFE_IsIndividualBaselineForEachFreq @ 6bb2d0e7
// ============================================================================

bool AFE_IsIndividualBaselineForEachFreq(void)

{
  int iVar1;
  char *pcVar2;
  
                    /* 0xed0e7  39  AFE_IsIndividualBaselineForEachFreq */
  pcVar2 = (char *)TSA_GetPrmtName();
  iVar1 = strcmp(pcVar2,"ALPS491000");
  if (iVar1 != 0) {
    pcVar2 = (char *)TSA_GetPrmtName();
    iVar1 = strcmp(pcVar2,"ALPS491600");
    if (iVar1 != 0) {
      return false;
    }
  }
  pcVar2 = (char *)AFE_GetVersionInfo();
  iVar1 = strcmp(pcVar2,"ALPS491600_P5007_T91");
  return iVar1 != 0;
}


// ============================================================================
// Function: AFE_Init @ 6bb2ca4f
// ============================================================================

void AFE_Init(void)

{
  int iVar1;
  
                    /* 0xeca4f  27  AFE_Init */
  g_thpAFEStruct = 0;
  iVar1 = memset_s(&g_thpInternalAFEFrame,0x60,0,0x60);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/TSA_AFE/TSA_AFE_Process.c",0xae,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  PTR_AFE_Frame = &g_thpInternalAFEFrame;
  iVar1 = memset_s(&g_thpInternalCaps,0x60,0,0x60);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/TSA_AFE/TSA_AFE_Process.c",0xb2,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  DAT_6bbaa5c8 = &g_thpInternalCaps;
  AFE_Simulation();
  AFE_ScanInit();
  AFE_AutoCheckInit();
  return;
}


// ============================================================================
// Function: AFE_ScanInit @ 6bb2d3e8
// ============================================================================

void AFE_ScanInit(void)

{
  int iVar1;
  char *_Str1;
  
                    /* 0xed3e8  53  AFE_ScanInit */
  iVar1 = memset_s(&g_thpAFEScanStruct,0x108,0,0x108);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/TSA_AFE/TSA_AFE_Scan.c",0x5f,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(&DAT_6bca3a88,0x80,0,0x80);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/TSA_AFE/TSA_AFE_Scan.c",0x61,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  g_thpAFEScanStruct = 0;
  DAT_6bca3a88 = 0;
  DAT_6bca3a8c = *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x61e;
  _Str1 = (char *)TSA_GetPrmtName();
  iVar1 = strcmp(_Str1,"ALPS470800");
  if (iVar1 == 0) {
    g_thpAFEScanStruct = 3;
  }
  AFE_ScanReset();
  return;
}


// ============================================================================
// Function: AFE_ScanReset @ 6bb2d3a3
// ============================================================================

void AFE_ScanReset(void)

{
  AFE_ScanFreqReset();
  AFE_ScanRateReset();
  DAT_6bca3988 = 0;
  return;
}


// ============================================================================
// Function: AFE_Simulation @ 6bb2c8aa
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void AFE_Simulation(void)

{
  int iVar1;
  char *pcVar2;
  
  pcVar2 = (char *)TSA_GetPrmtName();
  iVar1 = strcmp(pcVar2,"ALPS491000");
  if (iVar1 != 0) {
    pcVar2 = (char *)TSA_GetPrmtName();
    iVar1 = strcmp(pcVar2,"ALPS491600");
    if (iVar1 != 0) {
      pcVar2 = (char *)TSA_GetPrmtName();
      iVar1 = strncmp(pcVar2,"JHSN",4);
      if (iVar1 == 0) {
        AFE_SimulationForJHSNProj();
        g_thpAFEStruct = &g_simuAFEInfo;
        DAT_6bbaa5c8 = &g_simuAFECaps;
        PTR_AFE_Frame = &g_simuAFEFrame;
        _DAT_6bbaa7b0 = 1;
        return;
      }
      AFE_SimulationForDefaultProj();
      g_thpAFEStruct = &g_simuAFEInfo;
      DAT_6bbaa5c8 = &g_simuAFECaps;
      PTR_AFE_Frame = &g_simuAFEFrame;
      _DAT_6bbaa7b0 = 1;
      return;
    }
  }
  AFE_SimulationForALPSProj();
  _DAT_6bbaa7b0 = 1;
  DAT_6bbaa5c8 = &g_simuAFECaps;
  g_thpAFEStruct = &g_simuAFEInfo;
  PTR_AFE_Frame = &g_simuAFEFrame;
  return;
}


// ============================================================================
// Function: AFE_GetColNum @ 6bb2d5c3
// ============================================================================

undefined2 AFE_GetColNum(void)

{
  undefined2 uVar1;
  
                    /* 0xed5c3  8  AFE_GetColNum */
  if (*(longlong *)(_refptr_g_thpAFEStruct + 8) == 0) {
    uVar1 = 10;
  }
  else {
    uVar1 = **(undefined2 **)(_refptr_g_thpAFEStruct + 8);
  }
  return uVar1;
}


// ============================================================================
// Function: AFE_GetRowNum @ 6bb2d597
// ============================================================================

undefined2 AFE_GetRowNum(void)

{
  undefined2 uVar1;
  
                    /* 0xed597  14  AFE_GetRowNum */
  if (*(longlong *)(_refptr_g_thpAFEStruct + 8) == 0) {
    uVar1 = 10;
  }
  else {
    uVar1 = *(undefined2 *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 2);
  }
  return uVar1;
}


// ============================================================================
// Function: AFE_SetFrame @ 6bb2cb9d
// ============================================================================

void AFE_SetFrame(undefined8 param_1)

{
  errno_t eVar1;
  
                    /* 0xecb9d  62  AFE_SetFrame */
  if (PTR_AFE_Frame == (AFE_Frame *)0x0) {
    PTR_AFE_Frame = &g_thpInternalAFEFrame;
  }
  AFE_SetFrameTrans(param_1,PTR_AFE_Frame,g_afeHalSpec);
  if (PTR_AFE_Frame->pRawData != (void *)0x0) {
    eVar1 = memcpy_s(g_afeGridData,(ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,
                     PTR_AFE_Frame->pRawData,(ulonglong)*(ushort *)_refptr_g_gridBufSize * 2);
    if (eVar1 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/TSA_AFE/TSA_AFE_Process.c",0xe5,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
    PTR_AFE_Frame->pRawData = g_afeGridData;
  }
  if (PTR_AFE_Frame->pLineData != (void *)0x0) {
    eVar1 = memcpy_s(g_afeLineData,(ulonglong)*(ushort *)_refptr_g_lineBufSize * 2,
                     PTR_AFE_Frame->pLineData,(ulonglong)*(ushort *)_refptr_g_lineBufSize * 2);
    if (eVar1 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/TSA_AFE/TSA_AFE_Process.c",0xec,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
    PTR_AFE_Frame->pLineData = g_afeLineData;
  }
  return;
}


// ============================================================================
// Function: AFE_SetCaps @ 6bb2cb53
// ============================================================================

void AFE_SetCaps(undefined8 param_1)

{
                    /* 0xecb53  60  AFE_SetCaps */
  if (DAT_6bbaa5c8 == (undefined *)0x0) {
    DAT_6bbaa5c8 = &g_thpInternalCaps;
  }
  AFE_SetCapsTrans(param_1,DAT_6bbaa5c8,g_afeHalSpec);
  return;
}


// ============================================================================
// Function: AFE_SetInfo @ 6bb2cb3e
// ============================================================================

void AFE_SetInfo(undefined8 param_1)

{
                    /* 0xecb3e  64  AFE_SetInfo */
  g_thpAFEStruct = param_1;
  return;
}


// ============================================================================
// Function: AFE_SetAFESpecVersion @ 6bb2c94d
// ============================================================================

undefined8 AFE_SetAFESpecVersion(byte param_1,byte param_2,byte param_3)

{
                    /* 0xec94d  59  AFE_SetAFESpecVersion */
  g_afeHalSpec = (longlong)(int)((uint)param_3 + ((uint)param_2 + (uint)param_1 * 0x100) * 0x100);
  return 0;
}


// ============================================================================
// Function: AFE_Malloc @ 6bb2c9b5
// ============================================================================

undefined8 AFE_Malloc(void)

{
  undefined8 uVar1;
  
                    /* 0xec9b5  50  AFE_Malloc */
  g_afeGridData = TSA_MallocAndInit((uint)*(ushort *)_refptr_g_gridBufSize * 2);
  if (g_afeGridData == 0) {
    uVar1 = 0;
  }
  else {
    g_afeLineData = TSA_MallocAndInit((uint)*(ushort *)_refptr_g_lineBufSize * 2);
    if (g_afeLineData == 0) {
      uVar1 = 0;
    }
    else {
      uVar1 = 1;
    }
  }
  return uVar1;
}


// ============================================================================
// Function: AFE_ScanRateReset @ 6bb2d374
// ============================================================================

void AFE_ScanRateReset(void)

{
  byte local_9;
  
  for (local_9 = 0; local_9 < 10; local_9 = local_9 + 1) {
    AFE_ScanRateResetUnit(local_9);
  }
  return;
}


// ============================================================================
// Function: AFE_SimulationForALPSProj @ 6bb2c660
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void AFE_SimulationForALPSProj(void)

{
  _DAT_6bbaa7ac = 2;
  DAT_6bbaa78a = 4;
  _g_simuScanFreq = 0x62;
  _DAT_6bbaa862 = 0xb9;
  _DAT_6bbaa864 = 0x8e;
  _DAT_6bbaa866 = 0x36;
  _DAT_6bbaa790 = &g_simuScanFreq;
  DAT_6bbaa798 = 2;
  _g_simuScanRate = 0x78;
  _DAT_6bbaa882 = 0x3c;
  _DAT_6bbaa7a0 = &g_simuScanRate;
  _DAT_6bbaa7b0 = 2;
  g_simuAFEFrame.wScanFreqIndex = 0x62;
  g_simuAFEFrame.wScanRateIndex = 0x78;
  strcpy_s((char*)&g_simuAFEInfo,0x20,"Novatek");
  return;
}


// ============================================================================
// Function: AFE_SimulationForDefaultProj @ 6bb2c7d5
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void AFE_SimulationForDefaultProj(void)

{
  _DAT_6bbaa7ac = (uint)*(uint8_t*)((char*)_refptr_g_tsaPrmtFlash + 2 * 0x840 + 0x61c);
  DAT_6bbaa78a = 3;
  _g_simuScanFreq = 0xc3;
  _DAT_6bbaa862 = 0xc3;
  _DAT_6bbaa864 = 0xb0;
  _DAT_6bbaa790 = &g_simuScanFreq;
  DAT_6bbaa798 = 6;
  _g_simuScanRate = 0x78;
  _DAT_6bbaa882 = 0xb4;
  _DAT_6bbaa884 = 0x3c;
  _DAT_6bbaa886 = 0x46;
  _DAT_6bbaa888 = 0x41;
  _DAT_6bbaa88a = 0x5a;
  _DAT_6bbaa7a0 = &g_simuScanRate;
  _DAT_6bbaa7b0 = 2;
  g_simuAFEFrame.wScanFreqIndex = 0xc3;
  g_simuAFEFrame.wScanRateIndex = 0x78;
  strcpy_s((char*)&g_simuAFEInfo,0x20,"S");
  return;
}


// ============================================================================
// Function: AFE_SimulationForJHSNProj @ 6bb2c70d
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void AFE_SimulationForJHSNProj(void)

{
  _DAT_6bbaa7ac = 2;
  DAT_6bbaa78a = 4;
  _g_simuScanFreq = 0x60;
  _DAT_6bbaa862 = 0x53;
  _DAT_6bbaa864 = 0x52;
  _DAT_6bbaa866 = 0x65;
  _DAT_6bbaa790 = &g_simuScanFreq;
  DAT_6bbaa798 = 5;
  _g_simuScanRate = 0x78;
  _DAT_6bbaa882 = 0x3c;
  _DAT_6bbaa884 = 0x28;
  _DAT_6bbaa886 = 0x1e;
  _DAT_6bbaa888 = 0xf;
  _DAT_6bbaa7a0 = &g_simuScanRate;
  _DAT_6bbaa7b0 = 1;
  g_simuAFEFrame.wScanFreqIndex = 0x60;
  g_simuAFEFrame.wScanRateIndex = 0x78;
  strcpy_s((char*)&g_simuAFEInfo,0x20,"N");
  return;
}


// ============================================================================
// Function: AFE_SetFrameTrans @ 6bb2fc4b
// ============================================================================

void AFE_SetFrameTrans(undefined8 param_1,undefined8 param_2,ulonglong param_3)

{
                    /* 0xefc4b  63  AFE_SetFrameTrans */
  if (param_3 < 0x11300) {
    if (param_3 < 0x10300) {
      if (param_3 < 0x11e) {
        AFE_SetFrameByV0113(param_1,param_2);
      }
      else {
        AFE_SetFrameByV0130(param_1,param_2);
      }
    }
    else {
      AFE_SetFrameByV130(param_1,param_2);
    }
  }
  else {
    AFE_SetFrameNormal(param_1,param_2,param_3);
  }
  return;
}


// ============================================================================
// Function: AFE_SetCapsTrans @ 6bb2fbb0
// ============================================================================

void AFE_SetCapsTrans(undefined8 param_1,undefined8 param_2,ulonglong param_3)

{
                    /* 0xefbb0  61  AFE_SetCapsTrans */
  if (param_3 < 0x11300) {
    if (param_3 < 0x10900) {
      if (param_3 < 0x10300) {
        if (param_3 < 0x10000) {
          AFE_SetCapsByV0113(param_1,param_2);
        }
        else {
          AFE_SetCapsByV100(param_1,param_2);
        }
      }
      else {
        AFE_SetCapsByV130(param_1,param_2);
      }
    }
    else {
      AFE_SetCapsByV190(param_1,param_2);
    }
  }
  else {
    AFE_SetCapsNormal(param_1,param_2);
  }
  return;
}


// ============================================================================
// Function: AFE_SetFrameByV0113 @ 6bb2f3b6
// ============================================================================

void AFE_SetFrameByV0113(undefined8 *param_1,undefined8 *param_2)

{
  *param_2 = *param_1;
  *(undefined2 *)(param_2 + 1) = *(undefined2 *)(param_1 + 1);
  param_2[2] = param_1[2];
  param_2[3] = param_1[3];
  param_2[4] = param_1[4];
  param_2[5] = 0;
  *(undefined2 *)(param_2 + 6) = *(undefined2 *)(param_1 + 5);
  *(ushort *)((longlong)param_2 + 0x32) = (ushort)*(byte *)((longlong)param_1 + 0x2a);
  *(undefined4 *)((longlong)param_2 + 0x34) = *(undefined4 *)((longlong)param_1 + 0x2c);
  *(undefined4 *)(param_2 + 7) = 0;
  param_2[8] = 0;
  param_2[9] = 0;
  param_2[10] = 0;
  return;
}


// ============================================================================
// Function: AFE_SetFrameByV0130 @ 6bb2f47e
// ============================================================================

void AFE_SetFrameByV0130(undefined8 *param_1,undefined8 *param_2)

{
  *param_2 = *param_1;
  *(undefined2 *)(param_2 + 1) = *(undefined2 *)(param_1 + 1);
  param_2[2] = param_1[2];
  param_2[3] = param_1[3];
  param_2[4] = param_1[4];
  param_2[5] = param_1 + 6;
  *(undefined2 *)(param_2 + 6) = *(undefined2 *)(param_1 + 5);
  *(ushort *)((longlong)param_2 + 0x32) = (ushort)*(byte *)((longlong)param_1 + 0x2a);
  *(undefined4 *)((longlong)param_2 + 0x34) = *(undefined4 *)((longlong)param_1 + 0x2c);
  *(undefined4 *)(param_2 + 7) = *(undefined4 *)((longlong)param_1 + 0x34);
  param_2[8] = 0;
  param_2[9] = 0;
  param_2[10] = 0;
  return;
}


// ============================================================================
// Function: AFE_SetFrameByV130 @ 6bb2f8f6
// ============================================================================

void AFE_SetFrameByV130(undefined8 *param_1,undefined8 *param_2)

{
  *param_2 = *param_1;
  *(undefined2 *)(param_2 + 1) = *(undefined2 *)(param_1 + 1);
  param_2[2] = param_1[2];
  param_2[3] = param_1[3];
  param_2[4] = param_1[4];
  param_2[5] = param_1[5];
  *(undefined2 *)(param_2 + 6) = *(undefined2 *)(param_1 + 6);
  *(undefined2 *)((longlong)param_2 + 0x32) = *(undefined2 *)((longlong)param_1 + 0x32);
  *(undefined4 *)((longlong)param_2 + 0x34) = *(undefined4 *)((longlong)param_1 + 0x34);
  *(undefined4 *)(param_2 + 7) = *(undefined4 *)(param_1 + 7);
  param_2[8] = 0;
  param_2[9] = 0;
  param_2[10] = 0;
  return;
}


// ============================================================================
// Function: AFE_SetFrameNormal @ 6bb2f0ef
// ============================================================================

void AFE_SetFrameNormal(undefined8 *param_1,undefined8 *param_2,ulonglong param_3)

{
  *param_2 = *param_1;
  *(undefined2 *)(param_2 + 1) = *(undefined2 *)(param_1 + 1);
  param_2[2] = param_1[2];
  param_2[3] = param_1[3];
  param_2[4] = param_1[4];
  param_2[5] = param_1[5];
  *(undefined2 *)(param_2 + 6) = *(undefined2 *)(param_1 + 6);
  *(undefined2 *)((longlong)param_2 + 0x32) = *(undefined2 *)((longlong)param_1 + 0x32);
  *(undefined4 *)((longlong)param_2 + 0x34) = *(undefined4 *)((longlong)param_1 + 0x34);
  *(undefined4 *)(param_2 + 7) = *(undefined4 *)(param_1 + 7);
  param_2[8] = param_1[8];
  param_2[9] = param_1[9];
  param_2[10] = param_1[10];
  if (0x114ff < param_3) {
    *(undefined4 *)(param_2 + 0xb) = *(undefined4 *)(param_1 + 0xb);
  }
  return;
}


// ============================================================================
// Function: AFE_SetCapsByV100 @ 6bb2f54d
// ============================================================================

void AFE_SetCapsByV100(undefined2 *param_1,undefined2 *param_2)

{
  *param_2 = *param_1;
  param_2[1] = param_1[1];
  *(undefined1 *)(param_2 + 2) = *(undefined1 *)(param_1 + 2);
  *(undefined1 *)((longlong)param_2 + 5) = *(undefined1 *)((longlong)param_1 + 5);
  *(undefined1 *)(param_2 + 3) = *(undefined1 *)(param_1 + 3);
  *(undefined1 *)((longlong)param_2 + 7) = *(undefined1 *)((longlong)param_1 + 7);
  param_2[4] = param_1[4];
  *(undefined1 *)(param_2 + 5) = *(undefined1 *)(param_1 + 5);
  *(undefined2 **)(param_2 + 8) = param_1 + 6;
  *(undefined1 *)(param_2 + 0xc) = *(undefined1 *)(param_1 + 0x10);
  *(undefined2 **)(param_2 + 0x10) = param_1 + 0x11;
  *(undefined4 *)(param_2 + 0x14) = 0;
  *(undefined4 *)(param_2 + 0x16) = *(undefined4 *)(param_1 + 0x1c);
  *(undefined4 *)(param_2 + 0x18) = *(undefined4 *)(param_1 + 0x1e);
  *(undefined4 *)(param_2 + 0x1a) = *(undefined4 *)(param_1 + 0x20);
  *(undefined4 *)(param_2 + 0x1c) = *(undefined4 *)(param_1 + 0x22);
  *(undefined4 *)(param_2 + 0x1e) = *(undefined4 *)(param_1 + 0x24);
  *(undefined4 *)(param_2 + 0x20) = 0;
  *(undefined1 *)(param_2 + 0x22) = 0;
  *(undefined8 *)(param_2 + 0x24) = 0;
  *(undefined4 *)(param_2 + 0x28) = 0;
  *(undefined1 *)(param_2 + 0x2a) = 0;
  *(undefined4 *)(param_2 + 0x2c) = 0;
  *(undefined1 *)(param_2 + 0x2e) = 0;
  *(undefined1 *)((longlong)param_2 + 0x5d) = 0;
  if (10 < *(byte *)(param_1 + 5)) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/TSA_AFE/TSA_AFE_Trans.c",0x139,
                      "srcCaps->num_scan_freq <= THP_MAX_SCAN_FREQ_NUMBER_SUPPORTED");
    HardwareAnalyzer_SetSoftException();
  }
  if (10 < *(byte *)(param_1 + 0x10)) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/TSA_AFE/TSA_AFE_Trans.c",0x13a,
                      "srcCaps->num_scan_rate <= THP_MAX_SCAN_RATE_NUMBER_SUPPORTED");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: AFE_SetCapsByV130 @ 6bb2f720
// ============================================================================

void AFE_SetCapsByV130(undefined2 *param_1,undefined2 *param_2)

{
  *param_2 = *param_1;
  param_2[1] = param_1[1];
  *(undefined1 *)(param_2 + 2) = *(undefined1 *)(param_1 + 2);
  *(undefined1 *)((longlong)param_2 + 5) = *(undefined1 *)((longlong)param_1 + 5);
  *(undefined1 *)(param_2 + 3) = *(undefined1 *)(param_1 + 3);
  *(undefined1 *)((longlong)param_2 + 7) = *(undefined1 *)((longlong)param_1 + 7);
  param_2[4] = param_1[4];
  *(undefined1 *)(param_2 + 5) = *(undefined1 *)(param_1 + 5);
  *(undefined8 *)(param_2 + 8) = *(undefined8 *)(param_1 + 8);
  *(undefined1 *)(param_2 + 0xc) = *(undefined1 *)(param_1 + 0xc);
  *(undefined8 *)(param_2 + 0x10) = *(undefined8 *)(param_1 + 0x10);
  *(undefined4 *)(param_2 + 0x14) = *(undefined4 *)(param_1 + 0x14);
  *(undefined4 *)(param_2 + 0x16) = *(undefined4 *)(param_1 + 0x16);
  *(undefined4 *)(param_2 + 0x18) = *(undefined4 *)(param_1 + 0x18);
  *(undefined4 *)(param_2 + 0x1a) = *(undefined4 *)(param_1 + 0x1a);
  *(undefined4 *)(param_2 + 0x1c) = *(undefined4 *)(param_1 + 0x1c);
  *(undefined4 *)(param_2 + 0x1e) = *(undefined4 *)(param_1 + 0x1e);
  *(undefined4 *)(param_2 + 0x20) = 0;
  *(undefined1 *)(param_2 + 0x22) = 0;
  *(undefined8 *)(param_2 + 0x24) = 0;
  *(undefined4 *)(param_2 + 0x28) = 0;
  *(undefined1 *)(param_2 + 0x2a) = 0;
  *(undefined4 *)(param_2 + 0x2c) = 0;
  *(undefined1 *)(param_2 + 0x2e) = 0;
  *(undefined1 *)((longlong)param_2 + 0x5d) = 0;
  if (10 < *(byte *)(param_1 + 5)) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/TSA_AFE/TSA_AFE_Trans.c",0x15a,
                      "srcCaps->num_scan_freq <= THP_MAX_SCAN_FREQ_NUMBER_SUPPORTED");
    HardwareAnalyzer_SetSoftException();
  }
  if (10 < *(byte *)(param_1 + 0xc)) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/TSA_AFE/TSA_AFE_Trans.c",0x15b,
                      "srcCaps->num_scan_rate <= THP_MAX_SCAN_RATE_NUMBER_SUPPORTED");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: AFE_SetCapsNormal @ 6bb2eef0
// ============================================================================

void AFE_SetCapsNormal(undefined2 *param_1,undefined2 *param_2)

{
  *param_2 = *param_1;
  param_2[1] = param_1[1];
  *(undefined1 *)(param_2 + 2) = *(undefined1 *)(param_1 + 2);
  *(undefined1 *)((longlong)param_2 + 5) = *(undefined1 *)((longlong)param_1 + 5);
  *(undefined1 *)(param_2 + 3) = *(undefined1 *)(param_1 + 3);
  *(undefined1 *)((longlong)param_2 + 7) = *(undefined1 *)((longlong)param_1 + 7);
  param_2[4] = param_1[4];
  *(undefined1 *)(param_2 + 5) = *(undefined1 *)(param_1 + 5);
  *(undefined8 *)(param_2 + 8) = *(undefined8 *)(param_1 + 8);
  *(undefined1 *)(param_2 + 0xc) = *(undefined1 *)(param_1 + 0xc);
  *(undefined8 *)(param_2 + 0x10) = *(undefined8 *)(param_1 + 0x10);
  *(undefined4 *)(param_2 + 0x14) = *(undefined4 *)(param_1 + 0x14);
  *(undefined4 *)(param_2 + 0x16) = *(undefined4 *)(param_1 + 0x16);
  *(undefined4 *)(param_2 + 0x18) = *(undefined4 *)(param_1 + 0x18);
  *(undefined4 *)(param_2 + 0x1a) = *(undefined4 *)(param_1 + 0x1a);
  *(undefined4 *)(param_2 + 0x1c) = *(undefined4 *)(param_1 + 0x1c);
  *(undefined4 *)(param_2 + 0x1e) = *(undefined4 *)(param_1 + 0x1e);
  *(undefined4 *)(param_2 + 0x20) = *(undefined4 *)(param_1 + 0x20);
  *(undefined1 *)(param_2 + 0x22) = *(undefined1 *)(param_1 + 0x22);
  *(undefined8 *)(param_2 + 0x24) = *(undefined8 *)(param_1 + 0x24);
  *(undefined4 *)(param_2 + 0x28) = *(undefined4 *)(param_1 + 0x28);
  *(undefined1 *)(param_2 + 0x2a) = *(undefined1 *)(param_1 + 0x2a);
  *(undefined4 *)(param_2 + 0x2c) = *(undefined4 *)(param_1 + 0x2c);
  *(undefined1 *)(param_2 + 0x2e) = *(undefined1 *)(param_1 + 0x2e);
  *(undefined1 *)((longlong)param_2 + 0x5d) = *(undefined1 *)((longlong)param_1 + 0x5d);
  if (10 < *(byte *)(param_1 + 5)) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/TSA_AFE/TSA_AFE_Trans.c",0xba,
                      "srcCaps->num_scan_freq <= THP_MAX_SCAN_FREQ_NUMBER_SUPPORTED");
    HardwareAnalyzer_SetSoftException();
  }
  if (10 < *(byte *)(param_1 + 0xc)) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/TSA_AFE/TSA_AFE_Trans.c",0xbb,
                      "srcCaps->num_scan_rate <= THP_MAX_SCAN_RATE_NUMBER_SUPPORTED");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: AFE_SetCapsByV0113 @ 6bb2f1e3
// ============================================================================

void AFE_SetCapsByV0113(undefined2 *param_1,undefined2 *param_2)

{
  *param_2 = *param_1;
  param_2[1] = param_1[1];
  *(undefined1 *)(param_2 + 2) = *(undefined1 *)(param_1 + 2);
  *(undefined1 *)((longlong)param_2 + 5) = *(undefined1 *)((longlong)param_1 + 5);
  *(undefined1 *)(param_2 + 3) = *(undefined1 *)(param_1 + 3);
  *(undefined1 *)((longlong)param_2 + 7) = *(undefined1 *)((longlong)param_1 + 7);
  param_2[4] = param_1[4];
  *(undefined1 *)(param_2 + 5) = *(undefined1 *)(param_1 + 5);
  *(undefined2 **)(param_2 + 8) = param_1 + 6;
  *(undefined1 *)(param_2 + 0xc) = *(undefined1 *)(param_1 + 0xb);
  *(undefined2 **)(param_2 + 0x10) = param_1 + 0xc;
  *(undefined4 *)(param_2 + 0x14) = 0;
  *(undefined4 *)(param_2 + 0x16) = *(undefined4 *)(param_1 + 0xe);
  *(undefined4 *)(param_2 + 0x18) = *(undefined4 *)(param_1 + 0x10);
  *(undefined4 *)(param_2 + 0x1a) = *(undefined4 *)(param_1 + 0x12);
  *(undefined4 *)(param_2 + 0x1c) = *(undefined4 *)(param_1 + 0x14);
  *(undefined4 *)(param_2 + 0x1e) = *(undefined4 *)(param_1 + 0x16);
  *(undefined4 *)(param_2 + 0x20) = 0;
  *(undefined1 *)(param_2 + 0x22) = 0;
  *(undefined8 *)(param_2 + 0x24) = 0;
  *(undefined4 *)(param_2 + 0x28) = 0;
  *(undefined1 *)(param_2 + 0x2a) = 0;
  *(undefined4 *)(param_2 + 0x2c) = 0;
  *(undefined1 *)(param_2 + 0x2e) = 0;
  *(undefined1 *)((longlong)param_2 + 0x5d) = 0;
  if (10 < *(byte *)(param_1 + 5)) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/TSA_AFE/TSA_AFE_Trans.c",0xf2,
                      "srcCaps->num_scan_freq <= THP_MAX_SCAN_FREQ_NUMBER_SUPPORTED");
    HardwareAnalyzer_SetSoftException();
  }
  if (10 < *(byte *)(param_1 + 0xb)) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/TSA_AFE/TSA_AFE_Trans.c",0xf3,
                      "srcCaps->num_scan_rate <= THP_MAX_SCAN_RATE_NUMBER_SUPPORTED");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: AFE_SetCapsByV190 @ 6bb2f9c2
// ============================================================================

void AFE_SetCapsByV190(undefined2 *param_1,undefined2 *param_2)

{
  *param_2 = *param_1;
  param_2[1] = param_1[1];
  *(undefined1 *)(param_2 + 2) = *(undefined1 *)(param_1 + 2);
  *(undefined1 *)((longlong)param_2 + 5) = *(undefined1 *)((longlong)param_1 + 5);
  *(undefined1 *)(param_2 + 3) = *(undefined1 *)(param_1 + 3);
  *(undefined1 *)((longlong)param_2 + 7) = *(undefined1 *)((longlong)param_1 + 7);
  param_2[4] = param_1[4];
  *(undefined1 *)(param_2 + 5) = *(undefined1 *)(param_1 + 5);
  *(undefined8 *)(param_2 + 8) = *(undefined8 *)(param_1 + 8);
  *(undefined1 *)(param_2 + 0xc) = *(undefined1 *)(param_1 + 0xc);
  *(undefined8 *)(param_2 + 0x10) = *(undefined8 *)(param_1 + 0x10);
  *(undefined4 *)(param_2 + 0x14) = *(undefined4 *)(param_1 + 0x14);
  *(undefined4 *)(param_2 + 0x16) = *(undefined4 *)(param_1 + 0x16);
  *(undefined4 *)(param_2 + 0x18) = *(undefined4 *)(param_1 + 0x18);
  *(undefined4 *)(param_2 + 0x1a) = *(undefined4 *)(param_1 + 0x1a);
  *(undefined4 *)(param_2 + 0x1c) = *(undefined4 *)(param_1 + 0x1c);
  *(undefined4 *)(param_2 + 0x1e) = *(undefined4 *)(param_1 + 0x1e);
  *(undefined4 *)(param_2 + 0x20) = *(undefined4 *)(param_1 + 0x20);
  *(undefined1 *)(param_2 + 0x22) = *(undefined1 *)(param_1 + 0x22);
  *(undefined8 *)(param_2 + 0x24) = *(undefined8 *)(param_1 + 0x24);
  *(undefined4 *)(param_2 + 0x28) = *(undefined4 *)(param_1 + 0x28);
  *(undefined1 *)(param_2 + 0x2a) = *(undefined1 *)(param_1 + 0x2a);
  *(undefined4 *)(param_2 + 0x2c) = 0;
  *(undefined1 *)(param_2 + 0x2e) = 0;
  *(undefined1 *)((longlong)param_2 + 0x5d) = 0;
  if (10 < *(byte *)(param_1 + 5)) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/TSA_AFE/TSA_AFE_Trans.c",0x18e,
                      "srcCaps->num_scan_freq <= THP_MAX_SCAN_FREQ_NUMBER_SUPPORTED");
    HardwareAnalyzer_SetSoftException();
  }
  if (10 < *(byte *)(param_1 + 0xc)) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/TSA_AFE/TSA_AFE_Trans.c",399,
                      "srcCaps->num_scan_rate <= THP_MAX_SCAN_RATE_NUMBER_SUPPORTED");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: AFE_IsManualFreqShift @ 6bb2d1b9
// ============================================================================

bool AFE_IsManualFreqShift(void)

{
                    /* 0xed1b9  41  AFE_IsManualFreqShift */
  return _refptr_g_tsaPrmtFlash->dwPeakProcessingMode != 3;
}


// ============================================================================
// Function: AFE_GetSpecVersion @ 6bb2c989
// ============================================================================

undefined8 AFE_GetSpecVersion(void)

{
                    /* 0xec989  22  AFE_GetSpecVersion */
  return 0x11a00;
}


// ============================================================================
// Function: AFE_GetSpecMajorVersion @ 6bb2c994
// ============================================================================

undefined8 AFE_GetSpecMajorVersion(void)

{
                    /* 0xec994  19  AFE_GetSpecMajorVersion */
  return 1;
}


// ============================================================================
// Function: AFE_GetSpecMinorVersion @ 6bb2c99f
// ============================================================================

undefined8 AFE_GetSpecMinorVersion(void)

{
                    /* 0xec99f  20  AFE_GetSpecMinorVersion */
  return 0x1a;
}


// ============================================================================
// Function: AFE_GetSpecPatchVersion @ 6bb2c9aa
// ============================================================================

undefined8 AFE_GetSpecPatchVersion(void)

{
                    /* 0xec9aa  21  AFE_GetSpecPatchVersion */
  return 0;
}


// ============================================================================
// Function: AFE_AutoCheckProcess @ 6bb2c5ed
// ============================================================================

bool AFE_AutoCheckProcess(longlong param_1,undefined4 param_2,undefined8 param_3)

{
  longlong lVar1;
  
                    /* 0xec5ed  2  AFE_AutoCheckProcess */
  if (param_1 != 0) {
    AFE_SetFrame(param_1);
    lVar1 = AFE_GetFrame();
    AFE_AutoCheckProcessInternal
              (*(undefined8 *)(lVar1 + 0x10),_refptr_g_tsaPrmtFlash->wBufElementCount * 2,param_2,
               param_3);
  }
  return param_1 == 0;
}


// ============================================================================
// Function: AFE_SetTestLevel @ 6bb2c1c5
// ============================================================================

void AFE_SetTestLevel(undefined4 param_1)

{
                    /* 0xec1c5  66  AFE_SetTestLevel */
  g_afeTestLevel = param_1;
  return;
}


// ============================================================================
// Function: AFE_GetTestResult @ 6bb2c1d7
// ============================================================================

undefined * AFE_GetTestResult(void)

{
  char cVar1;
  
                    /* 0xec1d7  24  AFE_GetTestResult */
  cVar1 = TSALog_RtEnable(2,0x3e,0x10);
  if (cVar1 != '\0') {
    StrPrintf_Process(0,"\ng_afeTestErrorInfo=%s\n",g_afeTestErrorInfo);
  }
  return g_afeTestErrorInfo;
}


