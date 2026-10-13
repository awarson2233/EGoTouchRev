// ============================================================================
// Function: TSAIDE_Reset @ 6bb37700
// ============================================================================

void TSAIDE_Reset(void)

{
                    /* 0xf7700  1559  TSAIDE_Reset */
  DAT_6bca3d85 = 0;
  DAT_6bca3d84 = 0;
  g_tsaIdeSurSensor = 0;
  g_tsaIdeSurPoint = 0;
  g_touchesPreFltGrid = 0;
  g_touchesPreFltProx = 0;
  g_touchesPreFltSide = 0;
  g_touchesPreFltHand = 0;
  return;
}


// ============================================================================
// Function: TSAIDE_LogPreFltGridRpt @ 6bb3778b
// ============================================================================

void TSAIDE_LogPreFltGridRpt(void)

{
                    /* 0xf778b  1553  TSAIDE_LogPreFltGridRpt */
  memcpy(&g_touchesPreFltGrid,_refptr_g_touchesGrid,0x1d60);
  return;
}


// ============================================================================
// Function: TSAIDE_CleanPointXY @ 6bb38149
// ============================================================================

void TSAIDE_CleanPointXY(void)

{
                    /* 0xf8149  1544  TSAIDE_CleanPointXY */
  g_tsaIdeSurPoint = 0;
  return;
}


// ============================================================================
// Function: TSAIDE_LogPreFltSideRpt @ 6bb377e8
// ============================================================================

void TSAIDE_LogPreFltSideRpt(void)

{
                    /* 0xf77e8  1556  TSAIDE_LogPreFltSideRpt */
  memcpy(&g_touchesPreFltSide,_refptr_g_touchesSide,0x1d60);
  return;
}


// ============================================================================
// Function: TSAIDE_GetState @ 6bb37767
// ============================================================================

undefined4 TSAIDE_GetState(void)

{
                    /* 0xf7767  1547  TSAIDE_GetState */
  return g_tsaIdePreFltRpt;
}


// ============================================================================
// Function: TSAIDE_SetState @ 6bb37776
// ============================================================================

void TSAIDE_SetState(undefined4 param_1)

{
                    /* 0xf7776  1561  TSAIDE_SetState */
  g_tsaIdePreFltRpt = param_1;
  return;
}


// ============================================================================
// Function: TSAIDE_SetIndexVolume @ 6bb37fd1
// ============================================================================

void TSAIDE_SetIndexVolume(byte param_1)

{
  byte local_res8;
  
                    /* 0xf7fd1  1560  TSAIDE_SetIndexVolume */
  local_res8 = param_1;
  if (param_1 == 0xff) {
    local_res8 = _refptr_g_tsaPrmtFlash->bCols - 1;
  }
  if (_refptr_g_tsaPrmtFlash->bCols == local_res8) {
    local_res8 = 0;
  }
  DAT_6bca4642 = local_res8 % _refptr_g_tsaPrmtFlash->bCols;
  return;
}


// ============================================================================
// Function: TSAIDE_GetIndexVolume @ 6bb3802a
// ============================================================================

undefined1 TSAIDE_GetIndexVolume(void)

{
                    /* 0xf802a  1545  TSAIDE_GetIndexVolume */
  return DAT_6bca4642;
}


// ============================================================================
// Function: TSAIDE_LogSensor @ 6bb3803b
// ============================================================================

void TSAIDE_LogSensor(byte param_1,byte param_2,undefined4 param_3)

{
  byte local_res8;
  byte local_res10;
  
                    /* 0xf803b  1557  TSAIDE_LogSensor */
  if (199 < g_tsaIdeSurSensor) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Plugin/IDE/TSA_IDE.c",
                      0xe7,"g_tsaIdeSurSensor.num < TSAIDE_SENSOR_UNIT_LIMIT");
    HardwareAnalyzer_SetSoftException();
  }
  local_res8 = param_1;
  if (_refptr_g_tsaPrmtFlash->bCols <= param_1) {
    local_res8 = _refptr_g_tsaPrmtFlash->bCols - 1;
  }
  local_res10 = param_2;
  if (_refptr_g_tsaPrmtFlash->bRows <= param_2) {
    local_res10 = _refptr_g_tsaPrmtFlash->bRows - 1;
  }
  (&DAT_6bca4644)[(longlong)(int)(uint)g_tsaIdeSurSensor * 8] = local_res8;
  (&DAT_6bca4645)[(longlong)(int)(uint)g_tsaIdeSurSensor * 8] = local_res10;
  *(undefined4 *)(&DAT_6bca4648 + (longlong)(int)(uint)g_tsaIdeSurSensor * 8) = param_3;
  g_tsaIdeSurSensor = g_tsaIdeSurSensor + 1;
  return;
}


// ============================================================================
// Function: TSAIDE_LogPreFiltTouch @ 6bb38314
// ============================================================================

void TSAIDE_LogPreFiltTouch(void)

{
  char cVar1;
  undefined2 uVar2;
  byte local_9;
  
                    /* 0xf8314  1552  TSAIDE_LogPreFiltTouch */
  StrPrintf_Process(0,"\nPreFiltTouchNum = %d",g_touchesPreFltGrid);
  for (local_9 = 0; local_9 < g_touchesPreFltGrid; local_9 = local_9 + 1) {
    StrPrintf_Process(0,&DAT_6bb5ccd5,(&DAT_6bbae314)[(longlong)(int)(uint)local_9 * 0x168]);
    uVar2 = TSAOut_Pixel2DisplayDim1
                      (*(undefined2 *)(&DAT_6bbae330 + (longlong)(int)(uint)local_9 * 0x168));
    StrPrintf_Process(0,&DAT_6bb5ccda,uVar2);
    uVar2 = TSAOut_Pixel2DisplayDim2
                      (*(undefined2 *)(&DAT_6bbae332 + (longlong)(int)(uint)local_9 * 0x168));
    StrPrintf_Process(0,&DAT_6bb5ccda,uVar2);
    StrPrintf_Process(0,&DAT_6bb5ccda,
                      *(undefined2 *)(&DAT_6bbae31c + (longlong)(int)(uint)local_9 * 0x168));
    StrPrintf_Process(0,&DAT_6bb5ccda,
                      *(undefined2 *)(&DAT_6bbae316 + (longlong)(int)(uint)local_9 * 0x168));
    uVar2 = TS_MMToPixel((char)*(undefined2 *)(&DAT_6bbae348 + (longlong)(int)(uint)local_9 * 0x168)
                        );
    uVar2 = TSAOut_Axis(uVar2);
    StrPrintf_Process(0,&DAT_6bb5ccda,uVar2);
    uVar2 = TS_MMToPixel((char)*(undefined2 *)(&DAT_6bbae34a + (longlong)(int)(uint)local_9 * 0x168)
                        );
    uVar2 = TSAOut_Axis(uVar2);
    StrPrintf_Process(0,&DAT_6bb5ccda,uVar2);
    cVar1 = TSAOut_Angle((int)(char)(&DAT_6bbae352)[(longlong)(int)(uint)local_9 * 0x168]);
    StrPrintf_Process(0,&DAT_6bb5ccde,(int)cVar1);
  }
  return;
}


// ============================================================================
// Function: TSAIDE_LogPointXY @ 6bb3815b
// ============================================================================

void TSAIDE_LogPointXY(undefined2 param_1,undefined2 param_2,undefined4 param_3)

{
                    /* 0xf815b  1551  TSAIDE_LogPointXY */
  if (0x5db < g_tsaIdeSurPoint) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Plugin/IDE/TSA_IDE.c",
                      0xfc,"g_tsaIdeSurPoint.num < TSAIDE_POINT_UNIT_LIMIT");
    HardwareAnalyzer_SetSoftException();
  }
  *(undefined2 *)(&DAT_6bca4cc4 + (longlong)(int)(uint)g_tsaIdeSurPoint * 8) = param_1;
  *(undefined2 *)(&DAT_6bca4cc6 + (longlong)(int)(uint)g_tsaIdeSurPoint * 8) = param_2;
  *(undefined4 *)(&DAT_6bca4cc8 + (longlong)(int)(uint)g_tsaIdeSurPoint * 8) = param_3;
  g_tsaIdeSurPoint = g_tsaIdeSurPoint + 1;
  return;
}


// ============================================================================
// Function: TSAIDE_LogPreFltHandRpt @ 6bb37815
// ============================================================================

void TSAIDE_LogPreFltHandRpt(void)

{
                    /* 0xf7815  1554  TSAIDE_LogPreFltHandRpt */
  memcpy(&g_touchesPreFltHand,_refptr_g_touchesHand,0x1d60);
  return;
}


// ============================================================================
// Function: TSAIDE_LogPoint @ 6bb3822b
// ============================================================================

void TSAIDE_LogPoint(undefined2 param_1,undefined2 param_2,undefined4 param_3)

{
  ushort uVar1;
  undefined2 uVar2;
  
                    /* 0xf822b  1550  TSAIDE_LogPoint */
  if (0x5db < g_tsaIdeSurPoint) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Plugin/IDE/TSA_IDE.c",
                      0x106,"g_tsaIdeSurPoint.num < TSAIDE_POINT_UNIT_LIMIT");
    HardwareAnalyzer_SetSoftException();
  }
  uVar1 = g_tsaIdeSurPoint;
  uVar2 = TSAOut_Pixel2DisplayDim1(param_1);
  *(undefined2 *)(&DAT_6bca4cc4 + (longlong)(int)(uint)uVar1 * 8) = uVar2;
  uVar1 = g_tsaIdeSurPoint;
  uVar2 = TSAOut_Pixel2DisplayDim2(param_2);
  *(undefined2 *)(&DAT_6bca4cc6 + (longlong)(int)(uint)uVar1 * 8) = uVar2;
  *(undefined4 *)(&DAT_6bca4cc8 + (longlong)(int)(uint)g_tsaIdeSurPoint * 8) = param_3;
  g_tsaIdeSurPoint = g_tsaIdeSurPoint + 1;
  return;
}


// ============================================================================
// Function: TSAIDE_LogPreFltProxRpt @ 6bb377b8
// ============================================================================

void TSAIDE_LogPreFltProxRpt(void)

{
                    /* 0xf77b8  1555  TSAIDE_LogPreFltProxRpt */
  memcpy(&g_touchesPreFltProx,*(void **)_refptr_g_touchesProxPtr,0x1d60);
  return;
}


