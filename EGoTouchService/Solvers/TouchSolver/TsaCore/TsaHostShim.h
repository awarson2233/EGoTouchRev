#pragma once
// ══════════════════════════════════════════════════════════════════════
// TsaHostShim — 顶替 ApDaemon 的 TsaFrame：TSACore 的初始化序列、每帧的
// ctrl flags 维护、TSA_ProcessingAFEFrame 调用与帧后的请求查询，以及笔会话
// 状态的下发（ApDaemon 的 DriverThp::GetStatus 与 stylus3 / stylusaft 监听）。
//
// TSACore 的全部状态都在进程级的内存镜像里，所以整个进程只能有一个 shim
// 处于已初始化状态；第二个实例的 Initialize 会失败。
// ══════════════════════════════════════════════════════════════════════

#include <cstdint>

#include "HimaxAfeAdapter.h"
#include "TsaAfeControl.h"
#include "TsaCoreApi.h"

namespace Solvers::Tsa {

/// TSA_ProcessingAFEFrame 第二参的各个位（ApDaemon ThpBase 维护的控制字）。
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

/// 业务侧 StylusSolver 解出的本帧笔报告，坐标已换到 TSACore 的坐标系（显示像素）。
struct StylusReport {
    bool     inRange = false;
    bool     tipDown = false;
    uint16_t x = 0;
    uint16_t y = 0;
    int16_t  tiltX = 0;   // 度
    int16_t  tiltY = 0;
    uint16_t pressure = 0;
};

/// 笔状态字（THP_Service 汇总的 16 位状态，业务侧是 RuntimePenState::factoryStatusFlags）的各个位。
namespace PenStatusWord {
inline constexpr uint16_t kConnected  = 1u << 1;   // PEN_CONN_STATUS
inline constexpr unsigned kCurStatusShift = 2;     // PEN_CUR_STATUS：bit0 休眠，bit1 吸附
inline constexpr unsigned kRotateShift = 6;        // PEN_ROATE_ANGLE，2 位
inline constexpr uint16_t kTouchMode  = 1u << 8;   // PEN_TOUCH_MODE
}  // namespace PenStatusWord

/// TSA_EnableScenario 的位：手写笔防误触场景（ApDaemon 的 "THP#32"）。
inline constexpr uint32_t kScenarioStylusAft = 0x20;

class TsaHostShim {
public:
    /// TSA_RptTouchXPos/YPos 的量程是「显示像素 × kResScale」，与 HID 逻辑量程 25600 × 16000 一致。
    static constexpr uint16_t kXRes = 2560;
    static constexpr uint16_t kYRes = 1600;
    static constexpr uint16_t kResScale = 10;

    TsaHostShim() = default;
    ~TsaHostShim();
    TsaHostShim(const TsaHostShim&) = delete;
    TsaHostShim& operator=(const TsaHostShim&) = delete;

    /// 镜像初始化 + TSA_InitAFEProject_UI + 分辨率 / feature。已有别的实例占用时返回 false。
    bool Initialize();
    [[nodiscard]] bool IsInitialized() const noexcept { return m_initialized; }

    /// 喂一帧适配层输出。info.valid 为假或带 SOS 位的帧不应调用。
    /// rawData 为空表示驱动跳过了 master 的那一帧：TSACore 只跑笔与帧后处理，触点保持上一帧。
    void ProcessFrame(const uint16_t* rawData, const AfeFrameInfo& info, const AfeStylusInfo& stylus,
                      const StylusReport& report, uint64_t timestampMs);

    /// 笔状态字。与 ApDaemon 一样，每次都下发旋转角与笔状态，连接位与防误触位只在变化时处理。
    void SetPenStatusWord(uint16_t word) noexcept;
    /// 蓝牙压力包到达（ApDaemon 收到笔压力时调用 TSA_SetASABluetoothPressure）。
    void OnBtPressure(uint16_t p0, uint16_t p1, uint16_t p2, uint16_t p3) noexcept;

    /// AFE 重新启动（宿主 Init / 亮屏）：下一帧带上首帧 / 重建基线标志。
    void NotifyAfeStarted() noexcept;
    /// 宿主自己观察到的 IDLE 退出（IDLE 期间的帧没有喂进来时用）：下一帧带上退出标志。
    void NotifyIdleExit() noexcept;

    [[nodiscard]] uint32_t CtrlFlags() const noexcept { return m_ctrl; }
    /// 最近一帧处理后 TSACore 给出的请求。
    [[nodiscard]] const AfeRequests& Requests() const noexcept { return m_requests; }

private:
    void QueryRequests() noexcept;

    bool        m_initialized = false;
    uint32_t    m_ctrl = 0;
    uint32_t    m_features = 0;
    bool        m_prevIdle = false;
    bool        m_prevCalibDone = false;
    uint16_t    m_sequence = 0;
    uint16_t    m_penStatusWord = 0;
    AfeRequests m_requests{};
};

}  // namespace Solvers::Tsa
