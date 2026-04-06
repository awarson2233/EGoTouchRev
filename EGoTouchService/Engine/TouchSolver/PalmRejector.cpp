#include "PalmRejector.h"
#include <algorithm>

namespace Engine {

int PalmRejector::Process(std::vector<MacroZone>& macroZones,
                          const HeatmapFrame& frame) {
    if (!m_enabled) return 0;

    m_lastRejectedCount = 0;

    auto it = std::remove_if(macroZones.begin(), macroZones.end(),
        [&](const MacroZone& zone) -> bool {
            // Rule 1: Hard area gate
            if (zone.area >= m_areaThreshold) {
                return true;
            }

            // Compute zone signal sum + bounding box from heatmap pixels
            int signalSum = 0;
            int minR = 39, maxR = 0, minC = 59, maxC = 0;
            for (int idx : zone.pixels) {
                int r = idx / 60;
                int c = idx % 60;
                int16_t sig = frame.heatmapMatrix[r][c];
                if (sig > 0) signalSum += sig;
                if (r < minR) minR = r;
                if (r > maxR) maxR = r;
                if (c < minC) minC = c;
                if (c > maxC) maxC = c;
            }

            // Rule 2: Hard signal sum gate
            if (signalSum >= m_signalSumThreshold) {
                return true;
            }

            // Rule 3: Low signal density with significant area
            if (zone.area >= m_areaMinForDensity && zone.area > 0) {
                float density = static_cast<float>(signalSum) /
                                static_cast<float>(zone.area);
                if (density < m_densityThresholdLow) {
                    return true;
                }
            }

            // Rule 4: Elongated press / fist — high bounding box aspect ratio
            if (m_elongatedEnabled && zone.area >= m_elongatedMinArea) {
                int bboxW = maxC - minC + 1;
                int bboxH = maxR - minR + 1;
                float longSide  = static_cast<float>(std::max(bboxW, bboxH));
                float shortSide = static_cast<float>(std::min(bboxW, bboxH));
                if (shortSide > 0.0f) {
                    float aspect = longSide / shortSide;
                    if (aspect >= m_elongatedAspectRatio) {
                        return true;
                    }
                }
            }

            return false;  // Keep this zone (it's a finger)
        });

    m_lastRejectedCount = static_cast<int>(macroZones.end() - it);
    macroZones.erase(it, macroZones.end());

    return m_lastRejectedCount;
}

} // namespace Engine
