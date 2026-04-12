#pragma once
// ── TouchPipeline Module: PalmRejector ──
// Header-only. Converted from TouchSolver/PalmRejector.{h,cpp}.
// Removes palm/fist MacroZones before peak detection.

#include "SolverTypes.h"
#include <array>
#include <vector>
#include <algorithm>
#include <cstdint>

namespace Solvers { namespace Touch {

class PalmRejector {
public:
    static constexpr int kRows = 40;
    static constexpr int kCols = 60;
    static constexpr int kMaxPalmRects = 8;

    struct PalmRect {
        int minR = kRows;
        int maxR = -1;
        int minC = kCols;
        int maxC = -1;

        bool IsValid() const {
            return maxR >= minR && maxC >= minC;
        }
    };

    bool  m_enabled = true;
    int   m_areaThreshold = 41;
    int   m_signalSumThreshold = 65459;
    float m_densityThresholdLow = 282.739f;
    int   m_areaMinForDensity  = 20;
    bool  m_elongatedEnabled    = true;
    int   m_elongatedMinArea    = 10;
    float m_elongatedAspectRatio = 4;
    int   m_lastRejectedCount = 0;

    inline void ResetTemporalState() {
        m_prevPalmRectCount = 0;
        m_nextPalmRectCount = 0;
    }

    inline int Process(std::vector<MacroZone>& macroZones,
                       const HeatmapFrame& /*frame*/) {
        if (!m_enabled) {
            m_lastRejectedCount = 0;
            ResetTemporalState();
            return 0;
        }
        m_lastRejectedCount = 0;
        m_nextPalmRectCount = 0;

        int writePos = 0;
        for (int i = 0; i < static_cast<int>(macroZones.size()); ++i) {
            const MacroZone& zone = macroZones[static_cast<size_t>(i)];
            const bool reject =
                IsStaticPalmZone(zone) ||
                TouchesPreviousPalm(zone);
            if (reject) {
                AddPalmRect(zone);
                ++m_lastRejectedCount;
                continue;
            }
            if (writePos != i) {
                macroZones[static_cast<size_t>(writePos)] = zone;
            }
            ++writePos;
        }

        macroZones.resize(static_cast<size_t>(writePos));
        for (int i = 0; i < m_nextPalmRectCount; ++i) {
            m_prevPalmRects[static_cast<size_t>(i)] =
                m_nextPalmRects[static_cast<size_t>(i)];
        }
        m_prevPalmRectCount = m_nextPalmRectCount;
        return m_lastRejectedCount;
    }

private:
    std::array<PalmRect, kMaxPalmRects> m_prevPalmRects{};
    std::array<PalmRect, kMaxPalmRects> m_nextPalmRects{};
    int m_prevPalmRectCount = 0;
    int m_nextPalmRectCount = 0;

    inline bool IsStaticPalmZone(const MacroZone& zone) const {
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
    }

    inline bool TouchesPreviousPalm(const MacroZone& zone) const {
        if (m_prevPalmRectCount <= 0) return false;

        const PalmRect current = MakeRect(zone);
        for (int i = 0; i < m_prevPalmRectCount; ++i) {
            if (RectsTouch(current, m_prevPalmRects[static_cast<size_t>(i)])) {
                return true;
            }
        }
        return false;
    }

    inline void AddPalmRect(const MacroZone& zone) {
        PalmRect rect = MakeRect(zone);
        if (!rect.IsValid()) return;

        for (int i = 0; i < m_nextPalmRectCount; ++i) {
            PalmRect& existing = m_nextPalmRects[static_cast<size_t>(i)];
            if (RectsAdjacentOrOverlapping(existing, rect)) {
                MergeRects(existing, rect);
                return;
            }
        }

        if (m_nextPalmRectCount < kMaxPalmRects) {
            m_nextPalmRects[static_cast<size_t>(m_nextPalmRectCount++)] = rect;
            return;
        }

        // Cap the worst-case compare cost by coalescing overflow into the last rect.
        MergeRects(m_nextPalmRects[static_cast<size_t>(kMaxPalmRects - 1)], rect);
    }

    static inline PalmRect MakeRect(const MacroZone& zone) {
        PalmRect rect;
        rect.minR = zone.minR;
        rect.maxR = zone.maxR;
        rect.minC = zone.minC;
        rect.maxC = zone.maxC;
        return rect;
    }

    static inline bool RectsTouch(const PalmRect& current, const PalmRect& previous) {
        if (!current.IsValid() || !previous.IsValid()) return false;
        const int prevMinR = std::max(0, previous.minR - 1);
        const int prevMaxR = std::min(kRows - 1, previous.maxR + 1);
        const int prevMinC = std::max(0, previous.minC - 1);
        const int prevMaxC = std::min(kCols - 1, previous.maxC + 1);
        return current.maxR >= prevMinR &&
               current.minR <= prevMaxR &&
               current.maxC >= prevMinC &&
               current.minC <= prevMaxC;
    }

    static inline bool RectsAdjacentOrOverlapping(const PalmRect& a, const PalmRect& b) {
        if (!a.IsValid() || !b.IsValid()) return false;
        return a.maxR >= (b.minR - 1) &&
               a.minR <= (b.maxR + 1) &&
               a.maxC >= (b.minC - 1) &&
               a.minC <= (b.maxC + 1);
    }

    static inline void MergeRects(PalmRect& dst, const PalmRect& src) {
        dst.minR = std::min(dst.minR, src.minR);
        dst.maxR = std::max(dst.maxR, src.maxR);
        dst.minC = std::min(dst.minC, src.minC);
        dst.maxC = std::max(dst.maxC, src.maxC);
    }
};

}} // namespace Solvers::Touch
