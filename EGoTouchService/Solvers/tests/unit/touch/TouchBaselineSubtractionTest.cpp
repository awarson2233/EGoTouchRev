#include "TouchSolver/BaselineSubtraction.hpp"

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

Solvers::Touch::BaselineInputState BaselineInput(Solvers::Touch::FingerState fingerState) {
    return {true, fingerState};
}

Solvers::Touch::BaselineInputState InvalidMasterInput() {
    return {false, Solvers::Touch::FingerState::Unknown};
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

void PrimeBaseline(Solvers::Touch::BaselineSubtraction& baseline) {
    baseline.m_baseline = kRawBaseline;
    Solvers::HeatmapFrame frame;
    FillRaw(frame, kRawBaseline);
    baseline.Process(frame);
    Require(PeakValue(frame) == 0, "baseline prime frame should subtract to zero");
}

void TestLocalPositivePeakFreezesWithoutReset() {
    Solvers::Touch::BaselineSubtraction baseline;
    PrimeBaseline(baseline);

    Solvers::HeatmapFrame frame;
    for (int i = 0; i < 4; ++i) {
        FillRawWithHighCell(frame);
        baseline.Process(frame);
    }

    Require(PeakValue(frame) >= baseline.m_touchFreezeThreshold,
            "local positive peak should remain frozen without reset");
}

void TestRequestReacquireFramesOnlyResetsBaseline() {
    Solvers::Touch::BaselineSubtraction baseline;
    baseline.m_baseline = kRawBaseline;
    PrimeBaseline(baseline);

    baseline.m_baseline = 2000;
    baseline.RequestReacquireFrames(8);

    Solvers::HeatmapFrame frame;
    FillRaw(frame, 2500);
    baseline.Process(frame);

    Require(PeakValue(frame) == 0,
            "request reacquire should suppress fallback common-mode diff after default reset");
}

void TestFallbackCommonModeOffsetDoesNotRaiseBackground() {
    Solvers::Touch::BaselineSubtraction baseline;
    baseline.m_freezeCandidateThreshold = 350;
    PrimeBaseline(baseline);

    Solvers::HeatmapFrame frame;
    FillRaw(frame, kRawBaseline + 600);
    frame.heatmapMatrix[kPeakRow][kPeakCol] = static_cast<int16_t>(kRawBaseline + 1000);
    baseline.Process(frame);

    Require(BackgroundValue(frame) == 0,
            "fallback common-mode offset should not produce high background output");
    Require(PeakValue(frame) >= baseline.m_freezeCandidateThreshold,
            "fallback common-mode correction should preserve local peak diff");
}

void TestRequestReacquireFramesSuppressesBoundedWindow() {
    Solvers::Touch::BaselineSubtraction baseline;
    PrimeBaseline(baseline);

    baseline.RequestReacquireFrames(2);

    Solvers::HeatmapFrame frame;
    FillRawWithHighCell(frame);
    baseline.Process(frame);
    Require(PeakValue(frame) == 0,
            "request reacquire should suppress the first recovery frame");

    FillRawWithHighCell(frame);
    baseline.Process(frame);
    Require(PeakValue(frame) == 0,
            "request reacquire should honor the requested bounded recovery window");

    FillRawWithHighCell(frame);
    baseline.Process(frame);
    Require(PeakValue(frame) >= baseline.m_touchFreezeThreshold,
            "request reacquire should stop suppressing after requested frames");
}

void TestResetDropsPreviousDynamicBaselineAndUsesDefault() {
    Solvers::Touch::BaselineSubtraction baseline;
    baseline.m_baseline = kRawBaseline;
    PrimeBaseline(baseline);

    baseline.m_baseline = 2000;
    baseline.Reset();

    Solvers::HeatmapFrame frame;
    FillRaw(frame, 2500);
    baseline.Process(frame);

    Require(PeakValue(frame) == 0,
            "reset should initialize from BaselineValue while suppressing fallback common-mode diff");
}

void TestNoFingerUpdatesAllCellsEvenWhenPeakIsHigh() {
    Solvers::Touch::BaselineSubtraction baseline;
    baseline.m_noFingerAlphaShift = 0;
    baseline.m_noFingerMaxStep = 2000;
    PrimeBaseline(baseline);

    Solvers::HeatmapFrame frame;
    FillRawWithHighCell(frame);
    baseline.Process(frame, BaselineInput(Solvers::Touch::FingerState::NoFinger));
    Require(PeakValue(frame) == 0,
            "no-finger baseline should suppress output while absorbing every cell");

    FillRawWithHighCell(frame);
    baseline.Process(frame, BaselineInput(Solvers::Touch::FingerState::Finger));
    Require(PeakValue(frame) == 0,
            "high cell seen during confirmed no-finger should be part of baseline later");
}

void TestFingerFreezesCandidatePeakButTracksBackground() {
    Solvers::Touch::BaselineSubtraction baseline;
    baseline.m_freezeCandidateThreshold = 350;
    baseline.m_fingerBackgroundAlphaShift = 0;
    baseline.m_fingerBackgroundMaxStep = 2000;
    PrimeBaseline(baseline);

    Solvers::HeatmapFrame frame;
    FillRaw(frame, kRawBaseline + 600);
    frame.heatmapMatrix[kPeakRow][kPeakCol] = static_cast<int16_t>(kRawBaseline + 1000);

    baseline.Process(frame, BaselineInput(Solvers::Touch::FingerState::Finger));

    Require(BackgroundValue(frame) == 0,
            "finger background cells should continue dynamic baseline tracking");
    Require(PeakValue(frame) >= baseline.m_freezeCandidateThreshold,
            "finger candidate peak cell should be frozen and reported as diff");
}

void TestInvalidMasterDoesNotPolluteBaseline() {
    Solvers::Touch::BaselineSubtraction baseline;
    PrimeBaseline(baseline);

    Solvers::HeatmapFrame frame;
    FillRawWithHighCell(frame);
    baseline.Process(frame, InvalidMasterInput());
    Require(PeakValue(frame) == 0,
            "invalid master frame should produce safe zero output");

    FillRawWithHighCell(frame);
    baseline.Process(frame, BaselineInput(Solvers::Touch::FingerState::Finger));
    Require(PeakValue(frame) >= baseline.m_touchFreezeThreshold,
            "invalid master frame should not absorb a later candidate touch peak");
}

void TestFingerFreezeUsesTouchFreezeThreshold() {
    Solvers::Touch::BaselineSubtraction baseline;
    baseline.m_touchFreezeThreshold = 305;
    baseline.m_freezeCandidateThreshold = 350;
    baseline.m_fingerBackgroundAlphaShift = 0;
    baseline.m_fingerBackgroundMaxStep = 2000;
    PrimeBaseline(baseline);

    Solvers::HeatmapFrame frame;
    FillRaw(frame, kRawBaseline);
    frame.heatmapMatrix[kPeakRow][kPeakCol] = static_cast<int16_t>(kRawBaseline + 320);
    baseline.Process(frame, BaselineInput(Solvers::Touch::FingerState::Finger));

    Require(PeakValue(frame) >= baseline.m_touchFreezeThreshold,
            "explicit finger freeze should use BaselineTouchFreezeThreshold");
}

void TestFingerFreezeUsesCommonModeCorrectedDiff() {
    Solvers::Touch::BaselineSubtraction baseline;
    baseline.m_touchFreezeThreshold = 305;
    baseline.m_freezeCandidateThreshold = 350;
    baseline.m_fingerBackgroundAlphaShift = 0;
    baseline.m_fingerBackgroundMaxStep = 2000;
    PrimeBaseline(baseline);

    Solvers::HeatmapFrame frame;
    FillRaw(frame, kRawBaseline - 500);
    frame.heatmapMatrix[kPeakRow][kPeakCol] = static_cast<int16_t>(kRawBaseline - 150);
    baseline.Process(frame, BaselineInput(Solvers::Touch::FingerState::Finger));

    Require(PeakValue(frame) >= baseline.m_touchFreezeThreshold,
            "common-mode-corrected positive touch should freeze even when raw delta is negative");
}

void TestBroadPositiveShiftDoesNotFreezePartialPanel() {
    Solvers::Touch::BaselineSubtraction baseline;
    baseline.m_touchFreezeThreshold = 305;
    baseline.m_freezeCandidateThreshold = 350;
    baseline.m_fingerBackgroundAlphaShift = 0;
    baseline.m_fingerBackgroundMaxStep = 2000;
    PrimeBaseline(baseline);

    Solvers::HeatmapFrame frame;
    FillRaw(frame, kRawBaseline);
    for (int i = 0; i <= (Solvers::Touch::BaselineSubtraction::kCellCount / 8); ++i) {
        (&frame.heatmapMatrix[0][0])[i] = static_cast<int16_t>(kRawBaseline + 500);
    }

    baseline.Process(frame, BaselineInput(Solvers::Touch::FingerState::Finger));

    Require(frame.heatmapMatrix[0][0] == 0,
            "broad partial-panel positive shift should not be frozen as touch");
}

void TestUnknownBroadPartialPositiveShiftSuppressesOutput() {
    Solvers::Touch::BaselineSubtraction baseline;
    baseline.m_touchFreezeThreshold = 305;
    baseline.m_freezeCandidateThreshold = 350;
    baseline.m_fingerBackgroundAlphaShift = 0;
    baseline.m_fingerBackgroundMaxStep = 2000;
    PrimeBaseline(baseline);

    Solvers::HeatmapFrame frame;
    FillRaw(frame, kRawBaseline);
    for (int i = 0; i <= (Solvers::Touch::BaselineSubtraction::kCellCount / 8); ++i) {
        (&frame.heatmapMatrix[0][0])[i] = static_cast<int16_t>(kRawBaseline + 500);
    }

    baseline.Process(frame, BaselineInput(Solvers::Touch::FingerState::Unknown));

    for (const auto& row : frame.heatmapMatrix) {
        for (int16_t cell : row) {
            Require(cell <= 0,
                    "unknown no-finger state should not emit positive broad partial-panel output");
        }
    }
}

void TestFingerReleaseHoldProtectsNegativeRebound() {
    Solvers::Touch::BaselineSubtraction baseline;
    baseline.m_releaseHoldFrames = 2;
    baseline.m_fingerBackgroundAlphaShift = 0;
    baseline.m_fingerBackgroundMaxStep = 2000;
    PrimeBaseline(baseline);

    Solvers::HeatmapFrame frame;
    FillRawWithHighCell(frame);
    baseline.Process(frame, BaselineInput(Solvers::Touch::FingerState::Finger));
    Require(PeakValue(frame) >= baseline.m_touchFreezeThreshold,
            "finger frame should establish release hold on frozen peak");

    FillRaw(frame, kRawBaseline);
    frame.heatmapMatrix[kPeakRow][kPeakCol] = static_cast<int16_t>(kRawBaseline - 100);
    baseline.Process(frame, BaselineInput(Solvers::Touch::FingerState::Finger));
    Require(PeakValue(frame) < -baseline.m_negativeDeadband,
            "release hold should preserve negative rebound instead of absorbing it");
}

void TestUnknownGapPreservesReleaseHoldForRebound() {
    Solvers::Touch::BaselineSubtraction baseline;
    baseline.m_releaseHoldFrames = 3;
    baseline.m_fingerBackgroundAlphaShift = 0;
    baseline.m_fingerBackgroundMaxStep = 2000;
    PrimeBaseline(baseline);

    Solvers::HeatmapFrame frame;
    FillRawWithHighCell(frame);
    baseline.Process(frame, BaselineInput(Solvers::Touch::FingerState::Finger));
    Require(PeakValue(frame) >= baseline.m_touchFreezeThreshold,
            "finger frame should establish release hold before unknown gap");

    FillRawWithHighCell(frame);
    baseline.Process(frame, BaselineInput(Solvers::Touch::FingerState::Unknown));
    Require(PeakValue(frame) == 0,
            "unknown gap should suppress positive local diff output");

    FillRaw(frame, kRawBaseline);
    frame.heatmapMatrix[kPeakRow][kPeakCol] = static_cast<int16_t>(kRawBaseline - 100);
    baseline.Process(frame, BaselineInput(Solvers::Touch::FingerState::Finger));
    Require(PeakValue(frame) < -baseline.m_negativeDeadband,
            "release hold should survive unknown gap for the first rebound frame");

    FillRaw(frame, kRawBaseline);
    frame.heatmapMatrix[kPeakRow][kPeakCol] = static_cast<int16_t>(kRawBaseline - 100);
    baseline.Process(frame, BaselineInput(Solvers::Touch::FingerState::Finger));
    Require(PeakValue(frame) < -baseline.m_negativeDeadband,
            "unknown gap should not clear hold or absorb rebound into baseline before hold expires");
}

} // namespace

int main() {
    try {
        TestLocalPositivePeakFreezesWithoutReset();
        TestRequestReacquireFramesOnlyResetsBaseline();
        TestFallbackCommonModeOffsetDoesNotRaiseBackground();
        TestRequestReacquireFramesSuppressesBoundedWindow();
        TestResetDropsPreviousDynamicBaselineAndUsesDefault();
        TestNoFingerUpdatesAllCellsEvenWhenPeakIsHigh();
        TestFingerFreezesCandidatePeakButTracksBackground();
        TestInvalidMasterDoesNotPolluteBaseline();
        TestFingerFreezeUsesTouchFreezeThreshold();
        TestFingerFreezeUsesCommonModeCorrectedDiff();
        TestBroadPositiveShiftDoesNotFreezePartialPanel();
        TestUnknownBroadPartialPositiveShiftSuppressesOutput();
        TestFingerReleaseHoldProtectsNegativeRebound();
        TestUnknownGapPreservesReleaseHoldForRebound();
        std::cout << "[TEST] Touch baseline subtraction tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
