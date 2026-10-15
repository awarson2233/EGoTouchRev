// ============================================================================
// Function: Proximtiy_SignalProcess @ 6bafb057
// ============================================================================

void Proximtiy_SignalProcess(void)

{
                    /* 0xbb057  1130  Proximtiy_SignalProcess */
  if ((*(uint *)(g_proxData + 0x54) & 6) != 0) {
    Proximity_RawProcess();
    Proximity_BaselineProcess();
    Proximity_DifProcess();
  }
  if ((*(uint *)(g_proxData + 0x54) & 4) == 0) {
    Proximity_SelfCalc();
  }
  else {
    Proximity_ZnProcess(g_tsaBufProxDif,g_tsaBufProxZnDif);
  }
  *(undefined4 *)(g_proxData + 8) = *(undefined4 *)(g_proxData + 0x48);
  if (*(char *)(g_proxData + 1) == '\0') {
    *(undefined4 *)(g_proxData + 0xc) = 0;
  }
  else if (*(int *)(g_proxData + 0xc) < *(int *)(g_proxData + 8)) {
    *(undefined4 *)(g_proxData + 0xc) = *(undefined4 *)(g_proxData + 8);
  }
  Proximity_DynamicThreshold();
  return;
}


