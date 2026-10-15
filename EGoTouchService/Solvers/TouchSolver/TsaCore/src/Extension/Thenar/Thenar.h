#pragma once
// 大鱼际 / 侧握手掌检测（原 src/Rejection/Thenar）：沿屏幕两条侧边扫 Dif，把超过门限的连续段记成
// ThenarPoint，按两边短段的个数、段间距与最长段判断手掌贴在哪一侧，再按段的信号强度判断手掌手势；
// 持续判在同一侧时置 extFlags 0x2 / 0x4（带手势时再置 0x8 / 0x10）。

#include <cstdint>

namespace Solvers::Tsa::Core {
struct ThenarInfo;
}

namespace Solvers::Tsa::Core::Thenar {

void Init();
/// 本帧检测并更新侧握计数，结果存入历史表。
void ProcessInfoByDif();
/// 把检测结果报到 ThenarExternal。
void UpdateExternalInfo();
/// 两帧之间拷贝大鱼际信息（确认时间不拷）。
void CopyInfo(ThenarInfo& dst, const ThenarInfo& src);

}  // namespace Solvers::Tsa::Core::Thenar
