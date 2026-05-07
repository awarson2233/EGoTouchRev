#pragma once

#include "SolverTypes.h"

#include <algorithm>
#include <array>

namespace Solvers::Stylus {

class GridPeakDetector {
public:
    bool m_enabled = true;
    int m_noiseThreshold = 50;
    int m_maxConnected = 81;
    int m_projectionRadius = 1;

    inline bool Process(HeatmapFrame& frame) const {
        auto& stylus = frame.stylus;
        auto& flow = stylus.runtime.flow;
        auto& parse = stylus.runtime.parse;

        flow.pipelineStage = 3;
        if (!m_enabled || !parse.valid) {
            flow.terminal = true;
            return true;
        }

        AnalyzeBlock(parse.gridData.tx1, stylus.runtime.tx1);
        if (parse.gridData.tx2.valid) {
            AnalyzeBlock(parse.gridData.tx2, stylus.runtime.tx2);
        } else {
            stylus.runtime.tx2 = {};
        }

        if (!stylus.runtime.tx1.peak.valid) {
            flow.terminal = true;
            flow.frameClass = Asa::StylusFrameClass::Tx1Missing;
        }
        return true;
    }

private:
    struct FourNeighborList {
        std::array<uint8_t, 4> indices{};
        uint8_t count = 0;
    };

    struct NineNeighborList {
        std::array<uint8_t, 9> indices{};
        uint8_t count = 0;
    };

    static inline FourNeighborList GetFourNeighbors(int idx) {
        FourNeighborList list{};
        const int r = idx / Asa::kGridDim;
        const int c = idx % Asa::kGridDim;
        if (r > 0) list.indices[list.count++] = static_cast<uint8_t>(idx - Asa::kGridDim);
        if (r + 1 < Asa::kGridDim) list.indices[list.count++] = static_cast<uint8_t>(idx + Asa::kGridDim);
        if (c > 0) list.indices[list.count++] = static_cast<uint8_t>(idx - 1);
        if (c + 1 < Asa::kGridDim) list.indices[list.count++] = static_cast<uint8_t>(idx + 1);
        return list;
    }

    static inline NineNeighborList GetNineNeighbors(int idx) {
        NineNeighborList list{};
        const int r = idx / Asa::kGridDim;
        const int c = idx % Asa::kGridDim;
        for (int dr = -1; dr <= 1; ++dr) {
            const int nr = r + dr;
            if (nr < 0 || nr >= Asa::kGridDim) continue;
            for (int dc = -1; dc <= 1; ++dc) {
                const int nc = c + dc;
                if (nc < 0 || nc >= Asa::kGridDim) continue;
                list.indices[list.count++] = static_cast<uint8_t>(nr * Asa::kGridDim + nc);
            }
        }
        return list;
    }

    inline void AnalyzeBlock(const Asa::FreqBlock& block, StylusRuntimeProjection& out) const {
        out = {};
        if (!block.valid) return;

        const int16_t* flat = &block.grid[0][0];
        for (int r = 0; r < Asa::kGridDim; ++r)
            for (int c = 0; c < Asa::kGridDim; ++c)
                out.grid[r][c] = block.grid[r][c];

        std::array<uint8_t, Asa::kGridSize> visited{};
        for (int idx = 0; idx < Asa::kGridSize; ++idx) {
            if (visited[static_cast<std::size_t>(idx)] != 0) continue;
            if (!IsPeak(flat, idx)) continue;

            const int connected = FloodFill(flat, visited, idx);
            if (connected >= m_maxConnected) continue;

            const int32_t neighborSum = Calc3x3Sum(flat, idx);
            if (!out.peak.valid || neighborSum > out.peak.neighborSum3x3) {
                out.peak.valid = true;
                out.peak.peakRow = idx / Asa::kGridDim;
                out.peak.peakCol = idx % Asa::kGridDim;
                out.peak.peakValue = flat[idx];
                out.peak.neighborSum3x3 = neighborSum;
                out.peak.connectedPixels = connected;
            }
        }

        if (!out.peak.valid) return;
        out.peakSignal = static_cast<uint16_t>(std::clamp<int>(out.peak.peakValue, 0, 0xFFFF));
        ProjectTo1D(flat, out);
    }

    inline bool IsPeak(const int16_t* flat, int idx) const {
        const int16_t value = flat[idx];
        if (value <= m_noiseThreshold) return false;
        const FourNeighborList neighbors = GetFourNeighbors(idx);
        for (uint8_t i = 0; i < neighbors.count; ++i)
            if (flat[neighbors.indices[i]] > value) return false;
        return true;
    }

    inline int FloodFill(const int16_t* flat, std::array<uint8_t, Asa::kGridSize>& visited, int startIdx) const {
        std::array<uint8_t, Asa::kGridSize> stack{};
        int stackSize = 0;
        int regionCount = 0;
        stack[static_cast<std::size_t>(stackSize++)] = static_cast<uint8_t>(startIdx);
        visited[static_cast<std::size_t>(startIdx)] = 1;
        while (stackSize > 0) {
            const uint8_t cell = stack[static_cast<std::size_t>(--stackSize)];
            ++regionCount;
            const FourNeighborList neighbors = GetFourNeighbors(cell);
            for (uint8_t i = 0; i < neighbors.count; ++i) {
                const uint8_t next = neighbors.indices[i];
                if (visited[next] != 0) continue;
                if (flat[next] <= m_noiseThreshold) continue;
                visited[next] = 1;
                stack[static_cast<std::size_t>(stackSize++)] = next;
            }
        }
        return regionCount;
    }

    static inline int32_t Calc3x3Sum(const int16_t* flat, int idx) {
        int32_t sum = 0;
        const NineNeighborList neighbors = GetNineNeighbors(idx);
        for (uint8_t i = 0; i < neighbors.count; ++i)
            sum += flat[neighbors.indices[i]];
        return sum;
    }

    inline void ProjectTo1D(const int16_t* flat, StylusRuntimeProjection& out) const {
        const int rMin = std::max(0, out.peak.peakRow - m_projectionRadius);
        const int rMax = std::min(Asa::kGridDim - 1, out.peak.peakRow + m_projectionRadius);
        const int cMin = std::max(0, out.peak.peakCol - m_projectionRadius);
        const int cMax = std::min(Asa::kGridDim - 1, out.peak.peakCol + m_projectionRadius);
        out.projection.spanDim1 = rMax - rMin + 1;
        out.projection.spanDim2 = cMax - cMin + 1;

        for (int c = 0; c < Asa::kGridDim; ++c) {
            int32_t sum = 0;
            for (int r = rMin; r <= rMax; ++r) sum += flat[r * Asa::kGridDim + c];
            out.projection.dim1[c] = sum;
        }
        for (int r = 0; r < Asa::kGridDim; ++r) {
            int32_t sum = 0;
            const int rowBase = r * Asa::kGridDim;
            for (int c = cMin; c <= cMax; ++c) sum += flat[rowBase + c];
            out.projection.dim2[r] = sum;
        }
        out.projection.peakIdxDim1 = FindLinePeak(out.projection.dim1);
        out.projection.peakIdxDim2 = FindLinePeak(out.projection.dim2);
    }

    static inline int FindLinePeak(const int32_t (&signal)[Asa::kGridDim]) {
        int best = 0;
        for (int i = 1; i < Asa::kGridDim; ++i)
            if (signal[i] > signal[best]) best = i;
        return signal[best] > 0 ? best : -1;
    }
};

} // namespace Solvers::Stylus
