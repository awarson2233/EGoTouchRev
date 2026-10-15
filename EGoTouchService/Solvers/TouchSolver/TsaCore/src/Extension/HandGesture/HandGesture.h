#pragma once
// 握持手势（原 src/Extension/HandGesture）：一只手握持屏幕边缘、另一处有关联的握持触点抬起时，以当时的 raw
// 为基线另算两侧边缘几列的 Dif，在手势数据集（DataSwitch::ToHand）上跑一遍峰检测到触点跟踪，得到手掌在边缘
// 的区域；结果记成每格的时间（TimeHand，供 TZ 判握持合并）与上一次的区域图（tzPrev，供 GripFilter 用）。
// 另外判断是否双手握持（Is2HandDetected）。

#include <cstdint>

#include "Common/Image.h"

namespace Solvers::Tsa::Core {

/// 0x6bba3500：握持手势的缓冲（每格一项）。
struct HandBuffers {
    int8_t* area;        // 关联握持触点所在区域覆盖的格
    int8_t* tz;          // 手势数据集的区域图
    int8_t* tzPrev;      // 上一次有效手势的区域图
    int16_t* time;       // 每格自上一次落在有效手势区域里以来的时间（ms，0x7fff 为无）
    int16_t* baseline;   // 手势基线：握持触点抬起那一帧的 raw
    int16_t* dif;        // 按手势基线算的 Dif（只算两侧边缘几列）
};

[[nodiscard]] inline HandBuffers& HandBufs() noexcept { return Image::At<HandBuffers>(0x6bba3500); }

}  // namespace Solvers::Tsa::Core

namespace Solvers::Tsa::Core::HandGesture {

[[nodiscard]] bool Malloc();
void Free();
void Init();
void Process();
[[nodiscard]] bool Is2HandDetected();

}  // namespace Solvers::Tsa::Core::HandGesture
