#pragma once
// ── TouchPipeline Module: MacroZoneDetector ──
// Header-only. Faithful replica of TouchSolver/MacroZoneDetector.{h,cpp}.
// BFS 8-connected component labeling on the heatmap.
// Optimized: stack-based BFS queue, zone storage reuse across frames.

#include "SolverTypes.h"
#include <array>
#include <vector>
#include <cstdint>
#include <cstring>

namespace Solvers { namespace Touch {

class MacroZoneDetector {
public:
    static constexpr int kRows = 40;
    static constexpr int kCols = 60;
    static constexpr int kGridSize = kRows * kCols; // 2400

    inline void Process(const HeatmapFrame& frame, int threshold) {
        const uint32_t visitEpoch = NextVisitEpoch();
        const int16_t* const heatmap = &frame.heatmapMatrix[0][0];
        m_pixelArenaCount = 0;

        static constexpr int dr[] = {-1, 1, 0, 0, -1, -1, 1, 1};
        static constexpr int dc[] = {0, 0, -1, 1, -1, 1, -1, 1};

        int zoneIdx = 0;

        for (int r = 0; r < kRows; ++r) {
            for (int c = 0; c < kCols; ++c) {
                int idx = r * kCols + c;
                if (m_visitMarks[idx] == visitEpoch) continue;

                if (heatmap[idx] >= threshold) {
                    // Reuse existing zone storage to avoid vector reallocation
                    if (zoneIdx >= static_cast<int>(m_macroZones.size())) {
                        m_macroZones.emplace_back();
                    }
                    auto& zone = m_macroZones[zoneIdx];
                    zone.pixels = {};
                    zone.area = 0;
                    zone.signalSum = 0;
                    zone.minR = kRows - 1;
                    zone.maxR = 0;
                    zone.minC = kCols - 1;
                    zone.maxC = 0;
                    const int zonePixelOffset = m_pixelArenaCount;

                    // Stack-based BFS queue (no heap allocation)
                    m_queueHead = 0;
                    m_queueTail = 0;
                    m_queueBuf[m_queueTail++] = idx;
                    m_visitMarks[idx] = visitEpoch;

                    while (m_queueHead < m_queueTail) {
                        int currIdx = m_queueBuf[m_queueHead++];

                        m_pixelArena[static_cast<size_t>(m_pixelArenaCount++)] = currIdx;
                        zone.area++;

                        int currR = currIdx / kCols;
                        int currC = currIdx % kCols;
                        const int16_t sig = heatmap[currIdx];
                        if (sig > 0) {
                            zone.signalSum += sig;
                        }
                        if (currR < zone.minR) zone.minR = currR;
                        if (currR > zone.maxR) zone.maxR = currR;
                        if (currC < zone.minC) zone.minC = currC;
                        if (currC > zone.maxC) zone.maxC = currC;

                        for (int d = 0; d < 8; ++d) {
                            int nr = currR + dr[d];
                            int nc = currC + dc[d];

                            if (nr >= 0 && nr < kRows && nc >= 0 && nc < kCols) {
                                int nIdx = nr * kCols + nc;
                                if (m_visitMarks[nIdx] != visitEpoch &&
                                    heatmap[nIdx] >= threshold) {
                                    m_visitMarks[nIdx] = visitEpoch;
                                    m_queueBuf[m_queueTail++] = nIdx;
                                }
                            }
                        }
                    }

                    if (zone.area > 0) {
                        zone.pixels = std::span<const int>(
                            m_pixelArena.data() + zonePixelOffset,
                            static_cast<size_t>(zone.area));
                        zoneIdx++;
                    }
                }
            }
        }
        // Trim excess zones (keeps capacity for future frames)
        m_macroZones.resize(zoneIdx);
    }

    const std::vector<MacroZone>& GetMacroZones() const { return m_macroZones; }
    std::vector<MacroZone>& GetMutableMacroZones() { return m_macroZones; }

private:
    std::vector<MacroZone> m_macroZones;
    // Every threshold-passing cell can belong to at most one MacroZone, so a
    // single frame-local arena with kGridSize capacity is enough.
    std::array<int, kGridSize> m_pixelArena{};
    int m_pixelArenaCount = 0;
    uint32_t m_visitMarks[kGridSize] = {};
    uint32_t m_visitEpoch = 0;
    // Pre-allocated BFS queue buffer (max = grid size, no heap alloc)
    int m_queueBuf[kGridSize];
    int m_queueHead = 0;
    int m_queueTail = 0;

    inline uint32_t NextVisitEpoch() {
        ++m_visitEpoch;
        if (m_visitEpoch == 0) {
            std::memset(m_visitMarks, 0, sizeof(m_visitMarks));
            m_visitEpoch = 1;
        }
        return m_visitEpoch;
    }
};

}} // namespace Solvers::Touch
