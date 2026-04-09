#pragma once
#include "AsaTypes.hpp"
#include "EngineTypes.h"
#include <cmath>
#include <cstdint>

namespace Asa {

/// NoiseGate — HPP3 noise jump detection and pen exit smoothing.
///
/// Two related mechanisms:
///   1. ApplyHpp3NoisePost: Detects large coordinate jumps and triggers frame freeze
///   2. HandlePenExitSmooth: On pen lift while inking, outputs one frozen frame
///      with edge coordinate snapping (TSACore: ReleaseASAReportExitStylus)
class NoiseGate {
public:
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
    /// @return true if caller should output the frozen frame
    inline bool ShouldExitSmooth(bool wasInking, bool hasGoodFrame) const {
        if (!exitSmoothEnabled) return false;
        if (!wasInking) return false;
        if (!hasGoodFrame) return false;
        return true;
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
