#pragma once
// ── TouchPipeline Module: PalmRejector ──
// Header-only. Converted from TouchSolver/PalmRejector.{h,cpp}.
// Removes palm/fist MacroZones before peak detection.

<<<<<<< HEAD:EGoTouchService/Engine/TouchSolver/PalmRejector.hpp
#include "EngineTypes.h"
=======
#include "SolverTypes.h"
>>>>>>> origin/pr/03-hardware-diagnostics:EGoTouchService/Solvers/TouchSolver/PalmRejector.hpp
#include <vector>
#include <algorithm>
#include <cstdint>

<<<<<<< HEAD:EGoTouchService/Engine/TouchSolver/PalmRejector.hpp
namespace Engine { namespace Touch {
=======
namespace Solvers { namespace Touch {
>>>>>>> origin/pr/03-hardware-diagnostics:EGoTouchService/Solvers/TouchSolver/PalmRejector.hpp

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
<<<<<<< HEAD:EGoTouchService/Engine/TouchSolver/PalmRejector.hpp
                       const HeatmapFrame& frame) {
=======
                       const HeatmapFrame& /*frame*/) {
>>>>>>> origin/pr/03-hardware-diagnostics:EGoTouchService/Solvers/TouchSolver/PalmRejector.hpp
        if (!m_enabled) return 0;
        m_lastRejectedCount = 0;

        auto it = std::remove_if(macroZones.begin(), macroZones.end(),
            [&](const MacroZone& zone) -> bool {
                if (zone.area >= m_areaThreshold) return true;

<<<<<<< HEAD:EGoTouchService/Engine/TouchSolver/PalmRejector.hpp
                int signalSum = 0;
                int minR = 39, maxR = 0, minC = 59, maxC = 0;
                for (int idx : zone.pixels) {
                    int r = idx / 60, c = idx % 60;
                    int16_t sig = frame.heatmapMatrix[r][c];
                    if (sig > 0) signalSum += sig;
                    if (r < minR) minR = r;
                    if (r > maxR) maxR = r;
                    if (c < minC) minC = c;
                    if (c > maxC) maxC = c;
                }

                if (signalSum >= m_signalSumThreshold) return true;

                if (zone.area >= m_areaMinForDensity && zone.area > 0) {
                    float density = static_cast<float>(signalSum) /
=======
                if (zone.signalSum >= m_signalSumThreshold) return true;

                if (zone.area >= m_areaMinForDensity && zone.area > 0) {
                    float density = static_cast<float>(zone.signalSum) /
>>>>>>> origin/pr/03-hardware-diagnostics:EGoTouchService/Solvers/TouchSolver/PalmRejector.hpp
                                    static_cast<float>(zone.area);
                    if (density < m_densityThresholdLow) return true;
                }

                if (m_elongatedEnabled && zone.area >= m_elongatedMinArea) {
<<<<<<< HEAD:EGoTouchService/Engine/TouchSolver/PalmRejector.hpp
                    int bboxW = maxC - minC + 1;
                    int bboxH = maxR - minR + 1;
=======
                    int bboxW = zone.maxC - zone.minC + 1;
                    int bboxH = zone.maxR - zone.minR + 1;
>>>>>>> origin/pr/03-hardware-diagnostics:EGoTouchService/Solvers/TouchSolver/PalmRejector.hpp
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

<<<<<<< HEAD:EGoTouchService/Engine/TouchSolver/PalmRejector.hpp
}} // namespace Engine::Touch
=======
}} // namespace Solvers::Touch
>>>>>>> origin/pr/03-hardware-diagnostics:EGoTouchService/Solvers/TouchSolver/PalmRejector.hpp
