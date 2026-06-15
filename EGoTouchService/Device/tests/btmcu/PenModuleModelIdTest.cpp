#include "btmcu/PenModuleModelId.h"
#include "TestRequire.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <span>

namespace {

using DeviceTests::Require;
using Himax::Pen::PenModuleModel;
using Himax::Pen::PenModuleProtocolHint;

uint32_t ParseOrFail(std::span<const uint8_t> payload, uint8_t payloadLength) {
    auto modelId = Himax::Pen::TryParsePenModuleModelId(payload, payloadLength);
    Require(modelId.has_value(), "PenModule model id payload should parse");
    return *modelId;
}

void TestKnownCd54Mappings() {
    const std::array<uint8_t, 2> cd52Payload{0x1A, 0x01};
    const uint32_t cd52 = ParseOrFail(cd52Payload, 2);
    Require(cd52 == Himax::Pen::kPenModuleModelIdCd52,
            "CD52 payload should parse as 0x00011A");
    auto cd52Info = Himax::Pen::ResolvePenModuleModel(cd52);
    Require(cd52Info.model == PenModuleModel::Cd52,
            "0x00011A should resolve to CD52");
    Require(cd52Info.protocolHint == PenModuleProtocolHint::Hpp2,
            "CD52 should route to HPP2");

    const std::array<uint8_t, 2> cd54Payload{0x1B, 0x01};
    const uint32_t cd54 = ParseOrFail(cd54Payload, 2);
    Require(cd54 == Himax::Pen::kPenModuleModelIdCd54,
            "CD54 payload should parse as 0x00011B");
    auto cd54Info = Himax::Pen::ResolvePenModuleModel(cd54);
    Require(cd54Info.model == PenModuleModel::Cd54,
            "0x00011B should resolve to CD54");
    Require(cd54Info.protocolHint == PenModuleProtocolHint::Hpp3,
            "CD54 should route to HPP3");

    const std::array<uint8_t, 3> cd54RPayload{0x1B, 0x01, 0x01};
    const uint32_t cd54R = ParseOrFail(cd54RPayload, 3);
    Require(cd54R == Himax::Pen::kPenModuleModelIdCd54R,
            "CD54R payload should parse as 0x01011B");
    auto cd54RInfo = Himax::Pen::ResolvePenModuleModel(cd54R);
    Require(cd54RInfo.model == PenModuleModel::Cd54R,
            "0x01011B should resolve to CD54R");
    Require(cd54RInfo.protocolHint == PenModuleProtocolHint::Hpp3,
            "CD54R should route to HPP3");

    const std::array<uint8_t, 3> cd54SPayload{0x02, 0x30, 0x44};
    const uint32_t cd54S = ParseOrFail(cd54SPayload, 3);
    Require(cd54S == Himax::Pen::kPenModuleModelIdCd54S,
            "CD54S payload should parse as 0x443002");
    auto cd54SInfo = Himax::Pen::ResolvePenModuleModel(cd54S);
    Require(cd54SInfo.model == PenModuleModel::Cd54S,
            "0x443002 should resolve to CD54S");
    Require(cd54SInfo.protocolHint == PenModuleProtocolHint::Hpp3,
            "CD54S should route to HPP3");
}

void TestInvalidLengthsAreRejected() {
    const std::array<uint8_t, 4> payload{0x1B, 0x01, 0x00, 0x00};
    Require(!Himax::Pen::TryParsePenModuleModelId(payload, 0).has_value(),
            "empty PenModule payload should be rejected");
    Require(!Himax::Pen::TryParsePenModuleModelId(payload, 5).has_value(),
            "PenModule payloads longer than uint32 should be rejected");
    Require(!Himax::Pen::TryParsePenModuleModelId(std::span<const uint8_t>(payload.data(), 2), 3).has_value(),
            "PenModule payload length larger than available bytes should be rejected");
}

void TestUnknownIdsStayAuto() {
    const std::array<uint8_t, 2> payload{0x34, 0x12};
    const uint32_t modelId = ParseOrFail(payload, 2);
    Require(modelId == 0x1234,
            "unknown PenModule payload should still parse as little-endian integer");
    auto info = Himax::Pen::ResolvePenModuleModel(modelId);
    Require(info.model == PenModuleModel::Unknown,
            "unknown PenModule ID should not resolve to a known model");
    Require(info.protocolHint == PenModuleProtocolHint::Hpp3,
            "unknown PenModule ID should default to HPP3");
}

void TestPayloadLengthControlsTrailingBytes() {
    const std::array<uint8_t, 4> payload{0x1B, 0x01, 0x00, 0x00};
    const uint32_t modelId = ParseOrFail(payload, 2);
    Require(modelId == Himax::Pen::kPenModuleModelIdCd54,
            "PenModule parser should ignore trailing report padding beyond payload length");
}

} // namespace

int main() {
    try {
        TestKnownCd54Mappings();
        TestInvalidLengthsAreRejected();
        TestUnknownIdsStayAuto();
        TestPayloadLengthControlsTrailingBytes();
        std::cout << "[TEST] Device PenModule model id tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
