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
    case AFE_Command::ForceToFreqPoint:  return "ForceToFreqPoint";
    default:                             return "Unknown";
    }
}

} // anonymous namespace

namespace Himax {

// ── AFE 命令分发器 ──────────────────────────────────────────────────────────

ChipResult<> AfeController::SendCommand(command cmd) {
    // IC 不肯进 IDLE 时 TSACore 每 5 帧会再请求一次 EnterIdle，它的日志由 EnterIdle 自己节流
    if (cmd.type != AFE_Command::EnterIdle) {
        LOG_INFO("HimaxAFE", __func__, m_chip.GetStateStr(), "Dispatch cmd={}({}), param={}", AfeCommandToString(cmd.type), static_cast<int>(cmd.type), static_cast<unsigned int>(cmd.param));
    }

    switch (cmd.type) {
    case AFE_Command::ClearStatus:       return ClearStatus(cmd.param);
    case AFE_Command::StartCalibration:  return StartCalibration(cmd.param);
    case AFE_Command::EnterIdle:         return EnterIdle(cmd.param);
    case AFE_Command::ForceExitIdle:     return ForceExitIdle();
    case AFE_Command::ForceToScanRate:   return ForceToScanRate(cmd.param);
    case AFE_Command::InitStylus:        return InitStylus(cmd.param);
    case AFE_Command::SetStylusId:       return SetStylusId(cmd.param);
    case AFE_Command::DisconnectStylus:  return DisconnectStylus();
    case AFE_Command::ForceToFreqPoint:  return ForceToFreqPoint(cmd.param);
    default:
        return std::unexpected(ChipError::InvalidOperation);
    }
}

// ── AFE 模式控制 ────────────────────────────────────────────────────────────

ChipResult<> AfeController::EnterIdle(uint8_t param) {
    if (m_chip.GetConnectionState() != ConnectionState::Connected)
        return std::unexpected(ChipError::InvalidOperation);

    if (auto res = HimaxProtocol::send_command(m_chip.GetMasterDevice(), 0x0a, param, m_chip.GetCurrentSlot()); !res) {
        LOG_ERROR("HimaxAFE", __func__, m_chip.GetStateStr(), "Send ENTER_IDLE command failed!");
        return res;
    }
    // thp_afe_enter_idle：命令发出去就按 IDLE 的方式读帧；IC 进没进 IDLE 以之后帧尾的 word0 为准
    m_chip.ArmIdleRetry();

    const auto now = std::chrono::steady_clock::now();
    if (now - m_lastEnterIdleLog >= std::chrono::seconds{1}) {
        m_lastEnterIdleLog = now;
        LOG_INFO("HimaxAFE", __func__, m_chip.GetStateStr(), "ENTER_IDLE sent (param={}).", static_cast<unsigned>(param));
    }
    return {};
}

ChipResult<> AfeController::ForceExitIdle() {
    if (m_chip.GetConnectionState() != ConnectionState::Connected)
        return std::unexpected(ChipError::InvalidOperation);

    // thp_afe_force_exit_idle。原厂宿主平时不发它（IC 被触摸唤醒是自己退出 IDLE 的），只留给外部命令用
    if (auto res = HimaxProtocol::send_command(m_chip.GetMasterDevice(), 0x0b, 0, m_chip.GetCurrentSlot()); !res) {
        LOG_ERROR("HimaxAFE", __func__, m_chip.GetStateStr(), "Send FORCE_EXIT_IDLE command failed!");
        return res;
    }
    return {};
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

ChipResult<> AfeController::ClearStatus(uint8_t status) {
    if (m_chip.GetConnectionState() != ConnectionState::Connected)
        return std::unexpected(ChipError::InvalidOperation);

    LOG_INFO("HimaxAFE", __func__, m_chip.GetStateStr(), "Entering with status=0x{:02X}", static_cast<unsigned>(status));
    return HimaxProtocol::send_command(m_chip.GetMasterDevice(), 0x06, status, m_chip.GetCurrentSlot());
}

ChipResult<> AfeController::ForceToFreqPoint(uint8_t freq_idx) {
    if (m_chip.GetConnectionState() != ConnectionState::Connected)
        return std::unexpected(ChipError::InvalidOperation);
    // 本机型只有两个互容频点
    if (freq_idx >= 2)
        return std::unexpected(ChipError::InvalidOperation);

    LOG_INFO("HimaxAFE", __func__, m_chip.GetStateStr(), "Entering with freq_idx={}", static_cast<unsigned>(freq_idx));
    return HimaxProtocol::send_command(m_chip.GetMasterDevice(), 0x0c, freq_idx, m_chip.GetCurrentSlot());
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

    {
        std::lock_guard<std::mutex> lock(m_stylusMutex);
        m_stylus.connected = true;
        m_stylus.pen_id = pen_id;
    }

    LOG_INFO("HimaxAFE", __func__, m_chip.GetStateStr(), "Stylus connected, pen_id={}", static_cast<unsigned>(pen_id));
    return {};
}

ChipResult<> AfeController::SetStylusId(uint8_t pen_id) {
    if (m_chip.GetConnectionState() != ConnectionState::Connected)
        return std::unexpected(ChipError::InvalidOperation);

    {
        std::lock_guard<std::mutex> lock(m_stylusMutex);
        m_stylus.pen_id = pen_id;
    }

    LOG_INFO("HimaxAFE", __func__, m_chip.GetStateStr(),
             "pen_id={}", static_cast<unsigned>(pen_id));
    return {};
}

ChipResult<> AfeController::DisconnectStylus() {
    if (m_chip.GetConnectionState() != ConnectionState::Connected)
        return std::unexpected(ChipError::InvalidOperation);

    LOG_INFO("HimaxAFE", __func__, m_chip.GetStateStr(), "Reset StylusState");
    ResetStylusState();
    return {};
}

StylusState AfeController::GetStylusStateSnapshot() const {
    std::lock_guard<std::mutex> lock(m_stylusMutex);
    return m_stylus;
}

void AfeController::ResetStylusState() {
    std::lock_guard<std::mutex> lock(m_stylusMutex);
    m_stylus = StylusState{};
}

} // namespace Himax
