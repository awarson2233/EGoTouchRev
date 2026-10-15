#pragma once
// 报点判定（原 src/Rejection/TouchReport）：逐个触点决定本帧是否对外报点（persistFlags bit3），处理被姿态
// 判为误触后的坐标回退（一步回到按下 / 首报位置，或分 6 步平滑退回），以及历史缓冲的回放。

#include <cstdint>

namespace Solvers::Tsa::Core::TouchReport {

void Init();
void Process();
/// 把触点坐标退回按下 / 首报位置（开启多步回退且已判为姿态误触时分步退回）。
void GoBack(uint8_t curIdx, uint8_t prevIdx);

}  // namespace Solvers::Tsa::Core::TouchReport
