// ============================================================================
// Function: BigData_StatProcess @ 6bad7f51
// ============================================================================

void BigData_StatProcess(void)

{
  char cVar1;
  int iVar2;
  byte local_9;
  
                    /* 0x97f51  442  BigData_StatProcess */
  for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
    cVar1 = Touch_IsOff(local_9);
    if (cVar1 != '\0') {
      cVar1 = GripFilter_IsGripTouch(local_9);
      if (cVar1 != '\0') {
        g_bigDataStat = g_bigDataStat + 1;
      }
      if ((*(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x214) & 0x200000) != 0) {
        DAT_6bc76de4 = DAT_6bc76de4 + 1;
      }
    }
  }
  iVar2 = TSA_GetThenarSide();
  if (iVar2 == 4) {
    DAT_6bc76de8 = DAT_6bc76de8 + (*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
  }
  iVar2 = TSA_GetThenarSide();
  if (iVar2 == 8) {
    DAT_6bc76dec = DAT_6bc76dec + (*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
  }
  cVar1 = TSAStatic_IsHWResetPrev();
  if (cVar1 != '\0') {
    cVar1 = TSAStatic_IsHWReset();
    if (cVar1 == '\0') {
      cVar1 = TSAStatic_IsSafeBaselineApplied();
      if (cVar1 == '\0') {
        DAT_6bc76df0 = DAT_6bc76df0 + 1;
      }
      else {
        DAT_6bc76df4 = DAT_6bc76df4 + 1;
      }
    }
  }
  cVar1 = AFE_IsInIDLEMode();
  if (cVar1 == '\0') {
    DAT_6bc76dfc = DAT_6bc76dfc + (*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
  }
  else {
    DAT_6bc76df8 = DAT_6bc76df8 + (*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
    cVar1 = TSA_IsGripOKToIDLE();
    if (cVar1 != '\0') {
      DAT_6bc76e04 = DAT_6bc76e04 + (*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
    }
  }
  cVar1 = Posture_IsHangingPosture();
  if (cVar1 != '\0') {
    DAT_6bc76e00 = DAT_6bc76e00 + (*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
  }
  return;
}


// ============================================================================
// Function: BigData_RecordProcess @ 6bad90ae
// ============================================================================

void BigData_RecordProcess(void)

{
  char cVar1;
  byte local_a;
  byte local_9;
  
                    /* 0x990ae  440  BigData_RecordProcess */
  for (local_9 = 0; local_9 < 5; local_9 = local_9 + 1) {
    cVar1 = BigData_IsTagRecordReady(local_9);
    if (cVar1 != '\0') {
      BigData_TagRecord(local_9);
    }
  }
  for (local_a = 0; local_a < **(byte **)_refptr_g_touchesPtr; local_a = local_a + 1) {
    cVar1 = BigData_IsOKToRecordFilm(local_a);
    if (cVar1 != '\0') {
      *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
           *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) | 0x800000;
    }
  }
  return;
}


// ============================================================================
// Function: BigData_IsReportReady @ 6bad9209
// ============================================================================

undefined1 BigData_IsReportReady(void)

{
                    /* 0x99209  430  BigData_IsReportReady */
  return _refptr_g_bigData[0x3948];
}


// ============================================================================
// Function: BigData_ReportProcess @ 6bad995f
// ============================================================================

void BigData_ReportProcess(void)

{
  char cVar1;
  byte local_9;
  
                    /* 0x9995f  441  BigData_ReportProcess */
  _refptr_g_bigData[0x3948] = 0;
  local_9 = 0;
  while( true ) {
    if (4 < local_9) {
      return;
    }
    cVar1 = BigData_IsTagReportReady(local_9);
    if (cVar1 != '\0') break;
    local_9 = local_9 + 1;
  }
  _refptr_g_bigData[0x3948] = 1;
  *(void **)(_refptr_g_bigData + (longlong)(int)(uint)local_9 * 0x840 + 0x1838) =
       (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  *(short *)(_refptr_g_bigData + 0x394a) = *(short *)(_refptr_g_bigData + 0x394a) + 1;
  *(short *)(_refptr_g_bigData + (longlong)(int)(uint)local_9 * 0x840 + 0x1832) =
       *(short *)(_refptr_g_bigData + (longlong)(int)(uint)local_9 * 0x840 + 0x1832) + 1;
  *(short *)(_refptr_g_bigData + (longlong)(int)(uint)local_9 * 0x840 + 0x1834) =
       *(short *)(_refptr_g_bigData + (longlong)(int)(uint)local_9 * 0x840 + 0x1834) + 1;
  if (*(short *)(_refptr_g_bigData + (longlong)(int)(uint)local_9 * 0x840 + 0x1834) == 1) {
    *(void **)(_refptr_g_bigData + (longlong)(int)(uint)local_9 * 0x840 + 0x1840) =
         (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  }
  BigData_CreateReport(local_9);
  BigData_ClearTagData(local_9);
  return;
}


// ============================================================================
// Function: BigData_IsTagRecordReady @ 6bad81c3
// ============================================================================

undefined8 BigData_IsTagRecordReady(byte param_1)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0x981c3  431  BigData_IsTagRecordReady */
  cVar1 = BigData_IsRecordReady();
  if (cVar1 == '\0') {
    uVar2 = 0;
  }
  else {
    if (86400000 <
        (ulonglong)
        ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
        *(longlong *)(_refptr_g_bigData + (longlong)(int)(uint)param_1 * 0x840 + 0x1840))) {
      *(undefined2 *)(_refptr_g_bigData + (longlong)(int)(uint)param_1 * 0x840 + 0x1834) = 0;
    }
    if (((((ulonglong)*(uint *)(&g_reportLimitedInterval + (longlong)(int)(uint)param_1 * 4) <
           (ulonglong)
           ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
           *(longlong *)(_refptr_g_bigData + (longlong)(int)(uint)param_1 * 0x840 + 0x1838))) &&
         (*(ushort *)(_refptr_g_bigData + (longlong)(int)(uint)param_1 * 0x840 + 0x1832) <
          *(ushort *)(&g_reportLimitedTotal + (longlong)(int)(uint)param_1 * 2))) &&
        ((ushort)*(wchar_t *)(_refptr_g_bigData + (longlong)(int)(uint)param_1 * 0x840 + 0x1834) <
         (ushort)L"\n\n\n\n"[(int)(uint)param_1])) &&
       (_refptr_g_bigData[(longlong)(int)(uint)param_1 * 0x840 + 0x1830] == '\0')) {
      uVar2 = 1;
    }
    else {
      uVar2 = 0;
    }
  }
  return uVar2;
}


// ============================================================================
// Function: BigData_IsOKToRecordFilm @ 6bad86a5
// ============================================================================

undefined8 BigData_IsOKToRecordFilm(byte param_1)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0x986a5  423  BigData_IsOKToRecordFilm */
  cVar1 = BigData_IsOKToRecordMaxSig(param_1);
  if (cVar1 == '\0') {
    uVar2 = 0;
  }
  else if (((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold * 2) / 3 <
           (int)*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x1be)) {
    uVar2 = 0;
  }
  else {
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: BigData_TagRecord @ 6bad9011
// ============================================================================

void BigData_TagRecord(byte param_1)

{
  bool bVar1;
  char cVar2;
  byte local_a;
  
                    /* 0x99011  443  BigData_TagRecord */
  bVar1 = false;
  for (local_a = 0; local_a < **(byte **)_refptr_g_touchesPtr; local_a = local_a + 1) {
    cVar2 = BigData_IsOKToRecord(param_1,local_a);
    if (cVar2 != '\0') {
      BigData_RecordFrame(param_1,local_a);
      bVar1 = true;
    }
  }
  if ((_refptr_g_bigData[(longlong)(int)(uint)param_1 * 0x840 + 0x1008] != '\0') && (!bVar1)) {
    BigData_ClearTagData(param_1);
  }
  return;
}


// ============================================================================
// Function: BigData_ClearTagData @ 6bad7d00
// ============================================================================

void BigData_ClearTagData(byte param_1)

{
  int iVar1;
  
                    /* 0x97d00  408  BigData_ClearTagData */
  iVar1 = memset_s(&DAT_6bc7448c + (longlong)(int)(uint)param_1 * 0x840,0x820,0,0x820);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/BigData/BigData.c",0x1f,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  (&DAT_6bc74488)[(longlong)(int)(uint)param_1 * 0x840] = 0;
  (&DAT_6bc74cb0)[(longlong)(int)(uint)param_1 * 0x840] = 0;
  return;
}


// ============================================================================
// Function: BigData_CreateReport @ 6bad98ee
// ============================================================================

void BigData_CreateReport(undefined1 param_1)

{
                    /* 0x998ee  409  BigData_CreateReport */
  BigData_PrePrint();
  BigData_CreateReportHeader();
  BigData_CreateTagReport(param_1);
  BigData_PostPrint();
  return;
}


// ============================================================================
// Function: BigData_IsTagReportReady @ 6bad991c
// ============================================================================

bool BigData_IsTagReportReady(byte param_1)

{
                    /* 0x9991c  432  BigData_IsTagReportReady */
  return _refptr_g_bigData[(longlong)(int)(uint)param_1 * 0x840 + 0x1830] != '\0';
}


// ============================================================================
// Function: BigData_Init @ 6bad7e5c
// ============================================================================

void BigData_Init(void)

{
  int iVar1;
  byte local_9;
  
                    /* 0x97e5c  421  BigData_Init */
  if (flagOnceOnly_10335 != '\0') {
    iVar1 = memset_s(&g_bigData,0x3958,0,0x3958);
    if (iVar1 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/BigData/BigData.c",0x3b,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
    iVar1 = memset_s(&g_bigDataStat,0x28,0,0x28);
    if (iVar1 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/BigData/BigData.c",0x3d,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
  }
  flagOnceOnly_10335 = 0;
  for (local_9 = 0; local_9 < 5; local_9 = local_9 + 1) {
    BigData_ClearTagData(local_9);
  }
  return;
}


// ============================================================================
// Function: BigData_IsRecordReady @ 6bad8170
// ============================================================================

undefined8 BigData_IsRecordReady(void)

{
  undefined8 uVar1;
  
                    /* 0x98170  429  BigData_IsRecordReady */
  if (((ulonglong)
       ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
       *(longlong *)(_refptr_g_bigData + 0x3950)) < 0x927c1) ||
     (9999 < *(ushort *)(_refptr_g_bigData + 0x394a))) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: BigData_IsOKToRecordMaxSig @ 6bad8378
// ============================================================================

undefined8 BigData_IsOKToRecordMaxSig(byte param_1)

{
  byte bVar1;
  int iVar2;
  undefined8 uVar3;
  
                    /* 0x98378  426  BigData_IsOKToRecordMaxSig */
  bVar1 = *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x287
                   );
  iVar2 = TSA_GetThenarSide();
  if (iVar2 == 0) {
    uVar3 = 0;
  }
  else if ((*(uchar *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x285) == _refptr_g_tsaPrmtFlash->bCols) ||
          (*(uchar *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x286) == _refptr_g_tsaPrmtFlash->bRows)) {
    uVar3 = 0;
  }
  else if (*(ushort *)
            ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x18a) < 10)
  {
    uVar3 = 0;
  }
  else if (*(ushort *)
            ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x156) < 5)
  {
    uVar3 = 0;
  }
  else if ((((*(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x1dd) < 0x1c) ||
            (*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)bVar1 * 0x478 + 0x1dd) < 0x1c)) ||
           (*(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1de) < 0x1c)) ||
          (*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)bVar1 * 0x478 + 0x1de) < 0x1c)) {
    uVar3 = 0;
  }
  else if (*(int *)(*(longlong *)_refptr_g_touchesPtr + 0x20) == 0) {
    if (((((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1fc) & 0x40000) == 0) &&
         ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1fc) & 0x20000) == 0)) &&
        (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 500)
          & 0x1000) != 0 &&
         (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1f8) & 8) != 0 &&
          (2 < *(ushort *)
                ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x23a)))
         )))) && (((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold * 2) / 5 <=
                  (int)*(short *)((longlong)(int)(uint)param_1 * 0x168 +
                                  *(longlong *)_refptr_g_touchesPtr + 0x1be))) {
      uVar3 = 1;
    }
    else {
      uVar3 = 0;
    }
  }
  else {
    uVar3 = 0;
  }
  return uVar3;
}


// ============================================================================
// Function: BigData_IsOKToRecord @ 6bad8983
// ============================================================================

undefined8 BigData_IsOKToRecord(byte param_1,byte param_2)

{
  undefined8 uVar1;
  
                    /* 0x98983  422  BigData_IsOKToRecord */
  if (_refptr_g_bigData[(longlong)(int)(uint)param_1 * 0x840 + 0x1008] == '\0') {
    uVar1 = BigData_IsOKToRecordSub(param_1,param_2);
  }
  else if (_refptr_g_bigData
           [(longlong)
            (int)((byte)_refptr_g_bigData[(longlong)(int)(uint)param_1 * 0x840 + 0x1008] - 1) * 0xd0
            + (longlong)(int)(uint)param_1 * 0x840 + 0x100c] ==
           *(char *)((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x154)) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: BigData_RecordFrame @ 6bad8fa1
// ============================================================================

void BigData_RecordFrame(byte param_1,undefined1 param_2)

{
                    /* 0x98fa1  437  BigData_RecordFrame */
  if (*(int *)(&g_reportFrameFormat + (longlong)(int)(uint)param_1 * 4) == 0) {
    BigData_RecordFrame7x7(param_1,param_2);
  }
  else if (*(int *)(&g_reportFrameFormat + (longlong)(int)(uint)param_1 * 4) == 1) {
    BigData_RecordFrameEdge(param_1,param_2);
  }
  return;
}


// ============================================================================
// Function: BigData_PrePrint @ 6bad922a
// ============================================================================

void BigData_PrePrint(void)

{
  int iVar1;
  
                    /* 0x9922a  434  BigData_PrePrint */
  iVar1 = memset_s(_refptr_g_bigData,0x1000,0,0x1000);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/BigData/BigData_Report.c",0x37,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  *(undefined2 *)(_refptr_g_bigData + 0x1000) = 0;
  StrPrintf_Backup();
  StrPrintf_Init(BigData_PutStr);
  return;
}


// ============================================================================
// Function: BigData_PostPrint @ 6bad92af
// ============================================================================

void BigData_PostPrint(void)

{
                    /* 0x992af  433  BigData_PostPrint */
  StrPrintf_Retore();
  _refptr_g_bigData[0xfff] = 0;
  return;
}


// ============================================================================
// Function: BigData_CreateTagReport @ 6bad9741
// ============================================================================

void BigData_CreateTagReport(byte param_1)

{
                    /* 0x99741  417  BigData_CreateTagReport */
  StrPrintf_Process(0,"tagStart[%s]\n",
                    *(undefined8 *)(_refptr_g_tagName + (longlong)(int)(uint)param_1 * 8));
  StrPrintf_Process(0,"tagCntReportedTotal[%d]\n",
                    *(undefined2 *)
                     (_refptr_g_bigData + (longlong)(int)(uint)param_1 * 0x840 + 0x1832));
  StrPrintf_Process(0,"tagCntReportedDaily[%d]\n",
                    *(undefined2 *)
                     (_refptr_g_bigData + (longlong)(int)(uint)param_1 * 0x840 + 0x1834));
  BigData_CreateTagData(param_1);
  StrPrintf_Process(0,"tagEnd[%s]\n",
                    *(undefined8 *)(_refptr_g_tagName + (longlong)(int)(uint)param_1 * 8));
  return;
}


// ============================================================================
// Function: BigData_CreateReportHeader @ 6bad982a
// ============================================================================

void BigData_CreateReportHeader(void)

{
  undefined1 uVar1;
  undefined2 uVar2;
  undefined8 uVar3;
  
                    /* 0x9982a  410  BigData_CreateReportHeader */
  *(void **)(_refptr_g_bigData + 0x3950) = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  uVar3 = TSA_GetBranchName();
  StrPrintf_Process(0,"TSABranch[%s]\n",uVar3);
  uVar2 = TSA_GetMinorVersion();
  uVar1 = TSA_GetMajorVersion();
  StrPrintf_Process(0,"TSAVersion[%d.%d]\n",uVar1,uVar2);
  StrPrintf_Process(0,"TimeStamp[%lld]\n",(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr);
  StrPrintf_Process(0,"cntReportedTotal[%d]\n",*(undefined2 *)(_refptr_g_bigData + 0x394a));
  BigData_CreateStatusReport();
  return;
}


// ============================================================================
// Function: BigData_IsOKToRecordSub @ 6bad88fa
// ============================================================================

undefined8 BigData_IsOKToRecordSub(undefined1 param_1,undefined1 param_2)

{
  undefined8 uVar1;
  
                    /* 0x988fa  427  BigData_IsOKToRecordSub */
  switch(param_1) {
  case 0:
    uVar1 = BigData_IsOKToRecordMaxSig(param_2);
    break;
  case 1:
    uVar1 = BigData_IsOKToRecordGhostTouch(param_2);
    break;
  case 2:
    uVar1 = BigData_IsOKToRecordGrip(param_2);
    break;
  case 3:
    uVar1 = BigData_IsOKToRecordThenar(param_2);
    break;
  case 4:
    uVar1 = BigData_IsOKToRecordFilm(param_2);
    break;
  default:
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: BigData_RecordFrameEdge @ 6bad8a8a
// ============================================================================

void BigData_RecordFrameEdge(byte param_1,byte param_2)

{
  byte bVar1;
  undefined *puVar2;
  char cVar3;
  longlong lVar4;
  undefined1 *puVar5;
  char local_a;
  byte local_9;
  
                    /* 0x98a8a  439  BigData_RecordFrameEdge */
  puVar2 = _refptr_g_bigData;
  lVar4 = (longlong)
          (int)(uint)(byte)_refptr_g_bigData[(longlong)(int)(uint)param_1 * 0x840 + 0x1008] * 0xd0 +
          (longlong)(int)(uint)param_1 * 0x840;
  puVar5 = _refptr_g_bigData + lVar4 + 0x100c;
  local_9 = 0;
  for (local_a = '\0'; (int)local_a < (int)(uint)_refptr_g_tsaPrmtFlash->bRows;
      local_a = local_a + '\x01') {
    bVar1 = local_9 + 1;
    *(undefined2 *)(puVar2 + (longlong)(int)(uint)local_9 * 2 + lVar4 + 0x1014) =
         *(undefined2 *)
          ((ulonglong)(ushort)((ushort)_refptr_g_tsaPrmtFlash->bCols * (short)local_a) * 2 +
          *(longlong *)_refptr_g_tsaBufDif);
    local_9 = local_9 + 2;
    *(undefined2 *)(puVar2 + (longlong)(int)(uint)bVar1 * 2 + lVar4 + 0x1014) =
         *(undefined2 *)
          ((ulonglong)
           ((uint)_refptr_g_tsaPrmtFlash->bCols * ((ushort)(short)local_a + 1) - 1 & 0xffff) * 2 +
          *(longlong *)_refptr_g_tsaBufDif);
  }
  puVar2[lVar4 + 0x100d] = 0;
  puVar2[lVar4 + 0x100e] = 0;
  *puVar5 = *(undefined1 *)
             ((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x154);
  *(undefined4 *)(puVar2 + lVar4 + 0x1010) = 1;
  _refptr_g_bigData[(longlong)(int)(uint)param_1 * 0x840 + 0x1008] =
       _refptr_g_bigData[(longlong)(int)(uint)param_1 * 0x840 + 0x1008] + '\x01';
  if ((_refptr_g_bigData[(longlong)(int)(uint)param_1 * 0x840 + 0x1008] != '\n') &&
     (cVar3 = Touch_IsOff(param_2), cVar3 == '\0')) {
    return;
  }
  _refptr_g_bigData[(longlong)(int)(uint)param_1 * 0x840 + 0x1830] = 1;
  return;
}


// ============================================================================
// Function: BigData_RecordFrame7x7 @ 6bad8cd0
// ============================================================================

void BigData_RecordFrame7x7(byte param_1,byte param_2)

{
  undefined *puVar1;
  char cVar2;
  byte bVar3;
  byte bVar4;
  char cVar5;
  longlong lVar6;
  undefined1 *puVar7;
  byte local_b;
  byte local_a;
  byte local_9;
  
                    /* 0x98cd0  438  BigData_RecordFrame7x7 */
  puVar1 = _refptr_g_bigData;
  cVar5 = (char)((ushort)*(undefined2 *)
                          ((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x162) >> 8);
  cVar2 = (char)((ushort)*(undefined2 *)
                          ((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x164) >> 8);
  bVar3 = cVar5 - 3;
  lVar6 = (longlong)
          (int)(uint)(byte)_refptr_g_bigData[(longlong)(int)(uint)param_1 * 0x840 + 0x1008] * 0xd0 +
          (longlong)(int)(uint)param_1 * 0x840;
  puVar7 = _refptr_g_bigData + lVar6 + 0x100c;
  local_9 = 0;
  bVar4 = cVar2 - 3;
  for (local_a = bVar4; (int)(char)local_a < (char)bVar4 + 7; local_a = local_a + 1) {
    for (local_b = bVar3; (int)(char)local_b < (char)bVar3 + 7; local_b = local_b + 1) {
      if ((local_b < _refptr_g_tsaPrmtFlash->bCols) && (local_a < _refptr_g_tsaPrmtFlash->bRows)) {
        *(undefined2 *)(puVar1 + (longlong)(int)(uint)local_9 * 2 + lVar6 + 0x1014) =
             *(undefined2 *)
              ((ulonglong)
               (ushort)((short)(char)local_b +
                       (short)(char)local_a * (ushort)_refptr_g_tsaPrmtFlash->bCols) * 2 +
              *(longlong *)_refptr_g_tsaBufDif);
      }
      else {
        *(undefined2 *)(puVar1 + (longlong)(int)(uint)local_9 * 2 + lVar6 + 0x1014) = 0;
      }
      local_9 = local_9 + 1;
    }
  }
  puVar1[lVar6 + 0x100d] = cVar5;
  puVar1[lVar6 + 0x100e] = cVar2;
  *puVar7 = *(undefined1 *)
             ((longlong)(int)(uint)param_2 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x154);
  *(undefined4 *)(puVar1 + lVar6 + 0x1010) = 0;
  _refptr_g_bigData[(longlong)(int)(uint)param_1 * 0x840 + 0x1008] =
       _refptr_g_bigData[(longlong)(int)(uint)param_1 * 0x840 + 0x1008] + '\x01';
  if ((_refptr_g_bigData[(longlong)(int)(uint)param_1 * 0x840 + 0x1008] != '\n') &&
     (cVar5 = Touch_IsOff(param_2), cVar5 == '\0')) {
    return;
  }
  _refptr_g_bigData[(longlong)(int)(uint)param_1 * 0x840 + 0x1830] = 1;
  return;
}


// ============================================================================
// Function: BigData_PutStr @ 6bad9140
// ============================================================================

void BigData_PutStr(longlong param_1)

{
  ushort uVar1;
  ushort local_a;
  
                    /* 0x99140  436  BigData_PutStr */
  if (param_1 != 0) {
    local_a = 0;
    while ((*(char *)(param_1 + (ulonglong)local_a) != '\0' &&
           (*(ushort *)(_refptr_g_bigData + 0x1000) < 0xfff))) {
      uVar1 = *(ushort *)(_refptr_g_bigData + 0x1000);
      *(ushort *)(_refptr_g_bigData + 0x1000) = uVar1 + 1;
      _refptr_g_bigData[(int)(uint)uVar1] = *(undefined1 *)(param_1 + (ulonglong)local_a);
      local_a = local_a + 1;
    }
    if (*(ushort *)(_refptr_g_bigData + 0x1000) < 0x1000) {
      _refptr_g_bigData[(int)(uint)*(ushort *)(_refptr_g_bigData + 0x1000)] = 0;
    }
  }
  return;
}


// ============================================================================
// Function: BigData_CreateTagData @ 6bad96e2
// ============================================================================

void BigData_CreateTagData(byte param_1)

{
  byte local_9;
  
                    /* 0x996e2  412  BigData_CreateTagData */
  for (local_9 = 0; local_9 < (byte)_refptr_g_bigData[(longlong)(int)(uint)param_1 * 0x840 + 0x1008]
      ; local_9 = local_9 + 1) {
    BigData_CreateTagFrame(param_1,local_9);
  }
  return;
}


// ============================================================================
// Function: BigData_CreateStatusReport @ 6bad92d0
// ============================================================================

void BigData_CreateStatusReport(void)

{
  undefined1 uVar1;
  undefined4 uVar2;
  undefined8 uVar3;
  
                    /* 0x992d0  411  BigData_CreateStatusReport */
  uVar3 = TSAPrmt_GetProjectName();
  StrPrintf_Process(0,"projID[%s]\n",uVar3);
  StrPrintf_Process(0,"panel[%d][%d]\n",_refptr_g_tsaPrmtFlash->bRows,_refptr_g_tsaPrmtFlash->bCols)
  ;
  uVar1 = TSAStatic_IsChargerPlugged();
  StrPrintf_Process(0,"charger[%d]\n",uVar1);
  uVar2 = TSA_GetThenarSide();
  StrPrintf_Process(0,"thenar[%d]\n",uVar2);
  uVar1 = Posture_IsHangingPosture();
  StrPrintf_Process(0,"posture[%d]\n",uVar1);
  StrPrintf_Process(0,"exception[0x%x]\n",*(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x20))
  ;
  return;
}


// ============================================================================
// Function: BigData_GetReport @ 6bad921d
// ============================================================================

undefined * BigData_GetReport(void)

{
                    /* 0x9921d  418  BigData_GetReport */
  return _refptr_g_bigData;
}


// ============================================================================
// Function: BigData_IsOKToRecordGhostTouch @ 6bad872b
// ============================================================================

undefined8 BigData_IsOKToRecordGhostTouch(byte param_1)

{
  char cVar1;
  
                    /* 0x9872b  424  BigData_IsOKToRecordGhostTouch */
  cVar1 = SafeBaseline_IsSafeDiffChecked();
  if (((((cVar1 != '\0') && (cVar1 = SafeBaseline_IsSafeRawChecked(), cVar1 != '\0')) &&
       (1000 < *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x158))) &&
      ((*(int *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1ec)
        == 0 && ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                           + 500) & 0x1000) != 0)))) &&
     (cVar1 = TSA_IsGameScenario(), cVar1 == '\0')) {
    cVar1 = Touch_IsGhostByMinSigOff(param_1);
    if (cVar1 != '\0') {
      return 1;
    }
    if ((10 < *(ushort *)
               ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x188))
       && (0xe < *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                           + 0x268))) {
      return 1;
    }
  }
  return 0;
}


// ============================================================================
// Function: BigData_IsOKToRecordGrip @ 6bad885c
// ============================================================================

undefined8 BigData_IsOKToRecordGrip(void)

{
                    /* 0x9885c  425  BigData_IsOKToRecordGrip */
  return 0;
}


// ============================================================================
// Function: BigData_IsOKToRecordThenar @ 6bad889a
// ============================================================================

undefined8 BigData_IsOKToRecordThenar(byte param_1)

{
  undefined8 uVar1;
  
                    /* 0x9889a  428  BigData_IsOKToRecordThenar */
  if (((*_refptr_g_prevTouchesPtr)[1].field_0x1c == '\0') ||
     (*(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1be)
      != *(short *)(*_refptr_g_peaksPtr + 2))) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: BigData_CreateTagFrame @ 6bad95e2
// ============================================================================

void BigData_CreateTagFrame(byte param_1,byte param_2)

{
                    /* 0x995e2  413  BigData_CreateTagFrame */
  if (*(int *)(_refptr_g_bigData +
              (longlong)(int)(uint)param_2 * 0xd0 + (longlong)(int)(uint)param_1 * 0x840 + 0x1010)
      == 0) {
    StrPrintf_Process(0,"frameVersion[%d]\n",0);
    BigData_CreateTagFrame7x7(param_1,param_2);
  }
  else if (*(int *)(_refptr_g_bigData +
                   (longlong)(int)(uint)param_2 * 0xd0 + (longlong)(int)(uint)param_1 * 0x840 +
                   0x1010) == 1) {
    StrPrintf_Process(0,"frameVersion[%d]\n",1);
    BigData_CreateTagFrameEdge(param_1,param_2);
  }
  return;
}


// ============================================================================
// Function: BigData_CreateTagFrameEdge @ 6bad9573
// ============================================================================

void BigData_CreateTagFrameEdge(undefined1 param_1,undefined1 param_2)

{
                    /* 0x99573  416  BigData_CreateTagFrameEdge */
  BigData_CreateTagFrameCommon(param_1,param_2,2,_refptr_g_tsaPrmtFlash->bRows);
  return;
}


// ============================================================================
// Function: BigData_CreateTagFrame7x7 @ 6bad95b0
// ============================================================================

void BigData_CreateTagFrame7x7(undefined1 param_1,undefined1 param_2)

{
                    /* 0x995b0  414  BigData_CreateTagFrame7x7 */
  BigData_CreateTagFrameCommon(param_1,param_2,7,7);
  return;
}


// ============================================================================
// Function: BigData_CreateTagFrameCommon @ 6bad939d
// ============================================================================

void BigData_CreateTagFrameCommon(byte param_1,byte param_2,byte param_3,byte param_4)

{
  byte local_b;
  byte local_a;
  byte local_9;
  
                    /* 0x9939d  415  BigData_CreateTagFrameCommon */
  StrPrintf_Process(0,"frameFormat[%d][%d]\n",param_4,param_3);
  StrPrintf_Process(0,"frameStart[%d][%d]\n",
                    _refptr_g_bigData
                    [(longlong)(int)(uint)param_2 * 0xd0 + (longlong)(int)(uint)param_1 * 0x840 +
                     0x100e],_refptr_g_bigData
                             [(longlong)(int)(uint)param_2 * 0xd0 +
                              (longlong)(int)(uint)param_1 * 0x840 + 0x100d]);
  local_9 = 0;
  for (local_a = 0; local_a < param_4; local_a = local_a + 1) {
    for (local_b = 0; local_b < param_3; local_b = local_b + 1) {
      StrPrintf_Process(0,&DAT_6bb56a6a,
                        (int)*(short *)(_refptr_g_bigData +
                                       ((longlong)(int)(uint)param_2 * 0x68 +
                                        (longlong)(int)(uint)param_1 * 0x420 +
                                        (longlong)(int)(uint)local_9 + 0x800) * 2 + 0x14));
      local_9 = local_9 + 1;
    }
    StrPrintf_Process(0,&DAT_6bb56a6e);
  }
  StrPrintf_Process(0,"frameEnd\n",param_2);
  return;
}


