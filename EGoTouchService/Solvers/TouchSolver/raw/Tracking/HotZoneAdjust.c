#include "TSACoreInternal.h"

// ============================================================================
// Function: HotZoneAdjust_Process @ 6baf729e
// ============================================================================

void HotZoneAdjust_Process(byte param_1,byte param_2)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0xb729e  883  HotZoneAdjust_Process */
  cVar1 = HotZoneAdjustIsFeatureOn();
  if (cVar1 != '\0') {
    cVar1 = HotZoneAdjustScenario();
    if (cVar1 == '\0') {
      ClearHotZone();
    }
    else {
      if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1f8) & 8) != 0) {
        if (((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_2 * 0x478 + 0x1f8) & 8) == 0) ||
           (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x1f8) & 0x20000) != 0 &&
            ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_2 * 0x478 + 0x1f8) & 0x20000) == 0)))) {
          HotZoneAdjustProcessFirst(param_1);
        }
        else {
          HotZoneAdjustProcessAfter(param_1,param_2);
        }
      }
      if (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1f8) & 8) == 0) &&
         ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_2 * 0x478 + 0x1f8) & 8) != 0)) {
        cVar1 = IsNeedRecord(param_2,&g_hotZoneAnalyzer);
        if (cVar1 != '\0') {
          HotZoneRecord(param_2,&g_hotZoneAnalyzer);
        }
        StrPrintf_Process(0,"\nAppLog: ");
        uVar2 = GenerateAndGetDetailDescription();
        StrPrintf_Process(0x10,&DAT_6bb5814a,uVar2);
        ClearHotZone();
      }
    }
  }
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
// Function: HotZoneAdjust_GenerateAndGetDmdDescription @ 6baf6d70
// ============================================================================

undefined * HotZoneAdjust_GenerateAndGetDmdDescription(void)

{
                    /* 0xb6d70  881  HotZoneAdjust_GenerateAndGetDmdDescription */
  UpdateEndTime(&g_hotZoneAnalyzer);
  GenerateDmdDescription();
  ClearStatisticClickNum(&g_hotZoneAnalyzer);
  UpdateStartTime(&g_hotZoneAnalyzer);
  return &DAT_6bba3788;
}



// ============================================================================
// Function: ClearHotZone @ 6baf3be8
// ============================================================================

void ClearHotZone(void)

{
  memset_s(&g_hotZoneRect,700,0,700);
  StrPrintf_Process(0,"\nAppLog: ");
  StrPrintf_Process(0x10,"\nHotZone cleared");
  return;
}


// ============================================================================
// Function: HotZoneAdjustIsFeatureOn @ 6baf404a
// ============================================================================

undefined4 HotZoneAdjustIsFeatureOn(void)

{
  TSA_FeatureFlags TVar1;
  
  TVar1 = (*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x400000;
  return CONCAT31((int3)(TVar1 >> 8),TVar1 != 0);
}


// ============================================================================
// Function: HotZoneAdjustScenario @ 6baf406a
// ============================================================================

undefined8 HotZoneAdjustScenario(void)

{
  char cVar1;
  undefined8 uVar2;
  
  cVar1 = TSA_IsTypingScenario();
  if (cVar1 == '\0') {
    cVar1 = TSAStatic_IsScreenRotated();
    if (cVar1 == '\0') {
      uVar2 = 1;
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
// Function: HotZoneAdjustProcessFirst @ 6baf5d93
// ============================================================================

void HotZoneAdjustProcessFirst(byte param_1)

{
  undefined1 local_18 [15];
  byte local_9;
  
  memset_s(local_18,0xe,0,0xe);
  local_9 = UpdateHotZonePosition(param_1,local_18);
  if (local_9 == 1) {
    StrPrintf_Process(0,"\nAppLog: ");
    StrPrintf_Process(0x10,"\nHotZone Id=%d, NO Widgets",
                      *(undefined1 *)
                       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x154));
    return;
  }
  if (local_9 < 2) {
    if (local_9 == 0) {
      HotZoneAdjustProcessFirstUnit(param_1,local_18);
      return;
    }
  }
  else {
    if (local_9 == 2) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(0x10,"\nHotZone Id=%d, Too close",
                        *(undefined1 *)
                         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x154));
      return;
    }
    if (local_9 == 3) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(0x10,"\nHotZone Id=%d, In core area",
                        *(undefined1 *)
                         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x154));
      return;
    }
  }
  StrPrintf_Process(0,"\nAppLog: ");
  StrPrintf_Process(0x10,"\nHotZone Id=%d, Default not adjust",
                    *(undefined1 *)
                     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x154));
  return;
}


// ============================================================================
// Function: HotZoneAdjustProcessAfter @ 6baf5f50
// ============================================================================

void HotZoneAdjustProcessAfter(byte param_1,byte param_2)

{
  undefined2 uVar1;
  undefined2 uVar2;
  undefined2 uVar3;
  undefined2 uVar4;
  short sVar5;
  short sVar6;
  
  sVar6 = *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x170);
  sVar5 = *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x172);
  if ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x200)
      & 8) != 0) {
    AdjustByHistory(param_1,param_2);
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x200) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x200)
         | 8;
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      uVar1 = TSAOut_Dim2PosInternal
                        (*(undefined2 *)
                          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x172),1);
      uVar2 = TSAOut_Dim1PosInternal
                        (*(undefined2 *)
                          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x170),1);
      uVar3 = TSAOut_Dim2PosInternal(sVar5,1);
      uVar4 = TSAOut_Dim1PosInternal(sVar6,1);
      StrPrintf_Process(0,"\nHotZone Id=%d, Adjust OK before %d#%d, after %d#%d\n",
                        *(undefined1 *)
                         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x154),uVar4,uVar3,uVar2,uVar1);
    }
    StrPrintf_Process(0,"\nAppLog: ");
    sVar5 = TSAOut_IntToExtDist((int)(short)(*(short *)((longlong)(int)(uint)param_1 * 0x168 +
                                                        *(longlong *)_refptr_g_touchesPtr + 0x172) -
                                            sVar5));
    sVar6 = TSAOut_IntToExtDist((int)(short)(*(short *)((longlong)(int)(uint)param_1 * 0x168 +
                                                        *(longlong *)_refptr_g_touchesPtr + 0x170) -
                                            sVar6));
    StrPrintf_Process(0x10,"\nHotZone Id=%d, Adjust OK %d#%d\n",
                      *(undefined1 *)
                       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x154),(int)sVar6,(int)sVar5);
  }
  return;
}


// ============================================================================
// Function: GenerateAndGetDetailDescription @ 6baf6d4a
// ============================================================================

undefined * GenerateAndGetDetailDescription(void)

{
  GenerateDetailDescription();
  ClearDiffNum(&g_hotZoneAnalyzer);
  return &DAT_6bba3590;
}


// ============================================================================
// Function: IsNeedRecord @ 6baf6dae
// ============================================================================

undefined8 IsNeedRecord(byte param_1,longlong param_2)

{
  short sVar1;
  char cVar2;
  ushort uVar3;
  ushort uVar4;
  ushort uVar5;
  ushort *puVar6;
  
  param_2 = param_2 + (longlong)
                      (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                          (longlong)(int)(uint)param_1 * 0x478 + 0x154) * 0xe;
  puVar6 = (ushort *)(param_2 + 0x412);
  if ((((*puVar6 != 0) || (*(short *)(param_2 + 0x416) != 0)) || (*(short *)(param_2 + 0x414) != 0))
     || (*(short *)(param_2 + 0x418) != 0)) {
    if ((*(int *)&_refptr_g_tsaPrmtFlash[2].field_0x7b8 == 0) &&
       (cVar2 = HotZoneIsInCore(puVar6), cVar2 != '\0')) {
      return 0;
    }
    sVar1 = *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_1 * 0x478 + 0x4ac);
    uVar3 = TSAOut_ExtToIntDist(100);
    if ((((int)(uint)*puVar6 <= (int)((int)sVar1 + (uint)uVar3)) &&
        (sVar1 = *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                           (longlong)(int)(uint)param_1 * 0x478 + 0x4ac),
        uVar3 = *(ushort *)(param_2 + 0x416), uVar4 = TSAOut_ExtToIntDist(100),
        (int)sVar1 <= (int)((uint)uVar4 + (uint)uVar3))) &&
       ((sVar1 = *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                           (longlong)(int)(uint)param_1 * 0x478 + 0x4ae),
        uVar3 = TSAOut_ExtToIntDist(100),
        (int)(uint)*(ushort *)(param_2 + 0x414) <= (int)((int)sVar1 + (uint)uVar3) &&
        (sVar1 = *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                           (longlong)(int)(uint)param_1 * 0x478 + 0x4ae),
        uVar3 = *(ushort *)(param_2 + 0x418), uVar4 = TSAOut_ExtToIntDist(100),
        (int)sVar1 <= (int)((uint)uVar4 + (uint)uVar3))))) {
      uVar3 = *(ushort *)
               ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x16c);
      uVar4 = TSAOut_ExtToIntDist(100);
      if (((((uint)uVar3 + (uint)uVar4 < (uint)*puVar6) ||
           (uVar3 = *(ushort *)
                     ((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)param_1 * 0x478 + 0x16c),
           uVar4 = *(ushort *)(param_2 + 0x416), uVar5 = TSAOut_ExtToIntDist(100),
           (uint)uVar5 + (uint)uVar4 < (uint)uVar3)) ||
          (uVar3 = *(ushort *)
                    ((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_1 * 0x478 + 0x16e), uVar4 = TSAOut_ExtToIntDist(100),
          (uint)uVar3 + (uint)uVar4 < (uint)*(ushort *)(param_2 + 0x414))) ||
         (uVar3 = *(ushort *)
                   ((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_1 * 0x478 + 0x16e),
         uVar4 = *(ushort *)(param_2 + 0x418), uVar5 = TSAOut_ExtToIntDist(100),
         (uint)uVar5 + (uint)uVar4 < (uint)uVar3)) {
        return 0;
      }
      uVar3 = *(ushort *)
               ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x16c);
      sVar1 = *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                        (longlong)(int)(uint)param_1 * 0x478 + 0x4ac);
      uVar4 = TSAOut_ExtToIntDist(10);
      if ((((int)(uint)uVar3 <= (int)((uint)uVar4 + (int)sVar1)) &&
          (uVar3 = *(ushort *)
                    ((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_1 * 0x478 + 0x16e),
          sVar1 = *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                            (longlong)(int)(uint)param_1 * 0x478 + 0x4ae),
          uVar4 = TSAOut_ExtToIntDist(10), (int)(uint)uVar3 <= (int)((uint)uVar4 + (int)sVar1))) &&
         ((sVar1 = *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)param_1 * 0x478 + 0x4ac),
          uVar3 = *(ushort *)
                   ((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_1 * 0x478 + 0x16c), uVar4 = TSAOut_ExtToIntDist(10),
          (int)sVar1 <= (int)((uint)uVar4 + (uint)uVar3) &&
          (sVar1 = *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)param_1 * 0x478 + 0x4ae),
          uVar3 = *(ushort *)
                   ((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_1 * 0x478 + 0x16e), uVar4 = TSAOut_ExtToIntDist(10),
          (int)sVar1 <= (int)((uint)uVar4 + (uint)uVar3))))) {
        return 1;
      }
      return 0;
    }
  }
  return 0;
}


// ============================================================================
// Function: HotZoneRecord @ 6baf726b
// ============================================================================

void HotZoneRecord(undefined1 param_1,undefined8 param_2)

{
  CalculateAppIndex(param_2);
  CalculateSuccessNum(param_1,param_2);
  return;
}


// ============================================================================
// Function: UpdateHotZonePosition @ 6baf522e
// ============================================================================

undefined8 UpdateHotZonePosition(byte param_1,undefined8 *param_2)

{
  char cVar1;
  undefined8 uVar2;
  longlong lVar3;
  byte local_2c9;
  undefined8 local_2c8;
  undefined4 auStack_2bc [172];
  byte local_9;
  
  local_9 = *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x154);
  memset_s(&DAT_6bba3992 + (longlong)(int)(uint)local_9 * 0xe,0xe,0,0xe);
  memset_s(&local_2c8,700,0,700);
  CalculateInternalHotZone(&local_2c8,0x32);
  cVar1 = HasHotZone();
  if (cVar1 == '\0') {
    uVar2 = 1;
  }
  else {
    local_2c9 = 0;
    cVar1 = SearchNearest(param_1,&local_2c8,0x32,&local_2c9);
    if (cVar1 == '\0') {
      uVar2 = 2;
    }
    else {
      lVar3 = (longlong)(int)(uint)local_2c9;
      *param_2 = *(undefined8 *)((longlong)&local_2c8 + lVar3 * 0xe);
      *(undefined4 *)(param_2 + 1) = *(undefined4 *)((longlong)auStack_2bc + (lVar3 * 7 + -2) * 2);
      *(undefined2 *)((longlong)param_2 + 0xc) =
           *(undefined2 *)((longlong)auStack_2bc + lVar3 * 0xe);
      CalculateHotZoneIdx(param_2);
      if ((*(int *)&_refptr_g_tsaPrmtFlash[2].field_0x7b8 == 0) &&
         (cVar1 = HotZoneIsInCore(param_2), cVar1 != '\0')) {
        return 3;
      }
      lVar3 = (longlong)(int)(uint)local_9 * 0xe;
      *(undefined8 *)(&DAT_6bba3992 + lVar3) = *param_2;
      *(undefined4 *)(&DAT_6bba399a + lVar3) = *(undefined4 *)(param_2 + 1);
      *(undefined2 *)(&DAT_6bba399e + lVar3) = *(undefined2 *)((longlong)param_2 + 0xc);
      uVar2 = 0;
    }
  }
  return uVar2;
}


// ============================================================================
// Function: HotZoneAdjustProcessFirstUnit @ 6baf5c13
// ============================================================================

void HotZoneAdjustProcessFirstUnit(byte param_1,undefined2 *param_2)

{
  undefined1 local_78 [32];
  ushort local_58;
  ushort local_56;
  ushort local_54;
  ushort local_52;
  int local_50;
  int local_4c;
  
  CalculateExpandZone(param_2,&local_58);
  CalculateTargetZone(param_2,&local_58,local_78);
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,
                      "\nHotZone Id=%d, Widget:%d#%d#%d#%d, Index:%d#%d, Size:%ddp, ExpandWidget:%d#%d#%d#%d"
                      ,*(undefined1 *)
                        ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x154),*param_2,param_2[1],param_2[2],param_2[3],param_2[4],param_2[5],
                      ((ushort)param_2[6] + 1) * 8,local_50 - (uint)local_58,
                      local_4c - (uint)local_56,local_50 + (uint)local_54,local_4c + (uint)local_52)
    ;
  }
  HotZoneAdjustPoint(param_1,&local_58,local_78);
  return;
}


// ============================================================================
// Function: AdjustByHistory @ 6baf540d
// ============================================================================

void AdjustByHistory(byte param_1,byte param_2)

{
  if ((int)(*(int *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_2 * 0x478 + 0x174) +
           (uint)*(ushort *)
                  ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170)
           ) < 1) {
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x174) =
         -(uint)*(ushort *)
                 ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170);
  }
  else if ((int)((uint)*(ushort *)
                        ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x170) +
                *(int *)((longlong)*_refptr_g_prevTouchesPtr +
                        (longlong)(int)(uint)param_2 * 0x478 + 0x174)) <
           (int)(*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x20) - 1)) {
    *(undefined4 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x174) =
         *(undefined4 *)
          ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x174);
  }
  else {
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x174) =
         (*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x20) - 1) -
         (uint)*(ushort *)
                ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170);
  }
  *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170) =
       *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170)
       + (short)*(undefined4 *)
                 ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x174);
  if ((int)(*(int *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_2 * 0x478 + 0x178) +
           (uint)*(ushort *)
                  ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172)
           ) < 1) {
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x178) =
         -(uint)*(ushort *)
                 ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
  }
  else if ((int)((uint)*(ushort *)
                        ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x172) +
                *(int *)((longlong)*_refptr_g_prevTouchesPtr +
                        (longlong)(int)(uint)param_2 * 0x478 + 0x178)) <
           (int)(*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x22) - 1)) {
    *(undefined4 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x178) =
         *(undefined4 *)
          ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x178);
  }
  else {
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x178) =
         (*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x22) - 1) -
         (uint)*(ushort *)
                ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
  }
  *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172) =
       *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172)
       + (short)*(undefined4 *)
                 ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x178);
  return;
}


// ============================================================================
// Function: ClearDiffNum @ 6baf6268
// ============================================================================

void ClearDiffNum(longlong param_1)

{
  byte local_9;
  
  for (local_9 = 0; local_9 < 6; local_9 = local_9 + 1) {
    *(undefined4 *)((longlong)(int)(uint)local_9 * 0x18 + param_1 + 0x65c) = 0;
    *(undefined4 *)((longlong)(int)(uint)local_9 * 0x18 + param_1 + 0x660) = 0;
    *(undefined4 *)((longlong)(int)(uint)local_9 * 0x18 + param_1 + 0x664) = 0;
  }
  return;
}


// ============================================================================
// Function: GenerateDetailDescription @ 6baf6a1d
// ============================================================================

void GenerateDetailDescription(void)

{
  byte local_19;
  
  ClearDetailDescription();
  StrPrintf_Backup();
  StrPrintf_Init(DetailDescriptionPutStr);
  for (local_19 = 0; local_19 < 6; local_19 = local_19 + 1) {
    if (*(int *)(&DAT_6bba3bdc + (longlong)(int)(uint)local_19 * 0x18) == 0) {
      StrPrintf_Process(0,"\nHotZone %s total=%d, origin=%d, optimize=%d",
                        (&g_appList)[(int)(uint)local_19],
                        *(undefined4 *)(&DAT_6bba3bd0 + (longlong)(int)(uint)local_19 * 0x18),
                        *(undefined4 *)(&DAT_6bba3bd4 + (longlong)(int)(uint)local_19 * 0x18),
                        *(undefined4 *)(&DAT_6bba3bd8 + (longlong)(int)(uint)local_19 * 0x18));
    }
    else {
      StrPrintf_Process(0,"\nHotZone %s total=%d(+%d), origin=%d(+%d), optimize=%d(+%d)",
                        (&g_appList)[(int)(uint)local_19],
                        *(undefined4 *)(&DAT_6bba3bd0 + (longlong)(int)(uint)local_19 * 0x18),
                        *(undefined4 *)(&DAT_6bba3bdc + (longlong)(int)(uint)local_19 * 0x18),
                        *(undefined4 *)(&DAT_6bba3bd4 + (longlong)(int)(uint)local_19 * 0x18),
                        *(undefined4 *)(&DAT_6bba3be0 + (longlong)(int)(uint)local_19 * 0x18),
                        *(undefined4 *)(&DAT_6bba3bd8 + (longlong)(int)(uint)local_19 * 0x18),
                        *(undefined4 *)(&DAT_6bba3be4 + (longlong)(int)(uint)local_19 * 0x18));
    }
  }
  StrPrintf_Retore();
  return;
}


// ============================================================================
// Function: HotZoneIsInCore @ 6baf51b8
// ============================================================================

undefined8 HotZoneIsInCore(longlong param_1)

{
  undefined8 uVar1;
  
  if ((((*(short *)(param_1 + 8) == 0) ||
       ((int)((byte)_refptr_g_tsaPrmtFlash[2].field_0x7bd - 1) <=
        (int)(uint)*(ushort *)(param_1 + 8))) || (*(short *)(param_1 + 10) == 0)) ||
     ((int)((byte)_refptr_g_tsaPrmtFlash[2].field_0x7be - 1) <= (int)(uint)*(ushort *)(param_1 + 10)
     )) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: CalculateAppIndex @ 6baf61ed
// ============================================================================

void CalculateAppIndex(longlong param_1)

{
  int iVar1;
  byte local_9;
  
  *(undefined1 *)(param_1 + 0x410) = 6;
  for (local_9 = 1; local_9 < 6; local_9 = local_9 + 1) {
    if (*(longlong *)(param_1 + 0x400) != 0) {
      iVar1 = strcmp(*(char **)(param_1 + 0x400),(&g_appList)[(int)(uint)local_9]);
      if (iVar1 == 0) {
        *(byte *)(param_1 + 0x410) = local_9;
      }
    }
  }
  return;
}


// ============================================================================
// Function: CalculateSuccessNum @ 6baf6380
// ============================================================================

void CalculateSuccessNum(byte param_1,longlong param_2)

{
  byte bVar1;
  char cVar2;
  longlong lVar3;
  
  bVar1 = *(byte *)(param_2 + 0x410);
  lVar3 = param_2 + (longlong)
                    (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                        (longlong)(int)(uint)param_1 * 0x478 + 0x154) * 0xe + 0x412;
  *(int *)(param_2 + 0x650) = *(int *)(param_2 + 0x650) + 1;
  *(int *)(param_2 + 0x65c) = *(int *)(param_2 + 0x65c) + 1;
  if (bVar1 < 6) {
    *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x650) =
         *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x650) + 1;
    *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x65c) =
         *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x65c) + 1;
  }
  cVar2 = IsInHotZone(*(undefined2 *)
                       ((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_1 * 0x478 + 0x16c),
                      *(undefined2 *)
                       ((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_1 * 0x478 + 0x16e),lVar3);
  if ((cVar2 == '\0') ||
     (cVar2 = IsInHotZone(*(undefined2 *)
                           ((longlong)*_refptr_g_prevTouchesPtr +
                           (longlong)(int)(uint)param_1 * 0x478 + 0x4ac),
                          *(undefined2 *)
                           ((longlong)*_refptr_g_prevTouchesPtr +
                           (longlong)(int)(uint)param_1 * 0x478 + 0x4ae),lVar3), cVar2 == '\0')) {
    if (((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_1 * 0x478 + 0x200) & 8) != 0) &&
       (cVar2 = IsInHotZone(*(undefined2 *)
                             ((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)param_1 * 0x478 + 0x170),
                            *(undefined2 *)
                             ((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)param_1 * 0x478 + 0x172),lVar3), cVar2 != '\0')) {
      *(int *)(param_2 + 0x658) = *(int *)(param_2 + 0x658) + 1;
      *(int *)(param_2 + 0x664) = *(int *)(param_2 + 0x664) + 1;
      if (bVar1 < 6) {
        *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x658) =
             *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x658) + 1;
        *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x664) =
             *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x664) + 1;
      }
    }
  }
  else {
    *(int *)(param_2 + 0x654) = *(int *)(param_2 + 0x654) + 1;
    *(int *)(param_2 + 0x660) = *(int *)(param_2 + 0x660) + 1;
    *(int *)(param_2 + 0x658) = *(int *)(param_2 + 0x658) + 1;
    *(int *)(param_2 + 0x664) = *(int *)(param_2 + 0x664) + 1;
    if (bVar1 < 6) {
      *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x654) =
           *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x654) + 1;
      *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x660) =
           *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x660) + 1;
      *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x658) =
           *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x658) + 1;
      *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x664) =
           *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x664) + 1;
    }
  }
  return;
}


// ============================================================================
// Function: HotZoneAdjustClearAnalyzer @ 6baf3fb1
// ============================================================================

void HotZoneAdjustClearAnalyzer(void)

{
  memset_s(&g_hotZoneAnalyzer,0x6e0,0,0x6e0);
  return;
}


// ============================================================================
// Function: HasHotZone @ 6baf4008
// ============================================================================

undefined8 HasHotZone(void)

{
  undefined8 uVar1;
  
  if ((((g_hotZoneRect == 0) && (DAT_6bba3c84 == 0)) && (DAT_6bba3c82 == 0)) && (DAT_6bba3c86 == 0))
  {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: CalculateInternalHotZone @ 6baf4f8a
// ============================================================================

void CalculateInternalHotZone(undefined8 param_1,undefined1 param_2)

{
  ExtToInt(param_1,param_2,&g_hotZoneRect,0x32);
  return;
}


// ============================================================================
// Function: SearchNearest @ 6baf4fbe
// ============================================================================

bool SearchNearest(byte param_1,undefined8 param_2,undefined1 param_3,undefined8 param_4)

{
  ushort uVar1;
  ushort local_12;
  ushort local_10 [2];
  undefined2 local_c;
  undefined2 local_a;
  
  local_a = *(undefined2 *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170);
  local_c = *(undefined2 *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
  local_10[0] = 0xffff;
  local_12 = 0xffff;
  SearchNearestDisTwo(local_a,local_c,param_2,param_3,local_10,&local_12,param_4);
  uVar1 = TSAOut_ExtToIntDist(_refptr_g_tsaPrmtFlash[2].field_0x7bc);
  return (int)(uint)uVar1 < (int)((uint)local_12 - (uint)local_10[0]);
}


// ============================================================================
// Function: CalculateHotZoneIdx @ 6baf50ad
// ============================================================================

void CalculateHotZoneIdx(ushort *param_1)

{
  ushort uVar1;
  
  param_1[4] = (ushort)((ulonglong)
                        (ushort)((short)((int)((uint)param_1[2] - (uint)*param_1) / 2) + *param_1) /
                       (ulonglong)
                       (longlong)
                       (int)((ulonglong)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x20) /
                            (ulonglong)
                            (longlong)(int)(uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x7bd));
  param_1[5] = (ushort)((ulonglong)
                        (ushort)((short)((int)((uint)param_1[3] - (uint)param_1[1]) / 2) +
                                param_1[1]) /
                       (ulonglong)
                       (longlong)
                       (int)((ulonglong)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x22) /
                            (ulonglong)
                            (longlong)(int)(uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x7be));
  uVar1 = CalculateSizeIndex(param_1);
  param_1[6] = uVar1;
  return;
}


// ============================================================================
// Function: CalculateExpandZone @ 6baf480c
// ============================================================================

void CalculateExpandZone(short *param_1,ushort *param_2)

{
  byte bVar1;
  short sVar2;
  short sVar3;
  ushort uVar4;
  ushort uVar5;
  ushort uVar6;
  ushort uVar7;
  ushort uVar8;
  ushort uVar9;
  ushort uVar10;
  ushort uVar11;
  ushort uVar12;
  ushort uVar13;
  ushort uVar14;
  ushort uVar15;
  ushort uVar16;
  ushort uVar17;
  
  uVar4 = (ushort)(param_1[2] - *param_1) >> 1;
  uVar5 = (ushort)(param_1[3] - param_1[1]) >> 1;
  sVar2 = *param_1;
  sVar3 = param_1[1];
  uVar16 = param_1[4];
  uVar17 = param_1[5];
  uVar15 = param_1[6];
  uVar6 = TSAOut_ExtToIntDist(*(undefined2 *)
                               (&_refptr_g_tsaPrmtFlash[2].field_0x7c0 +
                               (longlong)(int)(uint)uVar15 * 2));
  bVar1 = _refptr_g_tsaPrmtFlash[2].field_0x7bf;
  uVar7 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x7d4);
  uVar8 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x7d6);
  uVar9 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x7d8);
  uVar10 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x7da);
  uVar11 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x7dc);
  uVar12 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x7de);
  uVar13 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x7e0);
  uVar14 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x7e2);
  uVar11 = uVar11 - (short)((int)((uint)uVar15 * ((uint)uVar11 - (uint)uVar7)) / (int)(bVar1 - 1));
  uVar12 = uVar12 - (short)((int)((uint)uVar15 * ((uint)uVar12 - (uint)uVar8)) / (int)(bVar1 - 1));
  uVar13 = uVar13 - (short)((int)((uint)uVar15 * ((uint)uVar13 - (uint)uVar9)) / (int)(bVar1 - 1));
  uVar14 = uVar14 - (short)((int)((uint)uVar15 * ((uint)uVar14 - (uint)uVar10)) / (int)(bVar1 - 1));
  uVar15 = uVar11 + (short)((int)((uint)uVar16 * ((uint)uVar12 - (uint)uVar11)) /
                           (int)((byte)_refptr_g_tsaPrmtFlash[2].field_0x7bd - 1)) + (uVar6 >> 1);
  uVar16 = (uVar12 - (short)((int)((uint)uVar16 * ((uint)uVar12 - (uint)uVar11)) /
                            (int)((byte)_refptr_g_tsaPrmtFlash[2].field_0x7bd - 1))) + (uVar6 >> 1);
  uVar7 = uVar13 + (short)((int)((uint)uVar17 * ((uint)uVar14 - (uint)uVar13)) /
                          (int)((byte)_refptr_g_tsaPrmtFlash[2].field_0x7be - 1)) + (uVar6 >> 1);
  uVar17 = (uVar14 - (short)((int)((uint)uVar17 * ((uint)uVar14 - (uint)uVar13)) /
                            (int)((byte)_refptr_g_tsaPrmtFlash[2].field_0x7be - 1))) + (uVar6 >> 1);
  if (uVar15 <= uVar4) {
    uVar15 = uVar4;
  }
  *param_2 = uVar15;
  if (uVar16 <= uVar4) {
    uVar16 = uVar4;
  }
  param_2[2] = uVar16;
  if (uVar7 <= uVar5) {
    uVar7 = uVar5;
  }
  param_2[1] = uVar7;
  if (uVar17 <= uVar5) {
    uVar17 = uVar5;
  }
  param_2[3] = uVar17;
  param_2[8] = *param_2 + param_2[2];
  param_2[9] = param_2[1] + param_2[3];
  *(uint *)(param_2 + 4) =
       ((uint)(ushort)(uVar4 + sVar2) + (uint)(param_2[8] >> 1)) - (uint)*param_2;
  *(uint *)(param_2 + 6) =
       ((uint)(ushort)(uVar5 + sVar3) + (uint)(param_2[9] >> 1)) - (uint)param_2[1];
  return;
}


// ============================================================================
// Function: CalculateTargetZone @ 6baf4d02
// ============================================================================

void CalculateTargetZone(ushort *param_1,longlong param_2,longlong param_3)

{
  ushort uVar1;
  ushort uVar2;
  ushort uVar3;
  ushort uVar4;
  
  uVar1 = param_1[2];
  uVar2 = *param_1;
  uVar3 = param_1[3];
  uVar4 = param_1[1];
  CalculateTargetZoneLen
            (*(undefined2 *)(param_2 + 0x10),*(undefined2 *)(param_2 + 0x12),uVar1 - uVar2,
             uVar3 - uVar4,param_3 + 0x10,param_3 + 0x12);
  *(uint *)(param_3 + 8) = (uint)*param_1 + (uint)((ushort)(uVar1 - uVar2) >> 1);
  *(uint *)(param_3 + 0xc) = (uint)param_1[1] + (uint)((ushort)(uVar3 - uVar4) >> 1);
  return;
}


// ============================================================================
// Function: HotZoneAdjustPoint @ 6baf5859
// ============================================================================

void HotZoneAdjustPoint(byte param_1,undefined8 param_2,undefined8 param_3)

{
  char cVar1;
  undefined2 uVar2;
  ushort uVar3;
  ushort uVar4;
  undefined2 uVar5;
  short sVar6;
  short sVar7;
  ushort *puVar8;
  undefined4 uVar10;
  undefined8 uVar9;
  ushort *puVar11;
  undefined4 uVar12;
  ushort local_30;
  ushort local_2e;
  ushort local_2c;
  ushort local_2a;
  
  local_2a = *(ushort *)
              ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170);
  local_2c = *(ushort *)
              ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
  cVar1 = IsInAdjustArea(local_2a,local_2c,param_2);
  if (cVar1 == '\0') {
    StrPrintf_Process(0,"\nAppLog: ");
    StrPrintf_Process(0x10,"\nHotZone Id=%d, Not in the Expand Zone",
                      *(undefined1 *)
                       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x154));
  }
  else {
    puVar11 = &local_30;
    puVar8 = &local_2e;
    CalculateNewPosition(param_2,local_2a,local_2c,param_3,puVar8,puVar11);
    uVar10 = (undefined4)((ulonglong)puVar8 >> 0x20);
    uVar12 = (undefined4)((ulonglong)puVar11 >> 0x20);
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x174) =
         (uint)local_2e - (uint)local_2a;
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x178) =
         (uint)local_30 - (uint)local_2c;
    *(ushort *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170) =
         local_2e;
    *(ushort *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172) =
         local_30;
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x200) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x200)
         | 8;
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      uVar2 = TSAOut_Dim2PosInternal
                        (*(undefined2 *)
                          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x172),1);
      uVar3 = TSAOut_Dim1PosInternal
                        (*(undefined2 *)
                          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x170),1);
      uVar4 = TSAOut_Dim2PosInternal(local_2c,1);
      uVar5 = TSAOut_Dim1PosInternal(local_2a,1);
      uVar9 = CONCAT44(uVar10,(uint)uVar4);
      StrPrintf_Process(0,"\nHotZone Id=%d, Adjust OK before %d#%d, after %d#%d\n",
                        *(undefined1 *)
                         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x154),uVar5,uVar9,CONCAT44(uVar12,(uint)uVar3),uVar2);
      uVar10 = (undefined4)((ulonglong)uVar9 >> 0x20);
    }
    StrPrintf_Process(0,"\nAppLog: ");
    sVar6 = TSAOut_IntToExtDist((int)(short)(*(short *)((longlong)(int)(uint)param_1 * 0x168 +
                                                        *(longlong *)_refptr_g_touchesPtr + 0x172) -
                                            local_2c));
    sVar7 = TSAOut_IntToExtDist((int)(short)(*(short *)((longlong)(int)(uint)param_1 * 0x168 +
                                                        *(longlong *)_refptr_g_touchesPtr + 0x170) -
                                            local_2a));
    StrPrintf_Process(0x10,"\nHotZone Id=%d, Adjust OK %d#%d\n",
                      *(undefined1 *)
                       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x154),(int)sVar7,CONCAT44(uVar10,(int)sVar6));
  }
  return;
}


// ============================================================================
// Function: ClearDetailDescription @ 6baf69b5
// ============================================================================

void ClearDetailDescription(void)

{
  memset_s(&DAT_6bba3590,0x1f5,0,0x1f5);
  DAT_6bba3786 = 0;
  return;
}


// ============================================================================
// Function: DetailDescriptionPutStr @ 6baf6865
// ============================================================================

void DetailDescriptionPutStr(longlong param_1)

{
  uint uVar1;
  ushort local_a;
  
  if (param_1 != 0) {
    local_a = 0;
    while ((*(char *)(param_1 + (ulonglong)local_a) != '\0' && (DAT_6bba3786 < 500))) {
      uVar1 = (uint)DAT_6bba3786;
      DAT_6bba3786 = DAT_6bba3786 + 1;
      (&DAT_6bba3590)[(int)uVar1] = *(undefined1 *)(param_1 + (ulonglong)local_a);
      local_a = local_a + 1;
    }
    if (DAT_6bba3786 < 0x1f5) {
      (&DAT_6bba3590)[(int)(uint)DAT_6bba3786] = 0;
    }
  }
  return;
}


// ============================================================================
// Function: IsInHotZone @ 6baf4e98
// ============================================================================

undefined8 IsInHotZone(ushort param_1,ushort param_2,ushort *param_3)

{
  undefined8 uVar1;
  
  if ((((param_1 < *param_3) || (param_3[2] < param_1)) || (param_2 < param_3[1])) ||
     (param_3[3] < param_2)) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: ExtToInt @ 6baf409d
// ============================================================================

void ExtToInt(longlong param_1,byte param_2,longlong param_3,byte param_4)

{
  undefined2 uVar1;
  byte local_19;
  
  local_19 = 0;
  while (((local_19 < param_2 && (local_19 < param_4)) &&
         ((*(short *)(param_3 + (ulonglong)local_19 * 0xe) != 0 ||
          (((*(short *)(param_3 + (ulonglong)local_19 * 0xe + 4) != 0 ||
            (*(short *)(param_3 + (ulonglong)local_19 * 0xe + 2) != 0)) ||
           (*(short *)(param_3 + (ulonglong)local_19 * 0xe + 6) != 0))))))) {
    uVar1 = TSAOut_RevertDim1Pos(*(undefined2 *)(param_3 + (ulonglong)local_19 * 0xe));
    *(undefined2 *)((ulonglong)local_19 * 0xe + param_1) = uVar1;
    uVar1 = TSAOut_RevertDim1Pos(*(undefined2 *)(param_3 + (ulonglong)local_19 * 0xe + 4));
    *(undefined2 *)((ulonglong)local_19 * 0xe + param_1 + 4) = uVar1;
    uVar1 = TSAOut_RevertDim2Pos(*(undefined2 *)(param_3 + (ulonglong)local_19 * 0xe + 2));
    *(undefined2 *)((ulonglong)local_19 * 0xe + param_1 + 2) = uVar1;
    uVar1 = TSAOut_RevertDim2Pos(*(undefined2 *)(param_3 + (ulonglong)local_19 * 0xe + 6));
    *(undefined2 *)((ulonglong)local_19 * 0xe + param_1 + 6) = uVar1;
    local_19 = local_19 + 1;
  }
  return;
}


// ============================================================================
// Function: SearchNearestDisTwo @ 6baf458d
// ============================================================================

void SearchNearestDisTwo(undefined2 param_1,undefined2 param_2,longlong param_3,byte param_4,
                        ushort *param_5,ushort *param_6,byte *param_7)

{
  ushort uVar1;
  byte local_9;
  
  local_9 = 0;
  while ((local_9 < param_4 &&
         ((((*(short *)(param_3 + (ulonglong)local_9 * 0xe) != 0 ||
            (*(short *)(param_3 + (ulonglong)local_9 * 0xe + 4) != 0)) ||
           (*(short *)(param_3 + (ulonglong)local_9 * 0xe + 2) != 0)) ||
          (*(short *)(param_3 + (ulonglong)local_9 * 0xe + 6) != 0))))) {
    uVar1 = CalculateDistance(param_3 + (ulonglong)local_9 * 0xe,param_1,param_2);
    if (uVar1 < *param_5) {
      *param_6 = *param_5;
      *param_5 = uVar1;
      *param_7 = local_9;
    }
    else if (uVar1 < *param_6) {
      *param_6 = uVar1;
    }
    local_9 = local_9 + 1;
  }
  return;
}


// ============================================================================
// Function: CalculateSizeIndex @ 6baf46dd
// ============================================================================

ushort CalculateSizeIndex(ushort *param_1)

{
  ushort uVar1;
  ushort uVar2;
  short sVar3;
  short sVar4;
  uint uVar5;
  uint uVar6;
  ushort local_20;
  ushort local_1e;
  uint local_1c;
  
  uVar1 = param_1[3];
  uVar2 = param_1[1];
  sVar3 = TSAOut_IntToExtDist((int)(short)((int)((uint)param_1[2] - (uint)*param_1) / 2));
  sVar4 = TSAOut_IntToExtDist((int)(short)((int)((uint)uVar1 - (uint)uVar2) / 2));
  uVar5 = (int)sVar4 * sVar3 * 4;
  local_1c = 0xffffffff;
  local_1e = 0;
  for (local_20 = 0; local_20 < (byte)_refptr_g_tsaPrmtFlash[2].field_0x7bf; local_20 = local_20 + 1
      ) {
    uVar1 = *(ushort *)(&_refptr_g_tsaPrmtFlash[2].field_0x7c0 + (longlong)(int)(uint)local_20 * 2);
    if ((uint)uVar1 * (uint)uVar1 < uVar5) {
      uVar6 = uVar5 - (uint)uVar1 * (uint)uVar1;
    }
    else {
      uVar6 = (uint)uVar1 * (uint)uVar1 - uVar5;
    }
    if (uVar6 < local_1c) {
      local_1e = local_20;
      local_1c = uVar6;
    }
  }
  return local_1e;
}


// ============================================================================
// Function: CalculateTargetZoneLen @ 6baf4c6c
// ============================================================================

void CalculateTargetZoneLen
               (ushort param_1,ushort param_2,ushort param_3,ushort param_4,ushort *param_5,
               ushort *param_6)

{
  ushort uVar1;
  ushort uVar2;
  ushort local_res8;
  ushort local_res10;
  
  local_res10 = param_2;
  if (param_2 == 0) {
    local_res10 = 1;
  }
  local_res8 = param_1;
  if (param_1 == 0) {
    local_res8 = 1;
  }
  uVar1 = (ushort)((int)((uint)local_res8 * (uint)param_4) / (int)(uint)local_res10);
  uVar2 = (ushort)((int)((uint)local_res10 * (uint)param_3) / (int)(uint)local_res8);
  if (uVar1 <= param_3) {
    param_3 = uVar1;
  }
  *param_5 = param_3;
  if (uVar2 <= param_4) {
    param_4 = uVar2;
  }
  *param_6 = param_4;
  return;
}


// ============================================================================
// Function: IsInAdjustArea @ 6baf4e28
// ============================================================================

undefined8 IsInAdjustArea(undefined2 param_1,undefined2 param_2,longlong param_3)

{
  char cVar1;
  
  cVar1 = IsInAdjustLine(param_1,*(undefined4 *)(param_3 + 8),*(undefined2 *)(param_3 + 0x10));
  if ((cVar1 != '\0') &&
     (cVar1 = IsInAdjustLine(param_2,*(undefined4 *)(param_3 + 0xc),*(undefined2 *)(param_3 + 0x12))
     , cVar1 != '\0')) {
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: CalculateNewPosition @ 6baf4eef
// ============================================================================

void CalculateNewPosition
               (longlong param_1,ushort param_2,ushort param_3,longlong param_4,short *param_5,
               short *param_6)

{
  *param_5 = (short)*(undefined4 *)(param_4 + 8) +
             (short)((int)(((uint)param_2 - *(int *)(param_1 + 8)) *
                          (uint)*(ushort *)(param_4 + 0x10)) /
                    (int)(uint)*(ushort *)(param_1 + 0x10));
  *param_6 = (short)*(undefined4 *)(param_4 + 0xc) +
             (short)((int)(((uint)param_3 - *(int *)(param_1 + 0xc)) *
                          (uint)*(ushort *)(param_4 + 0x12)) /
                    (int)(uint)*(ushort *)(param_1 + 0x12));
  return;
}


// ============================================================================
// Function: CalculateDistance @ 6baf4286
// ============================================================================

int CalculateDistance(ushort *param_1,ushort param_2,ushort param_3)
{
  ushort uVar1;
  ushort uVar2;
  ushort uVar3;
  ushort uVar4;
  int local_c;
  
  uVar1 = *param_1;
  uVar2 = param_1[2];
  uVar3 = param_1[1];
  uVar4 = param_1[3];
  local_c = 0;
  if ((param_2 < uVar1) && (param_3 < uVar3)) {
    local_c = ((uint)uVar3 - (uint)param_3) * ((uint)uVar3 - (uint)param_3) +
              ((uint)uVar1 - (uint)param_2) * ((uint)uVar1 - (uint)param_2);
  }
  else if ((param_2 < uVar1) || ((uVar2 < param_2 || (uVar3 <= param_3)))) {
    if ((uVar2 < param_2) && (param_3 < uVar3)) {
      local_c = ((uint)uVar3 - (uint)param_3) * ((uint)uVar3 - (uint)param_3) +
                ((uint)param_2 - (uint)uVar2) * ((uint)param_2 - (uint)uVar2);
    }
    else if (((param_2 < uVar1) && (uVar3 <= param_3)) && (param_3 <= uVar4)) {
      local_c = ((uint)uVar1 - (uint)param_2) * ((uint)uVar1 - (uint)param_2);
    }
    else if (((param_2 < uVar1) || (uVar2 < param_2)) || ((param_3 < uVar3 || (uVar4 < param_3)))) {
      if (((uVar2 < param_2) && (uVar3 <= param_3)) && (param_3 <= uVar4)) {
        local_c = ((uint)param_2 - (uint)uVar2) * ((uint)param_2 - (uint)uVar2);
      }
      else if ((param_2 < uVar1) && (uVar4 < param_3)) {
        local_c = ((uint)param_3 - (uint)uVar4) * ((uint)param_3 - (uint)uVar4) +
                  ((uint)uVar1 - (uint)param_2) * ((uint)uVar1 - (uint)param_2);
      }
      else if ((param_2 < uVar1) || ((uVar2 < param_2 || (param_3 <= uVar4)))) {
        if ((uVar2 < param_2) && (uVar4 < param_3)) {
          local_c = ((uint)param_3 - (uint)uVar4) * ((uint)param_3 - (uint)uVar4) +
                    ((uint)param_2 - (uint)uVar2) * ((uint)param_2 - (uint)uVar2);
        }
      }
      else {
        local_c = ((uint)param_3 - (uint)uVar4) * ((uint)param_3 - (uint)uVar4);
      }
    }
    else {
      local_c = 0;
    }
  }
  else {
    local_c = ((uint)uVar3 - (uint)param_3) * ((uint)uVar3 - (uint)param_3);
  }
  Misc_SqrtUint32(local_c);
  return;
}


// ============================================================================
// Function: IsInAdjustLine @ 6baf4dd6
// ============================================================================

undefined8 IsInAdjustLine(ushort param_1,int param_2,ushort param_3)

{
  undefined8 uVar1;
  
  if (((int)(param_2 - (uint)(param_3 >> 1)) < (int)(uint)param_1) &&
     ((int)(uint)param_1 < (int)(param_2 + (uint)(param_3 >> 1)))) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: ClearStatisticClickNum @ 6baf6305
// ============================================================================

void ClearStatisticClickNum(longlong param_1)

{
  memset_s(param_1 + 0x650,0x90,0,0x90);
  return;
}


// ============================================================================
// Function: UpdateStartTime @ 6baf633b
// ============================================================================

void UpdateStartTime(undefined8 *param_1)

{
  *param_1 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  return;
}


// ============================================================================
// Function: GenerateDmdDescription @ 6baf6c49
// ============================================================================

void GenerateDmdDescription(void)

{
  byte local_9;
  
  ClearDmdDescription();
  StrPrintf_Backup();
  StrPrintf_Init(DmdDescriptionPutStr);
  StrPrintf_Process(0,"HotZone, [%lld,%lld]",g_hotZoneAnalyzer,DAT_6bba3588);
  for (local_9 = 0; local_9 < 6; local_9 = local_9 + 1) {
    StrPrintf_Process(0,"[%d,%d,%d,%d]",local_9,
                      *(undefined4 *)(&DAT_6bba3bd0 + (longlong)(int)(uint)local_9 * 0x18),
                      *(undefined4 *)(&DAT_6bba3bd4 + (longlong)(int)(uint)local_9 * 0x18),
                      *(undefined4 *)(&DAT_6bba3bd8 + (longlong)(int)(uint)local_9 * 0x18));
  }
  StrPrintf_Process(0,&DAT_6bb58148);
  StrPrintf_Retore();
  return;
}


// ============================================================================
// Function: UpdateEndTime @ 6baf635d
// ============================================================================

void UpdateEndTime(longlong param_1)

{
  *(void **)(param_1 + 8) = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  return;
}


// ============================================================================
// Function: ClearDmdDescription @ 6baf69e9
// ============================================================================

void ClearDmdDescription(void)

{
  memset_s(&DAT_6bba3788,0x1f5,0,0x1f5);
  DAT_6bba397e = 0;
  return;
}


// ============================================================================
// Function: DmdDescriptionPutStr @ 6baf690c
// ============================================================================

void DmdDescriptionPutStr(longlong param_1)

{
  uint uVar1;
  ushort local_a;
  
  if (param_1 != 0) {
    local_a = 0;
    while ((*(char *)(param_1 + (ulonglong)local_a) != '\0' && (DAT_6bba397e < 500))) {
      uVar1 = (uint)DAT_6bba397e;
      DAT_6bba397e = DAT_6bba397e + 1;
      (&DAT_6bba3788)[(int)uVar1] = *(undefined1 *)(param_1 + (ulonglong)local_a);
      local_a = local_a + 1;
    }
    if (DAT_6bba397e < 0x1f5) {
      (&DAT_6bba3788)[(int)(uint)DAT_6bba397e] = 0;
    }
  }
  return;
}

