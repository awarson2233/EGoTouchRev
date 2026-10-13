// ============================================================================
// Function: MtLsm_CalcMatrixB @ 6ba54882
// ============================================================================

void MtLsm_CalcMatrixB(longlong param_1,longlong param_2,ushort param_3,longlong param_4)

{
  float local_168 [28];
  float local_f8 [25];
  byte local_93;
  float local_88 [25];
  byte local_23;
  ushort local_20;
  byte local_1d;
  uint local_1c;
  byte local_15;
  uint local_14;
  byte local_10;
  byte local_f;
  byte local_e;
  byte local_d;
  uint local_c;
  
                    /* 0x14882  930  MtLsm_CalcMatrixB */
  local_1c = (uint)*(byte *)(param_4 + 0x10);
  local_1d = *(byte *)(param_4 + 0x11);
  Matrix_ReShape(local_88,1,local_1d);
  Matrix_Init(local_f8,local_1d,local_1d,0);
  for (local_c = 0; local_c < local_1c; local_c = local_c + 1) {
    MtLst_GetVectorA(local_88,local_c,param_4);
    for (local_d = 0; local_d < local_23; local_d = local_d + 1) {
      for (local_e = local_d; local_e < local_23; local_e = local_e + 1) {
        local_f8[(int)((uint)local_d * (uint)local_93 + (uint)local_e)] =
             local_f8[(int)((uint)local_e + (uint)local_d * (uint)local_93)] +
             local_88[(int)(uint)local_d] * local_88[(int)(uint)local_e];
      }
    }
  }
  Matrix_SymmetricFromUpper(local_f8);
  for (local_f = 0; local_f < local_23; local_f = local_f + 1) {
    for (local_10 = 0; local_10 < local_23; local_10 = local_10 + 1) {
      *(float *)((longlong)(int)((uint)local_10 + (uint)local_f * (uint)local_23) * 4 + param_2) =
           local_f8[(int)((uint)local_10 + (uint)local_f * (uint)local_93)];
    }
  }
  Matrix_Inv(local_f8,local_f8);
  Matrix_ReShape(local_88,*(undefined1 *)(param_4 + 0x11),1);
  for (local_14 = 0; local_14 < local_1c; local_14 = local_14 + 1) {
    MtLst_GetVectorA(local_88,local_14,param_4);
    Matrix_Mul(local_168,local_f8,local_88);
    for (local_15 = 0; local_15 < local_1d; local_15 = local_15 + 1) {
      local_20 = (ushort)local_15 * (short)local_1c + (short)local_14;
      if (local_20 < param_3) {
        *(short *)((ulonglong)local_20 * 2 + param_1) =
             (short)(int)(local_168[(int)(uint)local_15] * DAT_6bb51d6c);
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
  uint uVar1;
  undefined4 local_78 [26];
  byte local_d;
  uint local_c;
  
                    /* 0x15037  932  MtLsm_LinearRegressionCalcState */
  local_d = *(byte *)(param_1 + 0x11);
  Matrix_Init(local_78,local_d,1,0);
  if (*(char *)(param_1 + 0x30) == '\0') {
    MtLsm_GetStateByWeightSensor(local_78,param_1);
  }
  else {
    MtLsm_GetStateByAllSensor(local_78,param_1);
  }
  local_c = (uint)local_d;
  while (uVar1 = local_c, local_c = local_c - 1, uVar1 != 0) {
    *(undefined4 *)(param_1 + ((longlong)(int)local_c + 0x10) * 4) = local_78[(int)local_c];
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
  uint uVar1;
  float local_128 [28];
  float local_b8 [25];
  byte local_53;
  int local_44;
  longlong local_40;
  uint local_38;
  byte local_31;
  longlong local_30;
  uint local_24;
  uint local_20;
  uint local_1c;
  uint local_18;
  uint local_14;
  longlong local_10;
  
  local_30 = param_2[1];
  local_31 = *(byte *)((longlong)param_2 + 0x11);
  local_38 = (uint)*(byte *)(param_2 + 2);
  local_40 = *param_2;
  Matrix_Init(local_b8,local_31,local_31,param_2[5]);
  Matrix_Init(local_128,local_31,1,0);
  local_10 = param_2[4];
  local_14 = local_38;
  for (local_18 = 0; local_18 < local_38; local_18 = local_18 + 1) {
    if (*(short *)(local_30 + (ulonglong)local_18 * 2) == 0) {
      local_14 = local_14 - 1;
      local_20 = (uint)local_31;
      while( true ) {
        uVar1 = local_20;
        local_20 = local_20 - 1;
        if (uVar1 == 0) break;
        for (local_24 = local_20; local_24 < local_31; local_24 = local_24 + 1) {
          local_b8[local_53 * local_20 + local_24] =
               local_b8[local_24 + local_53 * local_20] -
               *(float *)(local_10 + (ulonglong)local_20 * 4) *
               *(float *)(local_10 + (ulonglong)local_24 * 4);
        }
      }
    }
    else {
      local_44 = (int)*(short *)(local_40 + (ulonglong)local_18 * 2);
      local_1c = (uint)local_31;
      while( true ) {
        uVar1 = local_1c;
        local_1c = local_1c - 1;
        if (uVar1 == 0) break;
        local_128[local_1c] =
             local_128[local_1c] + *(float *)(local_10 + (ulonglong)local_1c * 4) * (float)local_44;
      }
    }
    local_10 = local_10 + (ulonglong)local_31 * 4;
  }
  Matrix_SymmetricFromUpper(local_b8);
  if ((int)local_14 < (int)(uint)local_31) {
    Matrix_Init(param_1,local_31,1,0);
  }
  else {
    MtLsm_Solve(param_1,local_b8,local_128);
    if (*(char *)((longlong)param_2 + 0x12) != '\0') {
      MtLsm_SolveWithConstrain(param_1,local_b8,local_128,param_2);
    }
  }
  return;
}


// ============================================================================
// Function: MtLsm_Solve @ 6ba54158
// ============================================================================

void MtLsm_Solve(undefined8 param_1,undefined8 param_2,undefined8 param_3)

{
  undefined1 local_78 [112];
  
  Matrix_Inv(local_78,param_2);
  Matrix_Mul(param_1,local_78,param_3);
  return;
}


// ============================================================================
// Function: MtLsm_SolveWithConstrain @ 6ba543cb
// ============================================================================

void MtLsm_SolveWithConstrain
               (undefined8 param_1,undefined8 param_2,undefined8 param_3,longlong param_4)

{
  undefined4 local_88 [25];
  byte local_23;
  float local_1c;
  float local_18;
  byte local_11;
  float local_10;
  uint local_c;
  
  local_11 = *(byte *)(param_4 + 0x11);
  Matrix_Init(local_88,local_11,local_11,0);
  for (local_c = 0; local_c < local_11; local_c = local_c + 1) {
    local_88[(local_23 + 1) * local_c] =
         *(undefined4 *)(*(longlong *)(param_4 + 0x18) + (ulonglong)local_c * 4);
  }
  local_10 = DAT_6bb51d68;
  local_c = 0;
  while (((local_c < 6 && (local_18 = (float)MtLsm_GetF(param_1,local_88), 0.01 <= local_18)) &&
         (local_1c = (float)MtLsm_GetFDeriv(param_1,param_2,param_3,local_88,local_10),
         DAT_6bb51d68 != local_1c))) {
    local_10 = local_10 - local_18 / local_1c;
    MtLsm_GetP(param_1,param_2,param_3,local_88,local_10);
    local_c = local_c + 1;
  }
  return;
}


// ============================================================================
// Function: MtLsm_GetP @ 6ba54204
// ============================================================================

void MtLsm_GetP(undefined8 param_1,undefined8 param_2,undefined8 param_3,undefined8 param_4,
               undefined4 param_5)

{
  undefined1 local_78 [112];
  
  MtLsm_GetInvATA(local_78,param_2,param_4,param_5);
  Matrix_Mul(param_1,local_78,param_3);
  return;
}


// ============================================================================
// Function: MtLsm_GetF @ 6ba54260
// ============================================================================

float MtLsm_GetF(undefined8 param_1,undefined8 param_2)

{
  float local_78 [28];
  
  Matrix_Rotate(local_78,param_1);
  Matrix_Mul(local_78,local_78,param_2);
  Matrix_Mul(local_78,local_78,param_1);
  return local_78[0] - _rdata;
}


// ============================================================================
// Function: MtLsm_GetFDeriv @ 6ba542cc
// ============================================================================

float MtLsm_GetFDeriv(undefined8 param_1,undefined8 param_2,undefined8 param_3,undefined8 param_4,
                     undefined4 param_5)

{
  float local_e8 [28];
  undefined1 local_78 [112];
  
  MtLsm_GetInvATA(local_78,param_2,param_4,param_5);
  Matrix_Mul(local_e8,local_78,param_4);
  Matrix_Mul(local_e8,local_e8,local_78);
  Matrix_Mul(local_e8,local_e8,param_3);
  Matrix_Rotate(local_e8,local_e8);
  Matrix_Mul(local_e8,local_e8,param_4);
  Matrix_Mul(local_e8,local_e8,param_1);
  return local_e8[0] * DAT_6bb51d64;
}


// ============================================================================
// Function: MtLsm_GetInvATA @ 6ba541a0
// ============================================================================

void MtLsm_GetInvATA(undefined8 param_1,undefined8 param_2,undefined8 param_3,undefined4 param_4)

{
  undefined1 local_78 [112];
  
  Matrix_Mul_C(local_78,param_3,param_4);
  Matrix_Add(local_78,param_2,local_78);
  Matrix_Inv(param_1,local_78);
  return;
}


