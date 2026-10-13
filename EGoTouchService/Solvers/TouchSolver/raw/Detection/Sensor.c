// ============================================================================
// Function: Sensor_GetZoneIndex2 @ 6ba672e1
// ============================================================================

undefined1 Sensor_GetZoneIndex2(ushort param_1)

{
  byte bVar1;
  char cVar2;
  int iVar3;
  undefined1 local_9;
  
                    /* 0x272e1  1294  Sensor_GetZoneIndex2 */
  bVar1 = *(byte *)((ulonglong)param_1 + *(longlong *)_refptr_g_tsaBufTz);
  cVar2 = (bVar1 ^ (char)bVar1 >> 0x1f) - ((char)bVar1 >> 7);
  if (cVar2 == '\0') {
    local_9 = 0xff;
  }
  else {
    iVar3 = cVar2 + -0x10;
    if (iVar3 < 0) {
      iVar3 = cVar2 + -9;
    }
    local_9 = (undefined1)(iVar3 >> 3);
  }
  return local_9;
}


// ============================================================================
// Function: Sensor_GetPosType2 @ 6ba67374
// ============================================================================

uint Sensor_GetPosType2(ushort param_1)

{
  uint local_c;
  
                    /* 0x27374  1290  Sensor_GetPosType2 */
  local_c = (int)*(char *)((ulonglong)param_1 + *(longlong *)_refptr_g_tsaBufTz) >> 0x1f;
  local_c = ((int)*(char *)((ulonglong)param_1 + *(longlong *)_refptr_g_tsaBufTz) ^ local_c) -
            local_c;
  if (local_c == 0) {
    local_c = 0;
  }
  else {
    local_c = local_c & 7;
  }
  return local_c;
}


// ============================================================================
// Function: Sensor_GetPosType @ 6ba67339
// ============================================================================

void Sensor_GetPosType(byte param_1,byte param_2)

{
                    /* 0x27339  1289  Sensor_GetPosType */
  Sensor_GetPosType2((ushort)param_1 + (ushort)param_2 * (ushort)_refptr_g_tsaPrmtFlash->bCols);
  return;
}


// ============================================================================
// Function: Sensor_GetZoneIndex @ 6ba672a6
// ============================================================================

void Sensor_GetZoneIndex(byte param_1,byte param_2)

{
                    /* 0x272a6  1293  Sensor_GetZoneIndex */
  Sensor_GetZoneIndex2((ushort)param_1 + (ushort)param_2 * (ushort)_refptr_g_tsaPrmtFlash->bCols);
  return;
}


// ============================================================================
// Function: Sensor_IsTouchTraversed @ 6ba67472
// ============================================================================

undefined8 Sensor_IsTouchTraversed(undefined1 param_1,undefined1 param_2)

{
  int iVar1;
  undefined8 uVar2;
  
                    /* 0x27472  1301  Sensor_IsTouchTraversed */
  iVar1 = Sensor_GetPosType(param_1,param_2);
  if (((iVar1 == 0) || (iVar1 == 7)) || (iVar1 == 2)) {
    uVar2 = 0;
  }
  else {
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: Sensor_IsTouchTraversedBefore @ 6ba6742c
// ============================================================================

undefined8 Sensor_IsTouchTraversedBefore(undefined1 param_1,undefined1 param_2)

{
  int iVar1;
  undefined8 uVar2;
  
                    /* 0x2742c  1302  Sensor_IsTouchTraversedBefore */
  iVar1 = Sensor_GetPosType3(param_1,param_2);
  if (((iVar1 == 0) || (iVar1 == 7)) || (iVar1 == 2)) {
    uVar2 = 0;
  }
  else {
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: Sensor_IsInZone @ 6ba674b8
// ============================================================================

undefined8 Sensor_IsInZone(byte param_1,byte param_2,byte param_3)

{
  byte bVar1;
  char cVar2;
  undefined8 uVar3;
  
                    /* 0x274b8  1299  Sensor_IsInZone */
  bVar1 = *(byte *)((ulonglong)
                    ((uint)param_1 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) +
                   *(longlong *)_refptr_g_tsaBufTz);
  cVar2 = (bVar1 ^ (char)bVar1 >> 0x1f) - ((char)bVar1 >> 7);
  if ((((cVar2 == '\0') || (param_3 == 0)) || ((int)cVar2 < (int)(uint)param_3)) ||
     ((int)(param_3 + 8) <= (int)cVar2)) {
    uVar3 = 0;
  }
  else {
    uVar3 = 1;
  }
  return uVar3;
}


// ============================================================================
// Function: Sensor_GetSquareDistance @ 6ba66b55
// ============================================================================

int Sensor_GetSquareDistance(byte param_1,byte param_2,byte param_3,byte param_4)

{
                    /* 0x26b55  1292  Sensor_GetSquareDistance */
  return (uint)(ushort)(((ushort)param_2 - (ushort)param_4) * ((ushort)param_2 - (ushort)param_4)) +
         (uint)(ushort)(((ushort)param_1 - (ushort)param_3) * ((ushort)param_1 - (ushort)param_3));
}


// ============================================================================
// Function: Sensor_IsInCoreZone @ 6ba67540
// ============================================================================

undefined8 Sensor_IsInCoreZone(byte param_1,byte param_2,byte param_3)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0x27540  1295  Sensor_IsInCoreZone */
  cVar1 = *(char *)((ulonglong)
                    ((uint)param_1 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) +
                   *(longlong *)_refptr_g_tsaBufTzGrid);
  if ((((cVar1 == '\0') || (param_3 == 0)) || ((int)cVar1 < (int)(uint)param_3)) ||
     ((int)(param_3 + 8) <= (int)cVar1)) {
    uVar2 = 0;
  }
  else {
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: Sensor_IsInSideZone @ 6ba675c0
// ============================================================================

undefined8 Sensor_IsInSideZone(byte param_1,byte param_2,byte param_3)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0x275c0  1298  Sensor_IsInSideZone */
  cVar1 = *(char *)((ulonglong)
                    ((uint)param_1 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) +
                   *(longlong *)_refptr_g_tsaBufTzSide);
  if ((((cVar1 == '\0') || (param_3 == 0)) || ((int)cVar1 < (int)(uint)param_3)) ||
     ((int)(param_3 + 8) <= (int)cVar1)) {
    uVar2 = 0;
  }
  else {
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: Sensor_GetPosType3 @ 6ba673c2
// ============================================================================

uint Sensor_GetPosType3(byte param_1,byte param_2)

{
  uint uVar1;
  uint local_c;
  
                    /* 0x273c2  1291  Sensor_GetPosType3 */
  uVar1 = (uint)*(char *)((ulonglong)
                          ((uint)param_1 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols &
                          0xffff) + *(longlong *)_refptr_g_tsaBufTzPrev);
  local_c = (int)uVar1 >> 0x1f;
  local_c = (uVar1 ^ local_c) - local_c;
  if (local_c == 0) {
    local_c = 0;
  }
  else {
    local_c = local_c & 7;
  }
  return local_c;
}


// ============================================================================
// Function: Sensor_IsInDifferentTouchZone @ 6ba67640
// ============================================================================

undefined1
Sensor_IsInDifferentTouchZone
          (undefined1 param_1,undefined1 param_2,undefined1 param_3,undefined1 param_4)

{
  char cVar1;
  char cVar2;
  undefined1 local_9;
  
                    /* 0x27640  1296  Sensor_IsInDifferentTouchZone */
  local_9 = 0;
  cVar1 = Sensor_GetZoneIndex(param_1,param_2);
  cVar2 = Sensor_GetZoneIndex(param_3,param_4);
  if (((cVar1 != -1) && (cVar2 != -1)) && (cVar1 != cVar2)) {
    local_9 = 1;
  }
  return local_9;
}


// ============================================================================
// Function: Sensor_IsInNxNBox @ 6ba67234
// ============================================================================

undefined8 Sensor_IsInNxNBox(byte param_1,byte param_2,byte param_3,byte param_4,byte param_5)

{
  uint uVar1;
  undefined8 uVar2;
  uint uVar3;
  
                    /* 0x27234  1297  Sensor_IsInNxNBox */
  uVar1 = (int)(param_5 - 1) >> 1;
  uVar3 = (int)((uint)param_1 - (uint)param_3) >> 0x1f;
  if (((int)(uVar1 & 0xff) < (int)(((uint)param_1 - (uint)param_3 ^ uVar3) - uVar3)) ||
     (uVar3 = (int)((uint)param_2 - (uint)param_4) >> 0x1f,
     (int)(uVar1 & 0xff) < (int)(((uint)param_2 - (uint)param_4 ^ uVar3) - uVar3))) {
    uVar2 = 0;
  }
  else {
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: Sensor_GetDistance @ 6ba66b17
// ============================================================================

int Sensor_GetDistance(byte param_1,byte param_2,byte param_3,byte param_4)

{
  uint uVar1;
  uint uVar2;
  
                    /* 0x26b17  1288  Sensor_GetDistance */
  uVar1 = (int)((uint)param_1 - (uint)param_3) >> 0x1f;
  uVar2 = (int)((uint)param_2 - (uint)param_4) >> 0x1f;
  return (((uint)param_2 - (uint)param_4 ^ uVar2) - uVar2) +
         (((uint)param_1 - (uint)param_3 ^ uVar1) - uVar1);
}


