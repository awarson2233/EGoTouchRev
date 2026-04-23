#pragma once
#include "AsaTypes.hpp"
#include "StylusFrameState.hpp"

#include <algorithm>
#include <array>

namespace Asa {

/// GridPeakDetector — Flood-fill peak detection on 9×9 grid
/// Mirrors HPP3_FindPeakOfNormalGrid + GetGridTx1Peaks
class GridPeakDetector {
public:
    struct PeakProjectionAnalysis {
        GridPeakUnit peak{};
        AsaProjection projection{};
        uint16_t peakSignal = 0;
    };

    inline PeakProjectionAnalysis Process(
            Solvers::StylusFrameState& state) const {
        ResetProjectionState(state.tx1);
        ResetProjectionState(state.tx2);

        auto tx1Analysis = AnalyzePeakAndProjection(state.parse.gridData.tx1.grid);
        AssignAnalysis(state.tx1, tx1Analysis);

        if (!state.tx1.peak.valid) {
            SetInvalidTx1Terminal(state);
            return tx1Analysis;
        }

        if (state.parse.gridData.tx2.valid) {
            AssignAnalysis(
                state.tx2,
                AnalyzePeakAndProjection(state.parse.gridData.tx2.grid));
        }

        return tx1Analysis;
    }

    inline PeakProjectionAnalysis Process(
            const int16_t grid[kGridDim][kGridDim]) const {
        return AnalyzePeakAndProjection(grid);
    }

    inline PeakProjectionAnalysis AnalyzePeakAndProjection(
            const int16_t grid[kGridDim][kGridDim]) const {
        PeakProjectionAnalysis analysis{};
        analysis.projection.Clear();

        const int16_t* flat = &grid[0][0];
        std::array<uint8_t, kGridSize> visited{};

        for (int idx = 0; idx < kGridSize; ++idx) {
            if (visited[static_cast<size_t>(idx)] != 0) continue;
            if (!IsPeak(flat, idx)) continue;

            const int count = FloodFill(flat, visited, idx);
            if (count >= maxConnected) continue;

            const int32_t nsum = Calc3x3Sum(flat, idx);
            if (nsum > analysis.peak.neighborSum3x3) {
                analysis.peak.peakRow = idx / kGridDim;
                analysis.peak.peakCol = idx % kGridDim;
                analysis.peak.peakValue = flat[idx];
                analysis.peak.neighborSum3x3 = nsum;
                analysis.peak.connectedPixels = count;
                analysis.peak.valid = true;
            }
        }

        if (analysis.peak.valid) {
            analysis.projection = ProjectTo1DFlat(flat, analysis.peak);
            analysis.peakSignal = ClampGridSignal(
                grid[analysis.peak.peakRow][analysis.peak.peakCol]);
        }
        return analysis;
    }

    /// Run flood-fill peak detection on a 9×9 grid
    /// @return Primary peak unit (strongest valid peak)
    inline GridPeakUnit FindPeak(const int16_t grid[kGridDim][kGridDim]) const {
        return AnalyzePeakAndProjection(grid).peak;
    }

    /// Project grid onto 1D signals around the detected peak
    /// @param grid     The 9×9 grid
    /// @param peak     Peak location from FindPeak()
    /// @return Row/column 1D projections with peak indices
    inline AsaProjection ProjectTo1D(
            const int16_t grid[kGridDim][kGridDim],
            const GridPeakUnit& peak) const {
        return ProjectTo1DFlat(&grid[0][0], peak);
    }

    // Configuration
    int   noiseThreshold = 50;     // signal > this to be considered (lowered for bringup)
    int   maxConnected   = 81;     // disabled for bringup (full grid = 9*9)
    int   projRadius     = 1;      // rows/cols around peak for projection

private:
    static inline uint16_t ClampGridSignal(int16_t value) {
        return static_cast<uint16_t>(std::clamp(
            value,
            static_cast<int16_t>(0),
            static_cast<int16_t>(0x7FFF)));
    }

    static inline void ResetProjectionState(Solvers::StylusProjectionState& state) {
        state.peak = {};
        state.projection.Clear();
        state.peakSignal = 0;
        state.localCoor = {};
        state.globalCoor = {};
    }

    static inline void AssignAnalysis(
            Solvers::StylusProjectionState& state,
            const PeakProjectionAnalysis& analysis) {
        state.peak = analysis.peak;
        state.projection = analysis.projection;
        state.peakSignal = analysis.peakSignal;
    }

    static inline void SetInvalidTx1Terminal(Solvers::StylusFrameState& state) {
        state.flow.terminal = true;
        state.flow.pipelineStage = 3;
        state.flow.packetRoute = Solvers::StylusPacketRoute::InvalidZeroState;
        state.flow.clearCommitted = false;
        state.flow.resetPost = false;
        state.flow.resetNoise = false;
    }

    struct FourNeighborList {
        std::array<uint8_t, 4> indices{};
        uint8_t count = 0;
    };

    struct NineNeighborList {
        std::array<uint8_t, 9> indices{};
        uint8_t count = 0;
    };

    static constexpr std::array<FourNeighborList, kGridSize> BuildFourNeighborTable() {
        std::array<FourNeighborList, kGridSize> table{};
        for (int idx = 0; idx < kGridSize; ++idx) {
            FourNeighborList list{};
            const int r = idx / kGridDim;
            const int c = idx % kGridDim;
            if (r > 0) list.indices[list.count++] = static_cast<uint8_t>(idx - kGridDim);
            if (r + 1 < kGridDim) list.indices[list.count++] = static_cast<uint8_t>(idx + kGridDim);
            if (c > 0) list.indices[list.count++] = static_cast<uint8_t>(idx - 1);
            if (c + 1 < kGridDim) list.indices[list.count++] = static_cast<uint8_t>(idx + 1);
            table[static_cast<size_t>(idx)] = list;
        }
        return table;
    }

    static constexpr std::array<NineNeighborList, kGridSize> BuildNineNeighborTable() {
        std::array<NineNeighborList, kGridSize> table{};
        for (int idx = 0; idx < kGridSize; ++idx) {
            NineNeighborList list{};
            const int r = idx / kGridDim;
            const int c = idx % kGridDim;
            for (int dr = -1; dr <= 1; ++dr) {
                const int nr = r + dr;
                if (nr < 0 || nr >= kGridDim) continue;
                for (int dc = -1; dc <= 1; ++dc) {
                    const int nc = c + dc;
                    if (nc < 0 || nc >= kGridDim) continue;
                    list.indices[list.count++] =
                        static_cast<uint8_t>(nr * kGridDim + nc);
                }
            }
            table[static_cast<size_t>(idx)] = list;
        }
        return table;
    }

    inline static const auto kFourNeighbors = BuildFourNeighborTable();
    inline static const auto kNineNeighbors = BuildNineNeighborTable();

    inline bool IsPeak(const int16_t* flat, int idx) const {
        const int16_t val = flat[idx];
        if (val <= noiseThreshold) return false;
        const auto& neighbors = kFourNeighbors[static_cast<size_t>(idx)];
        for (uint8_t i = 0; i < neighbors.count; ++i) {
            if (flat[neighbors.indices[i]] > val) return false;
        }
        return true;
    }

    inline int FloodFill(const int16_t* flat,
                         std::array<uint8_t, kGridSize>& visited,
                         int startIdx) const {
        std::array<uint8_t, kGridSize> stack{};
        int stackSize = 0;
        int regionCount = 0;

        stack[static_cast<size_t>(stackSize++)] = static_cast<uint8_t>(startIdx);
        visited[static_cast<size_t>(startIdx)] = 1;
        while (stackSize > 0) {
            const uint8_t cell = stack[static_cast<size_t>(--stackSize)];
            ++regionCount;
            const auto& neighbors = kFourNeighbors[cell];
            for (uint8_t i = 0; i < neighbors.count; ++i) {
                const uint8_t next = neighbors.indices[i];
                if (visited[next] != 0) continue;
                if (flat[next] <= noiseThreshold) continue;
                visited[next] = 1;
                stack[static_cast<size_t>(stackSize++)] = next;
            }
        }
        return regionCount;
    }

    inline int32_t Calc3x3Sum(const int16_t* flat, int idx) const {
        int32_t sum = 0;
        const auto& neighbors = kNineNeighbors[static_cast<size_t>(idx)];
        for (uint8_t i = 0; i < neighbors.count; ++i)
            sum += flat[neighbors.indices[i]];
        return sum;
    }

    inline AsaProjection ProjectTo1DFlat(const int16_t* flat,
                                         const GridPeakUnit& peak) const {
        AsaProjection proj{};
        proj.Clear();
        if (!peak.valid) return proj;

        const int rMin = std::max(0, peak.peakRow - projRadius);
        const int rMax = std::min(kGridDim - 1, peak.peakRow + projRadius);
        const int cMin = std::max(0, peak.peakCol - projRadius);
        const int cMax = std::min(kGridDim - 1, peak.peakCol + projRadius);
        proj.spanDim1 = rMax - rMin + 1;
        proj.spanDim2 = cMax - cMin + 1;

        for (int c = 0; c < kGridDim; ++c) {
            int32_t sum = 0;
            for (int r = rMin; r <= rMax; ++r) {
                sum += flat[r * kGridDim + c];
            }
            proj.dim1[c] = sum;
        }

        for (int r = 0; r < kGridDim; ++r) {
            int32_t sum = 0;
            const int rowBase = r * kGridDim;
            for (int c = cMin; c <= cMax; ++c) {
                sum += flat[rowBase + c];
            }
            proj.dim2[r] = sum;
        }

        proj.peakIdxDim1 = FindLinePeak(proj.dim1, kGridDim);
        proj.peakIdxDim2 = FindLinePeak(proj.dim2, kGridDim);
        return proj;
    }

    inline int FindLinePeak(const int32_t* signal, int len) const {
        int best = 0;
        for (int i = 1; i < len; ++i)
            if (signal[i] > signal[best]) best = i;
        return (signal[best] > 0) ? best : -1;
    }
};

} // namespace Asa
