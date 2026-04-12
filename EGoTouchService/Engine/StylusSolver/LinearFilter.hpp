#pragma once
#include "AsaTypes.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace Asa {

/// LinearFilter — Straight-line detection with gradual perpendicular constraint.
///
/// Redesigned from TSACore 7-state FSM into 2-state (Curve/Straight) + smooth transition.
///
/// Purpose: When a user draws a straight line, natural hand tremor causes
/// perpendicular jitter. This module detects straight-line motion by
/// least-squares fitting a line to the coordinate history, then constrains
/// coordinates to lie on that line with a gradual alpha transition.
///
/// Key improvement over TSACore:
///   - No 3-frame startup delay (Init/Wait/Collect removed)
///   - Gradual constraint transition (no hard-cut coordinate jump)
///   - Self-contained 400-frame buffer (shared across speed changes, only cleared on Leave)
class LinearFilter {
public:
    enum class Mode { Curve, Straight };

    /// Reset state and clear buffer (call on pen Leave only)
    inline void Reset() {
        m_mode = Mode::Curve;
        m_bufCount = 0;
        m_constraintAlpha = 0.0f;
        m_fit = LineFit{};
        m_output = AsaCoorResult{};
    }

    /// Get current mode (for diagnostics)
    inline Mode GetMode() const { return m_mode; }
    inline float GetConstraintAlpha() const { return m_constraintAlpha; }
    inline int GetBufferCount() const { return m_bufCount; }

    /// Main entry point — process one coordinate frame.
    /// @param coor      Coordinate (GLOBAL space)
    /// @param enabled   Whether LinearFilter is allowed to run (from MotionProfile)
    /// @return Filtered coordinate (constrained to line if detected)
    inline AsaCoorResult Process(const AsaCoorResult& coor, bool enabled) {
        if (!enabled) {
            // Module disabled (Hover/Landing/Lifting) — don't clear buffer,
            // just pass through. Buffer persists for when we re-enter Moving.
            m_constraintAlpha = 0.0f;
            m_mode = Mode::Curve;
            return coor;
        }

        // 1. Push point to buffer
        PushPoint(coor);

        // 2. Fit line if enough data
        if (m_bufCount >= minFitLength) {
            m_fit = FitLine();
        }

        // 3. State transitions
        if (m_fit.valid) {
            if (m_mode == Mode::Curve) {
                if (m_fit.totalDist < enterResidualThreshold) {
                    m_mode = Mode::Straight;
                }
            } else { // Straight
                if (m_fit.maxDist > exitDeviation) {
                    m_mode = Mode::Curve;
                }
            }
        }

        // 4. Gradual constraint transition (smooth alpha ramp)
        float targetAlpha = (m_mode == Mode::Straight) ? perpConstraint : 0.0f;
        m_constraintAlpha += (targetAlpha - m_constraintAlpha) * transitionRate;

        // 5. Apply constraint
        if (m_constraintAlpha > 0.01f && m_fit.valid) {
            return ConstrainToLine(coor, m_fit, m_constraintAlpha);
        }
        return coor;
    }

    // ── Configuration ──

    /// Minimum buffer length before line fitting starts
    int minFitLength = 20;

    /// Residual threshold to enter straight-line mode
    float enterResidualThreshold = 30.0f;

    /// Maximum deviation to stay in straight-line mode
    float exitDeviation = 200.0f;

    /// Perpendicular constraint strength (0.0 = none, 1.0 = full)
    float perpConstraint = 0.7f;

    /// Transition rate for alpha ramp (higher = faster transition)
    float transitionRate = 0.3f;

private:
    static constexpr int kMaxBufLen = 400;

    struct Point { int32_t x = 0, y = 0; };

    struct LineFit {
        float slope = 0;
        float intercept = 0;
        float normFactor = 1.0f;
        float totalDist = 0;
        float maxDist = 0;
        bool  useYasX = false;
        bool  valid = false;
    };

    Mode m_mode = Mode::Curve;
    float m_constraintAlpha = 0.0f;
    int m_bufCount = 0;
    std::array<Point, kMaxBufLen> m_buf{};
    LineFit m_fit{};
    AsaCoorResult m_output{};

    inline void PushPoint(const AsaCoorResult& c) {
        if (m_bufCount < kMaxBufLen) {
            m_buf[static_cast<size_t>(m_bufCount)] = {c.dim1, c.dim2};
            m_bufCount++;
        } else {
            // Shift left by 1 (drop oldest)
            for (int i = 0; i < kMaxBufLen - 1; ++i) {
                m_buf[static_cast<size_t>(i)] =
                    m_buf[static_cast<size_t>(i + 1)];
            }
            m_buf[kMaxBufLen - 1] = {c.dim1, c.dim2};
        }
    }

    /// Least-squares line fit over all buffered points
    inline LineFit FitLine() const {
        LineFit result{};
        if (m_bufCount < 3) return result;

        // Reference point for numerical stability
        const double refX = static_cast<double>(m_buf[0].x);
        const double refY = static_cast<double>(m_buf[0].y);

        double sumX = 0, sumY = 0, sumXX = 0, sumXY = 0, sumYY = 0;
        const int n = m_bufCount;

        for (int i = 0; i < n; ++i) {
            double dx = static_cast<double>(m_buf[static_cast<size_t>(i)].x) - refX;
            double dy = static_cast<double>(m_buf[static_cast<size_t>(i)].y) - refY;
            sumX += dx; sumY += dy;
            sumXX += dx * dx; sumXY += dx * dy; sumYY += dy * dy;
        }

        const double dn = static_cast<double>(n);

        // Centered covariances
        double cXY = sumXY - (sumX * sumY) / dn;
        double cXX = sumXX - (sumX * sumX) / dn;
        double cYY = sumYY - (sumY * sumY) / dn;

        // Choice orientation with larger variance
        if (cXX <= cYY) {
            // x = A*y + B (steep line)
            if (std::abs(cYY) < 1e-10) return result;
            result.useYasX = true;
            result.slope = static_cast<float>(cXY / cYY);
            result.intercept = static_cast<float>(
                (sumX / dn + refX) - result.slope * (refY + sumY / dn));
        } else {
            // y = A*x + B
            if (std::abs(cXX) < 1e-10) return result;
            result.useYasX = false;
            result.slope = static_cast<float>(cXY / cXX);
            result.intercept = static_cast<float>(
                (sumY / dn + refY) - result.slope * (refX + sumX / dn));
        }

        result.normFactor = static_cast<float>(
            std::sqrt(static_cast<double>(result.slope) *
                      static_cast<double>(result.slope) + 1.0));

        // Compute distance statistics
        result.totalDist = 0;
        result.maxDist = 0;

        auto calcDist2 = [&](int32_t px, int32_t py) -> float {
            float d;
            if (!result.useYasX) {
                d = (result.slope * static_cast<float>(px) +
                     result.intercept - static_cast<float>(py)) / result.normFactor;
            } else {
                d = (result.slope * static_cast<float>(py) +
                     result.intercept - static_cast<float>(px)) / result.normFactor;
            }
            return d * d;
        };

        for (int i = 0; i < n; ++i) {
            float d2 = calcDist2(m_buf[static_cast<size_t>(i)].x,
                                 m_buf[static_cast<size_t>(i)].y);
            result.totalDist += d2;
            if (d2 > result.maxDist) result.maxDist = d2;
        }

        result.valid = true;
        return result;
    }

    /// Constrain coordinate to the fitted line with given alpha
    static inline AsaCoorResult ConstrainToLine(const AsaCoorResult& c,
                                                 const LineFit& fit,
                                                 float alpha) {
        if (!fit.valid) return c;
        AsaCoorResult out = c;
        const float strength = std::clamp(alpha, 0.0f, 1.0f);

        if (!fit.useYasX) {
            float predicted = fit.slope * static_cast<float>(c.dim1) +
                             fit.intercept;
            float actual = static_cast<float>(c.dim2);
            float filtered = actual + strength * (predicted - actual);
            out.dim2 = static_cast<int32_t>(std::lround(filtered));
        } else {
            float predicted = fit.slope * static_cast<float>(c.dim2) +
                             fit.intercept;
            float actual = static_cast<float>(c.dim1);
            float filtered = actual + strength * (predicted - actual);
            out.dim1 = static_cast<int32_t>(std::lround(filtered));
        }
        return out;
    }
};

} // namespace Asa
