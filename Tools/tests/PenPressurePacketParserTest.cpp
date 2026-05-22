#include "btmcu/PenPressurePacketParser.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

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
    Require(parsed->press[0] == parsed->rawPress[0], "12-bit p0 should be unscaled");
    Require(parsed->press[1] == parsed->rawPress[1], "12-bit p1 should be unscaled");
    Require(parsed->pressureMax == 4095, "pressure max should remain 4095");
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
    Require(parsed->rawPress[3] == 0x4000 && parsed->press[3] == 0x1000,
            "14-bit p3 should be divided by 4");
    Require(parsed->pressureMode == Himax::Pen::PenPressureRangeMode::Raw14Bit16382,
            "pressure mode should be copied");
}

} // namespace

int main() {
    try {
        TestInvalidPacketsAreRejected();
        TestRaw12BitPacketParses();
        TestRaw14BitPacketScales();
        std::cout << "[TEST] Pen pressure packet parser tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
