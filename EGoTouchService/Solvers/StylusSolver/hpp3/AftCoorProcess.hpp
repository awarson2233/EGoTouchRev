#pragma once

#include "StylusSolver/AsaTypes.hpp"
#include "SolverTypes.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>

namespace Solvers::Stylus {

class AftCoorProcess {
public:
    bool m_enabled = true;

    // ── Lock thresholds (flash params equivalent) ──
    // In-band thresholds (coordinate within sensor interior)
    uint8_t m_lockFlashInBandX = 0;  // flash[0xA5A]
    uint8_t m_lockFlashInBandY = 0;  // flash[0xA5B]

    // Edge thresholds (coordinate near sensor boundary)
    uint8_t m_lockFlashEdgeX = 1;  // flash[0xA58]
    uint8_t m_lockFlashEdgeY = 2;  // flash[0xA59]

    // Sensor dimensions (from asaPrmt)
    int m_sensorTxCount = 60;   // bTxCount
    int m_sensorRxCount = 40;   // bRxCount
    int m_sensorDim1 = 0x0A00;  // wField00
    int m_sensorDim2 = 0x0640;  // wField02

    // Bypass gate: skip lock when this ASA-style flag is active
    bool m_bypassLock = false;

    inline void Reset() {
        m_startX = 0;
        m_startY = 0;
        m_lockOffsetX = 0;
        m_lockOffsetY = 0;
        m_flagLockX = false;
        m_flagLockY = false;
        m_prevPressure = 0;
    }

    inline void Process(HeatmapFrame& frame) {
        auto& runtime = frame.stylus.runtime;
        auto& coor = runtime.post.finalCoor;
        const uint16_t curPressure = runtime.pressure.outputPressure;

        if (!m_enabled || m_bypassLock) {
            // Pass-through: just clamp to sensor bounds
            if (coor.valid) {
                ClampToSensor(coor);
            }
            m_prevPressure = curPressure;
            return;
        }

        if (!coor.valid) {
            Reset();
            m_prevPressure = curPressure;
            return;
        }

        // ── Select thresholds based on coordinate position ──
        // TSACore: if coor < 0x401 or coor >= (count-1)*0x400 → edge threshold
        const int32_t minBound = 0x401;
        const int32_t maxBoundX = (m_sensorTxCount - 1) * Asa::kCoorUnit;
        const int32_t maxBoundY = (m_sensorRxCount - 1) * Asa::kCoorUnit;

        uint32_t thresholdX;
        uint32_t thresholdY;

        if (coor.dim1 < minBound || coor.dim2 < minBound ||
            coor.dim1 >= maxBoundX || coor.dim2 >= maxBoundY) {
            thresholdX = ScaleLockThreshold(m_lockFlashEdgeX, m_sensorTxCount, m_sensorDim1);
            thresholdY = ScaleLockThreshold(m_lockFlashEdgeY, m_sensorRxCount, m_sensorDim2);
        } else {
            thresholdX = ScaleLockThreshold(m_lockFlashInBandX, m_sensorTxCount, m_sensorDim1);
            thresholdY = ScaleLockThreshold(m_lockFlashInBandY, m_sensorRxCount, m_sensorDim2);
        }

        // ── Pen-down transition: lock to start position ──
        if (curPressure != 0 && m_prevPressure == 0) {
            m_startX = coor.dim1;
            m_startY = coor.dim2;
            m_flagLockX = true;
            m_flagLockY = true;
            m_lockOffsetX = 0;
            m_lockOffsetY = 0;
        }

        // ── X-axis lock ──
        if (m_flagLockX) {
            const int32_t diff = coor.dim1 - m_startX;
            const uint32_t absDiff = static_cast<uint32_t>(std::abs(diff));
            if (absDiff > thresholdX) {
                m_flagLockX = false;  // movement exceeded threshold → release lock
            }
            if (m_flagLockX) {
                m_lockOffsetX = diff;  // track offset while locked
            }
        }

        // ── Y-axis lock ──
        if (m_flagLockY) {
            const int32_t diff = coor.dim2 - m_startY;
            const uint32_t absDiff = static_cast<uint32_t>(std::abs(diff));
            if (absDiff > thresholdY) {
                m_flagLockY = false;
            }
            if (m_flagLockY) {
                m_lockOffsetY = diff;
            }
        }

        // ── Apply offset and clamp ──
        int32_t finalX = coor.dim1 - m_lockOffsetX;
        int32_t finalY = coor.dim2 - m_lockOffsetY;

        finalX = std::clamp(finalX, 0, m_sensorTxCount * Asa::kCoorUnit);
        finalY = std::clamp(finalY, 0, m_sensorRxCount * Asa::kCoorUnit);

        coor.dim1 = finalX;
        coor.dim2 = finalY;

#if EGOTOUCH_DIAG
        runtime.post.lockActiveX = m_flagLockX;
        runtime.post.lockActiveY = m_flagLockY;
        runtime.post.lockOffsetX = m_lockOffsetX;
        runtime.post.lockOffsetY = m_lockOffsetY;
        runtime.post.lockThresholdX = static_cast<int32_t>(thresholdX);
        runtime.post.lockThresholdY = static_cast<int32_t>(thresholdY);
#endif

        m_prevPressure = curPressure;
    }

private:
    int32_t m_startX = 0;
    int32_t m_startY = 0;
    int32_t m_lockOffsetX = 0;
    int32_t m_lockOffsetY = 0;
    bool m_flagLockX = false;
    bool m_flagLockY = false;
    uint16_t m_prevPressure = 0;

    static inline uint32_t ScaleLockThreshold(uint8_t flashValue, int sensorCount, int physicalSize) {
        if (physicalSize <= 0) return 0;
        return (static_cast<uint32_t>(flashValue) * static_cast<uint32_t>(sensorCount) * Asa::kCoorUnit) /
               static_cast<uint32_t>(physicalSize);
    }

    inline void ClampToSensor(Asa::AsaCoorResult& coor) const {
        coor.dim1 = std::clamp(coor.dim1, 0, m_sensorTxCount * Asa::kCoorUnit);
        coor.dim2 = std::clamp(coor.dim2, 0, m_sensorRxCount * Asa::kCoorUnit);
    }
};

} // namespace Solvers::Stylus
