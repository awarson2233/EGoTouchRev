#pragma once

#include "StylusSolver/AsaTypes.hpp"
#include "SolverTypes.h"

#include <cstdint>

namespace Solvers::Stylus {

class CoorIIRProcess {
public:
    bool m_enabled = true;

    // ── IIR coefficient selection (GetIIRCoef equivalent) ──
    // Hover mode params (no pressure)
    int32_t m_coefLowHover = 12;     // asa[0xA5E]
    int32_t m_coefHighHover = 6;     // asa[0xA5F]
    int32_t m_speedTholdHover = 20;  // 0x14

    // Writing mode params (has pressure)
    int32_t m_coefLowWriting = 18;        // asa[0xA5C]
    int32_t m_coefHighWriting = 26;      // asa[0xA5D]
    int32_t m_speedTholdWriting = 10;    // 0x0A

    int32_t m_speedMax = 60;  // 0xCD — speed value at which high coef is fully engaged
    int32_t m_maxCoef = 32;    // asa[0xA60] — denominator in IIR formula

    // ── Output ──
    uint16_t m_currentCoef = 10;

    inline void Reset() {
        m_iirFracX = 0;
        m_iirFracY = 0;
        m_prevFiltX = 0;
        m_prevFiltY = 0;
        m_prevInRange = false;
        m_frameCount = 0;
        m_currentCoef = m_coefLowHover;
        m_counterC = 0;
        m_counter8 = 0;
        m_counterA = 0;
    }

    inline void Process(HeatmapFrame& frame) {
        auto& runtime = frame.stylus.runtime.Active();
        auto& coor = runtime.post.finalCoor;
        const bool inRange = coor.valid;  // stylus detected (hover OR touch)

        if (!m_enabled || !inRange) {
            runtime.post.postIirCoor = coor;
            Reset();
            return;
        }

        // ── Update TSACore-aligned static counters ──
        const bool hasPressureValue = runtime.pressure.outputPressure > 0;

        // Default: EnterInRangeMode behavior
        ++m_counterC;

        if (hasPressureValue) {
            m_counterC = 0;
            ++m_counter8;
            ++m_counterA;
        } else {
            m_counter8 = 0;
            m_counterA = 0;
        }

        // ── GetIIRCoef: speed-adaptive coefficient selection ──
        const bool edgeActive = runtime.signal.dim1EdgeActive ||
                                runtime.signal.dim2EdgeActive;
        const bool writingMode = hasPressureValue;
        m_currentCoef = SelectCoef(runtime.post.speedValue, writingMode, edgeActive);
        runtime.post.iirCoef = m_currentCoef;

        // ── CoorFilterProcess: TSACore gate ──
        const bool bypassGate = (((!m_prevInRange || m_counterC < 2) && m_counter8 < 2) && m_counterA < 2);

        if (bypassGate) {
            m_iirFracX = 0;
            m_iirFracY = 0;
            m_prevFiltX = coor.dim1;
            m_prevFiltY = coor.dim2;
            runtime.post.iirFilterActive = false;
        } else {
            runtime.post.iirFilterActive = true;
            ApplyIIR(coor.dim1, coor.dim2);
            m_prevFiltX = coor.dim1;
            m_prevFiltY = coor.dim2;
        }

        runtime.post.postIirCoor = coor;
        m_prevInRange = inRange;
        ++m_frameCount;
    }

private:
    uint8_t m_iirFracX = 0;
    uint8_t m_iirFracY = 0;
    int32_t m_prevFiltX = 0;
    int32_t m_prevFiltY = 0;
    bool m_prevInRange = false;
    int m_frameCount = 0;

    // TSACore status counters
    int m_counterC = 0;
    int m_counter8 = 0;
    int m_counterA = 0;

    inline uint16_t SelectCoef(int speedValue, bool writingMode, bool edgeActive) const {
        int32_t coefLow;
        int32_t coefHigh;
        int32_t speedThreshold;

        if (!writingMode) {
            // Hover (no pressure) uses hover params
            coefLow = m_coefLowHover;
            coefHigh = m_coefHighHover;
            speedThreshold = m_speedTholdHover;
        } else {
            // Writing (has pressure) uses writing params
            coefLow = m_coefLowWriting;
            coefHigh = m_coefHighWriting;
            speedThreshold = m_speedTholdWriting;
        }

        // True coordinate edge is an overlay; TSACore derives it from hover coefficients.
        if (edgeActive) {
            coefHigh = m_coefHighHover >> 1; // 16 >> 1 = 8
            coefLow = m_coefLowHover >> 1;   // 2 >> 1 = 1
        }

        uint16_t selected;
        if (speedValue < speedThreshold) {
            selected = coefLow;
        } else if (speedValue >= m_speedMax) {
            selected = coefHigh;
        } else {
            // Linear interpolation between low and high
            selected = static_cast<uint16_t>(
                coefLow + (static_cast<int>(coefHigh - coefLow) *
                           (speedValue - speedThreshold)) /
                              (m_speedMax - speedThreshold));
        }
        return selected;
    }

    inline void ApplyIIR(int32_t& dim1, int32_t& dim2) {
        const int64_t newX = static_cast<int64_t>(dim1) << 8;
        const int64_t oldX = (static_cast<int64_t>(m_prevFiltX) << 8) + static_cast<int64_t>(m_iirFracX);
        const int64_t newY = static_cast<int64_t>(dim2) << 8;
        const int64_t oldY = (static_cast<int64_t>(m_prevFiltY) << 8) + static_cast<int64_t>(m_iirFracY);

        const int64_t coef = static_cast<int64_t>(m_currentCoef);
        const int64_t antiCoef = static_cast<int64_t>(m_maxCoef) - coef;
        const int64_t denom = static_cast<int64_t>(m_maxCoef);

        const int64_t filtX = (coef * newX + antiCoef * oldX) / denom;
        const int64_t filtY = (coef * newY + antiCoef * oldY) / denom;

        dim1 = static_cast<int32_t>(filtX >> 8);
        m_iirFracX = static_cast<uint8_t>(filtX & 0xFF);

        dim2 = static_cast<int32_t>(filtY >> 8);
        m_iirFracY = static_cast<uint8_t>(filtY & 0xFF);
    }
};

} // namespace Solvers::Stylus
