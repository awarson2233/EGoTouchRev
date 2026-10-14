#include "TSACoreInternal.h"

// ============================================================================
// Function: AdaptiveIIR_Process @ 6bafad7d
// ============================================================================

int AdaptiveIIR_Process(int param_1,int param_2,byte param_3,byte param_4,int param_5)

{
  uint uVar1;
  char local_d;
  int local_c;
  
                    /* 0xbad7d  329  AdaptiveIIR_Process */
  uVar1 = param_1 - param_2 >> 0x1f;
  if ((int)((param_1 - param_2 ^ uVar1) - uVar1) < param_5) {
    local_c = (int)((uint)param_3 * param_1 + ((1 << (param_4 & 0x1f)) - (uint)param_3) * param_2) /
              (1 << (param_4 & 0x1f));
    if (local_c < param_1) {
      local_d = '\x01';
    }
    else if (param_1 < local_c) {
      local_d = -1;
    }
    else {
      local_d = '\0';
    }
    local_c = local_c + local_d;
  }
  else {
    local_c = param_1;
  }
  return local_c;
}


