#pragma once
#include "AsaTypes.hpp"
#include "CommonModeFilter.hpp"
#include "GridPeakDetector.hpp"
#include "StylusFrameState.hpp"

#include <algorithm>
#include <cstdint>

namespace Asa {

struct StylusSignalMetrics {
    uint16_t signalX = 0;
    uint16_t signalY = 0;
    uint16_t maxRawPeak = 0;
    uint16_t tx1Composite = 0;
    uint16_t tx2Composite = 0;
    bool dim1EdgeActive = false;
    bool dim2EdgeActive = false;
    uint16_t dim1EdgeSignal = 0;
    uint16_t dim2EdgeSignal = 0;
};

struct StylusRecheckContext {
    bool stable = false;
    bool active = false;
    bool overlapLike = false;
    uint16_t finalThreshold = 0;
    uint16_t sustainThreshold = 0;
};

struct StylusSignalProcessOptions {
    const Asa::CommonModeFilter* cmfFilter = nullptr;
    const Asa::GridPeakDetector* peakDetector = nullptr;
    bool recentlyWriting = false;
    int baseThreshold = 0;
    int multiThreshold = 0;
    bool preserveExistingRecheckPassed = true;
    bool recheckPassed = false;
};

struct StylusSignalAnalysis {
    StylusSignalMetrics metrics{};
    StylusRecheckContext recheck{};
    bool recheckPassed = false;
};

class StylusSignalAnalyzer {
public:
    int recheckThresholdBase = 800;
    int recheckThresholdMulti = 1200;

    inline StylusSignalAnalysis Process(Solvers::StylusFrameState& state,
                                        const StylusSignalProcessOptions& options) const {
        StylusSignalAnalysis analysis = AnalyzeState(state, options);
        ApplyAnalysisToState(state, analysis);
        return analysis;
    }

    inline StylusSignalMetrics Process(Solvers::StylusFrameState& state,
                                       const Asa::CommonModeFilter& cmfFilter,
                                       const Asa::GridPeakDetector& peakDetector,
                                       bool recentlyWriting) const {
        StylusSignalProcessOptions options{};
        options.cmfFilter = &cmfFilter;
        options.peakDetector = &peakDetector;
        options.recentlyWriting = recentlyWriting;
        options.baseThreshold = recheckThresholdBase;
        options.multiThreshold = recheckThresholdMulti;
        options.preserveExistingRecheckPassed = true;
        return Process(state, options).metrics;
    }

    inline StylusSignalMetrics Process(Solvers::StylusFrameState& state,
                                       const Asa::CommonModeFilter& cmfFilter,
                                       const Asa::GridPeakDetector& peakDetector,
                                       bool recentlyWriting,
                                       int baseThreshold,
                                       int multiThreshold) const {
        StylusSignalProcessOptions options{};
        options.cmfFilter = &cmfFilter;
        options.peakDetector = &peakDetector;
        options.recentlyWriting = recentlyWriting;
        options.baseThreshold = baseThreshold;
        options.multiThreshold = multiThreshold;
        options.preserveExistingRecheckPassed = true;
        return Process(state, options).metrics;
    }

    inline StylusSignalMetrics Process(Solvers::StylusFrameState& state,
                                       const Asa::CommonModeFilter& cmfFilter,
                                       const Asa::GridPeakDetector& peakDetector,
                                       bool recentlyWriting,
                                       int baseThreshold,
                                       int multiThreshold,
                                       bool recheckPassed) const {
        StylusSignalProcessOptions options{};
        options.cmfFilter = &cmfFilter;
        options.peakDetector = &peakDetector;
        options.recentlyWriting = recentlyWriting;
        options.baseThreshold = baseThreshold;
        options.multiThreshold = multiThreshold;
        options.preserveExistingRecheckPassed = false;
        options.recheckPassed = recheckPassed;
        return Process(state, options).metrics;
    }

    inline uint16_t Process(const Asa::AsaProjection& proj) const {
        return BuildProjectionComposite(proj);
    }

    inline StylusSignalMetrics Process(const Asa::AsaProjection& proj,
                                       const Asa::AsaCoorResult& rawCoor,
                                       uint16_t tx1PeakSignal,
                                       uint16_t tx2PeakSignal,
                                       int sensorCols,
                                       int sensorRows) const {
        return BuildSignalMetrics(proj, rawCoor, tx1PeakSignal, tx2PeakSignal, sensorCols, sensorRows);
    }

    inline StylusRecheckContext Process(const StylusSignalMetrics& metrics,
                                        bool coordValid,
                                        bool recentlyWriting,
                                        int baseThreshold,
                                        int multiThreshold) const {
        return BuildRecheckContext(metrics, coordValid, recentlyWriting, baseThreshold, multiThreshold);
    }

    static inline uint16_t ClampU16(int value) {
        return static_cast<uint16_t>(std::clamp(value, 0, 0xFFFF));
    }

    static inline uint16_t NormalizeProjectionPeak(const int32_t* signal, int peakIdx, int span) {
        if (peakIdx < 0 || span <= 0) {
            return 0;
        }
        const int32_t peak = signal[peakIdx];
        return ClampU16(static_cast<int>(peak / std::max(1, span)));
    }

    static inline bool IsAxisEdgeActive(int32_t coor, int sensorDim) {
        const int32_t margin = Asa::kCoorUnit;
        const int32_t maxCoor = sensorDim * Asa::kCoorUnit;
        return (coor < margin) || (coor > maxCoor - margin);
    }

    static inline uint16_t BuildProjectionComposite(const Asa::AsaProjection& proj) {
        const uint16_t dim1ProjectionPeak = NormalizeProjectionPeak(proj.dim1, proj.peakIdxDim1, proj.spanDim1);
        const uint16_t dim2ProjectionPeak = NormalizeProjectionPeak(proj.dim2, proj.peakIdxDim2, proj.spanDim2);
        return ClampU16((static_cast<int>(dim1ProjectionPeak) + static_cast<int>(dim2ProjectionPeak)) / 2);
    }

    static inline StylusSignalMetrics BuildSignalMetrics(const Asa::AsaProjection& proj,
                                                         const Asa::AsaCoorResult& rawCoor,
                                                         uint16_t tx1PeakSignal,
                                                         uint16_t tx2PeakSignal,
                                                         int sensorCols,
                                                         int sensorRows) {
        StylusSignalMetrics metrics{};
        metrics.signalX = tx1PeakSignal;
        metrics.signalY = tx2PeakSignal;
        metrics.maxRawPeak = std::max(metrics.signalX, metrics.signalY);

        const uint16_t dim1ProjectionPeak = NormalizeProjectionPeak(proj.dim1, proj.peakIdxDim1, proj.spanDim1);
        const uint16_t dim2ProjectionPeak = NormalizeProjectionPeak(proj.dim2, proj.peakIdxDim2, proj.spanDim2);
        metrics.tx1Composite = BuildProjectionComposite(proj);
        metrics.tx2Composite = tx2PeakSignal;
        metrics.dim1EdgeActive = rawCoor.valid && IsAxisEdgeActive(rawCoor.dim1, sensorCols);
        metrics.dim2EdgeActive = rawCoor.valid && IsAxisEdgeActive(rawCoor.dim2, sensorRows);
        metrics.dim1EdgeSignal = dim1ProjectionPeak;
        metrics.dim2EdgeSignal = dim2ProjectionPeak;
        return metrics;
    }

    static inline StylusRecheckContext BuildRecheckContext(const StylusSignalMetrics& metrics,
                                                           bool coordValid,
                                                           bool recentlyWriting,
                                                           int baseThreshold,
                                                           int multiThreshold) {
        StylusRecheckContext ctx{};
        ctx.finalThreshold = ClampU16(baseThreshold);
        ctx.sustainThreshold = ClampU16(multiThreshold);
        ctx.overlapLike = coordValid && metrics.signalY > 0 &&
                          metrics.tx2Composite > metrics.tx1Composite &&
                          metrics.signalX < ctx.sustainThreshold;
        ctx.stable = coordValid && metrics.tx1Composite >= ctx.finalThreshold;
        ctx.active = coordValid && (recentlyWriting || metrics.tx1Composite >= ctx.sustainThreshold);
        return ctx;
    }

private:
    inline StylusSignalAnalysis AnalyzeState(
            Solvers::StylusFrameState& state,
            const StylusSignalProcessOptions& options) const {
        StylusSignalAnalysis analysis{};

        const AsaProjection preCmfProjection = BuildPreCmfProjection(state, options);
        const Asa::AsaCoorResult& coordForMetrics =
            state.tx1.localCoor.valid ? state.tx1.localCoor : state.tx1.globalCoor;

        analysis.metrics = BuildSignalMetrics(
            state.tx1.projection,
            coordForMetrics,
            state.tx1.peakSignal,
            state.tx2.peakSignal,
            state.sensorCols,
            state.sensorRows);

        if (preCmfProjection.peakIdxDim1 >= 0 || preCmfProjection.peakIdxDim2 >= 0) {
            analysis.metrics.tx1Composite = BuildProjectionComposite(preCmfProjection);
        }

        const bool coordValid = state.tx1.localCoor.valid || state.tx1.globalCoor.valid;
        analysis.recheck = BuildRecheckContext(
            analysis.metrics,
            coordValid,
            options.recentlyWriting,
            options.baseThreshold,
            options.multiThreshold);

        analysis.recheckPassed = options.preserveExistingRecheckPassed
            ? state.signal.recheckPassed
            : options.recheckPassed;
        return analysis;
    }

    static inline AsaProjection BuildPreCmfProjection(
            const Solvers::StylusFrameState& state,
            const StylusSignalProcessOptions& options) {
        AsaProjection proj{};
        proj.Clear();
        if (!options.cmfFilter || !options.peakDetector) {
            return proj;
        }
        return options.cmfFilter->BuildPreCmfTx1Projection(*options.peakDetector, state);
    }

    static inline void ApplyAnalysisToState(
            Solvers::StylusFrameState& state,
            const StylusSignalAnalysis& analysis) {
        state.signal.signalX = analysis.metrics.signalX;
        state.signal.signalY = analysis.metrics.signalY;
        state.signal.maxRawPeak = analysis.metrics.maxRawPeak;
        state.signal.tx1Composite = analysis.metrics.tx1Composite;
        state.signal.tx2Composite = analysis.metrics.tx2Composite;
        state.signal.dim1EdgeActive = analysis.metrics.dim1EdgeActive;
        state.signal.dim2EdgeActive = analysis.metrics.dim2EdgeActive;
        state.signal.dim1EdgeSignal = analysis.metrics.dim1EdgeSignal;
        state.signal.dim2EdgeSignal = analysis.metrics.dim2EdgeSignal;
        state.signal.overlapLike = analysis.recheck.overlapLike;
        state.signal.recheckThreshold = analysis.recheck.finalThreshold;
        state.signal.recheckThresholdMulti = analysis.recheck.sustainThreshold;
        state.signal.recheckPassed = analysis.recheckPassed;

        state.stylus.tx1BlockValid = state.parse.gridData.tx1.valid;
        state.stylus.tx2BlockValid = state.parse.gridData.tx2.valid;
        state.stylus.signalX = state.signal.signalX;
        state.stylus.signalY = state.signal.signalY;
        state.stylus.maxRawPeak = state.signal.maxRawPeak;
        state.stylus.point.peakTx1 = state.signal.tx1Composite;
        state.stylus.point.peakTx2 = state.signal.tx2Composite;
        state.stylus.recheckThreshold = state.signal.recheckThreshold;
        state.stylus.recheckThresholdMulti = state.signal.recheckThresholdMulti;
        state.stylus.recheckOverlap = state.signal.overlapLike;
        state.stylus.recheckPassed = state.signal.recheckPassed;
        state.stylus.hpp3Dim1SignalValid = (state.signal.signalX > 0);
        state.stylus.hpp3Dim2SignalValid = (state.signal.signalY > 0);
    }
};

} // namespace Asa
