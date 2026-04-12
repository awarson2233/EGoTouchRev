#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Asa {

/// MotionProfile — per-frame parameter set output by PenStateMachine.
/// Drives IIR strength, jitter, LinearFilter enable, etc.
struct MotionProfile {
    // IIR control
    int   iirCoef       = 8;      // Numerator of IIR weight (coef/divisorN)
    int   iirDivisorN   = 16;     // IIR denominator
    bool  skipIIR       = false;  // true → bypass IIR this frame (direct pass)
    bool  freezeOutput  = false;  // true → output last known-good coordinate

    // Jitter control (0=off, 1=weak, 2=medium, 3=strong)
    int   jitterStrength = 2;

    // Module enables
    bool  enableLinearFilter = false;
    bool  enableCoorReviser  = true;

    // Pressure control
    float pressureAlpha  = 0.25f;  // Pressure IIR weight (0=frozen, 1=direct)
    bool  pressureDecay  = false;  // true → tail decay mode

    // Flags (informational, for app layer)
    bool  isLongPress    = false;
    bool  isTap          = false;
};

/// PenStateMachine v2.1 — 4-state pen lifecycle with speed-continuous MotionProfile.
///
/// States: Leave → Hover → Moving → Lifting → Leave
///
/// v2.1 fixes:
///   - Speed uses short-window EMA (3-frame) to suppress noise → stable IIR coef
///   - Directional halve: when moving predominantly on one axis, IIR halved
///   - movingIirHigh defaults to 16 (full pass-through at high speed, same as old TSACore)
class PenStateMachine {
public:
    enum class State : uint8_t {
        Leave   = 0,
        Hover   = 1,
        Moving  = 2,
        Lifting = 3,
    };

    /// Update state machine for current frame.
    /// Call AFTER coordinate solve and pressure solve.
    /// @param coordValid   Whether coordinate solve succeeded
    /// @param pressure     Current frame pressure (0 = no pressure)
    /// @param curDim1      Current GLOBAL coordinate dim1
    /// @param curDim2      Current GLOBAL coordinate dim2
    /// @return MotionProfile for this frame's post-processing
    inline MotionProfile Update(bool coordValid, uint16_t pressure,
                                 int32_t curDim1, int32_t curDim2) {
        // ── Speed Calculation ──
        float rawSpeedThisFrame = 0.0f;
        float rawVelDim1 = 0.0f;
        float rawVelDim2 = 0.0f;

        if (coordValid && m_hasPrevCoor) {
            float dx = static_cast<float>(curDim1 - m_prevDim1);
            float dy = static_cast<float>(curDim2 - m_prevDim2);
            rawSpeedThisFrame = std::sqrt(dx * dx + dy * dy);
            rawVelDim1 = std::abs(dx);
            rawVelDim2 = std::abs(dy);
        }

        // EMA smoothing on speed (α = 2/(N+1), N=speedSmoothWindow)
        // This prevents single-frame noise spikes from whipsawing IIR
        if (m_hasPrevCoor && coordValid) {
            const float alpha = 2.0f / (static_cast<float>(speedSmoothWindow) + 1.0f);
            m_smoothedSpeed = alpha * rawSpeedThisFrame + (1.0f - alpha) * m_smoothedSpeed;
            m_smoothedVelDim1 = alpha * rawVelDim1 + (1.0f - alpha) * m_smoothedVelDim1;
            m_smoothedVelDim2 = alpha * rawVelDim2 + (1.0f - alpha) * m_smoothedVelDim2;
        } else {
            m_smoothedSpeed = rawSpeedThisFrame;
            m_smoothedVelDim1 = rawVelDim1;
            m_smoothedVelDim2 = rawVelDim2;
        }

        // Expose raw for diagnostics
        m_instantSpeed = rawSpeedThisFrame;

        // Save for next frame
        if (coordValid) {
            m_prevDim1 = curDim1;
            m_prevDim2 = curDim2;
            m_hasPrevCoor = true;
        }

        // State transitions
        State prevState = m_state;
        m_stateFrameCount++;
        m_totalContactFrames = (pressure > 0) ?
            m_totalContactFrames + 1 : m_totalContactFrames;

        switch (m_state) {
        case State::Leave:
            if (coordValid) {
                m_state = (pressure > 0) ? State::Moving : State::Hover;
                m_stateFrameCount = 0;
                m_totalContactFrames = 0;
                m_lowSpeedFrames = 0;
            }
            break;

        case State::Hover:
            if (!coordValid) {
                EnterLeave();
            } else if (pressure > 0) {
                m_state = State::Moving;
                m_stateFrameCount = 0;
                m_totalContactFrames = 1;
                m_lowSpeedFrames = 0;
            }
            break;

        case State::Moving:
            if (!coordValid) {
                EnterLeave();
            } else if (pressure == 0) {
                m_state = State::Lifting;
                m_stateFrameCount = 0;
            }
            break;

        case State::Lifting:
            if (!coordValid || m_stateFrameCount > liftTimeout) {
                // Check for tap before leaving
                m_wasTap = (m_totalContactFrames <= tapMaxFrames);
                EnterLeave();
            } else if (pressure > 0) {
                m_state = State::Moving;
                m_stateFrameCount = 0;
            }
            break;
        }

        // Track low-speed frames for long-press detection (only in Moving)
        if (m_state == State::Moving) {
            if (m_smoothedSpeed < stillSpeedThreshold) {
                m_lowSpeedFrames++;
            } else {
                m_lowSpeedFrames = 0;
            }
        }

        // Check if we just left range (for external reset triggers)
        m_justLeftRange = (prevState != State::Leave && m_state == State::Leave);

        // Build profile
        return BuildProfile();
    }

    // ── Accessors ──

    State GetState() const { return m_state; }
    float GetInstantSpeed() const { return m_instantSpeed; }
    float GetSmoothedSpeed() const { return m_smoothedSpeed; }
    int   GetStateFrameCount() const { return m_stateFrameCount; }
    int   GetTotalContactFrames() const { return m_totalContactFrames; }
    int   GetLowSpeedFrames() const { return m_lowSpeedFrames; }
    bool  JustLeftRange() const { return m_justLeftRange; }
    bool  WasTap() const { return m_wasTap; }

    /// Reset everything (call on catastrophic error)
    inline void Reset() {
        m_state = State::Leave;
        m_stateFrameCount = 0;
        m_totalContactFrames = 0;
        m_lowSpeedFrames = 0;
        m_instantSpeed = 0.0f;
        m_smoothedSpeed = 0.0f;
        m_smoothedVelDim1 = 0.0f;
        m_smoothedVelDim2 = 0.0f;
        m_hasPrevCoor = false;
        m_justLeftRange = false;
        m_wasTap = false;
    }

    // ── Configuration ──

    // Speed thresholds (GLOBAL space, 0x400 units/frame)
    float stillSpeedThreshold = 3.0f;     // Long-press detection
    float speedLow            = 3.0f;     // IIR interpolation low reference
    float speedHigh           = 300.0f;   // IIR interpolation high reference

    // Speed smoothing window (EMA N)
    int   speedSmoothWindow   = 5;        // 5-frame EMA for speed stabilization

    // Frame counts
    int   longPressFrames     = 120;      // @240Hz ≈ 0.5s
    int   liftTimeout         = 10;
    int   tapMaxFrames        = 5;

    // Moving IIR range (divisor = iirDivisorN)
    // -- Matches old TSACore defaults: still=4~16/16, moving=8~16/16
    int   movingIirLow        = 4;        // Very slow → strong smoothing
    int   movingIirHigh       = 16;       // Fast → full pass-through (16/16 = 1.0)
    int   iirDivisorN         = 16;

    // Directional halve — when moving predominantly on one axis,
    // halve the IIR coefficients for stronger cross-axis smoothing
    bool  enableDirectionalHalve = true;
    float directionalVelThreshold = 1.0f; // Min avg velocity to trigger halve

    // Hover IIR
    int   hoverIirCoef        = 4;

    // Jitter maximum strength at low speed
    int   jitterMax           = 3;

private:
    State m_state = State::Leave;
    int   m_stateFrameCount = 0;
    int   m_totalContactFrames = 0;
    int   m_lowSpeedFrames = 0;
    float m_instantSpeed = 0.0f;
    float m_smoothedSpeed = 0.0f;
    float m_smoothedVelDim1 = 0.0f;
    float m_smoothedVelDim2 = 0.0f;

    // Previous frame coordinates for speed calculation
    int32_t m_prevDim1 = 0;
    int32_t m_prevDim2 = 0;
    bool    m_hasPrevCoor = false;

    bool  m_justLeftRange = false;
    bool  m_wasTap = false;

    inline void EnterLeave() {
        m_state = State::Leave;
        m_stateFrameCount = 0;
        m_hasPrevCoor = false;
        m_instantSpeed = 0.0f;
        m_smoothedSpeed = 0.0f;
        m_smoothedVelDim1 = 0.0f;
        m_smoothedVelDim2 = 0.0f;
        m_lowSpeedFrames = 0;
    }

    inline MotionProfile BuildProfile() const {
        MotionProfile p;
        p.iirDivisorN = iirDivisorN;

        switch (m_state) {
        case State::Leave:
            // Everything off / reset
            p.iirCoef = movingIirLow;
            p.skipIIR = true;
            p.jitterStrength = 0;
            p.enableLinearFilter = false;
            p.enableCoorReviser = false;
            p.pressureAlpha = 1.0f;
            p.pressureDecay = false;
            break;

        case State::Hover:
            // Strong smoothing for cursor indicator
            p.iirCoef = hoverIirCoef;
            p.skipIIR = false;
            p.jitterStrength = jitterMax;
            p.enableLinearFilter = false;
            p.enableCoorReviser = true;
            p.pressureAlpha = 1.0f;
            p.pressureDecay = false;
            break;

        case State::Moving:
            CalcMovingProfile(p);
            break;

        case State::Lifting:
            // Freeze output, pressure tail decay
            p.iirCoef = movingIirLow;
            p.skipIIR = false;
            p.freezeOutput = true;
            p.jitterStrength = 0;
            p.enableLinearFilter = false;
            p.enableCoorReviser = false;
            p.pressureAlpha = 0.0f;
            p.pressureDecay = true;
            p.isTap = (m_totalContactFrames <= tapMaxFrames);
            break;
        }

        return p;
    }

    inline void CalcMovingProfile(MotionProfile& p) const {
        // Use SMOOTHED speed for interpolation (prevents single-frame noise spikes)
        float effectiveSpeed = m_smoothedSpeed;

        // ── Directional Halve (TSACore compatibility) ──
        // When pen moves predominantly on one axis, reduce IIR coef range by half
        // → stronger smoothing on the cross-axis to suppress perpendicular jitter
        int effIirLow = movingIirLow;
        int effIirHigh = movingIirHigh;

        if (enableDirectionalHalve &&
            (m_smoothedVelDim1 > directionalVelThreshold ||
             m_smoothedVelDim2 > directionalVelThreshold)) {
            effIirHigh = std::max(1, effIirHigh >> 1);
            effIirLow  = std::max(1, effIirLow >> 1);
        }

        // Continuous speed interpolation
        const float range = speedHigh - speedLow;
        const float t = (range > 0.0f)
            ? std::clamp((effectiveSpeed - speedLow) / range, 0.0f, 1.0f)
            : 0.0f;

        // IIR: lerp [effIirLow .. effIirHigh]
        p.iirCoef = static_cast<int>(
            static_cast<float>(effIirLow) +
            t * static_cast<float>(effIirHigh - effIirLow) + 0.5f);
        p.skipIIR = false;
        p.freezeOutput = false;

        // Jitter: lerp [jitterMax .. 0]
        p.jitterStrength = static_cast<int>(
            static_cast<float>(jitterMax) * (1.0f - t) + 0.5f);

        // LinearFilter always active in Moving
        p.enableLinearFilter = true;

        // CoorReviser always on
        p.enableCoorReviser = true;

        // Pressure IIR: faster response at high speed
        p.pressureAlpha = 0.25f + 0.25f * t;  // [0.25 .. 0.5]
        p.pressureDecay = false;

        // Long-press detection
        p.isLongPress = (m_lowSpeedFrames > longPressFrames);
        p.isTap = false;
    }
};

} // namespace Asa
