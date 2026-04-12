#pragma once
// ── TouchPipeline Module: EdgeRejection ──
// Header-only. Converted from TouchSolver/EdgeRejection.{h,cpp}.
// Suppresses touch-down events at sensor boundaries.

#include "EngineTypes.h"
#include "EdgeCompensation.h"
#include <vector>
#include <cstdint>

namespace Engine { namespace Touch {

class EdgeRejector {
public:
    bool m_enabled = true;
    int  m_moveInDelay = 2;
    int  m_edgeMargin = 2;

    inline void Process(std::vector<TouchContact>& contacts,
                        const std::vector<ZoneEdgeInfo>& edgeInfos,
                        const EdgeBounds& bounds) {
        if (!m_enabled) return;

        for (int i = 0; i < (int)contacts.size(); ++i) {
            if (i >= (int)edgeInfos.size()) break;
            auto& tc = contacts[i];
            auto& ei = edgeInfos[i];

            if (!(ei.edgeFlags & 0x20)) continue;

            bool atEdge =
                ei.minCol <= bounds.colMin + m_edgeMargin ||
                ei.maxCol >= bounds.colMax - m_edgeMargin ||
                ei.minRow <= bounds.rowMin + m_edgeMargin ||
                ei.maxRow >= bounds.rowMax - m_edgeMargin;

            if (!atEdge) continue;

            if (tc.state == 0) {
                tc.debugFlags |= 0x400;
                tc.isReported = false;
            }
        }
    }
};

}} // namespace Engine::Touch
