// ============================================================================
// Function: BaliSynaFM_IsFastMoveDetected @ 6bb43690
// ============================================================================

undefined1 BaliSynaFM_IsFastMoveDetected(void)

{
  ushort uVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  byte local_b;
  ushort local_a;
  
                    /* 0x103690  392  BaliSynaFM_IsFastMoveDetected */
  local_b = 0;
  do {
    if ((*_refptr_g_prevTouchesPtr)->bPrevTouchCount <= local_b) {
LAB_6bb437f4:
      if ((*_refptr_g_prevTouchesPtr)->bPrevTouchCount == '\0') {
        fastFlag_5684 = 0;
      }
      return fastFlag_5684;
    }
    uVar1 = *(ushort *)
             ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_b * 0x478 + 0x150);
    local_a = (uVar1 ^ (short)uVar1 >> 0x1f) - ((short)uVar1 >> 0xf);
    if (local_a == 0) {
      if (*(longlong *)
           ((longlong)*_refptr_g_prevTouchesPtr + (longlong)(int)(uint)local_b * 0x478 + 0x148) == 0
         ) {
        local_a = 1;
      }
      else {
        local_a = (short)(*_refptr_g_tsaStaticPtr)->pFrameBufferPtr -
                  (short)*(undefined8 *)
                          ((longlong)*_refptr_g_prevTouchesPtr +
                          (longlong)(int)(uint)local_b * 0x478 + 0x148);
      }
    }
    uVar2 = (int)*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                           (longlong)(int)(uint)local_b * 0x478 + 0x4d4) / (int)(uint)local_a;
    uVar5 = (int)uVar2 >> 0x1f;
    iVar3 = (uVar2 ^ uVar5) - uVar5;
    uVar2 = (int)*(short *)((longlong)*_refptr_g_prevTouchesPtr +
                           (longlong)(int)(uint)local_b * 0x478 + 0x4d6) / (int)(uint)local_a;
    uVar5 = (int)uVar2 >> 0x1f;
    iVar4 = (uVar2 ^ uVar5) - uVar5;
    if (0x1000 < (uint)(iVar4 * iVar4 + iVar3 * iVar3)) {
      fastFlag_5684 = 1;
      goto LAB_6bb437f4;
    }
    local_b = local_b + 1;
  } while( true );
}


