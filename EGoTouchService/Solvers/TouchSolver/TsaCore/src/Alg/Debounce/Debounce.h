#pragma once
// 按下 / 抬起防抖（原 DB 模块）：TE 按这里给出的帧数决定触点要连续出现（消失）多少帧才对外报按下（抬起）。
// kNoTouchDown 表示本帧不允许按下。

#include <cstdint>

namespace Solvers::Tsa::Core::Debounce {

inline constexpr uint16_t kNoTouchDown = 0xffff;
/// TouchUnit::persistFlags：按下防抖期间判出的快速点击（抬起时可进入 kLiftOffHold）。
inline constexpr uint32_t kPersistFastClick = 0x40000;

/// 新触点的按下防抖帧数：按区域异常、峰门限、信号增长、侧边、噪声等逐项取较大值，任一否决条件成立时为 kNoTouchDown。
[[nodiscard]] uint16_t TouchDownNum(uint8_t prevIdx, uint8_t curIdx);
/// 报过点的触点的抬起防抖帧数。
[[nodiscard]] uint16_t LiftOffNum(uint8_t prevIdx);
/// 大物体的面积门限（mm²）：largeObjectSizeMm²，历史表上一帧有对外报抬起的触点（liftOffCount 非零）时取 3/4。
[[nodiscard]] uint16_t LargeObjectSize();

}  // namespace Solvers::Tsa::Core::Debounce
