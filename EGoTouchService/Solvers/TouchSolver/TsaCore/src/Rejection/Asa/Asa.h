#pragma once
// 主动笔（原 src/Rejection/ASA、ASAHostBridge）：笔模式状态机、蓝牙状态与本帧的笔报告。
//
// ASA 解算本体没有还原：笔的坐标 / 压力 / 倾角由业务侧的 StylusSolver 解出，经 TSAHost_SetStylusReport
// 在每帧之前送进来，这里只保留手指管线要读的那一层（笔在场、HPP2 / HPP3 模式、笔来时清掉触点）。

#include <cstdint>

namespace Solvers::Tsa::Core::Asa {

/// 装入笔参数（flash 是 ASA 参数表），清空笔状态与报告。
void InitAfeProject(const void* caps, const uint8_t* flash);
/// 每帧（ASA 开启时）：按 AFE 笔帧更新笔模式与报告；笔刚出现时按需清掉手指触点。ctrl 低 4 位非零时复位。
void Process(uint32_t ctrl);

/// 本帧报告：笔在场、坐标（ASA 坐标系，量程 Dim1Res / Dim2Res）、倾角（度）与压力。
[[nodiscard]] bool RptInRange();
[[nodiscard]] uint16_t RptX();
[[nodiscard]] uint16_t RptY();
[[nodiscard]] int16_t RptTiltX();
[[nodiscard]] int16_t RptTiltY();
[[nodiscard]] uint16_t RptPressure();
[[nodiscard]] uint16_t Dim1Res();
[[nodiscard]] uint16_t Dim2Res();

/// AFE 报告笔以 HPP2 / HPP3 协议在场（且已进入主动笔模式）。
[[nodiscard]] bool IsHpp2();
[[nodiscard]] bool IsHpp3();

}  // namespace Solvers::Tsa::Core::Asa
