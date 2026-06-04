#pragma once

#include "CoordinateSolver.hpp"
#include "GridFeatureExtractor.hpp"
#include "Hpp3NoisePostProcess.hpp"
#include "Hpp3PostPressureProcess.hpp"
#include "PressureSolver.hpp"
#include "TiltProcess.hpp"
#include "Hpp3Runtime.hpp"
#include "SolverTypes.h"

namespace Solvers::Stylus::Hpp3 {

// HPP3 Pipeline — Grid-mode stylus data processing.
//
// Encapsulates HPP3-only stages. Shared stateful stages that TSACore routes
// through ASA_CoorPostProcess (edge carry, linear/coor revise/speed/IIR/AFT)
// live in the containing StylusPipeline so HPP2 can reuse them.
//
// Process() returns false on a terminal frame (no valid stylus signal); the
// caller is responsible for shared cleanup and commit.
class Pipeline {
public:
    bool m_enabled = true;

    // ── HPP3-specific stages (public for ConfigKeys / direct access) ──
    GridFeatureExtractor     m_featureExtractor;
    CoordinateSolver         m_coordinateSolver;
    TiltProcess              m_tiltProcess;
    PressureSolver           m_pressureSolver;
    Hpp3PostPressureProcess  m_postPressure;
    Hpp3NoisePostProcess     m_noisePostProcess;

    // ── Main HPP3 entry point before shared edge/common post ──
    bool Process(HeatmapFrame& frame) {
        auto& runtime = frame.stylus.runtime.SelectHpp3();
        auto& flow = runtime.flow;
        const Settings settings{m_enabled};
        Context ctx{frame, runtime, settings, m_state};

        // ── Stage 1: Grid feature extraction ──
        m_featureExtractor.Process(ctx);
        if (flow.terminal) {
            return false;
        }

        // ── Stage 2: Coordinate solving (triangle + pitch) ──
        m_coordinateSolver.Process(ctx);
        if (flow.terminal) {
            return false;
        }

        // ── Stage 3: Noise post-processing + tilt ──
        m_noisePostProcess.Process(ctx);
        if (runtime.post.noiseRejected) {
            m_tiltProcess.Reset();
            runtime.tilt = {};
        } else {
            m_tiltProcess.Process(ctx);
        }

        // ── Stage 4: Pressure pipeline ──
        m_pressureSolver.Process(ctx);
        m_postPressure.Process(ctx);

        return true;  // non-terminal; caller runs shared edge/common post
    }

    // ── Reset HPP3-specific internal state on terminal transition ──
    void ResetOnTerminal() {
        m_tiltProcess.Reset();
        m_postPressure.Reset();
    }

private:
    State m_state{};
};

} // namespace Solvers::Stylus::Hpp3
