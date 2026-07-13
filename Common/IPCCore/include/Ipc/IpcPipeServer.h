#pragma once
// IpcPipeServer: Named Pipe server for EGoTouchService.
// Runs a background thread, waits for App connection, dispatches commands.

#include "Ipc/IpcProtocol.h"
#include "Ipc/IpcSecurity.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace Ipc {

// Callback type for command dispatch
using CommandHandler = std::function<IpcResponse(const IpcRequest&)>;

struct IpcPipeServerStartupOps {
    using BuildSecurity = std::function<bool(SECURITY_ATTRIBUTES&, ScopedSecurityDescriptor&)>;
    using CreatePipe = std::function<HANDLE(SECURITY_ATTRIBUTES*)>;

    BuildSecurity buildSecurity;
    CreatePipe createPipe;
};

class IpcPipeServer {
public:
    IpcPipeServer();
    explicit IpcPipeServer(IpcPipeServerStartupOps startupOps);
    ~IpcPipeServer();
    IpcPipeServer(const IpcPipeServer&) = delete;
    IpcPipeServer& operator=(const IpcPipeServer&) = delete;

    void SetCommandHandler(CommandHandler handler);
    bool Start();
    void Stop();
    bool IsRunning() const { return m_running.load(); }

private:
    enum class ServerState {
        Idle,
        Connecting,
        Reading,
        Handling,
    };

    enum class StartupState {
        Stopped,
        Starting,
        Ready,
        Failed,
    };

    void ServerLoop();
    void PublishStartupState(StartupState state) noexcept;

    IpcPipeServerStartupOps m_startupOps;
    CommandHandler m_handler;
    mutable std::mutex m_handlerMutex;

    std::atomic<bool> m_running{false};
    std::atomic<ServerState> m_state{ServerState::Idle};
    std::thread m_thread;

    mutable std::mutex m_lifecycleMutex;
    std::mutex m_startupMutex;
    std::condition_variable m_startupCv;
    StartupState m_startupState = StartupState::Stopped;
};

} // namespace Ipc
