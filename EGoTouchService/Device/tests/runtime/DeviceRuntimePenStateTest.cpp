#include "penevt/PenEventBridge.h"
#include "runtime/DeviceRuntime.h"
#include "TestRequire.h"

#include <array>
#include <cstdint>
#include <iostream>

struct DeviceRuntimePenStateTestAccess {
    struct PipelineState {
        Solvers::StylusPenSession session{};
        Asa::BtInputSnapshot btSample{};
        bool lastFrameWasTerminal = true;
    };

    static bool Process(DeviceRuntime& runtime, Solvers::HeatmapFrame& frame) {
        std::lock_guard<std::mutex> lock(runtime.m_pipelineMu);
        return runtime.m_stylusPipeline.Process(frame);
    }

    static PipelineState Snapshot(DeviceRuntime& runtime) {
        std::lock_guard<std::mutex> pipelineLock(runtime.m_pipelineMu);
        std::lock_guard<std::mutex> btLock(runtime.m_stylusPipeline.m_btMutex);
        return {
            runtime.m_stylusPipeline.m_penSession,
            runtime.m_stylusPipeline.m_btSample,
            runtime.m_stylusPipeline.m_lastFrameWasTerminal,
        };
    }
};

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

Solvers::HeatmapFrame MakeWritingHpp2Frame() {
    Solvers::HeatmapFrame frame{};
    auto& input = frame.stylus.input;
    input.auxStatusFlags = 0x1;
    input.mainFreq = 0x00b0;
    input.auxFreq = 0x00fc;
    input.framePressure = 512;
    input.hpp2LineValid = true;
    input.hpp2LineData.fill(10);
    input.hpp2LineData[12] = 2600;
    input.hpp2LineData[60 + 7] = 2400;
    return frame;
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
    Require(runtime.GetSnapshot().queue_depth == 0,
            "PenModule must not submit AFE work before the runtime command gate opens");

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
    Require(runtime.GetSnapshot().queue_depth == 0,
            "identity updates must not bypass the closed runtime command gate");

    auto connected = MakePayloadEvent(PenUsbEventCode::PenConnStatus, 1);
    connected.semantic.hasConnection = true;
    connected.semantic.connected = true;
    runtime.IngestPenEvent(connected);

    state = runtime.GetPenStateSnapshot();
    Require(state.hasConnection && state.connected,
            "PenConnStatus should be the event that establishes connection");
    Require(runtime.GetSnapshot().queue_depth == 0,
            "connected PenConnStatus must not bypass the closed runtime command gate");
}

void TestDuplicateDisconnectClearsIdentityObservably() {
    auto runtime = MakeRuntime();

    auto disconnected = MakePayloadEvent(PenUsbEventCode::PenConnStatus, 0);
    disconnected.semantic.hasConnection = true;
    disconnected.semantic.connected = false;
    runtime.IngestPenEvent(disconnected);

    auto serial = MakePayloadEvent(PenUsbEventCode::PenSerialNumber, 0);
    serial.semantic.hasSerialNumber = true;
    serial.semantic.serialNumber = "STALE-SERIAL";
    runtime.IngestPenEvent(serial);

    auto pairStatus = MakePayloadEvent(PenUsbEventCode::DevPairStatus, 7);
    pairStatus.semantic.hasPairStatus = true;
    pairStatus.semantic.pairStatus = 7;
    runtime.IngestPenEvent(pairStatus);

    auto state = runtime.GetPenStateSnapshot();
    Require(state.penRevision == 3 && state.hasSerialNumber,
            "disconnected identity update should remain observable until status refresh");

    runtime.IngestPenEvent(disconnected);
    state = runtime.GetPenStateSnapshot();
    Require(state.hasConnection && !state.connected,
            "duplicate disconnected status should preserve explicit connection state");
    Require(!state.hasSerialNumber && state.serialNumber.empty(),
            "duplicate disconnected status should clear stale identity");
    Require(state.hasPairStatus && state.pairStatus == 7,
            "disconnect identity cleanup should preserve independent pair status");
    Require(state.penRevision == 4,
            "identity cleanup on duplicate disconnect should advance revision");
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
    Require(state.pipelineRevision == 0,
            "pair-only updates should not advance stylus pipeline generation");
}

void TestPairStatusDuringWritingPreservesPipelineAndBtSample() {
    auto runtime = MakeRuntime();

    auto connected = MakePayloadEvent(PenUsbEventCode::PenConnStatus, 1);
    connected.semantic.hasConnection = true;
    connected.semantic.connected = true;
    runtime.IngestPenEvent(connected);

    auto module = MakePayloadEvent(PenUsbEventCode::PenModule, 0);
    module.semantic.hasPenModuleModelId = true;
    module.semantic.penModuleModelId = Himax::Pen::kPenModuleModelIdCd52;
    module.semantic.penModuleModel = Himax::Pen::PenModuleModel::Cd52;
    module.semantic.hasPenModuleProtocolHint = true;
    module.semantic.penModuleProtocolHint = Himax::Pen::PenModuleProtocolHint::Hpp2;
    runtime.IngestPenEvent(module);

    auto writingFrame = MakeWritingHpp2Frame();
    Require(DeviceRuntimePenStateTestAccess::Process(runtime, writingFrame),
            "connected HPP2 writing frame should process");
    Require(writingFrame.stylus.output.valid && writingFrame.stylus.output.tipDown,
            "test precondition should establish active writing state");

    const std::array<uint16_t, 4> pressure{100, 200, 300, 400};
    const std::array<uint16_t, 4> rawPressure{1000, 2000, 3000, 4000};
    runtime.IngestBtMcuPressurePacket(pressure, rawPressure, 0x12, 0x34);

    const auto penBefore = runtime.GetPenStateSnapshot();
    const auto pipelineBefore = DeviceRuntimePenStateTestAccess::Snapshot(runtime);
    Require(!pipelineBefore.lastFrameWasTerminal && pipelineBefore.btSample.hasSample,
            "test precondition should retain writing state and BT sample");

    auto pairStatus = MakePayloadEvent(PenUsbEventCode::DevPairStatus, 9);
    pairStatus.semantic.hasPairStatus = true;
    pairStatus.semantic.pairStatus = 9;
    runtime.IngestPenEvent(pairStatus);

    const auto penAfter = runtime.GetPenStateSnapshot();
    const auto pipelineAfter = DeviceRuntimePenStateTestAccess::Snapshot(runtime);
    Require(penAfter.penRevision == penBefore.penRevision + 1,
            "pair status should remain visible through diagnostic revision");
    Require(penAfter.pipelineRevision == penBefore.pipelineRevision,
            "pair status should not advance stylus pipeline generation");
    Require(pipelineAfter.session.revision == pipelineBefore.session.revision,
            "pair status should not publish a new stylus session");
    Require(!pipelineAfter.lastFrameWasTerminal,
            "pair status should not reset active writing stages");
    Require(pipelineAfter.btSample.hasSample &&
                pipelineAfter.btSample.seq == pipelineBefore.btSample.seq &&
                pipelineAfter.btSample.pressure == pipelineBefore.btSample.pressure &&
                pipelineAfter.btSample.rawPressure == pipelineBefore.btSample.rawPressure,
            "pair status should not clear the current BT pressure sample");
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
        TestDuplicateDisconnectClearsIdentityObservably();
        TestPairStatusIsIndependentFromConnection();
        TestPairStatusDuringWritingPreservesPipelineAndBtSample();
        TestPairStatusFrameProducesSemanticState();
        std::cout << "[TEST] DeviceRuntime pen state tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << '\n';
        return 1;
    }
}
