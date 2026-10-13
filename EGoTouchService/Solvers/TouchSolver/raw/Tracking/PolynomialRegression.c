#include "TSACoreInternal.h"

// ============================================================================
// Function: PolynomialRegression_SetComb @ 6baf7460
// ============================================================================

undefined8 PolynomialRegression_SetComb(longlong param_1,ushort param_2,byte param_3,byte param_4)

{
  byte local_16;
  byte local_15;
  ushort local_14;
  ushort local_12;
  ushort local_10;
  ushort local_e;
  byte local_c;
  byte local_b;
  byte local_a;
  byte local_9;
  
                    /* 0xb7460  1000  PolynomialRegression_SetComb */
  memset_s(param_1,param_2,0xffffffff,param_2);
  local_9 = 1;
  local_a = 0;
  for (local_b = 0; local_b < param_4; local_b = local_b + 1) {
    local_c = 0;
    local_e = (ushort)param_4 * ((ushort)local_9 + (ushort)local_a);
    for (local_10 = 0; local_10 < param_3; local_10 = local_10 + 1) {
      local_12 = (ushort)param_4 * (ushort)local_a;
      for (local_14 = 0; local_14 < local_9; local_14 = local_14 + 1) {
        if (((uint)local_e + (uint)param_4 <= (uint)param_2) &&
           ((*(char *)(param_1 + (ulonglong)local_12) == -1 ||
            ((int)(uint)local_10 <= (int)*(char *)(param_1 + (ulonglong)local_12))))) {
          *(char *)(param_1 + (ulonglong)local_e) = (char)local_10;
          local_15 = 1;
          local_16 = 0;
          while (((int)(uint)local_16 < (int)(param_4 - 1) &&
                 (*(char *)(param_1 + (int)((uint)local_16 + (uint)local_12)) != -1))) {
            *(undefined1 *)((int)((uint)local_15 + (uint)local_e) + param_1) =
                 *(undefined1 *)(param_1 + (int)((uint)local_16 + (uint)local_12));
            local_15 = local_15 + 1;
            local_16 = local_16 + 1;
          }
          local_c = local_c + 1;
          local_e = local_e + param_4;
        }
        local_12 = local_12 + param_4;
      }
    }
    local_a = local_a + local_9;
    local_9 = local_c;
  }
  return 1;
}


