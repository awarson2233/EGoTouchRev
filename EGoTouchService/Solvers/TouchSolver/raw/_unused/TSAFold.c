// ============================================================================
// Function: TSAFold_InputProcess @ 6bb26ef0
// ============================================================================

void TSAFold_InputProcess(undefined8 param_1,uint *param_2,longlong param_3)

{
  char cVar1;
  longlong lVar2;
  
                    /* 0xe6ef0  1517  TSAFold_InputProcess */
  cVar1 = TSAFold_IsSupported();
  if (cVar1 != '\0') {
    lVar2 = AFE_GetFrame();
    if (*(int *)(lVar2 + 0x58) != *(int *)&(*_refptr_g_tsaStaticPtr)->field_0x2d8) {
      TSA_ReloadASAPrmtByScanState(*(undefined4 *)(lVar2 + 0x58));
    }
    *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x2d8 = *(undefined4 *)(lVar2 + 0x58);
    cVar1 = TSAFold_IsValid((*_refptr_g_tsaStaticPtr)->nField_0x268);
    if (cVar1 != '\0') {
      g_data = (*_refptr_g_tsaStaticPtr)->nField_0x268;
    }
    DAT_6bca1de8 = &_refptr_g_tsaPrmtFlash[2].field_0x750 + (ulonglong)g_data * 0x14;
    DAT_6bca1df0 = &_refptr_g_tsaPrmtFlash[2].field_0x750 + (ulonglong)DAT_6bca1de4 * 0x14;
    cVar1 = TSAFold_IsRegionCut();
    if (cVar1 != '\0') {
      TSAFold_InputZoomIn(param_1);
    }
    cVar1 = TSAFold_IsRegionChanged();
    if (cVar1 != '\0') {
      if ((g_data == 0) && (60000 < (ulonglong)(param_3 - (longlong)DAT_6bca1df8))) {
        *param_2 = *param_2 | 4;
      }
      if (DAT_6bca1de4 == 0) {
        DAT_6bca1df8 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
      }
      DAT_6bca1e00 = param_3;
      TSAFold_PrmtProcess();
      CMF_Init(1);
    }
  }
  return;
}


// ============================================================================
// Function: TSAFold_OutputProcess @ 6bb270ab
// ============================================================================

void TSAFold_OutputProcess(void)

{
  char cVar1;
  
                    /* 0xe70ab  1528  TSAFold_OutputProcess */
  cVar1 = TSAFold_IsSupported();
  if (cVar1 != '\0') {
    DAT_6bca1de4 = g_data;
  }
  return;
}


// ============================================================================
// Function: TSAFold_IsSupported @ 6bb26806
// ============================================================================

bool TSAFold_IsSupported(void)

{
                    /* 0xe6806  1521  TSAFold_IsSupported */
  return 1 < (byte)_refptr_g_tsaPrmtFlash[2].field_0x74c;
}


// ============================================================================
// Function: TSAFold_IsValid @ 6bb2682a
// ============================================================================

undefined8 TSAFold_IsValid(uint param_1)

{
  undefined8 uVar1;
  
                    /* 0xe682a  1522  TSAFold_IsValid */
  if (param_1 < (byte)_refptr_g_tsaPrmtFlash[2].field_0x74c) {
    if (((byte)_refptr_g_tsaPrmtFlash->field_0x1f <
         (byte)(&_refptr_g_tsaPrmtFlash[2].field_0x758)[(ulonglong)param_1 * 0x14]) ||
       ((byte)_refptr_g_tsaPrmtFlash->field_0x20 <
        (byte)(&_refptr_g_tsaPrmtFlash[2].field_0x759)[(ulonglong)param_1 * 0x14])) {
      uVar1 = 0;
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
// Function: TSAFold_IsRegionCut @ 6bb268c4
// ============================================================================

undefined8 TSAFold_IsRegionCut(void)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0xe68c4  1520  TSAFold_IsRegionCut */
  cVar1 = TSAFold_IsSupported();
  if (cVar1 == '\0') {
    uVar2 = 0;
  }
  else if (((byte)(&_refptr_g_tsaPrmtFlash[2].field_0x758)[(ulonglong)g_data * 0x14] <
            (byte)_refptr_g_tsaPrmtFlash->field_0x1f) ||
          ((byte)(&_refptr_g_tsaPrmtFlash[2].field_0x759)[(ulonglong)g_data * 0x14] <
           (byte)_refptr_g_tsaPrmtFlash->field_0x20)) {
    uVar2 = 1;
  }
  else {
    uVar2 = 0;
  }
  return uVar2;
}


// ============================================================================
// Function: TSAFold_IsRegionChanged @ 6bb2696b
// ============================================================================

undefined8 TSAFold_IsRegionChanged(void)

{
  undefined8 uVar1;
  
                    /* 0xe696b  1519  TSAFold_IsRegionChanged */
  if ((&_refptr_g_tsaPrmtFlash[2].field_0x758)[(ulonglong)g_data * 0x14] ==
      (&_refptr_g_tsaPrmtFlash[2].field_0x758)[(ulonglong)DAT_6bca1de4 * 0x14]) {
    if ((&_refptr_g_tsaPrmtFlash[2].field_0x75a)[(ulonglong)g_data * 0x14] ==
        (&_refptr_g_tsaPrmtFlash[2].field_0x75a)[(ulonglong)DAT_6bca1de4 * 0x14]) {
      if ((&_refptr_g_tsaPrmtFlash[2].field_0x759)[(ulonglong)g_data * 0x14] ==
          (&_refptr_g_tsaPrmtFlash[2].field_0x759)[(ulonglong)DAT_6bca1de4 * 0x14]) {
        if ((&_refptr_g_tsaPrmtFlash[2].field_0x75b)[(ulonglong)g_data * 0x14] ==
            (&_refptr_g_tsaPrmtFlash[2].field_0x75b)[(ulonglong)DAT_6bca1de4 * 0x14]) {
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
  return uVar1;
}


// ============================================================================
// Function: TSAFold_PrmtProcess @ 6bb275a6
// ============================================================================

void TSAFold_PrmtProcess(void)

{
                    /* 0xe75a6  1530  TSAFold_PrmtProcess */
  if (*(int *)_refptr_g_data == 0) {
    memcpy(_refptr_g_tsaPrmtFlash,&g_tsaPrmtFlashWhole,0x1890);
  }
  else {
    TSAFold_LogicPrmtProcess();
    TSAFold_PhysicalPrmtProcess();
  }
  TSAPrmt_ReInit();
  return;
}


// ============================================================================
// Function: TSAFold_InputZoomIn @ 6bb285a4
// ============================================================================

void TSAFold_InputZoomIn(longlong param_1)

{
  longlong lVar1;
  
                    /* 0xe85a4  1518  TSAFold_InputZoomIn */
  if (param_1 != 0) {
    TSAFold_GridZoomInRemap
              (param_1,param_1,2,*(uint *)&_refptr_g_tsaPrmtFlash->field_0x5fc & 0xffff);
  }
  lVar1 = AFE_GetFrame();
  if ((lVar1 != 0) && (*(longlong *)(lVar1 + 0x18) != 0)) {
    TSAFold_LineZoomInRemap
              (*(undefined8 *)(lVar1 + 0x18),*(undefined8 *)(lVar1 + 0x18),2,
               *(uint *)&_refptr_g_tsaPrmtFlash->field_0x5fc & 0xffff);
  }
  return;
}


// ============================================================================
// Function: TSAFold_PrmtSetRawWithTouch @ 6bb2758d
// ============================================================================

void TSAFold_PrmtSetRawWithTouch(undefined4 param_1)

{
                    /* 0xe758d  1534  TSAFold_PrmtSetRawWithTouch */
  DAT_6bca26c0 = param_1;
  return;
}


// ============================================================================
// Function: TSAFold_GridZoomInRemap @ 6bb2828f
// ============================================================================

undefined *
TSAFold_GridZoomInRemap(undefined *param_1,undefined *param_2,undefined1 param_3,undefined2 param_4)

{
  char cVar1;
  undefined *local_res8;
  
                    /* 0xe828f  1513  TSAFold_GridZoomInRemap */
  cVar1 = TSAFold_IsSupported();
  if (cVar1 != '\0') {
    local_res8 = param_1;
    if (param_1 == (undefined *)0x0) {
      local_res8 = _refptr_g_tsaBufFoldGrid;
    }
    cVar1 = TSAFold_IsRegionCut();
    if (cVar1 != '\0') {
      TSAFold_RemapGridToRegion
                (*(undefined8 *)(_refptr_g_data + 8),local_res8,param_2,param_3,param_4);
      param_2 = local_res8;
    }
  }
  return param_2;
}


// ============================================================================
// Function: TSAFold_GetCurrState @ 6bb267e6
// ============================================================================

undefined4 TSAFold_GetCurrState(void)

{
                    /* 0xe67e6  1511  TSAFold_GetCurrState */
  return g_data;
}


// ============================================================================
// Function: TSAFold_PhysicalPrmtProcess @ 6bb272f3
// ============================================================================

void TSAFold_PhysicalPrmtProcess(void)

{
                    /* 0xe72f3  1529  TSAFold_PhysicalPrmtProcess */
  TSAFold_NotchPrmtProcess();
  TSAFold_CMFPrmtProcess();
  return;
}


// ============================================================================
// Function: TSAFold_LogicPrmtProcess @ 6bb274e0
// ============================================================================

void TSAFold_LogicPrmtProcess(void)

{
                    /* 0xe74e0  1525  TSAFold_LogicPrmtProcess */
  TSAFold_CentroidPrmtProcess();
  TSAFold_GripPrmtProcess();
  TSAFold_EdgeReachPrmtProcess();
  TSAFold_LogicRegionPrmtProcess();
  return;
}


// ============================================================================
// Function: TSAFold_LineZoomInRemap @ 6bb283a6
// ============================================================================

void TSAFold_LineZoomInRemap
               (undefined *param_1,undefined8 param_2,undefined1 param_3,undefined2 param_4)

{
  char cVar1;
  undefined *local_res8;
  
                    /* 0xe83a6  1523  TSAFold_LineZoomInRemap */
  cVar1 = TSAFold_IsSupported();
  if (cVar1 != '\0') {
    local_res8 = param_1;
    if (param_1 == (undefined *)0x0) {
      local_res8 = _refptr_g_tsaBufFoldLine;
    }
    cVar1 = TSAFold_IsRegionCut();
    if (cVar1 != '\0') {
      TSAFold_RemapLineToRegion
                (*(undefined8 *)(_refptr_g_data + 8),local_res8,param_2,param_3,param_4);
    }
  }
  return;
}


// ============================================================================
// Function: TSAFold_RemapGridToRegion @ 6bb27719
// ============================================================================

void TSAFold_RemapGridToRegion
               (longlong param_1,undefined8 param_2,undefined8 param_3,undefined1 param_4,
               short param_5)

{
  byte bVar1;
  byte bVar2;
  short local_10;
  ushort local_e;
  ushort local_c;
  ushort local_a;
  
                    /* 0xe7719  1536  TSAFold_RemapGridToRegion */
  bVar1 = *(byte *)(param_1 + 8);
  bVar2 = *(byte *)(param_1 + 9);
  local_e = 0;
  local_10 = (ushort)*(byte *)(param_1 + 10) +
             (ushort)*(byte *)(param_1 + 0xb) * (ushort)(byte)_refptr_g_tsaPrmtFlash->field_0x1f;
  for (local_c = 0; local_c < bVar2; local_c = local_c + 1) {
    for (local_a = 0; local_a < bVar1; local_a = local_a + 1) {
      TSAFold_CopyUnit(param_3,local_a + local_10,param_2,local_e,param_4);
      local_e = local_e + 1;
    }
    local_10 = local_10 + (ushort)(byte)_refptr_g_tsaPrmtFlash->field_0x1f;
  }
  if (param_5 != -0x2153) {
    bVar1 = _refptr_g_tsaPrmtFlash->field_0x1f;
    bVar2 = _refptr_g_tsaPrmtFlash->field_0x20;
    for (; local_e < (ushort)((ushort)bVar2 * (ushort)bVar1); local_e = local_e + 1) {
      TSAFold_SetUnit(param_2,local_e,param_4,param_5);
    }
  }
  return;
}


// ============================================================================
// Function: TSAFold_CopyRawRegionToWhole @ 6bb27872
// ============================================================================

void TSAFold_CopyRawRegionToWhole(undefined8 param_1,longlong param_2,undefined8 param_3)

{
  byte bVar1;
  byte bVar2;
  short local_10;
  short local_e;
  ushort local_c;
  ushort local_a;
  
                    /* 0xe7872  1506  TSAFold_CopyRawRegionToWhole */
  bVar1 = *(byte *)(param_2 + 8);
  bVar2 = *(byte *)(param_2 + 9);
  local_e = 0;
  local_10 = (ushort)*(byte *)(param_2 + 10) +
             (ushort)*(byte *)(param_2 + 0xb) * (ushort)(byte)_refptr_g_tsaPrmtFlash->field_0x1f;
  for (local_c = 0; local_c < bVar2; local_c = local_c + 1) {
    for (local_a = 0; local_a < bVar1; local_a = local_a + 1) {
      TSAFold_CopyUnit(param_3,local_e,param_1,local_a + local_10,2);
      local_e = local_e + 1;
    }
    local_10 = local_10 + (ushort)(byte)_refptr_g_tsaPrmtFlash->field_0x1f;
  }
  return;
}


// ============================================================================
// Function: TSAFold_NotchPrmtProcess @ 6bb270e0
// ============================================================================

void TSAFold_NotchPrmtProcess(void)

{
  char cVar1;
  undefined1 local_26;
  undefined1 local_25;
  errno_t local_24;
  void *local_20;
  undefined *local_18;
  char local_a;
  char local_9;
  
                    /* 0xe70e0  1527  TSAFold_NotchPrmtProcess */
  local_18 = &DAT_6bca2a0c;
  local_20 = (void *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x6c);
  memcpy(local_20,&DAT_6bca2a0c,0x13c);
  local_9 = *(char *)((longlong)local_20 + 0x98);
  while (local_9 = local_9 + -1, -1 < local_9) {
    local_25 = *(undefined1 *)((longlong)local_20 + (longlong)(int)local_9 * 3 + 0x99);
    local_26 = *(undefined1 *)((longlong)local_20 + (longlong)(int)local_9 * 3 + 0x9a);
    cVar1 = TSAFold_RemapSensorToRegion(*(undefined8 *)(_refptr_g_data + 8),&local_25,&local_26);
    if (cVar1 == '\0') {
      for (local_a = local_9; (int)local_a < *(char *)((longlong)local_20 + 0x98) + -1;
          local_a = local_a + '\x01') {
        local_24 = memcpy_s((void *)((longlong)local_20 + (longlong)(int)local_a * 3 + 0x99),3,
                            (void *)((longlong)local_20 + (longlong)(local_a + 1) * 3 + 0x99),3);
        if (local_24 != 0) {
          StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                            "../../src/Extension/TSAFold/TSAFold_Prmt.c",0x1d,"rc == EOK");
          HardwareAnalyzer_SetSoftException();
        }
      }
      *(char *)((longlong)local_20 + 0x98) = *(char *)((longlong)local_20 + 0x98) + -1;
    }
    else {
      *(undefined1 *)((longlong)local_20 + (longlong)(int)local_9 * 3 + 0x99) = local_25;
      *(undefined1 *)((longlong)local_20 + (longlong)(int)local_9 * 3 + 0x9a) = local_26;
    }
  }
  return;
}


// ============================================================================
// Function: TSAFold_CMFPrmtProcess @ 6bb272ed
// ============================================================================

void TSAFold_CMFPrmtProcess(void)

{
                    /* 0xe72ed  1504  TSAFold_CMFPrmtProcess */
  return;
}


// ============================================================================
// Function: TSAFold_LogicRegionPrmtProcess @ 6bb2730c
// ============================================================================

void TSAFold_LogicRegionPrmtProcess(void)

{
  TSAFlashPrmt *pTVar1;
  ushort uVar2;
  ushort uVar3;
  
                    /* 0xe730c  1526  TSAFold_LogicRegionPrmtProcess */
  _refptr_g_tsaPrmtFlash->bCols = *(uchar *)(*(longlong *)(_refptr_g_data + 8) + 8);
  _refptr_g_tsaPrmtFlash->bRows = *(uchar *)(*(longlong *)(_refptr_g_data + 8) + 9);
  _refptr_g_tsaPrmtFlash->wBufElementCount =
       (ushort)_refptr_g_tsaPrmtFlash->bCols * (ushort)_refptr_g_tsaPrmtFlash->bRows;
  uVar2 = (ushort)(byte)_refptr_g_tsaPrmtFlash->field_0x1b * (ushort)_refptr_g_tsaPrmtFlash->bCols;
  uVar3 = (ushort)(byte)_refptr_g_tsaPrmtFlash->field_0x1c * (ushort)_refptr_g_tsaPrmtFlash->bRows;
  if (uVar2 < uVar3) {
    *(ushort *)&_refptr_g_tsaPrmtFlash->field_0x2e = uVar3;
    *(ushort *)&_refptr_g_tsaPrmtFlash->field_0x30 = uVar2;
  }
  else {
    *(ushort *)&_refptr_g_tsaPrmtFlash->field_0x2e = uVar2;
    *(ushort *)&_refptr_g_tsaPrmtFlash->field_0x30 = uVar3;
  }
  *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a6 =
       *(undefined2 *)(*(longlong *)(_refptr_g_data + 8) + 0xc);
  *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a8 =
       *(undefined2 *)(*(longlong *)(_refptr_g_data + 8) + 0xe);
  pTVar1 = _refptr_g_tsaPrmtFlash;
  _refptr_g_tsaPrmtFlash->field_0x90 = 0;
  pTVar1->field_0x91 = _refptr_g_tsaPrmtFlash->bCols + 0xff;
  pTVar1->field_0x92 = 0;
  pTVar1->field_0x93 = _refptr_g_tsaPrmtFlash->bRows + 0xff;
  *(undefined2 *)&pTVar1->field_0x94 = 0;
  *(short *)&pTVar1->field_0x96 = *(short *)&_refptr_g_tsaPrmtFlash->field_0x5a6 + -1;
  *(undefined2 *)&pTVar1->field_0x98 = 0;
  *(short *)&pTVar1->field_0x9a = *(short *)&_refptr_g_tsaPrmtFlash->field_0x5a8 + -1;
  return;
}


// ============================================================================
// Function: TSAFold_CentroidPrmtProcess @ 6bb274ce
// ============================================================================

void TSAFold_CentroidPrmtProcess(void)

{
                    /* 0xe74ce  1505  TSAFold_CentroidPrmtProcess */
  return;
}


// ============================================================================
// Function: TSAFold_GripPrmtProcess @ 6bb274d4
// ============================================================================

void TSAFold_GripPrmtProcess(void)

{
                    /* 0xe74d4  1515  TSAFold_GripPrmtProcess */
  return;
}


// ============================================================================
// Function: TSAFold_EdgeReachPrmtProcess @ 6bb274da
// ============================================================================

void TSAFold_EdgeReachPrmtProcess(void)

{
                    /* 0xe74da  1510  TSAFold_EdgeReachPrmtProcess */
  return;
}


// ============================================================================
// Function: TSAFold_RemapLineToRegion @ 6bb27f79
// ============================================================================

void TSAFold_RemapLineToRegion
               (longlong param_1,undefined8 param_2,undefined8 param_3,undefined1 param_4,
               short param_5)

{
  byte bVar1;
  byte bVar2;
  byte bVar3;
  ushort local_10;
  ushort local_e;
  ushort local_c;
  ushort local_a;
  
                    /* 0xe7f79  1538  TSAFold_RemapLineToRegion */
  bVar1 = *(byte *)(param_1 + 8);
  bVar2 = *(byte *)(param_1 + 9);
  bVar3 = *(byte *)(param_1 + 0xb);
  local_10 = 0;
  local_e = (ushort)*(byte *)(param_1 + 10);
  for (local_a = 0; local_a < bVar1; local_a = local_a + 1) {
    TSAFold_CopyUnit(param_3,local_e,param_2,local_10,param_4);
    local_10 = local_10 + 1;
    local_e = local_e + 1;
  }
  local_e = (ushort)(byte)_refptr_g_tsaPrmtFlash->field_0x1f + (ushort)bVar3;
  for (local_c = 0; local_c < bVar2; local_c = local_c + 1) {
    TSAFold_CopyUnit(param_3,local_e,param_2,local_10,param_4);
    local_10 = local_10 + 1;
    local_e = local_e + 1;
  }
  if (param_5 != -0x2153) {
    for (; local_10 <
           (ushort)((ushort)(byte)_refptr_g_tsaPrmtFlash->field_0x20 +
                   (ushort)(byte)_refptr_g_tsaPrmtFlash->field_0x1f); local_10 = local_10 + 1) {
      TSAFold_SetUnit(param_2,local_10,param_4,param_5);
    }
  }
  return;
}


// ============================================================================
// Function: TSAFold_GetTimeStampRegionChange @ 6bb267f5
// ============================================================================

undefined8 TSAFold_GetTimeStampRegionChange(void)

{
                    /* 0xe67f5  1512  TSAFold_GetTimeStampRegionChange */
  return DAT_6bca1e00;
}


// ============================================================================
// Function: TSAFold_Dim1PosOutRemap @ 6bb28420
// ============================================================================

ushort TSAFold_Dim1PosOutRemap(ushort param_1,ushort param_2)

{
  char cVar1;
  ushort local_res8;
  
                    /* 0xe8420  1508  TSAFold_Dim1PosOutRemap */
  cVar1 = TSAFold_IsSupported();
  if ((cVar1 != '\0') && (cVar1 = TSAFold_IsRegionCut(), cVar1 != '\0')) {
    local_res8 = param_1 + *(short *)(*(longlong *)(_refptr_g_data + 8) + 0x10) * param_2;
    if ((int)((uint)param_2 *
              ((uint)*(ushort *)(*(longlong *)(_refptr_g_data + 8) + 0x10) +
              (uint)*(ushort *)(*(longlong *)(_refptr_g_data + 8) + 0xc)) + -1) <
        (int)(uint)local_res8) {
      local_res8 = (*(short *)(*(longlong *)(_refptr_g_data + 8) + 0xc) +
                   *(short *)(*(longlong *)(_refptr_g_data + 8) + 0x10)) * param_2 - 1;
    }
    param_1 = local_res8;
  }
  return param_1;
}


// ============================================================================
// Function: TSAFold_Dim2PosOutRemap @ 6bb284e2
// ============================================================================

ushort TSAFold_Dim2PosOutRemap(ushort param_1,ushort param_2)

{
  char cVar1;
  ushort local_res8;
  
                    /* 0xe84e2  1509  TSAFold_Dim2PosOutRemap */
  cVar1 = TSAFold_IsSupported();
  if ((cVar1 != '\0') && (cVar1 = TSAFold_IsRegionCut(), cVar1 != '\0')) {
    local_res8 = param_1 + *(short *)(*(longlong *)(_refptr_g_data + 8) + 0x12) * param_2;
    if ((int)((uint)param_2 *
              ((uint)*(ushort *)(*(longlong *)(_refptr_g_data + 8) + 0x12) +
              (uint)*(ushort *)(*(longlong *)(_refptr_g_data + 8) + 0xe)) + -1) <
        (int)(uint)local_res8) {
      local_res8 = (*(short *)(*(longlong *)(_refptr_g_data + 8) + 0xe) +
                   *(short *)(*(longlong *)(_refptr_g_data + 8) + 0x12)) * param_2 - 1;
    }
    param_1 = local_res8;
  }
  return param_1;
}


// ============================================================================
// Function: TSAFold_CopyUnit @ 6bb27600
// ============================================================================

void TSAFold_CopyUnit(longlong param_1,ushort param_2,longlong param_3,ushort param_4,char param_5)

{
                    /* 0xe7600  1507  TSAFold_CopyUnit */
  if (param_5 == '\x01') {
    *(undefined1 *)((ulonglong)param_4 + param_3) = *(undefined1 *)(param_1 + (ulonglong)param_2);
  }
  else if (param_5 == '\x02') {
    *(undefined2 *)((ulonglong)param_4 * 2 + param_3) =
         *(undefined2 *)(param_1 + (ulonglong)param_2 * 2);
  }
  else if (param_5 == '\x04') {
    *(undefined4 *)((ulonglong)param_4 * 4 + param_3) =
         *(undefined4 *)(param_1 + (ulonglong)param_2 * 4);
  }
  return;
}


// ============================================================================
// Function: TSAFold_SetUnit @ 6bb276a6
// ============================================================================

void TSAFold_SetUnit(longlong param_1,ushort param_2,char param_3,ushort param_4)

{
                    /* 0xe76a6  1542  TSAFold_SetUnit */
  if (param_3 == '\x01') {
    *(char *)(param_1 + (ulonglong)param_2) = (char)param_4;
  }
  else if (param_3 == '\x02') {
    *(ushort *)((ulonglong)param_2 * 2 + param_1) = param_4;
  }
  else if (param_3 == '\x04') {
    *(uint *)((ulonglong)param_2 * 4 + param_1) = (uint)param_4;
  }
  return;
}


// ============================================================================
// Function: TSAFold_RemapSensorToRegion @ 6bb280f0
// ============================================================================

undefined8 TSAFold_RemapSensorToRegion(longlong param_1,byte *param_2,byte *param_3)

{
  undefined8 uVar1;
  
                    /* 0xe80f0  1540  TSAFold_RemapSensorToRegion */
  if (*param_2 < *(byte *)(param_1 + 10)) {
    uVar1 = 0;
  }
  else if ((uint)*param_2 < (uint)*(byte *)(param_1 + 8) + (uint)*(byte *)(param_1 + 10)) {
    if (*param_3 < *(byte *)(param_1 + 0xb)) {
      uVar1 = 0;
    }
    else if ((uint)*param_3 < (uint)*(byte *)(param_1 + 9) + (uint)*(byte *)(param_1 + 0xb)) {
      *param_2 = *param_2 - *(char *)(param_1 + 10);
      *param_3 = *param_3 - *(char *)(param_1 + 0xb);
      uVar1 = 1;
    }
    else {
      uVar1 = 0;
    }
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


