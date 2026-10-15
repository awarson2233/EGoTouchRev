#pragma once
// 防误触（原 src/Rejection/AntiTouch）：按触点的尺寸、椭圆度、握持比与停留时间标出可疑触点
// （volatileFlags 0x4000000），在屏幕上部判出接近（stateFlags 0x80）；防误触模式（AFT）下把长期可疑的触点
// 坐标回退、暂不报点，直到它表现为正常手指再补报。

#include <cstdint>

namespace Solvers::Tsa::Core::AntiTouch {

void Init();
void Process();
/// 把接近 / 抬起状态报到 extFlags（0x1000000 有接近或抬起，0x2000000 解除）。
void Report();

}  // namespace Solvers::Tsa::Core::AntiTouch
