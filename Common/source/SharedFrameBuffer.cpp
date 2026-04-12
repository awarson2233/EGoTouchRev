#include "SharedFrameBuffer.h"
#include "SolverTypes.h"
#include "Logger.h"
#include <algorithm>
#include <cstring>

namespace Ipc {

// ─── SharedFrameWriter (Service side) ───────────────────

bool SharedFrameWriter::Open(const wchar_t* name) {
    m_mapHandle = OpenFileMappingW(FILE_MAP_WRITE, FALSE, name);
    if (!m_mapHandle) {
        // If OpenFileMapping fails, try creating with permissive access
        // (for cross-session Service writes to App-created mapping)
        SECURITY_DESCRIPTOR sd{};
        InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&sd, TRUE, nullptr, FALSE);
        SECURITY_ATTRIBUTES sa{};
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle = FALSE;
        m_mapHandle = OpenFileMappingW(FILE_MAP_WRITE, FALSE, name);
        if (!m_mapHandle) {
            LOG_ERROR("Common", __func__, "IPC", "OpenFileMapping failed: {}",  GetLastError());
            return false;
        }
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
    SECURITY_DESCRIPTOR sd{};
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, nullptr, FALSE);
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;

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
    m_writeIdx = 1;
    LOG_INFO("Common", __func__, "IPC", "Shared memory created for writing ({} bytes, 3 slots).",  sizeof(SharedTripleBuffer));

    // Create frame-ready event (auto-reset)
    m_frameEvent = CreateEventW(&sa, FALSE, FALSE, kFrameReadyEventName);
    if (!m_frameEvent) {
        LOG_WARN("Common", __func__, "IPC", "CreateEvent failed for FrameReadyEvent: {}",  GetLastError());
    }
    return true;
}

void SharedFrameWriter::Write(const Solvers::HeatmapFrame& frame) {
    if (!m_buf) return;

    // Triple-buffer: write to slots[m_writeIdx] — Reader never touches this slot
    SharedFrameData* m_data = &m_buf->slots[m_writeIdx];

    // Copy heatmap
    std::memcpy(m_data->heatmapMatrix, frame.heatmapMatrix,
                sizeof(frame.heatmapMatrix));
    m_data->timestamp = frame.timestamp;

    // Copy zones & peaks
#if EGOTOUCH_DIAG
    std::memcpy(m_data->touchZones, frame.touchZones.data(), sizeof(m_data->touchZones));
    std::memcpy(m_data->peakZones, frame.peakZones.data(), sizeof(m_data->peakZones));
    const int numPeaks = std::min(static_cast<int>(frame.peaks.size()), 30);
    m_data->peakCount = static_cast<uint8_t>(numPeaks);
    for (int i = 0; i < numPeaks; ++i) {
        m_data->peaks[i].r = frame.peaks[i].r;
        m_data->peaks[i].c = frame.peaks[i].c;
        m_data->peaks[i].z = frame.peaks[i].z;
        m_data->peaks[i].id = frame.peaks[i].id;
    }
#else
    std::memset(m_data->touchZones, 0, sizeof(m_data->touchZones));
    std::memset(m_data->peakZones, 0, sizeof(m_data->peakZones));
    m_data->peakCount = 0;
#endif

    // Copy contacts (flatten vector → fixed array)
    const int n = std::min(static_cast<int>(frame.contacts.size()),
                           kMaxSharedContacts);
    m_data->contactCount = static_cast<uint8_t>(n);
    for (int i = 0; i < n; ++i) {
        const auto& src = frame.contacts[i];
        auto& dst = m_data->contacts[i];
        dst.id = src.id; dst.x = src.x; dst.y = src.y;
        dst.state = src.state; dst.area = src.area;
        dst.signalSum = src.signalSum; dst.sizeMm = src.sizeMm;
        dst.isEdge = src.isEdge; dst.isReported = src.isReported;
        dst.prevIndex = src.prevIndex; dst.debugFlags = src.debugFlags;
        dst.lifeFlags = src.lifeFlags; dst.reportFlags = src.reportFlags;
        dst.reportEvent = src.reportEvent;
    }

    // Touch packets
    for (int i = 0; i < 2; ++i) {
        m_data->touchPackets[i].valid = frame.touchPackets[i].valid;
        m_data->touchPackets[i].reportId = frame.touchPackets[i].reportId;
        m_data->touchPackets[i].length = frame.touchPackets[i].length;
        std::memcpy(m_data->touchPackets[i].bytes,
                    frame.touchPackets[i].bytes.data(), 32);
    }

    // Stylus point
    const auto& sp = frame.stylus.point;
    auto& dp = m_data->stylusPoint;
    dp.valid = sp.valid; dp.x = sp.x; dp.y = sp.y;
    dp.reportX = sp.reportX; dp.reportY = sp.reportY;
    dp.pressure = sp.pressure; dp.rawPressure = sp.rawPressure;
    dp.mappedPressure = sp.mappedPressure;
    dp.peakTx1 = sp.peakTx1; dp.peakTx2 = sp.peakTx2;
    dp.tiltValid = sp.tiltValid;
    dp.preTiltX = sp.preTiltX; dp.preTiltY = sp.preTiltY;
    dp.tiltX = sp.tiltX; dp.tiltY = sp.tiltY;
    dp.tiltMagnitude = sp.tiltMagnitude;
    dp.tiltAzimuthDeg = sp.tiltAzimuthDeg;
    dp.tx1X = sp.tx1X; dp.tx1Y = sp.tx1Y;
    dp.tx2X = sp.tx2X; dp.tx2Y = sp.tx2Y;
    dp.confidence = sp.confidence;

    // Stylus packet
    m_data->stylusPacket.valid = frame.stylus.packet.valid;
    m_data->stylusPacket.reportId = frame.stylus.packet.reportId;
    m_data->stylusPacket.length = frame.stylus.packet.length;
    std::memcpy(m_data->stylusPacket.bytes,
                frame.stylus.packet.bytes.data(), 13);

    // Stylus debug fields
    const auto& s = frame.stylus;
    m_data->stylusSlaveValid = s.slaveValid;
    m_data->stylusChecksumOk = s.checksumOk;
    m_data->stylusSlaveOffset = s.slaveWordOffset;
    m_data->stylusChecksum16 = s.checksum16;
    m_data->stylusTx1Valid = s.tx1BlockValid;
    m_data->stylusTx2Valid = s.tx2BlockValid;
    m_data->stylusStatus = s.status;
    m_data->stylusTx1Freq = s.tx1Freq;
    m_data->stylusTx2Freq = s.tx2Freq;
    m_data->stylusPressure = s.pressure;
    m_data->stylusButton = s.button;
    m_data->stylusRawButton = s.rawButton;
    m_data->stylusButtonSource = s.buttonSource;
    m_data->stylusNextTx1Freq = s.nextTx1Freq;
    m_data->stylusNextTx2Freq = s.nextTx2Freq;
    m_data->stylusAsaMode = s.asaMode;
    m_data->stylusDataType = s.dataType;
    m_data->stylusProcessResult = s.processResult;
    m_data->stylusValidJudgment = s.validJudgmentPassed;
    m_data->stylusRecheckEnabled = s.recheckEnabled;
    m_data->stylusRecheckPassed = s.recheckPassed;
    m_data->stylusRecheckOverlap = s.recheckOverlap;
    m_data->stylusRecheckThreshold = s.recheckThreshold;
    m_data->stylusHpp3NoiseInvalid = s.hpp3NoiseInvalid;
    m_data->stylusHpp3NoiseDebounce = s.hpp3NoiseDebounce;
    m_data->stylusHpp3Dim1Valid = s.hpp3Dim1SignalValid;
    m_data->stylusHpp3Dim2Valid = s.hpp3Dim2SignalValid;
    m_data->stylusHpp3WarnX = s.hpp3RatioWarnCountX;
    m_data->stylusHpp3WarnY = s.hpp3RatioWarnCountY;
    m_data->stylusHpp3AvgX = s.hpp3SignalAvgX;
    m_data->stylusHpp3AvgY = s.hpp3SignalAvgY;
    m_data->stylusHpp3Samples = s.hpp3SignalSampleCount;
    m_data->stylusTouchNullLike = s.touchNullLike;
    m_data->stylusTouchSuppressActive = s.touchSuppressActive;
    m_data->stylusTouchSuppressFrames = s.touchSuppressFrames;
    m_data->stylusSignalX = s.signalX;
    m_data->stylusSignalY = s.signalY;
    m_data->stylusMaxRawPeak = s.maxRawPeak;
    m_data->stylusNoPressInk = s.noPressInkActive;
    m_data->stylusPipelineStage = s.pipelineStage;
    // Pipeline diagnostics (single copy)
    m_data->diag = s.diag;

    // Structured suffix data — copy from frame (populated by MasterFrameParser)
    m_data->masterSuffix = frame.masterSuffix;
    m_data->masterSuffixValid = frame.masterSuffixValid;
    m_data->slaveSuffix = frame.slaveSuffix;
    m_data->slaveSuffixValid = frame.slaveSuffixValid;

    // Triple-buffer: publish this slot and advance to next free slot
    //   readyIdx = m_writeIdx  (Reader sees this slot next)
    //   m_writeIdx = next slot that is neither ready nor being read
    const uint32_t justWritten = m_writeIdx;
    m_buf->readyIdx.store(justWritten, std::memory_order_release);
    // Pick next write slot: cycle through 0→1→2→0... skipping readyIdx
    m_writeIdx = (justWritten + 1) % SharedTripleBuffer::kSlotCount;
    // If we landed on readyIdx, skip to next (Reader might be reading readyIdx)
    if (m_writeIdx == m_buf->readyIdx.load(std::memory_order_relaxed)) {
        m_writeIdx = (m_writeIdx + 1) % SharedTripleBuffer::kSlotCount;
    }

    // Increment frame IDs
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
    // Build permissive security descriptor for cross-session access
    SECURITY_DESCRIPTOR sd{};
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, nullptr, FALSE);
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;

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
    m_lastReadId = 0;
    LOG_INFO("Common", __func__, "IPC", "Shared memory opened for reading.");

    // Open frame-ready event
    m_frameEvent = OpenEventW(SYNCHRONIZE, FALSE, kFrameReadyEventName);
    if (!m_frameEvent) {
        LOG_WARN("Common", __func__, "IPC", "OpenEvent failed for FrameReadyEvent: {}",  GetLastError());
    }
    return true;
}

bool SharedFrameReader::Read(Solvers::HeatmapFrame& out) {
    if (!m_buf) return false;

    // Triple-buffer: simply read from slots[readyIdx] — no retry needed
    const uint64_t currentId = m_buf->frameId.load(std::memory_order_acquire);
    if (currentId == m_lastReadId) return false; // no new frame

    const uint32_t idx = m_buf->readyIdx.load(std::memory_order_acquire);
    const SharedFrameData* m_data = &m_buf->slots[idx];

    // Copy heatmap
    std::memcpy(out.heatmapMatrix, m_data->heatmapMatrix,
                sizeof(out.heatmapMatrix));
    out.timestamp = m_data->timestamp;

    // Restore zones & peaks
#if EGOTOUCH_DIAG
    std::memcpy(out.touchZones.data(), m_data->touchZones, sizeof(m_data->touchZones));
    std::memcpy(out.peakZones.data(), m_data->peakZones, sizeof(m_data->peakZones));
    out.peaks.clear();
    for (int i = 0; i < m_data->peakCount; ++i) {
        out.peaks.push_back({m_data->peaks[i].r, m_data->peaks[i].c, m_data->peaks[i].z, m_data->peaks[i].id});
    }
#endif

    // Copy contacts
    out.contacts.resize(m_data->contactCount);
    for (int i = 0; i < m_data->contactCount; ++i) {
        const auto& src = m_data->contacts[i];
        auto& dst = out.contacts[i];
        dst.id = src.id; dst.x = src.x; dst.y = src.y;
        dst.state = src.state; dst.area = src.area;
        dst.signalSum = src.signalSum; dst.sizeMm = src.sizeMm;
        dst.isEdge = src.isEdge; dst.isReported = src.isReported;
        dst.prevIndex = src.prevIndex; dst.debugFlags = src.debugFlags;
        dst.lifeFlags = src.lifeFlags; dst.reportFlags = src.reportFlags;
        dst.reportEvent = src.reportEvent;
    }

    // Touch packets
    for (int i = 0; i < 2; ++i) {
        out.touchPackets[i].valid = m_data->touchPackets[i].valid;
        out.touchPackets[i].reportId = m_data->touchPackets[i].reportId;
        out.touchPackets[i].length = m_data->touchPackets[i].length;
        std::memcpy(out.touchPackets[i].bytes.data(),
                    m_data->touchPackets[i].bytes, 32);
    }

    // Stylus point → StylusFrameData.point
    const auto& spt = m_data->stylusPoint;
    auto& dpt = out.stylus.point;
    dpt.valid = spt.valid; dpt.x = spt.x; dpt.y = spt.y;
    dpt.reportX = spt.reportX; dpt.reportY = spt.reportY;
    dpt.pressure = spt.pressure; dpt.rawPressure = spt.rawPressure;
    dpt.mappedPressure = spt.mappedPressure;
    dpt.peakTx1 = spt.peakTx1; dpt.peakTx2 = spt.peakTx2;
    dpt.tiltValid = spt.tiltValid;
    dpt.preTiltX = spt.preTiltX; dpt.preTiltY = spt.preTiltY;
    dpt.tiltX = spt.tiltX; dpt.tiltY = spt.tiltY;
    dpt.tiltMagnitude = spt.tiltMagnitude;
    dpt.tiltAzimuthDeg = spt.tiltAzimuthDeg;
    dpt.tx1X = spt.tx1X; dpt.tx1Y = spt.tx1Y;
    dpt.tx2X = spt.tx2X; dpt.tx2Y = spt.tx2Y;
    dpt.confidence = spt.confidence;

    // Stylus packet
    out.stylus.packet.valid = m_data->stylusPacket.valid;
    out.stylus.packet.reportId = m_data->stylusPacket.reportId;
    out.stylus.packet.length = m_data->stylusPacket.length;
    std::memcpy(out.stylus.packet.bytes.data(),
                m_data->stylusPacket.bytes, 13);

    // Stylus debug fields
    auto& os = out.stylus;
    os.slaveValid = m_data->stylusSlaveValid;
    os.checksumOk = m_data->stylusChecksumOk;
    os.slaveWordOffset = m_data->stylusSlaveOffset;
    os.checksum16 = m_data->stylusChecksum16;
    os.tx1BlockValid = m_data->stylusTx1Valid;
    os.tx2BlockValid = m_data->stylusTx2Valid;
    os.status = m_data->stylusStatus;
    os.tx1Freq = m_data->stylusTx1Freq;
    os.tx2Freq = m_data->stylusTx2Freq;
    os.pressure = m_data->stylusPressure;
    os.button = m_data->stylusButton;
    os.rawButton = m_data->stylusRawButton;
    os.buttonSource = m_data->stylusButtonSource;
    os.nextTx1Freq = m_data->stylusNextTx1Freq;
    os.nextTx2Freq = m_data->stylusNextTx2Freq;
    os.asaMode = m_data->stylusAsaMode;
    os.dataType = m_data->stylusDataType;
    os.processResult = m_data->stylusProcessResult;
    os.validJudgmentPassed = m_data->stylusValidJudgment;
    os.recheckEnabled = m_data->stylusRecheckEnabled;
    os.recheckPassed = m_data->stylusRecheckPassed;
    os.recheckOverlap = m_data->stylusRecheckOverlap;
    os.recheckThreshold = m_data->stylusRecheckThreshold;
    os.hpp3NoiseInvalid = m_data->stylusHpp3NoiseInvalid;
    os.hpp3NoiseDebounce = m_data->stylusHpp3NoiseDebounce;
    os.hpp3Dim1SignalValid = m_data->stylusHpp3Dim1Valid;
    os.hpp3Dim2SignalValid = m_data->stylusHpp3Dim2Valid;
    os.hpp3RatioWarnCountX = m_data->stylusHpp3WarnX;
    os.hpp3RatioWarnCountY = m_data->stylusHpp3WarnY;
    os.hpp3SignalAvgX = m_data->stylusHpp3AvgX;
    os.hpp3SignalAvgY = m_data->stylusHpp3AvgY;
    os.hpp3SignalSampleCount = m_data->stylusHpp3Samples;
    os.touchNullLike = m_data->stylusTouchNullLike;
    os.touchSuppressActive = m_data->stylusTouchSuppressActive;
    os.touchSuppressFrames = m_data->stylusTouchSuppressFrames;
    os.signalX = m_data->stylusSignalX;
    os.signalY = m_data->stylusSignalY;
    os.maxRawPeak = m_data->stylusMaxRawPeak;
    os.noPressInkActive = m_data->stylusNoPressInk;
    os.pipelineStage = m_data->stylusPipelineStage;
    // Pipeline diagnostics (single copy)
    os.diag = m_data->diag;

    // Structured suffix — copy typed views directly
    out.masterSuffix = m_data->masterSuffix;
    out.masterSuffixValid = m_data->masterSuffixValid;
    out.slaveSuffix = m_data->slaveSuffix;
    out.slaveSuffixValid = m_data->slaveSuffixValid;

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
