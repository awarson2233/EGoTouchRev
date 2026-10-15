#include "TsaCoreSolver.h"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace Solvers::Tsa {

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

    if (!m_shim.IsInitialized()) {
        if (m_initFailed) return false;
        if (!m_shim.Initialize()) {
            m_initFailed = true;
            return false;
        }
        m_mapper.Reset();
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
    m_lastCtrl = m_shim.CtrlFlags();
    m_lastReport = MakeStylusReport(frame.stylus);
    m_shim.ProcessFrame(frame.masterWasRead ? m_adapter.RawData() : nullptr, info, stylus, m_lastReport,
                        timestampMs);
    m_actions = m_control.Step(info, m_shim.Requests());
    const int count = m_mapper.Collect(frame.touch.output.contacts);
    m_cachedContactCount.store(count, std::memory_order_relaxed);

    // 上一帧有点、本帧清空时也要走一次上报，让 VHF 发出全抬起
    return count > 0 || hadContacts;
}

void TsaCoreSolver::OnBtPressure(uint16_t p0, uint16_t p1, uint16_t p2, uint16_t p3) noexcept {
    m_btPressure.store(static_cast<uint64_t>(p0) | (static_cast<uint64_t>(p1) << 16) |
                           (static_cast<uint64_t>(p2) << 32) | (static_cast<uint64_t>(p3) << 48),
                       std::memory_order_relaxed);
    m_btPressureSeq.fetch_add(1, std::memory_order_release);
}

// TSACore 的全局状态只能在处理帧的线程上动：笔事件线程只留下最新值，这里统一落下去。
void TsaCoreSolver::ApplyPenSession() noexcept {
    m_adapter.SetStylusConnected(m_stylusConnected.load(std::memory_order_relaxed));

    const uint16_t word = m_penStatusWord.load(std::memory_order_relaxed);
    if (!m_penStatusApplied || word != m_appliedPenStatusWord) {
        m_shim.SetPenStatusWord(word);
        m_appliedPenStatusWord = word;
        m_penStatusApplied = true;
    }

    const uint32_t seq = m_btPressureSeq.load(std::memory_order_acquire);
    if (seq != m_btPressureSeen) {
        m_btPressureSeen = seq;
        const uint64_t packed = m_btPressure.load(std::memory_order_relaxed);
        m_shim.OnBtPressure(static_cast<uint16_t>(packed), static_cast<uint16_t>(packed >> 16),
                            static_cast<uint16_t>(packed >> 32), static_cast<uint16_t>(packed >> 48));
    }
}

// StylusSolver 的坐标是面板原始行列方向、每个节点 1024 单位；TSACore 用的是驱动倒序后的方向，
// 量程是显示像素（ASA 的 FitToLcdScreen：像素 = 分辨率 × 坐标 / 节点数 / 1024，夹在 [0, 分辨率 − 1]）。
// 两个轴都倒过来。倾角不用取反：ASA 的 TiltProcess 在倒序坐标系里按 TX2 − TX1 的坐标差算倾角，
// StylusSolver 在原始坐标系里按 TX1 − TX2 算，翻转与相减次序正好抵消，两者数值相同。
StylusReport TsaCoreSolver::MakeStylusReport(const StylusFrameData& stylus) noexcept {
    StylusReport report;
    const auto& out = stylus.output;
    if (!out.valid || !out.inRange) return report;

    const auto toPixel = [](float coor, int nodes, int res) {
        const float span = static_cast<float>(nodes) * 1024.0f;
        const long pixel = std::lround((span - coor) * static_cast<float>(res) / span);
        return static_cast<uint16_t>(std::clamp<long>(pixel, 0, res - 1));
    };
    report.inRange = true;
    report.tipDown = out.tipDown;
    report.x = toPixel(out.point.x, Frame::kRxCount, TsaHostShim::kXRes);
    report.y = toPixel(out.point.y, Frame::kTxCount, TsaHostShim::kYRes);
    report.tiltX = out.point.tiltX;
    report.tiltY = out.point.tiltY;
    report.pressure = out.tipDown ? out.pressure : 0;
    return report;
}

bool TsaCoreSolver::InStylusMode() const noexcept {
    return m_shim.IsInitialized() && TSA_InStylusMode() != 0;
}

bool TsaCoreSolver::StylusInRange() const noexcept {
    return m_shim.IsInitialized() && (TSA_RptASAInRange() & 0xffu) != 0;
}

void TsaCoreSolver::OnAfeStarted() noexcept {
    m_adapter.Reset();
    m_control.Reset();
    m_mapper.Reset();
    m_actions.clear();
    m_shim.NotifyAfeStarted();
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

const int16_t* TsaCoreSolver::DiffGrid() const noexcept {
    if (!m_shim.IsInitialized()) return nullptr;
    return reinterpret_cast<const int16_t*>(TSA_RptDiffPtr());
}

}  // namespace Solvers::Tsa
