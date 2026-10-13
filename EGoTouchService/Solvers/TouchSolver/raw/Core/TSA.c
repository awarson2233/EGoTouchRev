#include "TSACoreInternal.h"

// ============================================================================
// Function: TSA_Processing @ 6ba9a443
// ============================================================================

/* Top-level frame pipeline: runs TSA_MSProcess unless skipped, then side-touch/info, ASA when
   FEAT_ASA_ENABLED, optional StylusAFT when FEAT_STYLUS_AFT and runtime gate allow it, then
   output/log/finalization. */

void TSA_Processing(short *pMutualRawData,uint dwCtrlFlags,void *pFrameBuffer)

{
  char cConditionResult;
  int nDmdType;
  longlong llMsTimestamp;
  uint dwCtrlFlagsLocal [2];
  void *pFrameBufferLocal;
  TSAStatic *pTsaStatic;
  
                    /* 0x5a443  2122  TSA_Processing */
  dwCtrlFlagsLocal[0] = dwCtrlFlags;
  pFrameBufferLocal = pFrameBuffer;
  TSAFold_InputProcess(pMutualRawData,dwCtrlFlagsLocal,pFrameBuffer);
  if ((dwCtrlFlagsLocal[0] & 0x100000) == 0) {
    dwCtrlFlagsLocal[0] = Proximity_FilterExtraWakeup(dwCtrlFlagsLocal[0],pFrameBufferLocal);
  }
  AFE_PreProcess();
  (*_refptr_g_tsaStaticPtr)->dwProcessingState =
       (*_refptr_g_tsaStaticPtr)->dwProcessingState | PROC_TSA_ACTIVE;
  (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr = pFrameBufferLocal;
  pTsaStatic = *_refptr_g_tsaStaticPtr;
  llMsTimestamp = TSAStatic_GetMSTimeStamp();
  pTsaStatic->llMsTimestamp = llMsTimestamp;
  TSAStatic_UpdateCtrlFlags(dwCtrlFlagsLocal[0]);
  TSAStatic_PreProcess();
  BLSM_PreProcess();
  if ((dwCtrlFlagsLocal[0] & 0x100000) == 0) {
    cConditionResult = AFE_IsInAutoCheckProcess();
    if (cConditionResult != '\0') {
      AFE_ClearAutoCheckFlag();
    }
    if ((int)(*_refptr_g_tsaStaticPtr)->dwSystemFlags < 0) {
      cConditionResult = TSA_IsInProcess();
      if ((cConditionResult == '\0') && (pMutualRawData != (short *)0x0)) {
        SafeBaseline_SetBuf(pMutualRawData);
      }
      TSAStatic_PostProcess();
    }
    else {
      cConditionResult = TSA_IsInProcess();
      if (cConditionResult == '\0') {
        (*_refptr_g_tsaStaticPtr)->dwSystemFlags =
             (*_refptr_g_tsaStaticPtr)->dwSystemFlags | FLAG_PROCESSING_ACTIVE;
      }
      TSALog_Frame();
      TSAIDE_Reset();
      TSALog_Input(pMutualRawData,(*_refptr_g_tsaStaticPtr)->dwSystemFlags,
                   *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0xc8,
                   (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr);
      cConditionResult = TSAStatic_IsHWReset();
      if (cConditionResult != '\0') {
        EventAnalyzer_Init();
      }
      if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & FLAG_SKIP_MS_PROCESS) == 0) {
        TSA_MSProcess(pMutualRawData);
      }
      Proximity_TouchPostProcess();
      SideTouch_Process();
      Thenar_ProcessInfoByDif();
      SHBVolumeData_Process();
                    /* [FEAT_ASA_ENABLED=1 → ON] TSA_ASAProcess: stylus coordinate correction
                       pipeline runs */
      if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) != 0) {
        TSA_ASAProcess(dwCtrlFlagsLocal[0]);
      }
      if (((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) != 0) &&
          (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_STYLUS_AFT) != 0)) &&
         (((*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags & 4) == 0)) {
        TSA_StylusAftProcess();
      }
      TSAOut_PostProcess();
      nDmdType = TSA_SetDMDType();
                    /* [FEAT_SMART_COVER=0 → OFF] SmartCover_Process skipped */
      if ((nDmdType != 0) && (*_refptr_g_tsaLogLoglevel != '\0')) {
                    /* [FEAT_AFT_ENABLED=0 → OFF] AFT_Process skipped */
        StrPrintf_Process(0,"TSA Set DMD type = %d",nDmdType);
      }
      if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) != 0) {
        SmartCover_Process();
      }
      if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_AFT_ENABLED) != 0) {
        AFT_Process();
      }
      TSAStatic_PostProcess();
      DoubleReport_Process();
      TSALog_Output();
      (*_refptr_g_tsaStaticPtr)->bFrameDoneFlag = '\x01';
      (*_refptr_g_tsaStaticPtr)->nFrameCounter = (*_refptr_g_tsaStaticPtr)->nFrameCounter + 1;
      (*_refptr_g_tsaStaticPtr)->dwProcessingState =
           (*_refptr_g_tsaStaticPtr)->dwProcessingState & ~PROC_TSA_ACTIVE;
      AFE_Process();
      TSAFold_OutputProcess();
      TSA_PitchSizeRemapProcess();
    }
  }
  else {
    AFE_AutoCheckProcessInternal
              (pMutualRawData,_refptr_g_tsaPrmtFlash->wBufElementCount * 2,dwCtrlFlagsLocal[0],
               pFrameBufferLocal);
    TSAStatic_PostProcess();
  }
  return;
}


// ============================================================================
// Function: TSA_MSProcess @ 6ba90e43
// ============================================================================

/* Current W273AS2700 MS finger pipeline: raw->dif/BLSM->CMF fallback->GridIIR->Peak mode 3->touch
   tracking/report. Disabled branches: Rawdata_CMF, SafeBaseline, UnderWater, SignalDisparity_Post,
   AFT. */

void TSA_MSProcess(short *pMutualRawData)

{
  char cConditionResult;
  errno_t copyStatus;
  int nNoiseFilterSwitchResult;
  int nSignalDisparitySwitchResult;
  void *pAfeFrameOrSideFrame;
  longlong lVar1;
  bool bDifBufferSwitched;
  bool bKeepPreviousTouches;
  
                    /* === TSA_MSProcess: Mutual-Sensing Touch Pipeline Entry ===
                       Sets PROC_MS_ACTIVE flag, switches data mux to grid mode,
                       fetches AFE frame and copies raw data via SS_CopyRaw. */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&_rdata,"TSA_MSProcess");
  }
  (*_refptr_g_tsaStaticPtr)->dwProcessingState =
       (*_refptr_g_tsaStaticPtr)->dwProcessingState | PROC_MS_ACTIVE;
  DataSwitch_ToGrid();
  pAfeFrameOrSideFrame = (void *)AFE_GetFrame();
  SS_CopyRaw(*(undefined8 *)((longlong)pAfeFrameOrSideFrame + 0x18),
             (uint)_refptr_g_tsaPrmtFlash->bRows + (uint)_refptr_g_tsaPrmtFlash->bCols);
  if ((pMutualRawData == (short *)0x0) ||
     ((_refptr_g_tsaPrmtDynamic[0x24] != '\0' &&
      (cConditionResult = SS_IsRawValid(), cConditionResult == '\0')))) {
    if ((pMutualRawData == (short *)0x0) && (*_refptr_g_tsaLogLoglevel != '\0')) {
      StrPrintf_Process(0,"    DataPtr == NULL!");
    }
    cConditionResult = TSAStatic_IsHWReset();
    if (cConditionResult != '\0') {
      TSA_MSReset();
    }
    lVar1 = AFE_GetFrame();
    lVar1 = *(longlong *)(lVar1 + 0x40);
    cConditionResult = SS_IsRawValid();
    if (((cConditionResult != '\0') && (lVar1 != 0)) && ((*(uint *)(lVar1 + 0x1c) & 0x10) != 0)) {
      SS_Process();
    }
    TSA_MSProcessEnding();
  }
  else {
                    /* --- NORMAL PATH: pre-processing & raw check --- */
    TSAIDE_CleanPointXY();
    HardwareAnalyzer_Reset();
    cConditionResult = TSAStatic_IsHWReset();
    if (cConditionResult == '\0') {
      cConditionResult = TSA_IsToBypassCurrentFrame();
      if (cConditionResult == '\0') {
        Proximity_Process();
      }
    }
    else {
      TSA_MSReset();
    }
    Touch_Clean();
    copyStatus = memcpy_s(*(void **)_refptr_g_tsaBufPreDif,
                          (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,
                          *(void **)_refptr_g_tsaBufDif,
                          (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
    if (copyStatus != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                        0x1e8,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
    copyStatus = memcpy_s(_refptr_g_tsaBufPreRaw,0x2648,*(void **)_refptr_g_tsaBufRaw,
                          (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
    if (copyStatus != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                        0x1ed,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
    *(short **)_refptr_g_tsaUnnormalizedRawBuf = pMutualRawData;
    TSA_RawCheckProcess();
                    /* [FLAG_USE_DIF_BUF_PATH check] Determines raw data destination: g_tsaBufRaw vs
                       g_tsaBufDif */
    if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & FLAG_USE_DIF_BUF_PATH) == 0) {
      copyStatus = memcpy_s(*(void **)_refptr_g_tsaBufRaw,0x2648,pMutualRawData,
                            (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
      if (copyStatus != 0) {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                          0x1fa,"rc == EOK");
        HardwareAnalyzer_SetSoftException();
      }
      TPSensor_ProcessRaw();
    }
    else {
      copyStatus = memcpy_s(*(void **)_refptr_g_tsaBufDif,0x2648,pMutualRawData,
                            (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
      if (copyStatus != 0) {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                          0x1f7,"rc == EOK");
        HardwareAnalyzer_SetSoftException();
      }
    }
    Rawdata_Process();
    HardwareAnalyzer_Process();
    cConditionResult = TSA_IsToBypassCurrentFrame();
    if (cConditionResult == '\0') {
                    /* --- SIGNAL PROCESSING LAYER: baseline, CMF, IIR, TPSensor, self, side --- */
      TSAPrmt_PreProcess();
      SS_Process();
      copyStatus = memcpy_s(*(void **)_refptr_g_tsaBufPreCMFRaw,0x2648,*(void **)_refptr_g_tsaBufRaw
                            ,(ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
      if (copyStatus != 0) {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                          0x22d,"rc == EOK");
        HardwareAnalyzer_SetSoftException();
      }
                    /* [FLAG_USE_DIF_BUF_PATH check] If SET: skip BLSM+CMF+IIR, go directly to
                       HardwareAnalyzer_ProcessDif */
      if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & FLAG_USE_DIF_BUF_PATH) == 0) {
        BLIIR_CalcDiffCommon
                  (*(short **)_refptr_g_tsaBufDif,*(short **)_refptr_g_tsaBufRaw,
                   *(short **)_refptr_g_tsaBufBl);
        TSAPrpt_GetDifPreCMFPrpt();
        BLSM_UpdateForRawUnstable();
        memcpy(_refptr_g_tsaBufSelfForBlRst,_refptr_g_tsaBufSelf,0x1b80);
                    /* [bCmfEnabled=0 → OFF] Rawdata_CMF is skipped, CMF_Process runs later
                       instead */
        if (_refptr_g_tsaPrmtFlash->bCmfEnabled != '\0') {
          Rawdata_CMF();
        }
                    /* [FEAT_UNDERWATER_DETECT=0 → OFF] Exception_CheckUnderWater is skipped */
        if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_UNDERWATER_DETECT) != 0) {
          Exception_CheckUnderWater();
        }
                    /* [FEAT_SAFE_BASELINE=0 → OFF]
                       SafeBaseline_ScreenOnWithPhoneHeldInHandProcess is skipped */
        if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SAFE_BASELINE) != 0) {
          SafeBaseline_ScreenOnWithPhoneHeldInHandProcess();
        }
        cConditionResult = TSAStatic_IsHWReset();
        if (cConditionResult == '\0') {
          BLSM_Process(false,false);
        }
        else {
          BLSM_Process(true,false);
        }
        cConditionResult = BLSM_IsReset();
        if (cConditionResult == '\0') {
                    /* [bCmfEnabled=0 → OFF] Fallback: CMF_Process runs here */
          if (_refptr_g_tsaPrmtFlash->bCmfEnabled == '\0') {
            CMF_Process();
          }
          GridIIR_Process();
        }
      }
      HardwareAnalyzer_ProcessDif();
      TPSensor_Process();
      TSA_GetPrpt(*(short **)_refptr_g_tsaBufRaw,*(short **)_refptr_g_tsaBufBl);
      memcpy(&_refptr_g_tsaPrpt->field_0x16c,&_refptr_g_tsaPrpt->nSPeakPositive,0xc0);
      Self_Process();
      ToeSynaBl_GetSidePrpt();
      SS_CheckDirtyByMutual();
                    /* --- CHARGER NOISE CHECKPOINT: bypass → keep prev touch --- */
      TSA_MSRawDirectionDectect();
      bKeepPreviousTouches = false;
      Exception_CheckChargerNoiseInRxLines();
      if (((*_refptr_g_prevTouchesPtr)->nTouchCount != 0x100) &&
         (cConditionResult = Exception_IsChargerNoiseDetected(), cConditionResult != '\0')) {
        bKeepPreviousTouches = true;
        Exception_Process();
      }
      cConditionResult = TSA_IsToBypassCurrentFrame();
      if (cConditionResult != '\0') {
        bKeepPreviousTouches = true;
      }
      if (bKeepPreviousTouches) {
        Touch_KeepPrevTouchWithExceptionExcluded();
        SideTouch_KeepPrevTouchWithExceptionExcluded();
        TSA_MSProcessEnding();
      }
      else {
                    /* --- PEAK DETECTION: signal disparity, Peak_Process, switch buffer re-peak ---
                        */
        SS_ChargerNoiseFilterProcess();
        TSAPrmt_Process();
        SignalDisparity_Process();
        Peak_Process();
        memcpy(_refptr_g_peaksBeforeSwitchDifBuffer,*_refptr_g_peaksPtr,0x1a0);
        nNoiseFilterSwitchResult = SS_ChargerNoiseFilterSwitchBuffer();
        nSignalDisparitySwitchResult = SignalDisparity_SwitchBuffer();
        bDifBufferSwitched = nSignalDisparitySwitchResult != 0 || nNoiseFilterSwitchResult != 0;
        if (bDifBufferSwitched) {
          Peak_Process();
        }
        TSA_MSPeakFilter();
        Exception_CheckPanelSD();
        _refptr_g_tsaPrmtFlash->dwGripFlags = _refptr_g_tsaPrmtFlash->dwGripFlags | 0x400;
        GripFilter_RegionProcess();
                    /* --- TOUCH TRACKING: TZ, pre-filter, CTD, IDT, AssignID, TS_Process,
                       TE_Process --- */
        TSABuffer_PrevFrameProcess();
        TZ_Process();
        _refptr_g_tsaPrmtFlash->dwGripFlags = _refptr_g_tsaPrmtFlash->dwGripFlags & 0xfffffbff;
        TSA_MSTouchPreFilter();
        CTD_ECProcess();
        IDT_Process('\0');
        Touch_AssignPreIdxBasedOnID();
        PrevTouch_AssignCurIdxBasedOnID();
        TS_Process();
        TE_Process();
        TSABuffer_PreProcess();
                    /* [FEAT_SAFE_BASELINE=0 → OFF] SafeBaseline_Process is skipped */
        if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SAFE_BASELINE) != 0) {
          SafeBaseline_Process();
        }
        ER_Process();
                    /* --- POST-PROCESSING: TouchAction, Gesture, GripFilter, TouchMode --- */
        TouchAction_Process();
        TSA_MSTouchPostFilter();
        TS_PostProcess();
        TouchAction_PostProcess();
        Exception_Process();
        Gesture_Process();
        GripFilter_Process();
        TouchMode_Process();
                    /* [FEAT_SIGNAL_DISPARITY_POST=0 → OFF] SignalDisparity_PostProcess is skipped
                        */
        if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SIGNAL_DISPARITY_POST) != 0) {
          SignalDisparity_PostProcess();
        }
        SS_ChargerNoiseFilterPostProcess();
        TSAIDE_LogPreFltGridRpt();
                    /* --- REPORTING & FINALIZATION: report, ext info, big data, ending --- */
        AntiTouch_Process();
        Touch_ProcessBetaGrip();
        TouchReport_Process();
        Touch_ProcessExtInfo();
        HardwareAnalyzer_PostProcess();
        HandGesture_Process();
        BigData_RecordProcess();
        BigData_StatProcess();
        PrevTouch_PostProcess();
        PrevPeak_Process();
        TSABuffer_PostProcess();
        TSA_MSProcessEnding();
      }
    }
    else {
      Touch_KeepPrevTouchWithExceptionExcluded();
      SideTouch_KeepPrevTouchWithExceptionExcluded();
      BLIIR_CalcDiffCommon
                (*(short **)_refptr_g_tsaBufDif,*(short **)_refptr_g_tsaBufRaw,
                 *(short **)_refptr_g_tsaBufBl);
      TSAPrpt_GetDifPreCMFPrpt();
      copyStatus = memcpy_s(*(void **)_refptr_g_tsaBufDifGrid,
                            (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,
                            *(void **)_refptr_g_tsaBufPreDif,
                            (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
      if (copyStatus != 0) {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                          0x214,"rc == EOK");
        HardwareAnalyzer_SetSoftException();
      }
      TSA_MSProcessEnding();
    }
  }
  return;
}


// ============================================================================
// Function: TSA_SetDMDType @ 6ba9d881
// ============================================================================

int TSA_SetDMDType(void)

{
  char cVar1;
  int iVar2;
  int local_c;
  
                    /* 0x5d881  2275  TSA_SetDMDType */
  iVar2 = HardwareAnalyzer_GetDetectedException();
  if (iVar2 != 0) {
    HardwareAnalyzer_GenerateExceptionDescription();
  }
  switch(iVar2) {
  default:
    local_c = 0;
    break;
  case 1:
    local_c = 1;
    break;
  case 2:
    local_c = 2;
    break;
  case 3:
    local_c = 8;
    break;
  case 4:
    local_c = 0x20;
    break;
  case 5:
  case 0x1f:
    local_c = 0x8000;
    break;
  case 6:
  case 7:
    local_c = 4;
    break;
  case 8:
    local_c = 0x40;
    break;
  case 9:
    local_c = 0x80;
    break;
  case 10:
    local_c = 0x100;
    break;
  case 0xb:
    local_c = 0x200;
    break;
  case 0xc:
    local_c = 0x400;
    break;
  case 0xd:
    local_c = 0x800;
    break;
  case 0xe:
    local_c = 0x1000;
    break;
  case 0xf:
    local_c = 0x2000;
    break;
  case 0x10:
    local_c = 0x4000;
    break;
  case 0x11:
  case 0x12:
    local_c = 0x10000;
    break;
  case 0x13:
    local_c = 0x20000;
    break;
  case 0x14:
    local_c = 1;
    break;
  case 0x15:
    local_c = 0x40000;
    break;
  case 0x16:
  case 0x17:
    local_c = 0x80000;
    break;
  case 0x18:
    local_c = 0x800000;
    break;
  case 0x19:
    local_c = 0x1000000;
    break;
  case 0x1a:
    local_c = 0x2000000;
    break;
  case 0x1b:
    local_c = 0x4000000;
    break;
  case 0x1c:
    local_c = 0x8000000;
    break;
  case 0x1e:
    local_c = 1;
  }
  if (local_c == 0) {
    BigData_ReportProcess();
    cVar1 = BigData_IsReportReady();
    if (cVar1 != '\0') {
      local_c = 0x10;
    }
  }
  (*_refptr_g_tsaStaticPtr)->nField_0x138 = local_c;
  return local_c;
}


// ============================================================================
// Function: TSA_IsInProcess @ 6ba9f7d7
// ============================================================================

uchar TSA_IsInProcess(void)

{
                    /* 0x5f7d7  2072  TSA_IsInProcess */
  return (*_refptr_g_tsaStaticPtr)->bFrameDoneFlag;
}


// ============================================================================
// Function: TSA_PitchSizeRemapProcess @ 6bb0611c
// ============================================================================

void TSA_PitchSizeRemapProcess(void)

{
  int iVar1;
  int iVar2;
  char local_9;
  
                    /* 0xc611c  2121  TSA_PitchSizeRemapProcess */
  for (local_9 = '\0'; (int)local_9 < (int)(uint)**(byte **)_refptr_g_touchesPtr;
      local_9 = local_9 + '\x01') {
    iVar1 = SensorPitchSizeMapDim1
                      ((uint)*(ushort *)
                              ((longlong)(int)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                              0x170),0x100);
    iVar2 = SensorPitchSizeMapDim2
                      ((uint)*(ushort *)
                              ((longlong)(int)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                              0x172),0x100);
    *(short *)((longlong)(int)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170) =
         (short)iVar1;
    *(short *)((longlong)(int)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172) =
         (short)iVar2;
  }
  return;
}


// ============================================================================
// Function: TSA_StylusAftProcess @ 6bb22b1c
// ============================================================================

void TSA_StylusAftProcess(void)

{
  char cVar1;
  ushort uVar2;
  short sVar3;
  ushort uVar4;
  ushort uVar5;
  byte local_19;
  
                    /* 0xe2b1c  2323  TSA_StylusAftProcess */
  uVar2 = TSA_ASAAFEStatus();
  if ((uVar2 & 0x10) != 0) {
    cVar1 = ASA_GetRptInRange();
    if (cVar1 != '\0') {
      g_lastStylusTimeStamp = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    }
  }
  sVar3 = TSA_RptASAPressure();
  if (sVar3 != 0) {
    uVar2 = TSA_RptASAXPos();
    uVar4 = TSA_GetDim1Res();
    uVar5 = TSA_GetASADim1Res();
    g_lastStylusX = (int)((uint)uVar2 * (uint)uVar4) / (int)(uint)uVar5;
    uVar2 = TSA_RptASAYPos(uVar5,(longlong)(int)((uint)uVar2 * (uint)uVar4) %
                                 (longlong)(int)(uint)uVar5 & 0xffffffff);
    uVar4 = TSA_GetDim2Res();
    uVar5 = TSA_GetASADim2Res();
    g_lastStylusY = (int)((uint)uVar2 * (uint)uVar4) / (int)(uint)uVar5;
    sVar3 = TSA_RptASAXTilt(uVar5,(longlong)(int)((uint)uVar2 * (uint)uVar4) %
                                  (longlong)(int)(uint)uVar5 & 0xffffffff);
    g_lastTiltX = (int)sVar3;
    sVar3 = TSA_RptASAYTilt();
    g_lastTiltY = (int)sVar3;
  }
  for (local_19 = 0; local_19 < **(byte **)_refptr_g_touchesPtr; local_19 = local_19 + 1) {
    if ((*(uint *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c
                  ) & 0x20) == 0) {
      StylusAft_PostProcessUnit(local_19);
    }
  }
  return;
}


// ============================================================================
// Function: TSA_IsGameScenario @ 6ba8fe7f
// ============================================================================

undefined8 TSA_IsGameScenario(void)

{
  char cVar1;
  
                    /* 0x4fe7f  2065  TSA_IsGameScenario */
  cVar1 = TSA_IsHighRateGameScenario();
  if ((cVar1 == '\0') && (cVar1 = TSA_IsNormalRateGameScenario(), cVar1 == '\0')) {
    return 0;
  }
  return 1;
}


// ============================================================================
// Function: TSA_MSRawDirectionDectect @ 6ba90a3d
// ============================================================================

void TSA_MSRawDirectionDectect(void)

{
  int iVar1;
  char *_Str1;
  
  if (_refptr_g_tsaPrpt->field_0x90 == '\0') {
    _Str1 = (char *)TSAPrmt_GetProjectName();
    iVar1 = strcmp(_Str1,"DFLT000000");
    if ((iVar1 == 0) &&
       ((_refptr_g_tsaPrpt->nSPeakNegative < -1000 || (1000 < _refptr_g_tsaPrpt->nSPeakPositive))))
    {
      if (_refptr_g_tsaPrpt->nSPeakNegative < -1000) {
        iVar1 = (*(int *)&_refptr_g_tsaPrmtFlash->field_0x5fc -
                *(int *)&_refptr_g_tsaPrmtFlash->field_0x600) +
                *(int *)&_refptr_g_tsaPrmtFlash->field_0x5fc;
        *(int *)&_refptr_g_tsaPrmtFlash->field_0x600 = iVar1;
        TSAFold_PrmtSetRawWithTouch(iVar1);
      }
      TSAConstPrmt_Init();
      _refptr_g_tsaPrpt->field_0x90 = 1;
    }
  }
  return;
}


// ============================================================================
// Function: TSA_MSReset @ 6ba90b16
// ============================================================================

void TSA_MSReset(void)

{
  undefined1 uVar1;
  TSA_FeatureFlags TVar2;
  uchar local_1a;
  uchar local_19;
  void *local_18;
  undefined2 local_e;
  ushort local_c;
  undefined1 local_9;
  
                    /* 0x50b16  2119  TSA_MSReset */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&_rdata,"TSA_MSReset");
  }
  TVar2 = (*_refptr_g_tsaStaticPtr)->dwFeatureFlagsMasked;
  local_9 = 0;
  local_c = 0;
  local_e = 0;
  local_18 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  local_19 = '\0';
  local_1a = '\0';
  uVar1 = (*_refptr_g_tsaStaticPtr)->field_0xa1;
  if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 8) != 0) {
    local_9 = _refptr_g_tsaPrpt->field_0x29c;
    local_c = (*_refptr_g_tsaStaticPtr)->wField_0x120;
  }
  if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 4) == 0) {
    local_e = *(undefined2 *)&_refptr_g_tsaPrpt->field_0x284;
  }
  if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 1) == 0) {
    local_19 = (*_refptr_g_tsaStaticPtr)->bRecalFrameCount;
    local_1a = (*_refptr_g_tsaStaticPtr)->bRecalBoundaryHitCount;
  }
  if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 5) == 0) {
    local_18 = *(void **)&(*_refptr_g_tsaStaticPtr)->field_0x80;
  }
  TSA_MSInitCommon(0);
  SS_Reset();
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlags = (*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0xffffb7ec
  ;
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlags = (*_refptr_g_tsaStaticPtr)->dwFeatureFlags | TVar2;
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlagsMasked = TVar2;
  (*_refptr_g_tsaStaticPtr)->field_0xa1 = uVar1;
  if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 8) != 0) {
    _refptr_g_tsaPrpt->field_0x29c = local_9;
    (*_refptr_g_tsaStaticPtr)->wField_0x120 = local_c;
  }
  if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 4) == 0) {
    *(undefined2 *)&_refptr_g_tsaPrpt->field_0x284 = local_e;
  }
  if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 1) == 0) {
    (*_refptr_g_tsaStaticPtr)->bRecalFrameCount = local_19;
    (*_refptr_g_tsaStaticPtr)->bRecalBoundaryHitCount = local_1a;
  }
  *(void **)&(*_refptr_g_tsaStaticPtr)->field_0x80 = local_18;
  (*_refptr_g_tsaStaticPtr)->qwBaselineResetReferenceTime = 0;
  return;
}


// ============================================================================
// Function: TSA_MSPeakFilter @ 6ba90dbd
// ============================================================================

void TSA_MSPeakFilter(void)

{
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags2 & 1) != 0) {
    EdgePeakFilter_WorkAround();
  }
  return;
}


// ============================================================================
// Function: TSA_MSTouchPreFilter @ 6ba90de8
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

void TSA_MSTouchPreFilter(void)

{
  RxLineFilter_Process();
  SigSumFilter_ReserveTouch(_refptr_g_tsaPrmtFlash[1].field_0x74);
  return;
}


// ============================================================================
// Function: TSA_MSTouchPostFilter @ 6ba90e14
// ============================================================================

void TSA_MSTouchPostFilter(void)

{
  return;
}


// ============================================================================
// Function: TSA_MSProcessEnding @ 6ba90e1a
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

void TSA_MSProcessEnding(void)

{
                    /* Clears PROC_MS_ACTIVE flag to mark MS pipeline exit. */
  (*_refptr_g_tsaStaticPtr)->dwProcessingState =
       (*_refptr_g_tsaStaticPtr)->dwProcessingState & ~PROC_MS_ACTIVE;
  return;
}


// ============================================================================
// Function: TSA_IsToBypassCurrentFrame @ 6ba9f720
// ============================================================================

char TSA_IsToBypassCurrentFrame(void)
{
                    /* 0x5f720  2099  TSA_IsToBypassCurrentFrame */
  return HardwareAnalyzer_IsToBypassCurrentFrame();
}


// ============================================================================
// Function: TSA_GetPrpt @ 6baa1353
// ============================================================================

void TSA_GetPrpt(short *pRawGrid,short *pBaselineGrid)

{
                    /* 0x61353  1975  TSA_GetPrpt */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&_rdata,"TSA_GetPrpt");
  }
  if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & FLAG_USE_DIF_BUF_PATH) == 0) {
    TSA_GetRawPrpt(pRawGrid,pBaselineGrid);
  }
  TSA_GetDifPrpt();
  return;
}


// ============================================================================
// Function: TSA_RawCheckProcess @ 6bafcb0b
// ============================================================================

void TSA_RawCheckProcess(void)

{
  int iVar1;
  undefined8 uVar2;
  
                    /* 0xbcb0b  2131  TSA_RawCheckProcess */
  TSA_RawCheckFrozen();
  iVar1 = TSA_RawIsFrozen();
  if ((iVar1 != 0) && (*_refptr_g_tsaLogLoglevel != '\0')) {
    uVar2 = TSA_RawCheckGetExceptionDescription();
    StrPrintf_Process(0,uVar2);
  }
  return;
}


// ============================================================================
// Function: TSA_UpdateIDLEModeInternally @ 6ba9e458
// ============================================================================

bool TSA_UpdateIDLEModeInternally(void)

{
  char cVar1;
  bool local_9;
  
                    /* 0x5e458  2337  TSA_UpdateIDLEModeInternally */
  cVar1 = TSAMS_IsOKToEnterIDLEMode();
  if (cVar1 == '\0') {
    *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c =
         *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c | 0x400;
  }
  local_9 = cVar1 != '\0';
  cVar1 = TSAStatic_DoesAFERequireToExitIDLEMode();
  if (cVar1 != '\0') {
    local_9 = false;
    *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c =
         *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c | 0x800;
  }
  cVar1 = Proximity_IsToExitIDLE();
  if (cVar1 != '\0') {
    local_9 = false;
    *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c =
         *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c | 0x1000;
  }
  cVar1 = TSAStatic_IsAFEEnterIdleTimeout();
  if (cVar1 != '\0') {
    local_9 = false;
    *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c =
         *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c | 0x2000;
  }
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) != 0) {
    cVar1 = TSA_DelayToIdleModeHPPx();
    if (cVar1 != '\0') {
      local_9 = false;
      *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c =
           *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c | 0x4000000;
    }
  }
  if (local_9 == true) {
    if (*(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x38 == -1) {
      *(void **)&(*_refptr_g_tsaStaticPtr)->field_0x38 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
      local_9 = false;
      *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c =
           *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c | 0x4000;
    }
    else if ((*_refptr_g_tsaStaticPtr)->pFrameBufferPtr <
             (void *)((ulonglong)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x3c +
                     *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x38)) {
      local_9 = false;
      *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c =
           *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c | 0x8000;
    }
  }
  else {
    *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x38 = 0xffffffffffffffff;
  }
  return local_9;
}


// ============================================================================
// Function: TSA_UpdateSideIDLEModeInternally @ 6ba9e663
// ============================================================================

bool TSA_UpdateSideIDLEModeInternally(void)

{
  char cVar1;
  bool local_9;
  
                    /* 0x5e663  2338  TSA_UpdateSideIDLEModeInternally */
  local_9 = (*_refptr_g_tsaStaticPtr)->field_0x7 == '\0';
  if (local_9) {
    *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c =
         *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c | 0x10;
  }
  local_9 = !local_9;
  cVar1 = SideTouch_IsOKToEnterIDLEMode();
  if (cVar1 == '\0') {
    local_9 = false;
    *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c =
         *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c | 0x20;
  }
  cVar1 = TSAStatic_DoesAFERequireToExitSideIDLEMode();
  if (cVar1 != '\0') {
    local_9 = false;
    *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c =
         *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c | 0x40;
  }
  cVar1 = TSAStatic_IsAFEEnterSideIdleTimeout();
  if (cVar1 != '\0') {
    local_9 = false;
    *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c =
         *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c | 0x80;
  }
  return local_9;
}


// ============================================================================
// Function: TSA_IsOKToEnterIDLEMode @ 6ba9e755
// ============================================================================

undefined1 TSA_IsOKToEnterIDLEMode(void)

{
  char cVar1;
  undefined1 uVar2;
  undefined1 local_9;
  
                    /* 0x5e755  2078  TSA_IsOKToEnterIDLEMode */
  if ((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) != 0) &&
     (cVar1 = TSA_InStylusMode(), cVar1 != '\0')) {
    return 0;
  }
  cVar1 = AFE_IsInAutoCheckProcess();
  if (cVar1 == '\0') {
    if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) == 0) {
      uVar2 = TSAStatic_IsOKToEnterIDLEMode();
    }
    else {
      uVar2 = TSAStatic_IsOKToEnterSideIDLEMode();
    }
  }
  else {
    uVar2 = AFE_AutoCheckToEnterIDLE();
  }
  return uVar2;
}


// ============================================================================
// Function: TSA_IsScreenLockScenario @ 6ba8fb7c
// ============================================================================

undefined8 TSA_IsScreenLockScenario(void)

{
  undefined8 uVar1;
  
                    /* 0x4fb7c  2093  TSA_IsScreenLockScenario */
  if ((((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x400000) == 0) &&
     (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x800000) != 0)) {
    if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x4000) == 0) {
      if ((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x5e0 & 0x180) == 0) {
        uVar1 = 1;
      }
      else {
        uVar1 = 0;
      }
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
// Function: TSA_GetDim1Res @ 6ba9aecf
// ============================================================================

undefined2 TSA_GetDim1Res(void)

{
                    /* 0x5aecf  1918  TSA_GetDim1Res */
  return *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a6;
}


// ============================================================================
// Function: TSA_GetDim2Res @ 6ba9aee3
// ============================================================================

undefined2 TSA_GetDim2Res(void)

{
                    /* 0x5aee3  1922  TSA_GetDim2Res */
  return *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a8;
}


// ============================================================================
// Function: TSA_ASAAFEStatus @ 6ba9c607
// ============================================================================

undefined4 TSA_ASAAFEStatus(void)

{
  undefined4 uVar1;
  longlong lVar2;
  
                    /* 0x5c607  1732  TSA_ASAAFEStatus */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) == 0) {
    uVar1 = 0;
  }
  else {
    lVar2 = AFE_GetFrame();
    if (*(longlong *)(lVar2 + 0x40) == 0) {
      uVar1 = 0xfffffff6;
    }
    else {
      uVar1 = *(undefined4 *)(*(longlong *)(lVar2 + 0x40) + 0x1c);
    }
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_RptASAXPos @ 6ba9c84f
// ============================================================================

undefined8 TSA_RptASAXPos(void)

{
  undefined8 uVar1;
  
                    /* 0x5c84f  2157  TSA_RptASAXPos */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) == 0) {
    uVar1 = 0;
  }
  else {
    uVar1 = ASA_GetRptXPos();
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_RptASAYPos @ 6ba9c882
// ============================================================================

undefined8 TSA_RptASAYPos(void)

{
  undefined8 uVar1;
  
                    /* 0x5c882  2160  TSA_RptASAYPos */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) == 0) {
    uVar1 = 0;
  }
  else {
    uVar1 = ASA_GetRptYPos();
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_RptASAXTilt @ 6ba9cae5
// ============================================================================

undefined8 TSA_RptASAXTilt(void)

{
  undefined8 uVar1;
  
                    /* 0x5cae5  2159  TSA_RptASAXTilt */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) == 0) {
    uVar1 = 0;
  }
  else {
    uVar1 = ASA_GetRptXTilt();
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_RptASAYTilt @ 6ba9cb18
// ============================================================================

undefined8 TSA_RptASAYTilt(void)

{
  undefined8 uVar1;
  
                    /* 0x5cb18  2162  TSA_RptASAYTilt */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) == 0) {
    uVar1 = 0;
  }
  else {
    uVar1 = ASA_GetRptYTilt();
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_RptASAPressure @ 6ba9cb4b
// ============================================================================

undefined8 TSA_RptASAPressure(void)

{
  undefined8 uVar1;
  
                    /* 0x5cb4b  2152  TSA_RptASAPressure */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) == 0) {
    uVar1 = 0;
  }
  else {
    uVar1 = ASA_GetRptPressure();
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_GetASADim1Res @ 6ba9d3e0
// ============================================================================

ulonglong TSA_GetASADim1Res(void)

{
  ulonglong uVar1;
  
                    /* 0x5d3e0  1883  TSA_GetASADim1Res */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) == 0) {
    uVar1 = (ulonglong)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a6;
  }
  else {
    uVar1 = ASA_GetDim1Res();
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_GetASADim2Res @ 6ba9d41c
// ============================================================================

ulonglong TSA_GetASADim2Res(void)

{
  ulonglong uVar1;
  
                    /* 0x5d41c  1884  TSA_GetASADim2Res */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) == 0) {
    uVar1 = (ulonglong)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a8;
  }
  else {
    uVar1 = ASA_GetDim2Res();
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_ReloadASAPrmtByScanState @ 6bb26d3f
// ============================================================================

void TSA_ReloadASAPrmtByScanState(uint param_1)

{
  TSAFlashPrmt *pTVar1;
  TSAFlashPrmt *pTVar2;
  ushort uVar3;
  ushort uVar4;
  ushort uVar5;
  undefined8 uVar6;
  short local_38;
  short sStack_36;
  short sStack_34;
  short sStack_32;
  
  pTVar2 = _refptr_g_tsaPrmtFlash;
  pTVar1 = _refptr_g_tsaPrmtFlash + 2;
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) != 0) {
    uVar3 = ASA_GetDim1PhyRes();
    uVar4 = TSA_GetDim1PhyRes();
    uVar5 = uVar3 / uVar4;
    ASA_SetDim1Len((&pTVar2[2].field_0x758)[(ulonglong)param_1 * 0x14],uVar3 % uVar4);
    ASA_SetDim2Len((&pTVar2[2].field_0x759)[(ulonglong)param_1 * 0x14]);
    ASA_SetDim1Ofst((&pTVar2[2].field_0x75a)[(ulonglong)param_1 * 0x14]);
    ASA_SetDim2Ofst((&pTVar2[2].field_0x75b)[(ulonglong)param_1 * 0x14]);
    ASA_SetXRes((uVar5 & 0xff) * *(short *)(&pTVar2[2].field_0x75c + (ulonglong)param_1 * 0x14));
    ASA_SetYRes((uVar5 & 0xff) * *(short *)(&pTVar2[2].field_0x75e + (ulonglong)param_1 * 0x14));
    uVar6 = RefreshClipPrmt(&pTVar1->field_0x750 + (ulonglong)param_1 * 0x14);
    local_38 = (short)uVar6;
    ASA_SetXResClipLow((int)(short)((uVar5 & 0xff) * local_38));
    sStack_36 = (short)((ulonglong)uVar6 >> 0x10);
    ASA_SetXResClipHigh((int)(short)((uVar5 & 0xff) * sStack_36));
    sStack_34 = (short)((ulonglong)uVar6 >> 0x20);
    ASA_SetYResClipLow((int)(short)((uVar5 & 0xff) * sStack_34));
    sStack_32 = (short)((ulonglong)uVar6 >> 0x30);
    ASA_SetYResClipHigh((int)(short)((uVar5 & 0xff) * sStack_32));
    ASA_SetXResOfst((uVar5 & 0xff) * *(short *)(&pTVar2[2].field_0x760 + (ulonglong)param_1 * 0x14))
    ;
    ASA_SetYResOfst((uVar5 & 0xff) * *(short *)(&pTVar2[2].field_0x762 + (ulonglong)param_1 * 0x14))
    ;
    ASA_SetFoldState(param_1 & 0xff,
                     *(undefined4 *)(&pTVar1->field_0x750 + (ulonglong)param_1 * 0x14),
                     *(undefined4 *)(&pTVar2[2].field_0x754 + (ulonglong)param_1 * 0x14));
    ASA_Reset();
  }
  return;
}


// ============================================================================
// Function: TSA_SetXPos @ 6ba6a355
// ============================================================================

void TSA_SetXPos(byte param_1,undefined2 param_2)

{
  longlong lVar1;
  undefined2 uVar2;
  
                    /* 0x2a355  2315  TSA_SetXPos */
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    lVar1 = *(longlong *)_refptr_g_touchesPtr;
    uVar2 = TSAOut_RevertDim1Pos(param_2);
    *(undefined2 *)((longlong)(int)(uint)param_1 * 0x168 + lVar1 + 0x170) = uVar2;
  }
  else {
    lVar1 = *(longlong *)_refptr_g_touchesPtr;
    uVar2 = TSAOut_RevertDim2Pos(param_2);
    *(undefined2 *)((longlong)(int)(uint)param_1 * 0x168 + lVar1 + 0x172) = uVar2;
  }
  return;
}


// ============================================================================
// Function: TSA_IsHighRateGameScenario @ 6ba8feab
// ============================================================================

bool TSA_IsHighRateGameScenario(void)

{
  char cVar1;
  bool bVar2;
  
                    /* 0x4feab  2070  TSA_IsHighRateGameScenario */
  cVar1 = TSA_IsTypingScenario();
  if (cVar1 == '\0') {
    bVar2 = ((*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags & 4) != 0;
  }
  else {
    bVar2 = false;
  }
  return bVar2;
}


// ============================================================================
// Function: TSA_IsNormalRateGameScenario @ 6ba8fee1
// ============================================================================

bool TSA_IsNormalRateGameScenario(void)

{
  char cVar1;
  bool bVar2;
  
                    /* 0x4fee1  2076  TSA_IsNormalRateGameScenario */
  cVar1 = TSA_IsTypingScenario();
  if (cVar1 == '\0') {
    bVar2 = ((*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags & 0x40) != 0;
  }
  else {
    bVar2 = false;
  }
  return bVar2;
}


// ============================================================================
// Function: TSA_IsPossibleUnderWater @ 6baa082b
// ============================================================================

bool TSA_IsPossibleUnderWater(void)

{
  ushort uVar1;
  
                    /* 0x6082b  2087  TSA_IsPossibleUnderWater */
  uVar1 = _refptr_g_tsaPrmtFlash->wBufElementCount >> 3;
  return (int)((uint)uVar1 * 8 - (uint)uVar1) < (int)(uint)(ushort)g_tsaPrpt._188_2_;
}


// ============================================================================
// Function: TSA_IsUnderWaterDetected @ 6ba9d7e5
// ============================================================================

uint TSA_IsUnderWaterDetected(void)

{
                    /* 0x5d7e5  2105  TSA_IsUnderWaterDetected */
  return *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x20) & 0x2000;
}


// ============================================================================
// Function: TSA_IsOutOfWaterDetected @ 6ba9d7fd
// ============================================================================

uint TSA_IsOutOfWaterDetected(void)

{
                    /* 0x5d7fd  2082  TSA_IsOutOfWaterDetected */
  return *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x20) & 0x4000;
}


// ============================================================================
// Function: TSA_IsWindowsPadFeatureEnabled @ 6ba9ef4b
// ============================================================================

undefined4 TSA_IsWindowsPadFeatureEnabled(void)

{
  TSA_FeatureFlags TVar1;
  
                    /* 0x5ef4b  2112  TSA_IsWindowsPadFeatureEnabled */
  TVar1 = (*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x1000000;
  return CONCAT31((int3)(TVar1 >> 8),TVar1 != 0);
}


// ============================================================================
// Function: TSA_MSInitCommon @ 6ba90830
// ============================================================================

void TSA_MSInitCommon(char param_1)

{
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&_rdata,"TSA_MSInitCommon");
  }
  TSAPrmt_Reset();
  PrevTouch_Init();
  Touch_Init();
  TSABuffer_Init();
  PrevPeak_Init();
  TSAStatic_Init();
  BLSM_Init(param_1);
  CMF_Init(param_1);
  TPSensor_Init();
  TZ_Init();
  CTD_ECInit();
  TSAPrpt_Init();
  TE_Init();
  IDT_Init();
  ER_Init();
  TS_Init();
  TouchReport_Init();
  TouchAction_Init();
  TouchMode_Init();
  Exception_Init();
  RxLineFilter_Init();
  SafeBaseline_Init(param_1);
  AntiTouch_Init();
  HardwareAnalyzer_Init();
  SignalDisparity_Init();
  TSA_GridDataInit();
  Gesture_Init();
  GripFilter_Init();
  SensorData_Init();
  BadChannel_Init();
  BaliSensorBroken_Init();
  Proximity_Init();
  SensorGesture_Init();
  SideTouch_Init();
  SHBVolumeData_Init();
  HandGesture_Init(param_1);
  if (param_1 != '\0') {
    BLReset_FlagDisableBaselineUpdateInit();
  }
  Thenar_Init();
  BigData_Init();
  DoubleReport_Init();
  return;
}


// ============================================================================
// Function: TSA_GetRawPrpt @ 6baa08ab
// ============================================================================

void TSA_GetRawPrpt(longlong param_1,longlong param_2)

{
  byte bVar1;
  byte bVar2;
  ushort uVar3;
  bool bVar4;
  undefined2 uVar5;
  uint uVar6;
  union {
    uint64_t u64;
    uint32_t u32[2];
    uint16_t u16[4];
    struct {
      uint16_t _0_2_;
      uint16_t _2_2_;
      uint16_t _4_2_;
      uint16_t _6_2_;
    };
    struct {
      uint32_t _0_4_;
      uint32_t _4_4_;
    };
  } local_48;
  undefined2 local_40;
  undefined2 uStack_3e;
  union {
    uint16_t u16;
    struct {
      uint8_t _0_1_;
      uint8_t _1_1_;
    };
  } local_3c;
  int local_18;
  byte local_12;
  byte local_11;
  uint local_10;
  uint local_c;
  
  uStack_3e = 0;
  local_3c.u16 = 0;
  local_48._0_2_ = 0;
  local_48._2_2_ = 0xffff;
  local_48._4_4_ = 0xffff0000;
  local_c = 0;
  local_10 = 0;
  local_18 = 0;
  bVar1 = _refptr_g_tsaPrmtFlash->bCols;
  bVar2 = _refptr_g_tsaPrmtFlash->bRows;
  for (local_12 = 0; (uint)local_12 < (uint)bVar2; local_12 = local_12 + 1) {
    bVar4 = false;
    if ((local_12 == 0) || ((uint)local_12 == bVar2 - 1)) {
      bVar4 = true;
    }
    for (local_11 = 0; (uint)local_11 < (uint)bVar1; local_11 = local_11 + 1) {
      uVar3 = *(ushort *)(param_1 + (longlong)local_18 * 2);
      uVar6 = (uint)uVar3;
      if (local_48._0_2_ < uVar6) {
        local_48._0_2_ = uVar3;
        uStack_3e = CONCAT11(local_12,local_11);
      }
      else if (uVar6 < local_48._2_2_) {
        local_48._2_2_ = uVar3;
        local_3c._1_1_ = local_12;
        local_3c._0_1_ = local_11;
      }
      if (((!bVar4) && (local_11 != 0)) && ((uint)local_11 != bVar1 - 1)) {
        local_10 = local_10 + uVar6;
        local_c = local_c + *(ushort *)(param_2 + (longlong)local_18 * 2);
        uVar5 = MAX(uVar6,local_48._4_2_);
        local_48._4_2_ = uVar5;
        local_48._6_2_ = MIN(uVar6,local_48._6_2_);
      }
      local_18 = local_18 + 1;
    }
  }
  uVar6 = (_refptr_g_tsaPrmtFlash->bCols - 2) * (_refptr_g_tsaPrmtFlash->bRows - 2);
  *(short*)((char*)&g_tsaPrpt + 666) = (short)(local_c / uVar6);
  local_40 = (short)(local_10 / uVar6);
  g_tsaPrpt.wBoundaryPositive = local_48._0_2_;
  g_tsaPrpt.wBoundaryNegative = local_48._2_2_;
  *(uint32_t*)((char*)&g_tsaPrpt + 150) = local_48._4_4_;
  *(uint32_t*)((char*)&g_tsaPrpt + 154) = (uint32_t)(ushort)local_40 | ((uint32_t)(ushort)uStack_3e << 16);
  *(uint16_t*)((char*)&g_tsaPrpt + 158) = local_3c.u16;
  return;
}


// ============================================================================
// Function: TSA_GetDifPrpt @ 6baa121d
// ============================================================================

void TSA_GetDifPrpt(void)

{
  undefined4 local_10;
  undefined4 local_c;
  
  TSA_InitBlockPrpt();
  TSA_GetDifPrptCommon();
  TSA_CombineBlock((undefined8 *)TSA_MEM_AT(0x6bc4a700ULL),5);
  local_c = 0x7f;
  if ((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) != 0) &&
     (local_c = 0x7d, (*(uint *)&(*_refptr_g_tsaStaticPtr)->field_0xd8 & 0x40000) != 0)) {
    local_c = 0x55;
  }
  TSA_CombineBlock((undefined8 *)TSA_MEM_AT(0x6bc4a6e0ULL),local_c);
  TSA_CombineBlock((undefined8 *)TSA_MEM_AT(0x6bc4a720ULL),1);
  g_tsaPrpt._256_2_ = g_tsaPrpt.nSPeakPositive;
  g_tsaPrpt._258_2_ = g_tsaPrpt.nSPeakNegative;
  g_tsaPrpt._260_1_ = g_tsaPrpt.bSpecialThreshCol;
  g_tsaPrpt._261_1_ = g_tsaPrpt._165_1_;
  g_tsaPrpt._262_1_ = g_tsaPrpt.bNextSideThreshCol;
  g_tsaPrpt._263_1_ = g_tsaPrpt._167_1_;
  g_tsaPrpt._264_8_ = g_tsaPrpt._168_8_;
  g_tsaPrpt._272_8_ = g_tsaPrpt._176_8_;
  g_tsaPrpt._280_8_ = g_tsaPrpt._184_8_;
  local_10 = 2;
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) != 0) {
    local_10 = 0x1e;
  }
  TSA_CombineBlock((undefined8 *)TSA_MEM_AT(0x6bc4a780ULL),local_10);
  return;
}


// ============================================================================
// Function: TSA_IsGripOKToIDLE @ 6ba9e650
// ============================================================================

char TSA_IsGripOKToIDLE(void)
{
  return GripFilter_IsOKToEnterIDLE();
}


// ============================================================================
// Function: TSA_GetThenarSide @ 6ba9fb12
// ============================================================================

undefined4 TSA_GetThenarSide(void)

{
                    /* 0x5fb12  2001  TSA_GetThenarSide */
  return *(undefined4 *)(_refptr_g_tsaInitOnce + 4);
}


// ============================================================================
// Function: TSA_GetPrmtName @ 6ba9e911
// ============================================================================

char* TSA_GetPrmtName(void)
{
                    /* 0x5e911  1973  TSA_GetPrmtName */
  return (char *)TSAPrmt_GetProjectName();
}


// ============================================================================
// Function: TSA_IsPhoneCallScenario @ 6ba8fb38
// ============================================================================

undefined8 TSA_IsPhoneCallScenario(void)

{
  undefined8 uVar1;
  
                    /* 0x4fb38  2084  TSA_IsPhoneCallScenario */
  if ((((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x400000) == 0) ||
     (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x800000) != 0)) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_RawIsFrozen @ 6bafca30
// ============================================================================

undefined4 TSA_RawIsFrozen(void)

{
                    /* 0xbca30  2132  TSA_RawIsFrozen */
  return g_rawIsFrozen;
}


// ============================================================================
// Function: TSA_RawCheckFrozen @ 6bafca3c
// ============================================================================

void TSA_RawCheckFrozen(void)

{
  ushort local_a;
  
                    /* 0xbca3c  2129  TSA_RawCheckFrozen */
  local_a = 0;
  do {
    if (_refptr_g_tsaPrmtFlash->wBufElementCount <= local_a) {
LAB_6bafcabf:
      if (local_a == _refptr_g_tsaPrmtFlash->wBufElementCount) {
        g_rawDidNotChange = g_rawDidNotChange + 1;
      }
      if (9 < g_rawDidNotChange) {
        g_rawIsFrozen = 1;
        g_rawDidNotChange = 9;
      }
      return;
    }
    if (*(short *)(*(longlong *)_refptr_g_tsaUnnormalizedRawBuf + (ulonglong)local_a * 2) !=
        *(short *)(*(longlong *)_refptr_g_tsaBufPreUnnormalizedRaw + (ulonglong)local_a * 2)) {
      g_rawDidNotChange = 0;
      g_rawIsFrozen = 0;
      goto LAB_6bafcabf;
    }
    local_a = local_a + 1;
  } while( true );
}


// ============================================================================
// Function: TSA_RawCheckGetExceptionDescription @ 6bafcafe
// ============================================================================

char * TSA_RawCheckGetExceptionDescription(void)

{
                    /* 0xbcafe  2130  TSA_RawCheckGetExceptionDescription */
  return "\nThe input raw date did not change. Please check input!\n";
}


// ============================================================================
// Function: TSA_DelayToIdleModeHPPx @ 6ba9e3c6
// ============================================================================

undefined8 TSA_DelayToIdleModeHPPx(void)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0x5e3c6  1821  TSA_DelayToIdleModeHPPx */
  cVar1 = TSA_InStylusMode();
  if (cVar1 != '\0') {
    g_flagOKtoDelay3sToIdle = '\x01';
    g_lastStylusTimeStamp = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  }
  if ((g_flagOKtoDelay3sToIdle == '\0') ||
     (2999 < (ulonglong)
             ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - (longlong)g_lastStylusTimeStamp
             ))) {
    uVar2 = 0;
  }
  else {
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"IN STYLUS 3S DELAY TO IDLE!");
    }
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: TSA_InStylusMode @ 6ba9c771
// ============================================================================

undefined8 TSA_InStylusMode(void)

{
  bool bVar1;
  char cVar2;
  undefined8 uVar3;
  
                    /* 0x5c771  2037  TSA_InStylusMode */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) == 0) {
    uVar3 = 0;
  }
  else {
    bVar1 = ASA_IsEnterStylusHPP2();
    if ((bVar1) || (cVar2 = ASA_IsEnterStylusHPP3(), cVar2 != '\0')) {
      uVar3 = 1;
    }
    else {
      uVar3 = 0;
    }
  }
  return uVar3;
}


// ============================================================================
// Function: TSA_RptTouchDim1Center @ 6ba9b98a
// ============================================================================

ushort TSA_RptTouchDim1Center(byte param_1)

{
                    /* 0x5b98a  2208  TSA_RptTouchDim1Center */
  return *(ushort *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162) >> 8;
}


// ============================================================================
// Function: TSA_RptTouchDim2Center @ 6ba9b9bd
// ============================================================================

ushort TSA_RptTouchDim2Center(byte param_1)

{
                    /* 0x5b9bd  2210  TSA_RptTouchDim2Center */
  return *(ushort *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164) >> 8;
}


// ============================================================================
// Function: TSA_RptVitualKeyEventNum @ 6ba9fc37
// ============================================================================

void TSA_RptVitualKeyEventNum(void)

{
                    /* 0x5fc37  2250  TSA_RptVitualKeyEventNum */
  VirtualKey_GetEventNum();
  return;
}


// ============================================================================
// Function: TSA_RptVirtualKeyCode @ 6ba9fc4a
// ============================================================================

void TSA_RptVirtualKeyCode(undefined1 param_1)

{
                    /* 0x5fc4a  2249  TSA_RptVirtualKeyCode */
  VirtualKey_GetCode(param_1);
  return;
}


// ============================================================================
// Function: TSA_RptVirtualKeyAction @ 6ba9fc68
// ============================================================================

void TSA_RptVirtualKeyAction(undefined1 param_1)

{
                    /* 0x5fc68  2248  TSA_RptVirtualKeyAction */
  VirtualKey_GetAction(param_1);
  return;
}


// ============================================================================
// Function: TSA_IsVolumeControlEnabled @ 6ba9fdf0
// ============================================================================

void TSA_IsVolumeControlEnabled(void)

{
                    /* 0x5fdf0  2108  TSA_IsVolumeControlEnabled */
  SideTouch_IsVolumeControlEnabled();
  return;
}


// ============================================================================
// Function: TSA_GetDim1PhyRes @ 6ba9aea7
// ============================================================================

undefined2 TSA_GetDim1PhyRes(void)

{
                    /* 0x5aea7  1916  TSA_GetDim1PhyRes */
  return *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5aa;
}


// ============================================================================
// Function: TSA_RptLONum @ 6ba9b0eb
// ============================================================================

undefined1 TSA_RptLONum(void)

{
                    /* 0x5b0eb  2181  TSA_RptLONum */
  return *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 3);
}


// ============================================================================
// Function: TSA_GetDMDType @ 6ba9da17
// ============================================================================

int TSA_GetDMDType(void)

{
                    /* 0x5da17  1914  TSA_GetDMDType */
  return (*_refptr_g_tsaStaticPtr)->nField_0x138;
}


// ============================================================================
// Function: TSA_GetExceptionDescription @ 6ba9e284
// ============================================================================

void TSA_GetExceptionDescription(void)

{
                    /* 0x5e284  1931  TSA_GetExceptionDescription */
  TSA_GetMSExceptionDescription();
  return;
}


// ============================================================================
// Function: TSA_IsOKToEnterSideIDLEMode @ 6ba9e742
// ============================================================================

void TSA_IsOKToEnterSideIDLEMode(void)

{
                    /* 0x5e742  2079  TSA_IsOKToEnterSideIDLEMode */
  TSAStatic_IsOKToEnterSideIDLEMode();
  return;
}


// ============================================================================
// Function: TSA_GetIDLETouchThold @ 6ba9e86d
// ============================================================================

void TSA_GetIDLETouchThold(void)

{
                    /* 0x5e86d  1945  TSA_GetIDLETouchThold */
  TSA_GetMutualTouchTholdInIDLEMode();
  return;
}


// ============================================================================
// Function: TSA_GetTouchMode @ 6ba9e924
// ============================================================================

void TSA_GetTouchMode(void)

{
                    /* 0x5e924  2005  TSA_GetTouchMode */
  TSAMS_GetTouchMode();
  return;
}


// ============================================================================
// Function: TSA_GetTouchThold @ 6ba9e95d
// ============================================================================

void TSA_GetTouchThold(void)

{
                    /* 0x5e95d  2011  TSA_GetTouchThold */
  TSAMS_GetTouchThold();
  return;
}


// ============================================================================
// Function: TSA_GetBaselineThold @ 6ba9e970
// ============================================================================

undefined2 TSA_GetBaselineThold(void)

{
                    /* 0x5e970  1907  TSA_GetBaselineThold */
  return *(undefined2 *)(_refptr_g_tsaPrmtDynamic + 0x42);
}


// ============================================================================
// Function: TSA_GetException @ 6ba9ea62
// ============================================================================

undefined4 TSA_GetException(void)

{
                    /* 0x5ea62  1930  TSA_GetException */
  return *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x20);
}


// ============================================================================
// Function: TSA_GetMutualRawMax @ 6ba9ea75
// ============================================================================

ushort TSA_GetMutualRawMax(void)

{
                    /* 0x5ea75  1964  TSA_GetMutualRawMax */
  return _refptr_g_tsaPrpt->wBoundaryPositive;
}


// ============================================================================
// Function: TSA_GetMutualRawMin @ 6ba9ea8c
// ============================================================================

ushort TSA_GetMutualRawMin(void)

{
                    /* 0x5ea8c  1965  TSA_GetMutualRawMin */
  return _refptr_g_tsaPrpt->wBoundaryNegative;
}


// ============================================================================
// Function: TSA_GetMutualDifMax @ 6ba9eaa3
// ============================================================================

int TSA_GetMutualDifMax(void)

{
                    /* 0x5eaa3  1962  TSA_GetMutualDifMax */
  return (int)_refptr_g_tsaPrpt->nSPeakPositive;
}


// ============================================================================
// Function: TSA_GetMutualDifMin @ 6ba9eab8
// ============================================================================

int TSA_GetMutualDifMin(void)

{
                    /* 0x5eab8  1963  TSA_GetMutualDifMin */
  return (int)_refptr_g_tsaPrpt->nSPeakNegative;
}


// ============================================================================
// Function: TSA_GetState @ 6ba9f659
// ============================================================================

undefined4 TSA_GetState(void)

{
                    /* 0x5f659  1992  TSA_GetState */
  return *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x27c;
}


// ============================================================================
// Function: TSA_GetEvent @ 6ba9f66f
// ============================================================================

int TSA_GetEvent(void)

{
                    /* 0x5f66f  1928  TSA_GetEvent */
  return (*_refptr_g_tsaStaticPtr)->nField_0x280;
}


// ============================================================================
// Function: TSA_IsHangingPosture @ 6ba9f7ee
// ============================================================================

void TSA_IsHangingPosture(void)

{
                    /* 0x5f7ee  2069  TSA_IsHangingPosture */
  Posture_IsHangingPosture();
  return;
}


// ============================================================================
// Function: TSA_GetPosture @ 6ba9f801
// ============================================================================

void TSA_GetPosture(void)

{
                    /* 0x5f801  1972  TSA_GetPosture */
  Posture_GetPosture();
  return;
}


// ============================================================================
// Function: TSA_IsTypingScenario @ 6ba8fe17
// ============================================================================

bool TSA_IsTypingScenario(void)

{
                    /* 0x4fe17  2103  TSA_IsTypingScenario */
  return ((*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags & 2) != 0;
}


// ============================================================================
// Function: TSA_GetXRes @ 6ba9aef7
// ============================================================================

ushort TSA_GetXRes(void)
{
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    return TSA_GetDim1Res();
  } else {
    return TSA_GetDim2Res();
  }
}


// ============================================================================
// Function: TSA_GetYRes @ 6ba9af2b
// ============================================================================

ushort TSA_GetYRes(void)
{
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    return TSA_GetDim2Res();
  } else {
    return TSA_GetDim1Res();
  }
}


// ============================================================================
// Function: TSA_GridDataInit @ 6ba98f53
// ============================================================================

void TSA_GridDataInit(void)

{
  undefined8 uVar1;
  int local_c;
  
  for (local_c = 0; local_c < 0x5b; local_c = local_c + 1) {
    switch(local_c) {
    case 0:
      TSA_GridDataPreUnifiedRawInit(local_c);
      break;
    case 1:
      TSA_GridDataPreRawInit(local_c);
      break;
    case 2:
      TSA_GridDataRawInit(local_c);
      break;
    case 3:
      TSA_GridDataPreUnifiedDifInit(local_c);
      break;
    case 4:
      TSA_GridDataPreHWRawInit(local_c);
      break;
    case 5:
      TSA_GridDataRaw2RawDiffInit(local_c);
      break;
    case 6:
      TSA_GridDataRaw2RawDiff2Init(local_c);
      break;
    case 7:
      TSA_GridDataRaw2RawDiff3Init(local_c);
      break;
    case 8:
      TSA_GridDataRaw2RawDiffCmfInit(local_c);
      break;
    case 9:
      TSA_GridDataPreCMFDiffInit(local_c);
      break;
    case 10:
      TSA_GridDataAftSSCMFDiffInit(local_c);
      break;
    case 0xb:
      TSA_GridDataAftSSChargerNFilterDiffInit(local_c);
      break;
    case 0xc:
      TSA_GridDataAftPcaFilterDifInit(local_c);
      break;
    case 0xd:
      TSA_GridDataAftCMFDiffInit(local_c);
      break;
    case 0xe:
      TSA_GridDataCommonModeNoiseInit(local_c);
      break;
    case 0xf:
      TSA_GridDataCmfDraftNeighborDistanceInit(local_c);
      break;
    case 0x10:
      TSA_GridDataCmfDraftWeightInit(local_c);
      break;
    case 0x11:
      TSA_GridDataCmfNeighborDistanceInit(local_c);
      break;
    case 0x12:
      TSA_GridDataCmfWeightInit(local_c);
      break;
    case 0x13:
      TSA_GridDataAftDim1CMFDiffInit(local_c);
      break;
    case 0x14:
      TSA_GridDataOneOrderDiffRxInit(local_c);
      break;
    case 0x15:
      TSA_GridDataTwoOrderDiffRxInit(local_c);
      break;
    case 0x16:
      TSA_GridDataGridIIRDifInit(local_c);
      break;
    case 0x17:
      TSA_GridDataPreNotchCorrectDiffInit(local_c);
      break;
    case 0x18:
      TSA_GridDataAftNotchCorrectDiffInit(local_c);
      break;
    case 0x19:
      TSA_GridDataPreDiffInit(local_c);
      break;
    case 0x1a:
      TSA_GridDataDiffInit(local_c);
      break;
    case 0x1b:
      TSA_GridDataSDInit(local_c);
      break;
    case 0x1c:
      TSA_GridDataBaselineInit(local_c);
      break;
    case 0x1d:
      TSA_GridDataNormalBaselineInit(local_c);
      break;
    case 0x1e:
      TSA_GridDataCMFRemapInit(local_c);
      break;
    case 0x1f:
      TSA_GridDataCMFRemapBackInit(local_c);
      break;
    case 0x20:
      TSA_GridDataTZFlagInit(local_c);
      break;
    case 0x21:
      TSA_GridDataTZIDInit(local_c);
      break;
    case 0x22:
      TSA_GridDataTZZoneInit(local_c);
      break;
    case 0x23:
      TSA_GridDataSubTZInit(local_c);
      break;
    case 0x24:
      TSA_GridDataGripStageInit(local_c);
      break;
    case 0x25:
      TSA_GridDataGripTimeInit(local_c);
      break;
    case 0x26:
      TSA_GridDataGripTimeOffInit(local_c);
      break;
    case 0x27:
      TSA_GridDataGripRegionInit(local_c);
      break;
    case 0x28:
      TSA_GridDataDistanceMapInit(local_c);
      break;
    case 0x29:
      TSA_GridDataAfterDilateInit(local_c);
      break;
    case 0x2a:
      TSA_GridDataAfterErodeInit(local_c);
      break;
    case 0x2b:
      TSA_GridDataProxDifInit(local_c);
      break;
    case 0x2c:
      TSA_GridDataProxIIRRawInit(local_c);
      break;
    case 0x2d:
      TSA_GridDataProxZnDifInit(local_c);
      break;
    case 0x2e:
      TSA_GridDataProxBaselineInit(local_c);
      break;
    case 0x2f:
      TSA_GridDataProxTZInit(local_c);
      break;
    case 0x30:
      TSA_GridDataSideTZInit(local_c);
      break;
    case 0x31:
      TSA_GridDataSideTouchInit(local_c);
      break;
    case 0x32:
      TSA_GridDataSafeBaselineInit(local_c);
      break;
    case 0x33:
      TSA_GridDataDifSafeBL2RawInit(local_c);
      break;
    case 0x34:
      TSA_GridDataDifSafeBL2BLInit(local_c);
      break;
    case 0x35:
      TSA_GridDataDifSafeBL2RawCMFInit(local_c);
      break;
    case 0x36:
      TSA_GridDataDifSafeBL2BLCMFInit(local_c);
      break;
    case 0x37:
      TSA_GridDataTouchOnTimeInit(local_c);
      break;
    case 0x38:
      TSA_GridDataTouchRptTimeInit(local_c);
      break;
    case 0x39:
      TSA_GridDataMaxSigDownInit(local_c);
      break;
    case 0x3a:
      TSA_GridDataTouchOffTimeInit(local_c);
      break;
    case 0x3b:
      TSA_GridDataMinSigOffInit(local_c);
      break;
    case 0x3c:
      TSA_GridDataSilentTouchNumInit(local_c);
      break;
    case 0x3d:
      TSA_GridDataHandGestureBaselineInit(local_c);
      break;
    case 0x3e:
      TSA_GridDataHandGestureDifInit(local_c);
      break;
    case 0x3f:
      TSA_GridDataHandGestureAreaInit(local_c);
      break;
    case 0x40:
      TSA_GridDataHandGestureTZInit(local_c);
      break;
    case 0x41:
      TSA_GridDataHandGestureTZPrevInit(local_c);
      break;
    case 0x42:
      TSA_GridDataHandGestureTimeInit(local_c);
      break;
    case 0x43:
      TSA_GridDatatzAgeInit(local_c);
      break;
    case 0x44:
      TSA_GridDatatzOriTx1(local_c);
      break;
    case 0x45:
      TSA_GridDatatzOriTx2(local_c);
      break;
    case 0x46:
      TSA_GridDataPeakFlagTx1(local_c);
      break;
    case 0x47:
      TSA_GridDataPeakFlagTx2(local_c);
      break;
    case 0x48:
      TSA_GridNopressTable(local_c);
      break;
    case 0x49:
      TSA_GridNopressTableShortTerm(local_c);
      break;
    case 0x4a:
      TSA_GridNopressTableTx2(local_c);
      break;
    case 0x4b:
      TSA_GridNopressTableTx2ShortTerm(local_c);
      break;
    case 0x4c:
      TSA_GridNopressSafeTable(local_c);
      break;
    case 0x4d:
      uVar1 = ASA_GetGridTiedHandPtr(0);
      TSA_GridTiedGridUpdate(local_c,uVar1,"TiedGridHandDim1");
      break;
    case 0x4e:
      uVar1 = ASA_GetGridTiedHandPtr(1);
      TSA_GridTiedGridUpdate(local_c,uVar1,"TiedGridHandDim2");
      break;
    case 0x4f:
      uVar1 = ASA_GetGridTiedOriTx1Ptr(0);
      TSA_GridTiedGridUpdate(local_c,uVar1,"TiedGridOriTx1Dim1");
      break;
    case 0x50:
      uVar1 = ASA_GetGridTiedOriTx1Ptr(1);
      TSA_GridTiedGridUpdate(local_c,uVar1,"TiedGridOriTx1Dim2");
      break;
    case 0x51:
      uVar1 = ASA_GetGridTiedOriTx2Ptr(0);
      TSA_GridTiedGridUpdate(local_c,uVar1,"TiedGridOriTx2Dim1");
      break;
    case 0x52:
      uVar1 = ASA_GetGridTiedOriTx2Ptr(1);
      TSA_GridTiedGridUpdate(local_c,uVar1,"TiedGridOriTx2Dim2");
      break;
    case 0x53:
      uVar1 = ASA_GetGridTiedDeTraceNoiseTx1Ptr(0);
      TSA_GridTiedGridUpdate(local_c,uVar1,"TiedGridTraceDenoiseTx1Dim1");
      break;
    case 0x54:
      uVar1 = ASA_GetGridTiedDeTraceNoiseTx1Ptr(1);
      TSA_GridTiedGridUpdate(local_c,uVar1,"TiedGridTraceDenoiseTx1Dim2");
      break;
    case 0x55:
      uVar1 = ASA_GetGridTiedDeTraceNoiseTx2Ptr(0);
      TSA_GridTiedGridUpdate(local_c,uVar1,"TiedGridTraceDenoiseTx2Dim1");
      break;
    case 0x56:
      uVar1 = ASA_GetGridTiedDeTraceNoiseTx2Ptr(1);
      TSA_GridTiedGridUpdate(local_c,uVar1,"TiedGridTraceDenoiseTx2Dim2");
      break;
    case 0x57:
      uVar1 = ASA_GetGridTiedDePalmNoiseTx1Ptr(0);
      TSA_GridTiedGridUpdate(local_c,uVar1,"TiedGridPalmDenoiseTx1Dim1");
      break;
    case 0x58:
      uVar1 = ASA_GetGridTiedDePalmNoiseTx1Ptr(1);
      TSA_GridTiedGridUpdate(local_c,uVar1,"TiedGridPalmDenoiseTx1Dim2");
      break;
    case 0x59:
      uVar1 = ASA_GetGridTiedDePalmNoiseTx2Ptr(0);
      TSA_GridTiedGridUpdate(local_c,uVar1,"TiedGridPalmDenoiseTx2Dim1");
      break;
    case 0x5a:
      uVar1 = ASA_GetGridTiedDePalmNoiseTx2Ptr(1);
      TSA_GridTiedGridUpdate(local_c,uVar1,"TiedGridPalmDenoiseTx2Dim2");
      break;
    default:
      TSA_GridDataUnknownInit(local_c);
    }
  }
  return;
}


// ============================================================================
// Function: TSA_InitBlockPrpt @ 6baa0ac9
// ============================================================================

void TSA_InitBlockPrpt(void)

{
  longlong lVar1;
  uint local_c;
  
  for (local_c = 0; local_c < 7; local_c = local_c + 1) {
    lVar1 = (ulonglong)local_c * 0x1c;
    *(undefined2 *)(&g_tsaDifBlockPrpt + lVar1) = 0x8000;
    *(undefined2 *)(&DAT_6bc4a982 + lVar1) = 0x7fff;
    (&DAT_6bc4a984)[lVar1] = 0;
    (&DAT_6bc4a985)[lVar1] = 0;
    (&DAT_6bc4a986)[lVar1] = 0;
    (&DAT_6bc4a987)[lVar1] = 0;
    *(undefined2 *)(&DAT_6bc4a988 + lVar1) = 0;
    *(undefined2 *)(&DAT_6bc4a98a + lVar1) = 0;
    *(undefined4 *)(&DAT_6bc4a98c + lVar1) = 0;
    *(undefined4 *)(&DAT_6bc4a990 + lVar1) = 0;
    (&DAT_6bc4a994)[(ulonglong)local_c * 0xe] = 0;
  }
  return;
}


// ============================================================================
// ============================================================================
// Function: TSA_CombineBlock @ 6baa0da1
// ============================================================================

typedef union {
  uint64_t u64;
  uint8_t  u8[8];
  uint16_t u16[4];
  int16_t  s16[4];
  uint32_t u32[2];
  int32_t  s32[2];
} TsaBlockWord_t;

void TSA_CombineBlock(undefined8 *param_1,uint param_2)

{
  longlong lVar1;
  bool bVar2;
  int iVar5;
  short *psVar6;
  TsaBlockWord_t local_38;
  TsaBlockWord_t local_30;
  TsaBlockWord_t local_28;
  TsaBlockWord_t local_20;
  uint local_10;
  
  iVar5 = 0;
  local_28.u64 = 0;
  local_20.u64 = 0;
  local_30.u64 = 0;
  local_38.u64 = 0x7fff8000ULL;
  bVar2 = true;
  local_10 = 0;
  do {
    if (6 < local_10) {
      local_30.s32[0] = local_28.s32[0] + iVar5;
      local_30.s32[1] = iVar5;
      local_20.s16[0] = local_28.s16[2] + local_28.s16[3];
      param_1[0] = local_38.u64;
      param_1[1] = local_30.u64;
      param_1[2] = local_28.u64;
      param_1[3] = local_20.u64;
      return;
    }
    if ((1 << ((byte)local_10 & 0x1f) & param_2) != 0) {
      lVar1 = (ulonglong)local_10 * 0x1c;
      psVar6 = (short *)(&g_tsaDifBlockPrpt + lVar1);
      if (bVar2) {
        local_38.u8[4] = (&DAT_6bc4a984)[lVar1];
        local_38.u8[5] = (&DAT_6bc4a985)[lVar1];
        local_38.u8[6] = (&DAT_6bc4a986)[lVar1];
        local_38.u8[7] = (&DAT_6bc4a987)[lVar1];
        bVar2 = false;
      }
      if (local_38.s16[0] < *psVar6) {
LAB_6baa0edc:
        local_38.s16[0] = *psVar6;
        local_38.u8[4] = (&DAT_6bc4a984)[lVar1];
        local_38.u8[5] = (&DAT_6bc4a985)[lVar1];
      }
      else if (*psVar6 == local_38.s16[0]) {
        if ((ushort)((ushort)(byte)(&DAT_6bc4a985)[lVar1] * (ushort)_refptr_g_tsaPrmtFlash->bCols +
                    (ushort)(byte)(&DAT_6bc4a984)[lVar1]) <
            (ushort)((ushort)local_38.u8[4] +
                    (ushort)local_38.u8[5] * (ushort)_refptr_g_tsaPrmtFlash->bCols))
        goto LAB_6baa0edc;
      }
      if (*(short *)(&DAT_6bc4a982 + lVar1) < local_38.s16[1]) {
LAB_6baa0f6f:
        local_38.s16[1] = *(short *)(&DAT_6bc4a982 + lVar1);
        local_38.u8[6] = (&DAT_6bc4a986)[lVar1];
        local_38.u8[7] = (&DAT_6bc4a987)[lVar1];
      }
      else if (*(short *)(&DAT_6bc4a982 + lVar1) == local_38.s16[1]) {
        if ((ushort)((ushort)(byte)(&DAT_6bc4a987)[lVar1] * (ushort)_refptr_g_tsaPrmtFlash->bCols +
                    (ushort)(byte)(&DAT_6bc4a986)[lVar1]) <
            (ushort)((ushort)local_38.u8[6] +
                    (ushort)local_38.u8[7] * (ushort)_refptr_g_tsaPrmtFlash->bCols))
        goto LAB_6baa0f6f;
      }
      iVar5 += *(int *)(&DAT_6bc4a990 + lVar1);
      local_28.s32[0] += *(int *)(&DAT_6bc4a98c + lVar1);
      local_28.s16[2] += *(short *)(&DAT_6bc4a988 + lVar1);
      local_28.s16[3] += *(short *)(&DAT_6bc4a98a + lVar1);
      local_20.u16[1] += (ushort)(&DAT_6bc4a994)[(ulonglong)local_10 * 0xe];
    }
    local_10 = local_10 + 1;
  } while( true );
}


// Function: TSA_GetDifPrptCommon @ 6baa1035
// ============================================================================

void TSA_GetDifPrptCommon(void)

{
  byte bVar1;
  byte bVar2;
  short sVar3;
  longlong lVar4;
  short sVar5;
  short sVar6;
  longlong lVar7;
  byte *local_20;
  short *local_18;
  byte local_a;
  byte local_9;
  
  sVar5 = DAT_6bc4a910;
  sVar6 = -DAT_6bc4a910;
  bVar1 = _refptr_g_tsaPrmtFlash->bCols;
  bVar2 = _refptr_g_tsaPrmtFlash->bRows;
  local_18 = *(short **)_refptr_g_tsaBufDif;
  local_20 = g_tsaBlockIdx;
  for (local_a = 0; local_a < bVar2; local_a = local_a + 1) {
    for (local_9 = 0; local_9 < bVar1; local_9 = local_9 + 1) {
      sVar3 = *local_18;
      lVar7 = (longlong)(int)(uint)*local_20;
      lVar4 = lVar7 * 0x1c;
      if (*(short *)(&g_tsaDifBlockPrpt + lVar4) < sVar3) {
        *(short *)(&g_tsaDifBlockPrpt + lVar4) = sVar3;
        (&DAT_6bc4a984)[lVar4] = local_9;
        (&DAT_6bc4a985)[lVar4] = local_a;
      }
      if (sVar3 < *(short *)(&DAT_6bc4a982 + lVar4)) {
        *(short *)(&DAT_6bc4a982 + lVar4) = sVar3;
        (&DAT_6bc4a986)[lVar4] = local_9;
        (&DAT_6bc4a987)[lVar4] = local_a;
      }
      if (sVar3 < sVar5) {
        if (sVar3 <= sVar6) {
          *(int *)(&DAT_6bc4a98c + lVar4) = *(int *)(&DAT_6bc4a98c + lVar4) + (int)sVar3;
          *(short *)(&DAT_6bc4a98a + lVar4) = *(short *)(&DAT_6bc4a98a + lVar4) + 1;
        }
      }
      else {
        *(int *)(&DAT_6bc4a990 + lVar4) = *(int *)(&DAT_6bc4a990 + lVar4) + (int)sVar3;
        *(short *)(&DAT_6bc4a988 + lVar4) = *(short *)(&DAT_6bc4a988 + lVar4) + 1;
      }
      if ((100 < sVar3) || (sVar3 < -100)) {
        (&DAT_6bc4a994)[lVar7 * 0xe] = (&DAT_6bc4a994)[lVar7 * 0xe] + 1;
      }
      local_20 = local_20 + 1;
      local_18 = local_18 + 1;
    }
  }
  return;
}


// ============================================================================
// Function: TSA_IsJustEnterTypingScenario @ 6ba8fe53
// ============================================================================

undefined8 TSA_IsJustEnterTypingScenario(void)

{
  char cVar1;
  
                    /* 0x4fe53  2073  TSA_IsJustEnterTypingScenario */
  cVar1 = TSA_IsTypingScenario();
  if ((cVar1 != '\0') && (cVar1 = TSA_IsTypingScenarioPrev(), cVar1 == '\0')) {
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: TSA_GetTouchSide @ 6ba8f82b
// ============================================================================

undefined4 TSA_GetTouchSide(byte param_1)

{
                    /* 0x4f82b  2009  TSA_GetTouchSide */
  return TSA_GetSide(*(undefined2 *)
               ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162),
              *(undefined2 *)
               ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164));
}


// ============================================================================
// Function: TSA_GetMaxCMFNoiseValue @ 6ba9e207
// ============================================================================

int TSA_GetMaxCMFNoiseValue(void)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  
                    /* 0x5e207  1953  TSA_GetMaxCMFNoiseValue */
  uVar2 = (int)*(short *)&_refptr_g_tsaPrpt[1].field_0x46 >> 0x1f;
  uVar3 = (int)*(short *)&_refptr_g_tsaPrpt[1].field_0x44 >> 0x1f;
  if ((int)(((int)*(short *)&_refptr_g_tsaPrpt[1].field_0x44 ^ uVar3) - uVar3) <
      (int)(((int)*(short *)&_refptr_g_tsaPrpt[1].field_0x46 ^ uVar2) - uVar2)) {
    uVar2 = (int)*(short *)&_refptr_g_tsaPrpt[1].field_0x46 >> 0x1f;
    iVar1 = ((int)*(short *)&_refptr_g_tsaPrpt[1].field_0x46 ^ uVar2) - uVar2;
  }
  else {
    uVar2 = (int)*(short *)&_refptr_g_tsaPrpt[1].field_0x44 >> 0x1f;
    iVar1 = ((int)*(short *)&_refptr_g_tsaPrpt[1].field_0x44 ^ uVar2) - uVar2;
  }
  return iVar1;
}


// ============================================================================
// Function: TSA_ScenarioProcess @ 6ba904ed
// ============================================================================

void TSA_ScenarioProcess(void)

{
  int iVar1;
  char *_Str1;
  
                    /* 0x504ed  2253  TSA_ScenarioProcess */
  AFE_SetMaxScanRate(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x61e);
  TSA_AppTypeToScenario();
  TSARTPrmt_SetKeyValue("dbEnterIDLE",*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x40);
  iVar1 = TSA_GetScenarioInternal();
  if (iVar1 == 4) {
    TSA_ScenarioGamePrmtSetting();
  }
  else if (iVar1 == 0x40) {
    TSA_ScenarioNormalRateGamePrmtSetting();
  }
  else if (iVar1 == 2) {
    TSA_ScenarioTypingPrmtSetting();
  }
  else {
    iVar1 = UDFPEvent_IsEnabled();
    if (iVar1 != 0) {
      TSARTPrmt_SetKeyValue("dbEnterIDLE",2000);
    }
  }
  TSA_UpdateReportRate();
  if ((((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x80000) == 0) &&
     ((*_refptr_g_tsaStaticPtr)->wField_0x122 < 5)) {
    if (((*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags & 8) == 0) {
      _Str1 = (char *)TSA_GetPrmtName();
      iVar1 = strncmp(_Str1,"HIMA",4);
      if (iVar1 != 0) goto LAB_6ba906a7;
    }
    TSARTPrmt_SetKeyValue
              ("distToEnterGloveMode",*(undefined2 *)(_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 8));
    if ((*(int *)&_refptr_g_tsaPrpt[1].field_0x2c == 2) &&
       ((*(int *)&_refptr_g_tsaPrpt[1].field_0x2c != 2 ||
        (*(int *)&_refptr_g_tsaPrpt[1].field_0x30 != 2)))) {
      TSARTPrmt_SetKeyValue("accThold",*(ushort *)&_refptr_g_tsaPrmtFlash[1].field_0x4a / 3);
      if ((*_refptr_g_prevTouchesPtr)->bPrevTouchCount < 2) {
        TSA_EnableGloveFeature();
      }
    }
    else if ((*_refptr_g_prevTouchesPtr)->bPrevTouchCount < 3) {
      TSA_EnableGloveFeature();
    }
  }
  else {
LAB_6ba906a7:
    TSAStatic_RestoreExtFeatures(1);
  }
  TSA_UpdateSysScenario();
  TSA_SideTouchPrmtSetting();
  return;
}


// ============================================================================
// Function: TSA_RptTouchNum @ 6ba9b09c
// ============================================================================

uint TSA_RptTouchNum(void)

{
  uint uVar1;
  
                    /* 0x5b09c  2221  TSA_RptTouchNum */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_AFT_ENABLED) == 0) {
    uVar1 = (uint)**(byte **)_refptr_g_touchesPtr;
  }
  else {
    uVar1 = *(uint *)(_refptr_g_aftOutSortInfo + 0x3c0);
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_DetectedNoiseLevel @ 6ba9d789
// ============================================================================

uint TSA_DetectedNoiseLevel(void)

{
                    /* 0x5d789  1825  TSA_DetectedNoiseLevel */
  return *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x20) & 0x100;
}


// ============================================================================
// Function: TSA_GetEnabledSensor @ 6baa008c
// ============================================================================

undefined4 TSA_GetEnabledSensor(void)

{
  undefined4 uVar1;
  
                    /* 0x6008c  1927  TSA_GetEnabledSensor */
  if (((*_refptr_g_tsaStaticPtr)->field_0x3 == '\0') ||
     ((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6c4 & 8) != 0)) {
    uVar1 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x6c4;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_GetMSExceptionDescription @ 6ba99bca
// ============================================================================

void TSA_GetMSExceptionDescription(void)

{
  int iVar1;
  
                    /* 0x59bca  1949  TSA_GetMSExceptionDescription */
  iVar1 = TSA_GetDMDType();
  if (iVar1 == 0x10) {
    BigData_GetReport();
  }
  else {
    HardwareAnalyzer_GetExceptionDescription();
  }
  return;
}


// ============================================================================
// Function: TSA_GetMutualTouchTholdInIDLEMode @ 6ba9e880
// ============================================================================

ushort TSA_GetMutualTouchTholdInIDLEMode(void)

{
  ushort local_a;
  
                    /* 0x5e880  1967  TSA_GetMutualTouchTholdInIDLEMode */
  local_a = (ushort)(((int)(short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax *
                     (int)*(short *)&_refptr_g_tsaPrmtFlash->field_0x5b8) / 3000);
  if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3) {
    local_a = local_a >> 1;
  }
  return local_a;
}


// ============================================================================
// Function: TSA_IsVolumePressDown @ 6ba9fdb7
// ============================================================================

void TSA_IsVolumePressDown(void)

{
                    /* 0x5fdb7  2109  TSA_IsVolumePressDown */
  SideTouch_PressureIsVolumeDown();
  return;
}


// ============================================================================
// Function: TSA_IsMotorControlEnabled @ 6ba9fe03
// ============================================================================

void TSA_IsMotorControlEnabled(void)

{
                    /* 0x5fe03  2074  TSA_IsMotorControlEnabled */
  SideTouch_IsMotorControlEnabled();
  return;
}


// ============================================================================
// Function: TSA_GetExposedDataState @ 6ba9fe16
// ============================================================================

int TSA_GetExposedDataState(void)

{
                    /* 0x5fe16  1932  TSA_GetExposedDataState */
  return (*_refptr_g_tsaStaticPtr)->nField_0x28c;
}


// ============================================================================
// Function: TSA_GetAFEScanFreqIndex @ 6ba9f998
// ============================================================================

undefined8 TSA_GetAFEScanFreqIndex(longlong param_1)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0x5f998  1859  TSA_GetAFEScanFreqIndex */
  if (param_1 == 0) {
    uVar2 = 1;
  }
  else {
    if ((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) != 0) &&
       (cVar1 = TSA_InStylusMode(), cVar1 != '\0')) {
      return 1;
    }
    uVar2 = AFE_GetNextScanFreqIndex(param_1);
  }
  return uVar2;
}


// ============================================================================
// Function: TSA_GetAFEScanRateIndex @ 6ba9fa2d
// ============================================================================

undefined8 TSA_GetAFEScanRateIndex(longlong param_1)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x9]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x10];
  char cVar1;
  undefined8 uVar2;
  
                    /* 0x5fa2d  1860  TSA_GetAFEScanRateIndex */
  if (param_1 == 0) {
    uVar2 = 1;
  }
  else {
    if ((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) != 0) &&
       (cVar1 = TSA_InStylusMode(), cVar1 != '\0')) {
      return 1;
    }
    cVar1 = TSA_IsOKToEnterIDLEMode();
    if (cVar1 == '\0') {
      uVar2 = AFE_GetNextScanRateIndex(param_1,&(*(undefined1 *)(_stk + 0x7)));
    }
    else {
      uVar2 = 1;
    }
  }
  return uVar2;
}


// ============================================================================
// Function: TSA_RptTouchID @ 6ba9b256
// ============================================================================

int TSA_RptTouchID(byte param_1)

{
  int iVar1;
  
                    /* 0x5b256  2220  TSA_RptTouchID */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_AFT_ENABLED) == 0) {
    iVar1 = *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x154) - 1;
  }
  else {
    iVar1 = *(int *)(_refptr_g_aftOutSortInfo + (longlong)(int)(uint)param_1 * 0x30 + 4);
  }
  return iVar1;
}


// ============================================================================
// Function: TSA_RptTouchHand @ 6ba9b2f1
// ============================================================================

undefined1 TSA_RptTouchHand(byte param_1)

{
  undefined1 local_9;
  
                    /* 0x5b2f1  2219  TSA_RptTouchHand */
  local_9 = 0;
  if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1e4) &
      4) == 0) {
    if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1e4)
        & 8) != 0) {
      local_9 = 2;
    }
  }
  else {
    local_9 = 1;
  }
  return local_9;
}


// ============================================================================
// Function: TSA_RptTouchXPos @ 6ba9b428
// ============================================================================

ulonglong TSA_RptTouchXPos(byte param_1)

{
  ulonglong uVar1;
  
                    /* 0x5b428  2236  TSA_RptTouchXPos */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_AFT_ENABLED) == 0) {
    uVar1 = TSAOut_RptXPos(param_1);
  }
  else {
    uVar1 = (ulonglong)*(uint *)(_refptr_g_aftOutSortInfo + (longlong)(int)(uint)param_1 * 0x30 + 8)
    ;
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_RptTouchYPos @ 6ba9b482
// ============================================================================

ulonglong TSA_RptTouchYPos(byte param_1)

{
  ulonglong uVar1;
  
                    /* 0x5b482  2242  TSA_RptTouchYPos */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_AFT_ENABLED) == 0) {
    uVar1 = TSAOut_RptYPos(param_1);
  }
  else {
    uVar1 = (ulonglong)
            *(uint *)(_refptr_g_aftOutSortInfo + (longlong)(int)(uint)param_1 * 0x30 + 0xc);
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_RptTouchPressure @ 6ba9bc34
// ============================================================================

uint TSA_RptTouchPressure(byte param_1)

{
  uint uVar1;
  
                    /* 0x5bc34  2225  TSA_RptTouchPressure */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_AFT_ENABLED) == 0) {
    uVar1 = (uint)*(ushort *)
                   ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x15c
                   );
  }
  else {
    uVar1 = *(uint *)(_refptr_g_aftOutSortInfo + (longlong)(int)(uint)param_1 * 0x30 + 0x10);
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_RptTouchAxisMajor @ 6ba9bca2
// ============================================================================

ulonglong TSA_RptTouchAxisMajor(byte param_1)

{
  undefined2 uVar1;
  ulonglong uVar2;
  
                    /* 0x5bca2  2205  TSA_RptTouchAxisMajor */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_AFT_ENABLED) == 0) {
    uVar1 = TS_MMToPixel((char)*(undefined2 *)
                                ((longlong)(int)(uint)param_1 * 0x168 +
                                 *(longlong *)_refptr_g_touchesPtr + 0x188));
    uVar2 = TSAOut_Axis(uVar1);
  }
  else {
    uVar2 = (ulonglong)
            *(uint *)(_refptr_g_aftOutSortInfo + (longlong)(int)(uint)param_1 * 0x30 + 0x14);
  }
  return uVar2;
}


// ============================================================================
// Function: TSA_RptTouchAxisMajorExtend @ 6ba9bd2d
// ============================================================================

uint TSA_RptTouchAxisMajorExtend(byte param_1)

{
  undefined2 uVar1;
  ushort uVar2;
  uint local_c;
  
                    /* 0x5bd2d  2206  TSA_RptTouchAxisMajorExtend */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_AFT_ENABLED) == 0) {
    uVar1 = TS_MMToPixel((char)*(undefined2 *)
                                ((longlong)(int)(uint)param_1 * 0x168 +
                                 *(longlong *)_refptr_g_touchesPtr + 0x188));
    uVar2 = TSAOut_Axis(uVar1);
    local_c = (uint)uVar2;
    if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
        & 0x800000) != 0) {
      local_c = local_c | 0x8000000;
    }
    if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208)
        & 0x80) != 0) {
      local_c = local_c | 0x20000000;
    }
  }
  else {
    local_c = *(uint *)(_refptr_g_aftOutSortInfo + (longlong)(int)(uint)param_1 * 0x30 + 0x14);
  }
  return local_c;
}


// ============================================================================
// Function: TSA_RptTouchAxisMinor @ 6ba9be32
// ============================================================================

ulonglong TSA_RptTouchAxisMinor(byte param_1)

{
  undefined2 uVar1;
  ulonglong uVar2;
  
                    /* 0x5be32  2207  TSA_RptTouchAxisMinor */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_AFT_ENABLED) == 0) {
    uVar1 = TS_MMToPixel((char)*(undefined2 *)
                                ((longlong)(int)(uint)param_1 * 0x168 +
                                 *(longlong *)_refptr_g_touchesPtr + 0x18a));
    uVar2 = TSAOut_Axis(uVar1);
  }
  else {
    uVar2 = (ulonglong)
            *(uint *)(_refptr_g_aftOutSortInfo + (longlong)(int)(uint)param_1 * 0x30 + 0x18);
  }
  return uVar2;
}


// ============================================================================
// Function: TSA_RptTouchAxisAngle @ 6ba9bef5
// ============================================================================

ulonglong TSA_RptTouchAxisAngle(byte param_1)

{
  ulonglong uVar1;
  
                    /* 0x5bef5  2204  TSA_RptTouchAxisAngle */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_AFT_ENABLED) == 0) {
    uVar1 = TSAOut_Angle((int)*(char *)((longlong)(int)(uint)param_1 * 0x168 +
                                        *(longlong *)_refptr_g_touchesPtr + 0x192));
  }
  else {
    uVar1 = (ulonglong)
            *(uint *)(_refptr_g_aftOutSortInfo + (longlong)(int)(uint)param_1 * 0x30 + 0x1c);
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_RptTouchAge @ 6ba9bfc2
// ============================================================================

undefined2 TSA_RptTouchAge(byte param_1)

{
                    /* 0x5bfc2  2203  TSA_RptTouchAge */
  return *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x156);
}


// ============================================================================
// Function: TSA_GetDimEdge @ 6ba8f6f3
// ============================================================================

undefined4 TSA_GetDimEdge(ushort param_1,ushort param_2)

{
  ushort uVar1;
  ushort uVar2;
  undefined4 local_c;
  
                    /* 0x4f6f3  1923  TSA_GetDimEdge */
  uVar1 = (ushort)((uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2c) - (uint)param_1);
  if ((int)((uint)param_1 - (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2a)) <=
      (int)((uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2c) - (uint)param_1)) {
    uVar1 = (ushort)((uint)param_1 - (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2a));
  }
  uVar2 = (ushort)((uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x30) - (uint)param_2);
  if ((int)((uint)param_2 - (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2e)) <=
      (int)((uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x30) - (uint)param_2)) {
    uVar2 = (ushort)((uint)param_2 - (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2e));
  }
  if (uVar2 < uVar1) {
    if ((int)((uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x30) - (uint)param_2) <
        (int)((uint)param_2 - (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2e))) {
      local_c = 8;
    }
    else {
      local_c = 4;
    }
  }
  else if ((int)((uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2c) - (uint)param_1) <
           (int)((uint)param_1 - (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2a))) {
    local_c = 2;
  }
  else {
    local_c = 1;
  }
  return local_c;
}


// ============================================================================
// Function: TSA_SideToDimEdge @ 6ba8f560
// ============================================================================

undefined8 TSA_SideToDimEdge(int param_1)

{
                    /* 0x4f560  2318  TSA_SideToDimEdge */
  if (_refptr_g_tsaPrmtFlash->field_0x1a == '\0') {
    if (param_1 == 4) {
      return 1;
    }
    if (param_1 == 8) {
      return 2;
    }
    if (param_1 == 1) {
      return 4;
    }
    if (param_1 == 2) {
      return 8;
    }
  }
  else {
    if (param_1 == 4) {
      return 4;
    }
    if (param_1 == 8) {
      return 8;
    }
    if (param_1 == 1) {
      return 1;
    }
    if (param_1 == 2) {
      return 2;
    }
  }
  return 0;
}


// ============================================================================
// Function: TSA_GridDataRawInit @ 6ba917bb
// ============================================================================

void TSA_GridDataRawInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = *(undefined8 *)_refptr_g_tsaBufRaw;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"Raw");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x3cd,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 1;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  if (_refptr_g_tsaPrmtDynamic[0x24] != '\0') {
    *(undefined **)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = _refptr_g_tsaBufSSRaw;
  }
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataPreRawInit @ 6ba91967
// ============================================================================

void TSA_GridDataPreRawInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined **)(&g_ideGridData + (longlong)param_1 * 0x88) = _refptr_g_tsaBufPreRaw;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"PreRaw");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x3df,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 1;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataPreUnifiedRawInit @ 6ba91ad6
// ============================================================================

void TSA_GridDataPreUnifiedRawInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined **)(&g_ideGridData + (longlong)param_1 * 0x88) = _refptr_g_tsaBufPreUnifiedRaw;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"PreUnifiedRaw");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x3ec,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 1;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  if (_refptr_g_tsaPrmtDynamic[0x24] != '\0') {
    *(undefined **)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = _refptr_g_tsaBufSSPreUnifiedRaw;
  }
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataPreDiffInit @ 6ba91c7f
// ============================================================================

void TSA_GridDataPreDiffInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = *(undefined8 *)_refptr_g_tsaBufPreDif
  ;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"PreDiff");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x3fe,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataDiffInit @ 6ba91df5
// ============================================================================

void TSA_GridDataDiffInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = *(undefined8 *)_refptr_g_tsaBufDif;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"DIFF");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x40b,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  if (_refptr_g_tsaPrmtDynamic[0x24] != '\0') {
    *(undefined **)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = _refptr_g_tsaBufSSDif;
  }
  uVar2 = ASA_GetTx1LineDataPtr();
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = uVar2;
  uVar2 = ASA_GetTx2LineDataPtr();
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = uVar2;
  return;
}


// ============================================================================
// Function: TSA_GridDataPreUnifiedDifInit @ 6ba91fad
// ============================================================================

void TSA_GridDataPreUnifiedDifInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined **)(&g_ideGridData + (longlong)param_1 * 0x88) = _refptr_g_tsaBufPreUnifiedDif;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"PreUnifiedDiff");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x422,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataPreHWRawInit @ 6ba92120
// ============================================================================

void TSA_GridDataPreHWRawInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) =
       *(undefined8 *)_refptr_g_tsaBufPreHWRaw;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"PreHWRaw");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x430,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 1;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataRaw2RawDiffInit @ 6ba92292
// ============================================================================

void TSA_GridDataRaw2RawDiffInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) =
       *(undefined8 *)_refptr_g_tsaBufRaw2rawDif;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"Raw2RawDiff");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x43d,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataRaw2RawDiffCmfInit @ 6ba92408
// ============================================================================

void TSA_GridDataRaw2RawDiffCmfInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) =
       *(undefined8 *)_refptr_g_tsaBufRaw2rawDifAftCMF;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"Raw2RawDiffAftCMF");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x44a,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataRaw2RawDiff2Init @ 6ba9257e
// ============================================================================

void TSA_GridDataRaw2RawDiff2Init(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) =
       *(undefined8 *)_refptr_g_tsaBufRaw2rawDif2;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"Raw2RawDiff2");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x457,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataRaw2RawDiff3Init @ 6ba926f4
// ============================================================================

void TSA_GridDataRaw2RawDiff3Init(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) =
       *(undefined8 *)_refptr_g_tsaBufRaw2rawDif3;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"Raw2RawDiff3");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x464,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataPreCMFDiffInit @ 6ba9286a
// ============================================================================

void TSA_GridDataPreCMFDiffInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) =
       *(undefined8 *)_refptr_g_tsaBufPreCMFDif;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"PreCMFDiff");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x473,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  if (_refptr_g_tsaPrmtDynamic[0x24] != '\0') {
    *(undefined **)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = _refptr_g_tsaBufSSPreCMFDif;
  }
  uVar2 = ASA_GetTx1PreCmfLineDataPtr();
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = uVar2;
  uVar2 = ASA_GetTx2PreCmfLineDataPtr();
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = uVar2;
  return;
}


// ============================================================================
// Function: TSA_GridDataAftSSCMFDiffInit @ 6ba92a1e
// ============================================================================

void TSA_GridDataAftSSCMFDiffInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) =
       *(undefined8 *)_refptr_g_tsaBufAftSSCMFDif;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"AftSSCMFDiff");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x48b,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  if (_refptr_g_tsaPrmtDynamic[0x24] == '\0') {
    *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  }
  else {
    *(undefined **)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = _refptr_g_tsaBufSSDif;
  }
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataAftSSChargerNFilterDiffInit @ 6ba92bcc
// ============================================================================

void TSA_GridDataAftSSChargerNFilterDiffInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) =
       *(undefined8 *)_refptr_g_tsaBufSSChargerNFilterDif;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"AftSSChargerNFilterDiff");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x49c,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  if (_refptr_g_tsaPrmtDynamic[0x24] == '\0') {
    *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  }
  else {
    *(undefined **)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = _refptr_g_tsaBufSSDif;
  }
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataCommonModeNoiseInit @ 6ba92d7a
// ============================================================================

void TSA_GridDataCommonModeNoiseInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) =
       *(undefined8 *)_refptr_g_tsaBufCommonModeNoise;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"CommonModeNoise");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x4ae,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  if (_refptr_g_tsaPrmtDynamic[0x24] != '\0') {
    *(undefined **)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = _refptr_g_tsaBufSsDifForBl;
  }
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataAftCMFDiffInit @ 6ba92f26
// ============================================================================

void TSA_GridDataAftCMFDiffInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined **)(&g_ideGridData + (longlong)param_1 * 0x88) = _refptr_g_tsaBufAftCMFDif;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"AftCMFDiff");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x4c0,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  if (_refptr_g_tsaPrmtDynamic[0x24] != '\0') {
    *(undefined **)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = _refptr_g_tsaBufSsDifForBl;
  }
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataCmfDraftNeighborDistanceInit @ 6ba930cf
// ============================================================================

void TSA_GridDataCmfDraftNeighborDistanceInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined **)(&g_ideGridData + (longlong)param_1 * 0x88) =
       _refptr_g_tsaBufCmfDraftNeighborDistance;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"CMFDraftNeighborDistance");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x4d2,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  if (_refptr_g_tsaPrmtDynamic[0x24] != '\0') {
    *(undefined **)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = _refptr_g_tsaBufSsDifForBl;
  }
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataCmfNeighborDistanceInit @ 6ba93278
// ============================================================================

void TSA_GridDataCmfNeighborDistanceInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined **)(&g_ideGridData + (longlong)param_1 * 0x88) = _refptr_g_tsaBufCmfNeighborDistance;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"CMFNeighborDistance");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x4e4,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  if (_refptr_g_tsaPrmtDynamic[0x24] != '\0') {
    *(undefined **)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = _refptr_g_tsaBufSsDifForBl;
  }
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataAftPcaFilterDifInit @ 6ba93421
// ============================================================================

void TSA_GridDataAftPcaFilterDifInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined **)(&g_ideGridData + (longlong)param_1 * 0x88) = _refptr_g_tsaBufAftPcaFilterDif;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"AftPcaFilterDif");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x4f6,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  if (_refptr_g_tsaPrmtDynamic[0x24] != '\0') {
    *(undefined **)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = _refptr_g_tsaBufSsDifForBl;
  }
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataCmfWeightInit @ 6ba935ca
// ============================================================================

void TSA_GridDataCmfWeightInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined **)(&g_ideGridData + (longlong)param_1 * 0x88) = _refptr_g_tsaBufCmfWeight;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"CMFWeight");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x508,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  if (_refptr_g_tsaPrmtDynamic[0x24] != '\0') {
    *(undefined **)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = _refptr_g_tsaBufSsDifForBl;
  }
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataCmfDraftWeightInit @ 6ba93773
// ============================================================================

void TSA_GridDataCmfDraftWeightInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined **)(&g_ideGridData + (longlong)param_1 * 0x88) = _refptr_g_tsaBufCmfDraftWeight;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"CMFDraftWeight");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x51a,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  if (_refptr_g_tsaPrmtDynamic[0x24] != '\0') {
    *(undefined **)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = _refptr_g_tsaBufSsDifForBl;
  }
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataAftDim1CMFDiffInit @ 6ba9391c
// ============================================================================

void TSA_GridDataAftDim1CMFDiffInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined **)(&g_ideGridData + (longlong)param_1 * 0x88) = _refptr_g_tsaBufAftDim1CMFDif;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"AftDim1CMFDiff");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x52c,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  if (_refptr_g_tsaPrmtDynamic[0x24] != '\0') {
    *(undefined **)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = _refptr_g_tsaBufSsDifForBl;
  }
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataOneOrderDiffRxInit @ 6ba93ac5
// ============================================================================

void TSA_GridDataOneOrderDiffRxInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined **)(&g_ideGridData + (longlong)param_1 * 0x88) = _refptr_g_tsaBufOneOrderDiffRx;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"OneOrderDiffRx");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x53e,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  if (_refptr_g_tsaPrmtDynamic[0x24] != '\0') {
    *(undefined **)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = _refptr_g_tsaBufSsDifForBl;
  }
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataTwoOrderDiffRxInit @ 6ba93c6e
// ============================================================================

void TSA_GridDataTwoOrderDiffRxInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined **)(&g_ideGridData + (longlong)param_1 * 0x88) = _refptr_g_tsaBufTwoOrderDiffRx;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"TwoOrderDiffRx");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x550,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  if (_refptr_g_tsaPrmtDynamic[0x24] != '\0') {
    *(undefined **)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = _refptr_g_tsaBufSsDifForBl;
  }
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataPreNotchCorrectDiffInit @ 6ba93e17
// ============================================================================

void TSA_GridDataPreNotchCorrectDiffInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined **)(&g_ideGridData + (longlong)param_1 * 0x88) = _refptr_g_tsaBufPreNotchCorrectDif;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"PreNotchCorrectDiff");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x562,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataAftNotchCorrectDiffInit @ 6ba93f86
// ============================================================================

void TSA_GridDataAftNotchCorrectDiffInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined **)(&g_ideGridData + (longlong)param_1 * 0x88) = _refptr_g_tsaBufAftNotchCorrectDif;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"AftNotchCorrectDiff");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x56f,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataGridIIRDifInit @ 6ba940f5
// ============================================================================

void TSA_GridDataGridIIRDifInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) =
       *(undefined8 *)_refptr_g_tsaBufGridIIRDif;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"GridIIRDif");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x57d,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  if (_refptr_g_tsaPrmtDynamic[0x24] != '\0') {
    *(undefined **)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = _refptr_g_tsaBufSSIIRDif;
  }
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataSDInit @ 6ba942a1
// ============================================================================

void TSA_GridDataSDInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = *(undefined8 *)_refptr_g_tsaBufSd;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"SD");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x591,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  if (_refptr_g_tsaPrmtDynamic[0x24] != '\0') {
    *(undefined **)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = _refptr_g_tsaBufSSDif;
  }
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataBaselineInit @ 6ba94451
// ============================================================================

void TSA_GridDataBaselineInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = *(undefined8 *)_refptr_g_tsaBufBl;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"Baseline");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x5a4,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 1;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  if (_refptr_g_tsaPrmtDynamic[0x24] != '\0') {
    *(undefined **)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = _refptr_g_tsaBufSSBaseline;
  }
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataNormalBaselineInit @ 6ba945fd
// ============================================================================

void TSA_GridDataNormalBaselineInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) =
       *(undefined8 *)_refptr_g_tsaBufBlNormal;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"NormalBaseline");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x5b7,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 1;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataCMFRemapInit @ 6ba9476f
// ============================================================================

void TSA_GridDataCMFRemapInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined **)(&g_ideGridData + (longlong)param_1 * 0x88) = _refptr_g_tsaBufCMFRemap;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"CMFRemap");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x5c6,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataCMFRemapBackInit @ 6ba948de
// ============================================================================

void TSA_GridDataCMFRemapBackInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined **)(&g_ideGridData + (longlong)param_1 * 0x88) = _refptr_g_tsaBufCMFRemapBack;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"CMFRemapBack");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x5d5,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataTZFlagInit @ 6ba94a4d
// ============================================================================

void TSA_GridDataTZFlagInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = *(undefined8 *)_refptr_g_tsaBufTzGrid
  ;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"TZFlag");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x5e2,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 1;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataDistanceMapInit @ 6ba94bb8
// ============================================================================

void TSA_GridDataDistanceMapInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  uVar2 = TZ_GetDistanceMapPtr();
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = uVar2;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"DistanceMap");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x5ee,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 1;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 1;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataAfterDilateInit @ 6ba94d21
// ============================================================================

void TSA_GridDataAfterDilateInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined **)(&g_ideGridData + (longlong)param_1 * 0x88) = _refptr_g_tsaFlagAfterDilate;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"AfterDilate");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x5fa,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 1;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 1;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataAfterErodeInit @ 6ba94e89
// ============================================================================

void TSA_GridDataAfterErodeInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined **)(&g_ideGridData + (longlong)param_1 * 0x88) = _refptr_g_tsaFlagAfterErode;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"AfterErode");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x606,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 1;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 1;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataProxDifInit @ 6ba94ff1
// ============================================================================

void TSA_GridDataProxDifInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"ProxDif");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x612,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  if (*(longlong *)_refptr_g_tsaBufProxDif == 0) {
    *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = 0;
  }
  else {
    *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) =
         *(undefined8 *)_refptr_g_tsaBufProxDif;
    (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
    (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
    uVar2 = TSAMS_GetSelfDifPtr();
    *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
    *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
    *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  }
  return;
}


// ============================================================================
// Function: TSA_GridDataProxIIRRawInit @ 6ba9519f
// ============================================================================

void TSA_GridDataProxIIRRawInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"ProxIIRRaw");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x625,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  if (*(longlong *)_refptr_g_tsaBufProxIIRRaw == 0) {
    *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = 0;
  }
  else {
    *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) =
         *(undefined8 *)_refptr_g_tsaBufProxIIRRaw;
    (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
    (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 1;
    uVar2 = TSAMS_GetSelfDifPtr();
    *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
    *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
    *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  }
  return;
}


// ============================================================================
// Function: TSA_GridDataProxZnDifInit @ 6ba9534d
// ============================================================================

void TSA_GridDataProxZnDifInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"ProxZnDif");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x638,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  if (*(longlong *)_refptr_g_tsaBufProxZnDif == 0) {
    *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = 0;
  }
  else {
    *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) =
         *(undefined8 *)_refptr_g_tsaBufProxZnDif;
    (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
    (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
    uVar2 = TSAMS_GetSelfDifPtr();
    *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
    *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
    *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  }
  return;
}


// ============================================================================
// Function: TSA_GridDataProxBaselineInit @ 6ba954fb
// ============================================================================

void TSA_GridDataProxBaselineInit(int param_1)

{
  errno_t eVar1;
  
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"ProxBaseline");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x64b,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  if (*(longlong *)_refptr_g_tsaBufProxBL == 0) {
    *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = 0;
  }
  else {
    *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) =
         *(undefined8 *)_refptr_g_tsaBufProxBL;
    (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
    (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 1;
    *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
    *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
    *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  }
  return;
}


// ============================================================================
// Function: TSA_GridDataProxTZInit @ 6ba956a5
// ============================================================================

void TSA_GridDataProxTZInit(int param_1)

{
  errno_t eVar1;
  
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"ProxTZ");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x65e,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  if (*(longlong *)_refptr_g_tsaBufTzProx == 0) {
    *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = 0;
  }
  else {
    *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) =
         *(undefined8 *)_refptr_g_tsaBufTzProx;
    (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 1;
    (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
    *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
    *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
    *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  }
  return;
}


// ============================================================================
// Function: TSA_GridDataSideTZInit @ 6ba95848
// ============================================================================

void TSA_GridDataSideTZInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = *(undefined8 *)_refptr_g_tsaBufTzSide
  ;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"SideTZ");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x671,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 1;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataSideTouchInit @ 6ba959b3
// ============================================================================

void TSA_GridDataSideTouchInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined **)(&g_ideGridData + (longlong)param_1 * 0x88) = _refptr_g_tsaBufSideTouch;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"SideTouch");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x680,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataSafeBaselineInit @ 6ba95b26
// ============================================================================

void TSA_GridDataSafeBaselineInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  uVar2 = SafeBaseline_GetLatestBuf();
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = uVar2;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"SafeBaseline");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x68e,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 1;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataDifSafeBL2RawInit @ 6ba95c96
// ============================================================================

void TSA_GridDataDifSafeBL2RawInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  uVar2 = SafeBaseline_GetSafeBL2RawDif();
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = uVar2;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"DifSafeBL2Raw");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x69a,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataDifSafeBL2RawCMFInit @ 6ba95e0a
// ============================================================================

void TSA_GridDataDifSafeBL2RawCMFInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  uVar2 = SafeBaseline_GetSafeBL2RawDifCMF();
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = uVar2;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"DifSafeBL2RawCMF");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x6a6,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataDifSafeBL2BLInit @ 6ba95f7e
// ============================================================================

void TSA_GridDataDifSafeBL2BLInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  uVar2 = SafeBaseline_GetSafeBL2BLDif();
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = uVar2;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"DifSafeBL2BL");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x6b2,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataDifSafeBL2BLCMFInit @ 6ba960f2
// ============================================================================

void TSA_GridDataDifSafeBL2BLCMFInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  uVar2 = SafeBaseline_GetSafeBL2BLDifCMF();
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = uVar2;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"DifSafeBL2BLCMF");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x6be,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataMaxSigDownInit @ 6ba96266
// ============================================================================

void TSA_GridDataMaxSigDownInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = g_tsaBufMaxSigDown;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"MaxSigDown");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x6ca,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataMinSigOffInit @ 6ba963dc
// ============================================================================

void TSA_GridDataMinSigOffInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = g_tsaBufMinSigOff;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"MinSigOff");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x6d6,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataTouchOnTimeInit @ 6ba96552
// ============================================================================

void TSA_GridDataTouchOnTimeInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = g_tsaBufTimeDown;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"TouchOnTime");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x6e1,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataTouchRptTimeInit @ 6ba966c1
// ============================================================================

void TSA_GridDataTouchRptTimeInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = g_tsaBufTimeRpt;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"TouchRptTime");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x6ec,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataTouchOffTimeInit @ 6ba96830
// ============================================================================

void TSA_GridDataTouchOffTimeInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = g_tsaBufTimeOff;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"TouchOffTime");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x6f7,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataSilentTouchNumInit @ 6ba9699f
// ============================================================================

void TSA_GridDataSilentTouchNumInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = g_tsaBufSilentTouchNum;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"SilentTouchNum");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x702,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 1;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataTZIDInit @ 6ba96b0e
// ============================================================================

void TSA_GridDataTZIDInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = *(undefined8 *)_refptr_g_tsaBufTzID;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"tzID");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x710,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 1;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataTZZoneInit @ 6ba96c7d
// ============================================================================

void TSA_GridDataTZZoneInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = *(undefined8 *)_refptr_g_tsaBufTzZone
  ;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"TzZone");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x71d,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 1;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataSubTZInit @ 6ba96dec
// ============================================================================

void TSA_GridDataSubTZInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = *(undefined8 *)_refptr_g_tsaBufSubTZ;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"subTZ");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x72a,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 1;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataGripRegionInit @ 6ba96f5b
// ============================================================================

void TSA_GridDataGripRegionInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) =
       *(undefined8 *)_refptr_g_tsaBufGripRegion;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"GripRegion");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x737,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 1;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataGripStageInit @ 6ba970ca
// ============================================================================

void TSA_GridDataGripStageInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) =
       *(undefined8 *)_refptr_g_tsaBufGripStage;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"gripStage");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x742,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 1;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataGripTimeInit @ 6ba97239
// ============================================================================

void TSA_GridDataGripTimeInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) =
       *(undefined8 *)_refptr_g_tsaBufGripTime;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"gripTime");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x74d,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataGripTimeOffInit @ 6ba973a8
// ============================================================================

void TSA_GridDataGripTimeOffInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) =
       *(undefined8 *)_refptr_g_tsaBufGripTimeOff;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"gripTimeOff");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x758,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataHandGestureBaselineInit @ 6ba97517
// ============================================================================

void TSA_GridDataHandGestureBaselineInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  uVar2 = HandGesture_GetHandGestureBL();
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = uVar2;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"HandGestureBaseline");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x767,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 1;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataHandGestureDifInit @ 6ba97687
// ============================================================================

void TSA_GridDataHandGestureDifInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  uVar2 = HandGesture_GetHandGestureDif();
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = uVar2;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"HandGestureDif");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x773,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataHandGestureTZInit @ 6ba977fb
// ============================================================================

void TSA_GridDataHandGestureTZInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = *(undefined8 *)_refptr_g_tsaBufTzHand
  ;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"HandTZ");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x77e,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 1;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataHandGestureAreaInit @ 6ba97966
// ============================================================================

void TSA_GridDataHandGestureAreaInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) =
       *(undefined8 *)_refptr_g_tsaBufAreaHand;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"HandArea");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x789,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 1;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataHandGestureTZPrevInit @ 6ba97ad1
// ============================================================================

void TSA_GridDataHandGestureTZPrevInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) =
       *(undefined8 *)_refptr_g_tsaBufTzPrevHand;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"HandTZPrev");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x794,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 1;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDataHandGestureTimeInit @ 6ba97c3c
// ============================================================================

void TSA_GridDataHandGestureTimeInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) =
       *(undefined8 *)_refptr_g_tsaBufTimeHand;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"HandTime");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x79f,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDatatzAgeInit @ 6ba97da7
// ============================================================================

void TSA_GridDatatzAgeInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = *(undefined8 *)_refptr_g_tsaBufTzAge;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"tzAge(In ms)");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x7ad,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 1;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_GridDatatzOriTx1 @ 6ba97f1d
// ============================================================================

void TSA_GridDatatzOriTx1(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  uVar2 = ASA_GetOriDataToShowTX1Ptr();
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = uVar2;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"oriTX1");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x7ba,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  *(undefined **)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = _refptr_g_tsaBufSSDif;
  uVar2 = ASA_GetTx1LineDataPtr();
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = uVar2;
  uVar2 = ASA_GetTx2LineDataPtr();
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = uVar2;
  return;
}


// ============================================================================
// Function: TSA_GridDatatzOriTx2 @ 6ba98098
// ============================================================================

void TSA_GridDatatzOriTx2(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  uVar2 = ASA_GetOriDataToShowTX2Ptr();
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = uVar2;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"oriTX2");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x7c7,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  *(undefined **)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = _refptr_g_tsaBufSSDif;
  uVar2 = ASA_GetTx1LineDataPtr();
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = uVar2;
  uVar2 = ASA_GetTx2LineDataPtr();
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = uVar2;
  return;
}


// ============================================================================
// Function: TSA_GridDataPeakFlagTx1 @ 6ba98213
// ============================================================================

void TSA_GridDataPeakFlagTx1(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  uVar2 = ASA_GetPeakFlagToShowTX1Ptr();
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = uVar2;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"peakTX1");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x7d4,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  *(undefined **)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = _refptr_g_tsaBufSSDif;
  uVar2 = ASA_GetTx1LineDataPtr();
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = uVar2;
  uVar2 = ASA_GetTx2LineDataPtr();
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = uVar2;
  return;
}


// ============================================================================
// Function: TSA_GridDataPeakFlagTx2 @ 6ba9838e
// ============================================================================

void TSA_GridDataPeakFlagTx2(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  uVar2 = ASA_GetPeakFlagToShowTX2Ptr();
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = uVar2;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"peakTX2");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x7e1,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  *(undefined **)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = _refptr_g_tsaBufSSDif;
  uVar2 = ASA_GetTx1LineDataPtr();
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = uVar2;
  uVar2 = ASA_GetTx2LineDataPtr();
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = uVar2;
  return;
}


// ============================================================================
// Function: TSA_GridNopressTable @ 6ba98509
// ============================================================================

void TSA_GridNopressTable(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  uVar2 = ASA_GetGridNopressTablePtr();
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = uVar2;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"NoPressTable");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x7ee,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 1;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  uVar2 = ASA_GetTx1LineDataPtr();
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = uVar2;
  uVar2 = ASA_GetGridNopressTableLinePtr();
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = uVar2;
  return;
}


// ============================================================================
// Function: TSA_GridNopressTableShortTerm @ 6ba98681
// ============================================================================

void TSA_GridNopressTableShortTerm(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  uVar2 = ASA_GetGridNopressTableShortTermPtr();
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = uVar2;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"NoPressTableShortTerm");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x7fb,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 1;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  uVar2 = ASA_GetTx1LineDataPtr();
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = uVar2;
  uVar2 = ASA_GetTx2LineDataPtr();
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = uVar2;
  return;
}


// ============================================================================
// Function: TSA_GridNopressTableTx2 @ 6ba987f9
// ============================================================================

void TSA_GridNopressTableTx2(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  uVar2 = ASA_GetGridNopressTableTx2Ptr();
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = uVar2;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"NoPressTableTx2");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x808,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 1;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  uVar2 = ASA_GetTx1LineDataPtr();
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = uVar2;
  uVar2 = ASA_GetTx2LineDataPtr();
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = uVar2;
  return;
}


// ============================================================================
// Function: TSA_GridNopressTableTx2ShortTerm @ 6ba98971
// ============================================================================

void TSA_GridNopressTableTx2ShortTerm(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  uVar2 = ASA_GetGridNopressTableShortTermTx2Ptr();
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = uVar2;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"NoPressTableTx2ShortTerm");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x815,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 1;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  uVar2 = ASA_GetTx1LineDataPtr();
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = uVar2;
  uVar2 = ASA_GetTx2LineDataPtr();
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = uVar2;
  return;
}


// ============================================================================
// Function: TSA_GridNopressSafeTable @ 6ba98ae9
// ============================================================================

void TSA_GridNopressSafeTable(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  uVar2 = ASA_GetGridNopressSafeTablePtr();
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = uVar2;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"NoPressSafeTable");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x822,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 1;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  uVar2 = ASA_GetTx1LineDataPtr();
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = uVar2;
  uVar2 = ASA_GetTx2LineDataPtr();
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = uVar2;
  return;
}


// ============================================================================
// Function: TSA_GridTiedGridUpdate @ 6ba98c61
// ============================================================================

void TSA_GridTiedGridUpdate(int param_1,undefined8 param_2,char *param_3)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = param_2;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,param_3);
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x832,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 1;
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = 0;
  uVar2 = ASA_GetTx1LineDataPtr();
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = uVar2;
  uVar2 = ASA_GetTx2LineDataPtr();
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = uVar2;
  return;
}


// ============================================================================
// Function: TSA_GridDataUnknownInit @ 6ba98ddd
// ============================================================================

void TSA_GridDataUnknownInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = *(undefined8 *)_refptr_g_tsaBufDif;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL)),100,"Unknown");
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_MSProc.c",
                      0x83f,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc44fac)[(longlong)param_1 * 0x88] = 2;
  (&DAT_6bc44fad)[(longlong)param_1 * 0x88] = 0;
  uVar2 = TSAMS_GetSelfDifPtr();
  *(undefined8 *)(&DAT_6bc44fb0 + (longlong)param_1 * 0x88) = uVar2;
  *(undefined8 *)(&DAT_6bc44fb8 + (longlong)param_1 * 0x88) = 0;
  *(undefined8 *)(&DAT_6bc44fc0 + (longlong)param_1 * 0x88) = 0;
  return;
}


// ============================================================================
// Function: TSA_InitBlockIdx @ 6baa0b82
// ============================================================================

void TSA_InitBlockIdx(void)

{
  uint uVar1;
  uint uVar2;
  uint local_10;
  byte local_a;
  byte local_9;
  
  uVar1 = (uint)_refptr_g_tsaPrmtFlash->bCols;
  uVar2 = (uint)_refptr_g_tsaPrmtFlash->bRows;
  local_10 = (uint)_refptr_g_tsaPrmtFlash->wBufElementCount;
  while (local_10 = local_10 - 1, -1 < (int)local_10) {
    *(undefined1 *)((int)local_10 + g_tsaBlockIdx) = 0;
  }
  for (local_a = 0; local_a < uVar2; local_a = local_a + 1) {
    *(undefined1 *)
     ((ulonglong)((uint)_refptr_g_tsaPrmtFlash->bCols * (uint)local_a) + g_tsaBlockIdx) = 1;
    *(undefined1 *)
     ((ulonglong)(((uint)_refptr_g_tsaPrmtFlash->bCols * (uint)local_a + uVar1) - 1 & 0xffff) +
     g_tsaBlockIdx) = 1;
  }
  for (local_a = 1; (int)(uint)local_a < (int)(uVar2 - 1); local_a = local_a + 1) {
    *(undefined1 *)
     ((ulonglong)((uint)_refptr_g_tsaPrmtFlash->bCols * (uint)local_a + 1 & 0xffff) + g_tsaBlockIdx)
         = 2;
    *(undefined1 *)
     ((ulonglong)(((uint)_refptr_g_tsaPrmtFlash->bCols * (uint)local_a + uVar1) - 2 & 0xffff) +
     g_tsaBlockIdx) = 2;
  }
  *(undefined1 *)(g_tsaBlockIdx + 1) = 3;
  *(undefined1 *)((ulonglong)(uVar1 - 2 & 0xffff) + g_tsaBlockIdx) = 3;
  *(undefined1 *)
   ((ulonglong)((uint)_refptr_g_tsaPrmtFlash->bCols * (uVar2 - 1) + 1 & 0xffff) + g_tsaBlockIdx) = 4
  ;
  *(undefined1 *)
   ((ulonglong)(((uint)_refptr_g_tsaPrmtFlash->bCols * (uVar2 - 1) + uVar1) - 2 & 0xffff) +
   g_tsaBlockIdx) = 4;
  for (local_9 = 2; (int)(uint)local_9 < (int)(uVar1 - 2); local_9 = local_9 + 1) {
    *(undefined1 *)((ulonglong)local_9 + g_tsaBlockIdx) = 5;
    *(undefined1 *)
     ((ulonglong)((uint)local_9 + (uVar2 - 1) * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) +
     g_tsaBlockIdx) = 6;
  }
  return;
}


// ============================================================================
// Function: TSA_GetSensorData @ 6ba8f9be
// ============================================================================

longlong TSA_GetSensorData(void)

{
                    /* 0x4f9be  1986  TSA_GetSensorData */
  return (*_refptr_g_tsaStaticPtr)->llField_0x148;
}


// ============================================================================
// Function: TSA_IsTypingScenarioPrev @ 6ba8fe35
// ============================================================================

bool TSA_IsTypingScenarioPrev(void)

{
                    /* 0x4fe35  2104  TSA_IsTypingScenarioPrev */
  return ((*_refptr_g_tsaStaticPtr)->dwField_0x158 & 2) != 0;
}


// ============================================================================
// Function: TSA_GetSide @ 6ba8f7f7
// ============================================================================

undefined4 TSA_GetSide(undefined2 param_1,undefined2 param_2)

{
  undefined4 uVar1;
  
                    /* 0x4f7f7  1988  TSA_GetSide */
  uVar1 = TSA_GetDimEdge(param_1,param_2);
  return TSA_DimEdgeToSide(uVar1);
}


// ============================================================================
// Function: TSA_UpdateSysScenario @ 6ba8fc9b
// ============================================================================

void TSA_UpdateSysScenario(void)

{
  char cVar1;
  
                    /* 0x4fc9b  2339  TSA_UpdateSysScenario */
  (*_refptr_g_tsaStaticPtr)->dwAftRuntimeModeMask = 0;
  cVar1 = TSA_IsScreenLockPhoneCallScenario();
  if (cVar1 == '\0') {
    cVar1 = TSA_IsPhoneCallScenario();
    if (cVar1 == '\0') {
      cVar1 = TSA_IsScreenLockScenario();
      if (cVar1 == '\0') {
        cVar1 = TSA_IsPossibleProximity();
        if (cVar1 != '\0') {
          (*_refptr_g_tsaStaticPtr)->dwAftRuntimeModeMask =
               (*_refptr_g_tsaStaticPtr)->dwAftRuntimeModeMask | 8;
        }
      }
      else {
        (*_refptr_g_tsaStaticPtr)->dwAftRuntimeModeMask =
             (*_refptr_g_tsaStaticPtr)->dwAftRuntimeModeMask | 1;
      }
    }
    else {
      (*_refptr_g_tsaStaticPtr)->dwAftRuntimeModeMask =
           (*_refptr_g_tsaStaticPtr)->dwAftRuntimeModeMask | 2;
    }
  }
  else {
    (*_refptr_g_tsaStaticPtr)->dwAftRuntimeModeMask =
         (*_refptr_g_tsaStaticPtr)->dwAftRuntimeModeMask | 4;
  }
  return;
}


// ============================================================================
// Function: TSA_GetScenarioInternal @ 6ba8fd76
// ============================================================================

uint TSA_GetScenarioInternal(void)

{
  uint local_c;
  
                    /* 0x4fd76  1981  TSA_GetScenarioInternal */
  local_c = (*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags;
  if ((local_c & 2) == 0) {
    if ((local_c & 4) == 0) {
      if ((local_c & 0x10) == 0) {
        if ((local_c & 0x40) == 0) {
          if ((local_c & 1) != 0) {
            local_c = 1;
          }
        }
        else {
          local_c = 0x40;
        }
      }
      else {
        local_c = 0x10;
      }
    }
    else {
      local_c = 4;
    }
  }
  else {
    local_c = 2;
  }
  return local_c;
}


// ============================================================================
// Function: TSA_ScenarioTypingPrmtSetting @ 6ba8ff4d
// ============================================================================

void TSA_ScenarioTypingPrmtSetting(void)

{
  char cVar1;
  
  TSARTPrmt_SetKeyValue("dbEnterIDLE",*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x42);
  cVar1 = PrevTouch_IsAnyExceptionDetected();
  if (cVar1 == '\0') {
    TSARTPrmt_SetKeyValue("sigMult",_refptr_g_tsaPrmtFlash->field_0x81f);
    TSARTPrmt_SetKeyValue("sigMultSD",_refptr_g_tsaPrmtFlash->field_0x81f);
    TSARTPrmt_SetKeyValue("accThold",*(ushort *)&_refptr_g_tsaPrmtFlash[1].field_0x4a >> 1);
  }
  return;
}


// ============================================================================
// Function: TSA_ScenarioGamePrmtSetting @ 6ba8ffe1
// ============================================================================

void TSA_ScenarioGamePrmtSetting(void)

{
  char cVar1;
  
  cVar1 = PrevTouch_IsAnyExceptionDetected();
  if (cVar1 == '\0') {
    TSARTPrmt_SetKeyValue("sigRatioToDirectReport",0x10);
  }
  TSARTPrmt_SetKeyValue("dbEnterIDLE",*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x3e);
  TSARTPrmt_SetKeyValue("maxClickSuppressDistance",_refptr_g_tsaPrmtDynamic[0x7b]);
  TSARTPrmt_SetKeyValue("clickDuration",*(undefined2 *)(_refptr_g_tsaPrmtDynamic + 0x76));
  TSARTPrmt_SetKeyValue("clickSuppressDistance",_refptr_g_tsaPrmtDynamic[0x79]);
  AFE_SetMaxScanRate(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x620);
  return;
}


// ============================================================================
// Function: TSA_ScenarioNormalRateGamePrmtSetting @ 6ba90092
// ============================================================================

void TSA_ScenarioNormalRateGamePrmtSetting(void)

{
  char cVar1;
  
  cVar1 = PrevTouch_IsAnyExceptionDetected();
  if (cVar1 == '\0') {
    TSARTPrmt_SetKeyValue("sigRatioToDirectReport",0x10);
  }
  TSARTPrmt_SetKeyValue("dbEnterIDLE",*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x3e);
  TSARTPrmt_SetKeyValue("maxClickSuppressDistance",_refptr_g_tsaPrmtDynamic[0x7b]);
  TSARTPrmt_SetKeyValue("clickDuration",*(undefined2 *)(_refptr_g_tsaPrmtDynamic + 0x76));
  TSARTPrmt_SetKeyValue("clickSuppressDistance",_refptr_g_tsaPrmtDynamic[0x79]);
  return;
}


// ============================================================================
// Function: TSA_SideTouchPrmtSetting @ 6ba90223
// ============================================================================

void TSA_SideTouchPrmtSetting(void)

{
  if (((((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x400000) == 0) &&
      ((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6a0 & 0x4000) != 0)) &&
     (((*_refptr_g_tsaStaticPtr)->nField_0x264 == 1 ||
      ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 3)))) {
    TSARTPrmt_SetKeyValue
              ("volumeActiveAreaStart",*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x684);
    TSARTPrmt_SetKeyValue
              ("volumeActiveAreaEnd",*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x686);
    TSARTPrmt_SetKeyValue
              ("volumeGreenAreaStart",*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x688);
    TSARTPrmt_SetKeyValue
              ("volumeGreenAreaEnd",*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x68a);
    TSA_SetVolumeSideEnabledScreenRotate90or270();
  }
  return;
}


// ============================================================================
// Function: TSA_AppTypeToScenario @ 6ba90318
// ============================================================================

void TSA_AppTypeToScenario(void)

{
  int iVar1;
  
  if ((*_refptr_g_tsaStaticPtr)->field_0x150 != '\0') {
    (*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags =
         (*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags & 2;
    iVar1 = *(int *)&(*_refptr_g_tsaStaticPtr)->field_0x160;
    if (iVar1 == 8) {
      (*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags =
           (*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags | 0x80;
    }
    else {
      if (iVar1 < 9) {
        if (iVar1 == 4) {
          (*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags =
               (*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags | 0x100;
          return;
        }
      }
      else {
        if (iVar1 == 9) {
          (*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags =
               (*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags | 4;
          return;
        }
        if (iVar1 == 999) {
          (*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags =
               (*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags | 0x20;
          return;
        }
      }
      (*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags =
           (*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags | 1;
    }
  }
  return;
}


// ============================================================================
// Function: TSA_UpdateReportRate @ 6ba9044e
// ============================================================================

void TSA_UpdateReportRate(void)

{
  if ((*_refptr_g_tsaStaticPtr)->field_0x150 != '\0') {
    if ((*_refptr_g_tsaStaticPtr)->field_0x15c == '\0') {
      AFE_SetMaxScanRate(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x61e);
    }
    else if ((char)(*_refptr_g_tsaStaticPtr)->field_0x15c < '\a') {
      AFE_SetMaxScanRate((char)(*_refptr_g_tsaStaticPtr)->field_0x15c * 0x3c);
    }
    else {
      AFE_SetMaxScanRate(0x168);
    }
  }
  return;
}


// ============================================================================
// Function: TSA_EnableGloveFeature @ 6ba9eacd
// ============================================================================

bool TSA_EnableGloveFeature(void)

{
  bool bVar1;
  
                    /* 0x5eacd  1842  TSA_EnableGloveFeature */
  bVar1 = (_refptr_g_tsaPrmtDynamic[0x80] & 4) == 0;
  if (!bVar1) {
    (*_refptr_g_tsaStaticPtr)->dwFeatureFlags = (*_refptr_g_tsaStaticPtr)->dwFeatureFlags | 1;
  }
  return bVar1;
}


// ============================================================================
// Function: TSA_GetMajorVersion @ 6ba9ab60
// ============================================================================

undefined8 TSA_GetMajorVersion(void)

{
                    /* 0x5ab60  1952  TSA_GetMajorVersion */
  return 0;
}


// ============================================================================
// Function: TSA_GetMinorVersion @ 6ba9ab6b
// ============================================================================

undefined8 TSA_GetMinorVersion(void)

{
                    /* 0x5ab6b  1960  TSA_GetMinorVersion */
  return 0;
}


// ============================================================================
// Function: TSA_GetBranchName @ 6ba9e8f7
// ============================================================================

char * TSA_GetBranchName(void)

{
                    /* 0x5e8f7  1910  TSA_GetBranchName */
  return "TL_develop_gaokun";
}


// ============================================================================
// Function: TSA_GetRatio @ 6ba8f953
// ============================================================================

ulonglong TSA_GetRatio(int param_1,int param_2,int param_3)

{
  ulonglong uVar1;
  
                    /* 0x4f953  1976  TSA_GetRatio */
  if (param_2 == 0) {
    uVar1 = 0;
  }
  else {
    uVar1 = (longlong)(param_1 * param_3) / (longlong)param_2 & 0xffffffff;
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_GetPatchVersion @ 6ba9ab76
// ============================================================================

undefined8 TSA_GetPatchVersion(void)

{
                    /* 0x5ab76  1970  TSA_GetPatchVersion */
  return 0;
}


// ============================================================================
// Function: TSA_IsVideoScenario @ 6ba8fdf7
// ============================================================================

bool TSA_IsVideoScenario(void)

{
                    /* 0x4fdf7  2107  TSA_IsVideoScenario */
  return ((*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags & 0x80) != 0;
}


// ============================================================================
// Function: TSA_DimEdgeToSide @ 6ba8f5e5
// ============================================================================

undefined8 TSA_DimEdgeToSide(int param_1)

{
                    /* 0x4f5e5  1828  TSA_DimEdgeToSide */
  if (_refptr_g_tsaPrmtFlash->field_0x1a == '\0') {
    if (param_1 == 1) {
      return 4;
    }
    if (param_1 == 2) {
      return 8;
    }
    if (param_1 == 4) {
      return 1;
    }
    if (param_1 == 8) {
      return 2;
    }
  }
  else {
    if (param_1 == 4) {
      return 4;
    }
    if (param_1 == 8) {
      return 8;
    }
    if (param_1 == 1) {
      return 1;
    }
    if (param_1 == 2) {
      return 2;
    }
  }
  return 0;
}


// ============================================================================
// Function: TSA_SideToEdge @ 6ba8f66a
// ============================================================================

undefined8 TSA_SideToEdge(uint param_1)

{
  undefined8 uVar1;
  
                    /* 0x4f66a  2319  TSA_SideToEdge */
  if ((param_1 & 4) == 0) {
    if ((param_1 & 8) == 0) {
      if ((param_1 & 1) == 0) {
        if ((param_1 & 2) == 0) {
          StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                            "../../src/App/TSA_Common.c",0x59,&DAT_6bb53115);
          HardwareAnalyzer_SetSoftException();
          uVar1 = 0;
        }
        else {
          uVar1 = 1;
        }
      }
      else {
        uVar1 = 0;
      }
    }
    else {
      uVar1 = 3;
    }
  }
  else {
    uVar1 = 2;
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_RptTouchYCenter @ 6ba9ba35
// ============================================================================

ushort TSA_RptTouchYCenter(undefined1 param_1)
{
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    return TSA_RptTouchDim2Center(param_1);
  } else {
    return TSA_RptTouchDim1Center(param_1);
  }
}


// ============================================================================
// Function: TSA_RptTouchYGripCtd @ 6ba9b8a3
// ============================================================================

undefined1 TSA_RptTouchYGripCtd(byte param_1)

{
  undefined1 uVar1;
  
                    /* 0x5b8a3  2240  TSA_RptTouchYGripCtd */
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    uVar1 = *(undefined1 *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x197);
  }
  else {
    uVar1 = *(undefined1 *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x196);
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_RptTouchXCenter @ 6ba9b9f0
// ============================================================================

ushort TSA_RptTouchXCenter(undefined1 param_1)
{
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    return TSA_RptTouchDim1Center(param_1);
  } else {
    return TSA_RptTouchDim2Center(param_1);
  }
}


// ============================================================================
// Function: TSA_IsScreenLockPhoneCallScenario @ 6ba8fbfd
// ============================================================================

undefined8 TSA_IsScreenLockPhoneCallScenario(void)

{
  undefined8 uVar1;
  
                    /* 0x4fbfd  2092  TSA_IsScreenLockPhoneCallScenario */
  if ((((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x400000) == 0) ||
     (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x800000) == 0)) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_IsPossibleProximity @ 6ba8fc41
// ============================================================================

bool TSA_IsPossibleProximity(void)

{
  TSA_FeatureFlags TVar1;
  char cVar2;
  bool local_9;
  
                    /* 0x4fc41  2086  TSA_IsPossibleProximity */
  TVar1 = (*_refptr_g_tsaStaticPtr)->dwFeatureFlags;
  cVar2 = TSA_IsPhoneCallScenario();
  local_9 = cVar2 == '\0' && (TVar1 & 0x4000) != 0;
  cVar2 = TSA_IsScreenLockScenario();
  if (cVar2 != '\0') {
    local_9 = false;
  }
  cVar2 = TSA_IsScreenLockPhoneCallScenario();
  if (cVar2 != '\0') {
    local_9 = false;
  }
  return local_9;
}


// ============================================================================
// Function: TSA_SetVolumeSideEnabledScreenRotate90or270 @ 6ba9012b
// ============================================================================

void TSA_SetVolumeSideEnabledScreenRotate90or270(void)

{
  if ((*(short *)&_refptr_g_tsaPrmtFlash[2].field_0x68c == 0) ||
     (*(short *)&_refptr_g_tsaPrmtFlash[2].field_0x68c == 3)) {
    TSARTPrmt_SetKeyValue("volumeSideEnabled",*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x68c)
    ;
  }
  else {
    if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x68c & 1) != 0) {
      TSARTPrmt_SetKeyValue("volumeSideEnabled",1);
      if ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 3) {
        TSARTPrmt_SetKeyValue("volumeSideEnabled",2);
      }
    }
    if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x68c & 2) != 0) {
      TSARTPrmt_SetKeyValue("volumeSideEnabled",2);
      if ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 3) {
        TSARTPrmt_SetKeyValue("volumeSideEnabled",1);
      }
    }
  }
  return;
}


// ============================================================================
// Function: TSA_IsProximityDetected @ 6ba9fbec
// ============================================================================

bool TSA_IsProximityDetected(void)

{
  uint uVar1;
  
                    /* 0x5fbec  2089  TSA_IsProximityDetected */
  uVar1 = TSA_GetState();
  return (uVar1 & 0x80) != 0;
}


// ============================================================================
// Function: TSA_CoreTouchDim1Center @ 6ba8f893
// ============================================================================

ushort TSA_CoreTouchDim1Center(byte param_1)

{
                    /* 0x4f893  1743  TSA_CoreTouchDim1Center */
  return *(ushort *)(_refptr_g_touchesGrid + (longlong)(int)(uint)param_1 * 0x168 + 0x162) >> 8;
}


// ============================================================================
// Function: TSA_CoreTouchDim2Center @ 6ba8f8c3
// ============================================================================

ushort TSA_CoreTouchDim2Center(byte param_1)

{
                    /* 0x4f8c3  1744  TSA_CoreTouchDim2Center */
  return *(ushort *)(_refptr_g_touchesGrid + (longlong)(int)(uint)param_1 * 0x168 + 0x164) >> 8;
}


// ============================================================================
// Function: TSA_SideTouchDim1Center @ 6ba8f8f3
// ============================================================================

ushort TSA_SideTouchDim1Center(byte param_1)

{
                    /* 0x4f8f3  2320  TSA_SideTouchDim1Center */
  return *(ushort *)(_refptr_g_touchesSide + (longlong)(int)(uint)param_1 * 0x168 + 0x162) >> 8;
}


// ============================================================================
// Function: TSA_SideTouchDim2Center @ 6ba8f923
// ============================================================================

ushort TSA_SideTouchDim2Center(byte param_1)

{
                    /* 0x4f923  2321  TSA_SideTouchDim2Center */
  return *(ushort *)(_refptr_g_touchesSide + (longlong)(int)(uint)param_1 * 0x168 + 0x164) >> 8;
}


// ============================================================================
// Function: TSA_GetYLength @ 6ba9ae31
// ============================================================================

ushort TSA_GetYLength(void)
{
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    return TSA_GetDim2Length();
  } else {
    return TSA_GetDim1Length();
  }
}


// ============================================================================
// Function: TSA_GetASAFrameBypass @ 6ba9d37a
// ============================================================================

undefined8 TSA_GetASAFrameBypass(void)

{
  undefined8 uVar1;
  
                    /* 0x5d37a  1885  TSA_GetASAFrameBypass */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) == 0) {
    uVar1 = 0;
  }
  else {
    uVar1 = ASA_GetFrameBypass();
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_GetDim1Length @ 6ba9addb
// ============================================================================

uchar TSA_GetDim1Length(void)

{
                    /* 0x5addb  1915  TSA_GetDim1Length */
  return _refptr_g_tsaPrmtFlash->bCols;
}


// ============================================================================
// Function: TSA_GetDim2Length @ 6ba9adec
// ============================================================================

uchar TSA_GetDim2Length(void)

{
                    /* 0x5adec  1919  TSA_GetDim2Length */
  return _refptr_g_tsaPrmtFlash->bRows;
}


// ============================================================================
// Function: TSA_IsPadFeatureEnabled @ 6ba9eece
// ============================================================================

undefined4 TSA_IsPadFeatureEnabled(void)

{
  TSA_FeatureFlags TVar1;
  
                    /* 0x5eece  2083  TSA_IsPadFeatureEnabled */
  TVar1 = (*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x800000;
  return CONCAT31((int3)(TVar1 >> 8),TVar1 != 0);
}


// ============================================================================
// Function: TSA_GetMinSigPrpt @ 6baa07dc
// ============================================================================

short TSA_GetMinSigPrpt(void)

{
  short local_a;
  
                    /* 0x607dc  1959  TSA_GetMinSigPrpt */
  local_a = g_tsaPrpt.nSPeakNegative;
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) != 0) {
    local_a = g_tsaPrpt._226_2_;
  }
  return local_a;
}


// ============================================================================
// Function: TSA_Init_UI @ 6ba99d35
// ============================================================================

void TSA_Init_UI(void)

{
                    /* 0x59d35  2040  TSA_Init_UI */
  TSA_MSInit();
  TSA_InitCommonUI();
  return;
}


// ============================================================================
// Function: TSA_InitProject_UI @ 6ba99d4e
// ============================================================================

undefined8 TSA_InitProject_UI(undefined8 param_1)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0x59d4e  2039  TSA_InitProject_UI */
  cVar1 = TSA_MSInitProject(param_1);
  if (cVar1 == '\0') {
    uVar2 = 0;
  }
  else {
    if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) != 0) {
      ASA_InitProjectFromTsa(_refptr_g_tsaPrmtFlash);
      ASA_InitAFEProject(*(undefined8 *)(_refptr_g_thpAFEStruct + 8),_refptr_g_tsaPrmtFlashAsa);
    }
    AFT_Init();
    TSA_InitCommonUI();
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: TSA_InitAFEProject_UI @ 6ba99dc7
// ============================================================================

undefined8 TSA_InitAFEProject_UI(longlong param_1,longlong param_2,longlong param_3)

{
  char cVar1;
  int iVar2;
  undefined8 uVar3;
  
                    /* 0x59dc7  2038  TSA_InitAFEProject_UI */
  if (((param_1 == 0) || (param_2 == 0)) || (param_3 == 0)) {
    uVar3 = 1;
  }
  else {
    iVar2 = TSA_SetAFECaps(param_3);
    if (iVar2 == 1) {
      uVar3 = 1;
    }
    else {
      cVar1 = TSA_InitProject_UI(param_1);
      if (cVar1 == '\0') {
        uVar3 = 1;
      }
      else {
        iVar2 = TSA_SetAFECaps(param_3);
        if (iVar2 == 1) {
          uVar3 = 1;
        }
        else {
          if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) != 0) {
            ASA_InitProjectFromTsa(_refptr_g_tsaPrmtFlash);
            ASA_InitAFEProject(param_3,_refptr_g_tsaPrmtFlashAsa);
          }
          AFT_Init();
          uVar3 = TSA_SetAFEInfo(param_2);
        }
      }
    }
  }
  return uVar3;
}


// ============================================================================
// Function: TSA_InitCommonUI @ 6ba99ce0
// ============================================================================

void TSA_InitCommonUI(void)

{
  TSAPrintf_Init();
  TSALog_Init();
  TSAIDE_Init();
  TSALog_VersionInfo();
  SmartCover_Init();
  UDFPEvent_Init();
  EventAnalyzer_Init();
  TSAPlot_Init();
  AFE_Init();
  RecordTrigger_Init();
  SS_Init();
  TSAWrapper_Init();
  StylusAft_Init();
  TSAFold_Init();
  return;
}


// ============================================================================
// Function: TSA_MSInit @ 6ba90950
// ============================================================================

void TSA_MSInit(void)

{
                    /* 0x50950  2116  TSA_MSInit */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&_rdata,"TSA_MSInit");
  }
  TSAPrmt_Init();
  TSA_MSInitCommon(1);
  return;
}


// ============================================================================
// Function: TSA_MSInitProject @ 6ba90994
// ============================================================================

undefined8 TSA_MSInitProject(undefined8 param_1)

{
  char cVar1;
  int iVar2;
  undefined8 uVar3;
  char *_Str1;
  
                    /* 0x50994  2117  TSA_MSInitProject */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&_rdata,"TSA_MSInitProject");
  }
  cVar1 = TSAPrmt_LoadProject(param_1);
  if (cVar1 == '\0') {
    uVar3 = 0;
  }
  else {
    cVar1 = TSAMem_InitProject();
    if (cVar1 == '\0') {
      uVar3 = 0;
    }
    else {
      Context_Init();
      TSA_MSInitCommon(1);
      _refptr_g_tsaPrpt->field_0x90 = 1;
      _Str1 = (char *)TSAPrmt_GetProjectName();
      iVar2 = strcmp(_Str1,"DFLT000000");
      if (iVar2 == 0) {
        _refptr_g_tsaPrpt->field_0x90 = 0;
      }
      uVar3 = 1;
    }
  }
  return uVar3;
}


// ============================================================================
// Function: TSA_MallocAndInit @ 6baa3430
// ============================================================================

void * TSA_MallocAndInit(uint param_1)

{
  int iVar1;
  void *pvVar2;
  
                    /* 0x63430  2120  TSA_MallocAndInit */
  if ((param_1 == 0) || (0xffff < param_1)) {
    pvVar2 = (void *)0x0;
  }
  else {
    pvVar2 = malloc((ulonglong)param_1);
    if ((pvVar2 != (void *)0x0) && (iVar1 = memset_s(pvVar2,param_1,0,param_1), iVar1 != 0)) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Common/TSA_Malloc.c"
                        ,0x16,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
  }
  return pvVar2;
}


// ============================================================================
// Function: TSA_SetAFECaps @ 6ba9f93c
// ============================================================================

bool TSA_SetAFECaps(longlong param_1)

{
                    /* 0x5f93c  2254  TSA_SetAFECaps */
  if (param_1 != 0) {
    AFE_SetCaps(param_1);
  }
  return param_1 == 0;
}


// ============================================================================
// Function: TSA_SetAFEInfo @ 6ba9f90e
// ============================================================================

bool TSA_SetAFEInfo(longlong param_1)

{
                    /* 0x5f90e  2256  TSA_SetAFEInfo */
  if (param_1 != 0) {
    AFE_SetInfo(param_1);
  }
  return param_1 == 0;
}


// ============================================================================
// Function: TSA_GetFeatures @ 6ba9f329
// ============================================================================

TSA_FeatureFlags TSA_GetFeatures(void)

{
                    /* 0x5f329  1933  TSA_GetFeatures */
  return (*_refptr_g_tsaStaticPtr)->dwFeatureFlags;
}


// ============================================================================
// Function: TSA_SetFeatures @ 6ba9f179
// ============================================================================

undefined4 TSA_SetFeatures(TSA_FeatureFlags param_1)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x18]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x20];
  int iVar1;
  
                    /* 0x5f179  2285  TSA_SetFeatures */
  (*(undefined4 *)(_stk + 0x14)) = 0;
  (*(TSA_FeatureFlags *)(_stk + 0xc)) = param_1 & 0xffffb7ec;
  (*(TSA_FeatureFlags *)(_stk + 0x8)) = param_1 & 0x4813;
  (*(TSA_FeatureFlags *)(_stk + 0x10)) = (*(TSA_FeatureFlags *)(_stk + 0x8));
  iVar1 = TSA_SetGloveFeatures(param_1,&(*(TSA_FeatureFlags *)(_stk + 0xc)),&(*(TSA_FeatureFlags *)(_stk + 0x8)));
  if (iVar1 == 1) {
    (*(undefined4 *)(_stk + 0x14)) = 1;
  }
  iVar1 = TSA_SetSmartCoverFeatures(param_1,&(*(TSA_FeatureFlags *)(_stk + 0xc)),&(*(TSA_FeatureFlags *)(_stk + 0x8)));
  if (iVar1 == 1) {
    (*(undefined4 *)(_stk + 0x14)) = 1;
  }
  iVar1 = TSA_SetGripFilterFeatures(param_1,&(*(TSA_FeatureFlags *)(_stk + 0xc)),&(*(TSA_FeatureFlags *)(_stk + 0x8)));
  if (iVar1 == 1) {
    (*(undefined4 *)(_stk + 0x14)) = 1;
  }
  if ((param_1 & 8) != 0) {
    (*(undefined4 *)(_stk + 0x14)) = 1;
    (*(TSA_FeatureFlags *)(_stk + 0xc)) = (*(TSA_FeatureFlags *)(_stk + 0xc)) & 0xfffffff7;
  }
  iVar1 = TSA_SetStylusFeatures(param_1,&(*(TSA_FeatureFlags *)(_stk + 0xc)),&(*(TSA_FeatureFlags *)(_stk + 0x8)));
  if (iVar1 == 1) {
    (*(undefined4 *)(_stk + 0x14)) = 1;
  }
  iVar1 = TSA_SetUnderWaterFeatures(param_1,&(*(TSA_FeatureFlags *)(_stk + 0xc)),&(*(TSA_FeatureFlags *)(_stk + 0x8)));
  if (iVar1 == 1) {
    (*(undefined4 *)(_stk + 0x14)) = 1;
  }
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlags = (*(TSA_FeatureFlags *)(_stk + 0xc));
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlags = (*(TSA_FeatureFlags *)(_stk + 0x8)) | (*_refptr_g_tsaStaticPtr)->dwFeatureFlags;
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlags =
       _refptr_g_tsaPrmtFlash->dwFeatureFlagsBase1_or | (*_refptr_g_tsaStaticPtr)->dwFeatureFlags;
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlagsMasked = (*(TSA_FeatureFlags *)(_stk + 0x10));
  return (*(undefined4 *)(_stk + 0x14));
}


// ============================================================================
// Function: TSA_ProcessingAFEFrame @ 6ba9a7a7
// ============================================================================

int TSA_ProcessingAFEFrame(AFE_Frame *frame,uint ctrl,ulonglong data)

{
  int iVar1;
  errno_t eVar2;
  longlong lVar3;
  short *local_10;
  
                    /* 0x5a7a7  2123  TSA_ProcessingAFEFrame */
  EventAnalyzer_Reset();
  Context_Reset();
  RecordTrigger_ClearFlag();
  if (frame == (AFE_Frame *)0x0) {
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"\nError:frame == NULL!");
    }
    iVar1 = 1;
  }
  else {
    AFE_SetFrame(frame);
    lVar3 = AFE_GetFrame();
    TSA_SavePreUnifiedRaw(*(undefined8 *)(lVar3 + 0x10),ctrl);
    local_10 = (short *)0x0;
    if (*(longlong *)(lVar3 + 0x10) != 0) {
      eVar2 = memcpy_s(&g_rawDataBuf,0x2648,*(void **)(lVar3 + 0x10),
                       (ulonglong)(byte)_refptr_g_tsaPrmtFlash->field_0x20 *
                       (ulonglong)(byte)_refptr_g_tsaPrmtFlash->field_0x1f * 2);
      if (eVar2 != 0) {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_Main.c",
                          0x288,"rc == EOK");
        HardwareAnalyzer_SetSoftException();
      }
      local_10 = (short *)&g_rawDataBuf;
    }
    TSA_Processing(local_10,ctrl,(void *)data);
    RecordTrigger_Process();
    TSAWrapper_Process();
    StrPrintf_FlushProcess();
    if (*_refptr_g_tsaLogFlagLogCallBackEnabled == '\0') {
      TSAPrintf_FlushLogFile();
    }
    iVar1 = 0;
  }
  return iVar1;
}


// ============================================================================
// Function: TSA_IsNewSafeBaselineAvailable @ 6ba9c554
// ============================================================================

void TSA_IsNewSafeBaselineAvailable(void)

{
                    /* 0x5c554  2075  TSA_IsNewSafeBaselineAvailable */
  TSAMS_IsNewSafeBaselineAvailable();
  return;
}


// ============================================================================
// Function: TSA_GetScenario @ 6ba8f9f1
// ============================================================================

uint TSA_GetScenario(void)

{
                    /* 0x4f9f1  1980  TSA_GetScenario */
  return (*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags;
}


// ============================================================================
// Function: TSA_SetGloveFeatures @ 6ba9ef6b
// ============================================================================

undefined8 TSA_SetGloveFeatures(undefined8 param_1,undefined8 param_2,uint *param_3)

{
  if (((*param_3 ^ (*_refptr_g_tsaStaticPtr)->dwFeatureFlagsMasked) & 1) == 0) {
    if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 1) == 0) {
      *param_3 = *param_3 & 0xfffffffe;
    }
    else {
      *param_3 = *param_3 | 1;
    }
  }
  return 0;
}


// ============================================================================
// Function: TSA_SetSmartCoverFeatures @ 6ba9efdb
// ============================================================================

undefined8 TSA_SetSmartCoverFeatures(undefined8 param_1,undefined8 param_2,uint *param_3)

{
  if (((*param_3 ^ (*_refptr_g_tsaStaticPtr)->dwFeatureFlagsMasked) & 2) == 0) {
    if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) == 0) {
      *param_3 = *param_3 & 0xfffffffd;
    }
    else {
      *param_3 = *param_3 | 2;
    }
  }
  return 0;
}


// ============================================================================
// Function: TSA_SetStylusFeatures @ 6ba9f095
// ============================================================================

undefined8 TSA_SetStylusFeatures(undefined8 param_1,undefined8 param_2,uint *param_3)

{
  if (((*param_3 ^ (*_refptr_g_tsaStaticPtr)->dwFeatureFlagsMasked) & 0x10) == 0) {
    if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x10) == 0) {
      *param_3 = *param_3 & 0xffffffef;
    }
    else {
      *param_3 = *param_3 | 0x10;
    }
  }
  return 0;
}


// ============================================================================
// Function: TSA_SetUnderWaterFeatures @ 6ba9f105
// ============================================================================

undefined8 TSA_SetUnderWaterFeatures(undefined8 param_1,undefined8 param_2,uint *param_3)

{
  if (((*param_3 ^ (*_refptr_g_tsaStaticPtr)->dwFeatureFlagsMasked) & 0x800) == 0) {
    if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_UNDERWATER_DETECT) == 0) {
      *param_3 = *param_3 & 0xfffff7ff;
    }
    else {
      *param_3 = *param_3 | 0x800;
    }
  }
  return 0;
}


// ============================================================================
// Function: TSA_SetGripFilterFeatures @ 6ba9f04b
// ============================================================================

undefined8 TSA_SetGripFilterFeatures(uint param_1,uint *param_2, ...)

{
  int iVar1;
  
  if (((param_1 & 0x20) != 0) && (iVar1 = TSA_EnableGripFilterFeature(), iVar1 == 1)) {
    *param_2 = *param_2 & 0xffffffdf;
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: TSA_SavePreUnifiedRaw @ 6ba9a32b
// ============================================================================

void TSA_SavePreUnifiedRaw(void *param_1,uint param_2)

{
  errno_t eVar1;
  
  if (param_1 != (void *)0x0) {
    if ((param_2 & 0x10000) == 0) {
      eVar1 = memcpy_s(_refptr_g_tsaBufPreUnifiedRaw,0x2648,param_1,
                       (ulonglong)(byte)_refptr_g_tsaPrmtFlash->field_0x20 *
                       (ulonglong)(byte)_refptr_g_tsaPrmtFlash->field_0x1f * 2);
      if (eVar1 != 0) {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_Main.c",
                          0x1a9,"rc == EOK");
        HardwareAnalyzer_SetSoftException();
      }
    }
    else {
      eVar1 = memcpy_s(_refptr_g_tsaBufPreUnifiedDif,0x2648,param_1,
                       (ulonglong)(byte)_refptr_g_tsaPrmtFlash->field_0x20 *
                       (ulonglong)(byte)_refptr_g_tsaPrmtFlash->field_0x1f * 2);
      if (eVar1 != 0) {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_Main.c",
                          0x1a4,"rc == EOK");
        HardwareAnalyzer_SetSoftException();
      }
    }
  }
  return;
}


// ============================================================================
// Function: TSA_Free @ 6baa34c4
// ============================================================================

void TSA_Free(longlong *param_1)

{
                    /* 0x634c4  1856  TSA_Free */
  if (*param_1 != 0) {
    free((void *)*param_1);
    *param_1 = 0;
  }
  return;
}


// ============================================================================
// Function: TSA_EnableGripFilterFeature @ 6ba9ed08
// ============================================================================

undefined8 TSA_EnableGripFilterFeature(void)

{
                    /* 0x5ed08  1843  TSA_EnableGripFilterFeature */
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlags = (*_refptr_g_tsaStaticPtr)->dwFeatureFlags | 0x20;
  return 0;
}


// ============================================================================
// Function: TSA_GetVersion @ 6ba9ab55
// ============================================================================

undefined8 TSA_GetVersion(void)

{
                    /* 0x5ab55  2021  TSA_GetVersion */
  return 0;
}


// ============================================================================
// Function: TSA_InStylusHPP2Mode @ 6ba9c70b
// ============================================================================

bool TSA_InStylusHPP2Mode(void)

{
  bool bVar1;
  
                    /* 0x5c70b  2035  TSA_InStylusHPP2Mode */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) == 0) {
    bVar1 = false;
  }
  else {
    bVar1 = ASA_IsEnterStylusHPP2();
  }
  return bVar1;
}


// ============================================================================
// Function: TSA_SetSensorDataCallBackFunc @ 6ba8f980
// ============================================================================

void TSA_SetSensorDataCallBackFunc(void *param_1)

{
                    /* 0x4f980  2305  TSA_SetSensorDataCallBackFunc */
  (*_refptr_g_tsaStaticPtr)->pSensorDataCallback = param_1;
  return;
}


// ============================================================================
// Function: TSA_SetSensorData @ 6ba8f99f
// ============================================================================

void TSA_SetSensorData(longlong param_1)

{
                    /* 0x4f99f  2304  TSA_SetSensorData */
  (*_refptr_g_tsaStaticPtr)->llField_0x148 = param_1;
  return;
}


// ============================================================================
// Function: TSA_SetScenario @ 6ba8f9d5
// ============================================================================

void TSA_SetScenario(uint param_1)

{
                    /* 0x4f9d5  2300  TSA_SetScenario */
  (*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags = param_1;
  return;
}


// ============================================================================
// Function: TSA_EnableScenario @ 6ba8fa07
// ============================================================================

void TSA_EnableScenario(uint param_1)

{
                    /* 0x4fa07  1847  TSA_EnableScenario */
  (*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags =
       (*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags | param_1;
  return;
}


// ============================================================================
// Function: TSA_DisableScenario @ 6ba8fa33
// ============================================================================

void TSA_DisableScenario(uint param_1)

{
                    /* 0x4fa33  1834  TSA_DisableScenario */
  (*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags =
       (*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags & ~param_1;
  return;
}


// ============================================================================
// Function: TSA_SetAppFeatureCommandEnabled @ 6ba8fa63
// ============================================================================

void TSA_SetAppFeatureCommandEnabled(void)

{
                    /* 0x4fa63  2269  TSA_SetAppFeatureCommandEnabled */
  (*_refptr_g_tsaStaticPtr)->field_0x150 = 1;
  return;
}


// ============================================================================
// Function: TSA_SetAppName @ 6ba8fa7a
// ============================================================================

void TSA_SetAppName(char *param_1)

{
  errno_t eVar1;
  
                    /* 0x4fa7a  2270  TSA_SetAppName */
  eVar1 = strcpy_s((char*)(*_refptr_g_tsaStaticPtr)->raw + 0x164,0x100,param_1);
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_Console.c",
                      0x5c,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: TSA_SetReportRateLevel @ 6ba8fae6
// ============================================================================

void TSA_SetReportRateLevel(undefined1 param_1)

{
                    /* 0x4fae6  2298  TSA_SetReportRateLevel */
  (*_refptr_g_tsaStaticPtr)->field_0x15c = param_1;
  return;
}


// ============================================================================
// Function: TSA_SetAppType @ 6ba8fb02
// ============================================================================

void TSA_SetAppType(undefined4 param_1)

{
                    /* 0x4fb02  2271  TSA_SetAppType */
  *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x160 = param_1;
  return;
}


// ============================================================================
// Function: TSA_ConsoleReset @ 6ba8fb1e
// ============================================================================

void TSA_ConsoleReset(void)

{
                    /* 0x4fb1e  1739  TSA_ConsoleReset */
  (*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags = 1;
  return;
}


// ============================================================================
// Function: TSA_IsTopAppScenario @ 6ba8ff17
// ============================================================================

bool TSA_IsTopAppScenario(void)

{
  char cVar1;
  bool bVar2;
  
                    /* 0x4ff17  2101  TSA_IsTopAppScenario */
  cVar1 = TSA_IsTypingScenario();
  if (cVar1 == '\0') {
    bVar2 = ((*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags & 0x10) != 0;
  }
  else {
    bVar2 = false;
  }
  return bVar2;
}


// ============================================================================
// Function: TSA_RptTZFlagPtr @ 6ba91792
// ============================================================================

void TSA_RptTZFlagPtr(void)

{
                    /* 0x51792  2202  TSA_RptTZFlagPtr */
  TZ_GetAccessFlagPtr();
  return;
}


// ============================================================================
// Function: TSA_GetTZFlagTypeSize @ 6ba917a5
// ============================================================================

undefined8 TSA_GetTZFlagTypeSize(void)

{
                    /* 0x517a5  1998  TSA_GetTZFlagTypeSize */
  return 1;
}


// ============================================================================
// Function: TSA_GetTZFlagTypeSign @ 6ba917b0
// ============================================================================

undefined8 TSA_GetTZFlagTypeSign(void)

{
                    /* 0x517b0  1997  TSA_GetTZFlagTypeSign */
  return 1;
}


// ============================================================================
// Function: TSA_GetGridDataCount @ 6ba99615
// ============================================================================

undefined8 TSA_GetGridDataCount(void)

{
                    /* 0x59615  1936  TSA_GetGridDataCount */
  return 0x5b;
}


// ============================================================================
// Function: TSA_GetGridDataPtr @ 6ba99620
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined8 TSA_GetGridDataPtr(byte param_1)

{
  undefined8 uVar1;
  byte local_res8;
  
                    /* 0x59620  1938  TSA_GetGridDataPtr */
  TSA_GridDataInit();
  local_res8 = param_1;
  if (0x5a < param_1) {
    local_res8 = 0x1a;
  }
  g_indexGridData = local_res8;
  if (local_res8 == 0) {
    uVar1 = _g_ideGridData;
  }
  else {
    uVar1 = TSAFold_GridZoomOutRemap
                      (*(undefined8 *)(&g_ideGridData + (longlong)(int)(uint)local_res8 * 0x88),0,
                       (&DAT_6bc44fac)[(longlong)(int)(uint)local_res8 * 0x88],0);
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_GetGridDataName @ 6ba996d5
// ============================================================================

longlong TSA_GetGridDataName(byte param_1)

{
  byte local_res8;
  
                    /* 0x596d5  1937  TSA_GetGridDataName */
  local_res8 = param_1;
  if (0x5a < param_1) {
    local_res8 = 0x1a;
  }
  return (longlong)(int)(uint)local_res8 * 0x88 + (longlong)TSA_MEM_AT(0x6bc44f48ULL);
}


// ============================================================================
// Function: TSA_GetGridDataTypeSize @ 6ba9970c
// ============================================================================

undefined1 TSA_GetGridDataTypeSize(byte param_1)

{
  byte local_res8;
  
                    /* 0x5970c  1943  TSA_GetGridDataTypeSize */
  local_res8 = param_1;
  if (0x5a < param_1) {
    local_res8 = 0x1a;
  }
  return (&DAT_6bc44fac)[(longlong)(int)(uint)local_res8 * 0x88];
}


// ============================================================================
// Function: TSA_GetGridDataTypeSign @ 6ba99747
// ============================================================================

undefined1 TSA_GetGridDataTypeSign(byte param_1)

{
  byte local_res8;
  
                    /* 0x59747  1942  TSA_GetGridDataTypeSign */
  local_res8 = param_1;
  if (0x5a < param_1) {
    local_res8 = 0x1a;
  }
  return (&DAT_6bc44fad)[(longlong)(int)(uint)local_res8 * 0x88];
}


// ============================================================================
// Function: TSA_GetGridDataSelfPtr @ 6ba99782
// ============================================================================

void TSA_GetGridDataSelfPtr(byte param_1)

{
  byte local_res8;
  
                    /* 0x59782  1939  TSA_GetGridDataSelfPtr */
  if (_refptr_g_tsaPrmtDynamic[0x24] == '\0') {
    if ((*_refptr_g_prevTouchesPtr)->bPrevTouchCount == '\x01') {
      TSAFold_LineZoomOutRemap(_refptr_g_tsaBufSelf + 0x1400,&g_tsaBufFoldLineSelf,4,0);
    }
    else {
      TSAFold_LineZoomOutRemap(_refptr_g_tsaBufSelf,&g_tsaBufFoldLineSelf,4,0);
    }
  }
  else {
    local_res8 = param_1;
    if (0x5a < param_1) {
      local_res8 = 0x1a;
    }
    TSAFold_LineZoomOutRemap
              (*(undefined8 *)(&DAT_6bc44fb0 + (longlong)(int)(uint)local_res8 * 0x88),
               &g_tsaBufFoldLineSelf,4,0);
  }
  return;
}


// ============================================================================
// Function: TSA_GetGridDataStylusTx1Ptr @ 6ba99848
// ============================================================================

void TSA_GetGridDataStylusTx1Ptr(byte param_1)

{
                    /* 0x59848  1940  TSA_GetGridDataStylusTx1Ptr */
  TSA_GridDataInit();
  TSAFold_LineZoomOutRemap
            (*(undefined8 *)(&DAT_6bc44fb8 + (longlong)(int)(uint)param_1 * 0x88),
             &g_tsaBufFoldLineTX1,2,0);
  return;
}


// ============================================================================
// Function: TSA_GetGridDataStylusTx2Ptr @ 6ba998a1
// ============================================================================

void TSA_GetGridDataStylusTx2Ptr(byte param_1)

{
                    /* 0x598a1  1941  TSA_GetGridDataStylusTx2Ptr */
  TSAFold_LineZoomOutRemap
            (*(undefined8 *)(&DAT_6bc44fc0 + (longlong)(int)(uint)param_1 * 0x88),
             &g_tsaBufFoldLineTX2,2,0);
  return;
}


// ============================================================================
// Function: TSA_IsAFESupported @ 6ba9a79c
// ============================================================================

undefined8 TSA_IsAFESupported(void)

{
                    /* 0x5a79c  2041  TSA_IsAFESupported */
  return 1;
}


// ============================================================================
// Function: TSA_SetFrame @ 6ba9a903
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void TSA_SetFrame(undefined8 *param_1)

{
                    /* 0x5a903  2287  TSA_SetFrame */
  memcpy_s(&g_frameExt, sizeof(AFE_Frame), param_1, sizeof(AFE_Frame));
  return;
}


// ============================================================================
// Function: TSA_SetGridData @ 6ba9a991
// ============================================================================

void TSA_SetGridData(void *param_1)

{
                    /* 0x5a991  2288  TSA_SetGridData */
  g_frameExt.pRawData = param_1;
  return;
}


// ============================================================================
// Function: TSA_SetLineData @ 6ba9a9a6
// ============================================================================

void TSA_SetLineData(void *param_1)

{
                    /* 0x5a9a6  2290  TSA_SetLineData */
  g_frameExt.pLineData = param_1;
  return;
}


// ============================================================================
// Function: TSA_SetUsTimeData @ 6ba9a9bb
// ============================================================================

void TSA_SetUsTimeData(ulonglong param_1)

{
                    /* 0x5a9bb  2313  TSA_SetUsTimeData */
  g_frameExt.nUnixTimeSec = (int)(param_1 / 1000000);
  g_frameExt.nUnixTimeSubSec = (int)(param_1 % 1000000);
  return;
}


// ============================================================================
// Function: TSA_SetMsTimeData @ 6ba9aa11
// ============================================================================

void TSA_SetMsTimeData(ulonglong param_1)

{
                    /* 0x5aa11  2295  TSA_SetMsTimeData */
  g_frameExt.nUnixTimeSec = (int)(param_1 / 1000);
  g_frameExt.nUnixTimeSubSec = ((int)param_1 + (int)(param_1 / 1000) * -1000) * 1000;
  return;
}


// ============================================================================
// Function: TSA_SetStylusData @ 6ba9aa75
// ============================================================================

void TSA_SetStylusData(stylusFrame *param_1)

{
                    /* 0x5aa75  2308  TSA_SetStylusData */
  g_frameExt.pStylusData = param_1;
  return;
}


// ============================================================================
// Function: TSA_SetStylusDataType @ 6ba9aa8a
// ============================================================================

void TSA_SetStylusDataType(undefined1 param_1)

{
                    /* 0x5aa8a  2309  TSA_SetStylusDataType */
  ASA_SetStylusDataType(param_1);
  return;
}


// ============================================================================
// Function: TSA_UpdateFrameInfo @ 6ba9aaa9
// ============================================================================

void TSA_UpdateFrameInfo(ushort param_1,byte param_2,uint param_3)

{
  g_frameExt.wScanFreqIndex = param_1;
  g_frameExt.wScanRateIndex = (ushort)param_2;
  g_frameExt.dwFlags = param_3;
  return;
}


// ============================================================================
// Function: TSA_ProcessingExt @ 6ba9aadb
// ============================================================================

int TSA_ProcessingExt(ulong gridData,uint ctrl,ulong data,ushort field_88,byte field_28,
                     uint field_30)

{
  int iVar1;
  undefined4 in_register_0000000c;
  undefined4 in_register_00000084;
  
                    /* 0x5aadb  2124  TSA_ProcessingExt */
  TSA_SetGridData(CONCAT44(in_register_0000000c,gridData));
  TSA_UpdateFrameInfo(field_88,field_28,field_30);
  iVar1 = TSA_ProcessingAFEFrame(&g_frameExt,ctrl,CONCAT44(in_register_00000084,data));
  return iVar1;
}


// ============================================================================
// Function: TSA_IsDone @ 6ba9ab37
// ============================================================================

bool TSA_IsDone(void)

{
                    /* 0x5ab37  2057  TSA_IsDone */
  return ((*_refptr_g_tsaStaticPtr)->dwProcessingState & PROC_TSA_ACTIVE) == 0;
}


// ============================================================================
// Function: TSA_SetAfeHwVer @ 6ba9ab81
// ============================================================================

void TSA_SetAfeHwVer(undefined4 param_1)

{
                    /* 0x5ab81  2267  TSA_SetAfeHwVer */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) != 0) {
    ASA_SetAfeHwVer(param_1);
  }
  return;
}


// ============================================================================
// Function: TSA_SetDdicSn @ 6ba9abb4
// ============================================================================

void TSA_SetDdicSn(undefined8 param_1,undefined2 param_2)

{
                    /* 0x5abb4  2276  TSA_SetDdicSn */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) != 0) {
    ASA_SetDdicSn(param_1,param_2);
  }
  return;
}


// ============================================================================
// Function: TSA_GetMaxSensorNum @ 6ba9abf5
// ============================================================================

undefined8 TSA_GetMaxSensorNum(void)

{
                    /* 0x5abf5  1958  TSA_GetMaxSensorNum */
  return 0x1324;
}


// ============================================================================
// Function: TSA_GetMaxDim1Length @ 6ba9ac00
// ============================================================================

undefined8 TSA_GetMaxDim1Length(void)

{
                    /* 0x5ac00  1955  TSA_GetMaxDim1Length */
  return 0x50;
}


// ============================================================================
// Function: TSA_GetMaxDim2Length @ 6ba9ac0b
// ============================================================================

undefined8 TSA_GetMaxDim2Length(void)

{
                    /* 0x5ac0b  1956  TSA_GetMaxDim2Length */
  return 0x50;
}


// ============================================================================
// Function: TSA_GetMaxFingerSupported @ 6ba9ac16
// ============================================================================

undefined8 TSA_GetMaxFingerSupported(void)

{
                    /* 0x5ac16  1957  TSA_GetMaxFingerSupported */
  return 0x14;
}


// ============================================================================
// Function: TSA_GetFingerSupported @ 6ba9ac21
// ============================================================================

undefined8 TSA_GetFingerSupported(void)

{
                    /* 0x5ac21  1934  TSA_GetFingerSupported */
  return 0x14;
}


// ============================================================================
// Function: TSA_SetFingerSupported @ 6ba9ac2c
// ============================================================================

undefined8 TSA_SetFingerSupported(void)

{
                    /* 0x5ac2c  2286  TSA_SetFingerSupported */
  return 0;
}


// ============================================================================
// Function: TSA_GetMSRawTypeSize @ 6ba9ac3c
// ============================================================================

undefined8 TSA_GetMSRawTypeSize(void)

{
                    /* 0x5ac3c  1951  TSA_GetMSRawTypeSize */
  return 2;
}


// ============================================================================
// Function: TSA_GetMSRawTypeSign @ 6ba9ac47
// ============================================================================

undefined8 TSA_GetMSRawTypeSign(void)

{
                    /* 0x5ac47  1950  TSA_GetMSRawTypeSign */
  return 0;
}


// ============================================================================
// Function: TSA_GetMSDiffTypeSize @ 6ba9ac52
// ============================================================================

undefined8 TSA_GetMSDiffTypeSize(void)

{
                    /* 0x5ac52  1948  TSA_GetMSDiffTypeSize */
  return 2;
}


// ============================================================================
// Function: TSA_GetMSDiffTypeSign @ 6ba9ac5d
// ============================================================================

undefined8 TSA_GetMSDiffTypeSign(void)

{
                    /* 0x5ac5d  1947  TSA_GetMSDiffTypeSign */
  return 1;
}


// ============================================================================
// Function: TSA_GetXLength @ 6ba9adfd
// ============================================================================

void TSA_GetXLength(void)

{
                    /* 0x5adfd  2026  TSA_GetXLength */
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    TSA_GetDim1Length();
  }
  else {
    TSA_GetDim2Length();
  }
  return;
}


// ============================================================================
// Function: TSA_GetDim1Pitch @ 6ba9ae65
// ============================================================================

undefined1 TSA_GetDim1Pitch(void)

{
                    /* 0x5ae65  1917  TSA_GetDim1Pitch */
  return _refptr_g_tsaPrmtFlash->field_0x1b;
}


// ============================================================================
// Function: TSA_GetDim2Pitch @ 6ba9ae76
// ============================================================================

undefined1 TSA_GetDim2Pitch(void)

{
                    /* 0x5ae76  1921  TSA_GetDim2Pitch */
  return _refptr_g_tsaPrmtFlash->field_0x1c;
}


// ============================================================================
// Function: TSA_SetDim1Length @ 6ba9ae87
// ============================================================================

undefined8 TSA_SetDim1Length(void)

{
                    /* 0x5ae87  2277  TSA_SetDim1Length */
  return 0;
}


// ============================================================================
// Function: TSA_SetDim2Length @ 6ba9ae97
// ============================================================================

undefined8 TSA_SetDim2Length(void)

{
                    /* 0x5ae97  2279  TSA_SetDim2Length */
  return 0;
}


// ============================================================================
// Function: TSA_GetDim2PhyRes @ 6ba9aebb
// ============================================================================

undefined2 TSA_GetDim2PhyRes(void)

{
                    /* 0x5aebb  1920  TSA_GetDim2PhyRes */
  return *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5ac;
}


// ============================================================================
// Function: TSA_SetResScale @ 6ba9af5f
// ============================================================================

undefined8 TSA_SetResScale(undefined2 param_1)

{
                    /* 0x5af5f  2299  TSA_SetResScale */
  *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a0 = param_1;
  TSAFold_PrmtSetResScale(param_1);
  TSAConstPrmt_Init();
  return 0;
}


// ============================================================================
// Function: TSA_SetDim1Res @ 6ba9af9a
// ============================================================================

undefined8 TSA_SetDim1Res(undefined2 param_1)

{
                    /* 0x5af9a  2278  TSA_SetDim1Res */
  *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a6 = param_1;
  TSAFold_PrmtSetDim1Res(param_1);
  TSAConstPrmt_Init();
  return 0;
}


// ============================================================================
// Function: TSA_SetDim2Res @ 6ba9afd5
// ============================================================================

undefined8 TSA_SetDim2Res(undefined2 param_1)

{
                    /* 0x5afd5  2280  TSA_SetDim2Res */
  *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a8 = param_1;
  TSAFold_PrmtSetDim2Res(param_1);
  TSAConstPrmt_Init();
  return 0;
}


// ============================================================================
// Function: TSA_SetXRes @ 6ba9b010
// ============================================================================

void TSA_SetXRes(undefined2 param_1)

{
                    /* 0x5b010  2316  TSA_SetXRes */
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    TSA_SetDim1Res(param_1);
  }
  else {
    TSA_SetDim2Res(param_1);
  }
  return;
}


// ============================================================================
// Function: TSA_SetYRes @ 6ba9b056
// ============================================================================

void TSA_SetYRes(undefined2 param_1)

{
                    /* 0x5b056  2317  TSA_SetYRes */
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    TSA_SetDim2Res(param_1);
  }
  else {
    TSA_SetDim1Res(param_1);
  }
  return;
}


// ============================================================================
// Function: TSA_RptTouchZoneNum @ 6ba9b0d7
// ============================================================================

undefined1 TSA_RptTouchZoneNum(void)

{
                    /* 0x5b0d7  2244  TSA_RptTouchZoneNum */
  return *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 2);
}


// ============================================================================
// Function: TSA_RptUserStatus @ 6ba9b0ff
// ============================================================================

undefined4 TSA_RptUserStatus(void)

{
                    /* 0x5b0ff  2245  TSA_RptUserStatus */
  return *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x18);
}


// ============================================================================
// Function: TSA_RptIsTouchDown @ 6ba9b112
// ============================================================================

uint TSA_RptIsTouchDown(byte param_1)

{
  uint uVar1;
  
                    /* 0x5b112  2178  TSA_RptIsTouchDown */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x1000000) == 0) {
    uVar1 = *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x21c) & 2;
  }
  else {
    uVar1 = *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x21c) & 0x1e;
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_RptIsTrueTouchDown @ 6ba9b184
// ============================================================================

uint TSA_RptIsTrueTouchDown(byte param_1)

{
                    /* 0x5b184  2180  TSA_RptIsTrueTouchDown */
  return *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x290)
         & 2;
}


// ============================================================================
// Function: TSA_RptIsLiftOff @ 6ba9b1b4
// ============================================================================

uint TSA_RptIsLiftOff(byte param_1)

{
  uint uVar1;
  
                    /* 0x5b1b4  2175  TSA_RptIsLiftOff */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x1000000) == 0) {
    uVar1 = *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x21c) & 0x20;
  }
  else {
    uVar1 = *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x21c) & 0x40;
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_RptIsTrueLiftOff @ 6ba9b226
// ============================================================================

uint TSA_RptIsTrueLiftOff(byte param_1)

{
                    /* 0x5b226  2179  TSA_RptIsTrueLiftOff */
  return *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x290)
         & 0x20;
}


// ============================================================================
// Function: TSA_RptTouchShape @ 6ba9b2c4
// ============================================================================

undefined4 TSA_RptTouchShape(byte param_1)

{
                    /* 0x5b2c4  2226  TSA_RptTouchShape */
  return *(undefined4 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1e0);
}


// ============================================================================
// Function: TSA_RptTouchObject @ 6ba9b36a
// ============================================================================

undefined4 TSA_RptTouchObject(byte param_1)

{
                    /* 0x5b36a  2222  TSA_RptTouchObject */
  return *(undefined4 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1e8);
}


// ============================================================================
// Function: TSA_RptTouchEvent @ 6ba9b3cc
// ============================================================================

undefined4 TSA_RptTouchEvent(byte param_1)

{
                    /* 0x5b3cc  2213  TSA_RptTouchEvent */
  return *(undefined4 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c);
}


// ============================================================================
// Function: TSA_RptTouchException @ 6ba9b3fa
// ============================================================================

undefined4 TSA_RptTouchException(byte param_1)

{
                    /* 0x5b3fa  2215  TSA_RptTouchException */
  return *(undefined4 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1ec);
}


// ============================================================================
// Function: TSA_RptTouchXWidth @ 6ba9b4dc
// ============================================================================

byte TSA_RptTouchXWidth(byte param_1)

{
  byte bVar1;
  
                    /* 0x5b4dc  2237  TSA_RptTouchXWidth */
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    bVar1 = *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1ac);
    if (0xf < bVar1) {
      bVar1 = 0xf;
    }
  }
  else {
    bVar1 = *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1ad);
    if (0xf < bVar1) {
      bVar1 = 0xf;
    }
  }
  return bVar1;
}


// ============================================================================
// Function: TSA_RptTouchYWidth @ 6ba9b55f
// ============================================================================

byte TSA_RptTouchYWidth(byte param_1)

{
  byte bVar1;
  
                    /* 0x5b55f  2243  TSA_RptTouchYWidth */
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    bVar1 = *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1ad);
    if (0xf < bVar1) {
      bVar1 = 0xf;
    }
  }
  else {
    bVar1 = *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1ac);
    if (0xf < bVar1) {
      bVar1 = 0xf;
    }
  }
  return bVar1;
}


// ============================================================================
// Function: TSA_RptTouchXEdgeWidth @ 6ba9b5e2
// ============================================================================

byte TSA_RptTouchXEdgeWidth(byte param_1)

{
  byte bVar1;
  
                    /* 0x5b5e2  2233  TSA_RptTouchXEdgeWidth */
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    bVar1 = *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1b0);
    if (0xf < bVar1) {
      bVar1 = 0xf;
    }
  }
  else {
    bVar1 = *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1b1);
    if (0xf < bVar1) {
      bVar1 = 0xf;
    }
  }
  return bVar1;
}


// ============================================================================
// Function: TSA_RptTouchYEdgeWidth @ 6ba9b664
// ============================================================================

byte TSA_RptTouchYEdgeWidth(byte param_1)

{
  byte bVar1;
  
                    /* 0x5b664  2239  TSA_RptTouchYEdgeWidth */
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    bVar1 = *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1b1);
    if (0xf < bVar1) {
      bVar1 = 0xf;
    }
  }
  else {
    bVar1 = *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1b0);
    if (0xf < bVar1) {
      bVar1 = 0xf;
    }
  }
  return bVar1;
}


// ============================================================================
// Function: TSA_RptTouchXGripRatio @ 6ba9b6e6
// ============================================================================

ulonglong TSA_RptTouchXGripRatio(byte param_1)

{
  ulonglong uVar1;
  ushort local_a;
  
                    /* 0x5b6e6  2235  TSA_RptTouchXGripRatio */
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    local_a = *(ushort *)
               ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1a8);
  }
  else {
    local_a = *(ushort *)
               ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1aa);
  }
  local_a = local_a >> 2;
  if (local_a < 0xff) {
    uVar1 = (ulonglong)local_a;
  }
  else {
    uVar1 = 0xffffffff;
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_RptTouchYGripRatio @ 6ba9b780
// ============================================================================

ulonglong TSA_RptTouchYGripRatio(byte param_1)

{
  ulonglong uVar1;
  ushort local_a;
  
                    /* 0x5b780  2241  TSA_RptTouchYGripRatio */
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    local_a = *(ushort *)
               ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1aa);
  }
  else {
    local_a = *(ushort *)
               ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1a8);
  }
  local_a = local_a >> 2;
  if (local_a < 0xff) {
    uVar1 = (ulonglong)local_a;
  }
  else {
    uVar1 = 0xffffffff;
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_RptTouchXGripCtd @ 6ba9b81a
// ============================================================================

undefined1 TSA_RptTouchXGripCtd(byte param_1)

{
  undefined1 uVar1;
  
                    /* 0x5b81a  2234  TSA_RptTouchXGripCtd */
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    uVar1 = *(undefined1 *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x196);
  }
  else {
    uVar1 = *(undefined1 *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x197);
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_RptTouchGripRatio @ 6ba9b92c
// ============================================================================

undefined1 TSA_RptTouchGripRatio(byte param_1)

{
                    /* 0x5b92c  2218  TSA_RptTouchGripRatio */
  return *(undefined1 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x198);
}


// ============================================================================
// Function: TSA_RptTouchGripLevel @ 6ba9b95b
// ============================================================================

undefined1 TSA_RptTouchGripLevel(byte param_1)

{
                    /* 0x5b95b  2217  TSA_RptTouchGripLevel */
  return *(undefined1 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x199);
}


// ============================================================================
// Function: TSA_RptTouchDim1Peak @ 6ba9ba7a
// ============================================================================

uchar TSA_RptTouchDim1Peak(byte param_1)

{
  uchar uVar1;
  
                    /* 0x5ba7a  2209  TSA_RptTouchDim1Peak */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_AFT_ENABLED) == 0) {
    uVar1 = (*_refptr_g_peaksPtr)
            [(longlong)
             (int)(uint)*(byte *)((longlong)(int)(uint)param_1 * 0x168 +
                                  *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x15].
            bPeakCount;
  }
  else {
    uVar1 = (*_refptr_g_peaksPtr)
            [(longlong)
             (int)(uint)*(byte *)((longlong)
                                  *(int *)(_refptr_g_aftOutSortInfo +
                                          (longlong)(int)(uint)param_1 * 0x30 + 4) * 0x168 +
                                  *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x15].
            bPeakCount;
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_RptTouchDim2Peak @ 6ba9bb57
// ============================================================================

uchar TSA_RptTouchDim2Peak(byte param_1)

{
  uchar uVar1;
  
                    /* 0x5bb57  2211  TSA_RptTouchDim2Peak */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_AFT_ENABLED) == 0) {
    uVar1 = (*_refptr_g_peaksPtr)
            [(longlong)
             (int)(uint)*(byte *)((longlong)(int)(uint)param_1 * 0x168 +
                                  *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x16].
            bPeakCount;
  }
  else {
    uVar1 = (*_refptr_g_peaksPtr)
            [(longlong)
             (int)(uint)*(byte *)((longlong)
                                  *(int *)(_refptr_g_aftOutSortInfo +
                                          (longlong)(int)(uint)param_1 * 0x30 + 4) * 0x168 +
                                  *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x16].
            bPeakCount;
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_RptTouchToBeGoBack @ 6ba9bebd
// ============================================================================

undefined4 TSA_RptTouchToBeGoBack(byte param_1)

{
  uint uVar1;
  
                    /* 0x5bebd  2230  TSA_RptTouchToBeGoBack */
  uVar1 = *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8
                   ) & 0x800000;
  return CONCAT31((int3)(uVar1 >> 8),uVar1 != 0);
}


// ============================================================================
// Function: TSA_RptIsExistExtROIPeak @ 6ba9bf76
// ============================================================================

void TSA_RptIsExistExtROIPeak(void)

{
                    /* 0x5bf76  2174  TSA_RptIsExistExtROIPeak */
  TSAMS_IsExistExtROIPeak();
  return;
}


// ============================================================================
// Function: TSA_RptExtROIPeakDim1 @ 6ba9bf89
// ============================================================================

void TSA_RptExtROIPeakDim1(void)

{
                    /* 0x5bf89  2170  TSA_RptExtROIPeakDim1 */
  TSAMS_GetExtROIPeakDim1();
  return;
}


// ============================================================================
// Function: TSA_RptExtROIPeakDim2 @ 6ba9bf9c
// ============================================================================

void TSA_RptExtROIPeakDim2(void)

{
                    /* 0x5bf9c  2171  TSA_RptExtROIPeakDim2 */
  TSAMS_GetExtROIPeakDim2();
  return;
}


// ============================================================================
// Function: TSA_RptIsMatchWithROI @ 6ba9bfaf
// ============================================================================

void TSA_RptIsMatchWithROI(void)

{
                    /* 0x5bfaf  2176  TSA_RptIsMatchWithROI */
  TSAMS_RptIsMatchWithROI();
  return;
}


// ============================================================================
// Function: TSA_RptTouchSizeInMM @ 6ba9bff1
// ============================================================================

undefined1 TSA_RptTouchSizeInMM(byte param_1)

{
                    /* 0x5bff1  2228  TSA_RptTouchSizeInMM */
  return *(undefined1 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x194);
}


// ============================================================================
// Function: TSA_RptSQDistMMFromStartPos @ 6ba9c020
// ============================================================================

undefined2 TSA_RptSQDistMMFromStartPos(byte param_1)

{
                    /* 0x5c020  2193  TSA_RptSQDistMMFromStartPos */
  return *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x250);
}


// ============================================================================
// Function: TSA_RptSQDistMMFromPrevPos @ 6ba9c04e
// ============================================================================

undefined2 TSA_RptSQDistMMFromPrevPos(byte param_1)

{
                    /* 0x5c04e  2192  TSA_RptSQDistMMFromPrevPos */
  return *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x252);
}


// ============================================================================
// Function: TSA_RptSQDistDisplayPixelFromPrevPos @ 6ba9c07d
// ============================================================================

undefined4 TSA_RptSQDistDisplayPixelFromPrevPos(byte param_1)

{
                    /* 0x5c07d  2191  TSA_RptSQDistDisplayPixelFromPrevPos */
  return *(undefined4 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x254);
}


// ============================================================================
// Function: TSA_IsFalseFlickDetected @ 6ba9c0ab
// ============================================================================

undefined4 TSA_IsFalseFlickDetected(void)

{
  uint uVar1;
  undefined4 uVar2;
  
                    /* 0x5c0ab  2061  TSA_IsFalseFlickDetected */
  if (**(char **)_refptr_g_touchesPtr == '\0') {
    uVar2 = 0;
  }
  else {
    uVar1 = *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x1f0) & 0x100000;
    uVar2 = CONCAT31((int3)(uVar1 >> 8),uVar1 != 0);
  }
  return uVar2;
}


// ============================================================================
// Function: TSA_IsFlickCheckPassed @ 6ba9c0e8
// ============================================================================

void TSA_IsFlickCheckPassed(void)

{
                    /* 0x5c0e8  2062  TSA_IsFlickCheckPassed */
  TouchAction_IsFlickCheckPassed();
  return;
}


// ============================================================================
// Function: TSA_IsERCheckPassedButDisabled @ 6ba9c0fb
// ============================================================================

void TSA_IsERCheckPassedButDisabled(void)

{
                    /* 0x5c0fb  2058  TSA_IsERCheckPassedButDisabled */
  ER_GetFlagMoveInCorrectedButDisabled();
  return;
}


// ============================================================================
// Function: TSA_GetERLimit @ 6ba9c10e
// ============================================================================

void TSA_GetERLimit(void)

{
                    /* 0x5c10e  1925  TSA_GetERLimit */
  ER_GetLimit();
  return;
}


// ============================================================================
// Function: TSA_GetTouchShapeStr @ 6ba9c121
// ============================================================================

undefined * TSA_GetTouchShapeStr(undefined1 param_1)

{
  undefined *puVar1;
  
                    /* 0x5c121  2008  TSA_GetTouchShapeStr */
  switch(param_1) {
  default:
    puVar1 = &DAT_6bb53aef;
    break;
  case 1:
    puVar1 = &DAT_6bb53af1;
    break;
  case 2:
    puVar1 = &DAT_6bb53af4;
    break;
  case 3:
    puVar1 = &DAT_6bb53af7;
    break;
  case 4:
    puVar1 = &DAT_6bb53afa;
    break;
  case 5:
    puVar1 = &DAT_6bb53afd;
  }
  return puVar1;
}


// ============================================================================
// Function: TSA_RptTouchShapeStr @ 6ba9c18c
// ============================================================================

void TSA_RptTouchShapeStr(byte param_1)

{
                    /* 0x5c18c  2227  TSA_RptTouchShapeStr */
  TSA_GetTouchShapeStr
            (*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x1e0) & 0xff);
  return;
}


// ============================================================================
// Function: TSA_GetTouchObjectStr @ 6ba9c1cb
// ============================================================================

undefined * TSA_GetTouchObjectStr()

{
  int iVar1;
  undefined *puVar2;
  
                    /* 0x5c1cb  2007  TSA_GetTouchObjectStr */
  iVar1 = *(int *)&_refptr_g_tsaPrpt[1].field_0x2c;
  if (iVar1 == 2) {
    puVar2 = &DAT_6bb53b1c;
  }
  else if (iVar1 == 3) {
    puVar2 = &DAT_6bb53b1e;
  }
  else if (iVar1 == 1) {
    puVar2 = &DAT_6bb53b1a;
  }
  else {
    puVar2 = &DAT_6bb53b18;
  }
  return puVar2;
}


// ============================================================================
// Function: TSA_RptTouchObjectStr @ 6ba9c214
// ============================================================================

void TSA_RptTouchObjectStr(byte param_1)

{
                    /* 0x5c214  2223  TSA_RptTouchObjectStr */
  TSA_GetTouchObjectStr
            (*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x1e8) & 0xff);
  return;
}


// ============================================================================
// Function: TSA_GetTouchExceptionStr @ 6ba9c254
// ============================================================================

undefined * TSA_GetTouchExceptionStr(ushort param_1)

{
                    /* 0x5c254  2004  TSA_GetTouchExceptionStr */
  if (param_1 == 0x20) {
    return &DAT_6bb53b30;
  }
  if (param_1 < 0x21) {
    if (param_1 == 4) {
      return &DAT_6bb53b26;
    }
    if (param_1 < 5) {
      if (param_1 == 2) {
        return &DAT_6bb53b22;
      }
    }
    else {
      if (param_1 == 8) {
        return &DAT_6bb53b29;
      }
      if (param_1 == 0x10) {
        return &DAT_6bb53b2c;
      }
    }
  }
  else {
    if (param_1 == 0x80) {
      return &DAT_6bb53b37;
    }
    if (param_1 < 0x81) {
      if (param_1 == 0x40) {
        return &DAT_6bb53b34;
      }
    }
    else {
      if (param_1 == 0x100) {
        return &DAT_6bb53b3a;
      }
      if (param_1 == 0x200) {
        return &DAT_6bb53b3d;
      }
    }
  }
  return &DAT_6bb53b20;
}


// ============================================================================
// Function: TSA_GetTouchCancelStr @ 6ba9c306
// ============================================================================

undefined * TSA_GetTouchCancelStr(undefined1 param_1)

{
  uint uVar1;
  undefined *puVar2;
  
                    /* 0x5c306  2002  TSA_GetTouchCancelStr */
  uVar1 = TSA_RptTouchAxisMajorExtend(param_1);
  if ((uVar1 & 0x8000000) == 0) {
    puVar2 = &DAT_6bb53b42;
  }
  else {
    puVar2 = &DAT_6bb53b40;
  }
  return puVar2;
}


// ============================================================================
// Function: TSA_RptTouchExceptionStr @ 6ba9c33d
// ============================================================================

void TSA_RptTouchExceptionStr(byte param_1)

{
                    /* 0x5c33d  2216  TSA_RptTouchExceptionStr */
  TSA_GetTouchExceptionStr
            (*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x1ec) & 0xffff);
  return;
}


// ============================================================================
// Function: TSA_GetTouchEventStr @ 6ba9c37d
// ============================================================================

undefined * TSA_GetTouchEventStr(undefined1 param_1)

{
  undefined *puVar1;
  
                    /* 0x5c37d  2003  TSA_GetTouchEventStr */
  switch(param_1) {
  default:
    puVar1 = &DAT_6bb53b20;
    break;
  case 1:
    puVar1 = &DAT_6bb53b4a;
    break;
  case 2:
    puVar1 = &DAT_6bb53b44;
    break;
  case 4:
    puVar1 = &DAT_6bb53b46;
    break;
  case 8:
    puVar1 = &DAT_6bb53b4d;
    break;
  case 0x10:
    puVar1 = &DAT_6bb53b50;
    break;
  case 0x20:
    puVar1 = &DAT_6bb53b48;
  }
  return puVar1;
}


// ============================================================================
// Function: TSA_RptTouchEventStr @ 6ba9c3f1
// ============================================================================

void TSA_RptTouchEventStr(byte param_1)

{
                    /* 0x5c3f1  2214  TSA_RptTouchEventStr */
  TSA_GetTouchEventStr
            (*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x21c) & 0xff);
  return;
}


// ============================================================================
// Function: TSA_RptTouchVolatileFlag @ 6ba9c431
// ============================================================================

undefined4 TSA_RptTouchVolatileFlag(byte param_1)

{
                    /* 0x5c431  2231  TSA_RptTouchVolatileFlag */
  return *(undefined4 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0);
}


// ============================================================================
// Function: TSA_RptTouchPersistFlag @ 6ba9c45e
// ============================================================================

undefined4 TSA_RptTouchPersistFlag(byte param_1)

{
                    /* 0x5c45e  2224  TSA_RptTouchPersistFlag */
  return *(undefined4 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8);
}


// ============================================================================
// Function: TSA_RptTouchEdgeFlag @ 6ba9c48c
// ============================================================================

undefined4 TSA_RptTouchEdgeFlag(byte param_1)

{
                    /* 0x5c48c  2212  TSA_RptTouchEdgeFlag */
  return *(undefined4 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x204);
}


// ============================================================================
// Function: TSA_RptTouchSuppressFlag @ 6ba9c4ba
// ============================================================================

undefined4 TSA_RptTouchSuppressFlag(byte param_1)

{
                    /* 0x5c4ba  2229  TSA_RptTouchSuppressFlag */
  return *(undefined4 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208);
}


// ============================================================================
// Function: TSA_RptRawPtr @ 6ba9c4e8
// ============================================================================

undefined8 TSA_RptRawPtr(void)

{
                    /* 0x5c4e8  2188  TSA_RptRawPtr */
  return *(undefined8 *)_refptr_g_tsaBufRaw;
}


// ============================================================================
// Function: TSA_RptPreCMFRawPtr @ 6ba9c4f8
// ============================================================================

undefined8 TSA_RptPreCMFRawPtr(void)

{
                    /* 0x5c4f8  2184  TSA_RptPreCMFRawPtr */
  return *(undefined8 *)_refptr_g_tsaBufPreCMFRaw;
}


// ============================================================================
// Function: TSA_RptPreRawPtr @ 6ba9c508
// ============================================================================

void TSA_RptPreRawPtr(void)

{
                    /* 0x5c508  2187  TSA_RptPreRawPtr */
  TSAMS_GetPreRawPtr();
  return;
}


// ============================================================================
// Function: TSA_RptPreFilteredRawPtr @ 6ba9c51b
// ============================================================================

void TSA_RptPreFilteredRawPtr(void)

{
                    /* 0x5c51b  2186  TSA_RptPreFilteredRawPtr */
  TSAMS_GetPreFilteredRawPtr();
  return;
}


// ============================================================================
// Function: TSA_RptPreFilteredDifPtr @ 6ba9c52e
// ============================================================================

undefined8 TSA_RptPreFilteredDifPtr(void)

{
                    /* 0x5c52e  2185  TSA_RptPreFilteredDifPtr */
  return TSAMS_GetPreFilteredDifPtr();
}


// ============================================================================
// Function: TSA_RptBaselinePtr @ 6ba9c541
// ============================================================================

undefined8 TSA_RptBaselinePtr(void)

{
                    /* 0x5c541  2163  TSA_RptBaselinePtr */
  return (undefined8)TSAMS_GetBLPtr();
}


// ============================================================================
// Function: TSA_RptSafeBaselinePtr @ 6ba9c567
// ============================================================================

void TSA_RptSafeBaselinePtr(undefined8 param_1)

{
                    /* 0x5c567  2196  TSA_RptSafeBaselinePtr */
  TSAMS_GetSafeBLPtr(param_1);
  return;
}


// ============================================================================
// Function: TSA_RptNormalBaselinePtr @ 6ba9c582
// ============================================================================

void TSA_RptNormalBaselinePtr(void)

{
                    /* 0x5c582  2183  TSA_RptNormalBaselinePtr */
  TSAMS_GetNormalBLPtr();
  return;
}


// ============================================================================
// Function: TSA_RptSSBaselinePtr @ 6ba9c595
// ============================================================================

void TSA_RptSSBaselinePtr(void)

{
                    /* 0x5c595  2194  TSA_RptSSBaselinePtr */
  TSAMS_GetSSPreUnifiedBLPtr();
  return;
}


// ============================================================================
// Function: TSA_RptDiffPtr @ 6ba9c5a8
// ============================================================================

undefined8 TSA_RptDiffPtr(void)

{
                    /* 0x5c5a8  2167  TSA_RptDiffPtr */
  return TSAMS_GetDifPtr();
}


// ============================================================================
// Function: TSA_RptSSDiffPtr @ 6ba9c5bb
// ============================================================================

void TSA_RptSSDiffPtr(void)

{
                    /* 0x5c5bb  2195  TSA_RptSSDiffPtr */
  TSAMS_GetSSDifPtr();
  return;
}


// ============================================================================
// Function: TSA_RptSDPtr @ 6ba9c5ce
// ============================================================================

void TSA_RptSDPtr(void)

{
                    /* 0x5c5ce  2190  TSA_RptSDPtr */
  TSAMS_GetSDPtr();
  return;
}


// ============================================================================
// Function: TSA_RptSelfDiffPtr @ 6ba9c5e1
// ============================================================================

void TSA_RptSelfDiffPtr(void)

{
                    /* 0x5c5e1  2201  TSA_RptSelfDiffPtr */
  TSAMS_GetSelfDifPtr();
  return;
}


// ============================================================================
// Function: TSA_RptCMNPtr @ 6ba9c5f4
// ============================================================================

undefined8 TSA_RptCMNPtr(void)

{
                    /* 0x5c5f4  2164  TSA_RptCMNPtr */
  return (undefined8)TSAMS_GetCMNPtr();
}


// ============================================================================
// Function: TSA_InStylusHPP3Mode @ 6ba9c73e
// ============================================================================

undefined8 TSA_InStylusHPP3Mode(void)

{
  undefined8 uVar1;
  
                    /* 0x5c73e  2036  TSA_InStylusHPP3Mode */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) == 0) {
    uVar1 = 0;
  }
  else {
    uVar1 = ASA_IsEnterStylusHPP3();
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_IsBaselineOK @ 6ba9d776
// ============================================================================

void TSA_IsBaselineOK(void)

{
                    /* 0x5d776  2053  TSA_IsBaselineOK */
  TSAMS_IsBaselineOK();
  return;
}


// ============================================================================
// Function: TSA_DetectedSDLevel @ 6ba9d7a1
// ============================================================================

uint TSA_DetectedSDLevel(void)

{
                    /* 0x5d7a1  1826  TSA_DetectedSDLevel */
  return *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x20) & 8;
}


// ============================================================================
// Function: TSA_DetectedWaterLevel @ 6ba9d7b7
// ============================================================================

uint TSA_DetectedWaterLevel(void)

{
                    /* 0x5d7b7  1827  TSA_DetectedWaterLevel */
  return *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x20) & 0x40;
}


// ============================================================================
// Function: TSA_IsWakeupDetected @ 6ba9d7cd
// ============================================================================

uint TSA_IsWakeupDetected(void)

{
                    /* 0x5d7cd  2110  TSA_IsWakeupDetected */
  return *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x20) & 0x200;
}


// ============================================================================
// Function: TSA_IsWetFingerDetected @ 6ba9d815
// ============================================================================

uint TSA_IsWetFingerDetected(void)

{
                    /* 0x5d815  2111  TSA_IsWetFingerDetected */
  return *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x20) & 0x80;
}


// ============================================================================
// Function: TSA_DetectedESDLevel @ 6ba9d82d
// ============================================================================

undefined8 TSA_DetectedESDLevel(void)

{
                    /* 0x5d82d  1823  TSA_DetectedESDLevel */
  return 0;
}


// ============================================================================
// Function: TSA_DetectedHardwareException @ 6ba9d838
// ============================================================================

uint TSA_DetectedHardwareException(void)

{
  bool bVar1;
  
                    /* 0x5d838  1824  TSA_DetectedHardwareException */
  if ((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) != 0) &&
     (bVar1 = TSA_InStylusHPP2Mode(), bVar1)) {
    return 0;
  }
  return *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x20) & 0x400;
}


// ============================================================================
// Function: TSA_GetDMDString @ 6ba9da2d
// ============================================================================

char * TSA_GetDMDString(uint param_1)

{
                    /* 0x5da2d  1913  TSA_GetDMDString */
  if (param_1 == 0x4000) {
    return "TSA_DMD_ABNORMAL_TOUCH";
  }
  if (param_1 < 0x4001) {
    if (param_1 == 0x40) {
      return "TSA_DMD_UNDER_WATER";
    }
    if (param_1 < 0x41) {
      if (param_1 == 4) {
        return "TSA_DMD_SENSOR_BROKEN";
      }
      if (param_1 < 5) {
        if (param_1 == 1) {
          return "TSA_DMD_RF_NOISE";
        }
        if (param_1 == 0) {
          return "TSA_DMD_NONE";
        }
        if (param_1 == 2) {
          return "TSA_DMD_RAW_JUMP";
        }
      }
      else {
        if (param_1 == 0x10) {
          return "TSA_DMD_OTHER";
        }
        if (param_1 == 0x20) {
          return "TSA_DMD_RAW_OVER_LIMIT";
        }
        if (param_1 == 8) {
          return "TSA_DMD_ASSERTED";
        }
      }
    }
    else {
      if (param_1 == 0x400) {
        return "TSA_DMD_LOW_SENSITIVITY";
      }
      if (param_1 < 0x401) {
        if (param_1 == 0x100) {
          return "TSA_DMD_ALG_TIME_OUT";
        }
        if (param_1 == 0x200) {
          return "TSA_DMD_FORCE_EXIT_IDLE";
        }
        if (param_1 == 0x80) {
          return "TSA_DMD_CALIBRATION";
        }
      }
      else {
        if (param_1 == 0x1000) {
          return "TSA_DMD_FRAME_TIMEOUT";
        }
        if (param_1 == 0x2000) {
          return "TSA_DMD_THENAR_LONG_PRESS";
        }
        if (param_1 == 0x800) {
          return "TSA_DMD_LOW_TEMPERATURE";
        }
      }
    }
  }
  else {
    if (param_1 == 0x400000) {
      return "TSA_DMD_LINE_DATA_RAWJUMP";
    }
    if (param_1 < 0x400001) {
      if (param_1 == 0x40000) {
        return "TSA_DMD_SIDE_TOUCH";
      }
      if (param_1 < 0x40001) {
        if (param_1 == 0x10000) {
          return "TSA_DMD_PROXIMITY_ON_TIME_OUT";
        }
        if (param_1 == 0x20000) {
          return "TSA_DMD_PROXIMITY_OFF_TIME_OUT";
        }
        if (param_1 == 0x8000) {
          return "TSA_DMD_NOISY";
        }
      }
      else {
        if (param_1 == 0x100000) {
          return "TSA_DMD_ALGO_EXP1";
        }
        if (param_1 == 0x200000) {
          return "TSA_DMD_HOPPING_RAWJUMP";
        }
        if (param_1 == 0x80000) {
          return "TSA_DMD_AFE_RAW_ABNORMAL";
        }
      }
    }
    else {
      if (param_1 == 0x4000000) {
        return "TSA_DMD_AS_LARGE_NOISE";
      }
      if (param_1 < 0x4000001) {
        if (param_1 == 0x1000000) {
          return "TSA_DMD_AS_ABNORMAL_MODE_SWITCH";
        }
        if (param_1 == 0x2000000) {
          return "TSA_DMD_AS_ABNORMAL_FREQ_HOP";
        }
        if (param_1 == 0x800000) {
          return "TSA_DMD_AS_BL_TIMEOUT";
        }
      }
      else {
        if (param_1 == 0x10000000) {
          return "TSA_DMD_SIDETOUCH_EXCP1";
        }
        if (param_1 < 0x10000001) {
          if (param_1 == 0x8000000) {
            return "TSA_DMD_AS_LARGE_NOISE2";
          }
        }
        else {
          if (param_1 == 0x20000000) {
            return "TSA_DMD_SIDETOUCH_EXCP2";
          }
          if (param_1 == 0x40000000) {
            return "TSA_DMD_SIDETOUCH_EXCP3";
          }
        }
      }
    }
  }
  return "Unsupported DMD num";
}


// ============================================================================
// Function: TSA_GetEventString @ 6ba9dd4b
// ============================================================================

char * TSA_GetEventString(uint param_1)

{
                    /* 0x5dd4b  1929  TSA_GetEventString */
  if (param_1 == 0x80) {
    return "TSA_EVENT_PROXIMITY_DETECTED";
  }
  if (param_1 < 0x81) {
    if (param_1 == 8) {
      return "TSA_EVENT_THENAR_LONG_PRESS_ON_LEFT";
    }
    if (param_1 < 9) {
      if (param_1 == 2) {
        return "TSA_EVENT_PHONE_HELD_ON_LEFT_HAND";
      }
      if (param_1 == 4) {
        return "TSA_EVENT_PHONE_HELD_ON_RIGHT_HAND";
      }
      if (param_1 == 0) {
        return "TSA_EVENT_NONE";
      }
    }
    else {
      if (param_1 == 0x20) {
        return "TSA_EVENT_PHONE_UNDER_WATER";
      }
      if (param_1 == 0x40) {
        return "TSA_EVENT_PHONE_OUTOF_WATER";
      }
      if (param_1 == 0x10) {
        return "TSA_EVENT_THENAR_LONG_PRESS_ON_RIGHT";
      }
    }
  }
  else {
    if (param_1 == 0x800) {
      return "TSA_EVENT_RIGHT_VOLUME_TRIGGER";
    }
    if (param_1 < 0x801) {
      if (param_1 == 0x200) {
        return "TSA_EVENT_PROXIMITY_PUT_DOWN_GESTURE_DETECTED";
      }
      if (param_1 == 0x400) {
        return "TSA_EVENT_LEFT_VOLUME_TRIGGER";
      }
      if (param_1 == 0x100) {
        return "TSA_EVENT_PROXIMITY_PICK_UP_GESTURE_DETECTED";
      }
    }
    else {
      if (param_1 == 0x2000) {
        return "TSA_EVENT_VOLUME_DOWN";
      }
      if (param_1 < 0x2001) {
        if (param_1 == 0x1000) {
          return "TSA_EVENT_VOLUME_UP";
        }
      }
      else {
        if (param_1 == 0x4000) {
          return "TSA_EVENT_VOLUME_MUTE";
        }
        if (param_1 == 0x8000) {
          return "TSA_EVENT_POWER_DOUBLE_CLICK";
        }
      }
    }
  }
  return "Unsupported TSA Event enumerator";
}


// ============================================================================
// Function: TSA_GetMouseGestureString @ 6ba9dec8
// ============================================================================

char * TSA_GetMouseGestureString(int param_1)

{
                    /* 0x5dec8  1961  TSA_GetMouseGestureString */
  if (param_1 == 0x4000) {
    return "SWIPE_UP";
  }
  if (param_1 < 0x4001) {
    if (param_1 == 0x20) {
      return "ON_LEFT_CLICK_REGION";
    }
    if (param_1 < 0x21) {
      if (param_1 == 2) {
        return "DOUBLE_CLICK";
      }
      if (param_1 < 3) {
        if (param_1 == 0) {
          return "NONE";
        }
        if (0 < param_1) {
          return "CLICK";
        }
        if (param_1 == -0x80000000) {
          return "SWIPE_DOWN_AND_HOLD";
        }
      }
      else {
        if (param_1 == 8) {
          return "DRAG";
        }
        if (param_1 == 0x10) {
          return "BACK";
        }
        if (param_1 == 4) {
          return "LONG_PRESS";
        }
      }
    }
    else {
      if (param_1 == 0x200) {
        return "SCROLL_DOWN";
      }
      if (param_1 < 0x201) {
        if (param_1 == 0x80) {
          return "MIDDLE_CLICK";
        }
        if (param_1 == 0x100) {
          return "SCROLL_UP";
        }
        if (param_1 == 0x40) {
          return "ON_RIGHT_CLICK_REGION";
        }
      }
      else {
        if (param_1 == 0x800) {
          return "SCROLL_RIGHT";
        }
        if (param_1 < 0x801) {
          if (param_1 == 0x400) {
            return "SCROLL_LEFT";
          }
        }
        else {
          if (param_1 == 0x1000) {
            return "SWIPE_LEFT";
          }
          if (param_1 == 0x2000) {
            return "SWIPE_RIGHT";
          }
        }
      }
    }
  }
  else {
    if (param_1 == 0x400000) {
      return "DOUBLE_SWIPE_RIGHT";
    }
    if (param_1 < 0x400001) {
      if (param_1 == 0x40000) {
        return "DOUBLE_SWIPE_DOWN";
      }
      if (param_1 < 0x40001) {
        if (param_1 == 0x10000) {
          return "SWIPE_UP_AND_HOLD";
        }
        if (param_1 == 0x20000) {
          return "DOUBLE_SWIPE_UP";
        }
        if (param_1 == 0x8000) {
          return "SWIPE_DOWN";
        }
      }
      else {
        if (param_1 == 0x100000) {
          return "DOUBLE_SWIPE_DOWN_AND_HOLD";
        }
        if (param_1 == 0x200000) {
          return "DOUBLE_SWIPE_LEFT";
        }
        if (param_1 == 0x80000) {
          return "DOUBLE_SWIPE_UP_AND_HOLD";
        }
      }
    }
    else {
      if (param_1 == 0x4000000) {
        return "TRIPLE_SWIPE_LEFT";
      }
      if (param_1 < 0x4000001) {
        if (param_1 == 0x1000000) {
          return "SPREAD";
        }
        if (param_1 == 0x2000000) {
          return "ROTATE";
        }
        if (param_1 == 0x800000) {
          return "PINCH";
        }
      }
      else {
        if (param_1 == 0x10000000) {
          return "TRIPLE_CLICKS";
        }
        if (param_1 < 0x10000001) {
          if (param_1 == 0x8000000) {
            return "TRIPLE_SWIPE_RIGHT";
          }
        }
        else {
          if (param_1 == 0x20000000) {
            return "ROTATE_AND_PINCH";
          }
          if (param_1 == 0x40000000) {
            return "ROTATE_AND_SPREAD";
          }
        }
      }
    }
  }
  return "Unsupported TSA mouse gesture enumerator\n";
}


// ============================================================================
// Function: TSA_GetMaxContinuousCMFNoiseFrameNum @ 6ba9e26d
// ============================================================================

ushort TSA_GetMaxContinuousCMFNoiseFrameNum(void)

{
                    /* 0x5e26d  1954  TSA_GetMaxContinuousCMFNoiseFrameNum */
  return (*_refptr_g_tsaStaticPtr)->wField_0x120;
}


// ============================================================================
// Function: TSA_DetectedBendingException @ 6ba9e297
// ============================================================================

uint TSA_DetectedBendingException(void)

{
                    /* 0x5e297  1822  TSA_DetectedBendingException */
  return *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x20) & 4;
}


// ============================================================================
// Function: TSA_IsFreqHopNeeded @ 6ba9e2ad
// ============================================================================

bool TSA_IsFreqHopNeeded(void)

{
  int iVar1;
  
                    /* 0x5e2ad  2063  TSA_IsFreqHopNeeded */
  iVar1 = TSA_DetectedNoiseLevel();
  return iVar1 != 0;
}


// ============================================================================
// Function: TSA_IsAllFreqNoisy @ 6ba9e2c5
// ============================================================================

void TSA_IsAllFreqNoisy(void)

{
                    /* 0x5e2c5  2052  TSA_IsAllFreqNoisy */
  TSAStatic_IsAllFreqNoisy();
  return;
}


// ============================================================================
// Function: TSA_IsReCalibDetected @ 6ba9e2d8
// ============================================================================

bool TSA_IsReCalibDetected(void)

{
                    /* 0x5e2d8  2090  TSA_IsReCalibDetected */
  return (*(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x20) & 0x1000) != 0;
}


// ============================================================================
// Function: TSA_IsFreqShiftSupported @ 6ba9e308
// ============================================================================

bool TSA_IsFreqShiftSupported(void)

{
  char cVar1;
  
                    /* 0x5e308  2064  TSA_IsFreqShiftSupported */
  cVar1 = AFE_IsManualFreqShift();
  return cVar1 != '\0';
}


// ============================================================================
// Function: TSA_IsReCalibNeeded @ 6ba9e32b
// ============================================================================

static char flagOnceOnly_13269 = '\x01';

undefined1 TSA_IsReCalibNeeded(void)

{
  bool bVar1;
  char cVar2;
  undefined1 local_9;
  
                    /* 0x5e32b  2091  TSA_IsReCalibNeeded */
  local_9 = 0;
  if ((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) != 0) &&
     (bVar1 = TSA_InStylusHPP2Mode(), bVar1)) {
    return 0;
  }
  if (((flagOnceOnly_13269 != '\0') && (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 1) != 0)) &&
     (cVar2 = AFE_IsAutoCalibration(), cVar2 == '\0')) {
    local_9 = 1;
    flagOnceOnly_13269 = '\0';
  }
  cVar2 = TSA_IsReCalibDetected();
  if (cVar2 != '\0') {
    local_9 = 1;
  }
  cVar2 = AFE_IsIndividualBaselineForEachFreq();
  if ((cVar2 != '\0') && (cVar2 = BLReset_Status(), cVar2 == '\x01')) {
    local_9 = 1;
  }
  return local_9;
}


// ============================================================================
// Function: TSA_IsOKToResetIDLEBaseline @ 6ba9e7d5
// ============================================================================

undefined8 TSA_IsOKToResetIDLEBaseline(void)

{
  char cVar1;
  
                    /* 0x5e7d5  2080  TSA_IsOKToResetIDLEBaseline */
  if ((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) != 0) &&
     (cVar1 = TSA_InStylusMode(), cVar1 != '\0')) {
    return 0;
  }
  cVar1 = TSAStatic_IsJustOKToEnterIDLEMode();
  if ((cVar1 == '\0') && (cVar1 = TSAStatic_IsJustOKToEnterSideIDLEMode(), cVar1 == '\0')) {
    return 0;
  }
  return 1;
}


// ============================================================================
// Function: TSA_IsOKToDisableFreqShift @ 6ba9e82a
// ============================================================================

bool TSA_IsOKToDisableFreqShift(void)

{
  char cVar1;
  char cVar2;
  bool local_9;
  
                    /* 0x5e82a  2077  TSA_IsOKToDisableFreqShift */
  cVar1 = AFE_IsInAutoCheckProcess();
  cVar2 = Proximity_IsToDisableFreqShift();
  local_9 = cVar2 != '\0' || cVar1 != '\0';
  return local_9;
}


// ============================================================================
// Function: TSA_IsOKToSuspendAFE @ 6ba9e85a
// ============================================================================

void TSA_IsOKToSuspendAFE(void)

{
                    /* 0x5e85a  2081  TSA_IsOKToSuspendAFE */
  Proximity_IsToSuspendAFE();
  return;
}


// ============================================================================
// Function: TSA_GetProjectName @ 6ba9e8ea
// ============================================================================

char * TSA_GetProjectName(void)

{
                    /* 0x5e8ea  1974  TSA_GetProjectName */
  return "THP/DFT/TSACore";
}


// ============================================================================
// Function: TSA_GetOSName @ 6ba9e904
// ============================================================================

char * TSA_GetOSName(void)

{
                    /* 0x5e904  1969  TSA_GetOSName */
  return "Windows";
}


// ============================================================================
// Function: TSA_GetTouchModeStage @ 6ba9e937
// ============================================================================

void TSA_GetTouchModeStage(void)

{
                    /* 0x5e937  2006  TSA_GetTouchModeStage */
  TSAMS_GetTouchModeStage();
  return;
}


// ============================================================================
// Function: TSA_GetTouchSize @ 6ba9e94a
// ============================================================================

void TSA_GetTouchSize(void)

{
                    /* 0x5e94a  2010  TSA_GetTouchSize */
  TSAMS_GetTouchSize();
  return;
}


// ============================================================================
// Function: TSA_GetTZThold @ 6ba9e981
// ============================================================================

undefined2 TSA_GetTZThold(byte param_1)

{
                    /* 0x5e981  1999  TSA_GetTZThold */
  return *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x28a);
}


// ============================================================================
// Function: TSA_GetSigSum @ 6ba9e9b0
// ============================================================================

undefined4 TSA_GetSigSum(byte param_1)

{
                    /* 0x5e9b0  1991  TSA_GetSigSum */
  return *(undefined4 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1cc);
}


// ============================================================================
// Function: TSA_GetZ9 @ 6ba9e9de
// ============================================================================

undefined4 TSA_GetZ9(byte param_1)

{
                    /* 0x5e9de  2031  TSA_GetZ9 */
  return *(undefined4 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1d4);
}


// ============================================================================
// Function: TSA_GetZ4 @ 6ba9ea0c
// ============================================================================

undefined4 TSA_GetZ4(byte param_1)

{
                    /* 0x5ea0c  2030  TSA_GetZ4 */
  return *(undefined4 *)
          (*_refptr_g_peaksPtr +
          (longlong)
          (int)(uint)*(byte *)((longlong)(int)(uint)param_1 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x20);
}


// ============================================================================
// Function: TSA_DisableGloveFeature @ 6ba9eb1a
// ============================================================================

void TSA_DisableGloveFeature(void)

{
                    /* 0x5eb1a  1831  TSA_DisableGloveFeature */
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlags = (*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0xfffffffe
  ;
  return;
}


// ============================================================================
// Function: TSA_IsGloveFeatureEnabled @ 6ba9eb43
// ============================================================================

bool TSA_IsGloveFeatureEnabled(void)

{
                    /* 0x5eb43  2066  TSA_IsGloveFeatureEnabled */
  return ((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 1) != 0;
}


// ============================================================================
// Function: TSA_EnableStylusFeature @ 6ba9eb61
// ============================================================================

bool TSA_EnableStylusFeature(void)

{
  bool bVar1;
  
                    /* 0x5eb61  1850  TSA_EnableStylusFeature */
  bVar1 = (_refptr_g_tsaPrmtDynamic[0x80] & 8) == 0;
  if (!bVar1) {
    (*_refptr_g_tsaStaticPtr)->dwFeatureFlags = (*_refptr_g_tsaStaticPtr)->dwFeatureFlags | 0x10;
  }
  return bVar1;
}


// ============================================================================
// Function: TSA_DisableStylusFeature @ 6ba9ebae
// ============================================================================

void TSA_DisableStylusFeature(void)

{
                    /* 0x5ebae  1837  TSA_DisableStylusFeature */
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlags = (*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0xffffffef
  ;
  return;
}


// ============================================================================
// Function: TSA_IsStylusFeatureEnabled @ 6ba9ebd7
// ============================================================================

bool TSA_IsStylusFeatureEnabled(void)

{
                    /* 0x5ebd7  2096  TSA_IsStylusFeatureEnabled */
  return ((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x10) != 0;
}


// ============================================================================
// Function: TSA_SetSmartCoverRange @ 6ba9ebf5
// ============================================================================

undefined8
TSA_SetSmartCoverRange(undefined2 param_1,undefined2 param_2,undefined2 param_3,undefined2 param_4)

{
                    /* 0x5ebf5  2307  TSA_SetSmartCoverRange */
  SmartCover_SetRange(param_1,param_2,param_3,param_4);
  return 0;
}


// ============================================================================
// Function: TSA_EnableSmartCoverFeature @ 6ba9ec93
// ============================================================================

undefined8 TSA_EnableSmartCoverFeature(void)

{
                    /* 0x5ec93  1849  TSA_EnableSmartCoverFeature */
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlags =
       (*_refptr_g_tsaStaticPtr)->dwFeatureFlags | FEAT_SMART_COVER;
  return 0;
}


// ============================================================================
// Function: TSA_DisableSmartCoverFeature @ 6ba9ecc1
// ============================================================================

void TSA_DisableSmartCoverFeature(void)

{
                    /* 0x5ecc1  1836  TSA_DisableSmartCoverFeature */
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlags =
       (*_refptr_g_tsaStaticPtr)->dwFeatureFlags & ~FEAT_SMART_COVER;
  return;
}


// ============================================================================
// Function: TSA_IsSmartCoverFeatureEnabled @ 6ba9ecea
// ============================================================================

bool TSA_IsSmartCoverFeatureEnabled(void)

{
                    /* 0x5ecea  2095  TSA_IsSmartCoverFeatureEnabled */
  return ((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) != 0;
}


// ============================================================================
// Function: TSA_DisableGripFilterFeature @ 6ba9ed36
// ============================================================================

void TSA_DisableGripFilterFeature(void)

{
                    /* 0x5ed36  1832  TSA_DisableGripFilterFeature */
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlags = (*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0xffffffdf
  ;
  return;
}


// ============================================================================
// Function: TSA_IsGripFilterFeatureEnabled @ 6ba9ed5f
// ============================================================================

bool TSA_IsGripFilterFeatureEnabled(void)

{
                    /* 0x5ed5f  2067  TSA_IsGripFilterFeatureEnabled */
  return ((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x20) != 0;
}


// ============================================================================
// Function: TSA_EnableUnderWaterFeature @ 6ba9ed7d
// ============================================================================

undefined8 TSA_EnableUnderWaterFeature(void)

{
                    /* 0x5ed7d  1851  TSA_EnableUnderWaterFeature */
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlags =
       (*_refptr_g_tsaStaticPtr)->dwFeatureFlags | FEAT_UNDERWATER_DETECT;
  return 0;
}


// ============================================================================
// Function: TSA_DisableUnderWaterFeature @ 6ba9edab
// ============================================================================

void TSA_DisableUnderWaterFeature(void)

{
                    /* 0x5edab  1838  TSA_DisableUnderWaterFeature */
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlags =
       (*_refptr_g_tsaStaticPtr)->dwFeatureFlags & ~FEAT_UNDERWATER_DETECT;
  return;
}


// ============================================================================
// Function: TSA_IsUnderWaterFeatureEnabled @ 6ba9edd4
// ============================================================================

undefined4 TSA_IsUnderWaterFeatureEnabled(void)

{
  TSA_FeatureFlags TVar1;
  
                    /* 0x5edd4  2106  TSA_IsUnderWaterFeatureEnabled */
  TVar1 = (*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_UNDERWATER_DETECT;
  return CONCAT31((int3)(TVar1 >> 8),TVar1 != 0);
}


// ============================================================================
// Function: TSA_EnableActiveStylusFeature @ 6ba9edf4
// ============================================================================

undefined8 TSA_EnableActiveStylusFeature(void)

{
                    /* 0x5edf4  1840  TSA_EnableActiveStylusFeature */
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlags =
       (*_refptr_g_tsaStaticPtr)->dwFeatureFlags | FEAT_ASA_ENABLED;
  return 0;
}


// ============================================================================
// Function: TSA_DisableActiveStylusFeature @ 6ba9ee25
// ============================================================================

void TSA_DisableActiveStylusFeature(void)

{
                    /* 0x5ee25  1829  TSA_DisableActiveStylusFeature */
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlags =
       (*_refptr_g_tsaStaticPtr)->dwFeatureFlags & ~FEAT_ASA_ENABLED;
  return;
}


// ============================================================================
// Function: TSA_IsActiveStylusFeatureEnabled @ 6ba9ee51
// ============================================================================

undefined4 TSA_IsActiveStylusFeatureEnabled(void)

{
  TSA_FeatureFlags TVar1;
  
                    /* 0x5ee51  2051  TSA_IsActiveStylusFeatureEnabled */
  TVar1 = (*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED;
  return CONCAT31((int3)(TVar1 >> 8),TVar1 != 0);
}


// ============================================================================
// Function: TSA_EnablePadFeature @ 6ba9ee71
// ============================================================================

undefined8 TSA_EnablePadFeature(void)

{
                    /* 0x5ee71  1844  TSA_EnablePadFeature */
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlags = (*_refptr_g_tsaStaticPtr)->dwFeatureFlags | 0x800000;
  return 0;
}


// ============================================================================
// Function: TSA_DisablePadFeature @ 6ba9eea2
// ============================================================================

void TSA_DisablePadFeature(void)

{
                    /* 0x5eea2  1833  TSA_DisablePadFeature */
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlags = (*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0xff7fffff
  ;
  return;
}


// ============================================================================
// Function: TSA_EnableWindowsPadFeature @ 6ba9eeee
// ============================================================================

undefined8 TSA_EnableWindowsPadFeature(void)

{
                    /* 0x5eeee  1853  TSA_EnableWindowsPadFeature */
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlags = (*_refptr_g_tsaStaticPtr)->dwFeatureFlags | 0x1000000;
  return 0;
}


// ============================================================================
// Function: TSA_DisableWindowsPadFeature @ 6ba9ef1f
// ============================================================================

void TSA_DisableWindowsPadFeature(void)

{
                    /* 0x5ef1f  1839  TSA_DisableWindowsPadFeature */
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlags = (*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0xfeffffff
  ;
  return;
}


// ============================================================================
// Function: TSA_SetExternalFeatures @ 6ba9f2eb
// ============================================================================

void TSA_SetExternalFeatures(TSA_FeatureFlags param_1)

{
                    /* 0x5f2eb  2284  TSA_SetExternalFeatures */
  TSA_SetFeatures(param_1 & 0x4813 | (*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0xffffb7ec);
  return;
}


// ============================================================================
// Function: TSA_EnableEdgePeakFilterWorkaround @ 6ba9f33f
// ============================================================================

undefined8 TSA_EnableEdgePeakFilterWorkaround(void)

{
                    /* 0x5f33f  1841  TSA_EnableEdgePeakFilterWorkaround */
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlags2 = (*_refptr_g_tsaStaticPtr)->dwFeatureFlags2 | 1;
  return 0;
}


// ============================================================================
// Function: TSA_DisableEdgePeakFilterWorkaround @ 6ba9f36d
// ============================================================================

void TSA_DisableEdgePeakFilterWorkaround(void)

{
                    /* 0x5f36d  1830  TSA_DisableEdgePeakFilterWorkaround */
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlags2 =
       (*_refptr_g_tsaStaticPtr)->dwFeatureFlags2 & 0xfffffffe;
  return;
}


// ============================================================================
// Function: TSA_IsEdgePeakFilterWorkaroundEnabled @ 6ba9f396
// ============================================================================

bool TSA_IsEdgePeakFilterWorkaroundEnabled(void)

{
                    /* 0x5f396  2059  TSA_IsEdgePeakFilterWorkaroundEnabled */
  return ((*_refptr_g_tsaStaticPtr)->dwFeatureFlags2 & 1) != 0;
}


// ============================================================================
// Function: TSA_SetWorkarounds @ 6ba9f3b4
// ============================================================================

undefined4 TSA_SetWorkarounds(uint param_1)

{
  int iVar1;
  uint local_res8;
  undefined4 local_c;
  
                    /* 0x5f3b4  2314  TSA_SetWorkarounds */
  local_c = 0;
  local_res8 = param_1;
  if ((param_1 & 1) != 0) {
    iVar1 = TSA_EnableEdgePeakFilterWorkaround();
    if (iVar1 == 1) {
      local_c = 1;
      local_res8 = param_1 & 0xfffffffe;
    }
  }
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlags2 = local_res8;
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlags2 =
       _refptr_g_tsaPrmtFlash->dwFeatureFlagsBase2_or | (*_refptr_g_tsaStaticPtr)->dwFeatureFlags2;
  return local_c;
}


// ============================================================================
// Function: TSA_GetWorkarounds @ 6ba9f42d
// ============================================================================

uint TSA_GetWorkarounds(void)

{
                    /* 0x5f42d  2025  TSA_GetWorkarounds */
  return (*_refptr_g_tsaStaticPtr)->dwFeatureFlags2;
}


// ============================================================================
// Function: TSA_SetShbLogCallback @ 6ba9f443
// ============================================================================

undefined8 TSA_SetShbLogCallback(void)

{
                    /* 0x5f443  2306  TSA_SetShbLogCallback */
  return 1;
}


// ============================================================================
// Function: TSA_GripClickNum @ 6ba9f452
// ============================================================================

undefined8 TSA_GripClickNum(void)

{
  undefined8 uVar1;
  
                    /* 0x5f452  2032  TSA_GripClickNum */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x20) == 0) {
    uVar1 = 0;
  }
  else {
    uVar1 = GripFilter_ClickNum();
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_GripNum @ 6ba9f483
// ============================================================================

undefined8 TSA_GripNum(void)

{
  undefined8 uVar1;
  
                    /* 0x5f483  2034  TSA_GripNum */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x20) == 0) {
    uVar1 = 0;
  }
  else {
    uVar1 = GripFilter_GripNum();
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_GripFlickNum @ 6ba9f4b4
// ============================================================================

undefined8 TSA_GripFlickNum(void)

{
  undefined8 uVar1;
  
                    /* 0x5f4b4  2033  TSA_GripFlickNum */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x20) == 0) {
    uVar1 = 0;
  }
  else {
    uVar1 = GripFilter_FlickNum();
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_SetBaselineDataCallbackFunc @ 6ba9f4e5
// ============================================================================

undefined8 TSA_SetBaselineDataCallbackFunc(void)

{
                    /* 0x5f4e5  2272  TSA_SetBaselineDataCallbackFunc */
  return 1;
}


// ============================================================================
// Function: TSA_SetSysTraceFunc @ 6ba9f4f8
// ============================================================================

undefined8 TSA_SetSysTraceFunc(void)

{
                    /* 0x5f4f8  2310  TSA_SetSysTraceFunc */
  return 1;
}


// ============================================================================
// Function: TSA_RetrieveBaseline @ 6ba9f50b
// ============================================================================

void TSA_RetrieveBaseline(longlong param_1,longlong param_2)

{
                    /* 0x5f50b  2136  TSA_RetrieveBaseline */
  if ((param_1 != 0) && (param_2 != 0)) {
    Rawdata_RetrieveBaseline
              (param_1,_refptr_g_tsaPrmtFlash->wBufElementCount,param_2,
               _refptr_g_tsaPrmtFlash->wBufElementCount,0,0);
  }
  return;
}


// ============================================================================
// Function: TSA_RetrieveBaseline2 @ 6ba9f57e
// ============================================================================

void TSA_RetrieveBaseline2(longlong param_1,longlong param_2,longlong param_3)

{
                    /* 0x5f57e  2137  TSA_RetrieveBaseline2 */
  if (((param_1 != 0) && (param_2 != 0)) && (param_3 != 0)) {
    Rawdata_RetrieveBaseline
              (param_1,_refptr_g_tsaPrmtFlash->wBufElementCount,param_2,
               _refptr_g_tsaPrmtFlash->wBufElementCount,param_3,
               _refptr_g_tsaPrmtFlash->wBufElementCount);
  }
  return;
}


// ============================================================================
// Function: TSA_IsPreNthFrameTriggered @ 6ba9f60d
// ============================================================================

void TSA_IsPreNthFrameTriggered(void)

{
                    /* 0x5f60d  2088  TSA_IsPreNthFrameTriggered */
  RecordTrigger_IsTriggered2();
  return;
}


// ============================================================================
// Function: TSA_GetTriggeredFrameIndex @ 6ba9f620
// ============================================================================

void TSA_GetTriggeredFrameIndex(void)

{
                    /* 0x5f620  2016  TSA_GetTriggeredFrameIndex */
  RecordTrigger_TriggerIndex();
  return;
}


// ============================================================================
// Function: TSA_GetTriggerEventOffset @ 6ba9f633
// ============================================================================

void TSA_GetTriggerEventOffset(void)

{
                    /* 0x5f633  2015  TSA_GetTriggerEventOffset */
  RecordTrigger_GetEventOffset();
  return;
}


// ============================================================================
// Function: TSA_GetTriggerEventDescription @ 6ba9f646
// ============================================================================

void TSA_GetTriggerEventDescription(void)

{
                    /* 0x5f646  2014  TSA_GetTriggerEventDescription */
  RecordTrigger_GetEventDescription();
  return;
}


// ============================================================================
// Function: TSA_IsHighSensitivityMode @ 6ba9f685
// ============================================================================

undefined8 TSA_IsHighSensitivityMode(void)

{
  undefined8 uVar1;
  
                    /* 0x5f685  2071  TSA_IsHighSensitivityMode */
  if ((((2 >> ((byte)*(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x2c & 0x1f) & 1U) == 0) ||
      (*(int *)&_refptr_g_tsaPrpt[1].field_0x30 != 2)) ||
     ((_refptr_g_tsaPrmtDynamic[0x80] & 2) == 0)) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_GetPosCBody @ 6ba9f6e0
// ============================================================================

undefined8 TSA_GetPosCBody(void)

{
                    /* 0x5f6e0  1971  TSA_GetPosCBody */
  return 0;
}


// ============================================================================
// Function: TSA_GetNegCBody @ 6ba9f6eb
// ============================================================================

undefined8 TSA_GetNegCBody(void)

{
                    /* 0x5f6eb  1968  TSA_GetNegCBody */
  return 0;
}


// ============================================================================
// Function: TSA_SDOverCompensated @ 6ba9f6f6
// ============================================================================

undefined8 TSA_SDOverCompensated(void)

{
                    /* 0x5f6f6  2252  TSA_SDOverCompensated */
  return 0;
}


// ============================================================================
// Function: TSA_GetMutualSDCoef @ 6ba9f701
// ============================================================================

undefined8 TSA_GetMutualSDCoef(void)

{
                    /* 0x5f701  1966  TSA_GetMutualSDCoef */
  return 0;
}


// ============================================================================
// Function: TSA_GetLOSpec @ 6ba9f70c
// ============================================================================

undefined1 TSA_GetLOSpec(void)

{
                    /* 0x5f70c  1946  TSA_GetLOSpec */
  return _refptr_g_tsaPrmtFlash->field_0x836;
}


// ============================================================================
// Function: TSA_IsSensorValid @ 6ba9f733
// ============================================================================

void TSA_IsSensorValid(undefined4 param_1,undefined4 param_2)

{
                    /* 0x5f733  2094  TSA_IsSensorValid */
  TPSensor_IsSensorButNotNotch(param_1,param_2);
  return;
}


// ============================================================================
// Function: TSA_GetUnNormalizedBaseline @ 6ba9f754
// ============================================================================

void TSA_GetUnNormalizedBaseline(undefined8 param_1,undefined4 param_2)

{
                    /* 0x5f754  2019  TSA_GetUnNormalizedBaseline */
  Rawdata_GetUnNormalizedBaseline(param_1,param_2);
  return;
}


// ============================================================================
// Function: TSA_GetUnnormalizedRaw @ 6ba9f778
// ============================================================================

undefined8 TSA_GetUnnormalizedRaw(void)

{
  undefined8 uVar1;
  
                    /* 0x5f778  2020  TSA_GetUnnormalizedRaw */
  if (*(longlong *)_refptr_g_tsaUnnormalizedRawBuf == 0) {
    uVar1 = TSA_RptRawPtr();
  }
  else {
    uVar1 = *(undefined8 *)_refptr_g_tsaUnnormalizedRawBuf;
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_GetWaterSize @ 6ba9f7a6
// ============================================================================

undefined2 TSA_GetWaterSize(void)

{
                    /* 0x5f7a6  2024  TSA_GetWaterSize */
  return *(undefined2 *)(*(longlong *)_refptr_g_touchesPtr + 0x288);
}


// ============================================================================
// Function: TSA_IsCommandFlagEnabled @ 6ba9f7c0
// ============================================================================

uchar TSA_IsCommandFlagEnabled(void)

{
                    /* 0x5f7c0  2056  TSA_IsCommandFlagEnabled */
  return (*_refptr_g_tsaStaticPtr)->bField_0x11b;
}


// ============================================================================
// Function: TSA_SetCommandEnableFlag @ 6ba9f814
// ============================================================================

void TSA_SetCommandEnableFlag(uchar param_1)

{
                    /* 0x5f814  2274  TSA_SetCommandEnableFlag */
  (*_refptr_g_tsaStaticPtr)->bField_0x11b = param_1;
  return;
}


// ============================================================================
// Function: TSA_SetBaselineResetFlag @ 6ba9f833
// ============================================================================

void TSA_SetBaselineResetFlag(undefined1 param_1)

{
                    /* 0x5f833  2273  TSA_SetBaselineResetFlag */
  (*_refptr_g_tsaStaticPtr)[1].field_0x1 = param_1;
  return;
}


// ============================================================================
// Function: TSA_SetDisableBaselineUpdateFlag @ 6ba9f852
// ============================================================================

void TSA_SetDisableBaselineUpdateFlag(undefined1 param_1)

{
                    /* 0x5f852  2281  TSA_SetDisableBaselineUpdateFlag */
  (*_refptr_g_tsaStaticPtr)[1].field_0x2 = param_1;
  return;
}


// ============================================================================
// Function: TSA_SetDisableCMFFlag @ 6ba9f871
// ============================================================================

void TSA_SetDisableCMFFlag(uchar param_1)

{
                    /* 0x5f871  2282  TSA_SetDisableCMFFlag */
  (*_refptr_g_tsaStaticPtr)->bCmfProcessingBlocked = param_1;
  return;
}


// ============================================================================
// Function: TSA_SetAFESpecVersion @ 6ba9f890
// ============================================================================

void TSA_SetAFESpecVersion(undefined1 param_1,undefined1 param_2,undefined1 param_3)

{
                    /* 0x5f890  2257  TSA_SetAFESpecVersion */
  AFE_SetAFESpecVersion(param_1,param_2,param_3);
  return;
}


// ============================================================================
// Function: TSA_GetAFESpecVersion @ 6ba9f8c2
// ============================================================================

void TSA_GetAFESpecVersion(void)

{
                    /* 0x5f8c2  1865  TSA_GetAFESpecVersion */
  AFE_GetSpecVersion();
  return;
}


// ============================================================================
// Function: TSA_GetAFESpecMajorVersion @ 6ba9f8d5
// ============================================================================

void TSA_GetAFESpecMajorVersion(void)

{
                    /* 0x5f8d5  1862  TSA_GetAFESpecMajorVersion */
  AFE_GetSpecMajorVersion();
  return;
}


// ============================================================================
// Function: TSA_GetAFESpecMinorVersion @ 6ba9f8e8
// ============================================================================

void TSA_GetAFESpecMinorVersion(void)

{
                    /* 0x5f8e8  1863  TSA_GetAFESpecMinorVersion */
  AFE_GetSpecMinorVersion();
  return;
}


// ============================================================================
// Function: TSA_GetAFESpecPatchVersion @ 6ba9f8fb
// ============================================================================

void TSA_GetAFESpecPatchVersion(void)

{
                    /* 0x5f8fb  1864  TSA_GetAFESpecPatchVersion */
  AFE_GetSpecPatchVersion();
  return;
}


// ============================================================================
// Function: TSA_SetAFEFrame @ 6ba9f96a
// ============================================================================

bool TSA_SetAFEFrame(longlong param_1)

{
                    /* 0x5f96a  2255  TSA_SetAFEFrame */
  if (param_1 != 0) {
    AFE_SetFrame(param_1);
  }
  return param_1 == 0;
}


// ============================================================================
// Function: TSA_GetAFEScanState @ 6ba9f9ea
// ============================================================================

undefined8 TSA_GetAFEScanState(void)

{
  undefined8 uVar1;
  
                    /* 0x5f9ea  1861  TSA_GetAFEScanState */
  if ((*_refptr_g_tsaStaticPtr)->nField_0x268 == 1) {
    uVar1 = 1;
  }
  else if ((*_refptr_g_tsaStaticPtr)->nField_0x268 == 2) {
    uVar1 = 2;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_AutoCheckProcess @ 6ba9fa96
// ============================================================================

undefined8 TSA_AutoCheckProcess(longlong param_1,undefined4 param_2,undefined8 param_3)

{
  undefined8 uVar1;
  
                    /* 0x5fa96  1736  TSA_AutoCheckProcess */
  if (param_1 == 0) {
    uVar1 = 1;
  }
  else {
    uVar1 = AFE_AutoCheckProcess(param_1,param_2,param_3);
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_SetAFETestLevel @ 6ba9fad2
// ============================================================================

void TSA_SetAFETestLevel(undefined4 param_1)

{
                    /* 0x5fad2  2258  TSA_SetAFETestLevel */
  AFE_SetTestLevel(param_1);
  return;
}


// ============================================================================
// Function: TSA_GetAFETestResult @ 6ba9faec
// ============================================================================

void TSA_GetAFETestResult(void)

{
                    /* 0x5faec  1866  TSA_GetAFETestResult */
  AFE_GetTestResult();
  return;
}


// ============================================================================
// Function: TSA_GetAFEGesture @ 6ba9faff
// ============================================================================

void TSA_GetAFEGesture(void)

{
                    /* 0x5faff  1858  TSA_GetAFEGesture */
  AFE_GetGesture();
  return;
}


// ============================================================================
// Function: TSA_IsBothHandOperating @ 6ba9fb9f
// ============================================================================

undefined1 TSA_IsBothHandOperating(void)

{
                    /* 0x5fb9f  2054  TSA_IsBothHandOperating */
  return *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0x40);
}


// ============================================================================
// Function: TSA_GetGestureCode @ 6ba9fbb3
// ============================================================================

void TSA_GetGestureCode(void)

{
                    /* 0x5fbb3  1935  TSA_GetGestureCode */
  Gesture_GetCode();
  return;
}


// ============================================================================
// Function: TSA_GetSupportedGestureList @ 6ba9fbc6
// ============================================================================

void TSA_GetSupportedGestureList(void)

{
                    /* 0x5fbc6  1993  TSA_GetSupportedGestureList */
  Gesture_GetList();
  return;
}


// ============================================================================
// Function: TSA_IsAFTModeEnabled @ 6ba9fbd9
// ============================================================================

void TSA_IsAFTModeEnabled(void)

{
                    /* 0x5fbd9  2042  TSA_IsAFTModeEnabled */
  TSAStatic_IsAFTModeEnabled();
  return;
}


// ============================================================================
// Function: TSA_EnableProximityFeature @ 6ba9fc09
// ============================================================================

undefined8 TSA_EnableProximityFeature(void)

{
                    /* 0x5fc09  1846  TSA_EnableProximityFeature */
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlags = (*_refptr_g_tsaStaticPtr)->dwFeatureFlags | 0x4000;
  return 0;
}


// ============================================================================
// Function: TSA_RptGesture @ 6ba9fc86
// ============================================================================

undefined8 TSA_RptGesture(void)

{
                    /* 0x5fc86  2173  TSA_RptGesture */
  return 0;
}


// ============================================================================
// Function: TSA_RptMouseGesture @ 6ba9fc91
// ============================================================================

undefined8 TSA_RptMouseGesture(void)

{
                    /* 0x5fc91  2182  TSA_RptMouseGesture */
  return 0;
}


// ============================================================================
// Function: TSA_RptFilmGesture @ 6ba9fc9c
// ============================================================================

undefined8 TSA_RptFilmGesture(void)

{
                    /* 0x5fc9c  2172  TSA_RptFilmGesture */
  return 0;
}


// ============================================================================
// Function: TSA_RptDisplayXPos @ 6ba9fca7
// ============================================================================

ulonglong TSA_RptDisplayXPos(undefined1 param_1)

{
  byte bVar1;
  ushort uVar2;
  
                    /* 0x5fca7  2168  TSA_RptDisplayXPos */
  bVar1 = TSA_GetResScale();
  uVar2 = TSA_RptTouchXPos(param_1);
  return (ulonglong)uVar2 / (ulonglong)(longlong)(int)(uint)bVar1;
}


// ============================================================================
// Function: TSA_RptDisplayYPos @ 6ba9fcd7
// ============================================================================

ulonglong TSA_RptDisplayYPos(undefined1 param_1)

{
  byte bVar1;
  ushort uVar2;
  
                    /* 0x5fcd7  2169  TSA_RptDisplayYPos */
  bVar1 = TSA_GetResScale();
  uVar2 = TSA_RptTouchYPos(param_1);
  return (ulonglong)uVar2 / (ulonglong)(longlong)(int)(uint)bVar1;
}


// ============================================================================
// Function: TSA_RptScrollDX @ 6ba9fd07
// ============================================================================

undefined8 TSA_RptScrollDX(void)

{
                    /* 0x5fd07  2197  TSA_RptScrollDX */
  return 0;
}


// ============================================================================
// Function: TSA_RptScrollDY @ 6ba9fd12
// ============================================================================

undefined8 TSA_RptScrollDY(void)

{
                    /* 0x5fd12  2198  TSA_RptScrollDY */
  return 0;
}


// ============================================================================
// Function: TSA_RptScrollVX @ 6ba9fd1d
// ============================================================================

undefined8 TSA_RptScrollVX(void)

{
                    /* 0x5fd1d  2199  TSA_RptScrollVX */
  return 0;
}


// ============================================================================
// Function: TSA_RptScrollVY @ 6ba9fd28
// ============================================================================

undefined8 TSA_RptScrollVY(void)

{
                    /* 0x5fd28  2200  TSA_RptScrollVY */
  return 0;
}


// ============================================================================
// Function: TSA_RptDX @ 6ba9fd33
// ============================================================================

undefined8 TSA_RptDX(void)

{
                    /* 0x5fd33  2165  TSA_RptDX */
  return 0;
}


// ============================================================================
// Function: TSA_RptDY @ 6ba9fd43
// ============================================================================

undefined8 TSA_RptDY(void)

{
                    /* 0x5fd43  2166  TSA_RptDY */
  return 0;
}


// ============================================================================
// Function: TSA_RptVX @ 6ba9fd53
// ============================================================================

undefined8 TSA_RptVX(void)

{
                    /* 0x5fd53  2246  TSA_RptVX */
  return 0;
}


// ============================================================================
// Function: TSA_RptVY @ 6ba9fd63
// ============================================================================

undefined8 TSA_RptVY(void)

{
                    /* 0x5fd63  2247  TSA_RptVY */
  return 0;
}


// ============================================================================
// Function: TSA_RptRotateAngle @ 6ba9fd73
// ============================================================================

undefined8 TSA_RptRotateAngle(void)

{
                    /* 0x5fd73  2189  TSA_RptRotateAngle */
  return 0;
}


// ============================================================================
// Function: TSA_RptZoomScale @ 6ba9fd7e
// ============================================================================

undefined8 TSA_RptZoomScale(void)

{
                    /* 0x5fd7e  2251  TSA_RptZoomScale */
  return 10;
}


// ============================================================================
// Function: TSA_SetPressure @ 6ba9fd89
// ============================================================================

bool TSA_SetPressure(longlong param_1)

{
                    /* 0x5fd89  2297  TSA_SetPressure */
  if (param_1 != 0) {
    SideTouch_SetPressure(param_1);
  }
  return param_1 == 0;
}


// ============================================================================
// Function: TSA_GetRingDataPointer @ 6ba9fe2c
// ============================================================================

void TSA_GetRingDataPointer(void)

{
                    /* 0x5fe2c  1978  TSA_GetRingDataPointer */
  SHBVolumeData_GetPointer();
  return;
}


// ============================================================================
// Function: TSA_GetRingDataSize @ 6ba9fe3f
// ============================================================================

void TSA_GetRingDataSize(void)

{
                    /* 0x5fe3f  1979  TSA_GetRingDataSize */
  SHBVolumeData_GetSize();
  return;
}


// ============================================================================
// Function: TSA_GetScreenOnDataPointer @ 6ba9fe78
// ============================================================================

undefined8 TSA_GetScreenOnDataPointer(void)

{
                    /* 0x5fe78  1984  TSA_GetScreenOnDataPointer */
  return 0;
}


// ============================================================================
// Function: TSA_GetScreenOnDataSize @ 6ba9fe83
// ============================================================================

undefined8 TSA_GetScreenOnDataSize(void)

{
                    /* 0x5fe83  1985  TSA_GetScreenOnDataSize */
  return 0;
}


// ============================================================================
// Function: TSA_GetScreenOffDataPointer @ 6ba9fe8e
// ============================================================================

undefined8 TSA_GetScreenOffDataPointer(void)

{
                    /* 0x5fe8e  1982  TSA_GetScreenOffDataPointer */
  return 0;
}


// ============================================================================
// Function: TSA_GetScreenOffDataSize @ 6ba9fe99
// ============================================================================

undefined8 TSA_GetScreenOffDataSize(void)

{
                    /* 0x5fe99  1983  TSA_GetScreenOffDataSize */
  return 0;
}


// ============================================================================
// Function: TSA_SetScreenRotateState @ 6ba9fea4
// ============================================================================

void TSA_SetScreenRotateState(int param_1)

{
  int iVar1;
  char *pcVar2;
  int local_res8;
  
                    /* 0x5fea4  2303  TSA_SetScreenRotateState */
  pcVar2 = (char *)TSA_GetPrmtName();
  iVar1 = strncmp(pcVar2,"P167",4);
  if (iVar1 != 0) {
    pcVar2 = (char *)TSA_GetPrmtName();
    iVar1 = strncmp(pcVar2,"W157",4);
    local_res8 = param_1;
    if (iVar1 != 0) goto LAB_6ba9fefe;
  }
  if (param_1 == 0) {
    local_res8 = 3;
  }
  else {
    local_res8 = param_1 + -1;
  }
LAB_6ba9fefe:
  (*_refptr_g_tsaStaticPtr)->nField_0x264 = local_res8;
  return;
}


// ============================================================================
// Function: TSA_SetScreenOnOffSource @ 6ba9ff17
// ============================================================================

void TSA_SetScreenOnOffSource(int param_1)

{
                    /* 0x5ff17  2302  TSA_SetScreenOnOffSource */
  (*_refptr_g_tsaStaticPtr)->nField_0x26c = param_1;
  return;
}


// ============================================================================
// Function: TSA_SetScreenBrightness @ 6ba9ff33
// ============================================================================

void TSA_SetScreenBrightness(int param_1)

{
                    /* 0x5ff33  2301  TSA_SetScreenBrightness */
  (*_refptr_g_tsaStaticPtr)->nField_0x270 = param_1;
  return;
}


// ============================================================================
// Function: TSA_IsToExtendTHPLog @ 6ba9ff4f
// ============================================================================

uchar TSA_IsToExtendTHPLog(void)

{
                    /* 0x5ff4f  2100  TSA_IsToExtendTHPLog */
  return (*_refptr_g_tsaStaticPtr)->bField_0x295;
}


// ============================================================================
// Function: TSA_GetEabledTHPLog @ 6ba9ff66
// ============================================================================

int TSA_GetEabledTHPLog(void)

{
                    /* 0x5ff66  1926  TSA_GetEabledTHPLog */
  return (*_refptr_g_tsaStaticPtr)->nField_0x298;
}


// ============================================================================
// Function: TSA_GetTHPLogPreFrameNum @ 6ba9ff7c
// ============================================================================

undefined2 TSA_GetTHPLogPreFrameNum(int param_1)

{
  undefined2 local_a;
  
                    /* 0x5ff7c  1996  TSA_GetTHPLogPreFrameNum */
  if (param_1 == 1) {
    local_a = (undefined2)(*_refptr_g_tsaStaticPtr)->nField_0x29c;
  }
  else if (param_1 == 2) {
    local_a = SideTouch_GetRingLogPreFrameNum();
  }
  else {
    local_a = 0;
  }
  return local_a;
}


// ============================================================================
// Function: TSA_GetTHPLogPostFrameNum @ 6ba9ffc8
// ============================================================================

undefined2 TSA_GetTHPLogPostFrameNum(int param_1)

{
  undefined2 local_a;
  
                    /* 0x5ffc8  1995  TSA_GetTHPLogPostFrameNum */
  if (param_1 == 1) {
    local_a = (undefined2)(*_refptr_g_tsaStaticPtr)->nField_0x2a0;
  }
  else if (param_1 == 2) {
    local_a = SideTouch_GetRingLogPostFrameNum();
  }
  else {
    local_a = 0;
  }
  return local_a;
}


// ============================================================================
// Function: TSA_GetTHPLogName @ 6baa0014
// ============================================================================

char * TSA_GetTHPLogName(int param_1)

{
  char *pcVar1;
  
                    /* 0x60014  1994  TSA_GetTHPLogName */
  if (param_1 == 2) {
    pcVar1 = (char *)SideTouch_GetRingLogName();
  }
  else if (param_1 == 4) {
    (*_refptr_g_tsaStaticPtr)->nField_0x298 = (*_refptr_g_tsaStaticPtr)->nField_0x298 & 0xfffffffb;
    pcVar1 = &(*_refptr_g_tsaStaticPtr)->field_0x2a4;
  }
  else if (param_1 == 1) {
    pcVar1 = &(*_refptr_g_tsaStaticPtr)->field_0x2a4;
  }
  else {
    pcVar1 = "TSA_DEFAULT";
  }
  return pcVar1;
}


// ============================================================================
// Function: TSA_GetSensorRate @ 6baa00cc
// ============================================================================

undefined1 TSA_GetSensorRate(uint param_1)

{
  undefined1 uVar1;
  
                    /* 0x600cc  1987  TSA_GetSensorRate */
  if ((((param_1 & 1) == 0) || ((*_refptr_g_tsaStaticPtr)->field_0x3 == '\0')) ||
     (_refptr_g_tsaPrmtFlash[2].field_0x6c8 == '\0')) {
    uVar1 = _refptr_g_tsaPrmtFlash[2].field_0x6c8;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: TSA_SetAccData @ 6baa0118
// ============================================================================

void TSA_SetAccData(short param_1,short param_2,short param_3)

{
                    /* 0x60118  2266  TSA_SetAccData */
  Posture_AccProcess((int)param_1,(int)param_2,(int)param_3);
  return;
}


// ============================================================================
// Function: TSA_SetGyroData @ 6baa014e
// ============================================================================

void TSA_SetGyroData(void)

{
                    /* 0x6014e  2289  TSA_SetGyroData */
  return;
}


// ============================================================================
// Function: TSA_SetPostureData @ 6baa0165
// ============================================================================

void TSA_SetPostureData(undefined8 *param_1)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x38]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x40];
  
                    /* 0x60165  2296  TSA_SetPostureData */
  (*(undefined8 *)(_stk + 0x8)) = *param_1;
  (*(undefined8 *)(_stk + 0x10)) = param_1[1];
  (*(undefined8 *)(_stk + 0x18)) = (*(undefined8 *)(_stk + 0x8));
  (*(undefined8 *)(_stk + 0x20)) = (*(undefined8 *)(_stk + 0x10));
  SensorData_PostureProcess(&(*(undefined8 *)(_stk + 0x8)));
  return;
}


// ============================================================================
// Function: TSA_GetCoreRegion @ 6baa01b9
// ============================================================================

undefined4 TSA_GetCoreRegion(void)

{
                    /* 0x601b9  1912  TSA_GetCoreRegion */
  return *(undefined4 *)&_refptr_g_tsaPrmtFlash->field_0x28;
}


// ============================================================================
// Function: TSA_SetAODRingState @ 6baa0231
// ============================================================================

void TSA_SetAODRingState(undefined1 param_1)

{
                    /* 0x60231  2259  TSA_SetAODRingState */
  VirtualKey_SetAODRingState(param_1);
  return;
}


// ============================================================================
// Function: TSA_GetTouchXVelocity @ 6baa0250
// ============================================================================

void TSA_GetTouchXVelocity(undefined1 param_1)

{
                    /* 0x60250  2012  TSA_GetTouchXVelocity */
  TSAWrapper_GetTouchXVelocity(param_1);
  return;
}


// ============================================================================
// Function: TSA_GetResScale @ 6baa026e
// ============================================================================

undefined2 TSA_GetResScale(void)

{
                    /* 0x6026e  1977  TSA_GetResScale */
  return *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a0;
}


// ============================================================================
// Function: TSA_GetTouchYVelocity @ 6baa0285
// ============================================================================

void TSA_GetTouchYVelocity(undefined1 param_1)

{
                    /* 0x60285  2013  TSA_GetTouchYVelocity */
  TSAWrapper_GetTouchYVelocity(param_1);
  return;
}


// ============================================================================
// Function: TSA_EnablePreCMFDiffLog @ 6baa02a3
// ============================================================================

void TSA_EnablePreCMFDiffLog(undefined4 param_1)

{
                    /* 0x602a3  1845  TSA_EnablePreCMFDiffLog */
  CMF_EnablePreCMFDiffLog(param_1);
  return;
}


// ============================================================================
// Function: TSA_GetBigDataLength @ 6baa0375
// ============================================================================

void TSA_GetBigDataLength(void)

{
                    /* 0x60375  1908  TSA_GetBigDataLength */
  BigData_GetStatLength();
  return;
}


// ============================================================================
// Function: TSA_GetBigDataPtr @ 6baa0388
// ============================================================================

void TSA_GetBigDataPtr(void)

{
                    /* 0x60388  1909  TSA_GetBigDataPtr */
  BigData_GetStatPtr();
  return;
}


// ============================================================================
// Function: TSA_ClearBigData @ 6baa039b
// ============================================================================

void TSA_ClearBigData(void)

{
                    /* 0x6039b  1737  TSA_ClearBigData */
  BigData_ClearStat();
  return;
}


// ============================================================================
// Function: TSA_IsTraceTimeStackNull @ 6baa03af
// ============================================================================

undefined8 TSA_IsTraceTimeStackNull(void)

{
                    /* 0x603af  2102  TSA_IsTraceTimeStackNull */
  return 0;
}


// ============================================================================
// Function: TSA_SetExtendCtrlFlag @ 6baa03ba
// ============================================================================

void TSA_SetExtendCtrlFlag(undefined8 param_1)

{
                    /* 0x603ba  2283  TSA_SetExtendCtrlFlag */
  TSAStatic_UpdateExtendCtrlFlags(param_1);
  return;
}


// ============================================================================
// Function: TSA_ContextCurFrame @ 6baa03d6
// ============================================================================

void TSA_ContextCurFrame(undefined8 param_1)

{
                    /* 0x603d6  1740  TSA_ContextCurFrame */
  Context_ContextCurFrame(param_1);
  return;
}


// ============================================================================
// Function: TSA_ContextKeyFrame @ 6baa03f2
// ============================================================================

void TSA_ContextKeyFrame(undefined8 param_1)

{
                    /* 0x603f2  1742  TSA_ContextKeyFrame */
  Context_ContextKeyFrame(param_1);
  return;
}


// ============================================================================
// Function: TSA_UpdateContextKeyFrame @ 6baa040e
// ============================================================================

void TSA_UpdateContextKeyFrame(undefined8 param_1,undefined8 param_2)

{
                    /* 0x6040e  2336  TSA_UpdateContextKeyFrame */
  Context_UpdateContextKeyFrame(param_1,param_2);
  return;
}


// ============================================================================
// Function: TSA_GetContextMaxSize @ 6baa0434
// ============================================================================

void TSA_GetContextMaxSize(undefined8 param_1)

{
                    /* 0x60434  1911  TSA_GetContextMaxSize */
  Context_GetMaxSize(param_1);
  return;
}


// ============================================================================
// Function: TSA_ContextFrameToData @ 6baa044f
// ============================================================================

void TSA_ContextFrameToData(undefined8 param_1,undefined4 param_2,undefined8 param_3)

{
                    /* 0x6044f  1741  TSA_ContextFrameToData */
  Context_FrameToData(param_1,param_2,param_3);
  return;
}


// ============================================================================
// Function: TSA_SetAfeWorkMode @ 6baa047d
// ============================================================================

undefined8 TSA_SetAfeWorkMode(undefined1 param_1)

{
                    /* 0x6047d  2268  TSA_SetAfeWorkMode */
  DoubleReport_SetDaemonStatus(param_1);
  return 0;
}


// ============================================================================
// Function: TSA_TriggerDoubleReport @ 6baa04a0
// ============================================================================

void TSA_TriggerDoubleReport(void)

{
                    /* 0x604a0  2324  TSA_TriggerDoubleReport */
  DoubleReport_Trigger();
  return;
}


// ============================================================================
// Function: TSA_IsEnableDoubleReport @ 6baa04b3
// ============================================================================

void TSA_IsEnableDoubleReport(void)

{
                    /* 0x604b3  2060  TSA_IsEnableDoubleReport */
  DoubleReport_IsEnabled();
  return;
}


// ============================================================================
// Function: TSA_GetDoubleReportDelayUs @ 6baa04c6
// ============================================================================

void TSA_GetDoubleReportDelayUs(void)

{
                    /* 0x604c6  1924  TSA_GetDoubleReportDelayUs */
  DoubleReport_DelayUs();
  return;
}


// ============================================================================
// Function: TSA_GenerateAndGetHotZoneAdjustDescription @ 6baa04d9
// ============================================================================

void TSA_GenerateAndGetHotZoneAdjustDescription(void)

{
                    /* 0x604d9  1857  TSA_GenerateAndGetHotZoneAdjustDescription */
  HotZoneAdjust_GenerateAndGetDmdDescription();
  return;
}


// ============================================================================
// Function: TSA_IsPossibleOutOfWater @ 6baa086b
// ============================================================================

bool TSA_IsPossibleOutOfWater(void)

{
  ushort uVar1;
  
                    /* 0x6086b  2085  TSA_IsPossibleOutOfWater */
  uVar1 = _refptr_g_tsaPrmtFlash->wBufElementCount >> 3;
  return (int)((uint)uVar1 * 8 - (uint)uVar1) < (int)(uint)(ushort)*(short*)((char*)&g_tsaPrpt + 190);
}


