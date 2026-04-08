#pragma once
#include "AsaTypes.h"

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
    AsaCoorResult Revise(const AsaCoorResult& tx1,
                         const AsaCoorResult& tx2,
                         uint16_t curPressure = 0);

    /// Reset internal state (call on pen-up)
    void Reset();

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
