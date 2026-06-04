#pragma once

#include "SolverTypes.h"
#include "Hpp2Runtime.hpp"

#include <algorithm>
#include <cstdint>

namespace Solvers::Stylus::Hpp2 {

class Hpp2PressureProcess {
public:
    void Process(Context& ctx) const {
        FilterPressure(ctx);
        ApplyEdgePressureGuard(ctx);
    }

    static void PublishPressure(HeatmapFrame& frame) {
        auto& runtime = frame.stylus.runtime.hpp2;
        runtime.post.finalPressure = runtime.pressure.outputPressure;
        runtime.post.point.rawPressure = runtime.pressure.rawPressure;
        runtime.post.point.mappedPressure = runtime.pressure.mappedPressure;
        runtime.post.point.pressure = runtime.pressure.outputPressure;
    }

private:
    static void FilterPressure(Context& ctx) {
        auto& runtime = ctx.runtime;
        auto& pressure = runtime.pressure;
        const uint16_t raw = static_cast<uint16_t>(std::min<uint32_t>(runtime.rawPressure, 0x0fffu));
        uint16_t output = raw;
        if (output != 0 && ctx.state.m_prevPressure != 0) {
            output = LimitPressureDelta(ctx.state.m_prevPressure, output,
                                        ctx.settings.useTightPressureDelta ? ctx.settings.pressureDeltaTight : ctx.settings.pressureDeltaNormal);
            output = PressureIir(ctx.state.m_prevPressure, output, 0x40);
        }

        pressure.pressureIsReal = true;
        pressure.rawPressure = raw;
        pressure.mappedPressure = raw;
        pressure.outputPressure = output;
        pressure.lookaheadHoverGate = false;
        pressure.predictedAgeFrames = 0;
    }

    static void ApplyEdgePressureGuard(Context& ctx) {
        auto& runtime = ctx.runtime;
        auto& pressure = runtime.pressure;
        if (pressure.outputPressure == 0) {
            ctx.state.m_edgeSignalTooLowLatched = false;
            return;
        }

        const bool dim1Enter = runtime.signal.dim1EdgeActive &&
            runtime.signal.dim1EdgeSignal < ctx.settings.pressureEdgeEnterThreshold;
        const bool dim2Enter = runtime.signal.dim2EdgeActive &&
            runtime.signal.dim2EdgeSignal < ctx.settings.pressureEdgeEnterThreshold;
        if (!ctx.state.m_edgeSignalTooLowLatched && (dim1Enter || dim2Enter)) {
            ctx.state.m_edgeSignalTooLowLatched = true;
        }

        const bool dim1Clear = !runtime.signal.dim1EdgeActive ||
            runtime.signal.dim1EdgeSignal > ctx.settings.pressureEdgeExitThreshold;
        const bool dim2Clear = !runtime.signal.dim2EdgeActive ||
            runtime.signal.dim2EdgeSignal > ctx.settings.pressureEdgeExitThreshold;
        if (ctx.state.m_edgeSignalTooLowLatched && dim1Clear && dim2Clear) {
            ctx.state.m_edgeSignalTooLowLatched = false;
        }

        if (ctx.state.m_edgeSignalTooLowLatched) {
            pressure.outputPressure = 0;
        }
#if EGOTOUCH_DIAG
        pressure.edgeSignalTooLowLatched = ctx.state.m_edgeSignalTooLowLatched;
#endif
    }

    static uint16_t LimitPressureDelta(uint16_t previous, uint16_t current, uint16_t maxDelta) {
        if (current > previous) {
            return static_cast<uint16_t>(std::min<uint32_t>(current, static_cast<uint32_t>(previous) + maxDelta));
        }
        return static_cast<uint16_t>(current + maxDelta < previous ? previous - maxDelta : current);
    }

    static uint16_t PressureIir(uint16_t previous, uint16_t current, uint8_t alpha) {
        const uint32_t mixed = static_cast<uint32_t>(previous) * (0x80u - alpha) +
                               static_cast<uint32_t>(current) * alpha;
        return static_cast<uint16_t>(std::min<uint32_t>(mixed >> 7, 0x0fffu));
    }
};

} // namespace Solvers::Stylus::Hpp2
