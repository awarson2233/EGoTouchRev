// ============================================================================
// Function: strcpy_s @ 6ba45d15
// ============================================================================

errno_t __cdecl strcpy_s(char *_Dst,rsize_t _SizeInBytes,char *_Src)

{
  undefined8 uVar1;
  errno_t eVar2;
  char *local_28;
  char *local_20;
  rsize_t local_18;
  char *local_10;
  
                    /* 0x5d15  3008  strcpy_s */
  if ((((_SizeInBytes != 0) && (_SizeInBytes < 0x80000000)) && (_Dst != (char *)0x0)) &&
     ((_Src != (char *)0x0 && (_Dst != _Src)))) {
    local_18 = _SizeInBytes;
    for (local_10 = _Src; (local_18 != 0 && (*local_10 != '\0')); local_10 = local_10 + 1) {
      local_18 = local_18 - 1;
    }
    local_10 = local_10 + (1 - (longlong)_Src);
    if (local_10 <= _SizeInBytes) {
      if (((_Dst <= _Src) || (_Dst < _Src + (longlong)local_10)) &&
         ((_Src <= _Dst || (_Src < _Dst + (longlong)local_10)))) {
        *_Dst = '\0';
        return 0xb6;
      }
      if (local_10 < (char *)0x21) {
        if ((((ulonglong)_Dst & 7) == 0) && (((ulonglong)_Src & 7) == 0)) {
          switch(local_10) {
          case (char *)0x1:
            *_Dst = *_Src;
            break;
          case (char *)0x2:
            *(undefined2 *)_Dst = *(undefined2 *)_Src;
            break;
          case (char *)0x3:
            *(undefined2 *)_Dst = *(undefined2 *)_Src;
            _Dst[2] = _Src[2];
            break;
          case (char *)0x4:
            *(undefined4 *)_Dst = *(undefined4 *)_Src;
            break;
          case (char *)0x5:
            *(undefined4 *)_Dst = *(undefined4 *)_Src;
            _Dst[4] = _Src[4];
            break;
          case (char *)0x6:
            *(undefined4 *)_Dst = *(undefined4 *)_Src;
            *(undefined2 *)(_Dst + 4) = *(undefined2 *)(_Src + 4);
            break;
          case (char *)0x7:
            *(undefined4 *)_Dst = *(undefined4 *)_Src;
            *(undefined2 *)(_Dst + 4) = *(undefined2 *)(_Src + 4);
            _Dst[6] = _Src[6];
            break;
          case (char *)0x8:
            *(undefined8 *)_Dst = *(undefined8 *)_Src;
            break;
          case (char *)0x9:
            *(undefined8 *)_Dst = *(undefined8 *)_Src;
            _Dst[8] = _Src[8];
            break;
          case (char *)0xa:
            *(undefined8 *)_Dst = *(undefined8 *)_Src;
            *(undefined2 *)(_Dst + 8) = *(undefined2 *)(_Src + 8);
            break;
          case (char *)0xb:
            *(undefined8 *)_Dst = *(undefined8 *)_Src;
            *(undefined2 *)(_Dst + 8) = *(undefined2 *)(_Src + 8);
            _Dst[10] = _Src[10];
            break;
          case (char *)0xc:
            *(undefined8 *)_Dst = *(undefined8 *)_Src;
            *(undefined4 *)(_Dst + 8) = *(undefined4 *)(_Src + 8);
            break;
          case (char *)0xd:
            *(undefined8 *)_Dst = *(undefined8 *)_Src;
            *(undefined4 *)(_Dst + 8) = *(undefined4 *)(_Src + 8);
            _Dst[0xc] = _Src[0xc];
            break;
          case (char *)0xe:
            *(undefined8 *)_Dst = *(undefined8 *)_Src;
            *(undefined4 *)(_Dst + 8) = *(undefined4 *)(_Src + 8);
            *(undefined2 *)(_Dst + 0xc) = *(undefined2 *)(_Src + 0xc);
            break;
          case (char *)0xf:
            *(undefined8 *)_Dst = *(undefined8 *)_Src;
            *(undefined4 *)(_Dst + 8) = *(undefined4 *)(_Src + 8);
            *(undefined2 *)(_Dst + 0xc) = *(undefined2 *)(_Src + 0xc);
            _Dst[0xe] = _Src[0xe];
            break;
          case (char *)0x10:
            uVar1 = *(undefined8 *)(_Src + 8);
            *(undefined8 *)_Dst = *(undefined8 *)_Src;
            *(undefined8 *)(_Dst + 8) = uVar1;
            break;
          case (char *)0x11:
            *(undefined8 *)_Dst = *(undefined8 *)_Src;
            *(undefined8 *)(_Dst + 8) = *(undefined8 *)(_Src + 8);
            _Dst[0x10] = _Src[0x10];
            break;
          case (char *)0x12:
            *(undefined8 *)_Dst = *(undefined8 *)_Src;
            *(undefined8 *)(_Dst + 8) = *(undefined8 *)(_Src + 8);
            *(undefined2 *)(_Dst + 0x10) = *(undefined2 *)(_Src + 0x10);
            break;
          case (char *)0x13:
            *(undefined8 *)_Dst = *(undefined8 *)_Src;
            *(undefined8 *)(_Dst + 8) = *(undefined8 *)(_Src + 8);
            *(undefined2 *)(_Dst + 0x10) = *(undefined2 *)(_Src + 0x10);
            _Dst[0x12] = _Src[0x12];
            break;
          case (char *)0x14:
            *(undefined8 *)_Dst = *(undefined8 *)_Src;
            *(undefined8 *)(_Dst + 8) = *(undefined8 *)(_Src + 8);
            *(undefined4 *)(_Dst + 0x10) = *(undefined4 *)(_Src + 0x10);
            break;
          case (char *)0x15:
            *(undefined8 *)_Dst = *(undefined8 *)_Src;
            *(undefined8 *)(_Dst + 8) = *(undefined8 *)(_Src + 8);
            *(undefined4 *)(_Dst + 0x10) = *(undefined4 *)(_Src + 0x10);
            _Dst[0x14] = _Src[0x14];
            break;
          case (char *)0x16:
            *(undefined8 *)_Dst = *(undefined8 *)_Src;
            *(undefined8 *)(_Dst + 8) = *(undefined8 *)(_Src + 8);
            *(undefined4 *)(_Dst + 0x10) = *(undefined4 *)(_Src + 0x10);
            *(undefined2 *)(_Dst + 0x14) = *(undefined2 *)(_Src + 0x14);
            break;
          case (char *)0x17:
            *(undefined8 *)_Dst = *(undefined8 *)_Src;
            *(undefined8 *)(_Dst + 8) = *(undefined8 *)(_Src + 8);
            *(undefined4 *)(_Dst + 0x10) = *(undefined4 *)(_Src + 0x10);
            *(undefined2 *)(_Dst + 0x14) = *(undefined2 *)(_Src + 0x14);
            _Dst[0x16] = _Src[0x16];
            break;
          case (char *)0x18:
            *(undefined8 *)_Dst = *(undefined8 *)_Src;
            *(undefined8 *)(_Dst + 8) = *(undefined8 *)(_Src + 8);
            *(undefined8 *)(_Dst + 0x10) = *(undefined8 *)(_Src + 0x10);
            break;
          case (char *)0x19:
            *(undefined8 *)_Dst = *(undefined8 *)_Src;
            *(undefined8 *)(_Dst + 8) = *(undefined8 *)(_Src + 8);
            *(undefined8 *)(_Dst + 0x10) = *(undefined8 *)(_Src + 0x10);
            _Dst[0x18] = _Src[0x18];
            break;
          case (char *)0x1a:
            *(undefined8 *)_Dst = *(undefined8 *)_Src;
            *(undefined8 *)(_Dst + 8) = *(undefined8 *)(_Src + 8);
            *(undefined8 *)(_Dst + 0x10) = *(undefined8 *)(_Src + 0x10);
            *(undefined2 *)(_Dst + 0x18) = *(undefined2 *)(_Src + 0x18);
            break;
          case (char *)0x1b:
            *(undefined8 *)_Dst = *(undefined8 *)_Src;
            *(undefined8 *)(_Dst + 8) = *(undefined8 *)(_Src + 8);
            *(undefined8 *)(_Dst + 0x10) = *(undefined8 *)(_Src + 0x10);
            *(undefined2 *)(_Dst + 0x18) = *(undefined2 *)(_Src + 0x18);
            _Dst[0x1a] = _Src[0x1a];
            break;
          case (char *)0x1c:
            *(undefined8 *)_Dst = *(undefined8 *)_Src;
            *(undefined8 *)(_Dst + 8) = *(undefined8 *)(_Src + 8);
            *(undefined8 *)(_Dst + 0x10) = *(undefined8 *)(_Src + 0x10);
            *(undefined4 *)(_Dst + 0x18) = *(undefined4 *)(_Src + 0x18);
            break;
          case (char *)0x1d:
            *(undefined8 *)_Dst = *(undefined8 *)_Src;
            *(undefined8 *)(_Dst + 8) = *(undefined8 *)(_Src + 8);
            *(undefined8 *)(_Dst + 0x10) = *(undefined8 *)(_Src + 0x10);
            *(undefined4 *)(_Dst + 0x18) = *(undefined4 *)(_Src + 0x18);
            _Dst[0x1c] = _Src[0x1c];
            break;
          case (char *)0x1e:
            *(undefined8 *)_Dst = *(undefined8 *)_Src;
            *(undefined8 *)(_Dst + 8) = *(undefined8 *)(_Src + 8);
            *(undefined8 *)(_Dst + 0x10) = *(undefined8 *)(_Src + 0x10);
            *(undefined4 *)(_Dst + 0x18) = *(undefined4 *)(_Src + 0x18);
            *(undefined2 *)(_Dst + 0x1c) = *(undefined2 *)(_Src + 0x1c);
            break;
          case (char *)0x1f:
            *(undefined8 *)_Dst = *(undefined8 *)_Src;
            *(undefined8 *)(_Dst + 8) = *(undefined8 *)(_Src + 8);
            *(undefined8 *)(_Dst + 0x10) = *(undefined8 *)(_Src + 0x10);
            *(undefined4 *)(_Dst + 0x18) = *(undefined4 *)(_Src + 0x18);
            *(undefined2 *)(_Dst + 0x1c) = *(undefined2 *)(_Src + 0x1c);
            _Dst[0x1e] = _Src[0x1e];
            break;
          case (char *)0x20:
            *(undefined8 *)_Dst = *(undefined8 *)_Src;
            *(undefined8 *)(_Dst + 8) = *(undefined8 *)(_Src + 8);
            *(undefined8 *)(_Dst + 0x10) = *(undefined8 *)(_Src + 0x10);
            *(undefined8 *)(_Dst + 0x18) = *(undefined8 *)(_Src + 0x18);
          }
        }
        else {
          local_28 = _Src;
          local_20 = _Dst;
          switch(local_10) {
          case (char *)0x20:
            local_20 = _Dst + 1;
            local_28 = _Src + 1;
            *_Dst = *_Src;
          case (char *)0x1f:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0x1e:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0x1d:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0x1c:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0x1b:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0x1a:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0x19:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0x18:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0x17:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0x16:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0x15:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0x14:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0x13:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0x12:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0x11:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0x10:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0xf:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0xe:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0xd:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0xc:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0xb:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0xa:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0x9:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0x8:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0x7:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0x6:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0x5:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0x4:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0x3:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0x2:
            *local_20 = *local_28;
            local_28 = local_28 + 1;
            local_20 = local_20 + 1;
          case (char *)0x1:
            *local_20 = *local_28;
          }
        }
      }
      else {
        memcpy(_Dst,_Src,(size_t)local_10);
      }
      return 0;
    }
  }
  eVar2 = strcpy_error(_Dst,_SizeInBytes,_Src);
  return eVar2;
}


// ============================================================================
// Function: strcpy_error @ 6ba45ca2
// ============================================================================

undefined8 strcpy_error(undefined1 *param_1,ulonglong param_2,longlong param_3)

{
  undefined8 uVar1;
  
                    /* 0x5ca2  3007  strcpy_error */
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
  else {
    uVar1 = CheckSrcRange(param_1,param_2,param_3);
  }
  return uVar1;
}


