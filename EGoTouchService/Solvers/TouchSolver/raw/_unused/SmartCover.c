// ============================================================================
// Function: SmartCover_Process @ 6bb20855
// ============================================================================

void SmartCover_Process(void)

{
  char cVar1;
  char local_9;
  
                    /* 0xe0855  1427  SmartCover_Process */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&_rdata,"SmartCover_Process");
  }
  local_9 = **(char **)_refptr_g_touchesPtr;
  while (local_9 = local_9 + -1, -1 < local_9) {
    cVar1 = SmartCover_IsInOpenWindow((int)local_9);
    if (cVar1 == '\0') {
      Touch_UnitRemove(local_9);
    }
  }
  return;
}


// ============================================================================
// Function: SmartCover_IsInOpenWindow @ 6bb2062f
// ============================================================================

undefined8 SmartCover_IsInOpenWindow(char param_1)

{
  undefined8 uVar1;
  
                    /* 0xe062f  1426  SmartCover_IsInOpenWindow */
  if (*(ushort *)((longlong)(int)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170) <
      g_rangeSmartCover) {
    uVar1 = 0;
  }
  else if (DAT_6bbaa142 <
           *(ushort *)((longlong)(int)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x170))
  {
    uVar1 = 0;
  }
  else if (*(ushort *)((longlong)(int)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172) <
           DAT_6bbaa144) {
    uVar1 = 0;
  }
  else if (DAT_6bbaa146 <
           *(ushort *)((longlong)(int)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172))
  {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}


// ============================================================================
// Function: SmartCover_GetPrevTouchNumInOpenWindow @ 6bb2071c
// ============================================================================

int SmartCover_GetPrevTouchNumInOpenWindow(void)

{
  int local_10;
  int local_c;
  
                    /* 0xe071c  1424  SmartCover_GetPrevTouchNumInOpenWindow */
  local_10 = 0;
  for (local_c = 0; local_c < (int)(uint)(*_refptr_g_prevTouchesPtr)->bPrevTouchCount;
      local_c = local_c + 1) {
    if (((((*(uint *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)local_c * 0x478 + 0x1f0) &
           0x2000000) == 0) &&
         (g_rangeSmartCover <=
          *(ushort *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)local_c * 0x478 + 0x170))) &&
        (*(ushort *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)local_c * 0x478 + 0x170) <=
         DAT_6bbaa142)) &&
       ((DAT_6bbaa144 <=
         *(ushort *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)local_c * 0x478 + 0x172) &&
        (*(ushort *)((longlong)*_refptr_g_prevTouchesPtr + (longlong)local_c * 0x478 + 0x172) <=
         DAT_6bbaa146)))) {
      local_10 = local_10 + 1;
    }
  }
  return local_10;
}


