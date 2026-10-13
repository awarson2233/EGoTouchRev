// ============================================================================
// Function: ToeSynaBl_GetSidePrpt @ 6bb471b0
// ============================================================================

void ToeSynaBl_GetSidePrpt(void)

{
  uchar uVar1;
  char cVar2;
  int iVar3;
  errno_t eVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  bool bVar7;
  
                    /* 0x1071b0  2469  ToeSynaBl_GetSidePrpt */
  cVar2 = TSAStatic_IsSafeBaselineApplied();
  if ((cVar2 == '\0') && (_refptr_g_tsaPrmtFlash[2].field_0x6d0 != '\0')) {
    uVar1 = _refptr_g_tsaPrmtFlash->bCols;
    bVar7 = ((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) != 0;
    iVar3 = ToeSynaBl_IsColumnRawChanged(bVar7);
    if ((iVar3 != 0) || (iVar3 = ToeSynaBl_IsColumnRawChanged((uVar1 - bVar7) + -1), iVar3 != 0)) {
      eVar4 = memcpy_s(*(void **)_refptr_g_tsaBufRawCaptureNextSide,
                       (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,
                       *(void **)_refptr_g_tsaBufPreCMFRaw,
                       (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
      if (eVar4 != 0) {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                          "../../src/WorkAround/ToeSynaBl/ToeSynaBl.c",0x2f9,"rc == EOK");
        HardwareAnalyzer_SetSoftException();
      }
      *(void **)&(*_refptr_g_tsaStaticPtr)->field_0x398 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr
      ;
      *(void **)&(*_refptr_g_tsaStaticPtr)->field_0x3a0 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr
      ;
    }
    uVar5 = ToeSynaBl_IsNextSideDifNormal(bVar7);
    uVar6 = ToeSynaBl_IsNextSideDifNormal((uVar1 - bVar7) + -1);
    cVar2 = ToeSynaBl_PrptSideRawProtected(uVar5,uVar6);
    if (cVar2 != '\0') {
      *(void **)&(*_refptr_g_tsaStaticPtr)->field_0x398 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr
      ;
    }
    cVar2 = ToeSynaBl_PrptSideRawProtected2(uVar1,bVar7,uVar5,uVar6);
    if (cVar2 != '\0') {
      *(void **)&(*_refptr_g_tsaStaticPtr)->field_0x3a0 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr
      ;
    }
    if (((ulonglong)
         ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
         *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x398) < 0x3e9) &&
       ((ulonglong)
        ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
        *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x3a0) < 0x7d1)) {
      g_toeSynaIsExcludeSide = 0;
    }
    else {
      g_toeSynaIsExcludeSide = 1;
    }
  }
  return;
}


// ============================================================================
// Function: ToeSynaBl_IsColumnRawChanged @ 6bb46bc3
// ============================================================================

undefined4 ToeSynaBl_IsColumnRawChanged(byte param_1)

{
  uint uVar1;
  uint uVar2;
  byte local_9;
  
  local_9 = 0;
  while( true ) {
    if (_refptr_g_tsaPrmtFlash->bRows <= local_9) {
      return 0;
    }
    uVar1 = (uint)*(ushort *)
                   ((ulonglong)
                    ((uint)param_1 + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) *
                    2 + *(longlong *)_refptr_g_tsaBufPreCMFRaw) -
            (uint)*(ushort *)
                   ((ulonglong)
                    ((uint)param_1 + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) *
                    2 + *(longlong *)_refptr_g_tsaBufRawCaptureNextSide);
    uVar2 = (int)uVar1 >> 0x1f;
    if (200 < (int)((uVar1 ^ uVar2) - uVar2)) break;
    local_9 = local_9 + 1;
  }
  return 1;
}


// ============================================================================
// Function: ToeSynaBl_IsNextSideDifNormal @ 6bb46c9c
// ============================================================================

uint ToeSynaBl_IsNextSideDifNormal(byte param_1)

{
  byte bVar1;
  short sVar2;
  uint uVar3;
  uint local_10;
  byte local_9;
  
  bVar1 = _refptr_g_tsaPrmtFlash->bRows;
  uVar3 = (uint)bVar1;
  local_10 = 0;
  if ((0xf9 < *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)param_1 * 2)) ||
     (0xf9 < *(short *)((ulonglong)
                        ((uint)param_1 + (uVar3 - 1) * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)
                        * 2 + *(longlong *)_refptr_g_tsaBufDif))) {
    local_10 = 1;
  }
  for (local_9 = 1; local_9 < 7; local_9 = local_9 + 1) {
    sVar2 = *(short *)((ulonglong)
                       ((uint)param_1 + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff
                       ) * 2 + *(longlong *)_refptr_g_tsaBufDif);
    if (0x5dc < sVar2) {
      local_10 = local_10 | 2;
    }
    if (0xf9 < sVar2) {
      local_10 = local_10 | 1;
    }
    if (sVar2 < -0x4b0) {
      local_10 = local_10 | 8;
    }
  }
  for (local_9 = 7; (int)(uint)local_9 < (int)(uVar3 - 7); local_9 = local_9 + 1) {
    sVar2 = *(short *)((ulonglong)
                       ((uint)param_1 + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff
                       ) * 2 + *(longlong *)_refptr_g_tsaBufDif);
    if (0x3b6 < sVar2) {
      local_10 = local_10 | 2;
    }
    if (0xf9 < sVar2) {
      local_10 = local_10 | 1;
    }
    if (sVar2 < -900) {
      local_10 = local_10 | 8;
    }
  }
  for (local_9 = bVar1 - 7; (int)(uint)local_9 < (int)(uVar3 - 1); local_9 = local_9 + 1) {
    sVar2 = *(short *)((ulonglong)
                       ((uint)param_1 + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff
                       ) * 2 + *(longlong *)_refptr_g_tsaBufDif);
    if (0x5dc < sVar2) {
      local_10 = local_10 | 2;
    }
    if (0xf9 < sVar2) {
      local_10 = local_10 | 1;
    }
    if (sVar2 < -0x4b0) {
      local_10 = local_10 | 8;
    }
  }
  if (*(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)param_1 * 2) < 300) {
    for (local_9 = 1; local_9 < 7; local_9 = local_9 + 1) {
      if (300 < *(short *)((ulonglong)
                           ((uint)param_1 + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols &
                           0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif)) {
        local_10 = local_10 | 0x10;
        break;
      }
    }
  }
  if (*(short *)((ulonglong)
                 ((uint)param_1 + (uVar3 - 1) * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
                *(longlong *)_refptr_g_tsaBufDif) < 300) {
    for (local_9 = bVar1 - 7; (int)(uint)local_9 < (int)(uVar3 - 1); local_9 = local_9 + 1) {
      if (300 < *(short *)((ulonglong)
                           ((uint)param_1 + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols &
                           0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif)) {
        return local_10 | 0x10;
      }
    }
  }
  return local_10;
}


// ============================================================================
// Function: ToeSynaBl_PrptSideRawProtected @ 6bb46f9f
// ============================================================================

undefined1 ToeSynaBl_PrptSideRawProtected(uint param_1,uint param_2)

{
  undefined1 local_9;
  
  local_9 = 0;
  if (((param_1 & 1) == 0) && ((param_2 & 1) == 0)) {
    local_9 = 1;
  }
  else if (((param_1 & 2) == 0) &&
          ((((param_2 & 2) == 0 && ((param_1 & 0x10) == 0)) && ((param_2 & 0x10) == 0)))) {
    if (((param_1 & 8) != 0) || ((param_2 & 8) != 0)) {
      local_9 = 1;
    }
  }
  else {
    local_9 = 1;
  }
  return local_9;
}


// ============================================================================
// Function: ToeSynaBl_PrptSideRawProtected2 @ 6bb4701b
// ============================================================================

undefined1 ToeSynaBl_PrptSideRawProtected2(int param_1,byte param_2,uint param_3,uint param_4)

{
  bool bVar1;
  bool bVar2;
  undefined1 local_9;
  
  local_9 = 0;
  if ((*(int *)(_refptr_g_tsaBufSelfForBlRst + (longlong)(int)(uint)param_2 * 4) < 0x1f41) ||
     (*(int *)(_refptr_g_tsaBufSelfForBlRst + (longlong)(int)(uint)param_2 * 4) <=
      *(int *)(_refptr_g_tsaBufSsDifForBl + (longlong)(int)(uint)param_2 * 4) * 10)) {
    bVar1 = false;
  }
  else {
    bVar1 = true;
  }
  if ((*(int *)(_refptr_g_tsaBufSelfForBlRst + (longlong)(int)((param_1 + -1) - (uint)param_2) * 4)
       < 0x1f41) ||
     (*(int *)(_refptr_g_tsaBufSelfForBlRst + (longlong)(int)((param_1 + -1) - (uint)param_2) * 4)
      <= *(int *)(_refptr_g_tsaBufSsDifForBl + (longlong)(int)((param_1 + -1) - (uint)param_2) * 4)
         * 10)) {
    bVar2 = false;
  }
  else {
    bVar2 = true;
  }
  if (((param_3 & 1) == 0) && ((param_4 & 1) == 0)) {
    local_9 = 1;
  }
  else if (((param_3 & 2) == 0) || (bVar1)) {
    if (((param_4 & 2) == 0) || (bVar2)) {
      if (((param_3 & 1) == 0) ||
         ((*(int *)(_refptr_g_tsaBufSsDifForBl + (longlong)(int)(uint)param_2 * 4) < -2999 ||
          (bVar1)))) {
        if (((param_4 & 1) != 0) &&
           ((-3000 < *(int *)(_refptr_g_tsaBufSsDifForBl +
                             (longlong)(int)((param_1 + -1) - (uint)param_2) * 4) && (!bVar2)))) {
          local_9 = 1;
        }
      }
      else {
        local_9 = 1;
      }
    }
    else {
      local_9 = 1;
    }
  }
  else {
    local_9 = 1;
  }
  return local_9;
}


// ============================================================================
// Function: ToeSynaBl_BackLightShiftUnit @ 6bb45dd1
// ============================================================================

undefined8 ToeSynaBl_BackLightShiftUnit(ushort param_1)

{
  short sVar1;
  int iVar2;
  char cVar3;
  undefined8 uVar4;
  int iVar5;
  ushort local_1a;
  int local_18;
  int local_14;
  ushort local_10;
  ushort local_e;
  ushort local_c;
  short local_a;
  
                    /* 0x105dd1  2466  ToeSynaBl_BackLightShiftUnit */
  if (((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6d4 & 1) == 0) ||
     (cVar3 = ToeSynaBl_BackLightShiftCheckTouch(), cVar3 != '\0')) {
    uVar4 = 0;
  }
  else {
    local_c = 0;
    iVar2 = (int)((_refptr_g_tsaPrmtFlash->bRows - 9) * (uint)param_1) / 100;
    for (local_e = 2; (int)(uint)local_e < (int)(_refptr_g_tsaPrmtFlash->bCols - 2);
        local_e = local_e + 1) {
      local_10 = 0;
      local_14 = 0;
      local_18 = 0;
      local_a = *(short *)((ulonglong)
                           ((uint)local_e +
                            (uint)_refptr_g_tsaPrmtFlash->bCols +
                            (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
                          *(longlong *)_refptr_g_tsaBufDif);
      for (local_1a = 3; (int)(uint)local_1a < (int)(_refptr_g_tsaPrmtFlash->bRows - 2);
          local_1a = local_1a + 1) {
        local_10 = local_10 + 1;
        sVar1 = *(short *)((ulonglong)(ushort)(local_e + _refptr_g_tsaPrmtFlash->bCols * local_1a) *
                           2 + *(longlong *)_refptr_g_tsaBufDif);
        if ((0xaa < (int)sVar1 - (int)local_a) || ((int)sVar1 - (int)local_a < -0xaa)) {
          local_c = local_c + 1;
          break;
        }
        if (4 < local_10) {
          iVar5 = (int)sVar1 -
                  (int)*(short *)((ulonglong)
                                  ((uint)local_e +
                                   (local_1a - 5) * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) *
                                  2 + *(longlong *)_refptr_g_tsaBufDif);
          if (iVar5 < 0x1f) {
            if (iVar5 < -0x1e) {
              local_18 = local_18 + 1;
            }
          }
          else {
            local_14 = local_14 + 1;
          }
        }
        local_a = sVar1;
      }
      if ((local_14 < iVar2) && (local_18 < iVar2)) {
        local_c = local_c + 1;
      }
    }
    if (local_c < 5) {
      uVar4 = 1;
    }
    else {
      uVar4 = 0;
    }
  }
  return uVar4;
}


// ============================================================================
// Function: ToeSynaBl_BackLightShiftCheckTouch @ 6bb45d0d
// ============================================================================

undefined8 ToeSynaBl_BackLightShiftCheckTouch(void)

{
  char cVar1;
  undefined8 uVar2;
  ushort local_a;
  
  cVar1 = BLReset_IsTouchToBeProtected2(0);
  if (cVar1 == '\0') {
    cVar1 = TSA_IsGameScenario();
    if ((cVar1 != '\0') && ((*_refptr_g_prevTouchesPtr)->bPrevTouchCount < 6)) {
      for (local_a = 0; local_a < (*_refptr_g_prevTouchesPtr)->bPrevTouchCount;
          local_a = local_a + 1) {
        if (0xe < *(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                           (longlong)(int)(uint)local_a * 0x478 + 0x4ec)) {
          *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 =
               *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 | 1;
          return 1;
        }
      }
    }
    uVar2 = 0;
  }
  else {
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: ToeSynaBl_CheckNumOverNoise @ 6bb45640
// ============================================================================

undefined8 ToeSynaBl_CheckNumOverNoise(void)

{
  undefined8 uVar1;
  
                    /* 0x105640  2467  ToeSynaBl_CheckNumOverNoise */
  if ((_refptr_g_tsaPrmtFlash[2].field_0x6d0 == '\0') ||
     (4 < *(ushort *)&_refptr_g_tsaPrpt->field_0xc)) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: ToeSynaBl_IsBaselineAbnormalAftScreenOn @ 6bb465b4
// ============================================================================

undefined8 ToeSynaBl_IsBaselineAbnormalAftScreenOn(void)

{
  char cVar1;
  undefined8 uVar2;
  ushort local_12;
  ushort local_10;
  ushort local_e;
  ushort local_c;
  ushort local_a;
  
                    /* 0x1065b4  2472  ToeSynaBl_IsBaselineAbnormalAftScreenOn */
  cVar1 = TSA_IsGameScenario();
  if (cVar1 == '\0') {
    if ((_refptr_g_tsaPrmtFlash[2].field_0x6d0 == '\0') ||
       ((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6d4 & 1) == 0)) {
      *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3c0 = 0;
      *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3bc = 0;
    }
    else {
      local_a = 0;
      local_c = 0;
      local_e = 0;
      for (local_10 = 0; local_10 < _refptr_g_tsaPrmtFlash->bCols; local_10 = local_10 + 1) {
        if (600 < *(int *)(_refptr_g_tsaBufSelfForBlRst + ((longlong)(int)(uint)local_e + 0x3c0) * 4
                          )) {
          local_a = local_a + 1;
        }
        local_e = local_e + 1;
      }
      local_e = (ushort)_refptr_g_tsaPrmtFlash->bCols;
      for (local_12 = 0; local_12 < _refptr_g_tsaPrmtFlash->bRows; local_12 = local_12 + 1) {
        if (0x96 < *(int *)(_refptr_g_tsaBufSelfForBlRst +
                           ((longlong)(int)(uint)local_e + 0x460) * 4)) {
          local_c = local_c + 1;
        }
        local_e = local_e + 1;
      }
      if ((((((uint)_refptr_g_tsaPrmtFlash->bCols + (uint)_refptr_g_tsaPrmtFlash->bCols) / 3 <
             (uint)local_a) && (_refptr_g_tsaPrmtFlash->bRows >> 1 < local_c)) &&
          (3 < *(ushort *)&_refptr_g_tsaPrpt->field_0x180)) &&
         ((3 < *(ushort *)&_refptr_g_tsaPrpt->field_0x182 &&
          (cVar1 = BLReset_CheckNumOverNoise(), cVar1 != '\0')))) {
        ToeSynaBl_BackLightShiftProcess();
        ToeSynaBl_ClothRiskProcess();
      }
      else {
        *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3c0 = 0;
        *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3bc = 0;
      }
    }
    if (*(int *)&(*_refptr_g_tsaStaticPtr)->field_0x3bc < 0x1389) {
      if (*(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x3c0 < 0x14e) {
        uVar2 = 0;
      }
      else {
        *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3c0 = 0;
        uVar2 = 1;
      }
    }
    else {
      *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3bc = 0;
      uVar2 = 1;
    }
    return uVar2;
  }
  return 0;
}


// ============================================================================
// Function: ToeSynaBl_IsSideTouchToBeProtected @ 6bb4577d
// ============================================================================

undefined8 ToeSynaBl_IsSideTouchToBeProtected(void)

{
  byte bVar1;
  char cVar2;
  ushort local_e;
  short local_c;
  short local_a;
  
                    /* 0x10577d  2476  ToeSynaBl_IsSideTouchToBeProtected */
  if ((_refptr_g_tsaPrmtFlash[2].field_0x6d0 == '\0') || (cVar2 = SS_IsDim1Valid(), cVar2 == '\0'))
  {
    return 0;
  }
  bVar1 = _refptr_g_tsaPrmtFlash[2].field_0x6d1;
  if ((bVar1 != 0) && (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass != '\x01')) {
    local_a = 0;
    local_c = 0;
    for (local_e = (ushort)bVar1;
        (int)(uint)local_e < (int)((uint)_refptr_g_tsaPrmtFlash->bCols - (uint)bVar1);
        local_e = local_e + 1) {
      if (600 < *(int *)(_refptr_g_tsaBufSelfForBlRst + ((longlong)(int)(uint)local_e + 0x3c0) * 4))
      {
        local_a = local_a + 1;
      }
      if ((800 < *(int *)(_refptr_g_tsaBufSsDifForBl + (longlong)(int)(uint)local_e * 4)) ||
         (*(int *)(_refptr_g_tsaBufSsDifForBl + (longlong)(int)(uint)local_e * 4) < -800)) {
        local_c = local_c + 1;
      }
    }
    if ((local_a == 0) && (local_c == 0)) {
      if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) == 0) {
        cVar2 = ToeSynaBl_SideTouchCheckSS(0,(int)(short)(bVar1 - 2));
        if ((cVar2 != '\0') ||
           (cVar2 = ToeSynaBl_SideTouchCheckSS(0,(int)(short)(bVar1 - 1)), cVar2 != '\0')) {
          return 1;
        }
      }
      else {
        cVar2 = ToeSynaBl_SideTouchCheckSS(0,(int)(short)(bVar1 - 2));
        if ((((cVar2 != '\0') ||
             (cVar2 = ToeSynaBl_SideTouchCheckSS(1,(int)(short)(bVar1 - 1)), cVar2 != '\0')) ||
            (cVar2 = ToeSynaBl_SideTouchCheckSS(0,(int)(short)(bVar1 - 1)), cVar2 != '\0')) ||
           (cVar2 = ToeSynaBl_SideTouchCheckSS(1,(int)(short)(bVar1 - 2)), cVar2 != '\0')) {
          return 1;
        }
      }
    }
    return 0;
  }
  return 0;
}


// ============================================================================
// Function: ToeSynaBl_IsBaselineAbnormalSSProtectedByDim1 @ 6bb45c29
// ============================================================================

undefined8 ToeSynaBl_IsBaselineAbnormalSSProtectedByDim1(void)

{
  int iVar1;
  char cVar2;
  undefined8 uVar3;
  ushort local_12;
  int local_10;
  int local_c;
  
                    /* 0x105c29  2473  ToeSynaBl_IsBaselineAbnormalSSProtectedByDim1 */
  cVar2 = SS_IsDim1Valid();
  if ((cVar2 == '\0') || (_refptr_g_tsaPrmtFlash[2].field_0x6d0 == '\0')) {
    uVar3 = 0;
  }
  else {
    local_c = 0;
    local_10 = 0x7fffffff;
    for (local_12 = 1; (int)(uint)local_12 < (int)(_refptr_g_tsaPrmtFlash->bCols - 1);
        local_12 = local_12 + 1) {
      iVar1 = *(int *)(_refptr_g_tsaBufSsDifForBl + (longlong)(int)(uint)local_12 * 4);
      if (local_c < iVar1) {
        local_c = iVar1;
      }
      if (iVar1 < local_10) {
        local_10 = iVar1;
      }
    }
    if ((local_c < 3000) || (local_c * 2 <= -local_10)) {
      uVar3 = 0;
    }
    else {
      *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 =
           *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 | 0x200;
      uVar3 = 1;
    }
  }
  return uVar3;
}


// ============================================================================
// Function: ToeSynaBl_IsBaselineAbnormal @ 6bb4782f
// ============================================================================

void ToeSynaBl_IsBaselineAbnormal(char param_1,char param_2,undefined1 *param_3)

{
  char cVar1;
  undefined1 uVar2;
  int iVar3;
  char *pcVar4;
  
                    /* 0x10782f  2471  ToeSynaBl_IsBaselineAbnormal */
  if ((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) != 0) &&
     (cVar1 = ToeSynaBl_IsAbnormalTouchShape(param_1), cVar1 != '\0')) {
    *param_3 = 1;
  }
  if ((param_1 == '\0') && (param_2 == '\0')) {
    uVar2 = 0;
  }
  else {
    uVar2 = 1;
  }
  cVar1 = ToeSynaBl_IsSSNotMatchWithMutual(uVar2);
  if (cVar1 != '\0') {
    *param_3 = 1;
  }
  cVar1 = ToeSynaBl_GripRegionAbnormal(param_2);
  if (cVar1 != '\0') {
    *param_3 = 1;
  }
  cVar1 = ToeSynaBl_IsLongTimeSSNoSignal();
  if ((cVar1 != '\0') &&
     ((*_refptr_g_tsaStaticPtr)->pFrameBufferPtr <
      (void *)(*(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x80 + 60000))) {
    pcVar4 = (char *)TSA_GetPrmtName();
    iVar3 = strncmp(pcVar4,"P150",4);
    if (iVar3 != 0) {
      pcVar4 = (char *)TSA_GetPrmtName();
      iVar3 = strncmp(pcVar4,"P186",4);
      if (iVar3 != 0) {
        pcVar4 = (char *)TSA_GetPrmtName();
        iVar3 = strncmp(pcVar4,"P17392",6);
        if (iVar3 != 0) {
          pcVar4 = (char *)TSA_GetPrmtName();
          iVar3 = strncmp(pcVar4,"P17292",6);
          if (iVar3 != 0) {
            return;
          }
        }
      }
    }
    *param_3 = 1;
  }
  return;
}


// ============================================================================
// Function: ToeSynaBl_BackLightShiftProcess @ 6bb463d8
// ============================================================================

void ToeSynaBl_BackLightShiftProcess(void)

{
  TSAStatic *pTVar1;
  char cVar2;
  undefined4 uVar3;
  
  cVar2 = ToeSynaBl_BackLightShiftUnit(0x50);
  if (cVar2 == '\0') {
    *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3c0 = 0;
  }
  else if (*(int *)&(*_refptr_g_tsaStaticPtr)->field_0x3c0 == 0) {
    *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3c0 = 1;
    ToeSynaBl_IsRawChangedSinceBackLightShift(1);
  }
  else {
    cVar2 = ToeSynaBl_IsRawChangedSinceBackLightShift(0);
    if (cVar2 == '\0') {
      *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 =
           *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 | 0x20000;
      pTVar1 = *_refptr_g_tsaStaticPtr;
      uVar3 = BLReset_UpdateTimer(*(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3c0);
      *(undefined4 *)&pTVar1->field_0x3c0 = uVar3;
    }
    else {
      *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 =
           *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 | 0x40000;
      *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3c0 = 0;
    }
  }
  return;
}


// ============================================================================
// Function: ToeSynaBl_ClothRiskProcess @ 6bb464e6
// ============================================================================

void ToeSynaBl_ClothRiskProcess(void)

{
  if (*(short *)&_refptr_g_tsaPrpt->field_0x16e < -799) {
    *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3bc = 0;
  }
  else {
    *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 =
         *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 | 0x10000;
    if ((int)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal <
        *(int *)&(*_refptr_g_tsaStaticPtr)->field_0x3bc) {
      *(int *)&(*_refptr_g_tsaStaticPtr)->field_0x3bc =
           *(int *)&(*_refptr_g_tsaStaticPtr)->field_0x3bc -
           (int)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
    }
    else {
      *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3bc = 0;
    }
  }
  return;
}


// ============================================================================
// Function: ToeSynaBl_IsTouchToBeProtected2 @ 6bb45675
// ============================================================================

undefined1 ToeSynaBl_IsTouchToBeProtected2(undefined1 param_1,undefined1 param_2)

{
  undefined1 local_res10;
  
                    /* 0x105675  2477  ToeSynaBl_IsTouchToBeProtected2 */
  local_res10 = param_2;
  if (_refptr_g_tsaPrmtFlash[2].field_0x6d0 != '\0') {
    local_res10 = BLReset_IsTouchToBeProtected2(param_1);
  }
  return local_res10;
}


// ============================================================================
// Function: ToeSynaBl_IsWaterDetected @ 6bb45a50
// ============================================================================

undefined8 ToeSynaBl_IsWaterDetected(void)

{
  TSAStatic *pTVar1;
  bool bVar2;
  char cVar3;
  undefined4 uVar4;
  undefined8 uVar5;
  char local_1e;
  ushort local_1c;
  byte local_1a;
  byte local_19;
  
                    /* 0x105a50  2478  ToeSynaBl_IsWaterDetected */
  if ((_refptr_g_tsaPrmtFlash[2].field_0x6d0 == '\0') ||
     ((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6d4 & 2) == 0)) {
    uVar5 = 0;
  }
  else {
    local_19 = 0;
    local_1a = 0;
    for (local_1c = 2; (int)(uint)local_1c < (int)(_refptr_g_tsaPrmtFlash->bCols - 2);
        local_1c = local_1c + 1) {
      if (*(int *)(_refptr_g_tsaBufSelfForBlRst + ((longlong)(int)(uint)local_1c + 0x280) * 4) +
          0x4b0 < *(int *)(_refptr_g_tsaBufSelfForBlRst +
                          ((longlong)(int)(uint)local_1c + 0x3c0) * 4)) {
        bVar2 = false;
        for (local_1e = -1; local_1e < '\x02'; local_1e = local_1e + '\x01') {
          if ((*(int *)(_refptr_g_tsaBufSsDifForBl +
                       (longlong)(int)((uint)local_1c + (int)local_1e) * 4) < -400) ||
             (400 < *(int *)(_refptr_g_tsaBufSsDifForBl +
                            (longlong)(int)((uint)local_1c + (int)local_1e) * 4))) {
            bVar2 = true;
            break;
          }
        }
        if (bVar2) {
          local_19 = local_19 + 1;
        }
        else {
          local_1a = local_1a + 1;
        }
      }
    }
    if ((uint)local_19 + (uint)local_19 * 2 < (uint)local_1a) {
      cVar3 = ToeSynaBl_IsWaterDetectedZ8Check();
      if (cVar3 == '\0') {
        *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3b8 = 0;
      }
      else {
        pTVar1 = *_refptr_g_tsaStaticPtr;
        uVar4 = BLReset_UpdateTimer(*(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3b8);
        *(undefined4 *)&pTVar1->field_0x3b8 = uVar4;
      }
    }
    else {
      *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3b8 = 0;
    }
    if (*(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x3b8 < 0x51) {
      uVar5 = 0;
    }
    else {
      *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3b8 = 0;
      uVar5 = 1;
    }
  }
  return uVar5;
}


// ============================================================================
// Function: ToeSynaBl_SideTouchCheckSS @ 6bb456af
// ============================================================================

undefined8 ToeSynaBl_SideTouchCheckSS(ushort param_1,short param_2)

{
  bool bVar1;
  bool bVar2;
  undefined8 uVar3;
  ushort local_c;
  
  if ((param_2 < (short)param_1) || ((int)(_refptr_g_tsaPrmtFlash->bCols - 1) < (int)param_2)) {
    uVar3 = 0;
  }
  else {
    bVar1 = true;
    bVar2 = true;
    for (local_c = param_1; (int)(uint)local_c <= (int)param_2; local_c = local_c + 1) {
      if (*(int *)(_refptr_g_tsaBufSsDifForBl + (longlong)(int)(uint)local_c * 4) < 0xbb9) {
        bVar1 = false;
      }
      if (*(int *)(_refptr_g_tsaBufSsDifForBl +
                  (longlong)(int)((uint)_refptr_g_tsaPrmtFlash->bCols - (uint)local_c) * 4) < 0xbb9)
      {
        bVar2 = false;
      }
    }
    if ((bVar1) || (bVar2)) {
      uVar3 = 1;
    }
    else {
      uVar3 = 0;
    }
  }
  return uVar3;
}


// ============================================================================
// Function: ToeSynaBl_IsLongTimeSSNoSignal @ 6bb46228
// ============================================================================

bool ToeSynaBl_IsLongTimeSSNoSignal(void)

{
  char cVar1;
  bool local_9;
  
                    /* 0x106228  2474  ToeSynaBl_IsLongTimeSSNoSignal */
  local_9 = false;
  cVar1 = ToeSynaBl_SSIsDim2NoSignal();
  if ((((cVar1 == '\0') || (cVar1 = BLReset_CheckNumOverNoise(), cVar1 == '\0')) ||
      (*(short *)&_refptr_g_tsaPrpt->field_0xe0 <=
       (short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax / 2)) ||
     (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) != 0)) {
    *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3c4 = 0;
  }
  else {
    local_9 = *(int *)&(*_refptr_g_tsaStaticPtr)->field_0x3c4 < 0x12d;
    if (local_9) {
      *(int *)&(*_refptr_g_tsaStaticPtr)->field_0x3c4 =
           (int)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal +
           *(int *)&(*_refptr_g_tsaStaticPtr)->field_0x3c4;
    }
    else {
      *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3c4 = 0;
    }
    local_9 = !local_9;
    if (*(int *)&(*_refptr_g_tsaStaticPtr)->field_0x3c4 ==
        (int)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal) {
      ToeSynaBl_IsRawChangedSinceSSNoSignal(1);
    }
    else if ((int)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal <
             *(int *)&(*_refptr_g_tsaStaticPtr)->field_0x3c4) {
      cVar1 = ToeSynaBl_IsRawChangedSinceSSNoSignal(0);
      if (cVar1 == '\0') {
        *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 =
             *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 | 0x200000;
      }
      else {
        *(undefined4 *)&(*_refptr_g_tsaStaticPtr)->field_0x3c4 = 0;
      }
    }
  }
  (*_refptr_g_tsaStaticPtr)->field_0x328 = local_9;
  return local_9;
}


// ============================================================================
// Function: ToeSynaBl_IsAbnormalTouchShape @ 6bb4681a
// ============================================================================

bool ToeSynaBl_IsAbnormalTouchShape(char param_1)

{
  TSAStatic *pTVar1;
  bool bVar2;
  char cVar3;
  undefined8 uVar4;
  ushort local_1c;
  
                    /* 0x10681a  2470  ToeSynaBl_IsAbnormalTouchShape */
  bVar2 = false;
  if ((((_refptr_g_tsaPrmtFlash[2].field_0x6d0 != '\0') &&
       ((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6d4 & 8) != 0)) && (param_1 == '\0')) &&
     (cVar3 = ToeSynaBl_IsBaselineAbnormalSSProtectedByDim1(), cVar3 == '\0')) {
    for (local_1c = 0; local_1c < (*_refptr_g_prevTouchesPtr)->bPrevTouchCount;
        local_1c = local_1c + 1) {
      if (((0x10 < *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)local_1c * 0x478 + 0x1a6)) &&
          ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)local_1c * 0x478 + 0x1f8) & 0x20000000) != 0)) &&
         (((*_refptr_g_peaksPtr)
           [(longlong)
            (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                (longlong)(int)(uint)local_1c * 0x478 + 0x23c) * 0x14 + 0x15].
           bPeakCount < 2 ||
          ((int)(_refptr_g_tsaPrmtFlash->bCols - 3) <
           (int)(uint)(*_refptr_g_peaksPtr)
                      [(longlong)
                       (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                           (longlong)(int)(uint)local_1c * 0x478 + 0x23c) * 0x14 +
                       0x15].bPeakCount)))) {
        bVar2 = true;
      }
    }
    if (bVar2) {
      *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 =
           *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 | 0x40;
      pTVar1 = *_refptr_g_tsaStaticPtr;
      uVar4 = BLReset_UpdateTimer(*(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x3b0);
      *(undefined8 *)&pTVar1->field_0x3b0 = uVar4;
    }
    else {
      *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x3b0 = 0;
    }
    bVar2 = 300 < *(ulonglong *)&(*_refptr_g_tsaStaticPtr)->field_0x3b0;
    if (bVar2) {
      *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x3b0 = 0;
    }
    return bVar2;
  }
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x3b0 = 0;
  return false;
}


// ============================================================================
// Function: ToeSynaBl_IsSSNotMatchWithMutual @ 6bb46a77
// ============================================================================

bool ToeSynaBl_IsSSNotMatchWithMutual(char param_1)

{
  TSAStatic *pTVar1;
  bool bVar2;
  char cVar3;
  char cVar4;
  undefined8 uVar5;
  
                    /* 0x106a77  2475  ToeSynaBl_IsSSNotMatchWithMutual */
  bVar2 = false;
  cVar3 = TSA_IsGameScenario();
  if ((((_refptr_g_tsaPrmtFlash[2].field_0x6d0 != '\0') &&
       ((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6d4 & 4) != 0)) && (param_1 == '\0')) &&
     (cVar4 = SS_IsDim1Valid(), cVar4 != '\0')) {
    if (((cVar3 == '\0') && (2 < *(ushort *)&_refptr_g_tsaPrpt->field_0x1a2)) &&
       (cVar3 = SS_IsNotMatchWithMutual(1), cVar3 != '\0')) {
      bVar2 = true;
    }
    if (bVar2) {
      *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 =
           *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 | 0x80;
      pTVar1 = *_refptr_g_tsaStaticPtr;
      uVar5 = BLReset_UpdateTimer(*(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x3a8);
      *(undefined8 *)&pTVar1->field_0x3a8 = uVar5;
    }
    else {
      *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x3a8 = 0;
    }
    bVar2 = 300 < *(ulonglong *)&(*_refptr_g_tsaStaticPtr)->field_0x3a8;
    if (bVar2) {
      *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x3a8 = 0;
    }
    return bVar2;
  }
  *(undefined8 *)&(*_refptr_g_tsaStaticPtr)->field_0x3a8 = 0;
  return false;
}


// ============================================================================
// Function: ToeSynaBl_GripRegionAbnormal @ 6bb474ef
// ============================================================================

undefined8 ToeSynaBl_GripRegionAbnormal(char param_1)

{
  bool bVar1;
  char cVar2;
  char cVar3;
  uint uVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  ushort local_e;
  ushort local_c;
  
  if ((((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) == 0) ||
       (_refptr_g_tsaPrmtFlash->bCols < 4)) || (cVar2 = TSA_IsGameScenario(), cVar2 != '\0')) ||
     ((cVar2 = BLReset_CheckNumOverNoise(), cVar2 == '\0' || (param_1 != '\0')))) {
    *(void **)&(*_refptr_g_tsaStaticPtr)->field_0x3c8 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    return 0;
  }
  bVar1 = false;
  uVar4 = (uint)_refptr_g_tsaPrmtFlash->bCols;
  for (local_c = 1; local_c < 3; local_c = local_c + 1) {
    uVar5 = ToeSynaBl_IsNextSideDifNormal((char)local_c);
    uVar6 = ToeSynaBl_IsNextSideDifNormal((uVar4 - local_c) - 1 & 0xff);
    cVar2 = ToeSynaBl_PrptSideRawProtected(uVar5,uVar6);
    cVar3 = ToeSynaBl_PrptSideRawProtected2(uVar4,(char)local_c,uVar5,uVar6);
    if ((*(int *)(_refptr_g_tsaBufSelfForBlRst + (longlong)(int)(uint)local_c * 4) < 0x1f41) ||
       (*(int *)(_refptr_g_tsaBufSelfForBlRst + (longlong)(int)(uint)local_c * 4) <=
        *(int *)(_refptr_g_tsaBufSsDifForBl + (longlong)(int)(uint)local_c * 4) * 10)) {
      if (0x4b0 < *(short *)(_refptr_g_tsaBufSelfForBlRst +
                            ((longlong)(int)(uint)local_c + 0x460) * 2)) {
        bVar1 = false;
        break;
      }
    }
    else {
      bVar1 = true;
    }
    if ((*(int *)(_refptr_g_tsaBufSelfForBlRst + (longlong)(int)((uVar4 - 1) - (uint)local_c) * 4) <
         0x1f41) ||
       (*(int *)(_refptr_g_tsaBufSelfForBlRst + (longlong)(int)((uVar4 - 1) - (uint)local_c) * 4) <=
        *(int *)(_refptr_g_tsaBufSsDifForBl + (longlong)(int)((uVar4 - 1) - (uint)local_c) * 4) * 10
       )) {
      if (0x4b0 < *(short *)(_refptr_g_tsaBufSelfForBlRst +
                            ((longlong)(int)((uVar4 - 1) - (uint)local_c) + 0x460) * 2)) {
        bVar1 = false;
        break;
      }
    }
    else {
      bVar1 = true;
    }
    if ((cVar2 != '\0') && (cVar3 != '\0')) {
      bVar1 = false;
      break;
    }
  }
  local_e = 3;
  do {
    if ((int)(uVar4 - 3) <= (int)(uint)local_e) {
LAB_6bb4777b:
      if (bVar1) {
        if (2000 < (ulonglong)
                   ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
                   *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x3c8)) {
          *(void **)&(*_refptr_g_tsaStaticPtr)->field_0x3c8 =
               (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
          return 1;
        }
        *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 =
             *(uint *)&(*_refptr_g_tsaStaticPtr)[1].field_0x6 | 0x100000;
      }
      else {
        *(void **)&(*_refptr_g_tsaStaticPtr)->field_0x3c8 =
             (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
      }
      return 0;
    }
    if ((400 < *(short *)(_refptr_g_tsaBufSelfForBlRst + ((longlong)(int)(uint)local_e + 0x460) * 2)
        ) || (*(short *)(_refptr_g_tsaBufSelfForBlRst + ((longlong)(int)(uint)local_e + 0x460) * 2)
              < -400)) {
      bVar1 = false;
      goto LAB_6bb4777b;
    }
    local_e = local_e + 1;
  } while( true );
}


// ============================================================================
// Function: ToeSynaBl_IsRawChangedSinceBackLightShift @ 6bb46030
// ============================================================================

void ToeSynaBl_IsRawChangedSinceBackLightShift(undefined1 param_1)

{
  BLReset_IsRawChanged(g_tsaBufRawStoredSinceBackLightShift,param_1,1,1);
  return;
}


// ============================================================================
// Function: ToeSynaBl_IsWaterDetectedZ8Check @ 6bb45950
// ============================================================================

undefined1 ToeSynaBl_IsWaterDetectedZ8Check(void)

{
  ushort uVar1;
  ushort uVar2;
  char local_b;
  char local_a;
  undefined1 local_9;
  
  uVar1 = (ushort)(byte)_refptr_g_tsaPrpt->field_0x172;
  uVar2 = (ushort)(byte)_refptr_g_tsaPrpt->field_0x173;
  local_9 = 0;
  if ((((uVar1 == 0) || (uVar2 == 0)) ||
      ((int)(_refptr_g_tsaPrmtFlash->bCols - 1) <= (int)(uint)uVar1)) ||
     ((int)(_refptr_g_tsaPrmtFlash->bRows - 1) <= (int)(uint)uVar2)) {
    local_9 = 0;
  }
  else {
    for (local_a = -1; local_a < '\x02'; local_a = local_a + '\x01') {
      for (local_b = -1; local_b < '\x02'; local_b = local_b + '\x01') {
        if (-0x96 < *(short *)((ulonglong)
                               (ushort)((ushort)_refptr_g_tsaPrmtFlash->bCols *
                                        ((short)local_b + uVar2) + uVar1 + (short)local_a) * 2 +
                              *(longlong *)_refptr_g_tsaBufDif)) {
          local_9 = 1;
          break;
        }
      }
    }
  }
  return local_9;
}


// ============================================================================
// Function: ToeSynaBl_IsRawChangedSinceSSNoSignal @ 6bb46062
// ============================================================================

void ToeSynaBl_IsRawChangedSinceSSNoSignal(undefined1 param_1)

{
  BLReset_IsRawChanged(g_tsaBufRawStoredSinceDim2SSNoSignal,param_1,1,1);
  return;
}


// ============================================================================
// Function: ToeSynaBl_SSIsDim2NoSignal @ 6bb4616a
// ============================================================================

/* WARNING: Removing unreachable block (ram,0x6bb461f2) */

undefined8 ToeSynaBl_SSIsDim2NoSignal(void)

{
  byte bVar1;
  int local_10;
  byte local_9;
  
                    /* 0x10616a  2480  ToeSynaBl_SSIsDim2NoSignal */
  bVar1 = _refptr_g_tsaPrmtFlash->bRows + _refptr_g_tsaPrmtFlash->bCols;
  local_9 = _refptr_g_tsaPrmtFlash->bCols;
  while( true ) {
    if (bVar1 <= local_9) {
      return 1;
    }
    local_10 = (uint)*(ushort *)(_refptr_g_tsaPrmtDynamic + 0x2c) * 2;
    if ((local_9 == _refptr_g_tsaPrmtFlash->bCols) || ((uint)local_9 == bVar1 - 1)) {
      local_10 = (int)((uint)*(ushort *)(_refptr_g_tsaPrmtDynamic + 0x2c) * 10) >> 2;
    }
    if (local_10 < *(int *)(_refptr_g_tsaBufSsDifForBl + (longlong)(int)(uint)local_9 * 4)) break;
    local_9 = local_9 + 1;
  }
  return 0;
}


