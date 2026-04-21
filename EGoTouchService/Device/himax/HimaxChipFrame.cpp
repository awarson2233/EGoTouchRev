#include "himax/HimaxChip.h"
#include "himax/HimaxProtocol.h"
#include "Logger.h"
#include "FrameLayout.h"

#include <thread>
#include <chrono>

namespace {

constexpr auto kIdleProbeInterval = std::chrono::milliseconds{50};
constexpr uint32_t kIdleEntryThreshold = 600;

ChipResult<> ReadFrameBuffer(Himax::HalDevice* dev,
                             void* buffer,
                             uint32_t sizeBytes,
                             const char* logFunc,
                             const char* chipState,
                             const char* deviceName,
                             bool logFailures) {
    if (!dev || !dev->IsValid()) {
        if (logFailures) {
            LOG_ERROR("HimaxChip", logFunc, chipState, "{} handle invalid", deviceName);
        }
        return std::unexpected(ChipError::CommunicationError);
    }

    if (auto res = dev->GetFrame(buffer, sizeBytes, nullptr); !res) {
        if (dev->IsTimeoutError()) {
            return std::unexpected(ChipError::Timeout);
        }
        if (logFailures) {
            LOG_ERROR("HimaxChip", logFunc, chipState, "{} GetFrame failed!", deviceName);
        }
        return res;
    }

    return {};
}

bool ShouldSkipMasterRead(uint32_t frameCount, bool stylusActive) noexcept {
    return ((frameCount & 1u) != 0u) && stylusActive;
}

bool ShouldEnterIdle(uint32_t& zeroFrameCount, bool inputDetected) noexcept {
    if (inputDetected) {
        zeroFrameCount = 0;
        return false;
    }

    ++zeroFrameCount;
    return zeroFrameCount >= kIdleEntryThreshold;
}

} // anonymous namespace

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

    auto readMaster = [&](bool logFailures) -> ChipResult<> {
        return ReadFrameBuffer(m_master.get(),
                               back_data.data(),
                               kMasterFrameBytes,
                               __func__,
                               GetStateStr(),
                               "Master",
                               logFailures);
    };

    auto readSlave = [&](bool logFailures) -> ChipResult<> {
        return ReadFrameBuffer(m_slave.get(),
                               back_data.data() + kSlaveFrameOffset,
                               kSlaveFrameBytes,
                               __func__,
                               GetStateStr(),
                               "Slave",
                               logFailures);
    };

    if (afe_mode.load() == THP_AFE_MODE::Idle) {
        std::this_thread::sleep_for(kIdleProbeInterval);

        const auto masterRes = readMaster(false);
        const auto slaveRes = readSlave(false);

        if (masterRes && slaveRes && (isFingerDetected() || isStylusDetected())) {
            (void)NotifyTouchWakeup();
            LOG_INFO("HimaxChip", __func__, GetStateStr(), "Input detected in idle → wakeup to Normal");
        }

        return std::unexpected(ChipError::Timeout);
    }

    const bool skipMaster = ShouldSkipMasterRead(m_frameCount, m_stylusActive);

    if (auto res = readSlave(true); !res) {
        return res;
    }

    if (!skipMaster) {
        if (auto res = readMaster(true); !res) {
            return res;
        }
    }

    m_lastMasterWasRead = !skipMaster;
    ++m_frameCount;

    const bool fingerNow = isFingerDetected();
    const bool stylusNow = isStylusDetected();
    m_stylusActive = stylusNow;

    if (ShouldEnterIdle(m_zeroFrameCount, fingerNow || stylusNow)) {
        LOG_INFO("HimaxChip", __func__, GetStateStr(), "No input for {} frames → EnterIdle", m_zeroFrameCount);
        m_stylusActive = false;
        (void)m_afe.EnterIdle();
        m_zeroFrameCount = 0;
    }

    return {};
}

} // namespace Himax
