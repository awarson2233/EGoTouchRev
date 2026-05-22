#include "StylusSolver/CoordinateSolver.hpp"
#include "StylusSolver/CoorIIRProcess.hpp"
#include "StylusSolver/CoorSpeedProcess.hpp"
#include "StylusSolver/GridFeatureExtractor.hpp"
#include "StylusSolver/Hpp3NoisePostProcess.hpp"
#include "StylusSolver/Hpp3PostPressureProcess.hpp"
#include "StylusSolver/LinearFilterProcess.hpp"
#include "StylusSolver/TiltProcess.hpp"
#include "StylusSolver/StylusPipeline.h"

#include <array>
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

template <std::size_t N>
void WriteLe16(std::array<uint8_t, N>& bytes, std::size_t wordIndex, uint16_t value) {
    bytes[wordIndex * sizeof(uint16_t)] = static_cast<uint8_t>(value & 0xFFu);
    bytes[wordIndex * sizeof(uint16_t) + 1] = static_cast<uint8_t>(value >> 8);
}

void TestExtractGridFromSlavePayloadBytesMatchesWords() {
    static constexpr std::size_t wordCount = static_cast<std::size_t>(Asa::kBlockWords * 2);
    std::array<uint16_t, wordCount> words{};
    std::array<uint8_t, wordCount * sizeof(uint16_t)> bytes{};

    words[0] = 3;
    words[1] = 4;
    for (int r = 0; r < Asa::kGridDim; ++r) {
        for (int c = 0; c < Asa::kGridDim; ++c) {
            words[static_cast<std::size_t>(2 + r * Asa::kGridDim + c)] =
                static_cast<uint16_t>(100 + r * Asa::kGridDim + c);
        }
    }

    const std::size_t tx2Base = static_cast<std::size_t>(Asa::kBlockWords);
    words[tx2Base] = 7;
    words[tx2Base + 1] = 8;
    for (int r = 0; r < Asa::kGridDim; ++r) {
        for (int c = 0; c < Asa::kGridDim; ++c) {
            words[tx2Base + static_cast<std::size_t>(2 + r * Asa::kGridDim + c)] =
                static_cast<uint16_t>(1000 + r * Asa::kGridDim + c);
        }
    }

    for (std::size_t i = 0; i < words.size(); ++i) {
        WriteLe16(bytes, i, words[i]);
    }

    const Asa::AsaGridData fromWords = Asa::ExtractGridFromSlaveWords(words.data(), static_cast<int>(words.size()));
    const Asa::AsaGridData fromBytes = Asa::ExtractGridFromSlavePayloadBytes(bytes.data(), bytes.size());

    Require(fromBytes.tx1.valid == fromWords.tx1.valid &&
            fromBytes.tx2.valid == fromWords.tx2.valid,
            "byte and word grid extractors should agree on validity");
    Require(fromBytes.tx1.anchorRow == fromWords.tx1.anchorRow &&
            fromBytes.tx1.anchorCol == fromWords.tx1.anchorCol &&
            fromBytes.tx2.anchorRow == fromWords.tx2.anchorRow &&
            fromBytes.tx2.anchorCol == fromWords.tx2.anchorCol,
            "byte and word grid extractors should agree on anchors");

    for (int r = 0; r < Asa::kGridDim; ++r) {
        for (int c = 0; c < Asa::kGridDim; ++c) {
            Require(fromBytes.tx1.grid[r][c] == fromWords.tx1.grid[r][c] &&
                    fromBytes.tx2.grid[r][c] == fromWords.tx2.grid[r][c],
                    "byte and word grid extractors should agree on grid values");
        }
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

void SetBtPressure(Solvers::HeatmapFrame& frame, uint16_t pressure) {
    frame.stylus.input.btSample.hasSample = true;
    frame.stylus.input.btSample.pressure[3] = pressure;
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
    Solvers::Stylus::LinearFilterProcess filter;
    filter.m_enabled = true;
    Solvers::HeatmapFrame frame{};
    auto& stylus = frame.stylus;
    stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(1234, 2345);
    stylus.runtime.decision.inRangeCandidate = true;
    stylus.runtime.decision.tipDownCandidate = true;
    stylus.runtime.pressure.outputPressure = 300;

    filter.Process(frame);

    Require(stylus.runtime.post.finalCoor.dim1 == 1234 &&
            stylus.runtime.post.finalCoor.dim2 == 2345,
            "passthrough should copy raw coordinate to post output on first frame");
    Require(stylus.runtime.post.linearFilterState == 1,
            "single frame should advance state from 0 to 1");
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
    SetBtPressure(first, 10);
    extractor.Process(first);
    Require(first.stylus.runtime.tx1.feature.projection.peakIdxDim1 == 3,
            "history seed frame should select the initial dim1 peak");

    Solvers::HeatmapFrame second{};
    SetTx1LinePeakFrame(second, 3, 520, 6, 560);
    SetBtPressure(second, 10);
    extractor.Process(second);
    Require(second.stylus.runtime.tx1.feature.projection.peakIdxDim1 == 3,
            "TX1 line peak history should keep a nearby previous peak selected over a slightly stronger newcomer");
}

void TestTx1LinePeakHistoryDoesNotCarryDuringHover() {
    Solvers::Stylus::GridFeatureExtractor extractor;

    Solvers::HeatmapFrame first{};
    SetTx1LinePeakFrame(first, 3, 540, 6, 520);
    extractor.Process(first);
    Require(first.stylus.runtime.tx1.feature.projection.peakIdxDim1 == 3,
            "hover history seed frame should select the initial dim1 peak");

    Solvers::HeatmapFrame second{};
    SetTx1LinePeakFrame(second, 3, 520, 6, 560);
    extractor.Process(second);
    Require(second.stylus.runtime.tx1.feature.projection.peakIdxDim1 == 6,
            "hover TX1 line peak selection should follow the strongest current peak instead of history");
}

void TestTx1LinePeakHistoryResetsAfterInvalidParse() {
    Solvers::Stylus::GridFeatureExtractor extractor;

    Solvers::HeatmapFrame first{};
    SetTx1LinePeakFrame(first, 3, 540, 6, 520);
    SetBtPressure(first, 10);
    extractor.Process(first);

    Solvers::HeatmapFrame invalid{};
    extractor.Process(invalid);

    Solvers::HeatmapFrame second{};
    SetTx1LinePeakFrame(second, 3, 520, 6, 560);
    SetBtPressure(second, 10);
    extractor.Process(second);
    Require(second.stylus.runtime.tx1.feature.projection.peakIdxDim1 == 6,
            "invalid parse frame should clear TX1 line peak history before the next valid frame");
}

void TestPhysicalLowEdge3x3SumUsesInwardCells() {
    Solvers::HeatmapFrame frame{};
    auto& stylus = frame.stylus;
    stylus.runtime.parse.valid = true;
    stylus.runtime.rawGrid.asaGrid.tx1.valid = true;
    stylus.runtime.rawGrid.asaGrid.tx1.anchorRow = 10;
    stylus.runtime.rawGrid.asaGrid.tx1.anchorCol = 0;

    for (int r = 3; r <= 5; ++r) {
        stylus.runtime.rawGrid.asaGrid.tx1.grid[r][3] = 50;
        stylus.runtime.rawGrid.asaGrid.tx1.grid[r][4] = static_cast<int16_t>(r == 4 ? 300 : 250);
        stylus.runtime.rawGrid.asaGrid.tx1.grid[r][5] = 100;
        stylus.runtime.rawGrid.asaGrid.tx1.grid[r][6] = 70;
    }

    Solvers::Stylus::GridFeatureExtractor extractor;
    extractor.Process(frame);

    Require(stylus.runtime.tx1.feature.peak.valid,
            "physical low-edge fixture should produce a TX1 peak");
    Require(stylus.runtime.tx1.feature.peak.neighborSum3x3 == 1310,
            "physical low edge should sum inward columns [edge, edge+2], excluding the out-of-sensor side");
}

void TestCoordinateSolverDoesNotMarkLocalEdgeAsPhysicalEdge() {
    Solvers::HeatmapFrame frame{};
    auto& stylus = frame.stylus;
    stylus.runtime.tx1.feature.peak.valid = true;
    stylus.runtime.rawGrid.asaGrid.tx1.anchorRow = 10;
    stylus.runtime.rawGrid.asaGrid.tx1.anchorCol = 10;

    auto& projection = stylus.runtime.tx1.feature.projection;
    projection.Clear();
    projection.peakIdxDim1 = 0;
    projection.peakIdxDim2 = 4;
    projection.dim1[0] = 300;
    projection.dim1[1] = 100;
    projection.dim2[3] = 100;
    projection.dim2[4] = 300;
    projection.dim2[5] = 100;

    Solvers::Stylus::CoordinateSolver solver;
    solver.Process(frame);

    Require(stylus.runtime.tx1.coordinate.localGridCoor.valid,
            "local-window edge should still solve using missing-neighbor zeroes");
    Require(!stylus.runtime.signal.dim1EdgeActive,
            "local index 0 should not be reported as a physical sensor edge when anchor is interior");
}

void TestCoordinateSolverPublishesTx1AxisEdgeSignals() {
    Solvers::HeatmapFrame frame{};
    auto& stylus = frame.stylus;
    stylus.runtime.tx1.feature.peak.valid = true;
    stylus.runtime.tx1.feature.peakSignal = 4000;
    stylus.runtime.tx2.feature.peakSignal = 0;
    stylus.runtime.rawGrid.asaGrid.tx1.anchorRow = 0;
    stylus.runtime.rawGrid.asaGrid.tx1.anchorCol = 0;

    auto& projection = stylus.runtime.tx1.feature.projection;
    projection.Clear();
    projection.peakIdxDim1 = 4;
    projection.peakIdxDim2 = 4;
    projection.dim1[4] = 300;
    projection.dim1[5] = 100;
    projection.dim1[6] = 50;
    projection.dim2[4] = 300;
    projection.dim2[5] = 100;
    projection.dim2[6] = 50;
    stylus.runtime.tx1.feature.dim1SelectedPeakOnEdge = true;
    stylus.runtime.tx1.feature.dim2SelectedPeakOnEdge = true;
    stylus.runtime.tx1.feature.dim1SelectedPeakNetSignal = 2200;
    stylus.runtime.tx1.feature.dim2SelectedPeakNetSignal = 2300;

    Solvers::Stylus::CoordinateSolver solver;
    solver.Process(frame);

    Require(stylus.runtime.signal.signalY == 0,
            "TX2 peak signal should remain available separately for tilt");
    Require(stylus.runtime.signal.dim1EdgeSignal == 2200 &&
            stylus.runtime.signal.dim2EdgeSignal == 2300,
            "edge pressure signals should come from TX1 dim1/dim2 line peaks");

    Solvers::Stylus::Hpp3PostPressureProcess postPressure;
    auto& pressure = stylus.runtime.pressure;
    pressure.outputPressure = 200;
    pressure.rawPressure = 200;
    pressure.mappedPressure = 200;
    stylus.runtime.decision.tipDownCandidate = true;
    stylus.runtime.decision.authoritativeDown = true;
    postPressure.Process(frame);

    Require(stylus.runtime.pressure.outputPressure == 200,
            "low TX2 signal alone should not suppress edge pressure when TX1 axis signals are strong");
}

void TestNoisePostRejectsJumpBeforeTiltProcess() {
    Solvers::Stylus::Hpp3NoisePostProcess noise;
    Solvers::HeatmapFrame frame{};
    auto& runtime = frame.stylus.runtime;
    runtime.tx1.coordinate.reportGlobalCoor = Coor(10000, 10000);
    runtime.tx2.feature.refinedLocalCoor = Coor(0, 0);
    runtime.rawGrid.asaGrid.tx2.valid = true;
    runtime.rawGrid.asaGrid.tx2.anchorRow = 4;
    runtime.rawGrid.asaGrid.tx2.anchorCol = 4;
    runtime.signal.signalX = 1000;
    runtime.signal.signalY = 1000;

    noise.Process(frame);

    Require(runtime.post.noiseRejected,
            "noise post should reject TX1/TX2 jump before TiltProcess publishes TX2 coordinate");
    Require((runtime.post.noiseRejectReason & 4) != 0,
            "noise post should mark coordinate jump as the reject reason");
    Require(!runtime.tx2.coordinate.reportGlobalCoor.valid,
            "test setup should keep TX2 report coordinate unpublished before TiltProcess");
}

void TestAftCoorProcessScalesFactoryLockThresholds() {
    Solvers::Stylus::AftCoorProcess aft;
    Solvers::HeatmapFrame frame{};
    auto& stylus = frame.stylus;
    stylus.runtime.pressure.outputPressure = 100;
    stylus.runtime.post.finalCoor = Coor(100, 100);
    aft.Process(frame);

    stylus.runtime.post.finalCoor = Coor(120, 100);
    aft.Process(frame);
    Require(stylus.runtime.post.finalCoor.dim1 == 100,
            "scaled edge lock threshold should keep a 20-unit X move locked");

    stylus.runtime.post.finalCoor = Coor(130, 100);
    aft.Process(frame);
    Require(stylus.runtime.post.finalCoor.dim1 != 100,
            "scaled edge lock threshold should release once X movement exceeds about 24 units");
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

void TestCoordinateSolverUsesPhysicalTopLeftEdgeAtLocalCenter() {
    Solvers::HeatmapFrame frame{};
    auto& stylus = frame.stylus;
    stylus.runtime.tx1.feature.peak.valid = true;
    stylus.runtime.rawGrid.asaGrid.tx1.anchorRow = 0;
    stylus.runtime.rawGrid.asaGrid.tx1.anchorCol = 0;

    auto& projection = stylus.runtime.tx1.feature.projection;
    projection.Clear();
    projection.peakIdxDim1 = Asa::kGridDim / 2;
    projection.peakIdxDim2 = Asa::kGridDim / 2;
    projection.dim1[4] = 300;
    projection.dim1[5] = 100;
    projection.dim1[6] = 50;
    projection.dim2[4] = 300;
    projection.dim2[5] = 100;
    projection.dim2[6] = 50;
    stylus.runtime.tx1.feature.dim1SelectedPeakOnEdge = true;
    stylus.runtime.tx1.feature.dim2SelectedPeakOnEdge = true;
    stylus.runtime.tx1.feature.dim1SelectedPeakNetSignal = 300;
    stylus.runtime.tx1.feature.dim2SelectedPeakNetSignal = 300;

    Solvers::Stylus::CoordinateSolver solver;
    solver.Process(frame);

    Require(stylus.runtime.tx1.coordinate.localGridCoor.valid,
            "top-left physical edge at local center should produce a valid local coordinate");
    Require(stylus.runtime.tx1.coordinate.reportGlobalCoor.dim1 == 0 &&
            stylus.runtime.tx1.coordinate.reportGlobalCoor.dim2 == 0,
            "top-left physical edge at local center should clamp to zero");
    Require(stylus.runtime.signal.dim1EdgeActive && stylus.runtime.signal.dim2EdgeActive,
            "top-left physical edge at local center should mark both axes as edge-active");
}

void TestCoordinateSolverUsesPhysicalBottomRightEdgeAtLocalCenter() {
    Solvers::HeatmapFrame frame{};
    auto& stylus = frame.stylus;
    stylus.runtime.tx1.feature.peak.valid = true;
    stylus.runtime.rawGrid.asaGrid.tx1.anchorRow = 39;
    stylus.runtime.rawGrid.asaGrid.tx1.anchorCol = 59;

    auto& projection = stylus.runtime.tx1.feature.projection;
    projection.Clear();
    projection.peakIdxDim1 = Asa::kGridDim / 2;
    projection.peakIdxDim2 = Asa::kGridDim / 2;
    projection.dim1[4] = 300;
    projection.dim1[3] = 100;
    projection.dim1[2] = 50;
    projection.dim2[4] = 300;
    projection.dim2[3] = 100;
    projection.dim2[2] = 50;
    stylus.runtime.tx1.feature.dim1SelectedPeakOnEdge = true;
    stylus.runtime.tx1.feature.dim2SelectedPeakOnEdge = true;
    stylus.runtime.tx1.feature.dim1SelectedPeakNetSignal = 300;
    stylus.runtime.tx1.feature.dim2SelectedPeakNetSignal = 300;

    Solvers::Stylus::CoordinateSolver solver;
    solver.Process(frame);

    Require(stylus.runtime.tx1.coordinate.localGridCoor.valid,
            "bottom-right physical edge at local center should produce a valid local coordinate");
    Require(stylus.runtime.tx1.coordinate.reportGlobalCoor.dim1 == 60 * Asa::kCoorUnit - 1 &&
            stylus.runtime.tx1.coordinate.reportGlobalCoor.dim2 == 40 * Asa::kCoorUnit - 1,
            "bottom-right physical edge at local center should clamp to the physical sensor maximum");
    Require(stylus.runtime.signal.dim1EdgeActive && stylus.runtime.signal.dim2EdgeActive,
            "bottom-right physical edge at local center should mark both axes as edge-active");
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

    Require(secondStylus.runtime.post.point.tiltValid,
            "post point tilt should stay valid when TX2 drops and tilt is held");
    Require(secondStylus.runtime.post.point.tiltX == prevTiltX &&
            secondStylus.runtime.post.point.tiltY == prevTiltY,
            "post point tiltX/Y should match held report tilt when TX2 is invalid");
    Require(secondStylus.runtime.post.point.preTiltX == prevPreTiltX &&
            secondStylus.runtime.post.point.preTiltY == prevPreTiltY,
            "post point preTiltX/Y should match held pre tilt when TX2 is invalid");
}

void TestRawHistoryShiftAndLatestAtZero() {
    Solvers::Stylus::LinearFilterProcess filter;
    filter.m_enabled = true;

    Solvers::HeatmapFrame frame{};
    auto& stylus = frame.stylus;
    stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(1000, 2000);
    stylus.runtime.pressure.outputPressure = 300;

    filter.Process(frame);
    Require(stylus.runtime.post.postCoor.dim1 == 1000 &&
            stylus.runtime.post.postCoor.dim2 == 2000,
            "first frame postCoor should match raw input");
    Require(stylus.runtime.post.finalCoor.dim1 == 1000 &&
            stylus.runtime.post.finalCoor.dim2 == 2000,
            "first frame finalCoor should passthrough");

    // Second frame with different coor
    stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(1100, 2100);
    filter.Process(frame);
    Require(stylus.runtime.post.postCoor.dim1 == 1100 &&
            stylus.runtime.post.postCoor.dim2 == 2100,
            "second frame postCoor should match latest raw (history < 3 frames)");

    // Third frame — avg3 activates
    stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(1200, 2200);
    filter.Process(frame);
    const int32_t expectedDim1 = (1000 + 1100 + 1200) / 3;
    const int32_t expectedDim2 = (2000 + 2100 + 2200) / 3;
    Require(stylus.runtime.post.postCoor.dim1 == expectedDim1 &&
            stylus.runtime.post.postCoor.dim2 == expectedDim2,
            "third frame postCoor should be 3-point moving average");
}

void TestPenDownAnchorCapturedOnce() {
    Solvers::Stylus::LinearFilterProcess filter;
    filter.m_enabled = true;

    // First frame: pressure 0 → anchor should be captured when first non-0 pressure arrives
    Solvers::HeatmapFrame frame{};
    auto& stylus = frame.stylus;
    stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(1500, 2500);
    stylus.runtime.pressure.outputPressure = 100;
    filter.Process(frame);

    // Second frame: still pressure active, coordinate changed, anchor should NOT change
    stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(1600, 2600);
    stylus.runtime.pressure.outputPressure = 200;
    filter.Process(frame);

    // Third frame with a different coordinate
    stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(1700, 2700);
    stylus.runtime.pressure.outputPressure = 300;
    filter.Process(frame);

    // Anchor should be (1500, 2500) — the first coordinate when pressure first appeared
    // We verify indirectly: avg3 should use (1500, 1600, 1700)/3 = 1600 and (2500, 2600, 2700)/3 = 2600
    Require(stylus.runtime.post.postCoor.dim1 == 1600 &&
            stylus.runtime.post.postCoor.dim2 == 2600,
            "avg3 should reflect the three most recent raw coordinates in order");
}

void TestAvg3WarmUpPassthrough() {
    Solvers::Stylus::LinearFilterProcess filter;
    filter.m_enabled = true;

    Solvers::HeatmapFrame frame{};
    auto& stylus = frame.stylus;
    stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(800, 1800);
    stylus.runtime.pressure.outputPressure = 300;

    // Frame 1: history count = 1, less than 3
    filter.Process(frame);
    Require(stylus.runtime.post.postCoor.dim1 == 800 &&
            stylus.runtime.post.postCoor.dim2 == 1800,
            "warm-up frame 1 postCoor should passthrough raw");

    // Frame 2: history count = 2, still less than 3
    stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(880, 1880);
    filter.Process(frame);
    Require(stylus.runtime.post.postCoor.dim1 == 880 &&
            stylus.runtime.post.postCoor.dim2 == 1880,
            "warm-up frame 2 postCoor should passthrough raw");
}

void TestAvg3ThreePointAverage() {
    Solvers::Stylus::LinearFilterProcess filter;
    filter.m_enabled = true;

    Solvers::HeatmapFrame frame{};
    auto& stylus = frame.stylus;

    // Feed 3 frames with known values
    stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(300, 900);
    stylus.runtime.pressure.outputPressure = 300;
    filter.Process(frame);

    stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(600, 1200);
    filter.Process(frame);

    // On 3rd frame, avg3 = (300+600+900)/3 = 600, (900+1200+1500)/3 = 1200
    stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(900, 1500);
    filter.Process(frame);
    Require(stylus.runtime.post.postCoor.dim1 == 600 &&
            stylus.runtime.post.postCoor.dim2 == 1200,
            "3-point average should be (x0+x1+x2)/3");

    // 4th frame: avg3 = (600+900+1000)/3, (1200+1500+1600)/3
    stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(1000, 1600);
    filter.Process(frame);
    const int32_t expectedAvgX = (600 + 900 + 1000) / 3;
    const int32_t expectedAvgY = (1200 + 1500 + 1600) / 3;
    Require(stylus.runtime.post.postCoor.dim1 == expectedAvgX &&
            stylus.runtime.post.postCoor.dim2 == expectedAvgY,
            "sliding 3-point average should shift with each new frame");
}

void TestPredictedCoorQuadraticExtrapolation() {
    Solvers::Stylus::LinearFilterProcess filter;
    filter.m_enabled = true;

    Solvers::HeatmapFrame frame{};
    auto& stylus = frame.stylus;

    // Feed 3 frames: (100,200), (110,220), (120,240) — linear movement
    stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(100, 200);
    stylus.runtime.pressure.outputPressure = 300;
    filter.Process(frame);

    stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(110, 220);
    filter.Process(frame);

    stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(120, 240);
    filter.Process(frame);

    // predicted = 3*x0 - 3*x1 + x2 = 3*120 - 3*110 + 100 = 360 - 330 + 100 = 130
    // predicted = 3*y0 - 3*y1 + y2 = 3*240 - 3*220 + 200 = 720 - 660 + 200 = 260
    Require(stylus.runtime.post.predictedCoor.valid,
            "predicted coordinate should be valid after 3 frames");
    Require(stylus.runtime.post.predictedCoor.dim1 == 130 &&
            stylus.runtime.post.predictedCoor.dim2 == 260,
            "predicted = 3*x0 - 3*x1 + x2 (quadratic extrapolation)");

    // 4th frame: (135, 270)
    stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(135, 270);
    filter.Process(frame);
    // predicted = 3*135 - 3*120 + 110 = 405 - 360 + 110 = 155
    Require(stylus.runtime.post.predictedCoor.dim1 == 155 &&
            stylus.runtime.post.predictedCoor.dim2 == 310,
            "predicted should update with each new frame");
}

void TestLinearCorrectionUsesRawInputUntilLineConstraintApplies() {
    Solvers::Stylus::LinearFilterProcess filter;
    filter.m_enabled = true;
    ConfigureFastTestFilter(filter);

    Solvers::HeatmapFrame frame{};
    auto& stylus = frame.stylus;

    // Feed raw points with jitter: raw X alternates ±10 around a straight line
    stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(1000, 2000);
    stylus.runtime.pressure.outputPressure = 300;
    filter.Process(frame);

    stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(1080, 2000);
    filter.Process(frame);

    stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(1010, 2010);
    filter.Process(frame);

    // After 3 frames, avg3 smooths the jitter
    // avg3[0] = (1000+1080+1010)/3 = 1030
    Require(stylus.runtime.post.postCoor.dim1 == 1030,
            "postCoor should be the 3-point average of jittery raw points");

    // Before the line-fit state starts constraining points, finalCoor should still
    // follow the current raw coordinate rather than the avg3 side-channel value.
    Require(stylus.runtime.post.finalValid,
            "finalCoor should be valid when processing avg3 input");
    Require(stylus.runtime.post.finalCoor.dim1 == 1010 &&
            stylus.runtime.post.finalCoor.dim2 == 2010,
            "finalCoor should still track the raw input before any line correction applies");
}

void TestHoverPathKeepsAvg3OutOfFinalCoor() {
    Solvers::Stylus::LinearFilterProcess filter;
    filter.m_enabled = true;

    Solvers::HeatmapFrame frame{};
    auto& stylus = frame.stylus;
    stylus.runtime.pressure.outputPressure = 0;

    stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(1000, 2000);
    filter.Process(frame);

    stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(1100, 2100);
    filter.Process(frame);

    stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(1200, 2200);
    filter.Process(frame);

    Require(stylus.runtime.post.postCoor.dim1 == 1100 &&
            stylus.runtime.post.postCoor.dim2 == 2100,
            "avg3 diagnostic path should still produce a 3-point average in hover");
    Require(stylus.runtime.post.finalCoor.dim1 == 1200 &&
            stylus.runtime.post.finalCoor.dim2 == 2200,
            "hover finalCoor should bypass avg3 and keep the latest raw coordinate");
}

void TestPressureInactiveResetsHistoryState() {
    Solvers::Stylus::LinearFilterProcess filter;
    filter.m_enabled = true;

    Solvers::HeatmapFrame frame{};
    auto& stylus = frame.stylus;

    // Feed 4 frames to build up history and avg3
    for (int i = 0; i < 4; ++i) {
        stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(500 + i * 10, 1500 + i * 10);
        stylus.runtime.pressure.outputPressure = 300;
        filter.Process(frame);
    }

    Require(stylus.runtime.post.finalValid,
            "output should be valid with active pressure");

    // Now set pressure to 0 — should reset history
    stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(999, 1999);
    stylus.runtime.pressure.outputPressure = 0;
    filter.Process(frame);

    Require(stylus.runtime.post.finalCoor.dim1 == 999 &&
            stylus.runtime.post.finalCoor.dim2 == 1999,
            "pressure inactive should passthrough raw coordinate");
    Require(stylus.runtime.post.linearFilterState == 0,
            "linear filter state should reset to 0 on pressure inactive");

    // Resume pressure — final output should still start from the raw path even though
    // the avg3 diagnostic history remains live across the pressure gap.
    stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(700, 1700);
    stylus.runtime.pressure.outputPressure = 300;
    filter.Process(frame);
    Require(stylus.runtime.post.finalCoor.dim1 == 700 &&
            stylus.runtime.post.finalCoor.dim2 == 1700,
            "first pressured frame after resume should still follow the raw input");
}

void TestIirCoefSelectionUsesRawMappedHistorySpeed() {
    Solvers::Stylus::CoorSpeedProcess speed;
    Solvers::Stylus::CoorIIRProcess iir;

    Solvers::HeatmapFrame frame{};
    auto& stylus = frame.stylus;

    const int32_t rawX[3] = {1000, 1200, 1400};
    for (int i = 0; i < 3; ++i) {
        stylus.runtime.tx1.coordinate.reportGlobalCoor = Coor(rawX[i], 2000);
        stylus.runtime.post.finalCoor = Coor(1000, 2000);
        stylus.runtime.post.finalValid = true;

        speed.Process(frame);
        iir.Process(frame);
    }

    Require(stylus.runtime.post.speedValue > 0,
            "speed path should observe movement from raw mapped history");
    Require(iir.m_currentCoef > iir.m_coefLowInBand,
            "IIR coefficient selection should react to raw-history speed, not a frozen finalCoor");
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
        TestExtractGridFromSlavePayloadBytesMatchesWords();
        TestGridFeatureExtractorAlignsTx2WithFactoryFlow();
        TestTx2SeedThresholdIsGreaterThanNinetyNine();
        TestTx1LinePeakFirstFrameUsesStrongestCurrentPeak();
        TestTx1LinePeakHistoryCarriesPreviousSelection();
        TestTx1LinePeakHistoryDoesNotCarryDuringHover();
        TestTx1LinePeakHistoryResetsAfterInvalidParse();
        TestPhysicalLowEdge3x3SumUsesInwardCells();
        TestCoordinateSolverDoesNotMarkLocalEdgeAsPhysicalEdge();
        TestCoordinateSolverPublishesTx1AxisEdgeSignals();
        TestNoisePostRejectsJumpBeforeTiltProcess();
        TestAftCoorProcessScalesFactoryLockThresholds();
        TestTiltJitterFilterMatchesTsacore();
        TestCoordinateSolverUsesFixedDevicePitchTables();
        TestCoordinateSolverClampsGlobalCoordinateAtTopLeftEdge();
        TestCoordinateSolverClampsGlobalCoordinateAtBottomRightEdge();
        TestCoordinateSolverUsesPhysicalTopLeftEdgeAtLocalCenter();
        TestCoordinateSolverUsesPhysicalBottomRightEdgeAtLocalCenter();
        TestTiltProcessUsesTx2RefinedCoordinate();
        TestTiltProcessClampsAnchoredCoordinatesBeforeTiltDiff();
        TestTiltProcessProducesTiltAndPostOutput();
        TestTiltProcessKeepsLastFrameWhenTx2Invalid();
        TestRawHistoryShiftAndLatestAtZero();
        TestPenDownAnchorCapturedOnce();
        TestAvg3WarmUpPassthrough();
        TestAvg3ThreePointAverage();
        TestPredictedCoorQuadraticExtrapolation();
        TestLinearCorrectionUsesRawInputUntilLineConstraintApplies();
        TestHoverPathKeepsAvg3OutOfFinalCoor();
        TestPressureInactiveResetsHistoryState();
        TestIirCoefSelectionUsesRawMappedHistorySpeed();
        std::cout << "[TEST] Stylus linear filter process tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
