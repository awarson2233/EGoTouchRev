// ============================================================================
// Function: TSAOut_PostProcess @ 6ba6a77a
// ============================================================================

void TSAOut_PostProcess(void)

{
  char cVar1;
  uchar local_a;
  char local_9;
  
                    /* 0x2a77a  1622  TSAOut_PostProcess */
  local_a = '\0';
  *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 5) = 0;
  *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 6) = 0;
  if ((*(char *)(*(longlong *)_refptr_g_touchesPtr + 3) != '\0') &&
     (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) == 0)) {
    cVar1 = TSA_IsGameScenario();
    if (cVar1 == '\0') {
      **(undefined1 **)_refptr_g_touchesPtr = 0;
      goto LAB_6ba6a813;
    }
  }
  local_9 = **(char **)_refptr_g_touchesPtr;
  while (local_9 = local_9 + -1, -1 < local_9) {
    TSAOut_PostProcessBefUDFPUnit((int)local_9);
  }
LAB_6ba6a813:
  UDFPEvent_Process();
  local_9 = **(char **)_refptr_g_touchesPtr;
  while (local_9 = local_9 + -1, -1 < local_9) {
    TSAOut_PostProcessUnit((int)local_9,&local_a);
  }
  (*_refptr_g_prevTouchesPtr)->field_0x6 = *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 6);
  (*_refptr_g_tsaStaticPtr)->bField_0x00 = local_a;
  *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x27c =
       *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x18);
  (*_refptr_g_tsaStaticPtr)->nField_0x280 =
       *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x27c & ~(*_refptr_g_tsaStaticPtr)->nField_0x278;
  cVar1 = TSAStatic_IsHWResetPrev();
  if (cVar1 != '\0') {
    cVar1 = TSAStatic_IsHWReset();
    if (cVar1 == '\0') {
      (*_refptr_g_tsaStaticPtr)->nField_0x280 = (*_refptr_g_tsaStaticPtr)->nField_0x280 | 0x200000;
    }
  }
  SideTouch_Report();
  TSAOut_ReportExtInfo();
  return;
}


// ============================================================================
// Function: TSAOut_PostProcessUnit @ 6ba6a3ef
// ============================================================================

void TSAOut_PostProcessUnit(char param_1,char *param_2)

{
  byte bVar1;
  ushort uVar2;
  
  if (((((*(uint *)((longlong)(int)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) & 8)
         == 0) ||
       ((*(uint *)((longlong)(int)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) &
        0x8000000) != 0)) ||
      ((*(uint *)((longlong)(int)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 500) & 0x4000
       ) != 0)) ||
     ((*(uint *)((longlong)(int)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) &
      0x10000) != 0)) {
    Touch_UnitRemove(param_1);
  }
  else {
    uVar2 = TSAOut_DisplayXPos(param_1);
    if (uVar2 < (*_refptr_g_tsaStaticPtr)->wField_0x2e0) {
      TSA_SetXPos(param_1,(*_refptr_g_tsaStaticPtr)->wField_0x2e0);
    }
    else {
      uVar2 = TSAOut_DisplayXPos(param_1);
      if ((ushort)(*_refptr_g_tsaStaticPtr)->nXResMax < uVar2) {
        TSA_SetXPos(param_1,(*_refptr_g_tsaStaticPtr)->nXResMax);
      }
    }
    *(char *)(*(longlong *)_refptr_g_touchesPtr + 5) =
         *(char *)(*(longlong *)_refptr_g_touchesPtr + 5) + '\x01';
    *(char *)(*(longlong *)_refptr_g_touchesPtr + 6) =
         *(char *)(*(longlong *)_refptr_g_touchesPtr + 6) + '\x01';
    if ((*(uint *)((longlong)(int)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) & 4)
        != 0) {
      *param_2 = *param_2 + '\x01';
    }
    *(void **)&(*_refptr_g_tsaStaticPtr)->field_0x70 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    bVar1 = PrevTouch_GetIdxBasedOnID
                      (*(undefined1 *)
                        ((longlong)(int)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x154)
                      );
    if ((bVar1 < 0x14) &&
       ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x1f8)
        & 0x20000000) == 0)) {
      *(void **)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x598) =
           (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
      *(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x1f8) =
           *(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)bVar1 * 0x478 + 0x1f8) | 0x20000000;
    }
  }
  return;
}


// ============================================================================
// Function: TSAOut_ReportExtInfo @ 6ba6a6cd
// ============================================================================

void TSAOut_ReportExtInfo(void)

{
  char cVar1;
  
                    /* 0x2a6cd  1623  TSAOut_ReportExtInfo */
  cVar1 = SideTouch_UpdateExternalThenarInfo();
  if (cVar1 == '\0') {
    Thenar_UpdateExternalInfo();
  }
  return;
}


// ============================================================================
// Function: TSAOut_PostProcessBefUDFPUnit @ 6ba6a6f3
// ============================================================================

void TSAOut_PostProcessBefUDFPUnit(char param_1)

{
  if ((*(uint *)((longlong)(int)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) & 8) ==
      0) {
    Touch_UnitRemove(param_1);
  }
  else if ((*(uint *)((longlong)(int)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) &
           0x8000000) != 0) {
    Touch_UnitRemove(param_1);
  }
  return;
}


// ============================================================================
// Function: TSAOut_DisplayXPos @ 6ba6a241
// ============================================================================

void TSAOut_DisplayXPos(undefined1 param_1)

{
                    /* 0x2a241  1615  TSAOut_DisplayXPos */
  TSAOut_TouchXPosInternal(param_1,1);
  return;
}


// ============================================================================
// Function: TSAOut_Pixel2DisplayDim1 @ 6ba69b0b
// ============================================================================

void TSAOut_Pixel2DisplayDim1(undefined2 param_1)

{
                    /* 0x29b0b  1620  TSAOut_Pixel2DisplayDim1 */
  TSAOut_Dim1PosInternal(param_1,1);
  return;
}


// ============================================================================
// Function: TSAOut_IntToExtDist @ 6ba69bfb
// ============================================================================

uint TSAOut_IntToExtDist(short param_1)

{
  uint uVar1;
  short local_res8;
  
                    /* 0x29bfb  1619  TSAOut_IntToExtDist */
  local_res8 = param_1;
  if (param_1 < 0) {
    local_res8 = -param_1;
  }
  uVar1 = (int)local_res8 * *(int *)(_refptr_g_tsaPrmtFlashConst + 4) + 0x8000U >> 0x10;
  if (param_1 < 0) {
    uVar1 = -uVar1;
  }
  return uVar1;
}


// ============================================================================
// Function: TSAOut_Pixel2DisplayDim2 @ 6ba69e95
// ============================================================================

void TSAOut_Pixel2DisplayDim2(undefined2 param_1)

{
                    /* 0x29e95  1621  TSAOut_Pixel2DisplayDim2 */
  TSAOut_Dim2PosInternal(param_1,1);
  return;
}


// ============================================================================
// Function: TSAOut_Axis @ 6ba6a2e5
// ============================================================================

void TSAOut_Axis(short param_1)

{
                    /* 0x2a2e5  1611  TSAOut_Axis */
  TSAOut_IntToExtDist((int)param_1);
  return;
}


// ============================================================================
// Function: TSAOut_Angle @ 6ba6a305
// ============================================================================

char TSAOut_Angle(char param_1)

{
  char local_res8;
  
                    /* 0x2a305  1610  TSAOut_Angle */
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    if (param_1 < '\x01') {
      local_res8 = param_1 + 'Z';
    }
    else {
      local_res8 = param_1 + -0x5a;
    }
  }
  else {
    local_res8 = -param_1;
  }
  return local_res8;
}


// ============================================================================
// Function: TSAOut_TouchXPosInternal @ 6ba69f4d
// ============================================================================

ulonglong TSAOut_TouchXPosInternal(byte param_1,short param_2)

{
  short sVar1;
  int iVar2;
  ulonglong uVar3;
  
  if ((_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass ^ _refptr_g_tsaPrmtFlash->field_0x18) ==
      _refptr_g_tsaPrmtFlash->field_0x5a4) {
    if (_refptr_g_tsaPrmtFlash->field_0x5a2 == '\0') {
      uVar3 = TSAOut_Dim1PosInternal
                        (*(undefined2 *)
                          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x170),param_2);
    }
    else {
      sVar1 = *(short *)&_refptr_g_tsaPrmtFlash->field_0x5a6;
      iVar2 = TSAOut_Dim1PosInternal
                        (*(undefined2 *)
                          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x170),param_2);
      uVar3 = (ulonglong)(((uint)(ushort)(sVar1 * param_2) - iVar2) - 1);
    }
  }
  else if (_refptr_g_tsaPrmtFlash->field_0x5a3 == '\0') {
    uVar3 = TSAOut_Dim2PosInternal
                      (*(undefined2 *)
                        ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x172),param_2);
  }
  else {
    sVar1 = *(short *)&_refptr_g_tsaPrmtFlash->field_0x5a8;
    iVar2 = TSAOut_Dim2PosInternal
                      (*(undefined2 *)
                        ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x172),param_2);
    uVar3 = (ulonglong)(((uint)(ushort)(sVar1 * param_2) - iVar2) - 1);
  }
  return uVar3;
}


// ============================================================================
// Function: TSAOut_RevertDim1Pos @ 6ba69b2f
// ============================================================================

uint TSAOut_RevertDim1Pos(ushort param_1)

{
  uint local_10;
  int local_c;
  
                    /* 0x29b2f  1624  TSAOut_RevertDim1Pos */
  local_c = (int)(char)_refptr_g_tsaPrmtFlash->field_0x5ae + (uint)param_1;
  if (local_c < 0) {
    local_c = 0;
  }
  local_10 = (local_c * 0x10000 + (*(uint *)(_refptr_g_tsaPrmtFlashConst + 4) >> 1)) /
             *(uint *)(_refptr_g_tsaPrmtFlashConst + 4);
  if (*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x20) <= local_10) {
    local_10 = *(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x20) - 1;
  }
  return local_10;
}


// ============================================================================
// Function: TSAOut_RevertDim2Pos @ 6ba69eb9
// ============================================================================

uint TSAOut_RevertDim2Pos(ushort param_1)

{
  uint local_10;
  int local_c;
  
                    /* 0x29eb9  1625  TSAOut_RevertDim2Pos */
  local_c = (int)(char)_refptr_g_tsaPrmtFlash->field_0x5b0 + (uint)param_1;
  if (local_c < 0) {
    local_c = 0;
  }
  local_10 = (local_c * 0x10000 + (*(uint *)(_refptr_g_tsaPrmtFlashConst + 8) >> 1)) /
             *(uint *)(_refptr_g_tsaPrmtFlashConst + 8);
  if (*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x22) <= local_10) {
    local_10 = *(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x22) - 1;
  }
  return local_10;
}


// ============================================================================
// Function: TSAOut_Dim1PosInternal @ 6ba69a50
// ============================================================================

undefined2 TSAOut_Dim1PosInternal(ushort param_1,ushort param_2)

{
  undefined2 uVar1;
  uint local_c;
  
                    /* 0x29a50  1612  TSAOut_Dim1PosInternal */
  local_c = ((uint)param_1 * *(int *)(_refptr_g_tsaPrmtFlashConst + 4) * (uint)param_2 + 0x8000 >>
            0x10) - (uint)param_2 * (int)(char)_refptr_g_tsaPrmtFlash->field_0x5ae;
  if ((int)local_c < 0) {
    local_c = 0;
  }
  if ((int)((uint)param_2 * (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a6 + -1) <
      (int)local_c) {
    local_c = (uint)param_2 * (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a6 - 1;
  }
  uVar1 = TSAFold_Dim1PosOutRemap(local_c & 0xffff,param_2);
  return uVar1;
}


// ============================================================================
// Function: TSAOut_Dim2PosInternal @ 6ba69ccb
// ============================================================================

undefined2 TSAOut_Dim2PosInternal(ushort param_1,ushort param_2)

{
  byte bVar1;
  undefined2 uVar2;
  undefined4 uVar3;
  int iVar4;
  int iVar5;
  uint local_10;
  uint local_c;
  
                    /* 0x29ccb  1613  TSAOut_Dim2PosInternal */
  local_10 = (uint)param_1;
  if (param_1 < 0x100) {
    if ((char)_refptr_g_tsaPrmtFlash->field_0x5b0 < '\0') {
      uVar3 = TSAOut_Display2Pixel
                        ((int)(char)_refptr_g_tsaPrmtFlash->field_0x5b0 -
                         (int)(char)_refptr_g_tsaPrmtFlash->field_0x5b2);
      local_10 = TSAOut_EdgeScale(uVar3,param_1);
    }
  }
  else if (((int)((_refptr_g_tsaPrmtFlash->bRows - 1) * 0x100) < (int)local_10) &&
          ((char)_refptr_g_tsaPrmtFlash->field_0x5b1 < '\0')) {
    bVar1 = _refptr_g_tsaPrmtFlash->bRows;
    uVar3 = TSAOut_Display2Pixel
                      ((int)(char)_refptr_g_tsaPrmtFlash->field_0x5b1 -
                       (int)(char)_refptr_g_tsaPrmtFlash->field_0x5b3);
    iVar5 = TSAOut_EdgeScale(uVar3,(uint)bVar1 * 0x100 - local_10);
    local_10 = (uint)_refptr_g_tsaPrmtFlash->bRows * 0x100 - iVar5;
  }
  iVar4 = (uint)param_2 * *(int *)(_refptr_g_tsaPrmtFlashConst + 8) * local_10;
  iVar5 = iVar4 + 0x8000;
  if (iVar5 < 0) {
    iVar5 = iVar4 + 0x17fff;
  }
  local_c = (iVar5 >> 0x10) - (uint)param_2 * (int)(char)_refptr_g_tsaPrmtFlash->field_0x5b0;
  if ((int)local_c < 0) {
    local_c = 0;
  }
  if ((int)((uint)param_2 * (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a8) <= (int)local_c)
  {
    local_c = (uint)param_2 * (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a8 - 1;
  }
  uVar2 = TSAFold_Dim2PosOutRemap(local_c & 0xffff,param_2);
  return uVar2;
}


// ============================================================================
// Function: TSAOut_DisplayYPos @ 6ba6a293
// ============================================================================

void TSAOut_DisplayYPos(undefined1 param_1)

{
                    /* 0x2a293  1616  TSAOut_DisplayYPos */
  TSAOut_TouchYPosInternal(param_1,1);
  return;
}


// ============================================================================
// Function: TSAOut_Display2Pixel @ 6ba69c56
// ============================================================================

int TSAOut_Display2Pixel(int param_1)

{
                    /* 0x29c56  1614  TSAOut_Display2Pixel */
  return (param_1 * 0x10000 + *(int *)(_refptr_g_tsaPrmtFlashConst + 8) / 2) /
         *(int *)(_refptr_g_tsaPrmtFlashConst + 8);
}


// ============================================================================
// Function: TSAOut_EdgeScale @ 6ba69c9e
// ============================================================================

int TSAOut_EdgeScale(int param_1,int param_2)

{
                    /* 0x29c9e  1617  TSAOut_EdgeScale */
  param_2 = (0x100 - param_1) * param_2;
  if (param_2 < 0) {
    param_2 = param_2 + 0xff;
  }
  return param_1 + (param_2 >> 8);
}


// ============================================================================
// Function: TSAOut_TouchYPosInternal @ 6ba6a0c7
// ============================================================================

ulonglong TSAOut_TouchYPosInternal(byte param_1,short param_2)

{
  short sVar1;
  int iVar2;
  ulonglong uVar3;
  
  if ((_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass ^ _refptr_g_tsaPrmtFlash->field_0x18) ==
      _refptr_g_tsaPrmtFlash->field_0x5a4) {
    if (_refptr_g_tsaPrmtFlash->field_0x5a3 == '\0') {
      uVar3 = TSAOut_Dim2PosInternal
                        (*(undefined2 *)
                          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x172),param_2);
    }
    else {
      sVar1 = *(short *)&_refptr_g_tsaPrmtFlash->field_0x5a8;
      iVar2 = TSAOut_Dim2PosInternal
                        (*(undefined2 *)
                          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x172),param_2);
      uVar3 = (ulonglong)(((uint)(ushort)(sVar1 * param_2) - iVar2) - 1);
    }
  }
  else if (_refptr_g_tsaPrmtFlash->field_0x5a2 == '\0') {
    uVar3 = TSAOut_Dim1PosInternal
                      (*(undefined2 *)
                        ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x170),param_2);
  }
  else {
    sVar1 = *(short *)&_refptr_g_tsaPrmtFlash->field_0x5a6;
    iVar2 = TSAOut_Dim1PosInternal
                      (*(undefined2 *)
                        ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x170),param_2);
    uVar3 = (ulonglong)(((uint)(ushort)(sVar1 * param_2) - iVar2) - 1);
  }
  return uVar3;
}


// ============================================================================
// Function: TSAOut_ExtToIntDist @ 6ba69bc3
// ============================================================================

ulonglong TSAOut_ExtToIntDist(ushort param_1)

{
                    /* 0x29bc3  1618  TSAOut_ExtToIntDist */
  return ((ulonglong)param_1 << 0x10) / (ulonglong)*(uint *)(_refptr_g_tsaPrmtFlashConst + 4);
}


// ============================================================================
// Function: TSAOut_RptXPos @ 6ba6a264
// ============================================================================

void TSAOut_RptXPos(undefined1 param_1)

{
                    /* 0x2a264  1626  TSAOut_RptXPos */
  TSAOut_TouchXPosInternal(param_1,*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a0);
  return;
}


// ============================================================================
// Function: TSAOut_RptYPos @ 6ba6a2b6
// ============================================================================

void TSAOut_RptYPos(undefined1 param_1)

{
                    /* 0x2a2b6  1627  TSAOut_RptYPos */
  TSAOut_TouchYPosInternal(param_1,*(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5a0);
  return;
}


