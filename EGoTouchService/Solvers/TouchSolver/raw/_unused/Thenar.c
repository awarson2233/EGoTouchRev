// ============================================================================
// Function: Thenar_ProcessInfoByDif @ 6bb315e8
// ============================================================================

void Thenar_ProcessInfoByDif(void)

{
                    /* 0xf15e8  2459  Thenar_ProcessInfoByDif */
  Thenar_ProcessInfoByDifTraditional();
  Thenar_EventUpdate();
  Thenar_UnitCopy(&(*_refptr_g_prevTouchesPtr)[1].field_0x1c,
                  *(longlong *)_refptr_g_touchesPtr + 0x40);
  return;
}


// ============================================================================
// Function: Thenar_UnitCopy @ 6bb2fd40
// ============================================================================

void Thenar_UnitCopy(undefined1 *param_1,undefined1 *param_2)

{
  ushort local_a;
  
                    /* 0xefd40  2461  Thenar_UnitCopy */
  *param_1 = *param_2;
  param_1[1] = param_2[1];
  param_1[2] = param_2[2];
  param_1[3] = param_2[3];
  param_1[4] = param_2[4];
  param_1[5] = param_2[5];
  *(undefined4 *)(param_1 + 8) = *(undefined4 *)(param_2 + 8);
  *(undefined2 *)(param_1 + 0x10) = *(undefined2 *)(param_2 + 0x10);
  *(undefined8 *)(param_1 + 0x18) = *(undefined8 *)(param_2 + 0x18);
  for (local_a = 0; local_a < 0x14; local_a = local_a + 1) {
    *(undefined8 *)(param_1 + (longlong)(int)(uint)local_a * 10 + 0x28) =
         *(undefined8 *)(param_2 + (longlong)(int)(uint)local_a * 10 + 0x28);
    *(undefined2 *)(param_1 + (longlong)(int)(uint)local_a * 10 + 0x30) =
         *(undefined2 *)(param_2 + (longlong)(int)(uint)local_a * 10 + 0x30);
  }
  param_1[0xf0] = param_2[0xf0];
  return;
}


// ============================================================================
// Function: Thenar_EventUpdate @ 6bb30eb3
// ============================================================================

void Thenar_EventUpdate(void)

{
  Thenar_EventUpdateUnit(4,0,1);
  if ((0x23 < g_sideHandEnterCnt) &&
     (*(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
           *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) | 2,
     *(char *)(*(longlong *)_refptr_g_touchesPtr + 0x45) != '\0')) {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) | 8;
  }
  Thenar_EventUpdateUnit(8,1,0);
  if ((0x23 < DAT_6bbaa8d4) &&
     (*(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
           *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) | 4,
     *(char *)(*(longlong *)_refptr_g_touchesPtr + 0x45) != '\0')) {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) | 0x10;
  }
  return;
}


// ============================================================================
// Function: Thenar_ProcessInfoByDifTraditional @ 6bb31392
// ============================================================================

void Thenar_ProcessInfoByDifTraditional(void)

{
  ushort local_28;
  ushort local_26;
  ushort local_24;
  ushort local_22;
  byte local_20;
  byte local_1f;
  ushort local_1e;
  ushort local_1c;
  ushort local_c;
  byte local_9;
  
  Thenar_FeatureExtraction(*(undefined8 *)_refptr_g_tsaBufDif,0x1324);
  Thenar_UpdateDetectFeature(&local_28);
  for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
    Thenar_SideHandDetectByCoreTouch(local_9);
  }
  local_c = (ushort)(((uint)(byte)_refptr_g_tsaPrmtFlash->field_0x836 << 4) /
                    (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x1b);
  *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0x41) = 1;
  if ((local_20 < 3) || (2 < local_1f)) {
    if ((local_1f < 3) || (2 < local_20)) {
      if ((local_1e < 4) && (4 < local_1c)) {
        *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0x40) = 1;
        *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x48) = 8;
      }
      else if ((local_1c < 4) && (4 < local_1e)) {
        *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0x40) = 1;
        *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x48) = 4;
      }
      else if ((local_24 < local_28) && (local_c <= local_28)) {
        *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0x40) = 1;
        *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x48) = 4;
      }
      else if ((local_28 < local_24) && (local_c <= local_24)) {
        *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0x40) = 1;
        *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x48) = 8;
      }
    }
    else {
      *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0x40) = 1;
      *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x48) = 4;
    }
  }
  else {
    *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0x40) = 1;
    *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x48) = 8;
  }
  if (local_28 < local_24) {
    Thenar_ProcessGensture
              ((int)*(short *)((longlong)(int)(uint)local_22 * 10 +
                               *(longlong *)_refptr_g_touchesPtr + 0x70));
  }
  else {
    Thenar_ProcessGensture
              ((int)*(short *)((longlong)(int)(uint)local_26 * 10 +
                               *(longlong *)_refptr_g_touchesPtr + 0x70));
  }
  return;
}


// ============================================================================
// Function: Thenar_UpdateExternalInfo @ 6bb2fe59
// ============================================================================

bool Thenar_UpdateExternalInfo(void)

{
  bool local_9;
  
                    /* 0xefe59  2462  Thenar_UpdateExternalInfo */
  local_9 = *(char *)(*(longlong *)_refptr_g_touchesPtr + 0x40) == '\0';
  if (local_9) {
    *_refptr_g_tsaInitOnce = 0;
    *(undefined4 *)(_refptr_g_tsaInitOnce + 4) = 0;
    _refptr_g_tsaInitOnce[1] = 0;
  }
  else {
    *_refptr_g_tsaInitOnce = 1;
    *(undefined4 *)(_refptr_g_tsaInitOnce + 4) =
         *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x48);
    *(undefined2 *)(_refptr_g_tsaInitOnce + 0xc) =
         *(undefined2 *)(*(longlong *)_refptr_g_touchesPtr + 0x50);
    _refptr_g_tsaInitOnce[1] = *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0x45);
  }
  local_9 = !local_9;
  return local_9;
}


// ============================================================================
// Function: Thenar_EventUpdateUnit @ 6bb30d38
// ============================================================================

void Thenar_EventUpdateUnit(int param_1,uint param_2,uint param_3)

{
  if (*(int *)(*(longlong *)_refptr_g_touchesPtr + 0x48) == param_1) {
    if (((uint)(&g_sideHandEnterCnt)[param_2] < 0x25) &&
       (0xf < *(uint *)(&g_sideHandExitCnt + (ulonglong)param_3 * 4))) {
      (&g_sideHandEnterCnt)[param_2] = (&g_sideHandEnterCnt)[param_2] + 0xc;
    }
    *(undefined4 *)(&g_sideHandExitCnt + (ulonglong)param_2 * 4) = 0;
  }
  else {
    if (((&g_sideHandEnterCnt)[param_2] != 0) && ((uint)(&g_sideHandEnterCnt)[param_2] < 0x24)) {
      (&g_sideHandEnterCnt)[param_2] = (&g_sideHandEnterCnt)[param_2] + -1;
    }
    if (*(uint *)(&g_sideHandExitCnt + (ulonglong)param_2 * 4) < 0x10) {
      *(int *)(&g_sideHandExitCnt + (ulonglong)param_2 * 4) =
           *(int *)(&g_sideHandExitCnt + (ulonglong)param_2 * 4) + 1;
    }
  }
  if (0xc < *(uint *)(&g_sideHandExitCnt + (ulonglong)param_2 * 4)) {
    (&g_sideHandEnterCnt)[param_2] = 0;
  }
  return;
}


// ============================================================================
// Function: Thenar_ProcessGensture @ 6bb2fefc
// ============================================================================

void Thenar_ProcessGensture(short param_1)

{
  if (((*(char *)(*(longlong *)_refptr_g_touchesPtr + 0x40) != '\0') &&
      ((*(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) & 1) == 0)) &&
     (((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 3) * 2 < (int)param_1)) {
    *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0x44) = 1;
  }
  if (*(char *)(*(longlong *)_refptr_g_touchesPtr + 0x44) != '\0') {
    if ((char)(*_refptr_g_prevTouchesPtr)[1].nTouchCount == '\0') {
      *(void **)(*(longlong *)_refptr_g_touchesPtr + 0x58) =
           (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    }
    else if ((ulonglong)
             ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
             *(longlong *)&(*_refptr_g_prevTouchesPtr)[2].field_0x10) < 0x321) {
      *(undefined8 *)(*(longlong *)_refptr_g_touchesPtr + 0x58) =
           *(undefined8 *)&(*_refptr_g_prevTouchesPtr)[2].field_0x10;
    }
    else {
      *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0x45) = 1;
    }
  }
  if (*(char *)(*(longlong *)_refptr_g_touchesPtr + 0x45) == '\0') {
    if ((*(char *)((longlong)&(*_refptr_g_prevTouchesPtr)[1].nTouchCount + 1) != '\0') &&
       ((ulonglong)
        ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
        *(longlong *)&(*_refptr_g_prevTouchesPtr)[2].field_0x18) < 100)) {
      *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0x45) = 1;
      *(undefined8 *)(*(longlong *)_refptr_g_touchesPtr + 0x60) =
           *(undefined8 *)&(*_refptr_g_prevTouchesPtr)[2].field_0x18;
    }
  }
  else {
    *(void **)(*(longlong *)_refptr_g_touchesPtr + 0x60) =
         (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  }
  return;
}


// ============================================================================
// Function: Thenar_SideHandDetectByCoreTouch @ 6bb300a5
// ============================================================================

void Thenar_SideHandDetectByCoreTouch(byte param_1)

{
  byte bVar1;
  short sVar2;
  short sVar3;
  short sVar4;
  short sVar5;
  
  if (((*(int *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c)
        == 0x20) &&
      ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
       & 1) != 0)) &&
     (bVar1 = *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x287), bVar1 < 0x14)) {
    if (*(ushort *)
         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x250) < 0x25)
    {
      if (*(char *)(*(longlong *)_refptr_g_touchesPtr + 0x42) == '\0') {
        *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0x43) = 1;
        if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
          if (*(char *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)bVar1 * 0x478 + 0x193) < '\0') {
            *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x4c) = 4;
          }
          else {
            *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x4c) = 8;
          }
        }
        else if (*(char *)((longlong)*_refptr_g_prevTouchesPtr +
                          (longlong)(int)(uint)bVar1 * 0x478 + 0x193) < '\x01') {
          *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x4c) = 8;
        }
        else {
          *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x4c) = 4;
        }
      }
    }
    else {
      sVar2 = *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x170);
      sVar3 = *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                        (longlong)(int)(uint)bVar1 * 0x478 + 0x49a);
      sVar4 = *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x172);
      sVar5 = *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                        (longlong)(int)(uint)bVar1 * 0x478 + 0x49c);
      *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0x42) = 1;
      if ((int)(short)(sVar4 - sVar5) * (int)(short)(sVar2 - sVar3) < 1) {
        *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x4c) = 8;
      }
      else {
        *(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x4c) = 4;
      }
    }
  }
  return;
}


// ============================================================================
// Function: Thenar_FeatureExtraction @ 6bb30c47
// ============================================================================

void Thenar_FeatureExtraction(undefined8 param_1,undefined2 param_2)

{
  short sVar1;
  undefined4 local_38;
  undefined2 local_28;
  undefined2 local_18;
  
                    /* 0xf0c47  2457  Thenar_FeatureExtraction */
  Touch_ThenarFeatureReset();
  local_18 = CONCAT11(_refptr_g_tsaPrmtFlash->bCols,1);
  local_28._0_1_ = _refptr_g_tsaPrmtFlash->bCols;
  local_28._1_1_ = _refptr_g_tsaPrmtFlash->bRows;
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass != _refptr_g_tsaPrmtFlash->field_0x18) {
    local_18 = CONCAT11(1,_refptr_g_tsaPrmtFlash->bCols);
    local_28 = CONCAT11(_refptr_g_tsaPrmtFlash->bCols,_refptr_g_tsaPrmtFlash->bRows);
  }
  for (sVar1 = 0; sVar1 < (short)(local_28 & 0xff); sVar1 = (local_28 & 0xff) + sVar1 + -1) {
    local_38 = CONCAT22(sVar1,sVar1);
    Thenar_FeatureExtractionUnit(param_1,param_2,local_38,local_18,local_28);
  }
  return;
}


// ============================================================================
// Function: Thenar_UpdateDetectFeature @ 6bb30f99
// ============================================================================

undefined8 * Thenar_UpdateDetectFeature(undefined8 *param_1)

{
  ushort uVar1;
  undefined8 local_28;
  undefined4 local_20;
  ushort local_1c;
  ushort local_e;
  ushort local_c;
  ushort local_a;
  
  local_28 = 0;
  local_20 = 0xffff0000;
  local_1c = 0xffff;
  local_a = 0xffff;
  local_c = 0xffff;
  for (local_e = 0; local_e < *(byte *)(*(longlong *)_refptr_g_touchesPtr + 0x130);
      local_e = local_e + 1) {
    if ((short)((short)((ulonglong)
                        ((longlong)(*(short *)(_refptr_g_tsaPrmtDynamic + 0x44) * 5) * 0x2aaaaaab)
                       >> 0x20) - (*(short *)(_refptr_g_tsaPrmtDynamic + 0x44) >> 0xf)) <=
        *(short *)((longlong)(int)(uint)local_e * 10 + *(longlong *)_refptr_g_touchesPtr + 0x70)) {
      if (*(char *)((longlong)(int)(uint)local_e * 10 + *(longlong *)_refptr_g_touchesPtr + 0x6e) ==
          '\0') {
        if (local_28._4_2_ <
            *(ushort *)
             ((longlong)(int)(uint)local_e * 10 + *(longlong *)_refptr_g_touchesPtr + 0x6a)) {
          local_28 = CONCAT26(local_e,CONCAT24(*(undefined2 *)
                                                ((longlong)(int)(uint)local_e * 10 +
                                                 *(longlong *)_refptr_g_touchesPtr + 0x6a),
                                               (undefined4)local_28));
        }
      }
      else if ((ushort)local_28 <
               *(ushort *)
                ((longlong)(int)(uint)local_e * 10 + *(longlong *)_refptr_g_touchesPtr + 0x6a)) {
        local_28._0_4_ =
             CONCAT22(local_e,*(undefined2 *)
                               ((longlong)(int)(uint)local_e * 10 +
                                *(longlong *)_refptr_g_touchesPtr + 0x6a));
      }
      if (*(ushort *)((longlong)(int)(uint)local_e * 10 + *(longlong *)_refptr_g_touchesPtr + 0x6a)
          < 6) {
        if (*(char *)((longlong)(int)(uint)local_e * 10 + *(longlong *)_refptr_g_touchesPtr + 0x6e)
            == '\0') {
          local_20._0_2_ = CONCAT11(local_20._1_1_ + '\x01',(char)local_20);
        }
        else {
          local_20 = CONCAT31(local_20._1_3_,(char)local_20 + '\x01');
        }
      }
      if (*(char *)((longlong)(int)(uint)local_e * 10 + *(longlong *)_refptr_g_touchesPtr + 0x6e) ==
          '\0') {
        if (local_c == 0xffff) {
          local_c = local_e;
        }
        else {
          uVar1 = (*(short *)((longlong)(int)(short)local_c * 10 + *(longlong *)_refptr_g_touchesPtr
                             + 0x68) -
                  *(short *)((longlong)(int)(short)local_c * 10 + *(longlong *)_refptr_g_touchesPtr
                            + 0x6a)) -
                  *(short *)((longlong)(int)(uint)local_e * 10 + *(longlong *)_refptr_g_touchesPtr +
                            0x68);
          if (local_1c <= uVar1) {
            uVar1 = local_1c;
          }
          local_1c = uVar1;
        }
      }
      else if (local_a == 0xffff) {
        local_a = local_e;
      }
      else {
        uVar1 = (*(short *)((longlong)(int)(short)local_a * 10 + *(longlong *)_refptr_g_touchesPtr +
                           0x68) -
                *(short *)((longlong)(int)(short)local_a * 10 + *(longlong *)_refptr_g_touchesPtr +
                          0x6a)) -
                *(short *)((longlong)(int)(uint)local_e * 10 + *(longlong *)_refptr_g_touchesPtr +
                          0x68);
        if (local_20._2_2_ <= uVar1) {
          uVar1 = local_20._2_2_;
        }
        local_20 = CONCAT22(uVar1,(undefined2)local_20);
      }
    }
  }
  *param_1 = local_28;
  *(undefined4 *)(param_1 + 1) = local_20;
  *(ushort *)((longlong)param_1 + 0xc) = local_1c;
  return param_1;
}


// ============================================================================
// Function: Thenar_Init @ 6bb2fce0
// ============================================================================

void Thenar_Init(void)

{
  ushort local_a;
  
                    /* 0xefce0  2458  Thenar_Init */
  for (local_a = 0; local_a < 2; local_a = local_a + 1) {
    (&g_sideHandEnterCnt)[(int)(uint)local_a] = 0xf;
    *(undefined4 *)(&g_sideHandExitCnt + (longlong)(int)(uint)local_a * 4) = 0xf;
  }
  return;
}


// ============================================================================
// Function: Thenar_FeatureExtractionUnit @ 6bb306a0
// ============================================================================

void Thenar_FeatureExtractionUnit
               (longlong param_1,ushort param_2,short param_3,ushort param_4,undefined2 param_5)

{
  short sVar1;
  short sVar2;
  ushort uVar3;
  bool bVar4;
  undefined4 local_res18;
  short local_18;
  ushort local_14;
  short local_12;
  int local_10;
  byte local_b;
  ushort local_a;
  
  local_a = (param_4 >> 8) * (param_5._1_1_ - 1) + param_3 * (param_4 & 0xff);
  local_b = *(byte *)(*(longlong *)_refptr_g_touchesPtr + 0x130);
  if ((local_a < param_2) && (local_b < 0x14)) {
    local_10 = 0;
    local_12 = 0;
    local_14 = 0;
    local_18 = 0;
    sVar1 = *(short *)(_refptr_g_tsaPrmtDynamic + 0x44) / 2;
    bVar4 = (int)param_3 != (byte)param_5 - 1;
    local_res18 = CONCAT22(param_5._1_1_ - 1,param_3);
    while (-1 < local_res18) {
      if (0x13 < local_b) {
        local_b = local_b - 1;
        break;
      }
      if (local_10 == 1) {
        if (*(short *)(param_1 + (ulonglong)local_a * 2) < sVar1) {
          local_10 = 0;
          Thenar_FeatureUnitPostUpdate(local_b,local_12,local_14,(int)local_18);
          local_18 = 0;
          local_12 = 0;
          local_14 = 0;
        }
        else {
          local_12 = local_12 + 1;
          uVar3 = Thenar_FeatureUpdateEdgeWidthX(param_1,param_2,local_res18,param_4,param_5);
          if (uVar3 <= local_14) {
            uVar3 = local_14;
          }
          sVar2 = *(short *)(param_1 + (ulonglong)local_a * 2);
          if (local_18 <= sVar2) {
            local_18 = sVar2;
          }
          Thenar_FeatureUnitPostUpdate(local_b,local_12,uVar3,(int)local_18);
          sVar2 = Thenar_FeatureGradient(param_1,param_2,local_res18,param_4,param_5);
          if (sVar2 < -1000) {
            local_10 = 2;
          }
          else {
            local_10 = 1;
          }
          local_14 = uVar3;
        }
      }
      else if (local_10 == 0) {
        if (sVar1 < *(short *)(param_1 + (ulonglong)local_a * 2)) {
          local_10 = 1;
          local_b = local_b + 1;
          local_12 = 1;
          uVar3 = Thenar_FeatureUpdateEdgeWidthX(param_1,param_2,local_res18,param_4,param_5);
          if (uVar3 <= local_14) {
            uVar3 = local_14;
          }
          local_18 = *(short *)(param_1 + (ulonglong)local_a * 2);
          *(short *)((longlong)(int)(local_b - 1) * 10 + *(longlong *)_refptr_g_touchesPtr + 0x68) =
               local_res18._2_2_;
          *(bool *)((longlong)(int)(local_b - 1) * 10 + *(longlong *)_refptr_g_touchesPtr + 0x6e) =
               bVar4;
          local_14 = uVar3;
        }
      }
      else if (local_10 == 2) {
        if (*(short *)(param_1 + (ulonglong)local_a * 2) < sVar1) {
          local_10 = 0;
          Thenar_FeatureUnitPostUpdate(local_b,local_12,local_14,(int)local_18);
          local_18 = 0;
          local_12 = 0;
          local_14 = 0;
        }
        else {
          sVar2 = Thenar_FeatureGradient(param_1,param_2,local_res18,param_4,param_5);
          if (sVar2 < 0x3e9) {
            local_12 = local_12 + 1;
            uVar3 = Thenar_FeatureUpdateEdgeWidthX(param_1,param_2,local_res18,param_4,param_5);
            if (uVar3 <= local_14) {
              uVar3 = local_14;
            }
            sVar2 = *(short *)(param_1 + (ulonglong)local_a * 2);
            if (local_18 <= sVar2) {
              local_18 = sVar2;
            }
            Thenar_FeatureUnitPostUpdate(local_b,local_12,uVar3,(int)local_18);
            local_14 = uVar3;
          }
          else {
            Thenar_FeatureUnitPostUpdate(local_b,local_12,local_14,(int)local_18);
            local_10 = 1;
            local_b = local_b + 1;
            local_12 = 1;
            local_14 = Thenar_FeatureUpdateEdgeWidthX(param_1,param_2,local_res18,param_4,param_5);
            if (local_14 == 0) {
              local_14 = 0;
            }
            local_18 = *(short *)(param_1 + (ulonglong)local_a * 2);
            *(short *)((longlong)(int)(local_b - 1) * 10 + *(longlong *)_refptr_g_touchesPtr + 0x68)
                 = local_res18._2_2_;
            *(bool *)((longlong)(int)(local_b - 1) * 10 + *(longlong *)_refptr_g_touchesPtr + 0x6e)
                 = bVar4;
          }
        }
      }
      local_a = local_a - (param_4 >> 8);
      local_res18 = CONCAT22(local_res18._2_2_ + -1,(undefined2)local_res18);
    }
    *(byte *)(*(longlong *)_refptr_g_touchesPtr + 0x130) = local_b;
  }
  return;
}


// ============================================================================
// Function: Thenar_FeatureGradient @ 6bb3032e
// ============================================================================

short Thenar_FeatureGradient
                (longlong param_1,ushort param_2,undefined4 param_3,ushort param_4,
                undefined8 param_5)

{
  short sVar1;
  short sVar2;
  ushort uVar3;
  short local_res18;
  short sStackX_1a;
  short local_28 [13];
  short local_e;
  short local_c;
  short local_a;
  
  local_a = 0;
  local_28[0] = 1;
  local_28[1] = 2;
  local_28[2] = 1;
  local_28[3] = 0;
  local_28[4] = 0;
  local_28[5] = 0;
  local_28[6] = 0xffff;
  local_28[7] = 0xfffe;
  local_28[8] = 0xffff;
  for (local_c = 0; local_c < 3; local_c = local_c + 1) {
    for (local_e = 0; local_e < 3; local_e = local_e + 1) {
      local_res18 = (short)param_3;
      sVar1 = local_c + local_res18 + -1;
      sStackX_1a = (short)((uint)param_3 >> 0x10);
      sVar2 = local_e + sStackX_1a + -1;
      uVar3 = sVar2 * (param_4 >> 8) + (param_4 & 0xff) * sVar1;
      if ((((-1 < sVar1) && (sVar1 < (short)(ushort)(byte)param_5)) && (-1 < sVar2)) &&
         ((sVar2 < (short)(ushort)param_5._1_1_ && (uVar3 < param_2)))) {
        local_a = local_a + *(short *)(param_1 + (ulonglong)uVar3 * 2) *
                            local_28[(longlong)(int)local_e * 3 + (longlong)(int)local_c];
      }
    }
  }
  return local_a;
}


// ============================================================================
// Function: Thenar_FeatureUpdateEdgeWidthX @ 6bb30495
// ============================================================================

short Thenar_FeatureUpdateEdgeWidthX
                (longlong param_1,ushort param_2,undefined4 param_3,ushort param_4,byte param_5)

{
  short local_res18;
  short sStackX_1a;
  byte bStackX_21;
  short local_10;
  short local_e;
  short local_c;
  short local_a;
  
  local_a = 0;
  local_res18 = (short)param_3;
  sStackX_1a = (short)((uint)param_3 >> 0x10);
  bStackX_21 = (byte)(param_4 >> 8);
  local_c = (ushort)bStackX_21 * sStackX_1a + local_res18 * (param_4 & 0xff);
  local_e = 0;
  while (((local_e < (short)(ushort)param_5 &&
          (local_c = local_c + (param_4 & 0xff), (int)local_c < (int)(uint)param_2)) &&
         (*(short *)&_refptr_g_tsaPrpt[1].field_0x18 < *(short *)(param_1 + (longlong)local_c * 2)))
        ) {
    local_a = local_a + 1;
    local_e = local_e + 1;
  }
  local_c = (ushort)bStackX_21 * sStackX_1a + local_res18 * (param_4 & 0xff);
  local_10 = 0;
  while (((local_10 < (short)(ushort)param_5 && (local_c = local_c - (param_4 & 0xff), -1 < local_c)
          ) && (*(short *)&_refptr_g_tsaPrpt[1].field_0x18 <
                *(short *)(param_1 + (longlong)local_c * 2)))) {
    local_a = local_a + 1;
    local_10 = local_10 + 1;
  }
  return local_a;
}


// ============================================================================
// Function: Thenar_FeatureUnitPostUpdate @ 6bb305f5
// ============================================================================

void Thenar_FeatureUnitPostUpdate
               (byte param_1,undefined2 param_2,undefined2 param_3,undefined2 param_4)

{
  *(undefined2 *)((longlong)(int)(param_1 - 1) * 10 + *(longlong *)_refptr_g_touchesPtr + 0x6a) =
       param_2;
  *(undefined2 *)((longlong)(int)(param_1 - 1) * 10 + *(longlong *)_refptr_g_touchesPtr + 0x6c) =
       param_3;
  *(undefined2 *)((longlong)(int)(param_1 - 1) * 10 + *(longlong *)_refptr_g_touchesPtr + 0x70) =
       param_4;
  return;
}


