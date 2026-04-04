#include "IpcPipeClient.h"
#include "Logger.h"
#include <cstring>

namespace Ipc {

bool IpcPipeClient::Connect(DWORD timeoutMs) {
    if (m_pipe != INVALID_HANDLE_VALUE) return true;

    if (!WaitNamedPipeW(kPipeName, timeoutMs)) {
        LOG_ERROR("IPC", __func__, "IPC", "No pipe server available (timeout={}ms).",  timeoutMs);
        return false;
    }

    m_pipe = CreateFileW(
        kPipeName, GENERIC_READ | GENERIC_WRITE,
        0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (m_pipe == INVALID_HANDLE_VALUE) {
        LOG_ERROR("IPC", __func__, "IPC", "CreateFile failed: {}",  GetLastError());
        return false;
    }

    // Set message mode
    DWORD mode = PIPE_READMODE_MESSAGE;
    SetNamedPipeHandleState(m_pipe, &mode, nullptr, nullptr);

    LOG_INFO("IPC", __func__, "IPC", "Connected to service.");
    return true;
}

void IpcPipeClient::Disconnect() {
    if (m_pipe != INVALID_HANDLE_VALUE) {
        CloseHandle(m_pipe);
        m_pipe = INVALID_HANDLE_VALUE;
    }
}

IpcResponse IpcPipeClient::Send(const IpcRequest& req) {
    IpcResponse resp{};
    if (m_pipe == INVALID_HANDLE_VALUE) return resp;

    DWORD bytesWritten = 0;
    if (!WriteFile(m_pipe, &req, sizeof(req), &bytesWritten, nullptr)) {
        LOG_ERROR("IPC", __func__, "IPC", "WriteFile failed: {}",  GetLastError());
        Disconnect();
        return resp;
    }

    DWORD bytesRead = 0;
    if (!ReadFile(m_pipe, &resp, sizeof(resp), &bytesRead, nullptr)) {
        LOG_ERROR("IPC", __func__, "IPC", "ReadFile failed: {}",  GetLastError());
        Disconnect();
        return resp;
    }
    return resp;
}

IpcResponse IpcPipeClient::Ping() {
    IpcRequest req{}; req.command = IpcCommand::Ping;
    return Send(req);
}

IpcResponse IpcPipeClient::EnterDebugMode(const wchar_t* shmName) {
    IpcRequest req{}; req.command = IpcCommand::EnterDebugMode;
    const size_t nameBytes = (wcslen(shmName) + 1) * sizeof(wchar_t);
    req.paramLen = static_cast<uint16_t>(std::min(nameBytes, sizeof(req.param)));
    std::memcpy(req.param, shmName, req.paramLen);
    return Send(req);
}

IpcResponse IpcPipeClient::ExitDebugMode() {
    IpcRequest req{}; req.command = IpcCommand::ExitDebugMode;
    return Send(req);
}

IpcResponse IpcPipeClient::SendAfeCommand(uint8_t afeCmd, uint8_t param) {
    IpcRequest req{}; req.command = IpcCommand::AfeCommand;
    req.param[0] = afeCmd; req.param[1] = param; req.paramLen = 2;
    return Send(req);
}

IpcResponse IpcPipeClient::StartRuntime() {
    IpcRequest req{}; req.command = IpcCommand::StartRuntime;
    return Send(req);
}

IpcResponse IpcPipeClient::StopRuntime() {
    IpcRequest req{}; req.command = IpcCommand::StopRuntime;
    return Send(req);
}

IpcResponse IpcPipeClient::ReloadConfig() {
    IpcRequest req{}; req.command = IpcCommand::ReloadConfig;
    return Send(req);
}

IpcResponse IpcPipeClient::SaveConfig() {
    IpcRequest req{}; req.command = IpcCommand::SaveConfig;
    return Send(req);
}

} // namespace Ipc
