#pragma once

#include "Hpp2Runtime.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

namespace Solvers::Stylus::Hpp2 {

class Hpp2CmfProcess {
public:
    void Process(Context& ctx) const {
        ctx.state.m_cmnSum = {};
        ctx.state.m_cmnMin = {{0xffff, 0xffff}};
        ProcessGroup(ctx, 0, 0, ctx.settings.sensorTxCount);
        ProcessGroup(ctx, 1, ctx.settings.sensorTxCount, ctx.settings.sensorRxCount);
    }

private:
    static void ProcessGroup(Context& ctx, int group, int offset, int length) {
        auto& hpp2 = ctx.runtime;
        std::array<uint16_t, kMaxSamples> slidingMin{};
        for (int i = 0; i < length; ++i) {
            uint16_t minValue = 0xffff;
            const int start = std::max(0, i - ctx.settings.cmfWindowRadius);
            const int end = std::min(length - 1, i + ctx.settings.cmfWindowRadius);
            for (int j = start; j <= end; ++j) {
                minValue = std::min(minValue, hpp2.line.raw[static_cast<std::size_t>(offset + j)]);
            }
            slidingMin[static_cast<std::size_t>(i)] = minValue;
        }

        uint32_t cmnSum = 0;
        uint16_t cmnMin = 0xffff;
        for (int i = 0; i < length; ++i) {
            uint16_t maxValue = 0;
            const int start = std::max(0, i - ctx.settings.cmfWindowRadius);
            const int end = std::min(length - 1, i + ctx.settings.cmfWindowRadius);
            for (int j = start; j <= end; ++j) {
                maxValue = std::max(maxValue, slidingMin[static_cast<std::size_t>(j)]);
            }
            const int idx = offset + i;
            const uint16_t raw = hpp2.line.raw[static_cast<std::size_t>(idx)];
            hpp2.line.cmnBaseline[static_cast<std::size_t>(idx)] = maxValue;
            hpp2.line.cmnSubtracted[static_cast<std::size_t>(idx)] = raw > maxValue ? static_cast<uint16_t>(raw - maxValue) : 0;
            cmnSum += maxValue;
            cmnMin = std::min(cmnMin, maxValue);
        }
        ctx.state.m_cmnSum[static_cast<std::size_t>(group)] = cmnSum;
        ctx.state.m_cmnMin[static_cast<std::size_t>(group)] = cmnMin;
    }
};

} // namespace Solvers::Stylus::Hpp2
