#pragma once
#include "AsaTypes.hpp"
#include <algorithm>
#include <vector>

namespace Asa {

/// GridPeakDetector — Flood-fill peak detection on 9×9 grid
/// Mirrors HPP3_FindPeakOfNormalGrid + GetGridTx1Peaks
class GridPeakDetector {
public:
    /// Run flood-fill peak detection on a 9×9 grid
    /// @return Primary peak unit (strongest valid peak)
    inline GridPeakUnit FindPeak(const int16_t grid[kGridDim][kGridDim]) {
        GridPeakUnit best{};
        bool visited[kGridDim][kGridDim]{};
        std::vector<std::pair<int,int>> region;

        for (int r = 0; r < kGridDim; ++r) {
            for (int c = 0; c < kGridDim; ++c) {
                if (visited[r][c]) continue;
                if (!IsPeak(grid, r, c)) continue;

                int count = FloodFill(grid, visited, r, c, region);
                if (count >= maxConnected) continue; // noise

                // Compute 3×3 neighbor sum at the peak
                int32_t nsum = Calc3x3Sum(grid, r, c);
                if (nsum > best.neighborSum3x3) {
                    best.peakRow = r;
                    best.peakCol = c;
                    best.peakValue = grid[r][c];
                    best.neighborSum3x3 = nsum;
                    best.connectedPixels = count;
                    best.valid = true;
                }
            }
        }
        return best;
    }

    /// Project grid onto 1D signals around the detected peak
    /// @param grid     The 9×9 grid
    /// @param peak     Peak location from FindPeak()
    /// @return Row/column 1D projections with peak indices
    inline AsaProjection ProjectTo1D(
            const int16_t grid[kGridDim][kGridDim],
            const GridPeakUnit& peak) {
        AsaProjection proj{};
        proj.Clear();
        if (!peak.valid) return proj;

        // Determine row range for column projection (dim1)
        int rMin = std::max(0, peak.peakRow - projRadius);
        int rMax = std::min(kGridDim - 1, peak.peakRow + projRadius);
        // Determine col range for row projection (dim2)
        int cMin = std::max(0, peak.peakCol - projRadius);
        int cMax = std::min(kGridDim - 1, peak.peakCol + projRadius);

        // dim1[c] = sum of grid[rMin..rMax][c] (column signal)
        for (int c = 0; c < kGridDim; ++c) {
            int32_t sum = 0;
            for (int r = rMin; r <= rMax; ++r)
                sum += grid[r][c];
            proj.dim1[c] = sum;
        }

        // dim2[r] = sum of grid[r][cMin..cMax] (row signal)
        for (int r = 0; r < kGridDim; ++r) {
            int32_t sum = 0;
            for (int c = cMin; c <= cMax; ++c)
                sum += grid[r][c];
            proj.dim2[r] = sum;
        }

        proj.peakIdxDim1 = FindLinePeak(proj.dim1, kGridDim);
        proj.peakIdxDim2 = FindLinePeak(proj.dim2, kGridDim);
        return proj;
    }

    // Configuration
    int   noiseThreshold = 50;     // signal > this to be considered (lowered for bringup)
    int   maxConnected   = 81;     // disabled for bringup (full grid = 9*9)
    int   projRadius     = 2;      // rows/cols around peak for projection

private:
    inline bool IsPeak(const int16_t grid[kGridDim][kGridDim],
                       int r, int c) const {
        const int16_t val = grid[r][c];
        if (val <= noiseThreshold) return false;
        // Check 4-connected neighbors
        if (r > 0 && grid[r-1][c] > val) return false;
        if (r < kGridDim-1 && grid[r+1][c] > val) return false;
        if (c > 0 && grid[r][c-1] > val) return false;
        if (c < kGridDim-1 && grid[r][c+1] > val) return false;
        return true;
    }

    inline int FloodFill(const int16_t grid[kGridDim][kGridDim],
                         bool visited[kGridDim][kGridDim],
                         int r, int c,
                         std::vector<std::pair<int,int>>& region) const {
        region.clear();
        // Stack-based flood fill
        std::vector<std::pair<int,int>> stack;
        stack.push_back({r, c});
        visited[r][c] = true;
        while (!stack.empty()) {
            auto [cr, cc] = stack.back();
            stack.pop_back();
            region.push_back({cr, cc});
            // 4-connected expansion
            constexpr int dr[] = {-1, 1, 0, 0};
            constexpr int dc[] = {0, 0, -1, 1};
            for (int d = 0; d < 4; ++d) {
                int nr = cr + dr[d], nc = cc + dc[d];
                if (nr < 0 || nr >= kGridDim ||
                    nc < 0 || nc >= kGridDim) continue;
                if (visited[nr][nc]) continue;
                if (grid[nr][nc] <= noiseThreshold) continue;
                visited[nr][nc] = true;
                stack.push_back({nr, nc});
            }
        }
        return static_cast<int>(region.size());
    }

    inline int32_t Calc3x3Sum(const int16_t grid[kGridDim][kGridDim],
                              int r, int c) const {
        int32_t sum = 0;
        for (int dr = -1; dr <= 1; ++dr)
            for (int dc = -1; dc <= 1; ++dc) {
                int nr = r + dr, nc = c + dc;
                if (nr >= 0 && nr < kGridDim &&
                    nc >= 0 && nc < kGridDim)
                    sum += grid[nr][nc];
            }
        return sum;
    }

    inline int FindLinePeak(const int32_t* signal, int len) const {
        int best = 0;
        for (int i = 1; i < len; ++i)
            if (signal[i] > signal[best]) best = i;
        return (signal[best] > 0) ? best : -1;
    }
};

} // namespace Asa
