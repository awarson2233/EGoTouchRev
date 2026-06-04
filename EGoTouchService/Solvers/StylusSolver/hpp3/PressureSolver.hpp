#pragma once

#include "SolverTypes.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace Solvers::Stylus::Hpp3 {

struct PressureHistorySample {
    uint16_t rawPressure = 0;
    uint16_t mappedPressure = 0;
    uint16_t outputPressure = 0;
    uint32_t seq = 0;
    bool isReal = false;
};

class PressureSolver {
public:
    enum BtPressureMapOrderMode : int {
        Direct = 0,
        OnCell = 1,
        InCell = 2,
    };

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
    int m_btPressureMapOrderMode = Direct;
    uint16_t m_btPressSignalSuppressEnterThreshold = 2200;
    uint16_t m_btPressSignalSuppressExitThreshold = 3200;

    inline bool Process(Context& ctx) {
        auto& stylus = ctx.frame.stylus;
        auto& runtime = ctx.runtime;
        auto& flow = runtime.flow;
        auto& pressure = runtime.pressure;
        auto& decision = runtime.decision;

        flow.pipelineStage = 5;
        pressure = {};
        pressure.btSample = stylus.input.btSample;

        if (!m_enabled) return true;

        UpdateBtPacket(stylus.input.btSample);

        pressure.btSeq = m_haveBtPacket ? m_lastSeq : 0;
        pressure.pressureIsReal = m_haveBtPacket;
        pressure.predictedAgeFrames = 0;
        pressure.lookaheadHoverGate = m_haveBtPacket && m_btPressBuf[3] == 0;

        if (m_haveBtPacket && !pressure.lookaheadHoverGate) {
            pressure.rawPressure = GetPressureInMapOrder();
            pressure.mappedPressure = static_cast<uint16_t>(MapPressure(pressure.rawPressure));
            pressure.outputPressure = pressure.mappedPressure;
#if EGOTOUCH_DIAG
            pressure.preIirPressure = pressure.mappedPressure;
            pressure.polySegment = DeterminePolySegment(pressure.rawPressure);
#endif

            if (pressure.outputPressure != 0 && m_prevOutputPressure != 0) {
                pressure.outputPressure = ApplyIir(pressure.outputPressure, m_prevOutputPressure);
            }
        }

        if (m_haveBtPacket) {
            SuppressBtPressBySignal(runtime.signal, pressure);
        }
#if EGOTOUCH_DIAG
        pressure.btPressSuppressActive = m_btPressSignalSuppressLatched;
#endif

        decision.tipDownCandidate =
            decision.inRangeCandidate &&
            (pressure.outputPressure >= m_tipDownPressureThreshold);
        decision.authoritativeDown = decision.tipDownCandidate;

        PushHistory({pressure.rawPressure,
                     pressure.mappedPressure,
                     pressure.outputPressure,
                     pressure.btSeq,
                     pressure.pressureIsReal});

        m_prevOutputPressure = pressure.outputPressure;
        if (m_haveBtPacket) {
            m_btPressCnt = static_cast<uint8_t>(m_btPressCnt + 1);
        }
        return true;
    }

    inline void Reset() {
        m_lastSeq = 0;
        m_haveBtPacket = false;
        m_btPressCnt = 0;
        m_prevOutputPressure = 0;
        m_btPressSignalSuppressLatched = false;
        m_btPressBuf.fill(0);
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
    static constexpr std::array<uint8_t, 6> kOnCellOrder{{0, 1, 1, 2, 3, 3}};
    static constexpr std::array<uint8_t, 4> kInCellOrder{{0, 1, 2, 3}};

    uint32_t m_lastSeq = 0;
    bool m_haveBtPacket = false;
    uint8_t m_btPressCnt = 0;
    uint16_t m_prevOutputPressure = 0;
    bool m_btPressSignalSuppressLatched = false;
    std::array<uint16_t, 4> m_btPressBuf{};

    std::array<PressureHistorySample, kHistorySize> m_history{};
    int m_historyHead = 0;
    int m_historyCount = 0;

    inline void UpdateBtPacket(const Asa::BtInputSnapshot& btSample) {
        if (!btSample.hasSample) return;
        if (m_haveBtPacket && btSample.seq == m_lastSeq) return;
        m_btPressBuf = btSample.pressure;
        m_lastSeq = btSample.seq;
        m_btPressCnt = 0;
        m_haveBtPacket = true;
    }

    inline uint16_t GetPressureInMapOrder() const {
        if (m_btPressureMapOrderMode == OnCell) {
            if (m_btPressCnt < kOnCellOrder.size() && m_btPressBuf[0] != 0) {
                return m_btPressBuf[kOnCellOrder[static_cast<std::size_t>(m_btPressCnt)]];
            }
            return m_btPressBuf[3];
        }

        if (m_btPressureMapOrderMode == InCell) {
            if (m_btPressCnt < kInCellOrder.size() && m_btPressBuf[0] != 0) {
                return m_btPressBuf[kInCellOrder[static_cast<std::size_t>(m_btPressCnt)]];
            }
            return m_btPressBuf[3];
        }

        return m_btPressBuf[3];
    }

    inline int MapPressure(uint16_t rawPressure) const {
        const int x = static_cast<int>(rawPressure);
        if (x == 0x0FFF) {
            return 0x0FFF;
        }

        int mapped = 0;
        if (x <= m_seg1Threshold) {
            mapped = (x > 1) ? 1 : x;
        } else if (!m_polyEnabled) {
            mapped = x;
        } else if (x > m_seg2Threshold) {
            mapped = EvaluatePolynomial(m_polySeg2, x);
        } else {
            mapped = EvaluatePolynomial(m_polySeg1, x);
        }

        mapped = mapped * std::clamp(m_gainPercent, 1, 1000) / 100;
        return std::clamp(mapped, 0, 0x0FFF);
    }

    inline uint16_t ApplyIir(uint16_t current, uint16_t previous) const {
        const int weight = std::clamp(m_iirWeightQ8, 0, 0x7F);
        const int mixed = previous * (0x80 - weight) + current * weight;
        return static_cast<uint16_t>(std::clamp(mixed >> 7, 0, 0x0FFF));
    }

    inline void SuppressBtPressBySignal(const Asa::SignalRuntime& signal,
                                        Asa::PressureRuntime& pressure) {
        if (pressure.outputPressure == 0) {
            m_btPressSignalSuppressLatched = false;
        }

        if (!m_btPressSignalSuppressLatched) {
            if (signal.signalX < m_btPressSignalSuppressEnterThreshold &&
                !signal.dim2EdgeActive &&
                !signal.dim1EdgeActive) {
                m_btPressSignalSuppressLatched = true;
                pressure.outputPressure = 0;
            }
            return;
        }

        if (signal.signalX > m_btPressSignalSuppressExitThreshold) {
            m_btPressSignalSuppressLatched = false;
            return;
        }

        pressure.outputPressure = 0;
    }

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

    inline uint8_t DeterminePolySegment(uint16_t rawPressure) const {
        const int x = static_cast<int>(rawPressure);
        if (x <= 1) return 0;
        if (x <= m_seg1Threshold) return 1;
        if (!m_polyEnabled || x <= m_seg2Threshold) return 2;
        return 3;
    }
};

} // namespace Solvers::Stylus::Hpp3
