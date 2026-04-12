#include "StylusSolver/NoPressInkGate.hpp"
#include "StylusSolver/PressureSolver.hpp"
#include "StylusSolver/StylusPipeline.h"

#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

using Asa::EdgeSignalInputs;
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
                         uint64_t timestamp = 0) {
    HeatmapFrame frame;
    frame.rawPtr = raw.data();
    frame.rawLen = raw.size();
    frame.timestamp = timestamp;
    pipeline.SetBtMcuPressure(btPressure);
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
    solver.tailFrames = 0;
    solver.signalSuppressEnabled = true;
    solver.signalSuppressEnter = 2200;
    solver.signalSuppressExit = 3200;
    solver.edgeSignalSuppressEnabled = false;

    const auto s0 = solver.SolveStage(180, true, 1800, false);
    Require(s0.mappedPressure > 0, "mapped pressure should be produced before suppression");
    Require(s0.realPressure == 0, "low TX1 composite should suppress real pressure");
    Require(s0.signalSuppressActive, "ordinary signal suppression should become active");

    const auto s1 = solver.SolveStage(180, true, 2500, false);
    Require(s1.realPressure == 0, "suppression should hold until exit threshold");
    Require(s1.signalSuppressActive, "suppression should still be active below exit threshold");

    const auto s2 = solver.SolveStage(180, true, 3500, false);
    Require(s2.realPressure > 0, "pressure should recover after exit threshold");
    Require(!s2.signalSuppressActive, "suppression should clear after recovery");
}

void TestPressureStageEdgeSuppressHysteresis() {
    PressureSolver solver;
    solver.tailFrames = 0;
    solver.signalSuppressEnabled = false;
    solver.edgeSignalSuppressEnabled = true;
    solver.edgeSignalSuppressEnter = 1500;
    solver.edgeSignalSuppressExit = 3000;

    EdgeSignalInputs edge{};
    edge.dim1Active = true;
    edge.dim1Signal = 1400;
    const auto s0 = solver.SolveStage(180, true, 5000, true, edge);
    Require(s0.realPressure == 0, "edge signal under enter threshold should suppress pressure");
    Require(s0.edgeSignalSuppressActive, "edge suppression should latch active");

    edge.dim1Signal = 1800;
    const auto s1 = solver.SolveStage(180, true, 5000, true, edge);
    Require(s1.realPressure == 0, "edge suppression should hold below exit threshold");

    edge.dim1Signal = 3201;
    const auto s2 = solver.SolveStage(180, true, 5000, true, edge);
    Require(s2.realPressure > 0, "edge suppression should clear after exit threshold");
    Require(!s2.edgeSignalSuppressActive, "edge suppression should clear after recovery");
}

void TestNoPressInkGateEnterAndExit() {
    NoPressInkGate gate;
    gate.enabled = true;

    const auto r0 = gate.Apply(true, true, false, 0, 0, 10000, 0);
    Require(!r0.active, "first qualifying frame should not enter no-press ink yet");
    Require(r0.outputPressure == 0, "first qualifying frame should still output zero pressure");

    const auto r1 = gate.Apply(true, true, false, 0, 0, 10000, 0);
    Require(r1.active, "second qualifying frame should enter no-press ink");
    Require(r1.outputPressure == 10, "entry frame should emit synthetic minimum pressure");

    const auto r2 = gate.Apply(true, true, false, 120, 10, 20000, 0);
    Require(r2.outputPressure == 120, "real pressure should override synthetic pressure");

    const auto r3 = gate.Apply(true, true, false, 0, 10, 2000, 0);
    Require(r3.active, "first exit-qualifying frame should still debounce");
    Require(r3.outputPressure == 10, "debouncing exit should keep synthetic pressure");

    const auto r4 = gate.Apply(true, true, false, 0, 10, 2000, 0);
    Require(!r4.active, "second exit-qualifying frame should leave no-press ink");
    Require(r4.outputPressure == 0, "exit frame should drop to zero pressure");
}

void TestStylusPipelineNoPressSyntheticPressure() {
    StylusPipeline pipeline;
    pipeline.LoadConfig("sp.noPressEnabled", "1");

    const auto tx1Grid = MakeCrossGrid(16000, 14000, 12000, 10000);
    const auto tx2Grid = MakeCrossGrid(6000, 5000, 4000, 3000);
    const auto raw = BuildCombinedStylusFrame(10, 10, tx1Grid, 10, 10, tx2Grid);

    const auto f0 = RunFrame(pipeline, raw, 0, 8);
    Require(f0.point.valid, "strong zero-pressure frame should produce valid coordinates");
    Require(f0.pressure == 0, "first no-press frame should still debounce");
    Require(!f0.noPressInkActive, "first no-press frame should not be active");

    const auto f1 = RunFrame(pipeline, raw, 0, 16);
    Require(f1.noPressInkActive, "second no-press frame should activate no-press ink");
    Require(f1.pressure == 10, "second no-press frame should emit synthetic pressure");
    Require(f1.packet.valid, "no-press frame should still emit a stylus packet");
}

void TestStylusPipelineLowSignalPressureSuppress() {
    StylusPipeline pipeline;
    pipeline.LoadConfig("sp.sigSuppressEnabled", "1");
    pipeline.LoadConfig("sp.sigSuppressEnter", "2200");
    pipeline.LoadConfig("sp.sigSuppressExit", "3200");
    pipeline.LoadConfig("sp.noPressEnabled", "0");

    const auto weakGrid = MakeCrossGrid(1800, 1400, 900, 600);
    const auto raw = BuildCombinedStylusFrame(12, 12, weakGrid, 12, 12, {});

    const auto frame = RunFrame(pipeline, raw, 180, 8);
    Require(frame.point.valid, "weak-signal frame should still solve coordinates");
    Require(frame.pressure == 0, "weak-signal frame should quickly suppress pressure");
    Require(!frame.noPressInkActive, "suppressed pressure should not report no-press ink");
}

void TestStylusPipelineEdgeLiftFreeze() {
    StylusPipeline pipeline;
    pipeline.LoadConfig("sp.noPressEnabled", "0");
    pipeline.LoadConfig("sp.sigSuppressEnabled", "0");
    pipeline.LoadConfig("sp.filterMode", "2");

    const auto interiorGrid = MakeCrossGrid(12000, 9000, 7000, 5000, 4, 4);
    const auto edgeGrid = MakeCrossGrid(14000, 9000, 6000, 4000, 0, 0);
    const auto rawInterior = BuildCombinedStylusFrame(10, 10, interiorGrid, 10, 10, {});
    const auto rawEdge = BuildCombinedStylusFrame(0, 0, edgeGrid, 0, 0, {});

    const auto down = RunFrame(pipeline, rawInterior, 180, 8);
    Require(down.point.valid, "pen-down frame should be valid");
    Require(down.pressure > 0, "pen-down frame should report pressure");

    std::this_thread::sleep_for(std::chrono::milliseconds(35));
    const auto lift = RunFrame(pipeline, rawEdge, 0, 16);
    Require(lift.point.valid, "edge-lift frame should remain valid");
    Require(lift.pressure == 0, "edge-lift frame should not keep tail pressure");
    RequireNear(lift.point.x, down.point.x, 0.001f,
                "edge-lift frame should freeze X to previous point");
    RequireNear(lift.point.y, down.point.y, 0.001f,
                "edge-lift frame should freeze Y to previous point");
}

void TestStylusPipelineConfigRoundTrip() {
    StylusPipeline pipeline;
    pipeline.LoadConfig("sp.recheckThBase", "888");
    pipeline.LoadConfig("sp.recheckThMulti", "1333");
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

    std::ostringstream out;
    pipeline.SaveConfig(out);
    const std::string saved = out.str();

    Require(saved.find("sp.recheckThBase=888") != std::string::npos,
            "saved config should include recheck base threshold");
    Require(saved.find("sp.recheckThMulti=1333") != std::string::npos,
            "saved config should include recheck multi threshold");
    Require(saved.find("sp.edgeSigSuppressEnter=1444") != std::string::npos,
            "saved config should include edge signal suppress enter");
    Require(saved.find("sp.noPressBaseTh=9999") != std::string::npos,
            "saved config should include no-press base threshold");
    Require(saved.find("sp.noPressDebounceExit=4") != std::string::npos,
            "saved config should include no-press exit debounce");

    StylusPipeline loaded;
    LoadFromSavedText(loaded, saved);

    std::ostringstream outLoaded;
    loaded.SaveConfig(outLoaded);
    const std::string savedLoaded = outLoaded.str();
    Require(savedLoaded.find("sp.recheckThBase=888") != std::string::npos,
            "loaded config should preserve recheck base threshold");
    Require(savedLoaded.find("sp.edgeSigSuppressExit=3555") != std::string::npos,
            "loaded config should preserve edge signal suppress exit");
    Require(savedLoaded.find("sp.noPressSyntheticMin=12") != std::string::npos,
            "loaded config should preserve synthetic minimum");

    const auto schema = loaded.GetConfigSchema();
    auto hasKey = [&](const char* key) {
        for (const auto& param : schema) {
            if (param.key == key) return true;
        }
        return false;
    };
    Require(hasKey("sp.noPressEnabled"), "schema should expose no-press enable");
    Require(hasKey("sp.noPressBaseTh"), "schema should expose no-press base threshold");
    Require(hasKey("sp.edgeSigSuppressEnter"), "schema should expose edge signal suppress enter");
    Require(hasKey("sp.recheckThMulti"), "schema should expose recheck multi threshold");
}

} // namespace

int main() {
    try {
        TestPressureStageSignalSuppressHysteresis();
        TestPressureStageEdgeSuppressHysteresis();
        TestNoPressInkGateEnterAndExit();
        TestStylusPipelineNoPressSyntheticPressure();
        TestStylusPipelineLowSignalPressureSuppress();
        TestStylusPipelineEdgeLiftFreeze();
        TestStylusPipelineConfigRoundTrip();
        std::cout << "[TEST] Stylus fast ink tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
