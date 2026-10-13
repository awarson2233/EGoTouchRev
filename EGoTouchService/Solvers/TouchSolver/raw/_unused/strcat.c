// ============================================================================
// Function: strcat_s @ 6ba45bc8
// ============================================================================

errno_t __cdecl strcat_s(char *_Dst,rsize_t _SizeInBytes,char *_Src)

{
  errno_t eVar1;
  
                    /* 0x5bc8  3006  strcat_s */
  if ((_SizeInBytes == 0) || (0x7fffffff < _SizeInBytes)) {
    eVar1 = 0x22;
  }
  else if ((_Dst == (char *)0x0) || (_Src == (char *)0x0)) {
    if (_Dst == (char *)0x0) {
      eVar1 = 0x16;
    }
    else {
      *_Dst = '\0';
      eVar1 = 0x96;
    }
  }
  else {
    eVar1 = SecDoCat(_Dst,_SizeInBytes,_Src);
  }
  return eVar1;
}


