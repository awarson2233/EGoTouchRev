#pragma once
// 原 DLL 的 .bss（VA 0x6bb7f000..0x6bcad000）：各模块的全局状态按原地址排在一个结构里，模块经 Slot<T>() 按类型取用。
// 原代码有依赖排布的越界读写（历史下标为 kNoTouch 或越界时落在历史表之后的对象上、子区域下标越界等），
// 落点取决于对象之间的相对位置，所以 .bss 整体保持原排布；没有模块用到的区间是 reserved。
// 槽名取自原符号（raw/_metadata/bss_symbols.txt）；新增状态时从相邻 reserved 里切出槽，放在它的原地址上。

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <type_traits>

namespace Solvers::Tsa::Core {

/// .bss 在原 DLL 里的起点；回放的状态摘要按它把偏移还原成原地址。
inline constexpr uint64_t kBssVa = 0x6bb7f000;

struct alignas(64) BssLayout {
    std::byte reserved000000[0x18];
    std::byte blStopCheckTime[0x8];  // stopCheckTimeStamp.11212
    std::byte blsmNextEdgeSmallThresh[0x20];  // g_BLSMNextEdgeSmallThresh
    std::byte preCMFDiffLogEnabled[0x40];  // g_preCMFDiffLogEnabled
    std::byte cmfNoise[0xc0];  // g_cmfNoise
    std::byte reserved000140[0x1580];
    std::byte erLimit[0x2];  // g_erLimit
    std::byte flagMoveInCorrectedButDisabled[0xe];  // g_flagMoveInCorrectedButDisabled
    std::byte reached[0x30];  // g_reached
    std::byte reserved001700[0x700];
    std::byte tpSensorFlag[0x8];  // g_tpSensorFlag
    std::byte tpSensorDim1Offset[0x8];  // g_tpSensorDim1Offset
    std::byte tpSensorDim2Offset[0x8];  // g_tpSensorDim2Offset
    std::byte tpSensorDifScale[0x28];  // g_tpSensorDifScale
    std::byte tpSensorBrokenList[0x140];  // g_tpSensorBrokenList
    std::byte flickTestFailed[0x10];  // g_flickTestFailed
    std::byte prevOccupiedId[0x10];  // prevOccupiedID.10025
    std::byte ptrAccess[0x8];  // g_ptrAccess
    std::byte reserved001fa8[0x18];
    std::byte tzBuffer[0x40];  // g_tzBuffer
    std::byte contourEdgeDot[0x280];  // edgeDot.11595
    std::byte bufferTable[0x158];  // g_tsaBufTimeOff
    std::byte reserved0023d8[0x628];
    std::byte blockIdx[0x10];  // g_tsaBlockIdx
    std::byte idleTimeoutStamp[0x8];  // lastTimeOutStamp.10344
    std::byte sideIdleTimeoutStamp[0x8];  // lastTimeOutStamp.10349
    std::byte reserved002a20[0x21ae0];
    std::byte handBuffers[0x30];  // g_tsaBufAreaHand
    std::byte cntRepeat[0x50];  // g_cntRepeat
    std::byte hotZoneAnalyzer[0x700];  // g_hotZoneAnalyzer
    std::byte hotZoneRect[0x2c0];  // g_hotZoneRect
    std::byte reserved024f40[0x50];
    std::byte bufProxBL[0x8];  // g_tsaBufProxBL
    std::byte reserved024f98[0x6148];
    std::byte bufTzSide[0x8];  // g_tsaBufTzSide
    std::byte reserved02b0e8[0x4d8];
    std::byte afeSlots[0x18];  // g_thpAFEStruct
    std::byte reserved02b5d8[0x14f58];
    std::byte badChannelDebounce[0x10];  // g_badChannelDebounce
    std::byte reserved040540[0x20];
    std::byte pressureDrift[0x10];  // g_lastPressureDriftDetectedTimeStamp
    std::byte rfNoise[0x20];  // g_tsaBufPrevRaw2rawDif
    std::byte toeSynaBl[0x20];  // g_tsaBufRawStoredSinceBackLightShift
    std::byte reserved0405b0[0x13a0];
    std::byte zValPrevToCurRatio[0x4];  // g_zValPrevToCurRatio
    std::byte yPosAftIirFilter[0x4];  // g_yPosAftIirFilter
    std::byte xPosPreFilter[0x4];  // g_xPosPreFilter
    std::byte xPosAftIirFilter[0x4];  // g_xPosAftIirFilter
    std::byte yPosPreFilter[0x20];  // g_yPosPreFilter
    std::byte peaksGrid[0x1c0];  // g_peaksGrid
    std::byte peaksBeforeSwitchDifBuf[0x1c0];  // g_peaksBeforeSwitchDifBuf
    std::byte prevPeaksGrid[0x100];  // g_prevPeaksGrid
    std::byte reserved041e00[0x40];
    std::byte prevTouchesGrid[0x5aa0];  // g_prevTouchesGrid
    std::byte prevTouchInvalidID[0x20];  // g_prevTouchInvalidID
    std::byte touchesGrid[0x1d60];  // g_touchesGrid
    std::byte tmSwitchTimer[0x20];  // g_tmSwitchTimer
    std::byte subTZ[0x2d8];  // g_subTZ
    std::byte pkAccessedFlagSubTZ[0x8];  // g_pkAccessedFlagSubTZ
    std::byte tzAccess[0x10];  // g_tzAccess
    std::byte subTZAccess[0x10];  // g_subTZAccess
    std::byte tzBufferBin[0x19000];  // g_tzBufferBin
    std::byte tzBufferHighRes[0x32000];  // g_tzBufferHighRes
    std::byte afu[0x50];  // g_afu
    std::byte pkBeforeSDCAccessedFlag[0x4];  // g_pkBeforeSDCAccessedFlag
    std::byte pkAccessedFlag[0x2c];  // g_pkAccessedFlag
    std::byte reserved094a00[0x4e80];
    std::byte bufSelf[0x1b80];  // g_tsaBufSelf
    std::byte reserved09b400[0x4f48];
    std::byte gridMapBufSize[0x38];  // g_gridMapBufSize
    std::byte reserved0a0380[0x4d00];
    std::byte bufSelfForBlRst[0x1b80];  // g_tsaBufSelfForBlRst
    std::byte reserved0a6c00[0x8b88];
    std::byte bufTemp[0x38];  // g_tsaBufTemp
    std::byte reserved0af7c0[0x62c0];
    std::byte gridBufSize[0x40];  // g_gridBufSize
    std::byte reserved0b5ac0[0x62c0];
    std::byte bufSSPreCMFDif[0x280];  // g_tsaBufSSPreCMFDif
    std::byte reserved0bc000[0x2900];
    std::byte bufSsDifForBl[0x280];  // g_tsaBufSsDifForBl
    std::byte lineBufSize[0x40];  // g_lineBufSize
    std::byte reserved0bebc0[0x7348];
    std::byte maxDimSize[0x38];  // g_maxDimSize
    std::byte reserved0c5f40[0x5700];
    std::byte prpt[0x340];  // g_tsaPrpt
    std::byte difBlockPrpt[0x100];  // g_tsaDifBlockPrpt
    std::byte staticGrid[0x450];  // g_tsaStaticGrid
    std::byte reserved0cbed0[0x39f70];
    std::byte prevPeaksHand[0x100];  // g_prevPeaksHand
    std::byte touchesHand[0x1d60];  // g_touchesHand
    std::byte reserved107ca0[0x60];
    std::byte peaksHand[0x1a0];  // g_peaksHand
    std::byte reserved107ea0[0x60];
    std::byte prevTouchesHand[0x5ac0];  // g_prevTouchesHand
    std::byte hwAnalyzer[0x240];  // g_hwAnalyzer
    std::byte reserved10dc00[0x5c0];
    std::byte touchesSide[0x1d80];  // g_touchesSide
    std::byte reserved10ff40[0x18c80];
    std::byte prmtFlashAsa[0xe40];  // g_tsaPrmtFlashAsa
    std::byte prmtFlash[0x1890];  // g_tsaPrmtFlash
    std::byte reserved12b290[0x30];
    std::byte prmtFlashCmfPca[0x1340];  // g_tsaPrmtFlashCmfPca
    std::byte prmtFlashConst[0x64];  // g_tsaPrmtFlashConst
    std::byte prmtRamConst[0x1c];  // g_tsaPrmtRamConst
    std::byte prmtDynamic[0xa0];  // g_tsaPrmtDynamic
    std::byte prmtRam[0x1c];  // g_tsaPrmtRam
    std::byte reserved12c73c[0x44];
    std::byte rtPrmt[0x180];  // g_tsaRTPrmt
    std::byte rtPrmtDefault[0x180];  // g_tsaRTPrmtDefault
    std::byte ttPrmt[0xa40];  // g_tsaTTPrmt
    std::byte list[0x98];  // g_list
    std::byte reserved12d558[0xaa8];
};
static_assert(sizeof(BssLayout) == 0x12e000);
static_assert(offsetof(BssLayout, blStopCheckTime) == 0x000018);
static_assert(offsetof(BssLayout, blsmNextEdgeSmallThresh) == 0x000020);
static_assert(offsetof(BssLayout, preCMFDiffLogEnabled) == 0x000040);
static_assert(offsetof(BssLayout, cmfNoise) == 0x000080);
static_assert(offsetof(BssLayout, erLimit) == 0x0016c0);
static_assert(offsetof(BssLayout, flagMoveInCorrectedButDisabled) == 0x0016c2);
static_assert(offsetof(BssLayout, reached) == 0x0016d0);
static_assert(offsetof(BssLayout, tpSensorFlag) == 0x001e00);
static_assert(offsetof(BssLayout, tpSensorDim1Offset) == 0x001e08);
static_assert(offsetof(BssLayout, tpSensorDim2Offset) == 0x001e10);
static_assert(offsetof(BssLayout, tpSensorDifScale) == 0x001e18);
static_assert(offsetof(BssLayout, tpSensorBrokenList) == 0x001e40);
static_assert(offsetof(BssLayout, flickTestFailed) == 0x001f80);
static_assert(offsetof(BssLayout, prevOccupiedId) == 0x001f90);
static_assert(offsetof(BssLayout, ptrAccess) == 0x001fa0);
static_assert(offsetof(BssLayout, tzBuffer) == 0x001fc0);
static_assert(offsetof(BssLayout, contourEdgeDot) == 0x002000);
static_assert(offsetof(BssLayout, bufferTable) == 0x002280);
static_assert(offsetof(BssLayout, blockIdx) == 0x002a00);
static_assert(offsetof(BssLayout, idleTimeoutStamp) == 0x002a10);
static_assert(offsetof(BssLayout, sideIdleTimeoutStamp) == 0x002a18);
static_assert(offsetof(BssLayout, handBuffers) == 0x024500);
static_assert(offsetof(BssLayout, cntRepeat) == 0x024530);
static_assert(offsetof(BssLayout, hotZoneAnalyzer) == 0x024580);
static_assert(offsetof(BssLayout, hotZoneRect) == 0x024c80);
static_assert(offsetof(BssLayout, bufProxBL) == 0x024f90);
static_assert(offsetof(BssLayout, bufTzSide) == 0x02b0e0);
static_assert(offsetof(BssLayout, afeSlots) == 0x02b5c0);
static_assert(offsetof(BssLayout, badChannelDebounce) == 0x040530);
static_assert(offsetof(BssLayout, pressureDrift) == 0x040560);
static_assert(offsetof(BssLayout, rfNoise) == 0x040570);
static_assert(offsetof(BssLayout, toeSynaBl) == 0x040590);
static_assert(offsetof(BssLayout, zValPrevToCurRatio) == 0x041950);
static_assert(offsetof(BssLayout, yPosAftIirFilter) == 0x041954);
static_assert(offsetof(BssLayout, xPosPreFilter) == 0x041958);
static_assert(offsetof(BssLayout, xPosAftIirFilter) == 0x04195c);
static_assert(offsetof(BssLayout, yPosPreFilter) == 0x041960);
static_assert(offsetof(BssLayout, peaksGrid) == 0x041980);
static_assert(offsetof(BssLayout, peaksBeforeSwitchDifBuf) == 0x041b40);
static_assert(offsetof(BssLayout, prevPeaksGrid) == 0x041d00);
static_assert(offsetof(BssLayout, prevTouchesGrid) == 0x041e40);
static_assert(offsetof(BssLayout, prevTouchInvalidID) == 0x0478e0);
static_assert(offsetof(BssLayout, touchesGrid) == 0x047900);
static_assert(offsetof(BssLayout, tmSwitchTimer) == 0x049660);
static_assert(offsetof(BssLayout, subTZ) == 0x049680);
static_assert(offsetof(BssLayout, pkAccessedFlagSubTZ) == 0x049958);
static_assert(offsetof(BssLayout, tzAccess) == 0x049960);
static_assert(offsetof(BssLayout, subTZAccess) == 0x049970);
static_assert(offsetof(BssLayout, tzBufferBin) == 0x049980);
static_assert(offsetof(BssLayout, tzBufferHighRes) == 0x062980);
static_assert(offsetof(BssLayout, afu) == 0x094980);
static_assert(offsetof(BssLayout, pkBeforeSDCAccessedFlag) == 0x0949d0);
static_assert(offsetof(BssLayout, pkAccessedFlag) == 0x0949d4);
static_assert(offsetof(BssLayout, bufSelf) == 0x099880);
static_assert(offsetof(BssLayout, gridMapBufSize) == 0x0a0348);
static_assert(offsetof(BssLayout, bufSelfForBlRst) == 0x0a5080);
static_assert(offsetof(BssLayout, bufTemp) == 0x0af788);
static_assert(offsetof(BssLayout, gridBufSize) == 0x0b5a80);
static_assert(offsetof(BssLayout, bufSSPreCMFDif) == 0x0bbd80);
static_assert(offsetof(BssLayout, bufSsDifForBl) == 0x0be900);
static_assert(offsetof(BssLayout, lineBufSize) == 0x0beb80);
static_assert(offsetof(BssLayout, maxDimSize) == 0x0c5f08);
static_assert(offsetof(BssLayout, prpt) == 0x0cb640);
static_assert(offsetof(BssLayout, difBlockPrpt) == 0x0cb980);
static_assert(offsetof(BssLayout, staticGrid) == 0x0cba80);
static_assert(offsetof(BssLayout, prevPeaksHand) == 0x105e40);
static_assert(offsetof(BssLayout, touchesHand) == 0x105f40);
static_assert(offsetof(BssLayout, peaksHand) == 0x107d00);
static_assert(offsetof(BssLayout, prevTouchesHand) == 0x107f00);
static_assert(offsetof(BssLayout, hwAnalyzer) == 0x10d9c0);
static_assert(offsetof(BssLayout, touchesSide) == 0x10e1c0);
static_assert(offsetof(BssLayout, prmtFlashAsa) == 0x128bc0);
static_assert(offsetof(BssLayout, prmtFlash) == 0x129a00);
static_assert(offsetof(BssLayout, prmtFlashCmfPca) == 0x12b2c0);
static_assert(offsetof(BssLayout, prmtFlashConst) == 0x12c600);
static_assert(offsetof(BssLayout, prmtRamConst) == 0x12c664);
static_assert(offsetof(BssLayout, prmtDynamic) == 0x12c680);
static_assert(offsetof(BssLayout, prmtRam) == 0x12c720);
static_assert(offsetof(BssLayout, rtPrmt) == 0x12c780);
static_assert(offsetof(BssLayout, rtPrmtDefault) == 0x12c900);
static_assert(offsetof(BssLayout, ttPrmt) == 0x12ca80);
static_assert(offsetof(BssLayout, list) == 0x12d4c0);

extern BssLayout g_bss;  // Common/Bss.cpp

[[nodiscard]] inline BssLayout& Bss() noexcept { return g_bss; }

/// 槽里偏移 Offset 处的 T。std::byte 数组隐式创建其中的对象，T 须是隐式生存期类型（平凡结构体、标量、数组）。
template <class T, size_t Offset = 0, size_t N>
[[nodiscard]] inline T& Slot(std::byte (&slot)[N]) noexcept {
    static_assert(Offset + sizeof(T) <= N, "object does not fit its .bss slot");
    return *std::launder(reinterpret_cast<T*>(slot + Offset));
}

/// 把对象的全部字节（包括填充）清零，对应原代码的 memset_s(&obj, sizeof obj, 0, sizeof obj)。
/// 填充字节也是状态的一部分，不能用 obj = {} 代替。
template <class T>
inline void ZeroFill(T& obj) noexcept {
    static_assert(std::is_trivially_copyable_v<T>);
    std::memset(&obj, 0, sizeof(T));
}

}  // namespace Solvers::Tsa::Core
