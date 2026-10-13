// ============================================================================
// Function: TouchMode_Process @ 6ba78dde
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* Touch mode state machine: copies previous mode, walks priority table, runs mode handler pairs,
   and commits selected mode/result to g_tsaPrpt. */

void TouchMode_Process(void)

{
  uint dwPrevTouchMode;
  undefined4 dwPrevModeData;
  int nPrevModeResult;
  uint dwNextTouchMode;
  undefined4 dwNextModeData;
  int anModeResult [2];
  undefined **pModeHandlerPair;
  uint dwPriorityIndex;
  TSAPrpt *pTsaReport;
  
                    /* 0x38dde  2493  TouchMode_Process */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&DAT_6bb52a80,"TouchMode_Process");
  }
  pTsaReport = _refptr_g_tsaPrpt;
  nPrevModeResult = 0;
  dwPrevModeData = 0;
  pModeHandlerPair = (undefined **)0x0;
  *(undefined8 *)&_refptr_g_tsaPrpt[1].field_0x38 = *(undefined8 *)&_refptr_g_tsaPrpt[1].field_0x2c;
  *(undefined4 *)&pTsaReport[1].field_0x40 = *(undefined4 *)&pTsaReport[1].field_0x34;
  dwPrevTouchMode = *(uint *)&_refptr_g_tsaPrpt[1].field_0x38;
  if (3 < dwPrevTouchMode) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/TouchMode/TouchMode_Process.c",0xa4,
                      "prevModeUnit.mode < TOUCH_MODE_NUM");
    HardwareAnalyzer_SetSoftException();
  }
  TouchMode_SwitchPriorityProcess();
  if ((_refptr_g_tsaPrmtDynamic[0x80] & 8) != 0) {
    TouchMode_StylusFilter();
  }
  dwPriorityIndex = 0;
  do {
    if (3 < dwPriorityIndex) {
LAB_6ba78fbd:
      if (3 < dwPriorityIndex) {
        dwNextTouchMode = dwPrevTouchMode;
        anModeResult[0] = nPrevModeResult;
        dwNextModeData = dwPrevModeData;
      }
      *(uint *)&_refptr_g_tsaPrpt[1].field_0x2c = dwNextTouchMode;
      *(int *)&_refptr_g_tsaPrpt[1].field_0x34 = anModeResult[0];
      *(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x30 = dwNextModeData;
      return;
    }
    dwNextTouchMode =
         *(uint *)(&g_tmSwitchPriority +
                  ((ulonglong)dwPriorityIndex + (ulonglong)dwPrevTouchMode * 4) * 4);
    if (3 < dwNextTouchMode) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Alg/TouchMode/TouchMode_Process.c",0xaf,
                        "nextModeUnit.mode < TOUCH_MODE_NUM");
      HardwareAnalyzer_SetSoftException();
    }
    pModeHandlerPair = &g_tmHandlerTable + (ulonglong)dwNextTouchMode * 2;
    (*(code *)*pModeHandlerPair)(anModeResult,&dwNextModeData);
    if (dwNextTouchMode == dwPrevTouchMode) {
      nPrevModeResult = anModeResult[0];
      dwPrevModeData = dwNextModeData;
    }
    if (anModeResult[0] == 2) {
      if (dwNextTouchMode == dwPrevTouchMode) {
        (*(code *)pModeHandlerPair[1])(2,dwNextModeData);
        SWTimer_Stop(&g_tmSwitchTimer);
      }
      else {
        TouchMode_NewModeProcess(&dwNextTouchMode,&dwPrevTouchMode,pModeHandlerPair);
      }
      goto LAB_6ba78fbd;
    }
    dwPriorityIndex = dwPriorityIndex + 1;
  } while( true );
}


// ============================================================================
// Function: TouchMode_NewModeProcess @ 6ba78c16
// ============================================================================

void TouchMode_NewModeProcess(uint *param_1,uint *param_2,longlong param_3)

{
  undefined2 uVar1;
  int iVar2;
  
  if (param_2[2] < 2) {
    (**(code **)(param_3 + 8))(param_1[2],param_1[1]);
    if ((param_2[1] == 1) && (*param_1 != 0)) {
      uVar1 = 0;
    }
    else {
      uVar1 = *(undefined2 *)
               (&g_tmSwitchDebounce + ((ulonglong)*param_2 * 4 + (ulonglong)*param_1) * 2);
    }
    SWTimer_Start((*_refptr_g_tsaStaticPtr)->nFrameCounter,&g_tmSwitchTimer);
    iVar2 = SWTimer_IsExpired((*_refptr_g_tsaStaticPtr)->nFrameCounter,&g_tmSwitchTimer,uVar1);
    if (iVar2 == 1) {
      SWTimer_Stop(&g_tmSwitchTimer);
    }
    else {
      if (param_2[2] == 0) {
        param_1[2] = *(uint *)&_refptr_g_tsaPrpt[1].field_0x40;
        param_1[1] = *(uint *)&_refptr_g_tsaPrpt[1].field_0x3c;
      }
      else {
        param_1[2] = param_2[2];
        param_1[1] = param_2[1];
      }
      *param_1 = *param_2;
    }
  }
  else if (param_2[2] == 4) {
    (**(code **)(param_3 + 8))(param_1[2],param_1[1]);
    if ((uint)*(byte *)(*(longlong *)_refptr_g_touchesPtr + 3) +
        (uint)**(byte **)_refptr_g_touchesPtr == 0) {
      *param_1 = *param_2;
      param_1[2] = param_2[2];
      param_1[1] = param_2[1];
    }
    SWTimer_Stop(&g_tmSwitchTimer);
  }
  return;
}


// ============================================================================
// Function: TouchMode_SwitchPriorityProcess @ 6ba7900b
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void TouchMode_SwitchPriorityProcess(void)

{
  if (((_refptr_g_tsaPrmtDynamic[0x80] & 8) != 0) &&
     (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x10) != 0)) {
    if ((_refptr_g_tsaPrmtDynamic[0x81] & 8) == 0) {
      _DAT_6bb4f250 = 1;
      _DAT_6bb4f254 = 3;
      _DAT_6bb4f244 = 1;
      _DAT_6bb4f248 = 3;
    }
    else {
      _DAT_6bb4f250 = 3;
      _DAT_6bb4f254 = 1;
      _DAT_6bb4f244 = 3;
      _DAT_6bb4f248 = 1;
    }
  }
  return;
}


// ============================================================================
// Function: TouchMode_StylusFilter @ 6ba7a0ed
// ============================================================================

void TouchMode_StylusFilter(void)

{
  undefined1 uVar1;
  char cVar2;
  
                    /* 0x3a0ed  2494  TouchMode_StylusFilter */
  uVar1 = TouchMode_StylusFilterProcess();
  cVar2 = TouchMode_StylusGetCoreIndex();
  if (cVar2 != '\x14') {
    *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0xc) = uVar1;
  }
  return;
}


// ============================================================================
// Function: TouchMode_StylusGetCoreIndex @ 6ba79b0f
// ============================================================================

byte TouchMode_StylusGetCoreIndex(void)

{
  byte local_a;
  
                    /* 0x39b0f  2496  TouchMode_StylusGetCoreIndex */
  local_a = 0;
  while( true ) {
    if (**(byte **)_refptr_g_touchesPtr <= local_a) {
      return 0x14;
    }
    if (((*(uint *)((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208
                   ) & 8) == 0) &&
       ((*(uint *)((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x204)
        & 0xf000) == 0)) break;
    local_a = local_a + 1;
  }
  return local_a;
}


// ============================================================================
// Function: TouchMode_StylusFilterProcess @ 6ba79e26
// ============================================================================

byte TouchMode_StylusFilterProcess(void)

{
  char cVar1;
  bool bVar2;
  char cVar3;
  ushort uVar4;
  byte local_16;
  byte local_15;
  int local_14;
  ushort local_e;
  byte local_c;
  byte local_a;
  byte local_9;
  
                    /* 0x39e26  2495  TouchMode_StylusFilterProcess */
  local_9 = 0;
  local_a = 0;
  bVar2 = false;
  local_c = TouchMode_GetStylusNum();
  if (1 < local_c) {
    cVar3 = Peak_GetIdxBySig((int)*(short *)(*_refptr_g_peaksPtr + 2));
    local_e = 0;
    local_14 = -0x80000000;
    for (local_15 = 0; local_15 < **(byte **)_refptr_g_touchesPtr; local_15 = local_15 + 1) {
      if ((*(uint *)((longlong)(int)(uint)local_15 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x208) & 8) == 0) {
        if ((*(uint *)((longlong)(int)(uint)local_15 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x204) & 0xf000) == 0) {
          if (local_14 <
              *(int *)((longlong)(int)(uint)local_15 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x1d4)) {
            local_14 = *(int *)((longlong)(int)(uint)local_15 * 0x168 +
                                *(longlong *)_refptr_g_touchesPtr + 0x1d4);
            local_9 = local_15;
          }
          cVar1 = *(char *)((longlong)(int)(uint)local_15 * 0x168 +
                            *(longlong *)_refptr_g_touchesPtr + 0x23c);
          if (cVar1 == cVar3) {
            bVar2 = true;
          }
          uVar4 = Peak_GetSquareDistance(cVar3,cVar1);
          if (local_e < uVar4) {
            local_a = local_15;
            local_e = uVar4;
          }
        }
        else {
          *(uint *)((longlong)(int)(uint)local_15 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                   0x208) =
               *(uint *)((longlong)(int)(uint)local_15 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x208) | 8;
          local_c = local_c - 1;
          if (local_c == 1) break;
        }
      }
    }
    if (1 < local_c) {
      (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
           (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 0x200;
      if (!bVar2) {
        local_9 = local_a;
      }
      for (local_16 = 0; local_16 < **(byte **)_refptr_g_touchesPtr; local_16 = local_16 + 1) {
        if (local_16 != local_9) {
          *(uint *)((longlong)(int)(uint)local_16 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                   0x208) =
               *(uint *)((longlong)(int)(uint)local_16 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x208) | 8;
          local_c = local_c - 1;
        }
      }
    }
  }
  return local_c;
}


// ============================================================================
// Function: TouchMode_Init @ 6ba78b60
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void TouchMode_Init(void)

{
                    /* 0x38b60  2492  TouchMode_Init */
  *(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x2c = 0;
  *(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x34 = 0;
  *(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x30 = 0;
  _refptr_g_tsaPrpt[1].field_0x28 = 0;
  SWTimer_Stop(&g_tmSwitchTimer);
  _DAT_6bb4f202 = 0;
  if (_refptr_g_tsaPrmtFlash->field_0x44 == '\0') {
    _DAT_6bb4f208 = 0x3c;
  }
  else {
    _DAT_6bb4f208 = 0;
  }
  _DAT_6bb4f204 = 0x14;
  _DAT_6bb4f210 = 200;
  _DAT_6bb4f212 = 0;
  (*_refptr_g_tsaStaticPtr)[1].field_0xc = 0;
  return;
}


// ============================================================================
// Function: TouchMode_GetStylusNum @ 6ba79c23
// ============================================================================

char TouchMode_GetStylusNum(void)

{
  char cVar1;
  byte local_a;
  char local_9;
  
                    /* 0x39c23  2491  TouchMode_GetStylusNum */
  local_9 = '\0';
  for (local_a = 0; local_a < **(byte **)_refptr_g_touchesPtr; local_a = local_a + 1) {
    if ((*(byte *)((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1ac)
         < 3) && (*(byte *)((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr
                           + 0x1ad) < 3)) {
      if (*(short *)&_refptr_g_tsaPrmtFlash->field_0x5d8 * 3 <
          *(int *)((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1d4)
         ) {
        *(uint *)((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208)
             = *(uint *)((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x208) | 8;
      }
      else {
        cVar1 = TouchMode_StylusIsClearanceAreaSatisfied(local_a);
        if (cVar1 == '\0') {
          *(uint *)((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208
                   ) = *(uint *)((longlong)(int)(uint)local_a * 0x168 +
                                 *(longlong *)_refptr_g_touchesPtr + 0x208) | 8;
        }
        else if ((*(uint *)((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr
                           + 0x208) & 8) == 0) {
          local_9 = local_9 + '\x01';
        }
      }
    }
    else {
      *(uint *)((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208) =
           *(uint *)((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x208) | 8;
    }
  }
  return local_9;
}


// ============================================================================
// Function: TouchMode_StylusIsClearanceAreaSatisfied @ 6ba79930
// ============================================================================

undefined1 TouchMode_StylusIsClearanceAreaSatisfied(byte param_1)

{
  uchar uVar1;
  uchar uVar2;
  char cVar3;
  uint uVar4;
  byte local_c;
  undefined1 local_b;
  byte local_a;
  byte local_9;
  
                    /* 0x39930  2497  TouchMode_StylusIsClearanceAreaSatisfied */
  uVar1 = (*_refptr_g_peaksPtr)
          [(longlong)
           (int)(uint)*(byte *)((longlong)(int)(uint)param_1 * 0x168 +
                                *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x15].bPeakCount
  ;
  uVar2 = (*_refptr_g_peaksPtr)
          [(longlong)
           (int)(uint)*(byte *)((longlong)(int)(uint)param_1 * 0x168 +
                                *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x16].bPeakCount
  ;
  local_b = 1;
  if (((*_refptr_g_prevTouchesPtr)->field_0xc == '\0') ||
     (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x1000) == 0)) {
    local_c = 3;
  }
  else {
    local_c = 2;
  }
  for (local_a = uVar2 - local_c; (int)(char)local_a <= (int)((uint)local_c + (int)(char)uVar2);
      local_a = local_a + 1) {
    if (local_a < _refptr_g_tsaPrmtFlash->bRows) {
      for (local_9 = uVar1 - local_c; (int)(char)local_9 <= (int)((uint)local_c + (int)(char)uVar1);
          local_9 = local_9 + 1) {
        if ((local_9 < _refptr_g_tsaPrmtFlash->bCols) &&
           (((uVar4 = (int)(char)local_9 - (int)(char)uVar1 >> 0x1f,
             ((int)(char)local_9 - (int)(char)uVar1 ^ uVar4) - uVar4 == (uint)local_c ||
             (uVar4 = (int)(char)local_a - (int)(char)uVar2 >> 0x1f,
             ((int)(char)local_a - (int)(char)uVar2 ^ uVar4) - uVar4 == (uint)local_c)) &&
            (cVar3 = Sensor_IsInDifferentTouchZone(local_9,local_a,uVar1,uVar2), cVar3 != '\0')))) {
          local_b = 0;
        }
      }
    }
  }
  return local_b;
}


