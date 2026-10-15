#pragma once
// Toe 机型的基线工作区（原 Baseline/ToeSynaBl.c、ToeSyna.c，原厂 src/WorkAround/ToeSynaBl/）：
// 两侧列的 raw 受保护时把侧边排除在基线判定之外，并补充背光偏移、水、异常触点形状、握持区域等基线异常判定。
// 由 FlashPrmt::toeSynaBl 开关，各项判定再由 toeSynaBlFlags 的位开关。

#include <cstdint>

namespace Solvers::Tsa::Core::ToeSynaBl {

[[nodiscard]] bool Malloc();
void Free();
/// 每帧判断两侧列是否要排除（excludeSide）。
void GetSidePrpt();

/// 排除侧边时，基线状态机看的正 / 负向极值改用不含侧边的统计。
[[nodiscard]] int16_t PropertyMaxSig(int16_t v);
[[nodiscard]] int16_t PropertyMinSig(int16_t v);

/// 背光偏移：大部分列沿 Dim2 的 Dif 平缓（升降的格数都不到 percent%）。
[[nodiscard]] bool BackLightShiftUnit(uint16_t percent);
[[nodiscard]] bool CheckNumOverNoise();
[[nodiscard]] bool IsBaselineAbnormalAftScreenOn();
/// 补充的基线异常判定，成立时置 abnormal。
void IsBaselineAbnormal(bool skipShape, bool protected2, bool& abnormal);
[[nodiscard]] bool IsTouchToBeProtected2(bool needTwo, bool fallback);
[[nodiscard]] bool IsWaterDetected();

}  // namespace Solvers::Tsa::Core::ToeSynaBl
