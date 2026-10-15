#pragma once
// 握持过滤（原 src/Extension/GripFilter）：逐个触点判断它是不是握持屏幕边缘的手掌 / 手指造成的，记成
// TouchUnit::gripStage（kStage*）。判定先算一组属性位（gripFlags 等），再按属性与边缘比例门限给出本帧的
// 质量（绝对非握持 / 无 / 临时握持 / 绝对握持），最后在历史阶段上迁移。握持区域图（g_tsaBufGripRegion）
// 按屏幕旋转与机型标出边缘、角落与手掌区域；死区按握持触点沿边缘标出不报点的范围。

#include <cstdint>

namespace Solvers::Tsa::Core::GripFilter {

/// TouchUnit::gripStage
inline constexpr int32_t kStageTbd = 0;       // 新触点，待定
inline constexpr int32_t kStageAbsGrip = 1;   // 绝对握持
inline constexpr int32_t kStageTempGrip = 2;  // 临时握持
inline constexpr int32_t kStageNormal = 3;    // 非握持，仍可回到握持
inline constexpr int32_t kStageSafe = 4;      // 安全触点（绝对非握持）

void Init();
/// 每帧在 TZ 之前按屏幕旋转、机型与握持手势重建握持区域图。
void RegionProcess();
void Process();

[[nodiscard]] bool IsGrip(int32_t stage);
[[nodiscard]] bool IsGripOrTbd(int32_t stage);
[[nodiscard]] bool IsGripTouch(uint8_t curIdx);
[[nodiscard]] bool IsGripOrTbdTouch(uint8_t curIdx);
/// 峰被 TZ 标为绝对握持 / 绝对非握持（屏幕网格的峰表）。
[[nodiscard]] bool IsPeakAbsGrip(uint8_t peakIdx);
[[nodiscard]] bool IsPeakAbsNotGrip(uint8_t peakIdx);
/// ID 跟踪时取消这对触点的匹配：握持触点合并后变成绝对非握持，或被判为并入了握持。
[[nodiscard]] bool IsToCancelIdMapping(uint8_t curIdx, uint8_t prevIdx);
[[nodiscard]] bool IsOkToEnterIdle();
[[nodiscard]] int16_t GetTrackingThold();
/// 格 (dim1, dim2) 所在的边缘开启了握持合并处理。
[[nodiscard]] bool IsMergeHandlerSensorEnabled(uint8_t dim1, uint8_t dim2);

}  // namespace Solvers::Tsa::Core::GripFilter
