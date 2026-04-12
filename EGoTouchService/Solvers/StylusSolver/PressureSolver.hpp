#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <mutex>

namespace Asa {

struct EdgeSignalInputs {
    bool dim1Active = false;
    bool dim2Active = false;
    int  dim1Signal = 0;
    int  dim2Signal = 0;
};

struct PressureStageResult {
    uint16_t rawPressure = 0;
    uint16_t mappedPressure = 0;
    uint16_t realPressure = 0;
    uint16_t outputPressure = 0;
    bool signalSuppressActive = false;
    bool edgeSignalSuppressActive = false;
};

/// PressureSolver — BT MCU pressure mapping, IIR, tail decay, and signal suppression.
///
/// Mirrors TSACore pressure chain: polynomial mapping → IIR → tail decay.
/// Also implements HPP3_SuppressBtPressBySignal hysteresis.
class PressureSolver {
public:
    inline PressureStageResult SolveStage(uint16_t rawPressure, bool active,
                                          int signalStrength = 0, bool isEdge = false,
                                          const EdgeSignalInputs& edgeSignals = {}) {
        PressureStageResult result{};
        result.rawPressure = rawPressure;

        if (!active) {
            Reset();
            return result;
        }

        const int x = static_cast<int>(rawPressure);
        int mapped = 0;
        if (x <= seg1Threshold) {
            mapped = (x > 1) ? 1 : x;
        } else if (polyEnabled) {
            const auto eval = [x](const std::array<double, 5>& c) {
                double d = static_cast<double>(x);
                return static_cast<int>(
                    c[0] + c[1]*d + c[2]*d*d + c[3]*d*d*d + c[4]*d*d*d*d);
            };
            mapped = (x <= seg2Threshold) ? eval(polySeg1) : eval(polySeg2);
        } else {
            mapped = x;
        }
        mapped = mapped * std::clamp(gainPercent, 1, 1000) / 100;
        mapped = std::clamp(mapped, 0, 0x0FFF);

        if (mapped > 0 && m_prevPressure > 0) {
            const int w = std::clamp(iirWeightQ8, 1, 255);
            mapped = ((static_cast<int>(m_prevPressure) *
                       (256 - w)) + mapped * w + 128) >> 8;
            mapped = std::clamp(mapped, 0, 0x0FFF);
        }

        if (mapped == 0 && m_prevPressure > 0 &&
            tailFrames > 0 && m_tailCounter < tailFrames) {
            mapped = std::max(tailMin,
                std::max(0, static_cast<int>(m_prevPressure) -
                            std::max(1, tailDecay)));
            mapped = std::clamp(mapped, 0, 0x0FFF);
            m_tailCounter++;
        } else if (mapped > 0) {
            m_tailCounter = 0;
        }

        result.mappedPressure = static_cast<uint16_t>(mapped);
        result.realPressure = result.mappedPressure;

        if (mapped == 0) {
            m_signalSuppressActive = false;
            m_edgeSignalSuppressActive = false;
        } else {
            if (signalSuppressEnabled) {
                if (!m_signalSuppressActive) {
                    if (!isEdge && signalStrength < signalSuppressEnter) {
                        m_signalSuppressActive = true;
                    }
                } else if (signalStrength > signalSuppressExit) {
                    m_signalSuppressActive = false;
                }
            } else {
                m_signalSuppressActive = false;
            }

            const bool anyEdgeActive = edgeSignals.dim1Active || edgeSignals.dim2Active;
            if (!edgeSignalSuppressEnabled || !anyEdgeActive) {
                m_edgeSignalSuppressActive = false;
            } else if (!m_edgeSignalSuppressActive) {
                if ((edgeSignals.dim1Active &&
                     edgeSignals.dim1Signal < edgeSignalSuppressEnter) ||
                    (edgeSignals.dim2Active &&
                     edgeSignals.dim2Signal < edgeSignalSuppressEnter)) {
                    m_edgeSignalSuppressActive = true;
                }
            } else {
                bool recovered = true;
                if (edgeSignals.dim1Active) {
                    recovered = recovered &&
                                (edgeSignals.dim1Signal > edgeSignalSuppressExit);
                }
                if (edgeSignals.dim2Active) {
                    recovered = recovered &&
                                (edgeSignals.dim2Signal > edgeSignalSuppressExit);
                }
                if (recovered) {
                    m_edgeSignalSuppressActive = false;
                }
            }
        }

        if (m_signalSuppressActive || m_edgeSignalSuppressActive) {
            result.realPressure = 0;
            m_prevPressure = 0;
            m_tailCounter = 0;
        } else {
            m_prevPressure = result.mappedPressure;
        }

        result.signalSuppressActive = m_signalSuppressActive;
        result.edgeSignalSuppressActive = m_edgeSignalSuppressActive;
        result.outputPressure = result.realPressure;
        return result;
    }

    /// Solve pressure for current frame.
    /// @param rawPressure  Raw BT MCU pressure value
    /// @param active       Whether pen is in valid contact
    /// @param signalStrength  Peak signal magnitude (for signal suppression gate)
    /// @param isEdge       Whether pen is in edge region (bypasses suppression)
    /// @return Mapped + filtered pressure (0..4095)
    inline uint16_t Solve(uint16_t rawPressure, bool active,
                          int signalStrength = 0, bool isEdge = false) {
        return SolveStage(rawPressure, active, signalStrength, isEdge).outputPressure;
    }

    /// Reset pressure and suppression state (on pen-up / invalid frame).
    inline void Reset() {
        m_prevPressure = 0;
        m_tailCounter = 0;
        m_signalSuppressActive = false;
        m_edgeSignalSuppressActive = false;
    }

    /// Backward-compatible alias.
    inline void ResetSuppression() {
        Reset();
    }

    /// Inject BT MCU pressure sample with timestamp
    inline void SetBtMcuPressure(uint16_t p) {
        auto nowObj = std::chrono::steady_clock::now();
        uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              nowObj.time_since_epoch()).count();
        std::lock_guard<std::mutex> lock(m_btMutex);
        PushBtSample({now_ms, p});
    }

    /// Get the most recent valid pressure sample from BT MCU history
    inline uint16_t GetLatestBtPressure() {
        uint16_t btPress = 0;
        auto nowObj = std::chrono::steady_clock::now();
        uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              nowObj.time_since_epoch()).count();
        std::lock_guard<std::mutex> lock(m_btMutex);
        DiscardExpiredSamples(now_ms);
        for (size_t offset = 0; offset < m_btCount; ++offset) {
            const size_t logicalIndex = m_btCount - 1 - offset;
            const auto& sample = BtSampleAt(logicalIndex);
            if (now_ms <= sample.timestamp_ms + kBtAggregationWindowMs) {
                if (sample.pressure > btPress) btPress = sample.pressure;
            }
        }
        return btPress;
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
    int   tailFrames = 0;
    int   tailMin = 10;
    int   tailDecay = 48;

    // Signal suppression hysteresis
    bool  signalSuppressEnabled = true;
    int   signalSuppressEnter = 2200;
    int   signalSuppressExit = 3200;

    // Edge signal suppression hysteresis
    bool  edgeSignalSuppressEnabled = true;
    int   edgeSignalSuppressEnter = 1500;
    int   edgeSignalSuppressExit = 3000;

private:
    uint16_t m_prevPressure = 0;
    int      m_tailCounter = 0;
    bool     m_signalSuppressActive = false;
    bool     m_edgeSignalSuppressActive = false;

    struct BtPressureSample {
        uint64_t timestamp_ms;
        uint16_t pressure;
    };

    static constexpr uint64_t kBtAggregationWindowMs = 28;
    static constexpr uint64_t kBtHistoryKeepMs = 60;
    static constexpr size_t kBtHistoryCapacity = 20;

    mutable std::mutex m_btMutex;
    std::array<BtPressureSample, kBtHistoryCapacity> m_btHistory{};
    size_t m_btHead = 0;
    size_t m_btCount = 0;

    inline size_t BtPhysicalIndex(size_t logicalIndex) const {
        return (m_btHead + logicalIndex) % kBtHistoryCapacity;
    }

    inline const BtPressureSample& BtSampleAt(size_t logicalIndex) const {
        return m_btHistory[BtPhysicalIndex(logicalIndex)];
    }

    inline void PushBtSample(const BtPressureSample& sample) {
        if (m_btCount < kBtHistoryCapacity) {
            m_btHistory[BtPhysicalIndex(m_btCount)] = sample;
            ++m_btCount;
            return;
        }

        m_btHistory[m_btHead] = sample;
        m_btHead = (m_btHead + 1) % kBtHistoryCapacity;
    }

    inline void DiscardExpiredSamples(uint64_t now_ms) {
        while (m_btCount > 0 &&
               now_ms > BtSampleAt(0).timestamp_ms + kBtHistoryKeepMs) {
            m_btHead = (m_btHead + 1) % kBtHistoryCapacity;
            --m_btCount;
        }
    }
};

} // namespace Asa
