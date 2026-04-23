#pragma once

#include "AsaTypes.hpp"
#include "CoorReviser.hpp"
#include "PenStateMachine.hpp"
#include "SolverTypes.h"
#include "LinearFilter.hpp"
#include "StylusFrameState.hpp"

#include <algorithm>

namespace Solvers {

struct StylusDiagnosticsContext {
    uint16_t anchorRow = 0;
    uint16_t anchorCol = 0;
    int anchorCenterOffset = 0;
    Asa::AsaCoorResult rawCoor{};
    Asa::AsaCoorResult postCoor{};
    Asa::AsaCoorResult finalCoor{};
    float pointX = 0.0f;
    float pointY = 0.0f;
    float speedInstant = 0.0f;
    float speedShortAvg = 0.0f;
    int iirCoef = 0;
    bool isEdge = false;
    float tiltDiffX = 0.0f;
    float tiltDiffY = 0.0f;
    uint16_t peakSignal = 0;
    uint16_t rawPressure = 0;
    uint16_t mappedPressure = 0;
    uint32_t btSeq = 0;
    uint8_t predictedAgeFrames = 0;
    bool pressureIsReal = false;
    uint8_t vhfPenState = 0;
    Asa::LinearFilter::Mode linearFilterMode = Asa::LinearFilter::Mode::Curve;
    uint16_t signalRatio = 0;
    bool cmfEnabled = false;
    bool coorReviserActive = false;
    float coorRevDeltaX = 0.0f;
    float coorRevDeltaY = 0.0f;
    uint8_t penLifecycle = 0;
    bool wasInking = false;
};

struct StylusDiagnosticsExtras {
    float speedInstant = 0.0f;
    float speedShortAvg = 0.0f;
    Asa::LinearFilter::Mode linearFilterMode = Asa::LinearFilter::Mode::Curve;
    uint16_t signalRatio = 0;
    bool cmfEnabled = false;
    bool coorReviserEnabled = false;
    float tiltDiffX = 0.0f;
    float tiltDiffY = 0.0f;
    float coorRevDeltaX = 0.0f;
    float coorRevDeltaY = 0.0f;
    uint8_t vhfPenState = 0;
    uint8_t penLifecycle = 0;
    bool wasInking = false;
};

class StylusDiagnosticsWriter {
public:
    inline void Process(const StylusDiagnosticsContext& ctx) {
        Write(ctx);
    }

    inline void Process(const StylusFrameState& state,
                        const StylusDiagnosticsExtras& extras = {}) {
        Write(BuildContext(state, extras));
    }

    inline void Process(const StylusFrameState& state,
                        const Asa::PenStateMachine& penStateMachine,
                        Asa::LinearFilter::Mode linearFilterMode,
                        uint16_t signalRatio,
                        bool cmfEnabled,
                        const Asa::CoorReviser& coorReviser) {
        StylusDiagnosticsExtras extras = BuildExtras(
            state,
            penStateMachine,
            linearFilterMode,
            signalRatio,
            cmfEnabled,
            coorReviser);
        Process(state, extras);
    }

    inline void Process(const StylusFrameState& state,
                        const Asa::PenStateMachine& penStateMachine,
                        const Asa::LinearFilter& linearFilter,
                        uint16_t signalRatio,
                        bool cmfEnabled,
                        const Asa::CoorReviser& coorReviser) {
        StylusDiagnosticsExtras extras = BuildExtras(
            state,
            penStateMachine,
            linearFilter.GetMode(),
            signalRatio,
            cmfEnabled,
            coorReviser);
        extras.vhfPenState = ExtractVhfPenState(state);
        Process(state, extras);
    }

    inline void Process(uint8_t vhfPenState, Asa::LinearFilter::Mode linearFilterMode) {
        SetOutputState(vhfPenState, linearFilterMode);
    }

    inline void Reset() {
        m_diag = StylusFrameData::StylusDiagnostics{};
    }

    static inline StylusDiagnosticsContext BuildContext(
            const StylusFrameState& state,
            const StylusDiagnosticsExtras& extras = {}) {
        StylusDiagnosticsContext ctx{};
        ctx.anchorRow = state.parse.gridData.tx1.anchorRow;
        ctx.anchorCol = state.parse.gridData.tx1.anchorCol;
        ctx.anchorCenterOffset = state.anchorCenterOffset;
        ctx.rawCoor = state.tx1.globalCoor;
        ctx.postCoor = state.output.postCoor;
        ctx.finalCoor = state.output.finalCoor;
        ctx.pointX = static_cast<float>(state.output.finalCoor.dim1);
        ctx.pointY = static_cast<float>(state.output.finalCoor.dim2);
        ctx.speedInstant = extras.speedInstant;
        ctx.speedShortAvg = extras.speedShortAvg;
        ctx.iirCoef = state.lifecycle.iirCoef;
        ctx.isEdge = state.signal.dim1EdgeActive || state.signal.dim2EdgeActive;
        ctx.tiltDiffX = extras.tiltDiffX;
        ctx.tiltDiffY = extras.tiltDiffY;
        ctx.peakSignal = state.signal.maxRawPeak;
        ctx.rawPressure = state.lifecycle.btSample.pressure;
        ctx.mappedPressure = state.lifecycle.mappedPressure;
        ctx.btSeq = state.lifecycle.btSeq;
        ctx.predictedAgeFrames = static_cast<uint8_t>(std::clamp(state.lifecycle.predictedAgeFrames, 0, 0xFF));
        ctx.pressureIsReal = state.lifecycle.pressureIsReal;
        ctx.vhfPenState = extras.vhfPenState;
        ctx.linearFilterMode = extras.linearFilterMode;
        ctx.signalRatio = extras.signalRatio;
        ctx.cmfEnabled = extras.cmfEnabled;
        ctx.coorReviserActive = extras.coorReviserEnabled;
        ctx.coorRevDeltaX = extras.coorRevDeltaX;
        ctx.coorRevDeltaY = extras.coorRevDeltaY;
        ctx.penLifecycle = extras.penLifecycle;
        ctx.wasInking = extras.wasInking;
        return ctx;
    }

    static inline StylusDiagnosticsExtras BuildExtras(
            const StylusFrameState& state,
            const Asa::PenStateMachine& penStateMachine,
            Asa::LinearFilter::Mode linearFilterMode,
            uint16_t signalRatio,
            bool cmfEnabled,
            const Asa::CoorReviser& coorReviser) {
        StylusDiagnosticsExtras extras{};
        extras.speedInstant = penStateMachine.GetInstantSpeed();
        extras.speedShortAvg = penStateMachine.GetSmoothedSpeed();
        extras.linearFilterMode = linearFilterMode;
        extras.signalRatio = signalRatio;
        extras.cmfEnabled = cmfEnabled;
        extras.coorReviserEnabled = coorReviser.enabled;
        extras.tiltDiffX = static_cast<float>(coorReviser.GetLastTiltX());
        extras.tiltDiffY = static_cast<float>(coorReviser.GetLastTiltY());
        extras.coorRevDeltaX = static_cast<float>(coorReviser.GetLastReviseX());
        extras.coorRevDeltaY = static_cast<float>(coorReviser.GetLastReviseY());
        extras.penLifecycle = static_cast<uint8_t>(penStateMachine.GetState());
        extras.wasInking = state.lifecycle.tipSwitchActive;
        return extras;
    }

    static inline uint8_t ExtractVhfPenState(const StylusFrameState& state) {
        return state.stylus.diag.vhfPenState;
    }

    inline void Write(const StylusDiagnosticsContext& ctx) {
        m_diag = StylusFrameData::StylusDiagnostics{};
        m_diag.anchorRow = ctx.anchorRow;
        m_diag.anchorCol = ctx.anchorCol;
        m_diag.rawDim1 = ctx.rawCoor.dim1;
        m_diag.rawDim2 = ctx.rawCoor.dim2;
        m_diag.finalDim1 = ctx.finalCoor.dim1;
        m_diag.finalDim2 = ctx.finalCoor.dim2;
        m_diag.centerOff = static_cast<float>(ctx.anchorCenterOffset * Asa::kCoorUnit);
        m_diag.pointX = ctx.pointX;
        m_diag.pointY = ctx.pointY;
        m_diag.valid = ctx.finalCoor.valid;
        m_diag.speedInstant = ctx.speedInstant;
        m_diag.speedShortAvg = ctx.speedShortAvg;
        m_diag.iirCoef = static_cast<float>(ctx.iirCoef);
        m_diag.isHover = (ctx.mappedPressure == 0);
        m_diag.isEdge = ctx.isEdge;
        m_diag.tiltDiffX = ctx.tiltDiffX;
        m_diag.tiltDiffY = ctx.tiltDiffY;
        m_diag.peakSignal = ctx.peakSignal;
        m_diag.rawPressure = ctx.rawPressure;
        m_diag.mappedPressure = ctx.mappedPressure;
        m_diag.btSeq = ctx.btSeq;
        m_diag.predictedAgeFrames = ctx.predictedAgeFrames;
        m_diag.pressureIsReal = ctx.pressureIsReal;
        m_diag.vhfPenState = ctx.vhfPenState;
        m_diag.linearFilterState = static_cast<uint8_t>(ctx.linearFilterMode);
        m_diag.signalRatio = ctx.signalRatio;
        m_diag.cmfEnabled = ctx.cmfEnabled;
        m_diag.coorReviserActive = ctx.coorReviserActive;
        m_diag.coorRevDeltaX = ctx.coorRevDeltaX;
        m_diag.coorRevDeltaY = ctx.coorRevDeltaY;
        m_diag.penLifecycle = ctx.penLifecycle;
        m_diag.wasInking = ctx.wasInking;
        m_diag.avg3PtDim1 = ctx.postCoor.dim1;
        m_diag.avg3PtDim2 = ctx.postCoor.dim2;
    }

    inline const StylusFrameData::StylusDiagnostics& GetLastDiagnostics() const {
        return m_diag;
    }

    inline void SetOutputState(uint8_t vhfPenState, Asa::LinearFilter::Mode linearFilterMode) {
        m_diag.vhfPenState = vhfPenState;
        m_diag.linearFilterState = static_cast<uint8_t>(linearFilterMode);
    }

private:
    StylusFrameData::StylusDiagnostics m_diag{};
};

} // namespace Solvers
