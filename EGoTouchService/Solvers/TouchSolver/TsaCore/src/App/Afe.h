#pragma once
// AFE 接口（原 src/Extension/TSA_AFE）：宿主送来的 AFE 能力与每帧状态，以及扫描频点 / 扫描率的选择
// （跳频与降扫描率，只在能力里声明由 TSACore 跳频时生效）。原版还有一套 AFE 自检模式（控制位 0x100000）
// 和宿主送来能力之前的模拟 AFE，宿主从不触发，没有移植。

#include <cstdint>

#include "../TsaCore.h"

namespace Solvers::Tsa::Core::Afe {

using Caps = AfeCaps;
using Frame = AfeFrame;

void Init();
/// 宿主的能力 / 帧拷进本模块的副本（帧里的矩阵与笔帧仍指向宿主的缓冲）。
void SetCaps(const Caps* caps);
void SetFrame(const Frame* frame);
void SetMaxScanRate(uint16_t rate);
[[nodiscard]] Frame* GetFrame();
[[nodiscard]] const Caps* GetCaps();
[[nodiscard]] uint32_t FrameStatus();

/// 本帧开始：按帧上报的频点 / 扫描率更新当前索引。
void PreProcess();
/// 本帧结束：手动跳频时决定下一个频点与扫描率。
void Process();
/// 宿主查询要切到的频点 / 扫描率索引：返回 false 时 *index 是要切到的索引。
[[nodiscard]] bool GetNextScanFreqIndex(uint8_t* index);
[[nodiscard]] bool GetNextScanRateIndex(uint8_t* index);

[[nodiscard]] uint16_t ColNum();
[[nodiscard]] uint16_t RowNum();
[[nodiscard]] bool IsInIdle();
[[nodiscard]] bool IsInSideIdle();
[[nodiscard]] bool IsProximityScreenOff();
[[nodiscard]] bool IsRecalRequested();
[[nodiscard]] bool IsCoreTouchDetected();
[[nodiscard]] bool IsRawJump();
[[nodiscard]] bool IsRawUnstable();
[[nodiscard]] bool IsManualFreqHopping();
[[nodiscard]] bool IsAutoCalibration();
[[nodiscard]] bool IsAllFreqNoisy();
[[nodiscard]] bool IsFreqShiftDone();
[[nodiscard]] bool IsFrequentlyHopping();

}  // namespace Solvers::Tsa::Core::Afe
