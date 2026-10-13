// ============================================================================
// Function: CMFRemap_IsNeedRemap @ 6ba506e5
// ============================================================================

undefined8 CMFRemap_IsNeedRemap(void)

{
  undefined8 uVar1;
  
                    /* 0x106e5  461  CMFRemap_IsNeedRemap */
  if (((g_cmfRemapNumRxCombine == 1) && (_refptr_g_tsaPrmtFlash->field_0x6cd == '\0')) &&
     (_refptr_g_tsaPrmtFlash->field_0x80e == '\0')) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: CMFRemap_Enter @ 6ba50726
// ============================================================================

/* WARNING: Function: ___chkstk_ms replaced with injection: alloca_probe */

void CMFRemap_Enter(undefined1 *param_1,undefined1 *param_2)

{
  int iVar1;
  short local_2678 [4902];
  uint local_2c;
  int local_28;
  uint local_24;
  errno_t local_20;
  int local_1c;
  uint local_18;
  uint local_14;
  uint local_10;
  uint local_c;
  
                    /* 0x10726  453  CMFRemap_Enter */
  local_10 = 0x6ba50731;
  local_c = 0;
  iVar1 = CMFRemap_IsNeedRemap();
  if (iVar1 != 0) {
    if (param_1 != (undefined1 *)0x0) {
      *param_1 = (char)g_cmfRemapMapRxNum;
    }
    if (param_2 != (undefined1 *)0x0) {
      *param_2 = (char)g_cmfRemapMapTxNum;
    }
    local_18 = (uint)g_cmfRemapMapTxNum * (uint)g_cmfRemapMapRxNum;
    if (g_cmfRemapIsReshape == '\0') {
      local_20 = memcpy_s(local_2678,0x2648,*(void **)_refptr_g_tsaBufDif,
                          (ulonglong)g_cmfRemapTxNum * (ulonglong)g_cmfRemapRxNum * 2);
      if (local_20 != 0) {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                          "../../src/Alg/CMF/CMFRemap.c",0x1c9,"rc == EOK");
        HardwareAnalyzer_SetSoftException();
      }
      if (_refptr_g_tsaPrmtFlash->field_0x6cd == '\0') {
        for (local_14 = 0; local_14 < local_18; local_14 = local_14 + 1) {
          local_2c = (uint)*(ushort *)(g_cmfRemapDifIdx + (ulonglong)local_14 * 2);
          *(short *)((ulonglong)local_14 * 2 + *(longlong *)_refptr_g_tsaBufDif) =
               local_2678[local_2c];
        }
      }
      else {
        for (local_10 = 0; local_10 < local_18; local_10 = local_10 + 1) {
          local_24 = (uint)*(ushort *)(g_cmfRemapDifIdx + (ulonglong)local_10 * 2);
          local_28 = (int)*(short *)(g_dif2MapCoef + (ulonglong)local_24 * 2);
          iVar1 = local_2678[local_24] * local_28;
          if (iVar1 < 0) {
            iVar1 = iVar1 + 0x3ff;
          }
          *(short *)((ulonglong)local_10 * 2 + *(longlong *)_refptr_g_tsaBufDif) =
               (short)(iVar1 >> 10);
        }
      }
    }
    else if (_refptr_g_tsaPrmtFlash->field_0x6cd != '\0') {
      for (local_c = 0; local_c < local_18; local_c = local_c + 1) {
        local_1c = (int)*(short *)(g_dif2MapCoef + (ulonglong)local_c * 2);
        iVar1 = *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)local_c * 2) * local_1c;
        if (iVar1 < 0) {
          iVar1 = iVar1 + 0x3ff;
        }
        *(short *)((ulonglong)local_c * 2 + *(longlong *)_refptr_g_tsaBufDif) = (short)(iVar1 >> 10)
        ;
      }
    }
    local_20 = memcpy_s(_refptr_g_tsaBufCMFRemap,0x2648,*(void **)_refptr_g_tsaBufDif,
                        (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
    if (local_20 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/CMF/CMFRemap.c",
                        0x1dd,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
  }
  return;
}


// ============================================================================
// Function: CMFRemap_Exit @ 6ba50a69
// ============================================================================

/* WARNING: Function: ___chkstk_ms replaced with injection: alloca_probe */

void CMFRemap_Exit(void)

{
  int iVar1;
  short local_4cc8 [4904];
  short local_2678 [4902];
  uint local_2c;
  int local_28;
  uint local_24;
  errno_t local_20;
  int local_1c;
  uint local_18;
  uint local_14;
  uint local_10;
  uint local_c;
  
                    /* 0x10a69  454  CMFRemap_Exit */
  local_10 = 0x6ba50a74;
  local_c = 0;
  iVar1 = CMFRemap_IsNeedRemap();
  if (iVar1 != 0) {
    local_18 = (uint)g_cmfRemapTxNum * (uint)g_cmfRemapRxNum;
    if (g_cmfRemapIsReshape == '\0') {
      local_20 = memcpy_s(local_2678,0x2648,*(void **)_refptr_g_tsaBufDif,
                          (ulonglong)g_cmfRemapMapTxNum * (ulonglong)g_cmfRemapMapRxNum * 2);
      if (local_20 != 0) {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                          "../../src/Alg/CMF/CMFRemap.c",0x1fb,"rc == EOK");
        HardwareAnalyzer_SetSoftException();
      }
      local_20 = memcpy_s(local_4cc8,0x2648,*(void **)_refptr_g_tsaBufCommonModeNoise,
                          (ulonglong)g_cmfRemapMapTxNum * (ulonglong)g_cmfRemapMapRxNum * 2);
      if (local_20 != 0) {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                          "../../src/Alg/CMF/CMFRemap.c",0x1fe,"rc == EOK");
        HardwareAnalyzer_SetSoftException();
      }
      if (_refptr_g_tsaPrmtFlash->field_0x6cd == '\0') {
        for (local_14 = 0; local_14 < local_18; local_14 = local_14 + 1) {
          local_2c = (uint)*(ushort *)(g_cmfRemapMapIdx + (ulonglong)local_14 * 2);
          *(short *)((ulonglong)local_14 * 2 + *(longlong *)_refptr_g_tsaBufDif) =
               local_2678[local_2c];
          *(short *)((ulonglong)local_14 * 2 + *(longlong *)_refptr_g_tsaBufCommonModeNoise) =
               local_4cc8[local_2c];
        }
      }
      else {
        for (local_10 = 0; local_10 < local_18; local_10 = local_10 + 1) {
          local_24 = (uint)*(ushort *)(g_cmfRemapMapIdx + (ulonglong)local_10 * 2);
          local_28 = (int)*(short *)(g_map2DifCoef + (ulonglong)local_10 * 2);
          iVar1 = local_2678[local_24] * local_28;
          if (iVar1 < 0) {
            iVar1 = iVar1 + 0x3ff;
          }
          *(short *)((ulonglong)local_10 * 2 + *(longlong *)_refptr_g_tsaBufDif) =
               (short)(iVar1 >> 10);
          iVar1 = local_4cc8[local_24] * local_28;
          if (iVar1 < 0) {
            iVar1 = iVar1 + 0x3ff;
          }
          *(short *)((ulonglong)local_10 * 2 + *(longlong *)_refptr_g_tsaBufCommonModeNoise) =
               (short)(iVar1 >> 10);
        }
      }
    }
    else if (_refptr_g_tsaPrmtFlash->field_0x6cd != '\0') {
      for (local_c = 0; local_c < local_18; local_c = local_c + 1) {
        local_1c = (int)*(short *)(g_map2DifCoef + (ulonglong)local_c * 2);
        iVar1 = *(short *)(*(longlong *)_refptr_g_tsaBufDif + (ulonglong)local_c * 2) * local_1c;
        if (iVar1 < 0) {
          iVar1 = iVar1 + 0x3ff;
        }
        *(short *)((ulonglong)local_c * 2 + *(longlong *)_refptr_g_tsaBufDif) = (short)(iVar1 >> 10)
        ;
        iVar1 = *(short *)(*(longlong *)_refptr_g_tsaBufCommonModeNoise + (ulonglong)local_c * 2) *
                local_1c;
        if (iVar1 < 0) {
          iVar1 = iVar1 + 0x3ff;
        }
        *(short *)((ulonglong)local_c * 2 + *(longlong *)_refptr_g_tsaBufCommonModeNoise) =
             (short)(iVar1 >> 10);
      }
    }
    local_20 = memcpy_s(_refptr_g_tsaBufCMFRemapBack,0x2648,*(void **)_refptr_g_tsaBufDif,
                        (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
    if (local_20 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/CMF/CMFRemap.c",
                        0x213,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
  }
  return;
}


// ============================================================================
// Function: CMFRemap_Init @ 6ba503b6
// ============================================================================

void CMFRemap_Init(void)

{
  byte bVar1;
  byte bVar2;
  int iVar3;
  longlong lVar4;
  char *pcVar5;
  char local_78 [80];
  uint local_28;
  uint local_24;
  int local_20;
  int local_1c;
  
                    /* 0x103b6  458  CMFRemap_Init */
  g_cmfRemapNumRxCombine = (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x6cc;
  pcVar5 = local_78;
  for (lVar4 = 10; lVar4 != 0; lVar4 = lVar4 + -1) {
    pcVar5[0] = '\0';
    pcVar5[1] = '\0';
    pcVar5[2] = '\0';
    pcVar5[3] = '\0';
    pcVar5[4] = '\0';
    pcVar5[5] = '\0';
    pcVar5[6] = '\0';
    pcVar5[7] = '\0';
    pcVar5 = pcVar5 + 8;
  }
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    bVar2 = _refptr_g_tsaPrmtFlash->bCols;
    bVar1 = _refptr_g_tsaPrmtFlash->bRows;
  }
  else {
    bVar2 = _refptr_g_tsaPrmtFlash->bRows;
    bVar1 = _refptr_g_tsaPrmtFlash->bCols;
  }
  g_cmfRemapTxNum = (ushort)bVar2;
  g_cmfRemapRxNum = (ushort)bVar1;
  CMFRemap_InitCoef2D();
  if (_refptr_g_tsaPrmtFlash->field_0x80e == '\0') {
    local_1c = 0;
    for (local_20 = 0; local_20 < (int)(uint)g_cmfRemapTxNum; local_20 = local_20 + 1) {
      if (local_78[local_20] == '\0') {
        bVar2 = CMFRemap_Mapping(local_20,g_cmfRemapNumRxCombine,local_1c,local_78,0x50);
        local_28 = (uint)bVar2;
        if ((int)(uint)bVar2 < (int)g_cmfRemapNumRxCombine) {
          if (*_refptr_g_tsaLogLoglevel != '\0') {
            StrPrintf_Process(0,
                              "slotTable cann\'t support numRxCombine(%d) , g_cmfRemapNumRxCombine is set to 1\n"
                              ,g_cmfRemapNumRxCombine);
          }
          if (local_28 != g_cmfRemapNumRxCombine) {
            StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                              "../../src/Alg/CMF/CMFRemap.c",0x18d,"line == g_cmfRemapNumRxCombine")
            ;
            HardwareAnalyzer_SetSoftException();
          }
          g_cmfRemapNumRxCombine = 1;
          break;
        }
        local_1c = local_1c + 1;
      }
    }
    g_cmfRemapMapRxNum = g_cmfRemapRxNum * (short)g_cmfRemapNumRxCombine;
    g_cmfRemapMapTxNum = (ushort)local_1c;
    CMFRemap_GenerateIndexTable();
  }
  else {
    CMFRemap_GenerateIndexTableForDoubleIC
              (&_refptr_g_tsaPrmtFlash->field_0x67c,g_cmfRemapRxNum,g_cmfRemapTxNum);
  }
  iVar3 = CMFRemap_IsNeedRemap();
  if (iVar3 == 0) {
    for (local_24 = 0; local_24 < (uint)g_cmfRemapTxNum * (uint)g_cmfRemapRxNum;
        local_24 = local_24 + 1) {
      *(short *)(g_cmfRemapMapIdx + (ulonglong)local_24 * 2) = (short)local_24;
      *(short *)(g_cmfRemapDifIdx + (ulonglong)local_24 * 2) = (short)local_24;
    }
  }
  g_cmfRemapIsReshape = CMFRemap_IsReshape();
  if (0xff < g_cmfRemapMapRxNum) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/CMF/CMFRemap.c",
                      0x1a1,"g_cmfRemapMapRxNum < CMF_MAX_ELELEMENT_COUNT");
    HardwareAnalyzer_SetSoftException();
  }
  if (0xff < g_cmfRemapMapTxNum) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/CMF/CMFRemap.c",
                      0x1a2,"g_cmfRemapMapTxNum < CMF_MAX_ELELEMENT_COUNT");
    HardwareAnalyzer_SetSoftException();
  }
  if (0x1324 < (int)((uint)g_cmfRemapMapTxNum * (uint)g_cmfRemapMapRxNum)) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/CMF/CMFRemap.c",
                      0x1a3,"g_cmfRemapMapRxNum * g_cmfRemapMapTxNum <= TSA_MAX_SENSOR_NUM");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: CMFRemap_InitMapIdxBySlotTable @ 6ba4fda8
// ============================================================================

void CMFRemap_InitMapIdxBySlotTable
               (longlong param_1,undefined8 param_2,undefined8 param_3,longlong param_4)

{
  ushort uVar1;
  ushort uVar2;
  int local_48 [4];
  int local_38 [2];
  undefined1 local_30 [4];
  int local_2c;
  uint local_28;
  byte local_21;
  uint local_20;
  uint local_1c;
  uint local_18;
  uint local_14;
  uint local_10;
  uint local_c;
  
                    /* 0xfda8  460  CMFRemap_InitMapIdxBySlotTable */
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    uVar1 = *(ushort *)(param_4 + 0x2a);
    uVar2 = *(ushort *)(param_4 + 0x28);
  }
  else {
    uVar1 = *(ushort *)(param_4 + 0x28);
    uVar2 = *(ushort *)(param_4 + 0x2a);
  }
  local_c = (uint)uVar1;
  local_10 = (uint)uVar2;
  local_38[0] = 0;
  local_38[1] = 0;
  CMFRemap_GetRxSlotNum(param_1,local_c,local_30,local_38,2);
  *(short *)(param_4 + 0x2c) = (short)local_c;
  *(short *)(param_4 + 0x2e) = (short)local_10;
  for (local_18 = 0; local_18 < local_10; local_18 = local_18 + 1) {
    local_48[0] = 0;
    local_48[1] = 0;
    for (local_14 = 0; local_14 < local_c; local_14 = local_14 + 1) {
      local_28 = (uint)*(byte *)(param_1 + (ulonglong)local_14);
      local_2c = local_48[local_28];
      if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
        local_1c = local_18 + local_14 * local_10;
      }
      else {
        local_1c = local_14 + local_18 * local_c;
      }
      local_20 = local_48[local_28] + local_18 * local_c;
      for (local_21 = 0; local_21 < local_28; local_21 = local_21 + 1) {
        local_20 = local_20 + local_38[(int)(uint)local_21];
      }
      *(short *)(*(longlong *)(param_4 + 0x38) + (ulonglong)local_20 * 2) = (short)local_1c;
      *(short *)(*(longlong *)(param_4 + 0x30) + (ulonglong)local_1c * 2) = (short)local_20;
      local_48[local_28] = local_48[local_28] + 1;
    }
  }
  return;
}


// ============================================================================
// Function: CMFRemap_Mapping @ 6ba4f775
// ============================================================================

int CMFRemap_Mapping(int param_1,int param_2,int param_3,longlong param_4,byte param_5)

{
  char cVar1;
  TSAFlashPrmt *pTVar2;
  int local_14;
  int local_10;
  int local_c;
  
  pTVar2 = _refptr_g_tsaPrmtFlash;
  local_10 = 0;
  cVar1 = (&_refptr_g_tsaPrmtFlash->field_0x62c)[param_1];
  local_14 = param_1;
  local_c = param_1;
  do {
    if ((int)(uint)g_cmfRemapTxNum <= local_c) {
      do {
        do {
          local_c = local_14 + -1;
          if (local_c < 0) {
            return local_10;
          }
          local_14 = local_c;
        } while ((&pTVar2->field_0x62c)[local_c] != cVar1);
        (&g_cmfRemapMap2Dif)[g_cmfRemapNumRxCombine * param_3 + local_10] = (char)local_c;
        local_10 = local_10 + 1;
      } while (local_10 != param_2);
      return local_10;
    }
    if ((&pTVar2->field_0x62c)[local_c] == cVar1) {
      (&g_cmfRemapMap2Dif)[g_cmfRemapNumRxCombine * param_3 + local_10] = (char)local_c;
      local_14 = local_c;
      if (local_c < (int)(uint)param_5) {
        *(undefined1 *)(param_4 + local_c) = 1;
      }
      local_10 = local_10 + 1;
      if (local_10 == param_2) {
        return local_10;
      }
    }
    local_c = local_c + 1;
  } while( true );
}


// ============================================================================
// Function: CMFRemap_InitCoef2D @ 6ba4fa86
// ============================================================================

void CMFRemap_InitCoef2D(void)

{
  int iVar1;
  short local_298 [326];
  ushort local_c;
  byte local_a;
  byte local_9;
  
  CMFRemap_InitCoef(g_cmfRemapRxNum,&_refptr_g_tsaPrmtFlash->field_0x6ce,local_298 + 0xf0,
                    local_298 + 0x50);
  CMFRemap_InitCoef(g_cmfRemapTxNum,&_refptr_g_tsaPrmtFlash->field_0x76e,local_298 + 0xa0,local_298)
  ;
  for (local_9 = 0; local_9 < g_cmfRemapRxNum; local_9 = local_9 + 1) {
    for (local_a = 0; local_a < g_cmfRemapTxNum; local_a = local_a + 1) {
      if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\x01') {
        local_c = (ushort)local_9 + (ushort)local_a * (ushort)_refptr_g_tsaPrmtFlash->bCols;
      }
      else {
        local_c = (ushort)local_a + (ushort)local_9 * (ushort)_refptr_g_tsaPrmtFlash->bCols;
      }
      iVar1 = (int)local_298[(longlong)(int)(uint)local_a + 0xa0] *
              (int)local_298[(longlong)(int)(uint)local_9 + 0xf0];
      if (iVar1 < 0) {
        iVar1 = iVar1 + 0x3ff;
      }
      *(short *)((ulonglong)local_c * 2 + g_dif2MapCoef) = (short)(iVar1 >> 10);
      iVar1 = (int)local_298[(int)(uint)local_a] *
              (int)local_298[(longlong)(int)(uint)local_9 + 0x50];
      if (iVar1 < 0) {
        iVar1 = iVar1 + 0x3ff;
      }
      *(short *)((ulonglong)local_c * 2 + g_map2DifCoef) = (short)(iVar1 >> 10);
    }
  }
  return;
}


// ============================================================================
// Function: CMFRemap_GenerateIndexTableForDoubleIC @ 6ba4ff5e
// ============================================================================

void CMFRemap_GenerateIndexTableForDoubleIC(longlong param_1,uint param_2,uint param_3)

{
  uint uVar1;
  uint local_58 [4];
  undefined8 local_48;
  ushort local_40 [2];
  uint local_3c;
  uint local_38;
  int local_34;
  uint local_30;
  uint local_2c;
  uint local_28;
  int local_24;
  uint local_20;
  uint local_1c;
  uint local_18;
  uint local_14;
  uint local_10;
  uint local_c;
  
  local_48 = 0;
  local_1c = CMFRemap_GetRxSlotNum
                       (&_refptr_g_tsaPrmtFlash->field_0x67c,g_cmfRemapRxNum,local_40,&local_48,2);
  g_cmfRemapMapRxNum = local_40[0];
  g_cmfRemapMapTxNum = g_cmfRemapTxNum * (short)local_1c;
  for (local_c = 0; local_c < param_3; local_c = local_c + 1) {
    local_58[0] = 0;
    local_58[1] = 0;
    for (local_10 = 0; local_10 < param_2; local_10 = local_10 + 1) {
      local_20 = (uint)*(byte *)(param_1 + (ulonglong)local_10);
      local_24 = local_20 + local_c * local_1c;
      local_28 = local_58[local_20];
      if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\x01') {
        uVar1 = _refptr_g_tsaPrmtFlash->bCols * local_c + local_10;
      }
      else {
        uVar1 = _refptr_g_tsaPrmtFlash->bCols * local_10 + local_c;
      }
      local_2c = uVar1 & 0xffff;
      local_30 = local_58[local_20] + (uint)g_cmfRemapMapRxNum * local_24;
      *(short *)(g_cmfRemapDifIdx + (ulonglong)local_30 * 2) = (short)uVar1;
      *(short *)(g_cmfRemapMapIdx + (ulonglong)(uVar1 & 0xffff) * 2) = (short)local_30;
      local_58[local_20] = local_58[local_20] + 1;
    }
    for (local_14 = 0; local_14 < local_1c; local_14 = local_14 + 1) {
      local_18 = local_58[local_14] - 1;
      local_34 = local_14 + local_c * local_1c;
      local_38 = (local_58[local_14] - 1) + (uint)g_cmfRemapMapRxNum * local_34;
      for (local_18 = local_58[local_14]; local_18 < g_cmfRemapMapRxNum; local_18 = local_18 + 1) {
        local_34 = local_14 + local_c * local_1c;
        local_3c = local_18 + (uint)g_cmfRemapMapRxNum * local_34;
        *(undefined2 *)((ulonglong)local_3c * 2 + g_cmfRemapDifIdx) =
             *(undefined2 *)(g_cmfRemapDifIdx + (ulonglong)local_38 * 2);
      }
    }
  }
  return;
}


// ============================================================================
// Function: CMFRemap_GenerateIndexTable @ 6ba501aa
// ============================================================================

void CMFRemap_GenerateIndexTable(void)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  uint local_18;
  int local_14;
  int local_10;
  ushort local_a;
  
  uVar1 = (uint)g_cmfRemapRxNum;
  uVar2 = (uint)g_cmfRemapTxNum;
  for (local_a = 0; (int)(uint)local_a < (int)(uVar1 * uVar2); local_a = local_a + 1) {
    *(undefined2 *)(g_cmfRemapMapIdx + (ulonglong)local_a * 2) = 0xffff;
  }
  for (local_10 = 0; local_10 < (int)(g_cmfRemapNumRxCombine * (uint)g_cmfRemapMapTxNum);
      local_10 = local_10 + 1) {
    for (local_14 = 0; local_14 < (int)uVar1; local_14 = local_14 + 1) {
      if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
        local_18 = (uint)(byte)(&g_cmfRemapMap2Dif)[local_10] + local_14 * uVar2;
      }
      else {
        local_18 = local_14 + (byte)(&g_cmfRemapMap2Dif)[local_10] * uVar1;
      }
      uVar3 = local_14 + local_10 * uVar1;
      *(short *)(g_cmfRemapDifIdx + (ulonglong)uVar3 * 2) = (short)local_18;
      if (*(short *)(g_cmfRemapMapIdx + (ulonglong)local_18 * 2) == -1) {
        *(short *)(g_cmfRemapMapIdx + (ulonglong)local_18 * 2) = (short)uVar3;
      }
    }
  }
  return;
}


// ============================================================================
// Function: CMFRemap_IsReshape @ 6ba502fd
// ============================================================================

undefined8 CMFRemap_IsReshape(void)

{
  undefined8 uVar1;
  uint local_c;
  
  if ((uint)g_cmfRemapRxNum * (uint)g_cmfRemapTxNum ==
      (uint)g_cmfRemapMapTxNum * (uint)g_cmfRemapMapRxNum) {
    for (local_c = 0; local_c < (uint)g_cmfRemapTxNum * (uint)g_cmfRemapRxNum; local_c = local_c + 1
        ) {
      if (*(ushort *)(g_cmfRemapMapIdx + (ulonglong)local_c * 2) != local_c) {
        return 0;
      }
      if (*(ushort *)(g_cmfRemapDifIdx + (ulonglong)local_c * 2) != local_c) {
        return 0;
      }
    }
    uVar1 = 1;
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: CMFRemap_GetRxSlotNum @ 6ba4fc55
// ============================================================================

int CMFRemap_GetRxSlotNum(longlong param_1,uint param_2,uint *param_3,longlong param_4,uint param_5)

{
  uint uVar1;
  int *piVar2;
  uint local_14;
  uint local_10;
  uint local_c;
  
                    /* 0xfc55  456  CMFRemap_GetRxSlotNum */
  local_10 = 0;
  *param_3 = 0;
  for (local_14 = 0; local_14 < param_5; local_14 = local_14 + 1) {
    *(undefined4 *)(param_4 + (ulonglong)local_14 * 4) = 0;
  }
  for (local_c = 0; local_c < param_2; local_c = local_c + 1) {
    uVar1 = (uint)*(byte *)(param_1 + (ulonglong)local_c);
    if (uVar1 < param_5) {
      piVar2 = (int *)(param_4 + (ulonglong)uVar1 * 4);
      *piVar2 = *piVar2 + 1;
      if (local_10 < uVar1) {
        local_10 = uVar1;
      }
      if (*param_3 < *(uint *)(param_4 + (ulonglong)uVar1 * 4)) {
        *param_3 = *(uint *)(param_4 + (ulonglong)uVar1 * 4);
      }
    }
  }
  return local_10 + 1;
}


// ============================================================================
// Function: CMFRemap_GetTxSlotNum @ 6ba4fd4f
// ============================================================================

int CMFRemap_GetTxSlotNum(longlong param_1,uint param_2)

{
  uint local_10;
  uint local_c;
  
                    /* 0xfd4f  457  CMFRemap_GetTxSlotNum */
  local_10 = 0;
  for (local_c = 0; local_c < param_2; local_c = local_c + 1) {
    if (local_10 < *(byte *)(param_1 + (ulonglong)local_c)) {
      local_10 = (uint)*(byte *)(param_1 + (ulonglong)local_c);
    }
  }
  return local_10 + 1;
}


// ============================================================================
// Function: CMFRemap_InitCoef @ 6ba4f8b0
// ============================================================================

void CMFRemap_InitCoef(int param_1,longlong param_2,longlong param_3,longlong param_4)

{
  uint uVar1;
  uint uVar2;
  int local_res8;
  int local_18;
  int local_14;
  int local_10;
  int local_c;
  
  local_10 = 0;
  if (_refptr_g_tsaPrmtFlash->field_0x6cd == '\0') {
    for (local_c = 0; local_c < param_1; local_c = local_c + 1) {
      *(undefined2 *)(param_3 + (longlong)local_c * 2) = 0x400;
      *(undefined2 *)(param_4 + (longlong)local_c * 2) = 0x400;
    }
  }
  else {
    for (local_c = 0; local_c < param_1; local_c = local_c + 1) {
      uVar1 = (uint)*(short *)(param_2 + (longlong)local_c * 2);
      uVar2 = (int)uVar1 >> 0x1f;
      local_10 = local_10 + ((uVar1 ^ uVar2) - uVar2);
    }
    if (local_10 == 0) {
      local_10 = param_1;
    }
    local_res8 = param_1;
    if (param_1 == 0) {
      local_res8 = 1;
    }
    if (local_10 / local_res8 == 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/CMF/CMFRemap.c",
                        0x7b,"coefAverage != 0");
      HardwareAnalyzer_SetSoftException();
    }
    for (local_c = 0; local_c < local_res8; local_c = local_c + 1) {
      local_18 = (int)*(short *)(param_2 + (longlong)local_c * 2);
      if (local_18 == 0) {
        local_18 = 1;
      }
      local_14 = (local_10 / local_res8 << 10) / local_18;
      if (local_14 < 0x1000) {
        if ((local_14 < 0x100) && (-1 < local_14)) {
          local_14 = 0x100;
        }
        else if ((local_14 < 0) && (-0x101 < local_14)) {
          local_14 = -0x100;
        }
        else if (local_14 < -0x1000) {
          local_14 = -0x1000;
        }
      }
      else {
        local_14 = 0x1000;
      }
      *(short *)(param_3 + (longlong)local_c * 2) = (short)local_14;
      *(short *)((longlong)local_c * 2 + param_4) = (short)(0x100000 / (longlong)local_14);
    }
  }
  return;
}


// ============================================================================
// Function: CMFRemap_MapBuffer @ 6ba5103f
// ============================================================================

void CMFRemap_MapBuffer(undefined2 *param_1,undefined8 param_2,longlong param_3,longlong param_4)

{
  undefined2 *local_res8;
  ushort *local_18;
  int local_c;
  
                    /* 0x1103f  463  CMFRemap_MapBuffer */
  local_res8 = param_1;
  local_18 = *(ushort **)(param_4 + 0x38);
  local_c = (uint)*(ushort *)(param_4 + 0x2c) * (uint)*(ushort *)(param_4 + 0x2e);
  while (local_c != 0) {
    *local_res8 = *(undefined2 *)(param_3 + (ulonglong)*local_18 * 2);
    local_res8 = local_res8 + 1;
    local_18 = local_18 + 1;
    local_c = local_c + -1;
  }
  return;
}


// ============================================================================
// Function: CMFRemap_MapBufferBack @ 6ba510c5
// ============================================================================

void CMFRemap_MapBufferBack
               (undefined2 *param_1,undefined8 param_2,longlong param_3,undefined8 param_4,
               longlong param_5)

{
  undefined2 *local_res8;
  ushort *local_18;
  int local_c;
  
                    /* 0x110c5  464  CMFRemap_MapBufferBack */
  local_res8 = param_1;
  local_18 = *(ushort **)(param_5 + 0x30);
  local_c = (uint)*(ushort *)(param_5 + 0x2a) * (uint)*(ushort *)(param_5 + 0x28);
  while (local_c != 0) {
    *local_res8 = *(undefined2 *)(param_3 + (ulonglong)*local_18 * 2);
    local_res8 = local_res8 + 1;
    local_18 = local_18 + 1;
    local_c = local_c + -1;
  }
  return;
}


