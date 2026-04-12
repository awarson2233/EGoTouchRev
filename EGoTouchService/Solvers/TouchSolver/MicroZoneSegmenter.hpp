#pragma once
// ── TouchPipeline Module: MicroZoneSegmenter ──
// Header-only. Priority-queue watershed: assigns each MacroZone pixel to the nearest peak
// by signal strength (steepest-ascent flood fill), 8-connected.
// Optimized: pre-allocated heap buffer, no std::priority_queue per-frame allocation.

#include "SolverTypes.h"
#include "PeakDetector.hpp"
#include <vector>
#include <array>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <span>

namespace Solvers { namespace Touch {

class MicroZoneSegmenter {
public:
    static constexpr int kRows = 40;
    static constexpr int kCols = 60;
    static constexpr int kGridSize = kRows * kCols; // 2400

    inline void Process(const HeatmapFrame& frame,
                        const std::vector<MacroZone>& macroZones,
                        std::span<const Peak> peaks) {
        m_peakZones.fill(0);

        // Create a boolean map of all valid grid cells that belong to ANY MacroZone.
        bool validPixels[kGridSize] = {false};
        for (const auto& mz : macroZones) {
            for (int idx : mz.pixels) {
                validPixels[idx] = true;
            }
        }

        // Use pre-allocated heap buffer
        m_heapSize = 0;

        // Seed the heap with all peaks
        for (size_t i = 0; i < peaks.size(); ++i) {
            int idx = peaks[i].r * kCols + peaks[i].c;
            uint8_t zoneId = static_cast<uint8_t>(i + 1); // 1-indexed

            if (validPixels[idx]) {
                m_peakZones[idx] = zoneId;
                heapPush({idx, frame.heatmapMatrix[peaks[i].r][peaks[i].c], zoneId});
            }
        }

        static constexpr int dr[] = {-1, 1, 0, 0, -1, -1, 1, 1};
        static constexpr int dc[] = {0, 0, -1, 1, -1, 1, -1, 1};

        // Run steepest ascent / priority flood fill
        while (m_heapSize > 0) {
            Node curr = heapPop();

            int r = curr.idx / kCols;
            int c = curr.idx % kCols;

            for (int d = 0; d < 8; ++d) {
                int nr = r + dr[d];
                int nc = c + dc[d];

                if (nr >= 0 && nr < kRows && nc >= 0 && nc < kCols) {
                    int nIdx = nr * kCols + nc;

                    if (validPixels[nIdx] && m_peakZones[nIdx] == 0) {
                        m_peakZones[nIdx] = curr.zoneId;
                        heapPush({nIdx, frame.heatmapMatrix[nr][nc], curr.zoneId});
                    }
                }
            }
        }
    }

    const std::array<uint8_t, kGridSize>& GetPeakZones() const {
        return m_peakZones;
    }

private:
    std::array<uint8_t, kGridSize> m_peakZones{};

    // Pre-allocated max-heap buffer (eliminates std::priority_queue heap alloc)
    struct Node {
        int idx;
        int16_t sig;
        uint8_t zoneId;
        bool operator<(const Node& o) const { return sig < o.sig; }
    };
    Node m_heap[kGridSize];
    int m_heapSize = 0;

    inline void heapPush(Node n) {
        if (m_heapSize >= kGridSize) return;
        m_heap[m_heapSize] = n;
        // Sift up
        int i = m_heapSize++;
        while (i > 0) {
            int parent = (i - 1) / 2;
            if (m_heap[parent] < m_heap[i]) {
                std::swap(m_heap[parent], m_heap[i]);
                i = parent;
            } else break;
        }
    }

    inline Node heapPop() {
        Node top = m_heap[0];
        m_heap[0] = m_heap[--m_heapSize];
        // Sift down
        int i = 0;
        while (true) {
            int l = 2 * i + 1, r = 2 * i + 2, largest = i;
            if (l < m_heapSize && m_heap[largest] < m_heap[l]) largest = l;
            if (r < m_heapSize && m_heap[largest] < m_heap[r]) largest = r;
            if (largest == i) break;
            std::swap(m_heap[i], m_heap[largest]);
            i = largest;
        }
        return top;
    }
};

}} // namespace Solvers::Touch
