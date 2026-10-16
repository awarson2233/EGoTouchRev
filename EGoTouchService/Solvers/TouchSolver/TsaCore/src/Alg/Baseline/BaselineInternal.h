#pragma once
// Baseline 各文件之间、以及 WorkAround/ToeSynaBl 用到的内部接口。

#include <cstdint>

#include "App/TsaStatic.h"

namespace Solvers::Tsa::Core::Baseline {

/// TsaPrpt::blStage
enum Stage : uint32_t {
    kStageNoTouch = 0,     // 无触摸：按小步长跟踪
    kStageReset = 1,       // 复位（硬件复位或 BLReset 触发）
    kStageNoisyReset = 2,  // 噪声期间复位（只有常噪声的 ALPS 机型会进入）
    kStageHold = 3,        // 保持：回到上一阶段
    kStageSmallSig = 4,    // 小信号：等超时回到无触摸
    kStageTouched = 5,     // 有触摸：不更新
    kStageLiftOff = 6,     // 抬起：等超时
    kStageAfterTouch = 7,  // 抬起后跟踪
    kStage8 = 8,
    kStageNegative = 9,    // 负信号跟踪
};

/// TsaPrpt::blPropertyFlags
inline constexpr uint32_t kPropSmallSig = 0x001;
inline constexpr uint32_t kPropTouched = 0x002;
inline constexpr uint32_t kPropReset = 0x010;
inline constexpr uint32_t kPropRefresh = 0x040;
inline constexpr uint32_t kPropNegative = 0x080;
inline constexpr uint32_t kPropDebounce = 0x100;

/// 计时器累加：0 起算为 1，此后每帧加帧间隔。
template <class T>
[[nodiscard]] inline T UpdateTimer(T timer) noexcept {
    return timer == 0 ? T{1} : static_cast<T>(timer + static_cast<T>(static_cast<int64_t>(Static().frameIntervalMs)));
}
/// 按帧间隔累加 / 递减。
template <class T>
inline void AddInterval(T& v) noexcept {
    v = static_cast<T>(v + static_cast<T>(static_cast<int64_t>(Static().frameIntervalMs)));
}
template <class T>
inline void SubInterval(T& v) noexcept {
    v = static_cast<T>(v - static_cast<T>(static_cast<int64_t>(Static().frameIntervalMs)));
}
/// 帧间隔按 T 的宽度取值。
template <class T>
[[nodiscard]] inline T Interval() noexcept {
    return static_cast<T>(static_cast<int64_t>(Static().frameIntervalMs));
}

// BaselineIIR.cpp
namespace Iir {
void CalcDiff();
void ResetDiff();
void Update(bool force, int16_t thold, int16_t step);
void Reset();
}  // namespace Iir

// BaselineRecal.cpp
namespace Recal {
void Init();
void Process();
}  // namespace Recal

// BaselineReset.cpp
namespace Reset {
void Init(bool clearLatch);
bool Process(bool reset);
bool CheckNumOverNoise();
bool IsTouchToBeProtected2(bool needTwo);
/// 与 stored 里存的 CMF 前 raw 比较：store 时只存下当前值。
bool IsRawChanged(int16_t* stored, bool store);
}  // namespace Reset

}  // namespace Solvers::Tsa::Core::Baseline
