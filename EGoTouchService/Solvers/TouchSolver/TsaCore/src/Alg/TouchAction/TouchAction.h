#pragma once
// 触点动作（原 TouchAction 模块）：决定每个触点按点击还是按滑动处理（TouchUnit::action，取值见 TouchData.h 的
// TouchAction 常量）。新触点沿用 DynamicPrmt::defaultTouchAction，它由上一个报过点的触点抬起时更新。

#include <cstdint>

namespace Solvers::Tsa::Core::TouchAction {

/// 每帧给未抬起的触点定动作。
void Process();
/// 帧末：报过点的触点抬起时，按它是否滑动过更新下一个触点的默认动作，并记录轻扫测试结果。
void PostProcess();
/// 轻扫测试未失败：此前没有任何报过点的触点在抬起时 persistFlags bit7 未置位、且离第一次报点不足 20 mm²。
[[nodiscard]] bool IsFlickCheckPassed();

}  // namespace Solvers::Tsa::Core::TouchAction
