#pragma once
// 全局异常（水下、噪声、湿手指、基线异常等）的状态机。TSA 主流程每帧先做水下检测（CheckUnderWater），
// 再由 TZ 逐区域检出区域异常（ProcessTouchZone），最后 Process 把本帧检出的异常消抖成
// TouchTable::exceptionFlags。检测状态按传感器架构分 Hybrid Incell 与 Full Incell 两组
// （TsaStatic::hiException / fiException）；本机型的 Hybrid Incell 检测（HIException）被裁掉。

#include <cstdint>

namespace Solvers::Tsa::Core {
struct TouchZoneUnit;
}

namespace Solvers::Tsa::Core::Exception {

/// 异常字（TouchTable::exceptionFlags、TouchZoneUnit::exceptionType）的取值。
inline constexpr uint32_t kWetFinger = 0x80;           // 湿手指
inline constexpr uint32_t kNoiseLevel2 = 0x100;        // 噪声，noiseLevel 置 2
inline constexpr uint32_t kAbnormalBaseline = 0x200;   // 基线异常
inline constexpr uint32_t kExitedSpecialCtrl = 0x1000; // 刚退出特殊控制模式
inline constexpr uint32_t kUnderWater = 0x2000;        // 水下
inline constexpr uint32_t kUnderWaterReverted = 0x4000;// 水下且信号反向
inline constexpr uint32_t kNoiseLevel3 = 0x8000;       // 噪声，noiseLevel 置 3

[[nodiscard]] bool Malloc();
void Free();
void Init();

/// 统计 Dif 落在水下门限区间内的格数，可能处于水下时把本帧检出的异常记为水下。
void CheckUnderWater();
/// 一个触摸区域的区域异常检测（Full Incell 时）。
void ProcessTouchZone(TouchZoneUnit& zone);
/// 由本帧检出的异常更新全局异常字、噪声等级与手套相关的动态参数。
void Process();

}  // namespace Solvers::Tsa::Core::Exception
