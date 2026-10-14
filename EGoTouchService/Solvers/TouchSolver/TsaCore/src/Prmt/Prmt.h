#pragma once
// 参数表。原代码用 TSAPrmt(模块, 字段) / TSAConstPrmt(模块, 字段) 等宏访问，宏按字段分派到下面几块存储；
// 字段名能从断言文本、运行时参数的键名或调参表的说明得到的用原名，其余按用途或偏移命名。只列已改写的模块用到的字段。

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "Common/Image.h"

namespace Solvers::Tsa::Core {

/// 自适应 IIR 的系数表：位移量在 lightDelta 以下用 lightCoef，heavyDelta 以上用 heavyCoef，中间线性插值。
struct AiirPrmt {
    uint16_t lightDelta;
    uint16_t heavyDelta;
    uint8_t lightCoef;
    uint8_t heavyCoef;
};
static_assert(sizeof(AiirPrmt) == 0x006);
static_assert(offsetof(AiirPrmt, lightDelta) == 0x000);
static_assert(offsetof(AiirPrmt, heavyDelta) == 0x002);
static_assert(offsetof(AiirPrmt, lightCoef) == 0x004);
static_assert(offsetof(AiirPrmt, heavyCoef) == 0x005);

/// CMF（共模滤波）的一组门限。
struct CmfThold {
    int16_t amp;  // 传感器信号的共模噪声门限
    int16_t grad;  // 相邻传感器差分信号的共模噪声门限
    int16_t gameExtra;
};
static_assert(sizeof(CmfThold) == 0x006);
static_assert(offsetof(CmfThold, amp) == 0x000);
static_assert(offsetof(CmfThold, grad) == 0x002);
static_assert(offsetof(CmfThold, gameExtra) == 0x004);

/// 质心边缘修正的一段：触点尺寸不超过 maxSizeMm 时用这一段，把边缘信号比在 ln 表上的位置换算成补偿量。
struct CtdEdgeSegment {
    uint8_t maxSizeMm;
    uint8_t reserved001[0x1];
    uint8_t lnLowIdx;  // 补偿区间两端在 ln 表里的下标
    uint8_t lnHighIdx;
};
static_assert(sizeof(CtdEdgeSegment) == 0x004);
static_assert(offsetof(CtdEdgeSegment, maxSizeMm) == 0x000);
static_assert(offsetof(CtdEdgeSegment, lnLowIdx) == 0x002);
static_assert(offsetof(CtdEdgeSegment, lnHighIdx) == 0x003);

/// 质心边缘修正的分段表（一个边缘一张）。
struct CtdEdgePrmt {
    uint8_t segmentNum;
    CtdEdgeSegment segment[4];
};
static_assert(sizeof(CtdEdgePrmt) == 0x011);
static_assert(offsetof(CtdEdgePrmt, segmentNum) == 0x000);
static_assert(offsetof(CtdEdgePrmt, segment) == 0x001);

/// g_tsaPrmtFlash：机型参数表（W273AS2700 由 Prmt::LoadProject 装入）。原厂按 0x840 字节分成三段，这里按整块的偏移排布。
/// 调参表（TtPrmt）与运行时参数（RtPrmt）每帧把各自的当前值写回其中的对应字段，名字取自它们的键名与说明。
struct FlashPrmt {
    uint8_t reserved000[0x18];
    uint8_t displayXAxis;  // 与 dim1Axis 相等时 Dim1 是显示的 X 方向
    uint8_t dim1Axis;
    uint8_t reserved01A[0x1];
    uint8_t sensorPitchX16;  // 传感器间距（1/16 mm）；ConstPrmt 由它推导像素与 mm 的换算
    uint8_t reserved01C[0x1];
    uint8_t dim1Len;  // 列数
    uint8_t dim2Len;  // 行数
    uint8_t reserved01F[0xD];
    uint16_t sensorNum;  // dim1Len × dim2Len
    uint16_t field02E;  // TS_MMToDisplayPixel：mm × max(dim1Res, dim2Res) × 16 / field02E
    uint8_t reserved030[0x4];
    uint32_t sensorArchitecture;  // OnCell(1)、Hybrid Incell(2)、Full Incell(3)
    uint8_t reportRateHz;  // 0 时按 8 ms 一帧
    uint8_t reserved039[0x3];
    uint16_t dbEnterIdle;  // 允许进入 IDLE 后的防抖时间（ms）
    uint8_t reserved03E[0x22];
    uint32_t antiTouchScenarios;  // 哪些场景启用防误触
    uint32_t proximityScenarios;  // 哪些场景启用接近检测
    uint8_t reserved068[0x28];
    uint8_t dim1LogicFirst;  // Dim1 逻辑区域的首末列
    uint8_t dim1LogicLast;
    uint8_t dim2LogicFirst;
    uint8_t dim2LogicLast;
    int16_t field094;
    int16_t field096;
    int16_t field098;
    int16_t field09A;
    uint8_t reserved09C[0x50A];
    uint16_t dim1Res;  // Dim1 方向的显示分辨率
    uint16_t dim2Res;
    uint8_t reserved5AA[0x4];
    int8_t dim1ClipLow;  // 显示坐标两端的裁剪量
    int8_t dim1ClipHigh;
    int8_t dim2ClipLow;
    int8_t dim2ClipHigh;
    uint8_t reserved5B2[0x2];
    int16_t peakFilterReferenceThreshold;
    int16_t fingerThold;  // finger 模式的按下门限
    uint8_t reserved5B8[0x2];
    uint8_t sigRatioToDirectReport;  // 峰值超过 touchThold × 它 / 16 时直接上报，不做防抖
    uint8_t reserved5BB[0x15];
    uint8_t sdZnTholdScaleX16;
    uint8_t reserved5D1[0x7];
    int16_t gloveExitThold;  // 退出手套模式（确认手指）的门限；也是 DynPrmt 初值块的开头
    int16_t gloveThold;  // 手套模式的按下门限
    uint8_t reserved5DC[0x20];
    int32_t field5FC;
    int32_t field600;
    uint8_t reserved604[0x4];
    uint8_t field608;  // DynPrmt 初值块（0x608..0x626）的开头
    uint8_t reserved609[0x17];
    CmfThold cmfTholdNormal;  // 常态
    CmfThold cmfTholdAlt;  // TsaStatic::systemFlags bit18 置位时
    uint8_t field62C[0x50];  // ConstPrmt 取前 field025 项的最大值
    uint8_t reserved67C[0x194];
    uint32_t cmfMethodNormal;  // CMF 方法：常态 / 智能皮套（1 为 SOF）
    uint32_t cmfMethodSmartCover;
    uint8_t reserved818[0x6];
    uint8_t sigMult;  // 触点区域遍历门限相对峰值的比例（/128）：默认
    uint8_t sigMultNormal;  // 设备接地
    uint8_t sigMultSD;  // 设备悬浮
    uint8_t reserved821[0x15];
    uint8_t largeObjectSizeMm;  // 判为大物体的尺寸
    uint8_t reserved837[0x1];
    uint32_t aftEdgeWidthThold;  // AFT 的 ewx / ewy 边缘宽度门限
    uint8_t reserved83C[0x4];
    CtdEdgePrmt ctdEdge[4];  // 质心边缘修正：Dim1 首端、Dim1 末端、Dim2 首端、Dim2 末端
    uint8_t reserved884[0x6];
    uint16_t accThold;  // 两触点连线的加速度门限
    uint8_t reserved88C[0x2];
    AiirPrmt aiir[3];
    uint8_t field8A0;
    uint8_t field8A1;
    uint8_t reserved8A2[0x57];
    uint8_t defaultSizeMm;  // 尚无尺寸的触点的默认尺寸（mm）；DynPrmt 初值
    uint8_t reserved8FA[0x2];
    uint32_t defaultTouchAction;  // 新触点默认的 TouchAction；也是 DynPrmt 初值块（0x8fc..0x90c）的开头
    uint16_t clickDuration;  // 点击后允许移动的时间（ms）
    uint8_t reserved902[0x2];
    uint8_t clickSuppressDistance;  // 判点击 / 轻扫、决定是否做点击抑制的移动距离（像素）
    uint8_t reserved905[0x1];
    uint8_t maxClickSuppressDistance;
    uint8_t reserved907[0x1];
    uint8_t flickDistance;  // 决定下一个触点默认是否做点击抑制的移动距离（像素）
    uint8_t field909;  // 调参表里标为 obsolete
    uint8_t reserved90A[0x2];
    int16_t noiseTholdQuiet;  // 噪声门限：没有噪声触点时
    int16_t noiseTholdOneNoisy;  // 恰有一个噪声触点时
    int16_t noiseTholdMultiNoisy;  // 多个噪声触点时；也是初始的峰门限
    int16_t noisyTouchSigSum;  // 历史触点的 sigSum 超过它即计为噪声触点
    uint8_t snrScaleX16;  // 门限按 SNR 换算的比例（/16）
    uint8_t reserved915[0x3];
    uint8_t zTholdFlags;  // DynPrmt 初值块（0x918..0x924）的开头
    uint8_t reserved919[0x3];
    uint32_t gloveFilterSides;  // 手套过滤作用的边：上 1、下 2、左 4、右 8
    uint16_t distToEnterGloveMode;  // 进入手套模式的移动距离（mm）
    uint8_t reserved922[0x1E];
    int16_t chargerNoiseTholdPercent;  // Hybrid Incell 充电噪声门限相对 HTI 门限的百分比；也是 DynPrmt 初值块（0x940..0x94c）的开头
    uint16_t field942;  // 调参表里标为 obsolete
    uint8_t reserved944[0xD0C];
    uint16_t field1650;
    uint16_t field1652;
    uint16_t field1654;
    uint16_t field1656;
    uint16_t field1658;
    uint16_t field165A;
    uint8_t reserved165C[0x4];
    uint32_t proxConfig;
    uint32_t proxTriggerThold;
    uint32_t proxLiftOffTholdMin;
    uint32_t proxLiftOffTholdMax;
    uint8_t proxLiftOffRatio;  // 最小抬起门限相对触发门限的比例
    uint8_t proxTzRatio;  // 触点区域遍历门限相对触发门限的比例
    uint8_t reserved1672[0x6];
    uint16_t proxBaselineSpeed;
    int16_t proxBaselineThold;
    uint16_t callScanRateProxOff;  // 通话、接近未触发时的扫描率
    uint16_t callScanRateProxOn;
    uint16_t lockScanRateProxOff;  // 锁屏
    uint16_t lockScanRateProxOn;
    uint8_t reserved1684[0x8];
    uint8_t proxAreaY;  // 接近检测沿 Y 方向的范围
    uint8_t reserved168D[0x3];
    uint32_t proxTimeAutoScreenOff;
    uint8_t reserved1694[0x10];
    uint32_t sideTouchConfig;
    uint16_t sideTouchXCoreStart;
    uint16_t sideTouchXCoreEnd;
    uint8_t sideTouchMinGripRatio;
    uint8_t sideTouchMidGripRatio;
    uint8_t sideTouchMaxGripRatio;
    uint8_t reserved16AF[0x5];
    int16_t sideSigThold;  // 侧边触摸的信号门限
    uint8_t reserved16B6[0x4];
    uint8_t sideSigRatioToDirectReport;
    uint8_t reserved16BB[0x1];
    uint16_t volumeFlickUp;
    uint16_t volumeFlickDown;
    uint16_t volumeStepUp;
    uint16_t volumeStepDown;
    uint16_t volumeActiveAreaStart;
    uint16_t volumeActiveAreaEnd;
    uint16_t volumeGreenAreaStart;
    uint16_t volumeGreenAreaEnd;
    uint16_t volumeSideEnabled;
    uint16_t volumeTouchMin;
    uint16_t volumeTouchMax;
    uint16_t volumeTimeFlick;
    uint16_t volumeTimeOnMin;
    uint16_t volumeTimeOffMin;
    uint16_t volumeTimeOffMax;
    uint16_t volumeTimeGui;
    uint16_t volumeForceAreaStart;
    uint16_t volumeForceAreaEnd;
    uint16_t volumeForceAreaGap;
    uint16_t volumeForceAreaGapCenter;
    uint16_t volumeForceAreaDebounce;
    uint16_t volumeForceMoveMax;
    uint16_t volumeForceTouchMax;
    uint16_t volumeForceShbReportYPosDebounce;
    uint16_t volumeMappingTimeDelay;
    uint16_t volumeMuteMinMove;
    uint16_t volumeMuteMaxTime;
    uint8_t reserved16F2[0x2];
    uint16_t volumeTimeToAntiTrigger;
    uint8_t reserved16F6[0x2];
    uint32_t volumeGuiOnTimeOut;
    uint32_t volumeGuiOffTimeOut;
    uint32_t volumeKeyDebounceTime;
    uint16_t volumeActiveAreaStart2;
    uint16_t volumeActiveAreaEnd2;
    uint16_t volumeGreenAreaStart2;
    uint16_t volumeGreenAreaEnd2;
    uint16_t volumeSideEnabled2;
    uint16_t volumeActiveAreaStart3;
    uint16_t volumeActiveAreaEnd3;
    uint16_t volumeGreenAreaStart3;
    uint16_t volumeGreenAreaEnd3;
    uint16_t volumeGreenAreaToMid3;
    uint16_t volumeSideEnabled3;
    uint8_t reserved171A[0x2];
    uint32_t volumeSensitivityLevel;  // 1：侧边门限取 3/4；2：取 1/2
    uint32_t volumeGestureSet;
    uint16_t powerAreaStart;
    uint16_t powerAreaEnd;
    uint16_t powerMoveMax;
    uint16_t powerTimeOnMin;
    uint16_t powerTimeOnMax;
    uint16_t powerTimeOffMin;
    uint16_t powerTimeOffMax;
    uint8_t reserved1732[0x2];
    uint16_t powerTouchMax;
    uint8_t reserved1736[0x2];
    uint32_t powerGestureSet;
    uint8_t motorVolumeOnWaveform;
    uint8_t motorVolumeOffWaveform;
    uint8_t reserved173E[0x6];
    uint32_t sensorEnabled;
    uint8_t sensorRate;
    uint8_t reserved1749[0x147];
};
static_assert(sizeof(FlashPrmt) == 0x1890);
static_assert(offsetof(FlashPrmt, displayXAxis) == 0x018);
static_assert(offsetof(FlashPrmt, dim1Axis) == 0x019);
static_assert(offsetof(FlashPrmt, sensorPitchX16) == 0x01B);
static_assert(offsetof(FlashPrmt, dim1Len) == 0x01D);
static_assert(offsetof(FlashPrmt, dim2Len) == 0x01E);
static_assert(offsetof(FlashPrmt, sensorNum) == 0x02C);
static_assert(offsetof(FlashPrmt, field02E) == 0x02E);
static_assert(offsetof(FlashPrmt, sensorArchitecture) == 0x034);
static_assert(offsetof(FlashPrmt, reportRateHz) == 0x038);
static_assert(offsetof(FlashPrmt, dbEnterIdle) == 0x03C);
static_assert(offsetof(FlashPrmt, antiTouchScenarios) == 0x060);
static_assert(offsetof(FlashPrmt, proximityScenarios) == 0x064);
static_assert(offsetof(FlashPrmt, dim1LogicFirst) == 0x090);
static_assert(offsetof(FlashPrmt, dim1LogicLast) == 0x091);
static_assert(offsetof(FlashPrmt, dim2LogicFirst) == 0x092);
static_assert(offsetof(FlashPrmt, dim2LogicLast) == 0x093);
static_assert(offsetof(FlashPrmt, field094) == 0x094);
static_assert(offsetof(FlashPrmt, field096) == 0x096);
static_assert(offsetof(FlashPrmt, field098) == 0x098);
static_assert(offsetof(FlashPrmt, field09A) == 0x09A);
static_assert(offsetof(FlashPrmt, dim1Res) == 0x5A6);
static_assert(offsetof(FlashPrmt, dim2Res) == 0x5A8);
static_assert(offsetof(FlashPrmt, dim1ClipLow) == 0x5AE);
static_assert(offsetof(FlashPrmt, dim1ClipHigh) == 0x5AF);
static_assert(offsetof(FlashPrmt, dim2ClipLow) == 0x5B0);
static_assert(offsetof(FlashPrmt, dim2ClipHigh) == 0x5B1);
static_assert(offsetof(FlashPrmt, peakFilterReferenceThreshold) == 0x5B4);
static_assert(offsetof(FlashPrmt, fingerThold) == 0x5B6);
static_assert(offsetof(FlashPrmt, sigRatioToDirectReport) == 0x5BA);
static_assert(offsetof(FlashPrmt, sdZnTholdScaleX16) == 0x5D0);
static_assert(offsetof(FlashPrmt, gloveExitThold) == 0x5D8);
static_assert(offsetof(FlashPrmt, gloveThold) == 0x5DA);
static_assert(offsetof(FlashPrmt, field5FC) == 0x5FC);
static_assert(offsetof(FlashPrmt, field600) == 0x600);
static_assert(offsetof(FlashPrmt, field608) == 0x608);
static_assert(offsetof(FlashPrmt, cmfTholdNormal) == 0x620);
static_assert(offsetof(FlashPrmt, cmfTholdAlt) == 0x626);
static_assert(offsetof(FlashPrmt, field62C) == 0x62C);
static_assert(offsetof(FlashPrmt, cmfMethodNormal) == 0x810);
static_assert(offsetof(FlashPrmt, cmfMethodSmartCover) == 0x814);
static_assert(offsetof(FlashPrmt, sigMult) == 0x81E);
static_assert(offsetof(FlashPrmt, sigMultNormal) == 0x81F);
static_assert(offsetof(FlashPrmt, sigMultSD) == 0x820);
static_assert(offsetof(FlashPrmt, largeObjectSizeMm) == 0x836);
static_assert(offsetof(FlashPrmt, aftEdgeWidthThold) == 0x838);
static_assert(offsetof(FlashPrmt, ctdEdge) == 0x840);
static_assert(offsetof(FlashPrmt, accThold) == 0x88A);
static_assert(offsetof(FlashPrmt, aiir) == 0x88E);
static_assert(offsetof(FlashPrmt, field8A0) == 0x8A0);
static_assert(offsetof(FlashPrmt, field8A1) == 0x8A1);
static_assert(offsetof(FlashPrmt, defaultSizeMm) == 0x8F9);
static_assert(offsetof(FlashPrmt, defaultTouchAction) == 0x8FC);
static_assert(offsetof(FlashPrmt, clickDuration) == 0x900);
static_assert(offsetof(FlashPrmt, clickSuppressDistance) == 0x904);
static_assert(offsetof(FlashPrmt, maxClickSuppressDistance) == 0x906);
static_assert(offsetof(FlashPrmt, flickDistance) == 0x908);
static_assert(offsetof(FlashPrmt, field909) == 0x909);
static_assert(offsetof(FlashPrmt, noiseTholdQuiet) == 0x90C);
static_assert(offsetof(FlashPrmt, noiseTholdOneNoisy) == 0x90E);
static_assert(offsetof(FlashPrmt, noiseTholdMultiNoisy) == 0x910);
static_assert(offsetof(FlashPrmt, noisyTouchSigSum) == 0x912);
static_assert(offsetof(FlashPrmt, snrScaleX16) == 0x914);
static_assert(offsetof(FlashPrmt, zTholdFlags) == 0x918);
static_assert(offsetof(FlashPrmt, gloveFilterSides) == 0x91C);
static_assert(offsetof(FlashPrmt, distToEnterGloveMode) == 0x920);
static_assert(offsetof(FlashPrmt, chargerNoiseTholdPercent) == 0x940);
static_assert(offsetof(FlashPrmt, field942) == 0x942);
static_assert(offsetof(FlashPrmt, field1650) == 0x1650);
static_assert(offsetof(FlashPrmt, field1652) == 0x1652);
static_assert(offsetof(FlashPrmt, field1654) == 0x1654);
static_assert(offsetof(FlashPrmt, field1656) == 0x1656);
static_assert(offsetof(FlashPrmt, field1658) == 0x1658);
static_assert(offsetof(FlashPrmt, field165A) == 0x165A);
static_assert(offsetof(FlashPrmt, proxConfig) == 0x1660);
static_assert(offsetof(FlashPrmt, proxTriggerThold) == 0x1664);
static_assert(offsetof(FlashPrmt, proxLiftOffTholdMin) == 0x1668);
static_assert(offsetof(FlashPrmt, proxLiftOffTholdMax) == 0x166C);
static_assert(offsetof(FlashPrmt, proxLiftOffRatio) == 0x1670);
static_assert(offsetof(FlashPrmt, proxTzRatio) == 0x1671);
static_assert(offsetof(FlashPrmt, proxBaselineSpeed) == 0x1678);
static_assert(offsetof(FlashPrmt, proxBaselineThold) == 0x167A);
static_assert(offsetof(FlashPrmt, callScanRateProxOff) == 0x167C);
static_assert(offsetof(FlashPrmt, callScanRateProxOn) == 0x167E);
static_assert(offsetof(FlashPrmt, lockScanRateProxOff) == 0x1680);
static_assert(offsetof(FlashPrmt, lockScanRateProxOn) == 0x1682);
static_assert(offsetof(FlashPrmt, proxAreaY) == 0x168C);
static_assert(offsetof(FlashPrmt, proxTimeAutoScreenOff) == 0x1690);
static_assert(offsetof(FlashPrmt, sideTouchConfig) == 0x16A4);
static_assert(offsetof(FlashPrmt, sideTouchXCoreStart) == 0x16A8);
static_assert(offsetof(FlashPrmt, sideTouchXCoreEnd) == 0x16AA);
static_assert(offsetof(FlashPrmt, sideTouchMinGripRatio) == 0x16AC);
static_assert(offsetof(FlashPrmt, sideTouchMidGripRatio) == 0x16AD);
static_assert(offsetof(FlashPrmt, sideTouchMaxGripRatio) == 0x16AE);
static_assert(offsetof(FlashPrmt, sideSigThold) == 0x16B4);
static_assert(offsetof(FlashPrmt, sideSigRatioToDirectReport) == 0x16BA);
static_assert(offsetof(FlashPrmt, volumeFlickUp) == 0x16BC);
static_assert(offsetof(FlashPrmt, volumeFlickDown) == 0x16BE);
static_assert(offsetof(FlashPrmt, volumeStepUp) == 0x16C0);
static_assert(offsetof(FlashPrmt, volumeStepDown) == 0x16C2);
static_assert(offsetof(FlashPrmt, volumeActiveAreaStart) == 0x16C4);
static_assert(offsetof(FlashPrmt, volumeActiveAreaEnd) == 0x16C6);
static_assert(offsetof(FlashPrmt, volumeGreenAreaStart) == 0x16C8);
static_assert(offsetof(FlashPrmt, volumeGreenAreaEnd) == 0x16CA);
static_assert(offsetof(FlashPrmt, volumeSideEnabled) == 0x16CC);
static_assert(offsetof(FlashPrmt, volumeTouchMin) == 0x16CE);
static_assert(offsetof(FlashPrmt, volumeTouchMax) == 0x16D0);
static_assert(offsetof(FlashPrmt, volumeTimeFlick) == 0x16D2);
static_assert(offsetof(FlashPrmt, volumeTimeOnMin) == 0x16D4);
static_assert(offsetof(FlashPrmt, volumeTimeOffMin) == 0x16D6);
static_assert(offsetof(FlashPrmt, volumeTimeOffMax) == 0x16D8);
static_assert(offsetof(FlashPrmt, volumeTimeGui) == 0x16DA);
static_assert(offsetof(FlashPrmt, volumeForceAreaStart) == 0x16DC);
static_assert(offsetof(FlashPrmt, volumeForceAreaEnd) == 0x16DE);
static_assert(offsetof(FlashPrmt, volumeForceAreaGap) == 0x16E0);
static_assert(offsetof(FlashPrmt, volumeForceAreaGapCenter) == 0x16E2);
static_assert(offsetof(FlashPrmt, volumeForceAreaDebounce) == 0x16E4);
static_assert(offsetof(FlashPrmt, volumeForceMoveMax) == 0x16E6);
static_assert(offsetof(FlashPrmt, volumeForceTouchMax) == 0x16E8);
static_assert(offsetof(FlashPrmt, volumeForceShbReportYPosDebounce) == 0x16EA);
static_assert(offsetof(FlashPrmt, volumeMappingTimeDelay) == 0x16EC);
static_assert(offsetof(FlashPrmt, volumeMuteMinMove) == 0x16EE);
static_assert(offsetof(FlashPrmt, volumeMuteMaxTime) == 0x16F0);
static_assert(offsetof(FlashPrmt, volumeTimeToAntiTrigger) == 0x16F4);
static_assert(offsetof(FlashPrmt, volumeGuiOnTimeOut) == 0x16F8);
static_assert(offsetof(FlashPrmt, volumeGuiOffTimeOut) == 0x16FC);
static_assert(offsetof(FlashPrmt, volumeKeyDebounceTime) == 0x1700);
static_assert(offsetof(FlashPrmt, volumeActiveAreaStart2) == 0x1704);
static_assert(offsetof(FlashPrmt, volumeActiveAreaEnd2) == 0x1706);
static_assert(offsetof(FlashPrmt, volumeGreenAreaStart2) == 0x1708);
static_assert(offsetof(FlashPrmt, volumeGreenAreaEnd2) == 0x170A);
static_assert(offsetof(FlashPrmt, volumeSideEnabled2) == 0x170C);
static_assert(offsetof(FlashPrmt, volumeActiveAreaStart3) == 0x170E);
static_assert(offsetof(FlashPrmt, volumeActiveAreaEnd3) == 0x1710);
static_assert(offsetof(FlashPrmt, volumeGreenAreaStart3) == 0x1712);
static_assert(offsetof(FlashPrmt, volumeGreenAreaEnd3) == 0x1714);
static_assert(offsetof(FlashPrmt, volumeGreenAreaToMid3) == 0x1716);
static_assert(offsetof(FlashPrmt, volumeSideEnabled3) == 0x1718);
static_assert(offsetof(FlashPrmt, volumeSensitivityLevel) == 0x171C);
static_assert(offsetof(FlashPrmt, volumeGestureSet) == 0x1720);
static_assert(offsetof(FlashPrmt, powerAreaStart) == 0x1724);
static_assert(offsetof(FlashPrmt, powerAreaEnd) == 0x1726);
static_assert(offsetof(FlashPrmt, powerMoveMax) == 0x1728);
static_assert(offsetof(FlashPrmt, powerTimeOnMin) == 0x172A);
static_assert(offsetof(FlashPrmt, powerTimeOnMax) == 0x172C);
static_assert(offsetof(FlashPrmt, powerTimeOffMin) == 0x172E);
static_assert(offsetof(FlashPrmt, powerTimeOffMax) == 0x1730);
static_assert(offsetof(FlashPrmt, powerTouchMax) == 0x1734);
static_assert(offsetof(FlashPrmt, powerGestureSet) == 0x1738);
static_assert(offsetof(FlashPrmt, motorVolumeOnWaveform) == 0x173C);
static_assert(offsetof(FlashPrmt, motorVolumeOffWaveform) == 0x173D);
static_assert(offsetof(FlashPrmt, sensorEnabled) == 0x1744);
static_assert(offsetof(FlashPrmt, sensorRate) == 0x1748);

/// g_tsaPrmtFlashConst：由机型参数推导出的常量（ConstPrmt::Init；0x40..0x56 由 ER_Init 写）。
struct FlashConstPrmt {
    uint16_t field000;
    uint8_t reserved002[0x2];
    uint32_t dim1DisplayScale;  // 显示坐标（含两端裁剪）对 Q8.8 像素坐标的比例，Q16
    uint32_t dim2DisplayScale;
    uint16_t aiirCoefToDeltaRatio[3];  // 每组 AIIR 系数的插值斜率，Q10
    uint16_t field012;
    uint16_t field014;
    uint16_t field016;
    uint16_t field018;
    uint8_t q8PerMm;  // 每 mm 的 Q8.8 传感器单位数：0x1000 / sensorPitchX16 + 1
    uint8_t reserved01B[0x1];
    uint16_t mmPerQ8Q16;  // 每个 Q8.8 传感器单位的 mm 数（Q16）：0x10000 / q8PerMm
    uint16_t sizeAreaScale;  // 0x32400 / sensorPitchX16²，即 256π / 传感器面积（mm²）：TS_GetSizeInMM 用
    uint16_t dim1LenQ8;  // dim1Len << 8
    uint16_t dim2LenQ8;
    uint8_t field024;  // dim1Axis 为 0 时是 dim2Len，否则是 dim1Len
    uint8_t field025;  // 与 field024 相反
    uint8_t field026;  // field62C 前 field025 项的最大值 + 1
    uint8_t reserved027[0x1];
    uint8_t dim1LogicLen;
    uint8_t dim2LogicLen;
    uint16_t dim1NearEdge;  // Dim1 两端的逻辑边缘（Q8.8）
    uint16_t dim1FarEdge;
    uint16_t dim2NearEdge;
    uint16_t dim2FarEdge;
    uint16_t field032;  // field096 - field094 + 1
    uint16_t field034;  // field09A - field098 + 1
    uint8_t reserved036[0x2];
    uint32_t field038;
    uint32_t field03C;
    uint8_t reserved040[0x1A];
    uint16_t field05A;
    uint16_t field05C;
    uint16_t field05E;
    uint16_t field060;
    uint8_t reserved062[0x2];
};
static_assert(sizeof(FlashConstPrmt) == 0x064);
static_assert(offsetof(FlashConstPrmt, field000) == 0x000);
static_assert(offsetof(FlashConstPrmt, dim1DisplayScale) == 0x004);
static_assert(offsetof(FlashConstPrmt, dim2DisplayScale) == 0x008);
static_assert(offsetof(FlashConstPrmt, aiirCoefToDeltaRatio) == 0x00C);
static_assert(offsetof(FlashConstPrmt, field012) == 0x012);
static_assert(offsetof(FlashConstPrmt, field014) == 0x014);
static_assert(offsetof(FlashConstPrmt, field016) == 0x016);
static_assert(offsetof(FlashConstPrmt, field018) == 0x018);
static_assert(offsetof(FlashConstPrmt, q8PerMm) == 0x01A);
static_assert(offsetof(FlashConstPrmt, mmPerQ8Q16) == 0x01C);
static_assert(offsetof(FlashConstPrmt, sizeAreaScale) == 0x01E);
static_assert(offsetof(FlashConstPrmt, dim1LenQ8) == 0x020);
static_assert(offsetof(FlashConstPrmt, dim2LenQ8) == 0x022);
static_assert(offsetof(FlashConstPrmt, field024) == 0x024);
static_assert(offsetof(FlashConstPrmt, field025) == 0x025);
static_assert(offsetof(FlashConstPrmt, field026) == 0x026);
static_assert(offsetof(FlashConstPrmt, dim1LogicLen) == 0x028);
static_assert(offsetof(FlashConstPrmt, dim2LogicLen) == 0x029);
static_assert(offsetof(FlashConstPrmt, dim1NearEdge) == 0x02A);
static_assert(offsetof(FlashConstPrmt, dim1FarEdge) == 0x02C);
static_assert(offsetof(FlashConstPrmt, dim2NearEdge) == 0x02E);
static_assert(offsetof(FlashConstPrmt, dim2FarEdge) == 0x030);
static_assert(offsetof(FlashConstPrmt, field032) == 0x032);
static_assert(offsetof(FlashConstPrmt, field034) == 0x034);
static_assert(offsetof(FlashConstPrmt, field038) == 0x038);
static_assert(offsetof(FlashConstPrmt, field03C) == 0x03C);
static_assert(offsetof(FlashConstPrmt, field05A) == 0x05A);
static_assert(offsetof(FlashConstPrmt, field05C) == 0x05C);
static_assert(offsetof(FlashConstPrmt, field05E) == 0x05E);
static_assert(offsetof(FlashConstPrmt, field060) == 0x060);

/// g_tsaPrmtDynamic：随场景、噪声、触点状态每帧更新的门限（DynPrmt）。
struct DynamicPrmt {
    int16_t touchThold;
    int16_t fingerThold;
    uint8_t reserved004[0x2];
    uint8_t sigRatioToDirectReport;
    uint8_t reserved007[0x1];
    int16_t sigThold;  // 峰检测门限
    int16_t edgePeakThold;
    int16_t peakThold;
    int16_t edgePeakTholdLow;
    int32_t znTouchDownThold;
    int32_t znFingerThold;
    int32_t znTouchDownThold2;
    uint8_t sdZnTholdScaleX16;  // 信号失衡时 znTouchDownThold = sigThold × 它 / 16
    uint8_t lowTholdOnDim1FarEdge;  // 峰在 Dim1 远端最后一列时用低门限
    uint8_t lowTholdOnDim2FarEdge;
    uint8_t reserved01F[0x1];
    int16_t underWaterThold;
    int16_t underWaterTholdLow;
    uint8_t field024;  // 机型参数初值块（0x24..0x42）的开头
    uint8_t reserved025[0x7];
    int16_t ssTholdLow;
    int16_t ssThold;
    uint8_t reserved030[0xC];
    CmfThold cmfThold;  // 每帧按 systemFlags bit18 从机型参数的两组里选
    int16_t noiseThold;  // 本帧的噪声门限
    int16_t sideNoiseThold;
    int16_t smallThold;
    int16_t sideSmallThold;
    int16_t clampAfterTouch;  // noiseThold - 1
    int16_t clampAfterTouchSide;
    uint8_t field04E;
    uint8_t stepNoTouch;  // noiseThold / 256，限制在 1..255
    uint8_t stepNoTouchSide;
    uint8_t field051;
    uint8_t stepAfterTouch;  // noiseThold / 64，限制在 1..255
    uint8_t reserved053[0x1];
    int16_t stageTimeoutMs;
    uint8_t halfReportRate;
    uint8_t reportRate;
    int16_t resetCounterCap;  // reportRateHz × 200
    int16_t field05A;
    int16_t refThold;  // peakFilterReferenceThreshold
    int16_t negRefThold;
    uint8_t reserved060[0xC];
    uint8_t defaultSizeMm;  // 尚无尺寸的触点的默认尺寸（mm）：上一个抬起的触点的尺寸，封顶 9
    uint8_t reserved06D[0x3];
    uint32_t defaultTouchAction;  // 新触点默认的 TouchAction：初值来自机型参数，此后由报过点的触点抬起时更新；也是机型参数初值块（0x70..0x80）的开头
    uint16_t clickDuration;
    uint8_t reserved076[0x2];
    uint8_t clickSuppressDistance;  // 外部像素
    uint8_t reserved079[0x1];
    uint8_t maxClickSuppressDistance;
    uint8_t reserved07B[0x1];
    uint8_t flickDistance;
    uint8_t reserved07D[0x3];
    uint8_t zTholdFlags;  // bit1 抬起门限按区域分支；bit2 手套、bit3 指关节模式用按下门限判稳定
    uint8_t reserved081[0x7];
    uint16_t distToEnterGloveMode;
    uint8_t reserved08A[0x2];
    int16_t chargerNoiseTholdPercent;
    uint8_t reserved08E[0x8];
    uint8_t skipZ8CheckOnInnerEdge;
    uint8_t reserved097[0x9];
};
static_assert(sizeof(DynamicPrmt) == 0x0A0);
static_assert(offsetof(DynamicPrmt, touchThold) == 0x000);
static_assert(offsetof(DynamicPrmt, fingerThold) == 0x002);
static_assert(offsetof(DynamicPrmt, sigRatioToDirectReport) == 0x006);
static_assert(offsetof(DynamicPrmt, sigThold) == 0x008);
static_assert(offsetof(DynamicPrmt, edgePeakThold) == 0x00A);
static_assert(offsetof(DynamicPrmt, peakThold) == 0x00C);
static_assert(offsetof(DynamicPrmt, edgePeakTholdLow) == 0x00E);
static_assert(offsetof(DynamicPrmt, znTouchDownThold) == 0x010);
static_assert(offsetof(DynamicPrmt, znFingerThold) == 0x014);
static_assert(offsetof(DynamicPrmt, znTouchDownThold2) == 0x018);
static_assert(offsetof(DynamicPrmt, sdZnTholdScaleX16) == 0x01C);
static_assert(offsetof(DynamicPrmt, lowTholdOnDim1FarEdge) == 0x01D);
static_assert(offsetof(DynamicPrmt, lowTholdOnDim2FarEdge) == 0x01E);
static_assert(offsetof(DynamicPrmt, underWaterThold) == 0x020);
static_assert(offsetof(DynamicPrmt, underWaterTholdLow) == 0x022);
static_assert(offsetof(DynamicPrmt, field024) == 0x024);
static_assert(offsetof(DynamicPrmt, ssTholdLow) == 0x02C);
static_assert(offsetof(DynamicPrmt, ssThold) == 0x02E);
static_assert(offsetof(DynamicPrmt, cmfThold) == 0x03C);
static_assert(offsetof(DynamicPrmt, noiseThold) == 0x042);
static_assert(offsetof(DynamicPrmt, sideNoiseThold) == 0x044);
static_assert(offsetof(DynamicPrmt, smallThold) == 0x046);
static_assert(offsetof(DynamicPrmt, sideSmallThold) == 0x048);
static_assert(offsetof(DynamicPrmt, clampAfterTouch) == 0x04A);
static_assert(offsetof(DynamicPrmt, clampAfterTouchSide) == 0x04C);
static_assert(offsetof(DynamicPrmt, field04E) == 0x04E);
static_assert(offsetof(DynamicPrmt, stepNoTouch) == 0x04F);
static_assert(offsetof(DynamicPrmt, stepNoTouchSide) == 0x050);
static_assert(offsetof(DynamicPrmt, field051) == 0x051);
static_assert(offsetof(DynamicPrmt, stepAfterTouch) == 0x052);
static_assert(offsetof(DynamicPrmt, stageTimeoutMs) == 0x054);
static_assert(offsetof(DynamicPrmt, halfReportRate) == 0x056);
static_assert(offsetof(DynamicPrmt, reportRate) == 0x057);
static_assert(offsetof(DynamicPrmt, resetCounterCap) == 0x058);
static_assert(offsetof(DynamicPrmt, field05A) == 0x05A);
static_assert(offsetof(DynamicPrmt, refThold) == 0x05C);
static_assert(offsetof(DynamicPrmt, negRefThold) == 0x05E);
static_assert(offsetof(DynamicPrmt, defaultSizeMm) == 0x06C);
static_assert(offsetof(DynamicPrmt, defaultTouchAction) == 0x070);
static_assert(offsetof(DynamicPrmt, clickDuration) == 0x074);
static_assert(offsetof(DynamicPrmt, clickSuppressDistance) == 0x078);
static_assert(offsetof(DynamicPrmt, maxClickSuppressDistance) == 0x07A);
static_assert(offsetof(DynamicPrmt, flickDistance) == 0x07C);
static_assert(offsetof(DynamicPrmt, zTholdFlags) == 0x080);
static_assert(offsetof(DynamicPrmt, distToEnterGloveMode) == 0x088);
static_assert(offsetof(DynamicPrmt, chargerNoiseTholdPercent) == 0x08C);
static_assert(offsetof(DynamicPrmt, skipZ8CheckOnInnerEdge) == 0x096);

/// g_tsaPrmtRam：运行中由各模块按场景改写的参数。
struct RamPrmt {
    CtdEdgePrmt ctdEdge;  // CTD 按触点所在的边缘从机型参数里选一张写入
    uint8_t reserved011[0x5];
    AiirPrmt aiir;        // Filter 按触点状态从机型参数里选一组写入
};
static_assert(sizeof(RamPrmt) == 0x1C);
static_assert(offsetof(RamPrmt, aiir) == 0x16);

/// g_tsaPrmtRamConst
struct RamConstPrmt {
    uint8_t reserved000[0x2];
    uint16_t aiirScaledCoefToDeltaRatio;  // 系数插值斜率，Q10：每单位位移增加的系数
};
static_assert(offsetof(RamConstPrmt, aiirScaledCoefToDeltaRatio) == 0x2);

/// 调参表（g_tsaTTPrmt）与运行时参数表（g_tsaRTPrmt）的一项：当前值、缺省值、上下限和给调参工具看的说明。
struct TuningEntry {
    int32_t value;
    int32_t defaultValue;
    int32_t maxValue;
    int32_t minValue;
    const char* description;
};
static_assert(sizeof(TuningEntry) == 0x18);

inline constexpr size_t kRtPrmtNum = 16;
inline constexpr size_t kTtPrmtNum = 107;
using RtPrmtTable = std::array<TuningEntry, kRtPrmtNum>;
using TtPrmtTable = std::array<TuningEntry, kTtPrmtNum>;
static_assert(sizeof(RtPrmtTable) == 0x180);
static_assert(sizeof(TtPrmtTable) == 0xA08);

[[nodiscard]] inline FlashPrmt& PrmtFlash() noexcept { return Image::At<FlashPrmt>(0x6bca8a00); }
[[nodiscard]] inline FlashConstPrmt& PrmtFlashConst() noexcept { return Image::At<FlashConstPrmt>(0x6bcab600); }
[[nodiscard]] inline DynamicPrmt& PrmtDynamic() noexcept { return Image::At<DynamicPrmt>(0x6bcab680); }
[[nodiscard]] inline RamPrmt& PrmtRam() noexcept { return Image::At<RamPrmt>(0x6bcab720); }
[[nodiscard]] inline RamConstPrmt& PrmtRamConst() noexcept { return Image::At<RamConstPrmt>(0x6bcab664); }
/// 运行时参数：场景处理按键名改写的当前表，与每帧从机型参数重建的缺省表。
[[nodiscard]] inline RtPrmtTable& PrmtRunTime() noexcept { return Image::At<RtPrmtTable>(0x6bcab780); }
[[nodiscard]] inline RtPrmtTable& PrmtRunTimeDefault() noexcept { return Image::At<RtPrmtTable>(0x6bcab900); }
[[nodiscard]] inline TtPrmtTable& PrmtTuning() noexcept { return Image::At<TtPrmtTable>(0x6bcaba80); }

/// 机型名（参数表开头的字符串，Prmt::GetProjectName / TSA_GetPrmtName）。
[[nodiscard]] inline const char* ProjectName() noexcept { return reinterpret_cast<const char*>(&PrmtFlash()); }

/// 机型名是否以给定的 4 个字符开头。
[[nodiscard]] inline bool IsProject(const char* prefix) noexcept { return std::strncmp(ProjectName(), prefix, 4) == 0; }

/// 与 TSA_GetXRes 等相同的判定：dim1Axis 与 displayXAxis 相等时 Dim1 是显示的 X 方向。
[[nodiscard]] inline bool IsDim1DisplayX() noexcept {
    return PrmtFlash().dim1Axis == PrmtFlash().displayXAxis;
}

/// FlashPrmt::sensorArchitecture
inline constexpr uint32_t kFullIncell = 3;

}  // namespace Solvers::Tsa::Core
