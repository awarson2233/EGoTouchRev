#include "penevt/PenEventBridge.h"
#include "runtime/DeviceRuntime.h"
#include "TestRequire.h"

#include <array>
#include <cstdint>
#include <iostream>

namespace {

using DeviceTests::Require;
using Himax::Pen::PenEvent;
using Himax::Pen::PenUsbEventCode;

DeviceRuntime MakeRuntime() {
    return DeviceRuntime(
        L"\\\\?\\EGoTouchMissingMaster",
        L"\\\\?\\EGoTouchMissingSlave",
        L"\\\\?\\EGoTouchMissingInterrupt");
}

PenEvent MakePayloadEvent(PenUsbEventCode code, uint8_t payload) {
    PenEvent event{};
    event.code = code;
    const std::array<uint8_t, 1> bytes{payload};
    (void)event.payload.assign(bytes);
    return event;
}

void TestIdentityEventsDoNotInferConnection() {
    auto runtime = MakeRuntime();

    auto module = MakePayloadEvent(PenUsbEventCode::PenModule, 0);
    module.semantic.hasPenModuleModelId = true;
    module.semantic.penModuleModelId = Himax::Pen::kPenModuleModelIdCd54S;
    module.semantic.penModuleModel = Himax::Pen::PenModuleModel::Cd54S;
    module.semantic.hasPenModuleProtocolHint = true;
    module.semantic.penModuleProtocolHint = Himax::Pen::PenModuleProtocolHint::Hpp3;
    runtime.IngestPenEvent(module);

    auto state = runtime.GetPenStateSnapshot();
    Require(!state.hasConnection && !state.connected,
            "PenModule must not infer a connection state");
    Require(state.hasPenModuleModelId &&
                state.penModuleModelId == Himax::Pen::kPenModuleModelIdCd54S,
            "PenModule should update model identity");
    Require(state.hasStylusId && state.stylusId == 2,
            "PenModule should retain model-derived stylus ID behavior");
    Require(state.protocolHintFromPenModule &&
                state.protocolHint == Solvers::StylusProtocolHint::Hpp3,
            "PenModule should update the protocol hint");
    Require(runtime.GetSnapshot().queue_depth == 1,
            "PenModule should enqueue SetStylusId only, not InitStylus");

    auto serial = MakePayloadEvent(PenUsbEventCode::PenSerialNumber, 0);
    serial.semantic.hasSerialNumber = true;
    serial.semantic.serialNumber = "SERIAL-42";
    runtime.IngestPenEvent(serial);

    auto hardware = MakePayloadEvent(PenUsbEventCode::PenHardwareVersion, 0);
    hardware.semantic.hasHardwareVersion = true;
    hardware.semantic.hardwareVersion = "HW-1.2";
    runtime.IngestPenEvent(hardware);

    auto firmware = MakePayloadEvent(PenUsbEventCode::UsbdSwVersion, 0);
    firmware.semantic.hasFirmwareVersion = true;
    firmware.semantic.firmwareVersion = "CD54S 1.0.0.122";
    runtime.IngestPenEvent(firmware);

    auto penType = MakePayloadEvent(PenUsbEventCode::PenTypeInfo, 3);
    penType.semantic.hasStylusId = true;
    penType.semantic.stylusId = 3;
    runtime.IngestPenEvent(penType);

    state = runtime.GetPenStateSnapshot();
    Require(!state.hasConnection && !state.connected,
            "version and PenTypeInfo events must not infer connection");
    Require(state.hasSerialNumber && state.serialNumber == "SERIAL-42",
            "serial identity should update without connection inference");
    Require(state.hasHardwareVersion && state.hardwareVersion == "HW-1.2",
            "hardware version should update without connection inference");
    Require(state.hasFirmwareVersion && state.firmwareVersion == "CD54S 1.0.0.122",
            "firmware version should update without connection inference");
    Require(state.hasStylusId && state.stylusId == 3,
            "PenTypeInfo should update stylus ID");
    Require(state.penRevision == 5,
            "each distinct identity update should advance pen revision once");
    Require(runtime.GetSnapshot().queue_depth == 2,
            "identity updates should enqueue only the two SetStylusId commands");

    auto connected = MakePayloadEvent(PenUsbEventCode::PenConnStatus, 1);
    connected.semantic.hasConnection = true;
    connected.semantic.connected = true;
    runtime.IngestPenEvent(connected);

    state = runtime.GetPenStateSnapshot();
    Require(state.hasConnection && state.connected,
            "PenConnStatus should be the event that establishes connection");
    Require(runtime.GetSnapshot().queue_depth == 3,
            "connected PenConnStatus should enqueue InitStylus");
}

void TestPairStatusIsIndependentFromConnection() {
    auto runtime = MakeRuntime();

    auto pairStatus = MakePayloadEvent(PenUsbEventCode::DevPairStatus, 2);
    pairStatus.semantic.hasPairStatus = true;
    pairStatus.semantic.pairStatus = 2;
    runtime.IngestPenEvent(pairStatus);

    auto state = runtime.GetPenStateSnapshot();
    Require(state.hasPairStatus && state.pairStatus == 2,
            "DevPairStatus should update independent pair state");
    Require(!state.hasConnection && !state.connected,
            "DevPairStatus must not fabricate connection");
    Require(state.penRevision == 1,
            "first pair status should advance pen revision");
    Require(runtime.GetSnapshot().queue_depth == 0,
            "DevPairStatus must not enqueue an AFE connection command");

    runtime.IngestPenEvent(pairStatus);
    Require(runtime.GetPenStateSnapshot().penRevision == 1,
            "duplicate pair status should not advance revision");

    pairStatus.payload.bytes[0] = 3;
    pairStatus.semantic.pairStatus = 3;
    runtime.IngestPenEvent(pairStatus);
    state = runtime.GetPenStateSnapshot();
    Require(state.hasPairStatus && state.pairStatus == 3 && state.penRevision == 2,
            "changed pair status should update state and revision");
}

class TestPenEventBridge final : public Himax::Pen::PenEventBridge {
public:
    using PenEventBridge::OnPacketReceived;
};

void TestPairStatusFrameProducesSemanticState() {
    TestPenEventBridge bridge;
    PenEvent received{};
    bool called = false;
    bridge.SetEventCallback([&](const PenEvent& event) {
        received = event;
        called = true;
    });

    const std::array<uint8_t, 9> packet{
        0x00, 0x00, 0x07, 0x00,
        0x01, 0x12, 0x00, 0x01,
        0xA5,
    };
    bridge.OnPacketReceived(packet);

    Require(called, "DevPairStatus frame should be dispatched");
    Require(received.code == PenUsbEventCode::DevPairStatus,
            "0x12 frame should retain DevPairStatus code");
    Require(received.semantic.hasPairStatus && received.semantic.pairStatus == 0xA5,
            "0x12 payload should populate pair status semantics");
    Require(!received.semantic.hasConnection,
            "0x12 semantic parsing must not fabricate connection");
}

} // namespace

int main() {
    try {
        TestIdentityEventsDoNotInferConnection();
        TestPairStatusIsIndependentFromConnection();
        TestPairStatusFrameProducesSemanticState();
        std::cout << "[TEST] DeviceRuntime pen state tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << '\n';
        return 1;
    }
}
