#pragma once

#include "SolverTypes.h"
#include "MSType.hpp"
#include "ZoneExpander.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace Solvers { namespace Touch {

class MicroZoneSegmenter {
public:
    static constexpr int kRows = 40;
    static constexpr int kCols = 60;
    static constexpr int kGridSize = kRows * kCols;

    inline void Process(const HeatmapFrame& frame,
                        const std::vector<MacroZone>& macroZones,
                        std::span<const Peak> peaks) {
        m_peakZones.fill(0);

        bool validPixels[kGridSize] = {false};
        for (const auto& mz : macroZones) {
            for (int idx : mz.pixels) {
                validPixels[idx] = true;
            }
        }

        m_heapSize = 0;
        for (size_t i = 0; i < peaks.size(); ++i) {
            int idx = peaks[i].r * kCols + peaks[i].c;
            uint8_t zoneId = static_cast<uint8_t>(i + 1);

            if (validPixels[idx]) {
                m_peakZones[idx] = zoneId;
                heapPush({idx, frame.heatmapMatrix[peaks[i].r][peaks[i].c], zoneId});
            }
        }

        static constexpr int dr[] = {-1, 1, 0, 0, -1, -1, 1, 1};
        static constexpr int dc[] = {0, 0, -1, 1, -1, 1, -1, 1};

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

class ContactExtractor {
public:
    class TouchSizeCalculator {
    public:
        float m_pixelPitchMm = 4.5f;
        int   m_unitPerSigMm2 = 128;
        uint8_t m_fallbackSizeMm = 5;

        inline void Process(std::span<TouchContact> contacts) {
            for (auto& tc : contacts) {
                uint8_t sizeMm = GetSizeInMM(tc.signalSum, m_unitPerSigMm2);
                if (sizeMm == 0) sizeMm = m_fallbackSizeMm;
                tc.sizeMm = static_cast<float>(sizeMm);
            }
        }

    private:
        static inline uint8_t GetSizeInMM(int sigSum, int scale) {
            if (sigSum >= 0x200000) return 0xFF;
            uint8_t r = 1;
            int shifted = sigSum << 10;
            while (r < 15) {
                int threshold = scale * r * (r + r * r);
                if (threshold > shifted) break;
                r++;
            }
            return r;
        }
    };

    MicroZoneSegmenter m_microZoneSeg;
    ZoneExpander m_zoneExp;
    TouchSizeCalculator m_touchSize;

    inline void ProcessDiagnostics(const HeatmapFrame& frame,
                                   const std::vector<MacroZone>& macroZones,
                                   std::span<const Peak> peaks) {
        m_microZoneSeg.Process(frame, macroZones, peaks);
    }

    inline void Process(HeatmapFrame& frame,
                        std::span<const Peak> peaks,
                        int16_t sigThold,
                        std::span<const PeakEvaluation> evaluations = {}) {
        m_zoneExp.Process(frame, peaks, sigThold, evaluations);
        m_touchSize.Process(frame.touch.output.contacts.span());
    }

    const std::array<uint8_t, MicroZoneSegmenter::kGridSize>& GetPeakZones() const { return m_microZoneSeg.GetPeakZones(); }
    std::span<const ZoneEdgeInfo> GetEdgeInfos() const { return m_zoneExp.GetEdgeInfos(); }
    const std::array<uint8_t, ZoneExpander::kGridSize>& GetZoneEdge() const { return m_zoneExp.GetZoneEdge(); }
    const EdgeBounds& GetEdgeBounds() const { return m_zoneExp.m_edgeBounds; }
    int GetZoneCount() const { return m_zoneExp.GetZoneCount(); }
};

}} // namespace Solvers::Touch
