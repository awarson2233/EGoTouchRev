#pragma once
// ══════════════════════════════════════════════════════════════════════
// TsaHostShim — 顶替 ApDaemon 的 TsaFrame：TSACore 的初始化序列、每帧的
// ctrl flags 维护、TSA_ProcessingAFEFrame 调用与帧后的请求查询。
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
    void ProcessFrame(const uint16_t* rawData, const AfeFrameInfo& info, uint64_t timestampMs);

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
    AfeRequests m_requests{};
};

}  // namespace Solvers::Tsa
