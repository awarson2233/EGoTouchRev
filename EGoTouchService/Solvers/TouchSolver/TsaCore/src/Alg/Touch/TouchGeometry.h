#pragma once
// 触点模块用到的距离、边缘距离与时间间隔换算（原 src/Alg/Touch/TouchData.c 中不涉及触点状态的部分）。
// 坐标都是 Q8.8 传感器坐标；mm 换算经 TS 模块。

#include <cstdint>

namespace Solvers::Tsa::Core::Touch {

/// 两点距离的平方（Q8.8 像素²），溢出时饱和到 0xffffffff。
[[nodiscard]] uint32_t GetSqDistPixel(uint16_t dim1A, uint16_t dim2A, uint16_t dim1B, uint16_t dim2B);
/// 两点距离的平方换算成 mm²。
[[nodiscard]] uint16_t GetSqDistMm(uint16_t dim1A, uint16_t dim2A, uint16_t dim1B, uint16_t dim2B);
/// 从 (dim1, dim2) 沿 (toDim1, toDim2) → (dim1, dim2) 的方向反向外推到逻辑边缘，返回外推点到起点的距离平方。
[[nodiscard]] int GetSqDistPixelToEdge(int16_t dim1, int16_t dim2, int16_t toDim1, int16_t toDim2);
/// 到指定逻辑边缘（1 / 2：Dim1 近 / 远端，4 / 8：Dim2 近 / 远端）的距离；其他值为 0。
[[nodiscard]] int16_t GetDistToLogicEdge(int edge, int16_t dim1, int16_t dim2);
/// 到 Dim1 较近一端逻辑边缘的距离。越过近端时按无符号回绕，与原代码一致。
[[nodiscard]] uint32_t GetDim1EdgeDist(uint16_t dim1);
[[nodiscard]] uint32_t GetDim2EdgeDist(uint16_t dim2);
/// 补报轨迹的点数：起止两点在主方向上每 2 mm 一个点，限制在 2..5。
[[nodiscard]] uint32_t GetPostReportLength(int startDim1, int startDim2, int endDim1, int endDim2);
/// 把两帧时间戳之差整理成可用的帧间隔（ms）：0 按报点率推算，过小按 ±8，过大限制在 ±1000。
[[nodiscard]] int16_t GetFormattedTimeInterval(int16_t dt);
/// 7/8 IIR，结果再向当前值靠拢 1。
[[nodiscard]] int IirProcess(int previous, int current);

}  // namespace Solvers::Tsa::Core::Touch
