#include "btmcu/PenUsbTransport.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <span>
#include <utility>
#include <vector>
#include <windows.h>

namespace Himax::Pen {

namespace {

constexpr DWORD kCancelDrainTimeoutMs = 50;
constexpr size_t kMaxPreservedPendingTransfers = 16;

struct EventHandle {
    HANDLE value = nullptr;

    EventHandle() = default;
    explicit EventHandle(HANDLE h) : value(h) {}
    ~EventHandle() { Reset(); }

    EventHandle(const EventHandle&) = delete;
    EventHandle& operator=(const EventHandle&) = delete;

    EventHandle(EventHandle&& other) noexcept : value(other.value) {
        other.value = nullptr;
    }

    EventHandle& operator=(EventHandle&& other) noexcept {
        if (this != &other) {
            Reset();
            value = other.value;
            other.value = nullptr;
        }
        return *this;
    }

    void Reset(HANDLE h = nullptr) noexcept {
        if (value) {
            ::CloseHandle(value);
        }
        value = h;
    }

    bool IsValid() const noexcept { return value != nullptr; }
};

struct PendingTransfer {
    EventHandle event;
    OVERLAPPED ov{};
    std::array<uint8_t, 64> buffer{};
    HANDLE ownerHandle = INVALID_HANDLE_VALUE;
    bool handleClosed = false;

    PendingTransfer() : event(::CreateEventW(nullptr, TRUE, FALSE, nullptr)) {
        ov.hEvent = event.value;
    }

    bool IsValid() const noexcept { return event.IsValid(); }
};

struct PendingWaitResult {
    bool completed = false;
    bool drained = true;
    ChipError error = ChipError::CommunicationError;
};

bool IsOverlappedDrained(HANDLE handle, OVERLAPPED& ov) noexcept {
    DWORD ignored = 0;
    if (::GetOverlappedResult(handle, &ov, &ignored, FALSE)) {
        return true;
    }

    const DWORD err = ::GetLastError();
    if (err == ERROR_OPERATION_ABORTED) {
        return true;
    }
    return err != ERROR_IO_INCOMPLETE;
}

bool CancelAndDrainOverlapped(HANDLE handle, OVERLAPPED& ov) noexcept {
    if (!::CancelIoEx(handle, &ov)) {
        const DWORD cancelErr = ::GetLastError();
        if (cancelErr != ERROR_NOT_FOUND) {
            return IsOverlappedDrained(handle, ov);
        }
    }

    const DWORD waitRes = ::WaitForSingleObject(ov.hEvent, kCancelDrainTimeoutMs);
    if (waitRes == WAIT_OBJECT_0) {
        return IsOverlappedDrained(handle, ov);
    }
    if (waitRes == WAIT_TIMEOUT) {
        return false;
    }

    return IsOverlappedDrained(handle, ov);
}

PendingWaitResult WaitForPendingOverlapped(HANDLE handle,
                                           HANDLE event,
                                           HANDLE cancelEvent,
                                           OVERLAPPED& ov,
                                           DWORD timeoutMs,
                                           DWORD& transferred) noexcept {
    const HANDLE waitHandles[] = {event, cancelEvent};
    const DWORD waitRes = ::WaitForMultipleObjects(2, waitHandles, FALSE, timeoutMs);
    if (waitRes == WAIT_OBJECT_0) {
        if (::GetOverlappedResult(handle, &ov, &transferred, FALSE)) {
            return {.completed = true};
        }

        const DWORD err = ::GetLastError();
        if (err == ERROR_IO_INCOMPLETE) {
            return {.drained = CancelAndDrainOverlapped(handle, ov), .error = ChipError::CommunicationError};
        }
        return {.error = ChipError::CommunicationError};
    }

    if (waitRes == WAIT_TIMEOUT) {
        return {.drained = CancelAndDrainOverlapped(handle, ov), .error = ChipError::Timeout};
    }

    if (waitRes == WAIT_OBJECT_0 + 1) {
        return {.drained = CancelAndDrainOverlapped(handle, ov), .error = ChipError::CommunicationError};
    }

    return {.drained = CancelAndDrainOverlapped(handle, ov), .error = ChipError::CommunicationError};
}

bool IsPreservedTransferComplete(PendingTransfer& transfer) noexcept {
    if (!transfer.handleClosed && transfer.ownerHandle != INVALID_HANDLE_VALUE) {
        return IsOverlappedDrained(transfer.ownerHandle, transfer.ov);
    }

    // After CloseHandle(ownerHandle), GetOverlappedResult can no longer be used safely.
    // The per-transfer event is still owned by the transfer and becomes signaled when the
    // kernel has finished touching the OVERLAPPED and buffer storage.
    return ::WaitForSingleObject(transfer.event.value, 0) == WAIT_OBJECT_0;
}

using PendingTransferList = std::vector<std::unique_ptr<PendingTransfer>>;

std::atomic_size_t& OutstandingTransferSlots() noexcept {
    static std::atomic_size_t slots{0};
    return slots;
}

bool TryReserveTransferSlot() noexcept {
    auto& slots = OutstandingTransferSlots();
    size_t observed = slots.load(std::memory_order_relaxed);
    while (observed < kMaxPreservedPendingTransfers) {
        if (slots.compare_exchange_weak(observed,
                                        observed + 1,
                                        std::memory_order_acq_rel,
                                        std::memory_order_relaxed)) {
            return true;
        }
    }
    return false;
}

void ReleaseTransferSlot() noexcept {
    auto& slots = OutstandingTransferSlots();
    size_t observed = slots.load(std::memory_order_relaxed);
    while (observed > 0) {
        if (slots.compare_exchange_weak(observed,
                                        observed - 1,
                                        std::memory_order_acq_rel,
                                        std::memory_order_relaxed)) {
            return;
        }
    }
}

std::mutex& QuarantinedTransferMutex() noexcept {
    static std::mutex mutex;
    return mutex;
}

PendingTransferList& QuarantinedTransfers() noexcept {
    static PendingTransferList transfers;
    return transfers;
}

void ReapTransferList(PendingTransferList& transfers) noexcept {
    for (auto it = transfers.begin(); it != transfers.end();) {
        if (IsPreservedTransferComplete(**it)) {
            it = transfers.erase(it);
            ReleaseTransferSlot();
        } else {
            ++it;
        }
    }
}

void ReapQuarantinedTransfers() noexcept {
    std::lock_guard<std::mutex> lk(QuarantinedTransferMutex());
    ReapTransferList(QuarantinedTransfers());
}

void QuarantinePreservedTransfers(PendingTransferList& transfers) noexcept {
    std::lock_guard<std::mutex> lk(QuarantinedTransferMutex());
    auto& quarantined = QuarantinedTransfers();
    ReapTransferList(quarantined);
    ReapTransferList(transfers);
    for (auto it = transfers.begin(); it != transfers.end();) {
        quarantined.push_back(std::move(*it));
        it = transfers.erase(it);
    }
}

} // namespace

class PenUsbTransportWin32 final : public IPenUsbTransport {
public:
    PenUsbTransportWin32() = default;
    ~PenUsbTransportWin32() override {
        Close();
        QuarantinePreservedPendingTransfers();
    }

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

        m_cancelEvent.Reset(::CreateEventW(nullptr, TRUE, FALSE, nullptr));
        if (!m_cancelEvent.IsValid()) {
            ::CloseHandle(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
            return std::unexpected(ChipError::CommunicationError);
        }

        m_closing = false;
        return {};
    }

    void Close() override {
        std::unique_lock<std::mutex> lk(m_handleMu);
        ReapPreservedPendingTransfersLocked();
        if (m_handle == INVALID_HANDLE_VALUE) {
            m_closing = false;
            return;
        }

        m_closing = true;
        const HANDLE handle = m_handle;
        if (m_cancelEvent.IsValid()) {
            ::SetEvent(m_cancelEvent.value);
        }
        ::CancelIoEx(handle, nullptr);
        m_idleCv.wait(lk, [this]() { return m_activeIo == 0; });

        DrainPreservedPendingTransfersLocked(handle);
        ::CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
        MarkPreservedTransfersHandleClosedLocked(handle);
        m_cancelEvent.Reset();
        m_closing = false;
    }

    void CancelIo() override {
        std::lock_guard<std::mutex> lk(m_handleMu);
        if (m_cancelEvent.IsValid()) {
            ::SetEvent(m_cancelEvent.value);
        }
        if (m_handle != INVALID_HANDLE_VALUE) {
            ::CancelIoEx(m_handle, nullptr);
        }
    }

    bool IsOpen() const override {
        std::lock_guard<std::mutex> lk(m_handleMu);
        return m_handle != INVALID_HANDLE_VALUE;
    }

    ChipResult<> ReadPacket(std::vector<uint8_t>& outBytes, uint32_t timeoutMs) override {
        auto transfer = std::make_unique<PendingTransfer>();
        if (!transfer->IsValid()) {
            return std::unexpected(ChipError::CommunicationError);
        }

        DWORD bytesRead = 0;
        IoLease lease(*this);
        if (!lease.IsValid()) {
            return std::unexpected(ChipError::InvalidOperation);
        }

        BOOL ok = ::ReadFile(lease.Handle(),
                             transfer->buffer.data(),
                             static_cast<DWORD>(transfer->buffer.size()),
                             &bytesRead,
                             &transfer->ov);
        if (!ok) {
            const DWORD err = ::GetLastError();
            if (err != ERROR_IO_PENDING) {
                return std::unexpected(ChipError::CommunicationError);
            }

            const PendingWaitResult waitResult = WaitForPendingOverlapped(lease.Handle(),
                                                                          transfer->event.value,
                                                                          lease.CancelEvent(),
                                                                          transfer->ov,
                                                                          timeoutMs,
                                                                          bytesRead);
            if (!waitResult.completed) {
                if (!waitResult.drained) {
                    lease.KeepSlotForPendingLifetime();
                    PreservePendingTransferLifetime(std::move(transfer), lease.Handle());
                }
                return std::unexpected(waitResult.error);
            }
        }

        if (bytesRead == 0) {
            return std::unexpected(ChipError::Timeout);
        }

        outBytes.assign(transfer->buffer.begin(), transfer->buffer.begin() + static_cast<std::ptrdiff_t>(bytesRead));
        return {};
    }

    ChipResult<> WritePacket(std::span<const uint8_t> bytes) override {
        if (bytes.empty() || bytes.size() > 64) {
            return std::unexpected(ChipError::InvalidOperation);
        }

        auto transfer = std::make_unique<PendingTransfer>();
        if (!transfer->IsValid()) {
            return std::unexpected(ChipError::CommunicationError);
        }

        std::copy(bytes.begin(), bytes.end(), transfer->buffer.begin());
        DWORD bytesWritten = 0;
        IoLease lease(*this);
        if (!lease.IsValid()) {
            return std::unexpected(ChipError::InvalidOperation);
        }

        BOOL ok = ::WriteFile(lease.Handle(),
                              transfer->buffer.data(),
                              static_cast<DWORD>(bytes.size()),
                              &bytesWritten,
                              &transfer->ov);
        if (!ok) {
            const DWORD err = ::GetLastError();
            if (err != ERROR_IO_PENDING) {
                return std::unexpected(ChipError::CommunicationError);
            }

            const PendingWaitResult waitResult = WaitForPendingOverlapped(lease.Handle(),
                                                                          transfer->event.value,
                                                                          lease.CancelEvent(),
                                                                          transfer->ov,
                                                                          2000,
                                                                          bytesWritten);
            if (!waitResult.completed) {
                if (!waitResult.drained) {
                    lease.KeepSlotForPendingLifetime();
                    PreservePendingTransferLifetime(std::move(transfer), lease.Handle());
                }
                return std::unexpected(waitResult.error);
            }
        }

        if (bytesWritten != bytes.size()) {
            return std::unexpected(ChipError::CommunicationError);
        }
        return {};
    }

private:
    void ReapPreservedPendingTransfersLocked() noexcept {
        ReapTransferList(m_preservedPendingTransfers);
    }

    void QuarantinePreservedPendingTransfers() noexcept {
        std::lock_guard<std::mutex> lk(m_handleMu);
        QuarantinePreservedTransfers(m_preservedPendingTransfers);
    }

    void DrainPreservedPendingTransfersLocked(HANDLE handle) noexcept {
        for (auto it = m_preservedPendingTransfers.begin(); it != m_preservedPendingTransfers.end();) {
            PendingTransfer& transfer = **it;
            if (!transfer.handleClosed && transfer.ownerHandle == handle && CancelAndDrainOverlapped(handle, transfer.ov)) {
                it = m_preservedPendingTransfers.erase(it);
                ReleaseTransferSlot();
            } else if (IsPreservedTransferComplete(transfer)) {
                it = m_preservedPendingTransfers.erase(it);
                ReleaseTransferSlot();
            } else {
                ++it;
            }
        }
    }

    void MarkPreservedTransfersHandleClosedLocked(HANDLE handle) noexcept {
        for (auto& transfer : m_preservedPendingTransfers) {
            if (transfer->ownerHandle == handle) {
                transfer->handleClosed = true;
                transfer->ownerHandle = INVALID_HANDLE_VALUE;
            }
        }
    }

    void PreservePendingTransferLifetime(std::unique_ptr<PendingTransfer> transfer, HANDLE handle) noexcept {
        if (!transfer) {
            return;
        }

        transfer->ownerHandle = handle;
        transfer->handleClosed = false;

        std::lock_guard<std::mutex> lk(m_handleMu);
        ReapPreservedPendingTransfersLocked();
        // IoLease already reserved the process-wide lifetime slot. Moving the
        // transfer from active I/O into preserved ownership does not consume a
        // new slot; the slot is released when the transfer is reaped.
        m_preservedPendingTransfers.push_back(std::move(transfer));
    }

    class IoLease {
    public:
        explicit IoLease(PenUsbTransportWin32& owner) : m_owner(&owner) {
            std::lock_guard<std::mutex> lk(owner.m_handleMu);
            owner.ReapPreservedPendingTransfersLocked();
            ReapQuarantinedTransfers();
            if (owner.m_handle == INVALID_HANDLE_VALUE || owner.m_closing || !TryReserveTransferSlot()) {
                return;
            }
            if (owner.m_activeIo == 0 && owner.m_cancelEvent.IsValid()) {
                ::ResetEvent(owner.m_cancelEvent.value);
            }
            m_handle = owner.m_handle;
            m_cancelEvent = owner.m_cancelEvent.value;
            ++owner.m_activeIo;
            m_releaseSlotOnDestroy = true;
            m_valid = true;
        }

        ~IoLease() {
            if (!m_valid || !m_owner) {
                return;
            }
            {
                std::lock_guard<std::mutex> lk(m_owner->m_handleMu);
                if (m_owner->m_activeIo > 0) {
                    --m_owner->m_activeIo;
                }
                m_owner->m_idleCv.notify_all();
            }
            if (m_releaseSlotOnDestroy) {
                ReleaseTransferSlot();
            }
        }

        IoLease(const IoLease&) = delete;
        IoLease& operator=(const IoLease&) = delete;

        bool IsValid() const noexcept { return m_valid; }
        HANDLE Handle() const noexcept { return m_handle; }
        HANDLE CancelEvent() const noexcept { return m_cancelEvent; }
        void KeepSlotForPendingLifetime() noexcept { m_releaseSlotOnDestroy = false; }

    private:
        PenUsbTransportWin32* m_owner = nullptr;
        HANDLE m_handle = INVALID_HANDLE_VALUE;
        HANDLE m_cancelEvent = nullptr;
        bool m_valid = false;
        bool m_releaseSlotOnDestroy = false;
    };

    mutable std::mutex m_handleMu;
    std::condition_variable m_idleCv;
    HANDLE m_handle = INVALID_HANDLE_VALUE;
    EventHandle m_cancelEvent;
    std::vector<std::unique_ptr<PendingTransfer>> m_preservedPendingTransfers;
    uint32_t m_activeIo = 0;
    bool m_closing = false;
};

std::unique_ptr<IPenUsbTransport> CreatePenUsbTransportWin32() {
    return std::make_unique<PenUsbTransportWin32>();
}

} // namespace Himax::Pen
