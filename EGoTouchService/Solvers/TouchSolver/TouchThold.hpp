#pragma once
// ── TouchPipeline Stage 2.5: TouchThold（每帧运行时动态门限重算）───────
// Header-only C++23。对位原版 TSA_MSProcess 中 Peak_Process 前一行的
// TSAPrmt_Process() 调用链（文档 02 §14）：
//   TSAPrmt_Process        @0x6BAFBACB
//     → TSADynPrmt_Process @0x6BB3D12D
//     → TSADynPrmt_CoreProcess @0x6BB3D114 → TSADynPrmt_GetSig @0x6BB3D07D
//     → TSADynPrmt_GetPkThold  @0x6BB3CD91
//         ├─ TouchThold_Process @0x6BA7D0EE (420, 220) → 边缘副本
//         └─ TouchThold_Process @0x6BA7D0EE (600, 220) → 主门限组
//
// ══ 峰值门限的模式迟滞（文档 02 §14、§17 第 9 条）══════════════════════
// 先前版本把「peakThold 恒 600、220 只是异常低基准」写进注释，**那是错的**。
// `TouchThold_GetPeakThold @0x6BA7CEE9` 的第一个 if 在本机恒假（手套许可位
// cfg&4 与 dwFeatureFlags bit0 相与为真），真实走的永远是 else 分支：
//
//   ┌ mode == Finger(1) && stage == 2 && (cfg & 2) → 600（信号失配则 300）
//   └ 其他（NoObj / 手指未确认 / Glove）            → 220
//
// 即**落指确认前门限是 220，确认后才升到 600**；单指稳定再 ×3/4 = 450。
// 这是本机的正常工作方式，不是异常分支。少了它落指首帧会明显偏钝。
//
// mode / stage 取自 `DAT_6BC4A928`，由 `TouchMode_Process @0x6BA78DDE` 在
// **帧尾**（GripFilter 之后）写；`TSAPrmt_Process` 排在 Peak 之前，因此本级
// 读到的永远是**上一帧**的模式 —— 这是跨帧回路，不是帧内回路。详见下面
// 「时序」一节。
//
// 本模块只负责产出门限；Peak / TouchZone / Tracking 各级如何消费它们
// 属于下游各自的职责，这里不碰。
// ─────────────────────────────────────────────────────────────────────

#include <cstdint>
#include <cstddef>
#include <algorithm>

#include "TouchFixed.hpp"
#include "TouchGates.hpp"

namespace Solvers::Touch {

// ── Flash 整定常量（g_tsaPrmtFlashGaokunHimaxCSOT，tsaprmt-table-reader 实测）─
inline constexpr int16_t kPeakMode3ThresholdMax   = 600;   // flash 0x5B6 = 0x0258
inline constexpr int16_t kPeakTholdAltBase        = 220;   // flash 0x5DA = 0x00DC
inline constexpr int16_t kPeakFilterReferenceThld = 3000;  // flash 0x5B4 = 0x0BB8
inline constexpr uint8_t kZTholdRatioQ4           = 48;    // flash 0x5D0 = 0x0030
inline constexpr int32_t kPeakProcessingMode      = 3;     // flash 0x34  = 3

// TouchThold_GetNoiseThold @0x6BA7CA48 的三档噪声基准与 SNR 缩放
inline constexpr int16_t kNoiseBaseIdle   = 100;  // flash 0x90C = 0x0064
inline constexpr int16_t kNoiseBaseSingle = 150;  // flash 0x90E = 0x0096
inline constexpr int16_t kNoiseBaseMulti  = 200;  // flash 0x910 = 0x00C8
inline constexpr int32_t kNoiseSnrScaleQ4 = 20;   // flash 0x914 = 0x0014 → ×20/16
inline constexpr int32_t kNoisyTouchM0Thold = 6000; // flash 0x912 = 0x1770

// DAT_6BCAB700 = flash+0x918 = 0x0E（实测 `0E 02 00 00 01 00 00 00`）。
//   bit1 (0x2) 高灵敏/Finger 模式回拨许可
//   bit2 (0x4) 手套许可（配 dwFeatureFlags bit0）
//   bit3 (0x8) 厚手套许可（配 dwFeatureFlags bit4，本机该特性位为 0）
inline constexpr uint32_t kTholdCfgWord = 0x0Eu;

// TSADynPrmt_GetPkThold 第一次 TouchThold_Process 的高基准：600 × 0x38 / 0x50 = 420
inline constexpr int32_t kDynLowBaseNumerator   = 0x38;
inline constexpr int32_t kDynLowBaseDenominator = 0x50;

// DAT_6BC4A928 的低 32 位（触控模式）取值，与 TouchModeFsm::TouchModeId 同义。
inline constexpr uint32_t kTholdModeNoObject = 0;
inline constexpr uint32_t kTholdModeFinger   = 1;
inline constexpr uint32_t kTholdModeGlove    = 2;
inline constexpr uint32_t kTholdStageConfirmed = 2;  // stage == 2 表示该模式已确认

// ── 每帧输入状态 ────────────────────────────────────────────────────
struct TouchTholdInputs {
    // g_tsaStatic + 0x114：全互感信号分散锁存位。Peak_Process 写、
    // 下一帧 TouchThold_GetPeakThold 读，构成跨帧闭环。
    bool    bFullIncellSignalDisparity = false;

    // g_tsaStatic + 0x3F1（= g_tsaStaticPtr[1].field_0x3）。
    // ⚠ 它**不是**「基线复位进行中」。文档 02 §9 与 `BLReset_Check @0x6BA48AF0`
    // 的反编译一致：它只在 `g_pPrevTouches->nTouchCount == 0x2000`（水下异常字）
    // 且发生基线重置时置 1，而 BLReset_Check / SafeBaseline_ResetCheck 每帧都在
    // `nTouchCount != 0x2000` 时立刻清 0。本机不产生 0x2000 异常字 ⇒ **恒 0**。
    // 早先把它接到普通基线复位上，会让任何一次基线重置都把整组门限压到 220。
    bool    underwaterResetSentinel = false;

    // g_pPrevTouches->bPrevTouchCount：上一帧触点数
    uint8_t prevTouchCount = 0;

    // g_pPrevTouches->field_0x5 == 1：上一帧实际上报的触点数为 1
    bool    prevTouchStable = false;

    // g_pPrevTouches[0xd].field_0x1c & 0x1000：单指折扣的排除位
    bool    prevSingleTouchExcluded = false;

    // g_tsaPrpt.nSPeakPositive（+0xA0）：本帧全屏最大正差分，由 TSA_GetDifPrpt
    // 在 CMF/GridIIR 之后算出。只被 TouchThold_GetTouchDownThold 消费。
    int16_t framePeakPositive = 0;

    // ── DAT_6BC4A928：触控模式 / 阶段（低 32 位 = mode，高 32 位 = stage）──
    // 由 TouchMode_Process 在**帧尾**写，本级读到的是上一帧结论。
    uint32_t touchMode  = kTholdModeNoObject;
    uint32_t touchStage = 0;

    // DAT_6BC4A924：运行期手套闩锁。TouchMode_GloveDetect 置 1、
    // TouchMode_FingerDetect 连续确认 3 帧后清 0。只影响 *outNoise 的基准。
    bool    gloveLatched = false;

    // TouchThold_GetNoisyTouchNum @0x6BA7C896：上一帧中 touch[i]+0x1CC（M0）
    // 大于 flash[0x912] = 6000 的触点个数。
    uint8_t noisyTouchCount = 0;
};

// ── 每帧输出门限组 ──────────────────────────────────────────────────
// 字段名后标注原版全局量地址。默认值 = TouchThold_Init @0x6BA7C922 的上电初值。
struct TouchTholdSet {
    // DAT_6BC4A912 = GetTholdBasedOnSNR(flash[0x910]) = 200×20/16 = 250
    int16_t peakThold       = 250;
    int16_t detectThold     = 250;                     // Peak_Process 钳后探测门限
    int16_t z1Thold         = 250;                     // Peak_Z1Filter 回退门限
    int16_t sigThold        = kPeakMode3ThresholdMax;  // DAT_6BC4A914 = 600
    int16_t downThold       = kPeakMode3ThresholdMax;  // DAT_6BC4A91A = 600
    // DAT_6BC4A910。**TouchThold_Init 把它置成 flash[0x5B6] = 600，不是 125**。
    // 稳态经 GetNoiseThold 收敛到 100×20/16 = 125（单强触点 187 / 多触点 250）。
    int16_t noiseThold      = kPeakMode3ThresholdMax;

    int32_t downZThold      = 3 * kPeakMode3ThresholdMax; // DAT_6BCAB690 = 48×sigThold/16
    int32_t stableZThold    = 3 * kPeakMode3ThresholdMax; // DAT_6BCAB694 = DAT_6BC4A918×3
    int32_t secondaryZThold = 3 * kPeakMode3ThresholdMax; // DAT_6BCAB698 = 48×downThold/16

    // 边缘（side）副本：TSADynPrmt_GetPkThold 用 (420, 220) 再跑一遍
    int16_t peakTholdLow    = 420;                     // DAT_6BCAB68A
    int16_t sigTholdLow     = 420;                     // DAT_6BCAB68E

    int16_t zTholdRatioQ4   = kZTholdRatioQ4;          // DAT_6BCAB69C
    int16_t peakFilterRef   = kPeakFilterReferenceThld;// g_tsaPrmtDynamic
};

// ─────────────────────────────────────────────────────────────────────
class TouchThold {
public:
    bool m_enabled = true;

    // DAT_6BC4A918：TouchThold_Init 置为 wPeakMode3ThresholdMax = 600，
    // 此后只有 HandGesture_TouchProcess / SideTouch_TouchProcess 会改写它。
    // MS 手指路径不触碰，故常态恒 600。
    int16_t m_stableTholdBase = kPeakMode3ThresholdMax;

    // 跨帧输入状态。SD 锁存位由 Peak 级回写、prev* 三项与 mode/stage/手套闩锁
    // 由 TouchPipeline 在帧尾回写。
    TouchTholdInputs m_inputs{};

    const TouchTholdSet& Result() const { return m_result; }

    /// DAT_6BC4A910 的当前值。CMF_ProcessDimUnit 与 TSAPrpt_GetDifPreCMFPrpt
    /// 在 TSAPrmt_Process 之**前**执行，读到的因此是上一帧的值（文档 02 §14
    /// 「关键时序」）。Stage 2 必须在本级跑之前取它。
    [[nodiscard]] int16_t NoiseTholdPrevFrame() const noexcept { return m_noiseThold; }

    const TouchTholdSet& Process() { return Process(m_inputs); }

    // ── TSADynPrmt_GetPkThold @0x6BB3CD91 ────────────────────────────
    const TouchTholdSet& Process(const TouchTholdInputs& in) {
        if (!m_enabled) {
            return m_result;
        }

        TouchTholdSet out{};

        // 边缘副本：TouchThold_Process(600 × 0x38 / 0x50 = 420, 220, ...)
        const int16_t sideBase = static_cast<int16_t>(
            (static_cast<int32_t>(kPeakMode3ThresholdMax) * kDynLowBaseNumerator)
            / kDynLowBaseDenominator);
        const TholdTriple side = ProcessOne(sideBase, kPeakTholdAltBase, in);
        out.peakTholdLow = side.peak;
        out.sigTholdLow  = side.sig;

        // 主组：TouchThold_Process(600, 220, ...) → DAT_6BC4A912/914/91A
        const TholdTriple main = ProcessOne(kPeakMode3ThresholdMax, kPeakTholdAltBase, in);
        out.peakThold  = main.peak;   // DAT_6BCAB688 = DAT_6BC4A912
        out.sigThold   = main.sig;    // DAT_6BCAB68C = DAT_6BC4A914
        out.downThold  = main.down;   // DAT_6BC4A91A
        out.noiseThold = m_noiseThold;// DAT_6BC4A910（GetNoiseThold 的副作用）

        out.zTholdRatioQ4 = kZTholdRatioQ4;                 // DAT_6BCAB69C = flash[0x5D0]
        out.peakFilterRef = kPeakFilterReferenceThld;       // g_tsaPrmtDynamic = flash[0x5B4]

        // 三个 Z 幅度准入门限（Touch_SetVolatileFlag* 消费）
        out.downZThold      = ScaleQ4(kZTholdRatioQ4, out.sigThold);        // DAT_6BCAB690
        out.stableZThold    = static_cast<int32_t>(m_stableTholdBase) * 3;  // DAT_6BCAB694
        out.secondaryZThold = ScaleQ4(kZTholdRatioQ4, out.downThold);       // DAT_6BCAB698

        // ── Peak_Process @0x6BA68C4B 开头的两个派生门限 ──────────────
        //   nOriginalPeakThreshold = DAT_6BCAB688;                       ← Z1 回退用
        //   if (mode == 3 && wPeakMode3ThresholdMax/2 < DAT_6BCAB688)
        //       DAT_6BCAB688 = wPeakMode3ThresholdMax/2;                 ← 探测用
        // 即「先按 300 宽松探测、未检出信号分散再用原门限把弱峰撤销」。
        // 注意：手指确认前 peakThold 已经是 220 < 300，此时钳位不生效。
        out.z1Thold = out.peakThold;
        out.detectThold = out.peakThold;
        if constexpr (kPeakProcessingMode == 3) {
            const int16_t clampTo = static_cast<int16_t>(kPeakMode3ThresholdMax / 2);
            if (clampTo < out.detectThold) {
                out.detectThold = clampTo;
            }
        }

        m_result = out;
        return m_result;
    }

    /// TouchThold_Init @0x6BA7C922。首帧 DAT_6BC4A910 = flash[0x5B6] = **600**
    /// （不是稳态的 125）—— 首帧 CMF 与 pre-CMF 属性统计因此偏保守，必须保留。
    void Reset() noexcept {
        m_result = TouchTholdSet{};
        m_noiseThold = kPeakMode3ThresholdMax;
    }

    /// TouchThold_GetNoiseThold @0x6BA7CA48 的**纯函数体**（无副作用）。
    /// 原版 `DAT_6BC4A910` 是一个全局量，写入者只有 TouchThold_Process；
    /// HardwareAnalyzer_GetRaw2RawPrpt 等其它模块只是**读**同一个全局。
    /// 抽成 public static 以消除各模块自带一份私有拷贝的重复实现。
    [[nodiscard]] static int16_t ComputeNoiseThold(uint8_t noisyTouchCount) noexcept {
        int32_t c = kNoiseBaseIdle;
        if (noisyTouchCount == 1) {
            c = std::max<int32_t>(c, kNoiseBaseSingle);
        } else if (noisyTouchCount >= 2) {
            c = std::max<int32_t>(c, kNoiseBaseMulti);
        }
        // GetTholdBasedOnSNR：c × flash[0x914](20) / 16，负数向零取整
        return static_cast<int16_t>(Fixed::ShrTowardZero(kNoiseSnrScaleQ4 * c, 4));
    }

    // g_tsaPrpt.nSPeakPositive 的等价求值：CMF/GridIIR 之后全屏最大正差分。
    // 原版分块求最大再 TSA_CombineBlock 归并，结果等同于整屏最大值。
    static int16_t ScanFramePeakPositive(const int16_t* diff, size_t count) {
        int16_t maxSig = 0;
        for (size_t i = 0; i < count; ++i) {
            if (diff[i] > maxSig) {
                maxSig = diff[i];
            }
        }
        return maxSig;
    }

private:
    struct TholdTriple {
        int16_t peak = 0;
        int16_t sig  = 0;
        int16_t down = 0;
    };

    TouchTholdSet m_result{};

    // DAT_6BC4A910。TouchThold_GetNoiseThold 每帧写它；初值见 Reset()。
    int16_t m_noiseThold = kPeakMode3ThresholdMax;

    // (ratio × value) / 16，负数向零取整（原版 `v<0 ? v+0xF : v` 后算术右移 4）
    static int32_t ScaleQ4(int32_t ratio, int32_t value) {
        return Fixed::ShrTowardZero(ratio * value, 4);
    }

    // ── 三条许可位（文档 02 §16「手套分支许可」/「厚手套许可」）────────
    // cfg & 4 ∧ dwFeatureFlags bit0 = 1 ∧ 1 ⇒ **恒真**，于是 GetPeakThold 的
    // 第一个 if（「普通模式」支）在本机是**死代码**。
    static constexpr bool kGloveAllowed =
        (kTholdCfgWord & 0x4u) != 0 && (Gates::kFeatureFlags & 0x1u) != 0;
    static constexpr bool kThickGloveAllowed =
        (kTholdCfgWord & 0x8u) != 0 && (Gates::kFeatureFlags & 0x10u) != 0;
    static constexpr bool kSmartCover = Gates::IsOn("FEAT_SMART_COVER");
    static constexpr bool kSensitivityModeAllowed = (kTholdCfgWord & 0x2u) != 0;

    static_assert(kGloveAllowed,
                  "flash+0x918 bit2 与 dwFeatureFlags bit0 均为 1 ⇒ GetPeakThold "
                  "的『普通模式』分支是死代码，门限迟滞走 else 支（02 §14）");
    static_assert(!kThickGloveAllowed, "dwFeatureFlags bit4 = 0（02 §16）");
    static_assert(!kSmartCover, "本机无皮套（02 §16）");

    // ── TouchThold_Process @0x6BA7D0EE ───────────────────────────────
    TholdTriple ProcessOne(int16_t pk, int16_t alt, const TouchTholdInputs& in) {
        TholdTriple r{};

        const int16_t v = GetPeakThold(pk, alt, in);

        // 上一帧恰好是「单个稳定触点」时，峰值门限打 3/4 折（向零取整）
        if (in.prevTouchCount == 1 && in.prevTouchStable && !in.prevSingleTouchExcluded) {
            r.peak = static_cast<int16_t>(Fixed::ShrTowardZero(static_cast<int32_t>(v) * 3, 2));
        } else {
            r.peak = v;
        }

        // TouchThold_BasedOnExperience @0x6BA7CB32：函数首行即
        //   if (dwPeakProcessingMode != 3 && (s = TSA_GetMinSigPrpt(), mode != 3)) { ... }
        //   return param_1;
        // 本机 mode == 3 ⇒ **恒等返回**，`*param_3 = max(*param_3, e)` 退化成空操作。
        // （Ghidra 反编译逐行核对，不实现其内部的 nSPeakPositive/4 分支。）

        // ── 第二段：*outNoise 的基准选择 ──────────────────────────────
        // 与 GetPeakThold 的判据**不同** —— 手套许可前多一个 DAT_6BC4A924 前置：
        //   if (!(gloveLatch && (gloveAllowed || thickGloveAllowed))
        //       && !smartCover && !sentinel && !signalDisparity)  base = pk;
        //   else                                                   base = alt;
        // 普通手指场景 gloveLatch == 0 且 SD == 0 ⇒ base 恒 600，
        // 门限组的**下限**不受迟滞影响，只有峰值门限在动。
        const bool gloveBypass =
            in.gloveLatched && (kGloveAllowed || kThickGloveAllowed);
        int16_t base = (!gloveBypass && !kSmartCover && !in.underwaterResetSentinel &&
                        !in.bFullIncellSignalDisparity)
                           ? pk
                           : alt;

        const int16_t noise = GetNoiseThold(in);   // 副作用：写 m_noiseThold
        if (base < noise)  base = noise;
        if (base < r.peak) base = r.peak;
        r.sig = base;

        r.down = GetTouchDownThold(r.sig, in);
        return r;
    }

    // ── TouchThold_GetPeakThold @0x6BA7CEE9 ──────────────────────────
    // 逐行对位反编译。第一个 if 在本机恒假（见 kGloveAllowed 的 static_assert），
    // 真实路径是 else：模式迟滞。
    static int16_t GetPeakThold(int16_t pk, int16_t alt, const TouchTholdInputs& in) {
        int16_t v = pk;

        if constexpr (!kGloveAllowed && !kThickGloveAllowed) {
            // 【死代码】本机不可达，保留骨架以对位原版结构。
            if (!kSmartCover && !in.underwaterResetSentinel) {
                if (kPeakProcessingMode == 3 && in.bFullIncellSignalDisparity) {
                    v = static_cast<int16_t>(pk / 2);
                }
                return v;
            }
        }

        // 原版写作 `(2 >> (mode & 0x1f)) & 1`，即「mode == 1（Finger）」的位测试。
        const bool modeIsFinger = ((2u >> (in.touchMode & 0x1Fu)) & 1u) != 0;
        const bool fingerConfirmed = modeIsFinger && in.touchStage == kTholdStageConfirmed;

        if (!fingerConfirmed || !kSensitivityModeAllowed || kSmartCover) {
            v = alt;   // 220 —— 落指确认前 / NoObj / Glove
            if constexpr (!kSmartCover) {
                // Glove 稳态 + TouchThold_UseHighTholdAtSensitivityMode() → 360。
                // 后者按工程名匹配 P116/P150/P172/P173/P186，W273AS2700 不匹配
                // ⇒ 恒 0，这条不可达（Ghidra @0x6BA7CDE1 核实）。
                (void)in;
            }
        } else if (kPeakProcessingMode == 3 && in.bFullIncellSignalDisparity) {
            v = static_cast<int16_t>(pk / 2);   // 600 → 300
        }
        return v;
    }

    // ── TouchThold_GetNoiseThold @0x6BA7CA48 ─────────────────────────
    //   c = flash[0x90C] (100);
    //   n = GetNoisyTouchNum();
    //   if (n == 1) c = max(c, flash[0x90E] /*150*/);
    //   else if (n >= 2) c = max(c, flash[0x910] /*200*/);
    //   a = GetTholdBasedOnSNR(c) = c * flash[0x914](20) / 16;   → 125 / 187 / 250
    //   if (TSAStatic_IsSafeBaselineApplied())  a = max(a, GetTholdBasedOnTouchMode(pk, alt));
    //   DAT_6BC4A910 = a;  return a;
    //
    // 早先版本把它短路成恒 0（依据「那几个 DAT 位于未初始化 .bss」），
    // 表实测推翻了该前提：flash+0x90C/0x90E/0x910/0x914 = 100/150/200/20。
    // FEAT_SAFE_BASELINE 关闭 ⇒ GetTholdBasedOnTouchMode 的返回值被整体丢弃。
    int16_t GetNoiseThold(const TouchTholdInputs& in) {
        static_assert(!Gates::IsOn("FEAT_SAFE_BASELINE"),
                      "SafeBaseline 关闭 ⇒ GetTholdBasedOnTouchMode 的结果被丢弃");
        m_noiseThold = ComputeNoiseThold(in.noisyTouchCount);
        return m_noiseThold;
    }

    // ── TouchThold_GetTouchDownThold @0x6BA7CCA5（按汇编还原）─────────
    //   v = noiseThold;
    //   if (mode == 3 && !(dwFeatureFlags & 0x1000))
    //       v = min(g_tsaPrpt.nSPeakPositive, flash[0x5B4] /*3000*/) / 3;   // 向零截断
    //   return max(v, noiseThold);
    static int16_t GetTouchDownThold(int16_t sigThold, const TouchTholdInputs& in) {
        int32_t v = sigThold;
        if constexpr (kPeakProcessingMode == 3 && (Gates::kFeatureFlags & 0x1000u) == 0) {
            const int32_t capped =
                std::min<int32_t>(in.framePeakPositive, kPeakFilterReferenceThld);
            v = Fixed::TruncDiv(capped, 3);   // IMUL 0x5556 / SHR 16 / SAR 15 = 向零截断
        }
        return static_cast<int16_t>(std::max<int32_t>(v, sigThold));
    }
};

} // namespace Solvers::Touch
