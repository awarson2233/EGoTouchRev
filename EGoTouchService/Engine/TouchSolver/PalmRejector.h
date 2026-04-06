#pragma once

#include "EngineTypes.h"
#include <vector>
#include <cstdint>

namespace Engine {

/// Palm / Elongated-press Rejector — MacroZone-level filtering.
///
/// Runs AFTER MacroZoneDetector and BEFORE PeakDetector.
/// Removes entire MacroZones that look like palm / fist / side-finger contacts,
/// so that no peaks, zones, or contacts are ever generated from them.
///
/// Classification rules:
///   Rule 1: area >= m_areaThreshold → palm
///   Rule 2: total signal >= m_signalSumThreshold → palm
///   Rule 3: area >= m_areaMinForDensity AND density < m_densityThresholdLow → palm
///   Rule 4: area >= m_elongatedMinArea AND bbox aspect ratio >= m_elongatedAspectRatio → elongated press / fist
class PalmRejector {
public:
    /// Filter macroZones in-place: removes zones classified as palm / elongated.
    /// Returns the number of zones rejected.
    int Process(std::vector<MacroZone>& macroZones,
                const HeatmapFrame& frame);

    // ── Tuneable parameters (exposed to UI via FeatureExtractor config) ──
    bool m_enabled = true;

    // --- Palm rules ---
    int m_areaThreshold = 50;
    int m_signalSumThreshold = 80000;
    float m_densityThresholdLow = 400.0f;
    int   m_areaMinForDensity  = 20;

    // --- Elongated / fist rule ---
    // Bounding box aspect ratio = max(w,h) / min(w,h).
    // Typical finger: ~1.0–2.0; Elongated/side press: 3.0+
    bool  m_elongatedEnabled    = true;
    int   m_elongatedMinArea    = 10;   // Minimum area to check elongation
    float m_elongatedAspectRatio = 4.0f; // Aspect ratio threshold

    // Debug: last frame rejection count
    int m_lastRejectedCount = 0;
};

} // namespace Engine
