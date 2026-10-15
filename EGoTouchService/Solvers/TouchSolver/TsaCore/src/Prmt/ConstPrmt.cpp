#include "Prmt/TsaPrmt.h"

#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::ConstPrmt {

namespace {

// 一组 AIIR 系数的插值斜率（Q10）：(heavyCoef - lightCoef) × 1024 / (heavyDelta - lightDelta)，两个位移量相同时为 1。
// 全程是 32 位无符号运算，系数递减时按回绕的结果截成 16 位。
uint16_t AiirRatio(const AiirPrmt& aiir) {
    if (aiir.heavyDelta == aiir.lightDelta) return 1;
    const uint32_t coefSpan = static_cast<uint32_t>(aiir.heavyCoef) - aiir.lightCoef;
    const uint32_t deltaSpan = static_cast<uint32_t>(aiir.heavyDelta) - aiir.lightDelta;
    return static_cast<uint16_t>(coefSpan * 0x400u / deltaSpan);
}

// value × 阵列长度 × 256 / 显示分辨率（分辨率为 0 时为 1）。
uint16_t PerResolution(uint8_t value, uint8_t len, uint16_t res) {
    if (res == 0) return 1;
    return static_cast<uint16_t>(static_cast<uint32_t>(value) * len * 0x100u / res);
}

// 显示坐标范围（分辨率加两端裁剪）对 Q8.8 像素坐标的比例，Q16。
uint32_t DisplayScale(uint16_t res, int8_t clipLow, int8_t clipHigh, uint8_t len) {
    const uint32_t span = static_cast<uint32_t>(clipHigh + res + clipLow);
    return span * 0x10000u / (static_cast<uint32_t>(len) << 8);
}

}  // namespace

// TSAConstPrmt_Init @ 0x6bb3bb00
void Init() {
    const FlashPrmt& flash = PrmtFlash();
    FlashConstPrmt& c = PrmtFlashConst();
    const uint8_t dim1Len = flash.dim1Len;
    const uint8_t dim2Len = flash.dim2Len;

    c.dim1LenQ8 = static_cast<uint16_t>(dim1Len << 8);
    c.dim2LenQ8 = static_cast<uint16_t>(dim2Len << 8);
    c.dim1LogicLen = static_cast<uint8_t>(flash.dim1LogicLast - flash.dim1LogicFirst + 1);
    c.dim2LogicLen = static_cast<uint8_t>(flash.dim2LogicLast - flash.dim2LogicFirst + 1);
    c.dim1NearEdge = static_cast<uint16_t>(flash.dim1LogicFirst << 8);
    c.dim1FarEdge = static_cast<uint16_t>((flash.dim1LogicLast + 1) * 0x100);
    c.dim2NearEdge = static_cast<uint16_t>(flash.dim2LogicFirst << 8);
    c.dim2FarEdge = static_cast<uint16_t>((flash.dim2LogicLast + 1) * 0x100);
    c.field032 = static_cast<uint16_t>(flash.field096 - flash.field094 + 1);
    c.field034 = static_cast<uint16_t>(flash.field09A - flash.field098 + 1);
    if (flash.dim1Axis == 0) {
        c.field025 = flash.dim1Len;
        c.field024 = flash.dim2Len;
    } else {
        c.field025 = flash.dim2Len;
        c.field024 = flash.dim1Len;
    }
    uint8_t maxValue = 0;
    for (int i = 0; i < c.field025; ++i) {
        if (maxValue < flash.field62C[i]) maxValue = flash.field62C[i];
    }
    c.field026 = static_cast<uint8_t>(maxValue + 1);

    c.dim1DisplayScale = DisplayScale(flash.dim1Res, flash.dim1ClipLow, flash.dim1ClipHigh, dim1Len);
    c.dim2DisplayScale = DisplayScale(flash.dim2Res, flash.dim2ClipLow, flash.dim2ClipHigh, dim2Len);
    for (int i = 0; i < 3; ++i) c.aiirCoefToDeltaRatio[i] = AiirRatio(flash.aiir[i]);
    c.jitRadius = PerResolution(flash.jitRadiusDisplay, dim1Len, flash.dim1Res);
    c.jitRadiusDim2 = PerResolution(flash.jitRadiusDisplay, dim2Len, flash.dim2Res);
    c.field016 = PerResolution(flash.field8A0, dim1Len, flash.dim1Res);
    c.field018 = PerResolution(flash.field8A0, dim2Len, flash.dim2Res);

    // 以下按 64 位无符号除；sensorPitchX16 为 0 时与原代码一样除零
    const uint64_t pitch = flash.sensorPitchX16;
    c.q8PerMm = static_cast<uint8_t>(0x1000u / pitch + 1);
    c.sizeAreaScale = static_cast<uint16_t>(0x32400u / pitch / pitch);
    c.mmPerQ8Q16 = static_cast<uint16_t>(0x10000u / static_cast<uint64_t>(static_cast<int64_t>(static_cast<int>(0x1000u / pitch) + 1)));
    const uint32_t scaleProduct = static_cast<uint32_t>(flash.peakFilterReferenceThreshold) * c.sizeAreaScale * flash.defaultSizeMm *
                                  flash.defaultSizeMm;
    c.field000 = static_cast<uint16_t>(0x100000u / static_cast<uint64_t>(scaleProduct >> 10));

    // 参考门限到 [field600, field5FC] 区间的线性映射：斜率（有符号除）与截距（无符号除）
    c.field038 = static_cast<uint32_t>(flash.peakFilterReferenceThreshold * 0x3fff / (flash.field5FC - flash.field600));
    c.field03C = 0x7fffu - c.field038 * static_cast<uint32_t>(flash.field5FC) / 0x3fffu;

    const uint32_t refSum = static_cast<uint32_t>(flash.peakFilterReferenceThreshold) + flash.field1656 + flash.field1658 +
                            flash.field165A;
    const int margin = static_cast<int>(refSum * 3u) / 2;
    const uint32_t m = static_cast<uint32_t>(margin);
    c.field05A = static_cast<int>(m + flash.rawUpperLimit) < 0x10000 ? static_cast<uint16_t>(flash.rawUpperLimit + margin) : 0xffff;
    c.field05C = static_cast<int>(flash.rawLowerLimit - m) < 0 ? 0 : static_cast<uint16_t>(flash.rawLowerLimit - margin);
    c.field05E = static_cast<int>(m + flash.rawDeltaLimit) < 0x8000 ? static_cast<uint16_t>(margin + flash.rawDeltaLimit) : 0x7fff;
    c.field060 = static_cast<int>(static_cast<uint32_t>(-margin) - flash.rawDeltaLimit) < -0x8000
                     ? static_cast<uint16_t>(0x8000)
                     : static_cast<uint16_t>(-margin - flash.rawDeltaLimit);
}

}  // namespace Solvers::Tsa::Core::ConstPrmt
