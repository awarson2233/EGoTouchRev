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
        stylus.debug.coord.pointX = stylus.output.point.x;
        stylus.debug.coord.pointY = stylus.output.point.y;
        stylus.debug.coord.peakSignal = stylus.runtime.signal.maxRawPeak;
        stylus.debug.coord.rawPressure = stylus.runtime.pressure.rawPressure;
        stylus.debug.coord.mappedPressure = stylus.runtime.pressure.mappedPressure;
        stylus.debug.coord.btSeq = stylus.runtime.pressure.btSeq;
        stylus.debug.coord.predictedAgeFrames = stylus.runtime.pressure.predictedAgeFrames;
        stylus.debug.coord.pressureIsReal = stylus.runtime.pressure.pressureIsReal;
        stylus.debug.coord.linearFilterState = static_cast<uint8_t>(stylus.output.pipelineStage);
#endif

        stylus.SyncLegacyFieldsFromContract();
    }
};

} // namespace Solvers::Stylus
