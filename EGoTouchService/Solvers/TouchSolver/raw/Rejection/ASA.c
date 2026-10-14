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
// Function: ASA_GetDim1PhyRes @ 6bac75df
// ============================================================================

undefined2 ASA_GetDim1PhyRes(void)

{
                    /* 0x875df  179  ASA_GetDim1PhyRes */
  return *(undefined2 *)&(*_refptr_g_asaPrmtFlash)->field_0xa68;
}


// ============================================================================
// Function: ASA_SetDim1Len @ 6bac7ffa
// ============================================================================

void ASA_SetDim1Len(byte param_1, ...)

{
                    /* 0x87ffa  303  ASA_SetDim1Len */
  _refptr_g_asaPrmt->bTxCount = param_1;
  return;
}


// ============================================================================
// Function: ASA_SetDim2Len @ 6bac8013
// ============================================================================

void ASA_SetDim2Len(byte param_1)

{
                    /* 0x88013  305  ASA_SetDim2Len */
  _refptr_g_asaPrmt->bRxCount = param_1;
  return;
}


// ============================================================================
// Function: ASA_SetDim1Ofst @ 6bac802c
// ============================================================================

void ASA_SetDim1Ofst(byte param_1)

{
                    /* 0x8802c  304  ASA_SetDim1Ofst */
  _refptr_g_asaPrmt->bField12 = param_1;
  return;
}


// ============================================================================
// Function: ASA_SetDim2Ofst @ 6bac8045
// ============================================================================

void ASA_SetDim2Ofst(byte param_1)

{
                    /* 0x88045  306  ASA_SetDim2Ofst */
  _refptr_g_asaPrmt->bField13 = param_1;
  return;
}


// ============================================================================
// Function: ASA_SetXRes @ 6bac805e
// ============================================================================

void ASA_SetXRes(ushort param_1)

{
                    /* 0x8805e  312  ASA_SetXRes */
  _refptr_g_asaPrmt->wField00 = param_1;
  return;
}


// ============================================================================
// Function: ASA_SetYRes @ 6bac8078
// ============================================================================

void ASA_SetYRes(ushort param_1)

{
                    /* 0x88078  316  ASA_SetYRes */
  _refptr_g_asaPrmt->wField02 = param_1;
  return;
}


// ============================================================================
// Function: ASA_SetXResOfst @ 6bac8093
// ============================================================================

void ASA_SetXResOfst(ushort param_1)

{
                    /* 0x88093  315  ASA_SetXResOfst */
  _refptr_g_asaPrmt->wField04 = param_1;
  return;
}


// ============================================================================
// Function: ASA_SetYResOfst @ 6bac80ae
// ============================================================================

void ASA_SetYResOfst(ushort param_1)

{
                    /* 0x880ae  319  ASA_SetYResOfst */
  _refptr_g_asaPrmt->wField06 = param_1;
  return;
}


// ============================================================================
// Function: ASA_SetXResClipLow @ 6bac80c9
// ============================================================================

void ASA_SetXResClipLow(ushort param_1)

{
                    /* 0x880c9  314  ASA_SetXResClipLow */
  _refptr_g_asaPrmt->wField08 = param_1;
  return;
}


// ============================================================================
// Function: ASA_SetXResClipHigh @ 6bac80e4
// ============================================================================

void ASA_SetXResClipHigh(ushort param_1)

{
                    /* 0x880e4  313  ASA_SetXResClipHigh */
  _refptr_g_asaPrmt->wField0A = param_1;
  return;
}


// ============================================================================
// Function: ASA_SetYResClipLow @ 6bac80ff
// ============================================================================

void ASA_SetYResClipLow(ushort param_1)

{
                    /* 0x880ff  318  ASA_SetYResClipLow */
  _refptr_g_asaPrmt->wField0C = param_1;
  return;
}


// ============================================================================
// Function: ASA_SetYResClipHigh @ 6bac811a
// ============================================================================

void ASA_SetYResClipHigh(ushort param_1)

{
                    /* 0x8811a  317  ASA_SetYResClipHigh */
  _refptr_g_asaPrmt->wField0E = param_1;
  return;
}


// ============================================================================
// Function: ASA_SetFoldState @ 6bac82ff
// ============================================================================

void ASA_SetFoldState(byte param_1,ulonglong param_2,ulonglong param_3)

{
                    /* 0x882ff  307  ASA_SetFoldState */
  _refptr_g_asaPrmt->bField14 = param_1;
  _refptr_g_asaPrmt->qwField18 = param_2;
  _refptr_g_asaPrmt->qwField20 = param_3;
  return;
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
// Function: ASA_GetGridTiedHandPtr @ 6bac7e54
// ============================================================================

ushort * ASA_GetGridTiedHandPtr(char param_1)

{
  ushort *puVar1;
  
                    /* 0x87e54  196  ASA_GetGridTiedHandPtr */
  if (param_1 == '\0') {
    puVar1 = _refptr_g_asaData->pPeakFlagMap;
  }
  else {
    puVar1 = _refptr_g_asaData->pPeakFlagMap + 0x992;
  }
  return puVar1;
}


// ============================================================================
// Function: ASA_GetGridTiedOriTx1Ptr @ 6bac7e81
// ============================================================================

ushort * ASA_GetGridTiedOriTx1Ptr(char param_1)

{
  ushort *puVar1;
  
                    /* 0x87e81  197  ASA_GetGridTiedOriTx1Ptr */
  if (param_1 == '\0') {
    puVar1 = _refptr_g_asaData->pPeakFlagMapBackup;
  }
  else {
    puVar1 = _refptr_g_asaData->pPeakFlagMapBackup + 0x992;
  }
  return puVar1;
}


// ============================================================================
// Function: ASA_GetGridTiedOriTx2Ptr @ 6bac7eae
// ============================================================================

ushort * ASA_GetGridTiedOriTx2Ptr(char param_1)

{
  ushort *puVar1;
  
                    /* 0x87eae  198  ASA_GetGridTiedOriTx2Ptr */
  if (param_1 == '\0') {
    puVar1 = _refptr_g_asaData->pNormalGrid1Primary;
  }
  else {
    puVar1 = _refptr_g_asaData->pNormalGrid1Primary + 0x992;
  }
  return puVar1;
}


// ============================================================================
// Function: ASA_GetGridTiedDeTraceNoiseTx1Ptr @ 6bac7edb
// ============================================================================

ushort * ASA_GetGridTiedDeTraceNoiseTx1Ptr(char param_1)

{
  ushort *puVar1;
  
                    /* 0x87edb  194  ASA_GetGridTiedDeTraceNoiseTx1Ptr */
  if (param_1 == '\0') {
    puVar1 = _refptr_g_asaData->pNormalGrid2Primary;
  }
  else {
    puVar1 = _refptr_g_asaData->pNormalGrid2Primary + 0x992;
  }
  return puVar1;
}


// ============================================================================
// Function: ASA_GetGridTiedDeTraceNoiseTx2Ptr @ 6bac7f08
// ============================================================================

byte * ASA_GetGridTiedDeTraceNoiseTx2Ptr(char param_1)

{
  byte *pbVar1;
  
                    /* 0x87f08  195  ASA_GetGridTiedDeTraceNoiseTx2Ptr */
  if (param_1 == '\0') {
    pbVar1 = _refptr_g_asaData->pPad_9ce0;
  }
  else {
    pbVar1 = _refptr_g_asaData->pPad_9ce0 + 0x1324;
  }
  return pbVar1;
}


// ============================================================================
// Function: ASA_GetGridTiedDePalmNoiseTx1Ptr @ 6bac7f35
// ============================================================================

byte * ASA_GetGridTiedDePalmNoiseTx1Ptr(char param_1)

{
  byte *pbVar1;
  
                    /* 0x87f35  192  ASA_GetGridTiedDePalmNoiseTx1Ptr */
  if (param_1 == '\0') {
    pbVar1 = _refptr_g_asaData->pPad_9ce0 + 0x2648;
  }
  else {
    pbVar1 = _refptr_g_asaData->pPad_9ce0 + 0x396c;
  }
  return pbVar1;
}


// ============================================================================
// Function: ASA_GetGridTiedDePalmNoiseTx2Ptr @ 6bac7f62
// ============================================================================

byte * ASA_GetGridTiedDePalmNoiseTx2Ptr(char param_1)

{
  byte *pbVar1;
  
                    /* 0x87f62  193  ASA_GetGridTiedDePalmNoiseTx2Ptr */
  if (param_1 == '\0') {
    pbVar1 = _refptr_g_asaData->pPad_9ce0 + 0x4c90;
  }
  else {
    pbVar1 = _refptr_g_asaData->pPad_9ce0 + 0x5fb4;
  }
  return pbVar1;
}


// ============================================================================
// Function: ASA_GetTx1LineDataPtr @ 6bac7726
// ============================================================================

AsaDataRuntimeV2 * ASA_GetTx1LineDataPtr(void)

{
  AsaDataRuntimeV2 *pAVar1;
  
                    /* 0x87726  243  ASA_GetTx1LineDataPtr */
  if (*_refptr_g_flagHPP3Protocol == '\0') {
    pAVar1 = _refptr_g_asaData;
  }
  else {
    pAVar1 = (AsaDataRuntimeV2 *)(_refptr_g_asaData->lineProfiles).pLineSumPrimary;
  }
  return pAVar1;
}


// ============================================================================
// Function: ASA_GetTx2LineDataPtr @ 6bac7750
// ============================================================================

ushort * ASA_GetTx2LineDataPtr(void)

{
  ushort *puVar1;
  
                    /* 0x87750  245  ASA_GetTx2LineDataPtr */
  if (*_refptr_g_flagHPP3Protocol == '\0') {
    puVar1 = (_refptr_g_asaData->lineProfiles).pLineSumPrimary;
  }
  else {
    puVar1 = (_refptr_g_asaData->lineProfiles).pLineSumBackup;
  }
  return puVar1;
}


// ============================================================================
// Function: ASA_GetTx1PreCmfLineDataPtr @ 6bac7cc4
// ============================================================================

ushort * ASA_GetTx1PreCmfLineDataPtr(void)

{
  ushort *puVar1;
  
                    /* 0x87cc4  244  ASA_GetTx1PreCmfLineDataPtr */
  if (*_refptr_g_flagHPP3Protocol == '\0') {
    puVar1 = (_refptr_g_asaData->lineProfiles).pLineSumPrimary;
  }
  else {
    puVar1 = (ushort *)(_refptr_g_asaData->pPad_9ce0 + 0x72d8);
  }
  return puVar1;
}


// ============================================================================
// Function: ASA_GetTx2PreCmfLineDataPtr @ 6bac7cf4
// ============================================================================

ushort * ASA_GetTx2PreCmfLineDataPtr(void)

{
  ushort *puVar1;
  
                    /* 0x87cf4  246  ASA_GetTx2PreCmfLineDataPtr */
  if (*_refptr_g_flagHPP3Protocol == '\0') {
    puVar1 = (_refptr_g_asaData->lineProfiles).pLineSumPrimary;
  }
  else {
    puVar1 = (ushort *)(_refptr_g_asaData->pPad_9ce0 + 0x77d0);
  }
  return puVar1;
}


// ============================================================================
// Function: ASA_GetOriDataToShowTX1Ptr @ 6bac7bd8
// ============================================================================

AsaDataRuntimeV2 * ASA_GetOriDataToShowTX1Ptr(void)

{
  AsaDataRuntimeV2 *pAVar1;
  
                    /* 0x87bd8  205  ASA_GetOriDataToShowTX1Ptr */
  if (*_refptr_g_flagHPP3Protocol == '\0') {
    pAVar1 = _refptr_g_asaData;
  }
  else {
    pAVar1 = (AsaDataRuntimeV2 *)_refptr_g_asaData->pNormalGrid1Search;
  }
  return pAVar1;
}


// ============================================================================
// Function: ASA_GetOriDataToShowTX2Ptr @ 6bac7c02
// ============================================================================

AsaDataRuntimeV2 * ASA_GetOriDataToShowTX2Ptr(void)

{
  AsaDataRuntimeV2 *pAVar1;
  
                    /* 0x87c02  206  ASA_GetOriDataToShowTX2Ptr */
  if (*_refptr_g_flagHPP3Protocol == '\0') {
    pAVar1 = _refptr_g_asaData;
  }
  else {
    pAVar1 = (AsaDataRuntimeV2 *)_refptr_g_asaData->pNormalGrid2Search;
  }
  return pAVar1;
}


// ============================================================================
// Function: ASA_GetPeakFlagToShowTX1Ptr @ 6bac7c2c
// ============================================================================

ushort * ASA_GetPeakFlagToShowTX1Ptr(void)

{
                    /* 0x87c2c  207  ASA_GetPeakFlagToShowTX1Ptr */
  return _refptr_g_asaData->pPeakFlagMap;
}


// ============================================================================
// Function: ASA_GetPeakFlagToShowTX2Ptr @ 6bac7c3f
// ============================================================================

ushort * ASA_GetPeakFlagToShowTX2Ptr(void)

{
                    /* 0x87c3f  208  ASA_GetPeakFlagToShowTX2Ptr */
  return _refptr_g_asaData->pPeakFlagMapBackup;
}


// ============================================================================
// Function: ASA_GetGridNopressTableLinePtr @ 6bac7de4
// ============================================================================

ushort * ASA_GetGridNopressTableLinePtr(void)

{
                    /* 0x87de4  187  ASA_GetGridNopressTableLinePtr */
  return _refptr_g_noPressPara->pGridNopressTableLine;
}


// ============================================================================
// Function: ASA_GetGridNopressTablePtr @ 6bac7df7
// ============================================================================

ushort * ASA_GetGridNopressTablePtr(void)

{
                    /* 0x87df7  188  ASA_GetGridNopressTablePtr */
  return _refptr_g_noPressPara->pGridNopressTable;
}


// ============================================================================
// Function: ASA_GetGridNopressTableShortTermPtr @ 6bac7e0a
// ============================================================================

ushort * ASA_GetGridNopressTableShortTermPtr(void)

{
                    /* 0x87e0a  189  ASA_GetGridNopressTableShortTermPtr */
  return _refptr_g_noPressPara->pGridNopressShortTermTable;
}


// ============================================================================
// Function: ASA_GetGridNopressTableTx2Ptr @ 6bac7e1b
// ============================================================================

ushort * ASA_GetGridNopressTableTx2Ptr(void)

{
                    /* 0x87e1b  191  ASA_GetGridNopressTableTx2Ptr */
  return _refptr_g_noPressPara->pGridNopressTableTx2;
}


// ============================================================================
// Function: ASA_GetGridNopressTableShortTermTx2Ptr @ 6bac7e2e
// ============================================================================

ushort * ASA_GetGridNopressTableShortTermTx2Ptr(void)

{
                    /* 0x87e2e  190  ASA_GetGridNopressTableShortTermTx2Ptr */
  return _refptr_g_noPressPara->pGridNopressShortTermTx2Table;
}


// ============================================================================
// Function: ASA_GetGridNopressSafeTablePtr @ 6bac7e41
// ============================================================================

ushort * ASA_GetGridNopressSafeTablePtr(void)

{
                    /* 0x87e41  186  ASA_GetGridNopressSafeTablePtr */
  return _refptr_g_noPressPara->pGridNopressSafeTable;
}


// ============================================================================
// Function: ASA_GetFrameBypass @ 6bac76e7
// ============================================================================

undefined8 ASA_GetFrameBypass(void)

{
  undefined8 uVar1;
  
                    /* 0x876e7  185  ASA_GetFrameBypass */
  if (((_refptr_g_asaStatic->bBypassCurFrame == 1) || (_refptr_g_asaStatic->bRawAbnormal != 0)) ||
     (_refptr_g_asaStatic->bCmnAbnormal != 0)) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
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


