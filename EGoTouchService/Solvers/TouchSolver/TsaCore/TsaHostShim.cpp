#include "TsaHostShim.h"

#include <atomic>
#include <cstring>

#include "FrameLayout.h"

namespace Solvers::Tsa {

namespace {

// TSACore 保存 AfeHostStruct 的指针并在运行期反复读取，三个结构体因此放在静态存储里。
constinit AfeCaps g_caps{};
constinit AfeFrame g_frame{};
constinit AfeHostStruct g_host{};
constinit std::atomic<bool> g_claimed{false};

constexpr uint16_t kScanRateList[1] = {HimaxAfeAdapter::kScanRate};
constexpr char kProjectName[] = "W273AS2700";
constexpr char kAfeInfo[16] = "himax";

}  // namespace

TsaHostShim::~TsaHostShim() {
    if (m_initialized) g_claimed.store(false, std::memory_order_release);
}

bool TsaHostShim::Initialize() {
    if (m_initialized) return true;
    bool expected = false;
    if (!g_claimed.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        return false;
    }

    g_caps = {};
    g_caps.numCol = Frame::kRxCount;
    g_caps.numRow = Frame::kTxCount;
    g_caps.numScanFreq = 2;
    g_caps.scanFreq = HimaxAfeAdapter::kScanFreqTable;
    g_caps.numScanRate = 1;
    g_caps.scanRate = kScanRateList;

    g_frame = {};
    g_host = {};
    g_host.info = kAfeInfo;
    g_host.caps = &g_caps;
    g_host.frame = &g_frame;
    std::strcpy(g_host.name, "himax_thp_drv");

    TSACoreRaw_InitImage();
    if (TSA_InitAFEProject_UI(reinterpret_cast<int64_t>(kProjectName),
                              reinterpret_cast<int64_t>(&g_host),
                              reinterpret_cast<int64_t>(&g_caps)) != 0) {
        g_claimed.store(false, std::memory_order_release);
        return false;
    }
    TSA_SetXRes(kXRes);
    TSA_SetYRes(kYRes);
    TSA_SetResScale(kResScale);
    m_features = TSA_GetFeatures() & 0xffffffc0u;
    TSA_SetFeatures(m_features);

    m_ctrl = CtrlFlag::kAlways | CtrlFlag::kFirstFrame | CtrlFlag::kResetBaseline;
    m_prevIdle = false;
    m_prevCalibDone = false;
    m_sequence = 0;
    m_requests = {};
    m_initialized = true;
    return true;
}

void TsaHostShim::NotifyAfeStarted() noexcept {
    m_ctrl |= CtrlFlag::kFirstFrame | CtrlFlag::kResetBaseline;
    // 重启后的状态位从头算起：校准完成位要重新走一次上升沿
    m_prevIdle = false;
    m_prevCalibDone = false;
}

void TsaHostShim::NotifyIdleExit() noexcept {
    m_ctrl |= CtrlFlag::kIdleExit;
}

void TsaHostShim::ProcessFrame(const uint16_t* rawData, const AfeFrameInfo& info, uint64_t timestampMs) {
    if (!m_initialized) return;

    const bool idle = (info.status & AfeStatus::kIdle) != 0;
    m_ctrl &= ~(CtrlFlag::kIdle | CtrlFlag::kChargerNoise);
    if (idle) m_ctrl |= CtrlFlag::kIdle;
    if (info.status & AfeStatus::kChargerNoise) m_ctrl |= CtrlFlag::kChargerNoise;
    if (m_prevIdle && !idle) m_ctrl |= CtrlFlag::kIdleExit;
    // 固件的校准完成标志（suffix word3 = 0xbb）要等宿主发 ClearStatus(8) 才会清掉，期间每帧都带着；
    // ctrl 低 4 位任一非零都会让 TSACore 停在硬件复位态（TSAStatic_IsHWReset），所以只在上升沿置位一次，
    // 清除握手由 TsaAfeControl 负责。
    const bool calibDone = (info.status & AfeStatus::kCalibDone) != 0;
    if (calibDone && !m_prevCalibDone) m_ctrl |= CtrlFlag::kCalibDone;
    m_prevCalibDone = calibDone;
    if (info.status & AfeStatus::kFreqShiftDone) m_ctrl |= CtrlFlag::kFreqShiftDone;
    m_prevIdle = idle;

    g_frame.timestampSec = static_cast<uint32_t>(timestampMs / 1000);
    g_frame.timestampUsec = static_cast<uint32_t>(timestampMs % 1000) * 1000;
    g_frame.sequence = m_sequence++;
    g_frame.rawData = rawData;
    g_frame.lineData = nullptr;
    g_frame.scanFreq = info.scanFreq;
    g_frame.scanRate = info.scanRate;
    g_frame.status = info.status;

    TSA_ProcessingAFEFrame(&g_frame, m_ctrl, timestampMs);
    m_ctrl &= CtrlFlag::kPersistMask;
    TSA_SetFeatures(m_features);
    QueryRequests();
}

void TsaHostShim::QueryRequests() noexcept {
    uint8_t index = 0;
    m_requests.scanFreqIndex =
        TSA_GetAFEScanFreqIndex(reinterpret_cast<int64_t>(&index)) == 1 ? -1 : static_cast<int>(index);
    index = 0;
    m_requests.scanRateIndex =
        TSA_GetAFEScanRateIndex(reinterpret_cast<int64_t>(&index)) == 1 ? -1 : static_cast<int>(index);
    m_requests.okToEnterIdle = TSA_IsOKToEnterIDLEMode() != 0;
    m_requests.reCalibNeeded = TSA_IsReCalibNeeded() != 0;
}

}  // namespace Solvers::Tsa
