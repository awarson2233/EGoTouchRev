#pragma once
// 坐标滤波（原 Filter/Filter.c）：报点前对每个触点的位置做三级处理，结果依次写进 TouchUnit 的 iirPos、jitPos、rptPos。
//
//   pos ──► 点击锁定 / 自适应 IIR ──► iirPos ──► 抗抖动（JIT）──► jitPos ──► rptPos
//
// - 未报过点（或刚重新开始报点）的触点不滤波，直接取 pos。
// - 点击抑制：点击类触点按下后坐标锁在按下点，转为滑动后把锁定期间积累的偏移逐帧收回。
// - 自适应 IIR：权重随帧间位移自适应（AIIR），大物体 / 异常时用重系数组，stylus 模式用另一组；
//   轨迹识别出折返时按速度外推几帧；抬起过程中信号骤降时直接取当前值。
// - 抗抖动：位移不超过抗抖动半径时保持上一帧坐标，超过时只跟到距当前值一个半径处。

#include <cstdint>

namespace Solvers::Tsa::Core::Filter {

/// 当前帧触点 curIdx（对应历史触点 prevIdx）的坐标滤波。
void ProcessUnit(uint8_t curIdx, uint8_t prevIdx);

}  // namespace Solvers::Tsa::Core::Filter
