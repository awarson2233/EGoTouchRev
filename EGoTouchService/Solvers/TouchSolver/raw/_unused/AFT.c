// ============================================================================
// Function: AFT_Process @ 6bad5fbc
// ============================================================================

void AFT_Process(void)

{
                    /* 0x95fbc  81  AFT_Process */
  g_curTime = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  AFT_Reset();
  AFT_GetInInfo();
  AFT_FilterAlgo(&g_aftInInfo,&g_aftOutInfo);
  AFT_GetOutSortInfo();
  return;
}


// ============================================================================
// Function: AFT_FilterAlgo @ 6bad4e62
// ============================================================================

void AFT_FilterAlgo(undefined8 param_1,undefined8 param_2)

{
                    /* 0x94e62  72  AFT_FilterAlgo */
  AFT_ScreenStateProc();
  AFT_PreProjectProcess(param_1);
  AFT_PrintTsInfos(param_1);
  AFT_AntiEdgeFalseTouch(param_1,param_2);
  DAT_6bc7249c = DAT_6bc72498;
  g_lastScreenRotatedState = (*_refptr_g_tsaStaticPtr)->nField_0x264;
  AFT_PrintTsInfos(param_2);
  return;
}


// ============================================================================
// Function: AFT_GetInInfo @ 6bad4ed9
// ============================================================================

void AFT_GetInInfo(void)

{
  char cVar1;
  undefined1 uVar2;
  byte bVar3;
  byte bVar4;
  undefined2 uVar5;
  ushort uVar6;
  byte bVar7;
  byte local_1d;
  uint local_1c;
  
                    /* 0x94ed9  76  AFT_GetInInfo */
  DAT_6bc72880 = (uint)**(byte **)_refptr_g_touchesPtr;
  for (local_1c = 0; (int)local_1c < (int)DAT_6bc72880; local_1c = local_1c + 1) {
    bVar4 = *(char *)((longlong)(int)local_1c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x154) -
            1;
    *(undefined2 *)(&DAT_6bc724e0 + (longlong)(int)(uint)bVar4 * 0x30) = 0x40;
    *(uint *)(&DAT_6bc724d0 + (longlong)(int)(uint)bVar4 * 0x30) =
         (uint)*(ushort *)
                ((longlong)(int)local_1c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x15c);
    *(uint *)(&DAT_6bc724c4 + (longlong)(int)(uint)bVar4 * 0x30) =
         *(byte *)((longlong)(int)local_1c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x154) - 1;
    *(undefined4 *)(&g_aftInInfo + (longlong)(int)(uint)bVar4 * 0x30) = 0;
    uVar5 = TS_MMToPixel((char)*(undefined2 *)
                                ((longlong)(int)local_1c * 0x168 + *(longlong *)_refptr_g_touchesPtr
                                + 0x188));
    uVar6 = TSAOut_Axis(uVar5);
    *(uint *)(&DAT_6bc724d4 + (longlong)(int)(uint)bVar4 * 0x30) = (uint)uVar6;
    uVar5 = TS_MMToPixel((char)*(undefined2 *)
                                ((longlong)(int)local_1c * 0x168 + *(longlong *)_refptr_g_touchesPtr
                                + 0x18a));
    uVar6 = TSAOut_Axis(uVar5);
    *(uint *)(&DAT_6bc724d8 + (longlong)(int)(uint)bVar4 * 0x30) = (uint)uVar6;
    cVar1 = TSAOut_Angle((int)*(char *)((longlong)(int)local_1c * 0x168 +
                                        *(longlong *)_refptr_g_touchesPtr + 0x192));
    *(int *)(&DAT_6bc724dc + (longlong)(int)(uint)bVar4 * 0x30) = (int)cVar1;
    if (_refptr_g_tsaPrmtFlash->bCmfForceDim1Pass == _refptr_g_tsaPrmtFlash->field_0x18) {
      uVar6 = TSAOut_Pixel2DisplayDim1
                        (*(undefined2 *)
                          ((longlong)(int)local_1c * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                          0x170));
      *(uint *)(&DAT_6bc724c8 + (longlong)(int)(uint)bVar4 * 0x30) = (uint)uVar6;
      uVar6 = TSAOut_Pixel2DisplayDim2
                        (*(undefined2 *)
                          ((longlong)(int)local_1c * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                          0x172));
      *(uint *)(&DAT_6bc724cc + (longlong)(int)(uint)bVar4 * 0x30) = (uint)uVar6;
      bVar3 = *(byte *)((longlong)(int)local_1c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1ac)
      ;
      bVar7 = 0xf;
      if (bVar3 < 0x10) {
        bVar7 = bVar3;
      }
      (&DAT_6bc724e6)[(longlong)(int)(uint)bVar4 * 0x30] = bVar7;
      bVar3 = *(byte *)((longlong)(int)local_1c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1ad)
      ;
      bVar7 = 0xf;
      if (bVar3 < 0x10) {
        bVar7 = bVar3;
      }
      (&DAT_6bc724e7)[(longlong)(int)(uint)bVar4 * 0x30] = bVar7;
      bVar3 = *(byte *)((longlong)(int)local_1c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1b0)
      ;
      bVar7 = 0xf;
      if (bVar3 < 0x10) {
        bVar7 = bVar3;
      }
      (&DAT_6bc724e8)[(longlong)(int)(uint)bVar4 * 0x30] = bVar7;
      bVar3 = *(byte *)((longlong)(int)local_1c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1b1)
      ;
      bVar7 = 0xf;
      if (bVar3 < 0x10) {
        bVar7 = bVar3;
      }
      (&DAT_6bc724e9)[(longlong)(int)(uint)bVar4 * 0x30] = bVar7;
      if (*(ushort *)((longlong)(int)local_1c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1a8)
          >> 2 < 0xff) {
        uVar2 = (undefined1)
                (*(ushort *)
                  ((longlong)(int)local_1c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1a8) >> 2
                );
      }
      else {
        uVar2 = 0xff;
      }
      (&DAT_6bc724ea)[(longlong)(int)(uint)bVar4 * 0x30] = uVar2;
      if (*(ushort *)((longlong)(int)local_1c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1aa)
          >> 2 < 0xff) {
        uVar2 = (undefined1)
                (*(ushort *)
                  ((longlong)(int)local_1c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1aa) >> 2
                );
      }
      else {
        uVar2 = 0xff;
      }
      (&DAT_6bc724eb)[(longlong)(int)(uint)bVar4 * 0x30] = uVar2;
      bVar3 = TSA_RptTouchDim1Center(local_1c & 0xff);
      *(ushort *)(&DAT_6bc724e2 + (longlong)(int)(uint)bVar4 * 0x30) = (ushort)bVar3;
      bVar3 = TSA_RptTouchDim2Center(local_1c & 0xff);
      *(ushort *)(&DAT_6bc724e4 + (longlong)(int)(uint)bVar4 * 0x30) = (ushort)bVar3;
    }
    else {
      uVar6 = TSAOut_Pixel2DisplayDim2
                        (*(undefined2 *)
                          ((longlong)(int)local_1c * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                          0x172));
      *(uint *)(&DAT_6bc724c8 + (longlong)(int)(uint)bVar4 * 0x30) = (uint)uVar6;
      uVar6 = TSAOut_Pixel2DisplayDim1
                        (*(undefined2 *)
                          ((longlong)(int)local_1c * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                          0x170));
      *(uint *)(&DAT_6bc724cc + (longlong)(int)(uint)bVar4 * 0x30) = (uint)uVar6;
      bVar3 = *(byte *)((longlong)(int)local_1c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1ad)
      ;
      bVar7 = 0xf;
      if (bVar3 < 0x10) {
        bVar7 = bVar3;
      }
      (&DAT_6bc724e6)[(longlong)(int)(uint)bVar4 * 0x30] = bVar7;
      bVar3 = *(byte *)((longlong)(int)local_1c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1ac)
      ;
      bVar7 = 0xf;
      if (bVar3 < 0x10) {
        bVar7 = bVar3;
      }
      (&DAT_6bc724e7)[(longlong)(int)(uint)bVar4 * 0x30] = bVar7;
      bVar3 = *(byte *)((longlong)(int)local_1c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1b1)
      ;
      bVar7 = 0xf;
      if (bVar3 < 0x10) {
        bVar7 = bVar3;
      }
      (&DAT_6bc724e8)[(longlong)(int)(uint)bVar4 * 0x30] = bVar7;
      bVar3 = *(byte *)((longlong)(int)local_1c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1b0)
      ;
      bVar7 = 0xf;
      if (bVar3 < 0x10) {
        bVar7 = bVar3;
      }
      (&DAT_6bc724e9)[(longlong)(int)(uint)bVar4 * 0x30] = bVar7;
      if (*(ushort *)((longlong)(int)local_1c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1aa)
          >> 2 < 0xff) {
        uVar2 = (undefined1)
                (*(ushort *)
                  ((longlong)(int)local_1c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1aa) >> 2
                );
      }
      else {
        uVar2 = 0xff;
      }
      (&DAT_6bc724ea)[(longlong)(int)(uint)bVar4 * 0x30] = uVar2;
      if (*(ushort *)((longlong)(int)local_1c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1a8)
          >> 2 < 0xff) {
        uVar2 = (undefined1)
                (*(ushort *)
                  ((longlong)(int)local_1c * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1a8) >> 2
                );
      }
      else {
        uVar2 = 0xff;
      }
      (&DAT_6bc724eb)[(longlong)(int)(uint)bVar4 * 0x30] = uVar2;
      bVar3 = TSA_RptTouchDim2Center(local_1c & 0xff);
      *(ushort *)(&DAT_6bc724e2 + (longlong)(int)(uint)bVar4 * 0x30) = (ushort)bVar3;
      bVar3 = TSA_RptTouchDim1Center(local_1c & 0xff);
      *(ushort *)(&DAT_6bc724e4 + (longlong)(int)(uint)bVar4 * 0x30) = (ushort)bVar3;
    }
  }
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"\n *AFT Process in*:");
  }
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0," TouchNum = %d",DAT_6bc72880);
  }
  for (local_1d = 0; (int)(uint)local_1d < (int)DAT_6bc72880; local_1d = local_1d + 1) {
    bVar4 = *(char *)((longlong)(int)(uint)local_1d * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x154) - 1;
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,&DAT_6bb56873,
                        *(undefined4 *)(&DAT_6bc724c4 + (longlong)(int)(uint)bVar4 * 0x30));
    }
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,&DAT_6bb56878,
                        *(undefined4 *)(&DAT_6bc724c8 + (longlong)(int)(uint)bVar4 * 0x30));
    }
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,&DAT_6bb56878,
                        *(undefined4 *)(&DAT_6bc724cc + (longlong)(int)(uint)bVar4 * 0x30));
    }
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,&DAT_6bb5687c,
                        *(undefined4 *)(&DAT_6bc724d0 + (longlong)(int)(uint)bVar4 * 0x30));
    }
  }
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,&DAT_6bb56881);
  }
  return;
}


// ============================================================================
// Function: AFT_Reset @ 6bad59fe
// ============================================================================

void AFT_Reset(void)

{
  int iVar1;
  
                    /* 0x959fe  83  AFT_Reset */
  iVar1 = memset_s(&g_aftInInfo,0x3d8,0,0x3d8);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/AntiFalseTouch/AntiFalseTouch_Process.c",0xee,"rc == EOK"
                     );
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(&g_aftOutInfo,0x3d8,0,0x3d8);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/AntiFalseTouch/AntiFalseTouch_Process.c",0xf0,"rc == EOK"
                     );
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(&g_aftOutSortInfo,0x3d8,0,0x3d8);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/AntiFalseTouch/AntiFalseTouch_Process.c",0xf2,"rc == EOK"
                     );
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: AFT_GetOutSortInfo @ 6bad5b15
// ============================================================================

void AFT_GetOutSortInfo(void)

{
  byte local_11;
  int local_10;
  int local_c;
  
                    /* 0x95b15  77  AFT_GetOutSortInfo */
  DAT_6bc72c80 = DAT_6bc73440;
  local_c = 0;
  for (local_10 = 0; local_10 < 0x14; local_10 = local_10 + 1) {
    if ((*(short *)(&DAT_6bc730a0 + (longlong)local_10 * 0x30) != 0) &&
       (*(int *)(&DAT_6bc73090 + (longlong)local_10 * 0x30) != 0)) {
      *(undefined4 *)(&DAT_6bc728c8 + (longlong)local_c * 0x30) =
           *(undefined4 *)(&DAT_6bc73088 + (longlong)local_10 * 0x30);
      *(undefined4 *)(&DAT_6bc728cc + (longlong)local_c * 0x30) =
           *(undefined4 *)(&DAT_6bc7308c + (longlong)local_10 * 0x30);
      *(undefined4 *)(&DAT_6bc728d0 + (longlong)local_c * 0x30) =
           *(undefined4 *)(&DAT_6bc73090 + (longlong)local_10 * 0x30);
      *(undefined4 *)(&DAT_6bc728d4 + (longlong)local_c * 0x30) =
           *(undefined4 *)(&DAT_6bc73094 + (longlong)local_10 * 0x30);
      *(undefined4 *)(&DAT_6bc728d8 + (longlong)local_c * 0x30) =
           *(undefined4 *)(&DAT_6bc73098 + (longlong)local_10 * 0x30);
      *(undefined4 *)(&DAT_6bc728dc + (longlong)local_c * 0x30) =
           *(undefined4 *)(&DAT_6bc7309c + (longlong)local_10 * 0x30);
      *(undefined4 *)(&DAT_6bc728c4 + (longlong)local_c * 0x30) =
           *(undefined4 *)(&DAT_6bc73084 + (longlong)local_10 * 0x30);
      *(undefined4 *)(&g_aftOutSortInfo + (longlong)local_c * 0x30) =
           *(undefined4 *)(&g_aftOutInfo + (longlong)local_10 * 0x30);
      *(undefined2 *)(&DAT_6bc728e0 + (longlong)local_c * 0x30) =
           *(undefined2 *)(&DAT_6bc730a0 + (longlong)local_10 * 0x30);
      local_c = local_c + 1;
    }
  }
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0,"\n *AFT Process out*:");
  }
  if (*_refptr_g_tsaLogLoglevel != '\0') {
    StrPrintf_Process(0," TouchNum = %d",DAT_6bc72c80);
  }
  for (local_11 = 0; (int)(uint)local_11 < DAT_6bc73440; local_11 = local_11 + 1) {
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,&DAT_6bb56873,
                        *(undefined4 *)(&DAT_6bc728c4 + (longlong)(int)(uint)local_11 * 0x30));
    }
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,&DAT_6bb56878,
                        *(undefined4 *)(&DAT_6bc728c8 + (longlong)(int)(uint)local_11 * 0x30));
    }
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,&DAT_6bb56878,
                        *(undefined4 *)(&DAT_6bc728cc + (longlong)(int)(uint)local_11 * 0x30));
    }
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,&DAT_6bb5687c,
                        *(undefined4 *)(&DAT_6bc728d0 + (longlong)(int)(uint)local_11 * 0x30));
    }
  }
  return;
}


// ============================================================================
// Function: AFT_AntiEdgeFalseTouch @ 6bad4119
// ============================================================================

void AFT_AntiEdgeFalseTouch(undefined8 param_1,longlong param_2)

{
  uint local_28;
  short local_24;
  short local_20;
  short local_1c;
  int local_10;
  byte local_9;
  
                    /* 0x94119  69  AFT_AntiEdgeFalseTouch */
  local_10 = memset_s(&local_28,0x10,0,0x10);
  if (local_10 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/AntiFalseTouch/AntiEdgeFalseTouch.c",0x98c,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  local_10 = memset_s(param_2,0x3d8,0,0x3d8);
  if (local_10 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/AntiFalseTouch/AntiEdgeFalseTouch.c",0x98f,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  if (((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e2 & 1) == 0) ||
     (*_refptr_g_algoSwitch == '\0')) {
    AFT_TranferDataCompletely(param_1,param_2);
  }
  else {
    AFT_ParameterAdjust(0);
    AFT_MustMovFgProc();
    AFT_EdgeStateProcess(param_1,param_2,&local_28);
    AFT_EdgeStateReproc(param_1,param_2,&local_28);
    AFT_AddReportReproc(param_1,param_2,&local_28);
    if ((local_20 == 0) && (local_1c == 0)) {
      if (local_24 == 0) {
        *(undefined2 *)(param_2 + 0x20) = 0x20;
        *(undefined4 *)(_refptr_g_generalInfo + 8) = 0;
        *(undefined4 *)(_refptr_g_generalInfo + 4) = 0;
      }
      else {
        for (local_9 = 0; local_9 < 10; local_9 = local_9 + 1) {
          if ((1 << (local_9 & 0x1f) & local_28) != 0) {
            AFT_T3Report(local_9,param_1,param_2);
            *(uint *)((longlong)(int)(uint)local_9 * 0x30 + param_2 + 0x14) =
                 *(uint *)((longlong)(int)(uint)local_9 * 0x30 + param_2 + 0x14) | 0x8000000;
            *(undefined4 *)(param_2 + 0x3c4) = 1;
            *(int *)(param_2 + 0x3c0) = *(int *)(param_2 + 0x3c0) + 1;
            *(uint *)(_refptr_g_generalInfo + 8) =
                 *(uint *)(_refptr_g_generalInfo + 8) | 1 << (local_9 & 0x1f);
            *(uint *)(_refptr_g_generalInfo + 4) =
                 *(uint *)(_refptr_g_generalInfo + 4) | 1 << (local_9 & 0x1f);
            break;
          }
        }
      }
    }
    else {
      for (local_9 = 0; local_9 < 10; local_9 = local_9 + 1) {
        if (*(short *)((longlong)(int)(uint)local_9 * 0x30 + param_2 + 0x20) == 0x40) {
          *(int *)(param_2 + 0x3c0) = *(int *)(param_2 + 0x3c0) + 1;
          *(uint *)(_refptr_g_generalInfo + 8) =
               *(uint *)(_refptr_g_generalInfo + 8) | 1 << (local_9 & 0x1f);
          *(uint *)(_refptr_g_generalInfo + 4) =
               *(uint *)(_refptr_g_generalInfo + 4) | 1 << (local_9 & 0x1f);
        }
        else {
          *(uint *)(_refptr_g_generalInfo + 8) =
               *(uint *)(_refptr_g_generalInfo + 8) & ~(1 << (local_9 & 0x1f)) & 0xffffU;
          *(uint *)(_refptr_g_generalInfo + 4) =
               *(uint *)(_refptr_g_generalInfo + 4) & ~(1 << (local_9 & 0x1f)) & 0xffffU;
        }
      }
    }
    if (local_20 < g_aftState) {
      g_lastRelTime = *(undefined8 *)_refptr_g_curTime;
    }
    g_aftState = local_20;
    AFT_GetSuppressedAndAddedData(param_1,param_2);
    AFT_PrintSuppressedAndAddedData();
  }
  return;
}


// ============================================================================
// Function: AFT_PreProjectProcess @ 6bad4530
// ============================================================================

void AFT_PreProjectProcess(longlong param_1)

{
  int local_10;
  uint local_c;
  
                    /* 0x94530  80  AFT_PreProjectProcess */
  local_10 = 0;
  g_generalInfo = 0;
  for (local_c = 0; local_c < 0x14; local_c = local_c + 1) {
    if (*(short *)((ulonglong)local_c * 0x30 + param_1 + 0x20) == 0x40) {
      local_10 = local_10 + 1;
      g_generalInfo = g_generalInfo | 1 << ((byte)local_c & 0x1f);
    }
  }
  DAT_6bc72498 = g_generalInfo;
  DAT_6bc72494 = g_generalInfo;
  *(int *)(param_1 + 0x3c0) = local_10;
  if (local_10 == 0) {
    *(undefined2 *)(param_1 + 0x20) = 0x20;
  }
  return;
}


// ============================================================================
// Function: AFT_PrintTsInfos @ 6bad45f9
// ============================================================================

void AFT_PrintTsInfos(void)

{
  return;
}


// ============================================================================
// Function: AFT_ScreenStateProc @ 6bad4603
// ============================================================================

void AFT_ScreenStateProc(void)

{
  byte local_9;
  
  if ((*_refptr_g_tsaStaticPtr)->nField_0x264 != g_lastScreenRotatedState) {
    for (local_9 = 0; local_9 < 10; local_9 = local_9 + 1) {
      *(undefined4 *)(_refptr_g_aftFgInfo + (longlong)(int)(uint)local_9 * 0x58) = 0;
      *(undefined4 *)(_refptr_g_aftFgInfo + (longlong)(int)(uint)local_9 * 0x58 + 0x38) = 0;
    }
  }
  AFT_ReinitPalmEdge();
  return;
}


// ============================================================================
// Function: AFT_ParameterAdjust @ 6bac8e10
// ============================================================================

void AFT_ParameterAdjust(int param_1)

{
                    /* 0x88e10  79  AFT_ParameterAdjust */
  if ((((DAT_6bc71c46 & 4) == 0) || (*(int *)_refptr_g_foldedDisplay == 2)) &&
     (((DAT_6bc71c46 & 4) != 0 || (*(int *)_refptr_g_foldedDisplay != 2)))) {
    if ((param_1 != 0) || (g_aftPara2 != DAT_6bc7202a)) {
      DAT_6bc71c46 = DAT_6bc71c46 & 0xfff7;
      g_aftPara2 = DAT_6bc7202a;
      DAT_6bc72092 = DAT_6bc7202c;
      DAT_6bc72094 = DAT_6bc7202e;
      DAT_6bc72096 = DAT_6bc72030;
      DAT_6bc72098 = DAT_6bc72032;
      DAT_6bc7209a = DAT_6bc72034;
      DAT_6bc7209c = DAT_6bc72036;
      DAT_6bc7209e = DAT_6bc72038;
    }
  }
  else if ((param_1 != 0) || (g_aftPara2 == DAT_6bc7202a)) {
    DAT_6bc71c46 = DAT_6bc71c46 | 8;
    g_aftPara2 = DAT_6bc72032;
    DAT_6bc72092 = DAT_6bc72034;
    DAT_6bc72094 = DAT_6bc72036;
    DAT_6bc72096 = DAT_6bc72038;
    DAT_6bc72098 = (short)(((int)DAT_6bc7203a + (int)DAT_6bc72032) / 2);
    DAT_6bc7209a = (short)(((int)DAT_6bc7203c + (int)DAT_6bc72034) / 2);
    DAT_6bc7209c = (short)(((int)DAT_6bc7203e + (int)DAT_6bc72036) / 2);
    DAT_6bc7209e = (short)(((int)DAT_6bc72040 + (int)DAT_6bc72038) / 2);
  }
  return;
}


// ============================================================================
// Function: AFT_T3Report @ 6bacd362
// ============================================================================

void AFT_T3Report(undefined4 param_1,undefined8 param_2,undefined8 param_3)

{
  AFT_CopyInToOut(1,param_1,param_1,param_2,param_3);
  return;
}


// ============================================================================
// Function: AFT_GetSuppressedAndAddedData @ 6bacd39d
// ============================================================================

void AFT_GetSuppressedAndAddedData(longlong param_1,longlong param_2)

{
  int iVar1;
  undefined8 *puVar2;
  undefined8 *puVar3;
  int local_c;
  
  iVar1 = memset_s(_refptr_g_statisticsNum,4,0,4);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/AntiFalseTouch/AntiEdgeFalseTouch.c",0x3e1,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(_refptr_g_suppressedInfo,0x1e0,0,0x1e0);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/AntiFalseTouch/AntiEdgeFalseTouch.c",0x3e3,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar1 = memset_s(_refptr_g_addedInfo,0x1e0,0,0x1e0);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/AntiFalseTouch/AntiEdgeFalseTouch.c",0x3e5,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  for (local_c = 0; local_c < 10; local_c = local_c + 1) {
    if ((*(short *)((longlong)local_c * 0x30 + param_1 + 0x20) == 0x40) &&
       (*(int *)(&DAT_6bc71cb8 + (longlong)local_c * 0x58) == 0)) {
      puVar2 = (undefined8 *)
               (_refptr_g_suppressedInfo + (longlong)(int)*(short *)_refptr_g_statisticsNum * 0x30);
      puVar3 = (undefined8 *)((longlong)local_c * 0x30 + param_1);
      *puVar2 = *puVar3;
      puVar2[1] = puVar3[1];
      puVar2[2] = puVar3[2];
      puVar2[3] = puVar3[3];
      puVar2[4] = puVar3[4];
      puVar2[5] = puVar3[5];
      *(short *)_refptr_g_statisticsNum = *(short *)_refptr_g_statisticsNum + 1;
    }
    if ((*(short *)((longlong)local_c * 0x30 + param_2 + 0x20) == 0x40) &&
       (*(int *)(&DAT_6bc71cb8 + (longlong)local_c * 0x58) == 2)) {
      puVar2 = (undefined8 *)
               (_refptr_g_addedInfo + (longlong)(int)*(short *)(_refptr_g_statisticsNum + 2) * 0x30)
      ;
      puVar3 = (undefined8 *)((longlong)local_c * 0x30 + param_2);
      *puVar2 = *puVar3;
      puVar2[1] = puVar3[1];
      puVar2[2] = puVar3[2];
      puVar2[3] = puVar3[3];
      puVar2[4] = puVar3[4];
      puVar2[5] = puVar3[5];
      *(short *)(_refptr_g_statisticsNum + 2) = *(short *)(_refptr_g_statisticsNum + 2) + 1;
    }
  }
  return;
}


// ============================================================================
// Function: AFT_PrintSuppressedAndAddedData @ 6bacd69f
// ============================================================================

void AFT_PrintSuppressedAndAddedData(void)

{
  return;
}


// ============================================================================
// Function: AFT_MustMovFgProc @ 6bacd6a5
// ============================================================================

void AFT_MustMovFgProc(void)

{
  if ((DAT_6bc71c42 != 0) && (600 < *(longlong *)_refptr_g_curTime - g_lastRelTime)) {
    DAT_6bc71c42 = 0;
  }
  if ((DAT_6bc71c44 != 0) && (600 < *(longlong *)_refptr_g_curTime - g_lastMovTime)) {
    DAT_6bc71c44 = 0;
  }
  return;
}


// ============================================================================
// Function: AFT_EdgeStateReproc @ 6bad2aae
// ============================================================================

void AFT_EdgeStateReproc(longlong param_1,undefined8 param_2,longlong param_3)

{
  int local_c;
  
  for (local_c = 0; local_c < 10; local_c = local_c + 1) {
    if ((*(int *)(&g_aftFgInfo + (longlong)local_c * 0x58) != 0) &&
       (*(uint *)(&g_aftFgInfo + (longlong)local_c * 0x58) < 9)) {
      if ((*(short *)(param_3 + 8) != *(short *)(&DAT_6bc71c84 + (longlong)local_c * 0x58)) &&
         (*(ushort *)(&DAT_6bc71c86 + (longlong)local_c * 0x58) =
               *(ushort *)(&DAT_6bc71c86 + (longlong)local_c * 0x58) | 4,
         *(short *)(&DAT_6bc71c84 + (longlong)local_c * 0x58) < *(short *)(param_3 + 8))) {
        *(undefined2 *)(&DAT_6bc71c84 + (longlong)local_c * 0x58) = *(undefined2 *)(param_3 + 8);
      }
      if ((*(uint *)(&g_aftFgInfo + (longlong)local_c * 0x58) < 5) ||
         (8 < *(uint *)(&g_aftFgInfo + (longlong)local_c * 0x58))) {
        if ((*(int *)(&g_aftFgInfo + (longlong)local_c * 0x58) != 0) &&
           (*(uint *)(&g_aftFgInfo + (longlong)local_c * 0x58) < 5)) {
          AFT_EdgeStateCheck(local_c,1,1,param_1,param_3);
        }
      }
      else {
        AFT_EdgeStateCheck(local_c,1,0,param_1,param_3);
      }
      if (*(int *)(&g_aftFgInfo + (longlong)local_c * 0x58) == 10) {
        *(ushort *)(&DAT_6bc71c86 + (longlong)local_c * 0x58) =
             *(ushort *)(&DAT_6bc71c86 + (longlong)local_c * 0x58) & 0xfbff;
        *(undefined8 *)(&DAT_6bc71cd0 + (longlong)local_c * 0x58) = *(undefined8 *)_refptr_g_curTime
        ;
        *(short *)(param_3 + 8) = *(short *)(param_3 + 8) + 1;
        *(short *)(&DAT_6bc71cae + (longlong)local_c * 0x58) =
             (short)*(undefined4 *)((longlong)local_c * 0x30 + param_1 + 8);
        *(short *)(&DAT_6bc71cb0 + (longlong)local_c * 0x58) =
             (short)*(undefined4 *)((longlong)local_c * 0x30 + param_1 + 0xc);
        *(int *)((longlong)local_c * 0x30 + param_1 + 8) =
             (int)*(short *)(&DAT_6bc71c92 + (longlong)local_c * 0x58);
        *(int *)((longlong)local_c * 0x30 + param_1 + 0xc) =
             (int)*(short *)(&DAT_6bc71c94 + (longlong)local_c * 0x58);
        *(undefined2 *)(&DAT_6bc71c8c + (longlong)local_c * 0x58) = DAT_6bc72002;
        *(undefined4 *)(&DAT_6bc71cb8 + (longlong)local_c * 0x58) = 2;
        AFT_T3Report(local_c,param_1,param_2);
        if (*_refptr_g_tsaLogLoglevel != '\0') {
          StrPrintf_Process(0,"AEFT_ESR MUST_start: x = %d, y = %d\n",
                            *(undefined4 *)((longlong)local_c * 0x30 + param_1 + 8),
                            *(undefined4 *)((longlong)local_c * 0x30 + param_1 + 0xc));
        }
      }
      else if (*(int *)(&g_aftFgInfo + (longlong)local_c * 0x58) != 0) {
        *(short *)(param_3 + 6) = *(short *)(param_3 + 6) + 1;
      }
    }
  }
  if (*(short *)(param_3 + 6) != 0) {
    AFT_HandGrabRecheck(param_1);
  }
  if (((*(short *)(param_3 + 8) != 0) && (*(longlong *)_refptr_g_curTime - g_lastRelTime < 400)) &&
     (DAT_6bc71c42 == 0)) {
    DAT_6bc71c42 = 1;
  }
  return;
}


// ============================================================================
// Function: AFT_AddReportReproc @ 6bad33da
// ============================================================================

void AFT_AddReportReproc(undefined8 param_1,longlong param_2,longlong param_3)

{
  int iVar1;
  
                    /* 0x933da  67  AFT_AddReportReproc */
  if (*(short *)(param_3 + 0xc) != 0) {
    iVar1 = (int)*(short *)(param_3 + 10);
    if (((*(ushort *)(&DAT_6bc71c86 + (longlong)iVar1 * 0x58) & 4) == 0) &&
       (*(short *)(param_3 + 8) == *(short *)(&DAT_6bc71c84 + (longlong)iVar1 * 0x58))) {
      *(undefined4 *)(&DAT_6bc71cb8 + (longlong)iVar1 * 0x58) = 2;
      AFT_UseBackInfo(iVar1,param_1);
      AFT_T3Report(iVar1,param_1,param_2);
      if (*(short *)(param_3 + 0xe) != 0) {
        *(undefined4 *)(param_2 + 0x3c4) = 1;
      }
      if (*_refptr_g_tsaLogLoglevel != '\0') {
        StrPrintf_Process(0,"AEFT_ARR[%d], add_a_report, palmInfo=%d\n",iVar1,
                          *(undefined4 *)(&DAT_6bc71c88 + (longlong)iVar1 * 0x58));
      }
    }
    else {
      *(undefined2 *)(param_3 + 0xc) = 0;
    }
  }
  return;
}


// ============================================================================
// Function: AFT_EdgeStateProcess @ 6bad3f7a
// ============================================================================

void AFT_EdgeStateProcess(longlong param_1,undefined8 param_2,undefined8 param_3)

{
  byte local_9;
  
  if (*(int *)(param_1 + 0x3c0) == 0) {
    if ((hasFinger_10971 != 0) && (hasFinger_10971 = 0, *_refptr_g_tsaLogLoglevel != '\0')) {
      StrPrintf_Process(0,"AEFT_ESP_all release\n");
    }
    for (local_9 = 0; local_9 < 10; local_9 = local_9 + 1) {
      AFT_ReleaseRecheck(local_9,param_3,1);
    }
  }
  else {
    hasFinger_10971 = 1;
    for (local_9 = 0; local_9 < 10; local_9 = local_9 + 1) {
      if ((*(short *)((longlong)(int)(uint)local_9 * 0x30 + param_1 + 0x20) == 0) ||
         (*(int *)((longlong)(int)(uint)local_9 * 0x30 + param_1 + 0x10) == 0)) {
        AFT_ReleaseRecheck(local_9,param_3,0);
      }
      else {
        AFT_TouchFingerProcess(local_9,param_1,param_2,param_3);
      }
    }
  }
  return;
}


// ============================================================================
// Function: AFT_TranferDataCompletely @ 6bad40ac
// ============================================================================

void AFT_TranferDataCompletely(longlong param_1,longlong param_2)

{
  int local_c;
  
  for (local_c = 0; local_c < 10; local_c = local_c + 1) {
    AFT_CopyInToOut(0,local_c,local_c,param_1,param_2);
  }
  *(undefined4 *)(param_2 + 0x3c0) = *(undefined4 *)(param_1 + 0x3c0);
  *(undefined4 *)(param_2 + 0x3c4) = 0;
  return;
}


// ============================================================================
// Function: AFT_ReinitPalmEdge @ 6baca561
// ============================================================================

void AFT_ReinitPalmEdge(void)

{
  int iVar1;
  ushort uVar2;
  
                    /* 0x8a561  82  AFT_ReinitPalmEdge */
  iVar1 = (*_refptr_g_tsaStaticPtr)->nField_0x264;
  if (iVar1 == 1) {
    DAT_6bc71c4a = DAT_6bc7200e;
    DAT_6bc71c4c = DAT_6bc7200e;
  }
  else if (iVar1 == 0) {
    DAT_6bc71c4a = DAT_6bc72020;
    DAT_6bc71c4c = DAT_6bc72020;
  }
  else if (iVar1 == 2) {
    DAT_6bc71c4a = DAT_6bc7201e;
    DAT_6bc71c4c = DAT_6bc7201e;
  }
  else if (iVar1 == 3) {
    DAT_6bc71c4a = DAT_6bc72010;
    DAT_6bc71c4c = DAT_6bc72010;
  }
  DAT_6bc71c48 = 0;
  uVar2 = DAT_6bc71c46 & 0xfffe;
  DAT_6bc71c46 = uVar2 | 2;
  if (((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e8 & 4) != 0) &&
     (*(int *)_refptr_g_foldedState != 0)) {
    DAT_6bc71c46 = uVar2 | 3;
  }
  if (((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e8 & 5) != 0) ||
     (((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e8 & 2) != 0 &&
      (*(int *)_refptr_g_foldedState == 0)))) {
    DAT_6bc71c46 = DAT_6bc71c46 & 0xfffd;
  }
  return;
}


// ============================================================================
// Function: AFT_CopyInToOut @ 6bad46a5
// ============================================================================

void AFT_CopyInToOut(int param_1,int param_2,int param_3,longlong param_4,longlong param_5)

{
                    /* 0x946a5  71  AFT_CopyInToOut */
  *(undefined4 *)((longlong)param_3 * 0x30 + param_5 + 4) =
       *(undefined4 *)((longlong)param_2 * 0x30 + param_4 + 4);
  *(undefined4 *)((longlong)param_3 * 0x30 + param_5) =
       *(undefined4 *)((longlong)param_2 * 0x30 + param_4);
  *(undefined4 *)((longlong)param_3 * 0x30 + param_5 + 8) =
       *(undefined4 *)((longlong)param_2 * 0x30 + param_4 + 8);
  *(undefined4 *)((longlong)param_3 * 0x30 + param_5 + 0xc) =
       *(undefined4 *)((longlong)param_2 * 0x30 + param_4 + 0xc);
  *(undefined4 *)((longlong)param_3 * 0x30 + param_5 + 0x10) =
       *(undefined4 *)((longlong)param_2 * 0x30 + param_4 + 0x10);
  *(undefined4 *)((longlong)param_3 * 0x30 + param_5 + 0x14) =
       *(undefined4 *)((longlong)param_2 * 0x30 + param_4 + 0x14);
  *(undefined4 *)((longlong)param_3 * 0x30 + param_5 + 0x18) =
       *(undefined4 *)((longlong)param_2 * 0x30 + param_4 + 0x18);
  *(undefined4 *)((longlong)param_3 * 0x30 + param_5 + 0x1c) =
       *(undefined4 *)((longlong)param_2 * 0x30 + param_4 + 0x1c);
  *(undefined2 *)((longlong)param_3 * 0x30 + param_5 + 0x20) =
       *(undefined2 *)((longlong)param_2 * 0x30 + param_4 + 0x20);
  *(char *)((longlong)param_3 * 0x30 + param_5 + 0x26) =
       (char)*(undefined2 *)((longlong)param_2 * 0x30 + param_4 + 0x22);
  *(char *)((longlong)param_3 * 0x30 + param_5 + 0x27) =
       (char)*(undefined2 *)((longlong)param_2 * 0x30 + param_4 + 0x24);
  *(undefined1 *)((longlong)param_3 * 0x30 + param_5 + 0x26) =
       *(undefined1 *)((longlong)param_2 * 0x30 + param_4 + 0x26);
  *(undefined1 *)((longlong)param_3 * 0x30 + param_5 + 0x27) =
       *(undefined1 *)((longlong)param_2 * 0x30 + param_4 + 0x27);
  *(undefined1 *)((longlong)param_3 * 0x30 + param_5 + 0x28) =
       *(undefined1 *)((longlong)param_2 * 0x30 + param_4 + 0x28);
  *(undefined1 *)((longlong)param_3 * 0x30 + param_5 + 0x29) =
       *(undefined1 *)((longlong)param_2 * 0x30 + param_4 + 0x29);
  *(undefined1 *)((longlong)param_3 * 0x30 + param_5 + 0x2a) =
       *(undefined1 *)((longlong)param_2 * 0x30 + param_4 + 0x2a);
  *(undefined1 *)((longlong)param_3 * 0x30 + param_5 + 0x2b) =
       *(undefined1 *)((longlong)param_2 * 0x30 + param_4 + 0x2b);
  *(undefined1 *)((longlong)param_3 * 0x30 + param_5 + 0x2c) =
       *(undefined1 *)((longlong)param_2 * 0x30 + param_4 + 0x2c);
  *(undefined1 *)((longlong)param_3 * 0x30 + param_5 + 0x2d) =
       *(undefined1 *)((longlong)param_2 * 0x30 + param_4 + 0x2d);
  *(undefined1 *)((longlong)param_3 * 0x30 + param_5 + 0x2e) =
       *(undefined1 *)((longlong)param_2 * 0x30 + param_4 + 0x2e);
  if (param_1 != 0) {
    *(undefined2 *)((longlong)param_3 * 0x30 + param_5 + 0x20) = 0x40;
  }
  return;
}


// ============================================================================
// Function: AFT_HandGrabRecheck @ 6bacccec
// ============================================================================

void AFT_HandGrabRecheck(longlong param_1)

{
  int iVar1;
  int iVar2;
  char cVar3;
  int local_14;
  int local_10;
  int local_c;
  
  for (local_c = 0; local_c < 5; local_c = local_c + 1) {
    if ((*(int *)(&g_aftFgInfo + (longlong)local_c * 0x58) == 6) ||
       (*(int *)(&g_aftFgInfo + (longlong)local_c * 0x58) == 7)) {
      iVar1 = *(int *)((longlong)local_c * 0x30 + param_1 + 8);
      iVar2 = *(int *)((longlong)local_c * 0x30 + param_1 + 0xc);
      cVar3 = TSAStatic_IsScreenRotated();
      if (cVar3 == '\0') {
        for (local_10 = 0; local_10 < 5; local_10 = local_10 + 1) {
          if ((*(int *)(&g_aftFgInfo + (longlong)local_10 * 0x58) != 0) && (local_c != local_10)) {
            if (((DAT_6bc72012 < iVar1) &&
                ((int)DAT_6bc72044 < *(int *)((longlong)local_10 * 0x30 + param_1 + 8))) ||
               ((iVar1 <= DAT_6bc72012 &&
                (*(int *)((longlong)local_10 * 0x30 + param_1 + 8) <= (int)DAT_6bc72042)))) {
              local_14 = iVar2 - *(int *)((longlong)local_10 * 0x30 + param_1 + 0xc);
              if (local_14 < 0) {
                local_14 = -local_14;
              }
              if ((*(ushort *)(&DAT_6bc71c86 + (longlong)local_c * 0x58) & 0x10) == 0) {
                if (local_14 < DAT_6bc72028) {
                  *(undefined4 *)(&g_aftFgInfo + (longlong)local_c * 0x58) = 8;
                  *(ushort *)(&DAT_6bc71c86 + (longlong)local_c * 0x58) =
                       *(ushort *)(&DAT_6bc71c86 + (longlong)local_c * 0x58) | 0x80;
                }
                else if ((local_14 < DAT_6bc72028 * 2) &&
                        (-1 < (*(longlong *)(&DAT_6bc71cc0 + (longlong)local_10 * 0x58) -
                              *(longlong *)(&DAT_6bc71cc0 + (longlong)local_c * 0x58)) + 1000)) {
                  *(undefined4 *)(&g_aftFgInfo + (longlong)local_c * 0x58) = 8;
                  *(ushort *)(&DAT_6bc71c86 + (longlong)local_c * 0x58) =
                       *(ushort *)(&DAT_6bc71c86 + (longlong)local_c * 0x58) | 0x80;
                }
              }
            }
            else if (((((*_refptr_g_tsaStaticPtr)->nField_0x264 == 0) && (iVar2 < DAT_6bc72022)) ||
                     (((*_refptr_g_tsaStaticPtr)->nField_0x264 == 2 && (DAT_6bc72022 < iVar2)))) &&
                    (-1 < (*(longlong *)(&DAT_6bc71cc0 + (longlong)local_10 * 0x58) -
                          *(longlong *)(&DAT_6bc71cc0 + (longlong)local_c * 0x58)) + 500)) {
              *(undefined4 *)(&g_aftFgInfo + (longlong)local_c * 0x58) = 8;
              *(ushort *)(&DAT_6bc71c86 + (longlong)local_c * 0x58) =
                   *(ushort *)(&DAT_6bc71c86 + (longlong)local_c * 0x58) | 0x80;
            }
          }
        }
      }
      else {
        for (local_10 = 0; local_10 < 5; local_10 = local_10 + 1) {
          if ((((5 < *(uint *)(&g_aftFgInfo + (longlong)local_10 * 0x58)) &&
               (*(uint *)(&g_aftFgInfo + (longlong)local_10 * 0x58) < 10)) && (local_c != local_10))
             && ((*(ushort *)(&DAT_6bc71c86 + (longlong)local_c * 0x58) & 0x20) == 0)) {
            *(undefined4 *)(&g_aftFgInfo + (longlong)local_c * 0x58) = 8;
          }
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: AFT_EdgeStateCheck @ 6bad2657
// ============================================================================

void AFT_EdgeStateCheck(int param_1,int param_2,undefined4 param_3,longlong param_4)

{
  undefined8 in_stack_fffffffffffffee8;
  undefined4 uVar1;
  undefined1 local_88 [44];
  undefined4 local_5c;
  undefined4 local_58;
  undefined4 local_54;
  int local_50;
  short local_4a;
  
  uVar1 = (undefined4)((ulonglong)in_stack_fffffffffffffee8 >> 0x20);
  local_4a = (short)*(undefined4 *)(&g_aftFgInfo + (longlong)param_1 * 0x58);
  if (param_2 != 0) {
    AFT_DiffWew(param_1,param_4);
  }
  local_50 = memset_s(local_88,0x20,0,0x20);
  if (local_50 != 0) {
    uVar1 = 0;
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/AntiFalseTouch/AntiEdgeFalseTouch.c",0x886,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  AFT_CheckInfoPrepare(param_1,param_3,param_4,local_88);
  local_54 = AFT_PalmFeatureProc(param_1,local_88);
  local_58 = AFT_PositionFeatureProc(param_1,local_88);
  local_5c = AFT_HorizontalPlacedProc(param_1,local_88);
  AFT_GetFinalState(param_1,param_2,param_4,local_88);
  if (((param_2 != 2) && (*(int *)(&g_aftFgInfo + (longlong)param_1 * 0x58) != 0)) &&
     (*(uint *)(&g_aftFgInfo + (longlong)param_1 * 0x58) < 10)) {
    *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) =
         *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) | 0x200;
  }
  if (((param_2 != 0) && ((int)local_4a != *(int *)(&g_aftFgInfo + (longlong)param_1 * 0x58))) &&
     ((local_4a != 0 && (*_refptr_g_tsaLogLoglevel != '\0')))) {
    StrPrintf_Process(0,
                      "AEFT_ESC_%d, %d->%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, 0x%x, 0x%x\n"
                      ,param_1,(int)local_4a,
                      CONCAT44(uVar1,*(undefined4 *)(&g_aftFgInfo + (longlong)param_1 * 0x58)),
                      param_2,param_3,local_54,local_58,local_5c,
                      (*_refptr_g_tsaStaticPtr)->nField_0x264,(int)DAT_6bc71c4a,(int)DAT_6bc71c4c,
                      (int)DAT_6bc71c44,(int)DAT_6bc71c42,(int)g_aftState,
                      *(undefined4 *)((longlong)param_1 * 0x30 + param_4 + 8),
                      *(undefined4 *)((longlong)param_1 * 0x30 + param_4 + 0xc),
                      *(undefined4 *)(&DAT_6bc71c88 + (longlong)param_1 * 0x58),DAT_6bc71c48,
                      *(undefined2 *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58));
  }
  return;
}


// ============================================================================
// Function: AFT_UseBackInfo @ 6bad30a2
// ============================================================================

void AFT_UseBackInfo(int param_1,longlong param_2)

{
                    /* 0x930a2  84  AFT_UseBackInfo */
  *(int *)((longlong)param_1 * 0x30 + param_2 + 4) =
       (int)*(short *)(&DAT_6bc71c8e + (longlong)param_1 * 0x58);
  *(int *)((longlong)param_1 * 0x30 + param_2) =
       (int)*(short *)(&DAT_6bc71c90 + (longlong)param_1 * 0x58);
  *(int *)((longlong)param_1 * 0x30 + param_2 + 8) =
       (int)*(short *)(&DAT_6bc71c92 + (longlong)param_1 * 0x58);
  *(int *)((longlong)param_1 * 0x30 + param_2 + 0xc) =
       (int)*(short *)(&DAT_6bc71c94 + (longlong)param_1 * 0x58);
  *(uint *)((longlong)param_1 * 0x30 + param_2 + 0x10) =
       (uint)*(ushort *)(&DAT_6bc71c96 + (longlong)param_1 * 0x58);
  *(char *)((longlong)param_1 * 0x30 + param_2 + 0x26) =
       (char)(*(ushort *)(&DAT_6bc71c98 + (longlong)param_1 * 0x58) / 10);
  *(char *)((longlong)param_1 * 0x30 + param_2 + 0x27) =
       (char)(*(ushort *)(&DAT_6bc71c9a + (longlong)param_1 * 0x58) / 10);
  *(int *)((longlong)param_1 * 0x30 + param_2 + 0x14) =
       (int)*(short *)(&DAT_6bc71ca8 + (longlong)param_1 * 0x58);
  *(int *)((longlong)param_1 * 0x30 + param_2 + 0x18) =
       (int)*(short *)(&DAT_6bc71caa + (longlong)param_1 * 0x58);
  *(int *)((longlong)param_1 * 0x30 + param_2 + 0x1c) =
       (int)*(short *)(&DAT_6bc71cac + (longlong)param_1 * 0x58);
  return;
}


// ============================================================================
// Function: AFT_ReleaseRecheck @ 6bacd94e
// ============================================================================

void AFT_ReleaseRecheck(int param_1,longlong param_2,int param_3)

{
  short sVar1;
  short sVar2;
  uint uVar3;
  char cVar4;
  int iVar5;
  longlong lVar6;
  undefined2 uVar7;
  longlong local_18;
  
  sVar1 = *(short *)(&DAT_6bc71c92 + (longlong)param_1 * 0x58);
  sVar2 = *(short *)(&DAT_6bc71c94 + (longlong)param_1 * 0x58);
  uVar7 = (undefined2)param_1;
  if ((*(int *)(&g_aftFgInfo + (longlong)param_1 * 0x58) == 5) ||
     (*(int *)(&g_aftFgInfo + (longlong)param_1 * 0x58) == 1)) {
    *(short *)(param_2 + 0xc) = *(short *)(param_2 + 0xc) + 1;
    *(undefined2 *)(param_2 + 10) = uVar7;
    if (param_3 != 0) {
      *(undefined2 *)(param_2 + 0xe) = 1;
    }
    goto LAB_6bace219;
  }
  if ((((*(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) & 8) != 0) ||
      (uVar3 = *(uint *)(&g_aftFgInfo + (longlong)param_1 * 0x58), uVar3 < 2)) ||
     ((3 < uVar3 && (1 < uVar3 - 6)))) goto LAB_6bace219;
  if ((*(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) & 0x10) != 0) {
    cVar4 = TSAStatic_IsScreenRotated();
    if (cVar4 == '\0') {
      if (DAT_6bc7207a < *(short *)(&DAT_6bc71cb2 + (longlong)param_1 * 0x58)) goto LAB_6bace219;
      if ((0 < *(short *)(&DAT_6bc71cb2 + (longlong)param_1 * 0x58)) ||
         (*(int *)(&g_aftFgInfo + (longlong)param_1 * 0x58) == 7)) {
        if ((*(ushort *)(&DAT_6bc71c98 + (longlong)param_1 * 0x58) != 0) &&
           ((int)DAT_6bc7207c <
            (int)(((uint)*(ushort *)(&DAT_6bc71c9a + (longlong)param_1 * 0x58) * 10) /
                 (uint)*(ushort *)(&DAT_6bc71c98 + (longlong)param_1 * 0x58)))) {
          if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 8) != 0) goto LAB_6bace219;
          iVar5 = AFT_PalmSameSide(0,(int)sVar1,(int)sVar2);
joined_r0x6bacde0b:
          if (iVar5 != 0) goto LAB_6bace219;
        }
      }
    }
    else {
      if (DAT_6bc7207a < *(short *)(&DAT_6bc71cb2 + (longlong)param_1 * 0x58)) goto LAB_6bace219;
      if ((0 < *(short *)(&DAT_6bc71cb2 + (longlong)param_1 * 0x58)) ||
         (*(int *)(&g_aftFgInfo + (longlong)param_1 * 0x58) == 3)) {
        if ((*(ushort *)(&DAT_6bc71c9a + (longlong)param_1 * 0x58) != 0) &&
           ((int)DAT_6bc7207c <
            (int)(((uint)*(ushort *)(&DAT_6bc71c98 + (longlong)param_1 * 0x58) * 10) /
                 (uint)*(ushort *)(&DAT_6bc71c9a + (longlong)param_1 * 0x58)))) {
          if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 8) != 0) goto LAB_6bace219;
          iVar5 = AFT_PalmSameSide(1,(int)sVar1,(int)sVar2);
          goto joined_r0x6bacde0b;
        }
      }
    }
  }
  if ((((*(short *)(&DAT_6bc71c84 + (longlong)param_1 * 0x58) == 0) ||
       (*(uint *)(&DAT_6bc71c88 + (longlong)param_1 * 0x58) < 4)) &&
      (lVar6 = *(longlong *)_refptr_g_curTime -
               *(longlong *)(&DAT_6bc71cc0 + (longlong)param_1 * 0x58), 0x1e < lVar6)) &&
     ((iVar5 = AFT_MustRelation(param_1), iVar5 == 0 &&
      ((*(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) & 0x400) == 0)))) {
    if ((*(int *)(&g_aftFgInfo + (longlong)param_1 * 0x58) == 6) ||
       (*(int *)(&g_aftFgInfo + (longlong)param_1 * 0x58) == 2)) {
      local_18 = 0xc1c;
      if (((DAT_6bc71c44 != 0) || (DAT_6bc71c42 != 0)) ||
         (3 < *(uint *)(&DAT_6bc71c88 + (longlong)param_1 * 0x58))) {
        local_18 = 200;
      }
      if ((lVar6 < local_18) || (*(short *)(&DAT_6bc71c84 + (longlong)param_1 * 0x58) == 0)) {
        *(short *)(param_2 + 0xc) = *(short *)(param_2 + 0xc) + 1;
        if (*(short *)(param_2 + 0xc) == 1) {
          *(undefined2 *)(param_2 + 10) = uVar7;
        }
        else {
          iVar5 = AFT_SelectAdd(param_1,param_2);
          if (iVar5 != 0) {
            *(undefined2 *)(param_2 + 10) = uVar7;
          }
        }
        if (param_3 != 0) {
          *(undefined2 *)(param_2 + 0xe) = 1;
        }
      }
    }
    else if ((*(int *)(&g_aftFgInfo + (longlong)param_1 * 0x58) == 7) ||
            (*(int *)(&g_aftFgInfo + (longlong)param_1 * 0x58) == 3)) {
      local_18 = 1000;
      if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 0x80) != 0) {
        local_18 = 0x226;
      }
      if (((DAT_6bc71c44 == 0) && (DAT_6bc71c42 == 0)) &&
         (*(uint *)(&DAT_6bc71c88 + (longlong)param_1 * 0x58) < 4)) {
        if (*(short *)(&DAT_6bc71c84 + (longlong)param_1 * 0x58) != 0) {
          local_18 = 0x226;
        }
      }
      else {
        local_18 = 200;
      }
      if (lVar6 < local_18) {
        *(short *)(param_2 + 0xc) = *(short *)(param_2 + 0xc) + 1;
        if (*(short *)(param_2 + 0xc) == 1) {
          *(undefined2 *)(param_2 + 10) = uVar7;
        }
        else {
          iVar5 = AFT_SelectAdd(param_1,param_2);
          if (iVar5 != 0) {
            *(undefined2 *)(param_2 + 10) = uVar7;
          }
        }
        if (param_3 != 0) {
          *(undefined2 *)(param_2 + 0xe) = 1;
        }
      }
    }
  }
LAB_6bace219:
  *(undefined4 *)(&g_aftFgInfo + (longlong)param_1 * 0x58) = 0;
  *(undefined4 *)(&DAT_6bc71cb8 + (longlong)param_1 * 0x58) = 0;
  return;
}


// ============================================================================
// Function: AFT_TouchFingerProcess @ 6bad354a
// ============================================================================

void AFT_TouchFingerProcess(byte param_1,longlong param_2,undefined8 param_3,uint *param_4)

{
  char cVar1;
  int iVar2;
  
  AFT_FirstPrepare(param_1,param_2);
  if (*(int *)(&g_aftFgInfo + (longlong)(int)(uint)param_1 * 0x58) == 0) {
    AFT_InTheCorner(param_1,param_2);
    iVar2 = AFT_AboveFingerprint
                      (*(undefined4 *)((longlong)(int)(uint)param_1 * 0x30 + param_2 + 8),
                       *(undefined4 *)((longlong)(int)(uint)param_1 * 0x30 + param_2 + 0xc));
    if (iVar2 == 0) {
      cVar1 = TSAStatic_IsScreenRotated();
      AFT_EdgeStateCheck(param_1,0,cVar1 != '\0',param_2,param_4);
    }
    else {
      *(undefined4 *)(&g_aftFgInfo + (longlong)(int)(uint)param_1 * 0x58) = 10;
    }
    if (*(int *)(&g_aftFgInfo + (longlong)(int)(uint)param_1 * 0x58) == 10) {
      *(short *)(param_4 + 2) = (short)param_4[2] + 1;
      *(undefined4 *)(&DAT_6bc71cb8 + (longlong)(int)(uint)param_1 * 0x58) = 1;
      AFT_T3Report(param_1,param_2,param_3);
    }
  }
  else if (*(int *)(&g_aftFgInfo + (longlong)(int)(uint)param_1 * 0x58) == 9) {
    if (((*(ushort *)(&DAT_6bc71c86 + (longlong)(int)(uint)param_1 * 0x58) & 0x100) == 0) &&
       ((*(ushort *)(&DAT_6bc71c86 + (longlong)(int)(uint)param_1 * 0x58) & 3) == 3)) {
      *(undefined4 *)(&g_aftFgInfo + (longlong)(int)(uint)param_1 * 0x58) = 10;
      *(undefined4 *)(&DAT_6bc71cb8 + (longlong)(int)(uint)param_1 * 0x58) = 2;
      *(undefined2 *)(&DAT_6bc71c8c + (longlong)(int)(uint)param_1 * 0x58) = DAT_6bc72002;
      AFT_UseBackInfo(param_1,param_2);
      AFT_T3Report(param_1,param_2,param_3);
      *(short *)(param_4 + 2) = (short)param_4[2] + 1;
    }
  }
  else if (*(int *)(&g_aftFgInfo + (longlong)(int)(uint)param_1 * 0x58) == 10) {
    if ((*(ushort *)(&DAT_6bc71c86 + (longlong)(int)(uint)param_1 * 0x58) & 0x200) != 0) {
      if ((int)DAT_6bc7204a <
          (*(int *)((longlong)(int)(uint)param_1 * 0x30 + param_2 + 0xc) -
          (int)*(short *)(&DAT_6bc71cb0 + (longlong)(int)(uint)param_1 * 0x58)) *
          (*(int *)((longlong)(int)(uint)param_1 * 0x30 + param_2 + 0xc) -
          (int)*(short *)(&DAT_6bc71cb0 + (longlong)(int)(uint)param_1 * 0x58)) +
          (*(int *)((longlong)(int)(uint)param_1 * 0x30 + param_2 + 8) -
          (int)*(short *)(&DAT_6bc71cae + (longlong)(int)(uint)param_1 * 0x58)) *
          (*(int *)((longlong)(int)(uint)param_1 * 0x30 + param_2 + 8) -
          (int)*(short *)(&DAT_6bc71cae + (longlong)(int)(uint)param_1 * 0x58))) {
        *(ushort *)(&DAT_6bc71c86 + (longlong)(int)(uint)param_1 * 0x58) =
             *(ushort *)(&DAT_6bc71c86 + (longlong)(int)(uint)param_1 * 0x58) & 0xfbff;
        *(undefined8 *)(&DAT_6bc71cd0 + (longlong)(int)(uint)param_1 * 0x58) =
             *(undefined8 *)_refptr_g_curTime;
      }
      else if ((*(ushort *)(&DAT_6bc71c86 + (longlong)(int)(uint)param_1 * 0x58) & 0x400) == 0) {
        if (1000 < *(longlong *)_refptr_g_curTime -
                   *(longlong *)(&DAT_6bc71cd0 + (longlong)(int)(uint)param_1 * 0x58)) {
          *(ushort *)(&DAT_6bc71c86 + (longlong)(int)(uint)param_1 * 0x58) =
               *(ushort *)(&DAT_6bc71c86 + (longlong)(int)(uint)param_1 * 0x58) | 0x400;
          *(undefined8 *)(&DAT_6bc71cd0 + (longlong)(int)(uint)param_1 * 0x58) =
               *(undefined8 *)_refptr_g_curTime;
        }
      }
      else {
        cVar1 = TSAStatic_IsScreenRotated();
        AFT_EdgeStateCheck(param_1,2,cVar1 != '\0',param_2,param_4);
        if (((*(uint *)(&g_aftFgInfo + (longlong)(int)(uint)param_1 * 0x58) < 3) ||
            (4 < *(uint *)(&g_aftFgInfo + (longlong)(int)(uint)param_1 * 0x58))) &&
           ((*(uint *)(&g_aftFgInfo + (longlong)(int)(uint)param_1 * 0x58) < 7 ||
            (9 < *(uint *)(&g_aftFgInfo + (longlong)(int)(uint)param_1 * 0x58))))) {
          *(ushort *)(&DAT_6bc71c86 + (longlong)(int)(uint)param_1 * 0x58) =
               *(ushort *)(&DAT_6bc71c86 + (longlong)(int)(uint)param_1 * 0x58) & 0xfbff;
          *(undefined8 *)(&DAT_6bc71cd0 + (longlong)(int)(uint)param_1 * 0x58) =
               *(undefined8 *)_refptr_g_curTime;
          *(undefined4 *)(&g_aftFgInfo + (longlong)(int)(uint)param_1 * 0x58) = 10;
        }
        else if (*(longlong *)_refptr_g_curTime -
                 *(longlong *)(&DAT_6bc71cd0 + (longlong)(int)(uint)param_1 * 0x58) < 0x3e9) {
          *(undefined4 *)(&g_aftFgInfo + (longlong)(int)(uint)param_1 * 0x58) = 10;
        }
        else {
          *param_4 = *param_4 | 1 << (param_1 & 0x1f);
          *(short *)(param_4 + 1) = (short)param_4[1] + 1;
          *(short *)(&DAT_6bc71c92 + (longlong)(int)(uint)param_1 * 0x58) =
               (short)*(undefined4 *)((longlong)(int)(uint)param_1 * 0x30 + param_2 + 8);
          *(short *)(&DAT_6bc71c94 + (longlong)(int)(uint)param_1 * 0x58) =
               (short)*(undefined4 *)((longlong)(int)(uint)param_1 * 0x30 + param_2 + 0xc);
          *(undefined8 *)(&DAT_6bc71cc0 + (longlong)(int)(uint)param_1 * 0x58) =
               *(undefined8 *)_refptr_g_curTime;
        }
      }
    }
    if (*(int *)(&g_aftFgInfo + (longlong)(int)(uint)param_1 * 0x58) == 10) {
      AFT_MustStateProc(param_1,param_2,param_3);
      *(short *)(param_4 + 2) = (short)param_4[2] + 1;
    }
  }
  return;
}


// ============================================================================
// Function: AFT_PalmFeatureProc @ 6bacefe7
// ============================================================================

undefined4 AFT_PalmFeatureProc(int param_1,ushort *param_2)

{
  int iVar1;
  ushort uVar2;
  char cVar3;
  undefined4 local_c;
  
  local_c = 0;
  uVar2 = 0;
  iVar1 = *(int *)(&DAT_6bc71c88 + (longlong)param_1 * 0x58);
  cVar3 = TSAStatic_IsScreenRotated();
  if ((((cVar3 == '\0') && (param_2[4] == 0)) ||
      ((cVar3 = TSAStatic_IsScreenRotated(), cVar3 != '\0' && (param_2[4] == 1)))) && (1 < iVar1)) {
    if (iVar1 == 6) {
      local_c = 1;
      uVar2 = 9;
      *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) =
           *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) | 0x100;
    }
    else if (iVar1 == 5) {
      local_c = 2;
      uVar2 = 9;
    }
    else if ((iVar1 == 4) &&
            ((((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 0x20) == 0 ||
              ((*param_2 & 0x20) != 0)) || (param_2[0xb] != 0)))) {
      local_c = 3;
      uVar2 = 4;
    }
    else if (((iVar1 == 2) && (param_2[5] != 0)) &&
            ((*(short *)(&DAT_6bc71c84 + (longlong)param_1 * 0x58) != 0 &&
             ((*(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) & 0x10) != 0)))) {
      local_c = 4;
      uVar2 = 4;
    }
    else if (((iVar1 == 3) && ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 4) != 0)) &&
            ((((*param_2 & 4) != 0 && ((*param_2 & 0x10) != 0)) ||
             ((((*param_2 & 0x20) != 0 && (param_2[5] != 0)) && (2 < *(uint *)(param_2 + 2))))))) {
      local_c = 5;
      uVar2 = 4;
    }
    else if ((iVar1 == 4) ||
            ((iVar1 == 2 && (*(short *)(&DAT_6bc71c84 + (longlong)param_1 * 0x58) != 0)))) {
      local_c = 6;
      uVar2 = 3;
    }
    else if (((iVar1 == 3) && (*(short *)(&DAT_6bc71c84 + (longlong)param_1 * 0x58) != 0)) &&
            ((1 < *(uint *)(param_2 + 2) || ((*param_2 & 0x20) != 0)))) {
      local_c = 7;
      uVar2 = 3;
      *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) =
           *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) | 0x80;
    }
    else if ((iVar1 == 3) || (iVar1 == 2)) {
      local_c = 8;
      uVar2 = 2;
    }
  }
  if ((short)param_2[0xd] < (short)uVar2) {
    param_2[0xd] = uVar2;
  }
  return local_c;
}


// ============================================================================
// Function: AFT_PositionFeatureProc @ 6bacf38c
// ============================================================================

undefined4 AFT_PositionFeatureProc(int param_1,ushort *param_2)

{
  int iVar1;
  ushort uVar2;
  char cVar3;
  undefined4 local_c;
  
  local_c = 0;
  uVar2 = 0;
  iVar1 = *(int *)(&DAT_6bc71c88 + (longlong)param_1 * 0x58);
  if (((*(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) & 0x10) != 0) &&
     ((((param_2[0xb] != 0 || ((*param_2 & 0x10) != 0)) && (0 < (short)param_2[7])) &&
      (((short)param_2[7] < (short)param_2[8] &&
       ((int)DAT_6bc72078 < (int)(short)param_2[7] + (int)(short)param_2[8])))))) {
    local_c = 1;
    uVar2 = 9;
    goto LAB_6bacf7e7;
  }
  if (((*(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) & 0x10) != 0) &&
     ((DAT_6bc7207a < *(short *)(&DAT_6bc71cb2 + (longlong)param_1 * 0x58) && (param_2[5] != 0)))) {
    local_c = 2;
    uVar2 = 4;
    goto LAB_6bacf7e7;
  }
  if ((((*(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) & 0x10) != 0) &&
      ((*param_2 & 4) != 0)) ||
     (((*(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) & 0x10) == 0 && ((*param_2 & 8) != 0))
     )) {
    local_c = 3;
    uVar2 = 4;
    goto LAB_6bacf7e7;
  }
  if (4 < *(uint *)(param_2 + 2)) {
    local_c = 4;
    uVar2 = 4;
    goto LAB_6bacf7e7;
  }
  if (3 < *(uint *)(param_2 + 2)) {
    if ((((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 4) == 0) || ((*param_2 & 0x20) == 0))
       || (((*param_2 & 4) == 0 &&
           ((iVar1 != 3 || (*(short *)(&DAT_6bc71c84 + (longlong)param_1 * 0x58) == 0)))))) {
      local_c = 6;
      uVar2 = 3;
    }
    else {
      local_c = 5;
      uVar2 = 4;
    }
    goto LAB_6bacf7e7;
  }
  if ((((*param_2 & 1) != 0) && (param_2[0xc] != 0)) && (1 < *(uint *)(param_2 + 2))) {
    if (param_2[4] == 0) {
      cVar3 = TSAStatic_IsScreenRotated();
      if (cVar3 == '\0') goto LAB_6bacf6a7;
LAB_6bacf6be:
      local_c = 7;
      uVar2 = 3;
      goto LAB_6bacf7e7;
    }
LAB_6bacf6a7:
    if (param_2[4] == 1) {
      cVar3 = TSAStatic_IsScreenRotated();
      if (cVar3 == '\0') goto LAB_6bacf6be;
    }
  }
  if (*(uint *)(param_2 + 2) < 3) {
    if ((*(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) & 0x10) == 0) {
      if ((1 < *(uint *)(param_2 + 2)) &&
         ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 0x1002) != 0)) {
        local_c = 0xb;
        uVar2 = 1;
      }
    }
    else {
      local_c = 10;
      uVar2 = 2;
    }
  }
  else if ((((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 4) == 0) ||
           ((*param_2 & 0x20) == 0)) ||
          (((*param_2 & 4) == 0 &&
           ((iVar1 != 3 || (*(short *)(&DAT_6bc71c84 + (longlong)param_1 * 0x58) == 0)))))) {
    local_c = 9;
    uVar2 = 2;
  }
  else {
    local_c = 8;
    uVar2 = 3;
  }
LAB_6bacf7e7:
  if ((short)param_2[0xd] < (short)uVar2) {
    param_2[0xd] = uVar2;
  }
  return local_c;
}


// ============================================================================
// Function: AFT_HorizontalPlacedProc @ 6bacf80b
// ============================================================================

undefined4 AFT_HorizontalPlacedProc(int param_1,ushort *param_2)

{
  bool bVar1;
  ushort uVar2;
  undefined4 local_c;
  
  local_c = 0;
  uVar2 = 0;
  bVar1 = false;
  if (((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 1) != 0) || (param_2[0xc] == 0)) {
    if ((short)param_2[6] < 5) {
      if (((short)param_2[9] < (short)param_2[7]) && (2 < (short)param_2[6])) {
        local_c = 2;
        uVar2 = 4;
        bVar1 = true;
      }
      else if (((short)param_2[10] < (short)param_2[7]) && (2 < (short)param_2[6])) {
        local_c = 3;
        uVar2 = 3;
        bVar1 = true;
      }
      else if ((short)param_2[6] < 3) {
        if ((*(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) & 0x20) == 0) {
          if (1 < (short)param_2[6]) {
            if ((*param_2 & 2) == 0) {
              if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 0x1002) != 0) {
                local_c = 7;
                uVar2 = 1;
              }
            }
            else {
              local_c = 6;
              uVar2 = 2;
            }
          }
        }
        else {
          local_c = 5;
          uVar2 = 2;
        }
      }
      else {
        local_c = 4;
        uVar2 = 3;
        if (3 < (short)param_2[6]) {
          bVar1 = true;
        }
      }
    }
    else {
      local_c = 1;
      uVar2 = 4;
      bVar1 = true;
    }
  }
  if ((bVar1) && (param_2[0xc] == 0)) {
    *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) =
         *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) | 0x80;
  }
  param_2[0xe] = uVar2;
  return local_c;
}


// ============================================================================
// Function: AFT_CheckInfoPrepare @ 6bad0d68
// ============================================================================

void AFT_CheckInfoPrepare(int param_1,int param_2,longlong param_3,ushort *param_4)

{
  ushort uVar1;
  int local_18;
  int local_14;
  undefined4 local_10;
  undefined4 local_c;
  
  param_4[4] = (ushort)param_2;
  local_14 = *(int *)((longlong)param_1 * 0x30 + param_3 + 8);
  local_18 = *(int *)((longlong)param_1 * 0x30 + param_3 + 0xc);
  uVar1 = AFT_PalmSameSide(param_2,local_14,local_18);
  param_4[5] = uVar1;
  local_c = AFT_XEdgeState(local_14);
  local_10 = AFT_YEdgeState(local_18);
  AFT_NewXYValue(&local_14,&local_18);
  if (param_2 == 0) {
    *(undefined4 *)(param_4 + 2) = local_c;
    param_4[6] = (ushort)local_10;
    param_4[7] = *(ushort *)(&DAT_6bc71ca4 + (longlong)param_1 * 0x58);
    param_4[8] = *(ushort *)(&DAT_6bc71ca6 + (longlong)param_1 * 0x58);
    param_4[9] = DAT_6bc7206a;
    param_4[10] = DAT_6bc72068;
    param_4[0xb] = DAT_6bc71c46 & 1;
    param_4[0xc] = DAT_6bc71c46 & 2;
    if (DAT_6bc72082 < *(short *)(&DAT_6bc71cb4 + (longlong)param_1 * 0x58)) {
      *param_4 = *param_4 | 1;
    }
    if (DAT_6bc72084 < *(short *)(&DAT_6bc71cb6 + (longlong)param_1 * 0x58)) {
      *param_4 = *param_4 | 2;
    }
    if (DAT_6bc72086 < *(short *)(&DAT_6bc71cb4 + (longlong)param_1 * 0x58)) {
      *param_4 = *param_4 | 4;
    }
    if (DAT_6bc7207e < *(short *)(&DAT_6bc71cb4 + (longlong)param_1 * 0x58)) {
      *param_4 = *param_4 | 8;
    }
    if (DAT_6bc72022 < local_18) {
      *param_4 = *param_4 | 0x10;
    }
    if (DAT_6bc72026 < local_18) {
      *param_4 = *param_4 | 0x20;
    }
  }
  else if (param_2 == 1) {
    *(undefined4 *)(param_4 + 2) = local_10;
    param_4[6] = (ushort)local_c;
    param_4[7] = *(ushort *)(&DAT_6bc71ca6 + (longlong)param_1 * 0x58);
    param_4[8] = *(ushort *)(&DAT_6bc71ca4 + (longlong)param_1 * 0x58);
    param_4[9] = DAT_6bc72072;
    param_4[10] = DAT_6bc72070;
    param_4[0xb] = DAT_6bc71c46 & 2;
    param_4[0xc] = DAT_6bc71c46 & 1;
    if (DAT_6bc72084 < *(short *)(&DAT_6bc71cb6 + (longlong)param_1 * 0x58)) {
      *param_4 = *param_4 | 1;
    }
    if (DAT_6bc72082 < *(short *)(&DAT_6bc71cb4 + (longlong)param_1 * 0x58)) {
      *param_4 = *param_4 | 2;
    }
    if (DAT_6bc72088 < *(short *)(&DAT_6bc71cb6 + (longlong)param_1 * 0x58)) {
      *param_4 = *param_4 | 4;
    }
    if (DAT_6bc72080 < *(short *)(&DAT_6bc71cb6 + (longlong)param_1 * 0x58)) {
      *param_4 = *param_4 | 8;
    }
    if (local_14 < DAT_6bc72012) {
      *param_4 = *param_4 | 0x10;
    }
    if (local_14 < DAT_6bc72014) {
      *param_4 = *param_4 | 0x20;
    }
  }
  return;
}


// ============================================================================
// Function: AFT_GetFinalState @ 6bad1314
// ============================================================================

void AFT_GetFinalState(int param_1,int param_2,undefined8 param_3,longlong param_4)

{
  uint local_10;
  uint local_c;
  
  local_c = (uint)*(short *)(param_4 + 0x1a);
  if ((param_2 != 2) &&
     ((((DAT_6bc71c44 != 0 || (DAT_6bc71c42 != 0)) && (1 < (int)local_c)) && ((int)local_c < 4)))) {
    local_c = local_c + 1;
  }
  if ((int)local_c < (int)*(short *)(param_4 + 0x1c)) {
    local_c = (uint)*(short *)(param_4 + 0x1c);
    local_10 = local_c;
    if (((*(short *)(param_4 + 8) == 1) && (local_c != 0)) && ((int)local_c < 5)) {
      local_10 = local_c + 4;
    }
  }
  else {
    local_10 = local_c;
    if (((*(short *)(param_4 + 8) == 0) && (local_c != 0)) && ((int)local_c < 5)) {
      local_10 = local_c + 4;
    }
  }
  if (local_c != 0) {
    *(undefined8 *)(&DAT_6bc71cc8 + (longlong)param_1 * 0x58) = *(undefined8 *)_refptr_g_curTime;
  }
  if (param_2 == 1) {
    if (*(short *)(param_4 + 8) == 0) {
      if (*(int *)(&g_aftFgInfo + (longlong)param_1 * 0x58) - 4U < local_c) {
        *(uint *)(&g_aftFgInfo + (longlong)param_1 * 0x58) = local_10;
      }
      else {
        AFT_YAxisEdgeRecovery(param_1,param_3,param_4);
      }
    }
    else if (*(short *)(param_4 + 8) == 1) {
      if (*(uint *)(&g_aftFgInfo + (longlong)param_1 * 0x58) < local_c) {
        *(uint *)(&g_aftFgInfo + (longlong)param_1 * 0x58) = local_10;
      }
      else {
        AFT_XAxisEdgeRecovery(param_1,param_3,param_4);
      }
    }
    AFT_TimeoutRecovery(param_1,(int)*(short *)(param_4 + 8));
  }
  else if (local_c == 0) {
    *(undefined4 *)(&g_aftFgInfo + (longlong)param_1 * 0x58) = 10;
  }
  else {
    *(uint *)(&g_aftFgInfo + (longlong)param_1 * 0x58) = local_10;
  }
  return;
}


// ============================================================================
// Function: AFT_DiffWew @ 6bad230f
// ============================================================================

void AFT_DiffWew(int param_1)

{
  ushort uVar1;
  ushort uVar2;
  int iVar3;
  
  uVar1 = *(ushort *)(&DAT_6bc71ca4 + (longlong)param_1 * 0x58);
  uVar2 = *(ushort *)(&DAT_6bc71ca6 + (longlong)param_1 * 0x58);
  if (uVar1 < *(ushort *)(&DAT_6bc71c9c + (longlong)param_1 * 0x58)) {
    *(ushort *)(&DAT_6bc71c9c + (longlong)param_1 * 0x58) = uVar1;
  }
  else if (*(ushort *)(&DAT_6bc71ca0 + (longlong)param_1 * 0x58) < uVar1) {
    *(ushort *)(&DAT_6bc71ca0 + (longlong)param_1 * 0x58) = uVar1;
  }
  if (uVar2 < *(ushort *)(&DAT_6bc71c9e + (longlong)param_1 * 0x58)) {
    *(ushort *)(&DAT_6bc71c9e + (longlong)param_1 * 0x58) = uVar2;
  }
  else if (*(ushort *)(&DAT_6bc71ca2 + (longlong)param_1 * 0x58) < uVar2) {
    *(ushort *)(&DAT_6bc71ca2 + (longlong)param_1 * 0x58) = uVar2;
  }
  iVar3 = (((uint)*(ushort *)(&DAT_6bc71ca0 + (longlong)param_1 * 0x58) -
           (uint)*(ushort *)(&DAT_6bc71c9c + (longlong)param_1 * 0x58)) +
          (uint)*(ushort *)(&DAT_6bc71ca2 + (longlong)param_1 * 0x58)) -
          (uint)*(ushort *)(&DAT_6bc71c9e + (longlong)param_1 * 0x58);
  if (*(short *)(&DAT_6bc71cb2 + (longlong)param_1 * 0x58) < iVar3) {
    *(short *)(&DAT_6bc71cb2 + (longlong)param_1 * 0x58) = (short)iVar3;
  }
  return;
}


// ============================================================================
// Function: AFT_MustRelation @ 6baca729
// ============================================================================

undefined4 AFT_MustRelation(int param_1)

{
  undefined4 local_c;
  
  local_c = 0;
  if ((*(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) & 4) == 0) {
    if (g_aftState != *(short *)(&DAT_6bc71c84 + (longlong)param_1 * 0x58)) {
      local_c = 2;
    }
  }
  else {
    local_c = 1;
  }
  return local_c;
}


// ============================================================================
// Function: AFT_PalmSameSide @ 6baca7bd
// ============================================================================

undefined4 AFT_PalmSameSide(int param_1,int param_2,int param_3)

{
  char cVar1;
  undefined4 local_c;
  
  local_c = 0;
  if (param_1 == 0) {
    cVar1 = TSAStatic_IsScreenRotated();
    if ((cVar1 == '\0') &&
       ((((DAT_6bc71c48 & 1) != 0 && (param_2 < DAT_6bc72012)) ||
        (((DAT_6bc71c48 & 2) != 0 && (DAT_6bc72012 < param_2)))))) {
      local_c = 1;
    }
  }
  else {
    cVar1 = TSAStatic_IsScreenRotated();
    if ((cVar1 != '\0') &&
       ((((DAT_6bc71c48 & 1) != 0 && (param_3 < DAT_6bc72022)) ||
        (((DAT_6bc71c48 & 2) != 0 && (DAT_6bc72022 < param_3)))))) {
      local_c = 2;
    }
  }
  return local_c;
}


// ============================================================================
// Function: AFT_SelectAdd @ 6bacd737
// ============================================================================

undefined4 AFT_SelectAdd(int param_1,longlong param_2)

{
  int iVar1;
  undefined4 local_c;
  
  local_c = 0;
  iVar1 = (*_refptr_g_tsaStaticPtr)->nField_0x264;
  if (iVar1 == 1) {
    if (*(short *)(&DAT_6bc71c92 + (longlong)(int)*(short *)(param_2 + 10) * 0x58) <
        *(short *)(&DAT_6bc71c92 + (longlong)param_1 * 0x58)) {
      local_c = 1;
    }
  }
  else if (iVar1 == 0) {
    if (*(short *)(&DAT_6bc71c94 + (longlong)param_1 * 0x58) <
        *(short *)(&DAT_6bc71c94 + (longlong)(int)*(short *)(param_2 + 10) * 0x58)) {
      local_c = 1;
    }
  }
  else if (iVar1 == 2) {
    if (*(short *)(&DAT_6bc71c94 + (longlong)(int)*(short *)(param_2 + 10) * 0x58) <
        *(short *)(&DAT_6bc71c94 + (longlong)param_1 * 0x58)) {
      local_c = 1;
    }
  }
  else if ((iVar1 == 3) &&
          (*(short *)(&DAT_6bc71c92 + (longlong)param_1 * 0x58) <
           *(short *)(&DAT_6bc71c92 + (longlong)(int)*(short *)(param_2 + 10) * 0x58))) {
    local_c = 1;
  }
  return local_c;
}


// ============================================================================
// Function: AFT_FirstPrepare @ 6bace642
// ============================================================================

void AFT_FirstPrepare(int param_1,longlong param_2)

{
  byte bVar1;
  
                    /* 0x8e642  74  AFT_FirstPrepare */
  AFT_InputLegalProc(param_1,param_2);
  if (*(int *)(&g_aftFgInfo + (longlong)param_1 * 0x58) == 0) {
    AFT_SaveStartInfo(param_1,param_2);
  }
  else {
    *(short *)(&DAT_6bc71c96 + (longlong)param_1 * 0x58) =
         (short)((int)(*(int *)((longlong)param_1 * 0x30 + param_2 + 0x10) +
                      (uint)*(ushort *)(&DAT_6bc71c96 + (longlong)param_1 * 0x58)) / 2);
    bVar1 = *(byte *)((longlong)param_1 * 0x30 + param_2 + 0x26);
    *(short *)(&DAT_6bc71c98 + (longlong)param_1 * 0x58) =
         (short)((int)(((uint)bVar1 * 4 + (uint)bVar1) * 2 +
                      (uint)*(ushort *)(&DAT_6bc71c98 + (longlong)param_1 * 0x58)) >> 1);
    bVar1 = *(byte *)((longlong)param_1 * 0x30 + param_2 + 0x27);
    *(short *)(&DAT_6bc71c9a + (longlong)param_1 * 0x58) =
         (short)((int)(((uint)bVar1 * 4 + (uint)bVar1) * 2 +
                      (uint)*(ushort *)(&DAT_6bc71c9a + (longlong)param_1 * 0x58)) >> 1);
    bVar1 = *(byte *)((longlong)param_1 * 0x30 + param_2 + 0x28);
    if (*(ushort *)(&DAT_6bc71ca4 + (longlong)param_1 * 0x58) <
        (ushort)(((ushort)bVar1 * 4 + (ushort)bVar1) * 2)) {
      *(ushort *)(&DAT_6bc71ca4 + (longlong)param_1 * 0x58) =
           (ushort)*(byte *)((longlong)param_1 * 0x30 + param_2 + 0x28);
    }
    else {
      bVar1 = *(byte *)((longlong)param_1 * 0x30 + param_2 + 0x28);
      *(short *)(&DAT_6bc71ca4 + (longlong)param_1 * 0x58) =
           (short)((int)(((uint)bVar1 * 4 + (uint)bVar1) * 2 +
                        (uint)*(ushort *)(&DAT_6bc71ca4 + (longlong)param_1 * 0x58)) >> 1);
    }
    bVar1 = *(byte *)((longlong)param_1 * 0x30 + param_2 + 0x29);
    if (*(ushort *)(&DAT_6bc71ca6 + (longlong)param_1 * 0x58) <
        (ushort)(((ushort)bVar1 * 4 + (ushort)bVar1) * 2)) {
      bVar1 = *(byte *)((longlong)param_1 * 0x30 + param_2 + 0x29);
      *(ushort *)(&DAT_6bc71ca6 + (longlong)param_1 * 0x58) =
           ((ushort)bVar1 * 4 + (ushort)bVar1) * 2;
    }
    else {
      bVar1 = *(byte *)((longlong)param_1 * 0x30 + param_2 + 0x29);
      *(short *)(&DAT_6bc71ca6 + (longlong)param_1 * 0x58) =
           (short)((int)(((uint)bVar1 * 4 + (uint)bVar1) * 2 +
                        (uint)*(ushort *)(&DAT_6bc71ca6 + (longlong)param_1 * 0x58)) >> 1);
    }
    bVar1 = *(byte *)((longlong)param_1 * 0x30 + param_2 + 0x2a);
    *(short *)(&DAT_6bc71cb4 + (longlong)param_1 * 0x58) =
         (short)((int)(((uint)bVar1 * 4 + (uint)bVar1) * 2 +
                      (int)*(short *)(&DAT_6bc71cb4 + (longlong)param_1 * 0x58)) / 2);
    bVar1 = *(byte *)((longlong)param_1 * 0x30 + param_2 + 0x2b);
    *(short *)(&DAT_6bc71cb6 + (longlong)param_1 * 0x58) =
         (short)((int)(((uint)bVar1 * 4 + (uint)bVar1) * 2 +
                      (int)*(short *)(&DAT_6bc71cb6 + (longlong)param_1 * 0x58)) / 2);
  }
  if (((*(uint *)(&g_aftFgInfo + (longlong)param_1 * 0x58) < 9) &&
      (g_aftState != *(short *)(&DAT_6bc71c84 + (longlong)param_1 * 0x58))) &&
     (*(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) =
           *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) | 4,
     *(short *)(&DAT_6bc71c84 + (longlong)param_1 * 0x58) < g_aftState)) {
    *(short *)(&DAT_6bc71c84 + (longlong)param_1 * 0x58) = g_aftState;
  }
  AFT_FolderKillCheck(param_1,param_2);
  if (*(uint *)(&g_aftFgInfo + (longlong)param_1 * 0x58) < 9) {
    AFT_EdgeCoordinateAdjust(param_1,param_2);
  }
  AFT_PalmCheck(param_1,param_2);
  return;
}


// ============================================================================
// Function: AFT_MustStateProc @ 6bad158a
// ============================================================================

void AFT_MustStateProc(int param_1,longlong param_2,undefined8 param_3)

{
  int iVar1;
  int iVar2;
  
  iVar1 = *(int *)((longlong)param_1 * 0x30 + param_2 + 8) -
          (int)*(short *)(&DAT_6bc71cae + (longlong)param_1 * 0x58);
  iVar2 = *(int *)((longlong)param_1 * 0x30 + param_2 + 0xc) -
          (int)*(short *)(&DAT_6bc71cb0 + (longlong)param_1 * 0x58);
  if ((int)DAT_6bc7204c < iVar2 * iVar2 + iVar1 * iVar1) {
    if (DAT_6bc71c44 == 0) {
      DAT_6bc71c44 = 1;
    }
    g_lastMovTime = *(undefined8 *)_refptr_g_curTime;
  }
  *(short *)(&DAT_6bc71cae + (longlong)param_1 * 0x58) =
       (short)*(undefined4 *)((longlong)param_1 * 0x30 + param_2 + 8);
  *(short *)(&DAT_6bc71cb0 + (longlong)param_1 * 0x58) =
       (short)*(undefined4 *)((longlong)param_1 * 0x30 + param_2 + 0xc);
  if (*(short *)(&DAT_6bc71c8c + (longlong)param_1 * 0x58) < 1) {
    *(undefined4 *)(&DAT_6bc71cb8 + (longlong)param_1 * 0x58) = 1;
  }
  else {
    *(undefined4 *)(&DAT_6bc71cb8 + (longlong)param_1 * 0x58) = 2;
  }
  AFT_UpdateReportAddr(param_1,param_2);
  AFT_T3Report(param_1,param_2,param_3);
  return;
}


// ============================================================================
// Function: AFT_InTheCorner @ 6bad17e4
// ============================================================================

void AFT_InTheCorner(int param_1,longlong param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  bool bVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int local_10;
  uint local_c;
  
  local_c = 0;
  local_10 = 0;
  bVar4 = false;
  iVar5 = (int)DAT_6bc72054;
  iVar6 = (int)DAT_6bc72056;
  iVar7 = (int)DAT_6bc72058;
  iVar8 = (int)DAT_6bc7205a;
  iVar1 = *(int *)((longlong)param_1 * 0x30 + param_2 + 8);
  iVar2 = *(int *)((longlong)param_1 * 0x30 + param_2 + 0xc);
  if ((DAT_6bc72058 != 0) && (DAT_6bc7205a != 0)) {
    bVar4 = true;
  }
  iVar3 = (*_refptr_g_tsaStaticPtr)->nField_0x264;
  if (iVar3 == 1) {
    if (((iVar1 < iVar5 + DAT_6bc7200e) ||
        ((DAT_6bc72010 - iVar5 < iVar1 &&
         ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 0x200) != 0)))) &&
       ((iVar2 < iVar6 + DAT_6bc7201e || (DAT_6bc72020 - iVar6 < iVar2)))) {
      local_c = 3;
      if (((DAT_6bc71c46 & 2) != 0) && (DAT_6bc72012 < iVar1)) {
        local_c = 0xd;
      }
    }
    else if (((iVar1 < iVar6 + DAT_6bc7200e) ||
             ((DAT_6bc72010 - iVar6 < iVar1 &&
              ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 0x200) != 0)))) &&
            ((iVar2 < iVar5 + DAT_6bc7201e || (DAT_6bc72020 - iVar5 < iVar2)))) {
      local_c = 4;
    }
    if (bVar4) {
      if (((iVar1 < iVar7 + DAT_6bc7200e) ||
          ((DAT_6bc72010 - iVar7 < iVar1 &&
           ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 0x800) != 0)))) &&
         ((iVar2 < iVar8 + DAT_6bc7201e || (DAT_6bc72020 - iVar8 < iVar2)))) {
        local_10 = 3;
      }
      else if (((iVar1 < iVar8 + DAT_6bc7200e) ||
               ((DAT_6bc72010 - iVar8 < iVar1 &&
                ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 0x800) != 0)))) &&
              ((iVar2 < iVar7 + DAT_6bc7201e || (DAT_6bc72020 - iVar7 < iVar2)))) {
        local_10 = 4;
      }
    }
  }
  else if (iVar3 == 0) {
    if (((DAT_6bc72020 - iVar5 < iVar2) ||
        ((iVar2 < iVar5 + DAT_6bc7201e &&
         ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 0x100) != 0)))) &&
       ((iVar1 < iVar6 + DAT_6bc7200e || (DAT_6bc72010 - iVar6 < iVar1)))) {
      local_c = 1;
      if (((DAT_6bc71c46 & 1) != 0) && (iVar2 < DAT_6bc72022)) {
        local_c = 0xb;
      }
    }
    else if (((DAT_6bc72020 - iVar6 < iVar2) ||
             ((iVar2 < iVar6 + DAT_6bc7201e &&
              ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 0x100) != 0)))) &&
            ((iVar1 < iVar5 + DAT_6bc7200e || (DAT_6bc72010 - iVar5 < iVar1)))) {
      local_c = 2;
    }
    if (bVar4) {
      if (((DAT_6bc72020 - iVar7 < iVar2) ||
          ((iVar2 < iVar7 + DAT_6bc7201e &&
           ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 0x400) != 0)))) &&
         ((iVar1 < iVar8 + DAT_6bc7200e || (DAT_6bc72010 - iVar8 < iVar1)))) {
        local_10 = 1;
      }
      else if (((DAT_6bc72020 - iVar8 < iVar2) ||
               ((iVar2 < iVar8 + DAT_6bc7201e &&
                ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 0x400) != 0)))) &&
              ((iVar1 < iVar7 + DAT_6bc7200e || (DAT_6bc72010 - iVar7 < iVar1)))) {
        local_10 = 2;
      }
    }
  }
  else if (iVar3 == 2) {
    if (((iVar2 < iVar5 + DAT_6bc7201e) ||
        ((DAT_6bc72020 - iVar5 < iVar2 &&
         ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 0x100) != 0)))) &&
       ((iVar1 < iVar6 + DAT_6bc7200e || (DAT_6bc72010 - iVar6 < iVar1)))) {
      local_c = 5;
      if (((DAT_6bc71c46 & 1) != 0) && (DAT_6bc72022 < iVar2)) {
        local_c = 0xf;
      }
    }
    else if (((iVar2 < iVar6 + DAT_6bc7201e) ||
             ((DAT_6bc72020 - iVar6 < iVar2 &&
              ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 0x100) != 0)))) &&
            ((iVar1 < iVar5 + DAT_6bc7200e || (DAT_6bc72010 - iVar5 < iVar1)))) {
      local_c = 6;
    }
    if (bVar4) {
      if (((iVar2 < iVar7 + DAT_6bc7201e) ||
          ((DAT_6bc72020 - iVar7 < iVar2 &&
           ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 0x400) != 0)))) &&
         ((iVar1 < iVar8 + DAT_6bc7200e || (DAT_6bc72010 - iVar8 < iVar1)))) {
        local_10 = 5;
      }
      else if (((iVar2 < iVar8 + DAT_6bc7201e) ||
               ((DAT_6bc72020 - iVar8 < iVar2 &&
                ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 0x400) != 0)))) &&
              ((iVar1 < iVar7 + DAT_6bc7200e || (DAT_6bc72010 - iVar7 < iVar1)))) {
        local_10 = 6;
      }
    }
  }
  else if (iVar3 == 3) {
    if (((DAT_6bc72010 - iVar5 < iVar1) ||
        ((iVar1 < iVar5 + DAT_6bc7200e &&
         ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 0x200) != 0)))) &&
       ((iVar2 < iVar6 + DAT_6bc7201e || (DAT_6bc72020 - iVar6 < iVar2)))) {
      local_c = 7;
      if (((DAT_6bc71c46 & 2) != 0) && (iVar1 < DAT_6bc72012)) {
        local_c = 0x11;
      }
    }
    else if (((DAT_6bc72010 - iVar6 < iVar1) ||
             ((iVar1 < iVar6 + DAT_6bc7200e &&
              ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 0x200) != 0)))) &&
            ((iVar2 < iVar5 + DAT_6bc7201e || (DAT_6bc72020 - iVar5 < iVar2)))) {
      local_c = 8;
    }
    if (bVar4) {
      if (((DAT_6bc72010 - iVar7 < iVar1) ||
          ((iVar1 < iVar7 + DAT_6bc7200e &&
           ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 0x800) != 0)))) &&
         ((iVar2 < iVar8 + DAT_6bc7201e || (DAT_6bc72020 - iVar8 < iVar2)))) {
        local_10 = 7;
      }
      else if (((DAT_6bc72010 - iVar8 < iVar1) ||
               ((iVar1 < iVar8 + DAT_6bc7200e &&
                ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 0x800) != 0)))) &&
              ((iVar2 < iVar7 + DAT_6bc7201e || (DAT_6bc72020 - iVar7 < iVar2)))) {
        local_10 = 8;
      }
    }
  }
  if (local_c < 0xb) {
    if (local_c != 0) {
      *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) =
           *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) | 0x10;
    }
  }
  else {
    *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) =
         *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) | 0x20;
  }
  if (local_10 != 0) {
    *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) =
         *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) | 0x40;
  }
  return;
}


// ============================================================================
// Function: AFT_AboveFingerprint @ 6bad2292
// ============================================================================

undefined4 AFT_AboveFingerprint(int param_1,int param_2)

{
  undefined4 local_c;
  
  local_c = 0;
  if (((((int)(uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6f8 < param_1) &&
       (param_1 < (int)(uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6fa)) &&
      ((int)(uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6fc < param_2)) &&
     (param_2 < (int)(uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6fe)) {
    local_c = 1;
  }
  return local_c;
}


// ============================================================================
// Function: AFT_NewXYValue @ 6bacd2c0
// ============================================================================

void AFT_NewXYValue(int *param_1,int *param_2)

{
  if ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 2) {
    *param_2 = (int)DAT_6bc72020 - (*param_2 - (int)DAT_6bc7201e);
  }
  else if ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 3) {
    *param_1 = (int)DAT_6bc72010 - (*param_1 - (int)DAT_6bc7200e);
  }
  return;
}


// ============================================================================
// Function: AFT_XEdgeState @ 6baceda6
// ============================================================================

undefined4 AFT_XEdgeState(int param_1)

{
  undefined4 local_c;
  
  local_c = 0;
  if ((param_1 < DAT_6bc7200e) || (DAT_6bc72010 < param_1)) {
    local_c = 6;
  }
  else if ((DAT_6bc7200e == param_1) || (DAT_6bc72010 == param_1)) {
    local_c = 5;
  }
  else if ((param_1 < g_aftPara2) || (DAT_6bc72092 < param_1)) {
    local_c = 4;
  }
  else if ((param_1 < DAT_6bc72098) || (DAT_6bc7209a < param_1)) {
    local_c = 3;
  }
  else if ((param_1 < DAT_6bc7203a) || (DAT_6bc7203c < param_1)) {
    local_c = 2;
  }
  else if ((param_1 < DAT_6bc72042) || (DAT_6bc72044 < param_1)) {
    local_c = 1;
  }
  return local_c;
}


// ============================================================================
// Function: AFT_YEdgeState @ 6baceec6
// ============================================================================

undefined4 AFT_YEdgeState(int param_1)

{
  undefined4 local_c;
  
  local_c = 0;
  if ((param_1 < DAT_6bc7201e) || (DAT_6bc72020 < param_1)) {
    local_c = 6;
  }
  else if ((DAT_6bc7201e == param_1) || (DAT_6bc72020 == param_1)) {
    local_c = 5;
  }
  else if ((param_1 < DAT_6bc72094) || (DAT_6bc72096 < param_1)) {
    local_c = 4;
  }
  else if ((param_1 < DAT_6bc7209c) || (DAT_6bc7209e < param_1)) {
    local_c = 3;
  }
  else if ((param_1 < DAT_6bc7203e) || (DAT_6bc72040 < param_1)) {
    local_c = 2;
  }
  else if ((param_1 < DAT_6bc72046) || (DAT_6bc72048 < param_1)) {
    local_c = 1;
  }
  return local_c;
}


// ============================================================================
// Function: AFT_YAxisEdgeRecovery @ 6bad0321
// ============================================================================

void AFT_YAxisEdgeRecovery(int param_1,longlong param_2,ushort *param_3)

{
  int iVar1;
  int local_18;
  int local_14;
  int local_10;
  int local_c;
  
  local_10 = *(int *)((longlong)param_1 * 0x30 + param_2 + 8);
  iVar1 = *(int *)((longlong)param_1 * 0x30 + param_2 + 0xc);
  local_c = iVar1 - *(short *)(&DAT_6bc71c94 + (longlong)param_1 * 0x58);
  if (local_c < 0) {
    local_c = -local_c;
  }
  if (DAT_6bc72052 < local_c) {
    *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) =
         *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) | 8;
  }
  local_14 = (int)DAT_6bc7203a;
  local_18 = (int)DAT_6bc7203c;
  if ((*(int *)(&DAT_6bc71c88 + (longlong)param_1 * 0x58) == 0) && ((*param_3 & 0x20) == 0)) {
    if (*(short *)(&DAT_6bc71c84 + (longlong)param_1 * 0x58) != 0) {
      local_14 = ((local_14 - DAT_6bc7200e) * 6) / 5 + (int)DAT_6bc7200e;
      local_18 = (int)DAT_6bc72010 - ((DAT_6bc72010 - local_18) * 6) / 5;
    }
  }
  else {
    local_14 = (int)DAT_6bc72042;
    local_18 = (int)DAT_6bc72044;
  }
  if (*(uint *)(&DAT_6bc71c88 + (longlong)param_1 * 0x58) < 4) {
    if ((local_10 < local_14) || (local_18 < local_10)) {
      AFT_EdgeRecoverProc1(local_c,param_1,param_3);
    }
    else {
      AFT_EdgeRecoverProc2(param_1,param_2,param_3);
    }
    if (*(int *)(&g_aftFgInfo + (longlong)param_1 * 0x58) == 10) {
      local_10 = local_10 - *(short *)(&DAT_6bc71c92 + (longlong)param_1 * 0x58);
      if (local_10 < 0) {
        local_10 = -local_10;
      }
      if ((local_10 * 3 < local_c) &&
         (((int)DAT_6bc72052 + (int)DAT_6bc7204e < local_c ||
          ((DAT_6bc7204e < local_c && (param_3[0xb] != 0)))))) {
        *(short *)(&DAT_6bc71c94 + (longlong)param_1 * 0x58) = (short)iVar1;
      }
    }
  }
  return;
}


// ============================================================================
// Function: AFT_XAxisEdgeRecovery @ 6bad06bc
// ============================================================================

void AFT_XAxisEdgeRecovery(int param_1,longlong param_2,ushort *param_3)

{
  int iVar1;
  int local_18;
  int local_14;
  int local_10;
  int local_c;
  
  iVar1 = *(int *)((longlong)param_1 * 0x30 + param_2 + 8);
  local_10 = *(int *)((longlong)param_1 * 0x30 + param_2 + 0xc);
  local_c = iVar1 - *(short *)(&DAT_6bc71c92 + (longlong)param_1 * 0x58);
  if (local_c < 0) {
    local_c = -local_c;
  }
  if (DAT_6bc72052 < local_c) {
    *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) =
         *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) | 8;
  }
  local_14 = (int)DAT_6bc7203e;
  local_18 = (int)DAT_6bc72040;
  if ((*(int *)(&DAT_6bc71c88 + (longlong)param_1 * 0x58) == 0) && ((*param_3 & 0x20) == 0)) {
    if (*(short *)(&DAT_6bc71c84 + (longlong)param_1 * 0x58) != 0) {
      local_14 = ((local_14 - DAT_6bc7201e) * 6) / 5 + (int)DAT_6bc7201e;
      local_18 = (int)DAT_6bc72020 - ((DAT_6bc72020 - local_18) * 6) / 5;
    }
  }
  else {
    local_14 = (int)DAT_6bc72046;
    local_18 = (int)DAT_6bc72048;
  }
  if (*(uint *)(&DAT_6bc71c88 + (longlong)param_1 * 0x58) < 4) {
    if ((local_10 < local_14) || (local_18 < local_10)) {
      AFT_EdgeRecoverProc1(local_c,param_1,param_3);
    }
    else {
      AFT_EdgeRecoverProc2(param_1,param_2,param_3);
    }
    if (*(int *)(&g_aftFgInfo + (longlong)param_1 * 0x58) == 10) {
      local_10 = local_10 - *(short *)(&DAT_6bc71c94 + (longlong)param_1 * 0x58);
      if (local_10 < 0) {
        local_10 = -local_10;
      }
      if ((local_10 * 3 < local_c) &&
         (((int)DAT_6bc72052 + (int)DAT_6bc7204e < local_c ||
          ((DAT_6bc7204e < local_c && (param_3[0xb] != 0)))))) {
        *(short *)(&DAT_6bc71c92 + (longlong)param_1 * 0x58) = (short)iVar1;
      }
    }
  }
  return;
}


// ============================================================================
// Function: AFT_TimeoutRecovery @ 6bad0a57
// ============================================================================

undefined4 AFT_TimeoutRecovery(int param_1,int param_2)

{
  longlong lVar1;
  undefined4 local_18;
  int local_14;
  int local_10;
  undefined4 local_c;
  
  local_c = 0;
  local_10 = 0;
  local_14 = 0;
  local_18 = 0;
  if (param_2 == 0) {
    local_10 = 5;
    local_14 = 6;
    local_18 = 7;
  }
  else if (param_2 == 1) {
    local_10 = 1;
    local_14 = 2;
    local_18 = 3;
  }
  lVar1 = *(longlong *)_refptr_g_curTime - *(longlong *)(&DAT_6bc71cc0 + (longlong)param_1 * 0x58);
  if ((lVar1 < 0x33) || (local_10 != *(int *)(&g_aftFgInfo + (longlong)param_1 * 0x58))) {
    if (*(longlong *)_refptr_g_curTime - *(longlong *)(&DAT_6bc71cc8 + (longlong)param_1 * 0x58) <
        0xc9) {
      if (local_14 == *(int *)(&g_aftFgInfo + (longlong)param_1 * 0x58)) {
        if ((*(short *)(&DAT_6bc71c84 + (longlong)param_1 * 0x58) == 0) &&
           ((*(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) & 0x40) == 0)) {
          if ((499 < lVar1) ||
             ((0xf9 < lVar1 && (*(short *)(&DAT_6bc71cb2 + (longlong)param_1 * 0x58) == 0)))) {
            *(undefined4 *)(&g_aftFgInfo + (longlong)param_1 * 0x58) = 10;
            local_c = 3;
          }
        }
        else if (0x9c3 < lVar1) {
          *(undefined4 *)(&g_aftFgInfo + (longlong)param_1 * 0x58) = local_18;
          *(undefined8 *)(&DAT_6bc71cc0 + (longlong)param_1 * 0x58) =
               *(undefined8 *)_refptr_g_curTime;
          local_c = 4;
        }
      }
    }
    else {
      *(undefined4 *)(&g_aftFgInfo + (longlong)param_1 * 0x58) = 10;
      local_c = 2;
    }
  }
  else {
    *(undefined4 *)(&g_aftFgInfo + (longlong)param_1 * 0x58) = 10;
    local_c = 1;
  }
  return local_c;
}


// ============================================================================
// Function: AFT_SaveStartInfo @ 6baca8a3
// ============================================================================

void AFT_SaveStartInfo(int param_1,longlong param_2)

{
  byte bVar1;
  int local_10;
  int local_c;
  
  local_c = *(int *)((longlong)param_1 * 0x30 + param_2 + 8);
  local_10 = *(int *)((longlong)param_1 * 0x30 + param_2 + 0xc);
  *(undefined2 *)(&DAT_6bc71c84 + (longlong)param_1 * 0x58) = g_aftState;
  *(undefined2 *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) = 0;
  *(undefined4 *)(&DAT_6bc71c88 + (longlong)param_1 * 0x58) = 0;
  *(undefined2 *)(&DAT_6bc71cb2 + (longlong)param_1 * 0x58) = 0;
  *(undefined4 *)(&DAT_6bc71cb8 + (longlong)param_1 * 0x58) = 0;
  *(undefined2 *)(&DAT_6bc71c8c + (longlong)param_1 * 0x58) = 0;
  *(short *)(&DAT_6bc71c8e + (longlong)param_1 * 0x58) =
       (short)*(undefined4 *)((longlong)param_1 * 0x30 + param_2 + 4);
  *(short *)(&DAT_6bc71c90 + (longlong)param_1 * 0x58) =
       (short)*(undefined4 *)((longlong)param_1 * 0x30 + param_2);
  if (local_c < DAT_6bc7200e) {
    local_c = (int)DAT_6bc7200e;
  }
  else if (DAT_6bc72010 < local_c) {
    local_c = (int)DAT_6bc72010;
  }
  if (local_10 < DAT_6bc7201e) {
    local_10 = (int)DAT_6bc7201e;
  }
  else if (DAT_6bc72020 < local_10) {
    local_10 = (int)DAT_6bc72020;
  }
  *(short *)(&DAT_6bc71cae + (longlong)param_1 * 0x58) = (short)local_c;
  *(short *)(&DAT_6bc71cb0 + (longlong)param_1 * 0x58) = (short)local_10;
  *(short *)(&DAT_6bc71c92 + (longlong)param_1 * 0x58) = (short)local_c;
  *(short *)(&DAT_6bc71c94 + (longlong)param_1 * 0x58) = (short)local_10;
  *(short *)(&DAT_6bc71c96 + (longlong)param_1 * 0x58) =
       (short)*(undefined4 *)((longlong)param_1 * 0x30 + param_2 + 0x10);
  bVar1 = *(byte *)((longlong)param_1 * 0x30 + param_2 + 0x26);
  *(ushort *)(&DAT_6bc71c98 + (longlong)param_1 * 0x58) = ((ushort)bVar1 * 4 + (ushort)bVar1) * 2;
  bVar1 = *(byte *)((longlong)param_1 * 0x30 + param_2 + 0x27);
  *(ushort *)(&DAT_6bc71c9a + (longlong)param_1 * 0x58) = ((ushort)bVar1 * 4 + (ushort)bVar1) * 2;
  bVar1 = *(byte *)((longlong)param_1 * 0x30 + param_2 + 0x28);
  *(ushort *)(&DAT_6bc71c9c + (longlong)param_1 * 0x58) = ((ushort)bVar1 * 4 + (ushort)bVar1) * 2;
  bVar1 = *(byte *)((longlong)param_1 * 0x30 + param_2 + 0x29);
  *(ushort *)(&DAT_6bc71c9e + (longlong)param_1 * 0x58) = ((ushort)bVar1 * 4 + (ushort)bVar1) * 2;
  bVar1 = *(byte *)((longlong)param_1 * 0x30 + param_2 + 0x28);
  *(ushort *)(&DAT_6bc71ca0 + (longlong)param_1 * 0x58) = ((ushort)bVar1 * 4 + (ushort)bVar1) * 2;
  bVar1 = *(byte *)((longlong)param_1 * 0x30 + param_2 + 0x29);
  *(ushort *)(&DAT_6bc71ca2 + (longlong)param_1 * 0x58) = ((ushort)bVar1 * 4 + (ushort)bVar1) * 2;
  bVar1 = *(byte *)((longlong)param_1 * 0x30 + param_2 + 0x28);
  *(ushort *)(&DAT_6bc71ca4 + (longlong)param_1 * 0x58) = ((ushort)bVar1 * 4 + (ushort)bVar1) * 2;
  bVar1 = *(byte *)((longlong)param_1 * 0x30 + param_2 + 0x29);
  *(ushort *)(&DAT_6bc71ca6 + (longlong)param_1 * 0x58) = ((ushort)bVar1 * 4 + (ushort)bVar1) * 2;
  *(short *)(&DAT_6bc71ca8 + (longlong)param_1 * 0x58) =
       (short)*(undefined4 *)((longlong)param_1 * 0x30 + param_2 + 0x14);
  *(short *)(&DAT_6bc71caa + (longlong)param_1 * 0x58) =
       (short)*(undefined4 *)((longlong)param_1 * 0x30 + param_2 + 0x18);
  *(short *)(&DAT_6bc71cac + (longlong)param_1 * 0x58) =
       (short)*(undefined4 *)((longlong)param_1 * 0x30 + param_2 + 0x1c);
  bVar1 = *(byte *)((longlong)param_1 * 0x30 + param_2 + 0x2a);
  *(ushort *)(&DAT_6bc71cb4 + (longlong)param_1 * 0x58) = ((ushort)bVar1 * 4 + (ushort)bVar1) * 2;
  bVar1 = *(byte *)((longlong)param_1 * 0x30 + param_2 + 0x2b);
  *(ushort *)(&DAT_6bc71cb6 + (longlong)param_1 * 0x58) = ((ushort)bVar1 * 4 + (ushort)bVar1) * 2;
  *(undefined8 *)(&DAT_6bc71cc0 + (longlong)param_1 * 0x58) = *(undefined8 *)_refptr_g_curTime;
  *(undefined8 *)(&DAT_6bc71cc8 + (longlong)param_1 * 0x58) = *(undefined8 *)_refptr_g_curTime;
  return;
}


// ============================================================================
// Function: AFT_FolderKillCheck @ 6bacb142
// ============================================================================

void AFT_FolderKillCheck(int param_1,longlong param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  bool bVar4;
  int iVar5;
  int iVar6;
  
  bVar4 = false;
  iVar1 = *(int *)((longlong)param_1 * 0x30 + param_2 + 8);
  iVar2 = *(int *)((longlong)param_1 * 0x30 + param_2 + 0xc);
  if (((DAT_6bc72004 != 0) && (*(int *)_refptr_g_foldedState != 0)) &&
     ((*(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) & 0x100) == 0)) {
    iVar5 = (int)DAT_6bc7202a - (int)DAT_6bc7200e;
    iVar6 = (int)DAT_6bc72032 - (int)DAT_6bc7200e;
    iVar3 = *(int *)(&g_aftFgInfo + (longlong)param_1 * 0x58);
    if (((*(int *)_refptr_g_foldedDisplay == 1) && (DAT_6bc72004 == 1)) ||
       ((*(int *)_refptr_g_foldedDisplay == 2 && (DAT_6bc72004 == 2)))) {
      if (iVar6 + DAT_6bc72006 < iVar1) {
        *(undefined4 *)(&g_aftFgInfo + (longlong)param_1 * 0x58) = 9;
        if (DAT_6bc72008 < iVar1) {
          bVar4 = true;
        }
      }
      else if (iVar5 + DAT_6bc72006 < iVar1) {
        *(undefined4 *)(&g_aftFgInfo + (longlong)param_1 * 0x58) = 0;
      }
    }
    else if (((*(int *)_refptr_g_foldedDisplay == 1) && (DAT_6bc72004 == 2)) ||
            ((*(int *)_refptr_g_foldedDisplay == 2 && (DAT_6bc72004 == 1)))) {
      if (iVar1 < DAT_6bc72008 - iVar6) {
        *(undefined4 *)(&g_aftFgInfo + (longlong)param_1 * 0x58) = 9;
        if (iVar1 < DAT_6bc72006) {
          bVar4 = true;
        }
      }
      else if (iVar1 < DAT_6bc72008 - iVar5) {
        *(undefined4 *)(&g_aftFgInfo + (longlong)param_1 * 0x58) = 0;
      }
    }
    else if (((*(int *)_refptr_g_foldedDisplay == 1) && (DAT_6bc72004 == 3)) ||
            ((*(int *)_refptr_g_foldedDisplay == 2 && (DAT_6bc72004 == 4)))) {
      if (iVar6 + DAT_6bc72006 < iVar2) {
        *(undefined4 *)(&g_aftFgInfo + (longlong)param_1 * 0x58) = 9;
        if (DAT_6bc72008 < iVar2) {
          bVar4 = true;
        }
      }
      else if (iVar5 + DAT_6bc72006 < iVar2) {
        *(undefined4 *)(&g_aftFgInfo + (longlong)param_1 * 0x58) = 0;
      }
    }
    else if (((*(int *)_refptr_g_foldedDisplay == 1) && (DAT_6bc72004 == 4)) ||
            ((*(int *)_refptr_g_foldedDisplay == 2 && (DAT_6bc72004 == 3)))) {
      if (iVar2 < DAT_6bc72008 - iVar6) {
        *(undefined4 *)(&g_aftFgInfo + (longlong)param_1 * 0x58) = 9;
        if (iVar2 < DAT_6bc72006) {
          bVar4 = true;
        }
      }
      else if (iVar2 < DAT_6bc72008 - iVar5) {
        *(undefined4 *)(&g_aftFgInfo + (longlong)param_1 * 0x58) = 0;
      }
    }
    if ((*(int *)(&g_aftFgInfo + (longlong)param_1 * 0x58) != iVar3) &&
       (AFT_SaveStartInfo(param_1,param_2), bVar4)) {
      *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) =
           *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) | 0x100;
    }
  }
  return;
}


// ============================================================================
// Function: AFT_EdgeCoordinateAdjust @ 6bacb6a4
// ============================================================================

void AFT_EdgeCoordinateAdjust(int param_1,longlong param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int local_18;
  int local_14;
  int local_10;
  int local_c;
  
  iVar3 = (int)DAT_6bc7200a;
  iVar4 = (int)DAT_6bc7200c;
  iVar5 = (int)DAT_6bc7201a;
  iVar6 = (int)DAT_6bc7201c;
  iVar1 = *(int *)((longlong)param_1 * 0x30 + param_2 + 8);
  iVar2 = *(int *)((longlong)param_1 * 0x30 + param_2 + 0xc);
  if ((iVar3 < iVar1) || (*(short *)&_refptr_g_tsaPrmtFlash[2].field_0x6ec != 0)) {
    if ((iVar4 <= iVar1) && (*(short *)&_refptr_g_tsaPrmtFlash[2].field_0x6ee == 0)) {
      *(int *)((longlong)param_1 * 0x30 + param_2 + 8) = iVar4 + -1;
    }
  }
  else {
    *(int *)((longlong)param_1 * 0x30 + param_2 + 8) = iVar3 + 1;
  }
  if ((iVar5 < iVar2) || (*(short *)&_refptr_g_tsaPrmtFlash[2].field_0x6f0 != 0)) {
    if ((iVar6 <= iVar2) && (*(short *)&_refptr_g_tsaPrmtFlash[2].field_0x6f2 == 0)) {
      *(int *)((longlong)param_1 * 0x30 + param_2 + 0xc) = iVar6 + -1;
    }
  }
  else {
    *(int *)((longlong)param_1 * 0x30 + param_2 + 0xc) = iVar5 + 1;
  }
  if (DAT_6bc7205c != 0) {
    local_10 = (int)DAT_6bc72062;
    if (((((*_refptr_g_tsaStaticPtr)->nField_0x264 == 0) && (DAT_6bc72022 < iVar2)) ||
        (((*_refptr_g_tsaStaticPtr)->nField_0x264 == 2 && (iVar2 < DAT_6bc72022)))) ||
       ((((*_refptr_g_tsaStaticPtr)->nField_0x264 == 1 && (iVar1 < DAT_6bc72012)) ||
        (((*_refptr_g_tsaStaticPtr)->nField_0x264 == 3 && (DAT_6bc72012 < iVar1)))))) {
      local_10 = (int)DAT_6bc72060;
    }
    if (((local_10 < (int)(uint)*(ushort *)(&DAT_6bc71ca6 + (longlong)param_1 * 0x58)) &&
        (*(short *)(&DAT_6bc71c98 + (longlong)param_1 * 0x58) != 0)) &&
       ((((*(short *)&_refptr_g_tsaPrmtFlash[2].field_0x6ec == 0 && (iVar1 < DAT_6bc7203a)) ||
         ((*(short *)&_refptr_g_tsaPrmtFlash[2].field_0x6ee == 0 && (DAT_6bc7203c < iVar1)))) &&
        (local_c = (int)((uint)*(ushort *)(&DAT_6bc71ca6 + (longlong)param_1 * 0x58) *
                        (int)DAT_6bc7205c) /
                   (int)(uint)*(ushort *)(&DAT_6bc71c98 + (longlong)param_1 * 0x58), 0xc < local_c))
       )) {
      local_c = local_c + -0xc;
      if (DAT_6bc7205e < local_c) {
        local_c = (int)DAT_6bc7205e;
      }
      if (DAT_6bc72012 < iVar1) {
        local_14 = iVar1 + local_c;
        if (iVar4 < iVar1 + local_c) {
          local_14 = iVar4;
        }
      }
      else {
        local_14 = iVar1 - local_c;
        if (iVar1 - local_c < iVar3) {
          local_14 = iVar3;
        }
      }
      *(int *)(param_2 + (longlong)param_1 * 0x30 + 8) = local_14;
    }
    if (((local_10 < (int)(uint)*(ushort *)(&DAT_6bc71ca4 + (longlong)param_1 * 0x58)) &&
        (*(short *)(&DAT_6bc71c9a + (longlong)param_1 * 0x58) != 0)) &&
       ((((*(short *)&_refptr_g_tsaPrmtFlash[2].field_0x6f0 == 0 && (iVar2 < DAT_6bc7203e)) ||
         ((*(short *)&_refptr_g_tsaPrmtFlash[2].field_0x6f2 == 0 && (DAT_6bc72040 < iVar2)))) &&
        (local_c = (int)((uint)*(ushort *)(&DAT_6bc71ca4 + (longlong)param_1 * 0x58) *
                        (int)DAT_6bc7205c) /
                   (int)(uint)*(ushort *)(&DAT_6bc71c9a + (longlong)param_1 * 0x58), 0xc < local_c))
       )) {
      local_c = local_c + -0xc;
      if (DAT_6bc7205e < local_c) {
        local_c = (int)DAT_6bc7205e;
      }
      if (DAT_6bc72022 < iVar2) {
        local_18 = iVar2 + local_c;
        if (iVar6 < iVar2 + local_c) {
          local_18 = iVar6;
        }
      }
      else {
        local_18 = iVar2 - local_c;
        if (iVar2 - local_c < iVar5) {
          local_18 = iVar5;
        }
      }
      *(int *)(param_2 + (longlong)param_1 * 0x30 + 0xc) = local_18;
    }
  }
  return;
}


// ============================================================================
// Function: AFT_PalmCheck @ 6bacbea5
// ============================================================================

void AFT_PalmCheck(int param_1,longlong param_2)

{
  ushort uVar1;
  ushort uVar2;
  int iVar3;
  int iVar4;
  char cVar5;
  uint uVar6;
  uint uVar7;
  uint local_20;
  int local_1c;
  int local_18;
  int local_14;
  int local_10;
  uint local_c;
  
  local_18 = 0;
  local_20 = 0;
  iVar3 = *(int *)((longlong)param_1 * 0x30 + param_2 + 8);
  iVar4 = *(int *)((longlong)param_1 * 0x30 + param_2 + 0xc);
  uVar6 = (uint)*(ushort *)(&DAT_6bc71ca4 + (longlong)param_1 * 0x58);
  uVar7 = (uint)*(ushort *)(&DAT_6bc71ca6 + (longlong)param_1 * 0x58);
  uVar1 = *(ushort *)(&DAT_6bc71c98 + (longlong)param_1 * 0x58);
  uVar2 = *(ushort *)(&DAT_6bc71c9a + (longlong)param_1 * 0x58);
  if (((uVar7 == 0) && (DAT_6bc7203a < iVar3)) && (iVar3 < DAT_6bc7203c)) {
    *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) =
         *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) | 1;
  }
  else {
    *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) =
         *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) & 0xfffe;
  }
  if (((uVar6 == 0) && (DAT_6bc7203e < iVar4)) && (iVar4 < DAT_6bc72040)) {
    *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) =
         *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) | 2;
  }
  else {
    *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) =
         *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) & 0xfffd;
  }
  if ((*(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) & 3) == 3) {
    *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) =
         *(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) & 0xfdff;
    *(undefined4 *)(&DAT_6bc71c88 + (longlong)param_1 * 0x58) = 0;
  }
  else if ((*(int *)(&g_aftFgInfo + (longlong)param_1 * 0x58) != 10) ||
          ((*(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) & 0x200) != 0)) {
    cVar5 = TSAStatic_IsScreenRotated();
    if (cVar5 == '\0') {
      local_c = (uint)DAT_6bc7206c;
      local_10 = (int)DAT_6bc7206e;
      local_14 = (int)DAT_6bc72070;
      if (((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 8) != 0) &&
         ((*(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) & 0x10) != 0)) {
        local_c = local_c - 10;
        local_10 = local_10 + -10;
        local_14 = local_14 + -10;
      }
      if ((int)DAT_6bc72076 < (int)uVar7) {
        local_20 = 6;
      }
      else if ((int)DAT_6bc72074 < (int)uVar7) {
        local_20 = 5;
      }
      else if ((int)DAT_6bc72072 < (int)uVar7) {
        local_20 = 4;
      }
      else if ((((*_refptr_g_tsaStaticPtr)->nField_0x264 == 0) && (DAT_6bc72022 < iVar4)) &&
              (((local_14 < (int)uVar7 || ((local_10 < (int)uVar7 && (0x13 < uVar6)))) ||
               (((int)local_c < (int)uVar7 &&
                ((9 < uVar6 &&
                 (((iVar3 < DAT_6bc72032 || (DAT_6bc72034 < iVar3)) && (DAT_6bc72026 < iVar4))))))))
              )) {
        local_20 = 4;
      }
      else if ((((*_refptr_g_tsaStaticPtr)->nField_0x264 == 2) && (iVar4 < DAT_6bc72022)) &&
              ((local_14 < (int)uVar7 ||
               (((local_10 < (int)uVar7 && (0x13 < uVar6)) ||
                ((((int)local_c < (int)uVar7 && (9 < uVar6)) &&
                 (((iVar3 < DAT_6bc72032 || (DAT_6bc72034 < iVar3)) && (iVar4 < DAT_6bc72024))))))))
              )) {
        local_20 = 4;
      }
      else if ((((int)local_c < (int)uVar7) ||
               ((local_10 < (int)(uVar7 + uVar6) && (uVar6 + 10 < uVar7)))) ||
              (((uVar7 == local_c && ((9 < uVar6 && (uVar6 < 0x14)))) && (uVar1 < 3)))) {
        local_20 = 3;
      }
      else {
        if (iVar3 < DAT_6bc72042) {
          local_18 = 1;
        }
        else if (DAT_6bc72044 < iVar3) {
          local_18 = 2;
        }
        if (local_18 != 0) {
          local_1c = (short)(&DAT_6bc71c4a)[local_18 + -1] - iVar4;
          if ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 2) {
            local_1c = -local_1c;
          }
          if (local_1c < 0xb4) {
            if ((iVar3 < DAT_6bc72032) || (DAT_6bc72034 < iVar3)) {
              local_20 = 2;
            }
            else if ((iVar3 < DAT_6bc7203a) || (DAT_6bc7203c < iVar3)) {
              local_20 = 1;
            }
          }
          else if ((local_1c < 300) && ((iVar3 < DAT_6bc7203a || (DAT_6bc7203c < iVar3)))) {
            local_20 = 1;
          }
        }
      }
      if (3 < local_20) {
        if (iVar3 < DAT_6bc72012) {
          DAT_6bc71c48 = DAT_6bc71c48 | 1;
          DAT_6bc71c4a = (short)iVar4;
        }
        else {
          DAT_6bc71c48 = DAT_6bc71c48 | 2;
          DAT_6bc71c4c = (short)iVar4;
        }
      }
    }
    else {
      local_c = (uint)DAT_6bc72064;
      local_10 = (int)DAT_6bc72066;
      local_14 = (int)DAT_6bc72068;
      if (((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 8) != 0) &&
         ((*(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) & 0x10) != 0)) {
        local_c = local_c - 10;
        local_10 = local_10 + -10;
        local_14 = local_14 + -10;
      }
      if ((int)DAT_6bc72076 < (int)uVar6) {
        local_20 = 6;
      }
      else if ((int)DAT_6bc72074 < (int)uVar6) {
        local_20 = 5;
      }
      else if ((int)DAT_6bc7206a < (int)uVar6) {
        local_20 = 4;
      }
      else if ((((*_refptr_g_tsaStaticPtr)->nField_0x264 == 1) && (iVar3 < DAT_6bc72012)) &&
              (((local_14 < (int)uVar6 || ((local_10 < (int)uVar6 && (0x13 < uVar7)))) ||
               (((int)local_c < (int)uVar6 &&
                ((9 < uVar7 &&
                 (((iVar4 < DAT_6bc72036 || (DAT_6bc72038 < iVar4)) && (iVar3 < DAT_6bc72014))))))))
              )) {
        local_20 = 4;
      }
      else if ((((*_refptr_g_tsaStaticPtr)->nField_0x264 == 3) && (DAT_6bc72012 < iVar3)) &&
              ((local_14 < (int)uVar6 ||
               (((local_10 < (int)uVar6 && (0x13 < uVar7)) ||
                ((((int)local_c < (int)uVar6 && (9 < uVar7)) &&
                 (((iVar4 < DAT_6bc72036 || (DAT_6bc72038 < iVar4)) && (DAT_6bc72016 < iVar3))))))))
              )) {
        local_20 = 4;
      }
      else if ((((int)local_c < (int)uVar6) ||
               ((local_10 < (int)(uVar7 + uVar6) && (uVar7 + 10 < uVar6)))) ||
              (((uVar6 == local_c && ((9 < uVar7 && (uVar7 < 0x14)))) && (uVar2 < 3)))) {
        local_20 = 3;
      }
      else {
        if (iVar4 < DAT_6bc72046) {
          local_18 = 1;
        }
        else if (DAT_6bc72048 < iVar4) {
          local_18 = 2;
        }
        if (local_18 != 0) {
          local_1c = iVar3 - (short)(&DAT_6bc71c4a)[local_18 + -1];
          if ((*_refptr_g_tsaStaticPtr)->nField_0x264 == 3) {
            local_1c = -local_1c;
          }
          if (local_1c < 0xb4) {
            if ((iVar4 < DAT_6bc72036) || (DAT_6bc72038 < iVar4)) {
              local_20 = 2;
            }
            else if ((iVar4 < DAT_6bc7203e) || (DAT_6bc72040 < iVar4)) {
              local_20 = 1;
            }
          }
          else if ((local_1c < 300) && ((iVar4 < DAT_6bc7203e || (DAT_6bc72040 < iVar4)))) {
            local_20 = 1;
          }
        }
      }
      if (3 < local_20) {
        if (iVar4 < DAT_6bc72022) {
          DAT_6bc71c48 = DAT_6bc71c48 | 1;
          DAT_6bc71c4a = (short)iVar3;
        }
        else {
          DAT_6bc71c48 = DAT_6bc71c48 | 2;
          DAT_6bc71c4c = (short)iVar3;
        }
      }
    }
    if (*(uint *)(&DAT_6bc71c88 + (longlong)param_1 * 0x58) < local_20) {
      *(uint *)(&DAT_6bc71c88 + (longlong)param_1 * 0x58) = local_20;
    }
    if ((*(int *)(&DAT_6bc71c88 + (longlong)param_1 * 0x58) == 1) ||
       ((*(int *)(&DAT_6bc71c88 + (longlong)param_1 * 0x58) == 2 &&
        (*(short *)(&DAT_6bc71c84 + (longlong)param_1 * 0x58) != 0)))) {
      AFT_ClearRelevantPalm(iVar3,iVar4);
      *(undefined4 *)(&DAT_6bc71c88 + (longlong)param_1 * 0x58) = 0;
    }
  }
  return;
}


// ============================================================================
// Function: AFT_InputLegalProc @ 6bace298
// ============================================================================

void AFT_InputLegalProc(int param_1,longlong param_2)

{
  int iVar1;
  int iVar2;
  
  iVar1 = *(int *)((longlong)param_1 * 0x30 + param_2 + 8);
  iVar2 = *(int *)((longlong)param_1 * 0x30 + param_2 + 0xc);
  if (*(char *)((longlong)param_1 * 0x30 + param_2 + 0x26) == '\0') {
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"AEFT_ILP_wx\n");
    }
    *(undefined1 *)((longlong)param_1 * 0x30 + param_2 + 0x26) = 1;
  }
  if (*(char *)((longlong)param_1 * 0x30 + param_2 + 0x27) == '\0') {
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"AEFT_ILP_wy\n");
    }
    *(undefined1 *)((longlong)param_1 * 0x30 + param_2 + 0x27) = 1;
  }
  if (_refptr_g_tsaPrmtFlash->bCols < *(byte *)((longlong)param_1 * 0x30 + param_2 + 0x28)) {
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"AEFT_ILP_ewx, %d\n",
                        *(undefined1 *)((longlong)param_1 * 0x30 + param_2 + 0x28));
    }
    *(undefined1 *)((longlong)param_1 * 0x30 + param_2 + 0x28) = 0;
  }
  if (_refptr_g_tsaPrmtFlash->bRows < *(byte *)((longlong)param_1 * 0x30 + param_2 + 0x29)) {
    if (*_refptr_g_tsaLogLoglevel != '\0') {
      StrPrintf_Process(0,"AEFT_ILP_ewx, %d\n",
                        *(undefined1 *)((longlong)param_1 * 0x30 + param_2 + 0x29));
    }
    *(undefined1 *)((longlong)param_1 * 0x30 + param_2 + 0x29) = 0;
  }
  if ((DAT_6bc72004 != 0) && (*(int *)_refptr_g_foldedDisplay == 0)) {
    if ((DAT_6bc72004 == 1) || (DAT_6bc72004 == 2)) {
      if ((DAT_6bc72006 + -300 < iVar1) && (iVar1 < DAT_6bc72008 + 300)) {
        *(undefined1 *)((longlong)param_1 * 0x30 + param_2 + 0x29) = 0;
        *(undefined1 *)((longlong)param_1 * 0x30 + param_2 + 0x2a) = 0;
      }
    }
    else if ((((DAT_6bc72004 == 3) || (DAT_6bc72004 == 4)) && (DAT_6bc72006 + -300 < iVar2)) &&
            (iVar2 < DAT_6bc72008 + 300)) {
      *(undefined1 *)((longlong)param_1 * 0x30 + param_2 + 0x28) = 0;
      *(undefined1 *)((longlong)param_1 * 0x30 + param_2 + 0x2b) = 0;
    }
  }
  return;
}


// ============================================================================
// Function: AFT_UpdateReportAddr @ 6baccb2a
// ============================================================================

void AFT_UpdateReportAddr(int param_1,longlong param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  iVar1 = (int)*(short *)(&DAT_6bc71c8c + (longlong)param_1 * 0x58);
  if (0 < iVar1) {
    iVar2 = (DAT_6bc72002 + 1) - iVar1;
    iVar3 = iVar2 + iVar1;
    *(short *)(&DAT_6bc71c8c + (longlong)param_1 * 0x58) =
         *(short *)(&DAT_6bc71c8c + (longlong)param_1 * 0x58) + -1;
    if (iVar3 != 0) {
      *(int *)((longlong)param_1 * 0x30 + param_2 + 8) =
           (*(int *)((longlong)param_1 * 0x30 + param_2 + 8) * iVar2 +
           *(short *)(&DAT_6bc71c92 + (longlong)param_1 * 0x58) * iVar1) / iVar3;
      *(int *)((longlong)param_1 * 0x30 + param_2 + 0xc) =
           (*(int *)((longlong)param_1 * 0x30 + param_2 + 0xc) * iVar2 +
           *(short *)(&DAT_6bc71c94 + (longlong)param_1 * 0x58) * iVar1) / iVar3;
    }
  }
  return;
}


// ============================================================================
// Function: AFT_EdgeRecoverProc1 @ 6bacfa4e
// ============================================================================

void AFT_EdgeRecoverProc1(int param_1,int param_2,ushort *param_3)

{
  int iVar1;
  int local_c;
  
  if (param_3[4] == 0) {
    local_c = 8;
  }
  else {
    local_c = 4;
  }
  if (((param_3[0xc] != 0) && (3 < *(uint *)(param_3 + 2))) &&
     (((500 < *(longlong *)_refptr_g_curTime -
              *(longlong *)(&DAT_6bc71cc0 + (longlong)param_2 * 0x58) &&
       ((*(ushort *)(&DAT_6bc71c86 + (longlong)param_2 * 0x58) & 0x400) == 0)) ||
      ((*(short *)(&DAT_6bc71c84 + (longlong)param_2 * 0x58) != 0 || ((*param_3 & 8) != 0)))))) {
    *(ushort *)(&DAT_6bc71c86 + (longlong)param_2 * 0x58) =
         *(ushort *)(&DAT_6bc71c86 + (longlong)param_2 * 0x58) | 0x80;
  }
  if ((*(int *)(param_3 + 2) == 6) || (iVar1 = AFT_MustRelation(param_2), iVar1 != 0)) {
    *(int *)(&g_aftFgInfo + (longlong)param_2 * 0x58) = local_c;
  }
  else if (((*(ushort *)(&DAT_6bc71c86 + (longlong)param_2 * 0x58) & 0x80) == 0) ||
          ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 0x10) != 0)) {
    if (((*(ushort *)(&DAT_6bc71c86 + (longlong)param_2 * 0x58) & 0x10) != 0) &&
       ((((*param_3 & 0x10) != 0 && (2 < *(uint *)(&DAT_6bc71c88 + (longlong)param_2 * 0x58))) &&
        (param_3[5] != 0)))) {
      if (DAT_6bc72052 < param_1) {
        *(int *)(&g_aftFgInfo + (longlong)param_2 * 0x58) = local_c;
      }
      if ((*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x6e6 & 0x40) == 0) {
        return;
      }
    }
    if (param_3[0xb] == 0) {
      if (DAT_6bc7204e < param_1) {
        *(undefined4 *)(&g_aftFgInfo + (longlong)param_2 * 0x58) = 10;
      }
      else if (((*(int *)(&g_aftFgInfo + (longlong)param_2 * 0x58) != local_c) &&
               ((DAT_6bc71c46 & 8) == 0)) &&
              ((DAT_6bc72050 < param_1 || ((DAT_6bc72052 < param_1 && ((*param_3 & 0x10) == 0))))))
      {
        *(undefined4 *)(&g_aftFgInfo + (longlong)param_2 * 0x58) = 10;
      }
    }
    else if ((DAT_6bc72050 < param_1) &&
            ((*(short *)(&DAT_6bc71c84 + (longlong)param_2 * 0x58) == 0 || ((*param_3 & 0x20) == 0))
            )) {
      *(undefined4 *)(&g_aftFgInfo + (longlong)param_2 * 0x58) = 10;
    }
  }
  else if (DAT_6bc72052 < param_1) {
    *(int *)(&g_aftFgInfo + (longlong)param_2 * 0x58) = local_c;
  }
  return;
}


// ============================================================================
// Function: AFT_EdgeRecoverProc2 @ 6bacff05
// ============================================================================

void AFT_EdgeRecoverProc2(int param_1,longlong param_2,ushort *param_3)

{
  short sVar1;
  short sVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  
  iVar4 = *(int *)((longlong)param_1 * 0x30 + param_2 + 8);
  iVar3 = *(int *)((longlong)param_1 * 0x30 + param_2 + 0xc);
  sVar1 = *(short *)(&DAT_6bc71cb4 + (longlong)param_1 * 0x58);
  sVar2 = *(short *)(&DAT_6bc71cb6 + (longlong)param_1 * 0x58);
  if ((*(ushort *)(&DAT_6bc71c86 + (longlong)param_1 * 0x58) & 0x10) == 0) {
    if ((sVar1 < DAT_6bc7208a) && (sVar2 < DAT_6bc7208c)) {
      iVar5 = AFT_MustRelation(param_1);
      if ((iVar5 == 0) || ((param_3[0xc] != 0 && ((*param_3 & 0x20) == 0)))) {
        *(undefined4 *)(&g_aftFgInfo + (longlong)param_1 * 0x58) = 10;
      }
      else if ((param_3[0xc] == 0) &&
              ((((param_3[4] == 0 && (DAT_6bc72046 < iVar3)) && (iVar3 < DAT_6bc72048)) ||
               (((param_3[4] == 1 && (DAT_6bc72042 < iVar4)) && (iVar4 < DAT_6bc72044)))))) {
        *(undefined4 *)(&g_aftFgInfo + (longlong)param_1 * 0x58) = 10;
      }
    }
  }
  else if (((DAT_6bc71c44 == 0) && (DAT_6bc71c42 == 0)) &&
          (iVar4 = AFT_MustRelation(param_1), iVar4 == 0)) {
    if (((((sVar1 < DAT_6bc7208a) && (sVar2 < DAT_6bc7208c)) &&
         ((int)((uint)*(ushort *)(&DAT_6bc71ca4 + (longlong)param_1 * 0x58) +
               (uint)*(ushort *)(&DAT_6bc71ca6 + (longlong)param_1 * 0x58)) < (int)DAT_6bc72074)) ||
        ((sVar1 == 0 && (param_3[4] == 0)))) || ((sVar2 == 0 && (param_3[4] == 1)))) {
      *(undefined4 *)(&g_aftFgInfo + (longlong)param_1 * 0x58) = 10;
    }
  }
  else if (((sVar1 < DAT_6bc7208a / 3) && (sVar2 < DAT_6bc7208c / 3)) ||
          (((sVar1 == 0 && (param_3[4] == 0)) || ((sVar2 == 0 && (param_3[4] == 1)))))) {
    *(undefined4 *)(&g_aftFgInfo + (longlong)param_1 * 0x58) = 10;
  }
  return;
}


// ============================================================================
// Function: AFT_ClearRelevantPalm @ 6bacbc7a
// ============================================================================

void AFT_ClearRelevantPalm(int param_1,int param_2)

{
  int iVar1;
  
  iVar1 = (*_refptr_g_tsaStaticPtr)->nField_0x264;
  if (iVar1 == 1) {
    if (param_2 < DAT_6bc72022) {
      DAT_6bc71c4a = DAT_6bc7200e;
      DAT_6bc71c48 = DAT_6bc71c48 & 0xfffe;
    }
    else {
      DAT_6bc71c4c = DAT_6bc7200e;
      DAT_6bc71c48 = DAT_6bc71c48 & 0xfffd;
    }
  }
  else if (iVar1 == 0) {
    if (param_1 < DAT_6bc72012) {
      DAT_6bc71c4a = DAT_6bc72020;
      DAT_6bc71c48 = DAT_6bc71c48 & 0xfffe;
    }
    else {
      DAT_6bc71c4c = DAT_6bc72020;
      DAT_6bc71c48 = DAT_6bc71c48 & 0xfffd;
    }
  }
  else if (iVar1 == 2) {
    if (param_1 < DAT_6bc72012) {
      DAT_6bc71c4a = DAT_6bc7201e;
      DAT_6bc71c48 = DAT_6bc71c48 & 0xfffe;
    }
    else {
      DAT_6bc71c4c = DAT_6bc7201e;
      DAT_6bc71c48 = DAT_6bc71c48 & 0xfffd;
    }
  }
  else if (iVar1 == 3) {
    if (param_2 < DAT_6bc72022) {
      DAT_6bc71c4a = DAT_6bc72010;
      DAT_6bc71c48 = DAT_6bc71c48 & 0xfffe;
    }
    else {
      DAT_6bc71c4c = DAT_6bc72010;
      DAT_6bc71c48 = DAT_6bc71c48 & 0xfffd;
    }
  }
  return;
}


