// ============================================================================
// Function: MtLst_GetVectorA @ 6ba540e0
// ============================================================================

void MtLst_GetVectorA(longlong param_1,int param_2,longlong param_3)

{
  byte bVar1;
  longlong lVar2;
  uint local_c;
  
  lVar2 = *(longlong *)(param_3 + 0x20);
  bVar1 = *(byte *)(param_3 + 0x11);
  for (local_c = 0; local_c < *(byte *)(param_3 + 0x11); local_c = local_c + 1) {
    *(undefined4 *)(param_1 + (ulonglong)local_c * 4) =
         *(undefined4 *)
          ((longlong)(int)((uint)bVar1 * param_2) * 4 + lVar2 + (ulonglong)local_c * 4);
  }
  return;
}


