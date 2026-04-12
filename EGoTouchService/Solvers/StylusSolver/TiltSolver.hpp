#pragma once
#include "AsaTypes.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace Asa {

/// TiltSolver — Computes pen tilt from TX1–TX2 coordinate difference.
///
/// Mirrors TSACore GetTiltByCoorDif + tilt averaging/IIR/anomaly damping.
///
/// The tilt calculation uses the physical difference between TX1 (primary)
/// and TX2 (secondary) coordinate outputs. Since both drive at different
/// frequencies, the apparent position shift indicates the pen angle.
class TiltSolver {
public:
    /// Reset all state (pen-up or initialization)
    inline void Reset() {
        m_diffBufCount = 0;
        m_diffBufX.fill(0.0f);
        m_diffBufY.fill(0.0f);
        m_prevDiffX = 0.0f;
        m_prevDiffY = 0.0f;
        m_prevTiltX = 0;
        m_prevTiltY = 0;
        m_hasHistory = false;
        anomalyDamped = false;
    }

    /// Solve tilt from TX1 and TX2 global coordinates.
    /// @param tx1 TX1 coordinate (global, after anchor offset)
    /// @param tx2 TX2 coordinate (global, after anchor offset)
    /// @param[out] outTiltX Tilt X in degrees
    /// @param[out] outTiltY Tilt Y in degrees
    inline void Solve(const AsaCoorResult& tx1, const AsaCoorResult& tx2,
                      int16_t& outTiltX, int16_t& outTiltY) {
        if (!tx1.valid || !tx2.valid) {
            if (keepLastOnInvalid && m_hasHistory) {
                outTiltX = m_prevTiltX;
                outTiltY = m_prevTiltY;
            }
            return;
        }

        float diffX = static_cast<float>(tx2.dim1 - tx1.dim1) / 1024.0f;
        float diffY = static_cast<float>(tx2.dim2 - tx1.dim2) / 1024.0f;

        // Anomaly detection: if diff jumps too far, blend heavily with previous
        anomalyDamped = false;
        if (m_hasHistory) {
            const float jumpX = std::abs(diffX - m_prevDiffX);
            const float jumpY = std::abs(diffY - m_prevDiffY);
            if (jumpX > diffAnomalyThreshold || jumpY > diffAnomalyThreshold) {
                diffX = m_prevDiffX * 0.875f + diffX * 0.125f;
                diffY = m_prevDiffY * 0.875f + diffY * 0.125f;
                anomalyDamped = true;
            }
        }

        // Shift into ring buffer
        m_diffBufCount = std::min(10, m_diffBufCount + 1);
        for (int i = 9; i > 0; --i) {
            m_diffBufX[static_cast<size_t>(i)] = m_diffBufX[static_cast<size_t>(i - 1)];
            m_diffBufY[static_cast<size_t>(i)] = m_diffBufY[static_cast<size_t>(i - 1)];
        }
        m_diffBufX[0] = diffX;
        m_diffBufY[0] = diffY;

        // Sliding average
        const int cnt = std::max(1, std::min(m_diffBufCount,
            std::clamp(diffAverageWindow, 1, 10)));
        float sX = 0, sY = 0;
        for (int i = 0; i < cnt; ++i) {
            sX += m_diffBufX[static_cast<size_t>(i)];
            sY += m_diffBufY[static_cast<size_t>(i)];
        }
        diffX = sX / cnt;
        diffY = sY / cnt;

        // IIR smoothing
        if (m_hasHistory) {
            const float w = std::clamp(iirOldWeight, 0.f, 0.995f);
            diffX = m_prevDiffX * w + diffX * (1.f - w);
            diffY = m_prevDiffY * w + diffY * (1.f - w);
        }
        m_prevDiffX = diffX;
        m_prevDiffY = diffY;

        // Vector length clamp (TSACore step 9)
        {
            const float vecLen = std::sqrt(diffX * diffX + diffY * diffY);
            const float limit = std::max(0.1f, vectorClampLimit);
            if (vecLen > limit && vecLen > 0.001f) {
                diffX = diffX * (limit / vecLen);
                diffY = diffY * (limit / vecLen);
            }
        }

        int outX = ConvertDiffToTilt(diffX, false);
        int outY = ConvertDiffToTilt(diffY, true);

        // Jitter lock
        if (m_hasHistory) {
            const int jit = std::max(0, jitterThresholdDeg);
            if (std::abs(outX - m_prevTiltX) <= jit) outX = m_prevTiltX;
            if (std::abs(outY - m_prevTiltY) <= jit) outY = m_prevTiltY;
        }

        outTiltX = static_cast<int16_t>(outX);
        outTiltY = static_cast<int16_t>(outY);
        m_prevTiltX = outTiltX;
        m_prevTiltY = outTiltY;
        m_hasHistory = true;
    }

    // Diagnostic accessors
    float GetPrevDiffX() const { return m_prevDiffX; }
    float GetPrevDiffY() const { return m_prevDiffY; }

    // ── Configuration ──
    bool  enabled = false;
    bool  keepLastOnInvalid = true;
    int   diffAverageWindow = 5;
    float degreePerCellX = 8.0f;
    float degreePerCellY = 8.0f;
    float normLenX = 7.16f;
    float normLenY = 7.16f;
    int   maxDegree = 60;
    int   jitterThresholdDeg = 1;
    float iirOldWeight = 0.875f;
    float vectorClampLimit = 7.0f;
    float diffAnomalyThreshold = 3.0f;
    bool  anomalyDamped = false;  // diagnostic: true when anomaly damping fired

private:
    int   m_diffBufCount = 0;
    std::array<float, 10> m_diffBufX{};
    std::array<float, 10> m_diffBufY{};
    float m_prevDiffX = 0.0f;
    float m_prevDiffY = 0.0f;
    int16_t m_prevTiltX = 0;
    int16_t m_prevTiltY = 0;
    bool  m_hasHistory = false;

    inline int ConvertDiffToTilt(float coordDiff, bool dimY) const {
        const float normLen = std::max(0.1f, dimY ? normLenY : normLenX);
        const float legacyScale = std::max(0.1f,
            (dimY ? degreePerCellY : degreePerCellX) / 8.0f);
        const float scaled = coordDiff * legacyScale;
        float deg = 0.0f;
        if (std::abs(scaled) < normLen)
            deg = std::asin(scaled / normLen) * 57.2957795f;
        else
            deg = (scaled < 0.0f) ? -90.0f : 90.0f;
        const int maxT = std::clamp(maxDegree, 1, 89);
        return std::clamp(static_cast<int>(std::lround(deg)), -maxT, maxT);
    }
};

} // namespace Asa
