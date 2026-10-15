#pragma once
// 触点尺寸（原 src/Alg/TouchSize/TS.c）：按区域信号和与触点门限估算触点的尺寸（mm），以及传感器坐标与 mm、
// 显示像素之间的换算。传感器坐标是 Q8.8（一个传感器 256 份），换算系数由 ConstPrmt 按传感器间距推导。

#include <cstdint>

namespace Solvers::Tsa::Core::TouchSize {

/// 每帧给未抬起的触点估算尺寸（TouchUnit::sizeMm / udfpSizeMm）。
void Process();
/// 帧末：抬起的触点的尺寸（封顶 9 mm）作为下一个触点的默认尺寸。
void PostProcess();

/// mm → Q8.8 传感器单位。
[[nodiscard]] int MmToPixel(uint8_t mm);
/// mm → 显示像素（按较大的显示分辨率）。
[[nodiscard]] uint32_t MmToDisplayPixel(uint8_t mm);
/// Q8.8 传感器单位 → mm，四舍五入。
[[nodiscard]] int PixelToMm(uint16_t pixel);
/// 传感器单位的平方 → mm²；超过 0xffff 时为 0xffffffff。
[[nodiscard]] uint32_t SqPixelToSqMm(uint32_t sqPixel);
/// mm² → 传感器单位的平方。
[[nodiscard]] uint64_t SqMmToSqPixel(uint16_t sqMm);

}  // namespace Solvers::Tsa::Core::TouchSize
