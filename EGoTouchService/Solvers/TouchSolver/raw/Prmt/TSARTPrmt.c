#include "TSACoreInternal.h"

// ============================================================================
// Function: TSARTPrmt_Process @ 6bb3d933
// ============================================================================

void TSARTPrmt_Process(void)

{
                    /* 0xfd933  1664  TSARTPrmt_Process */
  TSARTPrmt_Reset();
  TSA_ScenarioProcess();
  _refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax = (ushort)g_tsaRTPrmt;
  *(short *)(_refptr_g_tsaPrmtDynamic + 2) = (short)g_tsaRTPrmt;
  *(char *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x24) = (char)DAT_6bcab798;
  _refptr_g_tsaPrmtDynamic[0x78] = (char)DAT_6bcab798;
  *(char *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x26) = (char)DAT_6bcab7b0;
  _refptr_g_tsaPrmtDynamic[0x7a] = (char)DAT_6bcab7b0;
  _refptr_g_tsaPrmtFlash->field_0x81e = (char)DAT_6bcab7c8;
  _refptr_g_tsaPrmtFlash->field_0x81e = (char)DAT_6bcab7c8;
  _refptr_g_tsaPrmtFlash->field_0x81f = (char)DAT_6bcab7e0;
  _refptr_g_tsaPrmtFlash->field_0x81f = (char)DAT_6bcab7e0;
  _refptr_g_tsaPrmtFlash->field_0x820 = (char)DAT_6bcab7f8;
  _refptr_g_tsaPrmtFlash->field_0x820 = (char)DAT_6bcab7f8;
  *(short *)&_refptr_g_tsaPrmtFlash[1].field_0x4a = (short)DAT_6bcab810;
  *(short *)&_refptr_g_tsaPrmtFlash[1].field_0x4a = (short)DAT_6bcab810;
  *(short *)&_refptr_g_tsaPrmtFlash->field_0x3c = (short)DAT_6bcab828;
  *(short *)&_refptr_g_tsaPrmtFlash->field_0x3c = (short)DAT_6bcab828;
  _refptr_g_tsaPrmtFlash->field_0x5ba = (char)DAT_6bcab840;
  _refptr_g_tsaPrmtDynamic[6] = (char)DAT_6bcab840;
  *(short *)(_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 8) = (short)DAT_6bcab858;
  *(short *)(_refptr_g_tsaPrmtDynamic + 0x88) = (short)DAT_6bcab858;
  *(short *)(_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 4) = (short)DAT_6bcab870;
  *(short *)(_refptr_g_tsaPrmtDynamic + 0x74) = (short)DAT_6bcab870;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x644 = (short)DAT_6bcab888;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x644 = (short)DAT_6bcab888;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x646 = (short)DAT_6bcab8a0;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x646 = (short)DAT_6bcab8a0;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x648 = (short)DAT_6bcab8b8;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x648 = (short)DAT_6bcab8b8;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x64a = (short)DAT_6bcab8d0;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x64a = (short)DAT_6bcab8d0;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x64c = (short)DAT_6bcab8e8;
  *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x64c = (short)DAT_6bcab8e8;
  return;
}


// ============================================================================
// Function: TSARTPrmt_Reset @ 6bb3d8fe
// ============================================================================

void TSARTPrmt_Reset(void)

{
                    /* 0xfd8fe  1665  TSARTPrmt_Reset */
  TSARTPrmt_SyncWithTTPrmt();
  memcpy(&g_tsaRTPrmt,&g_tsaRTPrmtDefault,0x180);
  return;
}


// ============================================================================
// Function: TSARTPrmt_Init @ 6bb3d3e0
// ============================================================================

void TSARTPrmt_Init(void)

{
  int iVar1;
  
                    /* 0xfd3e0  1663  TSARTPrmt_Init */
  TSARTPrmt_SyncWithTTPrmt();
  DAT_6bcab904 = g_tsaRTPrmtDefault;
  DAT_6bcab908 = g_tsaRTPrmtDefault * 2;
  iVar1 = g_tsaRTPrmtDefault;
  if (g_tsaRTPrmtDefault < 0) {
    iVar1 = g_tsaRTPrmtDefault + 3;
  }
  DAT_6bcab90c = g_tsaRTPrmtDefault - (iVar1 >> 2);
  DAT_6bcab910 = 
  "touch threshold for finger mode;\ntuning based on LTI of 4mm finger without film and 5mm finger with film in device floating situation;"
  ;
  DAT_6bcab91c = DAT_6bcab918;
  DAT_6bcab920 = 100;
  DAT_6bcab924 = 0;
  DAT_6bcab928 = 
  "non-axis-change related moving distance(pixel) threshold to determine click or flick action, which will be applied to determine whether to apply click suppression or not;\nthe more threshold is the better click performance and poorer flick performance will be;"
  ;
  DAT_6bcab934 = DAT_6bcab930;
  DAT_6bcab938 = 0x96;
  DAT_6bcab93c = 0;
  DAT_6bcab940 = 
  "moving distance threshold(pixel) to determine whether to apply click suppression or not;\nthe more threshold is the better click performance and poorer flick performance will be;"
  ;
  DAT_6bcab94c = DAT_6bcab948;
  DAT_6bcab950 = 0x60;
  DAT_6bcab954 = 0x10;
  DAT_6bcab958 = 
  "128 scale up ratio to peak signal to determine touch zone traversal threshold for default situation;\nthe more threshold is the better multi-finger performance and poorer fat-finger performance will be;"
  ;
  DAT_6bcab964 = DAT_6bcab960;
  DAT_6bcab968 = 0x60;
  DAT_6bcab96c = 0x30;
  DAT_6bcab970 = 
  "128 scale up ratio to peak signal to determine touch zone traversal threshold for device grounding situation;\nthe more threshold is the better multi-finger performance and poorer fat-finger performance will be;"
  ;
  DAT_6bcab97c = DAT_6bcab978;
  DAT_6bcab980 = 0x60;
  DAT_6bcab984 = 0x10;
  DAT_6bcab988 = 
  "128 scale up ratio to peak signal to determine touch zone traversal threshold in case device floating detected;\nthe more threshold is the better multi-finger performance and poorer fat-finger performance will be;"
  ;
  DAT_6bcab994 = DAT_6bcab990;
  DAT_6bcab998 = 0x18;
  DAT_6bcab99c = 8;
  DAT_6bcab9a0 = 
  "acceleration threshold to determine whether to do line connection between 2 touches;\nthe more threshold is the better line drawing performance and poorer 2 closed finger tapping performance will be;"
  ;
  DAT_6bcab9ac = DAT_6bcab9a8;
  DAT_6bcab9b0 = 60000;
  DAT_6bcab9b4 = 0;
  DAT_6bcab9b8 = 
  "debounce time threshold in ms to enter IDLE mode after being allowed;\nthe more threshold is the better response time but more power will be consumed;"
  ;
  DAT_6bcab9c4 = DAT_6bcab9c0;
  DAT_6bcab9c8 = 0x30;
  DAT_6bcab9cc = 0x10;
  DAT_6bcab9d0 = 
  "16 scale up ratio to touch threshold to determine whether to report touch directly or debounce the touch;\nthe less threshold is the better response time but more sensitive to noise;"
  ;
  DAT_6bcab9dc = DAT_6bcab9d8;
  DAT_6bcab9e0 = 0x1e;
  DAT_6bcab9e4 = 0;
  DAT_6bcab9e8 = 
  "distance threshold in mm to activate glove mode;\nthe less threshold is the easier to enter glove mode and the higher possibility for ghost touch;"
  ;
  DAT_6bcab9f4 = DAT_6bcab9f0;
  DAT_6bcab9f8 = 60000;
  DAT_6bcab9fc = 0;
  DAT_6bcaba00 = 
  "time threshold in ms to allowed moving after click;\nthe less threshold is the easier for move and the higher for better click;"
  ;
  DAT_6bcaba0c = DAT_6bcaba08;
  DAT_6bcaba10 = 0x7fffffff;
  DAT_6bcaba14 = 0x80000000;
  DAT_6bcaba18 = "volumeActiveAreaStart";
  DAT_6bcaba24 = DAT_6bcaba20;
  DAT_6bcaba28 = 0x7fffffff;
  DAT_6bcaba2c = 0x80000000;
  DAT_6bcaba30 = "volumeActiveAreaEnd";
  DAT_6bcaba3c = DAT_6bcaba38;
  DAT_6bcaba40 = 0x7fffffff;
  DAT_6bcaba44 = 0x80000000;
  DAT_6bcaba48 = "volumeGreenAreaStart";
  DAT_6bcaba54 = DAT_6bcaba50;
  DAT_6bcaba58 = 0x7fffffff;
  DAT_6bcaba5c = 0x80000000;
  DAT_6bcaba60 = "volumeGreenAreaEnd";
  DAT_6bcaba6c = DAT_6bcaba68;
  DAT_6bcaba70 = 0x7fffffff;
  DAT_6bcaba74 = 0x80000000;
  DAT_6bcaba78 = "volumeSideEnabled";
  memcpy(&g_tsaRTPrmt,&g_tsaRTPrmtDefault,0x180);
  return;
}


// ============================================================================
// Function: TSARTPrmt_SetKeyValue @ 6bb3de92
// ============================================================================

void TSARTPrmt_SetKeyValue(undefined8 param_1,undefined4 param_2)

{
                    /* 0xfde92  1667  TSARTPrmt_SetKeyValue */
  TSARTPrmt_SetKeyValueWithReturn(param_1,param_2);
  return;
}


// ============================================================================
// Function: TSARTPrmt_SyncWithTTPrmt @ 6bb3d210
// ============================================================================

void TSARTPrmt_SyncWithTTPrmt(void)

{
                    /* 0xfd210  1669  TSARTPrmt_SyncWithTTPrmt */
  g_tsaRTPrmtDefault = (int)(short)_refptr_g_tsaPrmtFlash->wPeakMode3ThresholdMax;
  DAT_6bcab918 = (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x24);
  DAT_6bcab930 = (uint)*(byte *)((longlong)_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 0x26);
  DAT_6bcab948 = (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x81e;
  DAT_6bcab960 = (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x81f;
  DAT_6bcab978 = (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x820;
  DAT_6bcab990 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[1].field_0x4a;
  DAT_6bcab9a8 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash->field_0x3c;
  DAT_6bcab9c0 = (uint)(byte)_refptr_g_tsaPrmtFlash->field_0x5ba;
  DAT_6bcab9d8 = (uint)*(ushort *)(_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 8);
  DAT_6bcab9f0 = (uint)*(ushort *)(_refptr_g_tsaPrmtFlash[1].pDim1PitchMap + 4);
  DAT_6bcaba08 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x644;
  DAT_6bcaba20 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x646;
  DAT_6bcaba38 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x648;
  DAT_6bcaba50 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64a;
  DAT_6bcaba68 = (uint)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x64c;
  return;
}


// ============================================================================
// Function: TSARTPrmt_SetKeyValueWithReturn @ 6bb3dde9
// ============================================================================

undefined8 TSARTPrmt_SetKeyValueWithReturn(longlong param_1,undefined4 param_2)

{
  /* 栈局部变量按原栈偏移还原为 _stk（Stack[-0x11]..）：被调方会越过单个局部变量访问相邻字段 */
  _Alignas(16) uint8_t _stk[0x20];
  int iVar1;
  undefined8 uVar2;
  undefined4 local_res10 [6];
  
                    /* 0xfdde9  1668  TSARTPrmt_SetKeyValueWithReturn */
  (*(int * *)(_stk + 0x10)) = (int *)0x0;
  if (param_1 == 0) {
    uVar2 = 1;
  }
  else {
    local_res10[0] = param_2;
    iVar1 = TSARTPrmt_GetKeyIndex(param_1,&(*(undefined1 *)(_stk + 0xf)));
    if (iVar1 == 0) {
      (*(int * *)(_stk + 0x10)) = (int *)TSARTPrmt_GetAddrOfIndexedKey((*(undefined1 *)(_stk + 0xf)));
      TSARTPrmt_CopyMember(local_res10,(*(int * *)(_stk + 0x10)));
      if ((*(int * *)(_stk + 0x10))[2] < *(*(int * *)(_stk + 0x10))) {
        *(*(int * *)(_stk + 0x10)) = (*(int * *)(_stk + 0x10))[2];
      }
      if (*(*(int * *)(_stk + 0x10)) < (*(int * *)(_stk + 0x10))[3]) {
        *(*(int * *)(_stk + 0x10)) = (*(int * *)(_stk + 0x10))[3];
      }
      uVar2 = 0;
    }
    else {
      uVar2 = 1;
    }
  }
  return uVar2;
}


// ============================================================================
// Function: TSARTPrmt_CopyMember @ 6bb3dd22
// ============================================================================

void TSARTPrmt_CopyMember(undefined4 *param_1,undefined4 *param_2)

{
  *param_2 = *param_1;
  return;
}


// ============================================================================
// Function: TSARTPrmt_GetKeyIndex @ 6bb3dd3c
// ============================================================================

bool TSARTPrmt_GetKeyIndex(char *param_1,byte *param_2)

{
  int iVar1;
  bool bVar2;
  byte local_9;
  
  local_9 = 0;
  while ((local_9 < DAT_6bb4fcd0 &&
         (iVar1 = strcmp(param_1,*(char **)(PTR___compound_literal_3_6bb4fcf8 +
                                           (ulonglong)local_9 * 8)), iVar1 != 0))) {
    local_9 = local_9 + 1;
  }
  bVar2 = local_9 < DAT_6bb4fcd0;
  if (bVar2) {
    *param_2 = local_9;
  }
  return !bVar2;
}


// ============================================================================
// Function: TSARTPrmt_GetAddrOfIndexedKey @ 6bb3ddbf
// ============================================================================

longlong TSARTPrmt_GetAddrOfIndexedKey(byte param_1)

{
  return (longlong)&g_tsaRTPrmt +
         *(longlong *)(PTR___compound_literal_1_6bb4fce8 + (ulonglong)param_1 * 8);
}


