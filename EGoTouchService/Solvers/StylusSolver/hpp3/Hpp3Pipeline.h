#pragma once

#include "shared/EdgeCoorProcess.hpp"
#include "AftCoorProcess.hpp"
#include "CoorIIRProcess.hpp"
#include "CoorReviseProcess.hpp"
#include "CoorSpeedProcess.hpp"
#include "CoordinateSolver.hpp"
#include "EdgeCoorPostProcess.hpp"
#include "GridFeatureExtractor.hpp"
#include "Hpp3NoisePostProcess.hpp"
#include "Hpp3PostPressureProcess.hpp"
#include "LinearFilterProcess.hpp"
#include "PressureSolver.hpp"
#include "TiltProcess.hpp"
#include "SolverTypes.h"

namespace Solvers::Stylus::Hpp3 {

// HPP3 Pipeline — Grid-mode stylus data processing.
//
// Encapsulates the full HPP3 (Grid protocol) post-processing chain as a single
// value-semantic aggregate.  The containing StylusPipeline dispatches to this
// when the frame carries HPP3 protocol flags.
//
// Process() returns false on a terminal frame (no valid stylus signal); the
// caller is responsible for shared cleanup and commit.

class Pipeline {
public:
    bool m_enabled = true;

    // ── Pipeline stages (public for ConfigKeys / direct access) ──

    GridFeatureExtractor     m_featureExtractor;
    CoordinateSolver         m_coordinateSolver;
    TiltProcess              m_tiltProcess;
    PressureSolver           m_pressureSolver;
    Hpp3PostPressureProcess  m_postPressure;
    EdgeCoorProcess          m_edgeCoorProcess;   // shared/ impl, but called mid-HPP3
    EdgeCoorPostProcess      m_edgeCoorPostProcess;
    Hpp3NoisePostProcess     m_noisePostProcess;
    LinearFilterProcess      m_linearFilterProcess;
    CoorReviseProcess        m_coorReviseProcess;
    CoorSpeedProcess         m_coorSpeedProcess;
    CoorIIRProcess           m_coorIIRProcess;
    AftCoorProcess           m_aftCoorProcess;

    // ── Main entry point ──
    //
    // Runs the full HPP3 pipeline from feature extraction through final
    // coordinate filtering.  Returns false when a terminal condition is
    // detected (stylus not present / parse failure); the caller should
    // call FinalizeTerminalFrame-style cleanup and commit.
    bool Process(HeatmapFrame& frame) {
        auto& runtime = frame.stylus.runtime;
        auto& flow = runtime.flow;

        // ── Stage 1: Grid feature extraction ──
        m_featureExtractor.Process(frame);
        if (flow.terminal) {
            return false;
        }

        // ── Stage 2: Coordinate solving (triangle + pitch) ──
        m_coordinateSolver.Process(frame);
        if (flow.terminal) {
            return false;
        }

        // ── Stage 3: Noise post-processing + tilt ──
        m_noisePostProcess.Process(frame);
        if (runtime.post.noiseRejected) {
            m_tiltProcess.Reset();
            runtime.tilt = {};
        } else {
            m_tiltProcess.Process(frame);
        }

        // ── Stage 4: Pressure pipeline ──
        m_pressureSolver.Process(frame);
        m_postPressure.Process(frame);

        // ── Stage 5: Shared edge-coordinate detection ──
        m_edgeCoorProcess.Process(frame);

        // ── Stage 6: Coordinate post-processing chain ──
        m_edgeCoorPostProcess.Process(frame);
        m_linearFilterProcess.Process(frame);
        m_coorReviseProcess.Process(frame);
        m_coorSpeedProcess.Process(frame);
        m_coorIIRProcess.Process(frame);
        m_aftCoorProcess.Process(frame);

        return true;  // non-terminal
    }

    // ── End-of-pipeline hook (called after HPP3 processing succeeds) ──
    void CaptureFinal(StylusRuntimeFrame& runtime) {
        m_edgeCoorProcess.CaptureFinal(runtime);
    }

    // ── Reset all HPP3 internal state on terminal transition ──
    void ResetOnTerminal() {
        m_tiltProcess.Reset();
        m_postPressure.Reset();
        m_edgeCoorProcess.Reset();
        m_edgeCoorPostProcess.Reset();
        m_linearFilterProcess.Reset();
        m_coorReviseProcess.Reset();
        m_coorSpeedProcess.Reset();
        m_coorIIRProcess.Reset();
        m_aftCoorProcess.Reset();
    }
};

} // namespace Solvers::Stylus::Hpp3
