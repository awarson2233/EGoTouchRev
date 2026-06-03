#pragma once

#include "StylusSolver/AsaTypes.hpp"
#include "SolverTypes.h"

#include <cstdint>

namespace Solvers::Stylus {

// EdgeCoorPostProcess – edge-region coordinate remapping.
//
// Replicates TSACore EdgeCoorPostProcess (0x6baaf688).
// When the pen is down and the coordinate falls within approximately one
// coordinate unit (0x400) of the sensor boundary the raw capacitive
// reading becomes non-linear and "pulls" toward the edge.  This module
// remaps those edge-affected coordinates:
//
//   Left/top  edge:  clamp [0, 64) to 0;  stretch [64, 1024) to [0, 1024).
//   Right/bot edge:  mirror of the left-edge logic.
//
// Only operates when pen pressure is non-zero.
class EdgeCoorPostProcess {
public:
    bool m_enabled = true;

    // Sensor dimensions – matches asa 0xa28 (bTxCount=60, bRxCount=40).
    int m_sensorTxCount = 60;
    int m_sensorRxCount = 40;

    inline void Reset() {}

    inline void Process(HeatmapFrame& frame) const {
        auto& runtime = frame.stylus.runtime;

        const auto& raw = runtime.tx1.coordinate.reportGlobalCoor;
        runtime.post.edgePostCoor = raw;
        runtime.post.finalCoor = raw;
        runtime.post.finalValid = raw.valid;

        if (!m_enabled) return;
        if (runtime.pressure.outputPressure == 0) return;

        auto& coor = runtime.post.edgePostCoor;
        if (!coor.valid) return;

        RemapAxis(coor.dim1, m_sensorTxCount);
        RemapAxis(coor.dim2, m_sensorRxCount);
        runtime.post.finalCoor = coor;
    }

private:
    // Hard-coded constants matching the original firmware (not in flash tables).
    static constexpr int kDeadZone  = 0x40;   //  64 – dead zone at the absolute edge
    static constexpr int kRemapSpan = 0x3c0;  // 960 – linear stretch range (= 0x400 - 0x40)

    inline static void RemapAxis(int32_t& coord, int sensorCount) {
        const int sensorMax = sensorCount * Asa::kCoorUnit;

        if (coord < Asa::kCoorUnit) {
            // Near the leading (left / top) edge.
            const uint16_t u = static_cast<uint16_t>(coord);
            if (u < kDeadZone) {
                coord = 0;
            } else {
                coord = (static_cast<int>(u - kDeadZone) * Asa::kCoorUnit) / kRemapSpan;
            }
        } else if (coord > (sensorCount - 1) * Asa::kCoorUnit) {
            // Near the trailing (right / bottom) edge.
            const uint16_t dist = static_cast<uint16_t>(sensorMax - coord);
            if (dist < kDeadZone) {
                coord = sensorMax;
            } else {
                coord = sensorMax -
                        (static_cast<int>(dist - kDeadZone) * Asa::kCoorUnit) / kRemapSpan;
            }
        }
    }
};

} // namespace Solvers::Stylus
