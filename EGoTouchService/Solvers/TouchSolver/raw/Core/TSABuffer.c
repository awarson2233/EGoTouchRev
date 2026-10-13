// ============================================================================
// Function: TSABuffer_PreProcess @ 6ba8f37f
// ============================================================================

void TSABuffer_PreProcess(void)

{
  uint uVar1;
  char cVar2;
  errno_t eVar3;
  int iVar4;
  uint local_c;
  
                    /* 0x4f37f  1483  TSABuffer_PreProcess */
  local_c = (uint)_refptr_g_tsaPrmtFlash->wBufElementCount;
  while (uVar1 = local_c - 1, local_c != 0) {
    local_c = uVar1;
    if (*(char *)((ulonglong)uVar1 + (longlong)g_tsaBufTzZone) == '\0') {
      TSABuffer_TzIsNotMergedHandle(uVar1);
    }
    else {
      TSABuffer_TzIsMergedHandle(uVar1);
    }
  }
  eVar3 = memcpy_s(g_tsaBufTzZonePrev,(ulonglong)g_gridBufSize,g_tsaBufTzZone,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount);
  if (eVar3 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_Buffer.c",
                      0x2bf,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  cVar2 = SafeBaseline_IsSignalStable();
  if (cVar2 == '\0') {
    iVar4 = memset_s(g_tsaBufSilentTouchNum,g_gridBufSize,0,_refptr_g_tsaPrmtFlash->wBufElementCount
                    );
    if (iVar4 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_Buffer.c",
                        0x2c4,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
  }
  TSABuffer_TZIDProcess();
  return;
}


// ============================================================================
// Function: TSABuffer_PostProcess @ 6ba8f4c1
// ============================================================================

void TSABuffer_PostProcess(void)

{
  errno_t eVar1;
  
                    /* 0x4f4c1  1482  TSABuffer_PostProcess */
  eVar1 = memcpy_s(g_tsaBufTzPrev,(ulonglong)g_gridBufSize,g_tsaBufTz,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount);
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/App/TSA_Buffer.c",
                      0x2d0,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: TSABuffer_PrevFrameProcess @ 6ba8f540
// ============================================================================

void TSABuffer_PrevFrameProcess(void)

{
                    /* 0x4f540  1484  TSABuffer_PrevFrameProcess */
  TSABuffer_TimeRptProcess();
  return;
}


// ============================================================================
// Function: TSABuffer_TZIDProcess @ 6ba8e7ac
// ============================================================================

void TSABuffer_TZIDProcess(void)

{
  ushort uVar1;
  char cVar2;
  longlong lVar3;
  undefined8 *puVar4;
  undefined8 local_128 [33];
  byte local_20;
  byte local_1f;
  byte local_1e;
  byte local_1d;
  ushort local_1c;
  byte local_19;
  
  puVar4 = local_128;
  for (lVar3 = 0x20; lVar3 != 0; lVar3 = lVar3 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  for (local_19 = 0; local_19 < **(byte **)_refptr_g_touchesPtr; local_19 = local_19 + 1) {
    cVar2 = Touch_IsOff(local_19);
    if (cVar2 == '\0') {
      local_1d = (byte)((ushort)*(undefined2 *)
                                 ((longlong)(int)(uint)local_19 * 0x168 +
                                  *(longlong *)_refptr_g_touchesPtr + 0x162) >> 8);
      local_1e = (byte)((ushort)*(undefined2 *)
                                 ((longlong)(int)(uint)local_19 * 0x168 +
                                  *(longlong *)_refptr_g_touchesPtr + 0x164) >> 8);
      local_1f = *(byte *)((ulonglong)
                           ((uint)local_1d + (uint)local_1e * (uint)_refptr_g_tsaPrmtFlash->bCols &
                           0xffff) + g_tsaBufSubTZ);
      *(undefined1 *)((longlong)local_128 + (longlong)(int)(uint)local_1f) =
           *(undefined1 *)
            ((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x154);
    }
  }
  local_128[0]._0_1_ = 0;
  local_1c = _refptr_g_tsaPrmtFlash->wBufElementCount;
  while( true ) {
    uVar1 = local_1c;
    local_1c = local_1c - 1;
    if (uVar1 == 0) break;
    local_20 = *(byte *)((ulonglong)local_1c + g_tsaBufSubTZ);
    *(undefined1 *)(g_tsaBufTzID + (ulonglong)local_1c) =
         *(undefined1 *)
          ((longlong)local_128 + (longlong)(int)(uint)*(byte *)((ulonglong)local_1c + g_tsaBufSubTZ)
          );
  }
  return;
}


// ============================================================================
// Function: TSABuffer_TzIsMergedHandle @ 6ba8ec68
// ============================================================================

void TSABuffer_TzIsMergedHandle(uint param_1)

{
  undefined2 *puVar1;
  undefined2 uVar2;
  
  if (*(char *)((ulonglong)param_1 + g_tsaBufTzZonePrev) == '\0') {
    *(undefined2 *)(g_tsaBufTimeDown + (ulonglong)param_1 * 2) = 0;
    *(undefined2 *)((ulonglong)param_1 * 2 + g_tsaBufMaxSigDown) =
         *(undefined2 *)(g_tsaBufDif + (ulonglong)param_1 * 2);
    if ((*_refptr_g_prevTouchesPtr)->bPrevTouchCount < **(byte **)_refptr_g_touchesPtr) {
      *(char *)((ulonglong)param_1 + g_tsaBufSilentTouchNum) =
           *(char *)((ulonglong)param_1 + g_tsaBufSilentTouchNum) + '\x01';
    }
  }
  else {
    puVar1 = (undefined2 *)(g_tsaBufTimeDown + (ulonglong)param_1 * 2);
    uVar2 = TSABuffer_GetNextTime((int)*(short *)(g_tsaBufTimeDown + (ulonglong)param_1 * 2));
    *puVar1 = uVar2;
    if (*(short *)(g_tsaBufMaxSigDown + (ulonglong)param_1 * 2) <
        *(short *)(g_tsaBufDif + (ulonglong)param_1 * 2)) {
      *(undefined2 *)((ulonglong)param_1 * 2 + g_tsaBufMaxSigDown) =
           *(undefined2 *)(g_tsaBufDif + (ulonglong)param_1 * 2);
    }
  }
  return;
}


// ============================================================================
// Function: TSABuffer_TzIsNotMergedHandle @ 6ba8ed8c
// ============================================================================

void TSABuffer_TzIsNotMergedHandle(uint param_1)

{
  undefined2 *puVar1;
  undefined2 uVar2;
  
  if (*(char *)((ulonglong)param_1 + g_tsaBufTzZonePrev) == '\0') {
    if (*(short *)(g_tsaBufTimeOff + (ulonglong)param_1 * 2) != 0x7fff) {
      puVar1 = (undefined2 *)(g_tsaBufTimeOff + (ulonglong)param_1 * 2);
      uVar2 = TSABuffer_GetNextTime((int)*(short *)(g_tsaBufTimeOff + (ulonglong)param_1 * 2));
      *puVar1 = uVar2;
    }
    if (*(short *)(g_tsaBufDif + (ulonglong)param_1 * 2) <
        *(short *)(g_tsaBufMinSigOff + (ulonglong)param_1 * 2)) {
      *(undefined2 *)((ulonglong)param_1 * 2 + g_tsaBufMinSigOff) =
           *(undefined2 *)(g_tsaBufDif + (ulonglong)param_1 * 2);
    }
  }
  else {
    *(short *)((ulonglong)param_1 * 2 + g_tsaBufTimeOff) =
         (*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
    *(undefined2 *)((ulonglong)param_1 * 2 + g_tsaBufMinSigOff) =
         *(undefined2 *)(g_tsaBufDif + (ulonglong)param_1 * 2);
  }
  return;
}


// ============================================================================
// Function: TSABuffer_TimeRptProcess @ 6ba8e9b0
// ============================================================================

void TSABuffer_TimeRptProcess(void)

{
  byte *pbVar1;
  char cVar2;
  longlong lVar3;
  char *pcVar4;
  char local_148 [264];
  short local_40;
  short local_3e;
  byte local_3b;
  byte local_3a;
  byte local_39;
  int local_38;
  ushort local_32;
  byte *local_30;
  short *local_28;
  byte local_19;
  
  pcVar4 = local_148;
  for (lVar3 = 0x20; lVar3 != 0; lVar3 = lVar3 + -1) {
    pcVar4[0] = '\0';
    pcVar4[1] = '\0';
    pcVar4[2] = '\0';
    pcVar4[3] = '\0';
    pcVar4[4] = '\0';
    pcVar4[5] = '\0';
    pcVar4[6] = '\0';
    pcVar4[7] = '\0';
    pcVar4 = pcVar4 + 8;
  }
  for (local_19 = 0; local_19 < (*_refptr_g_prevTouchesPtr)->bPrevTouchCount;
      local_19 = local_19 + 1) {
    cVar2 = Touch_IsOffEvent(*(undefined4 *)
                              ((longlong)*_refptr_g_prevTouchesPtr +
                              (longlong)(int)(uint)local_19 * 0x478 + 0x21c));
    if (cVar2 == '\0') {
      local_39 = *(byte *)((longlong)
                           (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                               (longlong)(int)(uint)local_19 * 0x478 + 0x23c) * 0xc
                           + *(longlong *)_refptr_g_prevPeaksPtr + 0xe);
      local_3a = *(byte *)((longlong)
                           (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                               (longlong)(int)(uint)local_19 * 0x478 + 0x23c) * 0xc
                           + *(longlong *)_refptr_g_prevPeaksPtr + 0xf);
      local_3b = g_tsaBufTzZone
                 [(uint)local_39 + (uint)local_3a * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff];
      if (local_3b != 0) {
        local_148[(int)(uint)local_3b] =
             (*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)local_19 * 0x478 + 0x1f8) & 0x20000000) != 0;
      }
    }
  }
  local_28 = g_tsaBufTimeRpt;
  local_30 = g_tsaBufTzZone;
  local_32 = _refptr_g_tsaPrmtFlash->wBufElementCount;
  local_3e = (*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
  do {
    pbVar1 = local_30;
    local_30 = local_30 + 1;
    if (local_148[(int)(uint)*pbVar1] == '\0') {
      local_40 = *local_28;
      local_38 = (int)local_3e + (int)*local_28;
      if (local_38 < 0x7fff) {
        if (local_38 < -0x8000) {
          local_38 = -0x8000;
        }
      }
      else {
        local_38 = 0x7fff;
      }
      *local_28 = (short)local_38;
    }
    else {
      *local_28 = 0;
    }
    local_28 = local_28 + 1;
    local_32 = local_32 - 1;
  } while (local_32 != 0);
  return;
}


// ============================================================================
// Function: TSABuffer_GripStageProcess @ 6ba8ef57
// ============================================================================

void TSABuffer_GripStageProcess(void)

{
  undefined2 *puVar1;
  char cVar2;
  undefined2 uVar3;
  longlong lVar4;
  undefined8 *puVar5;
  char *pcVar6;
  char local_228 [4];
  undefined2 uStack_224;
  char acStack_222 [250];
  undefined8 local_128;
  byte local_1d;
  ushort local_1c;
  byte local_19;
  
                    /* 0x4ef57  1478  TSABuffer_GripStageProcess */
  TSABuffer_GripTimeProcess();
  puVar5 = &local_128;
  for (lVar4 = 0x1f; lVar4 != 0; lVar4 = lVar4 + -1) {
    *puVar5 = 0;
    puVar5 = puVar5 + 1;
  }
  *(undefined4 *)puVar5 = 0;
  *(undefined2 *)((longlong)puVar5 + 4) = 0;
  *(undefined1 *)((longlong)puVar5 + 6) = 0;
  pcVar6 = local_228;
  for (lVar4 = 0x1f; lVar4 != 0; lVar4 = lVar4 + -1) {
    pcVar6[0] = '\0';
    pcVar6[1] = '\0';
    pcVar6[2] = '\0';
    pcVar6[3] = '\0';
    pcVar6[4] = '\0';
    pcVar6[5] = '\0';
    pcVar6[6] = '\0';
    pcVar6[7] = '\0';
    pcVar6 = pcVar6 + 8;
  }
  pcVar6[0] = '\0';
  pcVar6[1] = '\0';
  pcVar6[2] = '\0';
  pcVar6[3] = '\0';
  pcVar6[4] = '\0';
  pcVar6[5] = '\0';
  pcVar6[6] = '\0';
  for (local_19 = 0; local_19 < **(byte **)_refptr_g_touchesPtr; local_19 = local_19 + 1) {
    cVar2 = Touch_IsOff(local_19);
    if (cVar2 == '\0') {
      local_1d = *(byte *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x28c);
      if (*(int *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x210
                  ) == 1) {
        *(undefined1 *)((longlong)&local_128 + (longlong)(int)(uint)local_1d) = 1;
      }
      if (((*(uint *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x214) & 0x200) != 0) &&
         (*(short *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x156) == 1)) {
        *(undefined1 *)((longlong)&local_128 + (longlong)(int)(uint)local_1d) = 1;
      }
      if ((*(ushort *)
            ((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x234) <
           0x80) ||
         (*(ushort *)
           ((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x236) <
          0x80)) {
        *(undefined1 *)((longlong)&local_128 + (longlong)(int)(uint)local_1d) = 1;
      }
      if ((*(ushort *)
            ((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22c) <
           0x20) ||
         (*(ushort *)
           ((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22e) <
          0x20)) {
        *(undefined1 *)((longlong)&local_128 + (longlong)(int)(uint)local_1d) = 1;
      }
      local_228[(int)(uint)local_1d] =
           (*(uint *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x218) & 1) != 0;
    }
  }
  for (local_1c = 0; local_1c < _refptr_g_tsaPrmtFlash->wBufElementCount; local_1c = local_1c + 1) {
    local_1d = *(byte *)((ulonglong)local_1c + g_tsaBufTzZone);
    if (local_1d == 0) {
      if (*(char *)((ulonglong)local_1c + g_tsaBufGripStage) != '\0') {
        *(undefined2 *)(g_tsaBufGripTimeOff + (ulonglong)local_1c * 2) = 0;
      }
      *(undefined1 *)((ulonglong)local_1c + g_tsaBufGripStage) = 0;
      puVar1 = (undefined2 *)(g_tsaBufGripTimeOff + (ulonglong)local_1c * 2);
      uVar3 = TSABuffer_GetNextTime((int)*(short *)(g_tsaBufGripTimeOff + (ulonglong)local_1c * 2));
      *puVar1 = uVar3;
    }
    else if ((local_228[(int)(uint)local_1d] == '\0') &&
            ((int)(uint)*(ushort *)&_refptr_g_tsaPrmtFlash[1].field_0x7a <
             (int)*(short *)(g_tsaBufGripTime + (ulonglong)local_1c * 2))) {
      *(undefined1 *)(g_tsaBufGripStage + (ulonglong)local_1c) =
           *(undefined1 *)((longlong)&local_128 + (longlong)(int)(uint)local_1d);
    }
    if (*(short *)(g_tsaBufTimeRpt + (ulonglong)local_1c * 2) == 0) {
      *(undefined2 *)(g_tsaBufGripTimeOff + (ulonglong)local_1c * 2) = 0x7fff;
    }
  }
  return;
}


// ============================================================================
// Function: TSABuffer_GetNextTime @ 6ba8e95e
// ============================================================================

int TSABuffer_GetNextTime(int param_1)

{
  int local_c;
  
  local_c = param_1 + (*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
  if (local_c < 0x8000) {
    if (local_c < -0x8000) {
      local_c = -0x8000;
    }
  }
  else {
    local_c = 0x7fff;
  }
  return local_c;
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
// Function: TSABuffer_GripTimeProcess @ 6ba8eea1
// ============================================================================

void TSABuffer_GripTimeProcess(void)

{
  undefined2 *puVar1;
  short sVar2;
  short sVar3;
  undefined2 uVar4;
  ushort local_1a;
  
                    /* 0x4eea1  1479  TSABuffer_GripTimeProcess */
  sVar2 = GripFilter_GetTrackingThold();
  for (local_1a = 0; local_1a < _refptr_g_tsaPrmtFlash->wBufElementCount; local_1a = local_1a + 1) {
    if (sVar2 < *(short *)(g_tsaBufDif + (ulonglong)local_1a * 2)) {
      puVar1 = (undefined2 *)(g_tsaBufGripTime + (ulonglong)local_1a * 2);
      sVar3 = TSABuffer_GetNextTime((int)*(short *)(g_tsaBufGripTime + (ulonglong)local_1a * 2));
      uVar4 = _text(0,(int)sVar3);
      *puVar1 = uVar4;
    }
    else {
      *(undefined2 *)(g_tsaBufGripTime + (ulonglong)local_1a * 2) = 0;
    }
  }
  return;
}


