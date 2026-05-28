#include "IpcPipeServer.h"
#include "IpcSecurity.h"
#include "Logger.h"

#include <utility>

namespace Ipc {

namespace {

struct ScopedHandle {
    HANDLE value = nullptr;
    ~ScopedHandle() {
        if (value) {
            CloseHandle(value);
        }
    }
};

struct ScopedSid {
    PSID value = nullptr;
    ~ScopedSid() {
        if (value) {
            FreeSid(value);
        }
    }
};

struct ImpersonationScope {
    bool active = false;
    ~ImpersonationScope() {
        if (active) {
            RevertToSelf();
        }
    }
};

struct ClientSecurityContext {
    bool elevatedAdmin = false;
};

bool IsKnownCommand(IpcCommand command) noexcept {
    switch (command) {
    case IpcCommand::Ping:
    case IpcCommand::EnterDebugMode:
    case IpcCommand::ExitDebugMode:
    case IpcCommand::StartRuntime:
    case IpcCommand::StopRuntime:
    case IpcCommand::AfeCommand:
    case IpcCommand::SetVhfEnabled:
    case IpcCommand::SetVhfTranspose:
    case IpcCommand::ReloadConfig:
    case IpcCommand::SaveConfig:
    case IpcCommand::GetConfigSnapshot:
    case IpcCommand::ApplyConfigPatch:
    case IpcCommand::PersistConfig:
    case IpcCommand::GetLogs:
    case IpcCommand::GetPenBridgeStatus:
    case IpcCommand::GetDebugSchema:
    case IpcCommand::GetDebugSnapshot:
    case IpcCommand::SetPenPressureMode:
    case IpcCommand::SetMasterParserOnly:
        return true;
    default:
        return false;
    }
}

bool ValidateClient(HANDLE pipe, ClientSecurityContext& context) noexcept {
    if (!ImpersonateNamedPipeClient(pipe)) {
        return false;
    }
    ImpersonationScope impersonation{true};

    ScopedHandle token;
    if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &token.value)) {
        return false;
    }

    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    ScopedSid adminSid;
    if (!AllocateAndInitializeSid(&ntAuthority, 2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0,
                                  &adminSid.value)) {
        return false;
    }

    BOOL isAdmin = FALSE;
    if (!CheckTokenMembership(token.value, adminSid.value, &isAdmin) || !isAdmin) {
        return false;
    }

    TOKEN_ELEVATION elevation{};
    DWORD returned = 0;
    if (!GetTokenInformation(token.value,
                             TokenElevation,
                             &elevation,
                             sizeof(elevation),
                             &returned)) {
        return false;
    }

    context.elevatedAdmin = elevation.TokenIsElevated != 0;
    return context.elevatedAdmin;
}

bool IsCommandAllowed(IpcCommand command, const ClientSecurityContext& context) noexcept {
    return IsKnownCommand(command) && context.elevatedAdmin;
}

} // namespace

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
    HANDLE h = CreateFileW(
        kPipeName, GENERIC_READ | GENERIC_WRITE,
        0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
    if (m_thread.joinable()) m_thread.join();
    LOG_INFO("IPC", __func__, "IPC", "Pipe server stopped.");
}

void IpcPipeServer::ServerLoop() {
    ScopedSecurityDescriptor sd;
    SECURITY_ATTRIBUTES sa{};
    if (!BuildAdminOnlySecurityAttributes(sa, sd)) {
        LOG_ERROR("IPC", __func__, "IPC", "Failed to create pipe security descriptor: {}", GetLastError());
        return;
    }

    while (m_running.load()) {
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

        BOOL connected = ConnectNamedPipe(pipe, nullptr)
                         ? TRUE
                         : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (!connected || !m_running.load()) {
            CloseHandle(pipe);
            continue;
        }

        ClientSecurityContext client{};
        if (!ValidateClient(pipe, client)) {
            LOG_WARN("IPC", __func__, "IPC", "Rejected non-elevated or non-admin pipe client.");
            DisconnectNamedPipe(pipe);
            CloseHandle(pipe);
            continue;
        }

        LOG_INFO("IPC", __func__, "IPC", "Client connected.");

        while (m_running.load()) {
            IpcRequest req{};
            DWORD bytesRead = 0;
            BOOL ok = ReadFile(pipe, &req, sizeof(req),
                               &bytesRead, nullptr);
            if (!ok || bytesRead < sizeof(IpcCommand)) break;

            IpcResponse resp{};
            if (!IsKnownCommand(req.command)) {
                MarkFailure(resp, IpcStatusCode::UnsupportedCommand);
            } else if (!IsCommandAllowed(req.command, client)) {
                MarkFailure(resp, IpcStatusCode::PermissionDenied);
            } else if (m_handler) {
                resp = m_handler(req);
            } else {
                MarkFailure(resp, IpcStatusCode::InvalidState);
            }

            DWORD bytesWritten = 0;
            WriteFile(pipe, &resp, sizeof(resp),
                      &bytesWritten, nullptr);
        }

        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
        LOG_INFO("IPC", __func__, "IPC", "Client disconnected.");
    }
}

} // namespace Ipc
