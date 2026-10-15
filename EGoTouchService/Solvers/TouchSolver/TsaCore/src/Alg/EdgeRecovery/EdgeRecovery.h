#pragma once
// 边缘修正（原 ER 模块）：TE 之后按触点事件修正靠近逻辑边缘的轨迹。
//   - move-in：从屏外滑入的触点刚报按下时，把它的历史轨迹按首两点的步长向边缘外推补齐，让轨迹从边缘开始；
//     底边另有一种判定（从底部区域上划）。离边缘在修正区间内、但历史还不够时先压回按下防抖等一帧。
//   - move-out：报过点的触点抬起时，若按位移外推会越过逻辑边缘，把上报坐标贴到边缘上，并多保持一帧。

#include <cstdint>

namespace Solvers::Tsa::Core::EdgeRecovery {

/// 由 FlashPrmt::erEdgeMm 换算出 FlashConstPrmt::erEdge，并按上边初始化边缘宽度。
void Init();
void Process();
/// 最近一次 move-in 判定是否因所在边的修正区间为 0 而跳过。
[[nodiscard]] bool IsMoveInCorrectedButDisabled();
/// 最近一次判定所在边的边缘宽度（显示像素）。
[[nodiscard]] uint16_t GetLimit();

}  // namespace Solvers::Tsa::Core::EdgeRecovery
