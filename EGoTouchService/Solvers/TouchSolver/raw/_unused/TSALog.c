// ============================================================================
// Function: TSALog_Frame @ 6bb388e7
// ============================================================================

void TSALog_Frame(void)

{
  char cVar1;
  
                    /* 0xf88e7  1565  TSALog_Frame */
  cVar1 = TSALog_RtEnable(2,0x3e,4);
  if (cVar1 != '\0') {
    StrPrintf_Process(0,&DAT_6bb5cfe7,"frame=");
    StrPrintf_Process(0,"[%08X]",(*_refptr_g_tsaStaticPtr)->nFrameCounter);
  }
  return;
}


// ============================================================================
// Function: TSALog_Input @ 6bb38956
// ============================================================================

void TSALog_Input(undefined8 param_1,undefined4 param_2,undefined8 param_3,undefined8 param_4)

{
  char cVar1;
  
                    /* 0xf8956  1568  TSALog_Input */
  cVar1 = TSALog_RtEnable(2,0x3e,0x10);
  if (cVar1 != '\0') {
    TSALog_Ctrl(param_2,param_3);
    TSALog_TimeStamp(param_4);
    TSALog_Feature((*_refptr_g_tsaStaticPtr)->dwFeatureFlags);
  }
  return;
}


// ============================================================================
// Function: TSALog_Output @ 6bb389be
// ============================================================================

void TSALog_Output(void)

{
  char cVar1;
  
                    /* 0xf89be  1569  TSALog_Output */
  cVar1 = TSALog_RtEnable(2,0x3e,0x10);
  if (cVar1 != '\0') {
    TSALog_Report();
    TSALog_PrintApplog();
  }
  return;
}


// ============================================================================
// Function: TSALog_RtEnable @ 6bb388a2
// ============================================================================

undefined8 TSALog_RtEnable(int param_1,uint param_2,uint param_3)

{
  undefined8 uVar1;
  
                    /* 0xf88a2  1576  TSALog_RtEnable */
  if (g_tsaLogLevel == '\0') {
    uVar1 = 0;
  }
  else if (param_1 == 0) {
    uVar1 = 0;
  }
  else if ((param_2 & param_3) == 0) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: TSALog_Ctrl @ 6bb38a36
// ============================================================================

void TSALog_Ctrl(undefined4 param_1,undefined8 param_2)

{
                    /* 0xf8a36  1563  TSALog_Ctrl */
  StrPrintf_Process(0,&DAT_6bb5cffd,"ctrlflag=");
  StrPrintf_Process(0,"[%08X]",param_1);
  StrPrintf_Process(0,&DAT_6bb5cffd,"extendCtrl=");
  StrPrintf_Process(0,"[%08X]",param_2);
  return;
}


// ============================================================================
// Function: TSALog_Feature @ 6bb38aa9
// ============================================================================

void TSALog_Feature(undefined4 param_1)

{
                    /* 0xf8aa9  1564  TSALog_Feature */
  StrPrintf_Process(0,&DAT_6bb5cffd,"feature=");
  StrPrintf_Process(0,"[%04X]",param_1);
  return;
}


// ============================================================================
// Function: TSALog_TimeStamp @ 6bb38af3
// ============================================================================

void TSALog_TimeStamp(undefined8 param_1)

{
                    /* 0xf8af3  1577  TSALog_TimeStamp */
  StrPrintf_Process(0,&DAT_6bb5cffd,"time=");
  StrPrintf_Process(0,"[%lld]",param_1);
  return;
}


// ============================================================================
// Function: TSALog_Report @ 6bb390c8
// ============================================================================

void TSALog_Report(void)

{
  char cVar1;
  undefined1 uVar2;
  undefined2 uVar3;
  undefined4 uVar4;
  int iVar5;
  undefined8 uVar6;
  
                    /* 0xf90c8  1574  TSALog_Report */
  cVar1 = SensorData_IsAntiTouchStage();
  if (cVar1 != '\0') {
    StrPrintf_Process(0,"In posture anti touch stage\n");
  }
  uVar2 = TSA_IsHangingPosture();
  StrPrintf_Process(0,"\nIsHangingPosture = %d",uVar2);
  uVar4 = TSA_GetPosture();
  StrPrintf_Process(0,"\nPostureLabelEnum = %d",uVar4);
  uVar2 = TSA_IsOKToEnterIDLEMode();
  StrPrintf_Process(0,"\nOKToIDLE = %d",uVar2);
  uVar2 = TSA_IsOKToEnterSideIDLEMode();
  StrPrintf_Process(0," OKToSideIDLE = %d",uVar2);
  StrPrintf_Process(0," activeReason = 0x%x",*(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x9c);
  StrPrintf_Process(0," BLResetFailReason = 0x%x",
                    *(undefined4 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6);
  StrPrintf_Process(0," blStage = %d",_refptr_g_tsaPrpt->dwBaselineStage);
  StrPrintf_Process(0,"\nscenario = %d, ",(*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags);
  uVar4 = TSA_GetState();
  StrPrintf_Process(0,"state = %d, ",uVar4);
  uVar4 = TSA_GetEvent();
  StrPrintf_Process(0,"event = %d, ",uVar4);
  StrPrintf_Process(0,"extFeature = %d",(*_refptr_g_tsaStaticPtr)->dwFeatureFlagsMasked);
  StrPrintf_Process(0,"sideThenar = %d",*(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x48));
  uVar2 = TSA_GetTouchMode();
  StrPrintf_Process(0,"\nTouchMode = %d, ",uVar2);
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SAFE_BASELINE) != 0) {
    uVar2 = SafeBaseline_IsAvailable();
    StrPrintf_Process(0," SafeBLNum = %d, ",uVar2);
    uVar2 = SafeBaseline_IsPushed();
    StrPrintf_Process(0," SafeBLPush = %d, ",uVar2);
    uVar2 = SafeBaseline_IsReset();
    StrPrintf_Process(0," SafeBLReset = %d, ",uVar2);
    StrPrintf_Process(0," SafeBLState = %d, ",*(undefined4 *)(_refptr_g_safeBaselineReset + 0x2c));
    StrPrintf_Process(0," SafeBLStatus = %d, ",*(undefined4 *)(_refptr_g_safeBaseline + 0x10));
    StrPrintf_Process(0," SafeBLPushed = %d, ",_refptr_g_safeBaseline[0x21]);
    StrPrintf_Process(0," SafeBLAbnormal = %d, ",*(undefined4 *)(_refptr_g_safeBaselineReset + 0x20)
                     );
    StrPrintf_Process(0," SafeBLtriggerAbnormal = %d, ",
                      *(undefined4 *)(_refptr_g_safeBaselineReset + 0x24));
    StrPrintf_Process(0," SafeBLAllowed = %d, ",*(undefined4 *)(_refptr_g_safeBaselineReset + 0x28))
    ;
    StrPrintf_Process(0," numAbnormalShape = %d, ",(*_refptr_g_prevTouchesPtr)->field_0x10);
    StrPrintf_Process(0," wakeupTimeStamp = %lld, ",
                      *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x80);
  }
  uVar3 = TSA_GetTouchThold();
  StrPrintf_Process(0,"\nTouchThold = %d",uVar3);
  uVar3 = TSA_GetBaselineThold();
  StrPrintf_Process(0,"BLThold = %d",uVar3);
  uVar3 = TSA_GetIDLETouchThold();
  StrPrintf_Process(0,"IDLEThold = %d",uVar3);
  if (((*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags & 1) != 0) {
    StrPrintf_Process(0," BLReset = %d",_refptr_g_tsaPrpt->dwBaselinePropertyState);
  }
  StrPrintf_Process(0,"\nHTI = %d",(int)(short)(*_refptr_g_tsaStaticPtr)->wField_0x292);
  uVar4 = TSA_GetException();
  StrPrintf_Process(0," Exception = %d",uVar4);
  StrPrintf_Process(0," enabledTHPLog = %d",(*_refptr_g_tsaStaticPtr)->nField_0x298);
  uVar4 = TSA_GetMutualDifMax();
  StrPrintf_Process(0,"\nMaxDif = %d, ",uVar4);
  uVar4 = TSA_GetMutualDifMin();
  StrPrintf_Process(0,"MinDif = %d, ",uVar4);
  uVar4 = TSA_GetMutualRawMax();
  StrPrintf_Process(0,"MaxRaw = %d, ",uVar4);
  uVar4 = TSA_GetMutualRawMin();
  StrPrintf_Process(0,"MinRaw = %d",uVar4);
  StrPrintf_Process(0,"\nCMFSigThold = %d, CMFSigDiffThold = %d",
                    (int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x3c),
                    (int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x3e));
  StrPrintf_Process(0,"\nmaxRaw2RawDiff = %d, ",(int)*(short *)_refptr_g_tsaPrpt);
  StrPrintf_Process(0,"minRaw2RawDiff = %d, ",(int)*(short *)&_refptr_g_tsaPrpt->field_0x2);
  StrPrintf_Process(0,"numOverNoise = %d, ",*(undefined2 *)&_refptr_g_tsaPrpt->field_0x4);
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) != 0) {
    uVar2 = SideGesture_GetNum();
    StrPrintf_Process(0,"\nnumGesture = %d, ",uVar2);
    uVar4 = SideGesture_GetEnabledTypes();
    StrPrintf_Process(0,"enabledGesture = 0x%x, ",uVar4);
    if (*_refptr_g_touchesSide != '\0') {
      StrPrintf_Process(0,"sideGripRatio = %d",_refptr_g_touchesSide[0x198]);
    }
  }
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SIGNAL_DISPARITY_POST) != 0) {
    StrPrintf_Process(0,"\nsdScale = %d",*(undefined4 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x12);
  }
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x20) != 0) {
    GripFilter_Log();
  }
  TSALog_Afe();
  uVar2 = TSAStatic_IsAllFreqNoisy();
  StrPrintf_Process(0," AllFreqNoisy: %d",uVar2);
  iVar5 = TSA_GetDMDType();
  if (iVar5 != 0) {
    uVar4 = TSA_GetDMDType();
    StrPrintf_Process(0,"\nDMD[0x%x]",uVar4);
    uVar6 = TSA_GetExceptionDescription();
    StrPrintf_Process(0,&DAT_6bb5d462,uVar6);
  }
  uVar2 = TSA_RptLONum();
  StrPrintf_Process(0,"\nLargeObjectNum = %d",uVar2);
  TSALog_Touch();
  TSAIDE_LogPreFiltTouch();
  TSALog_VirtualKey();
  StrPrintf_Process(0,&DAT_6bb5cfde);
  return;
}


// ============================================================================
// Function: TSALog_PrintApplog @ 6bb3a043
// ============================================================================

void TSALog_PrintApplog(void)

{
                    /* 0xfa043  1572  TSALog_PrintApplog */
  if (((*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags & 1) != 0) {
    StrPrintf_Process(0,"\nAppLog: ");
    StrPrintf_Process(2,"BL reset, type: %d, raw[%d, %d]",_refptr_g_tsaPrpt->dwBaselinePropertyState
                      ,_refptr_g_tsaPrpt->wBoundaryNegative,_refptr_g_tsaPrpt->wBoundaryPositive);
  }
  TSALog_PrintEventApplog();
  return;
}


// ============================================================================
// Function: TSALog_VirtualKey @ 6bb38b3e
// ============================================================================

void TSALog_VirtualKey(void)

{
  char cVar1;
  undefined1 uVar2;
  byte bVar3;
  undefined1 uVar4;
  undefined2 uVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  undefined4 uVar8;
  uint uVar9;
  byte local_29;
  
                    /* 0xf8b3e  1580  TSALog_VirtualKey */
  cVar1 = TSA_RptVitualKeyEventNum();
  if (cVar1 != '\0') {
    uVar2 = TSA_RptVitualKeyEventNum();
    StrPrintf_Process(0,"\nVKNum = %d",uVar2);
    local_29 = 0;
    while( true ) {
      bVar3 = TSA_RptVitualKeyEventNum();
      if (bVar3 <= local_29) break;
      uVar5 = TSA_RptVirtualKeyCode(local_29);
      StrPrintf_Process(0,"  (%d",uVar5);
      uVar2 = TSA_RptVirtualKeyAction(local_29);
      StrPrintf_Process(0,&DAT_6bb5d03c,uVar2);
      local_29 = local_29 + 1;
    }
    uVar2 = TSA_IsMotorControlEnabled();
    uVar4 = TSA_IsVolumeControlEnabled();
    StrPrintf_Process(0," Vol(%d) Motor(%d)",uVar4,uVar2);
  }
  uVar2 = TSA_IsVolumePressDown();
  StrPrintf_Process(0,"\nPressure(%d)",uVar2);
  uVar6 = SHBVolumeData_GetYPos();
  uVar7 = SHBVolumeData_GetTPEvent();
  uVar8 = SHBVolumeData_GetVolumeKey();
  uVar9 = TSA_GetExposedDataState();
  StrPrintf_Process(0,"\nSHB(%d, %d, %d, %d)",(uVar9 & 2) != 0,uVar8,uVar7,uVar6);
  return;
}


// ============================================================================
// Function: TSALog_Afe @ 6bb38c66
// ============================================================================

void TSALog_Afe(void)

{
  char cVar1;
  short sVar2;
  byte local_a;
  byte local_9;
  
                    /* 0xf8c66  1562  TSALog_Afe */
  if (*(longlong *)(_refptr_g_thpAFEStruct + 8) != 0) {
    TSA_GetAFEScanFreqIndex(&local_a);
    local_9 = local_a;
    StrPrintf_Process(0,"\nScanFreq = %d",
                      *(undefined2 *)
                       (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x10) +
                       (ulonglong)local_a * 2));
    cVar1 = AFE_IsManualFreqHopping();
    if (cVar1 != '\0') {
      StrPrintf_Process(0," Noise: ");
      for (local_9 = 0; local_9 < *(byte *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 10);
          local_9 = local_9 + 1) {
        sVar2 = AFE_GetMaxNoise(local_9);
        StrPrintf_Process(0,"(%d, %d)",
                          *(undefined2 *)
                           (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x10) +
                           (ulonglong)local_9 * 2),(int)sVar2);
      }
    }
    TSA_GetAFEScanRateIndex(&local_a);
    StrPrintf_Process(0,"\nScanRate = %d",
                      *(undefined2 *)
                       (*(longlong *)(*(longlong *)(_refptr_g_thpAFEStruct + 8) + 0x20) +
                       (ulonglong)local_a * 2));
    cVar1 = DoubleReport_IsEnabled();
    if (cVar1 != '\0') {
      StrPrintf_Process(0,&DAT_6bb5d0a7);
    }
  }
  return;
}


// ============================================================================
// Function: TSALog_Touch @ 6bb38dad
// ============================================================================

void TSALog_Touch(void)

{
  undefined1 uVar1;
  char cVar2;
  byte bVar3;
  undefined2 uVar4;
  undefined4 uVar5;
  byte local_9;
  
                    /* 0xf8dad  1578  TSALog_Touch */
  uVar1 = TSA_RptTouchNum();
  StrPrintf_Process(0," TouchNum = %d",uVar1);
  StrPrintf_Process(0," PrevTouchNum = %d",(*_refptr_g_prevTouchesPtr)->bPrevTouchCount);
  StrPrintf_Process(0," PeakNum = %d",**(undefined1 **)_refptr_g_prevPeaksPtr);
  StrPrintf_Process(0," GripNum = %d",(*_refptr_g_prevTouchesPtr)->field_0xd);
  local_9 = 0;
  while( true ) {
    bVar3 = TSA_RptTouchNum();
    if (bVar3 <= local_9) break;
    uVar1 = TSA_RptTouchID(local_9);
    StrPrintf_Process(0,&DAT_6bb5d0e8,uVar1);
    uVar4 = TSA_RptTouchXPos(local_9);
    StrPrintf_Process(0,&DAT_6bb5d0ed,uVar4);
    uVar4 = TSA_RptTouchYPos(local_9);
    StrPrintf_Process(0,&DAT_6bb5d0ed,uVar4);
    uVar4 = TSA_RptTouchPressure(local_9);
    StrPrintf_Process(0,&DAT_6bb5d0ed,uVar4);
    uVar4 = TSA_RptTouchAxisMajor(local_9);
    StrPrintf_Process(0,&DAT_6bb5d0ed,uVar4);
    uVar4 = TSA_RptTouchAxisMinor(local_9);
    StrPrintf_Process(0,&DAT_6bb5d0ed,uVar4);
    cVar2 = TSA_RptTouchAxisAngle(local_9);
    StrPrintf_Process(0,&DAT_6bb5d03c,(int)cVar2);
    uVar4 = TSA_RptTouchAge(local_9);
    StrPrintf_Process(0,&DAT_6bb5d0ed,uVar4);
    uVar1 = TSA_RptTouchHand(local_9);
    StrPrintf_Process(0,&DAT_6bb5d0ed,uVar1);
    StrPrintf_Process(0,&DAT_6bb5d0ed,
                      (int)*(short *)((longlong)(int)(uint)local_9 * 0x168 +
                                      *(longlong *)_refptr_g_touchesPtr + 0x1a0));
    StrPrintf_Process(0,&DAT_6bb5d0ed,
                      *(undefined4 *)
                       ((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x210));
    uVar5 = TSA_RptTouchAxisMajorExtend(local_9);
    StrPrintf_Process(0,&DAT_6bb5d0ed,uVar5);
    StrPrintf_Process(0,&DAT_6bb5d0ed,
                      *(undefined4 *)
                       ((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x1f8));
    StrPrintf_Process(0,&DAT_6bb5d0ed,
                      *(undefined4 *)
                       ((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x1fc));
    StrPrintf_Process(0,&DAT_6bb5d0ed,
                      *(undefined4 *)
                       ((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x200));
    local_9 = local_9 + 1;
  }
  return;
}


// ============================================================================
// Function: TSALog_PrintEventApplog @ 6bb39c02
// ============================================================================

void TSALog_PrintEventApplog(void)

{
                    /* 0xf9c02  1573  TSALog_PrintEventApplog */
  if ((*_refptr_g_tsaStaticPtr)->nField_0x280 != 0) {
    if (((*_refptr_g_tsaStaticPtr)->nField_0x280 & 2U) != 0) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(1,"TSA-Event[phone on left hand]");
    }
    if (((*_refptr_g_tsaStaticPtr)->nField_0x280 & 4U) != 0) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(1,"TSA-Event[phone on right hand]");
    }
    if (((*_refptr_g_tsaStaticPtr)->nField_0x280 & 8U) != 0) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(1,"TSA-Event[thenar long pressed on left hand]");
    }
    if (((*_refptr_g_tsaStaticPtr)->nField_0x280 & 0x10U) != 0) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(1,"TSA-Event[thenar long pressed on right hand]");
    }
    if (((*_refptr_g_tsaStaticPtr)->nField_0x280 & 0x20U) != 0) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(1,"TSA-Event[phone under water]");
    }
    if (((*_refptr_g_tsaStaticPtr)->nField_0x280 & 0x40U) != 0) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(1,"TSA-Event[phone outof water]");
    }
    if (((*_refptr_g_tsaStaticPtr)->nField_0x280 & 0x400U) != 0) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(1,"TSA-Event[volume trigger on left hand]");
    }
    if (((*_refptr_g_tsaStaticPtr)->nField_0x280 & 0x800U) != 0) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(1,"TSA-Event[volume trigger on right hand]");
    }
    if (((*_refptr_g_tsaStaticPtr)->nField_0x280 & 0x1000U) != 0) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(1,"TSA-Event[volume+]");
    }
    if (((*_refptr_g_tsaStaticPtr)->nField_0x280 & 0x2000U) != 0) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(1,"TSA-Event[volume-]");
    }
    if (((*_refptr_g_tsaStaticPtr)->nField_0x280 & 0x4000U) != 0) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(1,"TSA-Event[volume0]");
    }
    if (((*_refptr_g_tsaStaticPtr)->nField_0x280 & 0x8000U) != 0) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(1,"TSA-Event[powerKey]");
    }
    if (((*_refptr_g_tsaStaticPtr)->nField_0x280 & 0x10000U) != 0) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(1,"TSA-Event[left volume indicator on]");
    }
    if (((*_refptr_g_tsaStaticPtr)->nField_0x280 & 0x20000U) != 0) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(1,"TSA-Event[right volume indicator on]");
    }
    if (((*_refptr_g_tsaStaticPtr)->nField_0x280 & 0x40000U) != 0) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(1,"TSA-Event[left volume indicator off]");
    }
    if (((*_refptr_g_tsaStaticPtr)->nField_0x280 & 0x80000U) != 0) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(1,"TSA-Event[right volume indicator off]");
    }
    if (((*_refptr_g_tsaStaticPtr)->nField_0x280 & 0x100000U) != 0) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(1,"TSA-Event[volume trigger off]");
    }
    if (((*_refptr_g_tsaStaticPtr)->nField_0x280 & 0x200000U) != 0) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(1,"TSA-Event[TP restart]");
    }
  }
  return;
}


