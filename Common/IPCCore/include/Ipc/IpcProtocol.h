#pragma once
// IpcProtocol: Command protocol for Named Pipe between App and Service.

#include <cstdint>

namespace Ipc {

constexpr uint16_t kIpcProtocolVersion = 2;
constexpr uint16_t kIpcResponseDataBytes = 4096;

constexpr const wchar_t* kPipeName = L"\\\\.\\pipe\\EGoTouchControl";
// IPC-related global events
constexpr const wchar_t* kLogReadyEventName = L"Global\\EGoTouchLogReady";
constexpr const wchar_t* kPenReadyEventName = L"Global\\EGoTouchPenStatusReady";

// NOTE:
// - Config commands 40-45 are legacy ABI tombstones. Service connected IPC must
//   return UnsupportedCommand for them; use the v3 commands 46-49 instead.
// - Config v3 is the only connected config-control path.
enum class IpcCommand : uint8_t {
    Ping = 0,
    // Debug mode: App tells Service to open shared memory and start pushing
    EnterDebugMode = 1,   // Request carries shared memory name (wchar_t[])
    ExitDebugMode  = 2,   // Service stops pushing, closes shared memory
    // Hardware control
    StartRuntime   = 10,  // Idempotent: success=true when runtime starts OR is already running
    StopRuntime    = 11,  // Idempotent: success=true when runtime stops OR is already stopped
    // AFE
    AfeCommand     = 20,  // param[0] = AFE_Command, param[1] = uint8_t
    // VHF
    SetVhfEnabled    = 30,
    SetVhfTranspose  = 31,
    // Config legacy ABI tombstones: retained values, unsupported by connected Service IPC.
    ReloadConfig   = 40,
    SaveConfig     = 41,
    GetConfigSnapshot = 42,
    ApplyConfigPatch  = 43,
    PersistConfig     = 44,
    ApplyConfigTlvChunk = 45,
    // Config v3 canonical control path.
    GetConfigCatalogV3 = 46,
    GetConfigSnapshotV3 = 47,
    ApplyConfigPatchV3 = 48,
    PersistConfigV3 = 49,
    // Logs
    GetLogs        = 50,  // App requests recent log lines from Service
    // PenBridge (BT MCU)
    GetPenBridgeStatus = 60,  // App queries live pressure stats + running state
    // Dynamic debug metadata + values
    GetDebugSchema   = 61,
    GetDebugSnapshot = 62,
    SetPenPressureMode = 63,  // param[0]: 0=4096 raw12, 1=16382 raw14 divided by 4
    SetMasterParserOnly = 64,  // param[0]: 0=normal, 1=service-side master parser only
    GetPenIdentityStatus = 65, // App queries current stylus identity + UTF-8 HW version
    TriggerQueryHardwareVersion = 66,
    TriggerQueryPenStatus = 67,
    TriggerQueryPenInfo = 68,
    TriggerSendScanMode = 69,
    TriggerSendPairInfoSet = 70,
    GetRuntimeStatus = 71,
    TriggerSendFactoryInitParams = 72,
};

constexpr bool IsLegacyConfigTombstoneCommand(IpcCommand command) noexcept {
    switch (command) {
    case IpcCommand::ReloadConfig:
    case IpcCommand::SaveConfig:
    case IpcCommand::GetConfigSnapshot:
    case IpcCommand::ApplyConfigPatch:
    case IpcCommand::PersistConfig:
    case IpcCommand::ApplyConfigTlvChunk:
        return true;
    default:
        return false;
    }
}

constexpr bool IsSupportedIpcCommand(IpcCommand command) noexcept {
    switch (command) {
    case IpcCommand::Ping:
    case IpcCommand::EnterDebugMode:
    case IpcCommand::ExitDebugMode:
    case IpcCommand::StartRuntime:
    case IpcCommand::StopRuntime:
    case IpcCommand::AfeCommand:
    case IpcCommand::SetVhfEnabled:
    case IpcCommand::SetVhfTranspose:
    case IpcCommand::GetConfigCatalogV3:
    case IpcCommand::GetConfigSnapshotV3:
    case IpcCommand::ApplyConfigPatchV3:
    case IpcCommand::PersistConfigV3:
    case IpcCommand::GetLogs:
    case IpcCommand::GetPenBridgeStatus:
    case IpcCommand::GetPenIdentityStatus:
    case IpcCommand::TriggerQueryHardwareVersion:
    case IpcCommand::TriggerQueryPenStatus:
    case IpcCommand::TriggerQueryPenInfo:
    case IpcCommand::TriggerSendScanMode:
    case IpcCommand::TriggerSendPairInfoSet:
    case IpcCommand::GetRuntimeStatus:
    case IpcCommand::TriggerSendFactoryInitParams:
    case IpcCommand::GetDebugSchema:
    case IpcCommand::GetDebugSnapshot:
    case IpcCommand::SetPenPressureMode:
    case IpcCommand::SetMasterParserOnly:
        return true;

    default:
        return false;
    }
}

enum class IpcStatusCode : uint8_t {
    Ok = 0,
    UnsupportedCommand = 1,
    InvalidRequest = 2,
    InvalidState = 3,
    NotFound = 4,
    PermissionDenied = 5,
    InternalError = 6,
};

enum class ConfigV3PayloadKind : uint8_t {
    Catalog = 1,
    Snapshot = 2,
};

struct ConfigV3PageRequestWire {
    uint16_t wireVersion = kIpcProtocolVersion;
    uint8_t payloadKind = static_cast<uint8_t>(ConfigV3PayloadKind::Catalog);
    uint8_t flags = 0;
    uint32_t schemaVersion = 0;
    uint32_t snapshotVersion = 0;
    uint32_t offset = 0;
    uint32_t maxBytes = 0;
    uint32_t reserved = 0;
};

struct ConfigV3PageResponseHeaderWire {
    uint16_t wireVersion = kIpcProtocolVersion;
    uint8_t payloadKind = static_cast<uint8_t>(ConfigV3PayloadKind::Catalog);
    uint8_t flags = 0;
    uint16_t headerBytes = sizeof(ConfigV3PageResponseHeaderWire);
    uint16_t pageBytes = 0;
    uint32_t totalBytes = 0;
    uint32_t schemaVersion = 0;
    uint32_t snapshotVersion = 0;
    uint32_t offset = 0;
    uint32_t checksum = 0;
};

constexpr uint16_t ConfigV3PageCapacityBytes() noexcept {
    return static_cast<uint16_t>(kIpcResponseDataBytes - sizeof(ConfigV3PageResponseHeaderWire));
}

constexpr bool IsKnownConfigV3PayloadKind(uint8_t payloadKind) noexcept {
    return payloadKind == static_cast<uint8_t>(ConfigV3PayloadKind::Catalog) ||
           payloadKind == static_cast<uint8_t>(ConfigV3PayloadKind::Snapshot);
}

constexpr bool IsValidConfigV3PageResponse(const ConfigV3PageResponseHeaderWire& header, uint32_t dataLen) noexcept {
    return header.wireVersion == kIpcProtocolVersion &&
           IsKnownConfigV3PayloadKind(header.payloadKind) &&
           header.flags == 0 &&
           header.headerBytes == sizeof(ConfigV3PageResponseHeaderWire) &&
           header.headerBytes < kIpcResponseDataBytes &&
           header.pageBytes <= ConfigV3PageCapacityBytes() &&
           header.offset <= header.totalBytes &&
           header.pageBytes <= header.totalBytes - header.offset &&
           dataLen == static_cast<uint32_t>(header.headerBytes) + header.pageBytes;
}

constexpr bool IsValidConfigV3PageResponse(const ConfigV3PageResponseHeaderWire& header) noexcept {
    return IsValidConfigV3PageResponse(header, static_cast<uint32_t>(header.headerBytes) + header.pageBytes);
}

constexpr uint16_t kConfigPatchV3PayloadBytes = 240;

enum class ConfigV3MutationStatus : uint8_t {
    Ok = 0,
    NoChanges = 1,
    VersionMismatch = 2,
    Rejected = 3,
    PersistFailed = 4,
};

struct ApplyConfigPatchV3RequestWire {
    uint16_t wireVersion = kIpcProtocolVersion;
    uint16_t headerBytes = 16;
    uint32_t baseSchemaVersion = 0;
    uint32_t baseSnapshotVersion = 0;
    uint16_t payloadBytes = 0;
    uint16_t flags = 0;
    uint8_t bytes[kConfigPatchV3PayloadBytes]{};
};

struct ConfigV3ApplyResultWire {
    uint16_t wireVersion = kIpcProtocolVersion;
    uint8_t status = static_cast<uint8_t>(ConfigV3MutationStatus::Ok);
    uint8_t failedValueType = 0;
    uint16_t changedCount = 0;
    uint16_t appliedCount = 0;
    uint16_t restartRequiredCount = 0;
    uint16_t rejectedCount = 0;
    uint16_t failedKeyId = 0;
    uint16_t reserved = 0;
};

struct PersistConfigV3ResponseWire {
    uint16_t wireVersion = kIpcProtocolVersion;
    uint8_t status = static_cast<uint8_t>(ConfigV3MutationStatus::Ok);
    uint8_t reserved = 0;
    uint16_t persistedCount = 0;
    uint16_t skippedCount = 0;
    uint16_t failedCount = 0;
};

constexpr bool IsValidApplyConfigPatchV3Request(const ApplyConfigPatchV3RequestWire& request) noexcept {
    return request.wireVersion == kIpcProtocolVersion &&
           request.headerBytes == 16 &&
           request.flags == 0 &&
           request.payloadBytes > 0 &&
           request.payloadBytes <= kConfigPatchV3PayloadBytes;
}

enum class DebugValueType : uint8_t {
    UInt32 = 0,
    Int32  = 1,
    Float32 = 2,
    Bool = 3,
};

enum class DebugSourceKind : uint8_t {
    MasterSuffixWord = 0,
    SlaveSuffixWord = 1,
    StylusField = 2,
    PenBridgeField = 3,
    DerivedField = 4,
};

enum class DebugDvrTarget : uint8_t {
    None = 0,
    MasterStatus = 1,
    SlaveSuffix = 2,
    DynamicDebug = 3,
};

enum class DebugDvrPositionMode : uint8_t {
    Append = 0,
    AfterAnchor = 1,
    Index = 2,
};

enum class DebugStylusSourceIndex : int16_t {
    Pressure = 0,
    SignalX = 1,
    SignalY = 2,
    MaxRawPeak = 3,
    Status = 4,
    PipelineStage = 5,
    PointX = 6,
    PointY = 7,
    RawPressure = 8,
    MappedPressure = 9,
    NoPressInkActive = 10,
    TouchSuppressActive = 11,
    BtSeq = 12,
    PredictedAgeFrames = 13,
    PressureIsReal = 14,
};

enum class DebugPenSourceIndex : int16_t {
    EvtRunning = 0,
    PressRunning = 1,
    ReportType = 2,
    Freq1 = 3,
    Freq2 = 4,
    Press0 = 5,
    Press1 = 6,
    Press2 = 7,
    Press3 = 8,
};

struct DebugSchemaRequest {
    uint16_t offset = 0;
    uint16_t limit = 0;
};

struct DebugSchemaResponseHeader {
    // Deterministic schema id derived from descriptor content.
    // It must change whenever field descriptors change, and match DebugSnapshotHeader::schemaVersion.
    uint16_t schemaVersion = 0;
    uint16_t totalFields = 0;
    uint16_t returnedFields = 0;
    uint16_t recordSize = 0;
    // Full 32-bit content hash of field descriptors (used for stronger schema identity).
    uint32_t schemaHash = 0;
};

struct DebugFieldSchemaWire {
    uint16_t fieldId = 0;
    uint8_t  valueType = static_cast<uint8_t>(DebugValueType::UInt32);
    uint8_t  sourceKind = static_cast<uint8_t>(DebugSourceKind::DerivedField);
    int16_t  sourceIndex = -1;
    uint8_t  uiOrder = 0;
    uint8_t  dvrTarget = static_cast<uint8_t>(DebugDvrTarget::None);
    uint8_t  dvrPositionMode = static_cast<uint8_t>(DebugDvrPositionMode::Append);
    uint8_t  _reserved0 = 0;
    int16_t  dvrIndex = -1;
    int16_t  _reserved1 = 0;

    char key[32]{};
    char displayName[48]{};
    char unit[16]{};
    char uiGroup[24]{};
    char dvrColumnName[32]{};
    char dvrAnchor[32]{};
};

constexpr uint16_t kDebugSnapshotLegacyHeaderSize = sizeof(uint16_t) * 4;
constexpr uint32_t kDebugSnapshotMetadataMagic = 0x44534D54u; // 'DSMT'
constexpr uint16_t kDebugSnapshotMetadataVersion = 1;
constexpr uint16_t kDebugSnapshotMetadataWireSize = 24;
constexpr uint32_t kDebugSnapshotHasFrameTimestamp = 1u << 0;

struct DebugSnapshotHeader {
    // Legacy 8-byte header. Values must immediately follow this header for
    // compatibility with existing GetDebugSnapshot clients.
    // Must be identical to DebugSchemaResponseHeader::schemaVersion for the same descriptor set.
    uint16_t schemaVersion = 0;
    uint16_t fieldCount = 0;
    uint16_t recordSize = 0;
    uint16_t _reserved0 = 0;
};

struct DebugSnapshotValueWire {
    uint16_t fieldId = 0;
    uint8_t  valueType = static_cast<uint8_t>(DebugValueType::UInt32);
    uint8_t  flags = 0; // bit0: value valid
    uint32_t _reserved0 = 0;
    uint64_t rawValue = 0;
};

constexpr uint16_t kDebugSnapshotMaxValues = static_cast<uint16_t>(
    (kIpcResponseDataBytes - kDebugSnapshotLegacyHeaderSize) / sizeof(DebugSnapshotValueWire));

struct DebugSnapshotMetadataWire {
    uint32_t magic = kDebugSnapshotMetadataMagic;
    uint16_t wireSize = kDebugSnapshotMetadataWireSize;
    uint16_t version = kDebugSnapshotMetadataVersion;
    uint32_t frameIdentityFlags = 0;
    uint32_t _reserved0 = 0;
    uint64_t frameTimestamp = 0;
};

constexpr uint8_t kPenIdentityHasStylusId = 1u << 0;
constexpr uint8_t kPenIdentityHasPenModuleModelId = 1u << 1;
constexpr uint8_t kPenIdentityHasHardwareVersion = 1u << 2;
constexpr uint8_t kPenIdentityConnected = 1u << 3;
constexpr uint8_t kPenIdentityHasSerialNumber = 1u << 4;
constexpr uint8_t kPenIdentityHasFirmwareVersion = 1u << 5;
constexpr uint8_t kPenIdentityHasProtocolHint = 1u << 6;
constexpr uint8_t kPenIdentityHasConnectionState = 1u << 7;

constexpr uint8_t kPenIdentityProtocolFromPenModule = 1u << 0;
constexpr uint8_t kPenIdentityHasPairStatus = 1u << 1;

enum class PenIdentityProtocolHint : uint8_t {
    Auto = 0,
    Hpp2 = 1,
    Hpp3 = 2,
};

struct PenIdentityStatusWire {
    uint16_t wireVersion = kIpcProtocolVersion;
    uint8_t flags = 0;
    uint8_t stylusId = 0;
    uint32_t penModuleModelId = 0;
    uint16_t hardwareVersionUtf8Len = 0;
    uint8_t protocolHint = static_cast<uint8_t>(PenIdentityProtocolHint::Auto);
    uint8_t protocolFlags = 0;
    char hardwareVersionUtf8[128]{};
    uint16_t serialNumberUtf8Len = 0;
    uint16_t firmwareVersionUtf8Len = 0;
    uint16_t factoryStatusFlags = 0;
    uint8_t pairStatus = 0;
    uint8_t _reserved0 = 0;
    char serialNumberUtf8[128]{};
    char firmwareVersionUtf8[128]{};
};

inline void SetPenIdentityPairStatus(PenIdentityStatusWire& wire,
                                     uint8_t pairStatus) noexcept {
    wire.protocolFlags |= kPenIdentityHasPairStatus;
    wire.pairStatus = pairStatus;
}

inline bool TryGetPenIdentityPairStatus(const PenIdentityStatusWire& wire,
                                        uint8_t& pairStatus) noexcept {
    if ((wire.protocolFlags & kPenIdentityHasPairStatus) == 0) {
        pairStatus = 0;
        return false;
    }
    pairStatus = wire.pairStatus;
    return true;
}

constexpr uint8_t kRuntimeStatusStreaming = 1u << 0;
constexpr uint8_t kRuntimeStatusVhfEnabled = 1u << 1;
constexpr uint8_t kRuntimeStatusVhfDeviceOpen = 1u << 2;
constexpr uint8_t kRuntimeStatusVhfTranspose = 1u << 3;

struct RuntimeStatusWire {
    uint16_t wireVersion = kIpcProtocolVersion;
    int8_t workerState = 127;
    uint8_t flags = 0;
    uint8_t recoverCount = 0;
    uint8_t _reserved0 = 0;
    uint16_t queueDepth = 0;
    uint16_t lastNoteUtf8Len = 0;
    uint64_t lastCommandId = 0;
    char lastNoteUtf8[64]{};
};

struct IpcRequest {
    IpcCommand command;
    uint16_t   paramLen = 0;
    uint8_t    param[256]{};
};

constexpr IpcRequest MakeTriggerSendScanModeRequest(uint8_t freq1,
                                                    uint8_t freq2,
                                                    uint8_t mode) noexcept {
    IpcRequest request{};
    request.command = IpcCommand::TriggerSendScanMode;
    request.paramLen = 3;
    request.param[0] = freq1;
    request.param[1] = freq2;
    request.param[2] = mode;
    return request;
}

constexpr IpcRequest MakeTriggerSendFactoryInitParamsRequest() noexcept {
    IpcRequest request{};
    request.command = IpcCommand::TriggerSendFactoryInitParams;
    return request;
}

struct IpcResponse {
    IpcStatusCode status = IpcStatusCode::InternalError;
    bool     success = false;
    uint16_t dataLen = 0;
    uint8_t  data[kIpcResponseDataBytes]{};
};

inline void MarkSuccess(IpcResponse& resp) noexcept {
    resp.success = true;
    resp.status = IpcStatusCode::Ok;
}

inline void MarkFailure(IpcResponse& resp, IpcStatusCode status) noexcept {
    resp.success = false;
    resp.status = status;
}

// ABI layout guards — catch cross-process struct layout changes at compile time
static_assert(sizeof(IpcRequest) == 260,
    "IpcRequest size changed");
static_assert(sizeof(IpcResponse) == 4100,
    "IpcResponse size changed");
static_assert(sizeof(DebugSnapshotHeader) == kDebugSnapshotLegacyHeaderSize,
    "DebugSnapshotHeader must remain the legacy 8-byte wire header");
static_assert(sizeof(DebugSnapshotMetadataWire) == kDebugSnapshotMetadataWireSize,
    "DebugSnapshotMetadataWire size changed");
static_assert(kDebugSnapshotMaxValues == 255,
    "DebugSnapshot value capacity changed");
static_assert(sizeof(ConfigV3PageRequestWire) <= 256,
    "Config v3 page request must fit in IpcRequest::param");
static_assert(sizeof(ApplyConfigPatchV3RequestWire) <= 256,
    "Config v3 patch request must fit in IpcRequest::param");
static_assert(sizeof(ConfigV3PageResponseHeaderWire) < kIpcResponseDataBytes,
    "Config v3 response header must leave room for page bytes");
static_assert(sizeof(ConfigV3ApplyResultWire) < kIpcResponseDataBytes,
    "Config v3 apply result must fit in IpcResponse::data");
static_assert(sizeof(PersistConfigV3ResponseWire) < kIpcResponseDataBytes,
    "Config v3 persist result must fit in IpcResponse::data");
static_assert(kConfigPatchV3PayloadBytes == 240,
    "Config v3 patch payload capacity changed");
static_assert(ConfigV3PageCapacityBytes() == kIpcResponseDataBytes - sizeof(ConfigV3PageResponseHeaderWire),
    "Config v3 page capacity calculation changed");

} // namespace Ipc
