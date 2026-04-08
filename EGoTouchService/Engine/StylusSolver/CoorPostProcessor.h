#pragma once
#include "AsaTypes.h"
#include <array>

namespace Asa {

/// Speed metrics computed from the 24-frame history ring buffer.
/// Mirrors TSACore GetCoorSpeed which outputs 3 velocity tiers.
struct SpeedMetrics {
    float instant  = 0.0f;  // 1-frame displacement (瞬时速度)
    float shortAvg = 0.0f;  // 3-frame average path  (短期速度)
    float fullAvg  = 0.0f;  // full-window average path (全窗口平均速度)
};

/// CoorPostProcessor — Multi-stage coordinate post-processing chain.
/// Mirrors ASA_CoorPostProcess from TSACore with full fidelity.
///
/// P1 improvements over original implementation:
///   - Speed: 3-tier path-accumulated speed (instant/short/full)
///   - IIR: hover/write/edge 3-mode coefficient switching
///   - Jitter: offset-compensation method (no unlock jump)
class CoorPostProcessor {
public:
    /// Process coordinates through the full post-processing chain.
    /// @param raw  Raw interpolated coordinate (0x400 units)
    /// @param isHover  true if pen is hovering (no pressure contact)
    /// @param isEdge   true if pen is in the edge region of the panel
    /// @return Post-processed coordinate
    AsaCoorResult Process(const AsaCoorResult& raw,
                          bool isHover = false,
                          bool isEdge  = false);

    /// Reset all state (on pen-up or fresh start)
    void Reset();

    /// Get last computed speed metrics (for external consumers like tilt)
    const SpeedMetrics& GetSpeed() const { return m_speed; }
    float GetLastIIRCoef() const { return m_lastIirCoef; }

    // ══════════════════════════════════════════════
    // Configuration — IIR coefficients (3 modes)
    // ══════════════════════════════════════════════

    // ── Write mode (pen in contact) ──
    // Gaokun flash: low=6, high=18, N=32 → α = coef/N
    float writeIirLow   = 6.0f / 32.0f;    // 0.1875: strong smoothing at low speed
    float writeIirHigh  = 18.0f / 32.0f;   // 0.5625: weak smoothing at high speed
    float writeLowThr   = 20.0f;   // speed threshold: low  (original ~10)
    float writeHighThr  = 204.0f;  // speed threshold: high (original 0xCC=204)

    // ── Hover mode (pen in range, no contact) ──
    // Gaokun flash: low=2, high=16, N=32
    float hoverIirLow   = 2.0f / 32.0f;    // 0.0625: very strong hover smoothing
    float hoverIirHigh  = 16.0f / 32.0f;   // 0.5: moderate at high speed
    float hoverLowThr   = 20.0f;   // speed threshold: low
    float hoverHighThr  = 204.0f;  // speed threshold: high (0xCC)

    // ── Edge mode modifier ──
    // When isEdge=true, IIR coefficients are halved to reduce lag
    // (mirrors TSACore: `if (edgeFlag) { coef >>= 1; }`)
    bool  enableEdgeHalve = true;

    // ══════════════════════════════════════════════
    // Configuration — Jitter suppression (AftCoorProcess)
    // ══════════════════════════════════════════════

    // Offset-compensation jitter suppression.
    // Instead of locking to a fixed point, we accumulate the offset
    // between the "anchor" and the current coordinate. When the pen
    // moves beyond the threshold, the offset is gradually released.
    //
    // This eliminates the "unlock jump" of the simple lock method.
    int32_t jitterCenterThreshold = 20;  // center region dead zone
    int32_t jitterEdgeThreshold   = 40;  // edge region dead zone (wider)

    // 3-point average filter
    bool enable3PointAvg = true;

    // Linear filter state machine (complex, disabled by default)
    bool enableLinearFilter = false;

private:
    // ── Internal state ──
    bool     m_initialized = false;
    int      m_frameCount  = 0;

    // ── History ring buffer (24 frames) ──
    // Index 0 = most recent, mirrors TSACore memmove layout.
    static constexpr int kHistoryLen = 24;
    std::array<AsaCoorResult, kHistoryLen> m_history{};
    int m_validHistory = 0;  // number of valid entries

    // ── 3-point average ──
    AsaCoorResult m_prev[2]{};

    // ── IIR filter state ──
    float m_iirDim1 = 0.0f;
    float m_iirDim2 = 0.0f;

    // ── Jitter offset compensation state (AftCoorProcess) ──
    // "anchor" = the coordinate at which the pen first touches down
    // "offset" = anchor - currentCoord, accumulated while in dead zone
    AsaCoorResult m_anchor{};
    int32_t m_offsetDim1 = 0;
    int32_t m_offsetDim2 = 0;
    bool    m_jitterActive = false;  // true = in dead zone

    // ── Speed ──
    SpeedMetrics m_speed{};
    float m_lastIirCoef = 0.0f;

    // ── Helpers ──
    void PushHistory(const AsaCoorResult& cur);
    SpeedMetrics CalcSpeed();
    float CalcIIRCoef(float speed, bool isHover, bool isEdge);
    AsaCoorResult ApplyIIR(const AsaCoorResult& cur, float coef);
    AsaCoorResult Apply3PointAvg(const AsaCoorResult& cur);
    AsaCoorResult ApplyJitterOffset(const AsaCoorResult& cur, bool isEdge);
};

} // namespace Asa
