#pragma once
// 系统级运行状态的维护与查询（原 src/App/TSA_Static.c）：每帧开头从 AFE 取 IDLE / 接近等状态、
// 结尾把本帧状态存成“上一帧”，以及系统复位位、充电器、通话、IDLE 计时等判断。状态本身在 TsaStatic。

#include <cstdint>

namespace Solvers::Tsa::Core::Sys {

/// TsaStatic::systemFlags 的位。
inline constexpr uint32_t kSysResetMask = 0xf;          // 各种系统复位
inline constexpr uint32_t kSysFreqShiftDone = 0x10;
inline constexpr uint32_t kSysCharger = 0x20;
inline constexpr uint32_t kSysScreenRotated = 0x80;
inline constexpr uint32_t kSysChargerNoise = 0x20000;
inline constexpr uint32_t kSysCharger2 = 0x200000;
inline constexpr uint32_t kSysPhoneCall = 0x400000;

void Init();
void PreProcess();
void PostProcess();
/// 宿主下发的系统标志：处理复位位的保持与清除，再做通话防抖。
void UpdateCtrlFlags(uint32_t flags);
/// 外部特性开关恢复成初值（只动 mask 里属于 kFeatExtMask 的位）。
void RestoreExtFeatures(uint32_t mask);

[[nodiscard]] bool IsHwReset();
[[nodiscard]] bool IsHwResetPrev();
[[nodiscard]] uint64_t CurrTimeStamp();
[[nodiscard]] bool IsProximityEnabled();
[[nodiscard]] bool IsAftModeEnabled();
[[nodiscard]] bool IsSafeBaselineApplied();
[[nodiscard]] bool IsSignalReverted();
[[nodiscard]] bool IsFreqShiftDone();
[[nodiscard]] bool IsScreenRotated();
/// 充电器：标志在位，或最近一次检出不到 2 s。
[[nodiscard]] bool IsChargerPlugged();
/// 只看标志；在位时记下时间（有副作用）。
[[nodiscard]] bool IsChargerPluggedNoDebounce();
/// 更新并返回 TsaStatic::allFreqNoisy。
bool IsAllFreqNoisy();
[[nodiscard]] bool IsAllFreqBecomeNoisy();
[[nodiscard]] bool IsAfeJustEnterIdle();
[[nodiscard]] bool IsAfeJustExitIdle();
[[nodiscard]] bool IsAfeJustExitSideIdle();
[[nodiscard]] bool IsAfeEnterIdleTimeout();
[[nodiscard]] bool IsAfeEnterSideIdleTimeout();
[[nodiscard]] bool DoesAfeRequireToExitIdle();
[[nodiscard]] bool DoesAfeRequireToExitSideIdle();
[[nodiscard]] bool IsOkToEnterIdle();
[[nodiscard]] bool IsOkToEnterSideIdle();
/// mode 1：可信触点峰值跟踪值的 2/5；否则动态参数的 sigThold。
[[nodiscard]] uint32_t GetSigThold(int mode);
/// 报点之后：跟踪可信触点的峰值，再判 Dif 是否可信。
void ProcessAftTouchEvent();
void SafeDiffCheck();

}  // namespace Solvers::Tsa::Core::Sys
