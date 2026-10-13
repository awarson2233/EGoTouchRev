// ============================================================================
// Function: HPP3_FreqShiftInit @ 6bab6b50
// ============================================================================

void HPP3_FreqShiftInit(void)

{
  byte local_9;
  
                    /* 0x76b50  823  HPP3_FreqShiftInit */
  g_asaHpp3FreqShiftEnable = 0;
  if (*(longlong *)&_refptr_g_asaStatic->field_0x40 == 0) {
    g_asaHpp3Freq = 0;
  }
  else {
    g_asaHpp3Freq = *(byte *)(*(longlong *)&_refptr_g_asaStatic->field_0x40 + 0x44);
    if (0x14 < g_asaHpp3Freq) {
      g_asaHpp3Freq = 0x14;
    }
    HPP3_EnableFreqShift();
    if (g_asaHpp3Freq != 0) {
      for (local_9 = 0; local_9 < g_asaHpp3Freq; local_9 = local_9 + 1) {
        (&DAT_6bb9e610)[(longlong)(int)(uint)local_9 * 6] =
             (char)*(undefined2 *)
                    (*(longlong *)(*(longlong *)&_refptr_g_asaStatic->field_0x40 + 0x48) +
                    (ulonglong)local_9 * 2);
        *(undefined2 *)(&DAT_6bb9e612 + (longlong)(int)(uint)local_9 * 6) = 0xffff;
        *(undefined2 *)(&DAT_6bb9e614 + (longlong)(int)(uint)local_9 * 6) = 0xffff;
      }
    }
  }
  return;
}


// ============================================================================
// Function: HPP3_EnableFreqShift @ 6bab6c66
// ============================================================================

void HPP3_EnableFreqShift(void)

{
                    /* 0x76c66  821  HPP3_EnableFreqShift */
  g_asaHpp3FreqShiftEnable = 1;
  return;
}


