#include "himax/HimaxChip.h"
#include "himax/HimaxProtocol.h"
#include "Logger.h"
#include "FrameLayout.h"

#include <thread>
#include <chrono>

namespace Himax {

ChipResult<> Chip::SetFrameReadPolicy(bool block, uint8_t timeoutMs) {
    auto apply_policy = [&](HalDevice* dev, const char* devName) -> ChipResult<> {
        if (!dev || !dev->IsValid()) {
            LOG_ERROR("HimaxChip", __func__, GetStateStr(), "{} handle invalid", devName);
            return std::unexpected(ChipError::CommunicationError);
        }

        if (auto res = dev->SetBlock(block); !res) {
            LOG_ERROR("HimaxChip", __func__, GetStateStr(), "{} SetBlock({}) failed", devName, block ? 1 : 0);
            return res;
        }

        if (auto res = dev->SetTimeOut(timeoutMs); !res) {
            LOG_ERROR("HimaxChip", __func__, GetStateStr(), "{} SetTimeOut({}) failed", devName, timeoutMs);
            return res;
        }
        return {};
    };

    if (auto res = apply_policy(m_master.get(), "Master"); !res) return res;
    if (auto res = apply_policy(m_slave.get(), "Slave"); !res) return res;

    LOG_INFO("HimaxChip", __func__, GetStateStr(), "Applied read policy: block={}, timeout={}ms", block ? 1 : 0, timeoutMs);
    return {};
}

ChipResult<> Chip::SetFrameReadNormalPolicy() {
    return SetFrameReadPolicy(true, 100);
}

ChipResult<> Chip::SetFrameReadIdlePolicy() {
    return SetFrameReadPolicy(true, 200);
}

ChipResult<> Chip::NotifyTouchWakeup() {
    if (m_connState.load() != ConnectionState::Connected) {
        return std::unexpected(ChipError::InvalidOperation);
    }

    if (afe_mode != THP_AFE_MODE::Idle) {
        return {};
    }

    if (auto res = SetFrameReadNormalPolicy(); !res) return res;
    afe_mode = THP_AFE_MODE::Normal;
    LOG_INFO("HimaxChip", __func__, GetStateStr(), "===== IDLE EXIT ===== Touch wakeup → Normal mode.");
    return {};
}

bool Chip::isFingerDetected() const {
    Frame::MasterSuffixView suffix;
    suffix.LoadFromBytes(back_data.data() + Frame::kMasterSuffixOffset);
    return suffix.hasFinger();
}

bool Chip::isStylusDetected() const {
    Frame::SlaveSuffixView suffix;
    suffix.LoadFromBytes(back_data.data() + Frame::kSlaveSuffixOffset);
    return suffix.hasStylus();
}

ChipResult<> Chip::GetFrame(void) {
    constexpr uint32_t kMasterFrameBytes = static_cast<uint32_t>(Frame::kMasterFrameSize);
    constexpr uint32_t kSlaveFrameBytes = static_cast<uint32_t>(Frame::kSlaveFrameSize);
    constexpr size_t kSlaveFrameOffset = static_cast<size_t>(Frame::kSlaveHeaderOffset);

    // Idle: periodic probe only
    if (afe_mode.load() == THP_AFE_MODE::Idle) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        auto m_res = m_master->GetFrame(back_data.data(), kMasterFrameBytes, nullptr);
        auto s_res = m_slave->GetFrame(back_data.data() + kSlaveFrameOffset, kSlaveFrameBytes, nullptr);

        if (m_res && s_res) {
            if (isFingerDetected() || isStylusDetected()) {
                (void)NotifyTouchWakeup();
                LOG_INFO("HimaxChip", __func__, GetStateStr(), "Input detected in idle → wakeup to Normal");
            }
            return std::unexpected(ChipError::Timeout);
        }
        return std::unexpected(ChipError::Timeout);
    }

    const bool skipMaster = (m_frameCount & 1) != 0 && m_stylusActive;

    if (auto res = m_slave->GetFrame(back_data.data() + kSlaveFrameOffset, kSlaveFrameBytes, nullptr); !res) {
        if (m_slave->IsTimeoutError())
            return std::unexpected(ChipError::Timeout);
        LOG_ERROR("HimaxChip", __func__, GetStateStr(), "Slave GetFrame failed!");
        return res;
    }

    if (!skipMaster) {
        if (auto res = m_master->GetFrame(back_data.data(), kMasterFrameBytes, nullptr); !res) {
            if (m_master->IsTimeoutError())
                return std::unexpected(ChipError::Timeout);
            LOG_ERROR("HimaxChip", __func__, GetStateStr(), "Master GetFrame failed!");
            return res;
        }
    }

    m_lastMasterWasRead = !skipMaster;
    m_frameCount++;

    const bool stylusNow = isStylusDetected();
    if (stylusNow && !m_stylusActive) {
        m_stylusActive = true;
    } else if (!stylusNow && m_stylusActive) {
        m_stylusActive = false;
    }

    constexpr uint32_t kIdleEntryThreshold = 600;

    if (isFingerDetected() || stylusNow) {
        m_zeroFrameCount = 0;
    } else {
        m_zeroFrameCount++;
        if (m_zeroFrameCount >= kIdleEntryThreshold) {
            LOG_INFO("HimaxChip", __func__, GetStateStr(), "No input for {} frames → EnterIdle", m_zeroFrameCount);
            m_stylusActive = false;
            (void)m_afe.EnterIdle();
            m_zeroFrameCount = 0;
        }
    }
    return {};
}

} // namespace Himax
