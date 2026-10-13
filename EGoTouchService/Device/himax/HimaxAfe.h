#pragma once

#include "common/DeviceError.h"
#include "common/StylusState.h"
#include "himax/AfeTypes.h"
#include <chrono>
#include <cstdint>
#include <mutex>

namespace Himax {

class Chip;  // forward declaration

/// AfeController — AFE command dispatcher extracted from Himax::Chip.
///
/// Owns all thp_afe_* methods and the stylus lifecycle commands
/// (InitStylus, SetStylusId, DisconnectStylus). Holds a non-owning
/// reference to Chip for low-level register access.
class AfeController {
public:
    explicit AfeController(Chip& chip) : m_chip(chip) {}

    /// Unified AFE command dispatcher (replaces Chip::afe_sendCommand)
    ChipResult<> SendCommand(command cmd);

    // ── AFE 模式控制 ───────────────────────────────────────────
    ChipResult<> EnterIdle(uint8_t param = 0);
    ChipResult<> ForceExitIdle();
    ChipResult<> StartCalibration(uint8_t param = 0);
    /// thp_afe_clear_status：让固件清掉帧尾里的完成标志（status = 0x04 跳频 / 0x08 校准）。
    ChipResult<> ClearStatus(uint8_t status);
    ChipResult<> ForceToScanRate(uint8_t rate_idx);
    /// thp_afe_force_to_freq_point：切到指定互容扫描频点（0 → 75，1 → 113）。
    ChipResult<> ForceToFreqPoint(uint8_t freq_idx);

    // ── 手写笔生命周期管理 ─────────────────────────────────────
    ChipResult<> InitStylus(uint8_t pen_id = 5);
    ChipResult<> SetStylusId(uint8_t pen_id);
    ChipResult<> DisconnectStylus();

    // ── 状态访问 ───────────────────────────────────────────────
    StylusState GetStylusStateSnapshot() const;

    /// 重置手写笔状态（由 Chip::Init/Deinit/Quiesce 调用）
    void ResetStylusState();

private:
    Chip& m_chip;
    mutable std::mutex m_stylusMutex;
    StylusState m_stylus;  // 从 Chip 移入 — 手写笔运行时状态
    std::chrono::steady_clock::time_point m_lastEnterIdleLog{};  // EnterIdle 日志节流
};

} // namespace Himax
