#include "TSACoreInternal.h"

// ============================================================================
// Function: TSADynPrmt_PreProcess @ 6bb3d1e7
// ============================================================================

void TSADynPrmt_PreProcess(void)

{
                    /* 0xfd1e7  1498  TSADynPrmt_PreProcess */
  TSADynPrmt_GetCMFCoef();
  return;
}


// ============================================================================
// Function: TSADynPrmt_Process @ 6bb3d12d
// ============================================================================

void TSADynPrmt_Process(void)

{
  int iVar1;
  
                    /* 0xfd12d  1499  TSADynPrmt_Process */
  if (((*_refptr_g_tsaStaticPtr)->dwProcessingState & 0x10) == 0) {
    TSADynPrmt_CoreProcess();
  }
  else if (*(int *)&_refptr_g_tsaPrmtFlash[2].field_0x69c == 1) {
    iVar1 = *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x634 * 3;
    if (iVar1 < 0) {
      iVar1 = iVar1 + 3;
    }
    TSADynPrmt_SideProcess((int)(short)(iVar1 >> 2));
  }
  else if (*(int *)&_refptr_g_tsaPrmtFlash[2].field_0x69c == 2) {
    TSADynPrmt_SideProcess((int)(*(short *)&_refptr_g_tsaPrmtFlash[2].field_0x634 / 2));
  }
  else {
    TSADynPrmt_SideProcess((int)*(short *)&_refptr_g_tsaPrmtFlash[2].field_0x634);
  }
  return;
}


// ============================================================================
// Function: TSADynPrmt_GetCMFCoef @ 6bb3d0af
// ============================================================================

void TSADynPrmt_GetCMFCoef(void)

{
                    /* 0xfd0af  1489  TSADynPrmt_GetCMFCoef */
  if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x40000) == 0) {
    DAT_6bcab6bc = *(undefined4 *)&_refptr_g_tsaPrmtFlash->field_0x620;
    DAT_6bcab6c0 = *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x624;
  }
  else {
    DAT_6bcab6bc = *(undefined4 *)&_refptr_g_tsaPrmtFlash->field_0x626;
    DAT_6bcab6c0 = *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x62a;
  }
  return;
}


// ============================================================================
// Function: TSADynPrmt_SideProcess @ 6bb3cf6e
// ============================================================================

void TSADynPrmt_SideProcess(short param_1)

{
  int iVar1;
  
                    /* 0xfcf6e  1500  TSADynPrmt_SideProcess */
  iVar1 = param_1 * 3;
  if (iVar1 < 0) {
    iVar1 = iVar1 + 3;
  }
  DAT_6bcab688 = (short)(iVar1 >> 2);
  DAT_6bcab68c = param_1;
  DAT_6bcab690 = (int)((uint)(DAT_6bcab69c >> 4) * (int)param_1) / 2;
  DAT_6bcab694 = DAT_6bcab690;
  DAT_6bcab698 = DAT_6bcab690;
  DAT_6bcab686 = _refptr_g_tsaPrmtFlash[2].field_0x63a;
  return;
}


// ============================================================================
// Function: TSADynPrmt_CoreProcess @ 6bb3d114
// ============================================================================

void TSADynPrmt_CoreProcess(void)

{
                    /* 0xfd114  1487  TSADynPrmt_CoreProcess */
  TSADynPrmt_GetSig();
  TSADynPrmt_GetBL();
  return;
}


// ============================================================================
// Function: TSADynPrmt_Init @ 6bb3c6f2
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void TSADynPrmt_Init(void)

{
                    /* 0xfc6f2  1497  TSADynPrmt_Init */
  _g_tsaPrmtDynamic = *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x5d8;
  _DAT_6bcab688 = *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x5e0;
  _DAT_6bcab690 = *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x5e8;
  _DAT_6bcab698 = *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x5f0;
  _DAT_6bcab6a0 = *(undefined4 *)&_refptr_g_tsaPrmtFlash->field_0x5f8;
  DAT_6bcab6ec = *(undefined1 *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x19);
  DAT_6bcab6f8 = *(undefined8 *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x24);
  DAT_6bcab6f0 = *(undefined8 *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x1c);
  _DAT_6bcab700 = _refptr_g_tsaPrmtFlash[1].pDim1PitchMap[7];
  DAT_6bcab708 = *(undefined4 *)(_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 8);
  DAT_6bcab6bc = *(undefined4 *)&_refptr_g_tsaPrmtFlash->field_0x620;
  DAT_6bcab6c0 = *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x624;
  _DAT_6bcab6a4 = *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x608;
  DAT_6bcab6ac = *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x610;
  _DAT_6bcab6b4 = *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x618;
  DAT_6bcab70c = _refptr_g_tsaPrmtFlash[1].pDim1PitchMap[0xc];
  DAT_6bcab714 = *(undefined4 *)(_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0xd);
  *(short*)TSA_MEM_AT(0x6bcab6daULL) = 2000;
  *(short*)TSA_MEM_AT(0x6bcab6d8ULL) = 2000;
  TouchThold_Init();
  TSADynPrmt_GetUnderWaterThold();
  TSADynPrmt_SSInit();
  return;
}


// ============================================================================
// Function: TSADynPrmt_GetBL @ 6bb3cb4f
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void TSADynPrmt_GetBL(void)

{
  int iVar1;
  short local_c;
  short local_a;
  
                    /* 0xfcb4f  1488  TSADynPrmt_GetBL */
  if (((_refptr_g_tsaPrpt[1].field_0x28 == '\0') ||
      ((((DAT_6bcab700 & 4) == 0 || (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 1) == 0)) &&
       (((DAT_6bcab700 & 8) == 0 || (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x10) == 0))))))
     && (((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) == 0 &&
          ((*_refptr_g_tsaStaticPtr)[1].field_0x3 == '\0')) &&
         ((*_refptr_g_tsaStaticPtr)->bFullIncellSignalDisparity == '\0')))) {
    local_a = *(short *)&_refptr_g_tsaPrpt[1].field_0x1c;
    iVar1 = *(short *)&_refptr_g_tsaPrpt[1].field_0x1c * 3;
    if (iVar1 < 0) {
      iVar1 = iVar1 + 3;
    }
    local_c = (short)(iVar1 >> 2);
  }
  else {
    local_a = *(short *)&_refptr_g_tsaPrpt[1].field_0x18;
    local_c = *(short *)&_refptr_g_tsaPrpt[1].field_0x18;
  }
  if (((*_refptr_g_tsaStaticPtr)->dwProcessingState & 0x10) == 0) {
    TSADynPrmt_UpdateNoiseTholdBfrTch((int)local_a,(int)local_c);
  }
  else {
    local_a = *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x634;
    DAT_6bcab6c6 = (short)((ulonglong)((longlong)(local_a * 2) * 0x55555556) >> 0x20) -
                   (local_a >> 0xf);
  }
  DAT_6bcab6c2 = local_a;
  _DAT_6bcab6c4 = local_c;
  TSADynPrmt_UpdateBfrTch((int)local_a,(int)local_c);
  TSADynPrmt_UpdateStepAftTch((int)local_a,(int)local_c);
  *(short*)TSA_MEM_AT(0x6bcab6d8ULL) =
       ((ushort)(byte)_refptr_g_tsaPrmtFlash->field_0x38 * 4 +
       (ushort)(byte)_refptr_g_tsaPrmtFlash->field_0x38) * 0x28;
  *(short*)TSA_MEM_AT(0x6bcab6daULL) = *(short*)TSA_MEM_AT(0x6bcab6d8ULL);
  *(short*)TSA_MEM_AT(0x6bcab6dcULL) = _refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold;
  *(short*)TSA_MEM_AT(0x6bcab6deULL) = -_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold;
  return;
}


// ============================================================================
// Function: TSADynPrmt_GetSig @ 6bb3d07d
// ============================================================================

void TSADynPrmt_GetSig(void)

{
                    /* 0xfd07d  1494  TSADynPrmt_GetSig */
  TSADynPrmt_GetHTI();
  TSADynPrmt_GetLTI();
  TSADynPrmt_GetMFLTI();
  TSADynPrmt_GetPkThold();
  TSADynPrmt_GetTZThold();
  TSADynPrmt_GetUnderWaterThold();
  return;
}


// ============================================================================
// Function: TSADynPrmt_SSInit @ 6bb3c6a0
// ============================================================================

void TSADynPrmt_SSInit(void)

{
  *(short*)TSA_MEM_AT(0x6bcab6aeULL) = _refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax;
  *(short*)TSA_MEM_AT(0x6bcab6acULL) = (short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax / 3;
  return;
}


// ============================================================================
// Function: TSADynPrmt_GetUnderWaterThold @ 6bb3d023
// ============================================================================

void TSADynPrmt_GetUnderWaterThold(void)

{
                    /* 0xfd023  1496  TSADynPrmt_GetUnderWaterThold */
  DAT_6bcab6a0 = ((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 2) * 3;
  DAT_6bcab6a2 = (short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 2;
  return;
}


// ============================================================================
// Function: TSADynPrmt_UpdateNoiseTholdBfrTch @ 6bb3c849
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void TSADynPrmt_UpdateNoiseTholdBfrTch(short param_1,short param_2)

{
  char cVar1;
  
                    /* 0xfc849  1502  TSADynPrmt_UpdateNoiseTholdBfrTch */
  cVar1 = TouchThold_ToeUseSpecialThold();
  if ((((cVar1 == '\0') || (*(int *)&_refptr_g_tsaPrpt[1].field_0x2c != 2)) ||
      (*(int *)&_refptr_g_tsaPrpt[1].field_0x30 != 2)) ||
     (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) != 0)) {
    cVar1 = TouchThold_ToeUseSpecialThold();
    if (cVar1 == '\0') {
      DAT_6bcab6c6 = param_1 / 2;
    }
    else {
      DAT_6bcab6c6 = (short)((ulonglong)((longlong)(param_1 * 2) * 0x55555556) >> 0x20) -
                     (param_1 >> 0xf);
    }
    _DAT_6bcab6c8 = param_2 / 2;
  }
  else {
    DAT_6bcab6c6 = (short)((ulonglong)((longlong)(param_1 * 2) * 0x55555556) >> 0x20) -
                   (param_1 >> 0xf);
    _DAT_6bcab6c8 =
         (short)((ulonglong)((longlong)(param_2 * 2) * 0x55555556) >> 0x20) - (param_2 >> 0xf);
  }
  return;
}


// ============================================================================
// Function: TSADynPrmt_UpdateBfrTch @ 6bb3c960
// ============================================================================

void TSADynPrmt_UpdateBfrTch(short param_1,short param_2)

{
  short local_a;
  
                    /* 0xfc960  1501  TSADynPrmt_UpdateBfrTch */
  if (param_1 < 0) {
    param_1 = param_1 + 0xff;
  }
  local_a = param_1 >> 8;
  if (local_a < 1) {
    local_a = 1;
  }
  else if (0xff < local_a) {
    local_a = 0xff;
  }
  *(char*)TSA_MEM_AT(0x6bcab6caULL + 5) = (char)local_a;
  if (param_2 < 0) {
    param_2 = param_2 + 0xff;
  }
  local_a = param_2 >> 8;
  if (local_a < 1) {
    local_a = 1;
  }
  else if (0xff < local_a) {
    local_a = 0xff;
  }
  *(char*)TSA_MEM_AT(0x6bcab6caULL + 6) = (char)local_a;
  *(char*)TSA_MEM_AT(0x6bcab6caULL + 4) = 1;
  if (((*_refptr_g_tsaStaticPtr)->dwProcessingState & 0x10) != 0) {
    *(char*)TSA_MEM_AT(0x6bcab6caULL + 5) = *(char*)TSA_MEM_AT(0x6bcab6caULL + 5) * '\x02';
  }
  *(short*)TSA_MEM_AT(0x6bcab6d2ULL + 2) = 0x198;
  return;
}


// ============================================================================
// Function: TSADynPrmt_UpdateStepAftTch @ 6bb3ca49
// ============================================================================

void TSADynPrmt_UpdateStepAftTch(short param_1,short param_2)

{
  short local_a;
  
                    /* 0xfca49  1503  TSADynPrmt_UpdateStepAftTch */
  *(short*)TSA_MEM_AT(0x6bcab6caULL + 0) = param_1 + -1;
  *(short*)TSA_MEM_AT(0x6bcab6caULL + 2) = param_2 + -1;
  if (param_1 < 0) {
    param_1 = param_1 + 0x3f;
  }
  local_a = param_1 >> 6;
  if (local_a < 1) {
    local_a = 1;
  }
  else if (0xff < local_a) {
    local_a = 0xff;
  }
  *(char*)TSA_MEM_AT(0x6bcab6d2ULL + 0) = (char)local_a;
  *(char*)TSA_MEM_AT(0x6bcab6caULL + 7) = 1;
  if (((*_refptr_g_tsaStaticPtr)->dwProcessingState & 0x10) != 0) {
    if ((char)DAT_6bcab6d2 < '\0') {
      *(char*)TSA_MEM_AT(0x6bcab6d2ULL + 0) = -1;
    }
    else {
      *(char*)TSA_MEM_AT(0x6bcab6d2ULL + 0) = (char)DAT_6bcab6d2 * '\x02';
    }
  }
  *(char*)TSA_MEM_AT(0x6bcab6d2ULL + 4) = (byte)_refptr_g_tsaPrmtFlash->field_0x38 >> 1;
  *(char*)TSA_MEM_AT(0x6bcab6d2ULL + 5) = _refptr_g_tsaPrmtFlash->field_0x38;
  return;
}


// ============================================================================
// Function: TSADynPrmt_GetHTI @ 6bb3cd7f
// ============================================================================

void TSADynPrmt_GetHTI(void)

{
                    /* 0xfcd7f  1490  TSADynPrmt_GetHTI */
  return;
}


// ============================================================================
// Function: TSADynPrmt_GetLTI @ 6bb3cd85
// ============================================================================

void TSADynPrmt_GetLTI(void)

{
                    /* 0xfcd85  1491  TSADynPrmt_GetLTI */
  return;
}


// ============================================================================
// Function: TSADynPrmt_GetMFLTI @ 6bb3cd8b
// ============================================================================

void TSADynPrmt_GetMFLTI(void)

{
                    /* 0xfcd8b  1492  TSADynPrmt_GetMFLTI */
  return;
}


// ============================================================================
// Function: TSADynPrmt_GetPkThold @ 6bb3cd91
// ============================================================================

void TSADynPrmt_GetPkThold(void)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0xe]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x10];
  int iVar1;
  
                    /* 0xfcd91  1493  TSADynPrmt_GetPkThold */
  TouchThold_Process((int)(short)(((short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax * 0x38) /
                                 0x50),(int)*(short *)&_refptr_g_tsaPrmtFlash->field_0x5da,&(*(undefined2 *)(_stk + 0x6)),
                     &(*(undefined2 *)(_stk + 0x4)),((undefined1 *)(_stk + 0x2)));
  DAT_6bcab68a = (*(undefined2 *)(_stk + 0x6));
  DAT_6bcab68e = (*(undefined2 *)(_stk + 0x4));
  TouchThold_Process((int)(short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax,
                     (int)*(short *)&_refptr_g_tsaPrmtFlash->field_0x5da,
                     &_refptr_g_tsaPrpt[1].field_0x16,&_refptr_g_tsaPrpt[1].field_0x18,
                     &_refptr_g_tsaPrpt[1].field_0x1e);
  DAT_6bcab69c = _refptr_g_tsaPrmtFlash->field_0x5d0;
  g_tsaPrmtDynamic = _refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold;
  DAT_6bcab688 = *(undefined2 *)&_refptr_g_tsaPrpt[1].field_0x16;
  DAT_6bcab68c = *(undefined2 *)&_refptr_g_tsaPrpt[1].field_0x18;
  iVar1 = (uint)DAT_6bcab69c * (int)*(short *)&_refptr_g_tsaPrpt[1].field_0x18;
  if (iVar1 < 0) {
    iVar1 = iVar1 + 0xf;
  }
  DAT_6bcab690 = iVar1 >> 4;
  DAT_6bcab694 = *(short *)&_refptr_g_tsaPrpt[1].field_0x1c * 3;
  iVar1 = (uint)DAT_6bcab69c * (int)*(short *)&_refptr_g_tsaPrpt[1].field_0x1e;
  if (iVar1 < 0) {
    iVar1 = iVar1 + 0xf;
  }
  DAT_6bcab698 = iVar1 >> 4;
  return;
}


// ============================================================================
// Function: TSADynPrmt_GetTZThold @ 6bb3d018
// ============================================================================

void TSADynPrmt_GetTZThold(void)

{
                    /* 0xfd018  1495  TSADynPrmt_GetTZThold */
  return;
}


