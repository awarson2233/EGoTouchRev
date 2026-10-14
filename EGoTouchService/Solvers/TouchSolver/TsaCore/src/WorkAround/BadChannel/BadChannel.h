#pragma once
// 坏通道（原 Signal/BadChannel.c，原厂 src/WorkAround/BadChannel/BadChannel.c）：raw 落在异常区间的 4 行一组、
// 或某组行的 raw 均值明显低于其他组时，把这些格标为坏点（防抖 120 帧），每帧报给 TPSensor 的坏点表。

namespace Solvers::Tsa::Core::BadChannel {

[[nodiscard]] bool Malloc();
void Free();
void Init();
void Process();
/// 供 DMD 上报：用更宽的区间检查一次是否有坏通道。
[[nodiscard]] bool IsDmdDetected();

}  // namespace Solvers::Tsa::Core::BadChannel
