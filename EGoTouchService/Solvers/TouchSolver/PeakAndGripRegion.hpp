#pragma once
// ── TouchPipeline Stage 3: PeakAndGripRegion ─────────────────────────
// Header-only C++23。对位原版 Peak_Process @0x6BA68C4B 的逐函数复刻。
// 权威规范：docs/finger_algorithms/03_peak_and_grip_region.md §1–§16。
//
//   §1  Peak_Process                    @0x6BA68C4B  顶层调度
//   §2  Peak_DetectInRange              @0x6BA685D7  8 邻域极大值扫描（不对称破平）
//   §3  PressureDrift_Detect            @0x6BB441C9  压力漂移带抑制
//   §4  Peak_Clean/Fill/CalcZn/Insert/Remove         峰数据层
//   §5  Peak_Z8Filter                   @0x6BA68502  孤立脉冲毛刺剔除
//   §6  Peak_IsFullIncellSDDetected     @0x6BA689C9  SD 迟滞判决器
//   §7  Peak_Z1Filter                   @0x6BA6848B  钳前门限二次回退
//   §8  Peak_SortInSigAscending         @0x6BA66013  升序选择排序 + 交织破平
//   §9  Peak_IDTracking                 @0x6BA68167  ID 预匹配
//   §10 TSA_MSPeakFilter                @0x6BA90DBD  本机空操作（gate 关）
//   §11 g_peaksBeforeSwitchDifBuf 快照            （模块 04 无条件消费）
//   §12 PrevPeak_Process                @0x6BA68F77  帧尾跨帧回灌
//   §13 GripFilter_RegionProcess        @0x6BAEC71C  握持等级掩码每帧重建
//
// 本级消费的全部门限都来自 Stage 2.5 TouchThold（bigFrame.tholds），对位原版
// TSAPrmt_Process 每帧覆写的 DAT_6bcab688 家族；`min(thold, 600/2)` 的**一次性**
// 钳位（§3 / §16-3）已在 TouchThold 里完成，本级只消费 detectThold / z1Thold。
//
// ⚠ 极性说明：复刻的输入是 Himax MCU 的偏移二进制已差分量（`wire ^ 0x8000`），
// 本底 ≈ 0、手指处为正峰，与 TSACore 的 `Bl − Raw` 同号。故 Peak 这一层的比较符、
// 门限符号与 maxSig/minSig 哨兵**逐字照抄原厂**，不做任何极性适配。
// ─────────────────────────────────────────────────────────────────────

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <algorithm>
#include <span>
#include <vector>

#include "SolverTypes.h"
#include "TouchCoreTypes.hpp"
#include "TouchFixed.hpp"
#include "TouchGates.hpp"
#include "TouchSharedTypes.h"

namespace Config {
class ConfigBinder;
class ConfigStore;
}

namespace Solvers::Touch {

// ── 握持掩码的值语义（§13 / §16-10）──────────────────────────────────
// g_tsaBufGripRegion 每格存的是**等级位** `1 << level`，不是几何来源位。
// GripFilter_GetGripLevel @0x6BAEA5A5 取最高置位的位序号（3→0）作为等级。
// 本机（直板 flash+0x8C4 = 2、屏幕旋转 nField_0x264 = 0）的最终形态：
//   col0 / col59            = 0x04（等级 2）
//   col1 / col58            = 0x02（等级 1）
//   row0 / row39            = 0x02（等级 1）
//   左下/右下角块 3 格       |= 0x08（等级 3）
//   col0 / col59 的行 39..30 |= 0x08（等级 3）
//   内部                     = 0x00
// bit0（值 1）在本机任何路径都不会被写入。
inline constexpr uint16_t kGripLevelBit1 = 1u << 1;  // 等级 1
inline constexpr uint16_t kGripLevelBit2 = 1u << 2;  // 等级 2
inline constexpr uint16_t kGripLevelBit3 = 1u << 3;  // 等级 3

/// GripFilter_GetGripLevel @0x6BAEA5A5：掩码中最高置位的位序号，无置位返回 0。
[[nodiscard]] constexpr uint8_t GripRegionLevel(uint16_t mask) noexcept {
    for (int k = 3; k >= 0; --k) {
        if (static_cast<int>(mask) >= (1 << k)) return static_cast<uint8_t>(k);
    }
    return 0;
}

class PeakAndGripRegion {
public:
    static constexpr int kRows = kSensorRows;        // 40 行 (Rx / Y轴)
    static constexpr int kCols = kSensorCols;        // 60 列 (Tx / X轴)
    static constexpr int kCellCount = kTotalCells;   // 2400 互容节点

    /// TSA_MAX_PEAK_NUM = 0x14。Peak_Insert 的硬上限，不是"建议容量"。
    static constexpr int kMaxPeakNum = kPeakMaxNum;

    // ── 模块使能开关 ──────────────────────────────────────────────────
    bool m_enabled = true;

    // ── 门控（全部取自 TouchGates 的单一事实源，§15）──────────────────
    // dwPeakProcessingMode = flash+0x34 = 3（FI 模式）
    uint32_t m_peakProcessingMode    = 3;
    // wPeakMode3ThresholdMax = flash+0x5B6 = 600。SD 的 2×/3× 分带、
    // PressureDrift 的 450/200 都由它派生。
    int16_t  m_peakMode3ThresholdMax = 600;
    // dwFeatureFlags2 bit5 = 1 ⇒ PressureDrift 开；bit0 = 0 ⇒ EdgePeakFilter 关。
    static constexpr uint32_t kFeatureFlags2 = Gates::kFeatureFlags2;
    // Peak_IsToExcludeSideRegion @0x6BA685AC 函数体是 `return 0` ⇒ 恒关。
    // 关掉它同时使 Peak_DetectInRange 里的边列专用门限分支永不生效。
    static constexpr bool kExcludeSideRegion = false;
    // Peak_needUDFPLargeFingerWorkaround @0x6BA68BC6 要求预置名 "P115" ⇒ 恒关，
    // 本机永远走 Peak_SortInSigAscending。
    static constexpr bool kUdfpLargeFingerWorkaround = false;

    /// g_pPrevTouches->bPrevTouchCount：上一帧触点数，PressureDrift_Detect
    /// 的第一个分支读它。由 TouchPipeline::LatchPrevTouchInputs 每帧写。
    uint8_t m_prevTouchCount = 0;

    // 峰条目 PeakEntry / 峰表 PeakTable / 历史表 PrevPeakTable 的结构定义在
    // TouchCoreTypes.hpp —— 它们是**跨阶段契约**（模块 04 读快照、模块 07 读
    // 历史表），不能留在本类内部。

    PeakAndGripRegion() { Reset(); }

    void Reset() noexcept {
        Peak_Clean();
        m_prevPeaks = PrevPeakTable{};
        m_snapshot = PeakTable{};
        m_legacyPeaks.clear();
        m_nextTrackId = 1;
        m_sdLatched = false;
        m_sdStrongCount = 0;
        m_sdStrongFrame = 0;
        m_frameStamp = 0;
        m_pressureDriftDebounce = 0;
        m_lastPressureDriftFrame = 0;
        m_prevTouchCount = 0;
        m_idTrackPrev.clear();
    }

    /// g_tsaStatic->bFullIncellSignalDisparity（+0x114）的当前锁存值。
    [[nodiscard]] bool IsFullIncellSignalDisparityLatched() const noexcept { return m_sdLatched; }

    void SetEnabled(bool enabled) noexcept { m_enabled = enabled; }
    [[nodiscard]] bool IsEnabled() const noexcept { return m_enabled; }

    [[nodiscard]] const PeakTable& Peaks() const noexcept { return m_peaks; }
    /// §11 g_peaksBeforeSwitchDifBuf @0x6BBC0B40：Peak_Process 之后立刻做的
    /// 0x1A0 字节快照。本机 TSA_MSPeakFilter 是空操作 ⇒ 内容与 m_peaks 一致，
    /// 但 TZ_BeforeSDCompensationPeakInfoRetrieval @0x6BA8ADEC **无条件**消费它，
    /// 所以这条通道必须存在。
    [[nodiscard]] const PeakTable& PeaksBeforeSwitchDif() const noexcept { return m_snapshot; }

    /// §12 跨帧历史表 g_prevPeaksPtr @0x6BB4F110（0xF4 = 4 + 20×12）。
    [[nodiscard]] const PrevPeakTable& PrevPeaks() const noexcept { return m_prevPeaks; }

    // ── 核心入口 1：直接在 TouchBigFrame 上原位运算 ───────────────────
    bool Process(TouchBigFrame& bigFrame) {
        if (!m_enabled) {
            bigFrame.peaks.clear();
            m_legacyPeaks.clear();
            return true;
        }

        // g_tsaStatic + 0xE0：帧计数。SD 冷却窗口（§6）与 PressureDrift 去抖
        // （§3）都按**帧**计，不是按时间。原版两处都从 0 开始递增。
        m_frameStamp = bigFrame.frameIndex;

        Peak_Process(bigFrame);

        // §11：Peak_Process 之后立刻 memcpy 0x1A0 字节快照，并发布到大帧供
        // 模块 04 的 TZ_BeforeSDCompensationPeakInfoRetrieval 消费。
        m_snapshot = m_peaks;
        bigFrame.peaksBeforeSwitchDif = m_snapshot;

        // §10 TSA_MSPeakFilter：dwFeatureFlags2 bit0 = 0 ⇒ 空操作。
        static_assert(!Gates::IsOn("EDGE_PEAK_WORKAROUND"),
                      "EdgePeakFilter_WorkAround 本机关闭，TSA_MSPeakFilter 是空操作（03 §10）");

        // §13：GripFilter_RegionProcess 在 Peak 之后、TZ_Process 之前。
        GripFilter_RegionProcess(bigFrame.gripRegion);

        PublishPeaks(bigFrame);

        // §12 PrevPeak_Process 原版在帧尾（TouchReport_Process 之后）执行，
        // 所以本帧的消费者读到的 g_prevPeaksPtr 是**上一帧**的历史表。
        // B2：Touch_SetVolatileFlagSigSum @0x6BA6F7AF（模块 05）拿它当跨帧
        // 能量比的分母，必须是上一帧的值 —— 先发布旧表、再刷新，顺序不能反。
        bigFrame.prevPeaks = m_prevPeaks;

        // 刷新给下一帧用。原版在帧尾；复刻侧 PeakEntry.flags 的唯一写入者
        // （TZ_GetPeakGripFlag，模块 04）尚未接通，此处与帧尾执行等价。
        PrevPeak_Process();

        return true;
    }

    // ── 核心入口 2：独立 HeatmapFrame（诊断 / 离线回放）───────────────
    bool Process(HeatmapFrame& frame) {
        if (frame.touch.bigFrame != nullptr) {
            const bool ok = Process(*frame.touch.bigFrame);
            frame.touch.runtime.peaks = GetLegacyPeaksSpan();
            frame.touch.runtime.peakThreshold = frame.touch.bigFrame->tholds.detectThold;
            return ok;
        }

        TouchBigFrame tempBigFrame;
        tempBigFrame.ResetPerFrame();
        tempBigFrame.timestampUs = frame.timestamp;
        for (int r = 0; r < kSensorRows; ++r) {
            for (int c = 0; c < kSensorCols; ++c) {
                tempBigFrame.diff[r][c] = frame.heatmapMatrix[r][c];
            }
        }

        const bool ok = Process(tempBigFrame);

        // §8 的等幅扰动会就地修改 Dif，回写才与原版一致。
        for (int r = 0; r < kSensorRows; ++r) {
            for (int c = 0; c < kSensorCols; ++c) {
                frame.heatmapMatrix[r][c] = tempBigFrame.diff[r][c];
            }
        }

        frame.touch.runtime.peaks = GetLegacyPeaksSpan();
        frame.touch.runtime.peakThreshold = tempBigFrame.tholds.detectThold;
        return ok;
    }

    void Detect(HeatmapFrame& frame, const std::vector<MacroZone>& zones = {}) {
        (void)zones;
        Process(frame);
    }

    [[nodiscard]] const std::vector<Touch::Peak>& GetPeaks() const noexcept { return m_legacyPeaks; }
    [[nodiscard]] std::span<const Touch::Peak> GetLegacyPeaksSpan() const noexcept {
        return {m_legacyPeaks.data(), m_legacyPeaks.size()};
    }

private:
    // ═════════════════════════════════════════════════════════════════
    // §1  Peak_Process @0x6BA68C4B
    // ═════════════════════════════════════════════════════════════════
    void Peak_Process(TouchBigFrame& bigFrame) {
        TouchTholdSet& tholds = bigFrame.tholds;

        // nOriginalPeakThreshold = DAT_6bcab688 的**钳前**值；钳位本身
        // （`min(thold, 600/2)`，只做一次、作用于全局门限变量）由 Stage 2.5
        // 完成，tholds.z1Thold 就是钳前值、tholds.detectThold 是钳后值。
        const int16_t thold0     = tholds.z1Thold;
        const int16_t detectThold = tholds.detectThold;

        // Peak_IsToExcludeSideRegion() 恒 0 ⇒ 扫描范围恒为整屏。
        const uint8_t startCol = kExcludeSideRegion ? 1 : 0;
        const uint8_t endCol   = static_cast<uint8_t>(kExcludeSideRegion ? kCols - 2 : kCols - 1);
        const uint8_t startRow = 0;
        const uint8_t endRow   = static_cast<uint8_t>(kRows - 1);

        Peak_DetectInRange(bigFrame.diff, detectThold, startRow, endRow, startCol, endCol);

        if (m_peakProcessingMode == 3) {
            Peak_Z8Filter();
            if (!Peak_IsFullIncellSDDetected(bigFrame)) {
                m_sdLatched = false;
                // 用未腰斩的原门限回扫，把 300~600 之间的推测式弱峰清掉，
                // 随后 DAT_6bcab688 恢复为 thold0。
                Peak_Z1Filter(thold0);
                tholds.peakThold = thold0;
            } else {
                m_sdLatched = true;
                // DAT_6bcab690 = (DAT_6bcab69c × DAT_6bcab688) >> 4 = (48 × 300) >> 4 = 900。
                tholds.downZThold = ScaleQ4(tholds.zTholdRatioQ4, detectThold);
                // ⚠ §1「易踩的坑」：这一支**不恢复** DAT_6bcab688。检出 SD 的那帧，
                // 帧尾 TZ_TraversalNeighbor 的 `DAT_6bcab688 * 3 < sig * 4` 用的是
                // 300 而不是 600，泛洪吸纳条件被放宽。必须保留该副作用。
                tholds.peakThold = detectThold;
            }
        }
        bigFrame.fullIncellSignalDisparity = m_sdLatched;

        // dwPeakProcessingMode == 2 分支：本机 mode = 3 ⇒ 永不进入（§1 表格）。

        // minPeakRow：无峰时写入 bRows（40），不是 0。
        uint8_t minRow = static_cast<uint8_t>(kRows);
        for (int i = 0; i < m_peaks.count; ++i) {
            if (m_peaks.e[i].r < minRow) minRow = m_peaks.e[i].r;
        }
        m_peaks.minPeakRow = minRow;

        if constexpr (!kUdfpLargeFingerWorkaround) {
            Peak_SortInSigAscending(bigFrame.diff);
        } else {
            Peak_SortInSigDescending(bigFrame.diff);
        }
        Peak_IDTracking();
    }

    // ═════════════════════════════════════════════════════════════════
    // §2  Peak_DetectInRange @0x6BA685D7
    //
    // 扫描顺序：行 startRow→endRow 外层、列 startCol→endCol 内层，线性索引
    // 递增。破平依赖此顺序（§16-1），不可换列优先或并行归约。
    // 比较符（§16-2）：右下/正下/左下/正右用 `<=`，正左/右上/正上/左上用 `<`；
    // 门限用 `>=`。越界由扫描范围边界布尔短路，**不是**网格边界（§16-9）。
    // ═════════════════════════════════════════════════════════════════
    void Peak_DetectInRange(const int16_t diff[kRows][kCols],
                            int16_t thold,
                            uint8_t startRow, uint8_t endRow,
                            uint8_t startCol, uint8_t endCol) {
        Peak_Clean();

        // specialEdge = Peak_IsToExcludeSideRegion() && TouchThold_ToeUseSpecialThold()
        // 两者本机都为 0 ⇒ 边列专用门限 DAT_6bcab68a 分支永不生效（§2）。
        constexpr bool kSpecialEdge = Gates::IsOn("PEAK_SIDE_SPECIAL_THOLD");
        static_assert(!kSpecialEdge, "本机中心区与边列使用同一门限，不存在分区门限（03 §2）");

        for (uint8_t r = startRow; r <= endRow; ++r) {
            for (uint8_t c = startCol; c <= endCol; ++c) {
                const int16_t v = diff[r][c];
                if (v < thold) continue;              // 原版：`thold <= v` 才继续

                const bool atRight  = (c >= endCol);
                const bool atBottom = (r >= endRow);
                const bool atLeft   = (c <= startCol);
                const bool atTop    = (r <= startRow);

                // 下 / 右四邻：非严格（`<= v` 才继续）
                if (!atBottom && !atRight && diff[r + 1][c + 1] > v) continue;
                if (!atBottom && diff[r + 1][c] > v) continue;
                if (!atBottom && !atLeft && diff[r + 1][c - 1] > v) continue;
                if (!atRight && diff[r][c + 1] > v) continue;
                // 左 / 上四邻：严格（`< v` 才继续）
                if (!atLeft && diff[r][c - 1] >= v) continue;
                if (!atTop && !atRight && diff[r - 1][c + 1] >= v) continue;
                if (!atTop && diff[r - 1][c] >= v) continue;
                if (!atTop && !atLeft && diff[r - 1][c - 1] >= v) continue;

                // §3 dwFeatureFlags2 & 0x20（实测 0x120 ⇒ 开）
                if ((kFeatureFlags2 & 0x20u) != 0 && PressureDrift_Detect(diff, c, r)) continue;

                Peak_Insert(diff, c, r);
            }
        }
    }

    // ═════════════════════════════════════════════════════════════════
    // §3  PressureDrift_Detect @0x6BB441C9
    //
    // 识别"整行被均匀抬起"的假象（大面积压迫导致的基线漂移带）。返回 true
    // 则该候选峰被丢弃。450/225 幅值窗、行梯度 ≤200、行正和 ≥4.5·v、
    // 梯度和 ≤6·v、25 帧/50 次去抖。
    // ═════════════════════════════════════════════════════════════════
    [[nodiscard]] bool PressureDrift_Detect(const int16_t diff[kRows][kCols],
                                            uint8_t col, uint8_t row) {
        const int16_t v = diff[row][col];
        // Th = (600 * 3) >> 2 = 450，负数向零取整
        const int16_t th = static_cast<int16_t>(
            Fixed::ShrTowardZero(static_cast<int32_t>(m_peakMode3ThresholdMax) * 3, 2));

        if (m_prevTouchCount < 3) {
            if (!PressureDrift_DetectByTimeStamp()) return false;
        } else {
            m_pressureDriftDebounce = 0;
        }

        // 幅值窗：v 必须落在 [Th/2, Th] = [225, 450]
        if (v > th || v < static_cast<int16_t>(th / 2)) return false;

        int32_t gradSum = 0;
        int32_t posSum  = 0;
        for (int c = 1; c < kCols - 1; ++c) {
            const int32_t g = PressureDrift_GradientCalUnit(diff, static_cast<uint8_t>(c), row);
            if (g > static_cast<int32_t>(m_peakMode3ThresholdMax) / 3) return false;  // > 200
            gradSum += g;
            if (diff[row][c] > 0) posSum += diff[row][c];
        }

        if (posSum < (static_cast<int32_t>(v) * 9) / 2 || static_cast<int32_t>(v) * 6 < gradSum) {
            return false;
        }
        m_lastPressureDriftFrame = m_frameStamp;
        return true;
    }

    /// PressureDrift_DetectByTimeStamp @0x6BB44172
    [[nodiscard]] bool PressureDrift_DetectByTimeStamp() noexcept {
        if (static_cast<uint64_t>(m_frameStamp - m_lastPressureDriftFrame) < 25u) {
            ++m_pressureDriftDebounce;
            return m_pressureDriftDebounce < 51;
        }
        return false;
    }

    /// PressureDrift_GradientCalUnit @0x6BB440D0：卷积核 [-1, 0, +1] 取绝对值。
    /// c ∈ [1, bCols-2] 保证不越行。
    [[nodiscard]] static int32_t PressureDrift_GradientCalUnit(
        const int16_t diff[kRows][kCols], uint8_t c, uint8_t row) noexcept {
        const int32_t g = static_cast<int32_t>(diff[row][c + 1]) - static_cast<int32_t>(diff[row][c - 1]);
        return g < 0 ? -g : g;
    }

    // ═════════════════════════════════════════════════════════════════
    // §4  峰数据层
    // ═════════════════════════════════════════════════════════════════

    /// Peak_Clean @0x6BA659B0：整块 0x1A0 清零后写入三个哨兵（§16-4）。
    /// 每次 Peak_DetectInRange 开头调用 ⇒ PeakEntry.flags 每帧全新。
    void Peak_Clean() noexcept {
        m_peaks = PeakTable{};
        m_peaks.maxSig = INT16_MIN;   // 0x8000
        m_peaks.minSig = INT16_MAX;   // 0x7FFF
        m_peaks.maxZn  = INT32_MIN;   // 0x80000000
    }

    /// Peak_Fill @0x6BA65CB8：写 sig/col/row → CalcZn → 单调刷新三个头部统计量。
    void Peak_Fill(const int16_t diff[kRows][kCols], int slot, uint8_t col, uint8_t row) noexcept {
        PeakEntry& e = m_peaks.e[slot];
        e.sig = diff[row][col];
        e.c = col;
        e.r = row;
        Peak_CalcZn(diff, slot);
        if (m_peaks.maxSig < e.sig) m_peaks.maxSig = e.sig;
        if (e.sig < m_peaks.minSig) m_peaks.minSig = e.sig;
        if (m_peaks.maxZn < e.zn) m_peaks.maxZn = e.zn;
    }

    /// Peak_CalcZn @0x6BA67C89：以**网格边界**（不是扫描范围）裁剪邻居，
    /// 一次遍历同时算出 z8 / nbrCnt / zn。
    ///   zn = max(TL, TR, BL, BR, plus)，四个象限和都含中心，plus 是十字五点和。
    ///   SideTouch 通道（dwProcessingState & 0x10）下只取 plus 且不含左右邻，
    ///   主管线走 MS 路径，不涉及。
    void Peak_CalcZn(const int16_t diff[kRows][kCols], int slot) noexcept {
        PeakEntry& e = m_peaks.e[slot];
        const int c = e.c;
        const int r = e.r;
        const int32_t center = diff[r][c];

        const bool hasUp    = (r != 0);
        const bool hasDown  = (r < kRows - 1);
        const bool hasLeft  = (c != 0);
        const bool hasRight = (c < kCols - 1);

        int32_t tl = center, tr = center, bl = center, br = center, plus = center;
        int32_t z8 = 0;
        uint8_t n = 0;

        if (hasDown) {
            if (hasRight) { const int32_t d = diff[r + 1][c + 1]; br += d; z8 += d; }
            const int32_t d = diff[r + 1][c];
            bl += d; br += d; z8 += d; plus += d;
            n = static_cast<uint8_t>((hasRight ? 1 : 0) + 1);
            if (hasLeft) { const int32_t dl = diff[r + 1][c - 1]; bl += dl; z8 += dl;
                           n = static_cast<uint8_t>((hasRight ? 1 : 0) + 2); }
        }
        if (hasRight) {
            const int32_t d = diff[r][c + 1];
            tr += d; br += d; z8 += d; plus += d; ++n;
        }
        if (hasLeft) {
            const int32_t d = diff[r][c - 1];
            tl += d; bl += d; z8 += d; plus += d; ++n;
        }
        if (hasUp) {
            if (hasRight) { const int32_t d = diff[r - 1][c + 1]; tr += d; z8 += d; ++n; }
            const int32_t d = diff[r - 1][c];
            tl += d; tr += d; z8 += d; plus += d;
            const uint8_t base = n;
            n = static_cast<uint8_t>(base + 1);
            if (hasLeft) { const int32_t dl = diff[r - 1][c - 1]; tl += dl; z8 += dl;
                           n = static_cast<uint8_t>(base + 2); }
        }

        int32_t zn = std::max({tl, tr, bl, br});
        if (zn <= plus) zn = plus;

        e.zn = zn;
        e.z8 = z8;
        e.nbrCnt = n;
    }

    /// Peak_Insert @0x6BA65E68。溢出（§6 / §16-8）：打印 "Peak Buffer Overflow!!!"
    /// 后用**最弱峰**替换，要求严格 `<`；被替换的峰直接消失，`innerCount`
    /// **不回滚**，头部 minSig 也不重算（只做单调刷新）。原厂缺陷照抄。
    int Peak_Insert(const int16_t diff[kRows][kCols], uint8_t col, uint8_t row) noexcept {
        if (m_peaks.count < kMaxPeakNum) {
            const int slot = m_peaks.count;
            Peak_Fill(diff, slot, col, row);
            if (!Peak_IsOnOuterEdge(slot)) ++m_peaks.innerCount;
            ++m_peaks.count;
            return slot;
        }
        int minIdx = 0;
        for (int i = 1; i < kMaxPeakNum; ++i) {
            if (m_peaks.e[i].sig < m_peaks.e[minIdx].sig) minIdx = i;
        }
        if (m_peaks.e[minIdx].sig < diff[row][col]) {
            Peak_Fill(diff, minIdx, col, row);
            return minIdx;
        }
        return kMaxPeakNum;   // 新峰被丢弃
    }

    /// Peak_IsOnOuterEdge @0x6BA66A9D
    [[nodiscard]] bool Peak_IsOnOuterEdge(int slot) const noexcept {
        const PeakEntry& e = m_peaks.e[slot];
        return e.c == 0 || e.c == kCols - 1 || e.r == 0 || e.r == kRows - 1;
    }

    /// Peak_Remove @0x6BA65B60：**前移式**删除，不修补头部统计量。
    /// 所有滤波器都必须倒序遍历以配合它（§16-8）。
    void Peak_Remove(int idx) noexcept {
        if (m_peaks.count > kMaxPeakNum) m_peaks.count = kMaxPeakNum;  // 原版断言后钳位
        if (idx >= m_peaks.count) return;
        for (int i = idx; i < m_peaks.count - 1; ++i) {
            m_peaks.e[i] = m_peaks.e[i + 1];
        }
        --m_peaks.count;
    }

    // ═════════════════════════════════════════════════════════════════
    // §5  Peak_Z8Filter @0x6BA68502：Z8 < Z1/32（向零取整）⇒ 剔除。
    //     只杀极端孤立尖峰（CMF/IIR 后邻域负向振铃），不参与强弱峰取舍。
    // ═════════════════════════════════════════════════════════════════
    void Peak_Z8Filter() noexcept {
        for (int i = m_peaks.count - 1; i >= 0; --i) {
            int16_t s = m_peaks.e[i].sig;
            if (s < 0) s = static_cast<int16_t>(s + 0x1F);   // 负数向零取整补偿
            if (m_peaks.e[i].z8 < static_cast<int32_t>(s >> 5)) Peak_Remove(i);
        }
    }

    // ═════════════════════════════════════════════════════════════════
    // §7  Peak_Z1Filter @0x6BA6848B：sig < cutoff ⇒ 剔除。倒序。
    // ═════════════════════════════════════════════════════════════════
    void Peak_Z1Filter(int16_t cutoff) noexcept {
        for (int i = m_peaks.count - 1; i >= 0; --i) {
            if (m_peaks.e[i].sig < cutoff) Peak_Remove(i);
        }
    }

    // ═════════════════════════════════════════════════════════════════
    // §6  Peak_IsFullIncellSDDetected @0x6BA689C9
    //
    // 本模块唯一的跨帧状态机；输出 bFullIncellSignalDisparity 被模块 02 的
    // TouchThold 在**下一帧**读取，使峰门限在 600 与 300 之间切换。
    //   · count == 0 ⇒ 直接返回 0，**不触碰**锁存与冷却计数（§16-6）；
    //   · result 初值 = 上一帧锁存值；
    //   · maxSig > 1800 ⇒ field_0xA0 累加（封顶 6），满后每帧刷新 field_0x78；
    //   · frameStamp − field_0x78 < 1001 ⇒ 强制 0。field_0x78 初值 0 使
    //     **开机后前 1001 帧 SD 恒为 0**（§16-5）；
    //   · maxSig ∈ [1200, 1800] 两个分支都不进 ⇒ 沿用上一帧（有意的迟滞带）。
    // maxSig 取头部 +0x02，是 Z8 滤波**之前**插入时刷新的值；count 是 Z8
    // 滤波**之后**的存活数 —— 两者时点不同，Peak_Remove 不回退 maxSig。
    // ═════════════════════════════════════════════════════════════════
    [[nodiscard]] bool Peak_IsFullIncellSDDetected(const TouchBigFrame& bigFrame) {
        if (m_peaks.count == 0) return false;

        bool result = m_sdLatched;

        const int32_t maxSig = m_peaks.maxSig;
        const int32_t band3  = static_cast<int32_t>(m_peakMode3ThresholdMax) * 3;  // 1800
        const int32_t band2  = static_cast<int32_t>(m_peakMode3ThresholdMax) * 2;  // 1200

        if (maxSig > band3) {
            if (m_sdStrongCount < 6) ++m_sdStrongCount;
            else m_sdStrongFrame = m_frameStamp;
        } else {
            m_sdStrongCount = 0;
        }

        if (static_cast<uint64_t>(m_frameStamp - m_sdStrongFrame) < 1001u) {
            return false;
        }
        if (maxSig < band2) {
            // g_tsaPrpt + 0xC8 = TSA_CombineBlock(…, 5) 的 posSum + negSum
            const int32_t innerSum = ScanInnerSignalSum(bigFrame.diff, bigFrame.tholds.noiseThold);
            if (innerSum > static_cast<int32_t>(bigFrame.tholds.peakFilterRef) * 2) {  // > 6000
                for (int i = 0; i < m_peaks.count; ++i) {
                    // 判据 A：存活峰数 > 4（原版写在循环里，与循环变量无关）
                    if (m_peaks.count > 4) result = true;
                    // 判据 B：该峰能量摊平（Z1 < Z8/5）且幅值 ≥ 主峰的 75%
                    if (Peak_IsFullIncellInsideSensor(i) &&
                        maxSig * 3 < static_cast<int32_t>(m_peaks.e[i].sig) * 4) {
                        result = true;
                    }
                }
            } else {
                result = false;
            }
        }
        return result;
    }

    /// Peak_IsFullIncellInsideSensor @0x6BA68944：`Z1 < Z8 / 5`。
    /// +0x0C 存的是 Z8（不含中心）；函数名里的 InsideSensor 指"能量摊在感应面内"，
    /// 与位置无关。**不是** `4·Z1 < Z8`。
    [[nodiscard]] bool Peak_IsFullIncellInsideSensor(int i) const noexcept {
        return static_cast<int32_t>(m_peaks.e[i].sig) < m_peaks.e[i].z8 / 5;
    }

    /// g_tsaPrpt + 0xC8 的等价求值。TSA_CombineBlock(0x6BC4A700, 5) = 块 0 ∪ 块 2
    /// = 行 1..R-2 × 列 1..C-2，取 posSum + negSum。
    /// ⚠ TSA_GetDifPrptCommon 的分桶判据是 `d >= noiseThold` → posSum、
    /// `d <= -noiseThold` → negSum，`|d| < noiseThold` 的节点**两桶都不计**。
    /// noiseThold（DAT_6BC4A910）稳态 125 / 187 / 250，首帧 600（文档 02 §14）。
    [[nodiscard]] static int32_t ScanInnerSignalSum(const int16_t diff[kRows][kCols],
                                                    int16_t noiseThold) noexcept {
        int32_t sum = 0;
        for (int r = 1; r <= kRows - 2; ++r) {
            for (int c = 1; c <= kCols - 2; ++c) {
                const int16_t d = diff[r][c];
                if (d >= noiseThold || d <= static_cast<int16_t>(-noiseThold)) sum += d;
            }
        }
        return sum;
    }

    // ═════════════════════════════════════════════════════════════════
    // §8  Peak_SortInSigAscending @0x6BA66013
    //
    // 选择式双重循环，按 sig **升序**（弱峰在前，先泛洪）。排序与破平**交织**：
    // 每遇到一对等幅峰，把靠后的那个 sig 抬 1，并**同步把 g_tsaBufDif 上对应
    // 节点也抬 1**（§16-7）。后级 TZ 泛洪与质心解算读到的就是被扰动过的值。
    // 注意 e.z8 / e.zn 不随之更新，这是原厂行为。
    // ═════════════════════════════════════════════════════════════════
    void Peak_SortInSigAscending(int16_t diff[kRows][kCols]) noexcept {
        for (int i = 0; i < m_peaks.count - 1; ++i) {
            for (int j = i + 1; j < m_peaks.count; ++j) {
                if (m_peaks.e[j].sig < m_peaks.e[i].sig) {
                    std::swap(m_peaks.e[i], m_peaks.e[j]);
                } else if (m_peaks.e[i].sig == m_peaks.e[j].sig) {
                    m_peaks.e[j].sig = Fixed::Wrap16(static_cast<int32_t>(m_peaks.e[j].sig) + 1);
                    int16_t& cell = diff[m_peaks.e[j].r][m_peaks.e[j].c];
                    cell = Fixed::Wrap16(static_cast<int32_t>(cell) + 1);
                }
            }
        }
    }

    /// Peak_SortInSigDescending @0x6BA66412：本机不会被调用（§1 表格）。
    void Peak_SortInSigDescending(int16_t diff[kRows][kCols]) noexcept {
        for (int i = 0; i < m_peaks.count - 1; ++i) {
            for (int j = i + 1; j < m_peaks.count; ++j) {
                if (m_peaks.e[j].sig > m_peaks.e[i].sig) {
                    std::swap(m_peaks.e[i], m_peaks.e[j]);
                } else if (m_peaks.e[i].sig == m_peaks.e[j].sig) {
                    m_peaks.e[j].sig = Fixed::Wrap16(static_cast<int32_t>(m_peaks.e[j].sig) - 1);
                    int16_t& cell = diff[m_peaks.e[j].r][m_peaks.e[j].c];
                    cell = Fixed::Wrap16(static_cast<int32_t>(cell) - 1);
                }
            }
        }
    }

    // ═════════════════════════════════════════════════════════════════
    // §9  Peak_IDTracking @0x6BA68167
    //
    // 原版把每个峰的格点中心 (col·256+128, row·256+128) 当成临时触点灌进
    // g_touchesPtr，调 IDT_Process(1) 做一次**峰级预匹配**，再把分配到的
    // Track ID 回写到 peak+0x04。峰**没有** age / tzAge 字段，ID 每帧重新分配。
    //
    // 复刻里 IDT 的完整实现在 Stage 5 TrackingAndEvents 上，且它关联的是区域
    // 重心而不是峰坐标（对位原版 TZ_Process 之后那次 IDT_Process(0)）。要把
    // 本函数改接到 Stage 5，需要把 Stage 5 的槽位表反向暴露给 Stage 3 并在一帧
    // 内跑两遍匹配。此处保留一个等价的峰级最近邻预匹配，产出的 peak.id 只被
    // 诊断通道与 TouchZoneAndCentroid 的区域-峰关联消费。
    // ═════════════════════════════════════════════════════════════════
    void Peak_IDTracking() {
        std::vector<bool> matched(m_idTrackPrev.size(), false);
        for (int i = 0; i < m_peaks.count; ++i) {
            PeakEntry& e = m_peaks.e[i];
            int bestDistSq = 4 * 4;
            int bestIdx = -1;
            for (size_t p = 0; p < m_idTrackPrev.size(); ++p) {
                if (matched[p]) continue;
                const int dr = static_cast<int>(e.r) - static_cast<int>(m_idTrackPrev[p].r);
                const int dc = static_cast<int>(e.c) - static_cast<int>(m_idTrackPrev[p].c);
                const int d2 = dr * dr + dc * dc;
                if (d2 < bestDistSq) { bestDistSq = d2; bestIdx = static_cast<int>(p); }
            }
            if (bestIdx >= 0) {
                matched[static_cast<size_t>(bestIdx)] = true;
                e.id = m_idTrackPrev[static_cast<size_t>(bestIdx)].id;
            } else {
                e.id = AllocateNextTrackId();
            }
        }
        m_idTrackPrev.assign(m_peaks.e, m_peaks.e + m_peaks.count);
    }

    // ═════════════════════════════════════════════════════════════════
    // §12 PrevPeak_Process @0x6BA68F77
    //
    // 把本帧峰列表压缩成 12 字节/条目的历史表。**Z8 与邻居数都不回灌**，
    // 所以任何跨帧判据都只能用 Zn 和 Z1。峰 ID 也不回灌。
    // ═════════════════════════════════════════════════════════════════
    void PrevPeak_Process() noexcept {
        m_prevPeaks.count  = m_peaks.count;
        m_prevPeaks.maxSig = m_peaks.maxSig;
        for (int i = 0; i < m_peaks.count; ++i) {
            m_prevPeaks.e[i].flags = m_peaks.e[i].flags;
            m_prevPeaks.e[i].zn    = m_peaks.e[i].zn;
            m_prevPeaks.e[i].sig   = m_peaks.e[i].sig;
            m_prevPeaks.e[i].c     = m_peaks.e[i].c;
            m_prevPeaks.e[i].r     = m_peaks.e[i].r;
        }
    }

    // ═════════════════════════════════════════════════════════════════
    // §13 GripFilter_RegionProcess @0x6BAEC71C
    //
    // **每帧完全重建**（先清 2400 字节再 OR 涂覆），无跨帧累积（§16-10）。
    // flash+0x8C4 = 2（直板）⇒ 走 GripFilter_RegionSetByScreenRotation；
    // g_tsaStatic->nField_0x264 = 0（屏幕未旋转）⇒ 底部分支。
    // ═════════════════════════════════════════════════════════════════
    void GripFilter_RegionProcess(uint16_t region[kRows][kCols]) const noexcept {
        // GripFilter_RegionReset @0x6BAE9E90
        for (int r = 0; r < kRows; ++r) {
            for (int c = 0; c < kCols; ++c) region[r][c] = 0;
        }

        static_assert(!Gates::IsOn("GRIP_PAD_MODE"),
                      "flash+0x8C4 = 2 ⇒ 走通用旋转掩码，不是 Pad / Fold（03 §13）");

        // GripFilter_RegionSetByScreenRotation @0x6BAEBDE9，nField_0x264 == 0：
        RegionSet(region, 0x00C, 2, 0);   // col 0 与 col 59 全行 |= 0x04
        RegionSet(region, 0x00C, 1, 1);   // col 1 与 col 58 全行 |= 0x02
        RegionSet(region, 0x040, 3, 0);   // 左下角 3 格 |= 0x08
        RegionSet(region, 0x080, 3, 0);   // 右下角 3 格 |= 0x08
        RegionSet(region, 0x400, 3, 0);   // col 0  的行 39..30 |= 0x08
        RegionSet(region, 0x800, 3, 0);   // col 59 的行 39..30 |= 0x08
        RegionSet(region, 0x003, 1, 0);   // row 0 与 row 39 全列 |= 0x02

        // dwFeatureFlags & 0x8000（掌压扩展区）：实测 0x01090121 ⇒ bit15 = 0，关。
        static_assert((Gates::kFeatureFlags & 0x8000u) == 0,
                      "bit15 = 0 ⇒ TypingScenario 掌压扩展关闭（03 §13）");

        // GripFilter_RegionSetByHandGesture @0x6BAEC611：
        // g_tsaBufTzPrevHand[i] != 0 ⇒ region[i] |= 0x08。它是本掩码**唯一**的
        // 跨帧来源，由 HandGesture_PostProcess 在上一帧写。复刻侧 HandGesture
        // 子管线未实现（DataSwitch_ToHand 通道不在手指主路径上），该层恒为全 0
        // —— 与原版"无手掌握持时全 0"一致。
    }

    /// GripFilter_RegionSet @0x6BAEBAD8 + GripFilter_RegionSetPalm @0x6BAEB512。
    /// 写入值恒为 `1 << level`（level 是等级，不是固定语义位），全部 `|=`。
    static void RegionSet(uint16_t region[kRows][kCols],
                          uint32_t mask, int level, int margin) noexcept {
        const uint16_t bit = static_cast<uint16_t>(1u << level);

        if (mask & 0x01) {                                      // 第 margin 行，全列
            for (int c = 0; c < kCols; ++c) region[margin][c] |= bit;
        }
        if (mask & 0x02) {                                      // 第 bRows-1-margin 行
            const int r = kRows - 1 - margin;
            for (int c = 0; c < kCols; ++c) region[r][c] |= bit;
        }
        if (mask & 0x04) {                                      // 第 margin 列，全行
            for (int r = 0; r < kRows; ++r) region[r][margin] |= bit;
        }
        if (mask & 0x08) {                                      // 第 bCols-1-margin 列
            const int c = kCols - 1 - margin;
            for (int r = 0; r < kRows; ++r) region[r][c] |= bit;
        }
        // 四个角块各 3 格：角点 + 同行相邻 + 同列相邻
        if (mask & 0x10) { region[0][0] |= bit; region[0][1] |= bit; region[1][0] |= bit; }
        if (mask & 0x20) { region[0][kCols - 1] |= bit; region[0][kCols - 2] |= bit;
                           region[1][kCols - 1] |= bit; }
        if (mask & 0x40) { region[kRows - 1][0] |= bit; region[kRows - 1][1] |= bit;
                           region[kRows - 2][0] |= bit; }
        if (mask & 0x80) { region[kRows - 1][kCols - 1] |= bit; region[kRows - 1][kCols - 2] |= bit;
                           region[kRows - 2][kCols - 1] |= bit; }

        // ── GripFilter_RegionSetPalm：函数尾部无条件调用，处理高 8 位 ──
        if (mask & 0x0100) { for (int r = 0; r < 10; ++r) region[r][margin] |= bit; }
        if (mask & 0x0200) { const int c = kCols - 1 - margin;
                             for (int r = 0; r < 10; ++r) region[r][c] |= bit; }
        if (mask & 0x0400) { for (int i = 0; i < 10; ++i) region[kRows - 1 - i][margin] |= bit; }
        if (mask & 0x0800) { const int c = kCols - 1 - margin;
                             for (int i = 0; i < 10; ++i) region[kRows - 1 - i][c] |= bit; }
        if (mask & 0x1000) { for (int c = 0; c < 10; ++c) region[margin][c] |= bit; }
        if (mask & 0x4000) { const int r = kRows - 1 - margin;
                             for (int c = 0; c < 10; ++c) region[r][c] |= bit; }
        if (mask & 0x2000) { for (int i = 0; i < 10; ++i) region[margin][kCols - 1 - i] |= bit; }
        if (mask & 0x8000) { const int r = kRows - 1 - margin;
                             for (int i = 0; i < 10; ++i) region[r][kCols - 1 - i] |= bit; }
    }

    // ── 发布到共享大帧 ────────────────────────────────────────────────
    void PublishPeaks(TouchBigFrame& bigFrame) {
        bigFrame.peaks.clear();
        m_legacyPeaks.clear();
        for (int i = 0; i < m_peaks.count; ++i) {
            const PeakEntry& e = m_peaks.e[i];
            PeakCandidate pc;
            pc.r = e.r;
            pc.c = e.c;
            pc.z = e.sig;
            pc.id = e.id;
            pc.age = 0;              // 峰没有 age 字段（§9），保留以兼容诊断结构
            pc.nZ8 = e.z8;
            pc.nZ4 = e.zn;
            pc.isEdge = Peak_IsOnOuterEdge(i);
            pc.isSuppressed = false;
            bigFrame.peaks.push_back(pc);

            Touch::Peak lp;
            lp.r = e.r;
            lp.c = e.c;
            lp.z = e.sig;
            lp.id = e.id;
            lp.tzAge = 0;
            lp.neighborSignalSum = e.z8;
            m_legacyPeaks.push_back(lp);
        }
    }

    // ── (ratio × value) >> 4，带原版的负数向零修正 ────────────────────
    static int32_t ScaleQ4(int32_t ratio, int32_t value) noexcept {
        return Fixed::ShrTowardZero(ratio * value, 4);
    }

    uint8_t AllocateNextTrackId() noexcept {
        const uint8_t id = m_nextTrackId++;
        if (m_nextTrackId > 32) m_nextTrackId = 1;
        return id;
    }

    // ── 状态 ──────────────────────────────────────────────────────────
    PeakTable     m_peaks{};        // g_pPeaks
    PeakTable     m_snapshot{};     // g_peaksBeforeSwitchDifBuf @0x6BBC0B40（§11）
    PrevPeakTable m_prevPeaks{};    // g_prevPeaksPtr @0x6BB4F110（§12）

    // Peak_IsFullIncellSDDetected 的三份跨帧记忆（§6）
    bool     m_sdLatched     = false;  // g_tsaStatic + 0x114
    uint8_t  m_sdStrongCount = 0;      // g_tsaStatic + 0xA0，封顶 6
    uint64_t m_sdStrongFrame = 0;      // g_tsaStatic + 0x78，初值 0 ⇒ 前 1001 帧恒 0

    // PressureDrift 的两份跨帧记忆（§3）
    uint32_t m_pressureDriftDebounce  = 0;   // g_pressureDriftDebounce
    uint64_t m_lastPressureDriftFrame = 0;   // g_lastPressureDriftDetectedTimeStamp

    uint64_t m_frameStamp = 0;         // g_tsaStatic + 0xE0

    uint8_t  m_nextTrackId = 1;
    std::vector<PeakEntry>   m_idTrackPrev;
    std::vector<Touch::Peak> m_legacyPeaks;
};

} // namespace Solvers::Touch
