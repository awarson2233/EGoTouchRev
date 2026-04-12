#pragma once
#include "AsaTypes.hpp"
#include <algorithm>

namespace Asa {

/// Pitch-periodic polynomial compensation coefficients (from factory calibration).
/// Mirrors TSACore CoorMultiOrderFitCompensate parameters stored in Flash.
struct PitchCompensation {
    double c[4] = {0, 0, 0, 0};  // c0 + c1*x + c2*x² + c3*x³
    bool   enabled = false;
};

/// TriangleEdgeParams — Factory-style edge compensation parameters for one axis.
///
/// ratio:                virtual outer-neighbor synthesis ratio
/// sumThresholdIdxLast:  threshold used when the peak is on the local valid end
/// sumThresholdIdx0:     threshold used when the peak is on the local valid begin
struct TriangleEdgeParams {
    int ratio = 50;
    int sumThresholdIdxLast = 5000;
    int sumThresholdIdx0 = 5000;
};

/// CoordinateSolver — Triangle/Gravity interpolation on 1D projections
/// Mirrors TX1CoordinateProcess + GetCoordinateByTriangleOf
class CoordinateSolver {
public:
    /// Solve coordinates from 1D projection signals
    /// @param proj  1D projections from GridPeakDetector::ProjectTo1D
    /// @param anchorRow  Window center anchor on the physical row axis
    /// @param anchorCol  Window center anchor on the physical col axis
    /// @param sensorRows Total physical sensor rows
    /// @param sensorCols Total physical sensor cols
    /// @param centerIndex Local window center index (defaults to 4 for 9x9)
    /// @return Coordinate in 0x400 units
    inline AsaCoorResult Solve(const AsaProjection& proj,
                               int anchorRow,
                               int anchorCol,
                               int sensorRows,
                               int sensorCols,
                               int centerIndex = kGridDim / 2) {
        AsaCoorResult result{};

        int32_t coorDim1, coorDim2;
        if (useTriangle) {
            coorDim1 = SolveByTriangle(
                proj.dim1, proj.peakIdxDim1, kGridDim,
                triEdgeDim1, anchorCol, sensorCols, centerIndex);
            coorDim2 = SolveByTriangle(
                proj.dim2, proj.peakIdxDim2, kGridDim,
                triEdgeDim2, anchorRow, sensorRows, centerIndex);
        } else {
            coorDim1 = SolveByGravity(proj.dim1, kGridDim, proj.peakIdxDim1);
            coorDim2 = SolveByGravity(proj.dim2, kGridDim, proj.peakIdxDim2);
        }

        // Invalid check
        if (coorDim1 == 0x7FFFFFFF || coorDim2 == 0x7FFFFFFF)
            return result;

        // ── P0: Apply pitch-periodic polynomial compensation ──
        coorDim1 = ApplyPitchCompensation(coorDim1, pitchCompDim1);
        coorDim2 = ApplyPitchCompensation(coorDim2, pitchCompDim2);

        // Clamp to the local 9x9 window range [0, 9*0x400 - 1]
        const int32_t maxDim1 = kGridDim * kCoorUnit - 1;
        const int32_t maxDim2 = kGridDim * kCoorUnit - 1;
        result.dim1 = std::clamp(coorDim1, 0, maxDim1);
        result.dim2 = std::clamp(coorDim2, 0, maxDim2);
        result.valid = true;
        return result;
    }

    // Configuration: algorithm selection
    // DAT_1820d630 bit0: 0=Triangle, 1=Gravity.
    // Gaokun flash: DAT_1820d630 = 0x0E → bit0=0 → Triangle.
    bool useTriangle = true;  // true → triangle (Gaokun default)

    // DAT_1820d630 bit3: edge compensation secondary logic enable
    // Gaokun flash: 0x0E & 8 = 8 → enabled
    bool triEdgeSecondaryBlend = true;

    // Triangle interpolation edge parameters (factory defaults from TSACore flash)
    TriangleEdgeParams triEdgeDim1 = {50, 5000, 5000};
    TriangleEdgeParams triEdgeDim2 = {50, 4500, 3700};

    // ── P0: Pitch-periodic polynomial compensation ──
    // Mirrors CoorMultiOrderFitCompensate from TSACore.
    // Grid-mode coefficients from Gaokun flash: DAT_1820d6C8 / DAT_1820d708
    PitchCompensation pitchCompDim1 = {
        {0.0, -1.710915149066293, 0.005959771652221362, -5.113555667385272e-6},
        true  // enabled
    };
    PitchCompensation pitchCompDim2 = {
        {0.0, -1.710915149066293, 0.005959771652221362, -5.113555667385272e-6},
        true  // enabled — TSACore TX1CoordinateProcess calls unconditionally
    };

    // ── P0: Gravity noise floor ──
    // Signals below this threshold are excluded from centroid calculation.
    // Mirrors the noiseFloor parameter passed to UpdateTX1GravityData.
    int32_t gravityNoiseFloor = 0;

    // ── P0: Gravity fictitious edge enable ──
    // When true, mirror-extend signals at grid boundaries to prevent
    // edge-biased centroid shift (mirrors GetFictiousEdge).
    bool gravityFictitiousEdge = true;

private:
    struct ValidRange {
        int begin = 0;
        int end = -1;
        int count = 0;
    };

    /// Triangle interpolation using 3 points (mid-grid)
    /// Params are int to match TSACore calling convention
    static inline int32_t TriangleAlgUsing3Point(int left, int center, int right) {
        if (right < left) {
            // Peak biased right
            int minVal = right;
            if (center <= right) minVal = center - 1;  // safety clamp
            int den = center - minVal;
            int offset = ((left - minVal) * kCoorUnit) / den;
            offset = offset / 2; // C/C++ integer division truncates towards zero
            return (kCoorUnit / 2) - offset; // 0x200 - offset
        } else {
            // Peak biased left or perfectly centered
            int minVal = left;
            if (center <= left) minVal = center - 1;  // safety clamp
            int den = center - minVal;
            int offset = ((right - minVal) * kCoorUnit) / den;
            offset = offset / 2; // C/C++ integer division truncates towards zero
            return offset + (kCoorUnit / 2); // offset + 0x200
        }
    }

    /// Edge compensation: creates virtual neighbor for edge interpolation
    /// Mirrors TSACore EdgeCompensating exactly
    inline int32_t EdgeCompensating(int peak, int n1, int n2,
                                    int param1, int param5) {
        int edgeParam = (param1 == 0) ? 1 : param1;

        int virtualNeighbor = ((peak - n1) * 10) / edgeParam;
        int comp2 = peak - ((n1 - n2) * edgeParam) / 10;

        if (virtualNeighbor < comp2) {
            virtualNeighbor = comp2;
            if (triEdgeSecondaryBlend) {  // DAT_1820d630 & 8 secondary logic
                int threshold = comp2;
                int sum = peak + n1 + comp2;
                if (sum < param5) {
                    threshold = param5 - peak - n1;
                }
                if (comp2 < threshold) {
                    virtualNeighbor = (comp2 + threshold) / 2;
                }
            }
        }

        if (peak <= virtualNeighbor) {
            virtualNeighbor = peak - 1;
        }

        return virtualNeighbor;
    }

    /// Triangle interpolation at grid edge
    inline int32_t TriangleAlgEdge(int peak, int n1, int n2,
                                   int param1, int param2) {
        int virtualNeighbor = EdgeCompensating(peak, n1, n2, param1, param2);
        int result = TriangleAlgUsing3Point(virtualNeighbor, peak, n1);

        // Sum threshold check using integer division simulating magic multiply
        if (peak + n1 + n2 < (param2 * 2) / 5) {
            result = 0;
        }
        return result;
    }

    static inline ValidRange ComputeValidRange(
            int anchor, int sensorDim, int len, int centerIndex) {
        ValidRange range{};
        if (len <= 0 || sensorDim <= 0) {
            return range;
        }

        range.begin = std::max(0, centerIndex - anchor);
        range.end = std::min(len - 1, centerIndex + (sensorDim - 1 - anchor));
        if (range.end >= range.begin) {
            range.count = range.end - range.begin + 1;
        }
        return range;
    }

    /// Solve one dimension using triangle interpolation
    inline int32_t SolveByTriangle(
            const int32_t* signal, int peakIdx, int len,
            const TriangleEdgeParams& edgeParam,
            int anchor, int sensorDim, int centerIndex) {
        if (peakIdx < 0 || peakIdx >= len) return 0x7FFFFFFF;

        // TSACore uses MOVZX word ptr [RAX], meaning signals are treated as uint16
        const auto s = [&](int i) -> int {
            return static_cast<int>(std::clamp(signal[i], 0, 65535));
        };

        const ValidRange validRange = ComputeValidRange(anchor, sensorDim, len, centerIndex);
        // Physical edge repair only applies when the 9x9 window is actually clipped
        // by the sensor boundary and the peak lands on that clipped-side valid edge.
        const bool hasInvalidLowSide = validRange.begin > 0;
        const bool hasInvalidHighSide = validRange.end >= 0 && validRange.end < (len - 1);
        if (validRange.count >= 3) {
            if (hasInvalidLowSide && peakIdx == validRange.begin) {
                const int peak = s(validRange.begin);
                const int n1 = s(validRange.begin + 1);
                const int n2 = s(validRange.begin + 2);
                return validRange.begin * kCoorUnit +
                       TriangleAlgEdge(peak, n1, n2,
                                       edgeParam.ratio,
                                       edgeParam.sumThresholdIdx0);
            }
            if (hasInvalidHighSide && peakIdx == validRange.end) {
                const int peak = s(validRange.end);
                const int n1 = s(validRange.end - 1);
                const int n2 = s(validRange.end - 2);
                const int edge = TriangleAlgEdge(peak, n1, n2,
                                                 edgeParam.ratio,
                                                 edgeParam.sumThresholdIdxLast);
                return (validRange.end + 1) * kCoorUnit - edge;
            }
        }

        if (peakIdx == 0) {
            return TriangleAlgEdge(s(0), s(1), s(2),
                                   edgeParam.ratio,
                                   edgeParam.sumThresholdIdx0);
        } else if (peakIdx == len - 1) {
            int edge = TriangleAlgEdge(s(len - 1), s(len - 2), s(len - 3),
                                       edgeParam.ratio,
                                       edgeParam.sumThresholdIdxLast);
            return len * kCoorUnit - edge;
        } else {
            // Middle: standard 3-point
            int offset = TriangleAlgUsing3Point(s(peakIdx - 1), s(peakIdx), s(peakIdx + 1));
            return peakIdx * kCoorUnit + offset;
        }
    }

    /// Gravity (centroid) interpolation on 1D signal
    inline int32_t SolveByGravity(const int32_t* signal, int len, int peakIdx) {
        if (peakIdx < 0 || peakIdx >= len) return 0x7FFFFFFF;

        // ── Build narrow-window buffer with baseline subtraction ──
        constexpr int kMaxBuf = 5;
        int32_t buf[kMaxBuf] = {};
        int bufLen = 0;
        int startIdx = peakIdx - 1;

        auto clampSig = [&](int i) -> int32_t {
            if (i < 0 || i >= len) return 0;
            return std::max(0, signal[i] - gravityNoiseFloor);
        };

        if (peakIdx == 0) {
            // ── Left edge case ──
            startIdx = -1;
            const int endIdx = std::min(peakIdx + 2, len - 1);
            const int32_t baseline = clampSig(endIdx);

            if (gravityFictitiousEdge && len >= 2) {
                int32_t mirror = clampSig(1);
                buf[bufLen++] = std::max(0, mirror - baseline);
            } else {
                startIdx = 0;
            }
            for (int i = 0; i <= endIdx; ++i) {
                buf[bufLen++] = std::max(0, clampSig(i) - baseline);
            }
        } else if (peakIdx == len - 1) {
            // ── Right edge case ──
            startIdx = std::max(0, peakIdx - 2);
            const int32_t baseline = clampSig(startIdx);

            for (int i = startIdx; i < len; ++i) {
                buf[bufLen++] = std::max(0, clampSig(i) - baseline);
            }
            if (gravityFictitiousEdge && len >= 2) {
                int32_t mirror = clampSig(len - 2);
                buf[bufLen++] = std::max(0, mirror - baseline);
            }
        } else {
            // ── Normal case: window [peak-1, peak, peak+1] ──
            const int32_t left  = clampSig(peakIdx - 1);
            const int32_t peak  = clampSig(peakIdx);
            const int32_t right = clampSig(peakIdx + 1);
            const int32_t baseline = std::min(left, right);

            buf[0] = std::max(0, left  - baseline);
            buf[1] = std::max(0, peak  - baseline);
            buf[2] = std::max(0, right - baseline);
            bufLen = 3;
        }

        // ── TSACore Gravity formula ──
        int64_t weightedSum = 0;
        int64_t totalWeight = 0;
        for (int i = 0; i < bufLen; ++i) {
            weightedSum += static_cast<int64_t>(i) * buf[i];
            totalWeight += buf[i];
        }

        if (totalWeight <= 0) return 0x7FFFFFFF;

        const int32_t gravity = static_cast<int32_t>(
            (weightedSum * kCoorUnit) / totalWeight) + (kCoorUnit / 2);

        int32_t result = startIdx * kCoorUnit + gravity;
        return std::clamp(result, 0, (len - 1) * kCoorUnit);
    }

    /// Apply pitch-periodic polynomial compensation to one coordinate
    static inline int32_t ApplyPitchCompensation(int32_t coor,
                                                  const PitchCompensation& comp) {
        if (!comp.enabled) return coor;

        const int remainder = ((coor % kCoorUnit) + kCoorUnit) % kCoorUnit;

        // Fold to half-pitch (exploit symmetry around pitch center 0x200)
        int x;
        if (remainder < 0x201) {
            x = 0x200 - remainder;  // left half: mirror
        } else {
            x = remainder - 0x200;  // right half: direct
        }

        // Cubic polynomial: c0 + c1*x + c2*x² + c3*x³
        const double dx = static_cast<double>(x);
        int compensation = static_cast<int>(
            comp.c[0] +
            comp.c[1] * dx +
            comp.c[2] * dx * dx +
            comp.c[3] * dx * dx * dx
        );

        // Right half: negate (symmetric compensation)
        if (remainder >= 0x201) {
            compensation = -compensation;
        }

        return coor + compensation;
    }
};

} // namespace Asa
