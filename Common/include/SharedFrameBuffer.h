#pragma once
// SharedFrameBuffer: Cross-process shared memory for real-time frame push.
// Created by EGoTouchService (writer, Session 0), opened by EGoTouchApp (reader, user session).

#include <atomic>
#include <cstdint>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace Engine { struct HeatmapFrame; }

namespace Ipc {

// Fixed shared memory name
constexpr const wchar_t* kSharedFrameName = L"Global\\EGoTouchSharedFrame";
// Frame-ready event name (Service signals after Write)
constexpr const wchar_t* kFrameReadyEventName = L"Global\\EGoTouchFrameReady";

// Maximum contacts in shared memory
constexpr int kMaxSharedContacts = 10;

// Raw suffix sizes (from hardware frame format)
constexpr int kMasterSuffixBytes = 256;  // 128 words * 2
constexpr int kSlaveSuffixBytes  = 332;  // 166 words * 2
// ───────────────────────────────────────────────────────────
// Flat contact (no STL, fixed layout)
struct SharedContact {
    int    id         = 0;
    float  x          = 0.f;
    float  y          = 0.f;
    int    state      = 0;
    int    area       = 0;
    int    signalSum  = 0;
    float  sizeMm     = 0.f;
    bool   isEdge     = false;
    bool   isReported = true;
    int    prevIndex  = -1;
    int    debugFlags = 0;
    uint32_t lifeFlags   = 0;
    uint32_t reportFlags = 0;
    int    reportEvent   = 0;
};

// Flat touch packet
struct SharedTouchPacket {
    bool    valid    = false;
    uint8_t reportId = 0x01;
    uint8_t length   = 0x20;
    uint8_t bytes[32]{};
};

// Flat stylus solve point
struct SharedStylusSolvePoint {
    bool     valid  = false;
    float    x = 0.f, y = 0.f;
    uint16_t reportX = 0, reportY = 0;
    uint16_t pressure = 0, rawPressure = 0, mappedPressure = 0;
    uint16_t peakTx1 = 0, peakTx2 = 0;
    bool     tiltValid = false;
    int16_t  preTiltX = 0, preTiltY = 0;
    int16_t  tiltX = 0, tiltY = 0;
    float    tiltMagnitude = 0.f, tiltAzimuthDeg = 0.f;
    float    tx1X = 0.f, tx1Y = 0.f, tx2X = 0.f, tx2Y = 0.f;
    float    confidence = 0.f;
};

// Flat stylus packet
struct SharedStylusPacket {
    bool    valid    = false;
    uint8_t reportId = 0x08;
    uint8_t length   = 13;
    uint8_t bytes[13]{};
};

// ───────────────────────────────────────────────────────────
// The actual shared memory layout (all POD, no virtual, no STL)
struct SharedFrameData {
    // Synchronization
    alignas(64) std::atomic<uint64_t> frameId{0};       // write counter (= slave rate, for frame-ready signal)
    alignas(64) std::atomic<uint64_t> slaveFrameId{0};  // slave frame counter (same as frameId)
    alignas(64) std::atomic<uint64_t> masterFrameId{0}; // master frame counter (incremented only when master was read)
    alignas(64) std::atomic<uint32_t> lockFlag{0}; // 0=readable, 1=writing

    // RuntimeSnapshot (flat)
    uint8_t  workerState       = 0;
    bool     streaming         = false;
    int64_t  lastFrameProcessUs = 0;
    int64_t  avgFrameProcessUs  = 0;
    int32_t  acquisitionFps    = 0;  // master FPS (unused by Service; computed by App)
    int32_t  slaveAcquisitionFps = 0; // slave FPS  (unused by Service; computed by App)
    bool     vhfEnabled        = false;
    bool     vhfDeviceOpen     = false;
    bool     vhfTranspose      = false;

    // Heatmap matrix
    int16_t  heatmapMatrix[40][60]{};
    uint64_t timestamp = 0;

    // Touch contacts
    uint8_t contactCount = 0;
    SharedContact contacts[kMaxSharedContacts]{};
    SharedTouchPacket touchPackets[2]{};

    // Stylus
    SharedStylusSolvePoint stylusPoint{};
    SharedStylusPacket     stylusPacket{};
    // Key stylus debug fields (subset of StylusFrameData)
    bool     stylusSlaveValid    = false;
    bool     stylusChecksumOk    = false;
    uint8_t  stylusSlaveOffset   = 0;
    uint16_t stylusChecksum16    = 0;
    bool     stylusTx1Valid      = false;
    bool     stylusTx2Valid      = false;
    uint32_t stylusStatus        = 0;
    uint16_t stylusTx1Freq       = 0;
    uint16_t stylusTx2Freq       = 0;
    uint16_t stylusPressure      = 0;
    uint32_t stylusButton        = 0;
    uint32_t stylusRawButton     = 0;
    uint8_t  stylusButtonSource  = 0;
    uint16_t stylusNextTx1Freq   = 0;
    uint16_t stylusNextTx2Freq   = 0;
    bool     stylusMasterMetaValid = false;
    uint8_t  stylusMasterMetaBase  = 0xFF;
    uint16_t stylusMasterMetaTx1   = 0;
    uint16_t stylusMasterMetaTx2   = 0;
    uint16_t stylusMasterMetaPress = 0;
    uint32_t stylusMasterMetaBtn   = 0;
    uint32_t stylusMasterMetaStat  = 0;
    // ASA/HPP fields
    uint8_t  stylusAsaMode      = 0;
    uint8_t  stylusDataType     = 0;
    uint8_t  stylusProcessResult = 5;
    bool     stylusValidJudgment = false;
    bool     stylusRecheckEnabled = false;
    bool     stylusRecheckPassed  = true;
    bool     stylusRecheckOverlap = false;
    uint16_t stylusRecheckThreshold = 0;
    bool     stylusHpp3NoiseInvalid  = false;
    bool     stylusHpp3NoiseDebounce = false;
    bool     stylusHpp3Dim1Valid = false;
    bool     stylusHpp3Dim2Valid = false;
    uint8_t  stylusHpp3WarnX = 0, stylusHpp3WarnY = 0;
    uint16_t stylusHpp3AvgX  = 0, stylusHpp3AvgY  = 0;
    uint8_t  stylusHpp3Samples = 0;
    bool     stylusTouchNullLike     = false;
    bool     stylusTouchSuppressActive = false;
    uint8_t  stylusTouchSuppressFrames = 0;
    uint16_t stylusSignalX = 0, stylusSignalY = 0;
    uint16_t stylusMaxRawPeak = 0;
    bool     stylusNoPressInk = false;
    uint8_t  stylusPipelineStage = 0;  // 0=ok,1=slaveParse,2=tx1,3=peak,4=coord,5=noise

    // Raw suffix data for Master/Slave status tables
    uint8_t  masterSuffix[kMasterSuffixBytes]{};
    uint8_t  slaveSuffix[kSlaveSuffixBytes]{};
    bool     masterSuffixValid = false;
    bool     slaveSuffixValid  = false;
};

// ───────────────────────────────────────────────────────────
// Writer: used by EGoTouchService to push frame data
class SharedFrameWriter {
public:
    SharedFrameWriter() = default;
    ~SharedFrameWriter() { Close(); }
    SharedFrameWriter(const SharedFrameWriter&) = delete;
    SharedFrameWriter& operator=(const SharedFrameWriter&) = delete;

    bool Open(const wchar_t* name);
    bool Create(const wchar_t* name);   // Service creates Global\ mapping
    void Write(const Engine::HeatmapFrame& frame);
    void Close();
    bool IsOpen() const { return m_data != nullptr; }

private:
    HANDLE          m_mapHandle = nullptr;
    SharedFrameData* m_data     = nullptr;
    HANDLE          m_frameEvent = nullptr;
};

// Reader: used by EGoTouchApp to read frame data
class SharedFrameReader {
public:
    SharedFrameReader() = default;
    ~SharedFrameReader() { Close(); }
    SharedFrameReader(const SharedFrameReader&) = delete;
    SharedFrameReader& operator=(const SharedFrameReader&) = delete;

    bool Create(const wchar_t* name);
    bool Open(const wchar_t* name);     // App opens existing mapping
    bool Read(Engine::HeatmapFrame& out);
    uint64_t LastFrameId() const;
    uint64_t LastSlaveFrameId() const;
    uint64_t LastMasterFrameId() const;
    const SharedFrameData* Raw() const { return m_data; }
    HANDLE FrameReadyEvent() const { return m_frameEvent; }
    void Close();
    bool IsOpen() const { return m_data != nullptr; }

private:
    HANDLE          m_mapHandle = nullptr;
    SharedFrameData* m_data     = nullptr;
    uint64_t        m_lastReadId = 0;
    HANDLE          m_frameEvent = nullptr;
};

} // namespace Ipc
