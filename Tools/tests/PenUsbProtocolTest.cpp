#include "btmcu/PenUsbTypes.h"

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

} // namespace

int main() {
    try {
        TestValidFactoryEventFrameParses();
        TestInvalidFactoryEventFramesAreRejected();
        TestFactoryAckTable();
        std::cout << "[TEST] Pen USB protocol tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
