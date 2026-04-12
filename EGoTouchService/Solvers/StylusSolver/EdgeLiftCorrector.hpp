#pragma once
#include "AsaTypes.hpp"
#include <cmath>
#include <cstdint>

namespace Asa {

/// EdgeLiftCorrector — Detects and corrects edge high-speed pen lift artifacts.
///
/// Mirrors TSACore EdgeCoorProcess from ASA_HPP3Process.
///
/// Problem: When the pen is lifted quickly at the panel edge, the last 1-2
/// frames often show the coordinate "snapping" back toward the panel center.
/// This is caused by the rapidly weakening signal being dominated by the
/// interior sensor channels.
///
/// Solution: Detect the snap condition and freeze the coordinate/pressure
/// at the last known-good values.
///
/// Detection criteria (all must be true):
///   1. Current pressure == 0, previous pressure != 0  (just lifted)
///   2. Coordinate is in the edge region of the panel
///   3. Coordinate jumped > jumpThreshold from previous frame
///   4. Previous frame was NOT in the edge region
class EdgeLiftCorrector {
public:
    /// Evaluate whether the current frame is an edge-lift artifact.
    /// If so, the caller should freeze coordinates to previous values.
    ///
    /// @param curDim1/curDim2     Current frame coordinates
    /// @param prevDim1/prevDim2   Previous frame coordinates
    /// @param curPressure         Current frame pressure (0 = just lifted)
    /// @param prevPressure        Previous frame pressure
    /// @param sensorRows          Total sensor rows (for edge detection)
    /// @param sensorCols          Total sensor columns (for edge detection)
    /// @return true if this frame should be frozen (edge lift artifact)
    inline bool IsEdgeLiftArtifact(
            float curDim1, float curDim2,
            float prevDim1, float prevDim2,
            uint16_t curPressure, uint16_t prevPressure,
            int sensorRows, int sensorCols) const {
        if (!enabled) return false;

        // Condition 1: pen just lifted (pressure dropped to 0)
        if (curPressure != 0 || prevPressure == 0) return false;

        // Condition 2: current coordinate is in the edge region
        if (!IsInEdgeRegion(curDim1, curDim2, sensorRows, sensorCols))
            return false;

        // Condition 3: coordinate jump exceeds threshold
        const float dx = curDim1 - prevDim1;
        const float dy = curDim2 - prevDim2;
        const float jumpDist = std::sqrt(dx * dx + dy * dy);
        if (jumpDist < jumpThreshold) return false;

        // Condition 4: previous frame was NOT in the edge region
        // (the pen was in the interior and the coordinate "snapped" to the edge)
        if (IsInEdgeRegion(prevDim1, prevDim2, sensorRows, sensorCols))
            return false;

        // All conditions met: this is an edge-lift artifact
        return true;
    }

    /// Check whether a coordinate is in the edge region.
    /// Used both internally and by StylusPipeline to set isEdge flag.
    inline bool IsInEdgeRegion(
            float dim1, float dim2,
            int sensorRows, int sensorCols) const {
        const float margin = edgeMarginCells * kCoorUnitF;
        const float maxDim1 = static_cast<float>(sensorCols) * kCoorUnitF;
        const float maxDim2 = static_cast<float>(sensorRows) * kCoorUnitF;

        // Check if within margin cells of any border
        if (dim1 < margin || dim1 > maxDim1 - margin) return true;
        if (dim2 < margin || dim2 > maxDim2 - margin) return true;
        return false;
    }

    // ── Configuration ──

    /// Edge region width in kCoorUnit cells from each border.
    /// A coordinate within this many cells from the border is "edge".
    float edgeMarginCells = 1.5f;

    /// Minimum coordinate jump (in kCoorUnit) to trigger edge-lift detection.
    /// TSACore uses 0x200 (half a pitch cell = 512 units).
    float jumpThreshold = 512.0f;

    /// Enable/disable the corrector
    bool enabled = true;

private:
    static constexpr float kCoorUnitF = static_cast<float>(Asa::kCoorUnit);
};

} // namespace Asa
