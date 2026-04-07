#include "IpcPipeClient.h"
#include <iostream>
#include <windows.h>
#include <thread>
#include <chrono>

using namespace Ipc;

bool TestTimeout() {
    std::cout << "[TEST] Running TestTimeout...\n";
    IpcPipeClient client;

    // There is no pipe server running, so this should timeout and return false.
    // We use a small timeout to speed up the test.
    bool connected = client.Connect(10);

    if (connected) {
        std::cerr << "[TEST] Failed: Connect() should have timed out and returned false.\n";
        return false;
    }

    std::cout << "[TEST] TestTimeout passed.\n";
    return true;
}

bool TestCreateFileFailure() {
    std::cout << "[TEST] Running TestCreateFileFailure...\n";

    // Create a pipe server that only allows INBOUND traffic.
    // When the client tries to connect with GENERIC_READ | GENERIC_WRITE,
    // CreateFileW will fail (likely with ERROR_ACCESS_DENIED or similar).
    HANDLE hPipe = CreateNamedPipeW(
        kPipeName,
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1, sizeof(IpcResponse), sizeof(IpcRequest),
        0, nullptr);

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "[TEST] Failed: Could not create mock pipe server. Error: " << GetLastError() << "\n";
        return false;
    }

    IpcPipeClient client;
    bool connected = client.Connect(100);

    if (connected) {
        std::cerr << "[TEST] Failed: Connect() should have failed at CreateFileW due to access restrictions.\n";
        CloseHandle(hPipe);
        return false;
    }

    CloseHandle(hPipe);
    std::cout << "[TEST] TestCreateFileFailure passed.\n";
    return true;
}

int main() {
    std::cout << "[TEST] Starting IpcPipeClient Edge Case Tests...\n";

    if (!TestTimeout()) {
        return 1;
    }

    if (!TestCreateFileFailure()) {
        return 1;
    }

    std::cout << "[TEST] All IpcPipeClient tests passed.\n";
    return 0;
}
