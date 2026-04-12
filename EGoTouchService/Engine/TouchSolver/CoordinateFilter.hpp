#pragma once
// ── TouchPipeline Module: CoordinateFilter (1-Euro) ──
// Header-only. Converted from TouchSolver/CoordinateFilter.{h,cpp}.

#include "EngineTypes.h"
#include <unordered_map>
#include <cmath>
#include <vector>
#include <algorithm>

namespace Engine { namespace Touch {

class CoordinateFilter {
public:
    bool  m_enabled = true;
    float m_minCutoff = 5.0f;
    float m_beta = 0.05f;
    float m_dCutoff = 1.0f;

    inline bool Process(HeatmapFrame& frame) {
        if (!m_enabled) return true;

        const uint64_t currentTimestamp = frame.timestamp;
        std::vector<int> activeIds;
        activeIds.reserve(frame.contacts.size());

        for (auto& contact : frame.contacts) {
            if (contact.id <= 0) continue;
            activeIds.push_back(contact.id);
            auto& state = m_states[contact.id];

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
            const float cutoff = m_minCutoff + m_beta * velocityMag;
            const float alpha = Alpha(rate, cutoff);

            state.x = state.x + alpha * (contact.x - state.x);
            state.y = state.y + alpha * (contact.y - state.y);
            state.lastTimestamp = currentTimestamp;

            contact.x = state.x;
            contact.y = state.y;
        }

        // Cleanup states for inactive touches
        for (auto it = m_states.begin(); it != m_states.end(); ) {
            if (std::find(activeIds.begin(), activeIds.end(), it->first) == activeIds.end())
                it = m_states.erase(it);
            else
                ++it;
        }
        return true;
    }

private:
    static constexpr float kPi = 3.14159265358979323846f;

    struct FilterState {
        float x = 0, y = 0, dx = 0, dy = 0;
        uint64_t lastTimestamp = 0;
        bool initialized = false;
    };

    std::unordered_map<int, FilterState> m_states;

    inline float Alpha(float rate, float cutoff) const {
        const float tau = 1.0f / (2.0f * kPi * cutoff);
        return 1.0f / (1.0f + tau * rate);
    }
};

}} // namespace Engine::Touch
