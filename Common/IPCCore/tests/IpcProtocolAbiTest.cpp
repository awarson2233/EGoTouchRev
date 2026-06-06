#if __has_include("Ipc/IpcProtocol.h")
#include "Ipc/IpcProtocol.h"
#else
#include "IpcProtocol.h"
#endif

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <type_traits>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "[FAIL] " << message << "\n";
        std::exit(1);
    }
}

template <typename Enum>
constexpr auto U(Enum value) noexcept {
    return static_cast<std::underlying_type_t<Enum>>(value);
}

} // namespace

int main() {
    using namespace Ipc;

    Require(kIpcProtocolVersion == 2, "protocol version remains stable");
    Require(U(IpcCommand::Ping) == 0, "Ping command value remains stable");
    Require(U(IpcCommand::EnterDebugMode) == 1, "EnterDebugMode command value remains stable");
    Require(U(IpcCommand::ExitDebugMode) == 2, "ExitDebugMode command value remains stable");
    Require(U(IpcCommand::StartRuntime) == 10, "StartRuntime command value remains stable");
    Require(U(IpcCommand::StopRuntime) == 11, "StopRuntime command value remains stable");
    Require(U(IpcCommand::AfeCommand) == 20, "AfeCommand command value remains stable");
    Require(U(IpcCommand::GetConfigSnapshot) == 42, "GetConfigSnapshot command value remains stable");
    Require(U(IpcCommand::ApplyConfigPatch) == 43, "ApplyConfigPatch command value remains stable");
    Require(U(IpcCommand::PersistConfig) == 44, "PersistConfig command value remains stable");
    Require(U(IpcCommand::GetDebugSchema) == 61, "GetDebugSchema command value remains stable");
    Require(U(IpcCommand::SetMasterParserOnly) == 64, "SetMasterParserOnly command value remains stable");
    Require(U(IpcCommand::GetPenIdentityStatus) == 65, "GetPenIdentityStatus command value remains stable");

    Require(U(IpcStatusCode::Ok) == 0, "Ok status value remains stable");
    Require(U(IpcStatusCode::UnsupportedCommand) == 1, "UnsupportedCommand status value remains stable");
    Require(U(IpcStatusCode::InvalidRequest) == 2, "InvalidRequest status value remains stable");
    Require(U(IpcStatusCode::InvalidState) == 3, "InvalidState status value remains stable");
    Require(U(IpcStatusCode::NotFound) == 4, "NotFound status value remains stable");
    Require(U(IpcStatusCode::PermissionDenied) == 5, "PermissionDenied status value remains stable");
    Require(U(IpcStatusCode::InternalError) == 6, "InternalError status value remains stable");

    constexpr uint8_t allConfigFields =
        ToBits(ServiceConfigFieldWire::Mode) |
        ToBits(ServiceConfigFieldWire::AutoMode) |
        ToBits(ServiceConfigFieldWire::StylusVhfEnabled) |
        ToBits(ServiceConfigFieldWire::PenButtonMode) |
        ToBits(ServiceConfigFieldWire::PenButtonRoute);
    Require(allConfigFields == 0x1f, "config field mask remains stable");
    Require(HasField(allConfigFields, ServiceConfigFieldWire::Mode), "HasField detects mode bit");
    Require(!HasField(ToBits(ServiceConfigFieldWire::Mode), ServiceConfigFieldWire::AutoMode), "HasField rejects absent bit");

    ConfigSnapshotWire snapshot{};
    Require(snapshot.wireVersion == kIpcProtocolVersion, "ConfigSnapshotWire version defaults to protocol version");
    Require(snapshot.definedFields == allConfigFields, "ConfigSnapshotWire exposes all expected fields");
    Require(snapshot.desiredMode == U(ServiceModeWire::Full), "ConfigSnapshotWire desired mode defaults to Full");
    Require(snapshot.activeMode == U(ServiceModeWire::Full), "ConfigSnapshotWire active mode defaults to Full");
    Require(snapshot.autoMode == 1, "ConfigSnapshotWire autoMode defaults enabled");
    Require(snapshot.stylusVhfEnabled == 1, "ConfigSnapshotWire stylus VHF defaults enabled");
    Require(snapshot.penButtonMode == U(PenButtonModeWire::OemCustom), "ConfigSnapshotWire pen button mode default is OEM custom");
    Require(snapshot.penButtonRoute == U(PenButtonRouteWire::VhfOnly), "ConfigSnapshotWire pen button route default is VHF only");

    ApplyConfigPatchRequestWire patch{};
    Require(patch.wireVersion == kIpcProtocolVersion, "ApplyConfigPatchRequestWire version defaults to protocol version");
    Require(patch.fieldMask == 0, "ApplyConfigPatchRequestWire field mask defaults empty");
    Require(patch.desiredMode == U(ServiceModeWire::Full), "ApplyConfigPatchRequestWire desired mode defaults to Full");

    PenIdentityStatusWire penIdentity{};
    Require(sizeof(PenIdentityStatusWire) == 140, "PenIdentityStatusWire layout remains fixed");
    Require(penIdentity.wireVersion == kIpcProtocolVersion, "PenIdentityStatusWire version defaults to protocol version");
    Require(penIdentity.flags == 0, "PenIdentityStatusWire flags default empty");
    Require(penIdentity.stylusId == 0, "PenIdentityStatusWire stylus id defaults zero");
    Require(penIdentity.penModuleModelId == 0, "PenIdentityStatusWire model id defaults zero");
    Require(penIdentity.hardwareVersionUtf8Len == 0, "PenIdentityStatusWire UTF-8 length defaults zero");
    Require(sizeof(penIdentity.hardwareVersionUtf8) == 128, "PenIdentityStatusWire UTF-8 buffer capacity remains 128 bytes");
    Require(penIdentity.hardwareVersionUtf8[0] == '\0', "PenIdentityStatusWire UTF-8 buffer is zero-initialized");

    DebugFieldSchemaWire schema{};
    Require(schema.fieldId == 0, "DebugFieldSchemaWire field id is zero-initialized");
    Require(schema.valueType == U(DebugValueType::UInt32), "DebugFieldSchemaWire value type defaults to UInt32");
    Require(schema.sourceKind == U(DebugSourceKind::DerivedField), "DebugFieldSchemaWire source kind defaults to DerivedField");
    Require(schema.sourceIndex == -1, "DebugFieldSchemaWire source index defaults to -1");
    Require(schema.dvrIndex == -1, "DebugFieldSchemaWire DVR index defaults to -1");
    Require(schema.key[0] == '\0' && schema.displayName[0] == '\0' && schema.unit[0] == '\0', "DebugFieldSchemaWire text fields are zero-initialized");

    IpcRequest request{};
    Require(request.paramLen == 0, "IpcRequest paramLen defaults to zero");
    Require(sizeof(request.param) == 256, "IpcRequest param capacity remains 256 bytes");
    for (uint8_t value : request.param) {
        Require(value == 0, "IpcRequest param buffer is zero-initialized");
    }

    IpcResponse response{};
    Require(response.status == IpcStatusCode::InternalError, "IpcResponse status defaults to InternalError");
    Require(!response.success, "IpcResponse success defaults false");
    Require(response.dataLen == 0, "IpcResponse dataLen defaults to zero");
    Require(sizeof(response.data) == 4096, "IpcResponse data capacity remains 4096 bytes");
    for (uint8_t value : response.data) {
        Require(value == 0, "IpcResponse data buffer is zero-initialized");
    }

    MarkSuccess(response);
    Require(response.success, "MarkSuccess sets success");
    Require(response.status == IpcStatusCode::Ok, "MarkSuccess sets Ok status");

    MarkFailure(response, IpcStatusCode::PermissionDenied);
    Require(!response.success, "MarkFailure clears success");
    Require(response.status == IpcStatusCode::PermissionDenied, "MarkFailure sets supplied status");

    std::cout << "[PASS] IpcProtocolAbiTest\n";
    return 0;
}
