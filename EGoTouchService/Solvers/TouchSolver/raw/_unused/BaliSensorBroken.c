// ============================================================================
// Function: BaliSensorBroken_Init @ 6bb43315
// ============================================================================

void BaliSensorBroken_Init(void)

{
  int iVar1;
  
                    /* 0x103315  388  BaliSensorBroken_Init */
  BaliSensorBroken_Reset();
  iVar1 = memset_s(g_badSensorDebounce,*(undefined2 *)_refptr_g_gridBufSize,0,
                   *(undefined2 *)_refptr_g_gridBufSize);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/WorkAround/BaliSensorBroken/BaliSensorBroken.c",0x39,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: BaliSensorBroken_Process @ 6bb4364b
// ============================================================================

void BaliSensorBroken_Process(void)

{
  char cVar1;
  
                    /* 0x10364b  390  BaliSensorBroken_Process */
  cVar1 = BaliSensorBroken_IsOkToCheckRaw();
  if (cVar1 != '\0') {
    BaliSensorBroken_Reset();
    BaliSensorBroken_RawCheck();
    if (DAT_6bbbf54a != '\0') {
      HardwareAnalyzer_SetException(6);
      BaliSensorBroken_AddDescription();
    }
  }
  return;
}


// ============================================================================
// Function: BaliSensorBroken_Reset @ 6bb432dc
// ============================================================================

void BaliSensorBroken_Reset(void)

{
                    /* 0x1032dc  391  BaliSensorBroken_Reset */
  DAT_6bbbf54c = 0xff;
  DAT_6bbbf54d = 0xff;
  DAT_6bbbf54e = 0;
  DAT_6bbbf54f = 0;
  g_brokenSensor = 0;
  DAT_6bbbf54a = 0;
  DAT_6bbbf54b = 0;
  return;
}


// ============================================================================
// Function: BaliSensorBroken_RawCheck @ 6bb434fa
// ============================================================================

void BaliSensorBroken_RawCheck(void)

{
  char cVar1;
  byte local_a;
  byte local_9;
  
  for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bRows; local_9 = local_9 + 1) {
    for (local_a = 0; local_a < _refptr_g_tsaPrmtFlash->bCols; local_a = local_a + 1) {
      cVar1 = BaliSensorBroken_IsDMDDetected(local_a,local_9);
      if (cVar1 != '\0') {
        DAT_6bbbf54a = 1;
        g_brokenSensor = g_brokenSensor + 1;
        BaliSensorBroken_UpdateBoxInf(local_a,local_9);
      }
    }
  }
  return;
}


// ============================================================================
// Function: BaliSensorBroken_AddDescription @ 6bb43582
// ============================================================================

void BaliSensorBroken_AddDescription(void)

{
  HardwareAnalyzer_AppendExceptionDescription("NumBrokenSensor",g_brokenSensor);
  HardwareAnalyzer_AppendExceptionDescription("BrokenArea:LUDim1",DAT_6bbbf54c);
  HardwareAnalyzer_AppendExceptionDescription("LUDim2",DAT_6bbbf54d);
  HardwareAnalyzer_AppendExceptionDescription("RDDim1",DAT_6bbbf54e);
  HardwareAnalyzer_AppendExceptionDescription("RDDim2",DAT_6bbbf54f);
  return;
}


// ============================================================================
// Function: BaliSensorBroken_IsOkToCheckRaw @ 6bb43609
// ============================================================================

undefined1 BaliSensorBroken_IsOkToCheckRaw(void)

{
  undefined1 local_9;
  
  local_9 = 1;
  if ((1 < (byte)(*_refptr_g_prevTouchesPtr)->field_0x6) &&
     (((*_refptr_g_prevTouchesPtr)->nTouchCount & 200U) != 0)) {
    local_9 = 0;
  }
  return local_9;
}


// ============================================================================
// Function: BaliSensorBroken_UpdateBoxInf @ 6bb43393
// ============================================================================

void BaliSensorBroken_UpdateBoxInf(byte param_1,byte param_2)

{
  byte bVar1;
  
  bVar1 = param_1;
  if (DAT_6bbbf54c <= param_1) {
    bVar1 = DAT_6bbbf54c;
  }
  DAT_6bbbf54c = bVar1;
  bVar1 = param_2;
  if (DAT_6bbbf54d <= param_2) {
    bVar1 = DAT_6bbbf54d;
  }
  DAT_6bbbf54d = bVar1;
  if (param_1 <= DAT_6bbbf54e) {
    param_1 = DAT_6bbbf54e;
  }
  DAT_6bbbf54e = param_1;
  if (param_2 <= DAT_6bbbf54f) {
    param_2 = DAT_6bbbf54f;
  }
  DAT_6bbbf54f = param_2;
  return;
}


// ============================================================================
// Function: BaliSensorBroken_IsDMDDetected @ 6bb43456
// ============================================================================

undefined8 BaliSensorBroken_IsDMDDetected(byte param_1,byte param_2)

{
  char cVar1;
  ushort uVar2;
  
  uVar2 = (ushort)_refptr_g_tsaPrmtFlash->bCols * (ushort)param_2 + (ushort)param_1;
  cVar1 = BaliSensorBroken_IsSensorOverRawLimit(uVar2);
  if (cVar1 == '\0') {
    *(undefined1 *)((ulonglong)uVar2 + g_badSensorDebounce) = 0;
  }
  else {
    if ((int)(200 - (uint)DAT_6bbbf54b) <=
        (int)(uint)*(byte *)((ulonglong)uVar2 + g_badSensorDebounce)) {
      return 1;
    }
    *(char *)((ulonglong)uVar2 + g_badSensorDebounce) =
         *(char *)((ulonglong)uVar2 + g_badSensorDebounce) + '\x01';
  }
  return 0;
}


// ============================================================================
// Function: BaliSensorBroken_IsSensorOverRawLimit @ 6bb433f9
// ============================================================================

undefined8 BaliSensorBroken_IsSensorOverRawLimit(ushort param_1)

{
  ushort uVar1;
  undefined8 uVar2;
  
  uVar1 = *(ushort *)(*(longlong *)_refptr_g_tsaBufRaw + (ulonglong)param_1 * 2);
  if ((*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x5a) < uVar1) ||
     (uVar1 < *(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x5c))) {
    uVar2 = 1;
  }
  else {
    uVar2 = 0;
  }
  return uVar2;
}


