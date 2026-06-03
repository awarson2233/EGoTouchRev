#pragma once
#include "SolverTypes.h"
#include <algorithm>
#include <array>
#include <cstdint>
namespace Solvers::Stylus {
class GridFeatureExtractor {
public:
    bool m_enabled = true;
    int m_tx1PeakSeedThreshold = 199;
    int m_peakRegionFloor = 100;
    int m_maxConnected = 10;
    int m_boundarySlopeQ5 = 0x23;
    int m_linePeakFloor = 250;
    int m_lineRegionEnergyFloor = 500;
    inline bool Process(HeatmapFrame& frame) {
        auto& stylus = frame.stylus;
        auto& flow = stylus.runtime.flow;
        auto& parse = stylus.runtime.parse;
        auto& rawGrid = stylus.runtime.rawGrid;
        flow.pipelineStage = 3;
        if (!m_enabled || !parse.valid) {
            ResetLinePeakHistory();
            flow.terminal = true;
            return true;
        }
        PeakFlags tx1PeakFlags{};
        AnalyzeTx1Block(rawGrid.asaGrid.tx1, stylus.runtime.tx1.feature, tx1PeakFlags);
        stylus.runtime.tx1.coordinate = {};
        if (rawGrid.asaGrid.tx2.valid) {
            AnalyzeTx2BlockFromTx1(stylus.runtime.tx1.feature.grid,
                                   tx1PeakFlags,
                                   rawGrid.asaGrid.tx2,
                                   stylus.runtime.tx2.feature);
            stylus.runtime.tx2.coordinate = {};
        } else {
            stylus.runtime.tx2 = {};
        }
        if (!stylus.runtime.tx1.feature.peak.valid) {
            flow.terminal = true;
            flow.frameClass = Asa::StylusFrameClass::Tx1Missing;
        }
        return true;
    }

private:
    using Grid = std::array<int16_t, Asa::kGridSize>;
    using PeakFlags = std::array<uint16_t, Asa::kGridSize>;
    using Grid2D = int16_t[Asa::kGridDim][Asa::kGridDim];
    using Axis = int32_t[Asa::kGridDim];
    static constexpr int kAnchorCenterOffset = Asa::kGridDim / 2;
    static constexpr int kSensorCols = 60;
    static constexpr int kSensorRows = 40;
    static constexpr int kFactoryTx2SeedThreshold = 99; // Original GetGridTx2Peaks accepts reduced TX2 cells only when value > 99.
    static constexpr int kFactoryTx2RegionFloor = 100;
    struct FourNeighborList { std::array<uint8_t, 4> indices; uint8_t count; };
    struct PeakRegion {
        int peakRow = -1, peakCol = -1, minRow = 0, maxRow = 0, minCol = 0, maxCol = 0, cellCount = 0;
        int32_t peakValue = 0, regionSum = 0, sum3x3 = 0, refinedDim1 = 0, refinedDim2 = 0;
        bool valid = false;
        std::array<uint8_t, Asa::kGridSize> cells{};
    };

    struct ProjectionBounds {
        int minRow = 0, maxRow = 0, minCol = 0, maxCol = 0;
    };

    struct AxisEdgeGeometry {
        int lowIdx = -1;
        int highIdx = -1;
        int validMin = 0;
        int validMax = Asa::kGridDim - 1;
    };

    struct LinePeakCandidate { int peakIdx = -1, globalPeakIdx = -1, leftBoundary = 0, rightBoundary = 0, age = 0; int32_t netSignal = 0, regionEnergy = 0; };
    struct LinePeakTable {
        std::array<LinePeakCandidate, 4> peaks{};
        int count = 0, strongestByNet = -1, largestByEnergy = -1, weakestByNet = -1, selectedByRank = -1;
    };

    LinePeakTable m_prevLinePeaksDim1{};
    LinePeakTable m_prevLinePeaksDim2{};

    static constexpr std::array<FourNeighborList, Asa::kGridSize> kFourNeighbors = [] {
        std::array<FourNeighborList, Asa::kGridSize> table{};
        for (int idx = 0; idx < Asa::kGridSize; ++idx) {
            auto& list = table[static_cast<std::size_t>(idx)];
            const int r = idx / Asa::kGridDim, c = idx % Asa::kGridDim;
            if (r > 0) list.indices[list.count++] = static_cast<uint8_t>(idx - Asa::kGridDim);
            if (r + 1 < Asa::kGridDim) list.indices[list.count++] = static_cast<uint8_t>(idx + Asa::kGridDim);
            if (c > 0) list.indices[list.count++] = static_cast<uint8_t>(idx - 1);
            if (c + 1 < Asa::kGridDim) list.indices[list.count++] = static_cast<uint8_t>(idx + 1);
        }
        return table;
    }();

    static inline int32_t NonNegative(int32_t value) { return std::max<int32_t>(value, 0); }

    static inline bool IsValidLocalIndex(int idx) { return idx >= 0 && idx < Asa::kGridDim; }

    static inline AxisEdgeGeometry GetAxisEdgeGeometry(int anchor, int sensorCount) {
        AxisEdgeGeometry geometry{};
        geometry.lowIdx = kAnchorCenterOffset - anchor;
        geometry.highIdx = kAnchorCenterOffset + sensorCount - 1 - anchor;
        geometry.validMin = std::clamp(geometry.lowIdx, 0, Asa::kGridDim - 1);
        geometry.validMax = std::clamp(geometry.highIdx, 0, Asa::kGridDim - 1);
        if (geometry.validMax < geometry.validMin) {
            geometry.validMin = 0;
            geometry.validMax = Asa::kGridDim - 1;
        }
        return geometry;
    }

    static inline bool IsPhysicalLowEdge(int idx, const AxisEdgeGeometry& geometry) {
        return IsValidLocalIndex(geometry.lowIdx) && idx == geometry.lowIdx;
    }

    static inline bool IsPhysicalHighEdge(int idx, const AxisEdgeGeometry& geometry) {
        return IsValidLocalIndex(geometry.highIdx) && idx == geometry.highIdx;
    }

    static inline bool IsPhysicalEdge(int idx, const AxisEdgeGeometry& geometry) {
        return IsPhysicalLowEdge(idx, geometry) || IsPhysicalHighEdge(idx, geometry);
    }

    static inline bool RangeTouchesPhysicalEdge(int minIdx, int maxIdx, const AxisEdgeGeometry& geometry) {
        return (IsValidLocalIndex(geometry.lowIdx) && minIdx <= geometry.lowIdx && geometry.lowIdx <= maxIdx) ||
               (IsValidLocalIndex(geometry.highIdx) && minIdx <= geometry.highIdx && geometry.highIdx <= maxIdx);
    }

    static inline std::size_t GridIndex(int row, int col) { return static_cast<std::size_t>(row * Asa::kGridDim + col); }
    static inline int32_t GridAt(const Grid& grid, int row, int col) { return grid[GridIndex(row, col)]; }
    template <typename Func>
    static inline void ForEachGridCell(Func func) {
        for (int r = 0; r < Asa::kGridDim; ++r)
            for (int c = 0; c < Asa::kGridDim; ++c)
                func(r, c, GridIndex(r, c));
    }

    template <typename Func>
    static inline void ForEachCellInRange(int rowMin, int rowMax, int colMin, int colMax, Func func) {
        for (int r = rowMin; r <= rowMax; ++r)
            for (int c = colMin; c <= colMax; ++c)
                func(r, c);
    }

    static inline void CopyGrid(const Asa::FreqBlock& block, Grid& linear, Grid2D& grid) {
        ForEachGridCell([&](int r, int c, std::size_t idx) {
            const int16_t value = block.grid[r][c];
            linear[idx] = value;
            grid[r][c] = value;
        });
    }

    static inline void CopyGrid(const Grid2D& grid, Grid& linear) {
        ForEachGridCell([&](int r, int c, std::size_t idx) { linear[idx] = grid[r][c]; });
    }

    static inline void CopyGrid(const Grid& linear, Grid2D& grid) {
        ForEachGridCell([&](int r, int c, std::size_t idx) { grid[r][c] = linear[idx]; });
    }

    template <typename ShouldSkip, typename OnRegion>
    inline void ScanPeakRegions(Grid& searchGrid, PeakFlags& peakFlags, int seedThreshold, int regionFloor,
                                ShouldSkip shouldSkip, OnRegion onRegion) const {
        uint16_t peakIndex = 1;
        for (int idx = 0; idx < Asa::kGridSize; ++idx) {
            if (shouldSkip(idx) || !IsGridPeak(searchGrid, peakFlags, idx, seedThreshold)) continue;
            PeakRegion region;
            InitPeakRegion(region, searchGrid, idx);
            peakFlags[static_cast<std::size_t>(idx)] = peakIndex;
            GrowPeakRegion(searchGrid, peakFlags, peakIndex, regionFloor, region);
            onRegion(region, peakIndex);
            if (peakIndex < 0x7fff) ++peakIndex;
        }
    }

    inline void AnalyzeTx1Block(const Asa::FreqBlock& block, StylusGridFeature& out, PeakFlags& peakFlagsOut) {
        out = {};
        peakFlagsOut.fill(0);
        if (!block.valid) {
            ResetLinePeakHistory();
            return;
        }
        Grid searchGrid{};
        CopyGrid(block, searchGrid, out.grid);
        const AxisEdgeGeometry dim1Edge = GetAxisEdgeGeometry(block.anchorCol, kSensorCols);
        const AxisEdgeGeometry dim2Edge = GetAxisEdgeGeometry(block.anchorRow, kSensorRows);
        PeakRegion best;
        best.valid = false;
        ScanPeakRegions(searchGrid, peakFlagsOut, m_tx1PeakSeedThreshold, m_peakRegionFloor, [](int) { return false; },
                        [&](PeakRegion& region, uint16_t) {
                            if (region.cellCount >= m_maxConnected) {
                                CleanLargeRegion(searchGrid, peakFlagsOut, region);
                                return;
                            }
                            region.sum3x3 = Calc3x3Sum(searchGrid, region.peakRow, region.peakCol, dim2Edge, dim1Edge);
                            if (!best.valid || region.peakValue >= best.peakValue) CopyPeakRegionSummary(best, region);
                        });
        CopyGrid(searchGrid, out.grid);
        if (!best.valid) {
            ResetLinePeakHistory();
            return;
        }
        ExportPrimaryPeak(best, out);
        ProjectTx1To1D(searchGrid, best, out, dim2Edge, dim1Edge,
                       static_cast<int>(block.anchorCol) - kAnchorCenterOffset,
                       static_cast<int>(block.anchorRow) - kAnchorCenterOffset);
    }

    inline void AnalyzeTx2BlockFromTx1(const Grid2D& tx1SearchGrid, const PeakFlags& tx1PeakFlags,
                                       const Asa::FreqBlock& block, StylusGridFeature& out) const {
        out = {};
        if (!block.valid) return;
        Grid tx1Linear{}, tx2Search{};
        CopyGrid(tx1SearchGrid, tx1Linear);
        CopyGrid(block, tx2Search, out.grid);
        const AxisEdgeGeometry dim1Edge = GetAxisEdgeGeometry(block.anchorCol, kSensorCols);
        const AxisEdgeGeometry dim2Edge = GetAxisEdgeGeometry(block.anchorRow, kSensorRows);
        ReduceTx2DataByTx1Peaks(tx1Linear, tx2Search);
        ScanTx2PeakRegions(tx2Search, tx1PeakFlags, out.peakTable, dim2Edge, dim1Edge);
        CopyGrid(tx2Search, out.grid);
        ExportStrongestTx2Peak(out);
    }

    inline void ScanTx2PeakRegions(Grid& tx2Search,
                                   const PeakFlags& tx1PeakFlags,
                                   Asa::GridPeakTable& peakTable,
                                   const AxisEdgeGeometry& dim2Edge,
                                   const AxisEdgeGeometry& dim1Edge) const {
        PeakFlags peakFlags{};
        uint16_t peakIndex = 1;
        for (int idx = 0; idx < Asa::kGridSize; ++idx) {
            const std::size_t uidx = static_cast<std::size_t>(idx);
            if (peakFlags[uidx] != 0) continue;

            const int32_t value = tx2Search[uidx];
            if (value <= kFactoryTx2SeedThreshold) continue;
            if (tx1PeakFlags[uidx] >= 0x81) continue;
            if (!IsGridPeakNeighborMax(tx2Search, idx, value)) continue;

            PeakRegion region;
            InitPeakRegion(region, tx2Search, idx);
            peakFlags[uidx] = peakIndex;
            GrowPeakRegion(tx2Search, peakFlags, peakIndex, kFactoryTx2RegionFloor, region);
            UpdatePeakData(region, tx2Search);
            UpdateRefinedPos(region, tx2Search, dim2Edge, dim1Edge);
            region.sum3x3 = Calc3x3Sum(tx2Search, region.peakRow, region.peakCol, dim2Edge, dim1Edge);
            UpdateTx2PeakTable(region, peakIndex, peakTable);

            if (peakIndex < 0x7fff) ++peakIndex;
        }
    }

    static inline void ReduceTx2DataByTx1Peaks(const Grid& tx1Search, Grid& tx2Search) {
        for (std::size_t i = 0; i < tx2Search.size(); ++i) {
            const int32_t tx1Contribution = NonNegative(tx1Search[i]) / 5;
            const int32_t tx2Value = NonNegative(tx2Search[i]);
            tx2Search[i] = static_cast<int16_t>(tx2Value > tx1Contribution ? tx2Value - tx1Contribution : 0);
        }
    }

    inline bool IsGridPeak(const Grid& grid, const PeakFlags& peakFlags, int idx, int seedThreshold) const {
        const std::size_t uidx = static_cast<std::size_t>(idx);
        const int32_t value = grid[uidx];
        if (peakFlags[uidx] != 0 || value <= seedThreshold) return false;
        return IsGridPeakNeighborMax(grid, idx, value);
    }

    inline bool IsGridPeakNeighborMax(const Grid& grid, int idx, int32_t value) const {
        const int row = idx / Asa::kGridDim, col = idx % Asa::kGridDim;
        const auto at = [&](int offset) { return grid[static_cast<std::size_t>(idx + offset)]; };
        if (col > 0 && (at(-1) >= value || (row > 0 && at(-Asa::kGridDim - 1) >= value) ||
                        (row + 1 < Asa::kGridDim && at(Asa::kGridDim - 1) >= value))) return false;
        if (col + 1 < Asa::kGridDim && (at(1) > value || (row > 0 && at(-Asa::kGridDim + 1) > value) ||
                                        (row + 1 < Asa::kGridDim && at(Asa::kGridDim + 1) > value))) return false;
        return !(row > 0 && at(-Asa::kGridDim) >= value) && !(row + 1 < Asa::kGridDim && at(Asa::kGridDim) > value);
    }

    static inline void InitPeakRegion(PeakRegion& region, const Grid& grid, int idx) {
        region.valid = true;
        region.peakRow = region.minRow = region.maxRow = idx / Asa::kGridDim;
        region.peakCol = region.minCol = region.maxCol = idx % Asa::kGridDim;
        region.cellCount = 1;
        region.peakValue = grid[static_cast<std::size_t>(idx)];
        region.regionSum = region.peakValue;
        region.sum3x3 = 0;
        region.refinedDim1 = 0;
        region.refinedDim2 = 0;
        region.cells[0] = static_cast<uint8_t>(idx);
    }

    static inline void CopyPeakRegionSummary(PeakRegion& dst, const PeakRegion& src) {
        dst.peakRow = src.peakRow;
        dst.peakCol = src.peakCol;
        dst.minRow = src.minRow;
        dst.maxRow = src.maxRow;
        dst.minCol = src.minCol;
        dst.maxCol = src.maxCol;
        dst.cellCount = src.cellCount;
        dst.peakValue = src.peakValue;
        dst.regionSum = src.regionSum;
        dst.sum3x3 = src.sum3x3;
        dst.refinedDim1 = src.refinedDim1;
        dst.refinedDim2 = src.refinedDim2;
        dst.valid = src.valid;
    }

    inline bool InPeak(const Grid& grid, const PeakFlags& peakFlags, int idx, int32_t centerValue,
                       int regionFloor, const PeakRegion& region) const {
        const int32_t value = grid[static_cast<std::size_t>(idx)];
        return peakFlags[static_cast<std::size_t>(idx)] == 0 && value > regionFloor &&
               value > (region.peakValue >> 1) && value > (centerValue / 3) && value < (centerValue * 2);
    }

    inline void GrowPeakRegion(const Grid& grid, PeakFlags& peakFlags, uint16_t peakIndex,
                               int regionFloor, PeakRegion& region) const {
        std::array<uint8_t, Asa::kGridSize> stack{};
        int stackSize = 0;
        stack[static_cast<std::size_t>(stackSize++)] = region.cells[0];
        while (stackSize > 0) {
            const uint8_t cell = stack[static_cast<std::size_t>(--stackSize)];
            const int32_t centerValue = grid[cell];
            const FourNeighborList& neighbors = kFourNeighbors[cell];
            for (uint8_t i = 0; i < neighbors.count; ++i) {
                const uint8_t next = neighbors.indices[i];
                if (!InPeak(grid, peakFlags, next, centerValue, regionFloor, region)) continue;
                peakFlags[next] = peakIndex;
                stack[static_cast<std::size_t>(stackSize++)] = next;
                region.cells[static_cast<std::size_t>(region.cellCount++)] = next;
                region.regionSum += grid[next];
                const int row = next / Asa::kGridDim, col = next % Asa::kGridDim;
                region.minRow = std::min(region.minRow, row);
                region.maxRow = std::max(region.maxRow, row);
                region.minCol = std::min(region.minCol, col);
                region.maxCol = std::max(region.maxCol, col);
            }
        }
    }

    static inline void CleanLargeRegion(Grid& grid, PeakFlags& peakFlags, const PeakRegion& region) {
        for (int i = 0; i < region.cellCount; ++i) {
            const uint8_t idx = region.cells[static_cast<std::size_t>(i)];
            peakFlags[idx] = 0x81;
            grid[idx] = 0;
        }
    }

    static inline void GetThreeCellAxisRange(int peakIdx, const AxisEdgeGeometry& geometry, int& minIdx, int& maxIdx) {
        if (IsPhysicalLowEdge(peakIdx, geometry)) {
            minIdx = peakIdx;
            maxIdx = peakIdx + 2;
        } else if (IsPhysicalHighEdge(peakIdx, geometry)) {
            minIdx = peakIdx - 2;
            maxIdx = peakIdx;
        } else {
            minIdx = peakIdx - 1;
            maxIdx = peakIdx + 1;
        }
        minIdx = std::clamp(minIdx, geometry.validMin, geometry.validMax);
        maxIdx = std::clamp(maxIdx, geometry.validMin, geometry.validMax);
    }

    static inline int32_t Calc3x3Sum(const Grid& grid,
                                     int peakRow,
                                     int peakCol,
                                     const AxisEdgeGeometry& rowEdge,
                                     const AxisEdgeGeometry& colEdge) {
        int rowMin = 0, rowMax = 0, colMin = 0, colMax = 0;
        GetThreeCellAxisRange(peakRow, rowEdge, rowMin, rowMax);
        GetThreeCellAxisRange(peakCol, colEdge, colMin, colMax);
        int32_t sum = 0;
        ForEachCellInRange(rowMin, rowMax, colMin, colMax, [&](int r, int c) { sum += GridAt(grid, r, c); });
        return sum;
    }

    static inline void UpdatePeakData(PeakRegion& region, const Grid& grid) {
        ForEachCellInRange(region.minRow, region.maxRow, region.minCol, region.maxCol, [&](int row, int col) {
            const int32_t value = GridAt(grid, row, col);
            if (value <= region.peakValue) return;
            region.peakValue = value;
            region.peakRow = row;
            region.peakCol = col;
        });
    }

    static inline void GetRefineAxisRange(int peakIdx, const AxisEdgeGeometry& geometry, int& minIdx, int& maxIdxOut) {
        if (IsPhysicalLowEdge(peakIdx, geometry)) {
            minIdx = peakIdx;
            maxIdxOut = peakIdx + 1;
        } else if (IsPhysicalHighEdge(peakIdx, geometry)) {
            minIdx = peakIdx - 1;
            maxIdxOut = peakIdx;
        } else if ((IsValidLocalIndex(geometry.lowIdx) && peakIdx == geometry.lowIdx + 1) ||
                   (IsValidLocalIndex(geometry.highIdx) && peakIdx == geometry.highIdx - 1)) {
            minIdx = peakIdx - 1;
            maxIdxOut = peakIdx + 1;
        } else {
            minIdx = peakIdx - 2;
            maxIdxOut = peakIdx + 2;
        }
        minIdx = std::clamp(minIdx, geometry.validMin, geometry.validMax);
        maxIdxOut = std::clamp(maxIdxOut, geometry.validMin, geometry.validMax);
    }

    static inline void UpdateRefinedPos(PeakRegion& region,
                                        const Grid& grid,
                                        const AxisEdgeGeometry& rowEdge,
                                        const AxisEdgeGeometry& colEdge) {
        int colMin = 0, colMax = 0, rowMin = 0, rowMax = 0;
        GetRefineAxisRange(region.peakCol, colEdge, colMin, colMax);
        GetRefineAxisRange(region.peakRow, rowEdge, rowMin, rowMax);
        int32_t total = 0;
        int sampleCount = 0;
        ForEachCellInRange(rowMin, rowMax, colMin, colMax, [&](int row, int col) {
            total += GridAt(grid, row, col);
            ++sampleCount;
        });
        if (sampleCount == 0) return;
        const int32_t baseline = total / sampleCount;
        int32_t totalWeight = 0;
        int64_t weightedDim1 = 0, weightedDim2 = 0;
        ForEachCellInRange(rowMin, rowMax, colMin, colMax, [&](int row, int col) {
            const int32_t value = GridAt(grid, row, col);
            if (value <= baseline) return;
            const int32_t weight = value - baseline;
            totalWeight += weight;
            weightedDim1 += static_cast<int64_t>(col) * weight;
            weightedDim2 += static_cast<int64_t>(row) * weight;
        });
        if (totalWeight == 0) {
            region.refinedDim1 = ((colMin + colMax) * Asa::kCoorUnit) / 2 + Asa::kCoorUnit / 2;
            region.refinedDim2 = ((rowMin + rowMax) * Asa::kCoorUnit) / 2 + Asa::kCoorUnit / 2;
            return;
        }
        region.refinedDim1 = static_cast<int32_t>((weightedDim1 * Asa::kCoorUnit) / totalWeight) + Asa::kCoorUnit / 2;
        region.refinedDim2 = static_cast<int32_t>((weightedDim2 * Asa::kCoorUnit) / totalWeight) + Asa::kCoorUnit / 2;
    }

    template <typename Items, typename Better>
    static inline int SelectSlot(const Items& items, int count, Better better, int exclude = -1) {
        int slot = -1;
        for (int i = 0; i < count; ++i) {
            if (i == exclude) continue;
            if (slot < 0 || better(items[static_cast<std::size_t>(i)], items[static_cast<std::size_t>(slot)])) slot = i;
        }
        return slot;
    }

    template <typename Item, std::size_t N, typename Score>
    static inline void InsertTopN(const Item& item, std::array<Item, N>& items, int& count, int weakest, Score score) {
        if (count == static_cast<int>(items.size())) {
            if (weakest >= 0 && score(items[static_cast<std::size_t>(weakest)]) < score(item)) items[static_cast<std::size_t>(weakest)] = item;
        } else {
            items[static_cast<std::size_t>(count++)] = item;
        }
    }

    static inline Asa::GridPeakRegion ToGridPeakRegion(const PeakRegion& region, int regionId) {
        return {region.peakRow, region.peakCol, region.peakValue, region.regionSum, region.sum3x3,
                region.minRow, region.maxRow, region.minCol, region.maxCol, region.refinedDim1,
                region.refinedDim2, region.cellCount, regionId, region.valid};
    }

    static inline void RecomputePeakTableSlots(Asa::GridPeakTable& table) {
        table.strongestSlot = SelectSlot(table.regions, table.count, [](const auto& a, const auto& b) { return a.peakValue >= b.peakValue; });
        table.weakestSlot = SelectSlot(table.regions, table.count, [](const auto& a, const auto& b) { return a.peakValue <= b.peakValue; });
        if (table.strongestSlot < 0) {
            table.strongestRegionId = -1;
            table.selectedPeak3x3Sum = 0;
            return;
        }
        const auto& strongest = table.regions[static_cast<std::size_t>(table.strongestSlot)];
        table.strongestRegionId = strongest.regionId;
        table.selectedPeak3x3Sum = strongest.sum3x3;
    }

    static inline void UpdateTx2PeakTable(const PeakRegion& region, int regionId, Asa::GridPeakTable& table) {
        InsertTopN(ToGridPeakRegion(region, regionId), table.regions, table.count, table.weakestSlot,
                   [](const Asa::GridPeakRegion& item) { return item.peakValue; });
        RecomputePeakTableSlots(table);
    }

    static inline void ExportPeakUnit(const Asa::GridPeakRegion& peak, StylusGridFeature& out, int32_t signal) {
        out.peak = {peak.peakRow, peak.peakCol, peak.peakValue, peak.sum3x3, peak.connectedPixels, peak.valid};
        out.peakSignal = static_cast<uint16_t>(std::clamp(signal, 0, 0xFFFF));
    }

    static inline void ExportPrimaryPeak(const PeakRegion& region, StylusGridFeature& out) {
        ExportPeakUnit(ToGridPeakRegion(region, 0), out, region.sum3x3);
    }

    static inline void ExportStrongestTx2Peak(StylusGridFeature& out) {
        if (out.peakTable.count == 0 || out.peakTable.strongestSlot < 0) return;
        const auto& peak = out.peakTable.regions[static_cast<std::size_t>(out.peakTable.strongestSlot)];
        out.refinedLocalCoor = {peak.refinedDim1, peak.refinedDim2, peak.valid};
        ExportPeakUnit(peak, out, out.peakTable.selectedPeak3x3Sum);
    }

    template <typename CellValue>
    static inline void FillProjectionAxis(Axis& out, int minIdx, int maxIdx, CellValue cellValue) {
        for (int i = 0; i < Asa::kGridDim; ++i) {
            int32_t sum = 0;
            for (int j = minIdx; j <= maxIdx; ++j) sum += std::max<int32_t>(cellValue(i, j), 0);
            out[i] = sum;
        }
    }

    inline void ProjectTx1To1D(const Grid& grid,
                               const PeakRegion& region,
                               StylusGridFeature& out,
                               const AxisEdgeGeometry& rowEdge,
                               const AxisEdgeGeometry& colEdge,
                               int dim1GlobalOffset,
                               int dim2GlobalOffset) {
        const ProjectionBounds projectionBounds = BuildProjectionBounds(region, rowEdge, colEdge);
        out.projection.spanDim1 = projectionBounds.maxRow - projectionBounds.minRow + 1;
        out.projection.spanDim2 = projectionBounds.maxCol - projectionBounds.minCol + 1;
        FillProjectionAxis(out.projection.dim1, projectionBounds.minRow, projectionBounds.maxRow,
                           [&](int c, int r) { return GridAt(grid, r, c); });
        FillProjectionAxis(out.projection.dim2, projectionBounds.minCol, projectionBounds.maxCol,
                           [&](int r, int c) { return GridAt(grid, r, c); });
        ProcessTx1LinePeaks(out, colEdge, rowEdge, dim1GlobalOffset, dim2GlobalOffset);
    }

    static inline ProjectionBounds BuildProjectionBounds(const PeakRegion& region,
                                                         const AxisEdgeGeometry& rowEdge,
                                                         const AxisEdgeGeometry& colEdge) {
        ProjectionBounds bounds{region.minRow, region.maxRow, region.minCol, region.maxCol};
        bounds.minRow = std::clamp(bounds.minRow, rowEdge.validMin, rowEdge.validMax);
        bounds.maxRow = std::clamp(bounds.maxRow, rowEdge.validMin, rowEdge.validMax);
        bounds.minCol = std::clamp(bounds.minCol, colEdge.validMin, colEdge.validMax);
        bounds.maxCol = std::clamp(bounds.maxCol, colEdge.validMin, colEdge.validMax);
        if (RangeTouchesPhysicalEdge(bounds.minCol, bounds.maxCol, colEdge)) {
            bounds.minRow = std::clamp(region.peakRow - 1, rowEdge.validMin, rowEdge.validMax);
            bounds.maxRow = std::clamp(region.peakRow + 1, rowEdge.validMin, rowEdge.validMax);
        }
        if (RangeTouchesPhysicalEdge(bounds.minRow, bounds.maxRow, rowEdge)) {
            bounds.minCol = std::clamp(region.peakCol - 1, colEdge.validMin, colEdge.validMax);
            bounds.maxCol = std::clamp(region.peakCol + 1, colEdge.validMin, colEdge.validMax);
        }
        return bounds;
    }

    static inline int AbsDiff(int lhs, int rhs) { return lhs > rhs ? lhs - rhs : rhs - lhs; }
    void ResetLinePeakHistory() { m_prevLinePeaksDim1 = {}; m_prevLinePeaksDim2 = {}; }
    static inline int GetSelectedLinePeakIdx(const LinePeakTable& table) {
        const int slot = table.selectedByRank >= 0 ? table.selectedByRank : table.strongestByNet;
        if (slot < 0 || slot >= table.count) return -1;
        return table.peaks[static_cast<std::size_t>(slot)].peakIdx;
    }

    static inline int GetSelectedLinePeakGlobalIdx(const LinePeakTable& table) {
        const int slot = table.selectedByRank >= 0 ? table.selectedByRank : table.strongestByNet;
        if (slot < 0 || slot >= table.count) return -1;
        return table.peaks[static_cast<std::size_t>(slot)].globalPeakIdx;
    }

    static inline void ApplyLinePeakHistory(LinePeakTable& current, const LinePeakTable& previous) {
        current.selectedByRank = -1;
        if (current.count == 0) return;
        if (previous.count == 0) { current.selectedByRank = current.strongestByNet; return; }
        const int previousSelectedIdx = GetSelectedLinePeakGlobalIdx(previous);
        const int strongestNet = current.strongestByNet >= 0
                                      ? std::max(current.peaks[static_cast<std::size_t>(current.strongestByNet)].netSignal, 1)
                                      : 1;
        int bestScore = -0x7fffffff;
        for (int i = 0; i < current.count; ++i) {
            auto& candidate = current.peaks[static_cast<std::size_t>(i)];
            candidate.age = 0;
            const int distanceFromPreviousSelection = previousSelectedIdx >= 0
                                                          ? AbsDiff(candidate.globalPeakIdx, previousSelectedIdx) * Asa::kCoorUnit
                                                          : 0;
            int matchedAge = 0;
            for (int j = 0; j < previous.count; ++j) {
                const auto& prev = previous.peaks[static_cast<std::size_t>(j)];
                if (AbsDiff(candidate.globalPeakIdx, prev.globalPeakIdx) > 2) continue;
                matchedAge = std::max(matchedAge, prev.age + 1);
            }
            candidate.age = std::min(matchedAge, 0xFFF5);
            int score = (candidate.netSignal * 10) / strongestNet;
            if (candidate.age > 20) ++score;
            if (previousSelectedIdx >= 0)
                score += distanceFromPreviousSelection < 0x200 ? 20 : 20 / std::max(1, distanceFromPreviousSelection >> 9);
            if (score < bestScore) continue;
            bestScore = score;
            current.selectedByRank = i;
        }
        if (current.selectedByRank < 0) current.selectedByRank = current.strongestByNet;
    }
    
    inline LinePeakCandidate ProcessTx1LinePeakAxis(const Axis& signal, LinePeakTable& previous, int globalOffset) {
        LinePeakTable current = SearchLinePeaks(signal, globalOffset);
        ApplyLinePeakHistory(current, previous);
        LinePeakCandidate selected{};
        const int selectedSlot = current.selectedByRank >= 0 ? current.selectedByRank : current.strongestByNet;
        if (selectedSlot >= 0 && selectedSlot < current.count) {
            selected = current.peaks[static_cast<std::size_t>(selectedSlot)];
        }
        previous = current;
        return selected;
    }

    inline void ProcessTx1LinePeaks(StylusGridFeature& out,
                                    const AxisEdgeGeometry& dim1Edge,
                                    const AxisEdgeGeometry& dim2Edge,
                                    int dim1GlobalOffset,
                                    int dim2GlobalOffset) {
        LinePeakCandidate dim1Peak = ProcessTx1LinePeakAxis(out.projection.dim1, m_prevLinePeaksDim1, dim1GlobalOffset);
        LinePeakCandidate dim2Peak = ProcessTx1LinePeakAxis(out.projection.dim2, m_prevLinePeaksDim2, dim2GlobalOffset);
        out.projection.peakIdxDim1 = dim1Peak.peakIdx;
        out.projection.peakIdxDim2 = dim2Peak.peakIdx;
        out.dim1SelectedPeakNetSignal = static_cast<uint16_t>(std::clamp(dim1Peak.netSignal, 0, 0xFFFF));
        out.dim2SelectedPeakNetSignal = static_cast<uint16_t>(std::clamp(dim2Peak.netSignal, 0, 0xFFFF));
        out.dim1SelectedPeakOnEdge = IsPhysicalEdge(dim1Peak.peakIdx, dim1Edge);
        out.dim2SelectedPeakOnEdge = IsPhysicalEdge(dim2Peak.peakIdx, dim2Edge);
    }

    inline LinePeakTable SearchLinePeaks(const Axis& signal, int globalOffset) const {
        LinePeakTable table{};
        for (int i = 0; i < Asa::kGridDim; ++i) {
            const int32_t value = NonNegative(signal[i]);
            if (value <= m_linePeakFloor) continue;
            if (i > 0 && NonNegative(signal[i - 1]) > value) continue;
            if (i + 1 < Asa::kGridDim && NonNegative(signal[i + 1]) >= value) continue;
            if (i > 1 && NonNegative(signal[i - 2]) > value) continue;
            if (i + 2 < Asa::kGridDim && NonNegative(signal[i + 2]) >= value) continue;
            LinePeakCandidate candidate = BuildLinePeakCandidate(signal, i, globalOffset);
            if (candidate.regionEnergy < m_lineRegionEnergyFloor) continue;
            UpdateLinePeakUnit(candidate, table);
        }
        return table;
    }

    inline LinePeakCandidate BuildLinePeakCandidate(const Axis& signal, int peakIdx, int globalOffset) const {
        LinePeakCandidate candidate{};
        candidate.peakIdx = peakIdx;
        candidate.globalPeakIdx = peakIdx + globalOffset;
        SearchPeakBoundary(signal, peakIdx, candidate.leftBoundary, candidate.rightBoundary);
        int32_t baselineMin = NonNegative(signal[candidate.leftBoundary]);
        int32_t regionSum = 0;
        for (int i = candidate.leftBoundary; i <= candidate.rightBoundary; ++i) {
            const int32_t value = NonNegative(signal[i]);
            baselineMin = std::min(baselineMin, value);
            regionSum += value;
        }
        candidate.netSignal = NonNegative(signal[peakIdx]) - baselineMin;
        if (candidate.netSignal == 0) candidate.netSignal = 1;
        candidate.regionEnergy = regionSum - (candidate.rightBoundary - candidate.leftBoundary + 1) * baselineMin;
        return candidate;
    }

    static inline void UpdateLinePeakUnit(const LinePeakCandidate& candidate, LinePeakTable& table) {
        InsertTopN(candidate, table.peaks, table.count, table.weakestByNet,
                   [](const LinePeakCandidate& item) { return item.netSignal; });
        RecomputeLinePeakTableSlots(table);
    }

    static inline void RecomputeLinePeakTableSlots(LinePeakTable& table) {
        table.largestByEnergy = SelectSlot(table.peaks, table.count, [](const auto& a, const auto& b) { return a.regionEnergy >= b.regionEnergy; });
        table.strongestByNet = SelectSlot(table.peaks, table.count, [](const auto& a, const auto& b) { return a.netSignal >= b.netSignal; });
        table.weakestByNet = SelectSlot(table.peaks, table.count, [](const auto& a, const auto& b) { return a.netSignal <= b.netSignal; }, table.largestByEnergy);
        if (table.weakestByNet < 0 && table.count > 0) table.weakestByNet = table.strongestByNet;
    }

    inline void SearchPeakBoundary(const Axis& signal, int peakIdx, int& leftBoundary, int& rightBoundary) const {
        leftBoundary = peakIdx;
        rightBoundary = peakIdx;
        ExtendPeakBoundary(signal, peakIdx, leftBoundary, -1);
        ExtendPeakBoundary(signal, peakIdx, rightBoundary, 1);
    }
    
    inline void ExtendPeakBoundary(const Axis& signal, int peakIdx, int& boundary, int step) const {
        const int adjacent = boundary + step;
        if (adjacent < 0 || adjacent >= Asa::kGridDim) return;
        boundary = adjacent;
        int contributionPermille = 1000;
        int32_t accumSignal = NonNegative(signal[peakIdx]);
        while (boundary + step >= 0 && boundary + step < Asa::kGridDim &&
               NonNegative(signal[boundary + step]) < ((m_boundarySlopeQ5 * NonNegative(signal[boundary])) >> 5) &&
               contributionPermille > 50) {
            accumSignal += NonNegative(signal[boundary]);
            if (accumSignal < 200) accumSignal = 200;
            contributionPermille = (NonNegative(signal[boundary]) * 1000) / accumSignal;
            if (contributionPermille > 50) boundary += step;
        }
    }
};
} // namespace Solvers::Stylus
