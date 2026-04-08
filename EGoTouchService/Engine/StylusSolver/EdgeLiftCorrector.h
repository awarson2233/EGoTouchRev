#pragma once
#include "AsaTypes.h"
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
    bool IsEdgeLiftArtifact(
        float curDim1, float curDim2,
        float prevDim1, float prevDim2,
        uint16_t curPressure, uint16_t prevPressure,
        int sensorRows, int sensorCols) const;

    /// Check whether a coordinate is in the edge region.
    /// Used both internally and by StylusPipeline to set isEdge flag.
    bool IsInEdgeRegion(
        float dim1, float dim2,
        int sensorRows, int sensorCols) const;

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
    static constexpr float kCoorUnit = static_cast<float>(Asa::kCoorUnit);
};

} // namespace Asa
