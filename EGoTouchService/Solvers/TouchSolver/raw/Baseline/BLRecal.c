// ============================================================================
// Function: BLRecal_Process @ 6ba47458
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* Tracks boundary-hit driven recalibration exit conditions and special-control transitions during
   baseline recalibration. */

void BLRecal_Process(void)

{
  char cVar1;
  
                    /* 0x7458  354  BLRecal_Process */
  (*_refptr_g_tsaStaticPtr)->bJustExitedSpecialCtrlMode = '\0';
  if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 1) != 0) {
    (*_refptr_g_tsaStaticPtr)->bRecalFrameCount = '\0';
    (*_refptr_g_tsaStaticPtr)->bRecalBoundaryHitCount = '\0';
  }
  if ((*_refptr_g_tsaStaticPtr)->bRecalFrameCount < 5) {
    (*_refptr_g_tsaStaticPtr)->bRecalFrameCount =
         (*_refptr_g_tsaStaticPtr)->bRecalFrameCount + '\x01';
  }
  else if ((*_refptr_g_tsaStaticPtr)->bRecalFrameCount < 0xf) {
    (*_refptr_g_tsaStaticPtr)->bRecalFrameCount =
         (*_refptr_g_tsaStaticPtr)->bRecalFrameCount + '\x01';
    if ((_refptr_g_tsaPrpt->nSPeakPositive < (short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax)
       && (-(int)_refptr_g_tsaPrpt->nSPeakNegative <
           (int)(short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax)) {
      if ((int)(uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x4a <
          (int)((uint)_refptr_g_tsaPrpt->wBoundaryPositive -
               (uint)_refptr_g_tsaPrpt->wBoundaryNegative)) {
        (*_refptr_g_tsaStaticPtr)->bRecalBoundaryHitCount =
             (*_refptr_g_tsaStaticPtr)->bRecalBoundaryHitCount + '\x01';
      }
      else if ((uint)(*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x4a >> 1) +
               (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x48 <
               (uint)_refptr_g_tsaPrpt->wBoundaryPositive) {
        (*_refptr_g_tsaStaticPtr)->bRecalBoundaryHitCount =
             (*_refptr_g_tsaStaticPtr)->bRecalBoundaryHitCount + '\x01';
      }
      else if ((int)(uint)_refptr_g_tsaPrpt->wBoundaryNegative <
               (int)((uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x48 -
                    (uint)(*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x4a >> 1))) {
        (*_refptr_g_tsaStaticPtr)->bRecalBoundaryHitCount =
             (*_refptr_g_tsaStaticPtr)->bRecalBoundaryHitCount + '\x01';
      }
    }
  }
  else {
    if (2 < (*_refptr_g_tsaStaticPtr)->bRecalBoundaryHitCount) {
      cVar1 = AFE_IsAutoCalibration();
      if (cVar1 == '\0') {
        (*_refptr_g_tsaStaticPtr)->bJustExitedSpecialCtrlMode = '\x01';
        (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
             (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 2;
      }
    }
    (*_refptr_g_tsaStaticPtr)->bRecalBoundaryHitCount = '\0';
  }
  cVar1 = AFE_IsAutoCalibration();
  if ((cVar1 != '\0') &&
     (300000 < (ulonglong)
               ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
               (*_refptr_g_tsaStaticPtr)->llAutoCalibrationStartTime))) {
    cVar1 = AFE_IsRecalRequested();
    if ((cVar1 != '\0') &&
       (((*_refptr_g_tsaStaticPtr)->bBaselineUpdatedThisFrame != '\0' &&
        (2 < (ushort)_refptr_g_tsaPrpt->nBaselineStageFrameCount)))) {
      (*_refptr_g_tsaStaticPtr)->bJustExitedSpecialCtrlMode = '\x01';
    }
  }
  return;
}


// ============================================================================
// Function: BLRecal_Init @ 6ba47430
// ============================================================================

void BLRecal_Init(void)

{
                    /* 0x7430  353  BLRecal_Init */
  (*_refptr_g_tsaStaticPtr)->bRecalFrameCount = 0xff;
  (*_refptr_g_tsaStaticPtr)->bRecalBoundaryHitCount = '\0';
  return;
}


