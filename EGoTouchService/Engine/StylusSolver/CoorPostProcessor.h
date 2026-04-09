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
    // TSACore: DAT_1820dc30/dc34 — average per-axis displacement magnitude
    float avgVelDim1 = 0.0f;  // |Σ dim1 displacement| / frames
    float avgVelDim2 = 0.0f;  // |Σ dim2 displacement| / frames
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

    /// Get last IIR coefficient as float (for diagnostic display)
    float GetLastIIRCoef() const {
        return (iirDivisorN > 0)
            ? static_cast<float>(m_lastIirCoefInt) / static_cast<float>(iirDivisorN)
            : 0.0f;
    }

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
    /// Updates internal m_speed and m_motionFrameCount.
    void StepCalcSpeed();

    /// Step 4: Calculate dynamic IIR coefficient (GetIIRCoef).
    /// Mirrors TSACore: uses still/moving state machine + directional override.
    /// @param isInking true when (status & 6) != 0 (Ink or NoPressInk active)
    ///                 → uses Moving coefficients (weaker smoothing)
    ///                 false → uses Still coefficients (stronger smoothing for hover)
    /// Returns the integer coefficient and stores it internally.
    int StepCalcIIRCoef(bool isInking);

    /// Step 5: Apply IIR filter (CoorFilterProcess / CoorIIRFilterType).
    /// Uses integer Q8 fixed-point matching TSACore exactly.
    /// @param coefInt   integer IIR coefficient from StepCalcIIRCoef()
    /// @param skipIIR   true → skip IIR (zero Q8 remainder, passthrough)
    ///                  TSACore skips IIR for first 2 frames after mode transition.
    AsaCoorResult StepIIR(const AsaCoorResult& cur, int coefInt, bool skipIIR = false);

    /// Step 6: Apply jitter offset compensation (AftCoorProcess).
    /// Mirrors TSACore: dynamic threshold based on sensor/screen dimensions,
    /// independent X/Y lock flags.
    AsaCoorResult StepJitter(const AsaCoorResult& cur, bool isEdge);

    /// Update 3-point history after the full chain completes.
    void StepUpdate3PtHistory(const AsaCoorResult& result);

    // ══════════════════════════════════════════════
    // Configuration — IIR coefficients (integer, matching TSACore)
    // g_asaPrmtFlash offsets: [0xa5c..0xa60]
    // ══════════════════════════════════════════════

    // ── Still mode (pen in contact but stationary): [0xa5e]/[0xa5f] ──
    // Stronger smoothing for hover/stationary pen
    int stillIirLow  = 6;   // [0xa5e]: low-speed coefficient
    int stillIirHigh = 18;  // [0xa5f]: high-speed coefficient

    // ── Moving mode (pen actively drawing): [0xa5c]/[0xa5d] ──
    // Weaker smoothing for responsive tracking
    int movingIirLow  = 6;   // [0xa5c]: low-speed coefficient
    int movingIirHigh = 18;  // [0xa5d]: high-speed coefficient

    // ── IIR divisor N: [0xa60] (Gaokun: 32) ──
    // IIR formula: out = (coef * cur + (N - coef) * prev) / N
    int iirDivisorN = 32;

    // ── Speed thresholds ──
    int stillLowSpeedThr  = 20;   // still mode: low speed threshold
    int movingLowSpeedThr = 10;   // moving mode: low speed threshold
    int highSpeedThr      = 204;  // 0xCC: high speed threshold (both modes)

    // ── Motion detection: frames of continuous movement ──
    // Mirrors TSACore (DAT_18231950 & 6): at least 2 frames of pen-valid + moving
    int motionDetectFrames = 2;

    // ── Directional velocity threshold for "edge" override ──
    // TSACore: halve coefficients when direction is consistent (DAT_18230a84/c34 != 0)
    bool enableDirectionalHalve = true;

    // ══════════════════════════════════════════════
    // Configuration — Jitter suppression (AftCoorProcess)
    // g_asaPrmtFlash offsets: [0xa58..0xa5b]
    // ══════════════════════════════════════════════

    // Jitter threshold parameters (multiplied by sensorDim * 0x400 / screenDim)
    // Edge: [0xa58]/[0xa59], Center: [0xa5a]/[0xa5b]
    int jitterEdgeParamDim1   = 3;   // [0xa58]
    int jitterEdgeParamDim2   = 3;   // [0xa59]
    int jitterCenterParamDim1 = 2;   // [0xa5a]
    int jitterCenterParamDim2 = 2;   // [0xa5b]

    // Sensor and screen dimensions (for dynamic threshold calculation)
    int sensorDimDim1 = 60;   // DAT_1820d610: number of sensor columns
    int sensorDimDim2 = 40;   // DAT_1820d611: number of sensor rows
    int screenDimDim1 = 16000; // g_asaPrmt[0x00]: screen logical width
    int screenDimDim2 = 25600; // DAT_1820d602: screen logical height

    // 3-point average filter
    bool enable3PointAvg = true;

    // Jitter suppression (AftCoorProcess)
    bool enableJitter = true;

    // ── IIR skip condition ──
    // TSACore: CoorFilterProcess skips IIR for the first few frames
    // (DAT_18231c28 & 1 == 0 || DAT_1823194c < 2 ... )
    // Approximation: skip until m_frameCount >= iirSkipFrames
    int iirSkipFrames = 2;

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

    // ── IIR filter state (Q8 fixed-point) ──
    // Stores coordinate × 256 + fractional remainder
    // Matches TSACore CoorIIRFilterType exactly
    int32_t m_iirDim1Q8 = 0;
    int32_t m_iirDim2Q8 = 0;

    // ── Jitter offset compensation state ──
    // TSACore: independent X/Y lock flags (flagLockX_7809, flagLockY_7810)
    AsaCoorResult m_anchor{};
    int32_t m_offsetDim1 = 0;
    int32_t m_offsetDim2 = 0;
    bool    m_lockDim1 = false;
    bool    m_lockDim2 = false;
    bool    m_jitterActive = false;

    // ── Speed ──
    SpeedMetrics m_speed{};
    int m_lastIirCoefInt = 0;

    // ── Motion state machine ──
    // Mirrors TSACore (DAT_18231950 & 6): track consecutive moving frames
    int m_motionFrameCount = 0;

    /// Integer IIR filter core: (coef * cur + (N - coef) * prev) / N
    static int32_t IIRFilterQ8(int32_t prevQ8, int32_t curQ8, int coef, int N);
};

} // namespace Asa
