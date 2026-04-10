#pragma once
#include "AsaTypes.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Asa {

/// CoorPostProcessor — Slim coordinate post-processing: IIR + Jitter only.
///
/// All decision-making (coefficient selection, speed calculation, mode switching)
/// has been moved to PenStateMachine MotionProfile. This module now only executes
/// the actual filtering.
///
/// Removed in v2:
///   - PushHistory (24-frame FIFO)
///   - StepCalcSpeed (path-accumulated speed)
///   - StepCalcIIRCoef (speed→coefficient lookup)
///   - Step3PointAvg (redundant with IIR at low speeds)
class CoorPostProcessor {
public:
    static constexpr int kCoorUnit = 0x400;

    inline void Reset() {
        m_initialized = false;
        m_frameCount = 0;
        m_iirDim1Q8 = m_iirDim2Q8 = 0;
        m_anchor = AsaCoorResult{};
        m_offsetDim1 = m_offsetDim2 = 0;
        m_lockDim1 = m_lockDim2 = false;
        m_jitterActive = false;
    }

    // ══════════════════════════════════════════════
    // IIR filter (TSACore: CoorFilterProcess + CoorIIRFilterType)
    // Q8 fixed-point: coordinate × 256 + fractional remainder
    // Remainder is preserved across frames for sub-LSB precision
    // ══════════════════════════════════════════════
    inline AsaCoorResult StepIIR(const AsaCoorResult& cur,
                                  int coefInt, int divisorN, bool skipIIR) {
        m_frameCount++;

        // Skip IIR: direct pass-through, reset IIR state
        if (skipIIR) {
            m_iirDim1Q8 = cur.dim1 << 8;
            m_iirDim2Q8 = cur.dim2 << 8;
            m_initialized = true;
            return cur;
        }

        // First valid frame: initialize
        if (!m_initialized) {
            m_iirDim1Q8 = cur.dim1 << 8;
            m_iirDim2Q8 = cur.dim2 << 8;
            m_initialized = true;
            return cur;
        }

        const int N = std::max(1, divisorN);

        m_iirDim1Q8 = IIRFilterQ8(m_iirDim1Q8, cur.dim1 << 8, coefInt, N);
        m_iirDim2Q8 = IIRFilterQ8(m_iirDim2Q8, cur.dim2 << 8, coefInt, N);

        AsaCoorResult out = cur;
        out.dim1 = m_iirDim1Q8 >> 8;
        out.dim2 = m_iirDim2Q8 >> 8;
        return out;
    }

    // ══════════════════════════════════════════════
    // Jitter offset compensation (TSACore: AftCoorProcess)
    // Dynamic threshold based on sensor/screen dimensions
    // Independent X/Y axis locking
    // ══════════════════════════════════════════════
    inline AsaCoorResult StepJitter(const AsaCoorResult& cur,
                                     int strength, bool isEdge) {
        if (strength <= 0) return cur;

        // Compute thresholds from strength (replaces old config params)
        // strength: 1=weak, 2=medium, 3=strong
        int32_t baseDim1 = (screenDimDim1 > 0)
            ? (strength * sensorDimCols * kCoorUnit) / screenDimDim1
            : static_cast<int32_t>(strength) * 10;
        int32_t baseDim2 = (screenDimDim2 > 0)
            ? (strength * sensorDimRows * kCoorUnit) / screenDimDim2
            : static_cast<int32_t>(strength) * 10;

        int32_t thrDim1 = isEdge ? baseDim1 * 3 / 2 : baseDim1;
        int32_t thrDim2 = isEdge ? baseDim2 * 3 / 2 : baseDim2;

        // Lock starts on first valid frame
        if (!m_jitterActive) {
            m_anchor = cur;
            m_offsetDim1 = 0;
            m_offsetDim2 = 0;
            m_lockDim1 = true;
            m_lockDim2 = true;
            m_jitterActive = true;
            return cur;
        }

        // Independent X/Y axis lock check
        if (m_lockDim1) {
            int32_t dx = cur.dim1 - m_anchor.dim1;
            if (std::abs(dx) > thrDim1) {
                m_lockDim1 = false;
            }
            if (m_lockDim1) {
                m_offsetDim1 = cur.dim1 - m_anchor.dim1;
            }
        }

        if (m_lockDim2) {
            int32_t dy = cur.dim2 - m_anchor.dim2;
            if (std::abs(dy) > thrDim2) {
                m_lockDim2 = false;
            }
            if (m_lockDim2) {
                m_offsetDim2 = cur.dim2 - m_anchor.dim2;
            }
        }

        // output = coordinate - accumulated offset, clamped
        AsaCoorResult out = cur;
        int32_t resultDim1 = cur.dim1 - m_offsetDim1;
        int32_t resultDim2 = cur.dim2 - m_offsetDim2;

        const int32_t maxDim1 = sensorDimCols * kCoorUnit;
        const int32_t maxDim2 = sensorDimRows * kCoorUnit;
        out.dim1 = std::clamp(resultDim1, 0, maxDim1);
        out.dim2 = std::clamp(resultDim2, 0, maxDim2);
        return out;
    }

    // ── Diagnostic accessors ──
    int GetFrameCount() const { return m_frameCount; }

    // ── Configuration (Jitter) ──
    int  screenDimDim1 = 16000;       // HID X resolution
    int  screenDimDim2 = 25600;       // HID Y resolution
    int  sensorDimCols = 60;          // Total sensor columns (dim1/X)
    int  sensorDimRows = 40;          // Total sensor rows (dim2/Y)

private:
    bool m_initialized = false;
    int  m_frameCount = 0;

    // IIR Q8 state
    int32_t m_iirDim1Q8 = 0;
    int32_t m_iirDim2Q8 = 0;

    // Jitter state
    AsaCoorResult m_anchor{};
    int32_t m_offsetDim1 = 0;
    int32_t m_offsetDim2 = 0;
    bool    m_lockDim1 = false;
    bool    m_lockDim2 = false;
    bool    m_jitterActive = false;

    /// Integer IIR core: (coef * cur + (N - coef) * prev) / N
    static inline int32_t IIRFilterQ8(
            int32_t prevQ8, int32_t curQ8, int coef, int N) {
        return (coef * curQ8 + (N - coef) * prevQ8) / N;
    }
};

} // namespace Asa
