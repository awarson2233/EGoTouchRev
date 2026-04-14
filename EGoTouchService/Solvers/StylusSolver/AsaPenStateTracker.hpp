#pragma once
#include <cstdint>

namespace Asa {

/// AsaPenStateTracker — Per-frame pen state tracking.
///
/// Mirrors TSACore's AsaPenState struct: tracks the cross-frame state
/// needed by NoPressInkGate, PostPressureProcess, and EdgeCoorProcess.
///
/// Usage:
///   1. Pipeline writes curPressure / realPressValid / noPressInkValid
///   2. PostProcess() computes curStaticBits from those flags
///   3. FrameEnd() latches prev state for next frame
struct AsaPenStateTracker {
    // ── Current frame pressure state (written by pipeline) ──
    uint16_t curPressure = 0;
    uint16_t prevPressure = 0;
    bool realPressValid = false;
    bool noPressInkValid = false;

    // ── Status bits (§7 simplified) ──
    // bit1 = realPressValid, bit2 = curPressure != 0
    uint8_t curStaticBits = 0;
    uint8_t prevStaticBits = 0;

    // ── Fake pressure decrease state (§5) ──
    uint8_t fakePressAdded = 0;
    uint8_t fakePressAddNum = 0;

    // ── Edge continuation (§6, interface reserved) ──
    bool firstRelease = false;

    /// Compute curStaticBits from current frame flags (§7).
    /// Call after PostPressureProcess has finalized curPressure.
    inline void PostProcess() {
        curStaticBits = 0;
        if (realPressValid) {
            curStaticBits |= 0x02;  // bit1: real pressure valid
        }
        if (curPressure != 0) {
            curStaticBits |= 0x04;  // bit2: has output pressure
        }
    }

    /// Latch current state as previous. Call at end of each frame.
    inline void FrameEnd() {
        prevStaticBits = curStaticBits;
        prevPressure = curPressure;
        // Reset per-frame flags for next frame
        curStaticBits = 0;
        realPressValid = false;
        noPressInkValid = false;
    }

    /// Full reset (pen leave / catastrophic error).
    inline void Reset() {
        curPressure = 0;
        prevPressure = 0;
        realPressValid = false;
        noPressInkValid = false;
        curStaticBits = 0;
        prevStaticBits = 0;
        fakePressAdded = 0;
        fakePressAddNum = 0;
        firstRelease = false;
    }
};

} // namespace Asa
