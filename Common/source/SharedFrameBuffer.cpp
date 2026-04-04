#include "SharedFrameBuffer.h"
#include "EngineTypes.h"
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
    m_data = static_cast<SharedFrameData*>(
        MapViewOfFile(m_mapHandle, FILE_MAP_WRITE, 0, 0,
                      sizeof(SharedFrameData)));
    if (!m_data) {
        LOG_ERROR("Common", __func__, "IPC", "MapViewOfFile failed: {}",  GetLastError());
        CloseHandle(m_mapHandle);
        m_mapHandle = nullptr;
        return false;
    }
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
        0, sizeof(SharedFrameData), name);
    if (!m_mapHandle) {
        LOG_ERROR("Common", __func__, "IPC", "CreateFileMapping failed: {}",  GetLastError());
        return false;
    }
    m_data = static_cast<SharedFrameData*>(
        MapViewOfFile(m_mapHandle, FILE_MAP_WRITE, 0, 0,
                      sizeof(SharedFrameData)));
    if (!m_data) {
        LOG_ERROR("Common", __func__, "IPC", "MapViewOfFile failed: {}",  GetLastError());
        CloseHandle(m_mapHandle);
        m_mapHandle = nullptr;
        return false;
    }
    std::memset(m_data, 0, sizeof(SharedFrameData));
    LOG_INFO("Common", __func__, "IPC", "Shared memory created for writing ({} bytes).",  sizeof(SharedFrameData));

    // Create frame-ready event (auto-reset)
    m_frameEvent = CreateEventW(&sa, FALSE, FALSE, kFrameReadyEventName);
    if (!m_frameEvent) {
        LOG_WARN("Common", __func__, "IPC", "CreateEvent failed for FrameReadyEvent: {}",  GetLastError());
    }
    return true;
}

void SharedFrameWriter::Write(const Engine::HeatmapFrame& frame) {
    if (!m_data) return;

    // Signal: writing in progress
    m_data->lockFlag.store(1, std::memory_order_release);
    // Copy heatmap
    std::memcpy(m_data->heatmapMatrix, frame.heatmapMatrix,
                sizeof(frame.heatmapMatrix));
    m_data->timestamp = frame.timestamp;

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
    m_data->stylusMasterMetaValid = s.masterMetaValid;
    m_data->stylusMasterMetaBase = s.masterMetaBaseWord;
    m_data->stylusMasterMetaTx1 = s.masterMetaTx1Freq;
    m_data->stylusMasterMetaTx2 = s.masterMetaTx2Freq;
    m_data->stylusMasterMetaPress = s.masterMetaPressure;
    m_data->stylusMasterMetaBtn = s.masterMetaButton;
    m_data->stylusMasterMetaStat = s.masterMetaStatus;
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

    // Raw suffix data from rawData
    if (frame.rawData.size() >= 5063) {
        std::memcpy(m_data->masterSuffix,
                    frame.rawData.data() + 4807, kMasterSuffixBytes);
        m_data->masterSuffixValid = true;
    } else {
        m_data->masterSuffixValid = false;
    }
    if (frame.rawData.size() >= 5402) {
        std::memcpy(m_data->slaveSuffix,
                    frame.rawData.data() + 5070, kSlaveSuffixBytes);
        m_data->slaveSuffixValid = true;
    } else {
        m_data->slaveSuffixValid = false;
    }

    // Increment frame IDs: slave always increments; master only when not skipped by 2:1 interleaving
    m_data->frameId.fetch_add(1, std::memory_order_relaxed);
    m_data->slaveFrameId.fetch_add(1, std::memory_order_relaxed);
    if (frame.masterWasRead) {
        m_data->masterFrameId.fetch_add(1, std::memory_order_relaxed);
    }
    m_data->lockFlag.store(0, std::memory_order_release);

    if (m_frameEvent) {
        SetEvent(m_frameEvent);
    }
}

void SharedFrameWriter::Close() {
    if (m_data) {
        UnmapViewOfFile(m_data);
        m_data = nullptr;
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
        0, sizeof(SharedFrameData), name);
    if (!m_mapHandle) {
        LOG_ERROR("Common", __func__, "IPC", "CreateFileMapping failed: {}",  GetLastError());
        return false;
    }
    m_data = static_cast<SharedFrameData*>(
        MapViewOfFile(m_mapHandle, FILE_MAP_ALL_ACCESS, 0, 0,
                      sizeof(SharedFrameData)));
    if (!m_data) {
        LOG_ERROR("Common", __func__, "IPC", "MapViewOfFile failed: {}",  GetLastError());
        CloseHandle(m_mapHandle);
        m_mapHandle = nullptr;
        return false;
    }
    // Zero-initialize
    std::memset(m_data, 0, sizeof(SharedFrameData));
    LOG_INFO("Common", __func__, "IPC", "Shared memory created ({} bytes).",  sizeof(SharedFrameData));

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
    m_data = static_cast<SharedFrameData*>(
        MapViewOfFile(m_mapHandle, FILE_MAP_READ, 0, 0,
                      sizeof(SharedFrameData)));
    if (!m_data) {
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

bool SharedFrameReader::Read(Engine::HeatmapFrame& out) {
    if (!m_data) return false;

    // Spin-wait if writer is active (very brief)
    int spins = 0;
    while (m_data->lockFlag.load(std::memory_order_acquire) != 0) {
        if (++spins > 10000) return false; // timeout
    }

    const uint64_t currentId = m_data->frameId.load(
        std::memory_order_acquire);
    if (currentId == m_lastReadId) return false; // no new frame

    // Copy heatmap
    std::memcpy(out.heatmapMatrix, m_data->heatmapMatrix,
                sizeof(out.heatmapMatrix));
    out.timestamp = m_data->timestamp;

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
    const auto& sp = m_data->stylusPoint;
    auto& dp = out.stylus.point;
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
    os.masterMetaValid = m_data->stylusMasterMetaValid;
    os.masterMetaBaseWord = m_data->stylusMasterMetaBase;
    os.masterMetaTx1Freq = m_data->stylusMasterMetaTx1;
    os.masterMetaTx2Freq = m_data->stylusMasterMetaTx2;
    os.masterMetaPressure = m_data->stylusMasterMetaPress;
    os.masterMetaButton = m_data->stylusMasterMetaBtn;
    os.masterMetaStatus = m_data->stylusMasterMetaStat;
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

    // Reconstruct rawData suffix for DrawMasterSuffixTable/DrawSlaveSuffixTable
    // We need rawData[4807..5062] for master and [5070..5401] for slave
    if (m_data->masterSuffixValid || m_data->slaveSuffixValid) {
        // Pre-allocate rawData once (avoid per-frame heap alloc)
        if (out.rawData.capacity() < 5402) out.rawData.reserve(5402);
        if (out.rawData.size() < 5402) out.rawData.resize(5402, 0);
        if (m_data->masterSuffixValid) {
            std::memcpy(out.rawData.data() + 4807,
                        m_data->masterSuffix, kMasterSuffixBytes);
        }
        if (m_data->slaveSuffixValid) {
            std::memcpy(out.rawData.data() + 5070,
                        m_data->slaveSuffix, kSlaveSuffixBytes);
        }
    }

    m_lastReadId = currentId;
    return true;
}

uint64_t SharedFrameReader::LastFrameId() const {
    if (!m_data) return 0;
    return m_data->frameId.load(std::memory_order_acquire);
}

uint64_t SharedFrameReader::LastSlaveFrameId() const {
    if (!m_data) return 0;
    return m_data->slaveFrameId.load(std::memory_order_acquire);
}

uint64_t SharedFrameReader::LastMasterFrameId() const {
    if (!m_data) return 0;
    return m_data->masterFrameId.load(std::memory_order_acquire);
}

void SharedFrameReader::Close() {
    if (m_data) {
        UnmapViewOfFile(m_data);
        m_data = nullptr;
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
