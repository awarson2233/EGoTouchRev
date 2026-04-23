#pragma once

#include "AsaTypes.hpp"
#include "BtPressBuffer.hpp"
#include "SolverTypes.h"

#include <array>
#include <cstdint>

namespace Asa {

enum class StylusFrameClass : uint8_t {
    Valid,
    ShortFrame,
    NoSignal,
    ParseFail,
    Tx1Missing,
};

} // namespace Asa

namespace Solvers {

struct StylusFlowState {
    bool terminal = false;
    bool reusedCommittedFrame = false;
    bool clearCommitted = false;
    bool resetPost = false;
    bool resetNoise = false;
    uint8_t pipelineStage = 0;
    StylusPacketRoute packetRoute = StylusPacketRoute::Valid;
};

struct StylusParseState {
    Asa::StylusFrameClass frameClass = Asa::StylusFrameClass::ShortFrame;
    bool valid = false;
    bool slaveValid = false;
    bool isFullFrame = false;
    bool checksumFailed = false;
    bool hasCurrentStylusSignal = false;
    uint16_t status = 0;
    uint16_t checksumValue = 0;
    std::array<uint8_t, Asa::kSlaveHeaderBytes> rawSlaveHdr{};
    Asa::AsaGridData gridData{};
};

struct StylusProjectionState {
    int16_t grid[Asa::kGridDim][Asa::kGridDim]{};
    Asa::GridPeakUnit peak{};
    Asa::AsaProjection projection{};
    uint16_t peakSignal = 0;
    Asa::AsaCoorResult localCoor{};
    Asa::AsaCoorResult globalCoor{};
};

struct StylusSignalState {
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
    uint16_t recheckThreshold = 0;
    uint16_t recheckThresholdMulti = 0;
    bool recheckPassed = false;
};

struct StylusLifecycleState {
    Asa::BtPressureSample btSample{};
    bool previouslyWriting = false;
    bool currentlyWriting = false;
    bool suspiciousDrop = false;
    bool hoverPresent = false;
    bool authoritativeDown = false;
    bool keepInkAlive = false;
    bool immediateRelease = false;
    bool pressureGateOpen = false;
    bool pressureIsReal = false;
    uint16_t mappedPressure = 0;
    uint16_t realPressure = 0;
    uint16_t outputPressure = 0;
    bool tipSwitchActive = false;
    uint32_t btSeq = 0;
    int predictedAgeFrames = 0;
    bool keepPreviousCoordinate = false;
    bool keepInRangeOnReleaseFrame = false;
    bool applyExitEdgeSnap = false;
    bool enableLinearFilter = false;
    bool enableCoorReviser = false;
    int iirCoef = 0;
    int iirDivisorN = 0;
    bool skipIIR = false;
    int jitterStrength = 0;
};

struct StylusOutputState {
    Asa::AsaCoorResult postCoor{};
    Asa::AsaCoorResult finalCoor{};
    int16_t tiltX = 0;
    int16_t tiltY = 0;
};

struct StylusFrameState {
    StylusFrameState(HeatmapFrame& frameIn, int sensorRowsIn, int sensorColsIn, int anchorCenterOffsetIn)
        : frame(frameIn)
        , stylus(frameIn.stylus)
        , sensorRows(sensorRowsIn)
        , sensorCols(sensorColsIn)
        , anchorCenterOffset(anchorCenterOffsetIn) {}

    HeatmapFrame& frame;
    StylusFrameData& stylus;
    int sensorRows = 0;
    int sensorCols = 0;
    int anchorCenterOffset = 0;

    StylusFlowState flow{};
    StylusParseState parse{};
    StylusProjectionState tx1{};
    StylusProjectionState tx2{};
    StylusSignalState signal{};
    StylusLifecycleState lifecycle{};
    StylusOutputState output{};
};

} // namespace Solvers
