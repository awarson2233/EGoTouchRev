#pragma once
// 触点门限（原 TouchThold 模块）：每帧由 DynPrmt 按 finger / 手套门限算出峰门限、触点门限与按下门限，
// 并按噪声触点数与 SNR 换算噪声门限（写入 TsaPrpt::noiseThold）。

#include <cstdint>

namespace Solvers::Tsa::Core::TouchThold {

void Init();
/// 由 finger / 手套门限算出峰门限、触点门限与按下门限。
void Process(int16_t fingerThold, int16_t gloveThold, int16_t* peakThold, int16_t* touchThold, int16_t* touchDownThold);
/// TsaPrpt::tholdSwitching（Init 清零）。
[[nodiscard]] bool IsSwitching();
/// 脚趾（边缘）区域使用特殊门限的机型：P116 / P150 / P186。
[[nodiscard]] bool ToeUseSpecialThold();
/// 触点门限与基线噪声门限取 finger 门限（否则取手套门限）：TsaPrpt::touchModeLatched 为 0 或手套 / 指关节模式都未生效，
/// 且无智能皮套、BLReset 未置 wakeUpExceptionLatch、无整屏信号失衡。
[[nodiscard]] bool UseFingerThold();

}  // namespace Solvers::Tsa::Core::TouchThold
