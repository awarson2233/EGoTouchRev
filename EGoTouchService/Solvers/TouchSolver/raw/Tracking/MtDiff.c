#include "TSACoreInternal.h"

// ============================================================================
// Function: MtDiff_MsLinearCorrectionFilterCore @ 6ba55131
// ============================================================================

void MtDiff_MsLinearCorrectionFilterCore
               (longlong param_1,longlong param_2,undefined8 param_3,longlong *param_4)

{
  byte bVar1;
  longlong lVar2;
  byte local_9;
  
                    /* 0x15131  929  MtDiff_MsLinearCorrectionFilterCore */
  lVar2 = *param_4;
  bVar1 = *(byte *)(param_4 + 2);
  MtLsm_LinearRegressionWeight(param_4,param_2,bVar1);
  for (local_9 = 0; local_9 < bVar1; local_9 = local_9 + 1) {
    *(short *)(param_1 + (ulonglong)local_9 * 2) =
         *(short *)(lVar2 + (ulonglong)local_9 * 2) - *(short *)(param_2 + (ulonglong)local_9 * 2);
  }
  return;
}


