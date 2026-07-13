#pragma once

#include "common/DeviceError.h"
#include "common/StylusState.h"
#include "himax/AfeTypes.h"
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
    // NOTE: ClearStatus command is currently not implemented in this refactor stage.
    // Callers must handle InvalidOperation until protocol semantics are fully defined.
    ChipResult<> ClearStatus(uint8_t cmd_val);
    ChipResult<> ForceToScanRate(uint8_t rate_idx);

    // ── 手写笔生命周期管理 ─────────────────────────────────────
    ChipResult<> InitStylus(uint8_t pen_id = 5);
    ChipResult<> SetStylusId(uint8_t pen_id);
    ChipResult<> DisconnectStylus();

    // ── 状态访问 ───────────────────────────────────────────────
    StylusState GetStylusStateSnapshot() const;

    /// 重置手写笔状态（由 Chip::Init/Deinit/HoldReset 调用）
    void ResetStylusState();

private:
    Chip& m_chip;
    mutable std::mutex m_stylusMutex;
    StylusState m_stylus;  // 从 Chip 移入 — 手写笔运行时状态
};

} // namespace Himax
