#pragma once
// ══════════════════════════════════════════════════════════════════════
// TouchPipeline Stage 2: CmfAndGridFilter
//
// 对位 TSACore `TSA_MSProcess @0x6BA90E43` 中 BLSM 之后、TSAPrmt_Process
// 之前的一整段（文档 `docs/finger_algorithms/02_cmf_and_grid_filter.md`）：
//
//   TSAPrpt_GetDifPreCMFPrpt @0x6BAA13BE   pre-CMF 差分极值与超噪点数（§11）
//   CMF_Process              @0x6BA4F2C8   §2
//     ├ memcpy → g_tsaBufPreCMFDif
//     ├ CMF_SaveOrderDiff    @0x6BA4ECAA   一阶/二阶空间差分（§3，含原厂缺陷）
//     ├ CMF_ProcessDim(1)    @0x6BA4E8B5   逐 Tx 行切片共模消除（§4–§8）
//     └ Baseline_Compensation@0x6BA46D00   全屏基线整体牵引（§9）
//   GridIIR_Process          @0x6BA61098   时域一阶 IIR（§11）
//   Exception_CheckChargerNoiseInRxLines   FI 模式空桩（§12）
//
// ── 本机（W273AS2700）门控结论，全部取自文档 §16 ──────────────────────
//   bCmfEnabled      = flash+0x68 = 0  ⇒ 走**后置** CMF（Diff 域），
//                      Rawdata_CMF @0x6BA69381 不执行。
//   CMF_GetCmfMethod = 0               ⇒ 1D 路径，CMF_Filter2D（2D PCA）关闭。
//   bCmfForceDim1Pass= flash+0x19 = 1  ⇒ 锁死 Dim1（40 个行切片 × 60 点）。
//   CMFRemap                           ⇒ 恒等旁路（flash+0x6CC=1/+0x6CD=0/+0x80E=0）。
//   加权模式                            ⇒ CMF_ProcessCore 硬编码传 weight = 0，
//                                        1D 路径的加权分支是**死代码**。
//   SS_IsValidForCMF                   ⇒ 函数体 `return 0`，SS 辅助分支死。
//   皮套二次收敛 / 边缘特殊门限 200 /
//   游戏场景附加 CMF                    ⇒ 均关闭（flash+0x6D=0 / 工程名不匹配 /
//                                        flash+0x624=0）。
//   充电器噪声检测 / 面板 SD            ⇒ FI 模式转空桩，恒不触发。
//
// ── 定点与截断（§17 第 3/4 条）────────────────────────────────────────
//   · 切片减法在 int32 域，回写 `(int16)` **截断、无饱和** → Fixed::Wrap16。
//   · 桶均值用 C 的 `/`（向零截断）。
//   · IIR_Process2 的分母是 `/(1<<shift)` 而非右移。
//   · CMF_NoiseCal 与 CMF_PushNoiseData 的比较量先截到 **short** 再比。
// ══════════════════════════════════════════════════════════════════════

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <array>
#include <algorithm>

#include "SolverTypes.h"
#include "TouchCoreTypes.hpp"
#include "TouchFixed.hpp"
#include "TouchGates.hpp"

namespace Config {
class ConfigBinder;
class ConfigStore;
}

namespace Solvers::Touch {

// ── 滤波维度模式（CMF_ProcessDimUnitOnBuf @0x6BA4E0EC 的轴向定义）──────
//   dim == 1 → base = bCols * lineIdx, stride = 1,     count = bCols
//              40 个切片，每片一整行 60 个元素（本机唯一路径）
//   dim == 2 → base = lineIdx,         stride = bCols, count = bRows
//              60 个切片，每片一整列 40 个元素（本机不可达）
enum class CmfDimension : int {
    Dim1Rows = 1,
    Dim2Cols = 2,
};

// ── 切片节点五态分类（CMF_NoiseCal @0x6BA4E376）───────────────────────
enum class SliceState : int {
    Candidate    = 1, // 平坦基底候选
    RisingEdge   = 2, // dx >  grad
    FallingEdge  = 3, // dx < -grad
    PositivePeak = 4, // x  >  amp
    NegativePeak = 5, // x  < -amp
};

// ── 聚类桶（CMF_PushNoiseData 的 192 字节静态区 @0x6BB7F080）───────────
struct CmfNoiseCluster {
    int32_t nSum       = 0;  // Σ x_i · w_i            @+0x04 + k*0x10
    uint8_t bCount     = 0;  // 样本数                  @+0x08 + k*0x10
    int32_t nLastVal   = 0;  // 最近入桶的原始值        @+0x0C + k*0x10
    int32_t nWeightSum = 0;  // Σ w_i                   @+0x10 + k*0x10
};

class CmfAndGridFilter {
public:
    static constexpr int kRows = kSensorRows;      // 40 行 Tx
    static constexpr int kCols = kSensorCols;      // 60 列 Rx
    static constexpr int kCellCount = kTotalCells; // 2400 互容节点
    /// TSA_MAX_SENSOR_NUM = 0x1324。所有网格缓冲按 4900 元素分配而非 2400，
    /// 这正是 CMF_SaveOrderDiff 的越界写不会踩堆的原因（§3 / §17 第 5 条）。
    static constexpr int kMaxSensorNum = 0x1324;
    static constexpr size_t kMaxClusters = 10;     // 聚类桶上限

    bool m_enabled = true;

    // ══ CMF 幅值 / 梯度门限（TSADynPrmt_GetCMFCoef @0x6BB3D0BD，§4）═══
    // 每帧由 TSAPrmt_PreProcess 按 `dwSystemFlags & 0x40000`（bit18）在两组
    // Flash 值之间切换，写进运行期镜像 DAT_6BCAB6BC（低 16 位幅值、高 16 位梯度）。
    //   常态组：flash+0x620 = 600、flash+0x622 = 150
    //   备用组：flash+0x626 = 15000、flash+0x628 = 200（等效「关掉幅值剔除」）
    // bit18 由宿主每帧经 dwCtrlFlags 灌入，**属不还原的宿主契约**；复刻按常态
    // 组固定，但保留切换点 UpdateCmfCoef() 与 m_useAltCmfCoef 这个注入口。
    static constexpr int16_t kAmpTholdNormal  = 600;
    static constexpr int16_t kGradTholdNormal = 150;
    static constexpr int16_t kAmpTholdAlt     = 15000;
    static constexpr int16_t kGradTholdAlt    = 200;
    /// flash+0x624 / +0x62A（游戏场景附加 CMF 的使能），两组实测均为 0。
    static constexpr int16_t kGameExtraCmfNormal = 0;
    static constexpr int16_t kGameExtraCmfAlt    = 0;

    bool    m_useAltCmfCoef = false;   // dwSystemFlags bit18 的注入口，常态 false
    int16_t m_ampThold      = kAmpTholdNormal;
    int16_t m_gradThold     = kGradTholdNormal;
    int16_t m_gameExtraCmf  = kGameExtraCmfNormal;

    /// TSADynPrmt_GetCMFCoef @0x6BB3D0BD —— 帧首刷新两个门限。
    void UpdateCmfCoef() noexcept {
        if (!m_useAltCmfCoef) {
            m_ampThold     = kAmpTholdNormal;
            m_gradThold    = kGradTholdNormal;
            m_gameExtraCmf = kGameExtraCmfNormal;
        } else {
            m_ampThold     = kAmpTholdAlt;
            m_gradThold    = kGradTholdAlt;
            m_gameExtraCmf = kGameExtraCmfAlt;
        }
    }

    // ══ 维度与旁路开关（§16）══════════════════════════════════════════
    /// bCmfForceDim1Pass = flash+0x19 = 1。锁死 Dim1。
    static constexpr bool kForceDim1Pass = true;
    CmfDimension m_dimMode = CmfDimension::Dim1Rows;
    /// TSAStatic+0x11C（TSA_SetDisableCMFFlag 的唯一落点，不在宿主解析清单）⇒ 恒 0。
    bool m_cmfProcessingBlocked = false;
    /// TouchThold_ToeUseSpecialThold @0x6BA7CD8F：按工程名匹配 P116/P150/P186，
    /// W273AS2700 不匹配 ⇒ 恒 0，边缘切片不降到 200。
    static constexpr bool kToeUseSpecialThold = false;
    static constexpr int16_t kToeEdgeAmpThold = 200;

    // ══ 全屏基线整体牵引（Baseline_Compensation @0x6BA46D00，§9）══════
    /// TSAStatic+0x3F0（= g_tsaStaticPtr[1].field_0x2）。唯一 setter 是导出
    /// TSA_SetDisableBaselineUpdateFlag，不在 ApDaemon.dll 的解析清单里 ⇒ 恒 0，
    /// 这道门永远是开的（不是「补偿关闭」）。
    bool m_baselineUpdateDisabled = false;
    /// TSAStatic+0x30A：BLReset_IsLongTimeRawJumpDetected 的「长时 Raw 跳变检测中」，
    /// 期间禁止补偿。由 Stage 1 回灌；未接通前恒 false。
    bool m_longTimeRawJumpDetected = false;
    /// TSAStatic+0x11A ← 每帧拷自 +0x3F1（0x2000 水下异常哨兵）。
    /// **不是极性反转位**；本机恒 0 ⇒ 补偿方向固定 `bl[i] -= step`（§9）。
    bool m_underwaterResetSentinel = false;
    static constexpr int16_t kDriftDeadband = 4;   // |drift| > 4
    static constexpr int16_t kDriftStep     = 5;   // ±5 LSB/帧

    // ══ GridIIR（§11）════════════════════════════════════════════════
    /// flash+0x8F8 = 0x07E0 → 低字节 α = 224（224/256 = 87.5% 当前帧权重）。
    static constexpr uint8_t kGridIirAlpha = 224;
    static constexpr uint8_t kGridIirShift = 8;
    uint8_t m_gridIirAlpha = kGridIirAlpha;

    /// TSAStatic_IsChargerPlugged @0x6BA6A434 的等价输入，由 Stage 0 回灌。
    bool m_chargerPlugged = false;
    /// dwSystemFlags bit17 (0x20000)：宿主每帧灌入的「全频受污染」指示。
    bool m_hostAllFreqNoisyFlag = false;
    /// AFE_IsAllFreqNoisy @0x6BB2E281：读 AFE 跳频表当前频点条目 bit0。
    /// 复刻侧没有 AFE 上下文通道 ⇒ 恒 false。
    bool m_afeAllFreqNoisy = false;

    // ══ 跨帧输入 ══════════════════════════════════════════════════════
    /// DAT_6BC4A910 的**上一帧**值。TSAPrmt_Process（TouchThold 重算）排在
    /// CMF/GridIIR 之后，所以 CMF_ProcessDimUnit 的共模超限标志与
    /// TSAPrpt_GetDifPreCMFPrpt 的超噪计数读到的都是上一帧的门限（§14 关键时序）。
    /// 首帧 = TouchThold_Init 写的 600（**不是** 125）。
    int16_t m_noiseTholdPrevFrame = 600;

    // ══ 运行状态 ══════════════════════════════════════════════════════
    int16_t m_gridIirBuf[kRows][kCols]{};  // g_tsaBufGridIIRDif @0x6BB81348，跨帧存活
    /// TSAStatic->field_0xD：全频大噪判决。**充电器插入时只置不清**（§11 坑、§17 第 6 条）。
    bool    m_allFreqNoisy = false;
    /// TSAStatic->field_0xE：TSAStatic_PreProcess 在帧首对 field_0xD 的快照，
    /// 给 IsLargeDisplayNoise 提供迟滞。
    bool    m_allFreqNoisyPrev = false;

    // CMF_ProcessDim 的全屏 N_cm 极值（DAT_6BC4A940 / DAT_6BC4A942）
    int16_t m_lastMinNoise = 0x7FFF;
    int16_t m_lastMaxNoise = -0x8000;
    int16_t m_lastGlobalDrift = 0;
    std::array<int16_t, kRows> m_lastSliceOffsets{};

    // TSAPrpt_GetDifPreCMFPrpt 的两项产出（g_tsaPrpt+352 / +354）
    int16_t  m_preCmfMaxAbsDif = 0;   // 全屏 |Dif| 最大值
    uint16_t m_preCmfOverCount = 0;   // |Dif| ≥ DAT_6BC4A910 的节点数

    /// g_tsaBufOneOrderDiffRx @0x6BC16080 / g_tsaBufTwoOrderDiffRx @0x6BC32180。
    /// 只被 CMF_SaveOrderDiff 自己与 IDE 抓图读取，不参与判决；跨帧保留残值
    /// （二阶边界缺陷依赖这一点，见 SaveOrderDiff）。
    std::array<int16_t, kMaxSensorNum> m_oneOrderDiff{};
    std::array<int16_t, kMaxSensorNum> m_twoOrderDiff{};

    CmfAndGridFilter() { Reset(); }

    void Reset() noexcept {
        std::memset(m_gridIirBuf, 0, sizeof(m_gridIirBuf));
        m_allFreqNoisy = false;
        m_allFreqNoisyPrev = false;
        m_lastMinNoise = 0x7FFF;
        m_lastMaxNoise = -0x8000;
        m_lastGlobalDrift = 0;
        m_lastSliceOffsets.fill(0);
        m_preCmfMaxAbsDif = 0;
        m_preCmfOverCount = 0;
        m_oneOrderDiff.fill(0);
        m_twoOrderDiff.fill(0);
        m_ampThold = kAmpTholdNormal;
        m_gradThold = kGradTholdNormal;
        m_gameExtraCmf = kGameExtraCmfNormal;
    }

    void SetEnabled(bool enabled) noexcept { m_enabled = enabled; }
    [[nodiscard]] bool IsEnabled() const noexcept { return m_enabled; }

    void registerBindings(Config::ConfigBinder& binder) { (void)binder; }
    void applyConfig(const Config::ConfigStore& store) { (void)store; }

    // ══════════════════════════════════════════════════════════════════
    // 核心入口：原位处理 TouchBigFrame::diff
    // ══════════════════════════════════════════════════════════════════
    bool Process(TouchBigFrame& bigFrame) {
        if (!m_enabled) {
            return true;
        }

        // TSAStatic_PreProcess @0x6BAA1BB0 的帧首快照：field_0xE = field_0xD。
        m_allFreqNoisyPrev = m_allFreqNoisy;

        // TSAStatic_IsChargerPlugged @0x6BA6A434 的等价输入，由 Stage 0 挂在大帧上。
        m_chargerPlugged = bigFrame.chargerPlugged;

        // TSAPrmt_PreProcess @0x6BAFBAB7 → TSADynPrmt_GetCMFCoef（帧首刷门限）
        UpdateCmfCoef();

        // TSAPrpt_GetDifPreCMFPrpt @0x6BAA13BE —— 原版位置在 BLSM 之前，
        // 但那时 g_tsaBufDif 与此处完全一致（CMF 尚未改动），等价。
        GetDifPreCMFPrpt(bigFrame);

        // CMF_Process @0x6BA4F2C8
        CMF_Process(bigFrame);

        // GridIIR_Process @0x6BA61098
        GridIIR_Process(bigFrame);

        // Exception_CheckChargerNoiseInRxLines @0x6BA59214 → FI 空桩（§12）
        Exception_CheckChargerNoiseInRxLines(bigFrame);

        return true;
    }

    // ── 兼容入口：HeatmapFrame ────────────────────────────────────────
    bool Process(HeatmapFrame& frame) {
        if (!m_enabled) {
            return true;
        }

        if (frame.touch.bigFrame != nullptr) {
            Process(*frame.touch.bigFrame);
            for (int r = 0; r < kRows; ++r) {
                for (int c = 0; c < kCols; ++c) {
                    frame.heatmapMatrix[r][c] = frame.touch.bigFrame->diff[r][c];
                }
            }
            return true;
        }

        // 独立模式（无 bigFrame 挂载）：只跑 CMF 主链与 GridIIR，无基线牵引。
        m_allFreqNoisyPrev = m_allFreqNoisy;
        UpdateCmfCoef();
        m_lastMinNoise = 0x7FFF;
        m_lastMaxNoise = -0x8000;

        for (int r = 0; r < kRows; ++r) {
            ProcessDimUnitOnBuf(&frame.heatmapMatrix[0][0], &frame.heatmapMatrix[0][0],
                                CmfDimension::Dim1Rows, r, m_ampThold, m_gradThold,
                                nullptr, nullptr, r);
        }
        GridIIROnBuf(&frame.heatmapMatrix[0][0]);
        return true;
    }

    // ══════════════════════════════════════════════════════════════════
    // TSAPrpt_GetDifPreCMFPrpt @0x6BAA13BE（§11 的两个统计量）
    //   overCount = #(d >= noiseThold) + #(d <= -noiseThold)
    //   maxAbsDif = max(maxPositive, -minNegative)，两者初值均为 0
    // ══════════════════════════════════════════════════════════════════
    void GetDifPreCMFPrpt(const TouchBigFrame& bigFrame) noexcept {
        const int32_t thold = m_noiseTholdPrevFrame;
        int32_t overPos = 0, overNeg = 0;
        int16_t maxPos = 0, minNeg = 0;

        for (int r = 0; r < kRows; ++r) {
            for (int c = 0; c < kCols; ++c) {
                const int16_t d = bigFrame.diff[r][c];
                if (d < thold) {
                    if (d <= -thold) ++overNeg;
                } else {
                    ++overPos;
                }
                if (maxPos < d)      maxPos = d;
                else if (d < minNeg) minNeg = d;
            }
        }
        m_preCmfOverCount = static_cast<uint16_t>(overPos + overNeg);
        m_preCmfMaxAbsDif = static_cast<int16_t>(
            std::max<int32_t>(maxPos, -static_cast<int32_t>(minNeg)));
    }

    // ══════════════════════════════════════════════════════════════════
    // CMF_Process @0x6BA4F2C8（§2）
    // ══════════════════════════════════════════════════════════════════
    void CMF_Process(TouchBigFrame& bigFrame) {
        // memcpy g_tsaBufDif → g_tsaBufPreCMFDif：复刻侧没有该快照的消费者，
        // 统计量已由 GetDifPreCMFPrpt 提前取走。

        CMF_SaveOrderDiff(bigFrame);
        // CMF_SavePreCmfDif：CSV 日志（g_preCMFDiffLogEnabled 默认关），不实现。

        if (m_cmfProcessingBlocked) {
            // TSAStatic+0x11C 置位时整个方法分支被跳过，连 g_tsaBufAftCMFDif
            // 快照也不更新；一阶/二阶差分与 pre-CMF 快照仍已更新。本机恒 0。
            return;
        }

        // CMF_GetCmfMethod @0x6BA4D903 恒返回 0（flash+0x810=0、+0x814=2）
        // ⇒ 走 1D 路径，CMF_Filter2D 不可达。
        // CMFRemap_Enter：恒等旁路，bCols/bRows 保持 60/40。

        // memset g_tsaBufCommonModeNoise = 0
        for (int r = 0; r < kRows; ++r) {
            for (int c = 0; c < kCols; ++c) bigFrame.cmNoise[r][c] = 0;
        }

        // bCmfForceDim1Pass == 1 ⇒ CMF_ProcessDim(1)
        if constexpr (kForceDim1Pass) {
            CMF_ProcessDim(bigFrame, CmfDimension::Dim1Rows);
        } else {
            CMF_ProcessDim(bigFrame, m_dimMode);
        }

        // TSAStatic+0x3F0 == 0（恒真）⇒ 补偿永远执行
        if (!m_baselineUpdateDisabled) {
            Baseline_Compensation(bigFrame);
        }

        // CMFRemap_Exit：空操作。
        // 游戏场景附加 CMF：flash+0x624 = 0 ⇒ 永不执行（§15）。
        //   if (m_gameExtraCmf != 0 && TSA_IsGameScenario())
        //       CMF_ProcessDimCommonNoise(1, 1200, 200, 60, 40);

        // memcpy → g_tsaBufAftDim1CMFDif / g_tsaBufAftCMFDif：仅抓图用，不实现。
    }

    // ══════════════════════════════════════════════════════════════════
    // CMF_SaveOrderDiff @0x6BA4ECAA（§3）—— 一阶 / 二阶空间差分
    //
    // ⚠ **原厂缺陷，照抄不修正**：二阶循环的两处边界清零用了与内层相反的寻址
    // 约定。Dim1Pass == 1 时清零的是 TwoOrder[a] 与 TwoOrder[a + 59*60]
    // （a = 0..39），而内层写的是 r*60 + c（c = 1..58）。结果是
    //   · 每行的 c = 0 与 c = 59 两列**从不被写入**，保留上一帧残值；
    //   · 在 [3540, 3579] 区间（超出 2400 有效区、仍在 4900 元素缓冲内）
    //     写了一串无用的 0。
    // 这两个缓冲只被自己和 IDE 抓图读取，不参与判决。
    // ══════════════════════════════════════════════════════════════════
    void CMF_SaveOrderDiff(const TouchBigFrame& bigFrame) noexcept {
        // Dim1Pass == 1：outer = bRows(40)，inner = bCols(60)
        const int outerCount = kForceDim1Pass ? kRows : kCols;
        const int innerCount = kForceDim1Pass ? kCols : kRows;
        const int16_t* dif = &bigFrame.diff[0][0];

        // ── 一阶 ──────────────────────────────────────────────────────
        for (int a = 0; a < outerCount; ++a) {
            const int rowBase = kForceDim1Pass ? (kCols * a) : a;
            m_oneOrderDiff[static_cast<size_t>(rowBase)] = 0;
            for (int i = 1; i < innerCount; ++i) {
                const int idx  = kForceDim1Pass ? (i + a * kCols) : (a + i * kCols);
                const int prev = kForceDim1Pass ? (i + a * kCols - 1) : (a + (i - 1) * kCols);
                m_oneOrderDiff[static_cast<size_t>(idx & 0xFFFF)] =
                    Fixed::Wrap16(static_cast<int32_t>(dif[idx & 0xFFFF]) -
                                  static_cast<int32_t>(dif[prev & 0xFFFF]));
            }
        }

        // ── 二阶（含边界寻址缺陷）─────────────────────────────────────
        for (int a = 0; a < outerCount; ++a) {
            // 缺陷 1：Dim1 下清的是 TwoOrder[a]，不是 TwoOrder[a*60]
            const int zeroLo = kForceDim1Pass ? a : (kCols * a);
            m_twoOrderDiff[static_cast<size_t>(zeroLo)] = 0;
            // 缺陷 2：Dim1 下清的是 TwoOrder[a + 59*60] = TwoOrder[3540 + a]
            const int zeroHi = kForceDim1Pass ? (a + (innerCount - 1) * kCols)
                                              : (innerCount + a * kCols - 1);
            m_twoOrderDiff[static_cast<size_t>(zeroHi & 0xFFFF)] = 0;

            for (int i = 1; i < innerCount - 1; ++i) {
                const int dst  = kForceDim1Pass ? (i + a * kCols) : (a + i * kCols);
                const int next = kForceDim1Pass ? (i + a * kCols + 1) : (a + (i + 1) * kCols);
                const int cur  = dst;
                m_twoOrderDiff[static_cast<size_t>(dst & 0xFFFF)] =
                    Fixed::Wrap16(static_cast<int32_t>(m_oneOrderDiff[static_cast<size_t>(next & 0xFFFF)]) -
                                  static_cast<int32_t>(m_oneOrderDiff[static_cast<size_t>(cur & 0xFFFF)]));
            }
        }
    }

    // ══════════════════════════════════════════════════════════════════
    // CMF_ProcessDim @0x6BA4E8B5（§4）—— 切片调度
    // ══════════════════════════════════════════════════════════════════
    void CMF_ProcessDim(TouchBigFrame& bigFrame, CmfDimension dim) noexcept {
        m_lastMinNoise = 0x7FFF;
        m_lastMaxNoise = -0x8000;

        const int unitCount = (dim == CmfDimension::Dim1Rows) ? kRows : kCols;
        // SS_IsValidForCMF() 函数体是 `return 0` ⇒ SS_DiffRelatCal /
        // SS_DiffCMFProcessDimUnit 两条分支永不执行，nCm 的 SS 修正项恒 0。

        for (int u = 0; u < unitCount; ++u) {
            int16_t amp = m_ampThold;
            if constexpr (kToeUseSpecialThold) {
                if (!(u > 1 && u < unitCount - 2)) amp = kToeEdgeAmpThold;
            }

            uint8_t outDraft = 0;
            const int16_t nCm = CMF_ProcessDimUnit(bigFrame, dim, u, amp, m_gradThold, &outDraft);

            CMF_CopyCommonModeNoise(bigFrame, dim, u, nCm);

            if (nCm < m_lastMinNoise) m_lastMinNoise = nCm;   // DAT_6BC4A940
            if (m_lastMaxNoise < nCm) m_lastMaxNoise = nCm;   // DAT_6BC4A942
        }
    }

    // ── CMF_ProcessDimUnit @0x6BA4E2AF ────────────────────────────────
    int16_t CMF_ProcessDimUnit(TouchBigFrame& bigFrame, CmfDimension dim, int lineIdx,
                               int16_t amp, int16_t grad, uint8_t* outDraft) noexcept {
        const int16_t nCm = ProcessDimUnitOnBuf(&bigFrame.diff[0][0], &bigFrame.diff[0][0],
                                                dim, lineIdx, amp, grad, outDraft,
                                                &bigFrame, lineIdx);
        // if (DAT_6BC4A910 < nCm) dwSpecialControlFlags |= 0x08;
        // 读的是**上一帧**的噪声基准（§14 关键时序）。
        if (m_noiseTholdPrevFrame < nCm) {
            bigFrame.specialControlFlags |= 0x08u;
        }
        return nCm;
    }

    // ── CMF_CopyCommonModeNoise @0x6BA4E7EE ───────────────────────────
    void CMF_CopyCommonModeNoise(TouchBigFrame& bigFrame, CmfDimension dim,
                                 int lineIdx, int16_t nCm) noexcept {
        if (dim == CmfDimension::Dim1Rows) {
            for (int c = 0; c < kCols; ++c) bigFrame.cmNoise[lineIdx][c] = nCm;
        } else {
            for (int r = 0; r < kRows; ++r) bigFrame.cmNoise[r][lineIdx] = nCm;
        }
    }

    // ══════════════════════════════════════════════════════════════════
    // CMF_ProcessDimUnitOnBuf @0x6BA4E0EC（§5）—— 跨步抽取与回写
    //   · 抽取：slice[i] = (int)(int16)src[base + i*stride]
    //   · 回写：dst[base + i*stride] = (int16)slice[i]   ← **截断，无饱和**
    //   · 第 7 个参数在 Ghidra 里叫 pWeight，但 CMF_ProcessCore 把它当 pOutDraft
    //     往下传，权重位置硬编码 0 ⇒ 1D 路径永远是无权重模式。
    // ══════════════════════════════════════════════════════════════════
    int16_t ProcessDimUnitOnBuf(int16_t* dst, const int16_t* src, CmfDimension dim,
                                int lineIdx, int16_t amp, int16_t grad,
                                uint8_t* outDraft, TouchBigFrame* bigFrame,
                                int sliceRecordIdx) noexcept {
        int base, stride, count;
        if (dim == CmfDimension::Dim1Rows) {
            base = kCols * lineIdx; stride = 1;     count = kCols;
        } else {
            base = lineIdx;         stride = kCols; count = kRows;
        }

        std::array<int32_t, 258> slice{};   // 原版是 int local_418[258]，容量参数 0x100
        int pos = base;
        for (int i = 0; i < count; ++i) { slice[i] = src[pos]; pos += stride; }

        const int16_t nCm = CMF_ProcessCoreWeight(slice.data(), count, amp, grad, outDraft);

        pos = base;
        for (int i = 0; i < count; ++i) { dst[pos] = Fixed::Wrap16(slice[i]); pos += stride; }

        if (bigFrame != nullptr && dim == CmfDimension::Dim1Rows &&
            sliceRecordIdx >= 0 && sliceRecordIdx < kRows) {
            m_lastSliceOffsets[static_cast<size_t>(sliceRecordIdx)] = nCm;
            bigFrame->cmOffsets[sliceRecordIdx] = nCm;
        }
        return nCm;
    }

    // ══════════════════════════════════════════════════════════════════
    // CMF_ProcessCoreWeight @0x6BA4E55B（§8）
    //   CMF_NoiseCal(...);
    //   // 皮套自适应二次收敛：FEAT_SMART_COVER 未置位 ∧ flash+0x6D = 0 ⇒ 不执行
    //   for (i = 0; i < n; ++i) buf[i] -= N_cm;      // int32 域，无饱和
    //   if (pOutDraft) *pOutDraft = outCount;
    //   return N_cm;
    // ══════════════════════════════════════════════════════════════════
    int16_t CMF_ProcessCoreWeight(int32_t* buf, int count, int16_t amp, int16_t grad,
                                  uint8_t* outDraft) noexcept {
        if (buf == nullptr) return 0;

        NoiseCalResult res = CMF_NoiseCal(buf, count, amp, grad);

        static_assert(!Gates::IsOn("FEAT_SMART_COVER"),
                      "皮套二次收敛需 FEAT_SMART_COVER ∧ flash+0x6D，本机两者皆 0（02 §8/§16）");

        for (int i = 0; i < count; ++i) {
            buf[i] = Fixed::WrapSub32(buf[i], res.nCm);
        }
        if (outDraft != nullptr) *outDraft = res.count;
        return static_cast<int16_t>(res.nCm);
    }

    struct NoiseCalResult {
        int32_t nCm = 0;    // DAT_6BB7F128
        uint8_t count = 0;  // DAT_6BB7F124
    };

    // ══════════════════════════════════════════════════════════════════
    // CMF_NoiseCal @0x6BA4E376（§6）—— 五态分类与双连续平坦判据
    //
    // 边界细节（逐行对位反编译）：
    //   · 分类循环**从 i = 1 开始**，state[0] 从不被赋值也从不被读。
    //   · 比较量先截到 short：`x = (short)slice[i]`、`dx = (short)(x - xPrev)`。
    //   · 门限比较是严格不等，落在边界上算平坦。
    //   · 采纳循环 i = 1..n-2，要求 state[i] 与 state[i+1] 都为 1（双连续平坦）。
    // ══════════════════════════════════════════════════════════════════
    NoiseCalResult CMF_NoiseCal(const int32_t* slice, int count,
                                int16_t amp, int16_t grad) const noexcept {
        if (slice == nullptr || count < 3) return {};

        std::array<int, 258> states{};

        int16_t prev = Fixed::Wrap16(slice[0]);
        for (int i = 1; i < count; ++i) {
            const int16_t x  = Fixed::Wrap16(slice[i]);
            const int16_t dx = Fixed::Wrap16(static_cast<int32_t>(x) - static_cast<int32_t>(prev));

            if (amp < x) {
                states[i] = static_cast<int>(SliceState::PositivePeak);
            } else if (static_cast<int32_t>(x) < -static_cast<int32_t>(amp)) {
                states[i] = static_cast<int>(SliceState::NegativePeak);
            } else if (grad < dx) {
                states[i] = static_cast<int>(SliceState::RisingEdge);
            } else if (static_cast<int32_t>(dx) < -static_cast<int32_t>(grad)) {
                states[i] = static_cast<int>(SliceState::FallingEdge);
            } else {
                states[i] = static_cast<int>(SliceState::Candidate);
            }
            prev = x;
        }

        // CMF_InitNoiseData：整块 192 字节 memset + lastIdx = 0xFFFF
        std::array<CmfNoiseCluster, kMaxClusters> clusters{};
        uint8_t  activeBucket = 0;
        uint8_t  bucketCount  = 0;
        uint16_t lastIndex    = 0xFFFF;

        for (int i = 1; i < count - 1; ++i) {
            if (states[i] == static_cast<int>(SliceState::Candidate) &&
                states[i + 1] == static_cast<int>(SliceState::Candidate)) {
                PushNoiseData(slice, i, grad, clusters, activeBucket, bucketCount, lastIndex);
            }
        }

        return NoiseStatistics(clusters, bucketCount);
    }

    // ══════════════════════════════════════════════════════════════════
    // IIR_Process2 @0x6BA6021B（§11）
    //   y = (α·cur + (2^k − α)·hist) / (1 << k)   ← **截断除，不是右移**
    //   y < cur → y+1 ；y > cur → y−1 ；否则 y
    // ══════════════════════════════════════════════════════════════════
    static inline int IIR_Process2(int cur, int hist, uint8_t alpha, uint8_t shift) noexcept {
        const int scale = 1 << shift;
        const int y = (static_cast<int>(alpha) * cur +
                       (scale - static_cast<int>(alpha)) * hist) / scale;
        if (y < cur)      return y + 1;
        else if (cur < y) return y - 1;
        return y;
    }

    // ══════════════════════════════════════════════════════════════════
    // TSAStatic_IsLargeDisplayNoise @0x6BAA24F9（§11）
    //   「大屏噪」只是进入条件的前半段，仍必须叠加宿主 0x20000 或 AFE 全频污染。
    //   field_0xE（上一帧判决）给这条判据提供迟滞。
    // ══════════════════════════════════════════════════════════════════
    [[nodiscard]] bool IsLargeDisplayNoise() const noexcept {
        const bool bigNoise =
            (static_cast<int32_t>(m_preCmfMaxAbsDif) >
                 static_cast<int32_t>(kPeakFilterReferenceThld) * 2) &&
            (m_preCmfOverCount > static_cast<uint16_t>(kCellCount >> 2));

        if (bigNoise || m_allFreqNoisyPrev) {
            if (m_hostAllFreqNoisyFlag) return true;
            if (m_afeAllFreqNoisy)      return true;
        }
        return false;
    }

    // ══════════════════════════════════════════════════════════════════
    // TSAStatic_IsAllFreqNoisy @0x6BAA2590（§11、§17 第 6 条）
    // ⚠ 充电器插入时**只置不清**：一旦在插充状态下进过噪声态，只有拔掉充电器
    //   （走 else 分支）才能恢复。照抄该粘滞行为。
    // ══════════════════════════════════════════════════════════════════
    bool TSAStatic_IsAllFreqNoisy() noexcept {
        if (!m_chargerPlugged) {
            m_allFreqNoisy = IsLargeDisplayNoise();
        } else {
            if (m_hostAllFreqNoisyFlag) m_allFreqNoisy = true;
            if (m_afeAllFreqNoisy)      m_allFreqNoisy = true;
        }
        return m_allFreqNoisy;
    }

    /// TSAStatic_IsAllFreqBecomeNoisy @0x6BAA260A：先刷新 field_0xD，再与
    /// 帧首快照 field_0xE 比较，取上升沿。
    bool TSAStatic_IsAllFreqBecomeNoisy() noexcept {
        m_allFreqNoisy = TSAStatic_IsAllFreqNoisy();
        return m_allFreqNoisy && !m_allFreqNoisyPrev;
    }

    // ══════════════════════════════════════════════════════════════════
    // GridIIR_Process @0x6BA61098（§11）
    //   先判上升沿：命中则**只播种、本帧不滤波**；否则若仍是噪声态才滤波。
    //   干净环境下历史缓冲完全不更新。
    // ══════════════════════════════════════════════════════════════════
    void GridIIR_Process(TouchBigFrame& bigFrame) noexcept {
        if (TSAStatic_IsAllFreqBecomeNoisy()) {
            std::memcpy(m_gridIirBuf, bigFrame.diff, sizeof(m_gridIirBuf));
        } else if (TSAStatic_IsAllFreqNoisy()) {
            GridIIR_ProcessCore(&bigFrame.diff[0][0]);
        }
    }

    /// GridIIR_ProcessCore @0x6BA60FD5：逐点 IIR，历史即输出。
    void GridIIR_ProcessCore(int16_t* dif) noexcept {
        int16_t* hist = &m_gridIirBuf[0][0];
        for (int i = 0; i < kCellCount; ++i) {
            const int y = IIR_Process2(dif[i], hist[i], m_gridIirAlpha, kGridIirShift);
            dif[i] = Fixed::Wrap16(y);
            hist[i] = dif[i];
        }
    }

    // ══════════════════════════════════════════════════════════════════
    // Baseline_Compensation @0x6BA46D00（§9）
    //   drift = (maxNcm < 0) ? maxNcm : (minNcm > 0 ? minNcm : 0);
    //   if (|drift| > 4 && !longTimeRawJump) {
    //       step = (drift < 1) ? -5 : +5;
    //       for all i: bl[i] += (sentinel ? +step : -step);   // 本机恒 -step
    //       dwSpecialControlFlags |= 0x100;
    //   }
    // 复刻侧 Stage 1 与 Stage 2 不共用同一份基线数组，改由
    // bigFrame.baselineDriftStep 交接：该字段语义 = **本帧要加到基线上的量**。
    // ══════════════════════════════════════════════════════════════════
    void Baseline_Compensation(TouchBigFrame& bigFrame) noexcept {
        int16_t drift = 0;
        if (m_lastMaxNoise < 0)      drift = m_lastMaxNoise;
        else if (0 < m_lastMinNoise) drift = m_lastMinNoise;
        m_lastGlobalDrift = drift;

        bigFrame.baselineDriftStep = 0;

        const int32_t absDrift = (drift < 0) ? -static_cast<int32_t>(drift) : drift;
        if (absDrift > kDriftDeadband && !m_longTimeRawJumpDetected) {
            const int16_t step = (drift < 1) ? static_cast<int16_t>(-kDriftStep) : kDriftStep;
            // +0x11A（镜像 +0x3F1 水下哨兵）恒 0 ⇒ 固定 `bl[i] -= step`。
            bigFrame.baselineDriftStep = m_underwaterResetSentinel
                                             ? step
                                             : static_cast<int16_t>(-step);
            bigFrame.specialControlFlags |= 0x100u;
        }
    }

    // ══════════════════════════════════════════════════════════════════
    // Exception_CheckChargerNoiseInRxLines @0x6BA59214（§12）
    // dwPeakProcessingMode == 3（flash+0x34 = 3）⇒ 转 FI 空桩
    // FIException_CheckChargerNoiseInRxLines @0x6BA59429，**函数体只有一条 ret**；
    // FIException_IsChargerNoiseDetected @0x6BA5942F **恒返回 0**。
    // 早先版本在这里自造了一套「Rx 线交流纹波极性翻转计数」检测并置
    // bigFrame.isChargerNoise —— 原厂没有这段逻辑，已删除。完整实现在
    // HIException_CheckChargerNoiseInRxLines @0x6BA5B9D1，只在非 FI 模式可达。
    // ══════════════════════════════════════════════════════════════════
    void Exception_CheckChargerNoiseInRxLines(TouchBigFrame& bigFrame) noexcept {
        (void)bigFrame;   // 空桩
    }

    /// FIException_IsChargerNoiseDetected @0x6BA5942F
    [[nodiscard]] static constexpr bool Exception_IsChargerNoiseDetected() noexcept {
        return false;
    }

    /// FIException_CheckPanelSD @0x6BA593BC
    static void Exception_CheckPanelSD() noexcept {}

    /// SS_IsValidForCMF：函数体 `return 0`
    [[nodiscard]] static constexpr bool SS_IsValidForCMF(int) noexcept { return false; }

    /// TSADynPrmt_GetHTI / GetLTI / GetMFLTI / GetTZThold：四个空桩（§17 第 10 条）
    static void TSADynPrmt_GetHTI() noexcept {}
    static void TSADynPrmt_GetLTI() noexcept {}
    static void TSADynPrmt_GetMFLTI() noexcept {}
    static void TSADynPrmt_GetTZThold() noexcept {}

    /// SignalDisparity_SwitchBuffer @0x6BB35E61 / SS_ChargerNoiseFilterSwitchBuffer
    /// @0x6BB01B92 —— 本机恒 0 ⇒ **永不重跑 Peak**（§12 / §13）。
    [[nodiscard]] static constexpr bool SwitchDifBuffer() noexcept {
        static_assert(!Gates::IsOn("SS_SELF_CAPACITANCE"),
                      "flash+0x608 = 0 ⇒ SS 双维无效，两个 SwitchBuffer 恒返回 0");
        return false;
    }

private:
    static constexpr int16_t kPeakFilterReferenceThld = 3000;  // flash+0x5B4

    void GridIIROnBuf(int16_t* dif) noexcept {
        if (TSAStatic_IsAllFreqBecomeNoisy()) {
            std::memcpy(&m_gridIirBuf[0][0], dif, sizeof(m_gridIirBuf));
        } else if (TSAStatic_IsAllFreqNoisy()) {
            GridIIR_ProcessCore(dif);
        }
    }

    // ══════════════════════════════════════════════════════════════════
    // CMF_PushNoiseData @0x6BA4DA6A（§7）—— 在线聚类
    //   1. 空间连续扩展：idx == lastIdx + 1 → 直接并入 activeBucket（不做值域检查）
    //   2. 值域邻近合并：线性扫描，第一个满足 -grad < (short)(x − last[k]) < grad 的桶
    //   3. 开新桶（上限 10）
    //   4. 桶满兜底：强行并入当前 activeBucket
    //   每次入桶都无条件更新 last[k] = x 与 lastIdx = idx。
    // lastIdx 初值 0xFFFF：原版比较写作 `(uint)idx == (uint)g_cmfNoise + 1`，
    // 提升到 int 后为 0x10000，因此 idx == 0 不会误判成连续。
    // 1D 路径权重恒 1（weightPtr == 0）。
    // ══════════════════════════════════════════════════════════════════
    void PushNoiseData(const int32_t* slice, int index, int16_t grad,
                       std::array<CmfNoiseCluster, kMaxClusters>& clusters,
                       uint8_t& activeBucket, uint8_t& bucketCount,
                       uint16_t& lastIndex) const noexcept {
        const int32_t val = slice[index];
        const int32_t w = 1;   // DAT_6BB7F138 == 0 ⇒ 权重恒 1

        auto accumulate = [&](uint8_t k) {
            clusters[k].nSum = Fixed::WrapAdd32(clusters[k].nSum, val * w);
            clusters[k].bCount = static_cast<uint8_t>(clusters[k].bCount + 1);
            clusters[k].nLastVal = val;
            clusters[k].nWeightSum = Fixed::WrapAdd32(clusters[k].nWeightSum, w);
        };

        if (static_cast<uint32_t>(index) == static_cast<uint32_t>(lastIndex) + 1u) {
            accumulate(activeBucket);
        } else {
            bool merged = false;
            for (uint8_t k = 0; k < bucketCount; ++k) {
                // 差值先截到 short 再比，与 CMF_NoiseCal 共用同一个 grad
                const int16_t d = Fixed::Wrap16(Fixed::WrapSub32(val, clusters[k].nLastVal));
                if (d < grad && -static_cast<int32_t>(grad) < d) {
                    activeBucket = k;
                    accumulate(k);
                    merged = true;
                    break;
                }
            }
            if (!merged) {
                if (bucketCount < kMaxClusters) {
                    activeBucket = bucketCount;
                    clusters[activeBucket].nSum = val * w;
                    clusters[activeBucket].bCount = 1;
                    clusters[activeBucket].nLastVal = val;
                    clusters[activeBucket].nWeightSum = w;
                    ++bucketCount;
                } else {
                    accumulate(activeBucket);   // 桶满兜底：不新开、不丢弃
                }
            }
        }
        lastIndex = static_cast<uint16_t>(index);
    }

    // ══════════════════════════════════════════════════════════════════
    // CMF_NoiseStatistics @0x6BA4DEB4（§8）—— 无权重路径（本机唯一路径）
    //   mean = (short)(sum[k] / count[k]);                 // 向零截断
    //   minMean / minCount   ：均值最小的桶
    //   zeroMean / zeroCount ：|均值| 最接近 0 的桶
    //   if (bucketCount == 0)                  N_cm = 0,       out = 0
    //   else if (zeroCount < minCount || minMean > 0)  N_cm = minMean,  out = minCount
    //   else                                   N_cm = zeroMean, out = zeroCount
    // 加权路径（取 wsum 最大的桶）只有 SS_CMFProcessDim2 可达，是死代码。
    // ══════════════════════════════════════════════════════════════════
    NoiseCalResult NoiseStatistics(const std::array<CmfNoiseCluster, kMaxClusters>& clusters,
                                   uint8_t bucketCount) const noexcept {
        int16_t minMean = 0x7FFF;
        uint8_t minCount = 0;
        int16_t zeroMean = 0x7FFF;
        uint8_t zeroCount = 0;

        for (uint8_t k = 0; k < bucketCount; ++k) {
            const int16_t mean = Fixed::Wrap16(
                Fixed::TruncDiv(clusters[k].nSum, static_cast<int32_t>(clusters[k].bCount)));
            if (mean < minMean) {
                minMean = mean;
                minCount = clusters[k].bCount;
            }
            const int32_t a = (mean < 0) ? -static_cast<int32_t>(mean) : mean;
            const int32_t b = (zeroMean < 0) ? -static_cast<int32_t>(zeroMean) : zeroMean;
            if (a < b) {
                zeroMean = mean;
                zeroCount = clusters[k].bCount;
            }
        }

        NoiseCalResult r{};
        if (bucketCount == 0) {
            r.nCm = 0;
            r.count = 0;
        } else if (zeroCount < minCount || minMean > 0) {
            r.nCm = minMean;
            r.count = minCount;
        } else {
            r.nCm = zeroMean;
            r.count = zeroCount;   // 原版此支不重写 outCount，它已持有 zeroCount
        }
        return r;
    }
};

} // namespace Solvers::Touch
