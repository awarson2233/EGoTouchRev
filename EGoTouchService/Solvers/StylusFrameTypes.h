#pragma once

#include <array>
#include <cstdint>

#include "SolverBuildConfig.h"
#include "StylusSolver/AsaTypes.hpp"
#include "StylusSolver/hpp2/Hpp2Runtime.hpp"
#include "StylusSolver/hpp3/Hpp3Runtime.hpp"

namespace Solvers {

struct StylusPacket {
    bool valid = false;
    uint8_t reportId = 0x08;
    uint8_t length = 17;
    std::array<uint8_t, 17> bytes{};
};

struct StylusInputSnapshot {
    bool slaveValid = false;
    bool checksumOk = false;
    uint8_t slaveWordOffset = 0;
    uint16_t checksum16 = 0;
    bool tx1BlockValid = false;
    bool tx2BlockValid = false;
    uint32_t status = 0;

    // HPP2 protocol/scalar fields mirror TSACore stylusFrame. They are zero
    // until the frame bridge/parser starts exposing line-mode stylus payloads.
    uint32_t auxStatusFlags = 0;
    uint16_t mainFreq = 0;
    uint16_t auxFreq = 0;
    uint16_t framePressure = 0;
    uint32_t buttonBits = 0;
    bool hpp2LineValid = false;
    std::array<uint16_t, 100> hpp2LineData{}; // 60 TX + 40 RX for current preset

    Asa::BtInputSnapshot btSample{};
};

struct StylusOutputState {
    bool valid = false;
    bool inRange = false;
    bool tipDown = false;
    bool buttonActive = false;
    uint16_t pressure = 0;
    float confidence = 0.0f;
    uint8_t pipelineStage = 0;
    Asa::SolvePoint point{};
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

struct StylusRuntime {
    enum class Protocol : uint8_t {
        None,
        Hpp2,
        Hpp3,
    };

    Protocol activeProtocol = Protocol::None;
    Stylus::Hpp2::Runtime hpp2{};
    Stylus::Hpp3::Runtime hpp3{};

    Asa::Runtime& Active() {
        return activeProtocol == Protocol::Hpp2 ? static_cast<Asa::Runtime&>(hpp2)
                                                : static_cast<Asa::Runtime&>(hpp3);
    }

    const Asa::Runtime& Active() const {
        return activeProtocol == Protocol::Hpp2 ? static_cast<const Asa::Runtime&>(hpp2)
                                                : static_cast<const Asa::Runtime&>(hpp3);
    }

    Stylus::Hpp2::Runtime& SelectHpp2() {
        activeProtocol = Protocol::Hpp2;
        return hpp2;
    }

    Stylus::Hpp3::Runtime& SelectHpp3() {
        activeProtocol = Protocol::Hpp3;
        return hpp3;
    }

    void ResetFrameFlags() {
        activeProtocol = Protocol::None;
        hpp2.ResetFrameFlags();
        hpp3.ResetFrameFlags();
    }

    void ResetPostOutputs() {
        hpp2.ResetPostOutputs();
        hpp3.ResetPostOutputs();
    }

#if EGOTOUCH_DIAG
    void ResetDiagnosticFields() {
        hpp2.ResetDiagnosticFields();
        hpp3.ResetDiagnosticFields();
    }
#endif

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
    StylusRuntime runtime{};
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

    inline void ResetPerFrameState() {
        runtime.ResetFrameFlags();
        runtime.ResetPostOutputs();
    }
};

} // namespace Solvers
