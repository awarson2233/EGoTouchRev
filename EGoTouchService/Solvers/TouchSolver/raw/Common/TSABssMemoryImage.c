#include "TSACoreInternal.h"

// Definition of the contiguous 1.44 MB memory image covering VA 0x6bb4e000 .. 0x6bcad000
_Alignas(64) uint8_t g_tsaMemoryImage[kTsaMemoryImageSize];

// 宿主入口：在任何 TSA_* 调用之前执行一次。装载 .data/.rdata 初值、套用内部指针重定位、
// 写入 W273AS2700 参数表。TSA_MemoryImage_Init 是头文件里的 static inline，宿主看不到。
void TSACoreRaw_InitImage(void)
{
  TSA_MemoryImage_Init();
}

// 过渡期：C++ 的 Prmt::LoadProject 经它重新装入 W273AS2700 的三张参数表（原 TSAPrmt_FlashPrmtLoadProject），
// 参数数据目前还在 raw 头文件里。
void TSACoreRaw_LoadProjectTables(void)
{
  memcpy(&g_tsaPrmtFlash, kW273AS2700_FlashPrmt, sizeof(kW273AS2700_FlashPrmt));
  memcpy(_refptr_g_tsaPrmtFlashCmfPca, kW273AS2700_CmfPcaPrmt, sizeof(kW273AS2700_CmfPcaPrmt));
  memcpy(_refptr_g_tsaPrmtFlashAsa, kW273AS2700_AsaPrmt, sizeof(kW273AS2700_AsaPrmt));
}
