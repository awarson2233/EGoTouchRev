#pragma once
// 对外输出（原 src/App/TSA_Out.c）：报点前清掉不该报的触点并统计，以及内部 Q8.8 传感器坐标与
// 对外显示坐标、距离之间的换算。显示坐标 = 传感器坐标 × displayScale（Q16）− 裁剪量，按需翻转、
// 交换 XY，再乘 scale（1 为显示分辨率，outputResScale 为对外分辨率）。

#include <cstdint>

namespace Solvers::Tsa::Core::Out {

/// 报点之后：去掉未报过点 / 被抑制的触点，统计报出的点数，记录 extFlags 变化。
void PostProcess();

/// 触点 idx 的显示坐标（scale = 1）。
[[nodiscard]] uint16_t DisplayXPos(uint8_t idx);
[[nodiscard]] uint16_t DisplayYPos(uint8_t idx);
/// 触点 idx 的对外坐标（scale = outputResScale）。
[[nodiscard]] uint16_t RptXPos(uint8_t idx);
[[nodiscard]] uint16_t RptYPos(uint8_t idx);

/// 一个方向上传感器坐标 → 显示坐标。
[[nodiscard]] uint16_t Dim1PosInternal(uint16_t pos, uint16_t scale);
[[nodiscard]] uint16_t Dim2PosInternal(uint16_t pos, uint16_t scale);
[[nodiscard]] uint16_t Pixel2DisplayDim1(uint16_t pos);
[[nodiscard]] uint16_t Pixel2DisplayDim2(uint16_t pos);
/// 显示坐标 → 传感器坐标（四舍五入，限制在传感器范围内）。
[[nodiscard]] uint32_t RevertDim1Pos(uint16_t pos);
[[nodiscard]] uint32_t RevertDim2Pos(uint16_t pos);

/// 传感器距离 ↔ 显示距离（按 Dim1 比例）。
[[nodiscard]] uint32_t IntToExtDist(int16_t dist);
[[nodiscard]] uint64_t ExtToIntDist(uint16_t dist);
[[nodiscard]] uint16_t Axis(int16_t axis);
/// 内部角度 → 对外角度（按显示 X 轴是否与 Dim1 同向旋转 90° 或取反）。
[[nodiscard]] int8_t Angle(int8_t angle);

}  // namespace Solvers::Tsa::Core::Out
