// ============================================================================
// Function: HandGesture_Process @ 6baef9fc
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void HandGesture_Process(void)

{
                    /* 0xaf9fc  855  HandGesture_Process */
  (*_refptr_g_tsaStaticPtr)->dwProcessingState = (*_refptr_g_tsaStaticPtr)->dwProcessingState | 0x80
  ;
  DAT_6bc86ca4 = 0;
  _DAT_6bc86ca8 = _DAT_6bc86ca8 & 0xffffff;
  HandGesture_TimeStep();
  HandGesture_2HandDetection();
  HandGesture_GripDetection();
  handGesturePrev = CONCAT44(DAT_6bc86ca4,handGesture);
  DAT_6bc86ea8 = CONCAT44(_DAT_6bc86cac,_DAT_6bc86ca8);
  DAT_6bc86eb0 = _DAT_6bc86cb0;
  DAT_6bc86eb8 = DAT_6bc86cb8;
  DAT_6bc86ec0 = DAT_6bc86cc0;
  (*_refptr_g_tsaStaticPtr)->dwProcessingState =
       (*_refptr_g_tsaStaticPtr)->dwProcessingState & 0xffffff7f;
  return;
}


// ============================================================================
// Function: HandGesture_TimeStep @ 6baeea17
// ============================================================================

void HandGesture_TimeStep(void)

{
  short sVar1;
  int iVar2;
  uint local_14;
  short *local_10;
  
  sVar1 = (*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
  local_10 = g_tsaBufTimeHand;
  local_14 = (uint)_refptr_g_tsaPrmtFlash->wBufElementCount;
  while (local_14 != 0) {
    if (*local_10 != 0x7fff) {
      iVar2 = (int)sVar1 + (int)*local_10;
      if (iVar2 < 0x8000) {
        if (iVar2 < -0x8000) {
          *local_10 = -0x8000;
        }
        else {
          *local_10 = (short)iVar2;
        }
      }
      else {
        *local_10 = 0x7fff;
      }
    }
    local_10 = local_10 + 1;
    local_14 = local_14 - 1;
  }
  return;
}


// ============================================================================
// Function: HandGesture_2HandDetection @ 6baef641
// ============================================================================

void HandGesture_2HandDetection(void)

{
  bool bVar1;
  bool bVar2;
  ushort uVar3;
  byte local_b;
  
  bVar2 = false;
  bVar1 = 1 < (byte)(*_refptr_g_prevTouchesPtr)->field_0x6;
  for (local_b = 0; local_b < **(byte **)_refptr_g_touchesPtr; local_b = local_b + 1) {
    if ((*(uint *)((longlong)(int)(uint)local_b * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
        & 0x20000000) == 0) {
      if ((*(int *)((longlong)(int)(uint)local_b * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x210
                   ) == 2) ||
         (*(int *)((longlong)(int)(uint)local_b * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x210)
          == 1)) {
        bVar2 = true;
      }
    }
    else {
      uVar3 = Touch_GetSQDistMM(*(undefined2 *)
                                 ((longlong)(int)(uint)local_b * 0x168 +
                                  *(longlong *)_refptr_g_touchesPtr + 0x162),
                                *(undefined2 *)
                                 ((longlong)(int)(uint)local_b * 0x168 +
                                  *(longlong *)_refptr_g_touchesPtr + 0x164),
                                *(undefined2 *)&(*_refptr_g_tsaStaticPtr)->field_0x98,
                                *(undefined2 *)&(*_refptr_g_tsaStaticPtr)->field_0x9a);
      if (((ulonglong)
           ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
           *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x90) < 100) && (399 < uVar3)) {
        bVar1 = true;
      }
    }
  }
  if (bVar1) {
    handGesture = handGesture | 2;
    DAT_6bc86cc0 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    HandGesture_ResetRecord();
  }
  else if ((bVar2) &&
          (4000 < (ulonglong)
                  ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - (longlong)DAT_6bc86cc0)))
  {
    handGesture = handGesture & 0xfffffffd;
  }
  return;
}


// ============================================================================
// Function: HandGesture_GripDetection @ 6baef865
// ============================================================================

void HandGesture_GripDetection(void)

{
  char cVar1;
  int iVar2;
  
  iVar2 = memset_s(g_tsaBufTzHand,*(undefined2 *)_refptr_g_gridBufSize,0,
                   *(undefined2 *)_refptr_g_gridBufSize);
  if (iVar2 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/HandGesture/HandGesture.c",0x1fb,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  cVar1 = HandGesture_IsOKToResetBaseline();
  if (cVar1 != '\0') {
    handGesture = handGesture | 1;
    DAT_6bc86ca4 = DAT_6bc86ca4 | 1;
    DAT_6bc86cb8 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    HandGesture_BaselineReset();
  }
  DataSwitch_ToHand();
  cVar1 = HandGesture_IsOKToCalcDiff();
  if (cVar1 == '\0') {
    DAT_6bc86ca8 = 0x8000;
  }
  else {
    DAT_6bc86ca4 = DAT_6bc86ca4 | 2;
    HandGesture_DifProcess();
    cVar1 = HandGesture_IsOKToProcessTouch();
    if (cVar1 != '\0') {
      DAT_6bc86ca4 = DAT_6bc86ca4 | 4;
      handGesture = handGesture & 0xfffffffe;
      HandGesture_TouchProcess();
    }
  }
  if ((DAT_6bc86ca4 & 4) == 0) {
    Touch_Clean();
    Peak_Clean();
    PrevTouch_PostProcess();
    PrevPeak_Process();
  }
  else {
    HandGesture_PostProcess();
  }
  DataSwitch_ToGrid();
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
// Function: HandGesture_Is2HandDetected @ 6baee0ce
// ============================================================================

bool HandGesture_Is2HandDetected(void)

{
                    /* 0xae0ce  851  HandGesture_Is2HandDetected */
  return (handGesture & 2) != 0;
}


// ============================================================================
// Function: HandGesture_ResetRecord @ 6baede69
// ============================================================================

void HandGesture_ResetRecord(void)

{
  int iVar1;
  ushort local_a;
  
  iVar1 = memset_s(g_tsaBufTzPrevHand,*(undefined2 *)_refptr_g_gridBufSize,0,
                   *(undefined2 *)_refptr_g_gridBufSize);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/HandGesture/HandGesture.c",0x57,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  for (local_a = 0; local_a < _refptr_g_tsaPrmtFlash->wBufElementCount; local_a = local_a + 1) {
    *(undefined2 *)(g_tsaBufTimeHand + (ulonglong)local_a * 2) = 0x7fff;
  }
  return;
}


// ============================================================================
// Function: HandGesture_BaselineReset @ 6baeddbd
// ============================================================================

void HandGesture_BaselineReset(void)

{
  errno_t eVar1;
  
  eVar1 = memcpy_s(g_tsaBufHandGestureBL,(ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,
                   *(void **)_refptr_g_tsaBufRaw,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/HandGesture/HandGesture.c",0x4f,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  BLIIR_CalcDiffCommon(g_tsaBufHandGestureDif,*(short **)_refptr_g_tsaBufRaw,g_tsaBufHandGestureBL);
  return;
}


// ============================================================================
// Function: HandGesture_IsOKToResetBaseline @ 6baee815
// ============================================================================

undefined8 HandGesture_IsOKToResetBaseline(void)

{
  byte bVar1;
  byte local_9;
  
  local_9 = 0;
  while( true ) {
    if ((byte)*_refptr_g_touchesGrid <= local_9) {
      return 0;
    }
    bVar1 = *(byte *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x287);
    HandGesture_CheckRelatedGrip(local_9,bVar1);
    if ((*(int *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c)
         == 0x20) &&
       (*(char *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x5a5)
        != '\0')) break;
    local_9 = local_9 + 1;
  }
  DAT_6bc86cac = *(undefined2 *)
                  ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x5a0)
  ;
  DAT_6bc86cae = *(undefined2 *)
                  ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x5a2)
  ;
  DAT_6bc86cb0 = *(undefined1 *)
                  ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x5a4)
  ;
  return 1;
}


// ============================================================================
// Function: HandGesture_IsOKToCalcDiff @ 6baee96e
// ============================================================================

undefined8 HandGesture_IsOKToCalcDiff(void)

{
  undefined8 uVar1;
  
  if (((handGesture & 1) == 0) ||
     (999 < (ulonglong)((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - DAT_6bc86cb8))) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: HandGesture_PostProcess @ 6baeed20
// ============================================================================

void HandGesture_PostProcess(void)

{
  errno_t eVar1;
  
  HandGesture_CheckValidity();
  if (DAT_6bc86cab != '\0') {
    HandGesture_TZFlagProcess();
    eVar1 = memcpy_s(g_tsaBufTzPrevHand,(ulonglong)*(ushort *)_refptr_g_gridBufSize,g_tsaBufTzHand,
                     (ulonglong)*(ushort *)_refptr_g_gridBufSize);
    if (eVar1 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/HandGesture/HandGesture.c",0x143,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
    HandGesture_TimeRecord();
  }
  return;
}


// ============================================================================
// Function: HandGesture_DifProcess @ 6baeef00
// ============================================================================

void HandGesture_DifProcess(void)

{
  byte bVar1;
  int iVar2;
  byte local_a;
  byte local_9;
  
  bVar1 = HandGesture_GetWidth();
  iVar2 = memset_s(g_tsaBufHandGestureDif,(ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,0,
                   (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2);
  if (iVar2 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/HandGesture/HandGesture.c",0x164,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  DAT_6bc86ca8 = 0x8000;
  for (local_9 = 0; local_9 <= bVar1; local_9 = local_9 + 1) {
    for (local_a = 0; local_a < _refptr_g_tsaPrmtFlash->bRows; local_a = local_a + 1) {
      HandGesture_DifUnit(local_9,local_a);
    }
  }
  for (local_9 = (_refptr_g_tsaPrmtFlash->bCols - bVar1) - 1;
      (int)(uint)local_9 <= (int)(_refptr_g_tsaPrmtFlash->bCols - 1); local_9 = local_9 + 1) {
    for (local_a = 0; local_a < _refptr_g_tsaPrmtFlash->bRows; local_a = local_a + 1) {
      HandGesture_DifUnit(local_9,local_a);
    }
  }
  return;
}


// ============================================================================
// Function: HandGesture_IsOKToProcessTouch @ 6baef06a
// ============================================================================

undefined8 HandGesture_IsOKToProcessTouch(void)

{
  short sVar1;
  short sVar2;
  
  sVar1 = DAT_6bc86ca8;
  if ((DAT_6bc86ca8 < (short)DAT_6bc86ea8) && (sVar2 = HandGesture_GetPkThold(), sVar2 < sVar1)) {
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: HandGesture_TouchProcess @ 6baef30b
// ============================================================================

/* WARNING: Function: ___chkstk_ms replaced with injection: alloca_probe */

void HandGesture_TouchProcess(void)

{
  byte bVar1;
  uchar uVar2;
  TSAFlashPrmt *pTVar3;
  TSAPrpt *pTVar4;
  short sVar5;
  errno_t eVar6;
  undefined8 in_stack_ffffffffffffd938;
  undefined4 uVar7;
  undefined1 local_26b8 [9808];
  undefined8 local_68;
  undefined8 local_60;
  undefined8 local_58;
  undefined8 local_50;
  uint local_48;
  undefined8 local_38;
  undefined8 local_30;
  undefined2 local_28;
  errno_t local_20;
  uint local_1c;
  
  uVar7 = (undefined4)((ulonglong)in_stack_ffffffffffffd938 >> 0x20);
  local_38 = *(undefined8 *)&_refptr_g_tsaPrpt[1].field_0x14;
  local_30 = *(undefined8 *)&_refptr_g_tsaPrpt[1].field_0x1c;
  local_28 = *(undefined2 *)&_refptr_g_tsaPrpt[1].field_0x24;
  local_68 = *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x81c;
  local_60 = *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x824;
  local_58 = *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x82c;
  local_50 = *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x834;
  local_48 = _refptr_g_tsaPrmtFlash->dwGripFlags;
  local_1c = (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags;
  local_20 = 0x6baef318;
  memset(local_26b8,0,0x2648);
  local_20 = memcpy_s(local_26b8,0x2648,*(void **)_refptr_g_tsaBufDif,
                      (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  if (local_20 != 0) {
    uVar7 = 0;
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/HandGesture/HandGesture.c",0x1a5,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  HandGesture_CopyDiff();
  TSAPrmt_Process();
  Touch_Clean();
  bVar1 = _refptr_g_tsaPrmtFlash->bCols;
  uVar2 = _refptr_g_tsaPrmtFlash->bRows;
  sVar5 = HandGesture_GetPkThold();
  Peak_DetectInRange((int)sVar5,0,uVar2 + 0xff,0,CONCAT44(uVar7,bVar1 - 1) & 0xffffffff000000ff);
  Peak_SortInSigAscending();
  *(short *)&_refptr_g_tsaPrmtFlash->field_0x81c = *(short *)(_refptr_g_tsaPrmtDynamic + 8) / 2;
  _refptr_g_tsaPrmtFlash->field_0x81e = _refptr_g_tsaPrmtFlash->field_0x820;
  _refptr_g_tsaPrmtFlash->field_0x81f = _refptr_g_tsaPrmtFlash->field_0x820;
  _refptr_g_tsaPrmtFlash->dwGripFlags = 0x17f;
  TZ_Process();
  CTD_ECProcess();
  IDT_Process('\0');
  Touch_AssignPreIdxBasedOnID();
  PrevTouch_AssignCurIdxBasedOnID();
  TE_Process();
  PrevTouch_PostProcess();
  PrevPeak_Process();
  TSAIDE_LogPreFltHandRpt();
  (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags = local_1c;
  eVar6 = memcpy_s(*(void **)_refptr_g_tsaBufDif,0x2648,local_26b8,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  _local_20 = CONCAT44(local_1c,eVar6);
  if (eVar6 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/HandGesture/HandGesture.c",0x1cf,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  pTVar3 = _refptr_g_tsaPrmtFlash;
  *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x81c = local_68;
  *(undefined8 *)&pTVar3->field_0x824 = local_60;
  *(undefined8 *)&pTVar3->field_0x82c = local_58;
  *(undefined8 *)&pTVar3->field_0x834 = local_50;
  pTVar3->dwGripFlags = local_48;
  pTVar4 = _refptr_g_tsaPrpt;
  *(undefined8 *)&_refptr_g_tsaPrpt[1].field_0x14 = local_38;
  *(undefined8 *)&pTVar4[1].field_0x1c = local_30;
  *(undefined2 *)&pTVar4[1].field_0x24 = local_28;
  return;
}


// ============================================================================
// Function: HandGesture_CheckRelatedGrip @ 6baee667
// ============================================================================

void HandGesture_CheckRelatedGrip(byte param_1,byte param_2)

{
  char cVar1;
  
  if (*(int *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x1d4) *
      3 < *(int *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1cc)
          * 4) {
    cVar1 = HandGesture_IsRelatedGripDetected(param_1,param_2);
    if (cVar1 == '\0') {
      *(undefined1 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5a5) = 0;
    }
    else {
      *(undefined1 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5a5) = 1;
      *(undefined2 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5a0) =
           *(undefined2 *)
            ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162);
      *(undefined2 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5a2) =
           *(undefined2 *)
            ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164);
      *(undefined1 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x5a4) =
           *(undefined1 *)
            ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x192);
    }
  }
  return;
}


// ============================================================================
// Function: HandGesture_TimeRecord @ 6baee9ba
// ============================================================================

void HandGesture_TimeRecord(void)

{
  ushort local_a;
  
  for (local_a = 0; local_a < _refptr_g_tsaPrmtFlash->wBufElementCount; local_a = local_a + 1) {
    if (*(char *)((ulonglong)local_a + g_tsaBufTzHand) != '\0') {
      *(undefined2 *)(g_tsaBufTimeHand + (ulonglong)local_a * 2) = 0;
    }
  }
  return;
}


// ============================================================================
// Function: HandGesture_CheckValidity @ 6baeec0f
// ============================================================================

void HandGesture_CheckValidity(void)

{
  char cVar1;
  
  DAT_6bc86cab = 1;
  if (**(char **)_refptr_g_touchesPtr == '\x01') {
    cVar1 = HandGesture_IsAngleValid();
    if (cVar1 == '\0') {
      DAT_6bc86cab = 0;
    }
    else {
      cVar1 = HandGesture_IsDistInValid
                        (*(undefined2 *)(*(longlong *)_refptr_g_touchesPtr + 0x162),
                         *(undefined2 *)(*(longlong *)_refptr_g_touchesPtr + 0x164),DAT_6bc86cac,
                         DAT_6bc86cae);
      if (cVar1 != '\0') {
        DAT_6bc86cab = 0;
      }
    }
  }
  else {
    DAT_6bc86cab = 0;
  }
  return;
}


// ============================================================================
// Function: HandGesture_TZFlagProcess @ 6baeecc0
// ============================================================================

void HandGesture_TZFlagProcess(void)

{
  ushort local_a;
  
  for (local_a = 0; local_a < _refptr_g_tsaPrmtFlash->wBufElementCount; local_a = local_a + 1) {
    if (*(short *)(g_tsaBufTimeRpt + (ulonglong)local_a * 2) < 5000) {
      *(undefined1 *)((ulonglong)local_a + g_tsaBufTzHand) = 0;
    }
  }
  return;
}


// ============================================================================
// Function: HandGesture_GetWidth @ 6baeedbc
// ============================================================================

undefined8 HandGesture_GetWidth(void)

{
  undefined8 uVar1;
  
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) == 0) {
    uVar1 = 2;
  }
  else {
    uVar1 = 3;
  }
  return uVar1;
}


// ============================================================================
// Function: HandGesture_DifUnit @ 6baeede7
// ============================================================================

void HandGesture_DifUnit(byte param_1,byte param_2)

{
  ushort uVar1;
  short sVar2;
  
  if ('\0' < *(char *)((ulonglong)
                       ((uint)param_1 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff
                       ) + g_tsaBufAreaHand)) {
    uVar1 = (ushort)param_1 + (ushort)param_2 * (ushort)_refptr_g_tsaPrmtFlash->bCols;
    if ((*_refptr_g_tsaStaticPtr)->bField_0x11a == '\0') {
      sVar2 = *(short *)(*(longlong *)_refptr_g_tsaBufRaw + (ulonglong)uVar1 * 2) -
              *(short *)(g_tsaBufHandGestureBL + (ulonglong)uVar1 * 2);
    }
    else {
      sVar2 = *(short *)(g_tsaBufHandGestureBL + (ulonglong)uVar1 * 2) -
              *(short *)(*(longlong *)_refptr_g_tsaBufRaw + (ulonglong)uVar1 * 2);
    }
    *(short *)((ulonglong)uVar1 * 2 + g_tsaBufHandGestureDif) = sVar2;
    if (DAT_6bc86ca8 < sVar2) {
      DAT_6bc86ca8 = sVar2;
    }
  }
  return;
}


// ============================================================================
// Function: HandGesture_GetPkThold @ 6baef045
// ============================================================================

int HandGesture_GetPkThold(void)

{
  int iVar1;
  
  iVar1 = *(short *)(_refptr_g_tsaPrmtDynamic + 8) * 3;
  if (iVar1 < 0) {
    iVar1 = iVar1 + 3;
  }
  return iVar1 >> 2;
}


// ============================================================================
// Function: HandGesture_CopyDiff @ 6baef0bb
// ============================================================================

void HandGesture_CopyDiff(void)

{
  byte bVar1;
  int iVar2;
  byte local_a;
  byte local_9;
  
  iVar2 = memset_s(*(undefined8 *)_refptr_g_tsaBufDif,0x2648,0,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  if (iVar2 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/HandGesture/HandGesture.c",0x187,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  bVar1 = HandGesture_GetWidth();
  for (local_9 = 0; local_9 <= bVar1; local_9 = local_9 + 1) {
    for (local_a = 0; local_a < _refptr_g_tsaPrmtFlash->bRows; local_a = local_a + 1) {
      if ('\0' < *(char *)((ulonglong)
                           ((uint)local_9 + (uint)local_a * (uint)_refptr_g_tsaPrmtFlash->bCols &
                           0xffff) + g_tsaBufAreaHand)) {
        *(undefined2 *)
         (*(longlong *)_refptr_g_tsaBufDif +
         (ulonglong)((uint)local_9 + (uint)local_a * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) *
         2) = *(undefined2 *)
               ((ulonglong)
                ((uint)local_9 + (uint)local_a * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
               g_tsaBufHandGestureDif);
      }
    }
  }
  for (local_9 = (_refptr_g_tsaPrmtFlash->bCols - bVar1) - 1;
      (int)(uint)local_9 <= (int)(_refptr_g_tsaPrmtFlash->bCols - 1); local_9 = local_9 + 1) {
    for (local_a = 0; local_a < _refptr_g_tsaPrmtFlash->bRows; local_a = local_a + 1) {
      if ('\0' < *(char *)((ulonglong)
                           ((uint)local_9 + (uint)local_a * (uint)_refptr_g_tsaPrmtFlash->bCols &
                           0xffff) + g_tsaBufAreaHand)) {
        *(undefined2 *)
         (*(longlong *)_refptr_g_tsaBufDif +
         (ulonglong)((uint)local_9 + (uint)local_a * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) *
         2) = *(undefined2 *)
               ((ulonglong)
                ((uint)local_9 + (uint)local_a * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
               g_tsaBufHandGestureDif);
      }
    }
  }
  return;
}


// ============================================================================
// Function: HandGesture_GetHandGestureBL @ 6baef9e2
// ============================================================================

undefined8 HandGesture_GetHandGestureBL(void)

{
                    /* 0xaf9e2  848  HandGesture_GetHandGestureBL */
  return g_tsaBufHandGestureBL;
}


// ============================================================================
// Function: HandGesture_GetHandGestureDif @ 6baef9ef
// ============================================================================

undefined8 HandGesture_GetHandGestureDif(void)

{
                    /* 0xaf9ef  849  HandGesture_GetHandGestureDif */
  return g_tsaBufHandGestureDif;
}


// ============================================================================
// Function: HandGesture_IsRelatedGripDetected @ 6baee4eb
// ============================================================================

undefined8 HandGesture_IsRelatedGripDetected(byte param_1,byte param_2)

{
  ushort uVar1;
  char cVar2;
  byte local_9;
  
  if ((((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_2 * 0x478 + 0x1f8) & 0x20000000) != 0) &&
      (uVar1 = *(ushort *)
                ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x18a),
      (uint)uVar1 * 4 + (uint)uVar1 <=
      (uint)*(ushort *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x188) * 4)
      ) && (7 < *(ushort *)
                 ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x18a))
     ) {
    for (local_9 = 0; local_9 < (*_refptr_g_prevTouchesPtr)->bPrevTouchCount; local_9 = local_9 + 1)
    {
      if ((*(char *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x154) !=
           *(char *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)local_9 * 0x478 + 0x154)) &&
         (cVar2 = HandGesture_IsGripRelated(param_1,local_9), cVar2 != '\0')) {
        HandGesture_SetHandArea(local_9);
        return 1;
      }
    }
  }
  return 0;
}


// ============================================================================
// Function: HandGesture_IsDistInValid @ 6baee0e5
// ============================================================================

undefined8
HandGesture_IsDistInValid
          (undefined2 param_1,undefined2 param_2,undefined2 param_3,undefined2 param_4)

{
  ushort uVar1;
  undefined8 uVar2;
  
  uVar1 = Touch_GetSQDistMM(param_1,param_2,param_3,param_4);
  if ((uVar1 < 400) || (0x1900 < uVar1)) {
    uVar2 = 1;
  }
  else {
    uVar2 = 0;
  }
  return uVar2;
}


// ============================================================================
// Function: HandGesture_IsAngleValid @ 6baeeb8b
// ============================================================================

void HandGesture_IsAngleValid(void)

{
  short sVar1;
  
  sVar1 = TSAMath_ATan2((int)(short)(*(short *)(*(longlong *)_refptr_g_touchesPtr + 0x162) -
                                    DAT_6bc86cac),
                        (int)(short)(*(short *)(*(longlong *)_refptr_g_touchesPtr + 0x164) -
                                    DAT_6bc86cae));
  HandGesture_IsAngleCorelated((int)-sVar1,(int)DAT_6bc86cb0);
  return;
}


// ============================================================================
// Function: HandGesture_SetHandArea @ 6baee147
// ============================================================================

void HandGesture_SetHandArea(byte param_1)

{
  char cVar1;
  int iVar2;
  ushort local_a;
  
  iVar2 = memset_s(g_tsaBufAreaHand,*(undefined2 *)_refptr_g_gridBufSize,0,
                   *(undefined2 *)_refptr_g_gridBufSize);
  if (iVar2 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/HandGesture/HandGesture.c",0x85,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  cVar1 = *(char *)((longlong)
                    (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                        (longlong)(int)(uint)param_1 * 0x478 + 0x4f2) * 0x168 +
                    *(longlong *)_refptr_g_touchesPtr + 0x28c);
  for (local_a = 0; local_a < _refptr_g_tsaPrmtFlash->wBufElementCount; local_a = local_a + 1) {
    if (*(char *)((ulonglong)local_a + *(longlong *)_refptr_g_tsaBufTzZone) == cVar1) {
      *(undefined1 *)((ulonglong)local_a + g_tsaBufAreaHand) = 1;
    }
  }
  return;
}


// ============================================================================
// Function: HandGesture_IsGripRelated @ 6baee25c
// ============================================================================

undefined8 HandGesture_IsGripRelated(byte param_1,byte param_2)

{
  char cVar1;
  char cVar2;
  short sVar3;
  
  sVar3 = TSAMath_ATan2((int)(short)(*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                                               (longlong)(int)(uint)param_2 * 0x478 + 0x162) -
                                    *(short *)((longlong)(int)(uint)param_1 * 0x168 +
                                               *(longlong *)_refptr_g_touchesPtr + 0x162)),
                        (int)(short)(*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                                               (longlong)(int)(uint)param_2 * 0x478 + 0x164) -
                                    *(short *)((longlong)(int)(uint)param_1 * 0x168 +
                                               *(longlong *)_refptr_g_touchesPtr + 0x164)));
  cVar1 = HandGesture_IsAngleCorelated
                    ((int)-sVar3,
                     (int)*(char *)((longlong)(int)(uint)param_1 * 0x168 +
                                    *(longlong *)_refptr_g_touchesPtr + 0x192));
  cVar2 = HandGesture_IsDistInValid
                    (*(undefined2 *)
                      ((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_2 * 0x478 + 0x162),
                     *(undefined2 *)
                      ((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_2 * 0x478 + 0x164),
                     *(undefined2 *)
                      ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x162),*(undefined2 *)
                              ((longlong)(int)(uint)param_1 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x164));
  if ((cVar1 != '\0') && (cVar2 == '\0')) {
    *(uint *)((longlong)
              (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                  (longlong)(int)(uint)param_2 * 0x478 + 0x4f2) * 0x168 +
              *(longlong *)_refptr_g_touchesPtr + 0x214) =
         *(uint *)((longlong)
                   (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                       (longlong)(int)(uint)param_2 * 0x478 + 0x4f2) * 0x168 +
                   *(longlong *)_refptr_g_touchesPtr + 0x214) | 0x8000000;
    cVar1 = GripFilter_IsGrip(*(undefined4 *)
                               ((longlong)*_refptr_g_prevTouchesPtr +
                               (longlong)(int)(uint)param_2 * 0x478 + 0x210));
    if (cVar1 != '\0') {
      return 1;
    }
  }
  return 0;
}


// ============================================================================
// Function: HandGesture_IsAngleCorelated @ 6baeeb33
// ============================================================================

bool HandGesture_IsAngleCorelated(short param_1,short param_2)

{
  short sVar1;
  short sVar2;
  uint uVar3;
  
                    /* 0xaeb33  852  HandGesture_IsAngleCorelated */
  sVar1 = HandGesture_AngleFormat1((int)param_1);
  sVar2 = HandGesture_AngleFormat2((int)param_2);
  uVar3 = (int)sVar1 - (int)sVar2 >> 0x1f;
  return (int)(((int)sVar1 - (int)sVar2 ^ uVar3) - uVar3) < 0x1f;
}


// ============================================================================
// Function: HandGesture_AngleFormat1 @ 6baeeabd
// ============================================================================

short HandGesture_AngleFormat1(short param_1)

{
  short local_res8;
  
  if (param_1 < 0x5b) {
    local_res8 = param_1;
    if (param_1 < -0x5a) {
      local_res8 = param_1 + 0xb4;
    }
  }
  else {
    local_res8 = param_1 + -0xb4;
  }
  return local_res8;
}


// ============================================================================
// Function: HandGesture_AngleFormat2 @ 6baeeaf5
// ============================================================================

void HandGesture_AngleFormat2(short param_1)

{
  short local_res8;
  
  if (param_1 < 1) {
    local_res8 = param_1 + 0x5a;
  }
  else {
    local_res8 = param_1 + -0x5a;
  }
  HandGesture_AngleFormat1((int)local_res8);
  return;
}


