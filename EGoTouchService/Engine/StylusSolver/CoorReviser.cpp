#include "CoorReviser.h"
#include <cmath>
#include <algorithm>

namespace Asa {

void CoorReviser::Reset() {
    m_initialized = false;
    m_iirDeltaDim1 = 0.0f;
    m_iirDeltaDim2 = 0.0f;
    m_prevValid = false;
}

AsaCoorResult CoorReviser::Revise(
        const AsaCoorResult& tx1,
        const AsaCoorResult& tx2) {
    // Default: pass through TX1 unchanged
    AsaCoorResult out = tx1;

    if (!enabled) return out;
    if (!tx1.valid) { Reset(); return out; }
    if (!tx2.valid) return out;  // TX2 unavailable → use TX1 as-is

    // ── Step 1: CoorReviseCalculation ──
    // Compute raw TX1-TX2 delta
    float rawDeltaDim1 = static_cast<float>(tx2.dim1 - tx1.dim1);
    float rawDeltaDim2 = static_cast<float>(tx2.dim2 - tx1.dim2);

    // Safety: if delta is too large, TX2 is unreliable
    if (std::abs(rawDeltaDim1) > maxDeltaThreshold ||
        std::abs(rawDeltaDim2) > maxDeltaThreshold) {
        return out;  // discard TX2 for this frame
    }

    // ── Step 2: IIR smooth the delta ──
    if (!m_initialized) {
        m_iirDeltaDim1 = rawDeltaDim1;
        m_iirDeltaDim2 = rawDeltaDim2;
        m_initialized = true;
    } else {
        const float a = std::clamp(deltaIirAlpha, 0.01f, 1.0f);
        m_iirDeltaDim1 = m_iirDeltaDim1 * (1.0f - a) + rawDeltaDim1 * a;
        m_iirDeltaDim2 = m_iirDeltaDim2 * (1.0f - a) + rawDeltaDim2 * a;
    }

    // ── Step 3: CoorReviseWork ──
    // Blend: revised = tx1 + alpha * smoothed_delta
    const float alpha = std::clamp(blendAlpha, 0.0f, 1.0f);
    out.dim1 = tx1.dim1 + static_cast<int32_t>(
        std::lround(alpha * m_iirDeltaDim1));
    out.dim2 = tx1.dim2 + static_cast<int32_t>(
        std::lround(alpha * m_iirDeltaDim2));

    m_prevValid = true;
    return out;
}

} // namespace Asa
