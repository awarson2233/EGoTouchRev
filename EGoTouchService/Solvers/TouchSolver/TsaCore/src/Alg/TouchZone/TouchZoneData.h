#pragma once
// TZ 的触摸区域单元与区域图访问句柄。

#include <cstddef>
#include <cstdint>

#include "Common/Bss.h"

namespace Solvers::Tsa::Core {

inline constexpr uint8_t kMaxZonePeakNum = 10;  // 一个区域最多登记的峰数

/// 轮廓点，高分辨率坐标（每格 4 个点）。
struct EdgeDot {
    int16_t dim1;
    int16_t dim2;
};
static_assert(sizeof(EdgeDot) == 0x004);
static_assert(offsetof(EdgeDot, dim1) == 0x000);
static_assert(offsetof(EdgeDot, dim2) == 0x002);

/// TZ 的一个触摸区域：从一个峰出发做区域生长时逐格累计，再由各属性判定形状并登记成触点。
/// 「边」指 FlashPrmt 的逻辑边（dim1LogicFirst / Last 等），「次边」是往里一格的那一行（列）。
struct TouchZoneUnit {
    int16_t sigThold;  // 区域生长门限；SubTZ 以它为下限，轮廓按它二值化
    uint8_t reserved002[0x2];
    uint32_t flags;  // kZone*
    uint32_t shape;  // kShape*
    uint16_t cellCount;  // 达到门限的正信号格数
    uint16_t negCellCount;  // 达到门限的非正信号格数
    uint16_t waterCells;  // FIException：沿历史轨迹累计的水膜格数
    uint8_t reserved012[0x2];
    int32_t sigSumAll;  // 遍历到的格子的信号和（含负值）
    int32_t sigSum;  // 正信号和
    int32_t centroidWeightDim1;  // 质心的权重和：正信号减去 centroidSigOffsetDim1（不小于 0）
    int32_t centroidWeightDim2;
    int32_t gripRegionSigSum;  // 落在握持区域里的正信号和
    int32_t negSigSum;  // 非正信号和
    int16_t minSig;  // 最小（最负）信号及其位置
    int16_t maxSig;
    uint8_t minSigDim1;
    uint8_t minSigDim2;
    uint8_t avgSigRatio;  // 每峰平均信号相对触点门限（或最强峰）的比值（×16），封顶 255
    uint8_t minDim1;  // 外接框
    uint8_t maxDim1;
    uint8_t minDim2;
    uint8_t maxDim2;
    int8_t edgeMinDim1;  // 落在 Dim2 边上的格子的 Dim1 范围（无时为 0x7f / 0x80）
    int8_t edgeMaxDim1;
    int8_t edgeMinDim2;  // 落在 Dim1 边上的格子的 Dim2 范围
    int8_t edgeMaxDim2;
    int8_t nearEdgeMinDim1;  // 同上，次边
    int8_t nearEdgeMaxDim1;
    int8_t nearEdgeMinDim2;
    int8_t nearEdgeMaxDim2;
    uint8_t edgeMinDim1Row;  // edgeMinDim1 / edgeMaxDim1 所在的行
    uint8_t edgeMaxDim1Row;
    uint8_t edgeMinDim2Col;  // edgeMinDim2 / edgeMaxDim2 所在的列
    uint8_t edgeMaxDim2Col;
    uint8_t edgeWidthDim1;  // 沿边按 aftEdgeWidthThold 往两侧延伸后的宽度
    uint8_t edgeWidthDim2;
    uint8_t edgeWidthSum;
    int16_t hrMinDim1;  // 轮廓插值范围：外接框外扩一格（夹在屏内），高分辨率坐标
    int16_t hrMaxDim1;
    int16_t hrMinDim2;
    int16_t hrMaxDim2;
    EdgeDot edgeDot[150];  // 轮廓点，从最强峰正上方的边界起逆时针
    int16_t edgeDotCount;
    int16_t circleRate;  // 轮廓到中心距离的离散度（×100），0 为正圆
    int16_t ellipseRate;  // 按轮廓自身的椭圆拉伸成圆后的离散度（×100）
    int16_t ellipseRateMirror;  // 贴边的轮廓去掉贴边点、镜像补全后的 ellipseRate
    uint16_t centroidDim1;  // 质心（Q8.8）
    uint16_t centroidDim2;
    uint8_t peakCount;  // 区域内的峰数
    uint8_t seedPeakIdx;  // 生长的起点峰
    uint8_t sdcPeakCount;  // 区域覆盖的、切换 Dif 前的峰数
    uint8_t sdcMaxPeakIdx;  // 其中峰值最大者（g_peaksBeforeSwitchDifBuffer 下标）
    uint8_t maxSigPeakIdx;  // 区域内峰值最大的峰
    uint8_t minSigPeakIdx;  // 区域内峰值最小的峰
    uint8_t maxZPeakIdx;  // 区域内峰值 + Z8 最大的峰
    uint8_t peakIdx[10];  // 区域内的峰（峰表下标）
    uint8_t reserved2C3[0x1];
    int32_t centroidMomentDim1;  // 质心的一阶矩：权重 ×（格坐标 ×128 + 传感器偏移）/ 128
    int32_t centroidMomentDim2;
    int32_t edgeSumDim1;  // Dim1 边上的信号和
    int32_t nearEdgeSumDim1;  // Dim1 次边
    int32_t edgeSumDim2;
    int32_t nearEdgeSumDim2;
    int16_t edgeMaxSigDim1;  // Dim1 边上达到门限的最大信号
    int16_t nearEdgeMaxSigDim1;  // Dim1 次边上未达门限的最大信号
    int16_t edgeMaxSigDim2;
    int16_t nearEdgeMaxSigDim2;
    int32_t maxCentZ9;  // 两峰中点附近各格 Z9 的最大值
    uint8_t centToMinPkRatio;  // 最近两峰的中点信号相对其中较弱峰的比值（×64）
    uint8_t reserved2E9[0x1];
    uint16_t curMajorMm;  // 登记成触点前的 majorMm
    uint16_t majorMm;  // 椭圆长轴（mm）
    uint16_t minorMm;
    uint16_t majorAxis;  // 椭圆长轴（Q8.8）
    uint16_t minorAxis;
    int8_t angle;  // 长轴方向（度）
    uint8_t shapeRatio;  // 格数相对椭圆面积的比值（×16），椭圆退化时为 255
    uint8_t reserved2F6[0x2];
    int32_t moment0;  // 椭圆用的各阶矩：|信号| 加权（达到门限或 AFT 模式下的格子）
    int32_t momentDim1;
    int32_t momentDim2;
    uint8_t reserved304[0x4];
    int64_t momentDim1Dim2;
    int64_t momentDim1Sq;
    int64_t momentDim2Sq;
    int32_t areaToSigRatio;  // 参考信号 × 椭圆面积 / sigSum，封顶 255
    uint8_t prevIdx;  // 对应的历史触点，kNoTouch 表示没有
    uint8_t peakId;  // 起点峰的 ID
    uint16_t gripRatioRawDim1;  // Dim1 边相对次边的信号比（×16），不封顶
    uint16_t gripRatioRawDim2;
    uint8_t gripRegion;  // 覆盖到的格子里最大的握持区域编号
    uint8_t reserved32B[0x1];
    int32_t outEdgeSumRatio;  // 边上信号和相对历史触点的百分比
    uint16_t moveDim1;  // 质心相对历史触点预测位置的偏移
    uint16_t moveDim2;
    uint8_t gripRatioDim1;  // gripRatioRaw 封顶到 255
    uint8_t gripRatioDim2;
    int16_t maxMinSigOff;  // 覆盖到的格子里 minSigOff 的最大值
    int16_t field338;  // UnitMaxMinInit 置 0x7fff，无人改写
    int16_t minTimeRpt;  // 覆盖到的格子里 timeRpt 的最小值
    uint16_t exceptionRatio[6];  // TZ_GetExceptionRatio：[0] 为二阶矩相对零阶矩的离散度
    uint8_t noiseException;  // 区域异常因全局噪声（0x100）而置位
    uint8_t reserved349[0x2];
    uint8_t fatFingerSD;  // 悬浮状态下的大手指（依赖 SS 与 SD 补偿，本机型恒为 0）
    uint8_t abnormalBaseline;  // 区域判为基线异常（0x200）
    uint8_t reserved34D[0x3];
    uint32_t exceptionType;  // 区域异常字（消抖后）
    uint32_t detectedException;  // 本帧检出的区域异常（消抖前）
    int16_t signChangeCells;  // 遍历时邻域内信号符号变化超过 2 次的格数
    int16_t isolatedCells;  // 邻域里边缘格不到 2 个且信号够强、被单独标记的格数
    uint8_t reserved35C[0x2];
    uint8_t mapTag;  // 本区域在区域图里的编码基数（ZoneMapAccess::tag）
    uint8_t mergedId;  // 覆盖到的上一帧触点 ID（tzId）：第一个
    uint8_t mergedId2;  // 与第一个不同的另一个
    uint8_t reserved361[0x1];
    int16_t minTimeHand;  // 覆盖到的格子里 timeHand 的最小值
    int16_t minGripTime;  // 覆盖到的握持格里 gripTime 的最小值
    int16_t maxNotGripTime;  // 非握持峰所在格 gripTime 的最大值
    int16_t minPeakGripTime;  // 各峰所在格 gripTime 的最小值
    int16_t maxPeakGripTimeOff;  // 各峰所在格 gripTimeOff 的最大值
    uint8_t innerCornerSigRatio;  // 峰在内角格时，边缘格相对它的信号比（×16）
    uint8_t field36D;  // 只有被裁掉的 HIException 写，恒为 0
    uint8_t reserved36E[0x2];
};
static_assert(sizeof(TouchZoneUnit) == 0x370);
static_assert(offsetof(TouchZoneUnit, sigThold) == 0x000);
static_assert(offsetof(TouchZoneUnit, flags) == 0x004);
static_assert(offsetof(TouchZoneUnit, shape) == 0x008);
static_assert(offsetof(TouchZoneUnit, cellCount) == 0x00C);
static_assert(offsetof(TouchZoneUnit, negCellCount) == 0x00E);
static_assert(offsetof(TouchZoneUnit, waterCells) == 0x010);
static_assert(offsetof(TouchZoneUnit, sigSumAll) == 0x014);
static_assert(offsetof(TouchZoneUnit, sigSum) == 0x018);
static_assert(offsetof(TouchZoneUnit, centroidWeightDim1) == 0x01C);
static_assert(offsetof(TouchZoneUnit, centroidWeightDim2) == 0x020);
static_assert(offsetof(TouchZoneUnit, gripRegionSigSum) == 0x024);
static_assert(offsetof(TouchZoneUnit, negSigSum) == 0x028);
static_assert(offsetof(TouchZoneUnit, minSig) == 0x02C);
static_assert(offsetof(TouchZoneUnit, maxSig) == 0x02E);
static_assert(offsetof(TouchZoneUnit, minSigDim1) == 0x030);
static_assert(offsetof(TouchZoneUnit, minSigDim2) == 0x031);
static_assert(offsetof(TouchZoneUnit, avgSigRatio) == 0x032);
static_assert(offsetof(TouchZoneUnit, minDim1) == 0x033);
static_assert(offsetof(TouchZoneUnit, maxDim1) == 0x034);
static_assert(offsetof(TouchZoneUnit, minDim2) == 0x035);
static_assert(offsetof(TouchZoneUnit, maxDim2) == 0x036);
static_assert(offsetof(TouchZoneUnit, edgeMinDim1) == 0x037);
static_assert(offsetof(TouchZoneUnit, edgeMaxDim1) == 0x038);
static_assert(offsetof(TouchZoneUnit, edgeMinDim2) == 0x039);
static_assert(offsetof(TouchZoneUnit, edgeMaxDim2) == 0x03A);
static_assert(offsetof(TouchZoneUnit, nearEdgeMinDim1) == 0x03B);
static_assert(offsetof(TouchZoneUnit, nearEdgeMaxDim1) == 0x03C);
static_assert(offsetof(TouchZoneUnit, nearEdgeMinDim2) == 0x03D);
static_assert(offsetof(TouchZoneUnit, nearEdgeMaxDim2) == 0x03E);
static_assert(offsetof(TouchZoneUnit, edgeMinDim1Row) == 0x03F);
static_assert(offsetof(TouchZoneUnit, edgeMaxDim1Row) == 0x040);
static_assert(offsetof(TouchZoneUnit, edgeMinDim2Col) == 0x041);
static_assert(offsetof(TouchZoneUnit, edgeMaxDim2Col) == 0x042);
static_assert(offsetof(TouchZoneUnit, edgeWidthDim1) == 0x043);
static_assert(offsetof(TouchZoneUnit, edgeWidthDim2) == 0x044);
static_assert(offsetof(TouchZoneUnit, edgeWidthSum) == 0x045);
static_assert(offsetof(TouchZoneUnit, hrMinDim1) == 0x046);
static_assert(offsetof(TouchZoneUnit, hrMaxDim1) == 0x048);
static_assert(offsetof(TouchZoneUnit, hrMinDim2) == 0x04A);
static_assert(offsetof(TouchZoneUnit, hrMaxDim2) == 0x04C);
static_assert(offsetof(TouchZoneUnit, edgeDot) == 0x04E);
static_assert(offsetof(TouchZoneUnit, edgeDotCount) == 0x2A6);
static_assert(offsetof(TouchZoneUnit, circleRate) == 0x2A8);
static_assert(offsetof(TouchZoneUnit, ellipseRate) == 0x2AA);
static_assert(offsetof(TouchZoneUnit, ellipseRateMirror) == 0x2AC);
static_assert(offsetof(TouchZoneUnit, centroidDim1) == 0x2AE);
static_assert(offsetof(TouchZoneUnit, centroidDim2) == 0x2B0);
static_assert(offsetof(TouchZoneUnit, peakCount) == 0x2B2);
static_assert(offsetof(TouchZoneUnit, seedPeakIdx) == 0x2B3);
static_assert(offsetof(TouchZoneUnit, sdcPeakCount) == 0x2B4);
static_assert(offsetof(TouchZoneUnit, sdcMaxPeakIdx) == 0x2B5);
static_assert(offsetof(TouchZoneUnit, maxSigPeakIdx) == 0x2B6);
static_assert(offsetof(TouchZoneUnit, minSigPeakIdx) == 0x2B7);
static_assert(offsetof(TouchZoneUnit, maxZPeakIdx) == 0x2B8);
static_assert(offsetof(TouchZoneUnit, peakIdx) == 0x2B9);
static_assert(offsetof(TouchZoneUnit, centroidMomentDim1) == 0x2C4);
static_assert(offsetof(TouchZoneUnit, centroidMomentDim2) == 0x2C8);
static_assert(offsetof(TouchZoneUnit, edgeSumDim1) == 0x2CC);
static_assert(offsetof(TouchZoneUnit, nearEdgeSumDim1) == 0x2D0);
static_assert(offsetof(TouchZoneUnit, edgeSumDim2) == 0x2D4);
static_assert(offsetof(TouchZoneUnit, nearEdgeSumDim2) == 0x2D8);
static_assert(offsetof(TouchZoneUnit, edgeMaxSigDim1) == 0x2DC);
static_assert(offsetof(TouchZoneUnit, nearEdgeMaxSigDim1) == 0x2DE);
static_assert(offsetof(TouchZoneUnit, edgeMaxSigDim2) == 0x2E0);
static_assert(offsetof(TouchZoneUnit, nearEdgeMaxSigDim2) == 0x2E2);
static_assert(offsetof(TouchZoneUnit, maxCentZ9) == 0x2E4);
static_assert(offsetof(TouchZoneUnit, centToMinPkRatio) == 0x2E8);
static_assert(offsetof(TouchZoneUnit, curMajorMm) == 0x2EA);
static_assert(offsetof(TouchZoneUnit, majorMm) == 0x2EC);
static_assert(offsetof(TouchZoneUnit, minorMm) == 0x2EE);
static_assert(offsetof(TouchZoneUnit, majorAxis) == 0x2F0);
static_assert(offsetof(TouchZoneUnit, minorAxis) == 0x2F2);
static_assert(offsetof(TouchZoneUnit, angle) == 0x2F4);
static_assert(offsetof(TouchZoneUnit, shapeRatio) == 0x2F5);
static_assert(offsetof(TouchZoneUnit, moment0) == 0x2F8);
static_assert(offsetof(TouchZoneUnit, momentDim1) == 0x2FC);
static_assert(offsetof(TouchZoneUnit, momentDim2) == 0x300);
static_assert(offsetof(TouchZoneUnit, momentDim1Dim2) == 0x308);
static_assert(offsetof(TouchZoneUnit, momentDim1Sq) == 0x310);
static_assert(offsetof(TouchZoneUnit, momentDim2Sq) == 0x318);
static_assert(offsetof(TouchZoneUnit, areaToSigRatio) == 0x320);
static_assert(offsetof(TouchZoneUnit, prevIdx) == 0x324);
static_assert(offsetof(TouchZoneUnit, peakId) == 0x325);
static_assert(offsetof(TouchZoneUnit, gripRatioRawDim1) == 0x326);
static_assert(offsetof(TouchZoneUnit, gripRatioRawDim2) == 0x328);
static_assert(offsetof(TouchZoneUnit, gripRegion) == 0x32A);
static_assert(offsetof(TouchZoneUnit, outEdgeSumRatio) == 0x32C);
static_assert(offsetof(TouchZoneUnit, moveDim1) == 0x330);
static_assert(offsetof(TouchZoneUnit, moveDim2) == 0x332);
static_assert(offsetof(TouchZoneUnit, gripRatioDim1) == 0x334);
static_assert(offsetof(TouchZoneUnit, gripRatioDim2) == 0x335);
static_assert(offsetof(TouchZoneUnit, maxMinSigOff) == 0x336);
static_assert(offsetof(TouchZoneUnit, field338) == 0x338);
static_assert(offsetof(TouchZoneUnit, minTimeRpt) == 0x33A);
static_assert(offsetof(TouchZoneUnit, exceptionRatio) == 0x33C);
static_assert(offsetof(TouchZoneUnit, noiseException) == 0x348);
static_assert(offsetof(TouchZoneUnit, fatFingerSD) == 0x34B);
static_assert(offsetof(TouchZoneUnit, abnormalBaseline) == 0x34C);
static_assert(offsetof(TouchZoneUnit, exceptionType) == 0x350);
static_assert(offsetof(TouchZoneUnit, detectedException) == 0x354);
static_assert(offsetof(TouchZoneUnit, signChangeCells) == 0x358);
static_assert(offsetof(TouchZoneUnit, isolatedCells) == 0x35A);
static_assert(offsetof(TouchZoneUnit, mapTag) == 0x35E);
static_assert(offsetof(TouchZoneUnit, mergedId) == 0x35F);
static_assert(offsetof(TouchZoneUnit, mergedId2) == 0x360);
static_assert(offsetof(TouchZoneUnit, minTimeHand) == 0x362);
static_assert(offsetof(TouchZoneUnit, minGripTime) == 0x364);
static_assert(offsetof(TouchZoneUnit, maxNotGripTime) == 0x366);
static_assert(offsetof(TouchZoneUnit, minPeakGripTime) == 0x368);
static_assert(offsetof(TouchZoneUnit, maxPeakGripTimeOff) == 0x36A);
static_assert(offsetof(TouchZoneUnit, innerCornerSigRatio) == 0x36C);
static_assert(offsetof(TouchZoneUnit, field36D) == 0x36D);

/// TouchZoneUnit::flags
inline constexpr uint32_t kZoneOuterEdgePeak = 0x00000001u;     // 有峰在外边缘
inline constexpr uint32_t kZonePrevTouchFlag2 = 0x00000002u;    // 覆盖到的上一帧触点 persistFlags3 带 bit1
inline constexpr uint32_t kZoneCentNoSignal = 0x00000004u;      // 求质心到最小峰信号比时中点附近的 Z9 全为 0
inline constexpr uint32_t kZoneOnBorder = 0x00000020u;          // 外接框贴到屏幕边
inline constexpr uint32_t kZoneVirtualPeak = 0x00000040u;       // 插入过虚拟峰
inline constexpr uint32_t kZoneHtiValid = 0x00000100u;          // 格数超过峰数的 4 倍
inline constexpr uint32_t kZoneMultiFatFinger = 0x00000800u;    // 多指区域其实是一个大手指，不按峰拆分
inline constexpr uint32_t kZoneLargeShape = 0x00001000u;        // 区域或最大子区域过长 / 过宽
inline constexpr uint32_t kZoneTouchesOther = 0x00004000u;      // 邻接已属于别的区域的格子
inline constexpr uint32_t kZoneIsolated = 0x00008000u;          // 两侧都有清晰的信号间隙
inline constexpr uint32_t kZoneBrokenSensor = 0x00010000u;      // TPSensor：区域里有坏传感器
inline constexpr uint32_t kZoneCmfWeak = 0x00020000u;           // 最强峰加共模噪声仍不到峰门限
inline constexpr uint32_t kZoneCmfSideNoise = 0x00040000u;      // 外接框外侧一行（列）的共模噪声足以解释区域信号
inline constexpr uint32_t kZoneNearBorder = 0x00080000u;        // 外接框离屏幕边不到 2 格
inline constexpr uint32_t kZoneEdgeMerged = 0x00100000u;        // 贴边的格子比起点峰更早进入握持
inline constexpr uint32_t kZoneIdMerged = 0x00200000u;          // 覆盖到两个上一帧触点
inline constexpr uint32_t kZonePeakInGrip = 0x00400000u;        // 有峰在握持格上
inline constexpr uint32_t kZonePeakNotInGrip = 0x00800000u;     // 有峰离边较远或握持时间够长，不算握持
inline constexpr uint32_t kZoneGripSeparated = 0x01000000u;     // 握持峰已移到峰列表末尾，SubTZ 按握持阶段拆分
inline constexpr uint32_t kZoneCoversGrip = 0x02000000u;        // 覆盖到握持格
inline constexpr uint32_t kZoneExtraMergeTried = 0x04000000u;   // SubTZ 已试过并入一个握持峰
inline constexpr uint32_t kZoneSeparateRegret = 0x08000000u;    // 握持拆分被撤回
inline constexpr uint32_t kZoneForceSeparate = 0x10000000u;     // 上一帧已拆分，本帧直接拆
inline constexpr uint32_t kZoneInnerNonGripPeak = 0x20000000u;  // 有不在握持格、也不在外边缘的峰
inline constexpr uint32_t kZoneMergedLastReported = 0x40000000u;// 覆盖到上一帧最后报点的触点
inline constexpr uint32_t kZoneMergedFatFinger = 0x80000000u;   // 覆盖到的上一帧触点是大手指

/// TouchZoneUnit::shape
inline constexpr uint32_t kShapeNone = 0;
inline constexpr uint32_t kShapeFinger = 1;
inline constexpr uint32_t kShapeFatFinger = 2;
inline constexpr uint32_t kShapeMultiFinger = 3;
inline constexpr uint32_t kShapeLargeObject = 5;

/// 区域图的访问句柄（g_tzAccess / g_subTZAccess）：TZ 与 SubTZ 各一份，g_ptrAccess 指向正在遍历的那份。
/// 遍历到的格子在 map 里记为 (tag + k) × Dif 符号，k 是 1~7 的格子类别；每完成一个区域 tag 加 8。
struct ZoneMapAccess {
    int8_t* map;
    uint8_t reserved008;
    uint8_t tag;
};
static_assert(sizeof(ZoneMapAccess) == 0x10);
static_assert(offsetof(ZoneMapAccess, tag) == 0x09);

[[nodiscard]] inline ZoneMapAccess& TzAccess() noexcept { return Slot<ZoneMapAccess>(Bss().tzAccess); }
[[nodiscard]] inline ZoneMapAccess& SubTzAccess() noexcept { return Slot<ZoneMapAccess>(Bss().subTZAccess); }
/// g_ptrAccess
[[nodiscard]] inline ZoneMapAccess*& CurrentAccess() noexcept { return Slot<ZoneMapAccess*>(Bss().ptrAccess); }

}  // namespace Solvers::Tsa::Core
