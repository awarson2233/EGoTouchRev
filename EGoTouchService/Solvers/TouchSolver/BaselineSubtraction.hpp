#pragma once
// ── TouchPipeline Module: BaselineSubtraction ──
// Header-only. Dynamic per-cell baseline tracker with gated EMA updates.

#include "SolverTypes.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

namespace Solvers { namespace Touch {

class BaselineSubtraction {
public:
    static constexpr int kRows = 40;
    static constexpr int kCols = 60;
    static constexpr int kCellCount = kRows * kCols;

    bool m_enabled = true;
    int  m_baseline = 0x7FFE;

    int  m_noiseDeadband = 16;
    int  m_positiveDriftDeadband = 12;
    int  m_negativeDeadband = 12;
    int  m_touchFreezeThreshold = 130;
    int  m_releaseHoldFrames = 6;
    int  m_positiveAlphaShift = 7;
    int  m_negativeAlphaShift = 5;
    int  m_noiseAlphaShift = 6;
    int  m_positiveMaxStep = 4;
    int  m_negativeMaxStep = 12;
    int  m_acquisitionAlphaShift = 3;
    int  m_acquisitionMaxStep = 128;
    bool m_noiseTrackingEnabled = true;

    inline bool Process(HeatmapFrame& frame) {
        int16_t* outPtr = &frame.heatmapMatrix[0][0];

        if (!m_enabled) {
            return true;
        }

        if (!m_initialized) {
            Initialize();
        }

        int positiveFreezeCandidates = 0;
        for (int i = 0; i < kCellCount; ++i) {
            const int baseline = m_baselineQ8[static_cast<size_t>(i)] >> kBaselineFractionBits;
            if (static_cast<int>(RawCell(outPtr[i])) - baseline >= m_touchFreezeThreshold) {
                ++positiveFreezeCandidates;
            }
        }
        const bool broadPositiveShift = positiveFreezeCandidates > (kCellCount / 8);

        for (int i = 0; i < kCellCount; ++i) {
            const int raw = static_cast<int>(RawCell(outPtr[i]));
            const int baseline = m_baselineQ8[static_cast<size_t>(i)] >> kBaselineFractionBits;
            const int delta = raw - baseline;
            const int absDelta = std::abs(delta);

            if (!m_acquired[static_cast<size_t>(i)]) {
                if (absDelta <= m_noiseDeadband) {
                    m_acquired[static_cast<size_t>(i)] = 1;
                } else {
                    UpdateBaseline(i, delta, m_acquisitionAlphaShift, m_acquisitionMaxStep);
                }
            } else if (!broadPositiveShift && delta >= m_touchFreezeThreshold) {
                m_releaseHold[static_cast<size_t>(i)] = static_cast<uint8_t>(
                    std::clamp(m_releaseHoldFrames, 0, 255));
                outPtr[i] = SaturateInt16(delta);
                continue;
            } else if (m_releaseHold[static_cast<size_t>(i)] > 0) {
                --m_releaseHold[static_cast<size_t>(i)];
                if (delta < -m_negativeDeadband) {
                    outPtr[i] = SaturateInt16(delta);
                    continue;
                }
            } else if (absDelta <= m_noiseDeadband) {
                if (delta != 0) {
                    UpdateBaseline(i, delta, m_noiseAlphaShift, 1);
                }
            } else if (delta > m_positiveDriftDeadband) {
                UpdateBaseline(i, delta, m_positiveAlphaShift, m_positiveMaxStep);
            } else if (delta < -m_negativeDeadband) {
                UpdateBaseline(i, delta, m_negativeAlphaShift, m_negativeMaxStep);
            } else if (m_noiseTrackingEnabled) {
                UpdateBaseline(i, delta, m_noiseAlphaShift, 1);
            }

            const int adjustedBaseline = m_baselineQ8[static_cast<size_t>(i)] >> kBaselineFractionBits;
            const int residual = raw - adjustedBaseline;
            outPtr[i] = (std::abs(residual) <= m_noiseDeadband) ? 0 : SaturateInt16(residual);
        }
        return true;
    }

    inline void Reset() {
        m_initialized = false;
        m_releaseHold.fill(0);
        m_acquired.fill(0);
        m_baselineQ8.fill(0);
    }

private:
    static constexpr int kBaselineFractionBits = 8;

    bool m_initialized = false;
    std::array<int32_t, kCellCount> m_baselineQ8{};
    std::array<uint8_t, kCellCount> m_releaseHold{};
    std::array<uint8_t, kCellCount> m_acquired{};

    inline void Initialize() {
        const int initialBaseline = std::clamp(m_baseline, 0, 0xFFFF);
        m_baselineQ8.fill(static_cast<int32_t>(initialBaseline) << kBaselineFractionBits);
        m_releaseHold.fill(0);
        m_acquired.fill(0);
        m_initialized = true;
    }

    inline void UpdateBaseline(int index, int delta, int alphaShift, int maxStep) {
        const int shift = std::clamp(alphaShift, 0, 15);
        const int32_t maxStepQ8 = static_cast<int32_t>(std::max(0, maxStep)) << kBaselineFractionBits;
        int32_t updateQ8 = (static_cast<int32_t>(delta) << kBaselineFractionBits) >> shift;
        if (maxStepQ8 > 0) {
            updateQ8 = std::clamp(updateQ8, -maxStepQ8, maxStepQ8);
        }
        auto& baseline = m_baselineQ8[static_cast<size_t>(index)];
        baseline = std::clamp(baseline + updateQ8, 0, 0xFFFF << kBaselineFractionBits);
    }

    static inline uint16_t RawCell(int16_t value) {
        return static_cast<uint16_t>(value);
    }

    static inline int16_t SaturateInt16(int value) {
        return static_cast<int16_t>(std::clamp(value,
                                             static_cast<int>(INT16_MIN),
                                             static_cast<int>(INT16_MAX)));
    }
};

}} // namespace Solvers::Touch
