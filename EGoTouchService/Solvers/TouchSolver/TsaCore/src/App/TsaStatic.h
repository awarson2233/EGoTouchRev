#pragma once
// 系统级运行状态（TSA_Static）与每帧统计的全局属性（TSA_Prpt）。只列已改写的模块用到的字段。

#include <cstddef>
#include <cstdint>

#include "Common/Image.h"
#include "Common/Types.h"

namespace Solvers::Tsa::Core {

/// Exception 的一组状态。Hybrid Incell 与 Full Incell 各有一组，按 FlashPrmt::sensorArchitecture 选用。
struct ExceptionState {
    uint16_t debounceFrames;  // 本帧检出的异常与上一帧相同的连续帧数
    uint8_t reserved002[0x6];
    uint64_t noiseSettleCount;  // 噪声异常之外的帧逐帧累加 frameIntervalMs，超过 0x52 时噪声等级清零
    uint32_t lastDetected;  // 上一帧检出的异常；强制进入水下、噪声等异常时为强制的值
    uint8_t wetFinger;  // FI：本帧有区域判为湿手指
    uint8_t reserved015[0x3];
    uint32_t detected;  // 本帧检出的异常（各区域与水下检测取最大），Exception_Process 末尾清零
    uint8_t reserved01C[0x2];
    int16_t field01E;  // HIException 写（被裁掉），RxLineFilter 读
};
static_assert(sizeof(ExceptionState) == 0x020);
static_assert(offsetof(ExceptionState, debounceFrames) == 0x000);
static_assert(offsetof(ExceptionState, noiseSettleCount) == 0x008);
static_assert(offsetof(ExceptionState, lastDetected) == 0x010);
static_assert(offsetof(ExceptionState, wetFinger) == 0x014);
static_assert(offsetof(ExceptionState, detected) == 0x018);
static_assert(offsetof(ExceptionState, field01E) == 0x01E);

/// HardwareAnalyzer 每帧对 raw2raw Dif（本帧 raw - 上一帧 raw）的统计，按扫描线分组。
struct Raw2RawPrpt {
    int16_t maxDif;
    int16_t minDif;
    uint16_t overNoiseCount;  // |Dif| 超过噪声门限的格数
    uint16_t overBigNoiseCount;  // 再超过 3 倍 peakFilterReferenceThreshold 的格数
    uint16_t overNoiseAfterCmf;  // CMF 后 |Dif| 仍超过噪声门限的格数（只在 sensorArchitecture 2 下统计）
    uint16_t overHtiCount;  // 超过噪声门限且超过 peakFilterReferenceThreshold 的格数
    uint16_t overLtiCount;  // 超过噪声门限且超过 fingerThold 的格数
    uint8_t reserved00E[0x2];
    int32_t posSum;  // 超过噪声门限的正 Dif 之和
    int32_t negSum;  // 超过噪声门限的非正 Dif 之和
    uint8_t maxOverNoisePercent;  // 各组扫描线里超噪声格占比的最大值（%）
    uint8_t reserved019[0x3];
};
static_assert(sizeof(Raw2RawPrpt) == 0x01C);
static_assert(offsetof(Raw2RawPrpt, maxDif) == 0x000);
static_assert(offsetof(Raw2RawPrpt, minDif) == 0x002);
static_assert(offsetof(Raw2RawPrpt, overNoiseCount) == 0x004);
static_assert(offsetof(Raw2RawPrpt, overBigNoiseCount) == 0x006);
static_assert(offsetof(Raw2RawPrpt, overNoiseAfterCmf) == 0x008);
static_assert(offsetof(Raw2RawPrpt, overHtiCount) == 0x00A);
static_assert(offsetof(Raw2RawPrpt, overLtiCount) == 0x00C);
static_assert(offsetof(Raw2RawPrpt, posSum) == 0x010);
static_assert(offsetof(Raw2RawPrpt, negSum) == 0x014);
static_assert(offsetof(Raw2RawPrpt, maxOverNoisePercent) == 0x018);

/// DifZ8 坏点的候选：同一格持续被判时计时，到 0x683 ms 即确认为坏点（计时置 5000）。
struct BrokenSensorZ8 {
    uint8_t dim1;
    uint8_t dim2;
    uint8_t reserved002[0x6];
    int64_t timerMs;
};
static_assert(sizeof(BrokenSensorZ8) == 0x010);
static_assert(offsetof(BrokenSensorZ8, dim1) == 0x000);
static_assert(offsetof(BrokenSensorZ8, dim2) == 0x001);
static_assert(offsetof(BrokenSensorZ8, timerMs) == 0x008);

/// TsaPrpt 里一个方向的线统计极值（Self_GetPropertyDim），下标相对该方向的第一条线。
struct SelfPrpt {
    int32_t maxSum;
    uint8_t maxSumIdx;
    uint8_t reserved005[0x3];
    int32_t minSum;
    uint8_t minSumIdx;
    uint8_t reserved00D[0x3];
    int32_t maxPosSum;
    uint8_t maxPosSumIdx;
    uint8_t reserved015[0x3];
    int32_t minNegSum;
    uint8_t minNegSumIdx;
    uint8_t reserved01D[0x3];
    int32_t strongSumTotal;  // 各线 strongSum 之和
};
static_assert(sizeof(SelfPrpt) == 0x024);
static_assert(offsetof(SelfPrpt, maxSum) == 0x000);
static_assert(offsetof(SelfPrpt, maxSumIdx) == 0x004);
static_assert(offsetof(SelfPrpt, minSum) == 0x008);
static_assert(offsetof(SelfPrpt, minSumIdx) == 0x00C);
static_assert(offsetof(SelfPrpt, maxPosSum) == 0x010);
static_assert(offsetof(SelfPrpt, maxPosSumIdx) == 0x014);
static_assert(offsetof(SelfPrpt, minNegSum) == 0x018);
static_assert(offsetof(SelfPrpt, minNegSumIdx) == 0x01C);
static_assert(offsetof(SelfPrpt, strongSumTotal) == 0x020);

/// g_tsaStaticGrid（经 g_tsaStaticPtr 选用）：系统级运行状态。
struct TsaStatic {
    uint8_t pressCount;  // TSAOut：本帧报出的触点里刚按下（volatileFlags bit2）的个数
    uint8_t prevPressCount;
    uint8_t okToIdle;  // 本帧 TSA_IsOKToEnterIDLEMode 的结果
    uint8_t afeIdle;  // AFE 处于 IDLE 模式（TSAStatic_PreProcess 每帧取）
    uint8_t prevAfeIdle;
    uint8_t afeSideIdle;
    uint8_t prevAfeSideIdle;
    uint8_t idleReady;  // 可以进 IDLE（TSA_UpdateIDLEModeInternally）
    uint8_t prevIdleReady;
    uint8_t idleRequest;  // 对外请求进 IDLE：idleReady 持续 idleEnterDelayMs 之后
    uint8_t sideIdleReady;
    uint8_t prevSideIdleReady;
    uint8_t sideIdleRequest;
    uint8_t allFreqNoisy;  // 所有频点都噪声大（充电器噪声，或显示噪声大）
    uint8_t prevAllFreqNoisy;
    uint8_t noisyBlReset;  // 上一帧的 blNoisyReset
    uint8_t prevNoisyBlReset;
    uint8_t reserved011[0x7];
    int64_t afeIdleExitTime;  // AFE 刚退出 IDLE 的时间（ms），-1 为无；之后 100 ms 内要求 AFE 保持退出
    int64_t afeSideIdleExitTime;
    int64_t idleReadyTime;  // idleReady 成立的时间（ms），-1 为不成立
    int64_t sideIdleReadyTime;
    int64_t field038;
    int64_t proxScreenOffTime;  // AFE 进入接近灭屏状态的时间（ms），-1 为不在
    int64_t field048;
    int64_t proximityOnTime;  // 接近检测启用的时间（ms），-1 为未启用
    int64_t proximityOffTime;  // 接近检测关闭的时间（ms）
    int64_t chargerTime;  // 最近一次检出充电器的时间（ms）：之后 2 s 内仍按插着处理
    int64_t phoneCallTime;  // 最近一次处于通话的时间（ms）：之后 1.2 s 内接近检测启用时仍按通话处理
    int64_t lastReportTime;  // 最近一次报点（或请求 AFE 复位）的时间（ms）
    uint64_t fullIncellHighPeakTime;  // 最大峰持续超过 3 倍 finger 门限时的时间（ms）
    uint64_t msInitTime;  // 互容主流程初始化的时间（ms）；只有系统复位才更新
    uint64_t autoCalibrationStartTime;  // AFE 开始自动校准的时间（ms）
    uint64_t markedLiftOffTime;  // 带 persistFlags 0x20000000 的触点抬起时的时间（ms）
    Point markedLiftOffPos;  // 以及它的位置
    uint32_t activeReason;  // 本帧不能进 IDLE 的原因位（只写，供日志）
    uint8_t fullIncellHighPeakFrames;  // 最大峰超过上述门限的连续帧数（到 6 为止）
    uint8_t resetHoldFrames;  // 系统复位请求之后，复位位再保持的帧数
    uint8_t reserved0A2[0x2];
    uint32_t specialControlFlags;
    uint8_t field0A8;
    uint8_t safeDif;  // TSAStatic_SafeDiffCheck：Dif 可信
    uint8_t unsafeDif;  // Dif 可疑（异常形状的触点多）
    uint8_t reserved0AB[0x1];
    int32_t trustedTouchCount;  // TouchUnit::trustTag 为 1 的累计次数
    int32_t suspectTouchCount;  // trustTag 为 2 的累计次数
    uint8_t reserved0B4[0x4];
    int32_t frameCounter;
    uint32_t systemFlags;
    uint32_t prevSystemFlags;
    uint8_t reserved0C4[0x4];
    int64_t field0C8;
    int64_t prevField0C8;
    uint32_t afeStatus;  // AFE 状态位：bit17 raw 跳变、bit18 raw 不稳定、bit19 侧边触摸、bit20 不查脏基线
    uint32_t prevAfeStatus;
    uint64_t curTimeMs;  // 本帧时间（ms）
    uint64_t prevTimeMs;  // 上一帧时间（ms）
    uint8_t reserved0F0[0x8];
    int16_t frameIntervalMs;  // 与上一帧的格式化时间间隔（ms），各模块的计时器按它累加
    uint8_t reserved0FA[0x2];
    uint32_t featureFlags;
    uint32_t prevFeatureFlags;
    uint32_t extFeatureFlagsInit;  // 外部可开关的特性位（featureFlags & kFeatExtMask）的初值
    uint32_t featureFlags2;
    uint32_t processingState;  // bit31：互容主流程进行中；bit4：峰 Zn 只取十字五点和
    uint8_t field110;  // 全局异常为 0x40 / 0x80 时置 1
    uint8_t noiseLevel;  // 噪声等级：异常 0x100 时 2、0x8000 时 3，噪声异常结束一段时间后清零
    uint8_t reserved112[0x2];
    uint8_t fullIncellSignalDisparity;  // 整屏 incell 信号失衡（峰检测按 SD 模式处理）
    uint8_t justExitedSpecialCtrlMode;  // 刚退出特殊控制模式
    uint8_t field116;
    uint8_t resetFlagsCleared;  // systemFlags 的复位位本帧刚清掉
    uint8_t resetJustCleared;  // 复位位刚清掉，等本帧是否噪声复位基线
    uint8_t noisyResetAfterReset;  // 复位位清掉之后紧接着噪声复位了基线
    uint8_t signalReverted;  // 信号反向（TSAStatic_IsSignalReverted）
    uint8_t field11B;
    uint8_t cmfBlocked;  // 本帧不做 CMF（只存 CMF 前的 Dif）
    uint8_t reserved11D[0x1];
    uint16_t rfNoiseFrames;  // 连续检出射频噪声（异常 1）的帧数
    uint16_t maxRfNoiseFrames;
    uint16_t strongTapCount;  // TE：报过点的触点抬起时峰值超过 2 倍 finger 门限、长轴小于 21 mm 的连续次数（到 5 为止）
    int32_t field124;
    int32_t field128;
    int32_t field12C;
    int32_t field130;
    int32_t field134;
    int32_t field138;
    uint8_t reserved13C[0x4];
    void* sensorDataCallback;
    int64_t field148;
    uint8_t reserved150[0x4];
    uint32_t aftGateFlags;  // bit3：查环境温度异常（否则只对 HIMA 参数查）
    uint32_t prevAftGateFlags;
    uint8_t reserved15C[0x108];
    int32_t screenRotation;  // 0 / 1 / 2 / 3：0° / 90° / 180° / 270°
    int32_t field268;
    int32_t field26C;
    int32_t field270;
    uint32_t scenarioMask;  // 当前场景位，与 FlashPrmt 的各 *Scenarios 相与
    uint32_t prevExtFlags;
    uint32_t extFlags;  // 本帧 TouchTable::extFlags
    uint32_t newExtFlags;  // 本帧新出现的 extFlags 位；bit21：刚结束硬件复位
    int32_t field284;
    int32_t envTemp;
    int32_t field28C;
    uint8_t field290;
    uint8_t reserved291[0x1];
    int16_t htiThold;  // 可信触点峰值的跟踪值（不超过 peakFilterReferenceThreshold），GetSigThold(1) 取它的 2/5
    uint8_t frameDone;
    uint8_t field295;
    uint8_t reserved296[0x2];
    int32_t field298;
    int32_t field29C;
    int32_t field2A0;
    char logName[0x32];
    uint8_t reserved2D6[0xA];
    uint16_t xMin;  // 对外 X 坐标的范围
    int16_t xMax;
    uint16_t yMin;
    int16_t yMax;
    uint8_t blResetLatched;  // BLReset：收到过硬件复位，Init 带参数时清零
    uint8_t recalFrames;  // BLRecal：系统复位后的帧数（到 15 为止）
    uint8_t recalBoundaryHits;  // 其中 raw 边界越界的帧数
    uint8_t blResetCheckEnabled;
    uint8_t blResetDebounce;  // BLReset：唤醒后异常触点的连续帧数，非 0 即在防抖中
    uint8_t reserved2ED[0x3];
    int64_t blWakeUpTimer;  // 唤醒后异常触点的累计时间（ms）
    uint8_t blResetStatus;  // 本帧是否触发了基线复位（BLReset_Status）
    uint8_t blNoisyReset;  // 噪声期间触发过基线复位
    uint8_t reserved2FA[0x6];
    uint64_t blNoisyTimer;  // 噪声期间基线复位的计时（ms），0 为未计时
    uint8_t reserved308[0x2];
    uint8_t rawJumpLatched;  // 检出过大面积 raw 跳变
    uint8_t reserved30B[0x5];
    int64_t rawJumpTimer;
    uint8_t rawJumpDetected;
    uint8_t reserved319[0x7];
    uint64_t blResetRefTime;  // 系统复位时的时间（ms）
    uint8_t ssNoSignal;
    uint8_t reserved329[0x7];
    int64_t ssNoSignalTimer;
    uint8_t ssNegSignal;
    uint8_t reserved339[0x7];
    int64_t ssNegSignalTimer;
    uint8_t ssNegSignalStable;
    uint8_t reserved349[0x7];
    int64_t ssNegSignalStableTimer;
    uint8_t reserved358[0x8];
    int64_t field360;
    uint64_t smartCoverOffTimer;  // 合上智能皮套后超噪声格数少的累计时间（ms）
    uint64_t abnormalType1Timer;  // 三类基线异常各自的累计时间（ms）
    uint64_t abnormalType2Timer;
    uint64_t abnormalType3Timer;
    uint64_t dirtyTimer;  // 当前基线判脏的累计时间（ms）
    int64_t afeRawJumpTimer;  // AFE raw 跳变后的计时：-1 为跳变已处理、等 AFE 清标志
    uint64_t toeSideRawTime;  // ToeSynaBl：两侧列 raw 最近一次判为受保护的时间（ms）
    uint64_t toeSideRawTime2;
    uint64_t toeSsNotMatchTimer;
    uint64_t toeAbnormalShapeTimer;
    uint32_t toeWaterTimer;
    int32_t toeClothRiskTimer;
    uint32_t toeBackLightShiftTimer;
    int32_t toeSsNoSignalTimer;
    uint64_t toeGripRegionTime;  // 两侧握持区域最近一次判为正常的时间（ms）
    int64_t field3D0;
    uint32_t cleanState;  // 干净基线状态机（BLReset_BaselineStateMachine），0x10 为已得到干净基线
    uint8_t blSuspect;  // 状态机判过基线可疑
    uint8_t reserved3DD[0x3];
    uint64_t cleanTimer;
    uint16_t cleanFrames;
    uint8_t cleanBaselineValid;  // cleanBaseline 缓冲里存着干净基线
    uint8_t blUpdatedThisFrame;
    uint8_t singleTouchLatch;  // BLSM 阶段 5：上一帧恰有一个未贴边的触点
    uint8_t resetDiffRequested;  // 本帧基线复位，Dif 清零
    uint8_t blIirUpdated;  // 本帧基线按 IIR 步进过
    uint8_t blHwResetRequest;  // 请求按硬件复位处理基线
    uint8_t blRefreshRequest;  // 请求刷新基线属性
    uint8_t field3F1;
    uint8_t reserved3F2[0x2];
    uint32_t blDebugFlags;  // BLReset 各判定的命中位（只写，供调试）
    uint8_t reserved3F8[0x2];
    uint8_t strongFingerFrames;  // TouchMode：手指模式下全局峰 Zn 超过 3 倍手套退出门限的连续帧数
    uint8_t field3FB;
    uint8_t vpAllowed;  // TZ：允许插入虚拟峰（区域够长时置 1，连续 5 帧不够长后清 0）
    uint8_t vpDenyFrames;  // 区域不够长的连续帧数（到 5 为止）
    uint8_t reserved3FE[0xA];
    ExceptionState hiException;  // Hybrid Incell 用（本机型 HIException 被裁掉）
    uint8_t reserved428[0x8];
    ExceptionState fiException;  // Full Incell 用
};
static_assert(sizeof(TsaStatic) == 0x450);
static_assert(offsetof(TsaStatic, pressCount) == 0x000);
static_assert(offsetof(TsaStatic, prevPressCount) == 0x001);
static_assert(offsetof(TsaStatic, okToIdle) == 0x002);
static_assert(offsetof(TsaStatic, afeIdle) == 0x003);
static_assert(offsetof(TsaStatic, prevAfeIdle) == 0x004);
static_assert(offsetof(TsaStatic, afeSideIdle) == 0x005);
static_assert(offsetof(TsaStatic, prevAfeSideIdle) == 0x006);
static_assert(offsetof(TsaStatic, idleReady) == 0x007);
static_assert(offsetof(TsaStatic, prevIdleReady) == 0x008);
static_assert(offsetof(TsaStatic, idleRequest) == 0x009);
static_assert(offsetof(TsaStatic, sideIdleReady) == 0x00A);
static_assert(offsetof(TsaStatic, prevSideIdleReady) == 0x00B);
static_assert(offsetof(TsaStatic, sideIdleRequest) == 0x00C);
static_assert(offsetof(TsaStatic, allFreqNoisy) == 0x00D);
static_assert(offsetof(TsaStatic, prevAllFreqNoisy) == 0x00E);
static_assert(offsetof(TsaStatic, noisyBlReset) == 0x00F);
static_assert(offsetof(TsaStatic, prevNoisyBlReset) == 0x010);
static_assert(offsetof(TsaStatic, afeIdleExitTime) == 0x018);
static_assert(offsetof(TsaStatic, afeSideIdleExitTime) == 0x020);
static_assert(offsetof(TsaStatic, idleReadyTime) == 0x028);
static_assert(offsetof(TsaStatic, sideIdleReadyTime) == 0x030);
static_assert(offsetof(TsaStatic, field038) == 0x038);
static_assert(offsetof(TsaStatic, proxScreenOffTime) == 0x040);
static_assert(offsetof(TsaStatic, field048) == 0x048);
static_assert(offsetof(TsaStatic, proximityOnTime) == 0x050);
static_assert(offsetof(TsaStatic, proximityOffTime) == 0x058);
static_assert(offsetof(TsaStatic, chargerTime) == 0x060);
static_assert(offsetof(TsaStatic, phoneCallTime) == 0x068);
static_assert(offsetof(TsaStatic, lastReportTime) == 0x070);
static_assert(offsetof(TsaStatic, fullIncellHighPeakTime) == 0x078);
static_assert(offsetof(TsaStatic, msInitTime) == 0x080);
static_assert(offsetof(TsaStatic, autoCalibrationStartTime) == 0x088);
static_assert(offsetof(TsaStatic, markedLiftOffTime) == 0x090);
static_assert(offsetof(TsaStatic, markedLiftOffPos) == 0x098);
static_assert(offsetof(TsaStatic, activeReason) == 0x09C);
static_assert(offsetof(TsaStatic, fullIncellHighPeakFrames) == 0x0A0);
static_assert(offsetof(TsaStatic, resetHoldFrames) == 0x0A1);
static_assert(offsetof(TsaStatic, specialControlFlags) == 0x0A4);
static_assert(offsetof(TsaStatic, field0A8) == 0x0A8);
static_assert(offsetof(TsaStatic, safeDif) == 0x0A9);
static_assert(offsetof(TsaStatic, unsafeDif) == 0x0AA);
static_assert(offsetof(TsaStatic, trustedTouchCount) == 0x0AC);
static_assert(offsetof(TsaStatic, suspectTouchCount) == 0x0B0);
static_assert(offsetof(TsaStatic, frameCounter) == 0x0B8);
static_assert(offsetof(TsaStatic, systemFlags) == 0x0BC);
static_assert(offsetof(TsaStatic, prevSystemFlags) == 0x0C0);
static_assert(offsetof(TsaStatic, field0C8) == 0x0C8);
static_assert(offsetof(TsaStatic, prevField0C8) == 0x0D0);
static_assert(offsetof(TsaStatic, afeStatus) == 0x0D8);
static_assert(offsetof(TsaStatic, prevAfeStatus) == 0x0DC);
static_assert(offsetof(TsaStatic, curTimeMs) == 0x0E0);
static_assert(offsetof(TsaStatic, prevTimeMs) == 0x0E8);
static_assert(offsetof(TsaStatic, frameIntervalMs) == 0x0F8);
static_assert(offsetof(TsaStatic, featureFlags) == 0x0FC);
static_assert(offsetof(TsaStatic, prevFeatureFlags) == 0x100);
static_assert(offsetof(TsaStatic, extFeatureFlagsInit) == 0x104);
static_assert(offsetof(TsaStatic, featureFlags2) == 0x108);
static_assert(offsetof(TsaStatic, processingState) == 0x10C);
static_assert(offsetof(TsaStatic, field110) == 0x110);
static_assert(offsetof(TsaStatic, noiseLevel) == 0x111);
static_assert(offsetof(TsaStatic, fullIncellSignalDisparity) == 0x114);
static_assert(offsetof(TsaStatic, justExitedSpecialCtrlMode) == 0x115);
static_assert(offsetof(TsaStatic, field116) == 0x116);
static_assert(offsetof(TsaStatic, resetFlagsCleared) == 0x117);
static_assert(offsetof(TsaStatic, resetJustCleared) == 0x118);
static_assert(offsetof(TsaStatic, noisyResetAfterReset) == 0x119);
static_assert(offsetof(TsaStatic, signalReverted) == 0x11A);
static_assert(offsetof(TsaStatic, field11B) == 0x11B);
static_assert(offsetof(TsaStatic, cmfBlocked) == 0x11C);
static_assert(offsetof(TsaStatic, rfNoiseFrames) == 0x11E);
static_assert(offsetof(TsaStatic, maxRfNoiseFrames) == 0x120);
static_assert(offsetof(TsaStatic, strongTapCount) == 0x122);
static_assert(offsetof(TsaStatic, field124) == 0x124);
static_assert(offsetof(TsaStatic, field128) == 0x128);
static_assert(offsetof(TsaStatic, field12C) == 0x12C);
static_assert(offsetof(TsaStatic, field130) == 0x130);
static_assert(offsetof(TsaStatic, field134) == 0x134);
static_assert(offsetof(TsaStatic, field138) == 0x138);
static_assert(offsetof(TsaStatic, sensorDataCallback) == 0x140);
static_assert(offsetof(TsaStatic, field148) == 0x148);
static_assert(offsetof(TsaStatic, aftGateFlags) == 0x154);
static_assert(offsetof(TsaStatic, prevAftGateFlags) == 0x158);
static_assert(offsetof(TsaStatic, screenRotation) == 0x264);
static_assert(offsetof(TsaStatic, field268) == 0x268);
static_assert(offsetof(TsaStatic, field26C) == 0x26C);
static_assert(offsetof(TsaStatic, field270) == 0x270);
static_assert(offsetof(TsaStatic, scenarioMask) == 0x274);
static_assert(offsetof(TsaStatic, prevExtFlags) == 0x278);
static_assert(offsetof(TsaStatic, extFlags) == 0x27C);
static_assert(offsetof(TsaStatic, newExtFlags) == 0x280);
static_assert(offsetof(TsaStatic, field284) == 0x284);
static_assert(offsetof(TsaStatic, envTemp) == 0x288);
static_assert(offsetof(TsaStatic, field28C) == 0x28C);
static_assert(offsetof(TsaStatic, field290) == 0x290);
static_assert(offsetof(TsaStatic, htiThold) == 0x292);
static_assert(offsetof(TsaStatic, frameDone) == 0x294);
static_assert(offsetof(TsaStatic, field295) == 0x295);
static_assert(offsetof(TsaStatic, field298) == 0x298);
static_assert(offsetof(TsaStatic, field29C) == 0x29C);
static_assert(offsetof(TsaStatic, field2A0) == 0x2A0);
static_assert(offsetof(TsaStatic, logName) == 0x2A4);
static_assert(offsetof(TsaStatic, xMin) == 0x2E0);
static_assert(offsetof(TsaStatic, xMax) == 0x2E2);
static_assert(offsetof(TsaStatic, yMin) == 0x2E4);
static_assert(offsetof(TsaStatic, yMax) == 0x2E6);
static_assert(offsetof(TsaStatic, blResetLatched) == 0x2E8);
static_assert(offsetof(TsaStatic, recalFrames) == 0x2E9);
static_assert(offsetof(TsaStatic, recalBoundaryHits) == 0x2EA);
static_assert(offsetof(TsaStatic, blResetCheckEnabled) == 0x2EB);
static_assert(offsetof(TsaStatic, blResetDebounce) == 0x2EC);
static_assert(offsetof(TsaStatic, blWakeUpTimer) == 0x2F0);
static_assert(offsetof(TsaStatic, blResetStatus) == 0x2F8);
static_assert(offsetof(TsaStatic, blNoisyReset) == 0x2F9);
static_assert(offsetof(TsaStatic, blNoisyTimer) == 0x300);
static_assert(offsetof(TsaStatic, rawJumpLatched) == 0x30A);
static_assert(offsetof(TsaStatic, rawJumpTimer) == 0x310);
static_assert(offsetof(TsaStatic, rawJumpDetected) == 0x318);
static_assert(offsetof(TsaStatic, blResetRefTime) == 0x320);
static_assert(offsetof(TsaStatic, ssNoSignal) == 0x328);
static_assert(offsetof(TsaStatic, ssNoSignalTimer) == 0x330);
static_assert(offsetof(TsaStatic, ssNegSignal) == 0x338);
static_assert(offsetof(TsaStatic, ssNegSignalTimer) == 0x340);
static_assert(offsetof(TsaStatic, ssNegSignalStable) == 0x348);
static_assert(offsetof(TsaStatic, ssNegSignalStableTimer) == 0x350);
static_assert(offsetof(TsaStatic, field360) == 0x360);
static_assert(offsetof(TsaStatic, smartCoverOffTimer) == 0x368);
static_assert(offsetof(TsaStatic, abnormalType1Timer) == 0x370);
static_assert(offsetof(TsaStatic, abnormalType2Timer) == 0x378);
static_assert(offsetof(TsaStatic, abnormalType3Timer) == 0x380);
static_assert(offsetof(TsaStatic, dirtyTimer) == 0x388);
static_assert(offsetof(TsaStatic, afeRawJumpTimer) == 0x390);
static_assert(offsetof(TsaStatic, toeSideRawTime) == 0x398);
static_assert(offsetof(TsaStatic, toeSideRawTime2) == 0x3A0);
static_assert(offsetof(TsaStatic, toeSsNotMatchTimer) == 0x3A8);
static_assert(offsetof(TsaStatic, toeAbnormalShapeTimer) == 0x3B0);
static_assert(offsetof(TsaStatic, toeWaterTimer) == 0x3B8);
static_assert(offsetof(TsaStatic, toeClothRiskTimer) == 0x3BC);
static_assert(offsetof(TsaStatic, toeBackLightShiftTimer) == 0x3C0);
static_assert(offsetof(TsaStatic, toeSsNoSignalTimer) == 0x3C4);
static_assert(offsetof(TsaStatic, toeGripRegionTime) == 0x3C8);
static_assert(offsetof(TsaStatic, field3D0) == 0x3D0);
static_assert(offsetof(TsaStatic, cleanState) == 0x3D8);
static_assert(offsetof(TsaStatic, blSuspect) == 0x3DC);
static_assert(offsetof(TsaStatic, cleanTimer) == 0x3E0);
static_assert(offsetof(TsaStatic, cleanFrames) == 0x3E8);
static_assert(offsetof(TsaStatic, cleanBaselineValid) == 0x3EA);
static_assert(offsetof(TsaStatic, blUpdatedThisFrame) == 0x3EB);
static_assert(offsetof(TsaStatic, singleTouchLatch) == 0x3EC);
static_assert(offsetof(TsaStatic, resetDiffRequested) == 0x3ED);
static_assert(offsetof(TsaStatic, blIirUpdated) == 0x3EE);
static_assert(offsetof(TsaStatic, blHwResetRequest) == 0x3EF);
static_assert(offsetof(TsaStatic, blRefreshRequest) == 0x3F0);
static_assert(offsetof(TsaStatic, field3F1) == 0x3F1);
static_assert(offsetof(TsaStatic, blDebugFlags) == 0x3F4);
static_assert(offsetof(TsaStatic, strongFingerFrames) == 0x3FA);
static_assert(offsetof(TsaStatic, field3FB) == 0x3FB);
static_assert(offsetof(TsaStatic, vpAllowed) == 0x3FC);
static_assert(offsetof(TsaStatic, vpDenyFrames) == 0x3FD);
static_assert(offsetof(TsaStatic, hiException) == 0x408);
static_assert(offsetof(TsaStatic, fiException) == 0x430);

/// g_tsaPrpt：每帧统计的全局属性。
struct TsaPrpt {
    Raw2RawPrpt raw2Raw;
    Raw2RawPrpt prevRaw2Raw;
    uint8_t brokenSensorNum;  // brokenSensor 的有效项数
    uint8_t brokenByDifZ8;  // 本帧有 DifZ8 坏点被确认
    uint8_t brokenByDif;  // 有格按 Dif 判为坏点
    uint8_t reserved03B[0x5];
    BrokenSensorZ8 brokenSensor[5];
    uint8_t reserved090[0x2];
    uint16_t boundaryPositive;  // raw 的上下边界
    uint16_t boundaryNegative;
    uint16_t rawMax;
    uint16_t rawMin;
    uint16_t field09A;
    uint8_t boundaryPositiveDim1;  // boundaryPositive / Negative 所在的格
    uint8_t boundaryPositiveDim2;
    uint8_t boundaryNegativeDim1;
    uint8_t boundaryNegativeDim2;
    int16_t sigPeakPositive;  // Dif 的正向极值
    int16_t sigPeakNegative;  // Dif 的负向极值
    uint8_t specialThreshCol;  // 用特殊门限的列
    uint8_t specialThreshRow;
    uint8_t nextSideThreshCol;
    uint8_t nextSideThreshRow;
    uint8_t reserved0A8[0x4];
    int32_t field0AC;
    int32_t field0B0;
    uint8_t reserved0B4[0x4];
    uint16_t field0B8;
    uint8_t reserved0BA[0x2];
    int16_t underWaterCells;  // Dif 落在水下门限区间内的格数
    int16_t underWaterNegCells;  // -Dif 落在该区间内的格数
    int16_t field0C0;
    int16_t field0C2;
    uint8_t reserved0C4[0x4];
    int32_t field0C8;
    uint8_t reserved0CC[0x14];
    int16_t field0E0;
    int16_t field0E2;
    uint8_t reserved0E4[0x7C];
    int16_t preCmfMaxAbsDif;  // TSAPrpt_GetDifPreCMFPrpt：CMF 前 Dif 的最大绝对值
    uint16_t preCmfOverCount;  // CMF 前 |Dif| 不低于门限的格数
    uint8_t reserved164[0x8];
    int16_t field16C;
    int16_t field16E;
    uint8_t field170;
    uint8_t field171;
    uint8_t field172;
    uint8_t field173;
    uint8_t reserved174[0x4];
    int32_t field178;
    int32_t field17C;
    uint16_t field180;
    uint16_t field182;
    uint16_t field184;
    uint16_t field186;
    uint8_t reserved188[0x4];
    int16_t field18C;
    int16_t field18E;
    uint8_t reserved190[0x10];
    uint16_t field1A0;
    uint16_t field1A2;
    uint8_t reserved1A4[0x20];
    uint16_t field1C4;
    uint8_t reserved1C6[0x66];
    SelfPrpt selfCols;  // 各列的线统计极值
    SelfPrpt selfRows;  // 各行的线统计极值
    uint8_t reserved274[0x4];
    uint32_t blStage;  // BLSM 阶段
    uint32_t prevBlStage;
    uint32_t blPropertyFlags;  // BLSM_GetProperty 本帧的基线属性（kBlProp*）
    uint16_t blResetFrames;  // BLReset：两个封顶的帧计数，后者在复位时清零
    uint16_t blSinceResetFrames;
    int32_t blStageFrames;  // 当前阶段的帧数
    uint8_t reserved28C[0x4];
    uint64_t blStageTime;  // 当前阶段的累计时间（ms）
    uint8_t reserved298[0x2];
    uint16_t field29A;
    uint8_t allFreqInitCount;  // 抬起期间按各频点初始化基线的次数（到 3 为止）
    uint8_t reserved29D[0x3];
    uint32_t blPropertyState;  // 基线复位的原因
    uint32_t prevBlPropertyState;
    uint32_t shbState;  // SHB 抓取状态机
    uint8_t reserved2AC[0x4];
    uint64_t shbStateStartTime;
    uint32_t shbFlags;
    uint8_t reserved2BC[0xC];
    uint64_t tzAgeLastTime;  // TZ_AgeProcess 上一次的时间（ms），Init 置为全 1
    int16_t noiseThold;  // TouchThold 按噪声触点数与 SNR 换算的噪声门限
    int16_t peakThold;  // TouchThold_Process 按 finger 门限算出的峰门限、触点门限与按下门限
    int16_t touchThold;
    int16_t field2D6;
    int16_t fingerTouchThold;  // TouchThold_Init 置为 finger 门限
    int16_t touchDownThold;
    uint8_t field2DC;
    uint8_t tholdSwitching;  // TouchThold_IsSwitching
    int16_t field2DE;
    uint8_t field2E0;
    uint8_t reserved2E1[0x3];
    uint8_t touchModeLatched;  // TouchMode：手套 / 指关节模式已锁定
    uint8_t reserved2E5[0x3];
    uint32_t touchMode;  // TouchMode：当前触控模式（0 无物体、1 手指、2 手套、3 指关节）
    int32_t touchModeLevel;  // 当前模式的检测级别（2 为确认）
    int32_t touchModeDetect;  // 当前模式的检测结果（1 未检出、2 检出）
    uint32_t prevTouchMode;  // 上一帧的 touchMode / Level / Detect
    int32_t prevTouchModeLevel;
    int32_t prevTouchModeDetect;
    int16_t cmfNoiseMin;  // CMF：本帧各线公共噪声的最小、最大值
    int16_t cmfNoiseMax;
    uint8_t reserved304[0x3C];
};
static_assert(sizeof(TsaPrpt) == 0x340);
static_assert(offsetof(TsaPrpt, raw2Raw) == 0x000);
static_assert(offsetof(TsaPrpt, prevRaw2Raw) == 0x01C);
static_assert(offsetof(TsaPrpt, brokenSensorNum) == 0x038);
static_assert(offsetof(TsaPrpt, brokenByDifZ8) == 0x039);
static_assert(offsetof(TsaPrpt, brokenByDif) == 0x03A);
static_assert(offsetof(TsaPrpt, brokenSensor) == 0x040);
static_assert(offsetof(TsaPrpt, boundaryPositive) == 0x092);
static_assert(offsetof(TsaPrpt, boundaryNegative) == 0x094);
static_assert(offsetof(TsaPrpt, rawMax) == 0x096);
static_assert(offsetof(TsaPrpt, rawMin) == 0x098);
static_assert(offsetof(TsaPrpt, field09A) == 0x09A);
static_assert(offsetof(TsaPrpt, boundaryPositiveDim1) == 0x09C);
static_assert(offsetof(TsaPrpt, boundaryPositiveDim2) == 0x09D);
static_assert(offsetof(TsaPrpt, boundaryNegativeDim1) == 0x09E);
static_assert(offsetof(TsaPrpt, boundaryNegativeDim2) == 0x09F);
static_assert(offsetof(TsaPrpt, sigPeakPositive) == 0x0A0);
static_assert(offsetof(TsaPrpt, sigPeakNegative) == 0x0A2);
static_assert(offsetof(TsaPrpt, specialThreshCol) == 0x0A4);
static_assert(offsetof(TsaPrpt, specialThreshRow) == 0x0A5);
static_assert(offsetof(TsaPrpt, nextSideThreshCol) == 0x0A6);
static_assert(offsetof(TsaPrpt, nextSideThreshRow) == 0x0A7);
static_assert(offsetof(TsaPrpt, field0AC) == 0x0AC);
static_assert(offsetof(TsaPrpt, field0B0) == 0x0B0);
static_assert(offsetof(TsaPrpt, field0B8) == 0x0B8);
static_assert(offsetof(TsaPrpt, underWaterCells) == 0x0BC);
static_assert(offsetof(TsaPrpt, underWaterNegCells) == 0x0BE);
static_assert(offsetof(TsaPrpt, field0C0) == 0x0C0);
static_assert(offsetof(TsaPrpt, field0C2) == 0x0C2);
static_assert(offsetof(TsaPrpt, field0C8) == 0x0C8);
static_assert(offsetof(TsaPrpt, field0E0) == 0x0E0);
static_assert(offsetof(TsaPrpt, field0E2) == 0x0E2);
static_assert(offsetof(TsaPrpt, preCmfMaxAbsDif) == 0x160);
static_assert(offsetof(TsaPrpt, preCmfOverCount) == 0x162);
static_assert(offsetof(TsaPrpt, field16C) == 0x16C);
static_assert(offsetof(TsaPrpt, field16E) == 0x16E);
static_assert(offsetof(TsaPrpt, field170) == 0x170);
static_assert(offsetof(TsaPrpt, field171) == 0x171);
static_assert(offsetof(TsaPrpt, field172) == 0x172);
static_assert(offsetof(TsaPrpt, field173) == 0x173);
static_assert(offsetof(TsaPrpt, field178) == 0x178);
static_assert(offsetof(TsaPrpt, field17C) == 0x17C);
static_assert(offsetof(TsaPrpt, field180) == 0x180);
static_assert(offsetof(TsaPrpt, field182) == 0x182);
static_assert(offsetof(TsaPrpt, field184) == 0x184);
static_assert(offsetof(TsaPrpt, field186) == 0x186);
static_assert(offsetof(TsaPrpt, field18C) == 0x18C);
static_assert(offsetof(TsaPrpt, field18E) == 0x18E);
static_assert(offsetof(TsaPrpt, field1A0) == 0x1A0);
static_assert(offsetof(TsaPrpt, field1A2) == 0x1A2);
static_assert(offsetof(TsaPrpt, field1C4) == 0x1C4);
static_assert(offsetof(TsaPrpt, selfCols) == 0x22C);
static_assert(offsetof(TsaPrpt, selfRows) == 0x250);
static_assert(offsetof(TsaPrpt, blStage) == 0x278);
static_assert(offsetof(TsaPrpt, prevBlStage) == 0x27C);
static_assert(offsetof(TsaPrpt, blPropertyFlags) == 0x280);
static_assert(offsetof(TsaPrpt, blResetFrames) == 0x284);
static_assert(offsetof(TsaPrpt, blSinceResetFrames) == 0x286);
static_assert(offsetof(TsaPrpt, blStageFrames) == 0x288);
static_assert(offsetof(TsaPrpt, blStageTime) == 0x290);
static_assert(offsetof(TsaPrpt, field29A) == 0x29A);
static_assert(offsetof(TsaPrpt, allFreqInitCount) == 0x29C);
static_assert(offsetof(TsaPrpt, blPropertyState) == 0x2A0);
static_assert(offsetof(TsaPrpt, prevBlPropertyState) == 0x2A4);
static_assert(offsetof(TsaPrpt, shbState) == 0x2A8);
static_assert(offsetof(TsaPrpt, shbStateStartTime) == 0x2B0);
static_assert(offsetof(TsaPrpt, shbFlags) == 0x2B8);
static_assert(offsetof(TsaPrpt, tzAgeLastTime) == 0x2C8);
static_assert(offsetof(TsaPrpt, noiseThold) == 0x2D0);
static_assert(offsetof(TsaPrpt, peakThold) == 0x2D2);
static_assert(offsetof(TsaPrpt, touchThold) == 0x2D4);
static_assert(offsetof(TsaPrpt, field2D6) == 0x2D6);
static_assert(offsetof(TsaPrpt, fingerTouchThold) == 0x2D8);
static_assert(offsetof(TsaPrpt, touchDownThold) == 0x2DA);
static_assert(offsetof(TsaPrpt, field2DC) == 0x2DC);
static_assert(offsetof(TsaPrpt, tholdSwitching) == 0x2DD);
static_assert(offsetof(TsaPrpt, field2DE) == 0x2DE);
static_assert(offsetof(TsaPrpt, field2E0) == 0x2E0);
static_assert(offsetof(TsaPrpt, touchModeLatched) == 0x2E4);
static_assert(offsetof(TsaPrpt, touchMode) == 0x2E8);
static_assert(offsetof(TsaPrpt, touchModeLevel) == 0x2EC);
static_assert(offsetof(TsaPrpt, touchModeDetect) == 0x2F0);
static_assert(offsetof(TsaPrpt, prevTouchMode) == 0x2F4);
static_assert(offsetof(TsaPrpt, prevTouchModeLevel) == 0x2F8);
static_assert(offsetof(TsaPrpt, prevTouchModeDetect) == 0x2FC);
static_assert(offsetof(TsaPrpt, cmfNoiseMin) == 0x300);
static_assert(offsetof(TsaPrpt, cmfNoiseMax) == 0x302);

/// TsaStatic::processingState
inline constexpr uint32_t kProcMsActive = 0x80000000u;  // 互容主流程进行中（PROC_MS_ACTIVE）
inline constexpr uint32_t kProcIdleScan = 0x00000010u;  // IDLE 扫描：峰 Zn 只取十字五点和，动态门限改按侧边门限

/// TsaStatic::featureFlags
inline constexpr uint32_t kFeatGloveMode = 0x00000001u;
inline constexpr uint32_t kFeatSmartCover = 0x00000002u;
inline constexpr uint32_t kFeatKnuckle = 0x00000010u;
inline constexpr uint32_t kFeatSideTouch = 0x00008000u;

[[nodiscard]] inline TsaStatic& Static() noexcept { return *Image::At<TsaStatic*>(0x6bb4f290); }
[[nodiscard]] inline TsaPrpt& Prpt() noexcept { return Image::At<TsaPrpt>(0x6bc4a640); }

}  // namespace Solvers::Tsa::Core
