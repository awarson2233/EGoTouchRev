#include "TSACoreInternal.h"

// ============================================================================
// Function: Baseline_Compensation @ 6ba46d00
// ============================================================================

void Baseline_Compensation(void)

{
  uint uVar1;
  short *local_18;
  ushort local_e;
  short local_c;
  short local_a;
  
                    /* 0x6d00  405  Baseline_Compensation */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&_rdata,"Baseline_Compensation");
  }
  local_a = 0;
  if (*(short *)&_refptr_g_tsaPrpt[1].field_0x46 < 0) {
    local_a = *(short *)&_refptr_g_tsaPrpt[1].field_0x46;
  }
  else if (0 < *(short *)&_refptr_g_tsaPrpt[1].field_0x44) {
    local_a = *(short *)&_refptr_g_tsaPrpt[1].field_0x44;
  }
  uVar1 = (int)local_a >> 0x1f;
  if ((4 < (int)(((int)local_a ^ uVar1) - uVar1)) &&
     ((*_refptr_g_tsaStaticPtr)->field_0x30a == '\0')) {
    if (local_a < 1) {
      local_c = -5;
    }
    else {
      local_c = 5;
    }
    local_e = _refptr_g_tsaPrmtFlash->wBufElementCount;
    local_18 = *(short **)_refptr_g_tsaBufBl;
    if ((*_refptr_g_tsaStaticPtr)->bField_0x11a == '\0') {
      do {
        *local_18 = *local_18 - local_c;
        local_e = local_e - 1;
        local_18 = local_18 + 1;
      } while (local_e != 0);
    }
    else {
      do {
        *local_18 = local_c + *local_18;
        local_e = local_e - 1;
        local_18 = local_18 + 1;
      } while (local_e != 0);
    }
    (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
         (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 0x100;
  }
  return;
}


