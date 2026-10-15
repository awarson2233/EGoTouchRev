#include "WorkAround/PressureDrift/PressureDrift.h"

#include <cstdlib>

#include "Alg/Touch/TouchData.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Common/Bss.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::PressureDrift {

namespace {

/// 0x6bbbf560 起的模块状态。
struct State {
    uint64_t lastDetectedTime;  // 上一次判为漂移的时间（ms）
    uint32_t debounce;          // 触点不多时沿用上次结论的帧数
};

State& S() { return Slot<State>(Bss().pressureDrift); }

constexpr uint64_t kHoldMs = 0x19;     // 上次判为漂移后这段时间内沿用
constexpr uint32_t kMaxHoldFrames = 0x33;

// PressureDrift_GradientCalUnit @ 0x6bb440d0
// 该格左右两格的 Dif 之差的绝对值。
int Gradient(uint8_t dim1, uint8_t dim2) {
    const int16_t* dif = Buffers().dif;
    const uint32_t center = PrmtFlash().dim1Len * static_cast<uint32_t>(dim2) + dim1;
    return std::abs(dif[static_cast<uint16_t>(center + 1)] - dif[static_cast<uint16_t>(center - 1)]);
}

// PressureDrift_DetectByTimeStamp @ 0x6bb44172
bool DetectByTimeStamp() {
    State& s = S();
    if (kHoldMs <= Static().curTimeMs - s.lastDetectedTime) return false;
    ++s.debounce;
    return s.debounce < kMaxHoldFrames;
}

}  // namespace

// PressureDrift_Detect @ 0x6bb441c9
// 峰值在 finger 门限的 3/8..3/4 之间，所在行（不含两端列）梯度都不超过门限的 1/3、
// 正 Dif 之和超过峰值的 4.5 倍、梯度之和不超过峰值的 6 倍。触点少于 3 个时只在上次判定后不久才看。
bool Detect(uint8_t dim1, uint8_t dim2) {
    const FlashPrmt& fl = PrmtFlash();
    const int16_t* dif = Buffers().dif;
    const int16_t sig = dif[GridIndex(dim1, dim2)];
    const auto high = static_cast<int16_t>(fl.fingerThold * 3 / 4);
    if (PrevTouches().count < 3) {
        if (!DetectByTimeStamp()) return false;
    } else {
        S().debounce = 0;
    }
    if (high < sig || sig < high / 2) return false;
    int gradSum = 0;
    int sigSum = 0;
    for (uint8_t c = 1; c < fl.dim1Len - 1; ++c) {
        const int g = Gradient(c, dim2);
        if (fl.fingerThold / 3 < g) return false;
        gradSum += g;
        const int16_t v = dif[GridIndex(c, dim2)];
        if (0 < v) sigSum += v;
    }
    if (sigSum < sig * 9 / 2 || sig * 6 < gradSum) return false;
    S().lastDetectedTime = Static().curTimeMs;
    return true;
}

}  // namespace Solvers::Tsa::Core::PressureDrift
