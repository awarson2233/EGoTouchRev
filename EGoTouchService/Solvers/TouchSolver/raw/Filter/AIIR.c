#include "TSACoreInternal.h"

// ============================================================================
// Function: AIIR_GetCoef @ 6ba60040
// ============================================================================

char AIIR_GetCoef(ushort param_1)

{
  int iVar1;
  char local_9;
  
                    /* 0x20040  85  AIIR_GetCoef */
  if (0xfffe < ((uint)*(ushort *)(_refptr_g_tsaPrmtRam + 0x18) -
               (uint)*(ushort *)(_refptr_g_tsaPrmtRam + 0x16)) *
               (uint)*(ushort *)(_refptr_g_tsaPrmtRamConst + 2)) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/Filter/IIRFilter/AIIR_Filter.c",0x15,
                      "(((INT32U)TSAConstPrmt(aiir, scaledCoefToDeltaRatio)) * (TSAPrmt(aiir, heavyDelta) - TSAPrmt(aiir, lightDelta))) < INT16U_MAX"
                     );
    HardwareAnalyzer_SetSoftException();
  }
  if (param_1 < *(ushort *)(_refptr_g_tsaPrmtRam + 0x18)) {
    if (*(ushort *)(_refptr_g_tsaPrmtRam + 0x16) < param_1) {
      iVar1 = ((uint)param_1 - (uint)*(ushort *)(_refptr_g_tsaPrmtRam + 0x16)) *
              (uint)*(ushort *)(_refptr_g_tsaPrmtRamConst + 2);
      if (iVar1 < 0) {
        iVar1 = iVar1 + 0x3ff;
      }
      local_9 = (char)(iVar1 >> 10) + _refptr_g_tsaPrmtRam[0x1a];
    }
    else {
      local_9 = _refptr_g_tsaPrmtRam[0x1a];
    }
  }
  else {
    local_9 = _refptr_g_tsaPrmtRam[0x1b];
  }
  if ((local_9 == '\0') && (param_1 != 0)) {
    local_9 = '\x01';
  }
  return local_9;
}


