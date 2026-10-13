// ============================================================================
// Function: AccQueue_Init @ 6ba6aed0
// ============================================================================

void AccQueue_Init(undefined8 param_1,undefined2 param_2)

{
                    /* 0x2aed0  325  AccQueue_Init */
  g_accQueue = param_1;
  DAT_6bbc0e08 = param_2;
  DAT_6bbc0e0c = 0;
  DAT_6bbc0e0a = 0;
  return;
}


