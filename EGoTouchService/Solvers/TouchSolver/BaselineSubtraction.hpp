#pragma once

#include "SolverTypes.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

namespace Solvers { namespace Touch {

enum class FingerState {
    Unknown,
    NoFinger,
    Finger,
};

struct BaselineInputState {
    bool masterValid = true;
    FingerState fingerState = FingerState::Unknown;
};

class BaselineSubtraction {
public:
    static constexpr int kRows = 40;
    static constexpr int kCols = 60;
    static constexpr int kCellCount = kRows * kCols;

    bool m_enabled = true;
    int  m_baseline = 0x7FEE;

    int  m_noiseDeadband = 90;
    int  m_positiveDriftDeadband = 14;
    int  m_negativeDeadband = 13;
    int  m_touchFreezeThreshold = 305;
    int  m_freezeCandidateThreshold = 350;
    int  m_releaseHoldFrames = 60;
    int  m_positiveAlphaShift = 7;
    int  m_negativeAlphaShift = 5;
    int  m_noiseAlphaShift = 6;
    int  m_positiveMaxStep = 20;
    int  m_negativeMaxStep = 20;
    int  m_acquisitionAlphaShift = 3;
    int  m_acquisitionMaxStep = 128;
    int  m_noFingerAlphaShift = 3;
    int  m_noFingerMaxStep = 512;
    int  m_fingerBackgroundAlphaShift = 3;
    int  m_fingerBackgroundMaxStep = 512;
    bool m_noiseTrackingEnabled = true;

    inline void RequestReacquireFrames(int frames) {
        const int clampedFrames = std::clamp(frames, 1, kMaxReacquireFrames);
        int current = m_requestedReacquireFrames.load(std::memory_order_relaxed);
        while (current < clampedFrames &&
               !m_requestedReacquireFrames.compare_exchange_weak(
                   current,
                   clampedFrames,
                   std::memory_order_release,
                   std::memory_order_relaxed)) {
        }
    }

    inline bool Process(HeatmapFrame& frame) {
        return ProcessFallback(frame);
    }

    inline bool Process(HeatmapFrame& frame, const BaselineInputState& input) {
        if (input.masterValid && input.fingerState == FingerState::Unknown) {
            return ProcessUnknownNoFinger(frame);
        }

        int16_t* outPtr = &frame.heatmapMatrix[0][0];

        if (!m_enabled) {
            return true;
        }

        if (!m_initialized) {
            Initialize();
        }

        if (!input.masterValid) {
            m_freezeMask.fill(0);
            ZeroOutput(outPtr);
            return true;
        }

        ConsumeReacquireRequest();
        if (ProcessReacquireFrame(outPtr)) {
            return true;
        }

        const int commonDiff = BuildFreezeMask(outPtr, input.fingerState);

        for (int i = 0; i < kCellCount; ++i) {
            const int raw = static_cast<int>(RawCell(outPtr[i]));
            const int baseline = m_baselineQ8[static_cast<size_t>(i)] >> kBaselineFractionBits;
            const int delta = raw - baseline;
            const int localDiff = delta - commonDiff;

            if (input.fingerState == FingerState::NoFinger) {
                m_releaseHold[static_cast<size_t>(i)] = 0;
                UpdateNoFingerBaseline(i, delta);

                const int adjustedBaseline = m_baselineQ8[static_cast<size_t>(i)] >> kBaselineFractionBits;
                const int diff = raw - adjustedBaseline;
                if (std::abs(diff) <= m_noiseDeadband) {
                    m_acquired[static_cast<size_t>(i)] = 1;
                }
                outPtr[i] = 0;
                continue;
            }

            if (m_freezeMask[static_cast<size_t>(i)] == 1) {
                m_releaseHold[static_cast<size_t>(i)] = static_cast<uint8_t>(
                    std::clamp(m_releaseHoldFrames, 0, 255));
                m_acquired[static_cast<size_t>(i)] = 1;
                if (commonDiff != 0) {
                    UpdateBaseline(i, commonDiff,
                                   m_fingerBackgroundAlphaShift,
                                   m_fingerBackgroundMaxStep);
                }
                outPtr[i] = SaturateInt16(localDiff);
                continue;
            }

            if (m_freezeMask[static_cast<size_t>(i)] == 2) {
                m_releaseHold[static_cast<size_t>(i)] = 0;
                UpdateFingerBackgroundBaseline(i, delta);
                m_acquired[static_cast<size_t>(i)] = 1;
                outPtr[i] = 0;
                continue;
            }

            const int absLocalDiff = std::abs(localDiff);
            if (m_releaseHold[static_cast<size_t>(i)] > 0) {
                --m_releaseHold[static_cast<size_t>(i)];
                if (localDiff < -m_negativeDeadband) {
                    outPtr[i] = SaturateInt16(localDiff);
                    continue;
                }
            } else {
                UpdateFingerBackgroundBaseline(i, delta);
            }

            if (absLocalDiff <= m_noiseDeadband) {
                m_acquired[static_cast<size_t>(i)] = 1;
            }
            outPtr[i] = (absLocalDiff <= m_noiseDeadband)
                ? 0
                : SaturateInt16(localDiff);
        }

        return true;
    }

    inline void Reset() {
        m_requestedReacquireFrames.store(0, std::memory_order_release);
        m_reacquireFramesRemaining = 0;
        m_reacquireSnapshotPending = false;
        ResetArrays();
    }

private:
    static constexpr int kBaselineFractionBits = 8;
    static constexpr int kMaxReacquireFrames = 120;

    bool m_initialized = false;
    int m_reacquireFramesRemaining = 0;
    bool m_reacquireSnapshotPending = false;
    std::atomic<int> m_requestedReacquireFrames{0};
    std::array<int32_t, kCellCount> m_baselineQ8{};
    std::array<uint8_t, kCellCount> m_releaseHold{};
    std::array<uint8_t, kCellCount> m_acquired{};
    std::array<uint8_t, kCellCount> m_freezeMask{};

    inline bool ProcessUnknownNoFinger(HeatmapFrame& frame) {
        int16_t* outPtr = &frame.heatmapMatrix[0][0];

        if (!m_enabled) {
            return true;
        }

        if (!m_initialized) {
            Initialize();
        }

        ConsumeReacquireRequest();
        if (ProcessReacquireFrame(outPtr)) {
            return true;
        }

        const int commonDiff = EstimateDiffMedian(outPtr);

        for (int i = 0; i < kCellCount; ++i) {
            const int raw = static_cast<int>(RawCell(outPtr[i]));
            const int baseline = m_baselineQ8[static_cast<size_t>(i)] >> kBaselineFractionBits;
            const int delta = raw - baseline;
            const int localDiff = delta - commonDiff;
            const int absLocalDiff = std::abs(localDiff);

            const bool releaseHeld = m_releaseHold[static_cast<size_t>(i)] > 0;
            if (releaseHeld) {
                --m_releaseHold[static_cast<size_t>(i)];
            }

            if (commonDiff != 0) {
                UpdateBaseline(i, commonDiff,
                               m_fingerBackgroundAlphaShift,
                               m_fingerBackgroundMaxStep);
            }

            if (!releaseHeld) {
                if (absLocalDiff <= m_noiseDeadband) {
                    m_acquired[static_cast<size_t>(i)] = 1;
                    if (localDiff != 0) {
                        UpdateBaseline(i, localDiff, m_noiseAlphaShift, 1);
                    }
                } else if (localDiff < -m_negativeDeadband) {
                    UpdateBaseline(i, localDiff,
                                   m_fingerBackgroundAlphaShift,
                                   m_fingerBackgroundMaxStep);
                }
            }

            outPtr[i] = 0;
        }

        return true;
    }

    inline bool ProcessFallback(HeatmapFrame& frame) {
        int16_t* outPtr = &frame.heatmapMatrix[0][0];

        if (!m_enabled) {
            return true;
        }

        if (!m_initialized) {
            Initialize();
        }

        ConsumeReacquireRequest();
        if (ProcessReacquireFrame(outPtr)) {
            return true;
        }

        const int commonDiff = EstimateDiffMedian(outPtr);

        for (int i = 0; i < kCellCount; ++i) {
            const int raw = static_cast<int>(RawCell(outPtr[i]));
            const int baseline = m_baselineQ8[static_cast<size_t>(i)] >> kBaselineFractionBits;
            const int delta = raw - baseline;
            const int localDiff = delta - commonDiff;
            const int absLocalDiff = std::abs(localDiff);

            if (!m_acquired[static_cast<size_t>(i)]) {
                if (absLocalDiff <= m_noiseDeadband) {
                    m_acquired[static_cast<size_t>(i)] = 1;
                    if (delta != 0) {
                        UpdateBaseline(i, delta, m_acquisitionAlphaShift, m_acquisitionMaxStep);
                    }
                } else {
                    UpdateBaseline(i, delta, m_acquisitionAlphaShift, m_acquisitionMaxStep);
                }
            } else if (localDiff >= m_touchFreezeThreshold) {
                m_releaseHold[static_cast<size_t>(i)] = static_cast<uint8_t>(
                    std::clamp(m_releaseHoldFrames, 0, 255));
                outPtr[i] = SaturateInt16(localDiff);
                continue;
            } else if (m_releaseHold[static_cast<size_t>(i)] > 0) {
                --m_releaseHold[static_cast<size_t>(i)];
                if (localDiff < -m_negativeDeadband) {
                    outPtr[i] = SaturateInt16(localDiff);
                    continue;
                }
            } else if (absLocalDiff <= m_noiseDeadband) {
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

            outPtr[i] = (absLocalDiff <= m_noiseDeadband) ? 0 : SaturateInt16(localDiff);
        }
        return true;
    }

    inline void Initialize() {
        const int initialBaseline = std::clamp(m_baseline, 0, 0xFFFF);
        m_baselineQ8.fill(static_cast<int32_t>(initialBaseline) << kBaselineFractionBits);
        m_releaseHold.fill(0);
        m_acquired.fill(0);
        m_freezeMask.fill(0);
        m_initialized = true;
    }

    inline void ResetArrays() {
        m_initialized = false;
        m_releaseHold.fill(0);
        m_acquired.fill(0);
        m_freezeMask.fill(0);
        m_baselineQ8.fill(0);
    }

    inline void ConsumeReacquireRequest() {
        const int requested = m_requestedReacquireFrames.exchange(0, std::memory_order_acquire);
        if (requested <= 0) {
            return;
        }
        if (!m_initialized) {
            Initialize();
        }
        m_reacquireFramesRemaining = std::max(m_reacquireFramesRemaining, requested);
        m_reacquireSnapshotPending = true;
        m_releaseHold.fill(0);
        m_acquired.fill(0);
        m_freezeMask.fill(0);
    }

    inline bool ProcessReacquireFrame(int16_t* cells) {
        if (m_reacquireFramesRemaining <= 0) {
            return false;
        }

        if (m_reacquireSnapshotPending) {
            const int commonDiff = EstimateDiffMedian(cells);
            const int32_t commonDiffQ8 = static_cast<int32_t>(commonDiff) << kBaselineFractionBits;
            for (int i = 0; i < kCellCount; ++i) {
                auto& baseline = m_baselineQ8[static_cast<size_t>(i)];
                baseline = std::clamp(baseline + commonDiffQ8,
                                      0,
                                      0xFFFF << kBaselineFractionBits);
                m_acquired[static_cast<size_t>(i)] = 1;
            }
            m_reacquireSnapshotPending = false;
        } else {
            for (int i = 0; i < kCellCount; ++i) {
                const int raw = static_cast<int>(RawCell(cells[i]));
                const int baseline = m_baselineQ8[static_cast<size_t>(i)] >> kBaselineFractionBits;
                UpdateNoFingerBaseline(i, raw - baseline);
            }
        }

        --m_reacquireFramesRemaining;
        ZeroOutput(cells);
        return true;
    }

    inline int BuildFreezeMask(int16_t* cells, FingerState fingerState) {
        m_freezeMask.fill(0);
        if (fingerState != FingerState::Finger) {
            return 0;
        }

        const int commonDiff = EstimateDiffMedian(cells);
        int broadCandidateCount = 0;
        for (int i = 0; i < kCellCount; ++i) {
            const int baseline = m_baselineQ8[static_cast<size_t>(i)] >> kBaselineFractionBits;
            const int delta = static_cast<int>(RawCell(cells[i])) - baseline;
            const int localDiff = delta - commonDiff;
            if (localDiff >= FreezeCandidateThreshold()) {
                ++broadCandidateCount;
            }
        }

        if (broadCandidateCount > (kCellCount / 8)) {
            for (int i = 0; i < kCellCount; ++i) {
                const int baseline = m_baselineQ8[static_cast<size_t>(i)] >> kBaselineFractionBits;
                const int delta = static_cast<int>(RawCell(cells[i])) - baseline;
                const int localDiff = delta - commonDiff;
                m_freezeMask[static_cast<size_t>(i)] =
                    (localDiff >= FreezeCandidateThreshold()) ? 2 : 0;
            }
            return commonDiff;
        }

        for (int i = 0; i < kCellCount; ++i) {
            const int baseline = m_baselineQ8[static_cast<size_t>(i)] >> kBaselineFractionBits;
            const int delta = static_cast<int>(RawCell(cells[i])) - baseline;
            const int localDiff = delta - commonDiff;
            m_freezeMask[static_cast<size_t>(i)] =
                (localDiff >= TouchFreezeThreshold()) ? 1 : 0;
        }
        return commonDiff;
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

    inline void UpdateNoFingerBaseline(int index, int delta) {
        const int absDelta = std::abs(delta);
        if (absDelta <= m_noiseDeadband) {
            if (delta != 0) {
                UpdateBaseline(index, delta, m_noiseAlphaShift, 1);
            }
            return;
        }
        UpdateBaseline(index, delta, m_noFingerAlphaShift, m_noFingerMaxStep);
    }

    inline void UpdateFingerBackgroundBaseline(int index, int delta) {
        const int absDelta = std::abs(delta);
        if (absDelta <= m_noiseDeadband) {
            if (delta != 0) {
                UpdateBaseline(index, delta, m_noiseAlphaShift, 1);
            }
            return;
        }
        UpdateBaseline(index, delta, m_fingerBackgroundAlphaShift, m_fingerBackgroundMaxStep);
    }

    inline int FreezeCandidateThreshold() const {
        return std::max(1, m_freezeCandidateThreshold);
    }

    inline int TouchFreezeThreshold() const {
        return std::max(1, m_touchFreezeThreshold);
    }

    inline int EstimateDiffMedian(int16_t* cells) const {
        std::array<int, kCellCount> diffs{};
        for (int i = 0; i < kCellCount; ++i) {
            const int baseline = m_baselineQ8[static_cast<size_t>(i)] >> kBaselineFractionBits;
            diffs[static_cast<size_t>(i)] = static_cast<int>(RawCell(cells[i])) - baseline;
        }

        auto mid = diffs.begin() + (kCellCount / 2);
        std::nth_element(diffs.begin(), mid, diffs.end());
        return *mid;
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
