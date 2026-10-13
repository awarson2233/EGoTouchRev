#pragma once
// ══════════════════════════════════════════════════════════════════════
// TsaCoreSolver — 用反编译的 TSACore 手指管线（TSACoreRaw）解算触点的外观类。
//
//   SPI master 帧 ─► HimaxAfeAdapter ─► TsaHostShim ─► TSACoreRaw
//                         ▲                  │              │
//                         │            TsaAfeControl        │
//        宿主执行并回报 ◄──┴── AFE 动作 ◄────┘              │
//                     frame.touch.output.contacts ◄─ TsaOutputMapper
//
// 调用面与 TouchPipeline::Process 相同：返回值表示本帧是否需要走 VHF 上报。
// TSACore 是进程级单例，同一进程里只有第一个完成初始化的实例可用。
// ══════════════════════════════════════════════════════════════════════

#include <atomic>
#include <cstdint>
#include <string>

#include "HimaxAfeAdapter.h"
#include "SolverTypes.h"
#include "TsaAfeControl.h"
#include "TsaHostShim.h"
#include "TsaOutputMapper.h"

namespace Solvers::Tsa {

class TsaCoreSolver {
public:
    /// 处理一帧，时间戳取单调时钟。
    bool Process(HeatmapFrame& frame);
    /// 回放用：显式给定毫秒时间戳，保证输出只取决于输入。
    bool Process(HeatmapFrame& frame, uint64_t timestampMs);

    std::string GetName() const { return "TsaCoreSolver"; }

    // ── 宿主状态机的进入动作 ─────────────────────────────────────────
    /// AFE 重新启动（芯片 Init 成功）：适配层、闭环策略、输出记账回到初始状态，
    /// TSACore 下一帧按首帧处理并重建基线。
    void OnAfeStarted() noexcept;
    /// AFE 停止取帧（灭屏 / 掉线）。在场的触点由宿主负责向 HID 抬起。
    void OnAfeStopped() noexcept;
    /// 宿主观察到 AFE 退出 IDLE（IDLE 期间没有帧喂进来）。
    void OnAfeIdleExit() noexcept { m_shim.NotifyIdleExit(); }

    // ── AFE 闭环 ────────────────────────────────────────────────────
    /// 最近一次 Process 之后要下发给 IC 的动作。没有进 TSACore 的帧不产生动作。
    [[nodiscard]] const AfeActionList& PendingAfeActions() const noexcept { return m_actions; }
    /// 宿主执行完一个动作后回报，适配层据此复刻驱动里的握手状态。
    void OnAfeActionExecuted(const AfeAction& action, bool ok) noexcept;
    void SetAfeLoopEnabled(uint32_t mask) noexcept { m_control.SetEnabled(mask); }
    [[nodiscard]] uint32_t AfeLoopEnabled() const noexcept { return m_control.Enabled(); }
    [[nodiscard]] const AfeRequests& LastAfeRequests() const noexcept { return m_shim.Requests(); }

    /// thp_afe_set_charger：充电器插拔。
    void SetChargerPlugged(bool plugged) noexcept { m_adapter.SetChargerPlugged(plugged); }

    [[nodiscard]] bool IsAvailable() const noexcept { return m_shim.IsInitialized(); }
    [[nodiscard]] uint32_t LastAfeStatus() const noexcept { return m_lastStatus; }
    [[nodiscard]] uint32_t LastCtrlFlags() const noexcept { return m_lastCtrl; }
    [[nodiscard]] int GetCachedContactCount() const noexcept {
        return m_cachedContactCount.load(std::memory_order_relaxed);
    }

    /// 诊断：TSACore 本帧的 diff 网格（40 × 60，int16，驱动倒序后的方向）。未初始化时为空。
    [[nodiscard]] const int16_t* DiffGrid() const noexcept;

private:
    HimaxAfeAdapter  m_adapter;
    TsaHostShim      m_shim;
    TsaAfeControl    m_control;
    TsaOutputMapper  m_mapper;
    AfeActionList    m_actions;
    uint32_t         m_lastStatus = 0;
    uint32_t         m_lastCtrl = 0;
    bool             m_initFailed = false;
    std::atomic<int> m_cachedContactCount{0};
};

}  // namespace Solvers::Tsa
