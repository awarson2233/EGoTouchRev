#include "TSACoreInternal.h"

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
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x128]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x130];
  ushort uVar1;
  char cVar2;
  longlong lVar3;
  undefined8 *puVar4;
  
  puVar4 = ((undefined8 *)(_stk + 0x8));
  for (lVar3 = 0x20; lVar3 != 0; lVar3 = lVar3 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  for ((*(byte *)(_stk + 0x117)) = 0; (*(byte *)(_stk + 0x117)) < **(byte **)_refptr_g_touchesPtr; (*(byte *)(_stk + 0x117)) = (*(byte *)(_stk + 0x117)) + 1) {
    cVar2 = Touch_IsOff((*(byte *)(_stk + 0x117)));
    if (cVar2 == '\0') {
      (*(byte *)(_stk + 0x113)) = (byte)((ushort)*(undefined2 *)
                                 ((longlong)(int)(uint)(*(byte *)(_stk + 0x117)) * 0x168 +
                                  *(longlong *)_refptr_g_touchesPtr + 0x162) >> 8);
      (*(byte *)(_stk + 0x112)) = (byte)((ushort)*(undefined2 *)
                                 ((longlong)(int)(uint)(*(byte *)(_stk + 0x117)) * 0x168 +
                                  *(longlong *)_refptr_g_touchesPtr + 0x164) >> 8);
      (*(byte *)(_stk + 0x111)) = *(byte *)((ulonglong)
                           ((uint)(*(byte *)(_stk + 0x113)) + (uint)(*(byte *)(_stk + 0x112)) * (uint)_refptr_g_tsaPrmtFlash->bCols &
                           0xffff) + g_tsaBufSubTZ);
      *(undefined1 *)((longlong)((undefined8 *)(_stk + 0x8)) + (longlong)(int)(uint)(*(byte *)(_stk + 0x111))) =
           *(undefined1 *)
            ((longlong)(int)(uint)(*(byte *)(_stk + 0x117)) * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x154);
    }
  }
  ((char*)((undefined8 *)(_stk + 0x8)))[0] = 0;
  (*(ushort *)(_stk + 0x114)) = _refptr_g_tsaPrmtFlash->wBufElementCount;
  while( true ) {
    uVar1 = (*(ushort *)(_stk + 0x114));
    (*(ushort *)(_stk + 0x114)) = (*(ushort *)(_stk + 0x114)) - 1;
    if (uVar1 == 0) break;
    (*(byte *)(_stk + 0x110)) = *(byte *)((ulonglong)(*(ushort *)(_stk + 0x114)) + g_tsaBufSubTZ);
    *(undefined1 *)(g_tsaBufTzID + (ulonglong)(*(ushort *)(_stk + 0x114))) =
         *(undefined1 *)
          ((longlong)((undefined8 *)(_stk + 0x8)) + (longlong)(int)(uint)*(byte *)((ulonglong)(*(ushort *)(_stk + 0x114)) + g_tsaBufSubTZ)
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
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x148]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x150];
  byte *pbVar1;
  char cVar2;
  longlong lVar3;
  char *pcVar4;
  
  pcVar4 = ((char *)(_stk + 0x8));
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
  for ((*(byte *)(_stk + 0x137)) = 0; (*(byte *)(_stk + 0x137)) < (*_refptr_g_prevTouchesPtr)->bPrevTouchCount;
      (*(byte *)(_stk + 0x137)) = (*(byte *)(_stk + 0x137)) + 1) {
    cVar2 = Touch_IsOffEvent(*(undefined4 *)
                              ((longlong)*_refptr_g_prevTouchesPtr +
                              (longlong)(int)(uint)(*(byte *)(_stk + 0x137)) * 0x478 + 0x21c));
    if (cVar2 == '\0') {
      (*(byte *)(_stk + 0x117)) = *(byte *)((longlong)
                           (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                               (longlong)(int)(uint)(*(byte *)(_stk + 0x137)) * 0x478 + 0x23c) * 0xc
                           + *(longlong *)_refptr_g_prevPeaksPtr + 0xe);
      (*(byte *)(_stk + 0x116)) = *(byte *)((longlong)
                           (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                               (longlong)(int)(uint)(*(byte *)(_stk + 0x137)) * 0x478 + 0x23c) * 0xc
                           + *(longlong *)_refptr_g_prevPeaksPtr + 0xf);
      (*(byte *)(_stk + 0x115)) = g_tsaBufTzZone
                 [(uint)(*(byte *)(_stk + 0x117)) + (uint)(*(byte *)(_stk + 0x116)) * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff];
      if ((*(byte *)(_stk + 0x115)) != 0) {
        ((char *)(_stk + 0x8))[(int)(uint)(*(byte *)(_stk + 0x115))] =
             (*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)(*(byte *)(_stk + 0x137)) * 0x478 + 0x1f8) & 0x20000000) != 0;
      }
    }
  }
  (*(short * *)(_stk + 0x128)) = g_tsaBufTimeRpt;
  (*(byte * *)(_stk + 0x120)) = g_tsaBufTzZone;
  (*(ushort *)(_stk + 0x11e)) = _refptr_g_tsaPrmtFlash->wBufElementCount;
  (*(short *)(_stk + 0x112)) = (*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
  do {
    pbVar1 = (*(byte * *)(_stk + 0x120));
    (*(byte * *)(_stk + 0x120)) = (*(byte * *)(_stk + 0x120)) + 1;
    if (((char *)(_stk + 0x8))[(int)(uint)*pbVar1] == '\0') {
      (*(short *)(_stk + 0x110)) = *(*(short * *)(_stk + 0x128));
      (*(int *)(_stk + 0x118)) = (int)(*(short *)(_stk + 0x112)) + (int)*(*(short * *)(_stk + 0x128));
      if ((*(int *)(_stk + 0x118)) < 0x7fff) {
        if ((*(int *)(_stk + 0x118)) < -0x8000) {
          (*(int *)(_stk + 0x118)) = -0x8000;
        }
      }
      else {
        (*(int *)(_stk + 0x118)) = 0x7fff;
      }
      *(*(short * *)(_stk + 0x128)) = (short)(*(int *)(_stk + 0x118));
    }
    else {
      *(*(short * *)(_stk + 0x128)) = 0;
    }
    (*(short * *)(_stk + 0x128)) = (*(short * *)(_stk + 0x128)) + 1;
    (*(ushort *)(_stk + 0x11e)) = (*(ushort *)(_stk + 0x11e)) - 1;
  } while ((*(ushort *)(_stk + 0x11e)) != 0);
  return;
}


// ============================================================================
// Function: TSABuffer_GripStageProcess @ 6ba8ef57
// ============================================================================

void TSABuffer_GripStageProcess(void)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x228]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x230];
  undefined2 *puVar1;
  char cVar2;
  undefined2 uVar3;
  longlong lVar4;
  undefined8 *puVar5;
  char *pcVar6;
  
                    /* 0x4ef57  1478  TSABuffer_GripStageProcess */
  TSABuffer_GripTimeProcess();
  puVar5 = &(*(undefined8 *)(_stk + 0x108));
  for (lVar4 = 0x1f; lVar4 != 0; lVar4 = lVar4 + -1) {
    *puVar5 = 0;
    puVar5 = puVar5 + 1;
  }
  *(undefined4 *)puVar5 = 0;
  *(undefined2 *)((longlong)puVar5 + 4) = 0;
  *(undefined1 *)((longlong)puVar5 + 6) = 0;
  pcVar6 = ((char *)(_stk + 0x8));
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
  for ((*(byte *)(_stk + 0x217)) = 0; (*(byte *)(_stk + 0x217)) < **(byte **)_refptr_g_touchesPtr; (*(byte *)(_stk + 0x217)) = (*(byte *)(_stk + 0x217)) + 1) {
    cVar2 = Touch_IsOff((*(byte *)(_stk + 0x217)));
    if (cVar2 == '\0') {
      (*(byte *)(_stk + 0x213)) = *(byte *)((longlong)(int)(uint)(*(byte *)(_stk + 0x217)) * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x28c);
      if (*(int *)((longlong)(int)(uint)(*(byte *)(_stk + 0x217)) * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x210
                  ) == 1) {
        *(undefined1 *)((longlong)&(*(undefined8 *)(_stk + 0x108)) + (longlong)(int)(uint)(*(byte *)(_stk + 0x213))) = 1;
      }
      if (((*(uint *)((longlong)(int)(uint)(*(byte *)(_stk + 0x217)) * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x214) & 0x200) != 0) &&
         (*(short *)((longlong)(int)(uint)(*(byte *)(_stk + 0x217)) * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x156) == 1)) {
        *(undefined1 *)((longlong)&(*(undefined8 *)(_stk + 0x108)) + (longlong)(int)(uint)(*(byte *)(_stk + 0x213))) = 1;
      }
      if ((*(ushort *)
            ((longlong)(int)(uint)(*(byte *)(_stk + 0x217)) * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x234) <
           0x80) ||
         (*(ushort *)
           ((longlong)(int)(uint)(*(byte *)(_stk + 0x217)) * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x236) <
          0x80)) {
        *(undefined1 *)((longlong)&(*(undefined8 *)(_stk + 0x108)) + (longlong)(int)(uint)(*(byte *)(_stk + 0x213))) = 1;
      }
      if ((*(ushort *)
            ((longlong)(int)(uint)(*(byte *)(_stk + 0x217)) * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22c) <
           0x20) ||
         (*(ushort *)
           ((longlong)(int)(uint)(*(byte *)(_stk + 0x217)) * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x22e) <
          0x20)) {
        *(undefined1 *)((longlong)&(*(undefined8 *)(_stk + 0x108)) + (longlong)(int)(uint)(*(byte *)(_stk + 0x213))) = 1;
      }
      ((char *)(_stk + 0x8))[(int)(uint)(*(byte *)(_stk + 0x213))] =
           (*(uint *)((longlong)(int)(uint)(*(byte *)(_stk + 0x217)) * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x218) & 1) != 0;
    }
  }
  for ((*(ushort *)(_stk + 0x214)) = 0; (*(ushort *)(_stk + 0x214)) < _refptr_g_tsaPrmtFlash->wBufElementCount; (*(ushort *)(_stk + 0x214)) = (*(ushort *)(_stk + 0x214)) + 1) {
    (*(byte *)(_stk + 0x213)) = *(byte *)((ulonglong)(*(ushort *)(_stk + 0x214)) + g_tsaBufTzZone);
    if ((*(byte *)(_stk + 0x213)) == 0) {
      if (*(char *)((ulonglong)(*(ushort *)(_stk + 0x214)) + g_tsaBufGripStage) != '\0') {
        *(undefined2 *)(g_tsaBufGripTimeOff + (ulonglong)(*(ushort *)(_stk + 0x214)) * 2) = 0;
      }
      *(undefined1 *)((ulonglong)(*(ushort *)(_stk + 0x214)) + g_tsaBufGripStage) = 0;
      puVar1 = (undefined2 *)(g_tsaBufGripTimeOff + (ulonglong)(*(ushort *)(_stk + 0x214)) * 2);
      uVar3 = TSABuffer_GetNextTime((int)*(short *)(g_tsaBufGripTimeOff + (ulonglong)(*(ushort *)(_stk + 0x214)) * 2));
      *puVar1 = uVar3;
    }
    else if ((((char *)(_stk + 0x8))[(int)(uint)(*(byte *)(_stk + 0x213))] == '\0') &&
            ((int)(uint)*(ushort *)&_refptr_g_tsaPrmtFlash[1].field_0x7a <
             (int)*(short *)(g_tsaBufGripTime + (ulonglong)(*(ushort *)(_stk + 0x214)) * 2))) {
      *(undefined1 *)(g_tsaBufGripStage + (ulonglong)(*(ushort *)(_stk + 0x214))) =
           *(undefined1 *)((longlong)&(*(undefined8 *)(_stk + 0x108)) + (longlong)(int)(uint)(*(byte *)(_stk + 0x213)));
    }
    if (*(short *)(g_tsaBufTimeRpt + (ulonglong)(*(ushort *)(_stk + 0x214)) * 2) == 0) {
      *(undefined2 *)(g_tsaBufGripTimeOff + (ulonglong)(*(ushort *)(_stk + 0x214)) * 2) = 0x7fff;
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
      uVar4 = MAX(0,(int)sVar3);
      *puVar1 = uVar4;
    }
    else {
      *(undefined2 *)(g_tsaBufGripTime + (ulonglong)local_1a * 2) = 0;
    }
  }
  return;
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
// Function: TSAMem_Free @ 6baa0608
// ============================================================================

void TSAMem_Free(void)

{
  RFNoise_Free();
  HandGesture_Free();
  ToeSynaBl_Free();
  HardwareAnalyzer_Free();
  SafeBaseline_Free();
  BadChannel_Free();
  BaliSensorBroken_Free();
  TSAPrpt_Free();
  SideTouch_Free();
  TSABuffer_Free();
  TPSensor_Free();
  CMF_Free();
  Exception_Free();
  TZ_Free();
  return;
}


// ============================================================================
// Function: TSAMem_Malloc @ 6baa04f0
// ============================================================================

undefined8 TSAMem_Malloc(void)

{
  char cVar1;
  undefined8 uVar2;
  
  cVar1 = RFNoise_Malloc();
  if (cVar1 == '\0') {
    uVar2 = 0;
  }
  else {
    cVar1 = HandGesture_Malloc();
    if (cVar1 == '\0') {
      uVar2 = 0;
    }
    else {
      cVar1 = ToeSynaBl_Malloc();
      if (cVar1 == '\0') {
        uVar2 = 0;
      }
      else {
        cVar1 = HardwareAnalyzer_Malloc();
        if (cVar1 == '\0') {
          uVar2 = 0;
        }
        else {
          cVar1 = SafeBaseline_Malloc();
          if (cVar1 == '\0') {
            uVar2 = 0;
          }
          else {
            cVar1 = BadChannel_Malloc();
            if (cVar1 == '\0') {
              uVar2 = 0;
            }
            else {
              cVar1 = BaliSensorBroken_Malloc();
              if (cVar1 == '\0') {
                uVar2 = 0;
              }
              else {
                cVar1 = TSAPrpt_Malloc();
                if (cVar1 == '\0') {
                  uVar2 = 0;
                }
                else {
                  cVar1 = SideTouch_Malloc();
                  if (cVar1 == '\0') {
                    uVar2 = 0;
                  }
                  else {
                    cVar1 = TSABuffer_Malloc();
                    if (cVar1 == '\0') {
                      uVar2 = 0;
                    }
                    else {
                      cVar1 = TPSensor_Malloc();
                      if (cVar1 == '\0') {
                        uVar2 = 0;
                      }
                      else {
                        cVar1 = CMF_Malloc();
                        if (cVar1 == '\0') {
                          uVar2 = 0;
                        }
                        else {
                          cVar1 = Exception_Malloc();
                          if (cVar1 == '\0') {
                            uVar2 = 0;
                          }
                          else {
                            cVar1 = TZ_Malloc();
                            if (cVar1 == '\0') {
                              uVar2 = 0;
                            }
                            else {
                              cVar1 = AFE_Malloc();
                              if (cVar1 == '\0') {
                                uVar2 = 0;
                              }
                              else {
                                uVar2 = 1;
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return uVar2;
}


// ============================================================================
// Function: TSABuffer_Free @ 6ba8df9f
// ============================================================================

void TSABuffer_Free(void)

{
                    /* 0x4df9f  1477  TSABuffer_Free */
  TSA_Free(&g_tsaBufGridIIRDif);
  TSA_Free(&g_tsaBufBlNormal);
  TSA_Free(&g_tsaBufBlGrid);
  g_tsaBufBl = 0;
  TSA_Free(&g_tsaBufTzGrid);
  g_tsaBufTz = 0;
  TSA_Free(&g_tsaBufSd);
  TSA_Free(&g_tsaBufSSChargerNFilterDif);
  TSA_Free(&g_tsaBufRawStoredSinceSSNoSignal);
  TSA_Free(&g_tsaBufRawStoredSinceMutualSSNotMatch);
  TSA_Free(&g_tsaBufTzPrev);
  TSA_Free(&g_tsaBufTzZonePrev);
  TSA_Free(&g_tsaBufTzZone);
  TSA_Free(&g_tsaBufTzID);
  TSA_Free(&g_tsaBufSubTZ);
  TSA_Free(&g_tsaBufTimeOff);
  TSA_Free(&g_tsaBufTimeDown);
  TSA_Free(&g_tsaBufTimeRpt);
  TSA_Free(&g_tsaBufMinSigOff);
  TSA_Free(&g_tsaBufGripStage);
  TSA_Free(&g_tsaBufGripTime);
  TSA_Free(&g_tsaBufGripTimeOff);
  TSA_Free(&g_tsaBufGripRegion);
  TSA_Free(&g_tsaBufMaxSigDown);
  TSA_Free(&g_tsaBufSilentTouchNum);
  TSA_Free(&g_tsaBufPreCMFDif);
  TSA_Free(&g_tsaBufPreDif);
  TSA_Free(&g_tsaBufPreHWRaw);
  TSA_Free(&g_tsaBufRaw2rawDif);
  TSA_Free(&g_tsaBufRaw2rawDifAftCMF);
  TSA_Free(&g_tsaBufCommonModeNoise);
  TSA_Free(&g_tsaBufRawCapture);
  TSA_Free(&g_tsaBufRawCaptureNextSide);
  TSA_Free(&g_tsaBufTzAge);
  TSA_Free(&g_tsaBufCleanBaseline);
  TSA_Free(&g_tsaBufPreUnnormalizedRaw);
  TSA_Free(&g_tsaBufPreCMFRawGrid);
  TSA_Free(&g_tsaBufRawGrid);
  TSA_Free(&g_tsaBufDifGrid);
  return;
}


// ============================================================================
// Function: TSABuffer_Malloc @ 6ba8d7f6
// ============================================================================

undefined8 TSABuffer_Malloc(void)

{
  undefined8 uVar1;
  
                    /* 0x4d7f6  1481  TSABuffer_Malloc */
  g_tsaBufGridIIRDif = TSA_MallocAndInit((uint)g_gridBufSize * 2);
  if (g_tsaBufGridIIRDif == 0) {
    uVar1 = 0;
  }
  else {
    g_tsaBufBlNormal = TSA_MallocAndInit((uint)g_gridBufSize * 2);
    if (g_tsaBufBlNormal == 0) {
      uVar1 = 0;
    }
    else {
      g_tsaBufBlGrid = TSA_MallocAndInit((uint)g_gridBufSize * 2);
      if (g_tsaBufBlGrid == 0) {
        uVar1 = 0;
      }
      else {
        g_tsaBufBl = g_tsaBufBlGrid;
        g_tsaBufTzGrid = TSA_MallocAndInit(g_gridBufSize);
        if (g_tsaBufTzGrid == 0) {
          uVar1 = 0;
        }
        else {
          g_tsaBufTz = g_tsaBufTzGrid;
          g_tsaBufSd = TSA_MallocAndInit((uint)g_gridBufSize * 2);
          if (g_tsaBufSd == 0) {
            uVar1 = 0;
          }
          else {
            g_tsaBufSSChargerNFilterDif = TSA_MallocAndInit((uint)g_gridBufSize * 2);
            if (g_tsaBufSSChargerNFilterDif == 0) {
              uVar1 = 0;
            }
            else {
              g_tsaBufRawStoredSinceSSNoSignal = TSA_MallocAndInit((uint)g_gridBufSize * 2);
              if (g_tsaBufRawStoredSinceSSNoSignal == 0) {
                uVar1 = 0;
              }
              else {
                g_tsaBufRawStoredSinceMutualSSNotMatch = TSA_MallocAndInit((uint)g_gridBufSize * 2);
                if (g_tsaBufRawStoredSinceMutualSSNotMatch == 0) {
                  uVar1 = 0;
                }
                else {
                  g_tsaBufTzPrev = TSA_MallocAndInit(g_gridBufSize);
                  if (g_tsaBufTzPrev == 0) {
                    uVar1 = 0;
                  }
                  else {
                    g_tsaBufTzZonePrev = TSA_MallocAndInit(g_gridBufSize);
                    if (g_tsaBufTzZonePrev == 0) {
                      uVar1 = 0;
                    }
                    else {
                      g_tsaBufTzZone = TSA_MallocAndInit(g_gridBufSize);
                      if (g_tsaBufTzZone == 0) {
                        uVar1 = 0;
                      }
                      else {
                        g_tsaBufTzID = TSA_MallocAndInit(g_gridBufSize);
                        if (g_tsaBufTzID == 0) {
                          uVar1 = 0;
                        }
                        else {
                          g_tsaBufSubTZ = TSA_MallocAndInit(g_gridBufSize);
                          if (g_tsaBufSubTZ == 0) {
                            uVar1 = 0;
                          }
                          else {
                            g_tsaBufTimeOff = TSA_MallocAndInit((uint)g_gridBufSize * 2);
                            if (g_tsaBufTimeOff == 0) {
                              uVar1 = 0;
                            }
                            else {
                              g_tsaBufTimeDown = TSA_MallocAndInit((uint)g_gridBufSize * 2);
                              if (g_tsaBufTimeDown == 0) {
                                uVar1 = 0;
                              }
                              else {
                                g_tsaBufTimeRpt = TSA_MallocAndInit((uint)g_gridBufSize * 2);
                                if (g_tsaBufTimeRpt == 0) {
                                  uVar1 = 0;
                                }
                                else {
                                  g_tsaBufMinSigOff = TSA_MallocAndInit((uint)g_gridBufSize * 2);
                                  if (g_tsaBufMinSigOff == 0) {
                                    uVar1 = 0;
                                  }
                                  else {
                                    g_tsaBufGripStage = TSA_MallocAndInit(g_gridBufSize);
                                    if (g_tsaBufGripStage == 0) {
                                      uVar1 = 0;
                                    }
                                    else {
                                      g_tsaBufGripTime = TSA_MallocAndInit((uint)g_gridBufSize * 2);
                                      if (g_tsaBufGripTime == 0) {
                                        uVar1 = 0;
                                      }
                                      else {
                                        g_tsaBufGripTimeOff =
                                             TSA_MallocAndInit((uint)g_gridBufSize * 2);
                                        if (g_tsaBufGripTimeOff == 0) {
                                          uVar1 = 0;
                                        }
                                        else {
                                          g_tsaBufGripRegion = TSA_MallocAndInit(g_gridBufSize);
                                          if (g_tsaBufGripRegion == 0) {
                                            uVar1 = 0;
                                          }
                                          else {
                                            g_tsaBufMaxSigDown =
                                                 TSA_MallocAndInit((uint)g_gridBufSize * 2);
                                            if (g_tsaBufMaxSigDown == 0) {
                                              uVar1 = 0;
                                            }
                                            else {
                                              g_tsaBufSilentTouchNum =
                                                   TSA_MallocAndInit(g_gridBufSize);
                                              if (g_tsaBufSilentTouchNum == 0) {
                                                uVar1 = 0;
                                              }
                                              else {
                                                g_tsaBufPreCMFDif =
                                                     TSA_MallocAndInit((uint)g_gridBufSize * 2);
                                                if (g_tsaBufPreCMFDif == 0) {
                                                  uVar1 = 0;
                                                }
                                                else {
                                                  g_tsaBufPreDif =
                                                       TSA_MallocAndInit((uint)g_gridBufSize * 2);
                                                  if (g_tsaBufPreDif == 0) {
                                                    uVar1 = 0;
                                                  }
                                                  else {
                                                    g_tsaBufPreHWRaw =
                                                         TSA_MallocAndInit((uint)g_gridBufSize * 2);
                                                    if (g_tsaBufPreHWRaw == 0) {
                                                      uVar1 = 0;
                                                    }
                                                    else {
                                                      g_tsaBufRaw2rawDif =
                                                           TSA_MallocAndInit((uint)g_gridBufSize * 2
                                                                            );
                                                      if (g_tsaBufRaw2rawDif == 0) {
                                                        uVar1 = 0;
                                                      }
                                                      else {
                                                        g_tsaBufRaw2rawDifAftCMF =
                                                             TSA_MallocAndInit((uint)g_gridBufSize *
                                                                               2);
                                                        if (g_tsaBufRaw2rawDifAftCMF == 0) {
                                                          uVar1 = 0;
                                                        }
                                                        else {
                                                          g_tsaBufCommonModeNoise =
                                                               TSA_MallocAndInit((uint)
                                                  g_gridMapBufSize * 2);
                                                  if (g_tsaBufCommonModeNoise == 0) {
                                                    uVar1 = 0;
                                                  }
                                                  else {
                                                    g_tsaBufRawCapture =
                                                         TSA_MallocAndInit((uint)g_gridBufSize * 2);
                                                    if (g_tsaBufRawCapture == 0) {
                                                      uVar1 = 0;
                                                    }
                                                    else {
                                                      g_tsaBufRawCaptureNextSide =
                                                           TSA_MallocAndInit((uint)g_gridBufSize * 2
                                                                            );
                                                      if (g_tsaBufRawCaptureNextSide == 0) {
                                                        uVar1 = 0;
                                                      }
                                                      else {
                                                        g_tsaBufTzAge =
                                                             TSA_MallocAndInit((uint)g_gridBufSize *
                                                                               2);
                                                        if (g_tsaBufTzAge == 0) {
                                                          uVar1 = 0;
                                                        }
                                                        else {
                                                          g_tsaBufCleanBaseline =
                                                               TSA_MallocAndInit((uint)g_gridBufSize
                                                                                 * 2);
                                                          if (g_tsaBufCleanBaseline == 0) {
                                                            uVar1 = 0;
                                                          }
                                                          else {
                                                            g_tsaBufPreUnnormalizedRaw =
                                                                 TSA_MallocAndInit((uint)
                                                  g_gridBufSize * 2);
                                                  if (g_tsaBufPreUnnormalizedRaw == 0) {
                                                    uVar1 = 0;
                                                  }
                                                  else {
                                                    g_tsaBufRawGrid =
                                                         TSA_MallocAndInit((uint)g_gridBufSize * 2);
                                                    if (g_tsaBufRawGrid == 0) {
                                                      uVar1 = 0;
                                                    }
                                                    else {
                                                      g_tsaBufRaw = g_tsaBufRawGrid;
                                                      g_tsaBufPreCMFRawGrid =
                                                           TSA_MallocAndInit((uint)g_gridBufSize * 2
                                                                            );
                                                      if (g_tsaBufPreCMFRawGrid == 0) {
                                                        uVar1 = 0;
                                                      }
                                                      else {
                                                        g_tsaBufPreCMFRaw = g_tsaBufPreCMFRawGrid;
                                                        g_tsaBufDifGrid =
                                                             TSA_MallocAndInit((uint)
                                                  g_gridMapBufSize * 2);
                                                  if (g_tsaBufDifGrid == 0) {
                                                    uVar1 = 0;
                                                  }
                                                  else {
                                                    uVar1 = 1;
                                                    g_tsaBufDif = g_tsaBufDifGrid;
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                }
                                              }
                                            }
                                          }
                                        }
                                      }
                                    }
                                  }
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return uVar1;
}


