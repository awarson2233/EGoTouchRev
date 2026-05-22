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

struct StylusBtInputSnapshot {
    std::array<uint16_t, 4> pressure{};
    std::array<uint16_t, 4> rawPressure{};
    uint32_t seq = 0;
    uint8_t freq1 = 0;
    uint8_t freq2 = 0;
    bool hasSample = false;
    bool hasFreq = false;
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
    StylusPacket packet{};
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
};

struct StylusRuntimeRawGrid {
    Asa::AsaGridData asaGrid{};
};

struct StylusGridFeature {
    int16_t grid[Asa::kGridDim][Asa::kGridDim]{};
    Asa::GridPeakUnit peak{};
    Asa::GridPeakTable peakTable{};
    Asa::AsaProjection projection{};
    Asa::AsaCoorResult refinedLocalCoor{};
    uint16_t peakSignal = 0;
};

struct StylusCoordinateResult {
    Asa::AsaCoorResult localGridCoor{};
    Asa::AsaCoorResult reportGlobalCoor{};
};

struct StylusTxRuntime {
    StylusGridFeature feature{};
    StylusCoordinateResult coordinate{};
#if EGOTOUCH_DIAG
    uint16_t triLeft = 0;
    uint16_t triCenter = 0;
    uint16_t triRight = 0;
    int16_t pitchComp = 0;
#endif
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
    bool dim1EdgeActive = false;
    bool dim2EdgeActive = false;
    uint16_t dim1EdgeSignal = 0;
    uint16_t dim2EdgeSignal = 0;
    bool overlapLike = false;
};

struct StylusRuntimeTilt {
    bool valid = false;
    bool anomalyDamped = false;
    uint16_t signalRatio = 0;
    uint16_t lenLimit = 0;
    int32_t diffDim1 = 0;
    int32_t diffDim2 = 0;
    int16_t preTiltDim1 = 0;
    int16_t preTiltDim2 = 0;
    int16_t reportTiltDim1 = 0;
    int16_t reportTiltDim2 = 0;
#if EGOTOUCH_DIAG
    int32_t rawDiffDim1 = 0;
    int32_t rawDiffDim2 = 0;
    bool circularClamped = false;
#endif
};

struct StylusRuntimePressure {
    StylusBtInputSnapshot btSample{};
    bool pressureIsReal = false;
    bool lookaheadHoverGate = false;
    uint16_t rawPressure = 0;
    uint16_t mappedPressure = 0;
    uint16_t outputPressure = 0;
    uint32_t btSeq = 0;
    uint8_t predictedAgeFrames = 0;
#if EGOTOUCH_DIAG
    uint16_t preIirPressure = 0;
    uint8_t polySegment = 0;
    bool btPressSuppressActive = false;
    bool edgeSignalTooLowLatched = false;
    bool fakePressureDecreaseActive = false;
    uint8_t fakePressureDecreaseFramesLeft = 0;
    uint8_t btFreqShiftDebounceFramesLeft = 0;
#endif
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
    Asa::AsaCoorResult predictedCoor{};
    StylusSolvePoint point{};
    bool finalValid = false;
    uint16_t finalPressure = 0;
    float confidence = 0.0f;
    uint8_t linearFilterState = 0;
    bool linearFilterActive = false;
    int32_t linearFilterDeltaDim1 = 0;
    int32_t linearFilterDeltaDim2 = 0;

    // ── Hpp3NoiseProcess outputs ──
    bool noiseRejected = false;
    uint8_t noiseRejectReason = 0;  // bit0=ratio, bit1=magnitude, bit2=jump
    bool freqBypassed = false;       // frame bypassed by frequency gate

    // ── CoorSpeedProcess outputs ──
    int32_t speedValue = 0;
    int32_t speedAvgDx = 0;
    int32_t speedAvgDy = 0;
    int32_t speedShortAvgDist = 0;
    int32_t speedFullAvgDist = 0;

    // ── CoorIIRProcess output ──
    uint16_t iirCoef = 0;
    bool iirFilterActive = false;

#if EGOTOUCH_DIAG
    float lfLineFitSlopeA = 0.f;
    float lfLineFitInterceptB = 0.f;
    bool lfLineFitValid = false;
    int32_t lfCos1000 = 0;
    int32_t lfStraightBufCount = 0;
    int32_t lfDragApplied = 0;
    bool coorReviseActive = false;
    int16_t coorReviseCorrectionDim1 = 0;
    int16_t coorReviseCorrectionDim2 = 0;

    // ── Hpp3NoiseProcess diagnostics ──
    bool noiseValidDim1 = true;
    bool noiseValidDim2 = true;
    uint8_t ratioAnomalyCntDim1 = 0;
    uint8_t ratioAnomalyCntDim2 = 0;
    uint32_t pressSigSumDim1 = 0;
    uint32_t pressSigSumDim2 = 0;
    uint16_t pressCnt = 0;
    uint32_t pressSigAvgDim1 = 0;
    uint32_t pressSigAvgDim2 = 0;
    int32_t coorJumpDim1 = 0;
    int32_t coorJumpDim2 = 0;

    // ── AftCoorProcess diagnostics ──
    bool lockActiveX = false;
    bool lockActiveY = false;
    int32_t lockOffsetX = 0;
    int32_t lockOffsetY = 0;
    int32_t lockThresholdX = 0;
    int32_t lockThresholdY = 0;
#endif
};

struct StylusRuntimeFrame {
    StylusRuntimeFlow flow{};
    StylusRuntimeParse parse{};
    StylusRuntimeRawGrid rawGrid{};
    StylusTxRuntime tx1{};
    StylusTxRuntime tx2{};
    StylusRuntimeSignal signal{};
    StylusRuntimeTilt tilt{};
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

        // ── GridFeatureExtractor ──
        uint16_t tx1PeakValue = 0;
        uint16_t tx1Sum3x3 = 0;
        uint16_t tx2PeakValue = 0;
        uint16_t tx2Sum3x3 = 0;
        bool tx2Valid = false;

        // ── CoordinateSolver ──
        uint16_t triDim1Left = 0;
        uint16_t triDim1Center = 0;
        uint16_t triDim1Right = 0;
        int16_t pitchCompApplied = 0;
        int32_t localCoorDim1 = 0;
        int32_t localCoorDim2 = 0;
        bool dim1Edge = false;
        bool dim2Edge = false;

        // ── TiltProcess ──
        uint16_t tiltLenLimit = 0;
        int32_t tiltRawDiffDim1 = 0;
        int32_t tiltRawDiffDim2 = 0;
        int16_t preTiltDim1 = 0;
        int16_t preTiltDim2 = 0;
        int16_t reportTiltDim1 = 0;
        int16_t reportTiltDim2 = 0;

        // ── PressureSolver ──
        uint16_t btRawPressure = 0;
        uint16_t preIirPressure = 0;
        bool btPressSuppressActive = false;
        uint8_t polySegment = 0;

        // ── PostPressure ──
        bool edgeSignalTooLowLatched = false;
        bool fakePressureDecreaseActive = false;
        uint8_t fakePressureDecreaseFramesLeft = 0;
        uint8_t btFreqShiftDebounceFramesLeft = 0;

        // ── LinearFilterProcess ──
        uint8_t lfStateMachine = 0;
        float lfLineFitSlopeA = 0.f;
        float lfLineFitInterceptB = 0.f;
        bool lfLineFitValid = false;
        int32_t lfCos1000 = 0;
        int32_t lfStraightBufCount = 0;
        int32_t lfDragApplied = 0;
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

    inline void SnapshotBtInput(uint16_t btPressure, uint32_t btSeq, bool hasBtSample) {
        input.btSample.pressure.fill(0);
        input.btSample.rawPressure.fill(0);
        input.btSample.pressure[3] = btPressure;
        input.btSample.seq = btSeq;
        input.btSample.freq1 = 0;
        input.btSample.freq2 = 0;
        input.btSample.hasSample = hasBtSample;
        input.btSample.hasFreq = false;
    }

    inline void SnapshotBtInput(const std::array<uint16_t, 4>& btPressure,
                                uint32_t btSeq,
                                bool hasBtSample) {
        input.btSample.pressure = btPressure;
        input.btSample.rawPressure.fill(0);
        input.btSample.seq = btSeq;
        input.btSample.freq1 = 0;
        input.btSample.freq2 = 0;
        input.btSample.hasSample = hasBtSample;
        input.btSample.hasFreq = false;
    }

    inline void ResetRuntime() {
        runtime.Reset();
    }
};

} // namespace Solvers
