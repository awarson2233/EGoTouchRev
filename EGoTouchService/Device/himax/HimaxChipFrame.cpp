#include "himax/HimaxChip.h"
#include "himax/HimaxProtocol.h"
#include "Logger.h"
#include "FrameLayout.h"

#include <thread>
#include <chrono>

// 读帧完全照 himax_thp_drv!thp_afe_get_frame 的做法：只有一条路径，不分 Normal / IDLE。
// IC 在 IDLE 里照样出帧，只是慢（slave 约 1.5 s 一帧、master 约 3 s 一帧），帧尾 word0 == 1；
// 有触摸时 IC 自己醒来，阻塞着的这次读立刻返回。宿主不维护自己的 IDLE 状态，也不探测。

namespace {

constexpr uint8_t  kFrameReadTimeoutMs = 200;   // SpiSetTimeout(0/1, 200)：一次读帧最多等这么久
constexpr int      kIdleRetryAttempts  = 17;    // g_FrameRetryMaxAttempts：IC 在 IDLE 时原地再读的次数
constexpr auto     kRearmDelay         = std::chrono::milliseconds{20};     // Sleep(0x14)
constexpr uint32_t kReadStallLimit     = 5;     // 5 < g_FrameRetryTimeoutCount
constexpr auto     kReadBudget         = std::chrono::milliseconds{3200};   // 0xc80
constexpr uint32_t kBadFrameLimit      = 4;     // 4 < g_PenChecksumErrorCount / g_HandAllZeroCount
constexpr uint32_t kIdleEntryThreshold = 600;
constexpr uint32_t kChecksumOffset     = 5;     // 校验从帧头第 5 字节开始
constexpr uint8_t  kBusFiller          = 0xEC;  // 总线上没有 IC 应答时读回的填充字节

enum class FrameCheck : uint8_t { Valid, AllZero, ChecksumError, BusFiller };

// thp_compute_checksum16：16 位字求和，低 16 位为 0 且和不为 0 才是有效帧
FrameCheck CheckFrame(const uint8_t* frame, uint32_t sizeBytes) noexcept {
    const uint8_t* body = frame + kChecksumOffset;
    const uint32_t bodyBytes = sizeBytes - kChecksumOffset;
    uint32_t sum = 0;
    for (uint32_t i = 0; i + 1 < bodyBytes; i += 2) {
        sum += static_cast<uint32_t>(body[i] | (body[i + 1] << 8));
    }
    if (sum == 0) return FrameCheck::AllZero;
    if ((sum & 0xFFFFu) == 0) return FrameCheck::Valid;
    for (uint32_t i = 0; i < bodyBytes; ++i) {
        if (body[i] != kBusFiller) return FrameCheck::ChecksumError;
    }
    return FrameCheck::BusFiller;
}

// 无效的 slave 帧不能被下游当成笔数据：两个块的锚点都写成「无笔」
void MarkNoStylus(uint8_t* slaveFrame) noexcept {
    for (const int word : {0, 1, Frame::kBlockWords, Frame::kBlockWords + 1}) {
        uint8_t* p = slaveFrame + Frame::kHeaderBytes + word * 2;
        p[0] = static_cast<uint8_t>(Frame::kAnchorInvalid & 0xFF);
        p[1] = static_cast<uint8_t>(Frame::kAnchorInvalid >> 8);
    }
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

ChipResult<> Chip::ApplyFrameReadPolicy() {
    auto apply_policy = [&](HalDevice* dev, const char* devName) -> ChipResult<> {
        if (!dev || !dev->IsValid()) {
            LOG_ERROR("HimaxChip", __func__, GetStateStr(), "{} handle invalid", devName);
            return std::unexpected(ChipError::CommunicationError);
        }

        if (auto res = dev->SetBlock(true); !res) {
            LOG_ERROR("HimaxChip", __func__, GetStateStr(), "{} SetBlock(1) failed", devName);
            return res;
        }

        if (auto res = dev->SetTimeOut(kFrameReadTimeoutMs); !res) {
            LOG_ERROR("HimaxChip", __func__, GetStateStr(), "{} SetTimeOut({}) failed", devName, kFrameReadTimeoutMs);
            return res;
        }
        return {};
    };

    if (auto res = apply_policy(m_master.get(), "Master"); !res) return res;
    if (auto res = apply_policy(m_slave.get(), "Slave"); !res) return res;

    LOG_INFO("HimaxChip", __func__, GetStateStr(), "Applied read policy: block=1, timeout={}ms", kFrameReadTimeoutMs);
    return {};
}

void Chip::ResetFrameReadState() {
    m_idleRetry = false;
    m_readStalls = 0;
    m_handZeroFrames = 0;
    m_handChecksumErrors = 0;
    m_penZeroFrames = 0;
    m_penChecksumErrors = 0;
    m_sosReason = "";
    m_zeroFrameCount = 0;
    m_frameCount = 0;
    m_stylusActive = false;
    m_lastMasterWasRead = true;
    afe_mode.store(THP_AFE_MODE::Normal);
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

Chip::FrameRead Chip::GetFrame(void) {
    using clock = std::chrono::steady_clock;
    constexpr uint32_t kMasterFrameBytes = static_cast<uint32_t>(Frame::kMasterFrameSize);
    constexpr uint32_t kSlaveFrameBytes = static_cast<uint32_t>(Frame::kSlaveFrameSize);
    uint8_t* const masterBuf = back_data.data();
    uint8_t* const slaveBuf = back_data.data() + Frame::kSlaveHeaderOffset;

    struct ActiveMark {
        std::atomic<bool>& flag;
        explicit ActiveMark(std::atomic<bool>& f) : flag(f) { flag.store(true, std::memory_order_release); }
        ~ActiveMark() { flag.store(false, std::memory_order_release); }
    } activeMark{m_frameReadActive};

    const auto start = clock::now();
    m_sosReason = "";

    // 读一路。读不到帧时：IC 在 IDLE（m_idleRetry）就原地再读，最多 17 次；之后重开这一路的
    // 中断通道、等 20 ms 再来。重开累计超过 5 次，或本次取帧已经耗掉 3.2 s，判 SOS。
    // IDLE 里 18 次 × 200 ms 就超过了 3.2 s，所以 IC 在 IDLE 里静默约 3.6 s 即 SOS——原厂两天
    // 的日志里这样的复位有 81 次，是它的常态而不是故障。
    enum class Read : uint8_t { Ok, Cancelled, Sos };
    auto readChannel = [&](HalDevice* dev, uint8_t* buf, uint32_t sizeBytes, const char* name,
                           const char* sosReason) -> Read {
        if (!dev || !dev->IsValid()) {
            m_sosReason = "device handle invalid";
            return Read::Sos;
        }
        for (;;) {
            int attempts = 0;
            do {
                if (dev->GetFrame(buf, sizeBytes, nullptr)) return Read::Ok;
                if (dev->IsCancelledError()) return Read::Cancelled;
            } while (m_idleRetry && attempts++ < kIdleRetryAttempts);

            const auto waited = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - start);
            const uint32_t err = dev->GetError();
            (void)dev->IntOpen();
            LOG_NOTICE("HimaxChip", "GetFrame", GetStateStr(), "{} INT re-opened: no frame for {} ms (stall {}, IC {}, err={}).",
                       name, waited.count(), m_readStalls + 1, m_idleRetry ? "IDLE" : "active", err);
            std::this_thread::sleep_for(kRearmDelay);
            if (m_readStalls++ > kReadStallLimit || waited > kReadBudget) {
                m_sosReason = sosReason;
                return Read::Sos;
            }
        }
    };

    // ── slave（笔）──
    // 只用 master 时不读 slave：与不读 master 的帧同理按「全零」参与收尾判定，笔数据标成无笔
    FrameCheck slaveCheck = FrameCheck::AllZero;
    if (m_slaveEnabled) {
        switch (readChannel(m_slave.get(), slaveBuf, kSlaveFrameBytes, "Slave", "no slave frame")) {
        case Read::Cancelled: return FrameRead::NoData;
        case Read::Sos:       return FrameRead::Sos;
        case Read::Ok:        break;
        }
        slaveCheck = CheckFrame(slaveBuf, kSlaveFrameBytes);
    }
    const bool slaveValid = slaveCheck == FrameCheck::Valid;
    if (slaveValid) {
        m_penZeroFrames = 0;
        m_penChecksumErrors = 0;
        m_readStalls = 0;
    } else if (!m_slaveEnabled) {
        MarkNoStylus(slaveBuf);
    } else {
        MarkNoStylus(slaveBuf);
        m_idleRetry = false;
        if (slaveCheck == FrameCheck::AllZero) {
            ++m_penZeroFrames;
        } else {
            ++m_penChecksumErrors;
            LOG_WARN("HimaxChip", __func__, GetStateStr(), "Slave frame checksum error ({} in a row).", m_penChecksumErrors);
            if (slaveCheck == FrameCheck::BusFiller) {
                m_sosReason = "slave bus returns filler bytes";
                return FrameRead::Sos;
            }
        }
    }
    const bool stylusNow = slaveValid && isStylusDetected();

    // ── master（手指）──
    const bool skipMaster = ShouldSkipMasterRead(m_frameCount, m_stylusActive);
    FrameCheck masterCheck = FrameCheck::AllZero;   // 这一帧不读 master 时，原厂同样按「全零」参与收尾判定
    bool masterValid = false;
    if (!skipMaster) {
        switch (readChannel(m_master.get(), masterBuf, kMasterFrameBytes, "Master", "no master frame")) {
        case Read::Cancelled: return FrameRead::NoData;
        case Read::Sos:       return FrameRead::Sos;
        case Read::Ok:        break;
        }
        masterCheck = CheckFrame(masterBuf, kMasterFrameBytes);
        masterValid = masterCheck == FrameCheck::Valid;
        if (masterValid) {
            m_handZeroFrames = 0;
            m_handChecksumErrors = 0;
            m_readStalls = 0;
        } else {
            m_idleRetry = false;
            if (masterCheck == FrameCheck::AllZero) {
                ++m_handZeroFrames;
            } else {
                ++m_handChecksumErrors;
                LOG_WARN("HimaxChip", __func__, GetStateStr(), "Master frame checksum error ({} in a row).", m_handChecksumErrors);
                if (masterCheck == FrameCheck::BusFiller) {
                    m_sosReason = "master bus returns filler bytes";
                    return FrameRead::Sos;
                }
            }
        }
    }

    // ── 无效帧的收尾（与原厂同一组判据）──
    if (!slaveValid && !masterValid) {
        const bool someAllZero = slaveCheck == FrameCheck::AllZero || masterCheck == FrameCheck::AllZero;
        if (someAllZero && m_readStalls >= kReadStallLimit) {
            m_sosReason = "no valid frame after repeated read stalls";
            return FrameRead::Sos;
        }
        return FrameRead::NoData;
    }
    if (!slaveValid && m_penChecksumErrors > kBadFrameLimit) {
        m_sosReason = "slave checksum errors";
        return FrameRead::Sos;
    }
    if (!masterValid && m_handZeroFrames > kBadFrameLimit) {
        m_sosReason = "master frames all zero";
        return FrameRead::Sos;
    }

    m_lastMasterWasRead = masterValid;
    ++m_frameCount;
    m_stylusActive = stylusNow;

    // ── 帧尾状态（thp_afe_process_stylus_status）──
    bool fingerNow = false;
    if (masterValid) {
        Frame::MasterSuffixView suffix;
        suffix.LoadFromBytes(masterBuf + Frame::kMasterSuffixOffset);
        fingerNow = suffix.hasFinger();

        const uint16_t idleFlag = suffix.idleFlag();
        if (idleFlag == 1) m_idleRetry = true;
        if (idleFlag == 0 || (stylusNow && IsStylusConnected())) m_idleRetry = false;

        const THP_AFE_MODE mode = idleFlag == 1 ? THP_AFE_MODE::Idle : THP_AFE_MODE::Normal;
        if (afe_mode.exchange(mode) != mode) {
            if (mode == THP_AFE_MODE::Idle) {
                LOG_NOTICE("HimaxChip", __func__, GetStateStr(), "===== IDLE ENTER ===== IC reports IDLE (frame #{}).", m_frameCount);
            } else {
                LOG_NOTICE("HimaxChip", __func__, GetStateStr(), "===== IDLE EXIT ===== IC reports active (frame #{}, finger={}, stylus={}).",
                           m_frameCount, fingerNow ? 1 : 0, stylusNow ? 1 : 0);
            }
        }

        if (suffix.fwResetFlag() != 0) {
            LOG_WARN("HimaxChip", __func__, GetStateStr(), "IC reports firmware reset (suffix word6=0x{:04X}).", suffix.fwResetFlag());
            m_sosReason = "IC reports firmware reset";
            return FrameRead::Sos;
        }
    }

    // parser-only 模式不跑 TSACore，没有 IDLE 请求：连续无输入帧够多、IC 又还没进 IDLE 时由这里下发
    if (ShouldEnterIdle(m_zeroFrameCount, fingerNow || stylusNow) && m_autoIdle) {
        m_zeroFrameCount = 0;
        if (afe_mode.load() != THP_AFE_MODE::Idle) {
            (void)m_afe.EnterIdle();
        }
    }

    return FrameRead::Frame;
}

} // namespace Himax
