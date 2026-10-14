#include "WorkAround/RfNoise/RfNoise.h"

#include <algorithm>
#include <cstddef>
#include <cstring>

#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Common/Image.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::RfNoise {

namespace {

constexpr uint32_t kFeature2SavePrevDif = 0x10;  // TsaStatic::featureFlags2：每帧保存 Raw2RawDif
constexpr int kBalanceDivisor = 4;                // 两半的偏移相差不超过较大者的 1/4

/// 0x6bbbf570 起的模块状态。noise / noisyLines 按扫描方向分两份：[0] 逐行（沿 Dim1）、[1] 逐列（沿 Dim2）。
struct State {
    int16_t* prevDif;        // g_tsaBufPrevRaw2rawDif：上一帧的帧差
    int16_t* dif2;           // g_tsaBufRaw2rawDif2：再往前一帧
    int16_t* dif3;           // g_tsaBufRaw2rawDif3：再往前两帧（只存不读）
    int16_t noise[2];        // g_maxAbsCMFNoiseDim1 / Dim2
    uint8_t noisyLines[2];   // g_numCMNLineDim1 / Dim2
};
static_assert(offsetof(State, noise) == 0x18);
static_assert(offsetof(State, noisyLines) == 0x1c);

State& S() { return Image::At<State>(0x6bbbf570); }

/// 扫描方向：dim1Axis 非 0 时逐行（沿 Dim1）扫，否则逐列（沿 Dim2）扫。
bool AlongDim1() { return PrmtFlash().dim1Axis != 0; }
int AxisIndex(bool alongDim1) { return alongDim1 ? 0 : 1; }

/// 一组值同号时离零最近者的绝对值，否则 0。
int16_t SameSignNearest(int16_t min, int16_t max) {
    int16_t r = 0;
    if (0 < min) r = min;
    if (max < 0) r = static_cast<int16_t>(-max);
    return r;
}

/// 一条线的结果：三种帧差（本帧、加上一帧、再加前一帧）各自的同号偏移，以及本帧帧差之和。
struct LineStat {
    int16_t offset[3];
    int32_t sum;
};

// RFNoise_Raw2RawProcessDimUnit @ 0x6bb446d9
// 本帧帧差为 PreHwRaw 与 Raw 之差（信号反向时反过来减）。逐格累计后把帧差历史后移一帧。
LineStat ScanLine(bool alongDim1, uint8_t line) {
    const FlashPrmt& fl = PrmtFlash();
    const TsaBufferTable& buf = Buffers();
    State& s = S();
    auto cell = static_cast<uint16_t>(alongDim1 ? fl.dim1Len * line : line);
    const uint8_t step = alongDim1 ? 1 : fl.dim1Len;
    const uint8_t len = alongDim1 ? fl.dim1Len : fl.dim2Len;
    int16_t lo[3] = {INT16_MAX, INT16_MAX, INT16_MAX};
    int16_t hi[3] = {INT16_MIN, INT16_MIN, INT16_MIN};
    LineStat st{};
    for (int k = 0; k < len; ++k, cell = static_cast<uint16_t>(cell + step)) {
        const auto d = static_cast<int16_t>(Static().signalReverted == 0 ? buf.preHwRaw[cell] - buf.raw[cell] : buf.raw[cell] - buf.preHwRaw[cell]);
        const int16_t v[3] = {d, static_cast<int16_t>(d + s.prevDif[cell]), static_cast<int16_t>(s.dif2[cell] + s.prevDif[cell] + d)};
        st.sum += d;
        for (int i = 0; i < 3; ++i) {
            hi[i] = std::max(hi[i], v[i]);
            lo[i] = std::min(lo[i], v[i]);
        }
        s.dif3[cell] = s.dif2[cell];
        s.dif2[cell] = s.prevDif[cell];
    }
    for (int i = 0; i < 3; ++i) st.offset[i] = SameSignNearest(lo[i], hi[i]);
    return st;
}

// RFNoise_IsVariationInRangeUpDown @ 0x6bb449e6（唯一的调用方除数固定为 4）
// a、b 同为正（或同为负，取反后看），且较小者超过较大者的 3/4。原代码先单独判两者为正；
// 任一方不为正时下面的比较本身就不成立，那一步省去。
bool IsClose(int a, int b) {
    if (a < 0 && b < 0) {
        a = -a;
        b = -b;
    }
    return b < a ? a - a / kBalanceDivisor < b : b - b / kBalanceDivisor < a;
}

// RFNoise_Raw2RawProcessDim @ 0x6bb44a73
// 逐条线扫描。本帧偏移超过 finger 门限的线计入噪声线数，它的帧差之和按所在的前半 / 后半累计；
// 超过噪声门限的线（不论是否已累计）再累计一次。有噪声线（或 Prpt::overNoiseCount 超过传感器数的一半）时，
// 前后两半的帧差反向且大小相近即判为射频噪声。噪声值取三种帧差各自在所有线上的最大偏移中最小的。
bool Scan(bool alongDim1, int16_t& noise, uint8_t& noisyLines) {
    const FlashPrmt& fl = PrmtFlash();
    const uint8_t lines = alongDim1 ? fl.dim2Len : fl.dim1Len;
    int16_t maxOffset[3] = {INT16_MIN, INT16_MIN, INT16_MIN};
    int32_t firstHalf = 0;
    int32_t secondHalf = 0;
    uint8_t count = 0;
    for (uint8_t line = 0; line < lines; ++line) {
        const LineStat st = ScanLine(alongDim1, line);
        const auto accumulate = [&] { (line < lines >> 1 ? firstHalf : secondHalf) += st.sum; };
        if (fl.fingerThold < st.offset[0]) {
            ++count;
            accumulate();
        }
        if (Prpt().noiseThold < st.offset[0]) accumulate();
        for (int i = 0; i < 3; ++i) maxOffset[i] = std::max(maxOffset[i], st.offset[i]);
    }
    bool detected = false;
    if (count != 0 || (fl.sensorNum >> 1) < Prpt().overNoiseCount) detected = IsClose(firstHalf, -secondHalf);
    noise = std::min({maxOffset[0], maxOffset[1], maxOffset[2]});
    noisyLines = count;
    return detected;
}

}  // namespace

// RFNoise_Malloc @ 0x6bb443e0
bool Malloc() {
    const uint32_t bytes = GridBufSize() * 2u;
    State& s = S();
    s.prevDif = static_cast<int16_t*>(TSA_MallocAndInit(bytes));
    if (s.prevDif == nullptr) return false;
    s.dif2 = static_cast<int16_t*>(TSA_MallocAndInit(bytes));
    if (s.dif2 == nullptr) return false;
    s.dif3 = static_cast<int16_t*>(TSA_MallocAndInit(bytes));
    return s.dif3 != nullptr;
}

// RFNoise_Free @ 0x6bb44483
void Free() {
    State& s = S();
    TSA_Free(reinterpret_cast<int64_t*>(&s.prevDif));
    TSA_Free(reinterpret_cast<int64_t*>(&s.dif2));
    TSA_Free(reinterpret_cast<int64_t*>(&s.dif3));
}

// RFNoise_Init @ 0x6bb444b6
void Init() {
    const size_t bytes = GridBufSize() * size_t{2};
    State& s = S();
    std::memset(s.prevDif, 0, bytes);
    std::memset(s.dif2, 0, bytes);
    std::memset(s.dif3, 0, bytes);
    s.noise[0] = 0;
    s.noise[1] = 0;
    s.noisyLines[0] = 0;
    s.noisyLines[1] = 0;
}

// RFNoise_Detect @ 0x6bb44ceb
bool Detect() {
    const bool alongDim1 = AlongDim1();
    State& s = S();
    const int i = AxisIndex(alongDim1);
    return Scan(alongDim1, s.noise[i], s.noisyLines[i]);
}

// RFNoise_IsDetected @ 0x6bb44c44
// 噪声值按无符号比较：没有扫描线时它是 -32768，按无符号即 32768，总是超过门限。
bool IsDetected() { return PrmtDynamic().sigThold < static_cast<uint16_t>(NoiseValue()); }

// RFNoise_GetNoiseValue @ 0x6bb44ca1
int16_t NoiseValue() { return S().noise[AxisIndex(AlongDim1())]; }

// RFNoise_GetNumNoisyLine @ 0x6bb44cc6
uint8_t NoisyLineCount() { return S().noisyLines[AxisIndex(AlongDim1())]; }

// RFNoise_CopyPrevRaw2RawDif @ 0x6bb4463a
void SavePrevRaw2RawDif() {
    if ((Static().featureFlags2 & kFeature2SavePrevDif) == 0) return;
    std::memcpy(S().prevDif, Buffers().raw2RawDif, GridBufSize() * size_t{2});
}

}  // namespace Solvers::Tsa::Core::RfNoise

// raw 的 HardwareAnalyzer / TSABuffer 还在调用的入口
extern "C" {
namespace R = Solvers::Tsa::Core::RfNoise;
uint64_t RFNoise_Malloc(void) { return R::Malloc() ? 1 : 0; }
void RFNoise_Free(void) { R::Free(); }
void RFNoise_Init(void) { R::Init(); }
uint8_t RFNoise_Detect(void) { return R::Detect() ? 1 : 0; }
uint8_t RFNoise_IsDetected(void) { return R::IsDetected() ? 1 : 0; }
int16_t RFNoise_GetNoiseValue(void) { return R::NoiseValue(); }
uint8_t RFNoise_GetNumNoisyLine(void) { return R::NoisyLineCount(); }
void RFNoise_CopyPrevRaw2RawDif(void) { R::SavePrevRaw2RawDif(); }
}  // extern "C"
