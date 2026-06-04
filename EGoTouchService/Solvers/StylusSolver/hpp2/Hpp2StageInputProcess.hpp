#pragma once

#include "SolverTypes.h"
#include "Hpp2Runtime.hpp"

#include <algorithm>
#include <cstdint>

namespace Solvers::Stylus::Hpp2 {

class Hpp2StageInputProcess {
public:
    void Process(Context& ctx) const {
        auto& input = ctx.frame.stylus.input;
        auto& hpp2 = ctx.runtime;
        auto& state = ctx.state;
        hpp2 = {};
        hpp2.mainFreq = input.mainFreq;
        hpp2.auxFreq = input.auxFreq;
        hpp2.rawPressure = input.framePressure;
        hpp2.buttonBits = input.buttonBits;
        state.m_curFreqIdx = (hpp2.mainFreq == kFreqF1) ? 0 : 1;

        const int count = ctx.settings.SampleCount();
        uint32_t sum = 0;
        for (int i = 0; i < count; ++i) {
            const uint16_t sample = input.hpp2LineData[static_cast<std::size_t>(i)];
            hpp2.line.raw[static_cast<std::size_t>(i)] = sample;
            hpp2.line.cmnSubtracted[static_cast<std::size_t>(i)] = sample;
            sum += sample;
        }
        hpp2.rawLineSum = sum;

        for (int i = kHistorySize - 1; i > 0; --i) {
            state.m_lineSumHistory[static_cast<std::size_t>(i)] = state.m_lineSumHistory[static_cast<std::size_t>(i - 1)];
        }
        state.m_lineSumHistory[0] = sum;
        hpp2.line.lineSumHistory = state.m_lineSumHistory;
        hpp2.energyRatioPrev = RatioToHistory(state, sum, 1);  // Mirrors pPeakFlagMap[0xdf0].
        hpp2.energyRatioPrev2 = RatioToHistory(state, sum, 2);
        if (state.m_curFreqIdx == 0) {
            state.m_energyRatioF1 = hpp2.energyRatioPrev2;     // Mirrors pPeakFlagMap[0xdf2].
        } else {
            state.m_energyRatioF2 = hpp2.energyRatioPrev2;     // Mirrors pPeakFlagMap[0xdf4].
        }
        hpp2.energyRatioF1Prev2 = state.m_energyRatioF1;
        hpp2.energyRatioF2Prev2 = state.m_energyRatioF2;
    }

private:
    static uint16_t RatioToHistory(const State& state, uint32_t current, int historyIndex) {
        if (historyIndex < 0 || historyIndex >= kHistorySize) {
            return 100;
        }
        const uint32_t denom = state.m_lineSumHistory[static_cast<std::size_t>(historyIndex)];
        if (denom == 0) {
            return 100;
        }
        return static_cast<uint16_t>(std::min<uint32_t>((current * 100u) / denom, 0xffffu));
    }
};

} // namespace Solvers::Stylus::Hpp2
