#pragma once
#include "AsaTypes.hpp"
#include <algorithm>
#include <array>
#include <cstdint>

namespace Asa {

/// CalibrationAvg — Rolling N-frame average on final LOCAL coordinates.
///
/// Mirrors TSACore ASACalibration_Process (Phase 6).
/// Simple N-frame rolling average with configurable window.
class CalibrationAvg {
public:
    inline AsaCoorResult Apply(const AsaCoorResult& c) {
        if (!enabled) return c;
        if (!c.valid) { Reset(); return c; }

        int idx = m_count % kWindow;
        m_dim1[static_cast<size_t>(idx)] = c.dim1;
        m_dim2[static_cast<size_t>(idx)] = c.dim2;
        m_count = std::min(m_count + 1, kWindow);

        int32_t s1 = 0, s2 = 0;
        for (int i = 0; i < m_count; ++i) {
            s1 += m_dim1[static_cast<size_t>(i)];
            s2 += m_dim2[static_cast<size_t>(i)];
        }
        AsaCoorResult out = c;
        out.dim1 = s1 / m_count;
        out.dim2 = s2 / m_count;
        return out;
    }

    inline void Reset() {
        m_count = 0;
        m_dim1.fill(0);
        m_dim2.fill(0);
    }

    // ── Configuration ──
    bool enabled = false;

private:
    static constexpr int kWindow = 5;
    int m_count = 0;
    std::array<int32_t, kWindow> m_dim1{};
    std::array<int32_t, kWindow> m_dim2{};
};

/// EdgeCoorPostProcess — Edge dead-zone coordinate compensation.
///
/// Mirrors TSACore EdgeCoorPostProcess.
/// Compensates for signal attenuation at the first and last sensor cell
/// by applying a linear rescale within the edge active zone.
class EdgeCoorPost {
public:
    inline void Apply(float& dim1, float& dim2,
                      int sensorCols, int sensorRows) const {
        if (!enabled) return;
        dim1 = EdgeClamp(dim1, sensorCols);
        dim2 = EdgeClamp(dim2, sensorRows);
    }

    bool enabled = true;

private:
    static constexpr int kDeadZone = 0x40;     // 64/1024 = 6.25%
    static constexpr int kCellUnit = 0x400;    // 1024 units per cell
    static constexpr int kActiveZone = kCellUnit - kDeadZone;

    static inline float EdgeClamp(float coord, int sensorDim) {
        constexpr float deadZone   = static_cast<float>(kDeadZone);
        constexpr float cellUnit   = static_cast<float>(kCellUnit);
        constexpr float activeZone = static_cast<float>(kActiveZone);
        const float maxCoord = static_cast<float>(sensorDim) * cellUnit;

        // First cell: coord in [0, cellUnit)
        if (coord < cellUnit) {
            if (coord < deadZone) return 0.0f;
            return (coord - deadZone) * cellUnit / activeZone;
        }
        // Last cell
        const float lastCellStart = static_cast<float>(sensorDim - 1) * cellUnit;
        if (coord > lastCellStart) {
            float distFromEnd = maxCoord - coord;
            if (distFromEnd < deadZone) return maxCoord;
            return maxCoord - (distFromEnd - deadZone) * cellUnit / activeZone;
        }
        return coord;  // interior: no change
    }
};

/// SignalRatioTracker — TX1/TX2 signal ratio ring buffer for tilt anomaly detection.
///
/// Mirrors TSACore GetTX1TX2SignalRatio + BufTX1TX2SignalRatio.
class SignalRatioTracker {
public:
    inline void Push(int16_t tx1Signal, int16_t tx2Signal) {
        const int tx1 = static_cast<int>(std::max(tx1Signal, static_cast<int16_t>(0)));
        const int tx2 = static_cast<int>(std::max(tx2Signal, static_cast<int16_t>(0)));

        uint16_t ratio;
        if (tx1 > 0 && tx2 < tx1 * 5) {
            ratio = static_cast<uint16_t>(tx2 * 100 / tx1);
        } else {
            ratio = 500;
        }

        m_count = std::min(m_count + 1, kBufLen);
        for (int i = kBufLen - 1; i > 0; --i)
            m_buf[static_cast<size_t>(i)] = m_buf[static_cast<size_t>(i - 1)];
        m_buf[0] = ratio;

        int sum = 0;
        for (int i = 0; i < m_count; ++i)
            sum += m_buf[static_cast<size_t>(i)];
        m_avgRatio = static_cast<uint16_t>(sum / std::max(1, m_count));
    }

    uint16_t GetAvgRatio() const { return m_avgRatio; }

private:
    static constexpr int kBufLen = 3;
    std::array<uint16_t, kBufLen> m_buf{};
    int m_count = 0;
    uint16_t m_avgRatio = 100;
};

} // namespace Asa
