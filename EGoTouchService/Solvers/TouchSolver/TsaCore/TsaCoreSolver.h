#pragma once
// ══════════════════════════════════════════════════════════════════════
// TsaCoreSolver — 用反编译的 TSACore 手指管线（TsaCore）解算触点的外观类，同时顶替 ApDaemon 的
// TsaFrame：TSACore 的初始化、每帧的 ctrl 控制字维护与帧后的请求查询，以及笔会话状态的下发
// （DriverThp::GetStatus 与 stylus3 / stylusaft 监听）。
//
//   SPI master 帧 ─► HimaxAfeAdapter ─► TsaCoreSolver ─► TsaCore（Core::ProcessFrame）
//   SPI slave 帧  ─►   （笔帧状态）      ▲   │                  │
//   frame.stylus.output（StylusSolver）──┘   │                  │
//                         ▲            TsaAfeControl            │
//        宿主执行并回报 ◄──┴── AFE 动作 ◄────┘                  │
//                     frame.touch.output.contacts ◄─ TsaOutputMapper
//
// Process 的返回值表示本帧是否需要走 VHF 上报。
// 笔的坐标仍由 StylusSolver 解算；这里把它的结果连同驱动的笔帧状态送进 TSACore，
// 手指侧依赖笔状态的逻辑（StylusAft 的手掌 / 笔尖误触抑制、笔模式下不进 IDLE、
// 不跳频）按原版运行。调用前 frame.stylus 必须已经由 StylusPipeline 处理过。
// TSACore 是进程级单例，同一进程里只有第一个完成初始化的实例可用。
// ══════════════════════════════════════════════════════════════════════

#include <atomic>
#include <cstdint>
#include <string>
#include <string_view>

#include "HimaxAfeAdapter.h"
#include "SolverTypes.h"
#include "TsaAfeControl.h"
#include "TsaCore.h"
#include "TsaOutputMapper.h"

namespace Solvers::Tsa {

/// Core::ProcessFrame 第二参的各个位（ApDaemon ThpBase 维护的控制字）。
namespace CtrlFlag {
inline constexpr uint32_t kFirstFrame      = 1u << 0;   // 一次性：Init / 亮屏
inline constexpr uint32_t kResetBaseline   = 1u << 1;   // 一次性：Init / 亮屏
inline constexpr uint32_t kIdleExit        = 1u << 2;   // 一次性：退出 IDLE
inline constexpr uint32_t kCalibDone       = 1u << 3;   // 一次性：status & 0x08 的上升沿
inline constexpr uint32_t kFreqShiftDone   = 1u << 4;   // 一次性：status & 0x04
inline constexpr uint32_t kIdle            = 1u << 5;   // 持续：status & 0x01
inline constexpr uint32_t kScreenOff       = 1u << 8;   // 持续：灭屏
inline constexpr uint32_t kChargerNoise    = 1u << 17;  // 持续：status & 0x20
inline constexpr uint32_t kAlways          = 1u << 18;  // 常驻
/// 每帧调用后保留的位：清掉 bit0–4 的一次性标志与 bit24–27。
inline constexpr uint32_t kPersistMask     = 0xf0ffffe0u;
}  // namespace CtrlFlag

/// 笔状态字（THP_Service 汇总的 16 位状态，业务侧是 RuntimePenState::factoryStatusFlags）的各个位。
namespace PenStatusWord {
inline constexpr uint16_t kConnected  = 1u << 1;   // PEN_CONN_STATUS
inline constexpr unsigned kCurStatusShift = 2;     // PEN_CUR_STATUS：bit0 休眠，bit1 吸附
inline constexpr unsigned kRotateShift = 6;        // PEN_ROATE_ANGLE，2 位
inline constexpr uint16_t kTouchMode  = 1u << 8;   // PEN_TOUCH_MODE
}  // namespace PenStatusWord

/// Core::EnableScenario 的位：手写笔防误触场景（ApDaemon 的 "THP#32"）。
inline constexpr uint32_t kScenarioStylusAft = 0x20;

/// 笔报告的状态位（Core::StylusReport::status）。
namespace StylusReportFlag {
inline constexpr uint32_t kInRange = 1u << 0;  // 悬停在场
inline constexpr uint32_t kTipDown = 1u << 1;  // 书写
}  // namespace StylusReportFlag

class TsaCoreSolver {
public:
    TsaCoreSolver() = default;
    ~TsaCoreSolver();
    TsaCoreSolver(const TsaCoreSolver&) = delete;
    TsaCoreSolver& operator=(const TsaCoreSolver&) = delete;

    /// 处理一帧，时间戳取单调时钟。
    bool Process(HeatmapFrame& frame);
    /// 回放用：显式给定毫秒时间戳，保证输出只取决于输入。
    bool Process(HeatmapFrame& frame, uint64_t timestampMs);

    std::string GetName() const { return "TsaCoreSolver"; }

    /// 面板机型（显示驱动 OTP 的 project id）：选 AFE 的 PixelGain 表与 TSACore 的机型参数表。
    /// TSACore 在第一帧初始化、之后不再换表，所以初始化之后只接受同一个机型，换机型返回 false。
    bool SetPanelProjectId(std::string_view projectId);
    [[nodiscard]] const std::string& PanelProjectId() const noexcept { return m_projectId; }
    /// TSACore 实际装入的参数表名（没有专表的机型是 "DFLT000000"）。未初始化时为空。
    [[nodiscard]] const char* LoadedPrmtName() const noexcept;

    // ── 宿主状态机的进入动作 ─────────────────────────────────────────
    /// AFE 重新启动（芯片 Init 成功）：适配层、闭环策略、输出记账回到初始状态，
    /// TSACore 下一帧按首帧处理并重建基线。
    void OnAfeStarted() noexcept;
    /// AFE 停止取帧（灭屏 / 掉线）。在场的触点由宿主负责向 HID 抬起。
    void OnAfeStopped() noexcept;
    /// 宿主观察到 AFE 退出 IDLE（IDLE 期间没有帧喂进来）：下一帧带上退出标志。
    void OnAfeIdleExit() noexcept { m_ctrl |= CtrlFlag::kIdleExit; }

    // ── AFE 闭环 ────────────────────────────────────────────────────
    /// 最近一次 Process 之后要下发给 IC 的动作。没有进 TSACore 的帧不产生动作。
    [[nodiscard]] const AfeActionList& PendingAfeActions() const noexcept { return m_actions; }
    /// 宿主执行完一个动作后回报，适配层据此复刻驱动里的握手状态。
    void OnAfeActionExecuted(const AfeAction& action, bool ok) noexcept;
    void SetAfeLoopEnabled(uint32_t mask) noexcept { m_control.SetEnabled(mask); }
    [[nodiscard]] uint32_t AfeLoopEnabled() const noexcept { return m_control.Enabled(); }
    /// 最近一帧处理后 TSACore 给出的请求。
    [[nodiscard]] const AfeRequests& LastAfeRequests() const noexcept { return m_requests; }

    /// thp_afe_set_charger：充电器插拔。
    void SetChargerPlugged(bool plugged) noexcept { m_adapter.SetChargerPlugged(plugged); }

    // ── 笔会话 ──────────────────────────────────────────────────────
    // 这三个可以在任意线程调用，实际写入 TSACore 发生在下一次 Process 开头。
    /// thp_afe_set_stylus_connect：笔是否已连接。
    void SetStylusConnected(bool connected) noexcept {
        m_stylusConnected.store(connected, std::memory_order_relaxed);
    }
    /// 笔状态字（RuntimePenState::factoryStatusFlags）。
    void SetPenStatusWord(uint16_t word) noexcept { m_penStatusWord.store(word, std::memory_order_relaxed); }
    /// 蓝牙压力包到达。压力值只给 ASA 解算本体用（本移植没有），TSACore 只记下蓝牙在线。
    void OnBtPressure() noexcept { m_btPressureSeq.fetch_add(1, std::memory_order_release); }

    /// 诊断：TSACore 是否处于笔模式 / 是否认为笔在场（最近一次 Process 之后）。
    [[nodiscard]] bool InStylusMode() const noexcept;
    [[nodiscard]] bool StylusInRange() const noexcept;
    /// 诊断：最近一次 Process 交给 TSACore 的笔帧与笔报告。
    [[nodiscard]] const AfeStylusInfo& LastStylusInfo() const noexcept { return m_lastStylus; }
    [[nodiscard]] const Core::StylusReport& LastStylusReport() const noexcept { return m_lastReport; }

    [[nodiscard]] bool IsAvailable() const noexcept { return m_initialized; }
    [[nodiscard]] uint32_t LastAfeStatus() const noexcept { return m_lastStatus; }
    [[nodiscard]] uint32_t LastCtrlFlags() const noexcept { return m_lastCtrl; }
    [[nodiscard]] int GetCachedContactCount() const noexcept {
        return m_cachedContactCount.load(std::memory_order_relaxed);
    }

    /// 诊断：TSACore 本帧的 diff 网格（40 × 60，int16，驱动倒序后的方向）。未初始化时为空。
    [[nodiscard]] const int16_t* DiffGrid() const noexcept;

private:
    bool InitCore();
    void ApplyPenSession() noexcept;
    void ApplyPenStatusWord(uint16_t word) noexcept;
    void UpdateCtrl(uint32_t status) noexcept;
    static Core::StylusReport MakeStylusReport(const StylusFrameData& stylus) noexcept;

    std::string      m_projectId{kDefaultPanelProjectId};
    HimaxAfeAdapter  m_adapter;
    TsaAfeControl    m_control;
    TsaOutputMapper  m_mapper;
    AfeActionList    m_actions;
    AfeRequests      m_requests{};
    uint32_t         m_lastStatus = 0;
    uint32_t         m_lastCtrl = 0;
    bool             m_initialized = false;
    bool             m_initFailed = false;
    std::atomic<int> m_cachedContactCount{0};

    // ApDaemon 的控制字与 SetFeatures 的值
    uint32_t m_ctrl = 0;
    uint32_t m_features = 0;
    bool     m_prevIdle = false;
    bool     m_prevCalibDone = false;
    // TSACore 保存帧的拷贝，帧里的笔帧指针在处理期间被读取，所以笔帧放在成员里
    Core::StylusFrame m_stylusFrame{};

    std::atomic<bool>     m_stylusConnected{false};
    std::atomic<uint16_t> m_penStatusWord{0};
    std::atomic<uint32_t> m_btPressureSeq{0};   // 收到的压力包个数
    uint32_t              m_btPressureSeen = 0;
    uint16_t              m_appliedPenStatusWord = 0;
    bool                  m_penStatusApplied = false;
    AfeStylusInfo         m_lastStylus{};
    Core::StylusReport    m_lastReport{};
};

}  // namespace Solvers::Tsa
