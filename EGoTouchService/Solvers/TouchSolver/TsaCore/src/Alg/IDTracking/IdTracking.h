#pragma once
// 触点编号（原 src/Alg/IDTracking/IDT.c）：把当前表的触点与历史表的触点配对，当前触点继承配上的历史 ID，
// 没配上的 ID 清零（由 TE 当作新触点分配）。
//   1. 代价矩阵：位置差的平方；位移小、外推也小、轨迹够老的配对直接锚定为极小代价，在加速度门限下
//      一对一相容的候选配对同样锚定；
//   2. 匈牙利算法求总代价最小的配对；
//   3. 加速度校验：加速度超门限、GripFilter 要求取消或历史触点禁止配对的，拆掉。
// 峰检测也借它给峰编号（strict 模式：加速度门限一律放大 4 倍）。

namespace Solvers::Tsa::Core::IdTracking {

void Process(bool strictAccCheck);

}  // namespace Solvers::Tsa::Core::IdTracking
