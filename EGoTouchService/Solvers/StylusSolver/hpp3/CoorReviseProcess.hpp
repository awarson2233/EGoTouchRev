#pragma once

#include "StylusSolver/AsaTypes.hpp"
#include "SolverTypes.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace Solvers::Stylus {

class CoorReviseProcess {
public:
    bool m_enabled = true;
    int m_factorDim1 = 5;
    int m_factorDim2 = 5;
    int m_maxChangePerFrame = 15;
    int m_sensorDim1Count = 60;
    int m_sensorDim2Count = 40;

    inline void Reset() {
        m_offsetDim1Buf.fill(0);
        m_offsetDim2Buf.fill(0);
        m_bufCnt = 0;
        m_smoothedOffsetDim1 = 0;
        m_smoothedOffsetDim2 = 0;
        m_prevPressure = 0;
    }

    inline void Process(HeatmapFrame& frame) {
        auto& runtime = frame.stylus.runtime;
        auto& finalCoor = runtime.post.finalCoor;
        const auto& tilt = runtime.tilt;

        runtime.flow.pipelineStage = 6;
        if (!m_enabled || !finalCoor.valid) {
            m_prevPressure = runtime.pressure.outputPressure;
#if EGOTOUCH_DIAG
            runtime.post.coorReviseActive = false;
            runtime.post.coorReviseCorrectionDim1 = 0;
            runtime.post.coorReviseCorrectionDim2 = 0;
#endif
            return;
        }

        // Stylus-lift reset: clear history on pressure transition to 0
        if (m_prevPressure != 0 && runtime.pressure.outputPressure == 0) {
            ResetHistory();
        }
        m_prevPressure = runtime.pressure.outputPressure;

        if (!tilt.valid) {
#if EGOTOUCH_DIAG
            runtime.post.coorReviseActive = false;
            runtime.post.coorReviseCorrectionDim1 = 0;
            runtime.post.coorReviseCorrectionDim2 = 0;
#endif
            return;
        }

        // rawOffset = flashParam * reportTiltAngle
        int16_t rawOffsetDim1 = static_cast<int16_t>(m_factorDim1 * tilt.reportTiltDim1);
        int16_t rawOffsetDim2 = static_cast<int16_t>(m_factorDim2 * tilt.reportTiltDim2);

        if (runtime.pressure.outputPressure != 0) {
            if (m_bufCnt != 0) {
                rawOffsetDim1 = LimitingFilter(m_smoothedOffsetDim1,
                                               rawOffsetDim1,
                                               static_cast<int16_t>(m_maxChangePerFrame));
                rawOffsetDim2 = LimitingFilter(m_smoothedOffsetDim2,
                                               rawOffsetDim2,
                                               static_cast<int16_t>(m_maxChangePerFrame));
            }
            m_smoothedOffsetDim1 = rawOffsetDim1;
            m_smoothedOffsetDim2 = rawOffsetDim2;
            PushOffset(rawOffsetDim1, rawOffsetDim2);
            m_smoothedOffsetDim1 = GetAverage(5, 0);
            m_smoothedOffsetDim2 = GetAverage(5, 1);
        } else {
            // Hover: use raw tilt offset directly (no smoothing)
            // TSACore CoorReviseCalculation stores raw values when pressure==0,
            // and CoorReviseWork always applies g_coorReviseX/Y regardless.
            m_smoothedOffsetDim1 = rawOffsetDim1;
            m_smoothedOffsetDim2 = rawOffsetDim2;
        }

        const int32_t maxDim1 = m_sensorDim1Count * Asa::kCoorUnit;
        const int32_t maxDim2 = m_sensorDim2Count * Asa::kCoorUnit;

        int32_t correctedDim1 = finalCoor.dim1 + m_smoothedOffsetDim1;
        int32_t correctedDim2 = finalCoor.dim2 + m_smoothedOffsetDim2;
        correctedDim1 = std::clamp(correctedDim1, 0, maxDim1);
        correctedDim2 = std::clamp(correctedDim2, 0, maxDim2);

        finalCoor.dim1 = correctedDim1;
        finalCoor.dim2 = correctedDim2;

#if EGOTOUCH_DIAG
        runtime.post.coorReviseActive = true;
        runtime.post.coorReviseCorrectionDim1 = m_smoothedOffsetDim1;
        runtime.post.coorReviseCorrectionDim2 = m_smoothedOffsetDim2;
#endif
    }

private:
    static constexpr int kHistorySize = 10;

    std::array<int16_t, kHistorySize> m_offsetDim1Buf{};
    std::array<int16_t, kHistorySize> m_offsetDim2Buf{};
    int m_bufCnt = 0;
    int16_t m_smoothedOffsetDim1 = 0;
    int16_t m_smoothedOffsetDim2 = 0;
    uint16_t m_prevPressure = 0;

    static inline int16_t LimitingFilter(int16_t prev, int16_t cur, int16_t limit) {
        const int16_t upper = static_cast<int16_t>(prev + limit);
        const int16_t lower = static_cast<int16_t>(prev - limit);
        if (upper < cur) return upper;
        if (cur < lower) return lower;
        return cur;
    }

    inline void PushOffset(int16_t dim1, int16_t dim2) {
        ++m_bufCnt;
        if (m_bufCnt > kHistorySize) m_bufCnt = kHistorySize;
        for (int i = kHistorySize - 1; i > 0; --i) {
            m_offsetDim1Buf[static_cast<std::size_t>(i)] =
                m_offsetDim1Buf[static_cast<std::size_t>(i - 1)];
            m_offsetDim2Buf[static_cast<std::size_t>(i)] =
                m_offsetDim2Buf[static_cast<std::size_t>(i - 1)];
        }
        m_offsetDim1Buf[0] = dim1;
        m_offsetDim2Buf[0] = dim2;
    }

    inline int16_t GetAverage(int count, int axis) const {
        const int n = std::min(count <= 0 ? 1 : count, m_bufCnt);
        if (n <= 0) return 0;
        int32_t sum = 0;
        for (int i = 0; i < n; ++i) {
            sum += (axis == 0) ? m_offsetDim1Buf[static_cast<std::size_t>(i)]
                               : m_offsetDim2Buf[static_cast<std::size_t>(i)];
        }
        return static_cast<int16_t>(sum / n);
    }

    inline void ResetHistory() {
        m_offsetDim1Buf.fill(0);
        m_offsetDim2Buf.fill(0);
        m_bufCnt = 0;
        m_smoothedOffsetDim1 = 0;
        m_smoothedOffsetDim2 = 0;
    }
};

} // namespace Solvers::Stylus
