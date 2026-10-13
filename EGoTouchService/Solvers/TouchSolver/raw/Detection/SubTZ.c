#include "TSACoreInternal.h"

// ============================================================================
// Function: SubTZ_Process @ 6ba7e646
// ============================================================================

void SubTZ_Process(longlong param_1)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0xca]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0xd0];
  byte bVar1;
  uchar uVar2;
  undefined1 uVar3;
  int iVar4;
  undefined8 in_stack_ffffffffffffff18;
  undefined4 uVar5;
  
                    /* 0x3e646  1459  SubTZ_Process */
  (*(int *)(_stk + 0xb4)) = -0x80000000;
  memcpy(((undefined1 *)(_stk + 0x8)),_refptr_g_list,0x98);
  *(undefined **)_refptr_g_ptrAccess = _refptr_g_subTZAccess;
  (*(int *)(_stk + 0xac)) = memset_s(&g_subTZ,0x2d8,0,0x2d8);
  if ((*(int *)(_stk + 0xac)) != 0) {
    in_stack_ffffffffffffff18 = 0;
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/TouchZone/SubTouchZone/SubTouchZone.c",0x188,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  for ((*(byte *)(_stk + 0xb3)) = 0; uVar5 = (undefined4)((ulonglong)in_stack_ffffffffffffff18 >> 0x20),
      (*(byte *)(_stk + 0xb3)) < *(byte *)(param_1 + 0x2b2); (*(byte *)(_stk + 0xb3)) = (*(byte *)(_stk + 0xb3)) + 1) {
    (*(byte *)(_stk + 0xab)) = *(byte *)(param_1 + 0x2b9 + (longlong)(int)(uint)(*(byte *)(_stk + 0xb3)));
    (*(ushort *)(_stk + 0xa8)) = (ushort)(*_refptr_g_peaksPtr)[(longlong)(int)(uint)(*(byte *)(_stk + 0xab)) * 0x14 + 0x15].bPeakCount
               + (ushort)(*_refptr_g_peaksPtr)[(longlong)(int)(uint)(*(byte *)(_stk + 0xab)) * 0x14 + 0x16].
                         bPeakCount * (ushort)_refptr_g_tsaPrmtFlash->bCols;
    if (*(char *)((ulonglong)(*(ushort *)(_stk + 0xa8)) + *(longlong *)_refptr_g_tsaBufSubTZ) == '\0') {
      *(undefined8 *)_refptr_g_tsaBufTemp = *(undefined8 *)_refptr_g_tsaBufDif;
      if ((*(uint *)(param_1 + 4) & 0x1000000) == 0) {
        (*(short *)(_stk + 0xa6)) = SubTz_GetSignalThresh(param_1,_refptr_g_tsaBufTemp);
        in_stack_ffffffffffffff18 =
             CONCAT44(uVar5,(uint)(*_refptr_g_peaksPtr)[(longlong)(int)(uint)(*(byte *)(_stk + 0xab)) * 0x14 + 0x16]
                                  .bPeakCount);
        SubTZ_PeakBasedTraversal
                  (param_1,&DAT_6bbc8688 + (longlong)(int)(uint)g_subTZ * 0x48,(int)(*(short *)(_stk + 0xa6)),
                   (*_refptr_g_peaksPtr)[(longlong)(int)(uint)(*(byte *)(_stk + 0xab)) * 0x14 + 0x15].bPeakCount,
                   in_stack_ffffffffffffff18);
      }
      else {
        bVar1 = (*_refptr_g_peaksPtr)[(longlong)(int)(uint)(*(byte *)(_stk + 0xab)) * 0x14 + 0x16].bPeakCount;
        uVar2 = (*_refptr_g_peaksPtr)[(longlong)(int)(uint)(*(byte *)(_stk + 0xab)) * 0x14 + 0x15].bPeakCount;
        uVar3 = GripFilter_IsPeakAbsGrip((*(byte *)(_stk + 0xab)));
        in_stack_ffffffffffffff18 = CONCAT44(uVar5,(uint)bVar1);
        SubTZ_GripBasedTraversal
                  (param_1,&DAT_6bbc8688 + (longlong)(int)(uint)g_subTZ * 0x48,uVar3,uVar2,
                   in_stack_ffffffffffffff18);
      }
      if (*(short *)(&DAT_6bbc868c + (longlong)(int)(uint)g_subTZ * 0x48) != 0) {
        _refptr_g_subTZAccess[9] = _refptr_g_subTZAccess[9] + '\b';
        SubTZ_Centroid(&DAT_6bbc8688 + (longlong)(int)(uint)g_subTZ * 0x48);
        SubTZ_GetAxis(&DAT_6bbc8688 + (longlong)(int)(uint)g_subTZ * 0x48);
        SubTZ_PeakInfoRetrieval
                  (*(undefined8 *)_refptr_g_tsaBufSubTZ,&g_pkAccessedFlagSubTZ,&(*(byte *)(_stk + 0x7)),&(*(undefined1 *)(_stk + 0x6)));
        if ((*(byte *)(_stk + 0x7)) == 0x14) {
          (*(byte *)(_stk + 0x7)) = (*(byte *)(_stk + 0xab));
        }
        (&DAT_6bbc868a)[(longlong)(int)(uint)g_subTZ * 0x48] = (*(byte *)(_stk + 0x7));
        (&DAT_6bbc868b)[(longlong)(int)(uint)g_subTZ * 0x48] = (*(undefined1 *)(_stk + 0x6));
        if (((*(byte *)(_stk + 0x7)) != 0x14) && (iVar4 = Peak_GetZ9((*(byte *)(_stk + 0x7))), (*(int *)(_stk + 0xb4)) < iVar4)) {
          (*(int *)(_stk + 0xb4)) = Peak_GetZ9((*(byte *)(_stk + 0x7)));
          DAT_6bbc8682 = g_subTZ;
        }
        TZ_SeparateZoneRegretProcess(param_1,&DAT_6bbc8688 + (longlong)(int)(uint)g_subTZ * 0x48);
        if (DAT_6bbc8684 < *(ushort *)(&DAT_6bbc868c + (longlong)(int)(uint)g_subTZ * 0x48)) {
          DAT_6bbc8684 = *(ushort *)(&DAT_6bbc868c + (longlong)(int)(uint)g_subTZ * 0x48);
          DAT_6bbc8686 = (ushort)g_subTZ;
        }
        g_subTZ = g_subTZ + 1;
      }
    }
  }
  if ((*(uint *)(param_1 + 4) & 0x1000000) == 0) {
    SubTZ_Filter(param_1);
  }
  memcpy(_refptr_g_list,((undefined1 *)(_stk + 0x8)),0x98);
  *(undefined **)_refptr_g_ptrAccess = _refptr_g_tzAccess;
  return;
}


// ============================================================================
// Function: SubTZ_PeakBasedTraversal @ 6ba7d838
// ============================================================================

void SubTZ_PeakBasedTraversal
               (undefined8 param_1,undefined8 param_2,short param_3,byte param_4,ushort param_5)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0xd8]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0xe0];
  ulonglong uVar1;
  char cVar2;
  
  PosnSpecial_Init(((undefined1 *)(_stk + 0x8)),100);
  SubTZ_UnitInit(param_2);
  (*(ushort *)(_stk + 0xd6)) = (ushort)param_4 + (param_5 & 0xff) * (ushort)_refptr_g_tsaPrmtFlash->bCols;
  PosnSpecial_Put((*(ushort *)(_stk + 0xd6)));
  while( true ) {
    cVar2 = PosnSpecial_IsEmpty();
    if (cVar2 != '\0') break;
    (*(ushort *)(_stk + 0xd6)) = PosnSpecial_Get();
    uVar1 = (ulonglong)(*(ushort *)(_stk + 0xd6)) % (ulonglong)(longlong)(int)(uint)_refptr_g_tsaPrmtFlash->bCols;
    (*(undefined1 *)(_stk + 0xd5)) = (undefined1)uVar1;
    (*(undefined1 *)(_stk + 0xd4)) = (undefined1)
              ((ulonglong)(*(ushort *)(_stk + 0xd6)) / (ulonglong)(longlong)(int)(uint)_refptr_g_tsaPrmtFlash->bCols);
    SubTZ_TraversalNeighbors(param_1,param_2,(int)param_3,uVar1,(*(undefined1 *)(_stk + 0xd4)));
  }
  return;
}


// ============================================================================
// Function: SubTZ_Centroid @ 6ba7d960
// ============================================================================

void SubTZ_Centroid(longlong param_1)

{
  undefined2 uVar1;
  
  uVar1 = CTD_BasicCalculation(*(undefined4 *)(param_1 + 0x10),*(undefined4 *)(param_1 + 0xc));
  *(undefined2 *)(param_1 + 6) = uVar1;
  uVar1 = CTD_BasicCalculation(*(undefined4 *)(param_1 + 0x14),*(undefined4 *)(param_1 + 0xc));
  *(undefined2 *)(param_1 + 8) = uVar1;
  return;
}


// ============================================================================
// Function: SubTZ_Filter @ 6ba7da00
// ============================================================================

void SubTZ_Filter(longlong param_1)

{
  short sVar1;
  char cVar2;
  byte bVar3;
  int iVar4;
  char local_a;
  byte local_9;
  
  local_a = '\0';
  sVar1 = *(short *)(*_refptr_g_peaksPtr +
                    (longlong)(int)(uint)*(byte *)(param_1 + 0x2b6) * 0x14 + 0x18);
  iVar4 = *(int *)(&DAT_6bbc8694 + (longlong)(int)(uint)DAT_6bbc8682 * 0x48);
  if (iVar4 < 0) {
    iVar4 = iVar4 + 7;
  }
  for (local_9 = 0; local_9 < g_subTZ; local_9 = local_9 + 1) {
    if (((sVar1 / 2 < *(short *)(&DAT_6bbc8688 + (longlong)(int)(uint)local_9 * 0x48)) &&
        (iVar4 >> 3 < *(int *)(&DAT_6bbc8694 + (longlong)(int)(uint)local_9 * 0x48))) ||
       (cVar2 = SubTZ_IsGrip(&DAT_6bbc8688 + (longlong)(int)(uint)local_9 * 0x48), cVar2 != '\0')) {
      local_a = local_a + '\x01';
      if ((DAT_6bbc8682 != local_9) &&
         (bVar3 = TZ_GetAutoCentToMinPkRatio
                            (param_1,(&DAT_6bbc868a)[(longlong)(int)(uint)local_9 * 0x48],
                             (&DAT_6bbc868a)[(longlong)(int)(uint)DAT_6bbc8682 * 0x48]),
         DAT_6bbc8683 < bVar3)) {
        DAT_6bbc8683 = bVar3;
      }
    }
    else {
      if (*(short *)(&DAT_6bbc8688 + (longlong)(int)(uint)local_9 * 0x48) <= sVar1 / 2) {
        *(uint *)(&DAT_6bbc86c8 + (longlong)(int)(uint)local_9 * 0x48) =
             *(uint *)(&DAT_6bbc86c8 + (longlong)(int)(uint)local_9 * 0x48) | 1;
      }
      if (*(int *)(&DAT_6bbc8694 + (longlong)(int)(uint)local_9 * 0x48) <= iVar4 >> 3) {
        *(uint *)(&DAT_6bbc86c8 + (longlong)(int)(uint)local_9 * 0x48) =
             *(uint *)(&DAT_6bbc86c8 + (longlong)(int)(uint)local_9 * 0x48) | 2;
      }
    }
  }
  DAT_6bbc8681 = local_a;
  return;
}


// ============================================================================
// Function: SubTZ_PeakInfoRetrieval @ 6ba7dcec
// ============================================================================

void SubTZ_PeakInfoRetrieval(longlong param_1,uint *param_2,byte *param_3,char *param_4)

{
  short local_c;
  byte local_9;
  
  *param_3 = 0x14;
  *param_4 = '\0';
  local_c = -0x8000;
  for (local_9 = 0; local_9 < (*_refptr_g_peaksPtr)->bPeakCount; local_9 = local_9 + 1) {
    if (((1 << (local_9 & 0x1f) & *param_2) == 0) &&
       (*(char *)(param_1 +
                 (ulonglong)
                 (ushort)((ushort)(*_refptr_g_peaksPtr)[(longlong)(int)(uint)local_9 * 0x14 + 0x15].
                                  bPeakCount +
                         (ushort)(*_refptr_g_peaksPtr)[(longlong)(int)(uint)local_9 * 0x14 + 0x16].
                                 bPeakCount * (ushort)_refptr_g_tsaPrmtFlash->bCols)) != '\0')) {
      *param_2 = *param_2 | 1 << (local_9 & 0x1f);
      *param_4 = *param_4 + '\x01';
      if (local_c < *(short *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)local_9 * 0x14 + 0x18)) {
        local_c = *(short *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)local_9 * 0x14 + 0x18);
        *param_3 = local_9;
      }
    }
  }
  return;
}


// ============================================================================
// Function: SubTZ_GripBasedTraversal @ 6ba7e51f
// ============================================================================

void SubTZ_GripBasedTraversal
               (undefined8 param_1,undefined8 param_2,undefined1 param_3,byte param_4,ushort param_5
               )

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0xd8]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0xe0];
  ulonglong uVar1;
  char cVar2;
  
  PosnSpecial_Init(((undefined1 *)(_stk + 0x8)),100);
  SubTZ_UnitInit(param_2);
  (*(ushort *)(_stk + 0xd6)) = (ushort)param_4 + (param_5 & 0xff) * (ushort)_refptr_g_tsaPrmtFlash->bCols;
  PosnSpecial_Put((*(ushort *)(_stk + 0xd6)));
  while( true ) {
    cVar2 = PosnSpecial_IsEmpty();
    if (cVar2 != '\0') break;
    (*(ushort *)(_stk + 0xd6)) = PosnSpecial_Get();
    uVar1 = (ulonglong)(*(ushort *)(_stk + 0xd6)) % (ulonglong)(longlong)(int)(uint)_refptr_g_tsaPrmtFlash->bCols;
    (*(undefined1 *)(_stk + 0xd5)) = (undefined1)uVar1;
    (*(undefined1 *)(_stk + 0xd4)) = (undefined1)
              ((ulonglong)(*(ushort *)(_stk + 0xd6)) / (ulonglong)(longlong)(int)(uint)_refptr_g_tsaPrmtFlash->bCols);
    SubTZ_TraversalGripNeighbors(param_1,param_2,param_3,uVar1,(*(undefined1 *)(_stk + 0xd4)));
  }
  return;
}


// ============================================================================
// Function: SubTZ_GetAxis @ 6ba7f1cc
// ============================================================================

void SubTZ_GetAxis(longlong param_1)

{
  byte bVar1;
  
                    /* 0x3f1cc  1456  SubTZ_GetAxis */
  TZ_GetAxisInternal((double)*(int *)(param_1 + 0xc),(double)*(int *)(param_1 + 0x10),
                     (double)*(int *)(param_1 + 0x14),(double)*(longlong *)(param_1 + 0x18),
                     (double)*(longlong *)(param_1 + 0x20),(double)*(longlong *)(param_1 + 0x28),
                     param_1 + 0x34,param_1 + 0x36,param_1 + 0x38);
  bVar1 = TS_PixelToMM(*(undefined2 *)(param_1 + 0x34));
  *(ushort *)(param_1 + 0x30) = (ushort)bVar1;
  bVar1 = TS_PixelToMM(*(undefined2 *)(param_1 + 0x36));
  *(ushort *)(param_1 + 0x32) = (ushort)bVar1;
  return;
}


// ============================================================================
// Function: SubTZ_IsMF @ 6ba7ec1d
// ============================================================================

bool SubTZ_IsMF(void)

{
  uchar uVar1;
  byte bVar2;
  uchar uVar3;
  bool local_19;
  
                    /* 0x3ec1d  1458  SubTZ_IsMF */
  local_19 = false;
  bVar2 = SubTZ_GetNumAftFilter();
  if (1 < bVar2) {
    local_19 = DAT_6bbc8683 < (byte)_refptr_g_tsaPrmtFlash->field_0x833;
    uVar1 = (*_refptr_g_prevTouchesPtr)->bPrevTouchCount;
    uVar3 = SubTZ_GetNumAftFilter();
    if ((uVar1 == uVar3) && (10 < DAT_6bbc8684)) {
      local_19 = true;
    }
  }
  return local_19;
}


// ============================================================================
// Function: SubTZ_UnitInit @ 6ba7d2e0
// ============================================================================

void SubTZ_UnitInit(undefined2 *param_1)

{
  int iVar1;
  
  iVar1 = memset_s(param_1,0x48,0,0x48);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/TouchZone/SubTouchZone/SubTouchZone.c",0x1d,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  *param_1 = 0x8000;
  *(undefined1 *)((longlong)param_1 + 0x39) = 0xff;
  *(undefined1 *)((longlong)param_1 + 0x3b) = 0xff;
  return;
}


// ============================================================================
// Function: SubTZ_TraversalNeighbors @ 6ba7d5a5
// ============================================================================

void SubTZ_TraversalNeighbors
               (longlong param_1,short *param_2,short param_3,char param_4,char param_5)

{
  char cVar1;
  char local_38 [16];
  char local_28 [10];
  short local_1e;
  undefined1 local_1b;
  ushort local_1a;
  longlong local_18;
  byte local_e;
  byte local_d;
  uint local_c;
  
  local_28[0] = '\0';
  local_28[1] = 1;
  local_28[2] = 1;
  local_28[3] = 1;
  local_28[4] = 0;
  local_28[5] = 0xff;
  local_28[6] = 0xff;
  local_28[7] = 0xff;
  local_28[8] = 0;
  local_38[0] = '\0';
  local_38[1] = 0xff;
  local_38[2] = 0;
  local_38[3] = 1;
  local_38[4] = 1;
  local_38[5] = 1;
  local_38[6] = 0;
  local_38[7] = 0xff;
  local_38[8] = 0xff;
  local_18 = *(longlong *)_refptr_g_subTZAccess;
  for (local_c = 0; local_c < 9; local_c = local_c + 1) {
    local_e = param_5 + local_38[local_c];
    local_d = param_4 + local_28[local_c];
    if ((local_d < _refptr_g_tsaPrmtFlash->bCols) && (local_e < _refptr_g_tsaPrmtFlash->bRows)) {
      local_1a = (ushort)local_d + (ushort)local_e * (ushort)_refptr_g_tsaPrmtFlash->bCols;
      local_1b = *(char *)(local_18 + (ulonglong)local_1a) == '\0';
      if (((bool)local_1b) &&
         (((*(char *)(param_1 + 0x34b) != '\0' || ((*(uint *)(param_1 + 0x350) & 200) != 0)) ||
          (cVar1 = SubTZ_IsInCurrTZ(param_1,local_1a), cVar1 != '\0')))) {
        local_1e = *(short *)(*(longlong *)_refptr_g_tsaBufTemp + (ulonglong)local_1a * 2);
        SubTZ_SetExtraFlag(local_1a,(int)local_1e,param_2);
        if (local_1e < param_3) {
          if (((*(char *)(param_1 + 0x34b) != '\0') || ((*(uint *)(param_1 + 0x350) & 8) != 0)) &&
             (0 < local_1e)) {
            SubTZ_UpdateUnit(param_2,(int)local_1e,local_d,local_e);
          }
        }
        else {
          param_2[2] = param_2[2] + 1;
          if (*param_2 < local_1e) {
            *param_2 = local_1e;
          }
          SubTZ_UpdateUnit(param_2,(int)local_1e,local_d,local_e);
          if (local_c != 0) {
            PosnSpecial_Put(local_1a);
          }
        }
        if (*(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)local_1a * 2) < 0) {
          cVar1 = -1;
        }
        else {
          cVar1 = '\x01';
        }
        *(char *)((ulonglong)local_1a + local_18) =
             (*(char *)(*(longlong *)_refptr_g_ptrAccess + 9) + '\x01') * cVar1;
      }
    }
  }
  return;
}


// ============================================================================
// Function: SubTZ_IsGrip @ 6ba7d9b0
// ============================================================================

undefined8 SubTZ_IsGrip(longlong param_1)

{
  undefined8 uVar1;
  
  if (((*(char *)(param_1 + 0x39) == '\0') ||
      ((uint)*(byte *)(param_1 + 0x3a) == _refptr_g_tsaPrmtFlash->bCols - 1)) &&
     (0x13 < *(ushort *)(param_1 + 0x30))) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: SubTZ_TraversalGripNeighbors @ 6ba7e28f
// ============================================================================

void SubTZ_TraversalGripNeighbors
               (longlong param_1,short *param_2,char param_3,byte param_4,byte param_5)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x38]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x40];
  char cVar1;
  
  memcpy(((char *)(_stk + 0x18)),"\x01\x01\x01",4);
  ((char *)(_stk + 0x18))[4] = 0xff;
  ((char *)(_stk + 0x18))[5] = 0xff;
  ((char *)(_stk + 0x18))[6] = 0xff;
  ((char *)(_stk + 0x18))[7] = 0;
  ((char *)(_stk + 0x18))[8] = 0;
  ((char *)(_stk + 0x8))[0] = -1;
  ((char *)(_stk + 0x8))[1] = 0;
  ((char *)(_stk + 0x8))[2] = 1;
  ((char *)(_stk + 0x8))[3] = 1;
  ((char *)(_stk + 0x8))[4] = 1;
  ((char *)(_stk + 0x8))[5] = 0;
  ((char *)(_stk + 0x8))[6] = 0xff;
  ((char *)(_stk + 0x8))[7] = 0xff;
  ((char *)(_stk + 0x8))[8] = 0;
  (*(longlong *)(_stk + 0x28)) = *(longlong *)_refptr_g_subTZAccess;
  (*(byte *)(_stk + 0x33)) = param_4;
  (*(byte *)(_stk + 0x32)) = param_5;
  (*(uint *)(_stk + 0x34)) = 0;
  do {
    if (8 < (*(uint *)(_stk + 0x34))) {
      return;
    }
    if (((*(byte *)(_stk + 0x33)) < _refptr_g_tsaPrmtFlash->bCols) && ((*(byte *)(_stk + 0x32)) < _refptr_g_tsaPrmtFlash->bRows)) {
      (*(ushort *)(_stk + 0x26)) = (ushort)(*(byte *)(_stk + 0x33)) + (ushort)(*(byte *)(_stk + 0x32)) * (ushort)_refptr_g_tsaPrmtFlash->bCols;
      (*(bool *)(_stk + 0x25)) = *(char *)((*(longlong *)(_stk + 0x28)) + (ulonglong)(*(ushort *)(_stk + 0x26))) == '\0';
      if ((*(bool *)(_stk + 0x25))) {
        if (param_3 == '\0') {
          if (*(char *)((ulonglong)(*(ushort *)(_stk + 0x26)) + *(longlong *)_refptr_g_tsaBufTzZone) ==
              *(char *)(param_1 + 0x35e)) {
            if (*(char *)((ulonglong)(*(ushort *)(_stk + 0x26)) + *(longlong *)_refptr_g_tsaBufGripStage) != '\x01')
            goto LAB_6ba7e404;
            TZ_TryExtraMergePeak(param_1,(*(byte *)(_stk + 0x33)),(*(byte *)(_stk + 0x32)));
          }
        }
        else if (*(char *)((ulonglong)(*(ushort *)(_stk + 0x26)) + *(longlong *)_refptr_g_tsaBufGripStage) == '\x01')
        {
LAB_6ba7e404:
          (*(short *)(_stk + 0x22)) = *(short *)(*(longlong *)_refptr_g_tsaBufTemp + (ulonglong)(*(ushort *)(_stk + 0x26)) * 2);
          param_2[2] = param_2[2] + 1;
          if (*param_2 < (*(short *)(_stk + 0x22))) {
            *param_2 = (*(short *)(_stk + 0x22));
          }
          if (0 < (*(short *)(_stk + 0x22))) {
            SubTZ_UpdateUnit(param_2,(int)(*(short *)(_stk + 0x22)),(*(byte *)(_stk + 0x33)),(*(byte *)(_stk + 0x32)));
          }
          if (*(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)(*(ushort *)(_stk + 0x26)) * 2) < 0) {
            cVar1 = -1;
          }
          else {
            cVar1 = '\x01';
          }
          *(char *)((ulonglong)(*(ushort *)(_stk + 0x26)) + (*(longlong *)(_stk + 0x28))) =
               (*(char *)(*(longlong *)_refptr_g_ptrAccess + 9) + '\x01') * cVar1;
          SubTZ_SetExtraFlag((*(ushort *)(_stk + 0x26)),(int)(*(short *)(_stk + 0x22)),param_2);
          if ((*(uint *)(_stk + 0x34)) != 0) {
            PosnSpecial_Put((*(ushort *)(_stk + 0x26)));
          }
        }
      }
    }
    (*(byte *)(_stk + 0x33)) = param_4 + ((char *)(_stk + 0x18))[(*(uint *)(_stk + 0x34))];
    (*(byte *)(_stk + 0x32)) = param_5 + ((char *)(_stk + 0x8))[(*(uint *)(_stk + 0x34))];
    (*(uint *)(_stk + 0x34)) = (*(uint *)(_stk + 0x34)) + 1;
  } while( true );
}


// ============================================================================
// Function: SubTZ_GetNumAftFilter @ 6ba7ec0c
// ============================================================================

undefined1 SubTZ_GetNumAftFilter(void)

{
                    /* 0x3ec0c  1457  SubTZ_GetNumAftFilter */
  return DAT_6bbc8681;
}


// ============================================================================
// Function: SubTZ_IsInCurrTZ @ 6ba7d35d
// ============================================================================

undefined1 SubTZ_IsInCurrTZ(longlong param_1,ushort param_2)

{
  undefined1 local_9;
  
  local_9 = 1;
  if ((int)*(char *)((ulonglong)param_2 + *(longlong *)_refptr_g_tsaBufTz) <
      (int)(uint)*(byte *)(param_1 + 0x35e)) {
    local_9 = 0;
  }
  else if ((int)(*(byte *)(param_1 + 0x35e) + 8) <=
           (int)*(char *)((ulonglong)param_2 + *(longlong *)_refptr_g_tsaBufTz)) {
    local_9 = 0;
  }
  return local_9;
}


// ============================================================================
// Function: SubTZ_UpdateUnit @ 6ba7d3cd
// ============================================================================

void SubTZ_UpdateUnit(longlong param_1,short param_2,byte param_3,byte param_4)

{
  int iVar1;
  
  *(int *)(param_1 + 0xc) = *(int *)(param_1 + 0xc) + (int)param_2;
  iVar1 = (int)param_2 * (uint)param_3;
  *(int *)(param_1 + 0x10) = *(int *)(param_1 + 0x10) + iVar1;
  *(longlong *)(param_1 + 0x18) =
       *(longlong *)(param_1 + 0x18) + (longlong)(int)((uint)param_4 * iVar1);
  *(longlong *)(param_1 + 0x20) =
       *(longlong *)(param_1 + 0x20) + (longlong)(int)((uint)param_3 * iVar1);
  *(uint *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + (int)param_2 * (uint)param_4;
  *(longlong *)(param_1 + 0x28) =
       *(longlong *)(param_1 + 0x28) + (longlong)(int)((uint)param_4 * (int)param_2 * (uint)param_4)
  ;
  if (param_3 < *(byte *)(param_1 + 0x39)) {
    *(byte *)(param_1 + 0x39) = param_3;
  }
  if (*(byte *)(param_1 + 0x3a) < param_3) {
    *(byte *)(param_1 + 0x3a) = param_3;
  }
  if (param_4 < *(byte *)(param_1 + 0x3b)) {
    *(byte *)(param_1 + 0x3b) = param_4;
  }
  if (*(byte *)(param_1 + 0x3c) < param_4) {
    *(byte *)(param_1 + 0x3c) = param_4;
  }
  return;
}


// ============================================================================
// Function: SubTZ_SetExtraFlag @ 6ba7d4fe
// ============================================================================

void SubTZ_SetExtraFlag(ushort param_1,short param_2,longlong param_3)

{
  if (((*_refptr_g_tsaStaticPtr)->dwProcessingState & PROC_MS_ACTIVE) != 0) {
    if (*(short *)(_refptr_g_tsaPrmtDynamic + 8) * 3 < param_2 * 4) {
      *(undefined *)(*(longlong *)_refptr_g_tsaBufTzZone + (ulonglong)param_1) =
           _refptr_g_tzAccess[9];
    }
    if (*(byte *)(param_3 + 0x3d) <
        *(byte *)((ulonglong)param_1 + *(longlong *)_refptr_g_tsaBufGripRegion)) {
      *(undefined1 *)(param_3 + 0x3d) =
           *(undefined1 *)((ulonglong)param_1 + *(longlong *)_refptr_g_tsaBufGripRegion);
    }
  }
  return;
}


// ============================================================================
// Function: SubTz_GetSignalThresh @ 6ba7de91
// ============================================================================

short SubTz_GetSignalThresh(short *param_1,undefined8 *param_2)

{
  char cVar1;
  int iVar2;
  short local_a;
  
  cVar1 = TSAStatic_IsAFTModeEnabled();
  if (cVar1 == '\0') {
    if ((*(char *)((longlong)param_1 + 0x34b) != '\0') || ((*(uint *)(param_1 + 0x1a8) & 200) != 0))
    {
      cVar1 = SignalDisparity_IsSignalCompensated();
      if ((cVar1 != '\0') &&
         (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SIGNAL_DISPARITY_POST) != 0)) {
        *param_2 = *(undefined8 *)_refptr_g_tsaBufSd;
        local_a = *(short *)((ulonglong)
                             ((uint)(*_refptr_g_peaksPtr)
                                    [(longlong)(int)(uint)*(byte *)(param_1 + 0x15b) * 0x14 + 0x15].
                                    bPeakCount +
                              (uint)(*_refptr_g_peaksPtr)
                                    [(longlong)(int)(uint)*(byte *)(param_1 + 0x15b) * 0x14 + 0x16].
                                    bPeakCount * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
                            *(longlong *)_refptr_g_tsaBufSd) / 2;
        goto LAB_6ba7e26d;
      }
    }
    if ((*(uint *)(param_1 + 2) & 2) == 0) {
      if (*(int *)(param_1 + 0x1a8) == 8) {
        iVar2 = (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x820 *
                (int)*(short *)(*_refptr_g_peaksPtr +
                               (longlong)(int)(uint)*(byte *)((longlong)param_1 + 0x2b7) * 0x14 +
                               0x18);
        if (iVar2 < 0) {
          iVar2 = iVar2 + 0x7f;
        }
        local_a = (short)(iVar2 >> 7);
      }
      else if (((((uint)(*_refptr_g_peaksPtr)
                        [(longlong)(int)(uint)*(byte *)(param_1 + 0x15b) * 0x14 + 0x15].bPeakCount
                  == _refptr_g_tsaPrmtFlash->bCols - 1) ||
                ((*_refptr_g_peaksPtr)
                 [(longlong)(int)(uint)*(byte *)(param_1 + 0x15b) * 0x14 + 0x15].bPeakCount == '\0')
                ) || ((uint)(*_refptr_g_peaksPtr)
                            [(longlong)(int)(uint)*(byte *)(param_1 + 0x15b) * 0x14 + 0x16].
                            bPeakCount == _refptr_g_tsaPrmtFlash->bRows - 1)) ||
              ((*_refptr_g_peaksPtr)[(longlong)(int)(uint)*(byte *)(param_1 + 0x15b) * 0x14 + 0x16].
               bPeakCount == '\0')) {
        iVar2 = (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x81f *
                (int)*(short *)(*_refptr_g_peaksPtr +
                               (longlong)(int)(uint)*(byte *)((longlong)param_1 + 0x2b7) * 0x14 +
                               0x18);
        if (iVar2 < 0) {
          iVar2 = iVar2 + 0x7f;
        }
        local_a = (short)(iVar2 >> 7);
      }
      else {
        iVar2 = (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x81f *
                (int)*(short *)(*_refptr_g_peaksPtr +
                               (longlong)(int)(uint)*(byte *)(param_1 + 0x15b) * 0x14 + 0x18);
        if (iVar2 < 0) {
          iVar2 = iVar2 + 0x7f;
        }
        local_a = (short)(iVar2 >> 7);
      }
    }
    else {
      iVar2 = (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x81f *
              (int)*(short *)(*_refptr_g_peaksPtr +
                             (longlong)(int)(uint)*(byte *)(param_1 + 0x15b) * 0x14 + 0x18);
      if (iVar2 < 0) {
        iVar2 = iVar2 + 0x7f;
      }
      local_a = (short)(iVar2 >> 7);
    }
  }
  else {
    local_a = *param_1;
  }
LAB_6ba7e26d:
  if (local_a < *param_1) {
    local_a = *param_1;
  }
  return local_a;
}


