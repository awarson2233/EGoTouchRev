// ============================================================================
// Function: BaselineQueue_Push @ 6bb024da
// ============================================================================

void BaselineQueue_Push(undefined8 param_1)

{
                    /* 0xc24da  401  BaselineQueue_Push */
  BaselineQueue_BlUnitUpdate
            ((ulonglong)*(ushort *)((longlong)g_ptrQueue + 10) * 0x28 + *g_ptrQueue,param_1,
             *(undefined1 *)((longlong)g_ptrQueue + 0xe));
  *(short *)((longlong)g_ptrQueue + 10) = *(short *)((longlong)g_ptrQueue + 10) + 1;
  if (*(short *)((longlong)g_ptrQueue + 10) == (short)g_ptrQueue[1]) {
    *(undefined2 *)((longlong)g_ptrQueue + 10) = 0;
  }
  if (*(char *)((longlong)g_ptrQueue + 0xe) != '\0') {
    *(undefined2 *)((longlong)g_ptrQueue + 0xc) = *(undefined2 *)((longlong)g_ptrQueue + 10);
  }
  if (*(short *)((longlong)g_ptrQueue + 0xc) == *(short *)((longlong)g_ptrQueue + 10)) {
    *(undefined1 *)((longlong)g_ptrQueue + 0xe) = 1;
  }
  else {
    *(undefined1 *)((longlong)g_ptrQueue + 0xe) = 0;
  }
  return;
}


// ============================================================================
// Function: BaselineQueue_GetLastestItem @ 6bb02731
// ============================================================================

undefined8 BaselineQueue_GetLastestItem(void)

{
  short sVar1;
  undefined8 uVar2;
  
                    /* 0xc2731  396  BaselineQueue_GetLastestItem */
  sVar1 = BaselineQueue_Count();
  uVar2 = BaselineQueue_GetIndexedItem(sVar1 + -1);
  return uVar2;
}


// ============================================================================
// Function: BaselineQueue_BlUnitUpdate @ 6bb02379
// ============================================================================

void BaselineQueue_BlUnitUpdate(undefined8 *param_1,undefined8 *param_2,char param_3)

{
  char cVar1;
  errno_t eVar2;
  
  *param_1 = *param_2;
  param_1[1] = param_2[1];
  *(undefined4 *)(param_1 + 2) = *(undefined4 *)(param_2 + 2);
  *(undefined4 *)((longlong)param_1 + 0x14) = *(undefined4 *)((longlong)param_2 + 0x14);
  *(undefined2 *)(param_1 + 3) = *(undefined2 *)(param_2 + 3);
  cVar1 = TSAFold_IsSupported();
  if ((cVar1 != '\0') && ((param_3 == '\0' || (*(int *)((longlong)param_1 + 0x1c) != 0)))) {
    *(undefined4 *)((longlong)param_1 + 0x1c) = *(undefined4 *)((longlong)param_2 + 0x1c);
  }
  cVar1 = TSAFold_IsSupported();
  if (cVar1 == '\0') {
    eVar2 = memcpy_s((void *)param_1[4],(ulonglong)*(ushort *)_refptr_g_gridBufSize * 2,
                     (void *)param_2[4],(ulonglong)*(ushort *)_refptr_g_gridBufSize * 2);
    if (eVar2 != 0) {
      StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                        "../../src/Extension/SafeBaseline/BaselineQueue/BaselineQueue.c",0x6f,
                        "rc == EOK");
      HardwareAnalyzer_SetSoftException();
    }
  }
  else {
    TSAFold_CopyRawRegionToWhole
              (param_1[4],
               &_refptr_g_tsaPrmtFlash[2].field_0x750 +
               (ulonglong)*(uint *)((longlong)param_2 + 0x1c) * 0x14,param_2[4]);
  }
  return;
}


// ============================================================================
// Function: BaselineQueue_IsFull @ 6bb02781
// ============================================================================

undefined1 BaselineQueue_IsFull(void)

{
                    /* 0xc2781  399  BaselineQueue_IsFull */
  return *(undefined1 *)(g_ptrQueue + 0xe);
}


// ============================================================================
// Function: BaselineQueue_Count @ 6bb02795
// ============================================================================

uint BaselineQueue_Count(void)

{
  uint local_c;
  
                    /* 0xc2795  394  BaselineQueue_Count */
  local_c = (uint)*(ushort *)(g_ptrQueue + 10) - (uint)*(ushort *)(g_ptrQueue + 0xc);
  if ((int)local_c < 0) {
    local_c = ((uint)*(ushort *)(g_ptrQueue + 10) + (uint)*(ushort *)(g_ptrQueue + 8)) -
              (uint)*(ushort *)(g_ptrQueue + 0xc);
  }
  else if ((local_c == 0) && (*(char *)(g_ptrQueue + 0xe) != '\0')) {
    local_c = (uint)*(ushort *)(g_ptrQueue + 8);
  }
  return local_c;
}


// ============================================================================
// Function: BaselineQueue_Reset @ 6bb021ae
// ============================================================================

void BaselineQueue_Reset(void)

{
                    /* 0xc21ae  402  BaselineQueue_Reset */
  *(undefined2 *)(g_ptrQueue + 0xc) = 0;
  *(undefined2 *)(g_ptrQueue + 10) = 0;
  *(undefined1 *)(g_ptrQueue + 0xe) = 0;
  return;
}


// ============================================================================
// Function: BaselineQueue_GetIndexedItem @ 6bb026c0
// ============================================================================

longlong BaselineQueue_GetIndexedItem(ushort param_1)

{
                    /* 0xc26c0  395  BaselineQueue_GetIndexedItem */
  return ((ulonglong)((uint)*(ushort *)((longlong)g_ptrQueue + 0xc) + (uint)param_1) %
         (ulonglong)(longlong)(int)(uint)*(ushort *)(g_ptrQueue + 1)) * 0x28 + *g_ptrQueue;
}


// ============================================================================
// Function: BaselineQueue_Init @ 6bb0226e
// ============================================================================

void BaselineQueue_Init(undefined8 param_1,undefined2 param_2,undefined8 param_3,undefined2 param_4)

{
                    /* 0xc226e  397  BaselineQueue_Init */
  BaselineQueue_InitTemp(param_3,param_4);
  BaselineQueue_InitNormal(param_1,param_2);
  return;
}


// ============================================================================
// Function: BaselineQueue_SwitchToNormal @ 6bb02180
// ============================================================================

void BaselineQueue_SwitchToNormal(void)

{
                    /* 0xc2180  403  BaselineQueue_SwitchToNormal */
  g_ptrQueue = &g_baselineQueue;
  return;
}


// ============================================================================
// Function: BaselineQueue_SwitchToTemp @ 6bb02197
// ============================================================================

void BaselineQueue_SwitchToTemp(void)

{
                    /* 0xc2197  404  BaselineQueue_SwitchToTemp */
  g_ptrQueue = &g_baselineQueueTemp;
  return;
}


// ============================================================================
// Function: BaselineQueue_InitNormal @ 6bb021e2
// ============================================================================

void BaselineQueue_InitNormal(undefined8 param_1,undefined2 param_2)

{
  BaselineQueue_SwitchToNormal();
  *g_ptrQueue = param_1;
  *(undefined2 *)(g_ptrQueue + 1) = param_2;
  BaselineQueue_Reset();
  return;
}


// ============================================================================
// Function: BaselineQueue_InitTemp @ 6bb02228
// ============================================================================

void BaselineQueue_InitTemp(undefined8 param_1,undefined2 param_2)

{
  BaselineQueue_SwitchToTemp();
  *g_ptrQueue = param_1;
  *(undefined2 *)(g_ptrQueue + 1) = param_2;
  BaselineQueue_Reset();
  return;
}


// ============================================================================
// Function: BaselineQueue_Pop @ 6bb025ee
// ============================================================================

longlong BaselineQueue_Pop(void)

{
  ushort uVar1;
  short sVar2;
  longlong local_10;
  
                    /* 0xc25ee  400  BaselineQueue_Pop */
  sVar2 = BaselineQueue_Count();
  if (sVar2 == 0) {
    local_10 = (ulonglong)*(ushort *)((longlong)g_ptrQueue + 0xc) * 0x28 + *g_ptrQueue;
  }
  else {
    local_10 = *g_ptrQueue;
    uVar1 = *(ushort *)((longlong)g_ptrQueue + 0xc);
    *(ushort *)((longlong)g_ptrQueue + 0xc) = uVar1 + 1;
    local_10 = (ulonglong)uVar1 * 0x28 + local_10;
  }
  if (*(short *)((longlong)g_ptrQueue + 0xc) == (short)g_ptrQueue[1]) {
    *(undefined2 *)((longlong)g_ptrQueue + 0xc) = 0;
  }
  *(undefined1 *)((longlong)g_ptrQueue + 0xe) = 0;
  return local_10;
}


