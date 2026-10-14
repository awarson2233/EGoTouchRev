#include "Alg/TouchSize/TouchSize.h"

#include <algorithm>

#include "Alg/Touch/PrevTouch.h"
#include "Alg/Touch/TouchData.h"
#include "Common/Assert.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::TouchSize {

namespace {

constexpr int32_t kMaxSigSum = 0x1fffff;    // TDIFS_MAX >> TS_UNIT_PER_SIG_MM2_SCALE_SHIFTER
constexpr uint8_t kMaxSizeMm = 15;
constexpr uint8_t kMaxDefaultSizeMm = 9;

// TS_GetSizeInMM @ 0x6ba7c692
// 满足 thold × sizeAreaScale × (s + s²) > sigSum × 1024 的最小 s（1..15），即信号和按门限折算的圆面积对应的尺寸。
// sigSum 超出范围时返回 0xffffffff。
uint32_t GetSizeInMm(int16_t thold, int32_t sigSum) {
    if (sigSum > kMaxSigSum) return 0xffffffff;
    const uint32_t scaledSig = static_cast<uint32_t>(sigSum) << 10;
    const uint16_t areaScale = PrmtFlashConst().sizeAreaScale;
    uint8_t size = 1;
    for (;;) {
        const auto area = static_cast<uint8_t>(size + static_cast<uint8_t>(size * size));
        TsaAssert(sigSum <= kMaxSigSum);
        if (static_cast<uint32_t>(thold) * (static_cast<uint32_t>(areaScale) * area) > scaledSig) break;
        if (++size >= kMaxSizeMm) break;
    }
    return size;
}

// TS_UpdateCurSizeInMM @ 0x6ba7c006
// 按信号和估算；椭圆长轴超过 (传感器间距 / 8) + 2 时直接取长轴。
uint8_t UpdateCurSizeInMm(uint8_t curIdx) {
    if (curIdx >= kMaxTouchNum) return 0;
    const TouchUnit& cur = Touches().unit[curIdx];
    auto size = static_cast<uint8_t>(GetSizeInMm(PrmtDynamic().touchThold, cur.sigSum));
    if ((PrmtFlash().sensorPitchX16 >> 3) + 2 < cur.majorMm) size = static_cast<uint8_t>(cur.majorMm);
    return size;
}

// TS_UpdateCurSizeInMMForUdfp @ 0x6ba7c0d6
// 指纹区用的尺寸：估算值 7..10 时向长短轴的几何平均线性过渡，11 以上直接取几何平均。
uint8_t UpdateCurSizeInMmForUdfp(uint8_t curIdx) {
    if (curIdx >= kMaxTouchNum) return 0;
    const TouchUnit& cur = Touches().unit[curIdx];
    auto size = static_cast<uint8_t>(GetSizeInMm(PrmtDynamic().touchThold, cur.sigSum));
    const auto axisMean = static_cast<uint32_t>(Misc_SqrtUint32(static_cast<uint32_t>(cur.minorMm) * cur.majorMm));
    if (size > 10) {
        size = static_cast<uint8_t>(axisMean);
    } else if (size > 6) {
        const uint32_t weight = size - 6u;
        size = static_cast<uint8_t>((axisMean * weight + size * (4u - weight)) >> 2);
    }
    return size;
}

// TS_ProcessUnit @ 0x6ba7c1d7
// 取本帧估算与历史尺寸的较大者；两者都为 0 时取默认尺寸。
void ProcessUnit(uint8_t prevIdx, uint8_t curIdx) {
    uint8_t size = UpdateCurSizeInMm(curIdx);
    const uint8_t udfpSize = UpdateCurSizeInMmForUdfp(curIdx);
    const uint8_t prevSize = prevIdx < kMaxTouchNum ? PrevTouches().unit[prevIdx].sizeMm : 0;
    if (size == 0 && prevSize == 0) {
        size = PrmtDynamic().defaultSizeMm;
    } else {
        size = std::max(size, prevSize);
    }
    if (curIdx < kMaxTouchNum) {
        TouchUnit& cur = Touches().unit[curIdx];
        cur.udfpSizeMm = udfpSize;
        cur.sizeMm = size;
    }
}

}  // namespace

// TS_Process @ 0x6ba7c2cb
void Process() {
    for (uint8_t i = 0; i < Touches().count; ++i) {
        const uint8_t prevIdx = PrevTouch::GetIdxBasedOnCurIdx(i);
        if ((Touches().unit[i].event & TouchEvent::kLiftOff) == 0) ProcessUnit(prevIdx, i);
    }
}

// TS_PostProcess @ 0x6ba7c368
// 原代码不检查历史下标：为 kNoTouch 时照样按下标读，落在历史表之后的内存上。
void PostProcess() {
    for (uint8_t i = 0; i < Touches().count; ++i) {
        const uint8_t prevIdx = PrevTouch::GetIdxBasedOnCurIdx(i);
        if ((Touches().unit[i].event & TouchEvent::kLiftOff) != 0) {
            PrmtDynamic().defaultSizeMm = std::min(PrevTouches().unit[prevIdx].sizeMm, kMaxDefaultSizeMm);
        }
    }
}

// TS_MMToPixel @ 0x6ba7c416
int MmToPixel(uint8_t mm) { return static_cast<int>(static_cast<uint32_t>(PrmtFlashConst().q8PerMm) * mm); }

// TS_MMToDisplayPixel @ 0x6ba7c436
uint32_t MmToDisplayPixel(uint8_t mm) {
    const FlashPrmt& flash = PrmtFlash();
    const uint16_t res = std::max(flash.dim1Res, flash.dim2Res);
    return static_cast<uint32_t>(mm) * res * 16u / flash.field02E;
}

// TS_PixelToMM @ 0x6ba7c561
int PixelToMm(uint16_t pixel) {
    const uint32_t mmQ16 = static_cast<uint32_t>(PrmtFlashConst().mmPerQ8Q16) * pixel;
    return static_cast<int>((mmQ16 >> 16) + ((mmQ16 >> 15) & 1));
}

// TS_SQPixelToSQMM @ 0x6ba7c4a0
uint32_t SqPixelToSqMm(uint32_t sqPixel) {
    const uint64_t scale = PrmtFlashConst().mmPerQ8Q16;
    const uint64_t sqMm = scale * (sqPixel * scale) >> 32;
    return sqMm > 0xffff ? 0xffffffff : static_cast<uint32_t>(sqMm);
}

// TS_SQMMToSQPixel @ 0x6ba7c4fb
uint64_t SqMmToSqPixel(uint16_t sqMm) {
    const uint64_t scale = PrmtFlashConst().mmPerQ8Q16;
    return (static_cast<uint64_t>(sqMm) << 32) / scale / scale;
}

}  // namespace Solvers::Tsa::Core::TouchSize

namespace TS = Solvers::Tsa::Core::TouchSize;

extern "C" {
void TS_Init(void) {}
void TS_Process(void) { TS::Process(); }
void TS_PostProcess(void) { TS::PostProcess(); }
int TS_MMToPixel(uint8_t mm) { return TS::MmToPixel(mm); }
uint32_t TS_MMToDisplayPixel(uint8_t mm) { return TS::MmToDisplayPixel(mm); }
int TS_PixelToMM(uint16_t pixel) { return TS::PixelToMm(pixel); }
uint64_t TS_SQPixelToSQMM(uint32_t sqPixel) { return TS::SqPixelToSqMm(sqPixel); }
uint64_t TS_SQMMToSQPixel(uint16_t sqMm) { return TS::SqMmToSqPixel(sqMm); }
}  // extern "C"
