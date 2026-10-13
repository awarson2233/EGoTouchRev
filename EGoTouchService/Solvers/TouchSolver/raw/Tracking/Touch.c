// ============================================================================
// Function: Touch_Clean @ 6ba6f49c
// ============================================================================

void Touch_Clean(void)

{
  int iVar1;
  
                    /* 0x2f49c  2515  Touch_Clean */
  iVar1 = memset_s(g_touchesPtr,0x1d60,0,0x1d60);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/Touch/TouchData.c"
                      ,0x34,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: Touch_AssignPreIdxBasedOnID @ 6ba73fc7
// ============================================================================

void Touch_AssignPreIdxBasedOnID(void)

{
  bool bVar1;
  byte local_a;
  byte local_9;
  
                    /* 0x33fc7  2514  Touch_AssignPreIdxBasedOnID */
  local_a = 0;
  do {
    if ((byte)*g_touchesPtr <= local_a) {
      return;
    }
    bVar1 = false;
    for (local_9 = 0; local_9 < (*_refptr_g_prevTouchesPtr)->bPrevTouchCount; local_9 = local_9 + 1)
    {
      if (*(char *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)local_9 * 0x478 + 0x154) ==
          g_touchesPtr[(longlong)(int)(uint)local_a * 0x168 + 0x154]) {
        bVar1 = true;
        break;
      }
    }
    if (bVar1) {
      g_touchesPtr[(longlong)(int)(uint)local_a * 0x168 + 0x287] = local_9;
    }
    else {
      g_touchesPtr[(longlong)(int)(uint)local_a * 0x168 + 0x287] = 0x14;
    }
    local_a = local_a + 1;
  } while( true );
}


// ============================================================================
// Function: Touch_KeepPrevTouchWithExceptionExcluded @ 6ba766b9
// ============================================================================

void Touch_KeepPrevTouchWithExceptionExcluded(void)

{
  char local_a;
  byte local_9;
  
                    /* 0x366b9  2544  Touch_KeepPrevTouchWithExceptionExcluded */
  g_touchesPtr[3] = (*_refptr_g_prevTouchesPtr)->field_0x3;
  g_touchesPtr[4] = (*_refptr_g_prevTouchesPtr)->field_0x4;
  *(undefined4 *)(g_touchesPtr + 0x18) = *(undefined4 *)&(*_refptr_g_prevTouchesPtr)->field_0x18;
  *(undefined4 *)(g_touchesPtr + 0x1c) = *(undefined4 *)&(*_refptr_g_prevTouchesPtr)->field_0x1c;
  g_touchesPtr[1] = (*_refptr_g_prevTouchesPtr)->field_0x1;
  g_touchesPtr[2] = (*_refptr_g_prevTouchesPtr)->field_0x2;
  *(undefined2 *)(g_touchesPtr + 0x14) = *(undefined2 *)&(*_refptr_g_prevTouchesPtr)->field_0x14;
  g_touchesPtr[5] = (*_refptr_g_prevTouchesPtr)->field_0x5;
  g_touchesPtr[7] = (*_refptr_g_prevTouchesPtr)->field_0x7;
  g_touchesPtr[0xb] = (*_refptr_g_prevTouchesPtr)->field_0xb;
  g_touchesPtr[0xc] = (*_refptr_g_prevTouchesPtr)->field_0xc;
  g_touchesPtr[0xd] = (*_refptr_g_prevTouchesPtr)->field_0xd;
  g_touchesPtr[0xe] = (*_refptr_g_prevTouchesPtr)->field_0xe;
  g_touchesPtr[0xf] = (*_refptr_g_prevTouchesPtr)->field_0xf;
  g_touchesPtr[0x10] = (*_refptr_g_prevTouchesPtr)->field_0x10;
  g_touchesPtr[0x11] = (*_refptr_g_prevTouchesPtr)->field_0x11;
  g_touchesPtr[0x12] = (*_refptr_g_prevTouchesPtr)->field_0x12;
  g_touchesPtr[0x13] = (*_refptr_g_prevTouchesPtr)->field_0x13;
  HardwareAnalyzer_ReportToTSA();
  (*_refptr_g_prevTouchesPtr)->nTouchCount = *(uint *)(g_touchesPtr + 0x20) & 0xfffffbff;
  *(undefined4 *)(g_touchesPtr + 0x24) = *(undefined4 *)(*_refptr_g_prevTouchesPtr + 1);
  *(undefined8 *)(g_touchesPtr + 0x38) = *(undefined8 *)&(*_refptr_g_prevTouchesPtr)[1].field_0x14;
  *(undefined4 *)(g_touchesPtr + 0x138) = *(undefined4 *)&(*_refptr_g_prevTouchesPtr)[8].field_0x18;
  Thenar_UnitCopy(g_touchesPtr + 0x40,&(*_refptr_g_prevTouchesPtr)[1].field_0x1c);
  local_a = '\0';
  for (local_9 = 0; local_9 < (*_refptr_g_prevTouchesPtr)->bPrevTouchCount; local_9 = local_9 + 1) {
    Touch_UnitCopy(local_a,local_9);
    local_a = local_a + '\x01';
  }
  *g_touchesPtr = (*_refptr_g_prevTouchesPtr)->bPrevTouchCount;
  return;
}


// ============================================================================
// Function: Touch_ProcessBetaGrip @ 6ba76ee8
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

void Touch_ProcessBetaGrip(void)

{
  byte bPrevTouchIndex;
  int nBetaGripState;
  byte bTouchIndex;
  
                    /* 0x36ee8  2547  Touch_ProcessBetaGrip */
  for (bTouchIndex = 0; bTouchIndex < (byte)*g_touchesPtr; bTouchIndex = bTouchIndex + 1) {
    bPrevTouchIndex = PrevTouch_GetIdxBasedOnCurIdx(bTouchIndex);
    nBetaGripState = Touch_IsBetaGrip(bTouchIndex,bPrevTouchIndex);
    if (nBetaGripState == 4) {
      *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                (longlong)(int)(uint)bPrevTouchIndex * 0x478 + 0x4e8) =
           *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)bPrevTouchIndex * 0x478 + 0x4e8) + 1;
      if ((1 < *(ushort *)
                ((longlong)*_refptr_g_prevTouchesPtr +
                (longlong)(int)(uint)bPrevTouchIndex * 0x478 + 0x4e8)) &&
         (*(uint *)(_refptr_g_tsaInitOnce + 0x10) < 0xfffffffe)) {
        *(uint *)(g_touchesPtr + (longlong)(int)(uint)bTouchIndex * 0x168 + 0x1f8) =
             *(uint *)(g_touchesPtr + (longlong)(int)(uint)bTouchIndex * 0x168 + 0x1f8) | 0x1000000;
        *(int *)(_refptr_g_tsaInitOnce + 0x10) = *(int *)(_refptr_g_tsaInitOnce + 0x10) + 1;
      }
    }
    else if (nBetaGripState == 8) {
      *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                (longlong)(int)(uint)bPrevTouchIndex * 0x478 + 0x4e8) =
           *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)bPrevTouchIndex * 0x478 + 0x4e8) + 1;
      if ((1 < *(ushort *)
                ((longlong)*_refptr_g_prevTouchesPtr +
                (longlong)(int)(uint)bPrevTouchIndex * 0x478 + 0x4e8)) &&
         (*(uint *)(_refptr_g_tsaInitOnce + 0x14) < 0xfffffffe)) {
        *(uint *)(g_touchesPtr + (longlong)(int)(uint)bTouchIndex * 0x168 + 0x1f8) =
             *(uint *)(g_touchesPtr + (longlong)(int)(uint)bTouchIndex * 0x168 + 0x1f8) | 0x1000000;
        *(int *)(_refptr_g_tsaInitOnce + 0x14) = *(int *)(_refptr_g_tsaInitOnce + 0x14) + 1;
      }
    }
    else {
      *(undefined2 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bPrevTouchIndex * 0x478 + 0x4e8)
           = 0;
    }
  }
  return;
}


// ============================================================================
// Function: Touch_ProcessExtInfo @ 6ba7713d
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* Adds report extension flags for water/proximity/posture/anti-touch state. UnderWater detection
   feature is off in current config, but the report flag plumbing remains active. */

void Touch_ProcessExtInfo(void)

{
  char cConditionResult;
  int nDetectedState;
  
                    /* 0x3713d  2548  Touch_ProcessExtInfo */
  nDetectedState = TSA_IsUnderWaterDetected();
  if (nDetectedState != 0) {
    *(uint *)(g_touchesPtr + 0x18) = *(uint *)(g_touchesPtr + 0x18) | 0x20;
  }
  nDetectedState = TSA_IsOutOfWaterDetected();
  if (nDetectedState != 0) {
    *(uint *)(g_touchesPtr + 0x18) = *(uint *)(g_touchesPtr + 0x18) | 0x40;
  }
  cConditionResult = Proximity_IsDetected();
  if (cConditionResult != '\0') {
    *(uint *)(g_touchesPtr + 0x18) = *(uint *)(g_touchesPtr + 0x18) | 0x80;
  }
  cConditionResult =
       SensorGesture_IsProximityPickUpGestureTimeOut
                 (*(undefined4 *)&_refptr_g_tsaPrmtFlash[2].bRawdataForceNormalize);
  if (cConditionResult == '\0') {
    *(uint *)(g_touchesPtr + 0x18) = *(uint *)(g_touchesPtr + 0x18) | 0x100;
  }
  cConditionResult =
       SensorGesture_IsProximityPutDownGestureTimeOut
                 (*(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x608);
  if (cConditionResult == '\0') {
    *(uint *)(g_touchesPtr + 0x18) = *(uint *)(g_touchesPtr + 0x18) | 0x200;
  }
  Posture_UpdatePostureType();
  AntiTouch_Report();
  return;
}


// ============================================================================
// Function: Touch_GetFormattedTimeInterval @ 6ba76329
// ============================================================================

short Touch_GetFormattedTimeInterval(short param_1)

{
  char cVar1;
  short local_a;
  
                    /* 0x36329  2520  Touch_GetFormattedTimeInterval */
  if (param_1 == 0) {
    if (_refptr_g_tsaPrmtFlash->field_0x38 == '\0') {
      local_a = 8;
    }
    else {
      local_a = (short)(1000 / (ulonglong)
                               (longlong)(int)(uint)(byte)_refptr_g_tsaPrmtFlash->field_0x38);
    }
  }
  else if (param_1 < 1) {
    if ((*_refptr_g_tsaStaticPtr)->nFrameCounter == 0) {
      local_a = 0;
    }
    else {
      if ((param_1 < -7) && (cVar1 = TSAStatic_IsAFEJustEnterIDLEMode(), cVar1 == '\0')) {
        if (-0x3e9 < param_1) {
          return param_1;
        }
        return -1000;
      }
      local_a = -8;
    }
  }
  else {
    if ((7 < param_1) && (cVar1 = TSAStatic_IsAFEJustExitIDLEMode(), cVar1 == '\0')) {
      if (param_1 < 0x3e9) {
        return param_1;
      }
      return 1000;
    }
    local_a = 8;
  }
  return local_a;
}


// ============================================================================
// Function: Touch_UnitRemove @ 6ba763f4
// ============================================================================

void Touch_UnitRemove(byte param_1)

{
  errno_t eVar1;
  byte local_9;
  
                    /* 0x363f4  2552  Touch_UnitRemove */
  if (0x14 < (byte)*g_touchesPtr) {
    if (0x14 < (byte)*g_touchesPtr) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Alg/Touch/TouchData.c",0x73b,
                        "TouchesPrpt(num) <= TSA_MAX_TOUCH_NUM");
      HardwareAnalyzer_SetSoftException();
    }
    *g_touchesPtr = 0x14;
  }
  if (param_1 < (byte)*g_touchesPtr) {
    for (local_9 = param_1; (int)(uint)local_9 < (int)((byte)*g_touchesPtr - 1);
        local_9 = local_9 + 1) {
      eVar1 = memcpy_s(g_touchesPtr + (longlong)(int)(uint)local_9 * 0x168 + 0x140,0x168,
                       g_touchesPtr + (longlong)(int)(local_9 + 1) * 0x168 + 0x140,0x168);
      if (eVar1 != 0) {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                          "../../src/Alg/Touch/TouchData.c",0x743,"rc == EOK");
        HardwareAnalyzer_SetSoftException();
      }
    }
    *g_touchesPtr = *g_touchesPtr + -1;
  }
  return;
}


// ============================================================================
// Function: Touch_SetEdgeDist @ 6ba72b19
// ============================================================================

void Touch_SetEdgeDist(byte param_1)

{
  undefined *puVar1;
  undefined2 uVar2;
  
                    /* 0x32b19  2549  Touch_SetEdgeDist */
  puVar1 = g_touchesPtr;
  uVar2 = Touch_GetDim1EdgeDist
                    (*(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x162));
  *(undefined2 *)(puVar1 + (longlong)(int)(uint)param_1 * 0x168 + 0x22c) = uVar2;
  puVar1 = g_touchesPtr;
  uVar2 = Touch_GetDim2EdgeDist
                    (*(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x164));
  *(undefined2 *)(puVar1 + (longlong)(int)(uint)param_1 * 0x168 + 0x22e) = uVar2;
  return;
}


// ============================================================================
// Function: Touch_UnitCopy @ 6ba74462
// ============================================================================

void Touch_UnitCopy(byte param_1,byte param_2)

{
  undefined1 uVar1;
  undefined1 uVar2;
  undefined1 uVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  errno_t eVar6;
  byte local_res8;
  byte local_res10;
  
                    /* 0x34462  2551  Touch_UnitCopy */
  if (0x13 < param_2) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/Touch/TouchData.c"
                      ,0x527,"preIdx < TSA_MAX_TOUCH_NUM");
    HardwareAnalyzer_SetSoftException();
  }
  if (0x13 < param_1) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/Touch/TouchData.c"
                      ,0x528,"curIdx < TSA_MAX_TOUCH_NUM");
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
  uVar4 = *(undefined4 *)(g_touchesPtr + (longlong)(int)(uint)local_res8 * 0x168 + 0x1d4);
  uVar1 = g_touchesPtr[(longlong)(int)(uint)local_res8 * 0x168 + 0x1dc];
  uVar2 = g_touchesPtr[(longlong)(int)(uint)local_res8 * 0x168 + 0x1dd];
  uVar3 = g_touchesPtr[(longlong)(int)(uint)local_res8 * 0x168 + 0x1de];
  uVar5 = *(undefined4 *)(g_touchesPtr + (longlong)(int)(uint)local_res8 * 0x168 + 0x200);
  eVar6 = memcpy_s(g_touchesPtr + (longlong)(int)(uint)local_res8 * 0x168 + 0x140,0x144,
                   (void *)((longlong)*_refptr_g_prevTouchesPtr +
                           (longlong)(int)(uint)local_res10 * 0x478 + 0x140),0x144);
  if (eVar6 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/Touch/TouchData.c"
                      ,0x53f,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  *(undefined4 *)(g_touchesPtr + (longlong)(int)(uint)local_res8 * 0x168 + 0x1d4) = uVar4;
  g_touchesPtr[(longlong)(int)(uint)local_res8 * 0x168 + 0x1dc] = uVar1;
  g_touchesPtr[(longlong)(int)(uint)local_res8 * 0x168 + 0x1dd] = uVar2;
  g_touchesPtr[(longlong)(int)(uint)local_res8 * 0x168 + 0x1de] = uVar3;
  *(undefined4 *)(g_touchesPtr + (longlong)(int)(uint)local_res8 * 0x168 + 0x200) = uVar5;
  return;
}


// ============================================================================
// Function: Touch_IsBetaGrip @ 6ba76d53
// ============================================================================

undefined8 Touch_IsBetaGrip(byte param_1,byte param_2)

{
  short sVar1;
  int iVar2;
  bool bVar3;
  char cVar4;
  char cVar5;
  ushort uVar6;
  ushort uVar7;
  ushort uVar8;
  undefined8 uVar9;
  
  cVar4 = TSAStatic_IsChargerPlugged();
  sVar1 = *(short *)(*_refptr_g_peaksPtr +
                    (longlong)
                    (int)(uint)(byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x23c] *
                    0x14 + 0x18);
  uVar6 = _refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold;
  iVar2 = *(int *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x21c);
  cVar5 = Touch_IsException(param_1,param_2);
  bVar3 = cVar5 == '\0' && (iVar2 == 4 && ((short)uVar6 / 2 <= sVar1 && cVar4 == '\0'));
  uVar6 = TSAOut_DisplayXPos(param_1);
  if (8 < uVar6) {
    uVar6 = TSAOut_DisplayXPos(param_1);
    uVar7 = TSA_GetXRes();
    if ((int)(uint)uVar6 < (int)(uVar7 - 9)) {
      bVar3 = false;
    }
  }
  uVar6 = TSAOut_DisplayYPos(param_1);
  uVar7 = TSA_GetYRes();
  uVar8 = TS_MMToDisplayPixel(0xf);
  if ((int)(uint)uVar6 < (int)((uVar7 - 1) - (uint)uVar8)) {
    bVar3 = false;
  }
  if ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x1f8)
      & 0x1000000) != 0) {
    bVar3 = false;
  }
  if (bVar3) {
    uVar6 = TSAOut_DisplayXPos(param_1);
    if (uVar6 < 9) {
      uVar9 = 4;
    }
    else {
      uVar9 = 8;
    }
  }
  else {
    uVar9 = 0;
  }
  return uVar9;
}


// ============================================================================
// Function: Touch_GetIdxBasedOnPreIdx @ 6ba74433
// ============================================================================

undefined1 Touch_GetIdxBasedOnPreIdx(byte param_1)

{
                    /* 0x34433  2521  Touch_GetIdxBasedOnPreIdx */
  return *(undefined1 *)
          ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x4f2);
}


// ============================================================================
// Function: Touch_ProcessAftTouchEvent @ 6ba75af5
// ============================================================================

void Touch_ProcessAftTouchEvent(void)

{
  byte local_9;
  
                    /* 0x35af5  2546  Touch_ProcessAftTouchEvent */
  g_touchesPtr[8] = 0;
  g_touchesPtr[10] = 0;
  *(undefined8 *)(g_touchesPtr + 0x28) = *(undefined8 *)&(*_refptr_g_prevTouchesPtr)[1].field_0x4;
  *(undefined8 *)(g_touchesPtr + 0x30) = *(undefined8 *)&(*_refptr_g_prevTouchesPtr)[1].field_0xc;
  for (local_9 = 0; local_9 < (byte)*g_touchesPtr; local_9 = local_9 + 1) {
    Touch_ProcessAftTouchEventUnit(local_9);
  }
  return;
}


// ============================================================================
// Function: Touch_IsOff @ 6ba74ab5
// ============================================================================

void Touch_IsOff(byte param_1)

{
                    /* 0x34ab5  2539  Touch_IsOff */
  Touch_IsOffEvent(*(undefined4 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x21c));
  return;
}


// ============================================================================
// Function: Touch_GetDim1EdgeDist @ 6ba72a59
// ============================================================================

uint Touch_GetDim1EdgeDist(ushort param_1)

{
  uint uVar1;
  
  if ((int)((uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2c) - (uint)param_1) <
      (int)((uint)param_1 - (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2a))) {
    uVar1 = (uint)(ushort)(*(short *)(_refptr_g_tsaPrmtFlashConst + 0x2c) - param_1);
  }
  else {
    uVar1 = (uint)param_1 - (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2a);
  }
  return uVar1;
}


// ============================================================================
// Function: Touch_GetDim2EdgeDist @ 6ba72ab9
// ============================================================================

uint Touch_GetDim2EdgeDist(ushort param_1)

{
  uint uVar1;
  
  if ((int)((uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x30) - (uint)param_1) <
      (int)((uint)param_1 - (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2e))) {
    uVar1 = (uint)(ushort)(*(short *)(_refptr_g_tsaPrmtFlashConst + 0x30) - param_1);
  }
  else {
    uVar1 = (uint)param_1 - (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2e);
  }
  return uVar1;
}


// ============================================================================
// Function: Touch_IsException @ 6ba7416a
// ============================================================================

undefined1 Touch_IsException(byte param_1,byte param_2)

{
  undefined1 local_9;
  
                    /* 0x3416a  2534  Touch_IsException */
  local_9 = 0;
  if ((param_1 != 0x14) &&
     (*(int *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1ec) != 0)) {
    local_9 = 1;
  }
  if ((param_2 != 0x14) &&
     (*(int *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x1ec)
      != 0)) {
    local_9 = 1;
  }
  if (*(int *)(g_touchesPtr + 0x20) != 0) {
    local_9 = 1;
  }
  if ((*_refptr_g_prevTouchesPtr)->nTouchCount != 0) {
    local_9 = 1;
  }
  return local_9;
}


// ============================================================================
// Function: Touch_ProcessAftTouchEventUnit @ 6ba7542a
// ============================================================================

void Touch_ProcessAftTouchEventUnit(byte param_1)

{
  byte bVar1;
  char cVar2;
  
  bVar1 = g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x287];
  Touch_CheckTrustabilityUnit(param_1,bVar1);
  Touch_SetTrustableTag(param_1);
  cVar2 = Touch_IsAbnormalShape(param_1);
  if ((cVar2 != '\0') ||
     ((*(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x200) & 4) != 0)) {
    g_touchesPtr[0x10] = g_touchesPtr[0x10] + '\x01';
  }
  cVar2 = Touch_IsAbnormalShape2(param_1);
  if (cVar2 != '\0') {
    g_touchesPtr[0x11] = g_touchesPtr[0x11] + '\x01';
  }
  cVar2 = Touch_IsAbnormalShape3(param_1);
  if (cVar2 != '\0') {
    g_touchesPtr[0x12] = g_touchesPtr[0x12] + '\x01';
    *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x200) =
         *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x200) | 4;
  }
  cVar2 = Touch_IsAbnormalShape4(param_1);
  if (cVar2 != '\0') {
    g_touchesPtr[0x13] = g_touchesPtr[0x13] + '\x01';
  }
  if ((*(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 500) & 0x1000) != 0) {
    cVar2 = Touch_IsOff(param_1);
    if (cVar2 == '\0') {
      g_touchesPtr[8] = g_touchesPtr[8] + '\x01';
      g_touchesPtr[9] = g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x154];
      if (((*(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x218) & 1) != 0) ||
         ((*(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x214) & 0x200000) != 0))
      {
        g_touchesPtr[10] = g_touchesPtr[10] + '\x01';
      }
      if (*(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x21c) < 3) {
        *(void **)(g_touchesPtr + 0x28) = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
      }
    }
    else {
      *(void **)(g_touchesPtr + 0x30) = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    }
  }
  cVar2 = Touch_IsLiftOffDueToMerged(param_1,bVar1);
  if (cVar2 != '\0') {
    *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) =
         *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) | 0x1000000;
    cVar2 = GripFilter_IsGripOrTbdTouch(param_1);
    if (cVar2 != '\0') {
      cVar2 = PrevPeak_IsOnOuterEdge(g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x23c]);
      if (cVar2 != '\0') {
        Touch_GetMergedInfo(param_1);
      }
    }
  }
  cVar2 = Touch_IsTouchDownDueToSeperated(param_1);
  if (cVar2 != '\0') {
    *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) =
         *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) | 0x2000000;
  }
  if (((*(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x214) & 0x100000) == 0) &&
     ((*(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 500) & 0x2000) != 0)) {
    *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x214) =
         *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x214) | 0x100000;
  }
  cVar2 = Touch_IsSideOverCore(param_1,bVar1);
  if (cVar2 != '\0') {
    *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) =
         *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) | 0x400000;
  }
  cVar2 = Touch_IsCoreOffInSide(param_1,bVar1);
  if (cVar2 != '\0') {
    *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) =
         *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) | 0x80000000;
  }
  Touch_IIRDataProcess(param_1,bVar1);
  if (((*_refptr_g_tsaStaticPtr)->dwProcessingState & PROC_MS_ACTIVE) == 0) {
    if (*(ushort *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x228) <
        *(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x22a)) {
      *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x228) =
           *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x22a);
    }
    else {
      *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x228) =
           *(undefined2 *)
            ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x228);
    }
  }
  else if ((*(ushort *)
             ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x228) <
            *(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x22a)) ||
          (*(int *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x21c) == 2)) {
    *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x228) =
         *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x22a);
  }
  else {
    *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x228) =
         *(undefined2 *)
          ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)bVar1 * 0x478 + 0x228);
  }
  return;
}


// ============================================================================
// Function: Touch_UnitUpdate @ 6ba760da
// ============================================================================

void Touch_UnitUpdate(byte param_1,byte param_2)

{
  undefined *puVar1;
  undefined2 uVar2;
  int iVar3;
  
                    /* 0x360da  2553  Touch_UnitUpdate */
  iVar3 = *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_2 * 0x478 + 0x1a2) * 7 +
          (int)*(short *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1a0);
  if (iVar3 < 0) {
    iVar3 = iVar3 + 7;
  }
  *(short *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1a2) = (short)(iVar3 >> 3);
  *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x23a) =
       *(undefined2 *)
        ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x23a);
  *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x152) =
       *(undefined2 *)
        ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x150);
  puVar1 = g_touchesPtr;
  uVar2 = Touch_GetFormattedTimeInterval
                    ((int)(short)((short)*(undefined8 *)
                                          (g_touchesPtr +
                                          (longlong)(int)(uint)param_1 * 0x168 + 0x148) -
                                 (short)*(undefined8 *)
                                         ((longlong)*_refptr_g_prevTouchesPtr +
                                         (longlong)(int)(uint)param_2 * 0x478 + 0x148)));
  *(undefined2 *)(puVar1 + (longlong)(int)(uint)param_1 * 0x168 + 0x150) = uVar2;
  Touch_GetSQDistMMFromPrevPos(param_1,param_2);
  Touch_GetSQDistPixelFromPrevPos(param_1,param_2);
  Touch_GetSQDistMMFromStartPos(param_1,param_2);
  Touch_GetSQAccMMFromPrevPos(param_1,param_2);
  Touch_SetEdgeFlag(param_1);
  Touch_SetRegion(param_1);
  Touch_SetVolatileFlag(param_1,param_2);
  Touch_SetPersistFlag(param_1,param_2);
  Touch_SetSuppressFlag(param_1,param_2);
  Touch_UnitUpdateMaxMin(param_1,param_2);
  Touch_UnitUpdateSqDistMMFromStartPosMax(param_1,param_2);
  Touch_UpdateGlobalState(param_1,param_2);
  return;
}


// ============================================================================
// Function: Touch_GetRptSQDistMMFromStartPos @ 6ba739e1
// ============================================================================

void Touch_GetRptSQDistMMFromStartPos(byte param_1,byte param_2)

{
  undefined *puVar1;
  undefined2 uVar2;
  
                    /* 0x339e1  2525  Touch_GetRptSQDistMMFromStartPos */
  puVar1 = g_touchesPtr;
  uVar2 = Touch_GetSQDistMM(*(undefined2 *)
                             (g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x170),
                            *(undefined2 *)
                             (g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x172),
                            *(undefined2 *)
                             ((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)param_2 * 0x478 + 0x49a),
                            *(undefined2 *)
                             ((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)param_2 * 0x478 + 0x49c));
  *(undefined2 *)(puVar1 + (longlong)(int)(uint)param_1 * 0x168 + 0x250) = uVar2;
  return;
}


// ============================================================================
// Function: Touch_GetRptSQDistMMFromPrevPos @ 6ba73ac1
// ============================================================================

void Touch_GetRptSQDistMMFromPrevPos(byte param_1,byte param_2)

{
  undefined *puVar1;
  undefined2 uVar2;
  
                    /* 0x33ac1  2524  Touch_GetRptSQDistMMFromPrevPos */
  puVar1 = g_touchesPtr;
  uVar2 = Touch_GetSQDistMM(*(undefined2 *)
                             (g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x170),
                            *(undefined2 *)
                             (g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x172),
                            *(undefined2 *)
                             ((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)param_2 * 0x478 + 0x170),
                            *(undefined2 *)
                             ((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)param_2 * 0x478 + 0x172));
  *(undefined2 *)(puVar1 + (longlong)(int)(uint)param_1 * 0x168 + 0x252) = uVar2;
  return;
}


// ============================================================================
// Function: Touch_GetRptSQDistDisplayPixelFromPrevPos @ 6ba73ba1
// ============================================================================

void Touch_GetRptSQDistDisplayPixelFromPrevPos(byte param_1,byte param_2)

{
  ushort uVar1;
  ushort uVar2;
  ushort uVar3;
  ushort uVar4;
  
                    /* 0x33ba1  2523  Touch_GetRptSQDistDisplayPixelFromPrevPos */
  uVar1 = TSAOut_Pixel2DisplayDim1
                    (*(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x170));
  uVar2 = TSAOut_Pixel2DisplayDim1
                    (*(undefined2 *)
                      ((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_2 * 0x478 + 0x170));
  uVar3 = TSAOut_Pixel2DisplayDim2
                    (*(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x172));
  uVar4 = TSAOut_Pixel2DisplayDim2
                    (*(undefined2 *)
                      ((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_2 * 0x478 + 0x172));
  *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x254) =
       ((uint)uVar3 - (uint)uVar4) * ((uint)uVar3 - (uint)uVar4) +
       ((uint)uVar1 - (uint)uVar2) * ((uint)uVar1 - (uint)uVar2);
  return;
}


// ============================================================================
// Function: Touch_GetFltSQDistPixelFromPrevPos @ 6ba73cc1
// ============================================================================

void Touch_GetFltSQDistPixelFromPrevPos(byte param_1,byte param_2)

{
  undefined *puVar1;
  undefined4 uVar2;
  
                    /* 0x33cc1  2519  Touch_GetFltSQDistPixelFromPrevPos */
  puVar1 = g_touchesPtr;
  uVar2 = Touch_GetSQDistPixel
                    (*(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x170),
                     *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x172),
                     *(undefined2 *)
                      ((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_2 * 0x478 + 0x170),
                     *(undefined2 *)
                      ((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_2 * 0x478 + 0x172));
  *(undefined4 *)(puVar1 + (longlong)(int)(uint)param_1 * 0x168 + 600) = uVar2;
  return;
}


// ============================================================================
// Function: Touch_IsOffEvent @ 6ba74a8e
// ============================================================================

undefined8 Touch_IsOffEvent(int param_1)

{
  undefined8 uVar1;
  
                    /* 0x34a8e  2540  Touch_IsOffEvent */
  if (((param_1 == 0x20) || (param_1 == 8)) || (param_1 == 0x10)) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
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
// Function: Touch_CreatePostReport @ 6ba76a03
// ============================================================================

void Touch_CreatePostReport(byte param_1)

{
  ushort uVar1;
  ushort uVar2;
  byte bVar3;
  short local_38;
  short local_36;
  longlong local_30;
  int local_28;
  int local_24;
  int local_20;
  uint local_1c;
  uint local_18;
  byte local_12;
  byte local_11;
  uint local_10;
  uint local_c;
  
                    /* 0x36a03  2516  Touch_CreatePostReport */
  local_12 = PrevTouch_GetIdxBasedOnCurIdx(param_1);
  if ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_12 * 0x478 + 0x1f8
                ) & 0x20020000) == 0) {
    *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f8) =
         *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f8) | 0x20000;
    uVar1 = *(ushort *)
             ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_12 * 0x478 + 0x496);
    uVar2 = *(ushort *)
             ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_12 * 0x478 + 0x498);
    if ((*(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f8) & 8) != 0) {
      uVar1 = *(ushort *)
               ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_12 * 0x478 + 0x49a)
      ;
      uVar2 = *(ushort *)
               ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_12 * 0x478 + 0x49c)
      ;
    }
    local_c = (uint)uVar1;
    local_10 = (uint)uVar2;
    local_18 = (uint)*(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x162);
    local_1c = (uint)*(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x164);
    bVar3 = Touch_GetPostReportLength
                      (local_c,local_10,local_18,local_1c,
                       *(char *)((longlong)*_refptr_g_prevTouchesPtr +
                                (longlong)(int)(uint)local_12 * 0x478 + 0x288) + '\x01');
    local_20 = (int)(local_18 - local_c) / (int)(bVar3 - 1);
    local_24 = (int)(local_1c - local_10) / (int)(bVar3 - 1);
    local_28 = (int)*(undefined8 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x148) -
               (int)*(undefined8 *)
                     ((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)local_12 * 0x478 + 0x148);
    PrevTouch_HistoryBufferClean(local_12);
    for (local_11 = 0; (int)(uint)local_11 < (int)(bVar3 - 1); local_11 = local_11 + 1) {
      local_38 = (ushort)local_11 * (short)local_20 + (short)local_c;
      local_36 = (ushort)local_11 * (short)local_24 + (short)local_10;
      local_30 = *(longlong *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x148) -
                 (longlong)(int)((uint)local_11 * local_28);
      PrevTouch_HistoryBufferPush(local_12,&local_38);
    }
    *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f8) =
         *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f8) | 0x80;
  }
  return;
}


// ============================================================================
// Function: Touch_GetSQDistMM @ 6ba734be
// ============================================================================

undefined2
Touch_GetSQDistMM(undefined2 param_1,undefined2 param_2,undefined2 param_3,undefined2 param_4)

{
  undefined2 uVar1;
  undefined4 uVar2;
  
                    /* 0x334be  2526  Touch_GetSQDistMM */
  uVar2 = Touch_GetSQDistPixel(param_1,param_2,param_3,param_4);
  uVar1 = TS_SQPixelToSQMM(uVar2);
  return uVar1;
}


// ============================================================================
// Function: Touch_ThenarFeatureReset @ 6bb30bf0
// ============================================================================

void Touch_ThenarFeatureReset(void)

{
  *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0x130) = 0;
  *(undefined2 *)(*(longlong *)_refptr_g_touchesPtr + 0x70) = 0;
  *(undefined2 *)(*(longlong *)_refptr_g_touchesPtr + 0x6c) = 0;
  *(undefined2 *)(*(longlong *)_refptr_g_touchesPtr + 0x68) = 0;
  *(undefined2 *)(*(longlong *)_refptr_g_touchesPtr + 0x6a) = 0;
  return;
}


// ============================================================================
// Function: Touch_GetDistToLogicEdge @ 6ba73009
// ============================================================================

short Touch_GetDistToLogicEdge(int param_1,short param_2,short param_3)

{
  short local_a;
  
                    /* 0x33009  2518  Touch_GetDistToLogicEdge */
  local_a = 0;
  if (param_1 == 1) {
    local_a = param_2 - *(short *)(_refptr_g_tsaPrmtFlashConst + 0x2a);
  }
  if (param_1 == 2) {
    local_a = (*(short *)(_refptr_g_tsaPrmtFlashConst + 0x2c) - param_2) + -1;
  }
  if (param_1 == 4) {
    local_a = param_3 - *(short *)(_refptr_g_tsaPrmtFlashConst + 0x2e);
  }
  if (param_1 == 8) {
    local_a = (*(short *)(_refptr_g_tsaPrmtFlashConst + 0x30) - param_3) + -1;
  }
  return local_a;
}


// ============================================================================
// Function: Touch_GetSQDistPixelToEdge @ 6ba7329a
// ============================================================================

void Touch_GetSQDistPixelToEdge(short param_1,short param_2,short param_3,short param_4)

{
  short local_c;
  short local_a;
  
                    /* 0x3329a  2529  Touch_GetSQDistPixelToEdge */
  local_c = param_2;
  local_a = param_1;
  while (((short)(param_3 - param_1) != 0 && ((short)(param_4 - param_2) != 0))) {
    local_a = local_a - (param_3 - param_1);
    local_c = local_c - (param_4 - param_2);
    if ((int)local_a < (int)(uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2a)) {
      local_a = *(short *)(_refptr_g_tsaPrmtFlashConst + 0x2a);
    }
    if ((int)local_c < (int)(uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2e)) {
      local_c = *(short *)(_refptr_g_tsaPrmtFlashConst + 0x2e);
    }
    if ((int)(uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2c) <= (int)local_a) {
      local_a = *(short *)(_refptr_g_tsaPrmtFlashConst + 0x2c) + -1;
    }
    if ((int)(uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x30) <= (int)local_c) {
      local_c = *(short *)(_refptr_g_tsaPrmtFlashConst + 0x30) + -1;
    }
    if (((((int)local_a == (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2a)) ||
         ((int)local_c == (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2e))) ||
        ((int)local_a == *(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2c) - 1)) ||
       ((int)local_c == *(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x30) - 1)) break;
  }
  Touch_GetSQDistPixel(local_a,local_c,param_1,param_2);
  return;
}


// ============================================================================
// Function: Touch_GetSQDistPixel @ 6ba73447
// ============================================================================

uint Touch_GetSQDistPixel(ushort param_1,ushort param_2,ushort param_3,ushort param_4)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  uint local_c;
  
                    /* 0x33447  2528  Touch_GetSQDistPixel */
  uVar3 = (int)((uint)param_1 - (uint)param_3) >> 0x1f;
  iVar1 = ((uint)param_1 - (uint)param_3 ^ uVar3) - uVar3;
  uVar3 = (int)((uint)param_2 - (uint)param_4) >> 0x1f;
  iVar2 = ((uint)param_2 - (uint)param_4 ^ uVar3) - uVar3;
  local_c = iVar2 * iVar2 + iVar1 * iVar1;
  if (local_c < (uint)(iVar1 * iVar1)) {
    local_c = 0xffffffff;
  }
  return local_c;
}


// ============================================================================
// Function: Touch_IsSideOverCore @ 6ba714db
// ============================================================================

undefined1 Touch_IsSideOverCore(byte param_1,byte param_2)

{
  char cVar1;
  byte local_a;
  undefined1 local_9;
  
  local_9 = 0;
  if (*(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x156)
      == 0) {
    for (local_a = 0; local_a < (byte)*_refptr_g_touchesSide; local_a = local_a + 1) {
      cVar1 = Touch_IsSideCoreInSameZone(param_1,local_a);
      if (((cVar1 != '\0') &&
          (0x50 < *(uint *)(_refptr_g_touchesSide + (longlong)(int)(uint)local_a * 0x168 + 0x158)))
         && (*(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x22c) < 0x101)) {
        local_9 = 1;
      }
    }
  }
  return local_9;
}


// ============================================================================
// Function: Touch_IsCoreOffInSide @ 6ba715a8
// ============================================================================

undefined1 Touch_IsCoreOffInSide(undefined1 param_1)

{
  char cVar1;
  byte local_a;
  undefined1 local_9;
  
  local_9 = 0;
  cVar1 = Touch_IsOff(param_1);
  if (cVar1 != '\0') {
    for (local_a = 0; local_a < (byte)*_refptr_g_touchesSide; local_a = local_a + 1) {
      cVar1 = Touch_IsSideCoreInSameZone(param_1,local_a);
      if (cVar1 != '\0') {
        local_9 = 1;
      }
    }
  }
  return local_9;
}


// ============================================================================
// Function: Touch_CheckTrustabilityUnit @ 6ba718d4
// ============================================================================

void Touch_CheckTrustabilityUnit(byte param_1,byte param_2)

{
  if ((((*(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) & 0x100000) == 0) &&
      (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x400000) == 0)) &&
     (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x800000) == 0)) {
    if (((*(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 500) & 0x1000) != 0) &&
       ((*(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f8) & 8) != 0)) {
      *(short *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x23a) =
           *(short *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x23a) + 1;
      if (0x19 < *(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x188)) {
        *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) =
             *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) | 0x20000;
      }
      if ((0xe < *(short *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1a2)) &&
         (4 < *(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x23e))) {
        *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) =
             *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) | 0x20000;
      }
      if ((0x19 < *(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x242)) &&
         (0x19 < *(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x240))) {
        *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) =
             *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) | 0x20000;
      }
      if (*(int *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1ec) != 0) {
        *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) =
             *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) | 0x20000;
      }
      if (1 < (byte)(*_refptr_g_prevTouchesPtr)->field_0x8) {
        *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) =
             *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) | 0x20000;
      }
      if (((*(int *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x21c) == 0x20) &&
          (2 < *(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                        (longlong)(int)(uint)param_2 * 0x478 + 0x21c))) &&
         ((int)*(short *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1be) <
          ((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold * 2) / 5)) {
        *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) =
             *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) | 0x20000;
      }
    }
  }
  else {
    *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) =
         *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) | 0x40000;
  }
  return;
}


// ============================================================================
// Function: Touch_SetTrustableTag @ 6ba71d77
// ============================================================================

void Touch_SetTrustableTag(byte param_1)

{
                    /* 0x31d77  2550  Touch_SetTrustableTag */
  *(undefined4 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x2a0) = 0;
  if (((*(int *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x21c) == 0x20) &&
      ((*(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) & 0x100000) == 0)) &&
     ((*(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) & 0x40000) == 0)) {
    *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) =
         *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) | 0x100000;
    if ((*(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1fc) & 0x20000) == 0) {
      if (2 < *(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x23a)) {
        *(undefined4 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x2a0) = 1;
        *(int *)&(*_refptr_g_tsaStaticPtr)->field_0xac =
             *(int *)&(*_refptr_g_tsaStaticPtr)->field_0xac + 1;
      }
    }
    else {
      *(undefined4 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x2a0) = 2;
      *(int *)&(*_refptr_g_tsaStaticPtr)->field_0xb0 =
           *(int *)&(*_refptr_g_tsaStaticPtr)->field_0xb0 + 1;
    }
  }
  return;
}


// ============================================================================
// Function: Touch_IsLiftOffDueToMerged @ 6ba73da0
// ============================================================================

undefined8 Touch_IsLiftOffDueToMerged(undefined1 param_1,byte param_2)

{
  undefined1 uVar1;
  undefined1 uVar2;
  char cVar3;
  int iVar4;
  
  uVar1 = *(undefined1 *)
           ((longlong)
            (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                (longlong)(int)(uint)param_2 * 0x478 + 0x23c) * 0xc +
            *(longlong *)_refptr_g_prevPeaksPtr + 0xe);
  uVar2 = *(undefined1 *)
           ((longlong)
            (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                (longlong)(int)(uint)param_2 * 0x478 + 0x23c) * 0xc +
            *(longlong *)_refptr_g_prevPeaksPtr + 0xf);
  cVar3 = Touch_IsOff(param_1);
  if ((cVar3 != '\0') && (iVar4 = Sensor_IsTouchTraversed(uVar1,uVar2), iVar4 != 0)) {
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: Touch_IsTouchDownDueToSeperated @ 6ba73e7a
// ============================================================================

undefined8 Touch_IsTouchDownDueToSeperated(byte param_1)

{
  byte bVar1;
  byte bVar2;
  char cVar3;
  int iVar4;
  
  bVar1 = (*_refptr_g_peaksPtr)
          [(longlong)(int)(uint)(byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x23c] *
           0x14 + 0x15].bPeakCount;
  bVar2 = (*_refptr_g_peaksPtr)
          [(longlong)(int)(uint)(byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x23c] *
           0x14 + 0x16].bPeakCount;
  cVar3 = Touch_IsOff(param_1);
  if ((((cVar3 == '\0') &&
       (*(short *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x156) == 1)) &&
      ((*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal <
       *(short *)((ulonglong)
                  ((uint)bVar1 + (uint)bVar2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
                 g_tsaBufTimeDown))) &&
     (iVar4 = Sensor_IsTouchTraversedBefore(bVar1,bVar2), iVar4 != 0)) {
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: Touch_IIRDataProcess @ 6ba7474f
// ============================================================================

void Touch_IIRDataProcess(byte param_1,byte param_2)

{
  PrevTouchArray *pPVar1;
  undefined *puVar2;
  undefined1 uVar3;
  undefined2 uVar4;
  int iVar5;
  char *_Str1;
  longlong lVar6;
  longlong lVar7;
  
  puVar2 = g_touchesPtr;
  lVar6 = (longlong)(int)(uint)param_1 * 0x168;
  pPVar1 = *_refptr_g_prevTouchesPtr;
  lVar7 = (longlong)(int)(uint)param_2 * 0x478;
  g_touchesPtr[lVar6 + 0x25c] = 1;
  _Str1 = (char *)TSA_GetPrmtName();
  iVar5 = strncmp(_Str1,"P116",4);
  if (iVar5 == 0) {
    if (100 < *(ushort *)(puVar2 + lVar6 + 0x1a8)) {
      *(undefined2 *)(puVar2 + lVar6 + 0x1a8) = 100;
    }
    if (100 < *(ushort *)(puVar2 + lVar6 + 0x1aa)) {
      *(undefined2 *)(puVar2 + lVar6 + 0x1aa) = 100;
    }
  }
  if ((*(uint *)(puVar2 + lVar6 + 0x21c) < 3) ||
     ((*(int *)(puVar2 + lVar6 + 0x1e0) == 3 && (*(int *)((longlong)pPVar1 + lVar7 + 0x1e0) != 3))))
  {
    *(undefined2 *)(puVar2 + lVar6 + 0x25e) = *(undefined2 *)(puVar2 + lVar6 + 0x188);
    *(undefined2 *)(puVar2 + lVar6 + 0x260) = *(undefined2 *)(puVar2 + lVar6 + 0x18a);
    *(undefined2 *)(puVar2 + lVar6 + 0x262) = *(undefined2 *)(puVar2 + lVar6 + 0x18c);
    *(undefined2 *)(puVar2 + lVar6 + 0x264) = *(undefined2 *)(puVar2 + lVar6 + 0x18e);
    puVar2[lVar6 + 0x266] = puVar2[lVar6 + 0x192];
    *(undefined2 *)(puVar2 + lVar6 + 0x268) = *(undefined2 *)(puVar2 + lVar6 + 0x1a6);
    *(undefined2 *)(puVar2 + lVar6 + 0x26a) = *(undefined2 *)(puVar2 + lVar6 + 0x1a8);
    *(undefined2 *)(puVar2 + lVar6 + 0x26c) = *(undefined2 *)(puVar2 + lVar6 + 0x1aa);
  }
  else {
    uVar4 = Touch_IIRProcess(*(undefined2 *)((longlong)pPVar1 + lVar7 + 0x25e),
                             *(undefined2 *)(puVar2 + lVar6 + 0x188));
    *(undefined2 *)(puVar2 + lVar6 + 0x25e) = uVar4;
    uVar4 = Touch_IIRProcess(*(undefined2 *)((longlong)pPVar1 + lVar7 + 0x260),
                             *(undefined2 *)(puVar2 + lVar6 + 0x18a));
    *(undefined2 *)(puVar2 + lVar6 + 0x260) = uVar4;
    uVar4 = Touch_IIRProcess(*(undefined2 *)((longlong)pPVar1 + lVar7 + 0x262),
                             *(undefined2 *)(puVar2 + lVar6 + 0x18c));
    *(undefined2 *)(puVar2 + lVar6 + 0x262) = uVar4;
    uVar4 = Touch_IIRProcess(*(undefined2 *)((longlong)pPVar1 + lVar7 + 0x264),
                             *(undefined2 *)(puVar2 + lVar6 + 0x18e));
    *(undefined2 *)(puVar2 + lVar6 + 0x264) = uVar4;
    uVar3 = Touch_IIRProcess((int)*(char *)((longlong)pPVar1 + lVar7 + 0x266),
                             (int)(char)puVar2[lVar6 + 0x192]);
    puVar2[lVar6 + 0x266] = uVar3;
    uVar4 = Touch_IIRProcess((int)*(short *)((longlong)pPVar1 + lVar7 + 0x268),
                             (int)*(short *)(puVar2 + lVar6 + 0x1a6));
    *(undefined2 *)(puVar2 + lVar6 + 0x268) = uVar4;
    uVar4 = Touch_IIRProcess(*(undefined2 *)((longlong)pPVar1 + lVar7 + 0x26a),
                             *(undefined2 *)(puVar2 + lVar6 + 0x1a8));
    *(undefined2 *)(puVar2 + lVar6 + 0x26a) = uVar4;
    uVar4 = Touch_IIRProcess(*(undefined2 *)((longlong)pPVar1 + lVar7 + 0x26c),
                             *(undefined2 *)(puVar2 + lVar6 + 0x1aa));
    *(undefined2 *)(puVar2 + lVar6 + 0x26c) = uVar4;
  }
  if (*(short *)(puVar2 + lVar6 + 0x264) == 0) {
    *(undefined2 *)(puVar2 + lVar6 + 0x264) = 1;
  }
  puVar2[lVar6 + 0x267] =
       (char)(((uint)*(ushort *)(puVar2 + lVar6 + 0x262) << 4) /
             (uint)*(ushort *)(puVar2 + lVar6 + 0x264));
  return;
}


// ============================================================================
// Function: Touch_GetMergedInfo @ 6ba74b56
// ============================================================================

void Touch_GetMergedInfo(byte param_1)

{
  char cVar1;
  byte local_9;
  
  for (local_9 = 0; local_9 < (byte)*g_touchesPtr; local_9 = local_9 + 1) {
    if ((param_1 != local_9) &&
       (cVar1 = Sensor_IsInZone((char)((ushort)*(undefined2 *)
                                                (g_touchesPtr +
                                                (longlong)(int)(uint)param_1 * 0x168 + 0x162) >> 8),
                                (char)((ushort)*(undefined2 *)
                                                (g_touchesPtr +
                                                (longlong)(int)(uint)param_1 * 0x168 + 0x164) >> 8),
                                g_touchesPtr[(longlong)(int)(uint)local_9 * 0x168 + 0x28c]),
       cVar1 != '\0')) {
      *(uint *)(g_touchesPtr + (longlong)(int)(uint)local_9 * 0x168 + 0x214) =
           *(uint *)(g_touchesPtr + (longlong)(int)(uint)local_9 * 0x168 + 0x214) | 0x100000;
    }
  }
  return;
}


// ============================================================================
// Function: Touch_IsAbnormalShape @ 6ba74dfe
// ============================================================================

undefined8 Touch_IsAbnormalShape(byte param_1)

{
  char cVar1;
  int iVar2;
  
  cVar1 = Touch_IsOff(param_1);
  if (cVar1 != '\0') {
    return 0;
  }
  if ((((*_refptr_g_prevTouchesPtr)->field_0x6 != '\0') &&
      (*(int *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1ec) == 0x40)) &&
     (iVar2 = Touch_GetTimeRpt(param_1), iVar2 < 0x7fff)) {
    return 0;
  }
  cVar1 = Touch_IsInAbnormalShapeDetectArea(param_1);
  if (cVar1 != '\0') {
    if ((*(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f0) & 0x2000000) != 0) {
      return 1;
    }
    if ((0xe < *(short *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1a6)) &&
       (0x18 < *(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x188))) {
      return 1;
    }
    if (0x1e < *(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x188)) {
      return 1;
    }
    if (((0x27 < *(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x22a)) &&
        (4 < (byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x1ac])) &&
       ((4 < (byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x1ad] &&
        (0x13 < *(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x188))))) {
      return 1;
    }
    if ((*(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 500) & 0x100) != 0) {
      return 1;
    }
    if ((*(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f0) & 0x1000000) != 0) {
      return 1;
    }
  }
  return 0;
}


// ============================================================================
// Function: Touch_IsAbnormalShape2 @ 6ba7504e
// ============================================================================

undefined8 Touch_IsAbnormalShape2(byte param_1)

{
  char cVar1;
  undefined8 uVar2;
  
  cVar1 = Touch_IsOff(param_1);
  if (cVar1 == '\0') {
    if (((*(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x188) < 0x19) ||
        ((uint)*(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x188) <=
         (uint)*(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x18a) * 2 +
         (uint)*(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x18a))) ||
       (*(short *)_refptr_g_tsaPrmtDynamic / 2 <=
        *(short *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x29e))) {
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
// Function: Touch_IsAbnormalShape3 @ 6ba7513e
// ============================================================================

undefined8 Touch_IsAbnormalShape3(byte param_1)

{
  char cVar1;
  int iVar2;
  
  cVar1 = Touch_IsOff(param_1);
  if (cVar1 == '\0') {
    if ((*(int *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1ec) == 0x40) &&
       (iVar2 = Touch_GetTimeRpt(param_1), iVar2 < 0x7fff)) {
      return 0;
    }
    if (((*(int *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1ec) != 0x80) ||
        (*(short *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1ba) <=
         (short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 2)) &&
       (cVar1 = Touch_IsInAbnormalShapeDetectArea(param_1), cVar1 != '\0')) {
      if ((0x31 < *(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x188)) ||
         ((*(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f0) & 0x2000) != 0)) {
        return 1;
      }
      if (((_refptr_g_tsaPrmtFlash->wBufElementCount / 3 <
            *(ushort *)&_refptr_g_tsaPrpt->field_0x186) &&
          (*(short *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1ba) <
           (short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 3)) &&
         (0x27 < *(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x188))) {
        return 1;
      }
    }
  }
  return 0;
}


// ============================================================================
// Function: Touch_IsAbnormalShape4 @ 6ba7532f
// ============================================================================

undefined8 Touch_IsAbnormalShape4(byte param_1)

{
  char cVar1;
  undefined8 uVar2;
  
  cVar1 = Touch_IsOff(param_1);
  if (cVar1 == '\0') {
    if ((((*(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x22a) < 0x1b) ||
         (*(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x188) < 0x1b)) ||
        (*(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x18a) < 0x14)) ||
       (((byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x1ac] < 6 ||
        ((byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x1ad] < 6)))) {
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
// Function: Touch_SetVolatileFlag @ 6ba70fdd
// ============================================================================

void Touch_SetVolatileFlag(byte param_1,byte param_2)

{
  PrevTouchArray *pPVar1;
  undefined *puVar2;
  char cVar3;
  undefined1 uVar4;
  short sVar5;
  longlong lVar6;
  longlong lVar7;
  
  puVar2 = g_touchesPtr;
  lVar6 = (longlong)(int)(uint)param_1 * 0x168;
  pPVar1 = *_refptr_g_prevTouchesPtr;
  lVar7 = (longlong)(int)(uint)param_2 * 0x478;
  Touch_SetVolatileFlagSigSum(param_1,param_2);
  Touch_SetVolatileFlagTouchAxisAbnormal(param_1,param_2);
  if ((byte)puVar2[lVar6 + 0x1de] < 0x18) {
    *(uint *)(puVar2 + lVar6 + 0x1f0) = *(uint *)(puVar2 + lVar6 + 0x1f0) | 0x80000;
  }
  if (100 < *(ushort *)(puVar2 + lVar6 + 0x240)) {
    *(uint *)(puVar2 + lVar6 + 0x1f0) = *(uint *)(puVar2 + lVar6 + 0x1f0) | 0x80;
  }
  if ((7 < *(ushort *)(puVar2 + lVar6 + 0x1a8)) && (7 < *(ushort *)(puVar2 + lVar6 + 0x1aa))) {
    *(uint *)(puVar2 + lVar6 + 0x1fc) = *(uint *)(puVar2 + lVar6 + 0x1fc) | 8;
  }
  if ((puVar2[lVar6 + 0x1b0] != '\0') && (puVar2[lVar6 + 0x1b1] != '\0')) {
    *(uint *)(puVar2 + lVar6 + 0x1f0) = *(uint *)(puVar2 + lVar6 + 0x1f0) | 0x4000;
  }
  if (_refptr_g_tsaPrmtDynamic[0x96] == '\0') {
    sVar5 = *(short *)(*_refptr_g_peaksPtr +
                      (longlong)(int)(uint)(byte)puVar2[lVar6 + 0x23c] * 0x14 + 0x18);
    if (sVar5 < 0) {
      sVar5 = sVar5 + 0x1f;
    }
    if (*(int *)(*_refptr_g_peaksPtr +
                (longlong)(int)(uint)(byte)puVar2[lVar6 + 0x23c] * 0x14 + 0x1c) < (int)(sVar5 >> 5))
    {
      *(uint *)(puVar2 + lVar6 + 0x1f0) = *(uint *)(puVar2 + lVar6 + 0x1f0) | 0x80000000;
      (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
           (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 0x40;
    }
  }
  else {
    cVar3 = Touch_IsPeakOnInnerDim1Edge(param_1);
    if ((cVar3 == '\0') || (puVar2[lVar6 + 0x2a4] == '\0')) {
      sVar5 = *(short *)(*_refptr_g_peaksPtr +
                        (longlong)(int)(uint)(byte)puVar2[lVar6 + 0x23c] * 0x14 + 0x18);
      if (sVar5 < 0) {
        sVar5 = sVar5 + 0x1f;
      }
      if (*(int *)(*_refptr_g_peaksPtr +
                  (longlong)(int)(uint)(byte)puVar2[lVar6 + 0x23c] * 0x14 + 0x1c) <
          (int)(sVar5 >> 5)) {
        *(uint *)(puVar2 + lVar6 + 0x1f0) = *(uint *)(puVar2 + lVar6 + 0x1f0) | 0x80000000;
        (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
             (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 0x40;
      }
    }
  }
  uVar4 = Touch_NeedNextEdgeLowThresh(param_1,param_2);
  Touch_SetVolatileFlagFingerZFailed(param_1,param_2);
  Touch_SetVolatileFlagTouchDownZFailed(param_1,param_2,uVar4);
  Touch_SetVolatileFlagLiftOffZFailed(param_1,param_2,uVar4);
  Touch_SetVolatileFlagStableTouch(param_1);
  Touch_UpdateStableProperty(param_1,param_2);
  if ((*(short *)((longlong)pPVar1 + lVar7 + 0x156) == 0) &&
     (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x10) != 0)) {
    *(uint *)(puVar2 + lVar6 + 0x1f0) = *(uint *)(puVar2 + lVar6 + 0x1f0) | 0x800000;
  }
  if (((*(uint *)((longlong)pPVar1 + lVar7 + 0x1f8) & 0x4000) != 0) ||
     ((*(uint *)(puVar2 + lVar6 + 0x1f0) & 0x800000) != 0)) {
    cVar3 = BLReset_CheckAbnormalTouch(param_1);
    if (cVar3 != '\0') {
      *(uint *)(puVar2 + lVar6 + 0x1f0) = *(uint *)(puVar2 + lVar6 + 0x1f0) | 0x400000;
    }
  }
  if ((*(short *)((longlong)pPVar1 + lVar7 + 0x156) == 0) && (*(int *)(puVar2 + lVar6 + 0x1ec) != 0)
     ) {
    (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
         (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 0x80;
  }
  Touch_GetPeakSumPosSig(param_1,param_2);
  Touch_UpdateHandEnum(param_1,param_2);
  return;
}


// ============================================================================
// Function: Touch_SetPersistFlag @ 6ba72776
// ============================================================================

void Touch_SetPersistFlag(byte param_1,byte param_2)

{
  PrevTouchArray *pPVar1;
  undefined *puVar2;
  char cVar3;
  longlong lVar4;
  longlong lVar5;
  
  puVar2 = g_touchesPtr;
  lVar4 = (longlong)(int)(uint)param_1 * 0x168;
  pPVar1 = *_refptr_g_prevTouchesPtr;
  lVar5 = (longlong)(int)(uint)param_2 * 0x478;
  if ((*(uint *)(g_touchesPtr + lVar4 + 0x1f0) & 4) != 0) {
    *(uint *)(g_touchesPtr + lVar4 + 0x1f8) = *(uint *)(g_touchesPtr + lVar4 + 0x1f8) | 1;
  }
  if ((int)((uint)*(ushort *)(_refptr_g_tsaPrmtDynamic + 0x88) *
           (uint)*(ushort *)(_refptr_g_tsaPrmtDynamic + 0x88)) <=
      (int)(uint)*(ushort *)(puVar2 + lVar4 + 0x24c)) {
    *(uint *)(puVar2 + lVar4 + 0x1f8) = *(uint *)(puVar2 + lVar4 + 0x1f8) | 4;
  }
  if ((*(uint *)(puVar2 + lVar4 + 0x1f0) & 0x1000) != 0) {
    *(uint *)(puVar2 + lVar4 + 0x1f8) = *(uint *)(puVar2 + lVar4 + 0x1f8) | 0x200;
  }
  if (((*(uint *)((longlong)pPVar1 + lVar5 + 0x1f8) & 8) != 0) &&
     (*(int *)((longlong)pPVar1 + lVar5 + 0x1e8) != 6)) {
    Touch_SetVolatileFlagFalseFlickDetected(param_1,param_2);
    Touch_SetPersistFlagMoveOverClickSuppressRange(param_1,param_2);
    Touch_SetPersistFlagMoveOverMaxClickSuppressRange(param_1,param_2);
    Touch_SetPersistFlagMoveOverFlickDistance(param_1,param_2);
  }
  if ((*(uint *)(puVar2 + lVar4 + 0x1f0) & 0x800000) != 0) {
    *(uint *)(puVar2 + lVar4 + 0x1f8) = *(uint *)(puVar2 + lVar4 + 0x1f8) | 0x4000;
  }
  if (((*(uint *)((longlong)pPVar1 + lVar5 + 0x1f0) & 0x400000) != 0) &&
     (*(short *)((longlong)pPVar1 + lVar5 + 0x4e4) == 10)) {
    *(uint *)(puVar2 + lVar4 + 0x1f8) = *(uint *)(puVar2 + lVar4 + 0x1f8) | 0x2000;
  }
  if ((((0x10 < *(ushort *)(puVar2 + lVar4 + 0x240)) || (*(int *)(puVar2 + lVar4 + 0x1ec) != 0)) ||
      ((short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax * 2 <
       (int)*(short *)(puVar2 + lVar4 + 0x1be))) || (1 < (byte)*g_touchesPtr)) {
    *(uint *)(puVar2 + lVar4 + 0x1f8) = *(uint *)(puVar2 + lVar4 + 0x1f8) | 0x80000;
  }
  if (*(short *)((longlong)pPVar1 + lVar5 + 0x156) == 0) {
    cVar3 = AFE_IsRawJump();
    if (cVar3 != '\0') {
      *(uint *)(puVar2 + lVar4 + 0x214) = *(uint *)(puVar2 + lVar4 + 0x214) | 0x10000000;
    }
  }
  *(uint *)(puVar2 + lVar4 + 0x1f8) =
       *(uint *)(puVar2 + lVar4 + 0x1f8) | *(uint *)((longlong)pPVar1 + lVar5 + 0x1f8);
  *(uint *)(puVar2 + lVar4 + 0x1fc) =
       *(uint *)(puVar2 + lVar4 + 0x1fc) | *(uint *)((longlong)pPVar1 + lVar5 + 0x1fc);
  *(uint *)(puVar2 + lVar4 + 0x200) =
       *(uint *)(puVar2 + lVar4 + 0x200) | *(uint *)((longlong)pPVar1 + lVar5 + 0x200);
  *(uint *)(puVar2 + lVar4 + 0x214) =
       *(uint *)(puVar2 + lVar4 + 0x214) | *(uint *)((longlong)pPVar1 + lVar5 + 0x214);
  return;
}


// ============================================================================
// Function: Touch_SetRegion @ 6ba72bce
// ============================================================================

void Touch_SetRegion(byte param_1)

{
  ushort uVar1;
  ushort uVar2;
  ushort local_c;
  ushort local_a;
  
  uVar1 = TSAOut_DisplayXPos(param_1);
  uVar2 = TSAOut_DisplayYPos(param_1);
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    local_a = *(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a6;
    local_c = *(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a8;
  }
  else {
    local_a = *(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a8;
    local_c = *(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a6;
  }
  if (uVar1 < local_a >> 1) {
    if (uVar2 < local_c >> 1) {
      *(undefined4 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x220) = 0;
    }
    else {
      *(undefined4 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x220) = 1;
    }
  }
  else if (uVar2 < local_c >> 1) {
    *(undefined4 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x220) = 2;
  }
  else {
    *(undefined4 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x220) = 3;
  }
  return;
}


// ============================================================================
// Function: Touch_SetEdgeFlag @ 6ba72d1c
// ============================================================================

void Touch_SetEdgeFlag(byte param_1)

{
  undefined1 uVar1;
  char cVar2;
  
  uVar1 = g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x23c];
  cVar2 = Peak_IsOnInnerEdgeDim1Near(uVar1);
  if (cVar2 == '\0') {
    cVar2 = Peak_IsOnInnerEdgeDim1Far(uVar1);
    if (cVar2 != '\0') {
      *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x204) =
           *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x204) | 0x200;
    }
  }
  else {
    *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x204) =
         *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x204) | 0x100;
  }
  cVar2 = Peak_IsOnInnerEdgeDim2Near(uVar1);
  if (cVar2 == '\0') {
    cVar2 = Peak_IsOnInnerEdgeDim2Far(uVar1);
    if (cVar2 != '\0') {
      *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x204) =
           *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x204) | 0x800;
    }
  }
  else {
    *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x204) =
         *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x204) | 0x400;
  }
  cVar2 = Peak_IsOnOuterEdgeDim1Near(uVar1);
  if (cVar2 == '\0') {
    cVar2 = Peak_IsOnOuterEdgeDim1Far(uVar1);
    if (cVar2 != '\0') {
      *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x204) =
           *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x204) | 0x2000;
    }
  }
  else {
    *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x204) =
         *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x204) | 0x1000;
  }
  cVar2 = Peak_IsOnOuterEdgeDim2Near(uVar1);
  if (cVar2 == '\0') {
    cVar2 = Peak_IsOnOuterEdgeDim2Far(uVar1);
    if (cVar2 != '\0') {
      *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x204) =
           *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x204) | 0x8000;
    }
  }
  else {
    *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x204) =
         *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x204) | 0x4000;
  }
  return;
}


// ============================================================================
// Function: Touch_SetSuppressFlag @ 6ba72ffb
// ============================================================================

void Touch_SetSuppressFlag(void)

{
  return;
}


// ============================================================================
// Function: Touch_GetSQDistMMFromPrevPos @ 6ba735c9
// ============================================================================

void Touch_GetSQDistMMFromPrevPos(byte param_1,byte param_2)

{
  undefined *puVar1;
  undefined2 uVar2;
  
  puVar1 = g_touchesPtr;
  uVar2 = Touch_GetSQDistMM(*(undefined2 *)
                             (g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x162),
                            *(undefined2 *)
                             (g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x164),
                            *(undefined2 *)
                             ((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)param_2 * 0x478 + 0x162),
                            *(undefined2 *)
                             ((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)param_2 * 0x478 + 0x164));
  *(undefined2 *)(puVar1 + (longlong)(int)(uint)param_1 * 0x168 + 0x240) = uVar2;
  return;
}


// ============================================================================
// Function: Touch_GetSQAccMMFromPrevPos @ 6ba736aa
// ============================================================================

void Touch_GetSQAccMMFromPrevPos(byte param_1,byte param_2)

{
  undefined *puVar1;
  short sVar2;
  short sVar3;
  undefined2 uVar4;
  uint local_1c;
  
  puVar1 = g_touchesPtr;
  sVar2 = (*(short *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x162) -
          *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_2 * 0x478 + 0x162)) -
          *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_2 * 0x478 + 0x4d4);
  sVar3 = (*(short *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x164) -
          *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_2 * 0x478 + 0x164)) -
          *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_2 * 0x478 + 0x4d6);
  local_1c = (int)sVar3 * (int)sVar3 + (int)sVar2 * (int)sVar2;
  if (local_1c < (uint)((int)sVar2 * (int)sVar2)) {
    local_1c = 0xffffffff;
  }
  uVar4 = TS_SQPixelToSQMM(local_1c);
  *(undefined2 *)(puVar1 + (longlong)(int)(uint)param_1 * 0x168 + 0x242) = uVar4;
  return;
}


// ============================================================================
// Function: Touch_GetSQDistPixelFromPrevPos @ 6ba7381e
// ============================================================================

void Touch_GetSQDistPixelFromPrevPos(byte param_1,byte param_2)

{
  undefined *puVar1;
  undefined4 uVar2;
  
  puVar1 = g_touchesPtr;
  uVar2 = Touch_GetSQDistPixel
                    (*(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x162),
                     *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x164),
                     *(undefined2 *)
                      ((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_2 * 0x478 + 0x162),
                     *(undefined2 *)
                      ((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_2 * 0x478 + 0x164));
  *(undefined4 *)(puVar1 + (longlong)(int)(uint)param_1 * 0x168 + 0x244) = uVar2;
  return;
}


// ============================================================================
// Function: Touch_GetSQDistMMFromStartPos @ 6ba738ff
// ============================================================================

void Touch_GetSQDistMMFromStartPos(byte param_1,byte param_2)

{
  undefined *puVar1;
  undefined2 uVar2;
  
  puVar1 = g_touchesPtr;
  uVar2 = Touch_GetSQDistMM(*(undefined2 *)
                             (g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x162),
                            *(undefined2 *)
                             (g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x164),
                            *(undefined2 *)
                             ((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)param_2 * 0x478 + 0x496),
                            *(undefined2 *)
                             ((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)param_2 * 0x478 + 0x498));
  *(undefined2 *)(puVar1 + (longlong)(int)(uint)param_1 * 0x168 + 0x24c) = uVar2;
  return;
}


// ============================================================================
// Function: Touch_UpdateGlobalState @ 6ba75b75
// ============================================================================

void Touch_UpdateGlobalState(byte param_1,byte param_2)

{
  if (((*(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f0) & 4) != 0) &&
     ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x1f8)
      & 8) != 0)) {
    *(uint *)(g_touchesPtr + 0x24) = *(uint *)(g_touchesPtr + 0x24) | 1;
  }
  if ((*(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f0) & 0x2000) != 0) {
    *(void **)(g_touchesPtr + 0x38) = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  }
  return;
}


// ============================================================================
// Function: Touch_UnitUpdateSqDistMMFromStartPosMax @ 6ba75c2e
// ============================================================================

void Touch_UnitUpdateSqDistMMFromStartPosMax(byte param_1,byte param_2)

{
  undefined *puVar1;
  char cVar2;
  undefined2 uVar3;
  
  cVar2 = Touch_IsFarAwayFromCornerCompareToStartPoint(param_1,param_2);
  puVar1 = g_touchesPtr;
  if (cVar2 == '\0') {
    *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x24e) =
         *(undefined2 *)
          ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x24e);
  }
  else {
    uVar3 = MAX(*(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x24c),
                *(undefined2 *)
                 ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x24e
                 ));
    *(undefined2 *)(puVar1 + (longlong)(int)(uint)param_1 * 0x168 + 0x24e) = uVar3;
  }
  return;
}


// ============================================================================
// Function: Touch_UnitUpdateMaxMin @ 6ba75d1a
// ============================================================================

void Touch_UnitUpdateMaxMin(byte param_1,byte param_2)

{
  PrevTouchArray *pPVar1;
  undefined *puVar2;
  undefined1 uVar3;
  short sVar4;
  undefined2 uVar5;
  undefined4 uVar6;
  longlong lVar7;
  longlong lVar8;
  
  puVar2 = g_touchesPtr;
  lVar7 = (longlong)(int)(uint)param_1 * 0x168;
  pPVar1 = *_refptr_g_prevTouchesPtr;
  lVar8 = (longlong)(int)(uint)param_2 * 0x478;
  sVar4 = MIN((int)*(short *)(*_refptr_g_peaksPtr + 2),
              (int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold);
  uVar3 = MAX(puVar2[lVar7 + 0x1b2],*(undefined1 *)((longlong)pPVar1 + lVar8 + 0x1b3));
  puVar2[lVar7 + 0x1b3] = uVar3;
  uVar3 = MAX(puVar2[lVar7 + 0x1b0],*(undefined1 *)((longlong)pPVar1 + lVar8 + 0x1b4));
  puVar2[lVar7 + 0x1b4] = uVar3;
  uVar3 = MAX(puVar2[lVar7 + 0x1b1],*(undefined1 *)((longlong)pPVar1 + lVar8 + 0x1b5));
  puVar2[lVar7 + 0x1b5] = uVar3;
  uVar3 = MAX(puVar2[lVar7 + 0x198],*(undefined1 *)((longlong)pPVar1 + lVar8 + 0x1b6));
  puVar2[lVar7 + 0x1b6] = uVar3;
  uVar3 = MAX(puVar2[lVar7 + 0x1b8],*(undefined1 *)((longlong)pPVar1 + lVar8 + 0x1b9));
  puVar2[lVar7 + 0x1b9] = uVar3;
  uVar5 = MAX(*(undefined2 *)(puVar2 + lVar7 + 0x22c),
              *(undefined2 *)((longlong)pPVar1 + lVar8 + 0x234));
  *(undefined2 *)(puVar2 + lVar7 + 0x234) = uVar5;
  uVar5 = MAX(*(undefined2 *)(puVar2 + lVar7 + 0x22e),
              *(undefined2 *)((longlong)pPVar1 + lVar8 + 0x236));
  *(undefined2 *)(puVar2 + lVar7 + 0x236) = uVar5;
  uVar6 = MAX(*(undefined4 *)(puVar2 + lVar7 + 0x244),
              *(undefined4 *)((longlong)pPVar1 + lVar8 + 0x248));
  *(undefined4 *)(puVar2 + lVar7 + 0x248) = uVar6;
  uVar5 = MAX((int)*(short *)(*_refptr_g_peaksPtr +
                             (longlong)(int)(uint)(byte)puVar2[lVar7 + 0x23c] * 0x14 + 0x18),
              (int)*(short *)((longlong)pPVar1 + lVar8 + 0x1be));
  *(undefined2 *)(puVar2 + lVar7 + 0x1be) = uVar5;
  uVar5 = MAX((int)sVar4,(int)*(short *)((longlong)pPVar1 + lVar8 + 0x1bc));
  *(undefined2 *)(puVar2 + lVar7 + 0x1bc) = uVar5;
  uVar6 = MAX(*(undefined4 *)
               (*_refptr_g_peaksPtr + (longlong)(int)(uint)(byte)puVar2[lVar7 + 0x23c] * 0x14 + 0x20
               ),*(undefined4 *)((longlong)pPVar1 + lVar8 + 0x1c0));
  *(undefined4 *)(puVar2 + lVar7 + 0x1c0) = uVar6;
  if (*(short *)((longlong)pPVar1 + lVar8 + 0x1ba) < *(short *)(puVar2 + lVar7 + 0x29e)) {
    uVar5 = MIN((int)*(short *)(puVar2 + lVar7 + 0x29e),
                (int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold);
    *(undefined2 *)(puVar2 + lVar7 + 0x1ba) = uVar5;
  }
  else {
    *(undefined2 *)(puVar2 + lVar7 + 0x1ba) = *(undefined2 *)((longlong)pPVar1 + lVar8 + 0x1ba);
  }
  if (*(short *)((longlong)pPVar1 + lVar8 + 0x156) == 0) {
    puVar2[lVar7 + 0x1b7] = puVar2[lVar7 + 0x198];
    *(undefined2 *)(puVar2 + lVar7 + 0x230) = *(undefined2 *)(puVar2 + lVar7 + 0x22c);
    *(undefined2 *)(puVar2 + lVar7 + 0x232) = *(undefined2 *)(puVar2 + lVar7 + 0x22e);
  }
  else {
    uVar3 = MIN(puVar2[lVar7 + 0x198],*(undefined1 *)((longlong)pPVar1 + lVar8 + 0x1b7));
    puVar2[lVar7 + 0x1b7] = uVar3;
    uVar5 = MIN(*(undefined2 *)(puVar2 + lVar7 + 0x22c),
                *(undefined2 *)((longlong)pPVar1 + lVar8 + 0x230));
    *(undefined2 *)(puVar2 + lVar7 + 0x230) = uVar5;
    uVar5 = MIN(*(undefined2 *)(puVar2 + lVar7 + 0x22e),
                *(undefined2 *)((longlong)pPVar1 + lVar8 + 0x232));
    *(undefined2 *)(puVar2 + lVar7 + 0x232) = uVar5;
  }
  return;
}


// ============================================================================
// Function: Touch_IsGrip @ 6ba742be
// ============================================================================

void Touch_IsGrip(undefined1 param_1)

{
                    /* 0x342be  2538  Touch_IsGrip */
  GripFilter_IsGripTouch(param_1);
  return;
}


// ============================================================================
// Function: Touch_GetPostReportLength @ 6ba76958
// ============================================================================

uint Touch_GetPostReportLength(int param_1,int param_2,int param_3,int param_4)

{
  ushort uVar1;
  uint uVar2;
  uint uVar3;
  uint local_c;
  
                    /* 0x36958  2522  Touch_GetPostReportLength */
  uVar1 = TS_MMToPixel(2);
  local_c = 0;
  if (uVar1 != 0) {
    uVar2 = param_3 - param_1 >> 0x1f;
    uVar3 = param_4 - param_2 >> 0x1f;
    if ((int)((param_4 - param_2 ^ uVar3) - uVar3) < (int)((param_3 - param_1 ^ uVar2) - uVar2)) {
      uVar2 = param_3 - param_1 >> 0x1f;
      local_c = (int)((param_3 - param_1 ^ uVar2) - uVar2) / (int)(uint)uVar1;
    }
    else {
      uVar2 = param_4 - param_2 >> 0x1f;
      local_c = (int)((param_4 - param_2 ^ uVar2) - uVar2) / (int)(uint)uVar1;
    }
  }
  if (5 < local_c) {
    local_c = 5;
  }
  if (local_c < 2) {
    local_c = 2;
  }
  return local_c;
}


// ============================================================================
// Function: Touch_IsGhostByMinSigOff2 @ 6ba7173d
// ============================================================================

bool Touch_IsGhostByMinSigOff2(byte param_1)

{
                    /* 0x3173d  2537  Touch_IsGhostByMinSigOff2 */
  return (int)*(short *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1ba) <
         *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)
                   (int)(uint)(byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x287] *
                   0x478 + 0x5a6) * 2;
}


// ============================================================================
// Function: Touch_IsSideCoreInSameZone @ 6ba71415
// ============================================================================

undefined8 Touch_IsSideCoreInSameZone(byte param_1,byte param_2)

{
  undefined1 uVar1;
  undefined1 uVar2;
  undefined1 uVar3;
  char cVar4;
  
                    /* 0x31415  2542  Touch_IsSideCoreInSameZone */
  uVar1 = (&DAT_6bbc6b8c)[(longlong)(int)(uint)param_1 * 0x168];
  uVar2 = TSA_SideTouchDim2Center(param_2);
  uVar3 = TSA_SideTouchDim1Center(param_2);
  cVar4 = Sensor_IsInCoreZone(uVar3,uVar2,uVar1);
  if (cVar4 == '\0') {
    uVar1 = _refptr_g_touchesSide[(longlong)(int)(uint)param_2 * 0x168 + 0x28c];
    uVar2 = TSA_CoreTouchDim2Center(param_1);
    uVar3 = TSA_CoreTouchDim1Center(param_1);
    cVar4 = Sensor_IsInSideZone(uVar3,uVar2,uVar1);
    if (cVar4 == '\0') {
      return 0;
    }
  }
  return 1;
}


// ============================================================================
// Function: Touch_IIRProcess @ 6ba746ed
// ============================================================================

int Touch_IIRProcess(int param_1,int param_2)

{
  int iVar1;
  char local_9;
  
  iVar1 = param_1 * 7 + param_2;
  if (iVar1 < 0) {
    iVar1 = iVar1 + 7;
  }
  iVar1 = iVar1 >> 3;
  if (iVar1 < param_2) {
    local_9 = '\x01';
  }
  else if (param_2 < iVar1) {
    local_9 = -1;
  }
  else {
    local_9 = '\0';
  }
  return iVar1 + local_9;
}


// ============================================================================
// Function: Touch_GetTimeRpt @ 6ba6f5ac
// ============================================================================

int Touch_GetTimeRpt(byte param_1)

{
                    /* 0x2f5ac  2531  Touch_GetTimeRpt */
  return (int)*(short *)((ulonglong)
                         ((uint)(byte)((ushort)*(undefined2 *)
                                                (g_touchesPtr +
                                                (longlong)(int)(uint)param_1 * 0x168 + 0x162) >> 8)
                          + (uint)(byte)((ushort)*(undefined2 *)
                                                  (g_touchesPtr +
                                                  (longlong)(int)(uint)param_1 * 0x168 + 0x164) >> 8
                                        ) * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
                        g_tsaBufTimeRpt);
}


// ============================================================================
// Function: Touch_IsInAbnormalShapeDetectArea @ 6ba74c6f
// ============================================================================

undefined8 Touch_IsInAbnormalShapeDetectArea(byte param_1)

{
  char cVar1;
  undefined8 uVar2;
  byte local_9;
  
  local_9 = 2;
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) != 0) {
    local_9 = 3;
  }
  if (((ushort)((ushort)local_9 * 0x100) <
       *(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x234)) &&
     (0x200 < *(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x236))) {
    uVar2 = 1;
  }
  else if (((byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x1ac] <
            (byte)(local_9 * '\x02')) ||
          ((byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x1ad] < 4)) {
    cVar1 = GripFilter_IsGripOrTbd
                      (*(undefined4 *)
                        ((longlong)*_refptr_g_prevTouchesPtr +
                        (longlong)
                        (int)(uint)(byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x287]
                        * 0x478 + 0x210));
    if (cVar1 == '\0') {
      uVar2 = 1;
    }
    else if ((*(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x214) & 0x10000000)
             == 0) {
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
// Function: Touch_GetPeakSumPosSig @ 6ba6f641
// ============================================================================

void Touch_GetPeakSumPosSig(byte param_1,byte param_2)

{
  if (*(int *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x1d4) <
      *(int *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1cc)) {
    *(undefined4 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1d4) =
         *(undefined4 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1cc);
    g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x193] =
         g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x192];
  }
  else {
    *(undefined4 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1d4) =
         *(undefined4 *)
          ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x1d4);
    g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x193] =
         *(undefined1 *)
          ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x193);
  }
  return;
}


// ============================================================================
// Function: Touch_SetVolatileFlagSigSum @ 6ba6f7af
// ============================================================================

void Touch_SetVolatileFlagSigSum(byte param_1,byte param_2)

{
  int iVar1;
  int iVar2;
  
  if (*(int *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x1cc)
      == 0) {
    g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x1dc] = 0xff;
  }
  else {
    iVar1 = (*(int *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1cc) << 5) /
            *(int *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_2 * 0x478 + 0x1cc);
    iVar2 = 0xff;
    if (iVar1 < 0x100) {
      iVar2 = iVar1;
    }
    g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x1dc] = (char)iVar2;
  }
  if (*(byte *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x23c)
      < **(byte **)_refptr_g_prevPeaksPtr) {
    if (*(int *)((longlong)
                 (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                     (longlong)(int)(uint)param_2 * 0x478 + 0x23c) * 0xc +
                 *(longlong *)_refptr_g_prevPeaksPtr + 8) == 0) {
      g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x1dd] = 0xff;
    }
    else {
      iVar1 = (*(int *)(*_refptr_g_peaksPtr +
                       (longlong)
                       (int)(uint)(byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x23c] *
                       0x14 + 0x20) << 5) /
              *(int *)((longlong)
                       (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                           (longlong)(int)(uint)param_2 * 0x478 + 0x23c) * 0xc +
                       *(longlong *)_refptr_g_prevPeaksPtr + 8);
      iVar2 = 0xff;
      if (iVar1 < 0x100) {
        iVar2 = iVar1;
      }
      g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x1dd] = (char)iVar2;
    }
    if (*(short *)((longlong)
                   (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                       (longlong)(int)(uint)param_2 * 0x478 + 0x23c) * 0xc +
                   *(longlong *)_refptr_g_prevPeaksPtr + 0xc) == 0) {
      g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x1de] = 0xff;
    }
    else {
      iVar1 = ((int)*(short *)(*_refptr_g_peaksPtr +
                              (longlong)
                              (int)(uint)(byte)g_touchesPtr
                                               [(longlong)(int)(uint)param_1 * 0x168 + 0x23c] * 0x14
                              + 0x18) << 5) /
              (int)*(short *)((longlong)
                              (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                                  (longlong)(int)(uint)param_2 * 0x478 + 0x23c) *
                              0xc + *(longlong *)_refptr_g_prevPeaksPtr + 0xc);
      iVar2 = 0xff;
      if (iVar1 < 0x100) {
        iVar2 = iVar1;
      }
      g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x1de] = (char)iVar2;
    }
  }
  else {
    g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x1dd] = 0xff;
    g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x1de] = 0xff;
  }
  if ((byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x1dc] < 0x18) {
    *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f0) =
         *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f0) | 2;
  }
  if (0x28 < (byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x1dc]) {
    *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f0) =
         *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f0) | 1;
  }
  return;
}


// ============================================================================
// Function: Touch_SetVolatileFlagTouchAxisAbnormal @ 6ba6fc6a
// ============================================================================

void Touch_SetVolatileFlagTouchAxisAbnormal(byte param_1,byte param_2)

{
  if (((((byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x1dd] < 0x18) &&
       (*(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f0) =
             *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f0) | 0x40000,
       (uint)*(ushort *)
              ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x188) *
       4 < (uint)*(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x188) * 3)) &&
      (0x19 < *(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x188))) &&
     (*(short *)(*_refptr_g_peaksPtr +
                (longlong)
                (int)(uint)(byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x23c] * 0x14 +
                0x18) * 2 < *(short *)(_refptr_g_tsaPrmtDynamic + 0xc) * 3)) {
    *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 500) =
         *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 500) | 0x40;
  }
  return;
}


// ============================================================================
// Function: Touch_NeedNextEdgeLowThresh @ 6ba6fe70
// ============================================================================

undefined1 Touch_NeedNextEdgeLowThresh(byte param_1)

{
  char cVar1;
  int iVar2;
  char *pcVar3;
  undefined1 local_9;
  
  local_9 = 0;
  pcVar3 = (char *)TSA_GetPrmtName();
  iVar2 = strncmp(pcVar3,"P116",4);
  if (iVar2 == 0) {
LAB_6ba6fec4:
    if ((((*_refptr_g_peaksPtr)
          [(longlong)(int)(uint)(byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x23c] *
           0x14 + 0x15].bPeakCount == '\x01') ||
        ((uint)(*_refptr_g_peaksPtr)
               [(longlong)
                (int)(uint)(byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x23c] * 0x14 +
                0x15].bPeakCount == _refptr_g_tsaPrmtFlash->bCols - 2)) ||
       ((uint)(*_refptr_g_peaksPtr)
              [(longlong)(int)(uint)(byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x23c]
               * 0x14 + 0x16].bPeakCount == _refptr_g_tsaPrmtFlash->bRows - 1)) {
      local_9 = 1;
      goto LAB_6ba6ffed;
    }
  }
  else {
    pcVar3 = (char *)TSA_GetPrmtName();
    iVar2 = strncmp(pcVar3,"P150",4);
    if (iVar2 == 0) goto LAB_6ba6fec4;
  }
  cVar1 = Touch_ToeUseSpecialZ4TholdEdge();
  if (cVar1 != '\0') {
    local_9 = 1;
  }
LAB_6ba6ffed:
  if ((_refptr_g_tsaPrmtDynamic[0x1d] != '\0') &&
     ((uint)(*_refptr_g_peaksPtr)
            [(longlong)(int)(uint)(byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x23c] *
             0x14 + 0x15].bPeakCount == _refptr_g_tsaPrmtFlash->bCols - 1)) {
    local_9 = 1;
  }
  if ((_refptr_g_tsaPrmtDynamic[0x1e] != '\0') &&
     ((uint)(*_refptr_g_peaksPtr)
            [(longlong)(int)(uint)(byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x23c] *
             0x14 + 0x16].bPeakCount == _refptr_g_tsaPrmtFlash->bRows - 1)) {
    local_9 = 1;
  }
  return local_9;
}


// ============================================================================
// Function: Touch_SetVolatileFlagFingerZFailed @ 6ba700e1
// ============================================================================

void Touch_SetVolatileFlagFingerZFailed(byte param_1)

{
  int local_c;
  
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) == 0) {
    local_c = *(int *)(_refptr_g_tsaPrmtDynamic + 0x14);
  }
  else {
    local_c = *(int *)(_refptr_g_tsaPrmtDynamic + 0x14) / 2;
  }
  if (*(int *)(*_refptr_g_peaksPtr +
              (longlong)(int)(uint)(byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x23c]
              * 0x14 + 0x20) < local_c) {
    *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f0) =
         *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f0) | 0x800;
  }
  return;
}


// ============================================================================
// Function: Touch_SetVolatileFlagTouchDownZFailed @ 6ba701c3
// ============================================================================

void Touch_SetVolatileFlagTouchDownZFailed(byte param_1,undefined8 param_2,char param_3)

{
  int local_10;
  int local_c;
  
  local_c = *(int *)(_refptr_g_tsaPrmtDynamic + 0x10);
  if (param_3 != '\0') {
    local_c = local_c * 3;
    if (local_c < 0) {
      local_c = local_c + 3;
    }
    local_c = local_c >> 2;
  }
  if (*(int *)(*_refptr_g_peaksPtr +
              (longlong)(int)(uint)(byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x23c]
              * 0x14 + 0x20) < local_c) {
    *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f0) =
         *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f0) | 0x400;
  }
  local_10 = *(int *)(_refptr_g_tsaPrmtDynamic + 0x18);
  if (param_3 != '\0') {
    local_10 = local_10 * 3;
    if (local_10 < 0) {
      local_10 = local_10 + 3;
    }
    local_10 = local_10 >> 2;
  }
  if (*(int *)(*_refptr_g_peaksPtr +
              (longlong)(int)(uint)(byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x23c]
              * 0x14 + 0x20) < local_10) {
    *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f0) =
         *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f0) | 0x40000000;
  }
  return;
}


// ============================================================================
// Function: Touch_SetVolatileFlagLiftOffZFailed @ 6ba70356
// ============================================================================

void Touch_SetVolatileFlagLiftOffZFailed(byte param_1,byte param_2,char param_3)

{
  int iVar1;
  short local_10;
  short local_e;
  int local_c;
  
  if ((((2 >> ((byte)*(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x2c & 0x1f) & 1U) == 0) ||
      (*(int *)&_refptr_g_tsaPrpt[1].field_0x30 != 2)) ||
     ((_refptr_g_tsaPrmtDynamic[0x80] & 2) == 0)) {
    if ((*(ushort *)
          ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x156) <
         0x3d) ||
       ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_2 * 0x478 + 0x1f8) & 4) == 0)) {
      iVar1 = *(short *)(_refptr_g_tsaPrmtDynamic + 0xc) * 3;
      if (iVar1 < 0) {
        iVar1 = iVar1 + 3;
      }
      local_10 = (short)(iVar1 >> 2);
      local_c = *(int *)(_refptr_g_tsaPrmtDynamic + 0x10) * 3;
      if (local_c < 0) {
        local_c = local_c + 3;
      }
      local_c = local_c >> 2;
      if (*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_2 * 0x478 + 0x1be) / 2 < local_10) {
        local_10 = *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)param_2 * 0x478 + 0x1be) / 2;
      }
      if (*(int *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_2 * 0x478 + 0x1c0) / 2 < local_c) {
        local_c = *(int *)((longlong)*_refptr_g_prevTouchesPtr +
                          (longlong)(int)(uint)param_2 * 0x478 + 0x1c0) / 2;
      }
      if ((*(int *)(*_refptr_g_peaksPtr +
                   (longlong)
                   (int)(uint)(byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x23c] *
                   0x14 + 0x20) < local_c) ||
         (*(short *)(*_refptr_g_peaksPtr +
                    (longlong)
                    (int)(uint)(byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x23c] *
                    0x14 + 0x18) < local_10)) {
        *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 500) =
             *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 500) | 1;
      }
    }
  }
  else {
    local_c = *(int *)(_refptr_g_tsaPrmtDynamic + 0x10) * 3;
    if (local_c < 0) {
      local_c = local_c + 3;
    }
    local_c = local_c >> 2;
    iVar1 = *(short *)(_refptr_g_tsaPrmtDynamic + 0xc) * 3;
    if (iVar1 < 0) {
      iVar1 = iVar1 + 3;
    }
    local_e = (short)(iVar1 >> 2);
    if (param_3 != '\0') {
      local_c = local_c * 3;
      if (local_c < 0) {
        local_c = local_c + 3;
      }
      local_c = local_c >> 2;
      iVar1 = *(short *)(_refptr_g_tsaPrmtDynamic + 0xe) * 3;
      if (iVar1 < 0) {
        iVar1 = iVar1 + 3;
      }
      local_e = (short)(iVar1 >> 2);
    }
    if ((*(int *)(*_refptr_g_peaksPtr +
                 (longlong)
                 (int)(uint)(byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x23c] * 0x14
                 + 0x20) < local_c) ||
       (*(short *)(*_refptr_g_peaksPtr +
                  (longlong)
                  (int)(uint)(byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x23c] * 0x14
                  + 0x18) < local_e)) {
      *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 500) =
           *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 500) | 1;
    }
  }
  return;
}


// ============================================================================
// Function: Touch_SetVolatileFlagStableTouch @ 6ba70775
// ============================================================================

void Touch_SetVolatileFlagStableTouch(byte param_1)

{
  int local_c;
  
  if (((_refptr_g_tsaPrpt[1].field_0x28 == '\0') ||
      ((((_refptr_g_tsaPrmtDynamic[0x80] & 4) == 0 ||
        (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 1) == 0)) &&
       (((_refptr_g_tsaPrmtDynamic[0x80] & 8) == 0 ||
        (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x10) == 0)))))) &&
     ((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) == 0 &&
      ((*_refptr_g_tsaStaticPtr)[1].field_0x3 == '\0')))) {
    local_c = *(int *)(_refptr_g_tsaPrmtDynamic + 0x14);
  }
  else {
    local_c = *(int *)(_refptr_g_tsaPrmtDynamic + 0x10);
  }
  if (local_c * 3 <
      *(int *)(*_refptr_g_peaksPtr +
              (longlong)(int)(uint)(byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x23c]
              * 0x14 + 0x20)) {
    *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f0) =
         *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f0) | 0x1000;
  }
  return;
}


// ============================================================================
// Function: Touch_UpdateStableProperty @ 6ba708d8
// ============================================================================

void Touch_UpdateStableProperty(byte param_1,byte param_2)

{
  ushort uVar1;
  ushort uVar2;
  short sVar3;
  int iVar4;
  
  if (*(ushort *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x156) < 0xb) {
    *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x180) =
         *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x162);
    *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x182) =
         *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x164);
    *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x184) =
         *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x188);
  }
  else {
    uVar2 = Touch_GetSQDistMM(*(undefined2 *)
                               (g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x162),
                              *(undefined2 *)
                               (g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x164),
                              *(undefined2 *)
                               ((longlong)*_refptr_g_prevTouchesPtr +
                               (longlong)(int)(uint)param_2 * 0x478 + 0x180),
                              *(undefined2 *)
                               ((longlong)*_refptr_g_prevTouchesPtr +
                               (longlong)(int)(uint)param_2 * 0x478 + 0x182));
    if (uVar2 < 0x19) {
      *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x180) =
           *(undefined2 *)
            ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x180);
      *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x182) =
           *(undefined2 *)
            ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x182);
      *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x184) =
           *(undefined2 *)
            ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x184);
      g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x186] = g_touchesPtr[1];
    }
    else {
      iVar4 = (uint)*(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x188) -
              (uint)*(ushort *)
                     ((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)param_2 * 0x478 + 0x184);
      uVar1 = (ushort)(iVar4 >> 0x1f);
      sVar3 = ((ushort)iVar4 ^ uVar1) - uVar1;
      if (((((ushort)(uVar2 >> 1) <= (ushort)(sVar3 * sVar3)) &&
           (0x14 < *(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x188))) &&
          (*(int *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_2 * 0x478 + 0x1ec) == 8)) &&
         ((g_touchesPtr[1] == '\x01' &&
          (*(char *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_2 * 0x478 + 0x186) == g_touchesPtr[1])))) {
        *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f0) =
             *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f0) | 0x200000;
      }
      *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x180) =
           *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x162);
      *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x182) =
           *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x164);
      *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x184) =
           *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x188);
      g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x186] = g_touchesPtr[1];
    }
  }
  return;
}


// ============================================================================
// Function: Touch_UpdateHandEnum @ 6ba70df0
// ============================================================================

void Touch_UpdateHandEnum(byte param_1,byte param_2)

{
  char cVar1;
  
  cVar1 = TSAOut_Angle((int)(char)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x193]);
  if (('\x0f' < cVar1) &&
     (cVar1 = TSAOut_Angle((int)(char)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x193]),
     cVar1 < 'K')) {
    *(undefined4 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1e4) = 4;
    return;
  }
  cVar1 = TSAOut_Angle((int)(char)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x193]);
  if ((cVar1 < -0xf) &&
     (cVar1 = TSAOut_Angle((int)(char)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x193]),
     -0x4b < cVar1)) {
    *(undefined4 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1e4) = 8;
    return;
  }
  *(undefined4 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1e4) =
       *(undefined4 *)
        ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x1e4);
  return;
}


// ============================================================================
// Function: Touch_IsPeakOnInnerDim1Edge @ 6ba70f56
// ============================================================================

undefined8 Touch_IsPeakOnInnerDim1Edge(byte param_1)

{
  undefined8 uVar1;
  
  if (((*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_1 * 0x14 + 0x15].bPeakCount == '\x01') ||
     ((uint)(*_refptr_g_peaksPtr)[(longlong)(int)(uint)param_1 * 0x14 + 0x15].bPeakCount ==
      _refptr_g_tsaPrmtFlash->bCols - 2)) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: Touch_SetVolatileFlagFalseFlickDetected @ 6ba71f3a
// ============================================================================

void Touch_SetVolatileFlagFalseFlickDetected(byte param_1,byte param_2)

{
  byte bVar1;
  byte bVar2;
  ushort uVar3;
  int iVar4;
  int iVar5;
  char *pcVar6;
  uint uVar7;
  uint uVar8;
  uint local_c;
  
  bVar1 = (*_refptr_g_peaksPtr)
          [(longlong)(int)(uint)(byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x23c] *
           0x14 + 0x15].bPeakCount;
  bVar2 = (*_refptr_g_peaksPtr)
          [(longlong)(int)(uint)(byte)g_touchesPtr[(longlong)(int)(uint)param_1 * 0x168 + 0x23c] *
           0x14 + 0x16].bPeakCount;
  if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x20) != 0) {
    pcVar6 = (char *)TSAPrmt_GetProjectName();
    iVar4 = strncmp(pcVar6,"W169",4);
    if (iVar4 != 0) {
      pcVar6 = (char *)TSAPrmt_GetProjectName();
      iVar4 = strncmp(pcVar6,"W189",4);
      if (iVar4 != 0) goto LAB_6ba7208b;
    }
    if ((((bVar1 < 2) || ((int)(_refptr_g_tsaPrmtFlash->bCols - 2) <= (int)(uint)bVar1)) ||
        (bVar2 < 2)) || ((int)(_refptr_g_tsaPrmtFlash->bRows - 2) <= (int)(uint)bVar2)) {
      uVar3 = TS_MMToPixel(2);
      goto LAB_6ba7209b;
    }
  }
LAB_6ba7208b:
  uVar3 = TS_MMToPixel(1);
LAB_6ba7209b:
  local_c = (uint)uVar3;
  if (local_c * local_c >> 1 <
      *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x244)) {
    uVar7 = (uint)*(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x188) -
            (uint)*(ushort *)
                   ((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_2 * 0x478 + 0x188);
    uVar8 = (int)uVar7 >> 0x1f;
    iVar4 = (uVar7 ^ uVar8) - uVar8;
    uVar7 = (uint)*(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x18a) -
            (uint)*(ushort *)
                   ((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_2 * 0x478 + 0x18a);
    uVar8 = (int)uVar7 >> 0x1f;
    iVar5 = (uVar7 ^ uVar8) - uVar8;
    uVar7 = TS_SQMMToSQPixel(iVar5 * iVar5 + iVar4 * iVar4 & 0xffff);
    if (*(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x244) < uVar7 >> 2) {
      *(undefined2 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4a4) =
           *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x162);
      *(undefined2 *)
       ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4a6) =
           *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x164);
      *(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4ac)
           = (*(short *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x162) -
             *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_2 * 0x478 + 0x162)) +
             *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_2 * 0x478 + 0x4ac);
      *(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4ae)
           = (*(short *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x164) -
             *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_2 * 0x478 + 0x164)) +
             *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_2 * 0x478 + 0x4ae);
      *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f0) =
           *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f0) | 0x100000;
    }
  }
  return;
}


// ============================================================================
// Function: Touch_SetPersistFlagMoveOverClickSuppressRange @ 6ba723e9
// ============================================================================

void Touch_SetPersistFlagMoveOverClickSuppressRange(byte param_1,byte param_2)

{
  ushort uVar1;
  uint uVar2;
  
  uVar2 = Touch_GetSQDistPixel
                    (*(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x162),
                     *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x164),
                     *(undefined2 *)
                      ((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_2 * 0x478 + 0x4a4),
                     *(undefined2 *)
                      ((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_2 * 0x478 + 0x4a6));
  uVar1 = TSAOut_ExtToIntDist(_refptr_g_tsaPrmtDynamic[0x78]);
  if ((uint)uVar1 * (uint)uVar1 < uVar2) {
    *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f8) =
         *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f8) | 0x100;
  }
  return;
}


// ============================================================================
// Function: Touch_SetPersistFlagMoveOverMaxClickSuppressRange @ 6ba72518
// ============================================================================

void Touch_SetPersistFlagMoveOverMaxClickSuppressRange(byte param_1,byte param_2)

{
  ushort uVar1;
  uint uVar2;
  
  uVar2 = Touch_GetSQDistPixel
                    (*(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x162),
                     *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x164),
                     *(undefined2 *)
                      ((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_2 * 0x478 + 0x49a),
                     *(undefined2 *)
                      ((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_2 * 0x478 + 0x49c));
  uVar1 = TSAOut_ExtToIntDist(_refptr_g_tsaPrmtDynamic[0x7a]);
  if ((uint)uVar1 * (uint)uVar1 < uVar2) {
    *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f8) =
         *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f8) | 0x8000;
  }
  return;
}


// ============================================================================
// Function: Touch_SetPersistFlagMoveOverFlickDistance @ 6ba72647
// ============================================================================

void Touch_SetPersistFlagMoveOverFlickDistance(byte param_1,byte param_2)

{
  ushort uVar1;
  uint uVar2;
  
  uVar2 = Touch_GetSQDistPixel
                    (*(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x162),
                     *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x164),
                     *(undefined2 *)
                      ((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_2 * 0x478 + 0x4ac),
                     *(undefined2 *)
                      ((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_2 * 0x478 + 0x4ae));
  uVar1 = TSAOut_ExtToIntDist(_refptr_g_tsaPrmtDynamic[0x7c]);
  if ((uint)uVar1 * (uint)uVar1 < uVar2) {
    *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f8) =
         *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f8) | 0x1000;
  }
  return;
}


// ============================================================================
// Function: Touch_IsFarAwayFromCornerCompareToStartPoint @ 6ba74aef
// ============================================================================

bool Touch_IsFarAwayFromCornerCompareToStartPoint(byte param_1)

{
  return *(int *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1d4) / 2 <=
         *(int *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1cc);
}


// ============================================================================
// Function: Touch_ValidComparison @ 6ba7435e
// ============================================================================

undefined8 Touch_ValidComparison(byte param_1,byte param_2)

{
  bool bVar1;
  undefined8 uVar2;
  
                    /* 0x3435e  2554  Touch_ValidComparison */
  if (((*(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f0) & 4) == 0) ||
     ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x1f0)
      & 4) == 0)) {
    bVar1 = false;
  }
  else {
    bVar1 = true;
  }
  if ((*(int *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1e0) ==
       *(int *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x1e0))
     && (bVar1)) {
    uVar2 = 1;
  }
  else {
    uVar2 = 0;
  }
  return uVar2;
}


// ============================================================================
// Function: Touch_IsGhostByMinSigOff @ 6ba7160b
// ============================================================================

undefined8 Touch_IsGhostByMinSigOff(byte param_1)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0x3160b  2536  Touch_IsGhostByMinSigOff */
  cVar1 = Peak_IsTouchValid(param_1);
  if (cVar1 == '\0') {
    uVar2 = 0;
  }
  else if ((int)*(short *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1ba) <
           *(short *)((ulonglong)
                      ((uint)(*_refptr_g_peaksPtr)
                             [(longlong)
                              (int)(uint)(byte)g_touchesPtr
                                               [(longlong)(int)(uint)param_1 * 0x168 + 0x23c] * 0x14
                              + 0x15].bPeakCount +
                       (uint)(*_refptr_g_peaksPtr)
                             [(longlong)
                              (int)(uint)(byte)g_touchesPtr
                                               [(longlong)(int)(uint)param_1 * 0x168 + 0x23c] * 0x14
                              + 0x16].bPeakCount * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2
                     + g_tsaBufMinSigOff) * 2) {
    uVar2 = 1;
  }
  else {
    uVar2 = 0;
  }
  return uVar2;
}


// ============================================================================
// Function: Touch_GetSQDistMMInBetween @ 6ba73515
// ============================================================================

void Touch_GetSQDistMMInBetween(byte param_1,byte param_2)

{
                    /* 0x33515  2527  Touch_GetSQDistMMInBetween */
  Touch_GetSQDistMM(*(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x162),
                    *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x164),
                    *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_2 * 0x168 + 0x162),
                    *(undefined2 *)(g_touchesPtr + (longlong)(int)(uint)param_2 * 0x168 + 0x164));
  return;
}


// ============================================================================
// Function: Touch_ToeUseSpecialZ4TholdEdge @ 6ba6fe1a
// ============================================================================

undefined8 Touch_ToeUseSpecialZ4TholdEdge(void)

{
  int iVar1;
  char *pcVar2;
  
  pcVar2 = (char *)TSA_GetPrmtName();
  iVar1 = strncmp(pcVar2,"B206",4);
  if (iVar1 != 0) {
    pcVar2 = (char *)TSA_GetPrmtName();
    iVar1 = strncmp(pcVar2,"B20Y",4);
    if (iVar1 != 0) {
      return 0;
    }
  }
  return 1;
}


// ============================================================================
// Function: Touch_NeedXYFilter @ 6ba742dc
// ============================================================================

bool Touch_NeedXYFilter(byte param_1,byte param_2)

{
  bool local_9;
  
                    /* 0x342dc  2545  Touch_NeedXYFilter */
  local_9 = (*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_2 * 0x478 + 0x1f8) & 0x40) != 0 ||
            (*(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x21c) & 0x14) != 0;
  return local_9;
}


// ============================================================================
// Function: Touch_GetTimeOn @ 6ba6f517
// ============================================================================

int Touch_GetTimeOn(byte param_1)

{
                    /* 0x2f517  2530  Touch_GetTimeOn */
  return (int)*(short *)((ulonglong)
                         ((uint)(byte)((ushort)*(undefined2 *)
                                                (g_touchesPtr +
                                                (longlong)(int)(uint)param_1 * 0x168 + 0x162) >> 8)
                          + (uint)(byte)((ushort)*(undefined2 *)
                                                  (g_touchesPtr +
                                                  (longlong)(int)(uint)param_1 * 0x168 + 0x164) >> 8
                                        ) * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
                        g_tsaBufTimeDown);
}


// ============================================================================
// Function: Touch_IsFullyLiftOff @ 6ba717cf
// ============================================================================

bool Touch_IsFullyLiftOff(byte param_1)

{
                    /* 0x317cf  2535  Touch_IsFullyLiftOff */
  return *(short *)((ulonglong)
                    ((uint)(*_refptr_g_peaksPtr)
                           [(longlong)
                            (int)(uint)(byte)g_touchesPtr
                                             [(longlong)(int)(uint)param_1 * 0x168 + 0x23c] * 0x14 +
                            0x15].bPeakCount +
                     (uint)(*_refptr_g_peaksPtr)
                           [(longlong)
                            (int)(uint)(byte)g_touchesPtr
                                             [(longlong)(int)(uint)param_1 * 0x168 + 0x23c] * 0x14 +
                            0x16].bPeakCount * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
                   g_tsaBufMinSigOff) <= *(short *)(_refptr_g_tsaPrmtDynamic + 8) / 2;
}


