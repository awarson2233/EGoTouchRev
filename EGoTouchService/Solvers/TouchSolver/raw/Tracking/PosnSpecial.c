// ============================================================================
// Function: PosnSpecial_Init @ 6bb422b6
// ============================================================================

void PosnSpecial_Init(undefined8 param_1,undefined2 param_2)

{
  int iVar1;
  
                    /* 0x1022b6  1022  PosnSpecial_Init */
  iVar1 = memset_s(&g_list,0x98,0,0x98);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Tool/PosnList/PosnSpecial/PosnSpecial.c",0x53,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  DAT_6bcac4ca = 0;
  DAT_6bcac4ce = 0;
  DAT_6bcac4cf = 1;
  DAT_6bcac4d0 = 1;
  DAT_6bcac4d1 = 0;
  DAT_6bcac4d2 = 0;
  DAT_6bcac4dd = 0;
  g_list = param_1;
  DAT_6bcac4c8 = param_2;
  DAT_6bcac4cc = param_2;
  PosnSpecial_ResetZone(0);
  return;
}


// ============================================================================
// Function: PosnSpecial_ReBase @ 6bb423b7
// ============================================================================

void PosnSpecial_ReBase(void)

{
                    /* 0x1023b7  1030  PosnSpecial_ReBase */
  DAT_6bcac4d3 = 0;
  DAT_6bcac4d4 = 0;
  DAT_6bcac4d5 = 0;
  DAT_6bcac4d8 = 0;
  DAT_6bcac4dc = 0xff;
  return;
}


// ============================================================================
// Function: PosnSpecial_Put @ 6bb42569
// ============================================================================

void PosnSpecial_Put(ushort param_1)

{
  ulonglong uVar1;
  
                    /* 0x102569  1029  PosnSpecial_Put */
  if (DAT_6bcac4ca < DAT_6bcac4cc) {
    if (*(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)param_1 * 2) < 0) {
      DAT_6bcac4cc = DAT_6bcac4cc - 1;
      *(ushort *)(g_list + (ulonglong)DAT_6bcac4cc * 2) = param_1;
    }
    else {
      uVar1 = (ulonglong)DAT_6bcac4ca;
      DAT_6bcac4ca = DAT_6bcac4ca + 1;
      *(ushort *)(g_list + uVar1 * 2) = param_1;
    }
  }
  return;
}


// ============================================================================
// Function: PosnSpecial_Get @ 6bb42624
// ============================================================================

undefined2 PosnSpecial_Get(void)

{
  ushort uVar1;
  undefined2 local_a;
  
                    /* 0x102624  1014  PosnSpecial_Get */
  PosnSpecial_UpdateSign();
  if (DAT_6bcac4ce == '\0') {
    DAT_6bcac4ca = DAT_6bcac4ca - 1;
    local_a = *(undefined2 *)((ulonglong)DAT_6bcac4ca * 2 + g_list);
    if (DAT_6bcac4cf != '\0') {
      DAT_6bcac4d1 = DAT_6bcac4d1 + '\x01';
      DAT_6bcac4cf = DAT_6bcac4ce;
    }
  }
  else {
    uVar1 = DAT_6bcac4cc + 1;
    local_a = *(undefined2 *)((ulonglong)DAT_6bcac4cc * 2 + g_list);
    DAT_6bcac4cc = uVar1;
    if (DAT_6bcac4ce != DAT_6bcac4cf) {
      DAT_6bcac4d2 = DAT_6bcac4d2 + '\x01';
      DAT_6bcac4cf = DAT_6bcac4ce;
    }
  }
  PosnSpecial_AddCellInZone(local_a);
  return local_a;
}


// ============================================================================
// Function: PosnSpecial_IsEmpty @ 6bb42761
// ============================================================================

undefined1 PosnSpecial_IsEmpty(void)

{
  char cVar1;
  
                    /* 0x102761  1023  PosnSpecial_IsEmpty */
  cVar1 = PosnSpecial_IsPosEmpty();
  if ((cVar1 != '\0') && (cVar1 = PosnSpecial_IsNegEmpty(), cVar1 != '\0')) {
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: PosnSpecial_GetNumSignChange @ 6bb42444
// ============================================================================

undefined1 PosnSpecial_GetNumSignChange(void)

{
                    /* 0x102444  1018  PosnSpecial_GetNumSignChange */
  return DAT_6bcac4d3;
}


// ============================================================================
// Function: PosnSpecial_GetNumValidSensor @ 6bb42455
// ============================================================================

undefined1 PosnSpecial_GetNumValidSensor(void)

{
                    /* 0x102455  1019  PosnSpecial_GetNumValidSensor */
  return DAT_6bcac4d4;
}


// ============================================================================
// Function: PosnSpecial_GetNumEdgeSensor @ 6bb42466
// ============================================================================

undefined1 PosnSpecial_GetNumEdgeSensor(void)

{
                    /* 0x102466  1017  PosnSpecial_GetNumEdgeSensor */
  return DAT_6bcac4d5;
}


// ============================================================================
// Function: PosnSpecial_GetSumEdgeSensor @ 6bb42477
// ============================================================================

undefined4 PosnSpecial_GetSumEdgeSensor(void)

{
                    /* 0x102477  1021  PosnSpecial_GetSumEdgeSensor */
  return DAT_6bcac4d8;
}


// ============================================================================
// Function: PosnSpecial_ResetZone @ 6bb420ba
// ============================================================================

void PosnSpecial_ResetZone(byte param_1)

{
                    /* 0x1020ba  1033  PosnSpecial_ResetZone */
  (&DAT_6bcac4e0)[(longlong)(int)(uint)param_1 * 6] = 0x8000;
  *(undefined2 *)(&DAT_6bcac4e2 + (longlong)(int)(uint)param_1 * 0xc) = 0x7fff;
  *(undefined4 *)(&DAT_6bcac4e4 + (longlong)(int)(uint)param_1 * 0xc) = 0;
  (&DAT_6bcac4e8)[(longlong)(int)(uint)param_1 * 0xc] = 0;
  return;
}


// ============================================================================
// Function: PosnSpecial_AddCellInZone @ 6bb421ae
// ============================================================================

void PosnSpecial_AddCellInZone(ushort param_1)

{
  short sVar1;
  longlong lVar2;
  
                    /* 0x1021ae  1011  PosnSpecial_AddCellInZone */
  sVar1 = *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)param_1 * 2);
  lVar2 = (longlong)(int)(uint)DAT_6bcac4dd * 0xc;
  if ((short)(&DAT_6bcac4e0)[(longlong)(int)(uint)DAT_6bcac4dd * 6] < sVar1) {
    (&DAT_6bcac4e0)[(longlong)(int)(uint)DAT_6bcac4dd * 6] = sVar1;
  }
  if (sVar1 < *(short *)(&DAT_6bcac4e2 + lVar2)) {
    *(short *)(&DAT_6bcac4e2 + lVar2) = sVar1;
  }
  *(int *)(&DAT_6bcac4e4 + lVar2) = *(int *)(&DAT_6bcac4e4 + lVar2) + (int)sVar1;
  (&DAT_6bcac4e8)[lVar2] = (&DAT_6bcac4e8)[lVar2] + '\x01';
  return;
}


// ============================================================================
// Function: PosnSpecial_UpdateSign @ 6bb42265
// ============================================================================

void PosnSpecial_UpdateSign(void)

{
  char cVar1;
  
                    /* 0x102265  1034  PosnSpecial_UpdateSign */
  if (DAT_6bcac4ce == '\x01') {
    cVar1 = PosnSpecial_IsNegEmpty();
    if (cVar1 != '\0') {
      PosnSpecial_NextZone();
      DAT_6bcac4ce = '\0';
    }
  }
  else {
    cVar1 = PosnSpecial_IsPosEmpty();
    if (cVar1 != '\0') {
      PosnSpecial_NextZone();
      DAT_6bcac4ce = '\x01';
    }
  }
  return;
}


// ============================================================================
// Function: PosnSpecial_IsPosEmpty @ 6bb42050
// ============================================================================

undefined2 PosnSpecial_IsPosEmpty(void)

{
                    /* 0x102050  1025  PosnSpecial_IsPosEmpty */
  return CONCAT11((char)((ushort)DAT_6bcac4ca >> 8),DAT_6bcac4ca == 0);
}


// ============================================================================
// Function: PosnSpecial_IsNegEmpty @ 6bb42067
// ============================================================================

undefined2 PosnSpecial_IsNegEmpty(void)

{
                    /* 0x102067  1024  PosnSpecial_IsNegEmpty */
  return CONCAT11((char)((ushort)DAT_6bcac4c8 >> 8),DAT_6bcac4cc == DAT_6bcac4c8);
}


// ============================================================================
// Function: PosnSpecial_Record @ 6bb42487
// ============================================================================

void PosnSpecial_Record(short param_1,short param_2,char param_3)

{
  byte bVar1;
  
                    /* 0x102487  1031  PosnSpecial_Record */
  if ((param_2 < param_1) && (param_3 != '\0')) {
    DAT_6bcac4d4 = DAT_6bcac4d4 + '\x01';
  }
  bVar1 = (byte)((ushort)param_1 >> 0xf);
  if (DAT_6bcac4dc != bVar1) {
    if (DAT_6bcac4dc != 0xff) {
      DAT_6bcac4d3 = DAT_6bcac4d3 + '\x01';
    }
    DAT_6bcac4dc = bVar1;
  }
  return;
}


// ============================================================================
// Function: PosnSpecial_Record2 @ 6bb4251d
// ============================================================================

void PosnSpecial_Record2(short param_1,short param_2)

{
                    /* 0x10251d  1032  PosnSpecial_Record2 */
  if (param_2 < param_1) {
    DAT_6bcac4d5 = DAT_6bcac4d5 + '\x01';
  }
  DAT_6bcac4d8 = DAT_6bcac4d8 + param_1;
  return;
}


// ============================================================================
// Function: PosnSpecial_NextZone @ 6bb42163
// ============================================================================

void PosnSpecial_NextZone(void)

{
                    /* 0x102163  1027  PosnSpecial_NextZone */
  if (DAT_6bcac4dd < 9) {
    DAT_6bcac4dd = DAT_6bcac4dd + 1;
    PosnSpecial_ResetZone(DAT_6bcac4dd);
  }
  return;
}


// ============================================================================
// Function: PosnSpecial_GetPosMarker @ 6bb423f7
// ============================================================================

char PosnSpecial_GetPosMarker(void)

{
                    /* 0x1023f7  1020  PosnSpecial_GetPosMarker */
  if (DAT_6bcac4d1 == '\0') {
    DAT_6bcac4d1 = '\x01';
  }
  return DAT_6bcac4d1;
}


