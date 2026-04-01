#include "himax/HimaxAfe.h"
#include "himax/HimaxChip.h"
#include "himax/HimaxProtocol.h"
#include "Logger.h"

#include <format>

namespace {

const char* AfeCommandToString(AFE_Command cmd) {
    switch (cmd) {
    case AFE_Command::ClearStatus:       return "ClearStatus";
    case AFE_Command::EnableFreqShift:   return "EnableFreqShift";
    case AFE_Command::DisableFreqShift:  return "DisableFreqShift";
    case AFE_Command::StartCalibration:  return "StartCalibration";
    case AFE_Command::EnterIdle:         return "EnterIdle";
    case AFE_Command::ForceExitIdle:     return "ForceExitIdle";
    case AFE_Command::ForceToFreqPoint:  return "ForceToFreqPoint";
    case AFE_Command::ForceToScanRate:   return "ForceToScanRate";
    case AFE_Command::InitStylus:        return "InitStylus";
    case AFE_Command::SetStylusId:       return "SetStylusId";
    case AFE_Command::DisconnectStylus:  return "DisconnectStylus";
    default:                             return "Unknown";
    }
}

/// 默认频率命令表（4 条目 + 1 默认笔 ID=5）
/// 来源：Ghidra 逆向 himax_thp_drv.dll DAT_18016dc80
static constexpr struct { uint8_t id; uint16_t f0; uint16_t f1; } kFreqTable[] = {
    {0, 0x00A1, 0x0018},   // 笔 0
    {1, 0x00A2, 0x0019},   // 笔 1
    {2, 0x00A3, 0x001A},   // 笔 2
    {3, 0x00A4, 0x001B},   // 笔 3
    {5, 0x00A1, 0x0018},   // 默认笔 "CD54" (ID=5)
};

} // anonymous namespace

namespace Himax {

// ── AFE 命令分发器 ──────────────────────────────────────────────────────────

ChipResult<> AfeController::SendCommand(command cmd) {
    LOG_INFO("Device", "AfeController::SendCommand", m_chip.GetStateStr(),
             "Dispatch cmd={}({}), param={}",
             AfeCommandToString(cmd.type),
             static_cast<int>(cmd.type),
             static_cast<unsigned int>(cmd.param));

    switch (cmd.type) {
    case AFE_Command::ClearStatus:       return ClearStatus(cmd.param);
    case AFE_Command::EnableFreqShift:   return EnableFreqShift();
    case AFE_Command::DisableFreqShift:  return DisableFreqShift();
    case AFE_Command::StartCalibration:  return StartCalibration(cmd.param);
    case AFE_Command::EnterIdle:         return EnterIdle(cmd.param);
    case AFE_Command::ForceExitIdle:     return ForceExitIdle();
    case AFE_Command::ForceToFreqPoint:  return ForceToFreqPoint(cmd.param);
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

    LOG_INFO("Device", "AfeController::EnterIdle", m_chip.GetStateStr(),
             "Entering with param={}", static_cast<unsigned>(param));

    if (auto res = HimaxProtocol::send_command(m_chip.GetMasterDevice(), 0x0a, param, m_chip.GetCurrentSlot()); !res) {
        LOG_ERROR("Device", "AfeController::EnterIdle", m_chip.GetStateStr(),
                  "Send ENTER_IDLE command failed!");
        return res;
    }

    if (auto res = m_chip.SetFrameReadIdlePolicy(); !res) return res;
    m_chip.SetAfeMode(THP_AFE_MODE::Idle);

    LOG_INFO("Device", "AfeController::EnterIdle", m_chip.GetStateStr(),
             "===== IDLE ENTER ===== No input detected, entering low-power idle.");
    return {};
}

ChipResult<> AfeController::ForceExitIdle() {
    if (m_chip.GetConnectionState() != ConnectionState::Connected)
        return std::unexpected(ChipError::InvalidOperation);

    LOG_INFO("Device", "AfeController::ForceExitIdle", m_chip.GetStateStr(), "Entering!");
    auto res = m_chip.NotifyTouchWakeup();
    LOG_INFO("Device", "AfeController::ForceExitIdle", m_chip.GetStateStr(), "Out!");
    return res;
}

ChipResult<> AfeController::StartCalibration(uint8_t param) {
    if (m_chip.GetConnectionState() != ConnectionState::Connected)
        return std::unexpected(ChipError::InvalidOperation);

    LOG_INFO("Device", "AfeController::StartCalibration", m_chip.GetStateStr(),
             "Entering with param={}", static_cast<unsigned>(param));

    if (auto res = HimaxProtocol::send_command(m_chip.GetMasterDevice(), 0x01, param, m_chip.GetCurrentSlot()); !res) {
        LOG_ERROR("Device", "AfeController::StartCalibration", m_chip.GetStateStr(),
                  "Send AFE_START_CALBRATION command failed!");
        return res;
    }

    LOG_INFO("Device", "AfeController::StartCalibration", m_chip.GetStateStr(), "Out!");
    return {};
}

ChipResult<> AfeController::EnableFreqShift() {
    if (m_chip.GetConnectionState() != ConnectionState::Connected)
        return std::unexpected(ChipError::InvalidOperation);

    LOG_INFO("Device", "AfeController::EnableFreqShift", m_chip.GetStateStr(), "Entering!");
    return HimaxProtocol::send_command(m_chip.GetMasterDevice(), 0x0d, 0x00, m_chip.GetCurrentSlot());
}

ChipResult<> AfeController::DisableFreqShift() {
    if (m_chip.GetConnectionState() != ConnectionState::Connected)
        return std::unexpected(ChipError::InvalidOperation);

    if (m_stylus.switchPolicy == 2) {
        LOG_INFO("Device", "AfeController::DisableFreqShift", m_chip.GetStateStr(),
                 "AUTO mode (switchPolicy=2), skipping disable command.");
        return {};
    }

    LOG_INFO("Device", "AfeController::DisableFreqShift", m_chip.GetStateStr(), "Entering!");
    return HimaxProtocol::send_command(m_chip.GetMasterDevice(), 0x02, 0x00, m_chip.GetCurrentSlot());
}

ChipResult<> AfeController::ClearStatus(uint8_t cmd_val) {
    if (m_chip.GetConnectionState() != ConnectionState::Connected)
        return std::unexpected(ChipError::InvalidOperation);

    if (cmd_val & 0x40) {
        m_stylus.switchReqPending = false;
        LOG_INFO("Device", "AfeController::ClearStatus", m_chip.GetStateStr(),
                 "Cleared switchReqPending (0x40).");
    }
    if (cmd_val & 0x20) {
        LOG_INFO("Device", "AfeController::ClearStatus", m_chip.GetStateStr(),
                 "Cleared FreqSwitchForceFlag (0x20).");
    }
    return {};
}

ChipResult<> AfeController::ForceToFreqPoint(uint8_t freq_idx) {
    if (m_chip.GetConnectionState() != ConnectionState::Connected)
        return std::unexpected(ChipError::InvalidOperation);

    LOG_INFO("Device", "AfeController::ForceToFreqPoint", m_chip.GetStateStr(),
             "Entering with freq_idx={}", static_cast<unsigned>(freq_idx));
    return HimaxProtocol::send_command(m_chip.GetMasterDevice(), 0x0c, freq_idx, m_chip.GetCurrentSlot());
}

ChipResult<> AfeController::ForceToScanRate(uint8_t rate_idx) {
    if (m_chip.GetConnectionState() != ConnectionState::Connected)
        return std::unexpected(ChipError::InvalidOperation);

    LOG_INFO("Device", "AfeController::ForceToScanRate", m_chip.GetStateStr(),
             "Entering with rate_idx={}", static_cast<unsigned>(rate_idx));
    return HimaxProtocol::send_command(m_chip.GetMasterDevice(), 0x0e, rate_idx, m_chip.GetCurrentSlot());
}

// ── 手写笔生命周期 ──────────────────────────────────────────────────────────

ChipResult<> AfeController::InitStylus(uint8_t pen_id) {
    if (m_chip.GetConnectionState() != ConnectionState::Connected)
        return std::unexpected(ChipError::InvalidOperation);

    LOG_INFO("Device", "AfeController::InitStylus", m_chip.GetStateStr(),
             "0x71 connect → EnableFreqShift + set_stylus_connect(1)");

    if (auto r = EnableFreqShift(); !r) return r;

    m_stylus.connected       = true;
    m_stylus.freqIdx         = 0;
    m_stylus.switchPolicy    = 2;       // AUTO模式
    m_stylus.switchTargetIdx = 0;
    m_stylus.switchReqPending = false;

    LOG_INFO("Device", "AfeController::InitStylus", m_chip.GetStateStr(),
             "Stylus connected, waiting for PenTypeInfo (0x73) to bind freq pair.");
    return {};
}

ChipResult<> AfeController::SetStylusId(uint8_t pen_id) {
    if (m_chip.GetConnectionState() != ConnectionState::Connected)
        return std::unexpected(ChipError::InvalidOperation);

    LOG_INFO("Device", "AfeController::SetStylusId", m_chip.GetStateStr(),
             "0x73 pen_type={} → 查频率表绑定 FreqPair",
             static_cast<unsigned>(pen_id));

    StylusFreqPair pair{0x00A1, 0x0018};  // 默认值（ID=5）
    for (auto& e : kFreqTable) {
        if (e.id == pen_id) {
            pair.freq0_cmd = e.f0;
            pair.freq1_cmd = e.f1;
            break;
        }
    }

    m_stylus.pen_id   = pen_id;
    m_stylus.freqPair = pair;

    LOG_INFO("Device", "AfeController::SetStylusId", m_chip.GetStateStr(),
             "Stylus freq pair bound: freq0=0x{:04X}, freq1=0x{:04X}",
             pair.freq0_cmd, pair.freq1_cmd);
    return {};
}

ChipResult<> AfeController::DisconnectStylus() {
    if (m_chip.GetConnectionState() != ConnectionState::Connected)
        return std::unexpected(ChipError::InvalidOperation);

    LOG_INFO("Device", "AfeController::DisconnectStylus", m_chip.GetStateStr(),
             "DisableFreqShift + reset StylusState");

    if (m_stylus.switchPolicy != 2) {
        (void)DisableFreqShift();
    } else {
        LOG_INFO("Device", "AfeController::DisconnectStylus", m_chip.GetStateStr(),
                 "AUTO mode (policy=2) → skipping disable_freq_shift.");
    }

    m_stylus = StylusState{};
    return {};
}

// ── 每帧频率状态追踪 ────────────────────────────────────────────────────────

bool AfeController::ProcessStylusStatus() {
    if (!m_stylus.connected || m_stylus.switchPolicy < 2) return false;

    m_stylus.frameCounter++;

    // 定位 master 帧内的状态表
    constexpr size_t kMasterStatusOffset = 4807;
    constexpr size_t kF0NoiseOffset      = kMasterStatusOffset + 0x1C;
    constexpr size_t kF1NoiseOffset      = kMasterStatusOffset + 0x20;
    constexpr size_t kFreqShiftDoneOffset = kMasterStatusOffset + 0x04;
    constexpr size_t kTpFreq1Offset      = kMasterStatusOffset + 0x10;
    constexpr size_t kTpFreq2Offset      = kMasterStatusOffset + 0x12;

    const auto& back_data = m_chip.GetFrameBuffer();
    if (kF1NoiseOffset + 2 > back_data.size()) return false;

    auto readU16 = [&](size_t off) -> uint16_t {
        return static_cast<uint16_t>(back_data[off]) |
               (static_cast<uint16_t>(back_data[off + 1]) << 8);
    };

    uint16_t f0_noise = readU16(kF0NoiseOffset);
    uint16_t f1_noise = readU16(kF1NoiseOffset);
    uint16_t freqDone = readU16(kFreqShiftDoneOffset);
    uint16_t tpFreq1  = readU16(kTpFreq1Offset);
    uint16_t tpFreq2  = readU16(kTpFreq2Offset);

    constexpr uint32_t kCooldownFrames     = 120;
    constexpr uint32_t kPendingTimeout     = 360;
    constexpr uint32_t kBtPressureTimeout  = 120;
    constexpr uint32_t kBtHibernateTimeout = 7200;
    constexpr uint32_t kBtMismatchDebounce = 4;
    constexpr int      kNoiseThresholdF0toF1 = 5001;
    constexpr int      kNoiseThresholdF1toF0 = 5000;

    // BT 心跳超时检查
    if (m_stylus.btActive) {
        uint32_t btAge = m_stylus.frameCounter - m_stylus.btLastSeenFrame;
        if (btAge > kBtPressureTimeout && !m_stylus.btPressureCleared) {
            m_stylus.btPressureCleared = true;
            LOG_WARN("Device", "AfeController::ProcessStylusStatus", m_chip.GetStateStr(),
                     "BT MCU silent >1s → pressure data reset.");
        }
        if (btAge > kBtHibernateTimeout) {
            m_stylus.btActive = false;
            LOG_WARN("Device", "AfeController::ProcessStylusStatus", m_chip.GetStateStr(),
                     "BT MCU heartbeat timeout (60s) → marking inactive.");
        }
    }

    // 频率切换完成确认
    if (freqDone != 0 && m_stylus.switchReqPending) {
        m_stylus.freqIdx = m_stylus.switchTargetIdx;
        m_stylus.switchReqPending = false;
        m_stylus.lastSwitchFrame = m_stylus.frameCounter;
        m_stylus.btMismatchActive = false;

        LOG_INFO("Device", "AfeController::ProcessStylusStatus", m_chip.GetStateStr(),
                 "FreqShift done → freqIdx={}, tpFreq=[{},{}], btFreq=[{},{}]",
                 m_stylus.freqIdx, tpFreq1, tpFreq2,
                 m_stylus.btFreq1, m_stylus.btFreq2);
    }

    // Pending 超时自愈
    if (m_stylus.switchReqPending) {
        uint32_t pendingAge = m_stylus.frameCounter - m_stylus.switchReqFrame;
        if (pendingAge > kPendingTimeout) {
            LOG_WARN("Device", "AfeController::ProcessStylusStatus", m_chip.GetStateStr(),
                     "FreqShift pending timeout ({} frames) → force clear.", pendingAge);
            m_stylus.switchReqPending = false;
            m_stylus.lastSwitchFrame = m_stylus.frameCounter;
        }
        return false;
    }

    // Cooldown 防抖
    uint32_t sinceLastSwitch = m_stylus.frameCounter - m_stylus.lastSwitchFrame;
    if (m_stylus.lastSwitchFrame != 0 && sinceLastSwitch < kCooldownFrames) {
        return false;
    }

    // BT-TP 频率不匹配 debounce
    if (m_stylus.btActive && tpFreq1 != 0 && m_stylus.btFreq1 != 0) {
        bool mismatch = (tpFreq1 != m_stylus.btFreq1);
        if (mismatch) {
            if (!m_stylus.btMismatchActive) {
                m_stylus.btMismatchActive = true;
                m_stylus.btMismatchFrame = m_stylus.frameCounter;
            } else {
                uint32_t mismatchAge = m_stylus.frameCounter - m_stylus.btMismatchFrame;
                if (mismatchAge > kBtMismatchDebounce) {
                    uint8_t targetIdx = (m_stylus.btFreq1 == m_stylus.freqPair.freq1_cmd) ? 1 : 0;
                    m_stylus.switchTargetIdx  = targetIdx;
                    m_stylus.switchReqPending = true;
                    m_stylus.switchReqFrame   = m_stylus.frameCounter;
                    m_stylus.btMismatchActive = false;
                    LOG_INFO("Device", "AfeController::ProcessStylusStatus", m_chip.GetStateStr(),
                             "BT-TP mismatch >30ms: tp={} bt={} → ForceToFreqPoint({})",
                             tpFreq1, m_stylus.btFreq1, targetIdx);
                    return true;
                }
            }
        } else {
            m_stylus.btMismatchActive = false;
        }
    }

    // 噪声差判断 → 频率切换请求
    int diff_01 = static_cast<int>(f0_noise) - static_cast<int>(f1_noise);
    int diff_10 = static_cast<int>(f1_noise) - static_cast<int>(f0_noise);

    if (diff_01 >= kNoiseThresholdF0toF1 && m_stylus.freqIdx != 1) {
        m_stylus.switchTargetIdx  = 1;
        m_stylus.switchReqPending = true;
        m_stylus.switchReqFrame   = m_stylus.frameCounter;
        LOG_INFO("Device", "AfeController::ProcessStylusStatus", m_chip.GetStateStr(),
                 "F0({})−F1({})={} ≥{} → ForceToFreqPoint(1), tpFreq=[{},{}]",
                 f0_noise, f1_noise, diff_01, kNoiseThresholdF0toF1, tpFreq1, tpFreq2);
        return true;
    } else if (diff_10 > kNoiseThresholdF1toF0 && m_stylus.freqIdx != 0) {
        m_stylus.switchTargetIdx  = 0;
        m_stylus.switchReqPending = true;
        m_stylus.switchReqFrame   = m_stylus.frameCounter;
        LOG_INFO("Device", "AfeController::ProcessStylusStatus", m_chip.GetStateStr(),
                 "F1({})−F0({})={} >{} → ForceToFreqPoint(0), tpFreq=[{},{}]",
                 f1_noise, f0_noise, diff_10, kNoiseThresholdF1toF0, tpFreq1, tpFreq2);
        return true;
    }
    return false;
}

void AfeController::UpdateBtHeartbeat(uint8_t freq1, uint8_t freq2) {
    if (!m_stylus.connected) return;
    if (freq1 == 0 && freq2 == 0) return;

    bool freqChanged = (freq1 != m_stylus.btFreq1 || freq2 != m_stylus.btFreq2);

    m_stylus.btFreq1           = freq1;
    m_stylus.btFreq2           = freq2;
    m_stylus.btActive          = true;
    m_stylus.btLastSeenFrame   = m_stylus.frameCounter;
    m_stylus.btPressureCleared = false;

    if (freqChanged) {
        LOG_INFO("Device", "AfeController::UpdateBtHeartbeat", m_chip.GetStateStr(),
                 "BT freq updated: [{}, {}]", freq1, freq2);
    }
}

} // namespace Himax
