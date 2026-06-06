#if __has_include("Ipc/IpcPipeClient.h")
#include "Ipc/IpcPipeClient.h"
#include "Ipc/IpcPipeServer.h"
#else
#include "IpcPipeClient.h"
#include "IpcPipeServer.h"
#endif

#include <atomic>
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
    ScopedHandle(ScopedHandle&& other) noexcept : value(other.value) {
        other.value = nullptr;
    }
    ScopedHandle& operator=(ScopedHandle&& other) noexcept {
        if (this != &other) {
            if (value && value != INVALID_HANDLE_VALUE) {
                CloseHandle(value);
            }
            value = other.value;
            other.value = nullptr;
        }
        return *this;
    }
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

ScopedHandle ConnectRawClient() {
    ScopedHandle pipe(CreateFileW(Ipc::kPipeName,
                                  GENERIC_READ | GENERIC_WRITE,
                                  0,
                                  nullptr,
                                  OPEN_EXISTING,
                                  0,
                                  nullptr));
    Require(pipe.value != INVALID_HANDLE_VALUE, "raw pipe client connects");
    DWORD mode = PIPE_READMODE_MESSAGE;
    SetNamedPipeHandleState(pipe.value, &mode, nullptr, nullptr);
    return pipe;
}

void WriteRawRequest(HANDLE pipe, const void* data, DWORD size) {
    DWORD bytesWritten = 0;
    const BOOL ok = WriteFile(pipe, data, size, &bytesWritten, nullptr);
    Require(ok && bytesWritten == size, "raw request writes complete test payload");
}

bool ReadRawResponse(HANDLE pipe, Ipc::IpcResponse& response, DWORD& bytesRead) {
    bytesRead = 0;
    return ReadFile(pipe, &response, sizeof(response), &bytesRead, nullptr) != FALSE;
}

Ipc::IpcResponse SendWithFakeServer(const Ipc::IpcRequest& request,
                                    const Ipc::IpcResponse* response,
                                    DWORD responseBytes,
                                    bool expectRequest) {
    std::atomic<bool> ready{false};
    std::thread fakeServer([&]() {
        ScopedHandle pipe(CreateNamedPipeW(Ipc::kPipeName,
                                           PIPE_ACCESS_DUPLEX,
                                           PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                           1,
                                           sizeof(Ipc::IpcResponse),
                                           sizeof(Ipc::IpcRequest),
                                           0,
                                           nullptr));
        Require(pipe.value != INVALID_HANDLE_VALUE, "fake pipe server creates pipe");
        ready.store(true, std::memory_order_release);
        const BOOL connected = ConnectNamedPipe(pipe.value, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        Require(connected, "fake pipe server accepts client");

        Ipc::IpcRequest received{};
        DWORD bytesRead = 0;
        const BOOL readOk = ReadFile(pipe.value, &received, sizeof(received), &bytesRead, nullptr);
        if (expectRequest) {
            Require(readOk && bytesRead == sizeof(received), "fake server receives complete request");
        } else if (readOk) {
            Require(bytesRead == 0, "client-side validation should not send request bytes");
        }

        if (response && responseBytes != 0) {
            DWORD bytesWritten = 0;
            WriteFile(pipe.value, response, responseBytes, &bytesWritten, nullptr);
        }
        DisconnectNamedPipe(pipe.value);
    });

    while (!ready.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    Ipc::IpcPipeClient client;
    Require(client.Connect(3000), "client connects to fake pipe server");
    Ipc::IpcResponse result = client.Send(request);
    fakeServer.join();
    return result;
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

    IpcRequest oversizedParamRequest{};
    oversizedParamRequest.command = IpcCommand::Ping;
    oversizedParamRequest.paramLen = static_cast<uint16_t>(sizeof(oversizedParamRequest.param) + 1);
    IpcResponse oversizedParamClientResponse = SendWithFakeServer(oversizedParamRequest, nullptr, 0, false);
    Require(!oversizedParamClientResponse.success, "client rejects oversized request paramLen");
    Require(oversizedParamClientResponse.status == IpcStatusCode::InvalidRequest, "client oversized paramLen returns InvalidRequest");

    IpcResponse shortWireResponse{};
    MarkSuccess(shortWireResponse);
    IpcResponse shortResponseResult = SendWithFakeServer(ping, &shortWireResponse, sizeof(IpcStatusCode), true);
    Require(!shortResponseResult.success, "client rejects short response frame");
    Require(shortResponseResult.status == IpcStatusCode::InternalError, "client short response returns default failure");

    IpcResponse oversizedDataWireResponse{};
    MarkSuccess(oversizedDataWireResponse);
    oversizedDataWireResponse.dataLen = static_cast<uint16_t>(sizeof(oversizedDataWireResponse.data) + 1);
    IpcResponse oversizedDataClientResponse = SendWithFakeServer(ping, &oversizedDataWireResponse, sizeof(oversizedDataWireResponse), true);
    Require(!oversizedDataClientResponse.success, "client rejects oversized response dataLen");
    Require(oversizedDataClientResponse.status == IpcStatusCode::InvalidRequest, "client oversized dataLen returns InvalidRequest");

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
        if (request.command == IpcCommand::StopRuntime) {
            MarkSuccess(response);
            response.dataLen = static_cast<uint16_t>(sizeof(response.data) + 1);
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

    IpcResponse handlerOversizedResponse = SendRequest(IpcCommand::StopRuntime);
    Require(!handlerOversizedResponse.success, "server rejects handler oversized response dataLen");
    Require(handlerOversizedResponse.status == IpcStatusCode::InternalError, "handler oversized dataLen returns InternalError");

    {
        ScopedHandle rawClient = ConnectRawClient();
        IpcRequest oversizedServerRequest{};
        oversizedServerRequest.command = IpcCommand::Ping;
        oversizedServerRequest.paramLen = static_cast<uint16_t>(sizeof(oversizedServerRequest.param) + 1);
        WriteRawRequest(rawClient.value, &oversizedServerRequest, sizeof(oversizedServerRequest));
        IpcResponse oversizedServerResponse{};
        DWORD bytesRead = 0;
        Require(ReadRawResponse(rawClient.value, oversizedServerResponse, bytesRead), "server sends response for oversized request paramLen");
        Require(bytesRead == sizeof(oversizedServerResponse), "server oversized paramLen response is complete");
        Require(!oversizedServerResponse.success, "server rejects oversized request paramLen");
        Require(oversizedServerResponse.status == IpcStatusCode::InvalidRequest, "server oversized paramLen returns InvalidRequest");
    }

    {
        ScopedHandle rawClient = ConnectRawClient();
        IpcRequest shortRequest{};
        shortRequest.command = IpcCommand::Ping;
        WriteRawRequest(rawClient.value, &shortRequest, sizeof(IpcCommand));
        IpcResponse shortRequestResponse{};
        DWORD bytesRead = 0;
        const bool readOk = ReadRawResponse(rawClient.value, shortRequestResponse, bytesRead);
        Require(!readOk || bytesRead == 0, "server closes pipe on short request without a response");
    }

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
