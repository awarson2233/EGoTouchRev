#pragma once
// IpcProtocol: Command protocol for Named Pipe between App and Service.

#include <cstdint>

namespace Ipc {

constexpr uint16_t kIpcProtocolVersion = 2;

constexpr const wchar_t* kPipeName = L"\\\\.\\pipe\\EGoTouchControl";
// IPC-related global events
constexpr const wchar_t* kLogReadyEventName = L"Global\\EGoTouchLogReady";
constexpr const wchar_t* kPenReadyEventName = L"Global\\EGoTouchPenStatusReady";

// NOTE:
// - ReloadConfig / SaveConfig remain for transition compatibility.
// - GetConfigSnapshot / ApplyConfigPatch / PersistConfig are the Phase 0
//   canonical config-control commands.
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
    // Config (legacy compatibility)
    ReloadConfig   = 40,  // Re-read config.ini; response data may include ReloadConfigSummaryWire
    SaveConfig     = 41,  // Legacy alias for PersistConfig
    // Config (Phase 0 canonical control path)
    GetConfigSnapshot = 42,
    ApplyConfigPatch  = 43,
    PersistConfig     = 44,
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
};

enum class IpcStatusCode : uint8_t {
    Ok = 0,
    UnsupportedCommand = 1,
    InvalidRequest = 2,
    InvalidState = 3,
    NotFound = 4,
    PermissionDenied = 5,
    InternalError = 6,
};

enum class ServiceModeWire : uint8_t {
    Full = 0,
    TouchOnly = 1,
};

enum class ServiceConfigFieldWire : uint8_t {
    None = 0,
    Mode = 1u << 0,
    AutoMode = 1u << 1,
    StylusVhfEnabled = 1u << 2,
    PenButtonMode = 1u << 3,
    PenButtonRoute = 1u << 4,
};

enum class PenButtonModeWire : uint8_t {
    OemCustom = 0,
    NativeBarrel = 1,
    NativeEraser = 2,
};

enum class PenButtonRouteWire : uint8_t {
    VhfOnly = 0,
    Win32Only = 1,
    VhfAndWin32 = 2,
};

constexpr uint8_t ToBits(ServiceConfigFieldWire field) noexcept {
    return static_cast<uint8_t>(field);
}

constexpr bool HasField(uint8_t fieldMask, ServiceConfigFieldWire field) noexcept {
    return (fieldMask & ToBits(field)) != 0;
}

struct ConfigSnapshotWire {
    uint16_t wireVersion = kIpcProtocolVersion;
    uint8_t definedFields =
        ToBits(ServiceConfigFieldWire::Mode) |
        ToBits(ServiceConfigFieldWire::AutoMode) |
        ToBits(ServiceConfigFieldWire::StylusVhfEnabled) |
        ToBits(ServiceConfigFieldWire::PenButtonMode) |
        ToBits(ServiceConfigFieldWire::PenButtonRoute);
    uint8_t desiredMode = static_cast<uint8_t>(ServiceModeWire::Full);
    uint8_t activeMode = static_cast<uint8_t>(ServiceModeWire::Full);
    uint8_t autoMode = 1;
    uint8_t stylusVhfEnabled = 1;
    uint8_t penButtonMode = static_cast<uint8_t>(PenButtonModeWire::OemCustom);
    uint8_t penButtonRoute = static_cast<uint8_t>(PenButtonRouteWire::VhfOnly);
};

struct ApplyConfigPatchRequestWire {
    uint16_t wireVersion = kIpcProtocolVersion;
    uint8_t fieldMask = 0;
    uint8_t desiredMode = static_cast<uint8_t>(ServiceModeWire::Full);
    uint8_t autoMode = 1;
    uint8_t stylusVhfEnabled = 1;
    uint8_t penButtonMode = static_cast<uint8_t>(PenButtonModeWire::OemCustom);
    uint8_t penButtonRoute = static_cast<uint8_t>(PenButtonRouteWire::VhfOnly);
};

struct ConfigMutationResultWire {
    uint16_t wireVersion = kIpcProtocolVersion;
    uint8_t changedFields = 0;
    uint8_t appliedFields = 0;
    uint8_t restartRequiredFields = 0;
    uint8_t _reserved0 = 0;
};

struct PersistConfigResponseWire {
    uint16_t wireVersion = kIpcProtocolVersion;
    uint8_t persistedFields = 0;
    uint8_t _reserved0 = 0;
};

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

struct DebugSnapshotHeader {
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

// Legacy response wire for ReloadConfig. Keep the 3-byte layout for transition compatibility.
struct ReloadConfigSummaryWire {
    // Bit layout (LSB-first):
    // bit0: [Service].mode
    // bit1: [Service].auto_mode
    // bit2: [Service].stylus_vhf_enabled
    uint8_t changedFields = 0;
    uint8_t appliedFields = 0;
    uint8_t restartRequiredFields = 0;
};

constexpr uint8_t kPenIdentityHasStylusId = 1u << 0;
constexpr uint8_t kPenIdentityHasPenModuleModelId = 1u << 1;
constexpr uint8_t kPenIdentityHasHardwareVersion = 1u << 2;
constexpr uint8_t kPenIdentityConnected = 1u << 3;

struct PenIdentityStatusWire {
    uint16_t wireVersion = kIpcProtocolVersion;
    uint8_t flags = 0;
    uint8_t stylusId = 0;
    uint32_t penModuleModelId = 0;
    uint16_t hardwareVersionUtf8Len = 0;
    uint16_t _reserved0 = 0;
    char hardwareVersionUtf8[128]{};
};

struct IpcRequest {
    IpcCommand command;
    uint16_t   paramLen = 0;
    uint8_t    param[256]{};
};

struct IpcResponse {
    IpcStatusCode status = IpcStatusCode::InternalError;
    bool     success = false;
    uint16_t dataLen = 0;
    uint8_t  data[4096]{};
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

} // namespace Ipc
