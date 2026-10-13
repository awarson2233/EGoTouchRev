// ============================================================================
// Function: SigSumFilter_ReserveTouch @ 6ba60977
// ============================================================================

void SigSumFilter_ReserveTouch(int param_1)

{
  uint uVar1;
  undefined1 local_188 [368];
  errno_t local_18;
  uint local_14;
  uint local_10;
  uint local_c;
  
                    /* 0x20977  1414  SigSumFilter_ReserveTouch */
  if (param_1 != 0) {
    for (local_10 = 0; uVar1 = local_10, (int)local_10 < (int)(**(byte **)_refptr_g_touchesPtr - 1);
        local_10 = local_10 + 1) {
      while (local_14 = uVar1 + 1, (int)local_14 < (int)(uint)**(byte **)_refptr_g_touchesPtr) {
        uVar1 = local_14;
        if (*(int *)((longlong)(int)local_10 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1d8) <
            *(int *)((longlong)(int)(local_14 & 0xff) * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1d8)) {
          local_18 = memcpy_s(local_188,0x168,
                              (void *)((longlong)(int)local_10 * 0x168 + 0x140 +
                                      *(longlong *)_refptr_g_touchesPtr),0x168);
          if (local_18 != 0) {
            StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                              "../../src/Alg/Filter/SigSumFilter/SigSumFilter_Process.c",0x4c,
                              "rc == EOK");
            HardwareAnalyzer_SetSoftException();
          }
          if (local_18 != 0) {
            return;
          }
          local_18 = memcpy_s((void *)(*(longlong *)_refptr_g_touchesPtr +
                                      (longlong)(int)local_10 * 0x168 + 0x140),0x168,
                              (void *)(*(longlong *)_refptr_g_touchesPtr +
                                      (longlong)(int)local_14 * 0x168 + 0x140),0x168);
          if (local_18 != 0) {
            StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                              "../../src/Alg/Filter/SigSumFilter/SigSumFilter_Process.c",0x51,
                              "rc == EOK");
            HardwareAnalyzer_SetSoftException();
          }
          if (local_18 != 0) {
            return;
          }
          local_18 = memcpy_s((void *)(*(longlong *)_refptr_g_touchesPtr +
                                      (longlong)(int)local_14 * 0x168 + 0x140),0x168,local_188,0x168
                             );
          if (local_18 != 0) {
            StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                              "../../src/Alg/Filter/SigSumFilter/SigSumFilter_Process.c",0x56,
                              "rc == EOK");
            HardwareAnalyzer_SetSoftException();
          }
          uVar1 = local_14;
          if (local_18 != 0) {
            return;
          }
        }
      }
    }
    local_c = (uint)**(byte **)_refptr_g_touchesPtr;
    while (local_c = local_c - 1, -1 < (int)local_c) {
      if (param_1 <= (int)local_c) {
        Touch_UnitRemove(local_c & 0xff);
      }
    }
  }
  return;
}


