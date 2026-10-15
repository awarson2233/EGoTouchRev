// ============================================================================
// Function: memset_s @ 6ba416ef
// ============================================================================

undefined8 memset_s(longlong param_1,ulonglong param_2,undefined4 param_3,ulonglong param_4)

{
  bool bVar1;
  undefined8 uVar2;
  
                    /* 0x16ef  3002  memset_s */
  if ((param_2 < 0x80000000) && (param_1 != 0)) {
    bVar1 = true;
  }
  else {
    bVar1 = false;
  }
  if ((bVar1) && (param_4 <= param_2)) {
    (*(code *)_refptr_memset)(param_1,param_3,param_4);
    uVar2 = 0;
  }
  else {
    uVar2 = SecMemsetError(param_1,param_2,param_3,param_4);
  }
  return uVar2;
}


