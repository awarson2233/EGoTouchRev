#pragma once

#include "common/DeviceError.h"
#include "common/StylusState.h"
#include "himax/AfeTypes.h"
#include <cstdint>

namespace Himax {

class Chip;  // forward declaration

/// AfeController — AFE command dispatcher extracted from Himax::Chip.
///
/// Owns all thp_afe_* methods and the stylus lifecycle commands
/// (InitStylus, SetStylusId, DisconnectStylus, ProcessStylusStatus,
/// UpdateBtHeartbeat). Holds a non-owning reference to Chip for
/// low-level register access.
class AfeController {
public:
    explicit AfeController(Chip& chip) : m_chip(chip) {}

    /// Unified AFE command dispatcher (replaces Chip::afe_sendCommand)
    ChipResult<> SendCommand(command cmd);

    // ── AFE 模式控制 ───────────────────────────────────────────
    ChipResult<> EnterIdle(uint8_t param = 0);
    ChipResult<> ForceExitIdle();
    ChipResult<> StartCalibration(uint8_t param = 0);
    ChipResult<> EnableFreqShift();
    ChipResult<> DisableFreqShift();
    ChipResult<> ClearStatus(uint8_t cmd_val);
    ChipResult<> ForceToFreqPoint(uint8_t freq_idx);
    ChipResult<> ForceToScanRate(uint8_t rate_idx);

    // ── 手写笔生命周期管理 ─────────────────────────────────────
    ChipResult<> InitStylus(uint8_t pen_id = 5);
    ChipResult<> SetStylusId(uint8_t pen_id);
    ChipResult<> DisconnectStylus();

    /// 每帧调用：从 master suffix 读取噪声计数，判断是否需要切频
    /// 返回 true 表示需要发送 ForceToFreqPoint 命令
    bool ProcessStylusStatus();

    /// BT MCU 心跳注入：更新 BT 侧频率和时间戳
    void UpdateBtHeartbeat(uint8_t freq1, uint8_t freq2);

    // ── 状态访问 ───────────────────────────────────────────────
    StylusState& GetStylusState() { return m_stylus; }
    const StylusState& GetStylusState() const { return m_stylus; }

    /// 重置手写笔状态（由 Chip::Init 调用）
    void ResetStylusState() { m_stylus = StylusState{}; }

private:
    Chip& m_chip;
    StylusState m_stylus;  // 从 Chip 移入 — 手写笔运行时状态
};

} // namespace Himax
