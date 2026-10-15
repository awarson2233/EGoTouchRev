#include "btmcu/BtHidChannel.h"

#include <windows.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <thread>

namespace {

using namespace std::chrono_literals;

constexpr auto kStateTimeout = 8s;

struct HandleGuard {
    HANDLE value = INVALID_HANDLE_VALUE;

    HandleGuard() = default;
    explicit HandleGuard(HANDLE handle) : value(handle) {}
    ~HandleGuard() { Reset(); }

    HandleGuard(const HandleGuard&) = delete;
    HandleGuard& operator=(const HandleGuard&) = delete;

    void Reset(HANDLE handle = INVALID_HANDLE_VALUE) noexcept {
        if (value != INVALID_HANDLE_VALUE && value != nullptr) {
            CloseHandle(value);
        }
        value = handle;
    }

    bool IsValid() const noexcept {
        return value != INVALID_HANDLE_VALUE && value != nullptr;
    }
};

/// Stands in for the col00 HID device. Each accepted connection parks in a
/// blocking ReadFile, so the server naturally recycles the instance as soon as
/// the channel closes its end — which is exactly what a reconnect looks like.
class FakeDeviceServer {
public:
    FakeDeviceServer() {
        std::wostringstream stream;
        stream << L"\\\\.\\pipe\\EGoTouchBtHidChannelReconnect_"
               << GetCurrentProcessId() << L"_" << GetCurrentThreadId();
        m_path = stream.str();
    }

    ~FakeDeviceServer() { Stop(); }

    const std::wstring& Path() const { return m_path; }
    int Connections() const { return m_connections.load(std::memory_order_acquire); }

    bool Start() {
        m_running.store(true, std::memory_order_release);
        m_thread = std::thread([this] { Loop(); });

        std::unique_lock<std::mutex> lock(m_mu);
        return m_cv.wait_for(lock, kStateTimeout, [this] { return m_listening || m_failed; }) &&
               !m_failed;
    }

    void Stop() {
        if (!m_running.exchange(false, std::memory_order_acq_rel)) return;
        // Unblock a thread parked in ConnectNamedPipe. The caller is expected to
        // have stopped the channel first, so no real client remains attached.
        HandleGuard waker(CreateFileW(m_path.c_str(),
                                      GENERIC_READ | GENERIC_WRITE,
                                      0,
                                      nullptr,
                                      OPEN_EXISTING,
                                      0,
                                      nullptr));
        if (m_thread.joinable()) m_thread.join();
    }

private:
    void Loop() {
        while (m_running.load(std::memory_order_acquire)) {
            HandleGuard pipe(CreateNamedPipeW(
                m_path.c_str(),
                PIPE_ACCESS_DUPLEX,
                PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                PIPE_UNLIMITED_INSTANCES,
                64,
                64,
                0,
                nullptr));
            if (!pipe.IsValid()) {
                std::lock_guard<std::mutex> lock(m_mu);
                m_failed = true;
                m_cv.notify_all();
                return;
            }

            {
                std::lock_guard<std::mutex> lock(m_mu);
                m_listening = true;
            }
            m_cv.notify_all();

            if (!ConnectNamedPipe(pipe.value, nullptr) &&
                GetLastError() != ERROR_PIPE_CONNECTED) {
                continue;
            }
            if (!m_running.load(std::memory_order_acquire)) return;

            m_connections.fetch_add(1, std::memory_order_acq_rel);

            // Park until the client end closes; that returns ERROR_BROKEN_PIPE
            // and lets the loop offer a fresh instance for the reconnect.
            uint8_t scratch = 0;
            DWORD transferred = 0;
            (void)ReadFile(pipe.value, &scratch, 1, &transferred, nullptr);
        }
    }

    std::wstring m_path;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<int> m_connections{0};
    std::mutex m_mu;
    std::condition_variable m_cv;
    bool m_listening = false;
    bool m_failed = false;
};

class ProbeChannel : public Himax::Pen::BtHidChannel {
public:
    explicit ProbeChannel(std::wstring path) : m_path(std::move(path)) {}
    ~ProbeChannel() override { Stop(); }

    int ConnectCount() const { return m_connectCount.load(std::memory_order_acquire); }
    int IdleTicks() const { return m_idleTicks.load(std::memory_order_acquire); }

protected:
    std::optional<std::wstring> FindDevicePath() override { return m_path; }
    void OnConnected() override { m_connectCount.fetch_add(1, std::memory_order_acq_rel); }
    void OnPacketReceived(std::span<const uint8_t>) override {}
    void OnIdleTick() override { m_idleTicks.fetch_add(1, std::memory_order_acq_rel); }
    const char* ChannelName() const override { return "ProbeChannel"; }

private:
    std::wstring m_path;
    std::atomic<int> m_connectCount{0};
    std::atomic<int> m_idleTicks{0};
};

template <typename Predicate>
bool WaitFor(Predicate predicate) {
    const auto deadline = std::chrono::steady_clock::now() + kStateTimeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate()) return true;
        std::this_thread::sleep_for(20ms);
    }
    return predicate();
}

bool RunReconnectRequestReopensChannelTest() {
    FakeDeviceServer server;
    if (!server.Start()) {
        std::cerr << "[TEST] Fake device server failed to start.\n";
        return false;
    }

    ProbeChannel channel(server.Path());
    if (!channel.Start()) {
        std::cerr << "[TEST] Channel failed to start.\n";
        return false;
    }

    if (!WaitFor([&] { return channel.ConnectCount() >= 1; })) {
        std::cerr << "[TEST] Channel never reported an initial connection.\n";
        return false;
    }

    // The read timeout must surface as an idle tick, otherwise the passive
    // handshake watchdog in PenEventBridge would never run.
    if (!WaitFor([&] { return channel.IdleTicks() >= 1; })) {
        std::cerr << "[TEST] Read timeouts did not produce an idle tick.\n";
        return false;
    }

    channel.RequestReconnect();

    if (!WaitFor([&] { return channel.ConnectCount() >= 2; })) {
        std::cerr << "[TEST] RequestReconnect did not reopen the channel; connects="
                  << channel.ConnectCount() << "\n";
        return false;
    }

    if (server.Connections() < 2) {
        std::cerr << "[TEST] Reconnect did not produce a second device open; opens="
                  << server.Connections() << "\n";
        return false;
    }

    channel.Stop();
    server.Stop();
    return true;
}

bool RunReconnectRequestIsIgnoredWhenStoppedTest() {
    FakeDeviceServer server;
    if (!server.Start()) {
        std::cerr << "[TEST] Fake device server failed to start.\n";
        return false;
    }

    ProbeChannel channel(server.Path());
    channel.RequestReconnect();
    if (!channel.Start()) {
        std::cerr << "[TEST] Channel failed to start after a stopped reconnect request.\n";
        return false;
    }

    if (!WaitFor([&] { return channel.ConnectCount() >= 1; })) {
        std::cerr << "[TEST] Channel never connected after a stopped reconnect request.\n";
        return false;
    }

    // A request raised while stopped must not survive into the new session and
    // tear down the freshly opened channel.
    std::this_thread::sleep_for(1500ms);
    if (channel.ConnectCount() != 1) {
        std::cerr << "[TEST] Stale reconnect request forced an extra reopen; connects="
                  << channel.ConnectCount() << "\n";
        return false;
    }

    channel.Stop();
    server.Stop();
    return true;
}

} // namespace

int main() {
    if (!RunReconnectRequestReopensChannelTest()) {
        return 1;
    }
    if (!RunReconnectRequestIsIgnoredWhenStoppedTest()) {
        return 1;
    }
    std::cout << "[TEST] BtHidChannel reconnect tests passed.\n";
    return 0;
}
