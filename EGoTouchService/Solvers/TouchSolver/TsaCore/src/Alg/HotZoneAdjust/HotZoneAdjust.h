#pragma once
// 热区调整（原 HotZoneAdjust 模块）：上层下发应用控件的热区（g_hotZoneRect，显示坐标），点击落在某个热区的
// 扩展范围内时，把上报坐标按比例拉进热区；报点期间沿用同一平移量。抬起时统计点击是否命中热区。
// 只在 featureFlags 0x400000 打开、且不是输入法场景、屏幕未旋转时工作。
// 原代码另把统计拼成日志与 DMD 描述串，StrPrintf 在本机型被裁掉，描述串只会被清空。

#include <cstdint>

namespace Solvers::Tsa::Core::HotZoneAdjust {

/// 清空统计并记下统计开始时间。
void Init();
/// TouchReport 每个触点调用一次。
void Process(uint8_t curIdx, uint8_t prevIdx);
/// 结束一个统计周期：记下结束时间、清空 DMD 描述与点击统计、开始下一周期，返回 DMD 描述串。
const char* GenerateAndGetDmdDescription();

}  // namespace Solvers::Tsa::Core::HotZoneAdjust
