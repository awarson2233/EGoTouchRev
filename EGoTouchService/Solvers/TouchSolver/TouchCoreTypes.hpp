#pragma once

#include <cstdint>
#include <cstddef>

#include "TouchFrameTypes.h"
#include "TouchThold.hpp"
#include "TouchFixed.hpp"
#include "TouchGates.hpp"

namespace Solvers::Touch {

// ── 硬件与显示物理拓扑常量 ──────────────────────────────────────────
inline constexpr int kSensorCols = 60;        // Dim1 (Tx / X轴)
inline constexpr int kSensorRows = 40;        // Dim2 (Rx / Y轴)
inline constexpr int kTotalCells = kSensorCols * kSensorRows; // 2400 互容节点

inline constexpr int kDisplayWidth  = 2560;   // 屏幕物理横向像素分辨率
inline constexpr int kDisplayHeight = 1600;   // 屏幕物理纵向像素分辨率

// 8.8 定点数缩放倍数
inline constexpr int kQ8Scale = 256;          // 1.0f in Q8.8
inline constexpr int kQ8Half  = 128;          // 0.5f in Q8.8 (0x80)

// Q16 显示仿射缩放比例因子
// K_dim1 = (2560 * 256) / 60 = 10922 (0x2AAA)
// K_dim2 = (1600 * 256) / 40 = 10240 (0x2800)
inline constexpr int32_t kAffineScaleDim1 = 10922;
inline constexpr int32_t kAffineScaleDim2 = 10240;

// 最大容量约束
inline constexpr size_t kMaxPeaks   = 32;
inline constexpr size_t kMaxZones   = 20;
inline constexpr size_t kMaxTracks  = 20;
inline constexpr size_t kMaxContacts = 20;

// ── TSACore 触控生命周期与事件标志位 ──────────────────────────────────
// ── TE 事件状态码 touch + 0x21C ──────────────────────────────────────
// 这是**跨三个模块的契约**：TrackingAndEvents(05) 产出、TouchReportAndFilter(07)
// 据它选网闸判据与滤波路径、PitchRemapAndDisplay(08) 据它决定进包与 Tip 位。
// 取值直接采用原厂位码，任何一侧都不得自行定义「等价」编码 —— 编码漂移会让
// 三层各自打补丁，这是复刻早期架构混乱的主要来源。
//
// 语义以反编译实测为准（文档 05 §5、07 §14、08 §5）：
enum TouchEventCode : int {
    EventIdle     = 0x01, // 待确认：网闸拒（判据 9 的 & 0x21），压入回放缓冲，不发包
    EventDown     = 0x02, // 按下帧：放行；不过 IIR（Touch_NeedXYFilter 退化为 TE ∈ {4,0x10}）
    EventMove     = 0x04, // 移动：放行，冻结判定 → IIR → JIT 全链
    EventHold     = 0x08, // 冻结坐标保持：放行；**不压**回放缓冲；不过 IIR，只过 JIT
    EventRescue   = 0x10, // 快速点击补报 / 边缘移出保持：放行，Tip=1 正常进包；过 IIR + JIT
    EventUpFinal  = 0x20, // 通用抬起：网闸拒，且被 Touch_UnitRemove 物理删除，ID 从掩码消失
    EventUpReport = 0x40, // WindowsPad 抬起帧：**唯一的 Tip=0 帧**，压入回放，只过 JIT
};

/// TE 状态在输出层的唯一解释点（对位 TSAOut_PostProcessUnit @0x6BA6A3EF）。
/// 08 之外的任何模块都不应再自行判断「这个状态要不要发包」。
struct PtpDisposition {
    bool inPacket;  ///< 是否进入本帧上报包
    bool tip;       ///< HID Tip 位；仅 EventUpReport 为 false
    bool removeNow; ///< 是否当帧物理删除该单元（ID 随即从掩码消失）
};

[[nodiscard]] constexpr PtpDisposition MapToPtp(int teState) noexcept {
    switch (teState) {
    case EventDown:
    case EventMove:
    case EventHold:
    case EventRescue:     return {true,  true,  false};
    case EventUpReport:   return {true,  false, false}; // 主机看到的唯一抬起
    case EventUpFinal:    return {false, false, true};  // 直接删除，不发任何包
    case EventIdle:
    default:              return {false, false, false};
    }
}

static_assert(MapToPtp(EventRescue).inPacket && MapToPtp(EventRescue).tip,
              "0x10 是 Tip=1 正常进包，不是抬起");
static_assert(!MapToPtp(EventUpFinal).inPacket && MapToPtp(EventUpFinal).removeNow,
              "0x20 永不上报且当帧删除");
static_assert(MapToPtp(EventUpReport).inPacket && !MapToPtp(EventUpReport).tip,
              "0x40 是唯一的 Tip=0 帧");

// ── TZ 区域标志字 zone + 0x4 的位定义 ────────────────────────────────
// 产出方分布在 TZ_GetProperty @0x6BA8B126 调用的一串 TZ_Get*Flag 上；唯一的
// 消费者是 TZ_GetType @0x6BA837F8 与 TZ_UpdateTouchFlag @0x6BA7FE99 —— 后者把
// 这些区域级结论逐位翻译成触点级标志。
enum TouchZoneFlags : uint32_t {
    ZONE_FLAG_SHAPE_HINT        = 0x00000001u, // TZ_GetType 的形态旁路提示
    ZONE_FLAG_CENTROID_GRIP     = 0x00000002u, // 重心落在握持区 (TZ_CentroidGripRatio)
    ZONE_FLAG_EDGE_TOUCHED      = 0x00000020u, // 包围盒贴到面板物理边框 (TZ_GetEdgeTouchedFlag)
    ZONE_FLAG_VIRTUAL_PEAK      = 0x00000040u, // 插入过虚拟峰 (TZ_VPProcess)
    ZONE_FLAG_HTI_VALID         = 0x00000100u, // 面积 > 4 × 峰数 (TZ_GetHTIValidFlag)
    ZONE_FLAG_MULTI_FAT_FINGER  = 0x00000800u, // 多胖指 (TZ_MultiFatFingerCheck)
    ZONE_FLAG_BIG_SHAPE         = 0x00001000u, // 尺寸/跨度超限的大形态 (TZ_GetShapeFlag)
    ZONE_FLAG_SURROUNDED        = 0x00008000u, // 四邻自容通道被包围 (TZ_GetSurroundingStatusFlag)
    ZONE_FLAG_GRIP_MERGE_FORCED = 0x00010000u, // 上一帧已拆分且本帧仍有非握持峰 (TZ_CheckToSeparateZone)
    ZONE_FLAG_LOW_NOISE_CMF     = 0x00020000u, // 峰信号 + 本地噪声仍低于信号门限 (TZ_GetCMFFlag)
    ZONE_FLAG_NEIGHBOR_NOISE    = 0x00040000u, // 相邻线噪声压过峰能量 (TZ_GetCMFFlag)
    ZONE_FLAG_NEAR_EDGE         = 0x00080000u, // 包围盒离边框不足 2 格 (TZ_GetEdgeTouchedFlag)
    ZONE_FLAG_SIDE_TOUCH        = 0x00100000u, // 侧边触控通道 (SideTouch_Process)
    ZONE_FLAG_ABS_GRIP          = 0x00200000u, // 绝对握持 (GripFilter_IsPeakAbsGrip)
    ZONE_FLAG_PEAK_IN_GRIP      = 0x00400000u, // 区域内存在握持峰 (TZ_GetPeakGripFlag)
    ZONE_FLAG_PEAK_NOT_IN_GRIP  = 0x00800000u, // 区域内存在非握持峰 (TZ_GetPeakGripFlag)
    ZONE_FLAG_ZONE_SEPARATED    = 0x01000000u, // 区域被握持分离 (TZ_CheckToSeparateZone)
    ZONE_FLAG_GRIP_ZONE         = 0x08000000u, // 整区判为握持 (GripFilter_RegionProcess)
    ZONE_FLAG_INNER_NON_GRIP_PK = 0x20000000u, // 存在非握持且不在外沿的峰 (TZ_GetPeakGripFlag)
};

// ── TZ 区域类型 zone + 0x8 (TZ_GetType @0x6BA837F8) ────────────────────
enum TouchZoneType : int32_t {
    ZoneTypeNormalFinger = 0,   // TZ_JudgeFFOrNF 的普通手指
    ZoneTypeGrip         = 1,   // 握持区
    ZoneTypeShapeFatFinger = 2, // 形态胖指，走 TZ_RegistForShapeFF
    ZoneTypeMultiFinger  = 3,   // 多指区，走 TZ_MFProcess
    ZoneTypeLargeObject  = 5,   // 大物体 (TZ_IsLO)，TZ_UpdateTouchFlag 据此打 +0x1f0 |= 0x2000
};

// ── TZ 区域异常字 zone + 0x350 (FIException_ProcessTouchZone @0x6BA59BDB) ──
// TZ_RegisterTouch 原样拷进 touch + 0x1ec，即上报网闸判据 4 读的异常类型枚举。
enum TouchZoneException : uint32_t {
    ZONE_EXC_NONE              = 0x0000u,
    ZONE_EXC_DEBOUNCED         = 0x0008u, // FIException_DebounceTouchZone 的收敛值
    ZONE_EXC_PALM              = 0x0040u, // 手掌（HI 分支产出，FI 分支不可达）
    ZONE_EXC_WET_FINGER        = 0x0080u, // FIException_CheckWetFinger
    ZONE_EXC_CHARGER_NOISE     = 0x0100u, // 全局充电器噪声（FI 分支检测器为空壳）
    ZONE_EXC_ABNORMAL_BASELINE = 0x0200u, // FIException_CheckAbnormalTouchZoneBaseline
};

enum TouchLifeFlags : uint32_t {
    LifeMapped        = 1u << 0,
    LifeNew           = 1u << 1,
    LifeLiftOff       = 1u << 2,
    LifeReportAdmit   = 1u << 3,  // 0x1f8 |= 8 准入上报（原版 bit3 语义）
    LifeDebounced     = 1u << 4,
    LifeAlwaysMatch   = 1u << 5,
    LifeEdge          = 1u << 6,  // 本实现的贴边标记，避开原版已占用的 bit3
    LifePalmSuppressed= 1u << 10,
    LifeGripSuppressed= 1u << 11,
};

// ── 阶段数据项结构体 ────────────────────────────────────────────────

// Stage 3 检索出的极大值候选峰
struct PeakCandidate {
    uint8_t  r = 0;
    uint8_t  c = 0;
    int16_t  z = 0;                // peak + 0x18：峰中心的 Dif 值（Peak_Fill 写）
    uint8_t  id = 0;
    uint8_t  age = 0;
    // peak + 0x1c：Peak_CalcZn 求得的 8 邻域代数和（**不含中心**）。
    int32_t  nZ8 = 0;
    // peak + 0x20：Peak_CalcZn 求得的**峰邻域能量**。取四个含中心的 2×2 象限和
    // 与十字形五格和（中心 + 上下左右）中的最大值。Z 幅度准入门（
    // Touch_SetVolatileFlag* 三兄弟）比较的就是它，不是 z 也不是 nZ8。
    int32_t  nZ4 = 0;
    bool     isEdge = false;
    bool     isSuppressed = false;
};

// ── 原厂峰表的逐字节对位副本（文档 03 §架构 / §11 / §12）───────────────
// PeakCandidate 是复刻侧的"好用形式"；下面两张表是**结构对位**形式，存在的
// 理由是有跨阶段消费者按原厂布局读它们：
//   · PeakTable  → g_peaksBeforeSwitchDifBuf @0x6BBC0B40，模块 04 的
//     TZ_BeforeSDCompensationPeakInfoRetrieval @0x6BA8ADEC **无条件**消费；
//   · PrevPeakTable → g_prevPeaksPtr @0x6BB4F110，模块 07 的
//     Touch_SetVolatileFlagSigSum @0x6BA6F7AF 用它算跨帧能量比。
inline constexpr int kPeakMaxNum = 20;   // TSA_MAX_PEAK_NUM = 0x14

/// 0x14 字节 stride 的峰条目。
struct PeakEntry {
    uint32_t flags  = 0;  // +0x00  0x400 in-grip / 0x800 not-in-grip（TZ 阶段写）
    uint8_t  id     = 0;  // +0x04
    uint8_t  c      = 0;  // +0x05
    uint8_t  r      = 0;  // +0x06
    uint8_t  nbrCnt = 0;  // +0x07  合法邻居数 8/5/3
    int16_t  sig    = 0;  // +0x08  Z1 = 峰中心 Dif
    int32_t  z8     = 0;  // +0x0C  Z8 = 合法邻居代数和，**不含中心**
    int32_t  zn     = 0;  // +0x10  Zn = max(四个含中心 2×2 象限和, 十字五点和)
};

/// g_pPeaks 指向的 0x1A0 字节结构。
struct PeakTable {
    uint8_t   count      = 0;          // +0x00
    uint8_t   innerCount = 0;          // +0x01
    int16_t   maxSig     = INT16_MIN;  // +0x02  Peak_Clean 初值 0x8000
    int16_t   minSig     = INT16_MAX;  // +0x04  初值 0x7FFF
    int32_t   maxZn      = INT32_MIN;  // +0x08  初值 0x80000000
    uint8_t   minPeakRow = 0;          // +0x0C  无峰时 = bRows
    PeakEntry e[kPeakMaxNum]{};        // +0x10
};

/// 0x0C 字节 stride 的历史峰条目。Z8 与邻居数**不回灌**。
struct PrevPeakEntry {
    uint32_t flags = 0;  // +0x00
    int32_t  zn    = 0;  // +0x04  ← PeakEntry.zn
    int16_t  sig   = 0;  // +0x08  ← PeakEntry.sig（Z1）
    uint8_t  c     = 0;  // +0x0A
    uint8_t  r     = 0;  // +0x0B
};

/// g_prevPeaksPtr 指向的 0xF4 = 4 + 20×12 字节结构。
struct PrevPeakTable {
    uint8_t       count  = 0;   // +0x00
    int16_t       maxSig = 0;   // +0x02
    PrevPeakEntry e[kPeakMaxNum]{};
};

// Stage 4 BFS 泛洪聚类生成的触控区域
struct ZoneCluster {
    uint8_t  id = 0;
    uint8_t  peakIndex = 0;
    int32_t  m0 = 0;              // 零阶矩 (电荷质量累加)
    int32_t  m1x = 0;             // 一阶矩 X (Tx 方向动量)
    int32_t  m1y = 0;             // 一阶矩 Y (Rx 方向动量)
    uint16_t cogX_q8 = 0;         // 8-bit 定点小数重心 X
    uint16_t cogY_q8 = 0;         // 8-bit 定点小数重心 Y
    uint16_t ecX_q8 = 0;          // 边缘非线性补偿后 X (8.8)
    uint16_t ecY_q8 = 0;          // 边缘非线性补偿后 Y (8.8)
    // Touch_SetEdgeDist @0x6BA72B19（原版 touch+0x22c / +0x22e）：补偿后的重心
    // 到该维两条边界中较近那条的距离。注意量程取 [0, Cols·256] / [0, Rows·256]，
    // 与 DAT_6bcab62a..630 一致，不是"最外一列中心"。GripFilter_IsBackToGrip 读它。
    uint16_t edgeDistDim1_q8 = 0;
    uint16_t edgeDistDim2_q8 = 0;
    uint8_t  minR = 39, maxR = 0;
    uint8_t  minC = 59, maxC = 0;
    uint16_t pixelCount = 0;      // 核心区正向节点数
    uint16_t haloCount = 0;       // 亚门限外壳节点数（原版 base+2 标记）
    int32_t  sigSum = 0;          // 区域正向信号总和
    int16_t  baseSigThold = 0;    // TZ_GetSigThold 求得的泛洪门限
    int16_t  edgeSigThold = 0;    // TZ_GetEdgeSigThold 求得的分类门限
    uint16_t gripRegionMax = 0;   // 区域覆盖到的最大 gripRegion 掩码 (+0x32a)
    uint16_t axisMajorQ8 = 0;     // TZ_GetAxisInternal 长轴 (Q8 节点, +0x2f0)
    uint16_t axisMinorQ8 = 0;     // TZ_GetAxisInternal 短轴 (Q8 节点, +0x2f2)
    int8_t   axisAngleDeg = 0;    // 长轴倾角 (度, +0x2f4)
    uint8_t  axisMajorMm = 0;     // TS_PixelToMM(长轴) (+0x2ec)
    uint8_t  axisMinorMm = 0;     // TS_PixelToMM(短轴) (+0x2ee)
    uint16_t ellipseRate = 0;     // TZ_GetERInernal 椭圆率 ×100 (+0x2a6 → touch+0x1a2)
    bool     fromMultiFinger = false; // 由 SubTZ 拆分产生 (原版 shape == 3, touch+0x1e0)
    // GripFilter 用的边沿能量比 (TZ_UpdateEdgeInfo → TZ_GetGripRatio)
    int32_t  edgeSumDim1 = 0;     // 最外两列信号和 (+0x2cc)
    int32_t  innerSumDim1 = 0;    // 次外两列信号和 (+0x2d0)
    int32_t  edgeSumDim2 = 0;     // 最外两行信号和 (+0x2d4)
    int32_t  innerSumDim2 = 0;    // 次外两行信号和 (+0x2d8)
    int16_t  edgeMaxDim1 = 0;     // 最外两列核心最大信号 (+0x2dc)
    int16_t  edgeMaxDim2 = 0;     // 最外两行核心最大信号 (+0x2e0)
    uint8_t  gripRatioDim1 = 0;   // (edgeSum << 4) / innerSum，上限 255 (+0x334)
    uint8_t  gripRatioDim2 = 0;   // 同上 (+0x335)
    uint8_t  gripRatio = 0;       // 依据贴边方向选出的有效比值 (touch+0x198)
    uint8_t  gripEdgeFlags = 0;   // 1=上 2=下 4=左 8=右 (touch+0x1c4)
    uint8_t  gripLevel = 0;       // 重心处 gripRegion 的等级 0..3 (GripFilter_GetCenterLevel)
    bool     isGripWall = false;

    // ── 区域级结论（TZ_GetProperty 尾段产出，TZ_RegisterTouch 消费）──────
    uint32_t zoneFlags = 0;       // zone + 0x4，见 TouchZoneFlags
    int32_t  zoneType = ZoneTypeNormalFinger; // zone + 0x8，见 TouchZoneType
    uint32_t exceptionState = ZONE_EXC_NONE;  // zone + 0x350，见 TouchZoneException
    // zone + 0x2e：TZ_GetAveCentSig 求得的重心邻域平均 Dif。TZ_RegisterTouch 把它
    // 写进 touch + 0x29e，再由 Touch_UnitUpdateMaxMin 锁存成 touch + 0x1ba。
    int16_t  centSig = 0;
    // zone + 0x2f5：TZ_GetShapeRatio 求得的"实际面积 / 椭圆理论面积"×16，TZ_IsLO 用。
    uint8_t  shapeRatio = 0;
    // zone + 0x10：FIException_CheckWetFinger 泛洪出的水膜格数。
    uint16_t waterSize = 0;

    // ── SubTZ 全局量快照（文档 04 §6.7 / §6.8、复刻要点 17）─────────────
    // 原版 DAT_6bbc8684 / DAT_6bbc8686 是**跨 TZ 累积**的「最大子区 coreCount」
    // 与「该子区下标」，整帧只在 TSA_MSProcess 级别复位，SubTZ_Process 不按 TZ
    // 清零。TZ_JudgeFFOrNF @0x6BA8D1ED、SubTZ_IsMF @0x6BA7EC1D、
    // TZ_MultiFatFingerCheck @0x6BA836A1、TZ_IsLO @0x6BA83474 四处都读它。
    // 复刻的 ClassifyZone 跑在全部 TZ 之后，直接读运行期全局会读到帧末值；
    // 因此在区域产出的**那一刻**把累积值快照进来，等价于原版「累积到本 TZ 为止」。
    uint16_t maxSubCoreCount = 0;   // DAT_6bbc8684
    uint8_t  maxSubMajorMm = 0;     // g_subTZ[DAT_6bbc8686] + 0x30
    uint8_t  maxSubMinorMm = 0;     // g_subTZ[DAT_6bbc8686] + 0x32
    // SubTZ_GetNumAftFilter() = DAT_6bbc8681，SubTZ_Filter 过滤后存活的子区数。
    uint8_t  subKeptNum = 0;
};

// Stage 5 匈牙利算法追踪槽位
struct TrackingSlot {
    uint8_t  id = 0;
    int      state = EventIdle;
    uint16_t curX_q8 = 0;
    uint16_t curY_q8 = 0;
    uint16_t prevX_q8 = 0;
    uint16_t prevY_q8 = 0;
    int16_t  vx_q8 = 0;
    int16_t  vy_q8 = 0;
    int16_t  ax_q8 = 0;
    int16_t  ay_q8 = 0;
    float    sizeMm = 0.0f;
    int32_t  m0 = 0;              // 区域信号总和 (原版 touch+0x1cc)
    uint8_t  axisMajorMm = 0;     // 触点长轴 (mm, 原版 touch+0x188)
    uint8_t  axisMinorMm = 0;     // 触点短轴 (mm, 原版 touch+0x18a)
    uint8_t  extentX = 0;         // 区域包围盒列跨度 (节点, 原版 touch+0x196)
    uint8_t  extentY = 0;         // 区域包围盒行跨度 (节点, 原版 touch+0x197)
    uint16_t ellipseRate = 0;     // 形态椭圆率 ×100 (原版 touch+0x1a2)
    bool     fromMultiFinger = false; // 源区域由 SubTZ 拆分产生 (原版 touch+0x1e0 == 3)
    uint8_t  gripRatio = 0;       // 边沿能量比 (原版 touch+0x198)
    uint8_t  gripEdgeFlags = 0;   // 贴边方向掩码 (原版 touch+0x1c4)
    uint8_t  gripLevel = 0;       // 重心处握持区等级 0..3
    // 原版 touch + 0x210：GripFilter_GetStage @0x6BAE492D 每帧写的握持阶段
    // 0..4。GripFilter_IsGrip @0x6BAE193F 判 stage ∈ {0,1,2} 为握持。产出方是
    // 模块 06，消费方是模块 06 自身的下一帧、以及模块 05 的 IDT_IsToIncAccThold。
    int32_t  gripStage210 = 0;
    // 原版 touch + 0x22c / +0x22e，由 Touch_SetEdgeDist 从**补偿后**的重心算出，
    // 随区域一路带到触点上。GripFilter_IsBackToGrip 的几何判据读它。
    uint16_t edgeDistDim1_q8 = 0;
    uint16_t edgeDistDim2_q8 = 0;
    uint16_t age = 0;
    uint16_t debounce = 0;

    // ── Stage 5 的 TE 收尾产出（Touch_ProcessAftTouchEventUnit @0x6BA7542A 与
    //    Touch_IIRDataProcess @0x6BA7474F，文档 05 §5.4）────────────────────
    // 这一组是模块 06 GripFilter 的 AbsGrip / TempGrip 比率判据的**唯一输入**，
    // 也是模块 04 收紧 TZ_GetDBShape 所需的轨迹形态/年龄来源。
    uint16_t rawAxisMajorMm = 0;   // touch + 0x22a，本帧原始长轴 mm
    uint16_t maxAxisMajorMm = 0;   // touch + 0x228，长轴在整条轨迹内的历史峰值
    uint8_t  shapeSnapshotValid = 0; // touch + 0x25c，形状 EMA 快照有效
    uint16_t iirAxisMajorMm = 0;   // touch + 0x25e
    uint16_t iirAxisMinorMm = 0;   // touch + 0x260
    uint16_t iirAxisMajorPx = 0;   // touch + 0x262
    uint16_t iirAxisMinorPx = 0;   // touch + 0x264
    int8_t   iirAxisAngleDeg = 0;  // touch + 0x266
    uint8_t  axisRatioX16 = 0;     // touch + 0x267，长短轴比 ×16
    int16_t  iirMinEllipseRate = 0; // touch + 0x268
    uint16_t iirGripRatioOuter = 0; // touch + 0x26a
    uint16_t iirGripRatioInner = 0; // touch + 0x26c
    // touch + 0x1f8：持久标志字。bit3 = 这条轨迹**曾经真的上报过**，
    // 模块 04 的 TZ_GetDBShape 需要的正是「上一帧该轨迹真的上报过」。
    uint32_t persistFlags1F8 = 0;

    // ── 模块 07 上游契约（文档 07 §8 的冻结四分支唯一输入）────────────────
    // 原版 touch + 0x224：TouchAction 动作字。1 = 自按下起位移未超过 20 外部像素
    // 且未超时；2 = 已移动 / 长按。冻结分支 1 读 prev 的值、分支 3 读当前值。
    // 产出方 TouchAction_ProcessUnit（06），复刻里由 PalmAndGripRejection 发布。
    uint32_t actionWord = 1;
    // 原版 touch + 0x1dc / +0x1dd / +0x1de：Touch_SetVolatileFlagSigSum @0x6BA6F7AF
    // 产出的三个跨帧能量比，统一形式 `min(255, (本帧量 << 5) / 上帧量)`，分母为 0
    // 或历史峰下标越界时取 255（= 无衰减）。
    //   sigSumRatio(+0x1dc) = 触点零阶矩比，Filter_IsHandLift 判 `< 0x1c`；
    //   znRatio   (+0x1dd) = 峰邻域 Zn 比，冻结分支 1 判 `< 0x10`；
    //   z1Ratio   (+0x1de) = 峰中心 Dif 比，冻结分支 1 判 `< 0x14`。
    // 复刻的模块 05 尚未产出这三个量，缺省 255 等价于「无能量塌陷」，与原版
    // 「历史峰不可用即置 255」的兜底路径一致。
    uint8_t  sigSumRatio = 255;
    uint8_t  znRatio     = 255;
    uint8_t  z1Ratio     = 255;
    // 原版 touch + 0x1be / 源峰 peak+0x18：本帧峰中心 Dif。冻结分支 1 的第三条判据
    // `peak[+0x18] < g_tsaPrmtDynamic / 2` 读它。
    int16_t  peakSig = 0;

    uint32_t lifeFlags = 0;
    // 原版 touch + 0x1f0 / + 0x1f4 两个易失标志字。Stage 5 的 Z 幅度准入门
    // （Touch_SetVolatileFlag*）写它们，Stage 7 的 TouchReport_ToBeReported 读。
    uint32_t volatileFlags = 0;
    uint32_t volatileFlags2 = 0;
    // 原版 touch + 0x208 抑制标志字（读取器 TSA_RptTouchSuppressFlag @0x6BA9C4DD）。
    // 写入方是 TouchMode_GloveFilter / TouchMode_StylusSuppression /
    // StylusAft_SuppressBaseContext。前两者挂在 Stage 6.5 的 TouchModeFsm 上：
    // bit1 由 TouchMode_GloveFilter 产出（基线复位窗口内可达），bit2 的调用前置
    // 条件与手套模式的进入条件互斥 ⇒ 本机不可达。StylusAft 属手写笔通道，不复刻。
    uint32_t suppressFlags = 0;
    // 原版 touch + 0x1ba：Touch_UnitUpdateMaxMin @0x6BA75xxx 跨帧锁存的信号刻度。
    // 递推式是 `prev < cur ? min(cur, peakFilterRef) : prev`，即**钳在
    // wPeakFilterReferenceThreshold 以内的历史最大重心信号**（函数名里的 Min 指的是
    // 那个 min 钳位，不是取小锁存）。只被 TouchReport_ToBeReported 的手套弱信号
    // 判据消费，判据形式是 `+0x1ba < peakFilterRef / 2`。
    int16_t  minSigLatched = 0;
    // 原版 touch + 0x1ec：区域异常类型枚举，由 TZ_RegisterTouch 从 zone + 0x350 原样拷入。
    uint32_t exceptionType = ZONE_EXC_NONE;
    // 原版 touch + 0x218：持久标志字。bit0 由 TZ_UpdateTouchFlag 在区域标志
    // ZONE_FLAG_ZONE_SEPARATED 命中时置位，下一帧回灌给 TZ_CheckToSeparateZone。
    uint32_t persistFlags = 0;
    // 原版 touch + 0x214：握持通道标志字。bit10 来自区域标志 ZONE_FLAG_GRIP_ZONE，
    // bit31 来自 ZONE_FLAG_INNER_NON_GRIP_PK ∧ ¬ZONE_FLAG_ZONE_SEPARATED。
    uint32_t gripChannelFlags = 0;
    // 原版 touch + 0x200：姿态/多胖指标志字。bit1 来自 ZONE_FLAG_MULTI_FAT_FINGER。
    uint32_t postureFlags = 0;
    bool     isPalm = false;
    bool     isGrip = false;
    bool     isReported = false;

    // Stage 7 滤波与余数累加器
    uint16_t fltX_q8 = 0;
    uint16_t fltY_q8 = 0;
    uint8_t  remX = 0;            // 低 8 位小数余数累加 (Noise Shaping)
    uint8_t  remY = 0;

    // 原版 touch + 0x170 / +0x172：**每帧临时的输出坐标槽**。
    // CTD_ECProcess @0x6BA55CA0 每帧先把 +0x15e/+0x160（原始重心）拷进 +0x162/+0x164
    // （EC 工作槽），跑完 CTD_ECProcessUnit 再把 +0x162/+0x164 拷进 +0x170/+0x172。
    // 此后 ER_MoveOutCorrection、TSA_SetXPos（边界硬裁剪反向投影）与
    // TSA_PitchSizeRemapProcess 读写的都是这一对，跨帧持久的滤波坐标不受污染。
    // 复刻里 fltX_q8/fltY_q8 承担 +0x162 的角色，Stage 8 入口把它们播种到这里。
    uint16_t outX_q8 = 0;
    uint16_t outY_q8 = 0;

    // Stage 8 最终屏幕像素坐标
    uint16_t dispX = 0;           // 0 .. 2559
    uint16_t dispY = 0;           // 0 .. 1599

    // 文档 08 §5：HID Tip 位。原版 g_report 里**没有**事件字段，宿主靠导出
    // TSA_RptTouchEvent @0x6BA9B3CC 原样读 +0x21C 再判 `!= 0x40` 合成 Tip。
    // 复刻把这条唯一解释点（MapToPtp）的结论落在这里，由
    // TSAOut_PostProcessUnit @0x6BA6A3EF 的等价实现每帧写。
    bool     tipDown = true;
};

// ── 共享大帧 (TouchBigFrame) ────────────────────────────────────────
// 整个管线 Stage 0 ~ Stage 8 共享同一物理数据总线，实现零拷贝原位处理
struct TouchBigFrame {
    // 1. 矩阵缓冲区 (40 行 x 60 列 = 2400 单元)
    int16_t  raw[kSensorRows][kSensorCols];         // AFE 原始互容帧
    // 动态基线矩阵曾在这里镜像一份（BaselineAndDiff 每帧 memcpy 4800 B 过来），
    // 但全工程没有任何消费者 —— 基线只有 Stage 1 自己用。已删除。
    int16_t  diff[kSensorRows][kSensorCols];        // 差分矩阵 (Dif = Baseline - Raw)
    int16_t  preDif[kSensorRows][kSensorCols];      // 上一帧差分矩阵
    int16_t  cmNoise[kSensorRows][kSensorCols];     // 2D 估计共模噪声图谱 (g_tsaBufCommonModeNoise)
    int16_t  cmOffsets[kSensorRows];                // 每个 Dim1 切片(行)提取的共模偏置

    // Stage 2 Baseline_Compensation 求得的全屏基线平移量，由 Stage 1 在下一帧起始处消费。
    // 原版中 CMF 与 BLSM 共用 g_tsaBufBl，此字段等价于那条隐式回灌路径。
    int16_t  baselineDriftStep = 0;

    // 2. 空间特征与掩码矩阵
    uint16_t gripRegion[kSensorRows][kSensorCols];  // 2D 握持掩码 (0x400 GripWall 等)
    uint8_t  touchZones[kSensorRows][kSensorCols];  // 4-连通 BFS 聚类划分区域 (1~20, 0=背景)
    uint8_t  zoneEdge[kSensorRows][kSensorCols];    // 触控区边缘标记 (用于诊断可视化)

    // ── 逐格握持驻留回路 (g_tsaBufGripTime / g_tsaBufGripStage) ────────
    // 这两张图是**跨帧锁存**的，ResetPerFrame 不清它们。
    //   gripTime：TSABuffer_GripTimeProcess @0x6BA8EEA1 每帧推进。该格 Dif 高于
    //     GripFilter_GetTrackingThold()（= wPeakMode3ThresholdMax / 2 = 300）就把
    //     帧间隔累加上去（饱和到 0x7fff），否则清零 —— 即"该格被按住了多久"。
    //   gripStage：TSABuffer_GripStageProcess @0x6BA8F274 在帧尾按触点结论逐格写。
    //     下一帧 TZ_TraversalNeighbor 读它：置位的格子**无条件判为核心格**，
    //     绕过泛洪门限。这是原版让贴边触点在最外一列信号摆动时不掉包围盒的
    //     唯一机制，缺了它 EC 的进入判据会逐帧开关，坐标整格跳动。
    int16_t  gripTime[kSensorRows][kSensorCols] = {};
    uint8_t  gripStage[kSensorRows][kSensorCols] = {};

    // 3. 阶段特征列表
    FixedVector<PeakCandidate, kMaxPeaks>   peaks;
    FixedVector<ZoneCluster, kMaxZones>     zones;
    FixedVector<TrackingSlot, kMaxTracks>   tracks;
    FixedVector<TouchContact, kMaxContacts> contacts;

    // 3.4 原厂布局的峰表副本，由 Stage 3 PeakAndGripRegion 每帧产出。
    //   peaksBeforeSwitchDif（文档 03 §11）：Peak_Process 之后立刻做的 0x1A0
    //     字节快照 g_peaksBeforeSwitchDifBuf @0x6BBC0B40。本机
    //     SS_ChargerNoiseFilterSwitchBuffer / SignalDisparity_SwitchBuffer 恒 0
    //     ⇒ Peak_Process 永不重跑，快照内容与本帧峰表一致；但
    //     TZ_BeforeSDCompensationPeakInfoRetrieval @0x6BA8ADEC **无条件**读它，
    //     按 count/col/row/sig 统计落入当前 TZ 的"补偿前峰"个数与最强者索引
    //     （TZ 单元 +0x2B4 / +0x2B5），所以这条通道必须存在。
    //   prevPeaks（文档 03 §12）：PrevPeak_Process @0x6BA68F77 帧尾回灌的历史
    //     表。**跨帧锁存**，ResetPerFrame 不清它。
    PeakTable     peaksBeforeSwitchDif{};
    PrevPeakTable prevPeaks{};

    // 3.5 本帧运行时动态门限组（Stage 2.5 TouchThold 重算，对位原版
    // TSAPrmt_Process 每帧覆写的 DAT_6bcab688 家族），供 Stage 3 起的各级消费。
    TouchTholdSet tholds{};

    // 3.5b 本帧全屏正峰值（g_tsaPrpt.nSPeakPositive）。Stage 2.5 扫一遍 diff
    // 得到，Stage 4 原本自己又扫了一遍**同一块内存算同一个量**（中间没有任何
    // 人改写 diff）。挂在这里共用，省掉一趟 2400 格全屏遍历。
    // framePeakValid 为 false 时（独立入口、未经 Stage 2.5）由消费方自行扫描。
    int16_t framePeakPositive = 0;
    bool    framePeakValid = false;

    // 3.6 本帧 Peak 级给出的全互感信号分散结论，对位原版
    // g_tsaStatic->bFullIncellSignalDisparity（+0x114）。Peak_Process 写、
    // 下一帧 TouchThold_GetPeakThold 读，构成跨帧闭环；TouchPipeline 在
    // Peak 级之后把它同步进 m_dynThold.m_inputs 完成回写。
    // 它是**锁存量**，ResetPerFrame 不清它。
    bool fullIncellSignalDisparity = false;

    // 4. 帧级状态与控制元数据
    uint64_t frameIndex = 0;
    uint64_t timestampUs = 0;
    // Touch_GetFormattedTimeInterval 的归一化帧间隔 (ms)，Stage 5 每帧写。
    // 原版 g_tsaStaticPtr->nBaselineStageInitialSignal 就是这个量，
    // TSABuffer_GripTimeProcess / TSABuffer_TimeRptProcess 用它推进逐格计时。
    int32_t  frameIntervalMs = 10;
    bool     masterSuffixValid = false;
    bool     slaveSuffixValid = false;
    bool     hasFinger = false;                     // Suffix 硬件探测快速标志
    bool     isChargerNoise = false;                // 充电器噪声判决
    // TSAStatic_IsChargerPlugged @0x6BA6A434 的等价输入：充电器已插入。
    // 原版由 TSAStatic_PreProcess 从宿主传入的电源状态推导，复刻侧目前没有
    // 这条通道，默认 false；接上电源状态上报后由 Stage 0 每帧填写。
    // TZ_GetSigThold 用它切到 TZ_GetSigTholdForCharger 门限模型。
    bool     chargerPlugged = false;
    bool     hasLiveTouches = false;                // 当前帧是否存在有效触控
    uint32_t activeTouchMask = 0;                   // 活跃触点 ID 掩码

    // 原版 g_touchesPtr[3]：被「区域 type == 5（大物体）」判据拒报的触点计数，
    // 由 07 的 TouchReport_ToBeReported 累加。08 的 TSAOut_PostProcess
    // @0x6BA6A77A 读它做**整帧封杀**（文档 08 §1）：本帧只要有一个点因大物体
    // 被拒，且 FEAT_SMART_COVER 关、不在游戏场景，就 `*g_touchesPtr = 0`。
    uint8_t  cancelledTouchCount = 0;

    // 原版全局触控状态字 g_touchesPtr[0x24]。TZ_UpdateTouchFlag @0x6BA7FE99 在区域
    // 标志命中 ZONE_FLAG_BIG_SHAPE 时置 bit3（同时给该触点打 touch + 0x1f0 |= 0x20000）、
    // 命中 ZONE_FLAG_SURROUNDED 时置 bit4；AntiTouch_* 置 bit5。
    // TouchReport_ToBeReported 只读 bit3。由 Stage 5 的 TZ_UpdateTouchFlag 每帧重建。
    uint32_t touchGlobalFlags = 0;

    // 原版 g_tsaStaticPtr->dwSpecialControlFlags。TZ_UpdateTouchFlag 在
    // ZONE_FLAG_SURROUNDED / 区域 type == 5 时置 0x800，在 ZONE_FLAG_LOW_NOISE_CMF /
    // ZONE_FLAG_NEIGHBOR_NOISE 时置 0x20；Exception_Process 在异常态跳变时置 0x80。
    // 原版这是一个跨阶段的"需要特殊控制"汇总字，复刻侧只做产出、暂无消费者。
    uint32_t specialControlFlags = 0;

    // 原版 BLReset_InDebounce() @0x6BA47750 的每帧结论（g_tsaStatic->bBaselineResetInDebounce）。
    // 由 Stage 1 BaselineAndDiff 在本帧基线阶段推进后回填。
    bool     baselineResetInDebounce = false;

    // ── 模块 06 产出的三个帧头字段（文档 06 §4.1 / §6 / §9）────────────
    // 原版 g_touchesPtr[0x0D]：本帧握持阶段 ∈ {1,2} 的触点数，唯一产出方是
    // GripFilter_BasicProcess @0x6BAE4C29。TouchMode_NoObjDetect / GloveDetect
    // 拿它和触点总数比较，是 NoObj / Glove 两态的**唯一**输入。
    uint8_t  gripSuppressedCount = 0;
    // 原版 g_touchesPtr + 0x18：Thenar_EventUpdate 置 bit1/2（左/右）、bit3/4
    // （带手势），AntiTouch_Report 置 bit24/25。只走导出接口，不影响手指报点。
    uint32_t thenarHeaderFlags = 0;
    // 原版 g_touchesPtr + 0x24：AntiTouch 的帧级异常字。
    // 0x20 = 大面积异常 / 0x80 = 接近 / 0x100 = 接近保护。主路径无消费者。
    uint32_t antiTouchHeaderFlags = 0;

    inline void ResetPerFrame() {
        peaks.clear();
        zones.clear();
        contacts.clear();
        hasLiveTouches = false;
        activeTouchMask = 0;
        touchGlobalFlags = 0;
        specialControlFlags = 0;
        framePeakValid = false;
        for (int r = 0; r < kSensorRows; ++r) {
            for (int c = 0; c < kSensorCols; ++c) {
                gripRegion[r][c] = 0;
                touchZones[r][c] = 0;
                zoneEdge[r][c] = 0;
                cmNoise[r][c] = 0;
            }
        }
        for (int r = 0; r < kSensorRows; ++r) {
            cmOffsets[r] = 0;
        }
    }
};

} // namespace Solvers::Touch
