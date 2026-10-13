#include "TSACoreInternal.h"

// ============================================================================
// Function: TS_Process @ 6ba7c2cb
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

void TS_Process(void)

{
  undefined1 bPrevTouchIndex;
  byte bTouchIndex;
  
                    /* 0x3c2cb  2349  TS_Process */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&_rdata,"TS_Process");
  }
  for (bTouchIndex = 0; bTouchIndex < **(byte **)_refptr_g_touchesPtr; bTouchIndex = bTouchIndex + 1
      ) {
    bPrevTouchIndex = PrevTouch_GetIdxBasedOnCurIdx(bTouchIndex);
    if ((*(uint *)((longlong)(int)(uint)bTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                  0x21c) & 0x20) == 0) {
      TS_ProcessUnit(bPrevTouchIndex,bTouchIndex);
    }
  }
  return;
}


// ============================================================================
// Function: TS_PostProcess @ 6ba7c368
// ============================================================================

void TS_PostProcess(void)

{
  byte bVar1;
  byte local_a;
  byte local_9;
  
                    /* 0x3c368  2348  TS_PostProcess */
  for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
    bVar1 = PrevTouch_GetIdxBasedOnCurIdx(local_9);
    if ((*(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c)
        & 0x20) != 0) {
      local_a = *(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                         (longlong)(int)(uint)bVar1 * 0x478 + 0x194);
      if (9 < local_a) {
        local_a = 9;
      }
      _refptr_g_tsaPrmtDynamic[0x6c] = local_a;
    }
  }
  return;
}


// ============================================================================
// Function: TS_ProcessUnit @ 6ba7c1d7
// ============================================================================

void TS_ProcessUnit(byte param_1,byte param_2)

{
  undefined1 uVar1;
  byte local_a;
  byte local_9;
  
                    /* 0x3c1d7  2350  TS_ProcessUnit */
  local_9 = 0;
  local_a = TS_UpdateCurSizeInMM(param_1,param_2);
  uVar1 = TS_UpdateCurSizeInMMForUdfp(param_1,param_2);
  if ((param_1 != 0x14) && (param_1 < 0x14)) {
    local_9 = *(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_1 * 0x478 + 0x194);
  }
  if ((local_a == 0) && (local_9 == 0)) {
    local_a = _refptr_g_tsaPrmtDynamic[0x6c];
  }
  else if (local_a <= local_9) {
    local_a = local_9;
  }
  if (param_2 < 0x14) {
    *(undefined1 *)
     ((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x195) = uVar1;
    *(byte *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x194) =
         local_a;
  }
  return;
}


// ============================================================================
// Function: TS_MMToPixel @ 6ba7c416
// ============================================================================

int TS_MMToPixel(byte param_1)

{
                    /* 0x3c416  2345  TS_MMToPixel */
  return (uint)(byte)_refptr_g_tsaPrmtFlashConst[0x1a] * (uint)param_1;
}


// ============================================================================
// Function: TS_MMToDisplayPixel @ 6ba7c436
// ============================================================================

uint TS_MMToDisplayPixel(byte param_1)

{
  ushort uVar1;
  
                    /* 0x3c436  2344  TS_MMToDisplayPixel */
  uVar1 = *(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a6;
  if (*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a6 <=
      *(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a8) {
    uVar1 = *(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a8;
  }
  return ((uint)param_1 * (uint)uVar1 * 0x10) / (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x2e
  ;
}


// ============================================================================
// Function: TS_UpdateCurSizeInMM @ 6ba7c006
// ============================================================================

undefined1 TS_UpdateCurSizeInMM(undefined8 param_1,byte param_2)

{
  undefined1 local_9;
  
  local_9 = 0;
  if ((param_2 != 0x14) && (param_2 < 0x14)) {
    local_9 = TS_GetSizeInMM((int)*(short *)_refptr_g_tsaPrmtDynamic,
                             *(undefined4 *)
                              ((longlong)(int)(uint)param_2 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x1cc));
    if ((ushort)(((byte)_refptr_g_tsaPrmtFlash->field_0x1b >> 3) + 2) <
        *(ushort *)
         ((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x188)) {
      local_9 = (undefined1)
                *(undefined2 *)
                 ((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x188);
    }
  }
  return local_9;
}


// ============================================================================
// Function: TS_UpdateCurSizeInMMForUdfp @ 6ba7c0d6
// ============================================================================

byte TS_UpdateCurSizeInMMForUdfp(undefined8 param_1,byte param_2)

{
  int iVar1;
  byte local_9;
  
  local_9 = 0;
  if ((param_2 != 0x14) && (param_2 < 0x14)) {
    local_9 = TS_GetSizeInMM((int)*(short *)_refptr_g_tsaPrmtDynamic,
                             *(undefined4 *)
                              ((longlong)(int)(uint)param_2 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x1cc));
    iVar1 = Misc_SqrtUint32((uint)*(ushort *)
                                   ((longlong)(int)(uint)param_2 * 0x168 +
                                    *(longlong *)_refptr_g_touchesPtr + 0x18a) *
                            (uint)*(ushort *)
                                   ((longlong)(int)(uint)param_2 * 0x168 +
                                    *(longlong *)_refptr_g_touchesPtr + 0x188));
    if (local_9 < 0xb) {
      if (6 < local_9) {
        local_9 = (byte)(iVar1 * (local_9 - 6) + (uint)local_9 * (4 - (local_9 - 6)) >> 2);
      }
    }
    else {
      local_9 = (byte)iVar1;
    }
  }
  return local_9;
}


// ============================================================================
// Function: TS_Init @ 6ba7c000
// ============================================================================

void TS_Init(void)

{
                    /* 0x3c000  2343  TS_Init */
  return;
}


// ============================================================================
// Function: TS_GetSizeInMM @ 6ba7c692
// ============================================================================

ulonglong TS_GetSizeInMM(short param_1,int param_2)

{
  ushort uVar1;
  ulonglong uVar2;
  byte local_9;
  
                    /* 0x3c692  2341  TS_GetSizeInMM */
  if (param_2 < 0x200000) {
    local_9 = 1;
    do {
      uVar1 = *(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x1e);
      if (0x1fffff < param_2) {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                          "../../src/Alg/TouchSize/TS.c",0xe8,
                          "sigSum <= (TDIFS_MAX >> (TS_UNIT_PER_SIG_MM2_SCALE_SHIFTER))");
        HardwareAnalyzer_SetSoftException();
      }
    } while (((int)param_1 * (uint)uVar1 * (uint)(byte)(local_9 + local_9 * local_9) <=
              (uint)(param_2 << 10)) && (local_9 = local_9 + 1, local_9 < 0xf));
    uVar2 = (ulonglong)local_9;
  }
  else {
    uVar2 = 0xffffffff;
  }
  return uVar2;
}


// ============================================================================
// Function: TS_SQPixelToSQMM @ 6ba7c4a0
// ============================================================================

ulonglong TS_SQPixelToSQMM(uint param_1)

{
  ulonglong uVar1;
  
                    /* 0x3c4a0  2352  TS_SQPixelToSQMM */
  uVar1 = (ulonglong)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x1c) *
          (ulonglong)param_1 * (ulonglong)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x1c) >> 0x20;
  if (0xffff < uVar1) {
    uVar1 = 0xffffffff;
  }
  return uVar1;
}


// ============================================================================
// Function: TS_PixelToMMScale10 @ 6ba7c5a7
// ============================================================================

int TS_PixelToMMScale10(int param_1)

{
  uint uVar1;
  int iVar2;
  int local_res8;
  
                    /* 0x3c5a7  2347  TS_PixelToMMScale10 */
  local_res8 = param_1;
  if (param_1 < 0) {
    local_res8 = -param_1;
  }
  uVar1 = (uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x1c) * local_res8 * 10;
  iVar2 = (uVar1 >> 0x10) + (uint)((byte)(uVar1 >> 0xf) & 1);
  if (param_1 < 0) {
    iVar2 = -iVar2;
  }
  return iVar2;
}


// ============================================================================
// Function: TS_PixelToMM @ 6ba7c561
// ============================================================================

int TS_PixelToMM(ushort param_1)

{
                    /* 0x3c561  2346  TS_PixelToMM */
  return ((uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x1c) * (uint)param_1 >> 0x10) +
         (uint)((byte)((uint)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x1c) * (uint)param_1 >> 0xf)
               & 1);
}


// ============================================================================
// Function: TS_SQMMToSQPixel @ 6ba7c4fb
// ============================================================================

ulonglong TS_SQMMToSQPixel(ushort param_1)

{
                    /* 0x3c4fb  2351  TS_SQMMToSQPixel */
  return (((ulonglong)param_1 << 0x20) / (ulonglong)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x1c))
         / (ulonglong)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x1c);
}


