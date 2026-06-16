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
    Require(U(IpcCommand::ReloadConfig) == 40, "ReloadConfig command value remains stable as a tombstone");
    Require(U(IpcCommand::SaveConfig) == 41, "SaveConfig command value remains stable as a tombstone");
    Require(U(IpcCommand::GetConfigSnapshot) == 42, "GetConfigSnapshot command value remains stable as a tombstone");
    Require(U(IpcCommand::ApplyConfigPatch) == 43, "ApplyConfigPatch command value remains stable as a tombstone");
    Require(U(IpcCommand::PersistConfig) == 44, "PersistConfig command value remains stable as a tombstone");
    Require(U(IpcCommand::ApplyConfigTlvChunk) == 45, "ApplyConfigTlvChunk command value remains stable as a tombstone");
    Require(U(IpcCommand::GetConfigCatalogV3) == 46, "GetConfigCatalogV3 command value is assigned");
    Require(U(IpcCommand::GetConfigSnapshotV3) == 47, "GetConfigSnapshotV3 command value is assigned");
    Require(U(IpcCommand::ApplyConfigPatchV3) == 48, "ApplyConfigPatchV3 command value is assigned");
    Require(U(IpcCommand::PersistConfigV3) == 49, "PersistConfigV3 command value is assigned");
    Require(IsLegacyConfigTombstoneCommand(IpcCommand::ReloadConfig), "ReloadConfig is classified as legacy tombstone");
    Require(IsLegacyConfigTombstoneCommand(IpcCommand::SaveConfig), "SaveConfig is classified as legacy tombstone");
    Require(IsLegacyConfigTombstoneCommand(IpcCommand::GetConfigSnapshot), "GetConfigSnapshot is classified as legacy tombstone");
    Require(IsLegacyConfigTombstoneCommand(IpcCommand::ApplyConfigPatch), "ApplyConfigPatch is classified as legacy tombstone");
    Require(IsLegacyConfigTombstoneCommand(IpcCommand::PersistConfig), "PersistConfig is classified as legacy tombstone");
    Require(IsLegacyConfigTombstoneCommand(IpcCommand::ApplyConfigTlvChunk), "ApplyConfigTlvChunk is classified as legacy tombstone");
    Require(!IsSupportedIpcCommand(IpcCommand::ReloadConfig), "ReloadConfig is unsupported by connected IPC");
    Require(!IsSupportedIpcCommand(IpcCommand::SaveConfig), "SaveConfig is unsupported by connected IPC");
    Require(!IsSupportedIpcCommand(IpcCommand::GetConfigSnapshot), "GetConfigSnapshot is unsupported by connected IPC");
    Require(!IsSupportedIpcCommand(IpcCommand::ApplyConfigPatch), "ApplyConfigPatch is unsupported by connected IPC");
    Require(!IsSupportedIpcCommand(IpcCommand::PersistConfig), "PersistConfig is unsupported by connected IPC");
    Require(!IsSupportedIpcCommand(IpcCommand::ApplyConfigTlvChunk), "ApplyConfigTlvChunk is unsupported by connected IPC");
    Require(IsSupportedIpcCommand(IpcCommand::GetConfigCatalogV3), "GetConfigCatalogV3 remains supported");
    Require(IsSupportedIpcCommand(IpcCommand::GetConfigSnapshotV3), "GetConfigSnapshotV3 remains supported");
    Require(IsSupportedIpcCommand(IpcCommand::ApplyConfigPatchV3), "ApplyConfigPatchV3 remains supported");
    Require(IsSupportedIpcCommand(IpcCommand::PersistConfigV3), "PersistConfigV3 remains supported");
    Require(U(IpcCommand::GetDebugSchema) == 61, "GetDebugSchema command value remains stable");
    Require(U(IpcCommand::SetMasterParserOnly) == 64, "SetMasterParserOnly command value remains stable");
    Require(U(IpcCommand::GetPenIdentityStatus) == 65, "GetPenIdentityStatus command value remains stable");
    Require(IsSupportedIpcCommand(IpcCommand::Ping), "Ping remains supported");
    Require(IsSupportedIpcCommand(IpcCommand::SetVhfEnabled), "SetVhfEnabled remains supported");
    Require(IsSupportedIpcCommand(IpcCommand::GetDebugSchema), "GetDebugSchema remains supported");
    Require(IsSupportedIpcCommand(IpcCommand::GetPenIdentityStatus), "GetPenIdentityStatus remains supported");

    Require(U(IpcStatusCode::Ok) == 0, "Ok status value remains stable");
    Require(U(IpcStatusCode::UnsupportedCommand) == 1, "UnsupportedCommand status value remains stable");
    Require(U(IpcStatusCode::InvalidRequest) == 2, "InvalidRequest status value remains stable");
    Require(U(IpcStatusCode::InvalidState) == 3, "InvalidState status value remains stable");
    Require(U(IpcStatusCode::NotFound) == 4, "NotFound status value remains stable");
    Require(U(IpcStatusCode::PermissionDenied) == 5, "PermissionDenied status value remains stable");
    Require(U(IpcStatusCode::InternalError) == 6, "InternalError status value remains stable");

    ConfigV3PageRequestWire pageRequest{};
    Require(sizeof(ConfigV3PageRequestWire) <= 256, "ConfigV3PageRequestWire fits in request param");
    Require(pageRequest.wireVersion == kIpcProtocolVersion, "ConfigV3PageRequestWire version defaults to protocol version");
    Require(pageRequest.payloadKind == U(ConfigV3PayloadKind::Catalog), "ConfigV3PageRequestWire payload kind defaults to Catalog");
    Require(pageRequest.flags == 0 && pageRequest.offset == 0 && pageRequest.maxBytes == 0, "ConfigV3PageRequestWire fields default to zero");

    ConfigV3PageResponseHeaderWire pageHeader{};
    Require(sizeof(ConfigV3PageResponseHeaderWire) == 28, "ConfigV3PageResponseHeaderWire layout remains fixed");
    Require(pageHeader.headerBytes == sizeof(ConfigV3PageResponseHeaderWire), "ConfigV3PageResponseHeaderWire headerBytes defaults to header size");
    Require(ConfigV3PageCapacityBytes() == kIpcResponseDataBytes - sizeof(ConfigV3PageResponseHeaderWire), "Config v3 page capacity matches response data tail");
    Require(IsValidConfigV3PageResponse(pageHeader), "Config v3 default page header is valid");
    Require(IsValidConfigV3PageResponse(pageHeader, sizeof(ConfigV3PageResponseHeaderWire)), "Config v3 explicit data length is valid");
    pageHeader.pageBytes = ConfigV3PageCapacityBytes() + 1;
    Require(!IsValidConfigV3PageResponse(pageHeader), "Config v3 page header rejects oversized page");

    pageHeader = ConfigV3PageResponseHeaderWire{};
    pageHeader.wireVersion = kIpcProtocolVersion + 1;
    Require(!IsValidConfigV3PageResponse(pageHeader), "Config v3 page header rejects invalid version");
    pageHeader = ConfigV3PageResponseHeaderWire{};
    pageHeader.payloadKind = 0xFFu;
    Require(!IsValidConfigV3PageResponse(pageHeader), "Config v3 page header rejects invalid kind");
    pageHeader = ConfigV3PageResponseHeaderWire{};
    pageHeader.flags = 0x01u;
    Require(!IsValidConfigV3PageResponse(pageHeader), "Config v3 page header rejects unknown flags");
    pageHeader = ConfigV3PageResponseHeaderWire{};
    pageHeader.offset = 1;
    pageHeader.totalBytes = 0;
    Require(!IsValidConfigV3PageResponse(pageHeader), "Config v3 page header rejects offset beyond total");
    pageHeader = ConfigV3PageResponseHeaderWire{};
    pageHeader.offset = 4;
    pageHeader.totalBytes = 5;
    pageHeader.pageBytes = 2;
    Require(!IsValidConfigV3PageResponse(pageHeader), "Config v3 page header rejects page beyond total");
    pageHeader = ConfigV3PageResponseHeaderWire{};
    pageHeader.pageBytes = 4;
    pageHeader.totalBytes = 4;
    Require(!IsValidConfigV3PageResponse(pageHeader, sizeof(ConfigV3PageResponseHeaderWire) + 3), "Config v3 page header rejects mismatched data length");

    ApplyConfigPatchV3RequestWire patchV3{};
    Require(sizeof(ApplyConfigPatchV3RequestWire) == 256, "ApplyConfigPatchV3RequestWire layout remains fixed");
    Require(sizeof(patchV3.bytes) == kConfigPatchV3PayloadBytes, "ApplyConfigPatchV3RequestWire payload capacity remains fixed");
    Require(kConfigPatchV3PayloadBytes == 240, "Config v3 patch payload capacity remains stable");
    Require(patchV3.wireVersion == kIpcProtocolVersion, "ApplyConfigPatchV3RequestWire version defaults to protocol version");
    Require(patchV3.headerBytes == 16, "ApplyConfigPatchV3RequestWire headerBytes defaults to fixed header");
    Require(patchV3.baseSchemaVersion == 0 && patchV3.baseSnapshotVersion == 0, "ApplyConfigPatchV3RequestWire baseline versions default zero");
    Require(patchV3.payloadBytes == 0 && patchV3.flags == 0, "ApplyConfigPatchV3RequestWire payload metadata defaults zero");
    Require(!IsValidApplyConfigPatchV3Request(patchV3), "ApplyConfigPatchV3RequestWire rejects empty payload");
    patchV3.payloadBytes = 1;
    Require(IsValidApplyConfigPatchV3Request(patchV3), "ApplyConfigPatchV3RequestWire accepts one-byte payload");
    patchV3.payloadBytes = kConfigPatchV3PayloadBytes + 1;
    Require(!IsValidApplyConfigPatchV3Request(patchV3), "ApplyConfigPatchV3RequestWire rejects oversized payload");
    patchV3 = ApplyConfigPatchV3RequestWire{};
    patchV3.payloadBytes = 1;
    patchV3.flags = 1;
    Require(!IsValidApplyConfigPatchV3Request(patchV3), "ApplyConfigPatchV3RequestWire rejects flags");

    ConfigV3ApplyResultWire applyV3{};
    Require(sizeof(ConfigV3ApplyResultWire) == 16, "ConfigV3ApplyResultWire layout remains fixed");
    Require(applyV3.wireVersion == kIpcProtocolVersion, "ConfigV3ApplyResultWire version defaults to protocol version");
    Require(applyV3.status == U(ConfigV3MutationStatus::Ok), "ConfigV3ApplyResultWire status defaults Ok");
    Require(applyV3.changedCount == 0 && applyV3.appliedCount == 0 && applyV3.restartRequiredCount == 0,
            "ConfigV3ApplyResultWire counters default zero");
    Require(applyV3.rejectedCount == 0 && applyV3.failedKeyId == 0 && applyV3.failedValueType == 0,
            "ConfigV3ApplyResultWire failure fields default zero");

    PersistConfigV3ResponseWire persistV3{};
    Require(sizeof(PersistConfigV3ResponseWire) == 10, "PersistConfigV3ResponseWire layout remains fixed");
    Require(persistV3.wireVersion == kIpcProtocolVersion, "PersistConfigV3ResponseWire version defaults to protocol version");
    Require(persistV3.status == U(ConfigV3MutationStatus::Ok), "PersistConfigV3ResponseWire status defaults Ok");
    Require(persistV3.persistedCount == 0 && persistV3.skippedCount == 0 && persistV3.failedCount == 0,
            "PersistConfigV3ResponseWire counters default zero");

    PenIdentityStatusWire penIdentity{};
    Require(sizeof(PenIdentityStatusWire) == 404, "PenIdentityStatusWire layout remains fixed");
    Require(penIdentity.wireVersion == kIpcProtocolVersion, "PenIdentityStatusWire version defaults to protocol version");
    Require(penIdentity.flags == 0, "PenIdentityStatusWire flags default empty");
    Require(penIdentity.stylusId == 0, "PenIdentityStatusWire stylus id defaults zero");
    Require(penIdentity.penModuleModelId == 0, "PenIdentityStatusWire model id defaults zero");
    Require(penIdentity.hardwareVersionUtf8Len == 0, "PenIdentityStatusWire hardware UTF-8 length defaults zero");
    Require(penIdentity.protocolHint == U(PenIdentityProtocolHint::Auto), "PenIdentityStatusWire protocol hint defaults Auto");
    Require(penIdentity.protocolFlags == 0, "PenIdentityStatusWire protocol flags default empty");
    Require(penIdentity.factoryStatusFlags == 0, "PenIdentityStatusWire factory status flags default zero");
    Require(penIdentity.serialNumberUtf8Len == 0, "PenIdentityStatusWire serial UTF-8 length defaults zero");
    Require(penIdentity.firmwareVersionUtf8Len == 0, "PenIdentityStatusWire firmware UTF-8 length defaults zero");
    Require(sizeof(penIdentity.hardwareVersionUtf8) == 128, "PenIdentityStatusWire hardware UTF-8 buffer capacity remains 128 bytes");
    Require(sizeof(penIdentity.serialNumberUtf8) == 128, "PenIdentityStatusWire serial UTF-8 buffer capacity remains 128 bytes");
    Require(sizeof(penIdentity.firmwareVersionUtf8) == 128, "PenIdentityStatusWire firmware UTF-8 buffer capacity remains 128 bytes");
    Require(penIdentity.hardwareVersionUtf8[0] == '\0', "PenIdentityStatusWire hardware UTF-8 buffer is zero-initialized");
    Require(penIdentity.serialNumberUtf8[0] == '\0', "PenIdentityStatusWire serial UTF-8 buffer is zero-initialized");
    Require(penIdentity.firmwareVersionUtf8[0] == '\0', "PenIdentityStatusWire firmware UTF-8 buffer is zero-initialized");

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
