#include "StylusSolver/CoordinateSolver.hpp"
#include "StylusSolver/GridFeatureExtractor.hpp"
#include "StylusSolver/LinearFilterProcess.hpp"
#include "StylusSolver/TiltProcess.hpp"
#include "StylusSolver/StylusPipeline.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

Asa::AsaCoorResult Coor(int32_t dim1, int32_t dim2) {
    Asa::AsaCoorResult coor{};
    coor.valid = true;
    coor.dim1 = dim1;
    coor.dim2 = dim2;
    return coor;
}

void ConfigureFastTestFilter(Solvers::Stylus::LinearFilterProcess& filter) {
    filter.m_sparseMoveThreshold = 10;
    filter.m_shortMoveThreshold = 4;
    filter.m_minFitPoints = 4;
    filter.m_anchorMoveThreshold = 1;
    filter.m_enterCountMax = 2;
    filter.m_exitCountMax = 2;
    filter.m_dragLimit = 32;
    filter.m_enterMaxDistSq = 4;
    filter.m_exitDistSq = 10000;
}

Asa::AsaCoorResult Feed(Solvers::Stylus::LinearFilterProcess& filter,
                        int32_t dim1,
                        int32_t dim2,
                        bool pressure = true) {
    return filter.Process(Coor(dim1, dim2), pressure, 60000, 40000);
}

void FeedStraightToStable(Solvers::Stylus::LinearFilterProcess& filter) {
    ConfigureFastTestFilter(filter);
    for (int i = 0; i < 9; ++i) {
        Feed(filter, 1000 + i * 80, 2000);
    }
    Require(filter.State() == 5,
            "straight sparse points should advance the filter into straight-line state");
}

void LoadFromSavedText(Solvers::StylusPipeline& pipeline, const std::string& saved) {
    std::istringstream in(saved);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        const size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        pipeline.LoadConfig(line.substr(0, eq), line.substr(eq + 1));
    }
}

void SetProjectionPeak(Asa::AsaProjection& projection,
                       int peakDim1,
                       int peakDim2,
                       int leftDim1,
                       int centerDim1,
                       int rightDim1,
                       int leftDim2,
                       int centerDim2,
                       int rightDim2) {
    projection.Clear();
    projection.peakIdxDim1 = peakDim1;
    projection.peakIdxDim2 = peakDim2;
    projection.dim1[peakDim1 - 1] = leftDim1;
    projection.dim1[peakDim1] = centerDim1;
    projection.dim1[peakDim1 + 1] = rightDim1;
    projection.dim2[peakDim2 - 1] = leftDim2;
    projection.dim2[peakDim2] = centerDim2;
    projection.dim2[peakDim2 + 1] = rightDim2;
}

void SetTx1LinePeakFrame(Solvers::HeatmapFrame& frame,
                         int leftCol,
                         int leftValue,
                         int rightCol = -1,
                         int rightValue = 0) {
    auto& stylus = frame.stylus;
    stylus.runtime.parse.valid = true;
    stylus.runtime.rawGrid.asaGrid.tx1.valid = true;
    stylus.runtime.rawGrid.asaGrid.tx1.anchorRow = 4;
    stylus.runtime.rawGrid.asaGrid.tx1.anchorCol = 4;
    stylus.runtime.rawGrid.asaGrid.tx1.grid[4][leftCol] = static_cast<int16_t>(leftValue);
    if (rightCol >= 0) {
        stylus.runtime.rawGrid.asaGrid.tx1.grid[4][rightCol] = static_cast<int16_t>(rightValue);
    }
}

void TestDefaultParametersMatchAsaTable() {
    Solvers::Stylus::LinearFilterProcess filter;

    Require(filter.m_sparseMoveThreshold == 64,
            "default straight sparse threshold should match TSACore");
    Require(filter.m_shortMoveThreshold == 16,
            "default short-distance threshold should match TSACore");
    Require(filter.m_anchorMoveThreshold == 32,
            "default enter/exit anchor threshold should match TSACore");
    Require(filter.m_dragLimit == 0x20,
            "default drag limit should match ASA table +0xa62");
    Require(filter.m_enterMaxDistSq == 0x0384,
            "default enter max distance should match ASA table +0xa64");
    Require(filter.m_exitDistSq == 0x0e10,
            "default exit distance should match ASA table +0xa66");
}

void TestPressureResetReturnsRawAndStateZero() {
    Solvers::Stylus::LinearFilterProcess filter;
    FeedStraightToStable(filter);

    const auto out = Feed(filter, 2500, 2300, false);
    Require(filter.State() == 0,
            "zero pressure should reset linear filter state");
    Require(out.dim1 == 2500 && out.dim2 == 2300,
            "zero pressure should pass raw coordinate through");
}

void TestStraightLineEntryAndDragLimit() {
    Solvers::Stylus::LinearFilterProcess filter;
    FeedStraightToStable(filter);

    const auto raw = Coor(1800, 2100);
    const auto out = filter.Process(raw, true, 60000, 40000);
    const int rawDist = std::abs(raw.dim2 - 2000);
    const int outDist = std::abs(out.dim2 - 2000);

    Require(outDist < rawDist,
            "straight-line mode should drag the point toward the fitted line");
    Require(std::abs(out.dim2 - raw.dim2) <= filter.m_dragLimit,
            "straight-line drag should be capped by drag limit");
}

void TestLargeDeviationExitsStraightLine() {
    Solvers::Stylus::LinearFilterProcess filter;
    FeedStraightToStable(filter);
    filter.m_exitDistSq = 100;

    Feed(filter, 1800, 2300);
    Require(filter.State() == 6,
            "large point-to-line distance should enter exit state");
}

void TestDirectionChangeExitsStraightLine() {
    Solvers::Stylus::LinearFilterProcess filter;
    FeedStraightToStable(filter);
    filter.m_exitDistSq = 1000000;

    Feed(filter, 1200, 2000);
    Require(filter.State() == 6,
            "sharp direction change should enter exit state");
}

void TestClampToSensorBounds() {
    Solvers::Stylus::LinearFilterProcess filter;
    ConfigureFastTestFilter(filter);

    const auto out = filter.Process(Coor(5000, -100), true, 3000, 2000);
    Require(out.dim1 == 3000,
            "filtered output should clamp dim1 to sensor maximum");
    Require(out.dim2 == 0,
            "filtered output should clamp dim2 to zero");
}

void TestBypassModeResetsPostFilter() {
    Solvers::HeatmapFrame frame{};
    auto& stylus = frame.stylus;
    stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(1234, 2345);
    stylus.runtime.decision.inRangeCandidate = true;
    stylus.runtime.decision.tipDownCandidate = true;
    stylus.runtime.pressure.outputPressure = 300;

    Require(stylus.runtime.post.finalCoor.dim1 == 1234 &&
            stylus.runtime.post.finalCoor.dim2 == 2345,
            "bypass mode should copy raw coordinate");
    Require(stylus.runtime.post.linearFilterState == 0,
            "bypass mode should reset linear filter state");
}

void TestConfigRoundTrip() {
    Solvers::StylusPipeline pipeline;
    pipeline.m_tiltProcess.m_enabled = false;
    pipeline.m_coordinateSolver.m_signalFloor = 77;

    std::ostringstream out;
    pipeline.SaveConfig(out);
    const std::string saved = out.str();

    Require(saved.find("sp.tiltProcessEnabled=0") != std::string::npos,
            "saved config should include tilt process enabled flag");
    Require(saved.find("sp.signalFloor=77") != std::string::npos,
            "saved config should include coordinate signal floor");

    Solvers::StylusPipeline loaded;
    LoadFromSavedText(loaded, saved);
}

void TestGridFeatureExtractorAlignsTx2WithFactoryFlow() {
    Solvers::HeatmapFrame frame{};
    auto& stylus = frame.stylus;
    stylus.runtime.parse.valid = true;
    stylus.runtime.rawGrid.asaGrid.tx1.valid = true;
    stylus.runtime.rawGrid.asaGrid.tx2.valid = true;
    stylus.runtime.rawGrid.asaGrid.tx1.anchorRow = 4;
    stylus.runtime.rawGrid.asaGrid.tx1.anchorCol = 4;
    stylus.runtime.rawGrid.asaGrid.tx2.anchorRow = 4;
    stylus.runtime.rawGrid.asaGrid.tx2.anchorCol = 4;
    stylus.runtime.rawGrid.asaGrid.tx1.grid[4][4] = 200;
    stylus.runtime.rawGrid.asaGrid.tx2.grid[4][4] = 130;
    stylus.runtime.rawGrid.asaGrid.tx2.grid[5][4] = 150;

    Solvers::Stylus::GridFeatureExtractor extractor;
    extractor.Process(frame);

    const auto& tx2 = stylus.runtime.tx2.feature;
    Require(tx2.grid[4][4] == 90,
            "TX2 grid should subtract TX1 contribution before peak detection");
    Require(tx2.peakTable.count == 1,
            "TX2 should retain one peak region in this fixture");
    Require(tx2.peakTable.strongestSlot == 0 && tx2.peakTable.weakestSlot == 0,
            "single TX2 region should be both strongest and weakest");
    Require(tx2.peak.valid && tx2.peak.peakRow == 5 && tx2.peak.peakCol == 4,
            "TX2 strongest peak should resolve to the higher local maximum");
    Require(tx2.refinedLocalCoor.valid,
            "TX2 refined local coordinate should be populated");
    Require(tx2.refinedLocalCoor.dim1 == 4 * Asa::kCoorUnit + (Asa::kCoorUnit / 2) &&
            tx2.refinedLocalCoor.dim2 == 5258,
            "TX2 refined coordinate should follow factory weighted refinement");
    Require(tx2.peakSignal == 240,
            "TX2 peak signal should use the strongest 3x3 sum");
}

void TestTx2SeedThresholdIsGreaterThanNinetyNine() {
    Solvers::HeatmapFrame lowFrame{};
    auto& low = lowFrame.stylus;
    low.runtime.parse.valid = true;
    low.runtime.rawGrid.asaGrid.tx1.valid = true;
    low.runtime.rawGrid.asaGrid.tx2.valid = true;
    low.runtime.rawGrid.asaGrid.tx1.anchorRow = 0;
    low.runtime.rawGrid.asaGrid.tx1.anchorCol = 0;
    low.runtime.rawGrid.asaGrid.tx2.anchorRow = 4;
    low.runtime.rawGrid.asaGrid.tx2.anchorCol = 4;
    low.runtime.rawGrid.asaGrid.tx1.grid[0][0] = 200;
    low.runtime.rawGrid.asaGrid.tx2.grid[4][4] = 99;

    Solvers::Stylus::GridFeatureExtractor extractor;
    extractor.Process(lowFrame);
    Require(low.runtime.tx2.feature.peakTable.count == 0,
            "TX2 candidate value 99 should not seed a peak");

    Solvers::HeatmapFrame highFrame{};
    auto& high = highFrame.stylus;
    high.runtime.parse.valid = true;
    high.runtime.rawGrid.asaGrid.tx1.valid = true;
    high.runtime.rawGrid.asaGrid.tx2.valid = true;
    high.runtime.rawGrid.asaGrid.tx1.anchorRow = 0;
    high.runtime.rawGrid.asaGrid.tx1.anchorCol = 0;
    high.runtime.rawGrid.asaGrid.tx2.anchorRow = 4;
    high.runtime.rawGrid.asaGrid.tx2.anchorCol = 4;
    high.runtime.rawGrid.asaGrid.tx1.grid[0][0] = 200;
    high.runtime.rawGrid.asaGrid.tx2.grid[4][4] = 100;

    extractor.Process(highFrame);
    Require(high.runtime.tx2.feature.peakTable.count == 1,
            "TX2 candidate value 100 should seed a peak under the factory >99 gate");
}

void TestTx1LinePeakFirstFrameUsesStrongestCurrentPeak() {
    Solvers::HeatmapFrame frame{};
    SetTx1LinePeakFrame(frame, 3, 540, 6, 520);

    Solvers::Stylus::GridFeatureExtractor extractor;
    extractor.Process(frame);

    Require(frame.stylus.runtime.tx1.feature.projection.peakIdxDim1 == 3,
            "first TX1 line peak frame should select the strongest current dim1 peak");
    Require(frame.stylus.runtime.tx1.feature.projection.peakIdxDim2 == 4,
            "first TX1 line peak frame should still resolve the dim2 projection peak");
}

void TestTx1LinePeakHistoryCarriesPreviousSelection() {
    Solvers::Stylus::GridFeatureExtractor extractor;

    Solvers::HeatmapFrame first{};
    SetTx1LinePeakFrame(first, 3, 540, 6, 520);
    extractor.Process(first);
    Require(first.stylus.runtime.tx1.feature.projection.peakIdxDim1 == 3,
            "history seed frame should select the initial dim1 peak");

    Solvers::HeatmapFrame second{};
    SetTx1LinePeakFrame(second, 3, 520, 6, 560);
    extractor.Process(second);
    Require(second.stylus.runtime.tx1.feature.projection.peakIdxDim1 == 3,
            "TX1 line peak history should keep a nearby previous peak selected over a slightly stronger newcomer");
}

void TestTx1LinePeakHistoryResetsAfterInvalidParse() {
    Solvers::Stylus::GridFeatureExtractor extractor;

    Solvers::HeatmapFrame first{};
    SetTx1LinePeakFrame(first, 3, 540, 6, 520);
    extractor.Process(first);

    Solvers::HeatmapFrame invalid{};
    extractor.Process(invalid);

    Solvers::HeatmapFrame second{};
    SetTx1LinePeakFrame(second, 3, 520, 6, 560);
    extractor.Process(second);
    Require(second.stylus.runtime.tx1.feature.projection.peakIdxDim1 == 6,
            "invalid parse frame should clear TX1 line peak history before the next valid frame");
}

void TestTiltJitterFilterMatchesTsacore() {
    Require(Solvers::Stylus::TiltProcess::JitterFilter1Degree(5, 7) == 6,
            "jitter filter should pull larger current tilt back by one degree");
    Require(Solvers::Stylus::TiltProcess::JitterFilter1Degree(7, 5) == 6,
            "jitter filter should pull smaller current tilt forward by one degree");
    Require(Solvers::Stylus::TiltProcess::JitterFilter1Degree(7, 7) == 7,
            "jitter filter should keep equal tilt unchanged");
}

void TestCoordinateSolverUsesFixedDevicePitchTables() {
    Solvers::HeatmapFrame frame{};
    auto& stylus = frame.stylus;
    stylus.runtime.tx1.feature.peak.valid = true;
    stylus.runtime.rawGrid.asaGrid.tx1.anchorRow = 4;
    stylus.runtime.rawGrid.asaGrid.tx1.anchorCol = 4;
    SetProjectionPeak(stylus.runtime.tx1.feature.projection,
                      4, 4,
                      100, 300, 100,
                      100, 300, 100);

    Solvers::Stylus::CoordinateSolver solver;
    Require(solver.m_signalFloor == 64,
            "coordinate solver signal floor should remain fixed to the device default");
    solver.Process(frame);

    Require(stylus.runtime.tx1.coordinate.localGridCoor.valid,
            "coordinate solver should produce a valid local coordinate for a symmetric peak");
    Require(stylus.runtime.tx1.coordinate.localGridCoor.dim1 == 4608 &&
            stylus.runtime.tx1.coordinate.localGridCoor.dim2 == 4608,
            "triangle path should resolve the symmetric peak to the cell center before pitch mapping");
    Require(stylus.runtime.tx1.coordinate.reportGlobalCoor.dim1 == 4536,
            "dim1 report coordinate should use the fixed TSA dim1 pitch table");
    Require(stylus.runtime.tx1.coordinate.reportGlobalCoor.dim2 == 4608,
            "dim2 report coordinate should stay unchanged because the fixed TSA dim2 pitch table is identity");
}

void TestCoordinateSolverClampsGlobalCoordinateAtTopLeftEdge() {
    Solvers::HeatmapFrame frame{};
    auto& stylus = frame.stylus;
    stylus.runtime.tx1.feature.peak.valid = true;
    stylus.runtime.rawGrid.asaGrid.tx1.anchorRow = 0;
    stylus.runtime.rawGrid.asaGrid.tx1.anchorCol = 0;

    auto& projection = stylus.runtime.tx1.feature.projection;
    projection.Clear();
    projection.peakIdxDim1 = 0;
    projection.peakIdxDim2 = 0;
    projection.dim1[0] = 300;
    projection.dim1[1] = 100;
    projection.dim1[2] = 50;
    projection.dim2[0] = 300;
    projection.dim2[1] = 100;
    projection.dim2[2] = 50;

    Solvers::Stylus::CoordinateSolver solver;
    solver.Process(frame);

    Require(stylus.runtime.tx1.coordinate.localGridCoor.valid,
            "edge projection should still produce a valid local coordinate");
    Require(stylus.runtime.tx1.coordinate.reportGlobalCoor.dim1 == 0 &&
            stylus.runtime.tx1.coordinate.reportGlobalCoor.dim2 == 0,
            "top-left anchored global coordinate should clamp to zero before pitch mapping");
}

void TestCoordinateSolverClampsGlobalCoordinateAtBottomRightEdge() {
    Solvers::HeatmapFrame frame{};
    auto& stylus = frame.stylus;
    stylus.runtime.tx1.feature.peak.valid = true;
    stylus.runtime.rawGrid.asaGrid.tx1.anchorRow = 39;
    stylus.runtime.rawGrid.asaGrid.tx1.anchorCol = 59;

    auto& projection = stylus.runtime.tx1.feature.projection;
    projection.Clear();
    projection.peakIdxDim1 = Asa::kGridDim - 1;
    projection.peakIdxDim2 = Asa::kGridDim - 1;
    projection.dim1[Asa::kGridDim - 1] = 300;
    projection.dim1[Asa::kGridDim - 2] = 100;
    projection.dim1[Asa::kGridDim - 3] = 50;
    projection.dim2[Asa::kGridDim - 1] = 300;
    projection.dim2[Asa::kGridDim - 2] = 100;
    projection.dim2[Asa::kGridDim - 3] = 50;

    Solvers::Stylus::CoordinateSolver solver;
    solver.Process(frame);

    Require(stylus.runtime.tx1.coordinate.localGridCoor.valid,
            "bottom-right edge projection should produce a valid local coordinate");
    Require(stylus.runtime.tx1.coordinate.reportGlobalCoor.dim1 == 60 * Asa::kCoorUnit - 1 &&
            stylus.runtime.tx1.coordinate.reportGlobalCoor.dim2 == 40 * Asa::kCoorUnit - 1,
            "bottom-right anchored global coordinate should clamp to the physical sensor maximum");
}

void TestTiltProcessUsesTx2RefinedCoordinate() {
    Solvers::HeatmapFrame frame{};
    auto& stylus = frame.stylus;
    stylus.SnapshotBtInput(256, 1, true);
    stylus.runtime.rawGrid.asaGrid.tx2.valid = true;
    stylus.runtime.tx1.feature.peak.valid = true;
    stylus.runtime.tx1.feature.peakSignal = 320;
    stylus.runtime.tx2.feature.peakSignal = 777;
    SetProjectionPeak(stylus.runtime.tx1.feature.projection,
                      4, 4,
                      100, 300, 100,
                      120, 320, 120);
    stylus.runtime.rawGrid.asaGrid.tx1.anchorRow = 4;
    stylus.runtime.rawGrid.asaGrid.tx1.anchorCol = 4;
    stylus.runtime.rawGrid.asaGrid.tx2.anchorRow = 6;
    stylus.runtime.rawGrid.asaGrid.tx2.anchorCol = 5;
    stylus.runtime.tx2.feature.refinedLocalCoor = {1234, 2345, true};

    Solvers::Stylus::CoordinateSolver solver;
    Solvers::Stylus::TiltProcess tilt;
    solver.Process(frame);
    tilt.Process(frame);

    Require(stylus.runtime.tx2.coordinate.localGridCoor.valid,
            "tilt process should accept TX2 refined coordinate");
    Require(stylus.runtime.tx2.coordinate.localGridCoor.dim1 == 1234 &&
            stylus.runtime.tx2.coordinate.localGridCoor.dim2 == 2345,
            "tilt process should consume TX2 refined coordinate directly");
    Require(stylus.runtime.tx2.coordinate.reportGlobalCoor.dim1 == 1234 + Asa::kCoorUnit &&
            stylus.runtime.tx2.coordinate.reportGlobalCoor.dim2 == 2345 + 2 * Asa::kCoorUnit,
            "tilt process should map TX2 refined coordinate into anchored global grid space");
    Require(stylus.runtime.signal.signalY == 777,
            "coordinate solver should keep TX2 peak signal in the runtime signal block before tilt processing");
}

void TestTiltProcessClampsAnchoredCoordinatesBeforeTiltDiff() {
    Solvers::HeatmapFrame frame{};
    auto& stylus = frame.stylus;
    stylus.SnapshotBtInput(256, 1, true);
    stylus.runtime.rawGrid.asaGrid.tx2.valid = true;
    stylus.runtime.tx1.feature.peak.valid = true;
    stylus.runtime.tx1.feature.peakSignal = 320;
    stylus.runtime.tx2.feature.peakSignal = 320;
    stylus.runtime.rawGrid.asaGrid.tx1.anchorRow = 0;
    stylus.runtime.rawGrid.asaGrid.tx1.anchorCol = 0;
    stylus.runtime.rawGrid.asaGrid.tx2.anchorRow = 0;
    stylus.runtime.rawGrid.asaGrid.tx2.anchorCol = 0;

    auto& projection = stylus.runtime.tx1.feature.projection;
    projection.Clear();
    projection.peakIdxDim1 = 0;
    projection.peakIdxDim2 = 0;
    projection.dim1[0] = 300;
    projection.dim1[1] = 100;
    projection.dim1[2] = 50;
    projection.dim2[0] = 300;
    projection.dim2[1] = 100;
    projection.dim2[2] = 50;
    stylus.runtime.tx2.feature.refinedLocalCoor = Coor(0, 0);

    Solvers::Stylus::CoordinateSolver solver;
    Solvers::Stylus::TiltProcess tilt;
    solver.Process(frame);
    tilt.Process(frame);

    Require(stylus.runtime.tx2.coordinate.reportGlobalCoor.dim1 == 0 &&
            stylus.runtime.tx2.coordinate.reportGlobalCoor.dim2 == 0,
            "tilt process should clamp TX2 global coordinate into the physical sensor space");
    Require(stylus.runtime.tilt.diffDim1 == 0 &&
            stylus.runtime.tilt.diffDim2 == 0,
            "tilt diff should be computed from clamped global coordinates at the top-left edge");
}

void TestTiltProcessProducesTiltAndPostOutput() {
    Solvers::HeatmapFrame frame{};
    auto& stylus = frame.stylus;
    stylus.SnapshotBtInput(256, 1, true);
    stylus.runtime.rawGrid.asaGrid.tx2.valid = true;
    stylus.runtime.rawGrid.asaGrid.tx1.anchorRow = 4;
    stylus.runtime.rawGrid.asaGrid.tx1.anchorCol = 4;
    stylus.runtime.rawGrid.asaGrid.tx2.anchorRow = 4;
    stylus.runtime.rawGrid.asaGrid.tx2.anchorCol = 4;
    stylus.runtime.tx1.feature.peak.valid = true;
    stylus.runtime.tx1.feature.peakSignal = 420;
    stylus.runtime.tx2.feature.peakSignal = 280;
    SetProjectionPeak(stylus.runtime.tx1.feature.projection,
                      4, 4,
                      120, 320, 120,
                      120, 320, 120);

    Solvers::Stylus::CoordinateSolver solver;
    solver.Process(frame);
    Require(stylus.runtime.tx1.coordinate.localGridCoor.valid,
            "coordinate solver should produce a valid TX1 coordinate before tilt processing");

    stylus.runtime.tx2.feature.refinedLocalCoor =
        Coor(stylus.runtime.tx1.coordinate.localGridCoor.dim1 + 320,
             stylus.runtime.tx1.coordinate.localGridCoor.dim2 - 192);

    Solvers::Stylus::TiltProcess tilt;
    tilt.Process(frame);
    stylus.runtime.decision.inRangeCandidate = true;
    stylus.runtime.decision.tipDownCandidate = true;
    stylus.runtime.pressure.outputPressure = 256;

    Require(stylus.runtime.tilt.valid,
            "tilt process should mark tilt valid when TX1/TX2 coordinates exist");
    Require(stylus.runtime.tilt.preTiltDim1 != 0 || stylus.runtime.tilt.preTiltDim2 != 0,
            "tilt process should generate a non-zero pre-filter tilt for a shifted TX2 coordinate");
    Require(stylus.runtime.post.point.tiltValid,
            "post processor should propagate tilt validity to the output point");
    Require(stylus.runtime.post.point.tiltX == stylus.runtime.tilt.reportTiltDim1 &&
            stylus.runtime.post.point.tiltY == stylus.runtime.tilt.reportTiltDim2,
            "post processor should publish tilt output from the tilt runtime block");
    Require(stylus.runtime.post.point.tiltMagnitude > 0.0f,
            "post processor should compute a positive tilt magnitude for non-zero tilt");
}

void TestTiltProcessKeepsLastFrameWhenTx2Invalid() {
    Solvers::Stylus::CoordinateSolver solver;
    Solvers::Stylus::TiltProcess tilt;

    Solvers::HeatmapFrame first{};
    auto& firstStylus = first.stylus;
    firstStylus.SnapshotBtInput(256, 1, true);
    firstStylus.runtime.rawGrid.asaGrid.tx2.valid = true;
    firstStylus.runtime.rawGrid.asaGrid.tx1.anchorRow = 4;
    firstStylus.runtime.rawGrid.asaGrid.tx1.anchorCol = 4;
    firstStylus.runtime.rawGrid.asaGrid.tx2.anchorRow = 4;
    firstStylus.runtime.rawGrid.asaGrid.tx2.anchorCol = 4;
    firstStylus.runtime.tx1.feature.peak.valid = true;
    firstStylus.runtime.tx1.feature.peakSignal = 420;
    firstStylus.runtime.tx2.feature.peakSignal = 280;
    SetProjectionPeak(firstStylus.runtime.tx1.feature.projection,
                      4, 4,
                      120, 320, 120,
                      120, 320, 120);
    solver.Process(first);
    firstStylus.runtime.tx2.feature.refinedLocalCoor =
        Coor(firstStylus.runtime.tx1.coordinate.localGridCoor.dim1 + 320,
             firstStylus.runtime.tx1.coordinate.localGridCoor.dim2 - 192);
    tilt.Process(first);

    const int16_t prevPreTiltX = firstStylus.runtime.tilt.preTiltDim1;
    const int16_t prevPreTiltY = firstStylus.runtime.tilt.preTiltDim2;
    const int16_t prevTiltX = firstStylus.runtime.tilt.reportTiltDim1;
    const int16_t prevTiltY = firstStylus.runtime.tilt.reportTiltDim2;

    Solvers::HeatmapFrame second{};
    auto& secondStylus = second.stylus;
    secondStylus.SnapshotBtInput(256, 2, true);
    secondStylus.runtime.rawGrid.asaGrid.tx2.valid = false;
    secondStylus.runtime.rawGrid.asaGrid.tx1.anchorRow = 4;
    secondStylus.runtime.rawGrid.asaGrid.tx1.anchorCol = 4;
    secondStylus.runtime.tx1.feature.peak.valid = true;
    secondStylus.runtime.tx1.feature.peakSignal = 420;
    SetProjectionPeak(secondStylus.runtime.tx1.feature.projection,
                      4, 4,
                      120, 320, 120,
                      120, 320, 120);
    solver.Process(second);
    tilt.Process(second);

    Require(secondStylus.runtime.tilt.preTiltDim1 == prevPreTiltX &&
            secondStylus.runtime.tilt.preTiltDim2 == prevPreTiltY &&
            secondStylus.runtime.tilt.reportTiltDim1 == prevTiltX &&
            secondStylus.runtime.tilt.reportTiltDim2 == prevTiltY,
            "tilt process should keep the previous frame tilt when TX2 is invalid");
}

} // namespace

int main() {
    try {
        TestDefaultParametersMatchAsaTable();
        TestPressureResetReturnsRawAndStateZero();
        TestStraightLineEntryAndDragLimit();
        TestLargeDeviationExitsStraightLine();
        TestDirectionChangeExitsStraightLine();
        TestClampToSensorBounds();
        TestBypassModeResetsPostFilter();
        TestConfigRoundTrip();
        TestGridFeatureExtractorAlignsTx2WithFactoryFlow();
        TestTx2SeedThresholdIsGreaterThanNinetyNine();
        TestTx1LinePeakFirstFrameUsesStrongestCurrentPeak();
        TestTx1LinePeakHistoryCarriesPreviousSelection();
        TestTx1LinePeakHistoryResetsAfterInvalidParse();
        TestTiltJitterFilterMatchesTsacore();
        TestCoordinateSolverUsesFixedDevicePitchTables();
        TestCoordinateSolverClampsGlobalCoordinateAtTopLeftEdge();
        TestCoordinateSolverClampsGlobalCoordinateAtBottomRightEdge();
        TestTiltProcessUsesTx2RefinedCoordinate();
        TestTiltProcessClampsAnchoredCoordinatesBeforeTiltDiff();
        TestTiltProcessProducesTiltAndPostOutput();
        TestTiltProcessKeepsLastFrameWhenTx2Invalid();
        std::cout << "[TEST] Stylus linear filter process tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
