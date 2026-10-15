#pragma once
// 笔防误触（原 src/Rejection/StylusAft）：书写时把手掌、笔附近的手指撤下（withdraw，整条轨迹不再报）或
// 压住若干帧（suppress）。判据是触点的手掌等级（最大长轴、椭圆度）、离笔尖的扇形区域、与其它手掌的距离、
// 贴边程度，以及蓝牙报告的笔状态。

namespace Solvers::Tsa::Core::StylusAft {

void Init();
/// 每帧（ASA 开启且未处于游戏场景时）：记下笔的位置与倾角，再逐个处理未报抬起的触点。
void Process();

}  // namespace Solvers::Tsa::Core::StylusAft
