#pragma once
// IpcProtocol: Command protocol for Named Pipe between App and Service.

#include <cstdint>

namespace Ipc {

constexpr const wchar_t* kPipeName = L"\\\\.\\pipe\\EGoTouchControl";
// IPC-related global events
constexpr const wchar_t* kLogReadyEventName = L"Global\\EGoTouchLogReady";
constexpr const wchar_t* kPenReadyEventName = L"Global\\EGoTouchPenStatusReady";

enum class IpcCommand : uint8_t {
    Ping = 0,
    // Debug mode: App tells Service to open shared memory and start pushing
    EnterDebugMode = 1,   // Request carries shared memory name (wchar_t[])
    ExitDebugMode  = 2,   // Service stops pushing, closes shared memory
    // Hardware control
    StartRuntime   = 10,
    StopRuntime    = 11,
    // AFE
    AfeCommand     = 20,  // param[0] = AFE_Command, param[1] = uint8_t
    // VHF
    SetVhfEnabled    = 30,
    SetVhfTranspose  = 31,
    SetAutoAfeSync   = 32,  // param[0] = bool enabled
    // Config
    ReloadConfig   = 40,  // Force Service to re-read config.ini
    SaveConfig     = 41,  // Service saves current params to config.ini
    // Logs
    GetLogs        = 50,  // App requests recent log lines from Service
    // PenBridge (BT MCU)
    GetPenBridgeStatus = 60,  // App queries live pressure stats + running state
};

struct IpcRequest {
    IpcCommand command;
    uint16_t   paramLen = 0;
    uint8_t    param[256]{};
};

struct IpcResponse {
    bool     success = false;
    uint16_t dataLen = 0;
    uint8_t  data[4096]{};
};

} // namespace Ipc
