#pragma once
// IpcPipeClient: Named Pipe client for EGoTouchApp.

#include "Ipc/IpcProtocol.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace Ipc {

class IpcPipeClient {
public:
    IpcPipeClient() = default;
    ~IpcPipeClient() { Disconnect(); }
    IpcPipeClient(const IpcPipeClient&) = delete;
    IpcPipeClient& operator=(const IpcPipeClient&) = delete;

    bool Connect(DWORD timeoutMs = 5000);
    void Disconnect();
    bool IsConnected() const { return m_pipe != INVALID_HANDLE_VALUE; }

    // Send a command and receive response
    IpcResponse Send(const IpcRequest& req);

    // Convenience helpers
    IpcResponse Ping();
    IpcResponse EnterDebugMode(const wchar_t* shmName);
    IpcResponse ExitDebugMode();
    IpcResponse SendAfeCommand(uint8_t afeCmd, uint8_t param);
    IpcResponse StartRuntime();
    IpcResponse StopRuntime();
    IpcResponse ReloadConfig();
    IpcResponse SaveConfig();
    IpcResponse GetConfigSnapshot();
    IpcResponse ApplyConfigPatch(const ApplyConfigPatchRequestWire& patch);
    IpcResponse PersistConfig();

private:
    HANDLE m_pipe = INVALID_HANDLE_VALUE;
};

} // namespace Ipc
