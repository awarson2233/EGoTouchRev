#pragma once

#include "SolverTypes.h"

namespace Solvers::Stylus {

class StylusRuntimeCommit {
public:
    inline void Commit(HeatmapFrame& frame) const {
        auto& stylus = frame.stylus;
        auto& runtime = stylus.runtime.Active();
        const bool isHpp2 = stylus.runtime.activeProtocol == StylusRuntime::Protocol::Hpp2;
        const bool isHpp3 = stylus.runtime.activeProtocol == StylusRuntime::Protocol::Hpp3;

        stylus.output = {};
        stylus.interop = {};
#if EGOTOUCH_DIAG
        stylus.debug = {};
#endif

        stylus.output.valid = runtime.post.finalValid;
        stylus.output.inRange =
            runtime.decision.inRangeCandidate && runtime.post.finalValid;
        stylus.output.tipDown =
            runtime.decision.tipDownCandidate && stylus.output.valid;
        // TODO: Feed buttonActive into the VHF barrel button bit when packet emission is wired.
        stylus.output.buttonActive = isHpp2 && stylus.runtime.hpp2.buttonPressed && stylus.output.inRange;
        stylus.output.pressure = runtime.post.finalPressure;
        stylus.output.confidence = runtime.post.confidence;
        stylus.output.pipelineStage = runtime.flow.pipelineStage;
        stylus.output.point = runtime.post.point;
        stylus.output.point.x = static_cast<float>(runtime.post.finalCoor.dim1);
        stylus.output.point.y = static_cast<float>(runtime.post.finalCoor.dim2);
        stylus.output.point.valid = stylus.output.valid;
        stylus.output.point.pressure = stylus.output.pressure;
        stylus.output.point.confidence = stylus.output.confidence;

        stylus.interop.recheckEnabled = runtime.signal.recheckEnabled;
        stylus.interop.recheckPassed = runtime.signal.recheckPassed;
        stylus.interop.recheckOverlap = runtime.signal.recheckOverlap;
        stylus.interop.recheckThreshold = runtime.signal.recheckThreshold;
        stylus.interop.recheckThresholdMulti = runtime.signal.recheckThresholdMulti;
        stylus.interop.touchNullLike = runtime.signal.touchNullLike;
        stylus.interop.touchSuppressActive = runtime.decision.touchSuppressCarry;
        stylus.interop.touchSuppressFrames = runtime.decision.touchSuppressFrames;
        stylus.interop.signalX = runtime.signal.signalX;
        stylus.interop.signalY = runtime.signal.signalY;
        stylus.interop.maxRawPeak = runtime.signal.maxRawPeak;

#if EGOTOUCH_DIAG
        const auto& hpp3 = stylus.runtime.hpp3;
        stylus.debug.parse.slaveValid = stylus.input.slaveValid;
        stylus.debug.parse.checksumOk = stylus.input.checksumOk;
        stylus.debug.parse.status = stylus.input.status;
        stylus.debug.parse.pipelineStage = stylus.output.pipelineStage;
        stylus.debug.coord.valid = stylus.output.valid;
        if (isHpp3) {
            stylus.debug.coord.anchorRow = hpp3.rawGrid.grid.tx1.anchorRow;
            stylus.debug.coord.anchorCol = hpp3.rawGrid.grid.tx1.anchorCol;
        }
        stylus.debug.coord.rawDim1 = runtime.tx1.coordinate.localGridCoor.dim1;
        stylus.debug.coord.rawDim2 = runtime.tx1.coordinate.localGridCoor.dim2;
        stylus.debug.coord.finalDim1 = runtime.post.finalCoor.dim1;
        stylus.debug.coord.finalDim2 = runtime.post.finalCoor.dim2;
        stylus.debug.coord.centerOff = (static_cast<float>(Hpp3::kGridDim) * 0.5f) * static_cast<float>(Asa::kCoorUnit);
        stylus.debug.coord.pointX = stylus.output.point.x;
        stylus.debug.coord.pointY = stylus.output.point.y;
        stylus.debug.coord.peakSignal = runtime.signal.maxRawPeak;
        stylus.debug.coord.rawPressure = runtime.pressure.rawPressure;
        stylus.debug.coord.mappedPressure = runtime.pressure.mappedPressure;
        stylus.debug.coord.btSeq = runtime.pressure.btSeq;
        stylus.debug.coord.predictedAgeFrames = runtime.pressure.predictedAgeFrames;
        stylus.debug.coord.pressureIsReal = runtime.pressure.pressureIsReal;
        stylus.debug.coord.linearFilterState = runtime.post.linearFilterState;
        stylus.debug.coord.tiltDiffX = static_cast<float>(runtime.tilt.diffDim1);
        stylus.debug.coord.tiltDiffY = static_cast<float>(runtime.tilt.diffDim2);
        stylus.debug.coord.signalRatio = runtime.tilt.signalRatio;
        stylus.debug.coord.tiltAnomalyDamped = runtime.tilt.anomalyDamped;

        // ── GridFeatureExtractor ──
        if (isHpp3) {
            stylus.debug.coord.tx1PeakValue = static_cast<uint16_t>(hpp3.tx1Grid.feature.peak.peakValue);
            stylus.debug.coord.tx1Sum3x3 = static_cast<uint16_t>(hpp3.tx1Grid.feature.peak.neighborSum3x3);
            stylus.debug.coord.tx2PeakValue = static_cast<uint16_t>(hpp3.tx2Grid.feature.peak.peakValue);
            stylus.debug.coord.tx2Sum3x3 = static_cast<uint16_t>(hpp3.tx2Grid.feature.peak.neighborSum3x3);
            stylus.debug.coord.tx2Valid = hpp3.rawGrid.grid.tx2.valid;
        }

        // ── CoordinateSolver ──
        if (isHpp3) {
            stylus.debug.coord.triDim1Left = hpp3.tx1Grid.triLeft;
            stylus.debug.coord.triDim1Center = hpp3.tx1Grid.triCenter;
            stylus.debug.coord.triDim1Right = hpp3.tx1Grid.triRight;
            stylus.debug.coord.pitchCompApplied = hpp3.tx1Grid.pitchComp;
        }
        stylus.debug.coord.localCoorDim1 = runtime.tx1.coordinate.localGridCoor.dim1;
        stylus.debug.coord.localCoorDim2 = runtime.tx1.coordinate.localGridCoor.dim2;
        stylus.debug.coord.dim1Edge = runtime.signal.dim1EdgeActive;
        stylus.debug.coord.dim2Edge = runtime.signal.dim2EdgeActive;

        // ── TiltProcess ──
        stylus.debug.coord.tiltLenLimit = runtime.tilt.lenLimit;
        stylus.debug.coord.tiltRawDiffDim1 = runtime.tilt.rawDiffDim1;
        stylus.debug.coord.tiltRawDiffDim2 = runtime.tilt.rawDiffDim2;
        stylus.debug.coord.preTiltDim1 = runtime.tilt.preTiltDim1;
        stylus.debug.coord.preTiltDim2 = runtime.tilt.preTiltDim2;
        stylus.debug.coord.reportTiltDim1 = runtime.tilt.reportTiltDim1;
        stylus.debug.coord.reportTiltDim2 = runtime.tilt.reportTiltDim2;

        // ── PressureSolver ──
        stylus.debug.coord.btRawPressure = runtime.pressure.rawPressure;
        stylus.debug.coord.preIirPressure = runtime.pressure.preIirPressure;
        stylus.debug.coord.btPressSuppressActive = runtime.pressure.btPressSuppressActive;
        stylus.debug.coord.polySegment = runtime.pressure.polySegment;

        // ── PostPressure ──
        stylus.debug.coord.edgeSignalTooLowLatched = runtime.pressure.edgeSignalTooLowLatched;
        stylus.debug.coord.fakePressureDecreaseActive = runtime.pressure.fakePressureDecreaseActive;
        stylus.debug.coord.fakePressureDecreaseFramesLeft = runtime.pressure.fakePressureDecreaseFramesLeft;
        stylus.debug.coord.btFreqShiftDebounceFramesLeft = runtime.pressure.btFreqShiftDebounceFramesLeft;

        // ── LinearFilterProcess ──
        stylus.debug.coord.avg3PtDim1 = runtime.post.postCoor.dim1;
        stylus.debug.coord.avg3PtDim2 = runtime.post.postCoor.dim2;
        stylus.debug.coord.lfStateMachine = runtime.post.linearFilterState;
        stylus.debug.coord.lfLineFitSlopeA = runtime.post.lfLineFitSlopeA;
        stylus.debug.coord.lfLineFitInterceptB = runtime.post.lfLineFitInterceptB;
        stylus.debug.coord.lfLineFitValid = runtime.post.lfLineFitValid;
        stylus.debug.coord.lfCos1000 = runtime.post.lfCos1000;
        stylus.debug.coord.lfStraightBufCount = runtime.post.lfStraightBufCount;
        stylus.debug.coord.lfDragApplied = runtime.post.lfDragApplied;

        // ── CoorSpeedProcess ──
        stylus.debug.coord.speedInstant = static_cast<float>(runtime.post.speedValue);
        stylus.debug.coord.speedShortAvg = static_cast<float>(runtime.post.speedShortAvgDist);
        stylus.debug.coord.speedFullAvg = static_cast<float>(runtime.post.speedFullAvgDist);

        // ── CoorIIRProcess ──
        stylus.debug.coord.iirCoef = static_cast<float>(runtime.post.iirCoef);
        stylus.debug.coord.isHover = runtime.pressure.outputPressure == 0;
        stylus.debug.coord.isEdge = runtime.signal.dim1EdgeActive ||
                                   runtime.signal.dim2EdgeActive;

        // ── CoorReviseProcess ──
        stylus.debug.coord.coorReviserActive = runtime.post.coorReviseActive;
        stylus.debug.coord.coorRevDeltaX = static_cast<float>(runtime.post.coorReviseCorrectionDim1);
        stylus.debug.coord.coorRevDeltaY = static_cast<float>(runtime.post.coorReviseCorrectionDim2);
#endif

    }
};

} // namespace Solvers::Stylus
