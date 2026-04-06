#include "CoorPostProcessor.h"
#include <algorithm>
#include <cmath>

namespace Asa {

void CoorPostProcessor::Reset() {
    m_initialized = false;
    m_frameCount = 0;
    m_validHistory = 0;
    m_history.fill(AsaCoorResult{});
    m_prev[0] = m_prev[1] = AsaCoorResult{};
    m_iirDim1 = m_iirDim2 = 0.0f;
    m_anchor = AsaCoorResult{};
    m_offsetDim1 = m_offsetDim2 = 0;
    m_jitterActive = false;
    m_speed = SpeedMetrics{};
}

// ══════════════════════════════════════════════
// PushHistory — FIFO shift (index 0 = newest)
// Mirrors TSACore memmove(&buf[1], &buf[0], 23*sizeof)
// ══════════════════════════════════════════════
void CoorPostProcessor::PushHistory(const AsaCoorResult& cur) {
    // Shift all entries one slot toward the end
    for (int i = kHistoryLen - 1; i > 0; --i) {
        m_history[static_cast<size_t>(i)] =
            m_history[static_cast<size_t>(i - 1)];
    }
    m_history[0] = cur;
    m_validHistory = std::min(m_validHistory + 1, kHistoryLen);
}

// ══════════════════════════════════════════════
// CalcSpeed — 3-tier path-accumulated speed
// Mirrors TSACore GetCoorSpeed exactly:
//   instant  = 1-frame segment distance
//   shortAvg = avg of first 3 segments
//   fullAvg  = avg of all valid segments
// ══════════════════════════════════════════════
SpeedMetrics CoorPostProcessor::CalcSpeed() {
    SpeedMetrics out{};
    if (m_validHistory < 2) return out;

    const int segments = m_validHistory - 1;
    float accumPath = 0.0f;

    for (int i = 0; i < segments; ++i) {
        const auto& a = m_history[static_cast<size_t>(i)];
        const auto& b = m_history[static_cast<size_t>(i + 1)];
        if (!a.valid || !b.valid) continue;

        const float dx = static_cast<float>(a.dim1 - b.dim1);
        const float dy = static_cast<float>(a.dim2 - b.dim2);
        const float segDist = std::sqrt(dx * dx + dy * dy);
        accumPath += segDist;

        // Instant speed = first segment
        if (i == 0) {
            out.instant = segDist;
        }
        // Short-term = average of first 3 segments
        if (i == 2) {
            out.shortAvg = accumPath / 3.0f;
        }
    }

    // Full-window average
    out.fullAvg = accumPath / static_cast<float>(segments);

    // If fewer than 3 segments, shortAvg = fullAvg
    if (segments < 3) {
        out.shortAvg = out.fullAvg;
    }

    return out;
}

// ══════════════════════════════════════════════
// CalcIIRCoef — 3-mode adaptive coefficient
// Mirrors TSACore GetIIRCoef with hover/write/edge modes
// ══════════════════════════════════════════════
float CoorPostProcessor::CalcIIRCoef(
        float speed, bool isHover, bool isEdge) {
    float lo, hi, thrLo, thrHi;

    if (isHover) {
        lo = hoverIirLow;   hi = hoverIirHigh;
        thrLo = hoverLowThr; thrHi = hoverHighThr;
    } else {
        lo = writeIirLow;   hi = writeIirHigh;
        thrLo = writeLowThr; thrHi = writeHighThr;
    }

    // Edge modifier: halve coefficients (reduces lag at edges)
    if (isEdge && enableEdgeHalve) {
        lo *= 0.5f;
        hi *= 0.5f;
    }

    // Linear interpolation between low and high
    if (speed <= thrLo) return lo;
    if (speed >= thrHi) return hi;
    const float t = (speed - thrLo) / (thrHi - thrLo);
    return lo + t * (hi - lo);
}

// ══════════════════════════════════════════════
// ApplyIIR — 1st-order IIR low-pass filter
// ══════════════════════════════════════════════
AsaCoorResult CoorPostProcessor::ApplyIIR(
        const AsaCoorResult& cur, float coef) {
    AsaCoorResult out = cur;
    m_iirDim1 = m_iirDim1 * (1.0f - coef) +
                static_cast<float>(cur.dim1) * coef;
    m_iirDim2 = m_iirDim2 * (1.0f - coef) +
                static_cast<float>(cur.dim2) * coef;
    out.dim1 = static_cast<int32_t>(std::lround(m_iirDim1));
    out.dim2 = static_cast<int32_t>(std::lround(m_iirDim2));
    return out;
}

// ══════════════════════════════════════════════
// Apply3PointAvg — 3-frame moving average
// ══════════════════════════════════════════════
AsaCoorResult CoorPostProcessor::Apply3PointAvg(
        const AsaCoorResult& cur) {
    if (!m_prev[0].valid || !m_prev[1].valid) return cur;
    AsaCoorResult out = cur;
    out.dim1 = (m_prev[1].dim1 + m_prev[0].dim1 + cur.dim1) / 3;
    out.dim2 = (m_prev[1].dim2 + m_prev[0].dim2 + cur.dim2) / 3;
    return out;
}

// ══════════════════════════════════════════════
// ApplyJitterOffset — Offset-compensation method
// Mirrors TSACore AftCoorProcess:
//   1. On first frame (pen-down), record "anchor" position
//   2. While displacement < threshold: accumulate offset
//   3. Output = current_coord - offset
//   4. When displacement > threshold: offset stays frozen
//      (no sudden jump — offset just stops growing)
//
// Key advantage over simple lock:
//   - Output always tracks current_coord minus a constant offset
//   - No discontinuity when leaving dead zone
// ══════════════════════════════════════════════
AsaCoorResult CoorPostProcessor::ApplyJitterOffset(
        const AsaCoorResult& cur, bool isEdge) {
    const int32_t thr = isEdge ? jitterEdgeThreshold
                               : jitterCenterThreshold;

    if (!m_jitterActive) {
        // First valid frame → set anchor, zero offset
        m_anchor = cur;
        m_offsetDim1 = 0;
        m_offsetDim2 = 0;
        m_jitterActive = true;
        return cur;
    }

    // Displacement from anchor
    const int32_t dx = cur.dim1 - m_anchor.dim1;
    const int32_t dy = cur.dim2 - m_anchor.dim2;

    if (std::abs(dx) <= thr && std::abs(dy) <= thr) {
        // Still in dead zone: accumulate offset
        // offset = anchor - cur, so output = cur + offset = anchor
        m_offsetDim1 = m_anchor.dim1 - cur.dim1;
        m_offsetDim2 = m_anchor.dim2 - cur.dim2;
    }
    // else: outside dead zone — offset stays frozen at last value.
    // Output smoothly follows cur with a constant shift.

    AsaCoorResult out = cur;
    out.dim1 = cur.dim1 + m_offsetDim1;
    out.dim2 = cur.dim2 + m_offsetDim2;
    return out;
}

// ══════════════════════════════════════════════
// Process — main post-processing chain
// ══════════════════════════════════════════════
AsaCoorResult CoorPostProcessor::Process(
        const AsaCoorResult& raw,
        bool isHover, bool isEdge) {
    if (!raw.valid) {
        Reset();
        return raw;
    }

    AsaCoorResult cur = raw;

    // Stage 1: Linear filter (optional, complex state machine)
    // if (enableLinearFilter) { ... } // TODO: Phase P2

    // Stage 2: Push to ring buffer (index 0 = newest)
    PushHistory(cur);

    // Stage 3: 3-point average filter
    if (enable3PointAvg && m_frameCount >= 2) {
        cur = Apply3PointAvg(cur);
    }

    // Stage 4: CoorRevise (TX2 correction) — TODO: Phase P2

    // Stage 5: Speed calculation (3-tier, path-accumulated)
    m_speed = CalcSpeed();

    // Stage 6: Dynamic IIR coefficient (hover/write/edge modes)
    // Use instant speed for coefficient selection (matches TSACore)
    const float coef = CalcIIRCoef(m_speed.instant, isHover, isEdge);
    m_lastIirCoef = coef;

    // Stage 7: IIR coordinate filter
    if (!m_initialized) {
        m_iirDim1 = static_cast<float>(cur.dim1);
        m_iirDim2 = static_cast<float>(cur.dim2);
        m_initialized = true;
    } else {
        cur = ApplyIIR(cur, coef);
    }

    // Stage 8: Jitter suppression (offset-compensation method)
    cur = ApplyJitterOffset(cur, isEdge);

    // Stage 9: FitToLcdScreen — done in StylusPipeline::BuildPacket

    // Update 3-point history
    m_prev[1] = m_prev[0];
    m_prev[0] = cur;
    m_frameCount++;
    return cur;
}

} // namespace Asa
