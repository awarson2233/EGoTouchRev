#include "btmcu/PenPressurePacketParser.h"
#include "TestRequire.h"

#include <array>
#include <cstdint>
#include <iostream>

namespace {

using DeviceTests::Require;

void TestInvalidPacketsAreRejected() {
    const std::array<uint8_t, 11> wrongType{
        0x54, 0x12, 0x34, 0x01, 0x00, 0x34, 0x12, 0xFF, 0x0F, 0x00, 0x40,
    };
    Require(!Himax::Pen::TryParsePenPressurePacket(
                wrongType,
                Himax::Pen::PenPressureRangeMode::Raw12Bit4096).has_value(),
            "non-U pressure packet should be rejected");

    const std::array<uint8_t, 10> shortPacket{
        0x55, 0x12, 0x34, 0x01, 0x00, 0x34, 0x12, 0xFF, 0x0F, 0x00,
    };
    Require(!Himax::Pen::TryParsePenPressurePacket(
                shortPacket,
                Himax::Pen::PenPressureRangeMode::Raw12Bit4096).has_value(),
            "short pressure packet should be rejected");
}

void TestRaw12BitPacketParses() {
    const std::array<uint8_t, 11> packet{
        0x55, 0x12, 0x34, 0x01, 0x00, 0x34, 0x12, 0xFF, 0x0F, 0x00, 0x40,
    };

    auto parsed = Himax::Pen::TryParsePenPressurePacket(
        packet,
        Himax::Pen::PenPressureRangeMode::Raw12Bit4096);
    Require(parsed.has_value(), "valid pressure packet should parse");
    Require(parsed->reportType == 0x55, "report type should be copied");
    Require(parsed->freq1 == 0x12, "freq1 should be copied");
    Require(parsed->freq2 == 0x34, "freq2 should be copied");
    Require(parsed->rawPress[0] == 0x0001, "p0 should parse little-endian");
    Require(parsed->rawPress[1] == 0x1234, "p1 should parse little-endian");
    Require(parsed->rawPress[2] == 0x0FFF, "p2 should parse little-endian");
    Require(parsed->rawPress[3] == 0x4000, "p3 should parse little-endian");
    Require(parsed->press[0] == parsed->rawPress[0], "12-bit p0 should be unscaled below max");
    Require(parsed->press[1] == parsed->pressureMax, "12-bit p1 above max should be clamped");
    Require(parsed->press[2] <= parsed->pressureMax && parsed->press[3] <= parsed->pressureMax,
            "all 12-bit scaled pressure values should be clamped to pressureMax");
    Require(parsed->pressureMax == 4095, "pressure max should remain 4095");
}

void TestTrailingBytesAreIgnored() {
    const std::array<uint8_t, 14> packet{
        0x55, 0xFF, 0x00, 0x00, 0x00, 0x04, 0x00, 0x08, 0x00, 0x0C, 0x00,
        0xAA, 0xBB, 0xCC,
    };

    auto parsed = Himax::Pen::TryParsePenPressurePacket(
        packet,
        Himax::Pen::PenPressureRangeMode::Raw12Bit4096);
    Require(parsed.has_value(), "valid pressure packet with trailing bytes should parse");
    Require(parsed->freq1 == 0xFF && parsed->freq2 == 0x00,
            "freq bytes should be copied exactly");
    Require(parsed->rawPress[0] == 0 && parsed->rawPress[1] == 4 &&
                parsed->rawPress[2] == 8 && parsed->rawPress[3] == 12,
            "parser should ignore trailing bytes after the first four pressure slots");
}

void TestRaw14BitPacketScales() {
    const std::array<uint8_t, 11> packet{
        0x55, 0x12, 0x34, 0x04, 0x00, 0x00, 0x10, 0xFC, 0x3F, 0x00, 0x40,
    };

    auto parsed = Himax::Pen::TryParsePenPressurePacket(
        packet,
        Himax::Pen::PenPressureRangeMode::Raw14Bit16382);
    Require(parsed.has_value(), "valid 14-bit pressure packet should parse");
    Require(parsed->rawPress[0] == 0x0004 && parsed->press[0] == 0x0001,
            "14-bit p0 should be divided by 4");
    Require(parsed->rawPress[1] == 0x1000 && parsed->press[1] == 0x0400,
            "14-bit p1 should be divided by 4");
    Require(parsed->rawPress[2] == 0x3FFC && parsed->press[2] == 0x0FFF,
            "14-bit p2 should be divided by 4");
    Require(parsed->rawPress[3] == 0x4000 && parsed->press[3] == parsed->pressureMax,
            "14-bit p3 should be divided by 4 and clamped to pressureMax");
    Require(parsed->pressureMode == Himax::Pen::PenPressureRangeMode::Raw14Bit16382,
            "pressure mode should be copied");
}

void TestRaw14BitEdgeScalingUsesIntegerDivision() {
    const std::array<uint8_t, 11> packet{
        0x55, 0x00, 0xFF, 0x00, 0x00, 0x03, 0x00, 0xFC, 0x3F, 0xFF, 0xFF,
    };

    auto parsed = Himax::Pen::TryParsePenPressurePacket(
        packet,
        Himax::Pen::PenPressureRangeMode::Raw14Bit16382);
    Require(parsed.has_value(), "14-bit edge pressure packet should parse");
    Require(parsed->freq1 == 0x00 && parsed->freq2 == 0xFF,
            "frequency bytes should preserve edge values");
    Require(parsed->press[0] == 0, "raw 0 should scale to 0");
    Require(parsed->press[1] == 0, "raw 3 should truncate to 0 when divided by 4");
    Require(parsed->press[2] == 0x0FFF, "raw 0x3FFC should scale to 4095");
    Require(parsed->rawPress[3] == 0xFFFF, "raw pressure should preserve original 0xFFFF value");
    Require(parsed->press[3] == parsed->pressureMax, "raw 0xFFFF should scale and clamp to pressureMax");
    for (int k = 0; k < 4; ++k) {
        Require(parsed->press[k] <= parsed->pressureMax,
                "all 14-bit scaled pressure values should be clamped to pressureMax");
    }
}

} // namespace

int main() {
    try {
        TestInvalidPacketsAreRejected();
        TestRaw12BitPacketParses();
        TestTrailingBytesAreIgnored();
        TestRaw14BitPacketScales();
        TestRaw14BitEdgeScalingUsesIntegerDivision();
        std::cout << "[TEST] Device Pen pressure packet parser tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
