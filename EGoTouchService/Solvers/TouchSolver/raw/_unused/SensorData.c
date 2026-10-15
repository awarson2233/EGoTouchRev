// ============================================================================
// Function: SensorData_IsAntiTouchStage @ 6ba6b855
// ============================================================================

undefined8 SensorData_IsAntiTouchStage(void)

{
  uint uVar1;
  longlong lVar2;
  
                    /* 0x2b855  1264  SensorData_IsAntiTouchStage */
  lVar2 = TSAFold_GetTimeStampRegionChange();
  if ((lVar2 != 0) &&
     ((ulonglong)((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - lVar2) < 100)) {
    DAT_6bb4f172 = 0;
    DAT_6bb4f178 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    return 1;
  }
  if (((*_refptr_g_tsaStaticPtr)->pFrameBufferPtr <= (void *)(DAT_6bb4f188 + 3000U)) &&
     (uVar1 = TSA_GetEnabledSensor(), (uVar1 & 8) != 0)) {
    if ((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6cc & 1) == 0) {
      if (((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6cc & 2) != 0) &&
         ((DAT_6bb4f172 == '\0' || (DAT_6bb4f173 == '\0')))) {
        return 1;
      }
    }
    else if (((DAT_6bb4f172 == '\0') || (DAT_6bb4f173 == '\0')) || (DAT_6bb4f190 == 1)) {
      return 1;
    }
    return 0;
  }
  DAT_6bb4f190 = 0;
  DAT_6bb4f188 = 0;
  DAT_6bb4f180 = 0;
  DAT_6bb4f178 = (void *)0x0;
  DAT_6bb4f173 = 1;
  DAT_6bb4f172 = 1;
  DAT_6bb4f171 = 0;
  DAT_6bb4f170 = 0;
  return 0;
}


// ============================================================================
// Function: SensorData_Init @ 6ba6b220
// ============================================================================

void SensorData_Init(void)

{
                    /* 0x2b220  1263  SensorData_Init */
  AccQueue_Init(&g_accQueueBuff,0x100);
  DAT_6bb4f172 = 1;
  DAT_6bb4f173 = 1;
  DAT_6bb4f190 = 0;
  DAT_6bb4f178 = 0;
  DAT_6bb4f180 = 0;
  DAT_6bb4f188 = 0;
  DAT_6bb4f170 = 0;
  DAT_6bb4f171 = 0;
  return;
}


// ============================================================================
// Function: SensorData_AntiTouchStageTimeout @ 6ba6b7e5
// ============================================================================

undefined8 SensorData_AntiTouchStageTimeout(void)

{
  undefined8 uVar1;
  
                    /* 0x2b7e5  1259  SensorData_AntiTouchStageTimeout */
  if ((((DAT_6bb4f173 == '\0') &&
       ((void *)(DAT_6bb4f180 + 6000U) < (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr)) &&
      (DAT_6bb4f172 != '\0')) &&
     ((void *)(DAT_6bb4f178 + 6000) <= (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr)) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


