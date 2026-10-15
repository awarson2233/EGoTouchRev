// ============================================================================
// Function: Proximity_FilterExtraWakeup @ 6baf950c
// ============================================================================

uint Proximity_FilterExtraWakeup(uint param_1)

{
  char cVar1;
  uint local_res8;
  
                    /* 0xb950c  1090  Proximity_FilterExtraWakeup */
  cVar1 = Proximity_IsEnabled();
  if (cVar1 != '\0') {
    local_res8 = param_1;
    if ((param_1 & 0x2000000) != 0) {
      local_res8 = param_1 | 4;
      (*_refptr_g_tsaStaticPtr)->llField_0x48 = (longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr
      ;
    }
    param_1 = local_res8;
  }
  return param_1;
}


// ============================================================================
// Function: Proximity_TouchPostProcess @ 6baf9561
// ============================================================================

void Proximity_TouchPostProcess(void)

{
  char cVar1;
  longlong lVar2;
  byte local_9;
  
                    /* 0xb9561  1127  Proximity_TouchPostProcess */
  cVar1 = Proximity_IsEnabled();
  if (cVar1 != '\0') {
    cVar1 = TSA_IsScreenLockScenario();
    if ((((cVar1 != '\0') && (cVar1 = Proximity_IsDetected(), cVar1 != '\0')) &&
        (lVar2 = TSAStatic_GetCurrTimeStamp(),
        1000 < (ulonglong)(lVar2 - *(longlong *)(g_proxData + 0x18)))) &&
       ((uint)*(byte *)(*(longlong *)_refptr_g_touchesPtr + 3) +
        (uint)**(byte **)_refptr_g_touchesPtr != 0)) {
      *(ushort *)(*(longlong *)_refptr_g_touchesPtr + 0x170) =
           *(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x20) >> 1;
      *(ushort *)(*(longlong *)_refptr_g_touchesPtr + 0x172) =
           *(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x22) >> 1;
      *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0x154) = 10;
      *(undefined2 *)(*(longlong *)_refptr_g_touchesPtr + 0x188) = 10;
      *(undefined2 *)(*(longlong *)_refptr_g_touchesPtr + 0x18a) = 10;
      *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x1f8) =
           *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x1f8) | 8;
      *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x1f0) =
           *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x1f0) & 0xf7ffffff;
      *(uint *)(*(longlong *)_refptr_g_touchesPtr + 500) =
           *(uint *)(*(longlong *)_refptr_g_touchesPtr + 500) | 4;
      **(undefined1 **)_refptr_g_touchesPtr = 1;
      *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 3) = 0;
    }
    cVar1 = AFE_IsInProximityScreenOffState();
    if ((cVar1 != '\0') && (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x400000) == 0)) {
      for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
        if ((*(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x1f8) & 0x800000) == 0) {
          *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 500)
               = *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 500) | 0x4000;
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: Proximity_Process @ 6bafb528
// ============================================================================

void Proximity_Process(void)

{
  char cVar1;
  int iVar2;
  
                    /* 0xbb528  1118  Proximity_Process */
  cVar1 = TSAStatic_IsProximityEnabled();
  if ((cVar1 == '\0') || (cVar1 = Proximity_MallocData(), cVar1 != '\0')) {
    (*_refptr_g_tsaStaticPtr)->dwProcessingState =
         (*_refptr_g_tsaStaticPtr)->dwProcessingState | 0x20;
    DataSwitch_ToProx();
    cVar1 = TSAStatic_IsProximityEnabled();
    if (cVar1 == '\0') {
      if ((g_proxData != 0) && (iVar2 = memset_s(g_proxData,0x68,0,0x68), iVar2 != 0)) {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                          "../../src/Extension/Proximity/Proximity.c",0x5b2,"rc == EOK");
        HardwareAnalyzer_SetSoftException();
      }
      SensorGesture_ClearExternally(6);
    }
    else {
      *(undefined4 *)(g_proxData + 0x54) = *(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x5e0;
      if (*(char *)(g_proxData + 6) == '\0') {
        Proximity_Reset();
      }
      Proximtiy_SignalProcess();
      cVar1 = TSA_IsScreenLockScenario();
      if (cVar1 != '\0') {
        *(uint *)(g_proxData + 0x54) = *(uint *)(g_proxData + 0x54) & 0xffffff8f;
      }
      cVar1 = TSA_IsPhoneCallScenario();
      if ((cVar1 != '\0') && ((*(uint *)(g_proxData + 0x54) & 0x800) != 0)) {
        *(uint *)(g_proxData + 0x54) = *(uint *)(g_proxData + 0x54) & 0xffffffdf;
      }
      if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x80000) != 0) {
        *(uint *)(g_proxData + 0x54) = *(uint *)(g_proxData + 0x54) & 0xffffff9f;
        *(uint *)(g_proxData + 0x54) = *(uint *)(g_proxData + 0x54) | 0x8000;
      }
      if (((*_refptr_g_tsaStaticPtr)->llField_0x50 != -1) &&
         ((ulonglong)
          ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
          (*_refptr_g_tsaStaticPtr)->llField_0x50) <=
          (ulonglong)*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x614)) {
        *(uint *)(g_proxData + 0x54) = *(uint *)(g_proxData + 0x54) & 0xffffff9f;
      }
      if (((*_refptr_g_tsaStaticPtr)->llField_0x48 != -1) &&
         ((ulonglong)
          ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
          (*_refptr_g_tsaStaticPtr)->llField_0x48) <=
          (ulonglong)*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x618)) {
        *(uint *)(g_proxData + 0x54) = *(uint *)(g_proxData + 0x54) & 0xffffffdf;
      }
      if (*(char *)(g_proxData + 1) == '\0') {
        Proximity_IsNotDetectedProcess();
      }
      else {
        Proximity_IsDetectedProcess();
      }
    }
    DataSwitch_ToGrid();
    (*_refptr_g_tsaStaticPtr)->dwProcessingState =
         (*_refptr_g_tsaStaticPtr)->dwProcessingState & 0xffffffdf;
  }
  return;
}


// ============================================================================
// Function: Proximity_IsEnabled @ 6baf8a12
// ============================================================================

undefined8 Proximity_IsEnabled(void)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0xb8a12  1101  Proximity_IsEnabled */
  cVar1 = TSAStatic_IsProximityEnabled();
  if (cVar1 == '\0') {
    uVar2 = 0;
  }
  else if (g_proxData == 0) {
    uVar2 = 0;
  }
  else {
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: Proximity_IsDetected @ 6baf8a70
// ============================================================================

undefined1 Proximity_IsDetected(void)

{
  char cVar1;
  undefined1 uVar2;
  
                    /* 0xb8a70  1099  Proximity_IsDetected */
  cVar1 = Proximity_IsEnabled();
  if (cVar1 == '\0') {
    uVar2 = 0;
  }
  else {
    uVar2 = *(undefined1 *)(g_proxData + 1);
  }
  return uVar2;
}


// ============================================================================
// Function: Proximity_Reset @ 6baf855d
// ============================================================================

void Proximity_Reset(void)

{
  undefined1 uVar1;
  undefined4 uVar2;
  undefined8 uVar3;
  undefined8 uVar4;
  int iVar5;
  
                    /* 0xb855d  1121  Proximity_Reset */
  uVar3 = *(undefined8 *)(g_proxData + 0x20);
  uVar2 = *(undefined4 *)(g_proxData + 0x3c);
  uVar1 = *(undefined1 *)(g_proxData + 2);
  uVar4 = *(undefined8 *)(g_proxData + 0x18);
  iVar5 = memset_s(g_proxData,0x68,0,0x68);
  if (iVar5 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/Proximity/Proximity.c",0x47,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar5 = memset_s(g_prevTouchesProxPtr,0x5aa0,0,0x5aa0);
  if (iVar5 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/Proximity/Proximity.c",0x49,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  Proximity_BaselineReset();
  *(undefined1 *)(g_proxData + 6) = 1;
  *(undefined8 *)(g_proxData + 0x20) = uVar3;
  *(undefined4 *)(g_proxData + 0x3c) = uVar2;
  *(undefined1 *)(g_proxData + 2) = uVar1;
  *(undefined8 *)(g_proxData + 0x18) = uVar4;
  return;
}


// ============================================================================
// Function: Proximity_MallocData @ 6baf8a48
// ============================================================================

bool Proximity_MallocData(void)

{
  char cVar1;
  
                    /* 0xb8a48  1117  Proximity_MallocData */
  cVar1 = Proximity_CreateData();
  if (cVar1 == '\0') {
    Proximity_DeleteData();
  }
  return cVar1 != '\0';
}


// ============================================================================
// Function: Proximity_IsDetectedProcess @ 6bafb2a5
// ============================================================================

void Proximity_IsDetectedProcess(void)

{
  char cVar1;
  
                    /* 0xbb2a5  1100  Proximity_IsDetectedProcess */
  if ((*(uint *)(g_proxData + 0x54) & 0x2000) != 0) {
    SensorGesture_ClearExternally(2);
  }
  if ((*(uint *)(g_proxData + 0x54) & 0x400) == 0) {
    if ((*(uint *)(g_proxData + 0x54) & 0x100) != 0) {
      if (((*(uint *)(g_proxData + 0x54) & 0x40) == 0) ||
         (cVar1 = SensorGesture_IsProximityPutDownGestureEnabled(), cVar1 == '\0')) {
        Proximity_ProcessInternal();
      }
      else {
        cVar1 = SensorGesture_IsProximityPutDownGestureTimeOut
                          (*(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x608);
        if (cVar1 == '\0') {
          Proximity_ProcessInternal();
        }
        else if ((((*(uint *)(g_proxData + 0x54) & 0x200) != 0) &&
                 (cVar1 = TSA_IsPhoneCallScenario(), cVar1 != '\0')) &&
                (*(char *)(g_proxData + 3) != '\0')) {
          *(uint *)(g_proxData + 0x54) = *(uint *)(g_proxData + 0x54) & 0xffffffef;
          Proximity_ProcessInternal();
        }
      }
    }
  }
  else {
    cVar1 = SensorGesture_IsProximityPutDownGestureTimeOut
                      (*(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x608);
    if (cVar1 == '\0') {
      *(undefined1 *)(g_proxData + 1) = 0;
      *(void **)(g_proxData + 0x20) = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    }
  }
  return;
}


// ============================================================================
// Function: Proximity_IsNotDetectedProcess @ 6bafb3cc
// ============================================================================

void Proximity_IsNotDetectedProcess(void)

{
  char cVar1;
  
                    /* 0xbb3cc  1103  Proximity_IsNotDetectedProcess */
  if ((*(uint *)(g_proxData + 0x54) & 0x2000) != 0) {
    SensorGesture_ClearExternally(4);
  }
  if ((*(uint *)(g_proxData + 0x54) & 0x400) == 0) {
    if ((*(uint *)(g_proxData + 0x54) & 0x80) != 0) {
      if (((*(uint *)(g_proxData + 0x54) & 0x20) != 0) &&
         (cVar1 = SensorGesture_IsProximityPickUpGestureEnabled(), cVar1 != '\0')) {
        cVar1 = SensorGesture_IsProximityPickUpGestureTimeOut
                          (*(undefined4 *)&_refptr_g_tsaPrmtFlash[2].bRawdataForceNormalize);
        if (cVar1 == '\0') {
          if ((*(char *)(g_proxData + 0x58) != '\0') &&
             ((*(uint *)(g_proxData + 0x54) & 0x1000) != 0)) {
            *(undefined1 *)(g_proxData + 6) = 0;
          }
          *(undefined1 *)(g_proxData + 0x58) = 0;
          Proximity_ProcessInternal();
          return;
        }
        *(undefined1 *)(g_proxData + 0x58) = 1;
        if ((*(uint *)(g_proxData + 0x54) & 0x200) == 0) {
          return;
        }
        cVar1 = TSA_IsPhoneCallScenario();
        if (cVar1 == '\0') {
          return;
        }
        *(uint *)(g_proxData + 0x54) = *(uint *)(g_proxData + 0x54) & 0xffffffef;
        Proximity_ProcessInternal();
        return;
      }
      Proximity_ProcessInternal();
    }
  }
  else {
    cVar1 = SensorGesture_IsProximityPickUpGestureTimeOut
                      (*(undefined4 *)&_refptr_g_tsaPrmtFlash[2].bRawdataForceNormalize);
    if (cVar1 == '\0') {
      *(undefined1 *)(g_proxData + 1) = 1;
      *(void **)(g_proxData + 0x18) = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    }
  }
  return;
}


// ============================================================================
// Function: Proximity_IsToExitIDLE @ 6baf8f82
// ============================================================================

bool Proximity_IsToExitIDLE(void)

{
  char cVar1;
  bool local_9;
  
                    /* 0xb8f82  1113  Proximity_IsToExitIDLE */
  cVar1 = Proximity_IsEnabled();
  if (cVar1 == '\0') {
    local_9 = false;
  }
  else {
    local_9 = false;
    cVar1 = TSAStatic_IsProximityEnabled();
    if (cVar1 != '\0') {
      local_9 = (ulonglong)
                ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
                *(longlong *)(g_proxData + 0x20)) < 3000 || *(char *)(g_proxData + 1) != '\0';
      if (((*(char *)(g_proxData + 1) != '\0') && ((*(uint *)(g_proxData + 0x54) & 0x40) != 0)) &&
         (cVar1 = SensorGesture_IsProximityPutDownGestureTimeOut
                            (*(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x608), cVar1 == '\0'))
      {
        local_9 = true;
      }
      if (((*(char *)(g_proxData + 1) == '\0') && ((*(uint *)(g_proxData + 0x54) & 0x20) != 0)) &&
         (cVar1 = SensorGesture_IsProximityPickUpGestureTimeOut
                            (*(undefined4 *)&_refptr_g_tsaPrmtFlash[2].bRawdataForceNormalize),
         cVar1 == '\0')) {
        local_9 = true;
      }
      cVar1 = Proximity_IsGestureEnabled();
      if ((cVar1 == '\0') && ((*(uint *)(g_proxData + 0x54) & 0x10) != 0)) {
        local_9 = true;
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                          "../../src/Extension/Proximity/Proximity.c",0x1a6,&DAT_6bb581bf);
        HardwareAnalyzer_SetSoftException();
      }
    }
  }
  return local_9;
}


// ============================================================================
// Function: Proximity_Init @ 6baf84e0
// ============================================================================

void Proximity_Init(void)

{
  int iVar1;
  
                    /* 0xb84e0  1098  Proximity_Init */
  if (g_proxData != 0) {
    iVar1 = memset_s(g_proxData,0x68,0,0x68);
    if (iVar1 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/Proximity/Proximity.c",0x37,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
  }
  SensorGesture_ClearExternally(6);
  return;
}


// ============================================================================
// Function: Proximity_IsScreenOffTimeOut @ 6baf8da2
// ============================================================================

undefined1 Proximity_IsScreenOffTimeOut(void)

{
  char cVar1;
  undefined1 local_9;
  
                    /* 0xb8da2  1108  Proximity_IsScreenOffTimeOut */
  cVar1 = Proximity_IsEnabled();
  if (cVar1 == '\0') {
    local_9 = 0;
  }
  else {
    local_9 = 0;
    if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x400000) == 0) {
      local_9 = 0;
    }
    else if (((*_refptr_g_tsaStaticPtr)->nField_0xc0 & 0x1000000U) == 0) {
      local_9 = 0;
    }
    else {
      if ((400 < (ulonglong)
                 ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
                 *(longlong *)(g_proxData + 0x18))) &&
         ((ulonglong)
          ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - *(longlong *)(g_proxData + 0x18))
          < 10000)) {
        local_9 = 1;
      }
      if ((((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x5e0 & 0x8000) == 0) &&
          (*(char *)(g_proxData + 1) == '\0')) &&
         ((ulonglong)*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x610 <=
          (ulonglong)
          ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
          *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x70))) {
        *(undefined1 *)(g_proxData + 1) = 1;
      }
    }
  }
  return local_9;
}


// ============================================================================
// Function: Proximity_AppendScreenOffTimeOutDescription @ 6baf929b
// ============================================================================

void Proximity_AppendScreenOffTimeOutDescription(void)

{
  char cVar1;
  
                    /* 0xb929b  1074  Proximity_AppendScreenOffTimeOutDescription */
  cVar1 = Proximity_IsEnabled();
  if (cVar1 != '\0') {
    HardwareAnalyzer_AppendExceptionDescription
              ("ProxOnDrvTime",
               (int)(*_refptr_g_tsaStaticPtr)->llField_0xe8 -
               (int)*(undefined8 *)(g_proxData + 0x18));
    HardwareAnalyzer_AppendExceptionDescription
              ("ProxOnAFETime",
               (int)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
               (int)(*_refptr_g_tsaStaticPtr)->llField_0xe8);
  }
  return;
}


// ============================================================================
// Function: Proximity_IsScreenOnTimeOut @ 6baf931b
// ============================================================================

undefined1 Proximity_IsScreenOnTimeOut(void)

{
  char cVar1;
  undefined1 local_9;
  
                    /* 0xb931b  1109  Proximity_IsScreenOnTimeOut */
  cVar1 = Proximity_IsEnabled();
  if (cVar1 == '\0') {
    local_9 = 0;
  }
  else {
    local_9 = 0;
    if ((((((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x400000) != 0) &&
         (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x2000000) != 0)) &&
        (500 < (ulonglong)
               ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
               *(longlong *)(g_proxData + 0x20)))) &&
       ((ulonglong)
        ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - *(longlong *)(g_proxData + 0x20)) <
        5000)) {
      local_9 = 1;
    }
  }
  return local_9;
}


// ============================================================================
// Function: Proximity_AppendScreenOnTimeOutDescription @ 6baf93ce
// ============================================================================

void Proximity_AppendScreenOnTimeOutDescription(void)

{
  char cVar1;
  
                    /* 0xb93ce  1075  Proximity_AppendScreenOnTimeOutDescription */
  cVar1 = Proximity_IsEnabled();
  if (cVar1 != '\0') {
    HardwareAnalyzer_AppendExceptionDescription
              ("ProxOffDrvTime",
               (int)(*_refptr_g_tsaStaticPtr)->llField_0xe8 -
               (int)*(undefined8 *)(g_proxData + 0x20));
    HardwareAnalyzer_AppendExceptionDescription
              ("ProxOffAFETime",
               (int)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
               (int)(*_refptr_g_tsaStaticPtr)->llField_0xe8);
  }
  return;
}


// ============================================================================
// Function: Proximity_IsPowerKeyPressedToScreenOn @ 6baf944e
// ============================================================================

undefined1 Proximity_IsPowerKeyPressedToScreenOn(void)

{
  char cVar1;
  undefined1 local_9;
  
                    /* 0xb944e  1105  Proximity_IsPowerKeyPressedToScreenOn */
  cVar1 = Proximity_IsEnabled();
  if (cVar1 == '\0') {
    local_9 = 0;
  }
  else {
    local_9 = 0;
    if ((((((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x400000) != 0) &&
         (*(char *)(g_proxData + 2) != '\0')) &&
        (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x2000000) != 0)) &&
       (cVar1 = Proximity_IsScreenOnTimeOut(), cVar1 == '\0')) {
      local_9 = 1;
    }
  }
  return local_9;
}


// ============================================================================
// Function: Proximity_AppendPowerKeyPressedToScreenOnDescription @ 6baf94c2
// ============================================================================

void Proximity_AppendPowerKeyPressedToScreenOnDescription(void)

{
  char cVar1;
  
                    /* 0xb94c2  1073  Proximity_AppendPowerKeyPressedToScreenOnDescription */
  cVar1 = Proximity_IsEnabled();
  if (cVar1 != '\0') {
    HardwareAnalyzer_AppendExceptionDescription
              ("ProxOnDuration",
               (int)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
               (int)*(undefined8 *)(g_proxData + 0x18));
  }
  return;
}


// ============================================================================
// Function: Proximity_BaselineReset @ 6bafb9f3
// ============================================================================

void Proximity_BaselineReset(void)

{
  errno_t eVar1;
  int iVar2;
  
                    /* 0xbb9f3  1077  Proximity_BaselineReset */
  eVar1 = memcpy_s(*(void **)_refptr_g_tsaBufProxBL,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2,
                   *(void **)_refptr_g_tsaBufRaw,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/Proximity/ProximityBaseline.c",0x31,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  eVar1 = memcpy_s(*(void **)_refptr_g_tsaBufProxIIRRaw,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2,
                   *(void **)_refptr_g_tsaBufRaw,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/Proximity/ProximityBaseline.c",0x34,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar2 = memset_s(*(undefined8 *)_refptr_g_tsaBufProxDif,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2,0,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  if (iVar2 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/Proximity/ProximityBaseline.c",0x37,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: Proximity_CreateData @ 6baf869d
// ============================================================================

undefined8 Proximity_CreateData(void)

{
  undefined8 uVar1;
  void *pvVar2;
  
                    /* 0xb869d  1080  Proximity_CreateData */
  if (g_proxData == (void *)0x0) {
    pvVar2 = malloc(0x68);
    if (pvVar2 == (void *)0x0) {
      uVar1 = 0;
    }
    else {
      g_proxData = pvVar2;
      pvVar2 = malloc(0x1d60);
      if (pvVar2 == (void *)0x0) {
        uVar1 = 0;
      }
      else {
        g_touchesProxPtr = pvVar2;
        pvVar2 = malloc(0x5aa0);
        if (pvVar2 == (void *)0x0) {
          uVar1 = 0;
        }
        else {
          g_prevTouchesProxPtr = pvVar2;
          pvVar2 = malloc((ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount);
          if (pvVar2 == (void *)0x0) {
            uVar1 = 0;
          }
          else {
            g_tsaBufTzProx = pvVar2;
            pvVar2 = malloc((ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
            if (pvVar2 == (void *)0x0) {
              uVar1 = 0;
            }
            else {
              g_tsaBufProxDif = pvVar2;
              pvVar2 = malloc((ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
              if (pvVar2 == (void *)0x0) {
                uVar1 = 0;
              }
              else {
                g_tsaBufProxIIRRaw = pvVar2;
                pvVar2 = malloc((ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
                if (pvVar2 == (void *)0x0) {
                  uVar1 = 0;
                }
                else {
                  g_tsaBufProxZnDif = pvVar2;
                  pvVar2 = malloc((ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
                  if (pvVar2 == (void *)0x0) {
                    uVar1 = 0;
                  }
                  else {
                    g_tsaBufProxBL = pvVar2;
                    pvVar2 = malloc((longlong)
                                    (int)((uint)_refptr_g_tsaPrmtFlash->bRows +
                                         (uint)_refptr_g_tsaPrmtFlash->bCols) << 2);
                    if (pvVar2 == (void *)0x0) {
                      uVar1 = 0;
                    }
                    else {
                      g_tsaBufProxSelf = pvVar2;
                      Proximity_Init();
                      Proximity_Reset();
                      uVar1 = 1;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: Proximity_DeleteData @ 6baf88ae
// ============================================================================

void Proximity_DeleteData(void)

{
                    /* 0xb88ae  1082  Proximity_DeleteData */
  if (g_tsaBufTzProx != (void *)0x0) {
    free(g_tsaBufTzProx);
    g_tsaBufTzProx = (void *)0x0;
  }
  if (g_touchesProxPtr != (void *)0x0) {
    free(g_touchesProxPtr);
    g_touchesProxPtr = (void *)0x0;
  }
  if (g_prevTouchesProxPtr != (void *)0x0) {
    free(g_prevTouchesProxPtr);
    g_prevTouchesProxPtr = (void *)0x0;
  }
  if (g_tsaBufProxDif != (void *)0x0) {
    free(g_tsaBufProxDif);
    g_tsaBufProxDif = (void *)0x0;
  }
  if (g_tsaBufProxIIRRaw != (void *)0x0) {
    free(g_tsaBufProxIIRRaw);
    g_tsaBufProxIIRRaw = (void *)0x0;
  }
  if (g_tsaBufProxZnDif != (void *)0x0) {
    free(g_tsaBufProxZnDif);
    g_tsaBufProxZnDif = (void *)0x0;
  }
  if (g_tsaBufProxBL != (void *)0x0) {
    free(g_tsaBufProxBL);
    g_tsaBufProxBL = (void *)0x0;
  }
  if (g_proxData != (void *)0x0) {
    free(g_proxData);
    g_proxData = (void *)0x0;
  }
  if (g_tsaBufProxSelf != (void *)0x0) {
    free(g_tsaBufProxSelf);
    g_tsaBufProxSelf = (void *)0x0;
  }
  return;
}


// ============================================================================
// Function: Proximity_DynamicThreshold @ 6baf97af
// ============================================================================

void Proximity_DynamicThreshold(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int local_10;
  int local_c;
  
                    /* 0xb97af  1087  Proximity_DynamicThreshold */
  if (*(char *)(g_proxData + 1) == '\0') {
    if (((ulonglong)
         ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - *(longlong *)(g_proxData + 0x20)) <
         0x1f5) && (*(longlong *)(g_proxData + 0x20) != -1)) {
      local_10 = (*(int *)(g_proxData + 0x3c) << 4) /
                 (int)(uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x5f0;
    }
    else {
      local_10 = *(int *)&_refptr_g_tsaPrmtFlash[2].field_0x5e4;
    }
    *(int *)(g_proxData + 0x30) = local_10;
  }
  else {
    iVar2 = (*(int *)&_refptr_g_tsaPrmtFlash[2].field_0x5ec << 6) /
            *(int *)&_refptr_g_tsaPrmtFlash[2].field_0x5e4;
    iVar3 = (*(int *)(g_proxData + 0xc) << 4) / *(int *)&_refptr_g_tsaPrmtFlash[2].field_0x5e4;
    iVar1 = *(int *)&_refptr_g_tsaPrmtFlash[2].field_0x5e8;
    local_c = *(int *)&_refptr_g_tsaPrmtFlash[2].field_0x5ec;
    if (iVar3 < 0x21) {
      local_c = iVar1;
    }
    else if (iVar3 < iVar2) {
      local_c = iVar1 + ((local_c - iVar1) * (iVar3 + -0x20)) / (iVar2 + -0x20);
    }
    *(int *)(g_proxData + 0x3c) = local_c;
  }
  return;
}


// ============================================================================
// Function: Proximity_ZnProcess @ 6baf9ba1
// ============================================================================

/* WARNING: Function: ___chkstk_ms replaced with injection: alloca_probe */

void Proximity_ZnProcess(void *param_1,longlong param_2)

{
  undefined8 uVar1;
  errno_t eVar2;
  int iVar3;
  undefined1 local_2668 [9812];
  int local_14;
  undefined8 local_10;
  
                    /* 0xb9ba1  1128  Proximity_ZnProcess */
  local_10 = 0x6baf9bac;
  eVar2 = memcpy_s(local_2668,0x2648,param_1,(ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2
                  );
  local_10 = CONCAT44(local_10._4_4_,eVar2);
  if (eVar2 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/Proximity/Proximity.c",0x30f,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  *(undefined4 *)(g_proxData + 0x44) = 0;
  *(undefined2 *)(g_proxData + 0x40) = 0;
  *(undefined2 *)(g_proxData + 0x42) = 0;
  *(undefined4 *)(g_proxData + 0x48) = 0x80000000;
  local_10._0_7_ = (uint7)local_10 & 0xffffffff;
  local_10 = (ulonglong)(uint7)local_10;
  while (local_10._7_1_ < _refptr_g_tsaPrmtFlash->bRows) {
    local_10._0_7_ = (uint7)(uint6)local_10;
    while( true ) {
      if ((_refptr_g_tsaPrmtFlash->bCols <= local_10._6_1_) || (0x1323 < local_10._4_2_)) break;
      local_14 = Proximity_ZnProcessUnitGetZ9
                           (local_10._6_1_,local_10._7_1_,local_10._4_2_,local_2668,0x1324);
      uVar1 = local_10;
      *(short *)(param_2 + (ulonglong)local_10._4_2_ * 2) = (short)local_14;
      *(int *)(g_proxData + 0x44) = local_14 + *(int *)(g_proxData + 0x44);
      iVar3 = *(int *)&_refptr_g_tsaPrmtFlash[2].field_0x5e4;
      if (iVar3 < 0) {
        iVar3 = iVar3 + 3;
      }
      if (iVar3 >> 2 < local_14) {
        *(short *)(g_proxData + 0x40) = *(short *)(g_proxData + 0x40) + 1;
      }
      iVar3 = -*(int *)&_refptr_g_tsaPrmtFlash[2].field_0x5e4;
      if (iVar3 < 0) {
        iVar3 = iVar3 + 3;
      }
      if (local_14 < iVar3 >> 2) {
        *(short *)(g_proxData + 0x42) = *(short *)(g_proxData + 0x42) + 1;
      }
      if (*(int *)(g_proxData + 0x48) < local_14) {
        *(int *)(g_proxData + 0x48) = local_14;
      }
      local_10._0_6_ = CONCAT24(local_10._4_2_ + 1,(undefined4)local_10);
      local_10._6_1_ = SUB81(uVar1,6);
      local_10._7_1_ = SUB81(uVar1,7);
      local_10._0_7_ = CONCAT16(local_10._6_1_ + '\x01',(uint6)local_10);
    }
    local_10 = CONCAT17(local_10._7_1_ + '\x01',(uint7)local_10);
  }
  return;
}


// ============================================================================
// Function: Proximity_DifProcess @ 6baf9f32
// ============================================================================

void Proximity_DifProcess(void)

{
  ushort uVar1;
  short sVar2;
  byte local_a;
  byte local_9;
  
                    /* 0xb9f32  1085  Proximity_DifProcess */
  *(undefined4 *)(g_proxData + 0x5c) = 0;
  *(undefined4 *)(g_proxData + 0x60) = 0;
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    for (local_9 = 0; local_9 < (byte)_refptr_g_tsaPrmtFlash[2].field_0x60c; local_9 = local_9 + 1)
    {
      for (local_a = 1; (int)(uint)local_a < (int)(_refptr_g_tsaPrmtFlash->bCols - 1);
          local_a = local_a + 1) {
        uVar1 = (ushort)local_a + (ushort)local_9 * (ushort)_refptr_g_tsaPrmtFlash->bCols;
        if ((*_refptr_g_tsaStaticPtr)->bField_0x11a == '\0') {
          sVar2 = *(short *)(g_tsaBufProxBL + (ulonglong)uVar1 * 2) -
                  *(short *)(g_tsaBufProxIIRRaw + (ulonglong)uVar1 * 2);
        }
        else {
          sVar2 = *(short *)(g_tsaBufProxIIRRaw + (ulonglong)uVar1 * 2) -
                  *(short *)(g_tsaBufProxBL + (ulonglong)uVar1 * 2);
        }
        *(int *)(g_proxData + 0x60) = (int)sVar2 + *(int *)(g_proxData + 0x60);
        *(short *)((ulonglong)uVar1 * 2 + g_tsaBufProxDif) = sVar2;
        if ((*_refptr_g_tsaStaticPtr)->bField_0x11a == '\0') {
          sVar2 = *(short *)(g_tsaBufProxBL + (ulonglong)uVar1 * 2) -
                  *(short *)(*(longlong *)_refptr_g_tsaBufRaw + (ulonglong)uVar1 * 2);
        }
        else {
          sVar2 = *(short *)(*(longlong *)_refptr_g_tsaBufRaw + (ulonglong)uVar1 * 2) -
                  *(short *)(g_tsaBufProxBL + (ulonglong)uVar1 * 2);
        }
        *(int *)(g_proxData + 0x5c) = (int)sVar2 + *(int *)(g_proxData + 0x5c);
      }
    }
  }
  else {
    for (local_9 = 1; (int)(uint)local_9 < (int)(_refptr_g_tsaPrmtFlash->bRows - 1);
        local_9 = local_9 + 1) {
      for (local_a = 0; local_a < (byte)_refptr_g_tsaPrmtFlash[2].field_0x60c; local_a = local_a + 1
          ) {
        uVar1 = (ushort)local_a + (ushort)local_9 * (ushort)_refptr_g_tsaPrmtFlash->bCols;
        if ((*_refptr_g_tsaStaticPtr)->bField_0x11a == '\0') {
          sVar2 = *(short *)(g_tsaBufProxBL + (ulonglong)uVar1 * 2) -
                  *(short *)(g_tsaBufProxIIRRaw + (ulonglong)uVar1 * 2);
        }
        else {
          sVar2 = *(short *)(g_tsaBufProxIIRRaw + (ulonglong)uVar1 * 2) -
                  *(short *)(g_tsaBufProxBL + (ulonglong)uVar1 * 2);
        }
        *(int *)(g_proxData + 0x60) = (int)sVar2 + *(int *)(g_proxData + 0x60);
        *(short *)((ulonglong)uVar1 * 2 + g_tsaBufProxDif) = sVar2;
        if ((*_refptr_g_tsaStaticPtr)->bField_0x11a == '\0') {
          sVar2 = *(short *)(g_tsaBufProxBL + (ulonglong)uVar1 * 2) -
                  *(short *)(*(longlong *)_refptr_g_tsaBufRaw + (ulonglong)uVar1 * 2);
        }
        else {
          sVar2 = *(short *)(*(longlong *)_refptr_g_tsaBufRaw + (ulonglong)uVar1 * 2) -
                  *(short *)(g_tsaBufProxBL + (ulonglong)uVar1 * 2);
        }
        *(int *)(g_proxData + 0x5c) = (int)sVar2 + *(int *)(g_proxData + 0x5c);
      }
    }
  }
  return;
}


// ============================================================================
// Function: Proximity_RawProcess @ 6bafaf11
// ============================================================================

void Proximity_RawProcess(void)

{
  char cVar1;
  
                    /* 0xbaf11  1120  Proximity_RawProcess */
  cVar1 = Proximity_IsDetected();
  if (cVar1 == '\0') {
    if ((*(uint *)(g_proxData + 0x54) & 0x4000) == 0) {
      Proximity_AdaptiveIIR
                (*(undefined8 *)_refptr_g_tsaBufRaw,g_tsaBufProxIIRRaw,g_tsaBufProxIIRRaw,
                 _refptr_g_tsaPrmtFlash[2].field_0x5f6);
    }
    else {
      GridIIR_ProcessCoreRaw
                (*(undefined8 *)_refptr_g_tsaBufRaw,g_tsaBufProxIIRRaw,g_tsaBufProxIIRRaw,
                 _refptr_g_tsaPrmtFlash->wBufElementCount,_refptr_g_tsaPrmtFlash[2].field_0x5f6);
    }
  }
  else if ((*(uint *)(g_proxData + 0x54) & 0x4000) == 0) {
    Proximity_AdaptiveIIR
              (*(undefined8 *)_refptr_g_tsaBufRaw,g_tsaBufProxIIRRaw,g_tsaBufProxIIRRaw,
               _refptr_g_tsaPrmtFlash[2].field_0x5f7);
  }
  else {
    GridIIR_ProcessCoreRaw
              (*(undefined8 *)_refptr_g_tsaBufRaw,g_tsaBufProxIIRRaw,g_tsaBufProxIIRRaw,
               _refptr_g_tsaPrmtFlash->wBufElementCount,_refptr_g_tsaPrmtFlash[2].field_0x5f7);
  }
  return;
}


// ============================================================================
// Function: Proximity_BaselineProcess @ 6bafbbe9
// ============================================================================

void Proximity_BaselineProcess(void)

{
  char cVar1;
  
                    /* 0xbbbe9  1076  Proximity_BaselineProcess */
  if (*(char *)(*(longlong *)_refptr_g_proxData + 1) == '\0') {
    cVar1 = Proximity_IsToResetBaseline();
    if (cVar1 == '\0') {
      Proximity_BaselineUpdate();
    }
    else {
      Proximity_BaselineReset();
    }
  }
  return;
}


// ============================================================================
// Function: Proximity_SelfCalc @ 6bafc1f7
// ============================================================================

void Proximity_SelfCalc(void)

{
  ushort local_a;
  
                    /* 0xbc1f7  1122  Proximity_SelfCalc */
  if ((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x5e0 & 2) == 0) {
    for (local_a = 0;
        local_a < (ushort)((ushort)_refptr_g_tsaPrmtFlash->bRows +
                          (ushort)_refptr_g_tsaPrmtFlash->bCols); local_a = local_a + 1) {
      *(undefined4 *)(*(longlong *)_refptr_g_tsaBufProxSelf + (ulonglong)local_a * 4) =
           *(undefined4 *)(_refptr_g_tsaBufSSDif + (longlong)(int)(uint)local_a * 4);
    }
  }
  else {
    Proximity_SelfCalcDim(1);
    Proximity_SelfCalcDim(2);
  }
  Proximity_SelfToMutual();
  return;
}


// ============================================================================
// Function: Proximity_ProcessInternal @ 6bafb1d2
// ============================================================================

void Proximity_ProcessInternal(void)

{
  longlong lVar1;
  char cVar2;
  undefined8 uVar3;
  
                    /* 0xbb1d2  1119  Proximity_ProcessInternal */
  lVar1 = g_proxData;
  uVar3 = TSAStatic_GetCurrTimeStamp();
  *(undefined8 *)(lVar1 + 0x28) = uVar3;
  if (*(char *)(g_proxData + 6) != '\0') {
    *(undefined1 *)(g_proxData + 5) = 0;
    *(undefined1 *)(g_proxData + 3) = 0;
    *(undefined1 *)(g_proxData + 4) = 0;
    if ((*(uint *)(g_proxData + 0x54) & 8) != 0) {
      cVar2 = Proximity_DetectBasedOnTouch();
      if (cVar2 != '\0') {
        *(undefined1 *)(g_proxData + 5) = 1;
        *(undefined1 *)(g_proxData + 3) = 1;
      }
    }
    if ((*(uint *)(g_proxData + 0x54) & 0x10) != 0) {
      cVar2 = Proximity_HoverProcess();
      if (cVar2 != '\0') {
        *(undefined1 *)(g_proxData + 5) = 1;
        *(undefined1 *)(g_proxData + 4) = 1;
      }
    }
    Proximity_Debounce();
    *(undefined1 *)(g_proxData + 2) = *(undefined1 *)(g_proxData + 5);
  }
  return;
}


// ============================================================================
// Function: Proximity_IsGestureEnabled @ 6baf8f3d
// ============================================================================

bool Proximity_IsGestureEnabled(void)

{
  char cVar1;
  char cVar2;
  bool local_9;
  
                    /* 0xb8f3d  1102  Proximity_IsGestureEnabled */
  cVar1 = SensorGesture_IsProximityPickUpGestureEnabled();
  cVar2 = SensorGesture_IsProximityPutDownGestureEnabled();
  local_9 = cVar2 != '\0' && cVar1 != '\0';
  if ((*(uint *)(g_proxData + 0x54) & 0x60) == 0) {
    local_9 = false;
  }
  return local_9;
}


// ============================================================================
// Function: Proximity_GetNextScanRate @ 6baf8b40
// ============================================================================

undefined2 Proximity_GetNextScanRate(void)

{
  char cVar1;
  undefined2 uVar2;
  
                    /* 0xb8b40  1093  Proximity_GetNextScanRate */
  cVar1 = Proximity_IsEnabled();
  if (cVar1 == '\0') {
    uVar2 = 1;
  }
  else {
    Proximity_HandleNextScanRate();
    uVar2 = *(undefined2 *)(g_proxData + 0x50);
  }
  return uVar2;
}


// ============================================================================
// Function: Proximity_ZnProcessUnitGetZ9 @ 6baf99bb
// ============================================================================

int Proximity_ZnProcessUnitGetZ9(byte param_1,byte param_2,ushort param_3,longlong param_4)

{
  byte bVar1;
  ushort uVar2;
  int local_c;
  
                    /* 0xb99bb  1129  Proximity_ZnProcessUnitGetZ9 */
  bVar1 = _refptr_g_tsaPrmtFlash->bCols - 1;
  local_c = (int)*(short *)(param_4 + (ulonglong)param_3 * 2);
  if (param_2 < (byte)(_refptr_g_tsaPrmtFlash->bRows - 1)) {
    uVar2 = param_3 + _refptr_g_tsaPrmtFlash->bCols;
    if (param_1 < bVar1) {
      local_c = local_c + *(short *)(param_4 + (ulonglong)(ushort)(uVar2 + 1) * 2);
    }
    local_c = local_c + *(short *)(param_4 + (ulonglong)uVar2 * 2);
    if (param_1 != 0) {
      local_c = local_c + *(short *)(param_4 + (ulonglong)(ushort)(uVar2 - 1) * 2);
    }
  }
  if ((param_1 < bVar1) && (param_3 < 0x1323)) {
    local_c = local_c + *(short *)(param_4 + ((ulonglong)param_3 + 1) * 2);
  }
  if (param_1 != 0) {
    local_c = local_c + *(short *)(param_4 + (ulonglong)param_3 * 2 + -2);
  }
  if (param_2 != 0) {
    param_3 = param_3 - _refptr_g_tsaPrmtFlash->bCols;
    if (param_1 < bVar1) {
      local_c = local_c + *(short *)(param_4 + (ulonglong)(ushort)(param_3 + 1) * 2);
    }
    local_c = local_c + *(short *)(param_4 + (ulonglong)param_3 * 2);
    if (param_1 != 0) {
      local_c = local_c + *(short *)(param_4 + (ulonglong)(ushort)(param_3 - 1) * 2);
    }
  }
  if (0x7fff < local_c) {
    local_c = 0x7fff;
  }
  if (local_c < -0x8000) {
    local_c = -0x8000;
  }
  return local_c;
}


// ============================================================================
// Function: Proximity_AdaptiveIIR @ 6bafae26
// ============================================================================

void Proximity_AdaptiveIIR(longlong param_1,longlong param_2,longlong param_3,undefined1 param_4)

{
  undefined2 uVar1;
  ushort local_a;
  
                    /* 0xbae26  1072  Proximity_AdaptiveIIR */
  for (local_a = 0; local_a < _refptr_g_tsaPrmtFlash->wBufElementCount; local_a = local_a + 1) {
    uVar1 = AdaptiveIIR_Process(*(undefined2 *)(param_1 + (ulonglong)local_a * 2),
                                *(undefined2 *)(param_3 + (ulonglong)local_a * 2),param_4,8,
                                *(int *)&_refptr_g_tsaPrmtFlash[2].field_0x5e4 /
                                (int)(uint)_refptr_g_tsaPrmtFlash->bCols);
    *(undefined2 *)(param_2 + (ulonglong)local_a * 2) = uVar1;
    *(undefined2 *)((ulonglong)local_a * 2 + param_3) =
         *(undefined2 *)(param_2 + (ulonglong)local_a * 2);
  }
  return;
}


// ============================================================================
// Function: Proximity_BaselineUpdate @ 6bafb7d0
// ============================================================================

void Proximity_BaselineUpdate(void)

{
  short sVar1;
  ushort local_e;
  short local_c;
  ushort local_a;
  
                    /* 0xbb7d0  1078  Proximity_BaselineUpdate */
  local_a = *(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x5f8;
  local_c = *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x5fa;
  if ((*(char *)(*(longlong *)_refptr_g_proxData + 0x58) != '\0') &&
     ((*(uint *)(*(longlong *)_refptr_g_proxData + 0x54) & 0x1000) == 0)) {
    local_a = local_a >> 1;
    local_c = local_c * 2;
  }
  if (g_blUpdateSpeed < local_a) {
    if (((*_refptr_g_tsaStaticPtr)[1].field_0x2 == '\0') &&
       (*(int *)(*(longlong *)_refptr_g_proxData + 0x48) <
        *(int *)&_refptr_g_tsaPrmtFlash[2].field_0x5e4)) {
      g_blUpdateSpeed = g_blUpdateSpeed + 1;
    }
  }
  else {
    for (local_e = 0; local_e < _refptr_g_tsaPrmtFlash->wBufElementCount; local_e = local_e + 1) {
      if ((*_refptr_g_tsaStaticPtr)->bField_0x11a == '\0') {
        sVar1 = *(short *)(*(longlong *)_refptr_g_tsaBufProxBL + (ulonglong)local_e * 2) -
                *(short *)(*(longlong *)_refptr_g_tsaBufProxIIRRaw + (ulonglong)local_e * 2);
      }
      else {
        sVar1 = *(short *)(*(longlong *)_refptr_g_tsaBufProxIIRRaw + (ulonglong)local_e * 2) -
                *(short *)(*(longlong *)_refptr_g_tsaBufProxBL + (ulonglong)local_e * 2);
      }
      if (sVar1 < 1) {
        *(undefined2 *)((ulonglong)local_e * 2 + *(longlong *)_refptr_g_tsaBufProxBL) =
             *(undefined2 *)(*(longlong *)_refptr_g_tsaBufProxIIRRaw + (ulonglong)local_e * 2);
      }
      else if (sVar1 < local_c) {
        if ((*_refptr_g_tsaStaticPtr)->bField_0x11a == '\0') {
          sVar1 = *(short *)(*(longlong *)_refptr_g_tsaBufProxBL + (ulonglong)local_e * 2) + -1;
        }
        else {
          sVar1 = *(short *)(*(longlong *)_refptr_g_tsaBufProxBL + (ulonglong)local_e * 2) + 1;
        }
        *(short *)((ulonglong)local_e * 2 + *(longlong *)_refptr_g_tsaBufProxBL) = sVar1;
      }
    }
    g_blUpdateSpeed = 0;
  }
  return;
}


// ============================================================================
// Function: Proximity_IsToResetBaseline @ 6bafbb75
// ============================================================================

undefined8 Proximity_IsToResetBaseline(void)

{
  longlong lVar1;
  
                    /* 0xbbb75  1114  Proximity_IsToResetBaseline */
  if (((*(longlong *)(*(longlong *)_refptr_g_proxData + 0x20) != -1) &&
      (lVar1 = TSAStatic_GetCurrTimeStamp(),
      (ulonglong)(lVar1 - *(longlong *)(*(longlong *)_refptr_g_proxData + 0x20)) < 500)) &&
     (lVar1 = TSAStatic_GetCurrTimeStamp(),
     100 < (ulonglong)(lVar1 - *(longlong *)(*(longlong *)_refptr_g_proxData + 0x20)))) {
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: Proximity_SelfCalcDim @ 6bafbca1
// ============================================================================

void Proximity_SelfCalcDim(int param_1)

{
  byte local_10;
  uchar local_f;
  ushort local_e;
  byte local_b;
  uchar local_a;
  byte local_9;
  
                    /* 0xbbca1  1123  Proximity_SelfCalcDim */
  if (param_1 == 1) {
    local_9 = _refptr_g_tsaPrmtFlash->bRows;
    local_a = _refptr_g_tsaPrmtFlash->bCols;
    local_f = '\x01';
    local_10 = _refptr_g_tsaPrmtFlash->bCols;
    local_e = (ushort)_refptr_g_tsaPrmtFlash->bCols;
  }
  else {
    local_9 = _refptr_g_tsaPrmtFlash->bCols;
    local_a = _refptr_g_tsaPrmtFlash->bRows;
    local_f = _refptr_g_tsaPrmtFlash->bCols;
    local_10 = 0;
    local_e = 1;
  }
  for (local_b = 0; local_b < local_9; local_b = local_b + 1) {
    Proximity_SelfCalculation(local_b * local_e,local_f,local_a,(uint)local_b + (uint)local_10);
  }
  return;
}


// ============================================================================
// Function: Proximity_SelfToMutual @ 6bafbe7e
// ============================================================================

void Proximity_SelfToMutual(void)

{
  uchar uVar1;
  int iVar2;
  int local_10;
  byte local_a;
  byte local_9;
  
                    /* 0xbbe7e  1125  Proximity_SelfToMutual */
  if (_refptr_g_tsaPrmtFlash->bRows <= _refptr_g_tsaPrmtFlash->bCols) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/Proximity/ProximitySelf.c",0x4f,
                      "TSAPrmt(sys, dim1Len) < TSAPrmt(sys, dim2Len)");
    HardwareAnalyzer_SetSoftException();
  }
  iVar2 = memset_s(*(undefined8 *)_refptr_g_tsaBufProxZnDif,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2,0,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  if (iVar2 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/Proximity/ProximitySelf.c",0x54,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  *(undefined4 *)(*(longlong *)_refptr_g_proxData + 0x44) = 0;
  *(undefined2 *)(*(longlong *)_refptr_g_proxData + 0x40) = 0;
  *(undefined2 *)(*(longlong *)_refptr_g_proxData + 0x42) = 0;
  *(undefined4 *)(*(longlong *)_refptr_g_proxData + 0x48) = 0x80000000;
  uVar1 = _refptr_g_tsaPrmtFlash->bRows;
  for (local_9 = 1; (int)(uint)local_9 < (int)(_refptr_g_tsaPrmtFlash->bCols - 1);
      local_9 = local_9 + 1) {
    local_10 = *(int *)((longlong)
                        (int)((uint)((int)(uint)local_9 < (int)(_refptr_g_tsaPrmtFlash->bCols - 2))
                             + (uint)local_9) * 4 + *(longlong *)_refptr_g_tsaBufProxSelf) +
               *(int *)(*(longlong *)_refptr_g_tsaBufProxSelf +
                       (longlong)(int)((uint)local_9 - (uint)(1 < local_9)) * 4) +
               *(int *)(*(longlong *)_refptr_g_tsaBufProxSelf + (ulonglong)local_9 * 4);
    if (_refptr_g_tsaPrmtFlash[2].field_0x60c == '\0') {
      if (_refptr_g_tsaPrmtFlash[2].field_0x60c == '\0') {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                          "../../src/Extension/Proximity/ProximitySelf.c",0x61,
                          "TSAPrmt(prox, tholdSensorAlongYDirection) != 0");
        HardwareAnalyzer_SetSoftException();
      }
    }
    else {
      local_10 = (int)(local_10 * (_refptr_g_tsaPrmtFlash->bCols - 2)) /
                 (int)(uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x60c;
    }
    Proximity_SelfToMutualUnit(local_9,uVar1 + 0xff,local_10);
  }
  uVar1 = _refptr_g_tsaPrmtFlash->bCols;
  for (local_a = 0; local_a < (byte)_refptr_g_tsaPrmtFlash[2].field_0x60c; local_a = local_a + 1) {
    Proximity_SelfToMutualUnit
              (uVar1 + 0xff,local_a,
               *(int *)((longlong)
                        (int)((uint)((int)(uint)local_a < (int)(_refptr_g_tsaPrmtFlash->bRows - 1))
                             + (uint)_refptr_g_tsaPrmtFlash->bCols + (uint)local_a) * 4 +
                       *(longlong *)_refptr_g_tsaBufProxSelf) +
               *(int *)((longlong)
                        (int)(((uint)_refptr_g_tsaPrmtFlash->bCols + (uint)local_a) -
                             (uint)(local_a != 0)) * 4 + *(longlong *)_refptr_g_tsaBufProxSelf) +
               *(int *)((longlong)(int)((uint)local_a + (uint)_refptr_g_tsaPrmtFlash->bCols) * 4 +
                       *(longlong *)_refptr_g_tsaBufProxSelf));
  }
  return;
}


// ============================================================================
// Function: Proximity_DetectBasedOnTouch @ 6bafa632
// ============================================================================

bool Proximity_DetectBasedOnTouch(void)

{
  uint uVar1;
  char cVar2;
  bool local_9;
  
                    /* 0xba632  1084  Proximity_DetectBasedOnTouch */
  *(undefined **)_refptr_g_touchesPtr = _refptr_g_touchesGrid;
  *_refptr_g_prevTouchesPtr = (PrevTouchArray *)_refptr_g_prevTouchesGrid;
  Touch_KeepPrevTouchWithExceptionExcluded();
  uVar1 = *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24);
  cVar2 = Proximity_DetectBasedOnHover(1);
  local_9 = cVar2 != '\0' || (uVar1 & 0x80) != 0;
  *(undefined8 *)_refptr_g_touchesPtr = g_touchesProxPtr;
  *_refptr_g_prevTouchesPtr = g_prevTouchesProxPtr;
  return local_9;
}


// ============================================================================
// Function: Proximity_Debounce @ 6bafad4e
// ============================================================================

void Proximity_Debounce(void)

{
                    /* 0xbad4e  1081  Proximity_Debounce */
  if (*(char *)(g_proxData + 1) == '\0') {
    Proximity_EnterDebounceJudge();
  }
  else {
    Proximity_ExitDebounceJudge();
  }
  return;
}


// ============================================================================
// Function: Proximity_HoverProcess @ 6bafb118
// ============================================================================

undefined1 Proximity_HoverProcess(void)

{
  undefined1 uVar1;
  char cVar2;
  undefined8 uVar3;
  longlong lVar4;
  
                    /* 0xbb118  1097  Proximity_HoverProcess */
  Proximity_DiffToTouch();
  uVar1 = Proximity_DetectBasedOnHover(0);
  lVar4 = g_proxData;
  if ((*(uint *)(*_refptr_g_prevTouchesPtr + 1) & 0x100) != 0) {
    uVar3 = TSAStatic_GetCurrTimeStamp();
    *(undefined8 *)(lVar4 + 0x10) = uVar3;
  }
  if (((*(longlong *)(g_proxData + 0x10) != -1) &&
      (lVar4 = TSAStatic_GetCurrTimeStamp(),
      (ulonglong)(lVar4 - *(longlong *)(g_proxData + 0x10)) < 0x1f5)) &&
     (cVar2 = Proximity_IsDetected(), cVar2 == '\0')) {
    lVar4 = TSAStatic_GetCurrTimeStamp();
    if ((lVar4 == *(longlong *)(g_proxData + 0x10)) &&
       (cVar2 = Proximity_IsDetected(), cVar2 == '\0')) {
      return uVar1;
    }
    return 0;
  }
  return uVar1;
}


// ============================================================================
// Function: Proximity_HandleNextScanRate @ 6baf8a99
// ============================================================================

void Proximity_HandleNextScanRate(void)

{
  char cVar1;
  undefined2 local_a;
  
                    /* 0xb8a99  1096  Proximity_HandleNextScanRate */
  cVar1 = TSAStatic_IsProximityEnabled();
  if (cVar1 == '\0') {
    local_a = 0;
  }
  else if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0x400000) == 0) {
    cVar1 = TSA_IsProximityDetected();
    if (cVar1 == '\0') {
      local_a = *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x600;
    }
    else {
      local_a = *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x602;
    }
  }
  else {
    cVar1 = TSA_IsProximityDetected();
    if (cVar1 == '\0') {
      local_a = *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x5fc;
    }
    else {
      local_a = *(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x5fe;
    }
  }
  *(undefined2 *)(g_proxData + 0x50) = local_a;
  return;
}


// ============================================================================
// Function: Proximity_IsValidTouch @ 6baf8b6e
// ============================================================================

undefined1 Proximity_IsValidTouch(byte param_1)

{
  char cVar1;
  byte bVar2;
  byte bVar3;
  ushort uVar4;
  undefined1 local_19;
  
                    /* 0xb8b6e  1116  Proximity_IsValidTouch */
  cVar1 = Proximity_IsEnabled();
  if (cVar1 == '\0') {
    local_19 = 0;
  }
  else {
    local_19 = 0;
    uVar4 = _refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold;
    if ((short)uVar4 < 0) {
      uVar4 = uVar4 + 3;
    }
    if ((short)uVar4 >> 2 <
        *(short *)(*_refptr_g_peaksPtr +
                  (longlong)
                  (int)(uint)*(byte *)((longlong)(int)(uint)param_1 * 0x168 +
                                       *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x18)) {
      if (*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1a2) < 10) {
        local_19 = 0;
      }
      else {
        bVar2 = TSA_RptTouchYCenter(param_1);
        bVar3 = TSA_GetYLength();
        if (bVar3 >> 3 < bVar2) {
          bVar2 = TSA_RptTouchYCenter(param_1);
          if (bVar2 <= (byte)_refptr_g_tsaPrmtFlash[2].field_0x60c) {
            if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
              if (0x100 < *(ushort *)
                           ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                           + 0x22c)) {
                local_19 = 1;
              }
            }
            else if (0x100 < *(ushort *)
                              ((longlong)(int)(uint)param_1 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x22e)) {
              local_19 = 1;
            }
          }
        }
        else {
          local_19 = 1;
        }
      }
    }
    else {
      local_19 = 0;
    }
  }
  return local_19;
}


// ============================================================================
// Function: Proximity_SelfCalculation @ 6bafbc20
// ============================================================================

void Proximity_SelfCalculation(ushort param_1,uint param_2,int param_3,uint param_4)

{
  short *local_18;
  int local_10;
  int local_c;
  
                    /* 0xbbc20  1124  Proximity_SelfCalculation */
  local_10 = 0;
  local_18 = (short *)(*(longlong *)_refptr_g_tsaBufProxDif + (ulonglong)param_1 * 2);
  local_c = param_3;
  while (local_c != 0) {
    local_10 = local_10 + *local_18;
    local_18 = local_18 + param_2;
    local_c = local_c + -1;
  }
  *(int *)((ulonglong)param_4 * 4 + *(longlong *)_refptr_g_tsaBufProxSelf) = local_10;
  return;
}


// ============================================================================
// Function: Proximity_SelfToMutualUnit @ 6bafbd75
// ============================================================================

void Proximity_SelfToMutualUnit(byte param_1,byte param_2,int param_3)

{
  int local_res18;
  
  local_res18 = param_3;
  if (0x7fff < param_3) {
    local_res18 = 0x7fff;
  }
  if (local_res18 < -0x8000) {
    local_res18 = -0x8000;
  }
  *(short *)((ulonglong)
             ((uint)param_1 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
            *(longlong *)_refptr_g_tsaBufProxZnDif) = (short)local_res18;
  *(int *)(*(longlong *)_refptr_g_proxData + 0x44) =
       local_res18 + *(int *)(*(longlong *)_refptr_g_proxData + 0x44);
  if (*(int *)&_refptr_g_tsaPrmtFlash[2].field_0x5e4 / 2 < local_res18) {
    *(short *)(*(longlong *)_refptr_g_proxData + 0x40) =
         *(short *)(*(longlong *)_refptr_g_proxData + 0x40) + 1;
  }
  if (local_res18 < -*(int *)&_refptr_g_tsaPrmtFlash[2].field_0x5e4 / 2) {
    *(short *)(*(longlong *)_refptr_g_proxData + 0x42) =
         *(short *)(*(longlong *)_refptr_g_proxData + 0x42) + 1;
  }
  if (*(int *)(*(longlong *)_refptr_g_proxData + 0x48) < local_res18) {
    *(int *)(*(longlong *)_refptr_g_proxData + 0x48) = local_res18;
  }
  return;
}


// ============================================================================
// Function: Proximity_DetectBasedOnHover @ 6bafa3a7
// ============================================================================

undefined1 Proximity_DetectBasedOnHover(char param_1)

{
  byte bVar1;
  byte bVar2;
  short local_1e;
  byte local_1b;
  byte local_1a;
  undefined1 local_19;
  
                    /* 0xba3a7  1083  Proximity_DetectBasedOnHover */
  local_19 = 0;
  local_1b = 0;
  local_1e = 0;
  for (local_1a = 0; local_1a < **(byte **)_refptr_g_touchesPtr; local_1a = local_1a + 1) {
    bVar1 = TSA_RptTouchYCenter(local_1a);
    bVar2 = TSA_GetYLength();
    if (bVar1 <= bVar2 >> 1) {
      if ((*(uint *)((longlong)(int)(uint)local_1a * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1f0) & 0x2000) != 0) {
        local_19 = 1;
      }
      if ((*(uint *)(g_proxData + 0x54) & 4) != 0) {
        if (((0x5dc < *(short *)(*_refptr_g_peaksPtr +
                                (longlong)
                                (int)(uint)*(byte *)((longlong)(int)(uint)local_1a * 0x168 +
                                                     *(longlong *)_refptr_g_touchesPtr + 0x23c) *
                                0x14 + 0x18)) &&
            (0xe < *(short *)((longlong)(int)(uint)local_1a * 0x168 +
                              *(longlong *)_refptr_g_touchesPtr + 0x1a0))) &&
           (0x13 < *(ushort *)
                    ((longlong)(int)(uint)local_1a * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x18a))) {
          local_19 = 1;
        }
        if (param_1 != '\0') {
          if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
            if (0x200 < *(ushort *)
                         ((longlong)(int)(uint)local_1a * 0x168 + *(longlong *)_refptr_g_touchesPtr
                         + 0x22c)) {
              local_1b = local_1b + 1;
            }
          }
          else if (0x200 < *(ushort *)
                            ((longlong)(int)(uint)local_1a * 0x168 +
                             *(longlong *)_refptr_g_touchesPtr + 0x22e)) {
            local_1b = local_1b + 1;
          }
          if (local_1e <
              *(short *)((longlong)(int)(uint)local_1a * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x1a0)) {
            local_1e = *(short *)((longlong)(int)(uint)local_1a * 0x168 +
                                  *(longlong *)_refptr_g_touchesPtr + 0x1a0);
          }
        }
      }
    }
  }
  *(byte *)(*(longlong *)_refptr_g_touchesPtr + 7) = local_1b;
  *(short *)(*(longlong *)_refptr_g_touchesPtr + 0x14) = local_1e;
  if (1 < local_1b) {
    if (0xe < *(short *)(*(longlong *)_refptr_g_touchesPtr + 0x14)) {
      local_19 = 1;
    }
    if (2 < local_1b) {
      local_19 = 1;
    }
  }
  return local_19;
}


// ============================================================================
// Function: Proximity_ExitDebounceJudge @ 6bafac2d
// ============================================================================

void Proximity_ExitDebounceJudge(void)

{
  byte *pbVar1;
  char cVar2;
  undefined8 uVar3;
  
                    /* 0xbac2d  1089  Proximity_ExitDebounceJudge */
  if (g_proxData[5] == 0) {
    if (*g_proxData < (byte)_refptr_g_tsaPrmtFlash[2].field_0x5f5) {
      cVar2 = Proximity_IsSignalIncTrend();
      if (cVar2 == '\0') {
        *g_proxData = *g_proxData + 1;
      }
    }
    else {
      g_proxData[1] = 0;
      *g_proxData = 0;
      pbVar1 = g_proxData;
      uVar3 = TSAStatic_GetCurrTimeStamp();
      *(undefined8 *)(pbVar1 + 0x20) = uVar3;
    }
  }
  else {
    *g_proxData = 0;
  }
  return;
}


// ============================================================================
// Function: Proximity_EnterDebounceJudge @ 6bafacb8
// ============================================================================

void Proximity_EnterDebounceJudge(void)

{
  byte *pbVar1;
  char cVar2;
  undefined8 uVar3;
  
                    /* 0xbacb8  1088  Proximity_EnterDebounceJudge */
  if (g_proxData[5] == 0) {
    *g_proxData = 0;
  }
  else if (*g_proxData < (byte)_refptr_g_tsaPrmtFlash[2].field_0x5f4) {
    cVar2 = Proximity_IsSignalDecTrend();
    if (cVar2 == '\0') {
      *g_proxData = *g_proxData + 1;
    }
  }
  else {
    g_proxData[1] = 1;
    *g_proxData = 0;
    pbVar1 = g_proxData;
    uVar3 = TSAStatic_GetCurrTimeStamp();
    *(undefined8 *)(pbVar1 + 0x18) = uVar3;
    g_proxData[0x38] = 0;
  }
  return;
}


// ============================================================================
// Function: Proximity_DiffToTouch @ 6bafa924
// ============================================================================

/* WARNING: Function: ___chkstk_ms replaced with injection: alloca_probe */

void Proximity_DiffToTouch(void)

{
  uint uVar1;
  undefined8 uVar2;
  undefined8 uVar3;
  undefined8 uVar4;
  undefined8 uVar5;
  TSAFlashPrmt *pTVar6;
  errno_t eVar7;
  undefined1 local_2658 [9800];
  errno_t local_10;
  uint local_c;
  
                    /* 0xba924  1086  Proximity_DiffToTouch */
  local_c = (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags;
  local_10 = 0x6bafa92f;
  local_10 = memcpy_s(local_2658,0x2648,*(void **)_refptr_g_tsaBufDif,
                      (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  if (local_10 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/Proximity/Proximity.c",0x424,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  Touch_Clean();
  eVar7 = memcpy_s(*(void **)_refptr_g_tsaBufDif,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2,g_tsaBufProxZnDif,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  _local_10 = CONCAT44(local_c,eVar7);
  if (eVar7 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/Proximity/Proximity.c",0x429,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  Proximity_PeakDetectInRange();
  memcpy(_refptr_g_peaksBeforeSwitchDifBuffer,*_refptr_g_peaksPtr,0x1a0);
  uVar2 = *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x81c;
  uVar3 = *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x824;
  uVar4 = *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x82c;
  uVar5 = *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x834;
  uVar1 = _refptr_g_tsaPrmtFlash->dwGripFlags;
  Proximity_RefreshTZPrmt();
  TZ_Process();
  CTD_ECProcess();
  IDT_Process('\0');
  Touch_AssignPreIdxBasedOnID();
  PrevTouch_AssignCurIdxBasedOnID();
  TE_Process();
  PrevTouch_PostProcess();
  if ((*(uint *)(g_proxData + 0x54) & 1) != 0) {
    Proximity_SelfTouchPostProcess();
  }
  pTVar6 = _refptr_g_tsaPrmtFlash;
  *(undefined8 *)&_refptr_g_tsaPrmtFlash->field_0x81c = uVar2;
  *(undefined8 *)&pTVar6->field_0x824 = uVar3;
  *(undefined8 *)&pTVar6->field_0x82c = uVar4;
  *(undefined8 *)&pTVar6->field_0x834 = uVar5;
  pTVar6->dwGripFlags = uVar1;
  *(undefined4 *)(g_proxData + 8) = *(undefined4 *)(g_proxData + 0x48);
  if (*(char *)(g_proxData + 1) == '\0') {
    *(undefined4 *)(g_proxData + 0xc) = 0;
  }
  else if (*(int *)(g_proxData + 0xc) < *(int *)(g_proxData + 8)) {
    *(undefined4 *)(g_proxData + 0xc) = *(undefined4 *)(g_proxData + 8);
  }
  (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags = local_c;
  eVar7 = memcpy_s(*(void **)_refptr_g_tsaBufDif,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2,local_2658,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  _local_10 = CONCAT44(local_c,eVar7);
  if (eVar7 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/Proximity/Proximity.c",0x44d,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  TSAIDE_LogPreFltProxRpt();
  return;
}


// ============================================================================
// Function: Proximity_IsSignalIncTrend @ 6bafa2e1
// ============================================================================

undefined1 Proximity_IsSignalIncTrend(void)

{
  int iVar1;
  undefined1 local_9;
  
                    /* 0xba2e1  1111  Proximity_IsSignalIncTrend */
  local_9 = 0;
  if (*(int *)(g_proxData + 0x60) < *(int *)(g_proxData + 0x5c)) {
    iVar1 = *(int *)(g_proxData + 0x5c);
    if (iVar1 < 0) {
      iVar1 = iVar1 + 3;
    }
    if (iVar1 >> 2 < *(int *)(g_proxData + 0x5c) - *(int *)(g_proxData + 0x60)) {
      local_9 = 1;
    }
  }
  return local_9;
}


// ============================================================================
// Function: Proximity_IsSignalDecTrend @ 6bafa344
// ============================================================================

undefined1 Proximity_IsSignalDecTrend(void)

{
  int iVar1;
  undefined1 local_9;
  
                    /* 0xba344  1110  Proximity_IsSignalDecTrend */
  local_9 = 0;
  if (*(int *)(g_proxData + 0x5c) < *(int *)(g_proxData + 0x60)) {
    iVar1 = *(int *)(g_proxData + 0x60);
    if (iVar1 < 0) {
      iVar1 = iVar1 + 3;
    }
    if (iVar1 >> 2 < *(int *)(g_proxData + 0x60) - *(int *)(g_proxData + 0x5c)) {
      local_9 = 1;
    }
  }
  return local_9;
}


// ============================================================================
// Function: Proximity_PeakDetectInRange @ 6bafa6bd
// ============================================================================

void Proximity_PeakDetectInRange(void)

{
  byte local_e;
  undefined1 local_d;
  byte local_c;
  undefined1 local_b;
  short local_a;
  
  local_a = (short)*(undefined4 *)(g_proxData + 0x30);
  if (*(char *)(g_proxData + 1) != '\0') {
    local_a = (short)*(undefined4 *)(g_proxData + 0x3c);
  }
  local_b = 0;
  local_c = _refptr_g_tsaPrmtFlash->bCols;
  local_d = 0;
  local_e = _refptr_g_tsaPrmtFlash->bRows;
  if ((*(uint *)(g_proxData + 0x54) & 4) == 0) {
    if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
      local_c = _refptr_g_tsaPrmtFlash->bCols - 1;
    }
    else {
      local_e = _refptr_g_tsaPrmtFlash->bRows - 1;
    }
  }
  else if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
    local_e = _refptr_g_tsaPrmtFlash->bRows >> 1;
    local_b = 3;
    local_c = _refptr_g_tsaPrmtFlash->bCols - 4;
  }
  else {
    local_d = 3;
    local_e = _refptr_g_tsaPrmtFlash->bRows - 4;
    local_c = _refptr_g_tsaPrmtFlash->bCols >> 1;
  }
  Peak_DetectInRange((int)local_a,local_d,local_e,local_b,local_c);
  return;
}


// ============================================================================
// Function: Proximity_RefreshTZPrmt @ 6bafa7f3
// ============================================================================

void Proximity_RefreshTZPrmt(void)

{
  ushort uVar1;
  int iVar2;
  int iVar3;
  int local_c;
  
  if (*(char *)(g_proxData + 1) == '\0') {
    _refptr_g_tsaPrmtFlash->field_0x836 = _refptr_g_tsaPrmtFlash[2].field_0x5f2;
  }
  else {
    _refptr_g_tsaPrmtFlash->field_0x836 = _refptr_g_tsaPrmtFlash[2].field_0x5f3;
  }
  uVar1 = _refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold;
  if (*(short *)(*_refptr_g_peaksPtr + 2) / 2 <
      (short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold) {
    uVar1 = *(short *)(*_refptr_g_peaksPtr + 2) / 2;
  }
  iVar2 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x5f1 *
          *(int *)&_refptr_g_tsaPrmtFlash[2].field_0x5e4;
  if (iVar2 < 0) {
    iVar2 = iVar2 + 0xf;
  }
  iVar3 = (uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x5f1 *
          *(int *)&_refptr_g_tsaPrmtFlash[2].field_0x5e4;
  if (iVar3 < 0) {
    iVar3 = iVar3 + 0xf;
  }
  local_c = (int)(short)uVar1;
  if (iVar2 >> 4 < (int)(short)uVar1) {
    local_c = iVar2 >> 4;
  }
  if (local_c < iVar3 >> 4) {
    local_c = iVar3 >> 4;
  }
  *(short *)&_refptr_g_tsaPrmtFlash->field_0x81c = (short)local_c;
  *(int *)(g_proxData + 0x34) = local_c;
  _refptr_g_tsaPrmtFlash->dwGripFlags = 0x7f;
  return;
}


// ============================================================================
// Function: Proximity_SelfTouchPostProcess @ 6bafc72e
// ============================================================================

void Proximity_SelfTouchPostProcess(void)

{
  byte bVar1;
  byte local_d;
  byte local_c;
  byte local_b;
  byte local_a;
  byte local_9;
  
                    /* 0xbc72e  1126  Proximity_SelfTouchPostProcess */
  local_9 = 0x14;
  local_a = 0x14;
  local_b = 0;
  local_c = 0;
  *(undefined1 *)(*(longlong *)_refptr_g_proxData + 7) = 0;
  for (local_d = 0; local_d < **(byte **)_refptr_g_touchesPtr; local_d = local_d + 1) {
    bVar1 = TSA_RptTouchYCenter(local_d);
    if ((uint)bVar1 == _refptr_g_tsaPrmtFlash->bRows - 1) {
      if ((ushort)local_b <
          *(ushort *)
           ((longlong)(int)(uint)local_d * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x188)) {
        local_b = (byte)*(undefined2 *)
                         ((longlong)(int)(uint)local_d * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x188);
        local_9 = local_d;
      }
    }
    else {
      bVar1 = TSA_RptTouchXCenter(local_d);
      if (((uint)bVar1 == _refptr_g_tsaPrmtFlash->bCols - 1) &&
         ((ushort)local_c <
          *(ushort *)
           ((longlong)(int)(uint)local_d * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x188))) {
        local_c = (byte)*(undefined2 *)
                         ((longlong)(int)(uint)local_d * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x188);
        local_a = local_d;
      }
    }
  }
  if ((local_9 == 0x14) || (local_a == 0x14)) {
    if ((local_9 == 0x14) && (local_a != 0x14)) {
      Proximity_SelfTouchPostProcessMoreBigAngle();
      **(undefined1 **)_refptr_g_touchesPtr = 1;
    }
    else {
      **(undefined1 **)_refptr_g_touchesPtr = 0;
    }
  }
  else {
    *(undefined2 *)(*(longlong *)_refptr_g_touchesPtr + 0x170) =
         *(undefined2 *)
          ((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x162);
    *(undefined2 *)(*(longlong *)_refptr_g_touchesPtr + 0x172) =
         *(undefined2 *)
          ((longlong)(int)(uint)local_a * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x164);
    *(ushort *)(*(longlong *)_refptr_g_touchesPtr + 0x188) = (ushort)local_c;
    *(ushort *)(*(longlong *)_refptr_g_touchesPtr + 0x18a) = (ushort)local_b;
    *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0x192) = 0x5a;
    *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0x154) = 1;
    if (((uint)(byte)_refptr_g_tsaPrmtFlash->field_0x836 *
         (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x836 < (uint)local_b * (uint)local_c) &&
       ((byte)_refptr_g_tsaPrmtFlash->field_0x836 < local_b)) {
      Proximity_SelfTouchPostProcessNormal();
    }
    else if (((uint)(byte)_refptr_g_tsaPrmtFlash->field_0x836 *
             (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x836) / 2 < (uint)local_b * (uint)local_c) {
      Proximity_SelfTouchPostProcessBigAngle();
    }
    **(undefined1 **)_refptr_g_touchesPtr = 1;
  }
  return;
}


// ============================================================================
// Function: Proximity_SelfTouchPostProcessNormal @ 6bafc297
// ============================================================================

void Proximity_SelfTouchPostProcessNormal(void)

{
  int iVar1;
  
  iVar1 = *(int *)(*(longlong *)_refptr_g_proxData + 0x3c);
  if (iVar1 < 0) {
    iVar1 = iVar1 + 3;
  }
  if ((iVar1 >> 2) * 5 <
      (int)*(short *)((ulonglong)(_refptr_g_tsaPrmtFlash->bCols - 1 & 0xffff) * 2 +
                     *(longlong *)_refptr_g_tsaBufDif)) {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x1f0) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x1f0) | 0x2000;
  }
  else {
    *(undefined1 *)(*(longlong *)_refptr_g_proxData + 7) = 1;
  }
  return;
}


// ============================================================================
// Function: Proximity_SelfTouchPostProcessBigAngle @ 6bafc333
// ============================================================================

void Proximity_SelfTouchPostProcessBigAngle(void)

{
  char cVar1;
  int iVar2;
  
  cVar1 = TSA_IsPhoneCallScenario();
  if (cVar1 != '\0') {
    if (((*(short *)((ulonglong)
                     (((uint)_refptr_g_tsaPrmtFlash->bCols * 2 + (uint)_refptr_g_tsaPrmtFlash->bCols
                      ) - 1 & 0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif) * 3) / 2 <
         (int)*(short *)((ulonglong)(_refptr_g_tsaPrmtFlash->bCols - 1 & 0xffff) * 2 +
                        *(longlong *)_refptr_g_tsaBufDif)) &&
       (*(int *)(*(longlong *)_refptr_g_proxData + 0x3c) <
        (int)*(short *)((ulonglong)(_refptr_g_tsaPrmtFlash->bCols - 1 & 0xffff) * 2 +
                       *(longlong *)_refptr_g_tsaBufDif))) {
      *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x1f0) =
           *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x1f0) | 0x2000;
    }
    if (((*(short *)((ulonglong)
                     ((uint)_refptr_g_tsaPrmtFlash->bCols * (_refptr_g_tsaPrmtFlash->bRows - 1) + 3
                     & 0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif) * 3) / 2 <
         (int)*(short *)((ulonglong)
                         ((uint)_refptr_g_tsaPrmtFlash->bCols * (_refptr_g_tsaPrmtFlash->bRows - 1)
                          + 1 & 0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif)) &&
       (*(int *)(*(longlong *)_refptr_g_proxData + 0x3c) <
        (int)*(short *)((ulonglong)
                        ((uint)_refptr_g_tsaPrmtFlash->bCols * (_refptr_g_tsaPrmtFlash->bRows - 1) +
                         1 & 0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif))) {
      *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x1f0) =
           *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x1f0) | 0x2000;
    }
    iVar2 = (int)*(short *)((ulonglong)
                            ((uint)_refptr_g_tsaPrmtFlash->bCols *
                             (uint)_refptr_g_tsaPrmtFlash->bRows - 2 & 0xffff) * 2 +
                           *(longlong *)_refptr_g_tsaBufDif);
    if (((*(short *)((ulonglong)
                     ((uint)(byte)(_refptr_g_tsaPrmtFlash->bCols - 4) +
                      (uint)(byte)(_refptr_g_tsaPrmtFlash->bRows - 1) *
                      (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
                    *(longlong *)_refptr_g_tsaBufDif) * 3) / 2 < iVar2) &&
       (*(int *)(*(longlong *)_refptr_g_proxData + 0x3c) < iVar2)) {
      *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x1f0) =
           *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x1f0) | 0x2000;
    }
  }
  return;
}


// ============================================================================
// Function: Proximity_SelfTouchPostProcessMoreBigAngle @ 6bafc62a
// ============================================================================

void Proximity_SelfTouchPostProcessMoreBigAngle(void)

{
  short sVar1;
  char cVar2;
  
  cVar2 = TSA_IsPhoneCallScenario();
  if (((cVar2 != '\0') &&
      (sVar1 = *(short *)((ulonglong)
                          (((uint)_refptr_g_tsaPrmtFlash->bCols * 2 +
                           (uint)_refptr_g_tsaPrmtFlash->bCols) - 1 & 0xffff) * 2 +
                         *(longlong *)_refptr_g_tsaBufDif),
      *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 0x154) = 1,
      (sVar1 * 3) / 2 <
      (int)*(short *)((ulonglong)(_refptr_g_tsaPrmtFlash->bCols - 1 & 0xffff) * 2 +
                     *(longlong *)_refptr_g_tsaBufDif))) &&
     (*(int *)(*(longlong *)_refptr_g_proxData + 0x3c) <
      (int)*(short *)((ulonglong)(_refptr_g_tsaPrmtFlash->bCols - 1 & 0xffff) * 2 +
                     *(longlong *)_refptr_g_tsaBufDif))) {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x1f0) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x1f0) | 0x2000;
  }
  return;
}


