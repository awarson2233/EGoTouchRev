#include "IpcPipeServer.h"
#include "Logger.h"
#include <sddl.h>

namespace Ipc {

void IpcPipeServer::SetCommandHandler(CommandHandler handler) {
    m_handler = std::move(handler);
}

bool IpcPipeServer::Start() {
    if (m_running.load()) return true;
    m_running.store(true);
    m_thread = std::thread(&IpcPipeServer::ServerLoop, this);
    LOG_INFO("IPC", __func__, "IPC", "Pipe server started.");
    return true;
}

void IpcPipeServer::Stop() {
    m_running.store(false);
    // Unblock ConnectNamedPipe by creating a dummy connection
    HANDLE h = CreateFileW(
        kPipeName, GENERIC_READ | GENERIC_WRITE,
        0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
    if (m_thread.joinable()) m_thread.join();
    LOG_INFO("IPC", __func__, "IPC", "Pipe server stopped.");
}

void IpcPipeServer::ServerLoop() {
    // Build secure security descriptor for cross-session access
    // (Service runs as SYSTEM, App runs as user)
    // D: (DACL)
    // (A;;GA;;;SY)  - Allow Full Access to SYSTEM
    // (A;;GA;;;BA)  - Allow Full Access to Built-in Administrators
    // (A;;GRGW;;;BU) - Allow Read/Write to Built-in Users
    LPCWSTR sddl = L"D:(A;;GA;;;SY)(A;;GA;;;BA)(A;;GRGW;;;BU)";
    PSECURITY_DESCRIPTOR pSd = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            sddl, SDDL_REVISION_1, &pSd, nullptr)) {
        LOG_ERROR("IPC", __func__, "IPC", "Failed to create security descriptor: {}", GetLastError());
        return;
    }

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = pSd;
    sa.bInheritHandle = FALSE;

    while (m_running.load()) {
        // Create pipe instance
        HANDLE pipe = CreateNamedPipeW(
            kPipeName,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1, sizeof(IpcResponse), sizeof(IpcRequest),
            0, &sa);
        if (pipe == INVALID_HANDLE_VALUE) {
            LOG_ERROR("IPC", __func__, "IPC", "CreateNamedPipe failed: {}",  GetLastError());
            break;
        }

        // Wait for client connection
        BOOL connected = ConnectNamedPipe(pipe, nullptr)
                         ? TRUE
                         : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (!connected || !m_running.load()) {
            CloseHandle(pipe);
            continue;
        }
        LOG_INFO("IPC", __func__, "IPC", "Client connected.");

        // Read/dispatch loop for this client
        while (m_running.load()) {
            IpcRequest req{};
            DWORD bytesRead = 0;
            BOOL ok = ReadFile(pipe, &req, sizeof(req),
                               &bytesRead, nullptr);
            if (!ok || bytesRead < sizeof(IpcCommand)) break;

            IpcResponse resp{};
            if (m_handler) {
                resp = m_handler(req);
            } else {
                resp.success = false;
            }

            DWORD bytesWritten = 0;
            WriteFile(pipe, &resp, sizeof(resp),
                      &bytesWritten, nullptr);
        }

        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
        LOG_INFO("IPC", __func__, "IPC", "Client disconnected.");
    }

    if (pSd) {
        LocalFree(pSd);
    }
}

} // namespace Ipc
