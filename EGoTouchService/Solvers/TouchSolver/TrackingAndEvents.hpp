#pragma once
// ══════════════════════════════════════════════════════════════════════
// TouchPipeline Stage 5: TrackingAndEvents (Header-only C++23)
//
// Reverse engineered from TSACore.dll (0x6BA40000) for W273AS2700
// (Gaokun Himax CSOT, 60 columns x 40 rows = 2400 mutual capacitance nodes).
// Full documentation in docs/finger_algorithms/05_tracking_and_events.md.
//
// Core Subsystems:
//   1. IDT_Process & IDT_HM:
//      - 带势函数的最短增广路匈牙利（Kuhn-Munkres），**不是 JV**；行列势是
//        int32 自然回绕，dim >= 3 起会真的溢出并改变配对 —— 原厂行为，照抄
//      - 2nd-order Newton kinematic prediction & velocity residual penalty
//      - Euclidean & Manhattan distance gating (5 mm -> 300 Q8 pixels)
//      - Strong match anchoring with -0x3FFFFFFF (-1073741823) negative weight
//      - Fixed stack allocation (ZERO heap allocations)
//   2. IDT_AccCheck & IDT_GetAcc:
//      - Instantaneous velocity and acceleration calculation
//      - Kinematic physical consistency check against human joint acceleration limit
//      - Cuts erroneous cross-finger association to prevent ID hopping/swapping
//   3. TS_Process & TS_UpdateCurSizeInMM:
//      - Physical touch size estimation via non-linear quadratic charge equation
//      - Solves K * (r^2 + r) <= M0 * 1024 with area constant 43
//      - Latch-max across frames to prevent sizing flicker
//   4. TE_Process & TE_WindowsLiftOffProcess:
//      - 事件状态字一律用原厂位码 TouchEventCode（文档 05 §5.1）：
//        1 待确认 / 2 按下 / 4 移动 / 8 冻结保持 / 0x10 补报 / 0x20 通用抬起
//        / 0x40 WindowsPad 抬起帧（**唯一 Tip=0 的一帧**）
//      - ID 分配是「1..20 中最小空闲」，不是轮转（TE_GetNextID 的 prevOccupiedID 恒 0）
//      - 普通手指抬起去抖为 0，失配当帧直接出 0x40，不经过 HOLD
//      - 帧尾 state & 0x70 的轨迹整条删除，其中 0x10 的 ID 进黑名单一帧
//      - Dual injective/surjective assertion check (TE_AssertProcess)
//   5. ER_Process:
//      - 2nd-order Taylor displacement extrapolation for fast stroke boundary escape
//      - Hardware boundary hard-clamping (0 or Max - 1)
//      - Cancels inward centroid recoil caused by fringing field cut-off
// ══════════════════════════════════════════════════════════════════════

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <array>
#include <algorithm>
#include <cmath>
#include <span>

#include "SolverTypes.h"
#include "TouchCoreTypes.hpp"
#include "TouchFrameTypes.h"

namespace Config {
class ConfigBinder;
class ConfigStore;
}

namespace Solvers::Touch {

// ── 原版触点易失标志字 touch + 0x1f0 / + 0x1f4 的位定义 ────────────────
// 只列出 Z 幅度准入门涉及的几位，命名对位原版 Touch_SetVolatileFlag* 家族。
inline constexpr uint32_t TOUCH_FLAG_DOWN_Z_FAILED      = 0x00000400u; // +0x1f0，按下 Z 不足
inline constexpr uint32_t TOUCH_FLAG_FINGER_Z_FAILED    = 0x00000800u; // +0x1f0，手指 Z 不足
inline constexpr uint32_t TOUCH_FLAG_STABLE_TOUCH       = 0x00001000u; // +0x1f0，稳定触控
inline constexpr uint32_t TOUCH_FLAG_SECONDARY_Z_FAILED = 0x40000000u; // +0x1f0，次级 Z 不足
inline constexpr uint32_t TOUCH_FLAG2_LIFTOFF_Z_FAILED  = 0x00000001u; // +0x1f4 bit0，抬手 Z 不足

// ── B2：Touch_SetVolatileFlagSigSum @0x6BA6F7AF 产出的三位 ────────────
// 比值按 1.0 == 32（原版 `<< 5`）标度，判据门限 0x18 = 0.75、0x28 = 1.25。
inline constexpr uint32_t TOUCH_FLAG_SIGSUM_DROP        = 0x00000002u; // +0x1f0 bit1，+0x1dc < 0x18
inline constexpr uint32_t TOUCH_FLAG_SIGSUM_RISE        = 0x00000001u; // +0x1f0 bit0，+0x1dc > 0x28
inline constexpr uint32_t TOUCH_FLAG_PEAK_Z1_DROP       = 0x00080000u; // +0x1f0 bit19，+0x1de < 0x18

// ── TZ_UpdateTouchFlag @0x6BA7FE99 产出的区域派生位 ───────────────────
// 区域标志（zone + 0x4，见 TouchZoneFlags）到触点标志的一一转换。注意前两位
// 是**反相**的：区域没贴边才置位。
inline constexpr uint32_t TOUCH_FLAG_NOT_EDGE_TOUCHED   = 0x00000004u; // +0x1f0，¬ZONE_FLAG_EDGE_TOUCHED
inline constexpr uint32_t TOUCH_FLAG_ZONE_TYPE5         = 0x00002000u; // +0x1f0，区域 type == 5（大物体）
inline constexpr uint32_t TOUCH_FLAG_BIG_SHAPE_ZONE     = 0x00020000u; // +0x1f0，ZONE_FLAG_BIG_SHAPE
inline constexpr uint32_t TOUCH_FLAG_LOW_NOISE_CMF      = 0x01000000u; // +0x1f0，ZONE_FLAG_LOW_NOISE_CMF
inline constexpr uint32_t TOUCH_FLAG_SURROUNDED         = 0x02000000u; // +0x1f0，ZONE_FLAG_SURROUNDED
inline constexpr uint32_t TOUCH_FLAG_GRIP_MERGE         = 0x10000000u; // +0x1f0，ZONE_FLAG_GRIP_MERGE_FORCED
inline constexpr uint32_t TOUCH_FLAG_GRIP_CHANNEL       = 0x20000000u; // +0x1f0，ZONE_FLAG_NEIGHBOR_NOISE
inline constexpr uint32_t TOUCH_FLAG2_NOT_NEAR_EDGE     = 0x00001000u; // +0x1f4，¬ZONE_FLAG_NEAR_EDGE
inline constexpr uint32_t TOUCH_FLAG2_SIDE_TOUCH        = 0x00002000u; // +0x1f4，ZONE_FLAG_SIDE_TOUCH

// ── 全局触控状态字 g_touchesPtr[0x24] 的位 ───────────────────────────
inline constexpr uint32_t TOUCH_GLOBAL_BIG_SHAPE        = 0x00000008u; // bit3，上报网闸判据 2 读它
inline constexpr uint32_t TOUCH_GLOBAL_SURROUNDED       = 0x00000010u; // bit4

// ── g_tsaStaticPtr->dwSpecialControlFlags 的位 ───────────────────────
inline constexpr uint32_t SPECIAL_CTRL_CMF_NOISE        = 0x00000020u;
inline constexpr uint32_t SPECIAL_CTRL_BIG_OBJECT       = 0x00000800u;

class TrackingAndEvents {
public:
    static constexpr int kRows = kSensorRows;       // 40 行 (Rx / Y轴)
    static constexpr int kCols = kSensorCols;       // 60 列 (Tx / X轴)
    static constexpr int kTotalCellCount = kTotalCells; // 2400 互容节点
    static constexpr size_t kMaxTrackSlots = kMaxTracks; // 20 槽位
    // TS 长轴门限 = (flash[0x1B] >> 3) + 2 = (69 >> 3) + 2 = 10 mm
    static constexpr uint16_t kTsAxisMajorTholdMm = 10;
    // DB_GetNumBySignalSum 门限 = (dyn[+6]·dyn[+0x14]) >> 4 = (48·440) >> 4 = 1320
    static constexpr int32_t kDbSignalSumThold = 1320;

    // ── 模块使能开关 ──────────────────────────────────────────────────
    bool m_enabled = true;

    // ── W273AS2700 专机预置算法参数 ──────────────────────────────────
    float    m_maxMatchDistMM             = 5.0f;     // Flash 0x0888: 匈牙利最大匹配距离 (5 mm)
    float    m_maxAccMM                   = 16.0f;    // Flash 0x088A: 加速度上限门限 (16 mm)
    int32_t  m_pitchPxPerMm               = 60;       // Flash 0x001B (Factor 69): 4096 / 69 + 1 = 60 px/mm
    int32_t  m_reportRateHz               = 100;      // Flash 0x0038: 上报率 (100 Hz)，缺省帧间隔 1000 / 100 = 10 ms
    bool     m_windowsPadFeatureEnabled   = true;     // Flash 0x0054 bit 24: 启用 Windows Precision Touchpad
    int32_t  m_touchDownDebounceFrames    = 0;        // 按下防抖帧数 (0 或 1，默认为 0 保证即时零延迟上报)
    int32_t  m_liftOffDebounceFrames      = 1;        // PTP 抬手维持帧数 (1 帧延时防止笔画断裂)
    uint16_t m_areaScale                  = 43;       // 固件面积标定常数 DAT_6BCAB61E (43)
    int16_t  m_dynamicGain                = 1000;     // Flash 0x05D8: 动态增益 g_tsaPrmtDynamic (1000)
    float    m_minSizeMm                  = 1.0f;     // 最小有效尺寸 (mm)
    float    m_maxSizeMm                  = 14.0f;    // 最大饱和尺寸 (mm)
    int32_t  m_maxTouchCount              = 20;       // 最大支持点数 (20)
    // IDT_PopulateDistanceMap 的 local_e：轨迹帧龄超过该值才享受强吸引
    uint16_t m_alwaysMatchMinAge          = 5;
    bool     m_useHungarian               = true;     // 启用 Kuhn-Munkres 匈牙利二分图匹配
    bool     m_strictAccCheck             = false;    // 严格加速度校验

    // ══════════════════════════════════════════════════════════════════
    // IDT 关联重排（**明确偏离原厂**）
    //
    // 原厂结构是「评分 → 指派 → 事后校验」，有三个互相叠加的缺陷：
    //
    //   A. 评分被并列常数抹平。IDT_PopulateDistanceMap 的「近+老+慢」分支与
    //      IDT_AlwaysMatchProcess 的唯一性检验都把代价格改写成**同一个**
    //      0xC0000001。匈牙利求总和最小，多个格子同值时覆盖它们的任意排列
    //      总代价相同 —— 目标函数在这些配对之间彻底失去分辨力。并拢双指、
    //      两条轨迹都「近+老+慢」时最容易同时被锚定。
    //
    //   B. 评分被惩罚项破坏。numCur < numPrev（任何一根手指消失）那一帧，
    //      `cost += 2*((dx−vx)² + (dy−vy)²)` 加在锚定常数之上，量级 1e8~1e9，
    //      锚定值被不同幅度抬起。惩罚项本身量纲还是错的：位移减速度。
    //
    //   C. 校验在指派之后，且不重新求解。IDT_AccCheck 把非法配对置 0xFF 之后，
    //      当前触点领新 ID、历史轨迹走抬起 —— 没有人回头去找那个本来正确的
    //      配对。一次误配同时毁掉两条轨迹的 ID 连续性。
    //
    // 重排后：合法性判据**前移进代价矩阵**（不合法写 kForbidden 哨兵），
    // 求解器只在合法配对里找最优，事后只需剔除落在哨兵上的配对。这样也就
    // 不再需要「二次求解」—— 落在哨兵上说明该行本来就没有任何合法去处。
    // 代价矩阵改用 int64 平凡运算，不再依赖 int32 回绕。
    // ══════════════════════════════════════════════════════════════════

    /// 关掉即回到原厂的「评分→指派→事后校验」，仅供对拍。
    bool     m_idtAdmissibilityInCost     = true;
    /// 位移硬门（mm）。超过即判不合法，不进入指派。并拢双指的指心间距约
    /// 12~18 mm，取 6 mm 可以保证一根手指不会去继承另一根的轨迹；
    /// 换算 10 ms 帧即 0.6 m/s 的位移上限，正常滑动远低于此。
    float    m_idtHardMatchDistMM         = 6.0f;
    /// 「近+老+慢」的优先奖励。不再用并列常数：奖励是一个固定偏置，减去之后
    /// 各锚定配对之间**仍按 dx²+dy² 可比**，只是整体排在非锚定配对之前。
    static constexpr int64_t kIdtAnchorBonus = 1LL << 34;
    /// 不合法配对的哨兵。取值远大于任何真实代价，但 20 行求和仍在 int64 内。
    static constexpr int64_t kIdtForbidden   = 1LL << 40;
    bool     m_edgeExtrapolateEnabled     = true;     // 边缘滑出二阶外推与钳位使能

    // ── 外部测试与兼容字段 ──
    float    m_maxTrackDistance           = 4.985f;
    int      m_stylusSuppressPenPeakThreshold = 1500;
    bool     m_bypassStateMachine        = false;

    // ── 边缘区 (ER) 参数 ─────────────────────────────────────────────
    // PREVTOUCH_HISTORY_BUFFER_SIZE：PrevTouch_HistoryBufferPush 的容量断言 0x1e
    static constexpr uint8_t kErHistoryCap = 30;
    // ER_IsTouchNeedMoveInCorrection 的 `touch + 0x188 < 0x1e`（主轴跨度，单位 mm）
    uint8_t  m_erMaxTouchSizeMm           = 30;
    // ER_MoveInCheck 的两个平方距离门限（原版立即数 0x9c4 与 10000，Q8 传感器坐标）
    uint32_t m_erMinStepSqDist            = 2500;
    uint32_t m_erMinTravelSqDist          = 10000;

    // ── 内部跨帧轨迹状态结构体 (维持 20 个独立槽位) ──────────────────
    struct InternalTrack {
        uint8_t  id = 0;                  // 绑定的 Tracking ID [1..20]
        uint16_t x_q8 = 0;                // 当前工作坐标 X (Q8.8 定点数)
        uint16_t y_q8 = 0;                // 当前工作坐标 Y (Q8.8 定点数)
        uint16_t prevX_q8 = 0;            // 上一帧工作坐标 X
        uint16_t prevY_q8 = 0;            // 上一帧工作坐标 Y
        uint16_t downX_q8 = 0;            // 首次按下坐标 X
        uint16_t downY_q8 = 0;            // 首次按下坐标 Y
        int16_t  vx_q8 = 0;               // 瞬时平滑速度 X
        int16_t  vy_q8 = 0;               // 瞬时平滑速度 Y
        int16_t  prevVx_q8 = 0;           // 前一帧速度 X (用于加速度与二阶外推)
        int16_t  prevVy_q8 = 0;           // 前一帧速度 Y
        int16_t  ax_q8 = 0;               // 瞬时加速度 X
        int16_t  ay_q8 = 0;               // 瞬时加速度 Y
        int16_t  velXPerMs = 0;           // 归一化到毫秒的速度 X (IDT 触点 +0x4CC)
        int16_t  velYPerMs = 0;           // 归一化到毫秒的速度 Y (IDT 触点 +0x4CE)
        // ── IDT 真正读的一组：原始质心（EC 前）与由它导出的运动学 ─────────
        uint16_t rawX_q8 = 0;             // +0x15e，IDT 全部距离与速度的来源
        uint16_t rawY_q8 = 0;             // +0x160
        int16_t  dt150 = 0;               // +0x150，本帧 dt（ms）
        int16_t  accRawX = 0;             // +0x4d0，v − v_old（原始坐标）
        int16_t  accRawY = 0;             // +0x4d2
        int16_t  dWrkX = 0;               // +0x4d4，工作坐标帧位移
        int16_t  dWrkY = 0;               // +0x4d6
        int16_t  ddWrkX = 0;              // +0x4d8，工作坐标二阶差分
        int16_t  ddWrkY = 0;              // +0x4da
        int32_t  sumWrkX = 0;             // +0x4dc，工作坐标累计位移
        int32_t  sumWrkY = 0;             // +0x4e0
        uint16_t sqMoveMm2 = 0;           // +0x240，本帧移动距离平方（mm²）
        uint16_t sqFromDownMm2 = 0;       // +0x24c，距按下点距离平方（mm²）
        int32_t  prevM0 = 0;              // 上一帧 +0x1cc，DB_GetNumBySignalSum 用
        int16_t  prevMinEllipseRate = 0;  // 上一帧 +0x1a6，DB_FastClickProcess 用
        uint8_t  prevAxisMajorMm = 0;     // 上一帧 +0x188，同上
        uint32_t prevVolatileFlags = 0;   // 上一帧 +0x1f0，第 9 步读它
        uint16_t maxSqMoveMm2 = 0;        // +0x4f6，+0x240 的历史最大值
        int32_t  gripStage210 = 0;        // +0x210，握持阶段（模块 06 写）
        uint32_t persist1FC = 0;          // +0x1fc，持久标志（bit10 = 按下已确认）
        bool     peakAbsGrip = false;     // GripFilter_IsPeakAbsGrip(峰下标) 的快照
        float    sizeMm = 0.0f;           // 物理接触尺寸 (mm)，= sizeMmU8 的浮点镜像
        uint8_t  sizeMmU8 = 0;            // +0x194，**生命周期内单调锁存**
        uint8_t  sizeUdfpMm = 0;          // +0x195，UDFP 尺寸，不锁存
        uint16_t outX_q8 = 0;             // +0x170，每帧输出坐标槽（ER 修正写它）
        uint16_t outY_q8 = 0;             // +0x172
        uint8_t  axisMajorMm = 0;         // 形态长轴 (mm, 来自 TZ_GetAxis)
        uint8_t  axisMinorMm = 0;         // 形态短轴 (mm)
        // 原版 touch + 0x22a：TZ_UnitToTouch 入口拍下的**原始**长轴 mm，未经 FF/MF 修正。
        uint16_t rawAxisMajorMm = 0;
        // 原版 touch + 0x228：+0x22a 在整条轨迹生命周期内的历史最大值。
        // 唯一写入者 Touch_ProcessAftTouchEventUnit @0x6BA7542A，消费者是模块 06
        // 的 GripFilter 全族、TZ_IsSD 与 StylusAft。文档 05 §5.4。
        uint16_t maxAxisMajorMm = 0;
        uint8_t  extentX = 0;             // 包围盒列跨度 (节点)
        uint8_t  extentY = 0;             // 包围盒行跨度 (节点)
        // ── Touch_IIRDataProcess @0x6BA7474F 的 8 个输入（原值）与 8 个输出（EMA）──
        // 输入：+0x188/+0x18a（长短轴 mm）、+0x18c/+0x18e（长短轴**像素**）、
        //       +0x192（主轴角）、+0x1a6（最小椭圆率）、+0x1a8/+0x1aa（grip ratio）。
        uint16_t axisMajorPx = 0;         // +0x18c
        uint16_t axisMinorPx = 0;         // +0x18e
        int8_t   axisAngleDeg = 0;        // +0x192
        int16_t  minEllipseRate = 0;      // +0x1a6
        uint16_t gripRatioOuter = 0;      // +0x1a8
        uint16_t gripRatioInner = 0;      // +0x1aa
        // 输出：+0x25c 快照有效标志 + +0x25e..+0x26c 的 7:1 EMA。模块 06 的
        // AbsGrip / TempGrip 比率判据只读这一组，文档 05 §5.4。
        uint8_t  shapeSnapshotValid = 0;  // +0x25c
        uint16_t iirAxisMajorMm = 0;      // +0x25e
        uint16_t iirAxisMinorMm = 0;      // +0x260
        uint16_t iirAxisMajorPx = 0;      // +0x262
        uint16_t iirAxisMinorPx = 0;      // +0x264（为 0 时钳到 1）
        int8_t   iirAxisAngleDeg = 0;     // +0x266
        uint8_t  axisRatioX16 = 0;        // +0x267 = (+0x262 << 4) / +0x264
        int16_t  iirMinEllipseRate = 0;   // +0x268
        uint16_t iirGripRatioOuter = 0;   // +0x26a
        uint16_t iirGripRatioInner = 0;   // +0x26c
        int32_t  prevZoneType = 0;        // 上一帧 +0x1e0，供首帧直通判据使用
        uint16_t ellipseRate = 0;         // 形态椭圆率 ×100
        bool     fromMultiFinger = false; // 源区域由 SubTZ 拆分产生
        uint8_t  gripRatio = 0;           // 边沿能量比
        uint8_t  gripEdgeFlags = 0;       // 贴边方向掩码
        uint8_t  gripLevel = 0;           // 重心处握持区等级
        // 原版 touch + 0x22c / +0x22e：Touch_SetEdgeDist 从补偿后重心算出的距边距离
        uint16_t edgeDistDim1_q8 = 0;
        uint16_t edgeDistDim2_q8 = 0;
        uint16_t age = 0;                 // 连续存活帧数
        // 原版 prevTouch + 0x4e4：**按下与抬起共用同一个计数器**（文档 05 §1.3）。
        uint16_t debounceCounter = 0;     // 按下/抬手防抖计数器
        // 原版 touch + 0x238 (wDebounceLimit)：动态防抖门限目标值。
        // ER_TouchDownProcess 延后一次按下时把它抬到至少 2。
        uint16_t debounceLimit = 0;
        uint16_t missingFrames = 0;       // 丢失未匹配连续帧数
        // 原版 touch + 0x21c。取值一律用 TouchCoreTypes.hpp 的 TouchEventCode：
        //   EventIdle 1 / EventDown 2 / EventMove 4 / EventHold 8 /
        //   EventRescue 0x10 / EventUpFinal 0x20 / EventUpReport 0x40
        // 见文档 05 §5.1。
        uint32_t state = 0;
        // 原版 touch + 0x1e0（TZ 区域类型）。DB_LiftOffNum @0x6BA55E79 读它。
        int32_t  zoneType = 0;
        uint32_t erFlags = 0;             // 边缘外推标志 (0x10=Left, 0x20=Right, 0x40=Top, 0x80=Bottom)

        // ── 边缘区入边链路的跨帧状态 (ER_TouchDownProcess) ─────────────
        // PrevTouch_HistoryBuffer：index 0 最旧，Push 追加在尾部。
        uint16_t histX[kErHistoryCap] = {};
        uint16_t histY[kErHistoryCap] = {};
        uint8_t  histNum = 0;
        uint16_t erMoveInCount = 0;       // prevTouch + 0x4e4：按下被延后的次数，上限 2
        bool     erDeferPending = false;  // touch + 0x1f8 bit10 (0x400)：本帧按下被延后
        bool     erHistoryPending = false;// touch + 0x1f8 bit7  (0x80) ：历史缓冲待回放
        bool     erCorrected = false;     // touch + 0x1f0 bit5  (0x20) ：入边已修正
        // touch + 0x1f8 bit3（LifeReportAdmit）的跨帧影子。原版由 Stage 7 的
        // TouchReport_ProcessUnit 置位、PrevTouch_UnitCopy 带到下一帧，ER 读的是
        // **上一帧**的值；复刻的 Stage 7 写在 TrackingSlot 上，每帧都会被 Stage 5
        // 的回写覆盖掉，所以这里按"上一帧 isReported"自行锁存一份。
        bool     reportAdmitted = false;
        uint32_t lifeFlags = 0;           // 生命周期标志位
        // 原版 touch + 0x1f8 持久标志字（Touch_SetPersistFlag 每帧 OR 上历史值）。
        //   8 已上报过 / 0x80 移入需回放 / 0x400 移入延后确认 / 0x40000 快速点击候选
        //   / 0x200000 强制断开 ID / 0x20000000 见 08。文档 05 §1.2。
        uint32_t persist1F8 = 0;
        uint32_t volatileFlags = 0;       // 原版 touch + 0x1f0，每帧重算
        uint32_t volatileFlags2 = 0;      // 原版 touch + 0x1f4，每帧重算
        uint32_t persistFlags = 0;        // 原版 touch + 0x218，bit0 = 区域被握持分离
        uint32_t gripChannelFlags = 0;    // 原版 touch + 0x214
        uint32_t postureFlags = 0;        // 原版 touch + 0x200，bit1 = 多胖指
        uint32_t exceptionType = 0;       // 原版 touch + 0x1ec，来自区域 + 0x350
        int16_t  centSig = 0;             // 原版 touch + 0x29e，来自区域 + 0x2e
        int16_t  minSigLatched = 0;       // 原版 touch + 0x1ba，跨帧锁存，不逐帧清
        int16_t  peakSig = 0;             // 本帧源峰的中心值 (peak + 0x18 → touch + 0x1be)
        int32_t  peakZ4 = 0;              // 本帧源峰的邻域能量 (peak + 0x20 → touch + 0x1c0)
        int16_t  prevPeakSig = 0;         // 上一帧的 peakSig (prevTouch + 0x1be)
        int32_t  prevPeakZ4 = 0;          // 上一帧的 peakZ4 (prevTouch + 0x1c0)
        // B2：原版 touch + 0x23C —— TZ_RegisterTouch @0x6BA8039B 从 TZ 单元 +0x2B6
        // （TZ_GetMaxMinPkIdx 选出的 maxSig 峰）拷来的**本帧峰下标**。
        // Touch_SetVolatileFlagSigSum 用它索引 g_pPeaks，用 prevTouch + 0x23C
        // 索引 g_prevPeaksPtr。
        uint8_t  peakIndex = 0;           // touch + 0x23c
        uint8_t  prevPeakIndex = 0;       // prevTouch + 0x23c
        // B2：Touch_SetVolatileFlagSigSum 产出的三个跨帧能量比（1.0 == 32）。
        // 缺省 255 = 无衰减，对位原版「分母为 0 / 历史峰下标越界」的兜底。
        uint8_t  sigSumRatio = 255;       // touch + 0x1dc
        uint8_t  znRatio = 255;           // touch + 0x1dd
        uint8_t  z1Ratio = 255;           // touch + 0x1de
        int32_t  m0 = 0;                  // 零阶矩电荷量
        uint16_t area = 0;                // 像素单元数
        bool     isEdge = false;          // 是否为边缘触点
        bool     isPalm = false;          // 手掌抑制标记
        bool     isGrip = false;          // 握持阻断标记
        bool     isReported = false;      // 是否向上层/系统呈现
        bool     active = false;          // 槽位是否被占用
        uint8_t  sourcePeakId = 0;        // 上游峰值 ID (弱关联提示)
        uint8_t  sourcePeakAge = 0;
    };

    TrackingAndEvents() noexcept {
        Reset();
    }

    /// 异常帧保持：Stage 0.5 判定 TSA_IsToBypassCurrentFrame 为真时调用。
    /// 语义见 Touch_KeepPrevTouchWithExceptionExcluded @0x6BA766B9（文档 05 §8）。
    void KeepPrevTouchForBypassedFrame() noexcept {
        Touch_KeepPrevTouchWithExceptionExcluded();
    }

    void Reset() noexcept {
        m_tracks.fill(InternalTrack{});
        // 文档 05 §7.4 列出的跨帧状态，Reset 是唯一允许整体清空的地方。
        m_bufTimeDown.fill(0);
        m_bufMaxSigDown.fill(0);
        m_bufTimeOff.fill(0);
        m_bufMinSigOff.fill(0);
        m_bufTimeRpt.fill(0);
        m_bufTzZonePrev.fill(0);
        m_bufTzPrev.fill(0);
        m_invalidIds.fill(0);
        m_invalidIdCount = 0;
        m_defaultSizeMm = 7;           // flash[0x8F9]
        m_liftOffStableCount = 0;
        m_prevExceptionWord = 0;
        m_peakCount = 0;
        m_prevPeakCount = 0;
        m_assertFailed = false;
        m_hasLiveTracks = false;
        m_activeMask = 0;
        m_prevFrameTsUs = 0;
        m_frameIntervalMs = (m_reportRateHz > 0) ? (1000 / m_reportRateHz) : 8;
    }

    void SetEnabled(bool enabled) noexcept { m_enabled = enabled; }
    [[nodiscard]] bool IsEnabled() const noexcept { return m_enabled; }
    [[nodiscard]] bool HasLiveTracks() const noexcept { return m_hasLiveTracks; }
    [[nodiscard]] uint32_t GetActiveMask() const noexcept { return m_activeMask; }
    void ClearLiveState() noexcept {
        for (auto& t : m_tracks) {
            t = InternalTrack{};
        }
        m_hasLiveTracks = false;
        m_activeMask = 0;
    }

    // ── 核心入口 1: 针对共享大帧 TouchBigFrame 的零拷贝原位处理 ────────
    bool Process(TouchBigFrame& bigFrame) {
        if (!m_enabled) {
            bigFrame.tracks.clear();
            bigFrame.contacts.clear();
            bigFrame.hasLiveTouches = false;
            bigFrame.activeTouchMask = 0;
            return true;
        }

        // 0.0 回灌上一帧的握持阶段 +0x210。原版它和 IDT 读的历史单元是同一块
        //     内存；复刻里模块 06 的 GripFilter_GetStage @0x6BAE492D 写在
        //     bigFrame.tracks 上（本帧尚未重建，仍是上一帧的内容），这里搬进
        //     InternalTrack 供 IDT_IsToIncAccThold 读（文档 06 §4.5）。
        // 0.0-B3 同一趟回灌 +0x1F8 bit29「该轨迹已首次上报」。
        //   写入者：TSAOut_PostProcessUnit @0x6BA6A6AA（模块 08）—— 它写的不是
        //   本帧触点池，而是 **g_pPrevTouches[PrevTouch_GetIdxBasedOnID(+0x154)]
        //   + 0x1F8**，同时把 pFrameBufferPtr 记进历史 +0x598，一次性锁存。
        //   回灌者：Touch_SetPersistFlag @0x6BA72776 结尾的
        //   `cur[0x1F8] |= prev[0x1F8]`，即**下一帧**模块 05 把它拉回当前池。
        //   消费者：TSABuffer_TimeRptProcess @0x6BA8EB26（模块 05，TZ 之前，读历史池）
        //   与 PrevTouch_RemoveOff @0x6BA6DF8F（轨迹死亡时记录最后抬手点）。
        //   ★ 07 文档 §15 把 Filter_ReportedProcessUnit @0x6BA5FB41 列为读点，
        //     反编译不支持：那里读的是 +0x1F8 的 bit3 与 bit17；
        //     TouchReport_ToBeReported @0x6BA7B06C 读的也是 **+0x1F0** bit29
        //     （握持通道位），不是 +0x1F8。以反编译为准。
        //   复刻里模块 08 写在 TrackingSlot 上（此刻仍是上一帧的内容），
        //   这里按原版的 OR 语义搬回 InternalTrack 的持久池。
        for (const auto& s : bigFrame.tracks) {
            for (auto& t : m_tracks) {
                if (t.active && t.id == s.id) {
                    t.gripStage210 = s.gripStage210;
                    t.persist1F8 |= (s.persistFlags1F8 & 0x20000000u);
                    break;
                }
            }
        }

        // 0. 帧间隔归一化 (Touch_GetFormattedTimeInterval @ 0x6BA76329)
        //    IDT 的速度/加速度全部按毫秒计量，不是按帧计量
        UpdateFrameInterval(bigFrame.timestampUs);

        // 0.5 TSABuffer_PrevFrameProcess：原版在 TZ **之前**执行。复刻里 Stage 4
        //     跑在 Stage 5 之前，这里是本模块能拿到的最早落点，语义上等于
        //     「为下一帧的 TZ 准备 g_tsaBufTimeRpt」。文档 05 §7.2。
        TSABuffer_PrevFrameProcess(bigFrame);
        // 原版 g_tsaStaticPtr->nBaselineStageInitialSignal：帧尾的
        // TSABuffer_GripTimeProcess 用它推进逐格握持驻留计时。
        bigFrame.frameIntervalMs = m_frameIntervalMs;

        // 峰数快照：DB_TouchDownNum 第 1 步用「峰数 − 上帧峰数」
        m_prevPeakCount = m_peakCount;
        m_peakCount = static_cast<uint8_t>(std::min<size_t>(bigFrame.peaks.size(), 255));

        // 1. 获取 Stage 4 聚类区域列表 (最多 20 个候选区域)
        const size_t curCount = std::min<size_t>(bigFrame.zones.size(), kMaxTrackSlots);

        // 统计当前活跃历史触点数
        size_t activeTrackCount = 0;
        std::array<int, kMaxTrackSlots> activeTrackIndices{};
        for (size_t i = 0; i < kMaxTrackSlots; ++i) {
            if (m_tracks[i].active) {
                activeTrackIndices[activeTrackCount++] = static_cast<int>(i);
            }
        }

        // 2. 求解二分图最优数据关联 (IDT_Process -> IDT_HMProcess)
        // matchCurToTrack: 当前候选点 i 匹配的历史槽位索引 (-1 表示未关联)
        // matchTrackToCur: 历史槽位 j 匹配的当前候选点索引 (-1 表示抬手丢失)
        std::array<int, kMaxTrackSlots> matchCurToTrack{};
        std::array<int, kMaxTrackSlots> matchTrackToCur{};
        matchCurToTrack.fill(-1);
        matchTrackToCur.fill(-1);

        // TZ_UpdateTouchFlag 在整帧内累加的两个全局字，每帧重建。
        // IDT_AccCheck 也会往 dwSpecialControlFlags 打 bit2，故先清零再跑 IDT。
        m_frameGlobalFlags = 0;
        m_frameSpecialFlags = 0;

        // 2/3. IDT_Process(0)：代价矩阵 → 匈牙利 → 加速度校验 → 抄 ID
        if (curCount > 0 && activeTrackCount > 0) {
            IDT_Process(bigFrame.zones.span().subspan(0, curCount),
                        activeTrackIndices, activeTrackCount,
                        matchCurToTrack, matchTrackToCur,
                        /*strictMode=*/m_strictAccCheck);
        }

        // 4. 触控事件生命周期状态机推进与物理接触尺寸解算 (TE_Process & TS_Process)
        UpdateLifecycleAndSizes(bigFrame, bigFrame.zones.span().subspan(0, curCount),
                                activeTrackIndices, activeTrackCount,
                                matchCurToTrack, matchTrackToCur);

        // 4.5 一致性断言 (TE_AssertProcess @0x6BA7872D)，仍在 TE 内部
        AssertAndCleanTracks();

        // 4.6 TE 收尾 (Touch_ProcessAftTouchEvent @0x6BA75AF5)。原版位置就是
        //     TE_Process 尾部、TSABuffer_PreProcess 与 ER 之前，见文档 05 §5.2。
        Touch_ProcessAftTouchEvent();

        // 4.7 逐格 TZ 时间/信号统计 (TSABuffer_PreProcess @0x6BA8F37F)，在 TE 与 ER 之间
        TSABuffer_PreProcess(bigFrame);

        // 5. 边缘区入边/出边修正 (ER_Process)
        if (m_edgeExtrapolateEnabled) {
            ApplyEdgeRegionProcess();
        }

        // 6.5 全局触控状态字回写。原版 TZ_UpdateTouchFlag 直接改
        //     g_touchesPtr[0x24] 与 g_tsaStaticPtr->dwSpecialControlFlags，
        //     复刻在帧内累加后统一挂到大帧上，供 Stage 7 的上报网闸读。
        bigFrame.touchGlobalFlags |= m_frameGlobalFlags;
        bigFrame.specialControlFlags |= m_frameSpecialFlags;

        // 7. 同步写出至 bigFrame.tracks 与 bigFrame.contacts
        bigFrame.tracks.clear();
        bigFrame.contacts.clear();
        m_activeMask = 0;
        // PrevTouch_RemoveOff 入口先清空 ID 黑名单再逐条重建（文档 05 §7.3）。
        // 必须晚于 TE_GetNextID，后者读的是**上一帧**留下的名单。
        m_invalidIdCount = 0;

        for (size_t i = 0; i < kMaxTrackSlots; ++i) {
            auto& t = m_tracks[i];
            if (!t.active) continue;

            // touch + 0x1f8 bit3（已上报过）的跨帧锁存。原版由 07 的
            // TouchReport_ProcessUnit 置位、PrevTouch_UnitCopy 带到下一帧。
            if (t.isReported) {
                t.reportAdmitted = true;
                t.persist1F8 |= 0x8u;
            }

            // TE 状态在输出层的唯一解释点（TouchCoreTypes::MapToPtp，对位
            // TSAOut_PostProcessUnit @0x6BA6A3EF）。文档 05 §5.9。
            const PtpDisposition ptp = MapToPtp(static_cast<int>(t.state));

            // ER 的两个持久位并进 +0x1f8，供 07 的回放通路与 DB_LiftOffNum 读取
            if (t.erDeferPending)   t.persist1F8 |= 0x400u; else t.persist1F8 &= ~0x400u;
            if (t.erHistoryPending) t.persist1F8 |= 0x80u;

            // TS_PostProcess @0x6BA7C368：以 0x20 结束的触点改写默认尺寸
            TS_PostProcess(t);

            // 填充 TrackingSlot 供下游 Stage 6/7/8 使用
            TrackingSlot slot{};
            slot.id = t.id;
            slot.state = static_cast<int>(t.state);
            slot.curX_q8 = t.x_q8;
            slot.curY_q8 = t.y_q8;
            slot.prevX_q8 = t.prevX_q8;
            slot.prevY_q8 = t.prevY_q8;
            slot.vx_q8 = t.vx_q8;
            slot.vy_q8 = t.vy_q8;
            slot.ax_q8 = t.ax_q8;
            slot.ay_q8 = t.ay_q8;
            slot.sizeMm = t.sizeMm;
            slot.m0 = static_cast<int32_t>(t.m0);
            slot.axisMajorMm = t.axisMajorMm;
            slot.axisMinorMm = t.axisMinorMm;
            slot.extentX = t.extentX;
            slot.extentY = t.extentY;
            slot.ellipseRate = t.ellipseRate;
            slot.fromMultiFinger = t.fromMultiFinger;
            slot.gripRatio = t.gripRatio;
            slot.gripEdgeFlags = t.gripEdgeFlags;
            slot.edgeDistDim1_q8 = t.edgeDistDim1_q8;
            slot.edgeDistDim2_q8 = t.edgeDistDim2_q8;
            slot.gripLevel = t.gripLevel;
            slot.age = t.age;
            slot.debounce = (t.state == EventIdle) ? t.debounceCounter : 0;
            // TE 收尾产出，供模块 06 的比率判据与模块 04 的 TZ_GetDBShape 消费
            slot.rawAxisMajorMm     = t.rawAxisMajorMm;
            slot.maxAxisMajorMm     = t.maxAxisMajorMm;
            slot.shapeSnapshotValid = t.shapeSnapshotValid;
            slot.iirAxisMajorMm     = t.iirAxisMajorMm;
            slot.iirAxisMinorMm     = t.iirAxisMinorMm;
            slot.iirAxisMajorPx     = t.iirAxisMajorPx;
            slot.iirAxisMinorPx     = t.iirAxisMinorPx;
            slot.iirAxisAngleDeg    = t.iirAxisAngleDeg;
            slot.axisRatioX16       = t.axisRatioX16;
            slot.iirMinEllipseRate  = t.iirMinEllipseRate;
            slot.iirGripRatioOuter  = t.iirGripRatioOuter;
            slot.iirGripRatioInner  = t.iirGripRatioInner;
            slot.persistFlags1F8    = t.persist1F8;
            // ER_MoveOutCorrection 钉住的上报坐标（+0x170/+0x172）。Stage 7/8 的
            // 滤波与投影应以它为准，跨帧跟踪坐标 curX/curY 保持未受污染。
            slot.outX_q8 = t.outX_q8;
            slot.outY_q8 = t.outY_q8;
            slot.lifeFlags = t.lifeFlags;
            slot.volatileFlags = t.volatileFlags;
            slot.volatileFlags2 = t.volatileFlags2;
            slot.persistFlags = t.persistFlags;
            slot.gripChannelFlags = t.gripChannelFlags;
            slot.postureFlags = t.postureFlags;
            slot.exceptionType = t.exceptionType;
            slot.minSigLatched = t.minSigLatched;
            // B2：Touch_SetVolatileFlagSigSum @0x6BA6F7AF 的三个跨帧能量比，
            // 模块 07 的冻结分支 1（+0x1DD/+0x1DE）与 Filter_IsHandLift
            // @0x6BA5C644（+0x1DC）读它们。
            slot.sigSumRatio = t.sigSumRatio;
            slot.znRatio     = t.znRatio;
            slot.z1Ratio     = t.z1Ratio;
            slot.peakSig     = t.peakSig;
            slot.isPalm = t.isPalm;
            slot.isGrip = t.isGrip;
            slot.isReported = t.isReported;
            bigFrame.tracks.push_back(slot);

            // 进包与否完全由 MapToPtp 决定：EventUpFinal 永不进包，
            // EventUpReport 是唯一 Tip=0 的一帧，其余在包内都是 Tip=1。
            if (ptp.inPacket) {
                TouchContact c{};
                c.id = t.id;
                c.x = static_cast<float>(t.x_q8) / 256.0f;
                c.y = static_cast<float>(t.y_q8) / 256.0f;
                c.sizeMm = t.sizeMm;
                c.signalSum = t.m0;
                c.area = t.area;
                c.isEdge = t.isEdge;
                c.isReported = true;
                c.sourcePeakId = t.sourcePeakId;
                c.sourcePeakAge = t.sourcePeakAge;
                c.lifeFlags = t.lifeFlags;

                if (!ptp.tip) {
                    // EventUpReport：主机看到的唯一一帧抬起
                    c.state = TouchStateUp;
                    c.reportEvent = TouchReportUp;
                    c.lifeFlags |= LifeLiftOff;
                } else if (t.state == EventDown) {
                    c.state = TouchStateDown;
                    c.reportEvent = TouchReportDown;
                    c.lifeFlags |= LifeNew;
                } else {
                    c.state = TouchStateMove;
                    c.reportEvent = TouchReportMove;
                    c.lifeFlags |= LifeMapped;
                }

                bigFrame.contacts.push_back(c);

                if (ptp.tip) {
                    m_activeMask |= (1u << t.id);
                }
            }

            // 帧尾轨迹生死：对位 PrevTouch_RemoveOff @0x6BA6DE46 的 `state & 0x70`
            // —— EventRescue / EventUpFinal / EventUpReport 三者都当帧删除轨迹，
            // 其中以 EventRescue 结束的 ID 进黑名单一帧，下一帧不得复用。
            if ((t.state & 0x70u) != 0) {
                if (t.state == EventRescue && m_invalidIdCount < kMaxTrackSlots) {
                    m_invalidIds[m_invalidIdCount++] = t.id;
                }
                t = InternalTrack{};
            }
        }

        m_hasLiveTracks = (m_activeMask != 0);
        bigFrame.hasLiveTouches = m_hasLiveTracks;
        bigFrame.activeTouchMask = m_activeMask;

        // TSABuffer_PostProcess @0x6BA8F4C1：帧尾把本帧 TZ 图存成上帧快照。
        TSABuffer_PostProcess(bigFrame);

        return true;
    }

    // ── 核心入口 2: 针对经典 HeatmapFrame 的兼容桥接 ──────────────────
    bool Process(HeatmapFrame& frame) {
        if (!m_enabled) return true;

        if (frame.touch.bigFrame != nullptr) {
            const bool ok = Process(*frame.touch.bigFrame);
            frame.touch.output.contacts.clear();
            for (const auto& c : frame.touch.bigFrame->contacts) {
                frame.touch.output.contacts.push_back(c);
            }
            frame.touch.runtime.hasLiveTouchState = m_hasLiveTracks;
            return ok;
        }

        // 独立模式: 使用堆栈轻量 TouchBigFrame 模拟运算
        TouchBigFrame tempBigFrame{};
        tempBigFrame.ResetPerFrame();

        // 映射已有触点为候选区域 (ZoneCluster)
        for (const auto& c : frame.touch.output.contacts) {
            if (tempBigFrame.zones.size() >= kMaxTrackSlots) break;
            ZoneCluster zc{};
            zc.id = static_cast<uint8_t>(tempBigFrame.zones.size() + 1);
            zc.cogX_q8 = static_cast<uint16_t>(std::clamp<int32_t>(static_cast<int32_t>(c.x * 256.0f), 0, (kCols * 256) - 1));
            zc.cogY_q8 = static_cast<uint16_t>(std::clamp<int32_t>(static_cast<int32_t>(c.y * 256.0f), 0, (kRows * 256) - 1));
            zc.ecX_q8 = zc.cogX_q8;
            zc.ecY_q8 = zc.cogY_q8;
            zc.m0 = c.signalSum > 0 ? c.signalSum : 1200;
            zc.pixelCount = c.area > 0 ? static_cast<uint16_t>(c.area) : 12;
            zc.minC = static_cast<uint8_t>(zc.ecX_q8 / 256);
            zc.maxC = zc.minC;
            zc.minR = static_cast<uint8_t>(zc.ecY_q8 / 256);
            zc.maxR = zc.minR;
            tempBigFrame.zones.push_back(zc);
        }

        const bool ok = Process(tempBigFrame);

        frame.touch.output.contacts.clear();
        for (const auto& c : tempBigFrame.contacts) {
            frame.touch.output.contacts.push_back(c);
        }
        frame.touch.runtime.hasLiveTouchState = m_hasLiveTracks;
        return ok;
    }

    // ── TS_GetSizeInMM @0x6BA7C692（文档 05 §4）──────────────────────────
    //
    //   sig >= 0x200000 → 0xFFFFFFFF（截成 u8 即 255）；
    //   否则 r 从 1 起，当 (uint)(gain*43*(u8)(r+r*r)) <= (uint)(sig<<10) 时 r++，到 15 停。
    //
    // 结果落在 [1,15]，是满足 K·(r²+r) > sig·1024 的最小 r。全程**无符号比较**：
    // 负的 sig 左移后变成大数，会一路走到 15。`(u8)(r+r*r)` 的截断照抄。
    // 43 = DAT_6bcab61e = (0x32400/69)/69；gain = g_tsaPrmtDynamic，初值 1000，
    // 运行时由 TSADynPrmt_GetPkThold 改写。
    [[nodiscard]] uint8_t TS_GetSizeInMM(int32_t gain, int32_t sig) const noexcept {
        if (sig >= 0x200000) return 0xFFu;
        const uint32_t rhs = static_cast<uint32_t>(sig) << 10;
        uint8_t r = 1;
        for (;;) {
            const uint32_t lhs = static_cast<uint32_t>(gain) *
                                 static_cast<uint32_t>(m_areaScale) *
                                 static_cast<uint32_t>(static_cast<uint8_t>(r + r * r));
            if (lhs > rhs) break;
            ++r;
            if (r >= 15) break;
        }
        return r;
    }

    /// TS_UpdateCurSizeInMM：长轴超过 (flash[0x1B]>>3)+2 = 10 mm 时**直接用长轴**
    /// 当尺寸，否则用电荷反解出来的 r。
    [[nodiscard]] uint8_t TS_UpdateCurSizeInMM(const ZoneCluster& z) const noexcept {
        uint8_t r = TS_GetSizeInMM(m_dynamicGain, z.m0);
        if (z.axisMajorMm > kTsAxisMajorTholdMm) {
            r = static_cast<uint8_t>(z.axisMajorMm);
        }
        return r;
    }

    /// TS_UpdateCurSizeInMMForUdfp：**无任何配置门控，每帧对每个触点恒执行**
    /// （差异 05-08）。s = sqrt(长轴 × 短轴)；r ≤ 6 取 r；7..10 线性插值；≥ 11 取 s。
    /// 结果只写 +0x195，不锁存。
    [[nodiscard]] uint8_t TS_UpdateCurSizeInMMForUdfp(const ZoneCluster& z) const noexcept {
        uint8_t r = TS_GetSizeInMM(m_dynamicGain, z.m0);
        const uint32_t prod = static_cast<uint32_t>(z.axisMinorMm) * static_cast<uint32_t>(z.axisMajorMm);
        const int32_t s = static_cast<int32_t>(Misc_SqrtUint32(prod));
        if (r < 11) {
            if (r > 6) {
                const int32_t k = static_cast<int32_t>(r) - 6;
                r = static_cast<uint8_t>(
                    (static_cast<uint32_t>(s * k + static_cast<int32_t>(r) * (4 - k))) >> 2);
            }
        } else {
            r = static_cast<uint8_t>(s);
        }
        return r;
    }

    /// Misc_SqrtUint32：整数平方根（向下取整）
    [[nodiscard]] static uint32_t Misc_SqrtUint32(uint32_t v) noexcept {
        uint32_t r = 0;
        while ((r + 1) * (r + 1) <= v) ++r;
        return r;
    }

    // ── TS_ProcessUnit @0x6BA7C1D7（文档 05 §4）──────────────────────────
    //
    //   if (size == 0 && prevSize == 0) size = DAT_6bcab6ec;   // 默认尺寸
    //   else if (size <= prevSize)      size = prevSize;       // **只增不减的锁存**
    //
    // 尺寸是整条轨迹生命周期内的单调最大值，不衰减（差异 05-08）。因为 r >= 1，
    // 默认值分支对有效下标实际不可达。历史尺寸只在 prevIdx < 0x14 时读取。
    void TS_ProcessUnit(InternalTrack& trk, const ZoneCluster& z, bool hasPrev) const noexcept {
        uint8_t size = TS_UpdateCurSizeInMM(z);
        const uint8_t udfp = TS_UpdateCurSizeInMMForUdfp(z);
        const uint8_t prevSize = hasPrev ? trk.sizeMmU8 : uint8_t{0};

        if (size == 0 && prevSize == 0) {
            size = m_defaultSizeMm;
        } else if (size <= prevSize) {
            size = prevSize;
        }
        trk.sizeMmU8 = size;
        trk.sizeUdfpMm = udfp;
        trk.sizeMm = static_cast<float>(size);
    }

    /// TS_PostProcess @0x6BA7C368：对 state & 0x20 的触点把默认尺寸改成
    /// min(prev.+0x194, 9)，即「最近一次按非 Windows 方式抬起的手指尺寸」。
    /// WindowsPad 下 0x20 极少出现，故本机 DAT_6bcab6ec 基本停在初值 7。
    void TS_PostProcess(const InternalTrack& trk) noexcept {
        if ((trk.state & static_cast<uint32_t>(EventUpFinal)) == 0) return;
        m_defaultSizeMm = std::min<uint8_t>(trk.sizeMmU8, 9);
    }

    /// 兼容入口：外部诊断/配置面板仍按 float 读尺寸。
    [[nodiscard]] float CalculateSizeInMM(int32_t sigSum, float prevSizeMM = 0.0f) const noexcept {
        const uint8_t r = TS_GetSizeInMM(m_dynamicGain, sigSum);
        return std::max(static_cast<float>(r), prevSizeMM);
    }

    // ── ConfigBinder 动态参数绑定接口 ────────────────────────────────
    void registerBindings(Config::ConfigBinder& binder) {
        (void)binder;
    }

    void applyConfig(const Config::ConfigStore& store) {
        (void)store;
    }

private:
    std::array<InternalTrack, kMaxTrackSlots> m_tracks{};
    // TE_AssertProcess 的失败锁存：ID 耗尽或索引不一致时置位，断言点清双池。
    bool     m_assertFailed = false;
    // 原版 DAT_6bcab6ec：默认尺寸，初值 flash[0x8F9] = 7，TS_PostProcess 改写。
    uint8_t  m_defaultSizeMm = 7;
    uint64_t m_prevFrameTsUs = 0;      // 上一帧时间戳，用于解算归一化帧间隔
    int32_t  m_frameIntervalMs = 10;   // 归一化帧间隔 (ms)，IDT 速度/加速度的分母
    bool     m_hasLiveTracks = false;
    uint32_t m_activeMask = 0;
    // TZ_UpdateTouchFlag 写的两个全局字在帧内的累加器，见 Process 第 6.5 步。
    uint32_t m_frameGlobalFlags = 0;    // g_touchesPtr[0x24]
    uint32_t m_frameSpecialFlags = 0;   // g_tsaStaticPtr->dwSpecialControlFlags
    // 原版 static.wField_0x122：TE_LiftOffReportProcess @0x6BA77D4B 维护，到 5 锁住。
    uint16_t m_liftOffStableCount = 0;
    // 原版 g_prevTouchInvalidID[20] / DAT_6bbc68f4：上一帧以 EventRescue(0x10) 结束的
    // 轨迹 ID 黑名单，由 PrevTouch_RemoveOff @0x6BA6DE46 每个 MS 帧清空重建，
    // TE_GetNextID 经 PrevTouch_IsValidId 跳过它们。文档 05 §5.3 / §7.3。
    std::array<uint8_t, kMaxTrackSlots> m_invalidIds{};
    uint8_t  m_invalidIdCount = 0;
    // ── 逐格跨帧缓冲（文档 05 §7.1 / §7.4），不随帧清零 ──────────────────
    std::array<int16_t, kTotalCells> m_bufTimeDown{};    // g_tsaBufTimeDown
    std::array<int16_t, kTotalCells> m_bufMaxSigDown{};  // g_tsaBufMaxSigDown
    std::array<int16_t, kTotalCells> m_bufTimeOff{};     // g_tsaBufTimeOff
    std::array<int16_t, kTotalCells> m_bufMinSigOff{};   // g_tsaBufMinSigOff
    std::array<int16_t, kTotalCells> m_bufTimeRpt{};     // g_tsaBufTimeRpt
    std::array<uint8_t, kTotalCells> m_bufTzZonePrev{};  // g_tsaBufTzZonePrev
    std::array<uint8_t, kTotalCells> m_bufTzPrev{};      // g_tsaBufTzPrev
    // 历史头 +0x20 异常字。Touch_KeepPrevTouchWithExceptionExcluded 把它清 0。
    uint32_t m_prevExceptionWord = 0;
    // DB_TouchDownNum 第 1 步的「峰数 − 上帧峰数」。
    uint8_t  m_peakCount = 0;
    uint8_t  m_prevPeakCount = 0;

    // ── 1. 匈牙利二分图最优数据关联 (IDT_Process -> IDT_HMProcess) ────
    /// IDT_IsEdgeTouch @0x6BA622E9：col = X>>8、row = Y>>8，本机
    /// dwFeatureFlags & 0x8000 == 0（TouchGates 的 FEAT_HIGH_SENSE 同族边缘内缩
    /// 开关关闭），故 col == 0 || col >= 59 || row == 0 || row >= 39 即为边缘。
    /// 边缘触点不进 AlwaysMatch 候选表。文档 05 §2.2。
    [[nodiscard]] static constexpr bool IDT_IsEdgeTouch(uint16_t x_q8, uint16_t y_q8) noexcept {
        const int col = x_q8 >> 8;
        const int row = y_q8 >> 8;
        return (col == 0 || col >= kCols - 1 || row == 0 || row >= kRows - 1);
    }

    /// TS_MMToPixel(mm) = mm × DAT_6bca b61a，本机 4096/flash[0x1B] + 1 = 60。
    [[nodiscard]] int32_t TS_MMToPixel(int32_t mm) const noexcept {
        return mm * m_pitchPxPerMm;
    }

    // ══════════════════════════════════════════════════════════════════
    //  IDT 引擎（文档 05 §2）
    //
    //  IDT_Process(mode) @0x6BA611A6 = IDT_HMProcess → IDT_AccCheck → IDT_Map。
    //  scratch[j] 以**历史下标 j** 为索引，值为匹配到的当前下标，0xFF 未匹配。
    //  引擎无任何跨帧静态状态（IDT_Init 是空函数）。
    //
    //  ⚠ 三处**原厂缺陷照抄，不得修正**：
    //   1. 代价矩阵的行列势是 int32 **自然回绕**（§2.4：dim ≥ 3 起 2.5%→14% 的
    //      帧会溢出并**改变配对**）。一律走 Fixed::WrapAdd32 / WrapSub32，
    //      禁止改 int64 或饱和。
    //   2. IDT_PopulateDistanceMap 的矩阵下标累加器 local_b / local_c 是 uint8，
    //      当前下标 ≥ 13 时 i*20 越过 255 回绕。
    //   3. IDT_AlwaysMatchProcess 写回时下标同样截成 uint8（0x6BA622B0 的
    //      `MOV byte ptr [RBP-0x35],AL`）。
    // ══════════════════════════════════════════════════════════════════

    static constexpr int kIdtMatDim = 20;   ///< 矩阵恒为 int32[400]，列主序

    struct IdtContext {
        std::span<const ZoneCluster> cur;                   ///< 当前集合（TZ 触点）
        const std::array<int, kMaxTrackSlots>* prevSlots;   ///< 历史下标 → 轨迹槽位
        size_t numCur = 0;
        size_t numPrev = 0;
        bool   strictMode = false;                          ///< IDT_Process 的 mode 参数
    };

    [[nodiscard]] const InternalTrack& PrevOf(const IdtContext& ctx, size_t j) const noexcept {
        return m_tracks[(*ctx.prevSlots)[j]];
    }

    // ── IDT_GetAcc @0x6BA61704 ──────────────────────────────────────────
    //
    //   dt = |static.dt|（异常字 0x100 的时间戳分支本机不可达）；dt == 0 → 1
    //   vx = (cur.X15E − prev.X15E) / dt         // C 截断除法
    //   ax = vx − prev.v4CC
    //   return (prev.age == 1) ? (vx²+vy²) >> 2 : min(ax²+ay², vx²+vy²)
    //
    // 即只有「速度与加速度都大」才判为不合理；轨迹第二帧没有可靠速度，只看速度
    // 并除以 4。换算：t = 3600 ⇔ v = 60 Q8/ms = 1 mm/ms = 1 m/s。
    [[nodiscard]] int32_t IDT_GetAcc(const ZoneCluster& cur, const InternalTrack& prev) const noexcept {
        // ⚠ 偏离原厂：分母固定为**标称**帧间隔，不用实测 m_frameIntervalMs。
        // 门限是按速度定的，而手指在一帧里能走多远是物理常数；帧间隔抖动或掉
        // 一帧时，实测 dt 变大会让允许位移等比放大（掉一帧 ⇒ 门限翻倍），这在
        // 原厂的恒定帧率下不会发生，在本机会。改成按标称 dt 折算即按位移设门。
        int32_t dt = m_idtAdmissibilityInCost
                         ? NominalFrameIntervalMs()
                         : (m_frameIntervalMs < 0 ? -m_frameIntervalMs : m_frameIntervalMs);
        if (dt == 0) dt = 1;

        const int32_t vx = (static_cast<int32_t>(cur.cogX_q8) - static_cast<int32_t>(prev.rawX_q8)) / dt;
        const int32_t vy = (static_cast<int32_t>(cur.cogY_q8) - static_cast<int32_t>(prev.rawY_q8)) / dt;
        const int32_t ax = vx - prev.velXPerMs;
        const int32_t ay = vy - prev.velYPerMs;
        const int32_t v2 = vx * vx + vy * vy;

        // ⚠ 偏离原厂：删掉 `age == 1 → v²>>2` 的放宽。新生轨迹没有速度先验，
        // 把它的允许位移放大到 2 倍只会让它更容易去抢别的区域 —— 双指交替点击
        // 时新生的那一条恰恰最该收紧。
        if (prev.age == 1) {
            return m_idtAdmissibilityInCost ? v2 : Fixed::ShrTowardZero(v2, 2);
        }
        return std::min(ax * ax + ay * ay, v2);
    }

    /// 标称帧间隔（ms）。m_reportRateHz = 100 ⇒ 10 ms。
    [[nodiscard]] int32_t NominalFrameIntervalMs() const noexcept {
        return (m_reportRateHz > 0) ? (1000 / m_reportRateHz) : 10;
    }

    // ── IDT_IsToIncAccThold @0x6BA615C7 ─────────────────────────────────
    //
    //   if (!PeakAbsGrip(cur.peak23C) && prev.grip210 == 1) return 0;
    //   if (prev.1FC & 0x1000000 || prev.1FC & 0x2000000) return 0;
    //   if (Touch_IsException(c,j) || static.bField_0x111 >= 2) return 1;
    //   return prev.maxSqMove4F6 >= 25;      // 曾有单帧位移 ≥ 5 mm
    //
    // static.bField_0x111 本机恒 < 2（文档 05 §2.5），该项不产生放宽。
    [[nodiscard]] bool IDT_IsToIncAccThold(const ZoneCluster& cur,
                                           const InternalTrack& prev) const noexcept {
        const bool absGrip = (cur.zoneFlags & ZONE_FLAG_ABS_GRIP) != 0;
        if (!absGrip && prev.gripStage210 == 1) return false;
        if ((prev.persist1FC & 0x3000000u) != 0) return false;
        if (cur.exceptionState != ZONE_EXC_NONE || prev.exceptionType != 0) return true;

        // ⚠ 偏离原厂：`prev.maxSqMoveMm2 >= 25` 是「这条轨迹**历史上曾经**有过
        // 一次 ≥ 5 mm 的单帧位移」，一旦成立就永久放宽 4 倍。任何一次正常拖动
        // 都会给轨迹打上这个标记，于是日常使用中绝大多数轨迹都在 4 倍门限下
        // 跑。改成只看**上一帧的瞬时速度**：真正在快速移动的轨迹才放宽。
        if (!m_idtAdmissibilityInCost) return prev.maxSqMoveMm2 >= 25;

        const int32_t dt = NominalFrameIntervalMs();
        const int32_t sx = prev.velXPerMs * dt;   // 上一帧单帧位移（Q8）
        const int32_t sy = prev.velYPerMs * dt;
        const int32_t mmPerFrameSq =
            (sx * sx + sy * sy) / (m_pitchPxPerMm * m_pitchPxPerMm);
        return mmPerFrameSq >= 25;                // 上一帧确实走了 ≥ 5 mm
    }

    // ── IDT_AlwaysMatchProcess @0x6BA620AC（文档 05 §2.3）────────────────
    //
    // 门限 T = (TS_MMToPixel(flash[0x88A]) >> 4)² = (960>>4)² = 3600。
    // 候选 k = {c, p} 只有在「c 对 p 合理、且 c 对其它轨迹都不合理、且 p 对其它
    // 当前触点也都不合理」时才被锚定成 -0x3FFFFFFF —— 唯一性检验，不是就近吸引。
    void IDT_AlwaysMatchProcess(int32_t* mat, bool transpose, const IdtContext& ctx,
                                const uint8_t (*cand)[2], uint8_t candNum) const noexcept {
        const int8_t s1 = transpose ? static_cast<int8_t>(kIdtMatDim) : int8_t{1};
        const int8_t s2 = transpose ? int8_t{1} : static_cast<int8_t>(kIdtMatDim);
        const int64_t base = static_cast<int64_t>(
            (TS_MMToPixel(static_cast<int32_t>(m_maxAccMM)) >> 4) *
            (TS_MMToPixel(static_cast<int32_t>(m_maxAccMM)) >> 4));

        for (uint8_t k = 0; k < candNum; ++k) {
            const uint8_t c = cand[k][0];
            const uint8_t p = cand[k][1];
            bool blocked = false;

            // 这个当前触点对所有轨迹：自身配对必须合理，别的轨迹必须都不合理
            for (size_t j = 0; j < ctx.numPrev; ++j) {
                const InternalTrack& prev = PrevOf(ctx, j);
                const int64_t acc = IDT_GetAcc(ctx.cur[c], prev);
                const int64_t t = IDT_IsToIncAccThold(ctx.cur[c], prev) ? (base << 2) : base;
                if (j == p) {
                    if (acc > t) { blocked = true; break; }
                } else if (acc <= t) {
                    blocked = true;
                }
            }
            // 这条轨迹对其它当前触点也必须都不合理（注意原版此处**不 break**）
            if (!blocked) {
                const InternalTrack& prev = PrevOf(ctx, p);
                for (size_t i = 0; i < ctx.numCur; ++i) {
                    if (i == c) continue;
                    const int64_t acc = IDT_GetAcc(ctx.cur[i], prev);
                    const int64_t t = IDT_IsToIncAccThold(ctx.cur[i], prev) ? (base << 2) : base;
                    if (acc <= t) blocked = true;
                }
            }
            if (!blocked) {
                // ⚠ 原厂缺陷照抄：下标被截成 uint8，p*s1 + c*s2 ≥ 256 时回绕，
                //   写到错误的格子（当前下标 ≥ 13 非转置 / 历史下标 ≥ 13 转置）。
                const uint8_t idx = static_cast<uint8_t>(
                    static_cast<int8_t>(static_cast<int8_t>(p) * s1 +
                                        s2 * static_cast<int8_t>(c)));
                mat[idx] = -0x3FFFFFFF;
            }
        }
    }

    // ── IDT_PopulateDistanceMap @0x6BA623E4（文档 05 §2.2）───────────────
    //
    //   N < M 时 transpose = 1：行 = 当前、列 = 历史；否则行 = 历史、列 = 当前。
    //   元素 [row][col] 永远在 row + col*20，矩阵按列主序存放。
    //
    //   cost = dx² + dy²（dx/dy 取原始质心 +0x15e/+0x160）
    //   pred = x + dt·v + dt·⌊a/2⌋     ← **不是** ½a·dt²；dt 取历史轨迹自己
    //                                    上一帧的 +0x150，v/a 取 +0x4cc/+0x4d0
    //   「近（曼哈顿 ≤ 300）+ 老（年龄 > 5）+ 慢（预测位移曼哈顿 ≤ 300）」三者
    //   同时成立 → 整格改写为 -0x3FFFFFFF；否则收进 AlwaysMatch 候选表。
    //   **没有**「超距即禁连」的无穷大代价，拦截完全交给 IDT_AccCheck。
    void IDT_PopulateDistanceMap(int32_t* mat, bool transpose, const IdtContext& ctx) const noexcept {
        const int8_t stepPrev = transpose ? static_cast<int8_t>(kIdtMatDim) : int8_t{1};
        const int8_t stepCur  = transpose ? int8_t{1} : static_cast<int8_t>(kIdtMatDim);
        const int32_t thr = TS_MMToPixel(static_cast<int32_t>(m_maxMatchDistMM));
        // ageE：dwProcessingState & 0x10 是 SideTouch 处理中，MS 管线恒 5
        const uint16_t ageE = m_alwaysMatchMinAge;
        const bool penalty = ctx.numCur < ctx.numPrev;

        uint8_t cand[kMaxTrackSlots][2]{};
        uint8_t candNum = 0;
        uint8_t rowBase = 0;   // ⚠ uint8 累加器，照抄回绕

        for (size_t i = 0; i < ctx.numCur; ++i) {
            const ZoneCluster& cur = ctx.cur[i];
            uint8_t idx = rowBase;
            for (size_t j = 0; j < ctx.numPrev; ++j) {
                const InternalTrack& prev = PrevOf(ctx, j);

                const int32_t dx = static_cast<int32_t>(cur.cogX_q8) - static_cast<int32_t>(prev.rawX_q8);
                const int32_t dy = static_cast<int32_t>(cur.cogY_q8) - static_cast<int32_t>(prev.rawY_q8);
                int32_t cost = dx * dx + dy * dy;

                const int32_t dt = prev.dt150;   // 历史轨迹上一帧的 dt
                const int32_t px = dt * prev.velXPerMs;
                const int32_t py = dt * prev.velYPerMs;
                // a/2 是 C 的截断除法（向零），不是算术右移
                const int32_t predX = dt * (prev.accRawX / 2) + px + prev.rawX_q8;
                const int32_t predY = dt * (prev.accRawY / 2) + py + prev.rawY_q8;
                const int32_t ex = std::abs(predX - static_cast<int32_t>(cur.cogX_q8));
                const int32_t ey = std::abs(predY - static_cast<int32_t>(cur.cogY_q8));

                if (thr < std::abs(dx) + std::abs(dy) ||
                    prev.age <= ageE ||
                    thr < std::abs(px) + std::abs(py)) {
                    if (ex + ey <= thr &&
                        !IDT_IsEdgeTouch(cur.cogX_q8, cur.cogY_q8) &&
                        candNum < kMaxTrackSlots) {
                        cand[candNum][0] = static_cast<uint8_t>(i);
                        cand[candNum][1] = static_cast<uint8_t>(j);
                        ++candNum;
                    }
                } else {
                    cost = -0x3FFFFFFF;   // 近、老、慢：直接锚定，欧氏项被丢弃
                }

                if (penalty) {
                    // 位移与速度直接相减，量纲不一致，照原样实现
                    const int32_t rx = dx - prev.velXPerMs;
                    const int32_t ry = dy - prev.velYPerMs;
                    cost = Fixed::WrapAdd32(cost, 2 * (rx * rx + ry * ry));
                }

                mat[idx] = cost;
                idx = static_cast<uint8_t>(idx + static_cast<uint8_t>(stepPrev));
            }
            rowBase = static_cast<uint8_t>(rowBase + static_cast<uint8_t>(stepCur));
        }

        IDT_AlwaysMatchProcess(mat, transpose, ctx, cand, candNum);
    }

    // ── IDT_HM @0x6BA61BA2（文档 05 §2.4）───────────────────────────────
    //
    // 带势函数的最短增广路匈牙利（Kuhn–Munkres 的 O(n²m) 实现），**不是 JV**：
    // 没有列归约与增广行归约初始化，逐行增广。
    //
    // ⚠ u / v / slack / red / d 一律 int32 **回绕**运算（§2.4 给了 3×3 反例：
    //   slack = 2 300 576 607 回绕成 −1 994 390 689 后选错列、改变最终配对）。
    //   改 int64 或饱和就是偏离原版。
    static void IDT_HM(uint8_t dim1Len, uint8_t dim2Len,
                       uint8_t* match, const int32_t* mat) noexcept {
        int8_t  way[kIdtMatDim]{};
        uint8_t used[kIdtMatDim]{};
        int32_t slack[kIdtMatDim]{};
        int32_t v[kIdtMatDim]{};        // 列势
        int32_t u[kIdtMatDim]{};        // 行势

        for (uint8_t c = 0; c < kIdtMatDim; ++c) match[c] = 0xFF;

        for (uint8_t r = 0; r < dim1Len; ++r) {
            for (uint8_t c = 0; c < dim2Len; ++c) {
                way[c] = -1;
                slack[c] = 0x7FFFFFFF;
                used[c] = 0;
            }
            uint8_t last = 0xFF;
            uint8_t row = r;

            while (row != 0xFF) {
                uint8_t best = 0xFF;
                uint8_t colBase = 0;   // ⚠ uint8 累加器：col*20 在 col ≥ 13 时回绕
                for (uint8_t c = 0; c < dim2Len; ++c) {
                    if (used[c] == 0) {
                        const int32_t red = Fixed::WrapSub32(
                            Fixed::WrapSub32(mat[static_cast<int>(row) + colBase], u[row]), v[c]);
                        if (red < slack[c]) {
                            slack[c] = red;
                            way[c] = static_cast<int8_t>(last);
                        }
                        if (best == 0xFF || slack[c] < slack[best]) best = c;
                    }
                    colBase = static_cast<uint8_t>(colBase + kIdtMatDim);
                }
                if (best == 0xFF) best = 0;   // 原版断言失败后仍继续，取 0

                const int32_t d = slack[best];
                for (uint8_t c = 0; c < dim2Len; ++c) {
                    if (used[c] == 0) {
                        slack[c] = Fixed::WrapSub32(slack[c], d);
                    } else {
                        u[match[c]] = Fixed::WrapAdd32(u[match[c]], d);
                        v[c] = Fixed::WrapSub32(v[c], d);
                    }
                }
                u[r] = Fixed::WrapAdd32(u[r], d);

                used[best] = 1;
                last = best;
                row = match[best];
            }

            uint8_t c = (last == 0xFF) ? uint8_t{0} : last;
            while (way[c] != -1) {
                const uint8_t p = static_cast<uint8_t>(way[c]);
                match[c] = match[p];
                c = p;
            }
            match[c] = r;
        }
    }

    // ── IDT_HMProcess @0x6BA61323 ───────────────────────────────────────
    // 以 dim1 = min(N,M) 为行数、dim2 = max(N,M) 为列数调用，再把结果统一成
    // 「以历史下标为索引」的 scratch。由于 dim1 <= dim2，较小一侧每个元素必被匹配。
    void IDT_HMProcess(uint8_t* scratch, const IdtContext& ctx) const noexcept {
        int32_t mat[kIdtMatDim * kIdtMatDim]{};
        uint8_t match[kIdtMatDim]{};

        for (uint8_t i = 0; i < kIdtMatDim; ++i) scratch[i] = 0xFF;
        if (ctx.numCur == 0 || ctx.numPrev == 0) return;

        const uint8_t n = static_cast<uint8_t>(ctx.numCur);
        const uint8_t m = static_cast<uint8_t>(ctx.numPrev);

        if (n < m) {
            IDT_PopulateDistanceMap(mat, /*transpose=*/true, ctx);
            IDT_HM(n, m, match, mat);
            for (uint8_t j = 0; j < m; ++j) {
                if (match[j] != 0xFF) scratch[j] = match[j];
            }
        } else {
            IDT_PopulateDistanceMap(mat, /*transpose=*/false, ctx);
            IDT_HM(m, n, match, mat);
            for (uint8_t i = 0; i < n; ++i) {
                if (match[i] != 0xFF && match[i] < kIdtMatDim) scratch[match[i]] = i;
            }
        }
    }

    // ── IDT_AccCheck @0x6BA619A3（文档 05 §2.5）──────────────────────────
    //
    //   1. acc = IDT_GetAcc(c, j)，t = 3600；IDT_IsToIncAccThold 或 mode != 0 → t <<= 2
    //   2. acc > t → 断开，static.dwSpecialControlFlags |= 4
    //   3. 否则 GripFilter_IsToCancelIDMapping → 断开，当前 +0x214 |= 0x200
    //   4. 不论前两步，历史 +0x1f8 & 0x200000 → 强制断开
    //
    // 被断开的一对，当前触点在 TE 中作为新手指领新 ID，历史轨迹走抬起流程；
    // 不存在二次匹配。
    void IDT_AccCheck(uint8_t* scratch, const IdtContext& ctx) noexcept {
        const int64_t base = static_cast<int64_t>(
            (TS_MMToPixel(static_cast<int32_t>(m_maxAccMM)) >> 4) *
            (TS_MMToPixel(static_cast<int32_t>(m_maxAccMM)) >> 4));
        const size_t lim = std::min<size_t>(std::max(ctx.numCur, ctx.numPrev), kIdtMatDim);

        for (size_t j = 0; j < lim; ++j) {
            if (scratch[j] == 0xFF) continue;
            const size_t c = scratch[j];
            if (c >= ctx.numCur || j >= ctx.numPrev) { scratch[j] = 0xFF; continue; }

            InternalTrack& prev = m_tracks[(*ctx.prevSlots)[j]];
            const int64_t acc = IDT_GetAcc(ctx.cur[c], prev);
            int64_t t = base;
            if (IDT_IsToIncAccThold(ctx.cur[c], prev) || ctx.strictMode) t <<= 2;

            if (acc > t) {
                scratch[j] = 0xFF;
                m_frameSpecialFlags |= 4u;   // 日志 "IDT: ACC(%d) too large..."
            }
            // GripFilter_IsToCancelIDMapping 属模块 06，其输入（本帧握持结论）在
            // IDT 时刻尚未产生，复刻侧留给 agent-06 接线，此处恒假。

            // 07 的 TouchReport_IsHistoryBufferApplied 在「移入回放期间手指已抬起」
            // 时置 +0x1f8 |= 0x200000，下一帧这里强制断开，轨迹随即走抬起流程。
            if ((prev.persist1F8 & 0x200000u) != 0) scratch[j] = 0xFF;
        }
    }

    // ══════════════════════════════════════════════════════════════════
    // 关联重排后的求解路径（偏离原厂，见成员声明处的说明）
    //
    //   ┌──────────────┐   合法？   ┌─────────────────┐
    //   │ 区域 i × 轨迹 j│──────────►│ cost = dx²+dy²   │──┐
    //   └──────────────┘    否      │ （近+老+慢再减奖励）│  │
    //          │                    └─────────────────┘  ├─► 匈牙利(int64)
    //          └───────────────────► cost = kForbidden ───┘        │
    //                                                              ▼
    //                                            剔除落在 kForbidden 上的配对
    //
    // 落在哨兵上说明该行本来就没有任何合法去处，所以不需要「二次求解」。
    // ══════════════════════════════════════════════════════════════════

    /// 单个配对是否合法。原 IDT_AccCheck 的判据 + 位移硬门，全部前移到这里。
    [[nodiscard]] bool IDT_IsAdmissible(const ZoneCluster& cur, const InternalTrack& prev,
                                        int64_t d2, bool strictMode) const noexcept {
        // 07 的 TouchReport_IsHistoryBufferApplied 在「移入回放期间手指已抬起」
        // 时置 +0x1f8 |= 0x200000，此后强制断开（原 IDT_AccCheck 第 4 条）。
        if ((prev.persist1F8 & 0x200000u) != 0) return false;

        const int64_t rHard = static_cast<int64_t>(TS_MMToPixel(
            static_cast<int32_t>(m_idtHardMatchDistMM)));
        if (d2 > rHard * rHard) return false;

        const int64_t base = static_cast<int64_t>(
            (TS_MMToPixel(static_cast<int32_t>(m_maxAccMM)) >> 4) *
            (TS_MMToPixel(static_cast<int32_t>(m_maxAccMM)) >> 4));
        int64_t t = base;
        if (IDT_IsToIncAccThold(cur, prev) || strictMode) t <<= 2;
        return static_cast<int64_t>(IDT_GetAcc(cur, prev)) <= t;
    }

    /// 建代价矩阵并求解。scratch[j] = 与历史 j 配对的当前下标，0xFF 表示未配。
    void IDT_SolveAdmissible(uint8_t* scratch, const IdtContext& ctx) const noexcept {
        for (uint8_t i = 0; i < kIdtMatDim; ++i) scratch[i] = 0xFF;
        const size_t nCur = ctx.numCur, nPrev = ctx.numPrev;
        if (nCur == 0 || nPrev == 0) return;

        static_assert(kIdtAnchorBonus < kIdtForbidden,
                      "奖励后的代价必须仍然低于哨兵");

        const int32_t thr = TS_MMToPixel(static_cast<int32_t>(m_maxMatchDistMM));
        const int32_t dtNom = NominalFrameIntervalMs();

        int64_t cost[kIdtMatDim][kIdtMatDim];
        for (size_t i = 0; i < nCur; ++i) {
            const ZoneCluster& cur = ctx.cur[i];
            for (size_t j = 0; j < nPrev; ++j) {
                const InternalTrack& prev = PrevOf(ctx, j);

                const int64_t dx = static_cast<int64_t>(cur.cogX_q8) - prev.rawX_q8;
                const int64_t dy = static_cast<int64_t>(cur.cogY_q8) - prev.rawY_q8;
                const int64_t d2 = dx * dx + dy * dy;

                if (!IDT_IsAdmissible(cur, prev, d2, ctx.strictMode)) {
                    cost[i][j] = kIdtForbidden;
                    continue;
                }

                int64_t c = d2;

                // 「近 + 老 + 慢」的优先奖励。与原厂不同的是这里**保留** d²，
                // 所以多个锚定配对之间仍然可比，不会并列成同一个常数。
                const int64_t px = static_cast<int64_t>(dtNom) * prev.velXPerMs;
                const int64_t py = static_cast<int64_t>(dtNom) * prev.velYPerMs;
                // ⚠ 不要把这些局部量叫 near / far：windef.h 把它们定义成了宏。
                const bool isNear = (std::abs(dx) + std::abs(dy)) <= thr;
                const bool isOld  = prev.age > m_alwaysMatchMinAge;
                const bool isSlow = (std::abs(px) + std::abs(py)) <= thr;
                if (isNear && isOld && isSlow && !IDT_IsEdgeTouch(cur.cogX_q8, cur.cogY_q8)) {
                    c -= kIdtAnchorBonus;
                }
                cost[i][j] = c;
            }
        }

        // 行取较少的一侧，保证每行都能被指派。
        uint8_t assign[kIdtMatDim];
        if (nCur <= nPrev) {
            IDT_Hungarian64(nCur, nPrev, &cost[0][0], kIdtMatDim, /*transposed=*/false, assign);
            // assign[j] = 与列 j（历史）配对的行（当前），0xFF 未配
            for (size_t j = 0; j < nPrev; ++j) {
                const uint8_t i = assign[j];
                if (i != 0xFF && cost[i][j] < kIdtForbidden) scratch[j] = i;
            }
        } else {
            IDT_Hungarian64(nPrev, nCur, &cost[0][0], kIdtMatDim, /*transposed=*/true, assign);
            // assign[i] = 与列 i（当前）配对的行（历史）
            for (size_t i = 0; i < nCur; ++i) {
                const uint8_t j = assign[i];
                if (j != 0xFF && cost[i][j] < kIdtForbidden) scratch[j] = static_cast<uint8_t>(i);
            }
        }
    }

    /// 带势函数的最短增广路匈牙利，int64 平凡运算（不再依赖 int32 回绕）。
    /// `a` 按 [row][col] 取值；transposed 时行列互换。rows <= cols。
    /// 输出 out[col] = 与该列配对的行，0xFF 表示该列未被占用。
    static void IDT_Hungarian64(size_t rows, size_t cols, const int64_t* a, size_t stride,
                                bool transposed, uint8_t* out) noexcept {
        constexpr int64_t kInf = INT64_MAX / 4;
        const size_t n = rows, m = cols;

        auto at = [&](size_t r, size_t c) -> int64_t {
            return transposed ? a[c * stride + r] : a[r * stride + c];
        };

        // 1-based，索引 0 是哑列
        int64_t u[kIdtMatDim + 1]{}, v[kIdtMatDim + 1]{}, minv[kIdtMatDim + 1];
        size_t  p[kIdtMatDim + 1]{}, way[kIdtMatDim + 1]{};
        bool    used[kIdtMatDim + 1];

        for (size_t i = 1; i <= n; ++i) {
            p[0] = i;
            size_t j0 = 0;
            for (size_t j = 0; j <= m; ++j) { minv[j] = kInf; used[j] = false; }
            do {
                used[j0] = true;
                const size_t i0 = p[j0];
                int64_t delta = kInf;
                size_t j1 = 0;
                for (size_t j = 1; j <= m; ++j) {
                    if (used[j]) continue;
                    const int64_t c = at(i0 - 1, j - 1) - u[i0] - v[j];
                    if (c < minv[j]) { minv[j] = c; way[j] = j0; }
                    if (minv[j] < delta) { delta = minv[j]; j1 = j; }
                }
                if (j1 == 0) break;             // 理论上不会发生（n <= m）
                for (size_t j = 0; j <= m; ++j) {
                    if (used[j]) { u[p[j]] += delta; v[j] -= delta; }
                    else         { minv[j] -= delta; }
                }
                j0 = j1;
            } while (p[j0] != 0);

            while (j0 != 0) {
                const size_t j1 = way[j0];
                p[j0] = p[j1];
                j0 = j1;
            }
        }

        for (size_t j = 0; j < kIdtMatDim; ++j) out[j] = 0xFF;
        for (size_t j = 1; j <= m && j <= kIdtMatDim; ++j) {
            if (p[j] != 0) out[j - 1] = static_cast<uint8_t>(p[j] - 1);
        }
    }

    // ── IDT_Process(mode) @0x6BA611A6 ───────────────────────────────────
    /// 产出 matchCurToTrack / matchTrackToCur 两张表（复刻侧的 IDT_Map 等价物：
    /// 原版是把历史 ID 抄到当前触点 +0x154，未匹配的当前触点 ID 保持 0）。
    void IDT_Process(std::span<const ZoneCluster> curZones,
                     const std::array<int, kMaxTrackSlots>& prevSlots,
                     size_t numPrev,
                     std::array<int, kMaxTrackSlots>& matchCurToTrack,
                     std::array<int, kMaxTrackSlots>& matchTrackToCur,
                     bool strictMode) noexcept {
        uint8_t scratch[kIdtMatDim]{};
        IdtContext ctx{curZones, &prevSlots, curZones.size(), numPrev, strictMode};

        if (m_idtAdmissibilityInCost) {
            IDT_SolveAdmissible(scratch, ctx);
        } else {
            IDT_HMProcess(scratch, ctx);
            IDT_AccCheck(scratch, ctx);
        }

        for (size_t j = 0; j < numPrev && j < kIdtMatDim; ++j) {
            if (scratch[j] == 0xFF) continue;
            const int slot = prevSlots[j];
            matchCurToTrack[scratch[j]] = slot;
            matchTrackToCur[slot] = static_cast<int>(scratch[j]);
        }
    }


    // ── 1.5 帧间隔归一化 (Touch_GetFormattedTimeInterval @ 0x6BA76329) ──
    // 原始差值单位是毫秒：0 取上报率倒数，|Δ| ≤ 7 钳到 8，|Δ| > 1000 钳到 1000。
    void UpdateFrameInterval(uint64_t timestampUs) noexcept {
        int32_t deltaMs = 0;
        if (m_prevFrameTsUs != 0 && timestampUs > m_prevFrameTsUs) {
            deltaMs = static_cast<int32_t>((timestampUs - m_prevFrameTsUs) / 1000ull);
        }
        m_prevFrameTsUs = timestampUs;

        int32_t interval;
        if (deltaMs == 0) {
            interval = (m_reportRateHz > 0) ? (1000 / m_reportRateHz) : 8;
        } else if (deltaMs > 7) {
            interval = std::min(deltaMs, 1000);
        } else {
            interval = 8;
        }
        m_frameIntervalMs = (interval != 0) ? interval : 1;
    }

    // ── 2.5 Z 幅度准入门 (Touch_SetVolatileFlag @0x6BA70FDD 的 Z 系四判据) ──
    //
    // 原版调用链：TE_Process → TE_TouchDownDebounce / TE_MoveProcess
    //            → Touch_UnitUpdate → Touch_SetVolatileFlag
    //            → Touch_SetVolatileFlagTouchDownZFailed  @0x6BA701E0
    //              Touch_SetVolatileFlagLiftOffZFailed    @0x6BA703C8
    //              Touch_SetVolatileFlagStableTouch       @0x6BA70825
    //
    // 四条判据统一拿**源峰的邻域能量** peak + 0x20（PeakCandidate::nZ4）与
    // Stage 2.5 产出的三档 Z 门限比较，稳态取值 1800/1800/1800，SD 命中时
    // downZThold 被 Peak 级就地改写为 900。
    //
    // 三个函数的第三个参数 param_3 由 Touch_NeedNextEdgeLowThresh @0x6BA6FE4F
    // 给出，它只在 P116 / P150 / B206 / B20Y 四个项目名下、或
    // DAT_6bcab69d / DAT_6bcab69e 非零时为真。本机项目名是 W273AS2700，而这两个
    // 全局量紧邻 DAT_6bcab69c（TSADynPrmt_GetPkThold 只以**字节**写 +0x1c），
    // 没有任何写入者 ⇒ 恒 0。故 needLowThold 在本机恒假，3/4 折扣分支不可达，
    // 这里保留形参以对位原版结构。
    struct ZGateInput {
        int16_t peakSig = 0;      // peak + 0x18
        int32_t peakZ4 = 0;       // peak + 0x20
        bool    valid = false;    // 区域没有关联到峰时整组判据不执行
    };

    [[nodiscard]] static ZGateInput ResolveZGateInput(const TouchBigFrame& bigFrame,
                                                      const ZoneCluster& zone) noexcept {
        ZGateInput in{};
        if (zone.peakIndex >= bigFrame.peaks.size()) {
            return in;
        }
        const PeakCandidate& pk = bigFrame.peaks[zone.peakIndex];
        in.peakSig = pk.z;
        in.peakZ4 = pk.nZ4;
        in.valid = true;
        return in;
    }

    // 原版的 `x * 3 >> 2` 带向零修正，这里逐行照搬。
    [[nodiscard]] static int32_t ThreeQuarters(int32_t v) noexcept {
        int32_t t = v * 3;
        if (t < 0) t += 3;
        return t >> 2;
    }

    /// 按下 Z 不足 + 次级 Z 不足（Touch_SetVolatileFlagTouchDownZFailed）
    static void ApplyTouchDownZGate(InternalTrack& trk, const TouchTholdSet& tholds,
                                    const ZGateInput& in, bool needLowThold) noexcept {
        int32_t downLimit = tholds.downZThold;          // DAT_6bcab690
        int32_t secondLimit = tholds.secondaryZThold;   // DAT_6bcab698
        if (needLowThold) {
            downLimit = ThreeQuarters(downLimit);
            secondLimit = ThreeQuarters(secondLimit);
        }
        if (in.peakZ4 < downLimit) {
            trk.volatileFlags |= TOUCH_FLAG_DOWN_Z_FAILED;
        }
        if (in.peakZ4 < secondLimit) {
            trk.volatileFlags |= TOUCH_FLAG_SECONDARY_Z_FAILED;
        }
    }

    /// 稳定触控（Touch_SetVolatileFlagStableTouch）
    /// 原版在噪声 / SmartCover / 基线复位进行中时改取 DAT_6bcab690 当基准，
    /// 本机 DAT_6bcab700 恒 0、SmartCover 关闭，只有基线复位那一条可达；
    /// 复刻侧没有把基线复位标志接到 Stage 5，按常态分支取 stableZThold。
    static void ApplyStableTouchGate(InternalTrack& trk, const TouchTholdSet& tholds,
                                     const ZGateInput& in) noexcept {
        if (static_cast<int32_t>(tholds.stableZThold) * 3 < in.peakZ4) {
            trk.volatileFlags |= TOUCH_FLAG_STABLE_TOUCH;
        }
    }

    /// 抬手 Z 不足（Touch_SetVolatileFlagLiftOffZFailed 的第一条分支）
    /// 分支选择由 DAT_6bc4a928（TouchMode）与 DAT_6bcab700 & 2 决定，后者恒 0
    /// ⇒ 必走第一条分支，param_3 在该分支里未被使用。
    /// 前置门 `prevAge < 0x3d || !(prevTouch + 0x1f8 & 4)`：后半个条件的
    /// +0x1f8 bit2 由 Touch_SetPersistFlag 在"累计位移超过 DAT_6bcab708²"时置位，
    /// 复刻未建模该持久标志 ⇒ 该位恒 0 ⇒ 前置门恒成立，与原版默认路径一致。
    static void ApplyLiftOffZGate(InternalTrack& trk, const TouchTholdSet& tholds,
                                  const ZGateInput& in) noexcept {
        int32_t sigLimit = ThreeQuarters(tholds.sigThold);   // 3/4 × DAT_6bcab68c
        int32_t zLimit   = ThreeQuarters(tholds.downZThold); // 3/4 × DAT_6bcab690

        // 再各自与"上一帧对应值的一半"取小：门限随着触点自身的历史强度收紧，
        // 弱触点不会因为绝对门限过高而被误判成抬手。
        sigLimit = std::min<int32_t>(sigLimit, trk.prevPeakSig / 2);
        zLimit   = std::min<int32_t>(zLimit, trk.prevPeakZ4 / 2);

        if (in.peakZ4 < zLimit || static_cast<int32_t>(in.peakSig) < sigLimit) {
            trk.volatileFlags2 |= TOUCH_FLAG2_LIFTOFF_Z_FAILED;
        }
    }

    // ── B2：Touch_SetVolatileFlagSigSum @0x6BA6F7AF（文档 05，07 §8/§15）──
    //
    // Touch_SetVolatileFlag @0x6BA71040 的**第一个**被调者，产出模块 07 冻结
    // 分支 1 与 Filter_IsHandLift @0x6BA5C644 的三个跨帧能量比。统一形式
    // `min(255, (本帧量 << 5) / 上帧量)`，分母为 0 时直接写 255。1.0 == 32。
    //
    //   +0x1DC sigSumRatio = touch[+0x1CC]（区域零阶矩 M0）
    //                      ÷ prevTouch[+0x1CC]
    //          ★ 分母是**上一帧触点的 M0**，不是 prevPeaks —— 这条与任务书
    //            描述不同，以反编译为准。
    //   +0x1DD znRatio     = g_pPeaks[touch+0x23C].zn (+0x20，Zn)
    //                      ÷ g_prevPeaksPtr[prevTouch+0x23C].zn (条目 +0x04)
    //   +0x1DE z1Ratio     = g_pPeaks[touch+0x23C].sig(+0x18，Z1)
    //                      ÷ g_prevPeaksPtr[prevTouch+0x23C].sig(条目 +0x08)
    //
    // 后两个共用同一道前置门 `prevTouch[+0x23C] < g_prevPeaksPtr[0]`（历史峰
    // 表条目数），不满足时两者一并置 255。注意判据只看**下标**是否在历史表
    // 长度内，不校验它是否还指向同一个物理峰 —— 原厂如此，照抄。
    //
    // 副作用（同函数尾部 + 调用方 Touch_SetVolatileFlag）：
    //   +0x1DC < 0x18 → +0x1F0 |= 2；+0x1DC > 0x28 → +0x1F0 |= 1；
    //   +0x1DE < 0x18 → +0x1F0 |= 0x80000（调用方那一句）。
    [[nodiscard]] static uint8_t SigRatio32(int32_t cur, int32_t prev) noexcept {
        if (prev == 0) return 0xFFu;
        const int32_t v = (cur << 5) / prev;
        return static_cast<uint8_t>(v < 0x100 ? v : 0xFF);
    }

    void Touch_SetVolatileFlagSigSum(InternalTrack& trk, const TouchBigFrame& bigFrame,
                                     int32_t prevM0, uint8_t prevPeakIdx) const noexcept {
        trk.sigSumRatio = SigRatio32(trk.m0, prevM0);

        const PrevPeakTable& prev = bigFrame.prevPeaks;
        if (prevPeakIdx < prev.count && prevPeakIdx < kPeakMaxNum &&
            trk.peakIndex < bigFrame.peaks.size()) {
            const PeakCandidate& pk = bigFrame.peaks[trk.peakIndex];
            trk.znRatio = SigRatio32(pk.nZ4, prev.e[prevPeakIdx].zn);
            trk.z1Ratio = SigRatio32(pk.z, prev.e[prevPeakIdx].sig);
        } else {
            trk.znRatio = 0xFFu;
            trk.z1Ratio = 0xFFu;
        }

        if (trk.sigSumRatio < 0x18u) trk.volatileFlags |= TOUCH_FLAG_SIGSUM_DROP;
        if (trk.sigSumRatio > 0x28u) trk.volatileFlags |= TOUCH_FLAG_SIGSUM_RISE;
        // Touch_SetVolatileFlag @0x6BA71040 紧接 SigSum 之后的那一句。
        if (trk.z1Ratio < 0x18u) trk.volatileFlags |= TOUCH_FLAG_PEAK_Z1_DROP;
    }

    /// 一帧一个触点的完整 Z 门评估。对位 Touch_SetVolatileFlag 里连续的
    /// FingerZFailed / TouchDownZFailed / LiftOffZFailed / StableTouch 四次调用。
    void ApplyZAdmissionGate(InternalTrack& trk, const TouchBigFrame& bigFrame,
                             const ZoneCluster& zone,
                             int32_t prevM0, uint8_t prevPeakIdx) noexcept {
        // Touch_Clean 每帧重建 g_touchesPtr，两个易失标志字随之归零。
        trk.volatileFlags = 0;
        trk.volatileFlags2 = 0;

        // touch + 0x23C：TZ_RegisterTouch 从 TZ 单元 +0x2B6 拷来的本帧峰下标。
        trk.peakIndex = zone.peakIndex;
        // B2：Touch_SetVolatileFlag 的第一个被调者，必须在其余 Z 门之前跑。
        Touch_SetVolatileFlagSigSum(trk, bigFrame, prevM0, prevPeakIdx);

        const ZGateInput in = ResolveZGateInput(bigFrame, zone);
        if (!in.valid) {
            trk.peakSig = 0;
            trk.peakZ4 = 0;
            return;
        }

        // 本机 Touch_NeedNextEdgeLowThresh 恒假，见上方说明。
        constexpr bool kNeedLowThold = false;

        ApplyTouchDownZGate(trk, bigFrame.tholds, in, kNeedLowThold);
        ApplyLiftOffZGate(trk, bigFrame.tholds, in);
        ApplyStableTouchGate(trk, bigFrame.tholds, in);

        trk.peakSig = in.peakSig;
        trk.peakZ4 = in.peakZ4;
    }

    // ── 2.6 区域标志 → 触点标志转换 (TZ_UpdateTouchFlag @0x6BA7FE99) ──
    //
    // 原版位置：TZ_RegisterTouch @0x6BA8039B 的尾段，也就是"把一个 TZ 区域注册成
    // 一个触点"这件事的最后几步（TZ_CentroidEdgeFlag → **TZ_UpdateTouchFlag** →
    // TZ_UpdateInsideSensor）。复刻里区域到轨迹的绑定发生在 Stage 5 的匹配之后，
    // 所以落点选在每条轨迹拿到本帧源区域的那一刻，紧跟 Z 幅度准入门之后 ——
    // 原版是 Touch_Clean 清零 → TZ_RegisterTouch 置区域位 → Touch_UnitUpdate 置 Z 位，
    // 复刻的 ApplyZAdmissionGate 承担了清零，故这里改成"清零 + Z 位"之后再 OR 区域位，
    // 两个来源写的是互不相交的位，顺序对结果没有影响。
    //
    // 十三条转换里有两条是**反相**的（区域没贴边 / 没靠近边框才置位），其余为正相。
    // 全局状态字 g_touchesPtr[0x24] 与 dwSpecialControlFlags 在整帧内累加，
    // 由 Process 尾部统一写回大帧。
    void ApplyZoneTouchFlags(InternalTrack& trk, const ZoneCluster& zone) noexcept {
        // Touch_Clean @0x6BA6F49C 每帧把整个 g_touchesPtr 清零，这三个标志字
        // 因此都是"本帧重建"的量（跨帧值靠 g_pPrevTouches 另存）。
        trk.persistFlags = 0;
        trk.gripChannelFlags = 0;
        trk.postureFlags = 0;

        const uint32_t zf = zone.zoneFlags;

        // 区域没有贴到面板物理边框
        if ((zf & ZONE_FLAG_EDGE_TOUCHED) == 0) {
            trk.volatileFlags |= TOUCH_FLAG_NOT_EDGE_TOUCHED;
        }
        // 区域离边框还有 2 格以上
        if ((zf & ZONE_FLAG_NEAR_EDGE) == 0) {
            trk.volatileFlags2 |= TOUCH_FLAG2_NOT_NEAR_EDGE;
        }
        // 握持强制合并
        if ((zf & ZONE_FLAG_GRIP_MERGE_FORCED) != 0) {
            trk.volatileFlags |= TOUCH_FLAG_GRIP_MERGE;
        }
        // 大形态区域 —— 上报网闸判据 2 的全局位就出在这里
        if ((zf & ZONE_FLAG_BIG_SHAPE) != 0) {
            trk.volatileFlags |= TOUCH_FLAG_BIG_SHAPE_ZONE;
            m_frameGlobalFlags |= TOUCH_GLOBAL_BIG_SHAPE;
        }
        // 四邻被包围（本机自容通道关闭 ⇒ 区域位恒 0，见 TouchZoneAndCentroid）
        if ((zf & ZONE_FLAG_SURROUNDED) != 0) {
            trk.volatileFlags |= TOUCH_FLAG_SURROUNDED;
            m_frameGlobalFlags |= TOUCH_GLOBAL_SURROUNDED;
            m_frameSpecialFlags |= SPECIAL_CTRL_BIG_OBJECT;
        }
        // 峰能量被本地噪声淹没
        if ((zf & ZONE_FLAG_LOW_NOISE_CMF) != 0) {
            trk.volatileFlags |= TOUCH_FLAG_LOW_NOISE_CMF;
            m_frameSpecialFlags |= SPECIAL_CTRL_CMF_NOISE;
        }
        // 相邻线噪声压过峰能量 —— 上报网闸判据 3 的握持通道位
        if ((zf & ZONE_FLAG_NEIGHBOR_NOISE) != 0) {
            trk.volatileFlags |= TOUCH_FLAG_GRIP_CHANNEL;
            m_frameSpecialFlags |= SPECIAL_CTRL_CMF_NOISE;
        }
        // 区域类型 5（大物体）—— 上报网闸判据 10
        if (zone.zoneType == ZoneTypeLargeObject) {
            trk.volatileFlags |= TOUCH_FLAG_ZONE_TYPE5;
            m_frameSpecialFlags |= SPECIAL_CTRL_BIG_OBJECT;
        }
        // 侧边触控通道（SideTouch_Process 未复刻 ⇒ 区域位恒 0）
        if ((zf & ZONE_FLAG_SIDE_TOUCH) != 0) {
            trk.volatileFlags2 |= TOUCH_FLAG2_SIDE_TOUCH;
        }
        // 区域被握持分离 ⇒ 持久位，下一帧回灌给 TZ_CheckToSeparateZone
        if ((zf & ZONE_FLAG_ZONE_SEPARATED) != 0) {
            trk.persistFlags |= 1u;
        }
        // 整区判为握持
        if ((zf & ZONE_FLAG_GRIP_ZONE) != 0) {
            trk.gripChannelFlags |= 0x400u;
        }
        // 存在非握持内部峰、且区域没有被分离
        if ((zf & ZONE_FLAG_INNER_NON_GRIP_PK) != 0 && (zf & ZONE_FLAG_ZONE_SEPARATED) == 0) {
            trk.gripChannelFlags |= 0x80000000u;
        }
        // 多胖指
        if ((zf & ZONE_FLAG_MULTI_FAT_FINGER) != 0) {
            trk.postureFlags |= 2u;
        }

        // TZ_RegisterTouch 把 zone + 0x350 原样拷进 touch + 0x1ec —— 上报网闸
        // 判据 4 读的异常类型枚举。
        trk.exceptionType = zone.exceptionState;
        // 同一处把 zone + 0x2e 拷进 touch + 0x29e，供 Touch_UnitUpdateMaxMin 消费。
        trk.centSig = zone.centSig;
    }

    // ── 2.7 跨帧信号锁存 (Touch_UnitUpdateMaxMin，Touch_UnitUpdate @0x6BA760DA 调用) ──
    //
    // 原版在这个函数里维护十来对 max/min 跨帧量，复刻只需要下游真正读的那一项：
    //   touch + 0x1ba ← prev + 0x1ba < cur + 0x29e
    //                     ? min(cur + 0x29e, wPeakFilterReferenceThreshold)
    //                     : prev + 0x1ba
    // 递推结果是"钳在 peakFilterRef 以内的历史最大重心信号"，唯一消费者是
    // TouchReport_ToBeReported 的手套弱信号判据 `+0x1ba < peakFilterRef / 2`。
    static void ApplyUnitUpdateMaxMin(InternalTrack& trk, int16_t peakFilterRef) noexcept {
        if (trk.minSigLatched < trk.centSig) {
            trk.minSigLatched = std::min<int16_t>(trk.centSig, peakFilterRef);
        }
    }

    /// DB_TouchDownNum @0x6BA5681C 的 Z 相关部分：按下 Z 不足或次级 Z 不足的
    /// 触点，防抖帧数被钳成 0xffff —— 于是 TE_TouchDownDebounce 的
    /// `debounceCounter < num` 永远成立，触点**永远停在 0x01 防抖态、
    /// 无法转成 0x02 已确认触点**。这就是 Z 幅度准入门的主消费点。
    // ── DB_TouchDownNum @0x6BA5681C（文档 05 §5.4 的 13 步）───────────────
    //
    // 返回按下去抖帧数下限；0xFFFF 表示永久阻断（触点被钉在 EventIdle，
    // 永远不转成已确认触点）。步骤编号对位文档。
    //
    // 未接线的输入（复刻侧尚无来源，逐条注明）：
    //   · cur.+0x2A4（TZ 附加位）与 +0x1D8 —— 模块 04 未产出；
    //   · static.bField_0x111 —— 由 Exception_Process 维护，本机恒 < 2（§2.5）；
    //   · Touch_ValidComparison —— 依赖 07 的上报历史，暂按「有效」处理；
    //   · flash[0x818] 噪声抬高（第 13 步）—— 本机为 0，整段关闭。
    [[nodiscard]] uint16_t DB_TouchDownNum(InternalTrack& trk) const noexcept {
        constexpr uint16_t kBlocked = 0xFFFFu;

        // 1. 峰数比上一帧多出 1 个以上（有符号差）
        uint16_t n = (static_cast<int>(m_peakCount) - static_cast<int>(m_prevPeakCount) > 1)
                         ? uint16_t{1} : uint16_t{0};

        // 2. 触点级异常字 +0x1ec
        if ((trk.exceptionType & 0x80u) != 0) n = std::max<uint16_t>(n, 2);
        if ((trk.exceptionType & 0x08u) != 0 && n == 0) n = 1;
        if ((trk.exceptionType & 0x200u) != 0) n = std::max<uint16_t>(n, 3);

        // 4. 以下任一成立即永久阻断。0x400 / 0x40000000 就是 Z 幅度准入门写的
        //    「按下 Z 不足 / 次级 Z 不足」，即 DB_TouchDownTholdCheck 失败。
        constexpr uint32_t kBlockMask = TOUCH_FLAG_DOWN_Z_FAILED |          // 0x400
                                        0x80000000u | 0x40000000u |          // 越界 / 次级 Z
                                        0x01000000u | 0x02000000u;           // 合并 / 分离
        if ((trk.exceptionType & 0x40u) != 0 || (trk.volatileFlags & kBlockMask) != 0) {
            n = kBlocked;
        }

        // 5. DB_GetNumBySignalSum：峰信号和低于 (dyn[+6]·dyn[+0x14])>>4 = 1320 时，
        //    新轨迹至少 1 帧；信号相比上一帧暴涨四分之一以上则抬到 3。
        if (n != kBlocked && trk.m0 < kDbSignalSumThold) {
            if (trk.age == 0) {
                if (n == 0) n = 1;
            } else {
                const int32_t quarter = Fixed::ShrTowardZero(trk.prevM0, 2);
                if (trk.m0 - trk.prevM0 > quarter && n < 3) n = 3;
            }
        }

        // 6. DB_GetNumBySideTouch（MS 分支）
        if (n != kBlocked) {
            if (trk.axisMajorMm > 14 && n < 2) n = 2;
            if ((trk.volatileFlags & TOUCH_FLAG_NOT_EDGE_TOUCHED) == 0 && n == 0) n = 1;
        }

        // 7. 区域类型与「手指 Z 不足」
        if (n != kBlocked) {
            if (trk.zoneType == ZoneTypeShapeFatFinger || trk.zoneType == ZoneTypeMultiFinger) {
                n = std::max<uint16_t>(n, 3);
            }
            if ((trk.volatileFlags & TOUCH_FLAG_FINGER_Z_FAILED) != 0) n = std::max<uint16_t>(n, 2);
        }

        // 8. 大形态区域 / 越界，直接阻断
        if ((trk.persist1F8 & 0x2000u) != 0 || (trk.volatileFlags & 0x400000u) != 0) {
            n = kBlocked;
        }

        // 9. 阻断时打易失位 0x10；上一帧被阻断过则历史计数清零
        if (n == kBlocked) {
            trk.volatileFlags |= 0x10u;
        } else if ((trk.prevVolatileFlags & 0x10u) != 0) {
            trk.debounceCounter = 0;
        }

        // 10. DB_FastClickProcess：为 EventRescue（快速点击补报）准备 +0x1f8 的 0x40000
        DB_FastClickProcess(trk);

        // 11. 与 ER 抬高过的 +0x238 取大
        n = std::max<uint16_t>(n, trk.debounceLimit);
        // 复刻侧保留的可配置下限（原版没有这一项，缺省 0 时是恒等变换）
        n = std::max<uint16_t>(n, static_cast<uint16_t>(std::max(m_touchDownDebounceFrames, 0)));

        // 12. 帧间隔 > 14 ms 时对半（int 运算，0xFFFF 变 0x8000 仍等价于阻断）
        if (m_frameIntervalMs > 14) n = static_cast<uint16_t>((n + 1) / 2);

        return n;
    }

    /// DB_FastClickProcess（文档 05 §5.4 第 10 步）：历史计数 < 4 且距按下点
    /// 距离平方 < 31 时，若两帧最小椭圆率都 < 11 且没有同时带上 +0x1f0 & 0x800，
    /// 则按贴边方向与长轴 < 16 的条件置 +0x1f8 |= 0x40000；历史计数 > 5 时清位。
    /// 这一位是 TE_WindowsLiftOffProcess 走 EventRescue 分支的唯一开关。
    static void DB_FastClickProcess(InternalTrack& trk) noexcept {
        if (trk.debounceCounter > 5) {
            trk.persist1F8 &= ~0x40000u;
            return;
        }
        if (trk.debounceCounter >= 4 || trk.sqFromDownMm2 >= 31) return;
        if (!(trk.minEllipseRate < 11 && trk.prevMinEllipseRate < 11)) return;
        if ((trk.volatileFlags & TOUCH_FLAG_FINGER_Z_FAILED) != 0) return;

        const bool byCur  = (trk.volatileFlags & 0x2u) != 0 && trk.axisMajorMm < 16;
        const bool byPrev = (trk.volatileFlags & 0x1u) != 0 && trk.prevAxisMajorMm < 16;
        if (byCur || byPrev) trk.persist1F8 |= 0x40000u;
    }

    // ── 3. 触控事件生命周期状态机推进与尺寸计算 (TE_Process & TS_Process)
    void UpdateLifecycleAndSizes(const TouchBigFrame& bigFrame,
                                 std::span<const ZoneCluster> curZones,
                                 const std::array<int, kMaxTrackSlots>& activeTrackIndices,
                                 size_t activeCount,
                                 const std::array<int, kMaxTrackSlots>& matchCurToTrack,
                                 const std::array<int, kMaxTrackSlots>& matchTrackToCur) {
        // 1. 处理匹配成功的触点: 连续移动 (Move)
        for (size_t r = 0; r < curZones.size(); ++r) {
            const int trkIdx = matchCurToTrack[r];
            if (trkIdx < 0) continue;

            const auto& cur = curZones[r];
            auto& trk = m_tracks[trkIdx];

            // ── PrevTouch_UnitUpdate @0x6BA6D5ED 的运动学段（文档 05 §5.7）──
            //   d  = (short)(cur.X15E − prev.X15E);  dt = cur.dt150 ? : 1
            //   v  = d / dt                                   // short 截断除法
            //   prev.4C8 = d; prev.4D0 = v − prev.4CC; prev.4CC = v
            //   d2 = (short)(cur.X162 − prev.X162)
            //   prev.4D8 = d2 − prev.4D4; prev.4D4 = d2; prev.4DC += d2
            //   prev.4F6 = max(prev.4F6, cur.240)
            // 16 位写回一律**回绕**，走 Fixed::Wrap16。
            trk.prevX_q8 = trk.x_q8;
            trk.prevY_q8 = trk.y_q8;
            trk.prevVx_q8 = trk.vx_q8;
            trk.prevVy_q8 = trk.vy_q8;

            trk.dt150 = static_cast<int16_t>(m_frameIntervalMs);
            const int16_t dtDiv = (trk.dt150 != 0) ? trk.dt150 : int16_t{1};

            // 原始质心（EC 前）—— IDT 的全部距离与速度都基于它
            const int16_t dRawX = Fixed::Wrap16(static_cast<int32_t>(cur.cogX_q8) -
                                                static_cast<int32_t>(trk.rawX_q8));
            const int16_t dRawY = Fixed::Wrap16(static_cast<int32_t>(cur.cogY_q8) -
                                                static_cast<int32_t>(trk.rawY_q8));
            const int16_t vRawX = Fixed::Wrap16(dRawX / dtDiv);
            const int16_t vRawY = Fixed::Wrap16(dRawY / dtDiv);
            trk.accRawX = Fixed::Wrap16(vRawX - trk.velXPerMs);
            trk.accRawY = Fixed::Wrap16(vRawY - trk.velYPerMs);
            trk.velXPerMs = vRawX;
            trk.velYPerMs = vRawY;
            trk.rawX_q8 = cur.cogX_q8;
            trk.rawY_q8 = cur.cogY_q8;

            // 工作坐标（EC 后）—— ER_MoveOutCheck 的二阶外推用它
            const int16_t d2x = Fixed::Wrap16(static_cast<int32_t>(cur.ecX_q8) -
                                              static_cast<int32_t>(trk.x_q8));
            const int16_t d2y = Fixed::Wrap16(static_cast<int32_t>(cur.ecY_q8) -
                                              static_cast<int32_t>(trk.y_q8));
            trk.ddWrkX = Fixed::Wrap16(d2x - trk.dWrkX);
            trk.ddWrkY = Fixed::Wrap16(d2y - trk.dWrkY);
            trk.dWrkX = d2x;
            trk.dWrkY = d2y;
            trk.sumWrkX += d2x;
            trk.sumWrkY += d2y;

            trk.x_q8 = cur.ecX_q8;
            trk.y_q8 = cur.ecY_q8;
            trk.vx_q8 = d2x;
            trk.vy_q8 = d2y;
            trk.ax_q8 = trk.ddWrkX;
            trk.ay_q8 = trk.ddWrkY;

            // +0x240 本帧移动距离平方（mm²，工作坐标）与 +0x4f6 的历史最大值。
            // IDT_IsToIncAccThold 的「曾有单帧位移 ≥ 5 mm」判据读 +0x4f6。
            {
                const int32_t mx = d2x / m_pitchPxPerMm;
                const int32_t my = d2y / m_pitchPxPerMm;
                const int32_t sq = mx * mx + my * my;
                trk.sqMoveMm2 = static_cast<uint16_t>(std::min<int32_t>(sq, 0xFFFF));
                if (trk.maxSqMoveMm2 < trk.sqMoveMm2) trk.maxSqMoveMm2 = trk.sqMoveMm2;

                // +0x24c：距按下点距离平方（mm²），DB_FastClickProcess 读它
                const int32_t fx = (static_cast<int32_t>(cur.ecX_q8) -
                                    static_cast<int32_t>(trk.downX_q8)) / m_pitchPxPerMm;
                const int32_t fy = (static_cast<int32_t>(cur.ecY_q8) -
                                    static_cast<int32_t>(trk.downY_q8)) / m_pitchPxPerMm;
                trk.sqFromDownMm2 = static_cast<uint16_t>(
                    std::min<int32_t>(fx * fx + fy * fy, 0xFFFF));
            }

            trk.m0 = cur.m0;
            trk.area = cur.pixelCount;
            trk.isEdge = (cur.minC == 0 || cur.maxC == kCols - 1 || cur.minR == 0 || cur.maxR == kRows - 1);
            trk.isGrip = false;
            trk.missingFrames = 0;
            trk.age += 1;

            // TS_Process @0x6BA7C2CB：原版在 TE 之前对全部当前触点执行
            TS_ProcessUnit(trk, cur, /*hasPrev=*/true);
            trk.axisMajorMm = cur.axisMajorMm;
            trk.axisMinorMm = cur.axisMinorMm;
            trk.extentX = static_cast<uint8_t>(cur.maxC - cur.minC + 1);
            trk.extentY = static_cast<uint8_t>(cur.maxR - cur.minR + 1);
            trk.ellipseRate = cur.ellipseRate;
            trk.fromMultiFinger = cur.fromMultiFinger;
            trk.gripRatio = cur.gripRatio;
            trk.gripEdgeFlags = cur.gripEdgeFlags;
            trk.edgeDistDim1_q8 = cur.edgeDistDim1_q8;
            trk.edgeDistDim2_q8 = cur.edgeDistDim2_q8;
            trk.gripLevel = cur.gripLevel;
            trk.zoneType = cur.zoneType;
            CopyShapeInputs(trk, cur);

            // Z 幅度准入门：先把本帧的峰强度归档成"上一帧值"，再重算四个标志。
            // 对位 Touch_UnitUpdate → Touch_SetVolatileFlag，位置在状态迁移之前。
            trk.prevPeakSig = trk.peakSig;
            trk.prevPeakZ4 = trk.peakZ4;
            ApplyZAdmissionGate(trk, bigFrame, cur, trk.prevM0, trk.prevPeakIndex);

            // 区域标志 → 触点标志（TZ_UpdateTouchFlag），随后跨帧锁存重心信号
            // （Touch_UnitUpdateMaxMin）。两者在原版分别属于 TZ_RegisterTouch
            // 与 Touch_UnitUpdate，落点都在状态迁移之前。
            ApplyZoneTouchFlags(trk, cur);
            ApplyUnitUpdateMaxMin(trk, bigFrame.tholds.peakFilterRef);

            // 状态机推进
            if (trk.state == EventIdle) { // 原版 prev.+0x21c & 1 → TE_TouchDownDebounce
                trk.debounceCounter += 1;
                // Z 不足时 ComputeTouchDownDebounceNum 返回 0xffff，触点被永久
                // 钉在防抖态，无法转成已确认触点 —— 原版 DB_TouchDownNum 的行为。
                if (trk.debounceCounter >= DB_TouchDownNum(trk)) {
                    trk.state = EventDown;   // +0x1fc |= 0x400（按下已确认）
                    trk.debounceCounter = 0;
                    trk.isReported = true;
                    trk.lifeFlags = LifeMapped | LifeNew | LifeDebounced;
                } else {
                    trk.isReported = false;
                }
            } else {
                trk.state = EventMove;
                trk.debounceCounter = 0;
                trk.isReported = true;
                trk.lifeFlags = LifeMapped;
                if (trk.isEdge) trk.lifeFlags |= LifeEdge;
            }

            ApplyLiftOffZReportVeto(trk);
        }

        // 2. 处理未匹配的历史触点: 抬手释放 (LiftOff / Windows PTP)
        for (size_t c = 0; c < activeCount; ++c) {
            const int trkIdx = activeTrackIndices[c];
            if (matchTrackToCur[trkIdx] >= 0) continue; // 已匹配

            auto& trk = m_tracks[trkIdx];
            trk.missingFrames += 1;
            if (m_windowsPadFeatureEnabled) {
                TE_WindowsLiftOffProcess(trk);
            } else {
                TE_LiftOffReportProcess(trk);
            }
        }

        // 3. 处理新候选触点: 注册与分配 ID (TE_TouchDownProcess)
        for (size_t r = 0; r < curZones.size(); ++r) {
            if (matchCurToTrack[r] >= 0) continue; // 已匹配历史触点

            const auto& cur = curZones[r];
            const uint8_t newId = TE_GetNextID();
            if (newId == 0) {
                // 无空闲 ID：原版此时既不写 ID 也不建轨迹，双池数量随即对不上，
                // TE_AssertProcess 失败并把两池整体清空（文档 05 §5.3 / §5.8）。
                m_assertFailed = true;
                continue;
            }

            // 寻找空闲内部槽位
            int freeSlot = -1;
            for (size_t s = 0; s < kMaxTrackSlots; ++s) {
                if (!m_tracks[s].active) {
                    freeSlot = static_cast<int>(s);
                    break;
                }
            }
            if (freeSlot < 0) continue;

            auto& trk = m_tracks[freeSlot];
            trk = InternalTrack{};
            trk.active = true;
            trk.id = newId;
            trk.x_q8 = cur.ecX_q8;
            trk.y_q8 = cur.ecY_q8;
            trk.prevX_q8 = cur.ecX_q8;
            trk.prevY_q8 = cur.ecY_q8;
            trk.downX_q8 = cur.ecX_q8;
            trk.downY_q8 = cur.ecY_q8;
            // TE_TouchDownProcess @0x6BA779F3 以当前单元为模板 PrevTouch_UnitCopy
            // 新建轨迹，原始质心与工作坐标同源；运动学各量由 UnitCopy 带来的 0 起步。
            trk.rawX_q8 = cur.cogX_q8;
            trk.rawY_q8 = cur.cogY_q8;
            trk.dt150 = static_cast<int16_t>(m_frameIntervalMs);
            trk.vx_q8 = 0;
            trk.vy_q8 = 0;
            trk.prevVx_q8 = 0;
            trk.prevVy_q8 = 0;
            trk.ax_q8 = 0;
            trk.ay_q8 = 0;
            trk.m0 = cur.m0;
            trk.area = cur.pixelCount;
            trk.isEdge = (cur.minC == 0 || cur.maxC == kCols - 1 || cur.minR == 0 || cur.maxR == kRows - 1);
            trk.isGrip = false;
            trk.age = 1;
            trk.missingFrames = 0;
            TS_ProcessUnit(trk, cur, /*hasPrev=*/false);
            trk.axisMajorMm = cur.axisMajorMm;
            trk.axisMinorMm = cur.axisMinorMm;
            trk.extentX = static_cast<uint8_t>(cur.maxC - cur.minC + 1);
            trk.extentY = static_cast<uint8_t>(cur.maxR - cur.minR + 1);
            trk.ellipseRate = cur.ellipseRate;
            trk.fromMultiFinger = cur.fromMultiFinger;
            trk.gripRatio = cur.gripRatio;
            trk.gripEdgeFlags = cur.gripEdgeFlags;
            trk.edgeDistDim1_q8 = cur.edgeDistDim1_q8;
            trk.edgeDistDim2_q8 = cur.edgeDistDim2_q8;
            trk.gripLevel = cur.gripLevel;
            trk.zoneType = cur.zoneType;
            CopyShapeInputs(trk, cur);

            // 新触点没有历史峰强度，抬手判据的"历史一半"下界退化为 0，
            // 与原版 PrevTouch_UnitCopy 后 +0x1be/+0x1c0 尚未填写的状态一致。
            trk.prevPeakSig = 0;
            trk.prevPeakZ4 = 0;
            // B2：TE_TouchDownProcess @0x6BA779F3 在 TE_TouchDownDebounce →
            // Touch_UnitUpdate → Touch_SetVolatileFlag 之**前**就做了
            // PrevTouch_UnitCopy（memcpy [0x140,0x284)，含 +0x1CC 与 +0x23C），
            // 所以新触点首帧的分母就是它自己：sigSumRatio 恒为 32，znRatio /
            // z1Ratio 拿本帧峰下标去查**上一帧**历史表（陈旧但确定）。照抄。
            ApplyZAdmissionGate(trk, bigFrame, cur, trk.m0, cur.peakIndex);

            // 新触点的 +0x1ba 从 0 起步（PrevTouch_UnitCopy 尚未填过），
            // 首帧递推等价于 min(centSig, peakFilterRef)。
            trk.minSigLatched = 0;
            ApplyZoneTouchFlags(trk, cur);
            ApplyUnitUpdateMaxMin(trk, bigFrame.tholds.peakFilterRef);

            // TE_TouchDownProcess 尾部立刻调 TE_TouchDownDebounce：Z 不足的
            // 新触点从诞生起就停在 0x01，不会产生按下事件。
            trk.debounceCounter = 0;
            if (trk.debounceCounter >= DB_TouchDownNum(trk)) {
                trk.state = EventDown;
                trk.debounceCounter = 0;
                trk.isReported = true;
                trk.lifeFlags = LifeNew | LifeDebounced;
            } else {
                trk.state = EventIdle;   // 去抖未满，本帧不出报文
                trk.isReported = false;
                trk.lifeFlags = LifeNew;
            }

            ApplyLiftOffZReportVeto(trk);
        }
    }

    // ── TZ_RegisterTouch 拷进触点的 8 个形状原值 + 原始长轴 +0x22a ────────
    // 对位 TZ_RegisterTouch @0x6BA8039B 尾段对 unit +0x2ea..+0x328 的搬运。
    // 说明：原版 +0x22a 是 TZ_UnitToTouch **入口**对 unit +0x2ec 的快照（未经后续
    // FF/MF 修正），复刻的 ZoneCluster 只保留修正后的 axisMajorMm，这里用它近似，
    // 差异仅在 FF/MF 修正真正改写过长轴的区域上（文档 05 §1.2 注）。
    static void CopyShapeInputs(InternalTrack& trk, const ZoneCluster& z) noexcept {
        trk.axisMajorPx    = z.axisMajorQ8;
        trk.axisMinorPx    = z.axisMinorQ8;
        trk.axisAngleDeg   = z.axisAngleDeg;
        trk.minEllipseRate = static_cast<int16_t>(z.ellipseRate);
        trk.gripRatioOuter = z.gripRatioDim1;
        trk.gripRatioInner = z.gripRatioDim2;
        trk.rawAxisMajorMm = z.axisMajorMm;
    }

    // ── Touch_IIRProcess @0x6BA746ED ────────────────────────────────────
    // 7:1 EMA 后再朝 cur 走 ±1，保证每帧至少推进 1 LSB、不卡死区。
    // 右移用 Fixed::ShrTowardZero（负数先 +7 再算术右移），与原版逐位一致。
    [[nodiscard]] static int32_t Touch_IIRProcess(int32_t prev, int32_t cur) noexcept {
        const int32_t v = Fixed::ShrTowardZero(prev * 7 + cur, 3);
        return v + ((v < cur) ? 1 : (v > cur) ? -1 : 0);
    }

    // ── Touch_IIRDataProcess @0x6BA7474F（文档 05 §5.4）──────────────────
    //
    // 把 8 个形状量从「本帧原值」滤成「跨帧平滑值」，写 +0x25e..+0x26c，并置
    // +0x25c = 1 表示快照有效。首帧直通：TE 状态 < 3（按下/去抖阶段）或区域类型
    // 从非 3 变成 3 时跳过滤波直接拷原值，所以按下首帧快照即等于原值。
    // 预置名 P116 才有的 +0x1a8/+0x1aa 钳 100 分支，本机（W273AS2700）不生效。
    static void Touch_IIRDataProcess(InternalTrack& t) noexcept {
        t.shapeSnapshotValid = 1;

        const bool passthrough = (t.state < 3u) ||
                                 (t.zoneType == ZoneTypeMultiFinger &&
                                  t.prevZoneType != ZoneTypeMultiFinger);
        if (passthrough) {
            t.iirAxisMajorMm    = t.axisMajorMm;
            t.iirAxisMinorMm    = t.axisMinorMm;
            t.iirAxisMajorPx    = t.axisMajorPx;
            t.iirAxisMinorPx    = t.axisMinorPx;
            t.iirAxisAngleDeg   = t.axisAngleDeg;
            t.iirMinEllipseRate = t.minEllipseRate;
            t.iirGripRatioOuter = t.gripRatioOuter;
            t.iirGripRatioInner = t.gripRatioInner;
        } else {
            t.iirAxisMajorMm = static_cast<uint16_t>(
                Touch_IIRProcess(t.iirAxisMajorMm, t.axisMajorMm));
            t.iirAxisMinorMm = static_cast<uint16_t>(
                Touch_IIRProcess(t.iirAxisMinorMm, t.axisMinorMm));
            t.iirAxisMajorPx = static_cast<uint16_t>(
                Touch_IIRProcess(t.iirAxisMajorPx, t.axisMajorPx));
            t.iirAxisMinorPx = static_cast<uint16_t>(
                Touch_IIRProcess(t.iirAxisMinorPx, t.axisMinorPx));
            t.iirAxisAngleDeg = static_cast<int8_t>(
                Touch_IIRProcess(t.iirAxisAngleDeg, t.axisAngleDeg));
            t.iirMinEllipseRate = static_cast<int16_t>(
                Touch_IIRProcess(t.iirMinEllipseRate, t.minEllipseRate));
            t.iirGripRatioOuter = static_cast<uint16_t>(
                Touch_IIRProcess(t.iirGripRatioOuter, t.gripRatioOuter));
            t.iirGripRatioInner = static_cast<uint16_t>(
                Touch_IIRProcess(t.iirGripRatioInner, t.gripRatioInner));
        }

        if (t.iirAxisMinorPx == 0) t.iirAxisMinorPx = 1;   // 分母先钳到至少 1
        t.axisRatioX16 = static_cast<uint8_t>(
            (static_cast<uint32_t>(t.iirAxisMajorPx) << 4) /
            static_cast<uint32_t>(t.iirAxisMinorPx));
    }

    // ── Touch_ProcessAftTouchEventUnit @0x6BA7542A（文档 05 §5.4）─────────
    //
    // AFT = After Touch Event。MS 模式下每帧跟在 TE 后面跑，是 +0x228 与
    // +0x25c..+0x26c 这两组字段的**唯一写入者** —— 缺了它，模块 06 的
    // GripFilter AbsGrip / TempGrip 一整族判据恒假（差异 05-09 / 06-09）。
    //
    // 未复刻的部分：Touch_IsAbnormalShape1..4 → 头部计数 hdr[0x10..0x13]，
    // 以及 +0x1fc 的合并/分离/越界四位。它们的消费者都在模块 06，且依赖
    // Touch_GetMergedInfo / PrevPeak_IsOnOuterEdge 等尚未复刻的 03/06 侧函数，
    // 留给对应 agent；本函数只保证 06 的比率判据有真实输入。
    void Touch_ProcessAftTouchEventUnit(InternalTrack& t) const noexcept {
        Touch_IIRDataProcess(t);

        // +0x228 = 长轴历史峰值。MS 模式多一条例外：TE 状态 == 2（本帧按下确认）
        // 时无条件重置为当帧 +0x22a，即每次新按下重新起算。
        if (t.maxAxisMajorMm < t.rawAxisMajorMm || t.state == EventDown) {
            t.maxAxisMajorMm = t.rawAxisMajorMm;
        }
        // 帧尾把本帧的形状/信号量归档成「上一帧值」，供下一帧 DB_TouchDownNum
        // 与 Touch_IIRDataProcess 的首帧直通判据读取。
        t.prevZoneType         = t.zoneType;
        t.prevM0               = t.m0;
        // B2：prevTouch + 0x23C。原版由帧尾 PrevTouch_Process 的整体 memcpy 带过去。
        t.prevPeakIndex        = t.peakIndex;
        t.prevMinEllipseRate   = t.minEllipseRate;
        t.prevAxisMajorMm      = t.axisMajorMm;
        t.prevVolatileFlags    = t.volatileFlags;
    }

    /// Touch_ProcessAftTouchEvent @0x6BA75AF5：清头部 +0x08/+0x0A、从历史头取回
    /// +0x28/+0x30，再对每个触点调用 Unit 版。本机 +0x1f4 & 0x1000 无任何写入者
    /// （文档 05 §1.1），那四个头部字段恒 0，故这里只保留逐触点遍历。
    void Touch_ProcessAftTouchEvent() noexcept {
        for (auto& t : m_tracks) {
            if (!t.active) continue;
            Touch_ProcessAftTouchEventUnit(t);
        }
    }

    // ── 抬起去抖帧数 DB_LiftOffNum @0x6BA55E79（文档 05 §5.6）────────────
    //
    //   +0x1f8 & 8（曾上报过）为 0 → 0；
    //   +0x1f8 & 0x80（移入修正轨迹）→ 0；
    //   否则区域类型 3 → 3，+0x1f0 & 0x10000000 至少 2，bField_0x111 > 2 至少 2。
    // **普通手指恒为 0**：失配当帧直接出 EventUpReport，不经过 HOLD。
    [[nodiscard]] uint16_t DB_LiftOffNum(const InternalTrack& trk) const noexcept {
        if ((trk.persist1F8 & 0x8u) == 0) return 0;   // 从未上报过
        if ((trk.persist1F8 & 0x80u) != 0) return 0;  // 移入修正轨迹
        uint16_t n = 0;
        if (trk.zoneType == ZoneTypeMultiFinger) n = 3;
        if ((trk.volatileFlags & TOUCH_FLAG_GRIP_MERGE) != 0 && n < 2) n = 2;
        // static.bField_0x111 由 Exception_Process 维护，本机恒 < 2（文档 05 §2.5），
        // 故 `bField_0x111 > 2` 分支不可达，此处不产生额外抬高。
        return n;
    }

    // ── TE_WindowsLiftOffProcess @0x6BA78033（文档 05 §5.6）──────────────
    //
    // 本机 dwFeatureFlags & 0x1000000 = 1，失配轨迹全部走这里；TE_LiftOffProcess
    // 关闭。原版把「保持单元」追加到当前池尾部，复刻的轨迹本身就是跨帧对象，
    // 坐标不更新即等价于 Touch_UnitCopy 拷回历史坐标。
    void TE_WindowsLiftOffProcess(InternalTrack& trk) noexcept {
        const uint32_t prevState = trk.state;
        // 上一帧不是 0x40 / 8 的轨迹，抬起计数从 0 起算（与按下计数共用 +0x4e4）
        if (prevState != EventUpReport && prevState != EventHold) {
            trk.debounceCounter = 0;
        }
        const uint16_t n = DB_LiftOffNum(trk);
        // `(u16)(n+1) − 1`：n = 0xffff 时门限为 −1，两条比较都不成立 → 直接抬起。
        const int limit = static_cast<int>(static_cast<uint16_t>(n + 1)) - 1;
        const int cnt = static_cast<int>(trk.debounceCounter);

        if (cnt < limit) {
            // 抬起去抖期：冻结坐标保持，仍以 Tip=1 上报
            trk.debounceCounter += 1;
            trk.state = EventHold;
            trk.age += 1;
            trk.isReported = true;
            trk.lifeFlags = LifeMapped;
        } else if (cnt == limit || prevState == EventRescue) {
            trk.debounceCounter += 1;
            trk.age += 1;
            const bool fastClick = (trk.persist1F8 & 0x40000u) != 0 &&
                                   prevState == EventIdle &&
                                   trk.exceptionType == 0;
            if (fastClick) {
                // 快速点击补报：最后一帧仍以 Tip=1 进包，ID 随后进黑名单一帧
                trk.state = EventRescue;
                trk.debounceCounter -= 1;
            } else {
                // WindowsPad 抬起帧：全流程里唯一的 Tip=0 帧
                trk.state = EventUpReport;
            }
            trk.isReported = true;
            trk.lifeFlags = LifeLiftOff;
        } else {
            TE_LiftOffReportProcess(trk);
        }
    }

    // ── TE_LiftOffReportProcess @0x6BA77D4B（文档 05 §5.6）───────────────
    //
    // 原版先按快速点击候选决定 0x10 / 0x20，随后 **WindowsPad 使能时无条件改写
    // 成 0x20**，所以本机这条路径的产物恒为 EventUpFinal —— 永不上报、帧尾物理
    // 删除。末尾维护 static.wField_0x122（到 5 即锁住），复刻暂无消费者。
    void TE_LiftOffReportProcess(InternalTrack& trk) noexcept {
        const bool fastClick = (trk.persist1F8 & 0x40000u) != 0 &&
                               trk.state == EventIdle &&
                               trk.exceptionType == 0;
        trk.state = (fastClick && !m_windowsPadFeatureEnabled) ? EventRescue : EventUpFinal;
        trk.age += 1;
        trk.isReported = false;
        trk.lifeFlags = LifeLiftOff;

        // static.wField_0x122：峰幅值高、非移入回放、长轴峰值 < 21 时自增到 5 锁住
        if (m_liftOffStableCount < 5) {
            const bool strong = trk.peakSig > 0 && (trk.persist1F8 & 0x80u) == 0 &&
                                trk.maxAxisMajorMm < 21;
            m_liftOffStableCount = strong ? static_cast<uint16_t>(m_liftOffStableCount + 1) : 0;
        }
    }

    /// TouchReport_ToBeReported @0x6BA7AF9B 对 touch + 0x1f4 bit0 的消费：
    ///   if ((touch[0x1f4] & 1) && touch[0x21c] != 0x10) → 不上报
    /// 复刻把这一条前移到 Stage 5，抬手 Z 不足的触点直接不进上报流。
    /// 最终 Up 报文（0x20）与边界保持态（0x10）不受影响 —— 吞掉 Up 会让
    /// 系统侧的触点永久滞留。
    static void ApplyLiftOffZReportVeto(InternalTrack& trk) noexcept {
        if ((trk.volatileFlags2 & TOUCH_FLAG2_LIFTOFF_Z_FAILED) == 0) return;
        // 判据原文是 `(touch[0x1f4] & 1) && touch[0x21c] != 0x10` —— 只豁免
        // EventRescue；抬起两态本就不由这里决定，一并跳过避免吞掉终结帧。
        if (trk.state == EventRescue || trk.state == EventUpFinal ||
            trk.state == EventUpReport) return;
        trk.isReported = false;
    }

    // ── 4. 屏幕边缘移出检测与外推牵引修正 (ER_Process @ 0x6BA584B3) ──
    //
    // 原版三段：
    //   ER_MoveOutCheck @0x6BA57BD3      —— 用"上一帧坐标 + 两个存档增量"外推，
    //       越过 DAT_6bcab62a(=0) / DAT_6bcab62c-1(=15359) 与 DAT_6bcab62e(=0) /
    //       DAT_6bcab630-1(=10239) 即置 prev+0x204 的 0x10/0x20/0x40/0x80；
    //   ER_MoveOutCorrection @0x6BA57E7D —— 把 **+0x170/+0x172（每帧输出槽）**
    //       吸附到对应的边界值，**不动** +0x162（跨帧跟踪坐标）；
    //   ER_Process 尾部                   —— 置 touch+0x1f0 |= 0x40，并在事件码
    //       为 0x20（本该抬手）时降级成 0x10。0x10 正是 TouchReport_ToBeReported
    //       判据 5 放行的那个值，所以贴边触点即使 nZ4 掉到抬手门限以下也照报不误 ——
    //       这就是原版在最外一列/行维持触点的真实机制（Touch_NeedNextEdgeLowThresh
    //       在本机恒假，边缘并没有任何门限折扣可言）。
    //
    // 进入条件也照原版收紧：事件码必须是 8（移动）或 0x20（抬手），且上一帧该
    // 触点确实上报过（prev + 0x1f8 & 8）。
    void ApplyEdgeRegionProcess() noexcept {
        for (auto& trk : m_tracks) {
            if (!trk.active) continue;
            trk.erFlags = 0;
            // CTD_ECProcess @0x6BA55CA0 每帧把 +0x162/+0x164 拷进 +0x170/+0x172，
            // 此后 ER 与 08 读写的都是这一对。
            trk.outX_q8 = trk.x_q8;
            trk.outY_q8 = trk.y_q8;

            // ── 分岔一：按下帧走入边链路 (ER_TouchDownProcess @0x6BA57FFD) ──
            if (trk.state == EventDown) {
                ErTouchDownProcess(trk);
            } else if (trk.state == 0x20 && trk.erDeferPending) {
                // 原版 ER_Process 的 else 支：上一帧被延后过的触点直接抬手时，
                // 清掉延后位并把它挂进历史回放，让攒下的点补报出去。
                trk.erDeferPending = false;
                trk.erCorrected = true;
                trk.erHistoryPending = true;
            }

            // ── 分岔二：移动/抬手帧走出边链路 (ER_MoveOutCheck/Correction) ──
            ErMoveOutProcess(trk);
        }

        // ── 历史坐标缓冲的**写入**（文档 05 §6.2 末段）────────────────────
        // 原版写入与回放都在 07 的 TouchReport_ProcessUnit /
        // TouchReport_IsHistoryBufferApplied：状态 & 0x38 == 0 的帧把工作坐标
        // 与时间戳压入 prev + 0x290。这里只做**写入**，因为 ER_MoveInCheck 需要
        // 缓冲里至少 2 条历史点；回放由 07 依据 +0x1f8 & 0x80 逐帧执行。
        for (auto& trk : m_tracks) {
            if (!trk.active) continue;
            if ((trk.state & 0x38u) == 0) {
                ErHistPush(trk, trk.x_q8, trk.y_q8);
            }
        }
    }

    // ── 入边链路 (ER_TouchDownProcess @0x6BA57FFD) ─────────────────────
    /// 手指从边框外划进来的那一下按下，首帧重心天然偏内：能量只有一小半落在
    /// 面板内。原版给两级处置 —— 先把这一帧的按下**延后**（事件码改 1，最多攒
    /// 2 次），攒够之后若确认是"快速直线朝边框运动"，就把历史缓冲沿运动反方向
    /// 一路外推补齐到物理边框，再由回放逐帧补报出去。
    void ErTouchDownProcess(InternalTrack& trk) noexcept {
        if (!trk.erDeferPending) {
            // ER_BottomMoveInCorrection：本机 flash[0x928] == 0，整条支路恒假。
            if (!ErIsTouchNeedMoveInCorrection(trk)) return;

            if (trk.erMoveInCount < 2) {
                trk.erDeferPending = true;
                trk.state = EventIdle;          // 原版 touch[0x21c] = 1
                trk.isReported = false;
                trk.erMoveInCount += 1;
                trk.debounceLimit = std::max<uint16_t>(trk.debounceLimit, 2);
                return;
            }
            if (ErMoveInCheck(trk)) {
                trk.erCorrected = true;
                trk.erHistoryPending = true;
            }
            return;
        }

        // 已经延后过：清掉延后位，直接判一次是否值得反向补齐
        trk.erDeferPending = false;
        if (ErMoveInCheck(trk)) {
            trk.erCorrected = true;
            trk.erHistoryPending = true;
        }
    }

    /// ER_IsTouchNeedMoveInCorrection @0x6BA5719B
    /// 先由 TSA_GetDimEdge 定出离哪条边最近，取该边的 [lower, upper] 带宽，要求
    /// 重心到该边的距离**落在带内**且触点主轴不超过 30 mm。
    /// `TPSensor_IsNoMoveInCorrection` 的挖孔区参数（flash 0xC09..0xC0C）全为 0，
    /// 该判据恒假，不复刻。
    [[nodiscard]] bool ErIsTouchNeedMoveInCorrection(const InternalTrack& trk) const noexcept {
        const int dimEdge = ErGetDimEdge(trk.x_q8, trk.y_q8);
        int lower = 0, upper = 0;
        ErGetPrmt(dimEdge, lower, upper);
        if (lower == 0) return false;   // 原版置 g_flagMoveInCorrectedButDisabled 后返回假
        const int d = ErDistToLogicEdge(dimEdge, trk.x_q8, trk.y_q8);
        return (lower < d && d < upper) && trk.axisMajorMm < m_erMaxTouchSizeMm;
    }

    /// ER_MoveInCheck @0x6BA57A07：取历史缓冲最后两点与本帧坐标，要求三段位移
    /// 都够长、且"已经走过的距离"不小于"沿反方向退回边框还要走的距离"，即确认
    /// 这是一笔从边框快速直线划进来的笔画。
    [[nodiscard]] bool ErMoveInCheck(InternalTrack& trk) const noexcept {
        if (trk.histNum <= 1) return false;

        const int p0x = trk.histX[trk.histNum - 2], p0y = trk.histY[trk.histNum - 2];
        const int p1x = trk.histX[trk.histNum - 1], p1y = trk.histY[trk.histNum - 1];
        const int p2x = trk.x_q8,                   p2y = trk.y_q8;

        if (ErSqDist(p0x, p0y, p1x, p1y) < m_erMinStepSqDist) return false;
        if (ErSqDist(p1x, p1y, p2x, p2y) < m_erMinStepSqDist) return false;
        const uint32_t travelled = ErSqDist(p0x, p0y, p2x, p2y);
        if (travelled < m_erMinTravelSqDist) return false;
        if (travelled < ErSqDistToEdge(p0x, p0y, p1x, p1y)) return false;

        ErMoveInCorrection(trk);
        return true;
    }

    /// ER_MoveInCorrection @0x6BA57786：把历史缓冲弹到只剩 2 个点，再以这两点的
    /// 位移为固定步长朝**反方向**逐点外推并压回缓冲头部，直到某一维碰到物理边框
    /// 或缓冲写满。补齐出来的点由 ErHistoryBufferApply 逐帧回放。
    /// 注意原版的循环守卫是 `dx != 0 ∧ dy != 0` —— 严格轴向的笔画不补齐。
    static void ErMoveInCorrection(InternalTrack& trk) noexcept {
        if (trk.histNum <= 1) return;
        while (trk.histNum > 2) ErHistPop(trk);

        const int p0x = trk.histX[0], p0y = trk.histY[0];
        const int dx = static_cast<int>(trk.histX[1]) - p0x;
        const int dy = static_cast<int>(trk.histY[1]) - p0y;
        if (dx == 0 || dy == 0) return;

        int cx = p0x, cy = p0y;
        while (trk.histNum < kErHistoryCap) {
            cx = std::clamp(cx - dx, kErDim1Lo, kErDim1Hi);
            cy = std::clamp(cy - dy, kErDim2Lo, kErDim2Hi);
            ErHistInsertFront(trk, static_cast<uint16_t>(cx), static_cast<uint16_t>(cy));
            if (cx == kErDim1Lo || cy == kErDim2Lo ||
                cx == kErDim1Hi || cy == kErDim2Hi) return;
        }
    }

    // ── 出边链路 (ER_MoveOutCheck / ER_MoveOutCorrection) ──────────────
    //
    //   ER_MoveOutCheck @0x6BA57BD3      —— 用"上一帧坐标 + 两个存档增量"外推，
    //       越过 DAT_6bcab62a(=0) / DAT_6bcab62c-1(=15359) 与 DAT_6bcab62e(=0) /
    //       DAT_6bcab630-1(=10239) 即置 prev+0x204 的 0x10/0x20/0x40/0x80；
    //   ER_MoveOutCorrection @0x6BA57E7D —— 把 **+0x170/+0x172（每帧输出槽）**
    //       吸附到对应的边界值，**不动** +0x162（跨帧跟踪坐标）；
    //   ER_Process 尾部                   —— 置 touch+0x1f0 |= 0x40，并在事件码
    //       为 0x20（本该抬手）时降级成 0x10。0x10 正是 TouchReport_ToBeReported
    //       判据 5 放行的那个值，所以贴边触点即使 nZ4 掉到抬手门限以下也照报不误 ——
    //       这就是原版在最外一列/行维持触点的真实机制（Touch_NeedNextEdgeLowThresh
    //       在本机恒假，边缘并没有任何门限折扣可言）。
    //
    // 进入条件也照原版收紧：事件码必须是 8（移动）或 0x20（抬手），且上一帧该
    // 触点确实上报过（prev + 0x1f8 & 8）。
    void ErMoveOutProcess(InternalTrack& trk) noexcept {
        // 原版 ER_Process @0x6BA584B3 的进入条件只认 **8 / 0x20** 两种状态
        // （文档 05 §6）：0x40 不在检查之列，所以 WindowsPad 下移出修正实际只
        // 对 HOLD 帧与少见的 0x20 帧生效。
        if (trk.state != EventHold && trk.state != EventUpFinal) return;
        // 原版要求历史 +0x1f8 & 8（上一帧确实上报过）。
        if ((trk.persist1F8 & 0x8u) == 0) return;

        // ER_MoveOutCheck @0x6BA57BD3：外推用**历史单元的工作坐标**加两个存档增量
        //   ex = (short)(prev.X162 + prev.d4D4 + prev.dd4D8) = x + 2·d − d_old
        // short 回绕加法、int 比较（文档 05 §6.1）。HOLD 帧不更新运动学，所以
        // 这里用的是最后一个真实运动帧的 d / dd。
        const int extrapX = Fixed::Wrap16(static_cast<int32_t>(trk.x_q8) + trk.dWrkX + trk.ddWrkX);
        const int extrapY = Fixed::Wrap16(static_cast<int32_t>(trk.y_q8) + trk.dWrkY + trk.ddWrkY);

        uint32_t erFlags = 0;
        if (extrapX <= kErDim1Lo)      erFlags |= 0x10; // 左边界逃逸
        else if (extrapX >= kErDim1Hi) erFlags |= 0x20; // 右边界逃逸
        if (extrapY <= kErDim2Lo)      erFlags |= 0x40; // 上边界逃逸
        else if (extrapY >= kErDim2Hi) erFlags |= 0x80; // 下边界逃逸

        trk.erFlags = erFlags;
        if (erFlags == 0) return;

        // ER_MoveOutCorrection @0x6BA57E7D：只把**上报坐标 +0x170/+0x172** 钉到
        // Xmin / Xmax−1 / Ymin / Ymax−1，**不动** +0x162（跨帧跟踪坐标）。
        // 方向位写在历史单元 +0x204，帧尾回灌时被当前值覆盖，不跨帧保留。
        if (erFlags & 0x10) trk.outX_q8 = static_cast<uint16_t>(kErDim1Lo);
        if (erFlags & 0x20) trk.outX_q8 = static_cast<uint16_t>(kErDim1Hi);
        if (erFlags & 0x40) trk.outY_q8 = static_cast<uint16_t>(kErDim2Lo);
        if (erFlags & 0x80) trk.outY_q8 = static_cast<uint16_t>(kErDim2Hi);

        // 本该抬手 ⇒ 降级成 EventRescue：最后一帧仍以 Tip=1 进包，帧尾删除并把
        // ID 放进黑名单一帧（文档 05 §5.1 / §6.1）。
        if (trk.state == EventUpFinal) {
            trk.state = EventRescue;
            trk.isReported = true;
        }
    }

    // ── 历史缓冲 (PrevTouch_HistoryBuffer*，容量 0x1e，index 0 最旧) ─────
    static void ErHistPush(InternalTrack& trk, uint16_t x, uint16_t y) noexcept {
        if (trk.histNum >= kErHistoryCap) ErHistPop(trk);
        trk.histX[trk.histNum] = x;
        trk.histY[trk.histNum] = y;
        ++trk.histNum;
    }

    static bool ErHistPop(InternalTrack& trk, uint16_t* outX = nullptr,
                          uint16_t* outY = nullptr) noexcept {
        if (trk.histNum == 0) return false;
        if (outX != nullptr) *outX = trk.histX[0];
        if (outY != nullptr) *outY = trk.histY[0];
        for (uint8_t i = 1; i < trk.histNum; ++i) {
            trk.histX[i - 1] = trk.histX[i];
            trk.histY[i - 1] = trk.histY[i];
        }
        --trk.histNum;
        return true;
    }

    static void ErHistInsertFront(InternalTrack& trk, uint16_t x, uint16_t y) noexcept {
        if (trk.histNum >= kErHistoryCap) return;
        for (uint8_t i = trk.histNum; i > 0; --i) {
            trk.histX[i] = trk.histX[i - 1];
            trk.histY[i] = trk.histY[i - 1];
        }
        trk.histX[0] = x;
        trk.histY[0] = y;
        ++trk.histNum;
    }

    // TouchReport_IsHistoryBufferApplied @0x6BA7B373 的**回放**段属于模块 07：
    // 它依据 +0x1f8 & 0x80 每帧弹出最旧的一个点当上报坐标，把补齐出来的边框
    // 起点逐帧补报出去。Stage 5 只负责产出该标志位与缓冲内容，回放留给 07
    // （差异 07-01 的另一半）。

    // ── ER 几何辅助 (TSA_GetDimEdge / Touch_GetDistToLogicEdge / SQDist) ──
    static constexpr int kErDim1Lo = 0;
    static constexpr int kErDim1Hi = (kCols * 256) - 1;   // DAT_6bcab62c - 1 = 15359
    static constexpr int kErDim2Lo = 0;
    static constexpr int kErDim2Hi = (kRows * 256) - 1;   // DAT_6bcab630 - 1 = 10239
    static constexpr int kErDim1Bound = kCols * 256;      // DAT_6bcab62c = 15360
    static constexpr int kErDim2Bound = kRows * 256;      // DAT_6bcab630 = 10240

    /// TSA_GetDimEdge @0x6BA8F6F3：取四条边里最近的一条，返回 1/2/4/8
    /// （左 / 右 / 上 / 下）。两维打平时取 Dim1，同维打平时取近端。
    [[nodiscard]] static constexpr int ErGetDimEdge(int x_q8, int y_q8) noexcept {
        const int dx = std::min(x_q8 - kErDim1Lo, kErDim1Bound - x_q8);
        const int dy = std::min(y_q8 - kErDim2Lo, kErDim2Bound - y_q8);
        if (dy < dx) {
            return (kErDim2Bound - y_q8 < y_q8 - kErDim2Lo) ? 8 : 4;
        }
        return (kErDim1Bound - x_q8 < x_q8 - kErDim1Lo) ? 2 : 1;
    }

    /// Touch_GetDistToLogicEdge @0x6BA73009
    [[nodiscard]] static constexpr int ErDistToLogicEdge(int dimEdge, int x_q8, int y_q8) noexcept {
        switch (dimEdge) {
            case 1: return x_q8 - kErDim1Lo;
            case 2: return kErDim1Bound - x_q8 - 1;
            case 4: return y_q8 - kErDim2Lo;
            case 8: return kErDim2Bound - y_q8 - 1;
            default: return 0;
        }
    }

    /// ER_GetPrmt @0x6BA56EC0 + ER_Init @0x6BA56F76 的固化结果。
    /// 原版把 flash 0x924 起的四组 (lowerMM, upperMM, flags) 经
    /// TS_MMToDisplayPixel → `(px >> 4) - 5` → TSAOut_RevertDimNPos 换算成 Q8
    /// 传感器坐标；W273 的四组入参是 (0x40,0xA0) (0x20,0x60) (0x40,0xA0)
    /// (0x40,0xA0)，按 TSA_DimEdgeToSide / TSA_SideToEdge 归位后得到下表。
    static void ErGetPrmt(int dimEdge, int& lower, int& upper) noexcept {
        switch (dimEdge) {
            case 1: lower = 342; upper = 900; break;   // 左：1.34 ~ 3.52 格
            case 2: lower = 342; upper = 900; break;   // 右：同上
            case 4: lower = 365; upper = 960; break;   // 上：1.43 ~ 3.75 格
            case 8: lower = 166; upper = 563; break;   // 下：0.65 ~ 2.20 格
            default: lower = 0; upper = 0; break;
        }
    }

    /// Touch_GetSQDistPixel @0x6BA73447：平方距离，溢出饱和到 0xffffffff
    [[nodiscard]] static constexpr uint32_t ErSqDist(int ax, int ay, int bx, int by) noexcept {
        const int64_t dx = ax - bx;
        const int64_t dy = ay - by;
        const int64_t d = dx * dx + dy * dy;
        return (d > 0xffffffffLL) ? 0xffffffffu : static_cast<uint32_t>(d);
    }

    /// Touch_GetSQDistPixelToEdge @0x6BA7329A：从 p0 沿 (p0 − p1) 方向一路外推到
    /// 物理边框，返回该边框点到 p0 的平方距离 —— 即"退回边框还差多远"。
    [[nodiscard]] static constexpr uint32_t ErSqDistToEdge(int p0x, int p0y,
                                                           int p1x, int p1y) noexcept {
        const int dx = p1x - p0x;
        const int dy = p1y - p0y;
        int cx = p0x, cy = p0y;
        if (dx != 0 && dy != 0) {
            for (int guard = 0; guard < kErDim1Bound; ++guard) {
                cx = std::clamp(cx - dx, kErDim1Lo, kErDim1Hi);
                cy = std::clamp(cy - dy, kErDim2Lo, kErDim2Hi);
                if (cx == kErDim1Lo || cy == kErDim2Lo ||
                    cx == kErDim1Hi || cy == kErDim2Hi) break;
            }
        }
        return ErSqDist(cx, cy, p0x, p0y);
    }

    // ══════════════════════════════════════════════════════════════════
    //  7. 跨帧缓冲（文档 05 §7）
    //
    //  逐格 TZ 时间/信号统计。原版是 g_tsaBufTimeDown / MaxSigDown / TimeOff /
    //  MinSigOff / TimeRpt 五张 2400 元素的表，配 g_tsaBufTzZonePrev 与
    //  g_tsaBufTzPrev 两张上帧快照。它们是**真正的跨帧锁存**，不随帧清零。
    //
    //  复刻侧现状：消费者（TZ 的 g_tsaBufMinSigOff / TimeOff 读点、
    //  PrevTouch_UnitUpdateCommon 的 +0x5A6 快照）都在模块 04，尚未接线；
    //  这里先把产出方按原厂语义实现并留出访问器，避免它们继续缺位。
    // ══════════════════════════════════════════════════════════════════

    /// 饱和到 ±0x7fff 的 16 位加（原版逐处手写，这里收敛成一个helper）
    [[nodiscard]] static int16_t SatAddS16(int16_t a, int32_t b) noexcept {
        const int32_t v = static_cast<int32_t>(a) + b;
        return static_cast<int16_t>(std::clamp<int32_t>(v, -0x7FFF, 0x7FFF));
    }

    /// TSABuffer_PrevFrameProcess（TZ 之前）：只调用 TSABuffer_TimeRptProcess
    /// @0x6BA8EA5F（文档 05 §7.2）。
    ///
    /// 原版分两趟：
    ///   ① 先建一张 **按 TZ 标签索引**的 264 字节布尔表。遍历历史触点池，跳过
    ///      Touch_IsOffEvent(+0x21C) 的，用 g_prevPeaksPtr[prevTouch+0x23C] 的
    ///      (c,r)（条目 +0x0A/+0x0B）去 g_tsaBufTzZone 取标签；标签非 0 时
    ///      `table[label] = (prevTouch[+0x1F8] & 0x20000000) != 0`。
    ///   ② 逐格：table[本格标签] 为真 → TimeRpt = 0，否则 TimeRpt += dt（饱和）。
    ///      标签 0 的格恒落在 table[0]（永不被写）⇒ 一律累加。
    ///
    /// B3 之前 bit29 在复刻侧没有回灌路径，这条判据恒不命中，所以原先写成了
    /// 「只要有任一带位轨迹就清掉所有有标签的格」。接通后必须按标签分辨，
    /// 否则会把不相干区域的驻留计时一起抹掉。
    void TSABuffer_PrevFrameProcess(const TouchBigFrame& bf) noexcept {
        // ① 按标签建表。标签取值域 = TZ 标签基 0x10 起的一字节值，用 256 项覆盖。
        std::array<uint8_t, 256> clearByLabel{};
        const PrevPeakTable& prevPk = bf.prevPeaks;
        for (const auto& t : m_tracks) {
            if (!t.active) continue;                       // ¬Touch_IsOffEvent
            if (t.prevPeakIndex >= kPeakMaxNum) continue;
            const uint8_t c = prevPk.e[t.prevPeakIndex].c;
            const uint8_t r = prevPk.e[t.prevPeakIndex].r;
            if (r >= kRows || c >= kCols) continue;
            const uint8_t label = bf.touchZones[r][c];
            if (label == 0) continue;
            clearByLabel[label] =
                static_cast<uint8_t>((t.persist1F8 & 0x20000000u) != 0);
        }

        // ② 逐格推进。
        for (int i = 0; i < kTotalCellCount; ++i) {
            const uint8_t tz = (&bf.touchZones[0][0])[i];
            m_bufTimeRpt[i] = clearByLabel[tz]
                                  ? int16_t{0}
                                  : SatAddS16(m_bufTimeRpt[i], m_frameIntervalMs);
        }
    }

    /// TSABuffer_PreProcess @0x6BA8F37F（TE 之后）：逐格按「本帧在 TZ / 上帧在 TZ」
    /// 的四种组合维护按下与离开两组统计（文档 05 §7.1）。
    ///   是/否 → TimeDown = 0、MaxSigDown = dif
    ///   是/是 → TimeDown += dt（饱和）、MaxSigDown = max
    ///   否/是 → TimeOff = dt、MinSigOff = dif
    ///   否/否 → TimeOff != 0x7FFF 时 += dt（饱和）、MinSigOff = min
    /// SilentTouchNum 分支在 TE 之后两池数量相等，**不可达**；且本机
    /// FEAT_SAFE_BASELINE 关闭 ⇒ g_tsaBufSilentTouchNum 每帧清零，故整段省略。
    void TSABuffer_PreProcess(const TouchBigFrame& bf) noexcept {
        for (int i = kTotalCellCount - 1; i >= 0; --i) {
            const uint8_t cur = (&bf.touchZones[0][0])[i];
            const uint8_t prev = m_bufTzZonePrev[i];
            const int16_t dif = (&bf.diff[0][0])[i];
            if (cur != 0) {
                if (prev == 0) {
                    m_bufTimeDown[i] = 0;
                    m_bufMaxSigDown[i] = dif;
                } else {
                    m_bufTimeDown[i] = SatAddS16(m_bufTimeDown[i], m_frameIntervalMs);
                    m_bufMaxSigDown[i] = std::max(m_bufMaxSigDown[i], dif);
                }
            } else {
                if (prev != 0) {
                    m_bufTimeOff[i] = static_cast<int16_t>(m_frameIntervalMs);
                    m_bufMinSigOff[i] = dif;
                } else {
                    if (m_bufTimeOff[i] != 0x7FFF) {
                        m_bufTimeOff[i] = SatAddS16(m_bufTimeOff[i], m_frameIntervalMs);
                    }
                    m_bufMinSigOff[i] = std::min(m_bufMinSigOff[i], dif);
                }
            }
        }
        std::memcpy(m_bufTzZonePrev.data(), &bf.touchZones[0][0], kTotalCellCount);
    }

    /// TSABuffer_PostProcess @0x6BA8F4C1：只做 g_tsaBufTzPrev = g_tsaBufTz。
    void TSABuffer_PostProcess(const TouchBigFrame& bf) noexcept {
        std::memcpy(m_bufTzPrev.data(), &bf.touchZones[0][0], kTotalCellCount);
    }

    // ── 8. 异常保持 Touch_KeepPrevTouchWithExceptionExcluded @0x6BA766B9 ──
    //
    // 在 TSA_IsToBypassCurrentFrame 为真时调用，此前 Touch_Clean 已清空当前池。
    // 它**不排除任何触点**（差异 05-10）：对每条轨迹整段 0x144 字节拷回当前池，
    // 当前数 = 轨迹数。函数名里的「ExceptionExcluded」指的是**异常字不继承**
    // —— 当前 +0x20 保持清零后的 0，随后 prev.+0x20 = cur.+0x20 & ~0x400，
    // 即历史异常字被清成 0。本帧不跑 IDT/TS/TE/ER，也不跑 PrevTouch_PostProcess，
    // 输出的是上一帧触点的原样拷贝，状态字仍是上一帧的值。
    //
    // 触发源（文档 05 §8）：raw2raw 统计的「大面积噪声」与「极大噪声」两条，
    // 与触点内容无关。复刻侧由 Stage 0.5 的 HardwareAnalyzer 决定是否进入本帧，
    // 此处只实现保持动作本身，供其调用。
    void Touch_KeepPrevTouchWithExceptionExcluded() noexcept {
        m_prevExceptionWord = 0;   // = cur.+0x20 & ~0x400，当前异常字本就是 0
        for (auto& t : m_tracks) {
            if (!t.active) continue;
            // 整段回拷等价于「什么都不改」：轨迹本身就是历史单元。
            // 唯一要做的是清掉本帧易失量，对位 Touch_UnitCopy 保留
            // +0x1D4 / +0x1DC..+0x1DE / +0x200 的当前值（均为清零后的 0）。
            t.volatileFlags = 0;
            t.volatileFlags2 = 0;
            t.postureFlags = 0;
        }
    }

    // ── 5. 完备性双向单射/满射校验 (TE_AssertProcess @ 0x6BA7872D) ───
    // 四项检查任一失败即返回 1：两池数量不等；索引越界；索引集合未覆盖。
    // 失败后 Touch_Clean + PrevTouch_Release —— **本帧不输出任何触点**，
    // 所有 ID 在下一帧从 1 重新分配（文档 05 §5.8）。
    void AssertAndCleanTracks() noexcept {
        uint32_t seenIds = 0;
        bool failed = m_assertFailed;
        for (auto& trk : m_tracks) {
            if (!trk.active) continue;
            if (trk.id == 0 || trk.id > 20 || (seenIds & (1u << trk.id)) != 0) {
                failed = true;
                continue;
            }
            seenIds |= (1u << trk.id);
        }
        if (failed) {
            for (auto& trk : m_tracks) trk = InternalTrack{};
            m_invalidIdCount = 0;
        }
        m_assertFailed = false;
    }

    // ── PrevTouch_IsValidId @0x6BA6E909 ─────────────────────────────────
    /// 被任一在场轨迹占用、或落在上一帧 EventRescue(0x10) 收集的黑名单里，
    /// 都视为不可用。黑名单每个 MS 帧由 PrevTouch_RemoveOff 清空重建，
    /// 所以以 0x10 结束的 ID 在下一帧不能复用（文档 05 §5.3）。
    [[nodiscard]] bool PrevTouch_IsValidId(uint8_t id) const noexcept {
        for (const auto& t : m_tracks) {
            if (t.active && t.id == id) return false;
        }
        for (uint8_t k = 0; k < m_invalidIdCount; ++k) {
            if (m_invalidIds[k] == id) return false;
        }
        return true;
    }

    // ── TE_GetNextID @0x6BA77536（文档 05 §5.3）──────────────────────────
    //
    //   for (k = 1; k <= 20; k++) { id = k + prevOccupiedID; if (id > 20) id -= 19; ... }
    //
    // 静态量 prevOccupiedID.10025 @0x6BB80F90 位于未初始化段，全库**只有这一处
    // 读、没有任何写入**，恒为 0 ⇒ 回绕分支是死代码，规则就是「1..20 中最小的
    // 空闲 ID」。此前复刻注释写的「Circular collision-free ID allocator」（轮转）
    // 是错的，差异 05-06。
    //
    // 返回 0（全部占用）时 TE_TouchDownProcess 什么也不做：触点 ID 仍为 0、
    // 没有轨迹，随后 TE_AssertProcess 因数量不等而失败，**双池被整体清空**。
    [[nodiscard]] uint8_t TE_GetNextID() const noexcept {
        for (uint8_t k = 1; k <= 20; ++k) {
            if (PrevTouch_IsValidId(k)) return k;
        }
        return 0;
    }
};

// 兼容别名
using Stage5TrackingAndEvents = TrackingAndEvents;

} // namespace Solvers::Touch
