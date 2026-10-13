// ============================================================================
// Function: Matrix_ReShape @ 6ba63b6f
// ============================================================================

void Matrix_ReShape(longlong param_1,undefined1 param_2,undefined1 param_3)

{
                    /* 0x23b6f  915  Matrix_ReShape */
  *(undefined1 *)(param_1 + 100) = param_2;
  *(undefined1 *)(param_1 + 0x65) = param_3;
  return;
}


// ============================================================================
// Function: Matrix_Init @ 6ba63b9a
// ============================================================================

void Matrix_Init(longlong param_1,byte param_2,byte param_3,longlong param_4)

{
  uint local_c;
  
                    /* 0x23b9a  908  Matrix_Init */
  Matrix_ReShape(param_1,param_2,param_3);
  if (param_4 == 0) {
    for (local_c = 0; local_c < (uint)param_3 * (uint)param_2; local_c = local_c + 1) {
      *(undefined4 *)(param_1 + (ulonglong)local_c * 4) = _rdata;
    }
  }
  else {
    for (local_c = 0; local_c < (uint)param_3 * (uint)param_2; local_c = local_c + 1) {
      *(undefined4 *)(param_1 + (ulonglong)local_c * 4) =
           *(undefined4 *)(param_4 + (ulonglong)local_c * 4);
    }
  }
  return;
}


// ============================================================================
// Function: Matrix_Mul @ 6ba63da0
// ============================================================================

void Matrix_Mul(undefined8 param_1,longlong param_2,longlong param_3)

{
  undefined1 local_78 [112];
  
                    /* 0x23da0  911  Matrix_Mul */
  Matrix_ReShape(local_78,*(undefined1 *)(param_2 + 100),*(undefined1 *)(param_3 + 0x65));
  Matrix_M(local_78,param_2,param_3);
  Matrix_Copy(param_1,local_78);
  return;
}


// ============================================================================
// Function: Matrix_Rotate @ 6ba63e5c
// ============================================================================

void Matrix_Rotate(longlong param_1,longlong param_2)

{
  undefined1 local_78 [100];
  undefined1 local_14;
  undefined1 local_13;
  
                    /* 0x23e5c  916  Matrix_Rotate */
  if (param_1 == param_2) {
    Matrix_ReShape(local_78,*(undefined1 *)(param_2 + 0x65),*(undefined1 *)(param_2 + 100));
    Matrix_T(local_78,param_2);
    Matrix_Init(param_1,local_14,local_13,local_78);
  }
  else {
    Matrix_ReShape(param_1,*(undefined1 *)(param_2 + 0x65),*(undefined1 *)(param_2 + 100));
    Matrix_T(param_1,param_2);
  }
  return;
}


// ============================================================================
// Function: Matrix_Eig @ 6ba64a0b
// ============================================================================

undefined8 Matrix_Eig(longlong param_1,undefined8 param_2,undefined8 param_3)

{
  undefined4 local_1c8 [28];
  undefined4 local_158 [25];
  byte local_f3;
  undefined1 local_e8 [112];
  undefined1 local_78 [104];
  uint local_10;
  uint local_c;
  
                    /* 0x24a0b  905  Matrix_Eig */
  local_10 = 0x32;
  Matrix_ReShape(local_1c8,*(undefined1 *)(param_1 + 100),1);
  Matrix_Copy(local_158,param_1);
  for (local_c = 0; local_c < local_10; local_c = local_c + 1) {
    Matrix_QR(local_158,local_78,local_e8);
    Matrix_Mul(local_158,local_e8,local_78);
  }
  for (local_c = 0; local_c < local_f3; local_c = local_c + 1) {
    local_1c8[local_c] = local_158[(local_f3 + 1) * local_c];
  }
  Matrix_SortVector(local_1c8);
  Matrix_Eigenvectors(local_78,param_1,local_1c8);
  Matrix_Copy(param_3,local_1c8);
  Matrix_Copy(param_2,local_78);
  return 0;
}


// ============================================================================
// Function: Matrix_Inv @ 6ba63e0d
// ============================================================================

undefined8 Matrix_Inv(undefined8 param_1,longlong param_2)

{
                    /* 0x23e0d  910  Matrix_Inv */
  Matrix_Init(param_1,*(undefined1 *)(param_2 + 100),*(undefined1 *)(param_2 + 0x65),param_2);
  Matrix_I(param_1);
  return 0;
}


// ============================================================================
// Function: Matrix_SymmetricFromUpper @ 6ba63f8e
// ============================================================================

void Matrix_SymmetricFromUpper(longlong param_1)

{
  byte bVar1;
  byte local_a;
  byte local_9;
  
                    /* 0x23f8e  918  Matrix_SymmetricFromUpper */
  local_9 = *(byte *)(param_1 + 100);
  while (bVar1 = local_9 - 1, local_9 != 0) {
    for (local_a = 0; local_9 = bVar1, local_a < bVar1; local_a = local_a + 1) {
      *(undefined4 *)
       (param_1 + (longlong)(int)((uint)bVar1 * (uint)*(byte *)(param_1 + 0x65) + (uint)local_a) * 4
       ) = *(undefined4 *)
            (param_1 +
            (longlong)(int)((uint)local_a * (uint)*(byte *)(param_1 + 0x65) + (uint)bVar1) * 4);
    }
  }
  return;
}


// ============================================================================
// Function: Matrix_M @ 6ba62a9f
// ============================================================================

void Matrix_M(longlong param_1,longlong param_2,longlong param_3)

{
  byte bVar1;
  byte bVar2;
  byte bVar3;
  float local_18;
  int local_14;
  int local_10;
  int local_c;
  
  bVar1 = *(byte *)(param_2 + 100);
  bVar2 = *(byte *)(param_3 + 0x65);
  bVar3 = *(byte *)(param_2 + 0x65);
  for (local_c = 0; local_c < (int)(uint)bVar1; local_c = local_c + 1) {
    for (local_10 = 0; local_10 < (int)(uint)bVar2; local_10 = local_10 + 1) {
      local_18 = _rdata;
      for (local_14 = 0; local_14 < (int)(uint)bVar3; local_14 = local_14 + 1) {
        local_18 = local_18 +
                   *(float *)(param_2 +
                             (longlong)(int)((uint)*(byte *)(param_2 + 0x65) * local_c + local_14) *
                             4) *
                   *(float *)(param_3 +
                             (longlong)(int)((uint)*(byte *)(param_3 + 0x65) * local_14 + local_10)
                             * 4);
      }
      *(float *)(param_1 + (longlong)(int)(local_10 + (uint)*(byte *)(param_1 + 0x65) * local_c) * 4
                ) = local_18;
    }
  }
  return;
}


// ============================================================================
// Function: Matrix_Copy @ 6ba63cb2
// ============================================================================

void Matrix_Copy(undefined8 param_1,longlong param_2)

{
                    /* 0x23cb2  904  Matrix_Copy */
  Matrix_Init(param_1,*(undefined1 *)(param_2 + 100),*(undefined1 *)(param_2 + 0x65),param_2);
  return;
}


// ============================================================================
// Function: Matrix_T @ 6ba62850
// ============================================================================

void Matrix_T(longlong param_1,longlong param_2)

{
  byte bVar1;
  byte bVar2;
  int local_10;
  int local_c;
  
  bVar1 = *(byte *)(param_2 + 100);
  bVar2 = *(byte *)(param_2 + 0x65);
  for (local_c = 0; local_c < (int)(uint)bVar1; local_c = local_c + 1) {
    for (local_10 = 0; local_10 < (int)(uint)bVar2; local_10 = local_10 + 1) {
      *(undefined4 *)
       (param_1 + (longlong)(int)((uint)*(byte *)(param_1 + 0x65) * local_10 + local_c) * 4) =
           *(undefined4 *)
            (param_2 + (longlong)(int)((uint)*(byte *)(param_2 + 0x65) * local_c + local_10) * 4);
    }
  }
  return;
}


// ============================================================================
// Function: Matrix_SortVector @ 6ba640cc
// ============================================================================

void Matrix_SortVector(longlong param_1)

{
  uint uVar1;
  uint local_18;
  uint local_14;
  uint local_10;
  float local_c;
  
  uVar1 = Matrix_GetSize(param_1);
  for (local_14 = 0; local_14 < uVar1; local_14 = local_14 + 1) {
    local_c = *(float *)(param_1 + (ulonglong)local_14 * 4);
    local_10 = local_14;
    local_18 = local_14;
    while (local_18 = local_18 + 1, local_18 < uVar1) {
      if (local_c < *(float *)(param_1 + (ulonglong)local_18 * 4)) {
        local_c = *(float *)(param_1 + (ulonglong)local_18 * 4);
        local_10 = local_18;
      }
    }
    *(undefined4 *)(param_1 + (longlong)(int)local_10 * 4) =
         *(undefined4 *)(param_1 + (ulonglong)local_14 * 4);
    *(float *)(param_1 + (ulonglong)local_14 * 4) = local_c;
  }
  return;
}


// ============================================================================
// Function: Matrix_QR @ 6ba64178
// ============================================================================

void Matrix_QR(longlong param_1,longlong param_2,longlong param_3)

{
  float local_f8 [28];
  float local_88 [27];
  float local_1c;
  uint local_18;
  uint local_14;
  uint local_10;
  uint local_c;
  
                    /* 0x24178  914  Matrix_QR */
  if (*(char *)(param_1 + 100) == *(char *)(param_1 + 0x65)) {
    Matrix_Init(param_2,*(undefined1 *)(param_1 + 100),*(undefined1 *)(param_1 + 0x65),0);
    Matrix_Init(param_3,*(undefined1 *)(param_1 + 100),*(undefined1 *)(param_1 + 0x65),0);
    Matrix_ReShape(local_88,*(undefined1 *)(param_1 + 100),1);
    Matrix_ReShape(local_f8,*(undefined1 *)(param_1 + 100),1);
    for (local_10 = 0; local_10 < *(byte *)(param_1 + 100); local_10 = local_10 + 1) {
      for (local_c = 0; local_c < *(byte *)(param_1 + 100); local_c = local_c + 1) {
        local_88[local_c] =
             *(float *)(param_1 + (ulonglong)(*(byte *)(param_1 + 0x65) * local_c + local_10) * 4);
        local_f8[local_c] =
             *(float *)(param_1 + (ulonglong)(*(byte *)(param_1 + 0x65) * local_c + local_10) * 4);
      }
      for (local_14 = 0; local_14 < local_10; local_14 = local_14 + 1) {
        *(float *)(param_3 + (ulonglong)(local_10 + *(byte *)(param_3 + 0x65) * local_14) * 4) =
             _rdata;
        for (local_18 = 0; local_18 < *(byte *)(param_1 + 100); local_18 = local_18 + 1) {
          *(float *)(param_3 + (ulonglong)(*(byte *)(param_3 + 0x65) * local_14 + local_10) * 4) =
               *(float *)(param_3 + (ulonglong)(*(byte *)(param_3 + 0x65) * local_14 + local_10) * 4
                         ) +
               local_88[local_18] *
               *(float *)(param_2 + (ulonglong)(*(byte *)(param_2 + 0x65) * local_18 + local_14) * 4
                         );
        }
        for (local_18 = 0; local_18 < *(byte *)(param_1 + 100); local_18 = local_18 + 1) {
          local_f8[local_18] =
               local_f8[local_18] -
               *(float *)(param_3 + (ulonglong)(*(byte *)(param_3 + 0x65) * local_14 + local_10) * 4
                         ) *
               *(float *)(param_2 + (ulonglong)(*(byte *)(param_2 + 0x65) * local_18 + local_14) * 4
                         );
        }
      }
      local_1c = (float)Matrix_Norm2(local_f8);
      *(float *)(param_3 + (ulonglong)((*(byte *)(param_3 + 0x65) + 1) * local_10) * 4) = local_1c;
      if (_rdata == local_1c) {
        local_1c = DAT_6bb523a0;
      }
      for (local_c = 0; local_c < *(byte *)(param_1 + 100); local_c = local_c + 1) {
        *(float *)(param_2 + (ulonglong)(*(byte *)(param_2 + 0x65) * local_c + local_10) * 4) =
             local_f8[local_c] / local_1c;
      }
    }
  }
  return;
}


// ============================================================================
// Function: Matrix_Eigenvectors @ 6ba645eb
// ============================================================================

void Matrix_Eigenvectors(longlong param_1,longlong param_2,longlong param_3)

{
  double dVar1;
  float local_98 [25];
  byte local_34;
  byte local_33;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  int local_1c;
  uint local_18;
  uint local_14;
  uint local_10;
  uint local_c;
  
  Matrix_ReShape(local_98,*(undefined1 *)(param_2 + 100),*(undefined1 *)(param_2 + 0x65));
  for (local_18 = 0; local_18 < *(byte *)(param_2 + 0x65); local_18 = local_18 + 1) {
    local_28 = *(float *)(param_3 + (ulonglong)local_18 * 4);
    Matrix_Copy(local_98,param_2);
    for (local_c = 0; local_c < local_33; local_c = local_c + 1) {
      local_98[(local_33 + 1) * local_c] = local_98[(local_33 + 1) * local_c] - local_28;
    }
    for (local_c = 0; local_c + 1 < (uint)local_34; local_c = local_c + 1) {
      local_2c = local_98[(local_33 + 1) * local_c];
      for (local_10 = local_c; local_10 < local_33; local_10 = local_10 + 1) {
        local_98[local_33 * local_c + local_10] = local_98[local_10 + local_33 * local_c] / local_2c
        ;
      }
      for (local_10 = local_c + 1; local_10 < local_34; local_10 = local_10 + 1) {
        local_2c = local_98[local_c + local_33 * local_10];
        for (local_14 = local_c; local_14 < local_33; local_14 = local_14 + 1) {
          local_98[local_33 * local_10 + local_14] =
               local_98[local_14 + local_33 * local_10] -
               local_98[local_14 + local_33 * local_c] * local_2c;
        }
      }
    }
    local_24 = DAT_6bb52380;
    *(float *)(param_1 +
              (ulonglong)
              (local_18 + (uint)*(byte *)(param_1 + 0x65) * (*(byte *)(param_1 + 100) - 1)) * 4) =
         DAT_6bb52380;
    for (local_1c = local_34 - 2; -1 < local_1c; local_1c = local_1c + -1) {
      local_20 = _rdata;
      for (local_10 = local_1c + 1; local_10 < local_33; local_10 = local_10 + 1) {
        local_20 = local_20 +
                   local_98[local_10 + (uint)local_33 * local_1c] *
                   *(float *)(param_1 +
                             (ulonglong)(*(byte *)(param_1 + 0x65) * local_10 + local_18) * 4);
      }
      local_20 = -local_20 / local_98[(int)((local_33 + 1) * local_1c)];
      local_24 = local_24 + local_20 * local_20;
      *(float *)(param_1 + (ulonglong)(local_18 + (uint)*(byte *)(param_1 + 0x65) * local_1c) * 4) =
           local_20;
    }
    dVar1 = sqrt((double)local_24);
    local_24 = (float)dVar1;
    for (local_c = 0; local_c < *(byte *)(param_1 + 100); local_c = local_c + 1) {
      *(float *)(param_1 + (ulonglong)(*(byte *)(param_1 + 0x65) * local_c + local_18) * 4) =
           *(float *)(param_1 + (ulonglong)(*(byte *)(param_1 + 0x65) * local_c + local_18) * 4) /
           local_24;
    }
  }
  return;
}


// ============================================================================
// Function: Matrix_I @ 6ba632c8
// ============================================================================

undefined8 Matrix_I(longlong param_1)

{
  undefined8 uVar1;
  longlong lVar2;
  float *pfVar3;
  int aiStack_208 [28];
  int aiStack_198 [28];
  float local_128 [28];
  float local_b8 [25];
  undefined4 local_54;
  undefined4 local_50;
  int local_4c;
  int local_48;
  undefined4 local_44;
  undefined4 local_40;
  uint local_3c;
  int local_38;
  int local_34;
  int local_30;
  int local_2c;
  int local_28;
  int local_24;
  float local_20;
  int local_1c;
  
  if (*(char *)(param_1 + 100) == '\x02') {
    uVar1 = Matrix_I2(param_1);
  }
  else if (*(char *)(param_1 + 100) == '\x04') {
    uVar1 = Matrix_I4(param_1);
  }
  else {
    local_3c = (uint)*(byte *)(param_1 + 100);
    pfVar3 = local_b8;
    for (lVar2 = 0xc; lVar2 != 0; lVar2 = lVar2 + -1) {
      pfVar3[0] = 0.0;
      pfVar3[1] = 0.0;
      pfVar3 = pfVar3 + 2;
    }
    *pfVar3 = 0.0;
    pfVar3 = local_128;
    for (lVar2 = 0xc; lVar2 != 0; lVar2 = lVar2 + -1) {
      pfVar3[0] = 0.0;
      pfVar3[1] = 0.0;
      pfVar3 = pfVar3 + 2;
    }
    *pfVar3 = 0.0;
    for (local_1c = 0; local_1c < (int)local_3c; local_1c = local_1c + 1) {
      local_20 = _rdata;
      local_24 = 0;
      local_28 = 0;
      for (local_30 = local_1c; local_30 < (int)local_3c; local_30 = local_30 + 1) {
        for (local_2c = local_1c; local_2c < (int)local_3c; local_2c = local_2c + 1) {
          if (ABS(local_20) <
              (float)(*(uint *)(param_1 +
                               (longlong)
                               (int)((uint)*(byte *)(param_1 + 0x65) * local_30 + local_2c) * 4) &
                     DAT_6bb52370)) {
            local_20 = *(float *)(param_1 +
                                 (longlong)
                                 (int)((uint)*(byte *)(param_1 + 0x65) * local_30 + local_2c) * 4);
            local_24 = local_30;
            local_28 = local_2c;
          }
        }
      }
      if (local_24 != local_1c) {
        for (local_2c = 0; local_2c < (int)local_3c; local_2c = local_2c + 1) {
          local_40 = *(undefined4 *)
                      (param_1 +
                      (longlong)(int)((uint)*(byte *)(param_1 + 0x65) * local_24 + local_2c) * 4);
          *(undefined4 *)
           (param_1 + (longlong)(int)((uint)*(byte *)(param_1 + 0x65) * local_24 + local_2c) * 4) =
               *(undefined4 *)
                (param_1 +
                (longlong)(int)((uint)*(byte *)(param_1 + 0x65) * local_1c + local_2c) * 4);
          *(undefined4 *)
           (param_1 + (longlong)(int)(local_2c + (uint)*(byte *)(param_1 + 0x65) * local_1c) * 4) =
               local_40;
        }
      }
      if (local_28 != local_1c) {
        for (local_30 = 0; local_30 < (int)local_3c; local_30 = local_30 + 1) {
          local_44 = *(undefined4 *)
                      (param_1 +
                      (longlong)(int)((uint)*(byte *)(param_1 + 0x65) * local_30 + local_28) * 4);
          *(undefined4 *)
           (param_1 + (longlong)(int)((uint)*(byte *)(param_1 + 0x65) * local_30 + local_28) * 4) =
               *(undefined4 *)
                (param_1 +
                (longlong)(int)((uint)*(byte *)(param_1 + 0x65) * local_30 + local_1c) * 4);
          *(undefined4 *)
           (param_1 + (longlong)(int)(local_1c + (uint)*(byte *)(param_1 + 0x65) * local_30) * 4) =
               local_44;
        }
      }
      aiStack_198[local_1c] = local_24;
      aiStack_208[local_1c] = local_28;
      local_20 = DAT_6bb52380 / local_20;
      for (local_2c = 0; local_2c < (int)local_3c; local_2c = local_2c + 1) {
        if (local_2c == local_1c) {
          local_b8[local_2c] = local_20;
          local_128[local_2c] = DAT_6bb52380;
        }
        else {
          local_b8[local_2c] =
               -*(float *)(param_1 +
                          (longlong)(int)((uint)*(byte *)(param_1 + 0x65) * local_1c + local_2c) * 4
                          ) * local_20;
          local_128[local_2c] =
               *(float *)(param_1 +
                         (longlong)(int)((uint)*(byte *)(param_1 + 0x65) * local_2c + local_1c) * 4)
          ;
        }
        *(float *)(param_1 +
                  (longlong)(int)(local_2c + (uint)*(byte *)(param_1 + 0x65) * local_1c) * 4) =
             _rdata;
        *(float *)(param_1 +
                  (longlong)(int)(local_1c + (uint)*(byte *)(param_1 + 0x65) * local_2c) * 4) =
             _rdata;
      }
      for (local_30 = 0; local_30 < (int)local_3c; local_30 = local_30 + 1) {
        for (local_2c = 0; local_2c < (int)local_3c; local_2c = local_2c + 1) {
          *(float *)(param_1 +
                    (longlong)(int)((uint)*(byte *)(param_1 + 0x65) * local_30 + local_2c) * 4) =
               *(float *)(param_1 +
                         (longlong)(int)((uint)*(byte *)(param_1 + 0x65) * local_30 + local_2c) * 4)
               + local_128[local_30] * local_b8[local_2c];
        }
      }
    }
    for (local_1c = local_3c - 1; -1 < local_1c; local_1c = local_1c + -1) {
      local_48 = aiStack_198[local_1c];
      local_4c = aiStack_208[local_1c];
      if (local_48 != local_1c) {
        for (local_34 = 0; local_34 < (int)local_3c; local_34 = local_34 + 1) {
          local_50 = *(undefined4 *)
                      (param_1 +
                      (longlong)(int)((uint)*(byte *)(param_1 + 0x65) * local_34 + local_48) * 4);
          *(undefined4 *)
           (param_1 + (longlong)(int)((uint)*(byte *)(param_1 + 0x65) * local_34 + local_48) * 4) =
               *(undefined4 *)
                (param_1 +
                (longlong)(int)((uint)*(byte *)(param_1 + 0x65) * local_34 + local_1c) * 4);
          *(undefined4 *)
           (param_1 + (longlong)(int)(local_1c + (uint)*(byte *)(param_1 + 0x65) * local_34) * 4) =
               local_50;
        }
      }
      if (local_4c != local_1c) {
        for (local_38 = 0; local_38 < (int)local_3c; local_38 = local_38 + 1) {
          local_54 = *(undefined4 *)
                      (param_1 +
                      (longlong)(int)((uint)*(byte *)(param_1 + 0x65) * local_4c + local_38) * 4);
          *(undefined4 *)
           (param_1 + (longlong)(int)((uint)*(byte *)(param_1 + 0x65) * local_4c + local_38) * 4) =
               *(undefined4 *)
                (param_1 +
                (longlong)(int)((uint)*(byte *)(param_1 + 0x65) * local_1c + local_38) * 4);
          *(undefined4 *)
           (param_1 + (longlong)(int)(local_38 + (uint)*(byte *)(param_1 + 0x65) * local_1c) * 4) =
               local_54;
        }
      }
    }
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: Matrix_GetSize @ 6ba64019
// ============================================================================

int Matrix_GetSize(longlong param_1)

{
  return (uint)*(byte *)(param_1 + 0x65) * (uint)*(byte *)(param_1 + 100);
}


// ============================================================================
// Function: Matrix_Norm2 @ 6ba6403c
// ============================================================================

undefined8 Matrix_Norm2(longlong param_1)

{
  uint uVar1;
  undefined8 extraout_XMM0_Qb;
  undefined1 auVar2 [16];
  undefined1 auVar3 [16];
  float local_10;
  uint local_c;
  
                    /* 0x2403c  913  Matrix_Norm2 */
  uVar1 = Matrix_GetSize(param_1);
  local_10 = _rdata;
  for (local_c = 0; local_c < uVar1; local_c = local_c + 1) {
    local_10 = local_10 +
               *(float *)(param_1 + (ulonglong)local_c * 4) *
               *(float *)(param_1 + (ulonglong)local_c * 4);
  }
  auVar2._0_8_ = sqrt((double)local_10);
  auVar2._8_8_ = extraout_XMM0_Qb;
  auVar3._4_12_ = auVar2._4_12_;
  auVar3._0_4_ = (float)auVar2._0_8_;
  return auVar3._0_8_;
}


// ============================================================================
// Function: Matrix_I4 @ 6ba62bba
// ============================================================================

undefined8 Matrix_I4(longlong param_1)

{
  int aiStack_88 [4];
  int aiStack_78 [4];
  float local_68 [9];
  undefined4 local_44;
  undefined4 local_40;
  int local_3c;
  int local_38;
  undefined4 local_34;
  undefined4 local_30;
  int local_2c;
  int local_28;
  int local_24;
  int local_20;
  int local_1c;
  int local_18;
  int local_14;
  float local_10;
  int local_c;
  
                    /* 0x22bba  907  Matrix_I4 */
  local_2c = 4;
  local_68[4] = 0.0;
  local_68[5] = 0.0;
  local_68[6] = 0.0;
  local_68[7] = 0.0;
  local_68[0] = 0.0;
  local_68[1] = 0.0;
  local_68[2] = 0.0;
  local_68[3] = 0.0;
  for (local_c = 0; local_c < 4; local_c = local_c + 1) {
    local_10 = _rdata;
    local_14 = 0;
    local_18 = 0;
    for (local_20 = local_c; local_20 < local_2c; local_20 = local_20 + 1) {
      for (local_1c = local_c; local_1c < local_2c; local_1c = local_1c + 1) {
        if (ABS(local_10) <
            (float)(*(uint *)(param_1 + (longlong)(local_20 * local_2c + local_1c) * 4) &
                   DAT_6bb52370)) {
          local_10 = *(float *)(param_1 + (longlong)(local_20 * local_2c + local_1c) * 4);
          local_14 = local_20;
          local_18 = local_1c;
        }
      }
    }
    if (local_14 != local_c) {
      for (local_1c = 0; local_1c < local_2c; local_1c = local_1c + 1) {
        local_30 = *(undefined4 *)(param_1 + (longlong)(local_14 * local_2c + local_1c) * 4);
        *(undefined4 *)(param_1 + (longlong)(local_14 * local_2c + local_1c) * 4) =
             *(undefined4 *)(param_1 + (longlong)(local_c * local_2c + local_1c) * 4);
        *(undefined4 *)(param_1 + (longlong)(local_1c + local_c * local_2c) * 4) = local_30;
      }
    }
    if (local_18 != local_c) {
      for (local_20 = 0; local_20 < local_2c; local_20 = local_20 + 1) {
        local_34 = *(undefined4 *)(param_1 + (longlong)(local_20 * local_2c + local_18) * 4);
        *(undefined4 *)(param_1 + (longlong)(local_20 * local_2c + local_18) * 4) =
             *(undefined4 *)(param_1 + (longlong)(local_20 * local_2c + local_c) * 4);
        *(undefined4 *)(param_1 + (longlong)(local_c + local_20 * local_2c) * 4) = local_34;
      }
    }
    aiStack_78[local_c] = local_14;
    aiStack_88[local_c] = local_18;
    local_10 = DAT_6bb52380 / local_10;
    for (local_1c = 0; local_1c < 4; local_1c = local_1c + 1) {
      if (local_1c == local_c) {
        local_68[(longlong)local_1c + 4] = local_10;
        local_68[local_1c] = DAT_6bb52380;
      }
      else {
        local_68[(longlong)local_1c + 4] =
             -*(float *)(param_1 + (longlong)(local_c * local_2c + local_1c) * 4) * local_10;
        local_68[local_1c] = *(float *)(param_1 + (longlong)(local_1c * local_2c + local_c) * 4);
      }
      *(float *)(param_1 + (longlong)(local_1c + local_c * local_2c) * 4) = _rdata;
      *(float *)(param_1 + (longlong)(local_c + local_1c * local_2c) * 4) = _rdata;
    }
    for (local_20 = 0; local_20 < local_2c; local_20 = local_20 + 1) {
      *(float *)(param_1 + (longlong)(local_20 * local_2c) * 4) =
           *(float *)(param_1 + (longlong)(local_20 * local_2c) * 4) +
           local_68[local_20] * local_68[4];
      *(float *)(param_1 + (longlong)(local_20 * local_2c + 1) * 4) =
           *(float *)(param_1 + (longlong)(local_20 * local_2c + 1) * 4) +
           local_68[local_20] * local_68[5];
      *(float *)(param_1 + (longlong)(local_20 * local_2c + 2) * 4) =
           *(float *)(param_1 + (longlong)(local_20 * local_2c + 2) * 4) +
           local_68[local_20] * local_68[6];
      *(float *)(param_1 + (longlong)(local_20 * local_2c + 3) * 4) =
           *(float *)(param_1 + (longlong)(local_20 * local_2c + 3) * 4) +
           local_68[local_20] * local_68[7];
    }
  }
  for (local_c = local_2c + -1; -1 < local_c; local_c = local_c + -1) {
    local_38 = aiStack_78[local_c];
    local_3c = aiStack_88[local_c];
    if (local_38 != local_c) {
      for (local_24 = 0; local_24 < local_2c; local_24 = local_24 + 1) {
        local_40 = *(undefined4 *)(param_1 + (longlong)(local_24 * local_2c + local_38) * 4);
        *(undefined4 *)(param_1 + (longlong)(local_24 * local_2c + local_38) * 4) =
             *(undefined4 *)(param_1 + (longlong)(local_24 * local_2c + local_c) * 4);
        *(undefined4 *)(param_1 + (longlong)(local_c + local_24 * local_2c) * 4) = local_40;
      }
    }
    if (local_3c != local_c) {
      for (local_28 = 0; local_28 < local_2c; local_28 = local_28 + 1) {
        local_44 = *(undefined4 *)(param_1 + (longlong)(local_3c * local_2c + local_28) * 4);
        *(undefined4 *)(param_1 + (longlong)(local_3c * local_2c + local_28) * 4) =
             *(undefined4 *)(param_1 + (longlong)(local_c * local_2c + local_28) * 4);
        *(undefined4 *)(param_1 + (longlong)(local_28 + local_c * local_2c) * 4) = local_44;
      }
    }
  }
  return 0;
}


// ============================================================================
// Function: Matrix_I2 @ 6ba63163
// ============================================================================

undefined8 Matrix_I2(float *param_1)

{
  float fVar1;
  float fVar2;
  
                    /* 0x23163  906  Matrix_I2 */
  fVar2 = DAT_6bb52380 /
          (*param_1 * param_1[(int)(*(byte *)((longlong)param_1 + 0x65) + 1)] -
          param_1[1] * param_1[(int)(uint)*(byte *)((longlong)param_1 + 0x65)]);
  fVar1 = *param_1;
  *param_1 = param_1[(int)(*(byte *)((longlong)param_1 + 0x65) + 1)] * fVar2;
  param_1[1] = param_1[1] * (float)((uint)fVar2 ^ DAT_6bb52390);
  param_1[(int)(uint)*(byte *)((longlong)param_1 + 0x65)] =
       param_1[(int)(uint)*(byte *)((longlong)param_1 + 0x65)] * (float)((uint)fVar2 ^ DAT_6bb52390)
  ;
  param_1[(int)(*(byte *)((longlong)param_1 + 0x65) + 1)] = fVar1 * fVar2;
  return 0;
}


// ============================================================================
// Function: Matrix_Add @ 6ba63cf4
// ============================================================================

void Matrix_Add(undefined8 param_1,longlong param_2,undefined8 param_3)

{
                    /* 0x23cf4  903  Matrix_Add */
  Matrix_ReShape(param_1,*(undefined1 *)(param_2 + 100),*(undefined1 *)(param_2 + 0x65));
  Matrix_A(param_1,param_2,param_3);
  return;
}


// ============================================================================
// Function: Matrix_Mul_C @ 6ba63f11
// ============================================================================

void Matrix_Mul_C(longlong param_1,undefined8 param_2,float param_3)

{
  int local_c;
  
                    /* 0x23f11  912  Matrix_Mul_C */
  Matrix_Copy(param_1,param_2);
  for (local_c = 0;
      local_c < (int)((uint)*(byte *)(param_1 + 0x65) * (uint)*(byte *)(param_1 + 100));
      local_c = local_c + 1) {
    *(float *)(param_1 + (longlong)local_c * 4) =
         *(float *)(param_1 + (longlong)local_c * 4) * param_3;
  }
  return;
}


// ============================================================================
// Function: Matrix_A @ 6ba628ed
// ============================================================================

void Matrix_A(longlong param_1,longlong param_2,longlong param_3)

{
  byte bVar1;
  byte bVar2;
  int local_10;
  int local_c;
  
  bVar1 = *(byte *)(param_2 + 100);
  bVar2 = *(byte *)(param_2 + 0x65);
  for (local_c = 0; local_c < (int)(uint)bVar1; local_c = local_c + 1) {
    for (local_10 = 0; local_10 < (int)(uint)bVar2; local_10 = local_10 + 1) {
      *(float *)(param_1 + (longlong)(int)((uint)*(byte *)(param_1 + 0x65) * local_c + local_10) * 4
                ) = *(float *)(param_2 +
                              (longlong)(int)((uint)*(byte *)(param_2 + 0x65) * local_c + local_10)
                              * 4) +
                    *(float *)(param_3 +
                              (longlong)(int)((uint)*(byte *)(param_3 + 0x65) * local_c + local_10)
                              * 4);
    }
  }
  return;
}


