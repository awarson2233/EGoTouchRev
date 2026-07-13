#include "Ipc/IpcPipeServer.h"
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
    return IsSupportedIpcCommand(command);
}

bool ValidateClient(HANDLE pipe, ClientSecurityContext& context) noexcept {
    if (!ImpersonateNamedPipeClient(pipe)) {
        LOG_WARN("IPC", __func__, "IPC", "ImpersonateNamedPipeClient failed: {}", GetLastError());
        return false;
    }
    ImpersonationScope impersonation{true};

    ScopedHandle token;
    if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &token.value)) {
        LOG_WARN("IPC", __func__, "IPC", "OpenThreadToken failed: {}", GetLastError());
        return false;
    }

    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    ScopedSid adminSid;
    if (!AllocateAndInitializeSid(&ntAuthority, 2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0,
                                  &adminSid.value)) {
        LOG_WARN("IPC", __func__, "IPC", "AllocateAndInitializeSid failed: {}", GetLastError());
        return false;
    }

    BOOL isAdmin = FALSE;
    if (!CheckTokenMembership(token.value, adminSid.value, &isAdmin)) {
        LOG_WARN("IPC", __func__, "IPC", "CheckTokenMembership failed: {}", GetLastError());
        return false;
    }
    if (!isAdmin) {
        LOG_WARN("IPC", __func__, "IPC", "Pipe client token is not an Administrators member.");
        return false;
    }

    TOKEN_ELEVATION elevation{};
    DWORD returned = 0;
    if (!GetTokenInformation(token.value,
                             TokenElevation,
                             &elevation,
                             sizeof(elevation),
                             &returned)) {
        LOG_WARN("IPC", __func__, "IPC", "GetTokenInformation(TokenElevation) failed: {}", GetLastError());
        return false;
    }

    context.elevatedAdmin = elevation.TokenIsElevated != 0;
    if (!context.elevatedAdmin) {
        LOG_WARN("IPC", __func__, "IPC", "Pipe client token is not elevated.");
    }
    return context.elevatedAdmin;
}

bool IsCommandAllowed(IpcCommand command, const ClientSecurityContext& context) noexcept {
    return IsKnownCommand(command) && context.elevatedAdmin;
}

bool WriteFullResponse(HANDLE pipe, const IpcResponse& resp) noexcept {
    DWORD bytesWritten = 0;
    const BOOL ok = WriteFile(pipe, &resp, sizeof(resp), &bytesWritten, nullptr);
    return ok && bytesWritten == sizeof(resp);
}

HANDLE CreateDefaultPipe(SECURITY_ATTRIBUTES* securityAttributes) {
    return CreateNamedPipeW(
        kPipeName,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        sizeof(IpcResponse),
        sizeof(IpcRequest),
        0,
        securityAttributes);
}

} // namespace

IpcPipeServer::IpcPipeServer()
    : IpcPipeServer(IpcPipeServerStartupOps{}) {}

IpcPipeServer::IpcPipeServer(IpcPipeServerStartupOps startupOps)
    : m_startupOps(std::move(startupOps)) {
    if (!m_startupOps.buildSecurity) {
        m_startupOps.buildSecurity = [](SECURITY_ATTRIBUTES& sa,
                                        ScopedSecurityDescriptor& sd) {
            return BuildAdminOnlySecurityAttributes(sa, sd);
        };
    }
    if (!m_startupOps.createPipe) {
        m_startupOps.createPipe = &CreateDefaultPipe;
    }
}

IpcPipeServer::~IpcPipeServer() {
    Stop();
}

void IpcPipeServer::SetCommandHandler(CommandHandler handler) {
    std::lock_guard<std::mutex> lock(m_handlerMutex);
    m_handler = std::move(handler);
}

bool IpcPipeServer::Start() {
    std::unique_lock<std::mutex> lifecycleLock(m_lifecycleMutex);
    if (m_running.load(std::memory_order_acquire)) {
        return true;
    }

    if (m_thread.joinable()) {
        m_thread.join();
    }

    {
        std::lock_guard<std::mutex> startupLock(m_startupMutex);
        m_startupState = StartupState::Starting;
    }
    m_state.store(ServerState::Idle, std::memory_order_release);
    m_running.store(true, std::memory_order_release);

    try {
        m_thread = std::thread(&IpcPipeServer::ServerLoop, this);
    } catch (...) {
        m_running.store(false, std::memory_order_release);
        PublishStartupState(StartupState::Failed);
        LOG_ERROR("IPC", __func__, "IPC", "Failed to create pipe server thread.");
        return false;
    }

    StartupState startupState = StartupState::Failed;
    {
        std::unique_lock<std::mutex> startupLock(m_startupMutex);
        m_startupCv.wait(startupLock, [this] {
            return m_startupState != StartupState::Starting;
        });
        startupState = m_startupState;
    }

    if (startupState != StartupState::Ready) {
        m_running.store(false, std::memory_order_release);
        if (m_thread.joinable()) {
            m_thread.join();
        }
        LOG_ERROR("IPC", __func__, "IPC", "Pipe server failed startup readiness handshake.");
        return false;
    }

    LOG_INFO("IPC", __func__, "IPC", "Pipe server ready.");
    return true;
}

void IpcPipeServer::Stop() {
    std::unique_lock<std::mutex> lifecycleLock(m_lifecycleMutex);
    m_running.store(false, std::memory_order_release);

    if (m_thread.joinable()) {
        const ServerState state = m_state.load(std::memory_order_acquire);
        if (state == ServerState::Connecting || state == ServerState::Reading) {
            CancelSynchronousIo(m_thread.native_handle());
        }

        HANDLE h = CreateFileW(
            kPipeName, GENERIC_READ | GENERIC_WRITE,
            0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (h != INVALID_HANDLE_VALUE) {
            CloseHandle(h);
        }
        m_thread.join();
    }

    m_state.store(ServerState::Idle, std::memory_order_release);
    {
        std::lock_guard<std::mutex> startupLock(m_startupMutex);
        m_startupState = StartupState::Stopped;
    }
    m_startupCv.notify_all();
    LOG_INFO("IPC", __func__, "IPC", "Pipe server stopped.");
}

void IpcPipeServer::PublishStartupState(StartupState state) noexcept {
    {
        std::lock_guard<std::mutex> startupLock(m_startupMutex);
        m_startupState = state;
    }
    m_startupCv.notify_all();
}

void IpcPipeServer::ServerLoop() {
    ScopedSecurityDescriptor sd;
    SECURITY_ATTRIBUTES sa{};
    try {
        if (!m_startupOps.buildSecurity(sa, sd)) {
            LOG_ERROR("IPC", __func__, "IPC", "Failed to create pipe security descriptor: {}", GetLastError());
            m_running.store(false, std::memory_order_release);
            PublishStartupState(StartupState::Failed);
            return;
        }
    } catch (...) {
        LOG_ERROR("IPC", __func__, "IPC", "Pipe security initialization threw an exception.");
        m_running.store(false, std::memory_order_release);
        PublishStartupState(StartupState::Failed);
        return;
    }

    bool startupPublished = false;
    while (m_running.load(std::memory_order_acquire)) {
        m_state.store(ServerState::Idle, std::memory_order_release);
        HANDLE pipe = INVALID_HANDLE_VALUE;
        try {
            pipe = m_startupOps.createPipe(&sa);
        } catch (...) {
            LOG_ERROR("IPC", __func__, "IPC", "Pipe creation threw an exception.");
        }

        if (pipe == INVALID_HANDLE_VALUE) {
            LOG_ERROR("IPC", __func__, "IPC", "CreateNamedPipe failed: {}", GetLastError());
            m_running.store(false, std::memory_order_release);
            if (!startupPublished) {
                PublishStartupState(StartupState::Failed);
            }
            break;
        }

        if (!startupPublished) {
            startupPublished = true;
            PublishStartupState(StartupState::Ready);
        }

        m_state.store(ServerState::Connecting, std::memory_order_release);
        BOOL connected = ConnectNamedPipe(pipe, nullptr)
                         ? TRUE
                         : (GetLastError() == ERROR_PIPE_CONNECTED);
        m_state.store(ServerState::Idle, std::memory_order_release);
        if (!connected || !m_running.load(std::memory_order_acquire)) {
            CloseHandle(pipe);
            continue;
        }

        ClientSecurityContext client{};
        bool clientValidated = false;

        LOG_INFO("IPC", __func__, "IPC", "Client connected.");

        while (m_running.load(std::memory_order_acquire)) {
            IpcRequest req{};
            DWORD bytesRead = 0;
            m_state.store(ServerState::Reading, std::memory_order_release);
            if (!m_running.load(std::memory_order_acquire)) {
                m_state.store(ServerState::Idle, std::memory_order_release);
                break;
            }
            BOOL ok = ReadFile(pipe, &req, sizeof(req),
                               &bytesRead, nullptr);
            m_state.store(ServerState::Idle, std::memory_order_release);
            if (!ok || bytesRead != sizeof(IpcRequest)) break;

            m_state.store(ServerState::Handling, std::memory_order_release);
            IpcResponse resp{};
            if (!clientValidated) {
                if (!ValidateClient(pipe, client)) {
                    MarkFailure(resp, IpcStatusCode::PermissionDenied);
                    WriteFullResponse(pipe, resp);
                    LOG_WARN("IPC", __func__, "IPC", "Rejected non-elevated or non-admin pipe client.");
                    m_state.store(ServerState::Idle, std::memory_order_release);
                    break;
                }
                clientValidated = true;
            }
            if (req.paramLen > sizeof(req.param)) {
                MarkFailure(resp, IpcStatusCode::InvalidRequest);
                m_state.store(ServerState::Idle, std::memory_order_release);
                if (!WriteFullResponse(pipe, resp)) break;
                continue;
            }

            CommandHandler handler;
            {
                std::lock_guard<std::mutex> handlerLock(m_handlerMutex);
                handler = m_handler;
            }

            if (!IsKnownCommand(req.command)) {
                MarkFailure(resp, IpcStatusCode::UnsupportedCommand);
            } else if (!IsCommandAllowed(req.command, client)) {
                MarkFailure(resp, IpcStatusCode::PermissionDenied);
            } else if (handler) {
                resp = handler(req);
            } else {
                MarkFailure(resp, IpcStatusCode::InvalidState);
            }

            if (resp.dataLen > sizeof(resp.data)) {
                LOG_WARN("IPC", __func__, "IPC", "Handler returned oversized IPC response dataLen={}", resp.dataLen);
                resp = {};
                MarkFailure(resp, IpcStatusCode::InternalError);
            }

            m_state.store(ServerState::Idle, std::memory_order_release);
            if (!WriteFullResponse(pipe, resp)) break;
        }

        m_state.store(ServerState::Idle, std::memory_order_release);
        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
        LOG_INFO("IPC", __func__, "IPC", "Client disconnected.");
    }

    if (!startupPublished) {
        PublishStartupState(StartupState::Failed);
    }
    m_state.store(ServerState::Idle, std::memory_order_release);
}

} // namespace Ipc
