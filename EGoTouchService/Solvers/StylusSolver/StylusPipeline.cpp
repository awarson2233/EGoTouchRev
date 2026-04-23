#include "StylusPipeline.h"

#include <algorithm>
#include <ostream>

namespace Solvers {

class StylusPipeline::OutputState {
public:
    inline void BeginFrame(StylusFrameData& frame) {
        frame = StylusFrameData{};
        m_lastResult = frame;
    }

    inline void BeginFrame(StylusFrameState& state) {
        BeginFrame(state.stylus);
        state.flow = {};
        state.parse = {};
        state.tx1 = {};
        state.tx2 = {};
        state.signal = {};
        state.lifecycle = {};
        state.output = {};
    }

    inline void CommitInvalid(StylusFrameData& frame, bool clearCommitted) {
        m_lastResult = frame;
        if (clearCommitted) {
            m_committedFrame = StylusFrameData{};
            m_hasCommittedFrame = false;
            m_previousPointX = 0.0f;
            m_previousPointY = 0.0f;
        }
    }

    inline void CommitFinal(StylusFrameData& frame) {
        m_lastResult = frame;
        m_committedFrame = frame;
        m_hasCommittedFrame = frame.point.valid;
        m_previousPointX = frame.point.x;
        m_previousPointY = frame.point.y;
    }

    inline void CommitTerminal(StylusFrameState& state) {
        ApplyTerminalFrame(state);
        CommitInvalid(state.stylus, state.flow.clearCommitted);
    }

    inline bool Finalize(StylusFrameData& frame) const {
        frame = m_lastResult;
        return true;
    }

    inline bool Finalize(StylusFrameState& state) const {
        return Finalize(state.stylus);
    }

    template <typename DiagnosticsLike>
    inline bool FinalizeWithDiagnostics(StylusFrameState& state,
                                        const DiagnosticsLike& diagnostics) const {
        const bool valid = Finalize(state);
#if EGOTOUCH_DIAG
        state.stylus.diag = diagnostics.GetLastDiagnostics();
#endif
        return valid;
    }

    template <typename DiagnosticsLike>
    inline bool FinalizeTerminalWithDiagnostics(StylusFrameState& state,
                                                const DiagnosticsLike& diagnostics) {
        CommitTerminal(state);
        return FinalizeWithDiagnostics(state, diagnostics);
    }

    inline void CommitFinal(StylusFrameState& state) {
        ApplyFinalFrame(state);
        CommitFinal(state.stylus);
    }

    template <typename EdgePostT>
    inline void CommitFinal(StylusFrameState& state,
                            const EdgePostT& edgePost) {
        ApplyFinalFrame(state, edgePost);
        CommitFinal(state.stylus);
    }

    template <typename DiagnosticsLike>
    inline bool FinalizeFinalWithDiagnostics(StylusFrameState& state,
                                             const DiagnosticsLike& diagnostics) {
        CommitFinal(state);
        return FinalizeWithDiagnostics(state, diagnostics);
    }

    template <typename EdgePostT, typename DiagnosticsLike>
    inline bool FinalizeFinalWithDiagnostics(StylusFrameState& state,
                                             const EdgePostT& edgePost,
                                             const DiagnosticsLike& diagnostics) {
        CommitFinal(state, edgePost);
        return FinalizeWithDiagnostics(state, diagnostics);
    }

    inline bool ReuseCommittedFrame(StylusFrameData& frame) {
        if (!m_hasCommittedFrame) {
            return false;
        }
        m_lastResult = m_committedFrame;
        frame = m_lastResult;
        return true;
    }

    inline bool ReuseCommittedFrame(StylusFrameState& state) {
        const bool reused = ReuseCommittedFrame(state.stylus);
        state.flow.reusedCommittedFrame = reused;
        return reused;
    }

    inline Asa::AsaCoorResult Process(StylusFrameState& state) const {
        state.output.finalCoor = ResolveFinalCoordinate(state.output.postCoor, state);
        return state.output.finalCoor;
    }

    inline Asa::AsaCoorResult ResolveFinalCoordinate(const Asa::AsaCoorResult& candidate,
                                                     const StylusFrameState& state) const {
        Asa::AsaCoorResult finalCoor = candidate;
        if (!state.lifecycle.keepPreviousCoordinate || !m_hasCommittedFrame) {
            return finalCoor;
        }

        finalCoor.valid = state.lifecycle.keepInRangeOnReleaseFrame || m_committedFrame.point.valid;
        finalCoor.dim1 = static_cast<int32_t>(m_committedFrame.point.x);
        finalCoor.dim2 = static_cast<int32_t>(m_committedFrame.point.y);
        return finalCoor;
    }

    inline bool HasCommittedFrame() const {
        return m_hasCommittedFrame;
    }

    inline const StylusFrameData& GetCommittedFrame() const {
        return m_committedFrame;
    }

    inline const StylusFrameData& GetLastResult() const {
        return m_lastResult;
    }

    inline float GetPreviousPointX() const {
        return m_previousPointX;
    }

    inline float GetPreviousPointY() const {
        return m_previousPointY;
    }

private:
    static inline void SyncPacketRoute(StylusFrameState& state) {
        state.stylus.packetRoute = state.flow.packetRoute;
    }

    static inline void ApplyFinalFrame(StylusFrameState& state) {
        SyncPacketRoute(state);
        state.stylus.pipelineStage = state.flow.pipelineStage;
        state.stylus.point.valid = state.output.finalCoor.valid;
        state.stylus.point.x = static_cast<float>(state.output.finalCoor.dim1);
        state.stylus.point.y = static_cast<float>(state.output.finalCoor.dim2);
        state.stylus.point.tiltX = state.output.tiltX;
        state.stylus.point.tiltY = state.output.tiltY;
    }

    template <typename EdgePostT>
    static inline void ApplyFinalFrame(StylusFrameState& state,
                                       const EdgePostT& edgePost) {
        ApplyFinalFrame(state);
        edgePost.Apply(
            state.stylus.point.x,
            state.stylus.point.y,
            state.sensorCols,
            state.sensorRows);
        state.output.finalCoor.dim1 = static_cast<int32_t>(state.stylus.point.x);
        state.output.finalCoor.dim2 = static_cast<int32_t>(state.stylus.point.y);
    }

    static inline void ApplyTerminalFrame(StylusFrameState& state) {
        auto& stylus = state.stylus;
        SyncPacketRoute(state);
        stylus.point.valid = false;
        stylus.point.x = 0.0f;
        stylus.point.y = 0.0f;
        stylus.pressure = 0;
        stylus.point.pressure = 0;
        stylus.tipSwitchActive = false;
        stylus.noPressInkActive = false;
        stylus.pipelineStage = state.flow.pipelineStage;
        state.output.finalCoor = {};
    }

    StylusFrameData m_lastResult{};
    StylusFrameData m_committedFrame{};
    bool m_hasCommittedFrame = false;
    float m_previousPointX = 0.0f;
    float m_previousPointY = 0.0f;
};

StylusPipeline::StylusPipeline()
    : m_output(std::make_unique<OutputState>()) {}

StylusPipeline::~StylusPipeline() = default;

const StylusFrameData& StylusPipeline::GetLastResult() const {
    return m_output->GetLastResult();
}

const StylusPipeline::DbgCoordBreakdown& StylusPipeline::GetDebugCoord() const {
    return m_diagnostics.GetLastDiagnostics();
}

bool StylusPipeline::Process(HeatmapFrame& frame) {
    StylusFrameState state(frame, m_sensorRows, m_sensorCols, m_anchorCenterOffset);

    m_postProcessor.sensorDimRows = m_sensorRows;
    m_postProcessor.sensorDimCols = m_sensorCols;

    m_output->BeginFrame(state);
    m_diagnostics.Reset();

    m_inputParser.Process(state);

    if (!state.flow.terminal) {
        m_cmfFilter.Process(state);
        m_peakDetector.Process(state);
    }

    if (!state.flow.terminal) {
        m_coordSolver.Process(state);
    }

    if (!state.flow.terminal) {
        m_noiseGate.ProcessJump(state);
    }

    if (state.flow.terminal) {
        if (state.flow.resetPost) {
            m_postProcessor.Reset();
            m_coorReviser.Reset();
            m_linearFilter.Reset();
        }
        if (state.flow.resetNoise) {
            m_noiseGate.Reset();
        }

        m_penState.ResetFallback(
            m_btPressBuf.ReadLatest().seq,
            m_pressureSolver,
            m_penStateMachine);
        Asa::PenFrameEvidence evidence{};
        evidence.noSignal = true;
        (void)m_penStateMachine.Process(evidence);
        Asa::StylusStateController::ApplyTerminalStylusStateMirrors(state, m_penStateMachine);
        return m_output->FinalizeTerminalWithDiagnostics(state, m_diagnostics);
    }

    state.lifecycle.btSample = m_btPressBuf.ReadLatest();
    state.lifecycle.previouslyWriting =
        (m_penStateMachine.GetState() == Asa::PenStateMachine::State::Writing);

    m_signalAnalyzer.Process(
        state,
        m_cmfFilter,
        m_peakDetector,
        state.lifecycle.previouslyWriting);
    m_noiseGate.Process(state);

    m_penState.Process(
        state,
        m_output->HasCommittedFrame(),
        m_pressureSolver,
        m_penStateMachine);

    if (state.flow.resetNoise) {
        m_noiseGate.Reset();
    }

    if (state.flow.reusedCommittedFrame && m_output->ReuseCommittedFrame(state)) {
        return m_output->FinalizeWithDiagnostics(state, m_diagnostics);
    }

    m_signalAnalyzer.Process(
        state,
        m_cmfFilter,
        m_peakDetector,
        state.lifecycle.currentlyWriting);

    if (state.lifecycle.enableCoorReviser && m_coorReviser.enabled &&
        state.parse.gridData.tx2.valid && state.tx2.peak.valid) {
        m_signalRatioTracker.Push(
            static_cast<int16_t>(state.tx1.peakSignal),
            static_cast<int16_t>(state.tx2.peakSignal));
    }

    m_postProcessor.Process(
        state,
        m_linearFilter,
        m_penStateMachine,
        m_coorReviser,
        m_noiseGate,
        *m_output);

#if EGOTOUCH_DIAG
    m_diagnostics.Process(
        state,
        m_penStateMachine,
        m_linearFilter,
        m_signalRatioTracker.GetAvgRatio(),
        m_cmfFilter.enabled,
        m_coorReviser);
#endif
    return m_output->FinalizeFinalWithDiagnostics(state, m_edgeCoorPost, m_diagnostics);
}

std::vector<ConfigParam> StylusPipeline::GetConfigSchema() const {
    using Cat = ConfigParam::Category;
    return {
        ConfigParam("sp.enableSlaveChecksum", "Enable Slave Checksum",
            ConfigParam::Bool, const_cast<bool*>(&m_inputParser.enableSlaveChecksum), Cat::General),
        ConfigParam("sp.emitPacketWhenInvalid", "Emit Packet When Invalid",
            ConfigParam::Bool, const_cast<bool*>(&m_emitPacketWhenInvalid), Cat::General),

        ConfigParam("sp.coordUseTriangle", "Use Triangle Mode",
            ConfigParam::Bool, const_cast<bool*>(&m_coordSolver.useTriangle), Cat::Solver),
        ConfigParam("sp.triEdgeSecondaryBlend", "Tri Edge Secondary Blend",
            ConfigParam::Bool, const_cast<bool*>(&m_coordSolver.triEdgeSecondaryBlend), Cat::Solver),
        ConfigParam("sp.triEdgeDim1Ratio", "Tri Edge Dim1 Ratio",
            ConfigParam::Int, const_cast<int*>(&m_coordSolver.triEdgeDim1.ratio), 0, 1000, Cat::Solver),
        ConfigParam("sp.triEdgeDim1ThLast", "Tri Edge Dim1 ThLast",
            ConfigParam::Int, const_cast<int*>(&m_coordSolver.triEdgeDim1.sumThresholdIdxLast), 0, 20000, Cat::Solver),
        ConfigParam("sp.triEdgeDim1Th0", "Tri Edge Dim1 Th0",
            ConfigParam::Int, const_cast<int*>(&m_coordSolver.triEdgeDim1.sumThresholdIdx0), 0, 20000, Cat::Solver),
        ConfigParam("sp.triEdgeDim2Ratio", "Tri Edge Dim2 Ratio",
            ConfigParam::Int, const_cast<int*>(&m_coordSolver.triEdgeDim2.ratio), 0, 1000, Cat::Solver),
        ConfigParam("sp.triEdgeDim2ThLast", "Tri Edge Dim2 ThLast",
            ConfigParam::Int, const_cast<int*>(&m_coordSolver.triEdgeDim2.sumThresholdIdxLast), 0, 20000, Cat::Solver),
        ConfigParam("sp.triEdgeDim2Th0", "Tri Edge Dim2 Th0",
            ConfigParam::Int, const_cast<int*>(&m_coordSolver.triEdgeDim2.sumThresholdIdx0), 0, 20000, Cat::Solver),
        ConfigParam("sp.sensorRows", "Sensor Rows (Y)",
            ConfigParam::Int, const_cast<int*>(&m_sensorRows), 9, 80, Cat::Solver),
        ConfigParam("sp.sensorCols", "Sensor Cols (X)",
            ConfigParam::Int, const_cast<int*>(&m_sensorCols), 9, 80, Cat::Solver),
        ConfigParam("sp.anchorCenterOffset", "Anchor Center Offset",
            ConfigParam::Int, const_cast<int*>(&m_anchorCenterOffset), 0, 8, Cat::Solver),
        ConfigParam("sp.pitchCompDim1Enabled", "Pitch Comp Dim1 Enable",
            ConfigParam::Bool, const_cast<bool*>(&m_coordSolver.pitchCompDim1.enabled), Cat::Solver),
        ConfigParam("sp.pitchCompDim2Enabled", "Pitch Comp Dim2 Enable",
            ConfigParam::Bool, const_cast<bool*>(&m_coordSolver.pitchCompDim2.enabled), Cat::Solver),
        ConfigParam("sp.gravityNoiseFloor", "Gravity Noise Floor",
            ConfigParam::Int, const_cast<int32_t*>(&m_coordSolver.gravityNoiseFloor), 0, 500, Cat::Solver),
        ConfigParam("sp.gravityFictEdge", "Gravity Fictitious Edge",
            ConfigParam::Bool, const_cast<bool*>(&m_coordSolver.gravityFictitiousEdge), Cat::Solver),
        ConfigParam("sp.recheckEnabled", "Enable Recheck",
            ConfigParam::Bool, const_cast<bool*>(&m_noiseGate.recheckEnabled), Cat::Solver),
        ConfigParam("sp.recheckThBase", "Signal Thresh Base",
            ConfigParam::Int, const_cast<int*>(&m_signalAnalyzer.recheckThresholdBase), 10, 5000, Cat::Solver),
        ConfigParam("sp.recheckThMulti", "Signal Thresh Multi",
            ConfigParam::Int, const_cast<int*>(&m_signalAnalyzer.recheckThresholdMulti), 10, 5000, Cat::Solver),
        ConfigParam("sp.tx1InkEnterTh", "TX1 Ink Enter Threshold",
            ConfigParam::Int, const_cast<int*>(&m_penState.tx1InkEnterThreshold), 10, 30000, Cat::Solver),
        ConfigParam("sp.tx1LiftSuspiciousTh", "TX1 Lift Suspicious Threshold",
            ConfigParam::Int, const_cast<int*>(&m_penState.tx1LiftSuspiciousThreshold), 10, 30000, Cat::Solver),
        ConfigParam("sp.tx1LiftAbsoluteTh", "TX1 Lift Absolute Threshold",
            ConfigParam::Int, const_cast<int*>(&m_penState.tx1LiftAbsoluteThreshold), 10, 30000, Cat::Solver),
        ConfigParam("sp.pitchMapEnabled", "Pitch Map Enabled",
            ConfigParam::Bool, const_cast<bool*>(&m_coordSolver.pitchMapEnabled), Cat::Solver),

        ConfigParam("sp.smSpeedLow", "SM Speed Low Ref",
            ConfigParam::Float, const_cast<float*>(&m_penStateMachine.speedLow), 1.0f, 100.0f, Cat::Filter),
        ConfigParam("sp.smSpeedHigh", "SM Speed High Ref",
            ConfigParam::Float, const_cast<float*>(&m_penStateMachine.speedHigh), 50.0f, 1000.0f, Cat::Filter),
        ConfigParam("sp.smStillThr", "SM Still Speed Threshold",
            ConfigParam::Float, const_cast<float*>(&m_penStateMachine.stillSpeedThreshold), 0.5f, 50.0f, Cat::Filter),
        ConfigParam("sp.smIirLow", "SM IIR Low-Speed Coef",
            ConfigParam::Int, const_cast<int*>(&m_penStateMachine.movingIirLow), 1, 16, Cat::Filter),
        ConfigParam("sp.smIirHigh", "SM IIR High-Speed Coef",
            ConfigParam::Int, const_cast<int*>(&m_penStateMachine.movingIirHigh), 1, 16, Cat::Filter),
        ConfigParam("sp.smIirDivisor", "SM IIR Divisor N",
            ConfigParam::Int, const_cast<int*>(&m_penStateMachine.iirDivisorN), 1, 256, Cat::Filter),
        ConfigParam("sp.smHoverIir", "SM Hover IIR Coef",
            ConfigParam::Int, const_cast<int*>(&m_penStateMachine.hoverIirCoef), 1, 16, Cat::Filter),
        ConfigParam("sp.smJitterMax", "SM Jitter Max Strength",
            ConfigParam::Int, const_cast<int*>(&m_penStateMachine.jitterMax), 0, 5, Cat::Filter),
        ConfigParam("sp.smSpdSmooth", "SM Speed Smooth Window",
            ConfigParam::Int, const_cast<int*>(&m_penStateMachine.speedSmoothWindow), 1, 20, Cat::Filter),
        ConfigParam("sp.smDirHalve", "SM Dir Halve Enable",
            ConfigParam::Bool, const_cast<bool*>(&m_penStateMachine.enableDirectionalHalve), Cat::Filter),
        ConfigParam("sp.smDirVelThr", "SM Dir Velocity Thr",
            ConfigParam::Float, const_cast<float*>(&m_penStateMachine.directionalVelThreshold), 0.1f, 50.0f, Cat::Filter),
        ConfigParam("sp.smLiftTimeout", "SM Lift Timeout",
            ConfigParam::Int, const_cast<int*>(&m_penStateMachine.liftTimeout), 1, 30, Cat::General),
        ConfigParam("sp.smLongPress", "SM Long Press Frames",
            ConfigParam::Int, const_cast<int*>(&m_penStateMachine.longPressFrames), 10, 600, Cat::General),

        ConfigParam("sp.jitEdgeDim1", "Jitter Edge Param Dim1",
            ConfigParam::Int, const_cast<int*>(&m_postProcessor.jitterEdgeParamDim1), 0, 20, Cat::Filter),
        ConfigParam("sp.jitEdgeDim2", "Jitter Edge Param Dim2",
            ConfigParam::Int, const_cast<int*>(&m_postProcessor.jitterEdgeParamDim2), 0, 20, Cat::Filter),
        ConfigParam("sp.jitCntrDim1", "Jitter Center Param Dim1",
            ConfigParam::Int, const_cast<int*>(&m_postProcessor.jitterCenterParamDim1), 0, 20, Cat::Filter),
        ConfigParam("sp.jitCntrDim2", "Jitter Center Param Dim2",
            ConfigParam::Int, const_cast<int*>(&m_postProcessor.jitterCenterParamDim2), 0, 20, Cat::Filter),

        ConfigParam("sp.lfMinFitLen", "LF Min Fit Length",
            ConfigParam::Int, const_cast<int*>(&m_linearFilter.minFitLength), 5, 100, Cat::Filter),
        ConfigParam("sp.lfEnterResidual", "LF Enter Residual Thr",
            ConfigParam::Float, const_cast<float*>(&m_linearFilter.enterResidualThreshold), 1.0f, 500.0f, Cat::Filter),
        ConfigParam("sp.lfExitDeviation", "LF Exit Deviation",
            ConfigParam::Float, const_cast<float*>(&m_linearFilter.exitDeviation), 10.0f, 1000.0f, Cat::Filter),
        ConfigParam("sp.lfPerpConstraint", "LF Perp Constraint (0-1)",
            ConfigParam::Float, const_cast<float*>(&m_linearFilter.perpConstraint), 0.0f, 1.0f, Cat::Filter),
        ConfigParam("sp.lfTransRate", "LF Transition Rate",
            ConfigParam::Float, const_cast<float*>(&m_linearFilter.transitionRate), 0.05f, 1.0f, Cat::Filter),

        ConfigParam("sp.hpp3NoiseEnabled", "Enable HPP3 Noise",
            ConfigParam::Bool, const_cast<bool*>(&m_noiseGate.noisePostEnabled), Cat::Filter),
        ConfigParam("sp.hpp3JumpTh", "Jump Threshold",
            ConfigParam::Float, const_cast<float*>(&m_noiseGate.coorJumpThreshold), 1.0f, 100.0f, Cat::Filter),
        ConfigParam("sp.cmfEnabled", "CMF Enabled",
            ConfigParam::Bool, const_cast<bool*>(&m_cmfFilter.enabled), Cat::Filter),
        ConfigParam("sp.cmfWindowSize", "CMF Window Size",
            ConfigParam::Int, const_cast<int*>(&m_cmfFilter.windowSize), 1, 8, Cat::Filter),

        ConfigParam("sp.edgeCoorPostEnabled", "Enable Edge Coordinate Process",
            ConfigParam::Bool, const_cast<bool*>(&m_edgeCoorPost.enabled), Cat::Behavior),
        ConfigParam("sp.crEnabled", "Enable TX2 Coor Reviser",
            ConfigParam::Bool, const_cast<bool*>(&m_coorReviser.enabled), Cat::Behavior),
        ConfigParam("sp.crTiltMultX", "CoorRevise Tilt Mult X",
            ConfigParam::Int, const_cast<int*>(&m_coorReviser.tiltMultiplierX), 0, 20, Cat::Behavior),
        ConfigParam("sp.crTiltMultY", "CoorRevise Tilt Mult Y",
            ConfigParam::Int, const_cast<int*>(&m_coorReviser.tiltMultiplierY), 0, 20, Cat::Behavior),
        ConfigParam("sp.crDiffAvgWin", "CoorRevise Diff Avg Window",
            ConfigParam::Int, const_cast<int*>(&m_coorReviser.diffAverageWindow), 1, 10, Cat::Behavior),
        ConfigParam("sp.crTiltAvgWin", "CoorRevise Tilt Avg Window",
            ConfigParam::Int, const_cast<int*>(&m_coorReviser.tiltAverageWindow), 1, 10, Cat::Behavior),
        ConfigParam("sp.crRevAvgWin", "CoorRevise Revise Avg Window",
            ConfigParam::Int, const_cast<int*>(&m_coorReviser.reviseAverageWindow), 1, 10, Cat::Behavior),
        ConfigParam("sp.crLimitStep", "CoorRevise Limit Step",
            ConfigParam::Int, const_cast<int*>(&m_coorReviser.reviseLimitStep), 1, 100, Cat::Behavior),
        ConfigParam("sp.crNormLenDim1", "CoorRevise NormLen Dim1",
            ConfigParam::Int, const_cast<int*>(&m_coorReviser.normLenDim1), 100, 10000, Cat::Behavior),
        ConfigParam("sp.crNormLenDim2", "CoorRevise NormLen Dim2",
            ConfigParam::Int, const_cast<int*>(&m_coorReviser.normLenDim2), 100, 10000, Cat::Behavior),
        ConfigParam("sp.crMaxTiltDeg", "CoorRevise Max Tilt Deg",
            ConfigParam::Int, const_cast<int*>(&m_coorReviser.maxTiltDeg), 10, 89, Cat::Behavior),
        ConfigParam("sp.crTiltJitterDeg", "CoorRevise Tilt Jitter Deg",
            ConfigParam::Int, const_cast<int*>(&m_coorReviser.tiltJitterDeg), 0, 10, Cat::Behavior),
        ConfigParam("sp.crKeepLast", "CoorRevise Keep Last On Invalid",
            ConfigParam::Bool, const_cast<bool*>(&m_coorReviser.keepLastOnInvalid), Cat::Behavior),

        ConfigParam("sp.pressPolyEnabled", "Polynomial Mapping",
            ConfigParam::Bool, const_cast<bool*>(&m_pressureSolver.polyEnabled), Cat::Output),
        ConfigParam("sp.pressSeg1Th", "Seg1 Threshold",
            ConfigParam::Int, const_cast<int*>(&m_pressureSolver.seg1Threshold), 0, 50, Cat::Output),
        ConfigParam("sp.pressSeg2Th", "Seg2 Threshold",
            ConfigParam::Int, const_cast<int*>(&m_pressureSolver.seg2Threshold), 50, 500, Cat::Output),
        ConfigParam("sp.pressGain", "Gain %",
            ConfigParam::Int, const_cast<int*>(&m_pressureSolver.gainPercent), 10, 500, Cat::Output),

        ConfigParam("sp.filterMode", "Filter Mode (0=IIR 1=1Euro 2=Off)",
            ConfigParam::Int, const_cast<int*>(&m_postProcessor.filterMode), 0, 2, Cat::Filter),
    };
}

void StylusPipeline::SaveConfig(std::ostream& out) const {
    out << "sp.enableSlaveChecksum=" << m_inputParser.enableSlaveChecksum << "\n";
    out << "sp.emitPacketWhenInvalid=" << m_emitPacketWhenInvalid << "\n";
    out << "sp.coordUseTriangle=" << m_coordSolver.useTriangle << "\n";
    out << "sp.triEdgeSecondaryBlend=" << m_coordSolver.triEdgeSecondaryBlend << "\n";
    out << "sp.triEdgeDim1Ratio=" << m_coordSolver.triEdgeDim1.ratio << "\n";
    out << "sp.triEdgeDim1ThLast=" << m_coordSolver.triEdgeDim1.sumThresholdIdxLast << "\n";
    out << "sp.triEdgeDim1Th0=" << m_coordSolver.triEdgeDim1.sumThresholdIdx0 << "\n";
    out << "sp.triEdgeDim2Ratio=" << m_coordSolver.triEdgeDim2.ratio << "\n";
    out << "sp.triEdgeDim2ThLast=" << m_coordSolver.triEdgeDim2.sumThresholdIdxLast << "\n";
    out << "sp.triEdgeDim2Th0=" << m_coordSolver.triEdgeDim2.sumThresholdIdx0 << "\n";
    out << "sp.sensorRows=" << m_sensorRows << "\n";
    out << "sp.sensorCols=" << m_sensorCols << "\n";
    out << "sp.anchorCenterOffset=" << m_anchorCenterOffset << "\n";
    out << "sp.pitchCompDim1Enabled=" << m_coordSolver.pitchCompDim1.enabled << "\n";
    out << "sp.pitchCompDim2Enabled=" << m_coordSolver.pitchCompDim2.enabled << "\n";
    out << "sp.gravityNoiseFloor=" << m_coordSolver.gravityNoiseFloor << "\n";
    out << "sp.gravityFictEdge=" << m_coordSolver.gravityFictitiousEdge << "\n";
    out << "sp.pitchMapEnabled=" << m_coordSolver.pitchMapEnabled << "\n";
    out << "sp.smSpeedLow=" << m_penStateMachine.speedLow << "\n";
    out << "sp.smSpeedHigh=" << m_penStateMachine.speedHigh << "\n";
    out << "sp.smStillThr=" << m_penStateMachine.stillSpeedThreshold << "\n";
    out << "sp.smIirLow=" << m_penStateMachine.movingIirLow << "\n";
    out << "sp.smIirHigh=" << m_penStateMachine.movingIirHigh << "\n";
    out << "sp.smIirDivisor=" << m_penStateMachine.iirDivisorN << "\n";
    out << "sp.smHoverIir=" << m_penStateMachine.hoverIirCoef << "\n";
    out << "sp.smJitterMax=" << m_penStateMachine.jitterMax << "\n";
    out << "sp.smSpdSmooth=" << m_penStateMachine.speedSmoothWindow << "\n";
    out << "sp.smDirHalve=" << m_penStateMachine.enableDirectionalHalve << "\n";
    out << "sp.smDirVelThr=" << m_penStateMachine.directionalVelThreshold << "\n";
    out << "sp.smLiftTimeout=" << m_penStateMachine.liftTimeout << "\n";
    out << "sp.smLongPress=" << m_penStateMachine.longPressFrames << "\n";
    out << "sp.jitEdgeDim1=" << m_postProcessor.jitterEdgeParamDim1 << "\n";
    out << "sp.jitEdgeDim2=" << m_postProcessor.jitterEdgeParamDim2 << "\n";
    out << "sp.jitCntrDim1=" << m_postProcessor.jitterCenterParamDim1 << "\n";
    out << "sp.jitCntrDim2=" << m_postProcessor.jitterCenterParamDim2 << "\n";
    out << "sp.lfMinFitLen=" << m_linearFilter.minFitLength << "\n";
    out << "sp.lfEnterResidual=" << m_linearFilter.enterResidualThreshold << "\n";
    out << "sp.lfExitDeviation=" << m_linearFilter.exitDeviation << "\n";
    out << "sp.lfPerpConstraint=" << m_linearFilter.perpConstraint << "\n";
    out << "sp.lfTransRate=" << m_linearFilter.transitionRate << "\n";
    out << "sp.crEnabled=" << m_coorReviser.enabled << "\n";
    out << "sp.crTiltMultX=" << m_coorReviser.tiltMultiplierX << "\n";
    out << "sp.crTiltMultY=" << m_coorReviser.tiltMultiplierY << "\n";
    out << "sp.crDiffAvgWin=" << m_coorReviser.diffAverageWindow << "\n";
    out << "sp.crTiltAvgWin=" << m_coorReviser.tiltAverageWindow << "\n";
    out << "sp.crRevAvgWin=" << m_coorReviser.reviseAverageWindow << "\n";
    out << "sp.crLimitStep=" << m_coorReviser.reviseLimitStep << "\n";
    out << "sp.crNormLenDim1=" << m_coorReviser.normLenDim1 << "\n";
    out << "sp.crNormLenDim2=" << m_coorReviser.normLenDim2 << "\n";
    out << "sp.crMaxTiltDeg=" << m_coorReviser.maxTiltDeg << "\n";
    out << "sp.crTiltJitterDeg=" << m_coorReviser.tiltJitterDeg << "\n";
    out << "sp.crKeepLast=" << m_coorReviser.keepLastOnInvalid << "\n";
    out << "sp.edgeCoorPostEnabled=" << m_edgeCoorPost.enabled << "\n";
    out << "sp.hpp3NoiseEnabled=" << m_noiseGate.noisePostEnabled << "\n";
    out << "sp.hpp3JumpTh=" << m_noiseGate.coorJumpThreshold << "\n";
    out << "sp.recheckEnabled=" << m_noiseGate.recheckEnabled << "\n";
    out << "sp.recheckThBase=" << m_signalAnalyzer.recheckThresholdBase << "\n";
    out << "sp.recheckThMulti=" << m_signalAnalyzer.recheckThresholdMulti << "\n";
    out << "sp.tx1InkEnterTh=" << m_penState.tx1InkEnterThreshold << "\n";
    out << "sp.tx1LiftSuspiciousTh=" << m_penState.tx1LiftSuspiciousThreshold << "\n";
    out << "sp.tx1LiftAbsoluteTh=" << m_penState.tx1LiftAbsoluteThreshold << "\n";
    out << "sp.cmfEnabled=" << m_cmfFilter.enabled << "\n";
    out << "sp.cmfWindowSize=" << m_cmfFilter.windowSize << "\n";
    out << "sp.pressPolyEnabled=" << m_pressureSolver.polyEnabled << "\n";
    out << "sp.pressSeg1Th=" << m_pressureSolver.seg1Threshold << "\n";
    out << "sp.pressSeg2Th=" << m_pressureSolver.seg2Threshold << "\n";
    out << "sp.pressGain=" << m_pressureSolver.gainPercent << "\n";
    out << "sp.filterMode=" << m_postProcessor.filterMode << "\n";
}

void StylusPipeline::LoadConfig(const std::string& key, const std::string& value) {
    auto toBool = [](const std::string& v) { return v == "1"; };
    auto toInt = [](const std::string& v) {
        try { return std::stoi(v); } catch (...) { return 0; }
    };
    auto toFloat = [](const std::string& v) {
        try { return std::stof(v); } catch (...) { return 0.0f; }
    };

    if (key == "sp.enableSlaveChecksum") m_inputParser.enableSlaveChecksum = toBool(value);
    else if (key == "sp.emitPacketWhenInvalid") m_emitPacketWhenInvalid = toBool(value);
    else if (key == "sp.coordUseTriangle") m_coordSolver.useTriangle = toBool(value);
    else if (key == "sp.triEdgeSecondaryBlend" || key == "sp.coordEdgeCompBit3") m_coordSolver.triEdgeSecondaryBlend = toBool(value);
    else if (key == "sp.triEdgeDim1Ratio") m_coordSolver.triEdgeDim1.ratio = std::clamp(toInt(value), 0, 1000);
    else if (key == "sp.triEdgeDim1ThLast") m_coordSolver.triEdgeDim1.sumThresholdIdxLast = std::clamp(toInt(value), 0, 20000);
    else if (key == "sp.triEdgeDim1Th0") m_coordSolver.triEdgeDim1.sumThresholdIdx0 = std::clamp(toInt(value), 0, 20000);
    else if (key == "sp.triEdgeDim2Ratio") m_coordSolver.triEdgeDim2.ratio = std::clamp(toInt(value), 0, 1000);
    else if (key == "sp.triEdgeDim2ThLast") m_coordSolver.triEdgeDim2.sumThresholdIdxLast = std::clamp(toInt(value), 0, 20000);
    else if (key == "sp.triEdgeDim2Th0") m_coordSolver.triEdgeDim2.sumThresholdIdx0 = std::clamp(toInt(value), 0, 20000);
    else if (key == "sp.sensorRows") m_sensorRows = toInt(value);
    else if (key == "sp.sensorCols") m_sensorCols = toInt(value);
    else if (key == "sp.anchorCenterOffset") m_anchorCenterOffset = toInt(value);
    else if (key == "sp.pitchCompDim1Enabled") m_coordSolver.pitchCompDim1.enabled = toBool(value);
    else if (key == "sp.pitchCompDim2Enabled") m_coordSolver.pitchCompDim2.enabled = toBool(value);
    else if (key == "sp.gravityNoiseFloor") m_coordSolver.gravityNoiseFloor = toInt(value);
    else if (key == "sp.gravityFictEdge") m_coordSolver.gravityFictitiousEdge = toBool(value);
    else if (key == "sp.pitchMapEnabled") m_coordSolver.pitchMapEnabled = toBool(value);
    else if (key == "sp.smSpeedLow") m_penStateMachine.speedLow = toFloat(value);
    else if (key == "sp.smSpeedHigh") m_penStateMachine.speedHigh = toFloat(value);
    else if (key == "sp.smStillThr") m_penStateMachine.stillSpeedThreshold = toFloat(value);
    else if (key == "sp.smIirLow") m_penStateMachine.movingIirLow = toInt(value);
    else if (key == "sp.smIirHigh") m_penStateMachine.movingIirHigh = toInt(value);
    else if (key == "sp.smIirDivisor") m_penStateMachine.iirDivisorN = toInt(value);
    else if (key == "sp.smHoverIir") m_penStateMachine.hoverIirCoef = toInt(value);
    else if (key == "sp.smJitterMax") m_penStateMachine.jitterMax = toInt(value);
    else if (key == "sp.smSpdSmooth") m_penStateMachine.speedSmoothWindow = toInt(value);
    else if (key == "sp.smDirHalve") m_penStateMachine.enableDirectionalHalve = toBool(value);
    else if (key == "sp.smDirVelThr") m_penStateMachine.directionalVelThreshold = toFloat(value);
    else if (key == "sp.smLiftTimeout") m_penStateMachine.liftTimeout = toInt(value);
    else if (key == "sp.smLongPress") m_penStateMachine.longPressFrames = toInt(value);
    else if (key == "sp.jitEdgeDim1") m_postProcessor.jitterEdgeParamDim1 = toInt(value);
    else if (key == "sp.jitEdgeDim2") m_postProcessor.jitterEdgeParamDim2 = toInt(value);
    else if (key == "sp.jitCntrDim1") m_postProcessor.jitterCenterParamDim1 = toInt(value);
    else if (key == "sp.jitCntrDim2") m_postProcessor.jitterCenterParamDim2 = toInt(value);
    else if (key == "sp.lfMinFitLen") m_linearFilter.minFitLength = toInt(value);
    else if (key == "sp.lfEnterResidual") m_linearFilter.enterResidualThreshold = toFloat(value);
    else if (key == "sp.lfExitDeviation") m_linearFilter.exitDeviation = toFloat(value);
    else if (key == "sp.lfPerpConstraint") m_linearFilter.perpConstraint = toFloat(value);
    else if (key == "sp.lfTransRate") m_linearFilter.transitionRate = toFloat(value);
    else if (key == "sp.crEnabled") m_coorReviser.enabled = toBool(value);
    else if (key == "sp.crTiltMultX") m_coorReviser.tiltMultiplierX = toInt(value);
    else if (key == "sp.crTiltMultY") m_coorReviser.tiltMultiplierY = toInt(value);
    else if (key == "sp.crDiffAvgWin") m_coorReviser.diffAverageWindow = toInt(value);
    else if (key == "sp.crTiltAvgWin") m_coorReviser.tiltAverageWindow = toInt(value);
    else if (key == "sp.crRevAvgWin") m_coorReviser.reviseAverageWindow = toInt(value);
    else if (key == "sp.crLimitStep") m_coorReviser.reviseLimitStep = toInt(value);
    else if (key == "sp.crNormLenDim1") m_coorReviser.normLenDim1 = toInt(value);
    else if (key == "sp.crNormLenDim2") m_coorReviser.normLenDim2 = toInt(value);
    else if (key == "sp.crMaxTiltDeg") m_coorReviser.maxTiltDeg = toInt(value);
    else if (key == "sp.crTiltJitterDeg") m_coorReviser.tiltJitterDeg = toInt(value);
    else if (key == "sp.crKeepLast") m_coorReviser.keepLastOnInvalid = toBool(value);
    else if (key == "sp.edgeCoorPostEnabled") m_edgeCoorPost.enabled = toBool(value);
    else if (key == "sp.hpp3NoiseEnabled") m_noiseGate.noisePostEnabled = toBool(value);
    else if (key == "sp.hpp3JumpTh") m_noiseGate.coorJumpThreshold = toFloat(value);
    else if (key == "sp.recheckEnabled") m_noiseGate.recheckEnabled = toBool(value);
    else if (key == "sp.recheckThBase") m_signalAnalyzer.recheckThresholdBase = toInt(value);
    else if (key == "sp.recheckThMulti") m_signalAnalyzer.recheckThresholdMulti = toInt(value);
    else if (key == "sp.tx1InkEnterTh") m_penState.tx1InkEnterThreshold = toInt(value);
    else if (key == "sp.tx1LiftSuspiciousTh") m_penState.tx1LiftSuspiciousThreshold = toInt(value);
    else if (key == "sp.tx1LiftAbsoluteTh") m_penState.tx1LiftAbsoluteThreshold = toInt(value);
    else if (key == "sp.cmfEnabled") m_cmfFilter.enabled = toBool(value);
    else if (key == "sp.cmfWindowSize") m_cmfFilter.windowSize = toInt(value);
    else if (key == "sp.pressPolyEnabled") m_pressureSolver.polyEnabled = toBool(value);
    else if (key == "sp.pressSeg1Th") m_pressureSolver.seg1Threshold = toInt(value);
    else if (key == "sp.pressSeg2Th") m_pressureSolver.seg2Threshold = toInt(value);
    else if (key == "sp.pressGain") m_pressureSolver.gainPercent = toInt(value);
    else if (key == "sp.filterMode") m_postProcessor.filterMode = std::clamp(toInt(value), 0, 2);
    else if (key == "sp.noPressEnabled" ||
             key == "sp.noPressBaseTh" ||
             key == "sp.noPressEnterRatio" ||
             key == "sp.noPressExitRatio" ||
             key == "sp.noPressTiltDeadzone" ||
             key == "sp.noPressTiltCap" ||
             key == "sp.noPressTiltScale" ||
             key == "sp.noPressDebounceEnter" ||
             key == "sp.noPressDebounceExit" ||
             key == "sp.noPressSyntheticMin" ||
             key == "sp.sigSuppressEnabled" ||
             key == "sp.sigSuppressEnter" ||
             key == "sp.sigSuppressExit" ||
             key == "sp.edgeSigSuppressEnabled" ||
             key == "sp.edgeSigSuppressEnter" ||
             key == "sp.edgeSigSuppressExit" ||
             key == "sp.btMapMode" ||
             key == "sp.pressIirQ8" ||
             key == "sp.tpPatternEnabled") {
        // legacy compatibility: accepted but no longer active in the main flow
    }
}

} // namespace Solvers
