#include "SharedFrameBuffer.h"
#include "IpcSecurity.h"
#include "Logger.h"
#include <cstring>

namespace Ipc {

namespace {

void InitializeAbiHeader(SharedTripleBuffer& buffer) noexcept {
    buffer.abi.abiVersion = kSharedFrameAbiVersion;
    buffer.abi.totalSize = sizeof(SharedTripleBuffer);
    buffer.abi.headerSize = sizeof(SharedFrameAbiHeader);
    buffer.abi.capabilities = kSharedFrameAbiCapabilities;
    buffer.abi.slotCount = SharedTripleBuffer::kSlotCount;
    buffer.abi.reserved = kSharedFrameAbiReserved;
}

bool HasCompatibleAbi(const SharedTripleBuffer& buffer) noexcept {
    return buffer.abi.abiVersion == kSharedFrameAbiVersion &&
           buffer.abi.totalSize == sizeof(SharedTripleBuffer) &&
           buffer.abi.headerSize == sizeof(SharedFrameAbiHeader) &&
           buffer.abi.slotCount == SharedTripleBuffer::kSlotCount;
}

} // namespace

// ─── SharedFrameWriter (Service side) ───────────────────

bool SharedFrameWriter::Open(const wchar_t* name) {
    m_mapHandle = OpenFileMappingW(FILE_MAP_WRITE, FALSE, name);
    if (!m_mapHandle) {
        LOG_ERROR("Common", __func__, "IPC", "OpenFileMapping failed: {}",  GetLastError());
        return false;
    }
    m_buf = static_cast<SharedTripleBuffer*>(
        MapViewOfFile(m_mapHandle, FILE_MAP_WRITE, 0, 0,
                      sizeof(SharedTripleBuffer)));
    if (!m_buf) {
        LOG_ERROR("Common", __func__, "IPC", "MapViewOfFile failed: {}",  GetLastError());
        CloseHandle(m_mapHandle);
        m_mapHandle = nullptr;
        return false;
    }
    if (!HasCompatibleAbi(*m_buf)) {
        LOG_ERROR("Common", __func__, "IPC", "Shared memory ABI mismatch: version={} totalSize={} headerSize={} slotCount={}",
                  m_buf->abi.abiVersion, m_buf->abi.totalSize, m_buf->abi.headerSize, m_buf->abi.slotCount);
        UnmapViewOfFile(m_buf);
        m_buf = nullptr;
        CloseHandle(m_mapHandle);
        m_mapHandle = nullptr;
        return false;
    }
    m_writeIdx = 1;  // Start writing to slot 1 (slot 0 is initial readyIdx)
    LOG_INFO("Common", __func__, "IPC", "Shared memory opened for writing.");

    // Open frame-ready event (optional)
    m_frameEvent = OpenEventW(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE,
                              kFrameReadyEventName);
    if (!m_frameEvent) {
        LOG_WARN("Common", __func__, "IPC", "OpenEvent failed for FrameReadyEvent: {}",  GetLastError());
    }
    return true;
}

bool SharedFrameWriter::Create(const wchar_t* name) {
    SECURITY_ATTRIBUTES sa{};
    ScopedSecurityDescriptor sd;
    if (!BuildAdminOnlySecurityAttributes(sa, sd)) {
        LOG_ERROR("Common", __func__, "IPC", "Build mapping security descriptor failed: {}",  GetLastError());
        return false;
    }

    m_mapHandle = CreateFileMappingW(
        INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE,
        0, sizeof(SharedTripleBuffer), name);
    if (!m_mapHandle) {
        LOG_ERROR("Common", __func__, "IPC", "CreateFileMapping failed: {}",  GetLastError());
        return false;
    }
    m_buf = static_cast<SharedTripleBuffer*>(
        MapViewOfFile(m_mapHandle, FILE_MAP_WRITE, 0, 0,
                      sizeof(SharedTripleBuffer)));
    if (!m_buf) {
        LOG_ERROR("Common", __func__, "IPC", "MapViewOfFile failed: {}",  GetLastError());
        CloseHandle(m_mapHandle);
        m_mapHandle = nullptr;
        return false;
    }
    std::memset(m_buf, 0, sizeof(SharedTripleBuffer));
    InitializeAbiHeader(*m_buf);
    m_writeIdx = 1;
    LOG_INFO("Common", __func__, "IPC", "Shared memory created for writing ({} bytes, 3 slots).",  sizeof(SharedTripleBuffer));

    // Create frame-ready event (auto-reset)
    m_frameEvent = CreateEventW(&sa, FALSE, FALSE, kFrameReadyEventName);
    if (!m_frameEvent) {
        LOG_WARN("Common", __func__, "IPC", "CreateEvent failed for FrameReadyEvent: {}",  GetLastError());
    }
    return true;
}

void SharedFrameWriter::Write(const SharedFrameData& frame) {
    if (!m_buf) return;

    SharedFrameData* m_data = &m_buf->slots[m_writeIdx];
    CopySharedFrameData(*m_data, frame);

    const uint32_t justWritten = m_writeIdx;
    m_buf->readyIdx.store(justWritten, std::memory_order_release);
    m_writeIdx = (justWritten + 1) % SharedTripleBuffer::kSlotCount;
    if (m_writeIdx == m_buf->readyIdx.load(std::memory_order_relaxed)) {
        m_writeIdx = (m_writeIdx + 1) % SharedTripleBuffer::kSlotCount;
    }

    m_buf->frameId.fetch_add(1, std::memory_order_relaxed);
    m_buf->slaveFrameId.fetch_add(1, std::memory_order_relaxed);
    if (frame.masterWasRead) {
        m_buf->masterFrameId.fetch_add(1, std::memory_order_relaxed);
    }

    if (m_frameEvent) {
        SetEvent(m_frameEvent);
    }
}

void SharedFrameWriter::Close() {
    if (m_buf) {
        UnmapViewOfFile(m_buf);
        m_buf = nullptr;
    }
    if (m_mapHandle) {
        CloseHandle(m_mapHandle);
        m_mapHandle = nullptr;
    }
    if (m_frameEvent) {
        CloseHandle(m_frameEvent);
        m_frameEvent = nullptr;
    }
}

// ─── SharedFrameReader (App side) ───────────────────────

bool SharedFrameReader::Create(const wchar_t* name) {
    SECURITY_ATTRIBUTES sa{};
    ScopedSecurityDescriptor sd;
    if (!BuildAdminOnlySecurityAttributes(sa, sd)) {
        LOG_ERROR("Common", __func__, "IPC", "Build mapping security descriptor failed: {}",  GetLastError());
        return false;
    }

    m_mapHandle = CreateFileMappingW(
        INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE,
        0, sizeof(SharedTripleBuffer), name);
    if (!m_mapHandle) {
        LOG_ERROR("Common", __func__, "IPC", "CreateFileMapping failed: {}",  GetLastError());
        return false;
    }
    m_buf = static_cast<SharedTripleBuffer*>(
        MapViewOfFile(m_mapHandle, FILE_MAP_ALL_ACCESS, 0, 0,
                      sizeof(SharedTripleBuffer)));
    if (!m_buf) {
        LOG_ERROR("Common", __func__, "IPC", "MapViewOfFile failed: {}",  GetLastError());
        CloseHandle(m_mapHandle);
        m_mapHandle = nullptr;
        return false;
    }
    // Zero-initialize
    std::memset(m_buf, 0, sizeof(SharedTripleBuffer));
    InitializeAbiHeader(*m_buf);
    LOG_INFO("Common", __func__, "IPC", "Shared memory created ({} bytes, 3 slots).",  sizeof(SharedTripleBuffer));

    // Create frame-ready event (auto-reset)
    m_frameEvent = CreateEventW(&sa, FALSE, FALSE, kFrameReadyEventName);
    if (!m_frameEvent) {
        LOG_WARN("Common", __func__, "IPC", "CreateEvent failed for FrameReadyEvent: {}",  GetLastError());
    }
    return true;
}

bool SharedFrameReader::Open(const wchar_t* name) {
    m_mapHandle = OpenFileMappingW(FILE_MAP_READ, FALSE, name);
    if (!m_mapHandle) {
        LOG_ERROR("Common", __func__, "IPC", "OpenFileMapping failed: {}",  GetLastError());
        return false;
    }
    m_buf = static_cast<SharedTripleBuffer*>(
        MapViewOfFile(m_mapHandle, FILE_MAP_READ, 0, 0,
                      sizeof(SharedTripleBuffer)));
    if (!m_buf) {
        LOG_ERROR("Common", __func__, "IPC", "MapViewOfFile failed: {}",  GetLastError());
        CloseHandle(m_mapHandle);
        m_mapHandle = nullptr;
        return false;
    }
    if (!HasCompatibleAbi(*m_buf)) {
        LOG_ERROR("Common", __func__, "IPC", "Shared memory ABI mismatch: version={} totalSize={} headerSize={} slotCount={}",
                  m_buf->abi.abiVersion, m_buf->abi.totalSize, m_buf->abi.headerSize, m_buf->abi.slotCount);
        UnmapViewOfFile(m_buf);
        m_buf = nullptr;
        CloseHandle(m_mapHandle);
        m_mapHandle = nullptr;
        return false;
    }
    m_lastReadId = 0;
    LOG_INFO("Common", __func__, "IPC", "Shared memory opened for reading.");

    // Open frame-ready event
    m_frameEvent = OpenEventW(SYNCHRONIZE, FALSE, kFrameReadyEventName);
    if (!m_frameEvent) {
        LOG_WARN("Common", __func__, "IPC", "OpenEvent failed for FrameReadyEvent: {}",  GetLastError());
    }
    return true;
}

bool SharedFrameReader::Read(SharedFrameData& out) {
    if (!m_buf) return false;

    const uint64_t currentId = m_buf->frameId.load(std::memory_order_acquire);
    if (currentId == m_lastReadId) return false;

    const uint32_t idx = m_buf->readyIdx.load(std::memory_order_acquire);
    CopySharedFrameData(out, m_buf->slots[idx]);

    m_lastReadId = currentId;
    return true;
}

uint64_t SharedFrameReader::LastFrameId() const {
    if (!m_buf) return 0;
    return m_buf->frameId.load(std::memory_order_acquire);
}

uint64_t SharedFrameReader::LastSlaveFrameId() const {
    if (!m_buf) return 0;
    return m_buf->slaveFrameId.load(std::memory_order_acquire);
}

uint64_t SharedFrameReader::LastMasterFrameId() const {
    if (!m_buf) return 0;
    return m_buf->masterFrameId.load(std::memory_order_acquire);
}

void SharedFrameReader::Close() {
    if (m_buf) {
        UnmapViewOfFile(m_buf);
        m_buf = nullptr;
    }
    if (m_mapHandle) {
        CloseHandle(m_mapHandle);
        m_mapHandle = nullptr;
    }
    if (m_frameEvent) {
        CloseHandle(m_frameEvent);
        m_frameEvent = nullptr;
    }
}

} // namespace Ipc
