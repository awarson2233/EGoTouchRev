#include "himax/HimaxAfe.h"
#include "himax/HimaxChip.h"
#include "himax/HimaxProtocol.h"
#include "Logger.h"

namespace {

const char* AfeCommandToString(AFE_Command cmd) {
    switch (cmd) {
    case AFE_Command::ClearStatus:       return "ClearStatus";
    case AFE_Command::StartCalibration:  return "StartCalibration";
    case AFE_Command::EnterIdle:         return "EnterIdle";
    case AFE_Command::ForceExitIdle:     return "ForceExitIdle";
    case AFE_Command::ForceToScanRate:   return "ForceToScanRate";
    case AFE_Command::InitStylus:        return "InitStylus";
    case AFE_Command::SetStylusId:       return "SetStylusId";
    case AFE_Command::DisconnectStylus:  return "DisconnectStylus";
    default:                             return "Unknown";
    }
}

} // anonymous namespace

namespace Himax {

// ── AFE 命令分发器 ──────────────────────────────────────────────────────────

ChipResult<> AfeController::SendCommand(command cmd) {
    LOG_INFO("HimaxAFE", __func__, m_chip.GetStateStr(), "Dispatch cmd={}({}), param={}", AfeCommandToString(cmd.type), static_cast<int>(cmd.type), static_cast<unsigned int>(cmd.param));

    switch (cmd.type) {
    case AFE_Command::ClearStatus:       return ClearStatus(cmd.param);
    case AFE_Command::StartCalibration:  return StartCalibration(cmd.param);
    case AFE_Command::EnterIdle:         return EnterIdle(cmd.param);
    case AFE_Command::ForceExitIdle:     return ForceExitIdle();
    case AFE_Command::ForceToScanRate:   return ForceToScanRate(cmd.param);
    case AFE_Command::InitStylus:        return InitStylus(cmd.param);
    case AFE_Command::SetStylusId:       return SetStylusId(cmd.param);
    case AFE_Command::DisconnectStylus:  return DisconnectStylus();
    default:
        return std::unexpected(ChipError::InvalidOperation);
    }
}

// ── AFE 模式控制 ────────────────────────────────────────────────────────────

ChipResult<> AfeController::EnterIdle(uint8_t param) {
    if (m_chip.GetConnectionState() != ConnectionState::Connected)
        return std::unexpected(ChipError::InvalidOperation);

    LOG_INFO("HimaxAFE", __func__, m_chip.GetStateStr(), "Entering with param={}", static_cast<unsigned>(param));

    if (auto res = HimaxProtocol::send_command(m_chip.GetMasterDevice(), 0x0a, param, m_chip.GetCurrentSlot()); !res) {
        LOG_ERROR("HimaxAFE", __func__, m_chip.GetStateStr(), "Send ENTER_IDLE command failed!");
        return res;
    }

    if (auto res = m_chip.SetFrameReadIdlePolicy(); !res) return res;
    m_chip.SetAfeMode(THP_AFE_MODE::Idle);

    LOG_INFO("HimaxAFE", __func__, m_chip.GetStateStr(), "===== IDLE ENTER ===== No input detected, entering low-power idle.");
    return {};
}

ChipResult<> AfeController::ForceExitIdle() {
    if (m_chip.GetConnectionState() != ConnectionState::Connected)
        return std::unexpected(ChipError::InvalidOperation);

    LOG_INFO("HimaxAFE", __func__, m_chip.GetStateStr(), "Entering!");
    auto res = m_chip.NotifyTouchWakeup();
    LOG_INFO("HimaxAFE", __func__, m_chip.GetStateStr(), "Out!");
    return res;
}

ChipResult<> AfeController::StartCalibration(uint8_t param) {
    if (m_chip.GetConnectionState() != ConnectionState::Connected)
        return std::unexpected(ChipError::InvalidOperation);

    LOG_INFO("HimaxAFE", __func__, m_chip.GetStateStr(), "Entering with param={}", static_cast<unsigned>(param));

    if (auto res = HimaxProtocol::send_command(m_chip.GetMasterDevice(), 0x01, param, m_chip.GetCurrentSlot()); !res) {
        LOG_ERROR("HimaxAFE", __func__, m_chip.GetStateStr(), "Send AFE_START_CALBRATION command failed!");
        return res;
    }

    LOG_INFO("HimaxAFE", __func__, m_chip.GetStateStr(), "Out!");
    return {};
}

ChipResult<> AfeController::ClearStatus(uint8_t cmd_val) {
    if (m_chip.GetConnectionState() != ConnectionState::Connected)
        return std::unexpected(ChipError::InvalidOperation);

    // WS-2.5 stage: this command path is intentionally explicit about being unsupported
    // until full protocol semantics are verified and implemented.
    LOG_WARN("HimaxAFE", __func__, m_chip.GetStateStr(),
             "ClearStatus is currently unsupported/incomplete (param=0x{:02X})",
             static_cast<unsigned>(cmd_val));
    return std::unexpected(ChipError::InvalidOperation);
}

ChipResult<> AfeController::ForceToScanRate(uint8_t rate_idx) {
    if (m_chip.GetConnectionState() != ConnectionState::Connected)
        return std::unexpected(ChipError::InvalidOperation);

    LOG_INFO("HimaxAFE", __func__, m_chip.GetStateStr(), "Entering with rate_idx={}", static_cast<unsigned>(rate_idx));
    return HimaxProtocol::send_command(m_chip.GetMasterDevice(), 0x0e, rate_idx, m_chip.GetCurrentSlot());
}

// ── 手写笔生命周期 ──────────────────────────────────────────────────────────

ChipResult<> AfeController::InitStylus(uint8_t pen_id) {
    if (m_chip.GetConnectionState() != ConnectionState::Connected)
        return std::unexpected(ChipError::InvalidOperation);

    m_stylus.connected = true;
    m_stylus.pen_id = pen_id;

    LOG_INFO("HimaxAFE", __func__, m_chip.GetStateStr(), "Stylus connected, pen_id={}", static_cast<unsigned>(pen_id));
    return {};
}

ChipResult<> AfeController::SetStylusId(uint8_t pen_id) {
    if (m_chip.GetConnectionState() != ConnectionState::Connected)
        return std::unexpected(ChipError::InvalidOperation);

    m_stylus.pen_id = pen_id;

    LOG_INFO("HimaxAFE", __func__, m_chip.GetStateStr(),
             "pen_id={}", static_cast<unsigned>(pen_id));
    return {};
}

ChipResult<> AfeController::DisconnectStylus() {
    if (m_chip.GetConnectionState() != ConnectionState::Connected)
        return std::unexpected(ChipError::InvalidOperation);

    LOG_INFO("HimaxAFE", __func__, m_chip.GetStateStr(), "Reset StylusState");
    m_stylus = StylusState{};
    return {};
}

} // namespace Himax
