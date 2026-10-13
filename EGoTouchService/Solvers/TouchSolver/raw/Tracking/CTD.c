#include "TSACoreInternal.h"

// ============================================================================
// Function: CTD_ECProcess @ 6ba55ca0
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

void CTD_ECProcess(void)

{
  byte bTouchIndex;
  
                    /* 0x15ca0  490  CTD_ECProcess */
  for (bTouchIndex = 0; bTouchIndex < **(byte **)_refptr_g_touchesPtr; bTouchIndex = bTouchIndex + 1
      ) {
    *(undefined2 *)
     ((longlong)(int)(uint)bTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162) =
         *(undefined2 *)
          ((longlong)(int)(uint)bTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x15e);
    *(undefined2 *)
     ((longlong)(int)(uint)bTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164) =
         *(undefined2 *)
          ((longlong)(int)(uint)bTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x160);
    CTD_ECProcessUnit(bTouchIndex);
    *(undefined2 *)
     ((longlong)(int)(uint)bTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170) =
         *(undefined2 *)
          ((longlong)(int)(uint)bTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162);
    *(undefined2 *)
     ((longlong)(int)(uint)bTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172) =
         *(undefined2 *)
          ((longlong)(int)(uint)bTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164);
    Touch_SetEdgeDist(bTouchIndex);
  }
  return;
}


// ============================================================================
// Function: CTD_ECProcessUnit @ 6ba55c6b
// ============================================================================

void CTD_ECProcessUnit(undefined1 param_1)

{
  CTD_ECUpdatePosToEC(param_1);
  CTD_ECProcessDim1(param_1);
  CTD_ECProcessDim2(param_1);
  return;
}


// ============================================================================
// Function: CTD_ECUpdatePosToEC @ 6ba554d8
// ============================================================================

void CTD_ECUpdatePosToEC(undefined1 param_1)

{
  Touch_SetEdgeDist(param_1);
  return;
}


// ============================================================================
// Function: CTD_ECProcessDim1 @ 6ba5555b
// ============================================================================

void CTD_ECProcessDim1(byte param_1)

{
  TSAFlashPrmt *pTVar1;
  undefined *puVar2;
  short sVar3;
  int iVar4;
  int iVar5;
  
  *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) =
       *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) &
       0xfffffeff;
  puVar2 = _refptr_g_tsaPrmtRam;
  pTVar1 = _refptr_g_tsaPrmtFlash;
  if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x204) &
      2) == 0) {
    if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x204)
        & 1) != 0) {
      *(undefined8 *)_refptr_g_tsaPrmtRam = *(undefined8 *)(_refptr_g_tsaPrmtFlash + 1);
      *(undefined8 *)(puVar2 + 8) = *(undefined8 *)&pTVar1[1].field_0x8;
      puVar2[0x10] = pTVar1[1].field_0x10;
      sVar3 = CTD_ECGetOffset(*(undefined1 *)
                               ((longlong)(int)(uint)param_1 * 0x168 +
                                *(longlong *)_refptr_g_touchesPtr + 0x196),
                              *(undefined1 *)
                               ((longlong)(int)(uint)param_1 * 0x168 +
                                *(longlong *)_refptr_g_touchesPtr + 0x194));
      iVar5 = (uint)*(ushort *)
                     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x15e) - (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2a);
      iVar4 = CTD_ECGetFinalOffset(iVar5,0x100 - sVar3);
      *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162) =
           (short)iVar4 + *(short *)(_refptr_g_tsaPrmtFlashConst + 0x2a);
      if (iVar4 != iVar5) {
        *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0)
             = *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x1f0) | 0x100;
      }
    }
  }
  else {
    *(undefined8 *)_refptr_g_tsaPrmtRam = *(undefined8 *)&_refptr_g_tsaPrmtFlash[1].field_0x11;
    *(undefined8 *)(puVar2 + 8) = *(undefined8 *)&pTVar1[1].bCmfForceDim1Pass;
    puVar2[0x10] = pTVar1[1].field_0x21;
    sVar3 = CTD_ECGetOffset(*(undefined1 *)
                             ((longlong)(int)(uint)param_1 * 0x168 +
                              *(longlong *)_refptr_g_touchesPtr + 0x196),
                            *(undefined1 *)
                             ((longlong)(int)(uint)param_1 * 0x168 +
                              *(longlong *)_refptr_g_touchesPtr + 0x194));
    iVar5 = (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2c) -
            (uint)*(ushort *)
                   ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x15e
                   );
    iVar4 = CTD_ECGetFinalOffset(iVar5,0x100 - sVar3);
    *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162) =
         *(short *)(_refptr_g_tsaPrmtFlashConst + 0x2c) - (short)iVar4;
    if (iVar4 != iVar5) {
      *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) =
           *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1f0) | 0x100;
    }
  }
  return;
}


// ============================================================================
// Function: CTD_ECProcessDim2 @ 6ba558e4
// ============================================================================

void CTD_ECProcessDim2(byte param_1)

{
  TSAFlashPrmt *pTVar1;
  undefined *puVar2;
  short sVar3;
  int iVar4;
  int iVar5;
  
  *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) =
       *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) &
       0xfffffdff;
  puVar2 = _refptr_g_tsaPrmtRam;
  pTVar1 = _refptr_g_tsaPrmtFlash;
  if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x204) &
      8) == 0) {
    if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x204)
        & 4) != 0) {
      *(undefined8 *)_refptr_g_tsaPrmtRam = *(undefined8 *)&_refptr_g_tsaPrmtFlash[1].field_0x22;
      *(undefined8 *)(puVar2 + 8) = *(undefined8 *)&pTVar1[1].field_0x2a;
      puVar2[0x10] = pTVar1[1].field_0x32;
      sVar3 = CTD_ECGetOffset(*(undefined1 *)
                               ((longlong)(int)(uint)param_1 * 0x168 +
                                *(longlong *)_refptr_g_touchesPtr + 0x197),
                              *(undefined1 *)
                               ((longlong)(int)(uint)param_1 * 0x168 +
                                *(longlong *)_refptr_g_touchesPtr + 0x194));
      iVar5 = (uint)*(ushort *)
                     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x160) - (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x2e);
      iVar4 = CTD_ECGetFinalOffset(iVar5,0x100 - sVar3);
      *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164) =
           (short)iVar4 + *(short *)(_refptr_g_tsaPrmtFlashConst + 0x2e);
      if (iVar4 != iVar5) {
        *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0)
             = *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x1f0) | 0x200;
      }
    }
  }
  else {
    *(undefined8 *)_refptr_g_tsaPrmtRam = *(undefined8 *)&_refptr_g_tsaPrmtFlash[1].field_0x33;
    *(undefined8 *)(puVar2 + 8) = *(undefined8 *)&pTVar1[1].field_0x3b;
    puVar2[0x10] = pTVar1[1].field_0x43;
    sVar3 = CTD_ECGetOffset(*(undefined1 *)
                             ((longlong)(int)(uint)param_1 * 0x168 +
                              *(longlong *)_refptr_g_touchesPtr + 0x197),
                            *(undefined1 *)
                             ((longlong)(int)(uint)param_1 * 0x168 +
                              *(longlong *)_refptr_g_touchesPtr + 0x194));
    iVar5 = (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x30) -
            (uint)*(ushort *)
                   ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x160
                   );
    iVar4 = CTD_ECGetFinalOffset(iVar5,0x100 - sVar3);
    *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164) =
         *(short *)(_refptr_g_tsaPrmtFlashConst + 0x30) - (short)iVar4;
    if (iVar4 != iVar5) {
      *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) =
           *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1f0) | 0x200;
    }
  }
  return;
}


// ============================================================================
// Function: CTD_ECInit @ 6ba553a0
// ============================================================================

void CTD_ECInit(void)

{
                    /* 0x153a0  489  CTD_ECInit */
  return;
}


// ============================================================================
// Function: CTD_ECGetOffset @ 6ba553a6
// ============================================================================

undefined2 CTD_ECGetOffset(byte param_1,byte param_2)

{
  byte local_d;
  int local_c;
  
  local_d = 0;
  while (((int)(uint)local_d < (int)((byte)*_refptr_g_tsaPrmtRam - 1) &&
         ((byte)_refptr_g_tsaPrmtRam[(longlong)(int)(uint)local_d * 4 + 1] < param_2))) {
    local_d = local_d + 1;
  }
  if ((uint)*(ushort *)
             (&g_ctd256Ln +
             (longlong)(int)(uint)(byte)_refptr_g_tsaPrmtRam[(longlong)(int)(uint)local_d * 4 + 4] *
             2) -
      (uint)*(ushort *)
             (&g_ctd256Ln +
             (longlong)(int)(uint)(byte)_refptr_g_tsaPrmtRam[(longlong)(int)(uint)local_d * 4 + 3] *
             2) == 0) {
    local_c = 0;
  }
  else {
    local_c = (int)(((uint)*(ushort *)(&g_ctd256Ln + (longlong)(int)(uint)param_1 * 2) -
                    (uint)*(ushort *)
                           (&g_ctd256Ln +
                           (longlong)
                           (int)(uint)(byte)_refptr_g_tsaPrmtRam
                                            [(longlong)(int)(uint)local_d * 4 + 3] * 2)) * 0x100) /
              (int)((uint)*(ushort *)
                           (&g_ctd256Ln +
                           (longlong)
                           (int)(uint)(byte)_refptr_g_tsaPrmtRam
                                            [(longlong)(int)(uint)local_d * 4 + 4] * 2) -
                   (uint)*(ushort *)
                          (&g_ctd256Ln +
                          (longlong)
                          (int)(uint)(byte)_refptr_g_tsaPrmtRam
                                           [(longlong)(int)(uint)local_d * 4 + 3] * 2));
  }
  if (0xff < local_c) {
    local_c = 0xff;
  }
  return (short)local_c;
}


// ============================================================================
// Function: CTD_ECGetFinalOffset @ 6ba554f7
// ============================================================================

int CTD_ECGetFinalOffset(int param_1,int param_2)

{
  int iVar1;
  
  iVar1 = param_1 + -0x100;
  if (iVar1 < 1) {
    param_1 = param_2;
  }
  else if (iVar1 < 0x40) {
    param_1 = param_1 * iVar1 * 4 + (iVar1 * -4 + 0x100) * param_2;
    if (param_1 < 0) {
      param_1 = param_1 + 0xff;
    }
    param_1 = param_1 >> 8;
  }
  return param_1;
}


// ============================================================================
// Function: CTD_General @ 6ba5523c
// ============================================================================

void CTD_General(byte param_1,byte param_2,byte param_3,byte param_4,undefined2 *param_5,
                undefined2 *param_6)

{
  short sVar1;
  undefined2 uVar2;
  int local_1c;
  int local_18;
  int local_14;
  int local_10;
  int local_c;
  
                    /* 0x1523c  491  CTD_General */
  local_14 = 0;
  local_18 = 0;
  local_1c = 0;
  for (local_c = (uint)param_1 - (uint)(param_3 >> 1);
      local_c <= (int)((uint)(param_3 >> 1) + (uint)param_1); local_c = local_c + 1) {
    if ((byte)local_c < _refptr_g_tsaPrmtFlash->bCols) {
      for (local_10 = (uint)param_2 - (uint)(param_4 >> 1);
          local_10 <= (int)((uint)(param_4 >> 1) + (uint)param_2); local_10 = local_10 + 1) {
        if (((byte)local_10 < _refptr_g_tsaPrmtFlash->bRows) &&
           (sVar1 = *(short *)((ulonglong)
                               ((uint)_refptr_g_tsaPrmtFlash->bCols * local_10 + local_c & 0xffff) *
                               2 + *(longlong *)_refptr_g_tsaBufDif), 0 < sVar1)) {
          local_14 = local_14 + sVar1 * local_c;
          local_18 = local_18 + sVar1 * local_10;
          local_1c = local_1c + sVar1;
        }
      }
    }
  }
  uVar2 = CTD_BasicCalculation(local_14,local_1c);
  *param_5 = uVar2;
  uVar2 = CTD_BasicCalculation(local_18,local_1c);
  *param_6 = uVar2;
  return;
}


// ============================================================================
// Function: CTD_BasicCalculation @ 6ba551e0
// ============================================================================

longlong CTD_BasicCalculation(int param_1,int param_2)

{
  longlong local_10;
  
                    /* 0x151e0  488  CTD_BasicCalculation */
  if (param_2 == 0) {
    local_10 = 0;
  }
  else {
    local_10 = ((longlong)param_1 << 8) / (longlong)param_2 + 0x80;
    if (local_10 < 0) {
      local_10 = 0;
    }
  }
  return local_10;
}


