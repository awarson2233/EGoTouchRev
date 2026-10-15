// ============================================================================
// Function: VirtualKey_Process @ 6bb3428b
// ============================================================================

void VirtualKey_Process(void)

{
  char cVar1;
  byte bVar2;
  undefined1 uVar3;
  short sVar4;
  uint uVar5;
  byte local_a;
  byte local_9;
  
                    /* 0xf428b  2587  VirtualKey_Process */
  VirtualKey_AutoLiftOff();
  g_outVirtualKey = 0;
  for (local_9 = 0; local_9 < g_dataVirtualKey; local_9 = local_9 + 1) {
    if ((&DAT_6bca3b4a)[(longlong)(int)(uint)local_9 * 0x10] !=
        (&DAT_6bca3c0a)[(longlong)(int)(uint)local_9 * 0x10]) {
      *(undefined2 *)(&DAT_6bca3cc8 + (longlong)(int)(uint)g_outVirtualKey * 0x10) =
           (&DAT_6bca3b48)[(longlong)(int)(uint)local_9 * 8];
      (&DAT_6bca3cca)[(longlong)(int)(uint)g_outVirtualKey * 0x10] =
           (&DAT_6bca3b4a)[(longlong)(int)(uint)local_9 * 0x10];
      g_outVirtualKey = g_outVirtualKey + 1;
      VirtualKey_UnitProcess(local_9);
    }
  }
  memcpy(&g_dataVirtualKeyPrev,&g_dataVirtualKey,0xa8);
  local_a = 0;
  do {
    bVar2 = TSA_RptVitualKeyEventNum();
    if (bVar2 <= local_a) {
      uVar3 = TSAIDE_GetIndexVolume();
      TSAIDE_LogSensor(uVar3,0,1);
      return;
    }
    cVar1 = TSA_RptVirtualKeyAction(local_a);
    if (cVar1 == '\x01') {
      cVar1 = TSA_IsVolumeControlEnabled();
      if (cVar1 == '\0') {
        cVar1 = SideTouch_IsSHBVolumeEventEnabled();
        if (cVar1 != '\0') goto LAB_6bb343e7;
      }
      else {
LAB_6bb343e7:
        sVar4 = TSA_RptVirtualKeyCode(local_a);
        if (sVar4 == 0x73) {
          cVar1 = TSAIDE_GetIndexVolume();
          TSAIDE_SetIndexVolume(cVar1 + -1);
        }
        else {
          sVar4 = TSA_RptVirtualKeyCode(local_a);
          if (sVar4 == 0x72) {
            cVar1 = TSAIDE_GetIndexVolume();
            TSAIDE_SetIndexVolume(cVar1 + '\x01');
          }
          else {
            sVar4 = TSA_RptVirtualKeyCode(local_a);
            if (sVar4 == 0x71) {
              TSAIDE_SetIndexVolume(0);
            }
          }
        }
      }
      sVar4 = TSA_RptVirtualKeyCode(local_a);
      if (sVar4 == 0x74) {
        uVar5 = TSAIDE_GetState();
        if ((uVar5 & 1) == 0) {
          uVar5 = TSAIDE_GetState();
          TSAIDE_SetState(uVar5 | 1);
        }
        else {
          uVar5 = TSAIDE_GetState();
          TSAIDE_SetState(uVar5 & 0xfffffffe);
        }
      }
    }
    local_a = local_a + 1;
  } while( true );
}


// ============================================================================
// Function: VirtualKey_Init @ 6bb33c60
// ============================================================================

void VirtualKey_Init(void)

{
  int iVar1;
  
                    /* 0xf3c60  2585  VirtualKey_Init */
  iVar1 = memset_s(&g_outVirtualKey,0xa8,0,0xa8);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/VirtualKey/VirtualKey.c",0x22,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  DAT_6bca3b48 = 0x73;
  DAT_6bca3b4a = 0;
  DAT_6bca3b4c = (undefined2)*(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x680;
  DAT_6bca3b50 = 0;
  DAT_6bca3b58 = 0x72;
  DAT_6bca3b5a = 0;
  DAT_6bca3b5c = (undefined2)*(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x680;
  DAT_6bca3b60 = 0;
  DAT_6bca3b68 = 0x74;
  DAT_6bca3b6a = 0;
  DAT_6bca3b6c = 0x32;
  DAT_6bca3b70 = 0;
  DAT_6bca3b78 = 0x71;
  DAT_6bca3b7a = 0;
  DAT_6bca3b7c = (undefined2)*(undefined4 *)&_refptr_g_tsaPrmtFlash[2].field_0x680;
  DAT_6bca3b80 = 0;
  g_dataVirtualKey = 4;
  memcpy(&g_dataVirtualKeyPrev,&g_dataVirtualKey,0xa8);
  VirtualKey_VolumeInit();
  return;
}


// ============================================================================
// Function: VirtualKey_AutoLiftOff @ 6bb33f8a
// ============================================================================

void VirtualKey_AutoLiftOff(void)

{
  byte local_9;
  
                    /* 0xf3f8a  2577  VirtualKey_AutoLiftOff */
  for (local_9 = 0; local_9 < g_dataVirtualKey; local_9 = local_9 + 1) {
    VirtualKey_Set((&DAT_6bca3b48)[(longlong)(int)(uint)local_9 * 8],0);
  }
  return;
}


// ============================================================================
// Function: VirtualKey_UnitProcess @ 6bb341e1
// ============================================================================

void VirtualKey_UnitProcess(byte param_1)

{
                    /* 0xf41e1  2592  VirtualKey_UnitProcess */
  if ((&DAT_6bca3b48)[(longlong)(int)(uint)param_1 * 8] == 0x73) {
    VirtualKey_VolumnUp();
  }
  else if ((&DAT_6bca3b48)[(longlong)(int)(uint)param_1 * 8] == 0x72) {
    VirtualKey_VolumnDown();
  }
  else if ((&DAT_6bca3b48)[(longlong)(int)(uint)param_1 * 8] == 0x74) {
    VirtualKey_Power();
  }
  else if ((&DAT_6bca3b48)[(longlong)(int)(uint)param_1 * 8] == 0x71) {
    VirtualKey_VolumnMute();
  }
  return;
}


// ============================================================================
// Function: VirtualKey_VolumeInit @ 6bb344d0
// ============================================================================

void VirtualKey_VolumeInit(void)

{
                    /* 0xf44d0  2595  VirtualKey_VolumeInit */
  DAT_6bca3d7a = 0x96;
  DAT_6bca3d78 = 10;
  DAT_6bca3d76 = 0x50;
  DAT_6bca3d72 = 0x50;
  DAT_6bca3d74 = 0x50;
  g_volumeData = 0x50;
  DAT_6bca3d7c = 8;
  DAT_6bca3d7e = 0;
  TSAIDE_SetIndexVolume(8);
  g_timeStampVolumeAction = 0;
  return;
}


// ============================================================================
// Function: VirtualKey_VolumeIsGUIActive @ 6bb3476d
// ============================================================================

undefined1 VirtualKey_VolumeIsGUIActive(void)

{
                    /* 0xf476d  2596  VirtualKey_VolumeIsGUIActive */
  return DAT_6bca3d7e;
}


// ============================================================================
// Function: VirtualKey_UpdateVolumeActive @ 6bb34799
// ============================================================================

void VirtualKey_UpdateVolumeActive(void)

{
  char cVar1;
  
                    /* 0xf4799  2593  VirtualKey_UpdateVolumeActive */
  DAT_6bca3d7e = (ulonglong)
                 ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr - g_timeStampVolumeAction) <
                 (ulonglong)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x65a;
  cVar1 = SideGesture_IsVolumeMapping();
  if (cVar1 != '\0') {
    cVar1 = SideTouch_IsVolumeGUIShown();
    if (cVar1 != '\0') {
      DAT_6bca3d7e = *_refptr_g_guiVolume;
      g_timeStampVolumeAction = 0;
    }
  }
  if (DAT_6bca3d7e == '\0') {
    prevNumTrulyReprotedTouchInCore_10753 = (*_refptr_g_tsaStaticPtr)->field_0x1;
  }
  else if (prevNumTrulyReprotedTouchInCore_10753 < (byte)(*_refptr_g_tsaStaticPtr)->field_0x1) {
    g_timeStampVolumeAction = 0;
  }
  return;
}


// ============================================================================
// Function: VirtualKey_GetEventNum @ 6bb33dee
// ============================================================================

undefined1 VirtualKey_GetEventNum(void)

{
                    /* 0xf3dee  2582  VirtualKey_GetEventNum */
  return g_outVirtualKey;
}


// ============================================================================
// Function: VirtualKey_GetCode @ 6bb33dfe
// ============================================================================

undefined2 VirtualKey_GetCode(byte param_1)

{
  undefined2 uVar1;
  
                    /* 0xf3dfe  2581  VirtualKey_GetCode */
  if (param_1 < 10) {
    uVar1 = *(undefined2 *)(&DAT_6bca3cc8 + (longlong)(int)(uint)param_1 * 0x10);
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: VirtualKey_GetAction @ 6bb33e2f
// ============================================================================

ulonglong VirtualKey_GetAction(byte param_1)

{
  ulonglong uVar1;
  
                    /* 0xf3e2f  2579  VirtualKey_GetAction */
  if (param_1 < 10) {
    uVar1 = (ulonglong)(byte)(&DAT_6bca3cca)[(longlong)(int)(uint)param_1 * 0x10];
  }
  else {
    uVar1 = 0xffffffff;
  }
  return uVar1;
}


// ============================================================================
// Function: VirtualKey_Set @ 6bb33e60
// ============================================================================

undefined4 VirtualKey_Set(short param_1,char param_2)

{
  byte local_d;
  undefined4 local_c;
  
                    /* 0xf3e60  2588  VirtualKey_Set */
  local_c = 1;
  for (local_d = 0; local_d < g_dataVirtualKey; local_d = local_d + 1) {
    if ((((&DAT_6bca3b48)[(longlong)(int)(uint)local_d * 8] == param_1) &&
        ((&DAT_6bca3b4a)[(longlong)(int)(uint)local_d * 0x10] != param_2)) &&
       ((ulonglong)(ushort)(&DAT_6bca3b4c)[(longlong)(int)(uint)local_d * 8] <=
        (ulonglong)
        ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
        (&DAT_6bca3b50)[(longlong)(int)(uint)local_d * 2]))) {
      (&DAT_6bca3b4a)[(longlong)(int)(uint)local_d * 0x10] = param_2;
      (&DAT_6bca3b50)[(longlong)(int)(uint)local_d * 2] = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr
      ;
      local_c = 0;
    }
  }
  return local_c;
}


// ============================================================================
// Function: VirtualKey_VolumnUp @ 6bb33fdf
// ============================================================================

void VirtualKey_VolumnUp(void)

{
  char cVar1;
  
                    /* 0xf3fdf  2603  VirtualKey_VolumnUp */
  if (DAT_6bca3b4a == '\0') {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) & 0xffffefff;
    cVar1 = SideTouch_IsVolumeControlEnabled();
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(4,"Ring-VirtualKey[+Off]");
    }
  }
  else {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) | 0x1000;
    cVar1 = SideTouch_IsVolumeControlEnabled();
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(4,"Ring-VirtualKey[+On]");
    }
  }
  return;
}


// ============================================================================
// Function: VirtualKey_VolumnDown @ 6bb3408f
// ============================================================================

void VirtualKey_VolumnDown(void)

{
  char cVar1;
  
                    /* 0xf408f  2601  VirtualKey_VolumnDown */
  if (DAT_6bca3b5a == '\0') {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) & 0xffffdfff;
    cVar1 = SideTouch_IsVolumeControlEnabled();
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(4,"Ring-VirtualKey[-Off]");
    }
  }
  else {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) | 0x2000;
    cVar1 = SideTouch_IsVolumeControlEnabled();
    if (cVar1 != '\0') {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(4,"Ring-VirtualKey[-On]");
    }
  }
  return;
}


// ============================================================================
// Function: VirtualKey_VolumnMute @ 6bb3413f
// ============================================================================

void VirtualKey_VolumnMute(void)

{
                    /* 0xf413f  2602  VirtualKey_VolumnMute */
  if (DAT_6bca3b7a == '\0') {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) & 0xffffbfff;
  }
  else {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) | 0x4000;
  }
  return;
}


// ============================================================================
// Function: VirtualKey_Power @ 6bb34190
// ============================================================================

void VirtualKey_Power(void)

{
                    /* 0xf4190  2586  VirtualKey_Power */
  if (DAT_6bca3b6a == '\0') {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) & 0xffff7fff;
  }
  else {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) | 0x8000;
  }
  return;
}


// ============================================================================
// Function: VirtualKey_TPSlideVolumeProcess @ 6bb34c72
// ============================================================================

void VirtualKey_TPSlideVolumeProcess(byte param_1)

{
  char cVar1;
  undefined2 uVar2;
  undefined2 uVar3;
  byte *pbVar4;
  
                    /* 0xf4c72  2591  VirtualKey_TPSlideVolumeProcess */
  cVar1 = SideTouch_IsGestureActive(param_1);
  if (cVar1 != '\0') {
    *(void **)(_refptr_g_sideGesture + 0x4290) = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
  }
  cVar1 = SideTouch_IsGestureJustActive(param_1);
  if (cVar1 != '\0') {
    cVar1 = SideGesture_IsVolumeTrigger(param_1);
    if (cVar1 != '\0') {
      *(void **)(_refptr_g_sideGesture + 0x4298) = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
      cVar1 = SideGesture_IsVolumeDoubleClickTrigger(param_1);
      if (cVar1 != '\0') {
        pbVar4 = _refptr_g_sideGesture + (longlong)(int)(uint)param_1 * 0x2a8 + 0x58;
        *(undefined2 *)(_refptr_g_sideGesture + 0x4308) =
             *(undefined2 *)(pbVar4 + (longlong)(int)(*pbVar4 - 2) * 0x30 + 0x1a);
        *(undefined2 *)(_refptr_g_sideGesture + 0x430a) =
             *(undefined2 *)(pbVar4 + (longlong)(int)(*pbVar4 - 2) * 0x30 + 0x1a);
      }
    }
  }
  uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x640);
  uVar3 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x642);
  VirtualKey_VolumeProcess(param_1,uVar2,uVar3);
  return;
}


// ============================================================================
// Function: VirtualKey_GetVolumePos @ 6bb34df1
// ============================================================================

undefined8 VirtualKey_GetVolumePos(byte param_1)

{
  undefined8 uVar1;
  
                    /* 0xf4df1  2584  VirtualKey_GetVolumePos */
  if (param_1 < 0x14) {
    uVar1 = TSAOut_DisplayYPos(param_1);
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: VirtualKey_GetVolumeKey @ 6bb34e1c
// ============================================================================

undefined4 VirtualKey_GetVolumeKey(byte param_1)

{
  ushort uVar1;
  ushort uVar2;
  int iVar3;
  short local_22;
  char local_20;
  char local_1e;
  undefined4 local_1c;
  
                    /* 0xf4e1c  2583  VirtualKey_GetVolumeKey */
  local_1c = 0;
  iVar3 = SideTouch_GetVirtualVolumeKey();
  local_1e = (char)*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x65c;
  local_20 = (char)*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x65e;
  local_22 = *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x662;
  if (iVar3 == 0x73) {
    local_1e = local_1e - (char)*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x664;
    local_22 = local_22 + *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x664;
  }
  else if (iVar3 == 0x72) {
    local_20 = local_20 + (char)*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x664;
    local_22 = local_22 - *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x664;
  }
  if (param_1 < 0x14) {
    uVar1 = *(ushort *)
             ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
    uVar2 = TS_MMToPixel(local_1e);
    if (uVar1 < uVar2) {
      local_1c = 0;
    }
    else {
      uVar1 = *(ushort *)
               ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
      uVar2 = TS_MMToPixel(local_20);
      if (uVar2 < uVar1) {
        local_1c = 0;
      }
      else {
        uVar1 = *(ushort *)
                 ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172);
        uVar2 = TS_MMToPixel(local_22 - *(short *)&_refptr_g_tsaPrmtFlash[2].field_0x660 & 0xff);
        if (uVar2 < uVar1) {
          uVar1 = *(ushort *)
                   ((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x172
                   );
          uVar2 = TS_MMToPixel(*(short *)&_refptr_g_tsaPrmtFlash[2].field_0x660 + local_22 & 0xff);
          if (uVar1 < uVar2) {
            local_1c = 0xff;
          }
          else {
            local_1c = 0x72;
          }
        }
        else {
          local_1c = 0x73;
        }
      }
    }
  }
  return local_1c;
}


// ============================================================================
// Function: VirtualKey_SetActualVolumeByMapping @ 6bb346f5
// ============================================================================

void VirtualKey_SetActualVolumeByMapping(short param_1)

{
  short local_res8;
  
                    /* 0xf46f5  2590  VirtualKey_SetActualVolumeByMapping */
  if ((int)(uint)*(ushort *)(_refptr_g_guiVolume + 0x16) < (int)param_1) {
    local_res8 = *(short *)(_refptr_g_guiVolume + 0x16);
  }
  else {
    local_res8 = param_1;
    if ((int)param_1 < (int)(uint)*(ushort *)(_refptr_g_guiVolume + 0x14)) {
      local_res8 = *(short *)(_refptr_g_guiVolume + 0x14);
    }
  }
  DAT_6bca3d74 = local_res8;
  return;
}


// ============================================================================
// Function: VirtualKey_GetActualVolumeByMapping @ 6bb3475c
// ============================================================================

undefined2 VirtualKey_GetActualVolumeByMapping(void)

{
                    /* 0xf475c  2580  VirtualKey_GetActualVolumeByMapping */
  return DAT_6bca3d74;
}


// ============================================================================
// Function: VirtualKey_VolumeProcess @ 6bb34ace
// ============================================================================

void VirtualKey_VolumeProcess(ulonglong param_1,ushort param_2,ushort param_3)

{
  short sVar1;
  int iVar2;
  byte bVar3;
  undefined *puVar4;
  uint local_c;
  
                    /* 0xf4ace  2599  VirtualKey_VolumeProcess */
  bVar3 = (byte)param_1;
  local_c = 0;
  sVar1 = *(short *)(_refptr_g_sideGesture + (longlong)(int)(uint)bVar3 * 0x2a8 + 0x25c);
  iVar2 = *(int *)(_refptr_g_sideGesture + (longlong)(int)(uint)bVar3 * 0x2a8 + 0x244);
  puVar4 = _refptr_g_sideGesture;
  if ((((byte)_refptr_g_sideGesture[(longlong)(int)(uint)bVar3 * 0x2a8 + 0x259] < 0x14) &&
      (param_2 != 0)) && (param_3 != 0)) {
    if (((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6a0 & 0x40) == 0) ||
       (*_refptr_g_guiVolume == '\0')) {
      if (sVar1 < 1) {
        param_1 = (ulonglong)param_2;
        local_c = ((int)DAT_6bca3d78 * -(int)sVar1) / (int)(uint)param_2;
        puVar4 = (undefined *)
                 ((longlong)((int)DAT_6bca3d78 * -(int)sVar1) % (longlong)(int)(uint)param_2 &
                 0xffffffff);
      }
      else {
        param_1 = (ulonglong)param_3;
        local_c = ((int)DAT_6bca3d78 * -(int)sVar1) / (int)(uint)param_3;
        puVar4 = (undefined *)
                 ((longlong)((int)DAT_6bca3d78 * -(int)sVar1) % (longlong)(int)(uint)param_3 &
                 0xffffffff);
      }
    }
    else {
      local_c = (int)DAT_6bca3d74 - (int)DAT_6bca3d76;
      puVar4 = (undefined *)(ulonglong)local_c;
    }
  }
  if (iVar2 != 2) {
    if (iVar2 == 3) {
      g_volumeData = DAT_6bca3d72;
      DAT_6bca3d76 = DAT_6bca3d72;
      return;
    }
    if (iVar2 != 1) {
      return;
    }
    if (*_refptr_g_guiVolume != '\0') {
      SideTouch_GestureVolumeMapToGUIRelatively(param_1,puVar4);
    }
  }
  VirtualKey_VolumeStateOn(bVar3,local_c);
  return;
}


// ============================================================================
// Function: VirtualKey_VolumeStateOn @ 6bb3485f
// ============================================================================

void VirtualKey_VolumeStateOn(undefined1 param_1,int param_2)

{
  short sVar1;
  char cVar2;
  int iVar3;
  
                    /* 0xf485f  2600  VirtualKey_VolumeStateOn */
  cVar2 = SideTouch_IsInVolumeTriggerStage();
  sVar1 = (short)param_2;
  if (cVar2 != '\0') {
    if ((DAT_6bca3d7e == '\0') && ((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6a0 & 0x2000) != 0))
    {
      if (0 < param_2) {
        cVar2 = SideGesture_IsVolumeDoubleClickTrigger(param_1);
        if (cVar2 == '\0') {
          DAT_6bca3d76 = (DAT_6bca3d76 + DAT_6bca3d78) - sVar1;
          goto LAB_6bb34969;
        }
      }
      DAT_6bca3d76 = (DAT_6bca3d76 - DAT_6bca3d78) - sVar1;
    }
    else {
      DAT_6bca3d76 = DAT_6bca3d72 - sVar1;
      if ((*(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6a0 & 0x2000) == 0) {
        g_timeStampVolumeAction = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
      }
    }
  }
LAB_6bb34969:
  g_volumeData = sVar1 + DAT_6bca3d76;
  if (DAT_6bca3d72 < g_volumeData) {
    if ((int)DAT_6bca3d78 <= (int)g_volumeData - (int)DAT_6bca3d72) {
      iVar3 = VirtualKey_Set(0x73,1);
      if (iVar3 == 0) {
        DAT_6bca3d72 = DAT_6bca3d78 + DAT_6bca3d72;
        g_timeStampVolumeAction = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
      }
    }
  }
  else if ((g_volumeData < DAT_6bca3d72) &&
          ((int)DAT_6bca3d78 <= (int)DAT_6bca3d72 - (int)g_volumeData)) {
    iVar3 = VirtualKey_Set(0x72,1);
    if (iVar3 == 0) {
      DAT_6bca3d72 = DAT_6bca3d72 - DAT_6bca3d78;
      g_timeStampVolumeAction = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    }
  }
  return;
}


