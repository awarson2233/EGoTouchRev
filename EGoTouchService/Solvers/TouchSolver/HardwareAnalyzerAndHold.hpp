#pragma once
// ── TouchPipeline Stage 0.5: HardwareAnalyzerAndHold ──────────────────
// Header-only C++23. 对位 TSACore 的硬件异常分析与异常帧保持：
//   1. HardwareAnalyzer_GetRaw2RawPrpt:
//      - raw(t) − raw(t−1) 的全屏帧间差分统计
//      - 四级计数 numOverNoise / numOverLTI / numOverHTI / numOverBigNoise
//      - 过噪节点的正负能量分别求和 (sumPos / sumNeg)
//   2. HardwareAnalyzer_ProcessByRaw2Raw:
//      - 由上述统计置位硬件异常码 2 (raw 不稳) 与 0x1D (大噪声)
//   3. HardwareAnalyzer_IsToBypassCurrentFrame:
//      - mode 3 下 bypass = 异常 2 或异常 0x1D
//   4. Touch_KeepPrevTouchWithExceptionExcluded:
//      - bypass 帧整帧保持上一帧的触点集合，跳过 Stage 1~8
//   5. TSA_RawCheckProcess @0x6BABCB0B / TSA_RawCheckFrozen @0x6BAFCA3C:
//      - 冻结帧检测（连续 10 帧逐元素完全一致）
// 规范：docs/finger_algorithms/01_baseline_and_diff.md §3 / §5。
//
// ── raw2raw 差分的符号（§5）────────────────────────────────────────────
// 原版 `HardwareAnalyzer_GetRaw2RawPrpt @0x6BAF10DC` 走
//   BLIIR_CalcDiffCommon(g_tsaBufRaw2rawDif, g_tsaBufRaw, g_tsaBufPreHWRaw)
// 而 CalcDiffCommon 在 bField_0x11a == 0 时算的是 `param3 - param2`，
// 即 **preHWRaw − raw**。本机 MasterFrameParser 输出的 Raw 与 TSACore 的绝对
// 互容读数符号相反（手指抬高本机读数、压低 TSACore 读数，见 BaselineAndDiff
// 文件头），两处反号相消 ⇒ 复刻这里算 `raw − prevRaw` 与原版逐格等值。
//
// ── Rx 分组（§5）──────────────────────────────────────────────────────
// 原版按 `flash+0x62C` 的通道映射表把列分到 DAT_6BCAB626 个 Rx 组里分别统计，
// 只有 `_24_1_`（超噪占比最大值）依赖分组。实测 flash+0x62C..+0x68B 全 0
// ⇒ 只有一组、覆盖全部 2400 个节点 ⇒ 分组循环退化成平坦扫描，
// 占比 = (numOverNoise × 100) / 2400。下面的平坦实现与原版逐值相等。
// ─────────────────────────────────────────────────────────────────────

#include <array>
#include <cstdint>
#include <cstring>
#include <algorithm>

#include "SolverTypes.h"
#include "TouchCoreTypes.hpp"

namespace Config {
class ConfigBinder;
class ConfigStore;
}

namespace Solvers::Touch {

class HardwareAnalyzerAndHold {
public:
    bool m_enabled = true;

    // ── W273AS2700 专机预置算法参数 ──────────────────────────────────
    int32_t m_peakMode3ThresholdMax        = kPeakMode3ThresholdMax;   // Flash 0x05B6: 低门限 (LTI)
    int32_t m_peakFilterReferenceThreshold = kPeakFilterReferenceThld; // Flash 0x05B4: 高门限 (HTI)

    // ── DAT_6BC4A910（噪声门限）的来源 ────────────────────────────────
    // 原版它是**一个全局量**，唯一写入者是 TouchThold_Process @0x6BA7CB1F 里的
    // TouchThold_GetNoiseThold @0x6BA7CA48；HardwareAnalyzer_GetRaw2RawPrpt
    // 只是读同一个全局。本级排在 TSAPrmt_Process 之前，读到的因此是**上一帧**
    // 的值（文档 02 §14「关键时序」）。
    // 早先这里维护了一份私有的 flash[0x90C..0x914] 常量与重算逻辑，与模块 02
    // 的 TouchThold 重复；现改为直接挂共享实例，消除双份。
    const TouchThold* m_sharedThold = nullptr;

    // ── HardwareAnalyzer_GetRaw2RawPrpt 的统计输出 ───────────────────
    struct Raw2RawStats {
        int32_t numOverNoise    = 0;  // |d| > noiseThold
        int32_t numOverLTI      = 0;  // |d| > peakMode3ThresholdMax
        int32_t numOverHTI      = 0;  // |d| > peakFilterReferenceThreshold
        int32_t numOverBigNoise = 0;  // |d| > peakFilterReferenceThreshold * 3
        int32_t sumPos          = 0;  // 过噪节点中正值之和
        int32_t sumNeg          = 0;  // 过噪节点中负值之和 (≤ 0)
        int16_t maxDelta        = 0;
        int16_t minDelta        = 0;
        uint8_t overNoisePct    = 0;  // 过噪节点占比 (%)，仅供诊断
    };

    // 硬件异常码，与原版 HardwareAnalyzer_SetException @0x6BAF00F6 的编号一致：
    // g_hwAnalyzer 是一个 32 位掩码，`g_hwAnalyzer |= 1 << code`。
    static constexpr uint32_t kExcRawUnstable = 1u << 2;   // code 2
    static constexpr uint32_t kExcBigNoise    = 1u << 29;  // code 0x1D

    void Reset() noexcept {
        m_prevExceptions = 0;
        m_repeatCount = {};
        m_hasPrevRaw = false;
        m_rawDidNotChange = 0;
        m_rawIsFrozen = false;
        m_hasPrevUnnormalizedRaw = false;
        m_exceptions = 0;
        m_stats = Raw2RawStats{};
        m_holdCount = 0;
        m_heldContacts.clear();
        m_hasHeld = false;
    }

    void SetEnabled(bool enabled) noexcept { m_enabled = enabled; }
    [[nodiscard]] bool IsEnabled() const noexcept { return m_enabled; }
    [[nodiscard]] const Raw2RawStats& Stats() const noexcept { return m_stats; }
    [[nodiscard]] uint32_t Exceptions() const noexcept { return m_exceptions; }
    /// DAT_6BC4A910 的当前值。没有挂共享实例时退化成 TouchThold_Init 的
    /// 上电初值 flash[0x5B6] = 600。
    [[nodiscard]] int32_t NoiseThold() const noexcept {
        return (m_sharedThold != nullptr) ? m_sharedThold->NoiseTholdPrevFrame()
                                          : static_cast<int32_t>(kPeakMode3ThresholdMax);
    }
    [[nodiscard]] uint32_t HoldCount() const noexcept { return m_holdCount; }

    // ══════════════════════════════════════════════════════════════════
    // B4：HardwareAnalyzer_GetDetectedException @0x6BAF0249（文档 08 §6）
    // ══════════════════════════════════════════════════════════════════
    // TSA_SetDMDType @0x6BA9D881 的唯一输入。返回**一个** 0..0x1F 的异常码
    // （0 = 无），不是掩码。三步：
    //   ① newMask = g_hwAnalyzer & ~DAT_6bc8c9c4（本帧新出现的异常位）；
    //   ② 按一张**写死的优先级序**逐位试探，命中第一个就停；
    //   ③ 对命中的 code 调 HardwareAnalyzer_CheckException @0x6BAF0182 做
    //      重复计数去抖：`cnt[code-1]++`，达到 g_numRepeatToReport[code-1]
    //      才清零并返回 code，否则返回 0（本次不报）。
    //
    // g_numRepeatToReport @0x6BB57A20 实测（0x1F 字节）：除下标 13
    // （= code 0x0E）为 2 外全是 1 ⇒ 除 0x0E 外都是**首次出现即上报**。
    //
    // 复刻侧目前只有 HardwareAnalyzer_ProcessByRaw2Raw 一条产出（code 2 与
    // code 0x1D），其余码位没有产出方，扫描自然扫不到 —— 这是覆盖面问题，
    // 不是编码问题。任务书说复刻用的是"自有位掩码编码"：反编译不支持，
    // m_exceptions 本来就是原厂的 `1 << code` 编码，缺的只是这一层取码 + 去抖。
    //
    // 副作用：命中时 `dwSpecialControlFlags |= 0x10`。复刻侧该标志字挂在
    // TouchBigFrame::specialControlFlags 上、由 Stage 5 每帧重建，Stage 0.5
    // 此刻还拿不到它；调用方若需要可自行 OR，这里只返回码值。
    [[nodiscard]] int32_t GetDetectedException() noexcept {
        const uint32_t newMask = m_exceptions & ~m_prevExceptions;
        for (const uint8_t code : kExceptionPriority) {
            if ((newMask & (1u << (code & 0x1Fu))) == 0) continue;
            return CheckException(code);
        }
        return 0;
    }
    /// TSA_RawIsFrozen：连续 10 帧 Raw 逐元素完全一致（§3）。
    /// 原版它只供日志查询，**不触发基线重置**。
    [[nodiscard]] bool RawIsFrozen() const noexcept { return m_rawIsFrozen; }

    // ── 核心入口 1: 针对 TouchBigFrame 处理 ──────────────────────────
    // 返回 true 表示本帧判为硬件异常帧，调用方应跳过 Stage 1~8 并保持上一帧输出。
    [[nodiscard]] bool Process(TouchBigFrame& bigFrame) noexcept {
        // B4：对位 HardwareAnalyzer_Process @0x6BAF1A37 开头的两句
        //   `DAT_6bc8c9c4 = g_hwAnalyzer; g_hwAnalyzer = 0;`
        // —— 先把上一帧的掩码存成"已知异常"，再清本帧掩码。
        // HardwareAnalyzer_GetDetectedNewException @0x6BAF013D 读的
        // `g_hwAnalyzer & ~DAT_6bc8c9c4` 就是本帧**新出现**的异常（上升沿）。
        m_prevExceptions = m_exceptions;
        m_exceptions = 0;
        m_stats = Raw2RawStats{};
        if (!m_enabled) {
            LatchOutput(bigFrame);
            return false;
        }

        // 首帧没有可比对的历史 raw，直接建立基准
        if (!m_hasPrevRaw) {
            StorePrevRaw(bigFrame);
            LatchOutput(bigFrame);
            return false;
        }

        // §3 TSA_RawCheckProcess @0x6BABCB0B（原版排在 raw 拷进 g_tsaBufRaw
        // 之前，用的是**未归一化**的线上缓冲；复刻里 Raw 只差一个恒定偏移，
        // 逐元素相等性判定不受影响）。
        TSA_RawCheckProcess(bigFrame);

        ComputeRaw2RawStats(bigFrame);
        EvaluateExceptions();

        // 原版 memcpy(g_tsaBufPreHWRaw, g_tsaBufRaw) 位于 HardwareAnalyzer_Process
        // 末尾，异常帧同样更新，因此这里无条件覆盖
        StorePrevRaw(bigFrame);

        const bool bypass = (m_exceptions & (kExcRawUnstable | kExcBigNoise)) != 0;
        if (bypass) {
            ++m_holdCount;
            RestoreHeldOutput(bigFrame);
        }
        return bypass;
    }

    // ── 核心入口 2: 针对 HeatmapFrame 处理 (兼容性管线入口) ────────────
    [[nodiscard]] bool Process(HeatmapFrame& frame) noexcept {
        if (frame.touch.bigFrame != nullptr) {
            return Process(*frame.touch.bigFrame);
        }
        return false;
    }

    // 非 bypass 帧在 Stage 8 之后调用，锁存本帧最终输出供下次保持使用
    void LatchOutput(const TouchBigFrame& bigFrame) noexcept {
        m_heldContacts.clear();
        for (const auto& c : bigFrame.contacts) {
            m_heldContacts.push_back(c);
        }
        m_heldHasLiveTouches = bigFrame.hasLiveTouches;
        m_heldActiveMask = bigFrame.activeTouchMask;
        m_hasHeld = true;
    }

    void registerBindings(Config::ConfigBinder& binder) { (void)binder; }
    void applyConfig(const Config::ConfigStore& store) { (void)store; }

private:
    // HardwareAnalyzer_GetDetectedException 里那串嵌套 if 的探测次序，逐条抄自
    // @0x6BAF0249。注意它**不是**升序：6/7 在最前，3 排在 0x13 之后。
    static constexpr uint8_t kExceptionPriority[] = {
        0x06, 0x07, 0x01, 0x02, 0x10, 0x0E, 0x09, 0x04, 0x0A, 0x05,
        0x0B, 0x08, 0x0C, 0x0D, 0x0F, 0x11, 0x12, 0x13, 0x03, 0x14,
        0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    };
    static constexpr int kExceptionCount = 0x1F;   // HARDWARE_EXCEPTION_LENGTH

    /// g_numRepeatToReport @0x6BB57A20 的实测内容（下标 = code − 1）。
    [[nodiscard]] static constexpr uint8_t NumRepeatToReport(uint8_t index) noexcept {
        return (index == 13) ? uint8_t{2} : uint8_t{1};   // 只有 code 0x0E 要连续两帧
    }

    /// HardwareAnalyzer_CheckException @0x6BAF0182。越界（code 0 或 > 0x1F）时
    /// 原版报断言并置软异常，这里按同样的语义返回 0。
    [[nodiscard]] int32_t CheckException(uint8_t code) noexcept {
        const uint8_t index = static_cast<uint8_t>(code - 1);
        if (index >= kExceptionCount) return 0;
        ++m_repeatCount[index];
        if (m_repeatCount[index] >= NumRepeatToReport(index)) {
            m_repeatCount[index] = 0;
            return static_cast<int32_t>(code);
        }
        return 0;
    }

    static constexpr int kRows = kSensorRows;
    static constexpr int kCols = kSensorCols;
    static constexpr int kNodeCount = kRows * kCols;

    // ── HardwareAnalyzer_GetRaw2RawPrpt ──────────────────────────────
    void ComputeRaw2RawStats(const TouchBigFrame& bigFrame) noexcept {
        const int32_t noise = NoiseThold();
        const int32_t lti   = m_peakMode3ThresholdMax;
        const int32_t hti   = m_peakFilterReferenceThreshold;
        const int32_t big   = m_peakFilterReferenceThreshold * 3;

        int16_t maxDelta = -0x8000;
        int16_t minDelta = 0x7FFF;

        for (int r = 0; r < kRows; ++r) {
            for (int c = 0; c < kCols; ++c) {
                const int32_t d = static_cast<int32_t>(bigFrame.raw[r][c]) -
                                  static_cast<int32_t>(m_prevRaw[r][c]);
                const int16_t d16 = static_cast<int16_t>(d);
                if (d16 > maxDelta) maxDelta = d16;
                if (d16 < minDelta) minDelta = d16;

                const int32_t mag = (d < 0) ? -d : d;
                if (mag <= noise) continue;

                ++m_stats.numOverNoise;
                if (d > 0) m_stats.sumPos += d;
                else       m_stats.sumNeg += d;

                // 三级门限是嵌套的：LTI ⊇ HTI ⊇ BigNoise
                if (mag <= lti) continue;
                ++m_stats.numOverLTI;
                if (mag <= hti) continue;
                ++m_stats.numOverHTI;
                if (mag <= big) continue;
                ++m_stats.numOverBigNoise;
            }
        }

        m_stats.maxDelta = maxDelta;
        m_stats.minDelta = minDelta;
        m_stats.overNoisePct = static_cast<uint8_t>((m_stats.numOverNoise * 100) / kNodeCount);
    }

    // ── HardwareAnalyzer_ProcessByRaw2Raw ────────────────────────────
    // W273AS2700 走 peak mode 3，因此「充电器已插入」的分叉恒取 mode 3 一侧；
    // RF 噪声（异常码 1）由 dwFeatureFlags2 bit4 门控，本机该位为 0，永不置位，
    // 故 IsToBypassCurrentFrame 里的 `异常2 且 非异常1` 退化成单纯的「异常 2」。
    void EvaluateExceptions() noexcept {
        if (m_stats.numOverBigNoise * 100 >= kNodeCount) {
            m_exceptions |= kExcBigNoise;
        }
        if (m_stats.numOverHTI > (kNodeCount >> 2)) {
            m_exceptions |= kExcRawUnstable;
        }
        if (m_stats.numOverLTI > (kNodeCount >> 1)) {
            m_exceptions |= kExcRawUnstable;
        }
        // 正负能量互相接近说明是噪声而非真实按压：|sumNeg|/4 < sumPos < 4·|sumNeg|
        if (m_stats.numOverNoise > (kNodeCount >> 1) &&
            m_stats.sumPos < m_stats.sumNeg * -4 &&
            -m_stats.sumNeg < m_stats.sumPos * 4) {
            m_exceptions |= kExcRawUnstable;
        }
    }

    // ── §3 TSA_RawCheckProcess @0x6BABCB0B → TSA_RawCheckFrozen @0x6BAFCA3C ──
    //   if (完全相同) { if (++g_rawDidNotChange >= 10) { frozen = 1; 计数回写 9; } }
    //   else          { g_rawDidNotChange = 0; frozen = 0; }
    // 计数器饱和在 9 ⇒ 一旦冻结就保持，直到出现任意一个不同元素。
    void TSA_RawCheckProcess(const TouchBigFrame& bigFrame) noexcept {
        if (!m_hasPrevUnnormalizedRaw) {
            std::memcpy(m_prevUnnormalizedRaw, bigFrame.raw, sizeof(m_prevUnnormalizedRaw));
            m_hasPrevUnnormalizedRaw = true;
            return;
        }
        const bool same = std::memcmp(m_prevUnnormalizedRaw, bigFrame.raw,
                                      sizeof(m_prevUnnormalizedRaw)) == 0;
        if (same) {
            ++m_rawDidNotChange;
        } else {
            m_rawDidNotChange = 0;
            m_rawIsFrozen = false;
        }
        if (m_rawDidNotChange > 9) {
            m_rawIsFrozen = true;
            m_rawDidNotChange = 9;
        }
        std::memcpy(m_prevUnnormalizedRaw, bigFrame.raw, sizeof(m_prevUnnormalizedRaw));
    }

    void StorePrevRaw(const TouchBigFrame& bigFrame) noexcept {
        std::memcpy(m_prevRaw, bigFrame.raw, sizeof(m_prevRaw));
        m_hasPrevRaw = true;
    }

    // ── Touch_KeepPrevTouchWithExceptionExcluded ─────────────────────
    // 原版是把上一帧的触点缓冲整体拷回当前帧后直接进入 TSA_MSProcessEnding，
    // 中间的信号、聚类、跟踪、上报全部不执行。复刻里 tracks 本就不被
    // ResetPerFrame 清空，因此只需还原被清掉的 contacts 与帧级状态位。
    void RestoreHeldOutput(TouchBigFrame& bigFrame) noexcept {
        bigFrame.contacts.clear();
        if (!m_hasHeld) {
            bigFrame.hasLiveTouches = false;
            bigFrame.activeTouchMask = 0;
            return;
        }
        for (const auto& c : m_heldContacts) {
            bigFrame.contacts.push_back(c);
        }
        bigFrame.hasLiveTouches = m_heldHasLiveTouches;
        bigFrame.activeTouchMask = m_heldActiveMask;
    }

    int16_t  m_prevRaw[kRows][kCols]{};          // g_tsaBufPreHWRaw
    bool     m_hasPrevRaw = false;
    int16_t  m_prevUnnormalizedRaw[kRows][kCols]{}; // g_tsaBufPreUnnormalizedRaw（§3）
    bool     m_hasPrevUnnormalizedRaw = false;
    uint32_t m_rawDidNotChange = 0;              // g_rawDidNotChange，饱和在 9
    bool     m_rawIsFrozen = false;              // g_rawIsFrozen
    uint32_t m_exceptions = 0;
    uint32_t m_prevExceptions = 0;               // DAT_6bc8c9c4，上一帧的 g_hwAnalyzer
    std::array<uint8_t, kExceptionCount> m_repeatCount{};  // g_cntRepeat @0x6BBA3530
    uint32_t m_holdCount = 0;
    Raw2RawStats m_stats{};

    FixedVector<TouchContact, kMaxContacts> m_heldContacts{};
    bool     m_heldHasLiveTouches = false;
    uint32_t m_heldActiveMask = 0;
    bool     m_hasHeld = false;
};

} // namespace Solvers::Touch
