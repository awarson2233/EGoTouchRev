#include "TSACoreInternal.h"

// Definition of the contiguous 1.44 MB memory image covering VA 0x6bb4e000 .. 0x6bcad000
_Alignas(64) uint8_t g_tsaMemoryImage[kTsaMemoryImageSize];
