#include "btmcu/PenUsbPacketBuilder.h"
#include "btmcu/PenUsbTypes.h"
#include "TestRequire.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <vector>

namespace {

using DeviceTests::Require;

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

void TestMinimumFactoryEventFrameParses() {
    const std::array<uint8_t, 9> packet{
        0x00, 0x00, 0x07, 0x00,
        0x01, 0x6F, 0x00, 0x00,
        0xEE,
    };

    auto parsed = Himax::Pen::TryParsePenUsbEventFrame(packet);
    Require(parsed.has_value(), "minimum 9-byte factory event frame should parse");
    Require(parsed->eventCode == 0x6F, "minimum frame event code should be preserved");
    Require(parsed->payload.size() == 1, "minimum frame should expose one payload byte");
    Require(parsed->payload[0] == 0xEE, "minimum frame payload should start at packet[8]");
}

void TestHardwareVersionEventFrameParses() {
    const std::array<uint8_t, 14> packet{
        0x00, 0x00, 0x07, 0x00,
        0x01, 0x02, 0x11, 0x05,
        'H', 'W', '1', '.', '2', 0x00,
    };

    auto parsed = Himax::Pen::TryParsePenUsbEventFrame(packet);
    Require(parsed.has_value(), "hardware version frame should parse");
    Require(parsed->eventCode == 0x02, "hardware version event code should be 0x02");
    Require(parsed->payload.size() == 6, "hardware version payload should start at packet[8]");
    Require(parsed->payload[0] == 'H', "hardware version payload[0] should preserve packet[8]");
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

    std::array<uint8_t, 9> validBytesAtWrongOffsets{
        0x07, 0x01, 0x00, 0x00,
        0x00, 0x71, 0x07, 0x01,
        0x01,
    };
    Require(!Himax::Pen::TryParsePenUsbEventFrame(validBytesAtWrongOffsets).has_value(),
            "factory parser should require signature bytes at packet[2] and packet[4]");
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
    Require(GetFactoryBtMcuAckCode(0x00) == -1, "PenModule 0x00 should not be ACKed without factory evidence");
    Require(GetFactoryBtMcuAckCode(0x28) == -1, "Open event 0x28 should not be ACKed without factory evidence");
    Require(GetFactoryBtMcuAckCode(0x6F) == -1, "0x6F should not be ACKed without factory evidence");
}

void TestEventCodeNames() {
    using Himax::Pen::PenUsbEventCode;
    using Himax::Pen::PenUsbEventCodeFromRaw;
    using Himax::Pen::PenUsbEventNameFromRaw;
    using Himax::Pen::ToString;

    Require(PenUsbEventCodeFromRaw(0x71) == PenUsbEventCode::PenConnStatus,
            "0x71 should map to PenConnStatus");
    Require(PenUsbEventCodeFromRaw(0x7B) == PenUsbEventCode::PenRepParam,
            "0x7B should map to PenRepParam");
    Require(PenUsbEventCodeFromRaw(0x28) == PenUsbEventCode::Unknown28,
            "0x28 should keep the documented open-event identity");
    Require(PenUsbEventCodeFromRaw(0x55) == PenUsbEventCode::Unknown,
            "unknown event code should map to Unknown");

    Require(std::string(ToString(PenUsbEventCode::PenConnStatus)) == "PEN_CONN_STATUS",
            "0x71 should format as PEN_CONN_STATUS");
    Require(std::string(ToString(PenUsbEventCode::PenRepParam)) == "PEN_REP_PARAM",
            "0x7B should format as PEN_REP_PARAM");
    Require(std::string(ToString(PenUsbEventCode::PenRotateAngle)) == "PEN_ROATE_ANGLE",
            "0x74 should preserve the protocol document spelling");
    Require(std::string(PenUsbEventNameFromRaw(0x28)) == "UNKNOWN_0x28",
            "0x28 should format as a named open event, not a raw-only code");
    Require(std::string(PenUsbEventNameFromRaw(0x55)) == "UNKNOWN_EVENT",
            "unknown event code should format as UNKNOWN_EVENT");
}

void TestCommandPacketBuilders() {
    using Himax::Pen::BuildPenUsbCommand;
    using Himax::Pen::BuildPenUsbEventAck;
    using Himax::Pen::BuildPenUsbFixedSizeCommand;
    using Himax::Pen::BuildPenUsbPayloadCommand;
    using Himax::Pen::PenUsbCommandId;

    Require(BuildPenUsbCommand(PenUsbCommandId::QueryHardwareVersion) ==
                std::vector<uint8_t>({0x07, 0x00, 0x02, 0x00, 0x01, 0x02, 0x11, 0x00}),
            "0x0201 command packet should query pen hardware version");
    const auto hardwareVersionQuery = BuildPenUsbFixedSizeCommand(PenUsbCommandId::QueryHardwareVersion);
    Require(hardwareVersionQuery.size() == 0x40,
            "hardware version fixed-size query should be padded to 64 bytes");
    Require(std::vector<uint8_t>(hardwareVersionQuery.begin(), hardwareVersionQuery.begin() + 8) ==
                std::vector<uint8_t>({0x07, 0x00, 0x02, 0x00, 0x01, 0x02, 0x11, 0x00}),
            "hardware version fixed-size query should preserve the request header");
    Require(hardwareVersionQuery[8] == 0x00 && hardwareVersionQuery[0x3F] == 0x00,
            "hardware version fixed-size query should be zero padded after the header");
    Require(BuildPenUsbCommand(PenUsbCommandId::QueryPenStatus) ==
                std::vector<uint8_t>({0x07, 0x00, 0x02, 0x00, 0x01, 0x71, 0x11, 0x00}),
            "0x7101 command packet should match factory bytes");
    Require(BuildPenUsbCommand(PenUsbCommandId::QueryPenInfo) ==
                std::vector<uint8_t>({0x07, 0x00, 0x02, 0x00, 0x01, 0x77, 0x11, 0x00}),
            "0x7701 command packet should match factory bytes");
    Require(BuildPenUsbCommand(PenUsbCommandId::PairInfoSet) ==
                std::vector<uint8_t>({0x07, 0x00, 0x02, 0x00, 0x01, 0x7E, 0x11, 0x00}),
            "0x7E01 command packet should use little-endian command id");
    Require(BuildPenUsbEventAck(0x0A) ==
                std::vector<uint8_t>({0x07, 0x01, 0x02, 0x00, 0x01, 0x80, 0x11, 0x20, 0x0A}),
            "0x8001 ACK packet should match factory bytes");

    const std::array<uint8_t, 3> payload{0xAA, 0xBB, 0xCC};
    Require(BuildPenUsbPayloadCommand(PenUsbCommandId::InitParamSet, payload) ==
                std::vector<uint8_t>({0x07, 0x01, 0x02, 0x00, 0x01, 0x7D, 0x11, 0x20, 0xAA, 0xBB, 0xCC}),
            "payload command should preserve header and append payload bytes");
}

void TestUtf8PayloadDecoding() {
    const std::array<uint8_t, 8> version{'H', 'W', '1', '.', '2', 0x00, 'X', 'Y'};
    Require(Himax::Pen::DecodePenUsbUtf8Payload(version) == "HW1.2",
            "UTF-8 payload should stop at the first NUL byte");

    const std::array<uint8_t, 7> utf8{'H', 'W', '-', 0xE7, 0xAC, 0x94, 0x00};
    Require(Himax::Pen::DecodePenUsbUtf8Payload(utf8) == std::string("HW-\xE7\xAC\x94", 6),
            "UTF-8 payload should preserve multibyte characters");

    const std::array<uint8_t, 6> mixed{'A', 0x01, 'B', 0x7F, 'C', 0x00};
    Require(Himax::Pen::DecodePenUsbUtf8Payload(mixed).empty(),
            "payload with control characters should be rejected");

    const std::array<uint8_t, 4> binary{0x01, 0x02, 0x1F, 0x00};
    Require(Himax::Pen::DecodePenUsbUtf8Payload(binary).empty(),
            "binary payload should not be exposed as a version string");

    const std::array<uint8_t, 3> invalid{0xE7, 0xAC, 0x00};
    Require(Himax::Pen::DecodePenUsbUtf8Payload(invalid).empty(),
            "truncated UTF-8 payload should be rejected");

    const std::array<uint8_t, 3> empty{0x00, 'H', 'W'};
    Require(Himax::Pen::DecodePenUsbUtf8Payload(empty).empty(),
            "payload starting with NUL should decode as an empty string");
}

void TestType3Encoding() {
    std::array<uint8_t, 8> out{};
    Require(Himax::Pen::EncodePenUsbType3Token("", out) == 0, "empty token should not emit bytes");

    out = {};
    Require(Himax::Pen::EncodePenUsbType3Token("7", out) == 1, "1-digit token should encode to 1 byte");
    Require(out[0] == 0x07, "7 should encode as 07");

    out = {};
    Require(Himax::Pen::EncodePenUsbType3Token("12", out) == 2, "2-digit token should encode to 2 bytes");
    Require(out[0] == 0x02 && out[1] == 0x01, "12 should encode as 02 01");

    out = {};
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

    const auto offPayload = Himax::Pen::BuildScanModePayload(51, 68, 0);
    Require(offPayload[4] == 0x00, "zero scan mode should encode as mode 0");
}

void TestFactoryInitParamsPacket() {
    const auto packet = Himax::Pen::BuildFactoryInitProtocolParamsCommand();
    const std::vector<uint8_t> expected{
        0x07, 0x01, 0x02, 0x00, 0x01, 0x7D, 0x11, 0x20,
        0x33, 0x33, 0x33, 0x33, 0xE7, 0x02, 0x12, 0x04,
        0x58, 0x02, 0x1A, 0x41, 0x0F, 0x01, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    Require(packet == expected, "factory init params packet should remain exact capture");
}

} // namespace

int main() {
    try {
        TestValidFactoryEventFrameParses();
        TestMinimumFactoryEventFrameParses();
        TestHardwareVersionEventFrameParses();
        TestInvalidFactoryEventFramesAreRejected();
        TestFactoryAckTable();
        TestEventCodeNames();
        TestCommandPacketBuilders();
        TestUtf8PayloadDecoding();
        TestType3Encoding();
        TestFactoryInitParamsPacket();
        std::cout << "[TEST] Device Pen USB protocol packet tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
