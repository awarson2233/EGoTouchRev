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
  
  if (*param_2 != __rdata) {
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
// Function: RefreshClipPrmt @ 6bb26b52
// ============================================================================

undefined8 RefreshClipPrmt(longlong param_1)

{
  short sVar1;
  short sVar2;
  short sVar3;
  short sVar4;
  short sVar5;
  short sVar6;
  short sVar7;
  short sVar8;
  
  sVar1 = *(short *)(param_1 + 0x10);
  sVar5 = GetSensorStartDspl((int)(char)_refptr_g_tsaPrmtFlash->field_0x5ae,
                             (int)(char)_refptr_g_tsaPrmtFlash->field_0x5af,
                             *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5aa,
                             _refptr_g_tsaPrmtFlash->field_0x1f,*(undefined1 *)(param_1 + 10));
  sVar6 = GetSensorStartDspl((int)(char)_refptr_g_tsaPrmtFlash->field_0x5ae,
                             (int)(char)_refptr_g_tsaPrmtFlash->field_0x5af,
                             *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5aa,
                             _refptr_g_tsaPrmtFlash->field_0x1f,
                             *(char *)(param_1 + 8) + *(char *)(param_1 + 10));
  sVar2 = *(short *)(param_1 + 0x10);
  sVar3 = *(short *)(param_1 + 0xc);
  sVar4 = *(short *)(param_1 + 0x12);
  sVar7 = GetSensorStartDspl((int)(char)_refptr_g_tsaPrmtFlash->field_0x5b0,
                             (int)(char)_refptr_g_tsaPrmtFlash->field_0x5b1,
                             *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5ac,
                             _refptr_g_tsaPrmtFlash->field_0x20,*(undefined1 *)(param_1 + 0xb));
  sVar8 = GetSensorStartDspl((int)(char)_refptr_g_tsaPrmtFlash->field_0x5b0,
                             (int)(char)_refptr_g_tsaPrmtFlash->field_0x5b1,
                             *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5ac,
                             _refptr_g_tsaPrmtFlash->field_0x20,
                             *(char *)(param_1 + 9) + *(char *)(param_1 + 0xb));
  return CONCAT26((sVar8 - *(short *)(param_1 + 0x12)) - *(short *)(param_1 + 0xe),
                  CONCAT24(sVar4 - sVar7,CONCAT22((sVar6 - sVar2) - sVar3,sVar1 - sVar5)));
}


// ============================================================================
// Function: GetSensorStartDspl @ 6bb26af1
// ============================================================================

int GetSensorStartDspl(char param_1,char param_2,short param_3,byte param_4,byte param_5)

{
  longlong lVar1;
  
  lVar1 = (longlong)(int)((uint)param_5 * (int)(short)(param_3 + (short)param_2 + (short)param_1)) /
          (longlong)(int)(uint)param_4;
  return (int)lVar1 - (int)CONCAT62((int6)((ulonglong)lVar1 >> 0x10),(short)param_1);
}


// ============================================================================
// Function: SinXDivX @ 6ba8573e
// ============================================================================

double SinXDivX(double param_1)

{
  double dVar1;
  double local_res8;
  
  local_res8 = param_1;
  if (param_1 < 0.0) {
    local_res8 = -param_1;
  }
  dVar1 = local_res8 * local_res8;
  if (1.0 < local_res8) {
    if (2.0 < local_res8) {
      dVar1 = 0.0;
    }
    else {
      dVar1 = (((double)DAT_6bb52ecc * dVar1 * local_res8 -
               (double)(DAT_6bb52ecc * DAT_6bb52ee8) * dVar1) +
              (double)(DAT_6bb52ecc * DAT_6bb52eec) * local_res8) -
              (double)(DAT_6bb52ecc * DAT_6bb52ef0);
    }
  }
  else {
    dVar1 = ((double)(DAT_6bb52ecc + DAT_6bb52ee0) * dVar1 * local_res8 -
            (double)(DAT_6bb52ecc + DAT_6bb52ee4) * dVar1) + 1.0;
  }
  return dVar1;
}


// ============================================================================
// Function: JITFilter @ 6ba602c6
// ============================================================================

void JITFilter(int *param_1,int *param_2,int *param_3)

{
  uint uVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  uint local_20;
  uint local_1c;
  
                    /* 0x202c6  893  JITFilter */
  uVar3 = *param_1 - param_1[2];
  uVar4 = param_1[1] - param_1[3];
  uVar1 = uVar4 * uVar4 + uVar3 * uVar3;
  iVar2 = MIN(param_1[5],param_1[4]);
  param_1[5] = iVar2;
  iVar2 = Misc_SqrtUint32(uVar1);
  if (((uint)(param_1[4] * param_1[4]) < uVar1) && (iVar2 != 0)) {
    local_1c = (int)(param_1[4] * uVar3) / iVar2;
    if ((int)(((int)local_1c >> 0x1f ^ local_1c) - ((int)local_1c >> 0x1f)) < param_1[5]) {
      uVar1 = (int)uVar3 >> 0x1f;
      if ((int)((uVar1 ^ uVar3) - uVar1) < param_1[5]) {
        local_1c = uVar3;
      }
      else {
        local_1c = (int)(param_1[5] * uVar3) / (int)((uVar1 ^ uVar3) - uVar1);
      }
    }
    *param_2 = *param_1 - local_1c;
    local_20 = (int)(param_1[4] * uVar4) / iVar2;
    if ((int)(((int)local_20 >> 0x1f ^ local_20) - ((int)local_20 >> 0x1f)) < param_1[5]) {
      uVar1 = (int)uVar4 >> 0x1f;
      if ((int)((uVar1 ^ uVar4) - uVar1) < param_1[5]) {
        local_20 = uVar4;
      }
      else {
        local_20 = (int)(param_1[5] * uVar4) / (int)((uVar1 ^ uVar4) - uVar1);
      }
    }
    *param_3 = param_1[1] - local_20;
  }
  else {
    *param_2 = param_1[2];
    *param_3 = param_1[3];
  }
  return;
}


// ============================================================================
// Function: Misc_GetShiftNum @ 6ba64b9c
// ============================================================================

char Misc_GetShiftNum(ushort *param_1,ushort param_2)

{
  char local_9;
  
                    /* 0x24b9c  922  Misc_GetShiftNum */
  local_9 = '\0';
  if (*param_1 == 0) {
    local_9 = '\0';
  }
  else if (*param_1 < param_2) {
    while (*param_1 < param_2) {
      *param_1 = *param_1 * 2;
      local_9 = local_9 + '\x01';
    }
  }
  else {
    while (param_2 < *param_1) {
      *param_1 = *param_1 >> 1;
      local_9 = local_9 + -1;
    }
  }
  return local_9;
}


// ============================================================================
// Function: Misc_ShiftInINT64S @ 6ba64c33
// ============================================================================

void Misc_ShiftInINT64S(ulonglong *param_1,byte param_2)

{
  ulonglong local_18;
  char local_9;
  
                    /* 0x24c33  925  Misc_ShiftInINT64S */
  local_9 = '\0';
  if ((longlong)*param_1 < 0) {
    local_18 = -*param_1;
  }
  else {
    local_18 = *param_1;
  }
  for (; (ulonglong)(1L << (param_2 & 0x3f)) < local_18; local_18 = local_18 >> 1) {
    local_9 = local_9 + '\x01';
  }
  if ((longlong)*param_1 < 0) {
    *param_1 = -local_18;
  }
  else {
    *param_1 = local_18;
  }
  *(char *)(param_1 + 1) = local_9;
  return;
}


// ============================================================================
// Function: Misc_GetShiftBits @ 6ba64cdb
// ============================================================================

byte Misc_GetShiftBits(ulonglong param_1,byte param_2)

{
  ulonglong local_res8;
  byte local_9;
  
                    /* 0x24cdb  921  Misc_GetShiftBits */
  local_9 = 0;
  local_res8 = param_1;
  if ((longlong)param_1 < 0) {
    local_res8 = -param_1;
  }
  for (; (ulonglong)(1L << (param_2 & 0x3f)) <= local_res8 >> (local_9 & 0x3f);
      local_9 = local_9 + 1) {
  }
  return local_9;
}


// ============================================================================
// Function: Misc_RightShift @ 6ba64d3c
// ============================================================================

ulonglong Misc_RightShift(ulonglong param_1,byte param_2)

{
                    /* 0x24d3c  924  Misc_RightShift */
  if ((longlong)param_1 < 0) {
    param_1 = -(-param_1 >> (param_2 & 0x3f));
  }
  else {
    param_1 = param_1 >> (param_2 & 0x3f);
  }
  return param_1;
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


// ============================================================================
// Function: Misc_SqrtUint64 @ 6ba64ddd
// ============================================================================

longlong Misc_SqrtUint64(ulonglong param_1)

{
  longlong local_18;
  ulonglong local_10;
  
                    /* 0x24ddd  928  Misc_SqrtUint64 */
  local_18 = 0;
  for (local_10 = 0x80000000; param_1 < local_10; local_10 = local_10 >> 1) {
  }
  for (; local_10 != 0; local_10 = local_10 >> 1) {
    local_18 = local_18 + local_10;
    if (param_1 <= (ulonglong)(local_18 * local_18) && local_18 * local_18 - param_1 != 0) {
      local_18 = local_18 - local_10;
    }
  }
  return local_18;
}


// ============================================================================
// Function: Misc_Sin @ 6ba64e40
// ============================================================================

int Misc_Sin(int param_1)

{
  int local_res8;
  int local_c;
  
                    /* 0x24e40  926  Misc_Sin */
  local_c = 1;
  local_res8 = param_1;
  if (param_1 < 0) {
    local_res8 = -param_1;
    local_c = -1;
  }
  if (0xb4 < local_res8) {
    local_res8 = 0xb4;
  }
  if (0x5a < local_res8) {
    local_res8 = 0xb4 - local_res8;
  }
  return (uint)*(ushort *)(&g_miscSinTable + (longlong)local_res8 * 2) * local_c;
}


// ============================================================================
// Function: Misc_Cos @ 6ba64eaa
// ============================================================================

int Misc_Cos(int param_1)

{
  int local_res8;
  int local_c;
  
                    /* 0x24eaa  919  Misc_Cos */
  local_res8 = param_1;
  if (param_1 < 0) {
    local_res8 = -param_1;
  }
  if (0xb4 < local_res8) {
    local_res8 = 0xb4;
  }
  local_c = 1;
  if (0x5a < local_res8) {
    local_res8 = 0xb4 - local_res8;
    local_c = -1;
  }
  return (uint)*(ushort *)(&g_miscSinTable + (longlong)(0x5a - local_res8) * 2) * local_c;
}


// ============================================================================
// Function: Misc_IsInRect @ 6ba64f1f
// ============================================================================

undefined8 Misc_IsInRect(ushort *param_1,int param_2,int param_3)

{
  undefined8 uVar1;
  
                    /* 0x24f1f  923  Misc_IsInRect */
  if ((int)(uint)*param_1 < param_2) {
    if (param_2 < (int)(uint)param_1[1]) {
      if ((int)(uint)param_1[2] < param_3) {
        if (param_3 < (int)(uint)param_1[3]) {
          uVar1 = 1;
        }
        else {
          uVar1 = 0;
        }
      }
      else {
        uVar1 = 0;
      }
    }
    else {
      uVar1 = 0;
    }
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: Misc_FindKthSmallest @ 6ba6572e
// ============================================================================

/* WARNING: Function: ___chkstk_ms replaced with injection: alloca_probe */

undefined4 Misc_FindKthSmallest(longlong param_1,int param_2,int param_3, ...)

{
  bool bVar1;
  undefined4 uVar2;
  undefined4 *puVar3;
  int aiStack_48 [6];
  int local_30;
  int local_2c;
  int local_24;
  int local_20;
  int local_1c;
  
                    /* 0x2572e  920  Misc_FindKthSmallest */
  if ((((param_1 == 0) || (param_2 < 1)) || (param_3 < 1)) || (param_2 <= param_3)) {
    uVar2 = 0;
  }
  else {
    puVar3 = (undefined4 *)
             ((longlong)aiStack_48 - ((longlong)param_2 * 4 + 0x12U & 0xfffffffffffffff0));
    for (local_1c = 0; local_1c < param_3; local_1c = local_1c + 1) {
      puVar3[local_1c] = 0;
    }
    local_24 = 0;
    for (local_20 = 0; local_20 < param_2; local_20 = local_20 + 1) {
      if (local_24 != 0) {
        bVar1 = false;
        local_2c = 0;
LAB_6ba658f1:
        if (local_2c < local_24) {
          if ((int)puVar3[local_2c] <= *(int *)(param_1 + (longlong)local_20 * 4))
          goto code_r0x6ba6586f;
          if (local_24 == param_3) {
            local_30 = local_24 + -1;
          }
          else {
            local_30 = local_24;
          }
          for (; local_2c < local_30; local_30 = local_30 + -1) {
            puVar3[local_30] = puVar3[local_30 + -1];
          }
          puVar3[local_2c] = *(undefined4 *)(param_1 + (longlong)local_20 * 4);
          if (local_24 < param_3) {
            local_24 = local_24 + 1;
          }
          bVar1 = true;
        }
        if ((local_24 < param_3) && (!bVar1)) {
          puVar3[local_24] = *(undefined4 *)(param_1 + (longlong)local_20 * 4);
          local_24 = local_24 + 1;
        }
        goto LAB_6ba65932;
      }
      *puVar3 = *(undefined4 *)(param_1 + (longlong)local_20 * 4);
      local_24 = 1;
LAB_6ba65932:
    }
    uVar2 = puVar3[param_3 + -1];
  }
  return uVar2;
code_r0x6ba6586f:
  local_2c = local_2c + 1;
  goto LAB_6ba658f1;
}


