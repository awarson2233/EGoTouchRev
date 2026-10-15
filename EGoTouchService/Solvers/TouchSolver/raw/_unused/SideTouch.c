// ============================================================================
// Function: SideTouch_Process @ 6bb0abf5
// ============================================================================

void SideTouch_Process(void)

{
  undefined4 uVar1;
  undefined8 uVar2;
  undefined8 uVar3;
  undefined8 uVar4;
  undefined8 uVar5;
  undefined *puVar6;
  char cVar7;
  
                    /* 0xcabf5  1398  SideTouch_Process */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) != 0) {
    cVar7 = SideTouch_IsFrameDataAvaliable();
    if (cVar7 != '\0') {
      (*_refptr_g_tsaStaticPtr)->dwProcessingState =
           (*_refptr_g_tsaStaticPtr)->dwProcessingState | 0x10;
      uVar2 = *(undefined8 *)_refptr_g_tsaPrmtDynamic;
      uVar3 = *(undefined8 *)(_refptr_g_tsaPrmtDynamic + 8);
      uVar4 = *(undefined8 *)(_refptr_g_tsaPrmtDynamic + 0x10);
      uVar5 = *(undefined8 *)(_refptr_g_tsaPrmtDynamic + 0x18);
      uVar1 = *(undefined4 *)(_refptr_g_tsaPrmtDynamic + 0x20);
      DataSwitch_ToSide();
      cVar7 = SideTouch_IsAllFeatureDisabled();
      if (cVar7 == '\0') {
        cVar7 = TSA_IsToBypassCurrentFrame();
        if (cVar7 == '\0') {
          cVar7 = SideTouch_IsToReset();
          if (cVar7 != '\0') {
            SideTouch_Init();
          }
          SideTouch_HeatmapProcess();
          SideTouch_TouchProcess();
          SideTouch_ProcessExtInfo();
          SideTouch_ReportProcess();
          SideTouch_PressureProcess();
          SideTouch_GestureProcess();
          VirtualKey_Process();
          PrevTouch_PostProcess();
          PrevPeak_Process();
          SideTouch_BaselineSideToFull();
          g_sideData = 0;
          SideTouch_IDEProcess();
        }
      }
      else {
        g_sideData = 1;
        DAT_6bc981f8 = 1;
        DAT_6bc99280 = 1;
        SideTouch_IDLEProcess();
        Touch_Clean();
        PrevTouch_Release();
      }
      DataSwitch_ToGrid();
      puVar6 = _refptr_g_tsaPrmtDynamic;
      *(undefined8 *)_refptr_g_tsaPrmtDynamic = uVar2;
      *(undefined8 *)(puVar6 + 8) = uVar3;
      *(undefined8 *)(puVar6 + 0x10) = uVar4;
      *(undefined8 *)(puVar6 + 0x18) = uVar5;
      *(undefined4 *)(puVar6 + 0x20) = uVar1;
      (*_refptr_g_tsaStaticPtr)->dwProcessingState =
           (*_refptr_g_tsaStaticPtr)->dwProcessingState & 0xffffffef;
    }
  }
  return;
}


// ============================================================================
// Function: SideTouch_Report @ 6bb1a7a5
// ============================================================================

void SideTouch_Report(void)

{
  char cVar1;
  byte local_b;
  byte local_a;
  byte local_9;
  
                    /* 0xda7a5  1400  SideTouch_Report */
  local_9 = 0;
  while( true ) {
    if (g_sideGesture <= local_9) {
      if ((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x624 & 3) != 0) {
        for (local_a = 0; local_a < (byte)*_refptr_g_touchesSide; local_a = local_a + 1) {
          if (((*(uint *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_a * 0x168 + 500) & 8)
               == 0) &&
             ((*(uint *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_a * 0x168 + 0x1f8) & 8)
              != 0)) {
            *(uint *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_a * 0x168 + 500) =
                 *(uint *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_a * 0x168 + 500) | 8;
            *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_a * 0x168 + 0x29a) =
                 *(undefined2 *)
                  (_refptr_g_touchesSide + (longlong)(int)(uint)local_a * 0x168 + 0x170);
            *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_a * 0x168 + 0x29c) =
                 *(undefined2 *)
                  (_refptr_g_touchesSide + (longlong)(int)(uint)local_a * 0x168 + 0x172);
          }
        }
      }
      for (local_b = 0; local_b < (byte)*_refptr_g_touchesSide; local_b = local_b + 1) {
        if ((*(uint *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_b * 0x168 + 500) & 8) != 0
           ) {
          if (*(byte *)(*(longlong *)_refptr_g_touchesPtr + 5) < 0x14) {
            SideTouch_UnitCopyToOutReport
                      (*(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 5),local_b);
            *(char *)(*(longlong *)_refptr_g_touchesPtr + 5) =
                 *(char *)(*(longlong *)_refptr_g_touchesPtr + 5) + '\x01';
            **(char **)_refptr_g_touchesPtr = **(char **)_refptr_g_touchesPtr + '\x01';
          }
          else if (0x14 < *(byte *)(*(longlong *)_refptr_g_touchesPtr + 5)) {
            StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                              "../../src/Extension/SideTouch/SideTouch_Gesture.c",0xd7e,
                              "TouchesPrpt(rptNum) <= TSA_MAX_TOUCH_NUM");
            HardwareAnalyzer_SetSoftException();
          }
        }
      }
      if ((*(uint *)(_refptr_g_touchesSide + 0x18) & 0x1e) != 0) {
        *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x27c =
             *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x27c & 0xffffffe1;
      }
      *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x27c =
           *(uint *)(_refptr_g_touchesSide + 0x18) |
           *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x27c;
      (*_refptr_g_tsaStaticPtr)->nField_0x280 =
           *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x27c &
           ~(*_refptr_g_tsaStaticPtr)->nField_0x278;
      return;
    }
    if (*(longlong *)(&DAT_6bc99820 + (longlong)(int)(uint)local_9 * 0x2a8) == 0) break;
    if (*(longlong *)(&DAT_6bc99828 + (longlong)(int)(uint)local_9 * 0x2a8) == 0) {
      return;
    }
    cVar1 = (**(code **)(&DAT_6bc99820 + (longlong)(int)(uint)local_9 * 0x2a8))(local_9);
    if (cVar1 != '\0') {
      (**(code **)(&DAT_6bc99828 + (longlong)(int)(uint)local_9 * 0x2a8))(local_9);
    }
    local_9 = local_9 + 1;
  }
  return;
}


// ============================================================================
// Function: SideTouch_KeepPrevTouchWithExceptionExcluded @ 6bb1e900
// ============================================================================

void SideTouch_KeepPrevTouchWithExceptionExcluded(void)

{
                    /* 0xde900  1390  SideTouch_KeepPrevTouchWithExceptionExcluded */
  *(undefined **)_refptr_g_touchesPtr = _refptr_g_touchesSide;
  *_refptr_g_prevTouchesPtr = (PrevTouchArray *)_refptr_g_prevTouchesSide;
  Touch_KeepPrevTouchWithExceptionExcluded();
  *(undefined **)_refptr_g_touchesPtr = _refptr_g_touchesGrid;
  *_refptr_g_prevTouchesPtr = (PrevTouchArray *)_refptr_g_prevTouchesGrid;
  return;
}


// ============================================================================
// Function: SideTouch_GetVirtualVolumeKey @ 6bb172ff
// ============================================================================

undefined4 SideTouch_GetVirtualVolumeKey(void)

{
                    /* 0xd72ff  1361  SideTouch_GetVirtualVolumeKey */
  return DAT_6bc9d7fc;
}


// ============================================================================
// Function: SideTouch_GetVirtualVolumeYPos @ 6bb17312
// ============================================================================

undefined2 SideTouch_GetVirtualVolumeYPos(void)

{
                    /* 0xd7312  1362  SideTouch_GetVirtualVolumeYPos */
  return DAT_6bc9d800;
}


// ============================================================================
// Function: SideTouch_Init @ 6bb063eb
// ============================================================================

void SideTouch_Init(void)

{
  undefined1 uVar1;
  int iVar2;
  
                    /* 0xc63eb  1366  SideTouch_Init */
  uVar1 = g_sideData;
  iVar2 = memset_s(&g_sideData,0x2120,0,0x2120);
  if (iVar2 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SideTouch/SideTouch.c",0x60,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar2 = memset_s(&g_prevTouchesSide,0x5aa0,0,0x5aa0);
  if (iVar2 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SideTouch/SideTouch.c",0x62,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  SideTouch_PressureInit();
  SideTouch_GestureInit();
  SideTouch_SHBBaselineInit();
  SideTouch_BaselineInit();
  VirtualKey_Init();
  if (flagOnce_11623 == '\0') {
    flagOnce_11623 = '\x01';
  }
  else {
    g_sideData = uVar1;
  }
  return;
}


// ============================================================================
// Function: SideTouch_IsToReset @ 6bb064f0
// ============================================================================

undefined8 SideTouch_IsToReset(void)

{
  char cVar1;
  undefined8 uVar2;
  
  cVar1 = TSAStatic_IsHWReset();
  if (cVar1 == '\0') {
    if (g_sideData == '\0') {
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
// Function: SideTouch_IDLEProcess @ 6bb06595
// ============================================================================

void SideTouch_IDLEProcess(void)

{
  char cVar1;
  
  if (DAT_6bc97188 < 0x1e) {
    DAT_6bc97188 = DAT_6bc97188 + 1;
  }
  cVar1 = SideTouch_IsAnyGestureInProcess();
  if (cVar1 != '\0') {
    DAT_6bc97188 = 0;
  }
  cVar1 = TSA_IsGripOKToIDLE();
  if (cVar1 == '\0') {
    if (DAT_6bc981f8 == '\0') {
      DAT_6bc97188 = 0;
    }
    if (DAT_6bc99280 == '\0') {
      DAT_6bc97188 = 0;
    }
  }
  return;
}


// ============================================================================
// Function: SideTouch_BaselineSideToFull @ 6bb08d96
// ============================================================================

void SideTouch_BaselineSideToFull(void)

{
  byte bVar1;
  byte local_9;
  
  for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bRows; local_9 = local_9 + 1) {
    *(undefined2 *)
     (*(longlong *)_refptr_g_tsaBufBlGrid +
     (ulonglong)((uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols) * 2) =
         *(undefined2 *)(&DAT_6bc97192 + (longlong)(int)(uint)local_9 * 0x30);
    *(short *)(*(longlong *)_refptr_g_tsaBufDifGrid +
              (ulonglong)((uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols) * 2) =
         *(short *)(_refptr_g_tsaBufSelf +
                   ((longlong)(int)((uint)_refptr_g_tsaPrmtFlash->bCols + (uint)local_9) + 0xb40) *
                   2) + *(short *)(&DAT_6bc97194 + (longlong)(int)(uint)local_9 * 0x30);
  }
  *(undefined4 *)_refptr_g_tsaBufSSBaseline = DAT_6bc98210;
  *(undefined4 *)_refptr_g_tsaBufSSDif = DAT_6bc98214;
  bVar1 = _refptr_g_tsaPrmtFlash->bCols - 1;
  for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bRows; local_9 = local_9 + 1) {
    *(undefined2 *)
     (*(longlong *)_refptr_g_tsaBufBlGrid +
     (ulonglong)((uint)bVar1 + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2) =
         *(undefined2 *)(&DAT_6bc9821a + (longlong)(int)(uint)local_9 * 0x30);
    *(short *)(*(longlong *)_refptr_g_tsaBufDifGrid +
              (ulonglong)
              ((uint)bVar1 + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2) =
         *(short *)(_refptr_g_tsaBufSelf +
                   ((longlong)(int)((uint)_refptr_g_tsaPrmtFlash->bCols + (uint)local_9) + 0xb40) *
                   2) + *(short *)(&DAT_6bc9821c + (longlong)(int)(uint)local_9 * 0x30);
  }
  *(undefined4 *)(_refptr_g_tsaBufSSBaseline + (longlong)(int)(uint)bVar1 * 4) = DAT_6bc99298;
  *(undefined4 *)(_refptr_g_tsaBufSSDif + (longlong)(int)(uint)bVar1 * 4) = DAT_6bc9929c;
  return;
}


// ============================================================================
// Function: SideTouch_HeatmapProcess @ 6bb09041
// ============================================================================

void SideTouch_HeatmapProcess(void)

{
  SideTouch_RawProcess();
  SideTouch_BaselineProcess();
  SideTouch_CalcDiff();
  return;
}


// ============================================================================
// Function: SideTouch_TouchProcess @ 6bb0a10d
// ============================================================================

/* WARNING: Function: ___chkstk_ms replaced with injection: alloca_probe */

void SideTouch_TouchProcess(void)

{
  TSAFlashPrmt *pTVar1;
  TSAPrpt *pTVar2;
  errno_t eVar3;
  undefined8 in_stack_ffffffffffffd948;
  undefined4 uVar4;
  undefined1 local_26a8 [9808];
  undefined8 local_58;
  undefined8 local_50;
  undefined8 local_48;
  undefined8 local_40;
  uint local_38;
  undefined8 local_28;
  undefined8 local_20;
  undefined2 local_18;
  errno_t local_10;
  uint local_c;
  
  uVar4 = (undefined4)((ulonglong)in_stack_ffffffffffffd948 >> 0x20);
  local_28 = *(undefined8 *)&_refptr_g_tsaPrpt[1].field_0x14;
  local_20 = *(undefined8 *)&_refptr_g_tsaPrpt[1].field_0x1c;
  local_18 = *(undefined2 *)&_refptr_g_tsaPrpt[1].field_0x24;
  local_58 = *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x81c;
  local_50 = *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x824;
  local_48 = *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x82c;
  local_40 = *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x834;
  local_38 = _refptr_g_tsaPrmtFlash->dwGripFlags;
  local_c = (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags;
  local_10 = 0x6bb0a118;
  local_10 = memcpy_s(local_26a8,0x2648,*(void **)_refptr_g_tsaBufDif,
                      (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  if (local_10 != 0) {
    uVar4 = 0;
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SideTouch/SideTouch.c",0x3da,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  local_10 = memset_s(*(undefined8 *)_refptr_g_tsaBufDif,0x2648,0,
                      (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  if (local_10 != 0) {
    uVar4 = 0;
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SideTouch/SideTouch.c",0x3dc,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  TSAPrmt_Process();
  Touch_Clean();
  SideTouch_DiffProcess();
  Peak_DetectInRange((int)*(short *)(_refptr_g_tsaPrmtDynamic + 8),0,
                     _refptr_g_tsaPrmtFlash->bRows + 0xff,0,
                     CONCAT44(uVar4,_refptr_g_tsaPrmtFlash->bCols - 1) & 0xffffffff000000ff);
  Peak_SortInSigAscending();
  _refptr_g_tsaPrmtFlash->field_0x81e = _refptr_g_tsaPrmtFlash->field_0x81f;
  _refptr_g_tsaPrmtFlash->field_0x820 = _refptr_g_tsaPrmtFlash->field_0x81f;
  _refptr_g_tsaPrmtFlash->dwGripFlags = 0x7b;
  TZ_Process();
  CTD_ECProcess();
  IDT_Process('\0');
  Touch_AssignPreIdxBasedOnID();
  PrevTouch_AssignCurIdxBasedOnID();
  TE_Process();
  SafeBaseline_CheckSideTouch();
  SideTouch_AgeProcess();
  TSAIDE_LogPreFltSideRpt();
  SideTouch_XYProcess();
  (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags = local_c;
  eVar3 = memcpy_s(*(void **)_refptr_g_tsaBufDif,0x2648,local_26a8,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  _local_10 = CONCAT44(local_c,eVar3);
  if (eVar3 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SideTouch/SideTouch.c",0x408,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  pTVar1 = _refptr_g_tsaPrmtFlash;
  *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x81c = local_58;
  *(undefined8 *)&pTVar1->field_0x824 = local_50;
  *(undefined8 *)&pTVar1->field_0x82c = local_48;
  *(undefined8 *)&pTVar1->field_0x834 = local_40;
  pTVar1->dwGripFlags = local_38;
  pTVar2 = _refptr_g_tsaPrpt;
  *(undefined8 *)&_refptr_g_tsaPrpt[1].field_0x14 = local_28;
  *(undefined8 *)&pTVar2[1].field_0x1c = local_20;
  *(undefined2 *)&pTVar2[1].field_0x24 = local_18;
  return;
}


// ============================================================================
// Function: SideTouch_ProcessExtInfo @ 6bb0a47b
// ============================================================================

void SideTouch_ProcessExtInfo(void)

{
  return;
}


// ============================================================================
// Function: SideTouch_IsFrameDataAvaliable @ 6bb0a4bd
// ============================================================================

undefined8 SideTouch_IsFrameDataAvaliable(void)

{
  longlong lVar1;
  undefined8 uVar2;
  
  lVar1 = AFE_GetFrame();
  if (*(longlong *)(lVar1 + 0x48) == 0) {
    lVar1 = AFE_GetFrame();
    if (*(longlong *)(lVar1 + 0x10) == 0) {
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
// Function: SideTouch_IDEProcess @ 6bb0ab8b
// ============================================================================

void SideTouch_IDEProcess(void)

{
  char cVar1;
  
  SideTouch_IDEProcessUnit(0,1,0,0);
  cVar1 = _refptr_g_tsaPrmtFlash->bCols + 0xff;
  SideTouch_IDEProcessUnit((int)cVar1,0xffffffff,1,(int)cVar1);
  return;
}


// ============================================================================
// Function: SideTouch_IsAllFeatureDisabled @ 6bb0dffa
// ============================================================================

undefined1 SideTouch_IsAllFeatureDisabled(void)

{
                    /* 0xcdffa  1368  SideTouch_IsAllFeatureDisabled */
  SideTouch_GestureIsPrmtChanged();
  return 0;
}


// ============================================================================
// Function: SideTouch_GestureProcess @ 6bb1ac07
// ============================================================================

void SideTouch_GestureProcess(void)

{
                    /* 0xdac07  1351  SideTouch_GestureProcess */
  SideTouch_GesturePrmtCheck();
  SideTouch_GestureDetect();
  SideTouch_GestureInternalProcess();
  SideTouch_GesturePostProcess();
  SideTouch_GestureLogProcess();
  return;
}


// ============================================================================
// Function: SideTouch_PressureProcess @ 6bb1ccbb
// ============================================================================

void SideTouch_PressureProcess(void)

{
                    /* 0xdccbb  1397  SideTouch_PressureProcess */
  return;
}


// ============================================================================
// Function: SideTouch_ReportProcess @ 6bb1f260
// ============================================================================

void SideTouch_ReportProcess(void)

{
  byte local_9;
  
                    /* 0xdf260  1401  SideTouch_ReportProcess */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&DAT_6bb59995,"SideTouch_ReportProcess");
  }
  *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 5) = 0;
  *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0xb) = 0;
  for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
    SideTouch_ReportProcessUnit
              (local_9,*(undefined1 *)
                        ((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x287));
  }
  return;
}


// ============================================================================
// Function: SideTouch_UpdateExternalThenarInfo @ 6bb0a481
// ============================================================================

undefined1 SideTouch_UpdateExternalThenarInfo(void)

{
  undefined1 uVar1;
  
                    /* 0xca481  1412  SideTouch_UpdateExternalThenarInfo */
  *(undefined1 **)_refptr_g_touchesPtr = &g_touchesSide;
  uVar1 = Thenar_UpdateExternalInfo();
  *(undefined **)_refptr_g_touchesPtr = _refptr_g_touchesGrid;
  return uVar1;
}


// ============================================================================
// Function: SideTouch_UnitCopyToOutReport @ 6bb1d48c
// ============================================================================

void SideTouch_UnitCopyToOutReport(byte param_1,byte param_2)

{
  byte local_res8;
  byte local_res10;
  
                    /* 0xdd48c  1411  SideTouch_UnitCopyToOutReport */
  if (0x13 < param_2) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SideTouch/SideTouch_Report.c",0xb6,
                      "sideIdx < TSA_MAX_TOUCH_NUM");
    HardwareAnalyzer_SetSoftException();
  }
  if (0x13 < param_1) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SideTouch/SideTouch_Report.c",0xb7,
                      "gridIdx < TSA_MAX_TOUCH_NUM");
    HardwareAnalyzer_SetSoftException();
  }
  local_res10 = param_2;
  if (0x13 < param_2) {
    local_res10 = 0x13;
  }
  local_res8 = param_1;
  if (0x13 < param_1) {
    local_res8 = 0x13;
  }
  *(undefined8 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x148) =
       *(undefined8 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x148);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x150) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x150);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x152) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x152);
  _refptr_g_touchesGrid[(longlong)(int)(uint)local_res8 * 0x168 + 0x154] =
       _refptr_g_touchesSide[(longlong)(int)(uint)local_res10 * 0x168 + 0x154] + '\x14';
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x156) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x156);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x15c) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x15c);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x15e) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x15e);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x160) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x160);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x162) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x162);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x164) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x164);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x166) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x166);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x168) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x168);
  _refptr_g_touchesGrid[(longlong)(int)(uint)local_res8 * 0x168 + 0x16a] =
       _refptr_g_touchesSide[(longlong)(int)(uint)local_res10 * 0x168 + 0x16a];
  _refptr_g_touchesGrid[(longlong)(int)(uint)local_res8 * 0x168 + 0x16b] =
       _refptr_g_touchesSide[(longlong)(int)(uint)local_res10 * 0x168 + 0x16b];
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x16c) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x16c);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x16e) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x16e);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x170) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x29a);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x172) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x29c);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x180) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x180);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x182) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x182);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x184) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x184);
  _refptr_g_touchesGrid[(longlong)(int)(uint)local_res8 * 0x168 + 0x186] =
       _refptr_g_touchesSide[(longlong)(int)(uint)local_res10 * 0x168 + 0x186];
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x188) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x188);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x18a) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x18a);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x18c) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x18c);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x18e) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x18e);
  _refptr_g_touchesGrid[(longlong)(int)(uint)local_res8 * 0x168 + 0x192] =
       _refptr_g_touchesSide[(longlong)(int)(uint)local_res10 * 0x168 + 0x192];
  _refptr_g_touchesGrid[(longlong)(int)(uint)local_res8 * 0x168 + 0x193] =
       _refptr_g_touchesSide[(longlong)(int)(uint)local_res10 * 0x168 + 0x193];
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x1a0) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x1a0);
  _refptr_g_touchesGrid[(longlong)(int)(uint)local_res8 * 0x168 + 0x194] =
       _refptr_g_touchesSide[(longlong)(int)(uint)local_res10 * 0x168 + 0x194];
  _refptr_g_touchesGrid[(longlong)(int)(uint)local_res8 * 0x168 + 0x196] =
       _refptr_g_touchesSide[(longlong)(int)(uint)local_res10 * 0x168 + 0x196];
  _refptr_g_touchesGrid[(longlong)(int)(uint)local_res8 * 0x168 + 0x197] =
       _refptr_g_touchesSide[(longlong)(int)(uint)local_res10 * 0x168 + 0x197];
  _refptr_g_touchesGrid[(longlong)(int)(uint)local_res8 * 0x168 + 0x198] =
       _refptr_g_touchesSide[(longlong)(int)(uint)local_res10 * 0x168 + 0x198];
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x1a8) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x1a8);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x1aa) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x1aa);
  _refptr_g_touchesGrid[(longlong)(int)(uint)local_res8 * 0x168 + 0x1ac] =
       _refptr_g_touchesSide[(longlong)(int)(uint)local_res10 * 0x168 + 0x1ac];
  _refptr_g_touchesGrid[(longlong)(int)(uint)local_res8 * 0x168 + 0x1ad] =
       _refptr_g_touchesSide[(longlong)(int)(uint)local_res10 * 0x168 + 0x1ad];
  _refptr_g_touchesGrid[(longlong)(int)(uint)local_res8 * 0x168 + 0x1ae] =
       _refptr_g_touchesSide[(longlong)(int)(uint)local_res10 * 0x168 + 0x1ae];
  _refptr_g_touchesGrid[(longlong)(int)(uint)local_res8 * 0x168 + 0x1af] =
       _refptr_g_touchesSide[(longlong)(int)(uint)local_res10 * 0x168 + 0x1af];
  _refptr_g_touchesGrid[(longlong)(int)(uint)local_res8 * 0x168 + 0x1b0] =
       _refptr_g_touchesSide[(longlong)(int)(uint)local_res10 * 0x168 + 0x1b0];
  _refptr_g_touchesGrid[(longlong)(int)(uint)local_res8 * 0x168 + 0x1b1] =
       _refptr_g_touchesSide[(longlong)(int)(uint)local_res10 * 0x168 + 0x1b1];
  *(undefined4 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x1c4) =
       *(undefined4 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x1c4);
  _refptr_g_touchesGrid[(longlong)(int)(uint)local_res8 * 0x168 + 0x1c8] =
       _refptr_g_touchesSide[(longlong)(int)(uint)local_res10 * 0x168 + 0x1c8];
  *(undefined4 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x1cc) =
       *(undefined4 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x1cc);
  *(undefined4 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x1d8) =
       *(undefined4 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x1d8);
  *(undefined4 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x1e0) =
       *(undefined4 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x1e0);
  *(undefined4 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x1e4) =
       *(undefined4 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x1e4);
  *(undefined4 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x1e8) =
       *(undefined4 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x1e8);
  *(undefined4 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x1ec) =
       *(undefined4 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x1ec);
  *(undefined4 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x1f0) =
       *(undefined4 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x1f0);
  *(undefined4 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 500) =
       *(undefined4 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 500);
  *(undefined4 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x1f8) =
       *(undefined4 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x1f8);
  *(undefined4 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x204) =
       *(undefined4 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x204);
  *(undefined4 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x208) =
       *(undefined4 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x208);
  *(undefined4 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x20c) =
       *(undefined4 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x20c);
  *(undefined4 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x210) =
       *(undefined4 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x210);
  *(undefined4 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x21c) =
       *(undefined4 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x21c);
  *(undefined4 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x224) =
       *(undefined4 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x224);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x228) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x228);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x22a) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x22a);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x22c) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x22c);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x22e) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x22e);
  _refptr_g_touchesGrid[(longlong)(int)(uint)local_res8 * 0x168 + 0x1b6] =
       _refptr_g_touchesSide[(longlong)(int)(uint)local_res10 * 0x168 + 0x1b6];
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x1be) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x1be);
  *(undefined4 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x1c0) =
       *(undefined4 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x1c0);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x238) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x238);
  _refptr_g_touchesGrid[(longlong)(int)(uint)local_res8 * 0x168 + 0x23c] =
       _refptr_g_touchesSide[(longlong)(int)(uint)local_res10 * 0x168 + 0x23c];
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x23e) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x23e);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x240) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x240);
  *(undefined4 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x244) =
       *(undefined4 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x244);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x24c) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x24c);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x250) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x250);
  *(undefined2 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x252) =
       *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x252);
  *(undefined4 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 0x254) =
       *(undefined4 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 0x254);
  *(undefined4 *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_res8 * 0x168 + 600) =
       *(undefined4 *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_res10 * 0x168 + 600);
  _refptr_g_touchesGrid[(longlong)(int)(uint)local_res8 * 0x168 + 0x278] =
       _refptr_g_touchesSide[(longlong)(int)(uint)local_res10 * 0x168 + 0x278];
  return;
}


// ============================================================================
// Function: SideTouch_IsOKToEnterIDLEMode @ 6bb06549
// ============================================================================

undefined4 SideTouch_IsOKToEnterIDLEMode(void)

{
                    /* 0xc6549  1378  SideTouch_IsOKToEnterIDLEMode */
  return CONCAT31((int3)((uint)DAT_6bc97188 >> 8),0x1d < DAT_6bc97188);
}


// ============================================================================
// Function: SideTouch_IsSHBVolumeEventEnabled @ 6bb0d882
// ============================================================================

undefined8 SideTouch_IsSHBVolumeEventEnabled(void)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0xcd882  1382  SideTouch_IsSHBVolumeEventEnabled */
  cVar1 = SideGesture_IsVolumeMapping();
  if (cVar1 == '\0') {
    if ((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6a0 & 0x80) == 0) {
      uVar2 = 0;
    }
    else {
      uVar2 = 1;
    }
  }
  else if (*_refptr_g_guiVolume == '\0') {
    if ((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6a0 & 0x80) == 0) {
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
// Function: SideTouch_IsSHBPowerEventEnabled @ 6bb0d8f4
// ============================================================================

undefined8 SideTouch_IsSHBPowerEventEnabled(void)

{
                    /* 0xcd8f4  1381  SideTouch_IsSHBPowerEventEnabled */
  return 0;
}


// ============================================================================
// Function: SideTouch_BaselineInit @ 6bb0bbc0
// ============================================================================

void SideTouch_BaselineInit(void)

{
                    /* 0xcbbc0  1336  SideTouch_BaselineInit */
  SideBaseline_ResetInit();
  return;
}


// ============================================================================
// Function: SideTouch_GestureInit @ 6bb16fa2
// ============================================================================

void SideTouch_GestureInit(void)

{
  int iVar1;
  
                    /* 0xd6fa2  1348  SideTouch_GestureInit */
  iVar1 = memset_s(&g_sideGesture,0x4310,0,0x4310);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SideTouch/SideTouch_Gesture.c",0x911,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  SideTouch_GesturePrmtHandler();
  SideTouch_SetRingTaskName("UnknownTask");
  return;
}


// ============================================================================
// Function: SideTouch_PressureInit @ 6bb1cc54
// ============================================================================

void SideTouch_PressureInit(void)

{
  int iVar1;
  
                    /* 0xdcc54  1395  SideTouch_PressureInit */
  iVar1 = memset_s(&g_outPressure,0xc,0,0xc);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SideTouch/SideTouch_Pressure.c",0x43,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: SideTouch_SHBBaselineInit @ 6bb1f310
// ============================================================================

void SideTouch_SHBBaselineInit(void)

{
  int iVar1;
  
                    /* 0xdf310  1402  SideTouch_SHBBaselineInit */
  iVar1 = memset_s(&g_blPrpt,0xf00,0,0xf00);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SideTouch/SideTouch_SHBBaseline.c",0x2b,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(&g_blStatic,0x280,0,0x280);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SideTouch/SideTouch_SHBBaseline.c",0x2d,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: SideTouch_IsAnyGestureInProcess @ 6bb0dcbc
// ============================================================================

undefined1 SideTouch_IsAnyGestureInProcess(void)

{
                    /* 0xcdcbc  1369  SideTouch_IsAnyGestureInProcess */
  return DAT_6bc9d7fa;
}


// ============================================================================
// Function: SideTouch_RawProcess @ 6bb06cb2
// ============================================================================

void SideTouch_RawProcess(void)

{
  byte bVar1;
  short sVar2;
  undefined2 uVar3;
  longlong lVar4;
  byte local_19;
  
  lVar4 = AFE_GetFrame();
  if (*(longlong *)(lVar4 + 0x10) == 0) {
    lVar4 = AFE_GetFrame();
    if (*(longlong *)(lVar4 + 0x48) != 0) {
      lVar4 = AFE_GetFrame();
      lVar4 = *(longlong *)(lVar4 + 0x48);
      for (local_19 = 0; local_19 < _refptr_g_tsaPrmtFlash->bRows; local_19 = local_19 + 1) {
        sVar2 = Rawdata_NormalizeGridUnit(*(undefined2 *)(lVar4 + (ulonglong)local_19 * 2));
        *(short *)(&DAT_6bc97196 + (longlong)(int)(uint)local_19 * 0x30) =
             sVar2 - *(short *)(&DAT_6bc97190 + (longlong)(int)(uint)local_19 * 0x30);
        uVar3 = Rawdata_NormalizeGridUnit(*(undefined2 *)(lVar4 + (ulonglong)local_19 * 2));
        *(undefined2 *)(&DAT_6bc97190 + (longlong)(int)(uint)local_19 * 0x30) = uVar3;
      }
      for (local_19 = 0; local_19 < _refptr_g_tsaPrmtFlash->bRows; local_19 = local_19 + 1) {
        sVar2 = Rawdata_NormalizeGridUnit(*(undefined2 *)(lVar4 + (ulonglong)local_19 * 2));
        *(short *)(&DAT_6bc9821e + (longlong)(int)(uint)local_19 * 0x30) =
             sVar2 - *(short *)(&DAT_6bc98218 + (longlong)(int)(uint)local_19 * 0x30);
        uVar3 = Rawdata_NormalizeGridUnit
                          (*(undefined2 *)
                            (lVar4 + (longlong)
                                     (int)((uint)_refptr_g_tsaPrmtFlash->bRows + (uint)local_19) * 2
                            ));
        *(undefined2 *)(&DAT_6bc98218 + (longlong)(int)(uint)local_19 * 0x30) = uVar3;
      }
    }
  }
  else {
    for (local_19 = 0; local_19 < _refptr_g_tsaPrmtFlash->bRows; local_19 = local_19 + 1) {
      *(short *)(&DAT_6bc97196 + (longlong)(int)(uint)local_19 * 0x30) =
           *(short *)((ulonglong)((uint)local_19 * (uint)_refptr_g_tsaPrmtFlash->bCols) * 2 +
                     *(longlong *)_refptr_g_tsaBufPreCMFRawGrid) -
           *(short *)(&DAT_6bc97190 + (longlong)(int)(uint)local_19 * 0x30);
      *(undefined2 *)(&DAT_6bc97190 + (longlong)(int)(uint)local_19 * 0x30) =
           *(undefined2 *)
            ((ulonglong)((uint)local_19 * (uint)_refptr_g_tsaPrmtFlash->bCols) * 2 +
            *(longlong *)_refptr_g_tsaBufPreCMFRawGrid);
    }
    DAT_6bc9820c = *(undefined4 *)_refptr_g_tsaBufSSRaw;
    bVar1 = _refptr_g_tsaPrmtFlash->bCols - 1;
    for (local_19 = 0; local_19 < _refptr_g_tsaPrmtFlash->bRows; local_19 = local_19 + 1) {
      *(short *)(&DAT_6bc9821e + (longlong)(int)(uint)local_19 * 0x30) =
           *(short *)((ulonglong)
                      ((uint)bVar1 + (uint)local_19 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)
                      * 2 + *(longlong *)_refptr_g_tsaBufPreCMFRawGrid) -
           *(short *)(&DAT_6bc98218 + (longlong)(int)(uint)local_19 * 0x30);
      *(undefined2 *)(&DAT_6bc98218 + (longlong)(int)(uint)local_19 * 0x30) =
           *(undefined2 *)
            ((ulonglong)
             ((uint)bVar1 + (uint)local_19 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
            *(longlong *)_refptr_g_tsaBufPreCMFRawGrid);
    }
    DAT_6bc99294 = *(undefined4 *)(_refptr_g_tsaBufSSRaw + (longlong)(int)(uint)bVar1 * 4);
  }
  return;
}


// ============================================================================
// Function: SideTouch_CalcDiff @ 6bb072cd
// ============================================================================

void SideTouch_CalcDiff(void)

{
  uchar uVar1;
  int iVar2;
  short sVar3;
  byte local_9;
  
  for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bRows; local_9 = local_9 + 1) {
    if ((*_refptr_g_tsaStaticPtr)->bField_0x11a == '\0') {
      sVar3 = *(short *)(&DAT_6bc97192 + (longlong)(int)(uint)local_9 * 0x30) -
              *(short *)(&DAT_6bc97190 + (longlong)(int)(uint)local_9 * 0x30);
    }
    else {
      sVar3 = *(short *)(&DAT_6bc97190 + (longlong)(int)(uint)local_9 * 0x30) -
              *(short *)(&DAT_6bc97192 + (longlong)(int)(uint)local_9 * 0x30);
    }
    *(short *)(&DAT_6bc97194 + (longlong)(int)(uint)local_9 * 0x30) = sVar3;
    iVar2 = CMF_GetCmfMethod();
    if ((iVar2 == 1) && (DAT_6bc98090 != 1)) {
      *(short *)(&DAT_6bc97194 + (longlong)(int)(uint)local_9 * 0x30) =
           *(short *)(&DAT_6bc97194 + (longlong)(int)(uint)local_9 * 0x30) -
           *(short *)((ulonglong)((uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols) * 2 +
                     *(longlong *)_refptr_g_tsaBufCommonModeNoise);
    }
  }
  if (_refptr_g_tsaPrmtDynamic[0x30] == '\0') {
    DAT_6bc98214 = DAT_6bc98210 - DAT_6bc9820c;
  }
  else {
    DAT_6bc98214 = DAT_6bc9820c - DAT_6bc98210;
  }
  uVar1 = _refptr_g_tsaPrmtFlash->bCols;
  for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bRows; local_9 = local_9 + 1) {
    if ((*_refptr_g_tsaStaticPtr)->bField_0x11a == '\0') {
      sVar3 = *(short *)(&DAT_6bc9821a + (longlong)(int)(uint)local_9 * 0x30) -
              *(short *)(&DAT_6bc98218 + (longlong)(int)(uint)local_9 * 0x30);
    }
    else {
      sVar3 = *(short *)(&DAT_6bc98218 + (longlong)(int)(uint)local_9 * 0x30) -
              *(short *)(&DAT_6bc9821a + (longlong)(int)(uint)local_9 * 0x30);
    }
    *(short *)(&DAT_6bc9821c + (longlong)(int)(uint)local_9 * 0x30) = sVar3;
    iVar2 = CMF_GetCmfMethod();
    if ((iVar2 == 1) && (DAT_6bc99118 != 1)) {
      *(short *)(&DAT_6bc9821c + (longlong)(int)(uint)local_9 * 0x30) =
           *(short *)(&DAT_6bc9821c + (longlong)(int)(uint)local_9 * 0x30) -
           *(short *)((ulonglong)
                      ((uint)(byte)(uVar1 - 1) + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols
                      & 0xffff) * 2 + *(longlong *)_refptr_g_tsaBufCommonModeNoise);
    }
  }
  if (_refptr_g_tsaPrmtDynamic[0x30] == '\0') {
    DAT_6bc9929c = DAT_6bc99298 - DAT_6bc99294;
  }
  else {
    DAT_6bc9929c = DAT_6bc99294 - DAT_6bc99298;
  }
  SideTouch_CalcNotchDiff();
  return;
}


// ============================================================================
// Function: SideTouch_BaselineProcess @ 6bb08d32
// ============================================================================

void SideTouch_BaselineProcess(void)

{
  DAT_6bc97181 = 0;
  SideTouch_UpdateForRawUnstable();
  if ((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x624 & 0x80) == 0) {
    SideTouch_BaselineProcessSide(0);
    SideTouch_BaselineProcessSide(1);
  }
  else {
    SideTouch_BaselineSideProcess(0);
    SideTouch_BaselineSideProcess(1);
  }
  SideTouch_IDLEProcess();
  return;
}


// ============================================================================
// Function: SideTouch_AgeProcess @ 6bb07f48
// ============================================================================

void SideTouch_AgeProcess(void)

{
  uchar uVar1;
  uchar uVar2;
  short sVar3;
  int iVar4;
  byte local_9;
  
  SideTouch_AgeProcessUnit(0,0);
  SideTouch_AgeProcessUnit(_refptr_g_tsaPrmtFlash->bCols + 0xff,1);
  for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
    if (*(int *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c)
        == 2) {
      uVar1 = (*_refptr_g_peaksPtr)
              [(longlong)
               (int)(uint)*(byte *)((longlong)(int)(uint)local_9 * 0x168 +
                                    *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x15].
              bPeakCount;
      uVar2 = (*_refptr_g_peaksPtr)
              [(longlong)
               (int)(uint)*(byte *)((longlong)(int)(uint)local_9 * 0x168 +
                                    *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x16].
              bPeakCount;
      iVar4 = SideTouch_GetAge(uVar1,uVar2);
      if (0x1e < iVar4 - ((int)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
                         (int)*(undefined8 *)
                               ((longlong)*_refptr_g_prevTouchesPtr +
                               (longlong)
                               (int)(uint)*(byte *)((longlong)(int)(uint)local_9 * 0x168 +
                                                    *(longlong *)_refptr_g_touchesPtr + 0x287) *
                               0x478 + 0x4c0))) {
        *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1fc)
             = *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x1fc) | 4;
      }
      sVar3 = SideTouch_GetMinDifOffDown(uVar1,uVar2);
      if (*(short *)&_refptr_g_tsaPrmtFlash[2].field_0x634 / 2 < sVar3) {
        *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1fc)
             = *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x1fc) | 0x80;
      }
      iVar4 = SideTouch_GetAntiTriggerCnt(uVar1,uVar2);
      if (1 < iVar4) {
        *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1fc)
             = *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x1fc) | 0x100;
      }
    }
  }
  return;
}


// ============================================================================
// Function: SideTouch_XYProcess @ 6bb09d97
// ============================================================================

void SideTouch_XYProcess(void)

{
  undefined1 uVar1;
  byte local_9;
  
  for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
    uVar1 = PrevTouch_GetIdxBasedOnCurIdx(local_9);
    *(undefined4 *)
     ((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1e8) = 6;
    SideTouch_XYCorrection(local_9,uVar1);
    SideTouch_XYFilter(local_9,uVar1);
  }
  return;
}


// ============================================================================
// Function: SideTouch_DiffProcess @ 6bb09e1a
// ============================================================================

void SideTouch_DiffProcess(void)

{
  byte bVar1;
  int iVar2;
  byte local_9;
  
  for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bRows; local_9 = local_9 + 1) {
    iVar2 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x636 *
            (int)*(short *)(&DAT_6bc97194 + (longlong)(int)(uint)local_9 * 0x30);
    if (iVar2 < 0) {
      iVar2 = iVar2 + 0xf;
    }
    *(short *)(*(longlong *)_refptr_g_tsaBufDif +
              (ulonglong)((uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols) * 2) =
         (short)(iVar2 >> 4);
  }
  **(undefined2 **)_refptr_g_tsaBufDif =
       *(undefined2 *)
        ((ulonglong)_refptr_g_tsaPrmtFlash->bCols * 2 + *(longlong *)_refptr_g_tsaBufDif);
  *(undefined2 *)
   (*(longlong *)_refptr_g_tsaBufDif +
   (ulonglong)((_refptr_g_tsaPrmtFlash->bRows - 1) * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) *
   2) = *(undefined2 *)
         ((ulonglong)
          ((_refptr_g_tsaPrmtFlash->bRows - 2) * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
         *(longlong *)_refptr_g_tsaBufDif);
  bVar1 = _refptr_g_tsaPrmtFlash->bCols - 1;
  for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bRows; local_9 = local_9 + 1) {
    iVar2 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x636 *
            (int)*(short *)(&DAT_6bc9821c + (longlong)(int)(uint)local_9 * 0x30);
    if (iVar2 < 0) {
      iVar2 = iVar2 + 0xf;
    }
    *(short *)(*(longlong *)_refptr_g_tsaBufDif +
              (ulonglong)
              ((uint)bVar1 + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2) =
         (short)(iVar2 >> 4);
  }
  *(undefined2 *)((ulonglong)bVar1 * 2 + *(longlong *)_refptr_g_tsaBufDif) =
       *(undefined2 *)
        ((ulonglong)((uint)bVar1 + (uint)_refptr_g_tsaPrmtFlash->bCols) * 2 +
        *(longlong *)_refptr_g_tsaBufDif);
  *(undefined2 *)
   (*(longlong *)_refptr_g_tsaBufDif +
   (ulonglong)
   ((uint)bVar1 + (_refptr_g_tsaPrmtFlash->bRows - 1) * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff
   ) * 2) = *(undefined2 *)
             ((ulonglong)
              ((uint)bVar1 +
               (_refptr_g_tsaPrmtFlash->bRows - 2) * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) *
              2 + *(longlong *)_refptr_g_tsaBufDif);
  return;
}


// ============================================================================
// Function: SideTouch_IDEProcessUnit @ 6bb0a4fa
// ============================================================================

void SideTouch_IDEProcessUnit(char param_1,char param_2,uint param_3,char param_4)

{
  byte bVar1;
  char cVar2;
  char cVar3;
  undefined2 uVar4;
  byte local_22;
  byte local_21;
  byte local_20;
  byte local_1f;
  byte local_1e;
  byte local_1d;
  byte local_1c;
  byte local_1b;
  byte local_1a;
  byte local_19;
  
  for (local_19 = 0; local_19 < _refptr_g_tsaPrmtFlash->bRows; local_19 = local_19 + 1) {
    *(undefined2 *)
     (&g_tsaBufSideTouch +
     (longlong)
     (int)((uint)(ushort)(short)param_1 + (uint)local_19 * (uint)_refptr_g_tsaPrmtFlash->bCols &
          0xffff) * 2) =
         *(undefined2 *)
          (&DAT_6bc97194 + (longlong)(int)(uint)local_19 * 0x30 + (ulonglong)param_3 * 0x1088);
  }
  for (local_1a = 0; local_1a < _refptr_g_tsaPrmtFlash->bRows; local_1a = local_1a + 1) {
    *(undefined2 *)
     (&g_tsaBufSideTouch +
     (longlong)
     (int)((uint)(ushort)(short)(char)(param_2 + param_1) +
           (uint)local_1a * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2) =
         *(undefined2 *)
          (&DAT_6bc97198 + (longlong)(int)(uint)local_1a * 0x30 + (ulonglong)param_3 * 0x1088);
  }
  cVar2 = param_2 + param_2 + param_1;
  for (local_1b = 0; local_1b < _refptr_g_tsaPrmtFlash->bRows; local_1b = local_1b + 1) {
    *(short *)(&g_tsaBufSideTouch +
              (longlong)
              (int)((uint)(ushort)(short)cVar2 +
                    (uint)local_1b * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2) =
         *(short *)(&DAT_6bc97192 +
                   (longlong)(int)(uint)local_1b * 0x30 + (ulonglong)param_3 * 0x1088) + -10000;
  }
  cVar2 = param_2 + cVar2;
  for (local_1c = 0; local_1c < _refptr_g_tsaPrmtFlash->bRows; local_1c = local_1c + 1) {
    bVar1 = _refptr_g_tsaPrmtFlash->bCols;
    uVar4 = SideTouch_GetAge(param_4,local_1c);
    *(undefined2 *)
     (&g_tsaBufSideTouch +
     (longlong)(int)((uint)(ushort)(short)cVar2 + (uint)local_1c * (uint)bVar1 & 0xffff) * 2) =
         uVar4;
  }
  cVar2 = param_2 + cVar2;
  for (local_1d = 0; local_1d < _refptr_g_tsaPrmtFlash->bRows; local_1d = local_1d + 1) {
    bVar1 = _refptr_g_tsaPrmtFlash->bCols;
    uVar4 = SideTouch_GetPrevAge(param_4,local_1d);
    *(undefined2 *)
     (&g_tsaBufSideTouch +
     (longlong)(int)((uint)(ushort)(short)cVar2 + (uint)local_1d * (uint)bVar1 & 0xffff) * 2) =
         uVar4;
  }
  cVar2 = param_2 + cVar2;
  for (local_1e = 0; local_1e < _refptr_g_tsaPrmtFlash->bRows; local_1e = local_1e + 1) {
    *(short *)(&g_tsaBufSideTouch +
              (longlong)
              (int)((uint)(ushort)(short)cVar2 +
                    (uint)local_1e * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2) =
         (short)*(char *)((ulonglong)
                          ((uint)(ushort)(short)param_4 +
                           (uint)local_1e * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) +
                         *(longlong *)_refptr_g_tsaBufTz);
  }
  cVar2 = param_2 + cVar2;
  for (local_1f = 0; local_1f < _refptr_g_tsaPrmtFlash->bRows; local_1f = local_1f + 1) {
    *(undefined2 *)
     (&g_tsaBufSideTouch +
     (longlong)
     (int)((uint)(ushort)(short)cVar2 + (uint)local_1f * (uint)_refptr_g_tsaPrmtFlash->bCols &
          0xffff) * 2) = 0;
  }
  for (local_20 = 0; local_20 < **(byte **)_refptr_g_touchesPtr; local_20 = local_20 + 1) {
    cVar3 = Peak_IsTouchValid(local_20);
    if ((cVar3 != '\0') &&
       ((*_refptr_g_peaksPtr)
        [(longlong)
         (int)(uint)*(byte *)((longlong)(int)(uint)local_20 * 0x168 +
                              *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x15].bPeakCount
        == '\0')) {
      *(short *)(&g_tsaBufSideTouch +
                (longlong)
                (int)((uint)(ushort)(short)cVar2 +
                      (uint)(*_refptr_g_peaksPtr)
                            [(longlong)
                             (int)(uint)*(byte *)((longlong)(int)(uint)local_20 * 0x168 +
                                                  *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14
                             + 0x16].bPeakCount * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2)
           = (short)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
             (short)*(undefined8 *)
                     ((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)
                     (int)(uint)*(byte *)((longlong)(int)(uint)local_20 * 0x168 +
                                          *(longlong *)_refptr_g_touchesPtr + 0x287) * 0x478 + 0x4c0
                     );
    }
  }
  for (local_21 = 0; local_21 < _refptr_g_tsaPrmtFlash->bRows; local_21 = local_21 + 1) {
    bVar1 = _refptr_g_tsaPrmtFlash->bCols;
    uVar4 = SideTouch_SHBGetStaticData(param_3,local_21);
    *(undefined2 *)
     (&g_tsaBufSideTouch +
     (longlong)
     (int)((uint)(ushort)(short)(char)(param_2 + cVar2) + (uint)local_21 * (uint)bVar1 & 0xffff) * 2
     ) = uVar4;
  }
  param_2 = param_2 + param_2 + cVar2;
  for (local_22 = 0; local_22 < _refptr_g_tsaPrmtFlash->bRows; local_22 = local_22 + 1) {
    if (*(longlong *)
         (&DAT_6bc971b8 + (longlong)(int)(uint)local_22 * 0x30 + (ulonglong)param_3 * 0x1088) == 0)
    {
      *(undefined2 *)
       (&g_tsaBufSideTouch +
       (longlong)
       (int)((uint)(ushort)(short)param_2 + (uint)local_22 * (uint)_refptr_g_tsaPrmtFlash->bCols &
            0xffff) * 2) = 0;
    }
    else {
      *(short *)(&g_tsaBufSideTouch +
                (longlong)
                (int)((uint)(ushort)(short)param_2 +
                      (uint)local_22 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2) =
           (short)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
           (short)*(undefined8 *)
                   (&DAT_6bc971b8 +
                   (longlong)(int)(uint)local_22 * 0x30 + (ulonglong)param_3 * 0x1088);
    }
  }
  return;
}


// ============================================================================
// Function: SideTouch_GestureIsPrmtChanged @ 6bb0dee6
// ============================================================================

undefined8 SideTouch_GestureIsPrmtChanged(void)

{
  undefined8 uVar1;
  
  if (DAT_6bc9d93c == *(int *)&_refptr_g_tsaPrmtFlash[2].field_0x6a0) {
    if (DAT_6bc9d8e8 == *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x64c) {
      if (DAT_6bc9d8e0 == *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x644) {
        if (DAT_6bc9d8e2 == *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x646) {
          if (DAT_6bc9d8d8 == *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x63c) {
            if (DAT_6bc9d8da == *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x63e) {
              if (DAT_6bc9d8ea == *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x64e) {
                uVar1 = 0;
              }
              else {
                uVar1 = 1;
              }
            }
            else {
              uVar1 = 1;
            }
          }
          else {
            uVar1 = 1;
          }
        }
        else {
          uVar1 = 1;
        }
      }
      else {
        uVar1 = 1;
      }
    }
    else {
      uVar1 = 1;
    }
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: SideTouch_GesturePrmtCheck @ 6bb1690a
// ============================================================================

void SideTouch_GesturePrmtCheck(void)

{
  char cVar1;
  char local_9;
  
  cVar1 = SideTouch_GestureIsPrmtChanged();
  if (cVar1 != '\0') {
    SideTouch_GestureKeepExternal();
    SideTouch_GesturePrmtHandler();
  }
  SideTouch_ClearGestureEnabled(0x100000);
  local_9 = g_sideGesture;
  do {
    local_9 = local_9 + -1;
    if (local_9 < '\0') {
      return;
    }
  } while (*(int *)(&DAT_6bc997e0 + (longlong)(int)local_9 * 0x2a8) != 0x100000);
  SideTouch_SetGestureEnabled(0x100000);
  return;
}


// ============================================================================
// Function: SideTouch_GesturePostProcess @ 6bb17023
// ============================================================================

void SideTouch_GesturePostProcess(void)

{
  char cVar1;
  byte local_9;
  
  VirtualKey_UpdateVolumeActive();
  cVar1 = VirtualKey_VolumeIsGUIActive();
  if (cVar1 == '\0') {
    DAT_6bc9d804 = 0;
  }
  else {
    DAT_6bc9d804 = DAT_6bc9d808 & 0x60;
  }
  for (local_9 = 0; local_9 < g_sideGesture; local_9 = local_9 + 1) {
    (&DAT_6bc997c1)[(longlong)(int)(uint)local_9 * 0x2a8] =
         (&DAT_6bc997c0)[(longlong)(int)(uint)local_9 * 0x2a8];
    cVar1 = SideTouch_IsGestureActive(local_9);
    if (cVar1 != '\0') {
      cVar1 = SideGesture_IsLeftVolumeTrigger(local_9);
      if (cVar1 != '\0') {
        cVar1 = SideGesture_IsLeftVolumeUpTrigger(local_9);
        if (cVar1 == '\0') {
          DAT_6bc9d804 = DAT_6bc9d804 | 2;
        }
        else {
          DAT_6bc9d804 = DAT_6bc9d804 | 1;
        }
        DAT_6bc9d804 = DAT_6bc9d804 & 0xffffffbf | 0x20;
      }
      cVar1 = SideGesture_IsRightVolumeTrigger(local_9);
      if (cVar1 != '\0') {
        cVar1 = SideGesture_IsRightVolumeUpTrigger(local_9);
        if (cVar1 == '\0') {
          DAT_6bc9d804 = DAT_6bc9d804 | 8;
        }
        else {
          DAT_6bc9d804 = DAT_6bc9d804 | 4;
        }
        DAT_6bc9d804 = DAT_6bc9d804 & 0xffffffdf | 0x40;
      }
      cVar1 = SideGesture_IsVolumeFineTune(local_9);
      if (cVar1 != '\0') {
        DAT_6bc9d804 = DAT_6bc9d804 | 0x10;
      }
    }
  }
  DAT_6bc9d808 = DAT_6bc9d804;
  SideTouch_GestureVolumeAreaUpdate();
  SideTouch_GestureAntiTriggerGlobally();
  cVar1 = VirtualKey_VolumeIsGUIActive();
  if (cVar1 == '\0') {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) & 0xfffff3ff;
  }
  else {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
         *(uint *)&(*_refptr_g_prevTouchesPtr)->field_0x18 & 0xc00 |
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18);
  }
  return;
}


// ============================================================================
// Function: SideTouch_GestureInternalProcess @ 6bb1a28d
// ============================================================================

void SideTouch_GestureInternalProcess(void)

{
  char cVar1;
  byte local_9;
  
  DAT_6bc9d7fa = 0;
  local_9 = 0;
  while (((local_9 < g_sideGesture &&
          (*(longlong *)(&DAT_6bc99810 + (longlong)(int)(uint)local_9 * 0x2a8) != 0)) &&
         (*(longlong *)(&DAT_6bc99818 + (longlong)(int)(uint)local_9 * 0x2a8) != 0))) {
    cVar1 = (**(code **)(&DAT_6bc99810 + (longlong)(int)(uint)local_9 * 0x2a8))(local_9);
    if ((cVar1 != '\0') &&
       ((**(code **)(&DAT_6bc99818 + (longlong)(int)(uint)local_9 * 0x2a8))(local_9),
       (&DAT_6bc997c0)[(longlong)(int)(uint)local_9 * 0x2a8] != '\0')) {
      DAT_6bc9d7fa = 1;
    }
    local_9 = local_9 + 1;
  }
  return;
}


// ============================================================================
// Function: SideTouch_GestureDetect @ 6bb1abca
// ============================================================================

void SideTouch_GestureDetect(void)

{
  byte local_9;
  
  SideTouch_ExcludeAllUnstableTouch();
  for (local_9 = 0; local_9 < g_sideGesture; local_9 = local_9 + 1) {
    SideTouch_GestureProcessRegion(local_9);
  }
  return;
}


// ============================================================================
// Function: SideTouch_GestureLogProcess @ 6bb1c944
// ============================================================================

void SideTouch_GestureLogProcess(void)

{
  byte bVar1;
  char cVar2;
  char cVar3;
  undefined1 uVar4;
  byte local_9;
  
                    /* 0xdc944  1349  SideTouch_GestureLogProcess */
  if ((*_refptr_g_tsaStaticPtr)->pFrameBufferPtr != lastExecutedTimeRecord_10680) {
    lastExecutedTimeRecord_10680 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    (*_refptr_g_tsaStaticPtr)->nField_0x298 = (*_refptr_g_tsaStaticPtr)->nField_0x298 & 0xfffffffd;
    for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
      if ((((*(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x1f8) & 0x10000000) == 0) &&
          ((*(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1f8) & 0x8000000) != 0)) &&
         (bVar1 = PrevTouch_GetIdxBasedOnCurIdx(local_9), bVar1 < 0x14)) {
        cVar2 = SideTouch_IsToLog(local_9,bVar1);
        cVar3 = SideTouch_IsVolumeTriggered(local_9,bVar1);
        uVar4 = SideTouch_IsVolumeAntiTriggered(local_9,bVar1);
        if ((cVar2 != '\0') || (cVar3 != '\0')) {
          *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8
                   ) = *(uint *)((longlong)(int)(uint)local_9 * 0x168 +
                                 *(longlong *)_refptr_g_touchesPtr + 0x1f8) | 0x10000000;
          (*_refptr_g_tsaStaticPtr)->nField_0x298 = (*_refptr_g_tsaStaticPtr)->nField_0x298 | 2;
          SideTouch_RingPrptPrint(local_9,bVar1);
          *(undefined2 *)(_refptr_g_sideGesture + 0x42a0) = 0xf0;
          *(undefined2 *)(_refptr_g_sideGesture + 0x42a2) = 0x1e;
          SideTouch_RingLogNameGenerate(bVar1,cVar3,uVar4);
          SideTouch_TriggeredGestureLogPrint(local_9,bVar1,uVar4);
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: SideTouch_ReportProcessUnit @ 6bb1ec31
// ============================================================================

void SideTouch_ReportProcessUnit(byte param_1,byte param_2)

{
  char cVar1;
  
  cVar1 = SideTouch_ToBeReported(param_1,param_2);
  if (cVar1 == '\0') {
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
         & 0xfffffff7;
  }
  else {
    *(char *)(*(longlong *)_refptr_g_touchesPtr + 5) =
         *(char *)(*(longlong *)_refptr_g_touchesPtr + 5) + '\x01';
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
         | 8;
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
         | 0x100000;
  }
  if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) &
      8) == 0) {
    if ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_2 * 0x478 + 0x1f8) & 0x100000) == 0) {
      *(undefined2 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x49a) =
           *(undefined2 *)
            ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170);
      *(undefined2 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x49c) =
           *(undefined2 *)
            ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
      *(undefined2 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4f8) =
           *(undefined2 *)
            ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170);
      *(undefined2 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4fa) =
           *(undefined2 *)
            ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
      *(void **)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x500)
           = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
      *(undefined2 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4fa) =
           *(undefined2 *)
            ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
      *(undefined2 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4fc) =
           *(undefined2 *)
            ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170);
      *(undefined2 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4fe) =
           *(undefined2 *)
            ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
    }
  }
  else if ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)param_2 * 0x478 + 0x1f8) & 8) == 0) {
    *(undefined2 *)
     ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x49a) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170);
    *(undefined2 *)
     ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x49c) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
    *(undefined2 *)
     ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4f8) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170);
    *(undefined2 *)
     ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4fa) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
    *(void **)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x500) =
         (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    *(undefined2 *)
     ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4fa) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
    *(undefined2 *)
     ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4fc) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170);
    *(undefined2 *)
     ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4fe) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
  }
  return;
}


// ============================================================================
// Function: SideTouch_IsAbnormalBaselineDetected @ 6bb06629
// ============================================================================

undefined1 SideTouch_IsAbnormalBaselineDetected(void)

{
                    /* 0xc6629  1367  SideTouch_IsAbnormalBaselineDetected */
  return DAT_6bc97181;
}


// ============================================================================
// Function: SideTouch_AppendAbnormalBaselineDescription @ 6bb0663a
// ============================================================================

void SideTouch_AppendAbnormalBaselineDescription(void)

{
                    /* 0xc663a  1333  SideTouch_AppendAbnormalBaselineDescription */
  HardwareAnalyzer_AppendExceptionDescription2
            ("AbnormalSideBaselineDetected",(longlong)DAT_6bc97182);
  return;
}


// ============================================================================
// Function: SideTouch_IsVolumeGUIOnTimeOut @ 6bb1a13a
// ============================================================================

undefined1 SideTouch_IsVolumeGUIOnTimeOut(void)

{
  char cVar1;
  undefined1 local_9;
  
                    /* 0xda13a  1386  SideTouch_IsVolumeGUIOnTimeOut */
  local_9 = 0;
  cVar1 = SideGesture_IsVolumeMapping();
  if (cVar1 != '\0') {
    cVar1 = SideTouch_IsVolumeGUIShown();
    if (((cVar1 == '\0') && (DAT_6bc9d818 != -1)) &&
       ((ulonglong)*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x678 <
        (ulonglong)((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - DAT_6bc9d818))) {
      local_9 = 1;
    }
  }
  return local_9;
}


// ============================================================================
// Function: SideTouch_AppendVolumeGUIOnTimeOutDescription @ 6bb1a1b0
// ============================================================================

void SideTouch_AppendVolumeGUIOnTimeOutDescription(void)

{
                    /* 0xda1b0  1335  SideTouch_AppendVolumeGUIOnTimeOutDescription */
  HardwareAnalyzer_AppendExceptionDescription2("VolumeGUIOnTimeOut",DAT_6bc9d818);
  return;
}


// ============================================================================
// Function: SideTouch_IsVolumeGUIOffTimeOut @ 6bb1a1dc
// ============================================================================

undefined1 SideTouch_IsVolumeGUIOffTimeOut(void)

{
  char cVar1;
  undefined1 local_9;
  
                    /* 0xda1dc  1385  SideTouch_IsVolumeGUIOffTimeOut */
  local_9 = 0;
  cVar1 = SideGesture_IsVolumeMapping();
  if ((((cVar1 != '\0') && (*_refptr_g_guiVolume != '\0')) && (DAT_6bc9d810 != -1)) &&
     ((ulonglong)*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x67c <
      (ulonglong)((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - DAT_6bc9d810))) {
    local_9 = 1;
    *_refptr_g_guiVolume = 0;
  }
  return local_9;
}


// ============================================================================
// Function: SideTouch_AppendVolumeGUIOffTimeOutDescription @ 6bb1a261
// ============================================================================

void SideTouch_AppendVolumeGUIOffTimeOutDescription(void)

{
                    /* 0xda261  1334  SideTouch_AppendVolumeGUIOffTimeOutDescription */
  HardwareAnalyzer_AppendExceptionDescription2("VolumeGUIOffTimeOut",DAT_6bc9d810);
  return;
}


// ============================================================================
// Function: SideTouch_GesturePrmtHandler @ 6bb16741
// ============================================================================

void SideTouch_GesturePrmtHandler(void)

{
  int iVar1;
  
  iVar1 = memset_s(_refptr_g_guiVolume,0x28,0,0x28);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SideTouch/SideTouch_Gesture.c",0x886,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  SideTouch_GestureSetUp
            (*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6b8 |
             *(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6a0);
  if (DAT_6bc9d8c4 != *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x628) {
    (*_refptr_g_tsaStaticPtr)->wField_0x2e0 = *(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x628;
  }
  if (DAT_6bc9d8c6 != *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x62a) {
    (*_refptr_g_tsaStaticPtr)->nXResMax = *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x62a;
  }
  memcpy(&g_sideTouchPrmtPrev,&_refptr_g_tsaPrmtFlash[2].field_0x624,0x9c);
  return;
}


// ============================================================================
// Function: SideTouch_SetRingTaskName @ 6bb1acb6
// ============================================================================

undefined8 SideTouch_SetRingTaskName(char *param_1)

{
  errno_t eVar1;
  
                    /* 0xdacb6  1408  SideTouch_SetRingTaskName */
  eVar1 = strncpy_s(&DAT_6bc9d856,0x32,param_1,0x31);
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SideTouch/SideTouch_Gesture.c",0xdc3,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  DAT_6bc9d887 = 0;
  return 0;
}


// ============================================================================
// Function: SideTouch_CalcNotchDiff @ 6bb07136
// ============================================================================

void SideTouch_CalcNotchDiff(void)

{
  byte bVar1;
  byte local_9;
  
  for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bRows; local_9 = local_9 + 1) {
    if ((*(byte *)((ulonglong)((uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols) +
                  *(longlong *)_refptr_g_tpSensorFlag) & 0x20) != 0) {
      *(undefined2 *)(&DAT_6bc97194 + (longlong)(int)(uint)local_9 * 0x30) =
           *(undefined2 *)
            ((ulonglong)((uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols) * 2 +
            *(longlong *)_refptr_g_tsaBufDif);
    }
  }
  bVar1 = _refptr_g_tsaPrmtFlash->bCols - 1;
  for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bRows; local_9 = local_9 + 1) {
    if ((*(byte *)((ulonglong)
                   ((uint)bVar1 + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) +
                  *(longlong *)_refptr_g_tpSensorFlag) & 0x20) != 0) {
      *(undefined2 *)(&DAT_6bc9821c + (longlong)(int)(uint)local_9 * 0x30) =
           *(undefined2 *)
            ((ulonglong)((uint)bVar1 + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)
             * 2 + *(longlong *)_refptr_g_tsaBufDif);
    }
  }
  return;
}


// ============================================================================
// Function: SideTouch_BaselineProcessSide @ 6bb088e8
// ============================================================================

void SideTouch_BaselineProcessSide(uint param_1)

{
  char cVar1;
  undefined4 uVar2;
  longlong lVar3;
  undefined *puVar4;
  byte local_9;
  
  lVar3 = (ulonglong)param_1 * 0x1088;
  puVar4 = &DAT_6bc97190 + lVar3;
  SideTouch_BaselineStageUpdate(puVar4,param_1);
  if (*(int *)(&DAT_6bc981fc + lVar3) == 1) {
    if (*(int *)(&DAT_6bc98200 + lVar3) < 0x14) {
      cVar1 = BLReset_CheckNumOverNoise();
      if (cVar1 == '\0') {
        *(undefined4 *)(&DAT_6bc98200 + lVar3) = 0;
      }
      else {
        *(int *)(&DAT_6bc98200 + lVar3) = *(int *)(&DAT_6bc98200 + lVar3) + 1;
      }
    }
    else {
      for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bRows; local_9 = local_9 + 1) {
        *(undefined2 *)(puVar4 + (longlong)(int)(uint)local_9 * 0x30 + 2) =
             *(undefined2 *)(puVar4 + (longlong)(int)(uint)local_9 * 0x30);
      }
      (&DAT_6bc98210)[(ulonglong)param_1 * 0x422] = (&DAT_6bc9820c)[(ulonglong)param_1 * 0x422];
      *(undefined4 *)(&DAT_6bc98200 + lVar3) = 0;
      *(undefined4 *)(&DAT_6bc98204 + lVar3) = 0;
    }
  }
  else {
    *(undefined4 *)(&DAT_6bc98200 + lVar3) = 0;
    if (*(int *)(&DAT_6bc981fc + lVar3) == 0) {
      if (*(int *)(&DAT_6bc98204 + lVar3) < 0x1e) {
        *(int *)(&DAT_6bc98204 + lVar3) = *(int *)(&DAT_6bc98204 + lVar3) + 1;
      }
      else {
        (&DAT_6bc981f8)[lVar3] = 1;
        uVar2 = IIR_Process2((&DAT_6bc9820c)[(ulonglong)param_1 * 0x422],
                             (&DAT_6bc98210)[(ulonglong)param_1 * 0x422],1,4);
        (&DAT_6bc98210)[(ulonglong)param_1 * 0x422] = uVar2;
      }
    }
    else {
      *(undefined4 *)(&DAT_6bc98204 + lVar3) = 0;
    }
  }
  (&DAT_6bc981f9)[(ulonglong)param_1 * 0x1088] = *(int *)(&DAT_6bc981fc + lVar3) == 0;
  return;
}


// ============================================================================
// Function: SideTouch_UpdateForRawUnstable @ 6bb08d0f
// ============================================================================

void SideTouch_UpdateForRawUnstable(void)

{
  SideTouch_UpdateForRawUnstableSide(0);
  SideTouch_UpdateForRawUnstableSide(1);
  return;
}


// ============================================================================
// Function: SideTouch_BaselineSideProcess @ 6bb0cdb6
// ============================================================================

void SideTouch_BaselineSideProcess(uint param_1)

{
  undefined2 uVar1;
  undefined8 uVar2;
  undefined8 uVar3;
  undefined8 uVar4;
  undefined8 uVar5;
  undefined8 uVar6;
  undefined *puVar7;
  undefined1 uVar8;
  
                    /* 0xccdb6  1337  SideTouch_BaselineSideProcess */
  uVar2 = *(undefined8 *)(_refptr_g_tsaPrmtDynamic + 0x42);
  uVar3 = *(undefined8 *)(_refptr_g_tsaPrmtDynamic + 0x4a);
  uVar4 = *(undefined8 *)(_refptr_g_tsaPrmtDynamic + 0x52);
  uVar5 = *(undefined8 *)(_refptr_g_tsaPrmtDynamic + 0x5a);
  uVar6 = *(undefined8 *)(_refptr_g_tsaPrmtDynamic + 0x62);
  uVar1 = *(undefined2 *)(_refptr_g_tsaPrmtDynamic + 0x6a);
  TSADynPrmt_GetBL();
  if (param_1 == 0) {
    g_baselineSide = 4;
  }
  else {
    g_baselineSide = 8;
  }
  g_baselinePrpt = (int *)(_refptr_g_sideData + (ulonglong)param_1 * 0x1088 + 0xf10);
  g_baselineStatic = _refptr_g_sideData + (ulonglong)param_1 * 0x1088 + 0xf58;
  g_baselineData = _refptr_g_sideData + (ulonglong)param_1 * 0x1088 + 0x10;
  SideTouch_BaselineGetFlag();
  SideTouch_BaselineStageUpdate();
  SideTouch_BaselineStageProcess();
  if ((*g_baselinePrpt == 0) || (*g_baselinePrpt == 1)) {
    uVar8 = 1;
  }
  else {
    uVar8 = 0;
  }
  g_baselineData[0x1068] = uVar8;
  if ((g_baselinePrpt[2] == 0) || (g_baselinePrpt[2] == 0x100)) {
    uVar8 = 1;
  }
  else {
    uVar8 = 0;
  }
  _refptr_g_sideData[(ulonglong)param_1 * 0x1088 + 0x1079] = uVar8;
  puVar7 = _refptr_g_tsaPrmtDynamic;
  *(undefined8 *)(_refptr_g_tsaPrmtDynamic + 0x42) = uVar2;
  *(undefined8 *)(puVar7 + 0x4a) = uVar3;
  *(undefined8 *)(puVar7 + 0x52) = uVar4;
  *(undefined8 *)(puVar7 + 0x5a) = uVar5;
  *(undefined8 *)(puVar7 + 0x62) = uVar6;
  *(undefined2 *)(puVar7 + 0x6a) = uVar1;
  return;
}


// ============================================================================
// Function: SideTouch_GetMinDifOffDown @ 6bb06667
// ============================================================================

short SideTouch_GetMinDifOffDown(char param_1,byte param_2)

{
  short local_e;
  uint local_c;
  
  local_c = (uint)(param_1 != '\0');
  local_e = *(short *)(&DAT_6bc97198 +
                      (longlong)(int)(uint)param_2 * 0x30 + (ulonglong)local_c * 0x1088);
  if ((param_2 != 0) &&
     (local_e < *(short *)(&DAT_6bc97198 +
                          (longlong)(int)(param_2 - 1) * 0x30 + (ulonglong)local_c * 0x1088))) {
    local_e = *(short *)(&DAT_6bc97198 +
                        (longlong)(int)(param_2 - 1) * 0x30 + (ulonglong)local_c * 0x1088);
  }
  if (((int)(uint)param_2 < (int)(_refptr_g_tsaPrmtFlash->bRows - 1)) &&
     (local_e < *(short *)(&DAT_6bc97198 +
                          (longlong)(int)(param_2 + 1) * 0x30 + (ulonglong)local_c * 0x1088))) {
    local_e = *(short *)(&DAT_6bc97198 +
                        (longlong)(int)(param_2 + 1) * 0x30 + (ulonglong)local_c * 0x1088);
  }
  return local_e;
}


// ============================================================================
// Function: SideTouch_GetAntiTriggerCnt @ 6bb067de
// ============================================================================

undefined1 SideTouch_GetAntiTriggerCnt(char param_1,byte param_2)

{
  undefined1 uVar1;
  
  if (param_1 == '\0') {
    uVar1 = (&g_sideData)[((longlong)(int)(uint)param_2 + 1) * 0x30];
  }
  else {
    uVar1 = (&DAT_6bc98238)[(longlong)(int)(uint)param_2 * 0x30];
  }
  return uVar1;
}


// ============================================================================
// Function: SideTouch_GetAge @ 6bb06845
// ============================================================================

int SideTouch_GetAge(char param_1,byte param_2)

{
  int iVar1;
  
  if (param_1 == '\0') {
    if (*(longlong *)(&DAT_6bc971a0 + (longlong)(int)(uint)param_2 * 0x30) == 0) {
      iVar1 = 0;
    }
    else {
      iVar1 = (int)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
              (int)*(undefined8 *)(&DAT_6bc971a0 + (longlong)(int)(uint)param_2 * 0x30);
    }
  }
  else if (*(longlong *)(&DAT_6bc98228 + (longlong)(int)(uint)param_2 * 0x30) == 0) {
    iVar1 = 0;
  }
  else {
    iVar1 = (int)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
            (int)*(undefined8 *)(&DAT_6bc98228 + (longlong)(int)(uint)param_2 * 0x30);
  }
  return iVar1;
}


// ============================================================================
// Function: SideTouch_AgeProcessUnit @ 6bb07c33
// ============================================================================

void SideTouch_AgeProcessUnit(byte param_1,uint param_2)

{
  byte local_9;
  
  for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bRows; local_9 = local_9 + 1) {
    if (*(char *)((ulonglong)
                  ((uint)param_1 + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) +
                 *(longlong *)_refptr_g_tsaBufTz) == '\0') {
      if (*(longlong *)
           (&DAT_6bc971a0 + (longlong)(int)(uint)local_9 * 0x30 + (ulonglong)param_2 * 0x1088) == 0)
      {
        if (*(short *)((ulonglong)
                       ((uint)param_1 + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff
                       ) * 2 + *(longlong *)_refptr_g_tsaBufDif) <
            *(short *)(&DAT_6bc97198 +
                      (longlong)(int)(uint)local_9 * 0x30 + (ulonglong)param_2 * 0x1088)) {
          *(undefined2 *)
           (&DAT_6bc97198 + (longlong)(int)(uint)local_9 * 0x30 + (ulonglong)param_2 * 0x1088) =
               *(undefined2 *)
                ((ulonglong)
                 ((uint)param_1 + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2
                + *(longlong *)_refptr_g_tsaBufDif);
        }
      }
      else {
        *(undefined8 *)
         (&DAT_6bc971a8 + (longlong)(int)(uint)local_9 * 0x30 + (ulonglong)param_2 * 0x1088) =
             *(undefined8 *)
              (&DAT_6bc971a0 + (longlong)(int)(uint)local_9 * 0x30 + (ulonglong)param_2 * 0x1088);
        *(undefined2 *)
         (&DAT_6bc97198 + (longlong)(int)(uint)local_9 * 0x30 + (ulonglong)param_2 * 0x1088) =
             *(undefined2 *)
              ((ulonglong)
               ((uint)param_1 + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
              *(longlong *)_refptr_g_tsaBufDif);
      }
      *(undefined8 *)
       (&DAT_6bc971a0 + (longlong)(int)(uint)local_9 * 0x30 + (ulonglong)param_2 * 0x1088) = 0;
    }
    else if (*(longlong *)
              (&DAT_6bc971a0 + (longlong)(int)(uint)local_9 * 0x30 + (ulonglong)param_2 * 0x1088) ==
             0) {
      *(void **)(&DAT_6bc971a0 + (longlong)(int)(uint)local_9 * 0x30 + (ulonglong)param_2 * 0x1088)
           = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    }
  }
  return;
}


// ============================================================================
// Function: SideTouch_XYCorrection @ 6bb0905f
// ============================================================================

/* WARNING: Removing unreachable block (ram,0x6bb092c2) */

void SideTouch_XYCorrection(byte param_1,byte param_2)

{
  ushort uVar1;
  ushort uVar2;
  ushort uVar3;
  int iVar4;
  
  if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 500) &
      0x40) != 0) {
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x188) =
         *(undefined2 *)
          ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x188);
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x18c) =
         *(undefined2 *)
          ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x18c);
    *(undefined2 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x228) =
         *(undefined2 *)
          ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x228);
  }
  uVar1 = *(ushort *)
           ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x18c);
  uVar2 = *(ushort *)
           ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x18c);
  if (*(ushort *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x156)
      < 4) {
    *(ushort *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 400) =
         uVar1;
    *(undefined2 *)
     ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4ae) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164);
  }
  else {
    uVar3 = *(ushort *)
             ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 400);
    iVar4 = (uint)uVar3 * 0x10 - (uint)uVar3;
    if (iVar4 < 0) {
      iVar4 = iVar4 + 0xf;
    }
    *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 400) =
         (short)(iVar4 >> 4) + (short)((int)(uint)uVar1 >> 4);
    if (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0
                   ) & 3) == 0) &&
       ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 500) &
        0x40) == 0)) {
      if ((-0x32 < (int)((uint)uVar1 - (uint)uVar2)) && ((int)((uint)uVar1 - (uint)uVar2) < 0x32)) {
        *(ushort *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4ae) =
             ((*(ushort *)
                ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 400)
              >> 1) - (*(ushort *)
                        ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        400) >> 1)) +
             *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_2 * 0x478 + 0x4ae);
      }
      *(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4ae)
           = (*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x164) -
             *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_2 * 0x478 + 0x164)) +
             *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_2 * 0x478 + 0x4ae);
    }
  }
  *(undefined2 *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172)
       = *(undefined2 *)
          ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4ae);
  return;
}


// ============================================================================
// Function: SideTouch_XYFilter @ 6bb09504
// ============================================================================

void SideTouch_XYFilter(byte param_1,byte param_2)

{
  int iVar1;
  
  *(undefined2 *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16e)
       = *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
  if (*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x156)
      != 0) {
    iVar1 = (uint)*(ushort *)
                   ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172
                   ) -
            (uint)*(ushort *)
                   ((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_2 * 0x478 + 0x16e);
    if (iVar1 < 0xb) {
      if (iVar1 < -10) {
        *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16e)
             = *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x16e) + 10;
      }
      else {
        *(undefined2 *)
         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16e) =
             *(undefined2 *)
              ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x16e);
      }
    }
    else {
      *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16e) =
           *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x16e) + -10;
    }
  }
  *(undefined2 *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172)
       = *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x16e);
  if ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x1f8)
      & 8) != 0) {
    if (*(ushort *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4fe) <
        *(ushort *)
         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172)) {
      if ((int)(uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x638 <=
          (int)((uint)*(ushort *)
                       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x172) -
               (uint)*(ushort *)
                      ((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_2 * 0x478 + 0x4fe))) {
        *(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x510
                 ) = *(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                              (longlong)(int)(uint)param_2 * 0x478 + 0x510) | 8;
        *(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x510
                 ) = *(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                              (longlong)(int)(uint)param_2 * 0x478 + 0x510) & 0xfffffffb;
        *(undefined2 *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4fa) =
             *(undefined2 *)
              ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4fe);
        *(undefined8 *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x500) =
             *(undefined8 *)
              ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x508);
      }
    }
    else if ((int)(uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x638 <=
             (int)((uint)*(ushort *)
                          ((longlong)*_refptr_g_prevTouchesPtr +
                          (longlong)(int)(uint)param_2 * 0x478 + 0x4fe) -
                  (uint)*(ushort *)
                         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x172))) {
      *(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x510)
           = *(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_2 * 0x478 + 0x510) & 0xfffffff7;
      *(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x510)
           = *(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_2 * 0x478 + 0x510) | 4;
      *(undefined2 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4fa) =
           *(undefined2 *)
            ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4fe);
      *(undefined8 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x500) =
           *(undefined8 *)
            ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x508);
    }
    if (((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_2 * 0x478 + 0x510) & 8) != 0) &&
       (*(ushort *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4fe) <
        *(ushort *)
         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172))) {
      *(undefined2 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4fe) =
           *(undefined2 *)
            ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
      *(void **)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x508)
           = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    }
    if (((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_2 * 0x478 + 0x510) & 4) != 0) &&
       (*(ushort *)
         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172) <
        *(ushort *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4fe))) {
      *(undefined2 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4fe) =
           *(undefined2 *)
            ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
      *(void **)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x508)
           = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    }
    TSAIDE_LogPoint(*(undefined2 *)
                     ((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)param_2 * 0x478 + 0x4fc),
                    *(undefined2 *)
                     ((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)param_2 * 0x478 + 0x4fe),0);
    TSAIDE_LogPoint(*(undefined2 *)
                     ((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)param_2 * 0x478 + 0x4f8),
                    *(undefined2 *)
                     ((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)param_2 * 0x478 + 0x4fa),0);
  }
  return;
}


// ============================================================================
// Function: SideTouch_GetPrevAge @ 6bb06942
// ============================================================================

int SideTouch_GetPrevAge(char param_1,byte param_2)

{
  int iVar1;
  
  if (param_1 == '\0') {
    if (*(longlong *)(&DAT_6bc971a8 + (longlong)(int)(uint)param_2 * 0x30) == 0) {
      iVar1 = 0;
    }
    else {
      iVar1 = (int)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
              (int)*(undefined8 *)(&DAT_6bc971a8 + (longlong)(int)(uint)param_2 * 0x30);
    }
  }
  else if (*(longlong *)(&DAT_6bc98230 + (longlong)(int)(uint)param_2 * 0x30) == 0) {
    iVar1 = 0;
  }
  else {
    iVar1 = (int)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
            (int)*(undefined8 *)(&DAT_6bc98230 + (longlong)(int)(uint)param_2 * 0x30);
  }
  return iVar1;
}


// ============================================================================
// Function: SideTouch_SHBGetStaticData @ 6bb1f3ea
// ============================================================================

undefined1 SideTouch_SHBGetStaticData(uint param_1,byte param_2)

{
                    /* 0xdf3ea  1404  SideTouch_SHBGetStaticData */
  return (&DAT_6bc9d981)[((ulonglong)param_1 * 0x50 + (longlong)(int)(uint)param_2) * 4];
}


// ============================================================================
// Function: SideTouch_SetGestureEnabled @ 6bb0e04a
// ============================================================================

void SideTouch_SetGestureEnabled(uint param_1)

{
  DAT_6bc9d7f0 = DAT_6bc9d7f0 | param_1;
  DAT_6bc9d7f4 = DAT_6bc9d7f0;
  return;
}


// ============================================================================
// Function: SideTouch_ClearGestureEnabled @ 6bb0e08c
// ============================================================================

void SideTouch_ClearGestureEnabled(uint param_1)

{
  DAT_6bc9d7f0 = ~param_1 & DAT_6bc9d7f0;
  DAT_6bc9d7f4 = DAT_6bc9d7f0;
  return;
}


// ============================================================================
// Function: SideTouch_GestureKeepExternal @ 6bb168a5
// ============================================================================

void SideTouch_GestureKeepExternal(void)

{
  char local_9;
  
  local_9 = g_sideGesture;
  while (local_9 = local_9 + -1, -1 < local_9) {
    if (*(int *)(&DAT_6bc997e0 + (longlong)(int)local_9 * 0x2a8) != 0x100000) {
      SideTouch_GestureRemove(local_9);
    }
  }
  return;
}


// ============================================================================
// Function: SideTouch_GestureAntiTriggerGlobally @ 6bb0baa2
// ============================================================================

void SideTouch_GestureAntiTriggerGlobally(void)

{
  byte bVar1;
  char cVar2;
  byte local_9;
  
                    /* 0xcbaa2  1344  SideTouch_GestureAntiTriggerGlobally */
  if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x400000) != 0) {
    prevNumTrulyReprotedTouchInCore_10612 = (*_refptr_g_tsaStaticPtr)->field_0x1;
  }
  bVar1 = SideTouch_GetNewNumInTime(300,1000,1);
  if ((2 < bVar1) || ((*_refptr_g_tsaStaticPtr)->field_0x1 != prevNumTrulyReprotedTouchInCore_10612)
     ) {
    timeStampAntiTrigger_10613 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  }
  if ((ulonglong)
      ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - (longlong)timeStampAntiTrigger_10613)
      < (ulonglong)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x674) {
    for (local_9 = 0; local_9 < (byte)*_refptr_g_sideGesture; local_9 = local_9 + 1) {
      cVar2 = SideGesture_IsVolumeTrigger(local_9);
      if (cVar2 != '\0') {
        SideTouch_GestureExcludeExistedTouch(local_9,0xd);
        SideTouch_GestureReset(local_9);
      }
    }
  }
  prevNumTrulyReprotedTouchInCore_10612 = (*_refptr_g_tsaStaticPtr)->field_0x1;
  SideTouch_AntiTriggerProcess();
  return;
}


// ============================================================================
// Function: SideTouch_IsGestureActive @ 6bb0eb74
// ============================================================================

undefined8 SideTouch_IsGestureActive(byte param_1)

{
  undefined8 uVar1;
  
                    /* 0xceb74  1372  SideTouch_IsGestureActive */
  if ((*(int *)(&DAT_6bc997c4 + (longlong)(int)(uint)param_1 * 0x2a8) == 1) ||
     (*(int *)(&DAT_6bc997c4 + (longlong)(int)(uint)param_1 * 0x2a8) == 2)) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: SideTouch_GestureVolumeAreaUpdate @ 6bb16a53
// ============================================================================

void SideTouch_GestureVolumeAreaUpdate(void)

{
  short sVar1;
  ushort uVar2;
  byte bVar3;
  byte bVar4;
  char cVar5;
  ushort uVar6;
  short sVar7;
  ushort local_1e;
  short local_1c;
  byte local_19;
  
  local_1c = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x644);
  local_1e = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x646);
  cVar5 = VirtualKey_VolumeIsGUIActive();
  if ((cVar5 != '\0') && ((DAT_6bc9d7f0 & 0x100000) == 0)) {
    SideGesture_GetVolumeGreenArea(&local_1c,&local_1e);
  }
  local_19 = 0;
  do {
    if (g_sideGesture <= local_19) {
      SideTouch_ShowVolumeArea((int)local_1c,(int)(short)local_1e,1);
      return;
    }
    cVar5 = SideGesture_IsTPSlideVolume(local_19);
    if (cVar5 == '\0') {
      cVar5 = SideGesture_IsVolumeMute(local_19);
      if (cVar5 != '\0') goto LAB_6bb16aef;
    }
    else {
LAB_6bb16aef:
      cVar5 = SideGesture_IsVolumeDoubleFlickTrigger(local_19);
      if ((cVar5 == '\0') || ((&DAT_6bc997c0)[(longlong)(int)(uint)local_19 * 0x2a8] != '\x01')) {
        cVar5 = SideGesture_IsVolumeDoubleClickTrigger(local_19);
        bVar3 = local_19;
        if ((cVar5 == '\0') || ((&DAT_6bc997c0)[(longlong)(int)(uint)local_19 * 0x2a8] != '\x02')) {
          cVar5 = SideGesture_IsVolumeMute(local_19);
          if ((cVar5 == '\0') ||
             (((&DAT_6bc997c0)[(longlong)(int)(uint)local_19 * 0x2a8] != '\x01' &&
              ((&DAT_6bc997c0)[(longlong)(int)(uint)local_19 * 0x2a8] != '\x04')))) {
            *(short *)(&DAT_6bc995c4 + (longlong)(int)(uint)local_19 * 0x2a8) = local_1c;
            *(ushort *)(&DAT_6bc995c6 + (longlong)(int)(uint)local_19 * 0x2a8) = local_1e;
          }
          else {
            *(undefined2 *)(&DAT_6bc995c4 + (longlong)(int)(uint)local_19 * 0x2a8) = 0;
            uVar2 = (*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x22) >> 2) * 3;
            uVar6 = local_1e;
            if (local_1e <= uVar2) {
              uVar6 = uVar2;
            }
            *(ushort *)(&DAT_6bc995c6 + (longlong)(int)(uint)local_19 * 0x2a8) = uVar6;
            SideTouch_ShowVolumeArea
                      ((int)*(short *)(&DAT_6bc995c4 + (longlong)(int)(uint)local_19 * 0x2a8),
                       (int)*(short *)(&DAT_6bc995c6 + (longlong)(int)(uint)local_19 * 0x2a8),2);
          }
        }
        else {
          sVar1 = *(short *)(&DAT_6bc995f2 + (longlong)(int)(uint)local_19 * 0x2a8);
          sVar7 = TS_MMToPixel(8);
          bVar4 = local_19;
          *(short *)(&DAT_6bc995c4 + (longlong)(int)(uint)bVar3 * 0x2a8) = sVar1 - sVar7;
          sVar1 = *(short *)(&DAT_6bc995f4 + (longlong)(int)(uint)local_19 * 0x2a8);
          sVar7 = TS_MMToPixel(8);
          *(short *)(&DAT_6bc995c6 + (longlong)(int)(uint)bVar4 * 0x2a8) = sVar7 + sVar1;
          SideTouch_ShowVolumeArea
                    ((int)*(short *)(&DAT_6bc995c4 + (longlong)(int)(uint)local_19 * 0x2a8),
                     (int)*(short *)(&DAT_6bc995c6 + (longlong)(int)(uint)local_19 * 0x2a8),2);
        }
      }
      else {
        *(undefined2 *)(&DAT_6bc995c4 + (longlong)(int)(uint)local_19 * 0x2a8) = 0;
        uVar2 = (*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x22) >> 2) * 3;
        uVar6 = local_1e;
        if (local_1e <= uVar2) {
          uVar6 = uVar2;
        }
        *(ushort *)(&DAT_6bc995c6 + (longlong)(int)(uint)local_19 * 0x2a8) = uVar6;
        SideTouch_ShowVolumeArea
                  ((int)*(short *)(&DAT_6bc995c4 + (longlong)(int)(uint)local_19 * 0x2a8),
                   (int)*(short *)(&DAT_6bc995c6 + (longlong)(int)(uint)local_19 * 0x2a8),2);
      }
    }
    local_19 = local_19 + 1;
  } while( true );
}


// ============================================================================
// Function: SideTouch_GestureProcessRegion @ 6bb15ec8
// ============================================================================

void SideTouch_GestureProcessRegion(byte param_1)

{
  undefined1 uVar1;
  char cVar2;
  int iVar3;
  byte local_c;
  char local_b;
  char local_a;
  char local_9;
  
  local_9 = '\0';
  local_a = '\0';
  local_b = '\0';
  (&DAT_6bc997d8)[(longlong)(int)(uint)param_1 * 0x2a8] = 0;
  (&DAT_6bc997d9)[(longlong)(int)(uint)param_1 * 0x2a8] = 0x14;
  for (local_c = 0; local_c < **(byte **)_refptr_g_touchesPtr; local_c = local_c + 1) {
    uVar1 = *(undefined1 *)
             ((longlong)(int)(uint)local_c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x287);
    cVar2 = SideTouch_IsTouchInRegion(local_c,param_1);
    if ((cVar2 != '\0') && (cVar2 = SideTouch_IsToExcludeTouch(local_c,param_1), cVar2 == '\0')) {
      local_b = local_b + '\x01';
      (&DAT_6bc997d8)[(longlong)(int)(uint)param_1 * 0x2a8] =
           (&DAT_6bc997d8)[(longlong)(int)(uint)param_1 * 0x2a8] + '\x01';
      (&DAT_6bc997d9)[(longlong)(int)(uint)param_1 * 0x2a8] = local_c;
      (&DAT_6bc997da)[(longlong)(int)(uint)param_1 * 0x2a8] =
           *(undefined1 *)
            ((longlong)(int)(uint)local_c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x154);
      *(void **)(&DAT_6bc997f0 + (longlong)(int)(uint)param_1 * 0x2a8) =
           (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
      iVar3 = SideTouch_IsActionDetected(local_c,uVar1,param_1);
      if (iVar3 == 0) {
        if ((byte)(&DAT_6bc997c0)[(longlong)(int)(uint)param_1 * 0x2a8] <
            (byte)(&DAT_6bc995d8)[(longlong)(int)(uint)param_1 * 0x2a8]) {
          (&DAT_6bc997c0)[(longlong)(int)(uint)param_1 * 0x2a8] =
               (&DAT_6bc997c0)[(longlong)(int)(uint)param_1 * 0x2a8] + '\x01';
        }
        if ((&DAT_6bc997c0)[(longlong)(int)(uint)param_1 * 0x2a8] ==
            (&DAT_6bc995d8)[(longlong)(int)(uint)param_1 * 0x2a8]) {
          local_9 = local_9 + '\x01';
        }
      }
      else if (iVar3 == 2) {
        local_a = local_a + '\x01';
      }
      *(undefined8 *)(&DAT_6bc997f8 + (longlong)(int)(uint)param_1 * 0x2a8) = 0;
    }
  }
  if (local_b == '\0') {
    if (*(longlong *)(&DAT_6bc997f8 + (longlong)(int)(uint)param_1 * 0x2a8) == 0) {
      *(void **)(&DAT_6bc997f8 + (longlong)(int)(uint)param_1 * 0x2a8) =
           (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    }
    iVar3 = SideTouch_IsActionDetected(0x14,0x14,param_1);
    if (iVar3 == 0) {
      if ((byte)(&DAT_6bc997c0)[(longlong)(int)(uint)param_1 * 0x2a8] <
          (byte)(&DAT_6bc995d8)[(longlong)(int)(uint)param_1 * 0x2a8]) {
        (&DAT_6bc997c0)[(longlong)(int)(uint)param_1 * 0x2a8] =
             (&DAT_6bc997c0)[(longlong)(int)(uint)param_1 * 0x2a8] + '\x01';
      }
      if ((&DAT_6bc997c0)[(longlong)(int)(uint)param_1 * 0x2a8] ==
          (&DAT_6bc995d8)[(longlong)(int)(uint)param_1 * 0x2a8]) {
        local_9 = local_9 + '\x01';
      }
    }
    else if (iVar3 == 2) {
      local_a = local_a + '\x01';
    }
  }
  SideTouch_GestureProcessState(param_1,local_9,local_a);
  return;
}


// ============================================================================
// Function: SideTouch_ExcludeAllUnstableTouch @ 6bb19bb8
// ============================================================================

void SideTouch_ExcludeAllUnstableTouch(void)

{
  byte bVar1;
  char cVar2;
  byte local_a;
  byte local_9;
  
  for (local_9 = 0; local_9 < g_sideGesture; local_9 = local_9 + 1) {
    cVar2 = SideGesture_IsVolumeDoubleClickTrigger(local_9);
    if (cVar2 != '\0') {
      for (local_a = 0; local_a < **(byte **)_refptr_g_touchesPtr; local_a = local_a + 1) {
        bVar1 = *(byte *)((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x287);
        if (((bVar1 < 0x14) &&
            ((*(uint *)((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       500) & 0x20) != 0)) &&
           (4 < *(ushort *)
                 ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x4e4))
           ) {
          *(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x588
                   ) = *(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                                (longlong)(int)(uint)bVar1 * 0x478 + 0x588) |
                       *(uint *)(&DAT_6bc997e0 + (longlong)(int)(uint)local_9 * 0x2a8);
          SideTouch_TouchFailureRecord(local_a,0xf,0);
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: SideTouch_IsVolumeTriggered @ 6bb1ae74
// ============================================================================

undefined8 SideTouch_IsVolumeTriggered(byte param_1,undefined1 param_2)

{
  char cVar1;
  undefined8 uVar2;
  
  if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) &
      0x4000000) == 0) {
    cVar1 = SideTouch_GetTriggeredGesture(param_1,param_2);
    if (cVar1 == '\x19') {
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
// Function: SideTouch_IsVolumeAntiTriggered @ 6bb1aedc
// ============================================================================

bool SideTouch_IsVolumeAntiTriggered(byte param_1)

{
  return (*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1fc
                   ) & 0x200) != 0;
}


// ============================================================================
// Function: SideTouch_IsToLog @ 6bb1af22
// ============================================================================

bool SideTouch_IsToLog(byte param_1)

{
  ushort uVar1;
  uint uVar2;
  int iVar3;
  uint uVar4;
  bool local_19;
  
  local_19 = (*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x21c) & 0x30) != 0 ||
             1000 < *(short *)((longlong)(int)(uint)param_1 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x294);
  uVar2 = (uint)*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x296);
  uVar4 = (int)uVar2 >> 0x1f;
  uVar1 = TSAOut_ExtToIntDist(*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x63c >> 1);
  if ((int)((uVar2 ^ uVar4) - uVar4) < (int)(uint)uVar1) {
    local_19 = false;
  }
  if (*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x294) <
      0x1e) {
    local_19 = false;
  }
  iVar3 = SideTouch_GetVolumeTriggerGestureTag();
  if (iVar3 == 0) {
    local_19 = false;
  }
  if ((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6a0 & 6) == 0) {
    local_19 = false;
  }
  return local_19;
}


// ============================================================================
// Function: SideTouch_TriggeredGestureLogPrint @ 6bb1b990
// ============================================================================

void SideTouch_TriggeredGestureLogPrint(undefined1 param_1,undefined1 param_2,undefined1 param_3)

{
  byte bVar1;
  undefined4 uVar2;
  
  bVar1 = SideTouch_GetTriggeredGesture(param_1,param_2);
  if (bVar1 < 0x19) {
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"[TimeOff]%d",
                        *(undefined4 *)
                         (_refptr_g_sideGesture + (longlong)(int)(uint)bVar1 * 0x2a8 + 0xac));
    }
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"[MaxSig]%d",
                        *(undefined4 *)
                         (_refptr_g_sideGesture + (longlong)(int)(uint)bVar1 * 0x2a8 + 0xd8));
    }
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      uVar2 = TSA_GetRatio(*(undefined4 *)
                            (_refptr_g_sideGesture + (longlong)(int)(uint)bVar1 * 0x2a8 + 0xd8),
                           *(undefined4 *)
                            (_refptr_g_sideGesture + (longlong)(int)(uint)bVar1 * 0x2a8 + 0x78),100)
      ;
      StrPrintf_Process(0,"[MaxSigRatio]%d",uVar2);
    }
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      uVar2 = TSA_GetRatio(*(undefined2 *)
                            (_refptr_g_sideGesture + (longlong)(int)(uint)bVar1 * 0x2a8 + 0xd6),
                           *(undefined2 *)
                            (_refptr_g_sideGesture + (longlong)(int)(uint)bVar1 * 0x2a8 + 0x76),100)
      ;
      StrPrintf_Process(0,"[MaxTouchRatio]%d",uVar2);
    }
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      uVar2 = TSA_GetRatio(*(undefined4 *)
                            (_refptr_g_sideGesture + (longlong)(int)(uint)bVar1 * 0x2a8 + 0xdc),
                           *(undefined4 *)
                            (_refptr_g_sideGesture + (longlong)(int)(uint)bVar1 * 0x2a8 + 0x7c),100)
      ;
      StrPrintf_Process(0,"[TouchTimeRatio]%d",uVar2);
    }
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"[AntiTriggerState]%d",
                        *(undefined4 *)
                         (_refptr_g_sideGesture + (longlong)(int)(uint)bVar1 * 0x2a8 + 0x24c));
    }
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"[IsRejected]%d",param_3);
    }
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"[TriggerSensitivity]%d",
                        *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x69c);
    }
  }
  else {
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"[TimeOff]%d",0);
    }
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"[MaxSig]%d",0);
    }
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"[MaxSigRatio]%d",0);
    }
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"[MaxTouchRatio]%d",0);
    }
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"[TouchTimeRatio]%d",0);
    }
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"[AntiTriggerState]%d",0);
    }
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"[IsRejected]%d",0);
    }
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"[TriggerSensitivity]%d",
                        *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x69c);
    }
  }
  return;
}


// ============================================================================
// Function: SideTouch_RingLogNameGenerate @ 6bb1bd6e
// ============================================================================

void SideTouch_RingLogNameGenerate(byte param_1,char param_2,char param_3)

{
  int iVar1;
  errno_t eVar2;
  uint uVar3;
  undefined8 uVar4;
  
  iVar1 = memset_s(_refptr_g_sideGesture + 0x42a4,0x32,0,0x32);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SideTouch/SideTouch_Log.c",200,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  eVar2 = strcpy_s(_refptr_g_sideGesture + 0x42a4,0x32,"TSA_RING");
  if (eVar2 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SideTouch/SideTouch_Log.c",0xca,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = SideTouch_GetVolumeTriggerGestureTag();
  if (iVar1 == 1) {
    eVar2 = strcat_s(_refptr_g_sideGesture + 0x42a4,0x32,"_OK");
    if (eVar2 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/SideTouch/SideTouch_Log.c",0xce,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
  }
  else if (iVar1 == -1) {
    eVar2 = strcat_s(_refptr_g_sideGesture + 0x42a4,0x32,"_NG");
    if (eVar2 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/SideTouch/SideTouch_Log.c",0xd1,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
  }
  else {
    eVar2 = strcat_s(_refptr_g_sideGesture + 0x42a4,0x32,"_NA");
    if (eVar2 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/SideTouch/SideTouch_Log.c",0xd4,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
  }
  eVar2 = strcat_s(_refptr_g_sideGesture + 0x42a4,0x32,"_VOLUME_SLIDER");
  if (eVar2 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SideTouch/SideTouch_Log.c",0xd7,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  if ((param_2 == '\0') || (param_3 != '\0')) {
    eVar2 = strcat_s(_refptr_g_sideGesture + 0x42a4,0x32,"_NG");
    if (eVar2 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/SideTouch/SideTouch_Log.c",0xda,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
    uVar3 = *(int *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_1 * 0x478 + 0x548) - 1;
    if (((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6a0 & 4) == 0) ||
       ((*(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) & 0x30000) == 0)) {
      if (uVar3 < 0xf) {
        if (*_refptr_g_tsaLogLoglevel != '\0') {
          uVar4 = SideGesture_GetFailureString
                            (*(undefined4 *)
                              ((longlong)*_refptr_g_prevTouchesPtr +
                              (longlong)(int)(uint)param_1 * 0x478 + 0x548));
          StrPrintf_Process(0,"[FailItem]%s",uVar4);
        }
        if (*_refptr_g_tsaLogLoglevel != '\0') {
          StrPrintf_Process(0,"[FailValue]%d",
                            *(undefined4 *)
                             ((longlong)*_refptr_g_prevTouchesPtr +
                             ((ulonglong)uVar3 + (longlong)(int)(uint)param_1 * 0x11e + 0x150) * 4 +
                             0xc));
        }
      }
      else {
        (*_refptr_g_tsaStaticPtr)->nField_0x298 =
             (*_refptr_g_tsaStaticPtr)->nField_0x298 & 0xfffffffd;
        if (*_refptr_g_tsaLogLoglevel != '\0') {
          StrPrintf_Process(0,"\n[PrevTouchPrpt(preIdx, failureInfo.failedItem)]%d",
                            *(undefined4 *)
                             ((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)param_1 * 0x478 + 0x548));
        }
      }
    }
    else {
      (*_refptr_g_tsaStaticPtr)->nField_0x298 = (*_refptr_g_tsaStaticPtr)->nField_0x298 & 0xfffffffd
      ;
    }
  }
  else {
    eVar2 = strcat_s(_refptr_g_sideGesture + 0x42a4,0x32,"_OK");
    if (eVar2 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/SideTouch/SideTouch_Log.c",0xeb,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      uVar4 = SideGesture_GetFailureString(0);
      StrPrintf_Process(0,"[FailItem]%s",uVar4);
    }
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"[FailValue]%d",0);
    }
  }
  return;
}


// ============================================================================
// Function: SideTouch_RingPrptPrint @ 6bb1c25f
// ============================================================================

void SideTouch_RingPrptPrint(byte param_1,byte param_2)

{
  undefined1 uVar1;
  undefined1 uVar2;
  undefined2 uVar3;
  undefined4 uVar4;
  undefined8 uVar5;
  
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"\nRingPrpt:");
  }
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    uVar1 = TSA_GetPatchVersion();
    uVar3 = TSA_GetMinorVersion();
    uVar2 = TSA_GetMajorVersion();
    uVar5 = TSA_GetBranchName();
    StrPrintf_Process(0,"[Version]%s_V%d.%d.%d",uVar5,uVar2,uVar3,uVar1);
  }
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"[TimeStamp]%ld",
                      *(undefined8 *)
                       ((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_2 * 0x478 + 0x4b8));
  }
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"[Task]%s",_refptr_g_sideGesture + 0x42d6);
  }
  if ((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6a0 & 2) == 0) {
    if ((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6a0 & 4) == 0) {
      if ((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6a0 & 0x800) == 0) {
        if (*_refptr_g_tsaLogLoglevel != '\0') {
          StrPrintf_Process(0,"[Gesture]VolumeUnknown");
        }
      }
      else if (*_refptr_g_tsaLogLoglevel != '\0') {
        StrPrintf_Process(0,"[Gesture]VolumeDoubleClick");
      }
    }
    else if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"[Gesture]VolumeDoubleFlick");
    }
  }
  else if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"[Gesture]VolumeSlider");
  }
  if ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 1) {
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"[ScreenState]ScreenLeft");
    }
  }
  else if ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 2) {
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"[ScreenState]ScreenDown");
    }
  }
  else if ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 3) {
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"[ScreenState]ScreenRight");
    }
  }
  else if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"[ScreenState]ScreenUp");
  }
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    uVar4 = TS_PixelToMMScale10(*(undefined2 *)
                                 ((longlong)*_refptr_g_prevTouchesPtr +
                                 (longlong)(int)(uint)param_2 * 0x478 + 0x528));
    StrPrintf_Process(0,"[MinTouch]%d",uVar4);
  }
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    uVar4 = TS_PixelToMMScale10(*(undefined2 *)
                                 ((longlong)*_refptr_g_prevTouchesPtr +
                                 (longlong)(int)(uint)param_2 * 0x478 + 0x52a));
    StrPrintf_Process(0,"[MaxTouch]%d",uVar4);
  }
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    uVar3 = TSAOut_Pixel2DisplayDim1
                      (*(undefined2 *)
                        ((longlong)*_refptr_g_prevTouchesPtr +
                        (longlong)(int)(uint)param_2 * 0x478 + 0x4b0));
    StrPrintf_Process(0,"[StartXPos]%d",uVar3);
  }
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"[MinXPos]%d",
                      *(undefined2 *)
                       ((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_2 * 0x478 + 0x52c));
  }
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"[MaxXPos]%d",
                      *(undefined2 *)
                       ((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_2 * 0x478 + 0x52e));
  }
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    uVar3 = TSAOut_Pixel2DisplayDim2
                      (*(undefined2 *)
                        ((longlong)*_refptr_g_prevTouchesPtr +
                        (longlong)(int)(uint)param_2 * 0x478 + 0x4b2));
    StrPrintf_Process(0,"[StartYPos]%d",uVar3);
  }
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"[MinYPos]%d",
                      *(undefined2 *)
                       ((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_2 * 0x478 + 0x530));
  }
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"[MaxYPos]%d",
                      *(undefined2 *)
                       ((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_2 * 0x478 + 0x532));
  }
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"[MinSideRatio]%d",
                      *(undefined2 *)
                       ((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_2 * 0x478 + 0x534));
  }
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"[MaxSideRatio]%d",
                      *(undefined2 *)
                       ((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_2 * 0x478 + 0x536));
  }
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    uVar4 = TS_PixelToMMScale10((int)*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                                               (longlong)(int)(uint)param_2 * 0x478 + 0x53a));
    StrPrintf_Process(0,"[MoveInTime]%d",uVar4);
  }
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"[TimeToMove]%d",
                      *(undefined2 *)
                       ((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_2 * 0x478 + 0x538));
  }
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    uVar4 = TS_PixelToMMScale10((int)*(short *)((longlong)(int)(uint)param_1 * 0x168 +
                                                *(longlong *)_refptr_g_touchesPtr + 0x296));
    StrPrintf_Process(0,"[Move]%d",uVar4);
  }
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"[Time]%d",
                      (int)*(short *)((longlong)(int)(uint)param_1 * 0x168 +
                                      *(longlong *)_refptr_g_touchesPtr + 0x294));
  }
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"[State]%d",*(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x27c);
  }
  return;
}


// ============================================================================
// Function: SideTouch_ToBeReported @ 6bb1e957
// ============================================================================

bool SideTouch_ToBeReported(byte param_1,byte param_2)

{
  char cVar1;
  bool local_9;
  
  cVar1 = SideTouch_IsPartOfCoreTouch(param_1);
  if (cVar1 != '\0') {
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
         | 0x2000000;
    *(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x1f8) =
         *(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_2 * 0x478 + 0x1f8) | 0x2000000;
  }
  local_9 = (*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x1f8) & 0x2000000) == 0;
  if (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 500) &
       1) != 0) &&
     (*(int *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c) !=
      0x10)) {
    local_9 = false;
  }
  if (((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x1f8
                 ) & 8) == 0) &&
     (((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x1f8
                 ) & 0x40000) == 0 || (cVar1 = Touch_IsException(param_1,param_2), cVar1 != '\0'))))
  {
    if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208)
        & 2) != 0) {
      local_9 = false;
    }
    if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208)
        & 4) != 0) {
      local_9 = false;
    }
  }
  if (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208)
       & 8) != 0) && (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x1000) != 0)) {
    local_9 = false;
  }
  if (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208)
       & 0x20) != 0) && (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x2000) != 0)) {
    local_9 = false;
  }
  if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c) &
      0x21) != 0) {
    local_9 = false;
  }
  return local_9;
}


// ============================================================================
// Function: SideTouch_IsVolumeControlEnabled @ 6bb0d810
// ============================================================================

undefined8 SideTouch_IsVolumeControlEnabled(void)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0xcd810  1384  SideTouch_IsVolumeControlEnabled */
  cVar1 = SideGesture_IsVolumeMapping();
  if (cVar1 == '\0') {
    if ((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6a0 & 0x80) == 0) {
      uVar2 = 1;
    }
    else {
      uVar2 = 0;
    }
  }
  else if (*_refptr_g_guiVolume == '\0') {
    if ((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6a0 & 0x80) == 0) {
      uVar2 = 1;
    }
    else {
      uVar2 = 0;
    }
  }
  else {
    uVar2 = 0;
  }
  return uVar2;
}


// ============================================================================
// Function: SideTouch_IsVolumeGUIShown @ 6bb1a0f9
// ============================================================================

undefined8 SideTouch_IsVolumeGUIShown(void)

{
  undefined8 uVar1;
  
                    /* 0xda0f9  1389  SideTouch_IsVolumeGUIShown */
  if ((*(longlong *)(_refptr_g_guiVolume + 0x20) == -1) ||
     (*(ulonglong *)(_refptr_g_guiVolume + 0x20) <= DAT_6bc9d818)) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: SideTouch_IsPrevTouchDetected @ 6bb08b39
// ============================================================================

undefined1 SideTouch_IsPrevTouchDetected(void)

{
  ushort local_c;
  undefined1 local_9;
  
                    /* 0xc8b39  1379  SideTouch_IsPrevTouchDetected */
  *(undefined1 **)_refptr_g_touchesPtr = &g_touchesSide;
  *_refptr_g_prevTouchesPtr = (PrevTouchArray *)&g_prevTouchesSide;
  local_9 = 0;
  local_c = 0;
  do {
    if ((*_refptr_g_prevTouchesPtr)->bPrevTouchCount <= local_c) {
LAB_6bb08bc0:
      *(undefined **)_refptr_g_touchesPtr = _refptr_g_touchesGrid;
      *_refptr_g_prevTouchesPtr = (PrevTouchArray *)_refptr_g_prevTouchesGrid;
      return local_9;
    }
    if ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)local_c * 0x478 + 0x1f8) & 8) != 0) {
      local_9 = 1;
      goto LAB_6bb08bc0;
    }
    local_c = local_c + 1;
  } while( true );
}


// ============================================================================
// Function: SideTouch_IsJustOKToEnterIDLEMode @ 6bb06533
// ============================================================================

undefined4 SideTouch_IsJustOKToEnterIDLEMode(void)

{
                    /* 0xc6533  1375  SideTouch_IsJustOKToEnterIDLEMode */
  return CONCAT31((int3)((uint)DAT_6bc97188 >> 8),DAT_6bc97188 == 0x1d);
}


// ============================================================================
// Function: SideTouch_GestureSetUp @ 6bb144e0
// ============================================================================

void SideTouch_GestureSetUp(uint param_1)

{
  int iVar1;
  
  iVar1 = memset_s(_refptr_g_guiVolume,0x28,0,0x28);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SideTouch/SideTouch_Gesture.c",0x60e,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  DAT_6bc9d818 = 0xffffffffffffffff;
  DAT_6bc9d810 = 0xffffffffffffffff;
  *(undefined8 *)(_refptr_g_guiVolume + 0x20) = 0xffffffffffffffff;
  DAT_6bc9d7f0 = 0;
  if ((param_1 & 0x20) != 0) {
    SideTouch_GestureVolumeForce(8);
  }
  if ((param_1 & 1) != 0) {
    SideTouch_TriggerMoveGestureSetUp();
  }
  if ((param_1 & 2) != 0) {
    SideTouch_TriggerFlickGestureSetUp();
  }
  if ((param_1 & 0x200) != 0) {
    SideTouch_GestureVolumeTriggerForceFlick(8);
  }
  if ((param_1 & 4) != 0) {
    SideTouch_TriggerDoubleFlickGestureSetUp();
  }
  if ((param_1 & 8) != 0) {
    SideTouch_OffMoveGestureSetUp();
  }
  if ((param_1 & 0x100) != 0) {
    SideTouch_MuteGestureSetUp();
  }
  if ((param_1 & 0x810) != 0) {
    SideTouch_DoubleClickGestureSetUp(param_1);
  }
  if ((param_1 & 0x1000) != 0) {
    SideTouch_TriggerClickFlickGestureSetUp();
  }
  if ((param_1 & 2) != 0) {
    SideTouch_TouchGestureSetUp();
  }
  return;
}


// ============================================================================
// Function: SideTouch_BaselineStageUpdate @ 6bb085c3
// ============================================================================

void SideTouch_BaselineStageUpdate(longlong param_1,uint param_2)

{
  undefined2 uVar1;
  char cVar2;
  short sVar3;
  undefined2 uVar4;
  int iVar5;
  uint uVar6;
  byte local_19;
  
  *(undefined4 *)(param_1 + 0x106c) = 0;
  *(undefined1 *)(param_1 + 0x1068) = 0;
  cVar2 = SideTouch_IsToReset();
  if (cVar2 == '\0') {
    SideTouch_BaselineStagePrptUpdate(param_1);
    if (_refptr_g_tsaPrmtDynamic[0x30] == '\0') {
      iVar5 = (&DAT_6bc98210)[(ulonglong)param_2 * 0x422] -
              (&DAT_6bc9820c)[(ulonglong)param_2 * 0x422];
    }
    else {
      iVar5 = (&DAT_6bc9820c)[(ulonglong)param_2 * 0x422] -
              (&DAT_6bc98210)[(ulonglong)param_2 * 0x422];
    }
    cVar2 = BLSM_IsReset();
    if (cVar2 == '\0') {
      if ((((_refptr_g_tsaPrpt->dwShbFlags & 2) == 0) ||
          (uVar6 = (int)*(short *)(param_1 + 0x1058) >> 0x1f,
          (int)(((int)*(short *)(param_1 + 0x1058) ^ uVar6) - uVar6) <=
          (int)*(short *)&_refptr_g_tsaPrmtFlash[2].field_0x632)) ||
         ((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x624 & 0x40) == 0)) {
        if ((iVar5 < -0x5dc) && (_refptr_g_tsaPrpt->dwBaselineStage == 0)) {
          *(undefined4 *)(param_1 + 0x106c) = 1;
        }
        else {
          for (local_19 = 0; local_19 < _refptr_g_tsaPrmtFlash->bRows; local_19 = local_19 + 1) {
            uVar4 = *(undefined2 *)((longlong)(int)(uint)local_19 * 0x30 + param_1);
            uVar1 = *(undefined2 *)((longlong)(int)(uint)local_19 * 0x30 + param_1 + 2);
            sVar3 = SideTouch_BLStageGetMaxDif
                              ((int)*(short *)((longlong)(int)(uint)local_19 * 0x30 + param_1 + 4),
                               local_19,param_1,param_2);
            if ((sVar3 < *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x632) &&
               (_refptr_g_tsaPrpt->dwBaselineStage == 0)) {
              if (0x1d < *(int *)(param_1 + 0x1074)) {
                uVar4 = IIR_Process2(uVar4,uVar1,1,4);
                *(undefined2 *)((longlong)(int)(uint)local_19 * 0x30 + param_1 + 2) = uVar4;
              }
            }
            else {
              *(undefined4 *)(param_1 + 0x106c) = 3;
            }
          }
        }
      }
      else {
        SideTouch_RawCaptureApply();
        DAT_6bc97181 = 1;
        DAT_6bc97182 = *(undefined2 *)(param_1 + 0x1058);
        *(undefined4 *)(param_1 + 0x106c) = 1;
        *(undefined4 *)(param_1 + 0x1070) = 0x14;
      }
    }
    else {
      *(undefined4 *)(param_1 + 0x106c) = 1;
      *(undefined4 *)(param_1 + 0x1070) = 0x14;
    }
  }
  else {
    *(undefined2 *)(param_1 + 0x1058) = 0;
    *(undefined4 *)(param_1 + 0x106c) = 1;
    *(undefined4 *)(param_1 + 0x1070) = 0x14;
  }
  return;
}


// ============================================================================
// Function: SideTouch_UpdateForRawUnstableSide @ 6bb08bec
// ============================================================================

void SideTouch_UpdateForRawUnstableSide(uint param_1)

{
  byte local_b;
  ushort local_a;
  
  if ((*(uint *)&(*_refptr_g_tsaStaticPtr)->field_0xd8 & 0x40000) != 0) {
    for (local_b = 0; local_b < _refptr_g_tsaPrmtFlash->bRows; local_b = local_b + 1) {
      local_a = *(ushort *)
                 (&DAT_6bc97192 + (longlong)(int)(uint)local_b * 0x30 + (ulonglong)param_1 * 0x1088)
      ;
      if ((int)((uint)*(ushort *)
                       (&DAT_6bc97190 +
                       (longlong)(int)(uint)local_b * 0x30 + (ulonglong)param_1 * 0x1088) -
               (uint)local_a) < 0x11) {
        if ((int)((uint)*(ushort *)
                         (&DAT_6bc97190 +
                         (longlong)(int)(uint)local_b * 0x30 + (ulonglong)param_1 * 0x1088) -
                 (uint)local_a) < -0x10) {
          local_a = local_a - 0x11;
        }
      }
      else {
        local_a = local_a + 0x11;
      }
      *(ushort *)(&DAT_6bc97192 + (longlong)(int)(uint)local_b * 0x30 + (ulonglong)param_1 * 0x1088)
           = local_a;
    }
  }
  return;
}


// ============================================================================
// Function: SideTouch_BaselineStageUpdate @ 6bb0bea3
// ============================================================================

void SideTouch_BaselineStageUpdate(void)

{
  undefined4 local_c;
  
  g_baselinePrpt[1] = *g_baselinePrpt;
  if ((g_baselinePrpt[2] & 0x10) == 0) {
    if ((g_baselinePrpt[2] & 0x200) == 0) {
      if ((g_baselinePrpt[2] & 0x100) == 0) {
        if ((g_baselinePrpt[2] & 0x80) == 0) {
          if ((g_baselinePrpt[2] & 0x40) == 0) {
            local_c = *g_baselinePrpt;
            *(short *)(g_baselinePrpt + 4) = *(short *)(g_baselinePrpt + 4) + 1;
            *(longlong *)(g_baselinePrpt + 6) =
                 (longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr +
                 (*(longlong *)(g_baselinePrpt + 6) - (*_refptr_g_tsaStaticPtr)->llField_0xe8);
            switch(local_c) {
            case 0:
              if ((g_baselinePrpt[2] & 2) == 0) {
                if ((g_baselinePrpt[2] & 1) != 0) {
                  *(undefined2 *)(g_baselinePrpt + 4) = 1;
                  *(longlong *)(g_baselinePrpt + 6) =
                       (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
                  local_c = 4;
                }
              }
              else {
                *(undefined2 *)(g_baselinePrpt + 4) = 1;
                *(longlong *)(g_baselinePrpt + 6) =
                     (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
                local_c = 5;
              }
              break;
            case 1:
            case 2:
              *(undefined2 *)(g_baselinePrpt + 4) = 1;
              *(longlong *)(g_baselinePrpt + 6) =
                   (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
              local_c = 0;
              break;
            case 3:
              *(undefined2 *)(g_baselinePrpt + 4) = 1;
              *(longlong *)(g_baselinePrpt + 6) =
                   (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
              local_c = 0;
              break;
            case 4:
              if ((g_baselinePrpt[2] & 2) == 0) {
                if ((ulonglong)*(ushort *)(_refptr_g_tsaPrmtDynamic + 0x54) <=
                    *(ulonglong *)(g_baselinePrpt + 6)) {
                  *(undefined2 *)(g_baselinePrpt + 4) = 1;
                  *(longlong *)(g_baselinePrpt + 6) =
                       (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
                  local_c = 0;
                }
              }
              else {
                *(undefined2 *)(g_baselinePrpt + 4) = 1;
                *(longlong *)(g_baselinePrpt + 6) =
                     (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
                local_c = 5;
              }
              break;
            case 5:
              if ((g_baselinePrpt[2] & 2) == 0) {
                if (((*_refptr_g_prevTouchesPtr)->bPrevTouchCount == '\x01') &&
                   ((*(uint *)(*_refptr_g_prevTouchesPtr + 0xe) & 0x200) == 0)) {
                  *(undefined2 *)(g_baselinePrpt + 4) = 1;
                  *(longlong *)(g_baselinePrpt + 6) =
                       (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
                  local_c = 4;
                }
                else {
                  *(undefined2 *)(g_baselinePrpt + 4) = 1;
                  *(longlong *)(g_baselinePrpt + 6) =
                       (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
                  local_c = 6;
                }
              }
              break;
            case 6:
              if ((g_baselinePrpt[2] & 2) == 0) {
                if ((ulonglong)*(ushort *)(_refptr_g_tsaPrmtDynamic + 0x54) <=
                    *(ulonglong *)(g_baselinePrpt + 6)) {
                  *(undefined2 *)(g_baselinePrpt + 4) = 1;
                  *(longlong *)(g_baselinePrpt + 6) =
                       (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
                  local_c = 7;
                }
              }
              else {
                *(undefined2 *)(g_baselinePrpt + 4) = 1;
                *(longlong *)(g_baselinePrpt + 6) =
                     (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
                local_c = 5;
              }
              break;
            case 7:
              if ((g_baselinePrpt[2] & 2) == 0) {
                if ((ulonglong)*(ushort *)(_refptr_g_tsaPrmtDynamic + 0x54) <=
                    *(ulonglong *)(g_baselinePrpt + 6)) {
                  *(undefined2 *)(g_baselinePrpt + 4) = 1;
                  *(longlong *)(g_baselinePrpt + 6) =
                       (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
                  local_c = 0;
                }
              }
              else {
                *(undefined2 *)(g_baselinePrpt + 4) = 1;
                *(longlong *)(g_baselinePrpt + 6) =
                     (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
                local_c = 5;
              }
              break;
            case 8:
              if ((g_baselinePrpt[2] & 2) == 0) {
                if ((ulonglong)*(ushort *)(_refptr_g_tsaPrmtDynamic + 0x54) <=
                    *(ulonglong *)(g_baselinePrpt + 6)) {
                  *(undefined2 *)(g_baselinePrpt + 4) = 1;
                  *(longlong *)(g_baselinePrpt + 6) =
                       (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
                  local_c = 0;
                }
              }
              else {
                *(undefined2 *)(g_baselinePrpt + 4) = 1;
                *(longlong *)(g_baselinePrpt + 6) =
                     (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
                local_c = 5;
              }
              break;
            case 9:
              if ((g_baselinePrpt[2] & 2) == 0) {
                if ((ulonglong)*(ushort *)(_refptr_g_tsaPrmtDynamic + 0x54) <=
                    *(ulonglong *)(g_baselinePrpt + 6)) {
                  *(undefined2 *)(g_baselinePrpt + 4) = 1;
                  *(longlong *)(g_baselinePrpt + 6) =
                       (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
                  local_c = 0;
                }
              }
              else {
                *(undefined2 *)(g_baselinePrpt + 4) = 1;
                *(longlong *)(g_baselinePrpt + 6) =
                     (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
                local_c = 5;
              }
              break;
            default:
              *(undefined2 *)(g_baselinePrpt + 4) = 1;
              *(longlong *)(g_baselinePrpt + 6) =
                   (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
              *g_baselinePrpt = 0;
            }
            *g_baselinePrpt = local_c;
          }
          else {
            *g_baselinePrpt = 3;
          }
        }
        else {
          *g_baselinePrpt = 9;
        }
      }
      else {
        *g_baselinePrpt = 3;
      }
    }
    else {
      *g_baselinePrpt = 2;
    }
  }
  else {
    *g_baselinePrpt = 1;
  }
  return;
}


// ============================================================================
// Function: SideTouch_BaselineStageProcess @ 6bb0c5fe
// ============================================================================

void SideTouch_BaselineStageProcess(void)

{
  short sVar1;
  short sVar2;
  int iVar3;
  uint uVar4;
  ushort local_a;
  
  *(undefined1 *)(g_baselineStatic + 0x103) = 0;
  *(undefined1 *)(g_baselineStatic + 0x106) = 0;
  switch(*g_baselinePrpt) {
  case 0:
    if ((*_refptr_g_tsaStaticPtr)->field_0x3 == '\0') {
      local_a = (ushort)(byte)_refptr_g_tsaPrmtDynamic[0x4f];
    }
    else {
      if ((int)*(short *)(g_baselineData + 0x1058) < -(int)*(short *)(g_baselineData + 0x105a)) {
        iVar3 = -(int)*(short *)(g_baselineData + 0x105a);
        if (iVar3 < 0) {
          iVar3 = iVar3 + 0xf;
        }
        local_a = (ushort)(iVar3 >> 4);
      }
      else {
        sVar1 = *(short *)(g_baselineData + 0x1058);
        if (sVar1 < 0) {
          sVar1 = sVar1 + 0xf;
        }
        local_a = sVar1 >> 4;
      }
      sVar1 = *(short *)(_refptr_g_tsaPrmtDynamic + 0x46);
      if (sVar1 < 0) {
        sVar1 = sVar1 + 3;
      }
      if (sVar1 >> 2 <= (short)local_a) {
        local_a = sVar1 >> 2;
      }
    }
    SideTouch_BaselineUpdate(1,(int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x46),(int)(short)local_a)
    ;
    *(undefined1 *)(g_baselineStatic + 0x103) = 1;
    break;
  case 1:
  case 2:
    SideTouch_BaselineDoReset();
    break;
  case 3:
    if (((g_baselinePrpt[1] == 1) || (g_baselinePrpt[1] == 2)) || (g_baselinePrpt[1] == 3)) {
      *g_baselinePrpt = 0;
    }
    else {
      *g_baselinePrpt = g_baselinePrpt[1];
    }
    break;
  case 7:
  case 8:
    SideTouch_BaselineUpdate
              (1,(int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x4a),_refptr_g_tsaPrmtDynamic[0x52]);
    *(undefined1 *)(g_baselineStatic + 0x103) = 1;
    break;
  case 9:
    uVar4 = (int)*(short *)(g_baselineData + 0x105a) >> 0x1f;
    iVar3 = ((int)*(short *)(g_baselineData + 0x105a) ^ uVar4) - uVar4;
    if (iVar3 < 0) {
      iVar3 = iVar3 + 0xf;
    }
    sVar2 = (short)(iVar3 >> 4);
    sVar1 = *(short *)(_refptr_g_tsaPrmtDynamic + 0x46);
    if (sVar1 < 0) {
      sVar1 = sVar1 + 3;
    }
    if (sVar1 >> 2 <= sVar2) {
      sVar2 = sVar1 >> 2;
    }
    SideTouch_BaselineUpdate(1,(int)-*(short *)(g_baselineData + 0x105a),(int)sVar2);
    *(undefined1 *)(g_baselineStatic + 0x103) = 1;
  }
  return;
}


// ============================================================================
// Function: SideTouch_BaselineGetFlag @ 6bb0cc97
// ============================================================================

void SideTouch_BaselineGetFlag(void)

{
  char cVar1;
  undefined4 local_c;
  
  local_c = 0;
  *(undefined4 *)(g_baselinePrpt + 0x2c) = *(undefined4 *)(g_baselinePrpt + 0x28);
  *(undefined4 *)(g_baselinePrpt + 0x28) = 0;
  SideTouch_BaselineGetPrpt();
  cVar1 = SideTouch_BaselineIsToResetGlobally();
  if (cVar1 != '\0') {
    local_c = 0x10;
    goto LAB_6bb0cda0;
  }
  if ((*_refptr_g_tsaStaticPtr)[1].field_0x2 != '\0') {
    local_c = 0x40;
    goto LAB_6bb0cda0;
  }
  cVar1 = SideTouch_BaselineIsToResetLocally();
  if (cVar1 == '\0') {
    cVar1 = AFE_IsRawJump();
    if (cVar1 == '\0') {
      cVar1 = BLSM_IsReset();
      if (cVar1 != '\0') {
        cVar1 = SideTouch_IsToSyncResetWithCoreArea();
        if (cVar1 != '\0') goto LAB_6bb0cd2a;
      }
      cVar1 = SideTouch_BaselineResetInDebounce();
      if (cVar1 == '\0') {
        if (*(short *)(g_baselineData + 0x1058) < *(short *)(_refptr_g_tsaPrmtDynamic + 0x42)) {
          cVar1 = BLSM_IsBaselineOK();
          if (cVar1 != '\0') {
            cVar1 = AFE_IsTouchDetected();
            if (cVar1 == '\0') {
              if (*(short *)(_refptr_g_tsaPrmtDynamic + 0x46) <= *(short *)(g_baselineData + 0x1058)
                 ) {
                local_c = 1;
              }
              goto LAB_6bb0cda0;
            }
          }
        }
        local_c = 2;
      }
      else {
        local_c = 0x100;
      }
      goto LAB_6bb0cda0;
    }
  }
LAB_6bb0cd2a:
  local_c = 0x10;
LAB_6bb0cda0:
  *(undefined4 *)(g_baselinePrpt + 8) = local_c;
  return;
}


// ============================================================================
// Function: SideTouch_GestureRemove @ 6bb0e0ee
// ============================================================================

void SideTouch_GestureRemove(byte param_1)

{
  errno_t eVar1;
  byte local_9;
  
  if (param_1 < g_sideGesture) {
    for (local_9 = param_1; (int)(uint)local_9 < (int)(g_sideGesture - 1); local_9 = local_9 + 1) {
      eVar1 = memcpy_s((void *)((longlong)(int)(uint)local_9 * 0x2a8 + 0x6bc99588),0x2a8,
                       (void *)((longlong)(int)(local_9 + 1) * 0x2a8 + 0x6bc99588),0x2a8);
      if (eVar1 != 0) {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                          "../../src/Extension/SideTouch/SideTouch_Gesture.c",0xc2,"rc == EOK");
        HardwareAnalyzer_SetSoftException();
      }
    }
    g_sideGesture = g_sideGesture - 1;
  }
  return;
}


// ============================================================================
// Function: SideTouch_GetNewNumInTime @ 6bb06a43
// ============================================================================

char SideTouch_GetNewNumInTime(int param_1,int param_2,char param_3)

{
  char cVar1;
  int iVar2;
  uint uVar3;
  uint local_14;
  int local_10;
  char local_a;
  byte local_9;
  
                    /* 0xc6a43  1357  SideTouch_GetNewNumInTime */
  local_a = '\0';
  local_10 = 0x7fffffff;
  local_14 = 0;
  for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
    cVar1 = Peak_IsTouchValid(local_9);
    if (((cVar1 != '\0') &&
        (iVar2 = SideTouch_GetAge((*_refptr_g_peaksPtr)
                                  [(longlong)
                                   (int)(uint)*(byte *)((longlong)(int)(uint)local_9 * 0x168 +
                                                        *(longlong *)_refptr_g_touchesPtr + 0x23c) *
                                   0x14 + 0x15].bPeakCount,
                                  (*_refptr_g_peaksPtr)
                                  [(longlong)
                                   (int)(uint)*(byte *)((longlong)(int)(uint)local_9 * 0x168 +
                                                        *(longlong *)_refptr_g_touchesPtr + 0x23c) *
                                   0x14 + 0x16].bPeakCount), iVar2 < param_1)) &&
       (param_2 < *(short *)((longlong)(int)(uint)local_9 * 0x168 +
                             *(longlong *)_refptr_g_touchesPtr + 0x1be))) {
      local_a = local_a + '\x01';
      if (iVar2 < local_10) {
        local_10 = iVar2;
      }
      uVar3 = TSA_GetSide(*(undefined2 *)
                           ((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                           + 0x170),
                          *(undefined2 *)
                           ((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                           + 0x172));
      local_14 = local_14 | uVar3;
    }
  }
  if ((0x32 < local_10) && (param_3 != '\0')) {
    local_a = '\0';
  }
  if ((local_14 & 4) == 0) {
    local_a = '\0';
  }
  if ((local_14 & 8) == 0) {
    local_a = '\0';
  }
  return local_a;
}


// ============================================================================
// Function: SideTouch_AntiTriggerProcess @ 6bb07834
// ============================================================================

void SideTouch_AntiTriggerProcess(void)

{
  byte local_9;
  
                    /* 0xc7834  1331  SideTouch_AntiTriggerProcess */
  if (DAT_6bc97184 < 0x14) {
    DAT_6bc97184 = DAT_6bc97184 + 1;
  }
  else {
    DAT_6bc97184 = 0;
  }
  for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bRows; local_9 = local_9 + 1) {
    SideTouch_AntiTriggerUnit(&DAT_6bc97190 + (longlong)(int)(uint)local_9 * 0x30);
    SideTouch_AntiTriggerUnit(&DAT_6bc98218 + (longlong)(int)(uint)local_9 * 0x30);
  }
  return;
}


// ============================================================================
// Function: SideTouch_GestureReset @ 6bb0d910
// ============================================================================

void SideTouch_GestureReset(byte param_1)

{
  byte local_9;
  
                    /* 0xcd910  1352  SideTouch_GestureReset */
  (&DAT_6bc997c0)[(longlong)(int)(uint)param_1 * 0x2a8] = 0;
  *(undefined4 *)(&DAT_6bc997c4 + (longlong)(int)(uint)param_1 * 0x2a8) = 0;
  *(undefined4 *)(&DAT_6bc997c8 + (longlong)(int)(uint)param_1 * 0x2a8) = 0;
  *(undefined4 *)(&DAT_6bc997cc + (longlong)(int)(uint)param_1 * 0x2a8) = 0;
  (&DAT_6bc997d8)[(longlong)(int)(uint)param_1 * 0x2a8] = 0;
  (&DAT_6bc997d9)[(longlong)(int)(uint)param_1 * 0x2a8] = 0x14;
  (&DAT_6bc997da)[(longlong)(int)(uint)param_1 * 0x2a8] = 0;
  *(undefined2 *)(&DAT_6bc997dc + (longlong)(int)(uint)param_1 * 0x2a8) = 0;
  *(code **)(&DAT_6bc99808 + (longlong)(int)(uint)param_1 * 0x2a8) = SideGesture_CallbackIsToDetect;
  *(code **)(&DAT_6bc99810 + (longlong)(int)(uint)param_1 * 0x2a8) = SideGesture_CallbackIsToProcess
  ;
  *(code **)(&DAT_6bc99818 + (longlong)(int)(uint)param_1 * 0x2a8) = SideGesture_CallbackProcess;
  *(code **)(&DAT_6bc99820 + (longlong)(int)(uint)param_1 * 0x2a8) = SideGesture_CallbackIsToReport;
  *(code **)(&DAT_6bc99828 + (longlong)(int)(uint)param_1 * 0x2a8) = SideGesture_CallbackReport;
  for (local_9 = 0; local_9 < (byte)(&DAT_6bc995d8)[(longlong)(int)(uint)param_1 * 0x2a8];
      local_9 = local_9 + 1) {
    *(undefined2 *)
     (&DAT_6bc995f4 + (longlong)(int)(uint)local_9 * 0x30 + (longlong)(int)(uint)param_1 * 0x2a8) =
         0;
    *(undefined2 *)
     (&DAT_6bc995f2 + (longlong)(int)(uint)local_9 * 0x30 + (longlong)(int)(uint)param_1 * 0x2a8) =
         0xffff;
    *(undefined2 *)
     (&DAT_6bc995f6 + (longlong)(int)(uint)local_9 * 0x30 + (longlong)(int)(uint)param_1 * 0x2a8) =
         1;
    *(undefined4 *)
     (&DAT_6bc995f8 + (longlong)(int)(uint)local_9 * 0x30 + (longlong)(int)(uint)param_1 * 0x2a8) =
         0xffff8000;
    *(undefined8 *)
     (&DAT_6bc99600 + (longlong)(int)(uint)local_9 * 0x30 + (longlong)(int)(uint)param_1 * 0x2a8) =
         0xffffffffffffffff;
    *(undefined8 *)
     (&DAT_6bc99608 + (longlong)(int)(uint)local_9 * 0x30 + (longlong)(int)(uint)param_1 * 0x2a8) =
         0xffffffffffffffff;
  }
  return;
}


// ============================================================================
// Function: SideTouch_GestureExcludeExistedTouch @ 6bb16eb8
// ============================================================================

void SideTouch_GestureExcludeExistedTouch(byte param_1,undefined4 param_2)

{
  byte bVar1;
  byte local_9;
  
                    /* 0xd6eb8  1346  SideTouch_GestureExcludeExistedTouch */
  for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
    bVar1 = *(byte *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x287);
    if (bVar1 < 0x14) {
      *(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x588) =
           *(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)bVar1 * 0x478 + 0x588) |
           *(uint *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_1 * 0x2a8);
      SideTouch_TouchFailureRecord(local_9,param_2,0);
    }
  }
  return;
}


// ============================================================================
// Function: SideTouch_ShowVolumeArea @ 6bb1698a
// ============================================================================

void SideTouch_ShowVolumeArea(undefined2 param_1,undefined2 param_2,undefined4 param_3)

{
  if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64c & 1) != 0) {
    TSAIDE_LogPoint(0,param_1,param_3);
    TSAIDE_LogPoint(0,param_2,param_3);
  }
  if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64c & 2) != 0) {
    TSAIDE_LogPoint(*(short *)(_refptr_g_tsaPrmtFlashConst + 0x20) + -1,param_1,param_3);
    TSAIDE_LogPoint(*(short *)(_refptr_g_tsaPrmtFlashConst + 0x20) + -1,param_2,param_3);
  }
  return;
}


// ============================================================================
// Function: SideTouch_IsTouchInRegion @ 6bb0e4b1
// ============================================================================

undefined8 SideTouch_IsTouchInRegion(byte param_1,byte param_2)

{
  undefined8 uVar1;
  
  if (*(short *)(&DAT_6bc995c0 + (longlong)(int)(uint)param_2 * 0x2a8) == 0) {
    if (0x100 < *(ushort *)
                 ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170))
    {
      return 0;
    }
  }
  else if ((int)(uint)*(ushort *)
                       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x170) < (int)(*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x20) - 0x100)) {
    return 0;
  }
  if ((int)(uint)*(ushort *)
                  ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172)
      < (int)*(short *)(&DAT_6bc995c4 + (longlong)(int)(uint)param_2 * 0x2a8)) {
    uVar1 = 0;
  }
  else if ((int)*(short *)(&DAT_6bc995c6 + (longlong)(int)(uint)param_2 * 0x2a8) <
           (int)(uint)*(ushort *)
                       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x172)) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: SideTouch_IsActionDetected @ 6bb15792
// ============================================================================

int SideTouch_IsActionDetected(undefined1 param_1,undefined1 param_2,byte param_3)

{
  char cVar1;
  longlong lVar2;
  int local_10;
  byte local_9;
  
  local_10 = 0;
  if (*(longlong *)(&DAT_6bc99808 + (longlong)(int)(uint)param_3 * 0x2a8) == 0) {
    local_10 = 2;
  }
  else {
    cVar1 = (**(code **)(&DAT_6bc99808 + (longlong)(int)(uint)param_3 * 0x2a8))(param_3);
    if (cVar1 == '\0') {
      local_10 = 2;
    }
    else if ((&DAT_6bc995d8)[(longlong)(int)(uint)param_3 * 0x2a8] != '\0') {
      local_9 = (&DAT_6bc997c0)[(longlong)(int)(uint)param_3 * 0x2a8];
      if ((byte)(&DAT_6bc995d8)[(longlong)(int)(uint)param_3 * 0x2a8] <= local_9) {
        local_9 = (&DAT_6bc995d8)[(longlong)(int)(uint)param_3 * 0x2a8] - 1;
      }
      lVar2 = (longlong)(int)(uint)local_9 * 0x30 + (longlong)(int)(uint)param_3 * 0x2a8;
      if (*(int *)(&DAT_6bc995e0 + lVar2) == 1) {
        local_10 = SideTouch_IsOnActionDetected(param_1,param_2,param_3,local_9);
      }
      else {
        local_10 = SideTouch_IsOffActionDetected(param_1,param_2,param_3,local_9);
      }
      if (*(longlong *)(&DAT_6bc99600 + lVar2) == -1) {
        *(void **)(&DAT_6bc99600 + lVar2) = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
      }
      if (local_10 == 0) {
        *(void **)(&DAT_6bc99608 + lVar2) = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
      }
      if (local_10 == 2) {
        *(void **)(&DAT_6bc997d0 + (longlong)(int)(uint)param_3 * 0x2a8) =
             (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
        *(uint *)(&DAT_6bc997c8 + (longlong)(int)(uint)param_3 * 0x2a8) =
             *(uint *)(&DAT_6bc997c8 + (longlong)(int)(uint)param_3 * 0x2a8) | 0x10;
      }
    }
  }
  return local_10;
}


// ============================================================================
// Function: SideTouch_IsToExcludeTouch @ 6bb15a07
// ============================================================================

undefined8 SideTouch_IsToExcludeTouch(byte param_1,byte param_2)

{
  byte bVar1;
  char cVar2;
  undefined8 uVar3;
  bool bVar4;
  byte local_a;
  
  bVar4 = false;
  local_a = (&DAT_6bc997c0)[(longlong)(int)(uint)param_2 * 0x2a8];
  if ((byte)(&DAT_6bc995d8)[(longlong)(int)(uint)param_2 * 0x2a8] <= local_a) {
    local_a = (&DAT_6bc995d8)[(longlong)(int)(uint)param_2 * 0x2a8] - 1;
  }
  if (*(longlong *)(&DAT_6bc99808 + (longlong)(int)(uint)param_2 * 0x2a8) == 0) {
    uVar3 = 0;
  }
  else {
    if ((param_1 < 0x14) &&
       (bVar1 = *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x287), bVar1 < 0x14)) {
      cVar2 = (**(code **)(&DAT_6bc99808 + (longlong)(int)(uint)param_2 * 0x2a8))(param_2);
      if (cVar2 == '\0') {
        *(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x588)
             = *(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                        (longlong)(int)(uint)bVar1 * 0x478 + 0x588) |
               *(uint *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_2 * 0x2a8);
      }
      bVar4 = (*(uint *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_2 * 0x2a8) &
              *(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)bVar1 * 0x478 + 0x588)) != 0;
    }
    if (bVar4) {
      uVar3 = 1;
    }
    else if (*(int *)(&DAT_6bc995e0 +
                     (longlong)(int)(uint)local_a * 0x30 + (longlong)(int)(uint)param_2 * 0x2a8) ==
             1) {
      if (((&DAT_6bc997da)[(longlong)(int)(uint)param_2 * 0x2a8] == '\0') || (0x13 < param_1)) {
        cVar2 = SideGesture_IsValidTouch(param_1);
        if (cVar2 == '\0') {
          uVar3 = 1;
        }
        else {
          uVar3 = 0;
        }
      }
      else if ((&DAT_6bc997da)[(longlong)(int)(uint)param_2 * 0x2a8] ==
               *(char *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x154)) {
        cVar2 = SideGesture_IsValidTouch(param_1);
        if (cVar2 == '\0') {
          uVar3 = 1;
        }
        else {
          uVar3 = 0;
        }
      }
      else {
        uVar3 = 1;
      }
    }
    else {
      cVar2 = SideGesture_IsValidTouch(param_1);
      if (cVar2 == '\0') {
        uVar3 = 1;
      }
      else {
        uVar3 = 0;
      }
    }
  }
  return uVar3;
}


// ============================================================================
// Function: SideTouch_GestureProcessState @ 6bb15cfc
// ============================================================================

void SideTouch_GestureProcessState(byte param_1,char param_2,char param_3)

{
  char cVar1;
  
  if (param_2 == '\0') {
    if (param_3 == '\0') {
      if (*(int *)(&DAT_6bc997c4 + (longlong)(int)(uint)param_1 * 0x2a8) == 1) {
        *(undefined4 *)(&DAT_6bc997c4 + (longlong)(int)(uint)param_1 * 0x2a8) = 2;
      }
      else if (*(int *)(&DAT_6bc997c4 + (longlong)(int)(uint)param_1 * 0x2a8) == 3) {
        *(undefined4 *)(&DAT_6bc997c4 + (longlong)(int)(uint)param_1 * 0x2a8) = 0;
      }
    }
    else {
      cVar1 = SideTouch_IsGestureActive(param_1);
      SideTouch_GestureReset(param_1);
      if (cVar1 == '\0') {
        *(undefined4 *)(&DAT_6bc997c4 + (longlong)(int)(uint)param_1 * 0x2a8) = 0;
      }
      else {
        *(undefined4 *)(&DAT_6bc997c4 + (longlong)(int)(uint)param_1 * 0x2a8) = 3;
      }
    }
  }
  else if ((*(int *)(&DAT_6bc997c4 + (longlong)(int)(uint)param_1 * 0x2a8) == 0) ||
          (*(int *)(&DAT_6bc997c4 + (longlong)(int)(uint)param_1 * 0x2a8) == 3)) {
    *(undefined4 *)(&DAT_6bc997c4 + (longlong)(int)(uint)param_1 * 0x2a8) = 1;
  }
  else {
    *(undefined4 *)(&DAT_6bc997c4 + (longlong)(int)(uint)param_1 * 0x2a8) = 2;
  }
  return;
}


// ============================================================================
// Function: SideTouch_TouchFailureRecord @ 6bb14636
// ============================================================================

void SideTouch_TouchFailureRecord(byte param_1,int param_2,undefined4 param_3)

{
  byte bVar1;
  
  if (((param_1 < 0x14) &&
      (bVar1 = *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x287), bVar1 < 0x14)) &&
     (*(int *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x548) ==
      0)) {
    *(void **)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x540) =
         (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    *(int *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x548) =
         param_2;
    *(undefined4 *)
     ((longlong)*_refptr_g_prevTouchesPtr +
     ((longlong)(int)(uint)bVar1 * 0x11e + (ulonglong)(param_2 - 1) + 0x150) * 4 + 0xc) = param_3;
  }
  return;
}


// ============================================================================
// Function: SideTouch_GetTriggeredGesture @ 6bb1adb0
// ============================================================================

byte SideTouch_GetTriggeredGesture(byte param_1)

{
  char cVar1;
  byte local_9;
  
  local_9 = 0;
  while( true ) {
    if ((byte)*_refptr_g_sideGesture <= local_9) {
      return 0x19;
    }
    cVar1 = SideGesture_IsVolumeDoubleClickTrigger(local_9);
    if (((cVar1 != '\0') &&
        (_refptr_g_sideGesture[(longlong)(int)(uint)local_9 * 0x2a8 + 0x240] == '\x03')) &&
       (_refptr_g_sideGesture[(longlong)(int)(uint)local_9 * 0x2a8 + 0x25a] ==
        *(char *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x154))
       ) break;
    local_9 = local_9 + 1;
  }
  return local_9;
}


// ============================================================================
// Function: SideTouch_GetVolumeTriggerGestureTag @ 6bb1ab5f
// ============================================================================

undefined4 SideTouch_GetVolumeTriggerGestureTag(void)

{
  char cVar1;
  byte local_d;
  
                    /* 0xdab5f  1365  SideTouch_GetVolumeTriggerGestureTag */
  local_d = 0;
  while( true ) {
    if (g_sideGesture <= local_d) {
      return 0;
    }
    cVar1 = SideGesture_IsVolumeTrigger(local_d);
    if (cVar1 != '\0') break;
    local_d = local_d + 1;
  }
  return *(undefined4 *)(&DAT_6bc997e8 + (longlong)(int)(uint)local_d * 0x2a8);
}


// ============================================================================
// Function: SideTouch_IsPartOfCoreTouch @ 6bb1d258
// ============================================================================

undefined1 SideTouch_IsPartOfCoreTouch(byte param_1)

{
  undefined1 uVar1;
  bool bVar2;
  char cVar3;
  undefined1 uVar4;
  undefined1 uVar5;
  char cVar6;
  byte local_1b;
  undefined1 local_19;
  
  local_19 = 0;
  bVar2 = false;
  cVar3 = SideGesture_IsInVolumeArea(param_1);
  local_1b = 0;
  do {
    if ((byte)*_refptr_g_touchesGrid <= local_1b) {
LAB_6bb1d3df:
      if ((!bVar2) && (cVar3 == '\0')) {
        *(uint *)(_refptr_g_touchesSide + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) =
             *(uint *)(_refptr_g_touchesSide + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) &
             0xffffffdf;
        *(uint *)(_refptr_g_touchesSide + (longlong)(int)(uint)param_1 * 0x168 + 0x1f8) =
             *(uint *)(_refptr_g_touchesSide + (longlong)(int)(uint)param_1 * 0x168 + 0x1f8) &
             0xfdffffff;
      }
      return local_19;
    }
    if ((*(uint *)(_refptr_g_touchesGrid + (longlong)(int)(uint)local_1b * 0x168 + 0x21c) & 0x28) ==
        0) {
      uVar1 = _refptr_g_touchesGrid[(longlong)(int)(uint)local_1b * 0x168 + 0x28c];
      uVar4 = TSA_SideTouchDim2Center(param_1);
      uVar5 = TSA_SideTouchDim1Center(param_1);
      cVar6 = Sensor_IsInCoreZone(uVar5,uVar4,uVar1);
      if (cVar6 == '\0') {
        uVar1 = _refptr_g_touchesSide[(longlong)(int)(uint)param_1 * 0x168 + 0x28c];
        uVar4 = TSA_CoreTouchDim2Center(local_1b);
        uVar5 = TSA_CoreTouchDim1Center(local_1b);
        cVar6 = Sensor_IsInSideZone(uVar5,uVar4,uVar1);
        if (cVar6 == '\0') goto LAB_6bb1d3c2;
      }
      bVar2 = true;
      *(undefined *)
       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x198) =
           _refptr_g_touchesGrid[(longlong)(int)(uint)local_1b * 0x168 + 0x198];
      cVar6 = SideTouch_IsPartOfCoreTouchUnit(param_1,local_1b,cVar3);
      if (cVar6 == '\0') {
        local_19 = 0;
        goto LAB_6bb1d3df;
      }
      local_19 = 1;
    }
LAB_6bb1d3c2:
    local_1b = local_1b + 1;
  } while( true );
}


// ============================================================================
// Function: SideTouch_PressureIsVolumeDown @ 6bb1cb98
// ============================================================================

undefined4 SideTouch_PressureIsVolumeDown(void)

{
                    /* 0xdcb98  1396  SideTouch_PressureIsVolumeDown */
  return CONCAT31((int3)((uint)g_outPressure >> 8),g_outPressure == 1);
}


// ============================================================================
// Function: SideTouch_IsMotorControlEnabled @ 6bb0d8ff
// ============================================================================

undefined8 SideTouch_IsMotorControlEnabled(void)

{
                    /* 0xcd8ff  1377  SideTouch_IsMotorControlEnabled */
  return 0;
}


// ============================================================================
// Function: SideTouch_GestureVolumeForce @ 6bb0f00e
// ============================================================================

void SideTouch_GestureVolumeForce(int param_1)

{
  byte bVar1;
  undefined2 uVar2;
  short sVar3;
  errno_t eVar4;
  
  bVar1 = g_sideGesture;
  if (g_sideGesture < 0x19) {
    *(undefined4 *)(&DAT_6bc995dc + (longlong)(int)(uint)g_sideGesture * 0x2a8) = 0;
    (&DAT_6bc995d8)[(longlong)(int)(uint)bVar1 * 0x2a8] = 1;
    eVar4 = strcpy_s((char *)((longlong)(int)(uint)bVar1 * 0x2a8 + 0x6bc9958c),0x32,"VolumeForce");
    if (eVar4 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/SideTouch/SideTouch_Gesture.c",0x2c4,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
    uVar2 = TSAOut_ExtToIntDist(*(short *)&_refptr_g_tsaPrmtFlash[2].field_0x65c -
                                *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x664);
    *(undefined2 *)(&DAT_6bc995c4 + (longlong)(int)(uint)bVar1 * 0x2a8) = uVar2;
    uVar2 = TSAOut_ExtToIntDist(*(short *)&_refptr_g_tsaPrmtFlash[2].field_0x664 +
                                *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x65e);
    *(undefined2 *)(&DAT_6bc995c6 + (longlong)(int)(uint)bVar1 * 0x2a8) = uVar2;
    *(undefined4 *)(&DAT_6bc995e0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 1;
    *(undefined2 *)(&DAT_6bc995e4 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x8000;
    *(undefined2 *)(&DAT_6bc995e6 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x7fff;
    sVar3 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x666);
    *(short *)(&DAT_6bc995e8 + (longlong)(int)(uint)bVar1 * 0x2a8) = -sVar3;
    *(undefined2 *)(&DAT_6bc995ea + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x7fff;
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x666);
    *(undefined2 *)(&DAT_6bc995ec + (longlong)(int)(uint)bVar1 * 0x2a8) = uVar2;
    *(undefined2 *)(&DAT_6bc995ee + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x8000;
    *(undefined2 *)(&DAT_6bc995f0 + (longlong)(int)(uint)bVar1 * 0x2a8) =
         *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x668;
    if (param_1 == 4) {
      *(undefined2 *)(&DAT_6bc995c0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0;
      *(undefined2 *)(&DAT_6bc995c2 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0;
    }
    else {
      *(undefined2 *)(&DAT_6bc995c0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 1;
      *(undefined2 *)(&DAT_6bc995c2 + (longlong)(int)(uint)bVar1 * 0x2a8) = 1;
    }
    *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x10000;
    SideTouch_SetGestureEnabled(0x10000);
    SideTouch_GestureReset(bVar1);
    g_sideGesture = g_sideGesture + 1;
  }
  return;
}


// ============================================================================
// Function: SideTouch_GestureVolumeTriggerForceFlick @ 6bb10783
// ============================================================================

void SideTouch_GestureVolumeTriggerForceFlick(void)

{
  byte bVar1;
  undefined2 uVar2;
  errno_t eVar3;
  
  bVar1 = g_sideGesture;
  if (g_sideGesture < 0x19) {
    eVar3 = strcpy_s((char *)((longlong)(int)(uint)g_sideGesture * 0x2a8 + 0x6bc9958c),0x32,
                     "VolumeTriggerForceFlick");
    if (eVar3 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/SideTouch/SideTouch_Gesture.c",0x3ca,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
    *(undefined2 *)(&DAT_6bc995ea + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x7fff;
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x644);
    *(undefined2 *)(&DAT_6bc995c4 + (longlong)(int)(uint)bVar1 * 0x2a8) = uVar2;
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x646);
    *(undefined2 *)(&DAT_6bc995c6 + (longlong)(int)(uint)bVar1 * 0x2a8) = uVar2;
    *(undefined4 *)(&DAT_6bc995dc + (longlong)(int)(uint)bVar1 * 0x2a8) = 0;
    (&DAT_6bc995d8)[(longlong)(int)(uint)bVar1 * 0x2a8] = 1;
    *(undefined4 *)(&DAT_6bc995e0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 1;
    *(undefined2 *)(&DAT_6bc995e4 + (longlong)(int)(uint)bVar1 * 0x2a8) =
         *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x654;
    *(undefined2 *)(&DAT_6bc995e6 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x7fff;
    *(undefined2 *)(&DAT_6bc995e8 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x8000;
    *(undefined2 *)(&DAT_6bc995ea + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x7fff;
    *(undefined2 *)(&DAT_6bc995ec + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x7fff;
    *(undefined2 *)(&DAT_6bc995ee + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x8000;
    *(undefined2 *)(&DAT_6bc995f0 + (longlong)(int)(uint)bVar1 * 0x2a8) =
         *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x650;
    *(undefined4 *)(&DAT_6bc995c8 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x10;
    *(undefined2 *)(&DAT_6bc995c0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 1;
    *(undefined2 *)(&DAT_6bc995c2 + (longlong)(int)(uint)bVar1 * 0x2a8) = 1;
    *(undefined4 *)(&DAT_6bc995cc + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x800;
    *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x80000;
    SideTouch_SetGestureEnabled(0x80000);
    SideTouch_GestureReset(bVar1);
    g_sideGesture = g_sideGesture + 1;
  }
  return;
}


// ============================================================================
// Function: SideTouch_TriggerMoveGestureSetUp @ 6bb14147
// ============================================================================

void SideTouch_TriggerMoveGestureSetUp(void)

{
  if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64c & 1) != 0) {
    SideTouch_GestureVolumeTriggerFlick(4,1,0);
    SideTouch_GestureVolumeTriggerFlick(4,2,0);
  }
  if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64c & 2) != 0) {
    SideTouch_GestureVolumeTriggerFlick(8,1,0);
    SideTouch_GestureVolumeTriggerFlick(8,2,0);
  }
  return;
}


// ============================================================================
// Function: SideTouch_TriggerFlickGestureSetUp @ 6bb141da
// ============================================================================

void SideTouch_TriggerFlickGestureSetUp(void)

{
  if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64c & 1) != 0) {
    SideTouch_GestureVolumeTriggerFlick(4,1,1);
    SideTouch_GestureVolumeTriggerFlick(4,2,1);
  }
  if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64c & 2) != 0) {
    SideTouch_GestureVolumeTriggerFlick(8,1,1);
    SideTouch_GestureVolumeTriggerFlick(8,2,1);
  }
  return;
}


// ============================================================================
// Function: SideTouch_TriggerDoubleFlickGestureSetUp @ 6bb1426d
// ============================================================================

void SideTouch_TriggerDoubleFlickGestureSetUp(void)

{
  if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64c & 1) != 0) {
    SideTouch_GestureVolumeTriggerDoubleFlick(4,1);
    SideTouch_GestureVolumeTriggerDoubleFlick(4,2);
  }
  if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64c & 2) != 0) {
    SideTouch_GestureVolumeTriggerDoubleFlick(8,1);
    SideTouch_GestureVolumeTriggerDoubleFlick(8,2);
  }
  return;
}


// ============================================================================
// Function: SideTouch_OffMoveGestureSetUp @ 6bb142e8
// ============================================================================

void SideTouch_OffMoveGestureSetUp(void)

{
  if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64c & 1) != 0) {
    SideTouch_GestureVolumeOffMove(4);
  }
  if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64c & 2) != 0) {
    SideTouch_GestureVolumeOffMove(8);
  }
  return;
}


// ============================================================================
// Function: SideTouch_MuteGestureSetUp @ 6bb1433b
// ============================================================================

void SideTouch_MuteGestureSetUp(void)

{
  if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64c & 1) != 0) {
    SideTouch_GestureVolumeMute(4,1);
    SideTouch_GestureVolumeMute(4,2);
  }
  if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64c & 2) != 0) {
    SideTouch_GestureVolumeMute(8,1);
    SideTouch_GestureVolumeMute(8,2);
  }
  return;
}


// ============================================================================
// Function: SideTouch_DoubleClickGestureSetUp @ 6bb143b6
// ============================================================================

void SideTouch_DoubleClickGestureSetUp(undefined4 param_1)

{
  if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64c & 1) != 0) {
    SideTouch_GestureDoubleClick(4,param_1);
  }
  if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64c & 2) != 0) {
    SideTouch_GestureDoubleClick(8,param_1);
  }
  return;
}


// ============================================================================
// Function: SideTouch_TriggerClickFlickGestureSetUp @ 6bb14412
// ============================================================================

void SideTouch_TriggerClickFlickGestureSetUp(void)

{
  if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64c & 1) != 0) {
    SideTouch_GestureVolumeTriggerClickFlick(4,1);
    SideTouch_GestureVolumeTriggerClickFlick(4,2);
  }
  if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64c & 2) != 0) {
    SideTouch_GestureVolumeTriggerClickFlick(8,1);
    SideTouch_GestureVolumeTriggerClickFlick(8,2);
  }
  return;
}


// ============================================================================
// Function: SideTouch_TouchGestureSetUp @ 6bb1448d
// ============================================================================

void SideTouch_TouchGestureSetUp(void)

{
  if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64c & 1) != 0) {
    SideTouch_GestureVolumeTouch(4);
  }
  if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64c & 2) != 0) {
    SideTouch_GestureVolumeTouch(8);
  }
  return;
}


// ============================================================================
// Function: SideTouch_BLStageGetMaxDif @ 6bb08229
// ============================================================================

short SideTouch_BLStageGetMaxDif(short param_1,byte param_2,longlong param_3,int param_4)

{
  short local_a;
  
  if ((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x624 & 0x40) == 0) {
    local_a = *(short *)(param_3 + 0x1058);
  }
  else {
    local_a = param_1;
    if ((param_2 != 0) && (param_1 < *(short *)((longlong)(int)(param_2 - 1) * 0x30 + param_3 + 4)))
    {
      local_a = *(short *)((longlong)(int)(param_2 - 1) * 0x30 + param_3 + 4);
    }
    if (((int)(uint)param_2 < (int)(_refptr_g_tsaPrmtFlash->bRows - 1)) &&
       (local_a < *(short *)((longlong)(int)(param_2 + 1) * 0x30 + param_3 + 4))) {
      local_a = *(short *)((longlong)(int)(param_2 + 1) * 0x30 + param_3 + 4);
    }
  }
  if ((param_4 == 0) &&
     (local_a < *(short *)((ulonglong)
                           ((uint)_refptr_g_tsaPrmtFlash->bCols * (uint)param_2 + 1 & 0xffff) * 2 +
                          *(longlong *)_refptr_g_tsaBufDifGrid))) {
    local_a = *(short *)((ulonglong)
                         ((uint)_refptr_g_tsaPrmtFlash->bCols * (uint)param_2 + 1 & 0xffff) * 2 +
                        *(longlong *)_refptr_g_tsaBufDifGrid);
  }
  if ((param_4 == 1) &&
     (local_a < *(short *)((ulonglong)
                           ((uint)_refptr_g_tsaPrmtFlash->bCols * (param_2 + 1) - 2 & 0xffff) * 2 +
                          *(longlong *)_refptr_g_tsaBufDifGrid))) {
    local_a = *(short *)((ulonglong)
                         ((uint)_refptr_g_tsaPrmtFlash->bCols * (param_2 + 1) - 2 & 0xffff) * 2 +
                        *(longlong *)_refptr_g_tsaBufDifGrid);
  }
  return local_a;
}


// ============================================================================
// Function: SideTouch_RawCaptureApply @ 6bb08415
// ============================================================================

void SideTouch_RawCaptureApply(void)

{
  uchar uVar1;
  byte local_9;
  
  for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bRows; local_9 = local_9 + 1) {
    *(undefined2 *)(&DAT_6bc97190 + (longlong)(int)(uint)local_9 * 0x30) =
         *(undefined2 *)
          ((ulonglong)((uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols) * 2 +
          *(longlong *)_refptr_g_tsaBufRawCapture);
  }
  uVar1 = _refptr_g_tsaPrmtFlash->bCols;
  for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bRows; local_9 = local_9 + 1) {
    *(undefined2 *)(&DAT_6bc98218 + (longlong)(int)(uint)local_9 * 0x30) =
         *(undefined2 *)
          ((ulonglong)
           ((uint)(byte)(uVar1 - 1) + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)
           * 2 + *(longlong *)_refptr_g_tsaBufRawCapture);
  }
  return;
}


// ============================================================================
// Function: SideTouch_BaselineStagePrptUpdate @ 6bb0852c
// ============================================================================

void SideTouch_BaselineStagePrptUpdate(longlong param_1)

{
  short sVar1;
  byte local_11;
  int local_10;
  short local_a;
  
  local_a = -0x8000;
  local_10 = 0;
  for (local_11 = 0; local_11 < _refptr_g_tsaPrmtFlash->bRows; local_11 = local_11 + 1) {
    sVar1 = *(short *)((longlong)(int)(uint)local_11 * 0x30 + param_1 + 4);
    if (local_a < sVar1) {
      local_a = sVar1;
    }
    local_10 = local_10 + sVar1;
  }
  *(short *)(param_1 + 0x1058) = local_a;
  *(int *)(param_1 + 0x1060) = local_10;
  return;
}


// ============================================================================
// Function: SideTouch_BaselineDoReset @ 6bb0bbd4
// ============================================================================

void SideTouch_BaselineDoReset(void)

{
  byte local_9;
  
  for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bRows; local_9 = local_9 + 1) {
    *(undefined2 *)((longlong)(int)(uint)local_9 * 0x30 + g_baselineData + 2) =
         *(undefined2 *)((longlong)(int)(uint)local_9 * 0x30 + g_baselineData);
  }
  *(undefined4 *)(g_baselineData + 0x1080) = *(undefined4 *)(g_baselineData + 0x107c);
  return;
}


// ============================================================================
// Function: SideTouch_BaselineUpdate @ 6bb0be40
// ============================================================================

void SideTouch_BaselineUpdate(char param_1,short param_2,short param_3)

{
  if (((param_1 != '\0') || (-(int)param_2 <= (int)*(short *)(g_baselineData + 0x105a))) &&
     (*(short *)(g_baselineData + 0x1058) <= param_2)) {
    SideTouch_BaselineDoUpdate((int)param_3);
  }
  return;
}


// ============================================================================
// Function: SideTouch_BaselineGetPrpt @ 6bb0c89a
// ============================================================================

void SideTouch_BaselineGetPrpt(void)

{
  short sVar1;
  int iVar2;
  byte local_16;
  byte local_15;
  int local_14;
  short local_e;
  short local_c;
  byte local_9;
  
  local_c = -0x8000;
  local_e = 0x7fff;
  local_14 = 0;
  local_15 = 0;
  local_16 = 0;
  for (local_9 = 1; (int)(uint)local_9 < (int)(_refptr_g_tsaPrmtFlash->bRows - 1);
      local_9 = local_9 + 1) {
    sVar1 = *(short *)((longlong)(int)(uint)local_9 * 0x30 + g_baselineData + 4);
    if (local_c < sVar1) {
      local_15 = local_9;
      local_c = sVar1;
    }
    if (sVar1 < local_e) {
      local_16 = local_9;
      local_e = sVar1;
    }
    local_14 = local_14 + sVar1;
  }
  *(short *)(g_baselineData + 0x1058) = local_c;
  *(byte *)(g_baselineData + 0x105c) = local_15;
  *(short *)(g_baselineData + 0x105a) = local_e;
  *(byte *)(g_baselineData + 0x105d) = local_16;
  *(int *)(g_baselineData + 0x1060) = local_14;
  if (_refptr_g_tsaPrmtDynamic[0x30] == '\0') {
    iVar2 = *(int *)(g_baselineData + 0x1080) - *(int *)(g_baselineData + 0x107c);
  }
  else {
    iVar2 = *(int *)(g_baselineData + 0x107c) - *(int *)(g_baselineData + 0x1080);
  }
  *(int *)(g_baselineData + 0x1064) = iVar2;
  return;
}


// ============================================================================
// Function: SideTouch_BaselineResetInDebounce @ 6bb0ca20
// ============================================================================

bool SideTouch_BaselineResetInDebounce(void)

{
  return *(char *)(g_baselineStatic + 4) != '\0';
}


// ============================================================================
// Function: SideTouch_BaselineIsToResetLocally @ 6bb0cbac
// ============================================================================

void SideTouch_BaselineIsToResetLocally(void)

{
  char cVar1;
  
  cVar1 = TSAStatic_IsSafeBaselineApplied();
  if (cVar1 == '\0') {
    SideTouch_IsResetTriggered();
  }
  else {
    SideBaseline_IsResetTriggered();
  }
  return;
}


// ============================================================================
// Function: SideTouch_BaselineIsToResetGlobally @ 6bb0cbcf
// ============================================================================

undefined1 SideTouch_BaselineIsToResetGlobally(void)

{
  char cVar1;
  undefined1 local_9;
  
  local_9 = 0;
  if ((*_refptr_g_tsaStaticPtr)[1].field_0x1 == '\0') {
    cVar1 = TSAStatic_IsHWReset();
    if (cVar1 == '\0') {
      if (*_refptr_g_sideData != '\0') {
        *(undefined4 *)(g_baselinePrpt + 0x28) = 0x11;
        local_9 = 1;
      }
    }
    else {
      *(undefined4 *)(g_baselinePrpt + 0x28) = 1;
      local_9 = 1;
    }
  }
  else {
    *(undefined4 *)(g_baselinePrpt + 0x28) = 3;
    local_9 = 1;
  }
  return local_9;
}


// ============================================================================
// Function: SideTouch_IsToSyncResetWithCoreArea @ 6bb0cc54
// ============================================================================

undefined8 SideTouch_IsToSyncResetWithCoreArea(void)

{
  char cVar1;
  undefined8 uVar2;
  
  cVar1 = TSAStatic_IsSafeBaselineApplied();
  if (cVar1 == '\0') {
    uVar2 = 1;
  }
  else {
    cVar1 = SafeBaseline_IsToSyncResetForSideArea();
    if (cVar1 == '\0') {
      cVar1 = SideBaseline_ResetInDebounce();
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
  return uVar2;
}


// ============================================================================
// Function: SideTouch_AntiTriggerUnit @ 6bb0772a
// ============================================================================

void SideTouch_AntiTriggerUnit(longlong param_1)

{
  if (2000 < (ulonglong)
             ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - *(longlong *)(param_1 + 0x28)))
  {
    *(undefined8 *)(param_1 + 0x28) = 0;
    *(undefined1 *)(param_1 + 0x20) = 0;
  }
  if (DAT_6bc97184 < 0x14) {
    if (*(short *)(param_1 + 4) < *(short *)(param_1 + 10)) {
      *(undefined2 *)(param_1 + 10) = *(undefined2 *)(param_1 + 4);
    }
    if (*(short *)(param_1 + 0xc) < *(short *)(param_1 + 4)) {
      *(undefined2 *)(param_1 + 0xc) = *(undefined2 *)(param_1 + 4);
    }
  }
  else {
    if ((*(short *)&_refptr_g_tsaPrmtFlash[2].field_0x634 * 2) / 3 <
        (int)*(short *)(param_1 + 0xc) - (int)*(short *)(param_1 + 10)) {
      *(undefined1 *)(param_1 + 0xe) = 1;
    }
    else {
      *(undefined1 *)(param_1 + 0xe) = 0;
    }
    *(undefined2 *)(param_1 + 0xc) = 0x8000;
    *(undefined2 *)(param_1 + 10) = 0x7fff;
  }
  return;
}


// ============================================================================
// Function: SideTouch_IsOnActionDetected @ 6bb1530b
// ============================================================================

undefined4 SideTouch_IsOnActionDetected(byte param_1,byte param_2,byte param_3,byte param_4)

{
  undefined4 uVar1;
  
  if ((param_1 < 0x14) && (param_2 < 0x14)) {
    if (*(short *)(&DAT_6bc995ee +
                  (longlong)(int)(uint)param_4 * 0x30 + (longlong)(int)(uint)param_3 * 0x2a8) !=
        -0x8000) {
      if ((*(int *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c
                   ) == 2) ||
         (*(int *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_2 * 0x478 + 0x21c) == 2)) {
        *(uint *)(&DAT_6bc997c8 + (longlong)(int)(uint)param_3 * 0x2a8) =
             *(uint *)(&DAT_6bc997c8 + (longlong)(int)(uint)param_3 * 0x2a8) | 1;
      }
      if ((*(uint *)(&DAT_6bc997c8 + (longlong)(int)(uint)param_3 * 0x2a8) & 1) == 0) {
        if (2 < *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x21c)) {
          SideTouch_GestureFailureRecord(param_3,10,0);
          return 2;
        }
        return 1;
      }
    }
    uVar1 = SideTouch_DetectOnAction(param_1,param_2,param_3,param_4);
  }
  else {
    if ((&DAT_6bc997da)[(longlong)(int)(uint)param_3 * 0x2a8] != '\0') {
      SideTouch_GestureFailureRecord(param_3,1,0);
    }
    uVar1 = 2;
  }
  return uVar1;
}


// ============================================================================
// Function: SideTouch_IsOffActionDetected @ 6bb154e9
// ============================================================================

int SideTouch_IsOffActionDetected(undefined1 param_1,undefined8 param_2,byte param_3,byte param_4)

{
  char cVar1;
  short sVar2;
  longlong lVar3;
  bool bVar4;
  int local_c;
  
  cVar1 = SideGesture_IsValidTouch(param_1);
  bVar4 = cVar1 == '\0';
  lVar3 = (longlong)(int)(uint)param_4 * 0x30 + (longlong)(int)(uint)param_3 * 0x2a8;
  local_c = 0;
  (&DAT_6bc997da)[(longlong)(int)(uint)param_3 * 0x2a8] = 0;
  *(uint *)(&DAT_6bc997c8 + (longlong)(int)(uint)param_3 * 0x2a8) =
       *(uint *)(&DAT_6bc997c8 + (longlong)(int)(uint)param_3 * 0x2a8) & 0xfffffff8;
  sVar2 = (short)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
          (short)*(undefined8 *)(&DAT_6bc997f8 + (longlong)(int)(uint)param_3 * 0x2a8);
  if (bVar4) {
    *(uint *)(&DAT_6bc997c8 + (longlong)(int)(uint)param_3 * 0x2a8) =
         *(uint *)(&DAT_6bc997c8 + (longlong)(int)(uint)param_3 * 0x2a8) | 8;
  }
  if ((*(uint *)(&DAT_6bc997c8 + (longlong)(int)(uint)param_3 * 0x2a8) & 8) == 0) {
    local_c = 1;
  }
  else if ((bVar4) || ((*(uint *)(&DAT_6bc997c4 + (longlong)(int)(uint)param_3 * 0x2a8) & 3) == 0))
  {
    if ((*(short *)(&DAT_6bc995e6 + lVar3) != 0x7fff) && (bVar4)) {
      if (*(short *)(&DAT_6bc995e6 + lVar3) < sVar2) {
        local_c = 2;
        SideTouch_GestureFailureRecord(param_3,2,(int)sVar2);
      }
      else {
        local_c = 1;
      }
    }
    if ((*(short *)(&DAT_6bc995e4 + lVar3) == -0x8000) || (bVar4)) {
      if ((*(short *)(&DAT_6bc995e4 + lVar3) != -0x8000) &&
         ((sVar2 < *(short *)(&DAT_6bc995e4 + lVar3) && (local_c != 2)))) {
        local_c = 1;
      }
    }
    else if (sVar2 < *(short *)(&DAT_6bc995e4 + lVar3)) {
      local_c = 2;
      SideTouch_GestureFailureRecord(param_3,1,(int)sVar2);
    }
  }
  else {
    local_c = 2;
    SideTouch_GestureFailureRecord(param_3,1,(int)sVar2);
  }
  *(int *)(&DAT_6bc995fc + lVar3) = (int)sVar2;
  return local_c;
}


// ============================================================================
// Function: SideTouch_IsPartOfCoreTouchUnit @ 6bb1d0bf
// ============================================================================

undefined1 SideTouch_IsPartOfCoreTouchUnit(byte param_1,byte param_2)

{
  char cVar1;
  undefined1 local_9;
  
  local_9 = 0;
  if ((*(short *)(_refptr_g_touchesGrid + (longlong)(int)(uint)param_2 * 0x168 + 0x156) == 1) ||
     (*(short *)(_refptr_g_touchesSide + (longlong)(int)(uint)param_1 * 0x168 + 0x156) == 1)) {
    *(uint *)(_refptr_g_touchesSide + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) =
         *(uint *)(_refptr_g_touchesSide + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) | 0x20;
  }
  if ((*(uint *)(_refptr_g_touchesSide + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) & 0x20) != 0)
  {
    cVar1 = SideTouch_IsSideOverCore(param_1,param_2);
    if (cVar1 == '\0') {
      cVar1 = SideTouch_IsInCore(param_1,param_2);
      if (cVar1 != '\0') {
        local_9 = 1;
      }
    }
    else {
      _refptr_g_prevTouchesGrid
      [(longlong)
       (int)(uint)(byte)_refptr_g_touchesGrid[(longlong)(int)(uint)param_2 * 0x168 + 0x287] * 0x478
       + 0x288] = 0;
      *(uint *)(_refptr_g_touchesGrid + (longlong)(int)(uint)param_2 * 0x168 + 0x1f0) =
           *(uint *)(_refptr_g_touchesGrid + (longlong)(int)(uint)param_2 * 0x168 + 0x1f0) |
           0x8000000;
    }
  }
  return local_9;
}


// ============================================================================
// Function: SideTouch_GestureVolumeTriggerFlick @ 6bb0f996
// ============================================================================

void SideTouch_GestureVolumeTriggerFlick(int param_1,undefined4 param_2,undefined1 param_3)

{
  byte bVar1;
  undefined2 uVar2;
  
  bVar1 = g_sideGesture;
  if (g_sideGesture < 0x19) {
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x644);
    *(undefined2 *)(&DAT_6bc995c4 + (longlong)(int)(uint)bVar1 * 0x2a8) = uVar2;
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x646);
    *(undefined2 *)(&DAT_6bc995c6 + (longlong)(int)(uint)bVar1 * 0x2a8) = uVar2;
    SideTouch_GestureVolumeTriggerFlickAction(bVar1,param_2,param_3);
    *(undefined4 *)(&DAT_6bc995c8 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x10;
    if (param_1 == 4) {
      *(undefined2 *)(&DAT_6bc995c0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0;
      *(undefined2 *)(&DAT_6bc995c2 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0;
      *(undefined4 *)(&DAT_6bc995cc + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x400;
    }
    else {
      *(undefined2 *)(&DAT_6bc995c0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 1;
      *(undefined2 *)(&DAT_6bc995c2 + (longlong)(int)(uint)bVar1 * 0x2a8) = 1;
      *(undefined4 *)(&DAT_6bc995cc + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x800;
    }
    SideTouch_InternalGestureVolumeTriggerFlick(param_1,param_2,param_3,bVar1);
    SideTouch_GestureReset(bVar1);
    g_sideGesture = g_sideGesture + 1;
  }
  return;
}


// ============================================================================
// Function: SideTouch_GestureVolumeTriggerDoubleFlick @ 6bb11db7
// ============================================================================

void SideTouch_GestureVolumeTriggerDoubleFlick(int param_1,int param_2)

{
  byte bVar1;
  undefined2 uVar2;
  errno_t eVar3;
  
  bVar1 = g_sideGesture;
  if (g_sideGesture < 0x19) {
    eVar3 = strcpy_s((char *)((longlong)(int)(uint)g_sideGesture * 0x2a8 + 0x6bc9958c),0x32,
                     "VolumeTriggerDoubleFlick");
    if (eVar3 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/SideTouch/SideTouch_Gesture.c",0x475,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x644);
    *(undefined2 *)(&DAT_6bc995c4 + (longlong)(int)(uint)bVar1 * 0x2a8) = uVar2;
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x646);
    *(undefined2 *)(&DAT_6bc995c6 + (longlong)(int)(uint)bVar1 * 0x2a8) = uVar2;
    SideTouch_GestureVolumeTriggerDoubleFlickAction(bVar1,param_2);
    *(undefined4 *)(&DAT_6bc995c8 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x10;
    if (param_1 == 4) {
      *(undefined2 *)(&DAT_6bc995c0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0;
      *(undefined2 *)(&DAT_6bc995c2 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0;
      *(undefined4 *)(&DAT_6bc995cc + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x400;
      if (param_2 == 1) {
        *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x200;
        SideTouch_SetGestureEnabled(0x200);
      }
      else {
        *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x400;
        SideTouch_SetGestureEnabled(0x400);
      }
    }
    else {
      *(undefined2 *)(&DAT_6bc995c0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 1;
      *(undefined2 *)(&DAT_6bc995c2 + (longlong)(int)(uint)bVar1 * 0x2a8) = 1;
      *(undefined4 *)(&DAT_6bc995cc + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x800;
      if (param_2 == 1) {
        *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x800;
        SideTouch_SetGestureEnabled(0x800);
      }
      else {
        *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x1000;
        SideTouch_SetGestureEnabled(0x1000);
      }
    }
    SideTouch_GestureReset(bVar1);
    g_sideGesture = g_sideGesture + 1;
  }
  return;
}


// ============================================================================
// Function: SideTouch_GestureVolumeOffMove @ 6bb130dd
// ============================================================================

void SideTouch_GestureVolumeOffMove(int param_1)

{
  byte bVar1;
  undefined2 uVar2;
  errno_t eVar3;
  
  bVar1 = g_sideGesture;
  if (g_sideGesture < 0x19) {
    eVar3 = strcpy_s((char *)((longlong)(int)(uint)g_sideGesture * 0x2a8 + 0x6bc9958c),0x32,
                     "VolumeOffMove");
    if (eVar3 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/SideTouch/SideTouch_Gesture.c",0x524,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x644);
    *(undefined2 *)(&DAT_6bc995c4 + (longlong)(int)(uint)bVar1 * 0x2a8) = uVar2;
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x646);
    *(undefined2 *)(&DAT_6bc995c6 + (longlong)(int)(uint)bVar1 * 0x2a8) = uVar2;
    *(undefined4 *)(&DAT_6bc995dc + (longlong)(int)(uint)bVar1 * 0x2a8) = 0;
    (&DAT_6bc995d8)[(longlong)(int)(uint)bVar1 * 0x2a8] = 1;
    *(undefined4 *)(&DAT_6bc995e0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 1;
    *(undefined2 *)(&DAT_6bc995e4 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x8000;
    *(undefined2 *)(&DAT_6bc995e6 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x7fff;
    *(undefined2 *)(&DAT_6bc995e8 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x8000;
    *(undefined2 *)(&DAT_6bc995ea + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x7fff;
    *(undefined2 *)(&DAT_6bc995ec + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x7fff;
    *(undefined2 *)(&DAT_6bc995ee + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x8000;
    *(undefined2 *)(&DAT_6bc995f0 + (longlong)(int)(uint)bVar1 * 0x2a8) =
         *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x650;
    if (param_1 == 4) {
      *(undefined2 *)(&DAT_6bc995c0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0;
      *(undefined2 *)(&DAT_6bc995c2 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0;
      *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x2000;
      SideTouch_SetGestureEnabled(0x2000);
    }
    else {
      *(undefined2 *)(&DAT_6bc995c0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 1;
      *(undefined2 *)(&DAT_6bc995c2 + (longlong)(int)(uint)bVar1 * 0x2a8) = 1;
      *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x4000;
      SideTouch_SetGestureEnabled(0x4000);
    }
    *(undefined4 *)(&DAT_6bc995c8 + (longlong)(int)(uint)bVar1 * 0x2a8) = 1;
    SideTouch_GestureReset(bVar1);
    g_sideGesture = g_sideGesture + 1;
  }
  return;
}


// ============================================================================
// Function: SideTouch_GestureVolumeMute @ 6bb12dff
// ============================================================================

void SideTouch_GestureVolumeMute(int param_1,int param_2)

{
  byte bVar1;
  undefined2 uVar2;
  errno_t eVar3;
  
  bVar1 = g_sideGesture;
  if (g_sideGesture < 0x19) {
    eVar3 = strcpy_s((char *)((longlong)(int)(uint)g_sideGesture * 0x2a8 + 0x6bc9958c),0x32,
                     "VolumeMute");
    if (eVar3 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/SideTouch/SideTouch_Gesture.c",0x4fa,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x644);
    *(undefined2 *)(&DAT_6bc995c4 + (longlong)(int)(uint)bVar1 * 0x2a8) = uVar2;
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x646);
    *(undefined2 *)(&DAT_6bc995c6 + (longlong)(int)(uint)bVar1 * 0x2a8) = uVar2;
    SideTouch_GestureVolumeMuteAction(bVar1,param_2);
    *(undefined4 *)(&DAT_6bc995c8 + (longlong)(int)(uint)bVar1 * 0x2a8) = 2;
    *(undefined4 *)(&DAT_6bc995cc + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x71;
    if (param_1 == 4) {
      *(undefined2 *)(&DAT_6bc995c0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0;
      *(undefined2 *)(&DAT_6bc995c2 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0;
      if (param_2 == 1) {
        *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x20000;
        SideTouch_SetGestureEnabled(0x20000);
      }
      else {
        *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x40000;
        SideTouch_SetGestureEnabled(0x40000);
      }
    }
    else {
      *(undefined2 *)(&DAT_6bc995c0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 1;
      *(undefined2 *)(&DAT_6bc995c2 + (longlong)(int)(uint)bVar1 * 0x2a8) = 1;
      if (param_2 == 1) {
        *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x20000000;
        SideTouch_SetGestureEnabled(0x20000000);
      }
      else {
        *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x40000000;
        SideTouch_SetGestureEnabled(0x40000000);
      }
    }
    SideTouch_GestureReset(bVar1);
    g_sideGesture = g_sideGesture + 1;
  }
  return;
}


// ============================================================================
// Function: SideTouch_GestureDoubleClick @ 6bb13d31
// ============================================================================

void SideTouch_GestureDoubleClick(int param_1,uint param_2)

{
  byte bVar1;
  undefined2 uVar2;
  errno_t eVar3;
  
  bVar1 = g_sideGesture;
  if (g_sideGesture < 0x19) {
    if ((param_2 & 0x10) == 0) {
      eVar3 = strcpy_s((char *)((longlong)(int)(uint)g_sideGesture * 0x2a8 + 0x6bc9958c),0x32,
                       "VolumeTriggerDoubleClick");
      if (eVar3 != 0) {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                          "../../src/Extension/SideTouch/SideTouch_Gesture.c",0x586,"rc == EOK");
        HardwareAnalyzer_SetSoftException();
      }
    }
    else {
      eVar3 = strcpy_s((char *)((longlong)(int)(uint)g_sideGesture * 0x2a8 + 0x6bc9958c),0x32,
                       "PowerOffDoubleClick");
      if (eVar3 != 0) {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                          "../../src/Extension/SideTouch/SideTouch_Gesture.c",0x583,"rc == EOK");
        HardwareAnalyzer_SetSoftException();
      }
    }
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x644);
    *(undefined2 *)(&DAT_6bc995c4 + (longlong)(int)(uint)bVar1 * 0x2a8) = uVar2;
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x646);
    *(undefined2 *)(&DAT_6bc995c6 + (longlong)(int)(uint)bVar1 * 0x2a8) = uVar2;
    SideTouch_GestureDoubleClickAction(bVar1);
    if ((param_2 & 0x10) == 0) {
      *(undefined4 *)(&DAT_6bc995c8 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x10;
      if (param_1 == 4) {
        *(undefined2 *)(&DAT_6bc995c0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0;
        *(undefined2 *)(&DAT_6bc995c2 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0;
        *(undefined4 *)(&DAT_6bc995cc + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x400;
        *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x200000;
        SideTouch_SetGestureEnabled(0x200000);
      }
      else {
        *(undefined2 *)(&DAT_6bc995c0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 1;
        *(undefined2 *)(&DAT_6bc995c2 + (longlong)(int)(uint)bVar1 * 0x2a8) = 1;
        *(undefined4 *)(&DAT_6bc995cc + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x800;
        *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x400000;
        SideTouch_SetGestureEnabled(0x400000);
      }
    }
    else {
      if (param_1 == 4) {
        *(undefined2 *)(&DAT_6bc995c0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0;
        *(undefined2 *)(&DAT_6bc995c2 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0;
      }
      else {
        *(undefined2 *)(&DAT_6bc995c0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 1;
        *(undefined2 *)(&DAT_6bc995c2 + (longlong)(int)(uint)bVar1 * 0x2a8) = 1;
      }
      *(undefined4 *)(&DAT_6bc995c8 + (longlong)(int)(uint)bVar1 * 0x2a8) = 2;
      *(undefined4 *)(&DAT_6bc995cc + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x74;
      *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x8000;
      SideTouch_SetGestureEnabled(0x8000);
    }
    SideTouch_GestureReset(bVar1);
    g_sideGesture = g_sideGesture + 1;
  }
  return;
}


// ============================================================================
// Function: SideTouch_GestureVolumeTriggerClickFlick @ 6bb10481
// ============================================================================

void SideTouch_GestureVolumeTriggerClickFlick(int param_1,int param_2)

{
  byte bVar1;
  undefined2 uVar2;
  errno_t eVar3;
  
  bVar1 = g_sideGesture;
  if (g_sideGesture < 0x19) {
    eVar3 = strcpy_s((char *)((longlong)(int)(uint)g_sideGesture * 0x2a8 + 0x6bc9958c),0x32,
                     "VolumeTriggerClickFlick");
    if (eVar3 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/SideTouch/SideTouch_Gesture.c",0x39c,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x644);
    *(undefined2 *)(&DAT_6bc995c4 + (longlong)(int)(uint)bVar1 * 0x2a8) = uVar2;
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x646);
    *(undefined2 *)(&DAT_6bc995c6 + (longlong)(int)(uint)bVar1 * 0x2a8) = uVar2;
    SideTouch_GestureVolumeTriggerClickFlickAction(bVar1,param_2);
    *(undefined4 *)(&DAT_6bc995c8 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x10;
    if (param_1 == 4) {
      *(undefined2 *)(&DAT_6bc995c0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0;
      *(undefined2 *)(&DAT_6bc995c2 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0;
      *(undefined4 *)(&DAT_6bc995cc + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x400;
      if (param_2 == 1) {
        *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x2000000;
        SideTouch_SetGestureEnabled(0x2000000);
      }
      else {
        *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x4000000;
        SideTouch_SetGestureEnabled(0x4000000);
      }
    }
    else {
      *(undefined2 *)(&DAT_6bc995c0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 1;
      *(undefined2 *)(&DAT_6bc995c2 + (longlong)(int)(uint)bVar1 * 0x2a8) = 1;
      *(undefined4 *)(&DAT_6bc995cc + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x800;
      if (param_2 == 1) {
        *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x8000000;
        SideTouch_SetGestureEnabled(0x8000000);
      }
      else {
        *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x10000000;
        SideTouch_SetGestureEnabled(0x10000000);
      }
    }
    SideTouch_GestureReset(bVar1);
    g_sideGesture = g_sideGesture + 1;
  }
  return;
}


// ============================================================================
// Function: SideTouch_GestureVolumeTouch @ 6bb11099
// ============================================================================

void SideTouch_GestureVolumeTouch(int param_1)

{
  byte bVar1;
  undefined2 uVar2;
  errno_t eVar3;
  
  bVar1 = g_sideGesture;
  if (g_sideGesture < 0x19) {
    eVar3 = strcpy_s((char *)((longlong)(int)(uint)g_sideGesture * 0x2a8 + 0x6bc9958c),0x32,
                     "VolumeTouch");
    if (eVar3 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/SideTouch/SideTouch_Gesture.c",0x411,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x644);
    *(undefined2 *)(&DAT_6bc995c4 + (longlong)(int)(uint)bVar1 * 0x2a8) = uVar2;
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x646);
    *(undefined2 *)(&DAT_6bc995c6 + (longlong)(int)(uint)bVar1 * 0x2a8) = uVar2;
    SideTouch_GestureVolumeTouchAction(bVar1);
    *(undefined4 *)(&DAT_6bc995c8 + (longlong)(int)(uint)bVar1 * 0x2a8) = 1;
    if (param_1 == 4) {
      *(undefined2 *)(&DAT_6bc995c0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0;
      *(undefined2 *)(&DAT_6bc995c2 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0;
      *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x800000;
      SideTouch_SetGestureEnabled(0x800000);
    }
    else {
      *(undefined2 *)(&DAT_6bc995c0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 1;
      *(undefined2 *)(&DAT_6bc995c2 + (longlong)(int)(uint)bVar1 * 0x2a8) = 1;
      *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)bVar1 * 0x2a8) = 0x1000000;
      SideTouch_SetGestureEnabled(0x1000000);
    }
    SideTouch_GestureReset(bVar1);
    g_sideGesture = g_sideGesture + 1;
  }
  return;
}


// ============================================================================
// Function: SideTouch_BaselineDoUpdate @ 6bb0bcd2
// ============================================================================

void SideTouch_BaselineDoUpdate(short param_1)

{
  uint local_18;
  short local_12;
  uint local_10;
  byte local_9;
  
  *(undefined1 *)(g_baselineStatic + 0x106) = 1;
  for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bRows; local_9 = local_9 + 1) {
    local_10 = (uint)*(ushort *)((longlong)(int)(uint)local_9 * 0x30 + g_baselineData);
    local_18 = (uint)*(ushort *)((longlong)(int)(uint)local_9 * 0x30 + g_baselineData + 2);
    SideTouch_BaselineDoUpdateUnit(local_10,&local_18,(int)param_1);
    *(short *)((longlong)(int)(uint)local_9 * 0x30 + g_baselineData + 2) = (short)local_18;
  }
  local_12 = param_1 * (((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 2) /
                        *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x634 + 1);
  local_10 = *(undefined4 *)(g_baselineData + 0x107c);
  local_18 = *(uint *)(g_baselineData + 0x1080);
  SideTouch_BaselineDoUpdateUnit(local_10,&local_18,(int)local_12);
  *(uint *)(g_baselineData + 0x1080) = local_18;
  return;
}


// ============================================================================
// Function: SideTouch_IsResetTriggered @ 6bb0caa8
// ============================================================================

undefined1 SideTouch_IsResetTriggered(void)

{
  char cVar1;
  undefined1 local_9;
  
  local_9 = 0;
  cVar1 = SideTouch_BaselineResetCheck();
  if (cVar1 == '\0') {
    if (*(char *)(g_baselineStatic + 0x10) == '\0') {
      if (*(byte *)(g_baselineStatic + 4) < 2) {
        *(undefined1 *)(g_baselineStatic + 4) = 0;
      }
      else {
        *(char *)(g_baselineStatic + 4) = *(char *)(g_baselineStatic + 4) + -1;
      }
    }
    else {
      *(undefined1 *)(g_baselineStatic + 4) = 0;
    }
  }
  else {
    *(char *)(g_baselineStatic + 4) = *(char *)(g_baselineStatic + 4) + '\x01';
    if (0x14 < *(byte *)(g_baselineStatic + 4)) {
      local_9 = 1;
      *(undefined1 *)(g_baselineStatic + 4) = 0;
      *(undefined4 *)(g_baselinePrpt + 0x28) = 0xb;
      (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
           (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 1;
    }
  }
  *(undefined1 *)(g_baselineStatic + 0x10) = local_9;
  return local_9;
}


// ============================================================================
// Function: SideTouch_GestureEnabledRunTime @ 6bb0e0d0
// ============================================================================

undefined4 SideTouch_GestureEnabledRunTime(uint param_1)

{
  return CONCAT31((int3)((DAT_6bc9d7f4 & param_1) >> 8),(DAT_6bc9d7f4 & param_1) != 0);
}


// ============================================================================
// Function: SideTouch_GestureAntiTriggerLocally @ 6bb0b941
// ============================================================================

void SideTouch_GestureAntiTriggerLocally(byte param_1)

{
  byte bVar1;
  char cVar2;
  
                    /* 0xcb941  1345  SideTouch_GestureAntiTriggerLocally */
  cVar2 = SideGesture_IsVolumeDoubleClickTrigger(param_1);
  if ((((cVar2 != '\0') &&
       (SideTouch_AntiTriggerStateProcess(param_1),
       _refptr_g_sideGesture[(longlong)(int)(uint)param_1 * 0x2a8 + 0x240] == '\x03')) &&
      ((uint)(byte)_refptr_g_sideGesture[(longlong)(int)(uint)param_1 * 0x2a8 + 0x240] ==
       (byte)_refptr_g_sideGesture[(longlong)(int)(uint)param_1 * 0x2a8 + 0x241] + 1)) &&
     (cVar2 = SideTouch_IsToResetTrigger(param_1), cVar2 != '\0')) {
    SideTouch_GestureFailureRecord(param_1,0xe,0);
    bVar1 = _refptr_g_sideGesture[(longlong)(int)(uint)param_1 * 0x2a8 + 0x259];
    if (bVar1 < 0x14) {
      *(uint *)((longlong)(int)(uint)bVar1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1fc) =
           *(uint *)((longlong)(int)(uint)bVar1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1fc)
           | 0x200;
      SideTouch_GestureLogProcess();
    }
    SideTouch_GestureReset(param_1);
  }
  return;
}


// ============================================================================
// Function: SideTouch_GestureLogUpdate @ 6bb1b6da
// ============================================================================

void SideTouch_GestureLogUpdate(byte param_1)

{
  byte bVar1;
  byte bVar2;
  char cVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  
                    /* 0xdb6da  1350  SideTouch_GestureLogUpdate */
  if ((byte)_refptr_g_sideGesture[(longlong)(int)(uint)param_1 * 0x2a8 + 0x259] < 0x14) {
    bVar1 = _refptr_g_sideGesture[(longlong)(int)(uint)param_1 * 0x2a8 + 0x259];
    bVar2 = *(byte *)((longlong)(int)(uint)bVar1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x287
                     );
    if ((bVar2 < 0x14) &&
       (SideTouch_GestureLogStaticsUpdate(bVar1,bVar2),
       *(short *)(_refptr_g_sideGesture + (longlong)(int)(uint)param_1 * 0x2a8 + 0x68) != -0x8000))
    {
      uVar4 = (uint)*(short *)((longlong)(int)(uint)bVar1 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x296);
      uVar5 = (int)uVar4 >> 0x1f;
      uVar6 = (int)*(short *)(_refptr_g_sideGesture + (longlong)(int)(uint)param_1 * 0x2a8 + 0x68)
              >> 0x1f;
      if (((int)(((int)*(short *)(_refptr_g_sideGesture +
                                 (longlong)(int)(uint)param_1 * 0x2a8 + 0x68) ^ uVar6) - uVar6) <=
           (int)((uVar4 ^ uVar5) - uVar5)) &&
         (*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)bVar2 * 0x478 + 0x538) == 0)) {
        *(undefined2 *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar2 * 0x478 + 0x538) =
             *(undefined2 *)
              ((longlong)(int)(uint)bVar1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x294);
      }
      if ((*(short *)(_refptr_g_sideGesture + (longlong)(int)(uint)param_1 * 0x2a8 + 0x6a) <=
           *(short *)((longlong)(int)(uint)bVar1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x294
                     )) &&
         (*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)bVar2 * 0x478 + 0x53a) == 0)) {
        *(undefined2 *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar2 * 0x478 + 0x53a) =
             *(undefined2 *)
              ((longlong)(int)(uint)bVar1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x296);
      }
    }
    cVar3 = SideTouch_IsGestureActive(param_1);
    if (cVar3 != '\0') {
      *(uint *)((longlong)(int)(uint)bVar1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) =
           *(uint *)((longlong)(int)(uint)bVar1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
           | 0x4000000;
    }
  }
  return;
}


// ============================================================================
// Function: SideTouch_CoordinateMapping @ 6bb163d5
// ============================================================================

void SideTouch_CoordinateMapping(byte param_1,byte param_2)

{
  undefined2 uVar1;
  uint uVar2;
  uint uVar3;
  
  uVar2 = (int)*(short *)(&DAT_6bc995ce + (longlong)(int)(uint)param_2 * 0x2a8) -
          (int)*(short *)(&DAT_6bc995cc + (longlong)(int)(uint)param_2 * 0x2a8) >> 0x1f;
  uVar3 = (int)*(short *)(&DAT_6bc995d2 + (longlong)(int)(uint)param_2 * 0x2a8) -
          (int)*(short *)(&DAT_6bc995d0 + (longlong)(int)(uint)param_2 * 0x2a8) >> 0x1f;
  if ((int)(((int)*(short *)(&DAT_6bc995d2 + (longlong)(int)(uint)param_2 * 0x2a8) -
             (int)*(short *)(&DAT_6bc995d0 + (longlong)(int)(uint)param_2 * 0x2a8) ^ uVar3) - uVar3)
      < (int)(((int)*(short *)(&DAT_6bc995ce + (longlong)(int)(uint)param_2 * 0x2a8) -
               (int)*(short *)(&DAT_6bc995cc + (longlong)(int)(uint)param_2 * 0x2a8) ^ uVar2) -
             uVar2)) {
    uVar1 = SideTouch_GetMappedCorePos
                      ((int)*(short *)(&DAT_6bc995c4 + (longlong)(int)(uint)param_2 * 0x2a8),
                       (int)*(short *)(&DAT_6bc995c6 + (longlong)(int)(uint)param_2 * 0x2a8),
                       (int)*(short *)(&DAT_6bc995cc + (longlong)(int)(uint)param_2 * 0x2a8),
                       (int)*(short *)(&DAT_6bc995ce + (longlong)(int)(uint)param_2 * 0x2a8),
                       (int)*(short *)(_refptr_g_touchesSide +
                                      (longlong)(int)(uint)param_1 * 0x168 + 0x172));
    *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)param_1 * 0x168 + 0x29a) = uVar1;
    *(short *)(_refptr_g_touchesSide + (longlong)(int)(uint)param_1 * 0x168 + 0x29c) =
         (short)(((int)*(short *)(&DAT_6bc995d2 + (longlong)(int)(uint)param_2 * 0x2a8) +
                 (int)*(short *)(&DAT_6bc995d0 + (longlong)(int)(uint)param_2 * 0x2a8)) / 2);
  }
  else {
    *(short *)(_refptr_g_touchesSide + (longlong)(int)(uint)param_1 * 0x168 + 0x29a) =
         (short)(((int)*(short *)(&DAT_6bc995ce + (longlong)(int)(uint)param_2 * 0x2a8) +
                 (int)*(short *)(&DAT_6bc995cc + (longlong)(int)(uint)param_2 * 0x2a8)) / 2);
    uVar1 = SideTouch_GetMappedCorePos
                      ((int)*(short *)(&DAT_6bc995c4 + (longlong)(int)(uint)param_2 * 0x2a8),
                       (int)*(short *)(&DAT_6bc995c6 + (longlong)(int)(uint)param_2 * 0x2a8),
                       (int)*(short *)(&DAT_6bc995d0 + (longlong)(int)(uint)param_2 * 0x2a8),
                       (int)*(short *)(&DAT_6bc995d2 + (longlong)(int)(uint)param_2 * 0x2a8),
                       (int)*(short *)(_refptr_g_touchesSide +
                                      (longlong)(int)(uint)param_1 * 0x168 + 0x172));
    *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)param_1 * 0x168 + 0x29c) = uVar1;
  }
  return;
}


// ============================================================================
// Function: SideTouch_GestureFailureRecord @ 6bb14744
// ============================================================================

void SideTouch_GestureFailureRecord(byte param_1,undefined4 param_2,undefined4 param_3)

{
  byte bVar1;
  byte bVar2;
  char cVar3;
  
                    /* 0xd4744  1347  SideTouch_GestureFailureRecord */
  bVar1 = (&DAT_6bc997d9)[(longlong)(int)(uint)param_1 * 0x2a8];
  SideTouch_TouchFailureRecord(bVar1,param_2,param_3);
  if (bVar1 < 0x14) {
    bVar2 = *(byte *)((longlong)(int)(uint)bVar1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x287
                     );
    if (bVar2 < 0x14) {
      cVar3 = SideGesture_IsVolumeDoubleClickTrigger(param_1);
      if (cVar3 != '\0') {
        *(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar2 * 0x478 + 0x588)
             = *(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                        (longlong)(int)(uint)bVar2 * 0x478 + 0x588) |
               *(uint *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_1 * 0x2a8);
        if ((*(uint *)((longlong)(int)(uint)bVar1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x1fc) & 0x40) == 0) {
          SideTouch_AntiTriggerRecord(bVar1);
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: SideTouch_DetectOnAction @ 6bb14e5b
// ============================================================================

uint SideTouch_DetectOnAction(byte param_1,byte param_2,byte param_3,byte param_4)

{
  short sVar1;
  short sVar2;
  short sVar3;
  uint uVar4;
  uint uVar5;
  longlong lVar6;
  undefined4 local_28;
  uint local_c;
  
  sVar2 = SideTouch_ActionDetectedGetStartPDim2(param_2,param_3);
  sVar3 = (short)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
          (short)*(undefined8 *)
                  ((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_2 * 0x478 + 0x4b8);
  sVar2 = *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x172) - sVar2;
  sVar1 = *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x228);
  lVar6 = (longlong)(int)(uint)param_4 * 0x30 + (longlong)(int)(uint)param_3 * 0x2a8;
  local_c = 0;
  *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x294) =
       sVar3;
  *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x296) =
       sVar2;
  *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x298) =
       sVar1;
  *(short *)(&DAT_6bc997dc + (longlong)(int)(uint)param_3 * 0x2a8) = sVar2;
  if (0x14 < *(ushort *)
              ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x252)) {
    local_c = 2;
    SideTouch_GestureFailureRecord
              (param_3,8,
               *(undefined2 *)
                ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x252));
  }
  if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) &
      0x2000000) != 0) {
    local_c = 2;
    SideTouch_GestureFailureRecord(param_3,9,0);
  }
  if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1fc) &
      4) != 0) {
    local_c = 2;
    SideTouch_GestureFailureRecord(param_3,0xb,0);
  }
  uVar4 = SideTouch_CheckOnTime
                    ((int)sVar3,(int)*(short *)(&DAT_6bc995e4 + lVar6),
                     (int)*(short *)(&DAT_6bc995e6 + lVar6),param_3);
  if (uVar4 <= local_c) {
    uVar4 = local_c;
  }
  local_28 = CONCAT22(*(undefined2 *)(&DAT_6bc995ec + lVar6),*(undefined2 *)(&DAT_6bc995e8 + lVar6))
  ;
  uVar5 = SideTouch_CheckOnMove
                    ((int)sVar3,(int)*(short *)(&DAT_6bc995ea + lVar6),(int)sVar2,local_28,param_3);
  if (uVar5 <= uVar4) {
    uVar5 = uVar4;
  }
  uVar4 = SideTouch_CheckOnTouch
                    ((int)sVar1,(int)*(short *)(&DAT_6bc995ee + lVar6),
                     (int)*(short *)(&DAT_6bc995f0 + lVar6),param_3);
  if (uVar4 <= uVar5) {
    uVar4 = uVar5;
  }
  *(int *)(&DAT_6bc995fc + lVar6) = (int)sVar3;
  if (*(ushort *)(&DAT_6bc995f4 + lVar6) <
      *(ushort *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172))
  {
    *(undefined2 *)(&DAT_6bc995f4 + lVar6) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
  }
  if (*(ushort *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172)
      < *(ushort *)(&DAT_6bc995f2 + lVar6)) {
    *(undefined2 *)(&DAT_6bc995f2 + lVar6) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
  }
  if (*(int *)(&DAT_6bc995f8 + lVar6) <
      (int)*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1be)) {
    *(int *)(&DAT_6bc995f8 + lVar6) =
         (int)*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x1be);
  }
  if ((int)(uint)*(ushort *)(&DAT_6bc995f6 + lVar6) < (int)sVar1) {
    *(short *)(&DAT_6bc995f6 + lVar6) = sVar1;
  }
  return uVar4;
}


// ============================================================================
// Function: SideTouch_IsInCore @ 6bb1ceae
// ============================================================================

undefined8 SideTouch_IsInCore(undefined1 param_1,byte param_2)

{
  ushort uVar1;
  char cVar2;
  ushort uVar3;
  
  if (((*(uint *)(_refptr_g_touchesGrid + (longlong)(int)(uint)param_2 * 0x168 + 0x218) & 1) != 0)
     && (cVar2 = GripFilter_IsPeakAbsGrip
                           (_refptr_g_touchesGrid[(longlong)(int)(uint)param_2 * 0x168 + 0x23c]),
        cVar2 != '\0')) {
    return 0;
  }
  if (((*(ushort *)(_refptr_g_touchesGrid + (longlong)(int)(uint)param_2 * 0x168 + 0x22c) <=
        *(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x630) &&
      (cVar2 = SideTouch_IsGripRatioTrustable(param_1), cVar2 != '\0')) &&
     (uVar1 = *(ushort *)(_refptr_g_touchesGrid + (longlong)(int)(uint)param_2 * 0x168 + 0x1a8),
     uVar3 = SideTouch_GetGripRatio
                       (param_2,_refptr_g_touchesGrid[(longlong)(int)(uint)param_2 * 0x168 + 0x287],
                        param_1), uVar3 < uVar1)) {
    return 0;
  }
  return 1;
}


// ============================================================================
// Function: SideTouch_IsSideOverCore @ 6bb1cfdd
// ============================================================================

undefined8 SideTouch_IsSideOverCore(undefined1 param_1,byte param_2)

{
  char cVar1;
  undefined8 uVar2;
  
  cVar1 = SideGesture_IsSideOverCore(param_1);
  if ((cVar1 == '\0') ||
     (*(ushort *)(_refptr_g_touchesGrid + (longlong)(int)(uint)param_2 * 0x168 + 0x1a8) <=
      (ushort)(byte)_refptr_g_tsaPrmtFlash[2].field_0x62c)) {
    if ((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x624 & 0x10) == 0) {
      uVar2 = 0;
    }
    else if ((*(uint *)(_refptr_g_touchesGrid + (longlong)(int)(uint)param_2 * 0x168 + 0x1f0) &
             0x8000000) == 0) {
      if ((*(uint *)(_refptr_g_touchesGrid + (longlong)(int)(uint)param_2 * 0x168 + 0x1f8) &
          0x100000) == 0) {
        uVar2 = 1;
      }
      else {
        uVar2 = 0;
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
// Function: SideTouch_GestureVolumeTriggerFlickAction @ 6bb0f3a4
// ============================================================================

void SideTouch_GestureVolumeTriggerFlickAction(byte param_1,int param_2,char param_3)

{
  short sVar1;
  undefined2 uVar2;
  errno_t eVar3;
  
  if (param_3 == '\0') {
    eVar3 = strcpy_s((char *)((longlong)(int)(uint)param_1 * 0x2a8 + 0x6bc9958c),0x32,
                     "VolumeTriggerMove");
    if (eVar3 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/SideTouch/SideTouch_Gesture.c",0x2ee,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
    *(undefined2 *)(&DAT_6bc995ea + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  }
  else {
    eVar3 = strcpy_s((char *)((longlong)(int)(uint)param_1 * 0x2a8 + 0x6bc9958c),0x32,
                     "VolumeTriggerFlick");
    if (eVar3 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/SideTouch/SideTouch_Gesture.c",0x2ea,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
    *(undefined2 *)(&DAT_6bc995ea + (longlong)(int)(uint)param_1 * 0x2a8) =
         *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x652;
  }
  *(undefined4 *)(&DAT_6bc995dc + (longlong)(int)(uint)param_1 * 0x2a8) = 0;
  (&DAT_6bc995d8)[(longlong)(int)(uint)param_1 * 0x2a8] = 2;
  *(undefined4 *)(&DAT_6bc995e0 + (longlong)(int)(uint)param_1 * 0x2a8) = 1;
  *(undefined2 *)(&DAT_6bc995e4 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x654;
  *(undefined2 *)(&DAT_6bc995e6 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc995ec + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc995ee + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x64e;
  *(undefined2 *)(&DAT_6bc995f0 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x650;
  *(undefined4 *)(&DAT_6bc99610 + (longlong)(int)(uint)param_1 * 0x2a8) = 1;
  *(undefined2 *)(&DAT_6bc99614 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc99616 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc99618 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc9961c + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9961e + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x64e;
  *(undefined2 *)(&DAT_6bc99620 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x650;
  if (param_2 == 1) {
    sVar1 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x63c);
    *(short *)(&DAT_6bc995e8 + (longlong)(int)(uint)param_1 * 0x2a8) = -sVar1;
  }
  else {
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x63e);
    *(undefined2 *)(&DAT_6bc995e8 + (longlong)(int)(uint)param_1 * 0x2a8) = uVar2;
  }
  return;
}


// ============================================================================
// Function: SideTouch_InternalGestureVolumeTriggerFlick @ 6bb0f7ba
// ============================================================================

void SideTouch_InternalGestureVolumeTriggerFlick(int param_1,int param_2,char param_3,byte param_4)

{
  if (param_3 == '\0') {
    if (param_1 == 4) {
      if (param_2 == 1) {
        *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_4 * 0x2a8) = 2;
        SideTouch_SetGestureEnabled(2);
      }
      else {
        *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_4 * 0x2a8) = 4;
        SideTouch_SetGestureEnabled(4);
      }
    }
    else if (param_2 == 1) {
      *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_4 * 0x2a8) = 8;
      SideTouch_SetGestureEnabled(8);
    }
    else {
      *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_4 * 0x2a8) = 0x10;
      SideTouch_SetGestureEnabled(0x10);
    }
  }
  else if (param_1 == 4) {
    if (param_2 == 1) {
      *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_4 * 0x2a8) = 0x20;
      SideTouch_SetGestureEnabled(0x20);
    }
    else {
      *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_4 * 0x2a8) = 0x40;
      SideTouch_SetGestureEnabled(0x40);
    }
  }
  else if (param_2 == 1) {
    *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_4 * 0x2a8) = 0x80;
    SideTouch_SetGestureEnabled(0x80);
  }
  else {
    *(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_4 * 0x2a8) = 0x100;
    SideTouch_SetGestureEnabled(0x100);
  }
  return;
}


// ============================================================================
// Function: SideTouch_GestureVolumeTriggerDoubleFlickAction @ 6bb112d9
// ============================================================================

void SideTouch_GestureVolumeTriggerDoubleFlickAction(byte param_1,int param_2)

{
  short sVar1;
  undefined2 uVar2;
  
  *(undefined4 *)(&DAT_6bc995dc + (longlong)(int)(uint)param_1 * 0x2a8) = 0;
  (&DAT_6bc995d8)[(longlong)(int)(uint)param_1 * 0x2a8] = 5;
  *(undefined4 *)(&DAT_6bc995e0 + (longlong)(int)(uint)param_1 * 0x2a8) = 1;
  *(undefined2 *)(&DAT_6bc995e4 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x654;
  *(undefined2 *)(&DAT_6bc995e6 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc995ea + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x652;
  *(undefined2 *)(&DAT_6bc995ec + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc995ee + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x64e;
  *(undefined2 *)(&DAT_6bc995f0 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x650;
  *(undefined4 *)(&DAT_6bc99610 + (longlong)(int)(uint)param_1 * 0x2a8) = 1;
  *(undefined2 *)(&DAT_6bc99614 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc99616 + (longlong)(int)(uint)param_1 * 0x2a8) = 1000;
  *(undefined2 *)(&DAT_6bc9961a + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9961c + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9961e + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x64e;
  *(undefined2 *)(&DAT_6bc99620 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x650;
  *(undefined4 *)(&DAT_6bc99640 + (longlong)(int)(uint)param_1 * 0x2a8) = 0;
  *(undefined2 *)(&DAT_6bc99644 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x656;
  *(undefined2 *)(&DAT_6bc99646 + (longlong)(int)(uint)param_1 * 0x2a8) = 1000;
  *(undefined2 *)(&DAT_6bc99648 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc9964a + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9964c + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9964e + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc99650 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x650;
  *(undefined4 *)(&DAT_6bc99670 + (longlong)(int)(uint)param_1 * 0x2a8) = 1;
  *(undefined2 *)(&DAT_6bc99674 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x654;
  *(undefined2 *)(&DAT_6bc99676 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9967a + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x652;
  *(undefined2 *)(&DAT_6bc9967c + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9967e + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x64e;
  *(undefined2 *)(&DAT_6bc99680 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x650;
  *(undefined4 *)(&DAT_6bc996a0 + (longlong)(int)(uint)param_1 * 0x2a8) = 1;
  *(undefined2 *)(&DAT_6bc996a4 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc996a6 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc996a8 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc996ac + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc996ae + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x64e;
  *(undefined2 *)(&DAT_6bc996b0 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x650;
  if (param_2 == 1) {
    sVar1 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x63c);
    *(short *)(&DAT_6bc995e8 + (longlong)(int)(uint)param_1 * 0x2a8) = -sVar1;
    sVar1 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x63c);
    *(short *)(&DAT_6bc99618 + (longlong)(int)(uint)param_1 * 0x2a8) = -sVar1;
    sVar1 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x63c);
    *(short *)(&DAT_6bc99678 + (longlong)(int)(uint)param_1 * 0x2a8) = -sVar1;
  }
  else {
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x63e);
    *(undefined2 *)(&DAT_6bc995e8 + (longlong)(int)(uint)param_1 * 0x2a8) = uVar2;
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x63e);
    *(undefined2 *)(&DAT_6bc99618 + (longlong)(int)(uint)param_1 * 0x2a8) = uVar2;
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x63e);
    *(undefined2 *)(&DAT_6bc99678 + (longlong)(int)(uint)param_1 * 0x2a8) = uVar2;
  }
  return;
}


// ============================================================================
// Function: SideTouch_GestureVolumeMuteAction @ 6bb120b8
// ============================================================================

void SideTouch_GestureVolumeMuteAction(byte param_1,int param_2)

{
  short sVar1;
  undefined2 uVar2;
  
  *(undefined4 *)(&DAT_6bc995dc + (longlong)(int)(uint)param_1 * 0x2a8) = 1;
  (&DAT_6bc995d8)[(longlong)(int)(uint)param_1 * 0x2a8] = 6;
  *(undefined4 *)(&DAT_6bc995e0 + (longlong)(int)(uint)param_1 * 0x2a8) = 1;
  *(undefined2 *)(&DAT_6bc995e4 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x654;
  *(undefined2 *)(&DAT_6bc995e6 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc995ea + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x652;
  *(undefined2 *)(&DAT_6bc995ec + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc995ee + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x64e;
  *(undefined2 *)(&DAT_6bc995f0 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x650;
  *(undefined4 *)(&DAT_6bc99610 + (longlong)(int)(uint)param_1 * 0x2a8) = 1;
  *(undefined2 *)(&DAT_6bc99614 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc99616 + (longlong)(int)(uint)param_1 * 0x2a8) = 1000;
  *(undefined2 *)(&DAT_6bc9961a + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9961c + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9961e + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x64e;
  *(undefined2 *)(&DAT_6bc99620 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x650;
  *(undefined4 *)(&DAT_6bc99640 + (longlong)(int)(uint)param_1 * 0x2a8) = 0;
  *(undefined2 *)(&DAT_6bc99644 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x656;
  *(undefined2 *)(&DAT_6bc99646 + (longlong)(int)(uint)param_1 * 0x2a8) = 1000;
  *(undefined2 *)(&DAT_6bc99648 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc9964a + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9964c + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9964e + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc99650 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x650;
  *(undefined4 *)(&DAT_6bc99670 + (longlong)(int)(uint)param_1 * 0x2a8) = 1;
  *(undefined2 *)(&DAT_6bc99674 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x654;
  *(undefined2 *)(&DAT_6bc99676 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9967a + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x652;
  *(undefined2 *)(&DAT_6bc9967c + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9967e + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x64e;
  *(undefined2 *)(&DAT_6bc99680 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x650;
  *(undefined4 *)(&DAT_6bc996a0 + (longlong)(int)(uint)param_1 * 0x2a8) = 1;
  *(undefined2 *)(&DAT_6bc996a4 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc996a6 + (longlong)(int)(uint)param_1 * 0x2a8) = 1000;
  *(undefined2 *)(&DAT_6bc996aa + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc996ac + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc996ae + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x64e;
  *(undefined2 *)(&DAT_6bc996b0 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x650;
  *(undefined4 *)(&DAT_6bc996d0 + (longlong)(int)(uint)param_1 * 0x2a8) = 0;
  *(undefined2 *)(&DAT_6bc996d4 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x656;
  *(undefined2 *)(&DAT_6bc996d6 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc996d8 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc996da + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc996dc + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc996de + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc996e0 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x650;
  if (param_2 == 1) {
    sVar1 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x63c);
    *(short *)(&DAT_6bc995e8 + (longlong)(int)(uint)param_1 * 0x2a8) = -sVar1;
    sVar1 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x63c);
    *(short *)(&DAT_6bc99618 + (longlong)(int)(uint)param_1 * 0x2a8) = -sVar1;
    sVar1 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x63c);
    *(short *)(&DAT_6bc99678 + (longlong)(int)(uint)param_1 * 0x2a8) = -sVar1;
    sVar1 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x63c);
    *(short *)(&DAT_6bc996a8 + (longlong)(int)(uint)param_1 * 0x2a8) = -sVar1;
  }
  else {
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x63e);
    *(undefined2 *)(&DAT_6bc995e8 + (longlong)(int)(uint)param_1 * 0x2a8) = uVar2;
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x63e);
    *(undefined2 *)(&DAT_6bc99618 + (longlong)(int)(uint)param_1 * 0x2a8) = uVar2;
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x63e);
    *(undefined2 *)(&DAT_6bc99678 + (longlong)(int)(uint)param_1 * 0x2a8) = uVar2;
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x63e);
    *(undefined2 *)(&DAT_6bc996a8 + (longlong)(int)(uint)param_1 * 0x2a8) = uVar2;
  }
  return;
}


// ============================================================================
// Function: SideTouch_GestureDoubleClickAction @ 6bb1346d
// ============================================================================

void SideTouch_GestureDoubleClickAction(byte param_1)

{
  short sVar1;
  undefined2 uVar2;
  
  *(undefined4 *)(&DAT_6bc995dc + (longlong)(int)(uint)param_1 * 0x2a8) = 1;
  (&DAT_6bc995d8)[(longlong)(int)(uint)param_1 * 0x2a8] = 4;
  *(undefined4 *)(&DAT_6bc995e0 + (longlong)(int)(uint)param_1 * 0x2a8) = 1;
  *(undefined2 *)(&DAT_6bc995e4 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x6aa;
  *(undefined2 *)(&DAT_6bc995e6 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x6ac;
  sVar1 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x6a8);
  *(short *)(&DAT_6bc995e8 + (longlong)(int)(uint)param_1 * 0x2a8) = -sVar1;
  *(undefined2 *)(&DAT_6bc995ea + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x6a8);
  *(undefined2 *)(&DAT_6bc995ec + (longlong)(int)(uint)param_1 * 0x2a8) = uVar2;
  *(undefined2 *)(&DAT_6bc995ee + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x6b2;
  *(undefined2 *)(&DAT_6bc995f0 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x6b4;
  *(undefined4 *)(&DAT_6bc99610 + (longlong)(int)(uint)param_1 * 0x2a8) = 0;
  *(undefined2 *)(&DAT_6bc99614 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x6ae;
  *(undefined2 *)(&DAT_6bc99616 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x6b0;
  *(undefined2 *)(&DAT_6bc99618 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc9961a + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9961c + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9961e + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc99620 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x6b4;
  *(undefined4 *)(&DAT_6bc99640 + (longlong)(int)(uint)param_1 * 0x2a8) = 1;
  *(undefined2 *)(&DAT_6bc99644 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x6aa;
  *(undefined2 *)(&DAT_6bc99646 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x6ac;
  sVar1 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x6a8);
  *(short *)(&DAT_6bc99648 + (longlong)(int)(uint)param_1 * 0x2a8) = -sVar1;
  *(undefined2 *)(&DAT_6bc9964a + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x6a8);
  *(undefined2 *)(&DAT_6bc9964c + (longlong)(int)(uint)param_1 * 0x2a8) = uVar2;
  *(undefined2 *)(&DAT_6bc9964e + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x6b2;
  *(undefined2 *)(&DAT_6bc99650 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x6b4;
  *(undefined4 *)(&DAT_6bc99670 + (longlong)(int)(uint)param_1 * 0x2a8) = 0;
  *(undefined2 *)(&DAT_6bc99674 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x32;
  *(undefined2 *)(&DAT_6bc99676 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc99678 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc9967a + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9967c + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9967e + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc99680 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x6b4;
  return;
}


// ============================================================================
// Function: SideTouch_GestureVolumeTriggerClickFlickAction @ 6bb0fb86
// ============================================================================

void SideTouch_GestureVolumeTriggerClickFlickAction(byte param_1,int param_2)

{
  short sVar1;
  undefined2 uVar2;
  
  *(undefined4 *)(&DAT_6bc995dc + (longlong)(int)(uint)param_1 * 0x2a8) = 0;
  (&DAT_6bc995d8)[(longlong)(int)(uint)param_1 * 0x2a8] = 4;
  *(undefined4 *)(&DAT_6bc995e0 + (longlong)(int)(uint)param_1 * 0x2a8) = 1;
  *(undefined2 *)(&DAT_6bc995e4 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x6aa;
  *(undefined2 *)(&DAT_6bc995e6 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x6ac;
  sVar1 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x6a8);
  *(short *)(&DAT_6bc995e8 + (longlong)(int)(uint)param_1 * 0x2a8) = sVar1 * -3;
  *(undefined2 *)(&DAT_6bc995ea + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  sVar1 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x6a8);
  *(short *)(&DAT_6bc995ec + (longlong)(int)(uint)param_1 * 0x2a8) = sVar1 * 3;
  *(undefined2 *)(&DAT_6bc995ee + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x64e;
  *(undefined2 *)(&DAT_6bc995f0 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x650;
  *(undefined4 *)(&DAT_6bc99610 + (longlong)(int)(uint)param_1 * 0x2a8) = 0;
  *(undefined2 *)(&DAT_6bc99614 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x6ae;
  *(undefined2 *)(&DAT_6bc99616 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x6b0;
  *(undefined2 *)(&DAT_6bc99618 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc9961a + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9961c + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9961e + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc99620 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x650;
  *(undefined4 *)(&DAT_6bc99640 + (longlong)(int)(uint)param_1 * 0x2a8) = 1;
  *(undefined2 *)(&DAT_6bc99644 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x654;
  *(undefined2 *)(&DAT_6bc99646 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9964a + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x652;
  *(undefined2 *)(&DAT_6bc9964c + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9964e + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x64e;
  *(undefined2 *)(&DAT_6bc99650 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x650;
  *(undefined4 *)(&DAT_6bc99670 + (longlong)(int)(uint)param_1 * 0x2a8) = 1;
  *(undefined2 *)(&DAT_6bc99674 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc99676 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9967a + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc99678 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc9967c + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9967e + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x64e;
  *(undefined2 *)(&DAT_6bc99680 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x650;
  if (param_2 == 1) {
    sVar1 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x63c);
    *(short *)(&DAT_6bc99648 + (longlong)(int)(uint)param_1 * 0x2a8) = -sVar1;
  }
  else {
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x63e);
    *(undefined2 *)(&DAT_6bc99648 + (longlong)(int)(uint)param_1 * 0x2a8) = uVar2;
  }
  return;
}


// ============================================================================
// Function: SideTouch_GestureVolumeTouchAction @ 6bb10aef
// ============================================================================

void SideTouch_GestureVolumeTouchAction(byte param_1)

{
  (&DAT_6bc995d8)[(longlong)(int)(uint)param_1 * 0x2a8] = 3;
  *(undefined4 *)(&DAT_6bc995e0 + (longlong)(int)(uint)param_1 * 0x2a8) = 1;
  *(undefined2 *)(&DAT_6bc995e4 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc995e6 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc995e8 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc995ea + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc995ec + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc995ee + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc995f0 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined4 *)(&DAT_6bc99610 + (longlong)(int)(uint)param_1 * 0x2a8) = 1;
  *(undefined2 *)(&DAT_6bc99614 + (longlong)(int)(uint)param_1 * 0x2a8) =
       *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x652;
  *(undefined2 *)(&DAT_6bc99616 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc99618 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc9961a + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9961c + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9961e + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc99620 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined4 *)(&DAT_6bc99640 + (longlong)(int)(uint)param_1 * 0x2a8) = 1;
  *(undefined2 *)(&DAT_6bc99644 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc99646 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc99648 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc9964a + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9964c + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  *(undefined2 *)(&DAT_6bc9964e + (longlong)(int)(uint)param_1 * 0x2a8) = 0x8000;
  *(undefined2 *)(&DAT_6bc99650 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x7fff;
  return;
}


// ============================================================================
// Function: SideTouch_BaselineDoUpdateUnit @ 6bb0bc6c
// ============================================================================

void SideTouch_BaselineDoUpdateUnit(int param_1,int *param_2,short param_3)

{
  if (param_1 - *param_2 < (int)param_3) {
    if (param_1 - *param_2 <= -(int)param_3) {
      *param_2 = *param_2 - (int)param_3;
    }
  }
  else {
    *param_2 = *param_2 + (int)param_3;
  }
  return;
}


// ============================================================================
// Function: SideTouch_BaselineResetCheck @ 6bb0ca44
// ============================================================================

undefined1 SideTouch_BaselineResetCheck(void)

{
  char cVar1;
  undefined1 local_9;
  
  local_9 = 0;
  cVar1 = BLReset_CheckNumOverNoise();
  if (((cVar1 != '\0') &&
      (*(int *)(g_baselineData + 0x1064) <
       -(int)((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 2))) &&
     (_refptr_g_tsaPrpt->dwBaselineStage == 0)) {
    local_9 = 1;
  }
  return local_9;
}


// ============================================================================
// Function: SideTouch_AntiTriggerStateProcess @ 6bb0b330
// ============================================================================

void SideTouch_AntiTriggerStateProcess(byte param_1)

{
  byte bVar1;
  
  bVar1 = SideTouch_GetNewNumInTime(1000,600,0);
  if (1 < bVar1) {
    *(uint *)(_refptr_g_sideGesture + (longlong)(int)(uint)param_1 * 0x2a8 + 0x24c) =
         *(uint *)(_refptr_g_sideGesture + (longlong)(int)(uint)param_1 * 0x2a8 + 0x24c) | 1;
  }
  bVar1 = _refptr_g_sideGesture[(longlong)(int)(uint)param_1 * 0x2a8 + 0x259];
  if (bVar1 < 0x14) {
    if ((*(uint *)((longlong)(int)(uint)bVar1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1fc) &
        0x80) != 0) {
      *(uint *)(_refptr_g_sideGesture + (longlong)(int)(uint)param_1 * 0x2a8 + 0x24c) =
           *(uint *)(_refptr_g_sideGesture + (longlong)(int)(uint)param_1 * 0x2a8 + 0x24c) | 4;
    }
    if ((*(uint *)((longlong)(int)(uint)bVar1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1fc) &
        0x100) != 0) {
      *(uint *)(_refptr_g_sideGesture + (longlong)(int)(uint)param_1 * 0x2a8 + 0x24c) =
           *(uint *)(_refptr_g_sideGesture + (longlong)(int)(uint)param_1 * 0x2a8 + 0x24c) | 8;
    }
  }
  return;
}


// ============================================================================
// Function: SideTouch_IsToResetTrigger @ 6bb0b8c1
// ============================================================================

undefined1 SideTouch_IsToResetTrigger(undefined1 param_1)

{
  char cVar1;
  byte bVar2;
  undefined1 local_9;
  
                    /* 0xcb8c1  1383  SideTouch_IsToResetTrigger */
  local_9 = 0;
  SideTouch_AntiTriggerPrmtUpdate();
  if (5000 < (ulonglong)
             ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
             (longlong)timeStampAntiTrigger_10604)) {
    cVar1 = SideTouch_IsToCheckTrigger(param_1);
    if (cVar1 != '\0') {
      bVar2 = SideTouch_TriggerPassLevel2Check(param_1);
      if (1 < bVar2) {
        local_9 = 1;
        timeStampAntiTrigger_10604 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
      }
    }
  }
  return local_9;
}


// ============================================================================
// Function: SideTouch_GestureLogStaticsUpdate @ 6bb1b04d
// ============================================================================

void SideTouch_GestureLogStaticsUpdate(byte param_1,byte param_2)

{
  ushort uVar1;
  PrevTouchArray *pPVar2;
  undefined2 uVar3;
  ushort uVar4;
  
  if (*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c) <
      3) {
    *(undefined2 *)
     ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x528) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x18c);
    *(undefined2 *)
     ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x52a) =
         *(undefined2 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x18c);
    pPVar2 = *_refptr_g_prevTouchesPtr;
    uVar3 = TSAOut_DisplayXPos(param_1);
    *(undefined2 *)((longlong)pPVar2 + (longlong)(int)(uint)param_2 * 0x478 + 0x52c) = uVar3;
    pPVar2 = *_refptr_g_prevTouchesPtr;
    uVar3 = TSAOut_DisplayXPos(param_1);
    *(undefined2 *)((longlong)pPVar2 + (longlong)(int)(uint)param_2 * 0x478 + 0x52e) = uVar3;
    pPVar2 = *_refptr_g_prevTouchesPtr;
    uVar3 = TSAOut_DisplayYPos(param_1);
    *(undefined2 *)((longlong)pPVar2 + (longlong)(int)(uint)param_2 * 0x478 + 0x530) = uVar3;
    pPVar2 = *_refptr_g_prevTouchesPtr;
    uVar3 = TSAOut_DisplayYPos(param_1);
    *(undefined2 *)((longlong)pPVar2 + (longlong)(int)(uint)param_2 * 0x478 + 0x532) = uVar3;
    *(ushort *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x534)
         = (ushort)*(byte *)((longlong)(int)(uint)param_1 * 0x168 +
                             *(longlong *)_refptr_g_touchesPtr + 0x198);
    *(ushort *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x536)
         = (ushort)*(byte *)((longlong)(int)(uint)param_1 * 0x168 +
                             *(longlong *)_refptr_g_touchesPtr + 0x198);
  }
  else {
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
         | 0x8000000;
    if (*(ushort *)
         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x18c) <
        *(ushort *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x528)) {
      *(undefined2 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x528) =
           *(undefined2 *)
            ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x18c);
    }
    uVar1 = *(ushort *)
             ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x52c);
    uVar4 = TSAOut_DisplayXPos(param_1);
    if (uVar4 < uVar1) {
      pPVar2 = *_refptr_g_prevTouchesPtr;
      uVar3 = TSAOut_DisplayXPos(param_1);
      *(undefined2 *)((longlong)pPVar2 + (longlong)(int)(uint)param_2 * 0x478 + 0x52c) = uVar3;
    }
    uVar1 = *(ushort *)
             ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x530);
    uVar4 = TSAOut_DisplayYPos(param_1);
    if (uVar4 < uVar1) {
      pPVar2 = *_refptr_g_prevTouchesPtr;
      uVar3 = TSAOut_DisplayYPos(param_1);
      *(undefined2 *)((longlong)pPVar2 + (longlong)(int)(uint)param_2 * 0x478 + 0x530) = uVar3;
    }
    if ((ushort)*(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x198) <
        *(ushort *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x534)) {
      *(ushort *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x534) =
           (ushort)*(byte *)((longlong)(int)(uint)param_1 * 0x168 +
                             *(longlong *)_refptr_g_touchesPtr + 0x198);
    }
    if (*(ushort *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x52a) <
        *(ushort *)
         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x18c)) {
      *(undefined2 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x52a) =
           *(undefined2 *)
            ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x18c);
    }
    uVar1 = *(ushort *)
             ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x52e);
    uVar4 = TSAOut_DisplayXPos(param_1);
    if (uVar1 < uVar4) {
      pPVar2 = *_refptr_g_prevTouchesPtr;
      uVar3 = TSAOut_DisplayXPos(param_1);
      *(undefined2 *)((longlong)pPVar2 + (longlong)(int)(uint)param_2 * 0x478 + 0x52e) = uVar3;
    }
    uVar1 = *(ushort *)
             ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x532);
    uVar4 = TSAOut_DisplayYPos(param_1);
    if (uVar1 < uVar4) {
      pPVar2 = *_refptr_g_prevTouchesPtr;
      uVar3 = TSAOut_DisplayYPos(param_1);
      *(undefined2 *)((longlong)pPVar2 + (longlong)(int)(uint)param_2 * 0x478 + 0x532) = uVar3;
    }
    if (*(ushort *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x536) <
        (ushort)*(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x198)) {
      *(ushort *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x536) =
           (ushort)*(byte *)((longlong)(int)(uint)param_1 * 0x168 +
                             *(longlong *)_refptr_g_touchesPtr + 0x198);
    }
  }
  return;
}


// ============================================================================
// Function: SideTouch_IsGestureJustActive @ 6bb0ebd5
// ============================================================================

bool SideTouch_IsGestureJustActive(byte param_1)

{
                    /* 0xcebd5  1373  SideTouch_IsGestureJustActive */
  return *(int *)(&DAT_6bc997c4 + (longlong)(int)(uint)param_1 * 0x2a8) == 1;
}


// ============================================================================
// Function: SideTouch_GetMappedCorePos @ 6bb16341
// ============================================================================

int SideTouch_GetMappedCorePos
              (short param_1,short param_2,short param_3,short param_4,short param_5)

{
  int local_c;
  
  local_c = ((int)param_5 - (int)param_1) * (((int)param_4 - (int)param_3) + 1);
  if ((int)param_2 - (int)param_1 != -1) {
    local_c = local_c / (((int)param_2 - (int)param_1) + 1);
  }
  local_c = local_c + param_3;
  if (local_c < 0) {
    local_c = 0;
  }
  return local_c;
}


// ============================================================================
// Function: SideTouch_AntiTriggerRecord @ 6bb0790e
// ============================================================================

void SideTouch_AntiTriggerRecord(byte param_1)

{
  byte local_a;
  byte local_9;
  
                    /* 0xc790e  1332  SideTouch_AntiTriggerRecord */
  if (param_1 < 0x14) {
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1fc) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1fc)
         | 0x40;
    if (*(ushort *)
         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170) < 0x100)
    {
      for (local_9 = *(byte *)((longlong)(int)(uint)param_1 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x17e);
          local_9 <=
          *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x17f
                   ); local_9 = local_9 + 1) {
        if (*(void **)(&DAT_6bc97188 + ((longlong)(int)(uint)local_9 + 1) * 0xc) !=
            (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr) {
          *(void **)(&DAT_6bc97188 + ((longlong)(int)(uint)local_9 + 1) * 0xc) =
               (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
          (&g_sideData)[((longlong)(int)(uint)local_9 + 1) * 0x30] =
               (&g_sideData)[((longlong)(int)(uint)local_9 + 1) * 0x30] + '\x01';
        }
      }
    }
    else if ((int)(*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x20) - 0x100) <
             (int)(uint)*(ushort *)
                         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x170)) {
      for (local_a = *(byte *)((longlong)(int)(uint)param_1 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x17e);
          local_a <=
          *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x17f
                   ); local_a = local_a + 1) {
        if (*(void **)(&DAT_6bc98240 + (longlong)(int)(uint)local_a * 0x30) !=
            (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr) {
          *(void **)(&DAT_6bc98240 + (longlong)(int)(uint)local_a * 0x30) =
               (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
          (&DAT_6bc98238)[(longlong)(int)(uint)local_a * 0x30] =
               (&DAT_6bc98238)[(longlong)(int)(uint)local_a * 0x30] + '\x01';
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: SideTouch_CheckOnTime @ 6bb14884
// ============================================================================

undefined4 SideTouch_CheckOnTime(short param_1,short param_2,short param_3,byte param_4)

{
  undefined4 local_c;
  
  local_c = 0;
  if ((param_2 != -0x8000) && (param_1 < param_2)) {
    local_c = 1;
  }
  if (param_3 != 0x7fff) {
    if (param_3 < param_1) {
      local_c = 2;
      SideTouch_GestureFailureRecord(param_4,2,(int)param_1);
    }
    else if (*(int *)((longlong)
                      (int)(uint)(byte)(&DAT_6bc997d9)[(longlong)(int)(uint)param_4 * 0x2a8] * 0x168
                      + *(longlong *)_refptr_g_touchesPtr + 0x21c) != 0x20) {
      local_c = 1;
    }
  }
  return local_c;
}


// ============================================================================
// Function: SideTouch_CheckOnMove @ 6bb1494b
// ============================================================================

undefined4 SideTouch_CheckOnMove(short param_1,short param_2,short param_3,int param_4,byte param_5)

{
  short local_res20;
  short sStackX_22;
  undefined4 local_c;
  
  local_c = 0;
  local_res20 = (short)param_4;
  sStackX_22 = (short)((uint)param_4 >> 0x10);
  if ((((local_res20 == -0x8000) || (sStackX_22 == 0x7fff)) || (-1 < local_res20)) ||
     (sStackX_22 < 1)) {
    if ((local_res20 != -0x8000) &&
       ((*(uint *)(&DAT_6bc997c8 + (longlong)(int)(uint)param_5 * 0x2a8) & 2) == 0)) {
      if (((param_3 < local_res20) && (0 < local_res20)) ||
         ((local_res20 < param_3 && (local_res20 < 0)))) {
        if (param_2 == 0x7fff) {
          local_c = 1;
        }
        else if (param_2 < param_1) {
          local_c = 2;
          SideTouch_GestureFailureRecord(param_5,4,(int)param_1);
        }
        else {
          local_c = 1;
        }
      }
      else if (param_2 == 0x7fff) {
        *(uint *)(&DAT_6bc997c8 + (longlong)(int)(uint)param_5 * 0x2a8) =
             *(uint *)(&DAT_6bc997c8 + (longlong)(int)(uint)param_5 * 0x2a8) | 2;
      }
      else if (param_2 < param_1) {
        local_c = 2;
        SideTouch_GestureFailureRecord(param_5,4,(int)param_1);
      }
      else {
        *(uint *)(&DAT_6bc997c8 + (longlong)(int)(uint)param_5 * 0x2a8) =
             *(uint *)(&DAT_6bc997c8 + (longlong)(int)(uint)param_5 * 0x2a8) | 2;
      }
    }
  }
  else if (param_3 < local_res20) {
    local_c = 2;
    SideTouch_GestureFailureRecord(param_5,3,(int)param_3);
  }
  if (sStackX_22 != 0x7fff) {
    if ((sStackX_22 < param_3) && (0 < sStackX_22)) {
      local_c = 2;
      SideTouch_GestureFailureRecord(param_5,5,(int)param_3);
    }
    else if ((param_3 < sStackX_22) && (param_4 < 0)) {
      local_c = 2;
      SideTouch_GestureFailureRecord(param_5,5,(int)param_3);
    }
  }
  return local_c;
}


// ============================================================================
// Function: SideTouch_CheckOnTouch @ 6bb14ba5
// ============================================================================

undefined4 SideTouch_CheckOnTouch(short param_1,short param_2,short param_3,byte param_4)

{
  undefined4 local_c;
  
  local_c = 0;
  if (param_2 < param_1) {
    *(uint *)(&DAT_6bc997c8 + (longlong)(int)(uint)param_4 * 0x2a8) =
         *(uint *)(&DAT_6bc997c8 + (longlong)(int)(uint)param_4 * 0x2a8) | 1;
  }
  if ((param_3 < param_1) && (param_3 != 0x7fff)) {
    local_c = 2;
    SideTouch_GestureFailureRecord(param_4,7,(int)param_1);
  }
  else if ((*(uint *)(&DAT_6bc997c8 + (longlong)(int)(uint)param_4 * 0x2a8) & 1) == 0) {
    if ((param_1 < param_2) && (param_2 != -0x8000)) {
      local_c = 1;
    }
  }
  else if ((param_1 < param_2) && (param_2 != -0x8000)) {
    local_c = 2;
    SideTouch_GestureFailureRecord(param_4,6,(int)param_1);
  }
  return local_c;
}


// ============================================================================
// Function: SideTouch_ActionDetectedGetStartPDim2 @ 6bb14cc5
// ============================================================================

ushort SideTouch_ActionDetectedGetStartPDim2(byte param_1,byte param_2)

{
  ushort local_a;
  
  if ((*(uint *)(&DAT_6bc997c8 + (longlong)(int)(uint)param_2 * 0x2a8) & 4) == 0) {
    local_a = *(ushort *)
               ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x4fa);
    *(ushort *)(&DAT_6bc997de + (longlong)(int)(uint)param_2 * 0x2a8) = local_a;
    *(uint *)(&DAT_6bc997c8 + (longlong)(int)(uint)param_2 * 0x2a8) =
         *(uint *)(&DAT_6bc997c8 + (longlong)(int)(uint)param_2 * 0x2a8) | 4;
  }
  else {
    local_a = *(ushort *)(&DAT_6bc997de + (longlong)(int)(uint)param_2 * 0x2a8);
  }
  if ((int)(uint)local_a < (int)*(short *)(&DAT_6bc995c4 + (longlong)(int)(uint)param_2 * 0x2a8)) {
    local_a = *(ushort *)(&DAT_6bc995c4 + (longlong)(int)(uint)param_2 * 0x2a8);
  }
  if ((int)*(short *)(&DAT_6bc995c6 + (longlong)(int)(uint)param_2 * 0x2a8) < (int)(uint)local_a) {
    local_a = *(ushort *)(&DAT_6bc995c6 + (longlong)(int)(uint)param_2 * 0x2a8);
  }
  return local_a;
}


// ============================================================================
// Function: SideTouch_IsGripRatioTrustable @ 6bb1ccd0
// ============================================================================

undefined1 SideTouch_IsGripRatioTrustable(byte param_1)

{
  char cVar1;
  undefined1 local_9;
  
  local_9 = 1;
  if (*(int *)&_refptr_g_tsaPrmtFlash[2].field_0x69c != 0) {
    if ((*(int *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c)
         != 2) &&
       (*(int *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c)
        != 4)) {
      local_9 = 0;
    }
    cVar1 = Peak_IsTouchValid(param_1);
    if (((cVar1 != '\0') &&
        (*(short *)(*_refptr_g_peaksPtr +
                   (longlong)
                   (int)(uint)*(byte *)((longlong)(int)(uint)param_1 * 0x168 +
                                        *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x18) <
         *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x634)) &&
       ((ushort)(byte)_refptr_g_tsaPrmtFlash[2].field_0x62c <
        *(ushort *)
         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1a8))) {
      local_9 = 0;
    }
  }
  return local_9;
}


// ============================================================================
// Function: SideTouch_GetGripRatio @ 6bb1ce05
// ============================================================================

undefined1 SideTouch_GetGripRatio(undefined8 param_1,undefined8 param_2,undefined1 param_3)

{
  undefined1 uVar1;
  char cVar2;
  
  uVar1 = _refptr_g_tsaPrmtFlash[2].field_0x62d;
  if (_refptr_g_tsaPrmtFlash[2].field_0x62f == '\0') {
    cVar2 = TSAStatic_IsScreenRotated();
    if (cVar2 != '\0') {
      cVar2 = SideGesture_IsInVolumeArea(param_3);
      if (cVar2 != '\0') {
        uVar1 = _refptr_g_tsaPrmtFlash[2].field_0x62c;
      }
    }
  }
  else {
    cVar2 = SideGesture_IsInVolumeArea(param_3);
    if (cVar2 != '\0') {
      cVar2 = SideGesture_IsInExternalArea(param_3);
      if (cVar2 == '\0') {
        uVar1 = _refptr_g_tsaPrmtFlash[2].field_0x62e;
      }
    }
  }
  return uVar1;
}


// ============================================================================
// Function: SideTouch_CheckAbnormalMaxMin @ 6bb0d365
// ============================================================================

undefined8 SideTouch_CheckAbnormalMaxMin(void)

{
  char cVar1;
  int iVar2;
  undefined8 uVar3;
  
  iVar2 = MAX((int)*(short *)(*(longlong *)_refptr_g_baselineData + 0x1058),
              -(int)*(short *)(*(longlong *)_refptr_g_baselineData + 0x105a));
  if (iVar2 < *(short *)(_refptr_g_tsaPrmtDynamic + 0x46)) {
    uVar3 = 0;
  }
  else if (-(int)*(short *)(*(longlong *)_refptr_g_baselineData + 0x105a) <
           (int)*(short *)(*(longlong *)_refptr_g_baselineData + 0x1058)) {
    if (*(int *)_refptr_g_baselineSide == 4) {
      cVar1 = SafeBaseline_CheckAbnormalSensor
                        (0,*(undefined1 *)(*(longlong *)_refptr_g_baselineData + 0x105c));
      if (cVar1 == '\0') {
        uVar3 = 0;
      }
      else {
        uVar3 = 1;
      }
    }
    else {
      cVar1 = SafeBaseline_CheckAbnormalSensor
                        (_refptr_g_tsaPrmtFlash->bCols + 0xff,
                         *(undefined1 *)(*(longlong *)_refptr_g_baselineData + 0x105c));
      if (cVar1 == '\0') {
        uVar3 = 0;
      }
      else {
        uVar3 = 1;
      }
    }
  }
  else if (*(int *)_refptr_g_baselineSide == 4) {
    cVar1 = SafeBaseline_CheckAbnormalSensor
                      (0,*(undefined1 *)(*(longlong *)_refptr_g_baselineData + 0x105d));
    if (cVar1 == '\0') {
      uVar3 = 0;
    }
    else {
      uVar3 = 1;
    }
  }
  else {
    cVar1 = SafeBaseline_CheckAbnormalSensor
                      (_refptr_g_tsaPrmtFlash->bCols + 0xff,
                       *(undefined1 *)(*(longlong *)_refptr_g_baselineData + 0x105d));
    if (cVar1 == '\0') {
      uVar3 = 0;
    }
    else {
      uVar3 = 1;
    }
  }
  return uVar3;
}


// ============================================================================
// Function: SideTouch_AntiTriggerPrmtUpdate @ 6bb0b1e0
// ============================================================================

void SideTouch_AntiTriggerPrmtUpdate(void)

{
  g_prmtAntiTrigger = 0x28;
  DAT_6bc994a4 = 0x96;
  DAT_6bc994a8 = 0x32;
  DAT_6bc994b0 = (*(short *)(_refptr_g_tsaPrmtDynamic + 0xc) * 3) / 2;
  DAT_6bc994b4 = 0x42;
  DAT_6bc994b8 = 0x96;
  DAT_6bc994bc = 0x42;
  DAT_6bc994c0 = 0x96;
  DAT_6bc994c4 = 0xfa;
  DAT_6bc994c8 = 0x42;
  DAT_6bc994cc = 0x96;
  DAT_6bc994d0 = 200;
  if (((*_refptr_g_tsaStaticPtr)->nField_0x264 == 1) ||
     ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 3)) {
    g_prmtAntiTrigger = 0x32;
    DAT_6bc994a4 = 200;
  }
  if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x400000) != 0) {
    DAT_6bc994a4 = 300;
  }
  DAT_6bc994ac = 600;
  return;
}


// ============================================================================
// Function: SideTouch_TriggerPassLevel2Check @ 6bb0b4b1
// ============================================================================

char SideTouch_TriggerPassLevel2Check(byte param_1)

{
  undefined *puVar1;
  char cVar2;
  int iVar3;
  char local_9;
  
  puVar1 = _refptr_g_sideGesture;
  local_9 = '\0';
  if ((*(int *)(_refptr_g_sideGesture + (longlong)(int)(uint)param_1 * 0x2a8 + 0xac) < DAT_6bc994a8)
     || (DAT_6bc994ac <
         *(int *)(_refptr_g_sideGesture + (longlong)(int)(uint)param_1 * 0x2a8 + 0xac))) {
    local_9 = '\x01';
  }
  if ((DAT_6bc994d0 <
       (int)((uint)*(ushort *)(_refptr_g_sideGesture + (longlong)(int)(uint)param_1 * 0x2a8 + 0x74)
            - (uint)*(ushort *)(_refptr_g_sideGesture + (longlong)(int)(uint)param_1 * 0x2a8 + 0x72)
            )) ||
     (DAT_6bc994d0 <
      (int)((uint)*(ushort *)(_refptr_g_sideGesture + (longlong)(int)(uint)param_1 * 0x2a8 + 0xd4) -
           (uint)*(ushort *)(_refptr_g_sideGesture + (longlong)(int)(uint)param_1 * 0x2a8 + 0xd2))))
  {
    local_9 = local_9 + '\x01';
  }
  iVar3 = TSA_GetRatio(*(undefined4 *)
                        (_refptr_g_sideGesture + (longlong)(int)(uint)param_1 * 0x2a8 + 0xd8),
                       *(undefined4 *)
                        (_refptr_g_sideGesture + (longlong)(int)(uint)param_1 * 0x2a8 + 0x78),100);
  if ((((iVar3 < DAT_6bc994b4) || (DAT_6bc994b8 < iVar3)) ||
      (*(int *)(puVar1 + (longlong)(int)(uint)param_1 * 0x2a8 + 0x78) <
       (*(short *)(_refptr_g_tsaPrmtDynamic + 0xc) * 3) / 2)) ||
     (*(int *)(puVar1 + (longlong)(int)(uint)param_1 * 0x2a8 + 0xd8) <
      (*(short *)(_refptr_g_tsaPrmtDynamic + 0xc) * 3) / 2)) {
    local_9 = local_9 + '\x01';
  }
  iVar3 = TSA_GetRatio(*(undefined4 *)(puVar1 + (longlong)(int)(uint)param_1 * 0x2a8 + 0xdc),
                       *(undefined4 *)(puVar1 + (longlong)(int)(uint)param_1 * 0x2a8 + 0x7c),100);
  if (((((*(int *)(puVar1 + (longlong)(int)(uint)param_1 * 0x2a8 + 0x7c) < g_prmtAntiTrigger) ||
        (DAT_6bc994a4 < *(int *)(puVar1 + (longlong)(int)(uint)param_1 * 0x2a8 + 0x7c))) ||
       ((*(int *)(puVar1 + (longlong)(int)(uint)param_1 * 0x2a8 + 0xdc) < g_prmtAntiTrigger ||
        ((DAT_6bc994a4 < *(int *)(puVar1 + (longlong)(int)(uint)param_1 * 0x2a8 + 0xdc) ||
         (iVar3 < DAT_6bc994bc)))))) || (cVar2 = local_9, DAT_6bc994c0 < iVar3)) &&
     (((cVar2 = local_9 + '\x01', iVar3 < 0x32 || (200 < iVar3)) &&
      (*(int *)(puVar1 + (longlong)(int)(uint)param_1 * 0x2a8 + 0x24c) != 0)))) {
    cVar2 = local_9 + '\x02';
  }
  local_9 = cVar2;
  iVar3 = TSA_GetRatio(*(undefined2 *)(puVar1 + (longlong)(int)(uint)param_1 * 0x2a8 + 0xd6),
                       *(undefined2 *)(puVar1 + (longlong)(int)(uint)param_1 * 0x2a8 + 0x76),100);
  if (((iVar3 < DAT_6bc994c8) || (DAT_6bc994cc < iVar3)) ||
     ((DAT_6bc994c4 < (int)(uint)*(ushort *)(puVar1 + (longlong)(int)(uint)param_1 * 0x2a8 + 0x76)
      || (DAT_6bc994c4 <
          (int)(uint)*(ushort *)(puVar1 + (longlong)(int)(uint)param_1 * 0x2a8 + 0xd6))))) {
    local_9 = local_9 + '\x01';
  }
  return local_9;
}


// ============================================================================
// Function: SideTouch_IsToCheckTrigger @ 6bb0b75d
// ============================================================================

bool SideTouch_IsToCheckTrigger(byte param_1)

{
  undefined *puVar1;
  int iVar2;
  bool local_9;
  
  puVar1 = _refptr_g_sideGesture;
  local_9 = *(int *)(_refptr_g_sideGesture + (longlong)(int)(uint)param_1 * 0x2a8 + 0x24c) != 0;
  if (((DAT_6bc994a4 < *(int *)(_refptr_g_sideGesture + (longlong)(int)(uint)param_1 * 0x2a8 + 0x7c)
       ) || (DAT_6bc994a4 <
             *(int *)(_refptr_g_sideGesture + (longlong)(int)(uint)param_1 * 0x2a8 + 0xdc))) &&
     ((iVar2 = TSA_GetRatio(*(undefined4 *)
                             (_refptr_g_sideGesture + (longlong)(int)(uint)param_1 * 0x2a8 + 0xdc),
                            *(undefined4 *)
                             (_refptr_g_sideGesture + (longlong)(int)(uint)param_1 * 0x2a8 + 0x7c),
                            100), iVar2 < 0x32 || (200 < iVar2)))) {
    local_9 = true;
  }
  if (((DAT_6bc994c4 <= (int)(uint)*(ushort *)(puVar1 + (longlong)(int)(uint)param_1 * 0x2a8 + 0x76)
       ) && (*(int *)(puVar1 + (longlong)(int)(uint)param_1 * 0x2a8 + 0x78) < DAT_6bc994b0)) ||
     ((DAT_6bc994c4 <= (int)(uint)*(ushort *)(puVar1 + (longlong)(int)(uint)param_1 * 0x2a8 + 0xd6)
      && (*(int *)(puVar1 + (longlong)(int)(uint)param_1 * 0x2a8 + 0xd8) < DAT_6bc994b0)))) {
    local_9 = true;
  }
  if ((DAT_6bc994d0 <
       (int)((uint)*(ushort *)(puVar1 + (longlong)(int)(uint)param_1 * 0x2a8 + 0x74) -
            (uint)*(ushort *)(puVar1 + (longlong)(int)(uint)param_1 * 0x2a8 + 0x72))) ||
     (DAT_6bc994d0 <
      (int)((uint)*(ushort *)(puVar1 + (longlong)(int)(uint)param_1 * 0x2a8 + 0xd4) -
           (uint)*(ushort *)(puVar1 + (longlong)(int)(uint)param_1 * 0x2a8 + 0xd2)))) {
    local_9 = true;
  }
  return local_9;
}


// ============================================================================
// Function: SideTouch_GestureVolumeMapToGUIRelatively @ 6bb189a6
// ============================================================================

void SideTouch_GestureVolumeMapToGUIRelatively(void)

{
  char cVar1;
  undefined4 uVar2;
  byte local_9;
  
                    /* 0xd89a6  1355  SideTouch_GestureVolumeMapToGUIRelatively */
  if ((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6a0 & 0x40) != 0) {
    uVar2 = SideGesture_TPSlideVolumeOffsetProcess();
    for (local_9 = 0; local_9 < g_sideGesture; local_9 = local_9 + 1) {
      cVar1 = SideGesture_IsTPSlideVolume(local_9);
      if (cVar1 != '\0') {
        SideTouch_GestureVolumeMapToGUIRelativelyUnit(local_9,uVar2);
      }
    }
  }
  return;
}


// ============================================================================
// Function: SideTouch_GestureVolumeMapToGUIRelativelyUnit @ 6bb1858a
// ============================================================================

void SideTouch_GestureVolumeMapToGUIRelativelyUnit(byte param_1,short param_2)

{
  char cVar1;
  undefined2 uVar2;
  short local_1c;
  short local_1a;
  
  if (*_refptr_g_guiVolume == '\0') {
    cVar1 = SideGesture_IsVolumeTrigger(param_1);
    if (cVar1 == '\0') {
      *(undefined4 *)(&DAT_6bc995c8 + (longlong)(int)(uint)param_1 * 0x2a8) = 1;
    }
    else {
      *(undefined4 *)(&DAT_6bc995c8 + (longlong)(int)(uint)param_1 * 0x2a8) = 0x10;
      cVar1 = SideGesture_IsLeftVolumeTrigger(param_1);
      if (cVar1 == '\0') {
        *(undefined4 *)(&DAT_6bc995cc + (longlong)(int)(uint)param_1 * 0x2a8) = 0x800;
      }
      else {
        *(undefined4 *)(&DAT_6bc995cc + (longlong)(int)(uint)param_1 * 0x2a8) = 0x400;
      }
    }
  }
  else {
    *(undefined4 *)(&DAT_6bc995c8 + (longlong)(int)(uint)param_1 * 0x2a8) = 4;
    if (_refptr_g_guiVolume[1] == '\0') {
      *(undefined2 *)(&DAT_6bc995cc + (longlong)(int)(uint)param_1 * 0x2a8) =
           *(undefined2 *)(_refptr_g_guiVolume + 4);
      *(undefined2 *)(&DAT_6bc995ce + (longlong)(int)(uint)param_1 * 0x2a8) =
           *(undefined2 *)(_refptr_g_guiVolume + 6);
    }
    else {
      *(undefined2 *)(&DAT_6bc995d0 + (longlong)(int)(uint)param_1 * 0x2a8) =
           *(undefined2 *)(_refptr_g_guiVolume + 8);
      *(undefined2 *)(&DAT_6bc995d2 + (longlong)(int)(uint)param_1 * 0x2a8) =
           *(undefined2 *)(_refptr_g_guiVolume + 10);
    }
  }
  if (*_refptr_g_guiVolume == '\0') {
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x644);
    *(undefined2 *)(&DAT_6bc995c4 + (longlong)(int)(uint)param_1 * 0x2a8) = uVar2;
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x646);
    *(undefined2 *)(&DAT_6bc995c6 + (longlong)(int)(uint)param_1 * 0x2a8) = uVar2;
  }
  else {
    local_1a = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x644);
    local_1c = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x646);
    cVar1 = SideGesture_IsVolumeMapping();
    if (cVar1 != '\0') {
      local_1a = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x648);
      local_1c = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x64a);
    }
    *(short *)(&DAT_6bc995c4 + (longlong)(int)(uint)param_1 * 0x2a8) = local_1a;
    if (_refptr_g_guiVolume[2] == '\0') {
      local_1a = local_1a - param_2;
    }
    else {
      local_1a = param_2 - local_1a;
    }
    if (_refptr_g_guiVolume[1] == '\0') {
      *(short *)(&DAT_6bc995d0 + (longlong)(int)(uint)param_1 * 0x2a8) = local_1a;
    }
    else {
      *(short *)(&DAT_6bc995cc + (longlong)(int)(uint)param_1 * 0x2a8) = local_1a;
    }
    *(short *)(&DAT_6bc995c6 + (longlong)(int)(uint)param_1 * 0x2a8) = local_1c;
    if (_refptr_g_guiVolume[2] == '\0') {
      param_2 = local_1c - param_2;
    }
    else {
      param_2 = param_2 - local_1c;
    }
    if (_refptr_g_guiVolume[1] == '\0') {
      *(short *)(&DAT_6bc995d2 + (longlong)(int)(uint)param_1 * 0x2a8) = param_2;
    }
    else {
      *(short *)(&DAT_6bc995ce + (longlong)(int)(uint)param_1 * 0x2a8) = param_2;
    }
  }
  return;
}


// ============================================================================
// Function: SideTouch_IsInVolumeTriggerStage @ 6bb1a093
// ============================================================================

undefined1 SideTouch_IsInVolumeTriggerStage(void)

{
  undefined1 local_9;
  
                    /* 0xda093  1374  SideTouch_IsInVolumeTriggerStage */
  local_9 = 0;
  if ((DAT_6bc9d818 != -1) &&
     ((ulonglong)((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - DAT_6bc9d818) <
      (ulonglong)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x672)) {
    local_9 = 1;
  }
  return local_9;
}


