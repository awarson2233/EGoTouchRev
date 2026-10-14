#pragma once
// 系统级运行状态（TSA_Static）与每帧统计的全局属性（TSA_Prpt）。只列已改写的模块用到的字段。

#include <cstddef>
#include <cstdint>

#include "Common/Image.h"
#include "Common/Types.h"

namespace Solvers::Tsa::Core {

/// g_tsaStaticGrid（经 g_tsaStaticPtr 选用）：系统级运行状态。
struct TsaStatic {
    uint8_t reserved000[0x90];
    uint64_t markedLiftOffTime;  // 带 persistFlags 0x20000000 的触点抬起时的时间（ms）
    Point markedLiftOffPos;  // 以及它的位置
    uint8_t reserved09C[0x8];
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
    uint8_t reserved100[0xC];
    uint32_t processingState;  // bit31：互容主流程进行中
    uint8_t reserved110[0x2E1];
    uint8_t field3F1;
    uint8_t reserved3F2[0x5E];
};
static_assert(sizeof(TsaStatic) == 0x450);
static_assert(offsetof(TsaStatic, markedLiftOffTime) == 0x090);
static_assert(offsetof(TsaStatic, markedLiftOffPos) == 0x098);
static_assert(offsetof(TsaStatic, specialControlFlags) == 0x0A4);
static_assert(offsetof(TsaStatic, trustedTouchCount) == 0x0AC);
static_assert(offsetof(TsaStatic, suspectTouchCount) == 0x0B0);
static_assert(offsetof(TsaStatic, frameCounter) == 0x0B8);
static_assert(offsetof(TsaStatic, systemFlags) == 0x0BC);
static_assert(offsetof(TsaStatic, curTimeMs) == 0x0E0);
static_assert(offsetof(TsaStatic, baselineStageInitialSignal) == 0x0F8);
static_assert(offsetof(TsaStatic, featureFlags) == 0x0FC);
static_assert(offsetof(TsaStatic, processingState) == 0x10C);
static_assert(offsetof(TsaStatic, field3F1) == 0x3F1);

/// g_tsaPrpt：每帧统计的全局属性。
struct TsaPrpt {
    uint8_t reserved000[0x186];
    uint16_t field186;
    uint8_t reserved188[0x15C];
    uint8_t field2E4;
    uint8_t reserved2E5[0x3];
    uint32_t field2E8;
    int32_t field2EC;
    uint8_t reserved2F0[0x50];
};
static_assert(sizeof(TsaPrpt) == 0x340);
static_assert(offsetof(TsaPrpt, field186) == 0x186);
static_assert(offsetof(TsaPrpt, field2E4) == 0x2E4);
static_assert(offsetof(TsaPrpt, field2E8) == 0x2E8);
static_assert(offsetof(TsaPrpt, field2EC) == 0x2EC);

/// TsaStatic::processingState
inline constexpr uint32_t kProcMsActive = 0x80000000u;  // 互容主流程进行中（PROC_MS_ACTIVE）

/// TsaStatic::featureFlags
inline constexpr uint32_t kFeatGloveMode = 0x00000001u;
inline constexpr uint32_t kFeatSmartCover = 0x00000002u;
inline constexpr uint32_t kFeatKnuckle = 0x00000010u;
inline constexpr uint32_t kFeatSideTouch = 0x00008000u;

[[nodiscard]] inline TsaStatic& Static() noexcept { return *Image::At<TsaStatic*>(0x6bb4f290); }
[[nodiscard]] inline TsaPrpt& Prpt() noexcept { return Image::At<TsaPrpt>(0x6bc4a640); }

}  // namespace Solvers::Tsa::Core
