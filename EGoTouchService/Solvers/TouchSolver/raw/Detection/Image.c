#include "TSACoreInternal.h"

// ============================================================================
// Function: Image_ClearNeighbor @ 6ba64f90
// ============================================================================

void Image_ClearNeighbor(longlong param_1,int param_2,int param_3,int param_4,int param_5,
                        int param_6,int param_7)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int local_10;
  int local_c;
  
  iVar1 = MAX(param_4 - param_6,0);
  iVar2 = MIN(param_6 + param_4,param_2 + -1);
  local_10 = MAX(param_5 - param_7,0);
  iVar3 = MIN(param_7 + param_5,param_3 + -1);
  for (; local_10 <= iVar3; local_10 = local_10 + 1) {
    for (local_c = iVar1; local_c <= iVar2; local_c = local_c + 1) {
      *(undefined2 *)(param_1 + (longlong)(local_c + local_10 * param_2) * 2) = 0;
    }
  }
  return;
}


// ============================================================================
// Function: Image_IsWhiteInNeighbor @ 6ba6504f
// ============================================================================

undefined8
Image_IsWhiteInNeighbor
          (longlong param_1,int param_2,int param_3,int param_4,int param_5,int param_6,int param_7)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int local_10;
  int local_c;
  
  iVar1 = MAX(param_4 - param_6,0);
  iVar2 = MIN(param_6 + param_4,param_2 + -1);
  local_10 = MAX(param_5 - param_7,0);
  iVar3 = MIN(param_7 + param_5,param_3 + -1);
  do {
    if (iVar3 < local_10) {
      return 0;
    }
    for (local_c = iVar1; local_c <= iVar2; local_c = local_c + 1) {
      if (*(short *)(param_1 + (longlong)(local_c + local_10 * param_2) * 2) != 0) {
        return 1;
      }
    }
    local_10 = local_10 + 1;
  } while( true );
}


// ============================================================================
// Function: Image_Dilation @ 6ba6511d
// ============================================================================

/* WARNING: Function: ___chkstk_ms replaced with injection: alloca_probe */

void Image_Dilation(void *param_1,int param_2,int param_3,undefined4 param_4,undefined4 param_5)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x2668]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x2670];
  int iVar1;
  
                    /* 0x2511d  891  Image_Dilation */
  (*(undefined8 *)(_stk + 0x2660)) = 0x6ba65128;
  (*(errno_t *)(_stk + 0x265c)) = memcpy_s(((short *)(_stk + 0x8)),0x2648,param_1,(longlong)param_3 * (longlong)param_2 * 2);
  if ((*(errno_t *)(_stk + 0x265c)) != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/Misc/TSA_Misc.c",
                      0x107,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  for (int col = 0; col < param_2; col++) {
    for (int row = 0; row < param_3; row++) {
      if ((((short *)(_stk + 0x8))[col + row * param_2] == 0) &&
         (Image_IsWhiteInNeighbor(((short *)(_stk + 0x8)), param_2, param_3, col, row, param_4, param_5) != 0)) {
        *(undefined2 *)((longlong)param_1 + (longlong)(col + row * param_2) * 2) = 1;
      }
    }
  }
  return;
}


// ============================================================================
// Function: Image_Erosion @ 6ba652ac
// ============================================================================

/* WARNING: Function: ___chkstk_ms replaced with injection: alloca_probe */

void Image_Erosion(void *param_1,int param_2,int param_3,undefined4 param_4,undefined4 param_5)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x2668]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x2670];
  
                    /* 0x252ac  892  Image_Erosion */
  (*(undefined8 *)(_stk + 0x2660)) = 0x6ba652b7;
  (*(errno_t *)(_stk + 0x265c)) = memcpy_s(((short *)(_stk + 0x8)),0x2648,param_1,(longlong)param_3 * (longlong)param_2 * 2);
  if ((*(errno_t *)(_stk + 0x265c)) != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/Misc/TSA_Misc.c",
                      0x11f,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  for (int row = 0; row < param_3; row++) {
    for (int col = 0; col < param_2; col++) {
      if (((short *)(_stk + 0x8))[col + row * param_2] == 0) {
        Image_ClearNeighbor((longlong)param_1, param_2, param_3, col, row, param_4, param_5);
      }
    }
  }
  return;
}


// ============================================================================
// Function: Image_CrossErosionDim1 @ 6ba6540b
// ============================================================================

void Image_CrossErosionDim1(longlong param_1,longlong param_2,uint param_3,int param_4)

{
  uint uVar1;
  uint local_c;
  
  uVar1 = param_3 * param_4;
  while (local_c = uVar1, uVar1 = local_c - 1, local_c != 0) {
    if (*(short *)(param_2 + (ulonglong)uVar1 * 2) == 0) {
      if (((uVar1 / param_3 == 0) || (*(short *)(param_2 + (ulonglong)(uVar1 - param_3) * 2) == 0))
         && ((param_4 - 1U == uVar1 / param_3 ||
             (*(short *)(param_2 + (ulonglong)(uVar1 + param_3) * 2) == 0)))) {
        if (-1 < (int)(uVar1 % param_3 - 1)) {
          *(undefined2 *)(param_1 + (ulonglong)(local_c - 2) * 2) = 0;
        }
        if ((int)(uVar1 % param_3 + 1) < (int)param_3) {
          *(undefined2 *)(param_1 + (ulonglong)local_c * 2) = 0;
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: Image_CrossErosionDim2 @ 6ba65518
// ============================================================================

void Image_CrossErosionDim2(longlong param_1,longlong param_2,uint param_3,int param_4)

{
  uint uVar1;
  uint local_c;
  
  uVar1 = param_3 * param_4;
  while (local_c = uVar1, uVar1 = local_c - 1, local_c != 0) {
    if (*(short *)(param_2 + (ulonglong)uVar1 * 2) == 0) {
      if (((uVar1 % param_3 == 0) || (*(short *)(param_2 + (ulonglong)(local_c - 2) * 2) == 0)) &&
         ((param_3 - 1 == uVar1 % param_3 || (*(short *)(param_2 + (ulonglong)local_c * 2) == 0))))
      {
        if (-1 < (int)(uVar1 / param_3 - 1)) {
          *(undefined2 *)(param_1 + (ulonglong)(uVar1 - param_3) * 2) = 0;
        }
        if ((int)(uVar1 / param_3 + 1) < (int)param_3) {
          *(undefined2 *)(param_1 + (ulonglong)(uVar1 + param_3) * 2) = 0;
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: Image_CrossErosion @ 6ba65625
// ============================================================================

/* WARNING: Function: ___chkstk_ms replaced with injection: alloca_probe */

void Image_CrossErosion(void *param_1,int param_2,int param_3,int param_4,int param_5)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x2658]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x2660];
  errno_t eVar1;
  
                    /* 0x25625  890  Image_CrossErosion */
  (*(undefined8 *)(_stk + 0x2650)) = 0x6ba65630;
  eVar1 = memcpy_s(((undefined1 *)(_stk + 0x8)),0x2648,param_1,(longlong)param_3 * (longlong)param_2 * 2);
  (*(undefined8 *)(_stk + 0x2650)) = CONCAT44(eVar1,(undefined4)(*(undefined8 *)(_stk + 0x2650)));
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/Misc/TSA_Misc.c",
                      0x173,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  if (param_4 != 0) {
    Image_CrossErosionDim1(param_1,((undefined1 *)(_stk + 0x8)),param_2,param_3);
  }
  if (param_5 != 0) {
    Image_CrossErosionDim2(param_1,((undefined1 *)(_stk + 0x8)),param_2,param_3);
  }
  return;
}


