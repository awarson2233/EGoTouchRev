#include "TSACoreInternal.h"

// ============================================================================
// Function: ASAStaticInit @ 6bac8481
// ============================================================================

void ASAStaticInit(longlong param_1)

{
  int iVar1;
  uint8_t *s;
  
                    /* 0x88481  111  ASAStaticInit */
  s = (uint8_t *)&g_asaStatic;
  if ((param_1 != 0) && (iVar1 = memset_s(s,0xf8,0,0xf8), iVar1 == 0)) {
    *(uint *)(s + 0x54) =
         *(uint *)((uint8_t *)*_refptr_g_asaPrmtFlash + 0xa40) |
         *(uint *)((uint8_t *)*_refptr_g_asaPrmtFlash + 0xa44);
    *(uint *)(s + 0x50) = *(uint *)(s + 0x54) & 0x4813;
    *(uint *)(s + 0x58) = *(uint *)(s + 0x54);
    *(uint *)(s + 0x5c) =
         *(uint *)((uint8_t *)*_refptr_g_asaPrmtFlash + 0xa48) |
         *(uint *)((uint8_t *)*_refptr_g_asaPrmtFlash + 0xa4c);
    *(longlong *)(s + 0x40) = param_1;
  }
  return;
}


// ============================================================================
// Function: ASA_GetRptInRange @ 6bac757f
// ============================================================================

bool ASA_GetRptInRange(void)

{
                    /* 0x8757f  221  ASA_GetRptInRange */
  return (_refptr_g_curASOut->dwRptStatusFlags & 7) != 0;
}


// ============================================================================
// Function: ASA_GetRptXPos @ 6bac762e
// ============================================================================

ushort ASA_GetRptXPos(void)

{
                    /* 0x8762e  224  ASA_GetRptXPos */
  return _refptr_g_curASOut->pReportX[4];
}


// ============================================================================
// Function: ASA_GetRptYPos @ 6bac7642
// ============================================================================

ushort ASA_GetRptYPos(void)

{
                    /* 0x87642  226  ASA_GetRptYPos */
  return _refptr_g_curASOut->pReportY[4];
}


// ============================================================================
// Function: ASA_GetRptXTilt @ 6bac7656
// ============================================================================

short ASA_GetRptXTilt(void)

{
                    /* 0x87656  225  ASA_GetRptXTilt */
  return _refptr_g_curASOut->nRptXTilt;
}


// ============================================================================
// Function: ASA_GetRptYTilt @ 6bac766a
// ============================================================================

short ASA_GetRptYTilt(void)

{
                    /* 0x8766a  227  ASA_GetRptYTilt */
  return _refptr_g_curASOut->nRptYTilt;
}


// ============================================================================
// Function: ASA_GetRptPressure @ 6bac767e
// ============================================================================

ushort ASA_GetRptPressure(void)

{
                    /* 0x8767e  223  ASA_GetRptPressure */
  return _refptr_g_curASOut->wPressure;
}


// ============================================================================
// Function: ASA_GetDim1Res @ 6bac760d
// ============================================================================

ushort ASA_GetDim1Res(void)

{
                    /* 0x8760d  180  ASA_GetDim1Res */
  return _refptr_g_asaPrmt->wField00;
}


// ============================================================================
// Function: ASA_GetDim2Res @ 6bac761d
// ============================================================================

ushort ASA_GetDim2Res(void)

{
                    /* 0x8761d  182  ASA_GetDim2Res */
  return _refptr_g_asaPrmt->wField02;
}


// ============================================================================
// Function: ASA_GetBluetoothConnectStatus @ 6bac77ec
// ============================================================================

undefined4 ASA_GetBluetoothConnectStatus(void)

{
                    /* 0x877ec  170  ASA_GetBluetoothConnectStatus */
  return *(undefined4 *)(_refptr_g_asaHpp3Freq + 4);
}


// ============================================================================
// Function: ASA_Reset @ 6bac4e3b
// ============================================================================

undefined8 ASA_Reset(void)

{
  memset_s(_refptr_g_curASOut,0xec,0,0xec);
  ASACalibration_Init();
  return 0;
}


// ============================================================================
// Function: ASA_IsEnterStylusHPP2 @ 6bac6fe8
// ============================================================================

bool ASA_IsEnterStylusHPP2(void)

{
  bool bVar1;
  
                    /* 0x86fe8  262  ASA_IsEnterStylusHPP2 */
  if (((_refptr_g_asaStatic->dwAsaAuxStatusFlags & 1) == 0) ||
     (_refptr_g_asaStatic->wActiveStylusFrameCount == 0)) {
    bVar1 = false;
  }
  else {
    bVar1 = true;
  }
  return bVar1;
}


// ============================================================================
// Function: ASA_IsEnterStylusHPP3 @ 6bac701a
// ============================================================================

undefined8 ASA_IsEnterStylusHPP3(void)

{
  undefined8 uVar1;
  
                    /* 0x8701a  263  ASA_IsEnterStylusHPP3 */
  if (((_refptr_g_asaStatic->dwAsaAuxStatusFlags & 2) == 0) ||
     (_refptr_g_asaStatic->wActiveStylusFrameCount == 0)) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: ASA_InitAFEProject @ 6bac4eb9
// ============================================================================

undefined8 ASA_InitAFEProject(undefined8 param_1,undefined8 param_2)

{
  ASA_LoadProjectPrmt(param_1,param_2);
  ASAStaticInit(param_1);
  ASA_Reset();
  ASACalibration_Init();
  return 0;
}


// ============================================================================
// Function: ASA_InitProjectFromTsa @ 6bac4e99
// ============================================================================

undefined8 ASA_InitProjectFromTsa(undefined8 param_1)

{
                    /* 0x84e99  259  ASA_InitProjectFromTsa */
  ASA_LoadProjectPrmtFromTsa(param_1);
  return 0;
}


// ============================================================================
// Function: ASA_LoadProjectPrmt @ 6bac4c75
// ============================================================================

void ASA_LoadProjectPrmt(undefined8 param_1, void *param_2)

{
  uint8_t *pFlash;

                    /* 0x84c75  279  ASA_LoadProjectPrmt */
  pFlash = (uint8_t *)param_2;
  g_curStylusHWVersion = 0;
  g_curPrmtUnit = 0;
  *_refptr_g_asaPrmtFlash = (AsaFlashPrmt_t *)param_2;
  if (param_2 != (void *)0x0) {
    StylusDataType = 0;
    _refptr_g_asaPrmt->bRxCount = pFlash[0xa28];
    _refptr_g_asaPrmt->bTxCount = pFlash[0xa29];
    _refptr_g_asaPrmt->wField00 = *(ushort *)(pFlash + 0xa68);
    _refptr_g_asaPrmt->wField02 = *(ushort *)(pFlash + 0xa6a);
    _refptr_g_asaPrmt->wField0A = 0;
    _refptr_g_asaPrmt->wField08 = 0;
    _refptr_g_asaPrmt->wField0E = 0;
    _refptr_g_asaPrmt->wField0C = 0;
    _refptr_g_asaPrmt->bField14 = 0;
    _refptr_g_asaPrmt->bField12 = 0;
    _refptr_g_asaPrmt->bField13 = 0;
    _refptr_g_asaPrmt->wField04 = 0;
    _refptr_g_asaPrmt->wField06 = 0;
    ASA_LoadStylusHWPrmt(0);
  }
  return;
}

// 笔硬件参数单元与 TSA 侧的两项参数只有 ASA 解算本体使用，随本体一起未还原。
void ASA_LoadProjectPrmtFromTsa(longlong param_1) { (void)param_1; }
void ASA_LoadStylusHWPrmt(byte param_1) { (void)param_1; }


