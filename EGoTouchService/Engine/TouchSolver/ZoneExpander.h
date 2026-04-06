#pragma once

#include "EngineTypes.h"
#include "PeakDetector.h"
#include "EdgeCompensation.h"
#include <array>
#include <cstdint>
#include <vector>

namespace Engine {

/// TSACore-aligned touch zone processor.
/// Replaces self-built BlobDetector + ZoneExpander with
/// official TZ_Process: BFS flood-fill + centroid + contacts.
class ZoneExpander {
public:
    static constexpr int kRows = 40;
    static constexpr int kCols = 60;
    static constexpr int kGridSize = kRows * kCols;
    static constexpr int kMaxTouches = 10;

    /// Main entry — mirrors TSACore TZ_PeakBasedProcess.
    /// Flood-fill from each peak, compute centroids,
    /// populate frame.contacts.
    void Process(HeatmapFrame& frame,
                 const std::vector<Peak>& peaks,
                 int16_t sigThold);

    const auto& GetTouchZones() const { return m_touchZones; }
    const auto& GetZoneEdge()   const { return m_zoneEdge; }
    const auto& GetEdgeInfos()  const { return m_edgeInfos; }
    int GetZoneCount() const { return m_zoneCount; }

    // Tuneable parameters
    int m_tholdScaleNumer = 0x40; // ~50%  (TSACore DAT)
    int m_tholdScaleShift = 7;    // >>7
    bool m_dilateErode = true;
    int m_maxTouches = kMaxTouches;    // SigSumFilter: max contacts (0=unlimited)
    EdgeBounds m_edgeBounds;           // Sensor grid limits for edge processing

private:
    // TSACore TZ_GetType zone classification
    enum ZoneType : uint8_t { NF = 1, FF = 2, MF = 3 };

    struct ZoneUnit {
        int signalSum = 0;      // Total signal in zone (core)
        int weightedColSum = 0; // For centroid X (col)
        int weightedRowSum = 0; // For centroid Y (row)
        int weightTotal = 0;    // Weight denominator
        int area = 0;           // Core pixel count
        int edgeArea = 0;       // Edge pixel count (below zoneThold)
        int edgeSignalSum = 0;  // Edge signal sum
        uint32_t flags = 0;     // Zone flags (0x4000 = overlap)
        ZoneType type = NF;     // TZ_GetType result
        int peakCol = 0, peakRow = 0;
        int16_t peakSig = 0;
        std::vector<int> peakIndices; // All peak indices in this zone
    };

    void Reset();
    int16_t CalcZoneThold(int16_t sigThold, int16_t peakSig) const;
    void FloodFill(const HeatmapFrame& frame, int peakIdx,
                   const Peak& peak, int16_t zoneThold);
    void MarkEdges();
    void DilateAndErode();
    void ScanAbsorbedPeaks(const std::vector<Peak>& peaks);
    void ComputeCentroidsAndContacts(HeatmapFrame& frame,
                                      const std::vector<Peak>& peaks);

    std::array<uint8_t, kGridSize> m_touchZones{};
    std::array<uint8_t, kGridSize> m_zoneEdge{};
    std::vector<ZoneUnit> m_units;
    std::vector<ZoneEdgeInfo> m_edgeInfos;
    int m_zoneCount = 0;
};

} // namespace Engine
