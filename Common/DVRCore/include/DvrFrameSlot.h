#pragma once
// DvrFrameSlot.h — Fixed-size, pure-POD frame slot for zero-copy DVR recording.
//
// Replaces the heap-allocating HeatmapFrame in the DVR ring buffer.
// All fields are fixed-size arrays — PushOverwriting becomes a single memcpy
// with zero heap allocation per frame.

#include <algorithm>
#include <cstdint>
#include <cstring>
#include "FrameLayout.h"
#include "SolverTypes.h"

namespace Dvr {

constexpr int kMaxContacts = 10;
constexpr int kMaxPeaks    = 30;

struct DvrContact {
    int32_t id = 0;
    float x = 0.0f;
    float y = 0.0f;
    int32_t state = 0;
    int32_t area = 0;
    int32_t signalSum = 0;
    float sizeMm = 0.0f;
    float edgeDistX = 0.0f;
    float edgeDistY = 0.0f;
    float rawXBeforeEC = 0.0f;
    float rawYBeforeEC = 0.0f;
    int32_t prevIndex = -1;
    int32_t debugFlags = 0;
    uint32_t edgeFlags = 0;
    uint32_t ecFlags = 0;
    uint32_t lifeFlags = 0;
    uint32_t reportFlags = 0;
    int32_t reportEvent = 0;
    uint8_t isEdge = 0;
    uint8_t isReported = 1;
    uint8_t centroidEdgeFlags = 0;
    uint8_t ecWidthX = 0;
    uint8_t ecWidthY = 0;
    uint8_t reserved[3]{};
};
static_assert(sizeof(DvrContact) == 80, "DvrContact must be exactly 80 bytes");

struct DvrPeak {
    int r = 0;
    int c = 0;
    int16_t z = 0;
    uint8_t id = 0;
    uint8_t _pad = 0;
};
static_assert(sizeof(DvrPeak) == 12, "DvrPeak must be exactly 12 bytes");

struct DvrTouchPacket {
    uint8_t valid = 0;
    uint8_t reportId = 0x01;
    uint8_t length = 0x20;
    uint8_t reserved = 0;
    uint8_t bytes[32]{};
};
static_assert(sizeof(DvrTouchPacket) == 36, "DvrTouchPacket must be exactly 36 bytes");

struct DvrStylusPacket {
    uint8_t valid = 0;
    uint8_t reportId = 0x08;
    uint8_t length = 17;
    uint8_t reserved = 0;
    uint8_t bytes[17]{};
    uint8_t reservedTail[3]{};
};
static_assert(sizeof(DvrStylusPacket) == 24, "DvrStylusPacket must be exactly 24 bytes");

constexpr int kMaxDynamicDebugSamples = 256;

struct DvrDynamicDebugSampleSlot {
    uint16_t fieldId = 0;
    uint8_t valueType = 0;
    uint8_t valid = 0;
    uint32_t reserved = 0;
    uint64_t rawValue = 0;
};
static_assert(sizeof(DvrDynamicDebugSampleSlot) == 16, "DvrDynamicDebugSampleSlot must be exactly 16 bytes");

struct DvrDynamicDebugFrameSlot {
    uint64_t dvrSeq = 0;
    uint16_t sampleCount = 0;
    uint16_t reserved0 = 0;
    uint32_t reserved1 = 0;
    DvrDynamicDebugSampleSlot samples[kMaxDynamicDebugSamples]{};
};

struct DvrStylusPoint {
    bool valid = false;
    float x = 0.0f;
    float y = 0.0f;
    uint16_t reportX = 0;
    uint16_t reportY = 0;
    uint16_t pressure = 0;
    uint16_t rawPressure = 0;
    uint16_t mappedPressure = 0;
    uint16_t peakTx1 = 0;
    uint16_t peakTx2 = 0;
    bool tiltValid = false;
    int16_t preTiltX = 0;
    int16_t preTiltY = 0;
    int16_t tiltX = 0;
    int16_t tiltY = 0;
    float tiltMagnitude = 0.0f;
    float tiltAzimuthDeg = 0.0f;
    float tx1X = 0.0f;
    float tx1Y = 0.0f;
    float tx2X = 0.0f;
    float tx2Y = 0.0f;
    float confidence = 0.0f;
};

struct DvrStylusData {
    bool slaveValid = false;
    bool checksumOk = false;
    uint8_t slaveWordOffset = 0;
    uint16_t checksum16 = 0;
    bool tx1BlockValid = false;
    bool tx2BlockValid = false;
    uint32_t status = 0;
    uint16_t pressure = 0;
    uint16_t btPressure[4]{};
    uint16_t btRawPressure[4]{};
    uint32_t btSeq = 0;
    uint8_t btFreq1 = 0;
    uint8_t btFreq2 = 0;
    bool btHasSample = false;
    bool btHasFreq = false;
    uint16_t signalX = 0;
    uint16_t signalY = 0;
    uint16_t maxRawPeak = 0;
    uint8_t pipelineStage = 0;
    bool outputValid = false;
    bool inRange = false;
    bool tipDown = false;
    float outputConfidence = 0.0f;
    bool recheckEnabled = false;
    bool recheckPassed = true;
    bool recheckOverlap = false;
    uint16_t recheckThreshold = 0;
    uint16_t recheckThresholdMulti = 0;
    bool touchNullLike = false;
    bool touchSuppressActive = false;
    uint8_t touchSuppressFrames = 0;
    bool pressureIsReal = false;
    uint8_t predictedAgeFrames = 0;
    Solvers::Stylus::Hpp3::GridData rawGrid{};
    DvrStylusPacket packet{};
    DvrStylusPoint point{};
};

struct DvrFrameSlot {
    uint64_t timestamp = 0;
    uint64_t receiveSystemEpochUs = 0;
    uint64_t dvrSeq = 0;
    bool masterWasRead = true;

    int16_t heatmapMatrix[40][60]{};
    uint16_t rawDataLength = 0;
    uint8_t rawData[Frame::kTotalFrameSize]{};

    Frame::MasterSuffixView masterSuffix{};
    Frame::SlaveSuffixView  slaveSuffix{};
    bool masterSuffixValid = false;
    bool slaveSuffixValid  = false;

    DvrTouchPacket touchPackets[2]{};
    uint8_t touchZones[Frame::kMatrixCells]{};
    uint8_t peakZones[Frame::kMatrixCells]{};

    DvrStylusData stylus{};

    DvrContact contacts[kMaxContacts]{};
    uint8_t contactCount = 0;

    DvrPeak peaks[kMaxPeaks]{};
    uint8_t peakCount = 0;

    void CopyFrom(const Solvers::HeatmapFrame& src) {
        timestamp = src.timestamp;
        receiveSystemEpochUs = src.receiveSystemEpochUs;
        masterWasRead = src.masterWasRead;

        std::memcpy(heatmapMatrix, src.heatmapMatrix, sizeof(heatmapMatrix));
        rawDataLength = static_cast<uint16_t>(std::min(src.rawLen, static_cast<size_t>(Frame::kTotalFrameSize)));
        if (rawDataLength != 0 && src.rawPtr) {
            std::memcpy(rawData, src.rawPtr, rawDataLength);
        }

        masterSuffix = src.masterSuffix;
        slaveSuffix  = src.slaveSuffix;
        masterSuffixValid = src.masterSuffixValid;
        slaveSuffixValid  = src.slaveSuffixValid;

        for (size_t i = 0; i < 2; ++i) {
            touchPackets[i].valid = src.touch.output.touchPackets[i].valid ? 1 : 0;
            touchPackets[i].reportId = src.touch.output.touchPackets[i].reportId;
            touchPackets[i].length = src.touch.output.touchPackets[i].length;
            std::memcpy(touchPackets[i].bytes, src.touch.output.touchPackets[i].bytes.data(), sizeof(touchPackets[i].bytes));
        }

#if EGOTOUCH_DIAG
        std::memcpy(touchZones, src.touch.debug.touchZones.data(), sizeof(touchZones));
        std::memcpy(peakZones, src.touch.debug.peakZones.data(), sizeof(peakZones));
#endif

        stylus.slaveValid = src.stylus.input.slaveValid;
        stylus.checksumOk = src.stylus.input.checksumOk;
        stylus.slaveWordOffset = src.stylus.input.slaveWordOffset;
        stylus.checksum16 = src.stylus.input.checksum16;
        stylus.tx1BlockValid = src.stylus.input.tx1BlockValid;
        stylus.tx2BlockValid = src.stylus.input.tx2BlockValid;
        stylus.status = src.stylus.input.status;
        stylus.pressure = src.stylus.output.pressure;
        for (int i = 0; i < 4; ++i) {
            stylus.btPressure[i] = src.stylus.input.btSample.pressure[static_cast<size_t>(i)];
            stylus.btRawPressure[i] = src.stylus.input.btSample.rawPressure[static_cast<size_t>(i)];
        }
        stylus.btSeq = src.stylus.input.btSample.seq;
        stylus.btFreq1 = src.stylus.input.btSample.freq1;
        stylus.btFreq2 = src.stylus.input.btSample.freq2;
        stylus.btHasSample = src.stylus.input.btSample.hasSample;
        stylus.btHasFreq = src.stylus.input.btSample.hasFreq;
        stylus.signalX = src.stylus.interop.signalX;
        stylus.signalY = src.stylus.interop.signalY;
        stylus.maxRawPeak = src.stylus.interop.maxRawPeak;
        stylus.pipelineStage = src.stylus.output.pipelineStage;
        stylus.outputValid = src.stylus.output.valid;
        stylus.inRange = src.stylus.output.inRange;
        stylus.tipDown = src.stylus.output.tipDown;
        stylus.outputConfidence = src.stylus.output.confidence;
        stylus.recheckEnabled = src.stylus.interop.recheckEnabled;
        stylus.recheckPassed = src.stylus.interop.recheckPassed;
        stylus.recheckOverlap = src.stylus.interop.recheckOverlap;
        stylus.recheckThreshold = src.stylus.interop.recheckThreshold;
        stylus.recheckThresholdMulti = src.stylus.interop.recheckThresholdMulti;
        stylus.touchNullLike = src.stylus.interop.touchNullLike;
        stylus.touchSuppressActive = src.stylus.interop.touchSuppressActive;
        stylus.touchSuppressFrames = src.stylus.interop.touchSuppressFrames;
        stylus.pressureIsReal = src.stylus.runtime.Active().pressure.pressureIsReal;
        stylus.predictedAgeFrames = src.stylus.runtime.Active().pressure.predictedAgeFrames;
        stylus.rawGrid = src.stylus.runtime.hpp3.rawGrid.grid;
        stylus.packet.valid = src.stylus.output.packet.valid ? 1 : 0;
        stylus.packet.reportId = src.stylus.output.packet.reportId;
        stylus.packet.length = src.stylus.output.packet.length;
        std::memcpy(stylus.packet.bytes, src.stylus.output.packet.bytes.data(), sizeof(stylus.packet.bytes));
        stylus.point.valid = src.stylus.output.point.valid;
        stylus.point.x = src.stylus.output.point.x;
        stylus.point.y = src.stylus.output.point.y;
        stylus.point.reportX = src.stylus.output.point.reportX;
        stylus.point.reportY = src.stylus.output.point.reportY;
        stylus.point.pressure = src.stylus.output.point.pressure;
        stylus.point.rawPressure = src.stylus.output.point.rawPressure;
        stylus.point.mappedPressure = src.stylus.output.point.mappedPressure;
        stylus.point.peakTx1 = src.stylus.output.point.peakTx1;
        stylus.point.peakTx2 = src.stylus.output.point.peakTx2;
        stylus.point.tiltValid = src.stylus.output.point.tiltValid;
        stylus.point.preTiltX = src.stylus.output.point.preTiltX;
        stylus.point.preTiltY = src.stylus.output.point.preTiltY;
        stylus.point.tiltX = src.stylus.output.point.tiltX;
        stylus.point.tiltY = src.stylus.output.point.tiltY;
        stylus.point.tiltMagnitude = src.stylus.output.point.tiltMagnitude;
        stylus.point.tiltAzimuthDeg = src.stylus.output.point.tiltAzimuthDeg;
        stylus.point.tx1X = src.stylus.output.point.tx1X;
        stylus.point.tx1Y = src.stylus.output.point.tx1Y;
        stylus.point.tx2X = src.stylus.output.point.tx2X;
        stylus.point.tx2Y = src.stylus.output.point.tx2Y;
        stylus.point.confidence = src.stylus.output.point.confidence;

        contactCount = static_cast<uint8_t>(std::min(static_cast<int>(src.touch.output.contacts.size()), kMaxContacts));
        for (int i = 0; i < contactCount; ++i) {
            const auto& sc = src.touch.output.contacts[i];
            contacts[i].id = sc.id;
            contacts[i].x = sc.x;
            contacts[i].y = sc.y;
            contacts[i].state = sc.state;
            contacts[i].area = sc.area;
            contacts[i].signalSum = sc.signalSum;
            contacts[i].sizeMm = sc.sizeMm;
            contacts[i].edgeDistX = sc.edgeDistX;
            contacts[i].edgeDistY = sc.edgeDistY;
            contacts[i].rawXBeforeEC = sc.rawXBeforeEC;
            contacts[i].rawYBeforeEC = sc.rawYBeforeEC;
            contacts[i].prevIndex = sc.prevIndex;
            contacts[i].debugFlags = sc.debugFlags;
            contacts[i].edgeFlags = sc.edgeFlags;
            contacts[i].centroidEdgeFlags = sc.centroidEdgeFlags;
            contacts[i].ecFlags = sc.ecFlags;
            contacts[i].ecWidthX = sc.ecWidthX;
            contacts[i].ecWidthY = sc.ecWidthY;
            contacts[i].lifeFlags = sc.lifeFlags;
            contacts[i].reportFlags = sc.reportFlags;
            contacts[i].reportEvent = sc.reportEvent;
            contacts[i].isEdge = sc.isEdge ? 1 : 0;
            contacts[i].isReported = sc.isReported ? 1 : 0;
        }
        for (int i = contactCount; i < kMaxContacts; ++i) {
            contacts[i] = {};
        }

#if EGOTOUCH_DIAG
        peakCount = static_cast<uint8_t>(std::min(static_cast<int>(src.touch.debug.peaks.size()), kMaxPeaks));
        for (int i = 0; i < peakCount; ++i) {
            const auto& sp = src.touch.debug.peaks[i];
            peaks[i] = { sp.r, sp.c, sp.z, sp.id, 0 };
        }
#else
        peakCount = 0;
#endif
        for (int i = peakCount; i < kMaxPeaks; ++i) {
            peaks[i] = {};
        }
    }
};

} // namespace Dvr
