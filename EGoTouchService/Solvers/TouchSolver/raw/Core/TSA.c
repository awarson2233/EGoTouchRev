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
      g_stylusAftLastStylusTimeStamp = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
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
// Function: TSA_IsGripOKToIDLE @ 6ba9e650
// ============================================================================

char TSA_IsGripOKToIDLE(void)
{
  return GripFilter_IsOKToEnterIDLE();
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
// Function: TSA_RptLONum @ 6ba9b0eb
// ============================================================================

undefined1 TSA_RptLONum(void)

{
                    /* 0x5b0eb  2181  TSA_RptLONum */
  return *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 3);
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
              ("distToEnterGloveMode",*(undefined2 *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x40));
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
    if (pvVar2 != (void *)0x0) {
      TSAState_OnAlloc(pvVar2,param_1);
    }
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
    TSAState_OnFree((void *)*param_1);
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
// Function: TSA_IsFlickCheckPassed @ 6ba9c0e8
// ============================================================================

void TSA_IsFlickCheckPassed(void)

{
                    /* 0x5c0e8  2062  TSA_IsFlickCheckPassed */
  TouchAction_IsFlickCheckPassed();
  return;
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
// Function: TSA_RptDiffPtr @ 6ba9c5a8
// ============================================================================

undefined8 TSA_RptDiffPtr(void)

{
                    /* 0x5c5a8  2167  TSA_RptDiffPtr */
  return TSAMS_GetDifPtr();
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
// Function: TSA_EnableEdgePeakFilterWorkaround @ 6ba9f33f
// ============================================================================

undefined8 TSA_EnableEdgePeakFilterWorkaround(void)

{
                    /* 0x5f33f  1841  TSA_EnableEdgePeakFilterWorkaround */
  (*_refptr_g_tsaStaticPtr)->dwFeatureFlags2 = (*_refptr_g_tsaStaticPtr)->dwFeatureFlags2 | 1;
  return 0;
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


