#include "TSACoreInternal.h"

// ============================================================================
// Function: IDT_Process @ 6ba611a6
// ============================================================================

/* ID tracking stage: builds a 20-byte scratch map, runs HM matching, acceleration check, then maps
   current touches to IDs. */

void IDT_Process(uchar bUseStrictAccCheck)

{
  uchar abIdtScratch [20];
  
                    /* 0x211a6  886  IDT_Process */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&_rdata,"IDT_Process");
  }
  abIdtScratch[0] = '\0';
  abIdtScratch[1] = '\0';
  abIdtScratch[2] = '\0';
  abIdtScratch[3] = '\0';
  abIdtScratch[4] = '\0';
  abIdtScratch[5] = '\0';
  abIdtScratch[6] = '\0';
  abIdtScratch[7] = '\0';
  abIdtScratch[8] = '\0';
  abIdtScratch[9] = '\0';
  abIdtScratch[10] = '\0';
  abIdtScratch[0xb] = '\0';
  abIdtScratch[0xc] = '\0';
  abIdtScratch[0xd] = '\0';
  abIdtScratch[0xe] = '\0';
  abIdtScratch[0xf] = '\0';
  abIdtScratch[0x10] = '\0';
  abIdtScratch[0x11] = '\0';
  abIdtScratch[0x12] = '\0';
  abIdtScratch[0x13] = '\0';
  IDT_HMProcess(abIdtScratch,0x14);
  IDT_AccCheck(bUseStrictAccCheck,abIdtScratch,0x14);
  IDT_Map(abIdtScratch,0x14);
  return;
}


// ============================================================================
// Function: IDT_Map @ 6ba6122e
// ============================================================================

void IDT_Map(longlong param_1,uint param_2)

{
  uint local_10;
  byte local_9;
  
  for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
    *(undefined1 *)
     ((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x154) = 0;
  }
  local_10 = (uint)(*_refptr_g_prevTouchesPtr)->bPrevTouchCount;
  if (param_2 < local_10) {
    local_10 = param_2;
  }
  for (local_9 = 0; local_9 < local_10; local_9 = local_9 + 1) {
    if (*(char *)(param_1 + (ulonglong)local_9) != -1) {
      *(undefined1 *)
       ((longlong)(int)*(char *)(param_1 + (ulonglong)local_9) * 0x168 +
        *(longlong *)_refptr_g_touchesPtr + 0x154) =
           *(undefined1 *)
            ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_9 * 0x478 + 0x154);
    }
  }
  return;
}


// ============================================================================
// Function: IDT_HMProcess @ 6ba61323
// ============================================================================

void IDT_HMProcess(longlong param_1,uint param_2)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x678]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x680];
  
  memset(((byte *)(_stk + 0x8)) + 0x20,0,0x640);
  ((byte *)(_stk + 0x8))[0] = 0;
  ((byte *)(_stk + 0x8))[1] = 0;
  ((byte *)(_stk + 0x8))[2] = 0;
  ((byte *)(_stk + 0x8))[3] = 0;
  ((byte *)(_stk + 0x8))[4] = 0;
  ((byte *)(_stk + 0x8))[5] = 0;
  ((byte *)(_stk + 0x8))[6] = 0;
  ((byte *)(_stk + 0x8))[7] = 0;
  ((byte *)(_stk + 0x8))[8] = 0;
  ((byte *)(_stk + 0x8))[9] = 0;
  ((byte *)(_stk + 0x8))[10] = 0;
  ((byte *)(_stk + 0x8))[0xb] = 0;
  ((byte *)(_stk + 0x8))[0xc] = 0;
  ((byte *)(_stk + 0x8))[0xd] = 0;
  ((byte *)(_stk + 0x8))[0xe] = 0;
  ((byte *)(_stk + 0x8))[0xf] = 0;
  ((byte *)(_stk + 0x8))[0x10] = 0;
  ((byte *)(_stk + 0x8))[0x11] = 0;
  ((byte *)(_stk + 0x8))[0x12] = 0;
  ((byte *)(_stk + 0x8))[0x13] = 0;
  if (0x14 < **(byte **)_refptr_g_touchesPtr) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/IDTracking/IDT.c",
                      0x53,"TouchesPrpt(num) <= TSA_MAX_TOUCH_NUM");
    HardwareAnalyzer_SetSoftException();
  }
  if (0x14 < (*_refptr_g_prevTouchesPtr)->bPrevTouchCount) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/IDTracking/IDT.c",
                      0x54,"PrevTouchesPrpt(num) <= TSA_MAX_TOUCH_NUM");
    HardwareAnalyzer_SetSoftException();
  }
  for ((*(byte *)(_stk + 0x677)) = 0; (*(byte *)(_stk + 0x677)) < param_2; (*(byte *)(_stk + 0x677)) = (*(byte *)(_stk + 0x677)) + 1) {
    *(undefined1 *)(param_1 + (ulonglong)(*(byte *)(_stk + 0x677))) = 0xff;
  }
  if (**(byte **)_refptr_g_touchesPtr < (*_refptr_g_prevTouchesPtr)->bPrevTouchCount) {
    IDT_PopulateDistanceMap(((byte *)(_stk + 0x8)) + 0x20,1);
    IDT_HM(**(undefined1 **)_refptr_g_touchesPtr,(*_refptr_g_prevTouchesPtr)->bPrevTouchCount,
           ((byte *)(_stk + 0x8)),((byte *)(_stk + 0x8)) + 0x20);
    for ((*(byte *)(_stk + 0x677)) = 0; (*(byte *)(_stk + 0x677)) < (*_refptr_g_prevTouchesPtr)->bPrevTouchCount; (*(byte *)(_stk + 0x677)) = (*(byte *)(_stk + 0x677)) + 1)
    {
      if (((byte *)(_stk + 0x8))[(int)(uint)(*(byte *)(_stk + 0x677))] != 0xff) {
        *(byte *)((ulonglong)(*(byte *)(_stk + 0x677)) + param_1) = ((byte *)(_stk + 0x8))[(int)(uint)(*(byte *)(_stk + 0x677))];
      }
    }
  }
  else {
    IDT_PopulateDistanceMap(((byte *)(_stk + 0x8)) + 0x20,0);
    IDT_HM((*_refptr_g_prevTouchesPtr)->bPrevTouchCount,**(undefined1 **)_refptr_g_touchesPtr,
           ((byte *)(_stk + 0x8)),((byte *)(_stk + 0x8)) + 0x20);
    for ((*(byte *)(_stk + 0x677)) = 0; (*(byte *)(_stk + 0x677)) < **(byte **)_refptr_g_touchesPtr; (*(byte *)(_stk + 0x677)) = (*(byte *)(_stk + 0x677)) + 1) {
      if ((((byte *)(_stk + 0x8))[(int)(uint)(*(byte *)(_stk + 0x677))] != 0xff) && (((byte *)(_stk + 0x8))[(int)(uint)(*(byte *)(_stk + 0x677))] < param_2)) {
        *(byte *)((char)((byte *)(_stk + 0x8))[(int)(uint)(*(byte *)(_stk + 0x677))] + param_1) = (*(byte *)(_stk + 0x677));
      }
    }
  }
  return;
}


// ============================================================================
// Function: IDT_AccCheck @ 6ba619a3
// ============================================================================

void IDT_AccCheck(char param_1,longlong param_2,uint param_3)

{
  byte bVar1;
  char cVar2;
  char cVar3;
  ushort uVar4;
  int iVar5;
  int iVar6;
  longlong local_18;
  uint local_10;
  uint local_c;
  
  uVar4 = TS_MMToPixel((char)*(undefined2 *)&_refptr_g_tsaPrmtFlash[1].field_0x4a);
  bVar1 = (*_refptr_g_prevTouchesPtr)->bPrevTouchCount;
  if ((*_refptr_g_prevTouchesPtr)->bPrevTouchCount <= **(byte **)_refptr_g_touchesPtr) {
    bVar1 = **(byte **)_refptr_g_touchesPtr;
  }
  local_10 = (uint)bVar1;
  if (param_3 < local_10) {
    local_10 = param_3;
  }
  for (local_c = 0; local_c < local_10; local_c = local_c + 1) {
    local_18 = (longlong)(int)((uint)(uVar4 >> 4) * (uint)(uVar4 >> 4));
    cVar3 = *(char *)(param_2 + (ulonglong)local_c);
    iVar5 = (int)cVar3;
    if (iVar5 != -1) {
      iVar6 = IDT_GetAcc(iVar5,local_c);
      cVar2 = IDT_IsToIncAccThold(cVar3,local_c & 0xff);
      if ((cVar2 != '\0') || (param_1 != '\0')) {
        local_18 = local_18 << 2;
      }
      if (local_18 < iVar6) {
        *(undefined1 *)(param_2 + (ulonglong)local_c) = 0xff;
        cVar3 = TSALog_RtEnable(1,1,1);
        if (cVar3 != '\0') {
          StrPrintf_Process(0,"\nIDT: ACC(%d) too large...",iVar6);
        }
        (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
             (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 4;
      }
      else {
        cVar3 = GripFilter_IsToCancelIDMapping(cVar3,local_c & 0xff);
        if (cVar3 != '\0') {
          *(undefined1 *)(param_2 + (ulonglong)local_c) = 0xff;
          *(uint *)((longlong)iVar5 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214) =
               *(uint *)((longlong)iVar5 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214) |
               0x200;
        }
      }
      if ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (ulonglong)local_c * 0x478 + 0x1f8) &
          0x200000) != 0) {
        *(undefined1 *)(param_2 + (ulonglong)local_c) = 0xff;
      }
    }
  }
  return;
}


// ============================================================================
// Function: IDT_HM @ 6ba61ba2
// ============================================================================

void IDT_HM(byte param_1,byte param_2,longlong param_3,longlong param_4)

{
  char acStack_158 [64];
  int aiStack_118 [20];
  int aiStack_c8 [20];
  int aiStack_78 [23];
  int local_1c;
  int local_18;
  byte local_12;
  byte local_11;
  byte local_10;
  byte local_f;
  byte local_e;
  byte local_d;
  byte local_c;
  byte local_b;
  byte local_a;
  byte local_9;
  
  local_b = 0;
  if (param_2 < param_1) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/IDTracking/IDT.c",
                      0x11c,"dim2Len >= dim1Len");
    HardwareAnalyzer_SetSoftException();
  }
  for (local_c = 0; local_c < 0x14; local_c = local_c + 1) {
    *(undefined1 *)(param_3 + (ulonglong)local_c) = 0xff;
  }
  for (local_d = 0; local_d < param_2; local_d = local_d + 1) {
    aiStack_78[(int)(uint)local_d] = 0;
    aiStack_c8[(int)(uint)local_d] = 0;
  }
  for (local_e = 0; local_e < param_1; local_e = local_e + 1) {
    for (local_f = 0; local_f < param_2; local_f = local_f + 1) {
      acStack_158[(longlong)(int)(uint)local_f + 0x20] = -1;
      aiStack_118[(int)(uint)local_f] = 0x7fffffff;
      acStack_158[(int)(uint)local_f] = '\0';
    }
    local_a = 0xff;
    local_9 = local_e;
    while (local_9 != 0xff) {
      local_b = 0xff;
      local_10 = 0;
      for (local_11 = 0; local_11 < param_2; local_11 = local_11 + 1) {
        if (acStack_158[(int)(uint)local_11] == '\0') {
          local_18 = (*(int *)(param_4 + (longlong)(int)((int)(char)local_9 + (uint)local_10) * 4) -
                     aiStack_78[(int)(char)local_9]) - aiStack_c8[(int)(uint)local_11];
          if (local_18 < aiStack_118[(int)(uint)local_11]) {
            aiStack_118[(int)(uint)local_11] = local_18;
            acStack_158[(longlong)(int)(uint)local_11 + 0x20] = local_a;
          }
          if ((local_b == 0xff) ||
             (aiStack_118[(int)(uint)local_11] < aiStack_118[(int)(char)local_b])) {
            local_b = local_11;
          }
        }
        local_10 = local_10 + 0x14;
      }
      if (local_b == 0xff) {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                          "../../src/Alg/IDTracking/IDT.c",0x143,"dim2Idx != HM_INVALID_MAP");
        HardwareAnalyzer_SetSoftException();
      }
      if (local_b == 0xff) {
        local_b = 0;
      }
      local_1c = aiStack_118[(int)(char)local_b];
      for (local_12 = 0; local_12 < param_2; local_12 = local_12 + 1) {
        if (acStack_158[(int)(uint)local_12] == '\0') {
          aiStack_118[(int)(uint)local_12] = aiStack_118[(int)(uint)local_12] - local_1c;
        }
        else {
          aiStack_78[(int)*(char *)(param_3 + (ulonglong)local_12)] =
               aiStack_78[(int)*(char *)(param_3 + (ulonglong)local_12)] + local_1c;
          aiStack_c8[(int)(uint)local_12] = aiStack_c8[(int)(uint)local_12] - local_1c;
        }
      }
      aiStack_78[(int)(uint)local_e] = aiStack_78[(int)(uint)local_e] + local_1c;
      acStack_158[(int)(char)local_b] = '\x01';
      local_a = local_b;
      local_9 = *(byte *)(param_3 + (char)local_b);
    }
    if (local_b == 0xff) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Alg/IDTracking/IDT.c",0x158,"dim2Idx != HM_INVALID_MAP");
      HardwareAnalyzer_SetSoftException();
    }
    if (local_b == 0xff) {
      local_b = 0;
    }
    while (acStack_158[(longlong)(int)(char)local_b + 0x20] != -1) {
      *(undefined1 *)((char)local_b + param_3) =
           *(undefined1 *)(param_3 + acStack_158[(longlong)(int)(char)local_b + 0x20]);
      local_b = acStack_158[(longlong)(int)(char)local_b + 0x20];
    }
    *(byte *)((char)local_b + param_3) = local_e;
  }
  return;
}


// ============================================================================
// Function: IDT_PopulateDistanceMap @ 6ba623e4
// ============================================================================

void IDT_PopulateDistanceMap(longlong param_1,char param_2)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0xa8]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0xb0];
  char cVar1;
  uint uVar2;
  uint uVar3;
  
  (*(char *)(_stk + 0xa3)) = **(byte **)_refptr_g_touchesPtr < (*_refptr_g_prevTouchesPtr)->bPrevTouchCount;
  if (param_2 == '\0') {
    (*(undefined4 *)(_stk + 0x94)) = 1;
    (*(undefined4 *)(_stk + 0x90)) = 0x14;
  }
  else {
    (*(undefined4 *)(_stk + 0x94)) = 0x14;
    (*(undefined4 *)(_stk + 0x90)) = 1;
  }
  (*(short *)(_stk + 0x8e)) = TS_MMToPixel(_refptr_g_tsaPrmtFlash[1].field_0x48);
  (*(byte *)(_stk + 0xa2)) = 5;
  if (((*_refptr_g_tsaStaticPtr)->dwProcessingState & 0x10) != 0) {
    (*(byte *)(_stk + 0xa2)) = 0;
  }
  (*(int *)(_stk + 0x88)) = memset_s(((byte *)(_stk + 0x8)),0x28,0,0x28);
  if ((*(int *)(_stk + 0x88)) != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/IDTracking/IDT.c",
                      0x1e5,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (*(byte *)(_stk + 0xa1)) = 0;
  (*(byte *)(_stk + 0xa5)) = 0;
  for ((*(byte *)(_stk + 0xa7)) = 0; (*(byte *)(_stk + 0xa7)) < **(byte **)_refptr_g_touchesPtr; (*(byte *)(_stk + 0xa7)) = (*(byte *)(_stk + 0xa7)) + 1) {
    (*(byte *)(_stk + 0xa4)) = (*(byte *)(_stk + 0xa5));
    (*(longlong *)(_stk + 0x80)) = *(longlong *)_refptr_g_touchesPtr + (longlong)(int)(uint)(*(byte *)(_stk + 0xa7)) * 0x168 + 0x140;
    for ((*(byte *)(_stk + 0xa6)) = 0; (*(byte *)(_stk + 0xa6)) < (*_refptr_g_prevTouchesPtr)->bPrevTouchCount; (*(byte *)(_stk + 0xa6)) = (*(byte *)(_stk + 0xa6)) + 1)
    {
      (*(longlong *)(_stk + 0x78)) = (longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)(*(byte *)(_stk + 0xa6)) * 0x478 + 0x140;
      (*(uint *)(_stk + 0x74)) = (uint)*(ushort *)((*(longlong *)(_stk + 0x78)) + 0x1e);
      (*(uint *)(_stk + 0x70)) = (uint)*(ushort *)((*(longlong *)(_stk + 0x78)) + 0x20);
      (*(uint *)(_stk + 0x6c)) = *(ushort *)((*(longlong *)(_stk + 0x80)) + 0x1e) - (*(uint *)(_stk + 0x74));
      (*(uint *)(_stk + 0x68)) = *(ushort *)((*(longlong *)(_stk + 0x80)) + 0x20) - (*(uint *)(_stk + 0x70));
      (*(int *)(_stk + 0x9c)) = (*(uint *)(_stk + 0x68)) * (*(uint *)(_stk + 0x68)) + (*(uint *)(_stk + 0x6c)) * (*(uint *)(_stk + 0x6c));
      (*(short *)(_stk + 0x9a)) = *(short *)((*(longlong *)(_stk + 0x78)) + 0x10);
      if ((*_refptr_g_prevTouchesPtr)->nTouchCount == 0x100) {
        if (*(longlong *)((*(longlong *)(_stk + 0x80)) + 8) == 0) {
          (*(short *)(_stk + 0x9a)) = (short)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
                     (short)*(undefined8 *)((*(longlong *)(_stk + 0x78)) + 8);
        }
        else {
          (*(short *)(_stk + 0x9a)) = (short)*(undefined8 *)((*(longlong *)(_stk + 0x80)) + 8) - (short)*(undefined8 *)((*(longlong *)(_stk + 0x78)) + 8);
        }
      }
      (*(int *)(_stk + 0x64)) = (int)*(short *)((*(longlong *)(_stk + 0x78)) + 0x38c);
      (*(int *)(_stk + 0x60)) = (int)*(short *)((*(longlong *)(_stk + 0x78)) + 0x38e);
      (*(uint *)(_stk + 0x5c)) = (*(short *)(_stk + 0x9a)) * (*(int *)(_stk + 0x64));
      (*(uint *)(_stk + 0x58)) = (*(short *)(_stk + 0x9a)) * (*(int *)(_stk + 0x60));
      (*(int *)(_stk + 0x54)) = (int)(*(short *)(_stk + 0x9a)) * (int)(*(short *)((*(longlong *)(_stk + 0x78)) + 0x390) / 2) + (*(uint *)(_stk + 0x5c)) + (*(uint *)(_stk + 0x74));
      (*(int *)(_stk + 0x50)) = (int)(*(short *)(_stk + 0x9a)) * (int)(*(short *)((*(longlong *)(_stk + 0x78)) + 0x392) / 2) + (*(uint *)(_stk + 0x58)) + (*(uint *)(_stk + 0x70));
      uVar3 = (*(int *)(_stk + 0x54)) - (uint)*(ushort *)((*(longlong *)(_stk + 0x80)) + 0x1e);
      uVar2 = (int)uVar3 >> 0x1f;
      (*(int *)(_stk + 0x4c)) = (uVar3 ^ uVar2) - uVar2;
      uVar3 = (*(int *)(_stk + 0x50)) - (uint)*(ushort *)((*(longlong *)(_stk + 0x80)) + 0x20);
      uVar2 = (int)uVar3 >> 0x1f;
      (*(int *)(_stk + 0x48)) = (uVar3 ^ uVar2) - uVar2;
      if ((((int)(*(short *)(_stk + 0x8e)) <
            (int)((((int)(*(uint *)(_stk + 0x6c)) >> 0x1f ^ (*(uint *)(_stk + 0x6c))) - ((int)(*(uint *)(_stk + 0x6c)) >> 0x1f)) +
                 (((int)(*(uint *)(_stk + 0x68)) >> 0x1f ^ (*(uint *)(_stk + 0x68))) - ((int)(*(uint *)(_stk + 0x68)) >> 0x1f)))) ||
          (*(ushort *)((*(longlong *)(_stk + 0x78)) + 0x16) <= (ushort)(*(byte *)(_stk + 0xa2)))) ||
         ((int)(*(short *)(_stk + 0x8e)) <
          (int)((((int)(*(uint *)(_stk + 0x5c)) >> 0x1f ^ (*(uint *)(_stk + 0x5c))) - ((int)(*(uint *)(_stk + 0x5c)) >> 0x1f)) +
               (((int)(*(uint *)(_stk + 0x58)) >> 0x1f ^ (*(uint *)(_stk + 0x58))) - ((int)(*(uint *)(_stk + 0x58)) >> 0x1f))))) {
        if ((((*(int *)(_stk + 0x4c)) + (*(int *)(_stk + 0x48)) <= (int)(*(short *)(_stk + 0x8e))) &&
            (cVar1 = IDT_IsEdgeTouch((*(byte *)(_stk + 0xa7))), cVar1 == '\0')) && ((*(byte *)(_stk + 0xa1)) < 0x14)) {
          ((byte *)(_stk + 0x8))[(longlong)(int)(uint)(*(byte *)(_stk + 0xa1)) * 2] = (*(byte *)(_stk + 0xa7));
          ((byte *)(_stk + 0x8))[(longlong)(int)(uint)(*(byte *)(_stk + 0xa1)) * 2 + 1] = (*(byte *)(_stk + 0xa6));
          (*(byte *)(_stk + 0xa1)) = (*(byte *)(_stk + 0xa1)) + 1;
        }
      }
      else {
        (*(int *)(_stk + 0x9c)) = -0x3fffffff;
      }
      if ((*(char *)(_stk + 0xa3)) != '\0') {
        (*(int *)(_stk + 0x44)) = (*(uint *)(_stk + 0x6c)) - (*(int *)(_stk + 0x64));
        (*(int *)(_stk + 0x40)) = (*(uint *)(_stk + 0x68)) - (*(int *)(_stk + 0x60));
        (*(int *)(_stk + 0x3c)) = (*(int *)(_stk + 0x40)) * (*(int *)(_stk + 0x40)) + (*(int *)(_stk + 0x44)) * (*(int *)(_stk + 0x44));
        (*(int *)(_stk + 0x9c)) = (*(int *)(_stk + 0x9c)) + (*(int *)(_stk + 0x3c)) * 2;
      }
      *(int *)((ulonglong)(*(byte *)(_stk + 0xa4)) * 4 + param_1) = (*(int *)(_stk + 0x9c));
      (*(byte *)(_stk + 0xa4)) = (*(byte *)(_stk + 0xa4)) + (char)(*(undefined4 *)(_stk + 0x94));
    }
    (*(byte *)(_stk + 0xa5)) = (*(byte *)(_stk + 0xa5)) + (char)(*(undefined4 *)(_stk + 0x90));
  }
  IDT_AlwaysMatchProcess(param_1,param_2,((byte *)(_stk + 0x8)),(*(byte *)(_stk + 0xa1)));
  return;
}


// ============================================================================
// Function: IDT_IsToIncAccThold @ 6ba615c7
// ============================================================================

undefined8 IDT_IsToIncAccThold(byte param_1,byte param_2)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0x215c7  885  IDT_IsToIncAccThold */
  cVar1 = GripFilter_IsPeakAbsGrip
                    (*(undefined1 *)
                      ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x23c));
  if ((cVar1 == '\0') &&
     (*(int *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x210)
      == 1)) {
    uVar2 = 0;
  }
  else if (((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_2 * 0x478 + 0x1fc) & 0x1000000) == 0) &&
          ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)param_2 * 0x478 + 0x1fc) & 0x2000000) == 0)) {
    cVar1 = Touch_IsException(param_1,param_2);
    if ((cVar1 == '\0') && ((*_refptr_g_tsaStaticPtr)->bField_0x111 < 2)) {
      if (*(ushort *)
           ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4f6) <
          0x19) {
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
// Function: IDT_GetAcc @ 6ba61704
// ============================================================================

int IDT_GetAcc(int param_1,int param_2)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int local_10;
  int local_c;
  
  uVar1 = (int)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal >> 0x1f;
  local_c = ((int)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal ^ uVar1) - uVar1;
  if ((*_refptr_g_prevTouchesPtr)->nTouchCount == 0x100) {
    if (*(longlong *)((longlong)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x148) == 0) {
      local_c = (int)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
                (int)*(undefined8 *)
                      ((longlong)*_refptr_g_prevTouchesPtr + (longlong)param_2 * 0x478 + 0x148);
    }
    else {
      local_c = (int)*(undefined8 *)
                      ((longlong)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x148) -
                (int)*(undefined8 *)
                      ((longlong)*_refptr_g_prevTouchesPtr + (longlong)param_2 * 0x478 + 0x148);
    }
  }
  if (local_c == 0) {
    local_c = 1;
  }
  iVar2 = (int)((uint)*(ushort *)
                       ((longlong)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x15e) -
               (uint)*(ushort *)
                      ((longlong)*_refptr_g_prevTouchesPtr + (longlong)param_2 * 0x478 + 0x15e)) /
          local_c;
  local_c = (int)((uint)*(ushort *)
                         ((longlong)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x160) -
                 (uint)*(ushort *)
                        ((longlong)*_refptr_g_prevTouchesPtr + (longlong)param_2 * 0x478 + 0x160)) /
            local_c;
  iVar3 = iVar2 - *(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)param_2 * 0x478 + 0x4cc
                            );
  iVar4 = local_c - *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                              (longlong)param_2 * 0x478 + 0x4ce);
  iVar2 = local_c * local_c + iVar2 * iVar2;
  if (*(short *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)param_2 * 0x478 + 0x156) == 1) {
    if (iVar2 < 0) {
      iVar2 = iVar2 + 3;
    }
    local_10 = iVar2 >> 2;
  }
  else {
    local_10 = iVar4 * iVar4 + iVar3 * iVar3;
    if (iVar2 < local_10) {
      local_10 = iVar2;
    }
  }
  return local_10;
}


// ============================================================================
// Function: IDT_Init @ 6ba611a0
// ============================================================================

void IDT_Init(void)

{
                    /* 0x211a0  884  IDT_Init */
  return;
}


// ============================================================================
// Function: IDT_AlwaysMatchProcess @ 6ba620ac
// ============================================================================

void IDT_AlwaysMatchProcess(longlong param_1,char param_2,longlong param_3,byte param_4)

{
  undefined1 uVar1;
  bool bVar2;
  char cVar3;
  char cVar4;
  char cVar5;
  ushort uVar6;
  int iVar7;
  int iVar8;
  longlong local_20;
  longlong local_18;
  byte local_c;
  byte local_a;
  byte local_9;
  
  if (param_2 == '\0') {
    cVar3 = '\x01';
    cVar4 = '\x14';
  }
  else {
    cVar3 = '\x14';
    cVar4 = '\x01';
  }
  uVar6 = TS_MMToPixel((char)*(undefined2 *)&_refptr_g_tsaPrmtFlash[1].field_0x4a);
  iVar7 = (uint)(uVar6 >> 4) * (uint)(uVar6 >> 4);
  local_9 = 0;
  do {
    if (param_4 <= local_9) {
      return;
    }
    uVar1 = *(undefined1 *)(param_3 + (ulonglong)local_9 * 2);
    bVar2 = false;
    for (local_c = 0; local_c < (*_refptr_g_prevTouchesPtr)->bPrevTouchCount; local_c = local_c + 1)
    {
      iVar8 = IDT_GetAcc(uVar1,local_c);
      local_18 = (longlong)iVar7;
      cVar5 = IDT_IsToIncAccThold(uVar1,local_c);
      if (cVar5 != '\0') {
        local_18 = local_18 << 2;
      }
      if (*(byte *)(param_3 + (ulonglong)local_9 * 2 + 1) == local_c) {
        if (local_18 < iVar8) {
          bVar2 = true;
          break;
        }
      }
      else if (iVar8 <= local_18) {
        bVar2 = true;
      }
    }
    if (!bVar2) {
      uVar1 = *(undefined1 *)(param_3 + (ulonglong)local_9 * 2 + 1);
      for (local_a = 0; local_a < **(byte **)_refptr_g_touchesPtr; local_a = local_a + 1) {
        if (*(byte *)(param_3 + (ulonglong)local_9 * 2) != local_a) {
          iVar8 = IDT_GetAcc(local_a,uVar1);
          local_20 = (longlong)iVar7;
          cVar5 = IDT_IsToIncAccThold(local_a,uVar1);
          if (cVar5 != '\0') {
            local_20 = local_20 << 2;
          }
          if (iVar8 <= local_20) {
            bVar2 = true;
          }
        }
      }
      if (!bVar2) {
        *(undefined4 *)
         (param_1 +
         (ulonglong)
         (byte)(*(char *)(param_3 + (ulonglong)local_9 * 2 + 1) * cVar3 +
               cVar4 * *(char *)(param_3 + (ulonglong)local_9 * 2)) * 4) = 0xc0000001;
      }
    }
    local_9 = local_9 + 1;
  } while( true );
}


// ============================================================================
// Function: IDT_IsEdgeTouch @ 6ba622e9
// ============================================================================

undefined8 IDT_IsEdgeTouch(byte param_1)

{
  ushort uVar1;
  ushort uVar2;
  undefined8 uVar3;
  bool bVar4;
  byte local_a;
  
  uVar1 = *(ushort *)
           ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x15e) >> 8;
  uVar2 = *(ushort *)
           ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x160) >> 8;
  local_a = _refptr_g_tsaPrmtFlash->bCols - 1;
  bVar4 = ((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) != 0;
  if (bVar4) {
    local_a = _refptr_g_tsaPrmtFlash->bCols - 2;
  }
  if ((((bVar4 < uVar1) && (uVar1 < local_a)) && (uVar2 != 0)) &&
     (uVar2 < (byte)(_refptr_g_tsaPrmtFlash->bRows - 1))) {
    uVar3 = 0;
  }
  else {
    uVar3 = 1;
  }
  return uVar3;
}


