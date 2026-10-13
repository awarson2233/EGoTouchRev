// ============================================================================
// Function: HotZoneAdjust_Process @ 6baf729e
// ============================================================================

void HotZoneAdjust_Process(byte param_1,byte param_2)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0xb729e  883  HotZoneAdjust_Process */
  cVar1 = HotZoneAdjustIsFeatureOn();
  if (cVar1 != '\0') {
    cVar1 = HotZoneAdjustScenario();
    if (cVar1 == '\0') {
      ClearHotZone();
    }
    else {
      if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1f8) & 8) != 0) {
        if (((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_2 * 0x478 + 0x1f8) & 8) == 0) ||
           (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x1f8) & 0x20000) != 0 &&
            ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                       (longlong)(int)(uint)param_2 * 0x478 + 0x1f8) & 0x20000) == 0)))) {
          HotZoneAdjustProcessFirst(param_1);
        }
        else {
          HotZoneAdjustProcessAfter(param_1,param_2);
        }
      }
      if (((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1f8) & 8) == 0) &&
         ((*(uint *)((longlong)*_refptr_g_prevTouchesPtr +
                    (longlong)(int)(uint)param_2 * 0x478 + 0x1f8) & 8) != 0)) {
        cVar1 = IsNeedRecord(param_2,&g_hotZoneAnalyzer);
        if (cVar1 != '\0') {
          HotZoneRecord(param_2,&g_hotZoneAnalyzer);
        }
        StrPrintf_Process(0,"\nAppLog: ");
        uVar2 = GenerateAndGetDetailDescription();
        StrPrintf_Process(0x10,&DAT_6bb5814a,uVar2);
        ClearHotZone();
      }
    }
  }
  return;
}


// ============================================================================
// Function: HotZoneAdjust_Init @ 6baf3fdd
// ============================================================================

void HotZoneAdjust_Init(void)

{
                    /* 0xb3fdd  882  HotZoneAdjust_Init */
  HotZoneAdjustClearAnalyzer();
  g_hotZoneAnalyzer = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  return;
}


