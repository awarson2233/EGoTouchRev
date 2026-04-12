#pragma once
// ── TouchPipeline Module: CMFProcessor ──
// Header-only. Converted from Preprocessing/CMFProcessor.{h,cpp}.
// Common Mode Filter: removes global row/column shift noise.

#include "SolverTypes.h"
#include "NeonCompat.h"
#include <algorithm>
#include <cstdint>

namespace Solvers { namespace Touch {

class CMFProcessor {
public:
    enum class DimensionMode {
        None = 0, RowWise = 1, ColumnWise = 2, DualDim = 3
    };

    bool m_enabled = true;
    DimensionMode m_mode = DimensionMode::RowWise;
    int m_exclusionThreshold = 250;
    int m_maxCorrection = 500;

    inline bool Process(HeatmapFrame& frame) {
        if (!m_enabled || m_mode == DimensionMode::None) return true;
        if (m_mode == DimensionMode::RowWise || m_mode == DimensionMode::DualDim)
            ProcessRowWise(frame);
        if (m_mode == DimensionMode::ColumnWise || m_mode == DimensionMode::DualDim)
            ProcessColumnWise(frame);
        return true;
    }

private:
    inline void ProcessRowWise(HeatmapFrame& frame) {
        const int16_t exclusionThreshold = static_cast<int16_t>(
            std::clamp(m_exclusionThreshold, 0, 0x7FFF));
        const int16_t negativeThreshold = static_cast<int16_t>(-exclusionThreshold);
        const int16_t maxCorrection = static_cast<int16_t>(
            std::clamp(m_maxCorrection, 0, 0x7FFF));

        for (int y = 0; y < 40; ++y) {
            int16_t* row = frame.heatmapMatrix[y];
            int64_t rowSum = 0;
            int validCount = 0;
#if defined(_M_ARM64)
            const int16x8_t positiveThresholdVec = vdupq_n_s16(exclusionThreshold);
            const int16x8_t negativeThresholdVec = vdupq_n_s16(negativeThreshold);
            const int16x8_t zero16 = vdupq_n_s16(0);
            int32x4_t sumLoAcc = vdupq_n_s32(0);
            int32x4_t sumHiAcc = vdupq_n_s32(0);
            uint32x4_t countLoAcc = vdupq_n_u32(0);
            uint32x4_t countHiAcc = vdupq_n_u32(0);
            int x = 0;

            for (; x + 8 <= 60; x += 8) {
                const int16x8_t values = vld1q_s16(row + x);
                const uint16x8_t validMask = vandq_u16(
                    vcltq_s16(values, positiveThresholdVec),
                    vcgtq_s16(values, negativeThresholdVec));
                const int16x8_t maskedValues = vbslq_s16(validMask, values, zero16);
                const uint16x8_t maskBits = vshrq_n_u16(validMask, 15);

                sumLoAcc = vaddq_s32(sumLoAcc, vmovl_s16(vget_low_s16(maskedValues)));
                sumHiAcc = vaddq_s32(sumHiAcc, vmovl_s16(vget_high_s16(maskedValues)));
                countLoAcc = vaddq_u32(countLoAcc, vmovl_u16(vget_low_u16(maskBits)));
                countHiAcc = vaddq_u32(countHiAcc, vmovl_u16(vget_high_u16(maskBits)));
            }

            rowSum += HorizontalAddS32(sumLoAcc) + HorizontalAddS32(sumHiAcc);
            validCount += static_cast<int>(
                HorizontalAddU32(countLoAcc) + HorizontalAddU32(countHiAcc));
            for (; x < 60; ++x) {
                const int16_t val = row[x];
                if (val < exclusionThreshold && val > negativeThreshold) {
                    rowSum += val;
                    validCount++;
                }
            }
#else
            for (int x = 0; x < 60; ++x) {
                const int16_t val = row[x];
                if (val < exclusionThreshold && val > negativeThreshold) {
                    rowSum += val;
                    validCount++;
                }
            }
#endif
            if (validCount > 0) {
                int16_t rowOffset = static_cast<int16_t>(rowSum / validCount);
                rowOffset = std::clamp<int16_t>(rowOffset,
                    static_cast<int16_t>(-maxCorrection),
                    maxCorrection);
                if (rowOffset == 0) continue;

#if defined(_M_ARM64)
                const int16x8_t rowOffsetVec = vdupq_n_s16(rowOffset);
                int x = 0;
                for (; x + 8 <= 60; x += 8) {
                    const int16x8_t values = vld1q_s16(row + x);
                    vst1q_s16(row + x, vsubq_s16(values, rowOffsetVec));
                }
                for (; x < 60; ++x) {
                    row[x] = static_cast<int16_t>(row[x] - rowOffset);
                }
#else
                for (int x = 0; x < 60; ++x)
                    row[x] = static_cast<int16_t>(row[x] - rowOffset);
#endif
            }
        }
    }

    inline void ProcessColumnWise(HeatmapFrame& frame) {
        const int16_t exclusionThreshold = static_cast<int16_t>(
            std::clamp(m_exclusionThreshold, 0, 0x7FFF));
        const int16_t negativeThreshold = static_cast<int16_t>(-exclusionThreshold);
        const int16_t maxCorrection = static_cast<int16_t>(
            std::clamp(m_maxCorrection, 0, 0x7FFF));
        for (int x = 0; x < 60; ++x) {
            int64_t colSum = 0;
            int validCount = 0;
            for (int y = 0; y < 40; ++y) {
                int16_t val = frame.heatmapMatrix[y][x];
                if (val < exclusionThreshold && val > negativeThreshold) {
                    colSum += val;
                    validCount++;
                }
            }
            if (validCount > 0) {
                int16_t colOffset = static_cast<int16_t>(colSum / validCount);
                colOffset = std::clamp<int16_t>(colOffset,
                    static_cast<int16_t>(-maxCorrection),
                    maxCorrection);
                if (colOffset == 0) continue;
                for (int y = 0; y < 40; ++y)
                    frame.heatmapMatrix[y][x] =
                        static_cast<int16_t>(frame.heatmapMatrix[y][x] - colOffset);
            }
        }
    }

#if defined(_M_ARM64)
    static inline int64_t HorizontalAddS32(int32x4_t values) {
        return static_cast<int64_t>(vgetq_lane_s32(values, 0)) +
               static_cast<int64_t>(vgetq_lane_s32(values, 1)) +
               static_cast<int64_t>(vgetq_lane_s32(values, 2)) +
               static_cast<int64_t>(vgetq_lane_s32(values, 3));
    }

    static inline uint32_t HorizontalAddU32(uint32x4_t values) {
        return vgetq_lane_u32(values, 0) +
               vgetq_lane_u32(values, 1) +
               vgetq_lane_u32(values, 2) +
               vgetq_lane_u32(values, 3);
    }
#endif
};

}} // namespace Solvers::Touch
