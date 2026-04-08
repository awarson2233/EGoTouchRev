#pragma once
#include "AsaTypes.h"
#include <array>
#include <cstdint>

namespace Asa {

/// LinearFilter — 7-state line detection and filtering state machine.
///
/// Mirrors TSACore LinearFilterProcess with states:
///   0: Init       → 1: Wait
///   1: Wait       → 2: Collect
///   2: Collect    → 3: CurveLine
///   3: CurveLine  → 4: EnterStraight (if line detected)
///   4: EnterStraightLine  → 5: StraightLine
///   5: StraightLine       → 6: ExitStraight (if deviation detected)
///   6: ExitStraightLine   → 3: CurveLine
///
/// P2: Refactored to match TSACore:
///   - Init→Wait→Collect are single-frame transitions (no bufCount gate)
///   - Two LineFit sets: global (all buffered points) + current (includes cur point)
///   - Aging weight: effective N = (kMaxBufLen - bufCount) for normalization
///   - BufStraightPaintPoint / BufShortDistancePoint tracking
class LinearFilter {
public:
    /// Process one coordinate frame through the state machine.
    AsaCoorResult Process(const AsaCoorResult& coor, uint16_t pressure);

    /// Reset state machine to Init
    void Reset();

    /// Get current state machine state (for monitoring)
    int GetState() const;

    /// Enable/disable the linear filter
    bool enabled = false;

    // ── Configuration ──
    /// Minimum buffer length before attempting line fit in CurveLine state
    int minFitLength = 20;

    /// Maximum residual (mean squared distance) to enter straight mode
    float enterResidualThreshold = 50.0f;

    /// Exit straight mode if max deviation exceeds this
    float exitDeviation = 200.0f;

    /// Perpendicular constraint strength (0–1).
    /// 1.0 = fully lock perp direction, 0.0 = no filtering
    float perpConstraint = 0.8f;

private:
    enum class State : int {
        Init = 0,
        Wait = 1,
        Collect = 2,
        CurveLine = 3,
        EnterStraight = 4,
        StraightLine = 5,
        ExitStraight = 6,
    };

    State m_state = State::Init;

    // ── Rolling buffer for line fitting (TSACore: g_asaStraightLineX/YBuf) ──
    static constexpr int kMaxBufLen = 400;
    struct Point { int32_t x; int32_t y; };
    std::array<Point, kMaxBufLen> m_buf{};
    int m_bufCount = 0;

    // ── Short distance buffer (TSACore: g_asaShortDisBuf) ──
    // Tracks recent short-distance points for line entry detection
    static constexpr int kShortDisBufLen = 20;
    std::array<Point, kShortDisBufLen> m_shortDisBuf{};
    int m_shortDisBufCount = 0;

    // ── Line fit results ──
    // TSACore maintains two fits: global (all points) and current (+ cur point)
    struct LineFit {
        float slope = 0.0f;     // A (y = Ax + B or x = Ay + B)
        float intercept = 0.0f; // B
        float normFactor = 1.0f;// sqrt(A*A + 1) for distance calculation
        float totalDist = 0.0f; // sum of squared perpendicular distances
        float maxDist = 0.0f;   // max single-point squared distance
        float lastDist = 0.0f;  // last-point squared distance
        bool  valid = false;
        bool  useYasX = false;  // true: fit x = Ay + B (steep line)
    };
    LineFit m_fitGlobal;   // global fit (all buffered points, no current)
    LineFit m_fitCurrent;  // current fit (including current point)

    // ── Methods ──
    void PushPoint(const AsaCoorResult& c);
    void PushShortDisPoint(const AsaCoorResult& c);

    /// TSACore: UpdateStraightLinePrmt
    /// @param includeCurrent if true, include current point in fit
    /// @param agingWeight effective N for normalization (400 - bufCount)
    LineFit FitLine(bool includeCurrent, int agingWeight) const;

    /// Constrain coordinate to the fitted line
    AsaCoorResult ConstrainToLine(const AsaCoorResult& c,
                                  const LineFit& fit) const;

    void ProcessCurveLine(const AsaCoorResult& c);
    void ProcessEnterStraight(const AsaCoorResult& c);
    void ProcessStraightLine(const AsaCoorResult& c);
    void ProcessExitStraight(const AsaCoorResult& c);

    // Current filtered output
    AsaCoorResult m_output{};
};

} // namespace Asa
