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
        m_btPressCnt.store(0, std::memory_order_relaxed);
        m_btActive.store(false, std::memory_order_relaxed);
        for (auto& sample : m_rawPressureBuf) {
            sample.store(0, std::memory_order_relaxed);
        }
    }

    /// Backward-compatible alias.
    inline void ResetSuppression() {
        Reset();
    }

    /// Inject BT MCU pressure sample with timestamp.
    /// The 4-channel packet is modeled as an ordered raw-pressure sequence so
    /// GetLatestBtPressure() can mimic TSACore GetPressInMapOrder().
    inline void SetBtMcuPressure(uint16_t p) {
        SetBtMcuPressureSequence(p, p, p, p);
    }

    inline void SetBtMcuPressureSequence(uint16_t p0, uint16_t p1,
                                         uint16_t p2, uint16_t p3) {
        auto nowObj = std::chrono::steady_clock::now();
        uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              nowObj.time_since_epoch()).count();
        const uint64_t writeSeq = m_btWriteSeq.load(std::memory_order_relaxed);
        const size_t slot = static_cast<size_t>(writeSeq % kBtHistoryCapacity);
        m_btHistory[slot].store(PackBtSample(now_ms, p3), std::memory_order_relaxed);
        m_btWriteSeq.store(writeSeq + 1, std::memory_order_release);

        m_rawPressureBuf[0].store(p0, std::memory_order_relaxed);
        m_rawPressureBuf[1].store(p1, std::memory_order_relaxed);
        m_rawPressureBuf[2].store(p2, std::memory_order_relaxed);
        m_rawPressureBuf[3].store(p3, std::memory_order_relaxed);
        m_btActive.store((p0 | p1 | p2 | p3) != 0, std::memory_order_relaxed);
        m_btPressCnt.store(0, std::memory_order_relaxed);
    }

    /// Get the latest raw pressure using a TSACore-like selection order.
    inline uint16_t GetLatestBtPressure() {
        constexpr uint8_t kBtPressMapOncell[6] = {0, 1, 1, 2, 3, 3};
        constexpr uint8_t kBtPressMapIncell[4] = {0, 1, 2, 3};

        const uint16_t rawNow = m_rawPressureBuf[3].load(std::memory_order_acquire);
        if (!m_btActive.load(std::memory_order_relaxed)) {
            return rawNow;
        }

        const uint8_t cnt = m_btPressCnt.load(std::memory_order_relaxed);
        uint16_t selected = rawNow;
        if (pressureMapMode == 1) {
            if (cnt < 6) {
                selected = m_rawPressureBuf[kBtPressMapOncell[cnt]].load(std::memory_order_acquire);
            }
        } else if (pressureMapMode == 2) {
            if (cnt < 4) {
                selected = m_rawPressureBuf[kBtPressMapIncell[cnt]].load(std::memory_order_acquire);
            }
        }

        if (cnt < 0xFF) {
            m_btPressCnt.store(static_cast<uint8_t>(cnt + 1), std::memory_order_relaxed);
        }
        return selected;
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

    static constexpr size_t kBtHistoryCapacity = 64;
    static constexpr uint64_t kBtPressureMask = 0xFFFFu;
    static constexpr uint64_t kBtTimestampMask = (uint64_t{1} << 48) - 1;
    static constexpr unsigned kBtTimestampShift = 16;

    std::array<std::atomic<uint64_t>, kBtHistoryCapacity> m_btHistory{};
    std::atomic<uint64_t> m_btWriteSeq{0};
    std::array<std::atomic<uint16_t>, 4> m_rawPressureBuf{};
    std::atomic<uint8_t> m_btPressCnt{0};
    std::atomic<bool> m_btActive{false};

public:
    uint8_t pressureMapMode = 2;

private:

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

public:
    // ══════════════════════════════════════════════
    // HPP3_PostPressureProcess (§5)
    //
    // Merges real pressure + NoPressInk active into final output pressure.
    // Handles:
    //   1. NoPressInk补压: realPress=0 && noPressInkActive → pressure=10 (or prev)
    //   2. Fake pressure decrease tail: prev>500, cur<11 → gradual decay over 1-3 frames
    //   3. Cleanup: pressure=0 → reset fake tail state
    // ══════════════════════════════════════════════

    /// In/out struct for PostPressureProcess.
    struct PostPressureIO {
        uint16_t& curPressure;        // in/out: final output pressure
        bool&     realPressValid;     // in/out: set to true if realPressure > 0

        // Read-only inputs
        bool      noPressInkValid;    // from NoPressInkGate
        uint16_t  prevPressure;       // previous frame's final pressure

        // Fake pressure state (persistent, stored in AsaPenStateTracker)
        uint8_t&  fakePressAdded;     // current fake frame count (counts down)
        uint8_t&  fakePressAddNum;    // total fake frames allocated
    };

    /// §5 line 1943-1968: PostPressureProcess
    inline void PostPressureProcess(PostPressureIO& io) const {
        // §5 line 1943-1948: NoPressInk补压
        if (!io.realPressValid && io.noPressInkValid) {
            if (io.prevPressure != 0) {
                io.curPressure = io.prevPressure;
            } else {
                io.curPressure = static_cast<uint16_t>(
                    std::max(1, postPressNoPressInkMin));
            }
        }

        // §5 line 1953-1963: Fake pressure decrease tail
        if (fakeTailEnabled &&
            io.prevPressure > fakeTailPrevThreshold &&
            io.curPressure < 11) {
            io.curPressure = FakePressureDecrease(
                io.prevPressure, io.fakePressAdded, io.fakePressAddNum);
        }

        // §5 line 1965-1968: cleanup on zero
        if (io.curPressure == 0) {
            io.fakePressAdded = 0;
            io.fakePressAddNum = 0;
        }
    }

    /// §5 line 1870-1940: HPP3_FakePressureDecreaseProcess
    static inline uint16_t FakePressureDecrease(
            uint16_t prevPressure,
            uint8_t& fakePressAdded,
            uint8_t& fakePressAddNum) {
        // First call: allocate fake frames (1-3 based on previous pressure)
        if (fakePressAddNum == 0) {
            if (prevPressure > 3000) {
                fakePressAddNum = 3;
            } else if (prevPressure > 1500) {
                fakePressAddNum = 2;
            } else {
                fakePressAddNum = 1;
            }
            fakePressAdded = fakePressAddNum;
        }

        if (fakePressAdded == 0) {
            return 0;
        }

        // Decay: fake = (added * prevPressure) / (addNum + 1)
        const uint32_t fakeVal =
            (static_cast<uint32_t>(fakePressAdded) *
             static_cast<uint32_t>(prevPressure)) /
            (static_cast<uint32_t>(fakePressAddNum) + 1u);

        fakePressAdded--;
        return static_cast<uint16_t>(std::clamp(
            static_cast<int>(fakeVal), 0, 0xFFFF));
    }

    // PostPressureProcess configuration
    int      postPressNoPressInkMin = 10;   // g_asaPrmtStylus + 0x240 related
    bool     fakeTailEnabled = true;        // Enable fake pressure decrease tail
    uint16_t fakeTailPrevThreshold = 500;   // Only trigger tail if prev > this
};

} // namespace Asa

