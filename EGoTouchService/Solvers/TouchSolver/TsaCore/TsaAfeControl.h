#pragma once
// ══════════════════════════════════════════════════════════════════════
// TsaAfeControl — TSACore → AFE 的闭环策略（顶替 ApDaemon TsaFrame::Process 里
// 帧后的那一段）：把本帧的 AFE 状态位与 TSACore 的请求翻成要下发给 IC 的动作。
//
// 这里只做决策，不碰硬件：动作由宿主（DeviceRuntime）执行，回放工具可以直接
// 打印出来看。同一个动作在条件持续期间按固定间隔重发，而不是像 ApDaemon 那样
// 每帧都发——业务侧的命令通道是同步的寄存器读写，每帧重发会拖慢取帧。
// ══════════════════════════════════════════════════════════════════════

#include <array>
#include <cstddef>
#include <cstdint>

#include "HimaxAfeAdapter.h"
#include "TsaCore.h"

namespace Solvers::Tsa {

/// TSACore 每帧处理后给出的请求（ApDaemon 里的 ctrl_buf）。
using AfeRequests = Core::Requests;

enum class AfeActionType : uint8_t {
    ClearStatus,       // param = 要清的状态位（AfeStatus::kCalibDone / kFreqShiftDone）
    ForceToFreqPoint,  // param = 扫描频点索引
    ForceToScanRate,   // param = 扫描率索引
    EnterIdle,
    StartCalibration,
};

struct AfeAction {
    AfeActionType type = AfeActionType::ClearStatus;
    uint8_t param = 0;
};

class AfeActionList {
public:
    void push(AfeActionType type, uint8_t param = 0) noexcept {
        if (m_count < m_items.size()) m_items[m_count++] = AfeAction{type, param};
    }
    void clear() noexcept { m_count = 0; }
    [[nodiscard]] bool empty() const noexcept { return m_count == 0; }
    [[nodiscard]] std::size_t size() const noexcept { return m_count; }
    [[nodiscard]] const AfeAction* begin() const noexcept { return m_items.data(); }
    [[nodiscard]] const AfeAction* end() const noexcept { return m_items.data() + m_count; }

private:
    std::array<AfeAction, 6> m_items{};
    std::size_t m_count = 0;
};

/// 闭环里可以单独开关的环节。
namespace AfeLoop {
inline constexpr uint32_t kStatusAck   = 1u << 0;  // 校准完成 / 跳频完成的清除握手
inline constexpr uint32_t kFreqHop     = 1u << 1;  // 跳频
inline constexpr uint32_t kScanRate    = 1u << 2;  // 扫描率
inline constexpr uint32_t kCalibration = 1u << 3;  // 重新校准
inline constexpr uint32_t kIdle        = 1u << 4;  // 进入 IDLE
inline constexpr uint32_t kAll = kStatusAck | kFreqHop | kScanRate | kCalibration | kIdle;
}  // namespace AfeLoop

class TsaAfeControl {
public:
    /// 条件持续时的重发间隔（帧）。120 Hz 下约 100 ms，足够 IC 处理完上一条命令。
    static constexpr int kResendFrames = 12;
    /// 两次校准命令之间的最小间隔（帧）。校准期间 TSACore 的请求会一直保持。
    static constexpr int kCalibrationHoldoffFrames = 240;
    /// ApDaemon：IDLE 请求成立而 AFE 还没进 IDLE 时，每 5 帧发一次进入命令。
    static constexpr int kIdleRequestPeriod = 5;

    void SetEnabled(uint32_t mask) noexcept { m_enabled = mask; }
    [[nodiscard]] uint32_t Enabled() const noexcept { return m_enabled; }

    /// AFE 重新启动后调用。宿主的启动序列自己做过一次校准，TSACore 首帧的
    /// 校准请求因此视为已满足。
    void Reset() noexcept {
        m_ackCalibCountdown = 0;
        m_ackFreqCountdown = 0;
        m_freqCountdown = 0;
        m_rateCountdown = 0;
        m_lastFreqIndex = -1;
        m_lastRateIndex = -1;
        m_idleRequestFrames = 0;
        m_calibrationCountdown = kCalibrationHoldoffFrames;
    }

    /// 每个进了 TSACore 的帧调用一次。
    AfeActionList Step(const AfeFrameInfo& info, const AfeRequests& req) noexcept {
        AfeActionList out;
        Tick(m_ackCalibCountdown);
        Tick(m_ackFreqCountdown);
        Tick(m_freqCountdown);
        Tick(m_rateCountdown);
        Tick(m_calibrationCountdown);

        if (m_enabled & AfeLoop::kStatusAck) {
            StepAck(out, info.status, AfeStatus::kCalibDone, m_ackCalibCountdown);
            StepAck(out, info.status, AfeStatus::kFreqShiftDone, m_ackFreqCountdown);
        }
        if (m_enabled & AfeLoop::kFreqHop) {
            StepIndex(out, AfeActionType::ForceToFreqPoint, req.scanFreqIndex, m_lastFreqIndex, m_freqCountdown);
        }
        if (m_enabled & AfeLoop::kScanRate) {
            StepIndex(out, AfeActionType::ForceToScanRate, req.scanRateIndex, m_lastRateIndex, m_rateCountdown);
        }
        if ((m_enabled & AfeLoop::kCalibration) && req.reCalibNeeded && m_calibrationCountdown == 0) {
            out.push(AfeActionType::StartCalibration);
            m_calibrationCountdown = kCalibrationHoldoffFrames;
        }

        const bool afeIdle = (info.status & AfeStatus::kIdle) != 0;
        if ((m_enabled & AfeLoop::kIdle) && req.okToEnterIdle && !afeIdle) {
            if (m_idleRequestFrames % kIdleRequestPeriod == 0) out.push(AfeActionType::EnterIdle);
            ++m_idleRequestFrames;
        } else {
            m_idleRequestFrames = 0;
        }
        return out;
    }

private:
    static void Tick(int& countdown) noexcept {
        if (countdown > 0) --countdown;
    }

    static void StepAck(AfeActionList& out, uint32_t status, uint32_t bit, int& countdown) noexcept {
        if ((status & bit) == 0) {
            countdown = 0;
            return;
        }
        if (countdown != 0) return;
        out.push(AfeActionType::ClearStatus, static_cast<uint8_t>(bit));
        countdown = kResendFrames;
    }

    static void StepIndex(AfeActionList& out, AfeActionType type, int requested, int& last, int& countdown) noexcept {
        if (requested < 0) {
            last = -1;
            countdown = 0;
            return;
        }
        if (requested == last && countdown != 0) return;
        out.push(type, static_cast<uint8_t>(requested));
        last = requested;
        countdown = kResendFrames;
    }

    uint32_t m_enabled = AfeLoop::kAll;
    int m_ackCalibCountdown = 0;
    int m_ackFreqCountdown = 0;
    int m_freqCountdown = 0;
    int m_rateCountdown = 0;
    int m_calibrationCountdown = kCalibrationHoldoffFrames;
    int m_lastFreqIndex = -1;
    int m_lastRateIndex = -1;
    int m_idleRequestFrames = 0;
};

}  // namespace Solvers::Tsa
