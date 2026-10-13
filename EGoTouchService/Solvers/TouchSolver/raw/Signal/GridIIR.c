// ============================================================================
// Function: GridIIR_Process @ 6ba61098
// ============================================================================

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

void GridIIR_Process(void)

{
  char cNoiseState;
  errno_t copyStatus;
  undefined8 in_stack_ffffffffffffffd8;
  undefined4 dwIirParamUpper;
  
                    /* 0x21098  637  GridIIR_Process */
  dwIirParamUpper = (undefined4)((ulonglong)in_stack_ffffffffffffffd8 >> 0x20);
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&_rdata,"GridIIR_Process");
  }
  cNoiseState = TSAStatic_IsAllFreqBecomeNoisy();
  if (cNoiseState == '\0') {
    cNoiseState = TSAStatic_IsAllFreqNoisy();
    if (cNoiseState != '\0') {
      GridIIR_ProcessCore(*(undefined8 *)_refptr_g_tsaBufDif,*(undefined8 *)_refptr_g_tsaBufDif,
                          *(undefined8 *)_refptr_g_tsaBufGridIIRDif,
                          _refptr_g_tsaPrmtFlash->wBufElementCount,
                          CONCAT44(dwIirParamUpper,
                                   (uint)*(byte *)(_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 3)));
    }
  }
  else {
    copyStatus = memcpy_s(*(void **)_refptr_g_tsaBufGridIIRDif,0x2648,*(void **)_refptr_g_tsaBufDif,
                          (ulonglong)_refptr_g_tsaPrmtFlash->wBufElementCount * 2);
    if (copyStatus != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Alg/GridIIR/GridIIR.c",0x33,"rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
  }
  return;
}


// ============================================================================
// Function: GridIIR_ProcessCore @ 6ba60fd5
// ============================================================================

void GridIIR_ProcessCore(longlong param_1,longlong param_2,longlong param_3,ushort param_4,
                        undefined1 param_5)

{
  undefined2 uVar1;
  ushort local_a;
  
  for (local_a = 0; local_a < param_4; local_a = local_a + 1) {
    uVar1 = IIR_Process2((int)*(short *)(param_1 + (ulonglong)local_a * 2),
                         (int)*(short *)(param_3 + (ulonglong)local_a * 2),param_5,8);
    *(undefined2 *)(param_2 + (ulonglong)local_a * 2) = uVar1;
    *(undefined2 *)((ulonglong)local_a * 2 + param_3) =
         *(undefined2 *)(param_2 + (ulonglong)local_a * 2);
  }
  return;
}


// ============================================================================
// Function: GridIIR_ProcessCoreRaw @ 6ba60f10
// ============================================================================

void GridIIR_ProcessCoreRaw
               (longlong param_1,longlong param_2,longlong param_3,ushort param_4,undefined1 param_5
               )

{
  undefined2 uVar1;
  ushort local_a;
  
                    /* 0x20f10  638  GridIIR_ProcessCoreRaw */
  for (local_a = 0; local_a < param_4; local_a = local_a + 1) {
    uVar1 = IIR_Process2(*(undefined2 *)(param_1 + (ulonglong)local_a * 2),
                         *(undefined2 *)(param_3 + (ulonglong)local_a * 2),param_5,8);
    *(undefined2 *)(param_2 + (ulonglong)local_a * 2) = uVar1;
    *(undefined2 *)((ulonglong)local_a * 2 + param_3) =
         *(undefined2 *)(param_2 + (ulonglong)local_a * 2);
  }
  return;
}


