#include "TSACoreInternal.h"

// ============================================================================
// Function: TSATTPrmt_Process @ 6bb41b54
// ============================================================================

void TSATTPrmt_Process(void)

{
                    /* 0x101b54  1725  TSATTPrmt_Process */
  TSATTPrmt_UpdataFlashPrmtSig();
  TSATTPrmt_UpdataFlashPrmtTm();
  TSATTPrmt_UpdataFlashPrmtTa();
  TSATTPrmt_UpdataFlashPrmtSys();
  TSATTPrmt_UpdataFlashPrmtTz();
  TSATTPrmt_UpdataFlashPrmtIdt();
  TSATTPrmt_UpdataFlashPrmtCmf();
  TSATTPrmt_UpdataFlashPrmtHiException();
  TSATTPrmt_UpdataFlashPrmtCtd();
  TSATTPrmt_UpdataFlashPrmtProx();
  TSATTPrmt_UpdataFlashPrmtSideTouch();
  TSATTPrmt_UpdataFlashPrmtSensorData();
  TSATTPrmt_UpdateFlashPrmtDisplay();
  return;
}


// ============================================================================
// Function: TSATTPrmt_UpdataFlashPrmtSig @ 6bb40ec5
// ============================================================================

void TSATTPrmt_UpdataFlashPrmtSig(void)

{
  *(short *)&_refptr_g_tsaPrmtFlash->field_0x5da = (short)g_tsaTTPrmt;
  _refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax = (ushort)DAT_6bcaba98;
  *(short *)&_refptr_g_tsaPrmtFlash->field_0x5d8 = (short)DAT_6bcabab0;
  return;
}


// ============================================================================
// Function: TSATTPrmt_UpdataFlashPrmtTm @ 6bb40f18
// ============================================================================

void TSATTPrmt_UpdataFlashPrmtTm(void)

{
  *(undefined4 *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x3c) = DAT_6bcabac8;
  *(short *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x40) = (short)DAT_6bcabae0;
  return;
}


// ============================================================================
// Function: TSATTPrmt_UpdataFlashPrmtTa @ 6bb40f51
// ============================================================================

void TSATTPrmt_UpdataFlashPrmtTa(void)

{
  *(short *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x20) = (short)DAT_6bcabaf8;
  *(char *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x24) = (char)DAT_6bcabb10;
  *(char *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x26) = (char)DAT_6bcabb28;
  *(char *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x28) = (char)DAT_6bcabb40;
  *(char *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x29) = (char)DAT_6bcabb58;
  return;
}


// ============================================================================
// Function: TSATTPrmt_UpdataFlashPrmtSys @ 6bb40fe1
// ============================================================================

void TSATTPrmt_UpdataFlashPrmtSys(void)

{
  _refptr_g_tsaPrmtFlash->dwPeakProcessingMode = DAT_6bcabb70;
  _refptr_g_tsaPrmtFlash->dwAftModeFeatureMask = DAT_6bcabb88;
  *(undefined4 *)&_refptr_g_tsaPrmtFlash->field_0x64 = DAT_6bcabba0;
  return;
}


// ============================================================================
// Function: TSATTPrmt_UpdataFlashPrmtTz @ 6bb41032
// ============================================================================

void TSATTPrmt_UpdataFlashPrmtTz(void)

{
  _refptr_g_tsaPrmtFlash->field_0x81f = (char)DAT_6bcabbb8;
  _refptr_g_tsaPrmtFlash->field_0x820 = (char)DAT_6bcabbd0;
  _refptr_g_tsaPrmtFlash->field_0x836 = (char)DAT_6bcabc90;
  *(undefined4 *)&_refptr_g_tsaPrmtFlash->field_0x838 = DAT_6bcabcd8;
  return;
}


// ============================================================================
// Function: TSATTPrmt_UpdataFlashPrmtIdt @ 6bb410a6
// ============================================================================

void TSATTPrmt_UpdataFlashPrmtIdt(void)

{
  *(short *)&_refptr_g_tsaPrmtFlash[1].field_0x4a = (short)DAT_6bcabbe8;
  return;
}


// ============================================================================
// Function: TSATTPrmt_UpdataFlashPrmtCmf @ 6bb410c9
// ============================================================================

void TSATTPrmt_UpdataFlashPrmtCmf(void)

{
  *(short *)&_refptr_g_tsaPrmtFlash->field_0x626 = (short)DAT_6bcabca8;
  *(short *)&_refptr_g_tsaPrmtFlash->field_0x628 = (short)DAT_6bcabcc0;
  return;
}


// ============================================================================
// Function: TSATTPrmt_UpdataFlashPrmtHiException @ 6bb41109
// ============================================================================

void TSATTPrmt_UpdataFlashPrmtHiException(void)

{
  *(short *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x60) = (short)DAT_6bcabc00;
  *(short *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x62) = (short)DAT_6bcabc18;
  return;
}


// ============================================================================
// Function: TSATTPrmt_UpdataFlashPrmtCtd @ 6bb41149
// ============================================================================

void TSATTPrmt_UpdataFlashPrmtCtd(void)

{
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    _refptr_g_tsaPrmtFlash[1].field_0x4 = (char)DAT_6bcabc30;
    _refptr_g_tsaPrmtFlash[1].field_0x15 = (char)DAT_6bcabc48;
    _refptr_g_tsaPrmtFlash[1].field_0x26 = (char)DAT_6bcabc60;
    *(char *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwPeakProcessingMode + 3) = (char)DAT_6bcabc78;
  }
  else {
    _refptr_g_tsaPrmtFlash[1].field_0x26 = (char)DAT_6bcabc30;
    *(char *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwPeakProcessingMode + 3) = (char)DAT_6bcabc48;
    _refptr_g_tsaPrmtFlash[1].field_0x4 = (char)DAT_6bcabc60;
    _refptr_g_tsaPrmtFlash[1].field_0x15 = (char)DAT_6bcabc78;
  }
  return;
}


// ============================================================================
// Function: TSATTPrmt_UpdataFlashPrmtProx @ 6bb4124b
// ============================================================================

void TSATTPrmt_UpdataFlashPrmtProx(void)

{
  *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x5e0 = DAT_6bcabcf0;
  _refptr_g_tsaPrmtFlash[2].field_0x60c = (char)DAT_6bcabd08;
  *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x5e4 = DAT_6bcabd20;
  *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x5e8 = DAT_6bcabd38;
  *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x5ec = DAT_6bcabd50;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x5f8 = (short)DAT_6bcabd68;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x5fa = (short)DAT_6bcabd80;
  _refptr_g_tsaPrmtFlash[2].field_0x5f0 = (char)DAT_6bcabd98;
  _refptr_g_tsaPrmtFlash[2].field_0x5f1 = (char)DAT_6bcabdb0;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x5fc = (short)DAT_6bcabdc8;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x5fe = (short)DAT_6bcabde0;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x600 = (short)DAT_6bcabdf8;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x602 = (short)DAT_6bcabe10;
  return;
}


// ============================================================================
// Function: TSATTPrmt_UpdataFlashPrmtSideTouch @ 6bb413bd
// ============================================================================

void TSATTPrmt_UpdataFlashPrmtSideTouch(void)

{
  *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x6a0 = DAT_6bcabe40;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x63c = (short)DAT_6bcabe58;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x63e = (short)DAT_6bcabe70;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x640 = (short)DAT_6bcabe88;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x642 = (short)DAT_6bcabea0;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x644 = (short)DAT_6bcabeb8;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x646 = (short)DAT_6bcabed0;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x648 = (short)DAT_6bcabee8;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x64a = (short)DAT_6bcabf00;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x64c = (short)DAT_6bcabf18;
  *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x69c = DAT_6bcabf30;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x684 = (short)DAT_6bcabf48;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x686 = (short)DAT_6bcabf60;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x688 = (short)DAT_6bcabf78;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x68a = (short)DAT_6bcabf90;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x68c = (short)DAT_6bcabfa8;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x68e = (short)DAT_6bcabfc0;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x690 = (short)DAT_6bcabfd8;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x692 = (short)DAT_6bcabff0;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x694 = (short)DAT_6bcac008;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x696 = (short)DAT_6bcac020;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x698 = (short)DAT_6bcac038;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x64e = (short)DAT_6bcac050;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x650 = (short)DAT_6bcac068;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x652 = (short)DAT_6bcac080;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x654 = (short)DAT_6bcac098;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x656 = (short)DAT_6bcac0b0;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x658 = (short)DAT_6bcac0c8;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x65a = (short)DAT_6bcac0e0;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x674 = (short)DAT_6bcac0f8;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x65c = (short)DAT_6bcac110;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x65e = (short)DAT_6bcac128;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x660 = (short)DAT_6bcac140;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x662 = (short)DAT_6bcac158;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x664 = (short)DAT_6bcac170;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x666 = (short)DAT_6bcac188;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x668 = (short)DAT_6bcac1a0;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x66a = (short)DAT_6bcac1b8;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x66c = (short)DAT_6bcac1d0;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x66e = (short)DAT_6bcac1e8;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x670 = (short)DAT_6bcac200;
  *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x678 = DAT_6bcac218;
  *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x67c = DAT_6bcac230;
  *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x680 = DAT_6bcac248;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x6a4 = (short)DAT_6bcac260;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x6a6 = (short)DAT_6bcac278;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x6b4 = (short)DAT_6bcac290;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x6a8 = (short)DAT_6bcac2a8;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x6aa = (short)DAT_6bcac2c0;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x6ac = (short)DAT_6bcac2d8;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x6ae = (short)DAT_6bcac2f0;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x6b0 = (short)DAT_6bcac308;
  *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x6b8 = DAT_6bcac320;
  _refptr_g_tsaPrmtFlash[2].field_0x6bc = (char)DAT_6bcac338;
  _refptr_g_tsaPrmtFlash[2].field_0x6bd = (char)DAT_6bcac350;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x628 = (short)DAT_6bcac368;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x62a = (short)DAT_6bcac380;
  _refptr_g_tsaPrmtFlash[2].field_0x62c = (char)DAT_6bcac398;
  _refptr_g_tsaPrmtFlash[2].field_0x62d = (char)DAT_6bcac3b0;
  _refptr_g_tsaPrmtFlash[2].field_0x62e = (char)DAT_6bcac3c8;
  *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x624 = DAT_6bcac3e0;
  return;
}


// ============================================================================
// Function: TSATTPrmt_UpdataFlashPrmtSensorData @ 6bb41aa0
// ============================================================================

void TSATTPrmt_UpdataFlashPrmtSensorData(void)

{
  *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x6c4 = DAT_6bcac3f8;
  _refptr_g_tsaPrmtFlash[2].field_0x6c8 = (char)DAT_6bcac410;
  return;
}


// ============================================================================
// Function: TSATTPrmt_UpdateFlashPrmtDisplay @ 6bb41ade
// ============================================================================

void TSATTPrmt_UpdateFlashPrmtDisplay(void)

{
  _refptr_g_tsaPrmtFlash->field_0x5ae = (char)DAT_6bcac428;
  _refptr_g_tsaPrmtFlash->field_0x5af = (char)DAT_6bcac440;
  _refptr_g_tsaPrmtFlash->field_0x5b0 = (char)DAT_6bcac458;
  _refptr_g_tsaPrmtFlash->field_0x5b1 = (char)DAT_6bcac470;
  return;
}


// ============================================================================
// Function: TSATTPrmt_Init @ 6bb3df90
// ============================================================================

void TSATTPrmt_Init(void)

{
  byte bVar1;
  byte bVar2;
  byte bVar3;
  byte bVar4;
  int iVar5;
  
                    /* 0xfdf90  1724  TSATTPrmt_Init */
  g_tsaTTPrmt = (int)*(short *)&_refptr_g_tsaPrmtFlash->field_0x5da;
  DAT_6bcaba84 = g_tsaTTPrmt;
  DAT_6bcaba88 = g_tsaTTPrmt * 2;
  iVar5 = g_tsaTTPrmt;
  if ((int)g_tsaTTPrmt < 0) {
    iVar5 = g_tsaTTPrmt + 3;
  }
  DAT_6bcaba8c = g_tsaTTPrmt - (iVar5 >> 2);
  DAT_6bcaba90 = 
  "touch threshold for glove mode;\ntuning based on LTI of smart-cover/thick-glove in device floating situation;\nNOTE: if noise bigger than glove threshold, then glove performance will be sacrificed to ensure no ghost touch;"
  ;
  DAT_6bcaba98 = (int)(short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax;
  DAT_6bcaba9c = DAT_6bcaba98;
  DAT_6bcabaa0 = DAT_6bcaba98 * 2;
  iVar5 = DAT_6bcaba98;
  if ((int)DAT_6bcaba98 < 0) {
    iVar5 = DAT_6bcaba98 + 3;
  }
  DAT_6bcabaa4 = DAT_6bcaba98 - (iVar5 >> 2);
  DAT_6bcabaa8 = 
  "touch threshold for finger mode;\ntuning based on LTI of 4mm finger without film and 5mm finger with film in device floating situation;"
  ;
  DAT_6bcabab0 = (int)*(short *)&_refptr_g_tsaPrmtFlash->field_0x5d8;
  DAT_6bcabab4 = (int)*(short *)&_refptr_g_tsaPrmtFlash->field_0x5d8;
  DAT_6bcabab8 = (short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax * 2;
  DAT_6bcababc = ((short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax * 3) / 2;
  DAT_6bcabac0 = 
  "touch threshold to exit glove mode or confirm finger mode;\nthe higher threshold is the better glove performance but might lead to too sensitive issue for finger;"
  ;
  DAT_6bcabac8 = *(undefined4 *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x3c);
  DAT_6bcabacc = DAT_6bcabac8;
  DAT_6bcabad0 = 0xf;
  DAT_6bcabad4 = 0;
  DAT_6bcabad8 = 
  "define sides to apply glove filter which will filter glove touch with >4 grip ratio;\nNOTE: TOP_SIDE=1,BOTTOM_SIDE=2,LEFT_SIDE=4,RIGHT_SIDE=8;\ntuning based on performance of status-bar drop down and navigation-bar drop up with finger and glove;"
  ;
  DAT_6bcabae0 = (uint)*(ushort *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x40);
  DAT_6bcabae4 = (uint)*(ushort *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x40);
  DAT_6bcabae8 = 0x1e;
  DAT_6bcabaec = 0;
  DAT_6bcabaf0 = 
  "distance threshold in mm to activate glove mode;\nthe less threshold is the easier to enter glove mode and the higher possibility for ghost touch;"
  ;
  DAT_6bcabaf8 = (uint)*(ushort *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x20);
  DAT_6bcabafc = DAT_6bcabaf8;
  DAT_6bcabb00 = DAT_6bcabaf8;
  DAT_6bcabb04 = DAT_6bcabaf8;
  DAT_6bcabb08 = "obsolete;";
  DAT_6bcabb10 = (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x24);
  DAT_6bcabb14 = DAT_6bcabb10;
  DAT_6bcabb18 = 100;
  DAT_6bcabb1c = 0;
  DAT_6bcabb20 = 
  "non-axis-change related moving distance(pixel) threshold to determine click or flick action, which will be applied to determine whether to apply click suppression or not;\nthe more threshold is the better click performance and poorer flick performance will be;"
  ;
  DAT_6bcabb28 = (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x26);
  DAT_6bcabb2c = DAT_6bcabb28;
  DAT_6bcabb30 = 0x96;
  DAT_6bcabb34 = 0;
  DAT_6bcabb38 = 
  "moving distance threshold(pixel) to determine whether to apply click suppression or not;\nthe more threshold is the better click performance and poorer flick performance will be;"
  ;
  DAT_6bcabb40 = (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x28);
  DAT_6bcabb44 = (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x28);
  DAT_6bcabb48 = 0x78;
  DAT_6bcabb4c = 0;
  DAT_6bcabb50 = 
  "non-axis-change related moving distance(pixel) threshold to determine default action of next touch, which will be applied to determine whether to apply click suppression or not by default for next touch;\nthe more threshold is the better click performance and poorer flick performance will be;"
  ;
  DAT_6bcabb58 = (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x29);
  DAT_6bcabb5c = DAT_6bcabb58;
  DAT_6bcabb60 = DAT_6bcabb58;
  DAT_6bcabb64 = DAT_6bcabb58;
  DAT_6bcabb68 = "obsolete;";
  DAT_6bcabb70 = _refptr_g_tsaPrmtFlash->dwPeakProcessingMode;
  DAT_6bcabb74 = _refptr_g_tsaPrmtFlash->dwPeakProcessingMode;
  DAT_6bcabb78 = 3;
  DAT_6bcabb7c = 1;
  DAT_6bcabb80 = 
  "Touch panel sensor architecture, could be OnCell(1), Hybrid Incell(2), Full Incell(3);";
  DAT_6bcabb88 = _refptr_g_tsaPrmtFlash->dwAftModeFeatureMask;
  DAT_6bcabb8c = _refptr_g_tsaPrmtFlash->dwAftModeFeatureMask;
  DAT_6bcabb90 = 0x7fffffff;
  DAT_6bcabb94 = 0;
  DAT_6bcabb98 = "specific in which scenarios that anti-touch feature will be enabled;";
  DAT_6bcabba0 = *(undefined4 *)&_refptr_g_tsaPrmtFlash->field_0x64;
  DAT_6bcabba4 = *(undefined4 *)&_refptr_g_tsaPrmtFlash->field_0x64;
  DAT_6bcabba8 = 0x7fffffff;
  DAT_6bcabbac = 0;
  DAT_6bcabbb0 = "specific in which scenarios that proximity feature will be enabled;";
  DAT_6bcabbb8 = (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x81f;
  DAT_6bcabbbc = (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x81f;
  DAT_6bcabbc0 = 0x60;
  DAT_6bcabbc4 = 0x30;
  DAT_6bcabbc8 = 
  "128 scale up ratio to peak signal to determine touch zone traversal threshold for device grounding situation;\nthe more threshold is the better multi-finger performance and poorer fat-finger performance will be;"
  ;
  DAT_6bcabbd0 = (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x820;
  DAT_6bcabbd4 = (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x820;
  DAT_6bcabbd8 = 0x60;
  DAT_6bcabbdc = 0x10;
  DAT_6bcabbe0 = 
  "128 scale up ratio to peak signal to determine touch zone traversal threshold in case device floating detected;\nthe more threshold is the better multi-finger performance and poorer fat-finger performance will be;"
  ;
  DAT_6bcabbe8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[1].field_0x4a;
  DAT_6bcabbec = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[1].field_0x4a;
  DAT_6bcabbf0 = 0x18;
  DAT_6bcabbf4 = 8;
  DAT_6bcabbf8 = 
  "acceleration threshold to determine whether to do line connection between 2 touches;\nthe more threshold is the better line drawing performance and poorer 2 closed finger tapping performance will be; "
  ;
  DAT_6bcabc90 = (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x836;
  DAT_6bcabc94 = (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x836;
  DAT_6bcabc98 = 0x28;
  DAT_6bcabc9c = 0x1c;
  DAT_6bcabca0 = "threshold of size in mm to determine larger object;";
  DAT_6bcabca8 = (int)*(short *)&_refptr_g_tsaPrmtFlash->field_0x626;
  DAT_6bcabcac = (int)*(short *)&_refptr_g_tsaPrmtFlash->field_0x626;
  DAT_6bcabcb0 = 0x7fff;
  DAT_6bcabcb4 = 0;
  DAT_6bcabcb8 = "common mode noise threshold for sensors\' signal;";
  DAT_6bcabcc0 = (int)*(short *)&_refptr_g_tsaPrmtFlash->field_0x628;
  DAT_6bcabcc4 = (int)*(short *)&_refptr_g_tsaPrmtFlash->field_0x628;
  DAT_6bcabcc8 = 1000;
  DAT_6bcabccc = 0;
  DAT_6bcabcd0 = "common mode noise threshold for delta signal of neighbouring sensor;";
  DAT_6bcabc00 = (int)*(short *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x60);
  DAT_6bcabc04 = (int)*(short *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x60);
  DAT_6bcabc08 = 0x32;
  DAT_6bcabc0c = 10;
  DAT_6bcabc10 = 
  "percentage to HTI threshold to determine charger noise threshold for hybrid incell;";
  DAT_6bcabc18 = (uint)*(ushort *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x62);
  DAT_6bcabc1c = DAT_6bcabc18;
  DAT_6bcabc20 = DAT_6bcabc18;
  DAT_6bcabc24 = DAT_6bcabc18;
  DAT_6bcabc28 = "obsolete;";
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    bVar1 = _refptr_g_tsaPrmtFlash[1].field_0x4;
    bVar2 = _refptr_g_tsaPrmtFlash[1].field_0x15;
    bVar3 = _refptr_g_tsaPrmtFlash[1].field_0x26;
    bVar4 = *(byte *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwPeakProcessingMode + 3);
  }
  else {
    bVar1 = _refptr_g_tsaPrmtFlash[1].field_0x26;
    bVar2 = *(byte *)((longlong)&_refptr_g_tsaPrmtFlash[1].dwPeakProcessingMode + 3);
    bVar3 = _refptr_g_tsaPrmtFlash[1].field_0x4;
    bVar4 = _refptr_g_tsaPrmtFlash[1].field_0x15;
  }
  DAT_6bcabc78 = (uint)bVar4;
  DAT_6bcabc60 = (uint)bVar3;
  DAT_6bcabc48 = (uint)bVar2;
  DAT_6bcabc30 = (uint)bVar1;
  DAT_6bcabc34 = DAT_6bcabc30;
  DAT_6bcabc38 = 0xff;
  DAT_6bcabc3c = 0x10;
  DAT_6bcabc40 = 
  "threshold for centroid top edge correction\n;the less threshold is, the easier to reach edge";
  DAT_6bcabc4c = DAT_6bcabc48;
  DAT_6bcabc50 = 0xff;
  DAT_6bcabc54 = 0x10;
  DAT_6bcabc58 = 
  "threshold for centroid bottom edge correction\n;the less threshold is, the easier to reach edge";
  DAT_6bcabc64 = DAT_6bcabc60;
  DAT_6bcabc68 = 0xff;
  DAT_6bcabc6c = 0x10;
  DAT_6bcabc70 = 
  "threshold for centroid left edge correction\n;the less threshold is, the easier to reach edge";
  DAT_6bcabc7c = DAT_6bcabc78;
  DAT_6bcabc80 = 0xff;
  DAT_6bcabc84 = 0x10;
  DAT_6bcabc88 = 
  "threshold for centroid right edge correction\n;the less threshold is, the easier to reach edge";
  DAT_6bcabcd8 = *(undefined4 *)&_refptr_g_tsaPrmtFlash->field_0x838;
  DAT_6bcabcdc = *(undefined4 *)&_refptr_g_tsaPrmtFlash->field_0x838;
  DAT_6bcabce0 = ((short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax * 3) / 2;
  DAT_6bcabce4 = 0;
  DAT_6bcabce8 = "edge width threshold of ewx/ewy for AFT";
  DAT_6bcabcf0 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x5e0;
  DAT_6bcabcf4 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x5e0;
  DAT_6bcabcf8 = 0x7fffffff;
  DAT_6bcabcfc = 0;
  DAT_6bcabd00 = "proximity feature configuration;";
  DAT_6bcabd08 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x60c;
  DAT_6bcabd0c = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x60c;
  DAT_6bcabd10 = _refptr_g_tsaPrmtFlash->bCols - 2;
  DAT_6bcabd14 = 5;
  DAT_6bcabd18 = "define proximity detection area along Y axis;";
  DAT_6bcabd20 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x5e4;
  DAT_6bcabd24 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x5e4;
  DAT_6bcabd28 = 5000;
  DAT_6bcabd2c = 200;
  DAT_6bcabd30 = 
  "signal threshold to trigger proximity event;\nthe bigger threshold is the better to avoid false trigger but the poorer to trigger proximity;"
  ;
  DAT_6bcabd38 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x5e8;
  DAT_6bcabd3c = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x5e8;
  DAT_6bcabd40 = 5000;
  DAT_6bcabd44 = 200;
  DAT_6bcabd48 = 
  "min threshold for proximity to lift off;\nthe bigger threshold is the better to exit proximity but the poorer to avoid false proximity exit;"
  ;
  DAT_6bcabd50 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x5ec;
  DAT_6bcabd54 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x5ec;
  DAT_6bcabd58 = 10000;
  DAT_6bcabd5c = 500;
  DAT_6bcabd60 = 
  "max threshold for proximity to lift off;\nthe bigger threshold is the better to exit proximity but the poorer to avoid false proximity exit;"
  ;
  DAT_6bcabd68 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5f8;
  DAT_6bcabd6c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5f8;
  DAT_6bcabd70 = 100;
  DAT_6bcabd74 = 5;
  DAT_6bcabd78 = 
  "baseline update speed for proximity;\nthe bigger threshold is the slower to update proximity baseline;"
  ;
  DAT_6bcabd80 = (int)*(short *)&_refptr_g_tsaPrmtFlash[2].field_0x5fa;
  DAT_6bcabd84 = (int)*(short *)&_refptr_g_tsaPrmtFlash[2].field_0x5fa;
  DAT_6bcabd88 = 5000;
  DAT_6bcabd8c = 0x32;
  DAT_6bcabd90 = "baseline update threshold for proximity;";
  DAT_6bcabd98 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x5f0;
  DAT_6bcabd9c = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x5f0;
  DAT_6bcabda0 = 0x10;
  DAT_6bcabda4 = 8;
  DAT_6bcabda8 = 
  "ratio to proximity trigger threshold for min proximity lift off threshold;\nthe bigger threshold is the better to avoid trigger but the poorer for proximity debounce;"
  ;
  DAT_6bcabdb0 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x5f1;
  DAT_6bcabdb4 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x5f1;
  DAT_6bcabdb8 = 0x10;
  DAT_6bcabdbc = 8;
  DAT_6bcabdc0 = 
  "ratio to proximity trigger threshold for touch zone traversal threshold;\nthe bigger threshold is the better to avoid false trigger but the poorer to trigger proximity;"
  ;
  DAT_6bcabdc8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5fc;
  DAT_6bcabdcc = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5fc;
  DAT_6bcabdd0 = 0x78;
  DAT_6bcabdd4 = 0x3c;
  DAT_6bcabdd8 = 
  "scan rate in phone call mode with proximity off;\neither 120 or 60 is selectable to trade off power consumption and touch performance;"
  ;
  DAT_6bcabde0 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5fe;
  DAT_6bcabde4 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5fe;
  DAT_6bcabde8 = 0x78;
  DAT_6bcabdec = 0xf;
  DAT_6bcabdf0 = 
  "scan rate in phone call mode with proximity on;\navailable option [120, 60, 40, 30, 15] to trade off power consumption and proximity performance;"
  ;
  DAT_6bcabdf8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x600;
  DAT_6bcabdfc = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x600;
  DAT_6bcabe00 = 0x78;
  DAT_6bcabe04 = 0x3c;
  DAT_6bcabe08 = 
  "scan rate in screen lock mode with proximity off;\neither 120 or 60 is selectable to trade off power consumption and touch performance;"
  ;
  DAT_6bcabe10 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x602;
  DAT_6bcabe14 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x602;
  DAT_6bcabe18 = 0x78;
  DAT_6bcabe1c = 0xf;
  DAT_6bcabe20 = 
  "scan rate in screen lock mode with proximity on;\navailable option [120, 60, 40, 30, 15] to trade off power consumption and proximity performance;"
  ;
  DAT_6bcabe28 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x610;
  DAT_6bcabe2c = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x610;
  DAT_6bcabe30 = 0x7fffffff;
  DAT_6bcabe34 = 0;
  DAT_6bcabe38 = 
  "in phone call mode, if screen is turned off automatically by either pressing power key,\nor system over proxTimeAutoScreenOff ms since last touch event and without proximity detected,\nthe screen off event will be identify as a proximity event,\nso that algorithm can detect proximity off once phone is away from face;"
  ;
  DAT_6bcabe40 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x6a0;
  DAT_6bcabe44 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x6a0;
  DAT_6bcabe48 = 0x7fffffff;
  DAT_6bcabe4c = 0;
  DAT_6bcabe50 = "volume gesture set";
  DAT_6bcabe58 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x63c;
  DAT_6bcabe5c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x63c;
  DAT_6bcabe60 = 0x7fffffff;
  DAT_6bcabe64 = 0x80000000;
  DAT_6bcabe68 = "volumeFlickUp";
  DAT_6bcabe70 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x63e;
  DAT_6bcabe74 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x63e;
  DAT_6bcabe78 = 0x7fffffff;
  DAT_6bcabe7c = 0x80000000;
  DAT_6bcabe80 = "volumeFlickDown";
  DAT_6bcabe88 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x640;
  DAT_6bcabe8c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x640;
  DAT_6bcabe90 = 0x7fffffff;
  DAT_6bcabe94 = 0x80000000;
  DAT_6bcabe98 = "volumeStepUp";
  DAT_6bcabea0 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x642;
  DAT_6bcabea4 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x642;
  DAT_6bcabea8 = 0x7fffffff;
  DAT_6bcabeac = 0x80000000;
  DAT_6bcabeb0 = "volumeStepDown";
  DAT_6bcabeb8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x644;
  DAT_6bcabebc = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x644;
  DAT_6bcabec0 = 0x7fffffff;
  DAT_6bcabec4 = 0x80000000;
  DAT_6bcabec8 = "volumeActiveAreaStart";
  DAT_6bcabed0 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x646;
  DAT_6bcabed4 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x646;
  DAT_6bcabed8 = 0x7fffffff;
  DAT_6bcabedc = 0x80000000;
  DAT_6bcabee0 = "volumeActiveAreaEnd";
  DAT_6bcabee8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x648;
  DAT_6bcabeec = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x648;
  DAT_6bcabef0 = 0x7fffffff;
  DAT_6bcabef4 = 0x80000000;
  DAT_6bcabef8 = "volumeGreenAreaStart";
  DAT_6bcabf00 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64a;
  DAT_6bcabf04 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64a;
  DAT_6bcabf08 = 0x7fffffff;
  DAT_6bcabf0c = 0x80000000;
  DAT_6bcabf10 = "volumeGreenAreaEnd";
  DAT_6bcabf18 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64c;
  DAT_6bcabf1c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64c;
  DAT_6bcabf20 = 0x7fffffff;
  DAT_6bcabf24 = 0x80000000;
  DAT_6bcabf28 = "volumeSideEnabled";
  DAT_6bcabf30 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x69c;
  DAT_6bcabf34 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x69c;
  DAT_6bcabf38 = 0x7fffffff;
  DAT_6bcabf3c = 0x80000000;
  DAT_6bcabf40 = "volumeSensitivityLevel";
  DAT_6bcabf48 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x684;
  DAT_6bcabf4c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x684;
  DAT_6bcabf50 = 0x7fffffff;
  DAT_6bcabf54 = 0x80000000;
  DAT_6bcabf58 = "volumeActiveAreaStart2";
  DAT_6bcabf60 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x686;
  DAT_6bcabf64 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x686;
  DAT_6bcabf68 = 0x7fffffff;
  DAT_6bcabf6c = 0x80000000;
  DAT_6bcabf70 = "volumeActiveAreaEnd2";
  DAT_6bcabf78 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x688;
  DAT_6bcabf7c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x688;
  DAT_6bcabf80 = 0x7fffffff;
  DAT_6bcabf84 = 0x80000000;
  DAT_6bcabf88 = "volumeGreenAreaStart2";
  DAT_6bcabf90 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x68a;
  DAT_6bcabf94 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x68a;
  DAT_6bcabf98 = 0x7fffffff;
  DAT_6bcabf9c = 0x80000000;
  DAT_6bcabfa0 = "volumeGreenAreaEnd2";
  DAT_6bcabfa8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x68c;
  DAT_6bcabfac = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x68c;
  DAT_6bcabfb0 = 0x7fffffff;
  DAT_6bcabfb4 = 0x80000000;
  DAT_6bcabfb8 = "volumeSideEnabled2";
  DAT_6bcabfc0 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x68e;
  DAT_6bcabfc4 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x68e;
  DAT_6bcabfc8 = 0x7fffffff;
  DAT_6bcabfcc = 0x80000000;
  DAT_6bcabfd0 = "volumeActiveAreaStart3";
  DAT_6bcabfd8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x690;
  DAT_6bcabfdc = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x690;
  DAT_6bcabfe0 = 0x7fffffff;
  DAT_6bcabfe4 = 0x80000000;
  DAT_6bcabfe8 = "volumeActiveAreaEnd3";
  DAT_6bcabff0 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x692;
  DAT_6bcabff4 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x692;
  DAT_6bcabff8 = 0x7fffffff;
  DAT_6bcabffc = 0x80000000;
  DAT_6bcac000 = "volumeGreenAreaStart3";
  DAT_6bcac008 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x694;
  DAT_6bcac00c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x694;
  DAT_6bcac010 = 0x7fffffff;
  DAT_6bcac014 = 0x80000000;
  DAT_6bcac018 = "volumeGreenAreaEnd3";
  DAT_6bcac020 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x696;
  DAT_6bcac024 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x696;
  DAT_6bcac028 = 0x7fffffff;
  DAT_6bcac02c = 0x80000000;
  DAT_6bcac030 = "volumeGreenAreaToMid3";
  DAT_6bcac038 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x698;
  DAT_6bcac03c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x698;
  DAT_6bcac040 = 0x7fffffff;
  DAT_6bcac044 = 0x80000000;
  DAT_6bcac048 = "volumeSideEnabled3";
  DAT_6bcac050 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64e;
  DAT_6bcac054 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64e;
  DAT_6bcac058 = 0x7fffffff;
  DAT_6bcac05c = 0x80000000;
  DAT_6bcac060 = "volumeTouchMin";
  DAT_6bcac068 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x650;
  DAT_6bcac06c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x650;
  DAT_6bcac070 = 0x7fffffff;
  DAT_6bcac074 = 0x80000000;
  DAT_6bcac078 = "volumeTouchMax";
  DAT_6bcac080 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x652;
  DAT_6bcac084 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x652;
  DAT_6bcac088 = 0x7fffffff;
  DAT_6bcac08c = 0x80000000;
  DAT_6bcac090 = "volumeTimeFlick";
  DAT_6bcac098 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x654;
  DAT_6bcac09c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x654;
  DAT_6bcac0a0 = 0x7fffffff;
  DAT_6bcac0a4 = 0x80000000;
  DAT_6bcac0a8 = "volumeTimeOnMin";
  DAT_6bcac0b0 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x656;
  DAT_6bcac0b4 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x656;
  DAT_6bcac0b8 = 0x7fffffff;
  DAT_6bcac0bc = 0x80000000;
  DAT_6bcac0c0 = "volumeTimeOffMin";
  DAT_6bcac0c8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x658;
  DAT_6bcac0cc = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x658;
  DAT_6bcac0d0 = 0x7fffffff;
  DAT_6bcac0d4 = 0x80000000;
  DAT_6bcac0d8 = "volumeTimeOffMax";
  DAT_6bcac0e0 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x65a;
  DAT_6bcac0e4 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x65a;
  DAT_6bcac0e8 = 0x7fffffff;
  DAT_6bcac0ec = 0x80000000;
  DAT_6bcac0f0 = "volumeTimeGUI";
  DAT_6bcac0f8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x674;
  DAT_6bcac0fc = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x674;
  DAT_6bcac100 = 0x7fffffff;
  DAT_6bcac104 = 0x80000000;
  DAT_6bcac108 = "volumeTimeToAntiTrigger";
  DAT_6bcac110 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x65c;
  DAT_6bcac114 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x65c;
  DAT_6bcac118 = 0x7fffffff;
  DAT_6bcac11c = 0x80000000;
  DAT_6bcac120 = "volumeForceAreaStart";
  DAT_6bcac128 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x65e;
  DAT_6bcac12c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x65e;
  DAT_6bcac130 = 0x7fffffff;
  DAT_6bcac134 = 0x80000000;
  DAT_6bcac138 = "volumeForceAreaEnd";
  DAT_6bcac140 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x660;
  DAT_6bcac144 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x660;
  DAT_6bcac148 = 0x7fffffff;
  DAT_6bcac14c = 0x80000000;
  DAT_6bcac150 = "volumeForceAreaGap";
  DAT_6bcac158 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x662;
  DAT_6bcac15c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x662;
  DAT_6bcac160 = 0x7fffffff;
  DAT_6bcac164 = 0x80000000;
  DAT_6bcac168 = "volumeForceAreaGapCenter";
  DAT_6bcac170 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x664;
  DAT_6bcac174 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x664;
  DAT_6bcac178 = 0x7fffffff;
  DAT_6bcac17c = 0x80000000;
  DAT_6bcac180 = "volumeForceAreaDebounce";
  DAT_6bcac188 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x666;
  DAT_6bcac18c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x666;
  DAT_6bcac190 = 0x7fffffff;
  DAT_6bcac194 = 0x80000000;
  DAT_6bcac198 = "volumeForceMoveMax";
  DAT_6bcac1a0 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x668;
  DAT_6bcac1a4 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x668;
  DAT_6bcac1a8 = 0x7fffffff;
  DAT_6bcac1ac = 0x80000000;
  DAT_6bcac1b0 = "volumeForceTouchMax";
  DAT_6bcac1b8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x66a;
  DAT_6bcac1bc = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x66a;
  DAT_6bcac1c0 = 0x7fffffff;
  DAT_6bcac1c4 = 0x80000000;
  DAT_6bcac1c8 = "volumeForceSHBReportYPosDebounce";
  DAT_6bcac1d0 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x66c;
  DAT_6bcac1d4 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x66c;
  DAT_6bcac1d8 = 0x7fffffff;
  DAT_6bcac1dc = 0x80000000;
  DAT_6bcac1e0 = "volumeMappingTimeDelay";
  DAT_6bcac1e8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x66e;
  DAT_6bcac1ec = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x66e;
  DAT_6bcac1f0 = 0x7fffffff;
  DAT_6bcac1f4 = 0x80000000;
  DAT_6bcac1f8 = "volumeMuteMinMove";
  DAT_6bcac200 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x670;
  DAT_6bcac204 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x670;
  DAT_6bcac208 = 0x7fffffff;
  DAT_6bcac20c = 0x80000000;
  DAT_6bcac210 = "volumeMuteMaxTime";
  DAT_6bcac218 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x678;
  DAT_6bcac21c = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x678;
  DAT_6bcac220 = 0x7fffffff;
  DAT_6bcac224 = 0x80000000;
  DAT_6bcac228 = "volumeGUIOnTimeOut";
  DAT_6bcac230 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x67c;
  DAT_6bcac234 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x67c;
  DAT_6bcac238 = 0x7fffffff;
  DAT_6bcac23c = 0x80000000;
  DAT_6bcac240 = "volumeGUIOffTimeOut";
  DAT_6bcac248 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x680;
  DAT_6bcac24c = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x680;
  DAT_6bcac250 = 0x7fffffff;
  DAT_6bcac254 = 0x80000000;
  DAT_6bcac258 = "volumeKeyDebounceTime";
  DAT_6bcac260 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6a4;
  DAT_6bcac264 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6a4;
  DAT_6bcac268 = 0x7fffffff;
  DAT_6bcac26c = 0x80000000;
  DAT_6bcac270 = "powerAreaStart";
  DAT_6bcac278 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6a6;
  DAT_6bcac27c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6a6;
  DAT_6bcac280 = 0x7fffffff;
  DAT_6bcac284 = 0x80000000;
  DAT_6bcac288 = "powerAreaEnd";
  DAT_6bcac290 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6b4;
  DAT_6bcac294 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6b4;
  DAT_6bcac298 = 0x7fffffff;
  DAT_6bcac29c = 0x80000000;
  DAT_6bcac2a0 = "powerTouchMax";
  DAT_6bcac2a8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6a8;
  DAT_6bcac2ac = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6a8;
  DAT_6bcac2b0 = 0x7fffffff;
  DAT_6bcac2b4 = 0x80000000;
  DAT_6bcac2b8 = "powerMoveMax";
  DAT_6bcac2c0 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6aa;
  DAT_6bcac2c4 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6aa;
  DAT_6bcac2c8 = 0x7fffffff;
  DAT_6bcac2cc = 0x80000000;
  DAT_6bcac2d0 = "powerTimeOnMin";
  DAT_6bcac2d8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6ac;
  DAT_6bcac2dc = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6ac;
  DAT_6bcac2e0 = 0x7fffffff;
  DAT_6bcac2e4 = 0x80000000;
  DAT_6bcac2e8 = "powerTimeOnMax";
  DAT_6bcac2f0 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6ae;
  DAT_6bcac2f4 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6ae;
  DAT_6bcac2f8 = 0x7fffffff;
  DAT_6bcac2fc = 0x80000000;
  DAT_6bcac300 = "powerTimeOffMin";
  DAT_6bcac308 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6b0;
  DAT_6bcac30c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6b0;
  DAT_6bcac310 = 0x7fffffff;
  DAT_6bcac314 = 0x80000000;
  DAT_6bcac318 = "powerTimeOffMax";
  DAT_6bcac320 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x6b8;
  DAT_6bcac324 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x6b8;
  DAT_6bcac328 = 0x7fffffff;
  DAT_6bcac32c = 0;
  DAT_6bcac330 = "power gesture set";
  DAT_6bcac338 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x6bc;
  DAT_6bcac33c = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x6bc;
  DAT_6bcac340 = 0x7fffffff;
  DAT_6bcac344 = 0;
  DAT_6bcac348 = "motorVolumeOnWaveform";
  DAT_6bcac350 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x6bd;
  DAT_6bcac354 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x6bd;
  DAT_6bcac358 = 0x7fffffff;
  DAT_6bcac35c = 0;
  DAT_6bcac360 = "motorVolumeOffWaveform";
  DAT_6bcac368 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x628;
  DAT_6bcac36c = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x628;
  DAT_6bcac370 = 0x7fffffff;
  DAT_6bcac374 = 0x80000000;
  DAT_6bcac378 = "sideTouchXCoreStart";
  DAT_6bcac380 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x62a;
  DAT_6bcac384 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x62a;
  DAT_6bcac388 = 0x7fffffff;
  DAT_6bcac38c = 0x80000000;
  DAT_6bcac390 = "sideTouchXCoreEnd";
  DAT_6bcac398 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x62c;
  DAT_6bcac39c = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x62c;
  DAT_6bcac3a0 = 0x7fffffff;
  DAT_6bcac3a4 = 0x80000000;
  DAT_6bcac3a8 = "sideTouchMinGripRatio";
  DAT_6bcac3b0 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x62d;
  DAT_6bcac3b4 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x62d;
  DAT_6bcac3b8 = 0x7fffffff;
  DAT_6bcac3bc = 0x80000000;
  DAT_6bcac3c0 = "sideTouchMidGripRatio";
  DAT_6bcac3c8 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x62e;
  DAT_6bcac3cc = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x62e;
  DAT_6bcac3d0 = 0x7fffffff;
  DAT_6bcac3d4 = 0x80000000;
  DAT_6bcac3d8 = "sideTouchMaxGripRatio";
  DAT_6bcac3e0 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x624;
  DAT_6bcac3e4 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x624;
  DAT_6bcac3e8 = 0x7fffffff;
  DAT_6bcac3ec = 0;
  DAT_6bcac3f0 = "side touch config";
  DAT_6bcac3f8 = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x6c4;
  DAT_6bcac3fc = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x6c4;
  DAT_6bcac400 = 0x7fffffff;
  DAT_6bcac404 = 0x80000000;
  DAT_6bcac408 = "sensorEnabled";
  DAT_6bcac410 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x6c8;
  DAT_6bcac414 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x6c8;
  DAT_6bcac418 = 0x7fffffff;
  DAT_6bcac41c = 0x80000000;
  DAT_6bcac420 = "sensorRate";
  DAT_6bcac428 = (int)(char)_refptr_g_tsaPrmtFlash->field_0x5ae;
  DAT_6bcac42c = (int)(char)_refptr_g_tsaPrmtFlash->field_0x5ae;
  DAT_6bcac430 = 0x7f;
  DAT_6bcac434 = 0xffffff80;
  DAT_6bcac438 = "dim1ClipLow parameter";
  DAT_6bcac440 = (int)(char)_refptr_g_tsaPrmtFlash->field_0x5af;
  DAT_6bcac444 = (int)(char)_refptr_g_tsaPrmtFlash->field_0x5af;
  DAT_6bcac448 = 0x7f;
  DAT_6bcac44c = 0xffffff80;
  DAT_6bcac450 = "dim1ClipHigh parameter";
  DAT_6bcac458 = (int)(char)_refptr_g_tsaPrmtFlash->field_0x5b0;
  DAT_6bcac45c = (int)(char)_refptr_g_tsaPrmtFlash->field_0x5b0;
  DAT_6bcac460 = 0x7f;
  DAT_6bcac464 = 0xffffff80;
  DAT_6bcac468 = "dim2ClipLow parameter";
  DAT_6bcac470 = (int)(char)_refptr_g_tsaPrmtFlash->field_0x5b1;
  DAT_6bcac474 = (int)(char)_refptr_g_tsaPrmtFlash->field_0x5b1;
  DAT_6bcac478 = 0x7f;
  DAT_6bcac47c = 0xffffff80;
  DAT_6bcac480 = "dim2ClipHigh parameter";
  return;
}


