#include "TSACoreInternal.h"

// ============================================================================
// Function: TSAMath_ATan2 @ 6bb429d4
// ============================================================================

int TSAMath_ATan2(ushort param_1,ushort param_2)

{
  char cVar1;
  int iVar2;
  short local_e;
  ushort local_c;
  ushort local_a;
  
                    /* 0x1029d4  1606  TSAMath_ATan2 */
  local_a = param_2;
  if ((short)param_2 < 0) {
    local_a = -param_2;
  }
  local_c = param_1;
  if ((short)param_1 < 0) {
    local_c = -param_1;
  }
  if ((local_c == 0) && (local_a == 0)) {
    local_e = 0x100;
  }
  else if (local_a < local_c) {
    cVar1 = TSAMath_ArcTanOctant(local_c,local_a);
    local_e = 0x80 - cVar1;
  }
  else {
    cVar1 = TSAMath_ArcTanOctant(local_a,local_c);
    local_e = (short)cVar1;
  }
  if (((short)param_1 < 0) || (-1 < (short)param_2)) {
    if (((short)param_1 < 0) && ((short)param_2 < 0)) {
      local_e = local_e + -0x100;
    }
    else if (((short)param_1 < 0) && (-1 < (short)param_2)) {
      local_e = -local_e;
    }
  }
  else {
    local_e = 0x100 - local_e;
  }
  iVar2 = local_e * 0xb4;
  if (iVar2 < 0) {
    iVar2 = iVar2 + 0xff;
  }
  return iVar2 >> 8;
}


// ============================================================================
// Function: TSAMath_ArcTanOctant @ 6bb428a0
// ============================================================================

int TSAMath_ArcTanOctant(int param_1,int param_2)

{
  uint uVar1;
  int iVar2;
  ulonglong uVar3;
  ulonglong uVar4;
  uint local_10;
  uint local_c;
  
                    /* 0x1028a0  1608  TSAMath_ArcTanOctant */
  local_10 = 0;
  for (local_c = 2; local_c != 0; local_c = local_c >> 1) {
    local_10 = local_10 + local_c;
    if ((uint)(byte)(&g_boundaryVectorXs)[local_10] * param_2 <
        (uint)(byte)(&g_boundaryVectorYs)[local_10] * param_1) {
      local_10 = local_10 - local_c;
    }
  }
  iVar2 = local_10 * 0x10;
  uVar3 = (ulonglong)local_10;
  uVar4 = (ulonglong)local_10;
  uVar1 = local_10 + 1;
  local_10 = 0;
  for (local_c = 8; local_c != 0; local_c = local_c >> 1) {
    local_10 = local_10 + local_c;
    if ((uint)(byte)(&g_boundaryVectorXs)[uVar3] * param_2 * 0x10 +
        ((uint)(byte)(&g_boundaryVectorXs)[uVar1] * param_2 -
        (uint)(byte)(&g_boundaryVectorXs)[uVar3] * param_2) * local_10 <
        ((uint)(byte)(&g_boundaryVectorYs)[uVar1] * param_1 -
        (uint)(byte)(&g_boundaryVectorYs)[uVar4] * param_1) * local_10 +
        (uint)(byte)(&g_boundaryVectorYs)[uVar4] * param_1 * 0x10) {
      local_10 = local_10 - local_c;
    }
  }
  return local_10 + iVar2;
}


