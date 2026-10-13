// ============================================================================
// Function: HardwareAnalyzer_SetSoftException @ 6baf0124
// ============================================================================

void HardwareAnalyzer_SetSoftException(void)

{
                    /* 0xb0124  879  HardwareAnalyzer_SetSoftException */
  HardwareAnalyzer_SetException(3);
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_Process @ 6baf19de
// ============================================================================

void HardwareAnalyzer_Process(void)

{
  TSAPrpt *pTVar1;
  char cVar2;
  errno_t eVar3;
  
                    /* 0xb19de  870  HardwareAnalyzer_Process */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&DAT_6bb57cd2,"HardwareAnalyzer_Process");
  }
  pTVar1 = _refptr_g_tsaPrpt;
  *(undefined8 *)&_refptr_g_tsaPrpt->field_0x1c = *(undefined8 *)_refptr_g_tsaPrpt;
  *(undefined8 *)&pTVar1->field_0x24 = *(undefined8 *)&pTVar1->field_0x8;
  *(undefined8 *)&pTVar1->field_0x2c = *(undefined8 *)&pTVar1->field_0x10;
  *(undefined4 *)&pTVar1->field_0x34 = *(undefined4 *)&pTVar1->field_0x18;
  DAT_6bc8c9c4 = g_hwAnalyzer;
  g_hwAnalyzer = 0;
  HardwareAnalyzer_ClearExceptionDescription();
  HardwareAnalyzer_GetRaw2RawPrpt();
  cVar2 = TSAStatic_IsHWReset();
  if (cVar2 == '\0') {
    if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & FLAG_USE_DIF_BUF_PATH) == 0) {
      HardwareAnalyzer_SensorBrokenRawProcess();
      HardwareAnalyzer_SensorBrokenPreDifProcess();
      HardwareAnalyzer_ProcessRFNoise();
      HardwareAnalyzer_ProcessByRaw2Raw();
    }
  }
  else {
    HardwareAnalyzer_Init();
  }
  HardwareAnalyzer_ProcessProximity();
  HardwareAnalyzer_ProcessSideTouch();
  eVar3 = memcpy_s(*(void **)_refptr_g_tsaBufPreHWRaw,
                   (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,*(void **)_refptr_g_tsaBufRaw,
                   (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
  if (eVar3 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/HardwareAnalyzer/HardwareAnalyzer_Process.c",0x358,
                      "rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  HardwareAnalyzer_ReportToTSA();
  cVar2 = HardwareAnalyzer_IsSpecifiedExceptionDetected(g_hwAnalyzer,1);
  if (cVar2 == '\0') {
    (*_refptr_g_tsaStaticPtr)->wField_0x11e = 0;
  }
  else {
    (*_refptr_g_tsaStaticPtr)->wField_0x11e = (*_refptr_g_tsaStaticPtr)->wField_0x11e + 1;
    if ((*_refptr_g_tsaStaticPtr)->wField_0x120 < (*_refptr_g_tsaStaticPtr)->wField_0x11e) {
      (*_refptr_g_tsaStaticPtr)->wField_0x120 = (*_refptr_g_tsaStaticPtr)->wField_0x11e;
    }
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_ProcessDif @ 6baf1c4e
// ============================================================================

void HardwareAnalyzer_ProcessDif(void)

{
                    /* 0xb1c4e  871  HardwareAnalyzer_ProcessDif */
  HardwareAnalyzer_SensorBrokenDifProcess();
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_PostProcess @ 6baf2151
// ============================================================================

void HardwareAnalyzer_PostProcess(void)

{
  char cVar1;
  int iVar2;
  char *_Str1;
  
                    /* 0xb2151  869  HardwareAnalyzer_PostProcess */
  cVar1 = TSAStatic_IsHWReset();
  if (cVar1 != '\0') goto LAB_6baf233b;
  HardwareAnalyzer_PostProcessRaw();
  if ((*(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x20) & 0x2000) != 0) {
    HardwareAnalyzer_SetException(8);
  }
  cVar1 = SafeBaseline_IsReset();
  if ((cVar1 != '\0') && (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SAFE_BASELINE) != 0)) {
    HardwareAnalyzer_SetException(8);
  }
  if (((*(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x20) & 0x1000) != 0) &&
     (0x32 < (*_refptr_g_tsaStaticPtr)->bRecalFrameCount)) {
    HardwareAnalyzer_SetException(9);
  }
  if (((*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags & 0x400) != 0) {
    HardwareAnalyzer_SetException(10);
  }
  if (((*_refptr_g_tsaStaticPtr)->dwStylusAftGateFlags & 8) == 0) {
    _Str1 = (char *)TSA_GetPrmtName();
    iVar2 = strncmp(_Str1,"HIMA",4);
    if (iVar2 == 0) goto LAB_6baf2248;
  }
  else {
LAB_6baf2248:
    if (((_refptr_g_tsaPrpt[1].field_0x28 != '\0') &&
        ((((_refptr_g_tsaPrmtDynamic[0x80] & 4) != 0 &&
          (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 1) != 0)) ||
         (((_refptr_g_tsaPrmtDynamic[0x80] & 8) != 0 &&
          (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x10) != 0)))))) &&
       (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) == 0)) {
      HardwareAnalyzer_SetException(0xd);
      HardwareAnalyzer_AppendExceptionDescription("envTemp",(*_refptr_g_tsaStaticPtr)->nField_0x288)
      ;
      HardwareAnalyzer_AppendExceptionDescription("maxDif",(int)_refptr_g_tsaPrpt->nSPeakPositive);
    }
  }
  HardwareAnalyzer_EventAnalyzer();
  HardwareAnalyzer_PostProcessTouch();
  HardwareAnalyzer_AfeRawJump();
  HardwareAnalyzer_AfeRawUnstable();
  HardwareAnalyzer_BackLightShift();
  HardwareAnalyzer_GetCouplingNoiseLevel();
  HardwareAnalyzer_LargeDisplayNoise();
LAB_6baf233b:
  HardwareAnalyzer_ReportToTSA();
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_Reset @ 6baf2499
// ============================================================================

void HardwareAnalyzer_Reset(void)

{
                    /* 0xb2499  873  HardwareAnalyzer_Reset */
  TPSensor_Reset();
  HardwareAnalyzer_SensorBrokenReset();
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_GenerateExceptionDescription @ 6baf001a
// ============================================================================

void HardwareAnalyzer_GenerateExceptionDescription(void)

{
                    /* 0xb001a  860  HardwareAnalyzer_GenerateExceptionDescription */
  StrPrintf_Backup();
  StrPrintf_Init(HardwareAnalyzer_PutStr);
  HardwareAnalyzer_PrintBackground();
  StrPrintf_Retore();
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_GetDetectedException @ 6baf0249
// ============================================================================

int HardwareAnalyzer_GetDetectedException(void)

{
  char cVar1;
  undefined4 uVar2;
  int local_c;
  
                    /* 0xb0249  861  HardwareAnalyzer_GetDetectedException */
  uVar2 = HardwareAnalyzer_GetDetectedNewException();
  local_c = 0;
  cVar1 = HardwareAnalyzer_IsSpecifiedExceptionDetected(uVar2,6);
  if (cVar1 == '\0') {
    cVar1 = HardwareAnalyzer_IsSpecifiedExceptionDetected(uVar2,7);
    if (cVar1 == '\0') {
      cVar1 = HardwareAnalyzer_IsSpecifiedExceptionDetected(uVar2,1);
      if (cVar1 == '\0') {
        cVar1 = HardwareAnalyzer_IsSpecifiedExceptionDetected(uVar2,2);
        if (cVar1 == '\0') {
          cVar1 = HardwareAnalyzer_IsSpecifiedExceptionDetected(uVar2,0x10);
          if (cVar1 == '\0') {
            cVar1 = HardwareAnalyzer_IsSpecifiedExceptionDetected(uVar2,0xe);
            if (cVar1 == '\0') {
              cVar1 = HardwareAnalyzer_IsSpecifiedExceptionDetected(uVar2,9);
              if (cVar1 == '\0') {
                cVar1 = HardwareAnalyzer_IsSpecifiedExceptionDetected(uVar2,4);
                if (cVar1 == '\0') {
                  cVar1 = HardwareAnalyzer_IsSpecifiedExceptionDetected(uVar2,10);
                  if (cVar1 == '\0') {
                    cVar1 = HardwareAnalyzer_IsSpecifiedExceptionDetected(uVar2,5);
                    if (cVar1 == '\0') {
                      cVar1 = HardwareAnalyzer_IsSpecifiedExceptionDetected(uVar2,0xb);
                      if (cVar1 == '\0') {
                        cVar1 = HardwareAnalyzer_IsSpecifiedExceptionDetected(uVar2,8);
                        if (cVar1 == '\0') {
                          cVar1 = HardwareAnalyzer_IsSpecifiedExceptionDetected(uVar2,0xc);
                          if (cVar1 == '\0') {
                            cVar1 = HardwareAnalyzer_IsSpecifiedExceptionDetected(uVar2,0xd);
                            if (cVar1 == '\0') {
                              cVar1 = HardwareAnalyzer_IsSpecifiedExceptionDetected(uVar2,0xf);
                              if (cVar1 == '\0') {
                                cVar1 = HardwareAnalyzer_IsSpecifiedExceptionDetected(uVar2,0x11);
                                if (cVar1 == '\0') {
                                  cVar1 = HardwareAnalyzer_IsSpecifiedExceptionDetected(uVar2,0x12);
                                  if (cVar1 == '\0') {
                                    cVar1 = HardwareAnalyzer_IsSpecifiedExceptionDetected
                                                      (uVar2,0x13);
                                    if (cVar1 == '\0') {
                                      cVar1 = HardwareAnalyzer_IsSpecifiedExceptionDetected(uVar2,3)
                                      ;
                                      if (cVar1 == '\0') {
                                        cVar1 = HardwareAnalyzer_IsSpecifiedExceptionDetected
                                                          (uVar2,0x14);
                                        if (cVar1 == '\0') {
                                          cVar1 = HardwareAnalyzer_IsSpecifiedExceptionDetected
                                                            (uVar2,0x15);
                                          if (cVar1 == '\0') {
                                            cVar1 = HardwareAnalyzer_IsSpecifiedExceptionDetected
                                                              (uVar2,0x16);
                                            if (cVar1 == '\0') {
                                              cVar1 = HardwareAnalyzer_IsSpecifiedExceptionDetected
                                                                (uVar2,0x17);
                                              if (cVar1 == '\0') {
                                                cVar1 = 
                                                  HardwareAnalyzer_IsSpecifiedExceptionDetected
                                                            (uVar2,0x18);
                                                if (cVar1 == '\0') {
                                                  cVar1 = 
                                                  HardwareAnalyzer_IsSpecifiedExceptionDetected
                                                            (uVar2,0x19);
                                                  if (cVar1 == '\0') {
                                                    cVar1 = 
                                                  HardwareAnalyzer_IsSpecifiedExceptionDetected
                                                            (uVar2,0x1a);
                                                  if (cVar1 == '\0') {
                                                    cVar1 = 
                                                  HardwareAnalyzer_IsSpecifiedExceptionDetected
                                                            (uVar2,0x1b);
                                                  if (cVar1 == '\0') {
                                                    cVar1 = 
                                                  HardwareAnalyzer_IsSpecifiedExceptionDetected
                                                            (uVar2,0x1c);
                                                  if (cVar1 == '\0') {
                                                    cVar1 = 
                                                  HardwareAnalyzer_IsSpecifiedExceptionDetected
                                                            (uVar2,0x1d);
                                                  if (cVar1 == '\0') {
                                                    cVar1 = 
                                                  HardwareAnalyzer_IsSpecifiedExceptionDetected
                                                            (uVar2,0x1e);
                                                  if (cVar1 == '\0') {
                                                    cVar1 = 
                                                  HardwareAnalyzer_IsSpecifiedExceptionDetected
                                                            (uVar2,0x1f);
                                                  if (cVar1 != '\0') {
                                                    local_c = HardwareAnalyzer_CheckException(0x1f);
                                                  }
                                                  }
                                                  else {
                                                    local_c = HardwareAnalyzer_CheckException(0x1e);
                                                  }
                                                  }
                                                  else {
                                                    local_c = HardwareAnalyzer_CheckException(0x1d);
                                                  }
                                                  }
                                                  else {
                                                    local_c = HardwareAnalyzer_CheckException(0x1c);
                                                  }
                                                  }
                                                  else {
                                                    local_c = HardwareAnalyzer_CheckException(0x1b);
                                                  }
                                                  }
                                                  else {
                                                    local_c = HardwareAnalyzer_CheckException(0x1a);
                                                  }
                                                  }
                                                  else {
                                                    local_c = HardwareAnalyzer_CheckException(0x19);
                                                  }
                                                }
                                                else {
                                                  local_c = HardwareAnalyzer_CheckException(0x18);
                                                }
                                              }
                                              else {
                                                local_c = HardwareAnalyzer_CheckException(0x17);
                                              }
                                            }
                                            else {
                                              local_c = HardwareAnalyzer_CheckException(0x16);
                                            }
                                          }
                                          else {
                                            local_c = HardwareAnalyzer_CheckException(0x15);
                                          }
                                        }
                                        else {
                                          local_c = HardwareAnalyzer_CheckException(0x14);
                                        }
                                      }
                                      else {
                                        local_c = HardwareAnalyzer_CheckException(3);
                                      }
                                    }
                                    else {
                                      local_c = HardwareAnalyzer_CheckException(0x13);
                                    }
                                  }
                                  else {
                                    local_c = HardwareAnalyzer_CheckException(0x12);
                                  }
                                }
                                else {
                                  local_c = HardwareAnalyzer_CheckException(0x11);
                                }
                              }
                              else {
                                local_c = HardwareAnalyzer_CheckException(0xf);
                              }
                            }
                            else {
                              local_c = HardwareAnalyzer_CheckException(0xd);
                            }
                          }
                          else {
                            local_c = HardwareAnalyzer_CheckException(0xc);
                          }
                        }
                        else {
                          local_c = HardwareAnalyzer_CheckException(8);
                        }
                      }
                      else {
                        local_c = HardwareAnalyzer_CheckException(0xb);
                      }
                    }
                    else {
                      local_c = HardwareAnalyzer_CheckException(5);
                    }
                  }
                  else {
                    local_c = HardwareAnalyzer_CheckException(10);
                  }
                }
                else {
                  local_c = HardwareAnalyzer_CheckException(4);
                }
              }
              else {
                local_c = HardwareAnalyzer_CheckException(9);
              }
            }
            else {
              local_c = HardwareAnalyzer_CheckException(0xe);
            }
          }
          else {
            local_c = HardwareAnalyzer_CheckException(0x10);
          }
        }
        else {
          local_c = HardwareAnalyzer_CheckException(2);
        }
      }
      else {
        local_c = HardwareAnalyzer_CheckException(1);
      }
    }
    else {
      local_c = HardwareAnalyzer_CheckException(7);
    }
  }
  else {
    local_c = HardwareAnalyzer_CheckException(6);
  }
  if (local_c != 0) {
    (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags =
         (*_refptr_g_tsaStaticPtr)->dwSpecialControlFlags | 0x10;
  }
  return local_c;
}


// ============================================================================
// Function: HardwareAnalyzer_GetRaw2RawPrpt @ 6baf10dc
// ============================================================================

void HardwareAnalyzer_GetRaw2RawPrpt(void)

{
  byte bVar1;
  byte bVar2;
  short sVar3;
  ushort uVar4;
  ushort uVar5;
  ushort uVar6;
  short sVar7;
  byte bVar8;
  ushort uVar9;
  undefined2 uVar10;
  undefined4 uVar11;
  uint uVar12;
  int iVar13;
  uint local_34;
  uint local_30;
  int local_2c;
  int local_28;
  uint local_24;
  byte local_1d;
  int local_1c;
  int local_18;
  short local_14;
  short local_12;
  short local_10;
  short local_e;
  short local_c;
  short local_a;
  
                    /* 0xb10dc  863  HardwareAnalyzer_GetRaw2RawPrpt */
  local_a = 0;
  local_c = 0;
  local_e = 0;
  local_10 = 0;
  local_12 = 0x7fff;
  local_14 = -0x8000;
  local_18 = 0;
  local_1c = 0;
  sVar3 = *(short *)&_refptr_g_tsaPrpt[1].field_0x14;
  uVar4 = _refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold;
  uVar5 = _refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax;
  uVar6 = _refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold;
  uVar9 = (ushort)(byte)_refptr_g_tsaPrmtFlashConst[0x24];
  bVar1 = _refptr_g_tsaPrmtFlashConst[0x25];
  bVar2 = _refptr_g_tsaPrmtFlashConst[0x26];
  if (0x50 < uVar9) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/HardwareAnalyzer/HardwareAnalyzer_Process.c",0x242,
                      "rxNum <= TSA_MAX_DIM");
    HardwareAnalyzer_SetSoftException();
  }
  RFNoise_CopyPrevRaw2RawDif();
  BLIIR_CalcDiffCommon
            (*(short **)_refptr_g_tsaBufRaw2rawDif,*(short **)_refptr_g_tsaBufRaw,
             *(short **)_refptr_g_tsaBufPreHWRaw);
  if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
    uVar11 = 2;
  }
  else {
    uVar11 = 1;
  }
  CMF_ProcessDimCommonOnBuf
            (*(undefined8 *)_refptr_g_tsaBufRaw2rawDifAftCMF,
             *(undefined8 *)_refptr_g_tsaBufRaw2rawDif,uVar11,0x7fff,
             (int)*(short *)(_refptr_g_tsaPrmtDynamic + 0x3e),_refptr_g_tsaPrmtFlash->bCols,
             _refptr_g_tsaPrmtFlash->bRows,0);
  local_1d = 0;
  for (local_24 = 0; local_24 < bVar2; local_24 = local_24 + 1) {
    local_28 = 0;
    local_2c = 0;
    for (local_30 = 0; local_30 < bVar1; local_30 = local_30 + 1) {
      if ((byte)(&_refptr_g_tsaPrmtFlash->field_0x62c)[local_30] == local_24) {
        local_2c = local_2c + 1;
        for (local_34 = 0; local_34 < uVar9; local_34 = local_34 + 1) {
          if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == '\0') {
            uVar12 = local_30 + local_34 * _refptr_g_tsaPrmtFlash->bCols;
          }
          else {
            uVar12 = local_34 + local_30 * _refptr_g_tsaPrmtFlash->bCols;
          }
          sVar7 = *(short *)(*(longlong *)_refptr_g_tsaBufRaw2rawDif + (ulonglong)uVar12 * 2);
          uVar12 = (uint)sVar7;
          if ((int)local_14 < (int)uVar12) {
            local_14 = sVar7;
          }
          if ((int)uVar12 < (int)local_12) {
            local_12 = sVar7;
          }
          iVar13 = ((int)uVar12 >> 0x1f ^ uVar12) - ((int)uVar12 >> 0x1f);
          if (sVar3 < iVar13) {
            local_28 = local_28 + 1;
            if ((int)uVar12 < 1) {
              local_1c = local_1c + uVar12;
            }
            else {
              local_18 = local_18 + uVar12;
            }
            if ((((short)uVar5 < iVar13) && (local_10 = local_10 + 1, (short)uVar4 < iVar13)) &&
               (local_e = local_e + 1, (short)(uVar6 * 3) < iVar13)) {
              local_c = local_c + 1;
            }
          }
        }
      }
    }
    local_a = local_a + (short)local_28;
    local_2c = (uint)uVar9 * local_2c;
    if (local_2c == 0) {
      local_2c = 1;
    }
    local_2c = (local_28 * 100) / local_2c;
    bVar8 = (byte)local_2c;
    if (local_2c <= (int)(uint)local_1d) {
      bVar8 = local_1d;
    }
    local_1d = bVar8;
  }
  uVar10 = HardwareAnalyzer_GetNumOverNoiseAfterCMF();
  *(short *)_refptr_g_tsaPrpt = local_14;
  *(short *)&_refptr_g_tsaPrpt->field_0x2 = local_12;
  *(short *)&_refptr_g_tsaPrpt->field_0x4 = local_a;
  *(short *)&_refptr_g_tsaPrpt->field_0x6 = local_c;
  *(undefined2 *)&_refptr_g_tsaPrpt->field_0x8 = uVar10;
  *(short *)&_refptr_g_tsaPrpt->field_0xa = local_e;
  *(short *)&_refptr_g_tsaPrpt->field_0xc = local_10;
  *(int *)&_refptr_g_tsaPrpt->field_0x10 = local_18;
  *(int *)&_refptr_g_tsaPrpt->field_0x14 = local_1c;
  _refptr_g_tsaPrpt->field_0x18 = local_1d;
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_ReportToTSA @ 6baf159b
// ============================================================================

void HardwareAnalyzer_ReportToTSA(void)

{
  char cVar1;
  
                    /* 0xb159b  872  HardwareAnalyzer_ReportToTSA */
  cVar1 = HardwareAnalyzer_IsAnyExceptionDetected();
  if (cVar1 == '\0') {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x20) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x20) & 0xfffffbff;
  }
  else {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x20) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x20) | 0x400;
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_IsToBypassCurrentFrame @ 6baf14ff
// ============================================================================

undefined1 HardwareAnalyzer_IsToBypassCurrentFrame(void)

{
  char cVar1;
  undefined1 local_9;
  
                    /* 0xb14ff  867  HardwareAnalyzer_IsToBypassCurrentFrame */
  local_9 = 0;
  if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3) {
    cVar1 = HardwareAnalyzer_IsSpecifiedExceptionDetected(g_hwAnalyzer,2);
    if (cVar1 != '\0') {
      cVar1 = HardwareAnalyzer_IsSpecifiedExceptionDetected(g_hwAnalyzer,1);
      if (cVar1 == '\0') {
        local_9 = 1;
      }
    }
  }
  cVar1 = HardwareAnalyzer_IsSpecifiedExceptionDetected(g_hwAnalyzer,0x1d);
  if (cVar1 != '\0') {
    local_9 = 1;
  }
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags2 & 0x10) != 0) {
    cVar1 = RFNoise_IsDetected();
    if (cVar1 != '\0') {
      local_9 = 1;
    }
  }
  return local_9;
}


// ============================================================================
// Function: HardwareAnalyzer_SetException @ 6baf00f6
// ============================================================================

void HardwareAnalyzer_SetException(byte param_1)

{
                    /* 0xb00f6  878  HardwareAnalyzer_SetException */
  g_hwAnalyzer = g_hwAnalyzer | 1 << (param_1 & 0x1f);
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_ClearExceptionDescription @ 6baefab0
// ============================================================================

void HardwareAnalyzer_ClearExceptionDescription(void)

{
  int iVar1;
  
  iVar1 = memset_s(&DAT_6bc8c9c8,0x1f5,0,0x1f5);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/HardwareAnalyzer/HardwareAnalyzer_Process.c",0x42,
                      "rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  DAT_6bc8cbbe = 0;
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_Init @ 6baefb27
// ============================================================================

void HardwareAnalyzer_Init(void)

{
  int iVar1;
  
                    /* 0xafb27  864  HardwareAnalyzer_Init */
  g_hwAnalyzer = 0;
  DAT_6bc8c9c4 = 0;
  DAT_6bc8cbc0 = 0;
  DAT_6bc8cbc4 = 0;
  DAT_6bc8cbc8 = 0;
  DAT_6bc8cbc9 = 0;
  HardwareAnalyzer_ClearExceptionDescription();
  iVar1 = memset_s(&_refptr_g_tsaPrpt->field_0x38,0x58,0,0x58);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/HardwareAnalyzer/HardwareAnalyzer_Process.c",0x51,
                      "rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(*(undefined8 *)_refptr_g_tsaBufRaw2rawDif,
                   (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,0,
                   (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/HardwareAnalyzer/HardwareAnalyzer_Process.c",0x53,
                      "rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(_refptr_g_tsaPrpt,0x1c,0,0x1c);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/HardwareAnalyzer/HardwareAnalyzer_Process.c",0x55,
                      "rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(*(undefined8 *)_refptr_g_tsaBufRaw2rawDifAftCMF,
                   (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,0,
                   (ulonglong)*(ushort *)_refptr_g_gridBufSize * 2);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/HardwareAnalyzer/HardwareAnalyzer_Process.c",0x57,
                      "rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags2 & 0x10) != 0) {
    RFNoise_Init();
  }
  HardwareAnalyzer_SensorBrokenInit();
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags2 & 8) != 0) {
    BadChannel_Init();
  }
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags2 & 0x80) != 0) {
    BaliSensorBroken_Init();
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_IsSpecifiedExceptionDetected @ 6baf00d4
// ============================================================================

undefined4 HardwareAnalyzer_IsSpecifiedExceptionDetected(uint param_1,byte param_2)

{
  param_1 = 1 << (param_2 & 0x1f) & param_1;
  return CONCAT31((int3)(param_1 >> 8),param_1 != 0);
}


// ============================================================================
// Function: HardwareAnalyzer_ProcessRFNoise @ 6baf15ee
// ============================================================================

void HardwareAnalyzer_ProcessRFNoise(void)

{
  char cVar1;
  undefined1 uVar2;
  short sVar3;
  
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags2 & 0x10) != 0) {
    cVar1 = RFNoise_Detect();
    if (cVar1 != '\0') {
      HardwareAnalyzer_SetException(1);
      sVar3 = RFNoise_GetNoiseValue();
      HardwareAnalyzer_AppendExceptionDescription("RFNoiseValue",(int)sVar3);
      uVar2 = RFNoise_GetNumNoisyLine();
      HardwareAnalyzer_AppendExceptionDescription("RFNoiseNum",uVar2);
      HardwareAnalyzer_AppendExceptionDescription
                ("maxContinousNoisyFrameCnt",(*_refptr_g_tsaStaticPtr)->wField_0x120);
    }
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_ProcessProximity @ 6baf1673
// ============================================================================

void HardwareAnalyzer_ProcessProximity(void)

{
  char cVar1;
  
  cVar1 = Proximity_IsScreenOffTimeOut();
  if (cVar1 != '\0') {
    HardwareAnalyzer_SetException(0x12);
    Proximity_AppendScreenOffTimeOutDescription();
  }
  cVar1 = Proximity_IsScreenOnTimeOut();
  if (cVar1 != '\0') {
    HardwareAnalyzer_SetException(0x11);
    Proximity_AppendScreenOnTimeOutDescription();
  }
  cVar1 = Proximity_IsPowerKeyPressedToScreenOn();
  if (cVar1 != '\0') {
    HardwareAnalyzer_SetException(0x13);
    Proximity_AppendPowerKeyPressedToScreenOnDescription();
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_ProcessSideTouch @ 6baf16ca
// ============================================================================

void HardwareAnalyzer_ProcessSideTouch(void)

{
  char cVar1;
  
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) != 0) {
    cVar1 = SideTouch_IsVolumeGUIOnTimeOut();
    if (cVar1 != '\0') {
      HardwareAnalyzer_SetException(0x15);
      SideTouch_AppendVolumeGUIOnTimeOutDescription();
    }
    cVar1 = SideTouch_IsVolumeGUIOffTimeOut();
    if (cVar1 != '\0') {
      HardwareAnalyzer_SetException(0x15);
      SideTouch_AppendVolumeGUIOffTimeOutDescription();
    }
    cVar1 = SideTouch_IsAbnormalBaselineDetected();
    if (cVar1 != '\0') {
      HardwareAnalyzer_SetException(0xd);
      SideTouch_AppendAbnormalBaselineDescription();
    }
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_ProcessByRaw2Raw @ 6baf173a
// ============================================================================

void HardwareAnalyzer_ProcessByRaw2Raw(void)

{
  char cVar1;
  
  if (((*_refptr_g_prevTouchesPtr)->nTouchCount != 0x4000) &&
     ((*_refptr_g_prevTouchesPtr)->nTouchCount != 0x2000)) {
    if ((uint)_refptr_g_tsaPrmtFlash->wBufElementCount <=
        (uint)*(ushort *)&_refptr_g_tsaPrpt->field_0x6 * 100) {
      HardwareAnalyzer_SetException(0x1d);
      HardwareAnalyzer_AppendExceptionDescription
                ("numOverBigNoise",*(undefined2 *)&_refptr_g_tsaPrpt->field_0x6);
    }
    if (_refptr_g_tsaPrmtFlash->wBufElementCount >> 2 < *(ushort *)&_refptr_g_tsaPrpt->field_0xa) {
      HardwareAnalyzer_SetException(2);
      HardwareAnalyzer_AppendExceptionDescription
                ("numOverHTI",*(undefined2 *)&_refptr_g_tsaPrpt->field_0xa);
    }
    cVar1 = TSAStatic_IsChargerPlugged();
    if ((cVar1 == '\0') || (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3)) {
      if (_refptr_g_tsaPrmtFlash->wBufElementCount >> 1 < *(ushort *)&_refptr_g_tsaPrpt->field_0xc)
      {
        HardwareAnalyzer_SetException(2);
        HardwareAnalyzer_AppendExceptionDescription
                  ("numOverLTI",*(undefined2 *)&_refptr_g_tsaPrpt->field_0xc);
      }
      if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3) {
        if (((_refptr_g_tsaPrmtFlash->wBufElementCount >> 1 <
              *(ushort *)&_refptr_g_tsaPrpt->field_0x4) &&
            (*(int *)&_refptr_g_tsaPrpt->field_0x10 < *(int *)&_refptr_g_tsaPrpt->field_0x14 * -4))
           && (-*(int *)&_refptr_g_tsaPrpt->field_0x14 < *(int *)&_refptr_g_tsaPrpt->field_0x10 * 4)
           ) {
          HardwareAnalyzer_SetException(2);
          HardwareAnalyzer_AppendExceptionDescription
                    ("numOverNoise",*(undefined2 *)&_refptr_g_tsaPrpt->field_0x4);
        }
      }
      else if (((_refptr_g_tsaPrmtFlash->wBufElementCount >> 1 <
                 *(ushort *)&_refptr_g_tsaPrpt->field_0x8) &&
               (*(int *)&_refptr_g_tsaPrpt->field_0x10 < *(int *)&_refptr_g_tsaPrpt->field_0x14 * -4
               )) && (-*(int *)&_refptr_g_tsaPrpt->field_0x14 <
                      *(int *)&_refptr_g_tsaPrpt->field_0x10 * 4)) {
        HardwareAnalyzer_SetException(2);
        HardwareAnalyzer_AppendExceptionDescription
                  ("numOverNoise",*(undefined2 *)&_refptr_g_tsaPrpt->field_0x8);
      }
    }
    else if (_refptr_g_tsaPrmtFlash->wBufElementCount >> 1 <
             *(ushort *)&_refptr_g_tsaPrpt->field_0xc) {
      HardwareAnalyzer_SetException(2);
      HardwareAnalyzer_AppendExceptionDescription
                ("numOverLTI",*(undefined2 *)&_refptr_g_tsaPrpt->field_0xc);
    }
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_SensorBrokenRawProcess @ 6baf2fbe
// ============================================================================

void HardwareAnalyzer_SensorBrokenRawProcess(void)

{
  char cVar1;
  
                    /* 0xb2fbe  877  HardwareAnalyzer_SensorBrokenRawProcess */
  if (*(ushort *)&_refptr_g_tsaPrpt->field_0x8 < 5) {
    if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode != 3) {
      cVar1 = TSAStatic_IsChargerPluggedNoDebounce();
      if (cVar1 != '\0') goto LAB_6baf3114;
    }
    if (((*(short *)&_refptr_g_tsaPrmtFlash[2].field_0x5d2 != 0) &&
        (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags2 & 0x80) == 0)) &&
       (*(short *)&_refptr_g_tsaPrmtFlash[2].field_0x5d0 != 0)) {
      if (*(short *)&_refptr_g_tsaPrmtFlash[2].field_0x5d2 != 0) {
        cVar1 = HardwareAnalyzer_IsSensorBrokenByRawRange
                          (_refptr_g_tsaPrpt->field_0x9c,_refptr_g_tsaPrpt->field_0x9d);
        if (cVar1 != '\0') {
          HardwareAnalyzer_SetException(6);
          HardwareAnalyzer_AppendExceptionDescription("maxRaw",_refptr_g_tsaPrpt->wBoundaryPositive)
          ;
        }
      }
      if (*(short *)&_refptr_g_tsaPrmtFlash[2].field_0x5d0 != 0) {
        cVar1 = HardwareAnalyzer_IsSensorBrokenByRawRange
                          (_refptr_g_tsaPrpt->field_0x9e,_refptr_g_tsaPrpt->field_0x9f);
        if (cVar1 != '\0') {
          HardwareAnalyzer_SetException(6);
          HardwareAnalyzer_AppendExceptionDescription("minRaw",_refptr_g_tsaPrpt->wBoundaryNegative)
          ;
        }
      }
    }
  }
LAB_6baf3114:
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags2 & 8) != 0) {
    cVar1 = BadChannel_IsDMDDetected();
    if (cVar1 != '\0') {
      HardwareAnalyzer_SetException(6);
      HardwareAnalyzer_AppendExceptionDescription("BadChannel_IsDMDDetected",1);
    }
    BadChannel_Process();
  }
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags2 & 0x80) != 0) {
    BaliSensorBroken_Process();
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_SensorBrokenPreDifProcess @ 6baf3179
// ============================================================================

void HardwareAnalyzer_SensorBrokenPreDifProcess(void)

{
                    /* 0xb3179  876  HardwareAnalyzer_SensorBrokenPreDifProcess */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags2 & 0x80) == 0) {
    HardwareAnalyzer_BrokenSensorStatByDifZ8();
    if (_refptr_g_tsaPrpt->field_0x39 != '\0') {
      HardwareAnalyzer_SetException(7);
      HardwareAnalyzer_AppendExceptionDescription("brokenSensor detected by DIF_Z8",1);
    }
    if (_refptr_g_tsaPrpt->field_0x3a != '\0') {
      HardwareAnalyzer_SetException(7);
      HardwareAnalyzer_AppendExceptionDescription("brokenSensor isAnySensorBrokenByDif",1);
    }
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_SensorBrokenDifProcess @ 6baf3975
// ============================================================================

void HardwareAnalyzer_SensorBrokenDifProcess(void)

{
  char cVar1;
  int iVar2;
  int local_10;
  byte local_a;
  byte local_9;
  
                    /* 0xb3975  874  HardwareAnalyzer_SensorBrokenDifProcess */
  _refptr_g_tsaPrpt->field_0x3a = 0;
  iVar2 = HardwareAnalyzer_IsSafeForCheckDif();
  if (iVar2 != 0) {
    local_10 = (int)((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 2);
    if (local_10 < (_refptr_g_tsaPrpt->nSPeakPositive * 0x5a) / 100) {
      local_10 = (_refptr_g_tsaPrpt->nSPeakPositive * 0x5a) / 100;
    }
    for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bCols; local_9 = local_9 + 1) {
      for (local_a = 0; local_a < _refptr_g_tsaPrmtFlash->bRows; local_a = local_a + 1) {
        if (((-(int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold <=
              _refptr_g_tsaPrpt->nSPeakNegative * 8) &&
            (local_10 <=
             *(short *)((ulonglong)
                        ((uint)local_9 + (uint)local_a * (uint)_refptr_g_tsaPrmtFlash->bCols &
                        0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif))) &&
           (iVar2 = TPSensor_IsSensorValid(local_9,local_a), iVar2 != 0)) {
          HardwareAnalyzer_CheckCrossAround(local_9,local_a);
        }
      }
    }
    for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bCols; local_9 = local_9 + 1) {
      for (local_a = 0; local_a < _refptr_g_tsaPrmtFlash->bRows; local_a = local_a + 1) {
        cVar1 = HardwareAnalyzer_DetectType4(local_9,local_a);
        if (cVar1 != -1) {
          HardwareAnalyzer_DebounceBrokenSensorByDif
                    ((ushort)local_9 + (ushort)local_a * (ushort)_refptr_g_tsaPrmtFlash->bCols,
                     (int)cVar1);
        }
      }
    }
    HardwareAnalyzer_CopyBadChannelList();
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_AppendExceptionDescription @ 6baf0044
// ============================================================================

void HardwareAnalyzer_AppendExceptionDescription(undefined8 param_1,undefined4 param_2)

{
                    /* 0xb0044  857  HardwareAnalyzer_AppendExceptionDescription */
  StrPrintf_Backup();
  StrPrintf_Init(HardwareAnalyzer_PutStr);
  StrPrintf_Process(0,"%s[%d],",param_1,param_2);
  StrPrintf_Retore();
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_AfeRawJump @ 6baf0912
// ============================================================================

void HardwareAnalyzer_AfeRawJump(void)

{
  if (((*(uint *)&(*_refptr_g_tsaStaticPtr)->field_0xd8 & 0x20000) != 0) &&
     (((*_refptr_g_tsaStaticPtr)->nField_0xdc & 0x20000U) == 0)) {
    DAT_6bc8cbc0 = DAT_6bc8cbc0 + 1;
    if (*(int *)&(*_refptr_g_tsaStaticPtr)->field_0x3d8 == 0x10) {
      HardwareAnalyzer_SetException(0x16);
      HardwareAnalyzer_AppendExceptionDescription2
                ("RawJumpTimeAftWakeUp",
                 (longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
                 *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x80);
      HardwareAnalyzer_AppendExceptionDescription("numAfeRawJump",DAT_6bc8cbc0);
    }
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_AfeRawUnstable @ 6baf0c82
// ============================================================================

void HardwareAnalyzer_AfeRawUnstable(void)

{
  char cVar1;
  
  if (((*(uint *)&(*_refptr_g_tsaStaticPtr)->field_0xd8 & 0x40000) != 0) &&
     (((*_refptr_g_tsaStaticPtr)->nField_0xdc & 0x40000U) == 0)) {
    DAT_6bc8cbc4 = DAT_6bc8cbc4 + 1;
    DAT_6bc8cbc8 = 0;
    DAT_6bc8cbc9 = 0;
    if ((void *)(*(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x80 + 65000) <
        (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr) {
      HardwareAnalyzer_SetException(0x17);
      HardwareAnalyzer_AppendExceptionDescription2
                ("RawUnstableTimeAftWakeUp",
                 (longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
                 *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x80);
      HardwareAnalyzer_AppendExceptionDescription("numAfeRawUnstable",DAT_6bc8cbc4);
      return;
    }
  }
  if ((*(uint *)&(*_refptr_g_tsaStaticPtr)->field_0xd8 & 0x40000) == 0) {
    DAT_6bc8cbc8 = 0;
    DAT_6bc8cbc9 = 0;
  }
  else {
    cVar1 = HardwareAnalyzer_AfeRawUnstableCoreDetect();
    if (cVar1 == '\0') {
      HardwareAnalyzer_AfeRawUnstableSideDetect();
    }
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_BackLightShift @ 6baf0dd9
// ============================================================================

void HardwareAnalyzer_BackLightShift(void)

{
  char cVar1;
  
  cVar1 = ToeSynaBl_BackLightShiftUnit(0x41);
  if (((cVar1 != '\0') &&
      (_refptr_g_tsaPrmtFlash->wBufElementCount >> 2 < *(ushort *)&_refptr_g_tsaPrpt->field_0x184))
     && (*(short *)(_refptr_g_tsaPrmtDynamic + 0x46) < _refptr_g_tsaPrpt->nSPeakPositive)) {
    HardwareAnalyzer_SetException(0x1e);
    HardwareAnalyzer_AppendExceptionDescription2
              ("MaxSig",(longlong)_refptr_g_tsaPrpt->nSPeakPositive);
    HardwareAnalyzer_AppendExceptionDescription("MinSig",(int)_refptr_g_tsaPrpt->nSPeakNegative);
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_LargeDisplayNoise @ 6baf0e82
// ============================================================================

void HardwareAnalyzer_LargeDisplayNoise(void)

{
  char cVar1;
  
  if ((*_refptr_g_tsaStaticPtr)->field_0xd != '\0') {
    cVar1 = TSAStatic_IsChargerPlugged();
    if (cVar1 == '\0') {
      HardwareAnalyzer_SetException(5);
      HardwareAnalyzer_AppendExceptionDescription2
                ("MaxSig",(longlong)*(short *)&_refptr_g_tsaPrpt->field_0x160);
      HardwareAnalyzer_AppendExceptionDescription2
                ("NumOverNoise",*(undefined2 *)&_refptr_g_tsaPrpt->field_0x162);
    }
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_EventAnalyzer @ 6baf1e4e
// ============================================================================

void HardwareAnalyzer_EventAnalyzer(void)

{
  char cVar1;
  undefined4 uVar2;
  undefined4 local_c;
  
  if ((*_refptr_g_prevTouchesPtr)->field_0x5 != '\0') {
    cVar1 = TSAStatic_IsProximityEnabled();
    if (cVar1 == '\0') {
      local_c = 0x1e;
      cVar1 = TSAStatic_IsFreqShiftDone();
      if (cVar1 != '\0') {
        local_c = 0x3c;
      }
      cVar1 = EventAnalyzer_Process(1,6,0xe,0,0,0,0,0,0,local_c,0);
      if (cVar1 != '\0') {
        HardwareAnalyzer_SetException(0xe);
        uVar2 = EventAnalyzer_GetCurValue();
        HardwareAnalyzer_AppendExceptionDescription("activeTimeInterval",uVar2);
      }
    }
  }
  cVar1 = EventAnalyzer_Process(1,6,0xd,0,0,0,0,0,0,0x4b0,0);
  if (cVar1 != '\0') {
    HardwareAnalyzer_SetException(0xe);
    uVar2 = EventAnalyzer_GetCurValue();
    HardwareAnalyzer_AppendExceptionDescription("idleTimeInterval",uVar2);
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_PostProcessTouch @ 6baf1f82
// ============================================================================

void HardwareAnalyzer_PostProcessTouch(void)

{
  char cVar1;
  
  if (_refptr_g_tsaPrpt->dwBaselinePropertyState == 9) {
    HardwareAnalyzer_SetException(0x10);
  }
  HardwareAnalyzer_GetMaxEllipseRateInCore();
  if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3) {
    cVar1 = TSAStatic_IsAFTModeEnabled();
    if ((cVar1 == '\0') && ((*(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) & 0x40) != 0)) {
      HardwareAnalyzer_SetException(0x10);
      HardwareAnalyzer_AppendExceptionDescription
                ("ellipseRate",(int)*(short *)(*(longlong *)_refptr_g_touchesPtr + 0x14));
    }
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_PostProcessRaw @ 6baf2005
// ============================================================================

void HardwareAnalyzer_PostProcessRaw(void)

{
  char cVar1;
  ushort uVar2;
  undefined2 uVar3;
  
  if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & FLAG_USE_DIF_BUF_PATH) == 0) {
    cVar1 = TSAStatic_IsChargerPlugged();
    if ((cVar1 == '\0') || (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3)) {
      HardwareAnalyzer_RawLimitProcess();
    }
  }
  if ((*_refptr_g_tsaStaticPtr)->field_0x318 != '\0') {
    HardwareAnalyzer_SetException(2);
    HardwareAnalyzer_AppendExceptionDescription("isLongTimeRawJumpDetected",1);
  }
  uVar2 = TSA_GetMaxCMFNoiseValue();
  if (((int)(short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax < (int)(uint)uVar2) &&
     (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3)) {
    HardwareAnalyzer_SetException(5);
    uVar3 = TSA_GetMaxCMFNoiseValue();
    HardwareAnalyzer_AppendExceptionDescription("maxCMFNoiseValue",uVar3);
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_GetCouplingNoiseLevel @ 6baf20c4
// ============================================================================

void HardwareAnalyzer_GetCouplingNoiseLevel(void)

{
  if ((*_refptr_g_tsaStaticPtr)->bField_0x111 == '\x03') {
    if (timerCouplingNoiseDMD_11263 < 2) {
      timerCouplingNoiseDMD_11263 =
           timerCouplingNoiseDMD_11263 +
           (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
    }
    else {
      HardwareAnalyzer_SetException(0x1f);
      HardwareAnalyzer_AppendExceptionDescription
                ("CouplingNoise",*(undefined4 *)&(*_refptr_g_tsaStaticPtr)[1].field_0x32);
    }
  }
  else {
    timerCouplingNoiseDMD_11263 = 0;
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_SensorBrokenReset @ 6baf2420
// ============================================================================

void HardwareAnalyzer_SensorBrokenReset(void)

{
  int iVar1;
  
  iVar1 = memset_s(g_brokenSensorByDifSensorChecked,*(undefined2 *)_refptr_g_gridBufSize,0,
                   *(undefined2 *)_refptr_g_gridBufSize);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/HardwareAnalyzer/HardwareAnalyzer_SensorBroken.c",0x4a,
                      "rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_PrintBackground @ 6baefe52
// ============================================================================

void HardwareAnalyzer_PrintBackground(void)

{
  undefined1 uVar1;
  
  StrPrintf_Process(0,"currHWException[0x%x],",g_hwAnalyzer);
  StrPrintf_Process(0,"prevHWException[0x%x],",DAT_6bc8c9c4);
  StrPrintf_Process(0,"Time[%lld],",(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr);
  StrPrintf_Process(0,"Ctrl[0x%x],",(*_refptr_g_tsaStaticPtr)->dwSystemFlags);
  StrPrintf_Process(0,"Feature[0x%x],",(*_refptr_g_tsaStaticPtr)->dwFeatureFlags);
  StrPrintf_Process(0,"Exception[0x%x],",*(undefined4 *)(*(longlong *)_refptr_g_touchesPtr + 0x20));
  StrPrintf_Process(0,"MaxDif[%d],",(int)_refptr_g_tsaPrpt->nSPeakPositive);
  StrPrintf_Process(0,"MinDif[%d],",(int)_refptr_g_tsaPrpt->nSPeakNegative);
  StrPrintf_Process(0,"MaxRaw2RawDif[%d],",(int)*(short *)_refptr_g_tsaPrpt);
  StrPrintf_Process(0,"MinRaw2RawDif[%d],",(int)*(short *)&_refptr_g_tsaPrpt->field_0x2);
  uVar1 = TSA_RptTouchNum();
  StrPrintf_Process(0,"TouchNum[%d],",uVar1);
  uVar1 = TSA_RptLONum();
  StrPrintf_Process(0,"LONum[%d],",uVar1);
  StrPrintf_Process(0,"numBrokenSensor[%d]",_refptr_g_tsaPrpt->field_0x38);
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_PutStr @ 6baefd87
// ============================================================================

void HardwareAnalyzer_PutStr(longlong param_1)

{
  uint uVar1;
  ushort local_a;
  
  if (param_1 != 0) {
    local_a = 0;
    while ((*(char *)(param_1 + (ulonglong)local_a) != '\0' && (DAT_6bc8cbbe < 500))) {
      uVar1 = (uint)DAT_6bc8cbbe;
      DAT_6bc8cbbe = DAT_6bc8cbbe + 1;
      (&DAT_6bc8c9c8)[(int)uVar1] = *(undefined1 *)(param_1 + (ulonglong)local_a);
      local_a = local_a + 1;
    }
    if (DAT_6bc8cbbe < 0x1f5) {
      (&DAT_6bc8c9c8)[(int)(uint)DAT_6bc8cbbe] = 0;
    }
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_GetDetectedNewException @ 6baf013d
// ============================================================================

uint HardwareAnalyzer_GetDetectedNewException(void)

{
  return g_hwAnalyzer & ~DAT_6bc8c9c4;
}


// ============================================================================
// Function: HardwareAnalyzer_CheckException @ 6baf0182
// ============================================================================

undefined4 HardwareAnalyzer_CheckException(undefined4 param_1)

{
  byte bVar1;
  undefined4 local_c;
  
  local_c = 0;
  bVar1 = (char)param_1 - 1;
  if (bVar1 < 0x1f) {
    (&g_cntRepeat)[(int)(uint)bVar1] = (&g_cntRepeat)[(int)(uint)bVar1] + '\x01';
    if ((byte)(&g_numRepeatToReport)[(int)(uint)bVar1] <= (byte)(&g_cntRepeat)[(int)(uint)bVar1]) {
      (&g_cntRepeat)[(int)(uint)bVar1] = 0;
      local_c = param_1;
    }
  }
  else if (0x1e < bVar1) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/HardwareAnalyzer/HardwareAnalyzer_Process.c",0xf3,
                      "index < HARDWARE_EXCEPTION_LENGTH");
    HardwareAnalyzer_SetSoftException();
  }
  return local_c;
}


// ============================================================================
// Function: HardwareAnalyzer_GetNumOverNoiseAfterCMF @ 6baf101f
// ============================================================================

short HardwareAnalyzer_GetNumOverNoiseAfterCMF(void)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  uint local_10;
  short local_c;
  short local_a;
  
  local_a = 0;
  iVar1 = CMF_GetCmfMethod();
  if (iVar1 == 1) {
    local_c = *(short *)&_refptr_g_tsaPrpt[1].field_0x14;
    if (local_c < 200) {
      local_c = 200;
    }
    for (local_10 = 0; local_10 < _refptr_g_tsaPrmtFlash->wBufElementCount; local_10 = local_10 + 1)
    {
      if (((*(byte *)((ulonglong)local_10 + *(longlong *)_refptr_g_tpSensorFlag) & 0x20) == 0) &&
         (uVar2 = (uint)*(short *)(*(longlong *)_refptr_g_tsaBufRaw2rawDifAftCMF +
                                  (ulonglong)local_10 * 2), uVar3 = (int)uVar2 >> 0x1f,
         (int)local_c < (int)((uVar2 ^ uVar3) - uVar3))) {
        local_a = local_a + 1;
      }
    }
  }
  else {
    local_a = HardwareAnalyzer_GetNumOverNoiseAfterCMFNoneSOF();
  }
  return local_a;
}


// ============================================================================
// Function: HardwareAnalyzer_IsAnyExceptionDetected @ 6baf016a
// ============================================================================

bool HardwareAnalyzer_IsAnyExceptionDetected(void)

{
  int iVar1;
  
                    /* 0xb016a  865  HardwareAnalyzer_IsAnyExceptionDetected */
  iVar1 = HardwareAnalyzer_GetDetectedException();
  return iVar1 != 0;
}


// ============================================================================
// Function: HardwareAnalyzer_SensorBrokenInit @ 6baf3755
// ============================================================================

void HardwareAnalyzer_SensorBrokenInit(void)

{
  int local_c;
  
                    /* 0xb3755  875  HardwareAnalyzer_SensorBrokenInit */
  HardwareAnalyzer_SensorBrokenReset();
  if (g_brokenSensorInitOnce == '\0') {
    g_brokenSensorInitOnce = '\x01';
    for (local_c = 0; local_c < (int)(uint)_refptr_g_tsaPrmtFlash->wBufElementCount;
        local_c = local_c + 1) {
      *(undefined1 *)(local_c + g_brokenSensorByDifDebounce) = 0;
      *(undefined1 *)(local_c + g_brokenSensorByDifState) = 0;
      *(byte *)(*(longlong *)_refptr_g_tpSensorFlag + (longlong)local_c) =
           *(byte *)((longlong)local_c + *(longlong *)_refptr_g_tpSensorFlag) & 0xf0;
    }
  }
  _refptr_g_tsaPrpt->field_0x3a = 0;
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_IsSensorBrokenByRawRange @ 6baf2f76
// ============================================================================

undefined8 HardwareAnalyzer_IsSensorBrokenByRawRange(undefined1 param_1,undefined1 param_2)

{
  char cVar1;
  
  cVar1 = HardwareAnalyzer_IsSensorOverRawLimit(param_1,param_2);
  if ((cVar1 != '\0') &&
     (cVar1 = HardwareAnalyzer_IsSensorOverRawDeltaLimit(param_1,param_2), cVar1 != '\0')) {
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: HardwareAnalyzer_BrokenSensorStatByDifZ8 @ 6baf2a26
// ============================================================================

void HardwareAnalyzer_BrokenSensorStatByDifZ8(void)

{
  uchar uVar1;
  undefined1 uVar2;
  undefined8 uVar3;
  TSAPrpt *pTVar4;
  char cVar5;
  char local_a;
  char local_9;
  
  _refptr_g_tsaPrpt->field_0x39 = 0;
  uVar1 = _refptr_g_tsaPrpt->bSpecialThreshCol;
  uVar2 = _refptr_g_tsaPrpt->field_0xa5;
  cVar5 = HardwareAnalyzer_IsSensorBrokenByDifZ8(uVar1,uVar2);
  if (cVar5 != '\0') {
    HardwareAnalyzer_DebounceBrokenSensorByDifZ8(uVar1,uVar2);
  }
  uVar1 = _refptr_g_tsaPrpt->bNextSideThreshCol;
  uVar2 = _refptr_g_tsaPrpt->field_0xa7;
  cVar5 = HardwareAnalyzer_IsSensorBrokenByDifZ8(uVar1,uVar2);
  if (cVar5 != '\0') {
    HardwareAnalyzer_DebounceBrokenSensorByDifZ8(uVar1,uVar2);
  }
  local_9 = _refptr_g_tsaPrpt->field_0x38;
  while (local_9 = local_9 + -1, -1 < local_9) {
    if ((0 < *(longlong *)(&_refptr_g_tsaPrpt->field_0x48 + (longlong)(int)local_9 * 0x10)) &&
       (*(longlong *)(&_refptr_g_tsaPrpt->field_0x48 + (longlong)(int)local_9 * 0x10) != 5000)) {
      *(longlong *)(&_refptr_g_tsaPrpt->field_0x48 + (longlong)(int)local_9 * 0x10) =
           *(longlong *)(&_refptr_g_tsaPrpt->field_0x48 + (longlong)(int)local_9 * 0x10) -
           (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
    }
    if (*(longlong *)(&_refptr_g_tsaPrpt->field_0x48 + (longlong)(int)local_9 * 0x10) < 1) {
      for (local_a = local_9; pTVar4 = _refptr_g_tsaPrpt,
          (int)local_a < (int)((byte)_refptr_g_tsaPrpt->field_0x38 - 1); local_a = local_a + '\x01')
      {
        uVar3 = *(undefined8 *)(&_refptr_g_tsaPrpt->field_0x48 + (longlong)(local_a + 1) * 0x10);
        *(undefined8 *)(&_refptr_g_tsaPrpt->field_0x40 + (longlong)(int)local_a * 0x10) =
             *(undefined8 *)(&_refptr_g_tsaPrpt->field_0x40 + (longlong)(local_a + 1) * 0x10);
        *(undefined8 *)(&pTVar4->field_0x48 + (longlong)(int)local_a * 0x10) = uVar3;
      }
      _refptr_g_tsaPrpt->field_0x38 = _refptr_g_tsaPrpt->field_0x38 + -1;
    }
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_IsSafeForCheckDif @ 6baf31fc
// ============================================================================

undefined8 HardwareAnalyzer_IsSafeForCheckDif(void)

{
  int iVar1;
  undefined8 uVar2;
  uint uVar3;
  
  if (_refptr_g_tsaPrmtFlash[2].field_0x5dc == '\0') {
    uVar2 = 0;
  }
  else if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 3) {
    if ((*(int *)(*(longlong *)_refptr_g_touchesPtr + 0x20) == 0) ||
       (*(int *)(*(longlong *)_refptr_g_touchesPtr + 0x20) == 0x400)) {
      iVar1 = (int)((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold / 0x14);
      uVar3 = (int)*(short *)&_refptr_g_tsaPrpt[1].field_0x44 >> 0x1f;
      if (iVar1 < (int)(((int)*(short *)&_refptr_g_tsaPrpt[1].field_0x44 ^ uVar3) - uVar3)) {
        uVar2 = 0;
      }
      else {
        uVar3 = (int)*(short *)&_refptr_g_tsaPrpt[1].field_0x46 >> 0x1f;
        if (iVar1 < (int)(((int)*(short *)&_refptr_g_tsaPrpt[1].field_0x46 ^ uVar3) - uVar3)) {
          uVar2 = 0;
        }
        else if ((_refptr_g_tsaPrpt->dwBaselinePropertyFlags & 0x198) == 0) {
          uVar2 = 1;
        }
        else {
          uVar2 = 0;
        }
      }
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
// Function: HardwareAnalyzer_DebounceBrokenSensorByDif @ 6baf354a
// ============================================================================

void HardwareAnalyzer_DebounceBrokenSensorByDif(int param_1,int param_2)

{
  if (*(char *)(param_1 + g_brokenSensorByDifState) == '\0') {
    if (param_2 == 0) {
      *(undefined1 *)(param_1 + g_brokenSensorByDifDebounce) = 0;
    }
    else {
      *(char *)(param_1 + g_brokenSensorByDifDebounce) =
           *(char *)(param_1 + g_brokenSensorByDifDebounce) + '\x01';
      if ('\x04' < *(char *)(param_1 + g_brokenSensorByDifDebounce)) {
        *(undefined1 *)(param_1 + g_brokenSensorByDifState) = 1;
        *(undefined1 *)(param_1 + g_brokenSensorByDifDebounce) = 0;
      }
    }
  }
  else if (param_2 == 1) {
    *(undefined1 *)(param_1 + g_brokenSensorByDifDebounce) = 0;
  }
  else {
    *(char *)(param_1 + g_brokenSensorByDifDebounce) =
         *(char *)(param_1 + g_brokenSensorByDifDebounce) + '\x01';
    if ('\x04' < *(char *)(param_1 + g_brokenSensorByDifDebounce)) {
      *(undefined1 *)(param_1 + g_brokenSensorByDifState) = 0;
      *(undefined1 *)(param_1 + g_brokenSensorByDifDebounce) = 0;
    }
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_CheckCrossAround @ 6baf364b
// ============================================================================

void HardwareAnalyzer_CheckCrossAround(int param_1,int param_2)

{
  char cVar1;
  ushort uVar2;
  uint uVar3;
  uint uVar4;
  int local_c;
  
  for (local_c = 0; local_c < 4; local_c = local_c + 1) {
    uVar3 = param_1 + *(int *)(&g_stepDelta + (longlong)local_c * 8);
    uVar4 = param_2 + *(int *)(&DAT_6bb57d84 + (longlong)local_c * 8);
    if ((((byte)uVar3 < _refptr_g_tsaPrmtFlash->bCols) &&
        ((byte)uVar4 < _refptr_g_tsaPrmtFlash->bRows)) &&
       (uVar2 = (ushort)_refptr_g_tsaPrmtFlash->bCols * (short)uVar4 + (short)uVar3,
       *(char *)((ulonglong)uVar2 + g_brokenSensorByDifSensorChecked) == '\0')) {
      *(undefined1 *)((ulonglong)uVar2 + g_brokenSensorByDifSensorChecked) = 1;
      cVar1 = HardwareAnalyzer_DetectType3(uVar3 & 0xff,uVar4 & 0xff);
      if (cVar1 != -1) {
        HardwareAnalyzer_DebounceBrokenSensorByDif(uVar2,(int)cVar1);
      }
    }
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_DetectType4 @ 6baf37f5
// ============================================================================

undefined1 HardwareAnalyzer_DetectType4(byte param_1,byte param_2)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  undefined1 local_9;
  
  uVar1 = (uint)*(short *)((ulonglong)
                           ((uint)param_1 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols &
                           0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif);
  uVar2 = (int)uVar1 >> 0x1f;
  iVar4 = (uVar2 ^ uVar1) - uVar2;
  iVar3 = (int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold;
  if (iVar3 < iVar4 * 5) {
    if (iVar3 < iVar4 * 2) {
      if (iVar3 * 6 < iVar4 * 5) {
        if (iVar3 * 2 < iVar4) {
          local_9 = 1;
        }
        else {
          local_9 = 0xff;
        }
      }
      else {
        local_9 = 0;
      }
    }
    else {
      local_9 = 0xff;
    }
  }
  else {
    local_9 = 0xff;
  }
  return local_9;
}


// ============================================================================
// Function: HardwareAnalyzer_CopyBadChannelList @ 6baf38c1
// ============================================================================

void HardwareAnalyzer_CopyBadChannelList(void)

{
  int iVar1;
  byte local_a;
  byte local_9;
  
  local_9 = 0;
  do {
    if (_refptr_g_tsaPrmtFlash->bRows <= local_9) {
      return;
    }
    for (local_a = 0; local_a < _refptr_g_tsaPrmtFlash->bCols; local_a = local_a + 1) {
      if (*(char *)((int)((uint)local_a + (uint)local_9 * (uint)_refptr_g_tsaPrmtFlash->bCols &
                         0xffff) + g_brokenSensorByDifState) == '\x01') {
        _refptr_g_tsaPrpt->field_0x3a = 1;
        iVar1 = TPSensor_AddToBrokenSensorList(local_a,local_9,4);
        if (iVar1 == 0) break;
      }
    }
    local_9 = local_9 + 1;
  } while( true );
}


// ============================================================================
// Function: HardwareAnalyzer_AppendExceptionDescription2 @ 6baf008b
// ============================================================================

void HardwareAnalyzer_AppendExceptionDescription2(undefined8 param_1,undefined8 param_2)

{
                    /* 0xb008b  858  HardwareAnalyzer_AppendExceptionDescription2 */
  StrPrintf_Backup();
  StrPrintf_Init(HardwareAnalyzer_PutStr);
  StrPrintf_Process(0,"%s[%ld],",param_1,param_2);
  StrPrintf_Retore();
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_AfeRawUnstableCoreDetect @ 6baf09e9
// ============================================================================

undefined8 HardwareAnalyzer_AfeRawUnstableCoreDetect(void)

{
  undefined2 uVar1;
  bool bVar2;
  undefined8 uVar3;
  ushort local_12;
  short local_c;
  
  bVar2 = false;
  local_c = 0;
  uVar1 = 0;
  local_12 = 0;
  do {
    if (**(byte **)_refptr_g_touchesPtr <= local_12) {
LAB_6baf0ae2:
      if (DAT_6bc8cbc8 < 5) {
        if (bVar2) {
          DAT_6bc8cbc8 = DAT_6bc8cbc8 + 1;
        }
        else {
          DAT_6bc8cbc8 = 0;
        }
        uVar3 = 0;
      }
      else {
        HardwareAnalyzer_SetException(0x17);
        HardwareAnalyzer_AppendExceptionDescription2
                  ("RawUnstableTimeAftWakeUp",
                   (longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
                   *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x80);
        HardwareAnalyzer_AppendExceptionDescription("numAfeRawUnstable",DAT_6bc8cbc4);
        HardwareAnalyzer_AppendExceptionDescription("touchEllipseRate",(int)local_c);
        HardwareAnalyzer_AppendExceptionDescription("touchAge",uVar1);
        uVar3 = 1;
      }
      return uVar3;
    }
    if (((*(uint *)((longlong)(int)(uint)local_12 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                   0x1f8) & 0x20000000) != 0) &&
       (*(short *)((longlong)(int)(uint)local_12 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1a0
                  ) < 0xc)) {
      uVar1 = *(undefined2 *)
               ((longlong)(int)(uint)local_12 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x156);
      local_c = *(short *)((longlong)(int)(uint)local_12 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x1a0);
      bVar2 = true;
      goto LAB_6baf0ae2;
    }
    local_12 = local_12 + 1;
  } while( true );
}


// ============================================================================
// Function: HardwareAnalyzer_AfeRawUnstableSideDetect @ 6baf0bc1
// ============================================================================

undefined8 HardwareAnalyzer_AfeRawUnstableSideDetect(void)

{
  char cVar1;
  undefined8 uVar2;
  
  if (DAT_6bc8cbc9 < 5) {
    cVar1 = SideTouch_IsPrevTouchDetected();
    if (cVar1 == '\0') {
      DAT_6bc8cbc9 = 0;
    }
    else {
      DAT_6bc8cbc9 = DAT_6bc8cbc9 + 1;
    }
    uVar2 = 0;
  }
  else {
    HardwareAnalyzer_SetException(0x17);
    HardwareAnalyzer_AppendExceptionDescription2
              ("RawUnstableTimeAftWakeUp",
               (longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
               *(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x80);
    HardwareAnalyzer_AppendExceptionDescription("numAfeRawUnstable",DAT_6bc8cbc4);
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: HardwareAnalyzer_GetMaxEllipseRateInCore @ 6baf1c62
// ============================================================================

void HardwareAnalyzer_GetMaxEllipseRateInCore(void)

{
  int local_c;
  
  *(undefined2 *)(*(longlong *)_refptr_g_touchesPtr + 0x14) = 0;
  *(undefined1 *)(*(longlong *)_refptr_g_touchesPtr + 7) = 0;
  for (local_c = 0; local_c < (int)(uint)**(byte **)_refptr_g_touchesPtr; local_c = local_c + 1) {
    if ((((*(char *)((longlong)local_c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1ae) == '\0')
         && (*(char *)((longlong)local_c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1af) ==
             '\0')) &&
        (*(int *)((longlong)local_c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1e0) != 3)) &&
       ((*(uint *)((longlong)local_c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1f8) & 8) != 0)
       ) {
      if (*(short *)(*(longlong *)_refptr_g_touchesPtr + 0x14) <
          *(short *)((longlong)local_c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1a0)) {
        *(undefined2 *)(*(longlong *)_refptr_g_touchesPtr + 0x14) =
             *(undefined2 *)((longlong)local_c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1a0);
      }
      *(char *)(*(longlong *)_refptr_g_touchesPtr + 7) =
           *(char *)(*(longlong *)_refptr_g_touchesPtr + 7) + '\x01';
    }
  }
  if (*(short *)(*(longlong *)_refptr_g_touchesPtr + 0x14) < 0x15) {
    timerEllipseRateDMD_11249 = 0;
  }
  else if (timerEllipseRateDMD_11249 < 0x19) {
    timerEllipseRateDMD_11249 =
         timerEllipseRateDMD_11249 +
         (longlong)(*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal;
  }
  else {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x24) | 0x40;
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_RawLimitProcess @ 6baf070a
// ============================================================================

void HardwareAnalyzer_RawLimitProcess(void)

{
  short sVar1;
  ushort uVar2;
  
  sVar1 = *(short *)&_refptr_g_tsaPrmtFlash->field_0x4a;
  HardwareAnalyzer_IsSpecifiedExceptionDetected(DAT_6bc8c9c4,4);
  if ((int)(uint)(ushort)(sVar1 * 2) <
      (int)((uint)*(ushort *)&_refptr_g_tsaPrpt->field_0x96 -
           (uint)*(ushort *)&_refptr_g_tsaPrpt->field_0x98)) {
    HardwareAnalyzer_SetException(4);
    HardwareAnalyzer_AppendExceptionDescription
              ("rawVariation",
               (uint)*(ushort *)&_refptr_g_tsaPrpt->field_0x96 -
               (uint)*(ushort *)&_refptr_g_tsaPrpt->field_0x98);
  }
  uVar2 = *(ushort *)&_refptr_g_tsaPrmtFlash->field_0x4a;
  HardwareAnalyzer_IsSpecifiedExceptionDetected(DAT_6bc8c9c4,4);
  if ((uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x48 +
      (((uint)uVar2 * 2 + (uint)uVar2) / 2 & 0xffff) <
      (uint)*(ushort *)&_refptr_g_tsaPrpt->field_0x96) {
    HardwareAnalyzer_SetException(4);
    HardwareAnalyzer_AppendExceptionDescription
              ("maxRaw",*(undefined2 *)&_refptr_g_tsaPrpt->field_0x96);
  }
  uVar2 = *(ushort *)&_refptr_g_tsaPrmtFlash->field_0x4a;
  HardwareAnalyzer_IsSpecifiedExceptionDetected(DAT_6bc8c9c4,4);
  if ((int)(uint)*(ushort *)&_refptr_g_tsaPrpt->field_0x98 <
      (int)((uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x48 -
           (((uint)uVar2 * 2 + (uint)uVar2) / 2 & 0xffff))) {
    HardwareAnalyzer_SetException(4);
    HardwareAnalyzer_AppendExceptionDescription
              ("minRaw",*(undefined2 *)&_refptr_g_tsaPrpt->field_0x98);
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_GetNumOverNoiseAfterCMFNoneSOF @ 6baf0ef7
// ============================================================================

short HardwareAnalyzer_GetNumOverNoiseAfterCMFNoneSOF(void)

{
  uint uVar1;
  uint uVar2;
  int local_1c;
  uint local_18;
  uint local_14;
  uint local_10;
  short local_a;
  
  local_a = 0;
  if (_refptr_g_tsaPrmtFlash->dwPeakProcessingMode == 2) {
    local_18 = 0;
    for (local_14 = 0; local_14 < _refptr_g_tsaPrmtFlash->bRows; local_14 = local_14 + 1) {
      for (local_10 = 0; local_10 < _refptr_g_tsaPrmtFlash->bCols; local_10 = local_10 + 1) {
        if ((local_10 == 0) || (_refptr_g_tsaPrmtFlash->bCols - 1 == local_10)) {
          local_1c = (*(short *)&_refptr_g_tsaPrpt[1].field_0x14 * 4) / 3;
        }
        else {
          local_1c = (int)*(short *)&_refptr_g_tsaPrpt[1].field_0x14;
        }
        uVar1 = (uint)*(short *)(*(longlong *)_refptr_g_tsaBufRaw2rawDifAftCMF +
                                (ulonglong)local_18 * 2);
        uVar2 = (int)uVar1 >> 0x1f;
        if ((local_1c < (int)((uVar1 ^ uVar2) - uVar2)) &&
           ((*(byte *)((ulonglong)local_18 + *(longlong *)_refptr_g_tpSensorFlag) & 0x20) == 0)) {
          local_a = local_a + 1;
        }
        local_18 = local_18 + 1;
      }
    }
  }
  else {
    local_a = 0;
  }
  return local_a;
}


// ============================================================================
// Function: HardwareAnalyzer_IsSensorOverRawLimit @ 6baf2c5a
// ============================================================================

bool HardwareAnalyzer_IsSensorOverRawLimit(byte param_1,byte param_2)

{
  ushort uVar1;
  bool local_9;
  
  uVar1 = *(ushort *)
           ((ulonglong)
            ((uint)param_1 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
           *(longlong *)_refptr_g_tsaBufRaw);
  local_9 = uVar1 < *(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x5c) ||
            *(ushort *)(_refptr_g_tsaPrmtFlashConst + 0x5a) < uVar1;
  return local_9;
}


// ============================================================================
// Function: HardwareAnalyzer_IsSensorOverRawDeltaLimit @ 6baf2cd7
// ============================================================================

bool HardwareAnalyzer_IsSensorOverRawDeltaLimit(byte param_1,byte param_2)

{
  short sVar1;
  short sVar2;
  bool local_9;
  
  sVar2 = *(short *)((ulonglong)
                     ((uint)param_1 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)
                     * 2 + *(longlong *)_refptr_g_tsaBufRaw);
  local_9 = false;
  if ((((byte)(param_1 - 1) < _refptr_g_tsaPrmtFlash->bCols) &&
      (param_2 < _refptr_g_tsaPrmtFlash->bRows)) &&
     (sVar1 = sVar2 - *(short *)((ulonglong)
                                 (((uint)param_1 +
                                  (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols) - 1 & 0xffff)
                                 * 2 + *(longlong *)_refptr_g_tsaBufRaw),
     local_9 = *(short *)(_refptr_g_tsaPrmtFlashConst + 0x5e) < sVar1,
     sVar1 < *(short *)(_refptr_g_tsaPrmtFlashConst + 0x60))) {
    local_9 = true;
  }
  if (((byte)(param_1 + 1) < _refptr_g_tsaPrmtFlash->bCols) &&
     (param_2 < _refptr_g_tsaPrmtFlash->bRows)) {
    sVar1 = sVar2 - *(short *)((ulonglong)
                               ((uint)param_1 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols
                                + 1 & 0xffff) * 2 + *(longlong *)_refptr_g_tsaBufRaw);
    if (*(short *)(_refptr_g_tsaPrmtFlashConst + 0x5e) < sVar1) {
      local_9 = true;
    }
    if (sVar1 < *(short *)(_refptr_g_tsaPrmtFlashConst + 0x60)) {
      local_9 = true;
    }
  }
  if ((param_1 < _refptr_g_tsaPrmtFlash->bCols) &&
     ((byte)(param_2 - 1) < _refptr_g_tsaPrmtFlash->bRows)) {
    sVar1 = sVar2 - *(short *)((ulonglong)
                               ((uint)param_1 + (param_2 - 1) * (uint)_refptr_g_tsaPrmtFlash->bCols
                               & 0xffff) * 2 + *(longlong *)_refptr_g_tsaBufRaw);
    if (*(short *)(_refptr_g_tsaPrmtFlashConst + 0x5e) < sVar1) {
      local_9 = true;
    }
    if (sVar1 < *(short *)(_refptr_g_tsaPrmtFlashConst + 0x60)) {
      local_9 = true;
    }
  }
  if ((param_1 < _refptr_g_tsaPrmtFlash->bCols) &&
     ((byte)(param_2 + 1) < _refptr_g_tsaPrmtFlash->bRows)) {
    sVar2 = sVar2 - *(short *)((ulonglong)
                               ((uint)param_1 + (param_2 + 1) * (uint)_refptr_g_tsaPrmtFlash->bCols
                               & 0xffff) * 2 + *(longlong *)_refptr_g_tsaBufRaw);
    if (*(short *)(_refptr_g_tsaPrmtFlashConst + 0x5e) < sVar2) {
      local_9 = true;
    }
    if (sVar2 < *(short *)(_refptr_g_tsaPrmtFlashConst + 0x60)) {
      local_9 = true;
    }
  }
  return local_9;
}


// ============================================================================
// Function: HardwareAnalyzer_IsSensorBrokenByDifZ8 @ 6baf258e
// ============================================================================

undefined1 HardwareAnalyzer_IsSensorBrokenByDifZ8(byte param_1,byte param_2)

{
  char cVar1;
  uint uVar2;
  uint uVar3;
  uint local_24;
  int local_20;
  byte local_19 [5];
  int local_14;
  uint local_10;
  undefined1 local_9;
  
  local_9 = 0;
  local_19[0] = 0;
  local_20 = 0;
  uVar3 = (uint)*(short *)((ulonglong)
                           ((uint)param_1 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols &
                           0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif);
  uVar2 = (int)uVar3 >> 0x1f;
  local_14 = (uVar3 ^ uVar2) - uVar2;
  local_24 = 0;
  if ((short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold < local_14) {
    HardwareAnalyzer_Z8InfoUpdate(param_1 - 1,param_2 - 1,local_19,&local_24,&local_20);
    HardwareAnalyzer_Z8InfoUpdate(param_1 - 1,param_2,local_19,&local_24,&local_20);
    HardwareAnalyzer_Z8InfoUpdate(param_1 - 1,param_2 + 1,local_19,&local_24,&local_20);
    HardwareAnalyzer_Z8InfoUpdate(param_1,param_2 - 1,local_19,&local_24,&local_20);
    HardwareAnalyzer_Z8InfoUpdate(param_1,param_2 + 1,local_19,&local_24,&local_20);
    HardwareAnalyzer_Z8InfoUpdate(param_1 + 1,param_2 - 1,local_19,&local_24,&local_20);
    HardwareAnalyzer_Z8InfoUpdate(param_1 + 1,param_2,local_19,&local_24,&local_20);
    HardwareAnalyzer_Z8InfoUpdate(param_1 + 1,param_2 + 1,local_19,&local_24,&local_20);
    if (local_19[0] == 0) {
      local_10 = 0;
    }
    else {
      local_10 = local_24 / local_19[0];
    }
    if (local_20 * 10 < local_14) {
      cVar1 = AFE_IsRawUnifiedToTargetValue();
      if (cVar1 == '\0') {
        local_9 = 1;
      }
      else {
        local_10 = *(ushort *)
                    ((ulonglong)
                     ((uint)param_1 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff)
                     * 2 + *(longlong *)_refptr_g_tsaBufRaw) - local_10;
        uVar2 = (int)local_10 >> 0x1f;
        if (local_14 / 2 < (int)((uVar2 ^ local_10) - uVar2)) {
          local_9 = 1;
        }
      }
    }
  }
  else {
    local_9 = 0;
  }
  return local_9;
}


// ============================================================================
// Function: HardwareAnalyzer_DebounceBrokenSensorByDifZ8 @ 6baf2804
// ============================================================================

void HardwareAnalyzer_DebounceBrokenSensorByDifZ8(char param_1,char param_2)

{
  bool bVar1;
  byte local_9;
  
  bVar1 = false;
  for (local_9 = 0; local_9 < (byte)_refptr_g_tsaPrpt->field_0x38; local_9 = local_9 + 1) {
    if (((&_refptr_g_tsaPrpt->field_0x40)[(longlong)(int)(uint)local_9 * 0x10] == param_1) &&
       ((&_refptr_g_tsaPrpt->field_0x41)[(longlong)(int)(uint)local_9 * 0x10] == param_2)) {
      bVar1 = true;
      if (*(longlong *)(&_refptr_g_tsaPrpt->field_0x48 + (longlong)(int)(uint)local_9 * 0x10) <
          0x683) {
        *(longlong *)(&_refptr_g_tsaPrpt->field_0x48 + (longlong)(int)(uint)local_9 * 0x10) =
             *(longlong *)(&_refptr_g_tsaPrpt->field_0x48 + (longlong)(int)(uint)local_9 * 0x10) +
             (longlong)((*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal * 2);
      }
      else if (*(longlong *)(&_refptr_g_tsaPrpt->field_0x48 + (longlong)(int)(uint)local_9 * 0x10)
               != 5000) {
        *(undefined8 *)(&_refptr_g_tsaPrpt->field_0x48 + (longlong)(int)(uint)local_9 * 0x10) = 5000
        ;
        _refptr_g_tsaPrpt->field_0x39 = 1;
      }
    }
  }
  if ((!bVar1) && ((byte)_refptr_g_tsaPrpt->field_0x38 < 5)) {
    *(longlong *)
     (&_refptr_g_tsaPrpt->field_0x48 +
     (longlong)(int)(uint)(byte)_refptr_g_tsaPrpt->field_0x38 * 0x10) =
         (longlong)((*_refptr_g_tsaStaticPtr)->nBaselineStageInitialSignal * 2);
    (&_refptr_g_tsaPrpt->field_0x40)
    [(longlong)(int)(uint)(byte)_refptr_g_tsaPrpt->field_0x38 * 0x10] = param_1;
    (&_refptr_g_tsaPrpt->field_0x41)
    [(longlong)(int)(uint)(byte)_refptr_g_tsaPrpt->field_0x38 * 0x10] = param_2;
    _refptr_g_tsaPrpt->field_0x38 = _refptr_g_tsaPrpt->field_0x38 + '\x01';
  }
  return;
}


// ============================================================================
// Function: HardwareAnalyzer_DetectType3 @ 6baf32fa
// ============================================================================

undefined1 HardwareAnalyzer_DetectType3(byte param_1,byte param_2)

{
  int iVar1;
  int iVar2;
  bool bVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int local_20;
  int local_1c;
  int local_14;
  int local_10;
  undefined1 local_9;
  
  uVar4 = (uint)*(short *)((ulonglong)
                           ((uint)param_1 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols &
                           0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif);
  uVar5 = (int)uVar4 >> 0x1f;
  iVar8 = (uVar5 ^ uVar4) - uVar5;
  iVar6 = (int)(short)_refptr_g_tsaPrmtFlash->wPeakFilterReferenceThreshold;
  if (iVar6 < iVar8 * 5) {
    local_9 = 0xff;
  }
  else {
    local_10 = 0;
    for (local_14 = 0; local_14 < 4; local_14 = local_14 + 1) {
      iVar1 = *(int *)(&g_stepDelta + (longlong)local_14 * 8);
      iVar2 = *(int *)(&DAT_6bb57d84 + (longlong)local_14 * 8);
      iVar7 = TPSensor_IsSensorValid(iVar1 + (uint)param_1,iVar2 + (uint)param_2);
      if ((iVar7 != 0) &&
         (iVar8 + iVar6 / 2 <
          (int)*(short *)((ulonglong)
                          ((uint)_refptr_g_tsaPrmtFlash->bCols * (iVar2 + (uint)param_2) +
                           iVar1 + (uint)param_1 & 0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif)))
      {
        local_10 = local_10 + 1;
      }
    }
    if ((local_10 < 2) || (iVar6 * local_10 <= iVar8 * 0x14)) {
      local_9 = 0xff;
    }
    else {
      bVar3 = false;
      for (local_1c = -2; local_1c < 3; local_1c = local_1c + 1) {
        for (local_20 = -2; local_20 < 3; local_20 = local_20 + 1) {
          if ((((local_1c != 0) || (local_20 != 0)) &&
              (iVar8 = TPSensor_IsSensorValid(local_1c + (uint)param_1,local_20 + (uint)param_2),
              iVar8 != 0)) &&
             (*(short *)((ulonglong)
                         ((uint)_refptr_g_tsaPrmtFlash->bCols * (local_20 + (uint)param_2) +
                          local_1c + (uint)param_1 & 0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif)
              * 0xf < -iVar6)) {
            bVar3 = true;
            break;
          }
        }
      }
      if (bVar3) {
        local_9 = 0xff;
      }
      else {
        local_9 = 1;
      }
    }
  }
  return local_9;
}


// ============================================================================
// Function: HardwareAnalyzer_IsNotOKToEnterIDLEMode @ 6baf06ff
// ============================================================================

undefined8 HardwareAnalyzer_IsNotOKToEnterIDLEMode(void)

{
                    /* 0xb06ff  866  HardwareAnalyzer_IsNotOKToEnterIDLEMode */
  return 0;
}


// ============================================================================
// Function: HardwareAnalyzer_GetExceptionDescription @ 6baf0009
// ============================================================================

undefined * HardwareAnalyzer_GetExceptionDescription(void)

{
                    /* 0xb0009  862  HardwareAnalyzer_GetExceptionDescription */
  return &DAT_6bc8c9c8;
}


// ============================================================================
// Function: HardwareAnalyzer_Z8InfoUpdate @ 6baf24b2
// ============================================================================

void HardwareAnalyzer_Z8InfoUpdate
               (byte param_1,byte param_2,char *param_3,int *param_4,int *param_5)

{
  uint uVar1;
  uint uVar2;
  
  if ((param_1 < _refptr_g_tsaPrmtFlash->bCols) && (param_2 < _refptr_g_tsaPrmtFlash->bRows)) {
    *param_3 = *param_3 + '\x01';
    *param_4 = *param_4 +
               (uint)*(ushort *)
                      ((ulonglong)
                       ((uint)param_1 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff
                       ) * 2 + *(longlong *)_refptr_g_tsaBufRaw);
    uVar1 = (uint)*(short *)((ulonglong)
                             ((uint)param_1 + (uint)param_2 * (uint)_refptr_g_tsaPrmtFlash->bCols &
                             0xffff) * 2 + *(longlong *)_refptr_g_tsaBufDif);
    uVar2 = (int)uVar1 >> 0x1f;
    *param_5 = *param_5 + ((uVar1 ^ uVar2) - uVar2);
  }
  return;
}


