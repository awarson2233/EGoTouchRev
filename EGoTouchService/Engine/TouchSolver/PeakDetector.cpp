#include "PeakDetector.h"
#include <algorithm>
#include <cstdlib>

namespace Engine {

// ────────────────────────────────────────────────────────
// Public entry — mirrors TSACore Peak_Process()
// ────────────────────────────────────────────────────────
void PeakDetector::Detect(const HeatmapFrame& frame, const std::vector<MacroZone>& macroZones) {
    // Step 1: DetectInRange — asymmetric 8-neighbor local max
    DetectInRange(frame, macroZones);

    // Step 2: Z8 Filter — signal >> 5 > neighborCount → remove
    if (m_z8Filter) ApplyZ8Filter();

    // Step 3: Z1 Filter — signal < threshold → remove
    if (m_z1Filter) ApplyZ1Filter(m_threshold);

    // Step 3.5: MacroZone min area filter — remove peaks from tiny zones
    if (m_macroZoneMinArea > 1) {
        m_peaks.erase(std::remove_if(m_peaks.begin(), m_peaks.end(),
            [this](const Peak& p) { return p.macroZoneArea < m_macroZoneMinArea; }),
            m_peaks.end());
    }

    // Step 4: Edge peak filter — weak edge peaks < maxSig*5/8
    if (m_edgePeakFilter) ApplyEdgePeakFilter();

    // Step 5: Sort ascending by signal (TSACore default)
    SortPeaks();

    // Step 6: Cap to max
    if (static_cast<int>(m_peaks.size()) > m_maxPeaks)
        m_peaks.resize(m_maxPeaks);

    // Step 7: Peak_IDTracking — assign persistent IDs
    TrackPeakIDs();
}

// ────────────────────────────────────────────────────────
// Peak_DetectInRange — TSACore asymmetric 8-neighbor
// Down neighbors: signal[n] <= peak  (allows equal)
// Up+Left neighbors: signal[n] < peak (strict)
// ────────────────────────────────────────────────────────
void PeakDetector::DetectInRange(const HeatmapFrame& frame, const std::vector<MacroZone>& macroZones) {
    m_peaks.clear();
    m_peaks.reserve(m_maxPeaks + 4);

    const int rowEnd = kRows - 1;
    const int colEnd = kCols - 1;

    auto val = [&](int r, int c) -> int16_t {
         return frame.heatmapMatrix[r][c];
    };

    for (const auto& zone : macroZones) {
        for (int idx : zone.pixels) {
            int r = idx / kCols;
            int c = idx % kCols;

            const int16_t v = val(r, c);

            // Edge-specific threshold (TSACore: g_toeSigThold)
            int16_t thold = m_threshold;
            if (m_edgeThresholdEnabled &&
                (c == 1 || c == colEnd - 1 || r == rowEnd)) {
                thold = m_edgeThreshold;
            }
            if (v < thold) continue;

            // --- Asymmetric Local-Max Test (scales with m_z8Radius) ---
            bool isPeak = true;
            for (int dr = -m_z8Radius; dr <= m_z8Radius; ++dr) {
                for (int dc = -m_z8Radius; dc <= m_z8Radius; ++dc) {
                    if (dr == 0 && dc == 0) continue;
                    int nr = r + dr;
                    int nc = c + dc;
                    if (nr >= 0 && nr < kRows && nc >= 0 && nc < kCols) {
                        int16_t nv = val(nr, nc);
                        bool after = (dr > 0) || (dr == 0 && dc > 0);
                        if (after) {
                            if (nv > v) { isPeak = false; break; }
                        } else {
                            if (nv >= v) { isPeak = false; break; }
                        }
                    }
                }
                if (!isPeak) break;
            }
            if (!isPeak) continue;

            // PressureDrift check
            if (m_pressureDriftFilter &&
                DetectPressureDrift(frame, c, r)) {
                continue;
            }

            // Peak_CalcZn: sum neighbors up to m_z8Radius
            int nbrSigSum = 0;
            for (int dr = -m_z8Radius; dr <= m_z8Radius; ++dr)
                for (int dc = -m_z8Radius; dc <= m_z8Radius; ++dc) {
                    if (dr == 0 && dc == 0) continue;
                    int nr = r + dr, nc = c + dc;
                    if (nr >= 0 && nr < kRows &&
                        nc >= 0 && nc < kCols)
                        nbrSigSum += val(nr, nc);
                }

            // TSACore Peak_Insert: cap at m_maxPeaks, replace weakest
            if (static_cast<int>(m_peaks.size()) < m_maxPeaks) {
                m_peaks.push_back({r, c, v, nbrSigSum, 0, 0, zone.area});
            } else {
                // Buffer full — find weakest peak and replace
                int weakIdx = 0;
                for (int k = 1; k < m_maxPeaks; ++k)
                    if (m_peaks[k].z < m_peaks[weakIdx].z)
                        weakIdx = k;
                if (m_peaks[weakIdx].z < v)
                    m_peaks[weakIdx] = {r, c, v, nbrSigSum, 0, 0, zone.area};
            }
        }
    }
}

// ────────────────────────────────────────────────────────
// PressureDrift_Detect — TSACore gradient-based palm press
// Returns true if the peak looks like a flat palm press.
// ────────────────────────────────────────────────────────
bool PeakDetector::DetectPressureDrift(const HeatmapFrame& frame,
                                       int col, int row) const {
    const int16_t peakSig = frame.heatmapMatrix[row][col];
    const int16_t limit3_4 = static_cast<int16_t>(m_sigTholdLimit * 3 / 4);
    const int16_t limit3_8 = static_cast<int16_t>(m_sigTholdLimit * 3 / 8);

    // Signal range gate: must be in [limit*3/8, limit*3/4]
    if (peakSig > limit3_4 || peakSig < limit3_8)
        return false;

    // Scan entire row: gradient and signal sum
    int gradientSum = 0, rowSignalSum = 0;
    for (int c = 1; c < kCols - 1; ++c) {
        // Gradient = |buf[c+1] - buf[c-1]|  (TSACore: cross-2-column)
        int grad = std::abs(static_cast<int>(frame.heatmapMatrix[row][c + 1])
                          - static_cast<int>(frame.heatmapMatrix[row][c - 1]));
        if (grad > m_sigTholdLimit / 3)
            return false;  // Sharp spike → not drift
        gradientSum += grad;
        if (frame.heatmapMatrix[row][c] > 0)
            rowSignalSum += frame.heatmapMatrix[row][c];
    }

    // Drift condition: rowSum >= peak*9/2 AND peak*6 >= gradSum
    return (rowSignalSum >= peakSig * 9 / 2) &&
           (peakSig * 6 >= gradientSum);
}

// ────────────────────────────────────────────────────────
// Peak_Z8Filter — signal >> 5 > neighborSignalSum → remove
// Isolated spikes: strong peak but neighbors sum is small
// ────────────────────────────────────────────────────────
void PeakDetector::ApplyZ8Filter() {
    m_peaks.erase(
        std::remove_if(m_peaks.begin(), m_peaks.end(),
            [](const Peak& p) {
                return (p.z >> 5) > p.neighborSignalSum;
            }),
        m_peaks.end());
}

// ────────────────────────────────────────────────────────
// Peak_Z1Filter — remove peaks with signal < threshold
// ────────────────────────────────────────────────────────
void PeakDetector::ApplyZ1Filter(int16_t thold) {
    m_peaks.erase(
        std::remove_if(m_peaks.begin(), m_peaks.end(),
            [thold](const Peak& p) { return p.z < thold; }),
        m_peaks.end());
}

// ────────────────────────────────────────────────────────
// EdgePeakFilter_WorkAround — remove weak edge peaks
// On first/last row: peaks with signal < maxSig*5/8
// ────────────────────────────────────────────────────────
void PeakDetector::ApplyEdgePeakFilter() {
    auto filterEdgeLine = [this](auto predicate) {
        // Find max signal among edge peaks
        int16_t maxSig = 0;
        for (auto& p : m_peaks)
            if (predicate(p) && p.z > maxSig) maxSig = p.z;
        if (maxSig == 0) return;
        const int16_t cutoff =
            static_cast<int16_t>((maxSig >> 3) * 5);  // 5/8
        m_peaks.erase(
            std::remove_if(m_peaks.begin(), m_peaks.end(),
                [&](const Peak& p) {
                    return predicate(p) && p.z < cutoff;
                }),
            m_peaks.end());
    };

    // Row 0 (top edge)
    filterEdgeLine([](const Peak& p) { return p.r == 0; });
    // Row last (bottom edge)
    filterEdgeLine([this](const Peak& p) { return p.r == kRows - 1; });
    // Col 0 (left edge)
    filterEdgeLine([](const Peak& p) { return p.c == 0; });
    // Col last (right edge)
    filterEdgeLine([this](const Peak& p) { return p.c == kCols - 1; });
}

// ────────────────────────────────────────────────────────
// Peak sort — ascending by signal (TSACore default)
// ────────────────────────────────────────────────────────
void PeakDetector::SortPeaks() {
    std::sort(m_peaks.begin(), m_peaks.end(),
        [](const Peak& a, const Peak& b) { return a.z < b.z; });
}

// ────────────────────────────────────────────────────────
// Peak_IDTracking — assign persistent IDs across frames.
// TSACore uses IDT_Process(1) (Hungarian); we use greedy
// nearest-neighbor which is sufficient for peak-level tracking.
// ────────────────────────────────────────────────────────
void PeakDetector::TrackPeakIDs() {
    if (m_prevPeaks.empty()) {
        // First frame: assign fresh IDs
        for (auto& pk : m_peaks)
            pk.id = m_nextPeakId++;
        m_prevPeaks = m_peaks;
        return;
    }

    // Mark which prev peaks have been matched
    std::vector<bool> prevUsed(m_prevPeaks.size(), false);

    for (auto& pk : m_peaks) {
        int bestDist = 9999;
        int bestIdx = -1;
        for (int j = 0; j < (int)m_prevPeaks.size(); ++j) {
            if (prevUsed[j]) continue;
            int dist = std::abs(pk.r - m_prevPeaks[j].r)
                     + std::abs(pk.c - m_prevPeaks[j].c);
            if (dist < bestDist) {
                bestDist = dist;
                bestIdx = j;
            }
        }
        // Match if within 3 cells Manhattan distance
        if (bestIdx >= 0 && bestDist <= 3) {
            pk.id = m_prevPeaks[bestIdx].id;
            pk.tzAge = m_prevPeaks[bestIdx].tzAge + 1; // TZ_UpdatePeakTzAge
            prevUsed[bestIdx] = true;
        } else {
            pk.id = m_nextPeakId++;
            pk.tzAge = 0;
        }
    }
    m_prevPeaks = m_peaks;
}

} // namespace Engine
