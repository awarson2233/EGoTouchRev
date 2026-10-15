// ============================================================================
// Function: Gesture_Process @ 6bae16db
// ============================================================================

void Gesture_Process(void)

{
                    /* 0xa16db  609  Gesture_Process */
  if (((_refptr_g_tsaPrmtDynamic[0x80] & 8) == 0) ||
     (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x10) == 0)) {
    Gesture_NormalProcess();
  }
  else {
    Gesture_StylusProcess();
  }
  return;
}


// ============================================================================
// Function: Gesture_NormalProcess @ 6bae1460
// ============================================================================

void Gesture_NormalProcess(void)

{
  byte local_b;
  byte local_a;
  char local_9;
  
  local_9 = '\0';
  local_a = 0;
  for (local_b = 0; local_b < **(byte **)_refptr_g_touchesPtr; local_b = local_b + 1) {
    if ((*(uint *)((longlong)(int)(uint)local_b * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x204)
        & 0xf000) == 0) {
      local_9 = local_9 + '\x01';
      local_a = local_b;
    }
  }
  if (local_9 == '\x01') {
    if (*(int *)((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c)
        == 4) {
      Gesture_AddSample(local_a);
    }
    else if (*(int *)((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x21c) == 0x20) {
      Gesture_Decode();
      *(undefined1 *)
       ((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x278) =
           g_gestureSamples;
    }
    else {
      Gesture_Reset();
    }
  }
  else {
    Gesture_Reset();
  }
  return;
}


// ============================================================================
// Function: Gesture_StylusProcess @ 6bae157c
// ============================================================================

void Gesture_StylusProcess(void)

{
  char cVar1;
  byte bVar2;
  
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x1000) == 0) {
    if (*(char *)(*(longlong *)_refptr_g_touchesPtr + 0xc) == '\0') {
      Gesture_NormalProcess();
    }
    else if ((*_refptr_g_prevTouchesPtr)->field_0xc == '\0') {
      Gesture_Reset();
    }
    else {
      bVar2 = TouchMode_StylusGetCoreIndex();
      if (*(uint *)((longlong)(int)(uint)bVar2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c)
          < 5) {
        Gesture_AddSample(bVar2);
      }
      else if (*(int *)((longlong)(int)(uint)bVar2 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x21c) == 0x20) {
        Gesture_Decode();
        cVar1 = Gesture_GetCode();
        if ((cVar1 == 'S') || (cVar1 = Gesture_GetCode(), cVar1 == 's')) {
          (*_refptr_g_tsaStaticPtr)->dwFeatureFlags =
               (*_refptr_g_tsaStaticPtr)->dwFeatureFlags | 0x1000;
        }
      }
      else {
        Gesture_Reset();
      }
    }
  }
  else {
    Gesture_NormalProcess();
    cVar1 = Gesture_GetCode();
    if (((cVar1 == 'S') || (cVar1 = Gesture_GetCode(), cVar1 == 's')) &&
       (*(char *)(*(longlong *)_refptr_g_touchesPtr + 0xc) == '\0')) {
      (*_refptr_g_tsaStaticPtr)->dwFeatureFlags =
           (*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0xffffefff;
    }
  }
  return;
}


// ============================================================================
// Function: Gesture_Init @ 6bae0083
// ============================================================================

void Gesture_Init(void)

{
                    /* 0xa0083  608  Gesture_Init */
  Gesture_Reset();
  Gesture_InitList();
  return;
}


// ============================================================================
// Function: Gesture_Reset @ 6bae0010
// ============================================================================

void Gesture_Reset(void)

{
  DAT_6bba2e82 = 0;
  g_gestureSamples = 0;
  return;
}


// ============================================================================
// Function: Gesture_Decode @ 6bae11af
// ============================================================================

undefined1 Gesture_Decode(void)

{
  short sVar1;
  short sVar2;
  ushort uVar3;
  ushort uVar4;
  uint uVar5;
  undefined1 local_21;
  uint local_20;
  byte local_1a;
  byte local_19;
  
  local_1a = 0;
  local_20 = 0xffffffff;
  local_21 = 0;
  if ((0xe < DAT_6bba2e82) || (0x1e < DAT_6bba2e82)) {
    Gesture_Resample();
    Gesture_PositionNormalization();
    Gesture_PrintSamples();
    local_19 = 0;
    while (local_19 == 0) {
      Gesture_ShapeNormalization(0);
      uVar5 = Gesture_CalcError(0);
      if (uVar5 < local_20) {
        local_1a = local_19;
        local_20 = uVar5;
      }
      local_19 = 1;
    }
    for (local_19 = 0; local_19 < 0x1e; local_19 = local_19 + 1) {
      sVar1 = *(short *)(&DAT_6bb576c4 +
                        (longlong)(int)(uint)local_19 * 4 + (longlong)(int)(uint)local_1a * 0x7a);
      uVar3 = TSA_GetYRes();
      sVar2 = *(short *)(&DAT_6bb576c2 +
                        (longlong)(int)(uint)local_19 * 4 + (longlong)(int)(uint)local_1a * 0x7a);
      uVar4 = TSA_GetXRes();
      TSAIDE_LogPointXY((uVar4 >> 1) + sVar2,(uVar3 >> 1) + sVar1,6);
    }
    if (local_20 < 200000) {
      local_21 = (&g_gestureTemplates)[(longlong)(int)(uint)local_1a * 0x7a];
    }
  }
  g_gestureSamples = local_21;
  return local_21;
}


// ============================================================================
// Function: Gesture_AddSample @ 6bae130d
// ============================================================================

void Gesture_AddSample(byte param_1)

{
  undefined2 uVar1;
  uint uVar2;
  
  if (((DAT_6bba2e82 < 200) &&
      (*(ushort *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x156)
       < 300)) &&
     (*(ushort *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x188)
      < 0x19)) {
    DAT_6bba2e84 = (short)*(undefined4 *)
                           ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                           + 0x244) + DAT_6bba2e84;
    if ((DAT_6bba2e82 == 0) || (1000 < DAT_6bba2e84)) {
      uVar2 = (uint)DAT_6bba2e82;
      uVar1 = TSAOut_DisplayXPos(param_1);
      (&DAT_6bba2e8c)[(longlong)(int)uVar2 * 2] = uVar1;
      uVar2 = (uint)DAT_6bba2e82;
      uVar1 = TSAOut_DisplayYPos(param_1);
      (&DAT_6bba2e8e)[(longlong)(int)uVar2 * 2] = uVar1;
      DAT_6bba2e82 = DAT_6bba2e82 + 1;
      DAT_6bba2e84 = 0;
    }
  }
  else {
    Gesture_Reset();
  }
  return;
}


// ============================================================================
// Function: Gesture_GetCode @ 6bae10c7
// ============================================================================

undefined1 Gesture_GetCode(void)

{
                    /* 0xa10c7  606  Gesture_GetCode */
  return g_gestureSamples;
}


// ============================================================================
// Function: Gesture_InitList @ 6bae0026
// ============================================================================

void Gesture_InitList(void)

{
  byte local_9;
  
  local_9 = 0;
  while (local_9 == 0) {
    g_gestureList = g_gestureTemplates;
    local_9 = 1;
  }
  (&g_gestureList)[(int)(uint)local_9] = 0;
  return;
}


// ============================================================================
// Function: Gesture_ShapeNormalization @ 6bae02b1
// ============================================================================

void Gesture_ShapeNormalization(byte param_1)

{
  short sVar1;
  short sVar2;
  ushort uVar3;
  ushort uVar4;
  double dVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  int local_50;
  float local_4c;
  float local_48;
  float local_44;
  float local_40;
  float local_3c;
  float local_38;
  float local_34;
  float local_30;
  int local_2c;
  
  local_30 = DAT_6bb577d4;
  local_34 = DAT_6bb577d4;
  local_38 = DAT_6bb577d4;
  local_3c = DAT_6bb577d4;
  local_40 = DAT_6bb577d4;
  local_44 = DAT_6bb577d4;
  local_48 = DAT_6bb577d4;
  local_4c = DAT_6bb577d4;
  for (local_2c = 0; local_2c < (int)(uint)DAT_6bba2e82; local_2c = local_2c + 1) {
    local_30 = local_30 +
               (float)((int)(short)(&DAT_6bba2e8c)[(longlong)local_2c * 2] *
                      (int)(short)(&DAT_6bba2e8c)[(longlong)local_2c * 2]);
    local_34 = local_34 +
               (float)((int)(short)(&DAT_6bba2e8e)[(longlong)local_2c * 2] *
                      (int)(short)(&DAT_6bba2e8c)[(longlong)local_2c * 2]);
    local_38 = local_38 +
               (float)((int)(short)(&DAT_6bba2e8c)[(longlong)local_2c * 2] *
                      (int)(short)(&DAT_6bba2e8e)[(longlong)local_2c * 2]);
    local_3c = local_3c +
               (float)((int)(short)(&DAT_6bba2e8e)[(longlong)local_2c * 2] *
                      (int)(short)(&DAT_6bba2e8e)[(longlong)local_2c * 2]);
    local_40 = local_40 +
               (float)((int)(short)(&DAT_6bba2e8c)[(longlong)local_2c * 2] *
                      (int)*(short *)(&DAT_6bb576c2 +
                                     (longlong)local_2c * 4 + (longlong)(int)(uint)param_1 * 0x7a));
    local_44 = local_44 +
               (float)((int)(short)(&DAT_6bba2e8e)[(longlong)local_2c * 2] *
                      (int)*(short *)(&DAT_6bb576c2 +
                                     (longlong)local_2c * 4 + (longlong)(int)(uint)param_1 * 0x7a));
    local_48 = local_48 +
               (float)((int)(short)(&DAT_6bba2e8c)[(longlong)local_2c * 2] *
                      (int)*(short *)(&DAT_6bb576c2 +
                                     (longlong)local_2c * 4 + (longlong)(int)(uint)param_1 * 0x7a +
                                     2));
    local_4c = local_4c +
               (float)((int)(short)(&DAT_6bba2e8e)[(longlong)local_2c * 2] *
                      (int)*(short *)(&DAT_6bb576c2 +
                                     (longlong)local_2c * 4 + (longlong)(int)(uint)param_1 * 0x7a +
                                     2));
  }
  fVar10 = local_30 * local_3c - local_34 * local_38;
  if ((float)((uint)fVar10 & DAT_6bb577e0) < 1e-06) {
    fVar10 = DAT_6bb577f0;
  }
  fVar6 = (local_40 * local_3c + -local_38 * local_44) / fVar10;
  fVar8 = (-local_34 * local_40 + local_44 * local_30) / fVar10;
  fVar9 = (local_48 * local_3c + -local_38 * local_4c) / fVar10;
  fVar10 = (-local_34 * local_48 + local_4c * local_30) / fVar10;
  fVar7 = fVar6 * fVar10 - fVar8 * fVar9;
  if ((float)((uint)fVar7 & DAT_6bb577e0) < 1e-06) {
    fVar7 = DAT_6bb577f0;
  }
  fVar7 = (fVar6 * fVar6 + fVar8 * fVar8 + fVar9 * fVar9 + fVar10 * fVar10) / fVar7;
  dVar5 = sqrt((double)(fVar7 * fVar7 - DAT_6bb57810));
  fVar7 = (float)((dVar5 + (double)fVar7) / 2.0);
  local_50 = 1;
  if ((fVar7 < DAT_6bb57814) || (DAT_6bb57818 / DAT_6bb57814 < fVar7)) {
    local_50 = 2;
  }
  for (local_2c = 0; local_2c < (int)(uint)DAT_6bba2e82; local_2c = local_2c + 1) {
    sVar1 = (&DAT_6bba2e8c)[(longlong)local_2c * 2];
    sVar2 = (&DAT_6bba2e8e)[(longlong)local_2c * 2];
    (&DAT_6bba2e8c)[(longlong)local_2c * 2] =
         (short)((int)(short)(int)((float)(int)sVar2 * fVar8 + (float)(int)sVar1 * fVar6) / local_50
                );
    (&DAT_6bba2e8e)[(longlong)local_2c * 2] =
         (short)((int)(short)(int)((float)(int)sVar2 * fVar10 + (float)(int)sVar1 * fVar9) /
                local_50);
  }
  uVar3 = TSA_GetYRes();
  uVar4 = TSA_GetXRes();
  Gesture_LogGestureSample(uVar4 >> 1,uVar3 >> 1,3);
  return;
}


// ============================================================================
// Function: Gesture_Resample @ 6bae09dc
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void Gesture_Resample(void)

{
  ushort uVar1;
  undefined4 uVar2;
  int iVar3;
  int local_18;
  undefined1 local_11;
  int local_10;
  ushort local_c;
  ushort local_a;
  
  local_10 = 0;
  local_18 = 0;
  if (DAT_6bba2e82 == 0) {
    local_11 = 1;
  }
  else {
    local_11 = (undefined1)(200 / (ulonglong)(longlong)(int)(uint)DAT_6bba2e82);
  }
  uVar1 = Gesture_LinearInterpolation(&DAT_6bba2e8c,DAT_6bba2e82,local_11,&DAT_6bba2e8c,200);
  for (local_c = 0; (int)(uint)local_c < (int)(uVar1 - 1); local_c = local_c + 1) {
    uVar2 = Touch_GetSQDistPixel
                      ((&DAT_6bba2e8c)[(longlong)(int)(uint)local_c * 2],
                       (&DAT_6bba2e8e)[(longlong)(int)(uint)local_c * 2],
                       (&DAT_6bba2e8c)[(longlong)(int)(local_c + 1) * 2],
                       (&DAT_6bba2e8e)[(longlong)(int)(local_c + 1) * 2]);
    iVar3 = Misc_SqrtUint32(uVar2);
    *(short *)(&distanceBuffer_10897 + (longlong)(int)(uint)local_c * 2) = (short)iVar3;
    local_18 = local_18 + iVar3;
  }
  local_c = 0;
  for (local_a = 1; local_a < 0x1e; local_a = local_a + 1) {
    for (; (local_10 < (int)((uint)local_a * (local_18 / 0x1d)) &&
           ((int)(uint)local_c < (int)(uVar1 - 1))); local_c = local_c + 1) {
      local_10 = local_10 +
                 (uint)*(ushort *)(&distanceBuffer_10897 + (longlong)(int)(uint)local_c * 2);
    }
    (&DAT_6bba2e8c)[(longlong)(int)(uint)local_a * 2] =
         (&DAT_6bba2e8c)[(longlong)(int)(uint)local_c * 2];
    (&DAT_6bba2e8e)[(longlong)(int)(uint)local_a * 2] =
         (&DAT_6bba2e8e)[(longlong)(int)(uint)local_c * 2];
  }
  DAT_6bba2e82 = local_a;
  _DAT_6bba2e88 = 0x488;
  Gesture_LogGestureSample(0,0,1);
  if (DAT_6bba2e82 != 0x1e) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/Gesture/Gesture.c",0xda,
                      "g_gestureSamples.num == GESTURE_TEMPLATE_SAMPLE_NUM");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: Gesture_PositionNormalization @ 6bae0ca8
// ============================================================================

void Gesture_PositionNormalization(ulonglong param_1,ulonglong param_2)

{
  longlong lVar1;
  ushort uVar2;
  ushort uVar3;
  int local_24;
  int local_20;
  ushort local_1a;
  
  local_20 = 0;
  local_24 = 0;
  for (local_1a = 0; local_1a < DAT_6bba2e82; local_1a = local_1a + 1) {
    local_20 = local_20 + (short)(&DAT_6bba2e8c)[(longlong)(int)(uint)local_1a * 2];
    param_2 = (longlong)(int)(uint)local_1a * 4;
    local_24 = local_24 + (short)(&DAT_6bba2e8e)[(longlong)(int)(uint)local_1a * 2];
  }
  if (DAT_6bba2e82 != 0) {
    local_20 = local_20 / (int)(uint)DAT_6bba2e82;
    lVar1 = (longlong)local_24;
    local_24 = local_24 / (int)(uint)DAT_6bba2e82;
    param_2 = lVar1 % (longlong)(int)(uint)DAT_6bba2e82 & 0xffffffff;
  }
  for (local_1a = 0; local_1a < DAT_6bba2e82; local_1a = local_1a + 1) {
    (&DAT_6bba2e8c)[(longlong)(int)(uint)local_1a * 2] =
         (&DAT_6bba2e8c)[(longlong)(int)(uint)local_1a * 2] - (short)local_20;
    param_1 = (ulonglong)
              ((uint)(ushort)(&DAT_6bba2e8e)[(longlong)(int)(uint)local_1a * 2] - local_24);
    param_2 = (longlong)(int)(uint)local_1a * 4;
    (&DAT_6bba2e8e)[(longlong)(int)(uint)local_1a * 2] =
         (short)((uint)(ushort)(&DAT_6bba2e8e)[(longlong)(int)(uint)local_1a * 2] - local_24);
  }
  uVar2 = TSA_GetYRes(param_1,param_2);
  uVar3 = TSA_GetXRes();
  Gesture_LogGestureSample(uVar3 >> 1,uVar2 >> 1,5);
  return;
}


// ============================================================================
// Function: Gesture_CalcError @ 6bae0e26
// ============================================================================

uint Gesture_CalcError(byte param_1)

{
  int iVar1;
  int iVar2;
  uint local_14;
  uint local_10;
  ushort local_a;
  
  local_14 = 0xffffffff;
  local_10 = 0;
  for (local_a = 0; local_a < DAT_6bba2e82; local_a = local_a + 1) {
    iVar1 = (int)(short)(&DAT_6bba2e8c)[(longlong)(int)(uint)local_a * 2] -
            (int)*(short *)(&DAT_6bb576c2 +
                           (ulonglong)local_a * 4 + (longlong)(int)(uint)param_1 * 0x7a);
    iVar2 = (int)(short)(&DAT_6bba2e8e)[(longlong)(int)(uint)local_a * 2] -
            (int)*(short *)(&DAT_6bb576c2 +
                           (ulonglong)local_a * 4 + (longlong)(int)(uint)param_1 * 0x7a + 2);
    local_10 = local_10 + iVar1 * iVar1 + iVar2 * iVar2;
  }
  if (local_10 != 0xffffffff) {
    local_14 = local_10;
  }
  local_10 = 0;
  for (local_a = 1; local_a < DAT_6bba2e82; local_a = local_a + 1) {
    iVar1 = (int)(short)(&DAT_6bba2e8c)[(longlong)(int)(uint)local_a * 2] -
            (int)*(short *)(&DAT_6bb576c2 +
                           (ulonglong)local_a * 4 + -4 + (longlong)(int)(uint)param_1 * 0x7a);
    iVar2 = (int)(short)(&DAT_6bba2e8e)[(longlong)(int)(uint)local_a * 2] -
            (int)*(short *)(&DAT_6bb576c2 +
                           (ulonglong)local_a * 4 + (longlong)(int)(uint)param_1 * 0x7a + -2);
    local_10 = local_10 + iVar1 * iVar1 + iVar2 * iVar2;
  }
  if (local_10 < local_14) {
    local_14 = local_10;
  }
  local_10 = 0;
  for (local_a = 1; local_a < DAT_6bba2e82; local_a = local_a + 1) {
    iVar1 = (int)(short)(&DAT_6bba2e8c)[(longlong)(int)(local_a - 1) * 2] -
            (int)*(short *)(&DAT_6bb576c2 +
                           (ulonglong)local_a * 4 + (longlong)(int)(uint)param_1 * 0x7a);
    iVar2 = (int)(short)(&DAT_6bba2e8e)[(longlong)(int)(local_a - 1) * 2] -
            (int)*(short *)(&DAT_6bb576c2 +
                           (ulonglong)local_a * 4 + (longlong)(int)(uint)param_1 * 0x7a + 2);
    local_10 = local_10 + iVar1 * iVar1 + iVar2 * iVar2;
  }
  if (local_10 < local_14) {
    local_14 = local_10;
  }
  return local_14;
}


// ============================================================================
// Function: Gesture_PrintSamples @ 6bae10d4
// ============================================================================

void Gesture_PrintSamples(void)

{
  char cVar1;
  byte local_9;
  
  cVar1 = TSALog_RtEnable(1,0xf,1);
  if (cVar1 != '\0') {
    for (local_9 = 0; local_9 < 0x1e; local_9 = local_9 + 1) {
      if (local_9 ==
          (byte)(((byte)(((uint)local_9 * 0x21 + (uint)local_9 * 8) * 5 >> 8) >> 2) * '\x05')) {
        StrPrintf_Process(0,&DAT_6bb577c4);
      }
      StrPrintf_Process(0,"{%4d, %4d}, ",
                        (int)(short)(&DAT_6bba2e8c)[(longlong)(int)(uint)local_9 * 2],
                        (int)(short)(&DAT_6bba2e8e)[(longlong)(int)(uint)local_9 * 2]);
    }
  }
  return;
}


// ============================================================================
// Function: Gesture_LogGestureSample @ 6bae0224
// ============================================================================

void Gesture_LogGestureSample(short param_1,short param_2,undefined4 param_3)

{
  int local_c;
  
  for (local_c = 0; local_c < (int)(uint)DAT_6bba2e82; local_c = local_c + 1) {
    TSAIDE_LogPointXY(param_1 + (&DAT_6bba2e8c)[(longlong)local_c * 2],
                      param_2 + (&DAT_6bba2e8e)[(longlong)local_c * 2],param_3);
  }
  return;
}


// ============================================================================
// Function: Gesture_LinearInterpolation @ 6bae00a9
// ============================================================================

short Gesture_LinearInterpolation
                (longlong param_1,ushort param_2,byte param_3,longlong param_4,ushort param_5)

{
  ushort uVar1;
  uint local_10;
  int local_c;
  
  for (local_c = param_2 - 2; -1 < local_c; local_c = local_c + -1) {
    for (local_10 = (uint)param_3; -1 < (int)local_10; local_10 = local_10 - 1) {
      uVar1 = (ushort)param_3 * (short)local_c + (short)local_10;
      if ((uVar1 < param_5) && (param_3 != 0)) {
        *(short *)((ulonglong)uVar1 * 4 + param_4) =
             (short)((int)((int)*(short *)(param_1 + ((longlong)local_c + 1) * 4) * local_10 +
                          (int)*(short *)(param_1 + (longlong)local_c * 4) * (param_3 - local_10)) /
                    (int)(uint)param_3);
        *(short *)((ulonglong)uVar1 * 4 + param_4 + 2) =
             (short)((int)((int)*(short *)(param_1 + ((longlong)local_c + 1) * 4 + 2) * local_10 +
                          (int)*(short *)(param_1 + (longlong)local_c * 4 + 2) *
                          (param_3 - local_10)) / (int)(uint)param_3);
      }
    }
  }
  return (ushort)param_3 * (param_2 - 1) + 1;
}


