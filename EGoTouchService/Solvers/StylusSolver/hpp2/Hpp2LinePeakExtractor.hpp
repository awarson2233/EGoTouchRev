#pragma once

#include "SolverTypes.h"
#include "Hpp2Runtime.hpp"
#include "Hpp2PeakSearchUtils.hpp"
#include "StylusSolver/AsaTypes.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace Solvers::Stylus::Hpp2 {

class Hpp2LinePeakExtractor {
public:
    void Process(Context& ctx) const {
        auto& runtime = ctx.runtime;
        auto& hpp2 = runtime;
        SearchPeak(ctx, 0, hpp2.line.cmnSubtracted, 0, ctx.settings.sensorTxCount,
                   ctx.state.m_peakTableDim1, ctx.state.m_peakCountDim1);
        SearchPeak(ctx, 1, hpp2.line.cmnSubtracted, ctx.settings.sensorTxCount, ctx.settings.sensorRxCount,
                   ctx.state.m_peakTableDim2, ctx.state.m_peakCountDim2);

        hpp2.selectedPeakDim1 = kInvalidPeak;
        hpp2.selectedPeakDim2 = kInvalidPeak;

        runtime.signal.signalX = 0;
        runtime.signal.signalY = 0;
        runtime.signal.maxRawPeak = 0;
        runtime.signal.recheckEnabled = true;
        runtime.signal.recheckThreshold = ctx.settings.peakSignalFloor;
        runtime.signal.recheckThresholdMulti = static_cast<uint16_t>(std::max<int>(ctx.settings.peakSignalFloor, 256));
        runtime.signal.recheckPassed = false;
        runtime.signal.dim1EdgeActive = false;
        runtime.signal.dim2EdgeActive = false;
        runtime.signal.dim1EdgeSignal = 0;
        runtime.signal.dim2EdgeSignal = 0;
    }

private:
    static void SearchPeak(const Context& ctx,
                           int groupId,
                           const std::array<uint16_t, kMaxSamples>& line,
                           int offset,
                           int length,
                           PeakTable& table,
                           int& count) {
        const auto previousTable = table;
        const int previousCount = count;
        table = {};
        count = 0;
        if (length <= 0) {
            return;
        }

        for (int i = 0; i < length; ++i) {
            if (!Hpp2PeakSearchUtils::IsLocalPeak(ctx.settings, line, offset, length, i)) {
                continue;
            }

            PeakUnit unit{};
            unit.valid = true;
            unit.index = i;
            Hpp2PeakSearchUtils::SearchPeakBoundary(line, offset, length, i, unit);
            Hpp2PeakSearchUtils::UpdatePeakPrpt(line, offset, length, unit);
            unit.candidateCoor = Hpp2PeakSearchUtils::GetPeakPos(groupId, line, offset, length, unit);
            Hpp2PeakSearchUtils::UpdatePeakNoiseFlags(ctx.settings, unit);
            unit.onEdge = unit.candidateCoor < Asa::kCoorUnit ||
                unit.candidateCoor > (length - 1) * Asa::kCoorUnit;

            if (unit.netSignal < ctx.settings.peakSignalFloor || unit.width < ctx.settings.peakMinWidth || unit.width > ctx.settings.peakMaxWidth) {
                continue;
            }
            InsertPeakUnit(unit, table, count);
        }

        UpdatePeaksAge(table, count, previousTable, previousCount);
    }

    static void InsertPeakUnit(const PeakUnit& unit,
                               PeakTable& table,
                               int& count) {
        int slot = count < kMaxPeaksPerDim ? count : -1;
        if (slot < 0) {
            uint16_t weakest = table[0].peakSignal;
            slot = 0;
            for (int i = 1; i < kMaxPeaksPerDim; ++i) {
                if (table[static_cast<std::size_t>(i)].peakSignal < weakest) {
                    weakest = table[static_cast<std::size_t>(i)].peakSignal;
                    slot = i;
                }
            }
            if (unit.peakSignal <= weakest) {
                return;
            }
        } else {
            ++count;
        }
        table[static_cast<std::size_t>(slot)] = unit;
    }

    static void UpdatePeaksAge(PeakTable& table,
                               int count,
                               const PeakTable& previousTable,
                               int previousCount) {
        // Intentional simplification for HPP2 line mode: inherit only peak age.
        // TSACore also carries long-scale IIR/average/min/max history fields that
        // feed HPP3 grid feature smoothing, which is not required on this path.
        for (int i = 0; i < count; ++i) {
            auto& unit = table[static_cast<std::size_t>(i)];
            for (int j = 0; j < previousCount; ++j) {
                const auto& previous = previousTable[static_cast<std::size_t>(j)];
                if (!previous.valid) {
                    continue;
                }
                const int delta = unit.index - previous.index;
                if (delta >= -2 && delta <= 2) {
                    unit.age = std::min(previous.age + 1, 0xfff5);
                    break;
                }
            }
        }
    }
};

} // namespace Solvers::Stylus::Hpp2
