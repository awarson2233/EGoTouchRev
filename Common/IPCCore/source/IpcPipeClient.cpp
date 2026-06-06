#include "Ipc/IpcPipeClient.h"
#include "Logger.h"

#include <algorithm>
#include <cstring>
#include <cwchar>

namespace Ipc {

bool IpcPipeClient::Connect(DWORD timeoutMs) {
    std::lock_guard<std::mutex> lock(m_pipeMutex);
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
    std::lock_guard<std::mutex> lock(m_pipeMutex);
    if (m_pipe != INVALID_HANDLE_VALUE) {
        CloseHandle(m_pipe);
        m_pipe = INVALID_HANDLE_VALUE;
    }
}

bool IpcPipeClient::IsConnected() const {
    std::lock_guard<std::mutex> lock(m_pipeMutex);
    return m_pipe != INVALID_HANDLE_VALUE;
}

IpcResponse IpcPipeClient::Send(const IpcRequest& req) {
    IpcResponse resp{};
    std::lock_guard<std::mutex> lock(m_pipeMutex);
    if (m_pipe == INVALID_HANDLE_VALUE) return resp;

    auto closePipe = [&]() noexcept {
        CloseHandle(m_pipe);
        m_pipe = INVALID_HANDLE_VALUE;
    };

    if (req.paramLen > sizeof(req.param)) {
        LOG_ERROR("IPC", __func__, "IPC", "Refusing oversized IPC request paramLen={}", req.paramLen);
        MarkFailure(resp, IpcStatusCode::InvalidRequest);
        closePipe();
        return resp;
    }

    DWORD bytesWritten = 0;
    if (!WriteFile(m_pipe, &req, sizeof(req), &bytesWritten, nullptr) || bytesWritten != sizeof(req)) {
        LOG_ERROR("IPC", __func__, "IPC", "WriteFile failed or wrote a partial request: error={} bytesWritten={}",  GetLastError(), bytesWritten);
        closePipe();
        return resp;
    }

    DWORD bytesRead = 0;
    if (!ReadFile(m_pipe, &resp, sizeof(resp), &bytesRead, nullptr) || bytesRead != sizeof(resp)) {
        LOG_ERROR("IPC", __func__, "IPC", "ReadFile failed or read a partial response: error={} bytesRead={}",  GetLastError(), bytesRead);
        closePipe();
        resp = {};
        return resp;
    }
    if (resp.dataLen > sizeof(resp.data)) {
        LOG_ERROR("IPC", __func__, "IPC", "Received oversized IPC response dataLen={}", resp.dataLen);
        closePipe();
        resp = {};
        MarkFailure(resp, IpcStatusCode::InvalidRequest);
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
    const size_t capacityChars = sizeof(req.param) / sizeof(wchar_t);
    const size_t sourceChars = shmName ? std::wcslen(shmName) : 0;
    const size_t copyChars = std::min(sourceChars, capacityChars - 1);
    auto* dst = reinterpret_cast<wchar_t*>(req.param);
    if (copyChars != 0) {
        std::memcpy(dst, shmName, copyChars * sizeof(wchar_t));
    }
    dst[copyChars] = L'\0';
    req.paramLen = static_cast<uint16_t>((copyChars + 1) * sizeof(wchar_t));
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

IpcResponse IpcPipeClient::GetConfigSnapshot() {
    IpcRequest req{}; req.command = IpcCommand::GetConfigSnapshot;
    return Send(req);
}

IpcResponse IpcPipeClient::ApplyConfigPatch(const ApplyConfigPatchRequestWire& patch) {
    IpcRequest req{};
    req.command = IpcCommand::ApplyConfigPatch;
    req.paramLen = static_cast<uint16_t>(sizeof(patch));
    std::memcpy(req.param, &patch, sizeof(patch));
    return Send(req);
}

IpcResponse IpcPipeClient::PersistConfig() {
    IpcRequest req{}; req.command = IpcCommand::PersistConfig;
    return Send(req);
}

} // namespace Ipc
