// ============================================================================
// Function: TouchMode_NoObjDetect @ 6ba790ae
// ============================================================================

void TouchMode_NoObjDetect(undefined4 *param_1,undefined4 *param_2)

{
  if ((param_1 != (undefined4 *)0x0) && (param_2 != (undefined4 *)0x0)) {
    *param_1 = 1;
    *param_2 = 2;
    if (**(char **)_refptr_g_touchesPtr == *(char *)(*(longlong *)_refptr_g_touchesPtr + 0xd)) {
      *param_1 = 2;
      *param_2 = 2;
    }
  }
  return;
}


// ============================================================================
// Function: TouchMode_NoObjProcess @ 6ba79113
// ============================================================================

void TouchMode_NoObjProcess(void)

{
  return;
}


// ============================================================================
// Function: TouchMode_FingerDetect @ 6ba7911f
// ============================================================================

void TouchMode_FingerDetect(undefined4 *param_1,undefined4 *param_2)

{
  byte local_9;
  
  if ((param_1 != (undefined4 *)0x0) && (param_2 != (undefined4 *)0x0)) {
    *param_1 = 1;
    *param_2 = 1;
    if (((_refptr_g_tsaPrmtDynamic[0x80] & 2) != 0) &&
       (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x1000) == 0)) {
      for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
        if (((*(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x1f0) & 0x800) == 0) &&
           (*(short *)(_refptr_g_tsaPrmtDynamic + 0xc) <=
            *(short *)(*_refptr_g_peaksPtr +
                      (longlong)
                      (int)(uint)*(byte *)((longlong)(int)(uint)local_9 * 0x168 +
                                           *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x18)
           )) {
          *param_1 = 2;
          break;
        }
      }
      if (*(int *)&_refptr_g_tsaPrpt[1].field_0x2c != 1) {
        (*_refptr_g_tsaStaticPtr)[1].field_0xc = 0;
      }
      if ((*(int *)&_refptr_g_tsaPrpt[1].field_0x2c != 1) ||
         (*(int *)&_refptr_g_tsaPrpt[1].field_0x30 != 2)) {
        if (*(short *)&_refptr_g_tsaPrmtFlash->field_0x5d8 * 3 < *(int *)(*_refptr_g_peaksPtr + 8))
        {
          (*_refptr_g_tsaStaticPtr)[1].field_0xc = (*_refptr_g_tsaStaticPtr)[1].field_0xc + '\x01';
        }
        else {
          (*_refptr_g_tsaStaticPtr)[1].field_0xc = 0;
        }
      }
      if (2 < (byte)(*_refptr_g_tsaStaticPtr)[1].field_0xc) {
        *param_2 = 2;
        _refptr_g_tsaPrpt[1].field_0x28 = 0;
      }
    }
  }
  return;
}


// ============================================================================
// Function: TouchMode_FingerProcess @ 6ba79318
// ============================================================================

void TouchMode_FingerProcess(undefined8 param_1,int param_2)

{
  byte local_9;
  
  if ((param_2 == 2) || (_refptr_g_tsaPrpt[1].field_0x28 == '\0')) {
    for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
      if (((*(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1f0) & 0x800) != 0) &&
         ((*_refptr_g_tsaStaticPtr)->bFullIncellSignalDisparity == '\0')) {
        *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208)
             = *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x208) | 2;
      }
    }
  }
  return;
}


// ============================================================================
// Function: TouchMode_GloveDetect @ 6ba793fe
// ============================================================================

void TouchMode_GloveDetect(undefined4 *param_1,undefined4 *param_2)

{
  byte local_9;
  
  if ((param_1 != (undefined4 *)0x0) && (param_2 != (undefined4 *)0x0)) {
    *param_1 = 1;
    *param_2 = 1;
    if ((((_refptr_g_tsaPrmtDynamic[0x80] & 4) != 0) &&
        (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 1) != 0)) ||
       (((((_refptr_g_tsaPrmtDynamic[0x80] & 8) != 0 &&
          (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x10) != 0)) ||
         (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) != 0)) ||
        ((*_refptr_g_tsaStaticPtr)[1].field_0x3 != '\0')))) {
      if (**(char **)_refptr_g_touchesPtr != *(char *)(*(longlong *)_refptr_g_touchesPtr + 0xd)) {
        *param_1 = 2;
      }
      if (((_refptr_g_tsaPrpt[1].field_0x28 == '\0') ||
          ((((_refptr_g_tsaPrmtDynamic[0x80] & 4) == 0 ||
            (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 1) == 0)) &&
           (((_refptr_g_tsaPrmtDynamic[0x80] & 8) == 0 ||
            (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x10) == 0)))))) &&
         ((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) == 0 &&
          ((*_refptr_g_tsaStaticPtr)[1].field_0x3 == '\0')))) {
        for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
          if (((((_refptr_g_tsaPrmtDynamic[0x80] & 4) != 0) &&
               (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 1) != 0)) &&
              ((*(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x1f8) & 4) != 0)) &&
             ((*(int *)&_refptr_g_tsaPrpt[1].field_0x2c != 1 &&
              ((*(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x1f8) & 0x80000) == 0)))) {
            _refptr_g_tsaPrpt[1].field_0x28 = 1;
            *param_2 = 2;
            Touch_CreatePostReport(local_9,5);
            return;
          }
          if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x1000) != 0) {
            _refptr_g_tsaPrpt[1].field_0x28 = 1;
            *param_2 = 2;
            return;
          }
        }
      }
      else {
        _refptr_g_tsaPrpt[1].field_0x28 = 1;
        *param_2 = 2;
      }
    }
  }
  return;
}


// ============================================================================
// Function: TouchMode_GloveProcess @ 6ba797a7
// ============================================================================

void TouchMode_GloveProcess(undefined8 param_1,int param_2)

{
  byte local_9;
  
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x1000) == 0) {
    if (((((_refptr_g_tsaPrmtDynamic[0x80] & 4) == 0) ||
         (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 1) == 0)) &&
        (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) == 0)) &&
       ((*_refptr_g_tsaStaticPtr)[1].field_0x3 == '\0')) {
      TouchMode_StylusSuppression();
    }
    else if (param_2 == 1) {
      for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
        *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208)
             = *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x208) | 2;
      }
    }
    else {
      TouchMode_GloveFilter();
    }
  }
  return;
}


// ============================================================================
// Function: TouchMode_StylusDetect @ 6ba798b8
// ============================================================================

void TouchMode_StylusDetect(undefined4 *param_1,undefined4 *param_2)

{
  if ((param_1 != (undefined4 *)0x0) && (param_2 != (undefined4 *)0x0)) {
    *param_1 = 1;
    *param_2 = 1;
  }
  return;
}


// ============================================================================
// Function: TouchMode_StylusProcess @ 6ba7991a
// ============================================================================

void TouchMode_StylusProcess(void)

{
  return;
}


// ============================================================================
// Function: Touch_CreatePostReport @ 6ba76a03
// ============================================================================

void Touch_CreatePostReport(byte param_1)

{
  ushort uVar1;
  ushort uVar2;
  byte bVar3;
  short local_38;
  short local_36;
  longlong local_30;
  int local_28;
  int local_24;
  int local_20;
  uint local_1c;
  uint local_18;
  byte local_12;
  byte local_11;
  uint local_10;
  uint local_c;
  
                    /* 0x36a03  2516  Touch_CreatePostReport */
  local_12 = PrevTouch_GetIdxBasedOnCurIdx(param_1);
  if ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_12 * 0x478 + 0x1f8
                ) & 0x20020000) == 0) {
    *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f8) =
         *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f8) | 0x20000;
    uVar1 = *(ushort *)
             ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_12 * 0x478 + 0x496);
    uVar2 = *(ushort *)
             ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_12 * 0x478 + 0x498);
    if ((*(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f8) & 8) != 0) {
      uVar1 = *(ushort *)
               ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_12 * 0x478 + 0x49a)
      ;
      uVar2 = *(ushort *)
               ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_12 * 0x478 + 0x49c)
      ;
    }
    local_c = (uint)uVar1;
    local_10 = (uint)uVar2;
    local_18 = (uint)*(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x162);
    local_1c = (uint)*(ushort *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x164);
    bVar3 = Touch_GetPostReportLength
                      (local_c,local_10,local_18,local_1c,
                       *(char *)((longlong)*_refptr_g_prevTouchesPtr +
                                (longlong)(int)(uint)local_12 * 0x478 + 0x288) + '\x01');
    local_20 = (int)(local_18 - local_c) / (int)(bVar3 - 1);
    local_24 = (int)(local_1c - local_10) / (int)(bVar3 - 1);
    local_28 = (int)*(undefined8 *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x148) -
               (int)*(undefined8 *)
                     ((longlong)*_refptr_g_prevTouchesPtr +
                     (longlong)(int)(uint)local_12 * 0x478 + 0x148);
    PrevTouch_HistoryBufferClean(local_12);
    for (local_11 = 0; (int)(uint)local_11 < (int)(bVar3 - 1); local_11 = local_11 + 1) {
      local_38 = (ushort)local_11 * (short)local_20 + (short)local_c;
      local_36 = (ushort)local_11 * (short)local_24 + (short)local_10;
      local_30 = *(longlong *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x148) -
                 (longlong)(int)((uint)local_11 * local_28);
      PrevTouch_HistoryBufferPush(local_12,&local_38);
    }
    *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f8) =
         *(uint *)(g_touchesPtr + (longlong)(int)(uint)param_1 * 0x168 + 0x1f8) | 0x80;
  }
  return;
}


// ============================================================================
// Function: TouchMode_GloveFilter @ 6ba796d1
// ============================================================================

void TouchMode_GloveFilter(void)

{
  byte local_9;
  
  for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
    if (((*(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208
                   ) & 1) == 0) &&
       (0x40 < *(byte *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x1c8))) {
      *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208) =
           *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x208) | 2;
    }
  }
  return;
}


// ============================================================================
// Function: TouchMode_StylusSuppression @ 6ba79ba6
// ============================================================================

void TouchMode_StylusSuppression(void)

{
  byte local_9;
  
                    /* 0x39ba6  2498  TouchMode_StylusSuppression */
  for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
    *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208) =
         *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208)
         | 4;
  }
  return;
}


// ============================================================================
// Function: PrevTouch_HistoryBufferClean @ 6ba6e9b0
// ============================================================================

void PrevTouch_HistoryBufferClean(byte param_1)

{
                    /* 0x2e9b0  1053  PrevTouch_HistoryBufferClean */
  *(undefined1 *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288) = 0;
  return;
}


// ============================================================================
// Function: PrevTouch_HistoryBufferPush @ 6ba6ea52
// ============================================================================

undefined8 PrevTouch_HistoryBufferPush(byte param_1,undefined8 *param_2)

{
  PrevTouchArray *pPVar1;
  undefined8 uVar2;
  longlong lVar3;
  
                    /* 0x2ea52  1058  PrevTouch_HistoryBufferPush */
  if (param_1 < 0x14) {
    if (0x1d < *(byte *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288)) {
      if (0x1e < *(byte *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288))
      {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                          "../../src/Alg/Touch/PrevTouchData.c",0x1c2,
                          "PrevTouchPrpt(preIdx, historyBuffer.num) <= PREVTOUCH_HISTORY_BUFFER_SIZE"
                         );
        HardwareAnalyzer_SetSoftException();
      }
      *(undefined1 *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288) =
           0x1e;
      PrevTouch_HistoryBufferRemove(param_1,0);
    }
    pPVar1 = g_pPrevTouches;
    lVar3 = (longlong)(int)(uint)param_1 * 0x478 +
            ((longlong)
             (int)(uint)*(byte *)((longlong)g_pPrevTouches +
                                 (longlong)(int)(uint)param_1 * 0x478 + 0x288) + 0x14) * 0x10;
    uVar2 = param_2[1];
    *(undefined8 *)((longlong)g_pPrevTouches + lVar3 + 0x150) = *param_2;
    *(undefined8 *)((longlong)pPVar1 + lVar3 + 0x158) = uVar2;
    *(char *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288) =
         *(char *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288) + '\x01'
    ;
    uVar2 = 1;
  }
  else {
    if (0x13 < param_1) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Alg/Touch/PrevTouchData.c",0x1be,"preIdx < TSA_MAX_TOUCH_NUM");
      HardwareAnalyzer_SetSoftException();
    }
    uVar2 = 0;
  }
  return uVar2;
}


// ============================================================================
// Function: PrevTouch_GetIdxBasedOnCurIdx @ 6ba6dcc4
// ============================================================================

undefined1 PrevTouch_GetIdxBasedOnCurIdx(byte param_1)

{
                    /* 0x2dcc4  1051  PrevTouch_GetIdxBasedOnCurIdx */
  return *(undefined1 *)
          ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x287);
}


// ============================================================================
// Function: Touch_GetPostReportLength @ 6ba76958
// ============================================================================

uint Touch_GetPostReportLength(int param_1,int param_2,int param_3,int param_4)

{
  ushort uVar1;
  uint uVar2;
  uint uVar3;
  uint local_c;
  
                    /* 0x36958  2522  Touch_GetPostReportLength */
  uVar1 = TS_MMToPixel(2);
  local_c = 0;
  if (uVar1 != 0) {
    uVar2 = param_3 - param_1 >> 0x1f;
    uVar3 = param_4 - param_2 >> 0x1f;
    if ((int)((param_4 - param_2 ^ uVar3) - uVar3) < (int)((param_3 - param_1 ^ uVar2) - uVar2)) {
      uVar2 = param_3 - param_1 >> 0x1f;
      local_c = (int)((param_3 - param_1 ^ uVar2) - uVar2) / (int)(uint)uVar1;
    }
    else {
      uVar2 = param_4 - param_2 >> 0x1f;
      local_c = (int)((param_4 - param_2 ^ uVar2) - uVar2) / (int)(uint)uVar1;
    }
  }
  if (5 < local_c) {
    local_c = 5;
  }
  if (local_c < 2) {
    local_c = 2;
  }
  return local_c;
}


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
// Function: PrevTouch_HistoryBufferRemove @ 6ba6f11a
// ============================================================================

void PrevTouch_HistoryBufferRemove(byte param_1,byte param_2)

{
  undefined8 uVar1;
  PrevTouchArray *pPVar2;
  longlong lVar3;
  longlong lVar4;
  byte local_9;
  
                    /* 0x2f11a  1059  PrevTouch_HistoryBufferRemove */
  if (param_1 < 0x14) {
    if (0x1e < *(byte *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288)) {
      if (0x1e < *(byte *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288))
      {
        StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                          "../../src/Alg/Touch/PrevTouchData.c",0x21b,
                          "PrevTouchPrpt(preIdx, historyBuffer.num) <= PREVTOUCH_HISTORY_BUFFER_SIZE"
                         );
        HardwareAnalyzer_SetSoftException();
      }
      *(undefined1 *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288) =
           0x1e;
    }
    if (param_2 < *(byte *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288)
       ) {
      for (local_9 = param_2; pPVar2 = g_pPrevTouches,
          (int)(char)local_9 <
          (int)(*(byte *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288) -
               1); local_9 = local_9 + 1) {
        lVar4 = (longlong)(int)(uint)param_1 * 0x478 + ((longlong)(int)(char)local_9 + 0x14) * 0x10;
        lVar3 = (longlong)(int)(uint)param_1 * 0x478 + ((longlong)((char)local_9 + 1) + 0x14) * 0x10
        ;
        uVar1 = *(undefined8 *)((longlong)g_pPrevTouches + lVar3 + 0x158);
        *(undefined8 *)((longlong)g_pPrevTouches + lVar4 + 0x150) =
             *(undefined8 *)((longlong)g_pPrevTouches + lVar3 + 0x150);
        *(undefined8 *)((longlong)pPVar2 + lVar4 + 0x158) = uVar1;
      }
      *(char *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288) =
           *(char *)((longlong)g_pPrevTouches + (longlong)(int)(uint)param_1 * 0x478 + 0x288) + -1;
    }
  }
  else if (0x13 < param_1) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/Touch/PrevTouchData.c",0x218,"preIdx < TSA_MAX_TOUCH_NUM");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: StrPrintf_Process @ 6bb3addf
// ============================================================================

void StrPrintf_Process(int param_1,char *param_2,undefined8 param_3,undefined8 param_4)

{
  longlong lVar1;
  char *pcVar2;
  undefined8 local_res18;
  undefined8 local_res20;
  char local_218 [508];
  int local_1c;
  
                    /* 0xfaddf  1432  StrPrintf_Process */
  if (g_strPrintfPutf != (code *)0x0) {
    pcVar2 = local_218;
    for (lVar1 = 0x3e; lVar1 != 0; lVar1 = lVar1 + -1) {
      pcVar2[0] = '\0';
      pcVar2[1] = '\0';
      pcVar2[2] = '\0';
      pcVar2[3] = '\0';
      pcVar2[4] = '\0';
      pcVar2[5] = '\0';
      pcVar2[6] = '\0';
      pcVar2[7] = '\0';
      pcVar2 = pcVar2 + 8;
    }
    pcVar2[0] = '\0';
    pcVar2[1] = '\0';
    pcVar2[2] = '\0';
    pcVar2[3] = '\0';
    local_res18 = param_3;
    local_res20 = param_4;
    local_1c = vsnprintf_s(local_218,500,499,param_2,(va_list)&local_res18);
    if (local_1c != -1) {
      if (param_1 == 0) {
        if (20000 < g_logTsaBufCnt + 500U) {
          (*g_strPrintfPutf)(&g_logBuf,0);
          StrPrintf_ClearLogBuffer();
        }
        g_logTsaBufPtr = StrPrintf_CatLogBuffer(g_logTsaBufPtr,&g_logTsaBufCnt,local_218);
      }
      else {
        (*g_strPrintfPutf)(local_218,param_1);
      }
    }
  }
  return;
}


// ============================================================================
// Function: TS_MMToPixel @ 6ba7c416
// ============================================================================

int TS_MMToPixel(byte param_1)

{
                    /* 0x3c416  2345  TS_MMToPixel */
  return (uint)(byte)_refptr_g_tsaPrmtFlashConst[0x1a] * (uint)param_1;
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
// Function: vsnprintf_s @ 6ba46a10
// ============================================================================

int __cdecl
vsnprintf_s(char *_DstBuf,size_t _DstSize,size_t _MaxCount,char *_Format,va_list _ArgList)

{
  int local_c;
  
                    /* 0x6a10  3012  vsnprintf_s */
  if (((((_Format == (char *)0x0) || (_DstBuf == (char *)0x0)) || (_DstSize == 0)) ||
      (0x7fffffff < _DstSize)) || ((0x7ffffffe < _MaxCount && (_MaxCount != 0xffffffffffffffff)))) {
    if ((_DstBuf != (char *)0x0) && ((_DstSize != 0 && (_DstSize < 0x80000000)))) {
      *_DstBuf = '\0';
    }
    local_c = -1;
  }
  else {
    if (_MaxCount < _DstSize) {
      local_c = SecVsnprintfImpl(_DstBuf,_MaxCount + 1,_Format,_ArgList);
      if (local_c == -2) {
        return -1;
      }
    }
    else {
      local_c = SecVsnprintfImpl(_DstBuf,_DstSize,_Format,_ArgList);
      if ((local_c == -2) && (_MaxCount == 0xffffffffffffffff)) {
        return -1;
      }
    }
    if (local_c < 0) {
      *_DstBuf = '\0';
      local_c = -1;
    }
  }
  return local_c;
}


// ============================================================================
// Function: StrPrintf_CatLogBuffer @ 6bb3ace3
// ============================================================================

char * StrPrintf_CatLogBuffer(char *param_1,int *param_2,char *param_3)

{
  char *local_res8;
  char *local_res18;
  
  for (local_res8 = param_1; local_res18 = param_3, *local_res8 != '\0'; local_res8 = local_res8 + 1
      ) {
  }
  while (*local_res18 != '\0') {
    *local_res8 = *local_res18;
    local_res8 = local_res8 + 1;
    local_res18 = local_res18 + 1;
    *param_2 = *param_2 + 1;
  }
  return local_res8 + -1;
}


// ============================================================================
// Function: StrPrintf_ClearLogBuffer @ 6bb3aca0
// ============================================================================

void StrPrintf_ClearLogBuffer(void)

{
  memset_s(&g_logBuf,20000,0,20000);
  g_logTsaBufPtr = &g_logBuf;
  g_logTsaBufCnt = 0;
  return;
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
// Function: memset_s @ 6ba416ef
// ============================================================================

undefined8 memset_s(longlong param_1,ulonglong param_2,undefined4 param_3,ulonglong param_4)

{
  bool bVar1;
  undefined8 uVar2;
  
                    /* 0x16ef  3002  memset_s */
  if ((param_2 < 0x80000000) && (param_1 != 0)) {
    bVar1 = true;
  }
  else {
    bVar1 = false;
  }
  if ((bVar1) && (param_4 <= param_2)) {
    (*(code *)_refptr_memset)(param_1,param_3,param_4);
    uVar2 = 0;
  }
  else {
    uVar2 = SecMemsetError(param_1,param_2,param_3,param_4);
  }
  return uVar2;
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
// Function: SecWriteFloatText @ 6ba42040
// ============================================================================

void SecWriteFloatText(undefined8 param_1,longlong param_2,undefined8 param_3)

{
  SecWriteString(*(undefined8 *)(param_2 + 8),*(undefined4 *)(param_2 + 0x10),param_1,param_3);
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
// Function: SecNumberCompatZero @ 6ba42f17
// ============================================================================

void SecNumberCompatZero(void)

{
  return;
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
// Function: SecUpdateWstringFlags @ 6ba42835
// ============================================================================

void SecUpdateWstringFlags(undefined8 param_1)

{
  SecUpdateWcharFlags(param_1);
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
// Function: SecNumberToString @ 6ba41b37
// ============================================================================

void SecNumberToString(undefined8 param_1,undefined8 param_2)

{
  SecNumber64ToString(param_1,param_2);
  return;
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


