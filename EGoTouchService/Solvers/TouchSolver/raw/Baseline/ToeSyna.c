#include "TSACoreInternal.h"

// ============================================================================
// Function: ToeSyna_PorpertyGetMaxSig @ 6bb47483
// ============================================================================

undefined2 ToeSyna_PorpertyGetMaxSig(undefined2 param_1)

{
  undefined2 local_res8;
  
                    /* 0x107483  2482  ToeSyna_PorpertyGetMaxSig */
  local_res8 = param_1;
  if (g_toeSynaIsExcludeSide != '\0') {
    if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) == 0) {
      local_res8 = *(undefined2 *)&_refptr_g_tsaPrpt->field_0xc0;
    }
    else {
      local_res8 = *(undefined2 *)&_refptr_g_tsaPrpt->field_0xe0;
    }
  }
  return local_res8;
}


// ============================================================================
// Function: ToeSyna_PorpertyGetMinSig @ 6bb47412
// ============================================================================

undefined2 ToeSyna_PorpertyGetMinSig(undefined2 param_1)

{
  char cVar1;
  undefined2 local_res8;
  
                    /* 0x107412  2483  ToeSyna_PorpertyGetMinSig */
  cVar1 = TSAStatic_IsSafeBaselineApplied();
  if (cVar1 == '\0') {
    local_res8 = param_1;
    if (g_toeSynaIsExcludeSide != '\0') {
      if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) == 0) {
        local_res8 = *(undefined2 *)&_refptr_g_tsaPrpt->field_0xc2;
      }
      else {
        local_res8 = *(undefined2 *)&_refptr_g_tsaPrpt->field_0xe2;
      }
    }
    param_1 = local_res8;
  }
  return param_1;
}


// ============================================================================
// Function: ToeSyna_KeepSSBaseline @ 6bb474dd
// ============================================================================

bool ToeSyna_KeepSSBaseline(void)

{
                    /* 0x1074dd  2481  ToeSyna_KeepSSBaseline */
  return g_toeSynaIsExcludeSide == '\x01';
}


