#pragma once
#include "AsaTypes.hpp"
#include <algorithm>
#include <array>
#include <cstdint>

namespace Asa {

/// CommonModeFilter — Morphological open (erosion→dilation) baseline removal.
///
/// Mirrors TSACore HPP3_CMFProcess / GetCMN.
/// Estimates common-mode baseline noise via 1D morphological opening
/// on each row and column of the 9×9 grid, then subtracts it.
class CommonModeFilter {
public:
    /// Apply CMF to a 9×9 grid in-place
    inline void Apply(int16_t grid[kGridDim][kGridDim]) const {
        if (!enabled) return;
        constexpr int N = kGridDim;
        const int w = std::clamp(windowSize, 1, N - 1);

        // Apply to each row
        for (int r = 0; r < N; ++r)
            MorphOpen1D(grid[r], N, w);

        // Apply to each column
        for (int c = 0; c < N; ++c) {
            std::array<int16_t, kGridDim> col{};
            for (int r = 0; r < N; ++r) col[static_cast<size_t>(r)] = grid[r][c];
            MorphOpen1D(col.data(), N, w);
            for (int r = 0; r < N; ++r) grid[r][c] = col[static_cast<size_t>(r)];
        }
    }

    // ── Configuration ──
    bool enabled = false;
    int  windowSize = 6;  // erosion/dilation window half-width

private:
    /// 1D morphological open (erosion→dilation) then subtract
    static inline void MorphOpen1D(int16_t* arr, int len, int w) {
        std::array<int16_t, kGridDim> eroded{};
        std::array<int16_t, kGridDim> dilated{};

        // Erosion: min over window [i-w, i+w]
        for (int i = 0; i < len; ++i) {
            int lo = std::max(0, i - w);
            int hi = std::min(len - 1, i + w);
            int16_t minVal = arr[lo];
            for (int j = lo + 1; j <= hi; ++j)
                if (arr[j] < minVal) minVal = arr[j];
            eroded[static_cast<size_t>(i)] = minVal;
        }

        // Dilation: max over erosion result
        for (int i = 0; i < len; ++i) {
            int lo = std::max(0, i - w);
            int hi = std::min(len - 1, i + w);
            int16_t maxVal = eroded[static_cast<size_t>(lo)];
            for (int j = lo + 1; j <= hi; ++j)
                if (eroded[static_cast<size_t>(j)] > maxVal)
                    maxVal = eroded[static_cast<size_t>(j)];
            dilated[static_cast<size_t>(i)] = maxVal;
        }

        // Subtract baseline (clamp to non-negative)
        for (int i = 0; i < len; ++i) {
            arr[i] -= dilated[static_cast<size_t>(i)];
            if (arr[i] < 0) arr[i] = 0;
        }
    }
};

} // namespace Asa
