#include "CoordinateSolver.h"
#include <algorithm>

namespace Asa {

// ── TriangleAlgUsing3Point ──
// One-sided ratio method matching TSACore TriangleAlgUsing3Piont exactly.
// Passes integers natively, handles rounding-toward-zero division.
int32_t CoordinateSolver::TriangleAlgUsing3Point(int left, int center, int right) {
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

// ── EdgeCompensating ──
// Creates a virtual neighbor for edge interpolation. Matches TSACore logic.
int32_t CoordinateSolver::EdgeCompensating(int peak, int n1, int n2, 
                                           uint16_t param1, uint16_t param5) {
    int edgeParam = (param1 == 0) ? 1 : static_cast<int>(param1);
    
    int virtualNeighbor = ((peak - n1) * 10) / edgeParam;
    int comp2 = peak - ((n1 - n2) * edgeParam) / 10;
    
    if (virtualNeighbor < comp2) {
        virtualNeighbor = comp2;
        if (edgeCompBit3) {  // DAT_1820d630 & 8 secondary logic
            int threshold = comp2;
            int sum = peak + n1 + comp2;
            if (sum < static_cast<int>(param5)) {
                threshold = static_cast<int>(param5) - peak - n1;
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

// ── TriangleAlgEdge ──
// Matches TSACore: calls EdgeCompensating -> TriangleAlgUsing3Point
int32_t CoordinateSolver::TriangleAlgEdge(int peak, int n1, int n2, 
                                          uint16_t param1, uint16_t param2) {
    int virtualNeighbor = EdgeCompensating(peak, n1, n2, param1, param2);
    int result = TriangleAlgUsing3Point(virtualNeighbor, peak, n1);
    
    // Sum threshold check using integer division simulating magic multiply
    if (peak + n1 + n2 < (static_cast<int>(param2) * 2) / 5) {
        result = 0;
    }
    return result;
}

// ── SolveByTriangle: dispatch to edge/mid interpolation ──
int32_t CoordinateSolver::SolveByTriangle(
        const int32_t* signal, int peakIdx, int len,
        const uint16_t* edgeParam) {
    if (peakIdx < 0 || peakIdx >= len) return 0x7FFFFFFF;

    // TSACore uses MOVZX word ptr [RAX], meaning signals are treated as uint16
    const auto s = [&](int i) -> int {
        return static_cast<int>(std::clamp(signal[i], 0, 65535));
    };

    if (peakIdx == 0) {
        // Left edge: param index 2
        return TriangleAlgEdge(s(0), s(1), s(2), edgeParam[0], edgeParam[2]);
    } else if (peakIdx == len - 1) {
        // Right edge: param index 1, inverted
        int edge = TriangleAlgEdge(s(len-1), s(len-2), s(len-3), edgeParam[0], edgeParam[1]);
        return len * kCoorUnit - edge;
    } else {
        // Middle: standard 3-point
        int offset = TriangleAlgUsing3Point(s(peakIdx - 1), s(peakIdx), s(peakIdx + 1));
        return peakIdx * kCoorUnit + offset;
    }
}

// ── SolveByGravity: narrow-window centroid matching TSACore exactly ──
// TSACore flow: UpdateTX1GravityData → Gravity
//   1. Window = [peak-1, peak, peak+1] (3 elements)
//   2. Baseline = min(signal[peak-1], signal[peak+1])
//   3. Subtract baseline from each element (keep ≥ 0)
//   4. Centroid: gravity = (Σ(i*s[i]) * 1024) / Σ(s[i]) + 512
//   5. Result = (peak-1) * 1024 + gravity
int32_t CoordinateSolver::SolveByGravity(
        const int32_t* signal, int len, int peakIdx) {
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
        // TSACore: startIdx=0, endIdx=peak+2, baseline=signal[endIdx]
        // Fictitious edge prepends mirrored signal[1]
        startIdx = -1;  // virtual start (TSACore sets local_18._1_1_ = 0xFF = -1)
        const int endIdx = std::min(peakIdx + 2, len - 1);
        const int32_t baseline = clampSig(endIdx);

        // Virtual left element (fictitious edge = mirror of signal[1])
        if (gravityFictitiousEdge && len >= 2) {
            int32_t mirror = clampSig(1);
            buf[bufLen++] = std::max(0, mirror - baseline);
        } else {
            startIdx = 0;
        }
        // Real elements [0 .. endIdx]
        for (int i = 0; i <= endIdx; ++i) {
            buf[bufLen++] = std::max(0, clampSig(i) - baseline);
        }
    } else if (peakIdx == len - 1) {
        // ── Right edge case ──
        // TSACore: startIdx=peak-2, endIdx=len-1, baseline=signal[startIdx]
        // Fictitious edge appends mirrored signal[len-2]
        startIdx = std::max(0, peakIdx - 2);
        const int32_t baseline = clampSig(startIdx);

        // Real elements [startIdx .. len-1]
        for (int i = startIdx; i < len; ++i) {
            buf[bufLen++] = std::max(0, clampSig(i) - baseline);
        }
        // Virtual right element (fictitious edge = mirror of signal[len-2])
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
    // gravity = (Σ(i * s[i]) << 10) / Σ(s[i]) + 0x200
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

// ── ApplyPitchCompensation ──
// Mirrors TSACore CoorMultiOrderFitCompensate exactly.
// Applies a symmetric cubic polynomial correction within each sensor pitch period.
//
// Physical model: Due to fringe field effects, the sensor response within each
// pitch cycle is not perfectly linear. This applies a factory-calibrated
// polynomial c0 + c1*x + c2*x² + c3*x³ symmetrically around the pitch center.
int32_t CoordinateSolver::ApplyPitchCompensation(
        int32_t coor, const PitchCompensation& comp) {
    if (!comp.enabled) return coor;

    const int remainder = ((coor % kCoorUnit) + kCoorUnit) % kCoorUnit; // handle negative

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

// ── Solve: main entry point ──
AsaCoorResult CoordinateSolver::Solve(
        const AsaProjection& proj,
        int gridDimRow, int gridDimCol) {
    AsaCoorResult result{};

    int32_t coorDim1, coorDim2;
    if (useTriangle) {
        coorDim1 = SolveByTriangle(
            proj.dim1, proj.peakIdxDim1, kGridDim, triParamDim1);
        coorDim2 = SolveByTriangle(
            proj.dim2, proj.peakIdxDim2, kGridDim, triParamDim2);
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

    // Clamp to valid range [0, dim*kCoorUnit - 1]
    const int32_t maxDim1 = gridDimCol * kCoorUnit - 1;
    const int32_t maxDim2 = gridDimRow * kCoorUnit - 1;
    result.dim1 = std::clamp(coorDim1, 0, maxDim1);
    result.dim2 = std::clamp(coorDim2, 0, maxDim2);
    result.valid = true;
    return result;
}

} // namespace Asa
