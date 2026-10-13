#include "TSACoreInternal.h"

// ============================================================================
// Function: BadChannel_Init @ 6bb42b6c
// ============================================================================

void BadChannel_Init(void)

{
  int iVar1;
  
                    /* 0x102b6c  383  BadChannel_Init */
  iVar1 = memset_s(g_badChannelDebounce,*(undefined2 *)_refptr_g_gridBufSize,0,
                   *(undefined2 *)_refptr_g_gridBufSize);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/WorkAround/BadChannel/BadChannel.c",0x35,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: BadChannel_IsDMDDetected @ 6bb42e68
// ============================================================================

undefined8 BadChannel_IsDMDDetected(void)

{
  char cVar1;
  undefined1 local_28 [30];
  byte local_a;
  byte local_9;
  
                    /* 0x102e68  384  BadChannel_IsDMDDetected */
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&DAT_6bb601ef,"BadChannel_IsDMDDetected");
  }
  for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bCols; local_9 = local_9 + 1) {
    for (local_a = 0; local_a < _refptr_g_tsaPrmtFlash->bRows; local_a = local_a + 4) {
      cVar1 = BadChannel_DetectType1(local_9,local_a,0x41a0,42000);
      if (cVar1 != '\0') {
        return 1;
      }
    }
  }
  local_9 = 0;
  while( true ) {
    if (_refptr_g_tsaPrmtFlash->bCols <= local_9) {
      return 0;
    }
    cVar1 = BadChannel_DetectType2(local_9,0x5cf8,0xdac,local_28);
    if (cVar1 != '\0') break;
    local_9 = local_9 + 1;
  }
  return 1;
}


// ============================================================================
// Function: BadChannel_Process @ 6bb4325f
// ============================================================================

void BadChannel_Process(void)

{
                    /* 0x10325f  386  BadChannel_Process */
  BadChannel_Detect();
  return;
}


// ============================================================================
// Function: BadChannel_DetectType1 @ 6bb42be5
// ============================================================================

undefined1 BadChannel_DetectType1(byte param_1,byte param_2,ushort param_3,ushort param_4)

{
  ushort uVar1;
  ushort local_a;
  
  local_a = 0;
  while( true ) {
    if (3 < local_a) {
      return 1;
    }
    uVar1 = *(ushort *)
             ((ulonglong)
              (ushort)((ushort)param_1 + (local_a + param_2) * (ushort)_refptr_g_tsaPrmtFlash->bCols
                      ) * 2 + *(longlong *)_refptr_g_tsaBufRaw);
    if ((param_3 < uVar1) && (uVar1 < param_4)) break;
    local_a = local_a + 1;
  }
  return 0;
}


// ============================================================================
// Function: BadChannel_DetectType2 @ 6bb42c80
// ============================================================================

undefined1 BadChannel_DetectType2(byte param_1,ushort param_2,ushort param_3,longlong param_4)

{
  ushort uVar1;
  int aiStack_38 [7];
  undefined1 local_19;
  uint local_18;
  int local_14;
  int local_10;
  byte local_b;
  ushort local_a;
  
  local_10 = 0;
  for (local_a = 0; local_a < 4; local_a = local_a + 1) {
    local_14 = 0;
    aiStack_38[(int)(uint)local_a] = 0;
    *(undefined4 *)(param_4 + (ulonglong)local_a * 4) = 0xffffffff;
    local_18 = 0;
    for (local_b = (byte)local_a; local_b < _refptr_g_tsaPrmtFlash->bRows; local_b = local_b + 4) {
      uVar1 = *(ushort *)
               ((ulonglong)
                ((uint)param_1 + (uint)local_b * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff) * 2 +
               *(longlong *)_refptr_g_tsaBufRaw);
      aiStack_38[(int)(uint)local_a] = aiStack_38[(int)(uint)local_a] + (uint)uVar1;
      local_14 = local_14 + 1;
      if (local_18 < uVar1) {
        local_18 = (uint)uVar1;
        *(uint *)((ulonglong)local_a * 4 + param_4) = (uint)local_b;
      }
    }
    if (1 < local_14) {
      aiStack_38[(int)(uint)local_a] = aiStack_38[(int)(uint)local_a] - local_18;
      aiStack_38[(int)(uint)local_a] = aiStack_38[(int)(uint)local_a] / (local_14 + -1);
    }
    if (local_10 < aiStack_38[(int)(uint)local_a]) {
      local_10 = aiStack_38[(int)(uint)local_a];
    }
  }
  local_19 = 0;
  for (local_a = 0; local_a < 4; local_a = local_a + 1) {
    if ((aiStack_38[(int)(uint)local_a] < (int)(uint)param_2) &&
       ((int)(uint)param_3 < local_10 - aiStack_38[(int)(uint)local_a])) {
      local_19 = 1;
    }
    else {
      *(undefined4 *)(param_4 + (ulonglong)local_a * 4) = 0xffffffff;
    }
  }
  return local_19;
}


// ============================================================================
// Function: BadChannel_Detect @ 6bb43154
// ============================================================================

void BadChannel_Detect(void)

{
  uint uVar1;
  int iVar2;
  byte local_a;
  byte local_9;
  
  if (3 < (byte)*_refptr_g_tsaLogLoglevel) {
    StrPrintf_Process(0,&DAT_6bb601ef,"BadChannel_Detect");
  }
  BadChannel_CheckType1();
  BadChannel_CheckType2();
  local_a = 0;
  do {
    if (_refptr_g_tsaPrmtFlash->bRows <= local_a) {
      return;
    }
    for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bCols; local_9 = local_9 + 1) {
      uVar1 = (uint)local_9 + (uint)local_a * (uint)_refptr_g_tsaPrmtFlash->bCols & 0xffff;
      if (*(char *)((int)uVar1 + g_badChannelDebounce) != '\0') {
        *(char *)((int)uVar1 + g_badChannelDebounce) =
             *(char *)((int)uVar1 + g_badChannelDebounce) + -1;
      }
      if ((*(char *)((int)uVar1 + g_badChannelDebounce) != '\0') &&
         (iVar2 = TPSensor_AddToBrokenSensorList(local_9,local_a,2), iVar2 == 0)) break;
    }
    local_a = local_a + 1;
  } while( true );
}


// ============================================================================
// Function: BadChannel_CheckType1 @ 6bb42f89
// ============================================================================

void BadChannel_CheckType1(void)

{
  char cVar1;
  ushort local_c;
  byte local_a;
  byte local_9;
  
  for (local_a = 0; (int)(uint)local_a < (int)(_refptr_g_tsaPrmtFlash->bRows - 3);
      local_a = local_a + 4) {
    for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bCols; local_9 = local_9 + 1) {
      cVar1 = BadChannel_DetectType1(local_9,local_a,14000,0xb478);
      if (cVar1 != '\0') {
        for (local_c = 0; local_c < 4; local_c = local_c + 1) {
          BadChannel_SetDebounce
                    ((ushort)local_9 + (local_c + local_a) * (ushort)_refptr_g_tsaPrmtFlash->bCols);
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: BadChannel_CheckType2 @ 6bb43052
// ============================================================================

void BadChannel_CheckType2(void)

{
  char cVar1;
  int iVar2;
  int local_28 [5];
  short local_12;
  byte local_f;
  ushort local_e;
  ushort local_c;
  byte local_9;
  
  for (local_9 = 0; local_9 < _refptr_g_tsaPrmtFlash->bCols; local_9 = local_9 + 1) {
    local_28[0] = 0;
    local_28[1] = 0;
    local_28[2] = 0;
    local_28[3] = 0;
    cVar1 = BadChannel_DetectType2(local_9,21000,0x15e0,local_28);
    if (cVar1 != '\0') {
      for (local_c = 0; local_c < 4; local_c = local_c + 1) {
        if (-1 < local_28[(int)(uint)local_c]) {
          iVar2 = local_28[(int)(uint)local_c];
          if (iVar2 < 0) {
            iVar2 = iVar2 + 3;
          }
          local_f = (byte)((iVar2 >> 2) << 2);
          for (local_e = 0; local_e < 4; local_e = local_e + 1) {
            local_12 = (ushort)local_9 + (local_e + local_f) * (ushort)_refptr_g_tsaPrmtFlash->bCols
            ;
            BadChannel_SetDebounce(local_12);
          }
        }
      }
    }
  }
  return;
}


// ============================================================================
// Function: BadChannel_SetDebounce @ 6bb42f49
// ============================================================================

void BadChannel_SetDebounce(ushort param_1)

{
  ushort local_res8;
  
  local_res8 = param_1;
  if (_refptr_g_tsaPrmtFlash->wBufElementCount <= param_1) {
    local_res8 = _refptr_g_tsaPrmtFlash->wBufElementCount - 1;
  }
  *(undefined1 *)((ulonglong)local_res8 + g_badChannelDebounce) = 0x78;
  return;
}


// ============================================================================
// Function: BadChannel_Free @ 6bb42b51
// ============================================================================

void BadChannel_Free(void)

{
                    /* 0x102b51  382  BadChannel_Free */
  TSA_Free(&g_badChannelDebounce);
  return;
}


// ============================================================================
// Function: BadChannel_Malloc @ 6bb42b10
// ============================================================================

bool BadChannel_Malloc(void)

{
                    /* 0x102b10  385  BadChannel_Malloc */
  g_badChannelDebounce = TSA_MallocAndInit(*(undefined2 *)_refptr_g_gridBufSize);
  return g_badChannelDebounce != 0;
}


