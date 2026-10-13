// ============================================================================
// Function: SideGesture_GetNum @ 6bb0dc99
// ============================================================================

undefined1 SideGesture_GetNum(void)

{
                    /* 0xcdc99  1318  SideGesture_GetNum */
  return g_sideGesture;
}


// ============================================================================
// Function: SideGesture_GetEnabledTypes @ 6bb0dca9
// ============================================================================

undefined4 SideGesture_GetEnabledTypes(void)

{
                    /* 0xcdca9  1316  SideGesture_GetEnabledTypes */
  return DAT_6bc9d7f0;
}


// ============================================================================
// Function: SideGesture_IsVolumeMapping @ 6bb0e377
// ============================================================================

undefined4 SideGesture_IsVolumeMapping(void)

{
  uint uVar1;
  
                    /* 0xce377  1328  SideGesture_IsVolumeMapping */
  uVar1 = *(uint *)&_refptr_g_tsaPrmtFlash[2].field_0x6a0 & 0x440;
  return CONCAT31((int3)(uVar1 >> 8),uVar1 != 0);
}


// ============================================================================
// Function: SideGesture_IsLeftVolumeUpTrigger @ 6bb0e606
// ============================================================================

bool SideGesture_IsLeftVolumeUpTrigger(byte param_1)

{
  return (*(uint *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_1 * 0x2a8) & 0x2000222) != 0;
}


// ============================================================================
// Function: SideGesture_IsRightVolumeUpTrigger @ 6bb0e686
// ============================================================================

bool SideGesture_IsRightVolumeUpTrigger(byte param_1)

{
  return (*(uint *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_1 * 0x2a8) & 0x8000888) != 0;
}


// ============================================================================
// Function: SideGesture_IsLeftVolumeTrigger @ 6bb0e706
// ============================================================================

bool SideGesture_IsLeftVolumeTrigger(byte param_1)

{
  return (*(uint *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_1 * 0x2a8) & 0x6200666) != 0;
}


// ============================================================================
// Function: SideGesture_IsRightVolumeTrigger @ 6bb0e746
// ============================================================================

bool SideGesture_IsRightVolumeTrigger(byte param_1)

{
  return (*(uint *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_1 * 0x2a8) & 0x18481998) != 0;
}


// ============================================================================
// Function: SideGesture_IsVolumeFineTune @ 6bb0eab7
// ============================================================================

bool SideGesture_IsVolumeFineTune(byte param_1)

{
  return (*(uint *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_1 * 0x2a8) & 0x6000) != 0;
}


// ============================================================================
// Function: SideGesture_IsVolumeTrigger @ 6bb0ea7a
// ============================================================================

undefined8 SideGesture_IsVolumeTrigger(undefined1 param_1)

{
  char cVar1;
  
                    /* 0xcea7a  1329  SideGesture_IsVolumeTrigger */
  cVar1 = SideGesture_IsLeftVolumeTrigger(param_1);
  if ((cVar1 == '\0') && (cVar1 = SideGesture_IsRightVolumeTrigger(param_1), cVar1 == '\0')) {
    return 0;
  }
  return 1;
}


// ============================================================================
// Function: SideGesture_GetVolumeGreenArea @ 6bb0e394
// ============================================================================

void SideGesture_GetVolumeGreenArea(undefined2 *param_1,undefined2 *param_2)

{
  char cVar1;
  undefined2 uVar2;
  
  cVar1 = SideGesture_IsVolumeMapping();
  if (cVar1 == '\0') {
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x644);
    *param_1 = uVar2;
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x646);
    *param_2 = uVar2;
  }
  else {
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x648);
    *param_1 = uVar2;
    uVar2 = TSAOut_ExtToIntDist(*(undefined2 *)&_refptr_g_tsaPrmtFlash[2].field_0x64a);
    *param_2 = uVar2;
  }
  return;
}


// ============================================================================
// Function: SideGesture_IsVolumeDoubleFlickTrigger @ 6bb0e786
// ============================================================================

bool SideGesture_IsVolumeDoubleFlickTrigger(byte param_1)

{
  return (*(uint *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_1 * 0x2a8) & 0x1e00) != 0;
}


// ============================================================================
// Function: SideGesture_IsVolumeMute @ 6bb0e7c6
// ============================================================================

bool SideGesture_IsVolumeMute(byte param_1)

{
  return (*(uint *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_1 * 0x2a8) & 0x60060000) != 0;
}


// ============================================================================
// Function: SideGesture_IsVolumeDoubleClickTrigger @ 6bb0e846
// ============================================================================

bool SideGesture_IsVolumeDoubleClickTrigger(byte param_1)

{
                    /* 0xce846  1327  SideGesture_IsVolumeDoubleClickTrigger */
  return (*(uint *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_1 * 0x2a8) & 0x600000) != 0;
}


// ============================================================================
// Function: SideGesture_IsTPSlideVolume @ 6bb0eaf7
// ============================================================================

undefined8 SideGesture_IsTPSlideVolume(undefined1 param_1)

{
  char cVar1;
  
  cVar1 = SideGesture_IsVolumeFineTune(param_1);
  if ((cVar1 == '\0') && (cVar1 = SideGesture_IsVolumeTrigger(param_1), cVar1 == '\0')) {
    return 0;
  }
  return 1;
}


// ============================================================================
// Function: SideGesture_GetFailureString @ 6bb195fc
// ============================================================================

char * SideGesture_GetFailureString(undefined4 param_1)

{
  char *pcVar1;
  
                    /* 0xd95fc  1317  SideGesture_GetFailureString */
  switch(param_1) {
  default:
    pcVar1 = "NONE";
    break;
  case 1:
    pcVar1 = "SIDETOUCH_ACTION_FAILED_MIN_TIME";
    break;
  case 2:
    pcVar1 = "SIDETOUCH_ACTION_FAILED_MAX_TIME";
    break;
  case 3:
    pcVar1 = "SIDETOUCH_ACTION_FAILED_MIN_MOVE";
    break;
  case 4:
    pcVar1 = "SIDETOUCH_ACTION_FAILED_MAX_TIME_TO_MOVE";
    break;
  case 5:
    pcVar1 = "SIDETOUCH_ACTION_FAILED_MAX_MOVE";
    break;
  case 6:
    pcVar1 = "SIDETOUCH_ACTION_FAILED_MIN_TOUCH";
    break;
  case 7:
    pcVar1 = "SIDETOUCH_ACTION_FAILED_MAX_TOUCH";
    break;
  case 8:
    pcVar1 = "SIDETOUCH_ACTION_FAILED_MOVE_SPEED";
    break;
  case 9:
    pcVar1 = "SIDETOUCH_ACTION_FAILED_PART_OF_CORE";
    break;
  case 10:
    pcVar1 = "SIDETOUCH_ACTION_FAILED_TOUCH_DOWN_OUTOF_AREA";
    break;
  case 0xb:
    pcVar1 = "SIDETOUCH_ACTION_FAILED_IS_SEPARATED_TOUCH";
    break;
  case 0xc:
    pcVar1 = "SIDETOUCH_ACTION_FAILED_IS_EXCLUDED_BY_OFF_DETECTION";
    break;
  case 0xd:
    pcVar1 = "SIDETOUCH_ACTION_FAILED_IS_EXCLUDED_BY_GLOBAL_ANTI_TRIGGER";
    break;
  case 0xe:
    pcVar1 = "SIDETOUCH_ACTION_FAILED_IS_EXCLUDED_BY_LOCAL_ANTI_TRIGGER";
    break;
  case 0xf:
    pcVar1 = "SIDETOUCH_ACTION_FAILED_IS_EXCLUDED_DUE_TO_UNSTABLE_SIGNAL";
  }
  return pcVar1;
}


// ============================================================================
// Function: SideGesture_CallbackIsToDetect @ 6bb19438
// ============================================================================

bool SideGesture_CallbackIsToDetect(byte param_1)

{
  uint uVar1;
  char cVar2;
  undefined1 uVar3;
  
                    /* 0xd9438  1311  SideGesture_CallbackIsToDetect */
  cVar2 = SideTouch_GestureEnabledRunTime
                    (*(undefined4 *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_1 * 0x2a8));
  if (cVar2 == '\0') {
    return false;
  }
  uVar1 = *(uint *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_1 * 0x2a8);
  if (uVar1 == 0x800) goto LAB_6bb195ac;
  if (uVar1 < 0x801) {
    if (uVar1 != 0x20) {
      if (uVar1 < 0x21) {
        if (uVar1 != 4) {
          if (4 < uVar1) {
            if ((uVar1 != 8) && (uVar1 != 0x10)) {
              return true;
            }
            goto LAB_6bb195ac;
          }
          if (uVar1 != 2) {
            return true;
          }
        }
      }
      else {
        if (uVar1 == 0x100) goto LAB_6bb195ac;
        if (uVar1 < 0x101) {
          if (uVar1 != 0x40) {
            if (uVar1 != 0x80) {
              return true;
            }
            goto LAB_6bb195ac;
          }
        }
        else if ((uVar1 != 0x200) && (uVar1 != 0x400)) {
          return true;
        }
      }
    }
  }
  else if (uVar1 != 0x200000) {
    if (uVar1 < 0x200001) {
      if (uVar1 == 0x2000) {
        uVar3 = SideGesture_CallbackIsToDetectMoveOffLeft(param_1);
        return (bool)uVar3;
      }
      if (uVar1 < 0x2001) {
        if (uVar1 != 0x1000) {
          return true;
        }
      }
      else {
        if (uVar1 == 0x4000) {
          uVar3 = SideGesture_CallbackIsToDetectMoveOffRight(param_1);
          return (bool)uVar3;
        }
        if (uVar1 != 0x80000) {
          return true;
        }
      }
    }
    else {
      if (uVar1 == 0x4000000) goto LAB_6bb19595;
      if (uVar1 < 0x4000001) {
        if (uVar1 != 0x400000) {
          if (uVar1 != 0x2000000) {
            return true;
          }
          goto LAB_6bb19595;
        }
      }
      else if ((uVar1 != 0x8000000) && (uVar1 != 0x10000000)) {
        return true;
      }
    }
LAB_6bb195ac:
    cVar2 = SideGesture_CallbackIsToDetectRightGesture(param_1);
    if (cVar2 != '\0') {
      return true;
    }
    return false;
  }
LAB_6bb19595:
  cVar2 = SideGesture_CallbackIsToDetectLeftGesture(param_1);
  return cVar2 != '\0';
}


// ============================================================================
// Function: SideGesture_CallbackIsToProcess @ 6bb195ec
// ============================================================================

undefined8 SideGesture_CallbackIsToProcess(void)

{
                    /* 0xd95ec  1312  SideGesture_CallbackIsToProcess */
  return 1;
}


// ============================================================================
// Function: SideGesture_CallbackProcess @ 6bb19d37
// ============================================================================

void SideGesture_CallbackProcess(byte param_1)

{
  char cVar1;
  
                    /* 0xd9d37  1314  SideGesture_CallbackProcess */
  cVar1 = SideGesture_IsVolumeForce(param_1);
  if (cVar1 != '\0') {
    DAT_6bc9d7fc = VirtualKey_GetVolumeKey((&DAT_6bc997d9)[(longlong)(int)(uint)param_1 * 0x2a8]);
    DAT_6bc9d800 = VirtualKey_GetVolumePos((&DAT_6bc997d9)[(longlong)(int)(uint)param_1 * 0x2a8]);
  }
  if (((*(uint *)(&DAT_6bc995c8 + (longlong)(int)(uint)param_1 * 0x2a8) & 2) != 0) &&
     (*(int *)(&DAT_6bc997c4 + (longlong)(int)(uint)param_1 * 0x2a8) == 1)) {
    VirtualKey_Set(*(uint *)(&DAT_6bc995cc + (longlong)(int)(uint)param_1 * 0x2a8) & 0xffff,1);
  }
  cVar1 = SideGesture_IsVolumeTrigger(param_1);
  if (cVar1 != '\0') {
    SideTouch_GestureLogUpdate(param_1);
  }
  cVar1 = SideGesture_IsTPSlideVolume(param_1);
  if (cVar1 != '\0') {
    VirtualKey_TPSlideVolumeProcess(param_1);
  }
  if ((*(uint *)(&DAT_6bc995c8 + (longlong)(int)(uint)param_1 * 0x2a8) & 0x10) != 0) {
    cVar1 = SideTouch_IsGestureActive(param_1);
    if (cVar1 != '\0') {
      *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
           *(uint *)(&DAT_6bc995cc + (longlong)(int)(uint)param_1 * 0x2a8) |
           *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18);
    }
  }
  if (((*(uint *)(&DAT_6bc995dc + (longlong)(int)(uint)param_1 * 0x2a8) & 1) != 0) &&
     (*(int *)(&DAT_6bc997c4 + (longlong)(int)(uint)param_1 * 0x2a8) == 2)) {
    SideTouch_GestureReset(param_1);
    *(undefined4 *)(&DAT_6bc997c4 + (longlong)(int)(uint)param_1 * 0x2a8) = 3;
  }
  cVar1 = VirtualKey_VolumeIsGUIActive();
  if (cVar1 == '\0') {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) | 0x100000;
  }
  else {
    *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
         *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) & 0xffefffff;
  }
  SideGesture_VolumeIndicatorBarProcess(param_1);
  SideTouch_GestureAntiTriggerLocally(param_1);
  cVar1 = SideTouch_IsGestureActive(param_1);
  if (((cVar1 == '\0') &&
      ((uint)(byte)(&DAT_6bc997c0)[(longlong)(int)(uint)param_1 * 0x2a8] ==
       (byte)(&DAT_6bc997c1)[(longlong)(int)(uint)param_1 * 0x2a8] + 1)) &&
     (*(int *)(&DAT_6bc995e0 +
              (longlong)(int)(uint)(byte)(&DAT_6bc997c0)[(longlong)(int)(uint)param_1 * 0x2a8] *
              0x30 + (longlong)(int)(uint)param_1 * 0x2a8) == 0)) {
    SideTouch_GestureExcludeExistedTouch(param_1,0xc);
  }
  return;
}


// ============================================================================
// Function: SideGesture_CallbackIsToReport @ 6bb1a3a9
// ============================================================================

bool SideGesture_CallbackIsToReport(byte param_1)

{
  char cVar1;
  bool local_9;
  
                    /* 0xda3a9  1313  SideGesture_CallbackIsToReport */
  local_9 = false;
  if (((byte)(&DAT_6bc997d9)[(longlong)(int)(uint)param_1 * 0x2a8] < 0x14) &&
     ((*(uint *)(_refptr_g_touchesSide +
                (longlong)(int)(uint)(byte)(&DAT_6bc997d9)[(longlong)(int)(uint)param_1 * 0x2a8] *
                0x168 + 0x1f8) & 8) != 0)) {
    if ((*(uint *)(&DAT_6bc995c8 + (longlong)(int)(uint)param_1 * 0x2a8) & 4) == 0) {
      if (((*(uint *)(&DAT_6bc995c8 + (longlong)(int)(uint)param_1 * 0x2a8) & 8) != 0) &&
         (cVar1 = SideTouch_IsGestureActive(param_1), cVar1 != '\0')) {
        local_9 = true;
      }
    }
    else {
      cVar1 = SideTouch_IsGestureActive(param_1);
      local_9 = cVar1 != '\0';
      cVar1 = SideTouch_IsGestureActive(param_1);
      if ((((cVar1 != '\0') && (cVar1 = SideGesture_IsTPSlideVolume(param_1), cVar1 != '\0')) &&
          (*(longlong *)(_refptr_g_guiVolume + 0x20) != -1)) &&
         ((ulonglong)
          ((longlong)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
          *(longlong *)(_refptr_g_guiVolume + 0x20)) <
          (ulonglong)*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x66c)) {
        local_9 = false;
      }
    }
  }
  if (local_9 != false) {
    *(uint *)(_refptr_g_touchesSide +
             (longlong)(int)(uint)(byte)(&DAT_6bc997d9)[(longlong)(int)(uint)param_1 * 0x2a8] *
             0x168 + 500) =
         *(uint *)(_refptr_g_touchesSide +
                  (longlong)(int)(uint)(byte)(&DAT_6bc997d9)[(longlong)(int)(uint)param_1 * 0x2a8] *
                  0x168 + 500) | 8;
  }
  return local_9;
}


// ============================================================================
// Function: SideGesture_CallbackReport @ 6bb1a5af
// ============================================================================

void SideGesture_CallbackReport(byte param_1)

{
  byte bVar1;
  char cVar2;
  undefined2 uVar3;
  
                    /* 0xda5af  1315  SideGesture_CallbackReport */
  bVar1 = (&DAT_6bc997d9)[(longlong)(int)(uint)param_1 * 0x2a8];
  if ((*(uint *)(&DAT_6bc995c8 + (longlong)(int)(uint)param_1 * 0x2a8) & 4) == 0) {
    if ((*(uint *)(&DAT_6bc995c8 + (longlong)(int)(uint)param_1 * 0x2a8) & 8) != 0) {
      *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)bVar1 * 0x168 + 0x29a) =
           *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)bVar1 * 0x168 + 0x170);
      *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)bVar1 * 0x168 + 0x29c) =
           *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)bVar1 * 0x168 + 0x172);
    }
  }
  else {
    SideTouch_CoordinateMapping(bVar1,param_1);
    cVar2 = SideGesture_IsTPSlideVolume(param_1);
    if (cVar2 != '\0') {
      if (_refptr_g_guiVolume[1] == '\0') {
        VirtualKey_SetActualVolumeByMapping
                  ((int)*(short *)(_refptr_g_touchesSide +
                                  (longlong)(int)(uint)bVar1 * 0x168 + 0x29c));
        uVar3 = VirtualKey_GetActualVolumeByMapping();
        *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)bVar1 * 0x168 + 0x29c) = uVar3;
      }
      else {
        VirtualKey_SetActualVolumeByMapping
                  ((int)*(short *)(_refptr_g_touchesSide +
                                  (longlong)(int)(uint)bVar1 * 0x168 + 0x29a));
        uVar3 = VirtualKey_GetActualVolumeByMapping();
        *(undefined2 *)(_refptr_g_touchesSide + (longlong)(int)(uint)bVar1 * 0x168 + 0x29a) = uVar3;
      }
    }
  }
  return;
}


// ============================================================================
// Function: SideGesture_IsValidTouch @ 6bb0e439
// ============================================================================

undefined8 SideGesture_IsValidTouch(byte param_1)

{
  undefined8 uVar1;
  
  if (param_1 < 0x14) {
    if ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x21c)
         < 2) || ((*(uint *)((longlong)(int)(uint)param_1 * 0x168 +
                             *(longlong *)_refptr_g_touchesPtr + 0x1fc) & 0x400) == 0)) {
      uVar1 = 0;
    }
    else {
      uVar1 = 1;
    }
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}


// ============================================================================
// Function: SideGesture_IsInVolumeArea @ 6bb0ed71
// ============================================================================

undefined1 SideGesture_IsInVolumeArea(undefined1 param_1)

{
  char cVar1;
  byte local_a;
  
                    /* 0xced71  1324  SideGesture_IsInVolumeArea */
  local_a = 0;
  while( true ) {
    if (g_sideGesture <= local_a) {
      return 0;
    }
    cVar1 = SideGesture_IsTPSlideVolume(local_a);
    if ((cVar1 != '\0') && (cVar1 = SideTouch_IsTouchInRegion(param_1,local_a), cVar1 != '\0'))
    break;
    local_a = local_a + 1;
  }
  return 1;
}


// ============================================================================
// Function: SideGesture_CallbackIsToDetectLeftGesture @ 6bb191f0
// ============================================================================

undefined1 SideGesture_CallbackIsToDetectLeftGesture(undefined1 param_1)

{
  char cVar1;
  undefined1 local_9;
  
  local_9 = 1;
  cVar1 = SideGesture_IsLeftVolumeUpTrigger(param_1);
  if ((cVar1 != '\0') && ((DAT_6bc9d808 & 2) != 0)) {
    local_9 = 0;
  }
  cVar1 = SideGesture_IsLeftVolumeDownTrigger(param_1);
  if ((cVar1 != '\0') && ((DAT_6bc9d808 & 1) != 0)) {
    local_9 = 0;
  }
  if ((DAT_6bc9d808 & 0xc) != 0) {
    local_9 = 0;
  }
  if ((DAT_6bc9d808 & 0x10) != 0) {
    local_9 = 0;
  }
  cVar1 = VirtualKey_VolumeIsGUIActive();
  if ((cVar1 != '\0') && ((DAT_6bc9d808 & 0x40) != 0)) {
    local_9 = 0;
  }
  return local_9;
}


// ============================================================================
// Function: SideGesture_CallbackIsToDetectRightGesture @ 6bb192aa
// ============================================================================

undefined1 SideGesture_CallbackIsToDetectRightGesture(byte param_1)

{
  char cVar1;
  undefined1 local_9;
  
  local_9 = 1;
  cVar1 = SideGesture_IsRightVolumeUpTrigger(param_1);
  if ((cVar1 != '\0') && ((DAT_6bc9d808 & 8) != 0)) {
    local_9 = 0;
  }
  cVar1 = SideGesture_IsRightVolumeDownTrigger(param_1);
  if ((cVar1 != '\0') && ((DAT_6bc9d808 & 4) != 0)) {
    local_9 = 0;
  }
  if ((DAT_6bc9d808 & 3) != 0) {
    local_9 = 0;
  }
  if ((DAT_6bc9d808 & 0x10) != 0) {
    local_9 = 0;
  }
  cVar1 = VirtualKey_VolumeIsGUIActive();
  if ((cVar1 != '\0') && ((DAT_6bc9d808 & 0x20) != 0)) {
    local_9 = 0;
  }
  if (*(int *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_1 * 0x2a8) == 0x80000) {
    cVar1 = SideTouch_PressureIsVolumeDown();
    if (cVar1 == '\0') {
      local_9 = 0;
    }
  }
  return local_9;
}


// ============================================================================
// Function: SideGesture_CallbackIsToDetectMoveOffLeft @ 6bb19398
// ============================================================================

undefined1 SideGesture_CallbackIsToDetectMoveOffLeft(void)

{
  char cVar1;
  undefined1 local_9;
  
  local_9 = 0;
  cVar1 = VirtualKey_VolumeIsGUIActive();
  if (((cVar1 != '\0') && ((DAT_6bc9d808 & 3) == 0)) && ((DAT_6bc9d808 & 0x20) != 0)) {
    local_9 = 1;
  }
  return local_9;
}


// ============================================================================
// Function: SideGesture_CallbackIsToDetectMoveOffRight @ 6bb193e8
// ============================================================================

undefined1 SideGesture_CallbackIsToDetectMoveOffRight(void)

{
  char cVar1;
  undefined1 local_9;
  
  local_9 = 0;
  cVar1 = VirtualKey_VolumeIsGUIActive();
  if (((cVar1 != '\0') && ((DAT_6bc9d808 & 0xc) == 0)) && ((DAT_6bc9d808 & 0x40) != 0)) {
    local_9 = 1;
  }
  return local_9;
}


// ============================================================================
// Function: SideGesture_IsVolumeForce @ 6bb0eb34
// ============================================================================

bool SideGesture_IsVolumeForce(byte param_1)

{
  return (*(uint *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_1 * 0x2a8) & 0x10000) != 0;
}


// ============================================================================
// Function: SideGesture_VolumeIndicatorBarProcess @ 6bb198bb
// ============================================================================

void SideGesture_VolumeIndicatorBarProcess(byte param_1)

{
  char cVar1;
  char local_a;
  char local_9;
  
  local_9 = '\0';
  local_a = '\0';
  SideGesture_VolumeIndicatorSideFlagUpdate(param_1,&local_9,&local_a);
  cVar1 = SideGesture_IsLeftVolumeIndicator(param_1);
  if (cVar1 != '\0') {
    if (local_9 == '\0') {
      cVar1 = VirtualKey_VolumeIsGUIActive();
      if ((cVar1 == '\0') &&
         ((*(uint *)(&DAT_6bc997e4 + (longlong)(int)(uint)param_1 * 0x2a8) & 1) != 0)) {
        *(uint *)(&DAT_6bc997e4 + (longlong)(int)(uint)param_1 * 0x2a8) =
             *(uint *)(&DAT_6bc997e4 + (longlong)(int)(uint)param_1 * 0x2a8) & 0xfffffffe;
        *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
             *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) & 0xfffeffff;
        *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
             *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) | 0x40000;
      }
    }
    else {
      *(uint *)(&DAT_6bc997e4 + (longlong)(int)(uint)param_1 * 0x2a8) =
           *(uint *)(&DAT_6bc997e4 + (longlong)(int)(uint)param_1 * 0x2a8) | 1;
      *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
           *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) | 0x10000;
      *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
           *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) & 0xfffbffff;
    }
  }
  cVar1 = SideGesture_IsRightVolumeIndicator(param_1);
  if (cVar1 != '\0') {
    if (local_a == '\0') {
      cVar1 = VirtualKey_VolumeIsGUIActive();
      if ((cVar1 == '\0') &&
         ((*(uint *)(&DAT_6bc997e4 + (longlong)(int)(uint)param_1 * 0x2a8) & 1) != 0)) {
        *(uint *)(&DAT_6bc997e4 + (longlong)(int)(uint)param_1 * 0x2a8) =
             *(uint *)(&DAT_6bc997e4 + (longlong)(int)(uint)param_1 * 0x2a8) & 0xfffffffe;
        *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
             *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) & 0xfffdffff;
        *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
             *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) | 0x80000;
      }
    }
    else {
      *(uint *)(&DAT_6bc997e4 + (longlong)(int)(uint)param_1 * 0x2a8) =
           *(uint *)(&DAT_6bc997e4 + (longlong)(int)(uint)param_1 * 0x2a8) | 1;
      *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
           *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) | 0x20000;
      *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) =
           *(uint *)(*(longlong *)_refptr_g_touchesPtr + 0x18) & 0xfff7ffff;
    }
  }
  return;
}


// ============================================================================
// Function: SideGesture_IsLeftVolumeDownTrigger @ 6bb0e646
// ============================================================================

bool SideGesture_IsLeftVolumeDownTrigger(byte param_1)

{
  return (*(uint *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_1 * 0x2a8) & 0x4000444) != 0;
}


// ============================================================================
// Function: SideGesture_IsRightVolumeDownTrigger @ 6bb0e6c6
// ============================================================================

bool SideGesture_IsRightVolumeDownTrigger(byte param_1)

{
  return (*(uint *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_1 * 0x2a8) & 0x10001110) != 0;
}


// ============================================================================
// Function: SideGesture_IsLeftVolumeIndicator @ 6bb0ea00
// ============================================================================

undefined8 SideGesture_IsLeftVolumeIndicator(undefined1 param_1)

{
  char cVar1;
  
  cVar1 = SideGesture_IsLeftVolumeTouch(param_1);
  if ((cVar1 == '\0') &&
     (cVar1 = SideGesture_IsLeftVolumeDoubleActionTrigger(param_1), cVar1 == '\0')) {
    return 0;
  }
  return 1;
}


// ============================================================================
// Function: SideGesture_IsRightVolumeIndicator @ 6bb0ea3d
// ============================================================================

undefined8 SideGesture_IsRightVolumeIndicator(undefined1 param_1)

{
  char cVar1;
  
  cVar1 = SideGesture_IsRightVolumeTouch(param_1);
  if ((cVar1 == '\0') &&
     (cVar1 = SideGesture_IsRightVolumeDoubleActionTrigger(param_1), cVar1 == '\0')) {
    return 0;
  }
  return 1;
}


// ============================================================================
// Function: SideGesture_VolumeIndicatorSideFlagUpdate @ 6bb196c4
// ============================================================================

void SideGesture_VolumeIndicatorSideFlagUpdate(byte param_1,undefined1 *param_2,undefined1 *param_3)

{
  char cVar1;
  
  cVar1 = SideGesture_IsVolumeTouch(param_1);
  if (cVar1 == '\0') {
    cVar1 = SideGesture_IsVolumeDoubleClickTrigger(param_1);
    if (cVar1 == '\0') {
      cVar1 = SideGesture_IsVolumeDoubleActionTrigger(param_1);
      if (cVar1 != '\0') {
        cVar1 = SideGesture_IsLeftVolumeDoubleActionTrigger(param_1);
        if (cVar1 == '\0') {
          if ((&DAT_6bc997c0)[(longlong)(int)(uint)param_1 * 0x2a8] == '\0') {
            *param_3 = 0;
          }
          else {
            *param_3 = 1;
          }
        }
        else if ((&DAT_6bc997c0)[(longlong)(int)(uint)param_1 * 0x2a8] == '\0') {
          *param_2 = 0;
        }
        else {
          *param_2 = 1;
        }
      }
    }
    else {
      cVar1 = SideGesture_IsLeftVolumeDoubleClickTrigger(param_1);
      if (cVar1 == '\0') {
        cVar1 = SideTouch_IsGestureActive(param_1);
        if (cVar1 == '\0') {
          *param_3 = 0;
        }
        else {
          *param_3 = 1;
        }
      }
      else {
        cVar1 = SideTouch_IsGestureActive(param_1);
        if (cVar1 == '\0') {
          *param_2 = 0;
        }
        else {
          *param_2 = 1;
        }
      }
    }
  }
  else {
    cVar1 = SideGesture_IsLeftVolumeTouch(param_1);
    if (cVar1 == '\0') {
      if (((&DAT_6bc997c0)[(longlong)(int)(uint)param_1 * 0x2a8] == '\0') ||
         (2 < (byte)(&DAT_6bc997c0)[(longlong)(int)(uint)param_1 * 0x2a8])) {
        *param_3 = 0;
      }
      else {
        *param_3 = 1;
      }
    }
    else if (((&DAT_6bc997c0)[(longlong)(int)(uint)param_1 * 0x2a8] == '\0') ||
            (2 < (byte)(&DAT_6bc997c0)[(longlong)(int)(uint)param_1 * 0x2a8])) {
      *param_2 = 0;
    }
    else {
      *param_2 = 1;
    }
  }
  return;
}


// ============================================================================
// Function: SideGesture_IsSideOverCore @ 6bb0efca
// ============================================================================

undefined8 SideGesture_IsSideOverCore(undefined1 param_1)

{
  char cVar1;
  undefined8 uVar2;
  
                    /* 0xcefca  1325  SideGesture_IsSideOverCore */
  cVar1 = SideGesture_IsInCameraArea(param_1);
  if (cVar1 == '\0') {
    cVar1 = SideGesture_IsInGUIArea(param_1);
    if (cVar1 == '\0') {
      uVar2 = 0;
    }
    else {
      uVar2 = 1;
    }
  }
  else {
    uVar2 = 1;
  }
  return uVar2;
}


// ============================================================================
// Function: SideGesture_IsLeftVolumeDoubleActionTrigger @ 6bb0e8c6
// ============================================================================

bool SideGesture_IsLeftVolumeDoubleActionTrigger(byte param_1)

{
  return (*(uint *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_1 * 0x2a8) & 0x6200600) != 0;
}


// ============================================================================
// Function: SideGesture_IsLeftVolumeTouch @ 6bb0e943
// ============================================================================

bool SideGesture_IsLeftVolumeTouch(byte param_1)

{
  return (*(uint *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_1 * 0x2a8) & 0x800000) != 0;
}


// ============================================================================
// Function: SideGesture_IsRightVolumeDoubleActionTrigger @ 6bb0e886
// ============================================================================

bool SideGesture_IsRightVolumeDoubleActionTrigger(byte param_1)

{
  return (*(uint *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_1 * 0x2a8) & 0x18401800) != 0;
}


// ============================================================================
// Function: SideGesture_IsRightVolumeTouch @ 6bb0e983
// ============================================================================

bool SideGesture_IsRightVolumeTouch(byte param_1)

{
  return (*(uint *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_1 * 0x2a8) & 0x1000000) != 0;
}


// ============================================================================
// Function: SideGesture_IsLeftVolumeDoubleClickTrigger @ 6bb0e806
// ============================================================================

bool SideGesture_IsLeftVolumeDoubleClickTrigger(byte param_1)

{
  return (*(uint *)(&DAT_6bc997e0 + (longlong)(int)(uint)param_1 * 0x2a8) & 0x200000) != 0;
}


// ============================================================================
// Function: SideGesture_IsVolumeDoubleActionTrigger @ 6bb0e906
// ============================================================================

undefined8 SideGesture_IsVolumeDoubleActionTrigger(undefined1 param_1)

{
  char cVar1;
  
  cVar1 = SideGesture_IsRightVolumeDoubleActionTrigger(param_1);
  if ((cVar1 == '\0') &&
     (cVar1 = SideGesture_IsLeftVolumeDoubleActionTrigger(param_1), cVar1 == '\0')) {
    return 0;
  }
  return 1;
}


// ============================================================================
// Function: SideGesture_IsVolumeTouch @ 6bb0e9c3
// ============================================================================

undefined8 SideGesture_IsVolumeTouch(undefined1 param_1)

{
  char cVar1;
  
  cVar1 = SideGesture_IsLeftVolumeTouch(param_1);
  if ((cVar1 == '\0') && (cVar1 = SideGesture_IsRightVolumeTouch(param_1), cVar1 == '\0')) {
    return 0;
  }
  return 1;
}


// ============================================================================
// Function: SideGesture_IsInExternalArea @ 6bb0edd3
// ============================================================================

undefined1 SideGesture_IsInExternalArea(undefined1 param_1)

{
  char cVar1;
  byte local_a;
  
                    /* 0xcedd3  1322  SideGesture_IsInExternalArea */
  local_a = 0;
  while( true ) {
    if (g_sideGesture <= local_a) {
      return 0;
    }
    if ((*(int *)(&DAT_6bc997e0 + (longlong)(int)(uint)local_a * 0x2a8) == 0x100000) &&
       (cVar1 = SideTouch_IsTouchInRegion(param_1,local_a), cVar1 != '\0')) break;
    local_a = local_a + 1;
  }
  return 1;
}


// ============================================================================
// Function: SideGesture_IsInCameraArea @ 6bb0eee7
// ============================================================================

undefined1 SideGesture_IsInCameraArea(undefined1 param_1)

{
  char cVar1;
  byte local_a;
  
                    /* 0xceee7  1321  SideGesture_IsInCameraArea */
  local_a = 0;
  while( true ) {
    if (g_sideGesture <= local_a) {
      return 0;
    }
    if (((*(int *)(&DAT_6bc997e0 + (longlong)(int)(uint)local_a * 0x2a8) == 0x100000) &&
        (cVar1 = SideGesture_IsApp(local_a,"camera"), cVar1 != '\0')) &&
       (cVar1 = SideTouch_IsTouchInRegion(param_1,local_a), cVar1 != '\0')) break;
    local_a = local_a + 1;
  }
  return 1;
}


// ============================================================================
// Function: SideGesture_IsInGUIArea @ 6bb0ef77
// ============================================================================

undefined1 SideGesture_IsInGUIArea(undefined1 param_1)

{
  char cVar1;
  undefined1 local_9;
  
                    /* 0xcef77  1323  SideGesture_IsInGUIArea */
  local_9 = 0;
  cVar1 = VirtualKey_VolumeIsGUIActive();
  if (cVar1 != '\0') {
    cVar1 = SideGesture_IsActiveVolumeSliderTouch(param_1);
    if (cVar1 != '\0') {
      local_9 = 1;
    }
  }
  cVar1 = SideGesture_IsVolumeIndicatorOn();
  if (cVar1 != '\0') {
    cVar1 = SideGesture_IsTouchInVolumeIndicatorRegion(param_1);
    if (cVar1 != '\0') {
      local_9 = 1;
    }
  }
  return local_9;
}


// ============================================================================
// Function: SideGesture_TPSlideVolumeOffsetProcess @ 6bb18527
// ============================================================================

undefined4 SideGesture_TPSlideVolumeOffsetProcess(void)

{
  char cVar1;
  byte local_d;
  undefined4 local_c;
  
  local_c = 0;
  for (local_d = 0; local_d < g_sideGesture; local_d = local_d + 1) {
    cVar1 = SideGesture_IsTPSlideVolume(local_d);
    if (cVar1 != '\0') {
      cVar1 = SideTouch_IsGestureActive(local_d);
      if (cVar1 != '\0') {
        local_c = SideGesture_GetTPSlideVolumeOffset(local_d);
      }
    }
  }
  return local_c;
}


// ============================================================================
// Function: SideGesture_IsApp @ 6bb0ee4d
// ============================================================================

undefined8 SideGesture_IsApp(byte param_1,char *param_2)

{
  int iVar1;
  char *_Str;
  size_t sVar2;
  size_t sVar3;
  
                    /* 0xcee4d  1320  SideGesture_IsApp */
  _Str = (char *)((longlong)(int)(uint)param_1 * 0x2a8 + 0x6bc9958c);
  sVar2 = strlen(_Str);
  sVar3 = strlen(param_2);
  if (((ushort)sVar3 <= (ushort)sVar2) &&
     (iVar1 = strcmp(_Str + (int)((uint)(ushort)sVar2 - (uint)(ushort)sVar3),param_2), iVar1 == 0))
  {
    return 1;
  }
  return 0;
}


// ============================================================================
// Function: SideGesture_IsActiveVolumeSliderTouch @ 6bb0ec11
// ============================================================================

undefined1 SideGesture_IsActiveVolumeSliderTouch(byte param_1)

{
  char cVar1;
  byte local_a;
  
  local_a = 0;
  while( true ) {
    if (g_sideGesture <= local_a) {
      return 0;
    }
    cVar1 = SideGesture_IsTPSlideVolume(local_a);
    if (((cVar1 != '\0') && (cVar1 = SideTouch_IsGestureActive(local_a), cVar1 != '\0')) &&
       ((&DAT_6bc997da)[(longlong)(int)(uint)local_a * 0x2a8] ==
        *(char *)((longlong)(int)(uint)param_1 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x154))
       ) break;
    local_a = local_a + 1;
  }
  return 1;
}


// ============================================================================
// Function: SideGesture_IsTouchInVolumeIndicatorRegion @ 6bb0ecbe
// ============================================================================

undefined1 SideGesture_IsTouchInVolumeIndicatorRegion(undefined1 param_1)

{
  char cVar1;
  byte local_a;
  
  local_a = 0;
  while( true ) {
    if (g_sideGesture <= local_a) {
      return 0;
    }
    cVar1 = SideGesture_IsVolumeDoubleActionTrigger(local_a);
    if (((cVar1 != '\0') &&
        ((*(uint *)(&DAT_6bc997e4 + (longlong)(int)(uint)local_a * 0x2a8) & 1) != 0)) &&
       (cVar1 = SideTouch_IsTouchInRegion(param_1,local_a), cVar1 != '\0')) break;
    local_a = local_a + 1;
  }
  return 1;
}


// ============================================================================
// Function: SideGesture_IsVolumeIndicatorOn @ 6bb0ed49
// ============================================================================

bool SideGesture_IsVolumeIndicatorOn(void)

{
  return (*(uint *)&(*_refptr_g_prevTouchesPtr)->field_0x18 & 0x30000) != 0;
}


// ============================================================================
// Function: SideGesture_GetTPSlideVolumeOffset @ 6bb18470
// ============================================================================

int SideGesture_GetTPSlideVolumeOffset(byte param_1)

{
  short sVar1;
  int local_c;
  
  local_c = 0;
  if ((byte)(&DAT_6bc997d9)[(longlong)(int)(uint)param_1 * 0x2a8] < 0x14) {
    sVar1 = VirtualKey_GetActualVolumeByMapping();
    if (_refptr_g_guiVolume[2] == '\0') {
      local_c = (uint)*(ushort *)
                       (_refptr_g_touchesSide +
                       (longlong)
                       (int)(uint)(byte)(&DAT_6bc997d9)[(longlong)(int)(uint)param_1 * 0x2a8] *
                       0x168 + 0x172) - (int)sVar1;
    }
    else {
      local_c = (int)sVar1 +
                (uint)*(ushort *)
                       (_refptr_g_touchesSide +
                       (longlong)
                       (int)(uint)(byte)(&DAT_6bc997d9)[(longlong)(int)(uint)param_1 * 0x2a8] *
                       0x168 + 0x172);
    }
  }
  return local_c;
}


