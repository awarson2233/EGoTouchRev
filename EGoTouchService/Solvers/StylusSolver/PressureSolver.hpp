#pragma once

#include "SolverTypes.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace Solvers::Stylus {

struct PressureHistorySample {
    uint16_t rawPressure = 0;
    uint16_t mappedPressure = 0;
    uint16_t outputPressure = 0;
    uint32_t seq = 0;
    bool isReal = false;
};

class PressureSolver {
public:
    static constexpr int kHistorySize = 6;

    bool m_enabled = true;
    uint16_t m_tipDownPressureThreshold = 1;

    int m_iirWeightQ8 = 64;
    bool m_polyEnabled = true;
    std::array<double, 5> m_polySeg1{{0.0, 0.0, 0.0078740157480315, 0.0, 0.0}};
    std::array<double, 5> m_polySeg2{{-409.317785463, 4.39982201266, -0.00161165641489,
                                      2.623779267e-07, -1.60182e-11}};
    int m_seg1Threshold = 11;
    int m_seg2Threshold = 127;
    int m_gainPercent = 100;

    double m_kalmanProcessNoisePos = 6.0;
    double m_kalmanProcessNoiseVel = 2.0;
    double m_kalmanMeasureNoise = 16.0;

    inline bool Process(HeatmapFrame& frame) {
        auto& stylus = frame.stylus;
        auto& flow = stylus.runtime.flow;
        auto& pressure = stylus.runtime.pressure;
        auto& decision = stylus.runtime.decision;

        flow.pipelineStage = 5;
        pressure = {};
        pressure.btSample = stylus.input.btSample;

        if (!m_enabled) return true;
        if (!decision.inRangeCandidate) { Reset(); return true; }

        const bool hasSample = stylus.input.btSample.hasSample;
        const bool hasNewRealSample =
            hasSample && stylus.input.btSample.seq != 0 && stylus.input.btSample.seq != m_lastSeq;

        pressure.rawPressure = hasSample ? stylus.input.btSample.pressure : 0;
        pressure.mappedPressure = static_cast<uint16_t>(MapPressure(pressure.rawPressure));
        pressure.pressureIsReal = hasNewRealSample;
        pressure.btSeq = hasSample ? stylus.input.btSample.seq : m_lastSeq;

        if (hasNewRealSample) {
            m_lastSeq = stylus.input.btSample.seq;
            PredictKalman();
            UpdateKalman(static_cast<double>(pressure.mappedPressure));
            m_predictedAgeFrames = 0;
        } else {
            PredictKalman();
            m_predictedAgeFrames = static_cast<uint8_t>(std::min<int>(255, m_predictedAgeFrames + 1));
        }

        int output = static_cast<int>(std::lround(m_statePos));
        if (hasNewRealSample && pressure.mappedPressure == 0) {
            output = 0;
            m_statePos = 0.0;
            m_stateVel = 0.0;
            ResetCovarianceForZero();
            m_predictedAgeFrames = 0;
        }

        pressure.outputPressure = static_cast<uint16_t>(std::clamp(output, 0, 0x0FFF));
        pressure.predictedAgeFrames = m_predictedAgeFrames;

        decision.tipDownCandidate =
            decision.inRangeCandidate && (pressure.outputPressure >= m_tipDownPressureThreshold);
        decision.authoritativeDown = decision.tipDownCandidate;

        PushHistory({pressure.rawPressure, pressure.mappedPressure,
                     pressure.outputPressure, pressure.btSeq, pressure.pressureIsReal});
        return true;
    }

    inline void Reset() {
        m_lastSeq = 0;
        m_predictedAgeFrames = 0;
        m_statePos = 0.0;
        m_stateVel = 0.0;
        ResetCovariance();
        ClearHistory();
    }

    inline int GetHistoryCount() const { return m_historyCount; }

    inline bool TryGetHistorySample(int logicalIndex, PressureHistorySample& out) const {
        if (logicalIndex < 0 || logicalIndex >= m_historyCount) return false;
        const int idx = (m_historyHead + logicalIndex) % kHistorySize;
        out = m_history[static_cast<std::size_t>(idx)];
        return true;
    }

    inline uint32_t GetLastSeq() const { return m_lastSeq; }

private:
    uint32_t m_lastSeq = 0;
    uint8_t m_predictedAgeFrames = 0;
    double m_statePos = 0.0;
    double m_stateVel = 0.0;
    double m_cov00 = 1.0, m_cov01 = 0.0, m_cov10 = 0.0, m_cov11 = 1.0;

    std::array<PressureHistorySample, kHistorySize> m_history{};
    int m_historyHead = 0;
    int m_historyCount = 0;

    inline int MapPressure(uint16_t rawPressure) const {
        const int x = static_cast<int>(rawPressure);
        int mapped = 0;
        if (x <= m_seg1Threshold) {
            mapped = (x > 1) ? 1 : x;
        } else if (m_polyEnabled) {
            mapped = (x <= m_seg2Threshold)
                ? EvaluatePolynomial(m_polySeg1, x)
                : EvaluatePolynomial(m_polySeg2, x);
        } else {
            mapped = x;
        }
        mapped = mapped * std::clamp(m_gainPercent, 1, 1000) / 100;
        return std::clamp(mapped, 0, 0x0FFF);
    }

    inline void PredictKalman() {
        m_statePos += m_stateVel;
        const double p00 = m_cov00 + m_cov01 + m_cov10 + m_cov11 + m_kalmanProcessNoisePos;
        const double p01 = m_cov01 + m_cov11;
        const double p10 = m_cov10 + m_cov11;
        const double p11 = m_cov11 + m_kalmanProcessNoiseVel;
        m_cov00 = p00; m_cov01 = p01; m_cov10 = p10; m_cov11 = p11;
    }

    inline void UpdateKalman(double measurement) {
        const double innovation = measurement - m_statePos;
        const double s = m_cov00 + std::max(1.0, m_kalmanMeasureNoise);
        const double k0 = m_cov00 / s;
        const double k1 = m_cov10 / s;
        m_statePos += k0 * innovation;
        m_stateVel += k1 * innovation;
        const double p00 = (1.0 - k0) * m_cov00;
        const double p01 = (1.0 - k0) * m_cov01;
        const double p10 = m_cov10 - k1 * m_cov00;
        const double p11 = m_cov11 - k1 * m_cov01;
        m_cov00 = p00; m_cov01 = p01; m_cov10 = p10; m_cov11 = p11;
    }

    inline void ResetCovariance() { m_cov00 = 1.0; m_cov01 = 0.0; m_cov10 = 0.0; m_cov11 = 1.0; }
    inline void ResetCovarianceForZero() { m_cov00 = 0.5; m_cov01 = 0.0; m_cov10 = 0.0; m_cov11 = 0.5; }

    inline void ClearHistory() {
        m_historyHead = 0;
        m_historyCount = 0;
        m_history.fill({});
    }

    inline void PushHistory(const PressureHistorySample& sample) {
        if (m_historyCount < kHistorySize) {
            const int insert = (m_historyHead + m_historyCount) % kHistorySize;
            m_history[static_cast<std::size_t>(insert)] = sample;
            ++m_historyCount;
            return;
        }
        m_history[static_cast<std::size_t>(m_historyHead)] = sample;
        m_historyHead = (m_historyHead + 1) % kHistorySize;
    }

    static inline int EvaluatePolynomial(const std::array<double, 5>& c, int x) {
        const double d = static_cast<double>(x);
        const double result = (((c[4] * d + c[3]) * d + c[2]) * d + c[1]) * d + c[0];
        return static_cast<int>(result);
    }
};

} // namespace Solvers::Stylus
