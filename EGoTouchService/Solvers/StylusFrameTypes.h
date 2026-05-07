#pragma once

#include <array>
#include <cstdint>

#include "SolverBuildConfig.h"
#include "StylusSolver/AsaTypes.hpp"

namespace Solvers {

struct StylusSolvePoint {
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

struct StylusPacket {
    bool valid = false;
    uint8_t reportId = 0x08;
    uint8_t length = 17;
    std::array<uint8_t, 17> bytes{};
};

enum class StylusPacketRoute : uint8_t {
    Valid,
    InvalidZeroState,
    ParseFailure13,
};

struct StylusBtInputSnapshot {
    uint16_t pressure = 0;
    uint32_t seq = 0;
    bool hasSample = false;
};

struct StylusInputSnapshot {
    bool slaveValid = false;
    bool checksumOk = false;
    uint8_t slaveWordOffset = 0;
    uint16_t checksum16 = 0;
    bool tx1BlockValid = false;
    bool tx2BlockValid = false;
    uint32_t status = 0;
    StylusBtInputSnapshot btSample{};
};

struct StylusOutputState {
    bool valid = false;
    bool inRange = false;
    bool tipDown = false;
    uint16_t pressure = 0;
    float confidence = 0.0f;
    uint8_t pipelineStage = 0;
    StylusSolvePoint point{};
};

struct StylusTouchInterop {
    bool recheckEnabled = false;
    bool recheckPassed = true;
    bool recheckOverlap = false;
    uint16_t recheckThreshold = 0;
    uint16_t recheckThresholdMulti = 0;
    bool touchNullLike = false;
    bool touchSuppressActive = false;
    uint8_t touchSuppressFrames = 0;
    uint16_t signalX = 0;
    uint16_t signalY = 0;
    uint16_t maxRawPeak = 0;
};

struct StylusRuntimeFlow {
    bool terminal = false;
    bool resetPost = false;
    bool resetNoise = false;
    uint8_t pipelineStage = 0;
    Asa::StylusFrameClass frameClass = Asa::StylusFrameClass::ShortFrame;
};

struct StylusRuntimeParse {
    bool valid = false;
    bool slaveValid = false;
    bool checksumOk = false;
    bool isFullFrame = false;
    bool hasCurrentStylusSignal = false;
    uint32_t status = 0;
    uint16_t checksum16 = 0;
    std::array<uint8_t, Asa::kSlaveHeaderBytes> rawSlaveHdr{};
    Asa::AsaGridData gridData{};
};

struct StylusRuntimeProjection {
    int16_t grid[Asa::kGridDim][Asa::kGridDim]{};
    Asa::GridPeakUnit peak{};
    Asa::AsaProjection projection{};
    uint16_t peakSignal = 0;
    Asa::AsaCoorResult localCoor{};
    Asa::AsaCoorResult globalCoor{};
};

struct StylusRuntimeSignal {
    bool recheckEnabled = false;
    bool recheckPassed = false;
    bool recheckOverlap = false;
    uint16_t recheckThreshold = 0;
    uint16_t recheckThresholdMulti = 0;
    bool touchNullLike = false;
    uint16_t signalX = 0;
    uint16_t signalY = 0;
    uint16_t maxRawPeak = 0;
    uint16_t tx1Composite = 0;
    uint16_t tx2Composite = 0;
    bool dim1EdgeActive = false;
    bool dim2EdgeActive = false;
    uint16_t dim1EdgeSignal = 0;
    uint16_t dim2EdgeSignal = 0;
    bool overlapLike = false;
};

struct StylusRuntimePressure {
    StylusBtInputSnapshot btSample{};
    bool pressureIsReal = false;
    uint16_t rawPressure = 0;
    uint16_t mappedPressure = 0;
    uint16_t outputPressure = 0;
    uint32_t btSeq = 0;
    uint8_t predictedAgeFrames = 0;
};

struct StylusRuntimeDecision {
    bool inRangeCandidate = false;
    bool tipDownCandidate = false;
    bool authoritativeDown = false;
    bool immediateRelease = false;
    bool keepInRange = false;
    bool touchSuppressCarry = false;
    uint8_t touchSuppressFrames = 0;
    bool enableCoordFilter = false;
    bool enableCoorReviser = false;
    bool enableEdgeCorrect = false;
};

struct StylusRuntimePost {
    Asa::AsaCoorResult postCoor{};
    Asa::AsaCoorResult finalCoor{};
    StylusSolvePoint point{};
    bool finalValid = false;
    uint16_t finalPressure = 0;
    float confidence = 0.0f;
};

struct StylusRuntimeFrame {
    StylusRuntimeFlow flow{};
    StylusRuntimeParse parse{};
    StylusRuntimeProjection tx1{};
    StylusRuntimeProjection tx2{};
    StylusRuntimeSignal signal{};
    StylusRuntimePressure pressure{};
    StylusRuntimeDecision decision{};
    StylusRuntimePost post{};

    void Reset() {
        *this = {};
    }
};

struct StylusDebugFrame {
    struct ParseSnapshot {
        bool slaveValid = false;
        bool checksumOk = false;
        uint32_t status = 0;
        uint8_t pipelineStage = 0;
    };

    struct StylusDiagnostics {
        uint16_t anchorRow = 0;
        uint16_t anchorCol = 0;
        int32_t rawDim1 = 0;
        int32_t rawDim2 = 0;
        int32_t finalDim1 = 0;
        int32_t finalDim2 = 0;
        float centerOff = 0.f;
        float pointX = 0.f;
        float pointY = 0.f;
        bool valid = false;

        float speedInstant = 0.f;
        float speedShortAvg = 0.f;
        float speedFullAvg = 0.f;
        float iirCoef = 0.f;
        bool isHover = false;
        bool isEdge = false;

        float tiltDiffX = 0.f;
        float tiltDiffY = 0.f;

        uint16_t peakSignal = 0;
        uint16_t rawPressure = 0;
        uint16_t mappedPressure = 0;
        uint32_t btSeq = 0;
        uint8_t predictedAgeFrames = 0;
        bool pressureIsReal = false;

        uint8_t vhfPenState = 0;
        uint8_t linearFilterState = 0;

        uint16_t signalRatio = 0;
        bool exitSmoothed = false;
        bool cmfEnabled = false;
        bool coorReviserActive = false;
        float coorRevDeltaX = 0.f;
        float coorRevDeltaY = 0.f;
        bool tiltAnomalyDamped = false;
        bool sigSuppressActive = false;
        uint8_t penLifecycle = 0;
        bool wasInking = false;
        int32_t avg3PtDim1 = 0;
        int32_t avg3PtDim2 = 0;
    };

    ParseSnapshot parse{};
    StylusDiagnostics coord{};
};

struct StylusFrameData {
    using StylusDiagnostics = StylusDebugFrame::StylusDiagnostics;

    StylusInputSnapshot input{};
    StylusOutputState output{};
    StylusTouchInterop interop{};
    StylusRuntimeFrame runtime{};
#if EGOTOUCH_DIAG
    StylusDebugFrame debug{};
#endif

    bool slaveValid = false;
    bool checksumOk = false;
    uint8_t slaveWordOffset = 0;
    uint16_t checksum16 = 0;
    bool tx1BlockValid = false;
    bool tx2BlockValid = false;

    uint32_t status = 0;
    uint16_t pressure = 0;
    bool tipSwitchActive = false;

    bool recheckEnabled = false;
    bool recheckPassed = true;
    bool recheckOverlap = false;
    uint16_t recheckThreshold = 0;
    uint16_t recheckThresholdMulti = 0;
    bool touchNullLike = false;
    bool touchSuppressActive = false;
    uint8_t touchSuppressFrames = 0;

    uint16_t signalX = 0;
    uint16_t signalY = 0;
    uint16_t maxRawPeak = 0;

#if EGOTOUCH_DIAG
    uint8_t asaMode = 0;
    uint8_t dataType = 0;
    uint8_t processResult = 5;
    bool validJudgmentPassed = false;
    bool modeExitRelease = false;
    bool hpp3NoiseInvalid = false;
    bool hpp3NoiseDebounce = false;
    bool hpp3Dim1SignalValid = false;
    bool hpp3Dim2SignalValid = false;
    uint8_t hpp3RatioWarnCountX = 0;
    uint8_t hpp3RatioWarnCountY = 0;
    uint16_t hpp3SignalAvgX = 0;
    uint16_t hpp3SignalAvgY = 0;
    uint8_t hpp3SignalSampleCount = 0;

    StylusPacket packet{};
    StylusPacketRoute packetRoute = StylusPacketRoute::Valid;
#endif

    StylusSolvePoint point{};
    uint8_t pipelineStage = 0;
    StylusDiagnostics diag{};

    inline void SnapshotBtInput(uint16_t btPressure, uint32_t btSeq, bool hasBtSample) {
        input.btSample.pressure = btPressure;
        input.btSample.seq = btSeq;
        input.btSample.hasSample = hasBtSample;
    }

    inline void ResetRuntime() {
        runtime.Reset();
    }

    inline void SyncContractFromLegacyFields() {
        input.slaveValid = slaveValid;
        input.checksumOk = checksumOk;
        input.slaveWordOffset = slaveWordOffset;
        input.checksum16 = checksum16;
        input.tx1BlockValid = tx1BlockValid;
        input.tx2BlockValid = tx2BlockValid;
        input.status = status;

        output.valid = point.valid;
        output.inRange = point.valid;
        output.tipDown = tipSwitchActive || pressure > 0;
        output.pressure = pressure;
        output.confidence = point.confidence;
        output.pipelineStage = pipelineStage;
        output.point = point;
        output.point.valid = output.valid;
        output.point.pressure = pressure;

        interop.recheckEnabled = recheckEnabled;
        interop.recheckPassed = recheckPassed;
        interop.recheckOverlap = recheckOverlap;
        interop.recheckThreshold = recheckThreshold;
        interop.recheckThresholdMulti = recheckThresholdMulti;
        interop.touchNullLike = touchNullLike;
        interop.touchSuppressActive = touchSuppressActive;
        interop.touchSuppressFrames = touchSuppressFrames;
        interop.signalX = signalX;
        interop.signalY = signalY;
        interop.maxRawPeak = maxRawPeak;
#if EGOTOUCH_DIAG
        debug.parse.slaveValid = slaveValid;
        debug.parse.checksumOk = checksumOk;
        debug.parse.status = status;
        debug.parse.pipelineStage = pipelineStage;
        debug.coord = diag;
#endif
    }

    inline void SyncLegacyFieldsFromContract() {
        slaveValid = input.slaveValid;
        checksumOk = input.checksumOk;
        slaveWordOffset = input.slaveWordOffset;
        checksum16 = input.checksum16;
        tx1BlockValid = input.tx1BlockValid;
        tx2BlockValid = input.tx2BlockValid;
        status = input.status;

        pressure = output.pressure;
        tipSwitchActive = output.tipDown;
        point = output.point;
        point.valid = output.valid;
        point.pressure = output.pressure;
        pipelineStage = output.pipelineStage;

        recheckEnabled = interop.recheckEnabled;
        recheckPassed = interop.recheckPassed;
        recheckOverlap = interop.recheckOverlap;
        recheckThreshold = interop.recheckThreshold;
        recheckThresholdMulti = interop.recheckThresholdMulti;
        touchNullLike = interop.touchNullLike;
        touchSuppressActive = interop.touchSuppressActive;
        touchSuppressFrames = interop.touchSuppressFrames;
        signalX = interop.signalX;
        signalY = interop.signalY;
        maxRawPeak = interop.maxRawPeak;
#if EGOTOUCH_DIAG
        diag = debug.coord;
#endif
    }
};

} // namespace Solvers
