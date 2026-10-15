// ============================================================================
// Function: EventAnalyzer_Init @ 6badef1e
// ============================================================================

void EventAnalyzer_Init(void)

{
  int iVar1;
  
                    /* 0x9ef1e  569  EventAnalyzer_Init */
  iVar1 = memset_s(&g_internalTouches,0x1d60,0,0x1d60);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/EventAnalyzer/EventAnalyzer_Process.c",0x3f,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: EventAnalyzer_GetCurValue @ 6badef00
// ============================================================================

undefined4 EventAnalyzer_GetCurValue(void)

{
                    /* 0x9ef00  564  EventAnalyzer_GetCurValue */
  return g_eventAnalyzerCurrVal;
}


// ============================================================================
// Function: EventAnalyzer_Process @ 6badfe1b
// ============================================================================

undefined8
EventAnalyzer_Process
          (uint param_1,undefined4 param_2,undefined4 param_3,undefined2 param_4,undefined2 param_5,
          undefined2 param_6,undefined2 param_7,undefined8 param_8,undefined8 param_9,
          undefined4 param_10,undefined4 param_11)

{
  char cVar1;
  undefined8 uVar2;
  bool bVar3;
  
                    /* 0x9fe1b  573  EventAnalyzer_Process */
  g_eventAnalyzer = param_1;
  DAT_6bc77604 = param_2;
  DAT_6bc77608 = param_3;
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    DAT_6bc7760c = TSAOut_RevertDim1Pos(param_4);
    DAT_6bc7760e = TSAOut_RevertDim1Pos(param_5);
    DAT_6bc77610 = TSAOut_RevertDim2Pos(param_6);
    DAT_6bc77612 = TSAOut_RevertDim2Pos(param_7);
  }
  else {
    DAT_6bc7760c = TSAOut_RevertDim1Pos(param_6);
    DAT_6bc7760e = TSAOut_RevertDim1Pos(param_7);
    DAT_6bc77610 = TSAOut_RevertDim2Pos(param_4);
    DAT_6bc77612 = TSAOut_RevertDim2Pos(param_5);
  }
  DAT_6bc77618 = param_8;
  DAT_6bc77620 = param_9;
  DAT_6bc77628 = param_10;
  DAT_6bc7762c = param_11;
  bVar3 = (g_eventAnalyzer & 1) != 0;
  if (((g_eventAnalyzer & 4) != 0) && (cVar1 = EventAnalyzer_IsInArea(), cVar1 == '\0')) {
    bVar3 = false;
  }
  if (((g_eventAnalyzer & 2) != 0) && (cVar1 = EventAnalyzer_IsInTime(), cVar1 == '\0')) {
    bVar3 = false;
  }
  if (bVar3) {
    uVar2 = EventAnalyzer_CheckEvent();
  }
  else {
    uVar2 = 0;
  }
  return uVar2;
}


// ============================================================================
// Function: EventAnalyzer_IsInArea @ 6badf6fb
// ============================================================================

undefined8 EventAnalyzer_IsInArea(void)

{
  undefined8 uVar1;
  ushort local_c;
  ushort local_a;
  
  if ((byte)_refptr_g_touchesGrid[5] < DAT_6bc77ac5) {
    local_a = *(ushort *)(&DAT_6bc77c30 + (longlong)(int)(uint)(byte)(DAT_6bc77ac5 - 1) * 0x168);
    local_c = *(ushort *)(&DAT_6bc77c32 + (longlong)(int)(uint)(byte)(DAT_6bc77ac5 - 1) * 0x168);
  }
  else {
    local_a = *(ushort *)
               (_refptr_g_touchesGrid +
               (longlong)(int)(uint)(byte)(_refptr_g_touchesGrid[5] - 1) * 0x168 + 0x170);
    local_c = *(ushort *)
               (_refptr_g_touchesGrid +
               (longlong)(int)(uint)(byte)(_refptr_g_touchesGrid[5] - 1) * 0x168 + 0x172);
  }
  if (local_a < DAT_6bc7760c) {
    uVar1 = 0;
  }
  else if (DAT_6bc7760e < local_a) {
    uVar1 = 0;
  }
  else if (local_c < DAT_6bc77610) {
    uVar1 = 0;
  }
  else if (DAT_6bc77612 < local_c) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: EventAnalyzer_IsInTime @ 6badf83e
// ============================================================================

undefined1 EventAnalyzer_IsInTime(void)

{
  longlong lVar1;
  int iVar2;
  ulonglong uVar3;
  undefined1 local_9;
  
  local_9 = 0;
  lVar1 = *(longlong *)(_refptr_g_tsaStaticGrid + 0xe0);
  iVar2 = EventAnalyzer_GetEventValue(DAT_6bc77608,1);
  if ((DAT_6bc77628 == iVar2) && (flagTimerStarted_10575 == '\0')) {
    flagTimerStarted_10575 = '\x01';
    startTime_10576 = lVar1;
  }
  if (((flagTimerStarted_10575 != '\0') && (uVar3 = lVar1 - startTime_10576, DAT_6bc77618 <= uVar3))
     && (uVar3 <= DAT_6bc77620)) {
    local_9 = 1;
  }
  if ((DAT_6bc7762c == iVar2) && (flagTimerStarted_10575 != '\0')) {
    flagTimerStarted_10575 = '\0';
  }
  return local_9;
}


// ============================================================================
// Function: EventAnalyzer_CheckEvent @ 6badf912
// ============================================================================

undefined1 EventAnalyzer_CheckEvent(void)

{
  uint uVar1;
  undefined1 local_9;
  
  local_9 = 0;
  uVar1 = EventAnalyzer_GetEventValue(DAT_6bc77608,1);
  g_eventAnalyzerPrevVal = EventAnalyzer_GetEventValue(DAT_6bc77608,0);
  g_eventAnalyzerCurrVal = uVar1;
  if ((uVar1 != 0x5a5a5a5a) && (uVar1 != 0x5a5a5a5a)) {
    switch(DAT_6bc77604) {
    case 0:
      if ((DAT_6bc7762c == uVar1) && (DAT_6bc77628 == g_eventAnalyzerPrevVal)) {
        local_9 = 1;
      }
      break;
    case 1:
      if (((int)uVar1 <= (int)DAT_6bc7762c) && ((int)DAT_6bc77628 <= (int)uVar1)) {
        local_9 = 1;
      }
      break;
    case 2:
      if (((int)DAT_6bc7762c < (int)uVar1) || ((int)uVar1 < (int)DAT_6bc77628)) {
        local_9 = 1;
      }
      break;
    case 3:
      if (uVar1 != g_eventAnalyzerPrevVal) {
        local_9 = 1;
      }
      break;
    case 4:
      if (DAT_6bc77628 != uVar1) {
        local_9 = 1;
      }
      break;
    case 5:
      if (DAT_6bc77628 == uVar1) {
        local_9 = 1;
      }
      break;
    case 6:
      if ((int)DAT_6bc77628 <= (int)uVar1) {
        local_9 = 1;
      }
      break;
    case 7:
      if ((int)DAT_6bc77628 < (int)uVar1) {
        local_9 = 1;
      }
      break;
    case 8:
      if ((int)uVar1 <= (int)DAT_6bc77628) {
        local_9 = 1;
      }
      break;
    case 9:
      if ((int)uVar1 < (int)DAT_6bc77628) {
        local_9 = 1;
      }
      break;
    case 10:
      if ((uVar1 & DAT_6bc77628) != 0) {
        local_9 = 1;
      }
    }
  }
  return local_9;
}


// ============================================================================
// Function: EventAnalyzer_GetEventValue @ 6badf1c7
// ============================================================================

uint EventAnalyzer_GetEventValue(undefined4 param_1,int param_2)

{
  byte bVar1;
  longlong lVar2;
  uint uVar3;
  undefined8 *local_50;
  undefined *local_48;
  undefined8 *local_40;
  TSAPrpt *local_38;
  undefined *local_30;
  byte *local_28;
  byte *local_20;
  undefined1 *local_18;
  uint local_c;
  
  if (param_2 == 0) {
    local_18 = &g_internalTouches;
    local_20 = &g_internalPrevTouches;
    local_28 = &g_internalPrevSideTouches;
    local_30 = &g_internalStatic;
    local_38 = (TSAPrpt *)&g_internalPrpt;
    local_40 = &g_internalAFEFrame;
    local_48 = &g_internalSafeBaseline;
    local_50 = &g_internalHandGesture;
  }
  else {
    local_18 = _refptr_g_touchesGrid;
    local_20 = _refptr_g_prevTouchesGrid;
    local_28 = _refptr_g_prevTouchesSide;
    local_30 = _refptr_g_tsaStaticGrid;
    local_38 = _refptr_g_tsaPrpt;
    local_40 = (undefined8 *)AFE_GetFrame();
    local_48 = _refptr_g_safeBaseline;
    local_50 = (undefined8 *)_refptr_handGesture;
  }
  switch(param_1) {
  case 0:
    local_c = (uint)(byte)local_18[5];
    break;
  case 1:
    local_c = (uint)*local_20;
    break;
  case 2:
    local_c = *(uint *)(local_30 + 0xbc);
    break;
  case 3:
    local_c = DAT_6bc7762c & *(uint *)(local_30 + 0xbc);
    break;
  case 4:
    local_c = (uint)(byte)local_30[2];
    break;
  case 5:
    local_c = DAT_6bc7762c & *(uint *)(local_30 + 0xa4);
    break;
  case 6:
    local_c = DAT_6bc7762c & *(uint *)(local_18 + 0x18);
    break;
  case 7:
    local_c = *(uint *)(local_30 + 0x280);
    break;
  case 8:
    local_c = (uint)(byte)local_18[3];
    break;
  case 9:
    local_c = (uint)((*(uint *)(local_18 + 0x18) & 0x80) != 0);
    break;
  case 10:
    local_c = (uint)*(ushort *)(local_40 + 6);
    break;
  case 0xb:
    local_c = (uint)*(ushort *)((longlong)local_40 + 0x32);
    break;
  case 0xc:
    local_c = *(uint *)((longlong)local_40 + 0x34) & 3;
    break;
  case 0xd:
    if (((DAT_6bc84df0._4_4_ & 1) == 0) ||
       (lVar2 = AFE_GetFrame(), (*(uint *)(lVar2 + 0x34) & 1) == 0)) {
      local_c = 0x5a5a5a5a;
    }
    else {
      local_c = (int)*(undefined8 *)(_refptr_g_tsaStaticGrid + 0xe0) - (int)DAT_6bc77720;
    }
    break;
  case 0xe:
    if (((DAT_6bc84df0._4_4_ & 2) == 0) ||
       (lVar2 = AFE_GetFrame(), (*(uint *)(lVar2 + 0x34) & 2) == 0)) {
      local_c = 0x5a5a5a5a;
    }
    else {
      local_c = (int)*(undefined8 *)(_refptr_g_tsaStaticGrid + 0xe0) - (int)DAT_6bc77720;
    }
    break;
  case 0xf:
    local_c = *(uint *)((longlong)local_40 + 0x34) & 0x60000;
    break;
  case 0x10:
    local_c = (uint)((*(uint *)(local_30 + 0xbc) & 4) != 0);
    break;
  case 0x11:
    local_c = (uint)((*(uint *)(local_30 + 0xbc) & 8) != 0);
    break;
  case 0x12:
    local_c = (uint)((*(uint *)(local_30 + 0xbc) & 1) != 0);
    break;
  case 0x13:
    local_c = (uint)((*(uint *)(local_30 + 0xbc) & 2) != 0);
    break;
  case 0x14:
    local_c = (uint)((*(uint *)(local_30 + 0xbc) & 0x10) != 0);
    break;
  case 0x15:
    local_c = (uint)((*(uint *)(local_30 + 0xbc) & 0x20) != 0);
    break;
  case 0x16:
    local_c = *(uint *)(local_30 + 0xfc);
    break;
  case 0x17:
    local_c = *(uint *)(local_18 + 0x20);
    break;
  case 0x18:
    local_c = *(uint *)(local_18 + 0x1ec);
    break;
  case 0x19:
    local_c = *(uint *)(local_18 + 0x1e0);
    break;
  case 0x1a:
    local_c = *(uint *)(local_18 + 0x224);
    break;
  case 0x1b:
    local_c = (uint)local_38->nSPeakPositive;
    break;
  case 0x1c:
    local_c = (uint)local_38->nSPeakNegative;
    break;
  case 0x1d:
    local_c = (uint)local_38->wBoundaryPositive;
    break;
  case 0x1e:
    local_c = (uint)local_38->wBoundaryNegative;
    break;
  case 0x1f:
    uVar3 = (int)*(short *)&local_38->field_0x2 >> 0x1f;
    uVar3 = ((int)*(short *)&local_38->field_0x2 ^ uVar3) - uVar3;
    local_c = (int)*(short *)local_38 >> 0x1f;
    local_c = ((int)*(short *)local_38 ^ local_c) - local_c;
    if ((int)local_c <= (int)uVar3) {
      local_c = uVar3;
    }
    break;
  case 0x20:
    local_c = EventAnalyzer_GetIDLEInterval();
    break;
  case 0x21:
    local_c = (uint)*(undefined8 *)(local_30 + 0xe0);
    break;
  case 0x22:
    local_c = (uint)*local_28;
    break;
  case 0x23:
    local_c = (uint)(byte)local_30[0xa8];
    break;
  case 0x24:
    local_c = 0x5a5a5a5a;
    break;
  case 0x25:
    bVar1 = TSA_GetASAFrameBypass();
    local_c = (uint)bVar1;
    break;
  case 0x26:
    local_c = *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0xd8;
    break;
  case 0x27:
    local_c = *(uint *)(local_48 + 4);
    break;
  case 0x28:
    local_c = *(uint *)((longlong)local_50 + 4);
    break;
  default:
    local_c = 0x5a5a5a5a;
  }
  return local_c;
}


// ============================================================================
// Function: EventAnalyzer_GetIDLEInterval @ 6badf121
// ============================================================================

longlong EventAnalyzer_GetIDLEInterval(void)

{
  char cVar1;
  
  if (_refptr_g_touchesGrid[5] != '\0') {
    flagTimerStarted_10507 = '\0';
  }
  if ((_refptr_g_touchesGrid[5] == '\0') && (DAT_6bc77ac5 != '\0')) {
    startTime_10508 = *(longlong *)(_refptr_g_tsaStaticGrid + 0xe0);
    flagTimerStarted_10507 = '\x01';
  }
  if ((flagTimerStarted_10507 != '\0') && (cVar1 = TSA_IsOKToEnterIDLEMode(), cVar1 != '\0')) {
    flagTimerStarted_10507 = 0;
    return *(longlong *)(_refptr_g_tsaStaticGrid + 0xe0) - startTime_10508;
  }
  return 0;
}


