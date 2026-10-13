// ============================================================================
// Function: Image_CrossErosion @ 6ba65625
// ============================================================================

/* WARNING: Function: ___chkstk_ms replaced with injection: alloca_probe */

void Image_CrossErosion(void *param_1,int param_2,int param_3,int param_4,int param_5)

{
  errno_t eVar1;
  undefined1 local_2658 [9800];
  undefined8 uStack_10;
  
                    /* 0x25625  890  Image_CrossErosion */
  uStack_10 = 0x6ba65630;
  eVar1 = memcpy_s(local_2658,0x2648,param_1,(longlong)param_3 * (longlong)param_2 * 2);
  uStack_10 = CONCAT44(eVar1,(undefined4)uStack_10);
  if (eVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Alg/Misc/TSA_Misc.c",
                      0x173,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  if (param_4 != 0) {
    Image_CrossErosionDim1(param_1,local_2658,param_2,param_3);
  }
  if (param_5 != 0) {
    Image_CrossErosionDim2(param_1,local_2658,param_2,param_3);
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


