#pragma once

#include "SolverTypes.h"
#include "Hpp2Runtime.hpp"
#include "Hpp2CoordinateMath.hpp"
#include "StylusSolver/AsaTypes.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

namespace Solvers::Stylus::Hpp2 {

class Hpp2CoordinateSolver {
public:
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
        dim1 = Asa::SensorPitchSizeMap(dim1, kDim1PitchTable.data(), Asa::kCoorUnit);
        dim2 = Asa::SensorPitchSizeMap(dim2, kDim2PitchTable.data(), Asa::kCoorUnit);

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
    static constexpr std::array<double, Asa::kMaxSensorDim + 1> kDim1PitchTable{{
        0, 0.984375, 1.96875, 2.953125, 3.9375, 4.921875, 5.90625, 6.890625, 7.875,
        8.859375, 9.84375, 10.8515625, 11.859375, 12.8671875, 13.875, 14.8828125,
        15.890625, 16.8984375, 17.90625, 18.9140625, 19.921875, 20.9296875, 21.9375,
        22.9453125, 23.953125, 24.9609375, 25.96875, 26.9765625, 27.984375, 28.9921875,
        30, 31.0078125, 32.015625, 33.0234375, 34.03125, 35.0390625, 36.046875,
        37.0546875, 38.0625, 39.0703125, 40.078125, 41.0859375, 42.09375, 43.1015625,
        44.109375, 45.1171875, 46.125, 47.1328125, 48.140625, 49.1484375, 50.15625,
        51.140625, 52.125, 53.109375, 54.09375, 55.078125, 56.0625, 57.046875,
        58.03125, 59.015625, 60, 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 100}};
    static constexpr std::array<double, Asa::kMaxSensorDim + 1> kDim2PitchTable{{100.0}};
};

} // namespace Solvers::Stylus::Hpp2
