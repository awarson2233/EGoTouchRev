#pragma once
#include "AsaTypes.hpp"
#include "PenStateMachine.hpp"
#include "PressureSolver.hpp"
#include "BtPressBuffer.hpp"
#include "StylusFrameState.hpp"
#include "StylusSignalAnalyzer.hpp"
#include <algorithm>
#include <cstdint>

namespace Asa {

struct StylusStateInput {
    StylusSignalMetrics metrics{};
    AsaCoorResult rawCoor{};
    bool tx1BlockValid = false;
    BtPressureSample btSample{};
    bool recheckPassed = false;
    int tx1InkEnterThreshold = 0;
    int tx1LiftSuspiciousThreshold = 0;
    int tx1LiftAbsoluteThreshold = 0;
    bool hasLastGoodFrame = false;
    bool previouslyWriting = false;
};

struct StylusStateOutput {
    bool valid = false;
    bool suspiciousDrop = false;
    bool hoverPresent = false;
    bool authoritativeDown = false;
    bool keepInkAlive = false;
    bool immediateRelease = false;
    bool recheckPassed = false;
    bool keepPreviousCoordinate = false;
    bool keepInRangeOnReleaseFrame = false;
    bool applyExitEdgeSnap = false;

    // Output mapped by state machine
    uint16_t mappedPressure = 0;
    uint16_t outputPressure = 0;
    bool tipSwitchActive = false;
    
    // For passing up to frame diagnostic/data
    bool pressureGateOpen = false;
    uint32_t btSeq = 0;
    bool pressureIsReal = false;
    int predictedAgeFrames = 0;
    
    MotionProfile motion{};
};

struct StylusStateControllerConfig {
    int tx1InkEnterThreshold = 0;
    int tx1LiftSuspiciousThreshold = 0;
    int tx1LiftAbsoluteThreshold = 0;
    bool hasCommittedFrame = false;
};

struct StylusStateControllerResult {
    StylusStateOutput stateOutput{};
    PressureStageResult pressureStage{};
    PenFrameEvidence evidence{};
    PenUpdateResult penUpdate{};
};

class StylusStateController {
public:
    StylusStateController() = default;

    int tx1InkEnterThreshold = 9000;
    int tx1LiftSuspiciousThreshold = 7000;
    int tx1LiftAbsoluteThreshold = 4500;

    inline void ClosePressureGate(uint32_t latestBtSeq, PressureSolver& solver) {
        solver.Reset();
        m_pressureGateOpen = false;
        m_waitingFreshBtAfterRelease = true;
        m_blockedBtSeqFloor = std::max(m_blockedBtSeqFloor, latestBtSeq);
    }

    inline void TrackBlockedBtSeq(uint32_t latestBtSeq) {
        if (!m_waitingFreshBtAfterRelease) {
            return;
        }
        m_blockedBtSeqFloor = std::max(m_blockedBtSeqFloor, latestBtSeq);
    }

    inline void ResetFallback(uint32_t latestBtSeq, PressureSolver& pSolver, PenStateMachine& sm) {
        if (sm.GetState() == PenStateMachine::State::Writing) {
            ClosePressureGate(latestBtSeq, pSolver);
        } else {
            TrackBlockedBtSeq(latestBtSeq);
            pSolver.Reset();
            m_pressureGateOpen = false;
        }
    }

    inline StylusStateOutput Process(const StylusStateInput& input, PressureSolver& pressureSolver) {
        StylusStateOutput out{};
        out.recheckPassed = input.recheckPassed;

        const bool baseValid = input.rawCoor.valid && input.tx1BlockValid && out.recheckPassed;
        out.hoverPresent = baseValid && (input.metrics.tx1Composite >= input.tx1LiftAbsoluteThreshold);
        out.authoritativeDown = baseValid && (input.metrics.tx1Composite >= input.tx1InkEnterThreshold);
        out.keepInkAlive = baseValid && (input.metrics.tx1Composite >= input.tx1LiftSuspiciousThreshold);
        
        out.suspiciousDrop = input.previouslyWriting && baseValid &&
                             (input.metrics.tx1Composite >= input.tx1LiftAbsoluteThreshold) &&
                             (input.metrics.tx1Composite < input.tx1LiftSuspiciousThreshold);
        
        out.immediateRelease = input.previouslyWriting && !out.suspiciousDrop &&
                               (!baseValid || (input.metrics.tx1Composite < input.tx1LiftAbsoluteThreshold));

        if (out.suspiciousDrop && input.hasLastGoodFrame) {
            TrackBlockedBtSeq(input.btSample.seq);
            return out; 
        }

        if (!out.authoritativeDown) {
            TrackBlockedBtSeq(input.btSample.seq);
        }
        if (out.immediateRelease) {
            ClosePressureGate(input.btSample.seq, pressureSolver);
        }

        if (out.authoritativeDown && !m_pressureGateOpen) {
            if (m_waitingFreshBtAfterRelease) {
                if (input.btSample.hasSample && input.btSample.seq > m_blockedBtSeqFloor) {
                    m_pressureGateOpen = true;
                    m_waitingFreshBtAfterRelease = false;
                    m_blockedBtSeqFloor = input.btSample.seq;
                }
            } else {
                m_pressureGateOpen = input.btSample.hasSample;
            }
        }

        out.pressureGateOpen = m_pressureGateOpen;
        return out;
    }

    inline StylusStateControllerResult Process(Solvers::StylusFrameState& state,
                                               const StylusStateControllerConfig& config,
                                               PressureSolver& pressureSolver,
                                               PenStateMachine& penStateMachine) {
        StylusStateControllerResult result{};
        state.flow.reusedCommittedFrame = false;
        state.flow.resetNoise = false;
        state.lifecycle.previouslyWriting =
            (penStateMachine.GetState() == PenStateMachine::State::Writing);

        const auto gateOutput = Process(
            BuildStateInput(state, config),
            pressureSolver);
        ApplyLifecycleState(state, gateOutput);
        result.stateOutput = BuildStateOutput(state);

        if (gateOutput.suspiciousDrop && config.hasCommittedFrame) {
            state.flow.reusedCommittedFrame = true;
            state.lifecycle.currentlyWriting = state.lifecycle.previouslyWriting;
            result.stateOutput = BuildStateOutput(state);
            return result;
        }

        result.pressureStage = pressureSolver.Process(state, gateOutput.pressureGateOpen);
        result.evidence = BuildEvidence(state, gateOutput, result.pressureStage);
        result.penUpdate = penStateMachine.Process(state, result.evidence);
        ApplyStylusStateMirrors(state, result.penUpdate, penStateMachine);
        state.lifecycle.pressureGateOpen = IsPressureGateOpen();
        if (penStateMachine.JustLeftRange()) {
            ClosePressureGate(state.lifecycle.btSample.seq, pressureSolver);
            state.lifecycle.pressureGateOpen = false;
            state.flow.resetNoise = true;
        }
        result.stateOutput = BuildStateOutput(state, &result.penUpdate.motion);
        result.stateOutput.valid = true;
        return result;
    }

    inline StylusStateControllerResult Process(Solvers::StylusFrameState& state,
                                               bool hasCommittedFrame,
                                               PressureSolver& pressureSolver,
                                               PenStateMachine& penStateMachine) {
        StylusStateControllerResult result{};
        state.flow.reusedCommittedFrame = false;
        state.flow.resetNoise = false;
        state.lifecycle.previouslyWriting =
            (penStateMachine.GetState() == PenStateMachine::State::Writing);

        const auto gateOutput = Process(
            BuildStateInput(state, hasCommittedFrame, state.lifecycle.previouslyWriting, *this),
            pressureSolver);
        ApplyLifecycleState(state, gateOutput);
        result.stateOutput = BuildStateOutput(state);

        if (gateOutput.suspiciousDrop && hasCommittedFrame) {
            state.flow.reusedCommittedFrame = true;
            state.lifecycle.currentlyWriting = state.lifecycle.previouslyWriting;
            result.stateOutput = BuildStateOutput(state);
            return result;
        }

        result.pressureStage = pressureSolver.Process(state, gateOutput.pressureGateOpen);
        result.evidence = BuildEvidence(state, gateOutput, result.pressureStage);
        result.penUpdate = penStateMachine.Process(state, result.evidence);
        ApplyStylusStateMirrors(state, result.penUpdate, penStateMachine);
        state.lifecycle.pressureGateOpen = IsPressureGateOpen();
        if (penStateMachine.JustLeftRange()) {
            ClosePressureGate(state.lifecycle.btSample.seq, pressureSolver);
            state.lifecycle.pressureGateOpen = false;
            state.flow.resetNoise = true;
        }
        result.stateOutput = BuildStateOutput(state, &result.penUpdate.motion);
        result.stateOutput.valid = true;
        return result;
    }

    inline bool IsPressureGateOpen() const {
        return m_pressureGateOpen;
    }

    static inline void ApplyStylusStateMirrors(
        Solvers::StylusFrameState& state,
        const PenUpdateResult& penUpdate,
        const PenStateMachine& penStateMachine) {
        auto& stylus = state.stylus;
        stylus.point.rawPressure = state.lifecycle.btSample.pressure;
        stylus.point.mappedPressure = state.lifecycle.mappedPressure;
        stylus.pressure = penUpdate.output.outputPressure;
        stylus.point.pressure = stylus.pressure;
        stylus.noPressInkActive = penUpdate.output.noPressInkActive;
        stylus.tipSwitchActive = penUpdate.output.tipSwitchActive;
        stylus.sustainOutput = penUpdate.output.sustainOutput;
        stylus.fastLiftOutput = penUpdate.output.fastLiftOutput;
        stylus.animState = penStateMachine.GetAnimState();
    }

    static inline void ApplyTerminalStylusStateMirrors(
        Solvers::StylusFrameState& state,
        const PenStateMachine& penStateMachine) {
        state.stylus.animState = penStateMachine.GetAnimState();
    }

private:
    bool m_pressureGateOpen = false;
    bool m_waitingFreshBtAfterRelease = false;
    uint32_t m_blockedBtSeqFloor = 0;

    static inline StylusSignalMetrics BuildSignalMetrics(const Solvers::StylusFrameState& state) {
        StylusSignalMetrics metrics{};
        metrics.signalX = state.signal.signalX;
        metrics.signalY = state.signal.signalY;
        metrics.maxRawPeak = state.signal.maxRawPeak;
        metrics.tx1Composite = state.signal.tx1Composite;
        metrics.tx2Composite = state.signal.tx2Composite;
        metrics.dim1EdgeActive = state.signal.dim1EdgeActive;
        metrics.dim2EdgeActive = state.signal.dim2EdgeActive;
        metrics.dim1EdgeSignal = state.signal.dim1EdgeSignal;
        metrics.dim2EdgeSignal = state.signal.dim2EdgeSignal;
        return metrics;
    }

    static inline StylusStateInput BuildStateInput(const Solvers::StylusFrameState& state,
                                                   const StylusStateControllerConfig& config) {
        StylusStateInput input{};
        input.metrics = BuildSignalMetrics(state);
        input.rawCoor = state.tx1.globalCoor;
        input.tx1BlockValid = state.parse.gridData.tx1.valid;
        input.btSample = state.lifecycle.btSample;
        input.recheckPassed = state.signal.recheckPassed;
        input.tx1InkEnterThreshold = config.tx1InkEnterThreshold;
        input.tx1LiftSuspiciousThreshold = config.tx1LiftSuspiciousThreshold;
        input.tx1LiftAbsoluteThreshold = config.tx1LiftAbsoluteThreshold;
        input.hasLastGoodFrame = config.hasCommittedFrame;
        input.previouslyWriting = state.lifecycle.previouslyWriting;
        return input;
    }

    static inline StylusStateInput BuildStateInput(const Solvers::StylusFrameState& state,
                                                   bool hasCommittedFrame,
                                                   bool previouslyWriting,
                                                   const StylusStateController& controller) {
        StylusStateInput input{};
        input.metrics = BuildSignalMetrics(state);
        input.rawCoor = state.tx1.globalCoor;
        input.tx1BlockValid = state.parse.gridData.tx1.valid;
        input.btSample = state.lifecycle.btSample;
        input.recheckPassed = state.signal.recheckPassed;
        input.tx1InkEnterThreshold = controller.tx1InkEnterThreshold;
        input.tx1LiftSuspiciousThreshold = controller.tx1LiftSuspiciousThreshold;
        input.tx1LiftAbsoluteThreshold = controller.tx1LiftAbsoluteThreshold;
        input.hasLastGoodFrame = hasCommittedFrame;
        input.previouslyWriting = previouslyWriting;
        return input;
    }

    static inline void ApplyLifecycleState(Solvers::StylusFrameState& state,
                                           const StylusStateOutput& out) {
        state.signal.recheckPassed = out.recheckPassed;
        state.lifecycle.suspiciousDrop = out.suspiciousDrop;
        state.lifecycle.hoverPresent = out.hoverPresent;
        state.lifecycle.authoritativeDown = out.authoritativeDown;
        state.lifecycle.keepInkAlive = out.keepInkAlive;
        state.lifecycle.immediateRelease = out.immediateRelease;
        state.lifecycle.pressureGateOpen = out.pressureGateOpen;
    }

    static inline MotionProfile BuildMotionProfile(const Solvers::StylusFrameState& state) {
        MotionProfile motion{};
        motion.iirCoef = state.lifecycle.iirCoef;
        motion.iirDivisorN = state.lifecycle.iirDivisorN;
        motion.skipIIR = state.lifecycle.skipIIR;
        motion.jitterStrength = state.lifecycle.jitterStrength;
        motion.enableLinearFilter = state.lifecycle.enableLinearFilter;
        motion.enableCoorReviser = state.lifecycle.enableCoorReviser;
        return motion;
    }

    static inline StylusStateOutput BuildStateOutput(const Solvers::StylusFrameState& state,
                                                     const MotionProfile* motionOverride = nullptr) {
        StylusStateOutput out{};
        out.recheckPassed = state.signal.recheckPassed;
        out.suspiciousDrop = state.lifecycle.suspiciousDrop;
        out.hoverPresent = state.lifecycle.hoverPresent;
        out.authoritativeDown = state.lifecycle.authoritativeDown;
        out.keepInkAlive = state.lifecycle.keepInkAlive;
        out.immediateRelease = state.lifecycle.immediateRelease;
        out.keepPreviousCoordinate = state.lifecycle.keepPreviousCoordinate;
        out.keepInRangeOnReleaseFrame = state.lifecycle.keepInRangeOnReleaseFrame;
        out.applyExitEdgeSnap = state.lifecycle.applyExitEdgeSnap;
        out.mappedPressure = state.lifecycle.mappedPressure;
        out.outputPressure = state.lifecycle.outputPressure;
        out.tipSwitchActive = state.lifecycle.tipSwitchActive;
        out.pressureGateOpen = state.lifecycle.pressureGateOpen;
        out.btSeq = state.lifecycle.btSeq;
        out.pressureIsReal = state.lifecycle.pressureIsReal;
        out.predictedAgeFrames = state.lifecycle.predictedAgeFrames;
        out.motion = motionOverride ? *motionOverride : BuildMotionProfile(state);
        return out;
    }

    static inline PenFrameEvidence BuildEvidence(
        const Solvers::StylusFrameState& state,
        const StylusStateOutput& stateOutput,
        const PressureStageResult& pressureStage) {
        PenFrameEvidence evidence{};
        evidence.coordValid = state.tx1.globalCoor.valid;
        evidence.noSignal = false;
        evidence.tx1BlockValid = state.parse.gridData.tx1.valid;
        evidence.sustainActive = false;
        evidence.activeStylusPresent = stateOutput.hoverPresent;
        evidence.hoverSignalPresent = stateOutput.hoverPresent;
        evidence.authoritativeDown = stateOutput.authoritativeDown;
        evidence.keepInkAlive = stateOutput.keepInkAlive;
        evidence.hoverPresent = stateOutput.hoverPresent && !stateOutput.keepInkAlive;
        evidence.immediateRelease = stateOutput.immediateRelease;
        evidence.recheckPassed = stateOutput.recheckPassed;
        evidence.overlapLike = state.signal.overlapLike;
        evidence.edgeLike = state.signal.dim1EdgeActive || state.signal.dim2EdgeActive;
        evidence.exitSmoothCandidate = false;
        evidence.suppressPressureButKeepContact = false;
        evidence.btPressureResidual = false;
        evidence.edgeSignalLow = false;
        evidence.pressureIsReal = pressureStage.isRealMeasurement;
        evidence.mappedPressure = pressureStage.mappedPressure;
        evidence.realPressure = pressureStage.realPressure;
        evidence.realMeasuredPressure =
            pressureStage.isRealMeasurement ? pressureStage.mappedPressure : 0;
        evidence.pressureForContact = pressureStage.realPressure;
        evidence.tx1Composite = state.signal.tx1Composite;
        evidence.tx2Composite = state.signal.tx2Composite;
        evidence.curDim1 = state.tx1.globalCoor.dim1;
        evidence.curDim2 = state.tx1.globalCoor.dim2;
        return evidence;
    }
};

} // namespace Asa
