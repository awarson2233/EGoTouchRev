#pragma once
// ── TouchPipeline Module: PeakDetector ──
// Header-only. Faithful replica of TouchSolver/PeakDetector.{h,cpp}.
// TSACore Peak_Process: detect local maxima, filter, sort, track IDs.

#include "SolverTypes.h"
#include <array>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <span>

namespace Solvers { namespace Touch {

struct Peak {
    int r = 0, c = 0;
    int16_t z = 0;
    int neighborSignalSum = 0;
    uint8_t id = 0;
    int tzAge = 0;
    int macroZoneArea = 0;
};

class PeakDetector {
public:
    static constexpr int kMaxStoredPeaks = 100;

    int  m_threshold = 130;
    int  m_sigTholdLimit = 300;
    bool m_z8Filter = true;
    bool m_z1Filter = false;
    bool m_pressureDriftFilter = true;
    bool m_edgePeakFilter = true;
    bool m_edgeThresholdEnabled = true;
    int  m_edgeThreshold = 300;
    int  m_z8Radius = 2;
    int  m_maxPeaks = 20;
    int  m_pressureDriftDebounceLimit = 3;
    int  m_macroZoneMinArea = 3;

    // ────────────────────────────────────────────────────────
    // Public entry — mirrors TSACore Peak_Process()
    // ────────────────────────────────────────────────────────
    inline void Detect(const HeatmapFrame& frame,
                       const std::vector<MacroZone>& macroZones) {
        // Step 1: DetectInRange — asymmetric 8-neighbor local max
        DetectInRange(frame, macroZones);

        // Step 2: Z8 Filter — signal >> 5 > neighborSignalSum → remove
        if (m_z8Filter) ApplyZ8Filter();

        // Step 3: Z1 Filter — signal < threshold → remove
        if (m_z1Filter) ApplyZ1Filter(m_threshold);

        // Step 3.5: MacroZone min area filter — remove peaks from tiny zones
        if (m_macroZoneMinArea > 1) {
            CompactPeaks([this](const Peak& p) {
                return p.macroZoneArea < m_macroZoneMinArea;
            });
        }

        // Step 4: Edge peak filter — weak edge peaks < maxSig*5/8
        if (m_edgePeakFilter) ApplyEdgePeakFilter();

        // Step 5: Sort ascending by signal (TSACore default)
        SortPeaks();

        // Step 6: Cap to max
        m_peakCount = std::min(m_peakCount, EffectiveMaxPeaks());

        // Step 7: Peak_IDTracking — assign persistent IDs
        TrackPeakIDs();
    }

    std::span<const Peak> GetPeaks() const {
        return std::span<const Peak>(m_peaks.data(),
                                     static_cast<size_t>(m_peakCount));
    }

private:
    static constexpr int kRows = 40;
    static constexpr int kCols = 60;

    std::array<Peak, kMaxStoredPeaks> m_peaks{};
    std::array<Peak, kMaxStoredPeaks> m_prevPeaks{};
    int m_peakCount = 0;
    int m_prevPeakCount = 0;
    uint8_t m_nextPeakId = 1;

    inline int EffectiveMaxPeaks() const {
        return std::clamp(m_maxPeaks, 1, kMaxStoredPeaks);
    }

    template <typename Pred>
    inline void CompactPeaks(Pred&& pred) {
        int writeIdx = 0;
        for (int i = 0; i < m_peakCount; ++i) {
            if (pred(m_peaks[static_cast<size_t>(i)])) {
                continue;
            }
            if (writeIdx != i) {
                m_peaks[static_cast<size_t>(writeIdx)] =
                    m_peaks[static_cast<size_t>(i)];
            }
            ++writeIdx;
        }
        m_peakCount = writeIdx;
    }

    // ────────────────────────────────────────────────────────
    // Peak_DetectInRange — TSACore asymmetric 8-neighbor
    // Down neighbors: signal[n] <= peak  (allows equal)
    // Up+Left neighbors: signal[n] < peak (strict)
    // ────────────────────────────────────────────────────────
    inline void DetectInRange(const HeatmapFrame& frame,
                              const std::vector<MacroZone>& macroZones) {
        m_peakCount = 0;
        const int maxPeaks = EffectiveMaxPeaks();

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
                    (c == 1 || c == colEnd - 1 || r == kRows - 1)) {
                    thold = m_edgeThreshold;
                }
                if (v < thold) continue;

                // --- Combined Local-Max + Neighbor Sum (single pass) ---
                bool isPeak = true;
                int nbrSigSum = 0;
                for (int dr = -m_z8Radius; dr <= m_z8Radius && isPeak; ++dr) {
                    for (int dc = -m_z8Radius; dc <= m_z8Radius; ++dc) {
                        if (dr == 0 && dc == 0) continue;
                        int nr = r + dr;
                        int nc = c + dc;
                        if (nr >= 0 && nr < kRows && nc >= 0 && nc < kCols) {
                            int16_t nv = val(nr, nc);
                            nbrSigSum += nv;
                            bool after = (dr > 0) || (dr == 0 && dc > 0);
                            if (after) {
                                if (nv > v) { isPeak = false; break; }
                            } else {
                                if (nv >= v) { isPeak = false; break; }
                            }
                        }
                    }
                }
                if (!isPeak) continue;

                // PressureDrift check
                if (m_pressureDriftFilter &&
                    DetectPressureDrift(frame, c, r)) {
                    continue;
                }

                // TSACore Peak_Insert: cap at m_maxPeaks, replace weakest
                if (m_peakCount < maxPeaks) {
                    m_peaks[static_cast<size_t>(m_peakCount++)] =
                        {r, c, v, nbrSigSum, 0, 0, zone.area};
                } else {
                    // Buffer full — find weakest peak and replace
                    int weakIdx = 0;
                    for (int k = 1; k < maxPeaks; ++k)
                        if (m_peaks[static_cast<size_t>(k)].z <
                            m_peaks[static_cast<size_t>(weakIdx)].z)
                            weakIdx = k;
                    if (m_peaks[static_cast<size_t>(weakIdx)].z < v) {
                        m_peaks[static_cast<size_t>(weakIdx)] =
                            {r, c, v, nbrSigSum, 0, 0, zone.area};
                    }
                }
            }
        }
    }

    // ────────────────────────────────────────────────────────
    // PressureDrift_Detect — TSACore gradient-based palm press
    // Returns true if the peak looks like a flat palm press.
    // ────────────────────────────────────────────────────────
    inline bool DetectPressureDrift(const HeatmapFrame& frame,
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
    inline void ApplyZ8Filter() {
        CompactPeaks([](const Peak& p) {
            return (p.z >> 5) > p.neighborSignalSum;
        });
    }

    // ────────────────────────────────────────────────────────
    // Peak_Z1Filter — remove peaks with signal < threshold
    // ────────────────────────────────────────────────────────
    inline void ApplyZ1Filter(int16_t thold) {
        CompactPeaks([thold](const Peak& p) { return p.z < thold; });
    }

    // ────────────────────────────────────────────────────────
    // EdgePeakFilter_WorkAround — remove weak edge peaks
    // On first/last row: peaks with signal < maxSig*5/8
    // ────────────────────────────────────────────────────────
    inline void ApplyEdgePeakFilter() {
        auto filterEdgeLine = [this](auto predicate) {
            // Find max signal among edge peaks
            int16_t maxSig = 0;
            for (int i = 0; i < m_peakCount; ++i) {
                const auto& p = m_peaks[static_cast<size_t>(i)];
                if (predicate(p) && p.z > maxSig) {
                    maxSig = p.z;
                }
            }
            if (maxSig == 0) return;
            const int16_t cutoff =
                static_cast<int16_t>((maxSig >> 3) * 5);  // 5/8
            CompactPeaks([&](const Peak& p) {
                return predicate(p) && p.z < cutoff;
            });
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
    inline void SortPeaks() {
        std::sort(m_peaks.begin(), m_peaks.begin() + m_peakCount,
            [](const Peak& a, const Peak& b) { return a.z < b.z; });
    }

    // ────────────────────────────────────────────────────────
    // Peak_IDTracking — assign persistent IDs across frames.
    // TSACore uses IDT_Process(1) (Hungarian); we use greedy
    // nearest-neighbor which is sufficient for peak-level tracking.
    // ────────────────────────────────────────────────────────
    inline void TrackPeakIDs() {
        if (m_prevPeakCount == 0) {
            // First frame: assign fresh IDs
            for (int i = 0; i < m_peakCount; ++i)
                m_peaks[static_cast<size_t>(i)].id = m_nextPeakId++;
            std::copy_n(m_peaks.begin(), m_peakCount, m_prevPeaks.begin());
            m_prevPeakCount = m_peakCount;
            return;
        }

        // Mark which prev peaks have been matched
        std::array<bool, kMaxStoredPeaks> prevUsed{};
        prevUsed.fill(false);

        for (int i = 0; i < m_peakCount; ++i) {
            auto& pk = m_peaks[static_cast<size_t>(i)];
            int bestDist = 9999;
            int bestIdx = -1;
            for (int j = 0; j < m_prevPeakCount; ++j) {
                if (prevUsed[static_cast<size_t>(j)]) continue;
                const auto& prevPk = m_prevPeaks[static_cast<size_t>(j)];
                int dist = std::abs(pk.r - prevPk.r)
                         + std::abs(pk.c - prevPk.c);
                if (dist < bestDist) {
                    bestDist = dist;
                    bestIdx = j;
                }
            }
            // Match if within 3 cells Manhattan distance
            if (bestIdx >= 0 && bestDist <= 3) {
                const auto& prevPk = m_prevPeaks[static_cast<size_t>(bestIdx)];
                pk.id = prevPk.id;
                pk.tzAge = prevPk.tzAge + 1; // TZ_UpdatePeakTzAge
                prevUsed[static_cast<size_t>(bestIdx)] = true;
            } else {
                pk.id = m_nextPeakId++;
                pk.tzAge = 0;
            }
        }
        std::copy_n(m_peaks.begin(), m_peakCount, m_prevPeaks.begin());
        m_prevPeakCount = m_peakCount;
    }
};

}} // namespace Solvers::Touch
