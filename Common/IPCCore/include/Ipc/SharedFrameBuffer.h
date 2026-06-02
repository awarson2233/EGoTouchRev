#pragma once
// SharedFrameBuffer: Cross-process shared memory for real-time frame push.
// Created by EGoTouchService (writer, Session 0), opened by EGoTouchApp (reader, user session).

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "FrameLayout.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace Ipc {

// Fixed shared memory name
constexpr const wchar_t* kSharedFrameName = L"Global\\EGoTouchSharedFrame";
// Frame-ready event name (Service signals after Write)
constexpr const wchar_t* kFrameReadyEventName = L"Global\\EGoTouchFrameReady";

// Stable shared-memory ABI contract for Phase 0.
constexpr uint32_t kSharedFrameAbiVersion = 5;
constexpr uint32_t kSharedFrameAbiCapabilities = 0;
constexpr uint32_t kSharedFrameAbiReserved = 0;

struct SharedFrameAbiHeader {
    uint32_t abiVersion = kSharedFrameAbiVersion;
    uint32_t totalSize = 0;
    uint32_t headerSize = 0;
    uint32_t capabilities = kSharedFrameAbiCapabilities;
    uint32_t slotCount = 0;
    uint32_t reserved = kSharedFrameAbiReserved;
};

// Maximum contacts in shared memory
constexpr int kMaxSharedContacts = 10;
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

constexpr int kSharedStylusRawGridDim = 9;

struct SharedStylusRawGridBlock {
    uint16_t anchorRow = 0x00FF;
    uint16_t anchorCol = 0x00FF;
    int16_t  grid[kSharedStylusRawGridDim][kSharedStylusRawGridDim]{};
    bool     valid = false;
};

struct SharedStylusRawGrid {
    SharedStylusRawGridBlock tx1{};
    SharedStylusRawGridBlock tx2{};
};

// Common-owned POD diagnostics mirror for the shared-memory ABI.
struct SharedStylusDiagnostics {
    uint16_t anchorRow = 0;
    uint16_t anchorCol = 0;
    int32_t  rawDim1 = 0;
    int32_t  rawDim2 = 0;
    int32_t  finalDim1 = 0;
    int32_t  finalDim2 = 0;
    float    centerOff = 0.f;
    float    pointX = 0.f;
    float    pointY = 0.f;
    bool     valid = false;
    float    speedInstant = 0.f;
    float    speedShortAvg = 0.f;
    float    speedFullAvg = 0.f;
    float    iirCoef = 0.f;
    bool     isHover = false;
    bool     isEdge = false;
    float    tiltDiffX = 0.f;
    float    tiltDiffY = 0.f;
    uint16_t peakSignal = 0;
    uint16_t rawPressure = 0;
    uint16_t mappedPressure = 0;
    uint32_t btSeq = 0;
    uint8_t  predictedAgeFrames = 0;
    bool     pressureIsReal = false;
    uint8_t  vhfPenState = 0;
    uint8_t  linearFilterState = 0;
    uint16_t signalRatio = 0;
    bool     exitSmoothed = false;
    bool     cmfEnabled = false;
    bool     coorReviserActive = false;
    float    coorRevDeltaX = 0.f;
    float    coorRevDeltaY = 0.f;
    bool     tiltAnomalyDamped = false;
    bool     sigSuppressActive = false;
    uint8_t  penLifecycle = 0;
    bool     wasInking = false;
    int32_t  avg3PtDim1 = 0;
    int32_t  avg3PtDim2 = 0;

    uint16_t tx1PeakValue = 0;
    uint16_t tx1Sum3x3 = 0;
    uint16_t tx2PeakValue = 0;
    uint16_t tx2Sum3x3 = 0;
    bool tx2Valid = false;

    uint16_t triDim1Left = 0;
    uint16_t triDim1Center = 0;
    uint16_t triDim1Right = 0;
    int16_t pitchCompApplied = 0;
    int32_t localCoorDim1 = 0;
    int32_t localCoorDim2 = 0;
    bool dim1Edge = false;
    bool dim2Edge = false;

    uint16_t tiltLenLimit = 0;
    int32_t tiltRawDiffDim1 = 0;
    int32_t tiltRawDiffDim2 = 0;
    int16_t preTiltDim1 = 0;
    int16_t preTiltDim2 = 0;
    int16_t reportTiltDim1 = 0;
    int16_t reportTiltDim2 = 0;

    uint16_t btRawPressure = 0;
    uint16_t preIirPressure = 0;
    bool btPressSuppressActive = false;
    uint8_t polySegment = 0;

    bool edgeSignalTooLowLatched = false;
    bool fakePressureDecreaseActive = false;
    uint8_t fakePressureDecreaseFramesLeft = 0;
    uint8_t btFreqShiftDebounceFramesLeft = 0;

    uint8_t lfStateMachine = 0;
    float lfLineFitSlopeA = 0.f;
    float lfLineFitInterceptB = 0.f;
    bool lfLineFitValid = false;
    int32_t lfCos1000 = 0;
    int32_t lfStraightBufCount = 0;
    int32_t lfDragApplied = 0;
};

// ───────────────────────────────────────────────────────────
// The actual shared memory layout (all POD, no virtual, no STL)
struct SharedFrameData {
    // RuntimeSnapshot (flat)
    int8_t   workerState       = 0;
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
    uint16_t rawDataLength = 0;
    uint8_t  rawData[Frame::kTotalFrameSize]{};
    uint64_t timestamp = 0;

    // Touch contacts & Features
    uint8_t touchZones[40][60]{};
    uint8_t peakZones[40][60]{};
    struct SharedPeak {
        int r, c;
        int16_t z;
        uint8_t id;
    } peaks[30]{};
    uint8_t peakCount = 0;
    
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
    uint16_t stylusPressure      = 0;
    uint16_t stylusBtRawPressure[4]{};
    SharedStylusRawGrid stylusRawGrid{};

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

    // ── Pipeline Diagnostics (Common-owned shared-memory POD mirror) ──
    SharedStylusDiagnostics diag{};

    // Structured suffix data — typed POD views over hardware status tables
    Frame::MasterSuffixView masterSuffix{};
    Frame::SlaveSuffixView  slaveSuffix{};
    bool     masterSuffixValid = false;
    bool     slaveSuffixValid  = false;
    bool     masterWasRead = true;
};

// ─────────────────────────────────────────────────────────────
// Triple-buffered shared memory layout
//
// Writer marks the target slot dirty before overwriting its payload, then
// publishes the slot frame id plus an even clean generation before updating
// readyIdx/frameId. Reader copies only when the slot generation is clean and
// unchanged before/after the payload copy.
struct SharedTripleBuffer {
    static constexpr uint32_t kSlotCount = 3;

    SharedFrameAbiHeader abi{
        .abiVersion = kSharedFrameAbiVersion,
        .totalSize = sizeof(SharedTripleBuffer),
        .headerSize = sizeof(SharedFrameAbiHeader),
        .capabilities = kSharedFrameAbiCapabilities,
        .slotCount = kSlotCount,
        .reserved = kSharedFrameAbiReserved,
    };

    // Control block (cache-line aligned atomics)
    alignas(64) std::atomic<uint32_t> readyIdx{0};    // latest complete slot for Reader
    alignas(64) std::atomic<uint64_t> frameId{0};      // monotonic frame counter
    alignas(64) std::atomic<uint64_t> slaveFrameId{0};
    alignas(64) std::atomic<uint64_t> masterFrameId{0};

    // Per-slot frame ids prove that readyIdx and frameId describe the same slot.
    alignas(64) std::atomic<uint64_t> slotFrameIds[kSlotCount]{};

    // Per-slot seqlock generations: odd = dirty/in-progress, even = clean/published.
    // This is intentionally separate from frameId to avoid ABA when a slot is reused.
    alignas(64) std::atomic<uint64_t> slotSequences[kSlotCount]{};

    // The three frame slots
    SharedFrameData slots[kSlotCount]{};
};

inline void ResetSharedFrameData(SharedFrameData& frame) noexcept {
    frame = SharedFrameData{};
}

inline void CopySharedFrameData(SharedFrameData& dst, const SharedFrameData& src) noexcept {
    if (&dst == &src) {
        return;
    }
    std::memcpy(&dst, &src, sizeof(SharedFrameData));
}

template <typename StylusType>
inline void PopulateLegacyStylusPacketForSharedFrame(SharedStylusPacket& dst,
                                                     const StylusType& stylus) {
    dst = SharedStylusPacket{};
    if constexpr (requires { stylus.output.packet.valid; stylus.output.packet.reportId; stylus.output.packet.length; stylus.output.packet.bytes.data(); }) {
        dst.valid = stylus.output.packet.valid;
        dst.reportId = stylus.output.packet.reportId;
        dst.length = stylus.output.packet.length;
        std::memcpy(dst.bytes, stylus.output.packet.bytes.data(), sizeof(dst.bytes));
    }
}

template <typename StylusType>
inline void PopulateStylusPacketFromLegacySharedFrame(StylusType& stylus,
                                                      const SharedStylusPacket& src) {
    if constexpr (requires { stylus.output.packet.valid = false; stylus.output.packet.reportId = uint8_t{}; stylus.output.packet.length = uint8_t{}; stylus.output.packet.bytes.data(); }) {
        stylus.output.packet.valid = src.valid;
        stylus.output.packet.reportId = src.reportId;
        stylus.output.packet.length = src.length;
        std::memcpy(stylus.output.packet.bytes.data(), src.bytes, sizeof(src.bytes));
    } else {
        (void)stylus;
        (void)src;
    }
}

template <typename SrcBlock>
inline void PopulateSharedStylusRawGridBlock(SharedStylusRawGridBlock& dst,
                                             const SrcBlock& src) {
    dst.anchorRow = src.anchorRow;
    dst.anchorCol = src.anchorCol;
    dst.valid = src.valid;
    std::memcpy(dst.grid, src.grid, sizeof(dst.grid));
}

template <typename DstBlock>
inline void PopulateStylusRawGridBlockFromShared(DstBlock& dst,
                                                 const SharedStylusRawGridBlock& src) {
    dst.anchorRow = src.anchorRow;
    dst.anchorCol = src.anchorCol;
    dst.valid = src.valid;
    std::memcpy(dst.grid, src.grid, sizeof(src.grid));
}

template <typename HeatmapFrame>
inline void PopulateSharedFrameDataFromSolverFrame(SharedFrameData& dst,
                                                   const HeatmapFrame& src) {
    ResetSharedFrameData(dst);
    std::memcpy(dst.heatmapMatrix, src.heatmapMatrix, sizeof(dst.heatmapMatrix));
    dst.rawDataLength = static_cast<uint16_t>(std::min(src.rawLen, static_cast<size_t>(Frame::kTotalFrameSize)));
    if (dst.rawDataLength != 0 && src.rawPtr) {
        std::memcpy(dst.rawData, src.rawPtr, dst.rawDataLength);
    }
    dst.timestamp = src.timestamp;
    dst.masterWasRead = src.masterWasRead;

#if EGOTOUCH_DIAG
    std::memcpy(dst.touchZones, src.touch.debug.touchZones.data(), sizeof(dst.touchZones));
    std::memcpy(dst.peakZones, src.touch.debug.peakZones.data(), sizeof(dst.peakZones));
    const int numPeaks = std::min(static_cast<int>(src.touch.debug.peaks.size()), 30);
    dst.peakCount = static_cast<uint8_t>(numPeaks);
    for (int i = 0; i < numPeaks; ++i) {
        dst.peaks[i].r = src.touch.debug.peaks[i].r;
        dst.peaks[i].c = src.touch.debug.peaks[i].c;
        dst.peaks[i].z = src.touch.debug.peaks[i].z;
        dst.peaks[i].id = src.touch.debug.peaks[i].id;
    }
#endif

    const int contactCount = std::min(static_cast<int>(src.touch.output.contacts.size()), kMaxSharedContacts);
    dst.contactCount = static_cast<uint8_t>(contactCount);
    for (int i = 0; i < contactCount; ++i) {
        const auto& srcContact = src.touch.output.contacts[static_cast<size_t>(i)];
        auto& dstContact = dst.contacts[i];
        dstContact.id = srcContact.id;
        dstContact.x = srcContact.x;
        dstContact.y = srcContact.y;
        dstContact.state = srcContact.state;
        dstContact.area = srcContact.area;
        dstContact.signalSum = srcContact.signalSum;
        dstContact.sizeMm = srcContact.sizeMm;
        dstContact.isEdge = srcContact.isEdge;
        dstContact.isReported = srcContact.isReported;
        dstContact.prevIndex = srcContact.prevIndex;
        dstContact.debugFlags = srcContact.debugFlags;
        dstContact.lifeFlags = srcContact.lifeFlags;
        dstContact.reportFlags = srcContact.reportFlags;
        dstContact.reportEvent = srcContact.reportEvent;
    }

    for (int i = 0; i < 2; ++i) {
        dst.touchPackets[i].valid = src.touch.output.touchPackets[i].valid;
        dst.touchPackets[i].reportId = src.touch.output.touchPackets[i].reportId;
        dst.touchPackets[i].length = src.touch.output.touchPackets[i].length;
        std::memcpy(dst.touchPackets[i].bytes, src.touch.output.touchPackets[i].bytes.data(), sizeof(dst.touchPackets[i].bytes));
    }

    const auto& srcPoint = src.stylus.output.point;
    auto& dstPoint = dst.stylusPoint;
    dstPoint.valid = srcPoint.valid;
    dstPoint.x = srcPoint.x;
    dstPoint.y = srcPoint.y;
    dstPoint.reportX = srcPoint.reportX;
    dstPoint.reportY = srcPoint.reportY;
    dstPoint.pressure = srcPoint.pressure;
    dstPoint.rawPressure = srcPoint.rawPressure;
    dstPoint.mappedPressure = srcPoint.mappedPressure;
    dstPoint.peakTx1 = srcPoint.peakTx1;
    dstPoint.peakTx2 = srcPoint.peakTx2;
    dstPoint.tiltValid = srcPoint.tiltValid;
    dstPoint.preTiltX = srcPoint.preTiltX;
    dstPoint.preTiltY = srcPoint.preTiltY;
    dstPoint.tiltX = srcPoint.tiltX;
    dstPoint.tiltY = srcPoint.tiltY;
    dstPoint.tiltMagnitude = srcPoint.tiltMagnitude;
    dstPoint.tiltAzimuthDeg = srcPoint.tiltAzimuthDeg;
    dstPoint.tx1X = srcPoint.tx1X;
    dstPoint.tx1Y = srcPoint.tx1Y;
    dstPoint.tx2X = srcPoint.tx2X;
    dstPoint.tx2Y = srcPoint.tx2Y;
    dstPoint.confidence = srcPoint.confidence;

    PopulateLegacyStylusPacketForSharedFrame(dst.stylusPacket, src.stylus);

    const auto& stylus = src.stylus;
    dst.stylusSlaveValid = stylus.input.slaveValid;
    dst.stylusChecksumOk = stylus.input.checksumOk;
    dst.stylusSlaveOffset = stylus.input.slaveWordOffset;
    dst.stylusChecksum16 = stylus.input.checksum16;
    dst.stylusTx1Valid = stylus.input.tx1BlockValid;
    dst.stylusTx2Valid = stylus.input.tx2BlockValid;
    dst.stylusStatus = stylus.input.status;
    dst.stylusPressure = stylus.output.pressure;
    for (int i = 0; i < 4; ++i) {
        dst.stylusBtRawPressure[i] = stylus.input.btSample.rawPressure[static_cast<size_t>(i)];
    }
    PopulateSharedStylusRawGridBlock(dst.stylusRawGrid.tx1, stylus.runtime.rawGrid.asaGrid.tx1);
    PopulateSharedStylusRawGridBlock(dst.stylusRawGrid.tx2, stylus.runtime.rawGrid.asaGrid.tx2);
    dst.stylusRecheckEnabled = stylus.interop.recheckEnabled;
    dst.stylusRecheckPassed = stylus.interop.recheckPassed;
    dst.stylusRecheckOverlap = stylus.interop.recheckOverlap;
    dst.stylusRecheckThreshold = stylus.interop.recheckThreshold;
    dst.stylusTouchNullLike = stylus.interop.touchNullLike;
    dst.stylusTouchSuppressActive = stylus.interop.touchSuppressActive;
    dst.stylusTouchSuppressFrames = stylus.interop.touchSuppressFrames;
    dst.stylusSignalX = stylus.interop.signalX;
    dst.stylusSignalY = stylus.interop.signalY;
    dst.stylusMaxRawPeak = stylus.interop.maxRawPeak;
    dst.stylusNoPressInk = false;
    dst.stylusPipelineStage = stylus.output.pipelineStage;

    auto& diag = dst.diag;
    const auto& srcDiag = stylus.debug.coord;
    diag.anchorRow = srcDiag.anchorRow;
    diag.anchorCol = srcDiag.anchorCol;
    diag.rawDim1 = srcDiag.rawDim1;
    diag.rawDim2 = srcDiag.rawDim2;
    diag.finalDim1 = srcDiag.finalDim1;
    diag.finalDim2 = srcDiag.finalDim2;
    diag.centerOff = srcDiag.centerOff;
    diag.pointX = srcDiag.pointX;
    diag.pointY = srcDiag.pointY;
    diag.valid = srcDiag.valid;
    diag.speedInstant = srcDiag.speedInstant;
    diag.speedShortAvg = srcDiag.speedShortAvg;
    diag.speedFullAvg = srcDiag.speedFullAvg;
    diag.iirCoef = srcDiag.iirCoef;
    diag.isHover = srcDiag.isHover;
    diag.isEdge = srcDiag.isEdge;
    diag.tiltDiffX = srcDiag.tiltDiffX;
    diag.tiltDiffY = srcDiag.tiltDiffY;
    diag.peakSignal = srcDiag.peakSignal;
    diag.rawPressure = srcDiag.rawPressure;
    diag.mappedPressure = srcDiag.mappedPressure;
    diag.btSeq = srcDiag.btSeq;
    diag.predictedAgeFrames = srcDiag.predictedAgeFrames;
    diag.pressureIsReal = srcDiag.pressureIsReal;
    diag.vhfPenState = srcDiag.vhfPenState;
    diag.linearFilterState = srcDiag.linearFilterState;
    diag.signalRatio = srcDiag.signalRatio;
    diag.exitSmoothed = srcDiag.exitSmoothed;
    diag.cmfEnabled = srcDiag.cmfEnabled;
    diag.coorReviserActive = srcDiag.coorReviserActive;
    diag.coorRevDeltaX = srcDiag.coorRevDeltaX;
    diag.coorRevDeltaY = srcDiag.coorRevDeltaY;
    diag.tiltAnomalyDamped = srcDiag.tiltAnomalyDamped;
    diag.sigSuppressActive = srcDiag.sigSuppressActive;
    diag.penLifecycle = srcDiag.penLifecycle;
    diag.wasInking = srcDiag.wasInking;
    diag.avg3PtDim1 = srcDiag.avg3PtDim1;
    diag.avg3PtDim2 = srcDiag.avg3PtDim2;
    diag.tx1PeakValue = srcDiag.tx1PeakValue;
    diag.tx1Sum3x3 = srcDiag.tx1Sum3x3;
    diag.tx2PeakValue = srcDiag.tx2PeakValue;
    diag.tx2Sum3x3 = srcDiag.tx2Sum3x3;
    diag.tx2Valid = srcDiag.tx2Valid;
    diag.triDim1Left = srcDiag.triDim1Left;
    diag.triDim1Center = srcDiag.triDim1Center;
    diag.triDim1Right = srcDiag.triDim1Right;
    diag.pitchCompApplied = srcDiag.pitchCompApplied;
    diag.localCoorDim1 = srcDiag.localCoorDim1;
    diag.localCoorDim2 = srcDiag.localCoorDim2;
    diag.dim1Edge = srcDiag.dim1Edge;
    diag.dim2Edge = srcDiag.dim2Edge;
    diag.tiltLenLimit = srcDiag.tiltLenLimit;
    diag.tiltRawDiffDim1 = srcDiag.tiltRawDiffDim1;
    diag.tiltRawDiffDim2 = srcDiag.tiltRawDiffDim2;
    diag.preTiltDim1 = srcDiag.preTiltDim1;
    diag.preTiltDim2 = srcDiag.preTiltDim2;
    diag.reportTiltDim1 = srcDiag.reportTiltDim1;
    diag.reportTiltDim2 = srcDiag.reportTiltDim2;
    diag.btRawPressure = srcDiag.btRawPressure;
    diag.preIirPressure = srcDiag.preIirPressure;
    diag.btPressSuppressActive = srcDiag.btPressSuppressActive;
    diag.polySegment = srcDiag.polySegment;
    diag.edgeSignalTooLowLatched = srcDiag.edgeSignalTooLowLatched;
    diag.fakePressureDecreaseActive = srcDiag.fakePressureDecreaseActive;
    diag.fakePressureDecreaseFramesLeft = srcDiag.fakePressureDecreaseFramesLeft;
    diag.btFreqShiftDebounceFramesLeft = srcDiag.btFreqShiftDebounceFramesLeft;
    diag.lfStateMachine = srcDiag.lfStateMachine;
    diag.lfLineFitSlopeA = srcDiag.lfLineFitSlopeA;
    diag.lfLineFitInterceptB = srcDiag.lfLineFitInterceptB;
    diag.lfLineFitValid = srcDiag.lfLineFitValid;
    diag.lfCos1000 = srcDiag.lfCos1000;
    diag.lfStraightBufCount = srcDiag.lfStraightBufCount;
    diag.lfDragApplied = srcDiag.lfDragApplied;

    dst.masterSuffix = src.masterSuffix;
    dst.masterSuffixValid = src.masterSuffixValid;
    dst.slaveSuffix = src.slaveSuffix;
    dst.slaveSuffixValid = src.slaveSuffixValid;
}

template <typename HeatmapFrame>
inline void PopulateSolverFrameFromSharedFrameData(HeatmapFrame& out,
                                                   const SharedFrameData& src) {
    std::memcpy(out.heatmapMatrix, src.heatmapMatrix, sizeof(out.heatmapMatrix));
#if EGOTOUCH_DIAG
    out.rawData.assign(src.rawData, src.rawData + std::min<int>(src.rawDataLength, Frame::kTotalFrameSize));
    out.rawPtr = out.rawData.empty() ? nullptr : out.rawData.data();
    out.rawLen = out.rawData.size();
#else
    out.rawPtr = nullptr;
    out.rawLen = 0;
#endif
    out.timestamp = src.timestamp;
    out.masterWasRead = src.masterWasRead;

#if EGOTOUCH_DIAG
    std::memcpy(out.touch.debug.touchZones.data(), src.touchZones, sizeof(src.touchZones));
    std::memcpy(out.touch.debug.peakZones.data(), src.peakZones, sizeof(src.peakZones));
    if (out.touch.debug.peaks.capacity() < 30) {
        out.touch.debug.peaks.reserve(30);
    }
    out.touch.debug.peaks.resize(src.peakCount);
    for (int i = 0; i < src.peakCount; ++i) {
        out.touch.debug.peaks[static_cast<size_t>(i)] = {src.peaks[i].r, src.peaks[i].c, src.peaks[i].z, src.peaks[i].id};
    }
#endif

    if (out.touch.output.contacts.capacity() < kMaxSharedContacts) {
        out.touch.output.contacts.reserve(kMaxSharedContacts);
    }
    out.touch.output.contacts.resize(src.contactCount);
    for (int i = 0; i < src.contactCount; ++i) {
        const auto& srcContact = src.contacts[i];
        auto& dstContact = out.touch.output.contacts[static_cast<size_t>(i)];
        dstContact.id = srcContact.id;
        dstContact.x = srcContact.x;
        dstContact.y = srcContact.y;
        dstContact.state = srcContact.state;
        dstContact.area = srcContact.area;
        dstContact.signalSum = srcContact.signalSum;
        dstContact.sizeMm = srcContact.sizeMm;
        dstContact.isEdge = srcContact.isEdge;
        dstContact.isReported = srcContact.isReported;
        dstContact.prevIndex = srcContact.prevIndex;
        dstContact.debugFlags = srcContact.debugFlags;
        dstContact.lifeFlags = srcContact.lifeFlags;
        dstContact.reportFlags = srcContact.reportFlags;
        dstContact.reportEvent = srcContact.reportEvent;
    }

    for (int i = 0; i < 2; ++i) {
        out.touch.output.touchPackets[i].valid = src.touchPackets[i].valid;
        out.touch.output.touchPackets[i].reportId = src.touchPackets[i].reportId;
        out.touch.output.touchPackets[i].length = src.touchPackets[i].length;
        std::memcpy(out.touch.output.touchPackets[i].bytes.data(), src.touchPackets[i].bytes, sizeof(src.touchPackets[i].bytes));
    }

    const auto& srcPoint = src.stylusPoint;
    auto& dstPoint = out.stylus.output.point;
    dstPoint.valid = srcPoint.valid;
    dstPoint.x = srcPoint.x;
    dstPoint.y = srcPoint.y;
    dstPoint.reportX = srcPoint.reportX;
    dstPoint.reportY = srcPoint.reportY;
    dstPoint.pressure = srcPoint.pressure;
    dstPoint.rawPressure = srcPoint.rawPressure;
    dstPoint.mappedPressure = srcPoint.mappedPressure;
    dstPoint.peakTx1 = srcPoint.peakTx1;
    dstPoint.peakTx2 = srcPoint.peakTx2;
    dstPoint.tiltValid = srcPoint.tiltValid;
    dstPoint.preTiltX = srcPoint.preTiltX;
    dstPoint.preTiltY = srcPoint.preTiltY;
    dstPoint.tiltX = srcPoint.tiltX;
    dstPoint.tiltY = srcPoint.tiltY;
    dstPoint.tiltMagnitude = srcPoint.tiltMagnitude;
    dstPoint.tiltAzimuthDeg = srcPoint.tiltAzimuthDeg;
    dstPoint.tx1X = srcPoint.tx1X;
    dstPoint.tx1Y = srcPoint.tx1Y;
    dstPoint.tx2X = srcPoint.tx2X;
    dstPoint.tx2Y = srcPoint.tx2Y;
    dstPoint.confidence = srcPoint.confidence;

    PopulateStylusPacketFromLegacySharedFrame(out.stylus, src.stylusPacket);

    auto& stylus = out.stylus;
    stylus.input.slaveValid = src.stylusSlaveValid;
    stylus.input.checksumOk = src.stylusChecksumOk;
    stylus.input.slaveWordOffset = src.stylusSlaveOffset;
    stylus.input.checksum16 = src.stylusChecksum16;
    stylus.input.tx1BlockValid = src.stylusTx1Valid;
    stylus.input.tx2BlockValid = src.stylusTx2Valid;
    stylus.input.status = src.stylusStatus;
    stylus.output.pressure = src.stylusPressure;
    for (int i = 0; i < 4; ++i) {
        stylus.input.btSample.rawPressure[static_cast<size_t>(i)] = src.stylusBtRawPressure[i];
    }
    PopulateStylusRawGridBlockFromShared(stylus.runtime.rawGrid.asaGrid.tx1, src.stylusRawGrid.tx1);
    PopulateStylusRawGridBlockFromShared(stylus.runtime.rawGrid.asaGrid.tx2, src.stylusRawGrid.tx2);
    stylus.interop.recheckEnabled = src.stylusRecheckEnabled;
    stylus.interop.recheckPassed = src.stylusRecheckPassed;
    stylus.interop.recheckOverlap = src.stylusRecheckOverlap;
    stylus.interop.recheckThreshold = src.stylusRecheckThreshold;
    stylus.interop.touchNullLike = src.stylusTouchNullLike;
    stylus.interop.touchSuppressActive = src.stylusTouchSuppressActive;
    stylus.interop.touchSuppressFrames = src.stylusTouchSuppressFrames;
    stylus.interop.signalX = src.stylusSignalX;
    stylus.interop.signalY = src.stylusSignalY;
    stylus.interop.maxRawPeak = src.stylusMaxRawPeak;
    stylus.output.pipelineStage = src.stylusPipelineStage;

    auto& diag = stylus.debug.coord;
    diag.anchorRow = src.diag.anchorRow;
    diag.anchorCol = src.diag.anchorCol;
    diag.rawDim1 = src.diag.rawDim1;
    diag.rawDim2 = src.diag.rawDim2;
    diag.finalDim1 = src.diag.finalDim1;
    diag.finalDim2 = src.diag.finalDim2;
    diag.centerOff = src.diag.centerOff;
    diag.pointX = src.diag.pointX;
    diag.pointY = src.diag.pointY;
    diag.valid = src.diag.valid;
    diag.speedInstant = src.diag.speedInstant;
    diag.speedShortAvg = src.diag.speedShortAvg;
    diag.speedFullAvg = src.diag.speedFullAvg;
    diag.iirCoef = src.diag.iirCoef;
    diag.isHover = src.diag.isHover;
    diag.isEdge = src.diag.isEdge;
    diag.tiltDiffX = src.diag.tiltDiffX;
    diag.tiltDiffY = src.diag.tiltDiffY;
    diag.peakSignal = src.diag.peakSignal;
    diag.rawPressure = src.diag.rawPressure;
    diag.mappedPressure = src.diag.mappedPressure;
    diag.btSeq = src.diag.btSeq;
    diag.predictedAgeFrames = src.diag.predictedAgeFrames;
    diag.pressureIsReal = src.diag.pressureIsReal;
    diag.vhfPenState = src.diag.vhfPenState;
    diag.linearFilterState = src.diag.linearFilterState;
    diag.signalRatio = src.diag.signalRatio;
    diag.exitSmoothed = src.diag.exitSmoothed;
    diag.cmfEnabled = src.diag.cmfEnabled;
    diag.coorReviserActive = src.diag.coorReviserActive;
    diag.coorRevDeltaX = src.diag.coorRevDeltaX;
    diag.coorRevDeltaY = src.diag.coorRevDeltaY;
    diag.tiltAnomalyDamped = src.diag.tiltAnomalyDamped;
    diag.sigSuppressActive = src.diag.sigSuppressActive;
    diag.penLifecycle = src.diag.penLifecycle;
    diag.wasInking = src.diag.wasInking;
    diag.avg3PtDim1 = src.diag.avg3PtDim1;
    diag.avg3PtDim2 = src.diag.avg3PtDim2;
    diag.tx1PeakValue = src.diag.tx1PeakValue;
    diag.tx1Sum3x3 = src.diag.tx1Sum3x3;
    diag.tx2PeakValue = src.diag.tx2PeakValue;
    diag.tx2Sum3x3 = src.diag.tx2Sum3x3;
    diag.tx2Valid = src.diag.tx2Valid;
    diag.triDim1Left = src.diag.triDim1Left;
    diag.triDim1Center = src.diag.triDim1Center;
    diag.triDim1Right = src.diag.triDim1Right;
    diag.pitchCompApplied = src.diag.pitchCompApplied;
    diag.localCoorDim1 = src.diag.localCoorDim1;
    diag.localCoorDim2 = src.diag.localCoorDim2;
    diag.dim1Edge = src.diag.dim1Edge;
    diag.dim2Edge = src.diag.dim2Edge;
    diag.tiltLenLimit = src.diag.tiltLenLimit;
    diag.tiltRawDiffDim1 = src.diag.tiltRawDiffDim1;
    diag.tiltRawDiffDim2 = src.diag.tiltRawDiffDim2;
    diag.preTiltDim1 = src.diag.preTiltDim1;
    diag.preTiltDim2 = src.diag.preTiltDim2;
    diag.reportTiltDim1 = src.diag.reportTiltDim1;
    diag.reportTiltDim2 = src.diag.reportTiltDim2;
    diag.btRawPressure = src.diag.btRawPressure;
    diag.preIirPressure = src.diag.preIirPressure;
    diag.btPressSuppressActive = src.diag.btPressSuppressActive;
    diag.polySegment = src.diag.polySegment;
    diag.edgeSignalTooLowLatched = src.diag.edgeSignalTooLowLatched;
    diag.fakePressureDecreaseActive = src.diag.fakePressureDecreaseActive;
    diag.fakePressureDecreaseFramesLeft = src.diag.fakePressureDecreaseFramesLeft;
    diag.btFreqShiftDebounceFramesLeft = src.diag.btFreqShiftDebounceFramesLeft;
    diag.lfStateMachine = src.diag.lfStateMachine;
    diag.lfLineFitSlopeA = src.diag.lfLineFitSlopeA;
    diag.lfLineFitInterceptB = src.diag.lfLineFitInterceptB;
    diag.lfLineFitValid = src.diag.lfLineFitValid;
    diag.lfCos1000 = src.diag.lfCos1000;
    diag.lfStraightBufCount = src.diag.lfStraightBufCount;
    diag.lfDragApplied = src.diag.lfDragApplied;

    out.masterSuffix = src.masterSuffix;
    out.masterSuffixValid = src.masterSuffixValid;
    out.slaveSuffix = src.slaveSuffix;
    out.slaveSuffixValid = src.slaveSuffixValid;
}

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
    void Write(const SharedFrameData& frame);
    void Close();
    bool IsOpen() const { return m_buf != nullptr; }

private:
    HANDLE             m_mapHandle = nullptr;
    SharedTripleBuffer* m_buf      = nullptr;
    uint32_t           m_writeIdx  = 0;  // slot currently being written to
    HANDLE             m_frameEvent = nullptr;
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
    bool Read(SharedFrameData& out);
    template <typename HeatmapFrame>
    bool Read(HeatmapFrame& out) {
        SharedFrameData frame{};
        if (!Read(frame)) {
            return false;
        }
        PopulateSolverFrameFromSharedFrameData(out, frame);
        return true;
    }
    uint64_t LastFrameId() const;
    uint64_t LastSlaveFrameId() const;
    uint64_t LastMasterFrameId() const;
    const SharedTripleBuffer* RawBuffer() const { return m_buf; }
    /// ⚠️  UNSAFE — returns a raw pointer to the current ready slot WITHOUT
    /// seqlock protection. The writer may overwrite this slot while the caller
    /// reads it, producing torn data. Prefer Read() for safe access with
    /// built-in integrity checking.
    const SharedFrameData* Raw() const {
        return m_buf ? &m_buf->slots[m_buf->readyIdx.load(std::memory_order_acquire)] : nullptr;
    }
    HANDLE FrameReadyEvent() const { return m_frameEvent; }
    void Close();
    bool IsOpen() const { return m_buf != nullptr; }

private:
    HANDLE             m_mapHandle = nullptr;
    SharedTripleBuffer* m_buf      = nullptr;
    uint64_t           m_lastReadId = 0;
    HANDLE             m_frameEvent = nullptr;
};

} // namespace Ipc
