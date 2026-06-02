#include "btmcu/PenUsbTransport.h"
#include <array>
#include <mutex>
#include <windows.h>

namespace Himax::Pen {

class PenUsbTransportWin32 final : public IPenUsbTransport {
public:
    PenUsbTransportWin32() = default;
    ~PenUsbTransportWin32() override { Close(); }

    ChipResult<> Open(const std::wstring& devicePath) override {
        Close();

        std::lock_guard<std::mutex> lk(m_handleMu);
        m_handle = ::CreateFileW(devicePath.c_str(),
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 nullptr,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                 nullptr);
        if (m_handle == INVALID_HANDLE_VALUE) {
            return std::unexpected(ChipError::CommunicationError);
        }

        m_readEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
        m_writeEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!m_readEvent || !m_writeEvent) {
            CloseLocked();
            return std::unexpected(ChipError::CommunicationError);
        }

        m_readOv = {};
        m_readOv.hEvent = m_readEvent;
        m_readPending = false;
        m_writeOv = {};
        m_writeOv.hEvent = m_writeEvent;
        m_writeBytes = 0;
        m_writePending = false;
        return {};
    }

    void Close() override {
        std::lock_guard<std::mutex> lk(m_handleMu);
        CloseLocked();
    }

    void CancelIo() override {
        std::lock_guard<std::mutex> lk(m_handleMu);
        if (m_handle != INVALID_HANDLE_VALUE) {
            ::CancelIoEx(m_handle, nullptr);
        }
    }

    bool IsOpen() const override {
        std::lock_guard<std::mutex> lk(m_handleMu);
        return m_handle != INVALID_HANDLE_VALUE;
    }

    ChipResult<> ReadPacket(std::vector<uint8_t>& outBytes, uint32_t timeoutMs) override {
        HANDLE handle = INVALID_HANDLE_VALUE;
        HANDLE readEvent = nullptr;
        {
            std::lock_guard<std::mutex> lk(m_handleMu);
            if (m_handle == INVALID_HANDLE_VALUE || !m_readEvent) {
                return std::unexpected(ChipError::InvalidOperation);
            }
            handle = m_handle;
            readEvent = m_readEvent;
            if (m_readPending) {
                DWORD drainedBytes = 0;
                if (!TryDrainReadLocked(drainedBytes)) {
                    return std::unexpected(ChipError::Timeout);
                }
            }

            m_readBuffer = {};
            m_readBytes = 0;
            m_readOv = {};
            m_readOv.hEvent = m_readEvent;
            ::ResetEvent(m_readEvent);
        }

        BOOL ok = ::ReadFile(handle,
                             m_readBuffer.data(),
                             static_cast<DWORD>(m_readBuffer.size()),
                             &m_readBytes,
                             &m_readOv);
        if (!ok) {
            DWORD err = ::GetLastError();
            if (err != ERROR_IO_PENDING) {
                return std::unexpected(ChipError::CommunicationError);
            }
            {
                std::lock_guard<std::mutex> lk(m_handleMu);
                m_readPending = true;
            }

            DWORD waitRes = ::WaitForSingleObject(readEvent, timeoutMs);
            if (waitRes == WAIT_TIMEOUT) {
                std::lock_guard<std::mutex> lk(m_handleMu);
                if (m_handle != INVALID_HANDLE_VALUE) {
                    ::CancelIoEx(m_handle, &m_readOv);
                }
                (void)DrainReadLocked(50);
                return std::unexpected(ChipError::Timeout);
            } else if (waitRes != WAIT_OBJECT_0) {
                std::lock_guard<std::mutex> lk(m_handleMu);
                if (m_handle != INVALID_HANDLE_VALUE) {
                    ::CancelIoEx(m_handle, &m_readOv);
                }
                (void)DrainReadLocked(50);
                return std::unexpected(ChipError::CommunicationError);
            }

            std::lock_guard<std::mutex> lk(m_handleMu);
            if (!TryDrainReadLocked(m_readBytes)) {
                return std::unexpected(ChipError::CommunicationError);
            }
        }

        if (m_readBytes == 0) {
            return std::unexpected(ChipError::Timeout);
        }

        outBytes.assign(m_readBuffer.begin(), m_readBuffer.begin() + static_cast<std::ptrdiff_t>(m_readBytes));
        return {};
    }

    ChipResult<> WritePacket(std::span<const uint8_t> bytes) override {
        if (bytes.empty()) {
            return std::unexpected(ChipError::InvalidOperation);
        }

        HANDLE writeEvent = nullptr;
        DWORD bytesWritten = 0;
        bool writePending = false;
        {
            std::lock_guard<std::mutex> lk(m_handleMu);
            if (m_handle == INVALID_HANDLE_VALUE || !m_writeEvent || m_writePending) {
                return std::unexpected(ChipError::InvalidOperation);
            }
            writeEvent = m_writeEvent;
            m_writeBytes = 0;
            m_writeOv = {};
            m_writeOv.hEvent = m_writeEvent;
            ::ResetEvent(m_writeEvent);
            m_writePending = true;

            BOOL ok = ::WriteFile(m_handle,
                                  bytes.data(),
                                  static_cast<DWORD>(bytes.size()),
                                  &m_writeBytes,
                                  &m_writeOv);
            if (ok) {
                bytesWritten = m_writeBytes;
                m_writePending = false;
            } else {
                DWORD err = ::GetLastError();
                if (err != ERROR_IO_PENDING) {
                    m_writePending = false;
                    return std::unexpected(ChipError::CommunicationError);
                }
                writePending = true;
            }
        }

        if (writePending) {
            DWORD waitRes = ::WaitForSingleObject(writeEvent, 2000);
            if (waitRes == WAIT_TIMEOUT) {
                std::lock_guard<std::mutex> lk(m_handleMu);
                if (m_handle != INVALID_HANDLE_VALUE) {
                    ::CancelIoEx(m_handle, &m_writeOv);
                }
                (void)DrainWriteLocked(INFINITE);
                return std::unexpected(ChipError::Timeout);
            } else if (waitRes != WAIT_OBJECT_0) {
                std::lock_guard<std::mutex> lk(m_handleMu);
                if (m_handle != INVALID_HANDLE_VALUE) {
                    ::CancelIoEx(m_handle, &m_writeOv);
                }
                (void)DrainWriteLocked(INFINITE);
                return std::unexpected(ChipError::CommunicationError);
            }

            std::lock_guard<std::mutex> lk(m_handleMu);
            if (m_writePending) {
                if (!TryDrainWriteLocked(bytesWritten)) {
                    return std::unexpected(ChipError::CommunicationError);
                }
            } else {
                if (m_handle == INVALID_HANDLE_VALUE) {
                    return std::unexpected(ChipError::CommunicationError);
                }
                bytesWritten = m_writeBytes;
            }
        }

        if (bytesWritten != bytes.size()) {
            return std::unexpected(ChipError::CommunicationError);
        }
        return {};
    }

private:
    bool TryDrainReadLocked(DWORD& bytesTransferred) {
        if (m_handle == INVALID_HANDLE_VALUE || !m_readPending) {
            m_readPending = false;
            return true;
        }

        if (!::GetOverlappedResult(m_handle, &m_readOv, &bytesTransferred, FALSE)) {
            const DWORD err = ::GetLastError();
            if (err == ERROR_IO_INCOMPLETE) {
                return false;
            }
        }
        m_readPending = false;
        return true;
    }

    bool DrainReadLocked(DWORD waitMs) {
        if (!m_readPending || !m_readEvent) {
            return true;
        }
        if (::WaitForSingleObject(m_readEvent, waitMs) != WAIT_OBJECT_0) {
            return false;
        }
        DWORD ignored = 0;
        return TryDrainReadLocked(ignored);
    }

    bool TryDrainWriteLocked(DWORD& bytesTransferred) {
        if (m_handle == INVALID_HANDLE_VALUE || !m_writePending) {
            m_writePending = false;
            return true;
        }

        if (!::GetOverlappedResult(m_handle, &m_writeOv, &bytesTransferred, FALSE)) {
            const DWORD err = ::GetLastError();
            if (err == ERROR_IO_INCOMPLETE) {
                return false;
            }
        }
        m_writeBytes = bytesTransferred;
        m_writePending = false;
        return true;
    }

    bool DrainWriteLocked(DWORD waitMs) {
        if (!m_writePending || !m_writeEvent) {
            return true;
        }
        if (::WaitForSingleObject(m_writeEvent, waitMs) != WAIT_OBJECT_0) {
            return false;
        }
        DWORD ignored = 0;
        return TryDrainWriteLocked(ignored);
    }

    void CloseLocked() {
        if (m_handle != INVALID_HANDLE_VALUE) {
            ::CancelIoEx(m_handle, nullptr);
            if (m_readPending && !DrainReadLocked(INFINITE)) {
                // Preserve the handle, event, OVERLAPPED storage and pending flag
                // rather than freeing resources that the kernel may still signal.
                return;
            }
            if (m_writePending && !DrainWriteLocked(INFINITE)) {
                // Preserve the handle, event, OVERLAPPED storage and pending flag
                // rather than freeing resources that the kernel may still signal.
                return;
            }
            ::CloseHandle(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
        }
        if (m_readEvent) {
            ::CloseHandle(m_readEvent);
            m_readEvent = nullptr;
        }
        if (m_writeEvent) {
            ::CloseHandle(m_writeEvent);
            m_writeEvent = nullptr;
        }
        m_readPending = false;
        m_writePending = false;
    }

    mutable std::mutex m_handleMu;
    HANDLE m_handle = INVALID_HANDLE_VALUE;
    HANDLE m_readEvent = nullptr;
    HANDLE m_writeEvent = nullptr;
    OVERLAPPED m_readOv{};
    OVERLAPPED m_writeOv{};
    std::array<uint8_t, 64> m_readBuffer{};
    DWORD m_readBytes = 0;
    DWORD m_writeBytes = 0;
    bool m_readPending = false;
    bool m_writePending = false;
};

std::unique_ptr<IPenUsbTransport> CreatePenUsbTransportWin32() {
    return std::make_unique<PenUsbTransportWin32>();
}

} // namespace Himax::Pen
