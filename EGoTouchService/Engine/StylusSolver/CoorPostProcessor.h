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
/// The TSACore post-processing order is:
///   1. LinearFilter         (external, in StylusPipeline)
///   2. PushHistory          (GetRealTimeCoor2Buf)
///   3. 3PointAvg            (Get3PointAvgFilter)
///   4. CoorRevise           (external, in StylusPipeline)
///   5. CalcSpeed            (GetCoorSpeed)
///   6. CalcIIRCoef          (GetIIRCoef)
///   7. IIR filter           (CoorFilterProcess)
///   8. Jitter offset        (AftCoorProcess)
///
/// Steps are exposed individually so StylusPipeline can interleave
/// external stages (LinearFilter, CoorRevise) in the correct order.
class CoorPostProcessor {
public:
    /// Reset all state (on pen-up or fresh start)
    void Reset();

    /// Get last computed speed metrics
    const SpeedMetrics& GetSpeed() const { return m_speed; }
    float GetLastIIRCoef() const { return m_lastIirCoef; }

    // ══════════════════════════════════════════════
    // Individual pipeline steps (called by StylusPipeline)
    // ══════════════════════════════════════════════

    /// Step 1: Push coordinate into 24-frame ring buffer.
    /// Must be called before CalcSpeed.
    void StepPushHistory(const AsaCoorResult& cur);

    /// Step 2: 3-point moving average (Get3PointAvgFilter).
    /// Uses prev[0], prev[1], cur. Returns averaged coordinate.
    AsaCoorResult Step3PointAvg(const AsaCoorResult& cur);

    /// Step 3: Calculate speed from ring buffer (GetCoorSpeed).
    /// Updates internal m_speed.
    void StepCalcSpeed();

    /// Step 4: Calculate dynamic IIR coefficient (GetIIRCoef).
    /// Uses m_speed.instant, hover/edge flags.
    /// Returns the coefficient and stores it internally.
    float StepCalcIIRCoef(bool isHover, bool isEdge);

    /// Step 5: Apply IIR filter (CoorFilterProcess).
    AsaCoorResult StepIIR(const AsaCoorResult& cur, float coef);

    /// Step 6: Apply jitter offset compensation (AftCoorProcess).
    AsaCoorResult StepJitter(const AsaCoorResult& cur, bool isEdge);

    /// Update 3-point history after the full chain completes.
    void StepUpdate3PtHistory(const AsaCoorResult& result);

    // ══════════════════════════════════════════════
    // Configuration — IIR coefficients (speed-driven interpolation)
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

    int32_t jitterCenterThreshold = 20;  // center region dead zone
    int32_t jitterEdgeThreshold   = 40;  // edge region dead zone (wider)

    // 3-point average filter
    bool enable3PointAvg = true;

private:
    // ── Internal state ──
    bool     m_initialized = false;
    int      m_frameCount  = 0;

    // ── History ring buffer (24 frames) ──
    static constexpr int kHistoryLen = 24;
    std::array<AsaCoorResult, kHistoryLen> m_history{};
    int m_validHistory = 0;

    // ── 3-point average ──
    AsaCoorResult m_prev[2]{};

    // ── IIR filter state ──
    float m_iirDim1 = 0.0f;
    float m_iirDim2 = 0.0f;

    // ── Jitter offset compensation state ──
    AsaCoorResult m_anchor{};
    int32_t m_offsetDim1 = 0;
    int32_t m_offsetDim2 = 0;
    bool    m_jitterActive = false;

    // ── Speed ──
    SpeedMetrics m_speed{};
    float m_lastIirCoef = 0.0f;
};

} // namespace Asa
