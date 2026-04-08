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
    m_lastIirCoef = 0.0f;
}

// ══════════════════════════════════════════════
// Step 1: PushHistory (TSACore: GetRealTimeCoor2Buf)
// FIFO shift (index 0 = newest)
// ══════════════════════════════════════════════
void CoorPostProcessor::StepPushHistory(const AsaCoorResult& cur) {
    for (int i = kHistoryLen - 1; i > 0; --i) {
        m_history[static_cast<size_t>(i)] =
            m_history[static_cast<size_t>(i - 1)];
    }
    m_history[0] = cur;
    m_validHistory = std::min(m_validHistory + 1, kHistoryLen);
    m_frameCount++;
}

// ══════════════════════════════════════════════
// Step 2: 3-point average (TSACore: Get3PointAvgFilter)
// ══════════════════════════════════════════════
AsaCoorResult CoorPostProcessor::Step3PointAvg(
        const AsaCoorResult& cur) {
    if (!enable3PointAvg) return cur;
    if (!m_prev[0].valid || !m_prev[1].valid) return cur;
    AsaCoorResult out = cur;
    out.dim1 = (m_prev[1].dim1 + m_prev[0].dim1 + cur.dim1) / 3;
    out.dim2 = (m_prev[1].dim2 + m_prev[0].dim2 + cur.dim2) / 3;
    return out;
}

// ══════════════════════════════════════════════
// Step 3: Speed calculation (TSACore: GetCoorSpeed)
// 3-tier path-accumulated speed
// ══════════════════════════════════════════════
void CoorPostProcessor::StepCalcSpeed() {
    SpeedMetrics out{};
    if (m_validHistory < 2) { m_speed = out; return; }

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

        if (i == 0) out.instant = segDist;
        if (i == 2) out.shortAvg = accumPath / 3.0f;
    }

    out.fullAvg = accumPath / static_cast<float>(segments);
    if (segments < 3) out.shortAvg = out.fullAvg;

    m_speed = out;
}

// ══════════════════════════════════════════════
// Step 4: Dynamic IIR coefficient (TSACore: GetIIRCoef)
// Speed-driven linear interpolation between lo and hi
// ══════════════════════════════════════════════
float CoorPostProcessor::StepCalcIIRCoef(bool isHover, bool isEdge) {
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

    // Linear interpolation between low and high speed thresholds
    float speed = m_speed.instant;
    float coef;
    if (speed <= thrLo) {
        coef = lo;
    } else if (speed >= thrHi) {
        coef = hi;
    } else {
        const float t = (speed - thrLo) / (thrHi - thrLo);
        coef = lo + t * (hi - lo);
    }

    m_lastIirCoef = coef;
    return coef;
}

// ══════════════════════════════════════════════
// Step 5: IIR filter (TSACore: CoorFilterProcess)
// 1st-order IIR low-pass: out = prev*(1-coef) + cur*coef
// ══════════════════════════════════════════════
AsaCoorResult CoorPostProcessor::StepIIR(
        const AsaCoorResult& cur, float coef) {
    if (!m_initialized) {
        m_iirDim1 = static_cast<float>(cur.dim1);
        m_iirDim2 = static_cast<float>(cur.dim2);
        m_initialized = true;
        return cur;
    }
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
// Step 6: Jitter offset compensation (TSACore: AftCoorProcess)
// ══════════════════════════════════════════════
AsaCoorResult CoorPostProcessor::StepJitter(
        const AsaCoorResult& cur, bool isEdge) {
    const int32_t thr = isEdge ? jitterEdgeThreshold
                               : jitterCenterThreshold;

    if (!m_jitterActive) {
        m_anchor = cur;
        m_offsetDim1 = 0;
        m_offsetDim2 = 0;
        m_jitterActive = true;
        return cur;
    }

    const int32_t dx = cur.dim1 - m_anchor.dim1;
    const int32_t dy = cur.dim2 - m_anchor.dim2;

    if (std::abs(dx) <= thr && std::abs(dy) <= thr) {
        m_offsetDim1 = m_anchor.dim1 - cur.dim1;
        m_offsetDim2 = m_anchor.dim2 - cur.dim2;
    }

    AsaCoorResult out = cur;
    out.dim1 = cur.dim1 + m_offsetDim1;
    out.dim2 = cur.dim2 + m_offsetDim2;
    return out;
}

// ══════════════════════════════════════════════
// Update 3-point history (after full chain)
// ══════════════════════════════════════════════
void CoorPostProcessor::StepUpdate3PtHistory(
        const AsaCoorResult& result) {
    m_prev[1] = m_prev[0];
    m_prev[0] = result;
}

} // namespace Asa
