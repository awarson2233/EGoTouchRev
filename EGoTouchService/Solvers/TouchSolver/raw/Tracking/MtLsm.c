#include "TSACoreInternal.h"

// ============================================================================
// Function: MtLsm_CalcMatrixB @ 6ba54882
// ============================================================================

void MtLsm_CalcMatrixB(longlong param_1,longlong param_2,ushort param_3,longlong param_4)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x168]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x170];
  
                    /* 0x14882  930  MtLsm_CalcMatrixB */
  (*(uint *)(_stk + 0x154)) = (uint)*(byte *)(param_4 + 0x10);
  (*(byte *)(_stk + 0x153)) = *(byte *)(param_4 + 0x11);
  Matrix_ReShape(((float *)(_stk + 0xe8)),1,(*(byte *)(_stk + 0x153)));
  Matrix_Init(((float *)(_stk + 0x78)),(*(byte *)(_stk + 0x153)),(*(byte *)(_stk + 0x153)),0);
  for ((*(uint *)(_stk + 0x164)) = 0; (*(uint *)(_stk + 0x164)) < (*(uint *)(_stk + 0x154)); (*(uint *)(_stk + 0x164)) = (*(uint *)(_stk + 0x164)) + 1) {
    MtLst_GetVectorA(((float *)(_stk + 0xe8)),(*(uint *)(_stk + 0x164)),param_4);
    for ((*(byte *)(_stk + 0x163)) = 0; (*(byte *)(_stk + 0x163)) < (*(byte *)(_stk + 0x14d)); (*(byte *)(_stk + 0x163)) = (*(byte *)(_stk + 0x163)) + 1) {
      for ((*(byte *)(_stk + 0x162)) = (*(byte *)(_stk + 0x163)); (*(byte *)(_stk + 0x162)) < (*(byte *)(_stk + 0x14d)); (*(byte *)(_stk + 0x162)) = (*(byte *)(_stk + 0x162)) + 1) {
        ((float *)(_stk + 0x78))[(int)((uint)(*(byte *)(_stk + 0x163)) * (uint)(*(byte *)(_stk + 0xdd)) + (uint)(*(byte *)(_stk + 0x162)))] =
             ((float *)(_stk + 0x78))[(int)((uint)(*(byte *)(_stk + 0x162)) + (uint)(*(byte *)(_stk + 0x163)) * (uint)(*(byte *)(_stk + 0xdd)))] +
             ((float *)(_stk + 0xe8))[(int)(uint)(*(byte *)(_stk + 0x163))] * ((float *)(_stk + 0xe8))[(int)(uint)(*(byte *)(_stk + 0x162))];
      }
    }
  }
  Matrix_SymmetricFromUpper(((float *)(_stk + 0x78)));
  for ((*(byte *)(_stk + 0x161)) = 0; (*(byte *)(_stk + 0x161)) < (*(byte *)(_stk + 0x14d)); (*(byte *)(_stk + 0x161)) = (*(byte *)(_stk + 0x161)) + 1) {
    for ((*(byte *)(_stk + 0x160)) = 0; (*(byte *)(_stk + 0x160)) < (*(byte *)(_stk + 0x14d)); (*(byte *)(_stk + 0x160)) = (*(byte *)(_stk + 0x160)) + 1) {
      *(float *)((longlong)(int)((uint)(*(byte *)(_stk + 0x160)) + (uint)(*(byte *)(_stk + 0x161)) * (uint)(*(byte *)(_stk + 0x14d))) * 4 + param_2) =
           ((float *)(_stk + 0x78))[(int)((uint)(*(byte *)(_stk + 0x160)) + (uint)(*(byte *)(_stk + 0x161)) * (uint)(*(byte *)(_stk + 0xdd)))];
    }
  }
  Matrix_Inv(((float *)(_stk + 0x78)),((float *)(_stk + 0x78)));
  Matrix_ReShape(((float *)(_stk + 0xe8)),*(undefined1 *)(param_4 + 0x11),1);
  for ((*(uint *)(_stk + 0x15c)) = 0; (*(uint *)(_stk + 0x15c)) < (*(uint *)(_stk + 0x154)); (*(uint *)(_stk + 0x15c)) = (*(uint *)(_stk + 0x15c)) + 1) {
    MtLst_GetVectorA(((float *)(_stk + 0xe8)),(*(uint *)(_stk + 0x15c)),param_4);
    Matrix_Mul(((float *)(_stk + 0x8)),((float *)(_stk + 0x78)),((float *)(_stk + 0xe8)));
    for ((*(byte *)(_stk + 0x15b)) = 0; (*(byte *)(_stk + 0x15b)) < (*(byte *)(_stk + 0x153)); (*(byte *)(_stk + 0x15b)) = (*(byte *)(_stk + 0x15b)) + 1) {
      (*(ushort *)(_stk + 0x150)) = (ushort)(*(byte *)(_stk + 0x15b)) * (short)(*(uint *)(_stk + 0x154)) + (short)(*(uint *)(_stk + 0x15c));
      if ((*(ushort *)(_stk + 0x150)) < param_3) {
        *(short *)((ulonglong)(*(ushort *)(_stk + 0x150)) * 2 + param_1) =
             (short)(int)(((float *)(_stk + 0x8))[(int)(uint)(*(byte *)(_stk + 0x15b))] * DAT_6bb51d6c);
      }
    }
  }
  return;
}


// ============================================================================
// Function: MtLsm_LinearRegressionWeight @ 6ba550d4
// ============================================================================

void MtLsm_LinearRegressionWeight(longlong param_1,undefined8 param_2,undefined1 param_3)

{
  MtLsm_LinearRegressionCalcState(param_1);
  MtLsm_LinearReconstruct
            (param_2,param_3,param_1 + 0x40,*(undefined1 *)(param_1 + 0x11),
             *(undefined8 *)(param_1 + 0x20));
  return;
}


// ============================================================================
// Function: MtLsm_LinearReconstruct @ 6ba5453b
// ============================================================================

void MtLsm_LinearReconstruct
               (undefined2 *param_1,byte param_2,float *param_3,int param_4,float *param_5)

{
  undefined2 *local_res8;
  int local_38;
  int local_34;
  float local_30;
  uint local_2c;
  int local_28;
  uint local_24;
  int local_20;
  uint local_1c;
  int local_18;
  uint local_14;
  float *local_10;
  
                    /* 0x1453b  931  MtLsm_LinearReconstruct */
  if (param_4 == 2) {
    local_res8 = param_1;
    local_14 = (uint)param_2;
    while (local_14 != 0) {
      local_18 = (int)(param_5[1] * param_3[1] + *param_5 * *param_3);
      param_5 = param_5 + 2;
      if (local_18 < 0x8000) {
        if (local_18 < -0x8000) {
          local_18 = -0x8000;
        }
      }
      else {
        local_18 = 0x7fff;
      }
      *local_res8 = (short)local_18;
      local_res8 = local_res8 + 1;
      local_14 = local_14 - 1;
    }
  }
  else if (param_4 == 3) {
    local_res8 = param_1;
    local_1c = (uint)param_2;
    while (local_1c != 0) {
      local_20 = (int)(param_5[2] * param_3[2] + *param_5 * *param_3 + param_5[1] * param_3[1]);
      param_5 = param_5 + 3;
      if (local_20 < 0x8000) {
        if (local_20 < -0x8000) {
          local_20 = -0x8000;
        }
      }
      else {
        local_20 = 0x7fff;
      }
      *local_res8 = (short)local_20;
      local_res8 = local_res8 + 1;
      local_1c = local_1c - 1;
    }
  }
  else if (param_4 == 4) {
    local_res8 = param_1;
    local_24 = (uint)param_2;
    while (local_24 != 0) {
      local_28 = (int)(param_5[3] * param_3[3] +
                      *param_5 * *param_3 + param_5[1] * param_3[1] + param_5[2] * param_3[2]);
      param_5 = param_5 + 4;
      if (local_28 < 0x8000) {
        if (local_28 < -0x8000) {
          local_28 = -0x8000;
        }
      }
      else {
        local_28 = 0x7fff;
      }
      *local_res8 = (short)local_28;
      local_res8 = local_res8 + 1;
      local_24 = local_24 - 1;
    }
  }
  else {
    local_res8 = param_1;
    local_2c = (uint)param_2;
    while (local_2c != 0) {
      local_30 = DAT_6bb51d68;
      local_34 = param_4;
      local_10 = param_3;
      while (local_34 != 0) {
        local_30 = local_30 + *param_5 * *local_10;
        param_5 = param_5 + 1;
        local_34 = local_34 + -1;
        local_10 = local_10 + 1;
      }
      local_38 = (int)local_30;
      if (local_38 < 0x8000) {
        if (local_38 < -0x8000) {
          local_38 = -0x8000;
        }
      }
      else {
        local_38 = 0x7fff;
      }
      *local_res8 = (short)local_38;
      local_res8 = local_res8 + 1;
      local_2c = local_2c - 1;
    }
  }
  return;
}


// ============================================================================
// Function: MtLsm_LinearRegressionCalcState @ 6ba55037
// ============================================================================

void MtLsm_LinearRegressionCalcState(longlong param_1)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x78]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x80];
  uint uVar1;
  
                    /* 0x15037  932  MtLsm_LinearRegressionCalcState */
  (*(byte *)(_stk + 0x73)) = *(byte *)(param_1 + 0x11);
  Matrix_Init(((undefined4 *)(_stk + 0x8)),(*(byte *)(_stk + 0x73)),1,0);
  if (*(char *)(param_1 + 0x30) == '\0') {
    MtLsm_GetStateByWeightSensor(((undefined4 *)(_stk + 0x8)),param_1);
  }
  else {
    MtLsm_GetStateByAllSensor(((undefined4 *)(_stk + 0x8)),param_1);
  }
  (*(uint *)(_stk + 0x74)) = (uint)(*(byte *)(_stk + 0x73));
  while (uVar1 = (*(uint *)(_stk + 0x74)), (*(uint *)(_stk + 0x74)) = (*(uint *)(_stk + 0x74)) - 1, uVar1 != 0) {
    *(undefined4 *)(param_1 + ((longlong)(int)(*(uint *)(_stk + 0x74)) + 0x10) * 4) = ((undefined4 *)(_stk + 0x8))[(int)(*(uint *)(_stk + 0x74))];
  }
  return;
}


// ============================================================================
// Function: MtLsm_GetStateByAllSensor @ 6ba54c29
// ============================================================================

void MtLsm_GetStateByAllSensor(longlong param_1,undefined8 *param_2)

{
  byte bVar1;
  byte bVar2;
  uint local_28;
  int local_24;
  short *local_20;
  int local_14;
  short *local_10;
  
  bVar1 = *(byte *)((longlong)param_2 + 0x11);
  bVar2 = *(byte *)(param_2 + 2);
  local_10 = (short *)param_2[7];
  for (local_14 = 0; local_14 < (int)(uint)bVar1; local_14 = local_14 + 1) {
    local_24 = 0;
    local_28 = (uint)bVar2;
    local_20 = (short *)*param_2;
    while (local_28 != 0) {
      local_24 = local_24 + (int)*local_20 * (int)*local_10;
      local_28 = local_28 - 1;
      local_20 = local_20 + 1;
      local_10 = local_10 + 1;
    }
    if (local_24 < 0) {
      local_24 = local_24 + 0x3ff;
    }
    *(float *)(param_1 + (longlong)local_14 * 4) = (float)(local_24 >> 10);
  }
  return;
}


// ============================================================================
// Function: MtLsm_GetStateByWeightSensor @ 6ba54cfe
// ============================================================================

void MtLsm_GetStateByWeightSensor(undefined8 param_1,longlong *param_2)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x128]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x130];
  uint uVar1;
  
  (*(longlong *)(_stk + 0x100)) = param_2[1];
  (*(byte *)(_stk + 0xff)) = *(byte *)((longlong)param_2 + 0x11);
  (*(uint *)(_stk + 0xf8)) = (uint)*(byte *)(param_2 + 2);
  (*(longlong *)(_stk + 0xf0)) = *param_2;
  Matrix_Init(((float *)(_stk + 0x78)),(*(byte *)(_stk + 0xff)),(*(byte *)(_stk + 0xff)),param_2[5]);
  Matrix_Init(((float *)(_stk + 0x8)),(*(byte *)(_stk + 0xff)),1,0);
  (*(longlong *)(_stk + 0x120)) = param_2[4];
  (*(uint *)(_stk + 0x11c)) = (*(uint *)(_stk + 0xf8));
  for ((*(uint *)(_stk + 0x118)) = 0; (*(uint *)(_stk + 0x118)) < (*(uint *)(_stk + 0xf8)); (*(uint *)(_stk + 0x118)) = (*(uint *)(_stk + 0x118)) + 1) {
    if (*(short *)((*(longlong *)(_stk + 0x100)) + (ulonglong)(*(uint *)(_stk + 0x118)) * 2) == 0) {
      (*(uint *)(_stk + 0x11c)) = (*(uint *)(_stk + 0x11c)) - 1;
      (*(uint *)(_stk + 0x110)) = (uint)(*(byte *)(_stk + 0xff));
      while( true ) {
        uVar1 = (*(uint *)(_stk + 0x110));
        (*(uint *)(_stk + 0x110)) = (*(uint *)(_stk + 0x110)) - 1;
        if (uVar1 == 0) break;
        for ((*(uint *)(_stk + 0x10c)) = (*(uint *)(_stk + 0x110)); (*(uint *)(_stk + 0x10c)) < (*(byte *)(_stk + 0xff)); (*(uint *)(_stk + 0x10c)) = (*(uint *)(_stk + 0x10c)) + 1) {
          ((float *)(_stk + 0x78))[(*(byte *)(_stk + 0xdd)) * (*(uint *)(_stk + 0x110)) + (*(uint *)(_stk + 0x10c))] =
               ((float *)(_stk + 0x78))[(*(uint *)(_stk + 0x10c)) + (*(byte *)(_stk + 0xdd)) * (*(uint *)(_stk + 0x110))] -
               *(float *)((*(longlong *)(_stk + 0x120)) + (ulonglong)(*(uint *)(_stk + 0x110)) * 4) *
               *(float *)((*(longlong *)(_stk + 0x120)) + (ulonglong)(*(uint *)(_stk + 0x10c)) * 4);
        }
      }
    }
    else {
      (*(int *)(_stk + 0xec)) = (int)*(short *)((*(longlong *)(_stk + 0xf0)) + (ulonglong)(*(uint *)(_stk + 0x118)) * 2);
      (*(uint *)(_stk + 0x114)) = (uint)(*(byte *)(_stk + 0xff));
      while( true ) {
        uVar1 = (*(uint *)(_stk + 0x114));
        (*(uint *)(_stk + 0x114)) = (*(uint *)(_stk + 0x114)) - 1;
        if (uVar1 == 0) break;
        ((float *)(_stk + 0x8))[(*(uint *)(_stk + 0x114))] =
             ((float *)(_stk + 0x8))[(*(uint *)(_stk + 0x114))] + *(float *)((*(longlong *)(_stk + 0x120)) + (ulonglong)(*(uint *)(_stk + 0x114)) * 4) * (float)(*(int *)(_stk + 0xec));
      }
    }
    (*(longlong *)(_stk + 0x120)) = (*(longlong *)(_stk + 0x120)) + (ulonglong)(*(byte *)(_stk + 0xff)) * 4;
  }
  Matrix_SymmetricFromUpper(((float *)(_stk + 0x78)));
  if ((int)(*(uint *)(_stk + 0x11c)) < (int)(uint)(*(byte *)(_stk + 0xff))) {
    Matrix_Init(param_1,(*(byte *)(_stk + 0xff)),1,0);
  }
  else {
    MtLsm_Solve(param_1,((float *)(_stk + 0x78)),((float *)(_stk + 0x8)));
    if (*(char *)((longlong)param_2 + 0x12) != '\0') {
      MtLsm_SolveWithConstrain(param_1,((float *)(_stk + 0x78)),((float *)(_stk + 0x8)),param_2);
    }
  }
  return;
}


// ============================================================================
// Function: MtLsm_Solve @ 6ba54158
// ============================================================================

void MtLsm_Solve(undefined8 param_1,undefined8 param_2,undefined8 param_3)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x78]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x80];
  
  Matrix_Inv(((undefined1 *)(_stk + 0x8)),param_2);
  Matrix_Mul(param_1,((undefined1 *)(_stk + 0x8)),param_3);
  return;
}


// ============================================================================
// Function: MtLsm_SolveWithConstrain @ 6ba543cb
// ============================================================================

void MtLsm_SolveWithConstrain
               (undefined8 param_1,undefined8 param_2,undefined8 param_3,longlong param_4)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x88]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x90];
  
  (*(byte *)(_stk + 0x7f)) = *(byte *)(param_4 + 0x11);
  Matrix_Init(((undefined4 *)(_stk + 0x8)),(*(byte *)(_stk + 0x7f)),(*(byte *)(_stk + 0x7f)),0);
  for ((*(uint *)(_stk + 0x84)) = 0; (*(uint *)(_stk + 0x84)) < (*(byte *)(_stk + 0x7f)); (*(uint *)(_stk + 0x84)) = (*(uint *)(_stk + 0x84)) + 1) {
    ((undefined4 *)(_stk + 0x8))[((*(byte *)(_stk + 0x6d)) + 1) * (*(uint *)(_stk + 0x84))] =
         *(undefined4 *)(*(longlong *)(param_4 + 0x18) + (ulonglong)(*(uint *)(_stk + 0x84)) * 4);
  }
  (*(float *)(_stk + 0x80)) = DAT_6bb51d68;
  (*(uint *)(_stk + 0x84)) = 0;
  while ((((*(uint *)(_stk + 0x84)) < 6 && ((*(float *)(_stk + 0x78)) = (float)MtLsm_GetF(param_1,((undefined4 *)(_stk + 0x8))), 0.01 <= (*(float *)(_stk + 0x78)))) &&
         ((*(float *)(_stk + 0x74)) = (float)MtLsm_GetFDeriv(param_1,param_2,param_3,((undefined4 *)(_stk + 0x8)),(*(float *)(_stk + 0x80))),
         DAT_6bb51d68 != (*(float *)(_stk + 0x74))))) {
    (*(float *)(_stk + 0x80)) = (*(float *)(_stk + 0x80)) - (*(float *)(_stk + 0x78)) / (*(float *)(_stk + 0x74));
    MtLsm_GetP(param_1,param_2,param_3,((undefined4 *)(_stk + 0x8)),(*(float *)(_stk + 0x80)));
    (*(uint *)(_stk + 0x84)) = (*(uint *)(_stk + 0x84)) + 1;
  }
  return;
}


// ============================================================================
// Function: MtLsm_GetP @ 6ba54204
// ============================================================================

void MtLsm_GetP(undefined8 param_1,undefined8 param_2,undefined8 param_3,undefined8 param_4,
               undefined4 param_5)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x78]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x80];
  
  MtLsm_GetInvATA(((undefined1 *)(_stk + 0x8)),param_2,param_4,param_5);
  Matrix_Mul(param_1,((undefined1 *)(_stk + 0x8)),param_3);
  return;
}


// ============================================================================
// Function: MtLsm_GetF @ 6ba54260
// ============================================================================

float MtLsm_GetF(undefined8 param_1,undefined8 param_2)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x78]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x80];
  
  Matrix_Rotate(((float *)(_stk + 0x8)),param_1);
  Matrix_Mul(((float *)(_stk + 0x8)),((float *)(_stk + 0x8)),param_2);
  Matrix_Mul(((float *)(_stk + 0x8)),((float *)(_stk + 0x8)),param_1);
  return ((float *)(_stk + 0x8))[0] - 1.0f;
}


// ============================================================================
// Function: MtLsm_GetFDeriv @ 6ba542cc
// ============================================================================

float MtLsm_GetFDeriv(undefined8 param_1,undefined8 param_2,undefined8 param_3,undefined8 param_4,
                     undefined4 param_5)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0xe8]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0xf0];
  
  MtLsm_GetInvATA(((undefined1 *)(_stk + 0x78)),param_2,param_4,param_5);
  Matrix_Mul(((float *)(_stk + 0x8)),((undefined1 *)(_stk + 0x78)),param_4);
  Matrix_Mul(((float *)(_stk + 0x8)),((float *)(_stk + 0x8)),((undefined1 *)(_stk + 0x78)));
  Matrix_Mul(((float *)(_stk + 0x8)),((float *)(_stk + 0x8)),param_3);
  Matrix_Rotate(((float *)(_stk + 0x8)),((float *)(_stk + 0x8)));
  Matrix_Mul(((float *)(_stk + 0x8)),((float *)(_stk + 0x8)),param_4);
  Matrix_Mul(((float *)(_stk + 0x8)),((float *)(_stk + 0x8)),param_1);
  return ((float *)(_stk + 0x8))[0] * DAT_6bb51d64;
}


// ============================================================================
// Function: MtLsm_GetInvATA @ 6ba541a0
// ============================================================================

void MtLsm_GetInvATA(undefined8 param_1,undefined8 param_2,undefined8 param_3,undefined4 param_4)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x78]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x80];
  
  Matrix_Mul_C(((undefined1 *)(_stk + 0x8)),param_3,param_4);
  Matrix_Add(((undefined1 *)(_stk + 0x8)),param_2,((undefined1 *)(_stk + 0x8)));
  Matrix_Inv(param_1,((undefined1 *)(_stk + 0x8)));
  return;
}


