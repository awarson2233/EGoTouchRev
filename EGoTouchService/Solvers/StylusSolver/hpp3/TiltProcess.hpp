#pragma once

#include "SolverTypes.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace Solvers::Stylus {

class TiltProcess {
public:
    bool m_enabled = true;

    inline void Reset() {
        m_signalRatioBuf.fill(0);
        m_signalRatioBufCnt = 0;
        m_coordDifDim1Buf.fill(0);
        m_coordDifDim2Buf.fill(0);
        m_coorDifBufCnt = 0;
        m_tiltDim1Buf.fill(0);
        m_tiltDim2Buf.fill(0);
        m_lastCoordDiffDim1 = 0;
        m_lastCoordDiffDim2 = 0;
        m_lastOutput = {};
        m_haveLastOutput = false;
        m_prevPressureActive = false;
    }

    static inline int16_t JitterFilter1Degree(int16_t prev, int16_t cur) {
        if (prev < cur) return static_cast<int16_t>(cur - 1);
        if (cur < prev) return static_cast<int16_t>(cur + 1);
        return cur;
    }

    inline bool Process(HeatmapFrame& frame) {
        auto& stylus = frame.stylus;
        auto& runtime = stylus.runtime;
        auto& tilt = runtime.tilt;
        auto& tx1 = runtime.tx1;
        auto& tx2 = runtime.tx2;

        runtime.flow.pipelineStage = 4;
        if (!m_enabled) {
            tilt = {};
            tx2.coordinate = {};
            PublishTiltToPost(runtime.post.point, tilt);
            return true;
        }

        if (!tx1.coordinate.localGridCoor.valid) {
            tx2.coordinate = {};
            KeepLastFrame(tilt);
            PublishTiltToPost(runtime.post.point, tilt);
            m_prevPressureActive = stylus.input.btSample.hasSample && stylus.input.btSample.pressure[3] != 0;
            return true;
        }

        if (!runtime.rawGrid.asaGrid.tx2.valid || !tx2.feature.refinedLocalCoor.valid) {
            tx2.coordinate = {};
            KeepLastFrame(tilt);
            PublishTiltToPost(runtime.post.point, tilt);
            m_prevPressureActive = stylus.input.btSample.hasSample && stylus.input.btSample.pressure[3] != 0;
            return true;
        }

        if (!m_haveLastOutput || !m_prevPressureActive) {
            ResetHistory();
        }

        tx2.coordinate.localGridCoor = tx2.feature.refinedLocalCoor;
        tx2.coordinate.reportGlobalCoor = tx2.coordinate.localGridCoor;
        LocalToGlobal(tx2.coordinate.reportGlobalCoor,
                      runtime.rawGrid.asaGrid.tx2.anchorRow,
                      runtime.rawGrid.asaGrid.tx2.anchorCol,
                      kAnchorCenterOffset);
        TiltProcess::ClampToSensorBounds(tx2.coordinate.reportGlobalCoor);

        Asa::AsaCoorResult tx1Global = tx1.coordinate.localGridCoor;
        LocalToGlobal(tx1Global,
                      runtime.rawGrid.asaGrid.tx1.anchorRow,
                      runtime.rawGrid.asaGrid.tx1.anchorCol,
                      kAnchorCenterOffset);
        TiltProcess::ClampToSensorBounds(tx1Global);

        const uint16_t signalRatio = GetSignalRatio(runtime.signal.signalX, runtime.signal.signalY);
        PushSignalRatio(signalRatio);
        const uint16_t signalRatioAvg = static_cast<uint16_t>(GetSignalRatioAverage(3));
        const uint16_t lenLimit = GetTX1TX2LenLimit(signalRatioAvg);

        const int32_t rawDiffDim1 = static_cast<int32_t>(tx1Global.dim1) -
                                    static_cast<int32_t>(tx2.coordinate.reportGlobalCoor.dim1);
        const int32_t rawDiffDim2 = static_cast<int32_t>(tx1Global.dim2) -
                                    static_cast<int32_t>(tx2.coordinate.reportGlobalCoor.dim2);
#if EGOTOUCH_DIAG
        tilt.rawDiffDim1 = rawDiffDim1;
        tilt.rawDiffDim2 = rawDiffDim2;
#endif

        int32_t diffDim1 = rawDiffDim1;
        int32_t diffDim2 = rawDiffDim2;
        bool anomalyDamped = false;

        if (m_coorDifBufCnt == 0) {
            if (diffDim1 > static_cast<int32_t>(lenLimit)) {
                diffDim1 = static_cast<int32_t>(lenLimit);
                anomalyDamped = true;
            } else if (diffDim1 < -static_cast<int32_t>(lenLimit)) {
                diffDim1 = -static_cast<int32_t>(lenLimit);
                anomalyDamped = true;
            }
            if (diffDim2 > static_cast<int32_t>(lenLimit)) {
                diffDim2 = static_cast<int32_t>(lenLimit);
                anomalyDamped = true;
            } else if (diffDim2 < -static_cast<int32_t>(lenLimit)) {
                diffDim2 = -static_cast<int32_t>(lenLimit);
                anomalyDamped = true;
            }
        } else {
            if (rawDiffDim1 > static_cast<int32_t>(lenLimit) || rawDiffDim1 < -static_cast<int32_t>(lenLimit) ||
                rawDiffDim2 > static_cast<int32_t>(lenLimit) || rawDiffDim2 < -static_cast<int32_t>(lenLimit)) {
                diffDim1 = (rawDiffDim1 + m_lastCoordDiffDim1 * 7) / 8;
                diffDim2 = (rawDiffDim2 + m_lastCoordDiffDim2 * 7) / 8;
                anomalyDamped = true;
            }
        }

        PushCoordinateDiff(diffDim1, diffDim2);
        diffDim1 = static_cast<int32_t>(GetCoordinateDiffAverage(5, 0));
        diffDim2 = static_cast<int32_t>(GetCoordinateDiffAverage(5, 1));
        m_lastCoordDiffDim1 = diffDim1;
        m_lastCoordDiffDim2 = diffDim2;

        int16_t preTiltDim1 = GetTiltByCoorDif(diffDim1, 0);
        int16_t preTiltDim2 = GetTiltByCoorDif(diffDim2, 1);

        if (diffDim1 > static_cast<int32_t>(lenLimit) || diffDim1 < -static_cast<int32_t>(lenLimit) ||
            diffDim2 > static_cast<int32_t>(lenLimit) || diffDim2 < -static_cast<int32_t>(lenLimit)) {
            diffDim1 = m_coordDifDim1Buf[0];
            diffDim2 = m_coordDifDim2Buf[0];
            preTiltDim1 = GetTiltByCoorDif(diffDim1, 0);
            preTiltDim2 = GetTiltByCoorDif(diffDim2, 1);
            anomalyDamped = true;
        }

        // Circular clamp: scale the 2D diff vector so its Euclidean magnitude does not exceed lenLimit.
        // Per-axis clamping alone allows √2× overshoot on the diagonal; this prevents it.
        {
            const int32_t sqMag = diffDim1 * diffDim1 + diffDim2 * diffDim2;
            int32_t magnitude = IntSqrtU32(static_cast<uint32_t>(sqMag));
            if (magnitude == 0) magnitude = 1;
            if (static_cast<int32_t>(lenLimit) < magnitude) {
                diffDim1 = (static_cast<int32_t>(lenLimit) * diffDim1) / magnitude;
                diffDim2 = (static_cast<int32_t>(lenLimit) * diffDim2) / magnitude;
                preTiltDim1 = GetTiltByCoorDif(diffDim1, 0);
                preTiltDim2 = GetTiltByCoorDif(diffDim2, 1);
#if EGOTOUCH_DIAG
                tilt.circularClamped = true;
#endif
            }
        }

        tilt.signalRatio = signalRatioAvg;
        tilt.lenLimit = lenLimit;
        tilt.diffDim1 = diffDim1;
        tilt.diffDim2 = diffDim2;
        tilt.preTiltDim1 = preTiltDim1;
        tilt.preTiltDim2 = preTiltDim2;
        tilt.anomalyDamped = anomalyDamped;
        tilt.valid = true;

        PushTilt(preTiltDim1, preTiltDim2);
        if (!m_prevPressureActive || m_lastOutput.reportTiltDim1 == 0) {
            tilt.reportTiltDim1 = preTiltDim1;
            tilt.reportTiltDim2 = preTiltDim2;
        } else {
            tilt.reportTiltDim1 = static_cast<int16_t>(GetTiltAverage(5, 0));
            tilt.reportTiltDim2 = static_cast<int16_t>(GetTiltAverage(5, 1));
        }

        tilt.reportTiltDim1 = JitterFilter1Degree(m_lastOutput.reportTiltDim1, tilt.reportTiltDim1);
        tilt.reportTiltDim2 = JitterFilter1Degree(m_lastOutput.reportTiltDim2, tilt.reportTiltDim2);

        m_lastOutput = tilt;
        m_haveLastOutput = true;
        m_prevPressureActive = stylus.input.btSample.hasSample && stylus.input.btSample.pressure[3] != 0;
        PublishTiltToPost(runtime.post.point, tilt);
        return true;
    }

private:
    static constexpr int kHistorySize = 10;
    static constexpr int kAnchorCenterOffset = Asa::kGridDim / 2;
    static constexpr bool kAxisRotated = false;
    static constexpr int kDim1Length = 60;
    static constexpr int kDim2Length = 40;
    static constexpr int kDim1PitchSize = 0x102C;
    static constexpr int kDim2PitchSize = 0x0A50;
    static constexpr int kTxPitch = 250;
    static constexpr int kRxPitch = 200;
    static constexpr int kStylusTiltParam = 5;
    static constexpr std::array<uint16_t, 4> kRatioThresholds{{20, 90, 155, 200}};
    static constexpr std::array<uint16_t, 4> kRatioScales{{0, 850, 950, 1000}};

    std::array<uint16_t, kHistorySize> m_signalRatioBuf{};
    int m_signalRatioBufCnt = 0;

    std::array<int32_t, kHistorySize> m_coordDifDim1Buf{};
    std::array<int32_t, kHistorySize> m_coordDifDim2Buf{};
    int m_coorDifBufCnt = 0;

    std::array<int16_t, kHistorySize> m_tiltDim1Buf{};
    std::array<int16_t, kHistorySize> m_tiltDim2Buf{};

    int32_t m_lastCoordDiffDim1 = 0;
    int32_t m_lastCoordDiffDim2 = 0;
    StylusRuntimeTilt m_lastOutput{};
    bool m_haveLastOutput = false;
    bool m_prevPressureActive = false;

    static inline void LocalToGlobal(Asa::AsaCoorResult& coor,
                                     int anchorRow,
                                     int anchorCol,
                                     int anchorCenterOffset) {
        if (!coor.valid) return;
        const int32_t centerOff = static_cast<int32_t>(anchorCenterOffset) * Asa::kCoorUnit;
        coor.dim1 += static_cast<int32_t>(anchorCol) * Asa::kCoorUnit - centerOff;
        coor.dim2 += static_cast<int32_t>(anchorRow) * Asa::kCoorUnit - centerOff;
    }

    static inline void ClampToSensorBounds(Asa::AsaCoorResult& coor) {
        if (!coor.valid) return;
        coor.dim1 = std::clamp(coor.dim1, 0, kDim1Length * Asa::kCoorUnit - 1);
        coor.dim2 = std::clamp(coor.dim2, 0, kDim2Length * Asa::kCoorUnit - 1);
    }

    static inline uint16_t GetSignalRatio(uint16_t tx1Signal, uint16_t tx2Signal) {
        if (tx1Signal == 0) return 0;
        const uint32_t ratio = static_cast<uint32_t>(tx2Signal) * 100u / tx1Signal;
        return static_cast<uint16_t>(std::min<uint32_t>(ratio, 500u));
    }

    inline void PushSignalRatio(uint16_t value) {
        ++m_signalRatioBufCnt;
        if (m_signalRatioBufCnt > kHistorySize) m_signalRatioBufCnt = kHistorySize;
        for (int i = kHistorySize - 1; i > 0; --i) {
            m_signalRatioBuf[static_cast<std::size_t>(i)] = m_signalRatioBuf[static_cast<std::size_t>(i - 1)];
        }
        m_signalRatioBuf[0] = value;
    }

    inline uint16_t GetSignalRatioAverage(int count) const {
        const int n = std::min(count <= 0 ? 1 : count, m_signalRatioBufCnt);
        if (n <= 0) return 0;
        uint32_t sum = 0;
        for (int i = 0; i < n; ++i) {
            sum += m_signalRatioBuf[static_cast<std::size_t>(i)];
        }
        return static_cast<uint16_t>(sum / static_cast<uint32_t>(n));
    }

    inline uint16_t GetTX1TX2LenLimit(uint16_t signalRatio) const {
        const int base = kAxisRotated
            ? (kTxPitch * kStylusTiltParam * Asa::kCoorUnit) / kDim2PitchSize
            : (kTxPitch * kStylusTiltParam * Asa::kCoorUnit) / kDim1PitchSize;

        if (signalRatio <= kRatioThresholds[0]) {
            return 0;
        }
        if (signalRatio > kRatioThresholds.back()) {
            return static_cast<uint16_t>(base);
        }

        int scale = kRatioScales.back();
        for (std::size_t i = 0; i + 1 < kRatioThresholds.size(); ++i) {
            const uint16_t lo = kRatioThresholds[i];
            const uint16_t hi = kRatioThresholds[i + 1];
            if (!(lo < signalRatio && signalRatio <= hi)) continue;
            const int loScale = static_cast<int>(kRatioScales[i]);
            const int hiScale = static_cast<int>(kRatioScales[i + 1]);
            const int span = static_cast<int>(hi - lo);
            const int t = static_cast<int>(signalRatio - lo);
            scale = loScale + (t * (hiScale - loScale)) / span;
            break;
        }

        return static_cast<uint16_t>((base * scale) / 1000);
    }

    inline void PushCoordinateDiff(int32_t dim1, int32_t dim2) {
        ++m_coorDifBufCnt;
        if (m_coorDifBufCnt > kHistorySize) m_coorDifBufCnt = kHistorySize;
        for (int i = kHistorySize - 1; i > 0; --i) {
            m_coordDifDim1Buf[static_cast<std::size_t>(i)] = m_coordDifDim1Buf[static_cast<std::size_t>(i - 1)];
            m_coordDifDim2Buf[static_cast<std::size_t>(i)] = m_coordDifDim2Buf[static_cast<std::size_t>(i - 1)];
        }
        m_coordDifDim1Buf[0] = dim1;
        m_coordDifDim2Buf[0] = dim2;
    }

    inline int32_t GetCoordinateDiffAverage(int count, int axis) const {
        const int n = std::min(count <= 0 ? 1 : count, m_coorDifBufCnt);
        if (n <= 0) return 0;
        int64_t sum = 0;
        for (int i = 0; i < n; ++i) {
            sum += (axis == 0) ? m_coordDifDim1Buf[static_cast<std::size_t>(i)]
                               : m_coordDifDim2Buf[static_cast<std::size_t>(i)];
        }
        return static_cast<int32_t>(sum / n);
    }

    inline int16_t GetTiltByCoorDif(int32_t diff, int axis) const {
        const int len = GetTiltAxisLength(axis);
        if (-len < diff && diff < len) {
            const double angle = std::asin(static_cast<double>(diff) / static_cast<double>(len));
            return static_cast<int16_t>(static_cast<int>((angle * 180.0) / 3.141592657));
        }
        if (diff < len) {
            return static_cast<int16_t>(-90);
        }
        return static_cast<int16_t>(90);
    }

    inline int GetTiltAxisLength(int axis) const {
        if (!kAxisRotated) {
            if (axis == 0) {
                return (kTxPitch * kStylusTiltParam * Asa::kCoorUnit) / kDim1PitchSize;
            }
            return (kRxPitch * kStylusTiltParam * Asa::kCoorUnit) / kDim2PitchSize;
        }
        if (axis == 0) {
            return (kTxPitch * kStylusTiltParam * Asa::kCoorUnit) / kDim2PitchSize;
        }
        return (kRxPitch * kStylusTiltParam * Asa::kCoorUnit) / kDim1PitchSize;
    }

    static inline int32_t IntSqrtU32(uint32_t x) {
        int32_t result = 0;
        int32_t bit = 0x8000;
        while (x < static_cast<uint32_t>(bit)) {
            bit >>= 1;
        }
        for (; bit != 0; bit >>= 1) {
            result += bit;
            if (x < static_cast<uint32_t>(result * result)) {
                result -= bit;
            }
        }
        return result;
    }

    inline void PushTilt(int16_t dim1, int16_t dim2) {
        for (int i = kHistorySize - 1; i > 0; --i) {
            m_tiltDim1Buf[static_cast<std::size_t>(i)] = m_tiltDim1Buf[static_cast<std::size_t>(i - 1)];
            m_tiltDim2Buf[static_cast<std::size_t>(i)] = m_tiltDim2Buf[static_cast<std::size_t>(i - 1)];
        }
        m_tiltDim1Buf[0] = dim1;
        m_tiltDim2Buf[0] = dim2;
    }

    inline int16_t GetTiltAverage(int count, int axis) const {
        const int n = std::min(count <= 0 ? 1 : count, m_coorDifBufCnt);
        if (n <= 0) return 0;
        int32_t sum = 0;
        for (int i = 0; i < n; ++i) {
            sum += (axis == 0) ? m_tiltDim1Buf[static_cast<std::size_t>(i)]
                               : m_tiltDim2Buf[static_cast<std::size_t>(i)];
        }
        return static_cast<int16_t>(sum / n);
    }

    inline void ResetHistory() {
        m_signalRatioBuf.fill(0);
        m_signalRatioBufCnt = 0;
        m_coordDifDim1Buf.fill(0);
        m_coordDifDim2Buf.fill(0);
        m_coorDifBufCnt = 0;
        m_tiltDim1Buf.fill(0);
        m_tiltDim2Buf.fill(0);
        m_lastCoordDiffDim1 = 0;
        m_lastCoordDiffDim2 = 0;
    }

    inline void PublishTiltToPost(StylusSolvePoint& point, const StylusRuntimeTilt& tilt) const {
        point.tiltValid = tilt.valid;
        point.preTiltX = tilt.preTiltDim1;
        point.preTiltY = tilt.preTiltDim2;
        point.tiltX = tilt.reportTiltDim1;
        point.tiltY = tilt.reportTiltDim2;
        if (tilt.valid) {
            const float tx = static_cast<float>(tilt.reportTiltDim1);
            const float ty = static_cast<float>(tilt.reportTiltDim2);
            point.tiltMagnitude = std::sqrt(tx * tx + ty * ty);
            point.tiltAzimuthDeg = std::atan2(ty, tx) * 57.2957795f;
            if (point.tiltAzimuthDeg < 0.0f) {
                point.tiltAzimuthDeg += 360.0f;
            }
        } else {
            point.tiltMagnitude = 0.0f;
            point.tiltAzimuthDeg = 0.0f;
        }
    }

    inline void KeepLastFrame(StylusRuntimeTilt& out) const {
        if (!m_haveLastOutput) {
            out = {};
            return;
        }
        out = m_lastOutput;
    }
};

} // namespace Solvers::Stylus
