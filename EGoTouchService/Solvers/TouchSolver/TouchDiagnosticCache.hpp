#pragma once
// ══════════════════════════════════════════════════════════════════════
// TouchDiagnosticCache.hpp — 诊断数据线程安全缓存
// ══════════════════════════════════════════════════════════════════════

#if EGOTOUCH_DIAG

#include "SolverTypes.h"
#include "PalmBoxSuppressor.hpp"
#include "ContactExtractor.hpp"
#include <mutex>
#include <vector>
#include <array>
#include <algorithm>

namespace Solvers { namespace Touch {

class DiagnosticCache {
public:
    inline void Update(HeatmapFrame& frame,
                       const PalmBoxSuppressor& palmBoxSuppressor,
                       const ContactExtractor& contactExtractor) {
        const auto* macroZonesPtr = frame.touch.runtime.macroZones;
        const auto peaks = frame.touch.runtime.peaks;

        // MacroZone → touchZones colormap and bbox cache for IPC visualization
        frame.touch.debug.touchZones.fill(0);
        frame.touch.debug.zoneBoxes.clear();
        if (macroZonesPtr) {
            const auto& macroZones = *macroZonesPtr;
            for (size_t i = 0; i < macroZones.size(); ++i) {
                const auto& zone = macroZones[i];
                const uint8_t colorId = static_cast<uint8_t>((i % 10) + 1);
                for (int idx : zone.pixels) {
                    if (idx >= 0 && idx < 2400) {
                        frame.touch.debug.touchZones[idx] = colorId;
                    }
                }

                TouchZoneDebugBox box;
                box.zoneId = colorId;
                box.zoneIndex = static_cast<uint8_t>(std::min<size_t>(i, 255));
                box.bbox = {zone.minR, zone.maxR, zone.minC, zone.maxC};
                box.area = zone.area;
                box.signalSum = zone.signalSum;
                frame.touch.debug.zoneBoxes.push_back(box);
            }
        }

        frame.touch.debug.palmBoxes.clear();
        for (const auto& palmBox : palmBoxSuppressor.GetPalmBoxes()) {
            TouchPalmDebugBox box;
            box.id = palmBox.id;
            box.bbox = {palmBox.bbox.minR, palmBox.bbox.maxR, palmBox.bbox.minC, palmBox.bbox.maxC};
            box.expandedBbox = {palmBox.expandedBbox.minR, palmBox.expandedBbox.maxR,
                                palmBox.expandedBbox.minC, palmBox.expandedBbox.maxC};
            box.age = palmBox.age;
            box.missed = palmBox.missed;
            box.lastMatchedZoneIndex = palmBox.lastMatchedZoneIndex;
            box.anchorPeakCount = palmBox.anchorPeakCount;
            box.signalSum = palmBox.signalSum;
            box.matchedPalmThisFrame = palmBox.matchedPalmThisFrame;
            frame.touch.debug.palmBoxes.push_back(box);
        }

        frame.touch.debug.peakZones = contactExtractor.GetPeakZones();

        if (frame.touch.debug.peaks.capacity() < peaks.size()) {
            frame.touch.debug.peaks.reserve(peaks.size());
        }
        frame.touch.debug.peaks.clear();
        for (const auto& pk : peaks) {
            frame.touch.debug.peaks.push_back({pk.r, pk.c, pk.z, pk.id});
        }

        const auto& zoneEdge = contactExtractor.GetZoneEdge();
        const bool touchZonesChanged = frame.touch.debug.touchZones != m_diagTouchZonesPrev;
        const bool zoneEdgeChanged = zoneEdge != m_diagZoneEdgePrev;

        {
            std::lock_guard<std::mutex> lk(m_mtx);
            if (m_diagPeaks.capacity() < peaks.size()) {
                m_diagPeaks.reserve(peaks.size());
            }
            m_diagPeaks.clear();
            for (const auto& pk : peaks) {
                m_diagPeaks.push_back(pk);
            }
            if (touchZonesChanged) {
                m_diagTouchZones = frame.touch.debug.touchZones;
                m_diagTouchZonesPrev = frame.touch.debug.touchZones;
            }
            if (zoneEdgeChanged) {
                m_diagZoneEdge = zoneEdge;
                m_diagZoneEdgePrev = zoneEdge;
            }
        }
    }

    inline void Reset(HeatmapFrame& frame) {
        (void)frame;
        std::lock_guard<std::mutex> lk(m_mtx);
        m_diagPeaks.clear();
        m_diagTouchZones.fill(0);
        m_diagZoneEdge.fill(0);
        m_diagTouchZonesPrev.fill(0);
        m_diagZoneEdgePrev.fill(0);
    }

    inline std::vector<Peak> GetPeaks() const {
        std::lock_guard<std::mutex> lk(m_mtx);
        return m_diagPeaks;
    }

    inline std::array<uint8_t, 2400> GetTouchZones() const {
        std::lock_guard<std::mutex> lk(m_mtx);
        return m_diagTouchZones;
    }

    inline std::array<uint8_t, 2400> GetZoneEdge() const {
        std::lock_guard<std::mutex> lk(m_mtx);
        return m_diagZoneEdge;
    }

private:
    mutable std::mutex m_mtx;
    std::vector<Peak> m_diagPeaks;
    std::array<uint8_t, 2400> m_diagTouchZones{};
    std::array<uint8_t, 2400> m_diagZoneEdge{};
    std::array<uint8_t, 2400> m_diagTouchZonesPrev{};
    std::array<uint8_t, 2400> m_diagZoneEdgePrev{};
};

}} // namespace Solvers::Touch

#endif // EGOTOUCH_DIAG

