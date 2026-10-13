#include "TSACoreInternal.h"

// ============================================================================
// Function: RFNoise_CopyPrevRaw2RawDif @ 6bb4463a
// ============================================================================

void RFNoise_CopyPrevRaw2RawDif(void)

{
  errno_t eVar1;
  
                    /* 0x10463a  1141  RFNoise_CopyPrevRaw2RawDif */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags2 & 0x10) != 0) {
    eVar1 = memcpy_s(g_tsaBufPrevRaw2rawDif,(ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,
                     *(void **)_refptr_g_tsaBufRaw2rawDif,
                     (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2);
    if (eVar1 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/WorkAround/RFNoise/RFNoise.c",0x43,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
  }
  return;
}


// ============================================================================
// Function: RFNoise_IsDetected @ 6bb44c44
// ============================================================================

undefined1 RFNoise_IsDetected(void)

{
  undefined1 local_9;
  
                    /* 0x104c44  1147  RFNoise_IsDetected */
  local_9 = 0;
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    if (*(short *)(_refptr_g_tsaPrmtDynamic + 8) < g_maxAbsCMFNoiseDim2) {
      local_9 = 1;
    }
  }
  else if (*(short *)(_refptr_g_tsaPrmtDynamic + 8) < g_maxAbsCMFNoiseDim1) {
    local_9 = 1;
  }
  return local_9;
}


// ============================================================================
// Function: RFNoise_Init @ 6bb444b6
// ============================================================================

void RFNoise_Init(void)

{
  int iVar1;
  
                    /* 0x1044b6  1146  RFNoise_Init */
  iVar1 = memset_s(g_tsaBufPrevRaw2rawDif,(ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,0,
                   (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/WorkAround/RFNoise/RFNoise.c",0x31,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(g_tsaBufRaw2rawDif2,(ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,0,
                   (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/WorkAround/RFNoise/RFNoise.c",0x33,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(g_tsaBufRaw2rawDif3,(ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,0,
                   (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/WorkAround/RFNoise/RFNoise.c",0x35,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  g_maxAbsCMFNoiseDim1 = 0;
  g_maxAbsCMFNoiseDim2 = 0;
  g_numCMNLineDim1 = 0;
  g_numCMNLineDim2 = 0;
  return;
}


// ============================================================================
// Function: RFNoise_GetNoiseValue @ 6bb44ca1
// ============================================================================

undefined2 RFNoise_GetNoiseValue(void)

{
  undefined2 uVar1;
  
                    /* 0x104ca1  1144  RFNoise_GetNoiseValue */
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    uVar1 = g_maxAbsCMFNoiseDim2;
  }
  else {
    uVar1 = g_maxAbsCMFNoiseDim1;
  }
  return uVar1;
}


// ============================================================================
// Function: RFNoise_GetNumNoisyLine @ 6bb44cc6
// ============================================================================

undefined1 RFNoise_GetNumNoisyLine(void)

{
  undefined1 uVar1;
  
                    /* 0x104cc6  1145  RFNoise_GetNumNoisyLine */
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    uVar1 = g_numCMNLineDim2;
  }
  else {
    uVar1 = g_numCMNLineDim1;
  }
  return uVar1;
}


// ============================================================================
// Function: RFNoise_Detect @ 6bb44ceb
// ============================================================================

undefined1 RFNoise_Detect(void)

{
  undefined1 local_9;
  
                    /* 0x104ceb  1142  RFNoise_Detect */
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    local_9 = RFNoise_Raw2RawProcessDim(2,&g_maxAbsCMFNoiseDim2,&g_numCMNLineDim2);
  }
  else {
    local_9 = RFNoise_Raw2RawProcessDim(1,&g_maxAbsCMFNoiseDim1,&g_numCMNLineDim1);
  }
  return local_9;
}


// ============================================================================
// Function: RFNoise_Raw2RawProcessDim @ 6bb44a73
// ============================================================================

undefined1 RFNoise_Raw2RawProcessDim(int param_1,short *param_2,char *param_3)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x28]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x30];
  char cVar1;
  
  (*(short *)(_stk + 0x24)) = -0x8000;
  (*(short *)(_stk + 0x22)) = -0x8000;
  (*(short *)(_stk + 0x20)) = -0x8000;
  (*(int *)(_stk + 0x1c)) = 0;
  (*(int *)(_stk + 0x18)) = 0;
  (*(char *)(_stk + 0x17)) = '\0';
  (*(undefined1 *)(_stk + 0x16)) = 0;
  if (param_1 == 1) {
    (*(byte *)(_stk + 0x27)) = _refptr_g_tsaPrmtFlash->bRows;
  }
  else {
    (*(byte *)(_stk + 0x27)) = _refptr_g_tsaPrmtFlash->bCols;
  }
  for ((*(byte *)(_stk + 0x26)) = 0; (*(byte *)(_stk + 0x26)) < (*(byte *)(_stk + 0x27)); (*(byte *)(_stk + 0x26)) = (*(byte *)(_stk + 0x26)) + 1) {
    RFNoise_Raw2RawProcessDimUnit(param_1,(*(byte *)(_stk + 0x26)),&(*(short *)(_stk + 0x12)),&(*(short *)(_stk + 0x10)),&(*(short *)(_stk + 0xe)),&(*(int *)(_stk + 0x8)));
    if ((short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax < (*(short *)(_stk + 0x12))) {
      (*(char *)(_stk + 0x17)) = (*(char *)(_stk + 0x17)) + '\x01';
      if ((*(byte *)(_stk + 0x26)) < (*(byte *)(_stk + 0x27)) >> 1) {
        (*(int *)(_stk + 0x1c)) = (*(int *)(_stk + 0x1c)) + (*(int *)(_stk + 0x8));
      }
      else {
        (*(int *)(_stk + 0x18)) = (*(int *)(_stk + 0x18)) + (*(int *)(_stk + 0x8));
      }
    }
    if (*(short *)&_refptr_g_tsaPrpt[1].field_0x14 < (*(short *)(_stk + 0x12))) {
      if ((*(byte *)(_stk + 0x26)) < (*(byte *)(_stk + 0x27)) >> 1) {
        (*(int *)(_stk + 0x1c)) = (*(int *)(_stk + 0x1c)) + (*(int *)(_stk + 0x8));
      }
      else {
        (*(int *)(_stk + 0x18)) = (*(int *)(_stk + 0x18)) + (*(int *)(_stk + 0x8));
      }
    }
    if ((*(short *)(_stk + 0x24)) < (*(short *)(_stk + 0x12))) {
      (*(short *)(_stk + 0x24)) = (*(short *)(_stk + 0x12));
    }
    if ((*(short *)(_stk + 0x22)) < (*(short *)(_stk + 0x10))) {
      (*(short *)(_stk + 0x22)) = (*(short *)(_stk + 0x10));
    }
    if ((*(short *)(_stk + 0x20)) < (*(short *)(_stk + 0xe))) {
      (*(short *)(_stk + 0x20)) = (*(short *)(_stk + 0xe));
    }
  }
  (*(short *)(_stk + 0x14)) = (*(short *)(_stk + 0x24));
  if ((*(short *)(_stk + 0x22)) <= (*(short *)(_stk + 0x24))) {
    (*(short *)(_stk + 0x14)) = (*(short *)(_stk + 0x22));
  }
  if ((*(short *)(_stk + 0x20)) <= (*(short *)(_stk + 0x14))) {
    (*(short *)(_stk + 0x14)) = (*(short *)(_stk + 0x20));
  }
  if (((*(char *)(_stk + 0x17)) != '\0') ||
     (_refptr_g_tsaPrmtFlash->wBufElementCount >> 1 < *(ushort *)&_refptr_g_tsaPrpt->field_0x4)) {
    cVar1 = RFNoise_IsVariationInRangeUpDown((*(int *)(_stk + 0x1c)),-(*(int *)(_stk + 0x18)),4);
    if (cVar1 != '\0') {
      (*(undefined1 *)(_stk + 0x16)) = 1;
    }
  }
  *param_2 = (*(short *)(_stk + 0x14));
  *param_3 = (*(char *)(_stk + 0x17));
  return (*(undefined1 *)(_stk + 0x16));
}


// ============================================================================
// Function: RFNoise_Raw2RawProcessDimUnit @ 6bb446d9
// ============================================================================

void RFNoise_Raw2RawProcessDimUnit
               (int param_1,byte param_2,short *param_3,short *param_4,short *param_5,int *param_6)

{
  short sVar1;
  short sVar2;
  short local_1c;
  short local_1a;
  short local_18;
  short local_16;
  short local_14;
  short local_12;
  char local_f;
  byte local_e;
  byte local_d;
  ushort local_c;
  ushort local_a;
  
  local_12 = -0x8000;
  local_14 = 0x7fff;
  local_16 = -0x8000;
  local_18 = 0x7fff;
  local_1a = -0x8000;
  local_1c = 0x7fff;
  *param_6 = 0;
  *param_3 = 0;
  *param_4 = 0;
  *param_5 = 0;
  if (param_1 == 1) {
    local_a = (ushort)_refptr_g_tsaPrmtFlash->bCols * (ushort)param_2;
    local_d = 1;
    local_e = _refptr_g_tsaPrmtFlash->bCols;
  }
  else {
    local_a = (ushort)param_2;
    local_d = _refptr_g_tsaPrmtFlash->bCols;
    local_e = _refptr_g_tsaPrmtFlash->bRows;
  }
  local_c = local_a;
  for (local_f = '\0'; (int)local_f < (int)(uint)local_e; local_f = local_f + '\x01') {
    if ((*_refptr_g_tsaStaticPtr)->bField_0x11a == '\0') {
      sVar2 = *(short *)(*(longlong *)_refptr_g_tsaBufPreHWRaw + (ulonglong)local_c * 2) -
              *(short *)(*(longlong *)_refptr_g_tsaBufRaw + (ulonglong)local_c * 2);
    }
    else {
      sVar2 = *(short *)(*(longlong *)_refptr_g_tsaBufRaw + (ulonglong)local_c * 2) -
              *(short *)(*(longlong *)_refptr_g_tsaBufPreHWRaw + (ulonglong)local_c * 2);
    }
    if (local_12 < sVar2) {
      local_12 = sVar2;
    }
    if (sVar2 < local_14) {
      local_14 = sVar2;
    }
    *param_6 = *param_6 + (int)sVar2;
    sVar1 = sVar2 + *(short *)(g_tsaBufPrevRaw2rawDif + (ulonglong)local_c * 2);
    if (local_16 < sVar1) {
      local_16 = sVar1;
    }
    if (sVar1 < local_18) {
      local_18 = sVar1;
    }
    sVar2 = *(short *)(g_tsaBufRaw2rawDif2 + (ulonglong)local_c * 2) +
            *(short *)(g_tsaBufPrevRaw2rawDif + (ulonglong)local_c * 2) + sVar2;
    if (local_1a < sVar2) {
      local_1a = sVar2;
    }
    if (sVar2 < local_1c) {
      local_1c = sVar2;
    }
    *(undefined2 *)((ulonglong)local_c * 2 + g_tsaBufRaw2rawDif3) =
         *(undefined2 *)(g_tsaBufRaw2rawDif2 + (ulonglong)local_c * 2);
    *(undefined2 *)((ulonglong)local_c * 2 + g_tsaBufRaw2rawDif2) =
         *(undefined2 *)(g_tsaBufPrevRaw2rawDif + (ulonglong)local_c * 2);
    local_c = local_c + local_d;
  }
  if (0 < local_14) {
    *param_3 = local_14;
  }
  if (local_12 < 0) {
    *param_3 = -local_12;
  }
  if (0 < local_18) {
    *param_4 = local_18;
  }
  if (local_16 < 0) {
    *param_4 = -local_16;
  }
  if (0 < local_1c) {
    *param_5 = local_1c;
  }
  if (local_1a < 0) {
    *param_5 = -local_1a;
  }
  return;
}


// ============================================================================
// Function: RFNoise_IsVariationInRangeUpDown @ 6bb449e6
// ============================================================================

undefined8 RFNoise_IsVariationInRangeUpDown(int param_1,int param_2,byte param_3)

{
  int local_res8;
  int local_res10;
  byte local_res18;
  
  local_res8 = param_1;
  local_res10 = param_2;
  if ((param_1 < 0) && (param_2 < 0)) {
    local_res8 = -param_1;
    local_res10 = -param_2;
  }
  local_res18 = param_3;
  if (param_3 == 0) {
    local_res18 = 1;
  }
  if ((0 < local_res8) && (0 < local_res10)) {
    if (local_res10 < local_res8) {
      if (local_res8 - local_res8 / (int)(uint)local_res18 < local_res10) {
        return 1;
      }
    }
    else if (local_res10 - local_res10 / (int)(uint)local_res18 < local_res8) {
      return 1;
    }
  }
  return 0;
}


// ============================================================================
// Function: RFNoise_Free @ 6bb44483
// ============================================================================

void RFNoise_Free(void)

{
                    /* 0x104483  1143  RFNoise_Free */
  TSA_Free(&g_tsaBufPrevRaw2rawDif);
  TSA_Free(&g_tsaBufRaw2rawDif2);
  TSA_Free(&g_tsaBufRaw2rawDif3);
  return;
}


// ============================================================================
// Function: RFNoise_Malloc @ 6bb443e0
// ============================================================================

undefined8 RFNoise_Malloc(void)

{
  undefined8 uVar1;
  
                    /* 0x1043e0  1148  RFNoise_Malloc */
  g_tsaBufPrevRaw2rawDif = TSA_MallocAndInit((uint)*(ushort *)_refptr_g_gridBufSize * 2);
  if (g_tsaBufPrevRaw2rawDif == 0) {
    uVar1 = 0;
  }
  else {
    g_tsaBufRaw2rawDif2 = TSA_MallocAndInit((uint)*(ushort *)_refptr_g_gridBufSize * 2);
    if (g_tsaBufRaw2rawDif2 == 0) {
      uVar1 = 0;
    }
    else {
      g_tsaBufRaw2rawDif3 = TSA_MallocAndInit((uint)*(ushort *)_refptr_g_gridBufSize * 2);
      if (g_tsaBufRaw2rawDif3 == 0) {
        uVar1 = 0;
      }
      else {
        uVar1 = 1;
      }
    }
  }
  return uVar1;
}


