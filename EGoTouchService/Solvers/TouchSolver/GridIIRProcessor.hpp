#pragma once
// ── TouchPipeline Module: GridIIRProcessor ──
// Header-only. Converted from Preprocessing/GridIIRProcessor.{h,cpp}.
// Dynamic threshold gated IIR with aggressive noise floor decay.

#include "SolverTypes.h"
#include "NeonCompat.h"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <cstdint>

namespace Solvers { namespace Touch {

class GridIIRProcessor {
public:
    static constexpr int kGridCellCount = 40 * 60;

    bool m_enabled = true;

    // Dynamic Touch Gate
    float m_gateRatio = 0.10f;
    int m_gateStaticFloor = 200;

    // Low-signal decay IIR
    int m_decayWeight = 200;
    int m_decayStep = 80;
    int m_noiseFloorCutoff = 5;

    // Residual correction (temporal)
    bool  m_residualEnabled = false;
    float m_residualAlpha   = 0.3f;

    GridIIRProcessor() {
        std::memset(m_historyBuffer, 0, sizeof(m_historyBuffer));
    }

    inline bool Process(HeatmapFrame& frame) {
        if (!m_enabled) return true;

        if (!m_historyInitialized) {
            std::memcpy(m_historyBuffer, frame.heatmapMatrix,
                        sizeof(m_historyBuffer));
            m_historyInitialized = true;
            return true;
        }

        // Compute per-frame dynamic threshold (merged scan: find max while iterating)
        // We still need a two-pass approach because dynThreshold depends on frameMax
        // which requires the full frame. Optimize by using pointer arithmetic instead of 2D indexing.
        const int16_t* matPtr = &frame.heatmapMatrix[0][0];
        int16_t frameMax = 0;
#if defined(_M_ARM64)
        int maxIdx = 0;
        int16x8_t maxVec = vdupq_n_s16(0);
        for (; maxIdx + 8 <= kGridCellCount; maxIdx += 8) {
            maxVec = vmaxq_s16(maxVec, vld1q_s16(matPtr + maxIdx));
        }
        frameMax = HorizontalMax(maxVec);
        for (; maxIdx < kGridCellCount; ++maxIdx) {
            if (matPtr[maxIdx] > frameMax) frameMax = matPtr[maxIdx];
        }
#else
        for (int i = 0; i < kGridCellCount; ++i) {
            if (matPtr[i] > frameMax) frameMax = matPtr[i];
        }
#endif

        const int16_t dynThreshold = static_cast<int16_t>(std::max(
            static_cast<int>(std::lround(frameMax * m_gateRatio)),
            m_gateStaticFloor));

        int16_t* framePixel = &frame.heatmapMatrix[0][0];
        int16_t* histPixel  = &m_historyBuffer[0][0];
        const int32_t decayWeight = static_cast<int32_t>(m_decayWeight);
        const int32_t historyWeight = 256 - decayWeight;
        const int32_t decayStep = static_cast<int32_t>(m_decayStep);
        const int32_t noiseFloorCutoff = static_cast<int32_t>(m_noiseFloorCutoff);

        auto applyDecay = [&](int16_t current, int16_t history) -> int16_t {
            if (current >= dynThreshold) {
                return current;
            }

            int32_t val = (decayWeight * static_cast<int32_t>(current) +
                           historyWeight * static_cast<int32_t>(history)) / 256;
            val = std::max<int32_t>(0, val - decayStep);
            if (val < noiseFloorCutoff) {
                return 0;
            }
            return static_cast<int16_t>(val);
        };

        if (!m_residualEnabled) {
#if defined(_M_ARM64)
            const int16x8_t dynThresholdVec = vdupq_n_s16(dynThreshold);
            const int32x4_t zero32 = vdupq_n_s32(0);
            const int32x4_t decayStepVec = vdupq_n_s32(decayStep);
            const int32x4_t noiseFloorVec = vdupq_n_s32(noiseFloorCutoff);
            int i = 0;
            for (; i + 8 <= kGridCellCount; i += 8) {
                const int16x8_t currentVec = vld1q_s16(framePixel + i);
                const int16x8_t historyVec = vld1q_s16(histPixel + i);
                const uint16x8_t keepCurrentMask = vcgeq_s16(currentVec, dynThresholdVec);

                const int32x4_t currentLo = vmovl_s16(vget_low_s16(currentVec));
                const int32x4_t currentHi = vmovl_s16(vget_high_s16(currentVec));
                const int32x4_t historyLo = vmovl_s16(vget_low_s16(historyVec));
                const int32x4_t historyHi = vmovl_s16(vget_high_s16(historyVec));

                int32x4_t lowLo = vmlaq_n_s32(vmulq_n_s32(currentLo, decayWeight),
                                              historyLo, historyWeight);
                int32x4_t lowHi = vmlaq_n_s32(vmulq_n_s32(currentHi, decayWeight),
                                              historyHi, historyWeight);

                lowLo = DivideBy256TruncTowardZero(lowLo);
                lowHi = DivideBy256TruncTowardZero(lowHi);
                lowLo = vmaxq_s32(vsubq_s32(lowLo, decayStepVec), zero32);
                lowHi = vmaxq_s32(vsubq_s32(lowHi, decayStepVec), zero32);
                lowLo = vbslq_s32(vcgeq_s32(lowLo, noiseFloorVec), lowLo, zero32);
                lowHi = vbslq_s32(vcgeq_s32(lowHi, noiseFloorVec), lowHi, zero32);

                const int16x8_t lowVec = vcombine_s16(vqmovn_s32(lowLo),
                                                      vqmovn_s32(lowHi));
                const int16x8_t filteredVec = vbslq_s16(keepCurrentMask, currentVec, lowVec);
                vst1q_s16(framePixel + i, filteredVec);
                vst1q_s16(histPixel + i, filteredVec);
            }
            for (; i < kGridCellCount; ++i) {
                const int16_t current = framePixel[i];
                const int16_t filtered = applyDecay(current, histPixel[i]);
                framePixel[i] = filtered;
                histPixel[i] = filtered;
            }
            return true;
#else
            for (int i = 0; i < kGridCellCount; ++i) {
                const int16_t current = framePixel[i];
                const int16_t filtered = applyDecay(current, histPixel[i]);
                framePixel[i] = filtered;
                histPixel[i] = filtered;
            }
            return true;
#endif
        }

        const float residualAlpha = m_residualAlpha;
        for (int i = 0; i < kGridCellCount; ++i) {
            int16_t current = framePixel[i];
            const int16_t history = histPixel[i];

            if (history > current) {
                const int16_t residual = static_cast<int16_t>(
                    (history - current) * residualAlpha);
                current = std::max<int16_t>(0, current - residual);
            }

            const int16_t filtered = applyDecay(current, history);
            framePixel[i] = filtered;
            histPixel[i] = filtered;
        }
        return true;
    }

private:
    bool m_historyInitialized = false;
    int16_t m_historyBuffer[40][60];

#if defined(_M_ARM64)
    static inline int16_t HorizontalMax(int16x8_t values) {
        alignas(16) int16_t lanes[8];
        vst1q_s16(lanes, values);
        int16_t maxValue = lanes[0];
        for (int i = 1; i < 8; ++i) {
            if (lanes[i] > maxValue) maxValue = lanes[i];
        }
        return maxValue;
    }

    static inline int32x4_t DivideBy256TruncTowardZero(int32x4_t values) {
        const int32x4_t bias = vandq_s32(vshrq_n_s32(values, 31), vdupq_n_s32(255));
        return vshrq_n_s32(vaddq_s32(values, bias), 8);
    }
#endif
};

}} // namespace Solvers::Touch
