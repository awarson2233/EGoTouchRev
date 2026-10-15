#pragma once
// 触点事件（原 src/Alg/TouchEvent/TE_Process.c）：IDT 编号之后，按历史决定每个触点本帧的 TouchEvent
// （状态值见 TouchData.h 的 TouchEvent 常量）：
//   - 历史里有、本帧没匹配上的触点进入抬起流程：先抬起防抖，再报抬起（WindowsPad 下另有一帧保持）；
//   - 新触点分配 ID 并在历史表里开一项，经按下防抖后报按下；
//   - 已报按下的触点进入移动。
// 然后检查当前表与历史表的下标是否一一对应，不一致时两张表都清空。

#include "Alg/Touch/TouchData.h"

namespace Solvers::Tsa::Core::TouchEvent {

void Process();

}  // namespace Solvers::Tsa::Core::TouchEvent
