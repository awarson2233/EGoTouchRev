#pragma once
#include <atomic>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>

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
            mapped = (x <= seg2Threshold)
                ? EvaluatePolynomial(polySeg1, x)
                : EvaluatePolynomial(polySeg2, x);
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
        const uint64_t writeSeq = m_btWriteSeq.load(std::memory_order_relaxed);
        const size_t slot = static_cast<size_t>(writeSeq % kBtHistoryCapacity);
        m_btHistory[slot].store(PackBtSample(now_ms, p), std::memory_order_relaxed);
        m_btWriteSeq.store(writeSeq + 1, std::memory_order_release);
    }

    /// Get the most recent valid pressure sample from BT MCU history
    inline uint16_t GetLatestBtPressure() {
        uint16_t btPress = 0;
        auto nowObj = std::chrono::steady_clock::now();
        uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              nowObj.time_since_epoch()).count();
        const uint64_t endSeq = m_btWriteSeq.load(std::memory_order_acquire);
        const size_t available = static_cast<size_t>(
            std::min<uint64_t>(endSeq, kBtHistoryCapacity));

        for (size_t offset = 0; offset < available; ++offset) {
            const uint64_t seq = endSeq - 1 - static_cast<uint64_t>(offset);
            const size_t slot = static_cast<size_t>(seq % kBtHistoryCapacity);
            const uint64_t packed = m_btHistory[slot].load(std::memory_order_acquire);
            const BtPressureSample sample = UnpackBtSample(packed);

            if (now_ms > sample.timestamp_ms + kBtHistoryKeepMs) {
                continue;
            }
            if (now_ms <= sample.timestamp_ms + kBtAggregationWindowMs &&
                sample.pressure > btPress) {
                btPress = sample.pressure;
            }
        }
        return btPress;
    }

    // ── Configuration ──
    int   iirWeightQ8 = 59;
    bool  polyEnabled = true;
    std::array<double, 5> polySeg1{{0.0, 0.0, 0.0078740157480315, 0.0, 0.0}};
    std::array<double, 5> polySeg2{{-409.317785463, 4.39982201266, -0.00161165641489,
                                     2.623779267e-07, -1.60182e-11}};
    int   seg1Threshold = 12;
    int   seg2Threshold = 127;
    int   gainPercent = 100;
    int   tailFrames = false;
    int   tailMin = false;
    int   tailDecay = 110;

    // Signal suppression hysteresis
<<<<<<< HEAD
    bool  signalSuppressEnabled = false;
=======
    bool  signalSuppressEnabled = true;
>>>>>>> 29173d3c1b03f35b9a1eebf911b0159c63b32529
    int   signalSuppressEnter = 582;
    int   signalSuppressExit = 1488;

    // Edge signal suppression hysteresis
    bool  edgeSignalSuppressEnabled = true;
    int   edgeSignalSuppressEnter = 2010;
    int   edgeSignalSuppressExit = 1828;

private:
    uint16_t m_prevPressure = 0;
    int      m_tailCounter = 0;
    bool     m_signalSuppressActive = false;
    bool     m_edgeSignalSuppressActive = false;

    struct BtPressureSample {
        uint64_t timestamp_ms;
        uint16_t pressure;
    };

    static constexpr uint64_t kBtAggregationWindowMs = 50;
    static constexpr uint64_t kBtHistoryKeepMs = 60;
    static constexpr size_t kBtHistoryCapacity = 64;
    static constexpr uint64_t kBtPressureMask = 0xFFFFu;
    static constexpr uint64_t kBtTimestampMask = (uint64_t{1} << 48) - 1;
    static constexpr unsigned kBtTimestampShift = 16;

    std::array<std::atomic<uint64_t>, kBtHistoryCapacity> m_btHistory{};
    std::atomic<uint64_t> m_btWriteSeq{0};

    static inline int EvaluatePolynomial(const std::array<double, 5>& c, int x) {
        const double d = static_cast<double>(x);
        const double result =
            (((c[4] * d + c[3]) * d + c[2]) * d + c[1]) * d + c[0];
        return static_cast<int>(result);
    }

    static inline uint64_t PackBtSample(uint64_t timestamp_ms, uint16_t pressure) {
        return ((timestamp_ms & kBtTimestampMask) << kBtTimestampShift) |
               static_cast<uint64_t>(pressure);
    }

    static inline BtPressureSample UnpackBtSample(uint64_t packed) {
        BtPressureSample sample{};
        sample.timestamp_ms = (packed >> kBtTimestampShift) & kBtTimestampMask;
        sample.pressure = static_cast<uint16_t>(packed & kBtPressureMask);
        return sample;
    }
};

} // namespace Asa
