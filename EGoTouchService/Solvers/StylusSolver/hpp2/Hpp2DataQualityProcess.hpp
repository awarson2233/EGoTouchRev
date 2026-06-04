#pragma once

#include "Hpp2Runtime.hpp"

#include <cstddef>

namespace Solvers::Stylus::Hpp2 {

class Hpp2DataQualityProcess {
public:
  void Process(Context &ctx) const {
    auto &hpp2 = ctx.runtime;
    auto &state = ctx.state;
    hpp2.rawAbnormal =
        hpp2.rawLineSum > ctx.settings.rawAbnormalLineSumThreshold &&
        hpp2.energyRatioPrev > ctx.settings.rawAbnormalEnergyRatioThreshold;

    hpp2.cmnAbnormal =
        (state.m_cmnSum[0] + state.m_cmnSum[1]) >
            ctx.settings.cmnAbnormalSumThreshold &&
        (state.m_cmnMin[0] < ctx.settings.cmnAbnormalMinThreshold ||
         state.m_cmnMin[1] < ctx.settings.cmnAbnormalMinThreshold);

    const std::size_t freqIdx = static_cast<std::size_t>(state.m_curFreqIdx);
    state.m_noiseSum[freqIdx] = 0;
    state.m_noiseFlag[freqIdx] = 0;
  }

  void UpdateFrequencyLatch(Context &ctx) const {
    auto &hpp2 = ctx.runtime;
    auto &state = ctx.state;
    const std::size_t freqIdx = static_cast<std::size_t>(state.m_curFreqIdx);
    const bool noisy =
        hpp2.rawAbnormal || hpp2.cmnAbnormal || state.m_noiseFlag[freqIdx] != 0;
    if (hpp2.mainFreq == kFreqF1) {
      state.m_freqNoiseLatchF1 = noisy;
    } else {
      state.m_freqNoiseLatchF2 = noisy;
    }
  }
};

} // namespace Solvers::Stylus::Hpp2
