#pragma once
// IpcPipeServer: Named Pipe server for EGoTouchService.
// Runs a background thread, waits for App connection, dispatches commands.

#include "Ipc/IpcProtocol.h"
#include <atomic>
#include <functional>
#include <thread>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace Ipc {

// Callback type for command dispatch
using CommandHandler = std::function<IpcResponse(const IpcRequest&)>;

class IpcPipeServer {
public:
    IpcPipeServer() = default;
    ~IpcPipeServer() { Stop(); }
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

    void ServerLoop();

    CommandHandler   m_handler;
    std::atomic<bool> m_running{false};
    std::atomic<ServerState> m_state{ServerState::Idle};
    std::thread      m_thread;
};

} // namespace Ipc
