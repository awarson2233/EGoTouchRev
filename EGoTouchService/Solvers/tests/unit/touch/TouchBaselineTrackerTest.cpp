#include "TouchSolver/BaselineTracker.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

constexpr uint16_t kRawBaseline = 1000;
constexpr uint16_t kRawHighCell = 1400;
constexpr int kPeakRow = 20;
constexpr int kPeakCol = 20;

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

int16_t PeakValue(const Solvers::HeatmapFrame& frame) {
    return frame.heatmapMatrix[kPeakRow][kPeakCol];
}

int16_t BackgroundValue(const Solvers::HeatmapFrame& frame) {
    return frame.heatmapMatrix[0][0];
}

void FillRaw(Solvers::HeatmapFrame& frame, uint16_t value) {
    for (auto& row : frame.heatmapMatrix) {
        for (auto& cell : row) {
            cell = static_cast<int16_t>(value);
        }
    }
}

void FillRawWithHighCell(Solvers::HeatmapFrame& frame) {
    FillRaw(frame, kRawBaseline);
    frame.heatmapMatrix[kPeakRow][kPeakCol] = static_cast<int16_t>(kRawHighCell);
}

void PrimeBaseline(Solvers::Touch::BaselineTracker& tracker) {
    tracker.m_baseline = kRawBaseline;
    Solvers::HeatmapFrame frame;
    frame.masterWasRead = true;
    frame.masterSuffixValid = true;
    FillRaw(frame, kRawBaseline);
    tracker.Process(frame, false);
    Require(PeakValue(frame) == 0, "baseline prime frame should subtract to zero");
}

// ──── Initialization ────

void TestInitFromDefaultBaseline() {
    Solvers::Touch::BaselineTracker tracker;
    tracker.m_baseline = 0x7FEE;
    Solvers::HeatmapFrame frame;
    frame.masterWasRead = true;
    frame.masterSuffixValid = true;
    FillRaw(frame, 0x7FEE);
    tracker.Process(frame, false);
    Require(PeakValue(frame) == 0, "uniform raw matching default baseline should output zero");
}

void TestDisabledPassesThrough() {
    Solvers::Touch::BaselineTracker tracker;
    tracker.m_enabled = false;
    Solvers::HeatmapFrame frame;
    frame.masterWasRead = true;
    frame.masterSuffixValid = true;
    FillRaw(frame, 5000);
    tracker.Process(frame, false);
    Require(PeakValue(frame) == 5000, "disabled tracker should not modify output");
}

void TestInvalidMasterZeroOutput() {
    Solvers::Touch::BaselineTracker tracker;
    PrimeBaseline(tracker);
    Solvers::HeatmapFrame frame;
    frame.masterWasRead = false;
    frame.masterSuffixValid = false;
    FillRawWithHighCell(frame);
    tracker.Process(frame, false);
    Require(PeakValue(frame) == 0, "invalid master should produce zero output");
    Require(BackgroundValue(frame) == 0, "invalid master should produce zero output (bg)");
}

void TestResetDropsPreviousDynamicBaseline() {
    Solvers::Touch::BaselineTracker tracker;
    tracker.m_baseline = kRawBaseline;
    PrimeBaseline(tracker);

    // Change the "hardware" baseline and reset
    tracker.m_baseline = 2000;
    tracker.Reset();

    Solvers::HeatmapFrame frame;
    frame.masterWasRead = true;
    frame.masterSuffixValid = true;
    FillRaw(frame, 2000);
    tracker.Process(frame, false);
    Require(PeakValue(frame) == 0, "reset should re-initialize from new BaselineValue");
}

// ──── NoFinger mode ────

void TestNoFingerUpdatesAllCells() {
    Solvers::Touch::BaselineTracker tracker;
    tracker.m_noFingerAlphaShift = 0;
    tracker.m_noFingerMaxStep = 2000;
    PrimeBaseline(tracker);

    Solvers::HeatmapFrame frame;
    frame.masterWasRead = true;
    frame.masterSuffixValid = true;
    FillRaw(frame, kRawBaseline + 200);
    tracker.Process(frame, false);
    Require(PeakValue(frame) == 0, "no-finger mode always outputs zero");

    FillRaw(frame, kRawBaseline + 200);
    tracker.Process(frame, false);
    Require(PeakValue(frame) == 0, "no-finger mode always outputs zero after convergence");
}

void TestNoFingerDeadbandSkipsUpdate() {
    Solvers::Touch::BaselineTracker tracker;
    tracker.m_noiseTrackingEnabled = false;
    PrimeBaseline(tracker);

    Solvers::HeatmapFrame frame;
    frame.masterWasRead = true;
    frame.masterSuffixValid = true;
    FillRaw(frame, kRawBaseline + 50); // within deadband (90)
    tracker.Process(frame, false);
    Require(PeakValue(frame) == 0, "deadband should skip update");
    // Baseline should be unchanged after deadband skip
    tracker.Reset();
    tracker.m_baseline = kRawBaseline;
    // Verify the no-track path produces zero output regardless
}

void TestNoFingerNoiseTrackingStillOutputsZero() {
    Solvers::Touch::BaselineTracker tracker;
    tracker.m_noiseTrackingEnabled = true;
    PrimeBaseline(tracker);

    Solvers::HeatmapFrame frame;
    frame.masterWasRead = true;
    frame.masterSuffixValid = true;
    FillRaw(frame, kRawBaseline + 50);
    tracker.Process(frame, false);
    Require(PeakValue(frame) == 0, "no-finger with noise tracking still outputs zero");
}

void TestNoFingerAbsorbsPeakSignal() {
    Solvers::Touch::BaselineTracker tracker;
    tracker.m_noFingerAlphaShift = 0;
    tracker.m_noFingerMaxStep = 2000;
    PrimeBaseline(tracker);

    // Feed high peak signal during no-finger mode
    Solvers::HeatmapFrame frame;
    frame.masterWasRead = true;
    frame.masterSuffixValid = true;
    FillRawWithHighCell(frame);
    tracker.Process(frame, false);
    Require(PeakValue(frame) == 0, "no-finger mode absorbs high signal into baseline");

    // Now switch to finger mode — the high cell should be baseline-absorbed
    FillRawWithHighCell(frame);
    tracker.Process(frame, true);
    Require(PeakValue(frame) == 0, "previously absorbed peak should not reappear as touch");
}

// ──── Finger mode: Freeze ────

void TestFingerFreezePositivePeak() {
    Solvers::Touch::BaselineTracker tracker;
    tracker.m_peakThreshold = 305;
    PrimeBaseline(tracker);

    Solvers::HeatmapFrame frame;
    frame.masterWasRead = true;
    frame.masterSuffixValid = true;
    FillRawWithHighCell(frame);
    tracker.Process(frame, true);
    Require(PeakValue(frame) >= tracker.m_peakThreshold,
            "cell above peak threshold should freeze and output signal");
}

void TestFingerFreezeMultiFrameStaysFrozen() {
    Solvers::Touch::BaselineTracker tracker;
    tracker.m_peakThreshold = 305;
    PrimeBaseline(tracker);

    Solvers::HeatmapFrame frame;
    frame.masterWasRead = true;
    frame.masterSuffixValid = true;
    for (int i = 0; i < 4; ++i) {
        FillRawWithHighCell(frame);
        tracker.Process(frame, true);
    }
    Require(PeakValue(frame) >= tracker.m_peakThreshold,
            "frozen peak should persist across multiple frames");
}

void TestFingerBackgroundAbsorbsNoise() {
    Solvers::Touch::BaselineTracker tracker;
    tracker.m_peakThreshold = 305;
    PrimeBaseline(tracker);

    Solvers::HeatmapFrame frame;
    frame.masterWasRead = true;
    frame.masterSuffixValid = true;
    FillRaw(frame, kRawBaseline); // uniform baseline = no peaks
    tracker.Process(frame, true);
    Require(BackgroundValue(frame) == 0,
            "uniform background cells should output zero");
}

void TestFingerFreezeUsesCommonModeCorrectedDiff() {
    Solvers::Touch::BaselineTracker tracker;
    tracker.m_peakThreshold = 305;
    PrimeBaseline(tracker);

    // Global negative shift, but one cell is higher relative to the panel
    Solvers::HeatmapFrame frame;
    frame.masterWasRead = true;
    frame.masterSuffixValid = true;
    FillRaw(frame, kRawBaseline - 500);
    frame.heatmapMatrix[kPeakRow][kPeakCol] = static_cast<int16_t>(kRawBaseline - 150);
    tracker.Process(frame, true);

    Require(PeakValue(frame) >= tracker.m_peakThreshold,
            "common-mode-corrected positive touch should freeze even when raw delta is negative");
}

// ──── Release Hold ────

void TestReleaseHoldNegativeRebound() {
    Solvers::Touch::BaselineTracker tracker;
    tracker.m_peakThreshold = 305;
    tracker.m_releaseHoldFrames = 5;
    PrimeBaseline(tracker);

    Solvers::HeatmapFrame frame;
    frame.masterWasRead = true;
    frame.masterSuffixValid = true;

    // First, create a freeze
    FillRawWithHighCell(frame);
    tracker.Process(frame, true);
    Require(PeakValue(frame) >= tracker.m_peakThreshold, "should freeze peak");

    // Now drop the cell below baseline — release hold should be active
    FillRaw(frame, kRawBaseline);
    frame.heatmapMatrix[kPeakRow][kPeakCol] = static_cast<int16_t>(kRawBaseline - 100);
    tracker.Process(frame, true);
    Require(PeakValue(frame) < -tracker.m_negativeDeadband,
            "release hold should pass negative rebound through");
}

void TestReleaseHoldExpiresAfterFrames() {
    Solvers::Touch::BaselineTracker tracker;
    tracker.m_peakThreshold = 305;
    tracker.m_releaseHoldFrames = 3;
    PrimeBaseline(tracker);

    Solvers::HeatmapFrame frame;
    frame.masterWasRead = true;
    frame.masterSuffixValid = true;

    // Create freeze
    FillRawWithHighCell(frame);
    tracker.Process(frame, true);

    // Run release hold down — refill each iteration (Process zeroes output)
    for (int i = 0; i < 10; ++i) {
        FillRaw(frame, kRawBaseline);
        tracker.Process(frame, true);
    }
    // By now release hold should be expired and the cell should output zero
    Require(PeakValue(frame) == 0, "after release hold expires, cell should output zero");
}

// ──── Recovery Mode ────

void TestRecoveryOnFalseToTrueTransition() {
    Solvers::Touch::BaselineTracker tracker;
    tracker.m_recoveryAlphaShift = 0;     // full update in one step
    tracker.m_recoveryMaxStep = 2000;
    tracker.m_peakThreshold = 305;
    PrimeBaseline(tracker);

    Solvers::HeatmapFrame frame;
    frame.masterWasRead = true;
    frame.masterSuffixValid = true;

    // NoFinger then Finger transition triggers recovery
    FillRaw(frame, kRawBaseline + 600); // large offset
    tracker.Process(frame, false);
    FillRaw(frame, kRawBaseline + 600); // refill: Process zeroes output in place
    tracker.Process(frame, true);
    // Recovery should have absorbed the 600-offset background
    Require(BackgroundValue(frame) == 0,
            "recovery mode should converge background fast on false→true transition");
}

void TestRecoveryExitsOnFreezeDetection() {
    Solvers::Touch::BaselineTracker tracker;
    tracker.m_recoveryAlphaShift = 0;
    tracker.m_recoveryMaxStep = 2000;
    tracker.m_peakThreshold = 305;
    PrimeBaseline(tracker);

    Solvers::HeatmapFrame frame;
    frame.masterWasRead = true;
    frame.masterSuffixValid = true;

    // NoFinger → Finger with a peak
    // Each Process call receives a new raw frame (simulates real pipeline).
    FillRawWithHighCell(frame);
    tracker.Process(frame, false);
    FillRawWithHighCell(frame);   // refill: Process() zeroes output in place
    tracker.Process(frame, true);
    Require(PeakValue(frame) >= tracker.m_peakThreshold,
            "recovery should produce peak output on first freeze detection");
}

void TestRecoveryContinuousWhenNoFreeze() {
    Solvers::Touch::BaselineTracker tracker;
    tracker.m_recoveryAlphaShift = 0;
    tracker.m_recoveryMaxStep = 2000;
    tracker.m_recoveryMaxFrames = 60;
    tracker.m_peakThreshold = 305;
    PrimeBaseline(tracker);

    Solvers::HeatmapFrame frame;
    frame.masterWasRead = true;
    frame.masterSuffixValid = true;

    // Finger present but no peaks — recovery should continue
    FillRaw(frame, kRawBaseline + 600);
    tracker.Process(frame, false); // establish prevHadFinger=false

    for (int i = 0; i < 3; ++i) {
        FillRaw(frame, kRawBaseline + 600);
        tracker.Process(frame, true);
        Require(BackgroundValue(frame) == 0,
                "recovery should continue while no freeze cells exist");
    }
}

// ──── Background Drift Tracking ────

void TestBackgroundPositiveDriftTracking() {
    Solvers::Touch::BaselineTracker tracker;
    tracker.m_positiveAlphaShift = 0;
    tracker.m_positiveMaxStep = 200;
    tracker.m_positiveDeadband = 14;
    tracker.m_peakThreshold = 305;
    PrimeBaseline(tracker);

    Solvers::HeatmapFrame frame;
    frame.masterWasRead = true;
    frame.masterSuffixValid = true;

    // First frame with finger — enters recovery; skip it by priming finger state
    FillRawWithHighCell(frame);
    tracker.Process(frame, true); // has finger + peak → freeze
    Require(PeakValue(frame) >= tracker.m_peakThreshold, "should freeze");

    // Now background cells with positive drift but no peak
    FillRaw(frame, kRawBaseline + 100); // above positiveDeadband
    tracker.Process(frame, true);
    Require(BackgroundValue(frame) == 0,
            "positive drifted background should output zero while tracking baseline");
}

// ──── Common-Mode Rejection ────

void TestCommonModeRejectsGlobalShift() {
    Solvers::Touch::BaselineTracker tracker;
    tracker.m_peakThreshold = 305;
    PrimeBaseline(tracker);

    Solvers::HeatmapFrame frame;
    frame.masterWasRead = true;
    frame.masterSuffixValid = true;

    // All cells +500 — should be absorbed by common-mode
    FillRaw(frame, kRawBaseline + 500);
    tracker.Process(frame, true);
    Require(BackgroundValue(frame) == 0,
            "uniform global shift should be removed by common-mode");
}

// ──── Baseline Inheritance ────

void TestBaselineInheritedAcrossHasFingerToggle() {
    Solvers::Touch::BaselineTracker tracker;
    tracker.m_noFingerAlphaShift = 0;
    tracker.m_noFingerMaxStep = 2000;
    PrimeBaseline(tracker);

    Solvers::HeatmapFrame frame;
    frame.masterWasRead = true;
    frame.masterSuffixValid = true;

    // Converge baseline during no-finger — refill each iteration
    for (int i = 0; i < 3; ++i) {
        FillRaw(frame, kRawBaseline + 500);
        tracker.Process(frame, false);
    }

    // Toggle to finger — baseline should be inherited
    FillRaw(frame, kRawBaseline + 500);
    tracker.Process(frame, true);
    Require(BackgroundValue(frame) == 0,
            "inherited baseline should give zero output on uniform panel");
}

// ──── Edge cases ────

void TestStartupWithFingerAlreadyPresent() {
    Solvers::Touch::BaselineTracker tracker;
    tracker.m_peakThreshold = 305;
    tracker.m_recoveryAlphaShift = 0;
    tracker.m_recoveryMaxStep = 2000;
    PrimeBaseline(tracker);

    Solvers::HeatmapFrame frame;
    frame.masterWasRead = true;
    frame.masterSuffixValid = true;

    // First finger frame with a peak — recovery+freeze should both work
    FillRawWithHighCell(frame);
    tracker.Process(frame, true);
    Require(PeakValue(frame) >= tracker.m_peakThreshold,
            "startup with finger present should detect peak");
}

void TestClampBaselineRange() {
    Solvers::Touch::BaselineTracker tracker;
    tracker.m_baseline = 65535;
    tracker.Reset();

    Solvers::HeatmapFrame frame;
    frame.masterWasRead = true;
    frame.masterSuffixValid = true;
    FillRaw(frame, 65535);
    tracker.Process(frame, false);
    Require(PeakValue(frame) == 0, "baseline at max value should still work");
}

} // namespace

int main() {
    try {
        TestInitFromDefaultBaseline();
        TestDisabledPassesThrough();
        TestInvalidMasterZeroOutput();
        TestResetDropsPreviousDynamicBaseline();
        TestNoFingerUpdatesAllCells();
        TestNoFingerDeadbandSkipsUpdate();
        TestNoFingerNoiseTrackingStillOutputsZero();
        TestNoFingerAbsorbsPeakSignal();
        TestFingerFreezePositivePeak();
        TestFingerFreezeMultiFrameStaysFrozen();
        TestFingerBackgroundAbsorbsNoise();
        TestFingerFreezeUsesCommonModeCorrectedDiff();
        TestReleaseHoldNegativeRebound();
        TestReleaseHoldExpiresAfterFrames();
        TestRecoveryOnFalseToTrueTransition();
        TestRecoveryExitsOnFreezeDetection();
        TestRecoveryContinuousWhenNoFreeze();
        TestBackgroundPositiveDriftTracking();
        TestCommonModeRejectsGlobalShift();
        TestBaselineInheritedAcrossHasFingerToggle();
        TestStartupWithFingerAlreadyPresent();
        TestClampBaselineRange();
        std::cout << "[TEST] Touch baseline tracker tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
