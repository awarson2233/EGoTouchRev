#include "btmcu/PenUsbPacketBuilder.h"
#include "btmcu/PenUsbTypes.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void TestValidFactoryEventFrameParses() {
    const std::array<uint8_t, 12> packet{
        0x00, 0x00, 0x07, 0x00,
        0x01, 0x71, 0x00, 0x00,
        0x01, 0xAA, 0xBB, 0xCC,
    };

    auto parsed = Himax::Pen::TryParsePenUsbEventFrame(packet);
    Require(parsed.has_value(), "valid factory event frame should parse");
    Require(parsed->eventCode == 0x71, "event code should come from packet[5]");
    Require(parsed->payload.size() == 4, "payload should start at packet[8]");
    Require(parsed->payload[0] == 0x01, "payload[0] should preserve packet[8]");
}

void TestInvalidFactoryEventFramesAreRejected() {
    const std::array<uint8_t, 8> shortPacket{
        0x00, 0x00, 0x07, 0x00,
        0x01, 0x71, 0x00, 0x00,
    };
    Require(!Himax::Pen::TryParsePenUsbEventFrame(shortPacket).has_value(),
            "short packet should be rejected");

    std::array<uint8_t, 9> wrongSignature{
        0x00, 0x00, 0x02, 0x00,
        0x01, 0x71, 0x00, 0x00,
        0x01,
    };
    Require(!Himax::Pen::TryParsePenUsbEventFrame(wrongSignature).has_value(),
            "packet with wrong packet[2] signature should be rejected");

    std::array<uint8_t, 9> wrongCommandLow{
        0x00, 0x00, 0x07, 0x00,
        0x00, 0x71, 0x00, 0x00,
        0x01,
    };
    Require(!Himax::Pen::TryParsePenUsbEventFrame(wrongCommandLow).has_value(),
            "packet with wrong packet[4] command low byte should be rejected");
}

void TestFactoryAckTable() {
    using Himax::Pen::GetFactoryBtMcuAckCode;

    Require(GetFactoryBtMcuAckCode(0x2F) == 0x0B, "0x2F should ACK 0x0B");
    Require(GetFactoryBtMcuAckCode(0x70) == 0x00, "0x70 should ACK 0");
    Require(GetFactoryBtMcuAckCode(0x71) == 0x01, "0x71 should ACK 1");
    Require(GetFactoryBtMcuAckCode(0x72) == 0x02, "0x72 should ACK 2");
    Require(GetFactoryBtMcuAckCode(0x73) == 0x0D, "0x73 should ACK 0x0D");
    Require(GetFactoryBtMcuAckCode(0x74) == 0x03, "0x74 should ACK 3");
    Require(GetFactoryBtMcuAckCode(0x75) == 0x04, "0x75 should ACK 4");
    Require(GetFactoryBtMcuAckCode(0x76) == 0x05, "0x76 should ACK 5");
    Require(GetFactoryBtMcuAckCode(0x77) == 0x06, "0x77 should ACK 6");
    Require(GetFactoryBtMcuAckCode(0x78) == 0x07, "0x78 should ACK 7");
    Require(GetFactoryBtMcuAckCode(0x79) == 0x08, "0x79 should ACK 8");
    Require(GetFactoryBtMcuAckCode(0x7B) == 0x0A, "0x7B should ACK 0x0A");
    Require(GetFactoryBtMcuAckCode(0x7C) == 0x0C, "0x7C should ACK 0x0C");
    Require(GetFactoryBtMcuAckCode(0x7F) == 0x09, "0x7F should ACK 9");
    Require(GetFactoryBtMcuAckCode(0x6F) == -1, "0x6F should not be ACKed without factory evidence");
}

void TestCommandPacketBuilders() {
    using Himax::Pen::BuildPenUsbCommand;
    using Himax::Pen::BuildPenUsbEventAck;
    using Himax::Pen::PenUsbCommandId;

    Require(BuildPenUsbCommand(PenUsbCommandId::QueryPenStatus) ==
                std::vector<uint8_t>({0x07, 0x00, 0x02, 0x00, 0x01, 0x71, 0x11, 0x00}),
            "0x7101 command packet should match factory bytes");
    Require(BuildPenUsbCommand(PenUsbCommandId::QueryPenInfo) ==
                std::vector<uint8_t>({0x07, 0x00, 0x02, 0x00, 0x01, 0x77, 0x11, 0x00}),
            "0x7701 command packet should match factory bytes");
    Require(BuildPenUsbEventAck(0x0A) ==
                std::vector<uint8_t>({0x07, 0x01, 0x02, 0x00, 0x01, 0x80, 0x11, 0x20, 0x0A}),
            "0x8001 ACK packet should match factory bytes");
}

void TestType3Encoding() {
    std::array<uint8_t, 8> out{};
    Require(Himax::Pen::EncodePenUsbType3Token("3333", out) == 2, "4-digit token should encode to 2 bytes");
    Require(out[0] == 0x33 && out[1] == 0x33, "3333 should encode as 33 33");

    out = {};
    Require(Himax::Pen::EncodePenUsbType3Token("2e7", out) == 2, "3-digit token should encode to 2 bytes");
    Require(out[0] == 0xE7 && out[1] == 0x02, "2e7 should encode as e7 02");

    const auto payload = Himax::Pen::BuildScanModePayload(51, 68, 1);
    Require(payload[0] == 0x01 && payload[1] == 0x05,
            "scan freq1 decimal token should preserve current type-3 behavior");
    Require(payload[2] == 0x08 && payload[3] == 0x06,
            "scan freq2 decimal token should preserve current type-3 behavior");
    Require(payload[4] == 0x03, "non-zero scan mode should encode as mode 3");
}

void TestFactoryInitParamsPacket() {
    const auto packet = Himax::Pen::BuildFactoryInitProtocolParamsCommand();
    const std::vector<uint8_t> expected{
        0x07, 0x01, 0x02, 0x00, 0x01, 0x7D, 0x11, 0x20,
        0x33, 0x33, 0x33, 0x33, 0xE7, 0x02, 0x12, 0x04,
        0x58, 0x02, 0x8D, 0x20, 0x0F, 0x01, 0x02, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    Require(packet == expected, "factory init params packet should remain exact capture");
}

} // namespace

int main() {
    try {
        TestValidFactoryEventFrameParses();
        TestInvalidFactoryEventFramesAreRejected();
        TestFactoryAckTable();
        TestCommandPacketBuilders();
        TestType3Encoding();
        TestFactoryInitParamsPacket();
        std::cout << "[TEST] Pen USB protocol tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
