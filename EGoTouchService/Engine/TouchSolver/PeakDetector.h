#pragma once

#include "EngineTypes.h"
#include <vector>
#include <cstdint>

namespace Engine {

struct Peak {
    int r, c;
    int16_t z;                // Signal strength
    int     neighborSignalSum; // Sum of 8-neighbor signals (Peak_CalcZn)
    uint8_t id = 0;           // Persistent ID (Peak_IDTracking)
    uint16_t tzAge = 0;       // TZ_UpdatePeakTzAge: frames in same zone
    int     macroZoneArea = 0; // Area of the hosting MacroZone
};

/// TSACore-aligned peak detector.
/// Implements: Peak_DetectInRange (asymmetric 8-neighbor),
///             PressureDrift_Detect, Peak_Z8Filter, Peak_Z1Filter,
///             EdgePeakFilter_WorkAround.
class PeakDetector {
public:
    static constexpr int kRows = 40;
    static constexpr int kCols = 60;

    void Detect(const HeatmapFrame& frame, const std::vector<MacroZone>& macroZones);

    const std::vector<Peak>& GetPeaks() const { return m_peaks; }

    // --- Tuneable parameters (exposed to UI) ---
    int m_threshold            = 800;   // g_sigThold
    int m_sigTholdLimit        = 1200;  // g_sigTholdLimit
    int m_edgeThreshold        = 400;   // g_toeSigThold
    bool m_edgeThresholdEnabled = false;
    bool    m_z8Filter         = true;
    bool    m_z1Filter         = true;
    bool    m_pressureDriftFilter  = true;
    bool    m_edgePeakFilter       = true;
    int     m_z8Radius             = 1;     // Radius for Z8 filter NMS
    int     m_maxPeaks             = 20;    // Maximum number of peaks
    int     m_pressureDriftDebounceLimit = 3; // Debounce frames
    int     m_prevTouchCount       = 0;  // Set by caller each frame
    int     m_macroZoneMinArea     = 2;  // Min MacroZone area for valid peak

private:
    // --- Sub-algorithms (TSACore-aligned) ---
    void DetectInRange(const HeatmapFrame& frame, const std::vector<MacroZone>& macroZones);
    bool DetectPressureDrift(const HeatmapFrame& frame,
                             int col, int row) const;
    void ApplyZ8Filter();
    void ApplyZ1Filter(int16_t thold);
    void ApplyEdgePeakFilter();
    void SortPeaks();
    void TrackPeakIDs();  // TSACore Peak_IDTracking

    std::vector<Peak> m_peaks;
    std::vector<Peak> m_prevPeaks;     // Previous frame peaks for ID tracking
    uint8_t m_nextPeakId = 1;          // Next available peak ID

    // PressureDrift state (persists across frames)
    int      m_pressureDriftDebounce = 0;
    uint64_t m_lastDriftTimestamp    = 0;
};

} // namespace Engine
