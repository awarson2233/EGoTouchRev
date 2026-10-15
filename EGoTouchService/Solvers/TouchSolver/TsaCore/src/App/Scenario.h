#pragma once
// 使用场景与外部特性开关（原 src/App/TSA_Scenario.c、TSA_Feature.c）：宿主告知的应用类型、通话 / 锁屏状态
// 决定当前场景（TsaStatic::aftGateFlags 与 scenarioMask），每帧按场景改写运行时参数与扫描率上限。

#include <cstdint>

namespace Solvers::Tsa::Core::Scenario {

/// TsaStatic::aftGateFlags 的场景位。
inline constexpr uint32_t kNormal = 0x1;
inline constexpr uint32_t kTyping = 0x2;
inline constexpr uint32_t kGame = 0x4;
inline constexpr uint32_t kTempCheck = 0x8;      // 查环境温度异常（手套判定的开关之一）
inline constexpr uint32_t kSpecialApp = 0x20;
inline constexpr uint32_t kNormalRateGame = 0x40;
inline constexpr uint32_t kVideo = 0x80;
inline constexpr uint32_t kInput = 0x100;

[[nodiscard]] bool IsTyping();
[[nodiscard]] bool IsJustEnterTyping();
[[nodiscard]] bool IsVideo();
/// 高帧率 / 普通帧率游戏（输入场景优先）。
[[nodiscard]] bool IsGame();
[[nodiscard]] bool IsPhoneCall();
[[nodiscard]] bool IsScreenLock();

/// 每帧（TtPrmt 之后）：按场景改写运行时参数、扫描率上限与手套开关，更新 scenarioMask。
void Process();
void Enable(uint32_t mask);
void Disable(uint32_t mask);

/// 宿主设置外部特性：手套、皮套、指关节、水下等可开关位按初值规则合并，再并上机型强制位。
/// 返回 1 表示请求里有不支持的位（bit3）。
uint32_t SetFeatures(uint32_t features);
[[nodiscard]] uint32_t GetFeatures();
/// 宿主告知屏幕旋转（0..3）；P167 / W157 机型的编号差一档。
void SetScreenRotateState(int state);

}  // namespace Solvers::Tsa::Core::Scenario
