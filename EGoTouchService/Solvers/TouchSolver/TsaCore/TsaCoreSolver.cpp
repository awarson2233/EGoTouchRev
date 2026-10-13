#include "TsaCoreSolver.h"

#include <chrono>

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
    }

    const AfeFrameInfo info = m_adapter.Build(frame.rawPtr, frame.rawLen);
    m_lastStatus = info.status;
    // 校验失败或带 SOS 位的帧不进 TSACore；不上报，VHF 保持上一帧状态
    if (!info.valid || (info.status & AfeStatus::kSos) != 0) return false;

    const bool hadContacts = m_mapper.HasActiveContacts();
    m_lastCtrl = m_shim.CtrlFlags();
    m_shim.ProcessFrame(m_adapter.RawData(), info, timestampMs);
    m_actions = m_control.Step(info, m_shim.Requests());
    const int count = m_mapper.Collect(frame.touch.output.contacts);
    m_cachedContactCount.store(count, std::memory_order_relaxed);

    // 上一帧有点、本帧清空时也要走一次上报，让 VHF 发出全抬起
    return count > 0 || hadContacts;
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
