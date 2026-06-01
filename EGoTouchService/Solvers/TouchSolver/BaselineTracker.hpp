#pragma once
// ══════════════════════════════════════════════════════════════════════
// BaselineTracker — Per-cell adaptive baseline subtraction with
// Q8.24 fixed-point IIR tracking, freeze/release-hold state machine,
// and aggressive recovery on finger-state transitions.
//
// Finger state is a simple bool (hasFinger), determined externally from
// the master suffix. The module makes no distinction between Unknown
// and NoFinger — it only cares whether a finger is present right now.
//
// Baseline is INHERITED across lid/display/idle state changes; only
// the very first frame or an explicit Reset() reinitializes from the
// configured default value.
// ══════════════════════════════════════════════════════════════════════

#include "SolverTypes.h"
#include <algorithm>
#include <array>
#include <cstdint>

namespace Solvers { namespace Touch {

class BaselineTracker {
public:
    static constexpr int kRows = 40;
    static constexpr int kCols = 60;
    static constexpr int kCellCount = kRows * kCols;

    // ── Public configurable parameters ────────────────────────────
    //  (exposed as public members for ConfigParam registration via
    //   const_cast, consistent with all other pipeline modules)

    bool m_enabled = true;

    // Initial baseline value in ADC units (0..65535).
    // Applied only on Initialize() or Reset().
    int  m_baseline = 0x7FEE;               // 32750

    // Deadband width: |delta| ≤ this is treated as noise.
    int  m_noiseDeadband = 90;              // 0..200

    // Thresholds for the three-tier IIR classification.
    // positiveDeadband: delta above this enters fast-positive tier.
    // negativeDeadband: delta below -this enters fast-negative tier;
    //                   also gates negative escape during release hold.
    int  m_positiveDeadband = 14;            // 0..200
    int  m_negativeDeadband = 13;            // 0..200

    // Freeze threshold: localDiff above this locks the cell's baseline.
    int  m_peakThreshold = 305;              // 1..2000

    // Number of frames to hold after a freeze cell is released,
    // protecting against baseline absorbing the negative rebound.
    int  m_releaseHoldFrames = 60;           // 0..255

    // ── IIR alpha-shift parameters (effective alpha = 2^(-shift)) ──
    // Higher shift = slower convergence.

    int  m_positiveAlphaShift = 7;           // Temperature drift (slow)
    int  m_negativeAlphaShift = 5;           // Release artifact (medium)
    int  m_noiseAlphaShift = 6;              // Within-noise tracking (slow)
    int  m_backgroundAlphaShift = 3;         // Background cell normal (moderate)
    int  m_noFingerAlphaShift = 3;           // No-finger mode (aggressive)

    // ── Per-frame step clamps (in ADC units, applied after Q8 conversion) ──
    int  m_positiveMaxStep = 20;             // 0..200
    int  m_negativeMaxStep = 20;             // 0..200
    int  m_backgroundMaxStep = 512;          // 1..2048
    int  m_noFingerMaxStep = 512;            // 1..2048

    // ── Aggressive recovery mode (false→true hasFinger transition) ──
    int  m_recoveryAlphaShift = 2;           // Recovery IIR speed (very fast)
    int  m_recoveryMaxStep = 256;            // Recovery step clamp
    int  m_recoveryMaxFrames = 30;           // Hard frame limit for recovery

    // Whether to apply minimal IIR within the deadband zone.
    bool m_noiseTrackingEnabled = true;

    // ═══════════════════════════════════════════════════════════════
    // Public API
    // ═══════════════════════════════════════════════════════════════

    /// Process one frame of heatmap data.
    /// @param frame   Heatmap frame (heatmapMatrix modified in-place).
    /// @param hasFinger  true when master suffix reports finger present.
    /// @return always true (reserved for future error reporting).
    inline bool Process(HeatmapFrame& frame, bool hasFinger) {
        if (!m_enabled) {
            return true;
        }

        if (!m_initialized) {
            Initialize();
        }

        // ── Guard: invalid master data → safe zero output, reset transient state ──
        const bool masterValid = frame.masterWasRead && frame.masterSuffixValid;
        if (!masterValid) {
            int16_t* const cells = &frame.heatmapMatrix[0][0];
            ZeroOutput(cells);
            m_freezeCellMask.fill(0);
            m_releaseHold.fill(0);
            m_prevHadFinger = false;
            m_hadFreezeLastFrame = false;
            m_recoveryFrameCounter = 0;
            return true;
        }

        // ── Dispatch ──
        if (hasFinger) {
            ProcessFinger(frame);
        } else {
            ProcessNoFinger(frame);
        }

        m_prevHadFinger = hasFinger;
        return true;
    }

    /// Reset all state. Next Process() call will re-Initialize from m_baseline.
    inline void Reset() {
        m_initialized = false;
        m_prevHadFinger = false;
        m_hadFreezeLastFrame = false;
        m_recoveryFrameCounter = 0;
        m_baselineQ8.fill(0);
        m_releaseHold.fill(0);
        m_freezeCellMask.fill(0);
    }

private:
    static constexpr int kBaselineFractionBits = 8;

    // ── Persistent state ──
    bool m_initialized = false;
    bool m_prevHadFinger = false;
    bool m_hadFreezeLastFrame = false;
    int  m_recoveryFrameCounter = 0;

    // ── SoA data arrays (cache-friendly linear traversal) ──
    // baselineQ8: Q8.24 fixed-point  (int32_t, 8 fractional bits)
    // releaseHold: remaining hold frames per cell
    // freezeCellMask: diagnostic flag (1 = cell is frozen)
    std::array<int32_t, kCellCount> m_baselineQ8{};
    std::array<uint8_t, kCellCount> m_releaseHold{};
    std::array<uint8_t, kCellCount> m_freezeCellMask{};

    // ═══════════════════════════════════════════════════════════════
    // Initialization
    // ═══════════════════════════════════════════════════════════════

    inline void Initialize() {
        const int initialBaseline = std::clamp(m_baseline, 0, 0xFFFF);
        m_baselineQ8.fill(static_cast<int32_t>(initialBaseline) << kBaselineFractionBits);
        m_releaseHold.fill(0);
        m_freezeCellMask.fill(0);
        m_prevHadFinger = false;
        m_hadFreezeLastFrame = false;
        m_recoveryFrameCounter = 0;
        m_initialized = true;
    }

    // ═══════════════════════════════════════════════════════════════
    // ProcessNoFinger — all cells update baseline, all output zero
    // ═══════════════════════════════════════════════════════════════

    inline void ProcessNoFinger(HeatmapFrame& frame) {
        int16_t* const cells = &frame.heatmapMatrix[0][0];

        for (int i = 0; i < kCellCount; ++i) {
            m_releaseHold[i] = 0;
            m_freezeCellMask[i] = 0;

            const int raw   = static_cast<int>(RawCell(cells[i]));
            const int baseline = m_baselineQ8[i] >> kBaselineFractionBits;
            const int delta = raw - baseline;

            if (std::abs(delta) <= m_noiseDeadband) {
                if (m_noiseTrackingEnabled && delta != 0) {
                    UpdateBaseline(i, delta, m_noiseAlphaShift, 1);
                }
            } else {
                UpdateBaseline(i, delta, m_noFingerAlphaShift, m_noFingerMaxStep);
            }

            cells[i] = 0;
        }

        m_hadFreezeLastFrame = false;
        m_recoveryFrameCounter = 0;
    }

    // ═══════════════════════════════════════════════════════════════
    // ProcessFinger — per-cell freeze vs background dispatch
    // ═══════════════════════════════════════════════════════════════

    inline void ProcessFinger(HeatmapFrame& frame) {
        int16_t* const cells = &frame.heatmapMatrix[0][0];

        // ── Common-mode rejection: median of all (raw - baseline) ──
        const int commonDiff = EstimateDiffMedian(cells);

        // ── Determine recovery mode ──
        // Recovery activates on:
        //   1. false→true hasFinger transition (baseline needs fast adaptation)
        //   2. No freeze cells in previous frame (finger present but signal weak)
        // Recovery deactivates when:
        //   - A freeze cell appears (next frame)
        //   - Frame counter exceeds m_recoveryMaxFrames
        bool inRecovery = false;

        if (!m_prevHadFinger) {
            inRecovery = true;
            m_recoveryFrameCounter = 0;
        } else if (!m_hadFreezeLastFrame) {
            inRecovery = true;
            ++m_recoveryFrameCounter;
            if (m_recoveryFrameCounter >= m_recoveryMaxFrames) {
                inRecovery = false;
            }
        }

        bool foundFreezeThisFrame = false;

        // ── Per-cell dispatch ──
        for (int i = 0; i < kCellCount; ++i) {
            const int raw      = static_cast<int>(RawCell(cells[i]));
            const int baseline = m_baselineQ8[i] >> kBaselineFractionBits;
            const int delta    = raw - baseline;
            const int localDiff = delta - commonDiff;

            if (localDiff > m_peakThreshold) {
                // ── FREEZE ──
                foundFreezeThisFrame = true;
                m_freezeCellMask[i] = 1;
                m_releaseHold[i] = static_cast<uint8_t>(
                    std::clamp(m_releaseHoldFrames, 0, 255));

                // Track common-mode shift for frozen cell (keep baseline
                // aligned with global panel changes)
                if (commonDiff != 0) {
                    UpdateBaseline(i, commonDiff,
                                   m_backgroundAlphaShift,
                                   m_backgroundMaxStep);
                }

                cells[i] = SaturateInt16(localDiff);
            } else {
                // ── BACKGROUND ──
                m_freezeCellMask[i] = 0;

                if (m_releaseHold[i] > 0) {
                    --m_releaseHold[i];

                    // Negative escape during release hold:
                    // If the local signal drops sharply (finger lifting),
                    // pass the negative value through instead of absorbing
                    // it into the baseline.
                    if (localDiff < -m_negativeDeadband) {
                        cells[i] = SaturateInt16(localDiff);
                        continue;
                    }
                }

                BackgroundBaselineUpdate(i, delta, inRecovery);
                cells[i] = 0;
            }
        }

        m_hadFreezeLastFrame = foundFreezeThisFrame;
    }

    // ═══════════════════════════════════════════════════════════════
    // BackgroundBaselineUpdate — three-tier adaptive IIR
    // ═══════════════════════════════════════════════════════════════

    inline void BackgroundBaselineUpdate(int index, int delta, bool inRecovery) {
        // Recovery mode: bypass tier classification, use uniform fast alpha
        if (inRecovery) {
            UpdateBaseline(index, delta, m_recoveryAlphaShift, m_recoveryMaxStep);
            return;
        }

        const int absDelta = std::abs(delta);

        // Tier 1: Deadband — skip or minimal noise tracking
        if (absDelta <= m_noiseDeadband) {
            if (m_noiseTrackingEnabled && delta != 0) {
                UpdateBaseline(index, delta, m_noiseAlphaShift, 1);
            }
            return;
        }

        // Tier 2+3: Beyond deadband — classify by delta direction and magnitude
        if (delta > m_positiveDeadband) {
            // Large positive: temperature / ambient drift (slow, clamped)
            UpdateBaseline(index, delta, m_positiveAlphaShift, m_positiveMaxStep);
        } else if (delta < -m_negativeDeadband) {
            // Large negative: finger release artifact (medium speed)
            UpdateBaseline(index, delta, m_negativeAlphaShift, m_negativeMaxStep);
        } else {
            // Normal range: moderate noise-level tracking
            UpdateBaseline(index, delta, m_noiseAlphaShift, m_backgroundMaxStep);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // UpdateBaseline — Q8.24 fixed-point IIR core
    // ═══════════════════════════════════════════════════════════════

    inline void UpdateBaseline(int index, int delta, int alphaShift, int maxStep) {
        const int shift = std::clamp(alphaShift, 0, 15);
        const int32_t maxStepQ8 = static_cast<int32_t>(std::max(0, maxStep)) << kBaselineFractionBits;

        int32_t updateQ8 = (static_cast<int32_t>(delta) << kBaselineFractionBits) >> shift;

        if (maxStepQ8 > 0) {
            updateQ8 = std::clamp(updateQ8, -maxStepQ8, maxStepQ8);
        }

        auto& baseline = m_baselineQ8[index];
        baseline = std::clamp(baseline + updateQ8,
                              0,
                              0xFFFF << kBaselineFractionBits);
    }

    // ═══════════════════════════════════════════════════════════════
    // Common-mode estimation
    // ═══════════════════════════════════════════════════════════════

    // Computes the median of (raw - baseline) across all cells.
    // Used as commonDiff to remove global panel-wide shifts (temperature,
    // display state, VCOM noise) from the freeze threshold comparison.
    inline int EstimateDiffMedian(int16_t* cells) const {
        std::array<int, kCellCount> diffs{};
        for (int i = 0; i < kCellCount; ++i) {
            const int baseline = m_baselineQ8[i] >> kBaselineFractionBits;
            diffs[i] = static_cast<int>(RawCell(cells[i])) - baseline;
        }

        auto mid = diffs.begin() + (kCellCount / 2);
        std::nth_element(diffs.begin(), mid, diffs.end());
        return *mid;
    }

    // ═══════════════════════════════════════════════════════════════
    // Static helpers
    // ═══════════════════════════════════════════════════════════════

    static inline void ZeroOutput(int16_t* cells) {
        for (int i = 0; i < kCellCount; ++i) {
            cells[i] = 0;
        }
    }

    static inline int16_t SaturateInt16(int value) {
        return static_cast<int16_t>(std::clamp(value,
                                               static_cast<int>(INT16_MIN),
                                               static_cast<int>(INT16_MAX)));
    }

    static inline uint16_t RawCell(int16_t value) {
        return static_cast<uint16_t>(value);
    }
};

}} // namespace Solvers::Touch
