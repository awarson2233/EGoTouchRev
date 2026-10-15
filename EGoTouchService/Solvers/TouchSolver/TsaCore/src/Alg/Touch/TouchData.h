#pragma once
// 触点表（Alg/Touch/TouchData.c、PrevTouchData.c 的数据）。
//
// 当前帧与上一帧各有 grid / hand / side 三张表，g_touchesPtr、g_prevTouchesPtr 指向在用的那一张。
// 两种表的表头布局相同；触点单元的前 kTouchCommonSize 字节布局也相同，帧末整段从当前帧拷进历史表，
// 历史表的单元在此之后还有轨迹队列、速度等只在历史里维护的量。
// 字段只命名已改写的模块用到的；名字以 field 加偏移开头的，含义待改写相关模块时确定。

#include <cstddef>
#include <cstdint>

#include "Common/Image.h"
#include "Common/Types.h"
#include "Extension/Thenar/ThenarData.h"

namespace Solvers::Tsa::Core {

inline constexpr uint8_t kNoTouch = kMaxTouchNum;  // 「没有对应触点」的下标
inline constexpr uint8_t kHistorySize = 30;         // PREVTOUCH_HISTORY_BUFFER_SIZE
inline constexpr size_t kTouchCommonSize = 0x144;   // 当前帧与历史表共有的单元前缀

/// TouchUnit::event（TouchEvent 状态）。
namespace TouchEvent {
inline constexpr uint32_t kTouchDownDebounce = 0x01;
inline constexpr uint32_t kTouchDown = 0x02;
inline constexpr uint32_t kMove = 0x04;
inline constexpr uint32_t kLiftOffDebounce = 0x08;
inline constexpr uint32_t kLiftOffHold = 0x10;
inline constexpr uint32_t kLiftOff = 0x20;
inline constexpr uint32_t kWindowsPadLiftOff = 0x40;
}  // namespace TouchEvent

/// TouchUnit::action（TouchAction）：按点击还是按滑动处理这个触点。
namespace TouchAction {
inline constexpr uint32_t kNone = 0;  // 未定，取 DynamicPrmt::defaultTouchAction
inline constexpr uint32_t kClick = 1;  // 按点击处理：Filter 把位置锁在按下点（点击抑制）
inline constexpr uint32_t kSlide = 2;  // 按滑动处理：不做点击抑制
}  // namespace TouchAction

/// 历史轨迹里的一个点。
struct HistoryEntry {
    SPoint pos;
    uint8_t reserved04[0x4];
    uint64_t time;  // ms
};
static_assert(sizeof(HistoryEntry) == 0x10);

/// 当前帧的一个触点。前 kTouchCommonSize 字节与 PrevTouchUnit 布局相同，帧末整段拷进历史表。
struct TouchUnit {
    uint8_t reserved000[0x8];
    uint64_t timestamp;  // 本帧时间（ms）
    int16_t dt;  // 与上一帧的格式化时间间隔
    int16_t prevDt;
    uint8_t id;  // 触点 ID，0 表示尚未分配
    uint8_t reserved015[0x1];
    uint16_t age;  // 自按下起的帧数
    uint32_t durationMs;  // 自按下消抖开始的毫秒数
    uint16_t field01C;
    Point rawPos;  // 质心（Q8.8 传感器坐标）
    Point pos;  // 边缘补偿后的坐标，后续各模块使用的当前位置
    Point iirPos;  // 自适应 IIR 滤波后坐标
    uint8_t iirRemDim1;  // IIR 的定点余数
    uint8_t iirRemDim2;
    Point jitPos;  // 抗抖动后坐标
    Point rptPos;  // 送去上报的坐标
    int32_t hotZoneShiftDim1;  // HotZoneAdjust 把上报坐标平移的量，报点期间逐帧沿用
    int32_t hotZoneShiftDim2;
    uint8_t field03C;
    uint8_t field03D;
    uint8_t field03E;
    uint8_t field03F;
    Point stablePos;  // 稳定锚点：位移超过阈值时更新
    uint16_t stableMajorMm;
    uint8_t stableReportedCount;
    uint8_t reserved047[0x1];
    uint16_t majorMm;  // 椭圆长轴（mm）
    uint16_t minorMm;  // 椭圆短轴（mm）
    uint16_t majorPx;  // 椭圆长轴（Q8.8 传感器单位）
    uint16_t minorPx;
    uint8_t reserved050[0x2];
    int8_t angle;  // 椭圆倾角
    int8_t maxSigSumAngle;  // 区域信号和最大的那一帧的倾角
    uint8_t sizeMm;
    uint8_t udfpSizeMm;
    uint8_t field056;
    uint8_t field057;
    uint8_t field058;
    uint8_t field059;
    int16_t maxMinSigOff;  // 来源区域里各格 minSigOff 的最大值
    int16_t field05C;
    int16_t field05E;
    int16_t ellipseRate;  // 来源区域的椭圆度（TouchZoneUnit::ellipseRate）
    int16_t smoothEllipseRate;  // (前一帧 smoothEllipseRate·7 + ellipseRate) / 8
    int16_t field064;
    int16_t field066;
    uint16_t field068;
    uint16_t field06A;
    uint8_t dim1Span;  // 区域跨越的列数
    uint8_t dim2Span;  // 区域跨越的行数
    int8_t field06E;
    int8_t field06F;
    uint8_t field070;
    uint8_t field071;
    uint8_t field072;
    uint8_t maxField072;  // field072 在整条轨迹上的最大值
    uint8_t maxField070;
    uint8_t maxField071;
    uint8_t maxField058;
    uint8_t minField058;
    uint8_t field078;
    uint8_t maxField078;
    int16_t signalScale;  // 跨帧锁存的信号刻度（不超过 peakFilterReferenceThreshold）
    int16_t maxRefPeakSig;  // 整条轨迹上 min(全局最大峰值, 参考阈值) 的最大值
    int16_t maxPeakSig;  // 整条轨迹上本触点峰值的最大值
    int32_t maxPeakZn;  // 整条轨迹上本触点峰 Zn 的最大值
    uint32_t field084;
    uint8_t field088;
    uint8_t reserved089[0x3];
    int32_t sigSum;  // 区域正信号和
    int32_t field090;
    int32_t maxSigSum;  // 整条轨迹上 sigSum 的最大值
    int32_t field098;
    uint8_t sigSumRatio;  // sigSum 与上一帧之比 ×32，封顶 255
    uint8_t znRatio;  // 峰 Zn 与上一帧之比 ×32
    uint8_t peakSigRatio;  // 峰值与上一帧之比 ×32
    uint8_t reserved09F[0x1];
    int32_t zoneShape;  // 来源区域类型（3 = 多指拆分）
    uint32_t handEnum;  // 4 / 8：倾角指示的左右手，否则沿用上一帧
    uint32_t field0A8;
    int32_t exceptionType;  // 区域异常类型，0 为正常
    uint32_t volatileFlags;  // 单帧标志
    uint32_t volatileFlags2;  // 单帧标志 2（bit12：本帧对外报点）
    uint32_t persistFlags;  // 跨帧继承的标志（bit3：曾经对外报点）
    uint32_t persistFlags2;
    uint32_t persistFlags3;
    uint32_t edgeFlags;  // 峰所在的内 / 外边缘
    uint32_t suppressFlags;
    uint8_t reserved0CC[0x4];
    int32_t gripStage;
    uint32_t gripFlags;
    uint32_t persistFlags4;
    uint32_t event;  // TouchEvent
    uint32_t quadrant;  // 显示坐标所在象限 0..3
    uint32_t action;  // TouchAction
    uint16_t maxMajorMm;
    uint16_t curMajorMm;
    uint16_t edgeDistDim1;  // 到 Dim1 最近逻辑边缘的距离
    uint16_t edgeDistDim2;
    uint16_t minEdgeDistDim1;  // 整条轨迹上的最小值
    uint16_t minEdgeDistDim2;
    uint16_t maxEdgeDistDim1;
    uint16_t maxEdgeDistDim2;
    uint16_t field0F8;
    uint16_t trustFrames;  // 报点且准入上报后的累计帧数
    uint8_t peakIdx;  // 对应的峰
    uint8_t sdcPeakIdx;  // 来源区域里切换 Dif 前峰值最大的峰（g_peaksBeforeSwitchDifBuffer 下标）
    uint16_t field0FE;
    uint16_t sqDistMmFromPrev;  // 与上一帧位置的距离平方（mm²）
    uint16_t sqAccMmFromPrev;  // 位移减去上一帧位移后的平方（mm²）
    uint32_t sqDistPxFromPrev;
    uint32_t maxSqDistPxFromPrev;
    uint16_t sqDistMmFromStart;
    uint16_t maxSqDistMmFromStart;
    uint16_t rptSqDistMmFromFirstRpt;
    uint16_t rptSqDistMmFromPrev;
    uint32_t rptSqDistDisplayPxFromPrev;
    uint32_t fltSqDistPxFromPrev;
    uint8_t shapeIirValid;
    uint8_t reserved11D[0x1];
    uint16_t iirMajorMm;  // 形状参数的 7/8 IIR
    uint16_t iirMinorMm;
    uint16_t iirMajorPx;
    uint16_t iirMinorPx;
    int8_t iirAngle;
    uint8_t iirAxisRatioX16;  // iirMajorPx·16 / iirMinorPx
    int16_t iirField066;
    uint16_t iirField068;
    uint16_t iirField06A;
    uint8_t reserved12E[0x2];
    int32_t zoneEdgeSumDim1;  // 来源区域在 Dim1 两条逻辑边上的信号和
    int32_t zoneEdgeSumDim2;
    uint8_t reserved138[0x1];
    uint8_t curveTracking;  // Filter：轨迹已稳定移动，开始识别折返
    uint8_t reserved13A[0x2];
    uint32_t jitRadius;  // Filter：本帧的抗抖动半径（Q8.8）
    uint8_t curveReturnFrames;  // Filter：识别到折返后的帧数（到 5 清零）
    uint8_t reserved141[0x1];
    uint8_t liftOffJitter;  // Filter：本帧判为抬起中的信号骤降
    uint8_t liftOffJitterCount;  // 抬起骤降的累计次数（到 11 为止）
    uint8_t reserved144[0x1];
    uint8_t insideDim1;  // 质心所在格（不在区域核心时为 dim1Len / dim2Len）
    uint8_t insideDim2;
    uint8_t prevIdx;  // 匹配到的历史触点下标，kNoTouch 表示新按下
    uint16_t waterCells;  // 来源区域的水膜格数
    uint16_t field14A;
    uint8_t zoneIdx;  // 来源区域
    uint8_t reserved14D[0x3];
    uint32_t field150;
    uint8_t reserved154[0xA];
    int16_t zoneSignalScale;  // 来源区域的信号刻度，锁存进 signalScale
    uint32_t trustTag;  // 1 / 2：本帧确认的可信点类别
    uint8_t field164;
    uint8_t reserved165[0x3];
};
static_assert(sizeof(TouchUnit) == 0x168);
static_assert(offsetof(TouchUnit, timestamp) == 0x008);
static_assert(offsetof(TouchUnit, dt) == 0x010);
static_assert(offsetof(TouchUnit, prevDt) == 0x012);
static_assert(offsetof(TouchUnit, id) == 0x014);
static_assert(offsetof(TouchUnit, age) == 0x016);
static_assert(offsetof(TouchUnit, durationMs) == 0x018);
static_assert(offsetof(TouchUnit, field01C) == 0x01C);
static_assert(offsetof(TouchUnit, rawPos) == 0x01E);
static_assert(offsetof(TouchUnit, pos) == 0x022);
static_assert(offsetof(TouchUnit, iirPos) == 0x026);
static_assert(offsetof(TouchUnit, iirRemDim1) == 0x02A);
static_assert(offsetof(TouchUnit, iirRemDim2) == 0x02B);
static_assert(offsetof(TouchUnit, jitPos) == 0x02C);
static_assert(offsetof(TouchUnit, rptPos) == 0x030);
static_assert(offsetof(TouchUnit, hotZoneShiftDim1) == 0x034);
static_assert(offsetof(TouchUnit, hotZoneShiftDim2) == 0x038);
static_assert(offsetof(TouchUnit, field03C) == 0x03C);
static_assert(offsetof(TouchUnit, field03D) == 0x03D);
static_assert(offsetof(TouchUnit, field03E) == 0x03E);
static_assert(offsetof(TouchUnit, field03F) == 0x03F);
static_assert(offsetof(TouchUnit, stablePos) == 0x040);
static_assert(offsetof(TouchUnit, stableMajorMm) == 0x044);
static_assert(offsetof(TouchUnit, stableReportedCount) == 0x046);
static_assert(offsetof(TouchUnit, majorMm) == 0x048);
static_assert(offsetof(TouchUnit, minorMm) == 0x04A);
static_assert(offsetof(TouchUnit, majorPx) == 0x04C);
static_assert(offsetof(TouchUnit, minorPx) == 0x04E);
static_assert(offsetof(TouchUnit, angle) == 0x052);
static_assert(offsetof(TouchUnit, maxSigSumAngle) == 0x053);
static_assert(offsetof(TouchUnit, sizeMm) == 0x054);
static_assert(offsetof(TouchUnit, udfpSizeMm) == 0x055);
static_assert(offsetof(TouchUnit, field056) == 0x056);
static_assert(offsetof(TouchUnit, field057) == 0x057);
static_assert(offsetof(TouchUnit, field058) == 0x058);
static_assert(offsetof(TouchUnit, field059) == 0x059);
static_assert(offsetof(TouchUnit, maxMinSigOff) == 0x05A);
static_assert(offsetof(TouchUnit, field05C) == 0x05C);
static_assert(offsetof(TouchUnit, field05E) == 0x05E);
static_assert(offsetof(TouchUnit, ellipseRate) == 0x060);
static_assert(offsetof(TouchUnit, smoothEllipseRate) == 0x062);
static_assert(offsetof(TouchUnit, field064) == 0x064);
static_assert(offsetof(TouchUnit, field066) == 0x066);
static_assert(offsetof(TouchUnit, field068) == 0x068);
static_assert(offsetof(TouchUnit, field06A) == 0x06A);
static_assert(offsetof(TouchUnit, dim1Span) == 0x06C);
static_assert(offsetof(TouchUnit, dim2Span) == 0x06D);
static_assert(offsetof(TouchUnit, field06E) == 0x06E);
static_assert(offsetof(TouchUnit, field06F) == 0x06F);
static_assert(offsetof(TouchUnit, field070) == 0x070);
static_assert(offsetof(TouchUnit, field071) == 0x071);
static_assert(offsetof(TouchUnit, field072) == 0x072);
static_assert(offsetof(TouchUnit, maxField072) == 0x073);
static_assert(offsetof(TouchUnit, maxField070) == 0x074);
static_assert(offsetof(TouchUnit, maxField071) == 0x075);
static_assert(offsetof(TouchUnit, maxField058) == 0x076);
static_assert(offsetof(TouchUnit, minField058) == 0x077);
static_assert(offsetof(TouchUnit, field078) == 0x078);
static_assert(offsetof(TouchUnit, maxField078) == 0x079);
static_assert(offsetof(TouchUnit, signalScale) == 0x07A);
static_assert(offsetof(TouchUnit, maxRefPeakSig) == 0x07C);
static_assert(offsetof(TouchUnit, maxPeakSig) == 0x07E);
static_assert(offsetof(TouchUnit, maxPeakZn) == 0x080);
static_assert(offsetof(TouchUnit, field084) == 0x084);
static_assert(offsetof(TouchUnit, field088) == 0x088);
static_assert(offsetof(TouchUnit, sigSum) == 0x08C);
static_assert(offsetof(TouchUnit, field090) == 0x090);
static_assert(offsetof(TouchUnit, maxSigSum) == 0x094);
static_assert(offsetof(TouchUnit, field098) == 0x098);
static_assert(offsetof(TouchUnit, sigSumRatio) == 0x09C);
static_assert(offsetof(TouchUnit, znRatio) == 0x09D);
static_assert(offsetof(TouchUnit, peakSigRatio) == 0x09E);
static_assert(offsetof(TouchUnit, zoneShape) == 0x0A0);
static_assert(offsetof(TouchUnit, handEnum) == 0x0A4);
static_assert(offsetof(TouchUnit, field0A8) == 0x0A8);
static_assert(offsetof(TouchUnit, exceptionType) == 0x0AC);
static_assert(offsetof(TouchUnit, volatileFlags) == 0x0B0);
static_assert(offsetof(TouchUnit, volatileFlags2) == 0x0B4);
static_assert(offsetof(TouchUnit, persistFlags) == 0x0B8);
static_assert(offsetof(TouchUnit, persistFlags2) == 0x0BC);
static_assert(offsetof(TouchUnit, persistFlags3) == 0x0C0);
static_assert(offsetof(TouchUnit, edgeFlags) == 0x0C4);
static_assert(offsetof(TouchUnit, suppressFlags) == 0x0C8);
static_assert(offsetof(TouchUnit, gripStage) == 0x0D0);
static_assert(offsetof(TouchUnit, gripFlags) == 0x0D4);
static_assert(offsetof(TouchUnit, persistFlags4) == 0x0D8);
static_assert(offsetof(TouchUnit, event) == 0x0DC);
static_assert(offsetof(TouchUnit, quadrant) == 0x0E0);
static_assert(offsetof(TouchUnit, action) == 0x0E4);
static_assert(offsetof(TouchUnit, maxMajorMm) == 0x0E8);
static_assert(offsetof(TouchUnit, curMajorMm) == 0x0EA);
static_assert(offsetof(TouchUnit, edgeDistDim1) == 0x0EC);
static_assert(offsetof(TouchUnit, edgeDistDim2) == 0x0EE);
static_assert(offsetof(TouchUnit, minEdgeDistDim1) == 0x0F0);
static_assert(offsetof(TouchUnit, minEdgeDistDim2) == 0x0F2);
static_assert(offsetof(TouchUnit, maxEdgeDistDim1) == 0x0F4);
static_assert(offsetof(TouchUnit, maxEdgeDistDim2) == 0x0F6);
static_assert(offsetof(TouchUnit, field0F8) == 0x0F8);
static_assert(offsetof(TouchUnit, trustFrames) == 0x0FA);
static_assert(offsetof(TouchUnit, peakIdx) == 0x0FC);
static_assert(offsetof(TouchUnit, sdcPeakIdx) == 0x0FD);
static_assert(offsetof(TouchUnit, field0FE) == 0x0FE);
static_assert(offsetof(TouchUnit, sqDistMmFromPrev) == 0x100);
static_assert(offsetof(TouchUnit, sqAccMmFromPrev) == 0x102);
static_assert(offsetof(TouchUnit, sqDistPxFromPrev) == 0x104);
static_assert(offsetof(TouchUnit, maxSqDistPxFromPrev) == 0x108);
static_assert(offsetof(TouchUnit, sqDistMmFromStart) == 0x10C);
static_assert(offsetof(TouchUnit, maxSqDistMmFromStart) == 0x10E);
static_assert(offsetof(TouchUnit, rptSqDistMmFromFirstRpt) == 0x110);
static_assert(offsetof(TouchUnit, rptSqDistMmFromPrev) == 0x112);
static_assert(offsetof(TouchUnit, rptSqDistDisplayPxFromPrev) == 0x114);
static_assert(offsetof(TouchUnit, fltSqDistPxFromPrev) == 0x118);
static_assert(offsetof(TouchUnit, shapeIirValid) == 0x11C);
static_assert(offsetof(TouchUnit, iirMajorMm) == 0x11E);
static_assert(offsetof(TouchUnit, iirMinorMm) == 0x120);
static_assert(offsetof(TouchUnit, iirMajorPx) == 0x122);
static_assert(offsetof(TouchUnit, iirMinorPx) == 0x124);
static_assert(offsetof(TouchUnit, iirAngle) == 0x126);
static_assert(offsetof(TouchUnit, iirAxisRatioX16) == 0x127);
static_assert(offsetof(TouchUnit, iirField066) == 0x128);
static_assert(offsetof(TouchUnit, iirField068) == 0x12A);
static_assert(offsetof(TouchUnit, iirField06A) == 0x12C);
static_assert(offsetof(TouchUnit, zoneEdgeSumDim1) == 0x130);
static_assert(offsetof(TouchUnit, zoneEdgeSumDim2) == 0x134);
static_assert(offsetof(TouchUnit, curveTracking) == 0x139);
static_assert(offsetof(TouchUnit, jitRadius) == 0x13C);
static_assert(offsetof(TouchUnit, curveReturnFrames) == 0x140);
static_assert(offsetof(TouchUnit, liftOffJitter) == 0x142);
static_assert(offsetof(TouchUnit, liftOffJitterCount) == 0x143);
static_assert(offsetof(TouchUnit, insideDim1) == 0x145);
static_assert(offsetof(TouchUnit, insideDim2) == 0x146);
static_assert(offsetof(TouchUnit, prevIdx) == 0x147);
static_assert(offsetof(TouchUnit, waterCells) == 0x148);
static_assert(offsetof(TouchUnit, field14A) == 0x14A);
static_assert(offsetof(TouchUnit, zoneIdx) == 0x14C);
static_assert(offsetof(TouchUnit, field150) == 0x150);
static_assert(offsetof(TouchUnit, zoneSignalScale) == 0x15E);
static_assert(offsetof(TouchUnit, trustTag) == 0x160);
static_assert(offsetof(TouchUnit, field164) == 0x164);

/// 历史表里的一个触点：当前帧触点的公共部分，加上只在历史里维护的轨迹与运动量。
struct PrevTouchUnit {
    uint8_t reserved000[0x8];
    uint64_t timestamp;  // 本帧时间（ms）
    int16_t dt;  // 与上一帧的格式化时间间隔
    int16_t prevDt;
    uint8_t id;  // 触点 ID，0 表示尚未分配
    uint8_t reserved015[0x1];
    uint16_t age;  // 自按下起的帧数
    uint32_t durationMs;  // 自按下消抖开始的毫秒数
    uint16_t field01C;
    Point rawPos;  // 质心（Q8.8 传感器坐标）
    Point pos;  // 边缘补偿后的坐标，后续各模块使用的当前位置
    Point iirPos;  // 自适应 IIR 滤波后坐标
    uint8_t iirRemDim1;  // IIR 的定点余数
    uint8_t iirRemDim2;
    Point jitPos;  // 抗抖动后坐标
    Point rptPos;  // 送去上报的坐标
    int32_t hotZoneShiftDim1;  // HotZoneAdjust 把上报坐标平移的量，报点期间逐帧沿用
    int32_t hotZoneShiftDim2;
    uint8_t field03C;
    uint8_t field03D;
    uint8_t field03E;
    uint8_t field03F;
    Point stablePos;  // 稳定锚点：位移超过阈值时更新
    uint16_t stableMajorMm;
    uint8_t stableReportedCount;
    uint8_t reserved047[0x1];
    uint16_t majorMm;  // 椭圆长轴（mm）
    uint16_t minorMm;  // 椭圆短轴（mm）
    uint16_t majorPx;  // 椭圆长轴（Q8.8 传感器单位）
    uint16_t minorPx;
    uint8_t reserved050[0x2];
    int8_t angle;  // 椭圆倾角
    int8_t maxSigSumAngle;  // 区域信号和最大的那一帧的倾角
    uint8_t sizeMm;
    uint8_t udfpSizeMm;
    uint8_t field056;
    uint8_t field057;
    uint8_t field058;
    uint8_t field059;
    int16_t maxMinSigOff;  // 来源区域里各格 minSigOff 的最大值
    int16_t field05C;
    int16_t field05E;
    int16_t ellipseRate;  // 来源区域的椭圆度（TouchZoneUnit::ellipseRate）
    int16_t smoothEllipseRate;  // (前一帧 smoothEllipseRate·7 + ellipseRate) / 8
    int16_t field064;
    int16_t field066;
    uint16_t field068;
    uint16_t field06A;
    uint8_t dim1Span;  // 区域跨越的列数
    uint8_t dim2Span;  // 区域跨越的行数
    int8_t field06E;
    int8_t field06F;
    uint8_t field070;
    uint8_t field071;
    uint8_t field072;
    uint8_t maxField072;  // field072 在整条轨迹上的最大值
    uint8_t maxField070;
    uint8_t maxField071;
    uint8_t maxField058;
    uint8_t minField058;
    uint8_t field078;
    uint8_t maxField078;
    int16_t signalScale;  // 跨帧锁存的信号刻度（不超过 peakFilterReferenceThreshold）
    int16_t maxRefPeakSig;  // 整条轨迹上 min(全局最大峰值, 参考阈值) 的最大值
    int16_t maxPeakSig;  // 整条轨迹上本触点峰值的最大值
    int32_t maxPeakZn;  // 整条轨迹上本触点峰 Zn 的最大值
    uint32_t field084;
    uint8_t field088;
    uint8_t reserved089[0x3];
    int32_t sigSum;  // 区域正信号和
    int32_t field090;
    int32_t maxSigSum;  // 整条轨迹上 sigSum 的最大值
    int32_t field098;
    uint8_t sigSumRatio;  // sigSum 与上一帧之比 ×32，封顶 255
    uint8_t znRatio;  // 峰 Zn 与上一帧之比 ×32
    uint8_t peakSigRatio;  // 峰值与上一帧之比 ×32
    uint8_t reserved09F[0x1];
    int32_t zoneShape;  // 来源区域类型（3 = 多指拆分）
    uint32_t handEnum;  // 4 / 8：倾角指示的左右手，否则沿用上一帧
    uint32_t field0A8;
    int32_t exceptionType;  // 区域异常类型，0 为正常
    uint32_t volatileFlags;  // 单帧标志
    uint32_t volatileFlags2;  // 单帧标志 2（bit12：本帧对外报点）
    uint32_t persistFlags;  // 跨帧继承的标志（bit3：曾经对外报点）
    uint32_t persistFlags2;
    uint32_t persistFlags3;
    uint32_t edgeFlags;  // 峰所在的内 / 外边缘
    uint32_t suppressFlags;
    uint8_t reserved0CC[0x4];
    int32_t gripStage;
    uint32_t gripFlags;
    uint32_t persistFlags4;
    uint32_t event;  // TouchEvent
    uint32_t quadrant;  // 显示坐标所在象限 0..3
    uint32_t action;  // TouchAction
    uint16_t maxMajorMm;
    uint16_t curMajorMm;
    uint16_t edgeDistDim1;  // 到 Dim1 最近逻辑边缘的距离
    uint16_t edgeDistDim2;
    uint16_t minEdgeDistDim1;  // 整条轨迹上的最小值
    uint16_t minEdgeDistDim2;
    uint16_t maxEdgeDistDim1;
    uint16_t maxEdgeDistDim2;
    uint16_t field0F8;
    uint16_t trustFrames;  // 报点且准入上报后的累计帧数
    uint8_t peakIdx;  // 对应的峰
    uint8_t sdcPeakIdx;  // 来源区域里切换 Dif 前峰值最大的峰（g_peaksBeforeSwitchDifBuffer 下标）
    uint16_t field0FE;
    uint16_t sqDistMmFromPrev;  // 与上一帧位置的距离平方（mm²）
    uint16_t sqAccMmFromPrev;  // 位移减去上一帧位移后的平方（mm²）
    uint32_t sqDistPxFromPrev;
    uint32_t maxSqDistPxFromPrev;
    uint16_t sqDistMmFromStart;
    uint16_t maxSqDistMmFromStart;
    uint16_t rptSqDistMmFromFirstRpt;
    uint16_t rptSqDistMmFromPrev;
    uint32_t rptSqDistDisplayPxFromPrev;
    uint32_t fltSqDistPxFromPrev;
    uint8_t shapeIirValid;
    uint8_t reserved11D[0x1];
    uint16_t iirMajorMm;  // 形状参数的 7/8 IIR
    uint16_t iirMinorMm;
    uint16_t iirMajorPx;
    uint16_t iirMinorPx;
    int8_t iirAngle;
    uint8_t iirAxisRatioX16;  // iirMajorPx·16 / iirMinorPx
    int16_t iirField066;
    uint16_t iirField068;
    uint16_t iirField06A;
    uint8_t reserved12E[0x2];
    int32_t zoneEdgeSumDim1;  // 来源区域在 Dim1 两条逻辑边上的信号和
    int32_t zoneEdgeSumDim2;
    uint8_t reserved138[0x1];
    uint8_t curveTracking;  // Filter：轨迹已稳定移动，开始识别折返
    uint8_t reserved13A[0x2];
    uint32_t jitRadius;  // Filter：本帧的抗抖动半径（Q8.8）
    uint8_t curveReturnFrames;  // Filter：识别到折返后的帧数（到 5 清零）
    uint8_t reserved141[0x1];
    uint8_t liftOffJitter;  // Filter：本帧判为抬起中的信号骤降
    uint8_t liftOffJitterCount;  // 抬起骤降的累计次数（到 11 为止）
    uint8_t reserved144[0x4];
    uint8_t historyCount;
    uint8_t reserved149[0x7];
    HistoryEntry history[kHistorySize];
    uint8_t reserved330[0x4];
    int8_t queueHead;  // 16 格网格轨迹队列
    int8_t queueTail;
    uint8_t queueDim1[16];
    uint8_t queueDim2[16];
    Point startPos;  // 按下消抖开始时的位置
    Point firstRptPos;  // 第一次对外报点的位置
    uint16_t field35E;
    uint32_t field360;
    Point falseFlickPos;
    uint8_t reserved368[0x4];
    SPoint falseFlickOffset;
    Point tdDebouncePos;
    uint8_t reserved374[0x4];
    uint64_t tdDebounceTime;  // 按下消抖最近一次开始的时间（ms）
    uint64_t field380;
    SPoint rawDelta;  // 质心的帧间位移
    SPoint rawVelocity;  // 位移 / dt
    SPoint rawAccel;  // 速度的帧间变化
    SPoint posDelta;  // 当前位置的帧间位移
    SPoint posDelta2;  // 位移的帧间变化
    int32_t posDeltaSumDim1;
    int32_t posDeltaSumDim2;
    uint16_t debounceFrames;  // 按下 / 抬起消抖已累计的帧数
    uint8_t field3A6;
    uint8_t field3A7;
    uint16_t betaGripFrames;
    uint8_t reserved3AA[0x2];
    uint8_t blTrustFrames;  // BLReset：未贴边、标记 0x20000000 的小触点持续可信的帧数（0..15）
    uint8_t reserved3AD[0x1];
    SPoint clickToFlickOffset;
    uint8_t curIdx;  // 匹配到的当前帧触点下标，kNoTouch 表示已丢失
    uint8_t postReportIirCoef;
    uint8_t prevIirCoef;
    uint8_t reserved3B5[0x1];
    uint16_t maxSqDistMmFromPrev;
    uint8_t reserved3B8[0x1C];
    Point field3D4;
    uint16_t minField068;
    uint16_t minField06A;
    uint16_t maxField068;
    uint16_t maxField06A;
    uint64_t strongZnTime;  // 本帧峰 Zn 不低于轨迹最大值 70% 时记下的时间（ms）
    uint8_t reserved3E8[0x64];
    int8_t field44C;
    int8_t field44D;
    uint8_t field44E;
    uint8_t reserved44F[0x1];
    SPoint field450;
    Point field454;
    uint64_t field458;
    uint16_t field460;
    uint16_t field462;
    uint8_t field464;
    uint8_t field465;
    int16_t minSigOffAtTouchDown;  // 按下那一格的 g_tsaBufMinSigOff
    int16_t timeOffAtTouchDown;  // 按下那一格的 g_tsaBufTimeOff
    int16_t timeRptAtTouchDown;  // 按下那一格的 g_tsaBufTimeRpt
    uint8_t predictAlongDim1;  // Filter：折返沿 Dim1（否则沿 Dim2）
    uint8_t predictFrames;  // 折返后已外推的帧数，0 为未在外推
    uint8_t predictTurnFrames;  // 按速度算出的转向帧数（到 12 为止）
    uint8_t predictForwardFrames;  // 其中继续前冲的帧数（到 2 为止）
    SPoint predictPos;  // 外推坐标
    uint8_t reserved474[0x4];
};
static_assert(sizeof(PrevTouchUnit) == 0x478);
static_assert(offsetof(PrevTouchUnit, timestamp) == 0x008);
static_assert(offsetof(PrevTouchUnit, dt) == 0x010);
static_assert(offsetof(PrevTouchUnit, prevDt) == 0x012);
static_assert(offsetof(PrevTouchUnit, id) == 0x014);
static_assert(offsetof(PrevTouchUnit, age) == 0x016);
static_assert(offsetof(PrevTouchUnit, durationMs) == 0x018);
static_assert(offsetof(PrevTouchUnit, field01C) == 0x01C);
static_assert(offsetof(PrevTouchUnit, rawPos) == 0x01E);
static_assert(offsetof(PrevTouchUnit, pos) == 0x022);
static_assert(offsetof(PrevTouchUnit, iirPos) == 0x026);
static_assert(offsetof(PrevTouchUnit, iirRemDim1) == 0x02A);
static_assert(offsetof(PrevTouchUnit, iirRemDim2) == 0x02B);
static_assert(offsetof(PrevTouchUnit, jitPos) == 0x02C);
static_assert(offsetof(PrevTouchUnit, rptPos) == 0x030);
static_assert(offsetof(PrevTouchUnit, hotZoneShiftDim1) == 0x034);
static_assert(offsetof(PrevTouchUnit, hotZoneShiftDim2) == 0x038);
static_assert(offsetof(PrevTouchUnit, field03C) == 0x03C);
static_assert(offsetof(PrevTouchUnit, field03D) == 0x03D);
static_assert(offsetof(PrevTouchUnit, field03E) == 0x03E);
static_assert(offsetof(PrevTouchUnit, field03F) == 0x03F);
static_assert(offsetof(PrevTouchUnit, stablePos) == 0x040);
static_assert(offsetof(PrevTouchUnit, stableMajorMm) == 0x044);
static_assert(offsetof(PrevTouchUnit, stableReportedCount) == 0x046);
static_assert(offsetof(PrevTouchUnit, majorMm) == 0x048);
static_assert(offsetof(PrevTouchUnit, minorMm) == 0x04A);
static_assert(offsetof(PrevTouchUnit, majorPx) == 0x04C);
static_assert(offsetof(PrevTouchUnit, minorPx) == 0x04E);
static_assert(offsetof(PrevTouchUnit, angle) == 0x052);
static_assert(offsetof(PrevTouchUnit, maxSigSumAngle) == 0x053);
static_assert(offsetof(PrevTouchUnit, sizeMm) == 0x054);
static_assert(offsetof(PrevTouchUnit, udfpSizeMm) == 0x055);
static_assert(offsetof(PrevTouchUnit, field056) == 0x056);
static_assert(offsetof(PrevTouchUnit, field057) == 0x057);
static_assert(offsetof(PrevTouchUnit, field058) == 0x058);
static_assert(offsetof(PrevTouchUnit, field059) == 0x059);
static_assert(offsetof(PrevTouchUnit, maxMinSigOff) == 0x05A);
static_assert(offsetof(PrevTouchUnit, field05C) == 0x05C);
static_assert(offsetof(PrevTouchUnit, field05E) == 0x05E);
static_assert(offsetof(PrevTouchUnit, ellipseRate) == 0x060);
static_assert(offsetof(PrevTouchUnit, smoothEllipseRate) == 0x062);
static_assert(offsetof(PrevTouchUnit, field064) == 0x064);
static_assert(offsetof(PrevTouchUnit, field066) == 0x066);
static_assert(offsetof(PrevTouchUnit, field068) == 0x068);
static_assert(offsetof(PrevTouchUnit, field06A) == 0x06A);
static_assert(offsetof(PrevTouchUnit, dim1Span) == 0x06C);
static_assert(offsetof(PrevTouchUnit, dim2Span) == 0x06D);
static_assert(offsetof(PrevTouchUnit, field06E) == 0x06E);
static_assert(offsetof(PrevTouchUnit, field06F) == 0x06F);
static_assert(offsetof(PrevTouchUnit, field070) == 0x070);
static_assert(offsetof(PrevTouchUnit, field071) == 0x071);
static_assert(offsetof(PrevTouchUnit, field072) == 0x072);
static_assert(offsetof(PrevTouchUnit, maxField072) == 0x073);
static_assert(offsetof(PrevTouchUnit, maxField070) == 0x074);
static_assert(offsetof(PrevTouchUnit, maxField071) == 0x075);
static_assert(offsetof(PrevTouchUnit, maxField058) == 0x076);
static_assert(offsetof(PrevTouchUnit, minField058) == 0x077);
static_assert(offsetof(PrevTouchUnit, field078) == 0x078);
static_assert(offsetof(PrevTouchUnit, maxField078) == 0x079);
static_assert(offsetof(PrevTouchUnit, signalScale) == 0x07A);
static_assert(offsetof(PrevTouchUnit, maxRefPeakSig) == 0x07C);
static_assert(offsetof(PrevTouchUnit, maxPeakSig) == 0x07E);
static_assert(offsetof(PrevTouchUnit, maxPeakZn) == 0x080);
static_assert(offsetof(PrevTouchUnit, field084) == 0x084);
static_assert(offsetof(PrevTouchUnit, field088) == 0x088);
static_assert(offsetof(PrevTouchUnit, sigSum) == 0x08C);
static_assert(offsetof(PrevTouchUnit, field090) == 0x090);
static_assert(offsetof(PrevTouchUnit, maxSigSum) == 0x094);
static_assert(offsetof(PrevTouchUnit, field098) == 0x098);
static_assert(offsetof(PrevTouchUnit, sigSumRatio) == 0x09C);
static_assert(offsetof(PrevTouchUnit, znRatio) == 0x09D);
static_assert(offsetof(PrevTouchUnit, peakSigRatio) == 0x09E);
static_assert(offsetof(PrevTouchUnit, zoneShape) == 0x0A0);
static_assert(offsetof(PrevTouchUnit, handEnum) == 0x0A4);
static_assert(offsetof(PrevTouchUnit, field0A8) == 0x0A8);
static_assert(offsetof(PrevTouchUnit, exceptionType) == 0x0AC);
static_assert(offsetof(PrevTouchUnit, volatileFlags) == 0x0B0);
static_assert(offsetof(PrevTouchUnit, volatileFlags2) == 0x0B4);
static_assert(offsetof(PrevTouchUnit, persistFlags) == 0x0B8);
static_assert(offsetof(PrevTouchUnit, persistFlags2) == 0x0BC);
static_assert(offsetof(PrevTouchUnit, persistFlags3) == 0x0C0);
static_assert(offsetof(PrevTouchUnit, edgeFlags) == 0x0C4);
static_assert(offsetof(PrevTouchUnit, suppressFlags) == 0x0C8);
static_assert(offsetof(PrevTouchUnit, gripStage) == 0x0D0);
static_assert(offsetof(PrevTouchUnit, gripFlags) == 0x0D4);
static_assert(offsetof(PrevTouchUnit, persistFlags4) == 0x0D8);
static_assert(offsetof(PrevTouchUnit, event) == 0x0DC);
static_assert(offsetof(PrevTouchUnit, quadrant) == 0x0E0);
static_assert(offsetof(PrevTouchUnit, action) == 0x0E4);
static_assert(offsetof(PrevTouchUnit, maxMajorMm) == 0x0E8);
static_assert(offsetof(PrevTouchUnit, curMajorMm) == 0x0EA);
static_assert(offsetof(PrevTouchUnit, edgeDistDim1) == 0x0EC);
static_assert(offsetof(PrevTouchUnit, edgeDistDim2) == 0x0EE);
static_assert(offsetof(PrevTouchUnit, minEdgeDistDim1) == 0x0F0);
static_assert(offsetof(PrevTouchUnit, minEdgeDistDim2) == 0x0F2);
static_assert(offsetof(PrevTouchUnit, maxEdgeDistDim1) == 0x0F4);
static_assert(offsetof(PrevTouchUnit, maxEdgeDistDim2) == 0x0F6);
static_assert(offsetof(PrevTouchUnit, field0F8) == 0x0F8);
static_assert(offsetof(PrevTouchUnit, trustFrames) == 0x0FA);
static_assert(offsetof(PrevTouchUnit, peakIdx) == 0x0FC);
static_assert(offsetof(PrevTouchUnit, sdcPeakIdx) == 0x0FD);
static_assert(offsetof(PrevTouchUnit, field0FE) == 0x0FE);
static_assert(offsetof(PrevTouchUnit, sqDistMmFromPrev) == 0x100);
static_assert(offsetof(PrevTouchUnit, sqAccMmFromPrev) == 0x102);
static_assert(offsetof(PrevTouchUnit, sqDistPxFromPrev) == 0x104);
static_assert(offsetof(PrevTouchUnit, maxSqDistPxFromPrev) == 0x108);
static_assert(offsetof(PrevTouchUnit, sqDistMmFromStart) == 0x10C);
static_assert(offsetof(PrevTouchUnit, maxSqDistMmFromStart) == 0x10E);
static_assert(offsetof(PrevTouchUnit, rptSqDistMmFromFirstRpt) == 0x110);
static_assert(offsetof(PrevTouchUnit, rptSqDistMmFromPrev) == 0x112);
static_assert(offsetof(PrevTouchUnit, rptSqDistDisplayPxFromPrev) == 0x114);
static_assert(offsetof(PrevTouchUnit, fltSqDistPxFromPrev) == 0x118);
static_assert(offsetof(PrevTouchUnit, shapeIirValid) == 0x11C);
static_assert(offsetof(PrevTouchUnit, iirMajorMm) == 0x11E);
static_assert(offsetof(PrevTouchUnit, iirMinorMm) == 0x120);
static_assert(offsetof(PrevTouchUnit, iirMajorPx) == 0x122);
static_assert(offsetof(PrevTouchUnit, iirMinorPx) == 0x124);
static_assert(offsetof(PrevTouchUnit, iirAngle) == 0x126);
static_assert(offsetof(PrevTouchUnit, iirAxisRatioX16) == 0x127);
static_assert(offsetof(PrevTouchUnit, iirField066) == 0x128);
static_assert(offsetof(PrevTouchUnit, iirField068) == 0x12A);
static_assert(offsetof(PrevTouchUnit, iirField06A) == 0x12C);
static_assert(offsetof(PrevTouchUnit, zoneEdgeSumDim1) == 0x130);
static_assert(offsetof(PrevTouchUnit, zoneEdgeSumDim2) == 0x134);
static_assert(offsetof(PrevTouchUnit, curveTracking) == 0x139);
static_assert(offsetof(PrevTouchUnit, jitRadius) == 0x13C);
static_assert(offsetof(PrevTouchUnit, curveReturnFrames) == 0x140);
static_assert(offsetof(PrevTouchUnit, liftOffJitter) == 0x142);
static_assert(offsetof(PrevTouchUnit, liftOffJitterCount) == 0x143);
static_assert(offsetof(PrevTouchUnit, historyCount) == 0x148);
static_assert(offsetof(PrevTouchUnit, history) == 0x150);
static_assert(offsetof(PrevTouchUnit, queueHead) == 0x334);
static_assert(offsetof(PrevTouchUnit, queueTail) == 0x335);
static_assert(offsetof(PrevTouchUnit, queueDim1) == 0x336);
static_assert(offsetof(PrevTouchUnit, queueDim2) == 0x346);
static_assert(offsetof(PrevTouchUnit, startPos) == 0x356);
static_assert(offsetof(PrevTouchUnit, firstRptPos) == 0x35A);
static_assert(offsetof(PrevTouchUnit, field35E) == 0x35E);
static_assert(offsetof(PrevTouchUnit, field360) == 0x360);
static_assert(offsetof(PrevTouchUnit, falseFlickPos) == 0x364);
static_assert(offsetof(PrevTouchUnit, falseFlickOffset) == 0x36C);
static_assert(offsetof(PrevTouchUnit, tdDebouncePos) == 0x370);
static_assert(offsetof(PrevTouchUnit, tdDebounceTime) == 0x378);
static_assert(offsetof(PrevTouchUnit, field380) == 0x380);
static_assert(offsetof(PrevTouchUnit, rawDelta) == 0x388);
static_assert(offsetof(PrevTouchUnit, rawVelocity) == 0x38C);
static_assert(offsetof(PrevTouchUnit, rawAccel) == 0x390);
static_assert(offsetof(PrevTouchUnit, posDelta) == 0x394);
static_assert(offsetof(PrevTouchUnit, posDelta2) == 0x398);
static_assert(offsetof(PrevTouchUnit, posDeltaSumDim1) == 0x39C);
static_assert(offsetof(PrevTouchUnit, posDeltaSumDim2) == 0x3A0);
static_assert(offsetof(PrevTouchUnit, debounceFrames) == 0x3A4);
static_assert(offsetof(PrevTouchUnit, field3A6) == 0x3A6);
static_assert(offsetof(PrevTouchUnit, field3A7) == 0x3A7);
static_assert(offsetof(PrevTouchUnit, betaGripFrames) == 0x3A8);
static_assert(offsetof(PrevTouchUnit, blTrustFrames) == 0x3AC);
static_assert(offsetof(PrevTouchUnit, clickToFlickOffset) == 0x3AE);
static_assert(offsetof(PrevTouchUnit, curIdx) == 0x3B2);
static_assert(offsetof(PrevTouchUnit, postReportIirCoef) == 0x3B3);
static_assert(offsetof(PrevTouchUnit, prevIirCoef) == 0x3B4);
static_assert(offsetof(PrevTouchUnit, maxSqDistMmFromPrev) == 0x3B6);
static_assert(offsetof(PrevTouchUnit, field3D4) == 0x3D4);
static_assert(offsetof(PrevTouchUnit, minField068) == 0x3D8);
static_assert(offsetof(PrevTouchUnit, minField06A) == 0x3DA);
static_assert(offsetof(PrevTouchUnit, maxField068) == 0x3DC);
static_assert(offsetof(PrevTouchUnit, maxField06A) == 0x3DE);
static_assert(offsetof(PrevTouchUnit, strongZnTime) == 0x3E0);
static_assert(offsetof(PrevTouchUnit, field44C) == 0x44C);
static_assert(offsetof(PrevTouchUnit, field44D) == 0x44D);
static_assert(offsetof(PrevTouchUnit, field44E) == 0x44E);
static_assert(offsetof(PrevTouchUnit, field450) == 0x450);
static_assert(offsetof(PrevTouchUnit, field454) == 0x454);
static_assert(offsetof(PrevTouchUnit, field458) == 0x458);
static_assert(offsetof(PrevTouchUnit, field460) == 0x460);
static_assert(offsetof(PrevTouchUnit, field462) == 0x462);
static_assert(offsetof(PrevTouchUnit, field464) == 0x464);
static_assert(offsetof(PrevTouchUnit, field465) == 0x465);
static_assert(offsetof(PrevTouchUnit, minSigOffAtTouchDown) == 0x466);
static_assert(offsetof(PrevTouchUnit, timeOffAtTouchDown) == 0x468);
static_assert(offsetof(PrevTouchUnit, timeRptAtTouchDown) == 0x46A);
static_assert(offsetof(PrevTouchUnit, predictAlongDim1) == 0x46C);
static_assert(offsetof(PrevTouchUnit, predictFrames) == 0x46D);
static_assert(offsetof(PrevTouchUnit, predictTurnFrames) == 0x46E);
static_assert(offsetof(PrevTouchUnit, predictForwardFrames) == 0x46F);
static_assert(offsetof(PrevTouchUnit, predictPos) == 0x470);

/// 当前帧触点表（g_touchesGrid / g_touchesHand / g_touchesSide，经 g_touchesPtr 选用）。
struct TouchTable {
    uint8_t count;  // 有效触点数
    uint8_t reportedCount;  // 上一次 TouchReport 对外报出的点数
    uint8_t field002;
    uint8_t field003;
    uint8_t field004;
    uint8_t field005;
    uint8_t field006;
    uint8_t coreTouchCount;  // HardwareAnalyzer：报过点、不贴边、非多指拆分的触点数
    uint8_t reportedThisFrame;  // 本帧对外报点且未抬起的触点数
    uint8_t lastReportedId;  // 其中最后一个的 ID
    uint8_t gripReportedThisFrame;  // 其中带握持 / 合并标记的个数
    uint8_t field00B;
    uint8_t field00C;
    uint8_t field00D;
    uint8_t field00E;
    uint8_t field00F;
    uint8_t abnormalShapeCount;  // Touch_IsAbnormalShape 或已标记 bit2 的触点数
    uint8_t abnormalShape2Count;
    uint8_t abnormalShape3Count;
    uint8_t abnormalShape4Count;
    int16_t maxCoreEllipseRate;  // 这些触点椭圆度的最大值
    uint8_t reserved016[0x2];
    uint32_t extFlags;  // 外部状态位：水下 0x20 / 出水 0x40 / 接近 0x80 / 拿起 0x100 / 放下 0x200 …
    uint32_t newExtFlags;  // 本帧新出现的 extFlags 位
    uint32_t exceptionFlags;  // 全局异常位
    uint32_t stateFlags;  // bit0 有稳定且报过点的触点；bit1 field003 由非零变零；bit2 由零变非零
    uint64_t lastTouchDownTime;  // 最近一次对外按下的时间（ms）
    uint64_t lastLiftOffTime;  // 最近一次对外抬起的时间（ms）
    uint64_t lastLargeObjectTime;  // 最近一次出现大物体（volatileFlags 0x2000）的时间（ms）
    ThenarInfo thenar;  // 大鱼际 / 手掌区域信息（Thenar 模块）
    uint32_t field138;
    uint8_t reserved13C[0x4];
    TouchUnit unit[kMaxTouchNum];
};
static_assert(sizeof(TouchTable) == 0x1D60);
static_assert(offsetof(TouchTable, count) == 0x000);
static_assert(offsetof(TouchTable, reportedCount) == 0x001);
static_assert(offsetof(TouchTable, field002) == 0x002);
static_assert(offsetof(TouchTable, field003) == 0x003);
static_assert(offsetof(TouchTable, field004) == 0x004);
static_assert(offsetof(TouchTable, field005) == 0x005);
static_assert(offsetof(TouchTable, field006) == 0x006);
static_assert(offsetof(TouchTable, coreTouchCount) == 0x007);
static_assert(offsetof(TouchTable, reportedThisFrame) == 0x008);
static_assert(offsetof(TouchTable, lastReportedId) == 0x009);
static_assert(offsetof(TouchTable, gripReportedThisFrame) == 0x00A);
static_assert(offsetof(TouchTable, field00B) == 0x00B);
static_assert(offsetof(TouchTable, field00C) == 0x00C);
static_assert(offsetof(TouchTable, field00D) == 0x00D);
static_assert(offsetof(TouchTable, field00E) == 0x00E);
static_assert(offsetof(TouchTable, field00F) == 0x00F);
static_assert(offsetof(TouchTable, abnormalShapeCount) == 0x010);
static_assert(offsetof(TouchTable, abnormalShape2Count) == 0x011);
static_assert(offsetof(TouchTable, abnormalShape3Count) == 0x012);
static_assert(offsetof(TouchTable, abnormalShape4Count) == 0x013);
static_assert(offsetof(TouchTable, maxCoreEllipseRate) == 0x014);
static_assert(offsetof(TouchTable, extFlags) == 0x018);
static_assert(offsetof(TouchTable, newExtFlags) == 0x01C);
static_assert(offsetof(TouchTable, exceptionFlags) == 0x020);
static_assert(offsetof(TouchTable, stateFlags) == 0x024);
static_assert(offsetof(TouchTable, lastTouchDownTime) == 0x028);
static_assert(offsetof(TouchTable, lastLiftOffTime) == 0x030);
static_assert(offsetof(TouchTable, lastLargeObjectTime) == 0x038);
static_assert(offsetof(TouchTable, thenar) == 0x040);
static_assert(offsetof(TouchTable, field138) == 0x138);
static_assert(offsetof(TouchTable, unit) == 0x140);

/// 上一帧触点表（g_prevTouchesGrid / Hand / Side，经 g_prevTouchesPtr 选用）。
struct PrevTouchTable {
    uint8_t count;  // 有效触点数
    uint8_t reportedCount;  // 上一次 TouchReport 对外报出的点数
    uint8_t field002;
    uint8_t field003;
    uint8_t field004;
    uint8_t field005;
    uint8_t field006;
    uint8_t coreTouchCount;  // HardwareAnalyzer：报过点、不贴边、非多指拆分的触点数
    uint8_t reportedThisFrame;  // 本帧对外报点且未抬起的触点数
    uint8_t lastReportedId;  // 其中最后一个的 ID
    uint8_t gripReportedThisFrame;  // 其中带握持 / 合并标记的个数
    uint8_t field00B;
    uint8_t field00C;
    uint8_t field00D;
    uint8_t field00E;
    uint8_t field00F;
    uint8_t abnormalShapeCount;  // Touch_IsAbnormalShape 或已标记 bit2 的触点数
    uint8_t abnormalShape2Count;
    uint8_t abnormalShape3Count;
    uint8_t abnormalShape4Count;
    int16_t maxCoreEllipseRate;  // 这些触点椭圆度的最大值
    uint8_t reserved016[0x2];
    uint32_t extFlags;  // 外部状态位：水下 0x20 / 出水 0x40 / 接近 0x80 / 拿起 0x100 / 放下 0x200 …
    uint32_t newExtFlags;  // 本帧新出现的 extFlags 位
    uint32_t exceptionFlags;  // 全局异常位
    uint32_t stateFlags;  // bit0 有稳定且报过点的触点；bit1 field003 由非零变零；bit2 由零变非零
    uint64_t lastTouchDownTime;  // 最近一次对外按下的时间（ms）
    uint64_t lastLiftOffTime;  // 最近一次对外抬起的时间（ms）
    uint64_t lastLargeObjectTime;  // 最近一次出现大物体（volatileFlags 0x2000）的时间（ms）
    ThenarInfo thenar;  // 大鱼际 / 手掌区域信息（Thenar 模块）
    uint32_t field138;
    uint8_t reserved13C[0x4];
    PrevTouchUnit unit[kMaxTouchNum];
};
static_assert(sizeof(PrevTouchTable) == 0x5AA0);
static_assert(offsetof(PrevTouchTable, count) == 0x000);
static_assert(offsetof(PrevTouchTable, reportedCount) == 0x001);
static_assert(offsetof(PrevTouchTable, field002) == 0x002);
static_assert(offsetof(PrevTouchTable, field003) == 0x003);
static_assert(offsetof(PrevTouchTable, field004) == 0x004);
static_assert(offsetof(PrevTouchTable, field005) == 0x005);
static_assert(offsetof(PrevTouchTable, field006) == 0x006);
static_assert(offsetof(PrevTouchTable, coreTouchCount) == 0x007);
static_assert(offsetof(PrevTouchTable, reportedThisFrame) == 0x008);
static_assert(offsetof(PrevTouchTable, lastReportedId) == 0x009);
static_assert(offsetof(PrevTouchTable, gripReportedThisFrame) == 0x00A);
static_assert(offsetof(PrevTouchTable, field00B) == 0x00B);
static_assert(offsetof(PrevTouchTable, field00C) == 0x00C);
static_assert(offsetof(PrevTouchTable, field00D) == 0x00D);
static_assert(offsetof(PrevTouchTable, field00E) == 0x00E);
static_assert(offsetof(PrevTouchTable, field00F) == 0x00F);
static_assert(offsetof(PrevTouchTable, abnormalShapeCount) == 0x010);
static_assert(offsetof(PrevTouchTable, abnormalShape2Count) == 0x011);
static_assert(offsetof(PrevTouchTable, abnormalShape3Count) == 0x012);
static_assert(offsetof(PrevTouchTable, abnormalShape4Count) == 0x013);
static_assert(offsetof(PrevTouchTable, maxCoreEllipseRate) == 0x014);
static_assert(offsetof(PrevTouchTable, extFlags) == 0x018);
static_assert(offsetof(PrevTouchTable, newExtFlags) == 0x01C);
static_assert(offsetof(PrevTouchTable, exceptionFlags) == 0x020);
static_assert(offsetof(PrevTouchTable, stateFlags) == 0x024);
static_assert(offsetof(PrevTouchTable, lastTouchDownTime) == 0x028);
static_assert(offsetof(PrevTouchTable, lastLiftOffTime) == 0x030);
static_assert(offsetof(PrevTouchTable, lastLargeObjectTime) == 0x038);
static_assert(offsetof(PrevTouchTable, thenar) == 0x040);
static_assert(offsetof(PrevTouchTable, field138) == 0x138);
static_assert(offsetof(PrevTouchTable, unit) == 0x140);

/// 最近被回收、暂不复用的触点 ID（PrevTouch_RemoveOff 记下，PrevTouch_IsValidId 检查）。
struct RetiredTouchIds {
    uint8_t id[kMaxTouchNum];
    uint8_t count;
};

/// 当前在用的触点表（g_touchesPtr 所指）。
[[nodiscard]] inline TouchTable& Touches() noexcept { return *Image::At<TouchTable*>(0x6bb4f1d0); }
[[nodiscard]] inline TouchTable& TouchesGrid() noexcept { return Image::At<TouchTable>(0x6bbc6900); }
[[nodiscard]] inline TouchTable& TouchesSide() noexcept { return Image::At<TouchTable>(0x6bc8d1c0); }
/// 当前在用的历史触点表（g_prevTouchesPtr 所指）。
[[nodiscard]] inline PrevTouchTable& PrevTouches() noexcept { return *Image::At<PrevTouchTable*>(0x6bb4f1c0); }
[[nodiscard]] inline RetiredTouchIds& RetiredIds() noexcept { return Image::At<RetiredTouchIds>(0x6bbc68e0); }

}  // namespace Solvers::Tsa::Core
