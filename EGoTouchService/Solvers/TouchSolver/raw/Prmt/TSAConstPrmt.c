#include "TSACoreInternal.h"

// ============================================================================
// Function: TSAConstPrmt_Init @ 6bb3bb00
// ============================================================================

void TSAConstPrmt_Init(void)

{
  byte bVar1;
  byte bVar2;
  byte bVar3;
  int local_10;
  
                    /* 0xfbb00  1485  TSAConstPrmt_Init */
  bVar1 = _refptr_g_tsaPrmtFlash->bCols;
  DAT_6bcab620 = (ushort)bVar1 << 8;
  bVar2 = _refptr_g_tsaPrmtFlash->bRows;
  DAT_6bcab622 = (ushort)bVar2 << 8;
  DAT_6bcab628 = (_refptr_g_tsaPrmtFlash->field_0x91 - _refptr_g_tsaPrmtFlash->field_0x90) + '\x01';
  DAT_6bcab629 = (_refptr_g_tsaPrmtFlash->field_0x93 - _refptr_g_tsaPrmtFlash->field_0x92) + '\x01';
  DAT_6bcab62a = (ushort)(byte)_refptr_g_tsaPrmtFlash->field_0x90 << 8;
  DAT_6bcab62c = ((byte)_refptr_g_tsaPrmtFlash->field_0x91 + 1) * 0x100;
  DAT_6bcab62e = (ushort)(byte)_refptr_g_tsaPrmtFlash->field_0x92 << 8;
  DAT_6bcab630 = ((byte)_refptr_g_tsaPrmtFlash->field_0x93 + 1) * 0x100;
  DAT_6bcab632 = (*(short *)&_refptr_g_tsaPrmtFlash->field_0x96 -
                 *(short *)&_refptr_g_tsaPrmtFlash->field_0x94) + 1;
  DAT_6bcab634 = (*(short *)&_refptr_g_tsaPrmtFlash->field_0x9a -
                 *(short *)&_refptr_g_tsaPrmtFlash->field_0x98) + 1;
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    DAT_6bcab625 = _refptr_g_tsaPrmtFlash->bCols;
    DAT_6bcab624 = _refptr_g_tsaPrmtFlash->bRows;
  }
  else {
    DAT_6bcab625 = _refptr_g_tsaPrmtFlash->bRows;
    DAT_6bcab624 = _refptr_g_tsaPrmtFlash->bCols;
  }
  bVar3 = 0;
  for (local_10 = 0; local_10 < (int)(uint)DAT_6bcab625; local_10 = local_10 + 1) {
    if (bVar3 < (byte)(&_refptr_g_tsaPrmtFlash->field_0x62c)[local_10]) {
      bVar3 = (&_refptr_g_tsaPrmtFlash->field_0x62c)[local_10];
    }
  }
  DAT_6bcab626 = bVar3 + 1;
  DAT_6bcab604 = (((int)(char)_refptr_g_tsaPrmtFlash->field_0x5af +
                  (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a6 +
                  (int)(char)_refptr_g_tsaPrmtFlash->field_0x5ae) * 0x10000) / ((uint)bVar1 << 8);
  DAT_6bcab608 = (((int)(char)_refptr_g_tsaPrmtFlash->field_0x5b1 +
                  (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a8 +
                  (int)(char)_refptr_g_tsaPrmtFlash->field_0x5b0) * 0x10000) / ((uint)bVar2 << 8);
  if ((short)_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase1 ==
      *(short *)&_refptr_g_tsaPrmtFlash[1].field_0x4e) {
    DAT_6bcab60c = 1;
  }
  else {
    DAT_6bcab60c = (undefined2)
                   ((((uint)*(byte *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase1 + 3)
                     - (uint)*(byte *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase1 + 2)
                     ) * 0x400) /
                   ((uint)(ushort)_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase1 -
                   (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[1].field_0x4e));
  }
  if (*(short *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase1_or + 2) ==
      (short)_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase1_or) {
    DAT_6bcab60e = 1;
  }
  else {
    DAT_6bcab60e = (undefined2)
                   ((((uint)*(byte *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase2 + 1)
                     - (uint)(byte)_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase2) * 0x400) /
                   ((uint)*(ushort *)
                           ((longlong)&_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase1_or + 2) -
                   (uint)(ushort)_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase1_or));
  }
  if ((short)_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase2_or ==
      *(short *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase2 + 2)) {
    DAT_6bcab610 = 1;
  }
  else {
    DAT_6bcab610 = (undefined2)
                   ((((uint)*(byte *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase2_or +
                                     3) -
                     (uint)*(byte *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase2_or + 2
                                    )) * 0x400) /
                   ((uint)(ushort)_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase2_or -
                   (uint)*(ushort *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwFeatureFlagsBase2 + 2)))
    ;
  }
  if (*(short *)&_refptr_g_tsaPrmtFlash->field_0x5a6 == 0) {
    DAT_6bcab612 = 1;
  }
  else {
    DAT_6bcab612 = (undefined2)
                   (((uint)*(byte *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwAftModeFeatureMask + 1)
                    * (uint)bVar1 * 0x100) / (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a6);
  }
  if (*(short *)&_refptr_g_tsaPrmtFlash->field_0x5a8 == 0) {
    DAT_6bcab614 = 1;
  }
  else {
    DAT_6bcab614 = (undefined2)
                   (((uint)*(byte *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwAftModeFeatureMask + 1)
                    * (uint)bVar2 * 0x100) / (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a8);
  }
  if (*(short *)&_refptr_g_tsaPrmtFlash->field_0x5a6 == 0) {
    DAT_6bcab616 = 1;
  }
  else {
    DAT_6bcab616 = (undefined2)
                   (((uint)(byte)_refptr_g_tsaPrmtFlash[1].dwAftModeFeatureMask *
                    (uint)bVar1 * 0x100) / (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a6);
  }
  if (*(short *)&_refptr_g_tsaPrmtFlash->field_0x5a8 == 0) {
    DAT_6bcab618 = 1;
  }
  else {
    DAT_6bcab618 = (undefined2)
                   (((uint)(byte)_refptr_g_tsaPrmtFlash[1].dwAftModeFeatureMask *
                    (uint)bVar2 * 0x100) / (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x5a8);
  }
  DAT_6bcab61a = (char)(0x1000 / (ulonglong)
                                 (longlong)(int)(uint)(byte)_refptr_g_tsaPrmtFlash->field_0x1b) +
                 '\x01';
  DAT_6bcab61e = (ushort)((0x32400 / (ulonglong)(byte)_refptr_g_tsaPrmtFlash->field_0x1b) /
                         (ulonglong)(byte)_refptr_g_tsaPrmtFlash->field_0x1b);
  DAT_6bcab61c = (short)(0x10000 /
                        (ulonglong)
                        (longlong)
                        ((int)(0x1000 / (ulonglong)
                                        (longlong)
                                        (int)(uint)(byte)_refptr_g_tsaPrmtFlash->field_0x1b) + 1));
  g_tsaPrmtFlashConst =
       (short)(0x100000 /
              (ulonglong)
              ((int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold *
               (uint)DAT_6bcab61e *
               (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x19) *
               (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x19) >> 10));
  DAT_6bcab638 = ((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold * 0x3fff) /
                 (*(int *)&_refptr_g_tsaPrmtFlash->field_0x5fc -
                 *(int *)&_refptr_g_tsaPrmtFlash->field_0x600);
  DAT_6bcab63c = 0x7fff - (DAT_6bcab638 * *(int *)&_refptr_g_tsaPrmtFlash->field_0x5fc) / 0x3fff;
  if ((int)((int)(((int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold +
                   (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d6 +
                   (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d8 +
                  (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5da) * 3) / 2 +
           (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d2) < 0x10000) {
    DAT_6bcab65a = *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x5d2 +
                   (short)((int)(((int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold
                                  + (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d6 +
                                  (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d8 +
                                 (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5da) * 3) / 2);
  }
  else {
    DAT_6bcab65a = -1;
  }
  if ((int)((uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d0 -
           (int)(((int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold +
                  (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d6 +
                  (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d8 +
                 (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5da) * 3) / 2) < 0) {
    DAT_6bcab65c = 0;
  }
  else {
    DAT_6bcab65c = *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x5d0 -
                   (short)((int)(((int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold
                                  + (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d6 +
                                  (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d8 +
                                 (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5da) * 3) / 2);
  }
  if ((int)((int)(((int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold +
                   (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d6 +
                   (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d8 +
                  (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5da) * 3) / 2 +
           (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d4) < 0x8000) {
    DAT_6bcab65e = (short)((int)(((int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold
                                  + (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d6 +
                                  (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d8 +
                                 (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5da) * 3) / 2)
                   + *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x5d4;
  }
  else {
    DAT_6bcab65e = 0x7fff;
  }
  if ((int)(-((int)(((int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold +
                     (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d6 +
                     (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d8 +
                    (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5da) * 3) / 2) -
           (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d4) < -0x8000) {
    DAT_6bcab660 = -0x8000;
  }
  else {
    DAT_6bcab660 = -(short)((int)(((int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold
                                   + (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d6 +
                                   (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5d8 +
                                  (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5da) * 3) / 2)
                   - *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x5d4;
  }
  return;
}


// ============================================================================
// Function: TSAConstPrmt_Process @ 6bb3c698
// ============================================================================

void TSAConstPrmt_Process(void)

{
                    /* 0xfc698  1486  TSAConstPrmt_Process */
  return;
}


