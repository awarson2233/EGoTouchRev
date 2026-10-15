#include "TSACoreInternal.h"

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


