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
// Function: TSAMS_GetTouchMode @ 6ba998f4
// ============================================================================

undefined4 TSAMS_GetTouchMode(void)

{
                    /* 0x598f4  1596  TSAMS_GetTouchMode */
  return *(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x2c;
}


// ============================================================================
// Function: TSAMS_GetTouchThold @ 6ba9992b
// ============================================================================

undefined2 TSAMS_GetTouchThold(void)

{
                    /* 0x5992b  1599  TSAMS_GetTouchThold */
  return *(undefined2 *)&_refptr_g_tsaPrpt[1].field_0x18;
}


// ============================================================================
// Function: TSAMS_GetSelfDifPtr @ 6ba9166f
// ============================================================================

undefined * TSAMS_GetSelfDifPtr(void)

{
                    /* 0x5166f  1595  TSAMS_GetSelfDifPtr */
  return _refptr_g_tsaBufSelf;
}


// ============================================================================
// Function: TSAMS_IsNewSafeBaselineAvailable @ 6ba916fa
// ============================================================================

undefined8 TSAMS_IsNewSafeBaselineAvailable(void)

{
  undefined8 uVar1;
  
                    /* 0x516fa  1602  TSAMS_IsNewSafeBaselineAvailable */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SAFE_BASELINE) == 0) {
    uVar1 = 0;
  }
  else {
    uVar1 = SafeBaseline_IsPushed();
  }
  return uVar1;
}


// ============================================================================
// Function: TSAMS_GetDifPtr @ 6ba91631
// ============================================================================

void TSAMS_GetDifPtr(void)

{
                    /* 0x51631  1583  TSAMS_GetDifPtr */
  TSAFold_GridZoomOutRemap(*(undefined8 *)_refptr_g_tsaBufDif,0,2,0);
  return;
}


// ============================================================================
// Function: TSAMS_GetSSDifPtr @ 6ba91662
// ============================================================================

undefined * TSAMS_GetSSDifPtr(void)

{
                    /* 0x51662  1592  TSAMS_GetSSDifPtr */
  return _refptr_g_tsaBufSSDif;
}


// ============================================================================
// Function: TSAMS_GetRawPtr @ 6ba9167c
// ============================================================================

undefined8 TSAMS_GetRawPtr(void)

{
                    /* 0x5167c  1590  TSAMS_GetRawPtr */
  return *(undefined8 *)_refptr_g_tsaBufRaw;
}


// ============================================================================
// Function: TSAMS_GetPreRawPtr @ 6ba9168c
// ============================================================================

undefined * TSAMS_GetPreRawPtr(void)

{
                    /* 0x5168c  1589  TSAMS_GetPreRawPtr */
  return _refptr_g_tsaBufPreRaw;
}


// ============================================================================
// Function: TSAMS_GetPreFilteredRawPtr @ 6ba91699
// ============================================================================

undefined8 TSAMS_GetPreFilteredRawPtr(void)

{
                    /* 0x51699  1588  TSAMS_GetPreFilteredRawPtr */
  return 0;
}


// ============================================================================
// Function: TSAMS_GetSDPtr @ 6ba916a4
// ============================================================================

undefined8 TSAMS_GetSDPtr(void)

{
                    /* 0x516a4  1591  TSAMS_GetSDPtr */
  return *(undefined8 *)_refptr_g_tsaBufSd;
}


// ============================================================================
// Function: TSAMS_GetCMNPtr @ 6ba916b4
// ============================================================================

undefined * TSAMS_GetCMNPtr(void)

{
                    /* 0x516b4  1582  TSAMS_GetCMNPtr */
  return _refptr_g_tsaBufSelf + 0x1680;
}


// ============================================================================
// Function: TSAMS_GetPreCMFDifPtr @ 6ba916c7
// ============================================================================

undefined8 TSAMS_GetPreCMFDifPtr(void)

{
  return *(undefined8 *)_refptr_g_tsaBufPreCMFDif;
}


// ============================================================================
// Function: TSAMS_GetPreFilteredDifPtr @ 6ba916d7
// ============================================================================

void TSAMS_GetPreFilteredDifPtr(void)

{
                    /* 0x516d7  1587  TSAMS_GetPreFilteredDifPtr */
  TSAMS_GetPreCMFDifPtr();
  return;
}


// ============================================================================
// Function: TSAMS_GetBLPtr @ 6ba916ea
// ============================================================================

undefined8 TSAMS_GetBLPtr(void)

{
                    /* 0x516ea  1581  TSAMS_GetBLPtr */
  return *(undefined8 *)_refptr_g_tsaBufBl;
}


// ============================================================================
// Function: TSAMS_GetSafeBLPtr @ 6ba9172d
// ============================================================================

undefined8 TSAMS_GetSafeBLPtr(undefined8 param_1)

{
  undefined8 uVar1;
  
                    /* 0x5172d  1594  TSAMS_GetSafeBLPtr */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SAFE_BASELINE) == 0) {
    uVar1 = 0;
  }
  else {
    uVar1 = SafeBaseline_GetBuf(param_1);
  }
  return uVar1;
}


// ============================================================================
// Function: TSAMS_GetNormalBLPtr @ 6ba91768
// ============================================================================

undefined8 TSAMS_GetNormalBLPtr(void)

{
                    /* 0x51768  1586  TSAMS_GetNormalBLPtr */
  return *(undefined8 *)_refptr_g_tsaBufBlNormal;
}


// ============================================================================
// Function: TSAMS_GetSSPreUnifiedBLPtr @ 6ba91778
// ============================================================================

undefined * TSAMS_GetSSPreUnifiedBLPtr(void)

{
                    /* 0x51778  1593  TSAMS_GetSSPreUnifiedBLPtr */
  SS_GetPreUnifiedBL();
  return _refptr_g_tsaBufSSPreUnifiedBaseline;
}


// ============================================================================
// Function: TSAMS_GetTouchModeStage @ 6ba99907
// ============================================================================

undefined4 TSAMS_GetTouchModeStage(void)

{
                    /* 0x59907  1597  TSAMS_GetTouchModeStage */
  return *(undefined4 *)&_refptr_g_tsaPrpt[1].field_0x30;
}


// ============================================================================
// Function: TSAMS_GetTouchSize @ 6ba9991a
// ============================================================================

undefined1 TSAMS_GetTouchSize(void)

{
                    /* 0x5991a  1598  TSAMS_GetTouchSize */
  return _refptr_g_tsaPrmtDynamic[0x6c];
}


// ============================================================================
// Function: TSAMS_IsBaselineOK @ 6ba9993f
// ============================================================================

undefined8 TSAMS_IsBaselineOK(void)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0x5993f  1600  TSAMS_IsBaselineOK */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) == 0) {
    uVar2 = BLSM_IsBaselineOK();
  }
  else {
    cVar1 = BLSM_IsBaselineOK();
    if ((cVar1 == '\0') || (cVar1 = SideTouch_IsBaselineOK(), cVar1 == '\0')) {
      uVar2 = 0;
    }
    else {
      uVar2 = 1;
    }
  }
  return uVar2;
}


// ============================================================================
// Function: TSAMS_IsExistExtROIPeak @ 6ba99bee
// ============================================================================

bool TSAMS_IsExistExtROIPeak(void)

{
  char cVar1;
  
                    /* 0x59bee  1601  TSAMS_IsExistExtROIPeak */
  cVar1 = AntiTouch_GetMaxTouchPeakIdx();
  return cVar1 != '\x14';
}


// ============================================================================
// Function: TSAMS_GetExtROIPeakDim1 @ 6ba99c16
// ============================================================================

uchar TSAMS_GetExtROIPeakDim1(void)

{
  byte bVar1;
  uchar uVar2;
  
                    /* 0x59c16  1584  TSAMS_GetExtROIPeakDim1 */
  bVar1 = AntiTouch_GetMaxTouchPeakIdx();
  if (bVar1 == 0x14) {
    uVar2 = '\0';
  }
  else {
    uVar2 = (*_refptr_g_peaksPtr)[(longlong)(int)(uint)bVar1 * 0x14 + 0x15].bPeakCount;
  }
  return uVar2;
}


// ============================================================================
// Function: TSAMS_GetExtROIPeakDim2 @ 6ba99c63
// ============================================================================

uchar TSAMS_GetExtROIPeakDim2(void)

{
  byte bVar1;
  uchar uVar2;
  
                    /* 0x59c63  1585  TSAMS_GetExtROIPeakDim2 */
  bVar1 = AntiTouch_GetMaxTouchPeakIdx();
  if (bVar1 == 0x14) {
    uVar2 = '\0';
  }
  else {
    uVar2 = (*_refptr_g_peaksPtr)[(longlong)(int)(uint)bVar1 * 0x14 + 0x16].bPeakCount;
  }
  return uVar2;
}


// ============================================================================
// Function: TSAMS_RptIsMatchWithROI @ 6ba99cb0
// ============================================================================

bool TSAMS_RptIsMatchWithROI(void)

{
  char cVar1;
  
                    /* 0x59cb0  1605  TSAMS_RptIsMatchWithROI */
  cVar1 = AntiTouch_IsRptDebounce();
  return cVar1 == '\0';
}


