#include "Alg/Touch/TouchGeometry.h"

#include "Alg/TouchSize/TouchSize.h"
#include "App/TsaStatic.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::Touch {

namespace {

/// 原代码用 (x ^ s) - s 求绝对值：INT_MIN 保持不变。
int AbsWrap(int x) {
    return x < 0 ? static_cast<int>(0u - static_cast<uint32_t>(x)) : x;
}

}  // namespace

// Touch_GetSQDistPixel @ 0x6ba73447
uint32_t GetSqDistPixel(uint16_t dim1A, uint16_t dim2A, uint16_t dim1B, uint16_t dim2B) {
    const auto d1 = static_cast<uint32_t>(AbsWrap(dim1A - dim1B));
    const auto d2 = static_cast<uint32_t>(AbsWrap(dim2A - dim2B));
    const uint32_t sq1 = d1 * d1;
    const uint32_t sum = d2 * d2 + sq1;
    return sum < sq1 ? 0xffffffffu : sum;
}

// Touch_GetSQDistMM @ 0x6ba734be
uint16_t GetSqDistMm(uint16_t dim1A, uint16_t dim2A, uint16_t dim1B, uint16_t dim2B) {
    return static_cast<uint16_t>(TouchSize::SqPixelToSqMm(GetSqDistPixel(dim1A, dim2A, dim1B, dim2B)));
}

// Touch_GetSQDistPixelToEdge @ 0x6ba7329a
int GetSqDistPixelToEdge(int16_t dim1, int16_t dim2, int16_t toDim1, int16_t toDim2) {
    const FlashConstPrmt& edge = PrmtFlashConst();
    const auto step1 = static_cast<int16_t>(toDim1 - dim1);
    const auto step2 = static_cast<int16_t>(toDim2 - dim2);
    int16_t x = dim1;
    int16_t y = dim2;
    if (step1 != 0 && step2 != 0) {
        for (;;) {
            x = static_cast<int16_t>(x - step1);
            y = static_cast<int16_t>(y - step2);
            if (x < edge.dim1NearEdge) x = static_cast<int16_t>(edge.dim1NearEdge);
            if (y < edge.dim2NearEdge) y = static_cast<int16_t>(edge.dim2NearEdge);
            if (edge.dim1FarEdge <= x) x = static_cast<int16_t>(edge.dim1FarEdge - 1);
            if (edge.dim2FarEdge <= y) y = static_cast<int16_t>(edge.dim2FarEdge - 1);
            if (x == edge.dim1NearEdge || y == edge.dim2NearEdge || x == edge.dim1FarEdge - 1 ||
                y == edge.dim2FarEdge - 1) {
                break;
            }
        }
    }
    return static_cast<int>(GetSqDistPixel(static_cast<uint16_t>(x), static_cast<uint16_t>(y),
                                           static_cast<uint16_t>(dim1), static_cast<uint16_t>(dim2)));
}

// Touch_GetDistToLogicEdge @ 0x6ba73009
int16_t GetDistToLogicEdge(int edge, int16_t dim1, int16_t dim2) {
    const FlashConstPrmt& prmt = PrmtFlashConst();
    switch (edge) {
    case 1: return static_cast<int16_t>(dim1 - static_cast<int16_t>(prmt.dim1NearEdge));
    case 2: return static_cast<int16_t>(static_cast<int16_t>(prmt.dim1FarEdge) - dim1 - 1);
    case 4: return static_cast<int16_t>(dim2 - static_cast<int16_t>(prmt.dim2NearEdge));
    case 8: return static_cast<int16_t>(static_cast<int16_t>(prmt.dim2FarEdge) - dim2 - 1);
    default: return 0;
    }
}

// Touch_GetDim1EdgeDist @ 0x6ba72a59
uint32_t GetDim1EdgeDist(uint16_t dim1) {
    const FlashConstPrmt& prmt = PrmtFlashConst();
    if (prmt.dim1FarEdge - dim1 < dim1 - prmt.dim1NearEdge) return static_cast<uint16_t>(prmt.dim1FarEdge - dim1);
    return static_cast<uint32_t>(dim1) - prmt.dim1NearEdge;
}

// Touch_GetDim2EdgeDist @ 0x6ba72ab9
uint32_t GetDim2EdgeDist(uint16_t dim2) {
    const FlashConstPrmt& prmt = PrmtFlashConst();
    if (prmt.dim2FarEdge - dim2 < dim2 - prmt.dim2NearEdge) return static_cast<uint16_t>(prmt.dim2FarEdge - dim2);
    return static_cast<uint32_t>(dim2) - prmt.dim2NearEdge;
}

// Touch_GetPostReportLength @ 0x6ba76958
uint32_t GetPostReportLength(int startDim1, int startDim2, int endDim1, int endDim2) {
    const auto pitch = static_cast<uint16_t>(TouchSize::MmToPixel(2));
    uint32_t length = 0;
    if (pitch != 0) {
        const int d1 = AbsWrap(static_cast<int>(static_cast<uint32_t>(endDim1) - static_cast<uint32_t>(startDim1)));
        const int d2 = AbsWrap(static_cast<int>(static_cast<uint32_t>(endDim2) - static_cast<uint32_t>(startDim2)));
        length = static_cast<uint32_t>((d2 < d1 ? d1 : d2) / static_cast<int>(pitch));
    }
    if (length > 5) length = 5;
    if (length < 2) length = 2;
    return length;
}

// Touch_GetFormattedTimeInterval @ 0x6ba76329
int16_t GetFormattedTimeInterval(int16_t dt) {
    if (dt == 0) {
        const uint8_t rate = PrmtFlash().reportRateHz;
        return rate == 0 ? int16_t{8} : static_cast<int16_t>(1000 / rate);
    }
    if (dt < 1) {
        if (Static().frameCounter == 0) return 0;
        if (dt < -7 && static_cast<char>(TSAStatic_IsAFEJustEnterIDLEMode()) == 0) {
            return dt > -1001 ? dt : int16_t{-1000};
        }
        return -8;
    }
    if (dt > 7 && static_cast<char>(TSAStatic_IsAFEJustExitIDLEMode()) == 0) {
        return dt < 1001 ? dt : int16_t{1000};
    }
    return 8;
}

// Touch_IIRProcess @ 0x6ba746ed
int IirProcess(int previous, int current) {
    const int filtered = (previous * 7 + current) / 8;
    if (filtered < current) return filtered + 1;
    if (current < filtered) return filtered - 1;
    return filtered;
}

}  // namespace Solvers::Tsa::Core::Touch
