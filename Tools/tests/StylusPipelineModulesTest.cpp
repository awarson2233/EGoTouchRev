#include "SolverTypes.h"
#include "StylusSolver/NoiseGate.hpp"
#include "StylusSolver/StylusCoordinateFilter.hpp"
#include "StylusSolver/StylusFrameParser.hpp"
#include "StylusSolver/StylusOutputGate.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

using Asa::StylusFrameClass;
using Asa::StylusFrameParser;
using Solvers::HeatmapFrame;
using Solvers::StylusFrameData;
using Solvers::StylusFrameState;

constexpr size_t kSlaveHeaderBytes = StylusFrameParser::kSlaveHeaderBytes;
constexpr size_t kSlaveWordCount = StylusFrameParser::kSlaveWordCount;
constexpr size_t kSlaveFrameBytes = kSlaveHeaderBytes + kSlaveWordCount * 2;
constexpr size_t kFrameRawOffset = StylusFrameParser::kFrameRawOffset;
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

// The current rollout still finalizes through legacy mirrors, so tests project
// the returned stylus state into the target contract before asserting on it.
StylusFrameData ContractView(const StylusFrameData& stylus) {
    StylusFrameData view = stylus;
    view.SyncContractFromLegacyFields();
    return view;
}

void TestStylusFrameParserClassifiesShortFrame() {
    const std::vector<uint8_t> raw(6, 0xAB);
    const auto parsed = StylusFrameParser::Parse(raw, false);
    Require(parsed.frameClass == StylusFrameClass::ShortFrame,
            "short raw data should classify as short frame");
    Require(!parsed.valid, "short frame should not be valid");
    Require(!parsed.slaveValid, "short frame should not claim slave validity");
}

void TestStylusFrameParserClassifiesNoSignalFrame() {
    const auto raw = BuildSlaveFrame(0x1234, 0x00FF, 0x00FF, {});
    const auto parsed = StylusFrameParser::Parse(raw, false);
    Require(parsed.frameClass == StylusFrameClass::NoSignal,
            "sentinel anchors should classify as no-signal");
    Require(parsed.slaveValid, "full no-signal frame should still report slave validity");
    Require(parsed.status == 0x1234, "parser should preserve raw status");
}

void TestStylusFrameParserClassifiesChecksumFail() {
    std::vector<uint8_t> raw(kSlaveFrameBytes, 0xFF);
    raw[0] = 0x78;
    raw[1] = 0x56;
    const auto parsed = StylusFrameParser::Parse(raw, true);
    Require(parsed.frameClass == StylusFrameClass::ParseFail,
            "checksum failure should classify as parse fail");
    Require(parsed.checksumFailed, "checksum-fail classification should preserve checksum flag");
}

void TestStylusFrameParserClassifiesValidFrame() {
    const auto raw = BuildSlaveFrame(
        0x4321,
        10,
        12,
        MakeCrossGrid(16000, 14000, 12000, 10000),
        10,
        12,
        MakeCrossGrid(6000, 5000, 4000, 3000));

    const auto parsed = StylusFrameParser::Parse(raw, false);
    Require(parsed.frameClass == StylusFrameClass::Valid,
            "full frame with TX1 data should classify as valid");
    Require(parsed.valid, "valid frame should set valid=true");
    Require(parsed.gridData.tx1.valid, "valid frame should decode TX1 grid");
}

void TestStylusFrameParserProcessSeedsContractInputView() {
    const auto raw = BuildCombinedFrameFromSlave(
        BuildSlaveFrame(0x4321,
                        10,
                        12,
                        MakeCrossGrid(16000, 14000, 12000, 10000),
                        10,
                        12,
                        MakeCrossGrid(6000, 5000, 4000, 3000)));

    HeatmapFrame frame{};
    frame.rawPtr = raw.data();
    frame.rawLen = raw.size();
    StylusFrameState state(frame, /*sensorRows=*/40, /*sensorCols=*/60, /*anchorCenterOffset=*/4);

    const auto parsed = StylusFrameParser{}.Process(state, false);
    const auto stylus = ContractView(state.stylus);

    Require(parsed.frameClass == StylusFrameClass::Valid,
            "state parser should still classify valid frame");
    Require(stylus.input.slaveValid,
            "state parser should seed slave-valid input for contract projection");
    Require(stylus.input.status == 0x4321,
            "state parser should seed status into the contract input view");
    Require(stylus.input.tx1BlockValid,
            "state parser should seed tx1-valid into the contract input view");
    Require(stylus.input.tx2BlockValid,
            "state parser should seed tx2-valid into the contract input view");
    Require(!stylus.output.valid,
            "parser-only stage should not claim a solved stylus output");
}

void TestStylusFrameDataSyncContractFromLegacyFields() {
    StylusFrameData stylus{};
    stylus.slaveValid = true;
    stylus.checksumOk = false;
    stylus.slaveWordOffset = 7;
    stylus.checksum16 = 0x55AA;
    stylus.tx1BlockValid = true;
    stylus.tx2BlockValid = false;
    stylus.status = 0x2468;
    stylus.pressure = 333;
    stylus.tipSwitchActive = true;
    stylus.pipelineStage = 4;
    stylus.recheckEnabled = true;
    stylus.recheckPassed = false;
    stylus.recheckOverlap = true;
    stylus.recheckThreshold = 700;
    stylus.recheckThresholdMulti = 1200;
    stylus.touchNullLike = true;
    stylus.touchSuppressActive = true;
    stylus.touchSuppressFrames = 3;
    stylus.signalX = 1600;
    stylus.signalY = 900;
    stylus.maxRawPeak = 1600;
    stylus.point.valid = true;
    stylus.point.x = 1234.0f;
    stylus.point.y = 2345.0f;
    stylus.point.pressure = 111;
    stylus.point.confidence = 0.75f;
#if EGOTOUCH_DIAG
    stylus.diag.anchorRow = 9;
    stylus.diag.btSeq = 5;
#endif

    stylus.SyncContractFromLegacyFields();

    Require(stylus.input.slaveValid, "input view should mirror slave validity");
    Require(!stylus.input.checksumOk, "input view should mirror checksum status");
    Require(stylus.input.status == 0x2468, "input view should mirror status");
    Require(stylus.input.tx1BlockValid, "input view should mirror tx1 validity");
    Require(!stylus.input.tx2BlockValid, "input view should mirror tx2 validity");
    Require(stylus.output.valid, "output view should mirror point validity");
    Require(stylus.output.inRange, "output view should treat valid point as in-range");
    Require(stylus.output.tipDown, "output view should mirror writing state");
    Require(stylus.output.pressure == 333, "output view should mirror final pressure");
    Require(stylus.output.pipelineStage == 4, "output view should mirror pipeline stage");
    Require(stylus.output.point.valid, "output point should stay valid");
    Require(stylus.output.point.pressure == 333,
            "output point pressure should be normalized from final pressure");
    Require(stylus.interop.recheckEnabled, "interop should mirror recheck enable");
    Require(!stylus.interop.recheckPassed, "interop should mirror recheck result");
    Require(stylus.interop.recheckOverlap, "interop should mirror overlap state");
    Require(stylus.interop.recheckThreshold == 700,
            "interop should mirror recheck threshold");
    Require(stylus.interop.recheckThresholdMulti == 1200,
            "interop should mirror multi-touch threshold");
    Require(stylus.interop.touchNullLike, "interop should mirror touch-null hint");
    Require(stylus.interop.touchSuppressActive,
            "interop should mirror touch suppression state");
    Require(stylus.interop.touchSuppressFrames == 3,
            "interop should mirror suppression hold count");
    Require(stylus.interop.signalX == 1600, "interop should mirror signalX");
    Require(stylus.interop.signalY == 900, "interop should mirror signalY");
    Require(stylus.interop.maxRawPeak == 1600, "interop should mirror max raw peak");
#if EGOTOUCH_DIAG
    Require(stylus.debug.parse.slaveValid,
            "debug parse snapshot should mirror slave validity");
    Require(stylus.debug.parse.status == 0x2468,
            "debug parse snapshot should mirror status");
    Require(stylus.debug.coord.anchorRow == 9,
            "debug coord snapshot should mirror diagnostics payload");
    Require(stylus.debug.coord.btSeq == 5,
            "debug coord snapshot should preserve BT sequence");
#endif
}

void TestStylusFrameDataSyncLegacyFieldsFromContract() {
    StylusFrameData stylus{};
    stylus.input.slaveValid = true;
    stylus.input.checksumOk = false;
    stylus.input.slaveWordOffset = 7;
    stylus.input.checksum16 = 0xAA55;
    stylus.input.tx1BlockValid = true;
    stylus.input.tx2BlockValid = true;
    stylus.input.status = 0x9876;
    stylus.output.valid = true;
    stylus.output.inRange = true;
    stylus.output.tipDown = true;
    stylus.output.pressure = 321;
    stylus.output.pipelineStage = 2;
    stylus.output.point.valid = true;
    stylus.output.point.x = 2048.0f;
    stylus.output.point.y = 1024.0f;
    stylus.output.point.pressure = 321;
    stylus.interop.recheckEnabled = true;
    stylus.interop.recheckPassed = false;
    stylus.interop.recheckOverlap = true;
    stylus.interop.recheckThreshold = 888;
    stylus.interop.recheckThresholdMulti = 1333;
    stylus.interop.touchNullLike = true;
    stylus.interop.touchSuppressActive = true;
    stylus.interop.touchSuppressFrames = 6;
    stylus.interop.signalX = 1700;
    stylus.interop.signalY = 1200;
    stylus.interop.maxRawPeak = 1700;
#if EGOTOUCH_DIAG
    stylus.debug.coord.rawPressure = 456;
    stylus.debug.coord.btSeq = 7;
#endif

    stylus.SyncLegacyFieldsFromContract();

    Require(stylus.slaveValid, "legacy slaveValid should mirror contract input");
    Require(!stylus.checksumOk, "legacy checksum flag should mirror contract input");
    Require(stylus.status == 0x9876, "legacy status should mirror contract input");
    Require(stylus.tx1BlockValid, "legacy tx1 validity should mirror contract input");
    Require(stylus.tx2BlockValid, "legacy tx2 validity should mirror contract input");
    Require(stylus.pressure == 321, "legacy pressure should mirror contract output");
    Require(stylus.tipSwitchActive, "legacy tip switch should mirror contract output");
    Require(stylus.point.valid, "legacy point should mirror contract output");
    Require(stylus.point.pressure == 321,
            "legacy point pressure should mirror contract output pressure");
    Require(stylus.pipelineStage == 2, "legacy pipeline stage should mirror contract output");
    Require(stylus.recheckEnabled, "legacy recheck flag should mirror contract interop");
    Require(!stylus.recheckPassed, "legacy recheck result should mirror contract interop");
    Require(stylus.recheckOverlap, "legacy overlap state should mirror contract interop");
    Require(stylus.recheckThreshold == 888,
            "legacy threshold should mirror contract interop");
    Require(stylus.recheckThresholdMulti == 1333,
            "legacy multi threshold should mirror contract interop");
    Require(stylus.touchNullLike, "legacy touch-null hint should mirror contract interop");
    Require(stylus.touchSuppressActive,
            "legacy touch suppression should mirror contract interop");
    Require(stylus.touchSuppressFrames == 6,
            "legacy suppression hold should mirror contract interop");
    Require(stylus.signalX == 1700, "legacy signalX should mirror contract interop");
    Require(stylus.signalY == 1200, "legacy signalY should mirror contract interop");
    Require(stylus.maxRawPeak == 1700,
            "legacy max raw peak should mirror contract interop");
#if EGOTOUCH_DIAG
    Require(stylus.diag.rawPressure == 456,
            "legacy diagnostics should mirror contract debug payload");
    Require(stylus.diag.btSeq == 7,
            "legacy diagnostics should preserve BT sequence");
#endif
}

void TestStylusCoordinateFilterProjectsLegacyOutputIntoContract() {
    StylusFrameData stylus{};
    stylus.slaveValid = true;
    stylus.tx1BlockValid = true;
    stylus.status = 0x1357;
    stylus.pressure = 222;
    stylus.tipSwitchActive = true;
    stylus.pipelineStage = 4;
    stylus.point.valid = true;
    stylus.point.x = 512.0f;
    stylus.point.y = 768.0f;
    stylus.point.pressure = 111;
    stylus.point.confidence = 0.8f;

    Asa::StylusCoordinateFilter filter;
    filter.Process(stylus);

    Require(stylus.input.status == 0x1357,
            "coordinate filter should keep contract input aligned");
    Require(stylus.output.valid,
            "coordinate filter should project legacy point validity into output");
    Require(stylus.output.tipDown,
            "coordinate filter should project writing state into output");
    Require(stylus.output.pressure == 222,
            "coordinate filter should project final pressure into output");
    Require(stylus.output.point.valid,
            "coordinate filter should keep output point valid");
    Require(stylus.output.point.pressure == 222,
            "coordinate filter should normalize point pressure in output");
}

void TestStylusOutputGateProjectsLegacyInteropIntoContract() {
    StylusFrameData stylus{};
    stylus.recheckEnabled = true;
    stylus.recheckPassed = false;
    stylus.recheckOverlap = true;
    stylus.recheckThreshold = 640;
    stylus.recheckThresholdMulti = 960;
    stylus.touchNullLike = true;
    stylus.touchSuppressActive = true;
    stylus.touchSuppressFrames = 4;
    stylus.signalX = 1400;
    stylus.signalY = 700;
    stylus.maxRawPeak = 1400;

    Asa::StylusOutputGate gate;
    gate.Process(stylus);

    Require(stylus.interop.recheckEnabled,
            "output gate should project recheck enable into interop");
    Require(!stylus.interop.recheckPassed,
            "output gate should project recheck result into interop");
    Require(stylus.interop.recheckOverlap,
            "output gate should project overlap into interop");
    Require(stylus.interop.recheckThreshold == 640,
            "output gate should project recheck threshold into interop");
    Require(stylus.interop.recheckThresholdMulti == 960,
            "output gate should project multi threshold into interop");
    Require(stylus.interop.touchNullLike,
            "output gate should project touch-null hint into interop");
    Require(stylus.interop.touchSuppressActive,
            "output gate should project touch suppression into interop");
    Require(stylus.interop.touchSuppressFrames == 4,
            "output gate should project suppression hold into interop");
    Require(stylus.interop.signalX == 1400,
            "output gate should project signalX into interop");
    Require(stylus.interop.signalY == 700,
            "output gate should project signalY into interop");
    Require(stylus.interop.maxRawPeak == 1400,
            "output gate should project max raw peak into interop");
}

void TestNoiseGateMirrorsOwnedRecheckEnabledIntoContractInterop() {
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
    auto stylus = ContractView(state.stylus);
    Require(!stylus.interop.recheckEnabled,
            "noise gate should project disabled recheck into interop");

    gate.recheckEnabled = true;
    gate.Process(state);
    stylus = ContractView(state.stylus);
    Require(stylus.interop.recheckEnabled,
            "noise gate should project enabled recheck into interop");
    Require(stylus.interop.recheckPassed,
            "noise gate should keep recheck result projected into interop");
}

} // namespace

int main() {
    try {
        TestStylusFrameParserClassifiesShortFrame();
        TestStylusFrameParserClassifiesNoSignalFrame();
        TestStylusFrameParserClassifiesChecksumFail();
        TestStylusFrameParserClassifiesValidFrame();
        TestStylusFrameParserProcessSeedsContractInputView();
        TestStylusFrameDataSyncContractFromLegacyFields();
        TestStylusFrameDataSyncLegacyFieldsFromContract();
        TestStylusCoordinateFilterProjectsLegacyOutputIntoContract();
        TestStylusOutputGateProjectsLegacyInteropIntoContract();
        TestNoiseGateMirrorsOwnedRecheckEnabledIntoContractInterop();
        std::cout << "[TEST] Stylus pipeline module tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
