// ============================================================================
// Function: DataSwitch_ToGrid @ 6ba906d0
// ============================================================================

void DataSwitch_ToGrid(void)

{
                    /* 0x506d0  528  DataSwitch_ToGrid */
  *(undefined8 *)_refptr_g_tsaBufTz = *(undefined8 *)_refptr_g_tsaBufTzGrid;
  *(undefined **)_refptr_g_touchesPtr = _refptr_g_touchesGrid;
  *_refptr_g_prevTouchesPtr = (PrevTouchArray *)_refptr_g_prevTouchesGrid;
  *_refptr_g_peaksPtr = (PeakArray *)_refptr_g_peaksGrid;
  *(undefined **)_refptr_g_prevPeaksPtr = _refptr_g_prevPeaksGrid;
  return;
}


// ============================================================================
// Function: DataSwitch_ToSide @ 6ba90770
// ============================================================================

void DataSwitch_ToSide(void)

{
                    /* 0x50770  531  DataSwitch_ToSide */
  *(undefined8 *)_refptr_g_tsaBufTz = *(undefined8 *)_refptr_g_tsaBufTzSide;
  *(undefined **)_refptr_g_touchesPtr = _refptr_g_touchesSide;
  *_refptr_g_prevTouchesPtr = (PrevTouchArray *)_refptr_g_prevTouchesSide;
  *_refptr_g_peaksPtr = (PeakArray *)_refptr_g_peaksSide;
  *(undefined **)_refptr_g_prevPeaksPtr = _refptr_g_prevPeaksSide;
  return;
}


// ============================================================================
// Function: DataSwitch_ToProx @ 6ba9072e
// ============================================================================

void DataSwitch_ToProx(void)

{
                    /* 0x5072e  530  DataSwitch_ToProx */
  *(undefined8 *)_refptr_g_tsaBufTz = *(undefined8 *)_refptr_g_tsaBufTzProx;
  *(undefined8 *)_refptr_g_touchesPtr = *(undefined8 *)_refptr_g_touchesProxPtr;
  *_refptr_g_prevTouchesPtr = *(PrevTouchArray **)_refptr_g_prevTouchesProxPtr;
  return;
}


// ============================================================================
// Function: DataSwitch_ToHand @ 6ba907ce
// ============================================================================

void DataSwitch_ToHand(void)

{
                    /* 0x507ce  529  DataSwitch_ToHand */
  *(undefined8 *)_refptr_g_tsaBufTz = *(undefined8 *)_refptr_g_tsaBufTzHand;
  *(undefined **)_refptr_g_touchesPtr = _refptr_g_touchesHand;
  *_refptr_g_prevTouchesPtr = (PrevTouchArray *)_refptr_g_prevTouchesHand;
  *_refptr_g_peaksPtr = (PeakArray *)_refptr_g_peaksHand;
  *(undefined **)_refptr_g_prevPeaksPtr = _refptr_g_prevPeaksHand;
  return;
}


