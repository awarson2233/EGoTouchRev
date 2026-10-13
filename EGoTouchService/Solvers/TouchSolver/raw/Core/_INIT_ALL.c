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
// Function: TSAPrmt_Init @ 6bb3b90d
// ============================================================================

bool TSAPrmt_Init(void)

{
  char cVar1;
  
                    /* 0xfb90d  1648  TSAPrmt_Init */
  cVar1 = TSAPrmt_FlashPrmtLoadProject(0);
  if (cVar1 != '\0') {
    TSAPrmt_ReInit();
  }
  return cVar1 != '\0';
}


// ============================================================================
// Function: TSAPrmt_LoadProject @ 6bb3b93a
// ============================================================================

undefined8 TSAPrmt_LoadProject(longlong param_1)

{
  char cVar1;
  int iVar2;
  undefined8 uVar3;
  char *_Str1;
  
                    /* 0xfb93a  1649  TSAPrmt_LoadProject */
  if (param_1 == 0) {
    uVar3 = 0;
  }
  else {
    cVar1 = TSAPrmt_FlashPrmtLoadProject(param_1);
    if (cVar1 == '\0') {
      uVar3 = 0;
    }
    else {
      _Str1 = (char *)TSAPrmt_GetProjectName();
      iVar2 = strcmp(_Str1,"DFLT000000");
      if (iVar2 == 0) {
        g_tsaPrmtFlash.bCols = AFE_GetColNum();
        g_tsaPrmtFlash.bRows = AFE_GetRowNum();
        g_tsaPrmtFlash.wBufElementCount =
             (ushort)g_tsaPrmtFlash.bCols * (ushort)g_tsaPrmtFlash.bRows;
      }
      TSAPrmt_ReInit();
      uVar3 = 1;
    }
  }
  return uVar3;
}


// ============================================================================
// Function: SecInitFloatFmt @ 6ba42ac1
// ============================================================================

undefined8 SecInitFloatFmt(longlong param_1,longlong param_2)

{
  void *pvVar1;
  longlong local_18;
  uint local_10;
  int local_c;
  
  local_18 = param_2 + -2;
  SecSeekToFrontPercent(&local_18);
  local_10 = ((int)param_2 - (int)local_18) + 1;
  if ((int)local_10 < 0x11) {
    *(longlong *)(param_1 + 0x10) = param_1;
    *(undefined8 *)(param_1 + 0x18) = 0;
  }
  else {
    pvVar1 = malloc((ulonglong)local_10);
    *(void **)(param_1 + 0x18) = pvVar1;
    if (*(longlong *)(param_1 + 0x18) == 0) {
      return 0xffffffff;
    }
    *(undefined8 *)(param_1 + 0x10) = *(undefined8 *)(param_1 + 0x18);
  }
  for (local_c = 0; local_c < (int)(local_10 - 1); local_c = local_c + 1) {
    *(undefined1 *)(*(longlong *)(param_1 + 0x10) + (longlong)local_c) =
         *(undefined1 *)(local_c + local_18);
  }
  *(undefined1 *)(*(longlong *)(param_1 + 0x10) + (longlong)(int)local_10 + -1) = 0;
  return 0;
}


// ============================================================================
// Function: SecInitFloatBuffer @ 6ba42bb5
// ============================================================================

undefined8 SecInitFloatBuffer(longlong param_1,undefined8 param_2,longlong param_3)

{
  int iVar1;
  void *pvVar2;
  undefined8 uVar3;
  
  *(undefined8 *)(param_1 + 0x18) = 0;
  *(undefined8 *)(param_1 + 0x10) = 0;
  *(undefined8 *)(param_1 + 0x20) = 0;
  if (*(int *)(param_3 + 0x24) < 0) {
    *(undefined4 *)(param_3 + 0x24) = 6;
  }
  if ((*(uint *)(param_3 + 0x1c) & 0x400) == 0) {
    if (0xccccb6b < *(int *)(param_3 + 0x24)) {
      return 0xffffffff;
    }
    *(int *)(param_1 + 0x28) = *(int *)(param_3 + 0x24) + 0x15f;
  }
  else {
    if (0xcccb95c < *(int *)(param_3 + 0x24)) {
      return 0xffffffff;
    }
    *(int *)(param_1 + 0x28) = *(int *)(param_3 + 0x24) + 0x136e;
  }
  if (*(int *)(param_1 + 0x28) < *(int *)(param_3 + 0x20)) {
    *(int *)(param_1 + 0x28) = *(int *)(param_3 + 0x20) + 2;
  }
  if (*(int *)(param_1 + 0x28) < 0x201) {
    *(longlong *)(param_3 + 8) = param_3 + 0x3a;
  }
  else {
    pvVar2 = malloc((ulonglong)*(uint *)(param_1 + 0x28));
    *(void **)(param_1 + 0x20) = pvVar2;
    if (*(longlong *)(param_1 + 0x20) == 0) {
      return 0xffffffff;
    }
    *(undefined8 *)(param_3 + 8) = *(undefined8 *)(param_1 + 0x20);
  }
  iVar1 = SecInitFloatFmt(param_1,param_2);
  if (iVar1 == 0) {
    uVar3 = 0;
  }
  else {
    if (*(longlong *)(param_1 + 0x20) != 0) {
      free(*(void **)(param_1 + 0x20));
      *(undefined8 *)(param_1 + 0x20) = 0;
    }
    uVar3 = 0xffffffff;
  }
  return uVar3;
}


// ============================================================================
// Function: SecInitFloatFmt @ 6ba44bc2
// ============================================================================

undefined8 SecInitFloatFmt(longlong param_1,longlong param_2)

{
  void *pvVar1;
  longlong local_18;
  uint local_10;
  int local_c;
  
  local_18 = param_2 + -4;
  SecSeekToFrontPercent(&local_18);
  local_10 = (int)(param_2 - local_18 >> 1) + 1;
  if ((int)local_10 < 0x11) {
    *(longlong *)(param_1 + 0x10) = param_1;
    *(undefined8 *)(param_1 + 0x18) = 0;
  }
  else {
    pvVar1 = malloc((ulonglong)local_10);
    *(void **)(param_1 + 0x18) = pvVar1;
    if (*(longlong *)(param_1 + 0x18) == 0) {
      return 0xffffffff;
    }
    *(undefined8 *)(param_1 + 0x10) = *(undefined8 *)(param_1 + 0x18);
  }
  for (local_c = 0; local_c < (int)(local_10 - 1); local_c = local_c + 1) {
    *(char *)((longlong)local_c + *(longlong *)(param_1 + 0x10)) =
         (char)*(undefined2 *)(local_18 + (longlong)local_c * 2);
  }
  *(undefined1 *)(*(longlong *)(param_1 + 0x10) + (longlong)(int)local_10 + -1) = 0;
  return 0;
}


// ============================================================================
// Function: SecInitFloatBuffer @ 6ba44cbd
// ============================================================================

undefined8 SecInitFloatBuffer(longlong param_1,undefined8 param_2,longlong param_3)

{
  int iVar1;
  void *pvVar2;
  undefined8 uVar3;
  
  *(undefined8 *)(param_1 + 0x18) = 0;
  *(undefined8 *)(param_1 + 0x10) = 0;
  *(undefined8 *)(param_1 + 0x20) = 0;
  if (*(int *)(param_3 + 0x24) < 0) {
    *(undefined4 *)(param_3 + 0x24) = 6;
  }
  if ((*(uint *)(param_3 + 0x1c) & 0x400) == 0) {
    if (0xccccb6b < *(int *)(param_3 + 0x24)) {
      return 0xffffffff;
    }
    *(int *)(param_1 + 0x28) = *(int *)(param_3 + 0x24) + 0x15f;
  }
  else {
    if (0xcccb95c < *(int *)(param_3 + 0x24)) {
      return 0xffffffff;
    }
    *(int *)(param_1 + 0x28) = *(int *)(param_3 + 0x24) + 0x136e;
  }
  if (*(int *)(param_1 + 0x28) < *(int *)(param_3 + 0x20)) {
    *(int *)(param_1 + 0x28) = *(int *)(param_3 + 0x20) + 2;
  }
  if (*(int *)(param_1 + 0x28) < 0x201) {
    *(longlong *)(param_3 + 8) = param_3 + 0x3c;
  }
  else {
    pvVar2 = malloc((ulonglong)*(uint *)(param_1 + 0x28));
    *(void **)(param_1 + 0x20) = pvVar2;
    if (*(longlong *)(param_1 + 0x20) == 0) {
      return 0xffffffff;
    }
    *(undefined8 *)(param_3 + 8) = *(undefined8 *)(param_1 + 0x20);
  }
  iVar1 = SecInitFloatFmt(param_1,param_2);
  if (iVar1 == 0) {
    uVar3 = 0;
  }
  else {
    if (*(longlong *)(param_1 + 0x20) != 0) {
      free(*(void **)(param_1 + 0x20));
      *(undefined8 *)(param_1 + 0x20) = 0;
    }
    uVar3 = 0xffffffff;
  }
  return uVar3;
}


// ============================================================================
// Function: BLIIR_Init @ 6ba46e70
// ============================================================================

void BLIIR_Init(void)

{
                    /* 0x6e70  349  BLIIR_Init */
  return;
}


// ============================================================================
// Function: BLRecal_Init @ 6ba47430
// ============================================================================

void BLRecal_Init(void)

{
                    /* 0x7430  353  BLRecal_Init */
  (*_refptr_g_tsaStaticPtr)->bRecalFrameCount = 0xff;
  (*_refptr_g_tsaStaticPtr)->bRecalBoundaryHitCount = '\0';
  return;
}


// ============================================================================
// Function: BLReset_Init @ 6ba47783
// ============================================================================

void BLReset_Init(char param_1)

{
  int iVar1;
  
                    /* 0x7783  359  BLReset_Init */
  *(undefined2 *)&_refptr_g_tsaPrpt->field_0x284 = 0;
  *(undefined2 *)&_refptr_g_tsaPrpt->field_0x286 = 0;
  _refptr_g_tsaPrpt->field_0x29c = 0;
  _refptr_g_tsaPrpt->dwBaselinePropertyState = 0;
  _refptr_g_tsaPrpt->dwPrevBaselinePropertyState = 0;
  (*_refptr_g_tsaStaticPtr)->bBaselineResetCheckEnabled = '\x01';
  (*_refptr_g_tsaStaticPtr)->bBaselineResetInDebounce = '\0';
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x2f0 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x310 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x330 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x340 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x350 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x360 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x368 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x370 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x378 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x380 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x388 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x390 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x3a8 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x3b0 = 0;
  *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3b8 = 0;
  *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3bc = 0;
  *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3c0 = 0;
  *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3c4 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x3d0 = 0;
  *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3d8 = 0;
  (*_refptr_g_tsaStaticPtr)->field_0x2f8 = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x300 = 0;
  (*_refptr_g_tsaStaticPtr)->field_0x2f9 = 0;
  (*_refptr_g_tsaStaticPtr)->field_0x30a = 0;
  (*_refptr_g_tsaStaticPtr)->field_0x318 = 0;
  (*_refptr_g_tsaStaticPtr)->bBaselineUpdatedThisFrame = '\0';
  (*_refptr_g_tsaStaticPtr)[1].field_0x1 = 0;
  (*_refptr_g_tsaStaticPtr)[1].field_0x3 = 0;
  *(undefined4 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 = 0;
  (*_refptr_g_tsaStaticPtr)->field_0x3ea = 0;
  iVar1 = memset_s(*(undefined8 *)_refptr_g_tsaBufCleanBaseline,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2,0,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/Baseline/BaselineReset.c",0x72,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  if (param_1 != '\0') {
    (*_refptr_g_tsaStaticPtr)->bBaselineResetPropertyLatched = '\0';
  }
  return;
}


// ============================================================================
// Function: BLReset_FlagDisableBaselineUpdateInit @ 6ba47ae6
// ============================================================================

void BLReset_FlagDisableBaselineUpdateInit(void)

{
                    /* 0x7ae6  357  BLReset_FlagDisableBaselineUpdateInit */
  (*_refptr_g_tsaStaticPtr)[1].field_0x2 = 0;
  return;
}


// ============================================================================
// Function: BLReset_IsAllFreqsBaselineInitializedDuringTouchLiftOff @ 6ba48b01
// ============================================================================

undefined1 BLReset_IsAllFreqsBaselineInitializedDuringTouchLiftOff(void)

{
  char cVar1;
  undefined1 local_9;
  
                    /* 0x8b01  360  BLReset_IsAllFreqsBaselineInitializedDuringTouchLiftOff */
  local_9 = 0;
  cVar1 = AFE_IsIndividualBaselineForEachFreq();
  if (cVar1 == '\0') {
    local_9 = 0;
  }
  else if (((((*_refptr_g_tsaStaticPtr)->field_0x117 != '\0') &&
            (cVar1 = AFE_IsAlwaysNoisy(), cVar1 == '\0')) &&
           ((int)(*(short *)(_refptr_g_tsaPrmtDynamic + 0xc) / 2) <
            (int)((uint)_refptr_g_tsaPrpt->wBoundaryPositive -
                 *(int *)&_refptr_g_tsaPrmtFlash->field_0x5fc))) &&
          ((byte)_refptr_g_tsaPrpt->field_0x29c < 3)) {
    local_9 = 1;
    _refptr_g_tsaPrpt->field_0x29c = _refptr_g_tsaPrpt->field_0x29c + '\x01';
  }
  return local_9;
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
// Function: CMF_Init @ 6ba4d8c1
// ============================================================================

void CMF_Init(void)

{
                    /* 0xd8c1  474  CMF_Init */
  CMFRemap_Init();
  if ((_refptr_g_tsaPrmtFlash->dwCmfMethodNormal == 1) ||
     (_refptr_g_tsaPrmtFlash->dwCmfMethodSmartCover == 1)) {
    CMF_Init2DFilter();
  }
  return;
}


// ============================================================================
// Function: CMF_InitNoiseData @ 6ba4d9e0
// ============================================================================

void CMF_InitNoiseData(undefined8 param_1,undefined8 param_2)

{
  int iVar1;
  
  iVar1 = memset_s(&g_cmfNoise,0xc0,0,0xc0);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/CMF/CMF.c",0x83,
                      "rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  DAT_6bb7f130 = param_1;
  DAT_6bb7f138 = param_2;
  g_cmfNoise = 0xffff;
  return;
}


// ============================================================================
// Function: CMFRemap_InitCoef @ 6ba4f8b0
// ============================================================================

void CMFRemap_InitCoef(int param_1,longlong param_2,longlong param_3,longlong param_4)

{
  uint uVar1;
  uint uVar2;
  int local_res8;
  int local_18;
  int local_14;
  int local_10;
  int local_c;
  
  local_10 = 0;
  if (_refptr_g_tsaPrmtFlash->field_0x6cd == '\0') {
    for (local_c = 0; local_c < param_1; local_c = local_c + 1) {
      *(undefined2 *)(param_3 + (longlong)local_c * 2) = 0x400;
      *(undefined2 *)(param_4 + (longlong)local_c * 2) = 0x400;
    }
  }
  else {
    for (local_c = 0; local_c < param_1; local_c = local_c + 1) {
      uVar1 = (uint)*(short *)(param_2 + (longlong)local_c * 2);
      uVar2 = (int)uVar1 >> 0x1f;
      local_10 = local_10 + ((uVar1 ^ uVar2) - uVar2);
    }
    if (local_10 == 0) {
      local_10 = param_1;
    }
    local_res8 = param_1;
    if (param_1 == 0) {
      local_res8 = 1;
    }
    if (local_10 / local_res8 == 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/CMF/CMFRemap.c",
                        0x7b,"coefAverage != 0");
      HardwareAnalyzer_SetSoftException();
    }
    for (local_c = 0; local_c < local_res8; local_c = local_c + 1) {
      local_18 = (int)*(short *)(param_2 + (longlong)local_c * 2);
      if (local_18 == 0) {
        local_18 = 1;
      }
      local_14 = (local_10 / local_res8 << 10) / local_18;
      if (local_14 < 0x1000) {
        if ((local_14 < 0x100) && (-1 < local_14)) {
          local_14 = 0x100;
        }
        else if ((local_14 < 0) && (-0x101 < local_14)) {
          local_14 = -0x100;
        }
        else if (local_14 < -0x1000) {
          local_14 = -0x1000;
        }
      }
      else {
        local_14 = 0x1000;
      }
      *(short *)(param_3 + (longlong)local_c * 2) = (short)local_14;
      *(short *)((longlong)local_c * 2 + param_4) = (short)(0x100000 / (longlong)local_14);
    }
  }
  return;
}


// ============================================================================
// Function: CMFRemap_InitCoef2D @ 6ba4fa86
// ============================================================================

void CMFRemap_InitCoef2D(void)

{
  int iVar1;
  short local_298 [326];
  ushort local_c;
  byte local_a;
  byte local_9;
  
  CMFRemap_InitCoef(g_cmfRemapRxNum,&_refptr_g_tsaPrmtFlash->field_0x6ce,local_298 + 0xf0,
                    local_298 + 0x50);
  CMFRemap_InitCoef(g_cmfRemapTxNum,&_refptr_g_tsaPrmtFlash->field_0x76e,local_298 + 0xa0,local_298)
  ;
  for (local_9 = 0; local_9 < g_cmfRemapRxNum; local_9 = local_9 + 1) {
    for (local_a = 0; local_a < g_cmfRemapTxNum; local_a = local_a + 1) {
      if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\x01') {
        local_c = (ushort)local_9 + (ushort)local_a * (ushort)_refptr_g_tsaPrmtFlash->bCols;
      }
      else {
        local_c = (ushort)local_a + (ushort)local_9 * (ushort)_refptr_g_tsaPrmtFlash->bCols;
      }
      iVar1 = (int)local_298[(longlong)(int)(uint)local_a + 0xa0] *
              (int)local_298[(longlong)(int)(uint)local_9 + 0xf0];
      if (iVar1 < 0) {
        iVar1 = iVar1 + 0x3ff;
      }
      *(short *)((ulonglong)local_c * 2 + g_dif2MapCoef) = (short)(iVar1 >> 10);
      iVar1 = (int)local_298[(int)(uint)local_a] *
              (int)local_298[(longlong)(int)(uint)local_9 + 0x50];
      if (iVar1 < 0) {
        iVar1 = iVar1 + 0x3ff;
      }
      *(short *)((ulonglong)local_c * 2 + g_map2DifCoef) = (short)(iVar1 >> 10);
    }
  }
  return;
}


// ============================================================================
// Function: CMFRemap_InitMapIdxBySlotTable @ 6ba4fda8
// ============================================================================

void CMFRemap_InitMapIdxBySlotTable
               (longlong param_1,undefined8 param_2,undefined8 param_3,longlong param_4)

{
  ushort uVar1;
  ushort uVar2;
  int local_48 [4];
  int local_38 [2];
  undefined1 local_30 [4];
  int local_2c;
  uint local_28;
  byte local_21;
  uint local_20;
  uint local_1c;
  uint local_18;
  uint local_14;
  uint local_10;
  uint local_c;
  
                    /* 0xfda8  460  CMFRemap_InitMapIdxBySlotTable */
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    uVar1 = *(ushort *)(param_4 + 0x2a);
    uVar2 = *(ushort *)(param_4 + 0x28);
  }
  else {
    uVar1 = *(ushort *)(param_4 + 0x28);
    uVar2 = *(ushort *)(param_4 + 0x2a);
  }
  local_c = (uint)uVar1;
  local_10 = (uint)uVar2;
  local_38[0] = 0;
  local_38[1] = 0;
  CMFRemap_GetRxSlotNum(param_1,local_c,local_30,local_38,2);
  *(short *)(param_4 + 0x2c) = (short)local_c;
  *(short *)(param_4 + 0x2e) = (short)local_10;
  for (local_18 = 0; local_18 < local_10; local_18 = local_18 + 1) {
    local_48[0] = 0;
    local_48[1] = 0;
    for (local_14 = 0; local_14 < local_c; local_14 = local_14 + 1) {
      local_28 = (uint)*(byte *)(param_1 + (ulonglong)local_14);
      local_2c = local_48[local_28];
      if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
        local_1c = local_18 + local_14 * local_10;
      }
      else {
        local_1c = local_14 + local_18 * local_c;
      }
      local_20 = local_48[local_28] + local_18 * local_c;
      for (local_21 = 0; local_21 < local_28; local_21 = local_21 + 1) {
        local_20 = local_20 + local_38[(int)(uint)local_21];
      }
      *(short *)(*(longlong *)(param_4 + 0x38) + (ulonglong)local_20 * 2) = (short)local_1c;
      *(short *)(*(longlong *)(param_4 + 0x30) + (ulonglong)local_1c * 2) = (short)local_20;
      local_48[local_28] = local_48[local_28] + 1;
    }
  }
  return;
}


// ============================================================================
// Function: CMFRemap_Init @ 6ba503b6
// ============================================================================

void CMFRemap_Init(void)

{
  byte bVar1;
  byte bVar2;
  int iVar3;
  longlong lVar4;
  char *pcVar5;
  char local_78 [80];
  uint local_28;
  uint local_24;
  int local_20;
  int local_1c;
  
                    /* 0x103b6  458  CMFRemap_Init */
  g_cmfRemapNumRxCombine = (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x6cc;
  pcVar5 = local_78;
  for (lVar4 = 10; lVar4 != 0; lVar4 = lVar4 + -1) {
    pcVar5[0] = '\0';
    pcVar5[1] = '\0';
    pcVar5[2] = '\0';
    pcVar5[3] = '\0';
    pcVar5[4] = '\0';
    pcVar5[5] = '\0';
    pcVar5[6] = '\0';
    pcVar5[7] = '\0';
    pcVar5 = pcVar5 + 8;
  }
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    bVar2 = _refptr_g_tsaPrmtFlash->bCols;
    bVar1 = _refptr_g_tsaPrmtFlash->bRows;
  }
  else {
    bVar2 = _refptr_g_tsaPrmtFlash->bRows;
    bVar1 = _refptr_g_tsaPrmtFlash->bCols;
  }
  g_cmfRemapTxNum = (ushort)bVar2;
  g_cmfRemapRxNum = (ushort)bVar1;
  CMFRemap_InitCoef2D();
  if (_refptr_g_tsaPrmtFlash->field_0x80e == '\0') {
    local_1c = 0;
    for (local_20 = 0; local_20 < (int)(uint)g_cmfRemapTxNum; local_20 = local_20 + 1) {
      if (local_78[local_20] == '\0') {
        bVar2 = CMFRemap_Mapping(local_20,g_cmfRemapNumRxCombine,local_1c,local_78,0x50);
        local_28 = (uint)bVar2;
        if ((int)(uint)bVar2 < (int)g_cmfRemapNumRxCombine) {
          if (*_refptr_g_tsaLogLoglevel != '\0') {
            StrPrintf_Process(0,
                              "slotTable cann\'t support numRxCombine(%d) , g_cmfRemapNumRxCombine is set to 1\n"
                              ,g_cmfRemapNumRxCombine);
          }
          if (local_28 != g_cmfRemapNumRxCombine) {
            StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                              "../../src/Alg/CMF/CMFRemap.c",0x18d,"line == g_cmfRemapNumRxCombine")
            ;
            HardwareAnalyzer_SetSoftException();
          }
          g_cmfRemapNumRxCombine = 1;
          break;
        }
        local_1c = local_1c + 1;
      }
    }
    g_cmfRemapMapRxNum = g_cmfRemapRxNum * (short)g_cmfRemapNumRxCombine;
    g_cmfRemapMapTxNum = (ushort)local_1c;
    CMFRemap_GenerateIndexTable();
  }
  else {
    CMFRemap_GenerateIndexTableForDoubleIC
              (&_refptr_g_tsaPrmtFlash->field_0x67c,g_cmfRemapRxNum,g_cmfRemapTxNum);
  }
  iVar3 = CMFRemap_IsNeedRemap();
  if (iVar3 == 0) {
    for (local_24 = 0; local_24 < (uint)g_cmfRemapTxNum * (uint)g_cmfRemapRxNum;
        local_24 = local_24 + 1) {
      *(short *)(g_cmfRemapMapIdx + (ulonglong)local_24 * 2) = (short)local_24;
      *(short *)(g_cmfRemapDifIdx + (ulonglong)local_24 * 2) = (short)local_24;
    }
  }
  g_cmfRemapIsReshape = CMFRemap_IsReshape();
  if (0xff < g_cmfRemapMapRxNum) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/CMF/CMFRemap.c",
                      0x1a1,"g_cmfRemapMapRxNum < CMF_MAX_ELELEMENT_COUNT");
    HardwareAnalyzer_SetSoftException();
  }
  if (0xff < g_cmfRemapMapTxNum) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/CMF/CMFRemap.c",
                      0x1a2,"g_cmfRemapMapTxNum < CMF_MAX_ELELEMENT_COUNT");
    HardwareAnalyzer_SetSoftException();
  }
  if (0x1324 < (int)((uint)g_cmfRemapMapTxNum * (uint)g_cmfRemapMapRxNum)) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/CMF/CMFRemap.c",
                      0x1a3,"g_cmfRemapMapRxNum * g_cmfRemapMapTxNum <= TSA_MAX_SENSOR_NUM");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: CMFRemap_InitMapIdxByDefault @ 6ba50e9d
// ============================================================================

void CMFRemap_InitMapIdxByDefault(char param_1,longlong param_2)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  uint local_18;
  uint local_14;
  uint local_10;
  uint local_c;
  
                    /* 0x10e9d  459  CMFRemap_InitMapIdxByDefault */
  if (param_1 == '\0') {
    *(undefined2 *)(param_2 + 0x2e) = *(undefined2 *)(param_2 + 0x28);
    *(undefined2 *)(param_2 + 0x2c) = *(undefined2 *)(param_2 + 0x2a);
    local_14 = (uint)*(ushort *)(param_2 + 0x2e);
    while (uVar2 = local_14 - 1, local_14 != 0) {
      local_18 = (uint)*(ushort *)(param_2 + 0x2c);
      while (uVar1 = local_18 - 1, local_14 = uVar2, local_18 != 0) {
        uVar3 = uVar1 + *(ushort *)(param_2 + 0x2c) * uVar2;
        uVar4 = uVar2 + *(ushort *)(param_2 + 0x2e) * uVar1;
        *(short *)(*(longlong *)(param_2 + 0x30) + (ulonglong)uVar4 * 2) = (short)uVar3;
        *(short *)(*(longlong *)(param_2 + 0x38) + (ulonglong)uVar3 * 2) = (short)uVar4;
        local_18 = uVar1;
      }
    }
  }
  else {
    *(undefined2 *)(param_2 + 0x2e) = *(undefined2 *)(param_2 + 0x2a);
    *(undefined2 *)(param_2 + 0x2c) = *(undefined2 *)(param_2 + 0x28);
    local_c = (uint)*(ushort *)(param_2 + 0x2e);
    while (uVar2 = local_c - 1, local_c != 0) {
      local_10 = (uint)*(ushort *)(param_2 + 0x2c);
      while (uVar1 = local_10 - 1, local_c = uVar2, local_10 != 0) {
        uVar3 = uVar1 + *(ushort *)(param_2 + 0x2c) * uVar2;
        uVar4 = uVar1 + *(ushort *)(param_2 + 0x2c) * uVar2;
        *(short *)(*(longlong *)(param_2 + 0x30) + (ulonglong)uVar4 * 2) = (short)uVar3;
        *(short *)(*(longlong *)(param_2 + 0x38) + (ulonglong)uVar3 * 2) = (short)uVar4;
        local_10 = uVar1;
      }
    }
  }
  return;
}


// ============================================================================
// Function: CMF_Init2DFilter @ 6ba51699
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void CMF_Init2DFilter(void)

{
  byte bVar1;
  undefined1 local_a8 [16];
  undefined1 local_98;
  byte local_97;
  undefined8 local_88;
  int local_48 [5];
  undefined1 local_34 [4];
  undefined *local_30;
  uint local_28;
  int local_24;
  uint local_20;
  uint local_1c;
  ushort local_16;
  ushort local_14;
  byte local_12;
  byte local_11;
  int local_10;
  byte local_9;
  
                    /* 0x11699  475  CMF_Init2DFilter */
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    bVar1 = _refptr_g_tsaPrmtFlash->bRows;
  }
  else {
    bVar1 = _refptr_g_tsaPrmtFlash->bCols;
  }
  local_14 = (ushort)bVar1;
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    bVar1 = _refptr_g_tsaPrmtFlash->bCols;
  }
  else {
    bVar1 = _refptr_g_tsaPrmtFlash->bRows;
  }
  local_16 = (ushort)bVar1;
  local_48[0] = 0;
  local_48[1] = 0;
  local_1c = CMFRemap_GetRxSlotNum
                       (&_refptr_g_tsaPrmtFlash->field_0x67c,local_14,local_34,local_48,2);
  local_20 = CMFRemap_GetTxSlotNum(&_refptr_g_tsaPrmtFlash->field_0x62c,local_16);
  local_97 = *_refptr_g_tsaPrmtFlashCmfPca;
  for (local_9 = 0; local_9 < local_20; local_9 = local_9 + 1) {
    local_10 = 0;
    for (local_11 = 0; local_11 < local_1c; local_11 = local_11 + 1) {
      local_24 = local_48[(int)(uint)local_11];
      local_28 = (uint)local_97 * ((uint)local_14 * (uint)local_9 + local_10);
      *(undefined **)
       (&g_pcaCoef + ((longlong)(int)(uint)local_11 + (longlong)(int)(uint)local_9 * 2) * 8) =
           _refptr_g_tsaPrmtFlashCmfPca + (ulonglong)local_28 * 4 + 4;
      local_88 = *(undefined8 *)
                  (&g_pcaCoef +
                  ((longlong)(int)(uint)local_11 + (longlong)(int)(uint)local_9 * 2) * 8);
      local_98 = (undefined1)local_24;
      MtLsm_CalcMatrixB(&g_pcaMatrixB +
                        (longlong)(int)(uint)local_11 * 800 + (longlong)(int)(uint)local_9 * 0x640,
                        &g_pcaCoefSqSum +
                        (longlong)(int)(uint)local_11 * 100 + (longlong)(int)(uint)local_9 * 200,400
                        ,local_a8);
      local_10 = local_10 + local_24;
    }
  }
  _g_param = 1;
  DAT_6bb800a8 = (ushort)_refptr_g_tsaPrmtFlash->bCols;
  DAT_6bb800aa = (ushort)_refptr_g_tsaPrmtFlash->bRows;
  _DAT_6bb800b8 = g_difIdx;
  _DAT_6bb800b0 = g_mapIdx;
  CMFRemap_InitMapIdxBySlotTable
            (&_refptr_g_tsaPrmtFlash->field_0x67c,_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass,
             _refptr_g_tsaPrmtFlash->field_0x6cc,&g_param);
  for (local_12 = 1; (int)(uint)local_12 < (int)(local_14 - 1); local_12 = local_12 + 1) {
    local_30 = &g_cmfNeighborCoefArray + (longlong)(int)((local_12 - 1) * 9) * 2;
    CMF_GetNeighborCoef(local_30,local_12 - 1,local_12,local_12 + 1);
  }
  return;
}


// ============================================================================
// Function: CMF_InitWeight @ 6ba5213d
// ============================================================================

void CMF_InitWeight(short param_1,short param_2,longlong param_3)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  short local_428 [256];
  short local_228 [256];
  uint local_28;
  byte local_23;
  byte local_22;
  byte local_21;
  longlong local_20;
  longlong local_18;
  uint local_10;
  byte local_a;
  byte local_9;
  
                    /* 0x1213d  476  CMF_InitWeight */
  local_18 = *(longlong *)(param_3 + 0x10);
  local_20 = *(longlong *)(param_3 + 0x40);
  local_21 = (byte)*(undefined2 *)(param_3 + 0x28);
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    local_22 = (byte)*(undefined2 *)(param_3 + 0x2a);
  }
  else {
    local_22 = (byte)*(undefined2 *)(param_3 + 0x28);
  }
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    local_23 = (byte)*(undefined2 *)(param_3 + 0x28);
  }
  else {
    local_23 = (byte)*(undefined2 *)(param_3 + 0x2a);
  }
  local_10 = (uint)local_23 * (uint)local_22;
  while (uVar1 = local_10 - 1, local_10 != 0) {
    *(undefined2 *)(local_20 + (ulonglong)uVar1 * 2) = 1;
    local_10 = uVar1;
  }
  for (local_a = 0; local_a < local_23; local_a = local_a + 1) {
    local_228[0] = 0;
    for (local_9 = 1; local_9 < local_22; local_9 = local_9 + 1) {
      if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
        iVar3 = (uint)local_21 * (uint)local_9 + (uint)local_a;
      }
      else {
        iVar3 = (uint)local_21 * (uint)local_a + (uint)local_9;
      }
      if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
        iVar2 = (uint)local_21 * (local_9 - 1) + (uint)local_a;
      }
      else {
        iVar2 = (uint)local_21 * (uint)local_a + (local_9 - 1);
      }
      local_228[(int)(uint)local_9] =
           *(short *)((longlong)iVar3 * 2 + local_18) - *(short *)((longlong)iVar2 * 2 + local_18);
    }
    if (local_22 != 0) {
      local_428[(int)(local_22 - 1)] = 0;
      local_428[0] = local_428[(int)(local_22 - 1)];
      for (local_9 = 1; (int)(uint)local_9 < (int)(local_22 - 1); local_9 = local_9 + 1) {
        local_428[(int)(uint)local_9] =
             local_228[(int)(local_9 + 1)] - local_228[(int)(uint)local_9];
      }
      for (local_9 = 0; local_9 < local_22; local_9 = local_9 + 1) {
        if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
          iVar3 = (uint)local_21 * (uint)local_9 + (uint)local_a;
        }
        else {
          iVar3 = (uint)local_21 * (uint)local_a + (uint)local_9;
        }
        local_28 = (uint)*(short *)((longlong)iVar3 * 2 + local_18);
        if (((int)param_1 < (int)(((int)local_28 >> 0x1f ^ local_28) - ((int)local_28 >> 0x1f))) ||
           (uVar4 = (int)local_428[(int)(uint)local_9] >> 0x1f,
           (int)param_2 < (int)(((int)local_428[(int)(uint)local_9] ^ uVar4) - uVar4))) {
          if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
            iVar3 = (uint)local_21 * (uint)local_9 + (uint)local_a;
          }
          else {
            iVar3 = (uint)local_21 * (uint)local_a + (uint)local_9;
          }
          *(undefined2 *)((longlong)iVar3 * 2 + local_20) = 0;
        }
      }
    }
  }
  local_10 = uVar1;
  Image_Erosion(local_20,*(undefined2 *)(param_3 + 0x28),*(undefined2 *)(param_3 + 0x2a),1,1);
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    Image_Dilation(local_20,*(undefined2 *)(param_3 + 0x28),*(undefined2 *)(param_3 + 0x2a),0,1);
  }
  else {
    Image_Dilation(local_20,*(undefined2 *)(param_3 + 0x28),*(undefined2 *)(param_3 + 0x2a),1,0);
  }
  return;
}


// ============================================================================
// Function: CMF_InitWeightByNeighbor @ 6ba529b7
// ============================================================================

/* WARNING: Function: ___chkstk_ms replaced with injection: alloca_probe */
/* WARNING: Removing unreachable block (ram,0x6ba52eef) */

void CMF_InitWeightByNeighbor
               (ushort param_1,longlong param_2,int param_3,longlong param_4,longlong param_5)

{
  uint uVar1;
  uint uVar2;
  double dVar3;
  int local_27f8 [80];
  short local_26b8 [4903];
  ushort local_6a;
  int local_68;
  ushort local_62;
  ushort local_60;
  ushort local_5e;
  int local_5c;
  int local_58;
  int local_54;
  int local_50;
  int local_4c;
  int local_48;
  byte local_42;
  byte local_41;
  undefined2 *local_40;
  byte local_32;
  byte local_31;
  int local_30;
  byte local_29;
  undefined *local_28;
  ushort local_1e;
  int local_1c;
  byte local_15;
  int local_14;
  undefined2 *local_10;
  
  local_10 = (undefined2 *)0x6ba529c2;
  CMFRemap_MapBuffer(local_26b8,0x1324,*(undefined8 *)(param_2 + 0x10),param_2);
  local_40 = *(undefined2 **)(param_2 + 0x40);
  local_41 = (byte)*(undefined2 *)(param_2 + 0x2c);
  local_42 = (byte)*(undefined2 *)(param_2 + 0x2e);
  local_48 = (uint)param_1 * (uint)param_1;
  local_4c = (int)(short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax;
  local_14 = (uint)local_41 * (uint)local_42;
  local_10 = local_40;
  while (local_14 = local_14 + -1, local_14 != 0) {
    *local_10 = 1;
    local_10 = local_10 + 1;
  }
  local_14 = 0;
  for (local_15 = 0; local_15 < local_42; local_15 = local_15 + 1) {
    if ((param_4 == 0) || (*(char *)(param_4 + (ulonglong)local_15) != '\0')) {
      local_1c = 0;
      if (param_5 != 0) {
        *(undefined1 *)(param_5 + (ulonglong)local_15) = 1;
      }
      local_1e = (ushort)local_41 * (ushort)local_15 + 1;
      local_28 = &g_cmfNeighborCoefArray;
      for (local_29 = 1; (int)(uint)local_29 < (int)(local_41 - 1); local_29 = local_29 + 1) {
        local_50 = (int)local_26b8[(int)(local_1e - 1)];
        local_54 = (int)local_26b8[(int)(uint)local_1e];
        local_1e = local_1e + 1;
        local_58 = (int)local_26b8[(int)(uint)local_1e];
        local_5c = CMF_GetDistance(local_50,local_54,local_58,local_28);
        local_27f8[(int)(uint)local_29] = local_5c;
        local_28 = local_28 + 0x12;
      }
      local_30 = 1;
      local_1e = (ushort)local_41 * (ushort)local_15;
      for (local_31 = 1; local_1e = local_1e + 1, (int)(uint)local_31 < (int)(local_41 - 1);
          local_31 = local_31 + 1) {
        local_5e = *(ushort *)(*(longlong *)(param_2 + 0x38) + (ulonglong)local_1e * 2);
        if (local_48 < local_27f8[(int)(uint)local_31]) {
          local_30 = 0;
          local_40[local_5e] = 0;
          local_1c = local_1c + 1;
        }
        else if ((param_3 != 0) &&
                (uVar1 = (uint)*(short *)(*(longlong *)_refptr_g_tsaBufPreDif +
                                         (ulonglong)local_5e * 2), uVar2 = (int)uVar1 >> 0x1f,
                local_4c < (int)((uVar1 ^ uVar2) - uVar2))) {
          local_40[local_5e] = 0;
          local_1c = local_1c + 1;
        }
      }
      if ((param_5 != 0) && (((int)(local_41 - 5) <= local_1c || (local_30 != 0)))) {
        *(undefined1 *)(param_5 + (ulonglong)local_15) = 0;
      }
      local_27f8[0] = local_27f8[1];
      local_27f8[(int)(local_41 - 1)] = local_27f8[(int)(local_41 - 2)];
      local_40[*(ushort *)
                (*(longlong *)(param_2 + 0x38) +
                (longlong)(int)((uint)local_41 * (uint)local_15) * 2)] =
           local_40[*(ushort *)
                     (*(longlong *)(param_2 + 0x38) +
                     (longlong)(int)((uint)local_41 * (uint)local_15 + 1) * 2)];
      local_60 = *(ushort *)
                  (*(longlong *)(param_2 + 0x38) +
                  (longlong)(int)((uint)local_41 * (uint)local_15 + (local_41 - 1)) * 2);
      local_62 = *(ushort *)
                  (*(longlong *)(param_2 + 0x38) +
                  (longlong)(int)((uint)local_41 * (uint)local_15 + (local_41 - 2)) * 2);
      local_40[local_60] = local_40[local_62];
      if ((int)((uint)local_41 * 2 + (uint)local_41) >> 2 < local_1c) {
        local_68 = Misc_FindKthSmallest(local_27f8,local_41,local_41 >> 2);
        dVar3 = sqrt((double)local_68);
        local_68 = ((uint)param_1 + (int)dVar3) * ((uint)param_1 + (int)dVar3);
        for (local_32 = 0; local_32 < local_41; local_32 = local_32 + 1) {
          local_1e = (ushort)local_41 * (ushort)local_15 + (ushort)local_32;
          local_6a = *(ushort *)(*(longlong *)(param_2 + 0x38) + (ulonglong)local_1e * 2);
          local_40[local_6a] = (ushort)(local_27f8[(int)(uint)local_32] <= local_68);
        }
      }
      if (*(longlong *)(param_2 + 0x48) != 0) {
        CMF_UpdateNeighborDistance
                  (*(undefined8 *)(param_2 + 0x48),local_27f8,local_41,local_15,param_2);
      }
    }
  }
  CMF_CrossErosionWeight(local_40,*(undefined2 *)(param_2 + 0x28),*(undefined2 *)(param_2 + 0x2a));
  CMF_SetNotchToZero(local_40,(uint)*(ushort *)(param_2 + 0x2a) * (uint)*(ushort *)(param_2 + 0x28))
  ;
  CMF_ConditionClearCmfToPreDif(param_5,local_42,param_2,local_40);
  return;
}


// ============================================================================
// Function: CTD_ECInit @ 6ba553a0
// ============================================================================

void CTD_ECInit(void)

{
                    /* 0x153a0  489  CTD_ECInit */
  return;
}


// ============================================================================
// Function: ER_Init @ 6ba56f76
// ============================================================================

void ER_Init(void)

{
  undefined2 uVar1;
  undefined4 uVar2;
  undefined1 local_c [2];
  undefined1 local_a [2];
  
                    /* 0x16f76  544  ER_Init */
  uVar1 = ER_MMPrmtToPixelPrmt
                    (*(undefined2 *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x44),1);
  *(undefined2 *)(_refptr_g_tsaPrmtFlashConst + 0x40) = uVar1;
  uVar1 = ER_MMPrmtToPixelPrmt
                    (*(undefined2 *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x46),1);
  *(undefined2 *)(_refptr_g_tsaPrmtFlashConst + 0x42) = uVar1;
  uVar1 = ER_MMPrmtToPixelPrmt
                    (*(undefined2 *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x4a),2);
  *(undefined2 *)(_refptr_g_tsaPrmtFlashConst + 0x46) = uVar1;
  uVar1 = ER_MMPrmtToPixelPrmt
                    (*(undefined2 *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x4c),2);
  *(undefined2 *)(_refptr_g_tsaPrmtFlashConst + 0x48) = uVar1;
  uVar1 = ER_MMPrmtToPixelPrmt(*(undefined2 *)(_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 10),4);
  *(undefined2 *)(_refptr_g_tsaPrmtFlashConst + 0x4c) = uVar1;
  uVar1 = ER_MMPrmtToPixelPrmt
                    (*(undefined2 *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x52),4);
  *(undefined2 *)(_refptr_g_tsaPrmtFlashConst + 0x4e) = uVar1;
  uVar1 = ER_MMPrmtToPixelPrmt
                    (*(undefined2 *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x56),8);
  *(undefined2 *)(_refptr_g_tsaPrmtFlashConst + 0x52) = uVar1;
  uVar1 = ER_MMPrmtToPixelPrmt(*(undefined2 *)(_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0xb),8);
  *(undefined2 *)(_refptr_g_tsaPrmtFlashConst + 0x54) = uVar1;
  uVar2 = TSA_SideToDimEdge(1);
  ER_GetPrmt(uVar2,local_a,local_c);
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
// Function: FIException_Init @ 6ba593c2
// ============================================================================

void FIException_Init(void)

{
                    /* 0x193c2  593  FIException_Init */
  *(undefined2 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x42 = 0;
  *(undefined8 *)((longlong)&(*_refptr_g_tsaStaticPtr)[1].llField_0x48 + 2) = 0;
  *(undefined4 *)((longlong)&(*_refptr_g_tsaStaticPtr)[1].llField_0x50 + 2) = 0;
  *(undefined1 *)((longlong)&(*_refptr_g_tsaStaticPtr)[1].llField_0x50 + 6) = 0;
  *(undefined4 *)((longlong)&(*_refptr_g_tsaStaticPtr)[1].llField_0x58 + 2) = 0;
  return;
}


// ============================================================================
// Function: HIException_Init @ 6ba5bf39
// ============================================================================

void HIException_Init(void)

{
                    /* 0x1bf39  801  HIException_Init */
  *(undefined2 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x1a = 0;
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x22 = 0;
  *(undefined4 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x2a = 0;
  (*_refptr_g_tsaStaticPtr)[1].field_0x2e = 0;
  *(undefined4 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x32 = 0;
  return;
}


// ============================================================================
// Function: RxLineFilter_Init @ 6ba60440
// ============================================================================

void RxLineFilter_Init(void)

{
                    /* 0x20440  1173  RxLineFilter_Init */
  return;
}


// ============================================================================
// Function: IDT_Init @ 6ba611a0
// ============================================================================

void IDT_Init(void)

{
                    /* 0x211a0  884  IDT_Init */
  return;
}


// ============================================================================
// Function: Matrix_Init @ 6ba63b9a
// ============================================================================

void Matrix_Init(longlong param_1,byte param_2,byte param_3,longlong param_4)

{
  uint local_c;
  
                    /* 0x23b9a  908  Matrix_Init */
  Matrix_ReShape(param_1,param_2,param_3);
  if (param_4 == 0) {
    for (local_c = 0; local_c < (uint)param_3 * (uint)param_2; local_c = local_c + 1) {
      *(undefined4 *)(param_1 + (ulonglong)local_c * 4) = _rdata;
    }
  }
  else {
    for (local_c = 0; local_c < (uint)param_3 * (uint)param_2; local_c = local_c + 1) {
      *(undefined4 *)(param_1 + (ulonglong)local_c * 4) =
           *(undefined4 *)(param_4 + (ulonglong)local_c * 4);
    }
  }
  return;
}


// ============================================================================
// Function: Matrix_Init_I @ 6ba63c42
// ============================================================================

undefined8 Matrix_Init_I(longlong param_1,byte param_2)

{
  int local_c;
  
                    /* 0x23c42  909  Matrix_Init_I */
  Matrix_Init(param_1,param_2,param_2,0);
  for (local_c = 0; local_c < (int)(uint)param_2; local_c = local_c + 1) {
    *(undefined4 *)(param_1 + (longlong)(int)((*(byte *)(param_1 + 0x65) + 1) * local_c) * 4) =
         DAT_6bb52380;
  }
  return 0;
}


// ============================================================================
// Function: PrevPeak_Init @ 6ba68f10
// ============================================================================

void PrevPeak_Init(void)

{
  int iVar1;
  
                    /* 0x28f10  1041  PrevPeak_Init */
  iVar1 = memset_s(g_prevPeaksPtr,0xf4,0,0xf4);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/Peak/PrevPeakData.c",0x14,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
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
// Function: AccQueue_Init @ 6ba6aed0
// ============================================================================

void AccQueue_Init(undefined8 param_1,undefined2 param_2)

{
                    /* 0x2aed0  325  AccQueue_Init */
  g_accQueue = param_1;
  DAT_6bbc0e08 = param_2;
  DAT_6bbc0e0c = 0;
  DAT_6bbc0e0a = 0;
  return;
}


// ============================================================================
// Function: SensorData_Init @ 6ba6b220
// ============================================================================

void SensorData_Init(void)

{
                    /* 0x2b220  1263  SensorData_Init */
  AccQueue_Init(&g_accQueueBuff,0x100);
  DAT_6bb4f172 = 1;
  DAT_6bb4f173 = 1;
  DAT_6bb4f190 = 0;
  DAT_6bb4f178 = 0;
  DAT_6bb4f180 = 0;
  DAT_6bb4f188 = 0;
  DAT_6bb4f170 = 0;
  DAT_6bb4f171 = 0;
  return;
}


// ============================================================================
// Function: TPSensor_Init @ 6ba6be2e
// ============================================================================

void TPSensor_Init(void)

{
  byte bVar1;
  TSAFlashPrmt *pTVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  int local_10;
  int local_c;
  
                    /* 0x2be2e  1464  TPSensor_Init */
  iVar3 = memset_s(g_tpSensorDim1Offset,*(undefined2 *)_refptr_g_gridBufSize,0,
                   _refptr_g_tsaPrmtFlash->wBufElementCount);
  if (iVar3 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/TPSensor/TPSensor.c",0xaa,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar3 = memset_s(g_tpSensorDim2Offset,*(undefined2 *)_refptr_g_gridBufSize,0,
                   _refptr_g_tsaPrmtFlash->wBufElementCount);
  if (iVar3 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/TPSensor/TPSensor.c",0xac,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  bVar1 = *(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x6c);
  for (local_10 = 0; pTVar2 = _refptr_g_tsaPrmtFlash, local_10 < (int)(uint)bVar1;
      local_10 = local_10 + 1) {
    uVar4 = (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                           (longlong)local_10 * 5 + 0x6d);
    uVar5 = (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                           (longlong)local_10 * 5 + 0x6e);
    if ((*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + (longlong)local_10 * 5 + 0x6f
                  ) & 1) != 0) {
      *(undefined1 *)
       (g_tpSensorDim1Offset + (ulonglong)(_refptr_g_tsaPrmtFlash->bCols * uVar5 + uVar4 & 0xffff))
           = *(undefined1 *)
              ((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + (longlong)local_10 * 5 + 0x70);
      *(undefined1 *)
       (g_tpSensorDim2Offset + (ulonglong)(_refptr_g_tsaPrmtFlash->bCols * uVar5 + uVar4 & 0xffff))
           = *(undefined1 *)((longlong)pTVar2[1].pDim1PitchMap + (longlong)local_10 * 5 + 0x71);
    }
  }
  NotchCorrect_Init();
  for (local_c = 0; local_c < (int)(uint)_refptr_g_tsaPrmtFlash->wBufElementCount;
      local_c = local_c + 1) {
    *(undefined2 *)(g_tpSensorDifScale + (longlong)local_c * 2) = 0x400;
  }
  bVar1 = *(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x12d);
  for (local_10 = 0; local_10 < (int)(uint)bVar1; local_10 = local_10 + 1) {
    *(undefined2 *)
     (g_tpSensorDifScale +
     (ulonglong)
     ((uint)_refptr_g_tsaPrmtFlash->bCols *
      (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                     (longlong)local_10 * 6 + 0x12f) +
      (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                     (longlong)local_10 * 6 + 0x12e) & 0xffff) * 2) =
         *(undefined2 *)
          ((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + (longlong)local_10 * 6 + 0x130);
  }
  return;
}


// ============================================================================
// Function: NotchCorrect_Init @ 6ba6c982
// ============================================================================

void NotchCorrect_Init(void)

{
  int iVar1;
  uint uVar2;
  int local_10;
  int local_c;
  
                    /* 0x2c982  943  NotchCorrect_Init */
  for (local_c = 0; local_c < (int)(uint)_refptr_g_tsaPrmtFlash->wBufElementCount;
      local_c = local_c + 1) {
    *(byte *)(*(longlong *)_refptr_g_tpSensorFlag + (longlong)local_c) =
         *(byte *)((longlong)local_c + *(longlong *)_refptr_g_tpSensorFlag) & 0xdf;
    *(byte *)(*(longlong *)_refptr_g_tpSensorFlag + (longlong)local_c) =
         *(byte *)((longlong)local_c + *(longlong *)_refptr_g_tpSensorFlag) & 0xbf;
  }
  iVar1 = NotchCorrect_GetNotchNum();
  for (local_10 = 0; local_10 < iVar1; local_10 = local_10 + 1) {
    uVar2 = (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                           (longlong)local_10 * 3 + 0x105) +
            (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                           (longlong)local_10 * 3 + 0x106) * (uint)_refptr_g_tsaPrmtFlash->bCols &
            0xffff;
    *(byte *)(*(longlong *)_refptr_g_tpSensorFlag + (longlong)(int)uVar2) =
         *(byte *)((longlong)(int)uVar2 + *(longlong *)_refptr_g_tpSensorFlag) | 0x20;
    if (*(char *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + (longlong)local_10 * 3 + 0x107
                 ) < '\0') {
      *(byte *)(*(longlong *)_refptr_g_tpSensorFlag + (longlong)(int)uVar2) =
           *(byte *)((longlong)(int)uVar2 + *(longlong *)_refptr_g_tpSensorFlag) | 0x40;
    }
  }
  return;
}


// ============================================================================
// Function: PrevTouch_Init @ 6ba6db42
// ============================================================================

void PrevTouch_Init(void)

{
                    /* 0x2db42  1062  PrevTouch_Init */
  PrevTouch_Release();
  return;
}


// ============================================================================
// Function: Touch_Init @ 6ba6f503
// ============================================================================

void Touch_Init(void)

{
                    /* 0x2f503  2532  Touch_Init */
  Touch_Clean();
  return;
}


// ============================================================================
// Function: TouchAction_Init @ 6ba77220
// ============================================================================

void TouchAction_Init(void)

{
                    /* 0x37220  2484  TouchAction_Init */
  return;
}


// ============================================================================
// Function: TE_Init @ 6ba77530
// ============================================================================

void TE_Init(void)

{
                    /* 0x37530  1460  TE_Init */
  return;
}


// ============================================================================
// Function: TouchMode_Init @ 6ba78b60
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void TouchMode_Init(void)

{
                    /* 0x38b60  2492  TouchMode_Init */
  *(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x2c = 0;
  *(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x34 = 0;
  *(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x30 = 0;
  _refptr_g_tsaPrpt[1].field_0x28 = 0;
  SWTimer_Stop(&g_tmSwitchTimer);
  _DAT_6bb4f202 = 0;
  if (_refptr_g_tsaPrmtFlash->field_0x44 == '\0') {
    _DAT_6bb4f208 = 0x3c;
  }
  else {
    _DAT_6bb4f208 = 0;
  }
  _DAT_6bb4f204 = 0x14;
  _DAT_6bb4f210 = 200;
  _DAT_6bb4f212 = 0;
  (*_refptr_g_tsaStaticPtr)[1].field_0xc = 0;
  return;
}


// ============================================================================
// Function: TouchReport_Init @ 6ba7a120
// ============================================================================

void TouchReport_Init(void)

{
                    /* 0x3a120  2500  TouchReport_Init */
  HotZoneAdjust_Init();
  return;
}


// ============================================================================
// Function: TS_Init @ 6ba7c000
// ============================================================================

void TS_Init(void)

{
                    /* 0x3c000  2343  TS_Init */
  return;
}


// ============================================================================
// Function: TouchThold_Init @ 6ba7c922
// ============================================================================

void TouchThold_Init(void)

{
  undefined2 uVar1;
  
                    /* 0x3c922  2508  TouchThold_Init */
  uVar1 = TouchThold_GetTholdBasedOnSNR
                    ((int)*(short *)(_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 6));
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
// Function: SubTZ_UnitInit @ 6ba7d2e0
// ============================================================================

void SubTZ_UnitInit(undefined2 *param_1)

{
  int iVar1;
  
  iVar1 = memset_s(param_1,0x48,0,0x48);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/TouchZone/SubTouchZone/SubTouchZone.c",0x1d,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  *param_1 = 0x8000;
  *(undefined1 *)((longlong)param_1 + 0x39) = 0xff;
  *(undefined1 *)((longlong)param_1 + 0x3b) = 0xff;
  return;
}


// ============================================================================
// Function: TZ_UnitInit @ 6ba7f620
// ============================================================================

void TZ_UnitInit(undefined8 param_1)

{
  int iVar1;
  
                    /* 0x3f620  2449  TZ_UnitInit */
  iVar1 = memset_s(param_1,0x370,0,0x370);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/TouchZone/TZ_Common.c",0x2c,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: TZ_ContourInit @ 6ba8590b
// ============================================================================

void TZ_ContourInit(void)

{
  double dVar1;
  float fVar2;
  int local_c;
  
                    /* 0x4590b  2363  TZ_ContourInit */
  for (local_c = 0; local_c < 5; local_c = local_c + 1) {
    fVar2 = (float)local_c / DAT_6bb52ef0;
    dVar1 = (double)SinXDivX((double)(fVar2 + DAT_6bb52ef4));
    *(float *)(&g_afu + (longlong)local_c * 0x10) = (float)dVar1;
    dVar1 = (double)SinXDivX((double)fVar2);
    *(float *)(&DAT_6bc13984 + (longlong)local_c * 0x10) = (float)dVar1;
    dVar1 = (double)SinXDivX((double)(DAT_6bb52ef4 - fVar2));
    *(float *)(&DAT_6bc13988 + (longlong)local_c * 0x10) = (float)dVar1;
    dVar1 = (double)SinXDivX((double)(DAT_6bb52ee0 - fVar2));
    *(float *)(&DAT_6bc1398c + (longlong)local_c * 0x10) = (float)dVar1;
  }
  return;
}


// ============================================================================
// Function: TZ_UnitMaxMinInit @ 6ba87d7e
// ============================================================================

void TZ_UnitMaxMinInit(undefined2 *param_1,undefined2 param_2)

{
                    /* 0x47d7e  2450  TZ_UnitMaxMinInit */
  *(undefined1 *)((longlong)param_1 + 0x33) = 0xff;
  *(undefined1 *)((longlong)param_1 + 0x35) = 0xff;
  *(undefined1 *)((longlong)param_1 + 0x37) = 0x7f;
  *(undefined1 *)((longlong)param_1 + 0x39) = 0x7f;
  *(undefined1 *)((longlong)param_1 + 0x3b) = 0x7f;
  *(undefined1 *)((longlong)param_1 + 0x3d) = 0x7f;
  *(undefined1 *)(param_1 + 0x1a) = 0;
  *(undefined1 *)(param_1 + 0x1b) = 0;
  *(undefined1 *)(param_1 + 0x1c) = 0x80;
  *(undefined1 *)(param_1 + 0x1d) = 0x80;
  *(undefined1 *)(param_1 + 0x1e) = 0x80;
  *(undefined1 *)(param_1 + 0x1f) = 0x80;
  param_1[0x16e] = 0x8000;
  param_1[0x16f] = 0x8000;
  param_1[0x170] = 0x8000;
  param_1[0x171] = 0x8000;
  *param_1 = param_2;
  *(undefined1 *)((longlong)param_1 + 0x35f) = 0;
  *(undefined1 *)(param_1 + 0x1b0) = 0;
  param_1[0x1b1] = 0x7fff;
  param_1[0x1b2] = 0x7fff;
  param_1[0x19b] = 0x8000;
  param_1[0x19c] = 0x7fff;
  param_1[0x19d] = 0x7fff;
  return;
}


// ============================================================================
// Function: TZ_Init @ 6ba8aa70
// ============================================================================

void TZ_Init(void)

{
                    /* 0x4aa70  2404  TZ_Init */
  TZ_VPInit();
  TZ_ContourInit();
  *(undefined8 *)&_refptr_g_tsaPrpt[1].field_0xc = 0xffffffffffffffff;
  _refptr_g_tzAccess[9] = 0x10;
  *(undefined8 *)_refptr_g_tzAccess = *(undefined8 *)_refptr_g_tsaBufTz;
  *(undefined **)_refptr_g_ptrAccess = _refptr_g_tzAccess;
  return;
}


// ============================================================================
// Function: TZ_VPInit @ 6ba8d320
// ============================================================================

void TZ_VPInit(void)

{
                    /* 0x4d320  2454  TZ_VPInit */
  (*_refptr_g_tsaStaticPtr)[1].field_0xe = 0;
  (*_refptr_g_tsaStaticPtr)[1].field_0xf = 0;
  return;
}


// ============================================================================
// Function: TSABuffer_Init @ 6ba8e183
// ============================================================================

void TSABuffer_Init(void)

{
  int iVar1;
  ushort local_c;
  ushort local_a;
  
                    /* 0x4e183  1480  TSABuffer_Init */
  iVar1 = memset_s(g_tsaBufTzPrev,g_gridBufSize,0,_refptr_g_tsaPrmtFlash->wBufElementCount);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_Buffer.c",
                      0x1d2,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(*(undefined8 *)_refptr_g_tsaBufTzSide,g_gridBufSize,0,g_gridBufSize);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_Buffer.c",
                      0x1d5,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(g_tsaBufTzZonePrev,g_gridBufSize,0,_refptr_g_tsaPrmtFlash->wBufElementCount);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_Buffer.c",
                      0x1d8,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(g_tsaBufTzID,g_gridBufSize,0,_refptr_g_tsaPrmtFlash->wBufElementCount);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_Buffer.c",
                      0x1db,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(g_tsaBufTzZone,g_gridBufSize,0,_refptr_g_tsaPrmtFlash->wBufElementCount);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_Buffer.c",
                      0x1df,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(g_tsaBufSubTZ,g_gridBufSize,0,_refptr_g_tsaPrmtFlash->wBufElementCount);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_Buffer.c",
                      0x1e3,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  for (local_a = 0; local_a < _refptr_g_tsaPrmtFlash->wBufElementCount; local_a = local_a + 1) {
    *(undefined2 *)(g_tsaBufTimeOff + (ulonglong)local_a * 2) = 0x7fff;
    *(undefined2 *)(g_tsaBufTimeRpt + (ulonglong)local_a * 2) = 0x7fff;
  }
  iVar1 = memset_s(g_tsaBufTimeDown,(ulonglong)g_gridBufSize * 2,0,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_Buffer.c",
                      0x1ea,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(g_tsaBufMinSigOff,(ulonglong)g_gridBufSize * 2,0,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_Buffer.c",
                      0x1ec,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(g_tsaBufMaxSigDown,(ulonglong)g_gridBufSize * 2,0,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_Buffer.c",
                      0x1ef,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(g_tsaBufSilentTouchNum,g_gridBufSize,0,_refptr_g_tsaPrmtFlash->wBufElementCount);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_Buffer.c",
                      0x1f2,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(g_tsaBufGripStage,g_gridBufSize,0,_refptr_g_tsaPrmtFlash->wBufElementCount);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_Buffer.c",
                      0x1f6,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(g_tsaBufGripRegion,g_gridBufSize,0,_refptr_g_tsaPrmtFlash->wBufElementCount);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_Buffer.c",
                      0x1f8,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(g_tsaBufGripTime,(ulonglong)g_gridBufSize * 2,0,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_Buffer.c",
                      0x1fa,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  for (local_c = 0; local_c < _refptr_g_tsaPrmtFlash->wBufElementCount; local_c = local_c + 1) {
    *(undefined2 *)(g_tsaBufGripTimeOff + (ulonglong)local_c * 2) = 0x7fff;
  }
  return;
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
// Function: TSA_GridDataRawInit @ 6ba917bb
// ============================================================================

void TSA_GridDataRawInit(int param_1)

{
  errno_t eVar1;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = *(undefined8 *)_refptr_g_tsaBufRaw;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"Raw");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"PreRaw");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"PreUnifiedRaw");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"PreDiff");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"DIFF");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"PreUnifiedDiff");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"PreHWRaw");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"Raw2RawDiff");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"Raw2RawDiffAftCMF");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"Raw2RawDiff2");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"Raw2RawDiff3");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"PreCMFDiff");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"AftSSCMFDiff");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"AftSSChargerNFilterDiff");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"CommonModeNoise");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"AftCMFDiff");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"CMFDraftNeighborDistance");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"CMFNeighborDistance");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"AftPcaFilterDif");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"CMFWeight");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"CMFDraftWeight");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"AftDim1CMFDiff");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"OneOrderDiffRx");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"TwoOrderDiffRx");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"PreNotchCorrectDiff");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"AftNotchCorrectDiff");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"GridIIRDif");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"SD");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"Baseline");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"NormalBaseline");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"CMFRemap");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"CMFRemapBack");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"TZFlag");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"DistanceMap");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"AfterDilate");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"AfterErode");
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
  
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"ProxDif");
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
  
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"ProxIIRRaw");
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
  
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"ProxZnDif");
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
  
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"ProxBaseline");
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
  
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"ProxTZ");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"SideTZ");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"SideTouch");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"SafeBaseline");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"DifSafeBL2Raw");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"DifSafeBL2RawCMF");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"DifSafeBL2BL");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"DifSafeBL2BLCMF");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"MaxSigDown");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"MinSigOff");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"TouchOnTime");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"TouchRptTime");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"TouchOffTime");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"SilentTouchNum");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"tzID");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"TzZone");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"subTZ");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"GripRegion");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"gripStage");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"gripTime");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"gripTimeOff");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"HandGestureBaseline");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"HandGestureDif");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"HandTZ");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"HandArea");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"HandTZPrev");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"HandTime");
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
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"tzAge(In ms)");
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
// Function: TSA_GridDataUnknownInit @ 6ba98ddd
// ============================================================================

void TSA_GridDataUnknownInit(int param_1)

{
  errno_t eVar1;
  undefined8 uVar2;
  
  *(undefined8 *)(&g_ideGridData + (longlong)param_1 * 0x88) = *(undefined8 *)_refptr_g_tsaBufDif;
  eVar1 = strcpy_s((char *)((longlong)param_1 * 0x88 + 0x6bc44f48),100,"Unknown");
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
// Function: TSA_PythonInit @ 6baa02bd
// ============================================================================

undefined8 TSA_PythonInit(longlong param_1,undefined8 param_2)

{
  undefined8 uVar1;
  
                    /* 0x602bd  2125  TSA_PythonInit */
  if (param_1 == 0) {
    uVar1 = 0;
  }
  else {
    uVar1 = Python_Init(param_1,param_2);
  }
  return uVar1;
}


// ============================================================================
// Function: TSAMem_InitProject @ 6baa065d
// ============================================================================

bool TSAMem_InitProject(void)

{
  byte bVar1;
  char cVar2;
  
                    /* 0x6065d  1609  TSAMem_InitProject */
  TSAMem_Free();
  *(ushort *)_refptr_g_gridBufSize = _refptr_g_tsaPrmtFlash->wBufElementCount;
  *(short *)_refptr_g_gridMapBufSize =
       (short)(((uint)*(ushort *)_refptr_g_gridBufSize * 2 + (uint)*(ushort *)_refptr_g_gridBufSize)
              / 2);
  *(ushort *)_refptr_g_lineBufSize =
       (ushort)_refptr_g_tsaPrmtFlash->bCols + (ushort)_refptr_g_tsaPrmtFlash->bRows;
  bVar1 = _refptr_g_tsaPrmtFlash->bCols;
  if (_refptr_g_tsaPrmtFlash->bCols <= _refptr_g_tsaPrmtFlash->bRows) {
    bVar1 = _refptr_g_tsaPrmtFlash->bRows;
  }
  *(ushort *)_refptr_g_maxDimSize = (ushort)bVar1;
  cVar2 = TSAMem_Malloc();
  if (cVar2 == '\0') {
    StrPrintf_Process(0,"\nAppLog: ");
    StrPrintf_Process(1,"\n TSA buffer malloc fail!");
  }
  return cVar2 != '\0';
}


// ============================================================================
// Function: TSAPrpt_Init @ 6baa07c8
// ============================================================================

void TSAPrpt_Init(void)

{
                    /* 0x607c8  1657  TSAPrpt_Init */
  TSA_InitBlockIdx();
  return;
}


// ============================================================================
// Function: TSAStatic_InitSysModuleEveryTime @ 6baa15c6
// ============================================================================

void TSAStatic_InitSysModuleEveryTime(void)

{
  g_tsaStaticPtr->llField_0xe8 = 0;
  g_tsaStaticPtr->nFrameCounter = 0;
  g_tsaStaticPtr->bField_0x110 = '\0';
  g_tsaStaticPtr->bField_0x111 = '\0';
  g_tsaStaticPtr->bField_0x116 = '\0';
  g_tsaStaticPtr->dwFeatureFlags = _refptr_g_tsaPrmtFlash->dwFeatureFlagsBase1;
  g_tsaStaticPtr->dwFeatureFlags =
       _refptr_g_tsaPrmtFlash->dwFeatureFlagsBase1_or | g_tsaStaticPtr->dwFeatureFlags;
  g_tsaStaticPtr->dwFeatureFlagsPrev = g_tsaStaticPtr->dwFeatureFlags;
  g_tsaStaticPtr->dwFeatureFlagsMasked = g_tsaStaticPtr->dwFeatureFlags & 0x4813;
  g_tsaStaticPtr->dwFeatureFlags2 = _refptr_g_tsaPrmtFlash->dwFeatureFlagsBase2;
  g_tsaStaticPtr->dwFeatureFlags2 =
       _refptr_g_tsaPrmtFlash->dwFeatureFlagsBase2_or | g_tsaStaticPtr->dwFeatureFlags2;
  g_tsaStaticPtr->wField_0x11e = 0;
  g_tsaStaticPtr->wField_0x120 = 0;
  g_tsaStaticPtr->wField_0x122 = 0;
  g_tsaStaticPtr->nField_0xc0 = 0;
  g_tsaStaticPtr->llField_0xd0 = 0;
  g_tsaStaticPtr->nField_0xdc = 0;
  g_tsaStaticPtr->nField_0x124 = 0;
  g_tsaStaticPtr->nField_0x128 = 0;
  g_tsaStaticPtr->nField_0x12c = 0;
  g_tsaStaticPtr->nField_0x130 = 0;
  g_tsaStaticPtr->nField_0x134 = 0;
  g_tsaStaticPtr->nField_0x138 = 0;
  g_tsaStaticPtr->bField_0x118 = '\0';
  g_tsaStaticPtr->bField_0x119 = '\0';
  g_tsaStaticPtr->bField_0x11a = '\0';
  g_tsaStaticPtr->bCmfProcessingBlocked = '\0';
  g_tsaStaticPtr->nField_0x280 = 0;
  g_tsaStaticPtr->nField_0x278 = 0;
  g_tsaStaticPtr->nField_0x284 = 4;
  g_tsaStaticPtr->bFullIncellSignalDisparity = '\0';
  g_tsaStaticPtr->nField_0x288 = 0;
  g_tsaStaticPtr->nField_0x28c = 0;
  g_tsaStaticPtr->nField_0x264 = 0;
  g_tsaStaticPtr->nField_0x270 = 0;
  g_tsaStaticPtr->bField_0x295 = '\0';
  g_tsaStaticPtr->bField_0x290 = '\0';
  g_tsaStaticPtr->nField_0x298 = 0;
  g_tsaStaticPtr->nField_0x29c = 0;
  g_tsaStaticPtr->nField_0x2a0 = 0;
  g_tsaStaticPtr->bFrameDoneFlag = '\0';
  return;
}


// ============================================================================
// Function: TSAStatic_InitInternalModuleEveryTime @ 6baa18b1
// ============================================================================

void TSAStatic_InitInternalModuleEveryTime(void)

{
  g_tsaStaticPtr->bField_0x00 = '\0';
  g_tsaStaticPtr->field_0xd = 0;
  g_tsaStaticPtr->field_0x2 = 0;
  g_tsaStaticPtr->field_0x3 = 0;
  g_tsaStaticPtr->field_0x5 = 0;
  g_tsaStaticPtr->field_0x7 = 0;
  g_tsaStaticPtr->field_0xa = 0;
  g_tsaStaticPtr->field_0xf = 0;
  *(undefined8 *)&g_tsaStaticPtr->field_0x18 = 0xffffffffffffffff;
  *(undefined8 *)&g_tsaStaticPtr->field_0x28 = 0xffffffffffffffff;
  *(undefined8 *)&g_tsaStaticPtr->field_0x20 = 0xffffffffffffffff;
  *(undefined8 *)&g_tsaStaticPtr->field_0x30 = 0xffffffffffffffff;
  *(undefined8 *)&g_tsaStaticPtr->field_0x38 = 0xffffffffffffffff;
  *(undefined8 *)&g_tsaStaticPtr->field_0x40 = 0xffffffffffffffff;
  *(undefined8 *)&g_tsaStaticPtr->field_0x60 = 0xffffffffffffffff;
  *(undefined8 *)&g_tsaStaticPtr->field_0x68 = 0xffffffffffffffff;
  *(undefined8 *)&g_tsaStaticPtr->field_0x70 = 0xffffffffffffffff;
  *(undefined8 *)&g_tsaStaticPtr->field_0x80 = 0xffffffffffffffff;
  *(undefined8 *)&g_tsaStaticPtr->field_0x78 = 0;
  g_tsaStaticPtr->llAutoCalibrationStartTime = 0;
  *(undefined8 *)&g_tsaStaticPtr->field_0x90 = 0;
  g_tsaStaticPtr->field_0xa0 = 0;
  g_tsaStaticPtr->field_0xa1 = 0;
  g_tsaStaticPtr->field_0xa8 = 0;
  *(undefined4 *)&g_tsaStaticPtr->field_0xac = 0;
  *(undefined4 *)&g_tsaStaticPtr->field_0xb0 = 0;
  g_tsaStaticPtr->field_0xa9 = 0;
  g_tsaStaticPtr->field_0xaa = 0;
  return;
}


// ============================================================================
// Function: TSAStatic_Init @ 6baa1a42
// ============================================================================

void TSAStatic_Init(void)

{
  TSAStatic *pTVar1;
  short sVar2;
  int iVar3;
  
                    /* 0x61a42  1681  TSAStatic_Init */
  TSAStatic_InitSysModuleEveryTime();
  TSAStatic_InitInternalModuleEveryTime();
  iVar3 = TSAStatic_SetLogName("TSA_DUMP");
  if (iVar3 != 1) {
    if (flagOnceOnly_10264 != '\0') {
      g_tsaStaticPtr->pSensorDataCallback = _refptr_TSA_GetSensorData;
      g_tsaStaticPtr->llField_0x148 = 0;
      g_tsaStaticPtr->dwAftRuntimeModeMask = 0;
      g_tsaStaticPtr->bField_0x11b = '\x01';
      g_tsaStaticPtr->wField_0x2e0 = 0;
      pTVar1 = g_tsaStaticPtr;
      sVar2 = TSA_GetXRes();
      pTVar1->nXResMax = sVar2 + -1;
      g_tsaStaticPtr->wField_0x2e4 = 0;
      pTVar1 = g_tsaStaticPtr;
      sVar2 = TSA_GetYRes();
      pTVar1->nYResMax = sVar2 + -1;
      g_tsaStaticPtr->nField_0x26c = 0;
      g_tsaStaticPtr->wField_0x292 = _refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold;
      g_tsaStaticPtr->llField_0x50 = -1;
      g_tsaStaticPtr->llField_0x58 = -1;
      g_tsaStaticPtr->llField_0x48 = -1;
      g_tsaStaticPtr->dwStylusAftGateFlags = 1;
      g_tsaStaticPtr->dwField_0x158 = 1;
      g_tsaStaticPtr->nField_0x268 = 0;
    }
    flagOnceOnly_10264 = '\0';
  }
  return;
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
// Function: AnimationInit @ 6baa3500
// ============================================================================

void AnimationInit(void)

{
                    /* 0x63500  333  AnimationInit */
  g_asaAnimalState = 0;
  g_asaAnimalPreState = 0;
  startX = 0xffffffff;
  startY = 0xffffffff;
  return;
}


// ============================================================================
// Function: ButtonInit @ 6baa3bb0
// ============================================================================

void ButtonInit(void)

{
                    /* 0x63bb0  447  ButtonInit */
  g_buttonReleaseCnt = 0;
  return;
}


// ============================================================================
// Function: LinearTestInit @ 6baa3c80
// ============================================================================

void LinearTestInit(void)

{
                    /* 0x63c80  899  LinearTestInit */
  g_frameBufCnt = 0;
  return;
}


// ============================================================================
// Function: OemInfoCfgInit @ 6baa727c
// ============================================================================

void OemInfoCfgInit(void)

{
  undefined1 local_d;
  uint local_c;
  
                    /* 0x6727c  950  OemInfoCfgInit */
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"\nOemInfoCfgInit! %d, %d",_refptr_g_asaPrmt->bTxCount,
                      _refptr_g_asaPrmt->bRxCount);
  }
  for (local_c = 0; local_c < 3; local_c = local_c + 1) {
    local_d = 0;
    *(undefined4 *)(&g_oemDataCfg + (ulonglong)local_c * 0xa8) = 0;
    if (local_c == 1) {
      OemSubDataRegister(1,0,_refptr_g_asaCalibrationTable + 0x9a19,1);
      OemSubDataRegister(1,1,_refptr_g_asaCalibrationTable + 0x9a1a,_refptr_g_asaPrmt->bTxCount);
      OemSubDataRegister(1,2,_refptr_g_asaCalibrationTable + 0x26ee,2);
      OemSubDataRegister(1,3,_refptr_g_asaCalibrationTable + 0x1326,
                         (uint)_refptr_g_asaPrmt->bRxCount * (uint)_refptr_g_asaPrmt->bTxCount);
      OemSubDataRegister(1,4,_refptr_g_asaCalibrationTable + 0x9a6c,2);
      local_d = 6;
      OemSubDataRegister(1,5,_refptr_g_asaCalibrationTable + 0x9a74,6);
    }
    else if (local_c == 0) {
      OemSubDataRegister(0,0,_refptr_g_asaCalibrationTable + 0x99c8,1);
      OemSubDataRegister(0,1,_refptr_g_asaCalibrationTable + 0x99c9,_refptr_g_asaPrmt->bRxCount);
      OemSubDataRegister(0,2,_refptr_g_asaCalibrationTable + 0x26ec,2);
      OemSubDataRegister(0,3,_refptr_g_asaCalibrationTable + 2,
                         (uint)_refptr_g_asaPrmt->bRxCount * (uint)_refptr_g_asaPrmt->bTxCount);
      OemSubDataRegister(0,4,_refptr_g_asaCalibrationTable + 0x9a6a,2);
      local_d = 6;
      OemSubDataRegister(0,5,_refptr_g_asaCalibrationTable + 0x9a6e,6);
    }
    else if (local_c == 2) {
      local_d = 1;
      OemSubDataRegister(2,0,_refptr_g_ltRet,0x54);
    }
    (&DAT_6bb86304)[(ulonglong)local_c * 0xa8] = local_d;
  }
  return;
}


// ============================================================================
// Function: PositionOffsetCalibrationInit @ 6baa7fb0
// ============================================================================

void PositionOffsetCalibrationInit(void)

{
                    /* 0x67fb0  1002  PositionOffsetCalibrationInit */
  g_dim1PstnOfst = 0;
  g_dim2PstnOfst = 0;
  g_positionOffsetCalLineNum = 0;
  return;
}


// ============================================================================
// Function: ASACalibration_Init @ 6baab7b0
// ============================================================================

void ASACalibration_Init(void)

{
                    /* 0x6b7b0  94  ASACalibration_Init */
  if (initOnlyOnce_8296 == '\0') {
    PositionOffsetCalibrationInit();
    g_mainFuncProc = 0;
    initOnlyOnce_8296 = '\x01';
  }
  OemInfoCfgInit();
  LinearTestInit();
  g_bufferCnt = 0;
  g_flagTx1DroppedSolutionEnable = 0;
  g_asaCalibStatus = 0;
  return;
}


// ============================================================================
// Function: CoordinateInit @ 6baac440
// ============================================================================

void CoordinateInit(void)

{
                    /* 0x6c440  522  CoordinateInit */
  memset_s(_refptr_g_asaPrpt->pReserved275_to79F + 0x4fb,0x30,0,0x30);
  g_coors = _refptr_g_asaPrpt->pReserved275_to79F + 0x4fb;
  return;
}


// ============================================================================
// Function: CompensationByPRInit @ 6baac486
// ============================================================================

void CompensationByPRInit(void)

{
  if ((*_refptr_g_asaPrmtFlash)[1].field_0x2 != '\0') {
    PolynomialRegression_SetComb
              (&g_signalCombPR,0x118,
               (char)*(undefined2 *)((*_refptr_g_asaPrmtFlash)[1].pStylus_unit_table + 0xe0),
               (char)*(undefined2 *)((*_refptr_g_asaPrmtFlash)[1].pStylus_unit_table + 0xe2));
    PolynomialRegression_SetComb
              (&g_posCombPR,0x118,
               (char)*(undefined2 *)((*_refptr_g_asaPrmtFlash)[1].pStylus_unit_table + 0x2b8),
               (char)*(undefined2 *)((*_refptr_g_asaPrmtFlash)[1].pStylus_unit_table + 0x2ba));
  }
  return;
}


// ============================================================================
// Function: CoorInit @ 6baac528
// ============================================================================

void CoorInit(void)

{
  ushort local_a;
  
                    /* 0x6c528  518  CoorInit */
  g_coorIIRCoef = 0x10;
  for (local_a = 0; local_a < 0x18; local_a = local_a + 1) {
    *(undefined4 *)_refptr_g_coorBuf = 0x7fffffff;
    *(undefined4 *)(_refptr_g_coorBuf + 4) = 0x7fffffff;
    *(undefined4 *)(_refptr_g_coorBuf + (longlong)(int)(uint)local_a * 4 + 8) = 0x7fffffff;
    *(undefined4 *)(_refptr_g_coorBuf + ((longlong)(int)(uint)local_a + 0x18) * 4 + 8) = 0x7fffffff;
    *(undefined4 *)(_refptr_g_coorBuf + ((longlong)(int)(uint)local_a + 0x30) * 4 + 8) = 0x7fffffff;
    *(undefined4 *)(_refptr_g_coorBuf + ((longlong)(int)(uint)local_a + 0x48) * 4 + 8) = 0x7fffffff;
    *(undefined4 *)(_refptr_g_coorBuf + ((longlong)(int)(uint)local_a + 0x60) * 4 + 8) = 0x7fffffff;
    *(undefined4 *)(_refptr_g_coorBuf + ((longlong)(int)(uint)local_a + 0x78) * 4 + 8) = 0x7fffffff;
    *(undefined4 *)(_refptr_g_coorBuf + ((longlong)(int)(uint)local_a + 0x90) * 4 + 8) = 0x7fffffff;
    *(undefined4 *)(_refptr_g_coorBuf + ((longlong)(int)(uint)local_a + 0xa8) * 4 + 8) = 0x7fffffff;
    *(undefined4 *)(_refptr_g_coorBuf + ((longlong)(int)(uint)local_a + 0xc0) * 4 + 8) = 0x7fffffff;
  }
  *(undefined4 *)(_refptr_g_coorBuf + 0x378) = 0;
  *(undefined4 *)(_refptr_g_coorBuf + 0x37c) = 0;
  *(undefined4 *)(_refptr_g_coorBuf + 0x380) = 0;
  CompensationByPRInit();
  return;
}


// ============================================================================
// Function: CoorReviseInit @ 6bab2370
// ============================================================================

void CoorReviseInit(void)

{
  byte local_9;
  
                    /* 0x72370  519  CoorReviseInit */
  for (local_9 = 0; local_9 < 10; local_9 = local_9 + 1) {
    *(undefined2 *)(&g_coorReviseXBuf + (longlong)(int)(uint)local_9 * 2) = 0;
    *(undefined2 *)(&g_coorReviseYBuf + (longlong)(int)(uint)local_9 * 2) = 0;
  }
  g_coorReviseBufCnt = 0;
  g_coorReviseX = 0;
  g_coorReviseY = 0;
  return;
}


// ============================================================================
// Function: FilterInit @ 6bab29a0
// ============================================================================

void FilterInit(void)

{
                    /* 0x729a0  597  FilterInit */
  return;
}


// ============================================================================
// Function: MultiPanleSwitchInit @ 6bab3440
// ============================================================================

void MultiPanleSwitchInit(void)

{
                    /* 0x73440  936  MultiPanleSwitchInit */
  g_tsaTpPenMatch = 0;
  g_tsaTpPenMatchCode = 0;
  g_mpsState = 0;
  g_mpsPreState = 0;
  g_mpsPreTriggerState = 0;
  g_mpsStateChangeTimeS = 0;
  g_mpsEventTriggerTimeS = 0;
  return;
}


// ============================================================================
// Function: PressTouchSignalInit @ 6bab37f0
// ============================================================================

void PressTouchSignalInit(void)

{
  _refptr_g_asaStatic->bExitNoPressInkDebounceCnt = 2;
  _refptr_g_asaStatic->bEnterNoPressInkDebounceCnt = 2;
  return;
}


// ============================================================================
// Function: NoPressInkInit @ 6bab380c
// ============================================================================

void NoPressInkInit(void)

{
  int iVar1;
  
                    /* 0x7380c  938  NoPressInkInit */
  PressTouchSignalInit();
  g_noPressInkTiltCompensation = 0;
  if (g_noPressInkInitOnce != 0) {
    g_noPressInkInitOnce = 0;
    iVar1 = memset_s(_refptr_g_noPressPara,0xc38c,0,0xc38c);
    if ((iVar1 != 0) && (*_refptr_g_tsaLogLoglevel != '\0')) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/ASA/Alg/no_press_ink/asa_no_press_ink.c",0x2d,
                        "rc == EOK");
    }
    _refptr_g_noPressPara->bTx2CompBinMin = 0xff;
    _refptr_g_noPressPara->wActiveTx2CompCoef =
         (ushort)(*_refptr_g_asaPrmtStylus)->bDefaultTx2CompCoef;
    _refptr_g_noPressPara->wTx2CompSignalMax = (*_refptr_g_asaPrmtStylus)->wTx2CompSignalMax;
  }
  _refptr_g_noPressPara->bSignalPosHistoryCount = 0;
  return;
}


// ============================================================================
// Function: HPP3_FreqShiftInit @ 6bab6b50
// ============================================================================

void HPP3_FreqShiftInit(void)

{
  byte local_9;
  
                    /* 0x76b50  823  HPP3_FreqShiftInit */
  g_asaHpp3FreqShiftEnable = 0;
  if (*(longlong *)&_refptr_g_asaStatic->field_0x40 == 0) {
    g_asaHpp3Freq = 0;
  }
  else {
    g_asaHpp3Freq = *(byte *)(*(longlong *)&_refptr_g_asaStatic->field_0x40 + 0x44);
    if (0x14 < g_asaHpp3Freq) {
      g_asaHpp3Freq = 0x14;
    }
    HPP3_EnableFreqShift();
    if (g_asaHpp3Freq != 0) {
      for (local_9 = 0; local_9 < g_asaHpp3Freq; local_9 = local_9 + 1) {
        (&DAT_6bb9e610)[(longlong)(int)(uint)local_9 * 6] =
             (char)*(undefined2 *)
                    (*(longlong *)(*(longlong *)&_refptr_g_asaStatic->field_0x40 + 0x48) +
                    (ulonglong)local_9 * 2);
        *(undefined2 *)(&DAT_6bb9e612 + (longlong)(int)(uint)local_9 * 6) = 0xffff;
        *(undefined2 *)(&DAT_6bb9e614 + (longlong)(int)(uint)local_9 * 6) = 0xffff;
      }
    }
  }
  return;
}


// ============================================================================
// Function: GridPeaksInit @ 6babb9d7
// ============================================================================

void GridPeaksInit(void)

{
                    /* 0x7b9d7  639  GridPeaksInit */
  memset_s(_refptr_g_asaData->pPeakFlagMap,0x2648,0,0x2648);
  memset_s(_refptr_g_asaData->pPeakFlagMapBackup,0x2648,0,0x2648);
  memset_s(&g_tx1PeakFlag,0x192,0,0x192);
  return;
}


// ============================================================================
// Function: PeaksInit @ 6babd750
// ============================================================================

void PeaksInit(void)

{
                    /* 0x7d750  998  PeaksInit */
  memset_s(&_refptr_g_asaPrpt->field_0xb0,0x6c0,0,0x6c0);
  *(ushort *)&_refptr_g_asaStatic->field_0x48 = _refptr_g_asaPrmt[1].wField00;
  *(ushort *)&_refptr_g_asaStatic->field_0x4c = _refptr_g_asaPrmt[1].wField00 * 2;
  return;
}


// ============================================================================
// Function: BtPressureInit @ 6babf350
// ============================================================================

void BtPressureInit(void)

{
                    /* 0x7f350  444  BtPressureInit */
  *_refptr_g_btPressBuf = 0;
  _refptr_g_btPressBuf[1] = 0;
  _refptr_g_btPressBuf[2] = 0;
  _refptr_g_btPressBuf[3] = 0;
  return;
}


// ============================================================================
// Function: PressureInit @ 6babf389
// ============================================================================

void PressureInit(void)

{
                    /* 0x7f389  1040  PressureInit */
  g_hpp3ExitFlag = false;
  if (*(int *)(_refptr_g_asaHpp3Freq + 4) == 0) {
    BtPressureInit();
  }
  return;
}


// ============================================================================
// Function: RawInit @ 6bac0260
// ============================================================================

void RawInit(void)

{
                    /* 0x80260  1149  RawInit */
  g_asaGroupInfo[0].pLineData = (ushort *)_refptr_g_asaData;
  g_asaGroupInfo[0].pRawBackup = (_refptr_g_asaData->lineProfiles).pLineSumPrimary;
  g_asaGroupInfo[0].pCmnBaseline = (_refptr_g_asaData->lineProfiles).pLineSumBackup + 0x14;
  g_asaGroupInfo[0].pCmnHistory = (int *)(_refptr_g_asaData->pPeakFlagMap + 0x14);
  g_asaGroupInfo[0].pAuxNoiseBuffer = _refptr_g_asaData->pPeakFlagMap + 0x68;
  g_asaGroupInfo[0].pPeakReport = &_refptr_g_asaPrpt->field_0xb0;
  g_asaGroupInfo[0].pGroupParam = &_refptr_g_asaPrePrpt->field_0xb0;
  g_asaGroupInfo[0].bGroupLength = _refptr_g_asaPrmt->bTxCount;
  g_asaGroupInfo[1].pLineData =
       (_refptr_g_asaData->lineProfiles).pRawLineSamples + (int)(uint)_refptr_g_asaPrmt->bTxCount;
  g_asaGroupInfo[1].pRawBackup =
       (_refptr_g_asaData->lineProfiles).pLineSumPrimary + (int)(uint)_refptr_g_asaPrmt->bTxCount;
  g_asaGroupInfo[1].pCmnBaseline =
       (_refptr_g_asaData->lineProfiles).pLineSumBackup +
       (longlong)(int)(uint)_refptr_g_asaPrmt->bTxCount + 0x14;
  g_asaGroupInfo[1].pCmnHistory = (int *)(_refptr_g_asaData->pPeakFlagMap + 0x3e);
  g_asaGroupInfo[1].pAuxNoiseBuffer =
       _refptr_g_asaData->pPeakFlagMap + (longlong)(int)(uint)_refptr_g_asaPrmt->bTxCount + 0x68;
  g_asaGroupInfo[1].pPeakReport = &_refptr_g_asaPrpt->field_0x260;
  g_asaGroupInfo[1].pGroupParam = &_refptr_g_asaPrePrpt->field_0x260;
  g_asaGroupInfo[1].bGroupLength = _refptr_g_asaPrmt->bRxCount;
  g_asaGroupInfo[2].pLineData = (_refptr_g_asaData->lineProfiles).pLineSumPrimary;
  g_asaGroupInfo[2].pRawBackup = (ushort *)(_refptr_g_asaData->pPad_9ce0 + 0x72d8);
  g_asaGroupInfo[2].pCmnBaseline = (ushort *)(_refptr_g_asaData->pPad_9ce0 + 0x7418);
  g_asaGroupInfo[2].pCmnHistory = (int *)(_refptr_g_asaData->pPad_9ce0 + 0x7728);
  g_asaGroupInfo[2].pAuxNoiseBuffer = (ushort *)(_refptr_g_asaData->pPad_9ce0 + 0x7558);
  g_asaGroupInfo[2].pPeakReport = &_refptr_g_asaPrpt->field_0xb0;
  g_asaGroupInfo[2].pGroupParam = &_refptr_g_asaPrePrpt->field_0xb0;
  g_asaGroupInfo[2].bGroupLength = _refptr_g_asaPrmt->bTxCount;
  g_asaGroupInfo[3].pLineData =
       (_refptr_g_asaData->lineProfiles).pLineSumPrimary + (int)(uint)_refptr_g_asaPrmt->bTxCount;
  g_asaGroupInfo[3].pRawBackup =
       (ushort *)
       (_refptr_g_asaData->pPad_9ce0 + (longlong)(int)(uint)_refptr_g_asaPrmt->bTxCount * 2 + 0x72d8
       );
  g_asaGroupInfo[3].pCmnBaseline =
       (ushort *)
       (_refptr_g_asaData->pPad_9ce0 + (longlong)(int)(uint)_refptr_g_asaPrmt->bTxCount * 2 + 0x7418
       );
  g_asaGroupInfo[3].pCmnHistory = (int *)(_refptr_g_asaData->pPad_9ce0 + 0x777c);
  g_asaGroupInfo[3].pAuxNoiseBuffer =
       (ushort *)
       (_refptr_g_asaData->pPad_9ce0 + (longlong)(int)(uint)_refptr_g_asaPrmt->bTxCount * 2 + 0x7558
       );
  g_asaGroupInfo[3].pPeakReport = &_refptr_g_asaPrpt->field_0x260;
  g_asaGroupInfo[3].pGroupParam = &_refptr_g_asaPrePrpt->field_0x260;
  g_asaGroupInfo[3].bGroupLength = _refptr_g_asaPrmt->bRxCount;
  g_asaGroupInfo[4].pLineData = (_refptr_g_asaData->lineProfiles).pLineSumBackup;
  g_asaGroupInfo[4].pRawBackup = (ushort *)(_refptr_g_asaData->pPad_9ce0 + 0x77d0);
  g_asaGroupInfo[4].pCmnBaseline = (ushort *)(_refptr_g_asaData->pPad_9ce0 + 0x7910);
  g_asaGroupInfo[4].pCmnHistory = (int *)(_refptr_g_asaData->pPad_9ce0 + 0x7c20);
  g_asaGroupInfo[4].pAuxNoiseBuffer = (ushort *)(_refptr_g_asaData->pPad_9ce0 + 0x7a50);
  g_asaGroupInfo[4].pPeakReport = _refptr_g_asaPrpt->pReserved275_to79F + 0x19b;
  g_asaGroupInfo[4].pGroupParam = &_refptr_g_asaPrePrpt[1].field_0x19b;
  g_asaGroupInfo[4].bGroupLength = _refptr_g_asaPrmt->bTxCount;
  g_asaGroupInfo[5].pLineData =
       (_refptr_g_asaData->lineProfiles).pLineSumBackup + (int)(uint)_refptr_g_asaPrmt->bTxCount;
  g_asaGroupInfo[5].pRawBackup =
       (ushort *)
       (_refptr_g_asaData->pPad_9ce0 + (longlong)(int)(uint)_refptr_g_asaPrmt->bTxCount * 2 + 0x77d0
       );
  g_asaGroupInfo[5].pCmnBaseline =
       (ushort *)
       (_refptr_g_asaData->pPad_9ce0 + (longlong)(int)(uint)_refptr_g_asaPrmt->bTxCount * 2 + 0x7910
       );
  g_asaGroupInfo[5].pCmnHistory = (int *)(_refptr_g_asaData->pPad_9ce0 + 0x7c74);
  g_asaGroupInfo[5].pAuxNoiseBuffer =
       (ushort *)
       (_refptr_g_asaData->pPad_9ce0 + (longlong)(int)(uint)_refptr_g_asaPrmt->bTxCount * 2 + 0x7a50
       );
  g_asaGroupInfo[5].pPeakReport = _refptr_g_asaPrpt->pReserved275_to79F + 0x34b;
  g_asaGroupInfo[5].pGroupParam = &_refptr_g_asaPrePrpt[2].field_0xd6;
  g_asaGroupInfo[5].bGroupLength = _refptr_g_asaPrmt->bRxCount;
  *_refptr_g_flagTX2Start = 0;
  return;
}


// ============================================================================
// Function: ReportInit @ 6bac311c
// ============================================================================

void ReportInit(void)

{
                    /* 0x8311c  1170  ReportInit */
  ASAOutClean();
  return;
}


// ============================================================================
// Function: TiltInit @ 6bac3700
// ============================================================================

void TiltInit(void)

{
  byte local_a;
  byte local_9;
  
                    /* 0x83700  2463  TiltInit */
  for (local_9 = 0; local_9 < 10; local_9 = local_9 + 1) {
    (&g_coordifdim1Buf)[(int)(uint)local_9] = 0;
    (&g_coordifdim2Buf)[(int)(uint)local_9] = 0;
    (&g_tiltdim1Buf)[(int)(uint)local_9] = 0;
    (&g_tiltdim2Buf)[(int)(uint)local_9] = 0;
  }
  g_coorDifBufCnt = 0;
  for (local_a = 0; local_a < 10; local_a = local_a + 1) {
    (&g_signalRatioBuf)[(int)(uint)local_a] = 0;
  }
  g_signalRatioBufCnt = 0;
  return;
}


// ============================================================================
// Function: ASAPropertyInit @ 6bac8350
// ============================================================================

void ASAPropertyInit(void)

{
                    /* 0x88350  106  ASAPropertyInit */
  memset_s(&g_asaPrpt,0x7ac,0,0x7ac);
  memset_s(&g_asaPrePrpt,0x7ac,0,0x7ac);
  return;
}


// ============================================================================
// Function: ASAStaticInit @ 6bac8481
// ============================================================================

void ASAStaticInit(longlong param_1)

{
  int iVar1;
  
                    /* 0x88481  111  ASAStaticInit */
  if ((param_1 != 0) && (iVar1 = memset_s(&g_asaStatic,0xf8,0,0xf8), iVar1 == 0)) {
    g_asaStatic.dwHpp3FeatureFlags =
         (*_refptr_g_asaPrmtFlash)->dwHpp3FeatureFlagsA |
         (*_refptr_g_asaPrmtFlash)->dwHpp3FeatureFlagsB;
    g_asaStatic._80_4_ = g_asaStatic.dwHpp3FeatureFlags & 0x4813;
    g_asaStatic._92_4_ =
         *(uint *)&(*_refptr_g_asaPrmtFlash)->field_0xa48 |
         *(uint *)&(*_refptr_g_asaPrmtFlash)->field_0xa4c;
    g_asaStatic._64_8_ = param_1;
    g_asaStatic._88_4_ = g_asaStatic.dwHpp3FeatureFlags;
  }
  return;
}


// ============================================================================
// Function: AFT_AntiAddrParaInit @ 6bac90e9
// ============================================================================

void AFT_AntiAddrParaInit(void)

{
  short sVar1;
  TSAFlashPrmt *pTVar2;
  uint uVar3;
  int iVar4;
  int local_18;
  uint local_14;
  uint local_10;
  short local_c;
  
                    /* 0x890e9  68  AFT_AntiAddrParaInit */
  pTVar2 = _refptr_g_tsaPrmtFlash;
  DAT_6bc7200a = 0;
  DAT_6bc7200c = *(short *)&_refptr_g_tsaPrmtFlash->field_0x5a6 + -1;
  DAT_6bc7201a = 0;
  DAT_6bc7201c = *(short *)&_refptr_g_tsaPrmtFlash->field_0x5a8 + -1;
  DAT_6bc7200e = *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x6ec;
  DAT_6bc72010 = DAT_6bc7200c - *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x6ee;
  DAT_6bc7201e = *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x6f0;
  DAT_6bc72020 = DAT_6bc7201c - *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x6f2;
  if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e8 & 2) != 0) {
    DAT_6bc72006 = *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x6f4;
    DAT_6bc72008 = *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x6f6;
    if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e8 & 4) == 0) {
      local_c = 1;
    }
    else {
      local_c = 3;
    }
    if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e8 & 8) != 0) {
      DAT_6bc72004 = local_c + 1;
    }
    if (((*(int *)_refptr_g_foldedDisplay == 1) && (DAT_6bc72004 == 1)) ||
       ((*(int *)_refptr_g_foldedDisplay == 2 && (DAT_6bc72004 == 2)))) {
      DAT_6bc72010 = DAT_6bc72006;
    }
    else if (((*(int *)_refptr_g_foldedDisplay == 1) && (DAT_6bc72004 == 2)) ||
            ((*(int *)_refptr_g_foldedDisplay == 2 && (DAT_6bc72004 == 1)))) {
      DAT_6bc7200e = DAT_6bc72008;
    }
    else if (((*(int *)_refptr_g_foldedDisplay == 1) && (DAT_6bc72004 == 3)) ||
            ((*(int *)_refptr_g_foldedDisplay == 2 && (DAT_6bc72004 == 4)))) {
      DAT_6bc72020 = DAT_6bc72006;
    }
    else if (((*(int *)_refptr_g_foldedDisplay == 1) && (DAT_6bc72004 == 4)) ||
            ((*(int *)_refptr_g_foldedDisplay == 2 && (DAT_6bc72004 == 3)))) {
      DAT_6bc7201e = DAT_6bc72008;
    }
  }
  DAT_6bc72018 = AFT_GetDistanceInPixel
                           (*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a6,100,
                            (int)*(short *)(_refptr_g_generalInfo + 0x14));
  DAT_6bc72012 = (undefined2)(((int)DAT_6bc72010 + (int)DAT_6bc7200e) / 2);
  DAT_6bc72014 = DAT_6bc72018 + DAT_6bc7200e;
  DAT_6bc72016 = DAT_6bc72010 - DAT_6bc72018;
  DAT_6bc72028 = AFT_GetDistanceInPixel
                           (*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a8,100,
                            (int)*(short *)(_refptr_g_generalInfo + 0x16));
  DAT_6bc72022 = (undefined2)(((int)DAT_6bc72020 + (int)DAT_6bc7201e) / 2);
  DAT_6bc72024 = DAT_6bc72028 + DAT_6bc7201e;
  DAT_6bc72026 = DAT_6bc72020 - DAT_6bc72028;
  uVar3 = AFT_GetDistanceInPixel
                    (*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a6,3,
                     (int)*(short *)(_refptr_g_generalInfo + 0x14));
  local_10 = (uint)*(ushort *)&pTVar2[2].field_0x720;
  if ((int)(uint)*(ushort *)&pTVar2[2].field_0x720 < (int)uVar3) {
    local_10 = uVar3;
  }
  DAT_6bc7202a = (short)local_10 + DAT_6bc7200e;
  DAT_6bc7202c = DAT_6bc72010 - (short)local_10;
  iVar4 = AFT_GetDistanceInPixel
                    (*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a6,8,
                     (int)*(short *)(_refptr_g_generalInfo + 0x14));
  local_14 = (uint)*(ushort *)&pTVar2[2].field_0x724;
  if ((int)local_14 < (int)(iVar4 + local_10)) {
    local_14 = iVar4 + local_10;
  }
  DAT_6bc72032 = (short)local_14 + DAT_6bc7200e;
  DAT_6bc72034 = DAT_6bc72010 - (short)local_14;
  iVar4 = AFT_GetDistanceInPixel
                    (*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a6,0x10,
                     (int)*(short *)(_refptr_g_generalInfo + 0x14));
  local_18 = AFT_GetDistanceInPixel
                       (*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a6,0x1e,
                        (int)*(short *)(_refptr_g_generalInfo + 0x14));
  if (local_18 < (int)(iVar4 + local_14)) {
    local_18 = iVar4 + local_14;
  }
  sVar1 = (short)local_18;
  DAT_6bc7203a = sVar1 + DAT_6bc7200e;
  DAT_6bc7203c = DAT_6bc72010 - sVar1;
  sVar1 = sVar1 + (short)(local_18 / 2);
  DAT_6bc72042 = sVar1 + DAT_6bc7200e;
  DAT_6bc72044 = DAT_6bc72010 - sVar1;
  uVar3 = AFT_GetDistanceInPixel
                    (*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a8,3,
                     (int)*(short *)(_refptr_g_generalInfo + 0x16));
  local_10 = (uint)*(ushort *)&pTVar2[2].field_0x722;
  if ((int)(uint)*(ushort *)&pTVar2[2].field_0x722 < (int)uVar3) {
    local_10 = uVar3;
  }
  DAT_6bc7202e = (short)local_10 + DAT_6bc7201e;
  DAT_6bc72030 = DAT_6bc72020 - (short)local_10;
  iVar4 = AFT_GetDistanceInPixel
                    (*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a8,8,
                     (int)*(short *)(_refptr_g_generalInfo + 0x16));
  local_14 = (uint)*(ushort *)&pTVar2[2].field_0x726;
  if ((int)local_14 < (int)(iVar4 + local_10)) {
    local_14 = iVar4 + local_10;
  }
  DAT_6bc72036 = (short)local_14 + DAT_6bc7201e;
  DAT_6bc72038 = DAT_6bc72020 - (short)local_14;
  iVar4 = AFT_GetDistanceInPixel
                    (*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a8,0x10,
                     (int)*(short *)(_refptr_g_generalInfo + 0x16));
  local_18 = AFT_GetDistanceInPixel
                       (*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a8,0x1e,
                        (int)*(short *)(_refptr_g_generalInfo + 0x16));
  if (local_18 < (int)(iVar4 + local_14)) {
    local_18 = iVar4 + local_14;
  }
  sVar1 = (short)local_18;
  DAT_6bc7203e = sVar1 + DAT_6bc7201e;
  DAT_6bc72040 = DAT_6bc72020 - sVar1;
  sVar1 = sVar1 + (short)(local_18 / 2);
  DAT_6bc72046 = sVar1 + DAT_6bc7201e;
  DAT_6bc72048 = DAT_6bc72020 - sVar1;
  AFT_ParameterAdjust(1);
  return;
}


// ============================================================================
// Function: AFT_AntiEdgeFalseTouchInit @ 6bac99a0
// ============================================================================

void AFT_AntiEdgeFalseTouchInit(void)

{
  TSAFlashPrmt *pTVar1;
  short sVar2;
  int iVar3;
  
                    /* 0x899a0  70  AFT_AntiEdgeFalseTouchInit */
  pTVar1 = _refptr_g_tsaPrmtFlash;
  iVar3 = memset_s(&g_aftPara,0x8e,0,0x8e);
  if (iVar3 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/AntiFalseTouch/AntiEdgeFalseTouch.c",0xb7,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar3 = memset_s(&g_aftPara2,0x10,0,0x10);
  if (iVar3 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/AntiFalseTouch/AntiEdgeFalseTouch.c",0xb9,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar3 = memset_s(&g_aftState,0xe,0,0xe);
  if (iVar3 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/AntiFalseTouch/AntiEdgeFalseTouch.c",0xbb,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar3 = memset_s(&g_aftFgInfo,0x370,0,0x370);
  if (iVar3 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/AntiFalseTouch/AntiEdgeFalseTouch.c",0xbe,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  *_refptr_g_algoSwitch = 1;
  g_aftPara = *(ushort *)&pTVar1[2].field_0x70c & 3;
  DAT_6bc72002 = *(ushort *)&pTVar1[2].field_0x70e & 0xf;
  AFT_AntiAddrParaInit();
  sVar2 = AFT_GetDistanceInPixel
                    (*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a8,1,
                     (int)*(short *)(_refptr_g_generalInfo + 0x16));
  DAT_6bc7204a = sVar2 * sVar2;
  sVar2 = AFT_GetDistanceInPixel
                    (*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a8,0x14,
                     (int)*(short *)(_refptr_g_generalInfo + 0x16));
  DAT_6bc7204c = sVar2 * sVar2;
  DAT_6bc7204e = AFT_GetDistanceInPixel
                           (*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a8,0x5a,
                            (int)*(short *)(_refptr_g_generalInfo + 0x16));
  DAT_6bc72050 = AFT_GetDistanceInPixel
                           (*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a8,0x3c,
                            (int)*(short *)(_refptr_g_generalInfo + 0x16));
  DAT_6bc72052 = AFT_GetDistanceInPixel
                           (*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a8,0x28,
                            (int)*(short *)(_refptr_g_generalInfo + 0x16));
  DAT_6bc72054 = AFT_GetDistanceInPixel
                           (*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a6,
                            (char)*(undefined2 *)&pTVar1[2].field_0x728,
                            (int)*(short *)(_refptr_g_generalInfo + 0x14));
  if (DAT_6bc72054 == 0) {
    DAT_6bc72054 = AFT_GetDistanceInPixel
                             (*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a6,0x3c,
                              (int)*(short *)(_refptr_g_generalInfo + 0x14));
  }
  DAT_6bc72056 = AFT_GetDistanceInPixel
                           (*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a8,
                            (char)*(undefined2 *)&pTVar1[2].field_0x72a,
                            (int)*(short *)(_refptr_g_generalInfo + 0x16));
  if (DAT_6bc72056 == 0) {
    DAT_6bc72056 = AFT_GetDistanceInPixel
                             (*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a8,0x5a,
                              (int)*(short *)(_refptr_g_generalInfo + 0x16));
  }
  DAT_6bc72058 = AFT_GetDistanceInPixel
                           (*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a8,
                            (char)*(undefined2 *)&pTVar1[2].field_0x72c,
                            (int)*(short *)(_refptr_g_generalInfo + 0x16));
  if (DAT_6bc72054 < DAT_6bc72058) {
    DAT_6bc72058 = DAT_6bc72054;
  }
  DAT_6bc7205a = AFT_GetDistanceInPixel
                           (*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a8,
                            (char)*(undefined2 *)&pTVar1[2].field_0x72e,
                            (int)*(short *)(_refptr_g_generalInfo + 0x16));
  if (DAT_6bc72056 < DAT_6bc7205a) {
    DAT_6bc7205a = DAT_6bc72056;
  }
  DAT_6bc7205c = *(ushort *)&pTVar1[2].field_0x730 & 0xf;
  DAT_6bc7205e = AFT_GetDistanceInPixel
                           (*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a6,0x10,
                            (int)*(short *)(_refptr_g_generalInfo + 0x14));
  DAT_6bc72060 = *(short *)&pTVar1[2].field_0x732;
  if (DAT_6bc72060 == 0) {
    DAT_6bc72060 = 0x1e;
  }
  DAT_6bc72062 = (undefined2)((DAT_6bc72060 * 0x28) / 0x1e);
  DAT_6bc7206c = *(short *)&pTVar1[2].field_0x736;
  DAT_6bc72064 = *(short *)&pTVar1[2].field_0x734;
  if (*(short *)&pTVar1[2].field_0x734 == 0) {
    DAT_6bc72064 = DAT_6bc7206c;
  }
  if (DAT_6bc72064 == 0) {
    sVar2 = (short)((int)(((uint)(byte)_refptr_g_tsaPrmtFlash->field_0x1b * 4 +
                          (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x1b) * 2) >> 4);
    DAT_6bc72064 = (short)(0x640 / (ulonglong)(longlong)(int)sVar2) + 1;
    DAT_6bc72066 = (short)(2000 / (ulonglong)(longlong)(int)sVar2) + 1;
    DAT_6bc72068 = (short)(0x960 / (ulonglong)(longlong)(int)sVar2) + 1;
    DAT_6bc7206a = (short)(0xaf0 / (ulonglong)(longlong)(int)sVar2) + 1;
  }
  else {
    iVar3 = DAT_6bc72064 * 5;
    if (iVar3 < 0) {
      iVar3 = iVar3 + 3;
    }
    DAT_6bc72066 = (short)(iVar3 >> 2);
    iVar3 = DAT_6bc72064 * 6;
    if (iVar3 < 0) {
      iVar3 = iVar3 + 3;
    }
    DAT_6bc72068 = (short)(iVar3 >> 2);
    iVar3 = DAT_6bc72064 * 7;
    if (iVar3 < 0) {
      iVar3 = iVar3 + 3;
    }
    DAT_6bc7206a = (short)(iVar3 >> 2);
  }
  if (DAT_6bc7206c == 0) {
    sVar2 = (short)((int)(((uint)(byte)_refptr_g_tsaPrmtFlash->field_0x1c * 4 +
                          (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x1c) * 2) >> 4);
    DAT_6bc7206c = (short)(0x640 / (ulonglong)(longlong)(int)sVar2) + 1;
    DAT_6bc7206e = (short)(2000 / (ulonglong)(longlong)(int)sVar2) + 1;
    DAT_6bc72070 = (short)(0x960 / (ulonglong)(longlong)(int)sVar2) + 1;
    DAT_6bc72072 = (short)(0xaf0 / (ulonglong)(longlong)(int)sVar2) + 1;
    DAT_6bc72074 = (short)(0xe10 / (ulonglong)(longlong)(int)sVar2) + 1;
    DAT_6bc72076 = (short)(0x12c0 / (ulonglong)(longlong)(int)sVar2) + 1;
    DAT_6bc72078 = (short)(0xaf0 / (ulonglong)(longlong)(int)sVar2) + 1;
  }
  else {
    iVar3 = DAT_6bc7206c * 5;
    if (iVar3 < 0) {
      iVar3 = iVar3 + 3;
    }
    DAT_6bc7206e = (short)(iVar3 >> 2);
    iVar3 = DAT_6bc7206c * 6;
    if (iVar3 < 0) {
      iVar3 = iVar3 + 3;
    }
    DAT_6bc72070 = (short)(iVar3 >> 2);
    iVar3 = DAT_6bc7206c * 7;
    if (iVar3 < 0) {
      iVar3 = iVar3 + 3;
    }
    DAT_6bc72072 = (short)(iVar3 >> 2);
    iVar3 = DAT_6bc7206c * 9;
    if (iVar3 < 0) {
      iVar3 = iVar3 + 3;
    }
    DAT_6bc72074 = (short)(iVar3 >> 2);
    iVar3 = DAT_6bc7206c * 0xc;
    if (DAT_6bc7206c * 0xc < 0) {
      iVar3 = iVar3 + 3;
    }
    DAT_6bc72076 = (short)(iVar3 >> 2);
    iVar3 = DAT_6bc7206c * 7;
    if (iVar3 < 0) {
      iVar3 = iVar3 + 3;
    }
    DAT_6bc72078 = (short)(iVar3 >> 2);
  }
  DAT_6bc7207a = *(short *)&pTVar1[2].field_0x738;
  if (DAT_6bc7207a == 0) {
    DAT_6bc7207a = 0x32;
  }
  DAT_6bc7207c = *(short *)&pTVar1[2].field_0x73a;
  if (DAT_6bc7207c == 0) {
    DAT_6bc7207c = 0x1e;
  }
  DAT_6bc7207e = *(short *)&pTVar1[2].field_0x73c;
  if (DAT_6bc7207e == 0) {
    DAT_6bc7207e = 30000;
  }
  DAT_6bc72080 = *(short *)&pTVar1[2].field_0x73e;
  if (DAT_6bc72080 == 0) {
    DAT_6bc72080 = 30000;
  }
  DAT_6bc72082 = *(short *)&pTVar1[2].field_0x740;
  if (DAT_6bc72082 == 0) {
    DAT_6bc72082 = 30000;
  }
  DAT_6bc72084 = *(short *)&pTVar1[2].field_0x742;
  if (DAT_6bc72084 == 0) {
    DAT_6bc72084 = 30000;
  }
  DAT_6bc72086 = *(short *)&pTVar1[2].field_0x744;
  if (DAT_6bc72086 == 0) {
    DAT_6bc72086 = DAT_6bc7207e / 2;
  }
  DAT_6bc72088 = *(short *)&pTVar1[2].field_0x746;
  if (DAT_6bc72088 == 0) {
    DAT_6bc72088 = DAT_6bc72080 / 2;
  }
  DAT_6bc7208a = *(short *)&pTVar1[2].field_0x748;
  if (DAT_6bc7208a < 0) {
    DAT_6bc7208a = 0;
  }
  DAT_6bc7208c = *(short *)&pTVar1[2].field_0x74a;
  if (DAT_6bc7208c < 0) {
    DAT_6bc7208c = 0;
  }
  AFT_ReinitPalmEdge();
  return;
}


// ============================================================================
// Function: AFT_FilterAlgoInit @ 6bad4bf9
// ============================================================================

void AFT_FilterAlgoInit(void)

{
  int iVar1;
  undefined8 in_stack_ffffffffffffffc8;
  undefined4 uVar2;
  
                    /* 0x94bf9  73  AFT_FilterAlgoInit */
  uVar2 = (undefined4)((ulonglong)in_stack_ffffffffffffffc8 >> 0x20);
  iVar1 = memset_s(&g_algoSwitch,0x20,0,0x20);
  if (iVar1 != 0) {
    uVar2 = 0;
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/AntiFalseTouch/AntiFalseTouch_Process.c",0x8a,"rc == EOK"
                     );
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(&g_suppressedInfo,0x3c0,0,0x3c0);
  if (iVar1 != 0) {
    uVar2 = 0;
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/AntiFalseTouch/AntiFalseTouch_Process.c",0x8c,"rc == EOK"
                     );
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(&g_addedInfo,0x3c0,0,0x3c0);
  if (iVar1 != 0) {
    uVar2 = 0;
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/AntiFalseTouch/AntiFalseTouch_Process.c",0x8e,"rc == EOK"
                     );
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(&g_generalInfo,0x18,0,0x18);
  if (iVar1 != 0) {
    uVar2 = 0;
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/AntiFalseTouch/AntiFalseTouch_Process.c",0x91,"rc == EOK"
                     );
    HardwareAnalyzer_SetSoftException();
  }
  DAT_6bc724a4 = (undefined2)
                 ((int)((uint)_refptr_g_tsaPrmtFlash->bCols *
                        (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x1b * 10) >> 4);
  DAT_6bc724a6 = (undefined2)
                 ((int)((uint)_refptr_g_tsaPrmtFlash->bRows *
                        (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x1c * 10) >> 4);
  g_lastScreenRotatedState = 0;
  AFT_AntiEdgeFalseTouchInit();
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"\n %s end, algoIdA=%d,stopWidth=%d, stopHeight=%d,markFlag1 =%d\n",
                      "AFT_FilterAlgoInit",*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x6e2,
                      CONCAT44(uVar2,(uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x720),
                      *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x722,
                      *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6);
  }
  return;
}


// ============================================================================
// Function: AFT_Init @ 6bad5fa8
// ============================================================================

void AFT_Init(void)

{
                    /* 0x95fa8  78  AFT_Init */
  AFT_FilterAlgoInit();
  return;
}


// ============================================================================
// Function: AntiTouch_Init @ 6bad6010
// ============================================================================

void AntiTouch_Init(void)

{
                    /* 0x96010  336  AntiTouch_Init */
  g_antiTouchAbnormalTimeStamp = 0xffffffffffffffff;
  return;
}


// ============================================================================
// Function: BigData_Init @ 6bad7e5c
// ============================================================================

void BigData_Init(void)

{
  int iVar1;
  byte local_9;
  
                    /* 0x97e5c  421  BigData_Init */
  if (flagOnceOnly_10335 != '\0') {
    iVar1 = memset_s(&g_bigData,0x3958,0,0x3958);
    if (iVar1 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/BigData/BigData.c",0x3b,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
    iVar1 = memset_s(&g_bigDataStat,0x28,0,0x28);
    if (iVar1 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/BigData/BigData.c",0x3d,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
  }
  flagOnceOnly_10335 = 0;
  for (local_9 = 0; local_9 < 5; local_9 = local_9 + 1) {
    BigData_ClearTagData(local_9);
  }
  return;
}


// ============================================================================
// Function: TSA_DaInitTsaData @ 6bad9d33
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void TSA_DaInitTsaData(longlong param_1)

{
                    /* 0x99d33  1792  TSA_DaInitTsaData */
  *(undefined **)(param_1 + 0x50) = &g_lineData;
  *(undefined8 *)(param_1 + 0x48) = g_gridData;
  *(undefined **)(param_1 + 0x58) = &g_sensorData;
  *(undefined **)(param_1 + 0x88) = &g_cmdList;
  *(undefined **)(param_1 + 0x80) = &g_tsaStylus;
  *(undefined **)(param_1 + 0x60) = &g_accData;
  *(undefined **)(param_1 + 0x70) = &g_postureData;
  _g_tsaStylus = g_tx1LineData;
  _DAT_6bb9ed88 = g_tx2LineData;
  _DAT_6bb9eda0 = &g_stylusNoise;
  return;
}


// ============================================================================
// Function: TSA_DaInitBuffer @ 6bad9f1d
// ============================================================================

void TSA_DaInitBuffer(void)

{
  int iVar1;
  
  iVar1 = memset_s(g_gridData,(ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,0,
                   (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/Data/TSA_DataAccess.c",0xe2,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(&g_lineData,0x140,0,0x140);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/Data/TSA_DataAccess.c",0xe4,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(g_tx1LineData,(ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,0,
                   (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/Data/TSA_DataAccess.c",0xe6,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(g_tx2LineData,(ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,0,
                   (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/Data/TSA_DataAccess.c",0xe8,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  *g_cmdListBuffer = 0;
  return;
}


// ============================================================================
// Function: TSA_DaInitFlag @ 6bada0e0
// ============================================================================

void TSA_DaInitFlag(void)

{
  g_tsaDataFlag = 0;
  DAT_6bba0d91 = 0;
  DAT_6bba0d92 = 0;
  DAT_6bba0d93 = 0;
  DAT_6bba0d94 = 0;
  DAT_6bba0d95 = 0;
  DAT_6bba0d96 = 0;
  DAT_6bba0d97 = 0;
  DAT_6bba0d98 = 0;
  DAT_6bba0d99 = 0;
  DAT_6bba0d9a = 0;
  DAT_6bba0d9e = 0;
  DAT_6bba0d9b = 0;
  DAT_6bba0d9c = 0;
  DAT_6bba0d9d = 0;
  DAT_6bba0d9f = 0;
  DAT_6bba0da0 = 0;
  DAT_6bba0da1 = 0;
  DAT_6bba0da2 = 0;
  DAT_6bba0da3 = 0;
  DAT_6bba0da4 = 0;
  DAT_6bba0da5 = 0;
  DAT_6bba0da6 = 0;
  DAT_6bba0da7 = 0;
  DAT_6bba0da8 = 0;
  DAT_6bba0da9 = 0;
  return;
}


// ============================================================================
// Function: ComPortFormat_InitAfeFrameData @ 6badbc20
// ============================================================================

void ComPortFormat_InitAfeFrameData(void)

{
                    /* 0x9bc20  502  ComPortFormat_InitAfeFrameData */
  DAT_6bc76f50 = &g_gridData;
  DAT_6bc76f58 = 0;
  DAT_6bc76f60 = 0;
  DAT_6bc76f68 = 0;
  DAT_6bc76f88 = 0;
  DAT_6bc76f90 = 0;
  DAT_6bc76f80 = 0;
  return;
}


// ============================================================================
// Function: DoubleReport_Init @ 6bade43c
// ============================================================================

void DoubleReport_Init(void)

{
  int iVar1;
  
                    /* 0x9e43c  533  DoubleReport_Init */
  iVar1 = memset_s(&g_doubleReportTouchUnits,0x1c20,0,0x1c20);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/DoubleReport/DoubleReport.c",0x26,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  g_doubleReportInit = 1;
  return;
}


// ============================================================================
// Function: EventAnalyzer_Init @ 6badef1e
// ============================================================================

void EventAnalyzer_Init(void)

{
  int iVar1;
  
                    /* 0x9ef1e  569  EventAnalyzer_Init */
  iVar1 = memset_s(&g_internalTouches,0x1d60,0,0x1d60);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/EventAnalyzer/EventAnalyzer_Process.c",0x3f,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: Gesture_InitList @ 6bae0026
// ============================================================================

void Gesture_InitList(void)

{
  byte local_9;
  
  local_9 = 0;
  while (local_9 == 0) {
    g_gestureList = g_gestureTemplates;
    local_9 = 1;
  }
  (&g_gestureList)[(int)(uint)local_9] = 0;
  return;
}


// ============================================================================
// Function: Gesture_Init @ 6bae0083
// ============================================================================

void Gesture_Init(void)

{
                    /* 0xa0083  608  Gesture_Init */
  Gesture_Reset();
  Gesture_InitList();
  return;
}


// ============================================================================
// Function: GripFilter_Init @ 6bae174a
// ============================================================================

void GripFilter_Init(void)

{
                    /* 0xa174a  695  GripFilter_Init */
  GripFilter_RegionInit();
  GripFilter_DeadZoneInit();
  return;
}


// ============================================================================
// Function: GripFilter_DeadZoneInit @ 6bae596d
// ============================================================================

void GripFilter_DeadZoneInit(void)

{
                    /* 0xa596d  664  GripFilter_DeadZoneInit */
  GripFilter_DeadzoneReset();
  return;
}


// ============================================================================
// Function: GripFilter_RegionInit @ 6bae9f0d
// ============================================================================

void GripFilter_RegionInit(void)

{
                    /* 0xa9f0d  782  GripFilter_RegionInit */
  GripFilter_RegionReset();
  return;
}


// ============================================================================
// Function: HandGesture_Init @ 6baedf1b
// ============================================================================

void HandGesture_Init(void)

{
  int iVar1;
  
                    /* 0xadf1b  850  HandGesture_Init */
  iVar1 = memset_s(&handGesture,0x28,0,0x28);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/HandGesture/HandGesture.c",0x62,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(&handGesturePrev,0x28,0,0x28);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/HandGesture/HandGesture.c",100,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(g_tsaBufAreaHand,*(undefined2 *)_refptr_g_gridBufSize,0,
                   *(undefined2 *)_refptr_g_gridBufSize);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/HandGesture/HandGesture.c",0x67,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(g_tsaBufTzHand,*(undefined2 *)_refptr_g_gridBufSize,0,
                   *(undefined2 *)_refptr_g_gridBufSize);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/HandGesture/HandGesture.c",0x69,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  HandGesture_ResetRecord();
  HandGesture_BaselineReset();
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_Init @ 6baefb27
// ============================================================================

void HardwareAnalyzer_Init(void)

{
  int iVar1;
  
                    /* 0xafb27  864  HardwareAnalyzer_Init */
  g_hwAnalyzer = 0;
  DAT_6bc8c9c4 = 0;
  DAT_6bc8cbc0 = 0;
  DAT_6bc8cbc4 = 0;
  DAT_6bc8cbc8 = 0;
  DAT_6bc8cbc9 = 0;
  HardwareAnalyzer_ClearExceptionDescription();
  iVar1 = memset_s(&_refptr_g_tsaPrpt->field_0x38,0x58,0,0x58);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/HardwareAnalyzer/HardwareAnalyzer_Process.c",0x51,
                      "rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(*(undefined8 *)_refptr_g_tsaBufRaw2rawDif,
                   (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,0,
                   (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/HardwareAnalyzer/HardwareAnalyzer_Process.c",0x53,
                      "rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(_refptr_g_tsaPrpt,0x1c,0,0x1c);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/HardwareAnalyzer/HardwareAnalyzer_Process.c",0x55,
                      "rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(*(undefined8 *)_refptr_g_tsaBufRaw2rawDifAftCMF,
                   (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,0,
                   (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/HardwareAnalyzer/HardwareAnalyzer_Process.c",0x57,
                      "rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags2 & 0x10) != 0) {
    RFNoise_Init();
  }
  HardwareAnalyzer_SensorBrokenInit();
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags2 & 8) != 0) {
    BadChannel_Init();
  }
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags2 & 0x80) != 0) {
    BaliSensorBroken_Init();
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_SensorBrokenInit @ 6baf3755
// ============================================================================

void HardwareAnalyzer_SensorBrokenInit(void)

{
  int local_c;
  
                    /* 0xb3755  875  HardwareAnalyzer_SensorBrokenInit */
  HardwareAnalyzer_SensorBrokenReset();
  if (g_brokenSensorInitOnce == '\0') {
    g_brokenSensorInitOnce = '\x01';
    for (local_c = 0; local_c < (int)(uint)_refptr_g_tsaPrmtFlash->wBufElementCount;
        local_c = local_c + 1) {
      *(undefined1 *)(local_c + g_brokenSensorByDifDebounce) = 0;
      *(undefined1 *)(local_c + g_brokenSensorByDifState) = 0;
      *(byte *)(*(longlong *)_refptr_g_tpSensorFlag + (longlong)local_c) =
           *(byte *)((longlong)local_c + *(longlong *)_refptr_g_tpSensorFlag) & 0xf0;
    }
  }
  _refptr_g_tsaPrpt->field_0x3a = 0;
  return;
}


// ============================================================================
// Function: HotZoneAdjust_Init @ 6baf3fdd
// ============================================================================

void HotZoneAdjust_Init(void)

{
                    /* 0xb3fdd  882  HotZoneAdjust_Init */
  HotZoneAdjustClearAnalyzer();
  g_hotZoneAnalyzer = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  return;
}


// ============================================================================
// Function: Posture_InitQuene @ 6baf787c
// ============================================================================

void Posture_InitQuene(longlong param_1)

{
  *(undefined2 *)(param_1 + 8) = 0;
  *(undefined2 *)(param_1 + 10) = 0;
  return;
}


// ============================================================================
// Function: Proximity_Init @ 6baf84e0
// ============================================================================

void Proximity_Init(void)

{
  int iVar1;
  
                    /* 0xb84e0  1098  Proximity_Init */
  if (g_proxData != 0) {
    iVar1 = memset_s(g_proxData,0x68,0,0x68);
    if (iVar1 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/Proximity/Proximity.c",0x37,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
  }
  SensorGesture_ClearExternally(6);
  return;
}


// ============================================================================
// Function: RecordTrigger_Init @ 6bafce68
// ============================================================================

void RecordTrigger_Init(void)

{
                    /* 0xbce68  1162  RecordTrigger_Init */
  RecordTrigger_ClearFlag();
  g_touchTotal = 0;
  return;
}


// ============================================================================
// Function: SHBVolumeData_Init @ 6bafe96d
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void SHBVolumeData_Init(void)

{
                    /* 0xbe96d  1181  SHBVolumeData_Init */
  SHBVolumeData_Reset();
  _g_dataTPToPressurePrev = g_dataTPToPressure;
  _DAT_6bbaa028 = DAT_6bbaa008;
  _DAT_6bbaa030 = _DAT_6bbaa010;
  return;
}


// ============================================================================
// Function: SS_Init @ 6baff068
// ============================================================================

void SS_Init(void)

{
  char cVar1;
  
                    /* 0xbf068  1194  SS_Init */
  g_ssRawValid = 0;
  cVar1 = SS_IsDim1Valid();
  if (cVar1 == '\0') {
    g_ssIdxStart = _refptr_g_tsaPrmtFlash->bCols;
  }
  else {
    g_ssIdxStart = '\0';
  }
  cVar1 = SS_IsDim2Valid();
  if (cVar1 == '\0') {
    g_ssIdxEnd = _refptr_g_tsaPrmtFlash->bCols;
  }
  else {
    g_ssIdxEnd = _refptr_g_tsaPrmtFlash->bRows + _refptr_g_tsaPrmtFlash->bCols;
  }
  return;
}


// ============================================================================
// Function: BaselineQueue_InitNormal @ 6bb021e2
// ============================================================================

void BaselineQueue_InitNormal(undefined8 param_1,undefined2 param_2)

{
  BaselineQueue_SwitchToNormal();
  *g_ptrQueue = param_1;
  *(undefined2 *)(g_ptrQueue + 1) = param_2;
  BaselineQueue_Reset();
  return;
}


// ============================================================================
// Function: BaselineQueue_InitTemp @ 6bb02228
// ============================================================================

void BaselineQueue_InitTemp(undefined8 param_1,undefined2 param_2)

{
  BaselineQueue_SwitchToTemp();
  *g_ptrQueue = param_1;
  *(undefined2 *)(g_ptrQueue + 1) = param_2;
  BaselineQueue_Reset();
  return;
}


// ============================================================================
// Function: BaselineQueue_Init @ 6bb0226e
// ============================================================================

void BaselineQueue_Init(undefined8 param_1,undefined2 param_2,undefined8 param_3,undefined2 param_4)

{
                    /* 0xc226e  397  BaselineQueue_Init */
  BaselineQueue_InitTemp(param_3,param_4);
  BaselineQueue_InitNormal(param_1,param_2);
  return;
}


// ============================================================================
// Function: SafeBaseline_SafetyUnitsInit @ 6bb022b1
// ============================================================================

void SafeBaseline_SafetyUnitsInit(undefined8 *param_1)

{
  int iVar1;
  
                    /* 0xc22b1  1244  SafeBaseline_SafetyUnitsInit */
  *param_1 = 0;
  param_1[1] = 0;
  *(undefined4 *)(param_1 + 2) = 0;
  *(undefined4 *)((longlong)param_1 + 0x14) = 0;
  *(undefined2 *)(param_1 + 3) = 0;
  *(undefined4 *)((longlong)param_1 + 0x1c) = 0;
  iVar1 = memset_s(param_1[4],(ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,0,
                   (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SafeBaseline/BaselineQueue/BaselineQueue.c",0x49,
                      "rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: SafeBaseline_Init @ 6bb02b65
// ============================================================================

void SafeBaseline_Init(char param_1)

{
  int iVar1;
  byte local_9;
  
                    /* 0xc2b65  1227  SafeBaseline_Init */
  iVar1 = memset_s(&g_safeBaseline,0x114,0,0x114);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SafeBaseline/SafeBaseline.c",0x7a,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  if (param_1 != '\0') {
    for (local_9 = 0; local_9 < 5; local_9 = local_9 + 1) {
      SafeBaseline_SafetyUnitsInit(&g_blSafetyUnits + (longlong)(int)(uint)local_9 * 0x28);
    }
    BaselineQueue_Init(&g_blSafetyUnits,5,&g_blSafetyTempUnits,5);
  }
  SafeBaseline_ResetInit();
  return;
}


// ============================================================================
// Function: SafeBaseline_InitItem @ 6bb02eaa
// ============================================================================

void SafeBaseline_InitItem(undefined8 *param_1)

{
  undefined2 uVar1;
  undefined4 uVar2;
  
                    /* 0xc2eaa  1228  SafeBaseline_InitItem */
  *param_1 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  *(int *)(param_1 + 2) = (*_refptr_g_tsaStaticPtr)->nField_0x26c;
  *(int *)((longlong)param_1 + 0x14) = (*_refptr_g_tsaStaticPtr)->nField_0x270;
  param_1[1] = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  uVar2 = TSAFold_GetCurrState();
  *(undefined4 *)((longlong)param_1 + 0x1c) = uVar2;
  uVar1 = AFE_GetScanFreq();
  *(undefined2 *)(param_1 + 3) = uVar1;
  return;
}


// ============================================================================
// Function: SafeBaseline_ResetInit @ 6bb04f10
// ============================================================================

void SafeBaseline_ResetInit(void)

{
  int iVar1;
  
                    /* 0xc4f10  1243  SafeBaseline_ResetInit */
  iVar1 = memset_s(&g_safeBaselineReset,0x34,0,0x34);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SafeBaseline/SafeBaselineReset.c",0x1c,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: SensorGesture_Init @ 6bb05aa0
// ============================================================================

void SensorGesture_Init(void)

{
  int iVar1;
  
                    /* 0xc5aa0  1272  SensorGesture_Init */
  iVar1 = memset_s(&g_sensorGesture,0x20,0,0x20);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SensorGesture/SensorGesture.c",0x15,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  if (*(int *)&_refptr_g_tsaPrmtFlash->field_0x64 != 0) {
    DAT_6bbaa0c4 = 6;
  }
  return;
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
// Function: SideTouch_BaselineInit @ 6bb0bbc0
// ============================================================================

void SideTouch_BaselineInit(void)

{
                    /* 0xcbbc0  1336  SideTouch_BaselineInit */
  SideBaseline_ResetInit();
  return;
}


// ============================================================================
// Function: SideBaseline_ResetInit @ 6bb0cf86
// ============================================================================

void SideBaseline_ResetInit(void)

{
  int iVar1;
  
                    /* 0xccf86  1308  SideBaseline_ResetInit */
  iVar1 = memset_s(&g_safeBaselineResetLeft,0x2c,0,0x2c);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SideTouch/SideTouch_BaselineReset.c",0x35,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(&g_safeBaselineResetRight,0x2c,0,0x2c);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SideTouch/SideTouch_BaselineReset.c",0x37,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  g_resetPtr = &g_safeBaselineResetLeft;
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
// Function: SmartCover_Init @ 6bb20410
// ============================================================================

void SmartCover_Init(void)

{
  short sVar1;
  undefined2 uVar2;
  undefined2 uVar3;
  ushort uVar4;
  
                    /* 0xe0410  1425  SmartCover_Init */
  if ((((*(short *)&_refptr_g_tsaPrmtFlash->field_0x6e == 0) &&
       (*(short *)&_refptr_g_tsaPrmtFlash->field_0x70 == 0)) &&
      (*(short *)&_refptr_g_tsaPrmtFlash->field_0x72 == 0)) &&
     (*(short *)&_refptr_g_tsaPrmtFlash->field_0x74 == 0)) {
    sVar1 = TSA_GetXRes();
    if (sVar1 == 0x5a0) {
      uVar2 = TSA_GetYRes();
      uVar3 = TSA_GetXRes();
      SmartCover_SetRange(0x37f,0,uVar3,uVar2);
    }
    else {
      sVar1 = TSA_GetXRes();
      if (sVar1 == 0x438) {
        uVar2 = TSA_GetYRes();
        uVar3 = TSA_GetXRes();
        SmartCover_SetRange(0x2a7,0,uVar3,uVar2);
      }
      else {
        uVar4 = TSA_GetXRes();
        uVar2 = TSA_GetYRes();
        uVar3 = TSA_GetXRes();
        SmartCover_SetRange(((uint)uVar4 * 0x3e) / 100,0,uVar3,uVar2);
      }
    }
  }
  else {
    SmartCover_SetRange(*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x6e,
                        *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x70,
                        *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x72,
                        *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x74);
  }
  return;
}


// ============================================================================
// Function: StylusAft_Init @ 6bb208d0
// ============================================================================

void StylusAft_Init(void)

{
                    /* 0xe08d0  1444  StylusAft_Init */
  g_lastStylusTimeStamp = 0;
  g_lastStylusX = 0;
  g_lastStylusY = 0;
  g_lastTiltX = 0;
  g_lastTiltY = 0;
  g_lastPalmTimeStamp = 0;
  g_lastPalmX = 0;
  g_lastPalmY = 0;
  *_refptr_g_btPenStatus = 0;
  return;
}


// ============================================================================
// Function: TSAFold_Init @ 6bb26700
// ============================================================================

void TSAFold_Init(void)

{
                    /* 0xe6700  1516  TSAFold_Init */
  *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x2d8 = 0xffffffff;
  g_data = 0;
  DAT_6bca1de4 = 0;
  DAT_6bca1de8 = &_refptr_g_tsaPrmtFlash[2].field_0x750;
  DAT_6bca1df0 = &_refptr_g_tsaPrmtFlash[2].field_0x750;
  if (isFirstInit_10254 != '\0') {
    DAT_6bca1e00 = 0;
    DAT_6bca1df8 = 0;
    isFirstInit_10254 = '\0';
  }
  TSAFold_PrmtSaveWhole();
  return;
}


// ============================================================================
// Function: TSAPlot_Init @ 6bb2972f
// ============================================================================

void TSAPlot_Init(void)

{
                    /* 0xe972f  1635  TSAPlot_Init */
  g_tsaPlot = 0;
  DAT_6bbaa186 = 0;
  DAT_6bbaa188 = 0;
  DAT_6bbaa18a = 0;
  return;
}


// ============================================================================
// Function: TSAWrapper_Init @ 6bb29760
// ============================================================================

void TSAWrapper_Init(void)

{
  int iVar1;
  
                    /* 0xe9760  1730  TSAWrapper_Init */
  iVar1 = memset_s(&g_preReport,0x130,0,0x130);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/TSAWrapper/TSAWrapper.c",0x21,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(&g_report,0x130,0,0x130);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/TSAWrapper/TSAWrapper.c",0x24,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
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
// Function: Thenar_Init @ 6bb2fce0
// ============================================================================

void Thenar_Init(void)

{
  ushort local_a;
  
                    /* 0xefce0  2458  Thenar_Init */
  for (local_a = 0; local_a < 2; local_a = local_a + 1) {
    (&g_sideHandEnterCnt)[(int)(uint)local_a] = 0xf;
    *(undefined4 *)(&g_sideHandExitCnt + (longlong)(int)(uint)local_a * 4) = 0xf;
  }
  return;
}


// ============================================================================
// Function: UDFPEvent_Init @ 6bb321ac
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void UDFPEvent_Init(void)

{
                    /* 0xf21ac  2561  UDFPEvent_Init */
  UDFPEvent_SetSensorArea
            (*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x76,
             *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x78,
             *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x7a,
             *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x7c);
  UDFPEvent_SetFingerArea
            (*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x7e,
             *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x80,
             *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x82,
             *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x84);
  _g_fpHoverArea = 0x200;
  _DAT_6bbaa8fa = (_refptr_g_tsaPrmtFlash->bCols - 2) * 0x100;
  _DAT_6bbaa8fc = 0x200;
  _DAT_6bbaa8fe = (ushort)_refptr_g_tsaPrmtFlash->bRows << 8;
  g_lastHoverUpTimeStamp = 0;
  g_lastFingerUpTimeStamp = 0;
  g_udfpEventLogDebounce = 0;
  return;
}


// ============================================================================
// Function: VirtualKey_Init @ 6bb33c60
// ============================================================================

void VirtualKey_Init(void)

{
  int iVar1;
  
                    /* 0xf3c60  2585  VirtualKey_Init */
  iVar1 = memset_s(&g_outVirtualKey,0xa8,0,0xa8);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/VirtualKey/VirtualKey.c",0x22,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  DAT_6bca3b48 = 0x73;
  DAT_6bca3b4a = 0;
  DAT_6bca3b4c = (undefined2)*(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x680;
  DAT_6bca3b50 = 0;
  DAT_6bca3b58 = 0x72;
  DAT_6bca3b5a = 0;
  DAT_6bca3b5c = (undefined2)*(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x680;
  DAT_6bca3b60 = 0;
  DAT_6bca3b68 = 0x74;
  DAT_6bca3b6a = 0;
  DAT_6bca3b6c = 0x32;
  DAT_6bca3b70 = 0;
  DAT_6bca3b78 = 0x71;
  DAT_6bca3b7a = 0;
  DAT_6bca3b7c = (undefined2)*(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x680;
  DAT_6bca3b80 = 0;
  g_dataVirtualKey = 4;
  memcpy(&g_dataVirtualKeyPrev,&g_dataVirtualKey,0xa8);
  VirtualKey_VolumeInit();
  return;
}


// ============================================================================
// Function: VirtualKey_VolumeInit @ 6bb344d0
// ============================================================================

void VirtualKey_VolumeInit(void)

{
                    /* 0xf44d0  2595  VirtualKey_VolumeInit */
  DAT_6bca3d7a = 0x96;
  DAT_6bca3d78 = 10;
  DAT_6bca3d76 = 0x50;
  DAT_6bca3d72 = 0x50;
  DAT_6bca3d74 = 0x50;
  g_volumeData = 0x50;
  DAT_6bca3d7c = 8;
  DAT_6bca3d7e = 0;
  TSAIDE_SetIndexVolume(8);
  g_timeStampVolumeAction = 0;
  return;
}


// ============================================================================
// Function: SignalDisparity_Init @ 6bb35510
// ============================================================================

void SignalDisparity_Init(void)

{
                    /* 0xf5510  1415  SignalDisparity_Init */
  *(undefined4 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x16 = 0;
  return;
}


// ============================================================================
// Function: Context_Init @ 6bb35f50
// ============================================================================

void Context_Init(void)

{
  int iVar1;
  
                    /* 0xf5f50  511  Context_Init */
  g_registryCount = 0;
  iVar1 = memset_s(&g_contextCurFrame,0x105c,0,0x105c);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Plugin/Context/Context.c",0x29,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  DAT_6bbab048 = 0;
  iVar1 = memset_s(&g_contextKeyFrame,0x105c,0,0x105c);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Plugin/Context/Context.c",0x2d,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  DAT_6bbac0c8 = 1;
  iVar1 = memset_s(&g_contextReader,0x105c,0,0x105c);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Plugin/Context/Context.c",0x31,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(&g_registry,0x230,0,0x230);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Plugin/Context/Context.c",0x34,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: TSAIDE_Init @ 6bb376e0
// ============================================================================

void TSAIDE_Init(void)

{
                    /* 0xf76e0  1548  TSAIDE_Init */
  TSAIDE_Reset();
  g_tsaIdePreFltRpt = 0;
  return;
}


// ============================================================================
// Function: TSALog_Init @ 6bb38530
// ============================================================================

void TSALog_Init(void)

{
                    /* 0xf8530  1567  TSALog_Init */
  if (g_tsaLogLevel != '\0') {
    StrPrintf_Process(0,
                      "\nTSA Library Started.............................................................\n"
                     );
    StrPrintf_Process(0,"\nsize of INT8U: ",1);
    StrPrintf_Process(0,"\nsize of INT8S: ",1);
    StrPrintf_Process(0,"\nsize of INT16U: ",2);
    StrPrintf_Process(0,"\nsize of INT16S: ",2);
    StrPrintf_Process(0,"\nsize of INT32U: ",4);
    StrPrintf_Process(0,"\nsize of INT32S: ",4);
    StrPrintf_Process(0,"\nsize of INT64U: ",8);
    StrPrintf_Process(0,"\nsize of INT64S: ",8);
  }
  return;
}


// ============================================================================
// Function: ChrPrintf_Init @ 6bb3a937
// ============================================================================

void ChrPrintf_Init(longlong param_1)

{
                    /* 0xfa937  496  ChrPrintf_Init */
  if (param_1 != 0) {
    g_chrPrintfPutf = param_1;
  }
  return;
}


// ============================================================================
// Function: TSAPrintf_Init @ 6bb3ac2b
// ============================================================================

void TSAPrintf_Init(void)

{
                    /* 0xfac2b  1641  TSAPrintf_Init */
  TSAPrintf_InitHW();
  StrPrintf_Init(TSAPrintf_PutStrHW);
  TSAPrintf_StartLogFile(g_logFileNameCT);
  return;
}


// ============================================================================
// Function: TSAPrintf_InitHW @ 6bb3ac5a
// ============================================================================

void TSAPrintf_InitHW(void)

{
                    /* 0xfac5a  1642  TSAPrintf_InitHW */
  return;
}


// ============================================================================
// Function: StrPrintf_Init @ 6bb3ad43
// ============================================================================

void StrPrintf_Init(undefined8 param_1)

{
  g_strPrintfPutf = param_1;
                    /* 0xfad43  1431  StrPrintf_Init */
  StrPrintf_ClearLogBuffer();
  return;
}


// ============================================================================
// Function: Python_Init @ 6bb3af00
// ============================================================================

undefined8 Python_Init(longlong param_1,ulonglong param_2)

{
  int iVar1;
  undefined8 uVar2;
  wchar_t local_1a8 [206];
  int local_c;
  
                    /* 0xfaf00  1131  Python_Init */
  if ((param_1 == 0) || (199 < param_2)) {
    uVar2 = 0;
  }
  else {
    local_c = swprintf_s(local_1a8,200,L"%hs",param_1);
    if (local_c == -1) {
      uVar2 = 0;
    }
    else {
      Py_SetPythonHome(local_1a8);
      Py_Initialize();
      iVar1 = Py_IsInitialized();
      if (iVar1 == 0) {
        if (*_refptr_g_tsaLogLoglevel != '\0') {
          StrPrintf_Process(0,"Python init failed!\n");
        }
        uVar2 = 0;
      }
      else {
        PyRun_SimpleStringFlags("import sys",0);
        g_pythonInitialized = 1;
        uVar2 = 1;
      }
    }
  }
  return uVar2;
}


// ============================================================================
// Function: TSAPrmt_ReInit @ 6bb3b8e5
// ============================================================================

void TSAPrmt_ReInit(void)

{
                    /* 0xfb8e5  1652  TSAPrmt_ReInit */
  TSATTPrmt_Init();
  TSARTPrmt_Init();
  TSAConstPrmt_Init();
  TSARamPrmt_Init();
  TSAPrmt_Reset();
  return;
}


// ============================================================================
// Function: TSAConstPrmt_Init @ 6bb3bb00
// ============================================================================

void TSAConstPrmt_Init(void)

{
  byte bVar1;
  byte bVar2;
  byte bVar3;
  int local_10;
  
                    /* 0xfbb00  1485  TSAConstPrmt_Init */
  bVar1 = _refptr_g_tsaPrmtFlash->bCols;
  DAT_6bcab620 = (ushort)bVar1 << 8;
  bVar2 = _refptr_g_tsaPrmtFlash->bRows;
  DAT_6bcab622 = (ushort)bVar2 << 8;
  DAT_6bcab628 = (_refptr_g_tsaPrmtFlash->field_0x91 - _refptr_g_tsaPrmtFlash->field_0x90) + '\x01';
  DAT_6bcab629 = (_refptr_g_tsaPrmtFlash->field_0x93 - _refptr_g_tsaPrmtFlash->field_0x92) + '\x01';
  DAT_6bcab62a = (ushort)(byte)_refptr_g_tsaPrmtFlash->field_0x90 << 8;
  DAT_6bcab62c = ((byte)_refptr_g_tsaPrmtFlash->field_0x91 + 1) * 0x100;
  DAT_6bcab62e = (ushort)(byte)_refptr_g_tsaPrmtFlash->field_0x92 << 8;
  DAT_6bcab630 = ((byte)_refptr_g_tsaPrmtFlash->field_0x93 + 1) * 0x100;
  DAT_6bcab632 = (*(short *)&_refptr_g_tsaPrmtFlash->field_0x96 -
                 *(short *)&_refptr_g_tsaPrmtFlash->field_0x94) + 1;
  DAT_6bcab634 = (*(short *)&_refptr_g_tsaPrmtFlash->field_0x9a -
                 *(short *)&_refptr_g_tsaPrmtFlash->field_0x98) + 1;
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    DAT_6bcab625 = _refptr_g_tsaPrmtFlash->bCols;
    DAT_6bcab624 = _refptr_g_tsaPrmtFlash->bRows;
  }
  else {
    DAT_6bcab625 = _refptr_g_tsaPrmtFlash->bRows;
    DAT_6bcab624 = _refptr_g_tsaPrmtFlash->bCols;
  }
  bVar3 = 0;
  for (local_10 = 0; local_10 < (int)(uint)DAT_6bcab625; local_10 = local_10 + 1) {
    if (bVar3 < (byte)(&_refptr_g_tsaPrmtFlash->field_0x62c)[local_10]) {
      bVar3 = (&_refptr_g_tsaPrmtFlash->field_0x62c)[local_10];
    }
  }
  DAT_6bcab626 = bVar3 + 1;
  DAT_6bcab604 = (((int)(char)_refptr_g_tsaPrmtFlash->field_0x5af +
                  (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a6 +
                  (int)(char)_refptr_g_tsaPrmtFlash->field_0x5ae) * 0x10000) / ((uint)bVar1 << 8);
  DAT_6bcab608 = (((int)(char)_refptr_g_tsaPrmtFlash->field_0x5b1 +
                  (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a8 +
                  (int)(char)_refptr_g_tsaPrmtFlash->field_0x5b0) * 0x10000) / ((uint)bVar2 << 8);
  if ((short)_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase1 ==
      *(short *)&_refptr_g_tsaPrmtFlash[1].field_0x4e) {
    DAT_6bcab60c = 1;
  }
  else {
    DAT_6bcab60c = (undefined2)
                   ((((uint)*(byte *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase1 + 3)
                     - (uint)*(byte *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase1 + 2)
                     ) * 0x400) /
                   ((uint)(ushort)_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase1 -
                   (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[1].field_0x4e));
  }
  if (*(short *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase1_or + 2) ==
      (short)_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase1_or) {
    DAT_6bcab60e = 1;
  }
  else {
    DAT_6bcab60e = (undefined2)
                   ((((uint)*(byte *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase2 + 1)
                     - (uint)(byte)_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase2) * 0x400) /
                   ((uint)*(ushort *)
                           ((longlong)&_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase1_or + 2) -
                   (uint)(ushort)_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase1_or));
  }
  if ((short)_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase2_or ==
      *(short *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase2 + 2)) {
    DAT_6bcab610 = 1;
  }
  else {
    DAT_6bcab610 = (undefined2)
                   ((((uint)*(byte *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase2_or +
                                     3) -
                     (uint)*(byte *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase2_or + 2
                                    )) * 0x400) /
                   ((uint)(ushort)_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase2_or -
                   (uint)*(ushort *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase2 + 2)))
    ;
  }
  if (*(short *)&_refptr_g_tsaPrmtFlash->field_0x5a6 == 0) {
    DAT_6bcab612 = 1;
  }
  else {
    DAT_6bcab612 = (undefined2)
                   (((uint)*(byte *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwAftModeFeatureMask + 1)
                    * (uint)bVar1 * 0x100) / (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a6);
  }
  if (*(short *)&_refptr_g_tsaPrmtFlash->field_0x5a8 == 0) {
    DAT_6bcab614 = 1;
  }
  else {
    DAT_6bcab614 = (undefined2)
                   (((uint)*(byte *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwAftModeFeatureMask + 1)
                    * (uint)bVar2 * 0x100) / (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a8);
  }
  if (*(short *)&_refptr_g_tsaPrmtFlash->field_0x5a6 == 0) {
    DAT_6bcab616 = 1;
  }
  else {
    DAT_6bcab616 = (undefined2)
                   (((uint)(byte)_refptr_g_tsaPrmtFlash[1].dwAftModeFeatureMask *
                    (uint)bVar1 * 0x100) / (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a6);
  }
  if (*(short *)&_refptr_g_tsaPrmtFlash->field_0x5a8 == 0) {
    DAT_6bcab618 = 1;
  }
  else {
    DAT_6bcab618 = (undefined2)
                   (((uint)(byte)_refptr_g_tsaPrmtFlash[1].dwAftModeFeatureMask *
                    (uint)bVar2 * 0x100) / (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a8);
  }
  DAT_6bcab61a = (char)(0x1000 / (ulonglong)
                                 (longlong)(int)(uint)(byte)_refptr_g_tsaPrmtFlash->field_0x1b) +
                 '\x01';
  DAT_6bcab61e = (ushort)((0x32400 / (ulonglong)(byte)_refptr_g_tsaPrmtFlash->field_0x1b) /
                         (ulonglong)(byte)_refptr_g_tsaPrmtFlash->field_0x1b);
  DAT_6bcab61c = (short)(0x10000 /
                        (ulonglong)
                        (longlong)
                        ((int)(0x1000 / (ulonglong)
                                        (longlong)
                                        (int)(uint)(byte)_refptr_g_tsaPrmtFlash->field_0x1b) + 1));
  g_tsaPrmtFlashConst =
       (short)(0x100000 /
              (ulonglong)
              ((int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold *
               (uint)DAT_6bcab61e *
               (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x19) *
               (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x19) >> 10));
  DAT_6bcab638 = ((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold * 0x3fff) /
                 (*(int *)&_refptr_g_tsaPrmtFlash->field_0x5fc -
                 *(int *)&_refptr_g_tsaPrmtFlash->field_0x600);
  DAT_6bcab63c = 0x7fff - (DAT_6bcab638 * *(int *)&_refptr_g_tsaPrmtFlash->field_0x5fc) / 0x3fff;
  if ((int)((int)(((int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold +
                   (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d6 +
                   (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d8 +
                  (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5da) * 3) / 2 +
           (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d2) < 0x10000) {
    DAT_6bcab65a = *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x5d2 +
                   (short)((int)(((int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold
                                  + (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d6 +
                                  (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d8 +
                                 (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5da) * 3) / 2);
  }
  else {
    DAT_6bcab65a = -1;
  }
  if ((int)((uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d0 -
           (int)(((int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold +
                  (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d6 +
                  (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d8 +
                 (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5da) * 3) / 2) < 0) {
    DAT_6bcab65c = 0;
  }
  else {
    DAT_6bcab65c = *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x5d0 -
                   (short)((int)(((int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold
                                  + (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d6 +
                                  (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d8 +
                                 (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5da) * 3) / 2);
  }
  if ((int)((int)(((int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold +
                   (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d6 +
                   (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d8 +
                  (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5da) * 3) / 2 +
           (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d4) < 0x8000) {
    DAT_6bcab65e = (short)((int)(((int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold
                                  + (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d6 +
                                  (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d8 +
                                 (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5da) * 3) / 2)
                   + *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x5d4;
  }
  else {
    DAT_6bcab65e = 0x7fff;
  }
  if ((int)(-((int)(((int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold +
                     (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d6 +
                     (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d8 +
                    (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5da) * 3) / 2) -
           (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d4) < -0x8000) {
    DAT_6bcab660 = -0x8000;
  }
  else {
    DAT_6bcab660 = -(short)((int)(((int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold
                                   + (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d6 +
                                   (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d8 +
                                  (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5da) * 3) / 2)
                   - *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x5d4;
  }
  return;
}


// ============================================================================
// Function: TSADynPrmt_SSInit @ 6bb3c6a0
// ============================================================================

void TSADynPrmt_SSInit(void)

{
  DAT_6bcab6ac._2_2_ = _refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax;
  DAT_6bcab6ac._0_2_ = (short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax / 3;
  return;
}


// ============================================================================
// Function: TSADynPrmt_Init @ 6bb3c6f2
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void TSADynPrmt_Init(void)

{
                    /* 0xfc6f2  1497  TSADynPrmt_Init */
  _g_tsaPrmtDynamic = *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x5d8;
  _DAT_6bcab688 = *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x5e0;
  _DAT_6bcab690 = *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x5e8;
  _DAT_6bcab698 = *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x5f0;
  _DAT_6bcab6a0 = *(undefined4 *)&_refptr_g_tsaPrmtFlash->field_0x5f8;
  DAT_6bcab6ec = *(undefined1 *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x19);
  DAT_6bcab6f8 = *(undefined8 *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x24);
  DAT_6bcab6f0 = *(undefined8 *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x1c);
  _DAT_6bcab700 = _refptr_g_tsaPrmtFlash[1].pDim1PitchMap[7];
  DAT_6bcab708 = *(undefined4 *)(_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 8);
  DAT_6bcab6bc = *(undefined4 *)&_refptr_g_tsaPrmtFlash->field_0x620;
  DAT_6bcab6c0 = *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x624;
  _DAT_6bcab6a4 = *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x608;
  DAT_6bcab6ac = *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x610;
  _DAT_6bcab6b4 = *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x618;
  DAT_6bcab70c = _refptr_g_tsaPrmtFlash[1].pDim1PitchMap[0xc];
  DAT_6bcab714 = *(undefined4 *)(_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0xd);
  DAT_6bcab6da._0_2_ = 2000;
  DAT_6bcab6d2._6_2_ = 2000;
  TouchThold_Init();
  TSADynPrmt_GetUnderWaterThold();
  TSADynPrmt_SSInit();
  return;
}


// ============================================================================
// Function: TSARamPrmt_Init @ 6bb3d200
// ============================================================================

void TSARamPrmt_Init(void)

{
                    /* 0xfd200  1670  TSARamPrmt_Init */
  return;
}


// ============================================================================
// Function: TSARTPrmt_Init @ 6bb3d3e0
// ============================================================================

void TSARTPrmt_Init(void)

{
  int iVar1;
  
                    /* 0xfd3e0  1663  TSARTPrmt_Init */
  TSARTPrmt_SyncWithTTPrmt();
  DAT_6bcab904 = g_tsaRTPrmtDefault;
  DAT_6bcab908 = g_tsaRTPrmtDefault * 2;
  iVar1 = g_tsaRTPrmtDefault;
  if (g_tsaRTPrmtDefault < 0) {
    iVar1 = g_tsaRTPrmtDefault + 3;
  }
  DAT_6bcab90c = g_tsaRTPrmtDefault - (iVar1 >> 2);
  DAT_6bcab910 = 
  "touch threshold for finger mode;\ntuning based on LTI of 4mm finger without film and 5mm finger with film in device floating situation;"
  ;
  DAT_6bcab91c = DAT_6bcab918;
  DAT_6bcab920 = 100;
  DAT_6bcab924 = 0;
  DAT_6bcab928 = 
  "non-axis-change related moving distance(pixel) threshold to determine click or flick action, which will be applied to determine whether to apply click suppression or not;\nthe more threshold is the better click performance and poorer flick performance will be;"
  ;
  DAT_6bcab934 = DAT_6bcab930;
  DAT_6bcab938 = 0x96;
  DAT_6bcab93c = 0;
  DAT_6bcab940 = 
  "moving distance threshold(pixel) to determine whether to apply click suppression or not;\nthe more threshold is the better click performance and poorer flick performance will be;"
  ;
  DAT_6bcab94c = DAT_6bcab948;
  DAT_6bcab950 = 0x60;
  DAT_6bcab954 = 0x10;
  DAT_6bcab958 = 
  "128 scale up ratio to peak signal to determine touch zone traversal threshold for default situation;\nthe more threshold is the better multi-finger performance and poorer fat-finger performance will be;"
  ;
  DAT_6bcab964 = DAT_6bcab960;
  DAT_6bcab968 = 0x60;
  DAT_6bcab96c = 0x30;
  DAT_6bcab970 = 
  "128 scale up ratio to peak signal to determine touch zone traversal threshold for device grounding situation;\nthe more threshold is the better multi-finger performance and poorer fat-finger performance will be;"
  ;
  DAT_6bcab97c = DAT_6bcab978;
  DAT_6bcab980 = 0x60;
  DAT_6bcab984 = 0x10;
  DAT_6bcab988 = 
  "128 scale up ratio to peak signal to determine touch zone traversal threshold in case device floating detected;\nthe more threshold is the better multi-finger performance and poorer fat-finger performance will be;"
  ;
  DAT_6bcab994 = DAT_6bcab990;
  DAT_6bcab998 = 0x18;
  DAT_6bcab99c = 8;
  DAT_6bcab9a0 = 
  "acceleration threshold to determine whether to do line connection between 2 touches;\nthe more threshold is the better line drawing performance and poorer 2 closed finger tapping performance will be;"
  ;
  DAT_6bcab9ac = DAT_6bcab9a8;
  DAT_6bcab9b0 = 60000;
  DAT_6bcab9b4 = 0;
  DAT_6bcab9b8 = 
  "debounce time threshold in ms to enter IDLE mode after being allowed;\nthe more threshold is the better response time but more power will be consumed;"
  ;
  DAT_6bcab9c4 = DAT_6bcab9c0;
  DAT_6bcab9c8 = 0x30;
  DAT_6bcab9cc = 0x10;
  DAT_6bcab9d0 = 
  "16 scale up ratio to touch threshold to determine whether to report touch directly or debounce the touch;\nthe less threshold is the better response time but more sensitive to noise;"
  ;
  DAT_6bcab9dc = DAT_6bcab9d8;
  DAT_6bcab9e0 = 0x1e;
  DAT_6bcab9e4 = 0;
  DAT_6bcab9e8 = 
  "distance threshold in mm to activate glove mode;\nthe less threshold is the easier to enter glove mode and the higher possibility for ghost touch;"
  ;
  DAT_6bcab9f4 = DAT_6bcab9f0;
  DAT_6bcab9f8 = 60000;
  DAT_6bcab9fc = 0;
  DAT_6bcaba00 = 
  "time threshold in ms to allowed moving after click;\nthe less threshold is the easier for move and the higher for better click;"
  ;
  DAT_6bcaba0c = DAT_6bcaba08;
  DAT_6bcaba10 = 0x7fffffff;
  DAT_6bcaba14 = 0x80000000;
  DAT_6bcaba18 = "volumeActiveAreaStart";
  DAT_6bcaba24 = DAT_6bcaba20;
  DAT_6bcaba28 = 0x7fffffff;
  DAT_6bcaba2c = 0x80000000;
  DAT_6bcaba30 = "volumeActiveAreaEnd";
  DAT_6bcaba3c = DAT_6bcaba38;
  DAT_6bcaba40 = 0x7fffffff;
  DAT_6bcaba44 = 0x80000000;
  DAT_6bcaba48 = "volumeGreenAreaStart";
  DAT_6bcaba54 = DAT_6bcaba50;
  DAT_6bcaba58 = 0x7fffffff;
  DAT_6bcaba5c = 0x80000000;
  DAT_6bcaba60 = "volumeGreenAreaEnd";
  DAT_6bcaba6c = DAT_6bcaba68;
  DAT_6bcaba70 = 0x7fffffff;
  DAT_6bcaba74 = 0x80000000;
  DAT_6bcaba78 = "volumeSideEnabled";
  memcpy(&g_tsaRTPrmt,&g_tsaRTPrmtDefault,0x180);
  return;
}


// ============================================================================
// Function: TSATTPrmt_Init @ 6bb3df90
// ============================================================================

void TSATTPrmt_Init(void)

{
  byte bVar1;
  byte bVar2;
  byte bVar3;
  byte bVar4;
  int iVar5;
  
                    /* 0xfdf90  1724  TSATTPrmt_Init */
  g_tsaTTPrmt = (int)*(short *)&_refptr_g_tsaPrmtFlash->field_0x5da;
  DAT_6bcaba84 = g_tsaTTPrmt;
  DAT_6bcaba88 = g_tsaTTPrmt * 2;
  iVar5 = g_tsaTTPrmt;
  if (g_tsaTTPrmt < 0) {
    iVar5 = g_tsaTTPrmt + 3;
  }
  DAT_6bcaba8c = g_tsaTTPrmt - (iVar5 >> 2);
  DAT_6bcaba90 = 
  "touch threshold for glove mode;\ntuning based on LTI of smart-cover/thick-glove in device floating situation;\nNOTE: if noise bigger than glove threshold, then glove performance will be sacrificed to ensure no ghost touch;"
  ;
  DAT_6bcaba98 = (int)(short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax;
  DAT_6bcaba9c = DAT_6bcaba98;
  DAT_6bcabaa0 = DAT_6bcaba98 * 2;
  iVar5 = DAT_6bcaba98;
  if (DAT_6bcaba98 < 0) {
    iVar5 = DAT_6bcaba98 + 3;
  }
  DAT_6bcabaa4 = DAT_6bcaba98 - (iVar5 >> 2);
  DAT_6bcabaa8 = 
  "touch threshold for finger mode;\ntuning based on LTI of 4mm finger without film and 5mm finger with film in device floating situation;"
  ;
  DAT_6bcabab0 = (int)*(short *)&_refptr_g_tsaPrmtFlash->field_0x5d8;
  DAT_6bcabab4 = (int)*(short *)&_refptr_g_tsaPrmtFlash->field_0x5d8;
  DAT_6bcabab8 = (short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax * 2;
  DAT_6bcababc = ((short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax * 3) / 2;
  DAT_6bcabac0 = 
  "touch threshold to exit glove mode or confirm finger mode;\nthe higher threshold is the better glove performance but might lead to too sensitive issue for finger;"
  ;
  DAT_6bcabac8 = *(undefined4 *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x3c);
  DAT_6bcabacc = DAT_6bcabac8;
  DAT_6bcabad0 = 0xf;
  DAT_6bcabad4 = 0;
  DAT_6bcabad8 = 
  "define sides to apply glove filter which will filter glove touch with >4 grip ratio;\nNOTE: TOP_SIDE=1,BOTTOM_SIDE=2,LEFT_SIDE=4,RIGHT_SIDE=8;\ntuning based on performance of status-bar drop down and navigation-bar drop up with finger and glove;"
  ;
  DAT_6bcabae0 = (uint)*(ushort *)(_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 8);
  DAT_6bcabae4 = (uint)*(ushort *)(_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 8);
  DAT_6bcabae8 = 0x1e;
  DAT_6bcabaec = 0;
  DAT_6bcabaf0 = 
  "distance threshold in mm to activate glove mode;\nthe less threshold is the easier to enter glove mode and the higher possibility for ghost touch;"
  ;
  DAT_6bcabaf8 = (uint)*(ushort *)(_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 4);
  DAT_6bcabafc = DAT_6bcabaf8;
  DAT_6bcabb00 = DAT_6bcabaf8;
  DAT_6bcabb04 = DAT_6bcabaf8;
  DAT_6bcabb08 = "obsolete;";
  DAT_6bcabb10 = (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x24);
  DAT_6bcabb14 = DAT_6bcabb10;
  DAT_6bcabb18 = 100;
  DAT_6bcabb1c = 0;
  DAT_6bcabb20 = 
  "non-axis-change related moving distance(pixel) threshold to determine click or flick action, which will be applied to determine whether to apply click suppression or not;\nthe more threshold is the better click performance and poorer flick performance will be;"
  ;
  DAT_6bcabb28 = (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x26);
  DAT_6bcabb2c = DAT_6bcabb28;
  DAT_6bcabb30 = 0x96;
  DAT_6bcabb34 = 0;
  DAT_6bcabb38 = 
  "moving distance threshold(pixel) to determine whether to apply click suppression or not;\nthe more threshold is the better click performance and poorer flick performance will be;"
  ;
  DAT_6bcabb40 = (uint)*(byte *)(_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 5);
  DAT_6bcabb44 = (uint)*(byte *)(_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 5);
  DAT_6bcabb48 = 0x78;
  DAT_6bcabb4c = 0;
  DAT_6bcabb50 = 
  "non-axis-change related moving distance(pixel) threshold to determine default action of next touch, which will be applied to determine whether to apply click suppression or not by default for next touch;\nthe more threshold is the better click performance and poorer flick performance will be;"
  ;
  DAT_6bcabb58 = (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x29);
  DAT_6bcabb5c = DAT_6bcabb58;
  DAT_6bcabb60 = DAT_6bcabb58;
  DAT_6bcabb64 = DAT_6bcabb58;
  DAT_6bcabb68 = "obsolete;";
  DAT_6bcabb70 = _refptr_g_tsaPrmtFlash->dwPeakProcessingMode;
  DAT_6bcabb74 = _refptr_g_tsaPrmtFlash->dwPeakProcessingMode;
  DAT_6bcabb78 = 3;
  DAT_6bcabb7c = 1;
  DAT_6bcabb80 = 
  "Touch panel sensor architecture, could be OnCell(1), Hybrid Incell(2), Full Incell(3);";
  DAT_6bcabb88 = _refptr_g_tsaPrmtFlash->dwAftModeFeatureMask;
  DAT_6bcabb8c = _refptr_g_tsaPrmtFlash->dwAftModeFeatureMask;
  DAT_6bcabb90 = 0x7fffffff;
  DAT_6bcabb94 = 0;
  DAT_6bcabb98 = "specific in which scenarios that anti-touch feature will be enabled;";
  DAT_6bcabba0 = *(undefined4 *)&_refptr_g_tsaPrmtFlash->field_0x64;
  DAT_6bcabba4 = *(undefined4 *)&_refptr_g_tsaPrmtFlash->field_0x64;
  DAT_6bcabba8 = 0x7fffffff;
  DAT_6bcabbac = 0;
  DAT_6bcabbb0 = "specific in which scenarios that proximity feature will be enabled;";
  DAT_6bcabbb8 = (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x81f;
  DAT_6bcabbbc = (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x81f;
  DAT_6bcabbc0 = 0x60;
  DAT_6bcabbc4 = 0x30;
  DAT_6bcabbc8 = 
  "128 scale up ratio to peak signal to determine touch zone traversal threshold for device grounding situation;\nthe more threshold is the better multi-finger performance and poorer fat-finger performance will be;"
  ;
  DAT_6bcabbd0 = (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x820;
  DAT_6bcabbd4 = (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x820;
  DAT_6bcabbd8 = 0x60;
  DAT_6bcabbdc = 0x10;
  DAT_6bcabbe0 = 
  "128 scale up ratio to peak signal to determine touch zone traversal threshold in case device floating detected;\nthe more threshold is the better multi-finger performance and poorer fat-finger performance will be;"
  ;
  DAT_6bcabbe8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[1].field_0x4a;
  DAT_6bcabbec = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[1].field_0x4a;
  DAT_6bcabbf0 = 0x18;
  DAT_6bcabbf4 = 8;
  DAT_6bcabbf8 = 
  "acceleration threshold to determine whether to do line connection between 2 touches;\nthe more threshold is the better line drawing performance and poorer 2 closed finger tapping performance will be; "
  ;
  DAT_6bcabc90 = (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x836;
  DAT_6bcabc94 = (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x836;
  DAT_6bcabc98 = 0x28;
  DAT_6bcabc9c = 0x1c;
  DAT_6bcabca0 = "threshold of size in mm to determine larger object;";
  DAT_6bcabca8 = (int)*(short *)&_refptr_g_tsaPrmtFlash->field_0x626;
  DAT_6bcabcac = (int)*(short *)&_refptr_g_tsaPrmtFlash->field_0x626;
  DAT_6bcabcb0 = 0x7fff;
  DAT_6bcabcb4 = 0;
  DAT_6bcabcb8 = "common mode noise threshold for sensors\' signal;";
  DAT_6bcabcc0 = (int)*(short *)&_refptr_g_tsaPrmtFlash->field_0x628;
  DAT_6bcabcc4 = (int)*(short *)&_refptr_g_tsaPrmtFlash->field_0x628;
  DAT_6bcabcc8 = 1000;
  DAT_6bcabccc = 0;
  DAT_6bcabcd0 = "common mode noise threshold for delta signal of neighbouring sensor;";
  DAT_6bcabc00 = (int)*(short *)(_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0xc);
  DAT_6bcabc04 = (int)*(short *)(_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0xc);
  DAT_6bcabc08 = 0x32;
  DAT_6bcabc0c = 10;
  DAT_6bcabc10 = 
  "percentage to HTI threshold to determine charger noise threshold for hybrid incell;";
  DAT_6bcabc18 = (uint)*(ushort *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x62);
  DAT_6bcabc1c = DAT_6bcabc18;
  DAT_6bcabc20 = DAT_6bcabc18;
  DAT_6bcabc24 = DAT_6bcabc18;
  DAT_6bcabc28 = "obsolete;";
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    bVar1 = _refptr_g_tsaPrmtFlash[1].field_0x4;
    bVar2 = _refptr_g_tsaPrmtFlash[1].field_0x15;
    bVar3 = _refptr_g_tsaPrmtFlash[1].field_0x26;
    bVar4 = *(byte *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwPeakProcessingMode + 3);
  }
  else {
    bVar1 = _refptr_g_tsaPrmtFlash[1].field_0x26;
    bVar2 = *(byte *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwPeakProcessingMode + 3);
    bVar3 = _refptr_g_tsaPrmtFlash[1].field_0x4;
    bVar4 = _refptr_g_tsaPrmtFlash[1].field_0x15;
  }
  DAT_6bcabc78 = (uint)bVar4;
  DAT_6bcabc60 = (uint)bVar3;
  DAT_6bcabc48 = (uint)bVar2;
  DAT_6bcabc30 = (uint)bVar1;
  DAT_6bcabc34 = DAT_6bcabc30;
  DAT_6bcabc38 = 0xff;
  DAT_6bcabc3c = 0x10;
  DAT_6bcabc40 = 
  "threshold for centroid top edge correction\n;the less threshold is, the easier to reach edge";
  DAT_6bcabc4c = DAT_6bcabc48;
  DAT_6bcabc50 = 0xff;
  DAT_6bcabc54 = 0x10;
  DAT_6bcabc58 = 
  "threshold for centroid bottom edge correction\n;the less threshold is, the easier to reach edge";
  DAT_6bcabc64 = DAT_6bcabc60;
  DAT_6bcabc68 = 0xff;
  DAT_6bcabc6c = 0x10;
  DAT_6bcabc70 = 
  "threshold for centroid left edge correction\n;the less threshold is, the easier to reach edge";
  DAT_6bcabc7c = DAT_6bcabc78;
  DAT_6bcabc80 = 0xff;
  DAT_6bcabc84 = 0x10;
  DAT_6bcabc88 = 
  "threshold for centroid right edge correction\n;the less threshold is, the easier to reach edge";
  DAT_6bcabcd8 = *(undefined4 *)&_refptr_g_tsaPrmtFlash->field_0x838;
  DAT_6bcabcdc = *(undefined4 *)&_refptr_g_tsaPrmtFlash->field_0x838;
  DAT_6bcabce0 = ((short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax * 3) / 2;
  DAT_6bcabce4 = 0;
  DAT_6bcabce8 = "edge width threshold of ewx/ewy for AFT";
  DAT_6bcabcf0 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x5e0;
  DAT_6bcabcf4 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x5e0;
  DAT_6bcabcf8 = 0x7fffffff;
  DAT_6bcabcfc = 0;
  DAT_6bcabd00 = "proximity feature configuration;";
  DAT_6bcabd08 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x60c;
  DAT_6bcabd0c = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x60c;
  DAT_6bcabd10 = _refptr_g_tsaPrmtFlash->bCols - 2;
  DAT_6bcabd14 = 5;
  DAT_6bcabd18 = "define proximity detection area along Y axis;";
  DAT_6bcabd20 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x5e4;
  DAT_6bcabd24 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x5e4;
  DAT_6bcabd28 = 5000;
  DAT_6bcabd2c = 200;
  DAT_6bcabd30 = 
  "signal threshold to trigger proximity event;\nthe bigger threshold is the better to avoid false trigger but the poorer to trigger proximity;"
  ;
  DAT_6bcabd38 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x5e8;
  DAT_6bcabd3c = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x5e8;
  DAT_6bcabd40 = 5000;
  DAT_6bcabd44 = 200;
  DAT_6bcabd48 = 
  "min threshold for proximity to lift off;\nthe bigger threshold is the better to exit proximity but the poorer to avoid false proximity exit;"
  ;
  DAT_6bcabd50 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x5ec;
  DAT_6bcabd54 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x5ec;
  DAT_6bcabd58 = 10000;
  DAT_6bcabd5c = 500;
  DAT_6bcabd60 = 
  "max threshold for proximity to lift off;\nthe bigger threshold is the better to exit proximity but the poorer to avoid false proximity exit;"
  ;
  DAT_6bcabd68 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5f8;
  DAT_6bcabd6c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5f8;
  DAT_6bcabd70 = 100;
  DAT_6bcabd74 = 5;
  DAT_6bcabd78 = 
  "baseline update speed for proximity;\nthe bigger threshold is the slower to update proximity baseline;"
  ;
  DAT_6bcabd80 = (int)*(short *)&_refptr_g_tsaPrmtFlash[2].field_0x5fa;
  DAT_6bcabd84 = (int)*(short *)&_refptr_g_tsaPrmtFlash[2].field_0x5fa;
  DAT_6bcabd88 = 5000;
  DAT_6bcabd8c = 0x32;
  DAT_6bcabd90 = "baseline update threshold for proximity;";
  DAT_6bcabd98 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x5f0;
  DAT_6bcabd9c = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x5f0;
  DAT_6bcabda0 = 0x10;
  DAT_6bcabda4 = 8;
  DAT_6bcabda8 = 
  "ratio to proximity trigger threshold for min proximity lift off threshold;\nthe bigger threshold is the better to avoid trigger but the poorer for proximity debounce;"
  ;
  DAT_6bcabdb0 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x5f1;
  DAT_6bcabdb4 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x5f1;
  DAT_6bcabdb8 = 0x10;
  DAT_6bcabdbc = 8;
  DAT_6bcabdc0 = 
  "ratio to proximity trigger threshold for touch zone traversal threshold;\nthe bigger threshold is the better to avoid false trigger but the poorer to trigger proximity;"
  ;
  DAT_6bcabdc8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5fc;
  DAT_6bcabdcc = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5fc;
  DAT_6bcabdd0 = 0x78;
  DAT_6bcabdd4 = 0x3c;
  DAT_6bcabdd8 = 
  "scan rate in phone call mode with proximity off;\neither 120 or 60 is selectable to trade off power consumption and touch performance;"
  ;
  DAT_6bcabde0 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5fe;
  DAT_6bcabde4 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5fe;
  DAT_6bcabde8 = 0x78;
  DAT_6bcabdec = 0xf;
  DAT_6bcabdf0 = 
  "scan rate in phone call mode with proximity on;\navailable option [120, 60, 40, 30, 15] to trade off power consumption and proximity performance;"
  ;
  DAT_6bcabdf8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x600;
  DAT_6bcabdfc = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x600;
  DAT_6bcabe00 = 0x78;
  DAT_6bcabe04 = 0x3c;
  DAT_6bcabe08 = 
  "scan rate in screen lock mode with proximity off;\neither 120 or 60 is selectable to trade off power consumption and touch performance;"
  ;
  DAT_6bcabe10 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x602;
  DAT_6bcabe14 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x602;
  DAT_6bcabe18 = 0x78;
  DAT_6bcabe1c = 0xf;
  DAT_6bcabe20 = 
  "scan rate in screen lock mode with proximity on;\navailable option [120, 60, 40, 30, 15] to trade off power consumption and proximity performance;"
  ;
  DAT_6bcabe28 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x610;
  DAT_6bcabe2c = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x610;
  DAT_6bcabe30 = 0x7fffffff;
  DAT_6bcabe34 = 0;
  DAT_6bcabe38 = 
  "in phone call mode, if screen is turned off automatically by either pressing power key,\nor system over proxTimeAutoScreenOff ms since last touch event and without proximity detected,\nthe screen off event will be identify as a proximity event,\nso that algorithm can detect proximity off once phone is away from face;"
  ;
  DAT_6bcabe40 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x6a0;
  DAT_6bcabe44 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x6a0;
  DAT_6bcabe48 = 0x7fffffff;
  DAT_6bcabe4c = 0;
  DAT_6bcabe50 = "volume gesture set";
  DAT_6bcabe58 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x63c;
  DAT_6bcabe5c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x63c;
  DAT_6bcabe60 = 0x7fffffff;
  DAT_6bcabe64 = 0x80000000;
  DAT_6bcabe68 = "volumeFlickUp";
  DAT_6bcabe70 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x63e;
  DAT_6bcabe74 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x63e;
  DAT_6bcabe78 = 0x7fffffff;
  DAT_6bcabe7c = 0x80000000;
  DAT_6bcabe80 = "volumeFlickDown";
  DAT_6bcabe88 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x640;
  DAT_6bcabe8c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x640;
  DAT_6bcabe90 = 0x7fffffff;
  DAT_6bcabe94 = 0x80000000;
  DAT_6bcabe98 = "volumeStepUp";
  DAT_6bcabea0 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x642;
  DAT_6bcabea4 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x642;
  DAT_6bcabea8 = 0x7fffffff;
  DAT_6bcabeac = 0x80000000;
  DAT_6bcabeb0 = "volumeStepDown";
  DAT_6bcabeb8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x644;
  DAT_6bcabebc = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x644;
  DAT_6bcabec0 = 0x7fffffff;
  DAT_6bcabec4 = 0x80000000;
  DAT_6bcabec8 = "volumeActiveAreaStart";
  DAT_6bcabed0 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x646;
  DAT_6bcabed4 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x646;
  DAT_6bcabed8 = 0x7fffffff;
  DAT_6bcabedc = 0x80000000;
  DAT_6bcabee0 = "volumeActiveAreaEnd";
  DAT_6bcabee8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x648;
  DAT_6bcabeec = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x648;
  DAT_6bcabef0 = 0x7fffffff;
  DAT_6bcabef4 = 0x80000000;
  DAT_6bcabef8 = "volumeGreenAreaStart";
  DAT_6bcabf00 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64a;
  DAT_6bcabf04 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64a;
  DAT_6bcabf08 = 0x7fffffff;
  DAT_6bcabf0c = 0x80000000;
  DAT_6bcabf10 = "volumeGreenAreaEnd";
  DAT_6bcabf18 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64c;
  DAT_6bcabf1c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64c;
  DAT_6bcabf20 = 0x7fffffff;
  DAT_6bcabf24 = 0x80000000;
  DAT_6bcabf28 = "volumeSideEnabled";
  DAT_6bcabf30 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x69c;
  DAT_6bcabf34 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x69c;
  DAT_6bcabf38 = 0x7fffffff;
  DAT_6bcabf3c = 0x80000000;
  DAT_6bcabf40 = "volumeSensitivityLevel";
  DAT_6bcabf48 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x684;
  DAT_6bcabf4c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x684;
  DAT_6bcabf50 = 0x7fffffff;
  DAT_6bcabf54 = 0x80000000;
  DAT_6bcabf58 = "volumeActiveAreaStart2";
  DAT_6bcabf60 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x686;
  DAT_6bcabf64 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x686;
  DAT_6bcabf68 = 0x7fffffff;
  DAT_6bcabf6c = 0x80000000;
  DAT_6bcabf70 = "volumeActiveAreaEnd2";
  DAT_6bcabf78 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x688;
  DAT_6bcabf7c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x688;
  DAT_6bcabf80 = 0x7fffffff;
  DAT_6bcabf84 = 0x80000000;
  DAT_6bcabf88 = "volumeGreenAreaStart2";
  DAT_6bcabf90 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x68a;
  DAT_6bcabf94 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x68a;
  DAT_6bcabf98 = 0x7fffffff;
  DAT_6bcabf9c = 0x80000000;
  DAT_6bcabfa0 = "volumeGreenAreaEnd2";
  DAT_6bcabfa8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x68c;
  DAT_6bcabfac = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x68c;
  DAT_6bcabfb0 = 0x7fffffff;
  DAT_6bcabfb4 = 0x80000000;
  DAT_6bcabfb8 = "volumeSideEnabled2";
  DAT_6bcabfc0 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x68e;
  DAT_6bcabfc4 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x68e;
  DAT_6bcabfc8 = 0x7fffffff;
  DAT_6bcabfcc = 0x80000000;
  DAT_6bcabfd0 = "volumeActiveAreaStart3";
  DAT_6bcabfd8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x690;
  DAT_6bcabfdc = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x690;
  DAT_6bcabfe0 = 0x7fffffff;
  DAT_6bcabfe4 = 0x80000000;
  DAT_6bcabfe8 = "volumeActiveAreaEnd3";
  DAT_6bcabff0 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x692;
  DAT_6bcabff4 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x692;
  DAT_6bcabff8 = 0x7fffffff;
  DAT_6bcabffc = 0x80000000;
  DAT_6bcac000 = "volumeGreenAreaStart3";
  DAT_6bcac008 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x694;
  DAT_6bcac00c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x694;
  DAT_6bcac010 = 0x7fffffff;
  DAT_6bcac014 = 0x80000000;
  DAT_6bcac018 = "volumeGreenAreaEnd3";
  DAT_6bcac020 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x696;
  DAT_6bcac024 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x696;
  DAT_6bcac028 = 0x7fffffff;
  DAT_6bcac02c = 0x80000000;
  DAT_6bcac030 = "volumeGreenAreaToMid3";
  DAT_6bcac038 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x698;
  DAT_6bcac03c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x698;
  DAT_6bcac040 = 0x7fffffff;
  DAT_6bcac044 = 0x80000000;
  DAT_6bcac048 = "volumeSideEnabled3";
  DAT_6bcac050 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64e;
  DAT_6bcac054 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64e;
  DAT_6bcac058 = 0x7fffffff;
  DAT_6bcac05c = 0x80000000;
  DAT_6bcac060 = "volumeTouchMin";
  DAT_6bcac068 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x650;
  DAT_6bcac06c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x650;
  DAT_6bcac070 = 0x7fffffff;
  DAT_6bcac074 = 0x80000000;
  DAT_6bcac078 = "volumeTouchMax";
  DAT_6bcac080 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x652;
  DAT_6bcac084 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x652;
  DAT_6bcac088 = 0x7fffffff;
  DAT_6bcac08c = 0x80000000;
  DAT_6bcac090 = "volumeTimeFlick";
  DAT_6bcac098 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x654;
  DAT_6bcac09c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x654;
  DAT_6bcac0a0 = 0x7fffffff;
  DAT_6bcac0a4 = 0x80000000;
  DAT_6bcac0a8 = "volumeTimeOnMin";
  DAT_6bcac0b0 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x656;
  DAT_6bcac0b4 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x656;
  DAT_6bcac0b8 = 0x7fffffff;
  DAT_6bcac0bc = 0x80000000;
  DAT_6bcac0c0 = "volumeTimeOffMin";
  DAT_6bcac0c8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x658;
  DAT_6bcac0cc = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x658;
  DAT_6bcac0d0 = 0x7fffffff;
  DAT_6bcac0d4 = 0x80000000;
  DAT_6bcac0d8 = "volumeTimeOffMax";
  DAT_6bcac0e0 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x65a;
  DAT_6bcac0e4 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x65a;
  DAT_6bcac0e8 = 0x7fffffff;
  DAT_6bcac0ec = 0x80000000;
  DAT_6bcac0f0 = "volumeTimeGUI";
  DAT_6bcac0f8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x674;
  DAT_6bcac0fc = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x674;
  DAT_6bcac100 = 0x7fffffff;
  DAT_6bcac104 = 0x80000000;
  DAT_6bcac108 = "volumeTimeToAntiTrigger";
  DAT_6bcac110 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x65c;
  DAT_6bcac114 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x65c;
  DAT_6bcac118 = 0x7fffffff;
  DAT_6bcac11c = 0x80000000;
  DAT_6bcac120 = "volumeForceAreaStart";
  DAT_6bcac128 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x65e;
  DAT_6bcac12c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x65e;
  DAT_6bcac130 = 0x7fffffff;
  DAT_6bcac134 = 0x80000000;
  DAT_6bcac138 = "volumeForceAreaEnd";
  DAT_6bcac140 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x660;
  DAT_6bcac144 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x660;
  DAT_6bcac148 = 0x7fffffff;
  DAT_6bcac14c = 0x80000000;
  DAT_6bcac150 = "volumeForceAreaGap";
  DAT_6bcac158 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x662;
  DAT_6bcac15c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x662;
  DAT_6bcac160 = 0x7fffffff;
  DAT_6bcac164 = 0x80000000;
  DAT_6bcac168 = "volumeForceAreaGapCenter";
  DAT_6bcac170 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x664;
  DAT_6bcac174 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x664;
  DAT_6bcac178 = 0x7fffffff;
  DAT_6bcac17c = 0x80000000;
  DAT_6bcac180 = "volumeForceAreaDebounce";
  DAT_6bcac188 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x666;
  DAT_6bcac18c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x666;
  DAT_6bcac190 = 0x7fffffff;
  DAT_6bcac194 = 0x80000000;
  DAT_6bcac198 = "volumeForceMoveMax";
  DAT_6bcac1a0 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x668;
  DAT_6bcac1a4 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x668;
  DAT_6bcac1a8 = 0x7fffffff;
  DAT_6bcac1ac = 0x80000000;
  DAT_6bcac1b0 = "volumeForceTouchMax";
  DAT_6bcac1b8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x66a;
  DAT_6bcac1bc = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x66a;
  DAT_6bcac1c0 = 0x7fffffff;
  DAT_6bcac1c4 = 0x80000000;
  DAT_6bcac1c8 = "volumeForceSHBReportYPosDebounce";
  DAT_6bcac1d0 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x66c;
  DAT_6bcac1d4 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x66c;
  DAT_6bcac1d8 = 0x7fffffff;
  DAT_6bcac1dc = 0x80000000;
  DAT_6bcac1e0 = "volumeMappingTimeDelay";
  DAT_6bcac1e8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x66e;
  DAT_6bcac1ec = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x66e;
  DAT_6bcac1f0 = 0x7fffffff;
  DAT_6bcac1f4 = 0x80000000;
  DAT_6bcac1f8 = "volumeMuteMinMove";
  DAT_6bcac200 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x670;
  DAT_6bcac204 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x670;
  DAT_6bcac208 = 0x7fffffff;
  DAT_6bcac20c = 0x80000000;
  DAT_6bcac210 = "volumeMuteMaxTime";
  DAT_6bcac218 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x678;
  DAT_6bcac21c = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x678;
  DAT_6bcac220 = 0x7fffffff;
  DAT_6bcac224 = 0x80000000;
  DAT_6bcac228 = "volumeGUIOnTimeOut";
  DAT_6bcac230 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x67c;
  DAT_6bcac234 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x67c;
  DAT_6bcac238 = 0x7fffffff;
  DAT_6bcac23c = 0x80000000;
  DAT_6bcac240 = "volumeGUIOffTimeOut";
  DAT_6bcac248 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x680;
  DAT_6bcac24c = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x680;
  DAT_6bcac250 = 0x7fffffff;
  DAT_6bcac254 = 0x80000000;
  DAT_6bcac258 = "volumeKeyDebounceTime";
  DAT_6bcac260 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6a4;
  DAT_6bcac264 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6a4;
  DAT_6bcac268 = 0x7fffffff;
  DAT_6bcac26c = 0x80000000;
  DAT_6bcac270 = "powerAreaStart";
  DAT_6bcac278 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6a6;
  DAT_6bcac27c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6a6;
  DAT_6bcac280 = 0x7fffffff;
  DAT_6bcac284 = 0x80000000;
  DAT_6bcac288 = "powerAreaEnd";
  DAT_6bcac290 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6b4;
  DAT_6bcac294 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6b4;
  DAT_6bcac298 = 0x7fffffff;
  DAT_6bcac29c = 0x80000000;
  DAT_6bcac2a0 = "powerTouchMax";
  DAT_6bcac2a8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6a8;
  DAT_6bcac2ac = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6a8;
  DAT_6bcac2b0 = 0x7fffffff;
  DAT_6bcac2b4 = 0x80000000;
  DAT_6bcac2b8 = "powerMoveMax";
  DAT_6bcac2c0 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6aa;
  DAT_6bcac2c4 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6aa;
  DAT_6bcac2c8 = 0x7fffffff;
  DAT_6bcac2cc = 0x80000000;
  DAT_6bcac2d0 = "powerTimeOnMin";
  DAT_6bcac2d8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6ac;
  DAT_6bcac2dc = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6ac;
  DAT_6bcac2e0 = 0x7fffffff;
  DAT_6bcac2e4 = 0x80000000;
  DAT_6bcac2e8 = "powerTimeOnMax";
  DAT_6bcac2f0 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6ae;
  DAT_6bcac2f4 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6ae;
  DAT_6bcac2f8 = 0x7fffffff;
  DAT_6bcac2fc = 0x80000000;
  DAT_6bcac300 = "powerTimeOffMin";
  DAT_6bcac308 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6b0;
  DAT_6bcac30c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6b0;
  DAT_6bcac310 = 0x7fffffff;
  DAT_6bcac314 = 0x80000000;
  DAT_6bcac318 = "powerTimeOffMax";
  DAT_6bcac320 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x6b8;
  DAT_6bcac324 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x6b8;
  DAT_6bcac328 = 0x7fffffff;
  DAT_6bcac32c = 0;
  DAT_6bcac330 = "power gesture set";
  DAT_6bcac338 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x6bc;
  DAT_6bcac33c = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x6bc;
  DAT_6bcac340 = 0x7fffffff;
  DAT_6bcac344 = 0;
  DAT_6bcac348 = "motorVolumeOnWaveform";
  DAT_6bcac350 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x6bd;
  DAT_6bcac354 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x6bd;
  DAT_6bcac358 = 0x7fffffff;
  DAT_6bcac35c = 0;
  DAT_6bcac360 = "motorVolumeOffWaveform";
  DAT_6bcac368 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x628;
  DAT_6bcac36c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x628;
  DAT_6bcac370 = 0x7fffffff;
  DAT_6bcac374 = 0x80000000;
  DAT_6bcac378 = "sideTouchXCoreStart";
  DAT_6bcac380 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x62a;
  DAT_6bcac384 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x62a;
  DAT_6bcac388 = 0x7fffffff;
  DAT_6bcac38c = 0x80000000;
  DAT_6bcac390 = "sideTouchXCoreEnd";
  DAT_6bcac398 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x62c;
  DAT_6bcac39c = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x62c;
  DAT_6bcac3a0 = 0x7fffffff;
  DAT_6bcac3a4 = 0x80000000;
  DAT_6bcac3a8 = "sideTouchMinGripRatio";
  DAT_6bcac3b0 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x62d;
  DAT_6bcac3b4 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x62d;
  DAT_6bcac3b8 = 0x7fffffff;
  DAT_6bcac3bc = 0x80000000;
  DAT_6bcac3c0 = "sideTouchMidGripRatio";
  DAT_6bcac3c8 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x62e;
  DAT_6bcac3cc = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x62e;
  DAT_6bcac3d0 = 0x7fffffff;
  DAT_6bcac3d4 = 0x80000000;
  DAT_6bcac3d8 = "sideTouchMaxGripRatio";
  DAT_6bcac3e0 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x624;
  DAT_6bcac3e4 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x624;
  DAT_6bcac3e8 = 0x7fffffff;
  DAT_6bcac3ec = 0;
  DAT_6bcac3f0 = "side touch config";
  DAT_6bcac3f8 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x6c4;
  DAT_6bcac3fc = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x6c4;
  DAT_6bcac400 = 0x7fffffff;
  DAT_6bcac404 = 0x80000000;
  DAT_6bcac408 = "sensorEnabled";
  DAT_6bcac410 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x6c8;
  DAT_6bcac414 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x6c8;
  DAT_6bcac418 = 0x7fffffff;
  DAT_6bcac41c = 0x80000000;
  DAT_6bcac420 = "sensorRate";
  DAT_6bcac428 = (int)(char)_refptr_g_tsaPrmtFlash->field_0x5ae;
  DAT_6bcac42c = (int)(char)_refptr_g_tsaPrmtFlash->field_0x5ae;
  DAT_6bcac430 = 0x7f;
  DAT_6bcac434 = 0xffffff80;
  DAT_6bcac438 = "dim1ClipLow parameter";
  DAT_6bcac440 = (int)(char)_refptr_g_tsaPrmtFlash->field_0x5af;
  DAT_6bcac444 = (int)(char)_refptr_g_tsaPrmtFlash->field_0x5af;
  DAT_6bcac448 = 0x7f;
  DAT_6bcac44c = 0xffffff80;
  DAT_6bcac450 = "dim1ClipHigh parameter";
  DAT_6bcac458 = (int)(char)_refptr_g_tsaPrmtFlash->field_0x5b0;
  DAT_6bcac45c = (int)(char)_refptr_g_tsaPrmtFlash->field_0x5b0;
  DAT_6bcac460 = 0x7f;
  DAT_6bcac464 = 0xffffff80;
  DAT_6bcac468 = "dim2ClipLow parameter";
  DAT_6bcac470 = (int)(char)_refptr_g_tsaPrmtFlash->field_0x5b1;
  DAT_6bcac474 = (int)(char)_refptr_g_tsaPrmtFlash->field_0x5b1;
  DAT_6bcac478 = 0x7f;
  DAT_6bcac47c = 0xffffff80;
  DAT_6bcac480 = "dim2ClipHigh parameter";
  return;
}


// ============================================================================
// Function: PosnQueue_Init @ 6bb41e70
// ============================================================================

void PosnQueue_Init(undefined8 param_1,undefined1 param_2)

{
                    /* 0x101e70  1008  PosnQueue_Init */
  g_queue = param_1;
  DAT_6bbbf528 = param_2;
  DAT_6bbbf52a = 0;
  DAT_6bbbf529 = 0;
  return;
}


// ============================================================================
// Function: PosnSpecial_Init @ 6bb422b6
// ============================================================================

void PosnSpecial_Init(undefined8 param_1,undefined2 param_2)

{
  int iVar1;
  
                    /* 0x1022b6  1022  PosnSpecial_Init */
  iVar1 = memset_s(&g_list,0x98,0,0x98);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Tool/PosnList/PosnSpecial/PosnSpecial.c",0x53,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  DAT_6bcac4ca = 0;
  DAT_6bcac4ce = 0;
  DAT_6bcac4cf = 1;
  DAT_6bcac4d0 = 1;
  DAT_6bcac4d1 = 0;
  DAT_6bcac4d2 = 0;
  DAT_6bcac4dd = 0;
  g_list = param_1;
  DAT_6bcac4c8 = param_2;
  DAT_6bcac4cc = param_2;
  PosnSpecial_ResetZone(0);
  return;
}


// ============================================================================
// Function: BadChannel_Init @ 6bb42b6c
// ============================================================================

void BadChannel_Init(void)

{
  int iVar1;
  
                    /* 0x102b6c  383  BadChannel_Init */
  iVar1 = memset_s(g_badChannelDebounce,*(undefined2 *)_refptr_g_gridBufSize,0,
                   *(undefined2 *)_refptr_g_gridBufSize);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/WorkAround/BadChannel/BadChannel.c",0x35,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: BaliSensorBroken_Init @ 6bb43315
// ============================================================================

void BaliSensorBroken_Init(void)

{
  int iVar1;
  
                    /* 0x103315  388  BaliSensorBroken_Init */
  BaliSensorBroken_Reset();
  iVar1 = memset_s(g_badSensorDebounce,*(undefined2 *)_refptr_g_gridBufSize,0,
                   *(undefined2 *)_refptr_g_gridBufSize);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/WorkAround/BaliSensorBroken/BaliSensorBroken.c",0x39,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: RFNoise_Init @ 6bb444b6
// ============================================================================

void RFNoise_Init(void)

{
  int iVar1;
  
                    /* 0x1044b6  1146  RFNoise_Init */
  iVar1 = memset_s(g_tsaBufPrevRaw2rawDif,(ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,0,
                   (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/WorkAround/RFNoise/RFNoise.c",0x31,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(g_tsaBufRaw2rawDif2,(ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,0,
                   (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/WorkAround/RFNoise/RFNoise.c",0x33,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(g_tsaBufRaw2rawDif3,(ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,0,
                   (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/WorkAround/RFNoise/RFNoise.c",0x35,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  g_maxAbsCMFNoiseDim1 = 0;
  g_maxAbsCMFNoiseDim2 = 0;
  g_numCMNLineDim1 = 0;
  g_numCMNLineDim2 = 0;
  return;
}


// ============================================================================
// Function: InitializeCriticalSection @ 6bb4e6b8
// ============================================================================

void __stdcall InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection)

{
                    /* WARNING: Could not recover jumptable at 0x6bb4e6b8. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  InitializeCriticalSection(lpCriticalSection);
  return;
}


