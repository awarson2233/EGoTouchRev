#pragma once
// ══════════════════════════════════════════════════════════════════════
// TouchPipeline Stage 7 —— TouchReport 上报网闸 / 自适应 IIR / JIT 迟滞死区
//                          / 历史回放 / ExtInfo
//
// 逐函数对位 TSACore.dll（image base 0x6BA40000）在 W273AS2700
// （Gaokun Himax CSOT，60 列 × 40 行，2560 × 1600，1 mm = 60 Q8）上的行为。
// 规范：docs/finger_algorithms/07_filtering_and_jitter.md §1–§17。
//
// 原厂调用链：
//   TouchReport_Process            @0x6BA7BE73   §1
//     TouchReport_ProcessUnit      @0x6BA7B86E   §1/§4
//       TouchReport_ToBeReported   @0x6BA7AFBC   §2   十条否决
//       TouchReport_IsHistoryBufferApplied @0x6BA7B373  §3  回放旁路
//       Filter_ProcessUnit         @0x6BA5FD96   §6
//         Filter_ReportedProcessUnit          @0x6BA5FB41  §7
//           Filter_ModeTouchReportedProcessUnit @0x6BA5F1C2 §8  冻结四分支
//             Filter_ReportClickToFlickProcessUnit @0x6BA5E6C3 §8
//             Filter_IIRPDims     @0x6BA5DCD4   §9.4
//               Filter_GetCoefBasedOnDistance @0x6BA5C99C §9.1
//               Filter_GetAdaptiveCoef        @0x6BA5C690 §9.3
//               AIIR_GetCoef                  @0x6BA60040 §9.2
//               Filter_IsCurveReturn          @0x6BA5D178 §10.1
//               Filter_IsPredictFinish        @0x6BA5D662 §10.2
//               Filter_ProcessPDim1/2 → IIR_Process @0x6BA601B0 §9.5
//             Filter_JitFilterProcessUnit @0x6BA5E994 → JITFilter @0x6BA602C6 §11
//       TouchReport_UpdateStartReportPoint @0x6BA7B657 §4
//   Touch_ProcessExtInfo           @0x6BA7713D   §13
//
// 整段省略（§5 / §12 / §16，门控实测关闭且不产出下游字段）：
//   · 姿态回退与多步回退（DAT_6bcaa14c 无写入者）
//   · HotZoneAdjust 全链（dwFeatureFlags bit22 = 0）
//   · Filter_IsTouchLiftOff 与二次 JIT（flash 0x8A2 = 0）
//   · +0x1F8 bit6 的系数指数恢复（全库无写入者）
//   · 特性 0x1000 系数表（dwFeatureFlags bit12 = 0）
//   · DAT_6bcab614 / 616 / 618（全库无读者）
//
// **原厂缺陷照抄、不得修正**（§3）：
//   · Touch_CreatePastReport 补插条目的时间戳是 `now − k·dt`，**递减**，
//     与正常压入的递增时间戳混在同一队列；
//   · Filter_IsCurveReturn 的 tsum 是 uint32，遇负步长回绕并提前退出，
//     于是回放后 ≤4 帧内折返检测被静默关闭（保守失效）。
// ══════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>

#include "SolverTypes.h"
#include "TouchCoreTypes.hpp"
#include "TouchFixed.hpp"
#include "TouchFrameTypes.h"
#include "TouchGates.hpp"

#include "config/ConfigBinder.h"
#include "config/ConfigStore.h"

namespace Solvers::Touch {

// ── touch + 0x1F8 持久标志字（§15）──────────────────────────────────────
enum TouchReportFlags : uint32_t {
    TOUCH_FLAG_REPORTED        = 0x00000008, // bit3  本帧获准上报
    TOUCH_FLAG_HEAVY_FILTER    = 0x00000010, // bit4  重阻尼锁存（粘滞）
    TOUCH_FLAG_RAMP_ALPHA      = 0x00000040, // bit6  系数指数恢复：**全库无写入者**
    TOUCH_FLAG_HIST_POP        = 0x00000080, // bit7  需要历史回放
    TOUCH_FLAG_MOVED_20PX      = 0x00000100, // bit8  距起报点 > 20 外部像素
    TOUCH_FLAG_CLICK_FREEZE    = 0x00000800, // bit11 点击冻结中
    TOUCH_FLAG_MOVED_50PX      = 0x00001000, // bit12 距 +0x4AC 基准 > 50 外部像素
    TOUCH_FLAG_FAST_MOVE       = 0x00008000, // bit15 距按下点 > 60 外部像素
    TOUCH_FLAG_POSTURE_CANCEL  = 0x00010000, // bit16 姿态取消（不可达）
    TOUCH_FLAG_REPORT_RESET    = 0x00020000, // bit17 已建立回放（重初始化触发）
    TOUCH_FLAG_FAST_CLICK_CAND = 0x00040000, // bit18 快速点击候选（判据 6 的前置）
    TOUCH_FLAG_CUR_REPORTED    = 0x00100000, // bit20 本帧已通过上报判决
    TOUCH_FLAG_FORCE_ID_BREAK  = 0x00200000, // bit21 回放期间遇抬起，强制断 ID
    TOUCH_FLAG_ROLLBACK_ACTIVE = 0x00800000, // bit23 回退活跃（不可达）
    TOUCH_FLAG_POSTURE_BLOCK   = 0x20000000, // bit29 姿态阻断
};

// ── touch + 0x1F0 易失标志字 ───────────────────────────────────────────
enum TouchVolatileFlags : uint32_t {
    TOUCH_VFLAG_ZONE_TYPE5      = 0x00002000, // 区域 type == 5（大物体），判据 10
    TOUCH_VFLAG_SMARTCOVER_ZONE = 0x00020000, // ZONE_FLAG_BIG_SHAPE，判据 2
    TOUCH_VFLAG_FALSE_FLICK     = 0x00100000, // 假滑动（手指滚动），冻结分支 2
    TOUCH_VFLAG_GRIP_SUPPRESS   = 0x08000000, // 握持剔除（08 读，不在本网闸内）
    TOUCH_VFLAG_GRIP_CHANNEL    = 0x20000000, // ZONE_FLAG_NEIGHBOR_NOISE，判据 3
};

// ── touch + 0x208 抑制标志字 ───────────────────────────────────────────
enum TouchSuppressFlags : uint32_t {
    SUPPRESS_GLOVE_WEAK   = 0x02, // TouchMode_GloveFilter，判据 6a
    SUPPRESS_BY_STYLUS    = 0x04, // TouchMode_StylusSuppression，判据 6b
    SUPPRESS_FEATURE_1000 = 0x08, // 判据 7
    SUPPRESS_FEATURE_2000 = 0x20, // 判据 8
};

struct Point2D_Q8 {
    int32_t x = 0;
    int32_t y = 0;
};

// ══════════════════════════════════════════════════════════════════════
// Stage 7 主模块
// ══════════════════════════════════════════════════════════════════════
class TouchReportAndFilter {
public:
    static constexpr int kCols = kSensorCols;
    static constexpr int kRows = kSensorRows;
    static constexpr size_t kMaxSlots = kMaxTracks;

    // ── W273AS2700 实测常量（§16）─────────────────────────────────────
    /// DAT_6bcab620 / DAT_6bcab622：两维内部量程上限（Q8）。
    static constexpr int32_t kDim1Full = kCols * 256;   // 15360
    static constexpr int32_t kDim2Full = kRows * 256;   // 10240
    /// DAT_6bcab612 = flash 0x8A1(6) · 60 · 256 / 2560 = 36 Q8 = 6 显示像素。
    /// 两轴同半径：Dim2 的 DAT_6bcab614 = 38 全库无读者（§11.2）。
    static constexpr uint32_t kDeadbandRQ8 = 36;
    /// Filter_JitFilterProcessUnit 的单轴牵引补偿
    /// M = flash._1446_2_ ? DAT_6bcab620 / 2560 + 1 : 2 = 15360/2560 + 1 = 7（§11.2）。
    static constexpr int32_t kJitMaxStepQ8 = 7;
    /// Filter_IIRPDims 的边界直通门限 0x31（§9.4）。
    static constexpr int32_t kEdgeBypassStepQ8 = 0x31;
    /// Filter_IsNeedCurve 的离边余量 0x180 = 1.5 格（§10.1）。
    static constexpr int32_t kCurveMarginQ8 = 0x180;
    /// TSAOut_ExtToIntDist(flash 0x90A = 3) = (3 << 16) / DAT_6bcab604(10922) = 18 Q8（§8）。
    static constexpr int32_t kClickToFlickStepQ8 = 18;
    /// TS_MMToPixel(2) = 2 · 60 = 120 Q8，Touch_GetPostReportLength 的分母（§3）。
    static constexpr int32_t kPostReportSegQ8 = 120;
    /// PREVTOUCH_HISTORY_BUFFER_SIZE：PrevTouch_HistoryBufferPush 的容量断言 0x1E。
    static constexpr uint8_t kHistCap = 30;

    // ── IIR 系数表（§9.1，Filter_GetCoefBasedOnDistance 的两组运行时全局）──
    // 正常表：DAT_6bca928e/90/92/93 = 0 / 384 / 0 / 32，斜率 DAT_6bcab60c = 85。
    // **lightCoef 是 0 不是 1**：α(0) = 0 才有「零位移完全锁死」，
    // AIIR_GetCoef 末尾的 `α == 0 ∧ d != 0 → 1` 负责 0 < d ≤ 12 的下限。
    uint16_t m_lightDelta             = 0;
    uint16_t m_heavyDelta             = 384;
    uint8_t  m_lightCoef              = 0;
    uint8_t  m_heavyCoef              = 32;
    uint16_t m_scaledRatio            = 85;
    // 重阻尼表：DAT_6bca9294/96/98/99 = 0 / 512 / 0 / 12，斜率 DAT_6bcab60e = 24。
    uint16_t m_heavyFilterLightDelta  = 0;
    uint16_t m_heavyFilterHeavyDelta  = 512;
    uint8_t  m_heavyFilterLightCoef   = 0;
    uint8_t  m_heavyFilterHeavyCoef   = 12;
    uint16_t m_heavyFilterScaledRatio = 24;

    // ── 门控（TouchGates 单一事实源，§16）──────────────────────────────
    /// dwFeatureFlags bit8 FEAT_COORD_FREEZE：冻结三分支总开关，本机**开**。
    static constexpr bool kCoordFreezeOn = Gates::IsOn("FEAT_COORD_FREEZE");
    /// dwFeatureFlags bit12：冻结否决位，本机 0。
    static constexpr bool kFreezeVetoOn  = (Gates::kFeatureFlags & 0x1000u) != 0;
    static constexpr bool kSmartCoverOn  = Gates::IsOn("FEAT_SMART_COVER");
    static constexpr bool kFeature0x1000 = kFreezeVetoOn;
    static constexpr bool kFeature0x2000 = (Gates::kFeatureFlags & 0x2000u) != 0;
    /// flash 0x8A2 = 0 ⇒ Filter_IsTouchLiftOff 恒 0、二次 JIT 不执行（§9.4）。
    static constexpr bool kLiftOffZDecayOn = Gates::IsOn("LIFTOFF_Z_DECAY");
    /// dwFeatureFlags bit22 = 0 ⇒ HotZone 全链不执行（§12）。
    static constexpr bool kHotZoneOn       = Gates::IsOn("FEAT_HOTZONE");
    /// DAT_6bcaa14c 无写入者 ⇒ 姿态回退 / 多步回退整链不可达（§5）。
    static constexpr bool kMultiStepBackOn = Gates::IsOn("POSTURE_MULTISTEP_BACK");
    /// DAT_6bca92a3（flash 0x8A3）= 0 ⇒ 走「外推 + 混合」分支（§10.2）。
    static constexpr bool kPredictDeadbandScaling = false;

    /// TSA_IsGameScenario：随宿主场景位，缺省关。
    bool m_gameScenario = false;
    bool m_enabled = true;

    // ── 帧级统计（§1）────────────────────────────────────────────────
    uint8_t  m_reportedTouchCount  = 0;  // g_touchesPtr[5]
    uint8_t  m_palmTouchCount      = 0;  // g_touchesPtr[0xB]
    uint8_t  m_cancelledTouchCount = 0;  // g_touchesPtr[3]
    uint32_t m_zValPrevToCurRatio  = 0;  // g_zValPrevToCurRatio，抬手 Z 衰减遥测，恒 0
    // Filter_ProcessUnit / Filter_IIRPDims 的四个遥测全局（§6 / §9.5）。
    uint32_t m_xPosPreFilter = 0, m_yPosPreFilter = 0;
    uint32_t m_xPosAftIirFilter = 0, m_yPosAftIirFilter = 0;
    /// Touch_ProcessExtInfo 叠加到帧头 +0x18 的全局标志（§13）。
    uint32_t m_extInfoFlags = 0;

    // ── 回放 FIFO 条目：原版 prev + 0x290 + 0x10·i，16 字节 ─────────────
    //   { u16 x; u16 y; pad4; s64 ts }
    struct HistEntry {
        uint16_t x = 0;
        uint16_t y = 0;
        int64_t  ts = 0;
    };

    // ── 历史单元：对位 g_pPrevTouches + j·0x478 ────────────────────────
    // 帧首 Touch_UnitCopy 把历史 0x140..0x283 段整段拷进当前，帧尾
    // PrevTouch_RemoveOff 再把当前整段拷回历史 ⇒ 下列字段跨帧自持（§概念）。
    struct SlotFilterState {
        uint8_t  id = 0;
        bool     valid = false;          // 历史单元是否存在（首帧为假）

        // 0x140..0x283 段（当前池 / 历史池语义相同）
        uint16_t raw162 = 0, raw164 = 0; // +0x162/+0x164 上一帧工作坐标
        uint16_t iir166 = 0, iir168 = 0; // +0x166/+0x168 IIR 输出 = 16.8 整数部
        uint8_t  rem16A = 0, rem16B = 0; // +0x16A/+0x16B  16.8 小数部
        uint16_t jit16C = 0, jit16E = 0; // +0x16C/+0x16E JIT 输出
        uint16_t out170 = 0, out172 = 0; // +0x170/+0x172 上报坐标
        uint32_t flags1F8 = 0;           // +0x1F8 持久标志字
        uint32_t action224 = 1;          // +0x224 TouchAction 动作字
        uint32_t deadband27C = kDeadbandRQ8; // +0x27C 死区半径
        uint8_t  fastMove279 = 0;        // +0x279 曾快速移动锁存
        uint8_t  handLiftTail280 = 0;    // +0x280 折返尾 / 抬手计数臂

        // 仅历史池（+0x28C 之后）
        int16_t  clickOff4EE = 0;        // +0x4EE 冻结期累积偏移 Dim1
        int16_t  clickOff4F0 = 0;        // +0x4F0 同上 Dim2
        uint8_t  rampAlpha4F3 = 0;       // +0x4F3 指数恢复值（死枝，恒 0）
        uint8_t  alphaLatch4F4 = 0;      // +0x4F4 α 迟滞锁存，0 视为未初始化
        uint16_t downAnchorX496 = 0;     // +0x496/+0x498 按下锚点
        uint16_t downAnchorY498 = 0;
        uint16_t startRptX49A = 0;       // +0x49A/+0x49C 起报点
        uint16_t startRptY49C = 0;
        uint16_t clickBaseX4A4 = 0;      // +0x4A4/+0x4A6 点击抑制基准
        uint16_t clickBaseY4A6 = 0;
        uint16_t flickBaseX4AC = 0;      // +0x4AC/+0x4AE 假滑动基准
        uint16_t flickBaseY4AE = 0;
        uint8_t  turnAxis5AC = 0;        // +0x5AC 主轴：1 = Dim1
        uint8_t  predStep5AD = 0;        // +0x5AD 预测帧计数
        uint8_t  turnNum5AE = 0;         // +0x5AE 折返总帧数
        uint8_t  fwdNum5AF = 0;          // +0x5AF 前瞻帧数
        int16_t  predX5B0 = 0;           // +0x5B0/+0x5B2 外推点
        int16_t  predY5B2 = 0;

        // 回放 FIFO：+0x288 条数 + +0x290 起的条目
        int64_t  lastTs148 = 0;          // +0x148 上一帧时间戳
        uint8_t  histCount288 = 0;
        std::array<HistEntry, kHistCap> hist{};

        int32_t  eventTransition290 = 0; // +0x290 事件迁移统计（纯遥测）
    };

    TouchReportAndFilter() noexcept { Reset(); }

    void Reset() noexcept {
        m_slotStates.fill(SlotFilterState{});
        m_reportedTouchCount = 0;
        m_palmTouchCount = 0;
        m_cancelledTouchCount = 0;
        m_zValPrevToCurRatio = 0;
        m_extInfoFlags = 0;
    }
    void ClearLiveState() noexcept { Reset(); }

    void SetEnabled(bool e) noexcept { m_enabled = e; }
    [[nodiscard]] bool IsEnabled() const noexcept { return m_enabled; }
    [[nodiscard]] uint8_t GetReportedTouchCount() const noexcept { return m_reportedTouchCount; }
    [[nodiscard]] uint8_t GetPalmTouchCount() const noexcept { return m_palmTouchCount; }
    [[nodiscard]] const SlotFilterState* GetSlotState(uint8_t id) const noexcept {
        return (id > 0 && id <= 20) ? &m_slotStates[id] : nullptr;
    }

    // ══════════════════════════════════════════════════════════════════
    // §1 主调度 TouchReport_Process @0x6BA7BE73
    // ══════════════════════════════════════════════════════════════════
    // 三件事：清帧级计数、按**当前池下标顺序**逐点调用 ProcessUnit、尾部批量吸附。
    // 尾部批处理依赖 TouchReport_IsMultStepBackEnable = (DAT_6bcaa14c & 2)，
    // 本机恒假，整段省略（§5）。每个触点都会进 ProcessUnit，**没有前置条件**。
    bool Process(TouchBigFrame& bigFrame) {
        if (!m_enabled) return true;

        m_reportedTouchCount  = 0;   // g_touchesPtr[5]
        m_palmTouchCount      = 0;   // g_touchesPtr[0xB]
        m_cancelledTouchCount = 0;   // g_touchesPtr[3]
        m_zValPrevToCurRatio  = 0;   // 抬手 Z 衰减遥测，本机恒 0（§9.4）

        std::array<bool, 21> alive{};
        for (auto& track : bigFrame.tracks) {
            if (track.id == 0 || track.id > 20) continue;
            alive[track.id] = true;
            TouchReport_ProcessUnit(track, bigFrame);
        }

        bigFrame.cancelledTouchCount = m_cancelledTouchCount;
        SynchronizeContacts(bigFrame);

        // §13 Touch_ProcessExtInfo @0x6BA7713D：紧随 TouchReport_Process 之后。
        Touch_ProcessExtInfo(bigFrame);

        // PrevTouch_UnitRelease：轨迹消失即整单元归零（§7 提到的生命周期归零点）。
        for (size_t id = 1; id <= 20; ++id) {
            if (!alive[id]) m_slotStates[id] = SlotFilterState{};
        }
        return true;
    }

    bool Process(HeatmapFrame& frame) {
        if (!m_enabled) return true;
        if (frame.touch.bigFrame == nullptr) return true;
        const bool ok = Process(*frame.touch.bigFrame);
        for (auto& c : frame.touch.output.contacts) {
            for (const auto& t : frame.touch.bigFrame->tracks) {
                if (t.id != c.id) continue;
                c.isReported = t.isReported;
                c.lifeFlags  = t.lifeFlags;
                c.x = static_cast<float>(t.fltX_q8) / 256.0f;
                c.y = static_cast<float>(t.fltY_q8) / 256.0f;
                break;
            }
        }
        return ok;
    }

    // ══════════════════════════════════════════════════════════════════
    // §2 上报网闸 TouchReport_ToBeReported @0x6BA7AFBC
    // ══════════════════════════════════════════════════════════════════
    // 初值来自 TouchReport_PostureAntiTouch（返回真即否决），随后十条独立的 if
    // 只会把结果往「假」推，**没有任何一条能把已经为假的结果翻回真**。
    // 判据 9 读的是**未被回放改写前**的 TE 值 —— 本函数在
    // TouchReport_IsHistoryBufferApplied 之前调用，顺序不可换（§17.1）。
    [[nodiscard]] bool TouchReport_ToBeReported(const TrackingSlot& track,
                                               const SlotFilterState& prev,
                                               const TouchBigFrame& bigFrame) noexcept {
        // (1) 初值：TouchReport_PostureAntiTouch @0x6BA7AE86。三个触发源
        //     （SensorData_IsAntiTouchStage / IsAnyTouchCancelByPosture /
        //      prev +0x200 bit0）在本机同时为假 ⇒ 恒「不拒」（§5）。
        bool allow = !TouchReport_PostureAntiTouch(track, prev);

        // (2) header[0x24] & 8 ∧ BLReset_InDebounce() ∧ (¬SmartCover ∨ +0x1F0 & 0x20000)
        if ((bigFrame.touchGlobalFlags & 0x8u) != 0 && bigFrame.baselineResetInDebounce &&
            (!kSmartCoverOn || (track.volatileFlags & TOUCH_VFLAG_SMARTCOVER_ZONE) != 0)) {
            allow = false;
        }

        // (3) +0x1F0 & 0x20000000（区域噪声压过峰 / 握持通道）
        if ((track.volatileFlags & TOUCH_VFLAG_GRIP_CHANNEL) != 0 ||
            track.isGrip || (track.lifeFlags & LifeGripSuppressed) != 0) {
            allow = false;
        }

        // (4) +0x1EC == 0x40（**等值**比较，与 header[0xB] 的位测试口径不同）∧ ¬SmartCover。
        //     0x40 只由 HI 分支的 HIException_* 产出，本机 FI 模式不可达。
        if (track.exceptionType == ZONE_EXC_PALM && !kSmartCoverOn) {
            allow = false;
        }

        // (5) +0x1F4 & 1 ∧ +0x21C != 0x10。原版**只豁免 0x10**，
        //     0x40（WindowsPad 抬起帧）若带该标志同样被否决。
        if ((track.volatileFlags2 & TOUCH_FLAG2_LIFTOFF_Z_FAILED) != 0 &&
            track.state != EventRescue) {
            allow = false;
        }

        // (6) 只在触点**第一次尝试上报**的那一帧检查：
        //     prev +0x1F8 & 8 == 0 ∧ (¬(prev +0x1F8 & 0x40000) ∨ Touch_IsException)。
        if ((prev.flags1F8 & TOUCH_FLAG_REPORTED) == 0 &&
            ((prev.flags1F8 & TOUCH_FLAG_FAST_CLICK_CAND) == 0 || Touch_IsException(track))) {
            // (6a) 手套弱信号：+0x208 & 2 ∧ +0x1BA < peakFilterRef/2 (= 1500) ∧ ¬SmartCover
            if ((track.suppressFlags & SUPPRESS_GLOVE_WEAK) != 0 &&
                track.minSigLatched < static_cast<int16_t>(bigFrame.tholds.peakFilterRef / 2) &&
                !kSmartCoverOn) {
                allow = false;
            }
            // (6b) 笔在屏全点抑制：+0x208 & 4。调用前置条件与手套模式互斥 ⇒ 惰性。
            if ((track.suppressFlags & SUPPRESS_BY_STYLUS) != 0) {
                allow = false;
            }
        }

        // (7) +0x208 & 8 ∧ dwFeatureFlags & 0x1000（特性位为 0，惰性）
        if constexpr (kFeature0x1000) {
            if ((track.suppressFlags & SUPPRESS_FEATURE_1000) != 0) allow = false;
        }
        // (8) +0x208 & 0x20 ∧ dwFeatureFlags & 0x2000（同上）
        if constexpr (kFeature0x2000) {
            if ((track.suppressFlags & SUPPRESS_FEATURE_2000) != 0) allow = false;
        }

        // (9) +0x21C & 0x21：待确认 1 与通用抬起 0x20 被拒；2/4/8/0x10/0x40 放行。
        //     TouchEventCode 已是原厂位码，直接按位判，不另写 TE 状态判断。
        if ((static_cast<uint32_t>(track.state) & 0x21u) != 0) {
            allow = false;
        }

        // (10) +0x1F0 & 0x2000（区域 type 5 = 大物体）：命中即拒，
        //      并在 +0x1EC != 0x40 时自增 header[3]。
        if ((track.volatileFlags & TOUCH_VFLAG_ZONE_TYPE5) != 0) {
            if (track.exceptionType != ZONE_EXC_PALM) ++m_cancelledTouchCount;
            allow = false;
        }

        return allow;
    }

    /// TouchReport_PostureAntiTouch @0x6BA7AE86 —— 三源恒假（§5），保留短路结构。
    [[nodiscard]] static bool TouchReport_PostureAntiTouch(const TrackingSlot& track,
                                                          const SlotFilterState& prev) noexcept {
        if constexpr (!kMultiStepBackOn) {
            (void)track; (void)prev;
            return false;   // SensorData_IsAntiTouchStage / IsAnyTouchCancelByPosture / prev+0x200 bit0 全假
        } else {
            return (prev.flags1F8 & TOUCH_FLAG_POSTURE_CANCEL) != 0;
        }
    }

    /// Touch_IsException @0x6BA7416A —— 当前或历史 +0x1EC 非零、或帧异常字非零。
    /// 仅被判据 6 的括号消费；+0x1F8 bit18 复刻侧无产出方 ⇒ 括号恒真。
    [[nodiscard]] static bool Touch_IsException(const TrackingSlot& track) noexcept {
        return track.exceptionType != ZONE_EXC_NONE || track.isPalm;
    }

    // ══════════════════════════════════════════════════════════════════
    // §3 回放 FIFO 原语（PrevTouch_HistoryBuffer*）
    // ══════════════════════════════════════════════════════════════════
    static void HistoryBufferClean(SlotFilterState& s) noexcept { s.histCount288 = 0; }

    static void HistoryBufferPush(SlotFilterState& s, const HistEntry& e) noexcept {
        if (s.histCount288 >= kHistCap) {          // 满 30 条先丢最旧
            for (uint8_t i = 1; i < kHistCap; ++i) s.hist[i - 1] = s.hist[i];
            s.histCount288 = kHistCap - 1;
        }
        s.hist[s.histCount288++] = e;
    }

    static bool HistoryBufferPop(SlotFilterState& s, HistEntry& out) noexcept {
        if (s.histCount288 == 0) return false;
        out = s.hist[0];
        for (uint8_t i = 1; i < s.histCount288; ++i) s.hist[i - 1] = s.hist[i];
        --s.histCount288;
        return true;
    }

    /// 0 = 最旧。
    [[nodiscard]] static HistEntry HistoryBufferGet(const SlotFilterState& s, int idx) noexcept {
        if (idx < 0 || idx >= static_cast<int>(s.histCount288)) return HistEntry{};
        return s.hist[static_cast<size_t>(idx)];
    }

    // ── Touch_GetPostReportLength @0x6BA76958 ─────────────────────────
    // L = clamp(max(|dx|, |dy|) / TS_MMToPixel(2), 2, 5)。
    [[nodiscard]] static uint8_t Touch_GetPostReportLength(int32_t ax, int32_t ay,
                                                          int32_t cx, int32_t cy) noexcept {
        const int32_t dx = std::abs(cx - ax);
        const int32_t dy = std::abs(cy - ay);
        int32_t len = Fixed::TruncDiv(std::max(dx, dy), kPostReportSegQ8);
        if (len > 5) len = 5;
        if (len < 2) len = 2;
        return static_cast<uint8_t>(len);
    }

    // ── Touch_CreatePostReport @0x6BA76A03（§3）────────────────────────
    // `+0x1F8 bit7`（回放触发位）的唯一产出方，三条调用路径：
    //   GripFilter_Report（握持延迟放行，WindowsPad 下 50 ms）、
    //   TouchMode_GloveDetect（手套锁存首帧）、AntiTouch_PostReportUnit（AFT，关闭）。
    // 另有 ER_* 入边修正直接置 `+0x1F8 |= 0x80`（复刻侧由 05 发布到 persistFlags1F8）。
    //
    // 一次性守卫：prev +0x1F8 & 0x20020000 != 0 时整函数不执行。
    //
    // ⚠ **原厂缺陷，照抄**：位置按「锚点 → 当前点」递增排列，时间戳却写成
    //    `now − k·dt`，按「新 → 旧」**递减**。正确写法应是 `now − (L−1−k)·dt`。
    //    唯一受影响的消费者是 Filter_IsCurveReturn 的无符号 tsum（§10.1）。
    void Touch_CreatePostReport(TrackingSlot& track, SlotFilterState& s,
                                uint32_t& cur1F8, int64_t nowTs) noexcept {
        if ((s.flags1F8 & (TOUCH_FLAG_REPORT_RESET | TOUCH_FLAG_POSTURE_BLOCK)) != 0) return;

        cur1F8 |= TOUCH_FLAG_REPORT_RESET;   // 0x20000，同时是 Filter_ReportedProcessUnit 的重初始化触发

        // 锚点：未报过用 prev+0x496/+0x498（按下点），报过用 prev+0x49A/+0x49C（起报点）。
        int32_t ax = s.downAnchorX496, ay = s.downAnchorY498;
        if ((cur1F8 & TOUCH_FLAG_REPORTED) != 0) {
            ax = s.startRptX49A;
            ay = s.startRptY49C;
        }
        const int32_t cx = track.curX_q8;
        const int32_t cy = track.curY_q8;

        const uint8_t L = Touch_GetPostReportLength(ax, ay, cx, cy);
        const int32_t stepX = Fixed::TruncDiv(cx - ax, L - 1);
        const int32_t stepY = Fixed::TruncDiv(cy - ay, L - 1);
        const int32_t dt    = static_cast<int32_t>(nowTs - s.lastTs148);

        HistoryBufferClean(s);
        for (uint8_t k = 0; k + 1 < L; ++k) {
            HistEntry e;
            e.x  = Fixed::WrapU16(ax + k * stepX);   // 位置递增
            e.y  = Fixed::WrapU16(ay + k * stepY);
            e.ts = nowTs - static_cast<int64_t>(static_cast<int32_t>(k * dt)); // 时间递减 ← 原厂缺陷
            HistoryBufferPush(s, e);
        }
        cur1F8 |= TOUCH_FLAG_HIST_POP;   // 0x80
    }

    // ── TouchReport_IsHistoryBufferApplied @0x6BA7B373（§3）───────────
    // 网闸的旁路通道：`允许上报 = ToBeReported() ∨ IsHistoryBufferApplied()`，
    // 回放**可越过全部十条否决**。bit7 无清除者，一旦开启该触点余生每帧
    // 「压一条、弹一条」，上报坐标永久滞后 L−1 帧。
    [[nodiscard]] bool TouchReport_IsHistoryBufferApplied(TrackingSlot& track, SlotFilterState& s,
                                                         uint32_t cur1F8, int64_t nowTs,
                                                         uint16_t& bakX, uint16_t& bakY) noexcept {
        // 只要 TE ∉ {8, 0x10, 0x20} 就每帧压入一条工作坐标 + 时间戳。
        if ((static_cast<uint32_t>(track.state) & 0x38u) == 0) {
            HistEntry e;
            e.x  = track.curX_q8;
            e.y  = track.curY_q8;
            e.ts = nowTs;
            HistoryBufferPush(s, e);
        }
        if ((cur1F8 & TOUCH_FLAG_HIST_POP) == 0) return false;

        HistEntry popped{};
        if (!HistoryBufferPop(s, popped)) return false;

        bakX = track.curX_q8;          // 备份真实质心，ProcessUnit 在滤波后还原
        bakY = track.curY_q8;
        track.curX_q8 = popped.x;      // 0x162/0x164 ← 弹出的旧坐标
        track.curY_q8 = popped.y;

        if ((cur1F8 & TOUCH_FLAG_REPORTED) == 0) {
            track.state = EventDown;   // +0x21C = 2 强制按下
        }
        if ((static_cast<uint32_t>(track.state) & 0x30u) != 0) {
            m_pendingIdBreak = true;   // +0x1F8 |= 0x200000，下一帧强制断开 ID
            track.state = EventMove;   // +0x21C = 4，改成移动多活一帧
        }
        return true;
    }

    // ══════════════════════════════════════════════════════════════════
    // §1/§4 单元调度 TouchReport_ProcessUnit @0x6BA7B86E
    // ══════════════════════════════════════════════════════════════════
    // 执行顺序**不可换**（§17.1）：
    //   ToBeReported → IsHistoryBufferApplied（压栈 + 可能改写 0x162/0x164 与 TE）
    //   → 置/清 +0x1F8 bit3 → Filter_ProcessUnit → 还原坐标 → 起报锚点 → +0x290。
    void TouchReport_ProcessUnit(TrackingSlot& track, TouchBigFrame& bigFrame) {
        SlotFilterState& s = m_slotStates[track.id];
        const SlotFilterState prev = s;          // 帧首 Touch_UnitCopy 的快照
        s.id = track.id;
        m_pendingIdBreak = false;

        // Touch_UnitCopy：历史 0x140..0x283 段整段继承，再并上本帧上游新置的位
        //（ER 的 0x80 / 0x400、TE 的 0x40000 等由模块 05 发布在 persistFlags1F8）。
        // ⚠ bit3 **不在这里预清**：Touch_CreatePostReport 的锚点选择与
        //   TouchReport_IsHistoryBufferApplied 的「+0x1F8 & 8 == 0 → 强制 TE = 2」
        //   读的都是 Touch_UnitCopy 继承下来的**上一帧**bit3。只有下面的
        //   if/else 才重新裁决它。
        uint32_t cur1F8 = prev.flags1F8 | track.persistFlags1F8;
        const int64_t nowTs = static_cast<int64_t>(bigFrame.timestampUs);

        // PrevTouch_UnitRelease 后的首帧：把按下锚点与滤波状态播种到当前点。
        if (!prev.valid) {
            s.downAnchorX496 = track.curX_q8;
            s.downAnchorY498 = track.curY_q8;
            s.raw162 = track.curX_q8;
            s.raw164 = track.curY_q8;
        }

        // GripFilter / GloveDetect / ER 三条路径请求建立回放（§3）。
        if (m_postReportRequest[track.id]) {
            m_postReportRequest[track.id] = false;
            Touch_CreatePostReport(track, s, cur1F8, nowTs);
        }

        // (1) 网闸 —— 读的是**未被回放改写前**的 TE。
        const bool allow = TouchReport_ToBeReported(track, prev, bigFrame);

        // (2) 回放旁路 —— 可能改写 0x162/0x164 与 TE。
        uint16_t bakX = 0, bakY = 0;
        const bool replayed =
            TouchReport_IsHistoryBufferApplied(track, s, cur1F8, nowTs, bakX, bakY);
        if (m_pendingIdBreak) cur1F8 |= TOUCH_FLAG_FORCE_ID_BREAK;

        // (3) 置/清 +0x1F8 bit3
        if (!allow && !replayed) {
            cur1F8 &= ~TOUCH_FLAG_REPORTED;
        } else {
            ++m_reportedTouchCount;                       // header[5]++
            cur1F8 |= TOUCH_FLAG_REPORTED | TOUCH_FLAG_CUR_REPORTED;
        }

        // header[0xB]：`+0x1EC & 0x40` **位测试**，与判据 4 的等值比较口径不同（§1）。
        if ((track.exceptionType & 0x40u) != 0) ++m_palmTouchCount;

        // (4) 滤波 —— 对**每个**触点都跑，没有前置条件（§6）。
        Filter_ProcessUnit(track, s, prev, cur1F8);

        // §5 的多步回退覆盖（prev+0x594/0x596 → +0x170/+0x172）本机不可达，省略。

        // (5) 还原真实质心，供 05/06 的跨帧量与帧尾回灌使用（§3）。
        if (replayed) {
            track.curX_q8 = bakX;
            track.curY_q8 = bakY;
        }

        // (6) 首次上报 → 起报锚点（§4.1）。原版写的是**上报坐标** +0x170/+0x172。
        if ((cur1F8 & TOUCH_FLAG_REPORTED) != 0 && (prev.flags1F8 & TOUCH_FLAG_REPORTED) == 0) {
            TouchReport_UpdateStartReportPoint(s);
        }

        // (7) +0x290 事件迁移分类（纯遥测，不回灌历史）（§4.3）。
        const bool curRpt  = (cur1F8 & TOUCH_FLAG_REPORTED) != 0;
        const bool prevRpt = (prev.flags1F8 & TOUCH_FLAG_REPORTED) != 0;
        if (!curRpt && !prevRpt)      s.eventTransition290 = 1;
        else if (curRpt && !prevRpt)  s.eventTransition290 = 2;
        else if (!curRpt)             s.eventTransition290 = 0x20;
        else if ((track.volatileFlags & TOUCH_VFLAG_GRIP_SUPPRESS) == 0 &&
                 (m_prevGripSuppressed[track.id]))
                                      s.eventTransition290 = 2;   // 握持放行视作重新按下
        else                          s.eventTransition290 = 4;
        m_prevGripSuppressed[track.id] = (track.volatileFlags & TOUCH_VFLAG_GRIP_SUPPRESS) != 0;

        // §12 HotZoneAdjust_Process：dwFeatureFlags bit22 = 0，全链不执行，省略。

        // ── PrevTouch_RemoveOff：当前整段回灌历史 ──────────────────────
        s.flags1F8  = cur1F8;
        s.raw162    = track.curX_q8;
        s.raw164    = track.curY_q8;
        s.action224 = track.actionWord;
        s.lastTs148 = nowTs;
        s.valid     = true;

        // ── 发布给下游（Stage 8 只靠 +0x1F8 & 8 决定要不要用坐标）──────
        track.persistFlags1F8 = cur1F8;
        track.isReported = curRpt;
        if (curRpt) track.lifeFlags |= LifeReportAdmit;
        else        track.lifeFlags &= ~LifeReportAdmit;
        // 复刻里 fltX_q8 承担 +0x170 的角色（Stage 8 的 SeedOutputSlots 从它播种）。
        track.fltX_q8 = s.out170;
        track.fltY_q8 = s.out172;
        track.remX = s.rem16A;
        track.remY = s.rem16B;
        track.dispX = static_cast<uint16_t>(
            (static_cast<uint32_t>(track.fltX_q8) * kDisplayWidth) / static_cast<uint32_t>(kDim1Full));
        track.dispY = static_cast<uint16_t>(
            (static_cast<uint32_t>(track.fltY_q8) * kDisplayHeight) / static_cast<uint32_t>(kDim2Full));
    }

    /// TouchReport_UpdateStartReportPoint @0x6BA7B657（§4.1）：
    /// 把**上报坐标** +0x170/+0x172 写进起报点 +0x49A/+0x49C、点击抑制基准
    /// +0x4A4/+0x4A6、假滑动基准 +0x4AC/+0x4AE。这三个锚点是 Touch_SetPersistFlag
    /// 判 20 / 60 / 50 外部像素位移的参照，直接决定点击冻结何时释放。
    static void TouchReport_UpdateStartReportPoint(SlotFilterState& s) noexcept {
        s.startRptX49A = s.out170;  s.startRptY49C = s.out172;
        s.clickBaseX4A4 = s.out170; s.clickBaseY4A6 = s.out172;
        s.flickBaseX4AC = s.out170; s.flickBaseY4AE = s.out172;
    }

    /// 上游（GripFilter 50 ms 放行 / GloveDetect / AFT）请求建立回放段（§3）。
    void RequestPostReport(uint8_t id) noexcept {
        if (id > 0 && id <= 20) m_postReportRequest[id] = true;
    }

    // ══════════════════════════════════════════════════════════════════
    // §6 Filter_ProcessUnit @0x6BA5FD96
    // ══════════════════════════════════════════════════════════════════
    // 滤波对**所有**触点都跑。被拒的点走直通分支：四个坐标全部被工作坐标覆盖，
    // 但**余数 0x16A/0x16B、死区半径 0x27C、冻结位 0x800 都不动**（保留继承值）。
    //
    // 结尾 `0x170 ← 0x16C` 是**无条件**的。因此模块 05 的 ER 移出钳位写在
    // +0x170 上的值会在同一帧被这里覆盖 —— 属原厂死写，照抄，不修正。
    // 滤波链根读的是 +0x162/+0x164，从不读 +0x170。
    void Filter_ProcessUnit(TrackingSlot& track, SlotFilterState& s,
                            const SlotFilterState& prev, uint32_t& cur1F8) noexcept {
        // 遥测：g_xPosPreFilter / g_yPosPreFilter
        m_xPosPreFilter = ToDisplayDim1(track.curX_q8);
        m_yPosPreFilter = ToDisplayDim2(track.curY_q8);

        if ((cur1F8 & TOUCH_FLAG_REPORTED) == 0) {
            s.iir166 = track.curX_q8;  s.iir168 = track.curY_q8;
            s.jit16C = track.curX_q8;  s.jit16E = track.curY_q8;
        } else {
            Filter_ReportedProcessUnit(track, s, prev, cur1F8);
        }

        s.out170 = s.jit16C;   // 无条件
        s.out172 = s.jit16E;
    }

    // ══════════════════════════════════════════════════════════════════
    // §7 首报重置 Filter_ReportedProcessUnit @0x6BA5FB41
    // ══════════════════════════════════════════════════════════════════
    // 命中条件（二选一）：prev +0x1F8 & 8 == 0（上一帧没报过），
    // 或 +0x1F8 & 0x20000 的**上升沿**（Touch_CreatePostReport 刚建立回放）。
    // 不清 0x800、不清 prev+0x4EE/0x4F0、不清 +0x4F4 与 +0x5AD ——
    // 这些由历史单元的生命周期（PrevTouch_UnitRelease）负责归零。
    void Filter_ReportedProcessUnit(TrackingSlot& track, SlotFilterState& s,
                                    const SlotFilterState& prev, uint32_t& cur1F8) noexcept {
        const bool firstReport = (prev.flags1F8 & TOUCH_FLAG_REPORTED) == 0;
        const bool replayRise  = (cur1F8 & TOUCH_FLAG_REPORT_RESET) != 0 &&
                                 (prev.flags1F8 & TOUCH_FLAG_REPORT_RESET) == 0;
        if (firstReport || replayRise) {
            s.deadband27C = kDeadbandRQ8;                    // 0x27C = 36
            s.iir166 = track.curX_q8;  s.jit16C = track.curX_q8;
            s.iir168 = track.curY_q8;  s.jit16E = track.curY_q8;
            s.rem16A = 0;  s.rem16B = 0;                     // 两个余数清 0
        } else {
            Filter_ModeTouchReportedProcessUnit(track, s, prev, cur1F8);
        }
    }

    // ══════════════════════════════════════════════════════════════════
    // §8 冻结四分支 Filter_ModeTouchReportedProcessUnit @0x6BA5F1C2
    // ══════════════════════════════════════════════════════════════════
    // 三条冻结分支的动作完全相同：把 0x166/0x168 钉回上一帧、置 +0x1F8 |= 0x800、
    // 把被吞掉的位移**覆盖**（不是累加）写进 prev+0x4EE/0x4F0。
    // 全部挂在 (dwFeatureFlags & 0x100) 且 (dwFeatureFlags & 0x1000) == 0 之下 ——
    // bit8 = FEAT_COORD_FREEZE，本机开；bit12 是它的否决位，本机 0。
    void Filter_ModeTouchReportedProcessUnit(TrackingSlot& track, SlotFilterState& s,
                                             const SlotFilterState& prev,
                                             uint32_t& cur1F8) noexcept {
        constexpr bool kFreezeGate = kCoordFreezeOn && !kFreezeVetoOn;

        // 分支 1：抬手能量塌陷。
        //   +0x1DD < 0x10（峰邻域 Zn 比 < 50%）∧ +0x1DE < 0x14（峰幅比 < 62.5%）
        //   ∧ peak[+0x18] < g_tsaPrmtDynamic / 2 ∧ prev +0x224 == 1
        //   ∧ 非 (prev +0x1F8 & 0x8000)。本机实际生效的抬手甩尾抑制。
        const bool branch1 = kFreezeGate &&
            track.znRatio < 0x10 && track.z1Ratio < 0x14 &&
            track.peakSig < static_cast<int16_t>(m_dynamicGain / 2) &&
            prev.action224 == 1 &&
            (prev.flags1F8 & TOUCH_FLAG_FAST_MOVE) == 0;

        // 分支 2：假滑动 / 手指滚动。(+0x1F8 & 0x9000) == 0 且 (+0x1F0 & 0x100000)。
        const bool branch2 = kFreezeGate &&
            (cur1F8 & (TOUCH_FLAG_MOVED_50PX | TOUCH_FLAG_FAST_MOVE)) == 0 &&
            (track.volatileFlags & TOUCH_VFLAG_FALSE_FLICK) != 0;

        // 分支 3：仍在点击范围内。(+0x224 & 1) 且 (+0x1F8 & 0x8000) == 0。
        //   按下后只要位移没超过 20 显示像素（= 120 Q8）就一律钉死。
        const bool branch3 = kFreezeGate &&
            (track.actionWord & 1u) != 0 &&
            (cur1F8 & TOUCH_FLAG_FAST_MOVE) == 0;

        if (branch1 || branch2 || branch3) {
            s.iir166 = prev.iir166;
            s.iir168 = prev.iir168;
            cur1F8 |= TOUCH_FLAG_CLICK_FREEZE;
            s.clickOff4EE = Fixed::Wrap16(static_cast<int32_t>(track.curX_q8) - s.iir166);
            s.clickOff4F0 = Fixed::Wrap16(static_cast<int32_t>(track.curY_q8) - s.iir168);
        } else if ((cur1F8 & TOUCH_FLAG_CLICK_FREEZE) != 0) {
            // 已冻结且开始滑动 → 渐进渗出；两维都收敛才清 0x800。
            if (Filter_ReportClickToFlickProcessUnit(track, s, prev, cur1F8)) {
                cur1F8 &= ~TOUCH_FLAG_CLICK_FREEZE;
            }
        } else if (Touch_NeedXYFilter(track, prev)) {
            Filter_IIRPDims(track, s, prev, cur1F8);
        } else {
            s.iir166 = track.curX_q8;   // 不过 IIR，只过 JIT
            s.iir168 = track.curY_q8;
        }

        Filter_JitFilterProcessUnit(track, s, prev);   // 所有分支汇合
    }

    // ── Touch_NeedXYFilter @0x6BA742DC（§8）───────────────────────────
    // = (prev +0x1F8 & 0x40) 或 (+0x21C & 0x14)。
    // **+0x1F8 bit6 在整个 DLL 里没有写入者** ⇒ 判据退化为 TE ∈ {4 移动, 0x10 补报}。
    // 后果：TE = 2（按下帧）、8（HOLD）、0x40（WindowsPad 抬起帧）**都不过 IIR**。
    [[nodiscard]] static bool Touch_NeedXYFilter(const TrackingSlot& track,
                                                 const SlotFilterState& prev) noexcept {
        return (prev.flags1F8 & TOUCH_FLAG_RAMP_ALPHA) != 0 ||
               (static_cast<uint32_t>(track.state) & 0x14u) != 0;
    }

    // ── Filter_ReportClickToFlickProcessUnit @0x6BA5E6C3（§8 释放路径）──
    // 先把 0x162/0x164 临时减去残余偏移（不足则钳到 0），用「去偏移后的输入」
    // 跑一遍 Filter_IIRPDims，恢复输入，再逐维渗出。
    bool Filter_ReportClickToFlickProcessUnit(TrackingSlot& track, SlotFilterState& s,
                                              const SlotFilterState& prev,
                                              uint32_t& cur1F8) noexcept {
        const uint16_t bakX = track.curX_q8;
        const uint16_t bakY = track.curY_q8;

        track.curX_q8 = (static_cast<int32_t>(bakX) < s.clickOff4EE)
                            ? 0 : Fixed::WrapU16(static_cast<int32_t>(bakX) - s.clickOff4EE);
        track.curY_q8 = (static_cast<int32_t>(bakY) < s.clickOff4F0)
                            ? 0 : Fixed::WrapU16(static_cast<int32_t>(bakY) - s.clickOff4F0);

        Filter_IIRPDims(track, s, prev, cur1F8);

        track.curX_q8 = bakX;
        track.curY_q8 = bakY;

        const bool okX = ClickToFlickDimOffsetCheck(s.iir166, prev.iir166, s.clickOff4EE);
        const bool okY = ClickToFlickDimOffsetCheck(s.iir168, prev.iir168, s.clickOff4F0);
        return okX && okY;
    }

    // ── Filter_ReportClickToFlickDim1/2OffsetCheck @0x6BA5E149 ────────
    // step = min(ExtToIntDist(flash 0x90A = 3) = 18 Q8, |0x166 − prev 0x166| >> 2)
    // 即「本帧 IIR 位移的 1/4、每帧至多 3 个显示像素」。
    // −step < offset < step 时该维返回真。
    static bool ClickToFlickDimOffsetCheck(uint16_t& iir, uint16_t prevIir,
                                           int16_t& offset) noexcept {
        const int32_t diff = std::abs(static_cast<int32_t>(iir) - static_cast<int32_t>(prevIir));
        const int32_t step = std::min<int32_t>(kClickToFlickStepQ8, diff >> 2);

        if (static_cast<int32_t>(offset) < step) {
            if (static_cast<int32_t>(offset) > -step) return true;      // 收敛
            // offset <= −step：把 0x166 往下推 step，偏移往上收 step
            iir = (static_cast<int32_t>(iir) < step)
                      ? 0 : Fixed::WrapU16(static_cast<int32_t>(iir) - step);
            offset = Fixed::Wrap16(static_cast<int32_t>(offset) + step);
        } else {
            // offset >= step：把 0x166 往上推 step，偏移往下收 step
            iir = Fixed::WrapU16(static_cast<int32_t>(iir) + step);
            offset = Fixed::Wrap16(static_cast<int32_t>(offset) - step);
        }
        return false;
    }

    // ══════════════════════════════════════════════════════════════════
    // §9.4 Filter_IIRPDims @0x6BA5DCD4 —— α 的四道覆盖 + 逐维落盘
    // ══════════════════════════════════════════════════════════════════
    void Filter_IIRPDims(TrackingSlot& track, SlotFilterState& s,
                         const SlotFilterState& prev, uint32_t& cur1F8) noexcept {
        uint8_t alpha = Filter_GetCoefBasedOnDistance(track, s, prev, cur1F8);

        if (s.predStep5AD == 0) Filter_IsCurveReturn(track, s, cur1F8);   // 还没在窗口 → 检测折返
        if (s.predStep5AD != 0) Filter_IsPredictFinish(s);                // 已在窗口 → 推进窗口

        // 折返窗口内或折返刚结束（prev+0x280 非 0）时若检出抬手且非游戏场景：
        // α 强制成 1、死区半径复位，+0x280 在 0..5 之间递增、到 5 归零（约 5 帧强阻尼尾）。
        if ((s.handLiftTail280 != 0 || s.predStep5AD != 0) &&
            Filter_IsHandLift(track) && !TSA_IsGameScenario()) {
            Filter_IsHandLiftFinish(s);
            alpha = 1;
            s.deadband27C = kDeadbandRQ8;
        }

        // Filter_IsTouchLiftOff @0x6BA5C363：总开关 flash 0x8A2 = 0 ⇒ 恒返回 0（§9.4）。
        if constexpr (kLiftOffZDecayOn) {
            alpha = 1;
            s.deadband27C = kDeadbandRQ8;
        }

        // 边界格直通：坐标恰为 0 或维度上限 − 1，且与**上一帧原始坐标**差 > 0x31
        // （49 Q8 ≈ 8 显示像素）⇒ 该维跳过 IIR，**且不更新余数字节**。
        const int32_t stepX = std::abs(static_cast<int32_t>(track.curX_q8) -
                                       static_cast<int32_t>(prev.raw162));
        const int32_t stepY = std::abs(static_cast<int32_t>(track.curY_q8) -
                                       static_cast<int32_t>(prev.raw164));
        const bool edgeX = (track.curX_q8 == kDim1Full - 1 || track.curX_q8 == 0);
        const bool edgeY = (track.curY_q8 == kDim2Full - 1 || track.curY_q8 == 0);

        if (edgeX && stepX > kEdgeBypassStepQ8) s.iir166 = track.curX_q8;
        else Filter_ProcessPDim(track.curX_q8, prev.iir166, prev.rem16A, alpha, s.iir166, s.rem16A);

        if (edgeY && stepY > kEdgeBypassStepQ8) s.iir168 = track.curY_q8;
        else Filter_ProcessPDim(track.curY_q8, prev.iir168, prev.rem16B, alpha, s.iir168, s.rem16B);

        m_xPosAftIirFilter = ToDisplayDim1(s.iir166);   // 遥测
        m_yPosAftIirFilter = ToDisplayDim2(s.iir168);
    }

    // ── §9.5 定点核 IIR_Process @0x6BA601B0 ──────────────────────────
    //   int IIR_Process(int x, int y, byte a, byte s) // 调用处 s 恒为 5
    //   { return (int)(a * x + (((1 << s) & 0xFF) - a) * y) / (1 << s); }
    // 中间量 32 位有符号即可：α ≤ 32、x ≤ 15359 << 8 ⇒ α·x ≤ 1.26e8。
    // **不要用 64 位**。除法是 C 的截断除。
    [[nodiscard]] static constexpr int32_t IIR_Process(int32_t x, int32_t y, uint8_t a) noexcept {
        return Fixed::TruncDiv(static_cast<int32_t>(a) * x +
                                   (32 - static_cast<int32_t>(a)) * y,
                               32);
    }

    // ── §9.5 Filter_ProcessPDim1/2 @0x6BA5CC76 / @0x6BA5CD9C ─────────
    // 状态量是 **16.8 定点**：整数部在 0x166（u16 Q8 坐标），小数部在 0x16A（u8）。
    //   r = IIR_Process(0x162 << 8, (prev 0x166 << 8) + prev 0x16A, α);
    //   0x166 = (short)((uint)(r < 0 ? r + 0xFF : r) >> 8);   // 向零取整
    //   0x16A = (byte)r;                                       // **取低 8 位**，不是 r − (int << 8)
    // α = 0 时输出恒等于上一帧的 16.8 状态（含余数）—— 真正的零位移锁死；
    // α = 32 时 r = x << 8，余数被清成 0。
    static void Filter_ProcessPDim(uint16_t raw, uint16_t prevIir, uint8_t prevRem,
                                   uint8_t alpha, uint16_t& outIir, uint8_t& outRem) noexcept {
        const int32_t x = static_cast<int32_t>(raw) << 8;
        const int32_t y = (static_cast<int32_t>(prevIir) << 8) + static_cast<int32_t>(prevRem);
        const int32_t r = IIR_Process(x, y, alpha);
        outIir = Fixed::WrapU16(Fixed::ShrUnsigned((r < 0) ? (r + 0xFF) : r, 8));
        outRem = static_cast<uint8_t>(static_cast<uint32_t>(r) & 0xFFu);
    }

    // ── §9.1 Filter_GetCoefBasedOnDistance @0x6BA5C99C ───────────────
    // 先按模式选表（正常 / 重阻尼 / 特性 0x1000），再调 Filter_GetAdaptiveCoef。
    // 末尾的「指数恢复 0 → 2 → 4 → 8 → 16 → 32」挂在 prev +0x1F8 & 0x40 上，
    // 该位**全库无写入者** ⇒ 死枝，+0x4F3 恒为 0（§9.1）。
    uint8_t Filter_GetCoefBasedOnDistance(const TrackingSlot& track, SlotFilterState& s,
                                          const SlotFilterState& prev,
                                          uint32_t& cur1F8) noexcept {
        // 特性 0x1000 表：dwFeatureFlags bit12 = 0，本机关闭，整段省略（§16）。
        const bool heavy = Filter_NeedHeavyCoef(track, cur1F8);
        // 原版随后还有一段「prev +0x1F8 & 0x40 → prev+0x4F3 按 0→2→4→8→16→32
        // 倍增并直接作为 α 返回」。bit6 全库无写入者 ⇒ 整段死枝、+0x4F3 恒 0，省略。
        return Filter_GetAdaptiveCoef(track, s, prev, cur1F8, heavy);
    }

    // ── Filter_NeedHeavyCoef @0x6BA5C240（§9.1）──────────────────────
    // 区域类型 +0x1E0 == 3（多指合并区）或帧异常字 header[0x20] != 0 → +0x1F8 |= 0x10。
    // 该位一旦置位就随 Touch_UnitCopy 粘住 ⇒ **该触点余生都走重阻尼表（α 上限 12）**。
    // header[0x20] 是**帧级**异常字（湿手指 0x80 / 基线复位 0x200 / 硬件异常 0x400 /
    // 重校准 0x1000），不是逐触点的 +0x1EC —— 复刻侧由 Exception_Process 的等价实现
    // 写 m_frameExceptionWord，尚无产出方时恒 0，等价于「本帧无异常」。
    bool Filter_NeedHeavyCoef(const TrackingSlot& track, uint32_t& cur1F8) const noexcept {
        const bool byZone = track.fromMultiFinger;        // +0x1E0 == 3
        const bool byExc  = m_frameExceptionWord != 0;    // header[0x20] != 0
        if (byZone) cur1F8 |= TOUCH_FLAG_HEAVY_FILTER;
        if (byExc)  cur1F8 |= TOUCH_FLAG_HEAVY_FILTER;
        return byZone || byExc || (cur1F8 & TOUCH_FLAG_HEAVY_FILTER) != 0;
    }

    // ── §9.2 AIIR_GetCoef @0x6BA60040 ────────────────────────────────
    //   d >= D_hi            → α_hi
    //   d <= D_lo            → α_lo
    //   否则                  → α_lo + ((d − D_lo) · R) >> 10
    //   末尾：α == 0 且 d != 0 → 1
    // 正常表 (0, 384, 0, 32, R = 85)：α = min(32, (d·85) >> 10)，
    // d = 0 给 0（完全静止锁死），0 < d ≤ 12 被钳到 1。
    [[nodiscard]] uint8_t AIIR_GetCoef(uint16_t dist, bool heavy) const noexcept {
        const uint16_t dLo   = heavy ? m_heavyFilterLightDelta  : m_lightDelta;
        const uint16_t dHi   = heavy ? m_heavyFilterHeavyDelta  : m_heavyDelta;
        const uint8_t  aLo   = heavy ? m_heavyFilterLightCoef   : m_lightCoef;
        const uint8_t  aHi   = heavy ? m_heavyFilterHeavyCoef   : m_heavyCoef;
        const uint16_t ratio = heavy ? m_heavyFilterScaledRatio : m_scaledRatio;

        uint8_t a;
        if (dist < dHi) {
            if (dist > dLo) {
                const int32_t v = (static_cast<int32_t>(dist) - static_cast<int32_t>(dLo)) *
                                  static_cast<int32_t>(ratio);
                a = static_cast<uint8_t>(Fixed::ShrTowardZero(v, 10) + aLo);
            } else {
                a = aLo;
            }
        } else {
            a = aHi;
        }
        if (a == 0 && dist != 0) a = 1;
        return a;
    }

    // ── §9.3 Filter_GetAdaptiveCoef @0x6BA5C690 ──────────────────────
    // 次序：AIIR_GetCoef(|raw − prevIIR|) → +0x279 快速移动锁存后**下限 8**
    //       → ±1 死带一步逼近（状态存历史 +0x4F4，0 视为未初始化）。
    // 注意距离取的是「**当前原始工作坐标**对**上一帧 IIR 输出**」之差，
    // 不是两帧原始坐标之差。
    uint8_t Filter_GetAdaptiveCoef(const TrackingSlot& track, SlotFilterState& s,
                                   const SlotFilterState& prev, uint32_t cur1F8,
                                   bool heavy) noexcept {
        const uint32_t dx = static_cast<uint32_t>(
            std::abs(static_cast<int32_t>(track.curX_q8) - static_cast<int32_t>(prev.iir166)));
        const uint32_t dy = static_cast<uint32_t>(
            std::abs(static_cast<int32_t>(track.curY_q8) - static_cast<int32_t>(prev.iir168)));
        const uint32_t dist = Fixed::Misc_SqrtUint32(dx * dx + dy * dy);

        uint8_t a = AIIR_GetCoef(static_cast<uint16_t>(dist & 0xFFFFu), heavy);

        if (prev.fastMove279 == 1) {
            s.fastMove279 = 1;
            if (a < 8) a = 8;                 // 曾快速移动过 ⇒ α 下限 8（≥ 25% 直通）
        } else if ((cur1F8 & TOUCH_FLAG_FAST_MOVE) != 0 && !Filter_IsHandLift(track)) {
            s.fastMove279 = 1;
        }

        // ±1 死带的一步逼近器（不是斜率限制器）：差 ≤ 1 时沿用上帧，
        // 差更大时走「目标 ∓ 1」而不是「上帧 ± 1」。
        if (s.alphaLatch4F4 == 0) s.alphaLatch4F4 = a;       // 0 视为未初始化
        if (static_cast<uint32_t>(s.alphaLatch4F4) + 1u < static_cast<uint32_t>(a)) {
            a = static_cast<uint8_t>(a - 1);
            s.alphaLatch4F4 = a;
        } else if (static_cast<uint32_t>(a) + 1u < static_cast<uint32_t>(s.alphaLatch4F4)) {
            a = static_cast<uint8_t>(a + 1);
            s.alphaLatch4F4 = a;
        } else {
            a = s.alphaLatch4F4;
        }
        return a;
    }

    /// Filter_IsHandLift @0x6BA5C644 = `+0x1DC < 0x1C`
    /// （本帧信号总量掉到上帧的 87.5% 以下）。
    [[nodiscard]] static bool Filter_IsHandLift(const TrackingSlot& track) noexcept {
        return track.sigSumRatio < 0x1C;
    }

    /// Filter_IsHandLiftFinish @0x6BA5DC30：+0x280 在 0..5 之间递增，到 5 归零。
    static void Filter_IsHandLiftFinish(SlotFilterState& s) noexcept {
        s.handLiftTail280 = (s.handLiftTail280 < 5)
                                ? static_cast<uint8_t>(s.handLiftTail280 + 1) : uint8_t{0};
    }

    /// TSA_IsGameScenario = TSA_IsHighRateGameScenario ∨ TSA_IsNormalRateGameScenario。
    [[nodiscard]] bool TSA_IsGameScenario() const noexcept { return m_gameScenario; }

    // ══════════════════════════════════════════════════════════════════
    // §10.1 Filter_IsCurveReturn @0x6BA5D178
    // ══════════════════════════════════════════════════════════════════
    // 前置：Filter_IsNeedCurve（两维都离边 0x180 = 1.5 格以上）、历史条数 != 0、
    //       cur +0x279 == 1（本次按下曾快速移动）。
    //
    // ⚠ tsum 声明为 **uint32**，累加的却是有符号差 ts[k] − ts[k−1]。
    //   Touch_CreatePostReport 补插的条目时间戳递减（§3），扫到补插段时 tsum
    //   立刻回绕成接近 2^32，`while (tsum < 0x50)` 当场退出、iT 停在首步，
    //   `iD < iT` 判假 ⇒ 折返检测在回放后最多 4 帧内被静默关闭。
    //   **保留无符号语义，不得改成有符号。**
    void Filter_IsCurveReturn(const TrackingSlot& track, SlotFilterState& s,
                              uint32_t /*cur1F8*/) noexcept {
        if (!Filter_IsNeedCurve(track)) return;
        const uint8_t n = s.histCount288;
        if (n == 0) return;
        if (s.fastMove279 != 1) return;

        uint32_t dsum = 0;      // 累计路程
        uint32_t tsum = 0;      // 累计时间（**uint32**，见上）
        int      iD = -1;       // 首次 dsum > 0x98 (152 Q8) 的下标
        int      iT = -1;       // 首次 tsum > 7 ms 的下标
        uint8_t  k = n;
        do {
            --k;
            if (k == 0) break;
            const HistEntry h1 = HistoryBufferGet(s, k);
            const HistEntry h0 = HistoryBufferGet(s, k - 1);
            const int32_t dx = static_cast<int32_t>(static_cast<int16_t>(h1.x - h0.x));
            const int32_t dy = static_cast<int32_t>(static_cast<int16_t>(h1.y - h0.y));
            tsum += static_cast<uint32_t>(static_cast<int32_t>(h1.ts) -
                                          static_cast<int32_t>(h0.ts));
            dsum += Fixed::Misc_SqrtUint32(static_cast<uint32_t>(dx * dx + dy * dy));
            if (iD < 0 && dsum > 0x98u) iD = k - 1;
            if (iT < 0 && tsum > 7u)    iT = k - 1;
        } while (tsum < 0x50u);

        if (!(tsum > 0x4Fu && dsum > 0x80u && iD >= 0 && iT >= 0 && iD < iT)) return;

        const HistEntry hD = HistoryBufferGet(s, iD);
        const HistEntry hN = HistoryBufferGet(s, n - 1);
        const HistEntry hT = HistoryBufferGet(s, iT);
        const int32_t ax = static_cast<int16_t>(hN.x - hD.x);   // A 向量：iD → 最新
        const int32_t ay = static_cast<int16_t>(hN.y - hD.y);
        const int32_t bx = static_cast<int16_t>(hT.x - hD.x);   // B 向量：iD → iT
        const int32_t by = static_cast<int16_t>(hT.y - hD.y);
        const uint32_t aLen = Fixed::Misc_SqrtUint32(static_cast<uint32_t>(ax * ax + ay * ay));
        const uint32_t bLen = Fixed::Misc_SqrtUint32(static_cast<uint32_t>(bx * bx + by * by));
        if (!(aLen < bLen)) return;        // 近端净位移反而更小才是折返

        s.handLiftTail280 = 1;             // 抬手尾巴计数臂

        if (std::abs(ay) * 5 < std::abs(ax) && std::abs(by) * 5 < std::abs(bx)) {
            s.predStep5AD = 1;  s.turnAxis5AC = 1;     // Dim1 主轴
        } else if (std::abs(ax) * 5 < std::abs(ay) && std::abs(bx) * 5 < std::abs(by)) {
            s.predStep5AD = 1;  s.turnAxis5AC = 0;     // Dim2 主轴
        }
        // 主轴不明确时只置 0x280，不进预测窗口。

        if constexpr (!kPredictDeadbandScaling) {
            if (s.predStep5AD != 0) {
                const int32_t v = Filter_GetSpeed(s, n - 1, s.turnAxis5AC);
                s.turnNum5AE = static_cast<uint8_t>(Filter_GetTurnNum(v));
                s.fwdNum5AF  = static_cast<uint8_t>(Filter_GetForwardNum(v));
            }
        }
    }

    /// Filter_IsNeedCurve @0x6BA5CEC2：两维都离两侧各 0x180 Q8（1.5 格）以上。
    [[nodiscard]] static bool Filter_IsNeedCurve(const TrackingSlot& track) noexcept {
        return track.curX_q8 > kCurveMarginQ8 &&
               static_cast<int32_t>(track.curX_q8) < kDim1Full - kCurveMarginQ8 &&
               track.curY_q8 > kCurveMarginQ8 &&
               static_cast<int32_t>(track.curY_q8) < kDim2Full - kCurveMarginQ8;
    }

    // ── Filter_GetSpeed @0x6BA5CF7A（§10.1）──────────────────────────
    // 取最近 6 个历史间隔，主轴分量的 |Δ| 求平均，再按首尾净位移的符号定号；
    // 不足 6 条返回 0。**完全不读条目的时间戳** —— 量纲是「每样本位移」，
    // Filter_GetTurnNum / GetForwardNum 的 660/|v| 与 60/|v| 据此标定。
    [[nodiscard]] static int32_t Filter_GetSpeed(const SlotFilterState& s, int newest,
                                                 uint8_t axisIsDim1) noexcept {
        const int oldest = newest - 6;
        if (oldest < 0) return 0;
        int32_t acc = 0;
        for (int k = oldest; k < newest; ++k) {
            const HistEntry a = HistoryBufferGet(s, k);
            const HistEntry b = HistoryBufferGet(s, k + 1);
            const int32_t d = axisIsDim1 ? (static_cast<int32_t>(b.x) - static_cast<int32_t>(a.x))
                                         : (static_cast<int32_t>(b.y) - static_cast<int32_t>(a.y));
            acc += std::abs(d);
        }
        int32_t v = acc / (newest - oldest);
        const HistEntry a = HistoryBufferGet(s, oldest);
        const HistEntry b = HistoryBufferGet(s, newest);
        const int32_t net = axisIsDim1 ? (static_cast<int32_t>(b.x) - static_cast<int32_t>(a.x))
                                       : (static_cast<int32_t>(b.y) - static_cast<int32_t>(a.y));
        if (net < 0) v = -v;
        return v;
    }

    /// Filter_GetTurnNum @0x6BA5D0DC：|v| <= 4 → 0，否则 min(12, 660 / |v|)。
    [[nodiscard]] static int32_t Filter_GetTurnNum(int32_t v) noexcept {
        if (v >= -4 && v <= 4) return 0;
        return std::min<int32_t>(12, 0x294 / std::abs(v));
    }

    /// Filter_GetForwardNum @0x6BA5D12A：|v| <= 4 → 0，否则 min(2, 60 / |v|)。
    [[nodiscard]] static int32_t Filter_GetForwardNum(int32_t v) noexcept {
        if (v >= -4 && v <= 4) return 0;
        return std::min<int32_t>(2, 0x3C / std::abs(v));
    }

    // ══════════════════════════════════════════════════════════════════
    // §10.2 Filter_IsPredictFinish @0x6BA5D662（DAT_6bca92a3 = 0 分支）
    // ══════════════════════════════════════════════════════════════════
    // flash 0x8A3 实测 0 ⇒ 走「外推 + 混合」分支；另一条按 0x5AD 分三段
    // 缩放死区半径的分支**不执行**。这条分支每次都把 0x27C 写回 36，
    // 因此本机 +0x27C 恒为 36。
    // 关键次序：0x5AD 在这里**自增后**才被 JIT 的混合条件读取（§17.11）。
    static void Filter_IsPredictFinish(SlotFilterState& s) noexcept {
        if (s.predStep5AD < s.turnNum5AE) {
            s.deadband27C = kDeadbandRQ8;
            const uint8_t n = s.histCount288;
            if (n > 1) {                          // 用最后两条历史做一阶外推
                const HistEntry h1 = HistoryBufferGet(s, n - 1);
                const HistEntry h2 = HistoryBufferGet(s, n - 2);
                s.predX5B0 = Fixed::Wrap16(static_cast<int32_t>(s.jit16C) +
                                           (static_cast<int32_t>(h1.x) - static_cast<int32_t>(h2.x)));
                s.predY5B2 = Fixed::Wrap16(static_cast<int32_t>(s.jit16E) +
                                           (static_cast<int32_t>(h1.y) - static_cast<int32_t>(h2.y)));
            }
            ++s.predStep5AD;
        } else {
            s.predStep5AD = 0;
            s.deadband27C = kDeadbandRQ8;
            s.predX5B0 = 0;
            s.predY5B2 = 0;
        }
    }

    // ══════════════════════════════════════════════════════════════════
    // §11.1 JITFilter @0x6BA602C6 —— 绳索 / 牵引模型
    // ══════════════════════════════════════════════════════════════════
    // 入参 6 × int32：[0][1] 当前 IIR 点、[2][3] 上一帧 JIT 输出、[4] 半径 R、
    // [5] 单轴最小步长上限 M（进函数先 M = min(M, R)）。
    //   未越界（dx² + dy² <= R² 或 d == 0）→ 输出完全等于上一帧（纯保持）；
    //   越界 → 输出落在以输入点为圆心、半径 R 的圆上，方向指向上一次输出。
    // 稳态滑动时上报点恒滞后真实点 R = 36 Q8 = 6 显示像素，跨越边界时连续。
    // M 逻辑补偿「分量太小」的轴：|off| < M 时，|d| < M 则该轴锁死，否则强制牵引 ±M。
    static Point2D_Q8 JITFilter(int32_t inX, int32_t inY, int32_t prevX, int32_t prevY,
                                int32_t R, int32_t M) noexcept {
        const int32_t dx = inX - prevX;
        const int32_t dy = inY - prevY;
        const uint32_t d2 = static_cast<uint32_t>(dx * dx + dy * dy);
        M = std::min(M, R);
        const int32_t d = static_cast<int32_t>(Fixed::Misc_SqrtUint32(d2));

        if (!(static_cast<uint32_t>(R * R) < d2 && d != 0)) {
            return Point2D_Q8{prevX, prevY};          // 完全冻结
        }

        int32_t offX = Fixed::TruncDiv(R * dx, d);
        if (std::abs(offX) < M) {
            offX = (std::abs(dx) < M) ? dx : Fixed::TruncDiv(M * dx, std::abs(dx));
        }
        int32_t offY = Fixed::TruncDiv(R * dy, d);
        if (std::abs(offY) < M) {
            offY = (std::abs(dy) < M) ? dy : Fixed::TruncDiv(M * dy, std::abs(dy));
        }
        return Point2D_Q8{inX - offX, inY - offY};
    }

    // ══════════════════════════════════════════════════════════════════
    // §11.2 / §11.3 Filter_JitFilterProcessUnit @0x6BA5E994
    // ══════════════════════════════════════════════════════════════════
    void Filter_JitFilterProcessUnit(const TrackingSlot& /*track*/, SlotFilterState& s,
                                     const SlotFilterState& prev) noexcept {
        const int32_t inX = s.iir166;
        const int32_t inY = s.iir168;
        const int32_t pX  = prev.jit16C;
        const int32_t pY  = prev.jit16E;

        // R = (prev 0x5AD == 0) ? DAT_6bcab612 : cur 0x27C。本机两者都是 36。
        // ⚠ 0x5AC..0x5B2 一组读的是**历史单元的实时值** —— Filter_IsCurveReturn 与
        //   Filter_IsPredictFinish 本帧刚写过它（0x5AD 已自增），JIT 看到的是自增后
        //   的值，不是帧首快照（§17.11）。只有 0x16C/0x16E 才取帧首的上一帧输出。
        const int32_t R = (s.predStep5AD == 0) ? static_cast<int32_t>(kDeadbandRQ8)
                                               : static_cast<int32_t>(s.deadband27C);
        const Point2D_Q8 out = JITFilter(inX, inY, pX, pY, R, kJitMaxStepQ8);

        // Filter_IsTouchLiftOffJitterSuppressEnable 被 flash 0x8A2 = 0 短路，
        // 那次「二次 JIT」调用不执行（§9.4）。

        // 逐维落盘：边界格（坐标恰为 0 或维度上限 − 1）直接绕过死区。
        if (inX == kDim1Full - 1 || inX == 0) {
            s.jit16C = Fixed::WrapU16(inX);
        } else {
            s.jit16C = Fixed::WrapU16(out.x);
            if (TSA_IsGameScenario()) {
                // §11.3 游戏微操阻尼：两条 if **顺序执行**，净效果是每帧朝上一帧
                // 输出方向再收 1 个 Q8（1/256 格 ≈ 0.17 显示像素），不是 1 显示像素。
                if (prev.jit16C < s.jit16C) s.jit16C = Fixed::WrapU16(s.jit16C - 1);
                if (s.jit16C < prev.jit16C) s.jit16C = Fixed::WrapU16(s.jit16C + 1);
            }
        }
        if (inY == kDim2Full - 1 || inY == 0) {
            s.jit16E = Fixed::WrapU16(inY);
        } else {
            s.jit16E = Fixed::WrapU16(out.y);
            if (TSA_IsGameScenario()) {
                if (prev.jit16E < s.jit16E) s.jit16E = Fixed::WrapU16(s.jit16E - 1);
                if (s.jit16E < prev.jit16E) s.jit16E = Fixed::WrapU16(s.jit16E + 1);
            }
        }

        // §11.3 折返混合：DAT_6bca92a3 == 0 且 prev 0x5AF < prev 0x5AD − 1
        // （越过前瞻帧数之后）。注意 0x5AD 已在 Filter_IsPredictFinish 里自增过。
        //   w   = ((0x5AD − 1) · 0x100) / 0x5AE
        //   out = ((0x100 − w) · pred + jit · w + 负值补 0xFF) >> 8
        if constexpr (!kPredictDeadbandScaling) {
            if (static_cast<int32_t>(s.fwdNum5AF) <
                static_cast<int32_t>(s.predStep5AD) - 1) {
                const int32_t w = Fixed::TruncDiv(
                    (static_cast<int32_t>(s.predStep5AD) - 1) * 0x100,
                    static_cast<int32_t>(s.turnNum5AE));
                int32_t t = (0x100 - w) * static_cast<int32_t>(s.predX5B0) +
                            static_cast<int32_t>(s.jit16C) * w;
                if (t < 0) t += 0xFF;
                s.jit16C = Fixed::WrapU16(Fixed::ShrUnsigned(t, 8));

                t = (0x100 - w) * static_cast<int32_t>(s.predY5B2) +
                    static_cast<int32_t>(s.jit16E) * w;
                if (t < 0) t += 0xFF;
                s.jit16E = Fixed::WrapU16(Fixed::ShrUnsigned(t, 8));
            }
        }
    }

    // ══════════════════════════════════════════════════════════════════
    // §13 Touch_ProcessExtInfo @0x6BA7713D
    // ══════════════════════════════════════════════════════════════════
    // **不写任何逐触点扩展量**（没有压力 / 面积 / 椭圆 / 置信度），
    // 只往**帧头 +0x18** 叠加全局标志，再调 Posture_UpdatePostureType 与
    // AntiTouch_Report。这两个字是给主机侧帧信息用的，**不进逐触点上报包**。
    void Touch_ProcessExtInfo(TouchBigFrame& bigFrame) noexcept {
        // 0x20 TSA_IsUnderWaterDetected / 0x40 TSA_IsOutOfWaterDetected：
        //   水下特性位（dwFeatureFlags bit11）为 0，两者恒假。
        // 0x80 Proximity_IsDetected：接近特性位为 0，恒假。
        // 0x100 / 0x200 SensorGesture_IsProximityPickUp/PutDownGestureTimeOut：
        //   复刻侧无 SensorGesture 通道，恒「已超时」⇒ 不置位。
        // AntiTouch_Report 按 header[3]（被拒报触点数）叠 0x1000000 / 0x2000000。
        if (m_cancelledTouchCount != 0) m_extInfoFlags |= 0x1000000u;
        bigFrame.specialControlFlags |= 0u;   // Posture_UpdatePostureType：复刻侧无姿态分类
    }

    // ── 遥测换算（TSAOut_Dim1PosInternal / Dim2PosInternal）──────────
    [[nodiscard]] static uint32_t ToDisplayDim1(uint16_t q8) noexcept {
        return (static_cast<uint32_t>(q8) * kDisplayWidth) / static_cast<uint32_t>(kDim1Full);
    }
    [[nodiscard]] static uint32_t ToDisplayDim2(uint16_t q8) noexcept {
        return (static_cast<uint32_t>(q8) * kDisplayHeight) / static_cast<uint32_t>(kDim2Full);
    }

    static void SynchronizeContacts(TouchBigFrame& bigFrame) noexcept {
        for (auto& contact : bigFrame.contacts) {
            for (const auto& track : bigFrame.tracks) {
                if (track.id != contact.id) continue;
                contact.isReported = track.isReported;
                contact.lifeFlags  = track.lifeFlags;
                contact.x = static_cast<float>(track.fltX_q8) / 256.0f;
                contact.y = static_cast<float>(track.fltY_q8) / 256.0f;
                break;
            }
        }
    }

    // Stage 7 的参数全部来自 Flash 整定表，不对上位机开放。
    void registerBindings(Config::ConfigBinder& binder) { (void)binder; }
    void applyConfig(const Config::ConfigStore& store) { (void)store; }

private:
    /// g_tsaPrmtDynamic 初值（flash 0x5D8 = 1000），冻结分支 1 的峰幅门限 = 500。
    int16_t m_dynamicGain = 1000;
    /// header[0x20] 帧级异常字，Filter_NeedHeavyCoef 的第二个触发源。
    uint32_t m_frameExceptionWord = 0;
    /// TouchReport_IsHistoryBufferApplied 回传的 +0x1F8 |= 0x200000 请求。
    bool m_pendingIdBreak = false;
    /// GripFilter / GloveDetect / AFT 对 Touch_CreatePostReport 的建段请求。
    std::array<bool, 21> m_postReportRequest{};
    /// 上一帧的 +0x1F0 & 0x8000000（握持剔除），+0x290 事件迁移分类要用。
    std::array<bool, 21> m_prevGripSuppressed{};

    std::array<SlotFilterState, 21> m_slotStates{};
};

} // namespace Solvers::Touch
