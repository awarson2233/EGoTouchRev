#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <deque>
#include <mutex>

namespace Asa {

/// PressureSolver — BT MCU pressure mapping, IIR, tail decay, and signal suppression.
///
/// Mirrors TSACore pressure chain: polynomial mapping → IIR → tail decay.
/// Also implements HPP3_SuppressBtPressBySignal hysteresis.
class PressureSolver {
public:
    /// Solve pressure for current frame.
    /// @param rawPressure  Raw BT MCU pressure value
    /// @param active       Whether pen is in valid contact
    /// @param signalStrength  Peak signal magnitude (for signal suppression gate)
    /// @param isEdge       Whether pen is in edge region (bypasses suppression)
    /// @return Mapped + filtered pressure (0..4095)
    inline uint16_t Solve(uint16_t rawPressure, bool active,
                          int signalStrength = 0, bool isEdge = false) {
        if (!active) {
            m_prevPressure = 0;
            m_tailCounter = 0;
            return 0;
        }

        // Signal suppression with hysteresis (TSACore HPP3_SuppressBtPressBySignal)
        if (signalSuppressEnabled && signalStrength > 0) {
            if (!m_signalSuppressActive) {
                if (signalStrength < signalSuppressEnter && !isEdge)
                    m_signalSuppressActive = true;
            } else {
                if (signalStrength > signalSuppressExit)
                    m_signalSuppressActive = false;
            }
            if (m_signalSuppressActive) {
                m_prevPressure = 0;
                return 0;
            }
        }

        // Polynomial mapping
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

        // IIR — Q8 (÷256) to match TSACore
        if (mapped > 0 && m_prevPressure > 0) {
            const int w = std::clamp(iirWeightQ8, 1, 255);
            mapped = ((static_cast<int>(m_prevPressure) *
                       (256 - w)) + mapped * w + 128) >> 8;
            mapped = std::clamp(mapped, 0, 0x0FFF);
        }

        // Tail decay
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

        m_prevPressure = static_cast<uint16_t>(mapped);
        return m_prevPressure;
    }

    /// Reset suppression state (on pen-up)
    inline void ResetSuppression() {
        m_signalSuppressActive = false;
    }

    /// Inject BT MCU pressure sample with timestamp
    inline void SetBtMcuPressure(uint16_t p) {
        auto nowObj = std::chrono::steady_clock::now();
        uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              nowObj.time_since_epoch()).count();
        std::lock_guard<std::mutex> lock(m_btMutex);
        m_btHistory.push_back({now_ms, p});
        if (m_btHistory.size() > 20)
            m_btHistory.pop_front();
    }

    /// Get the most recent valid pressure sample from BT MCU history
    inline uint16_t GetLatestBtPressure() {
        uint16_t btPress = 0;
        auto nowObj = std::chrono::steady_clock::now();
        uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              nowObj.time_since_epoch()).count();
        std::lock_guard<std::mutex> lock(m_btMutex);
        while (!m_btHistory.empty() &&
               now_ms > m_btHistory.front().timestamp_ms + 100) {
            m_btHistory.pop_front();
        }
        for (auto it = m_btHistory.rbegin(); it != m_btHistory.rend(); ++it) {
            if (now_ms <= it->timestamp_ms + 50) {
                if (it->pressure > btPress) btPress = it->pressure;
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
    bool  signalSuppressEnabled = false;
    int   signalSuppressEnter = 200;
    int   signalSuppressExit = 300;

private:
    uint16_t m_prevPressure = 0;
    int      m_tailCounter = 0;
    bool     m_signalSuppressActive = false;

    struct BtPressureSample {
        uint64_t timestamp_ms;
        uint16_t pressure;
    };
    mutable std::mutex m_btMutex;
    std::deque<BtPressureSample> m_btHistory;
};

} // namespace Asa
