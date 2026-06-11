#pragma once
// ── TouchPipeline Module: CoordinateFilter (1-Euro) ──
// Header-only. Converted from TouchSolver/CoordinateFilter.{h,cpp}.

#include "SolverTypes.h"
#include <array>
#include <cmath>

namespace Solvers { namespace Touch {

class CoordinateFilter {
public:
    bool  m_enabled = true;
    float m_minCutoff = 1.0f;   // 降低静止时的截止频率（原 20.0f），大幅增强静止时的去抖动能力
    float m_beta = 150.0f;      // 增大速度斜率（原 50.0f），一旦移动，截止频率迅速升高，减少滤波迟滞
    float m_dCutoff = 100.0f;

    inline bool Process(HeatmapFrame& frame) {
        if (!m_enabled) return true;

        const uint64_t currentTimestamp = frame.timestamp;
        m_activeMask.fill(0);

        for (auto& contact : frame.touch.output.contacts) {
            if (contact.id <= 0 || contact.id > kMaxTouchIds) continue;
            m_activeMask[static_cast<size_t>(contact.id)] = 1;
            auto& state = m_states[static_cast<size_t>(contact.id)];

            if (!state.initialized || contact.state == TouchStateDown) {
                state.x = contact.x;
                state.y = contact.y;
                state.dx = 0.0f;
                state.dy = 0.0f;
                state.lastTimestamp = currentTimestamp;
                state.initialized = true;
                continue;
            }

            float dt = 0.0f;
            if (currentTimestamp > state.lastTimestamp)
                dt = static_cast<float>(currentTimestamp - state.lastTimestamp) / 1000.0f;
            if (dt <= 0.0f) dt = 1.0f / 120.0f;

            const float rate = 1.0f / dt;
            const float dxRaw = (contact.x - state.x) * rate;
            const float dyRaw = (contact.y - state.y) * rate;
            const float alphaD = Alpha(rate, m_dCutoff);
            state.dx = state.dx + alphaD * (dxRaw - state.dx);
            state.dy = state.dy + alphaD * (dyRaw - state.dy);

            const float velocityMag = std::sqrt(state.dx * state.dx + state.dy * state.dy);
            // 采用非线性响应：手指开始移动时，速度平方（或非线性因子）会使 cutoff 呈指数/二次级增长，迅速降低滤波
            // 使得一开始移动，滤波效果大幅降低，保持高灵敏度；静止时速度极小，几乎完全由 m_minCutoff 控制，去抖能力极强
            const float cutoff = m_minCutoff + m_beta * (velocityMag * velocityMag);
            const float alpha = Alpha(rate, cutoff);

            state.x = state.x + alpha * (contact.x - state.x);
            state.y = state.y + alpha * (contact.y - state.y);
            state.lastTimestamp = currentTimestamp;

            contact.x = state.x;
            contact.y = state.y;
        }

        // Cleanup states for inactive touches
        for (int id = 1; id <= kMaxTouchIds; ++id) {
            if (m_activeMask[static_cast<size_t>(id)] == 0) {
                m_states[static_cast<size_t>(id)] = FilterState{};
            }
        }
        return true;
    }

private:
    static constexpr float kPi = 3.14159265358979323846f;
    static constexpr int kMaxTouchIds = 20;

    struct FilterState {
        float x = 0, y = 0, dx = 0, dy = 0;
        uint64_t lastTimestamp = 0;
        bool initialized = false;
    };

    std::array<FilterState, kMaxTouchIds + 1> m_states{};
    std::array<uint8_t, kMaxTouchIds + 1> m_activeMask{};

    inline float Alpha(float rate, float cutoff) const {
        const float tau = 1.0f / (2.0f * kPi * cutoff);
        return 1.0f / (1.0f + tau * rate);
    }
};

}} // namespace Solvers::Touch
