#pragma once
// 触控模式（原 Filter/TouchMode.c，原厂 src/Alg/TouchMode/TouchMode_Process.c）：在无物体、手指、手套、
// 指关节（stylus）四种模式间切换，并按当前模式给触点打抑制标记（suppressFlags）。
//
// 每帧从上一帧的模式出发，按切换优先级表依次试各模式的 Detect；第一个检出（detect = 2）的模式胜出：
// 与上一帧相同时直接执行它的 Process；不同时先按切换防抖表计时，防抖未到就退回上一帧的模式。
// 结果记在 TsaPrpt 的 touchMode / touchModeLevel / touchModeDetect。

#include <cstdint>

namespace Solvers::Tsa::Core::TouchMode {

void Init();
void Process();

}  // namespace Solvers::Tsa::Core::TouchMode
