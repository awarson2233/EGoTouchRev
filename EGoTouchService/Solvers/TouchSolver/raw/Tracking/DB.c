// ============================================================================
// Function: DB_TouchDownNum @ 6ba5681c
// ============================================================================

/* WARNING: Removing unreachable block (ram,0x6ba56862) */

ushort DB_TouchDownNum(byte param_1,byte param_2)

{
  char cVar1;
  undefined1 uVar2;
  undefined1 uVar3;
  undefined2 uVar4;
  ushort uVar5;
  short sVar6;
  ushort local_a;
  
                    /* 0x1681c  527  DB_TouchDownNum */
  local_a = (ushort)(1 < (int)((uint)(*_refptr_g_peaksPtr)->bPeakCount -
                              (uint)**(byte **)_refptr_g_prevPeaksPtr));
  if ((*(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1ec) &
      0x80) != 0) {
    uVar5 = 2;
    if (1 < local_a) {
      uVar5 = local_a;
    }
    local_a = uVar5;
  }
  if (((*(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1ec)
       & 8) != 0) && (local_a == 0)) {
    local_a = 1;
  }
  if ((*(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1ec) &
      0x200) != 0) {
    if (*(int *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1d8) <
        1) {
      local_a = 0xffff;
    }
    else {
      uVar5 = 3;
      if (2 < local_a) {
        uVar5 = local_a;
      }
      local_a = uVar5;
    }
  }
  if ((*_refptr_g_tsaStaticPtr)->bField_0x111 != '\0') {
    uVar5 = 2;
    if (1 < local_a) {
      uVar5 = local_a;
    }
    local_a = uVar5;
  }
  if (*(char *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x2a4) !=
      '\0') {
    uVar5 = 3;
    if (2 < local_a) {
      uVar5 = local_a;
    }
    local_a = uVar5;
  }
  if ((((((*(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1ec) & 0x40) != 0) ||
        (cVar1 = DB_TouchDownTholdCheck(param_1,param_2), cVar1 == '\0')) ||
       ((*(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0)
        & 0x400) != 0)) ||
      ((*(int *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) <
        0 || ((*(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x1f0) & 0x40000000) != 0)))) ||
     (((*(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0)
       & 0x1000000) != 0 ||
      ((*(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0)
       & 0x2000000) != 0)))) {
    local_a = 0xffff;
  }
  uVar4 = DB_GetNumBySignalSum(param_1,param_2,local_a);
  uVar5 = DB_GetNumBySideTouch(param_2,uVar4);
  local_a = uVar5;
  if ((*(int *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1e0) ==
       2) && (local_a = 3, 2 < uVar5)) {
    local_a = uVar5;
  }
  if (*(int *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1e0) ==
      3) {
    uVar5 = 3;
    if (2 < local_a) {
      uVar5 = local_a;
    }
    local_a = uVar5;
  }
  if ((*(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) &
      0x800) != 0) {
    uVar5 = 2;
    if (1 < local_a) {
      uVar5 = local_a;
    }
    local_a = uVar5;
  }
  if (((*(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
       & 0x2000) != 0) ||
     ((*(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) &
      0x400000) != 0)) {
    local_a = 0xffff;
  }
  if (local_a == 0xffff) {
    *(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0) =
         *(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0)
         | 0x10;
  }
  else if ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)param_1 * 0x478 + 0x1f0) & 0x10) != 0) {
    *(undefined2 *)
     ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x4e4) = 0;
  }
  DB_FastClickProcess(param_1,param_2);
  if (local_a < *(ushort *)
                 ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x238
                 )) {
    local_a = *(ushort *)
               ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x238);
  }
  if (0xe < (*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal) {
    local_a = (ushort)((local_a + 1) / 2);
  }
  if (_refptr_g_tsaPrmtFlash->field_0x818 != '\0') {
    uVar2 = (undefined1)
            ((ushort)*(undefined2 *)
                      ((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x170) >> 8);
    uVar3 = (undefined1)
            ((ushort)*(undefined2 *)
                      ((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x172) >> 8);
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      sVar6 = CMF_GetNoiseValue(uVar2,uVar3);
      StrPrintf_Process(0,"\nposition(%d , %d) current noise = %d\n",uVar2,uVar3,(int)sVar6);
    }
    sVar6 = CMF_GetNoiseValue(uVar2,uVar3);
    if ((500 < sVar6) || (sVar6 = CMF_GetNoiseValue(uVar2,uVar3), sVar6 < -400)) {
      if (local_a <= (byte)_refptr_g_tsaPrmtFlash->field_0x818) {
        local_a = (ushort)(byte)_refptr_g_tsaPrmtFlash->field_0x818;
      }
    }
  }
  return local_a;
}


// ============================================================================
// Function: DB_LiftOffNum @ 6ba55e79
// ============================================================================

ushort DB_LiftOffNum(byte param_1)

{
  ushort local_a;
  
                    /* 0x15e79  526  DB_LiftOffNum */
  local_a = 0;
  if ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x1f8)
      & 8) == 0) {
    local_a = 0;
  }
  else if ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)param_1 * 0x478 + 0x1f8) & 0x80) == 0) {
    if (*(int *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x1e0)
        == 3) {
      local_a = 3;
    }
    if (((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_1 * 0x478 + 0x1f0) & 0x10000000) != 0) &&
       (local_a < 2)) {
      local_a = 2;
    }
    if ((2 < (*_refptr_g_tsaStaticPtr)->bField_0x111) && (local_a < 2)) {
      local_a = 2;
    }
  }
  else {
    local_a = 0;
  }
  return local_a;
}


// ============================================================================
// Function: DB_TouchDownTholdCheck @ 6ba55f8d
// ============================================================================

bool DB_TouchDownTholdCheck(byte param_1,byte param_2)

{
  short sVar1;
  char cVar2;
  short local_c;
  bool local_9;
  
  local_c = *(short *)(_refptr_g_tsaPrmtDynamic + 0xc);
  if (((*_refptr_g_tsaStaticPtr)->dwProcessingState & 0x10) != 0) {
    cVar2 = TSA_IsGameScenario();
    if ((cVar2 == '\0') &&
       (((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_1 * 0x478 + 0x1fc) & 2) != 0 ||
        ((*(short *)(*_refptr_g_peaksPtr +
                    (longlong)
                    (int)(uint)*(byte *)((longlong)(int)(uint)param_2 * 0x168 +
                                         *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x18) <
          local_c &&
         ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_1 * 0x478 + 500) & 0x20) != 0)))))) {
      local_c = (short)((local_c * 3) / 2);
      *(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1fc) =
           *(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1fc) | 2;
    }
  }
  cVar2 = TouchThold_ToeUseSpecialThold();
  if ((cVar2 != '\0') &&
     ((((*_refptr_g_peaksPtr)
        [(longlong)
         (int)(uint)*(byte *)((longlong)(int)(uint)param_2 * 0x168 +
                              *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x15].bPeakCount
        == '\x01' ||
       ((uint)(*_refptr_g_peaksPtr)
              [(longlong)
               (int)(uint)*(byte *)((longlong)(int)(uint)param_2 * 0x168 +
                                    *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x15].
              bPeakCount == _refptr_g_tsaPrmtFlash->bCols - 2)) ||
      ((uint)(*_refptr_g_peaksPtr)
             [(longlong)
              (int)(uint)*(byte *)((longlong)(int)(uint)param_2 * 0x168 +
                                   *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x16].
             bPeakCount == _refptr_g_tsaPrmtFlash->bRows - 1)))) {
    local_c = *(short *)(_refptr_g_tsaPrmtDynamic + 0xe);
  }
  sVar1 = *(short *)(*_refptr_g_peaksPtr +
                    (longlong)
                    (int)(uint)*(byte *)((longlong)(int)(uint)param_2 * 0x168 +
                                         *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x18);
  if (sVar1 < local_c) {
    *(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 500) =
         *(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 500) |
         0x20;
  }
  local_9 = sVar1 >= local_c;
  return local_9;
}


// ============================================================================
// Function: DB_FastClickProcess @ 6ba562d4
// ============================================================================

void DB_FastClickProcess(byte param_1,byte param_2)

{
  if ((*(ushort *)
        ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x4e4) < 4) &&
     (*(ushort *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x24c)
      < 0x1f)) {
    if (((*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_1 * 0x478 + 0x1a6) < 0xb) &&
        (*(short *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1a6
                   ) < 0xb)) &&
       (((*(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0
                   ) & 0x800) == 0 ||
        ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_1 * 0x478 + 0x1f0) & 0x800) == 0)))) {
      if (((*(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1f0) & 2) != 0) &&
         (*(ushort *)
           ((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x188) < 0x10
         )) {
        *(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
             = *(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x1f8) | 0x40000;
      }
      if (((*(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1f0) & 1) != 0) &&
         (*(ushort *)
           ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x188) <
          0x10)) {
        *(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
             = *(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x1f8) | 0x40000;
      }
    }
  }
  else if (5 < *(ushort *)
                ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x4e4)
          ) {
    *(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) =
         *(uint *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
         & 0xfffbffff;
  }
  return;
}


// ============================================================================
// Function: DB_GetNumBySignalSum @ 6ba565b2
// ============================================================================

ushort DB_GetNumBySignalSum(byte param_1,byte param_2,ushort param_3)

{
  char cVar1;
  int iVar2;
  ushort local_res18;
  
                    /* 0x165b2  524  DB_GetNumBySignalSum */
  iVar2 = (uint)(byte)_refptr_g_tsaPrmtDynamic[6] * *(int *)(_refptr_g_tsaPrmtDynamic + 0x14);
  if (iVar2 < 0) {
    iVar2 = iVar2 + 0xf;
  }
  if (*(int *)(*_refptr_g_peaksPtr +
              (longlong)
              (int)(uint)*(byte *)((longlong)(int)(uint)param_2 * 0x168 +
                                   *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x20) <
      iVar2 >> 4) {
    local_res18 = param_3;
    if (*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                  (longlong)(int)(uint)param_1 * 0x478 + 0x156) == 0) {
      if (param_3 == 0) {
        local_res18 = 1;
      }
    }
    else {
      cVar1 = Touch_ValidComparison(param_2,param_1);
      if (cVar1 == '\0') {
        if (param_3 == 0) {
          local_res18 = 1;
        }
      }
      else {
        iVar2 = *(int *)((longlong)*_refptr_g_prevTouchesPtr +
                        (longlong)(int)(uint)param_1 * 0x478 + 0x1cc);
        if (iVar2 < 0) {
          iVar2 = iVar2 + 3;
        }
        if ((iVar2 >> 2 <
             *(int *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1cc) -
             *(int *)((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)param_1 * 0x478 + 0x1cc)) && (param_3 < 3)) {
          local_res18 = 3;
        }
      }
    }
    param_3 = local_res18;
  }
  return param_3;
}


// ============================================================================
// Function: DB_GetNumBySideTouch @ 6ba5674c
// ============================================================================

ushort DB_GetNumBySideTouch(byte param_1,ushort param_2)

{
  ushort local_res10;
  
                    /* 0x1674c  523  DB_GetNumBySideTouch */
  local_res10 = param_2;
  if (((*_refptr_g_tsaStaticPtr)->dwProcessingState & 0x10) == 0) {
    if ((0xe < *(ushort *)
                ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x188))
       && (param_2 < 2)) {
      local_res10 = 2;
    }
    if (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f0
                   ) & 4) == 0) && (local_res10 == 0)) {
      local_res10 = 1;
    }
  }
  else if ((0x1d < *(ushort *)
                    ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x188)) && (param_2 < 2)) {
    local_res10 = 2;
  }
  return local_res10;
}


// ============================================================================
// Function: DB_LOSize @ 6ba55e20
// ============================================================================

ushort DB_LOSize(void)

{
  ushort local_a;
  
                    /* 0x15e20  525  DB_LOSize */
  local_a = (ushort)(byte)_refptr_g_tsaPrmtFlash->field_0x836 *
            (ushort)(byte)_refptr_g_tsaPrmtFlash->field_0x836;
  if ((*_refptr_g_prevTouchesPtr)->field_0x3 != '\0') {
    local_a = (ushort)(local_a * 3) >> 2;
  }
  return local_a;
}


