#pragma once
#include "AsaTypes.hpp"
#include <cmath>

namespace Asa {

/// OneEuroFilter — Adaptive low-pass filter for coordinate smoothing.
///
/// Alternative to the TSACore-style Q8 IIR filter. The 1€ filter
/// automatically adapts its cutoff frequency based on signal speed:
///   - Low speed  → low cutoff  → strong smoothing (jitter suppression)
///   - High speed → high cutoff → weak smoothing (responsive tracking)
///
/// The cutoff frequency is: fc = minCutoff + beta * |dx/dt|
///
/// Reference: Casiez, Roussel, Vogel. "1€ Filter: A Simple
///            Speed-based Low-pass Filter for Noisy Input in
///            Interactive Systems" (CHI 2012).
class OneEuroFilter {
public:
    /// Reset all state (on pen-up or fresh start)
    void Reset() {
        m_initialized = false;
        m_prevDim1 = 0;
        m_prevDim2 = 0;
        m_filteredDim1 = 0.0;
        m_filteredDim2 = 0.0;
        m_dxDim1 = 0.0;
        m_dxDim2 = 0.0;
        m_lastAlpha = 0.0f;
    }

    /// Filter one coordinate frame.
    /// @param cur    Current raw coordinate
    /// @return       Filtered coordinate
    AsaCoorResult Filter(const AsaCoorResult& cur) {
        if (!m_initialized) {
            m_filteredDim1 = static_cast<double>(cur.dim1);
            m_filteredDim2 = static_cast<double>(cur.dim2);
            m_prevDim1 = cur.dim1;
            m_prevDim2 = cur.dim2;
            m_dxDim1 = 0.0;
            m_dxDim2 = 0.0;
            m_initialized = true;
            return cur;
        }

        const double dt = 1.0 / static_cast<double>(sampleRate);

        // Read config values (float → double for computation)
        const double minCutoff = static_cast<double>(minCutoffF);
        const double beta  = static_cast<double>(betaF);
        const double dCutoff = static_cast<double>(dCutoffF);

        // 1. Compute raw velocity (derivative)
        const double rawDx1 = static_cast<double>(cur.dim1 - m_prevDim1) / dt;
        const double rawDx2 = static_cast<double>(cur.dim2 - m_prevDim2) / dt;

        // 2. Smooth velocity with fixed-cutoff low-pass
        const double alphaD = ComputeAlpha(dCutoff, dt);
        m_dxDim1 = LowPass(rawDx1, m_dxDim1, alphaD);
        m_dxDim2 = LowPass(rawDx2, m_dxDim2, alphaD);

        // 3. Adaptive cutoff = minCutoff + beta * |smoothed velocity|
        const double speed = std::sqrt(m_dxDim1 * m_dxDim1 + m_dxDim2 * m_dxDim2);
        const double cutoff = minCutoff + beta * speed;

        // 4. Filter position with adaptive cutoff
        const double alpha = ComputeAlpha(cutoff, dt);
        m_lastAlpha = static_cast<float>(alpha);
        m_filteredDim1 = LowPass(static_cast<double>(cur.dim1), m_filteredDim1, alpha);
        m_filteredDim2 = LowPass(static_cast<double>(cur.dim2), m_filteredDim2, alpha);

        m_prevDim1 = cur.dim1;
        m_prevDim2 = cur.dim2;

        AsaCoorResult out = cur;
        out.dim1 = static_cast<int32_t>(std::lround(m_filteredDim1));
        out.dim2 = static_cast<int32_t>(std::lround(m_filteredDim2));
        return out;
    }

    /// Get the last computed alpha (for diagnostics)
    float GetLastAlpha() const { return m_lastAlpha; }

    // ══════════════════════════════════════════════
    // Configuration parameters
    // ══════════════════════════════════════════════

    /// Minimum cutoff frequency (Hz). Lower = stronger smoothing at low speed.
    /// Typical range: 0.5–10.0. Default 1.0 provides good jitter suppression.
    float minCutoffF = 1.0f;

    /// Speed coefficient. Higher = more responsive at high speed.
    /// Typical range: 0.001–1.0. Default 0.007 provides natural pen tracking.
    float betaF = 0.007f;

    /// Derivative cutoff frequency (Hz). Smooths the velocity estimate.
    /// Higher = less delay in speed response. Default 1.0.
    float dCutoffF = 1.0f;

    /// Nominal sample rate (Hz). Used for alpha calculation.
    /// Should match the actual frame rate of the stylus pipeline.
    int sampleRate = 240;

private:
    bool   m_initialized = false;
    int32_t m_prevDim1 = 0;
    int32_t m_prevDim2 = 0;
    double m_filteredDim1 = 0.0;
    double m_filteredDim2 = 0.0;
    double m_dxDim1 = 0.0;
    double m_dxDim2 = 0.0;
    float  m_lastAlpha = 0.0f;

    /// Compute smoothing factor from cutoff frequency and timestep
    static double ComputeAlpha(double cutoff, double dt) {
        const double tau = 1.0 / (2.0 * 3.14159265358979323846 * cutoff);
        return 1.0 / (1.0 + tau / dt);
    }

    /// Simple exponential low-pass filter
    static double LowPass(double x, double prevFiltered, double alpha) {
        return alpha * x + (1.0 - alpha) * prevFiltered;
    }
};

} // namespace Asa
