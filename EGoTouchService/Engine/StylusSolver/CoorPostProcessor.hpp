#pragma once
#include "AsaTypes.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace Asa {

/// Speed metrics (computed from 24-frame history)
struct SpeedMetrics {
    float instant    = 0.0f;  // frame-to-frame displacement
    float shortAvg   = 0.0f;  // 3-frame average speed
    float fullAvg    = 0.0f;  // 24-frame average speed
    float avgVelDim1 = 0.0f;  // directional velocity dim1
    float avgVelDim2 = 0.0f;  // directional velocity dim2
};

/// CoorPostProcessor — Multi-stage coordinate post-processing chain.
///
/// Mirrors TSACore ASA_CoorPostProcess exactly. Provides:
///   1. PushHistory: 24-frame FIFO ring buffer
///   2. 3-Point Average: Simple moving average (3 frames)
///   3. Speed Calculation: Path-accumulated velocity with direction
///   4. IIR Coefficient: Dynamic still/moving coefficient selection
///   5. IIR Filter: Q8 fixed-point IIR (CoorIIRFilterType)
///   6. Jitter: Dead-zone lock with dynamic threshold (AftCoorProcess)
class CoorPostProcessor {
public:
    /// Reset all internal state
    inline void Reset() {
        m_initialized = false;
        m_frameCount = 0;
        m_validHistory = 0;
        m_history.fill(AsaCoorResult{});
        m_prev[0] = m_prev[1] = AsaCoorResult{};
        m_iirDim1Q8 = m_iirDim2Q8 = 0;
        m_anchor = AsaCoorResult{};
        m_offsetDim1 = m_offsetDim2 = 0;
        m_lockDim1 = m_lockDim2 = false;
        m_jitterActive = false;
        m_speed = SpeedMetrics{};
        m_lastIirCoefInt = 0;
        m_motionFrameCount = 0;
    }

    // ══════════════════════════════════════════════
    // Step 1: PushHistory (TSACore: GetRealTimeCoor2Buf)
    // FIFO shift (index 0 = newest)
    // ══════════════════════════════════════════════
    inline void StepPushHistory(const AsaCoorResult& cur) {
        for (int i = kHistoryLen - 1; i > 0; --i) {
            m_history[static_cast<size_t>(i)] =
                m_history[static_cast<size_t>(i - 1)];
        }
        m_history[0] = cur;
        m_validHistory = std::min(m_validHistory + 1, kHistoryLen);
        m_frameCount++;
    }

    // ══════════════════════════════════════════════
    // Step 2: 3-point average (TSACore: Get3PointAvgFilter)
    // ══════════════════════════════════════════════
    inline AsaCoorResult Step3PointAvg(const AsaCoorResult& cur) {
        if (!enable3PointAvg) return cur;
        if (!m_prev[0].valid || !m_prev[1].valid) return cur;
        AsaCoorResult out = cur;
        out.dim1 = (m_prev[1].dim1 + m_prev[0].dim1 + cur.dim1) / 3;
        out.dim2 = (m_prev[1].dim2 + m_prev[0].dim2 + cur.dim2) / 3;
        return out;
    }

    // ══════════════════════════════════════════════
    // Step 3: Speed calculation (TSACore: GetCoorSpeed)
    // 24-frame path-accumulated speed with direction extraction
    // ══════════════════════════════════════════════
    inline void StepCalcSpeed() {
        SpeedMetrics out{};
        if (m_validHistory < 2) { m_speed = out; return; }

        const int segments = m_validHistory - 1;
        float accumPath = 0.0f;
        // TSACore: accumulate total displacement per axis for directional detection
        int32_t totalDisplaceDim1 = 0;
        int32_t totalDisplaceDim2 = 0;

        for (int i = 0; i < segments; ++i) {
            const auto& a = m_history[static_cast<size_t>(i)];
            const auto& b = m_history[static_cast<size_t>(i + 1)];
            if (!a.valid || !b.valid) continue;

            const float dx = static_cast<float>(a.dim1 - b.dim1);
            const float dy = static_cast<float>(a.dim2 - b.dim2);
            const float segDist = std::sqrt(dx * dx + dy * dy);
            accumPath += segDist;

            if (i == 0) out.instant = segDist;
            if (i == 2) out.shortAvg = accumPath / 3.0f;

            totalDisplaceDim1 += (a.dim1 - b.dim1);
            totalDisplaceDim2 += (a.dim2 - b.dim2);
        }

        out.fullAvg = accumPath / static_cast<float>(segments);
        if (segments < 3) out.shortAvg = out.fullAvg;

        if (segments > 0) {
            out.avgVelDim1 = static_cast<float>(std::abs(totalDisplaceDim1)) /
                             static_cast<float>(segments);
            out.avgVelDim2 = static_cast<float>(std::abs(totalDisplaceDim2)) /
                             static_cast<float>(segments);
        }

        // Motion state: update consecutive moving frame count
        if (out.instant > 1.0f) {
            m_motionFrameCount = std::min(m_motionFrameCount + 1, 100);
        } else {
            m_motionFrameCount = 0;
        }

        m_speed = out;
    }

    // ══════════════════════════════════════════════
    // Step 4: Dynamic IIR coefficient (TSACore: GetIIRCoef)
    // Uses still/moving state machine + directional override
    // Returns INTEGER coefficient (not normalized)
    // ══════════════════════════════════════════════
    inline int StepCalcIIRCoef(bool isInking) {
        int lo, hi, lowThr;

        if (!isInking) {
            // Still mode: stronger smoothing ([0xa5e]/[0xa5f])
            lo = stillIirLow;
            hi = stillIirHigh;
            lowThr = stillLowSpeedThr;
        } else {
            // Moving mode: weaker smoothing ([0xa5c]/[0xa5d])
            lo = movingIirLow;
            hi = movingIirHigh;
            lowThr = movingLowSpeedThr;
        }

        // TSACore: Directional velocity override
        if (enableDirectionalHalve &&
            (m_speed.avgVelDim1 > 1.0f || m_speed.avgVelDim2 > 1.0f)) {
            hi = stillIirHigh >> 1;
            lo = stillIirLow >> 1;
        }

        // TSACore: Integer speed-to-coefficient interpolation
        const int speed = static_cast<int>(m_speed.instant);
        int coef;

        if (speed >= highSpeedThr) {
            coef = hi;
        } else if (speed < lowThr) {
            coef = lo;
        } else if (lowThr == highSpeedThr) {
            coef = (lo + hi) / 2;
        } else {
            coef = lo + static_cast<int>(
                (static_cast<int64_t>(hi - lo) * (speed - lowThr)) /
                (highSpeedThr - lowThr));
        }

        m_lastIirCoefInt = coef;
        return coef;
    }

    // ══════════════════════════════════════════════
    // Step 5: IIR filter (TSACore: CoorFilterProcess + CoorIIRFilterType)
    // Q8 fixed-point: coordinate × 256 + fractional remainder
    // Remainder is preserved across frames for sub-LSB precision
    // ══════════════════════════════════════════════
    inline AsaCoorResult StepIIR(const AsaCoorResult& cur,
                                  int coefInt, bool skipIIR) {
        // TSACore CoorFilterProcess: skip IIR when mode just transitioned
        if (skipIIR) {
            m_iirDim1Q8 = cur.dim1 << 8;
            m_iirDim2Q8 = cur.dim2 << 8;
            m_initialized = true;
            return cur;
        }

        // TSACore: Skip IIR for first few frames (let coordinates settle)
        if (m_frameCount < iirSkipFrames) {
            m_iirDim1Q8 = cur.dim1 << 8;
            m_iirDim2Q8 = cur.dim2 << 8;
            m_initialized = true;
            return cur;
        }

        if (!m_initialized) {
            m_iirDim1Q8 = cur.dim1 << 8;
            m_iirDim2Q8 = cur.dim2 << 8;
            m_initialized = true;
            return cur;
        }

        const int N = std::max(1, iirDivisorN);

        m_iirDim1Q8 = IIRFilterQ8(m_iirDim1Q8, cur.dim1 << 8, coefInt, N);
        m_iirDim2Q8 = IIRFilterQ8(m_iirDim2Q8, cur.dim2 << 8, coefInt, N);

        AsaCoorResult out = cur;
        out.dim1 = m_iirDim1Q8 >> 8;  // Integer part (truncate toward zero)
        out.dim2 = m_iirDim2Q8 >> 8;
        return out;
    }

    // ══════════════════════════════════════════════
    // Step 6: Jitter offset compensation (TSACore: AftCoorProcess)
    // Dynamic threshold based on sensor/screen dimensions
    // Independent X/Y axis locking
    // ══════════════════════════════════════════════
    inline AsaCoorResult StepJitter(const AsaCoorResult& cur, bool isEdge) {
        if (!enableJitter) return cur;

        // TSACore AftCoorProcess: Edge detection in LOCAL space
        const bool isLocalEdge = isEdge ||
            cur.dim1 < (kCoorUnit + 1) || cur.dim2 < (kCoorUnit + 1) ||
            cur.dim1 >= (kGridDim - 1) * kCoorUnit ||
            cur.dim2 >= (kGridDim - 1) * kCoorUnit;

        // TSACore: Dynamic threshold = (param * gridDim * 0x400) / screenDim
        int32_t thrDim1, thrDim2;
        if (isLocalEdge) {
            thrDim1 = (screenDimDim1 > 0)
                ? (jitterEdgeParamDim1 * kGridDim * kCoorUnit) / screenDimDim1
                : 40;
            thrDim2 = (screenDimDim2 > 0)
                ? (jitterEdgeParamDim2 * kGridDim * kCoorUnit) / screenDimDim2
                : 40;
        } else {
            thrDim1 = (screenDimDim1 > 0)
                ? (jitterCenterParamDim1 * kGridDim * kCoorUnit) / screenDimDim1
                : 20;
            thrDim2 = (screenDimDim2 > 0)
                ? (jitterCenterParamDim2 * kGridDim * kCoorUnit) / screenDimDim2
                : 20;
        }

        // TSACore: Lock starts on first valid contact frame
        if (!m_jitterActive) {
            m_anchor = cur;
            m_offsetDim1 = 0;
            m_offsetDim2 = 0;
            m_lockDim1 = true;
            m_lockDim2 = true;
            m_jitterActive = true;
            return cur;
        }

        // TSACore: Independent X/Y axis lock check
        if (m_lockDim1) {
            int32_t dx = cur.dim1 - m_anchor.dim1;
            if (std::abs(dx) > thrDim1) {
                m_lockDim1 = false;
            }
            if (m_lockDim1) {
                m_offsetDim1 = cur.dim1 - m_anchor.dim1;
            }
        }

        if (m_lockDim2) {
            int32_t dy = cur.dim2 - m_anchor.dim2;
            if (std::abs(dy) > thrDim2) {
                m_lockDim2 = false;
            }
            if (m_lockDim2) {
                m_offsetDim2 = cur.dim2 - m_anchor.dim2;
            }
        }

        // TSACore: output = coordinate - accumulated offset, clamped
        AsaCoorResult out = cur;
        int32_t resultDim1 = cur.dim1 - m_offsetDim1;
        int32_t resultDim2 = cur.dim2 - m_offsetDim2;

        const int32_t maxDim1 = kGridDim * kCoorUnit;
        const int32_t maxDim2 = kGridDim * kCoorUnit;
        out.dim1 = std::clamp(resultDim1, 0, maxDim1);
        out.dim2 = std::clamp(resultDim2, 0, maxDim2);
        return out;
    }

    // ══════════════════════════════════════════════
    // Update 3-point history (after full chain)
    // ══════════════════════════════════════════════
    inline void StepUpdate3PtHistory(const AsaCoorResult& result) {
        m_prev[1] = m_prev[0];
        m_prev[0] = result;
    }

    // ── Diagnostic accessors ──
    const SpeedMetrics& GetSpeed() const { return m_speed; }
    int GetLastIIRCoef() const { return m_lastIirCoefInt; }
    int GetMotionFrameCount() const { return m_motionFrameCount; }
    int GetFrameCount() const { return m_frameCount; }

    // ══════════════════════════════════════════════
    // Configuration parameters
    // ══════════════════════════════════════════════

    // 3-point average (TSACore: DAT_1820d620 & 2)
    bool enable3PointAvg = false;

    // Jitter suppression (TSACore: AftCoorProcess)
    bool enableJitter = true;
    int  jitterEdgeParamDim1   = 3;   // TSACore: flash[0xa66]
    int  jitterEdgeParamDim2   = 3;   // TSACore: flash[0xa67]
    int  jitterCenterParamDim1 = 2;   // TSACore: flash[0xa64]
    int  jitterCenterParamDim2 = 2;   // TSACore: flash[0xa65]
    int  screenDimDim1 = 16000;       // HID X resolution
    int  screenDimDim2 = 25600;       // HID Y resolution

    // IIR Q8 coefficients (TSACore: GetIIRCoef / CoorIIRFilterType)
    int  stillIirLow    = 4;     // flash[0xa5e]: still mode low-speed coef
    int  stillIirHigh   = 16;    // flash[0xa5f]: still mode high-speed coef
    int  movingIirLow   = 8;     // flash[0xa5c]: moving mode low-speed coef
    int  movingIirHigh  = 16;    // flash[0xa5d]: moving mode high-speed coef
    int  iirDivisorN    = 16;    // flash[0xa5a]: IIR divisor N
    int  highSpeedThr   = 300;   // high-speed threshold (both modes)
    int  stillLowSpeedThr  = 3;  // still mode low-speed threshold
    int  movingLowSpeedThr = 3;  // moving mode low-speed threshold
    bool enableDirectionalHalve = true; // directional velocity halve

    // Motion detection (for IIR mode selection)
    int  motionDetectFrames = 3; // frames of continuous motion to enter "moving"
    int  iirSkipFrames = 2;     // skip IIR for first N frames after reset

private:
    static constexpr int kHistoryLen = 24;

    bool m_initialized = false;
    int  m_frameCount = 0;
    int  m_validHistory = 0;

    std::array<AsaCoorResult, kHistoryLen> m_history{};
    AsaCoorResult m_prev[2]{};  // for 3-point average

    // IIR Q8 state (preserved across frames for sub-LSB precision)
    int32_t m_iirDim1Q8 = 0;
    int32_t m_iirDim2Q8 = 0;

    // Jitter state
    AsaCoorResult m_anchor{};
    int32_t m_offsetDim1 = 0;
    int32_t m_offsetDim2 = 0;
    bool    m_lockDim1 = false;
    bool    m_lockDim2 = false;
    bool    m_jitterActive = false;

    SpeedMetrics m_speed{};
    int  m_lastIirCoefInt = 0;
    int  m_motionFrameCount = 0;

    /// Integer IIR core: (coef * cur + (N - coef) * prev) / N
    /// Matches TSACore CoorIIRFilter exactly
    static inline int32_t IIRFilterQ8(
            int32_t prevQ8, int32_t curQ8, int coef, int N) {
        return (coef * curQ8 + (N - coef) * prevQ8) / N;
    }
};

} // namespace Asa
