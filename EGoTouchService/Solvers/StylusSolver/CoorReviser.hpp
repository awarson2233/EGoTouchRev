#pragma once
#include "AsaTypes.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace Asa {

/// CoorReviser — TX2 tilt-based coordinate revision (factory algorithm).
///
/// Mirrors TSACore: TiltProcess → CoorReviseCalculation → CoorReviseWork.
///
/// Original factory flow:
///   1. TiltProcess: Computes TX1−TX2 coordinate difference (= tilt vector)
///   2. CoorReviseCalculation: reviseXY = tiltMultiplier × tiltAngle
///   3. CoorReviseWork: finalXY = TX1_XY − reviseXY
///
/// The tilt multiplier converts tilt angle (degrees) into a coordinate
/// offset that compensates for the electromagnetic projection bias
/// caused by pen inclination.
class CoorReviser {
public:
    /// Reset all internal state (call on pen-up or init).
    /// Mirrors TSACore CoorReviseInit + TiltInit.
    inline void Reset() {
        m_initialized = false;
        m_diffBufCount = 0;
        m_diffBufDim1.fill(0);
        m_diffBufDim2.fill(0);
        m_prevDiffDim1 = 0;
        m_prevDiffDim2 = 0;
        m_reviseBufCount = 0;
        m_reviseBufX.fill(0);
        m_reviseBufY.fill(0);
        m_prevReviseX = 0;
        m_prevReviseY = 0;
        m_lastTiltX = 0;
        m_lastTiltY = 0;
        m_lastReportedTiltX = 0;
        m_lastReportedTiltY = 0;
        m_prevPressure = 0;
    }

    /// Main entry: solve tilt from TX1/TX2, then revise TX1 coordinate.
    ///
    /// @param tx1  TX1 coordinate (after 3PtAvg, PitchMap-mapped)
    /// @param tx2  TX2 coordinate (PitchMap-mapped, global)
    /// @param curPressure  Current pressure (for pen-up detection → reset)
    /// @param[out] outTiltX  Tilt X in degrees (for HID output)
    /// @param[out] outTiltY  Tilt Y in degrees (for HID output)
    /// @return Revised TX1 coordinate
    inline AsaCoorResult Revise(const AsaCoorResult& tx1,
                                const AsaCoorResult& tx2,
                                uint16_t curPressure,
                                int16_t& outTiltX,
                                int16_t& outTiltY) {
        AsaCoorResult out = tx1;
        if (!enabled) return out;

        // Pressure-based reset (TSACore: prevPress!=0 && curPress==0 → CoorReviseInit)
        if (m_prevPressure != 0 && curPressure == 0) {
            Reset();
        }
        m_prevPressure = curPressure;

        if (!tx1.valid) { Reset(); return out; }
        if (!tx2.valid) {
            // TX2 unavailable → keep last tilt
            if (keepLastOnInvalid && m_initialized) {
                outTiltX = m_lastReportedTiltX;
                outTiltY = m_lastReportedTiltY;
            }
            return out;
        }

        // ═══════════════════════════════════════════════
        // Step 1: TiltProcess — compute TX1-TX2 difference
        // ═══════════════════════════════════════════════

        // Sign flipped for Windows-facing tilt orientation:
        // use TX2 - TX1 so positive tilt matches the observed device axes.
        int16_t rawDiffDim1 = static_cast<int16_t>(std::clamp(
            tx2.dim1 - tx1.dim1, -32768, 32767));
        int16_t rawDiffDim2 = static_cast<int16_t>(std::clamp(
            tx2.dim2 - tx1.dim2, -32768, 32767));

        // TSACore anomaly check: if diff jumps too far from IIR history,
        // blend 7/8 old + 1/8 new (mirrors the comparison with sVar4/sVar5)
        if (m_initialized) {
            const int jumpDim1 = std::abs(rawDiffDim1 - m_prevDiffDim1);
            const int jumpDim2 = std::abs(rawDiffDim2 - m_prevDiffDim2);
            const int lenLimit = ComputeLenLimit();

            if (jumpDim1 > lenLimit || jumpDim2 > lenLimit) {
                // TSACore: (cur + prev*7) >> 3
                rawDiffDim1 = static_cast<int16_t>(
                    (static_cast<int>(rawDiffDim1) +
                     static_cast<int>(m_prevDiffDim1) * 7) >> 3);
                rawDiffDim2 = static_cast<int16_t>(
                    (static_cast<int>(rawDiffDim2) +
                     static_cast<int>(m_prevDiffDim2) * 7) >> 3);
            }
        }

        // BufTX1TX2CoorDif — push into 10-frame ring buffer
        m_diffBufCount = std::min(10, m_diffBufCount + 1);
        for (int i = 9; i > 0; --i) {
            m_diffBufDim1[i] = m_diffBufDim1[i - 1];
            m_diffBufDim2[i] = m_diffBufDim2[i - 1];
        }
        m_diffBufDim1[0] = rawDiffDim1;
        m_diffBufDim2[0] = rawDiffDim2;
        m_prevDiffDim1 = rawDiffDim1;
        m_prevDiffDim2 = rawDiffDim2;

        // GetTX1TX2CoorDifAverage(5) — 5-frame sliding average
        const int diffAvgCnt = std::min(m_diffBufCount, diffAverageWindow);
        int sumD1 = 0, sumD2 = 0;
        for (int i = 0; i < diffAvgCnt; ++i) {
            sumD1 += m_diffBufDim1[i];
            sumD2 += m_diffBufDim2[i];
        }
        int16_t avgDiffDim1 = static_cast<int16_t>(sumD1 / std::max(1, diffAvgCnt));
        int16_t avgDiffDim2 = static_cast<int16_t>(sumD2 / std::max(1, diffAvgCnt));

        // Vector length clamp (TSACore: if |d| > lenLimit, scale down)
        {
            const int lenLimit = ComputeLenLimit();
            const int vecLenSq = static_cast<int>(avgDiffDim1) * avgDiffDim1 +
                                 static_cast<int>(avgDiffDim2) * avgDiffDim2;
            const int vecLen = static_cast<int>(std::sqrt(static_cast<double>(vecLenSq)));
            if (vecLen > lenLimit && vecLen > 0) {
                avgDiffDim1 = static_cast<int16_t>(
                    (static_cast<int>(lenLimit) * avgDiffDim1) / vecLen);
                avgDiffDim2 = static_cast<int16_t>(
                    (static_cast<int>(lenLimit) * avgDiffDim2) / vecLen);
            }
        }

        // GetTiltByCoorDif — convert coordinate diff to tilt angle (degrees)
        int16_t tiltDim1 = ConvertDiffToTilt(avgDiffDim1, false);
        int16_t tiltDim2 = ConvertDiffToTilt(avgDiffDim2, true);

        // Tilt averaging (5-frame) + 1° jitter filter
        if (m_initialized) {
            // BufDim1Dim2Tilt + GetTiltAverage(5)
            m_tiltBufCount = std::min(10, m_tiltBufCount + 1);
            for (int i = 9; i > 0; --i) {
                m_tiltBufX[i] = m_tiltBufX[i - 1];
                m_tiltBufY[i] = m_tiltBufY[i - 1];
            }
            m_tiltBufX[0] = tiltDim1;
            m_tiltBufY[0] = tiltDim2;
            const int tiltAvgCnt = std::min(m_tiltBufCount, tiltAverageWindow);
            int sumTX = 0, sumTY = 0;
            for (int i = 0; i < tiltAvgCnt; ++i) {
                sumTX += m_tiltBufX[i];
                sumTY += m_tiltBufY[i];
            }
            tiltDim1 = static_cast<int16_t>(sumTX / std::max(1, tiltAvgCnt));
            tiltDim2 = static_cast<int16_t>(sumTY / std::max(1, tiltAvgCnt));
        } else {
            m_tiltBufCount = 1;
            m_tiltBufX.fill(0); m_tiltBufY.fill(0);
            m_tiltBufX[0] = tiltDim1;
            m_tiltBufY[0] = tiltDim2;
        }

        // Tilt1DegreeJitFilter — suppress sub-threshold jitter
        if (m_initialized) {
            if (std::abs(tiltDim1 - m_lastTiltX) <= tiltJitterDeg)
                tiltDim1 = m_lastTiltX;
            if (std::abs(tiltDim2 - m_lastTiltY) <= tiltJitterDeg)
                tiltDim2 = m_lastTiltY;
        }
        m_lastTiltX = tiltDim1;
        m_lastTiltY = tiltDim2;

        // Windows pen tilt uses a different axis convention than the
        // internal TX1/TX2 tilt space used for coordinate revision.
        const int16_t reportedTiltX = static_cast<int16_t>(-tiltDim2);
        const int16_t reportedTiltY = tiltDim1;
        m_lastReportedTiltX = reportedTiltX;
        m_lastReportedTiltY = reportedTiltY;
        outTiltX = reportedTiltX;
        outTiltY = reportedTiltY;

        // ═══════════════════════════════════════════════
        // Step 2: CoorReviseCalculation
        //   reviseX = prmt[0x26b] × tiltDim1
        //   reviseY = prmt[0x26c] × tiltDim2
        // ═══════════════════════════════════════════════
        int16_t reviseX = static_cast<int16_t>(std::clamp(
            static_cast<int>(tiltMultiplierX) * tiltDim1, -32768, 32767));
        int16_t reviseY = static_cast<int16_t>(std::clamp(
            static_cast<int>(tiltMultiplierY) * tiltDim2, -32768, 32767));

        // CoorReviseLimitingFilter — max change per frame = 15
        if (m_initialized) {
            reviseX = LimitingFilter(m_prevReviseX, reviseX, reviseLimitStep);
            reviseY = LimitingFilter(m_prevReviseY, reviseY, reviseLimitStep);
        }
        m_prevReviseX = reviseX;
        m_prevReviseY = reviseY;

        // BufCoorRevise + GetTX1TX2CoorReviseAverage(5)
        m_reviseBufCount = std::min(10, m_reviseBufCount + 1);
        for (int i = 9; i > 0; --i) {
            m_reviseBufX[i] = m_reviseBufX[i - 1];
            m_reviseBufY[i] = m_reviseBufY[i - 1];
        }
        m_reviseBufX[0] = reviseX;
        m_reviseBufY[0] = reviseY;
        const int revAvgCnt = std::min(m_reviseBufCount, reviseAverageWindow);
        int sumRX = 0, sumRY = 0;
        for (int i = 0; i < revAvgCnt; ++i) {
            sumRX += m_reviseBufX[i];
            sumRY += m_reviseBufY[i];
        }
        reviseX = static_cast<int16_t>(sumRX / std::max(1, revAvgCnt));
        reviseY = static_cast<int16_t>(sumRY / std::max(1, revAvgCnt));

        m_initialized = true;

        // ═══════════════════════════════════════════════
        // Step 3: CoorReviseWork
        //   finalXY = TX1_XY − reviseXY
        // ═══════════════════════════════════════════════
        int32_t revisedDim1 = tx1.dim1 - static_cast<int32_t>(reviseX);
        int32_t revisedDim2 = tx1.dim2 - static_cast<int32_t>(reviseY);

        // Bounds check (TSACore: clamp to [0, gridDim * 0x400])
        const int32_t maxDim1 = static_cast<int32_t>(maxGridCols) * kCoorUnit;
        const int32_t maxDim2 = static_cast<int32_t>(maxGridRows) * kCoorUnit;
        revisedDim1 = std::clamp(revisedDim1, 0, maxDim1);
        revisedDim2 = std::clamp(revisedDim2, 0, maxDim2);

        out.dim1 = revisedDim1;
        out.dim2 = revisedDim2;

        // Diagnostic outputs
        m_lastReviseX = reviseX;
        m_lastReviseY = reviseY;
        return out;
    }

    // ── Diagnostic accessors ──
    int16_t GetLastReviseX() const { return m_lastReviseX; }
    int16_t GetLastReviseY() const { return m_lastReviseY; }
    int16_t GetLastTiltX()   const { return m_lastTiltX; }
    int16_t GetLastTiltY()   const { return m_lastTiltY; }

    // ══════════════════════════════════════════════
    // Configuration parameters
    // ══════════════════════════════════════════════

    bool enabled = true;       // ASA feature bit0 (0x23 & 1 = 1 on Gaokun)
    bool keepLastOnInvalid = true;

    /// Tilt multiplier: revise = multiplier × tilt_degrees.
    /// Temporarily scaled 10x for debugging direction/magnitude issues.
    /// Original Gaokun factory: prmt[0x26b] = prmt[0x26c] = 5.
    int tiltMultiplierX = 50;
    int tiltMultiplierY = 50;

    /// Coordinate diff averaging window (TSACore: GetTX1TX2CoorDifAverage(5))
    int diffAverageWindow = 5;

    /// Tilt averaging window (TSACore: GetTiltAverage(5))
    int tiltAverageWindow = 5;

    /// Revise averaging window (TSACore: GetTX1TX2CoorReviseAverage(5))
    int reviseAverageWindow = 5;

    /// Max change per frame for revise values (TSACore: CoorReviseLimitingFilter 0xF=15)
    int reviseLimitStep = 15;

    /// Tilt jitter suppression threshold (degrees)
    int tiltJitterDeg = 1;

    /// Tilt normalization length (TSACore: gridCols * prmt[0x26a] * 0x400 / totalPitch)
    /// Gaokun: (60 * 144 * 1024) / 2560 = 3456
    int normLenDim1 = 3456;
    /// Gaokun: (40 * 144 * 1024) / 1600 = 3686 (≈same as dim1 for square cells)
    int normLenDim2 = 3686;

    /// Max tilt angle (degrees)
    int maxTiltDeg = 60;

    /// Grid dimensions for CoorReviseWork bounds check
    int maxGridCols = 60;   // DAT_1820d610
    int maxGridRows = 40;   // DAT_1820d611

private:
    // ── Internal state ──
    bool m_initialized = false;

    // Diff ring buffer (BufTX1TX2CoorDif, 10 frames)
    int m_diffBufCount = 0;
    std::array<int16_t, 10> m_diffBufDim1{};
    std::array<int16_t, 10> m_diffBufDim2{};
    int16_t m_prevDiffDim1 = 0;
    int16_t m_prevDiffDim2 = 0;

    // Tilt ring buffer (BufDim1Dim2Tilt, 10 frames)
    int m_tiltBufCount = 0;
    std::array<int16_t, 10> m_tiltBufX{};
    std::array<int16_t, 10> m_tiltBufY{};

    // Revise ring buffer (BufCoorRevise, 10 frames)
    int m_reviseBufCount = 0;
    std::array<int16_t, 10> m_reviseBufX{};
    std::array<int16_t, 10> m_reviseBufY{};
    int16_t m_prevReviseX = 0;
    int16_t m_prevReviseY = 0;

    // Tilt output history
    int16_t m_lastTiltX = 0;
    int16_t m_lastTiltY = 0;
    int16_t m_lastReportedTiltX = 0;
    int16_t m_lastReportedTiltY = 0;

    // Pressure-based reset
    uint16_t m_prevPressure = 0;

    // Diagnostics
    int16_t m_lastReviseX = 0;
    int16_t m_lastReviseY = 0;

    /// Compute tilt angle from coordinate difference.
    /// Mirrors TSACore GetTiltByCoorDif.
    /// angle = asin(diff / normLen) × 180/π
    inline int16_t ConvertDiffToTilt(int16_t diff, bool dimY) const {
        const int normLen = std::max(1, dimY ? normLenDim2 : normLenDim1);
        const double ratio = static_cast<double>(diff) / static_cast<double>(normLen);
        int16_t deg;
        if (std::abs(ratio) < 1.0) {
            deg = static_cast<int16_t>(std::lround(
                std::asin(ratio) * 180.0 / 3.14159265358979323846));
        } else {
            deg = (diff < 0) ? static_cast<int16_t>(-90) : static_cast<int16_t>(90);
        }
        return static_cast<int16_t>(std::clamp(
            static_cast<int>(deg), -maxTiltDeg, maxTiltDeg));
    }

    /// Compute len limit for anomaly detection.
    /// TSACore: normLen = gridCols * prmt[0x26a] * 0x400 / totalPitch
    inline int ComputeLenLimit() const {
        return normLenDim1;  // Use dim1 as primary limit (TSACore default path)
    }

    /// CoorReviseLimitingFilter — max step per frame.
    static inline int16_t LimitingFilter(int16_t prev, int16_t cur, int maxStep) {
        const int delta = static_cast<int>(cur) - static_cast<int>(prev);
        if (delta > maxStep) return static_cast<int16_t>(prev + maxStep);
        if (delta < -maxStep) return static_cast<int16_t>(prev - maxStep);
        return cur;
    }
};

} // namespace Asa
