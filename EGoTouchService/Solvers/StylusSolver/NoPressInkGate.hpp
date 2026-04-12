#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>

namespace Asa {

struct NoPressInkResult {
    bool active = false;
    uint16_t outputPressure = 0;
    uint16_t enterThreshold = 0;
    uint16_t exitThreshold = 0;
    uint16_t tiltCompensation = 0;
};

/// NoPressInkGate — fixed-parameter NoPressInk replica.
///
/// Mirrors the core TSACore behavior without the learned lookup tables:
/// when real pressure is zero but TX1/TX2 signal remains strong, enter a
/// synthetic inking state after a short debounce and keep reporting a
/// non-zero pressure until the exit threshold holds long enough.
class NoPressInkGate {
public:
    inline NoPressInkResult Apply(bool coordValid,
                                  bool tx1BlockValid,
                                  bool lowSignalSuppressed,
                                  uint16_t realPressure,
                                  uint16_t prevOutputPressure,
                                  uint16_t tx1Composite,
                                  uint16_t tx2Composite) {
        NoPressInkResult out{};

        if (!enabled || !coordValid || !tx1BlockValid || lowSignalSuppressed) {
            Reset();
            out.outputPressure = realPressure;
            return out;
        }

        const uint16_t tiltCapClamped = static_cast<uint16_t>(
            std::clamp(tiltCap, 0, static_cast<int>(std::numeric_limits<uint16_t>::max())));
        const uint16_t tiltDeadzoneClamped = static_cast<uint16_t>(
            std::clamp(tiltDeadzone, 0, static_cast<int>(std::numeric_limits<uint16_t>::max())));
        const uint16_t cappedTx2 = std::min<uint16_t>(tx2Composite, tiltCapClamped);
        int tiltComp = 0;
        if (cappedTx2 > tiltDeadzoneClamped) {
            tiltComp = (static_cast<int>(cappedTx2) - tiltDeadzoneClamped) *
                       std::max(0, tiltScalePercent) / 100;
        }

        const int base = std::max(0, baseThreshold);
        const int enterBase = base + tiltComp;
        const int enter = enterBase * std::max(0, enterRatioPercent) / 100;
        const int exitBase = base + tiltComp;
        const int exitScaled = exitBase * std::max(0, exitRatioPercent) / 100;
        out.enterThreshold = static_cast<uint16_t>(std::clamp(enter, 0, 0xFFFF));
        out.exitThreshold = static_cast<uint16_t>(std::clamp(exitScaled, 0, 0xFFFF));
        out.tiltCompensation = static_cast<uint16_t>(std::clamp(tiltComp, 0, 0xFFFF));

        if (realPressure == 0) {
            if (!m_active) {
                if (static_cast<int>(tx1Composite) >= enter) {
                    ++m_enterStreak;
                } else {
                    m_enterStreak = 0;
                }
                m_exitStreak = 0;
                if (m_enterStreak >= std::max(1, enterDebounceFrames)) {
                    m_active = true;
                    m_exitStreak = 0;
                }
            } else {
                if (static_cast<int>(tx1Composite) <= exitScaled) {
                    ++m_exitStreak;
                } else {
                    m_exitStreak = 0;
                }
                m_enterStreak = 0;
                if (m_exitStreak >= std::max(1, exitDebounceFrames)) {
                    m_active = false;
                    m_enterStreak = 0;
                }
            }
        } else {
            m_enterStreak = 0;
            m_exitStreak = 0;
        }

        out.active = m_active;
        out.outputPressure = realPressure;
        if (realPressure == 0 && m_active) {
            out.outputPressure = (prevOutputPressure > 0)
                ? prevOutputPressure
                : static_cast<uint16_t>(std::clamp(syntheticMinPressure, 0, 0x0FFF));
        }
        return out;
    }

    inline void Reset() {
        m_active = false;
        m_enterStreak = 0;
        m_exitStreak = 0;
    }

    bool enabled = false;
    int  baseThreshold = 10000;
    int  enterRatioPercent = 100;
    int  exitRatioPercent = 30;
    int  tiltDeadzone = 1000;
    int  tiltCap = 10000;
    int  tiltScalePercent = 29;
    int  enterDebounceFrames = 2;
    int  exitDebounceFrames = 2;
    int  syntheticMinPressure = 10;

private:
    bool m_active = false;
    int  m_enterStreak = 0;
    int  m_exitStreak = 0;
};

} // namespace Asa
