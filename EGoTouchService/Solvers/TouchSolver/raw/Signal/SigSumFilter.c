#include "TSACoreInternal.h"

// ============================================================================
// Function: SigSumFilter_ReserveTouch @ 6ba60977
// ============================================================================

void SigSumFilter_ReserveTouch(int param_1)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x188]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x190];
  uint uVar1;
  
                    /* 0x20977  1414  SigSumFilter_ReserveTouch */
  if (param_1 != 0) {
    for ((*(uint *)(_stk + 0x180)) = 0; uVar1 = (*(uint *)(_stk + 0x180)), (int)(*(uint *)(_stk + 0x180)) < (int)(**(byte **)_refptr_g_touchesPtr - 1);
        (*(uint *)(_stk + 0x180)) = (*(uint *)(_stk + 0x180)) + 1) {
      while ((*(uint *)(_stk + 0x17c)) = uVar1 + 1, (int)(*(uint *)(_stk + 0x17c)) < (int)(uint)**(byte **)_refptr_g_touchesPtr) {
        uVar1 = (*(uint *)(_stk + 0x17c));
        if (*(int *)((longlong)(int)(*(uint *)(_stk + 0x180)) * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x1d8) <
            *(int *)((longlong)(int)((*(uint *)(_stk + 0x17c)) & 0xff) * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                    0x1d8)) {
          (*(errno_t *)(_stk + 0x178)) = memcpy_s(((undefined1 *)(_stk + 0x8)),0x168,
                              (void *)((longlong)(int)(*(uint *)(_stk + 0x180)) * 0x168 + 0x140 +
                                      *(longlong *)_refptr_g_touchesPtr),0x168);
          if ((*(errno_t *)(_stk + 0x178)) != 0) {
            StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                              "../../src/Alg/Filter/SigSumFilter/SigSumFilter_Process.c",0x4c,
                              "rc == EOK");
            HardwareAnalyzer_SetSoftException();
          }
          if ((*(errno_t *)(_stk + 0x178)) != 0) {
            return;
          }
          (*(errno_t *)(_stk + 0x178)) = memcpy_s((void *)(*(longlong *)_refptr_g_touchesPtr +
                                      (longlong)(int)(*(uint *)(_stk + 0x180)) * 0x168 + 0x140),0x168,
                              (void *)(*(longlong *)_refptr_g_touchesPtr +
                                      (longlong)(int)(*(uint *)(_stk + 0x17c)) * 0x168 + 0x140),0x168);
          if ((*(errno_t *)(_stk + 0x178)) != 0) {
            StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                              "../../src/Alg/Filter/SigSumFilter/SigSumFilter_Process.c",0x51,
                              "rc == EOK");
            HardwareAnalyzer_SetSoftException();
          }
          if ((*(errno_t *)(_stk + 0x178)) != 0) {
            return;
          }
          (*(errno_t *)(_stk + 0x178)) = memcpy_s((void *)(*(longlong *)_refptr_g_touchesPtr +
                                      (longlong)(int)(*(uint *)(_stk + 0x17c)) * 0x168 + 0x140),0x168,((undefined1 *)(_stk + 0x8)),0x168
                             );
          if ((*(errno_t *)(_stk + 0x178)) != 0) {
            StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                              "../../src/Alg/Filter/SigSumFilter/SigSumFilter_Process.c",0x56,
                              "rc == EOK");
            HardwareAnalyzer_SetSoftException();
          }
          uVar1 = (*(uint *)(_stk + 0x17c));
          if ((*(errno_t *)(_stk + 0x178)) != 0) {
            return;
          }
        }
      }
    }
    (*(uint *)(_stk + 0x184)) = (uint)**(byte **)_refptr_g_touchesPtr;
    while ((*(uint *)(_stk + 0x184)) = (*(uint *)(_stk + 0x184)) - 1, -1 < (int)(*(uint *)(_stk + 0x184))) {
      if (param_1 <= (int)(*(uint *)(_stk + 0x184))) {
        Touch_UnitRemove((*(uint *)(_stk + 0x184)) & 0xff);
      }
    }
  }
  return;
}


