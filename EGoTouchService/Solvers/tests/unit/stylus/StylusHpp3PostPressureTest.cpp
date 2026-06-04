#include "StylusSolver/hpp3/Hpp3PostPressureProcess.hpp"
#include "StylusSolver/hpp3/PressureSolver.hpp"
#include "StylusSolver/StylusPipeline.h"
#include "StylusSolver/StylusRuntimeCommit.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

using Solvers::HeatmapFrame;
using Solvers::Stylus::Hpp3PostPressureProcess;
using Solvers::Stylus::PressureSolver;

void Require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

HeatmapFrame MakeFrame(int32_t x,
                       int32_t y,
                       uint16_t pressure,
                       uint16_t signalX = 4000,
                       uint16_t signalY = 4000) {
    HeatmapFrame frame{};
    auto& stylus = frame.stylus;

    stylus.runtime.tx1.coordinate.reportGlobalCoor = {x, y, true};
    stylus.runtime.signal.signalX = signalX;
    stylus.runtime.signal.signalY = signalY;
    stylus.runtime.signal.maxRawPeak = signalX > signalY ? signalX : signalY;

    stylus.runtime.pressure.rawPressure = pressure;
    stylus.runtime.pressure.mappedPressure = pressure;
    stylus.runtime.pressure.outputPressure = pressure;
    stylus.runtime.pressure.btSample.pressure.fill(0);
    stylus.runtime.pressure.btSample.pressure[3] = pressure;
    stylus.runtime.pressure.btSample.hasSample = true;

    stylus.runtime.decision.inRangeCandidate = true;
    stylus.runtime.decision.tipDownCandidate = pressure != 0;
    stylus.runtime.decision.authoritativeDown = pressure != 0;
    return frame;
}

void SetBtFreq(HeatmapFrame& frame, uint8_t freq1, uint8_t freq2) {
    frame.stylus.runtime.pressure.btSample.freq1 = freq1;
    frame.stylus.runtime.pressure.btSample.freq2 = freq2;
    frame.stylus.runtime.pressure.btSample.hasFreq = true;
}

void SetBtInput(HeatmapFrame& frame,
                const std::array<uint16_t, 4>& pressure,
                uint32_t seq) {
    frame.stylus.input.btSample.pressure = pressure;
    frame.stylus.input.btSample.seq = seq;
    frame.stylus.input.btSample.hasSample = true;
}

void SetEdge(HeatmapFrame& frame,
             bool dim1Edge,
             bool dim2Edge,
             uint16_t dim1Signal,
             uint16_t dim2Signal) {
    auto& signal = frame.stylus.runtime.signal;
    signal.dim1EdgeActive = dim1Edge;
    signal.dim2EdgeActive = dim2Edge;
    signal.dim1EdgeSignal = dim1Signal;
    signal.dim2EdgeSignal = dim2Signal;
    signal.signalX = dim1Signal;
    signal.signalY = dim2Signal;
    signal.maxRawPeak = dim1Signal > dim2Signal ? dim1Signal : dim2Signal;
}

void TestDisabledModulePreservesRuntime() {
    Hpp3PostPressureProcess process;
    process.m_enabled = false;

    auto frame = MakeFrame(1000, 1000, 123);
    frame.stylus.runtime.decision.tipDownCandidate = false;
    process.Process(frame);

    Require(frame.stylus.runtime.pressure.outputPressure == 123,
            "disabled post-pressure should preserve pressure");
    Require(!frame.stylus.runtime.decision.tipDownCandidate,
            "disabled post-pressure should preserve tip decision");
}

void TestNoPressDisabledDoesNotInjectPressure() {
    Hpp3PostPressureProcess process;

    auto frame = MakeFrame(1000, 1000, 0);
    frame.stylus.runtime.decision.tipDownCandidate = false;
    process.Process(frame);

    Require(frame.stylus.runtime.pressure.outputPressure == 0,
            "zero pressure without a tip candidate should remain zero");
    Require(!frame.stylus.runtime.decision.tipDownCandidate,
            "zero pressure without a tip candidate should remain released");
}

void TestNoPressCandidateRetainsPreviousPressureOrFallback() {
    Hpp3PostPressureProcess process;

    auto first = MakeFrame(1000, 1000, 321);
    process.Process(first);

    auto retained = MakeFrame(1000, 1000, 0);
    retained.stylus.runtime.decision.tipDownCandidate = true;
    retained.stylus.runtime.decision.authoritativeDown = true;
    process.Process(retained);

    Require(retained.stylus.runtime.pressure.outputPressure == 321,
            "tip candidate should retain previous post-pressure output");

    Hpp3PostPressureProcess fresh;
    auto fallback = MakeFrame(1000, 1000, 0);
    fallback.stylus.runtime.decision.tipDownCandidate = true;
    fallback.stylus.runtime.decision.authoritativeDown = true;
    fresh.Process(fallback);

    Require(fallback.stylus.runtime.pressure.outputPressure == 10,
            "tip candidate with no previous pressure should use fallback pressure 10");
}

void TestPostPressurePublishesFinalPressureForCommit() {
    Hpp3PostPressureProcess process;
    Solvers::Stylus::StylusRuntimeCommit commit;

    auto frame = MakeFrame(1000, 1000, 321);
    frame.stylus.runtime.post.finalValid = true;
    process.Process(frame);
    commit.Commit(frame);

    Require(frame.stylus.runtime.post.finalPressure == 321,
            "post-pressure should publish output pressure as final pressure");
    Require(frame.stylus.runtime.post.point.rawPressure == 321,
            "post-pressure should publish raw pressure into the solve point");
    Require(frame.stylus.runtime.post.point.mappedPressure == 321,
            "post-pressure should publish mapped pressure into the solve point");
    Require(frame.stylus.runtime.post.point.pressure == 321,
            "post-pressure should publish output pressure into the solve point");
    Require(frame.stylus.output.pressure == 321,
            "commit should expose final pressure through stylus output");
    Require(frame.stylus.output.point.pressure == 321,
            "commit should expose final pressure through the output point");
}

void TestPressureSolverUsesTsacoreOnCellOrderAndResetsOnNewPacket() {
    PressureSolver solver;
    solver.m_btPressureMapOrderMode = PressureSolver::OnCell;
    solver.m_polyEnabled = false;

    auto frame = MakeFrame(0, 0, 0);
    SetBtInput(frame, std::array<uint16_t, 4>{{100, 200, 300, 400}}, 1);

    solver.Process(frame);
    Require(frame.stylus.runtime.pressure.rawPressure == 100,
            "OnCell slot 0 should use pressure[0]");
    solver.Process(frame);
    Require(frame.stylus.runtime.pressure.rawPressure == 200,
            "OnCell slot 1 should use pressure[1]");
    solver.Process(frame);
    Require(frame.stylus.runtime.pressure.rawPressure == 200,
            "OnCell slot 2 should repeat pressure[1]");
    solver.Process(frame);
    Require(frame.stylus.runtime.pressure.rawPressure == 300,
            "OnCell slot 3 should use pressure[2]");
    solver.Process(frame);
    Require(frame.stylus.runtime.pressure.rawPressure == 400,
            "OnCell slot 4 should use pressure[3]");
    solver.Process(frame);
    Require(frame.stylus.runtime.pressure.rawPressure == 400,
            "OnCell slot 5 should repeat pressure[3]");

    SetBtInput(frame, std::array<uint16_t, 4>{{500, 600, 700, 800}}, 2);
    solver.Process(frame);
    Require(frame.stylus.runtime.pressure.rawPressure == 500,
            "new BT packet should reset the pressure slot counter");
}

void TestPressureSolverUsesTsacoreInCellOrder() {
    PressureSolver solver;
    solver.m_btPressureMapOrderMode = PressureSolver::InCell;
    solver.m_polyEnabled = false;

    auto frame = MakeFrame(0, 0, 0);
    SetBtInput(frame, std::array<uint16_t, 4>{{100, 200, 300, 400}}, 1);

    solver.Process(frame);
    Require(frame.stylus.runtime.pressure.rawPressure == 100,
            "InCell slot 0 should use pressure[0]");
    solver.Process(frame);
    Require(frame.stylus.runtime.pressure.rawPressure == 200,
            "InCell slot 1 should use pressure[1]");
    solver.Process(frame);
    Require(frame.stylus.runtime.pressure.rawPressure == 300,
            "InCell slot 2 should use pressure[2]");
    solver.Process(frame);
    Require(frame.stylus.runtime.pressure.rawPressure == 400,
            "InCell slot 3 should use pressure[3]");
}

void TestLookaheadHoverGateClearsPressureAndDecision() {
    PressureSolver solver;
    solver.m_btPressureMapOrderMode = PressureSolver::OnCell;
    solver.m_polyEnabled = false;

    auto down = MakeFrame(0, 0, 0);
    SetBtInput(down, std::array<uint16_t, 4>{{380, 380, 380, 380}}, 1);
    solver.Process(down);
    Require(down.stylus.runtime.pressure.outputPressure != 0,
            "nonzero packet should produce pressure before look-ahead hover");
    Require(down.stylus.runtime.decision.tipDownCandidate,
            "nonzero packet should set tip-down after NoPressInk is removed from pipeline");

    auto hover = MakeFrame(0, 0, 0);
    SetBtInput(hover, std::array<uint16_t, 4>{{118, 118, 0, 0}}, 2);
    solver.Process(hover);
    Require(hover.stylus.runtime.pressure.lookaheadHoverGate,
            "last pressure slot zero should set lookahead hover gate");
    Require(hover.stylus.runtime.pressure.outputPressure == 0,
            "last pressure slot zero should clear output pressure immediately");
    Require(!hover.stylus.runtime.decision.tipDownCandidate,
            "last pressure slot zero should clear tip-down decision");
}

void TestLookaheadHoverGateBlocksPostPressureReinjection() {
    Hpp3PostPressureProcess process;

    auto first = MakeFrame(0, 0, 800);
    process.Process(first);

    auto hover = MakeFrame(600, 0, 0);
    hover.stylus.runtime.pressure.lookaheadHoverGate = true;
    hover.stylus.runtime.decision.tipDownCandidate = true;
    hover.stylus.runtime.decision.authoritativeDown = true;
    process.Process(hover);

    Require(hover.stylus.runtime.pressure.outputPressure == 0,
            "lookahead hover gate should block fake pressure reinjection");
    Require(!hover.stylus.runtime.decision.tipDownCandidate,
            "lookahead hover gate should force a hover tip decision");
    Require(!process.IsFakePressureDecreaseArmedForTest(),
            "lookahead hover gate should clear fake pressure state");
}

void TestBtPressSignalSuppressEnterClearsPressure() {
    PressureSolver solver;
    solver.m_polyEnabled = false;
    solver.m_btPressSignalSuppressEnterThreshold = 500;
    solver.m_btPressSignalSuppressExitThreshold = 700;

    auto frame = MakeFrame(0, 0, 0, 400, 1200);
    SetBtInput(frame, std::array<uint16_t, 4>{{200, 200, 200, 200}}, 1);
    solver.Process(frame);

    Require(frame.stylus.runtime.pressure.outputPressure == 0,
            "low TX1 signal below enter threshold should suppress BT pressure");
    Require(!frame.stylus.runtime.decision.tipDownCandidate,
            "low TX1 signal below enter threshold should clear tip decision");
}

void TestBtPressSignalSuppressSkipsEdgeFrames() {
    PressureSolver solver;
    solver.m_polyEnabled = false;
    solver.m_btPressSignalSuppressEnterThreshold = 500;
    solver.m_btPressSignalSuppressExitThreshold = 700;

    auto frame = MakeFrame(0, 0, 0, 400, 1200);
    SetBtInput(frame, std::array<uint16_t, 4>{{200, 200, 200, 200}}, 1);
    SetEdge(frame, true, false, 400, 1200);
    solver.Process(frame);

    Require(frame.stylus.runtime.pressure.outputPressure == 200,
            "edge frames should bypass BT pressure signal suppression");
    Require(frame.stylus.runtime.decision.tipDownCandidate,
            "edge frames should preserve tip decision when pressure remains nonzero");
}

void TestBtPressSignalSuppressLatchExitsOnlyAboveThreshold() {
    PressureSolver solver;
    solver.m_polyEnabled = false;
    solver.m_btPressSignalSuppressEnterThreshold = 500;
    solver.m_btPressSignalSuppressExitThreshold = 700;

    auto enter = MakeFrame(0, 0, 0, 400, 1200);
    SetBtInput(enter, std::array<uint16_t, 4>{{200, 200, 200, 200}}, 1);
    solver.Process(enter);
    Require(enter.stylus.runtime.pressure.outputPressure == 0,
            "enter frame should latch BT pressure suppression");

    auto equalExit = MakeFrame(0, 0, 0, 700, 1200);
    SetBtInput(equalExit, std::array<uint16_t, 4>{{200, 200, 200, 200}}, 2);
    solver.Process(equalExit);
    Require(equalExit.stylus.runtime.pressure.outputPressure == 0,
            "BT pressure suppression should not clear at the exact exit threshold");

    auto aboveExit = MakeFrame(0, 0, 0, 701, 1200);
    SetBtInput(aboveExit, std::array<uint16_t, 4>{{200, 200, 200, 200}}, 3);
    solver.Process(aboveExit);
    Require(aboveExit.stylus.runtime.pressure.outputPressure == 200,
            "BT pressure suppression should clear only after the exit threshold");
    Require(aboveExit.stylus.runtime.decision.tipDownCandidate,
            "tip decision should recover after BT pressure suppression exits");
}

void TestFakePressureDecreaseBuckets() {
    {
        Hpp3PostPressureProcess process;
        process.m_fakePressureDecreaseEnabled = true;
        auto first = MakeFrame(0, 0, 800);
        process.Process(first);
        auto second = MakeFrame(50, 0, 0);
        process.Process(second);
        Require(second.stylus.runtime.pressure.outputPressure == 0,
                "movement <=100 should not synthesize fake pressure");
    }
    {
        Hpp3PostPressureProcess process;
        process.m_fakePressureDecreaseEnabled = true;
        auto first = MakeFrame(0, 0, 800);
        process.Process(first);
        auto second = MakeFrame(200, 0, 0);
        process.Process(second);
        Require(second.stylus.runtime.pressure.outputPressure == 400,
                "movement 101..300 should synthesize one fake pressure frame");
    }
    {
        Hpp3PostPressureProcess process;
        process.m_fakePressureDecreaseEnabled = true;
        auto first = MakeFrame(0, 0, 800);
        process.Process(first);
        auto second = MakeFrame(400, 0, 0);
        process.Process(second);
        Require(second.stylus.runtime.pressure.outputPressure == 533,
                "movement 301..500 should synthesize first two-frame fake pressure value");
        auto third = MakeFrame(800, 0, 0);
        process.Process(third);
        Require(third.stylus.runtime.pressure.outputPressure == 266,
                "fake pressure should use previous processed output for the next value");
    }
    {
        Hpp3PostPressureProcess process;
        process.m_fakePressureDecreaseEnabled = true;
        auto first = MakeFrame(0, 0, 800);
        process.Process(first);
        auto second = MakeFrame(600, 0, 0);
        process.Process(second);
        Require(second.stylus.runtime.pressure.outputPressure == 600,
                "movement >500 should synthesize first three-frame fake pressure value");
        auto third = MakeFrame(1200, 0, 0);
        process.Process(third);
        Require(third.stylus.runtime.pressure.outputPressure == 400,
                "three-frame fake pressure should decay using previous processed output");
        auto fourth = MakeFrame(1800, 0, 0);
        process.Process(fourth);
        Require(fourth.stylus.runtime.pressure.outputPressure == 0,
                "fake pressure should stop once previous output is no longer above the factory gate");
    }
}

void TestBtFreqShiftDebounceSuppressesFakePressure() {
    Hpp3PostPressureProcess process;
    process.m_fakePressureDecreaseEnabled = true;
    process.m_btFreqShiftDebounceFrames = 2;

    auto first = MakeFrame(0, 0, 800);
    SetBtFreq(first, 1, 2);
    process.Process(first);

    auto shifted = MakeFrame(600, 0, 0);
    SetBtFreq(shifted, 2, 2);
    process.Process(shifted);

    Require(shifted.stylus.runtime.pressure.outputPressure == 0,
            "BT freq shift debounce should suppress fake pressure decrease");
    Require(process.GetBtFreqShiftDebounceFramesLeftForTest() == 1,
            "BT freq shift debounce should leave the remaining frame count after the first gated frame");
    Require(!process.IsFakePressureDecreaseArmedForTest(),
            "BT freq shift debounce should clear fake pressure state");
}

void TestSingleEdgeLowSignalSuppressesPressure() {
    Hpp3PostPressureProcess process;
    process.m_pressureEdgeEnterThreshold = 500;
    process.m_pressureEdgeExitThreshold = 700;

    auto frame = MakeFrame(1000, 1000, 200);
    SetEdge(frame, true, false, 400, 1200);
    process.Process(frame);

    Require(frame.stylus.runtime.pressure.outputPressure == 0,
            "single edge signal below enter threshold should suppress pressure");
    Require(!frame.stylus.runtime.decision.tipDownCandidate,
            "edge suppression should clear tip candidate");
}

void TestBothEdgeEnterUsesTwoThirdsThreshold() {
    Hpp3PostPressureProcess process;
    process.m_pressureEdgeEnterThreshold = 900;
    process.m_pressureEdgeExitThreshold = 1000;

    auto above = MakeFrame(1000, 1000, 200);
    SetEdge(above, true, true, 650, 650);
    process.Process(above);
    Require(above.stylus.runtime.pressure.outputPressure == 200,
            "both-edge signal above two-thirds threshold should keep pressure");

    auto below = MakeFrame(1000, 1000, 200);
    SetEdge(below, true, true, 590, 650);
    process.Process(below);
    Require(below.stylus.runtime.pressure.outputPressure == 0,
            "both-edge signal below two-thirds threshold should suppress pressure");
}

void TestEdgeLatchExitsOnlyAboveExitThreshold() {
    Hpp3PostPressureProcess process;
    process.m_pressureEdgeEnterThreshold = 500;
    process.m_pressureEdgeExitThreshold = 700;

    auto enter = MakeFrame(1000, 1000, 200);
    SetEdge(enter, true, false, 400, 1200);
    process.Process(enter);
    Require(process.IsEdgeSignalTooLowLatchedForTest(),
            "low edge signal should latch suppression");

    auto equalExit = MakeFrame(1000, 1000, 200);
    SetEdge(equalExit, true, false, 700, 1200);
    process.Process(equalExit);
    Require(equalExit.stylus.runtime.pressure.outputPressure == 0,
            "edge latch should not clear at the exact exit threshold");
    Require(process.IsEdgeSignalTooLowLatchedForTest(),
            "edge latch should remain at the exact exit threshold");

    auto aboveExit = MakeFrame(1000, 1000, 200);
    SetEdge(aboveExit, true, false, 701, 1200);
    process.Process(aboveExit);
    Require(aboveExit.stylus.runtime.pressure.outputPressure == 200,
            "edge latch should clear only after signal is above the exit threshold");
    Require(!process.IsEdgeSignalTooLowLatchedForTest(),
            "edge latch should clear after all active edge dimensions exit");
}

void TestConfigRoundTripIncludesPostPressure() {
    Solvers::StylusPipeline pipeline;
    pipeline.m_hpp3.m_postPressure.m_enabled = false;
    pipeline.m_hpp3.m_postPressure.m_fakePressureDecreaseEnabled = false;
    pipeline.m_hpp3.m_postPressure.m_btFreqShiftDebounceFrames = 7;
    pipeline.m_hpp3.m_postPressure.m_pressureEdgeEnterThreshold = 1234;
    pipeline.m_hpp3.m_postPressure.m_pressureEdgeExitThreshold = 2345;
    pipeline.m_hpp3.m_pressureSolver.m_btPressSignalSuppressEnterThreshold = 3456;
    pipeline.m_hpp3.m_pressureSolver.m_btPressSignalSuppressExitThreshold = 4567;

    std::ostringstream out;
    pipeline.SaveConfig(out);
    const std::string saved = out.str();

    Require(saved.find("sp.postPressureEnabled=0") != std::string::npos,
            "saved config should contain post-pressure enable flag");
    Require(saved.find("sp.fakePressureDecreaseEnabled=0") != std::string::npos,
            "saved config should contain fake pressure flag");
    Require(saved.find("sp.btFreqShiftDebounceFrames=7") != std::string::npos,
            "saved config should contain BT freq debounce frames");
    Require(saved.find("sp.pressureEdgeEnterThreshold=1234") != std::string::npos,
            "saved config should preserve enter threshold key");
    Require(saved.find("sp.pressureEdgeExitThreshold=2345") != std::string::npos,
            "saved config should preserve exit threshold key");
    Require(saved.find("sp.btPressSignalSuppressEnterThreshold=3456") != std::string::npos,
            "saved config should contain BT pressure suppress enter threshold");
    Require(saved.find("sp.btPressSignalSuppressExitThreshold=4567") != std::string::npos,
            "saved config should contain BT pressure suppress exit threshold");

    Solvers::StylusPipeline restored;
    LoadFromSavedText(restored, saved);

    Require(!restored.m_hpp3.m_postPressure.m_enabled,
            "loaded config should restore post-pressure enable flag");
    Require(!restored.m_hpp3.m_postPressure.m_fakePressureDecreaseEnabled,
            "loaded config should restore fake pressure flag");
    Require(restored.m_hpp3.m_postPressure.m_btFreqShiftDebounceFrames == 7,
            "loaded config should restore BT freq debounce frames");
    Require(restored.m_hpp3.m_postPressure.m_pressureEdgeEnterThreshold == 1234,
            "loaded config should restore enter threshold");
    Require(restored.m_hpp3.m_postPressure.m_pressureEdgeExitThreshold == 2345,
            "loaded config should restore exit threshold");
    Require(restored.m_hpp3.m_pressureSolver.m_btPressSignalSuppressEnterThreshold == 3456,
            "loaded config should restore BT pressure suppress enter threshold");
    Require(restored.m_hpp3.m_pressureSolver.m_btPressSignalSuppressExitThreshold == 4567,
            "loaded config should restore BT pressure suppress exit threshold");
}


} // namespace

int main() {
    try {
        TestDisabledModulePreservesRuntime();
        TestNoPressDisabledDoesNotInjectPressure();
        TestNoPressCandidateRetainsPreviousPressureOrFallback();
        TestPostPressurePublishesFinalPressureForCommit();
        TestPressureSolverUsesTsacoreOnCellOrderAndResetsOnNewPacket();
        TestPressureSolverUsesTsacoreInCellOrder();
        TestLookaheadHoverGateClearsPressureAndDecision();
        TestLookaheadHoverGateBlocksPostPressureReinjection();
        TestBtPressSignalSuppressEnterClearsPressure();
        TestBtPressSignalSuppressSkipsEdgeFrames();
        TestBtPressSignalSuppressLatchExitsOnlyAboveThreshold();
        TestFakePressureDecreaseBuckets();
        TestBtFreqShiftDebounceSuppressesFakePressure();
        TestSingleEdgeLowSignalSuppressesPressure();
        TestBothEdgeEnterUsesTwoThirdsThreshold();
        TestEdgeLatchExitsOnlyAboveExitThreshold();
        std::cout << "[TEST] Stylus HPP3 post-pressure tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
