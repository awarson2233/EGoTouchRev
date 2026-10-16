#include "TsaCoreSolver.h"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace Solvers::Tsa {

namespace {

// TSACore 的全部状态都是进程级的全局对象，整个进程只能有一个实例完成初始化。
constinit std::atomic<bool> g_claimed{false};

constexpr uint16_t kScanRateList[1] = {HimaxAfeAdapter::kScanRate};

}  // namespace

TsaCoreSolver::~TsaCoreSolver() {
    if (m_initialized) g_claimed.store(false, std::memory_order_release);
}

// TSA_InitAFEProject_UI(面板 project id) + 分辨率 / feature（ApDaemon 的 TsaFrame 初始化序列）。
bool TsaCoreSolver::InitCore() {
    bool expected = false;
    if (!g_claimed.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) return false;

    Core::AfeCaps caps;
    caps.numCol = Frame::kRxCount;
    caps.numRow = Frame::kTxCount;
    caps.numScanFreq = 2;
    caps.scanFreq = HimaxAfeAdapter::kScanFreqTable;
    caps.numScanRate = 1;
    caps.scanRate = kScanRateList;
    if (!Core::Init(m_projectId.c_str(), caps)) {
        g_claimed.store(false, std::memory_order_release);
        return false;
    }
    Core::SetResolution(kXRes, kYRes, kResScale);
    m_features = Core::GetFeatures() & 0xffffffc0u;
    Core::SetFeatures(m_features);

    // InitTsa 置 0x40003；充电器位可能在初始化之前就已下发
    m_ctrl = (m_ctrl & CtrlFlag::kCharger) | CtrlFlag::kAlways | CtrlFlag::kFirstFrame | CtrlFlag::kResetBaseline;
    m_prevCalibDone = false;
    m_requests = {};
    m_initialized = true;
    return true;
}

bool TsaCoreSolver::Process(HeatmapFrame& frame) {
    // TSACore 只拿时间戳做差（各种防抖窗口），用单调时钟，不受系统对时影响
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return Process(frame, static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now).count()));
}

bool TsaCoreSolver::Process(HeatmapFrame& frame, uint64_t timestampMs) {
    frame.touch.output.contacts.clear();
    frame.touch.output.touchPackets = {};
    m_actions.clear();

    if (!m_initialized) {
        if (m_initFailed) return false;
        if (!InitCore()) {
            m_initFailed = true;
            return false;
        }
        m_mapper.Reset();
        m_appliedPenStatusWord = 0;
        m_penStatusApplied = false;
    }
    ApplyPenSession();

    // 驱动在笔在场时隔帧跳过 master：那一帧没有矩阵，TSACore 照样被调用（ApDaemon 的
    // TsaFrame::Process 不看有没有手指数据），只是互容处理直接结束。
    AfeFrameInfo info;
    if (frame.masterWasRead) {
        info = m_adapter.Build(frame.rawPtr, frame.rawLen);
    }
    const AfeStylusInfo stylus = m_adapter.BuildStylus(frame.rawPtr, frame.rawLen);
    m_lastStylus = stylus;
    m_lastReport = {};
    if (!frame.masterWasRead) {
        if (stylus.status != StylusStatus::kActive) return false;
        info = m_adapter.BuildWithoutMaster();
    }
    m_lastStatus = info.status;
    // 校验失败或带 SOS 位的帧不进 TSACore；不上报，VHF 保持上一帧状态
    if (!info.valid || (info.status & AfeStatus::kSos) != 0) return false;

    const bool hadContacts = m_mapper.HasActiveContacts();
    m_lastCtrl = m_ctrl;
    m_lastReport = MakeStylusReport(frame.stylus);
    UpdateCtrl(info.status);

    // 驱动每帧都给笔帧（没有笔时 status = 6、矩阵为空）。频率 / 压力 / 按键字段只有 ASA 的
    // 解算本体读取，报告改由 SetStylusReport 送入。
    m_stylusFrame = {stylus.tx1, stylus.tx2, stylus.status};
    Core::AfeFrame afe;
    afe.timeMs = timestampMs;
    afe.rawData = frame.masterWasRead ? m_adapter.RawData() : nullptr;
    afe.scanFreq = info.scanFreq;
    afe.scanRate = info.scanRate;
    afe.status = info.status;
    afe.stylus = &m_stylusFrame;
    Core::SetStylusReport(m_lastReport);
    Core::ProcessFrame(afe, m_ctrl);
    // ApDaemon 每帧之后清掉一次性的控制位，并重设 feature
    m_ctrl &= CtrlFlag::kPersistMask;
    Core::SetFeatures(m_features);
    m_requests = Core::QueryRequests();

    m_actions = m_control.Step(info, m_requests);
    const int count = m_mapper.Collect(frame.touch.output.contacts);
    m_cachedContactCount.store(count, std::memory_order_relaxed);

    // 上一帧有点、本帧清空时也要走一次上报，让 VHF 发出全抬起
    return count > 0 || hadContacts;
}

// 本帧 AFE 状态位落到控制字上（TsaFrame::Process）。IDLE 位不进控制字：TSACore 自己从帧状态读
// （Afe::IsInIdle），ApDaemon 的 UpdateStatusIdle 只打日志。
void TsaCoreSolver::UpdateCtrl(uint32_t status) noexcept {
    m_ctrl &= ~CtrlFlag::kChargerNoise;
    if (status & AfeStatus::kChargerNoise) m_ctrl |= CtrlFlag::kChargerNoise;
    // 固件的校准完成标志（suffix word3 = 0xbb）要等宿主发 ClearStatus(8) 才会清掉，期间每帧都带着；
    // ctrl 低 4 位任一非零都会让 TSACore 停在硬件复位态（TSAStatic_IsHWReset），所以只在上升沿置位一次，
    // 清除握手由 TsaAfeControl 负责。
    const bool calibDone = (status & AfeStatus::kCalibDone) != 0;
    if (calibDone && !m_prevCalibDone) m_ctrl |= CtrlFlag::kCalibDone;
    m_prevCalibDone = calibDone;
    if (status & AfeStatus::kFreqShiftDone) m_ctrl |= CtrlFlag::kFreqShiftDone;
}

void TsaCoreSolver::SetChargerPlugged(bool plugged) noexcept {
    if (plugged) {
        m_ctrl |= CtrlFlag::kCharger;
    } else {
        m_ctrl &= ~CtrlFlag::kCharger;
    }
    m_adapter.SetChargerPlugged(plugged);
}

bool TsaCoreSolver::SetPanelProjectId(std::string_view projectId) {
    if (m_initialized) return projectId == m_projectId;
    m_projectId = projectId;
    m_adapter.SetPanelGain(GainForProject(projectId));
    return true;
}

const char* TsaCoreSolver::LoadedPrmtName() const noexcept {
    return m_initialized ? Core::PrmtName() : "";
}

// TSACore 的全局状态只能在处理帧的线程上动：笔事件线程只留下最新值，这里统一落下去。
void TsaCoreSolver::ApplyPenSession() noexcept {
    m_adapter.SetStylusConnected(m_stylusConnected.load(std::memory_order_relaxed));

    const uint16_t word = m_penStatusWord.load(std::memory_order_relaxed);
    if (!m_penStatusApplied || word != m_appliedPenStatusWord) {
        ApplyPenStatusWord(word);
        m_appliedPenStatusWord = word;
        m_penStatusApplied = true;
    }

    const uint32_t seq = m_btPressureSeq.load(std::memory_order_acquire);
    if (seq != m_btPressureSeen) {
        m_btPressureSeen = seq;
        Core::OnBtPressure();
    }
}

// 与 ApDaemon 一样，每次都下发旋转角与笔状态，连接位与防误触位只在变化时处理（m_appliedPenStatusWord
// 是上一次下发的状态字，初始化后从 0 算起）。
void TsaCoreSolver::ApplyPenStatusWord(uint16_t word) noexcept {
    const uint16_t changed = static_cast<uint16_t>(word ^ m_appliedPenStatusWord);

    // DriverThp::GetStatus：旋转角，然后连着下发两次笔状态（先休眠位、再吸附位）。
    // 第二次会盖掉第一次，原版就是这样，笔状态最终只留下吸附位。
    const unsigned curStatus = static_cast<unsigned>(word >> PenStatusWord::kCurStatusShift);
    Core::SetScreenRotateState(static_cast<int>((word >> PenStatusWord::kRotateShift) & 0x3u));
    Core::SetBtPenStatus(static_cast<uint8_t>(curStatus & 1u));
    Core::SetBtPenStatus(static_cast<uint8_t>(curStatus & 2u));

    // "stylus3" 监听：笔连上时通知一次；断开时原版不调 TSACore，靠 60 s 没有蓝牙包来降级。
    if ((changed & PenStatusWord::kConnected) != 0 && (word & PenStatusWord::kConnected) != 0) {
        Core::SetBtConnectStatus(1);
    }
    // "stylusaft" 监听：防误触模式位的变化翻成场景 32 的开关。
    if ((changed & PenStatusWord::kTouchMode) != 0) {
        if ((word & PenStatusWord::kTouchMode) != 0) {
            Core::EnableScenario(kScenarioStylusAft);
        } else {
            Core::DisableScenario(kScenarioStylusAft);
        }
    }
}

// StylusSolver 的坐标是面板原始行列方向、每个节点 1024 单位；TSACore 用的是驱动倒序后的方向，
// 量程是显示像素（ASA 的 FitToLcdScreen：像素 = 分辨率 × 坐标 / 节点数 / 1024，夹在 [0, 分辨率 − 1]）。
// 两个轴都倒过来。倾角不用取反：ASA 的 TiltProcess 在倒序坐标系里按 TX2 − TX1 的坐标差算倾角，
// StylusSolver 在原始坐标系里按 TX1 − TX2 算，翻转与相减次序正好抵消，两者数值相同。
Core::StylusReport TsaCoreSolver::MakeStylusReport(const StylusFrameData& stylus) noexcept {
    Core::StylusReport report;
    const auto& out = stylus.output;
    if (!out.valid || !out.inRange) return report;

    const auto toPixel = [](float coor, int nodes, int res) {
        const float span = static_cast<float>(nodes) * 1024.0f;
        const long pixel = std::lround((span - coor) * static_cast<float>(res) / span);
        return static_cast<uint16_t>(std::clamp<long>(pixel, 0, res - 1));
    };
    report.status = out.tipDown ? StylusReportFlag::kTipDown : StylusReportFlag::kInRange;
    report.x = toPixel(out.point.x, Frame::kRxCount, kXRes);
    report.y = toPixel(out.point.y, Frame::kTxCount, kYRes);
    report.tiltX = out.point.tiltX;
    report.tiltY = out.point.tiltY;
    report.pressure = out.tipDown ? out.pressure : 0;
    return report;
}

bool TsaCoreSolver::InStylusMode() const noexcept { return m_initialized && Core::InStylusMode(); }

bool TsaCoreSolver::StylusInRange() const noexcept { return m_initialized && Core::StylusInRange(); }

void TsaCoreSolver::OnAfeStarted() noexcept {
    m_adapter.Reset();
    m_control.Reset();
    m_mapper.Reset();
    m_actions.clear();
    // 重启后的状态位从头算起：校准完成位要重新走一次上升沿
    m_prevCalibDone = false;
}

void TsaCoreSolver::OnAfeStopped() noexcept {
    m_mapper.Reset();
    m_actions.clear();
    m_cachedContactCount.store(0, std::memory_order_relaxed);
}

void TsaCoreSolver::OnAfeActionExecuted(const AfeAction& action, bool ok) noexcept {
    if (!ok) return;
    switch (action.type) {
    case AfeActionType::ForceToFreqPoint:
        // thp_afe_force_to_freq_point：记下目标频点，等帧里出现「跳频完成」
        m_adapter.SetFreqShiftInFlight(
            true, HimaxAfeAdapter::kScanFreqTable[action.param != 0 ? 1 : 0]);
        break;
    case AfeActionType::ClearStatus:
        // thp_afe_clear_status(4)：跳频握手结束
        if (action.param == AfeStatus::kFreqShiftDone) m_adapter.SetFreqShiftInFlight(false, 0);
        break;
    default:
        break;
    }
}

const int16_t* TsaCoreSolver::DiffGrid() const noexcept { return m_initialized ? Core::DiffGrid() : nullptr; }

}  // namespace Solvers::Tsa
