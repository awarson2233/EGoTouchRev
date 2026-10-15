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
    uint8_t gameCommonNoise;  // 游戏场景下再按 Dim1 求一遍各行的公共噪声（写入 SelfData）
    uint8_t reserved005[0x1];
};
static_assert(sizeof(CmfThold) == 0x006);
static_assert(offsetof(CmfThold, amp) == 0x000);
static_assert(offsetof(CmfThold, grad) == 0x002);
static_assert(offsetof(CmfThold, gameCommonNoise) == 0x004);

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
/// 单个传感器中心相对格中心的偏移（1/128 格），TZ 求质心时加上。
struct SensorOffset {
    uint8_t dim1;
    uint8_t dim2;
    uint8_t flags;  // bit0：启用
    int8_t dim1Offset;
    int8_t dim2Offset;
};
static_assert(sizeof(SensorOffset) == 0x005);
static_assert(offsetof(SensorOffset, dim1) == 0x000);
static_assert(offsetof(SensorOffset, dim2) == 0x001);
static_assert(offsetof(SensorOffset, flags) == 0x002);
static_assert(offsetof(SensorOffset, dim1Offset) == 0x003);
static_assert(offsetof(SensorOffset, dim2Offset) == 0x004);

/// 单个传感器的 raw 平移与 Dif 缩放（TPSensor）。
struct SensorTranslate {
    uint8_t dim1;
    uint8_t dim2;
    uint16_t difScale;  // Dif × difScale / 1024
    int16_t rawOffset;  // raw 加上的平移量
};
static_assert(sizeof(SensorTranslate) == 0x006);
static_assert(offsetof(SensorTranslate, dim1) == 0x000);
static_assert(offsetof(SensorTranslate, dim2) == 0x001);
static_assert(offsetof(SensorTranslate, difScale) == 0x002);
static_assert(offsetof(SensorTranslate, rawOffset) == 0x004);

/// ER（边缘修正）的一条逻辑边：触点到逻辑边缘的距离落在 (minDistMm, maxDistMm) 内时做 move-in 修正（只用低字节）。
/// 四条边依次是上、下、左、右（TSA side 1、2、4、8）。
struct ErEdgeMmPrmt {
    uint16_t minDistMm;  // 也是 ER_GetLimit 的边缘宽度
    uint16_t maxDistMm;
    uint8_t field4;  // erEdgeMm[0].field4：底边 move-in 修正开关（原代码只读这一处）
    uint8_t reserved005[0x1];
};
static_assert(sizeof(ErEdgeMmPrmt) == 0x006);
static_assert(offsetof(ErEdgeMmPrmt, minDistMm) == 0x000);
static_assert(offsetof(ErEdgeMmPrmt, maxDistMm) == 0x002);
static_assert(offsetof(ErEdgeMmPrmt, field4) == 0x004);

/// ErEdgeMmPrmt 换算成 Q8.8 传感器坐标（ER_Init）。
struct ErEdgePrmt {
    uint16_t minDist;
    uint16_t maxDist;
    uint8_t reserved004[0x2];
};
static_assert(sizeof(ErEdgePrmt) == 0x006);
static_assert(offsetof(ErEdgePrmt, minDist) == 0x000);
static_assert(offsetof(ErEdgePrmt, maxDist) == 0x002);

struct FlashPrmt {
    uint8_t reserved000[0x18];
    uint8_t displayXAxis;  // 与 dim1Axis 相等时 Dim1 是显示的 X 方向
    uint8_t dim1Axis;
    uint8_t dim1OnLongAxis;  // 非 0 时 Dim1 沿屏幕长边（TZ 的握持比按长轴算）
    uint8_t sensorPitchX16;  // 传感器间距（1/16 mm）；ConstPrmt 由它推导像素与 mm 的换算
    uint8_t sensorPitch2X16;  // Dim2 方向的传感器间距（1/16 mm）
    uint8_t dim1Len;  // 列数
    uint8_t dim2Len;  // 行数
    uint8_t afeDim1Len;  // AFE 帧的列数、行数（帧数据按它拷贝）
    uint8_t afeDim2Len;
    uint8_t reserved021[0xB];
    uint16_t sensorNum;  // dim1Len × dim2Len
    uint16_t field02E;  // TS_MMToDisplayPixel：mm × max(dim1Res, dim2Res) × 16 / field02E
    uint8_t reserved030[0x4];
    uint32_t sensorArchitecture;  // OnCell(1)、Hybrid Incell(2)、Full Incell(3)
    uint8_t reportRateHz;  // 0 时按 8 ms 一帧
    uint8_t reserved039[0x1];
    uint16_t idleEnterDelayMs;  // 可以进 IDLE 之后再等多久才对外请求
    uint16_t dbEnterIdle;  // 允许进入 IDLE 后的防抖时间（ms）
    uint16_t dbEnterIdleGame;  // 游戏场景的 dbEnterIdle
    uint16_t dbEnterIdleNormal;  // 普通场景的 dbEnterIdle
    uint16_t dbEnterIdleTyping;  // 输入场景的 dbEnterIdle
    uint8_t field044;  // TouchMode：为 0 时手指 → 无物体的切换防抖 60 帧，否则立即切换
    uint8_t reserved045[0x1];
    uint16_t idleAfterInitMs;  // 互容主流程初始化后多久之内不进 IDLE
    uint16_t recalRawCenter;  // BLRecal：raw 上下边界的目标中心与允许范围
    uint16_t recalRawRange;
    uint8_t reserved04C[0x4];
    uint32_t featureFlagsDefault;  // TsaStatic::featureFlags 的初值（再或上 featureFlagsForce）
    uint32_t featureFlagsForce;
    uint32_t featureFlags2Default;
    uint32_t featureFlags2Force;
    uint32_t antiTouchScenarios;  // 哪些场景启用防误触
    uint32_t proximityScenarios;  // 哪些场景启用接近检测
    uint8_t cmfEnabled;
    uint8_t blUpdateFromPreCmfRaw;  // 基线 IIR 按 CMF 前的 raw 更新
    uint8_t blDirtyCheck;  // 查当前基线是否被污染
    uint8_t blAbnormalCheck;  // 查基线异常（否则走 BLReset_Check）
    uint8_t ssNegSignalStableCheck;
    uint8_t cmfSmartCoverRefine;  // 智能皮套时以 1.5 倍公共噪声为门限再求一次
    uint8_t reserved06E[0x22];
    uint8_t dim1LogicFirst;  // Dim1 逻辑区域的首末列
    uint8_t dim1LogicLast;
    uint8_t dim2LogicFirst;
    uint8_t dim2LogicLast;
    int16_t field094;
    int16_t field096;
    int16_t field098;
    int16_t field09A;
    uint8_t reserved09C[0x4];
    double dim1PitchMap[0x50];  // 传感器间距映射：第 i 格起点的物理位置（首项为 100.0 表示等距、不映射）
    double dim2PitchMap[0x50];
    uint16_t outputResScale;  // 对外坐标的分辨率倍数（TSA 可设置），TSAOut 的显示坐标按它放大
    uint8_t dim1Flip;  // 显示坐标在 Dim1 方向翻转
    uint8_t dim2Flip;
    uint8_t displaySwapXy;  // 与 dim1Axis ^ displayXAxis 相等时 X 取 Dim1
    uint8_t reserved5A5[0x1];
    uint16_t dim1Res;  // Dim1 方向的显示分辨率
    uint16_t dim2Res;
    uint8_t reserved5AA[0x4];
    int8_t dim1ClipLow;  // 显示坐标两端的裁剪量
    int8_t dim1ClipHigh;
    int8_t dim2ClipLow;
    int8_t dim2ClipHigh;
    int8_t dim2EdgeLow;  // Dim2 两端裁剪为负（向外延伸）时的边缘缩放参考
    int8_t dim2EdgeHigh;
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
    uint8_t rawForceNormalize;  // Rawdata：raw 按有符号值归一化，且总是做归一化
    uint8_t reserved605[0x3];
    uint8_t field608;  // DynPrmt 初值块（0x608..0x626）的开头
    uint8_t reserved609[0x17];
    CmfThold cmfTholdNormal;  // 常态
    CmfThold cmfTholdAlt;  // TsaStatic::systemFlags bit18 置位时
    uint8_t field62C[0x50];  // ConstPrmt 取前 field025 项的最大值
    uint8_t reserved67C[0x194];
    uint32_t cmfMethodNormal;  // CMF 方法：常态 / 智能皮套（1 为 SOF）
    uint32_t cmfMethodSmartCover;
    uint8_t noisyTouchDownDebounce;  // 触点处的共模噪声大于 500 或小于 -400 时按下防抖的最少帧数；0 为不启用
    uint8_t reserved819[0x3];
    int16_t gripModeSigThold;  // gripFlags 0x4 时 TZ 的区域遍历门限
    uint8_t sigMult;  // 触点区域遍历门限相对峰值的比例（/128）：默认
    uint8_t sigMultNormal;  // 设备接地
    uint8_t sigMultSD;  // 设备悬浮
    uint8_t reserved821[0x7];
    int32_t absFfZ9;  // TZ：区域中心 Z9 超过它判为大手指
    uint16_t ffMaxMajorMm;  // TZ：长轴不到它且不贴边时判为大手指
    uint16_t absFfMinorMm;  // TZ：短轴超过它判为大手指
    uint8_t absFfCentToMinPkRatio;  // TZ：质心到最小峰的信号比超过它判为大手指
    uint8_t mfCentToMinPkRatioLow;  // TZ：质心到最小峰的信号比低于它判为多指
    uint8_t peakFilterCentToMinPkRatio;  // TZ：相距不到 3 格的两峰之间的质心到最小峰信号比超过它时去掉较弱的峰
    uint8_t mfCentToMinPkRatio;  // 质心到最小峰的信号比低于它判为多指（TZ 形状判定、SubTZ 多指判定）
    uint8_t reserved834[0x2];
    uint8_t largeObjectSizeMm;  // 判为大物体的尺寸
    uint8_t reserved837[0x1];
    uint32_t aftEdgeWidthThold;  // AFT 的 ewx / ewy 边缘宽度门限
    uint32_t gripFlags;  // TZ 的握持 / 遍历模式开关（kGrip*）
    CtdEdgePrmt ctdEdge[4];  // 质心边缘修正：Dim1 首端、Dim1 末端、Dim2 首端、Dim2 末端
    int16_t centroidSigOffsetDim1;  // gripFlags 0x400 时，TZ 质心权重先减去的信号量
    int16_t centroidSigOffsetDim2;
    uint8_t idtSnapDistMm;  // IDT：位移、预测位移都在此距离内且帧龄超过 5 的历史触点直接吸附（代价置为极小）
    uint8_t reserved889[0x1];
    uint16_t accThold;  // 两触点连线的加速度门限
    uint8_t reserved88C[0x2];
    AiirPrmt aiir[3];
    uint8_t field8A0;
    uint8_t jitRadiusDisplay;  // 抗抖动半径（显示像素），ConstPrmt 换算成 jitRadius
    uint8_t liftOffJitterSuppress;  // Filter：抬起过程中信号骤降时按抗抖动半径压住坐标
    uint8_t noCurvePredict;  // Filter：为 0 时轨迹折返后按速度外推坐标
    uint8_t rxLineZ8Ratio;  // RxLineFilter：同一扫描线上更强的峰 Z8 超过本峰 Z8 的这个比例（/128）判为串扰
    uint8_t rxLineMaxDist;  // RxLineFilter：两峰的扫描线位置相差小于它才算同一线
    uint8_t reserved8A6[0xE];
    uint8_t sigSumReserveTouch;  // SigSumFilter 保留的触点数
    uint8_t reserved8B5[0x4];
    uint8_t gripPeakEdgeDist;  // 峰到边缘的距离超过它不算握持峰；也是握持合并峰的搜索半径
    uint16_t gripTimeInterval;  // 握持格与非握持格的按下时间差超过它才拆分区域
    uint16_t maxNotGripTime;  // 非握持峰的握持时间不超过它才看椭圆度
    uint8_t reserved8BE[0x3A];
    uint8_t gridIirWeight;  // GridIIR：全频点噪声时 Dif 的 IIR 权重（/256）
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
    uint8_t reserved922[0x2];
    ErEdgeMmPrmt erEdgeMm[4];  // ER：上、下、左、右四条逻辑边
    uint8_t wetFingerMinWaterCells;  // FIException：判湿手指所需的最少水膜格数
    uint8_t waterSigLowPercent;  // 水膜格的 Dif 区间：dif.maxSig 的百分比
    uint8_t waterSigHighPercent;
    uint8_t reserved93F[0x1];
    int16_t chargerNoiseTholdPercent;  // Hybrid Incell 充电噪声门限相对 HTI 门限的百分比；也是 DynPrmt 初值块（0x940..0x94c）的开头
    uint16_t field942;  // 调参表里标为 obsolete
    uint8_t reserved944[0x8];
    uint8_t sensorOffsetNum;
    SensorOffset sensorOffset[38];
    uint8_t reservedA0B[0x2];
    uint8_t sensorTranslateNum;
    SensorTranslate sensorTranslate[20];
    uint8_t brokenSensorCompensation;  // TPSensor：用邻格外推补偿坏传感器的 Dif
    uint8_t reservedA87[0xBC9];
    uint16_t rawLowerLimit;  // 坏点：raw 的上下限与相邻格之差的限（ConstPrmt 再放宽一个余量）
    uint16_t rawUpperLimit;
    uint16_t rawDeltaLimit;
    uint16_t field1656;
    uint16_t field1658;
    uint16_t field165A;
    uint8_t difBrokenCheck;  // HardwareAnalyzer：按 Dif 查坏点（只在 Full Incell 下）
    uint8_t reserved165D[0x3];
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
    uint8_t reserved1694[0x8];
    uint8_t afeFreqHopMode;  // 模拟 AFE 能力的跳频方式（1 为手动跳频）
    uint8_t reserved169D[0x1];
    uint16_t maxScanRate;  // 扫描率上限（Hz）
    uint16_t gameMaxScanRate;  // 高帧率游戏场景的扫描率上限
    uint8_t reserved16A2[0x2];
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
    uint8_t reserved173E[0x4];
    uint8_t antiTouchProxMode;  // AntiTouch：非 0 时按本帧接近状态直接报 extFlags（锁屏除外）
    uint8_t reserved1743[0x1];
    uint32_t sensorEnabled;
    uint8_t sensorRate;
    uint8_t reserved1749[0x3];
    uint32_t touchReportFlags;  // TouchReport：bit1 姿态误触时多步回退
    uint8_t toeSynaBl;  // ToeSynaBl 工作区开关
    uint8_t toeSideCols;  // 两侧各几列算侧边
    uint8_t reserved1752[0x2];
    uint32_t toeSynaBlFlags;  // bit0 背光偏移、bit1 水、bit2 自互容不符、bit3 异常触点形状
    uint8_t reserved1758[0xE0];
    uint32_t hotZoneAdjustCore;  // HotZoneAdjust：0 时中心落在核心区（非边缘格）的热区不调整
    uint8_t hotZoneMinGap;  // 触点到最近与次近热区的距离须相差超过它（外部单位）
    uint8_t hotZoneGridDim1;  // 判定热区所在格时把屏幕分成的列数、行数
    uint8_t hotZoneGridDim2;
    uint8_t hotZoneSizeNum;  // hotZoneSize 的项数
    uint16_t hotZoneSize[10];  // 热区尺寸档（外部单位）：取与热区面积最接近的一档
    uint16_t hotZoneExpandMax[4];  // 最大尺寸档的扩展量（外部单位）：Dim1 两端、Dim2 两端，按热区所在列 / 行在两端间插值
    uint16_t hotZoneExpandMin[4];  // 最小尺寸档的扩展量
    uint8_t reserved1864[0x2C];
};
static_assert(sizeof(FlashPrmt) == 0x1890);
static_assert(offsetof(FlashPrmt, displayXAxis) == 0x018);
static_assert(offsetof(FlashPrmt, dim1Axis) == 0x019);
static_assert(offsetof(FlashPrmt, dim1OnLongAxis) == 0x01A);
static_assert(offsetof(FlashPrmt, sensorPitchX16) == 0x01B);
static_assert(offsetof(FlashPrmt, sensorPitch2X16) == 0x01C);
static_assert(offsetof(FlashPrmt, dim1Len) == 0x01D);
static_assert(offsetof(FlashPrmt, dim2Len) == 0x01E);
static_assert(offsetof(FlashPrmt, afeDim1Len) == 0x01F);
static_assert(offsetof(FlashPrmt, afeDim2Len) == 0x020);
static_assert(offsetof(FlashPrmt, sensorNum) == 0x02C);
static_assert(offsetof(FlashPrmt, field02E) == 0x02E);
static_assert(offsetof(FlashPrmt, sensorArchitecture) == 0x034);
static_assert(offsetof(FlashPrmt, reportRateHz) == 0x038);
static_assert(offsetof(FlashPrmt, idleEnterDelayMs) == 0x03A);
static_assert(offsetof(FlashPrmt, dbEnterIdle) == 0x03C);
static_assert(offsetof(FlashPrmt, dbEnterIdleGame) == 0x03E);
static_assert(offsetof(FlashPrmt, dbEnterIdleNormal) == 0x040);
static_assert(offsetof(FlashPrmt, dbEnterIdleTyping) == 0x042);
static_assert(offsetof(FlashPrmt, field044) == 0x044);
static_assert(offsetof(FlashPrmt, idleAfterInitMs) == 0x046);
static_assert(offsetof(FlashPrmt, recalRawCenter) == 0x048);
static_assert(offsetof(FlashPrmt, recalRawRange) == 0x04A);
static_assert(offsetof(FlashPrmt, featureFlagsDefault) == 0x050);
static_assert(offsetof(FlashPrmt, featureFlagsForce) == 0x054);
static_assert(offsetof(FlashPrmt, featureFlags2Default) == 0x058);
static_assert(offsetof(FlashPrmt, featureFlags2Force) == 0x05C);
static_assert(offsetof(FlashPrmt, antiTouchScenarios) == 0x060);
static_assert(offsetof(FlashPrmt, proximityScenarios) == 0x064);
static_assert(offsetof(FlashPrmt, cmfEnabled) == 0x068);
static_assert(offsetof(FlashPrmt, blUpdateFromPreCmfRaw) == 0x069);
static_assert(offsetof(FlashPrmt, blDirtyCheck) == 0x06A);
static_assert(offsetof(FlashPrmt, blAbnormalCheck) == 0x06B);
static_assert(offsetof(FlashPrmt, ssNegSignalStableCheck) == 0x06C);
static_assert(offsetof(FlashPrmt, cmfSmartCoverRefine) == 0x06D);
static_assert(offsetof(FlashPrmt, dim1LogicFirst) == 0x090);
static_assert(offsetof(FlashPrmt, dim1LogicLast) == 0x091);
static_assert(offsetof(FlashPrmt, dim2LogicFirst) == 0x092);
static_assert(offsetof(FlashPrmt, dim2LogicLast) == 0x093);
static_assert(offsetof(FlashPrmt, field094) == 0x094);
static_assert(offsetof(FlashPrmt, field096) == 0x096);
static_assert(offsetof(FlashPrmt, field098) == 0x098);
static_assert(offsetof(FlashPrmt, field09A) == 0x09A);
static_assert(offsetof(FlashPrmt, dim1PitchMap) == 0x0A0);
static_assert(offsetof(FlashPrmt, dim2PitchMap) == 0x320);
static_assert(offsetof(FlashPrmt, outputResScale) == 0x5A0);
static_assert(offsetof(FlashPrmt, dim1Flip) == 0x5A2);
static_assert(offsetof(FlashPrmt, dim2Flip) == 0x5A3);
static_assert(offsetof(FlashPrmt, displaySwapXy) == 0x5A4);
static_assert(offsetof(FlashPrmt, dim1Res) == 0x5A6);
static_assert(offsetof(FlashPrmt, dim2Res) == 0x5A8);
static_assert(offsetof(FlashPrmt, dim1ClipLow) == 0x5AE);
static_assert(offsetof(FlashPrmt, dim1ClipHigh) == 0x5AF);
static_assert(offsetof(FlashPrmt, dim2ClipLow) == 0x5B0);
static_assert(offsetof(FlashPrmt, dim2ClipHigh) == 0x5B1);
static_assert(offsetof(FlashPrmt, dim2EdgeLow) == 0x5B2);
static_assert(offsetof(FlashPrmt, dim2EdgeHigh) == 0x5B3);
static_assert(offsetof(FlashPrmt, peakFilterReferenceThreshold) == 0x5B4);
static_assert(offsetof(FlashPrmt, fingerThold) == 0x5B6);
static_assert(offsetof(FlashPrmt, sigRatioToDirectReport) == 0x5BA);
static_assert(offsetof(FlashPrmt, sdZnTholdScaleX16) == 0x5D0);
static_assert(offsetof(FlashPrmt, gloveExitThold) == 0x5D8);
static_assert(offsetof(FlashPrmt, gloveThold) == 0x5DA);
static_assert(offsetof(FlashPrmt, field5FC) == 0x5FC);
static_assert(offsetof(FlashPrmt, field600) == 0x600);
static_assert(offsetof(FlashPrmt, rawForceNormalize) == 0x604);
static_assert(offsetof(FlashPrmt, field608) == 0x608);
static_assert(offsetof(FlashPrmt, cmfTholdNormal) == 0x620);
static_assert(offsetof(FlashPrmt, cmfTholdAlt) == 0x626);
static_assert(offsetof(FlashPrmt, field62C) == 0x62C);
static_assert(offsetof(FlashPrmt, cmfMethodNormal) == 0x810);
static_assert(offsetof(FlashPrmt, cmfMethodSmartCover) == 0x814);
static_assert(offsetof(FlashPrmt, noisyTouchDownDebounce) == 0x818);
static_assert(offsetof(FlashPrmt, gripModeSigThold) == 0x81C);
static_assert(offsetof(FlashPrmt, sigMult) == 0x81E);
static_assert(offsetof(FlashPrmt, sigMultNormal) == 0x81F);
static_assert(offsetof(FlashPrmt, sigMultSD) == 0x820);
static_assert(offsetof(FlashPrmt, absFfZ9) == 0x828);
static_assert(offsetof(FlashPrmt, ffMaxMajorMm) == 0x82C);
static_assert(offsetof(FlashPrmt, absFfMinorMm) == 0x82E);
static_assert(offsetof(FlashPrmt, absFfCentToMinPkRatio) == 0x830);
static_assert(offsetof(FlashPrmt, mfCentToMinPkRatioLow) == 0x831);
static_assert(offsetof(FlashPrmt, peakFilterCentToMinPkRatio) == 0x832);
static_assert(offsetof(FlashPrmt, mfCentToMinPkRatio) == 0x833);
static_assert(offsetof(FlashPrmt, largeObjectSizeMm) == 0x836);
static_assert(offsetof(FlashPrmt, aftEdgeWidthThold) == 0x838);
static_assert(offsetof(FlashPrmt, gripFlags) == 0x83C);
static_assert(offsetof(FlashPrmt, ctdEdge) == 0x840);
static_assert(offsetof(FlashPrmt, centroidSigOffsetDim1) == 0x884);
static_assert(offsetof(FlashPrmt, centroidSigOffsetDim2) == 0x886);
static_assert(offsetof(FlashPrmt, idtSnapDistMm) == 0x888);
static_assert(offsetof(FlashPrmt, accThold) == 0x88A);
static_assert(offsetof(FlashPrmt, aiir) == 0x88E);
static_assert(offsetof(FlashPrmt, field8A0) == 0x8A0);
static_assert(offsetof(FlashPrmt, jitRadiusDisplay) == 0x8A1);
static_assert(offsetof(FlashPrmt, liftOffJitterSuppress) == 0x8A2);
static_assert(offsetof(FlashPrmt, noCurvePredict) == 0x8A3);
static_assert(offsetof(FlashPrmt, rxLineZ8Ratio) == 0x8A4);
static_assert(offsetof(FlashPrmt, rxLineMaxDist) == 0x8A5);
static_assert(offsetof(FlashPrmt, sigSumReserveTouch) == 0x8B4);
static_assert(offsetof(FlashPrmt, gripPeakEdgeDist) == 0x8B9);
static_assert(offsetof(FlashPrmt, gripTimeInterval) == 0x8BA);
static_assert(offsetof(FlashPrmt, maxNotGripTime) == 0x8BC);
static_assert(offsetof(FlashPrmt, gridIirWeight) == 0x8F8);
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
static_assert(offsetof(FlashPrmt, erEdgeMm) == 0x924);
static_assert(offsetof(FlashPrmt, wetFingerMinWaterCells) == 0x93C);
static_assert(offsetof(FlashPrmt, waterSigLowPercent) == 0x93D);
static_assert(offsetof(FlashPrmt, waterSigHighPercent) == 0x93E);
static_assert(offsetof(FlashPrmt, chargerNoiseTholdPercent) == 0x940);
static_assert(offsetof(FlashPrmt, field942) == 0x942);
static_assert(offsetof(FlashPrmt, sensorOffsetNum) == 0x94C);
static_assert(offsetof(FlashPrmt, sensorOffset) == 0x94D);
static_assert(offsetof(FlashPrmt, sensorTranslateNum) == 0xA0D);
static_assert(offsetof(FlashPrmt, sensorTranslate) == 0xA0E);
static_assert(offsetof(FlashPrmt, brokenSensorCompensation) == 0xA86);
static_assert(offsetof(FlashPrmt, rawLowerLimit) == 0x1650);
static_assert(offsetof(FlashPrmt, rawUpperLimit) == 0x1652);
static_assert(offsetof(FlashPrmt, rawDeltaLimit) == 0x1654);
static_assert(offsetof(FlashPrmt, field1656) == 0x1656);
static_assert(offsetof(FlashPrmt, field1658) == 0x1658);
static_assert(offsetof(FlashPrmt, field165A) == 0x165A);
static_assert(offsetof(FlashPrmt, difBrokenCheck) == 0x165C);
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
static_assert(offsetof(FlashPrmt, afeFreqHopMode) == 0x169C);
static_assert(offsetof(FlashPrmt, maxScanRate) == 0x169E);
static_assert(offsetof(FlashPrmt, gameMaxScanRate) == 0x16A0);
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
static_assert(offsetof(FlashPrmt, antiTouchProxMode) == 0x1742);
static_assert(offsetof(FlashPrmt, sensorEnabled) == 0x1744);
static_assert(offsetof(FlashPrmt, sensorRate) == 0x1748);
static_assert(offsetof(FlashPrmt, touchReportFlags) == 0x174C);
static_assert(offsetof(FlashPrmt, toeSynaBl) == 0x1750);
static_assert(offsetof(FlashPrmt, toeSideCols) == 0x1751);
static_assert(offsetof(FlashPrmt, toeSynaBlFlags) == 0x1754);
static_assert(offsetof(FlashPrmt, hotZoneAdjustCore) == 0x1838);
static_assert(offsetof(FlashPrmt, hotZoneMinGap) == 0x183C);
static_assert(offsetof(FlashPrmt, hotZoneGridDim1) == 0x183D);
static_assert(offsetof(FlashPrmt, hotZoneGridDim2) == 0x183E);
static_assert(offsetof(FlashPrmt, hotZoneSizeNum) == 0x183F);
static_assert(offsetof(FlashPrmt, hotZoneSize) == 0x1840);
static_assert(offsetof(FlashPrmt, hotZoneExpandMax) == 0x1854);
static_assert(offsetof(FlashPrmt, hotZoneExpandMin) == 0x185C);

/// g_tsaPrmtFlashConst：由机型参数推导出的常量（ConstPrmt::Init；0x40..0x56 由 ER_Init 写）。
struct FlashConstPrmt {
    uint16_t field000;
    uint8_t reserved002[0x2];
    uint32_t dim1DisplayScale;  // 显示坐标（含两端裁剪）对 Q8.8 像素坐标的比例，Q16
    uint32_t dim2DisplayScale;
    uint16_t aiirCoefToDeltaRatio[3];  // 每组 AIIR 系数的插值斜率，Q10
    uint16_t jitRadius;  // 抗抖动半径（Q8.8 传感器单位，按 Dim1 分辨率换算）；Filter 两个方向都用它
    uint16_t jitRadiusDim2;  // 按 Dim2 分辨率换算的抗抖动半径（无人读）
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
    ErEdgePrmt erEdge[4];  // ER_Init 由 FlashPrmt::erEdgeMm 换算
    uint8_t reserved058[0x2];
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
static_assert(offsetof(FlashConstPrmt, jitRadius) == 0x012);
static_assert(offsetof(FlashConstPrmt, jitRadiusDim2) == 0x014);
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
static_assert(offsetof(FlashConstPrmt, erEdge) == 0x040);
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
    uint16_t gameClickDuration;  // 游戏场景的 clickDuration
    uint8_t clickSuppressDistance;  // 外部像素
    uint8_t gameClickSuppressDistance;
    uint8_t maxClickSuppressDistance;
    uint8_t gameMaxClickSuppressDistance;
    uint8_t flickDistance;
    uint8_t reserved07D[0x1];
    uint8_t clickToFlickStep;  // 点击转滑动后每帧收回的点击偏移（外部像素）
    uint8_t reserved07F[0x1];
    uint8_t zTholdFlags;  // bit1 抬起门限按区域分支；bit2 手套、bit3 指关节模式用按下门限判稳定
    uint8_t touchModeFlags;  // bit3：TouchMode 先试指关节（3）再试手指（1）
    uint8_t reserved082[0x2];
    uint32_t gloveFilterSides;
    uint16_t distToEnterGloveMode;
    uint8_t reserved08A[0x2];
    int16_t chargerNoiseTholdPercent;
    uint16_t freqNoisyValidMs;  // 频点在启用后超过这么久才变噪时仍可再次选用
    uint8_t reserved090[0x6];
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
static_assert(offsetof(DynamicPrmt, gameClickDuration) == 0x076);
static_assert(offsetof(DynamicPrmt, clickSuppressDistance) == 0x078);
static_assert(offsetof(DynamicPrmt, gameClickSuppressDistance) == 0x079);
static_assert(offsetof(DynamicPrmt, maxClickSuppressDistance) == 0x07A);
static_assert(offsetof(DynamicPrmt, gameMaxClickSuppressDistance) == 0x07B);
static_assert(offsetof(DynamicPrmt, flickDistance) == 0x07C);
static_assert(offsetof(DynamicPrmt, clickToFlickStep) == 0x07E);
static_assert(offsetof(DynamicPrmt, zTholdFlags) == 0x080);
static_assert(offsetof(DynamicPrmt, touchModeFlags) == 0x081);
static_assert(offsetof(DynamicPrmt, gloveFilterSides) == 0x084);
static_assert(offsetof(DynamicPrmt, distToEnterGloveMode) == 0x088);
static_assert(offsetof(DynamicPrmt, chargerNoiseTholdPercent) == 0x08C);
static_assert(offsetof(DynamicPrmt, freqNoisyValidMs) == 0x08E);
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
