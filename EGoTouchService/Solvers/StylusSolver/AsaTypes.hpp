#pragma once

#ifndef EGOTOUCH_SOLVERS_STYLUSSOLVER_ASATYPES_HPP
#define EGOTOUCH_SOLVERS_STYLUSSOLVER_ASATYPES_HPP

#include "SolverBuildConfig.h"

#include <array>
#include <cstdint>

namespace Solvers {
struct HeatmapFrame;
}

namespace Asa {

enum class FrameClass : uint8_t {
    Valid,
    ShortFrame,
    NoSignal,
    ParseFail,
    Tx1Missing,
};

static constexpr int kCoorUnit = 0x400;
static constexpr int kMaxSensorDim = 80;

struct CoorResult {
    int32_t dim1 = 0;
    int32_t dim2 = 0;
    bool valid = false;
};

struct CoordinateResult {
    CoorResult localGridCoor{};
    CoorResult reportGlobalCoor{};
};

struct SolvePoint {
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

struct BtInputSnapshot {
    std::array<uint16_t, 4> pressure{};
    std::array<uint16_t, 4> rawPressure{};
    uint32_t seq = 0;
    uint8_t freq1 = 0;
    uint8_t freq2 = 0;
    bool hasSample = false;
    bool hasFreq = false;
};

struct TxRuntime {
    CoordinateResult coordinate{};
};

struct FlowRuntime {
    bool terminal = false;
    bool resetPost = false;
    bool resetNoise = false;
    uint8_t pipelineStage = 0;
    FrameClass frameClass = FrameClass::ShortFrame;
};

struct ParseRuntime {
    bool valid = false;
    bool slaveValid = false;
    bool checksumOk = false;
    bool isFullFrame = false;
    bool hasCurrentStylusSignal = false;
    uint32_t status = 0;
    uint16_t checksum16 = 0;
};

struct SignalRuntime {
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

struct TiltRuntime {
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

struct PressureRuntime {
    BtInputSnapshot btSample{};
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

struct DecisionRuntime {
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

struct PostRuntime {
    CoorResult postCoor{};
    CoorResult finalCoor{};
    CoorResult edgePostCoor{};
    CoorResult postIirCoor{};
    CoorResult predictedCoor{};
    SolvePoint point{};
    bool finalValid = false;
    uint16_t finalPressure = 0;
    float confidence = 0.0f;
    uint8_t linearFilterState = 0;
    bool linearFilterActive = false;
    int32_t linearFilterDeltaDim1 = 0;
    int32_t linearFilterDeltaDim2 = 0;

    // ── Protocol/common post outputs ──
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

struct Runtime {
    FlowRuntime flow{};
    ParseRuntime parse{};
    TxRuntime tx1{};
    TxRuntime tx2{};
    SignalRuntime signal{};
    TiltRuntime tilt{};
    PressureRuntime pressure{};
    DecisionRuntime decision{};
    PostRuntime post{};

    void ResetFrameFlags() {
        flow = {};
        parse = {};
        decision = {};
        signal = {};
    }

    void ResetPostOutputs() {
        tilt = {};
        pressure = {};
        post = {};
    }

#if EGOTOUCH_DIAG
    void ResetDiagnosticFields() {
        tx1 = {};
        tx2 = {};
    }
#endif

    void Reset() {
        *this = {};
    }
};

struct Context {
    Solvers::HeatmapFrame& frame;
    Runtime& runtime;
};

inline uint16_t ReadLe16(const uint8_t* ptr) {
    return static_cast<uint16_t>(
        static_cast<uint16_t>(ptr[0]) |
        (static_cast<uint16_t>(ptr[1]) << 8));
}

inline int32_t SensorPitchSizeMap(int32_t localCoor,
                                  const double* pitchTable,
                                  int coorUnit = kCoorUnit) {
    if (pitchTable[0] == 100.0) {
        return localCoor;
    }
    if (coorUnit == 0) {
        return 0;
    }

    const int cellIdx = localCoor / coorUnit;
    const int frac = localCoor % coorUnit;
    if (cellIdx < 0 || cellIdx >= kMaxSensorDim - 1) {
        return 0;
    }

    const double result =
        pitchTable[cellIdx + 1] * static_cast<double>(frac) +
        pitchTable[cellIdx] * static_cast<double>(coorUnit - frac);
    return static_cast<int32_t>(result);
}

} // namespace Asa

#endif // EGOTOUCH_SOLVERS_STYLUSSOLVER_ASATYPES_HPP
