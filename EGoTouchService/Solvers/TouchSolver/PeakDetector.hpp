#pragma once
// ── TouchPipeline Module: PeakDetector ──
// Header-only. Faithful replica of TouchSolver/PeakDetector.{h,cpp}.
// TSACore Peak_Process: detect local maxima, filter, sort, track IDs.

#include "SolverTypes.h"
#include "MSType.hpp"
#include <array>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <span>

namespace Solvers { namespace Touch {

class PeakDetector {
public:
    static constexpr int kMaxStoredPeaks = 100;

    int  m_threshold = 350;
    int  m_sigTholdLimit = 1000;
    bool m_z8Filter = true;
    bool m_z1Filter = true;
    bool m_pressureDriftFilter = true;
    bool m_edgePeakFilter = true;
    bool m_edgeThresholdEnabled = true;
    int  m_edgeThreshold = 300;
    int  m_z8Radius = 1;
    int  m_localMaxRadius = 1;
    bool m_closePeakSaddleFilter = true;
    int  m_closePeakRadius = 2;
    int  m_closePeakMinSaddleDrop = 80;
    float m_closePeakMinSaddleRatio = 0.08f;
    int  m_maxPeaks = 20;
    int  m_pressureDriftDebounceLimit = 3;
    int  m_macroZoneMinArea = 4;

    // ────────────────────────────────────────────────────────
    // Public entry — mirrors TSACore Peak_Process()
    // ────────────────────────────────────────────────────────
    inline void Detect(const HeatmapFrame& frame,
                       const std::vector<MacroZone>& macroZones) {
        // Step 1: DetectInRange — asymmetric local max
        DetectInRange(frame, macroZones);

        if (m_closePeakSaddleFilter) ApplyClosePeakSaddleFilter(frame);

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
        if (m_edgePeakFilter) ApplyEdgePeakFilter(frame);

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

    struct PressureDriftRowStats {
        int gradientSum = 0;
        int signalSum = 0;
        bool hasSharpSpike = false;
    };

    std::array<Peak, kMaxStoredPeaks> m_peaks{};
    std::array<Peak, kMaxStoredPeaks> m_prevPeaks{};
    std::array<PressureDriftRowStats, kRows> m_pressureRows{};
    std::array<uint8_t, kRows> m_pressureRowsReady{};
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
        int weakIdx = -1;
        if (m_pressureDriftFilter) {
            m_pressureRowsReady.fill(0);
        }

        auto refreshWeakIdx = [&]() {
            weakIdx = 0;
            for (int k = 1; k < maxPeaks; ++k) {
                if (m_peaks[static_cast<size_t>(k)].z <
                    m_peaks[static_cast<size_t>(weakIdx)].z) {
                    weakIdx = k;
                }
            }
        };

        const int colEnd = kCols - 1;
        const int localRadius = std::clamp(m_localMaxRadius, 1, 5);
        const int z8Radius = std::clamp(m_z8Radius, 1, 5);

        auto val = [&](int r, int c) -> int16_t {
            return frame.heatmapMatrix[r][c];
        };

        for (int zi = 0; zi < static_cast<int>(macroZones.size()); ++zi) {
            const auto& zone = macroZones[static_cast<size_t>(zi)];
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

                bool isPeak = true;
                for (int dr = -localRadius; dr <= localRadius && isPeak; ++dr) {
                    for (int dc = -localRadius; dc <= localRadius; ++dc) {
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
                }
                if (!isPeak) continue;

                int nbrSigSum = 0;
                for (int dr = -z8Radius; dr <= z8Radius; ++dr) {
                    for (int dc = -z8Radius; dc <= z8Radius; ++dc) {
                        if (dr == 0 && dc == 0) continue;
                        int nr = r + dr;
                        int nc = c + dc;
                        if (nr >= 0 && nr < kRows && nc >= 0 && nc < kCols) {
                            nbrSigSum += val(nr, nc);
                        }
                    }
                }

                // PressureDrift check
                if (m_pressureDriftFilter && DetectPressureDrift(frame, c, r)) {
                    continue;
                }

                // TSACore Peak_Insert: cap at m_maxPeaks, replace weakest
                Peak peak;
                peak.r = r;
                peak.c = c;
                peak.z = v;
                peak.neighborSignalSum = nbrSigSum;
                peak.macroZoneIndex = zi;
                peak.macroZoneArea = zone.area;
                peak.macroZoneSignalSum = zone.signalSum;
                if (m_peakCount < maxPeaks) {
                    m_peaks[static_cast<size_t>(m_peakCount++)] = peak;
                    if (m_peakCount == maxPeaks) refreshWeakIdx();
                } else if (m_peaks[static_cast<size_t>(weakIdx)].z < v) {
                    m_peaks[static_cast<size_t>(weakIdx)] = peak;
                    refreshWeakIdx();
                }
            }
        }
    }

    inline int EstimateSaddleSignal(const HeatmapFrame& frame,
                                    const Peak& a,
                                    const Peak& b) const {
        const int rowDelta = b.r - a.r;
        const int colDelta = b.c - a.c;
        const int steps = std::max(std::abs(rowDelta), std::abs(colDelta));
        auto roundStep = [steps](int delta, int step) {
            const int numerator = delta * step;
            return numerator >= 0
                ? (numerator + steps / 2) / steps
                : (numerator - steps / 2) / steps;
        };
        int saddle = 0;
        for (int step = 1; step < steps; ++step) {
            const int r = a.r + roundStep(rowDelta, step);
            const int c = a.c + roundStep(colDelta, step);
            if (r < 0 || r >= kRows || c < 0 || c >= kCols) continue;
            if ((r == a.r && c == a.c) || (r == b.r && c == b.c)) continue;
            saddle = std::max(saddle, static_cast<int>(frame.heatmapMatrix[r][c]));
        }
        return saddle;
    }

    inline void CompactPeaksBySuppression(const std::array<bool, kMaxStoredPeaks>& suppress) {
        int writeIdx = 0;
        for (int i = 0; i < m_peakCount; ++i) {
            if (suppress[static_cast<size_t>(i)]) continue;
            if (writeIdx != i) {
                m_peaks[static_cast<size_t>(writeIdx)] = m_peaks[static_cast<size_t>(i)];
            }
            ++writeIdx;
        }
        m_peakCount = writeIdx;
    }

    inline void ApplyClosePeakSaddleFilter(const HeatmapFrame& frame) {
        if (m_peakCount <= 1) return;
        const int closeRadius = std::max(1, m_closePeakRadius);
        std::array<bool, kMaxStoredPeaks> suppress{};
        suppress.fill(false);

        for (int i = 0; i < m_peakCount; ++i) {
            if (suppress[static_cast<size_t>(i)]) continue;
            const auto& a = m_peaks[static_cast<size_t>(i)];
            for (int j = i + 1; j < m_peakCount; ++j) {
                if (suppress[static_cast<size_t>(j)]) continue;
                const auto& b = m_peaks[static_cast<size_t>(j)];
                if (a.macroZoneIndex != b.macroZoneIndex) continue;
                const int rowDist = std::abs(a.r - b.r);
                const int colDist = std::abs(a.c - b.c);
                if (std::max(rowDist, colDist) > closeRadius) continue;

                const int weakerIdx = (a.z <= b.z) ? i : j;
                const int weakerSignal = static_cast<int>(m_peaks[static_cast<size_t>(weakerIdx)].z);
                const int saddle = EstimateSaddleSignal(frame, a, b);
                const int requiredDrop = std::max(
                    m_closePeakMinSaddleDrop,
                    static_cast<int>(static_cast<float>(weakerSignal) * m_closePeakMinSaddleRatio));
                if (weakerSignal - saddle < requiredDrop) {
                    suppress[static_cast<size_t>(weakerIdx)] = true;
                    if (weakerIdx == i) break;
                }
            }
        }

        CompactPeaksBySuppression(suppress);
    }

    // ────────────────────────────────────────────────────────
    // PressureDrift_Detect — TSACore gradient-based palm press
    // Returns true if the peak looks like a flat palm press.
    // ────────────────────────────────────────────────────────
    inline void BuildPressureDriftRowStats(
            const HeatmapFrame& frame,
            int row,
            PressureDriftRowStats& stats) const {
        stats = {};
        const int sharpSpikeLimit = m_sigTholdLimit / 3;
        for (int c = 1; c < kCols - 1; ++c) {
            const int grad = std::abs(static_cast<int>(frame.heatmapMatrix[row][c + 1])
                                    - static_cast<int>(frame.heatmapMatrix[row][c - 1]));
            stats.hasSharpSpike = stats.hasSharpSpike || grad > sharpSpikeLimit;
            stats.gradientSum += grad;
            if (frame.heatmapMatrix[row][c] > 0) {
                stats.signalSum += frame.heatmapMatrix[row][c];
            }
        }
    }

    inline bool DetectPressureDrift(const HeatmapFrame& frame,
                                    int col,
                                    int row) {
        const int16_t peakSig = frame.heatmapMatrix[row][col];
        const int16_t limit3_4 = static_cast<int16_t>(m_sigTholdLimit * 3 / 4);
        const int16_t limit3_8 = static_cast<int16_t>(m_sigTholdLimit * 3 / 8);

        if (peakSig > limit3_4 || peakSig < limit3_8) return false;

        if (m_pressureRowsReady[static_cast<size_t>(row)] == 0) {
            BuildPressureDriftRowStats(frame, row, m_pressureRows[static_cast<size_t>(row)]);
            m_pressureRowsReady[static_cast<size_t>(row)] = 1;
        }
        const auto& stats = m_pressureRows[static_cast<size_t>(row)];
        if (stats.hasSharpSpike) return false;
        return (stats.signalSum >= peakSig * 9 / 2) &&
               (peakSig * 6 >= stats.gradientSum);
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
    // W273AS2700 uses the first/last column path.
    // ────────────────────────────────────────────────────────
    inline void ApplyEdgePeakFilter(const HeatmapFrame& frame) {
        auto filterEdgeColumn = [this, &frame](int col) {
            int16_t maxSig = 0;
            for (int row = 0; row < kRows; ++row) {
                maxSig = std::max(maxSig, frame.heatmapMatrix[row][col]);
            }
            if (maxSig == 0) return;
            const int16_t cutoff = static_cast<int16_t>((maxSig >> 3) * 5);
            CompactPeaks([&](const Peak& p) {
                return p.c == col && p.z < cutoff;
            });
        };

        filterEdgeColumn(0);
        filterEdgeColumn(kCols - 1);
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
