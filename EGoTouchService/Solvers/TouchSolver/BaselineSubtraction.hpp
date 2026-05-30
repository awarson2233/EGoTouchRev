#pragma once

#include "SolverTypes.h"
#include <algorithm>
#include <array>
#include <atomic>
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
    int  m_baseline = 32768;

    int  m_noiseDeadband = 90;
    int  m_positiveDriftDeadband = 14;
    int  m_negativeDeadband = 13;
    int  m_touchFreezeThreshold = 305;
    int  m_releaseHoldFrames = 60;
    int  m_positiveAlphaShift = 7;
    int  m_negativeAlphaShift = 5;
    int  m_noiseAlphaShift = 6;
    int  m_positiveMaxStep = 20;
    int  m_negativeMaxStep = 20;
    int  m_acquisitionAlphaShift = 3;
    int  m_acquisitionMaxStep = 128;
    bool m_noiseTrackingEnabled = true;
    int  m_settleFrames = 2;

    inline void RequestReacquireFrames(int frames) {
        const int requested = std::clamp(frames, 0, 255);
        if (requested <= 0) {
            return;
        }

        int current = m_reacquireFrames.load(std::memory_order_relaxed);
        while (current < requested &&
               !m_reacquireFrames.compare_exchange_weak(
                   current, requested, std::memory_order_relaxed)) {
        }
        m_snapshotBaseline.store(true, std::memory_order_relaxed);
    }

    inline bool Process(HeatmapFrame& frame) {
        int16_t* outPtr = &frame.heatmapMatrix[0][0];

        if (!m_enabled) {
            return true;
        }

        if (!m_initialized) {
            Initialize();
        }

        if (m_snapshotBaseline.exchange(false, std::memory_order_relaxed)) {
            SnapshotBaseline(outPtr);
        }

        if (m_settleCountdown > 0) {
            --m_settleCountdown;
            ZeroOutput(outPtr);
            return true;
        }

        const bool reacquiring = m_reacquireFrames.load(std::memory_order_relaxed) > 0;
        int positiveFreezeCandidates = 0;
        if (!reacquiring) {
            for (int i = 0; i < kCellCount; ++i) {
                const int baseline = m_baselineQ8[static_cast<size_t>(i)] >> kBaselineFractionBits;
                if (static_cast<int>(RawCell(outPtr[i])) - baseline >= m_touchFreezeThreshold) {
                    ++positiveFreezeCandidates;
                }
            }
        }
        const bool broadPositiveShift = !reacquiring && positiveFreezeCandidates > (kCellCount / 8);

        for (int i = 0; i < kCellCount; ++i) {
            const int raw = static_cast<int>(RawCell(outPtr[i]));
            const int baseline = m_baselineQ8[static_cast<size_t>(i)] >> kBaselineFractionBits;
            const int delta = raw - baseline;
            const int absDelta = std::abs(delta);

            if (reacquiring) {
                m_releaseHold[static_cast<size_t>(i)] = 0;
                if (absDelta <= m_noiseDeadband) {
                    m_acquired[static_cast<size_t>(i)] = 1;
                } else {
                    UpdateBaseline(i, delta, m_acquisitionAlphaShift, m_acquisitionMaxStep);
                }
            } else if (m_snapshotHighCells[static_cast<size_t>(i)]) {
                if (delta < -m_negativeDeadband) {
                    SetBaselineToRaw(i, raw);
                    m_snapshotHighCells[static_cast<size_t>(i)] = 0;
                } else if (delta > 0) {
                    UpdateBaseline(i, delta, m_acquisitionAlphaShift, m_acquisitionMaxStep);
                } else if (absDelta <= m_noiseDeadband && delta != 0) {
                    UpdateBaseline(i, delta, m_noiseAlphaShift, 1);
                }
                outPtr[i] = 0;
                continue;
            } else if (!m_acquired[static_cast<size_t>(i)]) {
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
            if (reacquiring && std::abs(residual) <= m_noiseDeadband) {
                m_acquired[static_cast<size_t>(i)] = 1;
            }
            outPtr[i] = (std::abs(residual) <= m_noiseDeadband) ? 0 : SaturateInt16(residual);
        }
        if (reacquiring) {
            int current = m_reacquireFrames.load(std::memory_order_relaxed);
            while (current > 0 &&
                   !m_reacquireFrames.compare_exchange_weak(
                       current, current - 1, std::memory_order_relaxed)) {
            }
        }
        return true;
    }

    inline void Reset() {
        m_initialized = false;
        m_reacquireFrames.store(0, std::memory_order_relaxed);
        m_snapshotBaseline.store(false, std::memory_order_relaxed);
        m_settleCountdown = 0;
        m_releaseHold.fill(0);
        m_acquired.fill(0);
        m_snapshotHighCells.fill(0);
        m_baselineQ8.fill(0);
    }

private:
    static constexpr int kBaselineFractionBits = 8;

    bool m_initialized = false;
    std::atomic<int> m_reacquireFrames{0};
    std::atomic<bool> m_snapshotBaseline{false};
    int m_settleCountdown = 0;
    std::array<int32_t, kCellCount> m_baselineQ8{};
    std::array<uint8_t, kCellCount> m_releaseHold{};
    std::array<uint8_t, kCellCount> m_acquired{};
    std::array<uint8_t, kCellCount> m_snapshotHighCells{};

    inline void Initialize() {
        const int initialBaseline = std::clamp(m_baseline, 0, 0xFFFF);
        m_baselineQ8.fill(static_cast<int32_t>(initialBaseline) << kBaselineFractionBits);
        m_releaseHold.fill(0);
        m_acquired.fill(0);
        m_snapshotHighCells.fill(0);
        m_initialized = true;
    }

    inline void SnapshotBaseline(int16_t* cells) {
        for (int i = 0; i < kCellCount; ++i) {
            const int raw = static_cast<int>(RawCell(cells[i]));
            const int previousBaseline = m_baselineQ8[static_cast<size_t>(i)] >> kBaselineFractionBits;
            m_snapshotHighCells[static_cast<size_t>(i)] =
                (raw - previousBaseline >= m_touchFreezeThreshold) ? 1 : 0;
            SetBaselineToRaw(i, raw);
            m_acquired[static_cast<size_t>(i)] = 1;
            m_releaseHold[static_cast<size_t>(i)] = 0;
        }
        m_settleCountdown = std::clamp(m_settleFrames, 0, 255);
        m_reacquireFrames.store(0, std::memory_order_relaxed);
    }

    inline void SetBaselineToRaw(int index, int raw) {
        const int clampedRaw = std::clamp(raw, 0, 0xFFFF);
        m_baselineQ8[static_cast<size_t>(index)] =
            static_cast<int32_t>(clampedRaw) << kBaselineFractionBits;
    }

    static inline void ZeroOutput(int16_t* cells) {
        for (int i = 0; i < kCellCount; ++i) {
            cells[i] = 0;
        }
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
