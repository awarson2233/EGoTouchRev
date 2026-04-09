#pragma once
#include "AsaTypes.hpp"
#include <cmath>
#include <algorithm>

namespace Asa {

/// CoorReviser — TX2 dual-frequency coordinate revision.
///
/// Mirrors TSACore CoorReviseProcess / CoorReviseCalculation / CoorReviseWork.
///
/// Principle: TX1 and TX2 use different driving frequencies. In theory
/// they should produce identical coordinates. The difference between them
/// originates from sensor non-ideality (crosstalk, EMI). This module:
///   1. CoorReviseCalculation: Computes TX1–TX2 coordinate delta
///   2. CoorReviseWork: Blends the delta into the TX1 output
///
/// This effectively doubles the sampling information and reduces
/// frequency-dependent systematic errors.
class CoorReviser {
public:
    /// Revise TX1 coordinates using TX2 as reference.
    /// @param tx1 TX1 interpolated coordinate (primary)
    /// @param tx2 TX2 interpolated coordinate (reference)
    /// @param curPressure current pressure (for lift-reset detection)
    /// @return Revised coordinate (improved TX1)
    inline AsaCoorResult Revise(const AsaCoorResult& tx1,
                                const AsaCoorResult& tx2,
                                uint16_t curPressure = 0) {
        // Default: pass through TX1 unchanged
        AsaCoorResult out = tx1;

        if (!enabled) return out;
        if (!tx1.valid) { Reset(); return out; }

        // P2: Pressure-based reset (TSACore: prevPress!=0 && curPress==0 → CoorReviseInit)
        if (m_prevPressure != 0 && curPressure == 0) {
            Reset();
        }
        m_prevPressure = curPressure;

        if (!tx2.valid) return out;  // TX2 unavailable → use TX1 as-is

        // ── Step 1: CoorReviseCalculation ──
        // Compute raw TX1-TX2 delta
        float rawDeltaDim1 = static_cast<float>(tx2.dim1 - tx1.dim1);
        float rawDeltaDim2 = static_cast<float>(tx2.dim2 - tx1.dim2);

        // Safety: if delta is too large, TX2 is unreliable
        if (std::abs(rawDeltaDim1) > maxDeltaThreshold ||
            std::abs(rawDeltaDim2) > maxDeltaThreshold) {
            return out;  // discard TX2 for this frame
        }

        // ── Step 2: IIR smooth the delta ──
        if (!m_initialized) {
            m_iirDeltaDim1 = rawDeltaDim1;
            m_iirDeltaDim2 = rawDeltaDim2;
            m_initialized = true;
        } else {
            const float a = std::clamp(deltaIirAlpha, 0.01f, 1.0f);
            m_iirDeltaDim1 = m_iirDeltaDim1 * (1.0f - a) + rawDeltaDim1 * a;
            m_iirDeltaDim2 = m_iirDeltaDim2 * (1.0f - a) + rawDeltaDim2 * a;
        }

        // ── Step 3: CoorReviseWork ──
        // Blend: revised = tx1 + alpha * smoothed_delta
        const float alpha = std::clamp(blendAlpha, 0.0f, 1.0f);
        int32_t revisedDim1 = tx1.dim1 + static_cast<int32_t>(
            std::lround(alpha * m_iirDeltaDim1));
        int32_t revisedDim2 = tx1.dim2 + static_cast<int32_t>(
            std::lround(alpha * m_iirDeltaDim2));

        // P2: Clamp to valid sensor range (TSACore CoorReviseWork bounds check)
        if (revisedDim1 < 0) revisedDim1 = 0;
        if (revisedDim2 < 0) revisedDim2 = 0;

        out.dim1 = revisedDim1;
        out.dim2 = revisedDim2;
        m_prevValid = true;
        return out;
    }

    /// Reset internal state (call on pen-up)
    inline void Reset() {
        m_initialized = false;
        m_iirDeltaDim1 = 0.0f;
        m_iirDeltaDim2 = 0.0f;
        m_prevValid = false;
    }

    // ── Configuration ──

    /// Enable/disable the reviser
    bool enabled = false;

    /// Blending weight for TX2 correction.
    /// revisedCoor = tx1 + alpha * (tx2 - tx1)
    /// alpha=0 → pure TX1, alpha=0.5 → average, alpha=1 → pure TX2
    float blendAlpha = 0.3f;

    /// Maximum allowable TX1-TX2 delta (in kCoorUnit).
    /// If delta exceeds this, TX2 is considered unreliable and ignored.
    float maxDeltaThreshold = 512.0f;

    /// IIR smoothing for the delta to reject transient TX2 glitches.
    float deltaIirAlpha = 0.25f;

    /// Diagnostic accessors for IIR delta
    float GetLastDeltaX() const { return m_iirDeltaDim1; }
    float GetLastDeltaY() const { return m_iirDeltaDim2; }

private:
    bool  m_initialized = false;
    float m_iirDeltaDim1 = 0.0f;
    float m_iirDeltaDim2 = 0.0f;
    bool  m_prevValid = false;
    // P2: Pressure-based reset (TSACore: prevPress!=0 && curPress==0 -> CoorReviseInit)
    uint16_t m_prevPressure = 0;
};

} // namespace Asa
