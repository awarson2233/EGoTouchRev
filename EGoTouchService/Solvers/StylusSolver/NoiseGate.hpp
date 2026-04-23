#pragma once
#include <algorithm>
#include "AsaTypes.hpp"
#include "StylusFrameState.hpp"
#include <cstdint>

namespace Asa {

/// NoiseGate — HPP3 noise jump detection and pen exit smoothing.
///
/// Two related mechanisms:
///   1. ApplyHpp3NoisePost: Detects large coordinate jumps and triggers frame freeze
///   2. HandlePenExitSmooth: On pen lift while inking, outputs one frozen frame
///      with edge coordinate snapping (TSACore: ReleaseASAReportExitStylus)
struct StylusExitSnapContext {
    bool hasCommittedFrame = false;
    float committedPointX = 0.0f;
    float committedPointY = 0.0f;
    float previousPointX = 0.0f;
    float previousPointY = 0.0f;

    template <typename CommitterLike>
    static inline StylusExitSnapContext FromCommitter(const CommitterLike& committer) {
        StylusExitSnapContext ctx{};
        ctx.hasCommittedFrame = committer.HasCommittedFrame();
        if (!ctx.hasCommittedFrame) {
            return ctx;
        }

        ctx.committedPointX = committer.GetCommittedFrame().point.x;
        ctx.committedPointY = committer.GetCommittedFrame().point.y;
        ctx.previousPointX = committer.GetPreviousPointX();
        ctx.previousPointY = committer.GetPreviousPointY();
        return ctx;
    }

    inline bool CanApply(const Solvers::StylusFrameState& state) const {
        return hasCommittedFrame &&
               state.lifecycle.applyExitEdgeSnap &&
               state.lifecycle.keepPreviousCoordinate;
    }
};

class NoiseGate {
public:
    inline bool Process(const AsaCoorResult& coor) {
        return DetectNoiseJump(coor);
    }

    inline bool ProcessJump(Solvers::StylusFrameState& state) {
        const bool detected = DetectNoiseJump(state.tx1.globalCoor);
        if (detected) {
            state.flow.terminal = true;
            state.flow.pipelineStage = 5;
            state.flow.packetRoute = Solvers::StylusPacketRoute::InvalidZeroState;
            state.flow.clearCommitted = false;
            state.flow.resetPost = true;
            state.flow.resetNoise = false;
        }
        return detected;
    }

    inline bool Process(Solvers::StylusSignalState& signal) const {
        return ProcessRecheck(signal);
    }

    inline bool Process(Solvers::StylusFrameState& state) const {
        return ProcessRecheck(state);
    }

    inline AsaCoorResult Process(Solvers::StylusFrameState& state,
                                 const StylusExitSnapContext& ctx) const {
        return ProcessExitSnap(state, ctx);
    }

    inline bool Process(uint16_t signalX, uint16_t signalY,
                        uint16_t maxRawPeak, uint16_t baseThreshold,
                        uint16_t sustainThreshold,
                        bool overlapLike) const {
        return EvaluateRecheck(signalX, signalY, maxRawPeak, baseThreshold, sustainThreshold, overlapLike);
    }

    inline void Process(float lastX, float lastY,
                        float prevX, float prevY,
                        int sensorRows, int sensorCols,
                        float& outX, float& outY) const {
        ApplyExitEdgeSnap(lastX, lastY, prevX, prevY, sensorRows, sensorCols, outX, outY);
    }

    inline bool ProcessRecheck(Solvers::StylusSignalState& signal) const {
        signal.recheckPassed = EvaluateRecheck(
            signal.signalX,
            signal.signalY,
            signal.maxRawPeak,
            signal.recheckThreshold,
            signal.recheckThresholdMulti,
            signal.overlapLike);
        return signal.recheckPassed;
    }

    inline bool ProcessRecheck(Solvers::StylusFrameState& state) const {
        const bool passed = ProcessRecheck(state.signal);
        state.stylus.recheckEnabled = recheckEnabled;
        state.stylus.recheckPassed = state.signal.recheckPassed;
        return passed;
    }

    inline AsaCoorResult ProcessExitSnap(Solvers::StylusFrameState& state,
                                         const StylusExitSnapContext& ctx) const {
        if (!ctx.CanApply(state)) {
            return state.output.finalCoor;
        }

        float snappedX = static_cast<float>(state.output.finalCoor.dim1);
        float snappedY = static_cast<float>(state.output.finalCoor.dim2);
        ApplyExitEdgeSnap(
            ctx.committedPointX,
            ctx.committedPointY,
            ctx.previousPointX,
            ctx.previousPointY,
            state.sensorRows,
            state.sensorCols,
            snappedX,
            snappedY);
        state.output.finalCoor.dim1 = static_cast<int32_t>(snappedX);
        state.output.finalCoor.dim2 = static_cast<int32_t>(snappedY);
        return state.output.finalCoor;
    }

    /// Detect coordinate jump noise.
    /// @param coor  Current coordinate
    /// @return true if noise jump detected (caller should freeze output)
    inline bool DetectNoiseJump(const AsaCoorResult& coor) {
        if (!noisePostEnabled) return false;
        if (!coor.valid) return false;

        const float cx = static_cast<float>(coor.dim1);
        const float cy = static_cast<float>(coor.dim2);

        if (m_prevValidPoint) {
            const float dx = cx - m_prevValidX;
            const float dy = cy - m_prevValidY;
            if (dx * dx + dy * dy > coorJumpThreshold * coorJumpThreshold)
                return true; // noise jump detected
        }
        m_prevValidX = cx;
        m_prevValidY = cy;
        m_prevValidPoint = true;
        return false;
    }

    /// Check if pen exit smoothing should activate.
    /// @param wasInking   Previous frame had pressure > 0
    /// @param hasGoodFrame  Whether a last-good frame exists
    /// @param releaseLike  Whether current lifecycle is a release transition
    /// @param weakRecheck  Whether current frame failed/weakly passed recheck
    /// @return true if caller should output the frozen frame
    inline bool ShouldExitSmooth(bool wasInking, bool hasGoodFrame,
                                 bool releaseLike = true,
                                 bool weakRecheck = false) const {
        if (!exitSmoothEnabled) return false;
        if (!wasInking && !releaseLike) return false;
        if (!hasGoodFrame) return false;
        if (weakRecheck) return true;
        return wasInking || releaseLike;
    }

    /// Apply edge coordinate snapping for pen exit.
    /// Modifies the frozen frame's coordinates if pen exited at a panel edge.
    /// @param lastX/lastY  Last good frame coordinates
    /// @param prevX/prevY  Previous frame coordinates
    /// @param sensorRows/Cols  Full sensor dimensions
    /// @param[out] outX/outY  Corrected coordinates
    inline void ApplyExitEdgeSnap(float lastX, float lastY,
                                   float prevX, float prevY,
                                   int sensorRows, int sensorCols,
                                   float& outX, float& outY) const {
        outX = lastX;
        outY = lastY;

        const float dimXMax = static_cast<float>(sensorCols) * kCoorUnit;
        const float dimYMax = static_cast<float>(sensorRows) * kCoorUnit;
        const float edgeTh = static_cast<float>(kCoorUnit);

        bool atEdge = (lastX < edgeTh || lastX > dimXMax - edgeTh ||
                       lastY < edgeTh || lastY > dimYMax - edgeTh);

        if (atEdge) {
            float dx = lastX - prevX;
            float dy = lastY - prevY;
            if (dx * dx + dy * dy > 0x200 * 0x200) { // 512² units threshold
                outX = prevX;
                outY = prevY;
            }
        }
    }

    /// Reset noise tracking state
    inline void Reset() {
        m_prevValidX = 0.0f;
        m_prevValidY = 0.0f;
        m_prevValidPoint = false;
    }

    /// Simple signal rechecking (EvaluateRecheck)
    inline bool EvaluateRecheck(uint16_t signalX, int noiseLevel) const {
        if (!recheckEnabled) return true;
        const int sig = static_cast<int>(signalX);
        const int th = (noiseLevel > 2) ? recheckSignalThreshBase * 2
                                        : recheckSignalThreshBase;
        return sig >= th;
    }

    inline bool EvaluateRecheck(uint16_t signalX, uint16_t signalY,
                                uint16_t maxRawPeak, uint16_t baseThreshold,
                                uint16_t sustainThreshold,
                                bool overlapLike) const {
        if (!recheckEnabled) return true;
        const int primary = std::max<int>(signalX, signalY);
        const int secondary = std::min<int>(signalX, signalY);
        if (primary < static_cast<int>(baseThreshold)) {
            return false;
        }
        if (overlapLike && primary < static_cast<int>(sustainThreshold)) {
            return false;
        }
        if (maxRawPeak != 0 && maxRawPeak < static_cast<uint16_t>(baseThreshold / 2)) {
            return false;
        }
        return secondary >= static_cast<int>(baseThreshold / 3) || !overlapLike;
    }

    // ── Configuration ──
    bool  noisePostEnabled = false;
    float coorJumpThreshold = 20.0f;
    bool  exitSmoothEnabled = true;
    bool  recheckEnabled = true;
    int   recheckSignalThreshBase = 120;

private:
    float m_prevValidX = 0.0f;
    float m_prevValidY = 0.0f;
    bool  m_prevValidPoint = false;
};

} // namespace Asa
