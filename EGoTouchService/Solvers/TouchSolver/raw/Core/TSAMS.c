#include "TSACoreInternal.h"

// ============================================================================
// Function: TSAMS_IsOKToEnterIDLEMode @ 6ba99ad6
// ============================================================================

bool TSAMS_IsOKToEnterIDLEMode(void)

{
  char cVar1;
  bool local_9;
  
                    /* 0x59ad6  1603  TSAMS_IsOKToEnterIDLEMode */
  cVar1 = TSAMS_IsReadyForIDLEMode();
  if (cVar1 == '\0') {
    *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c =
         *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c | 0x10000;
  }
  local_9 = cVar1 != '\0';
  if ((*_refptr_g_tsaStaticPtr)->pFrameBufferPtr <
      (void *)((ulonglong)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x46 +
              *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x80)) {
    local_9 = false;
    *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c =
         *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c | 0x2000000;
  }
  if ((*_refptr_g_tsaStaticPtr)->pFrameBufferPtr < *(void **)&(*_refptr_g_tsaStaticPtr)->field_0x80)
  {
    *(void **)&(*_refptr_g_tsaStaticPtr)->field_0x80 = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  }
  return local_9;
}


// ============================================================================
// Function: TSAMS_IsReadyForIDLEMode @ 6ba9998b
// ============================================================================

bool TSAMS_IsReadyForIDLEMode(void)

{
  char cVar1;
  bool local_9;
  
                    /* 0x5998b  1604  TSAMS_IsReadyForIDLEMode */
  local_9 = true;
  cVar1 = TSA_IsGripOKToIDLE();
  if (cVar1 == '\0') {
    local_9 = *(int *)&_refptr_g_tsaPrpt[1].field_0x2c != 0;
    if (local_9) {
      *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c =
           *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c | 0x40000;
    }
    local_9 = !local_9;
    cVar1 = BLSM_IsOKToIDLE();
    if (cVar1 == '\0') {
      local_9 = false;
      *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c =
           *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c | 0x100000;
    }
  }
  if (*(int *)(*(longlong *)_refptr_g_touchesPtr + 0x20) != 0) {
    if (*(int *)(*(longlong *)_refptr_g_touchesPtr + 0x20) == 0x400) {
      cVar1 = HardwareAnalyzer_IsNotOKToEnterIDLEMode();
      if (cVar1 == '\0') goto LAB_6ba99a66;
    }
    local_9 = false;
    *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c =
         *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c | 0x80000;
  }
LAB_6ba99a66:
  cVar1 = BLReset_InDebounce();
  if (cVar1 != '\0') {
    local_9 = false;
    *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c =
         *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c | 0x200000;
  }
  cVar1 = TouchThold_IsSwitching();
  if (cVar1 != '\0') {
    local_9 = false;
    *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c =
         *(uint *)&(*_refptr_g_tsaStaticPtr)->field_0x9c | 0x400000;
  }
  return local_9;
}


// ============================================================================
// Function: TSAMS_GetDifPtr @ 6ba91631
// ============================================================================

undefined8 TSAMS_GetDifPtr(void)

{
                    /* 0x51631  1583  TSAMS_GetDifPtr */
  return TSAFold_GridZoomOutRemap(*(undefined8 *)_refptr_g_tsaBufDif,0,2,0);
}


