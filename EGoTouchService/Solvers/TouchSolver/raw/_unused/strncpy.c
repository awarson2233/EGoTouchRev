// ============================================================================
// Function: strncpy_s @ 6ba467f5
// ============================================================================

errno_t __cdecl strncpy_s(char *_Dst,rsize_t _SizeInBytes,char *_Src,rsize_t _MaxCount)

{
  errno_t eVar1;
  size_t local_38;
  rsize_t local_30;
  char *local_28;
  rsize_t local_20;
  rsize_t local_18;
  char *local_10;
  
                    /* 0x67f5  3010  strncpy_s */
  if (((((_SizeInBytes == 0) || (0x7fffffff < _SizeInBytes)) || (_Dst == (char *)0x0)) ||
      (_Src == (char *)0x0)) ||
     (((0x7fffffff < _MaxCount && (_MaxCount != 0xffffffffffffffff)) || (_MaxCount == 0)))) {
    eVar1 = strncpy_error(_Dst,_SizeInBytes,_Src,_MaxCount);
  }
  else {
    if (_MaxCount < _SizeInBytes) {
      local_18 = _MaxCount;
      for (local_10 = _Src; (local_18 != 0 && (*local_10 != '\0')); local_10 = local_10 + 1) {
        local_18 = local_18 - 1;
      }
      local_38 = (longlong)local_10 - (longlong)_Src;
    }
    else {
      local_20 = _SizeInBytes;
      if (_MaxCount == 0xffffffffffffffff) {
        local_20 = _SizeInBytes - 1;
      }
      local_30 = local_20;
      for (local_28 = _Src; (local_30 != 0 && (*local_28 != '\0')); local_28 = local_28 + 1) {
        local_30 = local_30 - 1;
      }
      local_38 = (longlong)local_28 - (longlong)_Src;
      if (local_38 == _SizeInBytes) {
        *_Dst = '\0';
        return 0xa2;
      }
    }
    if ((((_Src < _Dst) && (_Src + local_38 < _Dst)) || ((_Dst < _Src && (_Dst + local_38 < _Src))))
       || (_Dst == _Src)) {
      memcpy(_Dst,_Src,local_38);
      _Dst[local_38] = '\0';
      eVar1 = 0;
    }
    else {
      *_Dst = '\0';
      eVar1 = 0xb6;
    }
  }
  return eVar1;
}


// ============================================================================
// Function: strncpy_error @ 6ba4674a
// ============================================================================

undefined8 strncpy_error(undefined1 *param_1,ulonglong param_2,longlong param_3,ulonglong param_4)

{
  undefined8 uVar1;
  
                    /* 0x674a  3009  strncpy_error */
  if ((param_2 == 0) || (0x7fffffff < param_2)) {
    uVar1 = 0x22;
  }
  else if ((param_1 == (undefined1 *)0x0) || (param_3 == 0)) {
    if (param_1 == (undefined1 *)0x0) {
      uVar1 = 0x16;
    }
    else {
      *param_1 = 0;
      uVar1 = 0x96;
    }
  }
  else if (param_4 < 0x80000000) {
    if (param_4 == 0) {
      *param_1 = 0;
      uVar1 = 0;
    }
    else {
      uVar1 = CheckSrcCountRange(param_1,param_2,param_3,param_4);
    }
  }
  else {
    *param_1 = 0;
    uVar1 = 0xa2;
  }
  return uVar1;
}


