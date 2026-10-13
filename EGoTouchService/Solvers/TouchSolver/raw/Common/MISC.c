// ============================================================================
// Function: memcpy @ 6bb4e548
// ============================================================================

void * __cdecl memcpy(void *_Dst,void *_Src,size_t _Size)

{
  void *pvVar1;
  
                    /* WARNING: Could not recover jumptable at 0x6bb4e548. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  pvVar1 = memcpy(_Dst,_Src,_Size);
  return pvVar1;
}


// ============================================================================
// Function: CaculateInterval @ 6bade69c
// ============================================================================

void CaculateInterval(void)

{
  g_uSTimeInterval = *(int *)(*(longlong *)(_refptr_g_thpAFEStruct + 0x10) + 4) - g_prevUsTimeStamp;
  g_prevUsTimeStamp = (longlong)*(int *)(*(longlong *)(_refptr_g_thpAFEStruct + 0x10) + 4);
  return;
}


// ============================================================================
// Function: SensorPitchSizeMapDim1 @ 6bb060b4
// ============================================================================

int SensorPitchSizeMapDim1(int sensorCoord,ushort scale)

{
  int iVar1;
  
                    /* 0xc60b4  1286  SensorPitchSizeMapDim1 */
  iVar1 = SensorPitchSizeMap(sensorCoord,_refptr_g_tsaPrmtFlash->pDim1PitchMap,scale);
  return iVar1;
}


// ============================================================================
// Function: SensorPitchSizeMapDim2 @ 6bb060e8
// ============================================================================

int SensorPitchSizeMapDim2(int sensorCoord,ushort scale)

{
  int iVar1;
  
                    /* 0xc60e8  1287  SensorPitchSizeMapDim2 */
  iVar1 = SensorPitchSizeMap(sensorCoord,_refptr_g_tsaPrmtFlash->pDim2PitchMap,scale);
  return iVar1;
}


// ============================================================================
// Function: .text @ 6ba41430
// ============================================================================

undefined8 _text(void *param_1,ulonglong param_2,void *param_3,ulonglong param_4)

{
  undefined8 uVar1;
  
  if ((param_2 == 0) || (0x7fffffff < param_2)) {
    uVar1 = 0x22;
  }
  else if ((param_1 == (void *)0x0) || (param_3 == (void *)0x0)) {
    if (param_1 == (void *)0x0) {
      uVar1 = 0x16;
    }
    else {
      memset(param_1,0,param_2);
      uVar1 = 0x96;
    }
  }
  else if (param_2 < param_4) {
    memset(param_1,0,param_2);
    uVar1 = 0xa2;
  }
  else if (((param_3 < param_1) && (param_1 < (void *)(param_4 + (longlong)param_3))) ||
          ((param_1 < param_3 && (param_3 < (void *)(param_4 + (longlong)param_1))))) {
    memset(param_1,0,param_2);
    uVar1 = 0xb6;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: strcmp @ 6bb4e518
// ============================================================================

int __cdecl strcmp(char *_Str1,char *_Str2)

{
  int iVar1;
  
                    /* WARNING: Could not recover jumptable at 0x6bb4e518. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  iVar1 = strcmp(_Str1,_Str2);
  return iVar1;
}


// ============================================================================
// Function: strncmp @ 6bb4e508
// ============================================================================

int __cdecl strncmp(char *_Str1,char *_Str2,size_t _MaxCount)

{
  int iVar1;
  
                    /* WARNING: Could not recover jumptable at 0x6bb4e508. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  iVar1 = strncmp(_Str1,_Str2,_MaxCount);
  return iVar1;
}


// ============================================================================
// Function: ___chkstk_ms @ 6bb48db0
// ============================================================================

/* WARNING: This is an inlined function */

ulonglong ___chkstk_ms(void)

{
  ulonglong in_RAX;
  ulonglong uVar1;
  undefined8 *puVar2;
  undefined8 local_res8 [4];
  
  puVar2 = local_res8;
  uVar1 = in_RAX;
  if (0xfff < in_RAX) {
    do {
      puVar2 = puVar2 + -0x200;
      *puVar2 = *puVar2;
      uVar1 = uVar1 - 0x1000;
    } while (0x1000 < uVar1);
  }
  *(undefined8 *)((longlong)puVar2 - uVar1) = *(undefined8 *)((longlong)puVar2 - uVar1);
  return in_RAX;
}


// ============================================================================
// Function: memcpy @ EXTERNAL:00000051
// ============================================================================
// DECOMPILATION FAILED OR TIMED OUT
// ============================================================================
// Function: Bezier @ 6ba6595d
// ============================================================================

int Bezier(int param_1,int param_2,int param_3)

{
                    /* 0x2595d  406  Bezier */
  param_1 = (param_3 * 3 + param_2 * 6) - param_1;
  if (param_1 < 0) {
    param_1 = param_1 + 7;
  }
  return param_1 >> 3;
}


// ============================================================================
// Function: MAX @ 6bade410
// ============================================================================

int MAX(int param_1,int param_2)

{
  if (param_1 <= param_2) {
    param_1 = param_2;
  }
  return param_1;
}


// ============================================================================
// Function: MIN @ 6bade426
// ============================================================================

int MIN(int param_1,int param_2)

{
  if (param_2 <= param_1) {
    param_1 = param_2;
  }
  return param_1;
}


// ============================================================================
// Function: SecMemsetError @ 6ba41680
// ============================================================================

undefined8 SecMemsetError(void *param_1,ulonglong param_2,int param_3,ulonglong param_4)

{
  undefined8 uVar1;
  
  if ((param_2 == 0) || (0x7fffffff < param_2)) {
    uVar1 = 0x22;
  }
  else if (param_1 == (void *)0x0) {
    uVar1 = 0x16;
  }
  else if (param_2 < param_4) {
    memset(param_1,param_3,param_2);
    uVar1 = 0xa2;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: memset @ 6bb4e540
// ============================================================================

void * __cdecl memset(void *_Dst,int _Val,size_t _Size)

{
  void *pvVar1;
  
                    /* WARNING: Could not recover jumptable at 0x6bb4e540. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  pvVar1 = memset(_Dst,_Val,_Size);
  return pvVar1;
}


// ============================================================================
// Function: SensorPitchSizeMap @ 6bb05fc0
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int SensorPitchSizeMap(int param_1,double *param_2,ushort param_3)

{
  ushort uVar1;
  
  if (*param_2 != __rdata) {
    if (param_3 == 0) {
      param_1 = 0;
    }
    else {
      uVar1 = (ushort)(param_1 / (int)(uint)param_3);
      if (uVar1 < 0x4f) {
        param_1 = (int)(param_2[(ulonglong)uVar1 + 1] *
                        (double)(param_1 % (int)(uint)param_3 & 0xffff) +
                       param_2[uVar1] *
                       (double)(int)((uint)param_3 - (param_1 % (int)(uint)param_3 & 0xffffU)));
      }
      else {
        param_1 = 0;
      }
    }
  }
  return param_1;
}


// ============================================================================
// Function: RefreshClipPrmt @ 6bb26b52
// ============================================================================

undefined8 RefreshClipPrmt(longlong param_1)

{
  short sVar1;
  short sVar2;
  short sVar3;
  short sVar4;
  short sVar5;
  short sVar6;
  short sVar7;
  short sVar8;
  
  sVar1 = *(short *)(param_1 + 0x10);
  sVar5 = GetSensorStartDspl((int)(char)_refptr_g_tsaPrmtFlash->field_0x5ae,
                             (int)(char)_refptr_g_tsaPrmtFlash->field_0x5af,
                             *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5aa,
                             _refptr_g_tsaPrmtFlash->field_0x1f,*(undefined1 *)(param_1 + 10));
  sVar6 = GetSensorStartDspl((int)(char)_refptr_g_tsaPrmtFlash->field_0x5ae,
                             (int)(char)_refptr_g_tsaPrmtFlash->field_0x5af,
                             *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5aa,
                             _refptr_g_tsaPrmtFlash->field_0x1f,
                             *(char *)(param_1 + 8) + *(char *)(param_1 + 10));
  sVar2 = *(short *)(param_1 + 0x10);
  sVar3 = *(short *)(param_1 + 0xc);
  sVar4 = *(short *)(param_1 + 0x12);
  sVar7 = GetSensorStartDspl((int)(char)_refptr_g_tsaPrmtFlash->field_0x5b0,
                             (int)(char)_refptr_g_tsaPrmtFlash->field_0x5b1,
                             *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5ac,
                             _refptr_g_tsaPrmtFlash->field_0x20,*(undefined1 *)(param_1 + 0xb));
  sVar8 = GetSensorStartDspl((int)(char)_refptr_g_tsaPrmtFlash->field_0x5b0,
                             (int)(char)_refptr_g_tsaPrmtFlash->field_0x5b1,
                             *(undefined2 *)&_refptr_g_tsaPrmtFlash->field_0x5ac,
                             _refptr_g_tsaPrmtFlash->field_0x20,
                             *(char *)(param_1 + 9) + *(char *)(param_1 + 0xb));
  return CONCAT26((sVar8 - *(short *)(param_1 + 0x12)) - *(short *)(param_1 + 0xe),
                  CONCAT24(sVar4 - sVar7,CONCAT22((sVar6 - sVar2) - sVar3,sVar1 - sVar5)));
}


// ============================================================================
// Function: SecVsnprintfImpl @ 6ba436a6
// ============================================================================

int SecVsnprintfImpl(undefined1 *param_1,longlong param_2,undefined8 param_3,undefined8 param_4)

{
  int iVar1;
  int local_28 [2];
  undefined1 *local_20;
  int local_c;
  
                    /* 0x36a6  1251  SecVsnprintfImpl */
  local_28[0] = (int)param_2;
  local_20 = param_1;
  local_c = SecOutputS(local_28,param_3,param_4);
  if ((-1 < local_c) && (iVar1 = SecPutZeroChar(local_28), iVar1 == 0)) {
    return local_c;
  }
  if (local_28[0] < 0) {
    param_1[param_2 + -1] = 0;
    iVar1 = -2;
  }
  else {
    *param_1 = 0;
    iVar1 = -1;
  }
  return iVar1;
}


// ============================================================================
// Function: fputc @ 6bb4e5b0
// ============================================================================

int __cdecl fputc(int _Ch,FILE *_File)

{
  int iVar1;
  
                    /* WARNING: Could not recover jumptable at 0x6bb4e5b0. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  iVar1 = fputc(_Ch,_File);
  return iVar1;
}


// ============================================================================
// Function: fprintf @ 6bb4e5b8
// ============================================================================

int __cdecl fprintf(FILE *_File,char *_Format,...)

{
  int iVar1;
  
                    /* WARNING: Could not recover jumptable at 0x6bb4e5b8. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  iVar1 = fprintf(_File,_Format);
  return iVar1;
}


// ============================================================================
// Function: fopen @ 6bb4e5c0
// ============================================================================

FILE * __cdecl fopen(char *_Filename,char *_Mode)

{
  FILE *pFVar1;
  
                    /* WARNING: Could not recover jumptable at 0x6bb4e5c0. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  pFVar1 = fopen(_Filename,_Mode);
  return pFVar1;
}


// ============================================================================
// Function: fclose @ 6bb4e5d8
// ============================================================================

int __cdecl fclose(FILE *_File)

{
  int iVar1;
  
                    /* WARNING: Could not recover jumptable at 0x6bb4e5d8. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  iVar1 = fclose(_File);
  return iVar1;
}


// ============================================================================
// Function: strcmp @ EXTERNAL:00000057
// ============================================================================
// DECOMPILATION FAILED OR TIMED OUT
// ============================================================================
// Function: MAX @ 6baa0740
// ============================================================================

int MAX(int param_1,int param_2)

{
  if (param_1 <= param_2) {
    param_1 = param_2;
  }
  return param_1;
}


// ============================================================================
// Function: MIN @ 6baa0756
// ============================================================================

int MIN(int param_1,int param_2)

{
  if (param_2 <= param_1) {
    param_1 = param_2;
  }
  return param_1;
}


// ============================================================================
// Function: strncmp @ EXTERNAL:00000059
// ============================================================================
// DECOMPILATION FAILED OR TIMED OUT
// ============================================================================
// Function: memset @ EXTERNAL:00000052
// ============================================================================
// DECOMPILATION FAILED OR TIMED OUT
// ============================================================================
// Function: AnimationInit @ 6baa3500
// ============================================================================

void AnimationInit(void)

{
                    /* 0x63500  333  AnimationInit */
  g_asaAnimalState = 0;
  g_asaAnimalPreState = 0;
  startX = 0xffffffff;
  startY = 0xffffffff;
  return;
}


// ============================================================================
// Function: ButtonInit @ 6baa3bb0
// ============================================================================

void ButtonInit(void)

{
                    /* 0x63bb0  447  ButtonInit */
  g_buttonReleaseCnt = 0;
  return;
}


// ============================================================================
// Function: CoordinateInit @ 6baac440
// ============================================================================

void CoordinateInit(void)

{
                    /* 0x6c440  522  CoordinateInit */
  memset_s(_refptr_g_asaPrpt->pReserved275_to79F + 0x4fb,0x30,0,0x30);
  g_coors = _refptr_g_asaPrpt->pReserved275_to79F + 0x4fb;
  return;
}


// ============================================================================
// Function: CoorReviseInit @ 6bab2370
// ============================================================================

void CoorReviseInit(void)

{
  byte local_9;
  
                    /* 0x72370  519  CoorReviseInit */
  for (local_9 = 0; local_9 < 10; local_9 = local_9 + 1) {
    *(undefined2 *)(&g_coorReviseXBuf + (longlong)(int)(uint)local_9 * 2) = 0;
    *(undefined2 *)(&g_coorReviseYBuf + (longlong)(int)(uint)local_9 * 2) = 0;
  }
  g_coorReviseBufCnt = 0;
  g_coorReviseX = 0;
  g_coorReviseY = 0;
  return;
}


// ============================================================================
// Function: FilterInit @ 6bab29a0
// ============================================================================

void FilterInit(void)

{
                    /* 0x729a0  597  FilterInit */
  return;
}


// ============================================================================
// Function: NoPressInkInit @ 6bab380c
// ============================================================================

void NoPressInkInit(void)

{
  int iVar1;
  
                    /* 0x7380c  938  NoPressInkInit */
  PressTouchSignalInit();
  g_noPressInkTiltCompensation = 0;
  if (g_noPressInkInitOnce != 0) {
    g_noPressInkInitOnce = 0;
    iVar1 = memset_s(_refptr_g_noPressPara,0xc38c,0,0xc38c);
    if ((iVar1 != 0) && (*_refptr_g_tsaLogLoglevel != '\0')) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/ASA/Alg/no_press_ink/asa_no_press_ink.c",0x2d,
                        "rc == EOK");
    }
    _refptr_g_noPressPara->bTx2CompBinMin = 0xff;
    _refptr_g_noPressPara->wActiveTx2CompCoef =
         (ushort)(*_refptr_g_asaPrmtStylus)->bDefaultTx2CompCoef;
    _refptr_g_noPressPara->wTx2CompSignalMax = (*_refptr_g_asaPrmtStylus)->wTx2CompSignalMax;
  }
  _refptr_g_noPressPara->bSignalPosHistoryCount = 0;
  return;
}


// ============================================================================
// Function: GridPeaksInit @ 6babb9d7
// ============================================================================

void GridPeaksInit(void)

{
                    /* 0x7b9d7  639  GridPeaksInit */
  memset_s(_refptr_g_asaData->pPeakFlagMap,0x2648,0,0x2648);
  memset_s(_refptr_g_asaData->pPeakFlagMapBackup,0x2648,0,0x2648);
  memset_s(&g_tx1PeakFlag,0x192,0,0x192);
  return;
}


// ============================================================================
// Function: PeaksInit @ 6babd750
// ============================================================================

void PeaksInit(void)

{
                    /* 0x7d750  998  PeaksInit */
  memset_s(&_refptr_g_asaPrpt->field_0xb0,0x6c0,0,0x6c0);
  *(ushort *)&_refptr_g_asaStatic->field_0x48 = _refptr_g_asaPrmt[1].wField00;
  *(ushort *)&_refptr_g_asaStatic->field_0x4c = _refptr_g_asaPrmt[1].wField00 * 2;
  return;
}


// ============================================================================
// Function: PressureInit @ 6babf389
// ============================================================================

void PressureInit(void)

{
                    /* 0x7f389  1040  PressureInit */
  g_hpp3ExitFlag = false;
  if (*(int *)(_refptr_g_asaHpp3Freq + 4) == 0) {
    BtPressureInit();
  }
  return;
}


// ============================================================================
// Function: RawInit @ 6bac0260
// ============================================================================

void RawInit(void)

{
                    /* 0x80260  1149  RawInit */
  g_asaGroupInfo[0].pLineData = (ushort *)_refptr_g_asaData;
  g_asaGroupInfo[0].pRawBackup = (_refptr_g_asaData->lineProfiles).pLineSumPrimary;
  g_asaGroupInfo[0].pCmnBaseline = (_refptr_g_asaData->lineProfiles).pLineSumBackup + 0x14;
  g_asaGroupInfo[0].pCmnHistory = (int *)(_refptr_g_asaData->pPeakFlagMap + 0x14);
  g_asaGroupInfo[0].pAuxNoiseBuffer = _refptr_g_asaData->pPeakFlagMap + 0x68;
  g_asaGroupInfo[0].pPeakReport = &_refptr_g_asaPrpt->field_0xb0;
  g_asaGroupInfo[0].pGroupParam = &_refptr_g_asaPrePrpt->field_0xb0;
  g_asaGroupInfo[0].bGroupLength = _refptr_g_asaPrmt->bTxCount;
  g_asaGroupInfo[1].pLineData =
       (_refptr_g_asaData->lineProfiles).pRawLineSamples + (int)(uint)_refptr_g_asaPrmt->bTxCount;
  g_asaGroupInfo[1].pRawBackup =
       (_refptr_g_asaData->lineProfiles).pLineSumPrimary + (int)(uint)_refptr_g_asaPrmt->bTxCount;
  g_asaGroupInfo[1].pCmnBaseline =
       (_refptr_g_asaData->lineProfiles).pLineSumBackup +
       (longlong)(int)(uint)_refptr_g_asaPrmt->bTxCount + 0x14;
  g_asaGroupInfo[1].pCmnHistory = (int *)(_refptr_g_asaData->pPeakFlagMap + 0x3e);
  g_asaGroupInfo[1].pAuxNoiseBuffer =
       _refptr_g_asaData->pPeakFlagMap + (longlong)(int)(uint)_refptr_g_asaPrmt->bTxCount + 0x68;
  g_asaGroupInfo[1].pPeakReport = &_refptr_g_asaPrpt->field_0x260;
  g_asaGroupInfo[1].pGroupParam = &_refptr_g_asaPrePrpt->field_0x260;
  g_asaGroupInfo[1].bGroupLength = _refptr_g_asaPrmt->bRxCount;
  g_asaGroupInfo[2].pLineData = (_refptr_g_asaData->lineProfiles).pLineSumPrimary;
  g_asaGroupInfo[2].pRawBackup = (ushort *)(_refptr_g_asaData->pPad_9ce0 + 0x72d8);
  g_asaGroupInfo[2].pCmnBaseline = (ushort *)(_refptr_g_asaData->pPad_9ce0 + 0x7418);
  g_asaGroupInfo[2].pCmnHistory = (int *)(_refptr_g_asaData->pPad_9ce0 + 0x7728);
  g_asaGroupInfo[2].pAuxNoiseBuffer = (ushort *)(_refptr_g_asaData->pPad_9ce0 + 0x7558);
  g_asaGroupInfo[2].pPeakReport = &_refptr_g_asaPrpt->field_0xb0;
  g_asaGroupInfo[2].pGroupParam = &_refptr_g_asaPrePrpt->field_0xb0;
  g_asaGroupInfo[2].bGroupLength = _refptr_g_asaPrmt->bTxCount;
  g_asaGroupInfo[3].pLineData =
       (_refptr_g_asaData->lineProfiles).pLineSumPrimary + (int)(uint)_refptr_g_asaPrmt->bTxCount;
  g_asaGroupInfo[3].pRawBackup =
       (ushort *)
       (_refptr_g_asaData->pPad_9ce0 + (longlong)(int)(uint)_refptr_g_asaPrmt->bTxCount * 2 + 0x72d8
       );
  g_asaGroupInfo[3].pCmnBaseline =
       (ushort *)
       (_refptr_g_asaData->pPad_9ce0 + (longlong)(int)(uint)_refptr_g_asaPrmt->bTxCount * 2 + 0x7418
       );
  g_asaGroupInfo[3].pCmnHistory = (int *)(_refptr_g_asaData->pPad_9ce0 + 0x777c);
  g_asaGroupInfo[3].pAuxNoiseBuffer =
       (ushort *)
       (_refptr_g_asaData->pPad_9ce0 + (longlong)(int)(uint)_refptr_g_asaPrmt->bTxCount * 2 + 0x7558
       );
  g_asaGroupInfo[3].pPeakReport = &_refptr_g_asaPrpt->field_0x260;
  g_asaGroupInfo[3].pGroupParam = &_refptr_g_asaPrePrpt->field_0x260;
  g_asaGroupInfo[3].bGroupLength = _refptr_g_asaPrmt->bRxCount;
  g_asaGroupInfo[4].pLineData = (_refptr_g_asaData->lineProfiles).pLineSumBackup;
  g_asaGroupInfo[4].pRawBackup = (ushort *)(_refptr_g_asaData->pPad_9ce0 + 0x77d0);
  g_asaGroupInfo[4].pCmnBaseline = (ushort *)(_refptr_g_asaData->pPad_9ce0 + 0x7910);
  g_asaGroupInfo[4].pCmnHistory = (int *)(_refptr_g_asaData->pPad_9ce0 + 0x7c20);
  g_asaGroupInfo[4].pAuxNoiseBuffer = (ushort *)(_refptr_g_asaData->pPad_9ce0 + 0x7a50);
  g_asaGroupInfo[4].pPeakReport = _refptr_g_asaPrpt->pReserved275_to79F + 0x19b;
  g_asaGroupInfo[4].pGroupParam = &_refptr_g_asaPrePrpt[1].field_0x19b;
  g_asaGroupInfo[4].bGroupLength = _refptr_g_asaPrmt->bTxCount;
  g_asaGroupInfo[5].pLineData =
       (_refptr_g_asaData->lineProfiles).pLineSumBackup + (int)(uint)_refptr_g_asaPrmt->bTxCount;
  g_asaGroupInfo[5].pRawBackup =
       (ushort *)
       (_refptr_g_asaData->pPad_9ce0 + (longlong)(int)(uint)_refptr_g_asaPrmt->bTxCount * 2 + 0x77d0
       );
  g_asaGroupInfo[5].pCmnBaseline =
       (ushort *)
       (_refptr_g_asaData->pPad_9ce0 + (longlong)(int)(uint)_refptr_g_asaPrmt->bTxCount * 2 + 0x7910
       );
  g_asaGroupInfo[5].pCmnHistory = (int *)(_refptr_g_asaData->pPad_9ce0 + 0x7c74);
  g_asaGroupInfo[5].pAuxNoiseBuffer =
       (ushort *)
       (_refptr_g_asaData->pPad_9ce0 + (longlong)(int)(uint)_refptr_g_asaPrmt->bTxCount * 2 + 0x7a50
       );
  g_asaGroupInfo[5].pPeakReport = _refptr_g_asaPrpt->pReserved275_to79F + 0x34b;
  g_asaGroupInfo[5].pGroupParam = &_refptr_g_asaPrePrpt[2].field_0xd6;
  g_asaGroupInfo[5].bGroupLength = _refptr_g_asaPrmt->bRxCount;
  *_refptr_g_flagTX2Start = 0;
  return;
}


// ============================================================================
// Function: ReportInit @ 6bac311c
// ============================================================================

void ReportInit(void)

{
                    /* 0x8311c  1170  ReportInit */
  ASAOutClean();
  return;
}


// ============================================================================
// Function: TiltInit @ 6bac3700
// ============================================================================

void TiltInit(void)

{
  byte local_a;
  byte local_9;
  
                    /* 0x83700  2463  TiltInit */
  for (local_9 = 0; local_9 < 10; local_9 = local_9 + 1) {
    (&g_coordifdim1Buf)[(int)(uint)local_9] = 0;
    (&g_coordifdim2Buf)[(int)(uint)local_9] = 0;
    (&g_tiltdim1Buf)[(int)(uint)local_9] = 0;
    (&g_tiltdim2Buf)[(int)(uint)local_9] = 0;
  }
  g_coorDifBufCnt = 0;
  for (local_a = 0; local_a < 10; local_a = local_a + 1) {
    (&g_signalRatioBuf)[(int)(uint)local_a] = 0;
  }
  g_signalRatioBufCnt = 0;
  return;
}


// ============================================================================
// Function: ASAStaticReset @ 6bac8829
// ============================================================================

void ASAStaticReset(void)

{
                    /* 0x88829  114  ASAStaticReset */
  ExitActiveStylusMode();
  return;
}


// ============================================================================
// Function: GetSensorStartDspl @ 6bb26af1
// ============================================================================

int GetSensorStartDspl(char param_1,char param_2,short param_3,byte param_4,byte param_5)

{
  longlong lVar1;
  
  lVar1 = (longlong)(int)((uint)param_5 * (int)(short)(param_3 + (short)param_2 + (short)param_1)) /
          (longlong)(int)(uint)param_4;
  return (int)lVar1 - (int)CONCAT62((int6)((ulonglong)lVar1 >> 0x10),(short)param_1);
}


// ============================================================================
// Function: SecOutputS @ 6ba42f21
// ============================================================================

int SecOutputS(undefined8 param_1,char *param_2,ulonglong *param_3)

{
  int iVar1;
  ulonglong *local_res18;
  undefined1 local_2c8 [48];
  char *local_298;
  undefined8 local_290;
  int local_288;
  undefined4 local_284;
  undefined4 local_280;
  uint local_27c;
  uint local_278;
  uint local_274;
  undefined4 local_270;
  undefined4 local_26c;
  int local_268;
  int local_264;
  undefined1 local_260;
  undefined1 local_25f;
  int local_4c;
  char *local_48;
  ulonglong local_40;
  ulonglong local_38;
  ulonglong local_30;
  uint local_24;
  char local_1d;
  int local_1c;
  ulonglong local_18;
  int local_10;
  int local_c;
  
  local_c = 0;
  local_27c = 0;
  local_284 = 0;
  local_278 = 0;
  local_274 = 0;
  local_270 = 0;
  local_26c = 0;
  local_298 = "0123456789ABCDEFX";
  local_280 = 10;
  local_268 = 0;
  local_288 = 0;
  local_290 = 0;
  local_264 = 0;
  local_260 = 0;
  local_25f = 0;
  local_4c = 0;
  local_10 = 0;
  local_res18 = param_3;
  local_48 = param_2;
  do {
    if ((*local_48 == '\0') || (local_4c < 0)) {
      if ((local_10 != 0) && (local_10 != 7)) {
        return -1;
      }
      return local_4c;
    }
    local_1c = local_10;
    local_1d = *local_48;
    local_48 = local_48 + 1;
    local_10 = SecDecodeState((int)local_1d,local_10);
    switch(local_10) {
    case 0:
      SecOutputOneChar((int)local_1d,param_1,&local_4c);
      break;
    case 1:
      local_c = 0;
      local_264 = 0;
      local_288 = 0;
      local_27c = 0;
      local_278 = 0;
      local_274 = 0xffffffff;
      local_284 = 0;
      local_270 = 0;
      local_26c = 0;
      break;
    case 2:
      SecDecodeFlags((int)local_1d,&local_298);
      break;
    case 3:
      if (local_1d == '*') {
        local_278 = (uint)*local_res18;
        local_270 = 1;
        local_res18 = local_res18 + 1;
      }
      iVar1 = SecDecodeWidth((int)local_1d,&local_298,local_1c);
      if (iVar1 != 0) {
        return -1;
      }
      break;
    case 4:
      local_274 = 0;
      break;
    case 5:
      if (local_1d == '*') {
        local_274 = (uint)*local_res18;
        local_26c = 1;
        local_res18 = local_res18 + 1;
      }
      iVar1 = SecDecodePrecision((int)local_1d,&local_298);
      if (iVar1 != 0) {
        return -1;
      }
      break;
    case 6:
      iVar1 = SecDecodeSize((int)local_1d,&local_298,&local_48);
      if (iVar1 != 0) {
        SecOutputOneChar((int)local_1d,param_1,&local_4c);
        local_10 = 0;
      }
      break;
    case 7:
      switch(local_1d) {
      case 'C':
        SecUpdateWcharFlags(&local_298);
      case 'c':
        local_24 = (uint)*local_res18;
        SecDecodeTypeC(&local_298,local_24);
        local_res18 = local_res18 + 1;
        break;
      case 'G':
      case 'g':
        if (local_274 == 0) {
          local_274 = 1;
        }
      case 'E':
      case 'F':
      case 'e':
      case 'f':
        local_c = 1;
        iVar1 = SecInitFloatBuffer(local_2c8,local_48,&local_298);
        if (iVar1 == 0) {
          if ((local_27c & 0x400) == 0) {
            local_40 = *local_res18;
            SecFormatDouble(&local_298,local_2c8,local_40);
          }
          else {
            local_38 = *local_res18;
            SecFormatDouble(&local_298,local_2c8,local_38);
          }
          local_res18 = local_res18 + 1;
          SecWriteFloatText(param_1,&local_298,&local_4c);
          SecFreeFloatBuffer(local_2c8);
        }
        break;
      case 'S':
        SecUpdateWstringFlags(&local_298);
      case 's':
        local_30 = *local_res18;
        SecDecodeTypeS(&local_298,local_30);
        local_res18 = local_res18 + 1;
        break;
      case 'X':
      case 'p':
      case 'x':
        SecUpdateXpxFlags(&local_298,(int)local_1d);
      case 'd':
      case 'i':
      case 'o':
      case 'u':
        SecUpdateOudiFlags(&local_298,(int)local_1d);
        if (((local_27c & 0x8000) == 0) && ((local_27c & 0x1000) == 0)) {
          if ((local_27c & 0x10) == 0) {
            if ((local_27c & 0x2000) == 0) {
              if ((local_27c & 0x20) == 0) {
                if ((local_27c & 0x40) == 0) {
                  local_18 = (ulonglong)(uint)*local_res18;
                }
                else {
                  local_18 = (ulonglong)(int)(uint)*local_res18;
                }
              }
              else if ((local_27c & 0x40) == 0) {
                local_18 = (ulonglong)((uint)*local_res18 & 0xffff);
              }
              else {
                local_18 = (ulonglong)(short)(uint)*local_res18;
              }
            }
            else if ((local_27c & 0x40) == 0) {
              local_18 = (ulonglong)((uint)*local_res18 & 0xff);
            }
            else {
              local_18 = SecUpdateNegativeChar(&local_298,(int)(char)(uint)*local_res18);
            }
          }
          else if ((local_27c & 0x40) == 0) {
            local_18 = (ulonglong)(uint)*local_res18;
          }
          else {
            local_18 = (ulonglong)(int)(uint)*local_res18;
          }
        }
        else {
          local_18 = *local_res18;
        }
        local_res18 = local_res18 + 1;
        SecNumberToBuffer(&local_298,local_18);
        SecNumberSatisfyPrecision(&local_298);
        SecNumberForceOctal(&local_298);
        SecUpdateSignedNumberPrefix(&local_298);
        if (local_18 == 0) {
          SecNumberCompatZero(&local_298);
        }
      }
      if (local_c == 0) {
        local_268 = (local_278 - local_288) - local_264;
        SecWriteLeftPadding(param_1,&local_298,&local_4c);
        SecWritePrefix(param_1,&local_298,&local_4c);
        SecWriteLeadingZero(param_1,&local_298,&local_4c);
        SecWriteText(param_1,&local_298,&local_4c);
        SecWriteRightPadding(param_1,&local_298,&local_4c);
      }
      break;
    default:
      return -1;
    }
  } while( true );
}


// ============================================================================
// Function: SecPutZeroChar @ 6ba43658
// ============================================================================

undefined8 SecPutZeroChar(int *param_1)

{
  undefined8 uVar1;
  
  *param_1 = *param_1 + -1;
  if (*param_1 < 0) {
    uVar1 = 0xffffffff;
  }
  else {
    **(undefined1 **)(param_1 + 2) = 0;
    *(longlong *)(param_1 + 2) = *(longlong *)(param_1 + 2) + 1;
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: fputc @ EXTERNAL:00000044
// ============================================================================
// DECOMPILATION FAILED OR TIMED OUT
// ============================================================================
// Function: fprintf @ EXTERNAL:00000043
// ============================================================================
// DECOMPILATION FAILED OR TIMED OUT
// ============================================================================
// Function: fopen @ EXTERNAL:00000042
// ============================================================================
// DECOMPILATION FAILED OR TIMED OUT
// ============================================================================
// Function: fclose @ EXTERNAL:0000003f
// ============================================================================
// DECOMPILATION FAILED OR TIMED OUT
// ============================================================================
// Function: ClearHotZone @ 6baf3be8
// ============================================================================

void ClearHotZone(void)

{
  memset_s(&g_hotZoneRect,700,0,700);
  StrPrintf_Process(0,"\nAppLog: ");
  StrPrintf_Process(0x10,"\nHotZone cleared");
  return;
}


// ============================================================================
// Function: HotZoneAdjustIsFeatureOn @ 6baf404a
// ============================================================================

undefined4 HotZoneAdjustIsFeatureOn(void)

{
  TSA_FeatureFlags TVar1;
  
  TVar1 = (*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x400000;
  return CONCAT31((int3)(TVar1 >> 8),TVar1 != 0);
}


// ============================================================================
// Function: HotZoneAdjustScenario @ 6baf406a
// ============================================================================

undefined8 HotZoneAdjustScenario(void)

{
  char cVar1;
  undefined8 uVar2;
  
  cVar1 = TSA_IsTypingScenario();
  if (cVar1 == '\0') {
    cVar1 = TSAStatic_IsScreenRotated();
    if (cVar1 == '\0') {
      uVar2 = 1;
    }
    else {
      uVar2 = 0;
    }
  }
  else {
    uVar2 = 0;
  }
  return uVar2;
}


// ============================================================================
// Function: HotZoneAdjustProcessFirst @ 6baf5d93
// ============================================================================

void HotZoneAdjustProcessFirst(byte param_1)

{
  undefined1 local_18 [15];
  byte local_9;
  
  memset_s(local_18,0xe,0,0xe);
  local_9 = UpdateHotZonePosition(param_1,local_18);
  if (local_9 == 1) {
    StrPrintf_Process(0,"\nAppLog: ");
    StrPrintf_Process(0x10,"\nHotZone Id=%d, NO Widgets",
                      *(undefined1 *)
                       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x154));
    return;
  }
  if (local_9 < 2) {
    if (local_9 == 0) {
      HotZoneAdjustProcessFirstUnit(param_1,local_18);
      return;
    }
  }
  else {
    if (local_9 == 2) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(0x10,"\nHotZone Id=%d, Too close",
                        *(undefined1 *)
                         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x154));
      return;
    }
    if (local_9 == 3) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(0x10,"\nHotZone Id=%d, In core area",
                        *(undefined1 *)
                         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x154));
      return;
    }
  }
  StrPrintf_Process(0,"\nAppLog: ");
  StrPrintf_Process(0x10,"\nHotZone Id=%d, Default not adjust",
                    *(undefined1 *)
                     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x154));
  return;
}


// ============================================================================
// Function: HotZoneAdjustProcessAfter @ 6baf5f50
// ============================================================================

void HotZoneAdjustProcessAfter(byte param_1,byte param_2)

{
  undefined2 uVar1;
  undefined2 uVar2;
  undefined2 uVar3;
  undefined2 uVar4;
  short sVar5;
  short sVar6;
  
  sVar6 = *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x170);
  sVar5 = *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x172);
  if ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x200)
      & 8) != 0) {
    AdjustByHistory(param_1,param_2);
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x200) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x200)
         | 8;
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      uVar1 = TSAOut_Dim2PosInternal
                        (*(undefined2 *)
                          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x172),1);
      uVar2 = TSAOut_Dim1PosInternal
                        (*(undefined2 *)
                          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x170),1);
      uVar3 = TSAOut_Dim2PosInternal(sVar5,1);
      uVar4 = TSAOut_Dim1PosInternal(sVar6,1);
      StrPrintf_Process(0,"\nHotZone Id=%d, Adjust OK before %d#%d, after %d#%d\n",
                        *(undefined1 *)
                         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x154),uVar4,uVar3,uVar2,uVar1);
    }
    StrPrintf_Process(0,"\nAppLog: ");
    sVar5 = TSAOut_IntToExtDist((int)(short)(*(short *)((longlong)(int)(uint)param_1 * 0x168 +
                                                        *(longlong *)_refptr_g_touchesPtr + 0x172) -
                                            sVar5));
    sVar6 = TSAOut_IntToExtDist((int)(short)(*(short *)((longlong)(int)(uint)param_1 * 0x168 +
                                                        *(longlong *)_refptr_g_touchesPtr + 0x170) -
                                            sVar6));
    StrPrintf_Process(0x10,"\nHotZone Id=%d, Adjust OK %d#%d\n",
                      *(undefined1 *)
                       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x154),(int)sVar6,(int)sVar5);
  }
  return;
}


// ============================================================================
// Function: GenerateAndGetDetailDescription @ 6baf6d4a
// ============================================================================

undefined * GenerateAndGetDetailDescription(void)

{
  GenerateDetailDescription();
  ClearDiffNum(&g_hotZoneAnalyzer);
  return &DAT_6bba3590;
}


// ============================================================================
// Function: IsNeedRecord @ 6baf6dae
// ============================================================================

undefined8 IsNeedRecord(byte param_1,longlong param_2)

{
  short sVar1;
  char cVar2;
  ushort uVar3;
  ushort uVar4;
  ushort uVar5;
  ushort *puVar6;
  
  param_2 = param_2 + (longlong)
                      (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                          (longlong)(int)(uint)param_1 * 0x478 + 0x154) * 0xe;
  puVar6 = (ushort *)(param_2 + 0x412);
  if ((((*puVar6 != 0) || (*(short *)(param_2 + 0x416) != 0)) || (*(short *)(param_2 + 0x414) != 0))
     || (*(short *)(param_2 + 0x418) != 0)) {
    if ((*(int *)&_refptr_g_tsaPrmtFlash[2].field_0x7b8 == 0) &&
       (cVar2 = HotZoneIsInCore(puVar6), cVar2 != '\0')) {
      return 0;
    }
    sVar1 = *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                      (longlong)(int)(uint)param_1 * 0x478 + 0x4ac);
    uVar3 = TSAOut_ExtToIntDist(100);
    if ((((int)(uint)*puVar6 <= (int)((int)sVar1 + (uint)uVar3)) &&
        (sVar1 = *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                           (longlong)(int)(uint)param_1 * 0x478 + 0x4ac),
        uVar3 = *(ushort *)(param_2 + 0x416), uVar4 = TSAOut_ExtToIntDist(100),
        (int)sVar1 <= (int)((uint)uVar4 + (uint)uVar3))) &&
       ((sVar1 = *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                           (longlong)(int)(uint)param_1 * 0x478 + 0x4ae),
        uVar3 = TSAOut_ExtToIntDist(100),
        (int)(uint)*(ushort *)(param_2 + 0x414) <= (int)((int)sVar1 + (uint)uVar3) &&
        (sVar1 = *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                           (longlong)(int)(uint)param_1 * 0x478 + 0x4ae),
        uVar3 = *(ushort *)(param_2 + 0x418), uVar4 = TSAOut_ExtToIntDist(100),
        (int)sVar1 <= (int)((uint)uVar4 + (uint)uVar3))))) {
      uVar3 = *(ushort *)
               ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x16c);
      uVar4 = TSAOut_ExtToIntDist(100);
      if (((((uint)uVar3 + (uint)uVar4 < (uint)*puVar6) ||
           (uVar3 = *(ushort *)
                     ((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)param_1 * 0x478 + 0x16c),
           uVar4 = *(ushort *)(param_2 + 0x416), uVar5 = TSAOut_ExtToIntDist(100),
           (uint)uVar5 + (uint)uVar4 < (uint)uVar3)) ||
          (uVar3 = *(ushort *)
                    ((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_1 * 0x478 + 0x16e), uVar4 = TSAOut_ExtToIntDist(100),
          (uint)uVar3 + (uint)uVar4 < (uint)*(ushort *)(param_2 + 0x414))) ||
         (uVar3 = *(ushort *)
                   ((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_1 * 0x478 + 0x16e),
         uVar4 = *(ushort *)(param_2 + 0x418), uVar5 = TSAOut_ExtToIntDist(100),
         (uint)uVar5 + (uint)uVar4 < (uint)uVar3)) {
        return 0;
      }
      uVar3 = *(ushort *)
               ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_1 * 0x478 + 0x16c);
      sVar1 = *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                        (longlong)(int)(uint)param_1 * 0x478 + 0x4ac);
      uVar4 = TSAOut_ExtToIntDist(10);
      if ((((int)(uint)uVar3 <= (int)((uint)uVar4 + (int)sVar1)) &&
          (uVar3 = *(ushort *)
                    ((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_1 * 0x478 + 0x16e),
          sVar1 = *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                            (longlong)(int)(uint)param_1 * 0x478 + 0x4ae),
          uVar4 = TSAOut_ExtToIntDist(10), (int)(uint)uVar3 <= (int)((uint)uVar4 + (int)sVar1))) &&
         ((sVar1 = *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)param_1 * 0x478 + 0x4ac),
          uVar3 = *(ushort *)
                   ((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_1 * 0x478 + 0x16c), uVar4 = TSAOut_ExtToIntDist(10),
          (int)sVar1 <= (int)((uint)uVar4 + (uint)uVar3) &&
          (sVar1 = *(short *)((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)param_1 * 0x478 + 0x4ae),
          uVar3 = *(ushort *)
                   ((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_1 * 0x478 + 0x16e), uVar4 = TSAOut_ExtToIntDist(10),
          (int)sVar1 <= (int)((uint)uVar4 + (uint)uVar3))))) {
        return 1;
      }
      return 0;
    }
  }
  return 0;
}


// ============================================================================
// Function: HotZoneRecord @ 6baf726b
// ============================================================================

void HotZoneRecord(undefined1 param_1,undefined8 param_2)

{
  CalculateAppIndex(param_2);
  CalculateSuccessNum(param_1,param_2);
  return;
}


// ============================================================================
// Function: .text @ 6ba8d7e0
// ============================================================================

int _text(int param_1,int param_2)

{
  if (param_1 <= param_2) {
    param_1 = param_2;
  }
  return param_1;
}


// ============================================================================
// Function: malloc @ 6bb4e560
// ============================================================================

void * __cdecl malloc(size_t _Size)

{
  void *pvVar1;
  
                    /* WARNING: Could not recover jumptable at 0x6bb4e560. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  pvVar1 = malloc(_Size);
  return pvVar1;
}


// ============================================================================
// Function: free @ 6bb4e5a0
// ============================================================================

void __cdecl free(void *_Memory)

{
                    /* WARNING: Could not recover jumptable at 0x6bb4e5a0. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  free(_Memory);
  return;
}


// ============================================================================
// Function: sqrt @ 6bb48e10
// ============================================================================

double __cdecl sqrt(double _X)

{
  int *piVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  int local_20;
  uint uStack_1c;
  
  uStack_1c = (uint)((ulonglong)_X >> 0x20);
  local_20 = SUB84(_X,0);
  if ((((ulonglong)_X & 0xfffff00000000) == 0 && local_20 == 0) &&
      ((ulonglong)_X & 0x7ff0000000000000) == 0) {
    if (-1 < (longlong)_X) {
      return 0.0;
    }
    return DAT_6bb60658;
  }
  if ((((ulonglong)_X & 0x7ff0000000000000) == 0) || ((uStack_1c & 0x7ff00000) != 0x7ff00000)) {
    if (-1 < (longlong)_X) {
      if (_X == DAT_6bb60668) {
        return DAT_6bb60668;
      }
      return SQRT(_X);
    }
  }
  else if (((ulonglong)_X & 0xfffff00000000) == 0 && local_20 == 0) {
    if (-1 < (longlong)_X) {
      return DAT_6bb60660;
    }
  }
  else if (-1 < (longlong)_X) {
    uVar2 = (undefined4)DAT_6bb60650;
    uVar3 = (undefined4)((ulonglong)DAT_6bb60650 >> 0x20);
    goto LAB_6bb48ecd;
  }
  uVar2 = (undefined4)DAT_6bb60648;
  uVar3 = (undefined4)((ulonglong)DAT_6bb60648 >> 0x20);
LAB_6bb48ecd:
  piVar1 = _errno();
  *piVar1 = 0x21;
  _text(1,&_rdata,_X,0,CONCAT44(uVar3,uVar2));
  return (double)CONCAT44(uVar3,uVar2);
}


// ============================================================================
// Function: LinearTestInit @ 6baa3c80
// ============================================================================

void LinearTestInit(void)

{
                    /* 0x63c80  899  LinearTestInit */
  g_frameBufCnt = 0;
  return;
}


// ============================================================================
// Function: OemInfoCfgInit @ 6baa727c
// ============================================================================

void OemInfoCfgInit(void)

{
  undefined1 local_d;
  uint local_c;
  
                    /* 0x6727c  950  OemInfoCfgInit */
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"\nOemInfoCfgInit! %d, %d",_refptr_g_asaPrmt->bTxCount,
                      _refptr_g_asaPrmt->bRxCount);
  }
  for (local_c = 0; local_c < 3; local_c = local_c + 1) {
    local_d = 0;
    *(undefined4 *)(&g_oemDataCfg + (ulonglong)local_c * 0xa8) = 0;
    if (local_c == 1) {
      OemSubDataRegister(1,0,_refptr_g_asaCalibrationTable + 0x9a19,1);
      OemSubDataRegister(1,1,_refptr_g_asaCalibrationTable + 0x9a1a,_refptr_g_asaPrmt->bTxCount);
      OemSubDataRegister(1,2,_refptr_g_asaCalibrationTable + 0x26ee,2);
      OemSubDataRegister(1,3,_refptr_g_asaCalibrationTable + 0x1326,
                         (uint)_refptr_g_asaPrmt->bRxCount * (uint)_refptr_g_asaPrmt->bTxCount);
      OemSubDataRegister(1,4,_refptr_g_asaCalibrationTable + 0x9a6c,2);
      local_d = 6;
      OemSubDataRegister(1,5,_refptr_g_asaCalibrationTable + 0x9a74,6);
    }
    else if (local_c == 0) {
      OemSubDataRegister(0,0,_refptr_g_asaCalibrationTable + 0x99c8,1);
      OemSubDataRegister(0,1,_refptr_g_asaCalibrationTable + 0x99c9,_refptr_g_asaPrmt->bRxCount);
      OemSubDataRegister(0,2,_refptr_g_asaCalibrationTable + 0x26ec,2);
      OemSubDataRegister(0,3,_refptr_g_asaCalibrationTable + 2,
                         (uint)_refptr_g_asaPrmt->bRxCount * (uint)_refptr_g_asaPrmt->bTxCount);
      OemSubDataRegister(0,4,_refptr_g_asaCalibrationTable + 0x9a6a,2);
      local_d = 6;
      OemSubDataRegister(0,5,_refptr_g_asaCalibrationTable + 0x9a6e,6);
    }
    else if (local_c == 2) {
      local_d = 1;
      OemSubDataRegister(2,0,_refptr_g_ltRet,0x54);
    }
    (&DAT_6bb86304)[(ulonglong)local_c * 0xa8] = local_d;
  }
  return;
}


// ============================================================================
// Function: PositionOffsetCalibrationInit @ 6baa7fb0
// ============================================================================

void PositionOffsetCalibrationInit(void)

{
                    /* 0x67fb0  1002  PositionOffsetCalibrationInit */
  g_dim1PstnOfst = 0;
  g_dim2PstnOfst = 0;
  g_positionOffsetCalLineNum = 0;
  return;
}


// ============================================================================
// Function: PressTouchSignalInit @ 6bab37f0
// ============================================================================

void PressTouchSignalInit(void)

{
  _refptr_g_asaStatic->bExitNoPressInkDebounceCnt = 2;
  _refptr_g_asaStatic->bEnterNoPressInkDebounceCnt = 2;
  return;
}


// ============================================================================
// Function: BtPressureInit @ 6babf350
// ============================================================================

void BtPressureInit(void)

{
                    /* 0x7f350  444  BtPressureInit */
  *_refptr_g_btPressBuf = 0;
  _refptr_g_btPressBuf[1] = 0;
  _refptr_g_btPressBuf[2] = 0;
  _refptr_g_btPressBuf[3] = 0;
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
// Function: ExitActiveStylusMode @ 6bac87b1
// ============================================================================

void ExitActiveStylusMode(void)

{
                    /* 0x887b1  586  ExitActiveStylusMode */
  ASAStaticCounterClean();
  g_asaStatic.dwAsaStatusFlags = 0;
  DAT_6bc71a2c = 0;
  DAT_6bc71a2d = 1;
  ASAOutClean();
  CoorInit();
  memcpy(&g_prevASOut,&g_curASOut,0xec);
  ASAStaticPostProcess();
  ExitInRangeMode();
  AnimationInit();
  return;
}


// ============================================================================
// Function: SecFormatDouble @ 6ba41813
// ============================================================================

void SecFormatDouble(longlong param_1,longlong param_2,undefined8 param_3)

{
  int iVar1;
  undefined4 uVar2;
  
  if ((*(uint *)(param_1 + 0x1c) & 4) == 0) {
    iVar1 = *(int *)(param_1 + 0x20);
  }
  else {
    iVar1 = -*(int *)(param_1 + 0x20);
  }
  if ((*(int *)(param_1 + 0x28) == 0) || (*(int *)(param_1 + 0x2c) == 0)) {
    if (*(int *)(param_1 + 0x28) == 0) {
      if (*(int *)(param_1 + 0x2c) == 0) {
        uVar2 = SecFormatFloat(*(undefined8 *)(param_1 + 8),*(undefined8 *)(param_2 + 0x10),param_3)
        ;
        *(undefined4 *)(param_1 + 0x10) = uVar2;
      }
      else {
        uVar2 = SecFormatFloat(*(undefined8 *)(param_1 + 8),*(undefined8 *)(param_2 + 0x10),
                               *(undefined4 *)(param_1 + 0x24),param_3);
        *(undefined4 *)(param_1 + 0x10) = uVar2;
      }
    }
    else {
      uVar2 = SecFormatFloat(*(undefined8 *)(param_1 + 8),*(undefined8 *)(param_2 + 0x10),iVar1,
                             param_3);
      *(undefined4 *)(param_1 + 0x10) = uVar2;
    }
  }
  else {
    uVar2 = SecFormatFloat(*(undefined8 *)(param_1 + 8),*(undefined8 *)(param_2 + 0x10),iVar1,
                           *(undefined4 *)(param_1 + 0x24),param_3);
    *(undefined4 *)(param_1 + 0x10) = uVar2;
  }
  if ((*(int *)(param_1 + 0x10) < 0) || (*(int *)(param_2 + 0x28) <= *(int *)(param_1 + 0x10))) {
    *(undefined4 *)(param_1 + 0x10) = 0;
  }
  return;
}


// ============================================================================
// Function: SecNumberToBuffer @ 6ba41bb4
// ============================================================================

void SecNumberToBuffer(longlong param_1,ulonglong param_2)

{
  int iVar1;
  ulonglong local_10;
  
  if (((*(uint *)(param_1 + 0x1c) & 0x40) == 0) || (-1 < (longlong)param_2)) {
    local_10 = param_2;
  }
  else {
    local_10 = -param_2;
    *(uint *)(param_1 + 0x1c) = *(uint *)(param_1 + 0x1c) | 0x100;
  }
  iVar1 = SecIsNumberNeedTo32Bit(param_1);
  if (iVar1 != 0) {
    local_10 = local_10 & 0xffffffff;
  }
  *(longlong *)(param_1 + 8) = param_1 + 0x23a;
  if (local_10 == 0) {
    *(undefined4 *)(param_1 + 0x34) = 0;
    *(undefined4 *)(param_1 + 0x10) = 0;
  }
  else {
    SecNumberToString(local_10,param_1);
    *(int *)(param_1 + 0x10) = ((int)param_1 + 0x23a) - (int)*(undefined8 *)(param_1 + 8);
  }
  return;
}


// ============================================================================
// Function: SecWriteLeftPadding @ 6ba41d98
// ============================================================================

void SecWriteLeftPadding(undefined8 param_1,longlong param_2,undefined8 param_3)

{
  if (((*(uint *)(param_2 + 0x1c) & 0xc) == 0) && (0 < *(int *)(param_2 + 0x30))) {
    SecWriteMultiChar(0x20,*(undefined4 *)(param_2 + 0x30),param_1,param_3);
  }
  return;
}


// ============================================================================
// Function: SecWritePrefix @ 6ba41dea
// ============================================================================

void SecWritePrefix(undefined8 param_1,longlong param_2,int *param_3)

{
  int iVar1;
  
  if (0 < *(int *)(param_2 + 0x34)) {
    iVar1 = SecIsStreamBufEnough(param_1,*(undefined4 *)(param_2 + 0x34));
    if (iVar1 == 0) {
      SecWriteString(param_2 + 0x38,*(undefined4 *)(param_2 + 0x34),param_1,param_3);
    }
    else {
      SecWriteStringToStreamOpt(param_1,param_2 + 0x38,*(undefined4 *)(param_2 + 0x34));
      *param_3 = *param_3 + *(int *)(param_2 + 0x34);
    }
  }
  return;
}


// ============================================================================
// Function: SecWriteLeadingZero @ 6ba41e7c
// ============================================================================

void SecWriteLeadingZero(undefined8 param_1,longlong param_2,undefined8 param_3)

{
  if ((((*(uint *)(param_2 + 0x1c) & 8) != 0) && ((*(uint *)(param_2 + 0x1c) & 4) == 0)) &&
     (0 < *(int *)(param_2 + 0x30))) {
    SecWriteMultiChar(0x30,*(undefined4 *)(param_2 + 0x30),param_1,param_3);
  }
  return;
}


// ============================================================================
// Function: SecWriteRightPadding @ 6ba41edc
// ============================================================================

void SecWriteRightPadding(undefined8 param_1,longlong param_2,int *param_3)

{
  if (((-1 < *param_3) && ((*(uint *)(param_2 + 0x1c) & 4) != 0)) && (0 < *(int *)(param_2 + 0x30)))
  {
    SecWriteMultiChar(0x20,*(undefined4 *)(param_2 + 0x30),param_1,param_3);
  }
  return;
}


// ============================================================================
// Function: SecWriteFloatText @ 6ba42040
// ============================================================================

void SecWriteFloatText(undefined8 param_1,longlong param_2,undefined8 param_3)

{
  SecWriteString(*(undefined8 *)(param_2 + 8),*(undefined4 *)(param_2 + 0x10),param_1,param_3);
  return;
}


// ============================================================================
// Function: SecWriteText @ 6ba4207d
// ============================================================================

void SecWriteText(undefined8 param_1,longlong param_2,undefined8 param_3)

{
  if (*(int *)(param_2 + 0x14) == 0) {
    SecWriteStringChk(param_1,*(undefined8 *)(param_2 + 8),*(undefined4 *)(param_2 + 0x10),param_3);
  }
  else {
    SecWriteTextAfterWctomb(param_1,param_2,param_3);
  }
  return;
}


// ============================================================================
// Function: SecDecodeState @ 6ba420e1
// ============================================================================

undefined1 SecDecodeState(byte param_1,uint param_2)

{
  return (&stateTable_4116)
         [(int)((param_2 & 0xff) +
                (uint)(byte)(&stateTable_4116)[(int)(uint)param_1] +
                (uint)(byte)(&stateTable_4116)[(int)(uint)param_1] * 8 + 0x100)];
}


// ============================================================================
// Function: SecDecodeFlags @ 6ba42138
// ============================================================================

void SecDecodeFlags(undefined1 param_1,longlong param_2)

{
  switch(param_1) {
  case 0x20:
    *(uint *)(param_2 + 0x1c) = *(uint *)(param_2 + 0x1c) | 2;
    break;
  case 0x23:
    *(uint *)(param_2 + 0x1c) = *(uint *)(param_2 + 0x1c) | 0x80;
    break;
  case 0x2b:
    *(uint *)(param_2 + 0x1c) = *(uint *)(param_2 + 0x1c) | 1;
    break;
  case 0x2d:
    *(uint *)(param_2 + 0x1c) = *(uint *)(param_2 + 0x1c) | 4;
    break;
  case 0x30:
    *(uint *)(param_2 + 0x1c) = *(uint *)(param_2 + 0x1c) | 8;
  }
  return;
}


// ============================================================================
// Function: SecDecodeSize @ 6ba422f6
// ============================================================================

undefined8 SecDecodeSize(undefined1 param_1,longlong param_2,longlong *param_3)

{
  int iVar1;
  
  switch(param_1) {
  case 0x49:
    iVar1 = SecDecodeSizeI(param_2,param_3);
    if (iVar1 != 0) {
      return 0xffffffff;
    }
    break;
  case 0x4c:
  case 0x71:
    *(uint *)(param_2 + 0x1c) = *(uint *)(param_2 + 0x1c) | 0x1400;
    break;
  case 0x68:
    if (*(char *)*param_3 == 'h') {
      *param_3 = *param_3 + 1;
      *(uint *)(param_2 + 0x1c) = *(uint *)(param_2 + 0x1c) | 0x2000;
    }
    else {
      *(uint *)(param_2 + 0x1c) = *(uint *)(param_2 + 0x1c) | 0x20;
    }
    break;
  case 0x6c:
    if (*(char *)*param_3 == 'l') {
      *param_3 = *param_3 + 1;
      *(uint *)(param_2 + 0x1c) = *(uint *)(param_2 + 0x1c) | 0x1000;
    }
    else {
      *(uint *)(param_2 + 0x1c) = *(uint *)(param_2 + 0x1c) | 0x10;
    }
    break;
  case 0x74:
    *(uint *)(param_2 + 0x1c) = *(uint *)(param_2 + 0x1c) | 0x10000;
    break;
  case 0x77:
    *(uint *)(param_2 + 0x1c) = *(uint *)(param_2 + 0x1c) | 0x800;
  }
  return 0;
}


// ============================================================================
// Function: SecDecodeTypeC @ 6ba42445
// ============================================================================

void SecDecodeTypeC(longlong param_1,undefined2 param_2)

{
  *(undefined4 *)(param_1 + 0x10) = 1;
  if ((*(uint *)(param_1 + 0x1c) & 0x810) == 0) {
    *(char *)(param_1 + 0x3a) = (char)param_2;
    *(longlong *)(param_1 + 8) = param_1 + 0x3a;
    *(undefined4 *)(param_1 + 0x14) = 0;
  }
  else {
    *(undefined2 *)(param_1 + 0x3a) = param_2;
    *(longlong *)(param_1 + 8) = param_1 + 0x3a;
    *(undefined4 *)(param_1 + 0x14) = 1;
  }
  return;
}


// ============================================================================
// Function: SecDecodeTypeS @ 6ba425f6
// ============================================================================

void SecDecodeTypeS(longlong param_1,undefined8 param_2)

{
  *(undefined8 *)(param_1 + 8) = param_2;
  if ((*(uint *)(param_1 + 0x1c) & 0x810) == 0) {
    SecDecodeTypeSchar(param_1);
  }
  else {
    SecDecodeTypeSwchar(param_1);
  }
  if (*(int *)(param_1 + 0x10) < 0) {
    *(undefined4 *)(param_1 + 0x10) = 0;
  }
  return;
}


// ============================================================================
// Function: SecOutputOneChar @ 6ba42652
// ============================================================================

void SecOutputOneChar(undefined1 param_1,int *param_2,int *param_3)

{
  *param_2 = *param_2 + -1;
  if (*param_2 < 0) {
    *param_3 = -1;
  }
  else {
    **(undefined1 **)(param_2 + 2) = param_1;
    *(longlong *)(param_2 + 2) = *(longlong *)(param_2 + 2) + 1;
    *param_3 = *param_3 + 1;
  }
  return;
}


// ============================================================================
// Function: SecDecodePrecision @ 6ba426bb
// ============================================================================

undefined8 SecDecodePrecision(byte param_1,longlong param_2)

{
  if (*(int *)(param_2 + 0x2c) == 0) {
    if (0x147ae14 < *(int *)(param_2 + 0x24)) {
      return 0xffffffff;
    }
    *(uint *)(param_2 + 0x24) =
         (*(int *)(param_2 + 0x24) + *(int *)(param_2 + 0x24) * 4) * 2 + (param_1 - 0x30 & 0xff);
  }
  else {
    if (*(int *)(param_2 + 0x24) < 0) {
      *(undefined4 *)(param_2 + 0x24) = 0xffffffff;
    }
    if (0xcccccc8 < *(int *)(param_2 + 0x24)) {
      return 0xffffffff;
    }
  }
  return 0;
}


// ============================================================================
// Function: SecDecodeWidth @ 6ba4274a
// ============================================================================

undefined8 SecDecodeWidth(byte param_1,longlong param_2,int param_3)

{
  if (*(int *)(param_2 + 0x28) == 0) {
    if (param_3 != 3) {
      *(undefined4 *)(param_2 + 0x20) = 0;
    }
    if (0x147ae14 < *(int *)(param_2 + 0x20)) {
      return 0xffffffff;
    }
    *(uint *)(param_2 + 0x20) =
         (*(int *)(param_2 + 0x20) + *(int *)(param_2 + 0x20) * 4) * 2 + (param_1 - 0x30 & 0xff);
  }
  else if (*(int *)(param_2 + 0x20) < 0) {
    *(uint *)(param_2 + 0x1c) = *(uint *)(param_2 + 0x1c) | 4;
    *(int *)(param_2 + 0x20) = -*(int *)(param_2 + 0x20);
    if (0xcccccc8 < *(int *)(param_2 + 0x20)) {
      return 0xffffffff;
    }
  }
  return 0;
}


// ============================================================================
// Function: SecUpdateWcharFlags @ 6ba42808
// ============================================================================

void SecUpdateWcharFlags(longlong param_1)

{
  if ((*(uint *)(param_1 + 0x1c) & 0x830) == 0) {
    *(uint *)(param_1 + 0x1c) = *(uint *)(param_1 + 0x1c) | 0x800;
  }
  return;
}


// ============================================================================
// Function: SecUpdateWstringFlags @ 6ba42835
// ============================================================================

void SecUpdateWstringFlags(undefined8 param_1)

{
  SecUpdateWcharFlags(param_1);
  return;
}


// ============================================================================
// Function: SecUpdateXpxFlags @ 6ba428e2
// ============================================================================

void SecUpdateXpxFlags(longlong *param_1,char param_2)

{
  *param_1 = (longlong)"0123456789abcdefx";
  *(undefined4 *)(param_1 + 3) = 0x10;
  if (param_2 == 'X') {
    *param_1 = (longlong)"0123456789ABCDEFX";
  }
  else if (param_2 == 'p') {
    SecUpdatePointFlags(param_1);
    return;
  }
  if ((*(uint *)((longlong)param_1 + 0x1c) & 0x80) != 0) {
    *(undefined1 *)(param_1 + 7) = 0x30;
    *(undefined1 *)((longlong)param_1 + 0x39) = *(undefined1 *)(*param_1 + 0x10);
    *(undefined4 *)((longlong)param_1 + 0x34) = 2;
  }
  return;
}


// ============================================================================
// Function: SecUpdateOudiFlags @ 6ba4296f
// ============================================================================

void SecUpdateOudiFlags(undefined8 *param_1,char param_2)

{
  if (param_2 != 'i') {
    if ('i' < param_2) {
      if (param_2 == 'o') {
        *(undefined4 *)(param_1 + 3) = 8;
        *param_1 = "0123456789abcdefx";
        if ((*(uint *)((longlong)param_1 + 0x1c) & 0x80) == 0) {
          return;
        }
        *(uint *)((longlong)param_1 + 0x1c) = *(uint *)((longlong)param_1 + 0x1c) | 0x200;
        return;
      }
      if (param_2 != 'u') {
        return;
      }
      goto LAB_6ba429b0;
    }
    if (param_2 != 'd') {
      return;
    }
  }
  *(uint *)((longlong)param_1 + 0x1c) = *(uint *)((longlong)param_1 + 0x1c) | 0x40;
LAB_6ba429b0:
  *(undefined4 *)(param_1 + 3) = 10;
  *param_1 = "0123456789abcdefx";
  return;
}


// ============================================================================
// Function: SecFreeFloatBuffer @ 6ba42a0c
// ============================================================================

void SecFreeFloatBuffer(longlong param_1)

{
  if (*(longlong *)(param_1 + 0x20) != 0) {
    free(*(void **)(param_1 + 0x20));
  }
  if (*(longlong *)(param_1 + 0x18) != 0) {
    free(*(void **)(param_1 + 0x18));
  }
  *(undefined8 *)(param_1 + 0x20) = 0;
  *(undefined8 *)(param_1 + 0x18) = 0;
  *(undefined8 *)(param_1 + 0x10) = 0;
  *(undefined4 *)(param_1 + 0x28) = 0;
  return;
}


// ============================================================================
// Function: SecInitFloatBuffer @ 6ba42bb5
// ============================================================================

undefined8 SecInitFloatBuffer(longlong param_1,undefined8 param_2,longlong param_3)

{
  int iVar1;
  void *pvVar2;
  undefined8 uVar3;
  
  *(undefined8 *)(param_1 + 0x18) = 0;
  *(undefined8 *)(param_1 + 0x10) = 0;
  *(undefined8 *)(param_1 + 0x20) = 0;
  if (*(int *)(param_3 + 0x24) < 0) {
    *(undefined4 *)(param_3 + 0x24) = 6;
  }
  if ((*(uint *)(param_3 + 0x1c) & 0x400) == 0) {
    if (0xccccb6b < *(int *)(param_3 + 0x24)) {
      return 0xffffffff;
    }
    *(int *)(param_1 + 0x28) = *(int *)(param_3 + 0x24) + 0x15f;
  }
  else {
    if (0xcccb95c < *(int *)(param_3 + 0x24)) {
      return 0xffffffff;
    }
    *(int *)(param_1 + 0x28) = *(int *)(param_3 + 0x24) + 0x136e;
  }
  if (*(int *)(param_1 + 0x28) < *(int *)(param_3 + 0x20)) {
    *(int *)(param_1 + 0x28) = *(int *)(param_3 + 0x20) + 2;
  }
  if (*(int *)(param_1 + 0x28) < 0x201) {
    *(longlong *)(param_3 + 8) = param_3 + 0x3a;
  }
  else {
    pvVar2 = malloc((ulonglong)*(uint *)(param_1 + 0x28));
    *(void **)(param_1 + 0x20) = pvVar2;
    if (*(longlong *)(param_1 + 0x20) == 0) {
      return 0xffffffff;
    }
    *(undefined8 *)(param_3 + 8) = *(undefined8 *)(param_1 + 0x20);
  }
  iVar1 = SecInitFloatFmt(param_1,param_2);
  if (iVar1 == 0) {
    uVar3 = 0;
  }
  else {
    if (*(longlong *)(param_1 + 0x20) != 0) {
      free(*(void **)(param_1 + 0x20));
      *(undefined8 *)(param_1 + 0x20) = 0;
    }
    uVar3 = 0xffffffff;
  }
  return uVar3;
}


// ============================================================================
// Function: SecUpdateNegativeChar @ 6ba42d3f
// ============================================================================

longlong SecUpdateNegativeChar(longlong param_1,byte param_2)

{
  longlong local_10;
  
  local_10 = (longlong)(char)param_2;
  if (0x7f < local_10) {
    local_10 = (longlong)(int)((byte)~param_2 + 1);
    *(uint *)(param_1 + 0x1c) = *(uint *)(param_1 + 0x1c) | 0x100;
  }
  return local_10;
}


// ============================================================================
// Function: SecNumberSatisfyPrecision @ 6ba42d93
// ============================================================================

void SecNumberSatisfyPrecision(longlong param_1)

{
  int local_c;
  
  if (*(int *)(param_1 + 0x24) < 0) {
    local_c = 1;
  }
  else {
    *(uint *)(param_1 + 0x1c) = *(uint *)(param_1 + 0x1c) & 0xfffffff7;
    if (0x200 < *(int *)(param_1 + 0x24)) {
      *(undefined4 *)(param_1 + 0x24) = 0x200;
    }
    local_c = *(int *)(param_1 + 0x24);
  }
  while (*(int *)(param_1 + 0x10) < local_c) {
    *(longlong *)(param_1 + 8) = *(longlong *)(param_1 + 8) + -1;
    **(undefined1 **)(param_1 + 8) = 0x30;
    *(int *)(param_1 + 0x10) = *(int *)(param_1 + 0x10) + 1;
  }
  return;
}


// ============================================================================
// Function: SecNumberForceOctal @ 6ba42e2d
// ============================================================================

void SecNumberForceOctal(longlong param_1)

{
  if (((*(uint *)(param_1 + 0x1c) & 0x200) != 0) &&
     ((*(int *)(param_1 + 0x10) == 0 || (**(char **)(param_1 + 8) != '0')))) {
    *(longlong *)(param_1 + 8) = *(longlong *)(param_1 + 8) + -1;
    **(undefined1 **)(param_1 + 8) = 0x30;
    *(int *)(param_1 + 0x10) = *(int *)(param_1 + 0x10) + 1;
  }
  return;
}


// ============================================================================
// Function: SecUpdateSignedNumberPrefix @ 6ba42e91
// ============================================================================

void SecUpdateSignedNumberPrefix(longlong param_1)

{
  if ((*(uint *)(param_1 + 0x1c) & 0x40) != 0) {
    if ((*(uint *)(param_1 + 0x1c) & 0x100) == 0) {
      if ((*(uint *)(param_1 + 0x1c) & 1) == 0) {
        if ((*(uint *)(param_1 + 0x1c) & 2) != 0) {
          *(undefined1 *)(param_1 + 0x38) = 0x20;
          *(undefined4 *)(param_1 + 0x34) = 1;
        }
      }
      else {
        *(undefined1 *)(param_1 + 0x38) = 0x2b;
        *(undefined4 *)(param_1 + 0x34) = 1;
      }
    }
    else {
      *(undefined1 *)(param_1 + 0x38) = 0x2d;
      *(undefined4 *)(param_1 + 0x34) = 1;
    }
  }
  return;
}


// ============================================================================
// Function: SecNumberCompatZero @ 6ba42f17
// ============================================================================

void SecNumberCompatZero(void)

{
  return;
}


// ============================================================================
// Function: MAX @ 6ba6f470
// ============================================================================

int MAX(int param_1,int param_2)

{
  if (param_1 <= param_2) {
    param_1 = param_2;
  }
  return param_1;
}


// ============================================================================
// Function: MIN @ 6ba6f486
// ============================================================================

int MIN(int param_1,int param_2)

{
  if (param_2 <= param_1) {
    param_1 = param_2;
  }
  return param_1;
}


// ============================================================================
// Function: MIN @ 6baa15b0
// ============================================================================

int MIN(int param_1,int param_2)

{
  if (param_2 <= param_1) {
    param_1 = param_2;
  }
  return param_1;
}


// ============================================================================
// Function: UpdateHotZonePosition @ 6baf522e
// ============================================================================

undefined8 UpdateHotZonePosition(byte param_1,undefined8 *param_2)

{
  char cVar1;
  undefined8 uVar2;
  longlong lVar3;
  byte local_2c9;
  undefined8 local_2c8;
  undefined4 auStack_2bc [172];
  byte local_9;
  
  local_9 = *(byte *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x154);
  memset_s(&DAT_6bba3992 + (longlong)(int)(uint)local_9 * 0xe,0xe,0,0xe);
  memset_s(&local_2c8,700,0,700);
  CalculateInternalHotZone(&local_2c8,0x32);
  cVar1 = HasHotZone();
  if (cVar1 == '\0') {
    uVar2 = 1;
  }
  else {
    local_2c9 = 0;
    cVar1 = SearchNearest(param_1,&local_2c8,0x32,&local_2c9);
    if (cVar1 == '\0') {
      uVar2 = 2;
    }
    else {
      lVar3 = (longlong)(int)(uint)local_2c9;
      *param_2 = *(undefined8 *)((longlong)&local_2c8 + lVar3 * 0xe);
      *(undefined4 *)(param_2 + 1) = *(undefined4 *)((longlong)auStack_2bc + (lVar3 * 7 + -2) * 2);
      *(undefined2 *)((longlong)param_2 + 0xc) =
           *(undefined2 *)((longlong)auStack_2bc + lVar3 * 0xe);
      CalculateHotZoneIdx(param_2);
      if ((*(int *)&_refptr_g_tsaPrmtFlash[2].field_0x7b8 == 0) &&
         (cVar1 = HotZoneIsInCore(param_2), cVar1 != '\0')) {
        return 3;
      }
      lVar3 = (longlong)(int)(uint)local_9 * 0xe;
      *(undefined8 *)(&DAT_6bba3992 + lVar3) = *param_2;
      *(undefined4 *)(&DAT_6bba399a + lVar3) = *(undefined4 *)(param_2 + 1);
      *(undefined2 *)(&DAT_6bba399e + lVar3) = *(undefined2 *)((longlong)param_2 + 0xc);
      uVar2 = 0;
    }
  }
  return uVar2;
}


// ============================================================================
// Function: HotZoneAdjustProcessFirstUnit @ 6baf5c13
// ============================================================================

void HotZoneAdjustProcessFirstUnit(byte param_1,undefined2 *param_2)

{
  undefined1 local_78 [32];
  ushort local_58;
  ushort local_56;
  ushort local_54;
  ushort local_52;
  int local_50;
  int local_4c;
  
  CalculateExpandZone(param_2,&local_58);
  CalculateTargetZone(param_2,&local_58,local_78);
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,
                      "\nHotZone Id=%d, Widget:%d#%d#%d#%d, Index:%d#%d, Size:%ddp, ExpandWidget:%d#%d#%d#%d"
                      ,*(undefined1 *)
                        ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x154),*param_2,param_2[1],param_2[2],param_2[3],param_2[4],param_2[5],
                      ((ushort)param_2[6] + 1) * 8,local_50 - (uint)local_58,
                      local_4c - (uint)local_56,local_50 + (uint)local_54,local_4c + (uint)local_52)
    ;
  }
  HotZoneAdjustPoint(param_1,&local_58,local_78);
  return;
}


// ============================================================================
// Function: AdjustByHistory @ 6baf540d
// ============================================================================

void AdjustByHistory(byte param_1,byte param_2)

{
  if ((int)(*(int *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_2 * 0x478 + 0x174) +
           (uint)*(ushort *)
                  ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170)
           ) < 1) {
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x174) =
         -(uint)*(ushort *)
                 ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170);
  }
  else if ((int)((uint)*(ushort *)
                        ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x170) +
                *(int *)((longlong)*_refptr_g_prevTouchesPtr +
                        (longlong)(int)(uint)param_2 * 0x478 + 0x174)) <
           (int)(*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x20) - 1)) {
    *(undefined4 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x174) =
         *(undefined4 *)
          ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x174);
  }
  else {
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x174) =
         (*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x20) - 1) -
         (uint)*(ushort *)
                ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170);
  }
  *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170) =
       *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170)
       + (short)*(undefined4 *)
                 ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x174);
  if ((int)(*(int *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_2 * 0x478 + 0x178) +
           (uint)*(ushort *)
                  ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172)
           ) < 1) {
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x178) =
         -(uint)*(ushort *)
                 ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
  }
  else if ((int)((uint)*(ushort *)
                        ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x172) +
                *(int *)((longlong)*_refptr_g_prevTouchesPtr +
                        (longlong)(int)(uint)param_2 * 0x478 + 0x178)) <
           (int)(*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x22) - 1)) {
    *(undefined4 *)
     ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x178) =
         *(undefined4 *)
          ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)param_2 * 0x478 + 0x178);
  }
  else {
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x178) =
         (*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x22) - 1) -
         (uint)*(ushort *)
                ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
  }
  *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172) =
       *(short *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172)
       + (short)*(undefined4 *)
                 ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x178);
  return;
}


// ============================================================================
// Function: ClearDiffNum @ 6baf6268
// ============================================================================

void ClearDiffNum(longlong param_1)

{
  byte local_9;
  
  for (local_9 = 0; local_9 < 6; local_9 = local_9 + 1) {
    *(undefined4 *)((longlong)(int)(uint)local_9 * 0x18 + param_1 + 0x65c) = 0;
    *(undefined4 *)((longlong)(int)(uint)local_9 * 0x18 + param_1 + 0x660) = 0;
    *(undefined4 *)((longlong)(int)(uint)local_9 * 0x18 + param_1 + 0x664) = 0;
  }
  return;
}


// ============================================================================
// Function: GenerateDetailDescription @ 6baf6a1d
// ============================================================================

void GenerateDetailDescription(void)

{
  byte local_19;
  
  ClearDetailDescription();
  StrPrintf_Backup();
  StrPrintf_Init(DetailDescriptionPutStr);
  for (local_19 = 0; local_19 < 6; local_19 = local_19 + 1) {
    if (*(int *)(&DAT_6bba3bdc + (longlong)(int)(uint)local_19 * 0x18) == 0) {
      StrPrintf_Process(0,"\nHotZone %s total=%d, origin=%d, optimize=%d",
                        (&g_appList)[(int)(uint)local_19],
                        *(undefined4 *)(&DAT_6bba3bd0 + (longlong)(int)(uint)local_19 * 0x18),
                        *(undefined4 *)(&DAT_6bba3bd4 + (longlong)(int)(uint)local_19 * 0x18),
                        *(undefined4 *)(&DAT_6bba3bd8 + (longlong)(int)(uint)local_19 * 0x18));
    }
    else {
      StrPrintf_Process(0,"\nHotZone %s total=%d(+%d), origin=%d(+%d), optimize=%d(+%d)",
                        (&g_appList)[(int)(uint)local_19],
                        *(undefined4 *)(&DAT_6bba3bd0 + (longlong)(int)(uint)local_19 * 0x18),
                        *(undefined4 *)(&DAT_6bba3bdc + (longlong)(int)(uint)local_19 * 0x18),
                        *(undefined4 *)(&DAT_6bba3bd4 + (longlong)(int)(uint)local_19 * 0x18),
                        *(undefined4 *)(&DAT_6bba3be0 + (longlong)(int)(uint)local_19 * 0x18),
                        *(undefined4 *)(&DAT_6bba3bd8 + (longlong)(int)(uint)local_19 * 0x18),
                        *(undefined4 *)(&DAT_6bba3be4 + (longlong)(int)(uint)local_19 * 0x18));
    }
  }
  StrPrintf_Retore();
  return;
}


// ============================================================================
// Function: HotZoneIsInCore @ 6baf51b8
// ============================================================================

undefined8 HotZoneIsInCore(longlong param_1)

{
  undefined8 uVar1;
  
  if ((((*(short *)(param_1 + 8) == 0) ||
       ((int)((byte)_refptr_g_tsaPrmtFlash[2].field_0x7bd - 1) <=
        (int)(uint)*(ushort *)(param_1 + 8))) || (*(short *)(param_1 + 10) == 0)) ||
     ((int)((byte)_refptr_g_tsaPrmtFlash[2].field_0x7be - 1) <= (int)(uint)*(ushort *)(param_1 + 10)
     )) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: CalculateAppIndex @ 6baf61ed
// ============================================================================

void CalculateAppIndex(longlong param_1)

{
  int iVar1;
  byte local_9;
  
  *(undefined1 *)(param_1 + 0x410) = 6;
  for (local_9 = 1; local_9 < 6; local_9 = local_9 + 1) {
    if (*(longlong *)(param_1 + 0x400) != 0) {
      iVar1 = strcmp(*(char **)(param_1 + 0x400),(&g_appList)[(int)(uint)local_9]);
      if (iVar1 == 0) {
        *(byte *)(param_1 + 0x410) = local_9;
      }
    }
  }
  return;
}


// ============================================================================
// Function: CalculateSuccessNum @ 6baf6380
// ============================================================================

void CalculateSuccessNum(byte param_1,longlong param_2)

{
  byte bVar1;
  char cVar2;
  longlong lVar3;
  
  bVar1 = *(byte *)(param_2 + 0x410);
  lVar3 = param_2 + (longlong)
                    (int)(uint)*(byte *)((longlong)*_refptr_g_prevTouchesPtr +
                                        (longlong)(int)(uint)param_1 * 0x478 + 0x154) * 0xe + 0x412;
  *(int *)(param_2 + 0x650) = *(int *)(param_2 + 0x650) + 1;
  *(int *)(param_2 + 0x65c) = *(int *)(param_2 + 0x65c) + 1;
  if (bVar1 < 6) {
    *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x650) =
         *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x650) + 1;
    *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x65c) =
         *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x65c) + 1;
  }
  cVar2 = IsInHotZone(*(undefined2 *)
                       ((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_1 * 0x478 + 0x16c),
                      *(undefined2 *)
                       ((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_1 * 0x478 + 0x16e),lVar3);
  if ((cVar2 == '\0') ||
     (cVar2 = IsInHotZone(*(undefined2 *)
                           ((longlong)*_refptr_g_prevTouchesPtr +
                           (longlong)(int)(uint)param_1 * 0x478 + 0x4ac),
                          *(undefined2 *)
                           ((longlong)*_refptr_g_prevTouchesPtr +
                           (longlong)(int)(uint)param_1 * 0x478 + 0x4ae),lVar3), cVar2 == '\0')) {
    if (((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                   (longlong)(int)(uint)param_1 * 0x478 + 0x200) & 8) != 0) &&
       (cVar2 = IsInHotZone(*(undefined2 *)
                             ((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)param_1 * 0x478 + 0x170),
                            *(undefined2 *)
                             ((longlong)*_refptr_g_prevTouchesPtr +
                             (longlong)(int)(uint)param_1 * 0x478 + 0x172),lVar3), cVar2 != '\0')) {
      *(int *)(param_2 + 0x658) = *(int *)(param_2 + 0x658) + 1;
      *(int *)(param_2 + 0x664) = *(int *)(param_2 + 0x664) + 1;
      if (bVar1 < 6) {
        *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x658) =
             *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x658) + 1;
        *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x664) =
             *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x664) + 1;
      }
    }
  }
  else {
    *(int *)(param_2 + 0x654) = *(int *)(param_2 + 0x654) + 1;
    *(int *)(param_2 + 0x660) = *(int *)(param_2 + 0x660) + 1;
    *(int *)(param_2 + 0x658) = *(int *)(param_2 + 0x658) + 1;
    *(int *)(param_2 + 0x664) = *(int *)(param_2 + 0x664) + 1;
    if (bVar1 < 6) {
      *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x654) =
           *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x654) + 1;
      *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x660) =
           *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x660) + 1;
      *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x658) =
           *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x658) + 1;
      *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x664) =
           *(int *)((longlong)(int)(uint)bVar1 * 0x18 + param_2 + 0x664) + 1;
    }
  }
  return;
}


// ============================================================================
// Function: HotZoneAdjustClearAnalyzer @ 6baf3fb1
// ============================================================================

void HotZoneAdjustClearAnalyzer(void)

{
  memset_s(&g_hotZoneAnalyzer,0x6e0,0,0x6e0);
  return;
}


// ============================================================================
// Function: SinXDivX @ 6ba8573e
// ============================================================================

double SinXDivX(double param_1)

{
  double dVar1;
  double local_res8;
  
  local_res8 = param_1;
  if (param_1 < 0.0) {
    local_res8 = -param_1;
  }
  dVar1 = local_res8 * local_res8;
  if (1.0 < local_res8) {
    if (2.0 < local_res8) {
      dVar1 = 0.0;
    }
    else {
      dVar1 = (((double)DAT_6bb52ecc * dVar1 * local_res8 -
               (double)(DAT_6bb52ecc * DAT_6bb52ee8) * dVar1) +
              (double)(DAT_6bb52ecc * DAT_6bb52eec) * local_res8) -
              (double)(DAT_6bb52ecc * DAT_6bb52ef0);
    }
  }
  else {
    dVar1 = ((double)(DAT_6bb52ecc + DAT_6bb52ee0) * dVar1 * local_res8 -
            (double)(DAT_6bb52ecc + DAT_6bb52ee4) * dVar1) + 1.0;
  }
  return dVar1;
}


// ============================================================================
// Function: malloc @ EXTERNAL:0000004e
// ============================================================================
// DECOMPILATION FAILED OR TIMED OUT
// ============================================================================
// Function: free @ EXTERNAL:00000046
// ============================================================================
// DECOMPILATION FAILED OR TIMED OUT
// ============================================================================
// Function: SecDoCat @ 6ba45a50
// ============================================================================

undefined8 SecDoCat(char *param_1,ulonglong param_2,char *param_3)

{
  undefined8 uVar1;
  ulonglong uVar2;
  longlong lVar3;
  longlong local_28;
  char *local_20;
  ulonglong local_18;
  char *local_10;
  
  local_18 = param_2;
  for (local_10 = param_1; (local_18 != 0 && (*local_10 != '\0')); local_10 = local_10 + 1) {
    local_18 = local_18 - 1;
  }
  uVar2 = (longlong)local_10 - (longlong)param_1;
  local_28 = param_2 - uVar2;
  for (local_20 = param_3; (local_28 != 0 && (*local_20 != '\0')); local_20 = local_20 + 1) {
    local_28 = local_28 + -1;
  }
  lVar3 = (longlong)local_20 - (longlong)param_3;
  if (((param_1 < param_3) && (param_3 <= param_1 + uVar2 + lVar3)) ||
     ((param_3 < param_1 && (param_1 <= param_3 + lVar3)))) {
    *param_1 = '\0';
    if ((param_3 < param_1 + uVar2) || (uVar2 != param_2)) {
      uVar1 = 0xb6;
    }
    else {
      uVar1 = 0x96;
    }
  }
  else if ((uVar2 + lVar3 < param_2) && (param_1 != param_3)) {
    memcpy(param_1 + uVar2,param_3,lVar3 + 1);
    uVar1 = 0;
  }
  else {
    *param_1 = '\0';
    if (uVar2 == param_2) {
      uVar1 = 0x96;
    }
    else {
      uVar1 = 0xa2;
    }
  }
  return uVar1;
}


// ============================================================================
// Function: _errno @ 6bb4e628
// ============================================================================

int * __cdecl _errno(void)

{
  int *piVar1;
  
                    /* WARNING: Could not recover jumptable at 0x6bb4e628. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  piVar1 = _errno();
  return piVar1;
}


// ============================================================================
// Function: .text @ 6bb4e700
// ============================================================================

void _text(undefined4 param_1,undefined8 param_2,undefined8 param_3,undefined8 param_4,
          undefined8 param_5)

{
  undefined4 local_38 [2];
  undefined8 local_30;
  undefined8 local_28;
  undefined8 local_20;
  undefined8 local_18;
  
  if (stUserMathErr != (code *)0x0) {
    local_18 = param_5;
    local_38[0] = param_1;
    local_30 = param_2;
    local_28 = param_3;
    local_20 = param_4;
    (*stUserMathErr)(local_38);
  }
  return;
}


// ============================================================================
// Function: OemSubDataRegister @ 6baa7170
// ============================================================================

void OemSubDataRegister(uint param_1,byte param_2,undefined8 param_3,undefined4 param_4)

{
  *(undefined8 *)(&DAT_6bb86308 + (longlong)(int)(uint)param_2 * 0x10 + (ulonglong)param_1 * 0xa8) =
       param_3;
  *(undefined4 *)(&DAT_6bb86310 + (longlong)(int)(uint)param_2 * 0x10 + (ulonglong)param_1 * 0xa8) =
       param_4;
  *(int *)(&g_oemDataCfg + (ulonglong)param_1 * 0xa8) =
       *(int *)(&g_oemDataCfg + (ulonglong)param_1 * 0xa8) +
       *(int *)(&DAT_6bb86310 + (longlong)(int)(uint)param_2 * 0x10 + (ulonglong)param_1 * 0xa8);
  return;
}


// ============================================================================
// Function: CoorInit @ 6baac528
// ============================================================================

void CoorInit(void)

{
  ushort local_a;
  
                    /* 0x6c528  518  CoorInit */
  g_coorIIRCoef = 0x10;
  for (local_a = 0; local_a < 0x18; local_a = local_a + 1) {
    *(undefined4 *)_refptr_g_coorBuf = 0x7fffffff;
    *(undefined4 *)(_refptr_g_coorBuf + 4) = 0x7fffffff;
    *(undefined4 *)(_refptr_g_coorBuf + (longlong)(int)(uint)local_a * 4 + 8) = 0x7fffffff;
    *(undefined4 *)(_refptr_g_coorBuf + ((longlong)(int)(uint)local_a + 0x18) * 4 + 8) = 0x7fffffff;
    *(undefined4 *)(_refptr_g_coorBuf + ((longlong)(int)(uint)local_a + 0x30) * 4 + 8) = 0x7fffffff;
    *(undefined4 *)(_refptr_g_coorBuf + ((longlong)(int)(uint)local_a + 0x48) * 4 + 8) = 0x7fffffff;
    *(undefined4 *)(_refptr_g_coorBuf + ((longlong)(int)(uint)local_a + 0x60) * 4 + 8) = 0x7fffffff;
    *(undefined4 *)(_refptr_g_coorBuf + ((longlong)(int)(uint)local_a + 0x78) * 4 + 8) = 0x7fffffff;
    *(undefined4 *)(_refptr_g_coorBuf + ((longlong)(int)(uint)local_a + 0x90) * 4 + 8) = 0x7fffffff;
    *(undefined4 *)(_refptr_g_coorBuf + ((longlong)(int)(uint)local_a + 0xa8) * 4 + 8) = 0x7fffffff;
    *(undefined4 *)(_refptr_g_coorBuf + ((longlong)(int)(uint)local_a + 0xc0) * 4 + 8) = 0x7fffffff;
  }
  *(undefined4 *)(_refptr_g_coorBuf + 0x378) = 0;
  *(undefined4 *)(_refptr_g_coorBuf + 0x37c) = 0;
  *(undefined4 *)(_refptr_g_coorBuf + 0x380) = 0;
  CompensationByPRInit();
  return;
}


// ============================================================================
// Function: ASAStaticCounterClean @ 6bac8414
// ============================================================================

void ASAStaticCounterClean(void)

{
                    /* 0x88414  110  ASAStaticCounterClean */
  g_asaStatic.wActiveStylusFrameCount = 0;
  g_asaStatic.wF1HistoryFrameCount = 0;
  g_asaStatic.wF2HistoryFrameCount = 0;
  g_asaStatic._6_2_ = 0;
  g_asaStatic._8_2_ = 0;
  g_asaStatic._10_2_ = 0;
  g_asaStatic._12_2_ = 0;
  g_asaStatic._14_2_ = 0;
  return;
}


// ============================================================================
// Function: ExitInRangeMode @ 6bac8726
// ============================================================================

void ExitInRangeMode(void)

{
                    /* 0x88726  587  ExitInRangeMode */
  g_asaStatic._6_2_ = 0;
  g_asaStatic._12_2_ = 0;
  g_asaStatic._8_2_ = 0;
  g_asaStatic._10_2_ = 0;
  g_asaStatic._72_2_ = _refptr_g_asaPrmt[1].wField00;
  if (g_asaStatic._48_1_ != '\0') {
    g_asaStatic._72_2_ = _refptr_g_asaPrmt[1].wField00 * 2;
  }
  CoorInit();
  ASAPropertyInit();
  return;
}


// ============================================================================
// Function: ASAStaticPostProcess @ 6bac8a04
// ============================================================================

void ASAStaticPostProcess(void)

{
                    /* 0x88a04  112  ASAStaticPostProcess */
  g_asaStatic._88_4_ = g_asaStatic.dwHpp3FeatureFlags;
  g_asaStatic.dwPrevAsaStatusFlags = g_asaStatic.dwAsaStatusFlags;
  return;
}


// ============================================================================
// Function: SecFormatFloat @ 6ba417d0
// ============================================================================

int SecFormatFloat(char *param_1,char *param_2,undefined8 param_3,undefined8 param_4)

{
  int iVar1;
  undefined8 local_res18;
  undefined8 local_res20;
  
  local_res18 = param_3;
  local_res20 = param_4;
  iVar1 = vsprintf(param_1,param_2,(va_list)&local_res18);
  return iVar1;
}


// ============================================================================
// Function: SecNumberToString @ 6ba41b37
// ============================================================================

void SecNumberToString(undefined8 param_1,undefined8 param_2)

{
  SecNumber64ToString(param_1,param_2);
  return;
}


// ============================================================================
// Function: SecIsNumberNeedTo32Bit @ 6ba41b5e
// ============================================================================

undefined8 SecIsNumberNeedTo32Bit(longlong param_1)

{
  undefined8 uVar1;
  
  if (((((*(uint *)(param_1 + 0x1c) & 0x8000) == 0) && ((*(uint *)(param_1 + 0x1c) & 0x10000) == 0))
      && ((*(uint *)(param_1 + 0x1c) & 0x20000) == 0)) &&
     ((*(uint *)(param_1 + 0x1c) & 0x1000) == 0)) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: SecWriteMultiChar @ 6ba43734
// ============================================================================

void SecWriteMultiChar(undefined1 param_1,int param_2,int *param_3,int *param_4)

{
  int local_c;
  
  local_c = param_2;
  while( true ) {
    if (local_c < 1) {
      return;
    }
    *param_3 = *param_3 + -1;
    if (*param_3 < 0) break;
    **(undefined1 **)(param_3 + 2) = param_1;
    *(longlong *)(param_3 + 2) = *(longlong *)(param_3 + 2) + 1;
    *param_4 = *param_4 + 1;
    local_c = local_c + -1;
  }
  *param_4 = -1;
  return;
}


// ============================================================================
// Function: SecWriteStringToStreamOpt @ 6ba41c83
// ============================================================================

void SecWriteStringToStreamOpt(int *param_1,undefined1 *param_2,int param_3)

{
  undefined1 *local_18;
  int local_c;
  
  local_18 = param_2;
  for (local_c = 0; local_c < param_3; local_c = local_c + 1) {
    **(undefined1 **)(param_1 + 2) = *local_18;
    *(longlong *)(param_1 + 2) = *(longlong *)(param_1 + 2) + 1;
    local_18 = local_18 + 1;
  }
  *param_1 = *param_1 - param_3;
  return;
}


// ============================================================================
// Function: SecIsStreamBufEnough @ 6ba41d7a
// ============================================================================

uint SecIsStreamBufEnough(int *param_1,int param_2)

{
  return (uint)~(*param_1 - param_2) >> 0x1f;
}


// ============================================================================
// Function: SecWriteString @ 6ba437ba
// ============================================================================

void SecWriteString(undefined1 *param_1,int param_2,int *param_3,int *param_4)

{
  int local_14;
  undefined1 *local_10;
  
  local_14 = param_2;
  local_10 = param_1;
  while( true ) {
    if (local_14 < 1) {
      *param_4 = ((int)local_10 - (int)param_1) + *param_4;
      return;
    }
    *param_3 = *param_3 + -1;
    if (*param_3 < 0) break;
    **(undefined1 **)(param_3 + 2) = *local_10;
    *(longlong *)(param_3 + 2) = *(longlong *)(param_3 + 2) + 1;
    local_10 = local_10 + 1;
    local_14 = local_14 + -1;
  }
  *param_4 = -1;
  return;
}


// ============================================================================
// Function: SecWriteStringChk @ 6ba41f38
// ============================================================================

void SecWriteStringChk(undefined8 param_1,undefined8 param_2,int param_3,int *param_4)

{
  int iVar1;
  
  iVar1 = SecIsStreamBufEnough(param_1,param_3);
  if (iVar1 == 0) {
    SecWriteString(param_2,param_3,param_1,param_4);
  }
  else {
    SecWriteStringToStream(param_1,param_2,param_3);
    *param_4 = *param_4 + param_3;
  }
  return;
}


// ============================================================================
// Function: SecWriteTextAfterWctomb @ 6ba41fac
// ============================================================================

void SecWriteTextAfterWctomb(undefined8 param_1,longlong param_2,int *param_3)

{
  char local_38 [32];
  int local_18;
  int local_14;
  wchar_t *local_10;
  
  local_10 = *(wchar_t **)(param_2 + 8);
  local_14 = *(int *)(param_2 + 0x10);
  while( true ) {
    if (local_14 < 1) {
      return;
    }
    local_18 = wctomb(local_38,*local_10);
    if (local_18 < 1) break;
    SecWriteString(local_38,local_18,param_1,param_3);
    if (*param_3 == -1) {
      return;
    }
    local_14 = local_14 + -1;
    local_10 = local_10 + 1;
  }
  *param_3 = -1;
  return;
}


// ============================================================================
// Function: SecDecodeSizeI @ 6ba421e4
// ============================================================================

undefined8 SecDecodeSizeI(longlong param_1,longlong *param_2)

{
  *(uint *)(param_1 + 0x1c) = *(uint *)(param_1 + 0x1c) | 0x8000;
  if ((*(char *)*param_2 == '6') && (*(char *)(*param_2 + 1) == '4')) {
    *param_2 = *param_2 + 2;
    *(uint *)(param_1 + 0x1c) = *(uint *)(param_1 + 0x1c) | 0x8000;
  }
  else if ((*(char *)*param_2 == '3') && (*(char *)(*param_2 + 1) == '2')) {
    *param_2 = *param_2 + 2;
    *(uint *)(param_1 + 0x1c) = *(uint *)(param_1 + 0x1c) & 0xffff7fff;
  }
  else if ((((*(char *)*param_2 != 'd') && (*(char *)*param_2 != 'i')) && (*(char *)*param_2 != 'o')
           ) && (((*(char *)*param_2 != 'u' && (*(char *)*param_2 != 'x')) &&
                 (*(char *)*param_2 != 'X')))) {
    return 0xffffffff;
  }
  return 0;
}


// ============================================================================
// Function: SecDecodeTypeSchar @ 6ba424be
// ============================================================================

void SecDecodeTypeSchar(longlong param_1)

{
  size_t sVar1;
  ulonglong local_18;
  char *local_10;
  
  if (*(longlong *)(param_1 + 8) == 0) {
    *(undefined1 **)(param_1 + 8) = &DAT_6bb4f000;
  }
  if (*(int *)(param_1 + 0x24) == -1) {
    sVar1 = strlen(*(char **)(param_1 + 8));
    *(int *)(param_1 + 0x10) = (int)sVar1;
  }
  else {
    local_18 = (ulonglong)*(uint *)(param_1 + 0x24);
    for (local_10 = *(char **)(param_1 + 8); (local_18 != 0 && (*local_10 != '\0'));
        local_10 = local_10 + 1) {
      local_18 = local_18 - 1;
    }
    *(int *)(param_1 + 0x10) = (int)local_10 - (int)*(undefined8 *)(param_1 + 8);
  }
  return;
}


// ============================================================================
// Function: SecDecodeTypeSwchar @ 6ba4256d
// ============================================================================

void SecDecodeTypeSwchar(longlong param_1)

{
  ulonglong local_18;
  short *local_10;
  
  *(undefined4 *)(param_1 + 0x14) = 1;
  if (*(longlong *)(param_1 + 8) == 0) {
    *(undefined2 **)(param_1 + 8) = &wStrNullString_4165;
  }
  local_10 = *(short **)(param_1 + 8);
  local_18 = 0;
  for (; (local_18 < *(uint *)(param_1 + 0x24) && (*local_10 != 0)); local_10 = local_10 + 1) {
    local_18 = local_18 + 1;
  }
  *(int *)(param_1 + 0x10) = (int)local_18;
  return;
}


// ============================================================================
// Function: SecUpdatePointFlags @ 6ba42851
// ============================================================================

void SecUpdatePointFlags(longlong *param_1)

{
  *(uint *)((longlong)param_1 + 0x1c) = *(uint *)((longlong)param_1 + 0x1c) | 0x4000;
  *(undefined4 *)((longlong)param_1 + 0x24) = 0x10;
  *param_1 = (longlong)"0123456789ABCDEFX";
  *(uint *)((longlong)param_1 + 0x1c) = *(uint *)((longlong)param_1 + 0x1c) & 0xfffffff7;
  *(uint *)((longlong)param_1 + 0x1c) = *(uint *)((longlong)param_1 + 0x1c) | 0x8000;
  if ((*(uint *)((longlong)param_1 + 0x1c) & 0x80) != 0) {
    *(undefined1 *)(param_1 + 7) = 0x30;
    *(undefined1 *)((longlong)param_1 + 0x39) = *(undefined1 *)(*param_1 + 0x10);
    *(undefined4 *)((longlong)param_1 + 0x34) = 2;
  }
  return;
}


// ============================================================================
// Function: SecInitFloatFmt @ 6ba42ac1
// ============================================================================

undefined8 SecInitFloatFmt(longlong param_1,longlong param_2)

{
  void *pvVar1;
  longlong local_18;
  uint local_10;
  int local_c;
  
  local_18 = param_2 + -2;
  SecSeekToFrontPercent(&local_18);
  local_10 = ((int)param_2 - (int)local_18) + 1;
  if ((int)local_10 < 0x11) {
    *(longlong *)(param_1 + 0x10) = param_1;
    *(undefined8 *)(param_1 + 0x18) = 0;
  }
  else {
    pvVar1 = malloc((ulonglong)local_10);
    *(void **)(param_1 + 0x18) = pvVar1;
    if (*(longlong *)(param_1 + 0x18) == 0) {
      return 0xffffffff;
    }
    *(undefined8 *)(param_1 + 0x10) = *(undefined8 *)(param_1 + 0x18);
  }
  for (local_c = 0; local_c < (int)(local_10 - 1); local_c = local_c + 1) {
    *(undefined1 *)(*(longlong *)(param_1 + 0x10) + (longlong)local_c) =
         *(undefined1 *)(local_c + local_18);
  }
  *(undefined1 *)(*(longlong *)(param_1 + 0x10) + (longlong)(int)local_10 + -1) = 0;
  return 0;
}


// ============================================================================
// Function: HasHotZone @ 6baf4008
// ============================================================================

undefined8 HasHotZone(void)

{
  undefined8 uVar1;
  
  if ((((g_hotZoneRect == 0) && (DAT_6bba3c84 == 0)) && (DAT_6bba3c82 == 0)) && (DAT_6bba3c86 == 0))
  {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: CalculateInternalHotZone @ 6baf4f8a
// ============================================================================

void CalculateInternalHotZone(undefined8 param_1,undefined1 param_2)

{
  ExtToInt(param_1,param_2,&g_hotZoneRect,0x32);
  return;
}


// ============================================================================
// Function: SearchNearest @ 6baf4fbe
// ============================================================================

bool SearchNearest(byte param_1,undefined8 param_2,undefined1 param_3,undefined8 param_4)

{
  ushort uVar1;
  ushort local_12;
  ushort local_10 [2];
  undefined2 local_c;
  undefined2 local_a;
  
  local_a = *(undefined2 *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170);
  local_c = *(undefined2 *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
  local_10[0] = 0xffff;
  local_12 = 0xffff;
  SearchNearestDisTwo(local_a,local_c,param_2,param_3,local_10,&local_12,param_4);
  uVar1 = TSAOut_ExtToIntDist(_refptr_g_tsaPrmtFlash[2].field_0x7bc);
  return (int)(uint)uVar1 < (int)((uint)local_12 - (uint)local_10[0]);
}


// ============================================================================
// Function: CalculateHotZoneIdx @ 6baf50ad
// ============================================================================

void CalculateHotZoneIdx(ushort *param_1)

{
  ushort uVar1;
  
  param_1[4] = (ushort)((ulonglong)
                        (ushort)((short)((int)((uint)param_1[2] - (uint)*param_1) / 2) + *param_1) /
                       (ulonglong)
                       (longlong)
                       (int)((ulonglong)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x20) /
                            (ulonglong)
                            (longlong)(int)(uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x7bd));
  param_1[5] = (ushort)((ulonglong)
                        (ushort)((short)((int)((uint)param_1[3] - (uint)param_1[1]) / 2) +
                                param_1[1]) /
                       (ulonglong)
                       (longlong)
                       (int)((ulonglong)*(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x22) /
                            (ulonglong)
                            (longlong)(int)(uint)(byte)_refptr_g_tsaPrmtFlash[2].field_0x7be));
  uVar1 = CalculateSizeIndex(param_1);
  param_1[6] = uVar1;
  return;
}


// ============================================================================
// Function: CalculateExpandZone @ 6baf480c
// ============================================================================

void CalculateExpandZone(short *param_1,ushort *param_2)

{
  byte bVar1;
  short sVar2;
  short sVar3;
  ushort uVar4;
  ushort uVar5;
  ushort uVar6;
  ushort uVar7;
  ushort uVar8;
  ushort uVar9;
  ushort uVar10;
  ushort uVar11;
  ushort uVar12;
  ushort uVar13;
  ushort uVar14;
  ushort uVar15;
  ushort uVar16;
  ushort uVar17;
  
  uVar4 = (ushort)(param_1[2] - *param_1) >> 1;
  uVar5 = (ushort)(param_1[3] - param_1[1]) >> 1;
  sVar2 = *param_1;
  sVar3 = param_1[1];
  uVar16 = param_1[4];
  uVar17 = param_1[5];
  uVar15 = param_1[6];
  uVar6 = TSAOut_ExtToIntDist(*(undefined2 *)
                               (&_refptr_g_tsaPrmtFlash[2].field_0x7c0 +
                               (longlong)(int)(uint)uVar15 * 2));
  bVar1 = _refptr_g_tsaPrmtFlash[2].field_0x7bf;
  uVar7 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x7d4);
  uVar8 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x7d6);
  uVar9 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x7d8);
  uVar10 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x7da);
  uVar11 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x7dc);
  uVar12 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x7de);
  uVar13 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x7e0);
  uVar14 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x7e2);
  uVar11 = uVar11 - (short)((int)((uint)uVar15 * ((uint)uVar11 - (uint)uVar7)) / (int)(bVar1 - 1));
  uVar12 = uVar12 - (short)((int)((uint)uVar15 * ((uint)uVar12 - (uint)uVar8)) / (int)(bVar1 - 1));
  uVar13 = uVar13 - (short)((int)((uint)uVar15 * ((uint)uVar13 - (uint)uVar9)) / (int)(bVar1 - 1));
  uVar14 = uVar14 - (short)((int)((uint)uVar15 * ((uint)uVar14 - (uint)uVar10)) / (int)(bVar1 - 1));
  uVar15 = uVar11 + (short)((int)((uint)uVar16 * ((uint)uVar12 - (uint)uVar11)) /
                           (int)((byte)_refptr_g_tsaPrmtFlash[2].field_0x7bd - 1)) + (uVar6 >> 1);
  uVar16 = (uVar12 - (short)((int)((uint)uVar16 * ((uint)uVar12 - (uint)uVar11)) /
                            (int)((byte)_refptr_g_tsaPrmtFlash[2].field_0x7bd - 1))) + (uVar6 >> 1);
  uVar7 = uVar13 + (short)((int)((uint)uVar17 * ((uint)uVar14 - (uint)uVar13)) /
                          (int)((byte)_refptr_g_tsaPrmtFlash[2].field_0x7be - 1)) + (uVar6 >> 1);
  uVar17 = (uVar14 - (short)((int)((uint)uVar17 * ((uint)uVar14 - (uint)uVar13)) /
                            (int)((byte)_refptr_g_tsaPrmtFlash[2].field_0x7be - 1))) + (uVar6 >> 1);
  if (uVar15 <= uVar4) {
    uVar15 = uVar4;
  }
  *param_2 = uVar15;
  if (uVar16 <= uVar4) {
    uVar16 = uVar4;
  }
  param_2[2] = uVar16;
  if (uVar7 <= uVar5) {
    uVar7 = uVar5;
  }
  param_2[1] = uVar7;
  if (uVar17 <= uVar5) {
    uVar17 = uVar5;
  }
  param_2[3] = uVar17;
  param_2[8] = *param_2 + param_2[2];
  param_2[9] = param_2[1] + param_2[3];
  *(uint *)(param_2 + 4) =
       ((uint)(ushort)(uVar4 + sVar2) + (uint)(param_2[8] >> 1)) - (uint)*param_2;
  *(uint *)(param_2 + 6) =
       ((uint)(ushort)(uVar5 + sVar3) + (uint)(param_2[9] >> 1)) - (uint)param_2[1];
  return;
}


// ============================================================================
// Function: CalculateTargetZone @ 6baf4d02
// ============================================================================

void CalculateTargetZone(ushort *param_1,longlong param_2,longlong param_3)

{
  ushort uVar1;
  ushort uVar2;
  ushort uVar3;
  ushort uVar4;
  
  uVar1 = param_1[2];
  uVar2 = *param_1;
  uVar3 = param_1[3];
  uVar4 = param_1[1];
  CalculateTargetZoneLen
            (*(undefined2 *)(param_2 + 0x10),*(undefined2 *)(param_2 + 0x12),uVar1 - uVar2,
             uVar3 - uVar4,param_3 + 0x10,param_3 + 0x12);
  *(uint *)(param_3 + 8) = (uint)*param_1 + (uint)((ushort)(uVar1 - uVar2) >> 1);
  *(uint *)(param_3 + 0xc) = (uint)param_1[1] + (uint)((ushort)(uVar3 - uVar4) >> 1);
  return;
}


// ============================================================================
// Function: HotZoneAdjustPoint @ 6baf5859
// ============================================================================

void HotZoneAdjustPoint(byte param_1,undefined8 param_2,undefined8 param_3)

{
  char cVar1;
  undefined2 uVar2;
  ushort uVar3;
  ushort uVar4;
  undefined2 uVar5;
  short sVar6;
  short sVar7;
  ushort *puVar8;
  undefined4 uVar10;
  undefined8 uVar9;
  ushort *puVar11;
  undefined4 uVar12;
  ushort local_30;
  ushort local_2e;
  ushort local_2c;
  ushort local_2a;
  
  local_2a = *(ushort *)
              ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170);
  local_2c = *(ushort *)
              ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
  cVar1 = IsInAdjustArea(local_2a,local_2c,param_2);
  if (cVar1 == '\0') {
    StrPrintf_Process(0,"\nAppLog: ");
    StrPrintf_Process(0x10,"\nHotZone Id=%d, Not in the Expand Zone",
                      *(undefined1 *)
                       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x154));
  }
  else {
    puVar11 = &local_30;
    puVar8 = &local_2e;
    CalculateNewPosition(param_2,local_2a,local_2c,param_3,puVar8,puVar11);
    uVar10 = (undefined4)((ulonglong)puVar8 >> 0x20);
    uVar12 = (undefined4)((ulonglong)puVar11 >> 0x20);
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x174) =
         (uint)local_2e - (uint)local_2a;
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x178) =
         (uint)local_30 - (uint)local_2c;
    *(ushort *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170) =
         local_2e;
    *(ushort *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172) =
         local_30;
    *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x200) =
         *(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x200)
         | 8;
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      uVar2 = TSAOut_Dim2PosInternal
                        (*(undefined2 *)
                          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x172),1);
      uVar3 = TSAOut_Dim1PosInternal
                        (*(undefined2 *)
                          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x170),1);
      uVar4 = TSAOut_Dim2PosInternal(local_2c,1);
      uVar5 = TSAOut_Dim1PosInternal(local_2a,1);
      uVar9 = CONCAT44(uVar10,(uint)uVar4);
      StrPrintf_Process(0,"\nHotZone Id=%d, Adjust OK before %d#%d, after %d#%d\n",
                        *(undefined1 *)
                         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x154),uVar5,uVar9,CONCAT44(uVar12,(uint)uVar3),uVar2);
      uVar10 = (undefined4)((ulonglong)uVar9 >> 0x20);
    }
    StrPrintf_Process(0,"\nAppLog: ");
    sVar6 = TSAOut_IntToExtDist((int)(short)(*(short *)((longlong)(int)(uint)param_1 * 0x168 +
                                                        *(longlong *)_refptr_g_touchesPtr + 0x172) -
                                            local_2c));
    sVar7 = TSAOut_IntToExtDist((int)(short)(*(short *)((longlong)(int)(uint)param_1 * 0x168 +
                                                        *(longlong *)_refptr_g_touchesPtr + 0x170) -
                                            local_2a));
    StrPrintf_Process(0x10,"\nHotZone Id=%d, Adjust OK %d#%d\n",
                      *(undefined1 *)
                       ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x154),(int)sVar7,CONCAT44(uVar10,(int)sVar6));
  }
  return;
}


// ============================================================================
// Function: ClearDetailDescription @ 6baf69b5
// ============================================================================

void ClearDetailDescription(void)

{
  memset_s(&DAT_6bba3590,0x1f5,0,0x1f5);
  DAT_6bba3786 = 0;
  return;
}


// ============================================================================
// Function: DetailDescriptionPutStr @ 6baf6865
// ============================================================================

void DetailDescriptionPutStr(longlong param_1)

{
  uint uVar1;
  ushort local_a;
  
  if (param_1 != 0) {
    local_a = 0;
    while ((*(char *)(param_1 + (ulonglong)local_a) != '\0' && (DAT_6bba3786 < 500))) {
      uVar1 = (uint)DAT_6bba3786;
      DAT_6bba3786 = DAT_6bba3786 + 1;
      (&DAT_6bba3590)[(int)uVar1] = *(undefined1 *)(param_1 + (ulonglong)local_a);
      local_a = local_a + 1;
    }
    if (DAT_6bba3786 < 0x1f5) {
      (&DAT_6bba3590)[(int)(uint)DAT_6bba3786] = 0;
    }
  }
  return;
}


// ============================================================================
// Function: IsInHotZone @ 6baf4e98
// ============================================================================

undefined8 IsInHotZone(ushort param_1,ushort param_2,ushort *param_3)

{
  undefined8 uVar1;
  
  if ((((param_1 < *param_3) || (param_3[2] < param_1)) || (param_2 < param_3[1])) ||
     (param_3[3] < param_2)) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: atan2 @ 6bb494e0
// ============================================================================

double __cdecl atan2(double _Y,double _X)

{
  float10 fVar1;
  
  fVar1 = (float10)fpatan((float10)_Y,(float10)_X);
  return (double)fVar1;
}


// ============================================================================
// Function: CheckSrcCountRange @ 6ba466d0
// ============================================================================

undefined8 CheckSrcCountRange(undefined1 *param_1,longlong param_2,char *param_3,longlong param_4)

{
  undefined8 uVar1;
  char *local_20;
  longlong local_18;
  longlong local_10;
  
  local_20 = param_3;
  local_10 = param_2;
  for (local_18 = param_4; ((*local_20 != '\0' && (local_10 != 0)) && (local_18 != 0));
      local_18 = local_18 + -1) {
    local_20 = local_20 + 1;
    local_10 = local_10 + -1;
  }
  if (local_10 == 0) {
    *param_1 = 0;
    uVar1 = 0xa2;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: CheckSrcRange @ 6ba45c40
// ============================================================================

undefined8 CheckSrcRange(undefined1 *param_1,longlong param_2,char *param_3)

{
  undefined8 uVar1;
  char *local_18;
  longlong local_10;
  
  local_18 = param_3;
  for (local_10 = param_2; (*local_18 != '\0' && (local_10 != 0)); local_10 = local_10 + -1) {
    local_18 = local_18 + 1;
  }
  if (local_10 == 0) {
    *param_1 = 0;
    uVar1 = 0xa2;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: _errno @ EXTERNAL:00000035
// ============================================================================
// DECOMPILATION FAILED OR TIMED OUT
// ============================================================================
// Function: CompensationByPRInit @ 6baac486
// ============================================================================

void CompensationByPRInit(void)

{
  if ((*_refptr_g_asaPrmtFlash)[1].field_0x2 != '\0') {
    PolynomialRegression_SetComb
              (&g_signalCombPR,0x118,
               (char)*(undefined2 *)((*_refptr_g_asaPrmtFlash)[1].pStylus_unit_table + 0xe0),
               (char)*(undefined2 *)((*_refptr_g_asaPrmtFlash)[1].pStylus_unit_table + 0xe2));
    PolynomialRegression_SetComb
              (&g_posCombPR,0x118,
               (char)*(undefined2 *)((*_refptr_g_asaPrmtFlash)[1].pStylus_unit_table + 0x2b8),
               (char)*(undefined2 *)((*_refptr_g_asaPrmtFlash)[1].pStylus_unit_table + 0x2ba));
  }
  return;
}


// ============================================================================
// Function: ASAPropertyInit @ 6bac8350
// ============================================================================

void ASAPropertyInit(void)

{
                    /* 0x88350  106  ASAPropertyInit */
  memset_s(&g_asaPrpt,0x7ac,0,0x7ac);
  memset_s(&g_asaPrePrpt,0x7ac,0,0x7ac);
  return;
}


// ============================================================================
// Function: vsprintf @ 6bb4e4d8
// ============================================================================

int __cdecl vsprintf(char *_Dest,char *_Format,va_list _Args)

{
  int iVar1;
  
                    /* WARNING: Could not recover jumptable at 0x6bb4e4d8. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  iVar1 = vsprintf(_Dest,_Format,_Args);
  return iVar1;
}


// ============================================================================
// Function: SecNumber64ToString @ 6ba41ad4
// ============================================================================

void SecNumber64ToString(undefined8 param_1,longlong param_2)

{
  int iVar1;
  
  iVar1 = *(int *)(param_2 + 0x18);
  if (iVar1 == 10) {
    SecNumber64ToDecString(param_1,param_2);
  }
  else if (iVar1 == 0x10) {
    SecNumber64ToHexString(param_1,param_2);
  }
  else if (iVar1 == 8) {
    SecNumber64ToOctalString(param_1,param_2);
  }
  return;
}


// ============================================================================
// Function: SecWriteStringToStream @ 6ba41cf5
// ============================================================================

void SecWriteStringToStream(int *param_1,void *param_2,uint param_3)

{
  if ((int)param_3 < 0xc) {
    SecWriteStringToStreamOpt(param_1,param_2,param_3);
  }
  else {
    memcpy(*(void **)(param_1 + 2),param_2,(ulonglong)param_3);
    *(ulonglong *)(param_1 + 2) = (ulonglong)param_3 + *(longlong *)(param_1 + 2);
    *param_1 = *param_1 - param_3;
  }
  return;
}


// ============================================================================
// Function: wctomb @ 6bb4e4d0
// ============================================================================

int __cdecl wctomb(char *_MbCh,wchar_t _WCh)

{
  int iVar1;
  
                    /* WARNING: Could not recover jumptable at 0x6bb4e4d0. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  iVar1 = wctomb(_MbCh,_WCh);
  return iVar1;
}


// ============================================================================
// Function: strlen @ 6bb4e510
// ============================================================================

size_t __cdecl strlen(char *_Str)

{
  size_t sVar1;
  
                    /* WARNING: Could not recover jumptable at 0x6bb4e510. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  sVar1 = strlen(_Str);
  return sVar1;
}


// ============================================================================
// Function: SecSeekToFrontPercent @ 6ba42a87
// ============================================================================

void SecSeekToFrontPercent(undefined8 *param_1)

{
  char *local_10;
  
  for (local_10 = (char *)*param_1; *local_10 != '%'; local_10 = local_10 + -1) {
  }
  *param_1 = local_10;
  return;
}


// ============================================================================
// Function: JITFilter @ 6ba602c6
// ============================================================================

void JITFilter(int *param_1,int *param_2,int *param_3)

{
  uint uVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  uint local_20;
  uint local_1c;
  
                    /* 0x202c6  893  JITFilter */
  uVar3 = *param_1 - param_1[2];
  uVar4 = param_1[1] - param_1[3];
  uVar1 = uVar4 * uVar4 + uVar3 * uVar3;
  iVar2 = MIN(param_1[5],param_1[4]);
  param_1[5] = iVar2;
  iVar2 = Misc_SqrtUint32(uVar1);
  if (((uint)(param_1[4] * param_1[4]) < uVar1) && (iVar2 != 0)) {
    local_1c = (int)(param_1[4] * uVar3) / iVar2;
    if ((int)(((int)local_1c >> 0x1f ^ local_1c) - ((int)local_1c >> 0x1f)) < param_1[5]) {
      uVar1 = (int)uVar3 >> 0x1f;
      if ((int)((uVar1 ^ uVar3) - uVar1) < param_1[5]) {
        local_1c = uVar3;
      }
      else {
        local_1c = (int)(param_1[5] * uVar3) / (int)((uVar1 ^ uVar3) - uVar1);
      }
    }
    *param_2 = *param_1 - local_1c;
    local_20 = (int)(param_1[4] * uVar4) / iVar2;
    if ((int)(((int)local_20 >> 0x1f ^ local_20) - ((int)local_20 >> 0x1f)) < param_1[5]) {
      uVar1 = (int)uVar4 >> 0x1f;
      if ((int)((uVar1 ^ uVar4) - uVar1) < param_1[5]) {
        local_20 = uVar4;
      }
      else {
        local_20 = (int)(param_1[5] * uVar4) / (int)((uVar1 ^ uVar4) - uVar1);
      }
    }
    *param_3 = param_1[1] - local_20;
  }
  else {
    *param_2 = param_1[2];
    *param_3 = param_1[3];
  }
  return;
}


// ============================================================================
// Function: ExtToInt @ 6baf409d
// ============================================================================

void ExtToInt(longlong param_1,byte param_2,longlong param_3,byte param_4)

{
  undefined2 uVar1;
  byte local_19;
  
  local_19 = 0;
  while (((local_19 < param_2 && (local_19 < param_4)) &&
         ((*(short *)(param_3 + (ulonglong)local_19 * 0xe) != 0 ||
          (((*(short *)(param_3 + (ulonglong)local_19 * 0xe + 4) != 0 ||
            (*(short *)(param_3 + (ulonglong)local_19 * 0xe + 2) != 0)) ||
           (*(short *)(param_3 + (ulonglong)local_19 * 0xe + 6) != 0))))))) {
    uVar1 = TSAOut_RevertDim1Pos(*(undefined2 *)(param_3 + (ulonglong)local_19 * 0xe));
    *(undefined2 *)((ulonglong)local_19 * 0xe + param_1) = uVar1;
    uVar1 = TSAOut_RevertDim1Pos(*(undefined2 *)(param_3 + (ulonglong)local_19 * 0xe + 4));
    *(undefined2 *)((ulonglong)local_19 * 0xe + param_1 + 4) = uVar1;
    uVar1 = TSAOut_RevertDim2Pos(*(undefined2 *)(param_3 + (ulonglong)local_19 * 0xe + 2));
    *(undefined2 *)((ulonglong)local_19 * 0xe + param_1 + 2) = uVar1;
    uVar1 = TSAOut_RevertDim2Pos(*(undefined2 *)(param_3 + (ulonglong)local_19 * 0xe + 6));
    *(undefined2 *)((ulonglong)local_19 * 0xe + param_1 + 6) = uVar1;
    local_19 = local_19 + 1;
  }
  return;
}


// ============================================================================
// Function: SearchNearestDisTwo @ 6baf458d
// ============================================================================

void SearchNearestDisTwo(undefined2 param_1,undefined2 param_2,longlong param_3,byte param_4,
                        ushort *param_5,ushort *param_6,byte *param_7)

{
  ushort uVar1;
  byte local_9;
  
  local_9 = 0;
  while ((local_9 < param_4 &&
         ((((*(short *)(param_3 + (ulonglong)local_9 * 0xe) != 0 ||
            (*(short *)(param_3 + (ulonglong)local_9 * 0xe + 4) != 0)) ||
           (*(short *)(param_3 + (ulonglong)local_9 * 0xe + 2) != 0)) ||
          (*(short *)(param_3 + (ulonglong)local_9 * 0xe + 6) != 0))))) {
    uVar1 = CalculateDistance(param_3 + (ulonglong)local_9 * 0xe,param_1,param_2);
    if (uVar1 < *param_5) {
      *param_6 = *param_5;
      *param_5 = uVar1;
      *param_7 = local_9;
    }
    else if (uVar1 < *param_6) {
      *param_6 = uVar1;
    }
    local_9 = local_9 + 1;
  }
  return;
}


// ============================================================================
// Function: CalculateSizeIndex @ 6baf46dd
// ============================================================================

ushort CalculateSizeIndex(ushort *param_1)

{
  ushort uVar1;
  ushort uVar2;
  short sVar3;
  short sVar4;
  uint uVar5;
  uint uVar6;
  ushort local_20;
  ushort local_1e;
  uint local_1c;
  
  uVar1 = param_1[3];
  uVar2 = param_1[1];
  sVar3 = TSAOut_IntToExtDist((int)(short)((int)((uint)param_1[2] - (uint)*param_1) / 2));
  sVar4 = TSAOut_IntToExtDist((int)(short)((int)((uint)uVar1 - (uint)uVar2) / 2));
  uVar5 = (int)sVar4 * sVar3 * 4;
  local_1c = 0xffffffff;
  local_1e = 0;
  for (local_20 = 0; local_20 < (byte)_refptr_g_tsaPrmtFlash[2].field_0x7bf; local_20 = local_20 + 1
      ) {
    uVar1 = *(ushort *)(&_refptr_g_tsaPrmtFlash[2].field_0x7c0 + (longlong)(int)(uint)local_20 * 2);
    if ((uint)uVar1 * (uint)uVar1 < uVar5) {
      uVar6 = uVar5 - (uint)uVar1 * (uint)uVar1;
    }
    else {
      uVar6 = (uint)uVar1 * (uint)uVar1 - uVar5;
    }
    if (uVar6 < local_1c) {
      local_1e = local_20;
      local_1c = uVar6;
    }
  }
  return local_1e;
}


// ============================================================================
// Function: CalculateTargetZoneLen @ 6baf4c6c
// ============================================================================

void CalculateTargetZoneLen
               (ushort param_1,ushort param_2,ushort param_3,ushort param_4,ushort *param_5,
               ushort *param_6)

{
  ushort uVar1;
  ushort uVar2;
  ushort local_res8;
  ushort local_res10;
  
  local_res10 = param_2;
  if (param_2 == 0) {
    local_res10 = 1;
  }
  local_res8 = param_1;
  if (param_1 == 0) {
    local_res8 = 1;
  }
  uVar1 = (ushort)((int)((uint)local_res8 * (uint)param_4) / (int)(uint)local_res10);
  uVar2 = (ushort)((int)((uint)local_res10 * (uint)param_3) / (int)(uint)local_res8);
  if (uVar1 <= param_3) {
    param_3 = uVar1;
  }
  *param_5 = param_3;
  if (uVar2 <= param_4) {
    param_4 = uVar2;
  }
  *param_6 = param_4;
  return;
}


// ============================================================================
// Function: IsInAdjustArea @ 6baf4e28
// ============================================================================

undefined8 IsInAdjustArea(undefined2 param_1,undefined2 param_2,longlong param_3)

{
  char cVar1;
  
  cVar1 = IsInAdjustLine(param_1,*(undefined4 *)(param_3 + 8),*(undefined2 *)(param_3 + 0x10));
  if ((cVar1 != '\0') &&
     (cVar1 = IsInAdjustLine(param_2,*(undefined4 *)(param_3 + 0xc),*(undefined2 *)(param_3 + 0x12))
     , cVar1 != '\0')) {
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: CalculateNewPosition @ 6baf4eef
// ============================================================================

void CalculateNewPosition
               (longlong param_1,ushort param_2,ushort param_3,longlong param_4,short *param_5,
               short *param_6)

{
  *param_5 = (short)*(undefined4 *)(param_4 + 8) +
             (short)((int)(((uint)param_2 - *(int *)(param_1 + 8)) *
                          (uint)*(ushort *)(param_4 + 0x10)) /
                    (int)(uint)*(ushort *)(param_1 + 0x10));
  *param_6 = (short)*(undefined4 *)(param_4 + 0xc) +
             (short)((int)(((uint)param_3 - *(int *)(param_1 + 0xc)) *
                          (uint)*(ushort *)(param_4 + 0x12)) /
                    (int)(uint)*(ushort *)(param_1 + 0x12));
  return;
}


// ============================================================================
// Function: sin @ 6bb49f40
// ============================================================================

double __cdecl sin(double _X)

{
  int *piVar1;
  double dVar2;
  float10 local_48;
  float10 local_38;
  undefined8 local_20;
  
  local_20._4_4_ = (uint)((ulonglong)_X >> 0x20);
  local_20._0_4_ = SUB84(_X,0);
  local_20 = _X;
  if (((((ulonglong)_X & 0xfffff00000000) == 0 && (int)local_20 == 0) &&
       ((ulonglong)_X & 0x7ff0000000000000) == 0) || ((local_20._4_4_ & 0x7ff00000) != 0x7ff00000))
  {
    local_48 = (float10)_X;
    __sinl_internal(&local_38,&local_48);
    dVar2 = (double)local_38;
  }
  else if (((ulonglong)_X & 0xfffff00000000) == 0 && (int)local_20 == 0) {
    piVar1 = _errno();
    dVar2 = DAT_6bb60758;
    *piVar1 = 0x21;
    _text(1,&_rdata,_X,0,dVar2);
  }
  else {
    piVar1 = _errno();
    *piVar1 = 0x21;
    _text(1,&_rdata,_X,0,_X);
    dVar2 = _X;
  }
  return dVar2;
}


// ============================================================================
// Function: GripFilterCheckMultiGripUnit @ 6bae73f3
// ============================================================================

void GripFilterCheckMultiGripUnit(byte param_1)

{
  char cVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  int iVar4;
  
                    /* 0xa73f3  642  GripFilterCheckMultiGripUnit */
  if ((((*_refptr_g_grip != param_1) && (cVar1 = Touch_IsGhostByMinSigOff(param_1), cVar1 == '\0'))
      && (cVar1 = Touch_IsOff(param_1), cVar1 == '\0')) &&
     ((((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x214)
        & 4) != 0 &&
       (cVar1 = GripFilter_IsTouchIntervalInTime(*_refptr_g_grip,param_1,0,0xfa), cVar1 != '\0')) &&
      (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8)
        & 0x20000000) == 0 && (cVar1 = GripFilter_IsRatioOK(3), cVar1 != '\0')))))) {
    uVar2 = TSA_GetSide(*(undefined2 *)
                         ((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                          *(longlong *)_refptr_g_touchesPtr + 0x170),
                        *(undefined2 *)
                         ((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                          *(longlong *)_refptr_g_touchesPtr + 0x172));
    uVar3 = TSA_GetSide(*(undefined2 *)
                         ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x170),*(undefined2 *)
                                 ((longlong)(int)(uint)param_1 * 0x168 +
                                  *(longlong *)_refptr_g_touchesPtr + 0x172));
    iVar4 = GripFilter_GetCoreDistance(*_refptr_g_grip,param_1);
    if (iVar4 < 6) {
      *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                *(longlong *)_refptr_g_touchesPtr + 0x214) =
           *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                     *(longlong *)_refptr_g_touchesPtr + 0x214) | 2;
    }
    else {
      cVar1 = GripFilter_IsOnOppositeSide(uVar2,uVar3);
      if (cVar1 != '\0') {
        cVar1 = GripFilter_IsVideoMode();
        if (cVar1 == '\0') {
          *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                    *(longlong *)_refptr_g_touchesPtr + 0x214) =
               *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                         *(longlong *)_refptr_g_touchesPtr + 0x214) | 0x100;
        }
        else {
          *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                    *(longlong *)_refptr_g_touchesPtr + 0x214) =
               *(uint *)((longlong)(int)(uint)(byte)*_refptr_g_grip * 0x168 +
                         *(longlong *)_refptr_g_touchesPtr + 0x214) | 2;
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: vsprintf @ EXTERNAL:0000005f
// ============================================================================
// DECOMPILATION FAILED OR TIMED OUT
// ============================================================================
// Function: SecNumber64ToDecString @ 6ba41977
// ============================================================================

void SecNumber64ToDecString(ulonglong param_1,longlong param_2)

{
  ulonglong local_10;
  
  local_10 = param_1;
  do {
    *(longlong *)(param_2 + 8) = *(longlong *)(param_2 + 8) + -1;
    **(char **)(param_2 + 8) = "0123456789abcdefx"[local_10 % 10];
    local_10 = local_10 / 10;
  } while (local_10 != 0);
  return;
}


// ============================================================================
// Function: SecNumber64ToOctalString @ 6ba41a0d
// ============================================================================

void SecNumber64ToOctalString(ulonglong param_1,longlong param_2)

{
  ulonglong local_10;
  
  local_10 = param_1;
  do {
    *(longlong *)(param_2 + 8) = *(longlong *)(param_2 + 8) + -1;
    **(char **)(param_2 + 8) = "0123456789abcdefx"[(uint)local_10 & 7];
    local_10 = local_10 >> 3;
  } while (local_10 != 0);
  return;
}


// ============================================================================
// Function: SecNumber64ToHexString @ 6ba41a71
// ============================================================================

void SecNumber64ToHexString(ulonglong param_1,longlong *param_2)

{
  ulonglong local_10;
  
  local_10 = param_1;
  do {
    param_2[1] = param_2[1] + -1;
    *(undefined1 *)param_2[1] = *(undefined1 *)(*param_2 + (ulonglong)((uint)local_10 & 0xf));
    local_10 = local_10 >> 4;
  } while (local_10 != 0);
  return;
}


// ============================================================================
// Function: wctomb @ EXTERNAL:00000060
// ============================================================================
// DECOMPILATION FAILED OR TIMED OUT
// ============================================================================
// Function: strlen @ EXTERNAL:00000058
// ============================================================================
// DECOMPILATION FAILED OR TIMED OUT
// ============================================================================
// Function: MIN @ 6ba602b0
// ============================================================================

int MIN(int param_1,int param_2)

{
  if (param_2 <= param_1) {
    param_1 = param_2;
  }
  return param_1;
}


// ============================================================================
// Function: CalculateDistance @ 6baf4286
// ============================================================================

void CalculateDistance(ushort *param_1,ushort param_2,ushort param_3)

{
  ushort uVar1;
  ushort uVar2;
  ushort uVar3;
  ushort uVar4;
  int local_c;
  
  uVar1 = *param_1;
  uVar2 = param_1[2];
  uVar3 = param_1[1];
  uVar4 = param_1[3];
  local_c = 0;
  if ((param_2 < uVar1) && (param_3 < uVar3)) {
    local_c = ((uint)uVar3 - (uint)param_3) * ((uint)uVar3 - (uint)param_3) +
              ((uint)uVar1 - (uint)param_2) * ((uint)uVar1 - (uint)param_2);
  }
  else if ((param_2 < uVar1) || ((uVar2 < param_2 || (uVar3 <= param_3)))) {
    if ((uVar2 < param_2) && (param_3 < uVar3)) {
      local_c = ((uint)uVar3 - (uint)param_3) * ((uint)uVar3 - (uint)param_3) +
                ((uint)param_2 - (uint)uVar2) * ((uint)param_2 - (uint)uVar2);
    }
    else if (((param_2 < uVar1) && (uVar3 <= param_3)) && (param_3 <= uVar4)) {
      local_c = ((uint)uVar1 - (uint)param_2) * ((uint)uVar1 - (uint)param_2);
    }
    else if (((param_2 < uVar1) || (uVar2 < param_2)) || ((param_3 < uVar3 || (uVar4 < param_3)))) {
      if (((uVar2 < param_2) && (uVar3 <= param_3)) && (param_3 <= uVar4)) {
        local_c = ((uint)param_2 - (uint)uVar2) * ((uint)param_2 - (uint)uVar2);
      }
      else if ((param_2 < uVar1) && (uVar4 < param_3)) {
        local_c = ((uint)param_3 - (uint)uVar4) * ((uint)param_3 - (uint)uVar4) +
                  ((uint)uVar1 - (uint)param_2) * ((uint)uVar1 - (uint)param_2);
      }
      else if ((param_2 < uVar1) || ((uVar2 < param_2 || (param_3 <= uVar4)))) {
        if ((uVar2 < param_2) && (uVar4 < param_3)) {
          local_c = ((uint)param_3 - (uint)uVar4) * ((uint)param_3 - (uint)uVar4) +
                    ((uint)param_2 - (uint)uVar2) * ((uint)param_2 - (uint)uVar2);
        }
      }
      else {
        local_c = ((uint)param_3 - (uint)uVar4) * ((uint)param_3 - (uint)uVar4);
      }
    }
    else {
      local_c = 0;
    }
  }
  else {
    local_c = ((uint)uVar3 - (uint)param_3) * ((uint)uVar3 - (uint)param_3);
  }
  Misc_SqrtUint32(local_c);
  return;
}


// ============================================================================
// Function: IsInAdjustLine @ 6baf4dd6
// ============================================================================

undefined8 IsInAdjustLine(ushort param_1,int param_2,ushort param_3)

{
  undefined8 uVar1;
  
  if (((int)(param_2 - (uint)(param_3 >> 1)) < (int)(uint)param_1) &&
     ((int)(uint)param_1 < (int)(param_2 + (uint)(param_3 >> 1)))) {
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: __sinl_internal @ 6bb4a020
// ============================================================================

float10 * __sinl_internal(float10 *param_1,unkbyte10 *param_2)

{
  ushort in_FPUStatusWord;
  float10 fVar1;
  float10 fVar2;
  
  fVar1 = (float10)fsin(*param_2);
  if ((in_FPUStatusWord & 0x400) == 0) {
    *(undefined8 *)((longlong)param_1 + 8) = 0;
    *param_1 = fVar1;
    return param_1;
  }
  fVar2 = (float10)3.141592653589793 + (float10)3.141592653589793;
  do {
    fVar1 = fVar1 - ROUND(fVar1 / fVar2) * fVar2;
  } while ((in_FPUStatusWord & 0x400) != 0);
  fVar1 = (float10)fsin(fVar1);
  *(undefined8 *)((longlong)param_1 + 8) = 0;
  *param_1 = fVar1;
  return param_1;
}


// ============================================================================
// Function: MAX @ 6bae1df0
// ============================================================================

int MAX(int param_1,int param_2)

{
  if (param_1 <= param_2) {
    param_1 = param_2;
  }
  return param_1;
}


// ============================================================================
// Function: MIN @ 6bae1e06
// ============================================================================

int MIN(int param_1,int param_2)

{
  if (param_2 <= param_1) {
    param_1 = param_2;
  }
  return param_1;
}


// ============================================================================
// Function: MAX @ 6ba7c760
// ============================================================================

int MAX(int param_1,int param_2)

{
  if (param_1 <= param_2) {
    param_1 = param_2;
  }
  return param_1;
}


// ============================================================================
// Function: MAX @ 6ba83ad0
// ============================================================================

int MAX(int param_1,int param_2)

{
  if (param_1 <= param_2) {
    param_1 = param_2;
  }
  return param_1;
}


// ============================================================================
// Function: MIN @ 6ba83ae6
// ============================================================================

int MIN(int param_1,int param_2)

{
  if (param_2 <= param_1) {
    param_1 = param_2;
  }
  return param_1;
}


// ============================================================================
// Function: cos @ 6bb49510
// ============================================================================

double __cdecl cos(double _X)

{
  int *piVar1;
  double dVar2;
  float10 local_48;
  float10 local_38;
  undefined8 local_20;
  
  local_20._4_4_ = (uint)((ulonglong)_X >> 0x20);
  local_20._0_4_ = SUB84(_X,0);
  local_20 = _X;
  if (((((ulonglong)_X & 0xfffff00000000) == 0 && (int)local_20 == 0) &&
       ((ulonglong)_X & 0x7ff0000000000000) == 0) || ((local_20._4_4_ & 0x7ff00000) != 0x7ff00000))
  {
    local_48 = (float10)_X;
    __cosl_internal(&local_38,&local_48);
    dVar2 = (double)local_38;
  }
  else if (((ulonglong)_X & 0xfffff00000000) == 0 && (int)local_20 == 0) {
    piVar1 = _errno();
    dVar2 = DAT_6bb60698;
    *piVar1 = 0x21;
    _text(1,&_rdata,_X,0,dVar2);
  }
  else {
    piVar1 = _errno();
    *piVar1 = 0x21;
    _text(1,&_rdata,_X,0,_X);
    dVar2 = _X;
  }
  return dVar2;
}


// ============================================================================
// Function: MAX @ 6bb0cf70
// ============================================================================

int MAX(int param_1,int param_2)

{
  if (param_1 <= param_2) {
    param_1 = param_2;
  }
  return param_1;
}


// ============================================================================
// Function: __cosl_internal @ 6bb495f0
// ============================================================================

float10 * __cosl_internal(float10 *param_1,unkbyte10 *param_2)

{
  ushort in_FPUStatusWord;
  float10 fVar1;
  float10 fVar2;
  
  fVar1 = (float10)fcos(*param_2);
  if ((in_FPUStatusWord & 0x400) != 0) {
    fVar2 = (float10)3.141592653589793 + (float10)3.141592653589793;
    do {
      fVar1 = fVar1 - ROUND(fVar1 / fVar2) * fVar2;
    } while ((in_FPUStatusWord & 0x400) != 0);
    fVar1 = (float10)fcos(fVar1);
  }
  *(undefined8 *)((longlong)param_1 + 8) = 0;
  *param_1 = fVar1;
  return param_1;
}



// ============================================================================
// Function: Misc_GetShiftNum @ 6ba64b9c

// ============================================================================
// Function: Misc_ShiftInINT64S @ 6ba64c33

// ============================================================================
// Function: Misc_GetShiftBits @ 6ba64cdb

// ============================================================================
// Function: Misc_RightShift @ 6ba64d3c

// ============================================================================
// Function: Misc_SqrtUint32 @ 6ba64d8b

// ============================================================================
// Function: Misc_SqrtUint64 @ 6ba64ddd

// ============================================================================
// Function: Misc_Sin @ 6ba64e40

// ============================================================================
// Function: Misc_Cos @ 6ba64eaa

// ============================================================================
// Function: Misc_IsInRect @ 6ba64f1f

// ============================================================================
// Function: Misc_FindKthSmallest @ 6ba6572e
