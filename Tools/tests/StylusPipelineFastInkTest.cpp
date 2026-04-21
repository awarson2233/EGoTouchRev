#include "StylusSolver/NoPressInkGate.hpp"
#include "StylusSolver/PressureSolver.hpp"
#include "StylusSolver/BtPressBuffer.hpp"
#include "StylusSolver/FakePressureDecay.hpp"
#include "StylusSolver/GridPeakDetector.hpp"
#include "StylusSolver/StylusPipeline.h"

#include <algorithm>
#include <atomic>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

using Asa::EdgeSignalInputs;
using Asa::GridPeakDetector;
using Asa::NoPressInkGate;
using Asa::PressureSolver;
using Solvers::HeatmapFrame;
using Solvers::StylusFrameData;
using Solvers::StylusPipeline;

constexpr size_t kMasterBytes = 5063;
constexpr size_t kSlaveHeaderBytes = 7;
constexpr size_t kSlaveWordCount = 166;
constexpr size_t kSlaveFrameBytes = kSlaveHeaderBytes + kSlaveWordCount * 2;
constexpr int kGridDim = 9;

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void RequireNear(float actual, float expected, float epsilon, const char* message) {
    if (std::fabs(actual - expected) > epsilon) {
        throw std::runtime_error(message);
    }
}

std::array<uint16_t, kGridDim * kGridDim> MakeCrossGrid(
    uint16_t center, uint16_t nearAxis, uint16_t diag, uint16_t farAxis,
    int peakRow = 4, int peakCol = 4) {
    std::array<uint16_t, kGridDim * kGridDim> grid{};
    auto set = [&](int r, int c, uint16_t v) {
        if (r >= 0 && r < kGridDim && c >= 0 && c < kGridDim) {
            grid[static_cast<size_t>(r * kGridDim + c)] = v;
        }
    };

    set(peakRow, peakCol, center);
    set(peakRow - 1, peakCol, nearAxis);
    set(peakRow + 1, peakCol, nearAxis);
    set(peakRow, peakCol - 1, nearAxis);
    set(peakRow, peakCol + 1, nearAxis);
    set(peakRow - 1, peakCol - 1, diag);
    set(peakRow - 1, peakCol + 1, diag);
    set(peakRow + 1, peakCol - 1, diag);
    set(peakRow + 1, peakCol + 1, diag);
    set(peakRow - 2, peakCol, farAxis);
    set(peakRow + 2, peakCol, farAxis);
    set(peakRow, peakCol - 2, farAxis);
    set(peakRow, peakCol + 2, farAxis);
    return grid;
}

std::vector<uint8_t> BuildCombinedStylusFrame(
    uint16_t tx1AnchorRow,
    uint16_t tx1AnchorCol,
    const std::array<uint16_t, kGridDim * kGridDim>& tx1Grid,
    uint16_t tx2AnchorRow = 0,
    uint16_t tx2AnchorCol = 0,
    const std::array<uint16_t, kGridDim * kGridDim>& tx2Grid = {}) {
    std::vector<uint8_t> raw(kMasterBytes + kSlaveFrameBytes, 0);
    auto writeWord = [&](size_t wordIndex, uint16_t value) {
        const size_t off = kMasterBytes + kSlaveHeaderBytes + wordIndex * 2;
        raw[off] = static_cast<uint8_t>(value & 0xFFu);
        raw[off + 1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
    };

    writeWord(0, tx1AnchorRow);
    writeWord(1, tx1AnchorCol);
    for (size_t i = 0; i < tx1Grid.size(); ++i) {
        writeWord(2 + i, tx1Grid[i]);
    }

    writeWord(83, tx2AnchorRow);
    writeWord(84, tx2AnchorCol);
    for (size_t i = 0; i < tx2Grid.size(); ++i) {
        writeWord(85 + i, tx2Grid[i]);
    }
    return raw;
}

StylusFrameData RunFrame(StylusPipeline& pipeline,
                         const std::vector<uint8_t>& raw,
                         uint16_t btPressure,
                         uint64_t timestamp = 0,
                         bool pushBtPressure = true) {
    HeatmapFrame frame;
    frame.rawPtr = raw.data();
    frame.rawLen = raw.size();
    frame.timestamp = timestamp;
    if (pushBtPressure) {
        pipeline.SetBtMcuPressure(btPressure);
    }
    pipeline.Process(frame);
    return frame.stylus;
}

void LoadFromSavedText(StylusPipeline& pipeline, const std::string& saved) {
    std::istringstream in(saved);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        const size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        pipeline.LoadConfig(line.substr(0, eq), line.substr(eq + 1));
    }
}

void TestPressureStageSignalSuppressHysteresis() {
    PressureSolver solver;
    solver.signalSuppressEnabled = true;
    solver.signalSuppressEnter = 2200;
    solver.signalSuppressExit = 3200;
    solver.edgeSignalSuppressEnabled = false;

    const auto s0 = solver.SolveStage(180, true, 1800, false);
    Require(s0.mappedPressure > 0, "mapped pressure should be produced");
    Require(s0.realPressure > 0, "real pressure should be produced");
    Require(!s0.signalSuppressActive, "signal suppression should stay disabled");

    const auto s1 = solver.SolveStage(180, true, 2500, false);
    Require(s1.realPressure > 0, "real pressure should remain unsuppressed");
    Require(!s1.signalSuppressActive, "signal suppression should remain disabled");

    const auto s2 = solver.SolveStage(180, true, 3500, false);
    Require(s2.realPressure > 0, "pressure should remain valid");
    Require(!s2.signalSuppressActive, "suppression should stay disabled");
}

void TestPressureStageEdgeSuppressHysteresis() {
    PressureSolver solver;
    solver.signalSuppressEnabled = false;
    solver.edgeSignalSuppressEnabled = true;
    solver.edgeSignalSuppressEnter = 1500;
    solver.edgeSignalSuppressExit = 3000;

    EdgeSignalInputs edge{};
    edge.dim1Active = true;
    edge.dim1Signal = 1400;
    const auto s0 = solver.SolveStage(180, true, 5000, true, edge);
    Require(s0.realPressure > 0, "edge path should no longer suppress pressure");
    Require(!s0.edgeSignalSuppressActive, "edge suppression should stay disabled");

    edge.dim1Signal = 1800;
    const auto s1 = solver.SolveStage(180, true, 5000, true, edge);
    Require(s1.realPressure >= 0, "edge path should remain valid");

    edge.dim1Signal = 3201;
    const auto s2 = solver.SolveStage(180, true, 5000, true, edge);
    Require(s2.realPressure >= 0, "edge pressure should remain valid");
    Require(!s2.edgeSignalSuppressActive, "edge suppression should stay disabled");
}

void TestPressureStagePredictsBetweenRealSamples() {
    PressureSolver solver;
    Asa::BtPressureSample real1{200, 1, true};
    const auto s1 = solver.SolveStage(real1, true);
    Require(s1.isRealMeasurement, "first sample should be marked real");
    Require(s1.btSeq == 1, "first sample should carry seq 1");
    Require(s1.predictedAgeFrames == 0, "real sample should reset predicted age");

    const auto s2 = solver.SolveStage(real1, true);
    Require(!s2.isRealMeasurement, "reused snapshot should become predicted frame");
    Require(s2.btSeq == 1, "predicted frame should keep last seq");
    Require(s2.predictedAgeFrames == 1, "predicted age should increment");

    Asa::BtPressureSample real2{260, 2, true};
    const auto s3 = solver.SolveStage(real2, true);
    Require(s3.isRealMeasurement, "new seq should become real measurement");
    Require(s3.btSeq == 2, "new real sample should update seq");
    Require(s3.predictedAgeFrames == 0, "new real sample should clear predicted age");
    Require(solver.GetHistoryCount() >= 3, "solver should retain short pressure history");
}

void TestPressureStageRealZeroDropsImmediately() {
    PressureSolver solver;
    const auto down = solver.SolveStage(Asa::BtPressureSample{240, 1, true}, true);
    Require(down.realPressure > 0, "non-zero real sample should produce pressure");

    const auto up = solver.SolveStage(Asa::BtPressureSample{0, 2, true}, true);
    Require(up.isRealMeasurement, "zero sample with new seq should be real");
    Require(up.realPressure == 0, "real zero sample should force immediate zero output");
    Require(up.predictedAgeFrames == 0, "real zero sample should not be treated as predicted");
}

void TestBtPressBufferDefaultMode() {
    Asa::BtPressBuffer buf;
    Require(buf.Resolve(0, 0) == 0, "empty buffer should return zero");
    buf.Push(100);
    buf.Push(200);
    buf.Push(300);
    buf.Push(400);
    Require(buf.Resolve(0, 0) == 400, "default mode should return latest slot");
    Require(buf.Resolve(5, 0) == 400, "default mode ignores frame count");
}

void TestBtPressBufferOnCellMapping() {
    Asa::BtPressBuffer buf;
    buf.Push(10);
    buf.Push(20);
    buf.Push(30);
    buf.Push(40);
    Require(buf.Resolve(0, 1) == 40, "OnCell mode should return latest slot");
    Require(buf.Resolve(1, 1) == 40, "OnCell mode should ignore frame mapping");
    Require(buf.Resolve(5, 1) == 40, "OnCell mode should always return latest slot");
    Require(buf.Resolve(6, 1) == 40, "OnCell mode should remain latest slot");
}

void TestBtPressBufferInCellMapping() {
    Asa::BtPressBuffer buf;
    buf.Push(10);
    buf.Push(20);
    buf.Push(30);
    buf.Push(40);
    Require(buf.Resolve(0, 2) == 40, "InCell mode should return latest slot");
    Require(buf.Resolve(1, 2) == 40, "InCell mode should ignore frame mapping");
    Require(buf.Resolve(2, 2) == 40, "InCell mode should stay latest slot");
    Require(buf.Resolve(3, 2) == 40, "InCell mode should stay latest slot");
    Require(buf.Resolve(4, 2) == 40, "InCell mode should remain latest slot");
}

void TestBtPressBufferReset() {
    Asa::BtPressBuffer buf;
    buf.Push(100);
    Require(buf.Resolve(0, 0) == 100, "push should store value");
    buf.Reset();
    Require(buf.Resolve(0, 0) == 0, "reset should clear buffer");
}

void TestBtPressBufferSeqSnapshot() {
    Asa::BtPressBuffer buf;
    const auto empty = buf.ReadLatest();
    Require(!empty.hasSample, "empty snapshot should report no sample");

    buf.Push(111);
    const auto s1 = buf.ReadLatest();
    Require(s1.hasSample, "snapshot should report sample after push");
    Require(s1.pressure == 111, "snapshot should carry pushed pressure");
    Require(s1.seq == 1, "first push should advance seq to 1");

    buf.Push(222);
    const auto s2 = buf.ReadLatest();
    Require(s2.pressure == 222, "latest snapshot should track latest pressure");
    Require(s2.seq == 2, "second push should advance seq to 2");
}

void TestFakePressureDecayBasic() {
    Asa::FakePressureDecay decay;
    Require(!decay.IsActive(), "initial state should not be active");
    decay.Init();
    Require(decay.IsActive(), "after init should be active with AddNum=2");
    uint16_t p1 = decay.Step(600);
    Require(p1 == 400, "step1: 2*600/3 = 400");
    Require(decay.IsActive(), "still active after step1");
    uint16_t p2 = decay.Step(p1);
    Require(p2 == 200, "step2: 1*400/2 = 200");
    Require(!decay.IsActive(), "not active after step2 (AddNum=0)");
    uint16_t p3 = decay.Step(p2);
    Require(p3 == 0, "step3 after inactive should return 0");
}

void TestFakePressureDecayReset() {
    Asa::FakePressureDecay decay;
    decay.Init();
    Require(decay.IsActive(), "should be active after init");
    decay.Reset();
    Require(!decay.IsActive(), "should not be active after reset");
    decay.Init();
    Require(decay.IsActive(), "should re-init after reset");
}

void TestNoPressInkGateEnterAndExit() {
    NoPressInkGate gate;
    gate.enabled = true;

    const auto r0 = gate.Apply(true, true, false, 0, 10000, 0);
    Require(!r0.active, "no-press gate should stay inactive");

    const auto r1 = gate.Apply(true, true, false, 0, 10000, 0);
    Require(!r1.active, "no-press gate should remain inactive");

    const auto r2 = gate.Apply(true, true, false, 120, 20000, 0);
    Require(!r2.active, "real pressure should not enable no-press gate");
}

void TestStylusPipelineNoPressSyntheticPressure() {
    StylusPipeline pipeline;
    pipeline.LoadConfig("sp.noPressEnabled", "1");

    const auto tx1Grid = MakeCrossGrid(16000, 14000, 12000, 10000);
    const auto tx2Grid = MakeCrossGrid(6000, 5000, 4000, 3000);
    const auto raw = BuildCombinedStylusFrame(10, 10, tx1Grid, 10, 10, tx2Grid);

    const auto f0 = RunFrame(pipeline, raw, 0, 8);
    Require(f0.point.valid, "strong zero-pressure frame should produce valid coordinates");
    Require(f0.pressure == 0, "zero BT pressure should produce zero final pressure");
    Require(!f0.noPressInkActive, "no-press ink should stay inactive");

    const auto f1 = RunFrame(pipeline, raw, 64, 16);
    Require(f1.pressure >= 0, "pressure should remain non-negative");
    Require(!f1.noPressInkActive, "no-press ink should remain inactive");
    Require(!f1.sustainOutput, "sustain output should remain disabled");
    Require(!f1.fastLiftOutput, "fast-lift output should remain disabled");
}

void TestStylusPipelineLowSignalKeepsCoordinateButZeroPressureUntilAuthoritativeDown() {
    StylusPipeline pipeline;

    const auto weakGrid = MakeCrossGrid(1800, 1400, 900, 600);
    const auto raw = BuildCombinedStylusFrame(12, 12, weakGrid, 12, 12, {});

    const auto frame = RunFrame(pipeline, raw, 180, 8);
    Require(frame.point.valid, "weak-signal frame should still solve coordinates");
    Require(frame.pressure == 0, "weak-signal frame should not output pressure before authoritative down");
    Require(!frame.tipSwitchActive, "weak-signal frame should remain out of writing state");
    Require(!frame.noPressInkActive, "no-press ink should stay inactive");
}

void TestStylusPipelineFastLiftToNoSignal() {
    StylusPipeline pipeline;
    pipeline.LoadConfig("sp.noPressEnabled", "0");
    pipeline.LoadConfig("sp.sigSuppressEnabled", "0");
    pipeline.LoadConfig("sp.filterMode", "2");

    const auto interiorGrid = MakeCrossGrid(12000, 9000, 7000, 5000, 4, 4);
    const auto rawInterior = BuildCombinedStylusFrame(10, 10, interiorGrid, 10, 10, {});

    const auto down = RunFrame(pipeline, rawInterior, 180, 8);
    Require(down.point.valid, "pen-down frame should be valid");
    Require(down.pressure > 0, "pen-down frame should report pressure");
    Require(down.tipSwitchActive, "pen-down frame should assert tip switch");
    Require(down.animState == 2, "pen-down frame should be in writing state");

    std::this_thread::sleep_for(std::chrono::milliseconds(35));
    const auto lift = RunFrame(pipeline, BuildCombinedStylusFrame(0x00FF, 0x00FF, {}, 0, 0, {}), 0, 16);
    Require(!lift.point.valid, "no-signal lift frame should not keep a valid point");
    Require(lift.pressure == 0, "no-signal lift frame should drop pressure");
    Require(!lift.tipSwitchActive, "no-signal lift frame should clear tip switch");
    Require(!lift.noPressInkActive, "no-signal lift frame should keep no-press inactive");
}

void TestStylusPipelineReleaseRetainsPreviousOutput() {
    StylusPipeline pipeline;
    pipeline.LoadConfig("sp.noPressEnabled", "0");
    pipeline.LoadConfig("sp.sigSuppressEnabled", "0");
    pipeline.LoadConfig("sp.filterMode", "2");
    pipeline.LoadConfig("sp.tx1InkEnterTh", "9000");
    pipeline.LoadConfig("sp.tx1LiftSuspiciousTh", "7000");
    pipeline.LoadConfig("sp.tx1LiftAbsoluteTh", "4500");

    const auto raw = BuildCombinedStylusFrame(10, 10,
        MakeCrossGrid(12000, 9000, 7000, 5000, 4, 4), 10, 10, {});
    const auto suspiciousRaw = BuildCombinedStylusFrame(10, 10,
        MakeCrossGrid(6500, 6500, 6400, 6300, 4, 4), 10, 10, {});
    const auto down = RunFrame(pipeline, raw, 220, 8);
    Require(down.point.valid, "precondition should produce a valid down frame");
    Require(down.pressure > 0, "precondition should carry pressure");

    const auto weak = RunFrame(pipeline, suspiciousRaw, 0, 16);
    Require(!weak.fastLiftOutput, "fast-lift output should stay disabled");
    Require(!weak.sustainOutput, "sustain output should stay disabled");
    Require(weak.pressure == down.pressure, "suspicious frame should reuse previous committed pressure");
    Require(weak.tipSwitchActive == down.tipSwitchActive, "suspicious frame should reuse previous tip state");
}

void TestStylusPipelineEdgeFastLiftRetainsCoordinate() {
    StylusPipeline pipeline;
    pipeline.LoadConfig("sp.noPressEnabled", "0");
    pipeline.LoadConfig("sp.sigSuppressEnabled", "0");
    pipeline.LoadConfig("sp.filterMode", "2");

    const auto edgeGrid = MakeCrossGrid(15000, 12000, 9000, 6000, 0, 0);
    const auto raw = BuildCombinedStylusFrame(0, 0, edgeGrid, 0, 0, {});
    const auto down = RunFrame(pipeline, raw, 220, 8);
    Require(down.point.valid, "edge frame should still solve a valid point");

    const auto weak = RunFrame(pipeline, raw, 0, 16);
    Require(!weak.fastLiftOutput, "edge weak-release should not enter fast-lift output");
    Require(weak.pressure == 0, "edge weak-release should output zero pressure with zero BT input");
}

void TestStylusPipelineConfigRoundTrip() {
    StylusPipeline pipeline;
    pipeline.LoadConfig("sp.recheckThBase", "888");
    pipeline.LoadConfig("sp.recheckThMulti", "1333");
    pipeline.LoadConfig("sp.tx1InkEnterTh", "9000");
    pipeline.LoadConfig("sp.tx1LiftSuspiciousTh", "7000");
    pipeline.LoadConfig("sp.tx1LiftAbsoluteTh", "4500");
    pipeline.LoadConfig("sp.edgeSigSuppressEnabled", "1");
    pipeline.LoadConfig("sp.edgeSigSuppressEnter", "1444");
    pipeline.LoadConfig("sp.edgeSigSuppressExit", "3555");
    pipeline.LoadConfig("sp.noPressEnabled", "1");
    pipeline.LoadConfig("sp.noPressBaseTh", "9999");
    pipeline.LoadConfig("sp.noPressEnterRatio", "95");
    pipeline.LoadConfig("sp.noPressExitRatio", "28");
    pipeline.LoadConfig("sp.noPressTiltDeadzone", "901");
    pipeline.LoadConfig("sp.noPressTiltCap", "12345");
    pipeline.LoadConfig("sp.noPressTiltScale", "33");
    pipeline.LoadConfig("sp.noPressDebounceEnter", "3");
    pipeline.LoadConfig("sp.noPressDebounceExit", "4");
    pipeline.LoadConfig("sp.noPressSyntheticMin", "12");
    pipeline.LoadConfig("sp.btMapMode", "2");
    pipeline.LoadConfig("sp.sigSuppressEnabled", "1");
    pipeline.LoadConfig("sp.sigSuppressEnter", "2222");
    pipeline.LoadConfig("sp.sigSuppressExit", "3333");
    pipeline.LoadConfig("sp.pressIirQ8", "77");

    std::ostringstream out;
    pipeline.SaveConfig(out);
    const std::string saved = out.str();

    Require(saved.find("sp.recheckThBase=888") != std::string::npos,
            "saved config should include recheck base threshold");
    Require(saved.find("sp.recheckThMulti=1333") != std::string::npos,
            "saved config should include recheck multi threshold");
    Require(saved.find("sp.tx1InkEnterTh=9000") != std::string::npos,
            "saved config should include tx1 ink enter threshold");
    Require(saved.find("sp.tx1LiftSuspiciousTh=7000") != std::string::npos,
            "saved config should include tx1 suspicious lift threshold");
    Require(saved.find("sp.tx1LiftAbsoluteTh=4500") != std::string::npos,
            "saved config should include tx1 absolute lift threshold");
    Require(saved.find("sp.edgeSigSuppressEnter=") == std::string::npos,
            "saved config should not include deprecated edge suppress keys");
    Require(saved.find("sp.noPressBaseTh=") == std::string::npos,
            "saved config should not include deprecated no-press keys");
    Require(saved.find("sp.btMapMode=") == std::string::npos,
            "saved config should not include deprecated bt map mode key");
    Require(saved.find("sp.pressIirQ8=") == std::string::npos,
            "saved config should not include deprecated pressure iir key");

    StylusPipeline loaded;
    LoadFromSavedText(loaded, saved);

    std::ostringstream outLoaded;
    loaded.SaveConfig(outLoaded);
    const std::string savedLoaded = outLoaded.str();
    Require(savedLoaded.find("sp.recheckThBase=888") != std::string::npos,
            "loaded config should preserve recheck base threshold");
    Require(savedLoaded.find("sp.edgeSigSuppressExit=") == std::string::npos,
            "loaded config should not emit deprecated edge suppress keys");
    Require(savedLoaded.find("sp.noPressSyntheticMin=") == std::string::npos,
            "loaded config should not emit deprecated no-press keys");
    Require(savedLoaded.find("sp.sigSuppressEnabled=") == std::string::npos,
            "loaded config should not emit deprecated suppress keys");
    Require(savedLoaded.find("sp.pressIirQ8=") == std::string::npos,
            "loaded config should not emit deprecated pressure iir key");

    const auto schema = loaded.GetConfigSchema();
    auto hasKey = [&](const char* key) {
        for (const auto& param : schema) {
            if (param.key == key) return true;
        }
        return false;
    };
    Require(!hasKey("sp.noPressEnabled"), "schema should not expose deprecated no-press keys");
    Require(!hasKey("sp.noPressBaseTh"), "schema should not expose deprecated no-press threshold");
    Require(!hasKey("sp.edgeSigSuppressEnter"), "schema should not expose deprecated edge suppress keys");
    Require(!hasKey("sp.sigSuppressEnabled"), "schema should not expose deprecated suppress keys");
    Require(!hasKey("sp.btMapMode"), "schema should not expose deprecated bt map mode key");
    Require(!hasKey("sp.pressIirQ8"), "schema should not expose deprecated pressure iir key");
    Require(hasKey("sp.recheckThMulti"), "schema should expose recheck multi threshold");
    Require(hasKey("sp.tx1InkEnterTh"), "schema should expose tx1 ink enter threshold");
    Require(hasKey("sp.tx1LiftSuspiciousTh"), "schema should expose tx1 suspicious lift threshold");
    Require(hasKey("sp.tx1LiftAbsoluteTh"), "schema should expose tx1 absolute lift threshold");
}

void TestGridPeakDetectorNoPeak() {
    GridPeakDetector detector;
    int16_t grid[kGridDim][kGridDim]{};

    const auto analysis = detector.AnalyzePeakAndProjection(grid);
    Require(!analysis.peak.valid, "empty grid should not produce a peak");
    Require(analysis.projection.peakIdxDim1 == -1, "empty grid should not produce a dim1 projection peak");
    Require(analysis.projection.peakIdxDim2 == -1, "empty grid should not produce a dim2 projection peak");
}

void TestGridPeakDetectorCenterPeakAndProjection() {
    GridPeakDetector detector;
    int16_t grid[kGridDim][kGridDim]{};
    const auto src = MakeCrossGrid(16000, 14000, 12000, 10000);
    for (int r = 0; r < kGridDim; ++r) {
        for (int c = 0; c < kGridDim; ++c) {
            grid[r][c] = static_cast<int16_t>(src[static_cast<size_t>(r * kGridDim + c)]);
        }
    }

    const auto analysis = detector.AnalyzePeakAndProjection(grid);
    Require(analysis.peak.valid, "center cross should produce a peak");
    Require(analysis.peak.peakRow == 4 && analysis.peak.peakCol == 4,
            "center cross should peak at the center cell");
    Require(analysis.projection.peakIdxDim1 == 4 && analysis.projection.peakIdxDim2 == 4,
            "center cross projection should peak on the center axis");
    Require(analysis.projection.spanDim1 == 3 && analysis.projection.spanDim2 == 3,
            "default projection radius should span three rows and columns");
}

void TestGridPeakDetectorEdgePeak() {
    GridPeakDetector detector;
    int16_t grid[kGridDim][kGridDim]{};
    const auto src = MakeCrossGrid(14000, 9000, 6000, 4000, 0, 0);
    for (int r = 0; r < kGridDim; ++r) {
        for (int c = 0; c < kGridDim; ++c) {
            grid[r][c] = static_cast<int16_t>(src[static_cast<size_t>(r * kGridDim + c)]);
        }
    }

    const auto analysis = detector.AnalyzePeakAndProjection(grid);
    Require(analysis.peak.valid, "corner cross should produce a peak");
    Require(analysis.peak.peakRow == 0 && analysis.peak.peakCol == 0,
            "corner cross should peak at the corner cell");
    Require(analysis.projection.peakIdxDim1 == 0 && analysis.projection.peakIdxDim2 == 0,
            "corner cross projection should clamp to the corner index");
}

void TestGridPeakDetectorConnectedReject() {
    GridPeakDetector detector;
    detector.maxConnected = 5;
    int16_t grid[kGridDim][kGridDim]{};
    grid[4][4] = 300;
    grid[3][4] = 250;
    grid[5][4] = 250;
    grid[4][3] = 250;
    grid[4][5] = 250;

    const auto analysis = detector.AnalyzePeakAndProjection(grid);
    Require(!analysis.peak.valid,
            "connected region at or above maxConnected should be rejected");
}

void TestGridPeakDetectorNeighborSumAndTieBreak() {
    GridPeakDetector detector;
    int16_t grid[kGridDim][kGridDim]{};

    const auto peakA = MakeCrossGrid(12000, 10000, 8000, 6000, 2, 2);
    const auto peakB = MakeCrossGrid(14000, 12000, 9000, 7000, 6, 6);
    for (size_t i = 0; i < peakA.size(); ++i) {
        grid[i / kGridDim][i % kGridDim] =
            static_cast<int16_t>(std::max<uint16_t>(peakA[i], peakB[i]));
    }

    auto analysis = detector.AnalyzePeakAndProjection(grid);
    Require(analysis.peak.valid, "dual isolated peaks should still yield a valid peak");
    Require(analysis.peak.peakRow == 6 && analysis.peak.peakCol == 6,
            "larger 3x3 neighbor sum should win peak selection");

    int16_t tieGrid[kGridDim][kGridDim]{};
    const auto tieA = MakeCrossGrid(11000, 9000, 7000, 5000, 2, 2);
    const auto tieB = MakeCrossGrid(11000, 9000, 7000, 5000, 6, 6);
    for (size_t i = 0; i < tieA.size(); ++i) {
        tieGrid[i / kGridDim][i % kGridDim] =
            static_cast<int16_t>(std::max<uint16_t>(tieA[i], tieB[i]));
    }

    analysis = detector.AnalyzePeakAndProjection(tieGrid);
    Require(analysis.peak.peakRow == 2 && analysis.peak.peakCol == 2,
            "equal neighbor sums should preserve row-major tie breaking");
}

void TestGridPeakDetectorProjectionRadius() {
    GridPeakDetector detector;
    detector.projRadius = 1;
    int16_t grid[kGridDim][kGridDim]{};
    const auto src = MakeCrossGrid(16000, 14000, 12000, 10000);
    for (int r = 0; r < kGridDim; ++r) {
        for (int c = 0; c < kGridDim; ++c) {
            grid[r][c] = static_cast<int16_t>(src[static_cast<size_t>(r * kGridDim + c)]);
        }
    }

    const auto analysis = detector.AnalyzePeakAndProjection(grid);
    Require(analysis.projection.spanDim1 == 3 && analysis.projection.spanDim2 == 3,
            "custom projection radius should change projection span");
}

void TestStylusPipelinePeakMetrics() {
    StylusPipeline pipeline;
    pipeline.LoadConfig("sp.noPressEnabled", "0");
    pipeline.LoadConfig("sp.sigSuppressEnabled", "0");

    const auto tx1Grid = MakeCrossGrid(16000, 14000, 12000, 10000);
    const auto tx2Grid = MakeCrossGrid(6000, 5000, 4000, 3000);
    const auto raw = BuildCombinedStylusFrame(10, 10, tx1Grid, 10, 10, tx2Grid);

    const auto frame = RunFrame(pipeline, raw, 300, 8);
    Require(frame.point.valid, "pipeline should still solve a valid stylus point");
    Require(frame.signalX == 16000, "TX1 peak signal should match the detector peak");
    Require(frame.signalY == 6000, "TX2 peak signal should match the detector peak");
    Require(frame.point.peakTx1 == 14666, "TX1 composite peak should use the pre-CMF projection average");
    Require(frame.point.peakTx2 == 6000, "TX2 composite peak should mirror TX2 peak signal");
}

void TestStylusPipelineUsesPredictedIntermediatePressure() {
    StylusPipeline pipeline;
    pipeline.LoadConfig("sp.filterMode", "2");

    const auto raw = BuildCombinedStylusFrame(10, 10,
        MakeCrossGrid(12000, 9000, 7000, 5000, 4, 4), 10, 10, {});

    const auto f0 = RunFrame(pipeline, raw, 220, 8, true);
    Require(f0.point.valid, "first frame should be valid");
    Require(f0.pressure > 0, "real BT frame should output pressure");
#if EGOTOUCH_DIAG
    Require(f0.diag.pressureIsReal, "first frame should be marked as real pressure");
    Require(f0.diag.btSeq == 1, "first frame should carry first BT seq");
#endif

    const auto f1 = RunFrame(pipeline, raw, 220, 16, false);
    Require(f1.point.valid, "predicted frame should remain valid");
    Require(f1.pressure > 0, "predicted intermediate frame should keep pressure");
#if EGOTOUCH_DIAG
    Require(!f1.diag.pressureIsReal, "intermediate frame without new BT push should be predicted");
    Require(f1.diag.btSeq == 1, "predicted frame should retain previous BT seq");
    Require(f1.diag.predictedAgeFrames >= 1, "predicted frame should increase predicted age");
#endif

    const auto lift = RunFrame(pipeline, BuildCombinedStylusFrame(0x00FF, 0x00FF, {}, 0, 0, {}), 220, 20, false);
    Require(lift.pressure == 0, "release should close pressure gate immediately");
    Require(!lift.tipSwitchActive, "release should clear tip switch");

    const auto hoverOnly = RunFrame(pipeline, raw, 220, 24, false);
    Require(hoverOnly.tipSwitchActive, "TX1 should still allow immediate writing on the next down frame");
    Require(hoverOnly.pressure == 0, "stale BT sample should not reopen pressure gate");

    const auto f2 = RunFrame(pipeline, raw, 260, 32, true);
    Require(f2.pressure > 0, "new BT push after release should reopen pressure output");
#if EGOTOUCH_DIAG
    Require(f2.diag.pressureIsReal, "new BT push should restore real pressure flag");
    Require(f2.diag.btSeq >= 2, "new BT push should advance sequence");
#endif
}

void TestStylusPipelineRealZeroDropsPressureButKeepsWritingWhenTx1StillDown() {
    StylusPipeline pipeline;
    pipeline.LoadConfig("sp.filterMode", "2");

    const auto raw = BuildCombinedStylusFrame(10, 10,
        MakeCrossGrid(12000, 9000, 7000, 5000, 4, 4), 10, 10, {});

    const auto down = RunFrame(pipeline, raw, 240, 8, true);
    Require(down.tipSwitchActive, "down frame should assert tip switch");

    const auto up = RunFrame(pipeline, raw, 0, 16, true);
    Require(up.pressure == 0, "real zero BT sample should drop output pressure immediately");
    Require(up.tipSwitchActive, "real zero BT sample should keep writing while TX1 still indicates down");
#if EGOTOUCH_DIAG
    Require(up.diag.pressureIsReal, "real zero frame should still be marked as real");
#endif
}

} // namespace

int main() {
    try {
        TestPressureStageSignalSuppressHysteresis();
        TestPressureStageEdgeSuppressHysteresis();
        TestPressureStagePredictsBetweenRealSamples();
        TestPressureStageRealZeroDropsImmediately();
        TestBtPressBufferDefaultMode();
        TestBtPressBufferOnCellMapping();
        TestBtPressBufferInCellMapping();
        TestBtPressBufferReset();
        TestBtPressBufferSeqSnapshot();
        TestFakePressureDecayBasic();
        TestFakePressureDecayReset();
        TestNoPressInkGateEnterAndExit();
        TestStylusPipelineNoPressSyntheticPressure();
        TestStylusPipelineLowSignalKeepsCoordinateButZeroPressureUntilAuthoritativeDown();
        TestStylusPipelineFastLiftToNoSignal();
        TestStylusPipelineReleaseRetainsPreviousOutput();
        TestStylusPipelineEdgeFastLiftRetainsCoordinate();
        TestStylusPipelineConfigRoundTrip();
        TestGridPeakDetectorNoPeak();
        TestGridPeakDetectorCenterPeakAndProjection();
        TestGridPeakDetectorEdgePeak();
        TestGridPeakDetectorConnectedReject();
        TestGridPeakDetectorNeighborSumAndTieBreak();
        TestGridPeakDetectorProjectionRadius();
        TestStylusPipelinePeakMetrics();
        TestStylusPipelineUsesPredictedIntermediatePressure();
        TestStylusPipelineRealZeroDropsPressureButKeepsWritingWhenTx1StillDown();
        std::cout << "[TEST] Stylus fast ink tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
