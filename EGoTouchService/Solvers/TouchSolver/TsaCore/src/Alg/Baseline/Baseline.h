#pragma once
// 基线（原 Baseline/*.c，原厂 src/Alg/Baseline/）：互容基线的跟踪、复位与 Dif 计算。
//
// 每帧由 TSA_MSProcess 调 Process：BaselineRecal 先看系统复位后的 raw 边界，BaselineStateMachine 按触点
// 与噪声情况得到本帧的基线属性（GetProperty，其中 BaselineReset 判断是否需要复位基线），推进阶段，
// 执行该阶段（IIR 步进、复位或保持），最后算出 Dif 并维护 SHB 抓取。

#include <cstdint>

namespace Solvers::Tsa::Core::Baseline {

// ── BaselineIIR ─────────────────────────────────────────────────────────
/// Dif = 基线 - raw（信号反向时反过来减）。
void CalcDiffCommon(int16_t* dif, const int16_t* raw, const int16_t* bl);
/// 由 Dif 还原 raw：raw = 基线 - Dif（信号反向时 Dif + 基线）。
void CalcRawCommon(int16_t* raw, const int16_t* bl, const int16_t* dif);

// ── BaselineStateMachine ────────────────────────────────────────────────
void Init(bool clearResetLatch);
void PreProcess();
void Process(bool hwReset, bool refresh);
/// raw 不稳定期间（AFE 状态 bit18）按固定步长拉近基线。
void UpdateForRawUnstable();
[[nodiscard]] bool IsReset();
[[nodiscard]] bool IsTouched();
[[nodiscard]] bool IsOkToIdle();
[[nodiscard]] bool IsBaselineOk();
[[nodiscard]] int16_t MaxSig();
[[nodiscard]] int16_t MinSig();

// ── BaselineReset ───────────────────────────────────────────────────────
[[nodiscard]] bool ResetInDebounce();
/// 本帧是否触发了基线复位（原样返回字节，调用方按 == 1 判断）。
[[nodiscard]] uint8_t ResetStatus();
void ClearRefreshRequest();
/// 触点所在峰的 raw 偏离基线比偏离 raw 参考值更远。
[[nodiscard]] bool CheckAbnormalTouch(uint8_t touchIdx);

// ── Baseline ────────────────────────────────────────────────────────────
/// 整屏偏移补偿：上下边界偏移超过 4 时整体平移基线 5。
void Compensation();

}  // namespace Solvers::Tsa::Core::Baseline
