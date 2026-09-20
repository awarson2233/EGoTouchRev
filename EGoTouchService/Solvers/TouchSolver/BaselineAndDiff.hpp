#pragma once
// ══════════════════════════════════════════════════════════════════════
// TouchPipeline Stage 1: BaselineAndDiff
//
// 原厂对位：TSACore.dll（image base 0x6BA40000）的基线子系统，
// 规范文档 docs/finger_algorithms/01_baseline_and_diff.md（§1–§14）。
//
//   BLIIR_CalcDiffCommon        @0x6BA46F4C   §2   差分
//   BLIIR_CalcDiff              @0x6BA46EFC   §2
//   BLIIR_ResetDiff             @0x6BA46E76   §6
//   BLIIR_DoUpdate              @0x6BA47141   §6   定步长带死区跟踪
//   BLIIR_Update                @0x6BA470E7   §6   信号闸门
//   BLIIR_Reset                 @0x6BA4721F   §6   硬对齐
//   BLSM_Init                   @0x6BA4C220   §7
//   BLSM_Process                @0x6BA4C645   §7
//   BLSM_GetProperty            @0x6BA4D523   §7.1
//   BLSM_UpdateStage            @0x6BA4C6F1   §7.2
//   BLSM_ProcessStage           @0x6BA4D170   §7.3
//   BLSM_GetNoTouchBlStep       @0x6BA4CF9C   §7.3
//   BLSM_SaveNormalBl           @0x6BA4D0C3   §7.3
//   BLSM_ShbProcess             @0x6BA4C4CC   §7.4
//   BLSM_ShbCaptureBl           @0x6BA4C422   §7.4
//   BLSM_ShbIsRawChanges        @0x6BA4C382   §7.4
//   BLSM_IsReset / IsTouched…   @0x6BA4D794   §7.5
//   BLSM_UpdateForRawUnstable   @0x6BA4C27B   §9.3
//   TSADynPrmt_GetBL            @0x6BB3CB4F   §8   每帧门限与步长
//   TSADynPrmt_UpdateBfrTch     @0x6BB3C960   §8
//   TSADynPrmt_UpdateStepAftTch @0x6BB3CA49   §8
//   BLReset_Init                @0x6BA47A26   §9
//   BLReset_Process             @0x6BA4BF90   §9
//   BLReset_IsBaselineNoisy     @0x6BA4C0BA   §9
//   BLReset_InDebounce          @0x6BA47750   §9
//   BLReset_IsTouchToBeProtected@0x6BA47B08   §9.1
//   BLReset_CheckTouchTrustability @0x6BA4A613 §9.1
//   BLRecal_Process             @0x6BA47458   §12
//   Touch_GetFormattedTimeInterval @0x6BA76329 §7.2
//   TSA_GetRawPrpt              @0x6BAA0A6E   §10
//
// ── 本机输入极性（唯一一处与 TSACore 语义不同的适配）───────────────────
// TSACore 的 `g_tsaBufRaw` 是 AFE 上送的**绝对互容读数**，手指按压会让读数
// 下降，故 `bField_0x11a == 0` 分支取 `Dif = Bl - Raw`。
//
// EGoTouch 的 Himax MCU 经 SPI 送来的是**偏移二进制的已差分量**：
// `MasterFrameParser` 做 `wire ^ 0x8000` 之后，本底噪声均值≈0（±100），
// 手指处是 +2800~4800 的**正峰**（证据：docs/artifacts/touchsolver-tsacore-
// pipeline-diff.md「输入线上格式与极性」，以及 perf/touch.dvrbin 逐帧
// heatmap.max≈3300 / avg≈3 且 contacts.count==1）。
// 因此本模块固定走 TSACore 的 `bField_0x11a == 1` 反向分支 `Dif = Raw - Bl`，
// 基线冷启动值为 0（不采样首帧，否则开机瞬间已按下的手指会被吸进基线）。
//
// ⚠ 这**不是**「极性判反了」：`TSAStatic + 0x3F1` 确实恒 0（见下），但它决定的
// 是 TSACore 输入约定下的方向；本机输入约定本身与 TSACore 相反，两者叠加后
// 落在 `Raw - Bl` 这一支。把 m_signalReverted 改成 false 会让全屏信号反号，
// 峰值检测永远不触发。
//
// ── TSAStatic + 0x3F1（水下异常哨兵）──────────────────────────────────
// `BLReset_Check @0x6BA48AF0` 的反编译确认：该字节只被
// `BLReset_IsTriggered2AfeWakeUp` / `SafeBaseline_IsResetTriggered` 在
// `g_pPrevTouches->nTouchCount == 0x2000`（水下异常字）时置 1，而
// `BLReset_Check` / `SafeBaseline_ResetCheck` 每帧在 `nTouchCount != 0x2000`
// 时**无条件清 0**。本机 FEAT_UNDERWATER 关闭 ⇒ 异常字取不到 0x2000 ⇒ 恒 0。
// 它**不是**「基线复位进行中」，本模块因此不再对外暴露任何这样语义的接口。
// ══════════════════════════════════════════════════════════════════════

#include <cstdint>
#include <cstring>
#include <algorithm>

#include "SolverTypes.h"
#include "FrameLayout.h"
#include "TouchCoreTypes.hpp"
#include "TouchFixed.hpp"
#include "TouchGates.hpp"

namespace Solvers::Touch {

// ── 基线有限状态机阶段 (g_tsaPrpt.dwBaselineStage @ +0x278) ────────────
// Stage 8 / Stage 9 在本库**无任何入口**：唯一能选中它们的属性位 0x80 的写入者
// 只有 BLSM_GetProperty，而它产出的取值集合是 {0,1,2,0x10,0x40,0x100,0x200}。
// 文档 §复刻检查清单-9：可以不实现，但不要为了补全而伪造跳转。
// 这里保留枚举值（以便阶段号与原版一一对应）但**不提供进入路径**。
enum class BaselineStage : uint32_t {
    Normal           = 0, // 无触点定步长跟踪
    ResetHw          = 1, // 外部 / BLReset 触发的重置
    ResetNoise       = 2, // 噪声重置（BLReset_IsBaselineNoisy）
    Hold             = 3, // 保持 / 就地回退到上一阶段
    SmallSignalWait  = 4, // 小信号等待（基线冻结）
    TouchActive      = 5, // 触点锁定（基线冻结）
    LiftoffDebounce  = 6, // 抬起去抖（基线冻结）
    PostTouchRecover = 7, // 抬起后快速回追
    LargeRecover     = 8, // 【无入口】
    NegativeTracking = 9, // 【无入口】
};

// ── 基线属性位 (g_tsaPrpt.dwBaselinePropertyFlags @ +0x280) ────────────
enum BaselinePropertyFlags : uint32_t {
    FlagClean     = 0x000, // maxSig < smallThold
    FlagSmallSig  = 0x001, // smallThold <= maxSig < noiseThold
    FlagTouch     = 0x002, // maxSig >= noiseThold 或 AFE 报核心区触点
    FlagReset     = 0x010, // 外部重置或 BLReset_Process 命中
    FlagRefresh   = 0x040, // bForceBaselineProperty / TSA_SetDisableBaselineUpdateFlag
    FlagNegative  = 0x080, // 【全库无产出方】保留编号，不得伪造
    FlagDebounce  = 0x100, // BLReset_InDebounce
    FlagNoisy     = 0x200, // BLReset_IsBaselineNoisy
};

// ── 重置原因码 (g_tsaPrpt.dwBaselinePropertyState @ +0x2A0) ────────────
// 本模块可达的子集；其余码（4/5/7..0x17）由未复刻的 BLReset_IsTriggered2
// 各触发源产出，见文档 §9 表。
enum BaselineResetReason : uint32_t {
    ReasonNone          = 0,
    ReasonExternalWake  = 1, // dwSystemFlags & 5
    ReasonExternalCal   = 2, // dwSystemFlags & 8
    ReasonExternalOther = 3, // 其余外部重置
    ReasonBaselineNoisy = 6, // BLReset_IsBaselineNoisy 计时中
};

// ── §8 TSADynPrmt_GetBL 每帧算出的门限与步长 ─────────────────────────
// 名字后的括号是原版裸地址。W273AS2700 实测非 IDLE 稳态值写在默认值里。
struct BaselineDynParams {
    int16_t  noiseThold      = 600; // DAT_6BCAB6C2
    int16_t  sideNoiseThold  = 450; // _DAT_6BCAB6C4 = (noise*3)>>2
    int16_t  smallThold      = 300; // DAT_6BCAB6C6 = noise/2
    int16_t  sideSmallThold  = 225; // _DAT_6BCAB6C8 = side/2
    int16_t  clampAftTch     = 599; // DAT_6BCAB6CA._0_2_ = noise - 1
    int16_t  clampAftTchSide = 449; // DAT_6BCAB6CA._2_2_ = side  - 1
    uint8_t  stepNoTouch     = 2;   // DAT_6BCAB6CA._5_1_ = clamp(noise>>8,1,255)
    uint8_t  stepNoTouchSide = 1;   // DAT_6BCAB6CA._6_1_ = clamp(side >>8,1,255)
    uint8_t  stepAftTch      = 9;   // DAT_6BCAB6D2._0_1_ = clamp(noise>>6,1,255)
    uint16_t stageTimeoutMs  = 0x198; // DAT_6BCAB6D2._2_2_ = 408 ms（硬编码）
    uint16_t resetCounterCap = 20000; // DAT_6BCAB6D2._6_2_ = flash[0x38]*5*0x28
};

// ── §10 本模块自产的帧属性子集 ────────────────────────────────────────
// 完整的 7 块区块统计（TSA_GetDifPrpt / TSA_CombineBlock）在复刻侧被拆到了
// 它的实际消费者那里：核心区代数总和 g_tsaPrpt+0xC8 由模块 03 的
// ScanInnerSignalSum（行 1..38 × 列 1..58，与掩码 5 = 块0∪块2 逐格等价）产出，
// pre-CMF 统计 g_tsaPrpt+0x160.. 由模块 02 的 GetDifPreCMFPrpt 产出。
// 本结构只保留模块 01 自己的消费者（BLSM / BLRecal）需要的字段。
struct BaselinePrpt {
    // +0xA0 / +0xA2：全屏 Dif 极值。原版由 TSA_GetPrpt 在 CMF/GridIIR **之后**
    // 写，而 BLSM_Process 排在 CMF 之前 ⇒ BLSM 本帧读到的是**上一帧**的值。
    int16_t nSPeakPositive = 0;
    int16_t nSPeakNegative = 0;
    // +0x92 / +0x94：全屏 Raw 极值（原版按 u16 比较线上字；本机 Raw 已由
    // MasterFrameParser 去掉 0x8000 偏移，序关系不变，故按 s16 比较等价）
    int16_t wBoundaryPositive = 0;
    int16_t wBoundaryNegative = 0;
    // +0x96 / +0x98 / +0x9A：内区（去掉最外一圈）Raw 极值与均值
    int16_t innerRawMax  = 0;
    int16_t innerRawMin  = 0;
    int16_t innerRawMean = 0;
    // +0x29A：内区 Baseline 均值（同一除数 (bCols-2)*(bRows-2) = 2204）
    int16_t innerBlMean  = 0;
};

// ── 宿主 / AFE 侧输入（§14 明确属「不还原」范围，这里只定义读取契约）────
// 缺省值 = 本机常态。没有来源的项保持缺省，对应原版那条分支不可达。
struct BaselineInputs {
    /// TSAStatic_IsHWReset() = dwSystemFlags & 0xF
    bool     hardwareReset = false;
    /// dwSystemFlags（§14.1）。bit0 供 BLRecal_Process 清计数，bit0/2/3 供
    /// BLSM_GetProperty 给重置原因码分类。
    uint32_t systemFlags = 0;
    /// AFE_IsAlwaysNoisy()。BLReset_IsBaselineNoisy 的**唯一起搏器**：
    /// 复刻侧无此通道 ⇒ 噪声计时器永不启动 ⇒ Stage 2 不可达。
    /// 这是「输入缺失」而不是「逻辑缺失」，函数体按原版实现。
    bool     afeAlwaysNoisy = false;
    /// static->field_0xD8 & 0x40000（AFE 帧「raw 失稳」标志）。
    /// BLSM_UpdateForRawUnstable 的门控；复刻侧无此通道 ⇒ 默认关。
    bool     afeRawUnstable = false;
    /// TSA_IsGameScenario()
    bool     gameScenario = false;
    /// static->field_0x3 = AFE_IsInIDLEMode() 的本帧缓存。
    /// ⚠ 它与 dwProcessingState & 0x10 是**两个不同的门**（§7.3 / §8），不可合并。
    bool     afeIdle = false;
    /// dwProcessingState & 0x10：TSADynPrmt_UpdateBfrTch / UpdateStepAftTch
    /// 的步长加倍门，以及 TSADynPrmt_GetBL 的 IDLE 噪声门限分支。
    bool     processingIdle = false;
    /// g_tsaPrpt._4_2_：raw2raw 超噪节点数，由 HardwareAnalyzer_GetRaw2RawPrpt
    /// 产出，BLReset_IsBaselineNoisy 读它（`< 6` 判「噪声已散」）。
    uint16_t raw2rawNumOverNoise = 0;
    /// AFE_IsAutoCalibration() / AFE_IsRecalRequested()（§12）
    bool     afeAutoCalibration = false;
    bool     afeRecalRequested  = false;
};

// ── 模块运行状态 ──────────────────────────────────────────────────────
struct BaselineState {
    BaselineStage stage = BaselineStage::Normal;      // +0x278
    BaselineStage prevStage = BaselineStage::Normal;  // +0x27C
    uint32_t      propertyFlags = FlagClean;          // +0x280
    uint32_t      prevPropertyFlags = FlagClean;
    uint32_t      propertyState = ReasonNone;         // +0x2A0
    uint32_t      prevPropertyState = ReasonNone;     // +0x2A4
    uint16_t      stageFrameCount = 1;                // +0x288（BLSM_Init 置 1）
    int64_t       stageReferenceMs = 0;               // +0x290，单位**毫秒**

    int16_t       maxSig = 0;   // BLSM_GetMaxSig  @0x6BA4D41D
    int16_t       minSig = 0;   // BLSM_GetMinSig  @0x6BA4D4A0

    bool          singleTouchLatch = false;         // bSingleTouchBaselineLatch
    bool          baselineUpdatedThisFrame = false; // static +0x3EB
    bool          resetDiffRequested = false;       // static +0x3ED
    bool          baselineWrittenThisFrame = false; // static +0x3EE
    bool          baselineResetPropertyLatched = false;
    bool          justExitedSpecialCtrlMode = false; // §12
    uint32_t      specialControlFlags = 0;           // dwSpecialControlFlags
    uint32_t      resetReasonBitmap = 0;             // static +0x3F4（每帧清）

    uint16_t      framesSinceHwReset      = 0;       // +0x284
    uint16_t      framesSinceBaselineReset = 0;      // +0x286

    // §7.4 SHB 影子基线
    uint32_t      shbState = 0;        // +0x2A8
    int64_t       shbStateStartMs = 0; // +0x2B0
    uint32_t      shbFlags = 0;        // +0x2B8

    // §12 BLRecal
    uint8_t       recalFrameCount = 0xFF;   // BLRecal_Init 置 0xFF
    uint8_t       recalBoundaryHitCount = 0;

    int32_t       frameIntervalMs = 10;     // nBaselineStageInitialSignal
};

class BaselineAndDiff {
public:
    static constexpr int kRows = kSensorRows;      // 40
    static constexpr int kCols = kSensorCols;      // 60
    static constexpr int kCellCount = kTotalCells; // 2400

    // 内区除数 (bCols-2)*(bRows-2) = 58*38 = 2204（§10）
    static constexpr int kInnerCellCount = (kCols - 2) * (kRows - 2);

    // ── 模块使能 ──────────────────────────────────────────────────────
    bool m_enabled = true;

    // ── 差分极性（见文件头「本机输入极性」）──────────────────────────
    // false: Dif = Bl  - Raw（TSACore bField_0x11a == 0，绝对互容输入）
    // true : Dif = Raw - Bl （TSACore bField_0x11a == 1，本机 MCU 预差分输入）
    bool m_signalReverted = true;

    // ── 冷启动基线（输入已是差分量 ⇒ 0 即正确，不采样首帧）───────────
    int32_t m_baseline = 0;

    // ── §8 每帧门限与步长（RecomputeDynParams 重算）──────────────────
    // DAT_6BC4A918：TouchThold_Init 置 wPeakMode3ThresholdMax = 600，
    // MS 手指路径不改写它（只有 HandGesture/SideTouch 临时保存-恢复）。
    int16_t m_stableTholdBase = kPeakMode3ThresholdMax;
    BaselineDynParams m_dyn{};

    // ── 宿主 / AFE 输入 ───────────────────────────────────────────────
    BaselineInputs m_inputs{};

    // ── 构造与复位 ────────────────────────────────────────────────────
    BaselineAndDiff() { Reset(); }

    /// BLSM_Init @0x6BA4C220 = BLIIR_Init + BLReset_Init + BLRecal_Init
    ///                       + stage=0 / frameCount=1 / reference=0
    void Reset() noexcept {
        m_state = BaselineState{};
        m_state.stage = BaselineStage::Normal;
        m_state.stageFrameCount = 1;   // BLSM_Init
        m_state.stageReferenceMs = 0;  // BLSM_Init
        m_state.recalFrameCount = 0xFF;  // BLRecal_Init @0x6BA478E0
        m_state.recalBoundaryHitCount = 0;
        m_prpt = BaselinePrpt{};
        m_resetPending = false;
        m_refreshPending = false;
        m_resetInDebounce = false;
        m_noisyTimerMs = 0;
        m_frameCounter = 0;
        m_prevTimestampUs = 0;
        m_hasPrevTimestamp = false;
        RecomputeDynParams();

        const int16_t seed = Fixed::Wrap16(m_baseline);
        for (int r = 0; r < kRows; ++r) {
            for (int c = 0; c < kCols; ++c) {
                m_bl[r][c] = seed;
                m_diff[r][c] = 0;
                m_normalBl[r][c] = seed;
                m_rawCapture[r][c] = 0;
                m_cleanBaseline[r][c] = 0;  // BLReset_Init: memset g_tsaBufCleanBaseline
            }
        }
        for (auto& t : m_trustability) t = 0;
    }

    // ── 外部控制接口 ──────────────────────────────────────────────────
    void SetEnabled(bool enabled) noexcept { m_enabled = enabled; }
    [[nodiscard]] bool IsEnabled() const noexcept { return m_enabled; }
    [[nodiscard]] BaselineStage GetStage() const noexcept { return m_state.stage; }
    [[nodiscard]] const BaselineState& GetState() const noexcept { return m_state; }
    [[nodiscard]] const BaselinePrpt&  GetPrpt()  const noexcept { return m_prpt; }
    [[nodiscard]] const BaselineDynParams& GetDynParams() const noexcept { return m_dyn; }

    /// 请求一次硬复位（TSAStatic_IsHWReset() → BLSM_Process(true, false)）
    void RequestReset() noexcept { m_resetPending = true; }
    /// 请求一次刷新（bForceBaselineProperty / TSA_SetDisableBaselineUpdateFlag）
    void RequestRefresh() noexcept { m_refreshPending = true; }

    /// BLSM_IsReset @0x6BA4D794：**只认 Stage 1**。
    /// TSA_MSProcess 用它决定是否跳过 CMF_Process / GridIIR_Process ⇒
    /// Stage 2（噪声重置）那一帧 Dif 已被清零但 CMF/GridIIR **照跑**（§7.5）。
    [[nodiscard]] bool BLSM_IsReset() const noexcept {
        return m_state.stage == BaselineStage::ResetHw;
    }
    /// BLSM_IsTouched @0x6BA4D7AD
    [[nodiscard]] bool BLSM_IsTouched() const noexcept {
        return m_state.stage == BaselineStage::TouchActive;
    }
    /// BLSM_IsBaselineOK @0x6BA4D7C6
    [[nodiscard]] bool BLSM_IsBaselineOK() const noexcept {
        return m_state.propertyFlags == FlagClean || m_state.propertyFlags == FlagDebounce;
    }
    /// BLSM_IsOKToIDLE @0x6BA4D7FD。flash[0x44] 实测 0 ⇒ 仅 Stage 0。
    [[nodiscard]] bool BLSM_IsOKToIDLE() const noexcept {
        return m_state.stage == BaselineStage::Normal;
    }

    /// BLReset_InDebounce @0x6BA47750 的只读快照（原版就是读一个字节，
    /// 没有副作用；由 BLReset_IsTriggered2AfeWakeUp 递增/递减）。
    [[nodiscard]] bool IsResetDebounceActive() const noexcept { return m_resetInDebounce; }

    /// 只读访问当前基线矩阵（诊断用）
    [[nodiscard]] const int16_t (&GetBaseline() const noexcept)[kRows][kCols] { return m_bl; }
    /// BLSM_SaveNormalBl 的快照（g_tsaBufBlNormal）
    [[nodiscard]] const int16_t (&GetNormalBaseline() const noexcept)[kRows][kCols] { return m_normalBl; }

    // ══════════════════════════════════════════════════════════════════
    // 管线入口
    // ══════════════════════════════════════════════════════════════════

    /// 从 masterSuffix / bigFrame 自动提取 hasFinger
    inline bool Process(HeatmapFrame& frame) {
        bool hasFinger = frame.masterWasRead && frame.masterSuffixValid && frame.masterSuffix.hasFinger();
        if (frame.touch.bigFrame != nullptr) {
            hasFinger = hasFinger || frame.touch.bigFrame->hasFinger;
        }
        return Process(frame, hasFinger);
    }

    /// 显式 hasFinger（兼容路径）
    inline bool Process(HeatmapFrame& frame, bool hasFinger) {
        if (!m_enabled) {
            return true;
        }

        if (!(frame.masterWasRead && frame.masterSuffixValid)) {
            InvalidateFrame(frame.touch.bigFrame);
            std::memset(frame.heatmapMatrix, 0, sizeof(frame.heatmapMatrix));
            return true;
        }

        auto* bigFrame = frame.touch.bigFrame;

        // heatmapMatrix 在本阶段既是 raw 输入也是 dif 输出，先取一份 raw 快照
        int16_t raw[kRows][kCols];
        std::memcpy(raw, frame.heatmapMatrix, sizeof(raw));

        if (bigFrame != nullptr) {
            std::memcpy(bigFrame->preDif, bigFrame->diff, sizeof(bigFrame->preDif));
        }

        RunBaselineFrame(raw, hasFinger, bigFrame);

        std::memcpy(frame.heatmapMatrix, m_diff, sizeof(m_diff));
        if (bigFrame != nullptr) {
            PublishTo(*bigFrame);
        }
        return true;
    }

    /// 原生 TouchBigFrame 入口
    bool Process(TouchBigFrame& bigFrame, bool hasFinger = false) {
        if (!m_enabled) return true;

        if (!bigFrame.masterSuffixValid) {
            InvalidateFrame(&bigFrame);
            return true;
        }

        hasFinger = hasFinger || bigFrame.hasFinger;
        std::memcpy(bigFrame.preDif, bigFrame.diff, sizeof(bigFrame.preDif));

        RunBaselineFrame(bigFrame.raw, hasFinger, &bigFrame);
        PublishTo(bigFrame);
        return true;
    }

    // ══════════════════════════════════════════════════════════════════
    // 对外暴露的原版算子
    // ══════════════════════════════════════════════════════════════════

    /// BLIIR_CalcDiffCommon @0x6BA46F4C
    ///   signalReverted == false → Dif = Baseline - Raw（bField_0x11a == 0）
    ///   signalReverted == true  → Dif = Raw - Baseline（bField_0x11a == 1）
    /// 原版写回是 `(short)` 截断 ⇒ 用 Fixed::Wrap16，不得饱和（§复刻清单-1）。
    static void BLIIR_CalcDiffCommon(
        const int16_t raw[kRows][kCols],
        const int16_t baseline[kRows][kCols],
        int16_t diff[kRows][kCols],
        bool signalReverted = false) noexcept
    {
        for (int r = 0; r < kRows; ++r) {
            for (int c = 0; c < kCols; ++c) {
                const int32_t d = signalReverted
                    ? static_cast<int32_t>(raw[r][c]) - static_cast<int32_t>(baseline[r][c])
                    : static_cast<int32_t>(baseline[r][c]) - static_cast<int32_t>(raw[r][c]);
                diff[r][c] = Fixed::Wrap16(d);
            }
        }
    }

    /// BLIIR_ResetDiff @0x6BA46E76
    static void BLIIR_ResetDiff(int16_t diff[kRows][kCols]) noexcept {
        std::memset(diff, 0, sizeof(int16_t) * kCellCount);
    }

    /// Touch_GetFormattedTimeInterval @0x6BA76329（§7.2）
    ///   Δ == 0            → 1000 / flash[0x38] = 10 ms
    ///   Δ < 0 且首帧      → 0
    ///   |Δ| <= 7 或刚进/出 IDLE → ±8
    ///   |Δ| > 1000        → ±1000
    ///   其余              → Δ 原样
    [[nodiscard]] static int16_t Touch_GetFormattedTimeInterval(
        int16_t delta, uint32_t frameCounter,
        bool afeJustEnterIdle = false, bool afeJustExitIdle = false) noexcept
    {
        constexpr int16_t kFrameRateBase = 100; // flash[0x38] = 0x64
        if (delta == 0) {
            return static_cast<int16_t>(1000 / kFrameRateBase); // 10 ms
        }
        if (delta < 0) {
            if (frameCounter == 0) return 0;
            if (delta < -7 && !afeJustEnterIdle) {
                return (delta > -1001) ? delta : static_cast<int16_t>(-1000);
            }
            return -8;
        }
        if (delta > 7 && !afeJustExitIdle) {
            return (delta < 1001) ? delta : static_cast<int16_t>(1000);
        }
        return 8;
    }

    /// BLReset_IsTouchToBeProtected @0x6BA47B08（§9.1）
    /// `dwPeakProcessingMode = flash[0x34] = 3`（实测）⇒ 单指与双指
    /// **无条件受保护**，自容/互容幅值判据全部走不到。
    /// 命中时置 `static[0x3F4] |= 8`。
    bool BLReset_IsTouchToBeProtected(const TouchBigFrame* bigFrame) noexcept {
        bool protectedNow = false;

        // 第一条（FEAT_SMART_COVER）本机恒 0，整条不可达。
        static_assert(!Gates::IsOn("FEAT_SMART_COVER"),
                      "皮套关闭 ⇒ SmartCover_GetPrevTouchNumInOpenWindow 分支不可达");

        const int n = m_prevTouchCount;
        if (n == 1) {
            if (!m_prevTouchBigShape[0]) {
                protectedNow = true;   // mode 3 无条件
            }
        } else if (n == 2) {
            if (!m_prevTouchBigShape[0] || !m_prevTouchBigShape[1]) {
                protectedNow = true;   // mode 3 无条件
            }
        }
        (void)bigFrame;

        if (protectedNow) {
            m_state.resetReasonBitmap |= 8u;
        }
        return protectedNow;
    }

private:
    // ── 内部状态 ──────────────────────────────────────────────────────
    BaselineState m_state{};
    BaselinePrpt  m_prpt{};

    int16_t  m_bl[kRows][kCols]{};            // g_tsaBufBl
    int16_t  m_diff[kRows][kCols]{};          // g_tsaBufDif
    int16_t  m_normalBl[kRows][kCols]{};      // g_tsaBufBlNormal
    int16_t  m_rawCapture[kRows][kCols]{};    // g_tsaBufRawCapture（SHB）
    int16_t  m_cleanBaseline[kRows][kCols]{}; // g_tsaBufCleanBaseline

    bool     m_resetPending = false;
    bool     m_refreshPending = false;
    bool     m_resetInDebounce = false;   // static->bBaselineResetInDebounce
    int64_t  m_noisyTimerMs = 0;          // static +0x300
    uint32_t m_frameCounter = 0;          // static->nFrameCounter
    uint64_t m_prevTimestampUs = 0;
    bool     m_hasPrevTimestamp = false;
    int64_t  m_nowMs = 0;                 // static->pFrameBufferPtr（毫秒时间戳）

    // §9.1 信誉计数 touch + 0x4EC，按轨迹 ID 索引
    uint8_t  m_trustability[kMaxTracks]{};
    // 上一帧触点的 §9.1 输入快照（PrevTouch_PostProcess 的等价物）
    int      m_prevTouchCount = 0;
    bool     m_prevTouchBigShape[2]{};    // touch +0x1F0 & 0x20000（ZONE_FLAG_BIG_SHAPE）

    void InvalidateFrame(TouchBigFrame* bigFrame) noexcept {
        BLIIR_ResetDiff(m_diff);
        m_state.stage = BaselineStage::Normal;
        m_state.prevStage = BaselineStage::Normal;
        m_state.propertyFlags = FlagClean;
        m_state.singleTouchLatch = false;
        m_state.maxSig = 0;
        m_state.minSig = 0;
        m_noisyTimerMs = 0;
        // 基线本身仍然有效，掉帧不触发重新播种
        if (bigFrame != nullptr) {
            BLIIR_ResetDiff(bigFrame->diff);
            bigFrame->hasFinger = false;
        }
    }

    void PublishTo(TouchBigFrame& bigFrame) noexcept {
        std::memcpy(bigFrame.diff, m_diff, sizeof(m_diff));
        std::memcpy(bigFrame.baseline, m_bl, sizeof(m_bl));
        bigFrame.baselineResetInDebounce = m_resetInDebounce;
        bigFrame.specialControlFlags |= m_state.specialControlFlags;
    }

    // ══════════════════════════════════════════════════════════════════
    // 单帧主流程 —— 对位 TSA_MSProcess 中基线段的调用序（§架构流程图）
    // ══════════════════════════════════════════════════════════════════
    void RunBaselineFrame(const int16_t raw[kRows][kCols], bool hasFinger, TouchBigFrame* bigFrame) noexcept {
        // 0. TSAStatic_PreProcess：本帧规整时间间隔（§7.2）
        AdvanceFrameClock(bigFrame);
        // dwSpecialControlFlags 在复刻里对位 TouchBigFrame::specialControlFlags，
        // 后者由 ResetPerFrame 每帧清零 ⇒ 本模块侧同样按帧重建，避免变成粘滞位。
        m_state.specialControlFlags = 0;

        // 1. §10 全屏 Dif 极值来自**上一帧** TSA_GetPrpt（它排在 CMF 之后，
        //    而 BLSM_Process 排在 CMF 之前）。bigFrame->diff 在本级改写之前
        //    保存的正是上一帧 CMF/GridIIR 之后的最终 Dif。
        CaptureDifExtremes(bigFrame != nullptr ? bigFrame->diff : m_diff);

        // 2. §9.2 消费上一帧 CMF 的 Baseline_Compensation 全屏平移量
        if (bigFrame != nullptr && bigFrame->baselineDriftStep != 0) {
            Baseline_CompensationApply(bigFrame->baselineDriftStep);
            bigFrame->baselineDriftStep = 0;
        }

        // 3. §9.1 上一帧触点快照（BLReset_* 全族的输入）
        CapturePrevTouchSnapshot(bigFrame, hasFinger);

        // 4. §2 BLIIR_CalcDiffCommon —— 全屏差分，先于 BLSM
        BLIIR_CalcDiffCommon(raw, m_bl, m_diff, m_signalReverted);
        //   （§10 的 pre-CMF 统计由模块 02 的 GetDifPreCMFPrpt 产出，不重复）

        // 5. §9.3 BLSM_UpdateForRawUnstable —— 原版位于 BLSM_Process **之前**
        BLSM_UpdateForRawUnstable(raw);

        // 6. §8 TSADynPrmt_GetBL：本帧门限与步长
        RecomputeDynParams();

        // 7. §7 BLSM_Process
        BLSM_Process(raw, bigFrame);

        // 8. §10 TSA_GetRawPrpt 在原版里排在 BLSM 之后（块 V），因此
        //    BLRecal_Process 读到的 wBoundaryPositive/Negative 是上一帧的值。
        TSA_GetRawPrpt(raw);
    }

    // ── TSAStatic_PreProcess 的时间部分（§7.2）────────────────────────
    void AdvanceFrameClock(const TouchBigFrame* bigFrame) noexcept {
        const uint64_t nowUs = (bigFrame != nullptr) ? bigFrame->timestampUs : 0;
        int16_t deltaMs = 0;
        if (m_hasPrevTimestamp && nowUs >= m_prevTimestampUs) {
            const uint64_t dUs = nowUs - m_prevTimestampUs;
            deltaMs = Fixed::Wrap16(static_cast<int32_t>(dUs / 1000u));
        } else if (m_hasPrevTimestamp) {
            deltaMs = 0;
        }
        m_state.frameIntervalMs =
            Touch_GetFormattedTimeInterval(deltaMs, m_frameCounter);
        m_prevTimestampUs = nowUs;
        m_hasPrevTimestamp = true;
        m_nowMs += m_state.frameIntervalMs;
        ++m_frameCounter;
    }

    // ── §10 全屏 Dif 极值（TSA_CombineBlock(…, 0x7F) 的 max/min）────────
    // 块记录初值 max = 0x8000 / min = 0x7FFF ⇒ 结果是真正的极值，可为负。
    void CaptureDifExtremes(const int16_t dif[kRows][kCols]) noexcept {
        int16_t maxSig = INT16_MIN;
        int16_t minSig = INT16_MAX;
        for (int r = 0; r < kRows; ++r) {
            for (int c = 0; c < kCols; ++c) {
                const int16_t v = dif[r][c];
                if (v > maxSig) maxSig = v;
                if (v < minSig) minSig = v;
            }
        }
        m_prpt.nSPeakPositive = maxSig;
        m_prpt.nSPeakNegative = minSig;
        // BLSM_GetMaxSig / GetMinSig @0x6BA4D41D / 0x6BA4D4A0：
        // SafeBaseline 关闭 + TouchThold_ToeUseSpecialThold() 恒假
        // ⇒ 核心区极值替换与 ToeSyna 修正都不生效，取全屏值。
        static_assert(!Gates::IsOn("FEAT_SAFE_BASELINE"),
                      "SafeBaseline 关闭 ⇒ BLSM_GetMaxSig/MinSig 走 ToeSyna 分支");
        m_state.maxSig = maxSig;
        m_state.minSig = minSig;
    }

    // ── §10 TSA_GetRawPrpt @0x6BAA0A6E ────────────────────────────────
    void TSA_GetRawPrpt(const int16_t raw[kRows][kCols]) noexcept {
        int16_t boundMax = INT16_MIN, boundMin = INT16_MAX;
        int16_t innerMax = INT16_MIN, innerMin = INT16_MAX;
        int32_t innerRawSum = 0, innerBlSum = 0;
        for (int r = 0; r < kRows; ++r) {
            for (int c = 0; c < kCols; ++c) {
                const int16_t v = raw[r][c];
                if (v > boundMax) boundMax = v;
                if (v < boundMin) boundMin = v;
                if (r >= 1 && r <= kRows - 2 && c >= 1 && c <= kCols - 2) {
                    if (v > innerMax) innerMax = v;
                    if (v < innerMin) innerMin = v;
                    innerRawSum += v;
                    innerBlSum  += m_bl[r][c];
                }
            }
        }
        m_prpt.wBoundaryPositive = boundMax;
        m_prpt.wBoundaryNegative = boundMin;
        m_prpt.innerRawMax  = innerMax;
        m_prpt.innerRawMin  = innerMin;
        m_prpt.innerRawMean = Fixed::Wrap16(Fixed::TruncDiv(innerRawSum, kInnerCellCount));
        m_prpt.innerBlMean  = Fixed::Wrap16(Fixed::TruncDiv(innerBlSum,  kInnerCellCount));
    }

    // ── §9.1 上一帧触点快照 ───────────────────────────────────────────
    // 原版读的是 g_pPrevTouches（PrevTouch_PostProcess 在上一帧末尾写）。
    // 复刻里 tracks 本就跨帧存活，帧首读到的就是上一帧的结论。
    void CapturePrevTouchSnapshot(const TouchBigFrame* bigFrame, bool hasFinger) noexcept {
        m_prevTouchCount = 0;
        m_prevTouchBigShape[0] = false;
        m_prevTouchBigShape[1] = false;
        m_trustTrackCount = 0;
        if (bigFrame == nullptr) {
            // 无大帧（单元测试路径）：退化成 hasFinger 的单指等价物
            m_prevTouchCount = hasFinger ? 1 : 0;
            return;
        }
        for (const auto& t : bigFrame->tracks) {
            if (m_prevTouchCount < 2) {
                // touch + 0x1F0 bit17（0x20000）：TZ_UpdateTouchFlag 在
                // ZONE_FLAG_BIG_SHAPE 命中时置位 —— 「大物体」标记。
                m_prevTouchBigShape[m_prevTouchCount] =
                    (t.volatileFlags & 0x20000u) != 0;
            }
            ++m_prevTouchCount;

            if (m_trustTrackCount < kMaxTracks) {
                TrustInput& ti = m_trustInputs[m_trustTrackCount];
                // 原版读的是 g_pPeaks[touch+0x23C] 指向的源峰行列；复刻里
                // 轨迹重心（Q8）落在同一格上，整数部分即峰值行列。
                ti.peakCol = static_cast<uint8_t>(
                    std::min<int32_t>(Fixed::ShrUnsigned(t.curX_q8, 8), kCols - 1));
                ti.peakRow = static_cast<uint8_t>(
                    std::min<int32_t>(Fixed::ShrUnsigned(t.curY_q8, 8), kRows - 1));
                ti.persistFlags1F8 = t.persistFlags1F8;   // touch + 0x1F8
                // touch + 0x1A0 是一个形态量，复刻侧尚无对应字段。原版判据是
                // `<= 12`，缺省 0 取「通过」一侧，保持可信分支可达而不是死掉。
                ti.shapeMetric1A0 = 0;
                ti.peakSig = t.peakSig;                   // touch + 0x1BE
                // 信誉计数存在触点自己的 touch+0x4EC 上，跨帧随触点走；
                // 复刻里按轨迹 ID 定槽，避免 tracks 次序变动串味。
                ti.slot = static_cast<uint8_t>(t.id % kMaxTracks);
                ++m_trustTrackCount;
            }
        }
    }

    // ══════════════════════════════════════════════════════════════════
    // §8 TSADynPrmt_GetBL @0x6BB3CB4F
    // ══════════════════════════════════════════════════════════════════
    void RecomputeDynParams() noexcept {
        // special 的四项是并列的或：
        //   ① 手套闩锁 DAT_6BC4A924 且 (cfg&4 ∧ feat bit0) —— 门控恒真，
        //      故第一项完全由运行期闩锁决定；普通手指场景闩锁为 0。
        //   ② FEAT_SMART_COVER —— 本机恒 0
        //   ③ TSAStatic + 0x3F1（水下哨兵）—— 本机恒 0
        //   ④ bFullIncellSignalDisparity —— Peak 每帧写
        // ①④ 在本模块没有输入通道，且两条分支的噪声门限相同
        // （DAT_6BC4A914 与 DAT_6BC4A918 都 = wPeakMode3ThresholdMax = 600），
        // 只有边列门限不同（450 vs 600）。本模块不读边列门限（ToeUseSpecialThold
        // 恒假 ⇒ g_BLSMNextEdgeSmallThresh / BLSM_ProcessUseNextSideThold 恒假），
        // 因此两条分支对本模块**等价**，统一取非 special 支。
        static_assert(!Gates::IsOn("UNDERWATER_SENTINEL"),
                      "TSAStatic+0x3F1 恒 0（§2 / 02 §9）");
        static_assert(!Gates::IsOn("CMF_TOE_EDGE_THOLD"),
                      "TouchThold_ToeUseSpecialThold 恒假 ⇒ 边列特殊门限全族关闭（§8）");

        int16_t noise = m_stableTholdBase;                       // DAT_6BC4A918 = 600
        int16_t side  = Fixed::Wrap16(Fixed::ShrTowardZero(static_cast<int32_t>(noise) * 3, 2));

        if (m_inputs.processingIdle) {
            // IDLE 支：noise = DAT_6BCAA0B4（无写入者，保持 0），smallThold = 2n/3。
            // 本机 AFE IDLE 通道未接入，此支不可达；保留结构以便换预置。
            m_dyn.smallThold = Fixed::Wrap16(Fixed::TruncDiv(2 * static_cast<int32_t>(noise), 3));
        } else {
            // TSADynPrmt_UpdateNoiseTholdBfrTch @0x6BB3C849（ToeUseSpecialThold 假）
            m_dyn.smallThold     = Fixed::Wrap16(Fixed::TruncDiv(noise, 2));
            m_dyn.sideSmallThold = Fixed::Wrap16(Fixed::TruncDiv(side, 2));
        }
        m_dyn.noiseThold     = noise;
        m_dyn.sideNoiseThold = side;

        // TSADynPrmt_UpdateBfrTch @0x6BB3C960
        m_dyn.stepNoTouch     = ClampStep(Fixed::ShrTowardZero(noise, 8));
        m_dyn.stepNoTouchSide = ClampStep(Fixed::ShrTowardZero(side, 8));
        if (m_inputs.processingIdle) {
            // 直接乘 2，无饱和保护（原厂行为，照抄）
            m_dyn.stepNoTouch = static_cast<uint8_t>(m_dyn.stepNoTouch * 2);
        }
        m_dyn.stageTimeoutMs = 0x198;   // 408 ms，硬编码

        // TSADynPrmt_UpdateStepAftTch @0x6BB3CA49
        m_dyn.clampAftTch     = Fixed::Wrap16(static_cast<int32_t>(noise) - 1);
        m_dyn.clampAftTchSide = Fixed::Wrap16(static_cast<int32_t>(side) - 1);
        m_dyn.stepAftTch      = ClampStep(Fixed::ShrTowardZero(noise, 6));
        if (m_inputs.processingIdle) {
            const int8_t s = static_cast<int8_t>(m_dyn.stepAftTch);
            m_dyn.stepAftTch = (s < 0) ? static_cast<uint8_t>(0xFF)
                                       : static_cast<uint8_t>(static_cast<int8_t>(s * 2));
        }
        // DAT_6BCAB6D2._6_2_ = flash[0x38] * 5 * 0x28 = 100 * 200 = 20000
        m_dyn.resetCounterCap = static_cast<uint16_t>(100 * 5 * 0x28);
    }

    /// `if (v < 1) v = 1; else if (v > 255) v = 255;`
    [[nodiscard]] static uint8_t ClampStep(int32_t v) noexcept {
        if (v < 1) v = 1;
        else if (v > 255) v = 255;
        return static_cast<uint8_t>(v);
    }

    // ══════════════════════════════════════════════════════════════════
    // §7 BLSM_Process @0x6BA4C645
    // ══════════════════════════════════════════════════════════════════
    void BLSM_Process(const int16_t raw[kRows][kCols], TouchBigFrame* bigFrame) noexcept {
        BLRecal_Process();

        // refresh = bForceBaselineProperty || static[0x3F0]
        // reset   = bHardwareReset         || static[0x3EF]
        const bool refresh = m_refreshPending;
        const bool reset   = m_resetPending || m_inputs.hardwareReset;

        BLSM_GetProperty(reset, refresh);
        BLSM_UpdateStage(bigFrame);
        BLSM_ProcessStage(raw);
        BLSM_ShbProcess(raw);

        m_refreshPending = false;
        m_resetPending = false;
    }

    // ── §12 BLRecal_Process @0x6BA47458 ───────────────────────────────
    // 参数：flash+0x48 = 0x8000（中心值）、flash+0x4A = 0x1F40 = 8000（窗口宽度）。
    // 本机 Raw 已被 MasterFrameParser 去掉 0x8000 偏移，故中心值折算为 0。
    void BLRecal_Process() noexcept {
        constexpr int32_t kRecalCenter = 0;     // flash[0x48] = 0x8000 − 偏移
        constexpr int32_t kRecalWindow = 8000;  // flash[0x4A] = 0x1F40

        m_state.justExitedSpecialCtrlMode = false;

        if ((m_inputs.systemFlags & 1u) != 0) {
            m_state.recalFrameCount = 0;
            m_state.recalBoundaryHitCount = 0;
        }

        if (m_state.recalFrameCount < 5) {
            ++m_state.recalFrameCount;          // 预热，不统计
        } else if (m_state.recalFrameCount < 0xF) {
            ++m_state.recalFrameCount;
            const int32_t pk  = m_prpt.nSPeakPositive;
            const int32_t neg = -static_cast<int32_t>(m_prpt.nSPeakNegative);
            if (pk < m_stableTholdBase && neg < m_stableTholdBase) {
                // 三条判据是 else-if 链：每帧至多 +1
                const int32_t span = static_cast<int32_t>(m_prpt.wBoundaryPositive)
                                   - static_cast<int32_t>(m_prpt.wBoundaryNegative);
                if (span > kRecalWindow) {
                    ++m_state.recalBoundaryHitCount;
                } else if (m_prpt.wBoundaryPositive > kRecalCenter + kRecalWindow / 2) {
                    ++m_state.recalBoundaryHitCount;
                } else if (m_prpt.wBoundaryNegative < kRecalCenter - kRecalWindow / 2) {
                    ++m_state.recalBoundaryHitCount;
                }
            }
        } else {
            if (m_state.recalBoundaryHitCount > 2 && !m_inputs.afeAutoCalibration) {
                m_state.justExitedSpecialCtrlMode = true;
                m_state.specialControlFlags |= 2u;
            }
            m_state.recalBoundaryHitCount = 0;  // frameCount 停在 15
        }

        // AFE 自校准超过 300 s 后的二次出口
        if (m_inputs.afeAutoCalibration && m_inputs.afeRecalRequested &&
            m_state.baselineUpdatedThisFrame && m_state.stageFrameCount > 2) {
            m_state.justExitedSpecialCtrlMode = true;
        }
    }

    // ── §7.1 BLSM_GetProperty @0x6BA4D523 ─────────────────────────────
    void BLSM_GetProperty(bool reset, bool refresh) noexcept {
        const int16_t maxSig = m_state.maxSig;   // BLSM_GetMaxSig 先于一切求值
        m_state.prevPropertyFlags = m_state.propertyFlags;
        m_state.prevPropertyState = m_state.propertyState;
        m_state.propertyState = ReasonNone;
        // g_BLSMNextEdgeSmallThresh 恒 0（ToeUseSpecialThold 恒假），不再建模。

        if (reset) {
            m_state.baselineResetPropertyLatched = true;
            m_state.propertyFlags = FlagReset;
            if ((m_inputs.systemFlags & 5u) != 0)      m_state.propertyState = ReasonExternalWake;
            else if ((m_inputs.systemFlags & 8u) != 0) m_state.propertyState = ReasonExternalCal;
            else                                       m_state.propertyState = ReasonExternalOther;
            return;
        }
        if (refresh) {
            m_state.propertyFlags = FlagRefresh;
            return;
        }
        // ⚠ 原版在 reset / refresh 两支里**不调用** BLReset_Process，
        // 因此 +0x284 / +0x286 两个帧计数在那两帧不推进。照抄。
        if (BLReset_Process(false)) {
            m_state.propertyFlags = FlagReset;
            return;
        }
        if (BLReset_IsBaselineNoisy()) {
            m_state.propertyFlags = FlagNoisy;
            return;
        }
        if (m_resetInDebounce) {   // BLReset_InDebounce @0x6BA47750
            m_state.propertyFlags = FlagDebounce;
            return;
        }
        // AFE_IsCoreTouchDetected() 在本机无通道，恒假。
        if (maxSig < m_dyn.noiseThold) {
            m_state.propertyFlags = (maxSig < m_dyn.smallThold) ? FlagClean : FlagSmallSig;
        } else {
            m_state.propertyFlags = FlagTouch;
        }
    }

    // ── §9 BLReset_Process @0x6BA4BF90 ────────────────────────────────
    bool BLReset_Process(bool externalReset) noexcept {
        m_state.resetReasonBitmap = 0;              // static[0x3F4]，每帧清
        if (externalReset) m_state.framesSinceBaselineReset = 0;

        // BLReset_IsToDisable() 恒返回 0 ⇒ bBaselineResetCheckEnabled 恒 1
        bool triggered = externalReset;
        if (!triggered) {
            // BLReset_IsTriggered @0x6BA4BF6D：SafeBaseline 关闭 ⇒ IsTriggered2。
            // BLReset_IsTriggered2 @0x6BA4BC66 的 11 个触发源里，本机可达的
            // 那些（LongTimeRawJump / AfeWakeUp / BaselineAbnormal /
            // CurBaselineDirty / AfeRawJump / AntiTouchStageTimeout）全部依赖
            // AFE 帧标志与未复刻的模块状态；复刻侧没有这些输入通道，
            // 因此本函数在本机等价于「只随外部重置命中」。
            // ⚠ 这是**输入缺失**，不是判据被简化：一旦接入 AFE 帧标志，
            // 应在此处按 §9 的表补齐各触发源（非短路、原因码取最后命中者）。
            triggered = false;
        }

        if (m_state.framesSinceHwReset < m_dyn.resetCounterCap)      ++m_state.framesSinceHwReset;
        if (m_state.framesSinceBaselineReset < m_dyn.resetCounterCap) ++m_state.framesSinceBaselineReset;
        if (triggered) m_state.framesSinceBaselineReset = 0;

        // 尾部无条件执行
        BLReset_CheckTouchTrustability();
        return triggered;
    }

    // ── §9 BLReset_IsBaselineNoisy @0x6BA4C0BA ────────────────────────
    bool BLReset_IsBaselineNoisy() noexcept {
        const uint32_t prev = m_state.prevPropertyState;
        if (prev != 6 && prev != 3 && prev != 0 && m_inputs.afeAlwaysNoisy) {
            m_noisyTimerMs = m_state.frameIntervalMs;
        }
        if (m_noisyTimerMs == 0) {
            return false;
        }
        if (m_inputs.raw2rawNumOverNoise < 6) {
            m_noisyTimerMs = 0;
            return true;
        }
        if (m_noisyTimerMs < 0xA7) {   // 167 ms
            m_noisyTimerMs += m_state.frameIntervalMs;
            m_state.propertyState = ReasonBaselineNoisy;
            return true;
        }
        m_noisyTimerMs = 0;
        return false;
    }

    // ── §9.1 BLReset_CheckTouchTrustability @0x6BA4A613 ───────────────
    // 信誉计数 touch + 0x4EC，范围 0..15。
    // ⚠ 增减两条路径的守卫**都是** `counter < 0xF` ⇒ 计数一旦达到 15 就既不能
    // 加也不能减，成为**吸收态**（§复刻清单-7）。照抄。
    void BLReset_CheckTouchTrustability() noexcept {
        // FEAT bit15（侧列偏移）关闭 ⇒ 列边界 = (0, bCols-1)、行边界 = (0, bRows-1)
        static_assert((Gates::kFeatureFlags & 0x8000u) == 0,
                      "dwFeatureFlags bit15 = 0 ⇒ 列上界为 bCols-1");
        // 原版循环只覆盖存在的上帧触点；不在列表里的槽位计数保持不变。
        for (size_t i = 0; i < m_trustTrackCount; ++i) {
            const TrustInput& t = m_trustInputs[i];
            uint8_t& counter = m_trustability[t.slot];
            const bool inCore = (t.peakCol > 0) && (t.peakCol < kCols - 1) &&
                                (t.peakRow != 0) && (t.peakRow < kRows - 1);
            if (inCore) {
                const bool trusted = ((t.persistFlags1F8 & 0x20000000u) != 0) &&
                                     (t.shapeMetric1A0 <= 12) &&
                                     (static_cast<int32_t>(t.peakSig) * 2 >
                                      -static_cast<int32_t>(m_prpt.nSPeakNegative));
                if (trusted) {
                    if (counter < 0xF) ++counter;
                } else {
                    if (counter < 0xF && counter != 0) --counter;
                }
            } else {
                if (counter < 0xF && counter != 0) --counter;
            }
        }
    }

    // §9.1 信誉计数的逐触点输入（由 CapturePrevTouchSnapshot 之外的调用方填）
    struct TrustInput {
        uint8_t  slot = 0;              // 信誉计数的宿主槽位（按轨迹 ID 稳定）
        uint8_t  peakCol = 0;
        uint8_t  peakRow = 0;
        uint32_t persistFlags1F8 = 0;   // touch + 0x1F8
        int16_t  shapeMetric1A0 = 0;    // touch + 0x1A0
        int16_t  peakSig = 0;           // touch + 0x1BE
    };
    TrustInput m_trustInputs[kMaxTracks]{};
    size_t     m_trustTrackCount = 0;

    // ── §7.2 BLSM_UpdateStage @0x6BA4C6F1 ─────────────────────────────
    void BLSM_UpdateStage(const TouchBigFrame* bigFrame) noexcept {
        const uint32_t flags = m_state.propertyFlags;
        m_state.prevStage = m_state.stage;

        // 优先级自上而下，命中即直接赋值并返回（**不更新计时器**）
        if (flags & FlagReset)    { m_state.stage = BaselineStage::ResetHw;   return; }
        if (flags & FlagNoisy)    { m_state.stage = BaselineStage::ResetNoise; return; }
        if (flags & FlagDebounce) { m_state.stage = BaselineStage::Hold;       return; }
        // flags & 0x80 → Stage 9：属性位 0x80 全库无产出方，**不伪造入口**。
        if (flags & FlagRefresh)  { m_state.stage = BaselineStage::Hold;       return; }

        BaselineStage next = m_state.stage;
        ++m_state.stageFrameCount;
        m_state.stageReferenceMs += m_state.frameIntervalMs;   // 单位：毫秒

        const bool touched  = (flags & FlagTouch) != 0;
        const bool smallSig = (flags & FlagSmallSig) != 0;
        const bool timedOut = m_state.stageReferenceMs >= static_cast<int64_t>(m_dyn.stageTimeoutMs);

        switch (m_state.stage) {
        case BaselineStage::Normal:
            if (touched)       { RestartStageTimer(); next = BaselineStage::TouchActive; }
            else if (smallSig) { RestartStageTimer(); next = BaselineStage::SmallSignalWait; }
            break;

        case BaselineStage::ResetHw:
        case BaselineStage::ResetNoise:
        case BaselineStage::Hold:
            // 1/2/3 一律下一帧无条件回 0
            RestartStageTimer();
            next = BaselineStage::Normal;
            break;

        case BaselineStage::SmallSignalWait:
            if (touched)       { RestartStageTimer(); next = BaselineStage::TouchActive; }
            else if (timedOut) { RestartStageTimer(); next = BaselineStage::Normal; }
            break;

        case BaselineStage::TouchActive:
            if (!touched) {
                RestartStageTimer();
                next = m_state.singleTouchLatch ? BaselineStage::SmallSignalWait
                                                : BaselineStage::LiftoffDebounce;
            }
            // bSingleTouchBaselineLatch 在「是否离开 Stage 5」判断**之后**
            // 无条件重算，所以本帧用到的是上一帧的值（§7.2 易错点 2）。
            //   latch = (prevTouchCount == 1 && (prevTouches[0xE] & 0x200) == 0)
            m_state.singleTouchLatch =
                (m_prevTouchCount == 1) && !SingleTouchLatchVeto(bigFrame);
            break;

        case BaselineStage::LiftoffDebounce:
            if (touched)       { RestartStageTimer(); next = BaselineStage::TouchActive; }
            else if (timedOut) { RestartStageTimer(); next = BaselineStage::PostTouchRecover; }
            break;

        case BaselineStage::PostTouchRecover: {
            // local_e = DAT_6BCAB6C6（小信号门限 300）；边列替换恒不成立。
            const int16_t exitThold = m_dyn.smallThold;
            if (touched) {
                RestartStageTimer();
                next = BaselineStage::TouchActive;
            } else if (!timedOut) {
                // ⚠ 峰值判定只在 ref < 408 ms 时求值（`if (ref < to) {...} else {回0}`）
                if (m_prpt.nSPeakPositive < exitThold &&
                    -static_cast<int32_t>(m_prpt.nSPeakNegative) < static_cast<int32_t>(exitThold)) {
                    RestartStageTimer();
                    next = BaselineStage::Normal;
                }
            } else {
                RestartStageTimer();
                next = BaselineStage::Normal;
            }
            break;
        }

        case BaselineStage::LargeRecover:
        case BaselineStage::NegativeTracking:
            // 无入口；保留 switch 分支与原版 case 8/9 对齐
            if (touched)       { RestartStageTimer(); next = BaselineStage::TouchActive; }
            else if (timedOut) { RestartStageTimer(); next = BaselineStage::Normal; }
            break;
        }

        m_state.stage = next;
    }

    /// prevTouches[0xE] & 0x200：Stage 5 单指锁存的否决位。
    /// 复刻侧最接近的等价物是触点的抑制/大物体标志；没有该字段时不否决。
    [[nodiscard]] static bool SingleTouchLatchVeto(const TouchBigFrame* bigFrame) noexcept {
        if (bigFrame == nullptr || bigFrame->tracks.empty()) return false;
        return (bigFrame->tracks[0].volatileFlags & 0x200u) != 0;
    }

    /// 换阶段时 `nBaselineStageFrameCount = 1;`
    /// `qwBaselineStageReference = static->nBaselineStageInitialSignal;`
    /// —— 即**规整后的本帧间隔**，不是 0（§复刻清单-4）。
    void RestartStageTimer() noexcept {
        m_state.stageFrameCount = 1;
        m_state.stageReferenceMs = m_state.frameIntervalMs;
    }

    // ── §7.3 BLSM_ProcessStage @0x6BA4D170 ────────────────────────────
    void BLSM_ProcessStage(const int16_t raw[kRows][kCols]) noexcept {
        m_state.baselineUpdatedThisFrame = false;
        m_state.resetDiffRequested = false;
        m_state.baselineWrittenThisFrame = false;

        switch (m_state.stage) {
        case BaselineStage::Normal: {
            int16_t clamp = m_dyn.smallThold;   // 调用方给的初值 DAT_6BCAB6C6
            uint16_t step = 0;
            BLSM_GetNoTouchBlStep(clamp, step);
            BLIIR_Update(raw, /*allowNegOnly=*/false, clamp, static_cast<int16_t>(step));
            m_state.baselineUpdatedThisFrame = true;
            BLSM_SaveNormalBl();
            break;
        }

        case BaselineStage::ResetHw:
        case BaselineStage::ResetNoise:
            m_state.resetDiffRequested = true;
            BLSM_Reset(raw);
            break;

        case BaselineStage::Hold:
            // ⚠ **就地改写**：UpdateStage 先定 3，ProcessStage 再覆盖。
            m_state.stage = (m_state.prevStage == BaselineStage::ResetHw ||
                             m_state.prevStage == BaselineStage::ResetNoise ||
                             m_state.prevStage == BaselineStage::Hold)
                            ? BaselineStage::Normal
                            : m_state.prevStage;
            break;

        case BaselineStage::PostTouchRecover:
        case BaselineStage::LargeRecover:
            // g_BLSMNextEdgeSmallThresh / BLSM_ProcessUseNextSideThold 恒假
            // ⇒ 恒取 (short)DAT_6BCAB6CA = noiseThold − 1 = 599
            BLIIR_Update(raw, /*allowNegOnly=*/false, m_dyn.clampAftTch,
                         static_cast<int16_t>(m_dyn.stepAftTch));
            m_state.baselineUpdatedThisFrame = true;
            break;

        case BaselineStage::NegativeTracking: {
            // 无入口（死代码，按原版记录以备换预置）
            const int32_t v = -static_cast<int32_t>(m_prpt.nSPeakNegative);
            int16_t step = Fixed::Wrap16(Fixed::ShrTowardZero(v, 4));
            const int16_t cap = Fixed::Wrap16(Fixed::ShrTowardZero(m_dyn.smallThold, 2));
            if (step > cap) step = cap;
            BLIIR_Update(raw, /*allowNegOnly=*/true, Fixed::Wrap16(v), step);
            m_state.baselineUpdatedThisFrame = true;
            break;
        }

        case BaselineStage::SmallSignalWait:
        case BaselineStage::TouchActive:
        case BaselineStage::LiftoffDebounce:
            // 基线完全冻结（原版 case 4/5/6 无 case 标签，落进空 default）
            break;
        }

        // 尾部统一收口：每一帧结尾都重算一次 Dif（重置帧除外）
        if (m_state.resetDiffRequested) {
            BLIIR_ResetDiff(m_diff);
        } else {
            BLIIR_CalcDiffCommon(raw, m_bl, m_diff, m_signalReverted);
        }
    }

    // ── §7.3 BLSM_GetNoTouchBlStep @0x6BA4CF9C ────────────────────────
    // ⚠ 动态步长的门控是 `static->field_0x3`（AFE_IsInIDLEMode 的本帧缓存），
    // **不是** `dwProcessingState & 0x10`（后者是 UpdateBfrTch 的加倍门）。
    void BLSM_GetNoTouchBlStep(int16_t& clamp, uint16_t& step) const noexcept {
        step = m_dyn.stepNoTouch;
        // g_BLSMNextEdgeSmallThresh / BLSM_ProcessUseNextSideThold 恒假 ⇒
        // 不改写 clamp，也不切到 stepNoTouchSide。
        if (!m_inputs.afeIdle) {
            return;
        }
        int32_t v = std::max<int32_t>(m_prpt.nSPeakPositive,
                                      -static_cast<int32_t>(m_prpt.nSPeakNegative));
        v = Fixed::ShrTowardZero(v, 4);                       // 向零取整的 /16
        const int32_t cap = Fixed::ShrTowardZero(clamp, 2);   // 向零取整的 clamp/4
        if (v > cap) v = cap;
        step = static_cast<uint16_t>(v);
    }

    // ── §6 BLIIR_Update @0x6BA470E7 ───────────────────────────────────
    void BLIIR_Update(const int16_t raw[kRows][kCols], bool allowNegOnly,
                      int16_t clamp, int16_t step) noexcept {
        const bool negOk = allowNegOnly ||
                           (-static_cast<int32_t>(clamp) <= static_cast<int32_t>(m_state.minSig));
        if (negOk && m_state.maxSig <= clamp) {
            BLIIR_DoUpdate(raw, step);
        }
    }

    // ── §6 BLIIR_DoUpdate @0x6BA47141 ─────────────────────────────────
    //   int d = (int)(uint16)src - (int)(uint16)bl;      ← **零扩展**相减
    //   if (d < step) { if (d <= -step) bl -= step; } else bl += step;
    // 死区是 `(-step, step)` 的**开区间**：d == -step 属于下行（会减）。
    // 写回按 16 位**回绕**，不饱和。
    // 源缓冲：flash[0x69] = 1 ⇒ g_tsaBufPreCMFRaw（复刻里就是本帧 raw）。
    void BLIIR_DoUpdate(const int16_t raw[kRows][kCols], int16_t step) noexcept {
        for (int r = 0; r < kRows; ++r) {
            for (int c = 0; c < kCols; ++c) {
                const int32_t d = Fixed::ZeroExtSub(raw[r][c], m_bl[r][c]);
                if (d < step) {
                    if (d <= -static_cast<int32_t>(step)) {
                        m_bl[r][c] = Fixed::Wrap16(static_cast<int32_t>(m_bl[r][c]) - step);
                    }
                } else {
                    m_bl[r][c] = Fixed::Wrap16(static_cast<int32_t>(m_bl[r][c]) + step);
                }
            }
        }
        m_state.baselineWrittenThisFrame = true;   // static + 0x3EE
    }

    // ── §6 BLSM_Reset @0x6BA4CF23 = BLIIR_Reset + Self_Reset + SS_Reset ─
    // BLIIR_Reset @0x6BA4721F 的三条互斥分支在本机退化：
    //   AFE_IsRawUnifiedToTargetValue() 无通道 ⇒ 取第一支 bl := PreCMF Raw。
    //   第三支（全屏填 flash[0x5FC] = 32767）需要 AFE 已统一到目标值，不可达。
    // Proximity 同步分支：FEAT_PROXIMITY 关闭 ⇒ g_tsaBufProxBL == NULL ⇒ 跳过。
    void BLSM_Reset(const int16_t raw[kRows][kCols]) noexcept {
        static_assert(!Gates::IsOn("FEAT_PROXIMITY"),
                      "Proximity 关闭 ⇒ BLIIR_Reset 的 ProxBL 同步分支不可达");
        std::memcpy(m_bl, raw, sizeof(m_bl));
        m_noisyTimerMs = 0;
        m_state.singleTouchLatch = false;
    }

    // ── §7.3 BLSM_SaveNormalBl @0x6BA4D0C3 ────────────────────────────
    // 仅当 `static[0x3F1] == 0`（水下哨兵）且 `nTouchCount != 0x2000` 时才存。
    // 两个条件在本机恒真 ⇒ 快照每帧都存。
    void BLSM_SaveNormalBl() noexcept {
        static_assert(!Gates::IsOn("UNDERWATER_SENTINEL"),
                      "水下哨兵恒 0 ⇒ SaveNormalBl 的两个门恒真");
        std::memcpy(m_normalBl, m_bl, sizeof(m_normalBl));
    }

    // ── §7.4 SHB 影子基线 BLSM_ShbProcess @0x6BA4C4CC ─────────────────
    void BLSM_ShbProcess(const int16_t raw[kRows][kCols]) noexcept {
        // BLSM_PreProcess @0x6BA4C36B 每帧把 dwShbFlags 清零（更上层调用）
        m_state.shbFlags = 0;

        if (BLSM_IsReset())              m_state.shbState = 0;
        if (BLSM_ShbIsRawChanges(raw))   m_state.shbState = 0;

        if (m_state.shbState == 1) {
            if (m_nowMs >= m_state.shbStateStartMs + 5000) {
                m_state.shbState = 2;
                m_state.shbFlags |= 1u;
            }
        } else if (m_state.shbState == 2) {
            if (m_nowMs >= m_state.shbStateStartMs + 5000) {
                m_state.shbFlags |= 3u;
            }
        } else {
            m_state.shbState = 1;
            m_state.shbFlags |= 1u;
        }

        if ((m_state.shbFlags & 1u) != 0) {
            BLSM_ShbCaptureBl(raw);
        }
    }

    /// BLSM_ShbCaptureBl @0x6BA4C422 —— 拷的是 **PreCMF Raw**，不是 BL（函数名误导）
    void BLSM_ShbCaptureBl(const int16_t raw[kRows][kCols]) noexcept {
        std::memcpy(m_rawCapture, raw, sizeof(m_rawCapture));
        m_state.shbStateStartMs = m_nowMs;
    }

    /// BLSM_ShbIsRawChanges @0x6BA4C382 —— 比的是 `g_tsaBufRaw`（不是 PreCMF）。
    /// 门限 `wPeakMode3ThresholdMax / 2 = 300`；差值先按 int16 截断再取绝对值。
    [[nodiscard]] bool BLSM_ShbIsRawChanges(const int16_t raw[kRows][kCols]) const noexcept {
        const int32_t thold = kPeakMode3ThresholdMax / 2;
        for (int r = 0; r < kRows; ++r) {
            for (int c = 0; c < kCols; ++c) {
                const int16_t d = Fixed::Wrap16(static_cast<int32_t>(m_rawCapture[r][c]) -
                                                static_cast<int32_t>(raw[r][c]));
                const int32_t mag = (d < 0) ? -static_cast<int32_t>(d) : d;
                if (mag > thold) return true;
            }
        }
        return false;
    }

    // ── §9.3 BLSM_UpdateForRawUnstable @0x6BA4C27B ────────────────────
    //   if (!TSA_IsGameScenario() && (static[0xD8] & 0x40000)) { ... }
    //   d = (uint16)preCMFRaw - (uint16)bl;              ← 同样零扩展
    //   if (d < limit) { if (d >= 0x11) bl += 0x11; else if (d < -0x10) bl -= 0x11; }
    // ⚠ 死区是 **[-16, 16] 闭区间**、步长两边都是 17；SafeBaseline 关闭 ⇒
    //   limit = INT_MAX，上限判定恒真。发生在 BLSM_Process **之前**，
    //   且**不更新** bBaselineUpdatedThisFrame。
    //
    // 门控 `static[0xD8] & 0x40000` 是 AFE 帧标志「raw 失稳」，复刻侧没有这条
    // 通道（m_inputs.afeRawUnstable 恒 false）。**不能**把它当常真打开：那等价
    // 于把 AFE 标志硬编码为 1，会让基线在有手指时也以 17 LSB/帧 追赶 Raw，
    // 约 200 帧就把 3300 的触点信号吸进基线。接入 AFE 帧标志后置位即可。
    void BLSM_UpdateForRawUnstable(const int16_t raw[kRows][kCols]) noexcept {
        if (m_inputs.gameScenario || !m_inputs.afeRawUnstable) return;
        static_assert(!Gates::IsOn("FEAT_SAFE_BASELINE"),
                      "SafeBaseline 关闭 ⇒ limit = INT_MAX，上限判定恒真");
        for (int r = 0; r < kRows; ++r) {
            for (int c = 0; c < kCols; ++c) {
                const int32_t d = Fixed::ZeroExtSub(raw[r][c], m_bl[r][c]);
                if (d >= 0x11) {
                    m_bl[r][c] = Fixed::Wrap16(static_cast<int32_t>(m_bl[r][c]) + 0x11);
                } else if (d < -0x10) {
                    m_bl[r][c] = Fixed::Wrap16(static_cast<int32_t>(m_bl[r][c]) - 0x11);
                }
            }
        }
    }

    // ── §9.2 Baseline_Compensation @0x6BA46D00 的回灌 ─────────────────
    // 原版 `bField_0x11a == 0` 时 `bl -= s`，否则 `bl += s`；s = ±5。
    // 本机走反向分支（见文件头）⇒ 取 `bl += s`。步长由模块 02 传入。
    void Baseline_CompensationApply(int16_t step) noexcept {
        const int32_t delta = m_signalReverted ? step : -step;
        for (int r = 0; r < kRows; ++r) {
            for (int c = 0; c < kCols; ++c) {
                m_bl[r][c] = Fixed::Wrap16(static_cast<int32_t>(m_bl[r][c]) + delta);
            }
        }
        m_state.specialControlFlags |= 0x100u;
    }
};

} // namespace Solvers::Touch
