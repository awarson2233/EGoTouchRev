#include "StylusSolver/PipelineUtils.hpp"
#include "StylusSolver/PressureSolver.hpp"
#include "StylusSolver/StylusInputParser.hpp"
#include "StylusSolver/NoiseGate.hpp"
#include "StylusSolver/StylusStateController.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

using Asa::StylusFrameClass;
using Asa::StylusInputParser;
using Solvers::StylusFrameData;
using Solvers::StylusFrameState;
using Solvers::StylusPacketRoute;
using Solvers::HeatmapFrame;

constexpr size_t kSlaveHeaderBytes = StylusInputParser::kSlaveHeaderBytes;
constexpr size_t kSlaveWordCount = StylusInputParser::kSlaveWordCount;
constexpr size_t kSlaveFrameBytes = kSlaveHeaderBytes + kSlaveWordCount * 2;
constexpr size_t kFrameRawOffset = StylusInputParser::kFrameRawOffset;
constexpr int kGridDim = 9;

void Require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::array<uint16_t, kGridDim * kGridDim> MakeCrossGrid(
    uint16_t center,
    uint16_t nearAxis,
    uint16_t diag,
    uint16_t farAxis,
    int peakRow = 4,
    int peakCol = 4) {
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

std::vector<uint8_t> BuildSlaveFrame(
    uint16_t status,
    uint16_t tx1AnchorRow,
    uint16_t tx1AnchorCol,
    const std::array<uint16_t, kGridDim * kGridDim>& tx1Grid,
    uint16_t tx2AnchorRow = 0,
    uint16_t tx2AnchorCol = 0,
    const std::array<uint16_t, kGridDim * kGridDim>& tx2Grid = {}) {
    std::vector<uint8_t> raw(kSlaveFrameBytes, 0);
    raw[0] = static_cast<uint8_t>(status & 0xFFu);
    raw[1] = static_cast<uint8_t>((status >> 8) & 0xFFu);

    auto writeWord = [&](size_t wordIndex, uint16_t value) {
        const size_t off = kSlaveHeaderBytes + wordIndex * 2;
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

std::vector<uint8_t> BuildCombinedFrameFromSlave(const std::vector<uint8_t>& slaveRaw) {
    std::vector<uint8_t> raw(kFrameRawOffset + slaveRaw.size(), 0);
    std::copy(slaveRaw.begin(), slaveRaw.end(), raw.begin() + static_cast<std::ptrdiff_t>(kFrameRawOffset));
    return raw;
}

struct ParserStateHarness {
    std::vector<uint8_t> raw;
    HeatmapFrame frame{};
    StylusFrameState state;

    explicit ParserStateHarness(std::vector<uint8_t> rawIn)
        : raw(std::move(rawIn))
        , state(frame, /*sensorRows=*/40, /*sensorCols=*/60, /*anchorCenterOffset=*/4) {
        frame.rawPtr = raw.data();
        frame.rawLen = raw.size();
    }
};

void RequireParserFlow(const StylusFrameState& state,
                       StylusFrameClass expectedClass,
                       StylusPacketRoute expectedRoute,
                       uint8_t expectedStage,
                       bool expectedTerminal,
                       bool expectedClearCommitted,
                       bool expectedResetPost,
                       bool expectedResetNoise,
                       const char* context) {
    Require(state.parse.frameClass == expectedClass, context);
    Require(state.flow.packetRoute == expectedRoute, context);
    Require(state.flow.pipelineStage == expectedStage, context);
    Require(state.flow.terminal == expectedTerminal, context);
    Require(state.flow.clearCommitted == expectedClearCommitted, context);
    Require(state.flow.resetPost == expectedResetPost, context);
    Require(state.flow.resetNoise == expectedResetNoise, context);
}

void TestStylusInputParserClassifiesShortFrame() {
    const std::vector<uint8_t> raw(6, 0xAB);
    const auto parsed = StylusInputParser::Parse(raw, false);
    Require(parsed.frameClass == StylusFrameClass::ShortFrame,
            "short raw data should classify as short frame");
    Require(!parsed.valid, "short frame should not be valid");
    Require(!parsed.slaveValid, "short frame should not claim slave validity");
}

void TestStylusInputParserClassifiesNoSignalFrame() {
    const auto raw = BuildSlaveFrame(0x1234, 0x00FF, 0x00FF, {});
    const auto parsed = StylusInputParser::Parse(raw, false);
    Require(parsed.frameClass == StylusFrameClass::NoSignal,
            "sentinel anchors should classify as no-signal");
    Require(parsed.slaveValid, "full no-signal frame should still report slave validity");
    Require(parsed.status == 0x1234, "parser should preserve raw status");
}

void TestStylusInputParserClassifiesChecksumFail() {
    std::vector<uint8_t> raw(kSlaveFrameBytes, 0xFF);
    raw[0] = 0x78;
    raw[1] = 0x56;
    const auto parsed = StylusInputParser::Parse(raw, true);
    Require(parsed.frameClass == StylusFrameClass::ParseFail,
            "checksum failure should classify as parse fail");
    Require(parsed.checksumFailed, "checksum-fail classification should preserve checksum flag");
}

void TestStylusInputParserClassifiesValidFrame() {
    const auto raw = BuildSlaveFrame(
        0x4321,
        10,
        12,
        MakeCrossGrid(16000, 14000, 12000, 10000),
        10,
        12,
        MakeCrossGrid(6000, 5000, 4000, 3000));

    const auto parsed = StylusInputParser::Parse(raw, false);
    Require(parsed.frameClass == StylusFrameClass::Valid,
            "full frame with TX1 data should classify as valid");
    Require(parsed.valid, "valid frame should set valid=true");
    Require(parsed.gridData.tx1.valid, "valid frame should decode TX1 grid");
}

void TestStylusInputParserProcessStateClassifiesShortFrame() {
    ParserStateHarness harness(std::vector<uint8_t>(kFrameRawOffset + 6, 0xAB));
    const auto parsed = StylusInputParser{}.Process(harness.state, false);

    Require(parsed.frameClass == StylusFrameClass::ShortFrame,
            "state parser should classify short frame");
    Require(!harness.state.parse.valid, "short state parse should stay invalid");
    Require(!harness.state.stylus.slaveValid, "short state parse should clear slave validity");
    RequireParserFlow(harness.state,
                      StylusFrameClass::ShortFrame,
                      StylusPacketRoute::ParseFailure13,
                      /*expectedStage=*/1,
                      /*expectedTerminal=*/true,
                      /*expectedClearCommitted=*/true,
                      /*expectedResetPost=*/true,
                      /*expectedResetNoise=*/false,
                      "short state parse should map to parse-failure flow");
}

void TestStylusInputParserProcessStateClassifiesNoSignalFrame() {
    ParserStateHarness harness(BuildCombinedFrameFromSlave(
        BuildSlaveFrame(0x1234, 0x00FF, 0x00FF, {})));
    const auto parsed = StylusInputParser{}.Process(harness.state, false);

    Require(parsed.frameClass == StylusFrameClass::NoSignal,
            "state parser should classify no-signal frame");
    Require(harness.state.parse.slaveValid, "full no-signal frame should keep slave-valid flag");
    Require(harness.state.stylus.status == 0x1234, "state parser should preserve stylus status");
    RequireParserFlow(harness.state,
                      StylusFrameClass::NoSignal,
                      StylusPacketRoute::InvalidZeroState,
                      /*expectedStage=*/0,
                      /*expectedTerminal=*/true,
                      /*expectedClearCommitted=*/true,
                      /*expectedResetPost=*/true,
                      /*expectedResetNoise=*/true,
                      "no-signal state parse should map to invalid-zero flow");
}

void TestStylusInputParserProcessStateClassifiesChecksumFail() {
    ParserStateHarness harness(BuildCombinedFrameFromSlave(std::vector<uint8_t>(kSlaveFrameBytes, 0xFF)));
    harness.raw[kFrameRawOffset + 0] = 0x78;
    harness.raw[kFrameRawOffset + 1] = 0x56;

    const auto parsed = StylusInputParser{}.Process(harness.state, true);

    Require(parsed.frameClass == StylusFrameClass::ParseFail,
            "state parser should classify checksum failure");
    Require(harness.state.parse.checksumFailed, "checksum failure should reach parse state");
    Require(!harness.state.stylus.checksumOk, "checksum failure should clear stylus checksumOk");
    RequireParserFlow(harness.state,
                      StylusFrameClass::ParseFail,
                      StylusPacketRoute::ParseFailure13,
                      /*expectedStage=*/1,
                      /*expectedTerminal=*/true,
                      /*expectedClearCommitted=*/true,
                      /*expectedResetPost=*/true,
                      /*expectedResetNoise=*/false,
                      "checksum failure should map to parse-failure flow");
}

void TestStylusInputParserProcessStateClassifiesValidFrame() {
    ParserStateHarness harness(BuildCombinedFrameFromSlave(
        BuildSlaveFrame(0x4321,
                        10,
                        12,
                        MakeCrossGrid(16000, 14000, 12000, 10000),
                        10,
                        12,
                        MakeCrossGrid(6000, 5000, 4000, 3000))));

    const auto parsed = StylusInputParser{}.Process(harness.state, false);

    Require(parsed.frameClass == StylusFrameClass::Valid,
            "state parser should classify valid frame");
    Require(harness.state.parse.valid, "valid state parse should mark parse.valid");
    Require(harness.state.stylus.tx1BlockValid, "valid state parse should mark tx1 block valid");
    Require(harness.state.stylus.tx2BlockValid, "valid state parse should mark tx2 block valid");
    RequireParserFlow(harness.state,
                      StylusFrameClass::Valid,
                      StylusPacketRoute::Valid,
                      /*expectedStage=*/0,
                      /*expectedTerminal=*/false,
                      /*expectedClearCommitted=*/false,
                      /*expectedResetPost=*/false,
                      /*expectedResetNoise=*/false,
                      "valid state parse should map to valid flow");
}

void TestStylusInputParserUsesOwnedChecksumConfig() {
    ParserStateHarness harness(BuildCombinedFrameFromSlave(std::vector<uint8_t>(kSlaveFrameBytes, 0xFF)));
    harness.raw[kFrameRawOffset + 0] = 0x78;
    harness.raw[kFrameRawOffset + 1] = 0x56;

    StylusInputParser parser;
    parser.enableSlaveChecksum = true;
    const auto parsed = parser.Process(harness.state);

    Require(parsed.frameClass == StylusFrameClass::ParseFail,
            "owned checksum config should drive parse-fail classification");
    Require(harness.state.parse.checksumFailed,
            "owned checksum config should reach parse state");
}

void TestStylusStateControllerUsesOwnedThresholdConfig() {
    HeatmapFrame frame{};
    StylusFrameState state(frame, /*sensorRows=*/40, /*sensorCols=*/60, /*anchorCenterOffset=*/4);
    Asa::StylusStateController controller;
    Asa::PressureSolver pressureSolver;
    Asa::PenStateMachine penStateMachine;

    controller.tx1InkEnterThreshold = 900;
    controller.tx1LiftSuspiciousThreshold = 700;
    controller.tx1LiftAbsoluteThreshold = 500;

    state.tx1.globalCoor.valid = true;
    state.tx1.globalCoor.dim1 = 1234;
    state.tx1.globalCoor.dim2 = 2345;
    state.parse.gridData.tx1.valid = true;
    state.signal.recheckPassed = true;
    state.signal.tx1Composite = 1000;
    state.signal.tx2Composite = 200;

    const auto result = controller.Process(
        state,
        /*hasCommittedFrame=*/false,
        pressureSolver,
        penStateMachine);

    Require(result.stateOutput.valid,
            "owned thresholds should still yield a valid state output");
    Require(state.lifecycle.authoritativeDown,
            "owned enter threshold should mark authoritative down");
    Require(state.lifecycle.keepInkAlive,
            "owned suspicious threshold should keep ink alive");
}

void TestNoiseGateMirrorsOwnedRecheckEnabledToStylus() {
    HeatmapFrame frame{};
    StylusFrameState state(frame, /*sensorRows=*/40, /*sensorCols=*/60, /*anchorCenterOffset=*/4);
    Asa::NoiseGate gate;

    state.signal.signalX = 1200;
    state.signal.signalY = 400;
    state.signal.maxRawPeak = 1200;
    state.signal.recheckThreshold = 800;
    state.signal.recheckThresholdMulti = 1200;
    state.signal.overlapLike = false;

    gate.recheckEnabled = false;
    gate.Process(state);
    Require(!state.stylus.recheckEnabled,
            "noise gate should mirror disabled recheck config to stylus output");

    gate.recheckEnabled = true;
    gate.Process(state);
    Require(state.stylus.recheckEnabled,
            "noise gate should mirror enabled recheck config to stylus output");
}

void TestStylusStateControllerMirrorsStylusOutputs() {
    HeatmapFrame frame{};
    StylusFrameState state(frame, /*sensorRows=*/40, /*sensorCols=*/60, /*anchorCenterOffset=*/4);
    Asa::PenStateMachine penStateMachine;
    Asa::PenFrameEvidence evidence{};
    Asa::PenUpdateResult penUpdate{};

    evidence.coordValid = true;
    evidence.tx1BlockValid = true;
    evidence.activeStylusPresent = true;
    evidence.hoverSignalPresent = true;
    evidence.authoritativeDown = true;
    evidence.keepInkAlive = true;
    evidence.recheckPassed = true;
    evidence.curDim1 = 1024;
    evidence.curDim2 = 2048;
    (void)penStateMachine.Process(evidence);

    state.lifecycle.btSample = Asa::BtPressureSample{222, 7, true};
    state.lifecycle.mappedPressure = 111;
    penUpdate.output.outputPressure = 333;
    penUpdate.output.noPressInkActive = true;
    penUpdate.output.tipSwitchActive = true;
    penUpdate.output.sustainOutput = true;
    penUpdate.output.fastLiftOutput = true;

    Asa::StylusStateController::ApplyStylusStateMirrors(state, penUpdate, penStateMachine);

    Require(state.stylus.point.rawPressure == 222,
            "stylus mirror should expose BT raw pressure from lifecycle sample");
    Require(state.stylus.point.mappedPressure == 111,
            "stylus mirror should expose mapped pressure from lifecycle");
    Require(state.stylus.pressure == 333 && state.stylus.point.pressure == 333,
            "stylus mirror should expose output pressure on frame and point");
    Require(state.stylus.noPressInkActive,
            "stylus mirror should forward pen-state no-press output");
    Require(state.stylus.tipSwitchActive,
            "stylus mirror should forward pen-state tip-switch output");
    Require(state.stylus.sustainOutput,
            "stylus mirror should forward pen-state sustain output");
    Require(state.stylus.fastLiftOutput,
            "stylus mirror should forward pen-state fast-lift output");
    Require(state.stylus.animState == 2,
            "stylus mirror should source animation state from the pen state machine");
}

void TestStylusStateControllerMirrorsTerminalAnimState() {
    HeatmapFrame frame{};
    StylusFrameState state(frame, /*sensorRows=*/40, /*sensorCols=*/60, /*anchorCenterOffset=*/4);
    Asa::PenStateMachine penStateMachine;
    Asa::PenFrameEvidence down{};
    Asa::PenFrameEvidence up{};

    down.coordValid = true;
    down.tx1BlockValid = true;
    down.activeStylusPresent = true;
    down.hoverSignalPresent = true;
    down.authoritativeDown = true;
    down.keepInkAlive = true;
    down.recheckPassed = true;
    down.curDim1 = 512;
    down.curDim2 = 768;
    (void)penStateMachine.Process(down);

    up.noSignal = true;
    up.immediateRelease = true;
    up.recheckPassed = true;
    (void)penStateMachine.Process(up);

    Asa::StylusStateController::ApplyTerminalStylusStateMirrors(state, penStateMachine);
    Require(state.stylus.animState == 0,
            "terminal stylus mirror should copy the current pen-state animation");
}

} // namespace

int main() {
    try {
        TestStylusInputParserClassifiesShortFrame();
        TestStylusInputParserClassifiesNoSignalFrame();
        TestStylusInputParserClassifiesChecksumFail();
        TestStylusInputParserClassifiesValidFrame();
        TestStylusInputParserProcessStateClassifiesShortFrame();
        TestStylusInputParserProcessStateClassifiesNoSignalFrame();
        TestStylusInputParserProcessStateClassifiesChecksumFail();
        TestStylusInputParserProcessStateClassifiesValidFrame();
        TestStylusInputParserUsesOwnedChecksumConfig();
        TestStylusStateControllerUsesOwnedThresholdConfig();
        TestNoiseGateMirrorsOwnedRecheckEnabledToStylus();
        TestStylusStateControllerMirrorsStylusOutputs();
        TestStylusStateControllerMirrorsTerminalAnimState();
        std::cout << "[TEST] Stylus pipeline module tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
