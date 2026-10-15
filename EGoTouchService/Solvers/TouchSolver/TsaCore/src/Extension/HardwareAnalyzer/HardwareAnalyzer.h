#pragma once
// 硬件异常分析（原 Signal/HardwareAnalyzer.c，原厂 src/Extension/HardwareAnalyzer/）：每帧统计 raw2raw Dif、
// 查坏点与 raw 越限、AFE raw 跳变 / 不稳定等，把检出的异常记成位图（异常码见 HardwareAnalyzerInternal.h），
// 按优先级挑一个上报给宿主（DMD），并在 TouchTable::exceptionFlags 上置 0x400。
// 异常的文字描述靠 StrPrintf 拼写，本机型 StrPrintf 被裁掉，描述串只会被清空。

#include <cstdint>

namespace Solvers::Tsa::Core::HardwareAnalyzer {

[[nodiscard]] bool Malloc();
void Free();
void Init();
/// 清本帧的坏点表与按 Dif 查坏点的已查标记（每帧开头）。
void Reset();
/// raw 阶段：raw2raw 统计、按 raw 查坏点、射频噪声与 raw2raw 噪声。
void Process();
/// Dif 阶段：按 Dif 查坏点。
void ProcessDif();
/// 报点之后：raw 越限、AFE 状态、椭圆度、背光与耦合噪声等。
void PostProcess();
/// raw2raw Dif 的统计，写入 TsaPrpt::raw2Raw。
void GetRaw2RawPrpt();
/// 本帧有要上报的异常时置 TouchTable::exceptionFlags 0x400，否则清掉。
void ReportToTsa();
/// 记一次软件异常（断言失败）。
void SetSoftException();
/// 按优先级取本帧新出现、且已重复够次数的异常码，0 为无。
[[nodiscard]] int GetDetectedException();
/// Full Incell 下只有 raw2raw 噪声、或有大噪声 / 射频噪声时本帧不出点。
[[nodiscard]] bool IsToBypassCurrentFrame();
[[nodiscard]] const char* GetExceptionDescription();

}  // namespace Solvers::Tsa::Core::HardwareAnalyzer
