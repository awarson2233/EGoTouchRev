#include "ZoneExpander.h"
#include <algorithm>
#include <queue>

namespace Engine {

// ────────────────────────────────────────────────────────
// Public entry — mirrors TSACore TZ_PeakBasedProcess
// ────────────────────────────────────────────────────────
void ZoneExpander::Process(HeatmapFrame& frame,
                           const std::vector<Peak>& peaks,
                           int16_t sigThold) {
    Reset();
    m_units.resize(peaks.size());
    m_edgeInfos.resize(peaks.size());

    // For each peak: flood-fill a zone (absorbed peaks handled later)
    for (int pi = 0; pi < static_cast<int>(peaks.size()); ++pi) {
        const auto& pk = peaks[pi];
        int idx = pk.r * kCols + pk.c;
        if (m_touchZones[idx] != 0) continue; // handled by ScanAbsorbedPeaks

        int16_t zoneThold = CalcZoneThold(sigThold, pk.z);
        m_units[pi] = {};
        m_units[pi].peakCol = pk.c;
        m_units[pi].peakRow = pk.r;
        m_units[pi].peakSig = pk.z;
        m_units[pi].peakIndices.push_back(pi);
        FloodFill(frame, pi, pk, zoneThold);
        m_zoneCount++;
    }

    if (m_dilateErode) DilateAndErode();
    MarkEdges();
    ScanAbsorbedPeaks(peaks);
    ComputeCentroidsAndContacts(frame, peaks);

    // TSACore SigSumFilter_ReserveTouch: keep top-N by signalSum
    if (m_maxTouches > 0 &&
        static_cast<int>(frame.contacts.size()) > m_maxTouches) {
        std::sort(frame.contacts.begin(), frame.contacts.end(),
                  [](const TouchContact& a, const TouchContact& b) {
                      return a.signalSum > b.signalSum;
                  });
        frame.contacts.resize(m_maxTouches);
        // Re-assign IDs after truncation
        for (int i = 0; i < (int)frame.contacts.size(); ++i)
            frame.contacts[i].id = i;
    }
}

void ZoneExpander::Reset() {
    m_touchZones.fill(0);
    m_zoneEdge.fill(0);
    m_units.clear();
    m_edgeInfos.clear();
    m_zoneCount = 0;
}

// ────────────────────────────────────────────────────────
// TZ_GetSigTholdCommon — zone expansion threshold
// result = min(sigThold, peakSig) * numer >> shift  (≈50%)
// ────────────────────────────────────────────────────────
int16_t ZoneExpander::CalcZoneThold(int16_t sigThold,
                                     int16_t peakSig) const {
    int base = std::min(static_cast<int>(sigThold),
                        static_cast<int>(peakSig));
    int result = (base * m_tholdScaleNumer) >> m_tholdScaleShift;
    return static_cast<int16_t>(std::max(1, result));
}

// ────────────────────────────────────────────────────────
// TZ_PeakBasedTraversal — BFS flood-fill from a peak
// Expands to 8-neighbors where signal >= zoneThold.
// Accumulates weighted position sums for centroid.
// ────────────────────────────────────────────────────────
void ZoneExpander::FloodFill(const HeatmapFrame& frame,
                              int peakIdx,
                              const Peak& peak,
                              int16_t zoneThold) {
    static constexpr int dr[] = {-1,-1,-1, 0, 0, 1, 1, 1};
    static constexpr int dc[] = {-1, 0, 1,-1, 1,-1, 0, 1};

    uint8_t zoneId = static_cast<uint8_t>(peakIdx + 1);
    std::queue<int> q;

    int seedIdx = peak.r * kCols + peak.c;
    m_touchZones[seedIdx] = zoneId;
    q.push(seedIdx);

    auto& unit = m_units[peakIdx];
    auto addPixel = [&](int r, int c, int16_t sig) {
        unit.area++;
        unit.signalSum += sig;
        if (sig > 0) {
            // TSACore weighted centroid: pos*128*signal
            unit.weightedColSum += c * 128 * sig;
            unit.weightedRowSum += r * 128 * sig;
            unit.weightTotal += sig;
        }
    };

    addPixel(peak.r, peak.c, peak.z);
    // Seed pixel edge info (flagMask=7 = core)
    TZ_UpdateEdgeInfo(m_edgeInfos[peakIdx], peak.z,
                      peak.c, peak.r, 7);

    while (!q.empty()) {
        int idx = q.front(); q.pop();
        int r = idx / kCols, c = idx % kCols;

        for (int d = 0; d < 8; ++d) {
            int nr = r + dr[d], nc = c + dc[d];
            if (nr < 0 || nr >= kRows || nc < 0 || nc >= kCols)
                continue;
            int ni = nr * kCols + nc;
            int16_t sig = frame.heatmapMatrix[nr][nc];

            if (m_touchZones[ni] != 0) {
                // Already visited — check zone overlap
                uint8_t otherZone = m_touchZones[ni];
                if (otherZone != zoneId) {
                    unit.flags |= 0x4000; // zone overlap
                }
                continue;
            }

            if (sig >= zoneThold) {
                // ★ Core zone: mark + enqueue + accumulate centroid
                m_touchZones[ni] = zoneId;
                addPixel(nr, nc, sig);
                q.push(ni);
                TZ_UpdateEdgeInfo(m_edgeInfos[peakIdx], sig,
                    nc, nr, 7);
            } else if (sig > 0) {
                // ★ Edge zone: mark but do NOT enqueue
                m_touchZones[ni] = zoneId;
                unit.edgeArea++;
                unit.edgeSignalSum += sig;
                TZ_UpdateEdgeInfo(m_edgeInfos[peakIdx], sig,
                    nc, nr, 0);
            }
        }
    }
    // TZ_GetEdgeTouchedFlag — set boundary flags after BFS
    TZ_GetEdgeTouchedFlag(m_edgeInfos[peakIdx]);
}

// ────────────────────────────────────────────────────────
// TZ_DilateAndErode — morphological close on zone map
// Dilate: fill gaps where majority neighbor is a zone
// Erode:  remove dilated pixels that border empty space
// ────────────────────────────────────────────────────────
void ZoneExpander::DilateAndErode() {
    static constexpr int dr[] = {-1,-1,-1, 0, 0, 1, 1, 1};
    static constexpr int dc[] = {-1, 0, 1,-1, 1,-1, 0, 1};

    auto orig = m_touchZones;
    auto dilated = m_touchZones;

    // Dilate: fill empty pixels surrounded by zone pixels
    for (int r = 0; r < kRows; ++r) {
        for (int c = 0; c < kCols; ++c) {
            int idx = r * kCols + c;
            if (m_touchZones[idx] != 0) continue;
            uint8_t counts[22] = {};
            uint8_t best = 0; int bestCnt = 0;
            for (int d = 0; d < 8; ++d) {
                int nr = r + dr[d], nc = c + dc[d];
                if (nr < 0 || nr >= kRows || nc < 0 || nc >= kCols)
                    continue;
                uint8_t z = m_touchZones[nr * kCols + nc];
                if (z == 0 || z > 20) continue;
                if (++counts[z] > bestCnt) {
                    bestCnt = counts[z]; best = z;
                }
            }
            if (bestCnt >= 3) dilated[idx] = best;
        }
    }

    // Erode: remove newly-dilated pixels bordering empty
    m_touchZones = dilated;
    for (int r = 0; r < kRows; ++r) {
        for (int c = 0; c < kCols; ++c) {
            int idx = r * kCols + c;
            if (dilated[idx] == 0 || orig[idx] != 0) continue;
            for (int d = 0; d < 8; ++d) {
                int nr = r + dr[d], nc = c + dc[d];
                if (nr < 0 || nr >= kRows || nc < 0 || nc >= kCols)
                    continue;
                if (dilated[nr * kCols + nc] == 0) {
                    m_touchZones[idx] = 0; break;
                }
            }
        }
    }
}

// ────────────────────────────────────────────────────────
// TZ_PeakInfoRetrieval — scan ALL peaks to find which zone
// they fell into. Adds absorbed peaks to owning zone's list.
// ────────────────────────────────────────────────────────
void ZoneExpander::ScanAbsorbedPeaks(const std::vector<Peak>& peaks) {
    // Build zoneId→unitIndex map (zone ID is uint8_t, so max 256)
    std::array<int, 256> zoneToUnit{};
    zoneToUnit.fill(-1);
    for (int pi = 0; pi < static_cast<int>(m_units.size()); ++pi) {
        if (m_units[pi].area == 0) continue;
        uint8_t zid = static_cast<uint8_t>(pi + 1);
        zoneToUnit[zid] = pi;
    }

    for (int pi = 0; pi < static_cast<int>(peaks.size()); ++pi) {
        const auto& pk = peaks[pi];
        int idx = pk.r * kCols + pk.c;
        uint8_t zid = m_touchZones[idx];
        if (zid == 0) continue;
        int ownerUnit = zoneToUnit[zid];
        if (ownerUnit < 0 || ownerUnit >= (int)m_units.size()) continue;
        // Check if this peak is already registered
        auto& indices = m_units[ownerUnit].peakIndices;
        bool found = false;
        for (int existIdx : indices)
            if (existIdx == pi) { found = true; break; }
        if (!found)
            indices.push_back(pi);
    }
}

// ────────────────────────────────────────────────────────
// Mark zone edge pixels (border between zones or empty)
// ────────────────────────────────────────────────────────
void ZoneExpander::MarkEdges() {
    static constexpr int dr[] = {-1,-1,-1, 0, 0, 1, 1, 1};
    static constexpr int dc[] = {-1, 0, 1,-1, 1,-1, 0, 1};
    m_zoneEdge.fill(0);
    for (int r = 0; r < kRows; ++r) {
        for (int c = 0; c < kCols; ++c) {
            int idx = r * kCols + c;
            uint8_t zid = m_touchZones[idx];
            if (zid == 0) continue;
            for (int d = 0; d < 8; ++d) {
                int nr = r + dr[d], nc = c + dc[d];
                if (nr < 0 || nr >= kRows || nc < 0 || nc >= kCols
                    || m_touchZones[nr * kCols + nc] != zid) {
                    m_zoneEdge[idx] = 1; break;
                }
            }
        }
    }
}

// ────────────────────────────────────────────────────────
// Compute centroids and populate frame.contacts.
// Single-peak zones: standard weighted centroid.
// Multi-peak zones (TZ_MFProcess): 3×3 local centroid per peak.
// ────────────────────────────────────────────────────────
void ZoneExpander::ComputeCentroidsAndContacts(
        HeatmapFrame& frame, const std::vector<Peak>& peaks) {
    frame.contacts.clear();
    for (int pi = 0; pi < static_cast<int>(m_units.size()); ++pi) {
        auto& u = m_units[pi];
        if (u.area == 0 || u.weightTotal == 0) continue;

        // TSACore TZ_GetType: classify zone type
        if (u.peakIndices.size() >= 2) {
            u.type = MF;  // MultiFinger
        } else if (u.area > 9) {
            u.type = FF;  // FatFinger (large single touch)
        } else {
            u.type = NF;  // NormalFinger
        }

        if (u.type != MF) {
            // ── Single peak: standard CTD_BasicCalculation ──
            float cx = static_cast<float>(
                (static_cast<int64_t>(u.weightedColSum) * 2)
                / u.weightTotal + 0x80) / 256.0f;
            float cy = static_cast<float>(
                (static_cast<int64_t>(u.weightedRowSum) * 2)
                / u.weightTotal + 0x80) / 256.0f;

            TouchContact tc;
            tc.id = peaks[pi].id;  // Peak's persistent ID
            tc.x = cx;
            tc.y = cy;
            tc.area = u.area;
            tc.signalSum = u.signalSum;
            tc.state = 0;
            frame.contacts.push_back(tc);
        } else {
            // ── Multi-peak (TZ_MFProcess): CTD_General 3×3 per peak ──
            int sharedArea = u.area / static_cast<int>(u.peakIndices.size());
            int sharedSig  = u.signalSum / static_cast<int>(u.peakIndices.size());

            for (int pkIdx : u.peakIndices) {
                if (pkIdx < 0 || pkIdx >= (int)peaks.size()) continue;
                const Peak& pk = peaks[pkIdx];

                // 3×3 local weighted centroid around peak position
                int64_t wColSum = 0, wRowSum = 0;
                int wTotal = 0;
                for (int dr = -1; dr <= 1; ++dr) {
                    for (int dc = -1; dc <= 1; ++dc) {
                        int nr = pk.r + dr, nc = pk.c + dc;
                        if (nr < 0 || nr >= kRows ||
                            nc < 0 || nc >= kCols) continue;
                        int16_t sig = frame.heatmapMatrix[nr][nc];
                        if (sig <= 0) continue;
                        wColSum += static_cast<int64_t>(nc) * 128 * sig;
                        wRowSum += static_cast<int64_t>(nr) * 128 * sig;
                        wTotal  += sig;
                    }
                }
                if (wTotal == 0) wTotal = 1;

                float cx = static_cast<float>(
                    (wColSum * 2) / wTotal + 0x80) / 256.0f;
                float cy = static_cast<float>(
                    (wRowSum * 2) / wTotal + 0x80) / 256.0f;

                TouchContact tc;
                tc.id = pk.id;  // Peak's persistent ID
                tc.x = cx;
                tc.y = cy;
                tc.area = sharedArea;
                tc.signalSum = sharedSig;
                tc.state = 0;
                frame.contacts.push_back(tc);
            }
        }
    }
}

} // namespace Engine
