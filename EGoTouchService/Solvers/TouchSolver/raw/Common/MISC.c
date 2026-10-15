#include "TSACoreInternal.h"

// ============================================================================
// Function: CaculateInterval @ 6bade69c
// ============================================================================

void CaculateInterval(void)

{
  g_uSTimeInterval = *(int *)(*(longlong *)(_refptr_g_thpAFEStruct + 0x10) + 4) - g_prevUsTimeStamp;
  g_prevUsTimeStamp = (longlong)*(int *)(*(longlong *)(_refptr_g_thpAFEStruct + 0x10) + 4);
  return;
}


// ============================================================================
// Function: SensorPitchSizeMapDim1 @ 6bb060b4
// ============================================================================

int SensorPitchSizeMapDim1(int sensorCoord,ushort scale)

{
  int iVar1;
  
                    /* 0xc60b4  1286  SensorPitchSizeMapDim1 */
  iVar1 = SensorPitchSizeMap(sensorCoord,_refptr_g_tsaPrmtFlash->pDim1PitchMap,scale);
  return iVar1;
}


// ============================================================================
// Function: SensorPitchSizeMapDim2 @ 6bb060e8
// ============================================================================

int SensorPitchSizeMapDim2(int sensorCoord,ushort scale)

{
  int iVar1;
  
                    /* 0xc60e8  1287  SensorPitchSizeMapDim2 */
  iVar1 = SensorPitchSizeMap(sensorCoord,_refptr_g_tsaPrmtFlash->pDim2PitchMap,scale);
  return iVar1;
}


// ============================================================================
// Function: Bezier @ 6ba6595d
// ============================================================================

int Bezier(int param_1,int param_2,int param_3)

{
                    /* 0x2595d  406  Bezier */
  param_1 = (param_3 * 3 + param_2 * 6) - param_1;
  if (param_1 < 0) {
    param_1 = param_1 + 7;
  }
  return param_1 >> 3;
}


// ============================================================================
// Function: SensorPitchSizeMap @ 6bb05fc0
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int SensorPitchSizeMap(int param_1,double *param_2,ushort param_3)

{
  ushort uVar1;
  
  if (*param_2 != 100.0) {
    if (param_3 == 0) {
      param_1 = 0;
    }
    else {
      uVar1 = (ushort)(param_1 / (int)(uint)param_3);
      if (uVar1 < 0x4f) {
        param_1 = (int)(param_2[(ulonglong)uVar1 + 1] *
                        (double)(param_1 % (int)(uint)param_3 & 0xffff) +
                       param_2[uVar1] *
                       (double)(int)((uint)param_3 - (param_1 % (int)(uint)param_3 & 0xffffU)));
      }
      else {
        param_1 = 0;
      }
    }
  }
  return param_1;
}


// ============================================================================
// Function: SinXDivX @ 6ba8573e
// ============================================================================

double SinXDivX(double param_1)

{
  /* .rdata 常量是 float：0x6bb52ecc = -1.0f（三次卷积核的 a），0x6bb52ee0.. = 2/3/5/8/4。
     原汇编先求 x² 再求 x³ = x²·x，两段多项式都乘 x³。 */
  float fVar0;
  double dVar1;
  double dVar2;
  double dVar3;
  double local_res8;

  fVar0 = *(float *)&DAT_6bb52ecc;
  local_res8 = param_1;
  if (param_1 < 0.0) {
    local_res8 = -param_1;
  }
  dVar2 = local_res8 * local_res8;
  dVar3 = dVar2 * local_res8;
  if (local_res8 <= 1.0) {
    dVar1 = ((double)(fVar0 + *(float *)&DAT_6bb52ee0) * dVar3 -
            (double)(fVar0 + *(float *)&DAT_6bb52ee4) * dVar2) + 1.0;
  }
  else if (local_res8 <= 2.0) {
    dVar1 = (((double)fVar0 * dVar3 - (double)(fVar0 * *(float *)&DAT_6bb52ee8) * dVar2) +
            (double)(fVar0 * *(float *)&DAT_6bb52eec) * local_res8) -
            (double)(fVar0 * *(float *)&DAT_6bb52ef0);
  }
  else {
    dVar1 = 0.0;
  }
  return dVar1;
}


// ============================================================================
// Function: Misc_SqrtUint32 @ 6ba64d8b
// ============================================================================

int Misc_SqrtUint32(uint param_1)

{
  int local_10;
  uint local_c;
  
                    /* 0x24d8b  927  Misc_SqrtUint32 */
  local_10 = 0;
  for (local_c = 0x8000; param_1 < local_c; local_c = local_c >> 1) {
  }
  for (; local_c != 0; local_c = local_c >> 1) {
    local_10 = local_10 + local_c;
    if (param_1 < (uint)(local_10 * local_10)) {
      local_10 = local_10 - local_c;
    }
  }
  return local_10;
}


