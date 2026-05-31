#include "Ipc/SharedFrameBuffer.h"
#include "Ipc/IpcSecurity.h"
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

constexpr bool IsDirtySlotSequence(uint64_t sequence) noexcept {
    return (sequence & 1ull) != 0;
}

uint64_t NextDirtySlotSequence(uint64_t sequence) noexcept {
    return IsDirtySlotSequence(sequence) ? sequence : sequence + 1;
}

uint64_t NextCleanSlotSequence(uint64_t dirtySequence) noexcept {
    return IsDirtySlotSequence(dirtySequence) ? dirtySequence + 1 : dirtySequence;
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

    const uint32_t justWritten = m_writeIdx;
    const uint64_t nextFrameId = m_buf->frameId.load(std::memory_order_relaxed) + 1;
    const uint64_t nextSlaveFrameId = m_buf->slaveFrameId.load(std::memory_order_relaxed) + 1;
    const uint64_t nextMasterFrameId = m_buf->masterFrameId.load(std::memory_order_relaxed) + (frame.masterWasRead ? 1 : 0);

    const uint64_t dirtySequence = NextDirtySlotSequence(
        m_buf->slotSequences[justWritten].load(std::memory_order_relaxed));
    m_buf->slotSequences[justWritten].store(dirtySequence, std::memory_order_release);

    SharedFrameData* m_data = &m_buf->slots[justWritten];
    CopySharedFrameData(*m_data, frame);

    m_buf->slotFrameIds[justWritten].store(nextFrameId, std::memory_order_release);
    m_buf->slotSequences[justWritten].store(NextCleanSlotSequence(dirtySequence), std::memory_order_release);
    m_buf->readyIdx.store(justWritten, std::memory_order_release);
    m_buf->slaveFrameId.store(nextSlaveFrameId, std::memory_order_release);
    m_buf->masterFrameId.store(nextMasterFrameId, std::memory_order_release);
    m_buf->frameId.store(nextFrameId, std::memory_order_release);

    m_writeIdx = (justWritten + 1) % SharedTripleBuffer::kSlotCount;
    if (m_writeIdx == m_buf->readyIdx.load(std::memory_order_relaxed)) {
        m_writeIdx = (m_writeIdx + 1) % SharedTripleBuffer::kSlotCount;
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

    for (int attempt = 0; attempt < 2; ++attempt) {
        const uint64_t currentId = m_buf->frameId.load(std::memory_order_acquire);
        if (currentId == m_lastReadId) return false;

        const uint32_t idx = m_buf->readyIdx.load(std::memory_order_acquire);
        if (idx >= SharedTripleBuffer::kSlotCount) return false;

        const uint64_t sequenceBefore = m_buf->slotSequences[idx].load(std::memory_order_acquire);
        if (IsDirtySlotSequence(sequenceBefore)) continue;

        const uint64_t slotIdBefore = m_buf->slotFrameIds[idx].load(std::memory_order_acquire);
        if (slotIdBefore != currentId) continue;

        CopySharedFrameData(out, m_buf->slots[idx]);

        const uint64_t sequenceAfter = m_buf->slotSequences[idx].load(std::memory_order_acquire);
        const uint64_t slotIdAfter = m_buf->slotFrameIds[idx].load(std::memory_order_acquire);
        if (sequenceAfter != sequenceBefore || IsDirtySlotSequence(sequenceAfter) || slotIdAfter != currentId) continue;

        m_lastReadId = currentId;
        return true;
    }
    return false;
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
