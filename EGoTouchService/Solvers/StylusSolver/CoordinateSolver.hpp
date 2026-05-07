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
    bool m_useTriangle = true;
    bool m_triEdgeSecondaryBlend = true;
    bool m_pitchMapEnabled = false;
    bool m_gravityFictitiousEdge = true;
    int32_t m_gravityNoiseFloor = 0;

    TriangleEdgeParams m_triEdgeDim1 = {50, 5000, 5000};
    TriangleEdgeParams m_triEdgeDim2 = {50, 4500, 3700};

    PitchCompensation m_pitchCompDim1{};
    PitchCompensation m_pitchCompDim2{};

    std::array<double, Asa::kMaxSensorDim + 1> m_pitchTableDim1 = [] {
        std::array<double, Asa::kMaxSensorDim + 1> table{};
        table.fill(100.0);
        return table;
    }();

    std::array<double, Asa::kMaxSensorDim + 1> m_pitchTableDim2 = [] {
        std::array<double, Asa::kMaxSensorDim + 1> table{};
        table.fill(100.0);
        return table;
    }();

    inline bool Process(HeatmapFrame& frame) const {
        auto& stylus = frame.stylus;
        auto& flow = stylus.runtime.flow;

        flow.pipelineStage = 4;
        if (!m_enabled || !stylus.runtime.tx1.peak.valid) {
            flow.terminal = true;
            return true;
        }

        stylus.runtime.tx1.localCoor = Solve(
            stylus.runtime.tx1.projection,
            m_triEdgeDim1, m_triEdgeDim2,
            m_pitchCompDim1, m_pitchCompDim2);
        stylus.runtime.tx1.globalCoor = stylus.runtime.tx1.localCoor;
        if (stylus.runtime.tx1.globalCoor.valid) {
            LocalToGlobal(stylus.runtime.tx1.globalCoor,
                          stylus.runtime.parse.gridData.tx1.anchorRow,
                          stylus.runtime.parse.gridData.tx1.anchorCol,
                          kAnchorCenterOffset);
            if (m_pitchMapEnabled) ApplyPitchMap(stylus.runtime.tx1.globalCoor);
        }

        if (!stylus.runtime.tx1.globalCoor.valid) {
            flow.terminal = true;
            flow.frameClass = Asa::StylusFrameClass::Tx1Missing;
            return true;
        }

        if (stylus.runtime.parse.gridData.tx2.valid && stylus.runtime.tx2.peak.valid) {
            stylus.runtime.tx2.localCoor = Solve(
                stylus.runtime.tx2.projection,
                m_triEdgeDim1, m_triEdgeDim2,
                m_pitchCompDim1, m_pitchCompDim2);
            stylus.runtime.tx2.globalCoor = stylus.runtime.tx2.localCoor;
            if (stylus.runtime.tx2.globalCoor.valid) {
                LocalToGlobal(stylus.runtime.tx2.globalCoor,
                              stylus.runtime.parse.gridData.tx2.anchorRow,
                              stylus.runtime.parse.gridData.tx2.anchorCol,
                              kAnchorCenterOffset);
                if (m_pitchMapEnabled) ApplyPitchMap(stylus.runtime.tx2.globalCoor);
            }
        } else {
            stylus.runtime.tx2.localCoor = {};
            stylus.runtime.tx2.globalCoor = {};
        }

        auto& signal = stylus.runtime.signal;
        signal.signalX = static_cast<uint16_t>(std::clamp<int>(
            static_cast<int>(stylus.runtime.tx1.peakSignal), 0, 0xFFFF));
        signal.signalY = static_cast<uint16_t>(std::clamp<int>(
            static_cast<int>(stylus.runtime.tx2.peakSignal), 0, 0xFFFF));
        signal.maxRawPeak = std::max(signal.signalX, signal.signalY);
        signal.tx1Composite = signal.signalX;
        signal.tx2Composite = signal.signalY;
        signal.recheckPassed = signal.maxRawPeak >= m_signalFloor;
        signal.recheckEnabled = true;
        signal.recheckThreshold = m_signalFloor;
        signal.recheckThresholdMulti = static_cast<uint16_t>(std::max<uint16_t>(m_signalFloor, 256));

        signal.dim1EdgeActive =
            stylus.runtime.tx1.projection.peakIdxDim1 == 0 ||
            stylus.runtime.tx1.projection.peakIdxDim1 == (Asa::kGridDim - 1);
        signal.dim2EdgeActive =
            stylus.runtime.tx1.projection.peakIdxDim2 == 0 ||
            stylus.runtime.tx1.projection.peakIdxDim2 == (Asa::kGridDim - 1);
        signal.dim1EdgeSignal = signal.dim1EdgeActive ? signal.signalX : 0;
        signal.dim2EdgeSignal = signal.dim2EdgeActive ? signal.signalY : 0;

        stylus.runtime.decision.inRangeCandidate = stylus.runtime.tx1.globalCoor.valid;
        flow.terminal = false;
        return true;
    }

private:
    static constexpr int kAnchorCenterOffset = Asa::kGridDim / 2;
    static constexpr int kInvalidCoor = 0x7FFFFFFF;

    inline Asa::AsaCoorResult Solve(const Asa::AsaProjection& proj,
                                    const TriangleEdgeParams& edgeDim1,
                                    const TriangleEdgeParams& edgeDim2,
                                    const PitchCompensation& pitchDim1,
                                    const PitchCompensation& pitchDim2) const {
        Asa::AsaCoorResult result{};
        int32_t dim1 = m_useTriangle
            ? SolveByTriangle(proj.dim1, proj.peakIdxDim1, edgeDim1)
            : SolveByGravity(proj.dim1, proj.peakIdxDim1);
        int32_t dim2 = m_useTriangle
            ? SolveByTriangle(proj.dim2, proj.peakIdxDim2, edgeDim2)
            : SolveByGravity(proj.dim2, proj.peakIdxDim2);

        if (dim1 == kInvalidCoor || dim2 == kInvalidCoor) return result;

        dim1 = ApplyPitchCompensation(dim1, pitchDim1);
        dim2 = ApplyPitchCompensation(dim2, pitchDim2);

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
            if (m_triEdgeSecondaryBlend) {
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
                                   int peakIdx, const TriangleEdgeParams& edge) const {
        if (peakIdx < 0 || peakIdx >= Asa::kGridDim) return kInvalidCoor;
        const auto s = [&](int i) -> int { return static_cast<int>(std::clamp(signal[i], 0, 65535)); };

        if (peakIdx == 0)
            return TriangleAlgEdge(s(0), s(1), s(2), edge.ratio, edge.sumThresholdIdx0);
        if (peakIdx == Asa::kGridDim - 1) {
            const int e = TriangleAlgEdge(s(Asa::kGridDim - 1), s(Asa::kGridDim - 2),
                                          s(Asa::kGridDim - 3), edge.ratio, edge.sumThresholdIdxLast);
            return Asa::kGridDim * Asa::kCoorUnit - e;
        }
        const int offset = TriangleAlgUsing3Point(s(peakIdx - 1), s(peakIdx), s(peakIdx + 1));
        return peakIdx * Asa::kCoorUnit + offset;
    }

    inline int32_t SolveByGravity(const int32_t (&signal)[Asa::kGridDim], int peakIdx) const {
        if (peakIdx < 0 || peakIdx >= Asa::kGridDim) return kInvalidCoor;

        constexpr int kMaxBuf = 5;
        int32_t buf[kMaxBuf] = {};
        int bufLen = 0;
        int startIdx = peakIdx - 1;

        auto clampSig = [&](int i) -> int32_t {
            if (i < 0 || i >= Asa::kGridDim) return 0;
            return std::max<int32_t>(0, signal[i] - m_gravityNoiseFloor);
        };

        if (peakIdx == 0) {
            startIdx = -1;
            const int endIdx = std::min(peakIdx + 2, Asa::kGridDim - 1);
            const int32_t baseline = clampSig(endIdx);
            if (m_gravityFictitiousEdge && Asa::kGridDim >= 2) {
                buf[bufLen++] = std::max<int32_t>(0, clampSig(1) - baseline);
            } else { startIdx = 0; }
            for (int i = 0; i <= endIdx; ++i)
                buf[bufLen++] = std::max<int32_t>(0, clampSig(i) - baseline);
        } else if (peakIdx == Asa::kGridDim - 1) {
            startIdx = std::max(0, peakIdx - 2);
            const int32_t baseline = clampSig(startIdx);
            for (int i = startIdx; i < Asa::kGridDim; ++i)
                buf[bufLen++] = std::max<int32_t>(0, clampSig(i) - baseline);
            if (m_gravityFictitiousEdge && Asa::kGridDim >= 2)
                buf[bufLen++] = std::max<int32_t>(0, clampSig(Asa::kGridDim - 2) - baseline);
        } else {
            const int32_t left = clampSig(peakIdx - 1);
            const int32_t center = clampSig(peakIdx);
            const int32_t right = clampSig(peakIdx + 1);
            const int32_t baseline = std::min(left, right);
            buf[0] = std::max<int32_t>(0, left - baseline);
            buf[1] = std::max<int32_t>(0, center - baseline);
            buf[2] = std::max<int32_t>(0, right - baseline);
            bufLen = 3;
        }

        int64_t weighted = 0, total = 0;
        for (int i = 0; i < bufLen; ++i) {
            weighted += static_cast<int64_t>(i) * buf[i];
            total += buf[i];
        }
        if (total <= 0) return kInvalidCoor;

        const int32_t gravity = static_cast<int32_t>(
            (weighted * Asa::kCoorUnit) / total) + (Asa::kCoorUnit / 2);
        const int32_t result = startIdx * Asa::kCoorUnit + gravity;
        return std::clamp(result, 0, (Asa::kGridDim - 1) * Asa::kCoorUnit);
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
        coor.dim1 = Asa::SensorPitchSizeMap(coor.dim1, m_pitchTableDim1.data(), Asa::kCoorUnit);
        coor.dim2 = Asa::SensorPitchSizeMap(coor.dim2, m_pitchTableDim2.data(), Asa::kCoorUnit);
    }

    static inline void LocalToGlobal(Asa::AsaCoorResult& coor,
                                     int anchorRow, int anchorCol, int anchorCenterOffset) {
        if (!coor.valid) return;
        const int32_t centerOff = anchorCenterOffset * Asa::kCoorUnit;
        coor.dim1 += static_cast<int32_t>(anchorCol) * Asa::kCoorUnit - centerOff;
        coor.dim2 += static_cast<int32_t>(anchorRow) * Asa::kCoorUnit - centerOff;
    }
};

} // namespace Solvers::Stylus
