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
    uint64_t noiseSettleCount;  // 噪声异常之外的帧逐帧累加 baselineStageInitialSignal，超过 0x52 时噪声等级清零
    uint32_t lastDetected;  // 上一帧检出的异常；强制进入水下、噪声等异常时为强制的值
    uint8_t wetFinger;  // FI：本帧有区域判为湿手指
    uint8_t reserved015[0x3];
    uint32_t detected;  // 本帧检出的异常（各区域与水下检测取最大），Exception_Process 末尾清零
    uint8_t reserved01C[0x4];
};
static_assert(sizeof(ExceptionState) == 0x020);
static_assert(offsetof(ExceptionState, debounceFrames) == 0x000);
static_assert(offsetof(ExceptionState, noiseSettleCount) == 0x008);
static_assert(offsetof(ExceptionState, lastDetected) == 0x010);
static_assert(offsetof(ExceptionState, wetFinger) == 0x014);
static_assert(offsetof(ExceptionState, detected) == 0x018);

/// g_tsaStaticGrid（经 g_tsaStaticPtr 选用）：系统级运行状态。
struct TsaStatic {
    uint8_t reserved000[0x78];
    uint64_t fullIncellHighPeakTime;  // 最大峰持续超过 3 倍 finger 门限时的时间（ms）
    uint8_t reserved080[0x10];
    uint64_t markedLiftOffTime;  // 带 persistFlags 0x20000000 的触点抬起时的时间（ms）
    Point markedLiftOffPos;  // 以及它的位置
    uint8_t reserved09C[0x4];
    uint8_t fullIncellHighPeakFrames;  // 最大峰超过上述门限的连续帧数（到 6 为止）
    uint8_t reserved0A1[0x3];
    uint32_t specialControlFlags;
    uint8_t reserved0A8[0x4];
    int32_t trustedTouchCount;  // TouchUnit::trustTag 为 1 的累计次数
    int32_t suspectTouchCount;  // trustTag 为 2 的累计次数
    uint8_t reserved0B4[0x4];
    int32_t frameCounter;
    uint32_t systemFlags;
    uint8_t reserved0C0[0x20];
    uint64_t curTimeMs;  // 本帧时间（ms）
    uint8_t reserved0E8[0x10];
    int16_t baselineStageInitialSignal;  // 基线初始化阶段的信号门限
    uint8_t reserved0FA[0x2];
    uint32_t featureFlags;
    uint8_t reserved100[0x8];
    uint32_t featureFlags2;
    uint32_t processingState;  // bit31：互容主流程进行中；bit4：峰 Zn 只取十字五点和
    uint8_t field110;  // 全局异常为 0x40 / 0x80 时置 1
    uint8_t noiseLevel;  // 噪声等级：异常 0x100 时 2、0x8000 时 3，噪声异常结束一段时间后清零
    uint8_t reserved112[0x2];
    uint8_t fullIncellSignalDisparity;  // 整屏 incell 信号失衡（峰检测按 SD 模式处理）
    uint8_t justExitedSpecialCtrlMode;  // 刚退出特殊控制模式
    uint8_t reserved116[0x4];
    uint8_t signalReverted;  // 信号反向（TSAStatic_IsSignalReverted）
    uint8_t reserved11B[0x7];
    uint16_t strongTapCount;  // TE：报过点的触点抬起时峰值超过 2 倍 finger 门限、长轴小于 21 mm 的连续次数（到 5 为止）
    uint8_t reserved124[0x2CD];
    uint8_t field3F1;
    uint8_t reserved3F2[0x9];
    uint8_t field3FB;
    uint8_t reserved3FC[0xC];
    ExceptionState hiException;  // Hybrid Incell 用（本机型 HIException 被裁掉）
    uint8_t reserved428[0x8];
    ExceptionState fiException;  // Full Incell 用
};
static_assert(sizeof(TsaStatic) == 0x450);
static_assert(offsetof(TsaStatic, fullIncellHighPeakTime) == 0x078);
static_assert(offsetof(TsaStatic, markedLiftOffTime) == 0x090);
static_assert(offsetof(TsaStatic, markedLiftOffPos) == 0x098);
static_assert(offsetof(TsaStatic, fullIncellHighPeakFrames) == 0x0A0);
static_assert(offsetof(TsaStatic, specialControlFlags) == 0x0A4);
static_assert(offsetof(TsaStatic, trustedTouchCount) == 0x0AC);
static_assert(offsetof(TsaStatic, suspectTouchCount) == 0x0B0);
static_assert(offsetof(TsaStatic, frameCounter) == 0x0B8);
static_assert(offsetof(TsaStatic, systemFlags) == 0x0BC);
static_assert(offsetof(TsaStatic, curTimeMs) == 0x0E0);
static_assert(offsetof(TsaStatic, baselineStageInitialSignal) == 0x0F8);
static_assert(offsetof(TsaStatic, featureFlags) == 0x0FC);
static_assert(offsetof(TsaStatic, featureFlags2) == 0x108);
static_assert(offsetof(TsaStatic, processingState) == 0x10C);
static_assert(offsetof(TsaStatic, field110) == 0x110);
static_assert(offsetof(TsaStatic, noiseLevel) == 0x111);
static_assert(offsetof(TsaStatic, fullIncellSignalDisparity) == 0x114);
static_assert(offsetof(TsaStatic, justExitedSpecialCtrlMode) == 0x115);
static_assert(offsetof(TsaStatic, signalReverted) == 0x11A);
static_assert(offsetof(TsaStatic, strongTapCount) == 0x122);
static_assert(offsetof(TsaStatic, field3F1) == 0x3F1);
static_assert(offsetof(TsaStatic, field3FB) == 0x3FB);
static_assert(offsetof(TsaStatic, hiException) == 0x408);
static_assert(offsetof(TsaStatic, fiException) == 0x430);

/// g_tsaPrpt：每帧统计的全局属性。
struct TsaPrpt {
    uint8_t reserved000[0x4];
    uint16_t field004;  // RfNoise：超过传感器数的一半时即使没有噪声线也做前后两半的比较
    uint8_t reserved006[0x9A];
    int16_t sigPeakPositive;  // Dif 的正向极值
    uint8_t reserved0A2[0x1A];
    int16_t underWaterCells;  // Dif 落在水下门限区间内的格数
    int16_t underWaterNegCells;  // -Dif 落在该区间内的格数
    uint8_t reserved0C0[0x8];
    int32_t field0C8;
    uint8_t reserved0CC[0xBA];
    uint16_t field186;
    uint8_t reserved188[0x148];
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
    uint8_t field2E4;
    uint8_t reserved2E5[0x3];
    uint32_t field2E8;
    int32_t field2EC;
    uint8_t reserved2F0[0x50];
};
static_assert(sizeof(TsaPrpt) == 0x340);
static_assert(offsetof(TsaPrpt, field004) == 0x004);
static_assert(offsetof(TsaPrpt, sigPeakPositive) == 0x0A0);
static_assert(offsetof(TsaPrpt, underWaterCells) == 0x0BC);
static_assert(offsetof(TsaPrpt, underWaterNegCells) == 0x0BE);
static_assert(offsetof(TsaPrpt, field0C8) == 0x0C8);
static_assert(offsetof(TsaPrpt, field186) == 0x186);
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
static_assert(offsetof(TsaPrpt, field2E4) == 0x2E4);
static_assert(offsetof(TsaPrpt, field2E8) == 0x2E8);
static_assert(offsetof(TsaPrpt, field2EC) == 0x2EC);

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
