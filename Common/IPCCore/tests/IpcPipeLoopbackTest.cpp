#if __has_include("Ipc/IpcPipeClient.h")
#include "Ipc/IpcPipeClient.h"
#include "Ipc/IpcPipeServer.h"
#else
#include "IpcPipeClient.h"
#include "IpcPipeServer.h"
#endif

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "[FAIL] " << message << "\n";
        std::exit(1);
    }
}

struct ScopedHandle {
    HANDLE value = nullptr;
    explicit ScopedHandle(HANDLE handle = nullptr) : value(handle) {}
    ~ScopedHandle() {
        if (value && value != INVALID_HANDLE_VALUE) {
            CloseHandle(value);
        }
    }
    ScopedHandle(const ScopedHandle&) = delete;
    ScopedHandle& operator=(const ScopedHandle&) = delete;
};

bool IsElevatedAdmin() {
    ScopedHandle token;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token.value)) {
        return false;
    }

    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    PSID adminSid = nullptr;
    if (!AllocateAndInitializeSid(&ntAuthority, 2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0,
                                  &adminSid)) {
        return false;
    }

    BOOL isAdmin = FALSE;
    const BOOL membershipOk = CheckTokenMembership(nullptr, adminSid, &isAdmin);
    FreeSid(adminSid);
    if (!membershipOk || !isAdmin) {
        return false;
    }

    TOKEN_ELEVATION elevation{};
    DWORD returned = 0;
    if (!GetTokenInformation(token.value, TokenElevation, &elevation, sizeof(elevation), &returned)) {
        return false;
    }
    return elevation.TokenIsElevated != 0;
}

bool PipeAlreadyAvailable() {
    ScopedHandle pipe(CreateFileW(Ipc::kPipeName,
                                  GENERIC_READ | GENERIC_WRITE,
                                  0,
                                  nullptr,
                                  OPEN_EXISTING,
                                  0,
                                  nullptr));
    if (pipe.value != INVALID_HANDLE_VALUE) {
        return true;
    }

    const DWORD createError = GetLastError();
    if (createError == ERROR_PIPE_BUSY) {
        return true;
    }

    if (WaitNamedPipeW(Ipc::kPipeName, 0)) {
        return true;
    }
    const DWORD waitError = GetLastError();
    return waitError == ERROR_SEM_TIMEOUT || waitError == ERROR_PIPE_BUSY;
}

Ipc::IpcResponse SendRequest(Ipc::IpcCommand command) {
    Ipc::IpcPipeClient client;
    Require(client.Connect(3000), "client connects to loopback pipe server");
    Ipc::IpcRequest request{};
    request.command = command;
    return client.Send(request);
}

} // namespace

int main() {
    using namespace Ipc;

    if (!IsElevatedAdmin()) {
        std::cout << "[SKIP] IpcPipeLoopbackTest requires an elevated Administrators token because IpcPipeServer validates pipe clients.\n";
        return 0;
    }

    if (PipeAlreadyAvailable()) {
        std::cout << "[SKIP] " << "Fixed IPC pipe name appears to be in use; not connecting to a possible real service.\n";
        return 0;
    }

    IpcPipeClient disconnectedClient;
    IpcRequest ping{};
    ping.command = IpcCommand::Ping;
    IpcResponse disconnectedResponse = disconnectedClient.Send(ping);
    Require(disconnectedResponse.status == IpcStatusCode::InternalError, "disconnected client Send returns default InternalError response");
    Require(!disconnectedResponse.success, "disconnected client Send returns unsuccessful response");

    IpcPipeServer server;
    uint32_t pingPayload = 0x4f4b4f4bu;
    server.SetCommandHandler([&](const IpcRequest& request) {
        IpcResponse response{};
        if (request.command == IpcCommand::Ping) {
            MarkSuccess(response);
            response.dataLen = sizeof(pingPayload);
            std::memcpy(response.data, &pingPayload, sizeof(pingPayload));
            return response;
        }
        MarkFailure(response, IpcStatusCode::InvalidRequest);
        return response;
    });
    Require(server.Start(), "pipe server starts");
    Require(server.IsRunning(), "pipe server reports running");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    IpcResponse pingResponse = SendRequest(IpcCommand::Ping);
    Require(pingResponse.success, "Ping loopback succeeds");
    Require(pingResponse.status == IpcStatusCode::Ok, "Ping loopback returns Ok status");
    Require(pingResponse.dataLen == sizeof(pingPayload), "Ping loopback returns handler payload length");
    uint32_t returnedPayload = 0;
    std::memcpy(&returnedPayload, pingResponse.data, sizeof(returnedPayload));
    Require(returnedPayload == pingPayload, "Ping loopback returns handler payload bytes");

    IpcResponse handlerFailureResponse = SendRequest(IpcCommand::StartRuntime);
    Require(!handlerFailureResponse.success, "handler failure response is unsuccessful");
    Require(handlerFailureResponse.status == IpcStatusCode::InvalidRequest, "handler failure status is propagated");

    IpcResponse unknownResponse = SendRequest(static_cast<IpcCommand>(0xff));
    Require(!unknownResponse.success, "unknown command response is unsuccessful");
    Require(unknownResponse.status == IpcStatusCode::UnsupportedCommand, "unknown command returns UnsupportedCommand before handler dispatch");

    server.SetCommandHandler({});
    IpcResponse noHandlerResponse = SendRequest(IpcCommand::Ping);
    Require(!noHandlerResponse.success, "missing handler response is unsuccessful");
    Require(noHandlerResponse.status == IpcStatusCode::InvalidState, "missing handler returns InvalidState");

    server.Stop();
    Require(!server.IsRunning(), "pipe server stops");
    server.Stop();

    std::cout << "[PASS] IpcPipeLoopbackTest\n";
    return 0;
}
