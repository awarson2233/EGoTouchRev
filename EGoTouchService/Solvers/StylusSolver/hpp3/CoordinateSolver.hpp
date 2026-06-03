#pragma once

#include "SolverTypes.h"

#include <algorithm>
#include <array>

namespace Solvers::Stylus {

struct PitchCompensation {
    double c[4] = {0.0, 0.0, 0.0, 0.0};
    bool enabled = false;
};

struct TriangleEdgeParams {
    int ratio = 50;
    int sumThresholdIdxLast = 5000;
    int sumThresholdIdx0 = 5000;
};

class CoordinateSolver {
public:
    bool m_enabled = true;
    uint16_t m_signalFloor = 64;

    inline bool Process(HeatmapFrame& frame) const {
        auto& runtime = frame.stylus.runtime;
        auto& flow = runtime.flow;
        auto& tx1 = runtime.tx1;
        auto& tx2 = runtime.tx2;
        const auto dim1Edge = GetAxisEdgeGeometry(runtime.rawGrid.asaGrid.tx1.anchorCol, kSensorCols);
        const auto dim2Edge = GetAxisEdgeGeometry(runtime.rawGrid.asaGrid.tx1.anchorRow, kSensorRows);

        flow.pipelineStage = 4;
        if (!m_enabled || !tx1.feature.peak.valid) {
            flow.terminal = true;
            return true;
        }

        tx1.coordinate.localGridCoor = Solve(tx1.feature.projection, dim1Edge, dim2Edge);
        tx1.coordinate.reportGlobalCoor = tx1.coordinate.localGridCoor;
#if EGOTOUCH_DIAG
        {
            const auto& proj = tx1.feature.projection;
            const int peakIdx = proj.peakIdxDim1;
            if (peakIdx >= 0 && peakIdx < Asa::kGridDim) {
                const int leftIdx = peakIdx > 0 ? peakIdx - 1 : 0;
                const int rightIdx = peakIdx + 1 < Asa::kGridDim ? peakIdx + 1 : Asa::kGridDim - 1;
                tx1.triLeft = static_cast<uint16_t>(std::clamp(proj.dim1[leftIdx], 0, 65535));
                tx1.triCenter = static_cast<uint16_t>(std::clamp(proj.dim1[peakIdx], 0, 65535));
                tx1.triRight = static_cast<uint16_t>(std::clamp(proj.dim1[rightIdx], 0, 65535));
            }
            int32_t rawDim1 = SolveByTriangle(proj.dim1, proj.peakIdxDim1, kFactoryTriEdgeDim1, dim1Edge);
            if (rawDim1 != kInvalidCoor) {
                int32_t compDim1 = ApplyPitchCompensation(rawDim1, kFactoryPitchCompDim1);
                tx1.pitchComp = static_cast<int16_t>(compDim1 - rawDim1);
            }
        }
#endif
        if (tx1.coordinate.reportGlobalCoor.valid) {
            LocalToGlobal(tx1.coordinate.reportGlobalCoor,
                          runtime.rawGrid.asaGrid.tx1.anchorRow,
                          runtime.rawGrid.asaGrid.tx1.anchorCol,
                          kAnchorCenterOffset);
            CoordinateSolver::ClampToSensorBounds(tx1.coordinate.reportGlobalCoor);
            ApplyPitchMap(tx1.coordinate.reportGlobalCoor);
        }

        if (!tx1.coordinate.reportGlobalCoor.valid) {
            flow.terminal = true;
            flow.frameClass = Asa::StylusFrameClass::Tx1Missing;
            return true;
        }

        tx2.coordinate = {};

        auto& signal = runtime.signal;
        signal.signalX = static_cast<uint16_t>(std::clamp<int>(
            static_cast<int>(tx1.feature.peakSignal), 0, 0xFFFF));
        signal.signalY = static_cast<uint16_t>(std::clamp<int>(
            static_cast<int>(tx2.feature.peakSignal), 0, 0xFFFF));
        signal.maxRawPeak = std::max(signal.signalX, signal.signalY);
        signal.recheckPassed = signal.maxRawPeak >= m_signalFloor;
        signal.recheckEnabled = true;
        signal.recheckThreshold = m_signalFloor;
        signal.recheckThresholdMulti = static_cast<uint16_t>(std::max<uint16_t>(m_signalFloor, 256));

        signal.dim1EdgeActive = IsEdgeCoordinateCell(tx1.coordinate.reportGlobalCoor.dim1, kSensorCols);
        signal.dim2EdgeActive = IsEdgeCoordinateCell(tx1.coordinate.reportGlobalCoor.dim2, kSensorRows);
        signal.dim1EdgeSignal = signal.dim1EdgeActive ? tx1.feature.dim1SelectedPeakNetSignal : 0;
        signal.dim2EdgeSignal = signal.dim2EdgeActive ? tx1.feature.dim2SelectedPeakNetSignal : 0;

        runtime.decision.inRangeCandidate = tx1.coordinate.reportGlobalCoor.valid;
        flow.terminal = false;
        return true;
    }

private:
    static constexpr int kAnchorCenterOffset = Asa::kGridDim / 2;
    static constexpr int kSensorCols = 60;
    static constexpr int kSensorRows = 40;
    static constexpr int kInvalidCoor = 0x7FFFFFFF;
    static constexpr uint8_t kFactoryCoordCompFlags = 0x0E;
    static constexpr bool kFactoryUseTriangle = (kFactoryCoordCompFlags & 0x01) == 0;
    static constexpr bool kFactoryTriEdgeSecondaryBlend = (kFactoryCoordCompFlags & 0x08) != 0;
    static constexpr TriangleEdgeParams kFactoryTriEdgeDim1 = {50, 5000, 5000};
    static constexpr TriangleEdgeParams kFactoryTriEdgeDim2 = {50, 4500, 3700};
    static constexpr PitchCompensation kFactoryPitchCompDim1 = {
        {0.0, -1.7109151490662926, 0.005959771652221362, -5.113555667385272e-06}, true};
    static constexpr PitchCompensation kFactoryPitchCompDim2 = {
        {0.0, -1.4495726495726495, 0.004745726495726496, -3.7393162393162394e-06}, true};
    static constexpr std::array<double, Asa::kMaxSensorDim + 1> kFactoryPitchTableDim1 = {
        0.0,         0.984375,   1.96875,    2.953125,   3.9375,     4.921875,   5.90625,    6.890625,   7.875,
        8.859375,  9.84375,    10.8515625, 11.859375,  12.8671875, 13.875,     14.8828125, 15.890625,  16.8984375,
        17.90625,   18.9140625, 19.921875,  20.9296875, 21.9375,    22.9453125, 23.953125,  24.9609375, 25.96875,
        26.9765625, 27.984375,  28.9921875, 30.0,       31.0078125, 32.015625,  33.0234375, 34.03125,   35.0390625,
        36.046875,  37.0546875, 38.0625,    39.0703125, 40.078125,  41.0859375, 42.09375,   43.1015625, 44.109375,
        45.1171875, 46.125,     47.1328125, 48.140625,  49.1484375, 50.15625,   51.140625,  52.125,     53.109375,
        54.09375,   55.078125,  56.0625,    57.046875,  58.03125,   59.015625,  60.0,       60.0,       0.0,
        0.0,        0.0,        0.0,        0.0,        0.0,        0.0,        0.0,        0.0,        0.0,
        0.0,        0.0,        0.0,        0.0,        0.0,        0.0,        0.0,        0.0,        100.0};
    static constexpr std::array<double, Asa::kMaxSensorDim + 1> kFactoryPitchTableDim2 = {100.0};

    struct AxisEdgeGeometry {
        int lowIdx = -1;
        int highIdx = -1;
    };

    static inline bool IsValidLocalIndex(int idx) {
        return idx >= 0 && idx < Asa::kGridDim;
    }

    static inline AxisEdgeGeometry GetAxisEdgeGeometry(int anchor, int sensorCount) {
        AxisEdgeGeometry geometry{};
        const int lowIdx = kAnchorCenterOffset - anchor;
        const int highIdx = kAnchorCenterOffset + sensorCount - 1 - anchor;
        if (IsValidLocalIndex(lowIdx)) geometry.lowIdx = lowIdx;
        if (IsValidLocalIndex(highIdx)) geometry.highIdx = highIdx;
        return geometry;
    }

    static inline bool IsLowEdgePeak(int peakIdx, const AxisEdgeGeometry& geometry) {
        return IsValidLocalIndex(geometry.lowIdx) && peakIdx == geometry.lowIdx;
    }

    static inline bool IsHighEdgePeak(int peakIdx, const AxisEdgeGeometry& geometry) {
        return IsValidLocalIndex(geometry.highIdx) && peakIdx == geometry.highIdx;
    }

    static inline bool IsAnyEdgePeak(int peakIdx, const AxisEdgeGeometry& geometry) {
        return IsLowEdgePeak(peakIdx, geometry) || IsHighEdgePeak(peakIdx, geometry);
    }

    static inline bool IsEdgeCoordinateCell(int32_t coord, int sensorCount) {
        return coord < Asa::kCoorUnit || coord >= (sensorCount - 1) * Asa::kCoorUnit;
    }

    static inline bool HasRequiredLowNeighbors(int edgeIdx) {
        return edgeIdx + 2 < Asa::kGridDim;
    }

    static inline bool HasRequiredHighNeighbors(int edgeIdx) {
        return edgeIdx - 2 >= 0;
    }

    inline Asa::AsaCoorResult Solve(const Asa::AsaProjection& proj,
                                    const AxisEdgeGeometry& dim1Edge,
                                    const AxisEdgeGeometry& dim2Edge) const {
        Asa::AsaCoorResult result{};
        if (!kFactoryUseTriangle) return result;
        int32_t dim1 = SolveByTriangle(proj.dim1, proj.peakIdxDim1, kFactoryTriEdgeDim1, dim1Edge);
        int32_t dim2 = SolveByTriangle(proj.dim2, proj.peakIdxDim2, kFactoryTriEdgeDim2, dim2Edge);

        if (dim1 == kInvalidCoor || dim2 == kInvalidCoor) return result;

        dim1 = ApplyPitchCompensation(dim1, kFactoryPitchCompDim1);
        dim2 = ApplyPitchCompensation(dim2, kFactoryPitchCompDim2);

        const int32_t maxDim = Asa::kGridDim * Asa::kCoorUnit - 1;
        result.valid = true;
        result.dim1 = std::clamp(dim1, 0, maxDim);
        result.dim2 = std::clamp(dim2, 0, maxDim);
        return result;
    }

    static inline int32_t TriangleAlgUsing3Point(int left, int center, int right) {
        if (right < left) {
            int minVal = right;
            if (center <= right) minVal = center - 1;
            const int den = center - minVal;
            const int offset = (((left - minVal) * Asa::kCoorUnit) / den) / 2;
            return (Asa::kCoorUnit / 2) - offset;
        }
        int minVal = left;
        if (center <= left) minVal = center - 1;
        const int den = center - minVal;
        const int offset = (((right - minVal) * Asa::kCoorUnit) / den) / 2;
        return offset + (Asa::kCoorUnit / 2);
    }

    inline int32_t EdgeCompensating(int peak, int n1, int n2, int ratio, int threshold) const {
        const int safeRatio = (ratio == 0) ? 1 : ratio;
        int virtualNeighbor = ((peak - n1) * 10) / safeRatio;
        const int comp2 = peak - ((n1 - n2) * safeRatio) / 10;
        if (virtualNeighbor < comp2) {
            virtualNeighbor = comp2;
            if (kFactoryTriEdgeSecondaryBlend) {
                int gate = comp2;
                const int sum = peak + n1 + comp2;
                if (sum < threshold) gate = threshold - peak - n1;
                if (comp2 < gate) virtualNeighbor = (comp2 + gate) / 2;
            }
        }
        if (peak <= virtualNeighbor) virtualNeighbor = peak - 1;
        return virtualNeighbor;
    }

    inline int32_t TriangleAlgEdge(int peak, int n1, int n2, int ratio, int threshold) const {
        const int virtualNeighbor = EdgeCompensating(peak, n1, n2, ratio, threshold);
        int result = TriangleAlgUsing3Point(virtualNeighbor, peak, n1);
        if (peak + n1 + n2 < (threshold * 2) / 5) result = 0;
        return result;
    }

    inline int32_t SolveByTriangle(const int32_t (&signal)[Asa::kGridDim],
                                   int peakIdx,
                                   const TriangleEdgeParams& edge,
                                   const AxisEdgeGeometry& geometry) const {
        if (!IsValidLocalIndex(peakIdx)) return kInvalidCoor;
        const auto s = [&](int i) -> int {
            if (!IsValidLocalIndex(i)) return 0;
            return static_cast<int>(std::clamp(signal[i], 0, 65535));
        };

        if (IsLowEdgePeak(peakIdx, geometry) && HasRequiredLowNeighbors(peakIdx)) {
            return peakIdx * Asa::kCoorUnit +
                   TriangleAlgEdge(s(peakIdx), s(peakIdx + 1), s(peakIdx + 2), edge.ratio, edge.sumThresholdIdx0);
        }
        if (IsHighEdgePeak(peakIdx, geometry) && HasRequiredHighNeighbors(peakIdx)) {
            const int e = TriangleAlgEdge(s(peakIdx), s(peakIdx - 1), s(peakIdx - 2),
                                          edge.ratio, edge.sumThresholdIdxLast);
            return (peakIdx + 1) * Asa::kCoorUnit - e;
        }
        const int offset = TriangleAlgUsing3Point(s(peakIdx - 1), s(peakIdx), s(peakIdx + 1));
        return peakIdx * Asa::kCoorUnit + offset;
    }

    static inline int32_t ApplyPitchCompensation(int32_t coor, const PitchCompensation& comp) {
        if (!comp.enabled) return coor;
        const int remainder = ((coor % Asa::kCoorUnit) + Asa::kCoorUnit) % Asa::kCoorUnit;
        const int x = (remainder < 0x201) ? (0x200 - remainder) : (remainder - 0x200);
        const double dx = static_cast<double>(x);
        int compensation = static_cast<int>(
            comp.c[0] + comp.c[1] * dx + comp.c[2] * dx * dx + comp.c[3] * dx * dx * dx);
        if (remainder >= 0x201) compensation = -compensation;
        return coor + compensation;
    }

    void ApplyPitchMap(Asa::AsaCoorResult& coor) const {
        if (!coor.valid) return;
        coor.dim1 = Asa::SensorPitchSizeMap(coor.dim1, kFactoryPitchTableDim1.data(), Asa::kCoorUnit);
        coor.dim2 = Asa::SensorPitchSizeMap(coor.dim2, kFactoryPitchTableDim2.data(), Asa::kCoorUnit);
    }

    static inline void LocalToGlobal(Asa::AsaCoorResult& coor,
                                     int anchorRow, int anchorCol, int anchorCenterOffset) {
        if (!coor.valid) return;
        const int32_t centerOff = anchorCenterOffset * Asa::kCoorUnit;
        coor.dim1 += static_cast<int32_t>(anchorCol) * Asa::kCoorUnit - centerOff;
        coor.dim2 += static_cast<int32_t>(anchorRow) * Asa::kCoorUnit - centerOff;
    }

    static inline void ClampToSensorBounds(Asa::AsaCoorResult& coor) {
        if (!coor.valid) return;
        coor.dim1 = std::clamp(coor.dim1, 0, kSensorCols * Asa::kCoorUnit - 1);
        coor.dim2 = std::clamp(coor.dim2, 0, kSensorRows * Asa::kCoorUnit - 1);
    }
};

} // namespace Solvers::Stylus
