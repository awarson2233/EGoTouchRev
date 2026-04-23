#pragma once
#include "AsaTypes.hpp"
#include "StylusFrameState.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>

namespace Asa {

/// CommonModeFilter — Morphological open (erosion→dilation) baseline removal.
///
/// Mirrors TSACore HPP3_CMFProcess / GetCMN.
/// Estimates common-mode baseline noise via 1D morphological opening
/// on each row and column of the 9×9 grid, then subtracts it.
class CommonModeFilter {
public:
    // Cache the raw TX1 grid before CMF so downstream signal analysis can
    // rebuild the TX1 composite from the pre-CMF projection without asking
    // the pipeline to keep an extra frame-local copy.
    inline void Process(Solvers::StylusFrameState& state) {
        CachePreCmfTx1Grid(state.parse.gridData.tx1);
        state.stylus.tx1BlockValid = state.parse.gridData.tx1.valid;
        state.stylus.tx2BlockValid = state.parse.gridData.tx2.valid;

        Apply(state.parse.gridData.tx1.grid);
        if (state.parse.gridData.tx2.valid) {
            Apply(state.parse.gridData.tx2.grid);
        }
    }

    inline void Process(int16_t grid[kGridDim][kGridDim]) const {
        Apply(grid);
    }

    template <typename TPeakDetector>
    inline AsaProjection BuildPreCmfTx1Projection(
            const TPeakDetector& detector,
            const Solvers::StylusFrameState& state) const {
        AsaProjection proj{};
        proj.Clear();
        if (!m_hasPreCmfTx1Grid || !state.tx1.peak.valid) {
            return proj;
        }
        return detector.ProjectTo1D(m_preCmfTx1Grid, state.tx1.peak);
    }

    inline bool HasPreCmfTx1Grid() const {
        return m_hasPreCmfTx1Grid;
    }

    /// Apply CMF to a 9×9 grid in-place
    inline void Apply(int16_t grid[kGridDim][kGridDim]) const {
        if (!enabled) return;
        constexpr int N = kGridDim;
        const int w = std::clamp(windowSize, 1, N - 1);

        // Apply to each row
        for (int r = 0; r < N; ++r) {
            MorphOpen1D(grid[r], N, w);
        }

        // Apply to each column
        for (int c = 0; c < N; ++c) {
            std::array<int16_t, kGridDim> col;
            for (int r = 0; r < N; ++r) {
                col[static_cast<size_t>(r)] = grid[r][c];
            }
            MorphOpen1D(col.data(), N, w);
            for (int r = 0; r < N; ++r) {
                grid[r][c] = col[static_cast<size_t>(r)];
            }
        }
    }

    // ── Configuration ──
    bool enabled = false;
    int  windowSize = 6;  // erosion/dilation window half-width

private:
    inline void CachePreCmfTx1Grid(const FreqBlock& tx1) {
        if (!tx1.valid) {
            std::memset(m_preCmfTx1Grid, 0, sizeof(m_preCmfTx1Grid));
            m_hasPreCmfTx1Grid = false;
            return;
        }

        std::memcpy(m_preCmfTx1Grid, tx1.grid, sizeof(m_preCmfTx1Grid));
        m_hasPreCmfTx1Grid = true;
    }

    /// 1D morphological open (erosion→dilation) then subtract
    static inline void MorphOpen1D(int16_t* arr, int len, int w) {
        std::array<int16_t, kGridDim> eroded;
        std::array<int16_t, kGridDim> dilated;

        // For the fixed 9-point stylus grid, a direct bounded scan has lower
        // constant overhead than the generic deque-based sliding window while
        // preserving the exact same symmetric window semantics.
        for (int i = 0; i < len; ++i) {
            const int lo = std::max(0, i - w);
            const int hi = std::min(len - 1, i + w);

            int16_t minVal = arr[lo];
            for (int j = lo + 1; j <= hi; ++j) {
                if (arr[j] < minVal) {
                    minVal = arr[j];
                }
            }
            eroded[static_cast<size_t>(i)] = minVal;
        }

        for (int i = 0; i < len; ++i) {
            const int lo = std::max(0, i - w);
            const int hi = std::min(len - 1, i + w);

            int16_t maxVal = eroded[static_cast<size_t>(lo)];
            for (int j = lo + 1; j <= hi; ++j) {
                if (eroded[static_cast<size_t>(j)] > maxVal) {
                    maxVal = eroded[static_cast<size_t>(j)];
                }
            }
            dilated[static_cast<size_t>(i)] = maxVal;
        }

        // Subtract baseline (clamp to non-negative)
        for (int i = 0; i < len; ++i) {
            arr[i] -= dilated[static_cast<size_t>(i)];
            if (arr[i] < 0) arr[i] = 0;
        }
    }

    int16_t m_preCmfTx1Grid[kGridDim][kGridDim]{};
    bool m_hasPreCmfTx1Grid = false;
};

} // namespace Asa
