#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include "BtPressBuffer.hpp"
#include "StylusFrameState.hpp"

namespace Asa {

struct EdgeSignalInputs {
    bool dim1Active = false;
    bool dim2Active = false;
    int  dim1Signal = 0;
    int  dim2Signal = 0;
};

struct PressureHistorySample {
    uint16_t rawPressure = 0;
    uint16_t mappedPressure = 0;
    uint16_t outputPressure = 0;
    uint32_t seq = 0;
    bool isReal = false;
};

struct PressureStageResult {
    uint16_t rawPressure = 0;
    uint16_t mappedPressure = 0;
    uint16_t realPressure = 0;
    bool signalSuppressActive = false;
    bool edgeSignalSuppressActive = false;
    bool isRealMeasurement = false;
    uint32_t btSeq = 0;
    uint8_t predictedAgeFrames = 0;
};

/// PressureSolver — BT MCU pressure mapping + async real-sample history + prediction.
class PressureSolver {
public:
    static constexpr int kHistorySize = 6;

    inline PressureStageResult Process(Solvers::StylusFrameState& state, bool active) {
        const bool isEdge = state.signal.dim1EdgeActive || state.signal.dim2EdgeActive;
        const EdgeSignalInputs edgeSignals{
            state.signal.dim1EdgeActive,
            state.signal.dim2EdgeActive,
            static_cast<int>(state.signal.dim1EdgeSignal),
            static_cast<int>(state.signal.dim2EdgeSignal),
        };
        const auto result = SolveStage(
            state.lifecycle.btSample,
            active,
            static_cast<int>(state.signal.signalX),
            isEdge,
            edgeSignals);
        ApplyStageToState(state, result);
        return result;
    }

    inline PressureStageResult Process(Solvers::StylusFrameState& state) {
        return Process(state, state.lifecycle.pressureGateOpen);
    }

    inline PressureStageResult Process(uint16_t rawPressure, bool active,
                                       int signalStrength = 0, bool isEdge = false,
                                       const EdgeSignalInputs& edgeSignals = {}) {
        return SolveStage(rawPressure, active, signalStrength, isEdge, edgeSignals);
    }

    inline PressureStageResult Process(const BtPressureSample& sample, bool active,
                                       int signalStrength = 0, bool isEdge = false,
                                       const EdgeSignalInputs& edgeSignals = {}) {
        return SolveStage(sample, active, signalStrength, isEdge, edgeSignals);
    }

    static inline void ApplyStageToState(Solvers::StylusFrameState& state,
                                         const PressureStageResult& result) {
        state.lifecycle.mappedPressure = result.mappedPressure;
        state.lifecycle.realPressure = result.realPressure;
        state.lifecycle.pressureIsReal = result.isRealMeasurement;
        state.lifecycle.btSeq = result.btSeq;
        state.lifecycle.predictedAgeFrames = static_cast<int>(result.predictedAgeFrames);
    }

    inline PressureStageResult SolveStage(uint16_t rawPressure, bool active,
                                          int signalStrength = 0, bool isEdge = false,
                                          const EdgeSignalInputs& edgeSignals = {}) {
        BtPressureSample sample{};
        sample.pressure = rawPressure;
        sample.seq = (rawPressure > 0 || m_lastSeq != 0) ? (m_lastSeq + 1) : 0;
        sample.hasSample = (sample.seq != 0);
        return SolveStage(sample, active, signalStrength, isEdge, edgeSignals);
    }

    inline PressureStageResult SolveStage(const BtPressureSample& sample, bool active,
                                          int signalStrength = 0, bool isEdge = false,
                                          const EdgeSignalInputs& edgeSignals = {}) {
        PressureStageResult result{};
        result.rawPressure = sample.hasSample ? sample.pressure : 0;

        if (!active) {
            Reset();
            return result;
        }

        const bool hasNewRealSample = sample.hasSample && sample.seq != 0 && sample.seq != m_lastSeq;
        if (hasNewRealSample) {
            m_lastSeq = sample.seq;
        }

        const int mapped = MapPressure(result.rawPressure);
        result.mappedPressure = static_cast<uint16_t>(mapped);
        result.isRealMeasurement = hasNewRealSample;
        result.btSeq = sample.hasSample ? sample.seq : m_lastSeq;

        (void)signalStrength;
        (void)isEdge;
        (void)edgeSignals;

        if (hasNewRealSample) {
            PredictKalman();
            UpdateKalman(static_cast<double>(mapped));
            m_predictedAgeFrames = 0;
        } else {
            PredictKalman();
            m_predictedAgeFrames = static_cast<uint8_t>(std::min<int>(255, m_predictedAgeFrames + 1));
        }

        int output = static_cast<int>(std::lround(m_statePos));
        if (hasNewRealSample && mapped == 0) {
            output = 0;
            m_statePos = 0.0;
            m_stateVel = 0.0;
            ResetCovarianceForZero();
            m_predictedAgeFrames = 0;
        }
        output = std::clamp(output, 0, 0x0FFF);

        result.realPressure = static_cast<uint16_t>(output);
        result.predictedAgeFrames = m_predictedAgeFrames;
        result.signalSuppressActive = false;
        result.edgeSignalSuppressActive = false;

        PushHistory({
            result.rawPressure,
            result.mappedPressure,
            result.realPressure,
            result.btSeq,
            result.isRealMeasurement,
        });
        m_prevPressure = result.realPressure;
        m_signalSuppressActive = false;
        m_edgeSignalSuppressActive = false;
        return result;
    }

    inline uint16_t Solve(uint16_t rawPressure, bool active,
                          int signalStrength = 0, bool isEdge = false) {
        return SolveStage(rawPressure, active, signalStrength, isEdge).realPressure;
    }

    inline void Reset() {
        m_prevPressure = 0;
        m_signalSuppressActive = false;
        m_edgeSignalSuppressActive = false;
        m_lastSeq = 0;
        m_predictedAgeFrames = 0;
        m_statePos = 0.0;
        m_stateVel = 0.0;
        ResetCovariance();
        ClearHistory();
    }

    inline void ResetSuppression() {
        Reset();
    }

    inline int GetHistoryCount() const {
        return m_historyCount;
    }

    inline bool TryGetHistorySample(int logicalIndex, PressureHistorySample& out) const {
        if (logicalIndex < 0 || logicalIndex >= m_historyCount) {
            return false;
        }
        const int idx = (m_historyHead + logicalIndex) % kHistorySize;
        out = m_history[static_cast<size_t>(idx)];
        return true;
    }

    inline uint32_t GetLastSeq() const {
        return m_lastSeq;
    }

    // ── Configuration ──
    int   iirWeightQ8 = 64;
    bool  polyEnabled = true;
    std::array<double, 5> polySeg1{{0.0, 0.0, 0.0078740157480315, 0.0, 0.0}};
    std::array<double, 5> polySeg2{{-409.317785463, 4.39982201266, -0.00161165641489,
                                     2.623779267e-07, -1.60182e-11}};
    int   seg1Threshold = 11;
    int   seg2Threshold = 127;
    int   gainPercent = 100;

    bool  signalSuppressEnabled = true;
    int   signalSuppressEnter = 2200;
    int   signalSuppressExit = 3200;

    bool  edgeSignalSuppressEnabled = true;
    int   edgeSignalSuppressEnter = 1500;
    int   edgeSignalSuppressExit = 3000;

    double kalmanProcessNoisePos = 6.0;
    double kalmanProcessNoiseVel = 2.0;
    double kalmanMeasureNoise = 16.0;

private:
    uint16_t m_prevPressure = 0;
    bool     m_signalSuppressActive = false;
    bool     m_edgeSignalSuppressActive = false;
    uint32_t m_lastSeq = 0;
    uint8_t  m_predictedAgeFrames = 0;

    double m_statePos = 0.0;
    double m_stateVel = 0.0;
    double m_cov00 = 1.0;
    double m_cov01 = 0.0;
    double m_cov10 = 0.0;
    double m_cov11 = 1.0;

    std::array<PressureHistorySample, kHistorySize> m_history{};
    int m_historyHead = 0;
    int m_historyCount = 0;

    inline int MapPressure(uint16_t rawPressure) const {
        const int x = static_cast<int>(rawPressure);
        int mapped = 0;
        if (x <= seg1Threshold) {
            mapped = (x > 1) ? 1 : x;
        } else if (polyEnabled) {
            mapped = (x <= seg2Threshold)
                ? EvaluatePolynomial(polySeg1, x)
                : EvaluatePolynomial(polySeg2, x);
        } else {
            mapped = x;
        }
        mapped = mapped * std::clamp(gainPercent, 1, 1000) / 100;
        return std::clamp(mapped, 0, 0x0FFF);
    }

    inline void PredictKalman() {
        m_statePos += m_stateVel;

        const double p00 = m_cov00 + m_cov01 + m_cov10 + m_cov11 + kalmanProcessNoisePos;
        const double p01 = m_cov01 + m_cov11;
        const double p10 = m_cov10 + m_cov11;
        const double p11 = m_cov11 + kalmanProcessNoiseVel;
        m_cov00 = p00;
        m_cov01 = p01;
        m_cov10 = p10;
        m_cov11 = p11;
    }

    inline void UpdateKalman(double measurement) {
        const double innovation = measurement - m_statePos;
        const double s = m_cov00 + std::max(1.0, kalmanMeasureNoise);
        const double k0 = m_cov00 / s;
        const double k1 = m_cov10 / s;

        m_statePos += k0 * innovation;
        m_stateVel += k1 * innovation;

        const double p00 = (1.0 - k0) * m_cov00;
        const double p01 = (1.0 - k0) * m_cov01;
        const double p10 = m_cov10 - k1 * m_cov00;
        const double p11 = m_cov11 - k1 * m_cov01;
        m_cov00 = p00;
        m_cov01 = p01;
        m_cov10 = p10;
        m_cov11 = p11;
    }

    inline void ResetCovariance() {
        m_cov00 = 1.0;
        m_cov01 = 0.0;
        m_cov10 = 0.0;
        m_cov11 = 1.0;
    }

    inline void ResetCovarianceForZero() {
        m_cov00 = 0.5;
        m_cov01 = 0.0;
        m_cov10 = 0.0;
        m_cov11 = 0.5;
    }

    inline void ClearHistory() {
        m_historyHead = 0;
        m_historyCount = 0;
        m_history.fill({});
    }

    inline void PushHistory(const PressureHistorySample& sample) {
        if (m_historyCount < kHistorySize) {
            const int insert = (m_historyHead + m_historyCount) % kHistorySize;
            m_history[static_cast<size_t>(insert)] = sample;
            ++m_historyCount;
            return;
        }
        m_history[static_cast<size_t>(m_historyHead)] = sample;
        m_historyHead = (m_historyHead + 1) % kHistorySize;
    }

    static inline int EvaluatePolynomial(const std::array<double, 5>& c, int x) {
        const double d = static_cast<double>(x);
        const double result =
            (((c[4] * d + c[3]) * d + c[2]) * d + c[1]) * d + c[0];
        return static_cast<int>(result);
    }
};

} // namespace Asa
