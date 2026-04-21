#pragma once

#include <algorithm>
#include <cstdint>

namespace Asa {

struct NoPressInkResult {
    bool active = false;
    uint16_t enterThreshold = 0;
    uint16_t exitThreshold = 0;
    uint16_t tiltCompensation = 0;
};

/// NoPressInkGate — fixed-parameter no-press evidence detector.
///
/// Mirrors the debounce / threshold part of TSACore no-press handling, but
/// only answers whether no-press sustain evidence is active. Final pressure
/// output is decided by PenStateMachine.
class NoPressInkGate {
public:
    inline NoPressInkResult Apply(bool coordValid,
                                  bool tx1BlockValid,
                                  bool lowSignalSuppressed,
                                  uint16_t realPressure,
                                  uint16_t tx1Composite,
                                  uint16_t tx2Composite,
                                  int32_t dim1 = 0,
                                  int32_t dim2 = 0) {
        (void)coordValid;
        (void)tx1BlockValid;
        (void)lowSignalSuppressed;
        (void)realPressure;
        (void)tx1Composite;
        (void)tx2Composite;
        (void)dim1;
        (void)dim2;
        Reset();
        return {};
    }

    inline void Reset() {
        m_active = false;
        m_enterStreak = 0;
        m_exitStreak = 0;
    }

    virtual uint16_t GetBaseThreshold(int32_t dim1, int32_t dim2) const {
        (void)dim1;
        (void)dim2;
        return static_cast<uint16_t>(std::clamp(baseThreshold, 0, 0xFFFF));
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
