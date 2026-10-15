#pragma once
// ── TouchPipeline Stage 6.5: TouchModeFsm（四态触控模式状态机）────────
// Header-only C++23。对位原版 TSA_MSProcess 中 GripFilter_Process 之后、
// AntiTouch_Process / TouchReport_Process 之前的那一次调用：
//   TouchMode_Process            @0x6BA78DDE（调用点 0x6BA915C3）
//     ├─ TouchMode_SwitchPriorityProcess @0x6BA7900B
//     ├─ TouchMode_StylusFilter          @0x6BA7A0ED
//     ├─ g_tmSwitchPriority              @0x6BB4F240（4 × 4 uint32）
//     ├─ g_tmHandlerTable                @0x6BB52A40（4 对函数指针）
//     ├─ g_tmSwitchDebounce              @0x6BB4F200（4 × 4 uint16）
//     ├─ g_tmSwitchTimer / SWTimer_*     @0x6BBC8660 / 0x6BB027D0
//     └─ TouchMode_NewModeProcess        @0x6BA78C16
//
// 模式单元是三个 uint 的结构体（原版落在 g_tsaPrpt + 0x2E8 / 0x2EC / 0x2F0，
// 备份副本在 + 0x2F4 / 0x2F8 / 0x2FC）：
//   +0 mode（0..3）  +4 data（模式私有载荷）  +8 result（检测器判决）
//
// 它是 touch + 0x208 bit1 / bit2 的唯一产出方，也是 DAT_6BC4A924（手套锁存
// 位）的产出方；后者被 TouchThold_Process / Touch_SetVolatileFlagStableTouch /
// TouchThold_GetTholdBasedOnTouchMode 共用的那条「特殊灵敏度」判据消费。
//
// ── W273AS2700 可达性（文档 06 §1 / §5 / §5.1，反编译复核 2026-09）────────
// ⚠ 早先版本把 Finger / Glove 两态都判成「不可达」，这是**错的**：
//   · `DAT_6bcab700` = flash+0x918 = **0x0E**，不是 0。bit1（Finger 检测）与
//     bit2（Glove 许可）都置位；`TSADynPrmt_Init` 装载、`Exception_Process`
//     每帧维护，本机常态非零。
//   · `dwFeatureFlags bit0`（手套）恒 1（TouchGates 的 FEAT_GLOVE = Active）。
//   ⇒ NoObj / Finger / Glove 三态全部可达；真正的手套闩锁是 `DAT_6BC4A924`
//     （本类的 m_gloveLatch），不是 `TSAStatic + 0x3F1`。
//   · Stylus 仍不可达：`TouchMode_StylusDetect @0x6BA798B8` 无判据，恒返回 1。
// ─────────────────────────────────────────────────────────────────────

#include <cstdint>
#include <cstddef>
#include <algorithm>

#include "TouchCoreTypes.hpp"
#include "TouchGates.hpp"
// TOUCH_FLAG_FINGER_Z_FAILED（touch + 0x1f0 bit11）在 Stage 5 定义，本级消费它。
#include "TrackingAndEvents.hpp"

namespace Solvers::Touch {

// ── touch + 0x208 抑制标志位（与 TouchReportAndFilter 的 SUPPRESS_* 同源）──
inline constexpr uint32_t TOUCH_SUPPRESS_GLOVE_BASE = 0x01u; // GloveFilter 的前置排除位
inline constexpr uint32_t TOUCH_SUPPRESS_GLOVE_WEAK = 0x02u; // 上报网闸判据 6a
inline constexpr uint32_t TOUCH_SUPPRESS_BY_STYLUS  = 0x04u; // 上报网闸判据 6b

// ── touch + 0x1F8 持久位（Touch_SetPersistFlag @0x6BA72776 产出，模块 06）──
// GloveDetect 只读这两位：bit2 = 距按下点位移 ≥ 15 mm，bit19 = 强峰/异常/多指排除。
inline constexpr uint32_t TOUCH_PFLAG_MOVED_15MM  = 0x00000004u;
inline constexpr uint32_t TOUCH_PFLAG_GLOVE_EXCL  = 0x00080000u;

// ── 四态枚举（顺序由 g_tmHandlerTable 的排布确定）────────────────────
//   0 → TouchMode_NoObjDetect  @0x6BA790AE / TouchMode_NoObjProcess  @0x6BA79113
//   1 → TouchMode_FingerDetect @0x6BA7911F / TouchMode_FingerProcess @0x6BA79318
//   2 → TouchMode_GloveDetect  @0x6BA793FE / TouchMode_GloveProcess  @0x6BA797A7
//   3 → TouchMode_StylusDetect @0x6BA798B8 / TouchMode_StylusProcess @0x6BA7991A
enum TouchModeId : uint32_t {
    TouchModeNoObject = 0,
    TouchModeFinger   = 1,
    TouchModeGlove    = 2,
    TouchModeStylus   = 3,
};
inline constexpr uint32_t kTouchModeNum = 4;

// 检测器判决码。只有 2 会触发切换；4 是 NewModeProcess 的另一条分支入口，
// 四个检测器都不产出它 ⇒ 该分支不可达，结构保留。
enum TouchModeResult : int32_t {
    ModeResultNone      = 0,  // 本帧尚未评估
    ModeResultNotMet    = 1,  // 判据不成立
    ModeResultMet       = 2,  // 判据成立，进入/维持该模式
    ModeResultForceHold = 4,  // 强制保持（本机无产出方）
};

// ── g_tmSwitchPriority @0x6BB4F240（read_memory 实测）──────────────────
// 行 = 上一帧模式，列 = 尝试顺序。NoObject 永远排在最后一位。
inline constexpr uint32_t kTouchModeSwitchPriority[kTouchModeNum][kTouchModeNum] = {
    /* prev = NoObject */ { 1, 3, 2, 0 },
    /* prev = Finger   */ { 1, 3, 2, 0 },
    /* prev = Glove    */ { 1, 2, 3, 0 },
    /* prev = Stylus   */ { 3, 1, 2, 0 },
};

// ── g_tmSwitchDebounce @0x6BB4F200（TouchMode_Init @0x6BA78B60 覆写后）──
// 行 = 源模式，列 = 目标模式，单位帧。Finger→Glove = 0xFFFF，所以手指态下
// 不会直接切手套，必须先回 NoObj 再经 20 帧去抖进 Glove（文档 06 §5）。
inline constexpr uint16_t kTouchModeSwitchNever = 0xFFFFu;
inline constexpr uint16_t kTouchModeSwitchDebounce[kTouchModeNum][kTouchModeNum] = {
    /* NoObject → */ {   0,   0,  20, kTouchModeSwitchNever },
    /* Finger   → */ {  60,   0, kTouchModeSwitchNever, kTouchModeSwitchNever },
    /* Glove    → */ { 200,   0,   0, kTouchModeSwitchNever },
    /* Stylus   → */ { kTouchModeSwitchNever, kTouchModeSwitchNever, kTouchModeSwitchNever, 0 },
};

// ── SWTimer @0x6BB027D0 / 0x6BB02802 / 0x6BB02843 ─────────────────────
// 极简的「起点帧号 + 运行位」计时器。IsExpired 在未运行时返回 2（既非到期
// 也非未到期），调用方只判 == 1。
struct TouchModeSwTimer {
    bool     running = false;
    uint32_t startFrame = 0;

    /// SWTimer_Start：已在运行时**不**刷新起点。
    void Start(uint32_t frameCounter) noexcept {
        if (!running) {
            running = true;
            startFrame = frameCounter;
        }
    }
    void Stop() noexcept { running = false; }

    /// SWTimer_IsExpired：0 = 未到期、1 = 已到期、2 = 未运行。
    [[nodiscard]] int IsExpired(uint32_t frameCounter, uint32_t span) const noexcept {
        if (!running) return 2;
        return ((frameCounter - startFrame) < span) ? 0 : 1;
    }
};

// ─────────────────────────────────────────────────────────────────────
class TouchModeFsm {
public:
    bool m_enabled = true;

    // ── 门控（文档 06 §1 的配置门控总表）──────────────────────────────
    /// DAT_6bcab700 = flash+0x918 = 0x0E。bit1 高灵敏许可 / bit2 手套许可 /
    /// bit3 厚手套许可。`TSADynPrmt_Init` 装载，`Exception_Process` 维护。
    static constexpr uint32_t kCfgWord = 0x0Eu;
    static constexpr uint32_t kFeat    = Gates::kFeatureFlags;
    /// TouchMode_FingerDetect 的外层门：cfg & 2 且 !(feat & 0x1000)。本机真。
    static constexpr bool kFingerDetectOn = (kCfgWord & 2u) != 0 && (kFeat & 0x1000u) == 0;
    /// TouchMode_GloveDetect 外层门的第一项：cfg & 4 且 feat & 1。本机真。
    static constexpr bool kGloveGateMain  = (kCfgWord & 4u) != 0 && (kFeat & 1u) != 0;
    /// 第二项：cfg & 8 且 feat & 0x10（高灵敏）。本机 feat bit4 = 0 ⇒ 假。
    static constexpr bool kGloveGateHiSns = (kCfgWord & 8u) != 0 && (kFeat & 0x10u) != 0;
    /// 第三项：SmartCover。本机 feat bit1 = 0 ⇒ 假。
    static constexpr bool kSmartCoverOn   = Gates::IsOn("FEAT_SMART_COVER");
    /// TouchMode_StylusFilter / SwitchPriorityProcess 的门：cfg & 8 且 feat & 0x10。
    /// 即便成立，改写后的优先级表与静态表逐格相同 ⇒ 无操作，不实现。
    static constexpr bool kFeat0x1000     = (kFeat & 0x1000u) != 0;

    static_assert(kFingerDetectOn && Gates::IsOn("TM_FINGER_DETECT"),
                  "flash+0x918 = 0x0E ⇒ Finger 检测开启（TouchGates 同源）");
    static_assert(kGloveGateMain && Gates::IsOn("TM_GLOVE_DETECT"),
                  "flash+0x918 bit2 与 feat bit0 相与恒真 ⇒ Glove 全族可达");
    static_assert(!Gates::IsOn("TM_PRIORITY_SWITCH") && !Gates::IsOn("TM_STYLUS_DETECT"),
                  "优先级切换与 Stylus 检测本机不可达");

    /// 模式单元（原版 g_tsaPrpt + 0x2E8 起的三个 uint）
    struct Unit {
        uint32_t mode   = TouchModeNoObject;
        uint32_t data   = 0;
        int32_t  result = ModeResultNone;
    };

    /// 每帧输入。字段名后标注原版来源。
    struct Inputs {
        // g_touchesPtr[0]：本帧触点数
        uint8_t  touchCount = 0;
        // g_touchesPtr[0xd]：握持阶段 ∈ {1,2} 的触点数。产出方
        // GripFilter_BasicProcess @0x6BAE4C29 —— 本机握持区非空、边缘触点确实
        // 会走 GripFilter，所以它**不恒为 0**（文档 06 §4.1 第 6 步）。
        uint8_t  gripSuppressedCount = 0;
        // g_tsaStatic[1].field_0x3（= TSAStatic + 0x3F1）：本轮水下异常已触发过
        // 基线重置的一次性哨兵。它与 kGloveGateMain 是**并列的或**，恒 0 不影响
        // 手套通路（文档 06 §5.1）。
        bool     baselineResetInProgress = false;
        // g_tsaStatic + 0x114：全互感信号分散锁存位，FingerProcess 读它。
        bool     fullIncellSignalDisparity = false;
        // g_tsaStaticPtr->nFrameCounter：切换定时器的时基
        uint32_t frameCounter = 0;
        // g_pPeaks + 8：本帧峰值总量。FingerDetect 的 3 帧确认计数器读它。
        int32_t  framePeakSum = 0;
        // flash+0x5D8 = 1000（wPeakSumThreshold）。判据是 3 × 它。
        int16_t  peakSumThold = 1000;
        // DAT_6bcab68c（= TouchTholdSet::sigThold，稳态 600）：FingerDetect 的
        // 「峰中心 Dif ≥ 门限」判据。产出方是 TSADynPrmt_GetPkThold（文档 06 §5）。
        int16_t  fingerPeakThold = 600;
    };

    // ── 只读出口 ────────────────────────────────────────────────────
    [[nodiscard]] uint32_t Mode() const noexcept { return m_unit.mode; }
    [[nodiscard]] uint32_t ModeData() const noexcept { return m_unit.data; }
    [[nodiscard]] int32_t  ModeResult() const noexcept { return m_unit.result; }
    /// DAT_6BC4A924。GloveDetect 置位、FingerDetect 连续 3 帧确认后清零，由
    /// TouchThold_Process / TouchThold_GetPeakThold / TSA_IsHighSensitivityMode 消费。
    [[nodiscard]] bool GloveLatched() const noexcept { return m_gloveLatch; }

    /// Touch_CreatePostReport(i, 5) 的请求位图（GloveDetect 首次锁存时发出）。
    /// 由 TouchPipeline 在本级之后、TouchReport_Process 之前排给模块 07。
    [[nodiscard]] uint32_t TakePostReportRequests() noexcept {
        const uint32_t m = m_postReportMask;
        m_postReportMask = 0;
        return m;
    }

    /// TouchMode_Init @0x6BA78B60
    void Reset() noexcept {
        m_unit = Unit{};
        m_backup = Unit{};
        m_gloveLatch = false;
        m_fingerConfirmCnt = 0;
        m_postReportMask = 0;
        m_timer.Stop();
    }

    // ── TouchMode_Process @0x6BA78DDE ────────────────────────────────
    void Process(TouchBigFrame& bigFrame, const Inputs& in) {
        if (!m_enabled) return;

        // 保存上一帧模式单元（g_tsaPrpt + 0x2F4 起的备份副本）
        m_backup = m_unit;

        Unit prev{};
        prev.mode = m_unit.mode;
        prev.data = 0;                 // 原版这两个是**栈上局部量**，初值为 0，
        prev.result = ModeResultNone;  // 只有优先级循环真的跑到 prev 模式才被填上

        // TouchMode_SwitchPriorityProcess / TouchMode_StylusFilter 都挂在
        // `cfg & 8 ∧ feat & 0x10` 上，本机 feat bit4 = 0 ⇒ 不执行；且前者写入的
        // 四格与静态表逐格相同，是双重无操作。

        Unit next{};
        uint32_t priorityIndex = 0;
        for (; priorityIndex < kTouchModeNum; ++priorityIndex) {
            next.mode = kTouchModeSwitchPriority[prev.mode][priorityIndex];
            Detect(next.mode, bigFrame, in, next.result, next.data);

            if (next.mode == prev.mode) {
                prev.result = next.result;
                prev.data = next.data;
            }

            if (next.result != ModeResultMet) continue;

            if (next.mode == prev.mode) {
                // 维持当前模式：直接跑应用器并停表
                Apply(next.mode, ModeResultMet, next.data, bigFrame, in);
                m_timer.Stop();
            } else {
                NewModeProcess(next, prev, bigFrame, in);
            }
            break;
        }

        // 循环跑满未命中 ⇒ 沿用上一帧模式与其检测结果
        if (priorityIndex >= kTouchModeNum) {
            next = prev;
        }

        m_unit = next;
    }

private:
    Unit m_unit{};
    Unit m_backup{};
    bool m_gloveLatch = false;          // DAT_6bc4a924
    /// g_tsaStaticPtr[1].field_0xc：FingerDetect 的「强峰连续帧」计数器。
    uint8_t m_fingerConfirmCnt = 0;
    uint32_t m_postReportMask = 0;
    TouchModeSwTimer m_timer{};         // g_tmSwitchTimer @0x6BBC8660

    // ── TouchMode_NewModeProcess @0x6BA78C16 ─────────────────────────
    // 切到**不同**模式的流程：先无条件跑一次新模式的应用器（所以抑制位在
    // 防抖期内每帧都会被打上），再用切换定时器决定这一帧是否真的改写模式字。
    void NewModeProcess(Unit& next, const Unit& prev, TouchBigFrame& bigFrame, const Inputs& in) {
        if (prev.result < ModeResultMet) {
            Apply(next.mode, next.result, next.data, bigFrame, in);

            // prev.data == 1 且目标不是 NoObject 时零防抖直切。
            const uint32_t span =
                (prev.data == 1 && next.mode != TouchModeNoObject)
                    ? 0u
                    : static_cast<uint32_t>(kTouchModeSwitchDebounce[prev.mode][next.mode]);

            m_timer.Start(in.frameCounter);
            if (m_timer.IsExpired(in.frameCounter, span) == 1) {
                // 到期：接受 next（mode/data/result 保持检测器给出的值），停表
                m_timer.Stop();
            } else {
                // 未到期：回退到上一帧模式。prev 的载荷若本帧还没被填过
                // （prev.result == 0，即优先级表里 prev 模式排在命中项之后），
                // 就取帧首备份副本。
                if (prev.result == ModeResultNone) {
                    next.result = m_backup.result;
                    next.data = m_backup.data;
                } else {
                    next.result = prev.result;
                    next.data = prev.data;
                }
                next.mode = prev.mode;
            }
        } else if (prev.result == ModeResultForceHold) {
            // 不可达分支：四个检测器都不产出 4。结构保留。
            Apply(next.mode, next.result, next.data, bigFrame, in);
            // 原版判据是 g_touchesPtr[3]（取消计数）+ g_touchesPtr[0]（触点数）== 0。
            if (in.touchCount + bigFrame.cancelledTouchCount == 0) {
                next = prev;
            }
            m_timer.Stop();
        }
    }

    // ── 检测器分发（g_tmHandlerTable 偶数项）──────────────────────────
    void Detect(uint32_t mode, TouchBigFrame& bigFrame, const Inputs& in,
                int32_t& result, uint32_t& data) {
        switch (mode) {
        case TouchModeNoObject: NoObjDetect(in, result, data); break;
        case TouchModeFinger:   FingerDetect(bigFrame, in, result, data); break;
        case TouchModeGlove:    GloveDetect(bigFrame, in, result, data); break;
        default:                StylusDetect(result, data); break;
        }
    }

    // ── 应用器分发（g_tmHandlerTable 奇数项）──────────────────────────
    void Apply(uint32_t mode, int32_t result, uint32_t data,
               TouchBigFrame& bigFrame, const Inputs& in) {
        switch (mode) {
        case TouchModeFinger: FingerProcess(result, data, bigFrame, in); break;
        case TouchModeGlove:  GloveProcess(result, data, bigFrame, in); break;
        // NoObjProcess @0x6BA79113 与 StylusProcess @0x6BA7991A 的函数体都只有
        // 一句 return，原版就是空实现。
        default: break;
        }
    }

    // ── TouchMode_NoObjDetect @0x6BA790AE ────────────────────────────
    // 「屏上所有触点都处于握持阶段 1/2」⇒ 视作无对象。握持通道在本机是活的，
    // 所以这条不再退化成「一个触点都没有」。
    static void NoObjDetect(const Inputs& in, int32_t& result, uint32_t& data) {
        result = ModeResultNotMet;
        data = 2;
        if (in.touchCount == in.gripSuppressedCount) {
            result = ModeResultMet;
            data = 2;
        }
    }

    // ── TouchMode_FingerDetect @0x6BA7911F ───────────────────────────
    // 外层门 `cfg & 2 ∧ !(feat & 0x1000)` 在本机为真（文档 06 §1）。
    //   result = 2：存在一个「非 FingerZFailed 且峰中心 Dif ≥ DAT_6bcab68c」的触点。
    //   data   = 2：强峰（峰值总量 > 3 × flash+0x5D8）连续 3 帧 ⇒ 同时清手套闩锁。
    // 计数器在模式尚未稳定在 Finger（mode != 1）时清零，已稳定且 data == 2 时冻结。
    void FingerDetect(const TouchBigFrame& bigFrame, const Inputs& in,
                      int32_t& result, uint32_t& data) {
        result = ModeResultNotMet;
        data = 1;
        if constexpr (!kFingerDetectOn) return;

        for (const auto& t : bigFrame.tracks) {
            if ((t.volatileFlags & TOUCH_FLAG_FINGER_Z_FAILED) != 0) continue;
            if (t.peakSig >= in.fingerPeakThold) { result = ModeResultMet; break; }
        }

        if (m_unit.mode != TouchModeFinger) m_fingerConfirmCnt = 0;
        if (m_unit.mode != TouchModeFinger || m_unit.data != 2) {
            if (in.framePeakSum > static_cast<int32_t>(in.peakSumThold) * 3) {
                if (m_fingerConfirmCnt < 255) ++m_fingerConfirmCnt;
            } else {
                m_fingerConfirmCnt = 0;
            }
        }
        if (m_fingerConfirmCnt > 2) {
            data = 2;
            m_gloveLatch = false;          // DAT_6bc4a924 = 0
        }
    }

    // ── TouchMode_GloveDetect @0x6BA793FE ────────────────────────────
    // 外层门是四选一的析取，本机第一项（cfg&4 ∧ feat&1）恒真。
    //   result = 2：存在未被握持抑制的触点（触点数 != hdr[0xD]）。
    //   闩锁路径：尚未锁存且无 SmartCover / 基线复位哨兵时遍历触点，命中
    //     「已从按下点移动 ≥ 15 mm（0x1F8 & 4）∧ 模式不在 Finger ∧ 未被排除位
    //      0x80000 否决」即锁存、data = 2 并请求 Touch_CreatePostReport(i, 5)；
    //     已锁存（或走 SmartCover / 哨兵支路）则只把 data 置 2。
    void GloveDetect(const TouchBigFrame& bigFrame, const Inputs& in,
                     int32_t& result, uint32_t& data) {
        result = ModeResultNotMet;
        data = 1;

        const bool sentinel = kSmartCoverOn || in.baselineResetInProgress;
        if constexpr (!kGloveGateMain && !kGloveGateHiSns) {
            if (!sentinel) return;
        }
        if (!(kGloveGateMain || kGloveGateHiSns || sentinel)) return;

        if (in.touchCount != in.gripSuppressedCount) {
            result = ModeResultMet;
        }

        // 原版的第二次析取：`(!latch || !(主门 || 高灵敏门)) && !SmartCover && !哨兵`
        const bool takeScanPath =
            (!m_gloveLatch || !(kGloveGateMain || kGloveGateHiSns)) && !sentinel;

        if (!takeScanPath) {
            m_gloveLatch = true;
            data = 2;
            return;
        }

        for (const auto& t : bigFrame.tracks) {
            if constexpr (kGloveGateMain) {
                if ((t.persistFlags1F8 & TOUCH_PFLAG_MOVED_15MM) != 0 &&
                    m_unit.mode != TouchModeFinger &&
                    (t.persistFlags1F8 & TOUCH_PFLAG_GLOVE_EXCL) == 0) {
                    m_gloveLatch = true;
                    data = 2;
                    if (t.id > 0 && t.id < 32) m_postReportMask |= (1u << t.id);
                    return;
                }
            }
            if constexpr (kFeat0x1000) {   // 本机 0，结构保留
                m_gloveLatch = true;
                data = 2;
                return;
            }
        }
    }

    // ── TouchMode_StylusDetect @0x6BA798B8 ───────────────────────────
    // 函数体只有两句赋值，没有任何判据 ⇒ 恒返回 (1, 1)，
    // **Stylus 模式在本机（以及任何整定表下）都不可达**。
    static void StylusDetect(int32_t& result, uint32_t& data) {
        result = ModeResultNotMet;
        data = 1;
    }

    // ── TouchMode_FingerProcess @0x6BA79318 ──────────────────────────
    // `if (data == 2 || DAT_6bc4a924 == 0)`：把「手指 Z 不足」（+0x1f0 bit11）
    // 且未锁存全互感信号分散的触点标成弱信号（+0x208 |= 2）。
    // ⚠ 原版把 bFullIncellSignalDisparity 判在**循环内**，与逐点条件并列。
    void FingerProcess(int32_t result, uint32_t data, TouchBigFrame& bigFrame, const Inputs& in) {
        (void)result;
        if (data != 2 && m_gloveLatch) return;

        for (auto& t : bigFrame.tracks) {
            if ((t.volatileFlags & TOUCH_FLAG_FINGER_Z_FAILED) != 0 &&
                !in.fullIncellSignalDisparity) {
                t.suppressFlags |= TOUCH_SUPPRESS_GLOVE_WEAK;
            }
        }
    }

    // ── TouchMode_GloveProcess @0x6BA797A7 ───────────────────────────
    // 三选一（前置 `!(feat & 0x1000)`，本机成立）：
    //   a) 手套门全部不成立 ⇒ TouchMode_StylusSuppression（全点置 bit2）
    //   b) 成立且 data == 1  ⇒ 全点置 bit1
    //   c) 成立且 data != 1  ⇒ TouchMode_GloveFilter（按条件置 bit1）
    // 本机 a) 的判据恒假（kGloveGateMain 为真）⇒ **touch + 0x208 bit2 无产出方**。
    void GloveProcess(int32_t result, uint32_t data, TouchBigFrame& bigFrame, const Inputs& in) {
        (void)result;
        if constexpr (kFeat0x1000) return;

        const bool gateOn = kGloveGateMain || kSmartCoverOn || in.baselineResetInProgress;
        if (!gateOn) {
            StylusSuppression(bigFrame);
        } else if (data == 1) {
            for (auto& t : bigFrame.tracks) t.suppressFlags |= TOUCH_SUPPRESS_GLOVE_WEAK;
        } else {
            GloveFilter(bigFrame);
        }
    }

    // ── TouchMode_GloveFilter @0x6BA796D1 ────────────────────────────
    //   if (!(touch + 0x208 & 1) && (byte)(touch + 0x1c8) > 0x40) touch + 0x208 |= 2;
    // touch + 0x1c8 是 TZ_UpdateGripRatio @0x6BA7FB15 写的**第二个**边沿能量比
    // ——与 touch + 0x198 同出 TZ_CalcGripRatio，只是方向掩码取 _DAT_6bcab704。
    // 该整定量在 Ghidra 里无写入方，复刻沿用掩码 0xC 的 gripRatio 作等价量。
    static void GloveFilter(TouchBigFrame& bigFrame) {
        for (auto& t : bigFrame.tracks) {
            if ((t.suppressFlags & TOUCH_SUPPRESS_GLOVE_BASE) != 0) continue;
            if (t.gripRatio > 0x40) t.suppressFlags |= TOUCH_SUPPRESS_GLOVE_WEAK;
        }
    }

    // ── TouchMode_StylusSuppression @0x6BA79BA6 ──────────────────────
    // 无条件给全部触点打上 bit2。唯一调用方是 GloveProcess 的 a) 分支，
    // 本机不可达 —— 保留结构备换表。
    static void StylusSuppression(TouchBigFrame& bigFrame) {
        for (auto& t : bigFrame.tracks) t.suppressFlags |= TOUCH_SUPPRESS_BY_STYLUS;
    }
};

} // namespace Solvers::Touch
