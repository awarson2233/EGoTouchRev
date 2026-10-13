#include "TSACoreInternal.h"

// ============================================================================
// Function: TouchAction_Process @ 6ba7733c
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

void TouchAction_Process(void)

{
  undefined1 bPrevTouchIndex;
  byte bTouchIndex;
  
                    /* 0x3733c  2487  TouchAction_Process */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&_rdata,"TouchAction_Process");
  }
  for (bTouchIndex = 0; bTouchIndex < **(byte **)_refptr_g_touchesPtr; bTouchIndex = bTouchIndex + 1
      ) {
    bPrevTouchIndex = PrevTouch_GetIdxBasedOnCurIdx(bTouchIndex);
    if ((*(uint *)((longlong)(int)(uint)bTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                  0x21c) & 0x20) == 0) {
      TouchAction_ProcessUnit(bTouchIndex,bPrevTouchIndex);
    }
  }
  return;
}


// ============================================================================
// Function: TouchAction_PostProcess @ 6ba773eb
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

void TouchAction_PostProcess(void)

{
  undefined4 dwTouchActionResult;
  byte bTouchIndex;
  
                    /* 0x373eb  2486  TouchAction_PostProcess */
  for (bTouchIndex = 0; bTouchIndex < **(byte **)_refptr_g_touchesPtr; bTouchIndex = bTouchIndex + 1
      ) {
    if (((*(uint *)((longlong)(int)(uint)bTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                   0x21c) & 0x20) != 0) &&
       ((*(uint *)((longlong)(int)(uint)bTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                  0x1f8) & 8) != 0)) {
      if ((*(uint *)((longlong)(int)(uint)bTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1f8) & 0x1000) == 0) {
        dwTouchActionResult = 1;
      }
      else {
        dwTouchActionResult = 2;
      }
      if (((*(uint *)((longlong)(int)(uint)bTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1f8) & 0x80) == 0) &&
         (*(ushort *)
           ((longlong)(int)(uint)bTouchIndex * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x250) <
          0x14)) {
        g_flickTestFailed = 1;
      }
      *(undefined4 *)(_refptr_g_tsaPrmtDynamic + 0x70) = dwTouchActionResult;
    }
  }
  return;
}


// ============================================================================
// Function: TouchAction_ProcessUnit @ 6ba77226
// ============================================================================

void TouchAction_ProcessUnit(byte param_1,byte param_2)

{
  int local_c;
  
  local_c = *(int *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_2 * 0x478 + 0x224);
  if (local_c == 0) {
    local_c = *(int *)(_refptr_g_tsaPrmtDynamic + 0x70);
  }
  else if ((local_c != 2) &&
          (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x1f8) & 0x100) != 0 ||
           ((ulonglong)*(ushort *)(_refptr_g_tsaPrmtDynamic + 0x74) <
            (ulonglong)
            (*(longlong *)
              ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x148) -
            *(longlong *)
             ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x4b8))))
          )) {
    local_c = 2;
  }
  *(int *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x224) =
       local_c;
  return;
}


// ============================================================================
// Function: TouchAction_Init @ 6ba77220
// ============================================================================

void TouchAction_Init(void)

{
                    /* 0x37220  2484  TouchAction_Init */
  return;
}


// ============================================================================
// Function: TouchAction_IsFlickCheckPassed @ 6ba773d9
// ============================================================================

bool TouchAction_IsFlickCheckPassed(void)

{
                    /* 0x373d9  2485  TouchAction_IsFlickCheckPassed */
  return g_flickTestFailed == '\0';
}


