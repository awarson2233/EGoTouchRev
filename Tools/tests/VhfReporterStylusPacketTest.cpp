#include "vhf/VhfReporter.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

using Solvers::HeatmapFrame;
using Solvers::StylusPacketRoute;

void Require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

HeatmapFrame MakeValidStylusFrame() {
    HeatmapFrame frame{};
    frame.stylus.packetRoute = StylusPacketRoute::Valid;
    frame.stylus.point.valid = true;
    frame.stylus.point.x = 12.0f * 1024.0f;
    frame.stylus.point.y = 18.0f * 1024.0f;
    frame.stylus.point.tiltX = 7;
    frame.stylus.point.tiltY = -3;
    frame.stylus.pressure = 321;
    frame.stylus.tipSwitchActive = true;
    return frame;
}

void TestDispatchStylusBuildsAndBackfillsValidPacket() {
    VhfReporter reporter;
    reporter.SetEnabled(false);
    reporter.SetStylusPacketSensorRows(40);
    reporter.SetStylusPacketSensorCols(60);

    auto frame = MakeValidStylusFrame();
    reporter.DispatchStylus(frame);

    Require(frame.stylus.packet.valid,
            "VHF should backfill a valid stylus packet before dispatch");
    Require(frame.stylus.packet.length == 13,
            "VHF-built stylus packet should use 13-byte report length");
    Require(frame.stylus.packet.bytes[0] == 0x08,
            "VHF-built stylus packet should keep stylus report id");
    Require(frame.stylus.packet.bytes[1] == 0x21,
            "VHF-built valid stylus packet should encode TipSwitch and InRange");
    Require(frame.stylus.diag.vhfPenState == 0x21,
            "VHF should update diagnostics from the raw built packet state");
}

void TestDispatchStylusBuildsInvalidZeroStatePacketWhenEnabled() {
    VhfReporter reporter;
    reporter.SetEnabled(false);
    reporter.SetStylusPacketEmitWhenInvalid(true);

    HeatmapFrame frame{};
    frame.stylus.packetRoute = StylusPacketRoute::InvalidZeroState;
    reporter.DispatchStylus(frame);

    Require(frame.stylus.packet.valid,
            "invalid-zero route should still build a packet when emitWhenInvalid is enabled");
    Require(frame.stylus.packet.length == 13,
            "invalid-zero route should build a 13-byte packet");
    Require(frame.stylus.packet.bytes[1] == 0,
            "invalid-zero route should clear stylus state bits");
    Require(frame.stylus.diag.vhfPenState == 0,
            "invalid-zero route should write neutral diagnostics state");
}

void TestDispatchStylusBuildsParseFailurePacketWhenEnabled() {
    VhfReporter reporter;
    reporter.SetEnabled(false);
    reporter.SetStylusPacketEmitWhenInvalid(true);

    HeatmapFrame frame{};
    frame.stylus.packetRoute = StylusPacketRoute::ParseFailure13;
    reporter.DispatchStylus(frame);

    Require(frame.stylus.packet.valid,
            "parse-failure route should still build a packet when emitWhenInvalid is enabled");
    Require(frame.stylus.packet.length == 13,
            "parse-failure route should build a 13-byte packet");
    Require(frame.stylus.diag.vhfPenState == 0,
            "parse-failure route should keep neutral diagnostics state");
}

void TestDispatchStylusSuppressesInvalidPacketWhenDisabled() {
    VhfReporter reporter;
    reporter.SetEnabled(false);
    reporter.SetStylusPacketEmitWhenInvalid(false);

    HeatmapFrame frame{};
    frame.stylus.packetRoute = StylusPacketRoute::InvalidZeroState;
    reporter.DispatchStylus(frame);

    Require(!frame.stylus.packet.valid,
            "invalid routes should stay suppressed when emitWhenInvalid is disabled");
    Require(frame.stylus.packet.length == 13,
            "suppressed invalid packet should preserve HID report length");
    Require(frame.stylus.diag.vhfPenState == 0,
            "suppressed invalid packet should keep neutral diagnostics state");
}

void TestDispatchStylusDoesNotWriteBackEraserTransform() {
    VhfReporter reporter;
    reporter.SetEnabled(false);
    reporter.SetEraserState(1);

    auto frame = MakeValidStylusFrame();
    reporter.DispatchStylus(frame);

    Require(frame.stylus.packet.valid,
            "valid frame should still build a packet before any eraser transform");
    Require(frame.stylus.packet.bytes[1] == 0x21,
            "eraser post-transform must not be written back into frame.stylus.packet");
    Require(frame.stylus.diag.vhfPenState == 0x21,
            "diagnostics should reflect the raw packet, not the transformed write buffer");
}

void TestDispatchStylusBackfillsEvenWhenWriteDisabled() {
    VhfReporter reporter;
    reporter.SetEnabled(true);

    auto frame = MakeValidStylusFrame();
    reporter.DispatchStylus(frame, false);

    Require(frame.stylus.packet.valid,
            "write-disabled dispatch should still backfill the stylus packet");
    Require(frame.stylus.packet.bytes[1] == 0x21,
            "write-disabled dispatch should still encode the raw pen state");
    Require(frame.stylus.diag.vhfPenState == 0x21,
            "write-disabled dispatch should still publish diagnostics state");
}

} // namespace

int main() {
    try {
        TestDispatchStylusBuildsAndBackfillsValidPacket();
        TestDispatchStylusBuildsInvalidZeroStatePacketWhenEnabled();
        TestDispatchStylusBuildsParseFailurePacketWhenEnabled();
        TestDispatchStylusSuppressesInvalidPacketWhenDisabled();
        TestDispatchStylusDoesNotWriteBackEraserTransform();
        TestDispatchStylusBackfillsEvenWhenWriteDisabled();
        std::cout << "[TEST] VHF reporter stylus packet tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
