// ============================================================================
// Function: SensorGesture_IsProximityPickUpGestureTimeOut @ 6bb05cf2
// ============================================================================

undefined8 SensorGesture_IsProximityPickUpGestureTimeOut(uint param_1)

{
  undefined8 uVar1;
  
                    /* 0xc5cf2  1276  SensorGesture_IsProximityPickUpGestureTimeOut */
  if (DAT_6bbaa0d0 == 0) {
    uVar1 = 1;
  }
  else if ((ulonglong)((*_refptr_g_tsaStaticPtr)->llMsTimestamp - DAT_6bbaa0d0) < (ulonglong)param_1
          ) {
    uVar1 = 0;
  }
  else if ((ulonglong)(DAT_6bbaa0d0 - (*_refptr_g_tsaStaticPtr)->llMsTimestamp) < (ulonglong)param_1
          ) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: SensorGesture_IsProximityPutDownGestureTimeOut @ 6bb05d67
// ============================================================================

undefined8 SensorGesture_IsProximityPutDownGestureTimeOut(uint param_1)

{
  undefined8 uVar1;
  
                    /* 0xc5d67  1279  SensorGesture_IsProximityPutDownGestureTimeOut */
  if (DAT_6bbaa0d8 == 0) {
    uVar1 = 1;
  }
  else if ((ulonglong)((*_refptr_g_tsaStaticPtr)->llMsTimestamp - DAT_6bbaa0d8) < (ulonglong)param_1
          ) {
    uVar1 = 0;
  }
  else if ((ulonglong)(DAT_6bbaa0d8 - (*_refptr_g_tsaStaticPtr)->llMsTimestamp) < (ulonglong)param_1
          ) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: SensorGesture_ClearExternally @ 6bb05b45
// ============================================================================

void SensorGesture_ClearExternally(uint param_1)

{
                    /* 0xc5b45  1268  SensorGesture_ClearExternally */
  g_sensorGesture = g_sensorGesture & ~param_1;
  if ((param_1 & 2) != 0) {
    DAT_6bbaa0d0 = 0;
  }
  if ((param_1 & 4) != 0) {
    DAT_6bbaa0d8 = 0;
  }
  return;
}


// ============================================================================
// Function: SensorGesture_Init @ 6bb05aa0
// ============================================================================

void SensorGesture_Init(void)

{
  int iVar1;
  
                    /* 0xc5aa0  1272  SensorGesture_Init */
  iVar1 = memset_s(&g_sensorGesture,0x20,0,0x20);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SensorGesture/SensorGesture.c",0x15,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  if (*(int *)&_refptr_g_tsaPrmtFlash->field_0x64 != 0) {
    DAT_6bbaa0c4 = 6;
  }
  return;
}


// ============================================================================
// Function: SensorGesture_IsProximityPutDownGestureEnabled @ 6bb05cd7
// ============================================================================

bool SensorGesture_IsProximityPutDownGestureEnabled(void)

{
  uint uVar1;
  
                    /* 0xc5cd7  1278  SensorGesture_IsProximityPutDownGestureEnabled */
  uVar1 = SensorGesture_GetEnabledGesture();
  return (uVar1 & 4) != 0;
}


// ============================================================================
// Function: SensorGesture_IsProximityPickUpGestureEnabled @ 6bb05cbc
// ============================================================================

bool SensorGesture_IsProximityPickUpGestureEnabled(void)

{
  uint uVar1;
  
                    /* 0xc5cbc  1275  SensorGesture_IsProximityPickUpGestureEnabled */
  uVar1 = SensorGesture_GetEnabledGesture();
  return (uVar1 & 2) != 0;
}


// ============================================================================
// Function: SensorGesture_GetEnabledGesture @ 6bb05b1b
// ============================================================================

undefined4 SensorGesture_GetEnabledGesture(void)

{
                    /* 0xc5b1b  1269  SensorGesture_GetEnabledGesture */
  return DAT_6bbaa0c4;
}


