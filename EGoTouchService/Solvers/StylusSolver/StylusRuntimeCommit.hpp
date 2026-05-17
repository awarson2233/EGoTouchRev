#pragma once

#include "SolverTypes.h"

namespace Solvers::Stylus {

class StylusRuntimeCommit {
public:
    inline void Commit(HeatmapFrame& frame) const {
        auto& stylus = frame.stylus;

        stylus.output = {};
        stylus.interop = {};
#if EGOTOUCH_DIAG
        stylus.debug = {};
#endif

        stylus.output.valid = stylus.runtime.post.finalValid;
        stylus.output.inRange =
            stylus.runtime.decision.inRangeCandidate && stylus.runtime.post.finalValid;
        stylus.output.tipDown =
            stylus.runtime.decision.tipDownCandidate && stylus.output.valid;
        stylus.output.pressure = stylus.runtime.post.finalPressure;
        stylus.output.confidence = stylus.runtime.post.confidence;
        stylus.output.pipelineStage = stylus.runtime.flow.pipelineStage;
        stylus.output.point = stylus.runtime.post.point;
        stylus.output.point.valid = stylus.output.valid;
        stylus.output.point.pressure = stylus.output.pressure;
        stylus.output.point.confidence = stylus.output.confidence;

        stylus.interop.recheckEnabled = stylus.runtime.signal.recheckEnabled;
        stylus.interop.recheckPassed = stylus.runtime.signal.recheckPassed;
        stylus.interop.recheckOverlap = stylus.runtime.signal.recheckOverlap;
        stylus.interop.recheckThreshold = stylus.runtime.signal.recheckThreshold;
        stylus.interop.recheckThresholdMulti = stylus.runtime.signal.recheckThresholdMulti;
        stylus.interop.touchNullLike = stylus.runtime.signal.touchNullLike;
        stylus.interop.touchSuppressActive = stylus.runtime.decision.touchSuppressCarry;
        stylus.interop.touchSuppressFrames = stylus.runtime.decision.touchSuppressFrames;
        stylus.interop.signalX = stylus.runtime.signal.signalX;
        stylus.interop.signalY = stylus.runtime.signal.signalY;
        stylus.interop.maxRawPeak = stylus.runtime.signal.maxRawPeak;

#if EGOTOUCH_DIAG
        stylus.debug.parse.slaveValid = stylus.input.slaveValid;
        stylus.debug.parse.checksumOk = stylus.input.checksumOk;
        stylus.debug.parse.status = stylus.input.status;
        stylus.debug.parse.pipelineStage = stylus.output.pipelineStage;
        stylus.debug.coord.valid = stylus.output.valid;
        stylus.debug.coord.anchorRow = stylus.runtime.rawGrid.asaGrid.tx1.anchorRow;
        stylus.debug.coord.anchorCol = stylus.runtime.rawGrid.asaGrid.tx1.anchorCol;
        stylus.debug.coord.rawDim1 = stylus.runtime.tx1.coordinate.localGridCoor.dim1;
        stylus.debug.coord.rawDim2 = stylus.runtime.tx1.coordinate.localGridCoor.dim2;
        stylus.debug.coord.finalDim1 = stylus.runtime.tx1.coordinate.reportGlobalCoor.dim1;
        stylus.debug.coord.finalDim2 = stylus.runtime.tx1.coordinate.reportGlobalCoor.dim2;
        stylus.debug.coord.centerOff = (static_cast<float>(Asa::kGridDim) * 0.5f) * static_cast<float>(Asa::kCoorUnit);
        stylus.debug.coord.pointX = stylus.output.point.x;
        stylus.debug.coord.pointY = stylus.output.point.y;
        stylus.debug.coord.peakSignal = stylus.runtime.signal.maxRawPeak;
        stylus.debug.coord.rawPressure = stylus.runtime.pressure.rawPressure;
        stylus.debug.coord.mappedPressure = stylus.runtime.pressure.mappedPressure;
        stylus.debug.coord.btSeq = stylus.runtime.pressure.btSeq;
        stylus.debug.coord.predictedAgeFrames = stylus.runtime.pressure.predictedAgeFrames;
        stylus.debug.coord.pressureIsReal = stylus.runtime.pressure.pressureIsReal;
        stylus.debug.coord.linearFilterState = stylus.runtime.post.linearFilterState;
        stylus.debug.coord.coorReviserActive = stylus.runtime.post.linearFilterActive;
        stylus.debug.coord.coorRevDeltaX = static_cast<float>(stylus.runtime.post.linearFilterDeltaDim1);
        stylus.debug.coord.coorRevDeltaY = static_cast<float>(stylus.runtime.post.linearFilterDeltaDim2);
        stylus.debug.coord.tiltDiffX = static_cast<float>(stylus.runtime.tilt.diffDim1);
        stylus.debug.coord.tiltDiffY = static_cast<float>(stylus.runtime.tilt.diffDim2);
        stylus.debug.coord.signalRatio = stylus.runtime.tilt.signalRatio;
        stylus.debug.coord.tiltAnomalyDamped = stylus.runtime.tilt.anomalyDamped;

        // ── GridFeatureExtractor ──
        stylus.debug.coord.tx1PeakValue = static_cast<uint16_t>(stylus.runtime.tx1.feature.peak.peakValue);
        stylus.debug.coord.tx1Sum3x3 = static_cast<uint16_t>(stylus.runtime.tx1.feature.peak.neighborSum3x3);
        stylus.debug.coord.tx2PeakValue = static_cast<uint16_t>(stylus.runtime.tx2.feature.peak.peakValue);
        stylus.debug.coord.tx2Sum3x3 = static_cast<uint16_t>(stylus.runtime.tx2.feature.peak.neighborSum3x3);
        stylus.debug.coord.tx2Valid = stylus.runtime.rawGrid.asaGrid.tx2.valid;

        // ── CoordinateSolver ──
        stylus.debug.coord.triDim1Left = stylus.runtime.tx1.triLeft;
        stylus.debug.coord.triDim1Center = stylus.runtime.tx1.triCenter;
        stylus.debug.coord.triDim1Right = stylus.runtime.tx1.triRight;
        stylus.debug.coord.pitchCompApplied = stylus.runtime.tx1.pitchComp;
        stylus.debug.coord.localCoorDim1 = stylus.runtime.tx1.coordinate.localGridCoor.dim1;
        stylus.debug.coord.localCoorDim2 = stylus.runtime.tx1.coordinate.localGridCoor.dim2;
        stylus.debug.coord.dim1Edge = stylus.runtime.signal.dim1EdgeActive;
        stylus.debug.coord.dim2Edge = stylus.runtime.signal.dim2EdgeActive;

        // ── TiltProcess ──
        stylus.debug.coord.tiltLenLimit = stylus.runtime.tilt.lenLimit;
        stylus.debug.coord.tiltRawDiffDim1 = stylus.runtime.tilt.rawDiffDim1;
        stylus.debug.coord.tiltRawDiffDim2 = stylus.runtime.tilt.rawDiffDim2;

        // ── PressureSolver ──
        stylus.debug.coord.btRawPressure = stylus.runtime.pressure.rawPressure;
        stylus.debug.coord.preIirPressure = stylus.runtime.pressure.preIirPressure;
        stylus.debug.coord.btPressSuppressActive = stylus.runtime.pressure.btPressSuppressActive;
        stylus.debug.coord.polySegment = stylus.runtime.pressure.polySegment;

        // ── PostPressure ──
        stylus.debug.coord.edgeSignalTooLowLatched = stylus.runtime.pressure.edgeSignalTooLowLatched;
        stylus.debug.coord.fakePressureDecreaseActive = stylus.runtime.pressure.fakePressureDecreaseActive;
        stylus.debug.coord.fakePressureDecreaseFramesLeft = stylus.runtime.pressure.fakePressureDecreaseFramesLeft;
        stylus.debug.coord.btFreqShiftDebounceFramesLeft = stylus.runtime.pressure.btFreqShiftDebounceFramesLeft;

        // ── LinearFilterProcess ──
        stylus.debug.coord.lfStateMachine = stylus.runtime.post.linearFilterState;
        stylus.debug.coord.lfLineFitSlopeA = stylus.runtime.post.lfLineFitSlopeA;
        stylus.debug.coord.lfLineFitInterceptB = stylus.runtime.post.lfLineFitInterceptB;
        stylus.debug.coord.lfLineFitValid = stylus.runtime.post.lfLineFitValid;
        stylus.debug.coord.lfCos1000 = stylus.runtime.post.lfCos1000;
        stylus.debug.coord.lfStraightBufCount = stylus.runtime.post.lfStraightBufCount;
        stylus.debug.coord.lfDragApplied = stylus.runtime.post.lfDragApplied;
#endif

    }
};

} // namespace Solvers::Stylus
