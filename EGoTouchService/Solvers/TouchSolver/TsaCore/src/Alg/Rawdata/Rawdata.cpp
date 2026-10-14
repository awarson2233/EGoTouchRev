#include "Alg/Rawdata/Rawdata.h"

#include <cstdlib>

#include "Alg/Baseline/Baseline.h"
#include "Alg/TpSensor/TpSensor.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Common/Assert.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::Rawdata {

namespace {

constexpr uint32_t kSysDifBufPath = 0x10000;  // TsaStatic::systemFlags：走 Dif 缓冲路径
constexpr int kUnitScale = 0x3fff;            // 比例 1.0

bool DifBufPath() { return (Static().systemFlags & kSysDifBufPath) != 0; }
int32_t Scale() { return static_cast<int32_t>(PrmtFlashConst().field038); }
int32_t Offset() { return static_cast<int32_t>(PrmtFlashConst().field03C); }

// Rawdata_NormalizeGridUnit @ 0x6ba693f0
// raw × 比例 / 0x3fff + 偏移；rawForceNormalize 时 raw 按有符号值算。
uint16_t NormalizeUnit(uint16_t raw) {
    const uint32_t v = PrmtFlash().rawForceNormalize == 0 ? raw : static_cast<uint32_t>(static_cast<int16_t>(raw));
    return static_cast<uint16_t>(static_cast<int>(static_cast<uint32_t>(Scale()) * v) / kUnitScale + Offset());
}

// Rawdata_NormalizeGrid @ 0x6ba6946a
void NormalizeGrid(const int16_t* src, uint32_t n, int16_t* dst, uint32_t capacity) {
    if (!IsProcessNeeded()) {
        if (src != dst) CopyChecked(dst, capacity * size_t{2}, src, n * size_t{2});
        return;
    }
    const FlashPrmt& fl = PrmtFlash();
    uint16_t i = 0;
    for (uint8_t r = 0; r < fl.dim2Len; ++r) {
        for (uint8_t c = 0; c < fl.dim1Len; ++c, ++i) dst[i] = static_cast<int16_t>(NormalizeUnit(static_cast<uint16_t>(src[i])));
    }
}

}  // namespace

// Rawdata_IsProcessNeeded @ 0x6ba69306
// 比例为 1.0、偏移为 0 且不强制归一化时不需要（Dif 缓冲路径只看比例）。
bool IsProcessNeeded() {
    if (DifBufPath()) return Scale() != kUnitScale;
    return !(Scale() == kUnitScale && Offset() == 0 && PrmtFlash().rawForceNormalize == 0);
}

// Rawdata_Process @ 0x6ba69581
void Process() {
    if (!IsProcessNeeded()) return;
    TsaBufferTable& buf = Buffers();
    if (!DifBufPath()) {
        NormalizeGrid(buf.rawGrid, PrmtFlash().sensorNum, buf.rawGrid, GridBufSize());
        return;
    }
    const FlashPrmt& fl = PrmtFlash();
    const auto scale = static_cast<uint32_t>(std::abs(Scale()));
    uint16_t i = 0;
    for (uint8_t r = 0; r < fl.dim2Len; ++r) {
        for (uint8_t c = 0; c < fl.dim1Len; ++c, ++i) {
            buf.dif[i] = static_cast<int16_t>(static_cast<int>(scale * static_cast<uint32_t>(buf.dif[i])) / kUnitScale);
        }
    }
}

// Rawdata_CMF @ 0x6ba69381
void Cmf() {
    if (TSAStatic_IsHWReset()) return;
    const TsaBufferTable& buf = Buffers();
    Baseline::CalcDiffCommon(buf.dif, buf.raw, buf.bl);
    CMF_Process();
    Baseline::CalcRawCommon(buf.raw, buf.bl, buf.dif);
}

// Rawdata_RetrieveBaseline @ 0x6ba696ed（CMF_GetNoiseValueFromBuf 恒为 0，噪声缓冲不起作用）
// 侧边触摸时两端列的 Dif 不做逆缩放。
void RetrieveBaseline(int16_t* bl, const int16_t* dif) {
    if (bl == nullptr || dif == nullptr) return;
    const FlashPrmt& fl = PrmtFlash();
    const TsaStatic& stat = Static();
    for (uint8_t c = 0; c < fl.dim1Len; ++c) {
        for (uint8_t r = 0; r < fl.dim2Len; ++r) {
            const uint16_t i = GridIndex(c, r);
            if (DifBufPath()) {
                bl[i] = 0;
                continue;
            }
            int v = dif[i];
            if ((stat.featureFlags & kFeatSideTouch) == 0 || (c != 0 && c != fl.dim1Len - 1)) {
                v = TpSensor::RevertScalingDif(static_cast<int16_t>(v), c, r);
            }
            if (IsProcessNeeded()) v = v * kUnitScale / Scale();
            bl[i] = static_cast<int16_t>(stat.signalReverted == 0 ? v + bl[i] : bl[i] - v);
        }
    }
}

// Rawdata_GetUnNormalizedBaseline @ 0x6ba69937
// (基线 - 偏移) × 0x3fff / 比例，Dif 缓冲路径下不换算。
void GetUnNormalizedBaseline(int16_t* bl, uint32_t n) {
    if (IsProcessNeeded() && !DifBufPath()) {
        const FlashPrmt& fl = PrmtFlash();
        uint16_t i = 0;
        for (uint8_t r = 0; r < fl.dim2Len; ++r) {
            for (uint8_t c = 0; c < fl.dim1Len; ++c, ++i) {
                const uint32_t shifted = (static_cast<uint16_t>(bl[i]) - static_cast<uint32_t>(Offset())) * kUnitScale;
                bl[i] = static_cast<int16_t>(static_cast<int>(shifted) / Scale());
            }
        }
    }
    TpSensor::RevertTranslateRaw(bl, n);
}

}  // namespace Solvers::Tsa::Core::Rawdata

// raw 的 TSA / AFE 还在调用的入口
extern "C" {
namespace R = Solvers::Tsa::Core::Rawdata;
void Rawdata_Process(void) { R::Process(); }
void Rawdata_CMF(void) { R::Cmf(); }
void Rawdata_RetrieveBaseline(int16_t* bl, uint64_t, const int16_t* dif, uint64_t, int64_t, ...) { R::RetrieveBaseline(bl, dif); }
void Rawdata_GetUnNormalizedBaseline(int16_t* bl, uint32_t n) { R::GetUnNormalizedBaseline(bl, n); }
}  // extern "C"
