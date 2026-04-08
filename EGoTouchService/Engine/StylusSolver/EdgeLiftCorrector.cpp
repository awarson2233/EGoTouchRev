#include "EdgeLiftCorrector.h"
#include <cmath>

namespace Asa {

bool EdgeLiftCorrector::IsInEdgeRegion(
        float dim1, float dim2,
        int sensorRows, int sensorCols) const {
    const float margin = edgeMarginCells * kCoorUnit;
    const float maxDim1 = static_cast<float>(sensorCols) * kCoorUnit;
    const float maxDim2 = static_cast<float>(sensorRows) * kCoorUnit;

    // Check if within margin cells of any border
    if (dim1 < margin || dim1 > maxDim1 - margin) return true;
    if (dim2 < margin || dim2 > maxDim2 - margin) return true;
    return false;
}

bool EdgeLiftCorrector::IsEdgeLiftArtifact(
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

} // namespace Asa
