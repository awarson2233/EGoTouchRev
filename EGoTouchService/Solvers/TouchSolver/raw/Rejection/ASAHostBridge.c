#include "TSACoreInternal.h"

// ASA（主动笔解算）的本体没有还原：笔的坐标 / 压力 / 倾角由业务侧的 StylusSolver 解出。
// 手指管线读 ASA 状态的那一层在这个文件里：
//   - TSA_ASAProcess 的外壳、笔模式状态机、蓝牙状态、有效性判定按原函数还原；
//   - ASA_MainProcess 里「从网格数据解出报告」的部分换成宿主每帧送进来的报告
//     （TSAHost_SetStylusReport），对应 ASAHost_MainProcess。
// ASA 本体还原后，把 ASAHost_MainProcess 换回 ASA_MainProcess 即可，其余函数不用动。

// AFE_Frame +0x40 指向的笔帧（himax_thp_drv 的 g_StylusOut）。
struct stylusFrame {
  ushort *pTx1LineData;       // +0x00
  ushort *pTx2LineData;       // +0x08
  ushort wMainFreq;           // +0x10
  ushort wAuxiFreq;           // +0x12
  ushort wPressure;           // +0x14
  ushort _pad16;
  uint dwButtonBits;          // +0x18
  uint dwAuxStatusFlags;      // +0x1c  bit0 HPP2，bit1 HPP3，bit2 无笔数据，bit4 笔信号有效
};

_Static_assert(sizeof(stylusFrame) == 0x20, "stylusFrame layout");
_Static_assert(offsetof(AsaOut_t, pReportX) + 4 * sizeof(ushort) == 0x9c, "ASA_GetRptXPos reads +0x9c");
_Static_assert(offsetof(AsaOut_t, pReportY) + 4 * sizeof(ushort) == 0xae, "ASA_GetRptYPos reads +0xae");
_Static_assert(offsetof(AsaOut_t, nRptXTilt) == 0xd4, "ASA_GetRptXTilt reads +0xd4");
_Static_assert(offsetof(AsaOut_t, wPressure) == 0xd8, "ASA_GetRptPressure reads +0xd8");
_Static_assert(offsetof(AsaOut_t, dwRptStatusFlags) == 0xe8, "ASA_GetRptInRange reads +0xe8");
_Static_assert(offsetof(AsaStatic_t, dwHpp3FeatureFlags) == 0x54, "AsaStatic_t layout");

// 宿主送来的本帧笔报告。dwStatusFlags 沿用 g_asaStatic.dwAsaStatusFlags 的含义：
// bit0 悬停在场，bit1 书写（有压力）；手指侧只看「低 3 位非零 = 笔在场」。
static struct {
  uint dwStatusFlags;
  ushort wXPos;       // ASA 坐标系（驱动倒序后的方向），量程 ASA_GetDim1Res / ASA_GetDim2Res
  ushort wYPos;
  short nXTilt;       // 度
  short nYTilt;
  ushort wPressure;
} s_hostReport;

void TSAHost_SetStylusReport(uint dwStatusFlags,ushort wXPos,ushort wYPos,short nXTilt,short nYTilt,
                             ushort wPressure)

{
  s_hostReport.dwStatusFlags = dwStatusFlags;
  s_hostReport.wXPos = wXPos;
  s_hostReport.wYPos = wYPos;
  s_hostReport.nXTilt = nXTilt;
  s_hostReport.nYTilt = nYTilt;
  s_hostReport.wPressure = wPressure;
  return;
}


// ============================================================================
// Function: GetRealtime @ 6bac8400
// ============================================================================

ulonglong GetRealtime(void)

{
                    /* 0x88400  632  GetRealtime */
  return g_asaRealtime;
}


// ============================================================================
// Function: BtPressureInit @ 6babf350
// ============================================================================

void BtPressureInit(void)

{
                    /* 0x7f350  444  BtPressureInit */
  *(ushort *)_refptr_g_btPressBuf = 0;
  ((ushort *)_refptr_g_btPressBuf)[1] = 0;
  ((ushort *)_refptr_g_btPressBuf)[2] = 0;
  ((ushort *)_refptr_g_btPressBuf)[3] = 0;
  return;
}


// ============================================================================
// Function: ASA_SetBluetoothStatus @ 6bac72d3
// ============================================================================

void ASA_SetBluetoothStatus(uint param_1)

{
                    /* 0x872d3  301  ASA_SetBluetoothStatus */
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"\n[BT IN] status:%4d",param_1);
  }
  g_lastBtCmdTime = GetRealtime();
  if ((param_1 & 1) == 0) {
    *(undefined4 *)(_refptr_g_asaHpp3Freq + 4) = 0;
  }
  else {
    *(undefined4 *)(_refptr_g_asaHpp3Freq + 4) = 1;
  }
  return;
}


// ============================================================================
// Function: ASA_SetBluetoothPenStatus @ 6bac7135
// ============================================================================

void ASA_SetBluetoothPenStatus(undefined1 param_1)

{
                    /* 0x87135  299  ASA_SetBluetoothPenStatus */
  *_refptr_g_btPenStatus = param_1;
  return;
}


// ============================================================================
// Function: ASA_SetBluetoothPressure @ 6bac714d
// ============================================================================

// 原函数随后还有 HPP3_ClearBtPressCnt / MultiPanelSwitch* / HPP3_InkLeakageJudge，
// 都是 ASA 解算本体的内部状态，这里不保留。
void ASA_SetBluetoothPressure(ushort param_1,ushort param_2,ushort param_3,ushort param_4)

{
                    /* 0x8714d  300  ASA_SetBluetoothPressure */
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"\n[BT IN] Press_Buf:%4d,%4d,%4d,%4d",param_1,param_2,param_3,param_4);
  }
  g_lastBtCmdTime = GetRealtime();
  *(undefined4 *)(_refptr_g_asaHpp3Freq + 4) = 1;
  *(ushort *)_refptr_g_btPressBuf = param_1;
  ((ushort *)_refptr_g_btPressBuf)[1] = param_2;
  ((ushort *)_refptr_g_btPressBuf)[2] = param_3;
  ((ushort *)_refptr_g_btPressBuf)[3] = param_4;
  return;
}


// ============================================================================
// Function: TSA_SetASABluetoothConnectStatus @ 6ba9cd02
// ============================================================================

void TSA_SetASABluetoothConnectStatus(undefined4 param_1)

{
                    /* 0x5cd02  2261  TSA_SetASABluetoothConnectStatus */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) != 0) {
    ASA_SetBluetoothStatus(param_1);
  }
  return;
}


// ============================================================================
// Function: TSA_SetASABluetoothPenStatus @ 6ba9cc2d
// ============================================================================

void TSA_SetASABluetoothPenStatus(undefined1 param_1)

{
                    /* 0x5cc2d  2264  TSA_SetASABluetoothPenStatus */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) != 0) {
    ASA_SetBluetoothPenStatus(param_1);
  }
  return;
}


// ============================================================================
// Function: TSA_SetASABluetoothPressure @ 6ba9cc65
// ============================================================================

void TSA_SetASABluetoothPressure
               (undefined2 param_1,undefined2 param_2,undefined2 param_3,undefined2 param_4)

{
                    /* 0x5cc65  2265  TSA_SetASABluetoothPressure */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) != 0) {
    ASA_SetBluetoothPressure(param_1,param_2,param_3,param_4);
  }
  return;
}


// ============================================================================
// Function: TSA_RptASAInRange @ 6ba9c7e9
// ============================================================================

undefined8 TSA_RptASAInRange(void)

{
  undefined8 uVar1;

                    /* 0x5c7e9  2142  TSA_RptASAInRange */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) == 0) {
    uVar1 = 0;
  }
  else {
    uVar1 = ASA_GetRptInRange();
  }
  return uVar1;
}


// ============================================================================
// Function: HPP3_BtStatusProcess @ 6bab778a
// ============================================================================

void HPP3_BtStatusProcess(void)

{
  ulonglong uVar1;

  uVar1 = GetRealtime();
  if ((g_lastBtCmdTime + 60000U < uVar1) && (*(int *)(_refptr_g_asaHpp3Freq + 4) == 1)) {
    *(undefined4 *)(_refptr_g_asaHpp3Freq + 4) = 2;
  }
  else {
    uVar1 = GetRealtime();
    if (g_lastBtCmdTime + 1000U < uVar1) {
      BtPressureInit();
    }
  }
  return;
}


// ============================================================================
// Function: ValidJudgment @ 6bac4efc
// ============================================================================

/* Validate stylusFrame protocol/mode bits before ASA processing. bit0=HPP2 protocol, bit1=HPP3
   protocol, bit2/bit3/bit4 are mutually constrained mode bits. HPP2 requires pTx1LineData unless
   grid-mode bit2 is set; HPP3 additionally checks pTx2LineData. */

bool ValidJudgment(stylusFrame *pStylusFrame)

{
  bool bVar1;

                    /* 0x84efc  2576  ValidJudgment */
  if (((pStylusFrame->dwAuxStatusFlags & 1) == 0) || ((pStylusFrame->dwAuxStatusFlags & 2) == 0)) {
    if (((pStylusFrame->dwAuxStatusFlags & 1) == 0) && ((pStylusFrame->dwAuxStatusFlags & 2) == 0))
    {
      if (*_refptr_g_tsaLogLoglevel != '\0') {
        StrPrintf_Process(0,"\n PROTOCOL SET ERROR! MUST SET PROTOCL");
      }
      bVar1 = false;
    }
    else if ((((pStylusFrame->dwAuxStatusFlags & 4) == 0) &&
             ((pStylusFrame->dwAuxStatusFlags & 8) == 0)) &&
            ((pStylusFrame->dwAuxStatusFlags & 0x10) == 0)) {
      if (*_refptr_g_tsaLogLoglevel != '\0') {
        StrPrintf_Process(0,"\n MODE ERROR!");
      }
      bVar1 = false;
    }
    else if ((((pStylusFrame->dwAuxStatusFlags & 4) == 0) ||
             ((pStylusFrame->dwAuxStatusFlags & 8) == 0)) ||
            ((pStylusFrame->dwAuxStatusFlags & 0x10) == 0)) {
      if ((pStylusFrame->pTx1LineData == (ushort *)0x0) &&
         ((pStylusFrame->dwAuxStatusFlags & 4) == 0)) {
        if (*_refptr_g_tsaLogLoglevel != '\0') {
          StrPrintf_Process(0,"\n STYLUS FRAME TX1 LINE DATA NULL");
        }
        bVar1 = false;
      }
      else {
        if (((pStylusFrame->dwAuxStatusFlags & 2) != 0) &&
           (((pStylusFrame->pTx2LineData == (ushort *)0x0 &&
             ((pStylusFrame->dwAuxStatusFlags & 4) == 0)) && (*_refptr_g_tsaLogLoglevel != '\0'))))
        {
          StrPrintf_Process(0,"\n STYLUS FRAME TX2 LINE DATA NULL");
        }
        bVar1 = true;
      }
    }
    else {
      if (*_refptr_g_tsaLogLoglevel != '\0') {
        StrPrintf_Process(0,"\n MODE ERROR!");
      }
      bVar1 = false;
    }
  }
  else {
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"\n PROTOCOL SET ERROR! MUST SELECT ONE PROTOCOL");
    }
    bVar1 = false;
  }
  return bVar1;
}


// ============================================================================
// Function: ASAStaticCounterClean @ 6bac8414
// ============================================================================

void ASAStaticCounterClean(void)

{
                    /* 0x88414  110  ASAStaticCounterClean */
  _refptr_g_asaStatic->wActiveStylusFrameCount = 0;
  _refptr_g_asaStatic->wF1HistoryFrameCount = 0;
  _refptr_g_asaStatic->wF2HistoryFrameCount = 0;
  *(undefined2 *)(_refptr_g_asaStatic->raw + 6) = 0;
  *(undefined2 *)(_refptr_g_asaStatic->raw + 8) = 0;
  *(undefined2 *)(_refptr_g_asaStatic->raw + 10) = 0;
  *(undefined2 *)(_refptr_g_asaStatic->raw + 12) = 0;
  *(undefined2 *)(_refptr_g_asaStatic->raw + 14) = 0;
  return;
}


// ============================================================================
// Function: ASAOutClean @ 6bac30f0
// ============================================================================

void ASAOutClean(void)

{
                    /* 0x830f0  105  ASAOutClean */
  memset_s(_refptr_g_curASOut,0xec,0,0xec);
  return;
}


// ============================================================================
// Function: ASAStaticPreProcess @ 6bac89f0
// ============================================================================

void ASAStaticPreProcess(void)

{
                    /* 0x889f0  113  ASAStaticPreProcess */
  _refptr_g_asaStatic->dwAsaStatusFlags = 0;
  return;
}


// ============================================================================
// Function: ASAStaticPostProcess @ 6bac8a04
// ============================================================================

void ASAStaticPostProcess(void)

{
                    /* 0x88a04  112  ASAStaticPostProcess */
  *(uint *)(_refptr_g_asaStatic->raw + 0x58) = _refptr_g_asaStatic->dwHpp3FeatureFlags;
  _refptr_g_asaStatic->dwPrevAsaStatusFlags = _refptr_g_asaStatic->dwAsaStatusFlags;
  return;
}


// ============================================================================
// Function: EnterActiveStylusMode @ 6bac858b
// ============================================================================

// 原函数还按 HPP2 的主频累计 wF1/wF2HistoryFrameCount，本机型只有 HPP3，这两个计数只有
// ASA 解算本体读取。
void EnterActiveStylusMode(void)

{
                    /* 0x8858b  560  EnterActiveStylusMode */
  _refptr_g_asaStatic->wActiveStylusFrameCount = _refptr_g_asaStatic->wActiveStylusFrameCount + 1;
  _refptr_g_asaStatic->dwAsaStatusFlags = 0;
  *(undefined1 *)(_refptr_g_asaStatic->raw + 0xec) = 0;
  *(undefined1 *)(_refptr_g_asaStatic->raw + 0xed) = 1;
  return;
}


// ============================================================================
// Function: ExitActiveStylusMode @ 6bac87b1
// ============================================================================

// 原函数还调用 CoorInit / ExitInRangeMode / AnimationInit，复位的是 ASA 解算本体的坐标
// 滤波与动画状态。
void ExitActiveStylusMode(void)

{
                    /* 0x887b1  586  ExitActiveStylusMode */
  ASAStaticCounterClean();
  _refptr_g_asaStatic->dwAsaStatusFlags = 0;
  *(undefined1 *)(_refptr_g_asaStatic->raw + 0xec) = 0;
  *(undefined1 *)(_refptr_g_asaStatic->raw + 0xed) = 1;
  ASAOutClean();
  memcpy(&g_prevASOut,_refptr_g_curASOut,0xec);
  ASAStaticPostProcess();
  return;
}


// ============================================================================
// Function: ASAStaticStatusPreProcess @ 6bac883d
// ============================================================================

void ASAStaticStatusPreProcess(uint auxStatusFlags)

{
                    /* 0x8883d  115  ASAStaticStatusPreProcess */
  if ((auxStatusFlags & 0x10) == 0) {
    ExitActiveStylusMode();
  }
  else {
    EnterActiveStylusMode();
  }
  return;
}


// ============================================================================
// Function: ASA_IsHpp3TouchEnableFeatureEnabled @ 6bac8a70
// ============================================================================

bool ASA_IsHpp3TouchEnableFeatureEnabled(void)

{
                    /* 0x88a70  272  ASA_IsHpp3TouchEnableFeatureEnabled */
  return (_refptr_g_asaStatic->dwHpp3FeatureFlags & 2) != 0;
}


// ============================================================================
// Function: ASA_IsTouchNull @ 6bac704c
// ============================================================================

undefined8 ASA_IsTouchNull(longlong param_1)

{
  char cVar1;
  undefined8 uVar2;

                    /* 0x8704c  274  ASA_IsTouchNull */
  if (((*(uint *)(param_1 + 0x1c) & 1) == 0) || ((*(uint *)(param_1 + 0x1c) & 4) != 0)) {
    cVar1 = ASA_IsHpp3TouchEnableFeatureEnabled();
    if ((cVar1 == '\0') &&
       ((((*(uint *)(param_1 + 0x1c) & 2) != 0 && ((*(uint *)(param_1 + 0x1c) & 4) == 0)) &&
        ((*(uint *)(param_1 + 0x1c) & 8) == 0)))) {
      uVar2 = 1;
    }
    else {
      uVar2 = 0;
    }
  }
  else {
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: ASA_NeedRecordTrigger @ 6bac7780
// ============================================================================

bool ASA_NeedRecordTrigger(void)

{
                    /* 0x87780  286  ASA_NeedRecordTrigger */
  return _refptr_g_asaStatic->wActiveStylusFrameCount == 1;
}


// ============================================================================
// Function: HardwareAnalyzer_ASA_Process @ 6baf1bc2
// ============================================================================

// 原函数先查 ASA_BlTimeout / ASA_AbnormalModeSwitch / ASA_AbnormalFreqHop / ASA_LargeNoise /
// ASA_LargeNoise2，这五个在原 DLL 里都恒返回 0。
void HardwareAnalyzer_ASA_Process(void)

{
                    /* 0xb1bc2  856  HardwareAnalyzer_ASA_Process */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_ASA_ENABLED) != 0) {
    HardwareAnalyzer_ReportToTSA();
  }
  return;
}


// 笔信号消失那一帧的释放报告，对应 ReleaseASAReportExitStylus @ 6bac527b：书写中掉信号时
// 先给一帧撤掉压力的悬停，悬停中掉信号则直接清空。返回 true 表示已经完全退出。
// 原函数里的跳频期间保持（ReleaseASAReportInFreqShifting）、侧键释放与边缘补点
// （EdgeCoorProcessExitStylusWithInk）属于 ASA 解算本体，这里走它们都不成立的那一支。
static bool ASAHost_ReleaseReportExitStylus(void)

{
  bool bReleased;
  AsaOut_t *pPrevOut;

  bReleased = false;
  pPrevOut = (AsaOut_t *)&g_prevASOut;
  if (((pPrevOut->dwRptStatusFlags & 2) == 0) && ((pPrevOut->dwRptStatusFlags & 4) == 0)) {
    bReleased = true;
    ASAOutClean();
    ASAStaticCounterClean();
  }
  else {
    memcpy(_refptr_g_curASOut,pPrevOut,0xec);
    _refptr_g_curASOut->dwRptStatusFlags = 1;
    _refptr_g_curASOut->wPressure = 0;
  }
  memcpy(pPrevOut,_refptr_g_curASOut,0xec);
  return bReleased;
}


// 顶替 ASA_MainProcess @ 6bac6b5f。控制流与返回值（0 正常 / 1 无效复位 / 3 释放或无笔数据）
// 与原函数一致；原函数在 ASAOutClean 之后跑 HPP3_UpdataStylus2Buf / HPP3_DataProcess /
// ASA_CoorPostProcess 得到本帧报告，这里直接取宿主送来的报告。
// HPP3_FreqShiftProcess 只保留开头的 HPP3_BtStatusProcess，笔的跳频握手在业务侧。
static int ASAHost_MainProcess(AFE_Frame *pFrame)

{
  stylusFrame *pStylusFrame;
  int iVar3;

  g_asaRealtime = (longlong)pFrame->nUnixTimeSec * 1000 + (longlong)(pFrame->nUnixTimeSubSec / 1000);
  pStylusFrame = (stylusFrame *)pFrame->pStylusData;
  _refptr_g_asaStatic->dwAsaAuxStatusFlags = pStylusFrame->dwAuxStatusFlags;
  if ((pStylusFrame->dwAuxStatusFlags & 2) != 0) {
    HPP3_BtStatusProcess();
  }
  if (ValidJudgment(pStylusFrame)) {
    if (((_refptr_g_asaStatic->dwPrevAsaAuxStatusFlags & 0x10) == 0) ||
       ((_refptr_g_asaStatic->dwAsaAuxStatusFlags & 0x10) != 0)) {
      ASAStaticStatusPreProcess(pStylusFrame->dwAuxStatusFlags);
      _refptr_g_asaStatic->dwPrevAsaAuxStatusFlags = _refptr_g_asaStatic->dwAsaAuxStatusFlags;
      if ((pStylusFrame->dwAuxStatusFlags & 4) == 0) {
        ASAStaticPreProcess();
        ASAOutClean();
        _refptr_g_asaStatic->dwAsaStatusFlags = s_hostReport.dwStatusFlags;
        if ((s_hostReport.dwStatusFlags & 7) != 0) {
          _refptr_g_curASOut->pReportX[4] = s_hostReport.wXPos;
          _refptr_g_curASOut->pReportY[4] = s_hostReport.wYPos;
          _refptr_g_curASOut->nRptXTilt = s_hostReport.nXTilt;
          _refptr_g_curASOut->nRptYTilt = s_hostReport.nYTilt;
          _refptr_g_curASOut->wPressure = s_hostReport.wPressure;
        }
        _refptr_g_curASOut->dwRptStatusFlags = _refptr_g_asaStatic->dwAsaStatusFlags;
        ASAStaticPostProcess();
        memcpy(&g_prevASOut,_refptr_g_curASOut,0xec);
        g_isLastFrameBypass = 0;
        iVar3 = 0;
      }
      else {
        iVar3 = 3;
      }
    }
    else {
      if (ASAHost_ReleaseReportExitStylus()) {
        _refptr_g_asaStatic->dwPrevAsaAuxStatusFlags = _refptr_g_asaStatic->dwAsaAuxStatusFlags;
      }
      g_isLastFrameBypass = 1;
      if (*_refptr_g_tsaLogLoglevel != '\0') {
        StrPrintf_Process(0,"\n ASA RELEASE! \n");
      }
      iVar3 = 3;
    }
  }
  else {
    ASA_Reset();
    iVar3 = 1;
  }
  return iVar3;
}


// ============================================================================
// Function: TSA_ASAProcess @ 6ba99e8d
// ============================================================================

// 与原函数的差别：
//   - ASA_SetSSDiffdata / ASA_MultiPanleSwitchProcess / TSALog_OutputStylus 是 ASA 解算本体的
//     输入与日志，不保留；ASA_MainProcess 换成 ASAHost_MainProcess。
//   - 原函数在 ASA_MainProcess 之后有 StylusRecheck 的门（dwFeatureFlags2 & 0x100，本机型开启）：
//     复核不过时 ASA_Reset() 清掉本帧报告。复核是笔解算一侧的判定，宿主送来的报告已经是
//     判定后的结果，这里只留下复核通过的那一支。
void TSA_ASAProcess(uint param_1)

{
  stylusFrame *psVar3;
  char cVar4;
  AFE_Frame *lVar6;
  byte local_19;

  g_isAsaCurFrameExitAbnormal = 0;
  (*_refptr_g_tsaStaticPtr)->dwProcessingState = (*_refptr_g_tsaStaticPtr)->dwProcessingState | 0x40
  ;
  lVar6 = (AFE_Frame *)AFE_GetFrame();
  psVar3 = (stylusFrame *)lVar6->pStylusData;
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"\n*Stylus*...");
  }
  if (psVar3 == (stylusFrame *)0x0) {
    ASA_Reset();
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"STYLUS FRAME NULL!!");
    }
  }
  else if (psVar3->dwAuxStatusFlags == 0) {
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"DISABLE STYLUS!");
    }
    ASA_Reset();
  }
  else {
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"  status:%4d",psVar3->dwAuxStatusFlags);
    }
    if ((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x1000000) == 0) &&
       (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_STYLUS_AFT) != 0)) {
      g_preInRange = TSA_RptASAInRange();
    }
    g_isAsaCurFrameExitAbnormal = ASAHost_MainProcess(lVar6);
    cVar4 = ASA_IsTouchNull((longlong)psVar3);
    if (cVar4 != '\0') {
      g_asaDisableTouch = '\x03';
    }
    if (g_asaDisableTouch != '\0') {
      g_asaDisableTouch = g_asaDisableTouch + -1;
      if ((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x1000000) == 0) &&
         (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_STYLUS_AFT) != 0)) {
        cVar4 = TSA_RptASAInRange();
        if ((cVar4 == '\x01') && (g_preInRange == '\0')) {
          if (*_refptr_g_tsaLogLoglevel != '\0') {
            StrPrintf_Process(0,"\nStylusAFT: Pen Enter Withdrawn");
          }
          for (local_19 = 0; local_19 < **(byte **)_refptr_g_touchesPtr; local_19 = local_19 + 1) {
            *(uint *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x208) =
                 *(uint *)((longlong)(int)(uint)local_19 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x208) | 0x80;
          }
        }
        cVar4 = TSA_RptASAInRange();
        if ((cVar4 == '\x01') && (g_preInRange == '\x01')) {
          PrevTouch_Init();
          Touch_Init();
          PrevPeak_Init();
        }
      }
      else {
        cVar4 = ASA_IsHpp3TouchEnableFeatureEnabled();
        if (cVar4 == '\0') {
          PrevTouch_Init();
          Touch_Init();
          PrevPeak_Init();
        }
      }
      SideTouch_Clean();
    }
    cVar4 = ASA_NeedRecordTrigger();
    if (cVar4 != '\0') {
      (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
           (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 0x1000;
    }
    HardwareAnalyzer_ASA_Process();
  }
  if ((param_1 & 0xf) != 0) {
    ASA_Reset();
  }
  (*_refptr_g_tsaStaticPtr)->dwProcessingState =
       (*_refptr_g_tsaStaticPtr)->dwProcessingState & 0xffffffbf;
  return;
}
