// Table @ 0x6bb52a40 -> TouchMode_NoObjDetect @ 0x6ba790ae

void TouchMode_NoObjDetect(undefined4 *param_1,undefined4 *param_2)

{
  if ((param_1 != (undefined4 *)0x0) && (param_2 != (undefined4 *)0x0)) {
    *param_1 = 1;
    *param_2 = 2;
    if (**(char **)_refptr_g_touchesPtr == *(char *)(*(longlong *)_refptr_g_touchesPtr + 0xd)) {
      *param_1 = 2;
      *param_2 = 2;
    }
  }
  return;
}


// Table @ 0x6bb52a48 -> TouchMode_NoObjProcess @ 0x6ba79113

void TouchMode_NoObjProcess(void)

{
  return;
}


// Table @ 0x6bb52a50 -> TouchMode_FingerDetect @ 0x6ba7911f

void TouchMode_FingerDetect(undefined4 *param_1,undefined4 *param_2)

{
  byte local_9;
  
  if ((param_1 != (undefined4 *)0x0) && (param_2 != (undefined4 *)0x0)) {
    *param_1 = 1;
    *param_2 = 1;
    if (((_refptr_g_tsaPrmtDynamic[0x80] & 2) != 0) &&
       (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x1000) == 0)) {
      for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
        if (((*(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                       0x1f0) & 0x800) == 0) &&
           (*(short *)(_refptr_g_tsaPrmtDynamic + 0xc) <=
            *(short *)(*_refptr_g_peaksPtr +
                      (longlong)
                      (int)(uint)*(byte *)((longlong)(int)(uint)local_9 * 0x168 +
                                           *(longlong *)_refptr_g_touchesPtr + 0x23c) * 0x14 + 0x18)
           )) {
          *param_1 = 2;
          break;
        }
      }
      if (*(int *)&_refptr_g_tsaPrpt[1].field_0x2c != 1) {
        (*_refptr_g_tsaStaticPtr)[1].field_0xc = 0;
      }
      if ((*(int *)&_refptr_g_tsaPrpt[1].field_0x2c != 1) ||
         (*(int *)&_refptr_g_tsaPrpt[1].field_0x30 != 2)) {
        if (*(short *)&_refptr_g_tsaPrmtFlash->field_0x5d8 * 3 < *(int *)(*_refptr_g_peaksPtr + 8))
        {
          (*_refptr_g_tsaStaticPtr)[1].field_0xc = (*_refptr_g_tsaStaticPtr)[1].field_0xc + '\x01';
        }
        else {
          (*_refptr_g_tsaStaticPtr)[1].field_0xc = 0;
        }
      }
      if (2 < (byte)(*_refptr_g_tsaStaticPtr)[1].field_0xc) {
        *param_2 = 2;
        _refptr_g_tsaPrpt[1].field_0x28 = 0;
      }
    }
  }
  return;
}


// Table @ 0x6bb52a58 -> TouchMode_FingerProcess @ 0x6ba79318

void TouchMode_FingerProcess(undefined8 param_1,int param_2)

{
  byte local_9;
  
  if ((param_2 == 2) || (_refptr_g_tsaPrpt[1].field_0x28 == '\0')) {
    for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
      if (((*(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                     0x1f0) & 0x800) != 0) &&
         ((*_refptr_g_tsaStaticPtr)->bFullIncellSignalDisparity == '\0')) {
        *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208)
             = *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x208) | 2;
      }
    }
  }
  return;
}


// Table @ 0x6bb52a60 -> TouchMode_GloveDetect @ 0x6ba793fe

void TouchMode_GloveDetect(undefined4 *param_1,undefined4 *param_2)

{
  byte local_9;
  
  if ((param_1 != (undefined4 *)0x0) && (param_2 != (undefined4 *)0x0)) {
    *param_1 = 1;
    *param_2 = 1;
    if ((((_refptr_g_tsaPrmtDynamic[0x80] & 4) != 0) &&
        (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 1) != 0)) ||
       (((((_refptr_g_tsaPrmtDynamic[0x80] & 8) != 0 &&
          (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x10) != 0)) ||
         (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) != 0)) ||
        ((*_refptr_g_tsaStaticPtr)[1].field_0x3 != '\0')))) {
      if (**(char **)_refptr_g_touchesPtr != *(char *)(*(longlong *)_refptr_g_touchesPtr + 0xd)) {
        *param_1 = 2;
      }
      if (((_refptr_g_tsaPrpt[1].field_0x28 == '\0') ||
          ((((_refptr_g_tsaPrmtDynamic[0x80] & 4) == 0 ||
            (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 1) == 0)) &&
           (((_refptr_g_tsaPrmtDynamic[0x80] & 8) == 0 ||
            (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x10) == 0)))))) &&
         ((((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) == 0 &&
          ((*_refptr_g_tsaStaticPtr)[1].field_0x3 == '\0')))) {
        for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
          if (((((_refptr_g_tsaPrmtDynamic[0x80] & 4) != 0) &&
               (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 1) != 0)) &&
              ((*(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x1f8) & 4) != 0)) &&
             ((*(int *)&_refptr_g_tsaPrpt[1].field_0x2c != 1 &&
              ((*(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                         0x1f8) & 0x80000) == 0)))) {
            _refptr_g_tsaPrpt[1].field_0x28 = 1;
            *param_2 = 2;
            Touch_CreatePostReport(local_9,5);
            return;
          }
          if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x1000) != 0) {
            _refptr_g_tsaPrpt[1].field_0x28 = 1;
            *param_2 = 2;
            return;
          }
        }
      }
      else {
        _refptr_g_tsaPrpt[1].field_0x28 = 1;
        *param_2 = 2;
      }
    }
  }
  return;
}


// Table @ 0x6bb52a68 -> TouchMode_GloveProcess @ 0x6ba797a7

void TouchMode_GloveProcess(undefined8 param_1,int param_2)

{
  byte local_9;
  
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x1000) == 0) {
    if (((((_refptr_g_tsaPrmtDynamic[0x80] & 4) == 0) ||
         (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 1) == 0)) &&
        (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & FEAT_SMART_COVER) == 0)) &&
       ((*_refptr_g_tsaStaticPtr)[1].field_0x3 == '\0')) {
      TouchMode_StylusSuppression();
    }
    else if (param_2 == 1) {
      for (local_9 = 0; local_9 < **(byte **)_refptr_g_touchesPtr; local_9 = local_9 + 1) {
        *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr + 0x208)
             = *(uint *)((longlong)(int)(uint)local_9 * 0x168 + *(longlong *)_refptr_g_touchesPtr +
                        0x208) | 2;
      }
    }
    else {
      TouchMode_GloveFilter();
    }
  }
  return;
}


// Table @ 0x6bb52a70 -> TouchMode_StylusDetect @ 0x6ba798b8

void TouchMode_StylusDetect(undefined4 *param_1,undefined4 *param_2)

{
  if ((param_1 != (undefined4 *)0x0) && (param_2 != (undefined4 *)0x0)) {
    *param_1 = 1;
    *param_2 = 1;
  }
  return;
}


// Table @ 0x6bb52a78 -> TouchMode_StylusProcess @ 0x6ba7991a

void TouchMode_StylusProcess(void)

{
  return;
}


// Table @ 0x6bb60510 -> tls_callback_0 @ 0x6bb47db0

/* WARNING: Removing unreachable block (ram,0x6bb47df3) */
/* WARNING: Removing unreachable block (ram,0x6bb47dfb) */
/* WARNING: Removing unreachable block (ram,0x6bb47dfd) */
/* WARNING: Removing unreachable block (ram,0x6bb47e06) */

undefined8 tls_callback_0(undefined8 param_1,int param_2)

{
  if (*(int *)_refptr__CRT_MT != 2) {
    *(undefined4 *)_refptr__CRT_MT = 2;
  }
  if ((param_2 != 2) && (param_2 == 1)) {
    __mingw_TLScallback();
  }
  return 1;
}


// Table @ 0x6bb60cc0 -> RecordTrigger_PutStr @ 0x6bafcdd0

void RecordTrigger_PutStr(longlong param_1)

{
  ulonglong uVar1;
  ushort local_a;
  
                    /* 0xbcdd0  1165  RecordTrigger_PutStr */
  if (param_1 != 0) {
    local_a = 0;
    while ((*(char *)(param_1 + (ulonglong)local_a) != '\0' && (g_descriptionIndex < 499))) {
      uVar1 = (ulonglong)g_descriptionIndex;
      g_descriptionIndex = g_descriptionIndex + 1;
      (&g_description)[uVar1] = *(undefined1 *)(param_1 + (ulonglong)local_a);
      local_a = local_a + 1;
    }
    if (g_descriptionIndex < 500) {
      (&g_description)[g_descriptionIndex] = 0;
    }
  }
  return;
}


// Table @ 0x6bb60cd0 -> TSA_GetSensorData @ 0x6ba8f9be

longlong TSA_GetSensorData(void)

{
                    /* 0x4f9be  1986  TSA_GetSensorData */
  return (*_refptr_g_tsaStaticPtr)->llField_0x148;
}


// Table @ 0x6bb61cb0 -> memset @ 0x6bb4e540

void * __cdecl memset(void *_Dst,int _Val,size_t _Size)

{
  void *pvVar1;
  
                    /* WARNING: Could not recover jumptable at 0x6bb4e540. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  pvVar1 = memset(_Dst,_Val,_Size);
  return pvVar1;
}


