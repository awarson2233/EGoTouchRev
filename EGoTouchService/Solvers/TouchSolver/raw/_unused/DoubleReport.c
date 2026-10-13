// ============================================================================
// Function: DoubleReport_Process @ 6baded05
// ============================================================================

bool DoubleReport_Process(void)

{
  char cVar1;
  
                    /* 0x9ed05  535  DoubleReport_Process */
  DoubleReport_TryToEnable();
  CaculateInterval();
  cVar1 = DoubleReport_IsEnabled();
  if (cVar1 != '\0') {
    DoubleReport_Calculate();
  }
  return cVar1 == '\0';
}


// ============================================================================
// Function: DoubleReport_TryToEnable @ 6bade5e8
// ============================================================================

void DoubleReport_TryToEnable(void)

{
  undefined1 local_9;
  
  g_doubleReportIsEnable = 0;
  AFE_GetNextScanRateIndex(&local_9,&g_doubleReportIsEnable);
  return;
}


// ============================================================================
// Function: DoubleReport_IsEnabled @ 6bade611
// ============================================================================

bool DoubleReport_IsEnabled(void)

{
                    /* 0x9e611  534  DoubleReport_IsEnabled */
  return g_doubleReportIsEnable == '\x01';
}


// ============================================================================
// Function: DoubleReport_Calculate @ 6bade84b
// ============================================================================

void DoubleReport_Calculate(void)

{
  byte bVar1;
  ushort uVar2;
  ushort uVar3;
  short sVar4;
  short sVar5;
  ushort uVar6;
  errno_t eVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  byte local_9;
  
  eVar7 = memcpy_s(&g_doubleReportTouchUnits,0x1c20,
                   (void *)(*(longlong *)_refptr_g_touchesPtr + 0x140),0x1c20);
  if (eVar7 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/DoubleReport/DoubleReport.c",0x9d,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  local_9 = 0;
  do {
    if (**(byte **)_refptr_g_touchesPtr <= local_9) {
      DoubleReport_UpdatePrevTouch();
      return;
    }
    bVar1 = *(byte *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x154);
    uVar2 = *(ushort *)(&g_prevTouchesDoubleReportUnits + ((longlong)(int)(uint)bVar1 + 0x29) * 4);
    uVar3 = *(ushort *)(&DAT_6bc76fc2 + ((longlong)(int)(uint)bVar1 + 0x29) * 4);
    sVar4 = *(short *)(&g_prevTouchesDoubleReportUnits + (longlong)(int)(uint)bVar1 * 4);
    sVar5 = *(short *)(&DAT_6bc76fc2 + (longlong)(int)(uint)bVar1 * 4);
    if ((((sVar4 == 0) || (sVar5 == 0)) || (uVar2 == 0)) || (uVar3 == 0)) {
      if (((sVar4 == 0) && (sVar5 == 0)) && ((uVar2 != 0 && (uVar3 != 0)))) {
        uVar6 = *(ushort *)
                 ((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
        *(short *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170)
             = (short)((int)((uint)*(ushort *)
                                    ((longlong)(int)(uint)local_9 * 0x168 +
                                     *(longlong *)_refptr_g_touchesPtr + 0x170) + (uint)uVar2) >> 1)
        ;
        *(short *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172)
             = (short)((int)((uint)uVar6 + (uint)uVar3) >> 1);
      }
    }
    else {
      iVar8 = Bezier(sVar4,uVar2,
                     *(undefined2 *)
                      ((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x170));
      iVar9 = Bezier(sVar5,uVar3,
                     *(undefined2 *)
                      ((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                      0x172));
      iVar10 = MIN(uVar2,*(undefined2 *)
                          ((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr
                          + 0x170));
      if (iVar10 <= iVar8) {
        iVar10 = MAX(uVar2,*(undefined2 *)
                            ((longlong)(int)(uint)local_9 * 0x168 +
                             *(longlong *)_refptr_g_touchesPtr + 0x170));
        if (iVar8 <= iVar10) {
          iVar10 = MIN(uVar3,*(undefined2 *)
                              ((longlong)(int)(uint)local_9 * 0x168 +
                               *(longlong *)_refptr_g_touchesPtr + 0x172));
          if (iVar10 <= iVar9) {
            iVar10 = MAX(uVar3,*(undefined2 *)
                                ((longlong)(int)(uint)local_9 * 0x168 +
                                 *(longlong *)_refptr_g_touchesPtr + 0x172));
            if (iVar9 <= iVar10) {
              *(short *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x170) = (short)iVar8;
              *(short *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x172) = (short)iVar9;
              goto LAB_6badecd9;
            }
          }
        }
      }
      uVar6 = *(ushort *)
               ((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
      *(short *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170) =
           (short)((int)((uint)*(ushort *)
                                ((longlong)(int)(uint)local_9 * 0x168 +
                                 *(longlong *)_refptr_g_touchesPtr + 0x170) + (uint)uVar2) >> 1);
      *(short *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172) =
           (short)((int)((uint)uVar6 + (uint)uVar3) >> 1);
    }
LAB_6badecd9:
    local_9 = local_9 + 1;
  } while( true );
}


// ============================================================================
// Function: DoubleReport_UpdatePrevTouch @ 6bade6de
// ============================================================================

void DoubleReport_UpdatePrevTouch(void)

{
  byte bVar1;
  errno_t eVar2;
  int iVar3;
  byte local_9;
  
  eVar2 = memcpy_s(&g_prevTouchesDoubleReportUnits,0xa4,&DAT_6bc77064,0xa4);
  if (eVar2 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/DoubleReport/DoubleReport.c",0x8a,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  iVar3 = memset_s(&DAT_6bc77064,0xa4,0,0xa4);
  if (iVar3 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/DoubleReport/DoubleReport.c",0x8f,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
    bVar1 = (&DAT_6bba11d4)[(longlong)(int)(uint)local_9 * 0x168];
    *(undefined2 *)(&g_prevTouchesDoubleReportUnits + ((longlong)(int)(uint)bVar1 + 0x29) * 4) =
         *(undefined2 *)(&DAT_6bba11f0 + (longlong)(int)(uint)local_9 * 0x168);
    *(undefined2 *)(&DAT_6bc76fc2 + ((longlong)(int)(uint)bVar1 + 0x29) * 4) =
         *(undefined2 *)(&DAT_6bba11f2 + (longlong)(int)(uint)local_9 * 0x168);
  }
  return;
}


// ============================================================================
// Function: DoubleReport_Init @ 6bade43c
// ============================================================================

void DoubleReport_Init(void)

{
  int iVar1;
  
                    /* 0x9e43c  533  DoubleReport_Init */
  iVar1 = memset_s(&g_doubleReportTouchUnits,0x1c20,0,0x1c20);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/DoubleReport/DoubleReport.c",0x26,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  g_doubleReportInit = 1;
  return;
}


// ============================================================================
// Function: DoubleReport_Scenario @ 6bade4a6
// ============================================================================

undefined8 DoubleReport_Scenario(void)

{
  char cVar1;
  short sVar2;
  undefined8 uVar3;
  uint uVar4;
  
                    /* 0x9e4a6  536  DoubleReport_Scenario */
  if (((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x200000) == 0) ||
      (g_doubleReportStatus == '\0')) || (g_doubleReportInit == '\0')) {
    uVar3 = 0;
  }
  else if (_refptr_g_tsaPrmtFlash[1].field_0x4c == '\0') {
    if (((*_refptr_g_tsaStaticPtr)->dwSystemFlags & 0xc00000) == 0) {
      cVar1 = TSA_IsGameScenario();
      if (cVar1 == '\0') {
        sVar2 = TSA_ASAAFEStatus();
        if ((-1 < sVar2) &&
           (sVar2 = TSA_ASAAFEStatus(), uVar4 = (int)sVar2 >> 0x1f,
           (((int)sVar2 ^ uVar4) - uVar4 & 3) != 0)) {
          return 0;
        }
        cVar1 = TSA_IsOKToEnterIDLEMode();
        if (cVar1 == '\0') {
          cVar1 = TSA_IsTypingScenario();
          if (cVar1 == '\0') {
            if (((_refptr_g_tsaPrmtFlash[1].field_0x4d == '\0') ||
                (*(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x18 == -1)) ||
               ((void *)(*(longlong *)&(*_refptr_g_tsaStaticPtr)->field_0x18 + 100) <=
                (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr)) {
              uVar3 = 1;
            }
            else {
              uVar3 = 0;
            }
          }
          else {
            uVar3 = 0;
          }
        }
        else {
          uVar3 = 0;
        }
      }
      else {
        uVar3 = 0;
      }
    }
    else {
      uVar3 = 0;
    }
  }
  else {
    uVar3 = 1;
  }
  return uVar3;
}


