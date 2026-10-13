#include "himax/PanelPowerWatcher.h"
#include "Logger.h"

#include <windows.h>

namespace {

// 与 HimaxProtocol.cpp 的 IOCTL 表一致
constexpr DWORD kIoctlWaitInterrupt = 0x4001c20;
constexpr DWORD kIoctlReadAcpi      = 0x4001c38;
// 句柄打不开 / 通知立刻失败时的重试间隔
constexpr DWORD kRetryDelayMs = 1000;
constexpr DWORD kInlineCompletionPauseMs = 10;

struct Channel {
    HANDLE handle = INVALID_HANDLE_VALUE;
    HANDLE waitEvent = nullptr;   // 挂起的中断通知
    HANDLE acpiEvent = nullptr;   // ACPI 读取（与通知同时进行，要用各自的事件）

    ~Channel() {
        Close();
        if (waitEvent) CloseHandle(waitEvent);
        if (acpiEvent) CloseHandle(acpiEvent);
    }

    bool Open(const wchar_t* path) {
        if (!waitEvent) waitEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!acpiEvent) acpiEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!waitEvent || !acpiEvent) return false;
        handle = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
        return handle != INVALID_HANDLE_VALUE;
    }

    void Close() {
        if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
    }

    /// SpiModule!SpiReadAcpi：4 字节输出，首字节 bit0 = 面板已上电
    std::optional<uint8_t> ReadAcpi() {
        uint8_t out[4]{};
        DWORD got = 0;
        OVERLAPPED ov{};
        ov.hEvent = acpiEvent;
        ResetEvent(acpiEvent);
        BOOL ok = DeviceIoControl(handle, kIoctlReadAcpi, nullptr, 0, out, sizeof(out), &got, &ov);
        if (!ok && GetLastError() == ERROR_IO_PENDING) {
            ok = GetOverlappedResult(handle, &ov, &got, TRUE);
        }
        if (!ok || got < sizeof(out)) return std::nullopt;
        return out[0];
    }
};

} // anonymous namespace

namespace Himax {

PanelPowerWatcher::PanelPowerWatcher(std::wstring path) : m_path(std::move(path)) {
    m_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
}

PanelPowerWatcher::~PanelPowerWatcher() {
    Stop();
    if (m_stopEvent) CloseHandle(static_cast<HANDLE>(m_stopEvent));
}

void PanelPowerWatcher::Start(Callback cb) {
    if (m_thread.joinable() || !m_stopEvent) return;
    m_cb = std::move(cb);
    ResetEvent(static_cast<HANDLE>(m_stopEvent));
    m_thread = std::thread(&PanelPowerWatcher::Run, this);
}

void PanelPowerWatcher::Stop() {
    if (!m_thread.joinable()) return;
    SetEvent(static_cast<HANDLE>(m_stopEvent));
    m_thread.join();
}

void PanelPowerWatcher::Run() {
    const HANDLE stopEvent = static_cast<HANDLE>(m_stopEvent);
    auto stopRequestedWithin = [&](DWORD ms) { return WaitForSingleObject(stopEvent, ms) == WAIT_OBJECT_0; };

    Channel ch;
    bool openFailureLogged = false;
    bool waitFailureLogged = false;
    std::optional<uint8_t> reported;   // 上一次交给回调的 ACPI 值
    bool logLevel = true;              // 句柄刚打开 / 刚收到中断：这一轮读到的电平无论变没变都记一行

    for (;;) {
        if (ch.handle == INVALID_HANDLE_VALUE) {
            if (!ch.Open(m_path.c_str())) {
                if (!openFailureLogged) {
                    openFailureLogged = true;
                    LOG_WARN("PanelPower", __func__, "watcher", "Cannot open panel power channel (win32={}); retrying.",
                             GetLastError());
                }
                if (stopRequestedWithin(kRetryDelayMs)) return;
                continue;
            }
            openFailureLogged = false;
            logLevel = true;
        }

        // 先把通知挂上，再读电平：电平在两次通知之间变化也不会漏
        OVERLAPPED ov{};
        ov.hEvent = ch.waitEvent;
        ResetEvent(ch.waitEvent);
        DWORD got = 0;
        const BOOL completedInline = DeviceIoControl(ch.handle, kIoctlWaitInterrupt, nullptr, 0, nullptr, 0, &got, &ov);
        const DWORD issueError = completedInline ? ERROR_SUCCESS : GetLastError();
        const bool pending = !completedInline && issueError == ERROR_IO_PENDING;

        if (!completedInline && !pending) {
            if (!waitFailureLogged) {
                waitFailureLogged = true;
                LOG_WARN("PanelPower", __func__, "watcher", "Interrupt notification rejected (win32={}); reopening.",
                         issueError);
            }
            ch.Close();
            if (stopRequestedWithin(kRetryDelayMs)) return;
            continue;
        }

        const auto raw = ch.ReadAcpi();
        if (raw) {
            if (logLevel || raw != reported) {
                LOG_NOTICE("PanelPower", __func__, "watcher", "Panel power ACPI=0x{:02X} (bit0 {}).", *raw,
                           (*raw & 0x01) ? "on" : "off");
            }
            if (raw != reported) {
                reported = raw;
                if (m_cb) m_cb((*raw & 0x01) != 0, *raw);
            }
        } else if (logLevel) {
            LOG_WARN("PanelPower", __func__, "watcher", "Panel power ACPI unreadable (win32={}).", GetLastError());
        }
        logLevel = false;

        BOOL ok = completedInline;
        DWORD err = ERROR_SUCCESS;
        if (pending) {
            const HANDLE waits[2] = {stopEvent, ch.waitEvent};
            if (WaitForMultipleObjects(2, waits, FALSE, INFINITE) == WAIT_OBJECT_0) {
                // 请求还在驱动里，等它确实完成后 OVERLAPPED 才能离开作用域
                CancelIoEx(ch.handle, &ov);
                (void)GetOverlappedResult(ch.handle, &ov, &got, TRUE);
                return;
            }
            ok = GetOverlappedResult(ch.handle, &ov, &got, FALSE);
            err = ok ? ERROR_SUCCESS : GetLastError();
        }

        if (!ok) {
            if (!waitFailureLogged) {
                waitFailureLogged = true;
                LOG_WARN("PanelPower", __func__, "watcher", "Interrupt notification failed (win32={}); reopening.", err);
            }
            ch.Close();
            if (stopRequestedWithin(kRetryDelayMs)) return;
            continue;
        }
        waitFailureLogged = false;

        const uint32_t count = m_notifications.fetch_add(1, std::memory_order_relaxed) + 1;
        LOG_NOTICE("PanelPower", __func__, "watcher", "Panel power interrupt #{}.", count);
        logLevel = true;
        // 通知没有挂起就直接完成时不能空转
        if (stopRequestedWithin(completedInline ? kInlineCompletionPauseMs : 0)) return;
    }
}

} // namespace Himax
