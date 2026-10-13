// ============================================================================
// Function: UDFPEvent_Process @ 6bb3392c
// ============================================================================

void UDFPEvent_Process(void)

{
  ushort uVar1;
  short sVar2;
  undefined1 local_12;
  byte local_11;
  undefined4 local_10;
  undefined4 local_c;
  
                    /* 0xf392c  2564  UDFPEvent_Process */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&DAT_6bb5c898,"UDFPEvent_Process");
  }
  (*_refptr_g_tsaStaticPtr)->nField_0x28c = (*_refptr_g_tsaStaticPtr)->nField_0x28c & 0xfffffffe;
  if (_refptr_g_tsaPrmtFlash[2].field_0x6c0 != '\0') {
    UDFPEvent_UpdateEnableFlag((*_refptr_g_tsaStaticPtr)->dwSystemFlags);
    UDFPEvent_LogGeneralProperty();
    local_11 = 0;
    local_c = UDFPEvent_GetValidTouchNum(&local_11);
    local_10 = AFE_GetGesture();
    local_12 = 0;
    g_fingerSize = UDFPEvent_GetFingerSize(local_c,local_11,&local_12);
    uVar1 = TSAOut_Pixel2DisplayDim1
                      (*(undefined2 *)
                        ((longlong)(int)(uint)local_11 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x170));
    g_tp_x = (uint)uVar1;
    uVar1 = TSAOut_Pixel2DisplayDim2
                      (*(undefined2 *)
                        ((longlong)(int)(uint)local_11 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x172));
    g_tp_y = (uint)uVar1;
    sVar2 = TSAOut_IntToExtDist((int)*(short *)((longlong)(int)(uint)local_11 * 0x168 +
                                                *(longlong *)_refptr_g_touchesPtr + 0x18c));
    g_tp_major = (int)sVar2;
    sVar2 = TSAOut_IntToExtDist((int)*(short *)((longlong)(int)(uint)local_11 * 0x168 +
                                                *(longlong *)_refptr_g_touchesPtr + 0x18e));
    g_tp_minor = (int)sVar2;
    g_tp_ori = *(undefined1 *)
                ((longlong)(int)(uint)local_11 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x192);
    UDFPEvent_LogNxNOfLastTouch();
    UDFPEvent_LogTouchInfo(local_c,local_12,g_fingerSize);
    if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 5) == 0) {
      UDFPEvent_TransferState(local_c,local_11,local_12,g_fingerSize,local_10);
    }
    else {
      g_udfpState = 8;
    }
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"\ng_udfpState=%d\n",g_udfpState);
    }
    g_lastFingerid = local_12;
    UDFPEvent_PrintAppLog();
  }
  return;
}


// ============================================================================
// Function: UDFPEvent_UpdateEnableFlag @ 6bb32772
// ============================================================================

void UDFPEvent_UpdateEnableFlag(uint param_1)

{
  if ((param_1 & 0x4000000) == 0) {
    if ((param_1 & 0x8000000) != 0) {
      g_udfpEventEnabled = 0;
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(8,&DAT_6bb5c513,"TSA_UDFP_EVENT disabled");
    }
  }
  else {
    g_udfpEventEnabled = 1;
    StrPrintf_Process(0,"\nAppLog: ");
    StrPrintf_Process(8,&DAT_6bb5c513,"TSA_UDFP_EVENT enabled");
  }
  return;
}


// ============================================================================
// Function: UDFPEvent_LogGeneralProperty @ 6bb327fe
// ============================================================================

void UDFPEvent_LogGeneralProperty(void)

{
  uint uVar1;
  
  uVar1 = (int)_refptr_g_tsaPrpt->nSPeakNegative >> 0x1f;
  if ((0x96 < (int)(((int)_refptr_g_tsaPrpt->nSPeakNegative ^ uVar1) - uVar1)) ||
     (uVar1 = (int)_refptr_g_tsaPrpt->nSPeakPositive >> 0x1f,
     0x96 < (int)(((int)_refptr_g_tsaPrpt->nSPeakPositive ^ uVar1) - uVar1))) {
    UDFPEvent_LogDim2Line(_refptr_g_tsaPrmtFlash->bRows >> 1);
    UDFPEvent_LogDim1Line(_refptr_g_tsaPrmtFlash->bCols >> 1);
  }
  return;
}


// ============================================================================
// Function: UDFPEvent_GetValidTouchNum @ 6bb32871
// ============================================================================

int UDFPEvent_GetValidTouchNum(char *param_1)

{
  char cVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  int iVar8;
  int iVar9;
  int local_14;
  int local_10;
  char local_9;
  
  *param_1 = '\0';
  uVar2 = (uint)g_fpSensorArea;
  uVar3 = (uint)DAT_6bbaa8e2;
  uVar4 = (uint)DAT_6bbaa8e4;
  uVar5 = (uint)DAT_6bbaa8e6;
  local_10 = 0x7fffffff;
  local_14 = 0;
  local_9 = **(char **)_refptr_g_touchesPtr;
  while (local_9 = local_9 + -1, -1 < local_9) {
    uVar6 = (uint)*(ushort *)
                   ((longlong)(int)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170);
    uVar7 = (uint)*(ushort *)
                   ((longlong)(int)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
    cVar1 = UDFPEvent_IsInHoverArea(uVar6,uVar7);
    if (cVar1 != '\0') {
      local_14 = local_14 + 1;
      iVar8 = uVar6 - (uVar3 + uVar2) / 2;
      iVar9 = uVar7 - (uVar5 + uVar4) / 2;
      iVar8 = iVar9 * iVar9 + iVar8 * iVar8;
      if (iVar8 < local_10) {
        *param_1 = local_9;
        local_10 = iVar8;
      }
    }
  }
  return local_14;
}


// ============================================================================
// Function: UDFPEvent_GetFingerSize @ 6bb329b4
// ============================================================================

undefined2 UDFPEvent_GetFingerSize(int param_1,byte param_2,undefined1 *param_3)

{
  undefined2 uVar1;
  byte local_res10;
  
  if (param_1 == 0) {
    uVar1 = 0;
  }
  else {
    local_res10 = param_2;
    if (0x13 < param_2) {
      local_res10 = 0;
    }
    g_udfpEventPrePDim1 =
         (uint)*(ushort *)
                ((longlong)(int)(uint)local_res10 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                0x170);
    g_udfpEventPrePDim2 =
         (uint)*(ushort *)
                ((longlong)(int)(uint)local_res10 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                0x172);
    g_udfpEventLogDebounce = 3;
    uVar1 = *(undefined2 *)
             ((longlong)(int)(uint)local_res10 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x23e);
    *param_3 = *(undefined1 *)
                ((longlong)(int)(uint)local_res10 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                0x154);
    UDFPEvent_LogNxN((int)(char)((ushort)*(undefined2 *)
                                          ((longlong)(int)(uint)local_res10 * 0x168 +
                                           *(longlong *)_refptr_g_touchesPtr + 0x170) >> 8),
                     (int)(char)((ushort)*(undefined2 *)
                                          ((longlong)(int)(uint)local_res10 * 0x168 +
                                           *(longlong *)_refptr_g_touchesPtr + 0x172) >> 8));
  }
  return uVar1;
}


// ============================================================================
// Function: UDFPEvent_LogNxNOfLastTouch @ 6bb32b06
// ============================================================================

void UDFPEvent_LogNxNOfLastTouch(void)

{
  int iVar1;
  int iVar2;
  
  if ((g_udfpEventLogDebounce != 0) && (g_udfpEventLogDebounce < 3)) {
    iVar1 = g_udfpEventPrePDim2;
    if (g_udfpEventPrePDim2 < 0) {
      iVar1 = g_udfpEventPrePDim2 + 0xff;
    }
    iVar2 = g_udfpEventPrePDim1;
    if (g_udfpEventPrePDim1 < 0) {
      iVar2 = g_udfpEventPrePDim1 + 0xff;
    }
    UDFPEvent_LogNxN((int)(char)((uint)iVar2 >> 8),(int)(char)((uint)iVar1 >> 8));
  }
  if (g_udfpEventLogDebounce != 0) {
    g_udfpEventLogDebounce = g_udfpEventLogDebounce - 1;
  }
  return;
}


// ============================================================================
// Function: UDFPEvent_LogTouchInfo @ 6bb32b7a
// ============================================================================

void UDFPEvent_LogTouchInfo(int param_1,undefined1 param_2,undefined2 param_3)

{
  if (0 < param_1) {
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"\ntp_algo: fingerid=0x%02x fingerSize=%d\n",param_2,param_3);
    }
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"tp_algo: last_fingerid=0x%02x max_finger_size=%d\n",g_lastFingerid,
                        g_maxFingerSize);
    }
  }
  return;
}


// ============================================================================
// Function: UDFPEvent_PrintAppLog @ 6bb3370c
// ============================================================================

void UDFPEvent_PrintAppLog(void)

{
  int iVar1;
  
  if (((*_refptr_g_tsaStaticPtr)->nField_0x28c & 1U) != 0) {
    iVar1 = (*_refptr_g_tsaStaticPtr)->nField_0x284;
    if (iVar1 == 1) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(8,&DAT_6bb5c513,"TSA_UDFP_EVENT_FINGER_UP");
    }
    else if (iVar1 == 0) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(8,&DAT_6bb5c513,"TSA_UDFP_EVENT_FINGER_DOWN");
    }
    else if (iVar1 == 2) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(8,&DAT_6bb5c513,"TSA_UDFP_EVENT_HOVER_DOWN");
    }
    else if (iVar1 == 3) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(8,&DAT_6bb5c513,"TSA_UDFP_EVENT_HOVER_UP");
    }
  }
  return;
}


// ============================================================================
// Function: UDFPEvent_TransferState @ 6bb33811
// ============================================================================

void UDFPEvent_TransferState
               (undefined4 param_1,undefined1 param_2,undefined1 param_3,undefined2 param_4,
               undefined4 param_5)

{
                    /* 0xf3811  2567  UDFPEvent_TransferState */
  switch(g_udfpState) {
  case 0:
    UDFPEvent_ProcessIdle(param_1,param_2,param_4,param_5);
    break;
  case 1:
    UDFPEvent_ProcessAfeHoverCaptured(param_1,param_2,param_4,param_5);
    break;
  case 2:
    UDFPEvent_ProcessHoverCaptured(param_1,param_2,param_4,param_5);
    break;
  case 3:
    UDFPEvent_ProcessMoveCaptured(param_1,param_2,param_4);
    break;
  case 4:
    UDFPEvent_ProcessFingerCaptured(param_1,param_2,param_4);
    break;
  case 5:
    UDFPEvent_ProcessMissTouchedFinger(param_1,param_2);
    break;
  case 6:
    UDFPEvent_ProcessMissTouchedHover(param_1);
    break;
  case 7:
    UDFPEvent_ProcessFingerCaptureDisabled(param_1,param_3);
    break;
  case 8:
    UDFPEvent_ProcessDisabled();
    break;
  default:
    g_udfpState = 0;
  }
  return;
}


// ============================================================================
// Function: UDFPEvent_LogDim2Line @ 6bb31aee
// ============================================================================

void UDFPEvent_LogDim2Line(void)

{
  return;
}


// ============================================================================
// Function: UDFPEvent_LogDim1Line @ 6bb31af9
// ============================================================================

void UDFPEvent_LogDim1Line(void)

{
  return;
}


// ============================================================================
// Function: UDFPEvent_IsInHoverArea @ 6bb32303
// ============================================================================

void UDFPEvent_IsInHoverArea(undefined4 param_1,undefined4 param_2)

{
  Misc_IsInRect(&g_fpHoverArea,param_1,param_2);
  return;
}


// ============================================================================
// Function: UDFPEvent_LogNxN @ 6bb31ae0
// ============================================================================

void UDFPEvent_LogNxN(void)

{
  return;
}


// ============================================================================
// Function: UDFPEvent_ProcessIdle @ 6bb32c04
// ============================================================================

void UDFPEvent_ProcessIdle(int param_1,undefined1 param_2,undefined2 param_3,uint param_4)

{
  bool bVar1;
  int iVar2;
  
  bVar1 = 99 < (ulonglong)
               ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - g_lastHoverUpTimeStamp);
  if (g_udfpEventEnabled == '\0') {
    g_udfpState = 8;
  }
  else if (param_1 == 0) {
    if (((param_4 & 4) != 0) && (bVar1)) {
      g_udfpState = 1;
      UDFPEvent_Notify(2);
      if (*_refptr_g_tsaLogLoglevel != '\0') {
        StrPrintf_Process(0,"tp_algo: hover event\n");
      }
    }
  }
  else {
    iVar2 = UDFPEvent_IsLargeObject(param_2);
    if (iVar2 == 0) {
      iVar2 = UDFPEvent_IsValidFingerprint(param_2);
      if (iVar2 == 0) {
        iVar2 = UDFPEvent_IsTouchAtMoveZone(param_2);
        if (iVar2 == 0) {
          if (bVar1) {
            g_udfpState = 2;
            UDFPEvent_Notify(2);
            if (*_refptr_g_tsaLogLoglevel != '\0') {
              StrPrintf_Process(0,"tp_algo: first touch out move area, touch hover\n");
            }
          }
        }
        else if (bVar1) {
          g_udfpState = 3;
          g_timeFirstTouch = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
          UDFPEvent_Notify(2);
          if (*_refptr_g_tsaLogLoglevel != '\0') {
            StrPrintf_Process(0,"tp_algo: first touch in move area, touch hover\n");
          }
        }
      }
      else {
        g_udfpState = 4;
        UDFPEvent_Notify(0);
        if (*_refptr_g_tsaLogLoglevel != '\0') {
          StrPrintf_Process(0,"tp_algo: first touch, finger down\n");
        }
      }
    }
    else {
      g_udfpState = 6;
      UDFPEvent_Notify(2);
      if (*_refptr_g_tsaLogLoglevel != '\0') {
        StrPrintf_Process(0,"tp_algo: mistake touch finger size invalid, fingerSize=%d\n",param_3);
      }
    }
  }
  return;
}


// ============================================================================
// Function: UDFPEvent_ProcessDisabled @ 6bb32dff
// ============================================================================

void UDFPEvent_ProcessDisabled(void)

{
  if (g_udfpEventEnabled != '\0') {
    g_udfpState = 0;
    UDFPEvent_Notify(1);
    g_maxFingerSize = 0;
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"tp_algo: reset as fingerup\n");
    }
  }
  return;
}


// ============================================================================
// Function: UDFPEvent_ProcessAfeHoverCaptured @ 6bb32e55
// ============================================================================

void UDFPEvent_ProcessAfeHoverCaptured
               (int param_1,undefined1 param_2,undefined2 param_3,uint param_4)

{
  int iVar1;
  
  if (g_udfpEventEnabled == '\0') {
    g_udfpState = 8;
    UDFPEvent_Notify(3);
  }
  else if (param_1 == 0) {
    if ((param_4 & 4) == 0) {
      UDFPEvent_Notify(3);
      g_udfpState = 0;
    }
  }
  else {
    iVar1 = UDFPEvent_IsLargeObject(param_2);
    if (iVar1 == 0) {
      iVar1 = UDFPEvent_IsValidFingerprint(param_2);
      if (iVar1 == 0) {
        iVar1 = UDFPEvent_IsTouchAtMoveZone(param_2);
        if (iVar1 == 0) {
          g_udfpState = 2;
          if (*_refptr_g_tsaLogLoglevel != '\0') {
            StrPrintf_Process(0,"tp_algo: out move area, touch hover\n");
          }
        }
        else {
          g_udfpState = 3;
          g_timeFirstTouch = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
        }
      }
      else {
        g_udfpState = 4;
        UDFPEvent_Notify(0);
        g_totalTime = 0;
        if (*_refptr_g_tsaLogLoglevel != '\0') {
          StrPrintf_Process(0,"tp_algo: first touch, finger down\n");
        }
      }
    }
    else {
      g_udfpState = 6;
      if (*_refptr_g_tsaLogLoglevel != '\0') {
        StrPrintf_Process(0,"tp_algo: mistake touch finger size invalid, fingerSize=%d\n",param_3);
      }
    }
  }
  return;
}


// ============================================================================
// Function: UDFPEvent_ProcessHoverCaptured @ 6bb32fba
// ============================================================================

void UDFPEvent_ProcessHoverCaptured(int param_1,byte param_2,undefined2 param_3)

{
  char cVar1;
  int iVar2;
  byte local_res10;
  
  local_res10 = param_2;
  if (0x13 < param_2) {
    local_res10 = 0;
  }
  cVar1 = *(char *)((longlong)(int)(uint)local_res10 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                   0x154);
  if (g_udfpEventEnabled == '\0') {
    g_udfpState = 8;
    UDFPEvent_Notify(3);
  }
  else if (param_1 == 0) {
    g_udfpState = 0;
    UDFPEvent_Notify(3);
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"tp_algo: hover release\n");
    }
  }
  else {
    iVar2 = UDFPEvent_IsLargeObject(local_res10);
    if (iVar2 == 0) {
      if ((cVar1 == g_lastFingerid) ||
         (iVar2 = UDFPEvent_IsValidFingerprint(local_res10), iVar2 == 0)) {
        if ((cVar1 != g_lastFingerid) &&
           (iVar2 = UDFPEvent_IsTouchAtMoveZone(local_res10), iVar2 != 0)) {
          g_udfpState = 3;
          g_timeFirstTouch = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
        }
      }
      else {
        g_udfpState = 4;
        UDFPEvent_Notify(0);
        g_totalTime = 0;
        if (*_refptr_g_tsaLogLoglevel != '\0') {
          StrPrintf_Process(0,"tp_algo: first touch, finger down\n");
        }
      }
    }
    else {
      g_udfpState = 6;
      if (*_refptr_g_tsaLogLoglevel != '\0') {
        StrPrintf_Process(0,"tp_algo: mistake touch finger size invalid, fingerSize=%d\n",param_3);
      }
    }
  }
  return;
}


// ============================================================================
// Function: UDFPEvent_ProcessMoveCaptured @ 6bb33153
// ============================================================================

void UDFPEvent_ProcessMoveCaptured(int param_1,undefined1 param_2,undefined2 param_3)

{
  int iVar1;
  
  if (g_udfpEventEnabled == '\0') {
    g_udfpState = 8;
    UDFPEvent_Notify(3);
  }
  else if (param_1 == 0) {
    g_udfpState = 0;
    UDFPEvent_Notify(3);
  }
  else {
    iVar1 = UDFPEvent_IsLargeObject(param_2);
    if (iVar1 == 0) {
      iVar1 = UDFPEvent_IsValidFingerprint(param_2);
      if (iVar1 == 0) {
        if (*_refptr_g_tsaLogLoglevel != '\0') {
          StrPrintf_Process(0,"tp_algo: in move area, touch hover\n");
        }
      }
      else {
        g_udfpState = 4;
        UDFPEvent_Notify(0);
        g_totalTime = (int)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - (int)g_timeFirstTouch;
        if (*_refptr_g_tsaLogLoglevel != '\0') {
          StrPrintf_Process(0,"tp_algo: in move area, finger down, total_time=%d\n",g_totalTime);
        }
      }
    }
    else {
      g_udfpState = 6;
      if (*_refptr_g_tsaLogLoglevel != '\0') {
        StrPrintf_Process(0,"tp_algo: mistake touch finger size invalid, fingerSize=%d\n",param_3);
      }
    }
  }
  return;
}


// ============================================================================
// Function: UDFPEvent_ProcessFingerCaptured @ 6bb33296
// ============================================================================

void UDFPEvent_ProcessFingerCaptured(int param_1,byte param_2,ushort param_3)

{
  char cVar1;
  int iVar2;
  byte local_res10;
  
  local_res10 = param_2;
  if (0x13 < param_2) {
    local_res10 = 0;
  }
  cVar1 = *(char *)((longlong)(int)(uint)local_res10 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                   0x154);
  if (g_udfpEventEnabled == '\0') {
    g_udfpState = 7;
    UDFPEvent_Notify(1);
    g_maxFingerSize = 0;
    g_lastFingerUpTimeStamp = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  }
  else if (param_1 == 0) {
    g_udfpState = 0;
    UDFPEvent_Notify(1);
    g_maxFingerSize = 0;
    g_lastFingerUpTimeStamp = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"tp_algo: finger capture release\n");
    }
  }
  else {
    iVar2 = UDFPEvent_IsLargeObject(local_res10);
    if (iVar2 == 0) {
      if (cVar1 != g_lastFingerid) {
        g_udfpState = 0;
        UDFPEvent_Notify(1);
        if (*_refptr_g_tsaLogLoglevel != '\0') {
          StrPrintf_Process(0,"tp_algo: in capture, finger up 2\n");
        }
        g_lastFingerUpTimeStamp = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
      }
    }
    else {
      g_udfpState = 5;
      g_maxFingerSize = 0;
      if (*_refptr_g_tsaLogLoglevel != '\0') {
        StrPrintf_Process(0,"tp_algo: mistake touch finger size invalid, fingerSize=%d\n",param_3);
      }
      g_lastFingerUpTimeStamp = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    }
  }
  if (param_3 <= g_maxFingerSize) {
    param_3 = g_maxFingerSize;
  }
  g_maxFingerSize = param_3;
  return;
}


// ============================================================================
// Function: UDFPEvent_ProcessMissTouchedHover @ 6bb33466
// ============================================================================

void UDFPEvent_ProcessMissTouchedHover(int param_1)

{
  if (g_udfpEventEnabled == '\0') {
    g_udfpState = 8;
    UDFPEvent_Notify(3);
    g_misTouchCounter = g_misTouchCounter + 1;
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"tp_algo: finger touch release, g_misTouchCounter=%d\n",g_misTouchCounter)
      ;
    }
  }
  else if (param_1 == 0) {
    g_udfpState = 0;
    UDFPEvent_Notify(3);
    g_misTouchCounter = g_misTouchCounter + 1;
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"tp_algo: finger touch release, g_misTouchCounter=%d\n",g_misTouchCounter)
      ;
    }
  }
  else if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,
                      "tp_algo: mistake touch but finger size of this INT is valid, not in capture\n"
                     );
  }
  return;
}


// ============================================================================
// Function: UDFPEvent_ProcessMissTouchedFinger @ 6bb33546
// ============================================================================

void UDFPEvent_ProcessMissTouchedFinger(int param_1,byte param_2)

{
  int iVar1;
  byte local_res10;
  
  local_res10 = param_2;
  if (0x13 < param_2) {
    local_res10 = 0;
  }
  if (g_udfpEventEnabled == '\0') {
    g_udfpState = 8;
    UDFPEvent_Notify(1);
    g_misTouchCounter = g_misTouchCounter + 1;
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"tp_algo: finger touch release, g_misTouchCounter=%d\n",g_misTouchCounter)
      ;
    }
  }
  else if (param_1 == 0) {
    g_udfpState = 0;
    UDFPEvent_Notify(1);
    g_misTouchCounter = g_misTouchCounter + 1;
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"tp_algo: finger touch release, g_misTouchCounter=%d\n",g_misTouchCounter)
      ;
    }
  }
  else if (((param_1 < 1) ||
           (*(char *)((longlong)(int)(uint)local_res10 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x154) == g_lastFingerid)) ||
          (iVar1 = UDFPEvent_IsLargeObject(local_res10), iVar1 != 0)) {
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,
                        "tp_algo: mistake touch but finger size of this INT is valid, not in capture\n"
                       );
    }
  }
  else {
    g_udfpState = 0;
    UDFPEvent_Notify(1);
    g_misTouchCounter = g_misTouchCounter + 1;
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"tp_algo: finger touch release, g_misTouchCounter=%d\n",g_misTouchCounter)
      ;
    }
  }
  return;
}


// ============================================================================
// Function: UDFPEvent_ProcessFingerCaptureDisabled @ 6bb336d6
// ============================================================================

void UDFPEvent_ProcessFingerCaptureDisabled(int param_1,char param_2)

{
  if (param_1 == 0) {
    g_udfpState = 0;
  }
  else if (param_2 != g_lastFingerid) {
    g_udfpState = 0;
  }
  return;
}


// ============================================================================
// Function: UDFPEvent_IsEnabled @ 6bb31b04
// ============================================================================

undefined8 UDFPEvent_IsEnabled(void)

{
  undefined8 uVar1;
  
                    /* 0xf1b04  2563  UDFPEvent_IsEnabled */
  if ((_refptr_g_tsaPrmtFlash[2].field_0x6c0 == '\0') || (g_udfpEventEnabled == '\0')) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: UDFPEvent_Notify @ 6bb3232c
// ============================================================================

void UDFPEvent_Notify(int param_1)

{
  (*_refptr_g_tsaStaticPtr)->nField_0x284 = param_1;
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"tp_algo: real event is %d\n",param_1);
  }
  (*_refptr_g_tsaStaticPtr)->nField_0x28c = (*_refptr_g_tsaStaticPtr)->nField_0x28c | 1;
  if (param_1 == 3) {
    g_lastHoverUpTimeStamp = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  }
  return;
}


// ============================================================================
// Function: UDFPEvent_IsValidFingerprint @ 6bb325ec
// ============================================================================

undefined8 UDFPEvent_IsValidFingerprint(byte param_1)

{
  char cVar1;
  char cVar2;
  undefined8 uVar3;
  ulonglong uVar4;
  
  uVar4 = (longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - g_lastFingerUpTimeStamp;
  cVar1 = UDFPEvent_IsAreaMeet(param_1);
  cVar2 = UDFPEvent_IsInFingerArea
                    (*(undefined2 *)
                      ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x170),*(undefined2 *)
                              ((longlong)(int)(uint)param_1 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x172));
  if (((cVar2 == '\0') || (cVar1 == '\0')) || (uVar4 < 0x65)) {
    uVar3 = 0;
  }
  else {
    uVar3 = 1;
  }
  return uVar3;
}


// ============================================================================
// Function: UDFPEvent_IsTouchAtMoveZone @ 6bb326b9
// ============================================================================

undefined1 UDFPEvent_IsTouchAtMoveZone(byte param_1)

{
  undefined1 uVar1;
  
  uVar1 = UDFPEvent_IsInMoveArea
                    (*(undefined2 *)
                      ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x170),*(undefined2 *)
                              ((longlong)(int)(uint)param_1 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x172));
  return uVar1;
}


// ============================================================================
// Function: UDFPEvent_IsLargeObject @ 6bb3272f
// ============================================================================

bool UDFPEvent_IsLargeObject(byte param_1)

{
  byte local_res8;
  
  local_res8 = param_1;
  if (0x13 < param_1) {
    local_res8 = 0;
  }
  return 0x28 < *(ushort *)
                 ((longlong)(int)(uint)local_res8 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                 0x23e);
}


// ============================================================================
// Function: UDFPEvent_IsCloseFingerArea @ 6bb31c5a
// ============================================================================

undefined8 UDFPEvent_IsCloseFingerArea(ushort param_1,ushort param_2)

{
  ushort uVar1;
  uint uVar2;
  
                    /* 0xf1c5a  2562  UDFPEvent_IsCloseFingerArea */
  uVar1 = UDFPEvent_GetDim1Center();
  uVar2 = (int)((uint)param_1 - (uint)uVar1) >> 0x1f;
  if (((int)(((uint)param_1 - (uint)uVar1 ^ uVar2) - uVar2) < 0x400) &&
     (uVar1 = UDFPEvent_GetDim2Center(), uVar2 = (int)((uint)param_2 - (uint)uVar1) >> 0x1f,
     (int)(((uint)param_2 - (uint)uVar1 ^ uVar2) - uVar2) < 0x400)) {
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: UDFPEvent_IsInFingerArea @ 6bb322b1
// ============================================================================

void UDFPEvent_IsInFingerArea(undefined4 param_1,undefined4 param_2)

{
  Misc_IsInRect(&g_fpFingerArea,param_1,param_2);
  return;
}


// ============================================================================
// Function: UDFPEvent_IsAreaMeet @ 6bb323b4
// ============================================================================

bool UDFPEvent_IsAreaMeet(byte param_1)

{
  undefined2 local_28;
  undefined2 local_26;
  undefined2 local_24;
  undefined2 local_22;
  short local_20;
  undefined2 local_18;
  undefined2 local_16;
  undefined2 local_14;
  
  local_18 = (undefined2)(((uint)DAT_6bbaa8e2 + (uint)g_fpSensorArea) / 2);
  local_16 = (undefined2)(((uint)DAT_6bbaa8e6 + (uint)DAT_6bbaa8e4) / 2);
  local_14 = (undefined2)((int)((uint)DAT_6bbaa8e2 - (uint)g_fpSensorArea) / 2);
  local_28 = *(undefined2 *)
              ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170);
  local_26 = *(undefined2 *)
              ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
  local_20 = (short)*(char *)((longlong)(int)(uint)param_1 * 0x168 +
                              *(longlong *)_refptr_g_touchesPtr + 0x192);
  local_24 = *(undefined2 *)
              ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x18c);
  local_22 = *(undefined2 *)
              ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x18e);
  if (*(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x195) <
      0xe) {
    local_24 = TS_MMToPixel(*(undefined1 *)
                             ((longlong)(int)(uint)param_1 * 0x168 +
                              *(longlong *)_refptr_g_touchesPtr + 0x195));
    local_22 = TS_MMToPixel(*(undefined1 *)
                             ((longlong)(int)(uint)param_1 * 0x168 +
                              *(longlong *)_refptr_g_touchesPtr + 0x195));
  }
  g_coveredPercent = UDFPEvent_GetCoveredPercent(&local_18,&local_28);
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"\n tp_algo: area = %d%%",g_coveredPercent);
  }
  return (int)(uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x8e <= g_coveredPercent;
}


// ============================================================================
// Function: UDFPEvent_IsInMoveArea @ 6bb322da
// ============================================================================

void UDFPEvent_IsInMoveArea(undefined4 param_1,undefined4 param_2)

{
  Misc_IsInRect(&g_fpMoveArea,param_1,param_2);
  return;
}


// ============================================================================
// Function: UDFPEvent_GetDim1Center @ 6bb31c1e
// ============================================================================

int UDFPEvent_GetDim1Center(void)

{
  return (int)((uint)DAT_6bbaa8e2 + (uint)g_fpSensorArea) >> 1;
}


// ============================================================================
// Function: UDFPEvent_GetDim2Center @ 6bb31c3c
// ============================================================================

int UDFPEvent_GetDim2Center(void)

{
  return (int)((uint)DAT_6bbaa8e6 + (uint)DAT_6bbaa8e4) >> 1;
}


// ============================================================================
// Function: UDFPEvent_GetCoveredPercent @ 6bb3203d
// ============================================================================

int UDFPEvent_GetCoveredPercent(ushort *param_1,ushort *param_2)

{
  ushort uVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int local_1c;
  
  local_1c = 0;
  uVar2 = (uint)(param_2[2] >> 1);
  uVar1 = param_2[3];
  iVar3 = -(int)(short)param_2[4];
  iVar6 = (uint)*param_1 - (uint)*param_2;
  iVar7 = (uint)param_1[1] - (uint)param_2[1];
  if (iVar6 * iVar6 + iVar7 * iVar7 < (int)((uVar2 + param_1[2]) * (param_1[2] + uVar2))) {
    iVar4 = Misc_Cos(iVar3);
    iVar5 = Misc_Sin(iVar3);
    iVar4 = iVar4 * iVar6 - iVar5 * iVar7;
    if (iVar4 < 0) {
      iVar4 = iVar4 + 0xfff;
    }
    iVar5 = Misc_Sin(iVar3);
    iVar3 = Misc_Cos(iVar3);
    iVar3 = iVar3 * iVar7 + iVar5 * iVar6;
    if (iVar3 < 0) {
      iVar3 = iVar3 + 0xfff;
    }
    local_1c = UDFPEvent_GetCoveredArea(iVar4 >> 0xc,iVar3 >> 0xc,param_1[2],uVar2,uVar1 >> 1);
  }
  return (local_1c * 100) / 0x70;
}


// ============================================================================
// Function: UDFPEvent_GetCoveredArea @ 6bb31ec7
// ============================================================================

int UDFPEvent_GetCoveredArea(int param_1,int param_2,int param_3,int param_4,int param_5)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int local_18;
  int local_14;
  int local_10;
  int local_c;
  
  local_10 = 0;
  iVar1 = (param_3 * 2) / 0xc;
  if (param_4 == 0) {
    local_14 = 0x10;
  }
  else {
    local_14 = (param_5 << 4) / param_4;
  }
  for (local_c = -param_3; local_c <= param_3; local_c = local_c + iVar1) {
    iVar2 = param_1 + local_c;
    if ((iVar2 <= param_4) && (-param_4 <= iVar2)) {
      local_18 = UDFPEvent_CircleSqrt(param_3,local_c);
      iVar3 = param_2 - local_18;
      local_18 = param_2 + local_18;
      iVar2 = iVar2 * local_14;
      if (iVar2 < 0) {
        iVar2 = iVar2 + 0xf;
      }
      iVar2 = UDFPEvent_CircleSqrt(param_5,iVar2 >> 4);
      if (iVar3 < -iVar2) {
        if (local_18 < -iVar2) {
          local_18 = 0;
        }
        else if (local_18 < iVar2) {
          local_18 = local_18 + iVar2;
        }
        else {
          local_18 = iVar2 * 2;
        }
      }
      else if (iVar3 < iVar2) {
        if (local_18 < iVar2) {
          local_18 = local_18 - iVar3;
        }
        else {
          local_18 = iVar2 - iVar3;
        }
      }
      else {
        local_18 = 0;
      }
      local_10 = local_10 + local_18;
    }
  }
  if (iVar1 == 0) {
    local_10 = 0;
  }
  else {
    local_10 = local_10 / iVar1;
  }
  return local_10;
}


// ============================================================================
// Function: UDFPEvent_CircleSqrt @ 6bb31e5d
// ============================================================================

uint UDFPEvent_CircleSqrt(uint param_1,uint param_2)

{
  uint local_res8;
  uint local_c;
  
  local_res8 = param_1;
  if (param_1 == 0) {
    local_res8 = 1;
  }
  local_c = ((((int)param_2 >> 0x1f ^ param_2) - ((int)param_2 >> 0x1f)) * 0x100) / local_res8;
  if (0xff < local_c) {
    local_c = 0xff;
  }
  return (int)*(short *)(&g_circleSqrt + (ulonglong)local_c * 2) * local_res8 >> 0xc;
}


