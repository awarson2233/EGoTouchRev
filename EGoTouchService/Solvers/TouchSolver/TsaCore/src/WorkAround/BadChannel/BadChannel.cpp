#include "WorkAround/BadChannel/BadChannel.h"

#include <cstdint>

#include "App/TsaBuffer.h"
#include "Common/Assert.h"
#include "Common/Image.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::BadChannel {

namespace {

constexpr uint8_t kGroupRows = 4;        // 行按 4 行一组（同一组共用一路驱动）
constexpr uint8_t kDebounceFrames = 0x78;
constexpr uint8_t kBrokenByBadChannel = 2;

/// 每格的坏点防抖帧数（g_badChannelDebounce）。
uint8_t*& Debounce() { return Image::At<uint8_t*>(0x6bbbf530); }

uint16_t RawAt(uint16_t i) { return static_cast<uint16_t>(Buffers().raw[i]); }

// BadChannel_DetectType1 @ 0x6bb42be5
// 第 col 列从 row 起的 4 格 raw 都不在 (low, high) 内。
bool DetectType1(uint8_t col, uint8_t row, uint16_t low, uint16_t high) {
    const uint8_t cols = PrmtFlash().dim1Len;
    for (uint16_t k = 0; k < kGroupRows; ++k) {
        const uint16_t v = RawAt(static_cast<uint16_t>(col + (k + row) * cols));
        if (low < v && v < high) return false;
    }
    return true;
}

// BadChannel_DetectType2 @ 0x6bb42c80
// 第 col 列按行号模 4 分成 4 组，各组去掉最大值后求均值；均值低于 maxAvg 且比最大的组均值低 minDrop 以上的组为坏组。
// maxRow[g] 记坏组里 raw 最大的行，其余组记 -1。
bool DetectType2(uint8_t col, uint16_t maxAvg, uint16_t minDrop, int32_t maxRow[kGroupRows]) {
    const FlashPrmt& fl = PrmtFlash();
    int avg[kGroupRows];
    int best = 0;
    for (uint8_t g = 0; g < kGroupRows; ++g) {
        int n = 0;
        uint32_t max = 0;
        avg[g] = 0;
        maxRow[g] = -1;
        for (uint8_t r = g; r < fl.dim2Len; r = static_cast<uint8_t>(r + kGroupRows)) {
            const uint16_t v = RawAt(GridIndex(col, r));
            avg[g] += v;
            ++n;
            if (max < v) {
                max = v;
                maxRow[g] = r;
            }
        }
        if (1 < n) avg[g] = static_cast<int>(static_cast<uint32_t>(avg[g]) - max) / (n - 1);
        if (best < avg[g]) best = avg[g];
    }
    bool r = false;
    for (uint8_t g = 0; g < kGroupRows; ++g) {
        if (avg[g] < maxAvg && minDrop < best - avg[g]) {
            r = true;
        } else {
            maxRow[g] = -1;
        }
    }
    return r;
}

// BadChannel_SetDebounce @ 0x6bb42f49
void SetDebounce(uint16_t i) {
    const uint16_t n = PrmtFlash().sensorNum;
    if (n <= i) i = static_cast<uint16_t>(n - 1);
    Debounce()[i] = kDebounceFrames;
}

// BadChannel_CheckType1 @ 0x6bb42f89
void CheckType1() {
    const FlashPrmt& fl = PrmtFlash();
    for (uint8_t r = 0; r < fl.dim2Len - 3; r = static_cast<uint8_t>(r + kGroupRows)) {
        for (uint8_t c = 0; c < fl.dim1Len; ++c) {
            if (!DetectType1(c, r, 14000, 0xb478)) continue;
            for (uint16_t k = 0; k < kGroupRows; ++k) SetDebounce(static_cast<uint16_t>(c + (k + r) * fl.dim1Len));
        }
    }
}

// BadChannel_CheckType2 @ 0x6bb43052
// 坏组里 raw 最大的行所在的 4 行一组整组标坏。
void CheckType2() {
    const FlashPrmt& fl = PrmtFlash();
    for (uint8_t c = 0; c < fl.dim1Len; ++c) {
        int32_t maxRow[kGroupRows] = {};
        if (!DetectType2(c, 21000, 0x15e0, maxRow)) continue;
        for (uint16_t g = 0; g < kGroupRows; ++g) {
            if (maxRow[g] < 0) continue;
            const auto base = static_cast<uint8_t>(maxRow[g] / kGroupRows * kGroupRows);
            for (uint16_t k = 0; k < kGroupRows; ++k) SetDebounce(static_cast<uint16_t>(c + (k + base) * fl.dim1Len));
        }
    }
}

}  // namespace

// BadChannel_Malloc @ 0x6bb42b10
bool Malloc() {
    Debounce() = static_cast<uint8_t*>(TSA_MallocAndInit(GridBufSize()));
    return Debounce() != nullptr;
}

// BadChannel_Free @ 0x6bb42b51
void Free() { TSA_Free(reinterpret_cast<int64_t*>(&Debounce())); }

// BadChannel_Init @ 0x6bb42b6c
void Init() { ClearChecked(Debounce(), GridBufSize(), GridBufSize()); }

// BadChannel_Process @ 0x6bb4325f（BadChannel_Detect @ 0x6bb43154）
// 防抖逐帧减一，仍在防抖中的格报为坏点；某行报失败（坏点表满）时跳过该行余下的格。
void Process() {
    CheckType1();
    CheckType2();
    const FlashPrmt& fl = PrmtFlash();
    uint8_t* deb = Debounce();
    for (uint8_t r = 0; r < fl.dim2Len; ++r) {
        for (uint8_t c = 0; c < fl.dim1Len; ++c) {
            const uint16_t i = GridIndex(c, r);
            if (deb[i] != 0) --deb[i];
            if (deb[i] != 0 && !TPSensor_AddToBrokenSensorList(c, r, kBrokenByBadChannel)) break;
        }
    }
}

// BadChannel_IsDMDDetected @ 0x6bb42e68
bool IsDmdDetected() {
    const FlashPrmt& fl = PrmtFlash();
    for (uint8_t c = 0; c < fl.dim1Len; ++c) {
        for (uint8_t r = 0; r < fl.dim2Len; r = static_cast<uint8_t>(r + kGroupRows)) {
            if (DetectType1(c, r, 0x41a0, 42000)) return true;
        }
    }
    for (uint8_t c = 0; c < fl.dim1Len; ++c) {
        int32_t maxRow[kGroupRows];
        if (DetectType2(c, 0x5cf8, 0xdac, maxRow)) return true;
    }
    return false;
}

}  // namespace Solvers::Tsa::Core::BadChannel

// raw 的 HardwareAnalyzer / TSA / TSABuffer 还在调用的入口
extern "C" {
namespace BC = Solvers::Tsa::Core::BadChannel;
bool BadChannel_Malloc(void) { return BC::Malloc(); }
void BadChannel_Free(void) { BC::Free(); }
void BadChannel_Init(void) { BC::Init(); }
void BadChannel_Process(void) { BC::Process(); }
uint64_t BadChannel_IsDMDDetected(void) { return BC::IsDmdDetected() ? 1 : 0; }
}  // extern "C"
