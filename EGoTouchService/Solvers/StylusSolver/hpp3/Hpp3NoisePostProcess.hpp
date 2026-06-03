#pragma once

#include "StylusSolver/AsaTypes.hpp"
#include "SolverTypes.h"

#include <algorithm>
#include <cstdint>

namespace Solvers::Stylus {

// HPP3_NoisePostProcess — post-validates stylus signal quality after coordinate extraction.
//
// Replicates TSACore HPP3_NoisePostProcess (0x6bab9511).
//
// Three gates, each clearing peak-valid flags on failure:
//   1. Signal ratio   — |X:Y| > 5:1  ⇒  ratio anomaly
//   2. Signal drop    — cur * 5 < prevStable  ⇒  signal too small
//   3. Coordinate jump — |TX1−TX2| > 0x1400  (requires TX2 coor; skipped for now)
//
// When noise is rejected the current coordinate is frozen to the last valid frame,
// mirroring the original's memcpy(curASOut, prevASOut, 0xec) under bBypassCurFrame.

class Hpp3NoisePostProcess {
public:
    bool m_enabled = true;

    uint8_t m_signalRatioThreshold = 5;      // 5:1
    uint8_t m_signalMagnitudeDropRatio = 5;  // 5× drop
    int32_t m_coorJumpThreshold = 0x1400;    // TX1−TX2 jump gate (needs TX2 coor)

    // ── State reset ──

    inline void Reset() {
        m_haveStableSignal = false;
        m_stableSignalX = 0;
        m_stableSignalY = 0;
        m_ratioAnomalyCntX = 0;
        m_ratioAnomalyCntY = 0;
        m_prevValidCoor = {};
        m_havePrevValidCoor = false;
    }

    // ── Per-frame processing ──

    inline void Process(HeatmapFrame& frame) {
        auto& runtime = frame.stylus.runtime;

        // ── Clear outputs ──
        runtime.post.noiseRejected = false;
        runtime.post.noiseRejectReason = 0;
        runtime.post.freqBypassed = false;
#if EGOTOUCH_DIAG
        runtime.post.noiseValidDim1 = true;
        runtime.post.noiseValidDim2 = true;
        runtime.post.ratioAnomalyCntDim1 = m_ratioAnomalyCntX;
        runtime.post.ratioAnomalyCntDim2 = m_ratioAnomalyCntY;
        runtime.post.coorJumpDim1 = 0;
        runtime.post.coorJumpDim2 = 0;
#endif

        if (!m_enabled) {
            SnapshotCoordinate(runtime);
            return;
        }

        const auto& coor = runtime.tx1.coordinate.reportGlobalCoor;
        if (!coor.valid) {
            SnapshotCoordinate(runtime);
            return;
        }

        const uint16_t signalX = runtime.signal.signalX;
        const uint16_t signalY = runtime.signal.signalY;
        bool peakValidDim1 = true;
        bool peakValidDim2 = true;
        uint8_t rejectReason = 0;

        // ═══════════════════════════════════════════════════════════
        // Gate 1 — Signal ratio abnormal
        // TSACore: signalY * 5 < signalX  ||  signalX * 5 < signalY
        // ═══════════════════════════════════════════════════════════
        if (static_cast<uint32_t>(signalY) * m_signalRatioThreshold < signalX ||
            static_cast<uint32_t>(signalX) * m_signalRatioThreshold < signalY) {
            peakValidDim1 = false;
            peakValidDim2 = false;
            rejectReason |= 1;
        }

        // ═══════════════════════════════════════════════════════════
        // Gate 2 — Current signal too small vs. stable history
        // TSACore: curSignal * 5 < prevStableSignal  (when in-range)
        // ═══════════════════════════════════════════════════════════
        if (m_haveStableSignal) {
            if (static_cast<uint32_t>(signalX) * m_signalMagnitudeDropRatio < m_stableSignalX ||
                static_cast<uint32_t>(signalY) * m_signalMagnitudeDropRatio < m_stableSignalY) {
                peakValidDim1 = false;
                peakValidDim2 = false;
                rejectReason |= 2;
            }
        }

        // ═══════════════════════════════════════════════════════════
        // Gate 3 — Coordinate jump (TX1 vs TX2)
        // ═══════════════════════════════════════════════════════════
        const auto tx2Coor = ResolveTx2GlobalCoor(runtime);
        if (tx2Coor.valid) {
            const uint32_t jumpDim1 = AbsDiff(coor.dim1, tx2Coor.dim1);
            const uint32_t jumpDim2 = AbsDiff(coor.dim2, tx2Coor.dim2);
#if EGOTOUCH_DIAG
            runtime.post.coorJumpDim1 = static_cast<int32_t>(jumpDim1);
            runtime.post.coorJumpDim2 = static_cast<int32_t>(jumpDim2);
#endif
            if (jumpDim1 > static_cast<uint32_t>(m_coorJumpThreshold) ||
                jumpDim2 > static_cast<uint32_t>(m_coorJumpThreshold)) {
                peakValidDim1 = false;
                peakValidDim2 = false;
                rejectReason |= 4;
            }
        }

        // ── Moderate asymmetry accumulator (diagnostic, not gating) ──
        // TSACore: signalY * 1.5 < signalX  ||  signalX * 1.5 < signalY
        if (static_cast<uint32_t>(signalY) * 3 < static_cast<uint32_t>(signalX) * 2 ||
            static_cast<uint32_t>(signalX) * 3 < static_cast<uint32_t>(signalY) * 2) {
            ++m_ratioAnomalyCntX;
            ++m_ratioAnomalyCntY;
        }

        // ── Update stable history when all gates pass ──
        const bool allPassed = peakValidDim1 && peakValidDim2;
        if (allPassed) {
            m_stableSignalX = signalX;
            m_stableSignalY = signalY;
            m_haveStableSignal = true;
        }

#if EGOTOUCH_DIAG
        runtime.post.noiseValidDim1 = peakValidDim1;
        runtime.post.noiseValidDim2 = peakValidDim2;
        runtime.post.ratioAnomalyCntDim1 = m_ratioAnomalyCntX;
        runtime.post.ratioAnomalyCntDim2 = m_ratioAnomalyCntY;
#endif
        runtime.post.noiseRejected = !allPassed;
        runtime.post.noiseRejectReason = rejectReason;

        // ── Freeze coordinate on noise — mirrors TSACore bBypassCurFrame ──
        FreezeCoordinate(runtime, !allPassed);
    }

private:
    bool     m_haveStableSignal  = false;
    uint16_t m_stableSignalX     = 0;
    uint16_t m_stableSignalY     = 0;
    uint8_t  m_ratioAnomalyCntX  = 0;
    uint8_t  m_ratioAnomalyCntY  = 0;
    Asa::AsaCoorResult m_prevValidCoor{};
    bool     m_havePrevValidCoor = false;

    inline void SnapshotCoordinate(const StylusRuntimeFrame& runtime) {
        const auto& coor = runtime.tx1.coordinate.reportGlobalCoor;
        if (coor.valid) {
            m_prevValidCoor = coor;
            m_havePrevValidCoor = true;
        }
    }

    inline void FreezeCoordinate(StylusRuntimeFrame& runtime, bool freezeActive) {
        auto& coor = runtime.tx1.coordinate.reportGlobalCoor;

        if (!freezeActive) {
            if (coor.valid) {
                m_prevValidCoor = coor;
                m_havePrevValidCoor = true;
            }
            return;
        }

        if (m_havePrevValidCoor) {
            coor = m_prevValidCoor;
        }
    }

    static constexpr int kAnchorCenterOffset = Asa::kGridDim / 2;
    static constexpr int kSensorTxCount = 60;
    static constexpr int kSensorRxCount = 40;

    static inline Asa::AsaCoorResult ResolveTx2GlobalCoor(const StylusRuntimeFrame& runtime) {
        if (runtime.tx2.coordinate.reportGlobalCoor.valid) {
            return runtime.tx2.coordinate.reportGlobalCoor;
        }
        Asa::AsaCoorResult coor = runtime.tx2.feature.refinedLocalCoor;
        if (!coor.valid) return coor;
        LocalToGlobal(coor,
                      runtime.rawGrid.asaGrid.tx2.anchorRow,
                      runtime.rawGrid.asaGrid.tx2.anchorCol,
                      kAnchorCenterOffset);
        coor.dim1 = std::clamp(coor.dim1, 0, kSensorTxCount * Asa::kCoorUnit - 1);
        coor.dim2 = std::clamp(coor.dim2, 0, kSensorRxCount * Asa::kCoorUnit - 1);
        return coor;
    }

    static inline void LocalToGlobal(Asa::AsaCoorResult& coor,
                                     int anchorRow,
                                     int anchorCol,
                                     int anchorCenterOffset) {
        if (!coor.valid) return;
        const int32_t centerOff = static_cast<int32_t>(anchorCenterOffset) * Asa::kCoorUnit;
        coor.dim1 += static_cast<int32_t>(anchorCol) * Asa::kCoorUnit - centerOff;
        coor.dim2 += static_cast<int32_t>(anchorRow) * Asa::kCoorUnit - centerOff;
    }

    static inline uint32_t AbsDiff(int32_t a, int32_t b) {
        return a > b
            ? static_cast<uint32_t>(a - b)
            : static_cast<uint32_t>(b - a);
    }
};

} // namespace Solvers::Stylus
