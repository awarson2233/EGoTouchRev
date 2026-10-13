#include "TSACoreInternal.h"

// ============================================================================
// Function: IIR_Process2 @ 6ba6021b
// ============================================================================

int IIR_Process2(int param_1,int param_2,byte param_3,byte param_4)

{
  int iVar1;
  char local_9;
  
                    /* 0x2021b  888  IIR_Process2 */
  iVar1 = (int)((uint)param_3 * param_1 + ((1 << (param_4 & 0x1f)) - (uint)param_3) * param_2) /
          (1 << (param_4 & 0x1f));
  if (iVar1 < param_1) {
    local_9 = '\x01';
  }
  else if (param_1 < iVar1) {
    local_9 = -1;
  }
  else {
    local_9 = '\0';
  }
  return iVar1 + local_9;
}


// ============================================================================
// Function: IIR_Process @ 6ba601b0
// ============================================================================

int IIR_Process(int param_1,int param_2,byte param_3,byte param_4)

{
                    /* 0x201b0  887  IIR_Process */
  return (int)((uint)param_3 * param_1 + ((1 << (param_4 & 0x1f) & 0xffU) - (uint)param_3) * param_2
              ) / (1 << (param_4 & 0x1f));
}


