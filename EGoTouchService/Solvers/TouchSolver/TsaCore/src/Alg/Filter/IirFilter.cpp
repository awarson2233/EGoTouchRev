#include "Alg/Filter/IirFilter.h"

#include "Common/Assert.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::Iir {

// IIR_Process @ 0x6ba601b0
int Process(int current, int previous, uint8_t weight, uint8_t shift) noexcept {
    const int scale = 1 << (shift & 0x1f);
    const uint32_t sum = weight * static_cast<uint32_t>(current) +
                         ((static_cast<uint32_t>(scale) & 0xffu) - weight) * static_cast<uint32_t>(previous);
    return static_cast<int>(sum) / scale;
}

// IIR_Process2 @ 0x6ba6021b
int ProcessConverging(int current, int previous, uint8_t weight, uint8_t shift) noexcept {
    const int scale = 1 << (shift & 0x1f);
    const uint32_t sum = weight * static_cast<uint32_t>(current) +
                         (static_cast<uint32_t>(scale) - weight) * static_cast<uint32_t>(previous);
    const int filtered = static_cast<int>(sum) / scale;
    if (filtered < current) return filtered + 1;
    if (filtered > current) return filtered - 1;
    return filtered;
}

}  // namespace Solvers::Tsa::Core::Iir

namespace Solvers::Tsa::Core::Aiir {

// AIIR_GetCoef @ 0x6ba60040
uint8_t GetCoef(uint16_t delta) noexcept {
    const AiirPrmt& prmt = PrmtRam().aiir;
    const uint32_t ratio = PrmtRamConst().aiirScaledCoefToDeltaRatio;
    TsaAssert((static_cast<uint32_t>(prmt.heavyDelta) - prmt.lightDelta) * ratio < 0xffff);

    uint8_t coef;
    if (delta >= prmt.heavyDelta) {
        coef = prmt.heavyCoef;
    } else if (delta <= prmt.lightDelta) {
        coef = prmt.lightCoef;
    } else {
        const int extra = static_cast<int>((static_cast<uint32_t>(delta) - prmt.lightDelta) * ratio) / 1024;
        coef = static_cast<uint8_t>(extra + prmt.lightCoef);
    }
    if (coef == 0 && delta != 0) coef = 1;
    return coef;
}

}  // namespace Solvers::Tsa::Core::Aiir
