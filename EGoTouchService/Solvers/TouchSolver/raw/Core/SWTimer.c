#include "TSACoreInternal.h"

// ============================================================================
// Function: SWTimer_Stop @ 6bb42802
// ============================================================================

void SWTimer_Stop(undefined1 *param_1)

{
                    /* 0x102802  1210  SWTimer_Stop */
  if (param_1 != (undefined1 *)0x0) {
    *param_1 = 0;
  }
  return;
}


// ============================================================================
// Function: SWTimer_Start @ 6bb427d0
// ============================================================================

void SWTimer_Start(undefined4 param_1,char *param_2)

{
                    /* 0x1027d0  1209  SWTimer_Start */
  if ((param_2 != (char *)0x0) && (*param_2 == '\0')) {
    *param_2 = '\x01';
    *(undefined4 *)(param_2 + 4) = param_1;
  }
  return;
}


// ============================================================================
// Function: SWTimer_IsExpired @ 6bb42843
// ============================================================================

undefined8 SWTimer_IsExpired(int param_1,char *param_2,uint param_3)

{
  undefined8 uVar1;
  
                    /* 0x102843  1207  SWTimer_IsExpired */
  if (param_2 == (char *)0x0) {
    uVar1 = 2;
  }
  else if (*param_2 == '\0') {
    uVar1 = 2;
  }
  else if ((uint)(param_1 - *(int *)(param_2 + 4)) < param_3) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


