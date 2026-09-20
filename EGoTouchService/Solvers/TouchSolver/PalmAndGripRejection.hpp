#pragma once
// ══════════════════════════════════════════════════════════════════════
// TouchPipeline Stage 6: PalmAndGripRejection（握持抑制 / 触控动作 /
// AntiTouch / BetaGrip / 大鱼际），header-only C++23。
//
// 逐函数对位 TSACore.dll（image base 0x6BA40000）W273AS2700（Gaokun Himax
// CSOT，60 列 × 40 行）。权威规范：docs/finger_algorithms/06_palm_and_grip_rejection.md。
//
// ── 原厂调用序（TSA_MSProcess @0x6BA90E43 的本模块段）────────────────
//   Touch_UnitUpdate → Touch_SetPersistFlag @0x6BA72776   （§2 的产出方）
//   TouchAction_Process        @0x6BA7733C                （§2）
//   TSA_MSTouchPostFilter                                  （§3，空函数）
//   GripFilter_Process         @0x6BAE1D3C                （§4）
//   TouchMode_Process          @0x6BA78DDE                （§5，在 TouchModeFsm）
//   AntiTouch_Process          @0x6BAD7BBB                （§6）
//   Touch_ProcessBetaGrip      @0x6BA76EE8                （§7）
//   Gesture / HandGesture                                  （§8，本机关闭）
//   ── MS 之外 ──
//   Thenar_ProcessInfoByDif    @0x6BB315E8                （§9）
//   TSAOut_PostProcess         @0x6BA6A77A                （模块 08 执行抑制）
//
// ── 本文件相对早期复刻的五处**语义纠正**（均经反编译复核）───────────
//  1. GripFilter 对**每个**触点执行。早先断言「dwGripFlags 整字为 0 ⇒ 不执行」
//     是错的：`GripFilter_Process` 的循环没有任何 grip-flag 前置，握持区非空，
//     边缘触点真的会走 BasicProcess（§4.1）。
//  2. `GripFilter_IsGrip` = stage ∈ **{0,1,2}**，不是 {1,2}。阶段 0 本身算握持，
//     新触点在延时窗口内一律不上报（§4.5）。
//  3. `GripFilter_BackToGrip` **不改阶段**：它把 TE 置 0x20、坐标冻结回上一帧、
//     清历史缓冲，触点由 TE 在后续帧回收（§4.5）。
//  4. 抑制的**执行点在 TSAOut（模块 08）**，本模块只置 `+0x1F0 |= 0x8000000`。
//     Thenar / AntiTouch / BetaGrip 在本机**完全不影响手指报点**，早先的
//     ApplyThenarSuppression / AntiTouch 原位杀点 / ApplyFinalReportingBarrier
//     三处都是多做，已删除（§架构要点、§6、§9）。
//  5. Thenar 只扫**第 0 列与第 59 列**（`Thenar_FeatureExtraction` 的步进是
//     `i += cols - 1`），不是全网格 Sobel（§9）。
//
// ── 依赖 ────────────────────────────────────────────────────────────
//  · TouchFixed：所有移位取整 / 回绕 / 整数开方
//  · TouchGates：配置门控单一事实源
//  · TouchCoreTypes：TouchEventCode / MapToPtp（TE 状态判断唯一入口）
//  · TouchReportAndFilter：读上一帧的三个位移锚点（+0x49A/+0x4A4/+0x4AC）与
//    +0x1F8，以及 RequestPostReport() 入口
// ══════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

#include "SolverTypes.h"
#include "TouchCoreTypes.hpp"
#include "TouchFixed.hpp"
#include "TouchFrameTypes.h"
#include "TouchGates.hpp"
#include "TouchReportAndFilter.hpp"

namespace Config {
class ConfigBinder;
class ConfigStore;
}

namespace Solvers::Touch {

// ── 手侧代码（Thenar_ProcessInfoByDifTraditional 写 hdr + 0x48）────────
enum HandSideCode : uint8_t {
    HandSideUnknown = 0,
    HandSideLeft    = 4,   // 0x04
    HandSideRight   = 8,   // 0x08
};

// ── 握持阶段 touch + 0x210（GripFilter_GetStage @0x6BAE492D）──────────
// GripFilter_IsGrip @0x6BAE193F：**0 / 1 / 2 都算握持**。
enum GripStage : int32_t {
    GripStage0_Pending  = 0, // 新触点/延时窗口内 —— 算握持
    GripStage1_AbsGrip  = 1, // 绝对握持，只能由「安全」离开
    GripStage2_TempGrip = 2, // 暂定握持
    GripStage3_Watch    = 3, // 待定，已放行但密切监视
    GripStage4_Safe     = 4, // 安全
};

/// GripFilter_IsGrip @0x6BAE193F
[[nodiscard]] constexpr bool GripStageIsGrip(int32_t stage) noexcept {
    return stage == 0 || stage == 1 || stage == 2;
}

// ── touch + 0x1F8 持久标志（Touch_SetPersistFlag @0x6BA72776 产出）─────
enum TouchPersistFlags1F8 : uint32_t {
    PF_MIN_SIG_OFF     = 0x00000001u, // ← +0x1F0 & 4
    PF_MOVED_15MM      = 0x00000004u, // +0x24C ≥ ExtToMm(flash 0x920 = 15)²
    PF_REPORTED        = 0x00000008u, // 模块 07 置：曾真的上报过
    PF_HIST_POP        = 0x00000080u, // Touch_CreatePostReport
    PF_MOVED_20PX      = 0x00000100u, // 距 +0x4A4 > ExtToInt(flash 0x904 = 20)²
    PF_STABLE_LATCH    = 0x00000200u, // ← +0x1F0 & 0x1000
    PF_MOVED_50PX      = 0x00001000u, // 距 +0x4AC > ExtToInt(flash 0x908 = 50)²
    PF_CORNER_LATCH    = 0x00002000u, // prev +0x1F0 & 0x400000 ∧ prev +0x4E4 == 10
    PF_BIGSHAPE_LATCH  = 0x00004000u, // ← +0x1F0 & 0x800000
    PF_MOVED_60PX      = 0x00008000u, // 距 +0x49A > ExtToInt(flash 0x906 = 60)²
    PF_POST_REPORT     = 0x00020000u, // Touch_CreatePostReport
    PF_GLOVE_EXCLUDE   = 0x00080000u, // +0x240 > 16 ∨ 异常 ∨ 强峰 ∨ 多指
    PF_CUR_REPORTED    = 0x00100000u, // 模块 07 本帧上报判决
    PF_FORCE_ID_BREAK  = 0x00200000u,
    PF_PROXIMITY_PROT  = 0x00400000u, // AntiTouch IsToProtectProximity
    PF_BETA_GRIP       = 0x01000000u, // Touch_ProcessBetaGrip
    PF_POSTURE_BLOCK   = 0x20000000u,
};

// ── touch + 0x1F0 易失标志 ────────────────────────────────────────────
enum TouchVolatile1F0 : uint32_t {
    VF_FALSE_FLICK   = 0x00100000u, // Touch_SetVolatileFlagFalseFlickDetected（假滑动/手指滚动）
    VF_ANTI_TOUCH    = 0x04000000u, // AntiTouch 异常（主路径无 AND 读者）
    VF_GRIP_SUPPRESS = 0x08000000u, // GripFilter_Report → TSAOut 剔除
};

// ── touch + 0x214 握持通道标志（各检测器产出，文档 06 §10）────────────
enum TouchGrip214 : uint32_t {
    G214_NEW_NEIGHBOR   = 0x00000001u,
    G214_MULTI_GRIP     = 0x00000002u,
    G214_MULTI_REGION   = 0x00000004u,
    G214_NOT_HEAVY      = 0x00000020u,
    G214_OTHER_PRESENT  = 0x00000040u,
    G214_QUALITY1       = 0x00000080u,
    G214_MULTI_WEAK     = 0x00000100u,
    G214_ABS_NOT_GRIP   = 0x00000800u, // ← GripFilter_IsSafeTouch 读的就是它
    G214_FAST_MOVE      = 0x00001000u,
    G214_FLICK_BLOCKED  = 0x00008000u,
    G214_CORNER_DWELL   = 0x00010000u,
    G214_HIGH_PRIORITY  = 0x00020000u,
    G214_EDGE_OR_NARROW = 0x00080000u,
    G214_MERGE_ALLOWED  = 0x00200000u,
    G214_CORNER         = 0x00800000u,
    G214_CORNER_FIRST   = 0x01000000u,
    G214_PRIORITY_CLASH = 0x02000000u,
    G214_GRAB           = 0x04000000u,
    G214_RAW_JUMP       = 0x10000000u, // Touch_SetPersistFlag：age 0 时 AFE 跳变
    G214_ABOVE_GRIP     = 0x20000000u,
    G214_DELAY_TIMEOUT  = 0x40000000u,
};

// ── touch + 0x218 ─────────────────────────────────────────────────────
enum TouchGrip218 : uint32_t {
    G218_MERGED        = 0x01u,
    G218_HEAVY_CORNER  = 0x02u,
    G218_CLOSE_GRIP    = 0x04u,
    G218_INNER_EDGE_PK = 0x08u,
    G218_MERGE_BROKEN  = 0x10u,
};

// ── touch + 0x1FC ─────────────────────────────────────────────────────
enum TouchGrip1FC : uint32_t {
    G1FC_HEAVY_REGION       = 0x04000000u,
    G1FC_HEAVY_CORNER       = 0x08000000u,
    G1FC_HEAVY_CORNER_FIRST = 0x10000000u,
    G1FC_HEAVY_CORNER_OFF   = 0x20000000u,
    G1FC_GRIP_RELEASED      = 0x40000000u, // 曾被握持后又放行，Report 要回填
};

// ── 大鱼际特征片段（hdr + 0x68 起，10 字节 × 20，计数在 hdr + 0x130）──
struct ThenarFeatureUnit {
    int16_t  coord = 0;   // +0x00 沿扫描轴的坐标（特征末端行号）
    uint16_t len = 0;     // +0x02 连续长度（格）
    uint16_t width = 0;   // +0x04 正交方向线宽（格）
    uint8_t  edge = 0;    // +0x06 (col != 59)，1 = 左列
    uint8_t  pad = 0;     // +0x07
    int16_t  peak = 0;    // +0x08 片段内最大 Dif
};

// ══════════════════════════════════════════════════════════════════════
class PalmAndGripRejection {
public:
    static constexpr int kRows = kSensorRows;
    static constexpr int kCols = kSensorCols;
    static constexpr size_t kMaxSlots = kMaxTracks + 1;   // 按 id 1..20 直接索引
    static constexpr size_t kMaxThenarFeatures = 20;

    bool m_enabled = true;

    // ══════════════════════════════════════════════════════════════════
    // 门控（TouchGates 单一事实源，文档 06 §1）
    // ══════════════════════════════════════════════════════════════════
    static constexpr uint32_t kFeat = Gates::kFeatureFlags;
    /// dwFeatureFlags & 0x20：GripFilter 主体（Report / IDLE / Merge）。开。
    static constexpr bool kFeatGrip       = Gates::IsOn("FEAT_GRIP");
    /// dwFeatureFlags & 0x1000000：WindowsPad。GripFilter 的 50 ms 延时来自它，
    /// 参数行的延时字段（0 / 30 / 100）在本机**不被采用**（06-04）。
    static constexpr bool kWindowsPad     = Gates::IsOn("FEAT_WINDOWS_PAD");
    /// flash+0x8D0/0x8DC/0x8E8/0x8F4 四边合并使能全 0 ⇒ MergeHandler 与
    /// TSABuffer_GripStageProcess 永不执行，g_tsaBufGripStage / GripTime 恒 0。
    static constexpr bool kMergeHandlerOn = Gates::IsOn("GRIP_MERGE_HANDLER");
    /// flash+0x8C0 = 0 ⇒ IsInDynamicDeadZone 恒 0，UpdateDeadzone 关闭。
    static constexpr bool kDynDeadZoneOn  = Gates::IsOn("GRIP_DYN_DEADZONE");
    /// flash+0x8C2 = 0 ⇒ g_isOKToIDLE 恒 false。
    static constexpr bool kIdleProcessOn  = Gates::IsOn("GRIP_IDLE");
    /// flash+0x94A = 0 ⇒ NegSig 旁路整族关闭。
    static constexpr bool kNegSigOn       = Gates::IsOn("GRIP_NEG_SIG");
    /// flash+0x60 掩码 = 0 ⇒ AFT（AntiTouch 后报点）关闭。
    static constexpr bool kAftOn          = Gates::IsOn("FEAT_AFT");
    /// flash+0x1741 = 0 ⇒ UDFP 放宽关闭（IsTouchCloseToUdfpArea 恒假）。
    static constexpr bool kUdfpRelaxOn    = Gates::IsOn("UDFP_RELAX");
    /// dwFeatureFlags & 0x4000 / 0x8000 ⇒ Proximity / SideTouch 关闭（§8）。
    static constexpr bool kProximityOn    = Gates::IsOn("FEAT_PROXIMITY");
    static constexpr bool kSideTouchOn    = Gates::IsOn("FEAT_SIDE_TOUCH");
    /// TSAStatic_IsScreenRotated()：本机横屏固定，恒假。
    static constexpr bool kScreenRotated  = false;
    /// GripFilter_IsVideoMode()：场景由宿主下发，本机未启用。
    static constexpr bool kVideoMode      = false;

    static_assert(kFeatGrip,   "dwFeatureFlags bit5 = 1，GripFilter 主体必须执行");
    static_assert(kWindowsPad, "WindowsPad 决定 50 ms 延时；关掉会误用参数表的 0/30/100");
    static_assert(!kProximityOn && !kSideTouchOn, "Proximity / SideTouch 本机关闭");

    // ══════════════════════════════════════════════════════════════════
    // 整定量（tsaprmt-table-reader 实测，flash 偏移 = 运行时 − 0x6BCA8A00）
    // ══════════════════════════════════════════════════════════════════
    /// DAT_6bcab6f8 的三个字节：flash 0x904 / 0x906 / 0x908 = 20 / 60 / 50 外部像素。
    uint8_t  m_clickSuppressPx   = 20;   // → +0x1F8 bit8  （基准 +0x4A4）
    uint8_t  m_maxClickSuppressPx= 60;   // → +0x1F8 bit15 （基准 +0x49A）
    uint8_t  m_flickDistPx       = 50;   // → +0x1F8 bit12 （基准 +0x4AC）
    /// DAT_6bcab708 = flash 0x920 = 15 mm。`+0x24C ≥ 15²` → +0x1F8 bit2。
    uint16_t m_gloveMoveMm       = 15;
    /// DAT_6bcab6f0：+0 默认动作 = flash 0x8FC = 1；+4 超时 = flash 0x900 = 60000 ms。
    uint32_t m_defaultAction     = 1;
    uint32_t m_actionTimeoutMs   = 60000;
    /// DAT_6bca92b8 = flash 0x8B8 = 10：IsFlickDetected 的基准门限。
    uint16_t m_flickBaseThold    = 10;
    /// GripFilter_GetDelayTime：WindowsPad 路径固定 50 ms（0x32）。
    uint32_t m_gripDelayMs       = 50;
    /// AntiTouch：flash 0x168C = 10（接近判定的显示 Y 上限，TSA_RptTouchYCenter 口径）。
    uint8_t  m_proximityYThold   = 10;
    /// AntiTouch_ProcessUnit 的 `3·Y < local_a` 门限，非旋转取 flash+0x5A8(1446) 组。
    uint16_t m_antiProxYLimit    = 1600;
    /// Thenar：flash 0x836 = 30、flash 0x1B = 69 ⇒ LT = 30·16/69 = 6 格。
    uint8_t  m_thenarLenFactor   = 30;
    uint8_t  m_thenarPitch       = 69;
    /// _DAT_6bcab6c4 = 3/4 · prpt+0x2D8 = 450。进入阈值 T/2，峰阈值 5T/6。
    int16_t  m_thenarBaseThold   = 450;
    /// Thenar_FeatureExtractionUnit 的梯度门限与线宽扩散门限。
    int16_t  m_thenarDecayGrad   = -1000;
    int16_t  m_thenarRestartGrad = 1001;
    /// DAT_6bc4a914（= TouchTholdSet::sigThold 稳态 600）：正交线宽扩散门限。
    int16_t  m_thenarWidthThold  = 600;
    /// GrapDetect：3 · flash 0x5B4(wPeakFilterReferenceThreshold = 3000) < +0x1BA · 10。
    int16_t  m_peakFilterRefThold= 3000;
    /// DAT_6bca92ba = flash 0x8BA = 0x00C8 = 200（table-reader 实测）：
    /// TSABuffer_GripStageProcess 要求该格连续被按住超过 200 ms 才允许写黏滞位。
    int16_t  m_gripStageDwellMs  = 200;
    /// TSABuffer_GripStageProcess 里 `touch + 0x234 / +0x236 < 0x80` 这一支。
    /// 两个字段在 Ghidra 里没有写入方，身份未坐实 ⇒ 默认关闭，结构保留。
    static constexpr bool kGripStageSizeGateOn = false;

    /// TS_SQPixelToSQMM @0x6BA7C4A0 的换算常数 DAT_6bcab61c。
    static constexpr uint64_t kPixelToMmK = 1092;
    /// TSAOut_ExtToIntDist @0x6BA6A0B7：d · 65536 / DAT_6bcab604(10922)。
    [[nodiscard]] static constexpr uint32_t ExtToIntDist(uint32_t d) noexcept {
        return (d * 65536u) / 10922u;
    }

    // ── 握持参数表 gripPrmt @0x6BB4F900（4 行 × 0x18 字节，实测）─────────
    // 索引 = GripFilter_GetCenterLevel（重心格 gripRegion 的等级 0..3）。
    // 等级 0 整行为零 ⇒ BasicIsAbsGripPrmt / BasicIsTempGripPrmt 直接跳过。
    struct GripPrmt {
        uint16_t type;        // +0x00 非零才启用
        uint16_t f02;         // +0x02
        uint16_t f04;         // +0x04
        uint16_t delayMs;     // +0x06 非 WindowsPad 的延时（本机不采用）
        uint16_t absMajorMm;  // +0x08 比 +0x25E（长轴 mm 的 EMA）
        uint16_t axisRatio;   // +0x0A 比 +0x267（长短轴比 ×16）
        uint16_t ellipseRate; // +0x0C 比 +0x268（最小椭圆率 EMA）
        uint16_t gripOuter;   // +0x0E 比 +0x26A
        uint16_t gripInner;   // +0x10 比 +0x26C
    };
    static constexpr size_t kGripLevels = 4;
    GripPrmt m_gripPrmt[kGripLevels] = {
        {0,   0,   0,   0,   0,  0,  0,   0,   0},
        {1, 500, 500,   0,  25, 40, 15, 200, 200},
        {2, 500, 500,  30,  20, 40, 15, 200, 200},
        {3, 500, 500, 100,  20, 36, 12, 200, 200},
    };
    /// GripFilter_GetAbsThold @0x6BAE65E3：x + (x >> 2)（放大 1.25 倍）。
    [[nodiscard]] static constexpr uint32_t GetAbsThold(uint32_t x) noexcept {
        return x + (x >> 2);
    }

    // ── 边缘比值阈值表 flash 0x8C8 + edge·0xC（4 边相同，实测）────────
    //   +0 u32 = 0（GetClickGripRatio 的原始值）
    //   +4/+5/+6 u8 = 0xFF / 0xFF / 0xFF
    // GetEdgeClickRatioThold 末尾 `0xFE < v → 0xFFFF`，所以等级 0..2 的阈值
    // 全部退化成 0xFFFF（只有 ratio 饱和成 0xFFFF 的纯边缘细条才通得过），
    // 等级 ≥3 固定 16（文档 06 §4.3）。
    static constexpr uint8_t kEdgeRatioTbl[4][3] = {
        {0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF},
    };

    // ══════════════════════════════════════════════════════════════════
    // 跨帧单元（对位历史池 g_pPrevTouches 的 0x478 单元中本模块用到的字段）
    // ══════════════════════════════════════════════════════════════════
    struct PrevUnit {
        bool     valid = false;
        uint8_t  id = 0;

        int32_t  stage210 = GripStage0_Pending;   // +0x210
        uint32_t f1F8 = 0;                        // +0x1F8
        uint32_t f1FC = 0;                        // +0x1FC
        uint32_t f200 = 0;                        // +0x200
        uint32_t f214 = 0;                        // +0x214

        uint64_t downTsUs = 0;                    // +0x4B8 按下时间戳
        uint16_t downX = 0, downY = 0;            // +0x496/+0x498 按下点
        uint16_t x162 = 0, y164 = 0;              // +0x162/+0x164 上一帧工作坐标
        uint16_t axisMajorMm = 0, axisMinorMm = 0;// +0x188/+0x18A
        uint32_t action224 = 0;                   // +0x224
        int16_t  sig1BA = 0;                      // +0x1BA
        uint16_t iirOuter26A = 0, iirInner26C = 0;// +0x26A/+0x26C
        uint16_t pathSq250 = 0;                   // +0x250 距按下点位移平方（外部像素）
        uint16_t moveSqMm240 = 0;                 // +0x240 上一帧的帧间位移平方（mm²）
        uint16_t cornerDwell4E4 = 0;              // +0x4E4
        uint8_t  antiPersist4E6 = 0;              // +0x4E6
        uint8_t  antiProx4E7 = 0;                 // +0x4E7
        uint16_t betaGrip4E8 = 0;                 // +0x4E8
        uint16_t flickAnchorX4AC = 0;             // +0x4AC/+0x4AE 假滑动基准（本模块维护副本）
        uint16_t flickAnchorY4AE = 0;
    };

    // ── 本帧逐触点的临时量（g_grip 上下文 + GetProperty 的中间结论）────
    struct FrameUnit {
        uint32_t f1F8 = 0;   // 本帧 +0x1F8（已并入历史）
        uint32_t f1FC = 0;
        uint32_t f214 = 0;
        uint32_t f218 = 0;
        uint32_t durationMs = 0;   // +0x158 自按下起的毫秒
        uint16_t moveSqMm240 = 0;  // +0x240 帧间位移平方 mm²
        uint16_t fromDownSqMm24C = 0; // +0x24C 距按下点位移平方 mm²
        uint32_t movePxSq244 = 0;  // +0x244 帧间位移平方（内部 Q8 平方）
        uint16_t accSqMm242 = 0;   // +0x242 加速度模方 mm²
        uint16_t pathSq250 = 0;    // +0x250 距按下点位移平方（外部像素）
        uint8_t  edge = 0;         // DAT_6bc84e24（0 上 1 下 2 左 3 右）
        uint8_t  level = 0;        // GripFilter_GetCenterLevel
        int32_t  quality = 1;      // DAT_6bc84e28
        int32_t  stage = 0;        // +0x210
        uint16_t zoneGripMax199 = 0; // +0x199（TZ 最大握持掩码，复刻用包围盒采样）
    };

    PalmAndGripRejection() noexcept { Reset(); }

    void Reset() noexcept {
        m_prev.fill(PrevUnit{});
        m_gripTimeOff.fill(0);
        m_thenarFeatures.clear();
        m_thenarDetected = false;
        m_thenarSide = HandSideUnknown;
        m_thenarGesture = 0;
        m_thenarGestureStartMs = 0;
        m_sideHandEnterCnt = {15, 15};
        m_sideHandExitCnt  = {0, 0};
        m_antiTouchAbnormalTsUs = 0;
        m_antiTouchNumGrip = 0;
        m_proximityDetected = false;
        m_flickTestFailed = false;
        m_betaGripLeftCnt = 0;
        m_betaGripRightCnt = 0;
        m_gripEnterCnt = 0;
        m_postReportMask = 0;
        m_gripSuppressedCount = 0;
    }
    void ClearLiveState() noexcept { Reset(); }

    void SetEnabled(bool e) noexcept { m_enabled = e; }
    [[nodiscard]] bool IsEnabled() const noexcept { return m_enabled; }

    /// 由 TouchPipeline 在构造后注入：读上一帧的位移锚点、并转发
    /// Touch_CreatePostReport 请求（模块 07 的 RequestPostReport 入口）。
    void SetReportFilter(TouchReportAndFilter* rf) noexcept { m_reportFilter = rf; }

    // ── 导出查询（对位 TSA_GetThenar* / TSA_IsThenarDetected / BetaGrip）──
    [[nodiscard]] bool IsThenarDetected() const noexcept { return m_thenarDetected; }
    [[nodiscard]] uint8_t GetThenarSide() const noexcept { return m_thenarSide; }
    [[nodiscard]] uint8_t GetThenarGesture() const noexcept { return m_thenarGesture; }
    [[nodiscard]] bool IsFlickFailed() const noexcept { return m_flickTestFailed; }
    [[nodiscard]] uint32_t GetBetaGripLeftCount() const noexcept { return m_betaGripLeftCnt; }
    [[nodiscard]] uint32_t GetBetaGripRightCount() const noexcept { return m_betaGripRightCnt; }
    [[nodiscard]] uint8_t GetGripSuppressedCount() const noexcept { return m_gripSuppressedCount; }
    [[nodiscard]] bool IsProximityDetected() const noexcept { return m_proximityDetected; }
    void SetProximityDetected(bool d) noexcept { m_proximityDetected = d; }
    [[nodiscard]] int32_t GetSlotStage(uint8_t id) const noexcept {
        return (id < kMaxSlots) ? m_prev[id].stage210 : GripStage0_Pending;
    }
    [[nodiscard]] std::span<const ThenarFeatureUnit> GetThenarFeatures() const noexcept {
        return m_thenarFeatures;
    }

    // ══════════════════════════════════════════════════════════════════
    // 主入口
    // ══════════════════════════════════════════════════════════════════
    bool Process(TouchBigFrame& bigFrame) {
        if (!m_enabled) return true;

        const uint64_t nowUs = bigFrame.timestampUs;

        // 帧头字段每帧由 Touch_Clean 清零
        bigFrame.gripSuppressedCount = 0;
        bigFrame.antiTouchHeaderFlags = 0;
        m_gripSuppressedCount = 0;
        m_postReportMask = 0;

        BuildFrameUnits(bigFrame, nowUs);

        // §2 产出方：Touch_SetPersistFlag @0x6BA72776
        // 原厂调用点在 Touch_UnitUpdate @0x6BA762DF（模块 05 的 TE 收尾）。复刻
        // 把它放在模块 06 的最前面：所有消费者（TouchAction、GripFilter、
        // TouchMode_GloveDetect、模块 07 的冻结分支）都排在其后，语义等价。
        for (auto& t : bigFrame.tracks) Touch_SetPersistFlag(t, bigFrame);

        ProcessTouchAction(bigFrame);       // §2
        TSA_MSTouchPostFilter();            // §3（空函数）
        GripFilter_Process(bigFrame);       // §4
        // §5 TouchMode_Process 由 TouchPipeline::RunTouchModeFsm 紧随其后调用。
        AntiTouch_Process(bigFrame, nowUs); // §6
        Touch_ProcessBetaGrip(bigFrame);    // §7
        TouchAction_PostProcess(bigFrame);  // §2
        // §8 Gesture / HandGesture / SideTouch / Proximity 本机关闭。

        LatchPrevUnits(bigFrame, nowUs);
        bigFrame.gripSuppressedCount = m_gripSuppressedCount;
        DispatchPostReports();
        return true;
    }

    /// Thenar_ProcessInfoByDif @0x6BB315E8 —— MS 之外、TSAOut 之前。
    /// 判决只走导出接口与自身跨帧回路，**不进手指报点路径**。
    void ProcessThenarPostReport(TouchBigFrame& bigFrame) {
        if (!m_enabled) return;
        Thenar_ProcessInfoByDifTraditional(bigFrame);
        Thenar_EventUpdate(bigFrame);
    }

    bool Process(HeatmapFrame& frame) {
        if (frame.touch.bigFrame == nullptr) return true;
        return Process(*frame.touch.bigFrame);
    }

    void registerBindings(Config::ConfigBinder& binder) { (void)binder; }
    void applyConfig(const Config::ConfigStore& store) { (void)store; }

private:
    // ── 跨帧状态 ──────────────────────────────────────────────────────
    std::array<PrevUnit, kMaxSlots>  m_prev{};
    std::array<FrameUnit, kMaxSlots> m_cur{};
    TouchReportAndFilter* m_reportFilter = nullptr;

    /// g_tsaBufGripTimeOff：TSABuffer_GripStageProcess 维护的「该格离开握持多久」。
    /// 全库除该函数外无读者，故不发布到共享大帧，留在本模块内。
    std::array<int16_t, static_cast<size_t>(kRows) * kCols> m_gripTimeOff{};

    std::vector<ThenarFeatureUnit> m_thenarFeatures;
    bool     m_thenarDetected = false;
    uint8_t  m_thenarSide = HandSideUnknown;
    uint8_t  m_thenarGesture = 0;          // hdr[0x45]
    uint8_t  m_thenarGestureRaw = 0;       // hdr[0x44]
    uint64_t m_thenarGestureStartMs = 0;   // hdr[0x58]
    std::array<uint32_t, 2> m_sideHandEnterCnt{15, 15};  // g_sideHandEnterCnt，初值 15
    std::array<uint32_t, 2> m_sideHandExitCnt{0, 0};     // g_sideHandExitCnt
    int32_t  m_thenarFrameSide = 0;        // hdr + 0x48

    uint64_t m_antiTouchAbnormalTsUs = 0;  // g_antiTouchAbnormalTimeStamp
    uint8_t  m_antiTouchNumGrip = 0;       // g_antiTouchNumGrip
    int16_t  m_antiTouchErMax = 0;         // g_antiTouchIirEllipseRateMax
    bool     m_proximityDetected = false;

    bool     m_flickTestFailed = false;    // g_flickTestFailed（永不清零）
    uint32_t m_betaGripLeftCnt = 0;        // DAT_6bb4f2b0
    uint32_t m_betaGripRightCnt = 0;       // DAT_6bb4f2b4
    int8_t   m_gripEnterCnt = 0;           // DAT_6bc84e22
    uint8_t  m_gripSuppressedCount = 0;    // hdr[0x0D]
    uint32_t m_postReportMask = 0;         // Touch_CreatePostReport 请求位图

    // ══════════════════════════════════════════════════════════════════
    // 帧前准备：把原厂散在 Touch_* 里的派生量算出来挂到 FrameUnit 上
    // ══════════════════════════════════════════════════════════════════
    void BuildFrameUnits(const TouchBigFrame& bigFrame, uint64_t nowUs) {
        for (auto& u : m_cur) u = FrameUnit{};

        for (const auto& t : bigFrame.tracks) {
            if (t.id == 0 || t.id >= kMaxSlots) continue;
            FrameUnit& c = m_cur[t.id];
            PrevUnit& p = m_prev[t.id];

            if (!p.valid) {                    // PrevTouch_UnitRelease 后的首帧
                p = PrevUnit{};
                p.id = t.id;
                p.downTsUs = nowUs;
                p.downX = t.curX_q8;
                p.downY = t.curY_q8;
                p.x162 = t.curX_q8;
                p.y164 = t.curY_q8;
                p.flickAnchorX4AC = t.curX_q8;
                p.flickAnchorY4AE = t.curY_q8;
            }

            c.f1F8 = t.persistFlags1F8;
            c.f1FC = 0;
            c.f214 = t.gripChannelFlags;
            c.f218 = t.persistFlags;

            c.durationMs = static_cast<uint32_t>((nowUs - p.downTsUs) / 1000ull);

            // +0x240：帧间位移平方（mm²）；+0x24C：距按下点位移平方（mm²）
            c.moveSqMm240 = SqPixelToSqMm(SqDistQ8(t.curX_q8, t.curY_q8, p.x162, p.y164));
            c.fromDownSqMm24C = SqPixelToSqMm(SqDistQ8(t.curX_q8, t.curY_q8, p.downX, p.downY));
            c.movePxSq244 = SqDistQ8(t.curX_q8, t.curY_q8, p.x162, p.y164);

            // +0x242：加速度（本帧位移向量 − 上一帧位移向量）的模方，mm²。
            // Touch_GetSQAccMMFromPrevPos @0x6BA7380C 的等价：模块 05 已把
            // 加速度分量发布到 ax_q8 / ay_q8。
            {
                const int64_t ax = t.ax_q8, ay = t.ay_q8;
                c.accSqMm242 = SqPixelToSqMm(static_cast<uint32_t>(
                    std::min<int64_t>(ax * ax + ay * ay, 0xFFFFFFFFll)));
            }

            // +0x250：距按下点位移平方，**外部（显示）像素**口径。
            // IsFlickDetected 与 TouchAction_PostProcess 都读它。
            {
                const int32_t dx = ToDisplayDim1(t.curX_q8) - ToDisplayDim1(p.downX);
                const int32_t dy = ToDisplayDim2(t.curY_q8) - ToDisplayDim2(p.downY);
                const int32_t sq = dx * dx + dy * dy;
                c.pathSq250 = static_cast<uint16_t>(std::min<int32_t>(sq, 0xFFFF));
            }

            c.level = static_cast<uint8_t>(std::min<uint32_t>(t.gripLevel, kGripLevels - 1));
            c.edge = GripFilter_GetEdge(t);
            c.zoneGripMax199 = SampleZoneGripMax(bigFrame, t);
            c.stage = p.stage210;
        }
    }

    /// 释放消失的轨迹（PrevTouch_UnitRelease），并把本帧结论回灌历史池。
    void LatchPrevUnits(const TouchBigFrame& bigFrame, uint64_t nowUs) {
        std::array<bool, kMaxSlots> alive{};
        for (const auto& t : bigFrame.tracks) {
            if (t.id == 0 || t.id >= kMaxSlots) continue;
            alive[t.id] = true;
            PrevUnit& p = m_prev[t.id];
            const FrameUnit& c = m_cur[t.id];
            p.valid = true;
            p.id = t.id;
            p.stage210 = c.stage;
            p.f1F8 = c.f1F8;
            p.f1FC = c.f1FC;
            p.f214 = c.f214;
            p.x162 = t.curX_q8;
            p.y164 = t.curY_q8;
            p.axisMajorMm = t.axisMajorMm;
            p.axisMinorMm = t.axisMinorMm;
            p.action224 = t.actionWord;
            p.sig1BA = t.minSigLatched;
            p.iirOuter26A = t.iirGripRatioOuter;
            p.iirInner26C = t.iirGripRatioInner;
            p.pathSq250 = std::max(p.pathSq250, c.pathSq250);
            p.moveSqMm240 = c.moveSqMm240;
            (void)nowUs;
        }
        for (size_t i = 0; i < kMaxSlots; ++i) {
            if (!alive[i]) m_prev[i] = PrevUnit{};
        }
    }

    /// 把本帧攒下的 Touch_CreatePostReport 请求交给模块 07。
    void DispatchPostReports() noexcept {
        if (m_reportFilter == nullptr || m_postReportMask == 0) { m_postReportMask = 0; return; }
        for (uint8_t id = 1; id < 32; ++id) {
            if ((m_postReportMask & (1u << id)) != 0) m_reportFilter->RequestPostReport(id);
        }
        m_postReportMask = 0;
    }

    void RequestPostReport(uint8_t id) noexcept {
        if (id > 0 && id < 32) m_postReportMask |= (1u << id);
    }

    // ── 定点小工具（全部走 TouchFixed，禁止手写移位取整）──────────────
    [[nodiscard]] static uint32_t SqDistQ8(int32_t x0, int32_t y0, int32_t x1, int32_t y1) noexcept {
        const int64_t dx = x0 - x1, dy = y0 - y1;
        return static_cast<uint32_t>(std::min<int64_t>(dx * dx + dy * dy, 0xFFFFFFFFll));
    }
    /// TS_SQPixelToSQMM @0x6BA7C4A0：(K · sq · K) >> 32，K = DAT_6bcab61c = 1092。
    [[nodiscard]] static uint16_t SqPixelToSqMm(uint32_t sq) noexcept {
        const uint64_t mm2 = (kPixelToMmK * static_cast<uint64_t>(sq) * kPixelToMmK) >> 32;
        return static_cast<uint16_t>(std::min<uint64_t>(mm2, 0xFFFFu));
    }
    [[nodiscard]] static int32_t ToDisplayDim1(uint16_t q8) noexcept {
        return (static_cast<int32_t>(q8) * kDisplayWidth) / (kCols * 256);
    }
    [[nodiscard]] static int32_t ToDisplayDim2(uint16_t q8) noexcept {
        return (static_cast<int32_t>(q8) * kDisplayHeight) / (kRows * 256);
    }
    /// Touch_IsOff：TE 已进入抬起报告（0x20）或正常抬起（0x40）。
    [[nodiscard]] static bool Touch_IsOff(const TrackingSlot& t) noexcept {
        return t.state == EventUpFinal || t.state == EventUpReport;
    }

    /// unit + 0x32A（TZ 覆盖到的最大 gripRegion 掩码）的复刻替代：
    /// TrackingSlot 不带区域 id，改为在包围盒范围内采样本帧 gripRegion 取最大。
    /// 语义等价于「这条轨迹所在区域是否碰到握持区」，SafeTouchDetect 第一条读它。
    [[nodiscard]] static uint16_t SampleZoneGripMax(const TouchBigFrame& bf,
                                                    const TrackingSlot& t) noexcept {
        const int cx = std::clamp<int>(t.curX_q8 >> 8, 0, kCols - 1);
        const int cy = std::clamp<int>(t.curY_q8 >> 8, 0, kRows - 1);
        const int hx = std::max<int>(0, t.extentX / 2);
        const int hy = std::max<int>(0, t.extentY / 2);
        uint16_t m = 0;
        for (int r = std::max(0, cy - hy); r <= std::min(kRows - 1, cy + hy); ++r) {
            for (int c = std::max(0, cx - hx); c <= std::min(kCols - 1, cx + hx); ++c) {
                m = std::max(m, bf.gripRegion[r][c]);
            }
        }
        return m;
    }

    // ══════════════════════════════════════════════════════════════════
    // §2 Touch_SetPersistFlag @0x6BA72776
    // ══════════════════════════════════════════════════════════════════
    // 把七条「一旦成立就永久保留」的判据落在 +0x1F8 / +0x214 上，最后把历史
    // 的 +0x1F8 / +0x1FC / +0x200 / +0x214 **按位 OR** 合并进当前单元。
    //
    // 这一函数是全模块的**关键产出方**：
    //   bit8 (0x100)  → 模块 07 冻结分支 2 的前置、TouchAction 升 2 的唯一触发
    //   bit12 (0x1000)→ 模块 07 冻结分支 3 的豁免、TouchAction_PostProcess 的继承
    //   bit15 (0x8000)→ 模块 07 的 +0x279 快速移动锁存（α 下限 8 / 折返检测前置）
    //   bit2  (4)     → TouchMode_GloveDetect 的手套闩锁前置
    //   bit19 (0x80000)→ 同上的排除位
    void Touch_SetPersistFlag(TrackingSlot& t, const TouchBigFrame& bigFrame) {
        if (t.id == 0 || t.id >= kMaxSlots) return;
        FrameUnit& c = m_cur[t.id];
        const PrevUnit& p = m_prev[t.id];

        // (1) +0x1F0 & 4 → +0x1F8 |= 1
        if ((t.volatileFlags & 0x4u) != 0) c.f1F8 |= PF_MIN_SIG_OFF;

        // (2) +0x24C ≥ DAT_6bcab708²（flash 0x920 = 15 mm²）→ +0x1F8 |= 4
        {
            const uint32_t thr = static_cast<uint32_t>(m_gloveMoveMm) * m_gloveMoveMm;
            if (static_cast<uint32_t>(c.fromDownSqMm24C) >= thr) c.f1F8 |= PF_MOVED_15MM;
        }

        // (3) +0x1F0 & 0x1000 → +0x1F8 |= 0x200
        if ((t.volatileFlags & 0x1000u) != 0) c.f1F8 |= PF_STABLE_LATCH;

        // (4) 三条位移判据的共同前置：上一帧真的上报过（+0x1F8 & 8）且
        //     prev +0x1E8 != 6。复刻的 +0x1E8（上一帧区域类型）无产出方，
        //     按「非 6」处理（原版 6 是被撤销的姿态型，本机不产生）。
        if ((p.f1F8 & PF_REPORTED) != 0) {
            Touch_SetVolatileFlagFalseFlickDetected(t, c);
            Touch_SetPersistFlagMoveOverClickSuppressRange(t, c);
            Touch_SetPersistFlagMoveOverMaxClickSuppressRange(t, c);
            Touch_SetPersistFlagMoveOverFlickDistance(t, c);
        }

        // (5) +0x1F0 & 0x800000 → +0x1F8 |= 0x4000
        if ((t.volatileFlags & 0x800000u) != 0) c.f1F8 |= PF_BIGSHAPE_LATCH;

        // (6) prev +0x1F0 & 0x400000 且 prev +0x4E4 == 10 → +0x1F8 |= 0x2000
        if ((t.volatileFlags & 0x400000u) != 0 && p.cornerDwell4E4 == 10) {
            c.f1F8 |= PF_CORNER_LATCH;
        }

        // (7) 手套排除位 0x80000：强峰 / 有异常 / 位移过大 / 多触点
        {
            const int32_t peakMax2 = static_cast<int32_t>(bigFrame.tholds.peakThold) * 2;
            if (c.moveSqMm240 > 16 || t.exceptionType != 0 ||
                static_cast<int32_t>(t.peakSig) > peakMax2 ||
                bigFrame.tracks.size() > 1) {
                c.f1F8 |= PF_GLOVE_EXCLUDE;
            }
        }

        // (8) age 0（prev +0x156 == 0）时出现 AFE 原始跳变 → +0x214 |= 0x10000000。
        //     AFE_IsRawJump 的产出方在模块 01，复刻侧没有这条通道 ⇒ 恒假，保留结构。

        // (9) 历史持久字按位 OR 合并（持久位只置不清）
        c.f1F8 |= p.f1F8;
        c.f1FC |= p.f1FC;
        c.f214 |= p.f214;

        t.persistFlags1F8 = c.f1F8;
        t.gripChannelFlags = c.f214;
    }

    /// Touch_SetVolatileFlagFalseFlickDetected @0x6BA72ADB（+0x1F0 |= 0x100000）
    /// 「手指在原地滚动」的检测：帧间位移平方超过 TS_MMToPixel(1)²/2，且
    /// 长短轴的**变化量**折算成像素平方后的 1/4 仍然大于该位移 ⇒ 判定这不是
    /// 真正的滑动，而是接触面形变。命中时把点击抑制基准 +0x4A4 重锚到当前点、
    /// 把假滑动基准 +0x4AC 按帧间位移平移，并置 +0x1F0 |= 0x100000。
    /// 该位是模块 07 冻结分支 2 的另一个输入（此前复刻侧无产出方）。
    void Touch_SetVolatileFlagFalseFlickDetected(TrackingSlot& t, FrameUnit& c) noexcept {
        PrevUnit& p = m_prev[t.id];
        // TS_MMToPixel(1) = 1 mm = 60 Q8（DAT_6bcab61a）。W169/W189 的 2 mm 分支
        // 需要 dwSystemFlags & 0x20 且项目名匹配，本机项目是 W273 ⇒ 取 1 mm。
        const uint32_t px = 60;
        if (((px * px) >> 1) >= c.movePxSq244) return;

        const int32_t dMaj = std::abs(static_cast<int32_t>(t.axisMajorMm) -
                                      static_cast<int32_t>(p.axisMajorMm));
        const int32_t dMin = std::abs(static_cast<int32_t>(t.axisMinorMm) -
                                      static_cast<int32_t>(p.axisMinorMm));
        // TS_SQMMToSQPixel：SqPixelToSqMm 的逆，(sq << 32) / K²。
        const uint32_t sqMm = static_cast<uint32_t>((dMaj * dMaj + dMin * dMin) & 0xFFFF);
        const uint64_t sqPx64 = (static_cast<uint64_t>(sqMm) << 32) / (kPixelToMmK * kPixelToMmK);
        const uint32_t sqPx = static_cast<uint32_t>(std::min<uint64_t>(sqPx64, 0xFFFFFFFFull));

        if (c.movePxSq244 >= (sqPx >> 2)) return;

        // prev +0x4A4/+0x4A6 = 当前工作坐标；prev +0x4AC/+0x4AE += 帧间位移
        p.flickAnchorX4AC = Fixed::WrapU16(static_cast<int32_t>(p.flickAnchorX4AC) +
                                           (static_cast<int32_t>(t.curX_q8) - p.x162));
        p.flickAnchorY4AE = Fixed::WrapU16(static_cast<int32_t>(p.flickAnchorY4AE) +
                                           (static_cast<int32_t>(t.curY_q8) - p.y164));
        t.volatileFlags |= VF_FALSE_FLICK;
    }

    /// Touch_SetPersistFlagMoveOverClickSuppressRange @0x6BA72E6D
    void Touch_SetPersistFlagMoveOverClickSuppressRange(const TrackingSlot& t, FrameUnit& c) noexcept {
        const auto* s = PrevReportState(t.id);
        if (s == nullptr) return;
        const uint32_t thr = ExtToIntDist(m_clickSuppressPx);
        if (SqDistQ8(t.curX_q8, t.curY_q8, s->clickBaseX4A4, s->clickBaseY4A6) > thr * thr) {
            c.f1F8 |= PF_MOVED_20PX;
        }
    }

    /// Touch_SetPersistFlagMoveOverMaxClickSuppressRange @0x6BA72F1A
    void Touch_SetPersistFlagMoveOverMaxClickSuppressRange(const TrackingSlot& t, FrameUnit& c) noexcept {
        const auto* s = PrevReportState(t.id);
        if (s == nullptr) return;
        const uint32_t thr = ExtToIntDist(m_maxClickSuppressPx);
        if (SqDistQ8(t.curX_q8, t.curY_q8, s->startRptX49A, s->startRptY49C) > thr * thr) {
            c.f1F8 |= PF_MOVED_60PX;
        }
    }

    /// Touch_SetPersistFlagMoveOverFlickDistance @0x6BA72FC7
    void Touch_SetPersistFlagMoveOverFlickDistance(const TrackingSlot& t, FrameUnit& c) noexcept {
        const auto* s = PrevReportState(t.id);
        if (s == nullptr) return;
        const uint32_t thr = ExtToIntDist(m_flickDistPx);
        if (SqDistQ8(t.curX_q8, t.curY_q8, s->flickBaseX4AC, s->flickBaseY4AE) > thr * thr) {
            c.f1F8 |= PF_MOVED_50PX;
        }
    }

    /// 上一帧的三个锚点由模块 07 的 TouchReport_UpdateStartReportPoint 维护。
    [[nodiscard]] const TouchReportAndFilter::SlotFilterState* PrevReportState(uint8_t id) const noexcept {
        if (m_reportFilter == nullptr) return nullptr;
        const auto* s = m_reportFilter->GetSlotState(id);
        return (s != nullptr && s->valid) ? s : nullptr;
    }

    // ══════════════════════════════════════════════════════════════════
    // §2 TouchAction（动作类型 +0x224）
    // ══════════════════════════════════════════════════════════════════
    // 只有 1（默认）与 2（已移动 / 超时）两个值，**不是 Tap/Hold 语义枚举**，
    // 且只喂 EventAnalyzer_GetEventValue，不影响报点（06-07）。
    // 超时门限 60000 ms 实际不可能到达，真正的唯一触发是 +0x1F8 & 0x100。
    void ProcessTouchAction(TouchBigFrame& bigFrame) {
        for (auto& t : bigFrame.tracks) {
            if (t.id == 0 || t.id >= kMaxSlots) continue;
            if ((static_cast<uint32_t>(t.state) & 0x20u) != 0) continue;  // TE & 0x20 跳过
            TouchAction_ProcessUnit(t);
        }
    }

    /// TouchAction_ProcessUnit @0x6BA77226
    void TouchAction_ProcessUnit(TrackingSlot& t) noexcept {
        const PrevUnit& p = m_prev[t.id];
        const FrameUnit& c = m_cur[t.id];
        uint32_t a = p.action224;
        if (a == 0) {
            a = m_defaultAction;                       // DAT_6bcab6f0 +0
        } else if (a != 2 &&
                   (((c.f1F8 & PF_MOVED_20PX) != 0) || c.durationMs > m_actionTimeoutMs)) {
            a = 2;
        }
        t.actionWord = a;
    }

    /// TouchAction_PostProcess @0x6BA773EB
    /// 抬起帧（TE & 0x20 且 +0x1F8 & 8）决定**下一个新触点**的默认动作，并在
    /// 「未建立回放且位移不足 20 外部像素」时锁死 g_flickTestFailed。
    void TouchAction_PostProcess(const TouchBigFrame& bigFrame) noexcept {
        for (const auto& t : bigFrame.tracks) {
            if (t.id == 0 || t.id >= kMaxSlots) continue;
            const FrameUnit& c = m_cur[t.id];
            if ((static_cast<uint32_t>(t.state) & 0x20u) == 0) continue;
            if ((c.f1F8 & PF_REPORTED) == 0) continue;
            m_defaultAction = ((c.f1F8 & PF_MOVED_50PX) != 0) ? 2u : 1u;
            if ((c.f1F8 & PF_HIST_POP) == 0 && c.pathSq250 < 20) {
                m_flickTestFailed = true;   // 永不清零，只有 TSA_IsFlickCheckPassed 读
            }
        }
    }

    // ══════════════════════════════════════════════════════════════════
    // §3 TSA_MSTouchPostFilter @0x6BA90xxx —— 空函数，直接返回（06-08）
    // ══════════════════════════════════════════════════════════════════
    static void TSA_MSTouchPostFilter() noexcept {}

    // ══════════════════════════════════════════════════════════════════
    // §4 GripFilter
    // ══════════════════════════════════════════════════════════════════
    /// GripFilter_Process @0x6BAE1D3C
    void GripFilter_Process(TouchBigFrame& bigFrame) {
        GripFilter_GetPropertyGlobal(bigFrame);

        for (auto& t : bigFrame.tracks) {
            if (t.id == 0 || t.id >= kMaxSlots) continue;
            // 唯一的前置条件：+0x1F8 & 0x200000（回放期间强制断 ID）
            if ((m_cur[t.id].f1F8 & PF_FORCE_ID_BREAK) != 0) continue;
            GripFilter_BasicProcess(t, bigFrame);
        }

        if constexpr (kFeatGrip) {
            // 反汇编 0x6BAE1DCD..0x6BAE1DD6：
            //     CALL GripFilter_IsMergeHandlerEnabled   ; 0x6BAE18C4
            //     TEST AL,AL
            //     JZ   0x6BAE1DDB                         ; 为 0 则跳过
            //     CALL TSABuffer_GripStageProcess         ; 0x6BA8EF57
            // 即 GripStageProcess **受 IsMergeHandlerEnabled 门控**。
            // Ghidra 的 xref 类型 UNCONDITIONAL_CALL 只说明这条 CALL 指令本身
            // 不是条件调用指令，不代表它没有被外层分支包住 —— 不要据此判定它
            // 「无条件执行」。
            if constexpr (kMergeHandlerOn) TSABuffer_GripStageProcess(bigFrame);
            GripFilter_Report(bigFrame);
            // GripFilter_IDLEProcess @0x6BAE1C75：flash 0x8C2 = 0 ⇒ 空转。
            if constexpr (kIdleProcessOn) { /* GripFilter_IDLEProcess */ }
        }
    }

    /// GripFilter_GetPropertyGlobal @0x6BAE990D —— 先对全部触点跑一遍全局属性。
    void GripFilter_GetPropertyGlobal(TouchBigFrame& bigFrame) {
        for (auto& t : bigFrame.tracks) {
            if (t.id == 0 || t.id >= kMaxSlots) continue;
            FrameUnit& c = m_cur[t.id];

            // +0x1C4 = TSA_GetTouchSide（1 上 / 2 下 / 4 左 / 8 右），模块 04 已产出
            c.edge = GripFilter_GetEdge(t);

            // GripFilter_GetShapeFlag：只读 +0x228 / +0x267，产出 DAT_6bc84e2c，
            // 仅供 GetEdgeClickRatioThold 用；本复刻在那里就地计算。

            // age < 4 且 IsInMultiGripRegion → +0x214 |= 4
            if (t.age < 4 && GripFilter_IsInMultiGripRegion(t, c)) {
                c.f214 |= G214_MULTI_REGION;
            }

            // GripFilter_IsGripMergeAllowed @0x6BAE1EDA = hdr[8] − hdr[10] < 1。
            // **本机恒真**：两个计数只在 `touch + 0x1F4 & 0x1000` 分支里自增，
            // 而全库没有任何代码给 +0x1F4 置 0x1000 ⇒ 0 − 0 = 0 < 1 恒成立（§4.2）。
            constexpr bool kGripMergeAllowed = true;
            if constexpr (!kGripMergeAllowed) {
                // 死路径，结构保留
                if ((c.f218 & G218_MERGED) != 0 || (c.f214 & G214_MERGE_ALLOWED) != 0) {
                    c.f218 |= G218_MERGE_BROKEN;
                    c.f214 &= ~G214_ABS_NOT_GRIP;
                }
                c.f218 &= ~G218_MERGED;
                c.f214 &= ~G214_MERGE_ALLOWED;
            } else if ((c.f218 & G218_MERGED) != 0 && !Touch_IsOff(t)) {
                // GripFilter_IsPeakAbsNotGrip(峰标志 0x800)：复刻侧 TrackingSlot
                // 不带源峰下标，该位无产出方 ⇒ 分支不可达，结构保留。
            }

            // 中心等级 != 0 或 +0x1CC < 2 · +0x1D0 → +0x214 |= 0x80000
            // +0x1D0（区域外壳信号和）在复刻的 TrackingSlot 上没有对应字段，
            // 只落地第一条；第二条留结构。
            if (c.level != 0) c.f214 |= G214_EDGE_OR_NARROW;

            // 未离开、+0x1AC == 1、+0x1AD > 2 且 Peak_IsOnInnerEdgeDim1 → +0x218 |= 8。
            // +0x1AC/+0x1AD（峰在维内的序号与个数）无产出方 ⇒ 不可达，保留结构。

            t.gripChannelFlags = c.f214;
            t.persistFlags = c.f218;
        }
    }

    // ══════════════════════════════════════════════════════════════════
    // §4.7 逐格握持记忆场 —— TSABuffer_GripStageProcess @0x6BA8EF57
    // ══════════════════════════════════════════════════════════════════
    // ⚠ 可达性：唯一调用点是 GripFilter_Process @0x6BAE1DD6，**被
    //   GripFilter_IsMergeHandlerEnabled() 门控**（见 GripFilter_Process 里的
    //   反汇编注释）。该门 = 四条边的 `flash[0x8D0 + edge·0xC] & 2` 之或
    //   （GripFilter_IsMergeHandlerEdgeEnabled @0x6BAE1763，
    //    DAT_6bca92d0 − 0x6BCA8A00 = 0x8D0）。W273AS2700 实测：
    //       flash 0x8D0 / 0x8DC / 0x8E8 / 0x8F4 = 00 00 00 00（四条全 0）
    //   ⇒ 本机整个函数不执行，g_tsaBufGripStage / GripTime / GripTimeOff 恒 0。
    //   代码按原厂语义完整保留（Kind::DeadBranch 的惯例），换整定表即可生效。
    void TSABuffer_GripStageProcess(TouchBigFrame& bigFrame) {
        TSABuffer_GripTimeProcess(bigFrame);

        // local_128[256] / local_228[256]：按 zoneId 索引的两张栈表
        std::array<uint8_t, 256> isGrip{};
        std::array<uint8_t, 256> excluded{};

        for (const auto& t : bigFrame.tracks) {
            if (t.id == 0 || t.id >= kMaxSlots) continue;
            if (Touch_IsOff(t)) continue;
            // touch + 0x28C：该触点所属 TZ 区域号。复刻的 TrackingSlot 不带
            // 区域标号，改为在重心处采样本帧 touchZones，语义等价。
            const int cx = std::clamp<int>(t.curX_q8 >> 8, 0, kCols - 1);
            const int cy = std::clamp<int>(t.curY_q8 >> 8, 0, kRows - 1);
            const uint8_t z = bigFrame.touchZones[cy][cx];

            const FrameUnit& c = m_cur[t.id];
            if (c.stage == GripStage1_AbsGrip) isGrip[z] = 1;
            if ((c.f214 & 0x200u) != 0 && t.age == 1) isGrip[z] = 1;
            // touch + 0x234 / +0x236 < 0x80。这两个字段在 Ghidra 里追不到写入方
            // （只在 TZ_RegisterTouch 的整块拷贝里出现），身份未坐实，默认关闭；
            // 打开前必须先确认它到底是触点尺寸还是别的距离量。
            if constexpr (kGripStageSizeGateOn) {
                if (t.axisMajorMm < 0x80 || t.axisMinorMm < 0x80) isGrip[z] = 1;
            }
            if (t.edgeDistDim1_q8 < 0x20 || t.edgeDistDim2_q8 < 0x20) isGrip[z] = 1;

            excluded[z] = static_cast<uint8_t>((c.f218 & G218_MERGED) != 0);
        }

        for (int r = 0; r < kRows; ++r) {
            for (int cc = 0; cc < kCols; ++cc) {
                const size_t i = static_cast<size_t>(r) * kCols + cc;
                const uint8_t z = bigFrame.touchZones[r][cc];
                if (z == 0) {
                    if (bigFrame.gripStage[r][cc] != 0) m_gripTimeOff[i] = 0;
                    bigFrame.gripStage[r][cc] = 0;
                    m_gripTimeOff[i] = TSABuffer_GetNextTime(m_gripTimeOff[i],
                                                             bigFrame.frameIntervalMs);
                } else if (excluded[z] == 0 &&
                           static_cast<int32_t>(m_gripStageDwellMs) <
                               static_cast<int32_t>(bigFrame.gripTime[r][cc])) {
                    bigFrame.gripStage[r][cc] = isGrip[z];
                }
                // 原厂尾句 `if (g_tsaBufTimeRpt[i] == 0) g_tsaBufGripTimeOff[i] = 0x7FFF;`
                // 读的是模块 05 私有的 m_bufTimeRpt（未发布到共享大帧）。
                // g_tsaBufGripTimeOff 在本机没有任何读者，该句无可观测效果，
                // 缺输入时略去并记在此处，不编造替代量。
            }
        }
    }

    /// TSABuffer_GripTimeProcess @0x6BA8EEA1
    ///   Dif > GripFilter_GetTrackingThold() → gripTime += 帧间隔（饱和），否则清零。
    void TSABuffer_GripTimeProcess(TouchBigFrame& bigFrame) const noexcept {
        // GripFilter_GetTrackingThold @0x6BAE5D38 = wPeakMode3ThresholdMax / 2
        //（原式 `(v + (v >> 15)) >> 1`，即向零取整的 /2）。
        const int32_t thr = Fixed::ShrTowardZero(bigFrame.tholds.peakThold, 1);
        for (int r = 0; r < kRows; ++r) {
            for (int c = 0; c < kCols; ++c) {
                if (thr < bigFrame.diff[r][c]) {
                    const int32_t next = TSABuffer_GetNextTime(bigFrame.gripTime[r][c],
                                                               bigFrame.frameIntervalMs);
                    bigFrame.gripTime[r][c] = static_cast<int16_t>(std::max<int32_t>(0, next));
                } else {
                    bigFrame.gripTime[r][c] = 0;
                }
            }
        }
    }

    /// TSABuffer_GetNextTime @0x6BA8EE??：v + nBaselineStageInitialSignal（= 归一化
    /// 帧间隔 ms），**饱和**到 [-0x8000, 0x7FFF]。
    [[nodiscard]] static int16_t TSABuffer_GetNextTime(int16_t v, int32_t dtMs) noexcept {
        const int32_t n = static_cast<int32_t>(v) + dtMs;
        return static_cast<int16_t>(std::clamp<int32_t>(n, -0x8000, 0x7FFF));
    }

    /// GripFilter_GetEdge @0x6BAE9854：把 TSA_GetTouchSide 的 1/2/4/8 映射成 0..3。
    [[nodiscard]] static uint8_t GripFilter_GetEdge(const TrackingSlot& t) noexcept {
        const uint32_t side = t.gripEdgeFlags;
        if ((side & 4u) != 0) return 2;   // 左
        if ((side & 8u) != 0) return 3;   // 右
        if ((side & 1u) != 0) return 0;   // 上
        return 1;                          // 下（含无贴边）
    }

    /// GripFilter_IsInMultiGripRegion：+0x1F0 & 0x4000，或（距边 < 0x101 ∧ IsRatioOK(3)）。
    [[nodiscard]] bool GripFilter_IsInMultiGripRegion(const TrackingSlot& t,
                                                      const FrameUnit& c) const noexcept {
        if ((t.volatileFlags & 0x4000u) != 0) return true;
        const bool nearEdge = (t.edgeDistDim1_q8 < 0x101 || t.edgeDistDim2_q8 < 0x101);
        return nearEdge && IsRatioOK(t, c, 3);
    }

    /// GripFilter_BasicProcess @0x6BAE4AB3
    void GripFilter_BasicProcess(TrackingSlot& t, TouchBigFrame& bigFrame) {
        FrameUnit& c = m_cur[t.id];
        const PrevUnit& p = m_prev[t.id];

        // 1. 刚进入 typing 或已识别双手 → DAT_6bc84e22 = 0。
        //    TSA_IsJustEnterTypingScenario / HandGesture_Is2HandDetected 在复刻侧
        //    都没有上游（场景由宿主下发、HandGesture 未复刻）⇒ 恒假。

        // 2. g_grip / DAT_6bc84e21 / DAT_6bc84e24 上下文
        c.edge = GripFilter_GetEdge(t);

        // 3. GetPrmt：按中心握持等级取 gripPrmt 行
        const GripPrmt& prmt = m_gripPrmt[c.level];

        // 4. GetProperty → GetQuality → GetStage → UpdateDeadzone（关闭）
        GripFilter_GetProperty(t, c, bigFrame);
        c.quality = GripFilter_GetQuality(t, c, prmt, bigFrame);
        GripFilter_GetStage(t, c);
        if constexpr (kDynDeadZoneOn) { /* GripFilter_UpdateDeadzone，flash 0x8C0 = 0 */ }

        // 5. 阶段由非 1 变为 1 且 e22 != 0xFF → e22++
        if (c.stage == GripStage1_AbsGrip && p.stage210 != GripStage1_AbsGrip &&
            m_gripEnterCnt != -1) {
            ++m_gripEnterCnt;
        }

        // 6. 阶段为 1 或 2 → hdr[0x0D]++（TouchMode 的 NoObj / Glove 判据读它）
        if (c.stage == GripStage1_AbsGrip || c.stage == GripStage2_TempGrip) {
            if (m_gripSuppressedCount < 255) ++m_gripSuppressedCount;
        }

        // 7. 本帧已上报（+0x1F8 & 0x100000）且**上一帧**处于握持阶段 ⇒
        //    +0x1FC |= 0x40000000，供 GripFilter_Report 回填后报点。
        if ((c.f1F8 & PF_CUR_REPORTED) != 0 && GripStageIsGrip(p.stage210)) {
            c.f1FC |= G1FC_GRIP_RELEASED;
        }

        t.gripChannelFlags = c.f214;
        t.persistFlags = c.f218;
    }

    // ── §4.2 GetProperty：8 个检测器顺序执行 ──────────────────────────
    void GripFilter_GetProperty(TrackingSlot& t, FrameUnit& c, TouchBigFrame& bigFrame) {
        GrapDetect(t, c, bigFrame);
        AboveGripDetect(t, c, bigFrame);
        CloseGripDetect(t, c, bigFrame);
        MultiGripDetect(t, c);
        NewTouchDetect(t, c, bigFrame);
        FastMoveDetect(t, c);
        HeavyGripRegionDetect(t, c, bigFrame);
        SafeTouchDetect(t, c);
    }

    /// GripFilter_GrapDetect：整手抓握。满足「未离开、按下时间差 < 250 ms、
    /// 驻留 < 500 ms、3 · flash 0x5B4 < +0x1BA · 10、中心等级 != 0」的触点多于
    /// 3 个时，给**全部**触点打 +0x214 |= 0x4000000。
    void GrapDetect(const TrackingSlot& t, FrameUnit& c, TouchBigFrame& bigFrame) {
        (void)t;
        uint32_t minDur = 0xFFFFFFFFu;
        for (const auto& o : bigFrame.tracks) {
            if (o.id == 0 || o.id >= kMaxSlots) continue;
            minDur = std::min(minDur, m_cur[o.id].durationMs);
        }
        int n = 0;
        for (const auto& o : bigFrame.tracks) {
            if (o.id == 0 || o.id >= kMaxSlots) continue;
            const FrameUnit& oc = m_cur[o.id];
            if (Touch_IsOff(o)) continue;
            if (oc.durationMs - minDur >= 250) continue;
            if (oc.durationMs >= 500) continue;
            if (3 * static_cast<int32_t>(m_peakFilterRefThold) >=
                static_cast<int32_t>(o.minSigLatched) * 10) continue;
            if (oc.level == 0) continue;
            ++n;
        }
        if (n > 3) c.f214 |= G214_GRAB;
    }

    /// GripFilter_AboveGripDetect：本触点正下方还有一个同为「多握持区」的触点。
    void AboveGripDetect(const TrackingSlot& t, FrameUnit& c, const TouchBigFrame& bigFrame) {
        if ((c.f214 & G214_MULTI_REGION) == 0) return;
        for (const auto& o : bigFrame.tracks) {
            if (o.id == t.id || o.id == 0 || o.id >= kMaxSlots) continue;
            if ((m_cur[o.id].f214 & G214_MULTI_REGION) == 0) continue;
            const int32_t dDim2 = std::abs((o.curY_q8 >> 8) - (t.curY_q8 >> 8));
            if (dDim2 >= 6) continue;
            if (o.curY_q8 < t.curY_q8) { c.f214 |= G214_ABOVE_GRIP; return; }
        }
    }

    /// GripFilter_CloseGripDetect：不在手掌区 → +0x214 |= 0x20 并清 +0x218 的 8/0x10；
    /// 在手掌区则查近邻（CloseGripOnCore）。
    void CloseGripDetect(const TrackingSlot& t, FrameUnit& c, const TouchBigFrame& bigFrame) {
        if (!IsTouchInHeavyRegion(t, bigFrame)) {
            c.f214 |= G214_NOT_HEAVY;
            c.f218 &= ~(G218_INNER_EDGE_PK | G218_MERGE_BROKEN);
            return;
        }
        for (const auto& o : bigFrame.tracks) {
            if (o.id == t.id || o.id == 0 || o.id >= kMaxSlots) continue;
            const int32_t dDim2 = std::abs((o.curY_q8 >> 8) - (t.curY_q8 >> 8));
            if (dDim2 >= 6) continue;
            // GripFilter_IsSignalOK(o, 8, 1)：读 +0x1B8/+0x1BA 的信号档位，
            // 复刻用锁存信号 +0x1BA 是否达到本帧信号门限作为等价判据。
            if (o.minSigLatched < bigFrame.tholds.sigThold) continue;
            c.f218 |= G218_CLOSE_GRIP;
            if (IsTouchInHeavyRegion(o, bigFrame)) c.f218 |= G218_MERGE_BROKEN;
            if (m_cur[o.id].durationMs < 200) c.f218 |= G218_INNER_EDGE_PK;
        }
        // Side 分支（g_touchesSide）本机整池为空，不可达。
    }

    /// GripFilter_MultiGripDetect：!(0x214 & 2) 且 0x214 & 4 时做核心检查。
    /// CheckMultiGripOnCore 读峰的核心格分布（g_pPeaks + 0x15/0x16），复刻侧
    /// TrackingSlot 不带源峰下标 ⇒ 用「同侧还有另一个多握持区触点」等价近似。
    void MultiGripDetect(const TrackingSlot& t, FrameUnit& c) {
        if ((c.f214 & G214_MULTI_GRIP) != 0) return;
        if ((c.f214 & G214_MULTI_REGION) == 0) return;
        if ((c.f214 & G214_ABOVE_GRIP) != 0) c.f214 |= G214_MULTI_GRIP;
        else if ((c.f214 & G214_NEW_NEIGHBOR) != 0) c.f214 |= G214_MULTI_WEAK;
        (void)t;
    }

    /// GripFilter_NewTouchDetect @0x6BAE8AB3
    void NewTouchDetect(const TrackingSlot& t, FrameUnit& c, const TouchBigFrame& bigFrame) {
        const uint32_t window = (bigFrame.tracks.size() > 2) ? 300u : 200u;
        for (const auto& o : bigFrame.tracks) {
            if (o.id == 0 || o.id >= kMaxSlots) continue;
            if (o.id != t.id) {
                // Touch_IsGhostByMinSigOff：+0x1F0 & 4（MinSigOff ghost）
                if ((o.volatileFlags & 0x4u) == 0) {
                    const uint16_t dMm = SqPixelToSqMm(
                        SqDistQ8(t.curX_q8, t.curY_q8, o.curX_q8, o.curY_q8));
                    if (dMm < 0x1C39) {
                        // GripFilter_IsTouchIntervalInTime(g_grip, o, 0, window)
                        const uint32_t a = c.durationMs, b = m_cur[o.id].durationMs;
                        const uint32_t diff = (a > b) ? (a - b) : (b - a);
                        if (diff < window) c.f214 |= G214_NEW_NEIGHBOR;
                        // GripFilter_IsHigherPriorityTouch：对方更早按下且信号更强
                        if (b > a && o.minSigLatched >= t.minSigLatched) {
                            c.f214 |= G214_HIGH_PRIORITY;
                        }
                        if ((m_cur[o.id].f1F8 & PF_POSTURE_BLOCK) != 0 &&
                            (c.f214 & G214_HIGH_PRIORITY) != 0) {
                            c.f214 |= G214_PRIORITY_CLASH;
                        }
                    }
                }
                if (!Touch_IsOff(o)) c.f214 |= G214_OTHER_PRESENT;
            }
        }
    }

    /// GripFilter_FastMoveDetect @0x6BAE65FF
    ///   +0x248 > 0xFFFF ∧ !(+0x1F0 & 0x40000) ∧ +0x1C0 / 2 < 峰 zn(+0x20)
    /// +0x248（帧间位移的 Q8 平方，未换算 mm）与 +0x1C0、源峰下标在复刻侧
    /// 都缺产出方；只落地前两条并用 movePxSq244 代 +0x248，第三条留结构。
    void FastMoveDetect(const TrackingSlot& t, FrameUnit& c) const noexcept {
        if (c.movePxSq244 > 0xFFFF && (t.volatileFlags & 0x40000u) == 0) {
            // 第三条判据缺产出方，此处不再收紧。
        }
    }

    /// GripFilter_HeavyGripRegionDetect @0x6BAE8D98
    void HeavyGripRegionDetect(const TrackingSlot& t, FrameUnit& c, const TouchBigFrame& bigFrame) {
        const PrevUnit& p = m_prev[t.id];
        if (IsTouchInCornerRegion(t, bigFrame)) {
            c.f214 |= G214_CORNER;
            if (t.age == 1) c.f214 |= G214_CORNER_FIRST;
            if ((static_cast<uint32_t>(t.state) & 1u) != 0 && p.cornerDwell4E4 > 4) {
                c.f214 |= G214_CORNER_DWELL;
            }
        }
        if (IsTouchInHeavyRegion(t, bigFrame)) {
            c.f1FC |= G1FC_HEAVY_REGION;
            if (IsTouchInCornerRegion(t, bigFrame)) {
                c.f1FC |= G1FC_HEAVY_CORNER;
                if (t.age == 1) c.f1FC |= G1FC_HEAVY_CORNER_FIRST;
                if (Touch_IsOff(t)) c.f1FC |= G1FC_HEAVY_CORNER_OFF;
            }
        }
        // GripFilter_IsHeavyCornerTouched：扫全屏角落格是否有超门限信号。
        if (IsHeavyCornerTouched(bigFrame)) c.f218 |= G218_HEAVY_CORNER;
    }

    /// GripFilter_SafeTouchDetect @0x6BAE891D —— 置 +0x214 |= 0x800（绝对非握持）。
    void SafeTouchDetect(const TrackingSlot& t, FrameUnit& c) {
        bool safe = false;
        if (c.zoneGripMax199 == 0 && t.age > 1) {
            safe = true;
        } else if (BasicIsFlick(t, c)) {
            safe = true;
        } else if ((c.f214 & G214_MERGE_ALLOWED) != 0) {
            safe = true;
        } else if (IsInExcludedRegion(t, c)) {
            safe = true;
        } else if ((c.f1F8 & PF_POSTURE_BLOCK) != 0 && IsRatioOK(t, c, 0xF)) {
            safe = true;
        }
        if (safe) c.f214 |= G214_ABS_NOT_GRIP;
    }

    /// GripFilter_IsInExcludedRegion @0x6BAE9F40（非旋转，无 SideTouch）：
    ///   长轴 < 20 ∧ d1 > 0x200 ∧ d2 > 0x100 ∧ IsRatioOK(0xF)，或 d1 > 0x400 ∧ d2 > 0x300。
    /// 因此远离边缘的触点第一帧即安全，直接进入阶段 4。
    [[nodiscard]] bool IsInExcludedRegion(const TrackingSlot& t, const FrameUnit& c) const noexcept {
        constexpr uint16_t k1 = 0x200;   // !SideTouch ⇒ 0x200（否则 0x300）
        constexpr uint16_t k2 = 0x100;
        if constexpr (kVideoMode) { if (IsNearbyCorner(t)) return false; }
        if (t.axisMajorMm < 20 && t.edgeDistDim1_q8 > k1 && t.edgeDistDim2_q8 > k2 &&
            IsRatioOK(t, c, 0xF)) {
            return true;
        }
        return t.edgeDistDim1_q8 > 0x400 && t.edgeDistDim2_q8 > 0x300;
    }

    /// GripFilter_BasicIsFlick @0x6BAE81D7 = IsFlickAllowed ∧ IsFlickDetected，
    /// 并在 detected ∧ !allowed 时置 +0x214 |= 0x8000。
    [[nodiscard]] bool BasicIsFlick(const TrackingSlot& t, FrameUnit& c) const noexcept {
        const bool detected = IsFlickDetected(t);
        const bool allowed = IsFlickAllowed(t, c);
        if (detected && !allowed) c.f214 |= G214_FLICK_BLOCKED;
        return allowed && detected;
    }

    /// GripFilter_IsFlickDetected @0x6BAE80E0：
    ///   thr = max(DAT_6bca92b8 = 10, 3 · +0x228 / 4)，判 thr² < prev +0x250。
    [[nodiscard]] bool IsFlickDetected(const TrackingSlot& t) const noexcept {
        const uint32_t byShape = Fixed::ShrUnsigned(static_cast<int32_t>(t.maxAxisMajorMm) * 3, 2);
        const uint32_t thr = std::max<uint32_t>(m_flickBaseThold, byShape);
        return thr * thr < static_cast<uint32_t>(m_prev[t.id].pathSq250);
    }

    /// GripFilter_IsFlickAllowed @0x6BAE7DBD（本机路径）
    [[nodiscard]] bool IsFlickAllowed(const TrackingSlot& t, const FrameUnit& c) const noexcept {
        if ((c.f1FC & G1FC_HEAVY_CORNER_FIRST) != 0 && (t.volatileFlags & 0x4000u) != 0) {
            return false;
        }
        // GetSafeDistToEdgeThold 与 +0x214 & 0x20000000 的组合；非旋转路径。
        if (t.edgeDistDim1_q8 < 0x200 && (c.f214 & G214_ABOVE_GRIP) != 0) return false;
        if ((c.f214 & G214_MULTI_GRIP) != 0) {
            if (c.durationMs > 1000) return false;
            if ((c.f1FC & G1FC_HEAVY_CORNER_FIRST) != 0) return false;
        }
        // IsSideTouch / IsInDynamicDeadZone：本机关闭，恒假。
        if ((c.f214 & G214_PRIORITY_CLASH) != 0 && t.maxAxisMajorMm >= 0xF) return false;
        // IsVideoMode 段本机不执行。
        return true;
    }

    // ── §4.3 质量 GripFilter_GetQuality @0x6BAE42BC ────────────────────
    //   BLReset_Status() == 1        → 3
    //   +0x214 & 0x800（AbsNotGrip）  → 0
    //   BasicIsAbsGrip                → 3
    //   BasicIsTempGrip               → 2
    //   否则 +0x214 |= 0x80           → 1
    [[nodiscard]] int32_t GripFilter_GetQuality(const TrackingSlot& t, FrameUnit& c,
                                                const GripPrmt& prmt,
                                                const TouchBigFrame& bigFrame) {
        if (bigFrame.baselineResetInDebounce) return 3;
        if ((c.f214 & G214_ABS_NOT_GRIP) != 0) return 0;
        if (BasicIsAbsGrip(t, c, prmt)) return 3;
        if (BasicIsTempGrip(t, c, prmt)) return 2;
        c.f214 |= G214_QUALITY1;
        return 1;
    }

    /// GripFilter_BasicIsAbsGrip @0x6BAE3FC1
    [[nodiscard]] bool BasicIsAbsGrip(const TrackingSlot& t, const FrameUnit& c,
                                      const GripPrmt& prmt) const noexcept {
        if (BasicIsAbsGripPrmt(t, prmt)) return true;
        // IsSuppressedGlobally / 2：HandGesture 与 CoreTouch 通道未复刻 ⇒ 恒假。
        if ((c.f214 & G214_MULTI_GRIP) != 0) return true;
        // +0x218 & 1 ∧ 峰标志 0x400：源峰下标缺失 ⇒ 不可达，保留结构。
        if ((c.f218 & G218_MERGE_BROKEN) != 0) return true;
        // IsCornerGripClick / BasicIsAbsGripAbnormal：缺输入 ⇒ 恒假。
        if (IsLongPress(t, c) && (c.f1F8 & PF_POSTURE_BLOCK) == 0 /* ∧ !Is2HandGame */) {
            if ((IsOnDim2(c) || IsNearbyCorner(t)) && (c.f214 & G214_DELAY_TIMEOUT) != 0) {
                return true;
            }
            if constexpr (kScreenRotated) {
                if (IsNearbyCorner(t) && (c.f214 & G214_NEW_NEIGHBOR) != 0) return true;
            }
        }
        // GripFilter_IsNegSigTouchBypass：flash 0x94A = 0 ⇒ 恒假。
        return false;
    }

    /// GripFilter_BasicIsAbsGripPrmt @0x6BAE3C2F
    ///   参数行启用 ∧（+0x25C 非零 ∨ 已离开）∧ +0x25E > GetAbsThold(行 +0x08)
    /// +0x25C 由 Touch_IIRDataProcess @0x6BA7474F 一进入就置 1 ⇒ AFT 跑过之后恒真。
    [[nodiscard]] bool BasicIsAbsGripPrmt(const TrackingSlot& t, const GripPrmt& prmt) const noexcept {
        if (prmt.type == 0) return false;
        uint32_t thr = GetAbsThold(prmt.absMajorMm);
        if constexpr (kVideoMode) thr = 0x14;
        if (t.shapeSnapshotValid == 0 && !Touch_IsOff(t)) return false;
        return static_cast<uint32_t>(t.iirAxisMajorMm) > thr;
    }

    /// GripFilter_BasicIsTempGrip @0x6BAE39AA
    [[nodiscard]] bool BasicIsTempGrip(const TrackingSlot& t, const FrameUnit& c,
                                       const GripPrmt& prmt) const noexcept {
        // Touch_IsGhostByMinSigOff：+0x1F0 & 4
        if ((t.volatileFlags & 0x4u) != 0) return true;
        // GripFilter_IsGlobalSuppressedClick：CoreTouch 通道未复刻 ⇒ 恒假。
        if (IsRatioOK(t, c, 0xD)) return true;
        // IsLongPressAllowed：读场景标志，本机恒假 ⇒ 下面这条等价于 IsLongPress。
        if (IsLongPress(t, c)) return true;
        if ((c.f214 & G214_MULTI_WEAK) != 0 && IsRatioOK(t, c, 0x11)) return true;
        if ((c.f1FC & G1FC_HEAVY_CORNER) != 0 &&
            static_cast<int32_t>(t.peakSig) * 2 > static_cast<int32_t>(t.minSigLatched) * 3) {
            return true;
        }
        if ((c.f1FC & G1FC_HEAVY_CORNER_FIRST) != 0 && t.iirAxisMinorMm >= 10) return true;
        if (IsLongPress(t, c)) { if (IsRatioOK(t, c, 0xE)) return true; }
        else                   { if (IsRatioOK(t, c, 0xC)) return true; }
        if ((c.f214 & G214_MERGE_ALLOWED) != 0 && IsRatioOK(t, c, 5)) return true;
        // BasicIsTempGripAbnormal 读 +0x1F4 的异常位（缺产出方）；
        // IsNegSigTouch 的 flash 0x94A = 0 ⇒ 两条恒假。
        return BasicIsTempGripPrmt(t, prmt);
    }

    /// GripFilter_BasicIsTempGripPrmt @0x6BAE3244 —— 参数行与 IIR 形状快照逐项比。
    /// 这是 06-「AbsGrip / TempGrip 比率判据」的落点：五个字段的产出方
    /// （Touch_IIRDataProcess @0x6BA7474F）已由模块 05 补齐，不再是死代码。
    [[nodiscard]] bool BasicIsTempGripPrmt(const TrackingSlot& t, const GripPrmt& prmt) const noexcept {
        if (prmt.type == 0) return false;
        if (t.iirAxisMajorMm > prmt.absMajorMm) return true;                 // +0x25E vs +0x08
        if (t.iirMinEllipseRate > static_cast<int32_t>(prmt.ellipseRate)) return true; // +0x268 vs +0x0C
        // +0x267 vs +0x0A，且要求 +0x1AD == 1（该维只有一个峰）。复刻侧
        // +0x1AD 无产出方，按「单峰」处理以保留判据活性。
        if (t.axisRatioX16 > prmt.axisRatio) return true;
        if (t.iirGripRatioOuter > prmt.gripOuter) return true;               // +0x26A vs +0x0E
        if (t.iirGripRatioInner > prmt.gripInner) return true;               // +0x26C vs +0x10
        return false;
    }

    /// GripFilter_IsLongPress @0x6BAE675F：+0x158 > 299（视频模式或角落驻留 > 199）。
    [[nodiscard]] bool IsLongPress(const TrackingSlot&, const FrameUnit& c) const noexcept {
        if constexpr (kVideoMode) return c.durationMs > 199;
        if ((c.f214 & G214_CORNER_DWELL) != 0) return c.durationMs > 199;
        return c.durationMs > 299;
    }

    [[nodiscard]] static bool IsOnDim1(const FrameUnit& c) noexcept { return c.edge >= 2; }
    [[nodiscard]] static bool IsOnDim2(const FrameUnit& c) noexcept { return c.edge < 2; }

    /// GripFilter_IsNearbyCorner：重心落在四角 6 × 6 格窗内。
    [[nodiscard]] static bool IsNearbyCorner(const TrackingSlot& t) noexcept {
        const int cx = std::clamp<int>(t.curX_q8 >> 8, 0, kCols - 1);
        const int cy = std::clamp<int>(t.curY_q8 >> 8, 0, kRows - 1);
        return (cx < 6 || cx >= kCols - 6) && (cy < 6 || cy >= kRows - 6);
    }

    /// GripFilter_IsTouchInHeavyRegion @0x6BAAA454：重心格 gripRegion > 7（等级 3）。
    [[nodiscard]] static bool IsTouchInHeavyRegion(const TrackingSlot& t,
                                                   const TouchBigFrame& bf) noexcept {
        const int cx = std::clamp<int>(t.curX_q8 >> 8, 0, kCols - 1);
        const int cy = std::clamp<int>(t.curY_q8 >> 8, 0, kRows - 1);
        return bf.gripRegion[cy][cx] > 7;
    }

    /// GripFilter_IsTouchInCornerRegion @0x6BAAA327 → IsSensorInCornerRegion。
    /// 底部两角各 3 格（GripFilter_RegionSetBottomLeft/RightCorner，等级 3），
    /// 与模块 03 的 ProcessGripRegion 写入的几何完全一致。
    [[nodiscard]] static bool IsSensorInCornerRegion(int cx, int cy) noexcept {
        const int lastR = kRows - 1, lastC = kCols - 1;
        if (cy == lastR && (cx == 0 || cx == 1)) return true;
        if (cy == lastR - 1 && cx == 0) return true;
        if (cy == lastR && (cx == lastC || cx == lastC - 1)) return true;
        if (cy == lastR - 1 && cx == lastC) return true;
        return false;
    }
    [[nodiscard]] static bool IsTouchInCornerRegion(const TrackingSlot& t,
                                                    const TouchBigFrame& bf) noexcept {
        (void)bf;
        return IsSensorInCornerRegion(std::clamp<int>(t.curX_q8 >> 8, 0, kCols - 1),
                                      std::clamp<int>(t.curY_q8 >> 8, 0, kRows - 1));
    }

    /// GripFilter_IsHeavyCornerTouched：任一角落格的 Dif 超过跟踪门限
    /// （GripFilter_GetTrackingThold @0x6BAE5D38 = wPeakMode3ThresholdMax / 2）。
    [[nodiscard]] static bool IsHeavyCornerTouched(const TouchBigFrame& bf) noexcept {
        const int16_t thr = static_cast<int16_t>(bf.tholds.peakThold / 2);
        for (int r = kRows - 2; r < kRows; ++r) {
            for (int c = 0; c < kCols; ++c) {
                if (!IsSensorInCornerRegion(c, r)) continue;
                if (bf.diff[r][c] > thr) return true;
            }
        }
        return false;
    }

    // ── §4.3 IsRatioOK / GetEdgeClickRatioThold ───────────────────────
    /// GripFilter_GetEdgeClickRatioThold @0x6BAACAD0
    /// 形态越细长 / 越长按 → 阈值档位越高；档位 0..2 取边缘表（全 0xFF ⇒ 钳成
    /// 0xFFFF），≥3 固定 16。结果是只有 ratio 饱和成 0xFFFF 的纯边缘细条才通过。
    [[nodiscard]] uint16_t GetEdgeClickRatioThold(const TrackingSlot& t, const FrameUnit& c,
                                                  int base) const noexcept {
        int lvl = base;
        if (t.maxAxisMajorMm > 0x11) {
            lvl = base + 1;
            if (t.axisRatioX16 > 0x2F) lvl = base + 2;
            if (t.maxAxisMajorMm > 0x18) lvl += 1;
            // (+0x1FC & 8) ∧ IsHeavyRegionTouched：+0x1FC bit3 无产出方，略。
            if (IsLongPress(t, c)) lvl += 1;
        }
        // 角落弱化段需要 +0x1B9（峰邻域档位），缺产出方，略。
        // GetRatioTholdLevelExtra：长按时把 0 抬成 1（旋转分支本机不可达）。
        if (IsLongPress(t, c) && lvl == 0) lvl = 1;

        uint16_t v;
        if (lvl <= 0)      v = kEdgeRatioTbl[c.edge][0];
        else if (lvl == 1) v = kEdgeRatioTbl[c.edge][1];
        else if (lvl == 2) v = kEdgeRatioTbl[c.edge][2];
        else               v = 0x10;
        if (v > 0xFE) v = 0xFFFF;
        return v;
    }

    /// GripFilter_IsRatioOK @0x6BAACDC6 —— 只实现本机真正被调到的编号。
    /// 比值取上一帧的 IIR 值（prev +0x26A / +0x26C），Dim1 取外侧、Dim2 取内侧。
    [[nodiscard]] bool IsRatioOK(const TrackingSlot& t, const FrameUnit& c, int which) const noexcept {
        const PrevUnit& p = m_prev[t.id];
        const uint16_t outer = p.iirOuter26A;   // +0x26A
        const uint16_t inner = p.iirInner26C;   // +0x26C
        const uint8_t e = static_cast<uint8_t>(std::min<int>(c.edge, 3));

        switch (which) {
        case 3: {   // 阈值取边缘表 +6（0xFF）
            const uint16_t thr = kEdgeRatioTbl[e][2];
            return IsOnDim1(c) ? (outer >= thr) : (inner >= thr);
        }
        case 5: case 0xC: {
            const uint16_t thr = GetEdgeClickRatioThold(t, c, 0);
            if (IsOnDim1(c)) return outer >= thr && t.edgeDistDim1_q8 <= 0x100;
            return inner >= thr && t.edgeDistDim2_q8 <= 0x100;
        }
        case 0xD: {
            // 需要 Dim1 贴边且该轨迹已存在 > 40 帧（prev +0x520 时间戳），
            // 再判本帧 +0x1A8（外侧比）≥ 边缘表 +4。
            if (!IsOnDim1(c) || c.durationMs < 0x29) return false;
            return t.iirGripRatioOuter >= kEdgeRatioTbl[e][0];
        }
        case 0xE: {
            const uint16_t thr = GetEdgeClickRatioThold(t, c, 0);
            return IsOnDim1(c) ? (outer >= thr) : (inner >= thr);
        }
        case 0xF:   // 「比值都很小」= 非边缘细条
            return t.iirGripRatioOuter <= 8 && t.iirGripRatioInner <= 8;
        case 0x11: {
            const uint16_t thr = kEdgeRatioTbl[e][2];
            return IsOnDim1(c) ? (outer >= thr) : (inner >= thr);
        }
        default:
            return true;
        }
    }

    // ── §4.5 阶段机 GripFilter_GetStage @0x6BAE492D ────────────────────
    void GripFilter_GetStage(TrackingSlot& t, FrameUnit& c) {
        int32_t stage = m_prev[t.id].stage210;
        switch (stage) {
        case 0:
            if (IsSafeTouch(c)) {
                stage = 4;
            } else {
                stage = StageOnQuality(c.quality);
                if (!IsDelayTimeOut(t, c) && !Touch_IsOff(t) && (stage == 2 || stage == 3)) {
                    stage = 0;
                }
            }
            break;
        case 1:
            if (IsSafeTouch(c)) stage = 4;
            break;
        case 2:
            stage = IsSafeTouch(c) ? 4 : StageOnQuality(c.quality);
            break;
        case 3:
            if (IsBackToGrip(t, c)) {
                GripFilter_BackToGrip(t);          // ⚠ 不改 stage
            } else if (IsSafeTouch(c)) {
                stage = 4;
            } else {
                stage = StageOnQuality(c.quality);
                if (stage == 2) stage = 3;         // 3 不会退回 2
            }
            break;
        case 4:
            if (IsBackToGrip(t, c)) GripFilter_BackToGrip(t);   // ⚠ 不改 stage
            break;
        default:
            break;
        }
        c.stage = stage;
        t.gripStage210 = stage;   // +0x210：下一帧的 prev 阶段，模块 05 也读它
    }

    /// GripFilter_IsSafeTouch @0x6BAE42A9 = GripFilter_BasicIsAbsNotGrip = +0x214 & 0x800
    [[nodiscard]] static bool IsSafeTouch(const FrameUnit& c) noexcept {
        return (c.f214 & G214_ABS_NOT_GRIP) != 0;
    }

    /// GripFilter_StageOnQuality @0x6BAE48E1：q0→4，q3→1，q2→2，其余→3。
    [[nodiscard]] static int32_t StageOnQuality(int32_t q) noexcept {
        if (q == 0) return 4;
        if (q == 3) return 1;
        if (q == 2) return 2;
        return 3;
    }

    /// GripFilter_IsDelayTimeOut @0x6BAE47EC：TE < 2 返回 0；否则 +0x158 ≥
    /// GetDelayTime()（WindowsPad 固定 50 ms）时置 +0x214 |= 0x40000000 并返回 1。
    /// 这条同时是 GripFilter「50 ms 放行」→ Touch_CreatePostReport 的触发源。
    [[nodiscard]] bool IsDelayTimeOut(const TrackingSlot& t, FrameUnit& c) noexcept {
        if (static_cast<uint32_t>(t.state) < 2u) return false;
        const uint32_t delay = kWindowsPad ? m_gripDelayMs : m_gripPrmt[c.level].delayMs;
        if (c.durationMs < delay) return false;
        c.f214 |= G214_DELAY_TIMEOUT;
        return true;
    }

    /// GripFilter_IsBackToGrip @0x6BAE237B —— 三段判据，本机只有第 3 段活着。
    ///   1. feat & 0x20 ∧ IsStuckSideTouch()    → 真
    ///   2. IsBackToGripAllowed ∧ IsBackToGripSatisfied → 真
    ///   3. (+0x242 < 0x1A) ∨ (d1 > 0x3F ∧ d2 > 0x3F) → IsSeparated()，否则真
    /// 第 3 段的 else 分支是「贴到边框 0.25 格以内**且**有一次 ≥ 26 mm² 的加速度
    /// 突变」才回退。匀速滑到边缘的手指加速度接近 0，落进 IsSeparated 返回假。
    [[nodiscard]] bool IsBackToGrip(const TrackingSlot& t, const FrameUnit& c) const noexcept {
        if constexpr (kFeatGrip) { if (IsStuckSideTouch()) return true; }
        if (IsBackToGripAllowed(c) && IsBackToGripSatisfied(t)) return true;

        if (c.accSqMm242 < 0x1A ||
            (t.edgeDistDim1_q8 > 0x3F && t.edgeDistDim2_q8 > 0x3F)) {
            return IsSeparated(c);
        }
        return true;
    }

    /// GripFilter_IsStuckSideTouch @0x6BAE223A：两条支路在本机都拿不到输入。
    ///   · 主支路要 g_tsaBufGripTime[cell] > 500，而 GripTimeProcess 挂在
    ///     TSABuffer_GripStageProcess 下，四边合并使能全 0 ⇒ 整图恒 0。
    ///   · 备支路 IsInDynamicDeadZone 的总开关 flash 0x8C0 = 0 ⇒ 首行返回 0。
    [[nodiscard]] static bool IsStuckSideTouch() noexcept { return false; }

    /// GripFilter_IsBackToGripAllowed @0x6BAE1F52：+0x214 的 bit20 / bit21。
    [[nodiscard]] static bool IsBackToGripAllowed(const FrameUnit& c) noexcept {
        return (c.f214 & 0x00300000u) != 0;
    }

    /// GripFilter_IsBackToGripSatisfied @0x6BAE1FD5：前半支需要 Peak_IsOnOuterEdge
    /// 与 +0x29E（缺产出方），只落地后半支「任一轴距边不足 0.125 格」。
    [[nodiscard]] static bool IsBackToGripSatisfied(const TrackingSlot& t) noexcept {
        return t.edgeDistDim1_q8 < 0x20 || t.edgeDistDim2_q8 < 0x20;
    }

    /// GripFilter_IsSeparated @0x6BAE864D：首条判据就是 +0x214 的 bit9 / bit21，
    /// 产出方挂在 dwGripFlags 上 ⇒ 本机恒假，后续判据不可达。
    [[nodiscard]] static bool IsSeparated(const FrameUnit& c) noexcept {
        return (c.f214 & 0x00200200u) != 0;
    }

    /// GripFilter_BackToGrip @0x6BAE24A1（形参未用）：
    ///   TE = 0x20、坐标冻结回上一帧、PrevTouch_HistoryBufferClean。
    /// **阶段值不变**，触点以「抬起报告」方式结束，由 TE 在后续帧回收。
    void GripFilter_BackToGrip(TrackingSlot& t) noexcept {
        const PrevUnit& p = m_prev[t.id];
        t.state = EventUpFinal;          // 0x20
        t.curX_q8 = p.x162;
        t.curY_q8 = p.y164;
        // PrevTouch_HistoryBufferClean：清模块 07 的回放 FIFO。
        // 复刻侧 FIFO 归 07 持有，由它在 TE = 0x20 时自行清理。
    }

    /// GripFilter_Report @0x6BAE4C8E
    ///   握持 → +0x1F0 |= 0x8000000（TSAOut 在模块 08 据此剔除）
    ///   非握持 → 清该位；若 +0x1FC & 0x40000000 且 AFT 关闭 → Touch_CreatePostReport
    void GripFilter_Report(TouchBigFrame& bigFrame) {
        for (auto& t : bigFrame.tracks) {
            if (t.id == 0 || t.id >= kMaxSlots) continue;
            FrameUnit& c = m_cur[t.id];
            if (GripStageIsGrip(c.stage)) {
                t.volatileFlags |= VF_GRIP_SUPPRESS;
                t.isGrip = true;
                t.lifeFlags |= LifeGripSuppressed;
            } else {
                t.volatileFlags &= ~VF_GRIP_SUPPRESS;
                t.isGrip = false;
                t.lifeFlags &= ~LifeGripSuppressed;
                if ((c.f1FC & G1FC_GRIP_RELEASED) != 0 && !kAftOn) {
                    RequestPostReport(t.id);      // Touch_CreatePostReport(i, 5)
                }
            }
        }
    }

    // ══════════════════════════════════════════════════════════════════
    // §6 AntiTouch —— 只产出帧头状态与 +0x1F0 bit26，**不影响手指报点**
    // ══════════════════════════════════════════════════════════════════
    /// AntiTouch_Process @0x6BAD7BBB = CommonProcess + AFT 分支（flash 0x60 = 0，关闭）
    void AntiTouch_Process(TouchBigFrame& bigFrame, uint64_t nowUs) {
        AntiTouch_PreProcess(bigFrame);

        if (m_antiTouchAbnormalTsUs != 0 &&
            (nowUs - m_antiTouchAbnormalTsUs) < 500'000ull) {
            bigFrame.antiTouchHeaderFlags |= 0x20u;
        }
        if constexpr (kProximityOn) { /* Proximity_IsDetected()，本机关闭 */ }
        if (m_proximityDetected) bigFrame.antiTouchHeaderFlags |= 0x20u;

        for (auto& t : bigFrame.tracks) {
            if (t.id == 0 || t.id >= kMaxSlots) continue;
            AntiTouch_ProcessUnit(t, bigFrame, nowUs);
        }
        AntiTouch_Report(bigFrame);
    }

    /// AntiTouch_PreProcess @0x6BAD7A7C
    void AntiTouch_PreProcess(const TouchBigFrame& bigFrame) noexcept {
        m_antiTouchNumGrip = 0;
        m_antiTouchErMax = 0;
        for (const auto& t : bigFrame.tracks) {
            if (t.id == 0 || t.id >= kMaxSlots) continue;
            if (AntiTouch_IsGrip(t)) ++m_antiTouchNumGrip;
            m_antiTouchErMax = std::max(m_antiTouchErMax, static_cast<int16_t>(t.ellipseRate));
        }
    }

    /// AntiTouch_IsGrip：GripFilter 判为握持，或距边 < 0x100（非旋转取 Dim1）。
    [[nodiscard]] bool AntiTouch_IsGrip(const TrackingSlot& t) const noexcept {
        if (GripStageIsGrip(m_cur[t.id].stage)) return true;
        return t.edgeDistDim1_q8 < 0x100;
    }

    /// AntiTouch_ProcessUnit @0x6BAD7C4B
    void AntiTouch_ProcessUnit(TrackingSlot& t, TouchBigFrame& bigFrame, uint64_t nowUs) {
        PrevUnit& p = m_prev[t.id];

        // 1. 长轴 > 50 mm 且非握持 → 帧头 0x20 + 接近检查 + 记录异常时间戳
        if (t.axisMajorMm > 50 && !AntiTouch_IsGrip(t)) {
            bigFrame.antiTouchHeaderFlags |= 0x20u;
            AntiTouch_CheckProximityFlag(t, bigFrame);
            m_antiTouchAbnormalTsUs = nowUs;
        }

        // 2. AntiTouch_ProcessProximityUnit：feat & 0x4000 = 0 ⇒ IsEnabled 假，
        //    Proximity_TouchPostProcess 不可达；IsToProtectProximity 段随之关闭。

        // 3. !(0x214 & 0x200000) ∧ (包围盒任一轴 > 8) ∧ 椭圆率 > 15 → +0x1F0 |= 0x4000000
        if ((m_cur[t.id].f214 & G214_MERGE_ALLOWED) == 0 &&
            (t.extentX > 8 || t.extentY > 8) && t.ellipseRate > 15) {
            t.volatileFlags |= VF_ANTI_TOUCH;
        }

        // 4. 持续性记忆计数器 prev +0x4E6 / +0x4E7（0..9 饱和）
        if (AntiTouch_GetPersistFlag(t, bigFrame)) {
            if (p.antiPersist4E6 < 10) ++p.antiPersist4E6;
        } else {
            if (p.antiPersist4E6 != 0 && p.antiPersist4E6 < 10) --p.antiPersist4E6;
            if (p.antiProx4E7 < 10) {
                const int32_t y3 = ToDisplayDim2(t.curY_q8) * 3;
                if (t.ellipseRate < 10 && y3 > static_cast<int32_t>(m_antiProxYLimit)) {
                    ++p.antiProx4E7;
                } else {
                    p.antiProx4E7 = 0;
                }
            }
        }
    }

    /// AntiTouch_CheckProximityFlag @0x6BAD6A?? ：显示 Y ≤ flash 0x168C(10) → 帧头 0x80。
    void AntiTouch_CheckProximityFlag(const TrackingSlot& t, TouchBigFrame& bigFrame) const noexcept {
        if (ToDisplayDim2(t.curY_q8) <= static_cast<int32_t>(m_proximityYThold)) {
            bigFrame.antiTouchHeaderFlags |= 0x80u;
        }
    }

    /// AntiTouch_GetPersistFlag @0x6BAD6B3D —— 五条判据的析取。
    [[nodiscard]] bool AntiTouch_GetPersistFlag(TrackingSlot& t, TouchBigFrame& bigFrame) {
        bool persist = false;
        const FrameUnit& c = m_cur[t.id];

        // (a) 驻留 > 1500 ms 且 +0x24C < 100 mm²
        if (c.durationMs > 1500 && c.fromDownSqMm24C < 100) {
            persist = true;                  // dwSystemFlags & 0x800000 == 0
            t.volatileFlags |= VF_ANTI_TOUCH;
            if (t.ellipseRate > 9 && !AntiTouch_IsGrip(t)) {
                AntiTouch_CheckProximityFlag(t, bigFrame);
            }
        }

        const int32_t nonGrip =
            static_cast<int32_t>(bigFrame.tracks.size()) - static_cast<int32_t>(m_antiTouchNumGrip);

        // (b) 非握持触点 > 1 且椭圆率 ≥ 阈值且类型 != 3
        if (nonGrip > 1) {
            if (static_cast<int32_t>(t.ellipseRate) >= AntiTouch_GetEllipseRateThold(t) &&
                !t.fromMultiFinger) {
                t.volatileFlags |= VF_ANTI_TOUCH;
                persist = true;
                AntiTouch_CheckProximityFlag(t, bigFrame);
            }
        }

        // (c) 非握持触点 > 3（整手压覆）
        if (nonGrip > 3) {
            t.volatileFlags |= VF_ANTI_TOUCH;
            persist = true;
            AntiTouch_CheckProximityFlag(t, bigFrame);
        }

        // (d) 长轴 > AntiTouch_GetAxisMajorThold
        const uint16_t major = t.axisMajorMm;
        if (major > AntiTouch_GetAxisMajorThold(t)) {
            t.volatileFlags |= VF_ANTI_TOUCH;
            persist = true;
        }

        // (e) 5 · 短轴 < 2 · 长轴 且 长轴 > 15 → 记异常；再 6 · 短轴 < 2 · 长轴 才持久
        const uint32_t minor = t.axisMinorMm;
        if (minor * 5u < major * 2u && major > 15) {
            t.volatileFlags |= VF_ANTI_TOUCH;
            if (minor * 6u < major * 2u) persist = true;
        }
        return persist;
    }

    /// AntiTouch_GetEllipseRateThold：包围盒任一轴 > 8 且位于屏幕上半 → 10，否则 15。
    [[nodiscard]] static int32_t AntiTouch_GetEllipseRateThold(const TrackingSlot& t) noexcept {
        if (t.extentX > 8 || t.extentY > 8) {
            if (ToDisplayDim2(t.curY_q8) <= kDisplayHeight / 2) return 10;
        }
        return 15;
    }

    /// AntiTouch_GetAxisMajorThold：小接触 30、age < 5 时 30、否则 25；UDFP 40（关闭）。
    [[nodiscard]] uint8_t AntiTouch_GetAxisMajorThold(const TrackingSlot& t) const noexcept {
        uint8_t v;
        if (t.extentX < 9 && t.extentY < 9 && t.ellipseRate < 15) v = 30;
        else if (t.age < 5) v = 30;
        else v = 25;
        if constexpr (kUdfpRelaxOn) v = 40;
        return v;
    }

    /// AntiTouch_Report（由 Touch_ProcessExtInfo 调用）：按被拒计数写 hdr + 0x18。
    void AntiTouch_Report(TouchBigFrame& bigFrame) const noexcept {
        if (bigFrame.cancelledTouchCount > 0) {
            bigFrame.thenarHeaderFlags |= 0x1000000u;
        }
        if ((bigFrame.antiTouchHeaderFlags & 0x80u) != 0) {
            bigFrame.thenarHeaderFlags |= 0x2000000u;
        }
    }

    // ══════════════════════════════════════════════════════════════════
    // §7 BetaGrip —— Touch_ProcessBetaGrip @0x6BA76EE8，纯诊断（06-08）
    // ══════════════════════════════════════════════════════════════════
    void Touch_ProcessBetaGrip(TouchBigFrame& bigFrame) {
        for (auto& t : bigFrame.tracks) {
            if (t.id == 0 || t.id >= kMaxSlots) continue;
            PrevUnit& p = m_prev[t.id];
            const int state = Touch_IsBetaGrip(t, bigFrame);
            if (state == 4 || state == 8) {
                if (p.betaGrip4E8 < 0xFFFF) ++p.betaGrip4E8;
                uint32_t& cnt = (state == 4) ? m_betaGripLeftCnt : m_betaGripRightCnt;
                if (p.betaGrip4E8 > 1 && cnt < 0xFFFFFFFEu) {
                    m_cur[t.id].f1F8 |= PF_BETA_GRIP;
                    t.persistFlags1F8 |= PF_BETA_GRIP;
                    ++cnt;
                }
            } else {
                p.betaGrip4E8 = 0;
            }
        }
    }

    /// Touch_IsBetaGrip：未接充电器、峰值 ≥ 1500、TE == 4、无异常、
    /// 显示 X ≤ 8 或 ≥ Xres − 9、显示 Y ≥ Yres − 1 − mm(15)、prev 无 0x1000000。
    /// 返回 4 = 左，8 = 右。
    [[nodiscard]] int Touch_IsBetaGrip(const TrackingSlot& t, const TouchBigFrame& bf) const noexcept {
        if (bf.chargerPlugged) return 0;
        if (t.peakSig < 1500) return 0;
        if (t.state != EventMove) return 0;
        if (t.exceptionType != 0) return 0;
        if ((m_prev[t.id].f1F8 & PF_BETA_GRIP) != 0) return 0;
        const int32_t x = ToDisplayDim1(t.curX_q8);
        const int32_t y = ToDisplayDim2(t.curY_q8);
        // TS_MMToPixel(15) ≈ 15 mm 折算到 Dim2 显示像素
        const int32_t bottomBand = (15 * kDisplayHeight) / (kRows * 4);
        if (y < kDisplayHeight - 1 - bottomBand) return 0;
        if (x <= 8) return 4;
        if (x >= kDisplayWidth - 9) return 8;
        return 0;
    }

    // ══════════════════════════════════════════════════════════════════
    // §9 Thenar（大鱼际识别）
    // ══════════════════════════════════════════════════════════════════
    // Thenar_FeatureExtraction @0x6BB30C47 的扫描循环是
    //     for (i = 0; i < cols; i += cols - 1)
    // ⇒ 只跑 **col 0 与 col 59** 两条扫描线（06-06），不是全网格 Sobel。
    // 结果只经 TSAOut_ReportExtInfo → Thenar_UpdateExternalInfo 出去，
    // **不进手指上报路径**。
    void Thenar_ProcessInfoByDifTraditional(TouchBigFrame& bigFrame) {
        Thenar_FeatureExtraction(bigFrame);

        // Thenar_UpdateDetectFeature @0x6BB30E?? 的五组统计
        const int16_t peakThold = static_cast<int16_t>(
            Fixed::TruncDiv(static_cast<int32_t>(m_thenarBaseThold) * 5, 6));
        uint16_t maxLeft = 0, maxRight = 0;
        uint16_t idxLeft = 0xFFFF, idxRight = 0xFFFF;
        uint8_t  shortLeft = 0, shortRight = 0;
        uint16_t gapLeft = 0xFFFF, gapRight = 0xFFFF;
        uint16_t firstLeft = 0xFFFF, firstRight = 0xFFFF;

        for (uint16_t i = 0; i < m_thenarFeatures.size(); ++i) {
            const ThenarFeatureUnit& f = m_thenarFeatures[i];
            if (f.peak < peakThold) continue;
            if (f.edge != 0) {                              // col 0 → 左
                if (f.len > maxLeft) { maxLeft = f.len; idxLeft = i; }
                if (f.len < 6) ++shortLeft;
                if (firstLeft == 0xFFFF) firstLeft = i;
                else {
                    const ThenarFeatureUnit& g = m_thenarFeatures[firstLeft];
                    const uint16_t gap = Fixed::WrapU16(
                        static_cast<int32_t>(g.coord) - g.len - f.coord);
                    gapLeft = std::min(gapLeft, gap);
                }
            } else {                                        // col 59 → 右
                if (f.len > maxRight) { maxRight = f.len; idxRight = i; }
                if (f.len < 6) ++shortRight;
                if (firstRight == 0xFFFF) firstRight = i;
                else {
                    const ThenarFeatureUnit& g = m_thenarFeatures[firstRight];
                    const uint16_t gap = Fixed::WrapU16(
                        static_cast<int32_t>(g.coord) - g.len - f.coord);
                    gapRight = std::min(gapRight, gap);
                }
            }
        }

        // 长度阈值 LT = flash 0x836(30) · 16 / flash 0x1B(69) = 6 格
        const uint16_t lt = static_cast<uint16_t>(
            (static_cast<uint32_t>(m_thenarLenFactor) << 4) / m_thenarPitch);

        m_thenarDetected = false;
        m_thenarFrameSide = 0;
        if (shortLeft >= 3 && shortRight <= 2) {
            m_thenarDetected = true; m_thenarFrameSide = HandSideRight;
        } else if (shortRight >= 3 && shortLeft <= 2) {
            m_thenarDetected = true; m_thenarFrameSide = HandSideLeft;
        } else if (gapLeft < 4 && gapRight > 4) {
            m_thenarDetected = true; m_thenarFrameSide = HandSideRight;
        } else if (gapRight < 4 && gapLeft > 4) {
            m_thenarDetected = true; m_thenarFrameSide = HandSideLeft;
        } else if (maxRight < maxLeft && maxLeft >= lt) {
            m_thenarDetected = true; m_thenarFrameSide = HandSideLeft;
        } else if (maxLeft < maxRight && maxRight >= lt) {
            m_thenarDetected = true; m_thenarFrameSide = HandSideRight;
        }
        m_thenarSide = static_cast<uint8_t>(m_thenarFrameSide);

        // Thenar_SideHandDetectByCoreTouch：只写 hdr[0x42]/[0x43]/[0x4C]，
        // 唯一读者是导出 TSA_GetHandSide，不影响本管线，略。

        // Thenar_ProcessGensture(较长那一侧的特征 peak)
        const uint16_t useIdx = (maxLeft < maxRight) ? idxRight : idxLeft;
        const int16_t usePeak = (useIdx < m_thenarFeatures.size())
                                    ? m_thenarFeatures[useIdx].peak : 0;
        Thenar_ProcessGensture(bigFrame, usePeak);
    }

    /// Thenar_FeatureExtractionUnit：每列自底行向上做三状态扫描。
    void Thenar_FeatureExtraction(const TouchBigFrame& bigFrame) {
        m_thenarFeatures.clear();
        const int16_t enterThold = static_cast<int16_t>(m_thenarBaseThold / 2);   // T/2 = 225

        for (int col = 0; col < kCols; col += (kCols - 1)) {
            const uint8_t edge = static_cast<uint8_t>(col != kCols - 1);
            int nState = 0;
            uint16_t len = 0, width = 0;
            int16_t peak = 0;

            for (int r = kRows - 1; r >= 0; --r) {
                const int16_t sig = bigFrame.diff[r][col];
                if (nState == 0) {
                    if (sig > enterThold) {
                        nState = 1; len = 1; peak = sig;
                        width = OrthogonalWidth(bigFrame, r, col);
                    }
                } else if (nState == 1) {
                    if (sig < enterThold) {
                        SaveThenarFeature(static_cast<int16_t>(r), len, width, edge, peak);
                        nState = 0; len = 0; width = 0; peak = 0;
                    } else {
                        ++len;
                        peak = std::max(peak, sig);
                        width = std::max(width, OrthogonalWidth(bigFrame, r, col));
                        if (ThenarGradient(bigFrame, r, col) < m_thenarDecayGrad) nState = 2;
                    }
                } else {
                    if (sig < enterThold) {
                        SaveThenarFeature(static_cast<int16_t>(r), len, width, edge, peak);
                        nState = 0; len = 0; width = 0; peak = 0;
                    } else if (ThenarGradient(bigFrame, r, col) < m_thenarRestartGrad) {
                        ++len;
                        peak = std::max(peak, sig);
                        width = std::max(width, OrthogonalWidth(bigFrame, r, col));
                    } else {
                        SaveThenarFeature(static_cast<int16_t>(r), len, width, edge, peak);
                        nState = 1; len = 1; peak = sig;
                        width = OrthogonalWidth(bigFrame, r, col);
                    }
                }
            }
            if (nState != 0) SaveThenarFeature(0, len, width, edge, peak);
        }
    }

    /// 梯度核 [[1,2,1],[0,0,0],[−1,−2,−1]]，c 沿 dim1、e 沿 dim2。
    [[nodiscard]] static int32_t ThenarGradient(const TouchBigFrame& bf, int r, int c) noexcept {
        const int rm = std::max(0, r - 1), rp = std::min(kRows - 1, r + 1);
        const int cm = std::max(0, c - 1), cp = std::min(kCols - 1, c + 1);
        const int32_t upper = bf.diff[rm][cm] + 2 * bf.diff[rm][c] + bf.diff[rm][cp];
        const int32_t lower = bf.diff[rp][cm] + 2 * bf.diff[rp][c] + bf.diff[rp][cp];
        return upper - lower;
    }

    /// 沿 dim1 两侧连续 > DAT_6bc4a914(600) 的格数。
    [[nodiscard]] uint16_t OrthogonalWidth(const TouchBigFrame& bf, int r, int c) const noexcept {
        uint16_t w = 0;
        for (int cc = c + 1; cc < kCols; ++cc) { if (bf.diff[r][cc] <= m_thenarWidthThold) break; ++w; }
        for (int cc = c - 1; cc >= 0; --cc)    { if (bf.diff[r][cc] <= m_thenarWidthThold) break; ++w; }
        return w;
    }

    void SaveThenarFeature(int16_t coord, uint16_t len, uint16_t width,
                           uint8_t edge, int16_t peak) {
        if (m_thenarFeatures.size() >= kMaxThenarFeatures) return;
        ThenarFeatureUnit f{};
        f.coord = coord; f.len = len; f.width = width; f.edge = edge; f.peak = peak;
        m_thenarFeatures.push_back(f);
    }

    /// Thenar_ProcessGensture：检出 ∧ !(hdr+0x24 & 1) ∧ peak > 2000 → [0x44] = 1；
    /// 持续 ≥ 801 ms → [0x45] = 1，保持 100 ms。
    void Thenar_ProcessGensture(const TouchBigFrame& bigFrame, int16_t peak) {
        const uint64_t nowMs = bigFrame.timestampUs / 1000ull;
        const bool raw = m_thenarDetected &&
                         (bigFrame.antiTouchHeaderFlags & 1u) == 0 && peak > 2000;
        if (raw) {
            if (m_thenarGestureRaw == 0) m_thenarGestureStartMs = nowMs;
            m_thenarGestureRaw = 1;
            if (nowMs - m_thenarGestureStartMs >= 801) m_thenarGesture = 1;
        } else {
            m_thenarGestureRaw = 0;
            if (m_thenarGesture != 0 && nowMs - m_thenarGestureStartMs >= 901) {
                m_thenarGesture = 0;   // 保持 100 ms 后落下
            }
        }
    }

    /// Thenar_EventUpdate @0x6BB31?? + Thenar_EventUpdateUnit
    /// 进入计数初值 15、每命中 +12（上限 0x24），退出计数上限 16；
    /// exit > 12 清零 enter；enter > 35 时写 hdr + 0x18 的 bit1 / bit2（带手势再 bit3 / bit4）。
    void Thenar_EventUpdate(TouchBigFrame& bigFrame) {
        Thenar_EventUpdateUnit(HandSideLeft, 0, 1);
        if (m_sideHandEnterCnt[0] > 0x23) {
            bigFrame.thenarHeaderFlags |= 2u;
            if (m_thenarGesture != 0) bigFrame.thenarHeaderFlags |= 8u;
        }
        Thenar_EventUpdateUnit(HandSideRight, 1, 0);
        if (m_sideHandEnterCnt[1] > 0x23) {
            bigFrame.thenarHeaderFlags |= 4u;
            if (m_thenarGesture != 0) bigFrame.thenarHeaderFlags |= 0x10u;
        }
    }

    void Thenar_EventUpdateUnit(int32_t side, size_t idx, size_t other) noexcept {
        if (m_thenarFrameSide == side) {
            if (m_sideHandEnterCnt[idx] < 0x25 && m_sideHandExitCnt[other] > 0xF) {
                m_sideHandEnterCnt[idx] += 0xC;
            }
            m_sideHandExitCnt[idx] = 0;
        } else {
            if (m_sideHandEnterCnt[idx] != 0 && m_sideHandEnterCnt[idx] < 0x24) {
                --m_sideHandEnterCnt[idx];
            }
            if (m_sideHandExitCnt[idx] < 0x10) ++m_sideHandExitCnt[idx];
        }
        if (m_sideHandExitCnt[idx] > 0xC) m_sideHandEnterCnt[idx] = 0;
    }
};

} // namespace Solvers::Touch
