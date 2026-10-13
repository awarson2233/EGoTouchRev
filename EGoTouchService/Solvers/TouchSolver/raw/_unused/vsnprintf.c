// ============================================================================
// Function: vsnprintf_s @ 6ba46a10
// ============================================================================

int __cdecl
vsnprintf_s(char *_DstBuf,size_t _DstSize,size_t _MaxCount,char *_Format,va_list _ArgList)

{
  int local_c;
  
                    /* 0x6a10  3012  vsnprintf_s */
  if (((((_Format == (char *)0x0) || (_DstBuf == (char *)0x0)) || (_DstSize == 0)) ||
      (0x7fffffff < _DstSize)) || ((0x7ffffffe < _MaxCount && (_MaxCount != 0xffffffffffffffff)))) {
    if ((_DstBuf != (char *)0x0) && ((_DstSize != 0 && (_DstSize < 0x80000000)))) {
      *_DstBuf = '\0';
    }
    local_c = -1;
  }
  else {
    if (_MaxCount < _DstSize) {
      local_c = SecVsnprintfImpl(_DstBuf,_MaxCount + 1,_Format,_ArgList);
      if (local_c == -2) {
        return -1;
      }
    }
    else {
      local_c = SecVsnprintfImpl(_DstBuf,_DstSize,_Format,_ArgList);
      if ((local_c == -2) && (_MaxCount == 0xffffffffffffffff)) {
        return -1;
      }
    }
    if (local_c < 0) {
      *_DstBuf = '\0';
      local_c = -1;
    }
  }
  return local_c;
}


