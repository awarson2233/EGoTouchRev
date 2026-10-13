// ============================================================================
// Function: ASACalibration_Init @ 6baab7b0
// ============================================================================

void ASACalibration_Init(void)

{
                    /* 0x6b7b0  94  ASACalibration_Init */
  if (initOnlyOnce_8296 == '\0') {
    PositionOffsetCalibrationInit();
    g_mainFuncProc = 0;
    initOnlyOnce_8296 = '\x01';
  }
  OemInfoCfgInit();
  LinearTestInit();
  g_bufferCnt = 0;
  g_flagTx1DroppedSolutionEnable = 0;
  g_asaCalibStatus = 0;
  return;
}


