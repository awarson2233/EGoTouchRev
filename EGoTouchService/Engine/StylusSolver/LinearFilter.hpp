#pragma once
#include "AsaTypes.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace Asa {

/// LinearFilter — 7-state FSM for straight-line detection and constraint.
///
/// Mirrors TSACore StraightLinePaintFun / UpdateStraightLinePrmt.
///
/// Purpose: When a user draws a straight line, natural hand tremor causes
/// perpendicular jitter. This module detects straight-line motion by
/// least-squares fitting a line to the coordinate history, then constrains
/// coordinates to lie on that line.
///
/// State machine:
///   Init → Wait → Collect → CurveLine ⇄ EnterStraight ⇄ StraightLine → ExitStraight → CurveLine
class LinearFilter {
public:
    enum class State {
        Init = 0,
        Wait = 1,
        Collect = 2,
        CurveLine = 3,
        EnterStraight = 4,
        StraightLine = 5,
        ExitStraight = 6,
    };

    /// Reset state machine (call on pen-up)
    inline void Reset() {
        m_state = State::Init;
        m_bufCount = 0;
        m_shortDisBufCount = 0;
        m_fitGlobal = LineFit{};
        m_fitCurrent = LineFit{};
        m_output = AsaCoorResult{};
    }

    /// Get current FSM state (for diagnostics)
    inline int GetState() const { return static_cast<int>(m_state); }

    /// Main entry point — process one coordinate frame.
    /// @param coor      Raw coordinate (LOCAL space)
    /// @param pressure  Current pressure (0 = no-ink mode)
    /// @return Filtered coordinate (constrained to line if detected)
    inline AsaCoorResult Process(const AsaCoorResult& coor, uint16_t pressure) {
        if (!enabled) return coor;

        // Pen up or invalid → reset state machine
        if (pressure == 0 || !coor.valid) {
            Reset();
            return coor;
        }

        m_output = coor;

        // P2: Update dual fits when we have enough points (>= 20)
        if (m_bufCount > static_cast<int>(minFitLength)) {
            int agingWeight = kMaxBufLen - m_bufCount;
            if (agingWeight < 3) agingWeight = m_bufCount;
            m_fitCurrent = FitLine(true, agingWeight);
            m_fitGlobal = FitLine(false, agingWeight);
        }

        // P2: Per-frame state transitions (TSACore: case 0→1, 1→2, 2→3)
        switch (m_state) {
        case State::Init:
            m_state = State::Wait;
            break;
        case State::Wait:
            m_state = State::Collect;
            break;
        case State::Collect:
            m_state = State::CurveLine;
            break;
        case State::CurveLine:
            ProcessCurveLine(coor);
            break;
        case State::EnterStraight:
            ProcessEnterStraight(coor);
            break;
        case State::StraightLine:
            ProcessStraightLine(coor);
            break;
        case State::ExitStraight:
            ProcessExitStraight(coor);
            break;
        default:
            m_state = State::CurveLine;
            break;
        }

        // Push to buffers AFTER state machine (TSACore order)
        PushPoint(coor);
        PushShortDisPoint(coor);

        // Output clamping (TSACore: clamp to [0, sensorDim * 0x400])
        if (m_output.dim1 < 0) m_output.dim1 = 0;
        if (m_output.dim2 < 0) m_output.dim2 = 0;

        return m_output;
    }

    // ── Configuration ──

    /// Enable/disable the linear filter
    bool enabled = false;

    /// Minimum buffer length before line fitting starts
    int minFitLength = 20;

    /// Residual threshold to enter straight-line mode
    /// (summed squared distance from fitted line)
    float enterResidualThreshold = 30.0f;

    /// Maximum deviation to stay in straight-line mode
    float exitDeviation = 200.0f;

    /// Perpendicular constraint strength (0.0 = none, 1.0 = full)
    float perpConstraint = 0.7f;

private:
    static constexpr int kMaxBufLen = 400;
    static constexpr int kShortDisBufLen = 30;

    struct Point { int32_t x = 0, y = 0; };

    struct LineFit {
        float slope = 0;
        float intercept = 0;
        float normFactor = 1.0f;
        float totalDist = 0;
        float maxDist = 0;
        float lastDist = 0;
        bool  useYasX = false;
        bool  valid = false;
    };

    State m_state = State::Init;
    int m_bufCount = 0;
    int m_shortDisBufCount = 0;
    std::array<Point, kMaxBufLen> m_buf{};
    std::array<Point, kShortDisBufLen> m_shortDisBuf{};
    LineFit m_fitGlobal{};
    LineFit m_fitCurrent{};
    AsaCoorResult m_output{};

    inline void PushPoint(const AsaCoorResult& c) {
        if (m_bufCount < kMaxBufLen) {
            m_buf[static_cast<size_t>(m_bufCount)] = {c.dim1, c.dim2};
            m_bufCount++;
        } else {
            for (int i = 0; i < kMaxBufLen - 1; ++i) {
                m_buf[static_cast<size_t>(i)] =
                    m_buf[static_cast<size_t>(i + 1)];
            }
            m_buf[kMaxBufLen - 1] = {c.dim1, c.dim2};
        }
    }

    inline void PushShortDisPoint(const AsaCoorResult& c) {
        if (m_shortDisBufCount < kShortDisBufLen) {
            m_shortDisBuf[static_cast<size_t>(m_shortDisBufCount)] = {c.dim1, c.dim2};
            m_shortDisBufCount++;
        } else {
            for (int i = 0; i < kShortDisBufLen - 1; ++i) {
                m_shortDisBuf[static_cast<size_t>(i)] =
                    m_shortDisBuf[static_cast<size_t>(i + 1)];
            }
            m_shortDisBuf[kShortDisBufLen - 1] = {c.dim1, c.dim2};
        }
    }

    /// Least-squares line fit (TSACore: UpdateStraightLinePrmt)
    inline LineFit FitLine(bool includeCurrent, int agingWeight) const {
        LineFit result{};
        const int startIdx = std::max(0, kMaxBufLen - m_bufCount);
        const int endIdx = kMaxBufLen;
        int effectiveN = agingWeight;
        if (effectiveN < 3) effectiveN = m_bufCount;
        if (effectiveN < 3) return result;

        // Reference point for numerical stability
        const double refX = (m_bufCount > 0) ?
            static_cast<double>(m_buf[static_cast<size_t>(startIdx)].x) : 0.0;
        const double refY = (m_bufCount > 0) ?
            static_cast<double>(m_buf[static_cast<size_t>(startIdx)].y) : 0.0;

        // Accumulate sums
        double sumX = 0, sumY = 0, sumXX = 0, sumXY = 0, sumYY = 0;

        // If includeCurrent, add the newest point first (TSACore param_2==1)
        if (includeCurrent && m_bufCount > 0) {
            const int lastIdx = std::min(m_bufCount - 1, kMaxBufLen - 1);
            double dx = static_cast<double>(m_buf[static_cast<size_t>(lastIdx)].x) - refX;
            double dy = static_cast<double>(m_buf[static_cast<size_t>(lastIdx)].y) - refY;
            sumX += dx; sumY += dy;
            sumXX += dx * dx; sumXY += dx * dy; sumYY += dy * dy;
        }

        // Iterate over buffered points
        for (int i = startIdx; i < endIdx && i < m_bufCount; ++i) {
            double dx = static_cast<double>(m_buf[static_cast<size_t>(i)].x) - refX;
            double dy = static_cast<double>(m_buf[static_cast<size_t>(i)].y) - refY;
            sumX += dx; sumY += dy;
            sumXX += dx * dx; sumXY += dx * dy; sumYY += dy * dy;
        }

        const double dn = static_cast<double>(effectiveN);

        // TSACore: subtract mean products for centering
        double cXY = sumXY - (sumX * sumY) / dn;
        double cXX = sumXX - (sumX * sumX) / dn;
        double cYY = sumYY - (sumY * sumY) / dn;

        // Choose orientation with larger variance
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

        // Compute squared distances
        result.totalDist = 0;
        result.maxDist = 0;
        result.lastDist = 0;

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

        for (int i = startIdx; i < endIdx && i < m_bufCount; ++i) {
            float d2 = calcDist2(m_buf[static_cast<size_t>(i)].x,
                                 m_buf[static_cast<size_t>(i)].y);
            result.totalDist += d2;
            if (d2 > result.maxDist) result.maxDist = d2;
            if (i == m_bufCount - 1) result.lastDist = d2;
        }

        // If includeCurrent, also check current (last) point distance
        if (includeCurrent && m_bufCount > 0) {
            const int lastIdx = std::min(m_bufCount - 1, kMaxBufLen - 1);
            float d2 = calcDist2(m_buf[static_cast<size_t>(lastIdx)].x,
                                 m_buf[static_cast<size_t>(lastIdx)].y);
            if (d2 > result.maxDist) result.maxDist = d2;
            result.lastDist = d2;
        }

        result.valid = true;
        return result;
    }

    /// Constrain coordinate to the fitted line
    inline AsaCoorResult ConstrainToLine(const AsaCoorResult& c,
                                          const LineFit& fit) const {
        if (!fit.valid) return c;

        AsaCoorResult out = c;
        const float strength = std::clamp(perpConstraint, 0.0f, 1.0f);

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

    inline void ProcessCurveLine(const AsaCoorResult& c) {
        if (m_bufCount < minFitLength) return;

        int agingWeight = kMaxBufLen - m_bufCount;
        if (agingWeight < 3) agingWeight = m_bufCount;
        m_fitGlobal = FitLine(false, agingWeight);

        if (m_fitGlobal.valid && m_fitGlobal.totalDist < enterResidualThreshold) {
            m_state = State::EnterStraight;
        }
    }

    inline void ProcessEnterStraight(const AsaCoorResult& c) {
        m_fitCurrent = FitLine(true, kMaxBufLen - m_bufCount);
        m_output = ConstrainToLine(c, m_fitCurrent);
        m_state = State::StraightLine;
    }

    inline void ProcessStraightLine(const AsaCoorResult& c) {
        int agingWeight = kMaxBufLen - m_bufCount;
        if (agingWeight < 3) agingWeight = m_bufCount;
        m_fitGlobal = FitLine(false, agingWeight);
        m_fitCurrent = FitLine(true, agingWeight);

        if (!m_fitCurrent.valid || m_fitCurrent.maxDist > exitDeviation) {
            m_state = State::ExitStraight;
            return;
        }

        m_output = ConstrainToLine(c, m_fitCurrent);
    }

    inline void ProcessExitStraight(const AsaCoorResult& /*c*/) {
        m_state = State::CurveLine;
        // m_output already set to coor in Process()
    }
};

} // namespace Asa
