// ============================================================================
// Function: PrevPeak_Process @ 6ba68f77
// ============================================================================

void PrevPeak_Process(void)

{
  byte local_9;
  
                    /* 0x28f77  1049  PrevPeak_Process */
  *g_prevPeaksPtr = (*_refptr_g_peaksPtr)->bPeakCount;
  *(undefined2 *)(g_prevPeaksPtr + 2) = *(undefined2 *)(*_refptr_g_peaksPtr + 2);
  for (local_9 = 0; local_9 < (*_refptr_g_peaksPtr)->bPeakCount; local_9 = local_9 + 1) {
    *(undefined4 *)(g_prevPeaksPtr + (longlong)(int)(uint)local_9 * 0xc + 4) =
         *(undefined4 *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)local_9 * 0x14 + 0x10);
    *(undefined4 *)(g_prevPeaksPtr + (longlong)(int)(uint)local_9 * 0xc + 8) =
         *(undefined4 *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)local_9 * 0x14 + 0x20);
    *(undefined2 *)(g_prevPeaksPtr + (longlong)(int)(uint)local_9 * 0xc + 0xc) =
         *(undefined2 *)(*_refptr_g_peaksPtr + (longlong)(int)(uint)local_9 * 0x14 + 0x18);
    g_prevPeaksPtr[(longlong)(int)(uint)local_9 * 0xc + 0xe] =
         (*_refptr_g_peaksPtr)[(longlong)(int)(uint)local_9 * 0x14 + 0x15].bPeakCount;
    g_prevPeaksPtr[(longlong)(int)(uint)local_9 * 0xc + 0xf] =
         (*_refptr_g_peaksPtr)[(longlong)(int)(uint)local_9 * 0x14 + 0x16].bPeakCount;
  }
  return;
}


// ============================================================================
// Function: PrevPeak_Init @ 6ba68f10
// ============================================================================

void PrevPeak_Init(void)

{
  int iVar1;
  
                    /* 0x28f10  1041  PrevPeak_Init */
  iVar1 = memset_s(g_prevPeaksPtr,0xf4,0,0xf4);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Alg/Peak/PrevPeakData.c",0x14,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: PrevPeak_IsOnOuterEdge @ 6ba692c0
// ============================================================================

undefined8 PrevPeak_IsOnOuterEdge(undefined1 param_1)

{
  char cVar1;
  
                    /* 0x292c0  1042  PrevPeak_IsOnOuterEdge */
  cVar1 = PrevPeak_IsOnOuterEdgeDim1(param_1);
  if ((cVar1 == '\0') && (cVar1 = PrevPeak_IsOnOuterEdgeDim2(param_1), cVar1 == '\0')) {
    return 0;
  }
  return 1;
}


// ============================================================================
// Function: PrevPeak_IsOnOuterEdgeDim1 @ 6ba69246
// ============================================================================

undefined8 PrevPeak_IsOnOuterEdgeDim1(undefined1 param_1)

{
  char cVar1;
  
                    /* 0x29246  1043  PrevPeak_IsOnOuterEdgeDim1 */
  cVar1 = PrevPeak_IsOnOuterEdgeDim1Near(param_1);
  if ((cVar1 == '\0') && (cVar1 = PrevPeak_IsOnOuterEdgeDim1Far(param_1), cVar1 == '\0')) {
    return 0;
  }
  return 1;
}


// ============================================================================
// Function: PrevPeak_IsOnOuterEdgeDim2 @ 6ba69283
// ============================================================================

undefined8 PrevPeak_IsOnOuterEdgeDim2(undefined1 param_1)

{
  char cVar1;
  
                    /* 0x29283  1046  PrevPeak_IsOnOuterEdgeDim2 */
  cVar1 = PrevPeak_IsOnOuterEdgeDim2Near(param_1);
  if ((cVar1 == '\0') && (cVar1 = PrevPeak_IsOnOuterEdgeDim2Far(param_1), cVar1 == '\0')) {
    return 0;
  }
  return 1;
}


// ============================================================================
// Function: PrevPeak_IsOnOuterEdgeDim1Near @ 6ba69156
// ============================================================================

bool PrevPeak_IsOnOuterEdgeDim1Near(byte param_1)

{
                    /* 0x29156  1045  PrevPeak_IsOnOuterEdgeDim1Near */
  return g_prevPeaksPtr[(longlong)(int)(uint)param_1 * 0xc + 0xe] == '\0';
}


// ============================================================================
// Function: PrevPeak_IsOnOuterEdgeDim1Far @ 6ba69188
// ============================================================================

undefined4 PrevPeak_IsOnOuterEdgeDim1Far(byte param_1)

{
  uint uVar1;
  
                    /* 0x29188  1044  PrevPeak_IsOnOuterEdgeDim1Far */
  uVar1 = _refptr_g_tsaPrmtFlash->bCols - 1;
  return CONCAT31((int3)(uVar1 >> 8),
                  (byte)g_prevPeaksPtr[(longlong)(int)(uint)param_1 * 0xc + 0xe] == uVar1);
}


// ============================================================================
// Function: PrevPeak_IsOnOuterEdgeDim2Near @ 6ba691ce
// ============================================================================

bool PrevPeak_IsOnOuterEdgeDim2Near(byte param_1)

{
                    /* 0x291ce  1048  PrevPeak_IsOnOuterEdgeDim2Near */
  return g_prevPeaksPtr[(longlong)(int)(uint)param_1 * 0xc + 0xf] == '\0';
}


// ============================================================================
// Function: PrevPeak_IsOnOuterEdgeDim2Far @ 6ba69200
// ============================================================================

undefined4 PrevPeak_IsOnOuterEdgeDim2Far(byte param_1)

{
  uint uVar1;
  
                    /* 0x29200  1047  PrevPeak_IsOnOuterEdgeDim2Far */
  uVar1 = _refptr_g_tsaPrmtFlash->bRows - 1;
  return CONCAT31((int3)(uVar1 >> 8),
                  (byte)g_prevPeaksPtr[(longlong)(int)(uint)param_1 * 0xc + 0xf] == uVar1);
}


