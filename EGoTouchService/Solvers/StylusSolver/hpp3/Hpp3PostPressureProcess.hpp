#pragma once

#include "SolverTypes.h"

#include <algorithm>
#include <cstdint>

namespace Solvers::Stylus {

class Hpp3PostPressureProcess {
public:
    bool m_enabled = true;
    bool m_fakePressureDecreaseEnabled = false;
    uint16_t m_pressureEdgeEnterThreshold = 1500;
    uint16_t m_pressureEdgeExitThreshold = 3000;
    int m_btFreqShiftDebounceFrames = 2;

    inline bool Process(HeatmapFrame& frame) {
        auto& runtime = frame.stylus.runtime;
        auto& pressure = runtime.pressure;
        auto& decision = runtime.decision;
        const auto& coor = runtime.tx1.coordinate.reportGlobalCoor;

        if (!m_enabled) {
            PublishPressureToPost(runtime);
            return true;
        }

        if (pressure.lookaheadHoverGate) {
            ApplyBtFreqShiftGate(pressure.btSample);
            ClearTransientPressureState();
            pressure.outputPressure = 0;
            decision.tipDownCandidate = false;
            decision.authoritativeDown = false;
            UpdatePrevious(coor, 0);
            PublishPressureToPost(runtime);
#if EGOTOUCH_DIAG
            CaptureDebugState(pressure);
#endif
            return true;
        }

        if (pressure.outputPressure == 0 && decision.tipDownCandidate) {
            pressure.outputPressure = m_previousOutputPressure != 0 ? m_previousOutputPressure : 10;
        }

        const bool skipFakePressureDecrease = ApplyBtFreqShiftGate(pressure.btSample);
        if (!skipFakePressureDecrease &&
            m_fakePressureDecreaseEnabled &&
            m_previousOutputPressure > 500 &&
            pressure.outputPressure <= 10) {
            if (!m_fakePressureDecreaseArmed && m_fakePressureDecreaseFramesLeft == 0) {
                m_fakePressureDecreaseFramesLeft = FakePressureFramesForMovement(coor);
                m_fakePressureDecreaseArmed = true;
            }
            if (m_fakePressureDecreaseFramesLeft != 0) {
                const uint32_t fakePressure =
                    (static_cast<uint32_t>(m_fakePressureDecreaseFramesLeft) * m_previousOutputPressure) /
                    static_cast<uint32_t>(m_fakePressureDecreaseFramesLeft + 1u);
                pressure.outputPressure = static_cast<uint16_t>(std::min<uint32_t>(fakePressure, 0xFFFFu));
                --m_fakePressureDecreaseFramesLeft;
                if (pressure.outputPressure != 0) {
                    decision.tipDownCandidate = true;
                    decision.authoritativeDown = true;
                }
            }
        }

        if (pressure.outputPressure == 0) {
            ClearTransientPressureState();
            UpdatePrevious(coor, pressure.outputPressure);
            PublishPressureToPost(runtime);
#if EGOTOUCH_DIAG
            CaptureDebugState(pressure);
#endif
            return true;
        }

        ApplyEdgeSignalSuppression(runtime.signal, pressure, decision);
        UpdatePrevious(coor, pressure.outputPressure);
        PublishPressureToPost(runtime);
#if EGOTOUCH_DIAG
        CaptureDebugState(pressure);
#endif
        return true;
    }

    inline void Reset() {
        ClearTransientPressureState();
        m_previousOutputPressure = 0;
        m_havePreviousCoordinate = false;
        m_previousDim1 = 0;
        m_previousDim2 = 0;
        m_haveBtFreq = false;
        m_previousFreq1 = 0;
        m_previousFreq2 = 0;
        m_btFreqShiftDebounceFramesLeft = 0;
    }

    inline bool IsEdgeSignalTooLowLatchedForTest() const { return m_edgeSignalTooLowLatched; }
    inline bool IsFakePressureDecreaseArmedForTest() const { return m_fakePressureDecreaseArmed; }
    inline uint8_t GetFakePressureDecreaseFramesLeftForTest() const { return m_fakePressureDecreaseFramesLeft; }
    inline uint8_t GetBtFreqShiftDebounceFramesLeftForTest() const { return m_btFreqShiftDebounceFramesLeft; }
    inline uint16_t GetPreviousOutputPressureForTest() const { return m_previousOutputPressure; }

private:
    static inline void PublishPressureToPost(StylusRuntimeFrame& runtime) {
        runtime.post.finalPressure = runtime.pressure.outputPressure;
        runtime.post.point.rawPressure = runtime.pressure.rawPressure;
        runtime.post.point.mappedPressure = runtime.pressure.mappedPressure;
        runtime.post.point.pressure = runtime.pressure.outputPressure;
    }

    bool m_edgeSignalTooLowLatched = false;
    bool m_fakePressureDecreaseArmed = false;
    uint8_t m_fakePressureDecreaseFramesLeft = 0;
    uint16_t m_previousOutputPressure = 0;
    bool m_havePreviousCoordinate = false;
    int32_t m_previousDim1 = 0;
    int32_t m_previousDim2 = 0;
    bool m_haveBtFreq = false;
    uint8_t m_previousFreq1 = 0;
    uint8_t m_previousFreq2 = 0;
    uint8_t m_btFreqShiftDebounceFramesLeft = 0;

    inline bool ApplyBtFreqShiftGate(const StylusBtInputSnapshot& btSample) {
        if (btSample.hasFreq) {
            if (m_haveBtFreq &&
                (btSample.freq1 != m_previousFreq1 || btSample.freq2 != m_previousFreq2)) {
                m_btFreqShiftDebounceFramesLeft = static_cast<uint8_t>(
                    std::clamp(m_btFreqShiftDebounceFrames, 0, 0xFF));
            }
            m_previousFreq1 = btSample.freq1;
            m_previousFreq2 = btSample.freq2;
            m_haveBtFreq = true;
        }

        if (m_btFreqShiftDebounceFramesLeft == 0) {
            return false;
        }

        m_edgeSignalTooLowLatched = false;
        ClearFakePressureDecreaseState();
        --m_btFreqShiftDebounceFramesLeft;
        return true;
    }

    inline uint8_t FakePressureFramesForMovement(const Asa::AsaCoorResult& coor) const {
        if (!m_havePreviousCoordinate || !coor.valid) {
            return 0;
        }
        const uint32_t movement = std::max(AbsDiff(coor.dim1, m_previousDim1),
                                           AbsDiff(coor.dim2, m_previousDim2));
        if (movement > 500) return 3;
        if (movement > 300) return 2;
        if (movement > 100) return 1;
        return 0;
    }

    inline void ApplyEdgeSignalSuppression(const StylusRuntimeSignal& signal,
                                           StylusRuntimePressure& pressure,
                                           StylusRuntimeDecision& decision) {
        const bool dim1OnEdge = signal.dim1EdgeActive;
        const bool dim2OnEdge = signal.dim2EdgeActive;
        const uint16_t dim1Signal = Dim1SelectedPeakSignal(signal);
        const uint16_t dim2Signal = Dim2SelectedPeakSignal(signal);

        if (!m_edgeSignalTooLowLatched) {
            if (dim1OnEdge && dim2OnEdge) {
                const uint16_t bothEdgeEnter = static_cast<uint16_t>(
                    (static_cast<uint32_t>(m_pressureEdgeEnterThreshold) * 2u) / 3u);
                if (dim1Signal < bothEdgeEnter || dim2Signal < bothEdgeEnter) {
                    m_edgeSignalTooLowLatched = true;
                }
            } else if ((dim1OnEdge && dim1Signal < m_pressureEdgeEnterThreshold) ||
                       (dim2OnEdge && dim2Signal < m_pressureEdgeEnterThreshold)) {
                m_edgeSignalTooLowLatched = true;
            }
        }

        if (m_edgeSignalTooLowLatched) {
            const bool dim1Clear = !dim1OnEdge || dim1Signal > m_pressureEdgeExitThreshold;
            const bool dim2Clear = !dim2OnEdge || dim2Signal > m_pressureEdgeExitThreshold;
            if (dim1Clear && dim2Clear) {
                m_edgeSignalTooLowLatched = false;
            }
        }

        if (m_edgeSignalTooLowLatched) {
            pressure.outputPressure = 0;
            decision.tipDownCandidate = false;
            decision.authoritativeDown = false;
        }
    }

    inline void UpdatePrevious(const Asa::AsaCoorResult& coor, uint16_t outputPressure) {
        if (coor.valid) {
            m_previousDim1 = coor.dim1;
            m_previousDim2 = coor.dim2;
            m_havePreviousCoordinate = true;
        }
        m_previousOutputPressure = outputPressure;
    }

    inline void ClearTransientPressureState() {
        m_edgeSignalTooLowLatched = false;
        ClearFakePressureDecreaseState();
    }

    inline void ClearFakePressureDecreaseState() {
        m_fakePressureDecreaseArmed = false;
        m_fakePressureDecreaseFramesLeft = 0;
    }

#if EGOTOUCH_DIAG
    inline void CaptureDebugState(StylusRuntimePressure& pressure) const {
        pressure.edgeSignalTooLowLatched = m_edgeSignalTooLowLatched;
        pressure.fakePressureDecreaseActive = m_fakePressureDecreaseArmed;
        pressure.fakePressureDecreaseFramesLeft = m_fakePressureDecreaseFramesLeft;
        pressure.btFreqShiftDebounceFramesLeft = m_btFreqShiftDebounceFramesLeft;
    }
#endif

    static inline uint16_t Dim1SelectedPeakSignal(const StylusRuntimeSignal& signal) {
        return signal.dim1EdgeSignal != 0 ? signal.dim1EdgeSignal : signal.signalX;
    }

    static inline uint16_t Dim2SelectedPeakSignal(const StylusRuntimeSignal& signal) {
        return signal.dim2EdgeSignal != 0 ? signal.dim2EdgeSignal : signal.signalY;
    }

    static inline uint32_t AbsDiff(int32_t a, int32_t b) {
        return a > b
            ? static_cast<uint32_t>(a - b)
            : static_cast<uint32_t>(b - a);
    }
};

} // namespace Solvers::Stylus
