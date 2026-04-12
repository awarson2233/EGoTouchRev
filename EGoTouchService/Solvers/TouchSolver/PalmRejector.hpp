#pragma once
// ── TouchPipeline Module: PalmRejector ──
// Header-only. Converted from TouchSolver/PalmRejector.{h,cpp}.
// Removes palm/fist MacroZones before peak detection.

#include "SolverTypes.h"
#include <vector>
#include <algorithm>
#include <cstdint>

namespace Solvers { namespace Touch {

class PalmRejector {
public:
    bool  m_enabled = true;
    int   m_areaThreshold = 50;
    int   m_signalSumThreshold = 80000;
    float m_densityThresholdLow = 400.0f;
    int   m_areaMinForDensity  = 20;
    bool  m_elongatedEnabled    = true;
    int   m_elongatedMinArea    = 10;
    float m_elongatedAspectRatio = 4.0f;
    int   m_lastRejectedCount = 0;

    inline int Process(std::vector<MacroZone>& macroZones,
                       const HeatmapFrame& /*frame*/) {
        if (!m_enabled) return 0;
        m_lastRejectedCount = 0;

        auto it = std::remove_if(macroZones.begin(), macroZones.end(),
            [&](const MacroZone& zone) -> bool {
                if (zone.area >= m_areaThreshold) return true;

                if (zone.signalSum >= m_signalSumThreshold) return true;

                if (zone.area >= m_areaMinForDensity && zone.area > 0) {
                    float density = static_cast<float>(zone.signalSum) /
                                    static_cast<float>(zone.area);
                    if (density < m_densityThresholdLow) return true;
                }

                if (m_elongatedEnabled && zone.area >= m_elongatedMinArea) {
                    int bboxW = zone.maxC - zone.minC + 1;
                    int bboxH = zone.maxR - zone.minR + 1;
                    float longSide  = static_cast<float>(std::max(bboxW, bboxH));
                    float shortSide = static_cast<float>(std::min(bboxW, bboxH));
                    if (shortSide > 0.0f) {
                        float aspect = longSide / shortSide;
                        if (aspect >= m_elongatedAspectRatio) return true;
                    }
                }
                return false;
            });

        m_lastRejectedCount = static_cast<int>(macroZones.end() - it);
        macroZones.erase(it, macroZones.end());
        return m_lastRejectedCount;
    }
};

}} // namespace Solvers::Touch
