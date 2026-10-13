// ============================================================================
// Function: GripFilter_Process @ 6bae1d3c
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* Grip post-filter stage. Current config has grip feature bit 0x20 enabled, so merge
   handling/report/idle processing run after basic per-touch grip filtering. */

void GripFilter_Process(void)

{
  undefined1 bPrevTouchIndex;
  char cMergeHandlerEnabled;
  byte bTouchIndex;
  
                    /* 0xa1d3c  781  GripFilter_Process */
  GripFilter_GetPropertyGlobal();
  for (bTouchIndex = 0; bTouchIndex < **(byte **)_refptr_g_touchesPtr; bTouchIndex = bTouchIndex + 1
      ) {
    if ((*(uint *)((longlong)(int)(uint)bTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                  0x1f8) & 0x200000) == 0) {
      bPrevTouchIndex = PrevTouch_GetIdxBasedOnCurIdx(bTouchIndex);
      GripFilter_BasicProcess(bTouchIndex,bPrevTouchIndex);
    }
  }
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x20) != 0) {
    cMergeHandlerEnabled = GripFilter_IsMergeHandlerEnabled();
    if (cMergeHandlerEnabled != '\0') {
      TSABuffer_GripStageProcess();
    }
    GripFilter_Report();
    GripFilter_IDLEProcess();
  }
  return;
}


// ============================================================================
// Function: GripFilter_RegionProcess @ 6baec71c
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

void GripFilter_RegionProcess(void)

{
                    /* 0xac71c  783  GripFilter_RegionProcess */
  GripFilter_RegionReset();
  if (*(int *)&_refptr_g_tsaPrmtFlash[1].field_0x84 == 4) {
    GripFilter_RegionSetByScreenRotationPad();
  }
  else if (*(int *)&_refptr_g_tsaPrmtFlash[1].field_0x84 == 5) {
    GripFilter_RegionSetByScreenRotationFold();
  }
  else {
    GripFilter_RegionSetByScreenRotation();
  }
  GripFilter_RegionSetByHandGesture();
  return;
}


// ============================================================================
// Function: GripFilter_IsGripTouch @ 6bae19d1
// ============================================================================

bool GripFilter_IsGripTouch(byte param_1)

{
  char cVar1;
  
                    /* 0xa19d1  716  GripFilter_IsGripTouch */
  cVar1 = GripFilter_IsGrip(*(undefined4 *)
                             ((longlong)(int)(uint)param_1 * 0x168 +
                              *(longlong *)_refptr_g_touchesPtr + 0x210));
  return cVar1 != '\0';
}


// ============================================================================
// Function: GripFilter_IsMergeHandlerEnabled @ 6bae18c4
// ============================================================================

undefined8 GripFilter_IsMergeHandlerEnabled(void)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0xa18c4  734  GripFilter_IsMergeHandlerEnabled */
  cVar1 = GripFilter_IsMergeHandlerEdgeEnabled(0);
  if (cVar1 == '\0') {
    cVar1 = GripFilter_IsMergeHandlerEdgeEnabled(1);
    if (cVar1 == '\0') {
      cVar1 = GripFilter_IsMergeHandlerEdgeEnabled(2);
      if (cVar1 == '\0') {
        cVar1 = GripFilter_IsMergeHandlerEdgeEnabled(3);
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
// Function: GripFilter_Report @ 6bae1a5b
// ============================================================================

void GripFilter_Report(void)

{
  char cVar1;
  byte local_9;
  
  for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
    cVar1 = GripFilter_IsGripTouch(local_9);
    if (cVar1 == '\0') {
      *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) =
           *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1f0) & 0xf7ffffff;
      if ((*(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1fc) & 0x40000000) != 0) {
        cVar1 = TSAStatic_IsAFTModeEnabled();
        if (cVar1 == '\0') {
          Touch_CreatePostReport(local_9,5);
        }
      }
    }
    else {
      *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) =
           *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1f0) | 0x8000000;
    }
  }
  return;
}


// ============================================================================
// Function: GripFilter_IDLEProcess @ 6bae1c75
// ============================================================================

void GripFilter_IDLEProcess(void)

{
  bool bVar1;
  char cVar2;
  byte local_a;
  
                    /* 0xa1c75  694  GripFilter_IDLEProcess */
  if (_refptr_g_tsaPrmtFlash[1].field_0x82 == '\0') {
    bVar1 = false;
  }
  else if (**(char **)_refptr_g_touchesPtr == '\0') {
    bVar1 = false;
  }
  else {
    bVar1 = true;
    for (local_a = 0; local_a < **(byte **)_refptr_g_touchesPtr; local_a = local_a + 1) {
      cVar2 = GripFilter_IsAbsGrip(local_a);
      if (cVar2 == '\0') {
        bVar1 = false;
        break;
      }
    }
  }
  if (!bVar1) {
    g_timeStampInActive = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  }
  g_isOKToIDLE = 500 < (ulonglong)
                       ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
                       (longlong)g_timeStampInActive);
  return;
}


// ============================================================================
// Function: GripFilter_BasicProcess @ 6bae4ab3
// ============================================================================

void GripFilter_BasicProcess(byte param_1,byte param_2)

{
  char cVar1;
  
                    /* 0xa4ab3  654  GripFilter_BasicProcess */
  cVar1 = TSA_IsJustEnterTypingScenario();
  if (cVar1 != '\0') {
    DAT_6bc84e22 = '\0';
  }
  cVar1 = HandGesture_Is2HandDetected();
  if (cVar1 != '\0') {
    DAT_6bc84e22 = '\0';
  }
  g_grip = param_1;
  DAT_6bc84e21 = param_2;
  DAT_6bc84e24 = GripFilter_GetEdge();
  GripFilter_GetPrmt();
  GripFilter_GetProperty();
  GripFilter_GetQuality();
  GripFilter_GetStage();
  GripFilter_UpdateDeadzone();
  if (((*(int *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x210) ==
        1) && (*(int *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)DAT_6bc84e21 * 0x478 + 0x210) != 1)) &&
     (DAT_6bc84e22 != -1)) {
    DAT_6bc84e22 = DAT_6bc84e22 + '\x01';
  }
  if ((*(int *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x210) ==
       1) || (*(int *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x210) == 2)) {
    *(char *)(*(longlong *)_refptr_g_touchesPtr + 0xd) =
         *(char *)(*(longlong *)_refptr_g_touchesPtr + 0xd) + '\x01';
  }
  if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) &
      0x100000) != 0) {
    cVar1 = GripFilter_IsGrip(*(undefined4 *)
                               ((longlong)*_refptr_g_prevTouchesPtr +
                               (longlong)(int)(uint)param_2 * 0x478 + 0x210));
    if (cVar1 != '\0') {
      *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1fc) =
           *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1fc) | 0x40000000;
    }
  }
  return;
}


// ============================================================================
// Function: GripFilter_GetPropertyGlobal @ 6bae990d
// ============================================================================

void GripFilter_GetPropertyGlobal(void)

{
  longlong lVar1;
  char cVar2;
  undefined4 uVar3;
  byte local_19;
  
                    /* 0xa990d  678  GripFilter_GetPropertyGlobal */
  for (local_19 = 0; local_19 < **(byte **)_refptr_g_touchesPtr; local_19 = local_19 + 1) {
    *_refptr_g_grip = local_19;
    _refptr_g_grip[1] =
         *(undefined1 *)
          ((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x287);
    lVar1 = *(longlong *)_refptr_g_touchesPtr;
    uVar3 = TSA_GetTouchSide(local_19);
    *(undefined4 *)((longlong)(int)(uint)local_19 * 0x168 + lVar1 + 0x1c4) = uVar3;
    uVar3 = GripFilter_GetEdge();
    *(undefined4 *)(_refptr_g_grip + 4) = uVar3;
    GripFilter_GetShapeFlag();
    if (*(ushort *)
         ((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x156) < 4) {
      cVar2 = GripFilter_IsInMultiGripRegion(local_19);
      if (cVar2 != '\0') {
        *(uint *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214)
             = *(uint *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x214) | 4;
      }
    }
    cVar2 = GripFilter_IsGripMergeAllowed();
    if (cVar2 == '\0') {
      if (((*(uint *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x218) & 1) != 0) ||
         ((*(uint *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x214) & 0x200000) != 0)) {
        *(uint *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x218)
             = *(uint *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x218) | 0x10;
        *(uint *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214)
             = *(uint *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x214) & 0xfffff7ff;
      }
      *(uint *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x218) =
           *(uint *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x218) & 0xfffffffe;
      *(uint *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214) =
           *(uint *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x214) & 0xffdfffff;
    }
    else if ((*(uint *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x218) & 1) != 0) {
      cVar2 = Touch_IsOff(local_19);
      if (cVar2 == '\0') {
        cVar2 = GripFilter_IsPeakAbsNotGrip
                          (*(undefined1 *)
                            ((longlong)(int)(uint)local_19 * 0x168 +
                             *(longlong *)_refptr_g_touchesPtr + 0x23c));
        if (cVar2 != '\0') {
          *(uint *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                   0x214) =
               *(uint *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x214) | 0x200000;
        }
      }
    }
    cVar2 = GripFilter_GetCenterLevel(local_19);
    if ((cVar2 != '\0') ||
       (*(int *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1cc)
        < *(int *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1d0
                  ) * 2)) {
      *(uint *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214) =
           *(uint *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x214) | 0x80000;
    }
    cVar2 = Touch_IsOff(*_refptr_g_grip);
    if (((cVar2 == '\0') &&
        (*(char *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                   *(longlong *)_refptr_g_touchesPtr + 0x1ac) == '\x01')) &&
       (2 < *(byte *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                      *(longlong *)_refptr_g_touchesPtr + 0x1ad))) {
      cVar2 = Peak_IsOnInnerEdgeDim1
                        (*(undefined1 *)
                          ((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                           *(longlong *)_refptr_g_touchesPtr + 0x23c));
      if (cVar2 != '\0') {
        *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                  *(longlong *)_refptr_g_touchesPtr + 0x218) =
             *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                       *(longlong *)_refptr_g_touchesPtr + 0x218) | 8;
      }
    }
  }
  return;
}


// ============================================================================
// Function: GripFilter_RegionReset @ 6bae9e90
// ============================================================================

void GripFilter_RegionReset(void)

{
  int iVar1;
  
                    /* 0xa9e90  784  GripFilter_RegionReset */
  iVar1 = memset_s(*(undefined8 *)_refptr_g_tsaBufGripRegion,*(undefined2 *)_refptr_g_gridBufSize,0,
                   _refptr_g_tsaPrmtFlash->wBufElementCount);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/GripFilter/GripFilter_Region.c",0x2e,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: GripFilter_RegionSetByScreenRotation @ 6baebde9
// ============================================================================

void GripFilter_RegionSetByScreenRotation(void)

{
  char cVar1;
  int iVar2;
  char *_Str1;
  
                    /* 0xabde9  789  GripFilter_RegionSetByScreenRotation */
  GripFilter_RegionSet(0xc,2,0);
  GripFilter_RegionSet(0xc,1,1);
  if ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 1) {
    GripFilter_RegionSet(0x10,3,0);
    GripFilter_RegionSet(0x40,3,0);
    cVar1 = GripFilter_Is2HandGame();
    if ((cVar1 == '\0') && (*(int *)&_refptr_g_tsaPrmtFlash[1].field_0x84 != 4)) {
      GripFilter_RegionSet(0x20,3,0);
      GripFilter_RegionSet(0x80,3,0);
    }
  }
  else if ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 3) {
    GripFilter_RegionSet(0x20,3,0);
    GripFilter_RegionSet(0x80,3,0);
    cVar1 = GripFilter_Is2HandGame();
    if ((cVar1 == '\0') && (*(int *)&_refptr_g_tsaPrmtFlash[1].field_0x84 != 4)) {
      GripFilter_RegionSet(0x10,3,0);
      GripFilter_RegionSet(0x40,3,0);
    }
  }
  else if ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 2) {
    GripFilter_RegionSet(0x10,3,0);
    GripFilter_RegionSet(0x20,3,0);
  }
  else if ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 0) {
    GripFilter_RegionSet(0x40,3,0);
    GripFilter_RegionSet(0x80,3,0);
    GripFilter_RegionSet(0x400,3,0);
    GripFilter_RegionSet(0x800,3,0);
  }
  if (((*_refptr_g_tsaStaticPtr)->nField_0x264 == 0) ||
     ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 2)) {
    GripFilter_RegionSet(3,1,0);
    if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) != 0) {
      cVar1 = TSA_IsTypingScenario();
      if (cVar1 == '\0') {
        GripFilter_RegionSet(0x400,3,1);
        GripFilter_RegionSet(0x800,3,1);
      }
      else {
        GripFilter_RegionSet(0x400,2,1);
        GripFilter_RegionSet(0x800,2,1);
      }
    }
  }
  else {
    GripFilter_RegionSet(3,2,0);
    _Str1 = (char *)TSA_GetPrmtName();
    iVar2 = strncmp(_Str1,"P212",4);
    if (iVar2 == 0) {
      GripFilter_RegionSet(3,2,1);
    }
    if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) != 0) {
      GripFilter_RegionSet(0xc,2,1);
    }
  }
  return;
}


// ============================================================================
// Function: GripFilter_RegionSetByScreenRotationPad @ 6baec121
// ============================================================================

void GripFilter_RegionSetByScreenRotationPad(void)

{
  int iVar1;
  char cVar2;
  
                    /* 0xac121  791  GripFilter_RegionSetByScreenRotationPad */
  iVar1 = (*_refptr_g_tsaStaticPtr)->nField_0x264;
  if ((iVar1 == 1) || (iVar1 == 3)) {
    GripFilter_RegionSet(3,2,0);
    GripFilter_RegionSet(3,1,1);
    GripFilter_RegionSet(0xc,1,0);
    if (iVar1 == 1) {
      GripFilter_RegionSet(0x10,3,0);
      GripFilter_RegionSet(0x40,3,0);
      GripFilter_RegionSet(0x1000,3,0);
      GripFilter_RegionSet(0x4000,3,0);
      cVar2 = GripFilter_Is2HandGame();
      if ((cVar2 == '\0') && (*(int *)&_refptr_g_tsaPrmtFlash[1].field_0x84 != 4)) {
        GripFilter_RegionSet(0x20,3,0);
        GripFilter_RegionSet(0x80,3,0);
      }
    }
    else if (iVar1 == 3) {
      GripFilter_RegionSet(0x20,3,0);
      GripFilter_RegionSet(0x80,3,0);
      GripFilter_RegionSet(0x2000,3,0);
      GripFilter_RegionSet(0x8000,3,0);
      cVar2 = GripFilter_Is2HandGame();
      if ((cVar2 == '\0') && (*(int *)&_refptr_g_tsaPrmtFlash[1].field_0x84 != 4)) {
        GripFilter_RegionSet(0x10,3,0);
        GripFilter_RegionSet(0x40,3,0);
      }
    }
  }
  else {
    GripFilter_RegionSet(3,1,0);
    GripFilter_RegionSet(0xc,2,0);
    GripFilter_RegionSet(0xc,1,1);
    if (iVar1 == 2) {
      GripFilter_RegionSet(0x10,3,0);
      GripFilter_RegionSet(0x20,3,0);
      GripFilter_RegionSet(0x100,3,0);
      GripFilter_RegionSet(0x200,3,0);
    }
    else if (iVar1 == 0) {
      GripFilter_RegionSet(0x40,3,0);
      GripFilter_RegionSet(0x80,3,0);
      GripFilter_RegionSet(0x400,3,0);
      GripFilter_RegionSet(0x800,3,0);
    }
  }
  return;
}


// ============================================================================
// Function: GripFilter_RegionSetByScreenRotationFold @ 6baec3e4
// ============================================================================

void GripFilter_RegionSetByScreenRotationFold(void)

{
  char cVar1;
  
                    /* 0xac3e4  790  GripFilter_RegionSetByScreenRotationFold */
  GripFilter_RegionSet(3,2,0);
  GripFilter_RegionSet(3,1,1);
  if ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 1) {
    GripFilter_RegionSet(0x40,3,0);
    GripFilter_RegionSet(0x80,3,0);
    cVar1 = GripFilter_Is2HandGame();
    if (cVar1 == '\0') {
      GripFilter_RegionSet(0x20,3,0);
      GripFilter_RegionSet(0x10,3,0);
    }
  }
  else if ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 3) {
    GripFilter_RegionSet(0x20,3,0);
    GripFilter_RegionSet(0x10,3,0);
    cVar1 = GripFilter_Is2HandGame();
    if (cVar1 == '\0') {
      GripFilter_RegionSet(0x80,3,0);
      GripFilter_RegionSet(0x40,3,0);
    }
  }
  else if ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 2) {
    GripFilter_RegionSet(0x10,3,0);
    GripFilter_RegionSet(0x40,3,0);
  }
  else if ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 0) {
    GripFilter_RegionSet(0x20,3,0);
    GripFilter_RegionSet(0x80,3,0);
    GripFilter_RegionSet(0x8000,3,0);
    GripFilter_RegionSet(0x2000,3,0);
  }
  if (((*_refptr_g_tsaStaticPtr)->nField_0x264 == 0) ||
     ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 2)) {
    GripFilter_RegionSet(0xc,1,0);
  }
  else {
    GripFilter_RegionSet(0xc,2,0);
  }
  return;
}


// ============================================================================
// Function: GripFilter_RegionSetByHandGesture @ 6baec611
// ============================================================================

void GripFilter_RegionSetByHandGesture(void)

{
  ushort local_a;
  
                    /* 0xac611  788  GripFilter_RegionSetByHandGesture */
  for (local_a = 0; local_a < _refptr_g_tsaPrmtFlash->wBufElementCount; local_a = local_a + 1) {
    if (*(char *)((ulonglong)local_a + *(longlong *)_refptr_g_tsaBufTzPrevHand) != '\0') {
      *(byte *)(*(longlong *)_refptr_g_tsaBufGripRegion + (ulonglong)local_a) =
           *(byte *)((ulonglong)local_a + *(longlong *)_refptr_g_tsaBufGripRegion) | 8;
    }
  }
  return;
}


// ============================================================================
// Function: GripFilter_Log @ 6bae1b85
// ============================================================================

void GripFilter_Log(void)

{
  undefined1 uVar1;
  
                    /* 0xa1b85  778  GripFilter_Log */
  uVar1 = GripFilter_Is2HandGame();
  StrPrintf_Process(0,"\nis2HandGame = %d",uVar1);
  uVar1 = GripFilter_IsVideoMode();
  StrPrintf_Process(0," isVideoMode = %d",uVar1);
  return;
}


// ============================================================================
// Function: GripFilter_IsToCancelIDMapping @ 6bae8858
// ============================================================================

bool GripFilter_IsToCancelIDMapping(byte param_1,byte param_2)

{
  char cVar1;
  
                    /* 0xa8858  768  GripFilter_IsToCancelIDMapping */
  cVar1 = GripFilter_IsPeakAbsNotGrip
                    (*(undefined1 *)
                      ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x23c));
  if (((cVar1 != '\0') &&
      (cVar1 = GripFilter_IsGrip(*(undefined4 *)
                                  ((longlong)*_refptr_g_prevTouchesPtr +
                                  (longlong)(int)(uint)param_2 * 0x478 + 0x210)), cVar1 != '\0')) &&
     ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x218) &
      1) != 0)) {
    return true;
  }
  cVar1 = GripFilter_IsMerged(param_1,param_2);
  return cVar1 != '\0';
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
// Function: GripFilter_IsOKToEnterIDLE @ 6bae1bcc
// ============================================================================

undefined1 GripFilter_IsOKToEnterIDLE(void)

{
                    /* 0xa1bcc  739  GripFilter_IsOKToEnterIDLE */
  return g_isOKToIDLE;
}


// ============================================================================
// Function: GripFilter_IsGrip @ 6bae193f
// ============================================================================

undefined8 GripFilter_IsGrip(int param_1)

{
  undefined8 uVar1;
  
                    /* 0xa193f  711  GripFilter_IsGrip */
  if (((param_1 == 0) || (param_1 == 1)) || (param_1 == 2)) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: GripFilter_IsMergeHandlerEdgeEnabled @ 6bae1763
// ============================================================================

bool GripFilter_IsMergeHandlerEdgeEnabled(uint param_1)

{
                    /* 0xa1763  733  GripFilter_IsMergeHandlerEdgeEnabled */
  return (*(uint *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                   (ulonglong)param_1 * 0xc + -0x10) & 2) != 0;
}


// ============================================================================
// Function: GripFilter_IsAbsGrip @ 6bae1bd9
// ============================================================================

undefined8 GripFilter_IsAbsGrip(byte param_1)

{
  char cVar1;
  
                    /* 0xa1bd9  698  GripFilter_IsAbsGrip */
  if (*(int *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x210) ==
      1) {
    return 1;
  }
  cVar1 = GripFilter_IsGripOrTbdTouch(param_1);
  if (cVar1 != '\0') {
    if (200 < *(ushort *)
               ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x156)) {
      return 1;
    }
    cVar1 = Touch_IsGhostByMinSigOff2(param_1);
    if (cVar1 != '\0') {
      return 1;
    }
  }
  return 0;
}


// ============================================================================
// Function: GripFilter_GetQuality @ 6bae42bc
// ============================================================================

void GripFilter_GetQuality(void)

{
  char cVar1;
  
                    /* 0xa42bc  679  GripFilter_GetQuality */
  cVar1 = BLReset_Status();
  if (cVar1 == '\x01') {
    DAT_6bc84e28 = 3;
  }
  else {
    cVar1 = GripFilter_BasicIsAbsNotGrip();
    if (cVar1 == '\0') {
      cVar1 = GripFilter_BasicIsAbsGrip();
      if (cVar1 == '\0') {
        cVar1 = GripFilter_BasicIsTempGrip();
        if (cVar1 == '\0') {
          *(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214)
               = *(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                          0x214) | 0x80;
          DAT_6bc84e28 = 1;
        }
        else {
          DAT_6bc84e28 = 2;
        }
      }
      else {
        DAT_6bc84e28 = 3;
      }
    }
    else {
      DAT_6bc84e28 = 0;
    }
  }
  return;
}


// ============================================================================
// Function: GripFilter_GetPrmt @ 6bae43a3
// ============================================================================

void GripFilter_GetPrmt(void)

{
  byte bVar1;
  
                    /* 0xa43a3  676  GripFilter_GetPrmt */
  bVar1 = GripFilter_GetCenterLevel(g_grip);
  if (bVar1 < 4) {
    DAT_6bc84e30 = &gripPrmt + (longlong)(int)(uint)bVar1 * 0x18;
  }
  else {
    DAT_6bc84e30 = &gripPrmt;
  }
  return;
}


// ============================================================================
// Function: GripFilter_GetStage @ 6bae492d
// ============================================================================

void GripFilter_GetStage(void)

{
  char cVar1;
  int local_c;
  
                    /* 0xa492d  687  GripFilter_GetStage */
  local_c = *(int *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)DAT_6bc84e21 * 0x478 + 0x210);
  switch(local_c) {
  case 0:
    cVar1 = GripFilter_IsSafeTouch();
    if (cVar1 == '\0') {
      local_c = GripFilter_StageOnQuality();
      cVar1 = GripFilter_IsDelayTimeOut();
      if (((cVar1 == '\0') && (cVar1 = Touch_IsOff(g_grip), cVar1 == '\0')) &&
         ((local_c == 2 || (local_c == 3)))) {
        local_c = 0;
      }
    }
    else {
      local_c = 4;
    }
    break;
  case 1:
    cVar1 = GripFilter_IsSafeTouch();
    if (cVar1 != '\0') {
      local_c = 4;
    }
    break;
  case 2:
    cVar1 = GripFilter_IsSafeTouch();
    if (cVar1 == '\0') {
      local_c = GripFilter_StageOnQuality();
    }
    else {
      local_c = 4;
    }
    break;
  case 3:
    cVar1 = GripFilter_IsBackToGrip();
    if (cVar1 == '\0') {
      cVar1 = GripFilter_IsSafeTouch();
      if (cVar1 == '\0') {
        local_c = GripFilter_StageOnQuality();
        if (local_c == 2) {
          local_c = 3;
        }
      }
      else {
        local_c = 4;
      }
    }
    else {
      GripFilter_BackToGrip(&local_c);
    }
    break;
  case 4:
    cVar1 = GripFilter_IsBackToGrip();
    if (cVar1 != '\0') {
      GripFilter_BackToGrip(&local_c);
    }
  }
  *(int *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x210) =
       local_c;
  return;
}


// ============================================================================
// Function: GripFilter_UpdateDeadzone @ 6bae5981
// ============================================================================

void GripFilter_UpdateDeadzone(void)

{
                    /* 0xa5981  797  GripFilter_UpdateDeadzone */
  if (_refptr_g_tsaPrmtFlash[1].field_0x80 != '\0') {
    GripFilter_SetDeadZoneFlag();
    GripFilter_ClearDeadZoneFlag();
  }
  return;
}


// ============================================================================
// Function: GripFilter_GetProperty @ 6bae9747
// ============================================================================

void GripFilter_GetProperty(void)

{
                    /* 0xa9747  677  GripFilter_GetProperty */
  GripFilter_GrapDetect();
  GripFilter_AboveGripDetect();
  GripFilter_CloseGripDetect();
  GripFilter_MultiGripDetect();
  GripFilter_NewTouchDetect();
  GripFilter_FastMoveDetect();
  GripFilter_HeavyGripRegionDetect();
  GripFilter_SafeTouchDetect();
  return;
}


// ============================================================================
// Function: GripFilter_GetEdge @ 6bae9854
// ============================================================================

undefined8 GripFilter_GetEdge(void)

{
  undefined8 uVar1;
  
                    /* 0xa9854  673  GripFilter_GetEdge */
  if ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                 *(longlong *)_refptr_g_touchesPtr + 0x1c4) & 4) == 0) {
    if ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                   *(longlong *)_refptr_g_touchesPtr + 0x1c4) & 8) == 0) {
      if ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                     *(longlong *)_refptr_g_touchesPtr + 0x1c4) & 1) == 0) {
        uVar1 = 1;
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
// Function: GripFilter_IsGripMergeAllowed @ 6bae1eda
// ============================================================================

bool GripFilter_IsGripMergeAllowed(void)

{
                    /* 0xa1eda  712  GripFilter_IsGripMergeAllowed */
  return (int)((uint)*(byte *)(*(longlong *)_refptr_g_touchesPtr + 8) -
              (uint)*(byte *)(*(longlong *)_refptr_g_touchesPtr + 10)) < 1;
}


// ============================================================================
// Function: GripFilter_GetShapeFlag @ 6bae9201
// ============================================================================

void GripFilter_GetShapeFlag(void)

{
  char cVar1;
  
                    /* 0xa9201  683  GripFilter_GetShapeFlag */
  if ((*(int *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                *(longlong *)_refptr_g_touchesPtr + 0x1e0) == 3) &&
     (*(int *)((longlong)*_refptr_g_prevTouchesPtr +
              (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x1e0) != 3)) {
    *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr
             + 0x214) =
         *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                   *(longlong *)_refptr_g_touchesPtr + 0x214) & 0xffffdfff;
    *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr
             + 0x214) =
         *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                   *(longlong *)_refptr_g_touchesPtr + 0x214) & 0xffbfffff;
    *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr
             + 0x214) =
         *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                   *(longlong *)_refptr_g_touchesPtr + 0x214) & 0xffffbfff;
    *(uint *)((longlong)*_refptr_g_prevTouchesPtr +
             (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x214) =
         *(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x214) & 0xffffdfff;
    *(uint *)((longlong)*_refptr_g_prevTouchesPtr +
             (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x214) =
         *(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x214) & 0xffbfffff;
    *(uint *)((longlong)*_refptr_g_prevTouchesPtr +
             (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x214) =
         *(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x214) & 0xffffbfff;
  }
  cVar1 = GripFilter_IsInPalmRegion(*_refptr_g_grip);
  if (cVar1 != '\0') {
    if (*(char *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                  *(longlong *)_refptr_g_touchesPtr + 0x25c) == '\0') {
      cVar1 = Touch_IsOff(*_refptr_g_grip);
      if (cVar1 == '\0') goto LAB_6bae9678;
    }
    if ((0x20 < *(byte *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                          *(longlong *)_refptr_g_touchesPtr + 0x267)) &&
       (0xc < *(ushort *)
               ((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                *(longlong *)_refptr_g_touchesPtr + 0x25e))) {
      *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                *(longlong *)_refptr_g_touchesPtr + 0x214) =
           *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                     *(longlong *)_refptr_g_touchesPtr + 0x214) | 0x2000;
    }
    if (0xe < *(short *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                         *(longlong *)_refptr_g_touchesPtr + 0x268)) {
      *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                *(longlong *)_refptr_g_touchesPtr + 0x214) =
           *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                     *(longlong *)_refptr_g_touchesPtr + 0x214) | 0x4000;
    }
  }
LAB_6bae9678:
  if ((0x2f < *(byte *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                        *(longlong *)_refptr_g_touchesPtr + 0x267)) &&
     (0xe < *(ushort *)
             ((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr
             + 0x25e))) {
    *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr
             + 0x214) =
         *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                   *(longlong *)_refptr_g_touchesPtr + 0x214) | 0x400000;
  }
  return;
}


// ============================================================================
// Function: GripFilter_IsInMultiGripRegion @ 6bae977e
// ============================================================================

undefined8 GripFilter_IsInMultiGripRegion(byte param_1)

{
  char cVar1;
  ushort uVar2;
  
                    /* 0xa977e  726  GripFilter_IsInMultiGripRegion */
  if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) &
      0x4000) != 0) {
    return 1;
  }
  uVar2 = GripFilter_GetCtdDistToEdge(param_1);
  if ((uVar2 < 0x101) && (cVar1 = GripFilter_IsRatioOK(3), cVar1 != '\0')) {
    return 1;
  }
  cVar1 = GripFilter_IsScreenRotatedMultiGripEnabled();
  if (cVar1 != '\0') {
    if (*(ushort *)
         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x188) < 0x10)
    {
      uVar2 = GripFilter_GetScreenRotatedCtdDistToEdge(param_1);
      if (uVar2 < 0x101) {
        return 1;
      }
    }
    else {
      uVar2 = GripFilter_GetScreenRotatedCtdDistToEdge(param_1);
      if (uVar2 < 0x181) {
        return 1;
      }
    }
  }
  return 0;
}


// ============================================================================
// Function: GripFilter_IsPeakAbsNotGrip @ 6baea103
// ============================================================================

bool GripFilter_IsPeakAbsNotGrip(byte param_1)

{
                    /* 0xaa103  749  GripFilter_IsPeakAbsNotGrip */
  return (*(uint *)(_refptr_g_peaksGrid + (longlong)(int)(uint)param_1 * 0x14 + 0x10) & 0x800) != 0;
}


// ============================================================================
// Function: GripFilter_GetCenterLevel @ 6baea5fb
// ============================================================================

void GripFilter_GetCenterLevel(byte param_1)

{
                    /* 0xaa5fb  668  GripFilter_GetCenterLevel */
  GripFilter_GetGripLevel
            (*(undefined1 *)
              ((ulonglong)
               ((uint)(byte)((ushort)*(undefined2 *)
                                      ((longlong)(int)(uint)param_1 * 0x168 +
                                       *(longlong *)_refptr_g_touchesPtr + 0x162) >> 8) +
                (uint)(byte)((ushort)*(undefined2 *)
                                      ((longlong)(int)(uint)param_1 * 0x168 +
                                       *(longlong *)_refptr_g_touchesPtr + 0x164) >> 8) *
                (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) +
              *(longlong *)_refptr_g_tsaBufGripRegion));
  return;
}


// ============================================================================
// Function: GripFilter_Is2HandGame @ 6bae1e40
// ============================================================================

undefined8 GripFilter_Is2HandGame(void)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0xa1e40  696  GripFilter_Is2HandGame */
  cVar1 = GripFilter_IsPad();
  if (cVar1 == '\0') {
    cVar1 = TSA_IsTypingScenario();
    if ((cVar1 != '\0') && (cVar1 = TSAStatic_IsScreenRotated(), cVar1 == '\0')) {
      return 0;
    }
    cVar1 = TSA_IsGameScenario();
    if (cVar1 == '\0') {
      cVar1 = HandGesture_Is2HandDetected();
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
  else {
    uVar2 = 0;
  }
  return uVar2;
}


// ============================================================================
// Function: GripFilter_RegionSet @ 6baebad8
// ============================================================================

void GripFilter_RegionSet(uint param_1,undefined4 param_2,byte param_3)

{
  byte bVar1;
  byte bVar2;
  byte local_a;
  byte local_9;
  
                    /* 0xabad8  785  GripFilter_RegionSet */
  bVar1 = (byte)(1 << ((byte)param_2 & 0x1f));
  if ((param_1 & 1) != 0) {
    for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bCols; local_9 = local_9 + 1) {
      *(byte *)(*(longlong *)_refptr_g_tsaBufGripRegion +
               (ulonglong)
               ((uint)local_9 + (uint)param_3 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)) =
           *(byte *)((ulonglong)
                     ((uint)local_9 + (uint)param_3 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)
                    + *(longlong *)_refptr_g_tsaBufGripRegion) | bVar1;
    }
  }
  if ((param_1 & 2) != 0) {
    bVar2 = (_refptr_g_tsaPrmtFlash->bRows - param_3) - 1;
    for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bCols; local_9 = local_9 + 1) {
      *(byte *)(*(longlong *)_refptr_g_tsaBufGripRegion +
               (ulonglong)
               ((uint)local_9 + (uint)bVar2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)) =
           *(byte *)((ulonglong)
                     ((uint)local_9 + (uint)bVar2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) +
                    *(longlong *)_refptr_g_tsaBufGripRegion) | bVar1;
    }
  }
  if ((param_1 & 4) != 0) {
    for (local_a = 0; local_a < _refptr_g_tsaPrmtFlash->bRows; local_a = local_a + 1) {
      *(byte *)(*(longlong *)_refptr_g_tsaBufGripRegion +
               (ulonglong)
               ((uint)param_3 + (uint)local_a * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)) =
           *(byte *)((ulonglong)
                     ((uint)param_3 + (uint)local_a * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)
                    + *(longlong *)_refptr_g_tsaBufGripRegion) | bVar1;
    }
  }
  if ((param_1 & 8) != 0) {
    bVar2 = (_refptr_g_tsaPrmtFlash->bCols - param_3) - 1;
    for (local_a = 0; local_a < _refptr_g_tsaPrmtFlash->bRows; local_a = local_a + 1) {
      *(byte *)(*(longlong *)_refptr_g_tsaBufGripRegion +
               (ulonglong)
               ((uint)bVar2 + (uint)local_a * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)) =
           *(byte *)((ulonglong)
                     ((uint)bVar2 + (uint)local_a * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) +
                    *(longlong *)_refptr_g_tsaBufGripRegion) | bVar1;
    }
  }
  if ((param_1 & 0x10) != 0) {
    GripFilter_RegionSetTopLeftCorner(param_2);
  }
  if ((param_1 & 0x20) != 0) {
    GripFilter_RegionSetTopRightCorner(param_2);
  }
  if ((param_1 & 0x40) != 0) {
    GripFilter_RegionSetBottomLeftCorner(param_2);
  }
  if ((param_1 & 0x80) != 0) {
    GripFilter_RegionSetBottomRightCorner(param_2);
  }
  GripFilter_RegionSetPalm(param_1,param_2,param_3);
  return;
}


// ============================================================================
// Function: GripFilter_IsVideoMode @ 6bae1e9c
// ============================================================================

undefined8 GripFilter_IsVideoMode(void)

{
  char cVar1;
  
                    /* 0xa1e9c  776  GripFilter_IsVideoMode */
  cVar1 = TSA_IsVideoScenario();
  if ((((cVar1 != '\0') && (cVar1 = TSAStatic_IsScreenRotated(), cVar1 != '\0')) &&
      (cVar1 = GripFilter_Is2HandGame(), cVar1 == '\0')) &&
     (cVar1 = TSA_IsTypingScenario(), cVar1 == '\0')) {
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: GripFilter_IsPeakAbsGrip @ 6baea0be
// ============================================================================

bool GripFilter_IsPeakAbsGrip(byte param_1)

{
                    /* 0xaa0be  748  GripFilter_IsPeakAbsGrip */
  return (*(uint *)(_refptr_g_peaksGrid + (longlong)(int)(uint)param_1 * 0x14 + 0x10) & 0x400) != 0;
}


// ============================================================================
// Function: GripFilter_IsMerged @ 6bae83a6
// ============================================================================

undefined8 GripFilter_IsMerged(byte param_1,byte param_2)

{
  ushort uVar1;
  ushort uVar2;
  char cVar3;
  ushort uVar4;
  uint uVar5;
  undefined8 uVar6;
  
                    /* 0xa83a6  737  GripFilter_IsMerged */
  if ((int)(short)(*_refptr_g_tsaStaticPtr)->wField_0x292 / 2 <
      (int)*(short *)((longlong)
                      (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                          (longlong)(int)(uint)param_2 * 0x478 + 0x23c) * 0xc +
                      *(longlong *)_refptr_g_prevPeaksPtr + 0xc)) {
    uVar6 = 0;
  }
  else if ((*(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x218) & 1) == 0) {
    if (*(ushort *)
         ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x22c) < 0x81
       ) {
      uVar1 = *(ushort *)
               ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22c);
      uVar2 = *(ushort *)
               ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x22c);
      uVar4 = TS_MMToPixel(4);
      if ((uint)uVar1 < (uint)uVar4 + (uint)uVar2) {
        uVar6 = 0;
      }
      else if (((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                          (longlong)(int)(uint)param_2 * 0x478 + 0x1c4) & 4) == 0) &&
              ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                         (longlong)(int)(uint)param_2 * 0x478 + 0x1c4) & 8) == 0)) {
        uVar6 = 0;
      }
      else {
        uVar1 = *(ushort *)
                 ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x1a8
                 );
        uVar5 = TSA_SideToEdge(*(undefined4 *)
                                ((longlong)*_refptr_g_prevTouchesPtr +
                                (longlong)(int)(uint)param_2 * 0x478 + 0x1c4));
        if (uVar1 < *(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                             (ulonglong)uVar5 * 0xc + -0x12)) {
          uVar6 = 0;
        }
        else {
          cVar3 = Peak_IsOnOuterEdgeDim1
                            (*(undefined1 *)
                              ((longlong)(int)(uint)param_1 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x23c));
          if ((cVar3 == '\0') ||
             ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x1f8) & 8) == 0)) {
            uVar6 = 1;
          }
          else {
            uVar6 = 0;
          }
        }
      }
    }
    else {
      uVar6 = 0;
    }
  }
  else {
    uVar6 = 0;
  }
  return uVar6;
}


// ============================================================================
// Function: GripFilter_IsGripOrTbdTouch @ 6bae1995
// ============================================================================

void GripFilter_IsGripOrTbdTouch(byte param_1)

{
                    /* 0xa1995  715  GripFilter_IsGripOrTbdTouch */
  GripFilter_IsGripOrTbd
            (*(undefined4 *)
              ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x210));
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
// Function: GripFilter_GetTrackingThold @ 6bae5d38
// ============================================================================

undefined4 GripFilter_GetTrackingThold(void)

{
  int iVar1;
  
                    /* 0xa5d38  690  GripFilter_GetTrackingThold */
  iVar1 = (uint)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax +
          (uint)(_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax >> 0xf);
  return CONCAT22((short)((uint)iVar1 >> 0x10),(short)iVar1 >> 1);
}


// ============================================================================
// Function: GripFilter_BasicIsTempGrip @ 6bae39aa
// ============================================================================

undefined8 GripFilter_BasicIsTempGrip(void)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0xa39aa  651  GripFilter_BasicIsTempGrip */
  cVar1 = Touch_IsGhostByMinSigOff(g_grip);
  if (cVar1 == '\0') {
    cVar1 = GripFilter_IsGlobalSuppressedClick();
    if (cVar1 == '\0') {
      cVar1 = GripFilter_IsRatioOK(0xd);
      if (cVar1 == '\0') {
        cVar1 = GripFilter_IsLongPressAllowed();
        if ((cVar1 == '\0') && (cVar1 = GripFilter_IsLongPress(), cVar1 != '\0')) {
          return 1;
        }
        if (((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x214) & 0x100) != 0) && (cVar1 = GripFilter_IsRatioOK(0x11), cVar1 != '\0'))
        {
          return 1;
        }
        if (((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x1fc) & 0x8000000) == 0) ||
           (*(short *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x1bc) * 2 <=
            *(short *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x1ba) * 3)) {
          if (((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x1fc) & 0x10000000) == 0) ||
             (*(ushort *)
               ((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x24e) <
              10)) {
            cVar1 = GripFilter_IsLongPress();
            if (cVar1 == '\0') {
              cVar1 = GripFilter_IsRatioOK(0xc);
              if (cVar1 != '\0') {
                return 1;
              }
            }
            else {
              cVar1 = GripFilter_IsRatioOK(0xe);
              if (cVar1 != '\0') {
                return 1;
              }
            }
            if (((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr
                           + 0x214) & 0x400000) != 0) &&
               (cVar1 = GripFilter_IsRatioOK(5), cVar1 != '\0')) {
              return 1;
            }
            cVar1 = GripFilter_BasicIsTempGripAbnormal();
            if (cVar1 == '\0') {
              cVar1 = GripFilter_IsNegSigTouch();
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
          else {
            uVar2 = 1;
          }
        }
        else {
          uVar2 = 1;
        }
      }
      else {
        uVar2 = 1;
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
// Function: GripFilter_BasicIsAbsGrip @ 6bae3fc1
// ============================================================================

bool GripFilter_BasicIsAbsGrip(void)

{
  char cVar1;
  
                    /* 0xa3fc1  645  GripFilter_BasicIsAbsGrip */
  cVar1 = GripFilter_BasicIsAbsGripPrmt();
  if (cVar1 != '\0') {
    return true;
  }
  cVar1 = GripFilter_IsSuppressedGlobally();
  if (cVar1 != '\0') {
    return true;
  }
  cVar1 = GripFilter_IsSuppressedGlobally2();
  if (cVar1 != '\0') {
    return true;
  }
  if ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214) &
      2) == 0) {
    if (((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x218)
         & 1) != 0) &&
       ((*(uint *)(*_refptr_g_peaksPtr +
                  (longlong)
                  (int)(uint)*(byte *)((longlong)(int)(uint)g_grip * 0x168 +
                                       *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x10) &
        0x400) != 0)) {
      return true;
    }
    if ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x218)
        & 0x10) != 0) {
      return true;
    }
    cVar1 = GripFilter_IsCornerGripClick();
    if (cVar1 != '\0') {
      return true;
    }
    cVar1 = GripFilter_BasicIsAbsGripAbnormal();
    if (cVar1 != '\0') {
      return true;
    }
    cVar1 = GripFilter_IsLongPress();
    if (((cVar1 != '\0') &&
        ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
         & 0x20000000) == 0)) && (cVar1 = GripFilter_Is2HandGame(), cVar1 == '\0')) {
      cVar1 = GripFilter_IsOnDim2();
      if (((cVar1 != '\0') || (cVar1 = GripFilter_IsNearbyCorner(g_grip), cVar1 != '\0')) &&
         ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214
                    ) & 0x40000000) != 0)) {
        return true;
      }
      cVar1 = GripFilter_IsNearbyCorner(g_grip);
      if (((cVar1 != '\0') && (cVar1 = TSAStatic_IsScreenRotated(), cVar1 != '\0')) &&
         ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214
                    ) & 1) != 0)) {
        return true;
      }
    }
    cVar1 = GripFilter_IsNegSigTouchBypass();
    return cVar1 != '\0';
  }
  return true;
}


// ============================================================================
// Function: GripFilter_BasicIsAbsNotGrip @ 6bae4262
// ============================================================================

bool GripFilter_BasicIsAbsNotGrip(void)

{
                    /* 0xa4262  648  GripFilter_BasicIsAbsNotGrip */
  return (*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214)
         & 0x800) != 0;
}


// ============================================================================
// Function: GripFilter_IsBackToGrip @ 6bae237b
// ============================================================================

undefined8 GripFilter_IsBackToGrip(void)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0xa237b  700  GripFilter_IsBackToGrip */
  if ((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x20) != 0) &&
     (cVar1 = GripFilter_IsStuckSideTouch(), cVar1 != '\0')) {
    return 1;
  }
  cVar1 = GripFilter_IsBackToGripAllowed();
  if ((cVar1 != '\0') && (cVar1 = GripFilter_IsBackToGripSatisfied(), cVar1 != '\0')) {
    return 1;
  }
  if ((*(ushort *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x242)
       < 0x1a) ||
     ((0x3f < *(ushort *)
               ((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22c) &&
      (0x3f < *(ushort *)
               ((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22e)))))
  {
    cVar1 = GripFilter_IsSeparated(g_grip,DAT_6bc84e21);
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
  return uVar2;
}


// ============================================================================
// Function: GripFilter_BackToGrip @ 6bae24a1
// ============================================================================

void GripFilter_BackToGrip(void)

{
                    /* 0xa24a1  644  GripFilter_BackToGrip */
  *(undefined4 *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c) =
       0x20;
  *(undefined2 *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162) =
       *(undefined2 *)
        ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)DAT_6bc84e21 * 0x478 + 0x162);
  *(undefined2 *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164) =
       *(undefined2 *)
        ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)DAT_6bc84e21 * 0x478 + 0x164);
  PrevTouch_HistoryBufferClean(DAT_6bc84e21);
  return;
}


// ============================================================================
// Function: GripFilter_IsSafeTouch @ 6bae42a9
// ============================================================================

void GripFilter_IsSafeTouch(void)

{
                    /* 0xa42a9  752  GripFilter_IsSafeTouch */
  GripFilter_BasicIsAbsNotGrip();
  return;
}


// ============================================================================
// Function: GripFilter_IsDelayTimeOut @ 6bae47ec
// ============================================================================

undefined8 GripFilter_IsDelayTimeOut(void)

{
  uint uVar1;
  ushort uVar2;
  undefined8 uVar3;
  
                    /* 0xa47ec  707  GripFilter_IsDelayTimeOut */
  if (*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c) < 2
     ) {
    uVar3 = 0;
  }
  else {
    uVar1 = *(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x158);
    uVar2 = GripFilter_GetDelayTime();
    if (uVar1 < uVar2) {
      uVar3 = 0;
    }
    else {
      *(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214) =
           *(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214
                    ) | 0x40000000;
      uVar3 = 1;
    }
  }
  return uVar3;
}


// ============================================================================
// Function: GripFilter_StageOnQuality @ 6bae48e1
// ============================================================================

undefined8 GripFilter_StageOnQuality(void)

{
  undefined8 uVar1;
  
                    /* 0xa48e1  796  GripFilter_StageOnQuality */
  if (DAT_6bc84e28 == 0) {
    uVar1 = 4;
  }
  else if (DAT_6bc84e28 == 3) {
    uVar1 = 1;
  }
  else if (DAT_6bc84e28 == 2) {
    uVar1 = 2;
  }
  else {
    uVar1 = 3;
  }
  return uVar1;
}


// ============================================================================
// Function: GripFilter_SetDeadZoneFlag @ 6bae5021
// ============================================================================

void GripFilter_SetDeadZoneFlag(void)

{
  uchar uVar1;
  uchar uVar2;
  char cVar3;
  byte bVar4;
  byte local_9;
  
  bVar4 = *_refptr_g_grip;
  uVar1 = _refptr_g_tsaPrmtFlash->bCols;
  uVar2 = _refptr_g_tsaPrmtFlash->bRows;
  cVar3 = GripFilter_IsGripOrTbdTouch(bVar4);
  if ((cVar3 != '\0') || (cVar3 = GripFilter_IsGripCriteriaDeadzone(bVar4), cVar3 != '\0')) {
    if (((_refptr_g_tsaPrmtFlash[1].field_0x80 & 1) == 0) ||
       (0xff < *(ushort *)
                ((longlong)(int)(uint)bVar4 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x160))) {
      if (((_refptr_g_tsaPrmtFlash[1].field_0x80 & 2) == 0) ||
         ((int)(uint)*(ushort *)
                      ((longlong)(int)(uint)bVar4 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x160) <= (int)((_refptr_g_tsaPrmtFlash->bRows - 1) * 0x100))) {
        if (((_refptr_g_tsaPrmtFlash[1].field_0x80 & 4) == 0) ||
           (0xff < *(ushort *)
                    ((longlong)(int)(uint)bVar4 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x15e)
           )) {
          if (((_refptr_g_tsaPrmtFlash[1].field_0x80 & 8) != 0) &&
             ((int)((_refptr_g_tsaPrmtFlash->bCols - 1) * 0x100) <
              (int)(uint)*(ushort *)
                          ((longlong)(int)(uint)bVar4 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                          0x15e))) {
            local_9 = GripFilter_GetEdgeWidthStart
                                (*(undefined2 *)
                                  ((longlong)(int)(uint)bVar4 * 0x168 +
                                   *(longlong *)_refptr_g_touchesPtr + 0x160),
                                 *(undefined1 *)
                                  ((longlong)(int)(uint)bVar4 * 0x168 +
                                   *(longlong *)_refptr_g_touchesPtr + 0x1b1));
            bVar4 = GripFilter_GetEdgeWidthEnd
                              (*(undefined2 *)
                                ((longlong)(int)(uint)bVar4 * 0x168 +
                                 *(longlong *)_refptr_g_touchesPtr + 0x160),
                               *(undefined1 *)
                                ((longlong)(int)(uint)bVar4 * 0x168 +
                                 *(longlong *)_refptr_g_touchesPtr + 0x1b1),uVar2);
            for (; local_9 <= bVar4; local_9 = local_9 + 1) {
              (&DAT_6bba34b0)[(int)(uint)local_9] = 1;
            }
          }
        }
        else {
          local_9 = GripFilter_GetEdgeWidthStart
                              (*(undefined2 *)
                                ((longlong)(int)(uint)bVar4 * 0x168 +
                                 *(longlong *)_refptr_g_touchesPtr + 0x160),
                               *(undefined1 *)
                                ((longlong)(int)(uint)bVar4 * 0x168 +
                                 *(longlong *)_refptr_g_touchesPtr + 0x1b1));
          bVar4 = GripFilter_GetEdgeWidthEnd
                            (*(undefined2 *)
                              ((longlong)(int)(uint)bVar4 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x160),
                             *(undefined1 *)
                              ((longlong)(int)(uint)bVar4 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x1b1),uVar2);
          for (; local_9 <= bVar4; local_9 = local_9 + 1) {
            (&DAT_6bba3460)[(int)(uint)local_9] = 1;
          }
        }
      }
      else {
        local_9 = GripFilter_GetEdgeWidthStart
                            (*(undefined2 *)
                              ((longlong)(int)(uint)bVar4 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x15e),
                             *(undefined1 *)
                              ((longlong)(int)(uint)bVar4 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x1b0));
        bVar4 = GripFilter_GetEdgeWidthEnd
                          (*(undefined2 *)
                            ((longlong)(int)(uint)bVar4 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                            + 0x15e),
                           *(undefined1 *)
                            ((longlong)(int)(uint)bVar4 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                            + 0x1b0),uVar1);
        for (; local_9 <= bVar4; local_9 = local_9 + 1) {
          (&DAT_6bba3410)[(int)(uint)local_9] = 1;
        }
      }
    }
    else {
      local_9 = GripFilter_GetEdgeWidthStart
                          (*(undefined2 *)
                            ((longlong)(int)(uint)bVar4 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                            + 0x15e),
                           *(undefined1 *)
                            ((longlong)(int)(uint)bVar4 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                            + 0x1b0));
      bVar4 = GripFilter_GetEdgeWidthEnd
                        (*(undefined2 *)
                          ((longlong)(int)(uint)bVar4 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                          0x15e),*(undefined1 *)
                                  ((longlong)(int)(uint)bVar4 * 0x168 +
                                   *(longlong *)_refptr_g_touchesPtr + 0x1b0),uVar1);
      for (; local_9 <= bVar4; local_9 = local_9 + 1) {
        (&g_gripFilterDeadZone)[(int)(uint)local_9] = 1;
      }
    }
  }
  return;
}


// ============================================================================
// Function: GripFilter_ClearDeadZoneFlag @ 6bae5565
// ============================================================================

void GripFilter_ClearDeadZoneFlag(void)

{
  char local_158 [320];
  int local_18;
  byte local_12;
  byte local_11;
  int local_10;
  byte local_a;
  byte local_9;
  
  local_10 = (int)((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 6);
  local_11 = _refptr_g_tsaPrmtFlash->bCols;
  local_12 = _refptr_g_tsaPrmtFlash->bRows;
  local_18 = memset_s(local_158,0x140,0,0x140);
  if (local_18 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/GripFilter/GripFilter_DeadZone.c",0x7d,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  if ((_refptr_g_tsaPrmtFlash[1].field_0x80 & 1) != 0) {
    for (local_9 = 0; local_9 < local_11; local_9 = local_9 + 1) {
      if (local_10 < *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)local_9 * 2)) {
        local_158[(int)(uint)local_9] = '\x01';
      }
    }
  }
  if ((_refptr_g_tsaPrmtFlash[1].field_0x80 & 2) != 0) {
    for (local_9 = 0; local_9 < local_11; local_9 = local_9 + 1) {
      if (local_10 <
          *(short *)((ulonglong)
                     ((uint)local_9 + (local_12 - 1) * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)
                     * 2 + *(longlong *)_refptr_g_tsaBufDif)) {
        local_158[(longlong)(int)(uint)local_9 + 0x50] = '\x01';
      }
    }
  }
  if ((_refptr_g_tsaPrmtFlash[1].field_0x80 & 4) != 0) {
    for (local_9 = 0; local_9 < local_12; local_9 = local_9 + 1) {
      if (local_10 <
          *(short *)((ulonglong)((uint)_refptr_g_tsaPrmtFlash->bCols * (uint)local_9) * 2 +
                    *(longlong *)_refptr_g_tsaBufDif)) {
        local_158[(longlong)(int)(uint)local_9 + 0xa0] = '\x01';
      }
    }
  }
  if ((_refptr_g_tsaPrmtFlash[1].field_0x80 & 8) != 0) {
    for (local_9 = 0; local_9 < local_12; local_9 = local_9 + 1) {
      if (local_10 <
          *(short *)((ulonglong)
                     (((uint)local_11 + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols) - 1 &
                     0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif)) {
        local_158[(longlong)(int)(uint)local_9 + 0xf0] = '\x01';
      }
    }
  }
  if (**(char **)_refptr_g_touchesPtr == '\0') {
    local_18 = memset_s(local_158,0x140,0,0x140);
    if (local_18 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/GripFilter/GripFilter_DeadZone.c",0x9d,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
  }
  for (local_a = 0; local_a < 4; local_a = local_a + 1) {
    for (local_9 = 0; local_9 < 0x50; local_9 = local_9 + 1) {
      if (local_158[(longlong)(int)(uint)local_9 + (longlong)(int)(uint)local_a * 0x50] == '\0') {
        (&g_gripFilterDeadZone)[(longlong)(int)(uint)local_a * 0x50 + (longlong)(int)(uint)local_9]
             = 0;
      }
    }
  }
  return;
}


// ============================================================================
// Function: GripFilter_FastMoveDetect @ 6bae65ff
// ============================================================================

void GripFilter_FastMoveDetect(void)

{
                    /* 0xa65ff  665  GripFilter_FastMoveDetect */
  if (((0xffff < *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                           *(longlong *)_refptr_g_touchesPtr + 0x248)) &&
      ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                  *(longlong *)_refptr_g_touchesPtr + 0x1f0) & 0x40000) == 0)) &&
     (*(int *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
               *(longlong *)_refptr_g_touchesPtr + 0x1c0) / 2 <
      *(int *)(*_refptr_g_peaksPtr +
              (longlong)
              (int)(uint)*(byte *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                                   *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x20))) {
    *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr
             + 0x214) =
         *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                   *(longlong *)_refptr_g_touchesPtr + 0x214) | 0x1000;
  }
  return;
}


// ============================================================================
// Function: GripFilter_AboveGripDetect @ 6bae698c
// ============================================================================

void GripFilter_AboveGripDetect(void)

{
  int iVar1;
  byte local_9;
  
                    /* 0xa698c  643  GripFilter_AboveGripDetect */
  for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
    if (((*_refptr_g_grip != local_9) &&
        ((*(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214
                   ) & 4) != 0)) &&
       ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                   *(longlong *)_refptr_g_touchesPtr + 0x214) & 4) != 0)) {
      iVar1 = GripFilter_GetCoreDim2Distance(*_refptr_g_grip,local_9);
      if ((iVar1 < 6) &&
         (*(ushort *)
           ((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164) <
          *(ushort *)
           ((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
           0x164))) {
        *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                  *(longlong *)_refptr_g_touchesPtr + 0x214) =
             *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                       *(longlong *)_refptr_g_touchesPtr + 0x214) | 0x20000000;
      }
    }
  }
  return;
}


// ============================================================================
// Function: GripFilter_CloseGripDetect @ 6bae6e5b
// ============================================================================

void GripFilter_CloseGripDetect(void)

{
  char cVar1;
  
                    /* 0xa6e5b  661  GripFilter_CloseGripDetect */
  cVar1 = GripFilter_IsInPalmRegion(*_refptr_g_grip);
  if (cVar1 == '\0') {
    *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr
             + 0x214) =
         *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                   *(longlong *)_refptr_g_touchesPtr + 0x214) | 0x20;
  }
  if ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                 *(longlong *)_refptr_g_touchesPtr + 0x214) & 0x20) == 0) {
    if ((*_refptr_g_touchesSide != '\0') && (cVar1 = Peak_IsToExcludeSideRegion(), cVar1 != '\0')) {
      GripFilter_CloseGripOnSide();
      return;
    }
    GripFilter_CloseGripOnCore();
  }
  else {
    *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr
             + 0x214) =
         *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                   *(longlong *)_refptr_g_touchesPtr + 0x214) & 0xfffffff7;
    *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr
             + 0x214) =
         *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                   *(longlong *)_refptr_g_touchesPtr + 0x214) & 0xffffffef;
  }
  return;
}


// ============================================================================
// Function: GripFilter_GrapDetect @ 6bae71df
// ============================================================================

void GripFilter_GrapDetect(void)

{
  char cVar1;
  byte local_10;
  byte local_f;
  byte local_e;
  byte local_d;
  uint local_c;
  
                    /* 0xa71df  691  GripFilter_GrapDetect */
  local_c = 0xffffffff;
  for (local_d = 0; local_d < **(byte **)_refptr_g_touchesPtr; local_d = local_d + 1) {
    if (*(uint *)((longlong)(int)(uint)local_d * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x158)
        < local_c) {
      local_c = *(uint *)((longlong)(int)(uint)local_d * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x158);
    }
  }
  local_e = 0;
  for (local_f = 0; local_f < **(byte **)_refptr_g_touchesPtr; local_f = local_f + 1) {
    cVar1 = Touch_IsOff(local_f);
    if ((((cVar1 == '\0') &&
         (*(int *)((longlong)(int)(uint)local_f * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x158)
          - local_c < 0xfa)) &&
        (*(uint *)((longlong)(int)(uint)local_f * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x158)
         < 500)) &&
       (((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold * 3 <
         *(short *)((longlong)(int)(uint)local_f * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1ba
                   ) * 10 && (cVar1 = GripFilter_GetCenterLevel(local_f), cVar1 != '\0')))) {
      local_e = local_e + 1;
    }
  }
  if ((3 < local_e) && (**(byte **)_refptr_g_touchesPtr == local_e)) {
    for (local_10 = 0; local_10 < **(byte **)_refptr_g_touchesPtr; local_10 = local_10 + 1) {
      *(uint *)((longlong)(int)(uint)local_10 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214) =
           *(uint *)((longlong)(int)(uint)local_10 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x214) | 0x4000000;
    }
  }
  return;
}


// ============================================================================
// Function: GripFilter_MultiGripDetect @ 6bae7d43
// ============================================================================

void GripFilter_MultiGripDetect(void)

{
                    /* 0xa7d43  779  GripFilter_MultiGripDetect */
  if (((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                  *(longlong *)_refptr_g_touchesPtr + 0x214) & 2) == 0) &&
     ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                 *(longlong *)_refptr_g_touchesPtr + 0x214) & 4) != 0)) {
    GripFilter_CheckMultiGrip();
  }
  return;
}


// ============================================================================
// Function: GripFilter_SafeTouchDetect @ 6bae891d
// ============================================================================

void GripFilter_SafeTouchDetect(void)

{
  bool bVar1;
  char cVar2;
  
                    /* 0xa891d  795  GripFilter_SafeTouchDetect */
  bVar1 = false;
  if ((*(char *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                 *(longlong *)_refptr_g_touchesPtr + 0x199) == '\0') &&
     (1 < *(ushort *)
           ((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
           0x156))) {
    bVar1 = true;
  }
  else {
    cVar2 = GripFilter_BasicIsFlick();
    if (cVar2 == '\0') {
      if ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                     *(longlong *)_refptr_g_touchesPtr + 0x214) & 0x200000) == 0) {
        cVar2 = GripFilter_IsInExcludedRegion(*_refptr_g_grip);
        if (cVar2 == '\0') {
          if ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                         *(longlong *)_refptr_g_touchesPtr + 0x1f8) & 0x20000000) != 0) {
            cVar2 = GripFilter_IsRatioOK(0xf);
            if (cVar2 != '\0') {
              bVar1 = true;
            }
          }
        }
        else {
          bVar1 = true;
        }
      }
      else {
        bVar1 = true;
      }
    }
    else {
      bVar1 = true;
    }
  }
  if (bVar1) {
    *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr
             + 0x214) =
         *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                   *(longlong *)_refptr_g_touchesPtr + 0x214) | 0x800;
  }
  return;
}


// ============================================================================
// Function: GripFilter_NewTouchDetect @ 6bae8ab3
// ============================================================================

void GripFilter_NewTouchDetect(void)

{
  char cVar1;
  ushort uVar2;
  byte local_d;
  undefined4 local_c;
  
                    /* 0xa8ab3  780  GripFilter_NewTouchDetect */
  local_c = 200;
  if (2 < **(byte **)_refptr_g_touchesPtr) {
    local_c = 300;
  }
  for (local_d = 0; local_d < **(byte **)_refptr_g_touchesPtr; local_d = local_d + 1) {
    if (*_refptr_g_grip != local_d) {
      cVar1 = Touch_IsGhostByMinSigOff(local_d);
      if (cVar1 == '\0') {
        uVar2 = Touch_GetSQDistMMInBetween(*_refptr_g_grip,local_d);
        if (uVar2 < 0x1c39) {
          cVar1 = GripFilter_IsTouchIntervalInTime(*_refptr_g_grip,local_d,0,local_c);
          if (cVar1 != '\0') {
            *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                      *(longlong *)_refptr_g_touchesPtr + 0x214) =
                 *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                           *(longlong *)_refptr_g_touchesPtr + 0x214) | 1;
          }
          cVar1 = GripFilter_IsHigherPriorityTouch(*_refptr_g_grip,local_d);
          if (cVar1 != '\0') {
            *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                      *(longlong *)_refptr_g_touchesPtr + 0x214) =
                 *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                           *(longlong *)_refptr_g_touchesPtr + 0x214) | 0x20000;
          }
          if (((*(uint *)((longlong)(int)(uint)local_d * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x1f8) & 0x20000000) != 0) &&
             ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                         *(longlong *)_refptr_g_touchesPtr + 0x214) & 0x20000) != 0)) {
            *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                      *(longlong *)_refptr_g_touchesPtr + 0x214) =
                 *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                           *(longlong *)_refptr_g_touchesPtr + 0x214) | 0x2000000;
          }
        }
      }
    }
    if (*_refptr_g_grip != local_d) {
      cVar1 = Touch_IsOff(local_d);
      if (cVar1 == '\0') {
        *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                  *(longlong *)_refptr_g_touchesPtr + 0x214) =
             *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                       *(longlong *)_refptr_g_touchesPtr + 0x214) | 0x40;
      }
    }
  }
  return;
}


// ============================================================================
// Function: GripFilter_HeavyGripRegionDetect @ 6bae8d98
// ============================================================================

void GripFilter_HeavyGripRegionDetect(void)

{
  char cVar1;
  
                    /* 0xa8d98  693  GripFilter_HeavyGripRegionDetect */
  cVar1 = GripFilter_IsTouchInCornerRegion(*_refptr_g_grip);
  if (cVar1 != '\0') {
    *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr
             + 0x214) =
         *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                   *(longlong *)_refptr_g_touchesPtr + 0x214) | 0x800000;
    if (*(short *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                   *(longlong *)_refptr_g_touchesPtr + 0x156) == 1) {
      *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                *(longlong *)_refptr_g_touchesPtr + 0x214) =
           *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                     *(longlong *)_refptr_g_touchesPtr + 0x214) | 0x1000000;
    }
    if (((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                    *(longlong *)_refptr_g_touchesPtr + 0x21c) & 1) != 0) &&
       (4 < *(ushort *)
             ((longlong)*_refptr_g_prevTouchesPtr +
             (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x4e4))) {
      *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                *(longlong *)_refptr_g_touchesPtr + 0x214) =
           *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                     *(longlong *)_refptr_g_touchesPtr + 0x214) | 0x10000;
    }
  }
  cVar1 = GripFilter_IsTouchInHeavyRegion(*_refptr_g_grip);
  if (cVar1 != '\0') {
    *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr
             + 0x1fc) =
         *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                   *(longlong *)_refptr_g_touchesPtr + 0x1fc) | 0x4000000;
    cVar1 = GripFilter_IsTouchInCornerRegion(*_refptr_g_grip);
    if (cVar1 != '\0') {
      *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                *(longlong *)_refptr_g_touchesPtr + 0x1fc) =
           *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                     *(longlong *)_refptr_g_touchesPtr + 0x1fc) | 0x8000000;
      if (*(short *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                     *(longlong *)_refptr_g_touchesPtr + 0x156) == 1) {
        *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                  *(longlong *)_refptr_g_touchesPtr + 0x1fc) =
             *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                       *(longlong *)_refptr_g_touchesPtr + 0x1fc) | 0x10000000;
      }
      cVar1 = Touch_IsOff(*_refptr_g_grip);
      if (cVar1 != '\0') {
        *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                  *(longlong *)_refptr_g_touchesPtr + 0x1fc) =
             *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                       *(longlong *)_refptr_g_touchesPtr + 0x1fc) | 0x20000000;
      }
    }
  }
  cVar1 = GripFilter_IsHeavyCornerTouched(*_refptr_g_grip);
  if (cVar1 != '\0') {
    *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr
             + 0x218) =
         *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                   *(longlong *)_refptr_g_touchesPtr + 0x218) | 2;
  }
  return;
}


// ============================================================================
// Function: GripFilter_IsInPalmRegion @ 6bae696e
// ============================================================================

void GripFilter_IsInPalmRegion(undefined1 param_1)

{
                    /* 0xa696e  728  GripFilter_IsInPalmRegion */
  GripFilter_IsTouchInHeavyRegion(param_1);
  return;
}


// ============================================================================
// Function: GripFilter_GetCtdDistToEdge @ 6bae61b9
// ============================================================================

undefined2 GripFilter_GetCtdDistToEdge(byte param_1)

{
  undefined2 uVar1;
  
  if (*(int *)&_refptr_g_tsaPrmtFlash[1].field_0x84 == 5) {
    uVar1 = *(undefined2 *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22e);
  }
  else {
    uVar1 = *(undefined2 *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22c);
  }
  return uVar1;
}


// ============================================================================
// Function: GripFilter_GetScreenRotatedCtdDistToEdge @ 6bae6220
// ============================================================================

undefined2 GripFilter_GetScreenRotatedCtdDistToEdge(byte param_1)

{
  undefined2 uVar1;
  
  if (*(int *)&_refptr_g_tsaPrmtFlash[1].field_0x84 == 5) {
    uVar1 = *(undefined2 *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22c);
  }
  else {
    uVar1 = *(undefined2 *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22e);
  }
  return uVar1;
}


// ============================================================================
// Function: GripFilter_IsScreenRotatedMultiGripEnabled @ 6bae7ae6
// ============================================================================

undefined8 GripFilter_IsScreenRotatedMultiGripEnabled(void)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0xa7ae6  753  GripFilter_IsScreenRotatedMultiGripEnabled */
  cVar1 = TSAStatic_IsScreenRotated();
  if (cVar1 == '\0') {
    uVar2 = 0;
  }
  else {
    cVar1 = GripFilter_Is2HandGame();
    if (cVar1 == '\0') {
      uVar2 = 1;
    }
    else {
      uVar2 = 0;
    }
  }
  return uVar2;
}


// ============================================================================
// Function: GripFilter_IsRatioOK @ 6baecdc6
// ============================================================================

undefined8 GripFilter_IsRatioOK(undefined4 param_1)

{
  char cVar1;
  ushort uVar2;
  ushort local_a;
  
                    /* 0xacdc6  751  GripFilter_IsRatioOK */
  switch(param_1) {
  case 1:
    uVar2 = GripFilter_GetEdgeClickRatioThold(2);
    cVar1 = GripFilter_IsOnDim1();
    if (cVar1 == '\0') {
      if (*(ushort *)
           ((longlong)*_refptr_g_prevTouchesPtr +
           (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x51a) < uVar2) {
        return 0;
      }
    }
    else if (*(ushort *)
              ((longlong)*_refptr_g_prevTouchesPtr +
              (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x518) < uVar2) {
      return 0;
    }
    break;
  case 2:
  case 6:
  case 7:
    uVar2 = GripFilter_GetEdgeClickRatioThold(3);
    cVar1 = GripFilter_IsOnDim1();
    if (cVar1 == '\0') {
      if (*(ushort *)
           ((longlong)*_refptr_g_prevTouchesPtr +
           (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x51a) < uVar2) {
        return 0;
      }
    }
    else if (*(ushort *)
              ((longlong)*_refptr_g_prevTouchesPtr +
              (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x518) < uVar2) {
      return 0;
    }
    break;
  case 3:
  case 0xb:
    cVar1 = GripFilter_IsOnDim1();
    if (cVar1 == '\0') {
      if (*(ushort *)
           ((longlong)*_refptr_g_prevTouchesPtr +
           (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x516) <
          (ushort)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                           (ulonglong)*(uint *)(_refptr_g_grip + 4) * 0xc + -0x12)) {
        return 0;
      }
    }
    else if (*(ushort *)
              ((longlong)*_refptr_g_prevTouchesPtr +
              (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x514) <
             (ushort)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                              (ulonglong)*(uint *)(_refptr_g_grip + 4) * 0xc + -0x12)) {
      return 0;
    }
    break;
  case 4:
    cVar1 = TSAStatic_IsScreenRotated();
    if (((cVar1 != '\0') &&
        ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                    *(longlong *)_refptr_g_touchesPtr + 0x214) & 0x20000) != 0)) &&
       (cVar1 = GripFilter_IsNearbyCorner(*_refptr_g_grip), cVar1 != '\0')) {
      return 1;
    }
    cVar1 = GripFilter_IsOnDim1();
    if (cVar1 == '\0') {
      cVar1 = TSAStatic_IsScreenRotated();
      if (cVar1 == '\0') {
        if (*(ushort *)
             ((longlong)*_refptr_g_prevTouchesPtr +
             (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x51a) <
            (ushort)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                             (ulonglong)*(uint *)(_refptr_g_grip + 4) * 0xc + -0x13)) {
          return 0;
        }
      }
      else if ((*(ushort *)
                 ((longlong)*_refptr_g_prevTouchesPtr +
                 (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x51e) <
                (ushort)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                                 (ulonglong)*(uint *)(_refptr_g_grip + 4) * 0xc + -0x13)) ||
              (*(ushort *)
                ((longlong)*_refptr_g_prevTouchesPtr +
                (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x51a) <
               (ushort)(*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                                 (ulonglong)*(uint *)(_refptr_g_grip + 4) * 0xc + -0x13) >> 1))) {
        return 0;
      }
    }
    else if ((*(ushort *)
               ((longlong)*_refptr_g_prevTouchesPtr +
               (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x51c) <
              (ushort)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                               (ulonglong)*(uint *)(_refptr_g_grip + 4) * 0xc + -0x13)) ||
            (*(ushort *)
              ((longlong)*_refptr_g_prevTouchesPtr +
              (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x518) < 0x10)) {
      return 0;
    }
    break;
  case 5:
  case 0xc:
    uVar2 = GripFilter_GetEdgeClickRatioThold(0);
    cVar1 = GripFilter_IsOnDim1();
    if (cVar1 == '\0') {
      if ((*(ushort *)
            ((longlong)*_refptr_g_prevTouchesPtr +
            (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x51a) < uVar2) ||
         (0x100 < *(ushort *)
                   ((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                    *(longlong *)_refptr_g_touchesPtr + 0x236))) {
        return 0;
      }
    }
    else if ((*(ushort *)
               ((longlong)*_refptr_g_prevTouchesPtr +
               (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x518) < uVar2) ||
            (0x100 < *(ushort *)
                      ((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                       *(longlong *)_refptr_g_touchesPtr + 0x234))) {
      return 0;
    }
    break;
  case 9:
    if ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                   *(longlong *)_refptr_g_touchesPtr + 0x1f8) & 0x20000000) != 0) {
      return 0;
    }
    if (*(ushort *)
         ((longlong)*_refptr_g_prevTouchesPtr +
         (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x518) <
        (ushort)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                         (ulonglong)*(uint *)(_refptr_g_grip + 4) * 0xc + -0x14)) {
      return 0;
    }
    break;
  case 10:
    local_a = (ushort)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                               (ulonglong)*(uint *)(_refptr_g_grip + 4) * 0xc + -0x14);
    if ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                   *(longlong *)_refptr_g_touchesPtr + 0x1f8) & 0x20000000) != 0) {
      local_a = local_a + (*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                                    (ulonglong)*(uint *)(_refptr_g_grip + 4) * 0xc + -0x14) >> 2);
    }
    if (*(ushort *)
         ((longlong)*_refptr_g_prevTouchesPtr +
         (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x26a) < local_a) {
      return 0;
    }
    break;
  case 0xd:
    cVar1 = GripFilter_IsOnDim1();
    if ((cVar1 == '\0') ||
       ((ulonglong)
        ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
        *(longlong *)
         ((longlong)*_refptr_g_prevTouchesPtr +
         (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x520)) < 0x29)) {
      return 0;
    }
    if (*(ushort *)
         ((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
         0x1a8) < (ushort)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                                   (ulonglong)*(uint *)(_refptr_g_grip + 4) * 0xc + -0x14)) {
      return 0;
    }
    break;
  case 0xe:
    uVar2 = GripFilter_GetEdgeClickRatioThold(0);
    cVar1 = GripFilter_IsOnDim1();
    if (cVar1 == '\0') {
      cVar1 = TSAStatic_IsScreenRotated();
      if (cVar1 == '\0') {
        if (*(ushort *)
             ((longlong)*_refptr_g_prevTouchesPtr +
             (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x51a) < uVar2) {
          return 0;
        }
      }
      else if (*(ushort *)
                ((longlong)*_refptr_g_prevTouchesPtr +
                (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x26c) < uVar2) {
        return 0;
      }
    }
    else if (*(ushort *)
              ((longlong)*_refptr_g_prevTouchesPtr +
              (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x26a) < uVar2) {
      return 0;
    }
    break;
  case 0xf:
    if ((8 < *(ushort *)
              ((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
               *(longlong *)_refptr_g_touchesPtr + 0x1a8)) ||
       (8 < *(ushort *)
             ((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr
             + 0x1aa))) {
      return 0;
    }
    break;
  case 0x11:
    uVar2 = (ushort)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                             (ulonglong)*(uint *)(_refptr_g_grip + 4) * 0xc + -0x12);
    cVar1 = GripFilter_IsOnDim1();
    if (cVar1 == '\0') {
      if (*(ushort *)
           ((longlong)*_refptr_g_prevTouchesPtr +
           (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x26c) < uVar2) {
        return 0;
      }
    }
    else if (*(ushort *)
              ((longlong)*_refptr_g_prevTouchesPtr +
              (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x26a) < uVar2) {
      return 0;
    }
  }
  return 1;
}


// ============================================================================
// Function: GripFilter_GetGripLevel @ 6baea5a5
// ============================================================================

byte GripFilter_GetGripLevel(byte param_1)

{
  byte local_9;
  
                    /* 0xaa5a5  675  GripFilter_GetGripLevel */
  for (local_9 = 3; (-1 < (char)local_9 && ((int)(uint)param_1 < 1 << (local_9 & 0x1f)));
      local_9 = local_9 - 1) {
  }
  if ((char)local_9 < '\0') {
    local_9 = 0;
  }
  return local_9;
}


// ============================================================================
// Function: GripFilter_IsPad @ 6bae1e1c
// ============================================================================

bool GripFilter_IsPad(void)

{
                    /* 0xa1e1c  747  GripFilter_IsPad */
  return *(int *)&_refptr_g_tsaPrmtFlash[1].field_0x84 == 4;
}


// ============================================================================
// Function: GripFilter_RegionSetTopLeftCorner @ 6baeaf42
// ============================================================================

void GripFilter_RegionSetTopLeftCorner(byte param_1)

{
  byte bVar1;
  
                    /* 0xaaf42  793  GripFilter_RegionSetTopLeftCorner */
  bVar1 = (byte)(1 << (param_1 & 0x1f));
  **(byte **)_refptr_g_tsaBufGripRegion = **(byte **)_refptr_g_tsaBufGripRegion | bVar1;
  *(byte *)(*(longlong *)_refptr_g_tsaBufGripRegion + 1) =
       *(byte *)(*(longlong *)_refptr_g_tsaBufGripRegion + 1) | bVar1;
  *(byte *)(*(longlong *)_refptr_g_tsaBufGripRegion + (ulonglong)_refptr_g_tsaPrmtFlash->bCols) =
       *(byte *)((ulonglong)_refptr_g_tsaPrmtFlash->bCols + *(longlong *)_refptr_g_tsaBufGripRegion)
       | bVar1;
  return;
}


// ============================================================================
// Function: GripFilter_RegionSetTopRightCorner @ 6baeb0a9
// ============================================================================

void GripFilter_RegionSetTopRightCorner(byte param_1)

{
  byte bVar1;
  byte bVar2;
  
                    /* 0xab0a9  794  GripFilter_RegionSetTopRightCorner */
  bVar2 = _refptr_g_tsaPrmtFlash->bCols - 1;
  bVar1 = (byte)(1 << (param_1 & 0x1f));
  *(byte *)(*(longlong *)_refptr_g_tsaBufGripRegion + (ulonglong)bVar2) =
       *(byte *)((ulonglong)bVar2 + *(longlong *)_refptr_g_tsaBufGripRegion) | bVar1;
  *(byte *)(*(longlong *)_refptr_g_tsaBufGripRegion + (ulonglong)(bVar2 - 1 & 0xffff)) =
       *(byte *)((ulonglong)(bVar2 - 1 & 0xffff) + *(longlong *)_refptr_g_tsaBufGripRegion) | bVar1;
  *(byte *)(*(longlong *)_refptr_g_tsaBufGripRegion +
           (ulonglong)((uint)bVar2 + (uint)_refptr_g_tsaPrmtFlash->bCols)) =
       *(byte *)((ulonglong)((uint)bVar2 + (uint)_refptr_g_tsaPrmtFlash->bCols) +
                *(longlong *)_refptr_g_tsaBufGripRegion) | bVar1;
  return;
}


// ============================================================================
// Function: GripFilter_RegionSetBottomLeftCorner @ 6baeb21d
// ============================================================================

void GripFilter_RegionSetBottomLeftCorner(byte param_1)

{
  byte bVar1;
  byte bVar2;
  
                    /* 0xab21d  786  GripFilter_RegionSetBottomLeftCorner */
  bVar2 = _refptr_g_tsaPrmtFlash->bRows - 1;
  bVar1 = (byte)(1 << (param_1 & 0x1f));
  *(byte *)(*(longlong *)_refptr_g_tsaBufGripRegion +
           (ulonglong)((uint)bVar2 * (uint)_refptr_g_tsaPrmtFlash->bCols)) =
       *(byte *)((ulonglong)((uint)bVar2 * (uint)_refptr_g_tsaPrmtFlash->bCols) +
                *(longlong *)_refptr_g_tsaBufGripRegion) | bVar1;
  *(byte *)(*(longlong *)_refptr_g_tsaBufGripRegion +
           (ulonglong)((uint)bVar2 * (uint)_refptr_g_tsaPrmtFlash->bCols + 1 & 0xffff)) =
       *(byte *)((ulonglong)((uint)bVar2 * (uint)_refptr_g_tsaPrmtFlash->bCols + 1 & 0xffff) +
                *(longlong *)_refptr_g_tsaBufGripRegion) | bVar1;
  *(byte *)(*(longlong *)_refptr_g_tsaBufGripRegion +
           (ulonglong)((bVar2 - 1) * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)) =
       *(byte *)((ulonglong)((bVar2 - 1) * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) +
                *(longlong *)_refptr_g_tsaBufGripRegion) | bVar1;
  return;
}


// ============================================================================
// Function: GripFilter_RegionSetBottomRightCorner @ 6baeb391
// ============================================================================

void GripFilter_RegionSetBottomRightCorner(byte param_1)

{
  byte bVar1;
  byte bVar2;
  byte bVar3;
  
                    /* 0xab391  787  GripFilter_RegionSetBottomRightCorner */
  bVar2 = _refptr_g_tsaPrmtFlash->bCols - 1;
  bVar3 = _refptr_g_tsaPrmtFlash->bRows - 1;
  bVar1 = (byte)(1 << (param_1 & 0x1f));
  *(byte *)(*(longlong *)_refptr_g_tsaBufGripRegion +
           (ulonglong)((uint)bVar2 + (uint)bVar3 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)) =
       *(byte *)((ulonglong)
                 ((uint)bVar2 + (uint)bVar3 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) +
                *(longlong *)_refptr_g_tsaBufGripRegion) | bVar1;
  *(byte *)(*(longlong *)_refptr_g_tsaBufGripRegion +
           (ulonglong)
           (((uint)bVar2 + (uint)bVar3 * (uint)_refptr_g_tsaPrmtFlash->bCols) - 1 & 0xffff)) =
       *(byte *)((ulonglong)
                 (((uint)bVar2 + (uint)bVar3 * (uint)_refptr_g_tsaPrmtFlash->bCols) - 1 & 0xffff) +
                *(longlong *)_refptr_g_tsaBufGripRegion) | bVar1;
  *(byte *)(*(longlong *)_refptr_g_tsaBufGripRegion +
           (ulonglong)((uint)bVar2 + (bVar3 - 1) * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)) =
       *(byte *)((ulonglong)
                 ((uint)bVar2 + (bVar3 - 1) * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) +
                *(longlong *)_refptr_g_tsaBufGripRegion) | bVar1;
  return;
}


// ============================================================================
// Function: GripFilter_RegionSetPalm @ 6baeb512
// ============================================================================

void GripFilter_RegionSetPalm(uint param_1,byte param_2,byte param_3)

{
  byte bVar1;
  byte bVar2;
  byte local_a;
  byte local_9;
  
                    /* 0xab512  792  GripFilter_RegionSetPalm */
  bVar1 = (byte)(1 << (param_2 & 0x1f));
  if ((param_1 & 0x100) != 0) {
    for (local_a = 0; local_a < 10; local_a = local_a + 1) {
      *(byte *)(*(longlong *)_refptr_g_tsaBufGripRegion +
               (ulonglong)
               ((uint)param_3 + (uint)local_a * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)) =
           *(byte *)((ulonglong)
                     ((uint)param_3 + (uint)local_a * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)
                    + *(longlong *)_refptr_g_tsaBufGripRegion) | bVar1;
    }
  }
  if ((param_1 & 0x200) != 0) {
    bVar2 = (_refptr_g_tsaPrmtFlash->bCols - param_3) - 1;
    for (local_a = 0; local_a < 10; local_a = local_a + 1) {
      *(byte *)(*(longlong *)_refptr_g_tsaBufGripRegion +
               (ulonglong)
               ((uint)bVar2 + (uint)local_a * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)) =
           *(byte *)((ulonglong)
                     ((uint)bVar2 + (uint)local_a * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) +
                    *(longlong *)_refptr_g_tsaBufGripRegion) | bVar1;
    }
  }
  if ((param_1 & 0x400) != 0) {
    for (local_a = 0; local_a < 10; local_a = local_a + 1) {
      *(byte *)(*(longlong *)_refptr_g_tsaBufGripRegion +
               (ulonglong)
               ((uint)param_3 +
                ((_refptr_g_tsaPrmtFlash->bRows - 1) - (uint)local_a) *
                (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)) =
           *(byte *)((ulonglong)
                     ((uint)param_3 +
                      ((_refptr_g_tsaPrmtFlash->bRows - 1) - (uint)local_a) *
                      (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) +
                    *(longlong *)_refptr_g_tsaBufGripRegion) | bVar1;
    }
  }
  if ((param_1 & 0x800) != 0) {
    bVar2 = (_refptr_g_tsaPrmtFlash->bCols - param_3) - 1;
    for (local_a = 0; local_a < 10; local_a = local_a + 1) {
      *(byte *)(*(longlong *)_refptr_g_tsaBufGripRegion +
               (ulonglong)
               ((uint)bVar2 +
                ((_refptr_g_tsaPrmtFlash->bRows - 1) - (uint)local_a) *
                (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)) =
           *(byte *)((ulonglong)
                     ((uint)bVar2 +
                      ((_refptr_g_tsaPrmtFlash->bRows - 1) - (uint)local_a) *
                      (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) +
                    *(longlong *)_refptr_g_tsaBufGripRegion) | bVar1;
    }
  }
  if ((param_1 & 0x1000) != 0) {
    for (local_9 = 0; local_9 < 10; local_9 = local_9 + 1) {
      *(byte *)(*(longlong *)_refptr_g_tsaBufGripRegion +
               (ulonglong)
               ((uint)local_9 + (uint)param_3 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)) =
           *(byte *)((ulonglong)
                     ((uint)local_9 + (uint)param_3 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)
                    + *(longlong *)_refptr_g_tsaBufGripRegion) | bVar1;
    }
  }
  if ((param_1 & 0x4000) != 0) {
    bVar2 = (_refptr_g_tsaPrmtFlash->bRows - param_3) - 1;
    for (local_9 = 0; local_9 < 10; local_9 = local_9 + 1) {
      *(byte *)(*(longlong *)_refptr_g_tsaBufGripRegion +
               (ulonglong)
               ((uint)local_9 + (uint)bVar2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)) =
           *(byte *)((ulonglong)
                     ((uint)local_9 + (uint)bVar2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) +
                    *(longlong *)_refptr_g_tsaBufGripRegion) | bVar1;
    }
  }
  if ((param_1 & 0x2000) != 0) {
    for (local_9 = 0; local_9 < 10; local_9 = local_9 + 1) {
      *(byte *)(*(longlong *)_refptr_g_tsaBufGripRegion +
               (ulonglong)
               ((((uint)_refptr_g_tsaPrmtFlash->bCols - (uint)local_9) +
                (uint)param_3 * (uint)_refptr_g_tsaPrmtFlash->bCols) - 1 & 0xffff)) =
           *(byte *)((ulonglong)
                     ((((uint)_refptr_g_tsaPrmtFlash->bCols - (uint)local_9) +
                      (uint)param_3 * (uint)_refptr_g_tsaPrmtFlash->bCols) - 1 & 0xffff) +
                    *(longlong *)_refptr_g_tsaBufGripRegion) | bVar1;
    }
  }
  if ((param_1 & 0x8000) != 0) {
    bVar2 = (_refptr_g_tsaPrmtFlash->bRows - param_3) - 1;
    for (local_9 = 0; local_9 < 10; local_9 = local_9 + 1) {
      *(byte *)(*(longlong *)_refptr_g_tsaBufGripRegion +
               (ulonglong)
               ((((uint)_refptr_g_tsaPrmtFlash->bCols - (uint)local_9) +
                (uint)bVar2 * (uint)_refptr_g_tsaPrmtFlash->bCols) - 1 & 0xffff)) =
           *(byte *)((ulonglong)
                     ((((uint)_refptr_g_tsaPrmtFlash->bCols - (uint)local_9) +
                      (uint)bVar2 * (uint)_refptr_g_tsaPrmtFlash->bCols) - 1 & 0xffff) +
                    *(longlong *)_refptr_g_tsaBufGripRegion) | bVar1;
    }
  }
  return;
}


// ============================================================================
// Function: GripFilter_IsGripOrTbd @ 6bae1966
// ============================================================================

undefined8 GripFilter_IsGripOrTbd(int param_1)

{
  char cVar1;
  
                    /* 0xa1966  714  GripFilter_IsGripOrTbd */
  if ((param_1 != 0) && (cVar1 = GripFilter_IsGrip(param_1), cVar1 == '\0')) {
    return 0;
  }
  return 1;
}


// ============================================================================
// Function: GripFilter_DeadzoneReset @ 6bae4ce0
// ============================================================================

void GripFilter_DeadzoneReset(void)

{
  int iVar1;
  
  iVar1 = memset_s(&g_gripFilterDeadZone,0x140,0,0x140);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/GripFilter/GripFilter_DeadZone.c",0x1c,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: GripFilter_IsLongPressAllowed @ 6bae28e0
// ============================================================================

undefined8 GripFilter_IsLongPressAllowed(void)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0xa28e0  731  GripFilter_IsLongPressAllowed */
  if ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214) &
      0x2000) == 0) {
    cVar1 = GripFilter_IsRatioOK(6);
    if ((cVar1 == '\0') ||
       ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214)
        & 0x400000) == 0)) {
      if ((*(short *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x19e) < 5000) && (cVar1 = GripFilter_IsRatioOK(0xb), cVar1 != '\0')) {
        return 0;
      }
      cVar1 = GripFilter_IsRatioOK(0xe);
      if (cVar1 == '\0') {
        if ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x214) & 0x4000000) == 0) {
          cVar1 = GripFilter_IsCornerLongPressAllowed();
          if (cVar1 == '\0') {
            uVar2 = 0;
          }
          else {
            cVar1 = GripFilter_IsVideoModeLongPressAllowed();
            if (cVar1 == '\0') {
              uVar2 = 0;
            }
            else {
              cVar1 = GripFilter_IsScreenRotatedStateLongPressAllowed();
              if (cVar1 == '\0') {
                uVar2 = 0;
              }
              else {
                cVar1 = GripFilter_BasicIsTempGripAbnormal();
                if (cVar1 == '\0') {
                  if ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 +
                                 *(longlong *)_refptr_g_touchesPtr + 0x214) & 0x40000) == 0) {
                    uVar2 = 1;
                  }
                  else {
                    uVar2 = 0;
                  }
                }
                else {
                  uVar2 = 0;
                }
              }
            }
          }
        }
        else {
          uVar2 = 0;
        }
      }
      else {
        uVar2 = 0;
      }
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
// Function: GripFilter_BasicIsTempGripAbnormal @ 6bae3618
// ============================================================================

undefined8 GripFilter_BasicIsTempGripAbnormal(void)

{
  char cVar1;
  char cVar2;
  undefined8 uVar3;
  bool local_9;
  
                    /* 0xa3618  652  GripFilter_BasicIsTempGripAbnormal */
  cVar1 = GripFilter_IsToCheckAbnormal();
  if (cVar1 == '\0') {
    return 0;
  }
  cVar1 = GripFilter_IsSignalOK(g_grip,0x10,0);
  cVar2 = GripFilter_IsAbnormalLiftOff();
  local_9 = cVar2 != '\0' || cVar1 != '\0';
  if (local_9) {
    if (0x18 < *(ushort *)
                ((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x228)) {
      *(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214) =
           *(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214
                    ) | 0x40000;
      return 1;
    }
    if ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x218)
        & 4) != 0) {
      return 1;
    }
    if ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x218)
        & 8) != 0) {
      return 1;
    }
    if ((0xf < *(ushort *)
                ((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x228))
       && (cVar1 = GripFilter_IsHeavyRegionTouched(g_grip), cVar1 != '\0')) {
      return 1;
    }
    if (4 < *(byte *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1ad)) {
      return 1;
    }
  }
  if ((*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                 (longlong)(int)(uint)DAT_6bc84e21 * 0x478 + 0x5a8) < 0x32) &&
     ((int)*(short *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1ba) <
      *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                (longlong)(int)(uint)DAT_6bc84e21 * 0x478 + 0x5a6) * 2)) {
    uVar3 = 1;
  }
  else {
    uVar3 = 0;
  }
  return uVar3;
}


// ============================================================================
// Function: GripFilter_IsNegSigTouch @ 6bae38c7
// ============================================================================

undefined8 GripFilter_IsNegSigTouch(void)

{
  char cVar1;
  
  if ((((_refptr_g_tsaPrmtDynamic[0x96] != '\0') &&
       (*(char *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x2a4)
        != '\0')) &&
      (*(ushort *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x228)
       < 0xf)) &&
     ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x158) <
       1000 && (cVar1 = Touch_IsOff(g_grip), cVar1 == '\0')))) {
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: GripFilter_IsLongPress @ 6bae675f
// ============================================================================

undefined4 GripFilter_IsLongPress(void)

{
  uint uVar1;
  undefined4 uVar2;
  char cVar3;
  
                    /* 0xa675f  730  GripFilter_IsLongPress */
  cVar3 = GripFilter_IsVideoMode();
  if ((cVar3 == '\0') &&
     ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                 *(longlong *)_refptr_g_touchesPtr + 0x214) & 0x10000) == 0)) {
    uVar1 = *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                      *(longlong *)_refptr_g_touchesPtr + 0x158);
    uVar2 = CONCAT31((int3)(uVar1 >> 8),299 < uVar1);
  }
  else {
    uVar1 = *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                      *(longlong *)_refptr_g_touchesPtr + 0x158);
    uVar2 = CONCAT31((int3)(uVar1 >> 8),199 < uVar1);
  }
  return uVar2;
}


// ============================================================================
// Function: GripFilter_IsGlobalSuppressedClick @ 6bae82c8
// ============================================================================

undefined8 GripFilter_IsGlobalSuppressedClick(void)

{
  char cVar1;
  
                    /* 0xa82c8  710  GripFilter_IsGlobalSuppressedClick */
  if (((((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                    *(longlong *)_refptr_g_touchesPtr + 0x214) & 1) != 0) &&
       ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                   *(longlong *)_refptr_g_touchesPtr + 0x214) & 0x20000) != 0)) &&
      ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                  *(longlong *)_refptr_g_touchesPtr + 0x1fc) & 0x4000000) != 0)) &&
     (cVar1 = GripFilter_IsSignalOK(*_refptr_g_grip,1,1), cVar1 != '\0')) {
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: GripFilter_IsSuppressedGlobally2 @ 6bae2a9d
// ============================================================================

undefined8 GripFilter_IsSuppressedGlobally2(void)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0xa2a9d  763  GripFilter_IsSuppressedGlobally2 */
  if (((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214) &
       1) != 0) && (cVar1 = GripFilter_IsRatioOK(2), cVar1 != '\0')) {
    cVar1 = TSAStatic_IsScreenRotated();
    if ((cVar1 != '\0') &&
       ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214)
        & 0x20000) != 0)) {
      return 1;
    }
    if ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214)
        & 0x2000000) != 0) {
      return 1;
    }
  }
  if (((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214) &
       0x2000000) == 0) ||
     (*(ushort *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x228) <
      0x19)) {
    cVar1 = TSAStatic_IsScreenRotated();
    if ((cVar1 != '\0') &&
       ((((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214
                    ) & 0x2000000) != 0 && (_refptr_g_touchesSide[5] != '\0')) &&
        (cVar1 = GripFilter_IsRatioOK(2), cVar1 != '\0')))) {
      return 1;
    }
    uVar2 = 0;
  }
  else {
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: GripFilter_IsSuppressedGlobally @ 6bae3211
// ============================================================================

undefined8 GripFilter_IsSuppressedGlobally(void)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0xa3211  762  GripFilter_IsSuppressedGlobally */
  cVar1 = GripFilter_IsSuppressedByHandGesture();
  if (cVar1 == '\0') {
    cVar1 = GripFilter_IsSuppressedByCoreTouch();
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
  return uVar2;
}


// ============================================================================
// Function: GripFilter_BasicIsAbsGripPrmt @ 6bae3c2f
// ============================================================================

undefined8 GripFilter_BasicIsAbsGripPrmt(void)

{
  char cVar1;
  ushort local_a;
  
                    /* 0xa3c2f  647  GripFilter_BasicIsAbsGripPrmt */
  if (*DAT_6bc84e30 != '\0') {
    local_a = GripFilter_GetAbsThold(*(undefined2 *)(DAT_6bc84e30 + 8));
    cVar1 = GripFilter_IsVideoMode();
    if (cVar1 != '\0') {
      local_a = 0x14;
    }
    if (((*(char *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x25c)
          != '\0') || (cVar1 = Touch_IsOff(g_grip), cVar1 != '\0')) &&
       (local_a < *(ushort *)
                   ((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x25e)
       )) {
      return 1;
    }
  }
  return 0;
}


// ============================================================================
// Function: GripFilter_BasicIsAbsGripAbnormal @ 6bae3d07
// ============================================================================

undefined8 GripFilter_BasicIsAbsGripAbnormal(void)

{
  char cVar1;
  byte bVar2;
  undefined8 uVar3;
  
                    /* 0xa3d07  646  GripFilter_BasicIsAbsGripAbnormal */
  if ((((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1fc)
        & 0x3000000) != 0) && (cVar1 = GripFilter_IsToCheckAbnormal(), cVar1 != '\0')) &&
     ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214) &
      0x200) == 0)) {
    return 1;
  }
  if ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214) &
      0x10000000) == 0) {
    if (((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214)
         & 8) != 0) && (bVar2 = GripFilter_GetCenterLevel(g_grip), 1 < bVar2)) {
      return 1;
    }
    cVar1 = GripFilter_BasicIsTempGripAbnormal();
    if ((cVar1 != '\0') && (cVar1 = GripFilter_IsLongPress(), cVar1 != '\0')) {
      return 1;
    }
    if (((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214)
         & 0x80) == 0) && (cVar1 = GripFilter_IsLongPress(), cVar1 != '\0')) {
      return 1;
    }
    cVar1 = GripFilter_BasicIsTempGripAbnormal();
    if (((cVar1 == '\0') ||
        (*(ushort *)
          ((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x156) < 5)) ||
       (*(ushort *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x188)
        < 0x19)) {
      uVar3 = 0;
    }
    else {
      uVar3 = 1;
    }
  }
  else {
    uVar3 = 1;
  }
  return uVar3;
}


// ============================================================================
// Function: GripFilter_IsNegSigTouchBypass @ 6bae3f02
// ============================================================================

undefined8 GripFilter_IsNegSigTouchBypass(void)

{
  undefined8 uVar1;
  
  if (((_refptr_g_tsaPrmtDynamic[0x96] == '\0') ||
      (*(char *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x2a4) ==
       '\0')) ||
     ((*(ushort *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x228)
       < 0xf && (*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                          0x158) < 1000)))) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: GripFilter_IsCornerGripClick @ 6bae7060
// ============================================================================

undefined8 GripFilter_IsCornerGripClick(void)

{
  undefined8 uVar1;
  
                    /* 0xa7060  705  GripFilter_IsCornerGripClick */
  if (((((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                    *(longlong *)_refptr_g_touchesPtr + 0x1fc) & 0x10000000) == 0) ||
       ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                   *(longlong *)_refptr_g_touchesPtr + 0x1fc) & 0x20000000) == 0)) ||
      ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                  *(longlong *)_refptr_g_touchesPtr + 0x214) & 1) == 0)) ||
     (((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                  *(longlong *)_refptr_g_touchesPtr + 0x214) & 0x20000) == 0 ||
      (*(ushort *)
        ((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
        0x24e) < 6)))) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: GripFilter_IsNearbyCorner @ 6baea708
// ============================================================================

undefined8 GripFilter_IsNearbyCorner(byte param_1)

{
  undefined8 uVar1;
  
                    /* 0xaa708  738  GripFilter_IsNearbyCorner */
  if (((*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1a8)
        == 0) ||
      (*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1aa)
       == 0)) ||
     ((uint)*(ushort *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1aa) +
      (uint)*(ushort *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1a8) < 8)
     ) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: GripFilter_IsOnDim2 @ 6baec7e8
// ============================================================================

undefined8 GripFilter_IsOnDim2(void)

{
  undefined8 uVar1;
  
                    /* 0xac7e8  742  GripFilter_IsOnDim2 */
  if (((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                  *(longlong *)_refptr_g_touchesPtr + 0x1c4) & 1) == 0) &&
     ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                 *(longlong *)_refptr_g_touchesPtr + 0x1c4) & 2) == 0)) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: GripFilter_IsBackToGripAllowed @ 6bae1f52
// ============================================================================

undefined8 GripFilter_IsBackToGripAllowed(void)

{
  undefined8 uVar1;
  
                    /* 0xa1f52  701  GripFilter_IsBackToGripAllowed */
  if ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214) &
      0x200000) == 0) {
    if ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214)
        & 0x100000) == 0) {
      uVar1 = 0;
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
// Function: GripFilter_IsBackToGripSatisfied @ 6bae1fd5
// ============================================================================

undefined8 GripFilter_IsBackToGripSatisfied(void)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0xa1fd5  702  GripFilter_IsBackToGripSatisfied */
  if ((*(short *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x29e) <
       *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                 (longlong)(int)(uint)DAT_6bc84e21 * 0x478 + 0x1ba) / 3) &&
     (cVar1 = Peak_IsOnOuterEdge(*(undefined1 *)
                                  ((longlong)(int)(uint)g_grip * 0x168 +
                                   *(longlong *)_refptr_g_touchesPtr + 0x23c)), cVar1 != '\0')) {
    return 1;
  }
  if ((*(ushort *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22c)
       < 0x20) ||
     (*(ushort *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22e) <
      0x20)) {
    uVar2 = 1;
  }
  else {
    uVar2 = 0;
  }
  return uVar2;
}


// ============================================================================
// Function: GripFilter_IsStuckSideTouch @ 6bae223a
// ============================================================================

undefined8 GripFilter_IsStuckSideTouch(void)

{
  char cVar1;
  
                    /* 0xa223a  759  GripFilter_IsStuckSideTouch */
  cVar1 = GripFilter_IsToCheckStuckSideTouch();
  if (cVar1 != '\0') {
    if ((500 < *(short *)((ulonglong)
                          ((uint)(byte)((ushort)*(undefined2 *)
                                                 ((longlong)(int)(uint)g_grip * 0x168 +
                                                  *(longlong *)_refptr_g_touchesPtr + 0x162) >> 8) +
                           (uint)(byte)((ushort)*(undefined2 *)
                                                 ((longlong)(int)(uint)g_grip * 0x168 +
                                                  *(longlong *)_refptr_g_touchesPtr + 0x164) >> 8) *
                           (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
                         *(longlong *)_refptr_g_tsaBufGripTime)) &&
       ((*(ushort *)
          ((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22c) < 0x20
        || (*(ushort *)
             ((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22e) <
            0x20)))) {
      return 1;
    }
    cVar1 = GripFilter_IsInDynamicDeadZone(0x20);
    if (cVar1 != '\0') {
      return 1;
    }
  }
  return 0;
}


// ============================================================================
// Function: GripFilter_IsSeparated @ 6bae864d
// ============================================================================

undefined8 GripFilter_IsSeparated(byte param_1,byte param_2)

{
  ushort uVar1;
  ushort uVar2;
  uint uVar3;
  undefined8 uVar4;
  
                    /* 0xa864d  756  GripFilter_IsSeparated */
  if (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214)
       & 0x200) == 0) &&
     ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                 *(longlong *)_refptr_g_touchesPtr + 0x214) & 0x200000) == 0)) {
    uVar4 = 0;
  }
  else if (*(ushort *)
            ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22c) <
           0x101) {
    uVar1 = *(ushort *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22c);
    uVar2 = TS_MMToPixel(4);
    if ((uint)*(ushort *)
               ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x22c)
        < (uint)uVar1 + (uint)uVar2) {
      uVar4 = 0;
    }
    else if (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x1c4) & 4) == 0) &&
            ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x1c4) & 8) == 0)) {
      uVar4 = 0;
    }
    else {
      uVar1 = *(ushort *)
               ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1a8);
      uVar3 = TSA_SideToEdge(*(undefined4 *)
                              ((longlong)*_refptr_g_prevTouchesPtr +
                              (longlong)(int)(uint)param_2 * 0x478 + 0x1c4));
      if (uVar1 < *(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                           (ulonglong)uVar3 * 0xc + -0x12)) {
        uVar4 = 0;
      }
      else {
        uVar4 = 1;
      }
    }
  }
  else {
    uVar4 = 0;
  }
  return uVar4;
}


// ============================================================================
// Function: GripFilter_GetDelayTime @ 6bae4796
// ============================================================================

undefined2 GripFilter_GetDelayTime(void)

{
  char cVar1;
  undefined2 uVar2;
  
                    /* 0xa4796  671  GripFilter_GetDelayTime */
  cVar1 = TSA_IsWindowsPadFeatureEnabled();
  if (cVar1 == '\0') {
    cVar1 = GripFilter_IsToApplyMaxDelay();
    if (cVar1 == '\0') {
      cVar1 = GripFilter_IsToApplyDelay();
      if ((cVar1 == '\0') && (cVar1 = GripFilter_IsToBypassDelay(), cVar1 != '\0')) {
        return 0;
      }
      uVar2 = *(undefined2 *)(DAT_6bc84e30 + 6);
    }
    else {
      uVar2 = 600;
    }
  }
  else {
    uVar2 = 0x32;
  }
  return uVar2;
}


// ============================================================================
// Function: GripFilter_GetEdgeWidthStart @ 6bae4d44
// ============================================================================

char GripFilter_GetEdgeWidthStart(ushort param_1,ushort param_2)

{
  char cVar1;
  
  if (param_1 >> 8 < param_2 >> 1) {
    cVar1 = '\0';
  }
  else {
    cVar1 = (char)(param_1 >> 8) - (char)(param_2 >> 1);
  }
  return cVar1;
}


// ============================================================================
// Function: GripFilter_GetEdgeWidthEnd @ 6bae4d95
// ============================================================================

char GripFilter_GetEdgeWidthEnd(ushort param_1,ushort param_2,byte param_3)

{
  char cVar1;
  
  if ((int)(param_3 - 1) < (int)((uint)(param_1 >> 8) + (uint)(param_2 >> 1))) {
    cVar1 = param_3 - 1;
  }
  else {
    cVar1 = (char)(param_2 >> 1) + (char)(param_1 >> 8);
  }
  return cVar1;
}


// ============================================================================
// Function: GripFilter_IsGripCriteriaDeadzone @ 6bae4df8
// ============================================================================

undefined8 GripFilter_IsGripCriteriaDeadzone(byte param_1)

{
  undefined8 uVar1;
  
  if (((((_refptr_g_tsaPrmtFlash[1].field_0x80 & 4) == 0) &&
       ((_refptr_g_tsaPrmtFlash[1].field_0x80 & 8) == 0)) ||
      ((0xff < *(ushort *)
                ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x15e)
       && ((int)(uint)*(ushort *)
                       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x15e) <= (int)((_refptr_g_tsaPrmtFlash->bCols - 1) * 0x100))))) ||
     ((*(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1b1)
       <= _refptr_g_tsaPrmtFlash->bRows >> 2 &&
      (*(ushort *)
        ((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
        0x25e) < 0x1a)))) {
    if ((((_refptr_g_tsaPrmtFlash[1].field_0x80 & 1) == 0) &&
        ((_refptr_g_tsaPrmtFlash[1].field_0x80 & 2) == 0)) ||
       (((0xff < *(ushort *)
                  ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x160)
         && ((int)(uint)*(ushort *)
                         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x160) <= (int)((_refptr_g_tsaPrmtFlash->bRows - 1) * 0x100))) ||
        ((*(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1b0
                   ) <= _refptr_g_tsaPrmtFlash->bCols >> 1 &&
         (*(ushort *)
           ((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
           0x25e) < 0x1a)))))) {
      uVar1 = 0;
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
// Function: GripFilter_GetCoreDim2Distance @ 6bae5e21
// ============================================================================

undefined4 GripFilter_GetCoreDim2Distance(byte param_1,byte param_2)

{
  char cVar1;
  undefined4 uVar2;
  
                    /* 0xa5e21  670  GripFilter_GetCoreDim2Distance */
  uVar2 = GripFilter_GetDim2Distance
                    (*(undefined1 *)
                      ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x17e),*(undefined1 *)
                              ((longlong)(int)(uint)param_1 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x17f),
                     *(undefined1 *)
                      ((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x17e),*(undefined1 *)
                              ((longlong)(int)(uint)param_2 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x17f));
  if ((((-0x200 < (int)((uint)*(ushort *)
                               ((longlong)(int)(uint)param_1 * 0x168 +
                                *(longlong *)_refptr_g_touchesPtr + 0x162) -
                       (uint)*(ushort *)
                              ((longlong)(int)(uint)param_2 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x162))) &&
       ((int)((uint)*(ushort *)
                     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x162) -
             (uint)*(ushort *)
                    ((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x162)) < 0x200)) && (cVar1 = Touch_IsOff(param_1), cVar1 == '\0')) &&
     (cVar1 = Touch_IsOff(param_2), cVar1 == '\0')) {
    return uVar2;
  }
  return 100;
}


// ============================================================================
// Function: GripFilter_CloseGripOnCore @ 6bae6b13
// ============================================================================

void GripFilter_CloseGripOnCore(void)

{
  char cVar1;
  int iVar2;
  byte local_9;
  
                    /* 0xa6b13  662  GripFilter_CloseGripOnCore */
  for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
    if (*_refptr_g_grip != local_9) {
      cVar1 = GripFilter_IsSignalOK(*_refptr_g_grip,8,1);
      if (cVar1 != '\0') {
        iVar2 = GripFilter_GetCoreDim2Distance(*_refptr_g_grip,local_9);
        if (iVar2 < 6) {
          *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                    *(longlong *)_refptr_g_touchesPtr + 0x218) =
               *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                         *(longlong *)_refptr_g_touchesPtr + 0x218) | 4;
          cVar1 = GripFilter_IsInPalmRegion(local_9);
          if (cVar1 != '\0') {
            *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                      *(longlong *)_refptr_g_touchesPtr + 0x214) =
                 *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                           *(longlong *)_refptr_g_touchesPtr + 0x214) | 0x10;
          }
          cVar1 = GripFilter_IsTouchIntervalInTime(*_refptr_g_grip,local_9,0,200);
          if (cVar1 != '\0') {
            *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                      *(longlong *)_refptr_g_touchesPtr + 0x214) =
                 *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                           *(longlong *)_refptr_g_touchesPtr + 0x214) | 8;
          }
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: GripFilter_CloseGripOnSide @ 6bae6cf9
// ============================================================================

void GripFilter_CloseGripOnSide(void)

{
  char cVar1;
  int iVar2;
  byte local_9;
  
                    /* 0xa6cf9  663  GripFilter_CloseGripOnSide */
  for (local_9 = 0; local_9 < (byte)*_refptr_g_touchesSide; local_9 = local_9 + 1) {
    iVar2 = GripFilter_GetSideDim2Distance(*_refptr_g_grip,local_9);
    if (iVar2 < 6) {
      cVar1 = GripFilter_IsSignalOK(*_refptr_g_grip,8,1);
      if (cVar1 != '\0') {
        *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                  *(longlong *)_refptr_g_touchesPtr + 0x214) =
             *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                       *(longlong *)_refptr_g_touchesPtr + 0x214) | 0x10;
        cVar1 = GripFilter_IsTouchIntervalInTime(*_refptr_g_grip,local_9,1,200);
        if (cVar1 != '\0') {
          *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                    *(longlong *)_refptr_g_touchesPtr + 0x214) =
               *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                         *(longlong *)_refptr_g_touchesPtr + 0x214) | 8;
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: GripFilter_CheckMultiGrip @ 6bae7d07
// ============================================================================

void GripFilter_CheckMultiGrip(void)

{
  char cVar1;
  
                    /* 0xa7d07  656  GripFilter_CheckMultiGrip */
  if (*_refptr_g_touchesSide != '\0') {
    cVar1 = Peak_IsToExcludeSideRegion();
    if (cVar1 != '\0') {
      GripFilter_CheckMultiGripOnSide();
      goto LAB_6bae7d32;
    }
  }
  GripFilter_CheckMultiGripOnCore();
LAB_6bae7d32:
  GripFilter_CheckCornerMultiGrip();
  GripFilter_CheckScreenRotatedMultiGrip();
  return;
}


// ============================================================================
// Function: GripFilter_BasicIsFlick @ 6bae81d7
// ============================================================================

undefined8 GripFilter_BasicIsFlick(void)

{
  char cVar1;
  
                    /* 0xa81d7  649  GripFilter_BasicIsFlick */
  cVar1 = GripFilter_IsFlickDetected();
  if ((cVar1 != '\0') && (cVar1 = GripFilter_IsFlickAllowed(), cVar1 == '\0')) {
    *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr
             + 0x214) =
         *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                   *(longlong *)_refptr_g_touchesPtr + 0x214) | 0x8000;
  }
  cVar1 = GripFilter_IsFlickAllowed();
  if ((cVar1 != '\0') && (cVar1 = GripFilter_IsFlickDetected(), cVar1 != '\0')) {
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: GripFilter_IsInExcludedRegion @ 6bae9f40
// ============================================================================

undefined8 GripFilter_IsInExcludedRegion(byte param_1)

{
  ushort uVar1;
  ushort uVar2;
  char cVar3;
  undefined8 uVar4;
  
                    /* 0xa9f40  724  GripFilter_IsInExcludedRegion */
  cVar3 = TSAStatic_IsScreenRotated();
  if (cVar3 == '\0') {
    if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) == 0) {
      uVar1 = 0x200;
    }
    else {
      uVar1 = 0x300;
    }
    uVar2 = 0x100;
  }
  else {
    uVar1 = 0x200;
    uVar2 = 0x200;
  }
  cVar3 = GripFilter_IsVideoMode();
  if ((cVar3 != '\0') && (cVar3 = GripFilter_IsNearbyCorner(*_refptr_g_grip), cVar3 != '\0')) {
    return 0;
  }
  if ((((*(ushort *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x188) < 0x14)
       && (uVar1 < *(ushort *)
                    ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x22c))) &&
      (uVar2 < *(ushort *)
                ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22e)))
     && (cVar3 = GripFilter_IsRatioOK(0xf), cVar3 != '\0')) {
    return 1;
  }
  if ((*(ushort *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22c)
       < 0x401) ||
     (*(ushort *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22e)
      < 0x301)) {
    uVar4 = 0;
  }
  else {
    uVar4 = 1;
  }
  return uVar4;
}


// ============================================================================
// Function: GripFilter_IsTouchIntervalInTime @ 6bae6536
// ============================================================================

undefined8
GripFilter_IsTouchIntervalInTime(undefined1 param_1,undefined1 param_2,char param_3,int param_4)

{
  char cVar1;
  int iVar2;
  uint uVar3;
  int local_c;
  
                    /* 0xa6536  774  GripFilter_IsTouchIntervalInTime */
  cVar1 = Touch_IsOff(param_2);
  if ((cVar1 != '\0') || (cVar1 = Touch_IsOff(param_1), cVar1 != '\0')) {
    return 0;
  }
  iVar2 = Touch_GetTimeOn(param_1);
  local_c = Touch_GetTimeOn(param_2);
  if (param_3 != '\0') {
    local_c = GripFilter_GetSideTouchOnTime(param_2);
  }
  uVar3 = iVar2 - local_c >> 0x1f;
  if ((int)((iVar2 - local_c ^ uVar3) - uVar3) < param_4) {
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: GripFilter_IsHigherPriorityTouch @ 6baeaf03
// ============================================================================

void GripFilter_IsHigherPriorityTouch(undefined1 param_1,undefined1 param_2)

{
  char cVar1;
  
                    /* 0xaaf03  719  GripFilter_IsHigherPriorityTouch */
  cVar1 = GripFilter_IsPad();
  if (cVar1 == '\0') {
    GripFilter_IsHigherPriorityTouchNormal(param_1,param_2);
  }
  else {
    GripFilter_IsHigherPriorityTouchPad(param_1,param_2);
  }
  return;
}


// ============================================================================
// Function: GripFilter_IsTouchInCornerRegion @ 6baea327
// ============================================================================

void GripFilter_IsTouchInCornerRegion(byte param_1)

{
  char cVar1;
  uchar local_a;
  uchar local_9;
  
                    /* 0xaa327  771  GripFilter_IsTouchInCornerRegion */
  cVar1 = GripFilter_IsTouchPosInCornerRegion(param_1);
  if (cVar1 == '\0') {
    local_9 = (uchar)((ushort)*(undefined2 *)
                               ((longlong)(int)(uint)param_1 * 0x168 +
                                *(longlong *)_refptr_g_touchesPtr + 0x162) >> 8);
    local_a = (uchar)((ushort)*(undefined2 *)
                               ((longlong)(int)(uint)param_1 * 0x168 +
                                *(longlong *)_refptr_g_touchesPtr + 0x164) >> 8);
  }
  else {
    local_9 = (*_refptr_g_peaksPtr)
              [(longlong)
               (int)(uint)*(byte *)((longlong)(int)(uint)param_1 * 0x168 +
                                    *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x15].
              bPeakCount;
    local_a = (*_refptr_g_peaksPtr)
              [(longlong)
               (int)(uint)*(byte *)((longlong)(int)(uint)param_1 * 0x168 +
                                    *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x16].
              bPeakCount;
  }
  GripFilter_IsSensorInCornerRegion(local_9,local_a);
  return;
}


// ============================================================================
// Function: GripFilter_IsTouchInHeavyRegion @ 6baea454
// ============================================================================

bool GripFilter_IsTouchInHeavyRegion(byte param_1)

{
  char cVar1;
  byte local_a;
  byte local_9;
  
                    /* 0xaa454  773  GripFilter_IsTouchInHeavyRegion */
  cVar1 = GripFilter_IsTouchPosInCornerRegion(param_1);
  if (cVar1 == '\0') {
    local_9 = (byte)((ushort)*(undefined2 *)
                              ((longlong)(int)(uint)param_1 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x162) >> 8);
    local_a = (byte)((ushort)*(undefined2 *)
                              ((longlong)(int)(uint)param_1 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x164) >> 8);
  }
  else {
    local_9 = (*_refptr_g_peaksPtr)
              [(longlong)
               (int)(uint)*(byte *)((longlong)(int)(uint)param_1 * 0x168 +
                                    *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x15].
              bPeakCount;
    local_a = (*_refptr_g_peaksPtr)
              [(longlong)
               (int)(uint)*(byte *)((longlong)(int)(uint)param_1 * 0x168 +
                                    *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x16].
              bPeakCount;
  }
  return 7 < *(byte *)((ulonglong)
                       ((uint)local_9 + (uint)local_a * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff
                       ) + *(longlong *)_refptr_g_tsaBufGripRegion);
}


// ============================================================================
// Function: GripFilter_IsHeavyCornerTouched @ 6baea7ca
// ============================================================================

undefined8 GripFilter_IsHeavyCornerTouched(undefined1 param_1)

{
  char cVar1;
  
                    /* 0xaa7ca  717  GripFilter_IsHeavyCornerTouched */
  cVar1 = GripFilter_IsHeavyRegionTouched(param_1);
  if ((cVar1 != '\0') && (cVar1 = GripFilter_IsNearbyCorner(param_1), cVar1 != '\0')) {
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: GripFilter_IsOnDim1 @ 6baec770
// ============================================================================

undefined8 GripFilter_IsOnDim1(void)

{
  undefined8 uVar1;
  
                    /* 0xac770  741  GripFilter_IsOnDim1 */
  if (((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                  *(longlong *)_refptr_g_touchesPtr + 0x1c4) & 4) == 0) &&
     ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                 *(longlong *)_refptr_g_touchesPtr + 0x1c4) & 8) == 0)) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: GripFilter_GetEdgeClickRatioThold @ 6baecad0
// ============================================================================

ushort GripFilter_GetEdgeClickRatioThold(char param_1)

{
  char cVar1;
  char local_b;
  ushort local_a;
  
                    /* 0xacad0  674  GripFilter_GetEdgeClickRatioThold */
  local_b = param_1;
  if (0x11 < *(ushort *)
              ((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
               *(longlong *)_refptr_g_touchesPtr + 0x228)) {
    local_b = param_1 + '\x01';
    if (0x2f < *(byte *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                         *(longlong *)_refptr_g_touchesPtr + 0x267)) {
      local_b = param_1 + '\x02';
    }
    if (0x18 < *(ushort *)
                ((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                 *(longlong *)_refptr_g_touchesPtr + 0x228)) {
      local_b = local_b + '\x01';
    }
    if ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                   *(longlong *)_refptr_g_touchesPtr + 0x1fc) & 8) != 0) {
      cVar1 = GripFilter_IsHeavyRegionTouched(*_refptr_g_grip);
      if (cVar1 != '\0') {
        local_b = local_b + '\x01';
      }
    }
    cVar1 = GripFilter_IsLongPress();
    if (cVar1 != '\0') {
      local_b = local_b + '\x01';
    }
  }
  cVar1 = GripFilter_IsHeavyRegionTouched(*_refptr_g_grip);
  if (((cVar1 != '\0') &&
      ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                  *(longlong *)_refptr_g_touchesPtr + 0x1fc) & 0x10000000) != 0)) &&
     (*(int *)&_refptr_g_tsaPrmtFlash[1].field_0x84 != 4)) {
    cVar1 = GripFilter_IsEnableWeakenCornerClickSupress();
    if (cVar1 == '\0') {
      cVar1 = local_b + '\x01';
      if (0x20 < *(byte *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                           *(longlong *)_refptr_g_touchesPtr + 0x1b9)) {
        cVar1 = local_b + '\x02';
      }
      local_b = cVar1;
      if (0x30 < *(byte *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                           *(longlong *)_refptr_g_touchesPtr + 0x1b9)) {
        local_b = local_b + '\x01';
      }
    }
    else {
      GripFilter_WeakenCornerClickSupress(&local_b);
    }
  }
  GripFilter_GetRatioTholdLevelExtra(&local_b);
  if (local_b == '\0') {
    local_a = GripFilter_GetClickGripRatio();
  }
  else if (local_b == '\x01') {
    local_a = (ushort)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                               (ulonglong)*(uint *)(_refptr_g_grip + 4) * 0xc + -0x13);
  }
  else if (local_b == '\x02') {
    local_a = (ushort)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
                               (ulonglong)*(uint *)(_refptr_g_grip + 4) * 0xc + -0x12);
  }
  else {
    local_a = 0x10;
  }
  if (0xfe < local_a) {
    local_a = 0xffff;
  }
  return local_a;
}


// ============================================================================
// Function: GripFilter_IsCornerLongPressAllowed @ 6bae25b3
// ============================================================================

undefined8 GripFilter_IsCornerLongPressAllowed(void)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0xa25b3  706  GripFilter_IsCornerLongPressAllowed */
  if (9 < *(ushort *)
           ((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x24e)) {
    if ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214)
        & 0x1000000) != 0) {
      return 0;
    }
    cVar1 = GripFilter_IsNearbyCorner(g_grip);
    if (cVar1 != '\0') {
      return 0;
    }
  }
  if ((*(ushort *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x24e)
       < 5) || ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                          0x1fc) & 0x10000000) == 0)) {
    cVar1 = TSAStatic_IsScreenRotated();
    if ((cVar1 == '\0') &&
       (((cVar1 = GripFilter_Is2HandGame(), cVar1 == '\0' &&
         (cVar1 = TSA_IsTypingScenario(), cVar1 == '\0')) &&
        ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214)
         & 0x10000) != 0)))) {
      return 0;
    }
    uVar2 = 1;
  }
  else {
    uVar2 = 0;
  }
  return uVar2;
}


// ============================================================================
// Function: GripFilter_IsVideoModeLongPressAllowed @ 6bae2720
// ============================================================================

undefined8 GripFilter_IsVideoModeLongPressAllowed(void)

{
  char cVar1;
  
                    /* 0xa2720  777  GripFilter_IsVideoModeLongPressAllowed */
  cVar1 = GripFilter_IsVideoMode();
  if ((cVar1 != '\0') && (cVar1 = GripFilter_IsNearbyCorner(g_grip), cVar1 != '\0')) {
    if ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1fc)
        & 0x8000000) != 0) {
      return 0;
    }
    if (0xe < *(ushort *)
               ((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x228)) {
      return 0;
    }
    if ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214)
        & 1) != 0) {
      return 0;
    }
  }
  return 1;
}


// ============================================================================
// Function: GripFilter_IsScreenRotatedStateLongPressAllowed @ 6bae280c
// ============================================================================

undefined8 GripFilter_IsScreenRotatedStateLongPressAllowed(void)

{
  char cVar1;
  
                    /* 0xa280c  754  GripFilter_IsScreenRotatedStateLongPressAllowed */
  cVar1 = TSAStatic_IsScreenRotated();
  if (((cVar1 != '\0') &&
      (((cVar1 = GripFilter_IsOnDim2(), cVar1 != '\0' ||
        (cVar1 = GripFilter_IsNearbyCorner(g_grip), cVar1 != '\0')) &&
       (cVar1 = GripFilter_Is2HandGame(), cVar1 == '\0')))) &&
     (cVar1 = GripFilter_IsRatioOK(6), cVar1 != '\0')) {
    if (0xe < *(ushort *)
               ((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x228)) {
      return 0;
    }
    if ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214)
        & 1) != 0) {
      return 0;
    }
  }
  return 1;
}


// ============================================================================
// Function: GripFilter_IsAbnormalLiftOff @ 6bae340f
// ============================================================================

undefined8 GripFilter_IsAbnormalLiftOff(void)

{
  undefined8 uVar1;
  
                    /* 0xa340f  697  GripFilter_IsAbnormalLiftOff */
  if ((*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                 (longlong)(int)(uint)DAT_6bc84e21 * 0x478 + 0x5a8) < 0x32) &&
     (0x32 < (int)*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                            (longlong)(int)(uint)DAT_6bc84e21 * 0x478 + 0x5aa) -
             (int)*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                            (longlong)(int)(uint)DAT_6bc84e21 * 0x478 + 0x5a8))) {
    uVar1 = 1;
  }
  else if (*(short *)(_refptr_g_tsaPrmtDynamic + 8) * 3 <
           *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)DAT_6bc84e21 * 0x478 + 0x5a6) * 4) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: GripFilter_IsToCheckAbnormal @ 6bae3513
// ============================================================================

undefined8 GripFilter_IsToCheckAbnormal(void)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0xa3513  769  GripFilter_IsToCheckAbnormal */
  cVar1 = GripFilter_GetCenterLevel(g_grip);
  if (cVar1 == '\0') {
    uVar2 = 0;
  }
  else if (*(byte *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1ac
                    ) < 3) {
    cVar1 = Peak_IsTouchValid(g_grip);
    if (((cVar1 != '\0') &&
        (cVar1 = Peak_IsOnOuterEdgeDim1
                           (*(undefined1 *)
                             ((longlong)(int)(uint)g_grip * 0x168 +
                              *(longlong *)_refptr_g_touchesPtr + 0x23c)), cVar1 == '\0')) &&
       (cVar1 = Peak_IsOnInnerEdgeDim1
                          (*(undefined1 *)
                            ((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr
                            + 0x23c)), cVar1 == '\0')) {
      return 0;
    }
    uVar2 = 1;
  }
  else {
    uVar2 = 0;
  }
  return uVar2;
}


// ============================================================================
// Function: GripFilter_IsHeavyRegionTouched @ 6baea6e5
// ============================================================================

bool GripFilter_IsHeavyRegionTouched(undefined1 param_1)

{
  char cVar1;
  
                    /* 0xaa6e5  718  GripFilter_IsHeavyRegionTouched */
  cVar1 = GripFilter_GetTouchedLevel(param_1);
  return cVar1 == '\x03';
}


// ============================================================================
// Function: GripFilter_IsSignalOK @ 6baed803
// ============================================================================

undefined8 GripFilter_IsSignalOK(byte param_1,uint param_2,char param_3)

{
                    /* 0xad803  758  GripFilter_IsSignalOK */
  if ((param_3 != '\0') &&
     (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c)
        < 2 || (*(ushort *)
                 ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x156)
                < 3)) ||
      ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0)
       & 1) != 0)))) {
    return 0;
  }
  if (param_2 == 8) {
    if ((uint)*(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x17f) == _refptr_g_tsaPrmtFlash->bRows - 1) {
      return 1;
    }
    if ((int)*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x1ba) < ((short)(*_refptr_g_tsaStaticPtr)->wField_0x292 * 2) / 3) {
      return 1;
    }
    return 0;
  }
  if (param_2 < 9) {
    if (param_2 == 1) {
LAB_6baedaaf:
      if ((int)*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x1ba) <
          (*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1bc) * 2) / 3) {
        return 1;
      }
      if ((int)*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x1ba) < (*(short *)_refptr_g_tsaPrmtDynamic * 2) / 5) {
        return 1;
      }
      return 0;
    }
  }
  else {
    if (param_2 == 0x10) {
      if ((int)*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x1ba) < ((short)(*_refptr_g_tsaStaticPtr)->wField_0x292 * 2) / 3) {
        return 1;
      }
      goto LAB_6baedaaf;
    }
    if (param_2 == 0x11) {
      if ((int)*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x1ba) < ((short)(*_refptr_g_tsaStaticPtr)->wField_0x292 * 2) / 5) {
        return 0;
      }
      if ((int)*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x1ba) <
          (int)*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x1bc) / 2) {
        return 0;
      }
      return 1;
    }
  }
  return 1;
}


// ============================================================================
// Function: GripFilter_IsSuppressedByHandGesture @ 6bae2dd7
// ============================================================================

undefined8 GripFilter_IsSuppressedByHandGesture(void)

{
  short sVar1;
  char cVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  
                    /* 0xa2dd7  761  GripFilter_IsSuppressedByHandGesture */
  if (*(short *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x19e) <
      5000) {
    sVar1 = *(short *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x19e);
    iVar3 = Touch_GetTimeOn(g_grip);
    uVar5 = sVar1 - iVar3;
    uVar4 = (int)uVar5 >> 0x1f;
    if ((int)((uVar5 ^ uVar4) - uVar4) < (int)(uint)*(ushort *)&_refptr_g_tsaPrmtFlash[1].field_0x7e
       ) {
      return 1;
    }
    cVar2 = GripFilter_IsCoreTouchChangeInTime(*(undefined2 *)&_refptr_g_tsaPrmtFlash[1].field_0x7e)
    ;
    if (cVar2 != '\0') {
      if ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214
                    ) & 0x8000000) != 0) {
        return 1;
      }
      cVar2 = GripFilter_IsRatioOK(0xb);
      if ((cVar2 != '\0') || (cVar2 = GripFilter_IsSignalOK(g_grip,1,1), cVar2 != '\0')) {
        return 1;
      }
    }
  }
  if ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214) &
      0x8000000) != 0) {
    if ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x218)
        & 2) != 0) {
      return 1;
    }
    cVar2 = GripFilter_IsPad();
    if ((cVar2 != '\0') && (cVar2 = GripFilter_IsRatioOK(0xb), cVar2 != '\0')) {
      return 1;
    }
  }
  return 0;
}


// ============================================================================
// Function: GripFilter_IsSuppressedByCoreTouch @ 6bae2fb7
// ============================================================================

undefined8 GripFilter_IsSuppressedByCoreTouch(void)

{
  char cVar1;
  
                    /* 0xa2fb7  760  GripFilter_IsSuppressedByCoreTouch */
  cVar1 = GripFilter_Is2HandGame();
  if (cVar1 != '\0') {
    return 0;
  }
  cVar1 = GripFilter_IsCoreTouchChangeInTime(*(undefined2 *)&_refptr_g_tsaPrmtFlash[1].field_0x7e);
  if (cVar1 != '\0') {
    if ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214)
        & 0x10) != 0) {
      return 1;
    }
    if (((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214)
         & 0x8000000) != 0) && (cVar1 = GripFilter_IsRatioOK(0xb), cVar1 != '\0')) {
      if ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x218
                    ) & 2) != 0) {
        return 1;
      }
      cVar1 = GripFilter_IsPad();
      if (cVar1 != '\0') {
        return 1;
      }
    }
    cVar1 = TSAStatic_IsScreenRotated();
    if ((((cVar1 != '\0') || (cVar1 = GripFilter_IsPad(), cVar1 != '\0')) &&
        ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214)
         & 0x2000000) != 0)) && (cVar1 = GripFilter_IsRatioOK(0xb), cVar1 != '\0')) {
      return 1;
    }
    if ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214)
        & 0x2000000) != 0) {
      cVar1 = GripFilter_IsSignalOK(g_grip,1,1);
      if ((cVar1 != '\0') && (cVar1 = GripFilter_IsRatioOK(0xb), cVar1 != '\0')) {
        return 1;
      }
      cVar1 = GripFilter_IsCoreTouchChangeInTime(0x32);
      if (((cVar1 != '\0') && (cVar1 = TSA_IsTypingScenario(), cVar1 == '\0')) &&
         ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x218
                    ) & 2) != 0)) {
        return 1;
      }
    }
  }
  cVar1 = GripFilter_IsCoreTouchChangeInTime(100);
  if ((cVar1 != '\0') && (cVar1 = GripFilter_IsRatioOK(0xb), cVar1 != '\0')) {
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: GripFilter_GetAbsThold @ 6bae65e3
// ============================================================================

int GripFilter_GetAbsThold(ushort param_1)

{
                    /* 0xa65e3  667  GripFilter_GetAbsThold */
  return (uint)param_1 + (uint)(param_1 >> 2);
}


// ============================================================================
// Function: GripFilter_IsToCheckStuckSideTouch @ 6bae2113
// ============================================================================

undefined8 GripFilter_IsToCheckStuckSideTouch(void)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0xa2113  770  GripFilter_IsToCheckStuckSideTouch */
  if ((*(ushort *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x25e)
       < 0x1a) &&
     (*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                (longlong)(int)(uint)DAT_6bc84e21 * 0x478 + 0x1ba) / 3 <=
      *(short *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x29e)))
  {
    if ((((*(ushort *)
            ((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x234) <
           0x200) && (cVar1 = GripFilter_Is2HandGame(), cVar1 == '\0')) &&
        (cVar1 = TSA_IsTypingScenario(), cVar1 == '\0')) &&
       (1 < (byte)(*_refptr_g_prevTouchesPtr)->field_0x6)) {
      return 1;
    }
    uVar2 = 0;
  }
  else {
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: GripFilter_IsInDynamicDeadZone @ 6bae59ae
// ============================================================================

undefined8 GripFilter_IsInDynamicDeadZone(ushort param_1)

{
  byte bVar1;
  byte bVar2;
  byte bVar3;
  byte bVar4;
  byte local_9;
  
                    /* 0xa59ae  723  GripFilter_IsInDynamicDeadZone */
  bVar3 = *_refptr_g_grip;
  if (_refptr_g_tsaPrmtFlash[1].field_0x80 != '\0') {
    if ((((_refptr_g_tsaPrmtFlash[1].field_0x80 & 4) != 0) ||
        ((_refptr_g_tsaPrmtFlash[1].field_0x80 & 8) != 0)) &&
       (*(ushort *)((longlong)(int)(uint)bVar3 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22c)
        < param_1)) {
      bVar1 = (byte)((ushort)*(undefined2 *)
                              ((longlong)(int)(uint)bVar3 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x160) >> 8);
      if (bVar1 == 0) {
        bVar4 = 0;
      }
      else {
        bVar4 = bVar1 - 1;
      }
      if ((int)(uint)bVar1 < (int)(_refptr_g_tsaPrmtFlash->bRows - 1)) {
        bVar2 = bVar1 + 1;
      }
      else {
        bVar2 = _refptr_g_tsaPrmtFlash->bRows - 1;
      }
      for (local_9 = 0; local_9 < 4; local_9 = local_9 + 1) {
        if ((((&g_gripFilterDeadZone)
              [(longlong)(int)(uint)local_9 * 0x50 + (longlong)(int)(uint)bVar1] != '\0') ||
            ((&g_gripFilterDeadZone)
             [(longlong)(int)(uint)local_9 * 0x50 + (longlong)(int)(uint)bVar4] != '\0')) ||
           ((&g_gripFilterDeadZone)
            [(longlong)(int)(uint)local_9 * 0x50 + (longlong)(int)(uint)bVar2] != '\0')) {
          return 1;
        }
      }
    }
    if ((((_refptr_g_tsaPrmtFlash[1].field_0x80 & 1) != 0) ||
        ((_refptr_g_tsaPrmtFlash[1].field_0x80 & 2) != 0)) &&
       (*(ushort *)((longlong)(int)(uint)bVar3 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22e)
        < param_1)) {
      bVar3 = (byte)((ushort)*(undefined2 *)
                              ((longlong)(int)(uint)bVar3 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x15e) >> 8);
      if (bVar3 == 0) {
        bVar1 = 0;
      }
      else {
        bVar1 = bVar3 - 1;
      }
      if ((int)(uint)bVar3 < (int)(_refptr_g_tsaPrmtFlash->bCols - 1)) {
        bVar4 = bVar3 + 1;
      }
      else {
        bVar4 = _refptr_g_tsaPrmtFlash->bCols - 1;
      }
      for (local_9 = 0; local_9 < 4; local_9 = local_9 + 1) {
        if ((((&g_gripFilterDeadZone)
              [(longlong)(int)(uint)local_9 * 0x50 + (longlong)(int)(uint)bVar3] != '\0') ||
            ((&g_gripFilterDeadZone)
             [(longlong)(int)(uint)local_9 * 0x50 + (longlong)(int)(uint)bVar1] != '\0')) ||
           ((&g_gripFilterDeadZone)
            [(longlong)(int)(uint)local_9 * 0x50 + (longlong)(int)(uint)bVar4] != '\0')) {
          return 1;
        }
      }
    }
  }
  return 0;
}


// ============================================================================
// Function: GripFilter_IsToApplyMaxDelay @ 6bae4536
// ============================================================================

undefined8 GripFilter_IsToApplyMaxDelay(void)

{
  short sVar1;
  char cVar2;
  short sVar3;
  
                    /* 0xa4536  765  GripFilter_IsToApplyMaxDelay */
  if (((((((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1fc) & 2) == 0) &&
         ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214
                    ) & 8) == 0)) &&
        ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214)
         & 0x800000) == 0)) &&
       ((cVar2 = GripFilter_IsToApplyMaxDelayOnHeavyCorner(), cVar2 == '\0' &&
        (cVar2 = GripFilter_IsRatioOK(4), cVar2 == '\0')))) &&
      ((4999 < *(short *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x19e) &&
       ((cVar2 = GripFilter_IsLongPressAllowed(), cVar2 != '\0' &&
        (cVar2 = GripFilter_IsAbnormalLiftOff(), cVar2 == '\0')))))) &&
     (sVar1 = *(short *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x1ba), sVar3 = GripFilter_GetSafeTouchThold(), sVar3 <= sVar1)) {
    return 0;
  }
  return 1;
}


// ============================================================================
// Function: GripFilter_IsToApplyDelay @ 6bae4695
// ============================================================================

undefined8 GripFilter_IsToApplyDelay(void)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0xa4695  764  GripFilter_IsToApplyDelay */
  cVar1 = GripFilter_IsNearbyCorner(g_grip);
  if (cVar1 == '\0') {
    cVar1 = GripFilter_IsRatioOK(2);
    if (cVar1 == '\0') {
      cVar1 = GripFilter_IsSignalOK(g_grip,0x10,1);
      if (cVar1 == '\0') {
        if (*(ushort *)
             ((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x228) < 0xf
           ) {
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
// Function: GripFilter_IsToBypassDelay @ 6bae4740
// ============================================================================

undefined8 GripFilter_IsToBypassDelay(void)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0xa4740  767  GripFilter_IsToBypassDelay */
  cVar1 = TSA_IsTypingScenario();
  if ((cVar1 == '\0') || (DAT_6bc84e22 != '\0')) {
    if ((DAT_6bc84e28 == 0) || (DAT_6bc84e28 == 1)) {
      uVar2 = 1;
    }
    else {
      uVar2 = 0;
    }
  }
  else {
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: GripFilter_GetDim2Distance @ 6bae5dd6
// ============================================================================

int GripFilter_GetDim2Distance(byte param_1,byte param_2,byte param_3,byte param_4)

{
  int iVar1;
  
                    /* 0xa5dd6  672  GripFilter_GetDim2Distance */
  if (param_2 < param_3) {
    iVar1 = (uint)param_3 - (uint)param_2;
  }
  else if (param_4 < param_1) {
    iVar1 = (uint)param_1 - (uint)param_4;
  }
  else {
    iVar1 = 0;
  }
  return iVar1;
}


// ============================================================================
// Function: GripFilter_GetSideDim2Distance @ 6bae6287
// ============================================================================

undefined4 GripFilter_GetSideDim2Distance(byte param_1,byte param_2)

{
  char cVar1;
  undefined4 uVar2;
  
                    /* 0xa6287  684  GripFilter_GetSideDim2Distance */
  uVar2 = GripFilter_GetDim2Distance
                    (*(undefined1 *)
                      ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x17e),*(undefined1 *)
                              ((longlong)(int)(uint)param_1 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x17f),
                     _refptr_g_touchesSide[(longlong)(int)(uint)param_2 * 0x168 + 0x17e],
                     _refptr_g_touchesSide[(longlong)(int)(uint)param_2 * 0x168 + 0x17f]);
  cVar1 = Touch_IsSideCoreInSameZone(param_1,param_2);
  if ((cVar1 != '\0') || (cVar1 = Touch_IsOff(param_1), cVar1 != '\0')) {
    return 100;
  }
  if ((-0x200 < (int)((uint)*(ushort *)
                             (_refptr_g_touchesSide + (longlong)(int)(uint)param_2 * 0x168 + 0x162)
                     - (uint)*(ushort *)
                              (_refptr_g_touchesGrid + (longlong)(int)(uint)param_1 * 0x168 + 0x162)
                     )) &&
     ((int)((uint)*(ushort *)(_refptr_g_touchesSide + (longlong)(int)(uint)param_2 * 0x168 + 0x162)
           - (uint)*(ushort *)(_refptr_g_touchesGrid + (longlong)(int)(uint)param_1 * 0x168 + 0x162)
           ) < 0x200)) {
    return uVar2;
  }
  return 100;
}


// ============================================================================
// Function: GripFilter_CheckMultiGripOnCore @ 6bae7701
// ============================================================================

void GripFilter_CheckMultiGripOnCore(void)

{
  byte local_9;
  
                    /* 0xa7701  657  GripFilter_CheckMultiGripOnCore */
  for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
    GripFilterCheckMultiGripUnit(local_9);
  }
  return;
}


// ============================================================================
// Function: GripFilter_CheckMultiGripOnSide @ 6bae773c
// ============================================================================

void GripFilter_CheckMultiGripOnSide(void)

{
  char cVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  int iVar4;
  byte local_9;
  
                    /* 0xa773c  658  GripFilter_CheckMultiGripOnSide */
  for (local_9 = 0; local_9 < (byte)*_refptr_g_touchesSide; local_9 = local_9 + 1) {
    cVar1 = Touch_IsSideCoreInSameZone(*_refptr_g_grip,local_9);
    if (cVar1 == '\0') {
      cVar1 = GripFilter_IsTouchIntervalInTime(*_refptr_g_grip,local_9,1,0xfa);
      if (cVar1 != '\0') {
        uVar2 = TSA_GetSide(*(undefined2 *)
                             ((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                              *(longlong *)_refptr_g_touchesPtr + 0x170),
                            *(undefined2 *)
                             ((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                              *(longlong *)_refptr_g_touchesPtr + 0x172));
        uVar3 = TSA_GetSide(*(undefined2 *)
                             (_refptr_g_touchesSide + (longlong)(int)(uint)local_9 * 0x168 + 0x170),
                            *(undefined2 *)
                             (_refptr_g_touchesSide + (longlong)(int)(uint)local_9 * 0x168 + 0x172))
        ;
        iVar4 = GripFilter_GetSideDim2Distance(*_refptr_g_grip,local_9);
        if (iVar4 < 6) {
          *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                    *(longlong *)_refptr_g_touchesPtr + 0x214) =
               *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                         *(longlong *)_refptr_g_touchesPtr + 0x214) | 0x100;
        }
        else {
          cVar1 = GripFilter_IsOnOppositeSide(uVar2,uVar3);
          if (cVar1 != '\0') {
            *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                      *(longlong *)_refptr_g_touchesPtr + 0x214) =
                 *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                           *(longlong *)_refptr_g_touchesPtr + 0x214) | 0x100;
          }
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: GripFilter_CheckCornerMultiGrip @ 6bae7964
// ============================================================================

void GripFilter_CheckCornerMultiGrip(void)

{
  char cVar1;
  byte local_9;
  
                    /* 0xa7964  655  GripFilter_CheckCornerMultiGrip */
  local_9 = 0;
  do {
    if (**(byte **)_refptr_g_touchesPtr <= local_9) {
      return;
    }
    if ((((*_refptr_g_grip != local_9) &&
         ((*(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1f8) & 0x20000000) == 0)) &&
        ((*(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1fc
                   ) & 0x8000000) != 0)) &&
       ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                   *(longlong *)_refptr_g_touchesPtr + 0x1fc) & 0x8000000) != 0)) {
      cVar1 = GripFilter_IsTouchIntervalInTime(*_refptr_g_grip,local_9,0,0xfa);
      if (cVar1 == '\0') {
        cVar1 = Touch_IsFullyLiftOff(*_refptr_g_grip);
        if (cVar1 != '\0') goto LAB_6bae7ac0;
      }
      *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                *(longlong *)_refptr_g_touchesPtr + 0x214) =
           *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                     *(longlong *)_refptr_g_touchesPtr + 0x214) | 2;
    }
LAB_6bae7ac0:
    local_9 = local_9 + 1;
  } while( true );
}


// ============================================================================
// Function: GripFilter_CheckScreenRotatedMultiGrip @ 6bae7b19
// ============================================================================

void GripFilter_CheckScreenRotatedMultiGrip(void)

{
  char cVar1;
  bool bVar2;
  bool bVar3;
  int iVar4;
  byte local_9;
  
                    /* 0xa7b19  659  GripFilter_CheckScreenRotatedMultiGrip */
  cVar1 = GripFilter_IsScreenRotatedMultiGripEnabled();
  if (cVar1 != '\0') {
    for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
      if (*_refptr_g_grip != local_9) {
        if (((*(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x214) & 2) == 0) ||
           (cVar1 = GripFilter_IsOnSameShortSide(*_refptr_g_grip,local_9), cVar1 == '\0')) {
          bVar2 = false;
        }
        else {
          bVar2 = true;
        }
        if ((((*(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x214) & 0x1000000) == 0) ||
            (iVar4 = GripFilter_GetCoreDim1Distance(*_refptr_g_grip,local_9), 5 < iVar4)) ||
           (cVar1 = GripFilter_IsVideoMode(), cVar1 == '\0')) {
          bVar3 = false;
        }
        else {
          bVar3 = true;
        }
        if ((((*(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x1f8) & 0x20000000) == 0) && ((bVar2 || (bVar3)))) &&
           ((cVar1 = GripFilter_IsTouchIntervalInTime(*_refptr_g_grip,local_9,0,0xfa), cVar1 != '\0'
            || (cVar1 = Touch_IsFullyLiftOff(*_refptr_g_grip), cVar1 == '\0')))) {
          *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                    *(longlong *)_refptr_g_touchesPtr + 0x214) =
               *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                         *(longlong *)_refptr_g_touchesPtr + 0x214) | 2;
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: GripFilter_IsFlickAllowed @ 6bae7dbd
// ============================================================================

undefined8 GripFilter_IsFlickAllowed(void)

{
  ushort uVar1;
  char cVar2;
  ushort uVar3;
  undefined8 uVar4;
  
                    /* 0xa7dbd  708  GripFilter_IsFlickAllowed */
  if (((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                  *(longlong *)_refptr_g_touchesPtr + 0x1fc) & 0x10000000) != 0) &&
     ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                 *(longlong *)_refptr_g_touchesPtr + 0x1f0) & 0x4000) != 0)) {
    return 0;
  }
  uVar1 = *(ushort *)
           ((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
           0x22c);
  uVar3 = GripFilter_GetSafeDistToEdgeThold();
  if (((uVar1 < uVar3) && (cVar2 = TSAStatic_IsScreenRotated(), cVar2 == '\0')) &&
     ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                 *(longlong *)_refptr_g_touchesPtr + 0x214) & 0x20000000) != 0)) {
    return 0;
  }
  if ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                 *(longlong *)_refptr_g_touchesPtr + 0x214) & 2) != 0) {
    if (1000 < *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                         *(longlong *)_refptr_g_touchesPtr + 0x158)) {
      return 0;
    }
    if ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                   *(longlong *)_refptr_g_touchesPtr + 0x1fc) & 0x10000000) != 0) {
      return 0;
    }
  }
  cVar2 = GripFilter_IsSideTouch();
  if (cVar2 == '\0') {
    cVar2 = GripFilter_IsInDynamicDeadZone(0x20);
    if (cVar2 == '\0') {
      if (((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                      *(longlong *)_refptr_g_touchesPtr + 0x214) & 0x2000000) == 0) ||
         (*(ushort *)
           ((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
           0x228) < 0xf)) {
        cVar2 = GripFilter_IsVideoMode();
        if (cVar2 != '\0') {
          cVar2 = GripFilter_IsRatioOK(6);
          if ((cVar2 != '\0') &&
             ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                         *(longlong *)_refptr_g_touchesPtr + 0x214) & 1) != 0)) {
            return 0;
          }
          if ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                         *(longlong *)_refptr_g_touchesPtr + 0x214) & 0x8000) != 0) {
            return 0;
          }
          if ((*(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                         *(longlong *)_refptr_g_touchesPtr + 0x214) & 2) != 0) {
            return 0;
          }
        }
        uVar4 = 1;
      }
      else {
        uVar4 = 0;
      }
    }
    else {
      uVar4 = 0;
    }
  }
  else {
    uVar4 = 0;
  }
  return uVar4;
}


// ============================================================================
// Function: GripFilter_IsFlickDetected @ 6bae80e0
// ============================================================================

/* WARNING: Removing unreachable block (ram,0x6bae8137) */
/* WARNING: Removing unreachable block (ram,0x6bae817d) */

bool GripFilter_IsFlickDetected(void)

{
  ushort uVar1;
  uint local_c;
  
                    /* 0xa80e0  709  GripFilter_IsFlickDetected */
  local_c = (uint)(byte)_refptr_g_tsaPrmtFlash[1].field_0x78;
  uVar1 = *(ushort *)
           ((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
           0x228);
  if (local_c < (uint)((int)((uint)uVar1 * 2 + (uint)uVar1) >> 2)) {
    uVar1 = *(ushort *)
             ((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr
             + 0x228);
    local_c = (int)((uint)uVar1 * 2 + (uint)uVar1) >> 2;
  }
  return local_c * local_c <
         (uint)*(ushort *)
                ((longlong)*_refptr_g_prevTouchesPtr +
                (longlong)(int)(uint)(byte)_refptr_g_grip[1] * 0x478 + 0x250);
}


// ============================================================================
// Function: GripFilter_GetSideTouchOnTime @ 6bae6425
// ============================================================================

int GripFilter_GetSideTouchOnTime(byte param_1)

{
                    /* 0xa6425  686  GripFilter_GetSideTouchOnTime */
  return (int)*(short *)((ulonglong)
                         ((uint)(byte)((ushort)*(undefined2 *)
                                                (_refptr_g_touchesSide +
                                                (longlong)(int)(uint)param_1 * 0x168 + 0x162) >> 8)
                          + (uint)(byte)((ushort)*(undefined2 *)
                                                  (_refptr_g_touchesSide +
                                                  (longlong)(int)(uint)param_1 * 0x168 + 0x164) >> 8
                                        ) * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
                        g_tsaBufTimeDown);
}


// ============================================================================
// Function: GripFilter_IsHigherPriorityTouchPad @ 6baeabbe
// ============================================================================

undefined8 GripFilter_IsHigherPriorityTouchPad(byte param_1,byte param_2)

{
  char cVar1;
  
                    /* 0xaabbe  721  GripFilter_IsHigherPriorityTouchPad */
  if (((*_refptr_g_tsaStaticPtr)->nField_0x264 == 1) ||
     ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 3)) {
    cVar1 = GripFilter_IsOnSameSide(param_1,param_2,2);
    if (cVar1 != '\0') {
      if ((*(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1f8) & 0x20000000) != 0) {
        return 1;
      }
      if ((*(ushort *)
            ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x236) <
           *(ushort *)
            ((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x236)) &&
         (cVar1 = GripFilter_IsOnDim2(), cVar1 != '\0')) {
        return 1;
      }
    }
  }
  else {
    cVar1 = GripFilter_IsOnSameSide(param_1,param_2,1);
    if (cVar1 != '\0') {
      if ((*(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1f8) & 0x20000000) != 0) {
        return 1;
      }
      if ((*(ushort *)
            ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x234) <
           *(ushort *)
            ((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x234)) &&
         (cVar1 = GripFilter_IsOnDim2(), cVar1 != '\0')) {
        return 1;
      }
    }
  }
  return 0;
}


// ============================================================================
// Function: GripFilter_IsHigherPriorityTouchNormal @ 6baead61
// ============================================================================

undefined8 GripFilter_IsHigherPriorityTouchNormal(byte param_1,byte param_2)

{
  char cVar1;
  
                    /* 0xaad61  720  GripFilter_IsHigherPriorityTouchNormal */
  cVar1 = TSA_IsTypingScenario();
  if ((cVar1 != '\0') && (_refptr_g_grip[2] == '\0')) {
    return 0;
  }
  cVar1 = TSAStatic_IsScreenRotated();
  if (cVar1 == '\0') {
    cVar1 = HandGesture_Is2HandDetected();
    if (cVar1 != '\0') {
      return 0;
    }
    if ((*(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
        & 0x20000000) != 0) {
      return 1;
    }
    if ((*(ushort *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22c) <
         *(ushort *)
          ((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22c)) &&
       (cVar1 = GripFilter_IsOnDim1(), cVar1 != '\0')) {
      return 1;
    }
  }
  else {
    cVar1 = GripFilter_IsOnSameShortSide(param_1,param_2);
    if (cVar1 != '\0') {
      if ((*(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1f8) & 0x20000000) != 0) {
        return 1;
      }
      if ((*(ushort *)
            ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x236) <
           *(ushort *)
            ((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x236)) &&
         (cVar1 = GripFilter_IsOnDim2(), cVar1 != '\0')) {
        return 1;
      }
    }
  }
  return 0;
}


// ============================================================================
// Function: GripFilter_IsSensorInCornerRegion @ 6baea1f4
// ============================================================================

undefined8 GripFilter_IsSensorInCornerRegion(byte param_1,byte param_2)

{
  undefined8 uVar1;
  
                    /* 0xaa1f4  755  GripFilter_IsSensorInCornerRegion */
  if (((param_1 == 0) || ((uint)param_1 == _refptr_g_tsaPrmtFlash->bCols - 1)) &&
     ((param_2 == 0 ||
      (((param_2 == 1 || ((uint)param_2 == _refptr_g_tsaPrmtFlash->bRows - 2)) ||
       ((uint)param_2 == _refptr_g_tsaPrmtFlash->bRows - 1)))))) {
    uVar1 = 1;
  }
  else if (((param_1 == 1) || ((uint)param_1 == _refptr_g_tsaPrmtFlash->bCols - 2)) &&
          ((param_2 == 0 || ((uint)param_2 == _refptr_g_tsaPrmtFlash->bRows - 1)))) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: GripFilter_IsTouchPosInCornerRegion @ 6baea2b0
// ============================================================================

undefined8 GripFilter_IsTouchPosInCornerRegion(byte param_1)

{
  undefined8 uVar1;
  
                    /* 0xaa2b0  775  GripFilter_IsTouchPosInCornerRegion */
  if (((uint)*(ushort *)
              ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22c) * 2
       < 0x300) &&
     ((uint)*(ushort *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22e) * 2
      < 0x300)) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: GripFilter_GetClickGripRatio @ 6baec860
// ============================================================================

undefined1 GripFilter_GetClickGripRatio(void)

{
  char cVar1;
  
                    /* 0xac860  669  GripFilter_GetClickGripRatio */
  cVar1 = TSAStatic_IsScreenRotated();
  if (((cVar1 == '\0') && (cVar1 = GripFilter_IsOnDim2(), cVar1 != '\0')) &&
     (*(int *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
              (ulonglong)*(uint *)(_refptr_g_grip + 4) * 0xc + -0x18) == 1)) {
    return 0xff;
  }
  return *(undefined1 *)
          ((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap +
          (ulonglong)*(uint *)(_refptr_g_grip + 4) * 0xc + -0x14);
}


// ============================================================================
// Function: GripFilter_GetRatioTholdLevelExtra @ 6baec8e8
// ============================================================================

void GripFilter_GetRatioTholdLevelExtra(byte *param_1)

{
  char cVar1;
  
                    /* 0xac8e8  680  GripFilter_GetRatioTholdLevelExtra */
  cVar1 = GripFilter_IsLongPress();
  if (cVar1 != '\0') {
    if (*param_1 == 0) {
      *param_1 = *param_1 + 1;
    }
    if (((((*param_1 < 2) && (cVar1 = TSAStatic_IsScreenRotated(), cVar1 != '\0')) &&
         (cVar1 = TSA_IsGameScenario(), cVar1 == '\0')) &&
        ((cVar1 = GripFilter_IsOnDim2(), cVar1 != '\0' &&
         (0xe < *(ushort *)
                 ((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                  *(longlong *)_refptr_g_touchesPtr + 0x228))))) &&
       (0x1f < *(byte *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                         *(longlong *)_refptr_g_touchesPtr + 0x267))) {
      *param_1 = 2;
    }
  }
  cVar1 = GripFilter_IsVideoMode();
  if ((cVar1 != '\0') && (*param_1 == 0)) {
    cVar1 = GripFilter_IsSignalOK(*_refptr_g_grip,1,0);
    if ((cVar1 == '\0') && (cVar1 = GripFilter_IsOnDim2(), cVar1 == '\0')) {
      return;
    }
    *param_1 = 1;
  }
  return;
}


// ============================================================================
// Function: GripFilter_IsEnableWeakenCornerClickSupress @ 6baec9fe
// ============================================================================

undefined8 GripFilter_IsEnableWeakenCornerClickSupress(void)

{
  undefined8 uVar1;
  
  if ((_refptr_g_tsaPrmtFlash[1].field_0x81 == '\0') ||
     (299 < *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                      *(longlong *)_refptr_g_touchesPtr + 0x158))) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: GripFilter_WeakenCornerClickSupress @ 6baeca55
// ============================================================================

void GripFilter_WeakenCornerClickSupress(char *param_1)

{
  if ((1 < *(byte *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                     *(longlong *)_refptr_g_touchesPtr + 0x1b0)) &&
     (1 < *(byte *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                    *(longlong *)_refptr_g_touchesPtr + 0x1b1))) {
    *param_1 = *param_1 + '\x01';
  }
  return;
}


// ============================================================================
// Function: GripFilter_GetTouchedLevel @ 6baea6a0
// ============================================================================

void GripFilter_GetTouchedLevel(byte param_1)

{
                    /* 0xaa6a0  689  GripFilter_GetTouchedLevel */
  GripFilter_GetGripLevel
            (*(undefined1 *)
              ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x199));
  return;
}


// ============================================================================
// Function: GripFilter_IsCoreTouchChangeInTime @ 6bae2c51
// ============================================================================

undefined8 GripFilter_IsCoreTouchChangeInTime(ushort param_1)

{
  int iVar1;
  int iVar2;
  undefined8 uVar3;
  
                    /* 0xa2c51  704  GripFilter_IsCoreTouchChangeInTime */
  iVar1 = MAX(*(ulonglong *)
               ((longlong)*_refptr_g_prevTouchesPtr +
               (longlong)(int)(uint)DAT_6bc84e21 * 0x478 + 0x4b8) & 0xffffffff,
              *(ulonglong *)(*(longlong *)_refptr_g_touchesPtr + 0x28) & 0xffffffff);
  iVar2 = MIN(*(ulonglong *)
               ((longlong)*_refptr_g_prevTouchesPtr +
               (longlong)(int)(uint)DAT_6bc84e21 * 0x478 + 0x4b8) & 0xffffffff,
              *(ulonglong *)(*(longlong *)_refptr_g_touchesPtr + 0x28) & 0xffffffff);
  if ((ulonglong)((longlong)iVar1 - (longlong)iVar2) < (ulonglong)param_1) {
    uVar3 = 1;
  }
  else {
    iVar1 = MAX(*(ulonglong *)
                 ((longlong)*_refptr_g_prevTouchesPtr +
                 (longlong)(int)(uint)DAT_6bc84e21 * 0x478 + 0x4b8) & 0xffffffff,
                *(ulonglong *)(*(longlong *)_refptr_g_touchesPtr + 0x30) & 0xffffffff);
    iVar2 = MIN(*(ulonglong *)
                 ((longlong)*_refptr_g_prevTouchesPtr +
                 (longlong)(int)(uint)DAT_6bc84e21 * 0x478 + 0x4b8) & 0xffffffff,
                *(ulonglong *)(*(longlong *)_refptr_g_touchesPtr + 0x30) & 0xffffffff);
    if ((ulonglong)((longlong)iVar1 - (longlong)iVar2) < (ulonglong)param_1) {
      uVar3 = 1;
    }
    else {
      uVar3 = 0;
    }
  }
  return uVar3;
}


// ============================================================================
// Function: GripFilter_IsToApplyMaxDelayOnHeavyCorner @ 6bae440b
// ============================================================================

undefined8 GripFilter_IsToApplyMaxDelayOnHeavyCorner(void)

{
                    /* 0xa440b  766  GripFilter_IsToApplyMaxDelayOnHeavyCorner */
  if ((*(uint *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x218) &
      2) != 0) {
    if (0xe < *(ushort *)
               ((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x228)) {
      return 1;
    }
    if (*(ushort *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22c)
        < 0x181) {
      return 1;
    }
    if (*(ushort *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22e)
        < 0x181) {
      return 1;
    }
    if (0x10 < *(byte *)((longlong)(int)(uint)g_grip * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x1b6)) {
      return 1;
    }
  }
  return 0;
}


// ============================================================================
// Function: GripFilter_GetSafeTouchThold @ 6bae5d10
// ============================================================================

int GripFilter_GetSafeTouchThold(void)

{
                    /* 0xa5d10  682  GripFilter_GetSafeTouchThold */
  return (*(short *)_refptr_g_tsaPrmtDynamic * 2) / 5;
}


// ============================================================================
// Function: GripFilter_IsOnOppositeSide @ 6bae717f
// ============================================================================

undefined8 GripFilter_IsOnOppositeSide(int param_1,int param_2)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0xa717f  743  GripFilter_IsOnOppositeSide */
  if (((param_1 == 4) && (param_2 == 8)) || ((param_1 == 8 && (param_2 == 4)))) {
    uVar2 = 1;
  }
  else {
    cVar1 = TSAStatic_IsScreenRotated();
    if ((cVar1 == '\0') ||
       (((param_1 != 1 || (param_2 != 2)) && ((param_1 != 2 || (param_2 != 1)))))) {
      uVar2 = 0;
    }
    else {
      uVar2 = 1;
    }
  }
  return uVar2;
}


// ============================================================================
// Function: GripFilter_GetCoreDim1Distance @ 6bae5fc9
// ============================================================================

undefined4 GripFilter_GetCoreDim1Distance(byte param_1,byte param_2)

{
  char cVar1;
  undefined4 uVar2;
  
  uVar2 = GripFilter_GetDim2Distance
                    (*(undefined1 *)
                      ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x17c),*(undefined1 *)
                              ((longlong)(int)(uint)param_1 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x17d),
                     *(undefined1 *)
                      ((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x17c),*(undefined1 *)
                              ((longlong)(int)(uint)param_2 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x17d));
  if ((((-0x200 < (int)((uint)*(ushort *)
                               ((longlong)(int)(uint)param_1 * 0x168 +
                                *(longlong *)_refptr_g_touchesPtr + 0x164) -
                       (uint)*(ushort *)
                              ((longlong)(int)(uint)param_2 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x164))) &&
       ((int)((uint)*(ushort *)
                     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x164) -
             (uint)*(ushort *)
                    ((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x164)) < 0x200)) && (cVar1 = Touch_IsOff(param_1), cVar1 == '\0')) &&
     (cVar1 = Touch_IsOff(param_2), cVar1 == '\0')) {
    return uVar2;
  }
  return 100;
}


// ============================================================================
// Function: GripFilter_IsOnSameShortSide @ 6baeaac1
// ============================================================================

undefined8 GripFilter_IsOnSameShortSide(byte param_1,byte param_2)

{
  undefined8 uVar1;
  
                    /* 0xaaac1  744  GripFilter_IsOnSameShortSide */
  if ((*(ushort *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164)
       < *(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x22) >> 1) &&
     (*(ushort *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164)
      < *(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x22) >> 1)) {
    uVar1 = 1;
  }
  else if ((*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x22) >> 1 <
            *(ushort *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164)) &&
          (*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x22) >> 1 <
           *(ushort *)
            ((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164))) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: GripFilter_GetSafeDistToEdgeThold @ 6bae5d57
// ============================================================================

undefined2 GripFilter_GetSafeDistToEdgeThold(void)

{
  undefined2 local_a;
  
                    /* 0xa5d57  681  GripFilter_GetSafeDistToEdgeThold */
  local_a = 0x100;
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) != 0) {
    local_a = 0x1c0;
  }
  return local_a;
}


// ============================================================================
// Function: GripFilter_IsSideTouch @ 6bae5d8e
// ============================================================================

undefined8 GripFilter_IsSideTouch(void)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0xa5d8e  757  GripFilter_IsSideTouch */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) == 0) {
    uVar2 = 0;
  }
  else {
    cVar1 = GripFilter_IsRatioOK(9);
    if (cVar1 == '\0') {
      uVar2 = 0;
    }
    else {
      uVar2 = 1;
    }
  }
  return uVar2;
}


// ============================================================================
// Function: GripFilter_IsOnSameSide @ 6baea8bc
// ============================================================================

undefined8 GripFilter_IsOnSameSide(byte param_1,byte param_2,int param_3)

{
  undefined8 uVar1;
  
                    /* 0xaa8bc  745  GripFilter_IsOnSameSide */
  if (param_3 == 1) {
    if ((*(ushort *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162) <
         *(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x20) >> 1) &&
       (*(ushort *)
         ((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162) <
        *(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x20) >> 1)) {
      uVar1 = 1;
    }
    else if ((*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x20) >> 1 <
              *(ushort *)
               ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162))
            && (*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x20) >> 1 <
                *(ushort *)
                 ((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162))
            ) {
      uVar1 = 1;
    }
    else {
      uVar1 = 0;
    }
  }
  else if ((*(ushort *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164) <
            *(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x22) >> 1) &&
          (*(ushort *)
            ((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164) <
           *(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x22) >> 1)) {
    uVar1 = 1;
  }
  else if ((*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x22) >> 1 <
            *(ushort *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164)) &&
          (*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x22) >> 1 <
           *(ushort *)
            ((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164))) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: GripFilter_GetCoreDistance @ 6bae6171
// ============================================================================

void GripFilter_GetCoreDistance(undefined1 param_1,undefined1 param_2)

{
  if (*(int *)&_refptr_g_tsaPrmtFlash[1].field_0x84 == 5) {
    GripFilter_GetCoreDim1Distance(param_1,param_2);
  }
  else {
    GripFilter_GetCoreDim2Distance(param_1,param_2);
  }
  return;
}


// ============================================================================
// Function: GripFilter_IsMergeHandlerSensorEnabled @ 6bae1820
// ============================================================================

undefined1 GripFilter_IsMergeHandlerSensorEnabled(byte param_1,byte param_2)

{
  char cVar1;
  undefined1 local_9;
  
                    /* 0xa1820  735  GripFilter_IsMergeHandlerSensorEnabled */
  local_9 = 0;
  if (param_2 < 2) {
    cVar1 = GripFilter_IsMergeHandlerEdgeEnabled(0);
    if (cVar1 != '\0') {
      local_9 = 1;
    }
  }
  if ((int)(_refptr_g_tsaPrmtFlash->bRows - 2) <= (int)(uint)param_2) {
    cVar1 = GripFilter_IsMergeHandlerEdgeEnabled(1);
    if (cVar1 != '\0') {
      local_9 = 1;
    }
  }
  if (param_1 < 2) {
    cVar1 = GripFilter_IsMergeHandlerEdgeEnabled(2);
    if (cVar1 != '\0') {
      local_9 = 1;
    }
  }
  if ((int)(_refptr_g_tsaPrmtFlash->bCols - 2) <= (int)(uint)param_1) {
    cVar1 = GripFilter_IsMergeHandlerEdgeEnabled(3);
    if (cVar1 != '\0') {
      local_9 = 1;
    }
  }
  return local_9;
}


