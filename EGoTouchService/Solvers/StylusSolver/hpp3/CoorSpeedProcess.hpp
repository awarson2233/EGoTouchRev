#pragma once

#include "StylusSolver/AsaTypes.hpp"
#include "SolverTypes.h"

#include <array>
#include <cmath>
#include <cstdint>

namespace Solvers::Stylus {

class CoorSpeedProcess {
public:
    bool m_enabled = true;

    inline void Reset() {
        m_xHistory.fill(kInvalidCoord);
        m_yHistory.fill(kInvalidCoord);
        m_historyHead = 0;
        m_historyCount = 0;
        m_prevPressureActive = false;
    }

    inline void Process(HeatmapFrame& frame) {
        auto& runtime = frame.stylus.runtime;
        const auto& coor = runtime.tx1.coordinate.reportGlobalCoor;
        const bool pressureActive = runtime.pressure.outputPressure > 0;

        if (!m_enabled || !coor.valid) {
            m_prevPressureActive = pressureActive;
            return;
        }

        // Mirror TSACore GetRealTimeCoor2Buf/GetCoorSpeed: speed is derived from the
        // raw mapped coordinate history, not from the already filtered post chain.
        // That keeps IIR coefficient selection from being biased by earlier smoothing.
        PushHistory(coor.dim1, coor.dim2);

        // ── Pairwise distance accumulation (TSACore GetCoorSpeed loop) ──
        int cumulativeDistX10 = 0;  // local_14: sum of sqrt(dx²+dy²)*100, scaled x10
        int lastValidIdx = 0;

        for (int c = 1; c < m_historyCount; ++c) {
            if (HistoryX(c) == kInvalidCoord) {
                continue;  // skip invalid sentinel
            }
            lastValidIdx = c;

            const int32_t dx = HistoryX(c - 1) - HistoryX(c);
            const int32_t dy = HistoryY(c - 1) - HistoryY(c);

            // sqrt((dx² + dy²) * 100)
            const double dist = std::sqrt(static_cast<double>(
                static_cast<int64_t>(dx) * dx + static_cast<int64_t>(dy) * dy) * 100.0);
            cumulativeDistX10 += static_cast<int>(dist);
        }

        if (lastValidIdx == 0) {
            runtime.post.speedValue = 0;
            runtime.post.speedShortAvgDist = 0;
            runtime.post.speedFullAvgDist = 0;
            runtime.post.speedAvgDx = 0;
            runtime.post.speedAvgDy = 0;
            m_prevPressureActive = pressureActive;
            return;
        }

        const int32_t dxFirst = HistoryX(0) - HistoryX(lastValidIdx);
        const int32_t dyFirst = HistoryY(0) - HistoryY(lastValidIdx);

        // Cumulative distance (÷10)
        const int cumDist = (cumulativeDistX10 > 0) ? cumulativeDistX10 / 10 : 1;

        // Average distance per frame
        runtime.post.speedFullAvgDist = cumDist / lastValidIdx;

        int speedShortAvgDist = runtime.post.speedFullAvgDist;
        if (lastValidIdx > 3) {
            int dist3X10 = 0;
            for (int c = 1; c <= 3; ++c) {
                const int32_t dx = HistoryX(c - 1) - HistoryX(c);
                const int32_t dy = HistoryY(c - 1) - HistoryY(c);
                dist3X10 += static_cast<int>(std::sqrt(static_cast<double>(
                    static_cast<int64_t>(dx) * dx + static_cast<int64_t>(dy) * dy) * 100.0));
            }
            speedShortAvgDist = (dist3X10 / 10) / 3;
        }
        runtime.post.speedShortAvgDist = speedShortAvgDist;

        // Average dx/dy over valid points
        runtime.post.speedAvgDx = std::abs(dxFirst) / lastValidIdx;
        runtime.post.speedAvgDy = std::abs(dyFirst) / lastValidIdx;

        // Speed value: 1-frame instant distance (drives IIR coefficient selection)
        int speedInstant = 0;
        if (m_historyCount >= 2 && HistoryX(1) != kInvalidCoord) {
            const int32_t dx = HistoryX(0) - HistoryX(1);
            const int32_t dy = HistoryY(0) - HistoryY(1);
            speedInstant = static_cast<int>(std::sqrt(static_cast<double>(
                static_cast<int64_t>(dx) * dx + static_cast<int64_t>(dy) * dy) * 100.0)) / 10;
        }
        runtime.post.speedValue = speedInstant;

        m_prevPressureActive = pressureActive;
    }

private:
    static constexpr int kHistorySize = 24;
    static constexpr int32_t kInvalidCoord = 0x7FFFFFFF;

    std::array<int32_t, kHistorySize> m_xHistory{};
    std::array<int32_t, kHistorySize> m_yHistory{};
    int m_historyHead = 0;
    int m_historyCount = 0;
    bool m_prevPressureActive = false;

    inline void PushHistory(int32_t x, int32_t y) {
        if (m_historyCount != 0) {
            m_historyHead = (m_historyHead + kHistorySize - 1) % kHistorySize;
        }
        m_xHistory[static_cast<std::size_t>(m_historyHead)] = x;
        m_yHistory[static_cast<std::size_t>(m_historyHead)] = y;
        if (m_historyCount < kHistorySize) {
            ++m_historyCount;
        }
    }

    inline std::size_t HistoryIndex(int logicalIndex) const {
        return static_cast<std::size_t>((m_historyHead + logicalIndex) % kHistorySize);
    }

    inline int32_t HistoryX(int logicalIndex) const {
        return m_xHistory[HistoryIndex(logicalIndex)];
    }

    inline int32_t HistoryY(int logicalIndex) const {
        return m_yHistory[HistoryIndex(logicalIndex)];
    }
};

} // namespace Solvers::Stylus
