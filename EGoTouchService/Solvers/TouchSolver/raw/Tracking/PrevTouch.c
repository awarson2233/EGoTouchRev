#include "TSACoreInternal.h"

// ============================================================================
// Function: PrevTouch_AssignCurIdxBasedOnID @ 6ba6dbc9
// ============================================================================

void PrevTouch_AssignCurIdxBasedOnID(void)

{
  bool bVar1;
  byte local_a;
  byte local_9;
  
                    /* 0x2dbc9  1050  PrevTouch_AssignCurIdxBasedOnID */
  local_9 = 0;
  do {
    if (g_pPrevTouches->bPrevTouchCount <= local_9) {
      return;
    }
    bVar1 = false;
    for (local_a = 0; local_a < **(byte **)_refptr_g_touchesPtr; local_a = local_a + 1) {
      if (*(char *)((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x154
                   ) ==
          *(char *)((longlong)g_pPrevTouches + (longlong)(int)(uint)local_9 * 0x478 + 0x154)) {
        bVar1 = true;
        break;
      }
    }
    if (bVar1) {
      *(byte *)((longlong)g_pPrevTouches + (longlong)(int)(uint)local_9 * 0x478 + 0x4f2) = local_a;
    }
    else {
      *(undefined1 *)((longlong)g_pPrevTouches + (longlong)(int)(uint)local_9 * 0x478 + 0x4f2) =
           0x14;
    }
    local_9 = local_9 + 1;
  } while( true );
}


// ============================================================================
// Function: PrevTouch_PostProcess @ 6ba6e09d
// ============================================================================

void PrevTouch_PostProcess(void)

{
                    /* 0x2e09d  1068  PrevTouch_PostProcess */
  *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) =
       *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) & 0xfffffffd;
  if ((*(char *)(*(longlong *)_refptr_g_touchesPtr + 3) == '\0') &&
     (g_pPrevTouches->field_0x3 != '\0')) {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) | 2;
  }
  *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) =
       *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) & 0xfffffffb;
  if ((*(char *)(*(longlong *)_refptr_g_touchesPtr + 3) != '\0') &&
     (g_pPrevTouches->field_0x3 == '\0')) {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) | 4;
  }
  g_pPrevTouches->field_0x3 = *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 3);
  g_pPrevTouches->field_0x4 = *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 4);
  *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x1c) =
       *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) & ~*(uint *)&g_pPrevTouches->field_0x18;
  *(undefined4 *)&g_pPrevTouches->field_0x18 =
       *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x18);
  *(undefined4 *)&g_pPrevTouches->field_0x1c =
       *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x1c);
  g_pPrevTouches->field_0x1 = *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 1);
  g_pPrevTouches->field_0x2 = *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 2);
  *(undefined2 *)&g_pPrevTouches->field_0x14 =
       *(undefined2 *)(*(longlong *)_refptr_g_touchesPtr + 0x14);
  g_pPrevTouches->field_0x5 = *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 5);
  g_pPrevTouches->field_0x7 = *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 7);
  g_pPrevTouches->field_0x8 = *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 8);
  g_pPrevTouches->field_0x9 = *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 9);
  g_pPrevTouches->field_0xa = *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 10);
  g_pPrevTouches->field_0xb = *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0xb);
  g_pPrevTouches->field_0xc = *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0xc);
  g_pPrevTouches->field_0xd = *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0xd);
  g_pPrevTouches->field_0xe = *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0xe);
  g_pPrevTouches->field_0xf = *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0xf);
  g_pPrevTouches->field_0x10 = *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0x10);
  g_pPrevTouches->field_0x11 = *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0x11);
  g_pPrevTouches->field_0x12 = *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0x12);
  g_pPrevTouches->field_0x13 = *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0x13);
  g_pPrevTouches->nTouchCount = *(int *)(*(longlong *)_refptr_g_touchesPtr + 0x20);
  g_pPrevTouches->nTouchCount = g_pPrevTouches->nTouchCount & 0xfffffbff;
  *(undefined4 *)(g_pPrevTouches + 1) = *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x24);
  *(undefined4 *)&g_pPrevTouches[8].field_0x18 =
       *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x138);
  *(undefined8 *)&g_pPrevTouches[1].field_0x4 =
       *(undefined8 *)(*(longlong *)_refptr_g_touchesPtr + 0x28);
  *(undefined8 *)&g_pPrevTouches[1].field_0xc =
       *(undefined8 *)(*(longlong *)_refptr_g_touchesPtr + 0x30);
  if (*(longlong *)(*(longlong *)_refptr_g_touchesPtr + 0x38) != 0) {
    *(undefined8 *)&g_pPrevTouches[1].field_0x14 =
         *(undefined8 *)(*(longlong *)_refptr_g_touchesPtr + 0x38);
  }
  PrevTouch_RemoveOff();
  return;
}


// ============================================================================
// Function: PrevTouch_Release @ 6ba6da90
// ============================================================================

void PrevTouch_Release(void)

{
  int iVar1;
  
                    /* 0x2da90  1069  PrevTouch_Release */
  iVar1 = memset_s(g_pPrevTouches,0x5aa0,0,0x5aa0);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/Touch/PrevTouchData.c",0x7a,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  if (*(int *)(_refptr_g_tsaInitOnce + 4) == 4) {
    *(uint *)&g_pPrevTouches->field_0x18 = *(uint *)&g_pPrevTouches->field_0x18 | 2;
  }
  if (*(int *)(_refptr_g_tsaInitOnce + 4) == 8) {
    *(uint *)&g_pPrevTouches->field_0x18 = *(uint *)&g_pPrevTouches->field_0x18 | 4;
  }
  return;
}


// ============================================================================
// Function: PrevTouch_GetIdxBasedOnID @ 6ba6db56
// ============================================================================

byte PrevTouch_GetIdxBasedOnID(char param_1)

{
  bool bVar1;
  byte local_9;
  
                    /* 0x2db56  1052  PrevTouch_GetIdxBasedOnID */
  bVar1 = false;
  local_9 = 0;
  do {
    if (g_pPrevTouches->bPrevTouchCount <= local_9) {
LAB_6ba6dbb2:
      if (!bVar1) {
        local_9 = 0x14;
      }
      return local_9;
    }
    if (*(char *)((longlong)g_pPrevTouches + (longlong)(int)(uint)local_9 * 0x478 + 0x154) ==
        param_1) {
      bVar1 = true;
      goto LAB_6ba6dbb2;
    }
    local_9 = local_9 + 1;
  } while( true );
}


// ============================================================================
// Function: PrevTouch_GetIdxBasedOnCurIdx @ 6ba6dcc4
// ============================================================================

undefined1 PrevTouch_GetIdxBasedOnCurIdx(byte param_1)

{
                    /* 0x2dcc4  1051  PrevTouch_GetIdxBasedOnCurIdx */
  return *(undefined1 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x287);
}


// ============================================================================
// Function: PrevTouch_RemoveOff @ 6ba6de46
// ============================================================================

void PrevTouch_RemoveOff(void)

{
  byte bVar1;
  byte local_a;
  byte local_9;
  
  if (((*_refptr_g_tsaStaticPtr)->dwProcessingState & PROC_MS_ACTIVE) != 0) {
    DAT_6bbc68f4 = 0;
  }
  for (local_a = 0; local_a < **(byte **)_refptr_g_touchesPtr; local_a = local_a + 1) {
    bVar1 = PrevTouch_GetIdxBasedOnCurIdx(local_a);
    if (bVar1 < 0x14) {
      if (((((*_refptr_g_tsaStaticPtr)->dwProcessingState & PROC_MS_ACTIVE) != 0) &&
          ((*(uint *)((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x21c) & 0x10) != 0)) && (DAT_6bbc68f4 < 0x14)) {
        (&g_prevTouchInvalidID)[(int)(uint)DAT_6bbc68f4] =
             *(undefined1 *)
              ((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x154);
        DAT_6bbc68f4 = DAT_6bbc68f4 + 1;
      }
      if ((*(uint *)((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x21c) & 0x70) == 0) {
        PrevTouch_UnitCopy(bVar1,local_a);
      }
      else {
        if ((*(uint *)((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x1f8) & 0x20000000) != 0) {
          *(void **)&(*_refptr_g_tsaStaticPtr)->field_0x90 =
               (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
          *(undefined2 *)&(*_refptr_g_tsaStaticPtr)->field_0x98 =
               *(undefined2 *)
                ((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162);
          *(undefined2 *)&(*_refptr_g_tsaStaticPtr)->field_0x9a =
               *(undefined2 *)
                ((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164);
        }
        PrevTouch_UnitRemove(bVar1);
        Touch_AssignPreIdxBasedOnID();
      }
    }
  }
  for (local_9 = g_pPrevTouches->bPrevTouchCount; local_9 < 0x14; local_9 = local_9 + 1) {
    PrevTouch_UnitRelease(local_9);
  }
  return;
}


// ============================================================================
// Function: PrevTouch_UnitCopy @ 6ba6d300
// ============================================================================

void PrevTouch_UnitCopy(byte param_1,byte param_2)

{
  errno_t eVar1;
  
                    /* 0x2d300  1070  PrevTouch_UnitCopy */
  if (0x13 < param_1) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/Touch/PrevTouchData.c",0x14,"preIdx < TSA_MAX_TOUCH_NUM");
    HardwareAnalyzer_SetSoftException();
  }
  if (0x13 < param_2) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/Touch/PrevTouchData.c",0x15,"curIdx < TSA_MAX_TOUCH_NUM");
    HardwareAnalyzer_SetSoftException();
  }
  if (((param_1 < 0x14) && (param_2 < 0x14)) &&
     (eVar1 = memcpy_s((void *)((longlong)g_pPrevTouches +
                               (longlong)(int)(uint)param_1 * 0x478 + 0x140),0x144,
                       (void *)(*(longlong *)_refptr_g_touchesPtr +
                               (longlong)(int)(uint)param_2 * 0x168 + 0x140),0x144), eVar1 != 0)) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/Touch/PrevTouchData.c",0x24,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: PrevTouch_UnitRelease @ 6ba6da04
// ============================================================================

void PrevTouch_UnitRelease(byte param_1)

{
  int iVar1;
  
  if ((param_1 < 0x14) &&
     (iVar1 = memset_s((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x140,0x478
                       ,0,0x478), iVar1 != 0)) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/Touch/PrevTouchData.c",0x73,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: PrevTouch_UnitRemove @ 6ba6dcf3
// ============================================================================

void PrevTouch_UnitRemove(byte param_1)

{
  errno_t eVar1;
  byte local_9;
  
  if (0x14 < g_pPrevTouches->bPrevTouchCount) {
    if (0x14 < g_pPrevTouches->bPrevTouchCount) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Alg/Touch/PrevTouchData.c",0xc2,
                        "PrevTouchesPrpt(num) <= TSA_MAX_TOUCH_NUM");
      HardwareAnalyzer_SetSoftException();
    }
    g_pPrevTouches->bPrevTouchCount = '\x14';
  }
  if (param_1 < g_pPrevTouches->bPrevTouchCount) {
    for (local_9 = param_1; (int)(uint)local_9 < (int)(g_pPrevTouches->bPrevTouchCount - 1);
        local_9 = local_9 + 1) {
      eVar1 = memcpy_s((void *)((longlong)g_pPrevTouches +
                               (longlong)(int)(uint)local_9 * 0x478 + 0x140),0x478,
                       (void *)((longlong)g_pPrevTouches +
                               (longlong)(int)(local_9 + 1) * 0x478 + 0x140),0x478);
      if (eVar1 != 0) {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                          "../../src/Alg/Touch/PrevTouchData.c",0xcb,"rc == EOK");
        HardwareAnalyzer_SetSoftException();
      }
    }
    g_pPrevTouches->bPrevTouchCount = g_pPrevTouches->bPrevTouchCount + 0xff;
  }
  return;
}


// ============================================================================
// Function: PrevTouch_UnitUpdate @ 6ba6d5ed
// ============================================================================

void PrevTouch_UnitUpdate(byte param_1,byte param_2)

{
  int iVar1;
  short sVar2;
  PrevTouchArray *pPVar3;
  short sVar4;
  short sVar5;
  longlong lVar6;
  longlong lVar7;
  short local_a;
  
                    /* 0x2d5ed  1071  PrevTouch_UnitUpdate */
  PrevTouch_UnitUpdateCommon(param_1,param_2);
  pPVar3 = g_pPrevTouches;
  lVar7 = (longlong)(int)(uint)param_1 * 0x478;
  lVar6 = *(longlong *)_refptr_g_touchesPtr + (longlong)(int)(uint)param_2 * 0x168 + 0x140;
  iVar1 = *(int *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c)
  ;
  if ((iVar1 != 8) && (iVar1 != 0x20)) {
    sVar4 = *(short *)(lVar6 + 0x1e) - *(short *)((longlong)g_pPrevTouches + lVar7 + 0x15e);
    sVar5 = *(short *)(lVar6 + 0x20) - *(short *)((longlong)g_pPrevTouches + lVar7 + 0x160);
    local_a = *(short *)(lVar6 + 0x10);
    if (local_a == 0) {
      local_a = 1;
    }
    sVar2 = sVar4 / local_a;
    local_a = sVar5 / local_a;
    *(short *)((longlong)g_pPrevTouches + lVar7 + 0x4c8) = sVar4;
    *(short *)((longlong)pPVar3 + lVar7 + 0x4ca) = sVar5;
    *(short *)((longlong)pPVar3 + lVar7 + 0x4d0) =
         sVar2 - *(short *)((longlong)pPVar3 + lVar7 + 0x4cc);
    *(short *)((longlong)pPVar3 + lVar7 + 0x4d2) =
         local_a - *(short *)((longlong)pPVar3 + lVar7 + 0x4ce);
    *(short *)((longlong)pPVar3 + lVar7 + 0x4cc) = sVar2;
    *(short *)((longlong)pPVar3 + lVar7 + 0x4ce) = local_a;
    sVar4 = *(short *)(lVar6 + 0x22) - *(short *)((longlong)pPVar3 + lVar7 + 0x162);
    sVar5 = *(short *)(lVar6 + 0x24) - *(short *)((longlong)pPVar3 + lVar7 + 0x164);
    *(short *)((longlong)pPVar3 + lVar7 + 0x4d8) =
         sVar4 - *(short *)((longlong)pPVar3 + lVar7 + 0x4d4);
    *(short *)((longlong)pPVar3 + lVar7 + 0x4da) =
         sVar5 - *(short *)((longlong)pPVar3 + lVar7 + 0x4d6);
    *(short *)((longlong)pPVar3 + lVar7 + 0x4d4) = sVar4;
    *(short *)((longlong)pPVar3 + lVar7 + 0x4d6) = sVar5;
    *(int *)((longlong)pPVar3 + lVar7 + 0x4dc) =
         *(int *)((longlong)pPVar3 + lVar7 + 0x4dc) + (int)sVar4;
    *(int *)((longlong)pPVar3 + lVar7 + 0x4e0) =
         *(int *)((longlong)pPVar3 + lVar7 + 0x4e0) + (int)sVar5;
    if (*(ushort *)((longlong)pPVar3 + lVar7 + 0x4f6) < *(ushort *)(lVar6 + 0x100)) {
      *(undefined2 *)((longlong)pPVar3 + lVar7 + 0x4f6) = *(undefined2 *)(lVar6 + 0x100);
    }
    if (*(ushort *)((longlong)pPVar3 + lVar7 + 0x4f6) < *(ushort *)(lVar6 + 0x100)) {
      *(undefined2 *)((longlong)pPVar3 + lVar7 + 0x4f6) = *(undefined2 *)(lVar6 + 0x100);
    }
    if ((*(short *)((longlong)pPVar3 + lVar7 + 0x518) == 0) ||
       (*(ushort *)(lVar6 + 0x68) < *(ushort *)((longlong)pPVar3 + lVar7 + 0x518))) {
      *(undefined2 *)((longlong)pPVar3 + lVar7 + 0x518) = *(undefined2 *)(lVar6 + 0x68);
    }
    if ((*(short *)((longlong)pPVar3 + lVar7 + 0x51a) == 0) ||
       (*(ushort *)(lVar6 + 0x6a) < *(ushort *)((longlong)pPVar3 + lVar7 + 0x51a))) {
      *(undefined2 *)((longlong)pPVar3 + lVar7 + 0x51a) = *(undefined2 *)(lVar6 + 0x6a);
    }
    if ((*(short *)((longlong)pPVar3 + lVar7 + 0x51c) == 0) ||
       (*(ushort *)((longlong)pPVar3 + lVar7 + 0x51c) < *(ushort *)(lVar6 + 0x68))) {
      *(undefined2 *)((longlong)pPVar3 + lVar7 + 0x51c) = *(undefined2 *)(lVar6 + 0x68);
    }
    if ((*(short *)((longlong)pPVar3 + lVar7 + 0x51e) == 0) ||
       (*(ushort *)((longlong)pPVar3 + lVar7 + 0x51e) < *(ushort *)(lVar6 + 0x6a))) {
      *(undefined2 *)((longlong)pPVar3 + lVar7 + 0x51e) = *(undefined2 *)(lVar6 + 0x6a);
    }
    if (*(int *)(lVar6 + 0x80) <=
        (*(int *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)*(byte *)(lVar6 + 0xfc) * 0x14 + 0x20)
        * 100) / 0x46) {
      *(void **)((longlong)pPVar3 + lVar7 + 0x520) = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
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
// Function: PrevTouch_HistoryBufferGetNum @ 6ba6e9dc
// ============================================================================

undefined1 PrevTouch_HistoryBufferGetNum(byte param_1)

{
  undefined1 uVar1;
  
                    /* 0x2e9dc  1055  PrevTouch_HistoryBufferGetNum */
  if (param_1 < 0x14) {
    uVar1 = *(undefined1 *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288)
    ;
  }
  else {
    if (0x13 < param_1) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Alg/Touch/PrevTouchData.c",0x1b2,"preIdx < TSA_MAX_TOUCH_NUM");
      HardwareAnalyzer_SetSoftException();
    }
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: PrevTouch_HistoryBufferPop @ 6ba6ec0d
// ============================================================================

undefined8 PrevTouch_HistoryBufferPop(byte param_1,undefined8 *param_2)

{
  PrevTouchArray *pPVar1;
  int iVar2;
  undefined8 uVar3;
  longlong lVar4;
  longlong lVar5;
  byte local_9;
  
                    /* 0x2ec0d  1057  PrevTouch_HistoryBufferPop */
  iVar2 = memset_s(param_2,0x10,0,0x10);
  if (iVar2 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/Touch/PrevTouchData.c",0x1d1,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  if (param_1 < 0x14) {
    if (*(char *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288) == '\0')
    {
      uVar3 = 0;
    }
    else {
      uVar3 = *(undefined8 *)
               ((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x298);
      *param_2 = *(undefined8 *)
                  ((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x290);
      param_2[1] = uVar3;
      for (local_9 = 1; pPVar1 = g_pPrevTouches,
          local_9 < *(byte *)((longlong)g_pPrevTouches +
                             (longlong)(int)(uint)param_1 * 0x478 + 0x288); local_9 = local_9 + 1) {
        lVar5 = (longlong)(int)(uint)param_1 * 0x478 + ((longlong)(int)(local_9 - 1) + 0x14) * 0x10;
        lVar4 = (longlong)(int)(uint)param_1 * 0x478 + ((longlong)(int)(uint)local_9 + 0x14) * 0x10;
        uVar3 = *(undefined8 *)((longlong)g_pPrevTouches + lVar4 + 0x158);
        *(undefined8 *)((longlong)g_pPrevTouches + lVar5 + 0x150) =
             *(undefined8 *)((longlong)g_pPrevTouches + lVar4 + 0x150);
        *(undefined8 *)((longlong)pPVar1 + lVar5 + 0x158) = uVar3;
      }
      *(char *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288) =
           *(char *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288) + -1;
      uVar3 = 1;
    }
  }
  else {
    if (0x13 < param_1) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Alg/Touch/PrevTouchData.c",0x1d5,"preIdx < TSA_MAX_TOUCH_NUM");
      HardwareAnalyzer_SetSoftException();
    }
    uVar3 = 0;
  }
  return uVar3;
}


// ============================================================================
// Function: PrevTouch_HistoryBufferGet @ 6ba6ee1d
// ============================================================================

undefined8 PrevTouch_HistoryBufferGet(byte param_1,byte param_2,undefined8 *param_3)

{
  int iVar1;
  undefined8 uVar2;
  longlong lVar3;
  
                    /* 0x2ee1d  1054  PrevTouch_HistoryBufferGet */
  iVar1 = memset_s(param_3,0x10,0,0x10);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/Touch/PrevTouchData.c",0x1ec,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  if (param_1 < 0x14) {
    if (param_2 < *(byte *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288)
       ) {
      lVar3 = (longlong)(int)(uint)param_1 * 0x478 + ((longlong)(int)(uint)param_2 + 0x14) * 0x10;
      uVar2 = *(undefined8 *)((longlong)g_pPrevTouches + lVar3 + 0x158);
      *param_3 = *(undefined8 *)((longlong)g_pPrevTouches + lVar3 + 0x150);
      param_3[1] = uVar2;
      uVar2 = 1;
    }
    else {
      uVar2 = 0;
    }
  }
  else {
    if (0x13 < param_1) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Alg/Touch/PrevTouchData.c",0x1ee,"preIdx < TSA_MAX_TOUCH_NUM");
      HardwareAnalyzer_SetSoftException();
    }
    uVar2 = 0;
  }
  return uVar2;
}


// ============================================================================
// Function: PrevTouch_UnitUpdateCommon @ 6ba6d427
// ============================================================================

void PrevTouch_UnitUpdateCommon(byte param_1,byte param_2)

{
  byte bVar1;
  byte bVar2;
  
  if (*(short *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x156) == 0) {
    bVar1 = *(byte *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x23c);
    bVar2 = (*_refptr_g_peaksPtr)[(longlong)(int)(uint)bVar1 * 0x14 + 0x15].bPeakCount;
    bVar1 = (*_refptr_g_peaksPtr)[(longlong)(int)(uint)bVar1 * 0x14 + 0x16].bPeakCount;
    *(undefined2 *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x5a6) =
         *(undefined2 *)
          ((ulonglong)((uint)bVar2 + (uint)bVar1 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2
          + g_tsaBufMinSigOff);
    *(undefined2 *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x5a8) =
         *(undefined2 *)
          ((ulonglong)((uint)bVar2 + (uint)bVar1 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2
          + g_tsaBufTimeOff);
    *(undefined2 *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x5aa) =
         *(undefined2 *)
          ((ulonglong)((uint)bVar2 + (uint)bVar1 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2
          + g_tsaBufTimeRpt);
  }
  return;
}


// ============================================================================
// Function: PrevTouch_IsValidId @ 6ba6e909
// ============================================================================

undefined8 PrevTouch_IsValidId(char param_1)

{
  byte local_a;
  char local_9;
  
                    /* 0x2e909  1067  PrevTouch_IsValidId */
  for (local_9 = '\0'; (int)local_9 < (int)(uint)g_pPrevTouches->bPrevTouchCount;
      local_9 = local_9 + '\x01') {
    if (*(char *)((longlong)g_pPrevTouches + (longlong)(int)local_9 * 0x478 + 0x154) == param_1) {
      return 0;
    }
  }
  local_a = 0;
  while( true ) {
    if (DAT_6bbc68f4 <= local_a) {
      return 1;
    }
    if ((&g_prevTouchInvalidID)[(int)(uint)local_a] == param_1) break;
    local_a = local_a + 1;
  }
  return 0;
}


// ============================================================================
// Function: PrevTouch_HistoryBufferPush @ 6ba6ea52
// ============================================================================

undefined8 PrevTouch_HistoryBufferPush(byte param_1,undefined8 *param_2)

{
  PrevTouchArray *pPVar1;
  undefined8 uVar2;
  longlong lVar3;
  
                    /* 0x2ea52  1058  PrevTouch_HistoryBufferPush */
  if (param_1 < 0x14) {
    if (0x1d < *(byte *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288)) {
      if (0x1e < *(byte *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288))
      {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                          "../../src/Alg/Touch/PrevTouchData.c",0x1c2,
                          "PrevTouchPrpt(preIdx, historyBuffer.num) <= PREVTOUCH_HISTORY_BUFFER_SIZE"
                         );
        HardwareAnalyzer_SetSoftException();
      }
      *(undefined1 *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288) =
           0x1e;
      PrevTouch_HistoryBufferRemove(param_1,0);
    }
    pPVar1 = g_pPrevTouches;
    lVar3 = (longlong)(int)(uint)param_1 * 0x478 +
            ((longlong)
             (int)(uint)*(byte *)((longlong)g_pPrevTouches +
                                 (longlong)(int)(uint)param_1 * 0x478 + 0x288) + 0x14) * 0x10;
    uVar2 = param_2[1];
    *(undefined8 *)((longlong)g_pPrevTouches + lVar3 + 0x150) = *param_2;
    *(undefined8 *)((longlong)pPVar1 + lVar3 + 0x158) = uVar2;
    *(char *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288) =
         *(char *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288) + '\x01'
    ;
    uVar2 = 1;
  }
  else {
    if (0x13 < param_1) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Alg/Touch/PrevTouchData.c",0x1be,"preIdx < TSA_MAX_TOUCH_NUM");
      HardwareAnalyzer_SetSoftException();
    }
    uVar2 = 0;
  }
  return uVar2;
}


// ============================================================================
// Function: PrevTouch_HistoryBufferClean @ 6ba6e9b0
// ============================================================================

void PrevTouch_HistoryBufferClean(byte param_1)

{
                    /* 0x2e9b0  1053  PrevTouch_HistoryBufferClean */
  *(undefined1 *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288) = 0;
  return;
}


// ============================================================================
// Function: PrevTouch_HistoryBufferInsert @ 6ba6ef44
// ============================================================================

void PrevTouch_HistoryBufferInsert(byte param_1,byte param_2,undefined8 *param_3)

{
  undefined8 uVar1;
  PrevTouchArray *pPVar2;
  longlong lVar3;
  longlong lVar4;
  char local_9;
  
                    /* 0x2ef44  1056  PrevTouch_HistoryBufferInsert */
  if (param_1 < 0x14) {
    local_9 = *(char *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288);
    while (pPVar2 = g_pPrevTouches, local_9 = local_9 + -1, (int)(uint)param_2 <= (int)local_9) {
      if (local_9 + 1 < 0x1e) {
        lVar4 = (longlong)(int)(uint)param_1 * 0x478 + ((longlong)(local_9 + 1) + 0x14) * 0x10;
        lVar3 = (longlong)(int)(uint)param_1 * 0x478 + ((longlong)(int)local_9 + 0x14) * 0x10;
        uVar1 = *(undefined8 *)((longlong)g_pPrevTouches + lVar3 + 0x158);
        *(undefined8 *)((longlong)g_pPrevTouches + lVar4 + 0x150) =
             *(undefined8 *)((longlong)g_pPrevTouches + lVar3 + 0x150);
        *(undefined8 *)((longlong)pPVar2 + lVar4 + 0x158) = uVar1;
      }
    }
    lVar3 = (longlong)(int)(uint)param_1 * 0x478 + ((longlong)(int)(uint)param_2 + 0x14) * 0x10;
    uVar1 = param_3[1];
    *(undefined8 *)((longlong)g_pPrevTouches + lVar3 + 0x150) = *param_3;
    *(undefined8 *)((longlong)pPVar2 + lVar3 + 0x158) = uVar1;
    if (*(byte *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288) < 0x1e) {
      *(char *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288) =
           *(char *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288) +
           '\x01';
    }
  }
  else if (0x13 < param_1) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/Touch/PrevTouchData.c",0x201,"preIdx < TSA_MAX_TOUCH_NUM");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: PrevTouch_IsHistoryBufferFull @ 6ba6f3ef
// ============================================================================

bool PrevTouch_IsHistoryBufferFull(byte param_1)

{
  bool bVar1;
  
                    /* 0x2f3ef  1065  PrevTouch_IsHistoryBufferFull */
  if (param_1 < 0x14) {
    bVar1 = *(char *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288) ==
            '\x1e';
  }
  else {
    if (0x13 < param_1) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Alg/Touch/PrevTouchData.c",0x23d,"preIdx < TSA_MAX_TOUCH_NUM");
      HardwareAnalyzer_SetSoftException();
    }
    bVar1 = false;
  }
  return bVar1;
}


// ============================================================================
// Function: PrevTouch_HistoryBufferRemove @ 6ba6f11a
// ============================================================================

void PrevTouch_HistoryBufferRemove(byte param_1,byte param_2)

{
  undefined8 uVar1;
  PrevTouchArray *pPVar2;
  longlong lVar3;
  longlong lVar4;
  byte local_9;
  
                    /* 0x2f11a  1059  PrevTouch_HistoryBufferRemove */
  if (param_1 < 0x14) {
    if (0x1e < *(byte *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288)) {
      if (0x1e < *(byte *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288))
      {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                          "../../src/Alg/Touch/PrevTouchData.c",0x21b,
                          "PrevTouchPrpt(preIdx, historyBuffer.num) <= PREVTOUCH_HISTORY_BUFFER_SIZE"
                         );
        HardwareAnalyzer_SetSoftException();
      }
      *(undefined1 *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288) =
           0x1e;
    }
    if (param_2 < *(byte *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288)
       ) {
      for (local_9 = param_2; pPVar2 = g_pPrevTouches,
          (int)(char)local_9 <
          (int)(*(byte *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288) -
               1); local_9 = local_9 + 1) {
        lVar4 = (longlong)(int)(uint)param_1 * 0x478 + ((longlong)(int)(char)local_9 + 0x14) * 0x10;
        lVar3 = (longlong)(int)(uint)param_1 * 0x478 + ((longlong)((char)local_9 + 1) + 0x14) * 0x10
        ;
        uVar1 = *(undefined8 *)((longlong)g_pPrevTouches + lVar3 + 0x158);
        *(undefined8 *)((longlong)g_pPrevTouches + lVar4 + 0x150) =
             *(undefined8 *)((longlong)g_pPrevTouches + lVar3 + 0x150);
        *(undefined8 *)((longlong)pPVar2 + lVar4 + 0x158) = uVar1;
      }
      *(char *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288) =
           *(char *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288) + -1;
    }
  }
  else if (0x13 < param_1) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/Touch/PrevTouchData.c",0x218,"preIdx < TSA_MAX_TOUCH_NUM");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: PrevTouch_IsAnyExceptionDetected @ 6ba6e898
// ============================================================================

undefined1 PrevTouch_IsAnyExceptionDetected(void)

{
  byte local_a;
  undefined1 local_9;
  
                    /* 0x2e898  1063  PrevTouch_IsAnyExceptionDetected */
  local_9 = 0;
  local_a = 0;
  do {
    if (g_pPrevTouches->bPrevTouchCount <= local_a) {
LAB_6ba6e8ed:
      if (g_pPrevTouches->nTouchCount != 0) {
        local_9 = 1;
      }
      return local_9;
    }
    if (*(int *)((longlong)g_pPrevTouches + (longlong)(int)(uint)local_a * 0x478 + 0x1ec) != 0) {
      local_9 = 1;
      goto LAB_6ba6e8ed;
    }
    local_a = local_a + 1;
  } while( true );
}


// ============================================================================
// Function: PrevTouch_HistoryQueuePush @ 6ba6e650
// ============================================================================

undefined8 PrevTouch_HistoryQueuePush(byte param_1,ushort param_2,ushort param_3)

{
  char cVar1;
  int iVar2;
  undefined8 uVar3;
  char local_9;
  
                    /* 0x2e650  1061  PrevTouch_HistoryQueuePush */
  local_9 = '\0';
  iVar2 = (int)*(char *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x475);
  if (*(char *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x474) != iVar2) {
    iVar2 = (iVar2 + 0xf) % 0x10;
    local_9 = PrevTouch_HistoryQueuePush_internal
                        (param_1,((uint)(param_2 >> 8) +
                                 (uint)*(byte *)((longlong)g_pPrevTouches +
                                                (longlong)iVar2 +
                                                (longlong)(int)(uint)param_1 * 0x478 + 0x476)) / 2,
                         ((uint)(param_3 >> 8) +
                         (uint)*(byte *)((longlong)g_pPrevTouches +
                                        (longlong)iVar2 + (longlong)(int)(uint)param_1 * 0x478 +
                                        0x486)) / 2);
  }
  cVar1 = PrevTouch_HistoryQueuePush_internal(param_1,param_2 >> 8,param_3 >> 8);
  if ((local_9 == '\0') && (cVar1 == '\0')) {
    uVar3 = 0;
  }
  else {
    uVar3 = 1;
  }
  return uVar3;
}


// ============================================================================
// Function: PrevTouch_HistoryQueuePush_internal @ 6ba6e448
// ============================================================================

undefined8 PrevTouch_HistoryQueuePush_internal(byte param_1,ushort param_2,ushort param_3)

{
  int iVar1;
  int iVar2;
  undefined8 uVar3;
  int iVar4;
  
  iVar1 = (int)*(char *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x474);
  iVar2 = (int)*(char *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x475);
  if (((iVar1 == iVar2) ||
      (iVar4 = (iVar2 + 0xf) % 0x10,
      *(byte *)((longlong)g_pPrevTouches +
               (longlong)iVar4 + (longlong)(int)(uint)param_1 * 0x478 + 0x476) != param_2)) ||
     (*(byte *)((longlong)g_pPrevTouches +
               (longlong)iVar4 + (longlong)(int)(uint)param_1 * 0x478 + 0x486) != param_3)) {
    *(char *)((longlong)g_pPrevTouches +
             (longlong)iVar2 + (longlong)(int)(uint)param_1 * 0x478 + 0x476) = (char)param_2;
    *(char *)((longlong)g_pPrevTouches +
             (longlong)iVar2 + (longlong)(int)(uint)param_1 * 0x478 + 0x486) = (char)param_3;
    *(char *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x475) =
         (char)((iVar2 + 1) % 0x10);
    if (*(char *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x475) == iVar1)
    {
      *(char *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x474) =
           (char)((iVar1 + 1) % 0x10);
    }
    uVar3 = 1;
  }
  else {
    uVar3 = 0;
  }
  return uVar3;
}


