#pragma once

#include "Hpp2Runtime.hpp"
#include "StylusSolver/AsaTypes.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace Solvers::Stylus::Hpp2 {

struct Hpp2CoordinateMath {
    static int32_t ApplyPitchCompensation(int32_t coor, const std::array<double, 4>& comp) {
        const int remainder = ((coor % Asa::kCoorUnit) + Asa::kCoorUnit) % Asa::kCoorUnit;
        const int x = (remainder < 0x201) ? (0x200 - remainder) : (remainder - 0x200);
        const double dx = static_cast<double>(x);
        int correction = static_cast<int>(comp[0] + comp[1] * dx + comp[2] * dx * dx + comp[3] * dx * dx * dx);
        if (remainder >= 0x201) {
            correction = -correction;
        }
        return coor + correction;
    }

    static int32_t SolveByTriangle(const std::array<uint16_t, kMaxSamples>& line,
                                   int offset,
                                   int length,
                                   int peakIdx,
                                   int edgeRatio,
                                   int edgeThresholdLast,
                                   int edgeThresholdFirst) {
        if (peakIdx < 0 || peakIdx >= length) {
            return -1;
        }
        const auto s = [&](int localIndex) -> int {
            if (localIndex < 0 || localIndex >= length) {
                return 0;
            }
            return line[static_cast<std::size_t>(offset + localIndex)];
        };

        if (peakIdx == 0 && length >= 3) {
            return TriangleAlgEdge(s(0), s(1), s(2), edgeRatio, edgeThresholdFirst);
        }
        if (peakIdx == length - 1 && length >= 3) {
            const int edge = TriangleAlgEdge(s(peakIdx), s(peakIdx - 1), s(peakIdx - 2), edgeRatio, edgeThresholdLast);
            return length * Asa::kCoorUnit - edge;
        }
        const int offsetInCell = TriangleAlgUsing3Point(s(peakIdx - 1), s(peakIdx), s(peakIdx + 1));
        return peakIdx * Asa::kCoorUnit + offsetInCell;
    }

    static int TriangleAlgUsing3Point(int left, int center, int right) {
        if (center <= 0) {
            return Asa::kCoorUnit / 2;
        }
        if (right < left) {
            int minVal = right;
            if (center <= right) {
                minVal = center - 1;
            }
            const int den = std::max(1, center - minVal);
            const int offset = (((left - minVal) * Asa::kCoorUnit) / den) / 2;
            return (Asa::kCoorUnit / 2) - offset;
        }
        int minVal = left;
        if (center <= left) {
            minVal = center - 1;
        }
        const int den = std::max(1, center - minVal);
        const int offset = (((right - minVal) * Asa::kCoorUnit) / den) / 2;
        return offset + (Asa::kCoorUnit / 2);
    }

    static int TriangleAlgEdge(int peak, int n1, int n2, int ratio, int threshold) {
        const int safeRatio = ratio == 0 ? 1 : ratio;
        int virtualNeighbor = ((peak - n1) * 10) / safeRatio;
        const int comp2 = peak - ((n1 - n2) * safeRatio) / 10;
        if (virtualNeighbor < comp2) {
            virtualNeighbor = comp2;
            const int sum = peak + n1 + comp2;
            int gate = comp2;
            if (sum < threshold) {
                gate = threshold - peak - n1;
            }
            if (comp2 < gate) {
                virtualNeighbor = (comp2 + gate) / 2;
            }
        }
        if (peak <= virtualNeighbor) {
            virtualNeighbor = peak - 1;
        }
        int result = TriangleAlgUsing3Point(virtualNeighbor, peak, n1);
        if (peak + n1 + n2 < (threshold * 2) / 5) {
            result = 0;
        }
        return result;
    }
};

class Hpp2CoordinateSolver {
public:
    /// 当前面板机型的 dim1 走线间距表（StylusPipeline::SetPanelProjectId 下发）。
    void SetPitchMapDim1(const Asa::PitchMap& map) noexcept { m_pitchMapDim1 = map; }

    bool Process(Context& ctx) const {
        auto& runtime = ctx.runtime;
        const auto& hpp2 = runtime;
        if (hpp2.selectedPeakDim1 == kInvalidPeak || hpp2.selectedPeakDim2 == kInvalidPeak) {
            return false;
        }

        int32_t dim1 = Hpp2CoordinateMath::SolveByTriangle(hpp2.line.cmnSubtracted, 0, ctx.settings.sensorTxCount,
                                                           hpp2.selectedPeakDim1, 50, 5000, 5000);
        int32_t dim2 = Hpp2CoordinateMath::SolveByTriangle(hpp2.line.cmnSubtracted, ctx.settings.sensorTxCount, ctx.settings.sensorRxCount,
                                                           hpp2.selectedPeakDim2, 50, 4500, 3700);
        if (dim1 < 0 || dim2 < 0) {
            return false;
        }

        dim1 = Hpp2CoordinateMath::ApplyPitchCompensation(dim1, kPitchCompDim1);
        dim2 = Hpp2CoordinateMath::ApplyPitchCompensation(dim2, kPitchCompDim2);
        dim1 = Asa::SensorPitchSizeMap(dim1, m_pitchMapDim1.data(), Asa::kCoorUnit);
        dim2 = Asa::SensorPitchSizeMap(dim2, Asa::kIdentityPitchMap.data(), Asa::kCoorUnit);

        Asa::CoorResult coor{};
        coor.valid = true;
        coor.dim1 = std::clamp(dim1, 0, ctx.settings.sensorTxCount * Asa::kCoorUnit - 1);
        coor.dim2 = std::clamp(dim2, 0, ctx.settings.sensorRxCount * Asa::kCoorUnit - 1);
        runtime.tx1.coordinate.localGridCoor = coor;
        runtime.tx1.coordinate.reportGlobalCoor = coor;
        runtime.post.finalCoor = coor;
        runtime.post.finalValid = true;
        return true;
    }

private:
    static constexpr std::array<double, 4> kPitchCompDim1{{
        0.0, -1.7109151490662926, 0.005959771652221362, -5.113555667385272e-06}};
    static constexpr std::array<double, 4> kPitchCompDim2{{
        0.0, -1.4495726495726495, 0.004745726495726496, -3.7393162393162394e-06}};
    // dim2 的表各机型都是不重映射的哨兵
    Asa::PitchMap m_pitchMapDim1 = Asa::kIdentityPitchMap;
};

} // namespace Solvers::Stylus::Hpp2
