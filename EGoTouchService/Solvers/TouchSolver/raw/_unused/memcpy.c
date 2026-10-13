// ============================================================================
// Function: memcpy_s @ 6ba41521
// ============================================================================

errno_t __cdecl memcpy_s(void *_Dst,rsize_t _DstSize,void *_Src,rsize_t _MaxCount)

{
  bool bVar1;
  errno_t eVar2;
  
                    /* 0x1521  3001  memcpy_s */
  if ((_DstSize < _MaxCount) || (_Dst == (void *)0x0)) {
    bVar1 = false;
  }
  else {
    bVar1 = true;
  }
  if ((bVar1) && (_Src != (void *)0x0)) {
    bVar1 = true;
  }
  else {
    bVar1 = false;
  }
  if ((bVar1) && (_MaxCount != 0)) {
    bVar1 = true;
  }
  else {
    bVar1 = false;
  }
  if (!bVar1) goto LAB_6ba41650;
  if ((_Src < _Dst) && ((void *)(_MaxCount + (longlong)_Src) <= _Dst)) {
    bVar1 = true;
  }
  else {
    bVar1 = false;
  }
  if (bVar1) {
LAB_6ba41622:
    bVar1 = true;
  }
  else {
    if ((_Dst < _Src) && ((void *)(_MaxCount + (longlong)_Dst) <= _Src)) {
      bVar1 = true;
    }
    else {
      bVar1 = false;
    }
    if (bVar1) goto LAB_6ba41622;
    bVar1 = false;
  }
  if (bVar1) {
    memcpy(_Dst,_Src,_MaxCount);
    return 0;
  }
LAB_6ba41650:
  eVar2 = _text(_Dst,_DstSize,_Src,_MaxCount);
  return eVar2;
}


