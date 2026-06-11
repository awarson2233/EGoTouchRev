#pragma once
// ── TouchPipeline Module: ZoneExpander ──
// Header-only. Faithful replica of TouchSolver/ZoneExpander.{h,cpp}.
// TSACore TZ_PeakBasedProcess: BFS flood-fill from peaks → contacts,
// with DilateErode, MarkEdges, ScanAbsorbedPeaks, MultiFinger centroid.

#include "SolverTypes.h"
#include "MSType.hpp"
#include "EdgeCompensation.hpp"  // ZoneEdgeInfo, EdgeBounds, TZ_* helpers
#include <vector>
#include <array>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <span>
#include <utility>

namespace Solvers { namespace Touch {

class ZoneExpander {
public:
    static constexpr int kRows = 40;
    static constexpr int kCols = 60;
    static constexpr int kGridSize = kRows * kCols;
    static constexpr int kMaxTouchScratch = 256;

    int  m_tholdScaleNumer = 0x40; // ~50%  (TSACore DAT)
    int  m_tholdScaleShift = 7;    // >>7
    bool m_dilateErode = true;
    int  m_maxTouches = 10;
    bool m_palmAwareExpansionEnabled = true;
    float m_fingerInPalmThresholdRatio = 0.70f;
    int m_fingerInPalmMaxRadius = 3;
    int m_edgeWidthThreshold = 300;
    EdgeBounds m_edgeBounds;

    // ────────────────────────────────────────────────────────
    // Public entry — mirrors TSACore TZ_PeakBasedProcess
    // ────────────────────────────────────────────────────────
    inline void Process(HeatmapFrame& frame,
                        std::span<const Peak> peaks,
                        int16_t sigThold,
                        std::span<const PeakEvaluation> evaluations = {}) {
        Reset();
        m_units.resize(peaks.size());
        m_edgeInfos.resize(peaks.size());

        // For each peak: flood-fill a zone (absorbed peaks handled later)
        for (int pi = 0; pi < static_cast<int>(peaks.size()); ++pi) {
            if (!AllowContactPeak(evaluations, pi)) continue;
            const auto& pk = peaks[pi];
            int idx = pk.r * kCols + pk.c;
            if (m_touchZones[idx] != 0) continue; // handled by ScanAbsorbedPeaks

            int16_t zoneThold = CalcZoneThold(sigThold, pk.z, evaluations, pi);
            const int maxRadius = CalcMaxRadius(evaluations, pi);
            m_units[pi] = {};
            m_units[pi].peakCol = pk.c;
            m_units[pi].peakRow = pk.r;
            m_units[pi].peakSig = pk.z;
            m_units[pi].zoneThold = zoneThold;
            m_units[pi].peakCount = 0;
            m_units[pi].addPeakIndex(pi);
            FloodFill(frame, pi, pk, zoneThold, maxRadius);
            m_zoneCount++;
        }

        if (m_dilateErode) DilateAndErode();
        MarkEdges();
        ScanAbsorbedPeaks(peaks, evaluations);
        ComputeCentroidsAndContacts(frame, peaks, evaluations);

        // TSACore SigSumFilter_ReserveTouch: keep top-N by signalSum
        if (m_maxTouches > 0 &&
            static_cast<int>(frame.touch.output.contacts.size()) > m_maxTouches) {
            const int contactCount = std::min(static_cast<int>(frame.touch.output.contacts.size()), kGridSize);
            const int keepCount = std::min({m_maxTouches, contactCount, kMaxTouchScratch});
            std::array<int, kGridSize> order;
            for (int i = 0; i < contactCount; ++i) order[static_cast<size_t>(i)] = i;
            std::sort(order.begin(), order.begin() + contactCount, [&](int a, int b) {
                return frame.touch.output.contacts[static_cast<size_t>(a)].signalSum >
                       frame.touch.output.contacts[static_cast<size_t>(b)].signalSum;
            });

            std::array<TouchContact, kMaxTouchScratch> keptContacts;
            std::array<ZoneEdgeInfo, kMaxTouchScratch> keptEdgeInfos;
            for (int i = 0; i < keepCount; ++i) {
                const int src = order[static_cast<size_t>(i)];
                keptContacts[static_cast<size_t>(i)] = frame.touch.output.contacts[static_cast<size_t>(src)];
                keptContacts[static_cast<size_t>(i)].id = i;
                keptEdgeInfos[static_cast<size_t>(i)] = src < static_cast<int>(m_contactEdgeInfos.size())
                    ? m_contactEdgeInfos[static_cast<size_t>(src)]
                    : ZoneEdgeInfo{};
            }

            frame.touch.output.contacts.resize(static_cast<size_t>(keepCount));
            m_contactEdgeInfos.resize(static_cast<size_t>(keepCount));
            for (int i = 0; i < keepCount; ++i) {
                frame.touch.output.contacts[static_cast<size_t>(i)] = keptContacts[static_cast<size_t>(i)];
                m_contactEdgeInfos[static_cast<size_t>(i)] = keptEdgeInfos[static_cast<size_t>(i)];
            }
        }

    }

    const std::vector<ZoneEdgeInfo>& GetEdgeInfos() const { return m_contactEdgeInfos; }
    int GetZoneCount() const { return m_zoneCount; }
    const std::array<uint8_t, kGridSize>& GetTouchZones() const { return m_touchZones; }
    const std::array<uint8_t, kGridSize>& GetZoneEdge() const { return m_zoneEdge; }

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
        int16_t zoneThold = 0;
        static constexpr int kMaxPeaksPerZone = 16;
        int peakIndices[kMaxPeaksPerZone];
        int peakCount = 0;
        void addPeakIndex(int idx) {
            if (peakCount < kMaxPeaksPerZone) peakIndices[peakCount] = idx;
            peakCount++;
        }
        int getPeakCount() const { return std::min(peakCount, kMaxPeaksPerZone); }
    };

    struct MfNode {
        int idx = 0;
        int16_t sig = 0;
        uint8_t owner = 0;
    };

    struct MfAccum {
        int peakIndex = -1;
        int area = 0;
        int signalSum = 0;
        int64_t weightedColSum = 0;
        int64_t weightedRowSum = 0;
        int weightTotal = 0;
    };

    std::array<uint8_t, kGridSize> m_touchZones{};
    std::array<uint8_t, kGridSize> m_zoneEdge{};
    std::vector<ZoneUnit> m_units;
    std::vector<ZoneEdgeInfo> m_edgeInfos;
    std::vector<ZoneEdgeInfo> m_contactEdgeInfos;
    int m_zoneCount = 0;
    std::array<int, kGridSize> m_activeZoneCells{};
    int m_activeZoneCellCount = 0;
    std::array<int, kGridSize> m_dilateCandidates{};
    int m_dilateCandidateCount = 0;
    std::array<int, kGridSize> m_newDilatedCells{};
    int m_newDilatedCellCount = 0;
    std::array<uint8_t, kGridSize> m_candidateMask{};
    // Pre-allocated BFS queue buffer (max = grid size)
    int m_bfsQueue[kGridSize];
    int m_bfsHead = 0, m_bfsTail = 0;
    // Scratch buffer for DilateAndErode (avoids 2x 2400B array copies)
    std::array<uint8_t, kGridSize> m_scratch{};
    std::array<uint8_t, kGridSize> m_mfOwner{};
    std::array<int, kGridSize> m_mfTouched{};
    int m_mfTouchedCount = 0;
    MfNode m_mfHeap[kGridSize];
    int m_mfHeapSize = 0;

    inline void Reset() {
        m_touchZones.fill(0);
        m_zoneEdge.fill(0);
        m_units.clear();
        m_edgeInfos.clear();
        m_contactEdgeInfos.clear();
        m_zoneCount = 0;
        m_activeZoneCellCount = 0;
        m_dilateCandidateCount = 0;
        m_newDilatedCellCount = 0;
        ClearMfScratch();
    }

    inline void RecordZoneCell(int idx) {
        m_activeZoneCells[static_cast<size_t>(m_activeZoneCellCount++)] = idx;
    }

    // ────────────────────────────────────────────────────────
    // TZ_GetSigTholdCommon — zone expansion threshold
    // result = min(sigThold, peakSig) * numer >> shift  (≈50%)
    // ────────────────────────────────────────────────────────
    inline int16_t CalcZoneThold(int16_t sigThold,
                                 int16_t peakSig,
                                 std::span<const PeakEvaluation> evaluations,
                                 int peakIndex) const {
        int base = std::min(static_cast<int>(sigThold),
                            static_cast<int>(peakSig));
        int result = (base * m_tholdScaleNumer) >> m_tholdScaleShift;
        if (peakIndex >= 0 && peakIndex < static_cast<int>(evaluations.size())) {
            const auto& eval = evaluations[static_cast<size_t>(peakIndex)];
            if (m_palmAwareExpansionEnabled && eval.palmClass == PalmClass::FingerLikely &&
                (eval.zonePalmClass == PalmClass::PalmCandidate || eval.zonePalmClass == PalmClass::PalmLikely)) {
                result = std::max(result, static_cast<int>(static_cast<float>(peakSig) * m_fingerInPalmThresholdRatio));
            }
        }
        return static_cast<int16_t>(std::max(1, result));
    }

    inline int CalcMaxRadius(std::span<const PeakEvaluation> evaluations, int peakIndex) const {
        if (!m_palmAwareExpansionEnabled || peakIndex < 0 || peakIndex >= static_cast<int>(evaluations.size())) {
            return 0;
        }
        const auto& eval = evaluations[static_cast<size_t>(peakIndex)];
        if (eval.palmClass == PalmClass::FingerLikely &&
            (eval.zonePalmClass == PalmClass::PalmCandidate || eval.zonePalmClass == PalmClass::PalmLikely)) {
            return m_fingerInPalmMaxRadius;
        }
        return 0;
    }

    inline bool AllowContactPeak(std::span<const PeakEvaluation> evaluations, int peakIndex) const {
        return peakIndex < 0 || peakIndex >= static_cast<int>(evaluations.size()) ||
               evaluations[static_cast<size_t>(peakIndex)].allowContact;
    }

    // ────────────────────────────────────────────────────────
    // TZ_PeakBasedTraversal — BFS flood-fill from a peak
    // Expands to 8-neighbors where signal >= zoneThold.
    // Accumulates weighted position sums for centroid.
    // ────────────────────────────────────────────────────────
    inline void FloodFill(const HeatmapFrame& frame,
                          int peakIdx,
                          const Peak& peak,
                          int16_t zoneThold,
                          int maxRadius) {
        static constexpr int dr[] = {-1,-1,-1, 0, 0, 1, 1, 1};
        static constexpr int dc[] = {-1, 0, 1,-1, 1,-1, 0, 1};

        uint8_t zoneId = static_cast<uint8_t>(peakIdx + 1);

        int seedIdx = peak.r * kCols + peak.c;
        m_touchZones[seedIdx] = zoneId;
        RecordZoneCell(seedIdx);
        // Use member BFS queue buffer (no heap allocation)
        m_bfsHead = 0;
        m_bfsTail = 0;
        m_bfsQueue[m_bfsTail++] = seedIdx;

        auto& unit = m_units[peakIdx];
        auto addPixel = [&](int r, int c, int16_t sig) {
            unit.area++;
            unit.signalSum += sig;
            if (sig > 0) {
                unit.weightedColSum += c * 128 * sig;
                unit.weightedRowSum += r * 128 * sig;
                unit.weightTotal += sig;
            }
        };

        addPixel(peak.r, peak.c, peak.z);
        TZ_UpdateEdgeInfo(m_edgeInfos[peakIdx], peak.z,
                          peak.c, peak.r, 7);

        while (m_bfsHead < m_bfsTail) {
            int idx = m_bfsQueue[m_bfsHead++];
            int r = idx / kCols, c = idx % kCols;

            for (int d = 0; d < 8; ++d) {
                int nr = r + dr[d], nc = c + dc[d];
                if (nr < 0 || nr >= kRows || nc < 0 || nc >= kCols)
                    continue;
                if (maxRadius > 0) {
                    const int rowDist = (nr > peak.r) ? (nr - peak.r) : (peak.r - nr);
                    const int colDist = (nc > peak.c) ? (nc - peak.c) : (peak.c - nc);
                    if (std::max(rowDist, colDist) > maxRadius) continue;
                }
                int ni = nr * kCols + nc;
                int16_t sig = frame.heatmapMatrix[nr][nc];

                if (m_touchZones[ni] != 0) {
                    uint8_t otherZone = m_touchZones[ni];
                    if (otherZone != zoneId) {
                        unit.flags |= 0x4000;
                    }
                    continue;
                }

                if (sig >= zoneThold) {
                    m_touchZones[ni] = zoneId;
                    RecordZoneCell(ni);
                    addPixel(nr, nc, sig);
                    m_bfsQueue[m_bfsTail++] = ni;
                    TZ_UpdateEdgeInfo(m_edgeInfos[peakIdx], sig,
                        nc, nr, 7);
                } else if (sig > 0) {
                    m_touchZones[ni] = zoneId;
                    RecordZoneCell(ni);
                    unit.edgeArea++;
                    unit.edgeSignalSum += sig;
                    TZ_UpdateEdgeInfo(m_edgeInfos[peakIdx], sig,
                        nc, nr, 4);
                }
            }
        }
        TZ_GetEdgeTouchedFlag(m_edgeInfos[peakIdx]);
        TZ_GetEdgeWidth(m_edgeInfos[peakIdx], frame.heatmapMatrix,
                        static_cast<int16_t>(m_edgeWidthThreshold));
    }

    // ────────────────────────────────────────────────────────
    // TZ_DilateAndErode — morphological close on zone map
    // Dilate: fill gaps where majority neighbor is a zone
    // Erode:  remove dilated pixels that border empty space
    // ────────────────────────────────────────────────────────
    inline void DilateAndErode() {
        static constexpr int dr[] = {-1,-1,-1, 0, 0, 1, 1, 1};
        static constexpr int dc[] = {-1, 0, 1,-1, 1,-1, 0, 1};

        // Save original zone state in scratch (single 2400B copy)
        std::memcpy(m_scratch.data(), m_touchZones.data(), kGridSize);
        m_dilateCandidateCount = 0;
        m_newDilatedCellCount = 0;

        // Only empty cells adjacent to an originally occupied zone can be
        // modified by the dilation pass, because votes are read from m_scratch.
        for (int i = 0; i < m_activeZoneCellCount; ++i) {
            const int idx = m_activeZoneCells[static_cast<size_t>(i)];
            const int r = idx / kCols;
            const int c = idx % kCols;
            for (int d = 0; d < 8; ++d) {
                const int nr = r + dr[d];
                const int nc = c + dc[d];
                if (nr < 0 || nr >= kRows || nc < 0 || nc >= kCols) {
                    continue;
                }

                const int ni = nr * kCols + nc;
                if (m_scratch[ni] != 0 || m_candidateMask[ni] != 0) {
                    continue;
                }

                m_candidateMask[ni] = 1;
                m_dilateCandidates[static_cast<size_t>(m_dilateCandidateCount++)] = ni;
            }
        }

        // Dilate: fill empty pixels surrounded by zone pixels.
        for (int i = 0; i < m_dilateCandidateCount; ++i) {
            const int idx = m_dilateCandidates[static_cast<size_t>(i)];
            const int r = idx / kCols;
            const int c = idx % kCols;
            uint8_t counts[22] = {};
            uint8_t best = 0;
            int bestCnt = 0;
            for (int d = 0; d < 8; ++d) {
                const int nr = r + dr[d];
                const int nc = c + dc[d];
                if (nr < 0 || nr >= kRows || nc < 0 || nc >= kCols) {
                    continue;
                }
                const uint8_t z = m_scratch[nr * kCols + nc];
                if (z == 0 || z > 20) {
                    continue;
                }
                if (++counts[z] > bestCnt) {
                    bestCnt = counts[z];
                    best = z;
                }
            }
            if (bestCnt >= 3) {
                m_touchZones[idx] = best;
                m_newDilatedCells[static_cast<size_t>(m_newDilatedCellCount++)] = idx;
            }
        }

        // Erode: remove only the pixels created by the dilation pass.
        for (int i = 0; i < m_newDilatedCellCount; ++i) {
            const int idx = m_newDilatedCells[static_cast<size_t>(i)];
            const int r = idx / kCols;
            const int c = idx % kCols;
            for (int d = 0; d < 8; ++d) {
                const int nr = r + dr[d];
                const int nc = c + dc[d];
                if (nr < 0 || nr >= kRows || nc < 0 || nc >= kCols) {
                    continue;
                }
                if (m_touchZones[nr * kCols + nc] == 0) {
                    m_touchZones[idx] = 0;
                    break;
                }
            }
        }

        for (int i = 0; i < m_dilateCandidateCount; ++i) {
            m_candidateMask[static_cast<size_t>(
                m_dilateCandidates[static_cast<size_t>(i)])] = 0;
        }
    }

    // ────────────────────────────────────────────────────────
    // TZ_PeakInfoRetrieval — scan ALL peaks to find which zone
    // they fell into. Adds absorbed peaks to owning zone's list.
    // ────────────────────────────────────────────────────────
    inline void ScanAbsorbedPeaks(std::span<const Peak> peaks,
                                  std::span<const PeakEvaluation> evaluations) {
        // Build zoneId→unitIndex map (zone ID is uint8_t, so max 256)
        std::array<int, 256> zoneToUnit{};
        zoneToUnit.fill(-1);
        for (int pi = 0; pi < static_cast<int>(m_units.size()); ++pi) {
            if (m_units[pi].area == 0) continue;
            uint8_t zid = static_cast<uint8_t>(pi + 1);
            zoneToUnit[zid] = pi;
        }

        for (int pi = 0; pi < static_cast<int>(peaks.size()); ++pi) {
            if (!AllowContactPeak(evaluations, pi)) continue;
            const auto& pk = peaks[pi];
            int idx = pk.r * kCols + pk.c;
            uint8_t zid = m_touchZones[idx];
            if (zid == 0) continue;
            int ownerUnit = zoneToUnit[zid];
            if (ownerUnit < 0 || ownerUnit >= (int)m_units.size()) continue;
            // Check if this peak is already registered
            bool found = false;
            for (int j = 0; j < m_units[ownerUnit].getPeakCount(); ++j)
                if (m_units[ownerUnit].peakIndices[j] == pi) { found = true; break; }
            if (!found)
                m_units[ownerUnit].addPeakIndex(pi);
        }
    }

    // ────────────────────────────────────────────────────────
    // Mark zone edge pixels (border between zones or empty)
    // ────────────────────────────────────────────────────────
    inline void MarkEdges() {
        static constexpr int dr[] = {-1,-1,-1, 0, 0, 1, 1, 1};
        static constexpr int dc[] = {-1, 0, 1,-1, 1,-1, 0, 1};
        m_zoneEdge.fill(0);

        auto markIfEdge = [&](int idx) {
            const uint8_t zid = m_touchZones[idx];
            if (zid == 0 || m_zoneEdge[idx] != 0) {
                return;
            }

            const int r = idx / kCols;
            const int c = idx % kCols;
            for (int d = 0; d < 8; ++d) {
                const int nr = r + dr[d];
                const int nc = c + dc[d];
                if (nr < 0 || nr >= kRows || nc < 0 || nc >= kCols ||
                    m_touchZones[nr * kCols + nc] != zid) {
                    m_zoneEdge[idx] = 1;
                    break;
                }
            }
        };

        for (int i = 0; i < m_activeZoneCellCount; ++i) {
            markIfEdge(m_activeZoneCells[static_cast<size_t>(i)]);
        }
        for (int i = 0; i < m_newDilatedCellCount; ++i) {
            markIfEdge(m_newDilatedCells[static_cast<size_t>(i)]);
        }
    }

    inline void ClearMfScratch() {
        for (int i = 0; i < m_mfTouchedCount; ++i) {
            m_mfOwner[static_cast<size_t>(m_mfTouched[static_cast<size_t>(i)])] = 0;
        }
        m_mfTouchedCount = 0;
        m_mfHeapSize = 0;
    }

    static inline bool MfNodeLess(const MfNode& a, const MfNode& b) {
        if (a.sig != b.sig) return a.sig < b.sig;
        if (a.owner != b.owner) return a.owner > b.owner;
        return a.idx > b.idx;
    }

    inline void MfHeapPush(MfNode node) {
        if (m_mfHeapSize >= kGridSize) return;
        m_mfHeap[m_mfHeapSize] = node;
        int i = m_mfHeapSize++;
        while (i > 0) {
            const int parent = (i - 1) / 2;
            if (!MfNodeLess(m_mfHeap[parent], m_mfHeap[i])) break;
            std::swap(m_mfHeap[parent], m_mfHeap[i]);
            i = parent;
        }
    }

    inline MfNode MfHeapPop() {
        MfNode top = m_mfHeap[0];
        m_mfHeap[0] = m_mfHeap[--m_mfHeapSize];
        int i = 0;
        while (true) {
            const int left = i * 2 + 1;
            const int right = i * 2 + 2;
            int best = i;
            if (left < m_mfHeapSize && MfNodeLess(m_mfHeap[best], m_mfHeap[left])) best = left;
            if (right < m_mfHeapSize && MfNodeLess(m_mfHeap[best], m_mfHeap[right])) best = right;
            if (best == i) break;
            std::swap(m_mfHeap[i], m_mfHeap[best]);
            i = best;
        }
        return top;
    }

    inline void AssignMfOwner(int idx, uint8_t owner) {
        if (m_mfOwner[static_cast<size_t>(idx)] != 0) return;
        m_mfOwner[static_cast<size_t>(idx)] = owner;
        m_mfTouched[static_cast<size_t>(m_mfTouchedCount++)] = idx;
    }

    inline int PartitionMultiFingerZone(
            const HeatmapFrame& frame,
            int unitIndex,
            const ZoneUnit& unit,
            std::span<const Peak> peaks,
            std::span<const PeakEvaluation> evaluations,
            std::array<MfAccum, ZoneUnit::kMaxPeaksPerZone>& accums) {
        ClearMfScratch();
        for (auto& accum : accums) accum = {};

        const uint8_t ownerZoneId = static_cast<uint8_t>(unitIndex + 1);
        int partitionCount = 0;
        for (int j = 0; j < unit.getPeakCount(); ++j) {
            const int peakIndex = unit.peakIndices[j];
            if (peakIndex < 0 || peakIndex >= static_cast<int>(peaks.size())) continue;
            if (!AllowContactPeak(evaluations, peakIndex)) continue;
            const Peak& peak = peaks[static_cast<size_t>(peakIndex)];
            const int idx = peak.r * kCols + peak.c;
            if (idx < 0 || idx >= kGridSize) continue;
            if (m_touchZones[static_cast<size_t>(idx)] != ownerZoneId) continue;
            if (m_mfOwner[static_cast<size_t>(idx)] != 0) continue;
            if (partitionCount >= ZoneUnit::kMaxPeaksPerZone) break;

            const uint8_t owner = static_cast<uint8_t>(partitionCount + 1);
            accums[static_cast<size_t>(partitionCount)].peakIndex = peakIndex;
            AssignMfOwner(idx, owner);
            MfHeapPush({idx, peak.z, owner});
            ++partitionCount;
        }

        static constexpr int dr[] = {-1,-1,-1, 0, 0, 1, 1, 1};
        static constexpr int dc[] = {-1, 0, 1,-1, 1,-1, 0, 1};

        while (m_mfHeapSize > 0) {
            const MfNode node = MfHeapPop();
            const int r = node.idx / kCols;
            const int c = node.idx % kCols;
            for (int d = 0; d < 8; ++d) {
                const int nr = r + dr[d];
                const int nc = c + dc[d];
                if (nr < 0 || nr >= kRows || nc < 0 || nc >= kCols) continue;
                const int ni = nr * kCols + nc;
                if (m_touchZones[static_cast<size_t>(ni)] != ownerZoneId) continue;
                if (m_mfOwner[static_cast<size_t>(ni)] != 0) continue;
                AssignMfOwner(ni, node.owner);
                MfHeapPush({ni, frame.heatmapMatrix[nr][nc], node.owner});
            }
        }

        for (int i = 0; i < m_mfTouchedCount; ++i) {
            const int idx = m_mfTouched[static_cast<size_t>(i)];
            const uint8_t owner = m_mfOwner[static_cast<size_t>(idx)];
            if (owner == 0 || owner > partitionCount) continue;
            const int r = idx / kCols;
            const int c = idx % kCols;
            const int16_t sig = frame.heatmapMatrix[r][c];
            if (sig < unit.zoneThold) continue;
            auto& accum = accums[static_cast<size_t>(owner - 1)];
            accum.area++;
            accum.signalSum += sig;
            if (sig > 0) {
                accum.weightedColSum += static_cast<int64_t>(c) * 128 * sig;
                accum.weightedRowSum += static_cast<int64_t>(r) * 128 * sig;
                accum.weightTotal += sig;
            }
        }

        ClearMfScratch();
        return partitionCount;
    }

    static inline void ApplyEdgeInfo(TouchContact& contact, const ZoneEdgeInfo& edgeInfo) {
        contact.edgeFlags = edgeInfo.edgeFlags;
        contact.centroidEdgeFlags = TZ_GetCentroidEdgeFlags(edgeInfo, contact.x, contact.y);
        contact.ecWidthX = edgeInfo.colEdgeWidth;
        contact.ecWidthY = edgeInfo.rowEdgeWidth;
        contact.isEdge = (contact.edgeFlags & (0x20 | 0x80000)) != 0 ||
                         contact.centroidEdgeFlags != 0;
    }

    // ────────────────────────────────────────────────────────
    // Compute centroids and populate frame.touch.output.contacts.
    // Single-peak zones: standard weighted centroid.
    // Multi-peak zones: per-peak pixel partition inside merged zone.
    // ────────────────────────────────────────────────────────
    inline void ComputeCentroidsAndContacts(
            HeatmapFrame& frame,
            std::span<const Peak> peaks,
            std::span<const PeakEvaluation> evaluations) {
        frame.touch.output.contacts.clear();
        m_contactEdgeInfos.clear();
        const size_t desiredCapacity = static_cast<size_t>(
            std::max(m_maxTouches, static_cast<int>(peaks.size())));
        if (m_contactEdgeInfos.capacity() < desiredCapacity) {
            m_contactEdgeInfos.reserve(desiredCapacity);
        }
        for (int pi = 0; pi < static_cast<int>(m_units.size()); ++pi) {
            auto& u = m_units[pi];
            if (u.area == 0 || u.weightTotal == 0) continue;

            // TSACore TZ_GetType: classify zone type
            if (u.getPeakCount() >= 2) {
                u.type = MF;  // MultiFinger
            } else if (u.area > 9) {
                u.type = FF;  // FatFinger (large single touch)
            } else {
                u.type = NF;  // NormalFinger
            }

            if (u.type != MF) {
                // ── Single peak: standard CTD_BasicCalculation ──
                const int64_t colFixed =
                    (static_cast<int64_t>(u.weightedColSum) * 2) / u.weightTotal + 0x80;
                const int64_t rowFixed =
                    (static_cast<int64_t>(u.weightedRowSum) * 2) / u.weightTotal + 0x80;
                float cx = static_cast<float>(colFixed) / 256.0f;
                float cy = static_cast<float>(rowFixed) / 256.0f;

                TouchContact tc;
                tc.id = peaks[pi].id;  // Peak's persistent ID
                tc.sourcePeakId = peaks[pi].id;
                tc.sourcePeakAge = static_cast<uint8_t>(std::clamp(peaks[pi].tzAge, 0, 255));
                tc.x = cx;
                tc.y = cy;
                tc.area = u.area;
                tc.signalSum = u.signalSum;
                tc.state = 0;
                ApplyEdgeInfo(tc, m_edgeInfos[static_cast<size_t>(pi)]);
                frame.touch.output.contacts.push_back(tc);
                m_contactEdgeInfos.push_back(m_edgeInfos[static_cast<size_t>(pi)]);
            } else {
                std::array<MfAccum, ZoneUnit::kMaxPeaksPerZone> mfAccums{};
                const int partitionCount = PartitionMultiFingerZone(
                    frame, pi, u, peaks, evaluations, mfAccums);

                for (int j = 0; j < partitionCount; ++j) {
                    const auto& accum = mfAccums[static_cast<size_t>(j)];
                    if (accum.peakIndex < 0 || accum.peakIndex >= static_cast<int>(peaks.size())) continue;
                    if (accum.area == 0) continue;
                    const Peak& pk = peaks[static_cast<size_t>(accum.peakIndex)];

                    float cx = static_cast<float>(pk.c) + 0.5f;
                    float cy = static_cast<float>(pk.r) + 0.5f;
                    if (accum.weightTotal > 0) {
                        const int64_t colFixed = (accum.weightedColSum * 2) / accum.weightTotal + 0x80;
                        const int64_t rowFixed = (accum.weightedRowSum * 2) / accum.weightTotal + 0x80;
                        cx = static_cast<float>(colFixed) / 256.0f;
                        cy = static_cast<float>(rowFixed) / 256.0f;
                    }

                    TouchContact tc;
                    tc.id = pk.id;
                    tc.sourcePeakId = pk.id;
                    tc.sourcePeakAge = static_cast<uint8_t>(std::clamp(pk.tzAge, 0, 255));
                    tc.x = cx;
                    tc.y = cy;
                    tc.area = accum.area;
                    tc.signalSum = accum.signalSum;
                    tc.state = 0;
                    ApplyEdgeInfo(tc, m_edgeInfos[static_cast<size_t>(pi)]);
                    frame.touch.output.contacts.push_back(tc);
                    m_contactEdgeInfos.push_back(m_edgeInfos[static_cast<size_t>(pi)]);
                }
            }
        }
    }
};

}} // namespace Solvers::Touch
