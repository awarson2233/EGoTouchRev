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
/// When drawing a straight line (states 4-5), the coordinate perpendicular
/// to the line direction is locked/filtered, greatly reducing hand tremor.
///
/// Uses least-squares line fitting (UpdateStraightLinePrmt) on a rolling
/// buffer to detect line direction and measure deviation.
class LinearFilter {
public:
    /// Process one coordinate frame through the state machine.
    /// @param coor  Input coordinate (post-interpolation, pre-IIR)
    /// @param pressure  Current pressure (0 = pen up)
    /// @return Filtered coordinate
    AsaCoorResult Process(const AsaCoorResult& coor, uint16_t pressure);

    /// Reset state machine to Init
    void Reset();

    /// Get current state machine state (for monitoring)
    int GetState() const;

    /// Enable/disable the linear filter
    bool enabled = false;

    // ── Configuration ──
    /// Minimum frames to collect before judging line (states 0-2)
    int collectFrames = 3;

    /// Minimum buffer length before line fitting begins
    int minFitLength = 20;

    /// Maximum residual (mean squared error) to enter straight mode
    float enterResidualThreshold = 50.0f;

    /// Maximum deviation to stay in straight mode
    float stayMaxDeviation = 150.0f;

    /// Exit straight mode if deviation exceeds this
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

    // ── Rolling buffer for line fitting ──
    static constexpr int kMaxBufLen = 400;
    struct Point { int32_t x; int32_t y; };
    std::array<Point, kMaxBufLen> m_buf{};
    int m_bufCount = 0;

    // ── Line fit results ──
    struct LineFit {
        float slope = 0.0f;     // A (y = Ax + B)
        float intercept = 0.0f; // B
        float residual = 0.0f;  // mean squared error
        float maxDev = 0.0f;    // max single-point deviation
        bool  valid = false;
        bool  useYasX = false;  // true: fit x = Ay + B (steep line)
    };
    LineFit m_fit;

    // ── Methods ──
    void PushPoint(const AsaCoorResult& c);
    LineFit FitLine(int startIdx, int count) const;
    AsaCoorResult ConstrainToLine(const AsaCoorResult& c) const;

    void ProcessCurveLine(const AsaCoorResult& c);
    void ProcessEnterStraight(const AsaCoorResult& c);
    void ProcessStraightLine(const AsaCoorResult& c);
    void ProcessExitStraight(const AsaCoorResult& c);

    // Current filtered output
    AsaCoorResult m_output{};
};

} // namespace Asa
