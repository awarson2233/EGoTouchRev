#pragma once
// 对宿主的报点接口与几何换算（原 src/App/TSA_Rpt.c、TSA_Common.c）：报出的触点、显示分辨率，以及传感器
// 坐标落在哪条边（DimEdge：Dim1 近 / 远 1 / 2，Dim2 近 / 远 4 / 8）与屏幕哪一侧（Side，按 Dim1 是否在长边换算）。

#include <cstdint>

namespace Solvers::Tsa::Core::Report {

[[nodiscard]] uint8_t TouchNum();
/// 触点 ID（从 0 起）。
[[nodiscard]] int TouchId(uint8_t idx);
[[nodiscard]] uint16_t TouchXPos(uint8_t idx);
[[nodiscard]] uint16_t TouchYPos(uint8_t idx);
[[nodiscard]] uint16_t TouchPressure(uint8_t idx);
/// 本帧按下 / 抬起的事件位（非零即是）。
[[nodiscard]] uint32_t IsTouchDown(uint8_t idx);
[[nodiscard]] uint32_t IsLiftOff(uint8_t idx);
[[nodiscard]] uint8_t LiftOffNum();
/// 触点中心所在的格（显示方向），以及 Y 方向的握持比。
[[nodiscard]] uint16_t TouchXCenter(uint8_t idx);
[[nodiscard]] uint16_t TouchYCenter(uint8_t idx);
[[nodiscard]] uint8_t TouchYGripRatio(uint8_t idx);
/// 把显示坐标 x 写回触点的报点坐标（裁剪时用）。
void SetXPos(uint8_t idx, uint16_t x);

/// Grid / 侧边触点表里触点中心所在的格。
[[nodiscard]] uint16_t CoreTouchDim1Center(uint8_t idx);
[[nodiscard]] uint16_t CoreTouchDim2Center(uint8_t idx);
[[nodiscard]] uint16_t SideTouchDim1Center(uint8_t idx);
[[nodiscard]] uint16_t SideTouchDim2Center(uint8_t idx);

/// 显示分辨率（Dim1 / Dim2 按 IsDim1DisplayX 对应到 X / Y）。改分辨率后重算常量参数。
[[nodiscard]] uint16_t XRes();
[[nodiscard]] uint16_t YRes();
void SetXRes(uint16_t res);
void SetYRes(uint16_t res);
void SetResScale(uint16_t scale);
/// 显示 Y 方向的传感器格数。
[[nodiscard]] uint8_t YLength();

/// 最近的一条边（DimEdge）。
[[nodiscard]] uint32_t GetDimEdge(uint16_t dim1, uint16_t dim2);
/// DimEdge ↔ Side：Dim1 不在长边时两个方向对调；其余值为 0。
[[nodiscard]] uint32_t DimEdgeToSide(uint32_t dimEdge);
[[nodiscard]] uint32_t SideToDimEdge(uint32_t side);
/// Side 位 → 边的序号（1→0、2→1、4→2、8→3）。
[[nodiscard]] uint8_t SideToEdge(uint32_t side);
[[nodiscard]] uint32_t GetSide(uint16_t dim1, uint16_t dim2);
[[nodiscard]] uint32_t GetTouchSide(uint8_t idx);

[[nodiscard]] bool IsUnderWaterDetected();
[[nodiscard]] bool IsOutOfWaterDetected();
/// 同期统计里水下区间的格数超过 7/8 屏。
[[nodiscard]] bool IsPossibleUnderWater();
[[nodiscard]] bool IsPadFeatureEnabled();
[[nodiscard]] bool IsWindowsPadFeatureEnabled();

}  // namespace Solvers::Tsa::Core::Report
