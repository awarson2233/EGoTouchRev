#pragma once
// HardwareAnalyzer 各源文件之间共用的状态与函数（不对外）。

#include <cstddef>
#include <cstdint>

#include "Common/Image.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::HardwareAnalyzer {

/// 异常码（位图里的位号）。
enum Exception : uint8_t {
    kExcRfNoise = 1,
    kExcRaw2RawNoise = 2,          // raw2raw 超噪声的格多、或长时间 raw 跳变
    kExcSoft = 3,                  // 断言失败
    kExcRawLimit = 4,              // raw 的范围超出 BLRecal 的中心 ± 1.5 倍范围
    kExcDisplayNoise = 5,          // 显示噪声大、或 CMF 公共噪声超过 fingerThold
    kExcSensorBrokenByRaw = 6,
    kExcSensorBrokenByDif = 7,
    kExcBaselineReset = 8,
    kExcRecal = 9,
    kExcSpecialControl = 10,
    kExcAbnormalBaseline = 0xd,    // 侧边基线异常、或环境温度异常
    kExcEventInterval = 0xe,
    kExcAbnormalShape = 0x10,      // 基线复位原因 9、或核心触点椭圆度持续偏大
    kExcScreenOnTimeOut = 0x11,
    kExcScreenOffTimeOut = 0x12,
    kExcPowerKey = 0x13,
    kExcVolumeGui = 0x15,
    kExcAfeRawJump = 0x16,
    kExcAfeRawUnstable = 0x17,
    kExcBigNoise = 0x1d,
    kExcBackLightShift = 0x1e,
    kExcCouplingNoise = 0x1f,
};
inline constexpr int kExceptionNum = 31;

/// g_hwAnalyzer 起的异常状态。
struct State {
    uint32_t cur;   // 本帧的异常位图
    uint32_t prev;  // 上一帧的
    char description[0x1f5];
    uint8_t reserved1FD;
    uint16_t descriptionLen;
    uint32_t afeRawJumpCount;
    uint32_t afeRawUnstableCount;
    uint8_t coreUnstableFrames;  // raw 不稳定期间连续有可疑核心触点的帧数
    uint8_t sideUnstableFrames;
};
static_assert(offsetof(State, description) == 0x008);
static_assert(offsetof(State, descriptionLen) == 0x1fe);
static_assert(offsetof(State, afeRawJumpCount) == 0x200);
static_assert(offsetof(State, coreUnstableFrames) == 0x208);

/// g_cntRepeat 起的计数与坏点缓冲。
struct Counters {
    uint8_t repeat[kExceptionNum];  // 各异常连续检出的次数（够 kRepeatToReport 次才上报）
    uint8_t reserved01F;
    int64_t ellipseRateTimerMs;
    int64_t couplingNoiseTimerMs;
    uint8_t* difDebounce;  // 按 Dif 查坏点：每格的防抖计数
    uint8_t* difState;     // 每格的坏点状态（1 为坏点）
    uint8_t* difChecked;   // 本帧已查过的格
    uint8_t initOnce;
};
static_assert(offsetof(Counters, ellipseRateTimerMs) == 0x20);
static_assert(offsetof(Counters, difDebounce) == 0x30);
static_assert(offsetof(Counters, initOnce) == 0x48);

[[nodiscard]] inline State& St() noexcept { return Image::At<State>(0x6bc8c9c0); }
[[nodiscard]] inline Counters& Cnt() noexcept { return Image::At<Counters>(0x6bba3530); }

[[nodiscard]] inline bool FullIncell() noexcept { return PrmtFlash().sensorArchitecture == 3; }

void SetException(uint8_t code);
[[nodiscard]] inline bool IsDetected(uint32_t bits, uint8_t code) noexcept { return ((1u << (code & 31)) & bits) != 0; }

// SensorBroken.cpp
void SensorBrokenInit();
void SensorBrokenReset();
void SensorBrokenRawProcess();
void SensorBrokenPreDifProcess();
void SensorBrokenDifProcess();

}  // namespace Solvers::Tsa::Core::HardwareAnalyzer
