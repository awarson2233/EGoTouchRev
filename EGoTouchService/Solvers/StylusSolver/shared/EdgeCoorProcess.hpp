#pragma once

#include "StylusSolver/AsaTypes.hpp"
#include "SolverTypes.h"

#include <cstdint>
#include <cstdlib>

namespace Solvers::Stylus {

// EdgeCoorProcess – high-speed edge-exit detection.
//
// Replicates TSACore EdgeCoorProcess (0x6baaf250).
//
// The original computes an intra-frame delta between reused ASOutRuntime
// coordinate slots. Those pInner slots are stage scratch space, not stable
// semantic fields; pInner[3], for example, is written by more than one stage.
//
// This port keeps semantic snapshots instead:
//   prevPreFilter  = coordinate entering post-processing
//   prevPostFilter = coordinate after the active post-filter chain
//
// When |prevPostFilter - prevPreFilter| > 0x200, the filter chain made a large
// correction, indicating edge proximity. Combined with pen-up in the current
// frame and the peak sitting on a grid boundary, this signals a high-speed
// edge exit. The module then carries the previous frame's pressure into the
// current frame so downstream stages still treat it as "pen down."
//
// Process() is called mid-pipeline after PostPressure. It captures the
// pre-filter coordinate and performs detection using the stored previous-frame
// snapshots.
//
// CaptureFinal() stores the post-filter coordinate from the end of the active
// coordinate filter chain. With AFT disabled, this is post-IIR; if AFT is
// re-enabled, capture placement should be revisited intentionally.

class EdgeCoorProcess {
public:
    bool m_enabled = true;

    // Sensor dimensions – matches asa 0xa28 (bTxCount=60, bRxCount=40).
    int m_sensorTxCount = 60;
    int m_sensorRxCount = 40;

    inline void Reset() {
        m_firstRelease = true;
        m_needHighSpeed = false;

        m_prevPreFilterDim1 = 0;
        m_prevPreFilterDim2 = 0;
        m_prevPostFilterDim1 = 0;
        m_prevPostFilterDim2 = 0;
        m_prevPressure = 0;

        m_nextPreFilterDim1 = 0;
        m_nextPreFilterDim2 = 0;
    }

    // ── Mid-pipeline: detect edge exit using previous-frame snapshots ──

    inline void Process(HeatmapFrame& frame) {
        auto& runtime = frame.stylus.runtime;

        m_needHighSpeed = false;
        runtime.decision.enableEdgeCorrect = false;

        if (!m_enabled) {
            SnapshotPreFilter(runtime);
            return;
        }

        const auto& coor = runtime.tx1.coordinate.reportGlobalCoor;
        const uint16_t curPressure = runtime.pressure.outputPressure;

        // Capture current-frame pre-filter coordinate for next iteration.
        SnapshotPreFilter(runtime);

        if (!coor.valid) {
            m_firstRelease = true;
            return;
        }

        // ── Intra-frame delta (|pInnerX[3] - pInnerX[0]| from previous frame) ──
        const int32_t xDist = std::abs(m_prevPostFilterDim1 - m_prevPreFilterDim1);
        const int32_t yDist = std::abs(m_prevPostFilterDim2 - m_prevPreFilterDim2);

        const int32_t halfUnit   = Asa::kCoorUnit / 2;       // 0x200
        const int32_t sensorMaxX = m_sensorTxCount * Asa::kCoorUnit;
        const int32_t sensorMaxY = m_sensorRxCount * Asa::kCoorUnit;

        // ── X-axis edge high-speed exit ──
        const bool xEdge =
            m_firstRelease &&
            runtime.signal.dim1EdgeActive &&
            curPressure == 0 &&
            m_prevPressure != 0 &&
            xDist > halfUnit &&
            m_prevPostFilterDim1 > halfUnit &&
            m_prevPostFilterDim1 < sensorMaxX - halfUnit;

        if (xEdge) {
            m_needHighSpeed = true;
            m_firstRelease = false;
        }

        // ── Y-axis edge high-speed exit ──
        const bool yEdge =
            m_firstRelease &&
            runtime.signal.dim2EdgeActive &&
            curPressure == 0 &&
            m_prevPressure != 0 &&
            yDist > halfUnit &&
            m_prevPostFilterDim2 > halfUnit &&
            m_prevPostFilterDim2 < sensorMaxY - halfUnit;

        if (yEdge) {
            m_needHighSpeed = true;
            m_firstRelease = false;
        }

        // Track pen-down for next frame (original: if curPressure != 0 → firstRelease = 1)
        if (runtime.pressure.outputPressure != 0) {
            m_firstRelease = true;
        }

        // Carry previous pressure into current frame on high-speed exit.
        if (m_needHighSpeed) {
            runtime.pressure.outputPressure = m_prevPressure;
            runtime.decision.enableEdgeCorrect = true;
        }
    }

    // ── End-of-pipeline: store fully-filtered coordinate and final pressure ──

    inline void CaptureFinal(const StylusRuntimeFrame& runtime) {
        m_prevPreFilterDim1  = m_nextPreFilterDim1;
        m_prevPreFilterDim2  = m_nextPreFilterDim2;
        m_prevPostFilterDim1 = runtime.post.finalCoor.dim1;
        m_prevPostFilterDim2 = runtime.post.finalCoor.dim2;
        m_prevPressure       = runtime.pressure.outputPressure;
    }

private:
    // Previous-frame snapshots (used by Process for detection).
    bool     m_firstRelease       = true;
    bool     m_needHighSpeed      = false;
    int32_t  m_prevPreFilterDim1  = 0;
    int32_t  m_prevPreFilterDim2  = 0;
    int32_t  m_prevPostFilterDim1 = 0;
    int32_t  m_prevPostFilterDim2 = 0;
    uint16_t m_prevPressure       = 0;

    // Staging buffer: pre-filter coordinate captured now, promoted in CaptureFinal.
    int32_t  m_nextPreFilterDim1 = 0;
    int32_t  m_nextPreFilterDim2 = 0;

    inline void SnapshotPreFilter(const StylusRuntimeFrame& runtime) {
        const auto& coor = runtime.tx1.coordinate.reportGlobalCoor;
        m_nextPreFilterDim1 = coor.dim1;
        m_nextPreFilterDim2 = coor.dim2;
    }
};

} // namespace Solvers::Stylus
