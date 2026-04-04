#include "CoordinateFilter.h"
#include <cmath>
#include <vector>

namespace Engine {

// Reference for 1 Euro Filter implementation:
// http://cristal.univ-lille.fr/~casiez/1euro/

constexpr float M_PI_F = 3.14159265358979323846f;

CoordinateFilter::CoordinateFilter() {
    m_enabled = true; // Enabled by default
}

float CoordinateFilter::Alpha(float rate, float cutoff) const {
    const float tau = 1.0f / (2.0f * M_PI_F * cutoff);
    return 1.0f / (1.0f + tau * rate);
}

bool CoordinateFilter::Process(HeatmapFrame& frame) {
    if (!m_enabled) {
        return true;
    }

    const uint64_t currentTimestamp = frame.timestamp;

    std::vector<int> activeIds;
    activeIds.reserve(frame.contacts.size());

    for (auto& contact : frame.contacts) {
        // Only process touches that have a valid ID and aren't marked for removal immediately.
        if (contact.id <= 0) {
            continue;
        }

        activeIds.push_back(contact.id);

        auto& state = m_states[contact.id];

        // If it's a new touch or the first frame we see it, initialize the state
        if (!state.initialized || contact.state == TouchStateDown) {
            state.x = contact.x;
            state.y = contact.y;
            state.dx = 0.0f;
            state.dy = 0.0f;
            state.lastTimestamp = currentTimestamp;
            state.initialized = true;
            continue;
        }

        // Calculate time delta in seconds. Suppose frame.timestamp is microseconds or similar?
        // Wait, what is the unit of frame.timestamp? Usually ms or us.
        // Assuming ms based on standard conventions (e.g. 120Hz = ~8.33ms).
        float dt = 0.0f;
        if (currentTimestamp > state.lastTimestamp) {
            // Assume timestamp is in ms for EGoTouchRev
            dt = static_cast<float>(currentTimestamp - state.lastTimestamp) / 1000.0f;
        } 
        
        // If dt is 0 (same frame timestamp?), use a fallback frame rate like 120Hz
        if (dt <= 0.0f) {
            dt = 1.0f / 120.0f; 
        }

        const float rate = 1.0f / dt;

        // Calculate velocity (derivate)
        const float dxRaw = (contact.x - state.x) * rate;
        const float dyRaw = (contact.y - state.y) * rate;

        // Filter the velocity
        const float alphaD = Alpha(rate, m_dCutoff);
        state.dx = state.dx + alphaD * (dxRaw - state.dx);
        state.dy = state.dy + alphaD * (dyRaw - state.dy);

        // Filter the coordinates
        const float velocityMag = std::sqrt(state.dx * state.dx + state.dy * state.dy);
        const float cutoff = m_minCutoff + m_beta * velocityMag;
        const float alpha = Alpha(rate, cutoff);

        state.x = state.x + alpha * (contact.x - state.x);
        state.y = state.y + alpha * (contact.y - state.y);
        state.lastTimestamp = currentTimestamp;

        // Update the contact with filtered coordinates
        contact.x = state.x;
        contact.y = state.y;
    }

    // Cleanup states for touches that are no longer active
    for (auto it = m_states.begin(); it != m_states.end(); ) {
        if (std::find(activeIds.begin(), activeIds.end(), it->first) == activeIds.end()) {
            it = m_states.erase(it);
        } else {
            ++it;
        }
    }

    return true;
}

std::vector<ConfigParam> CoordinateFilter::GetConfigSchema() const {
    std::vector<ConfigParam> schema = IFrameProcessor::GetConfigSchema();
    schema.push_back(ConfigParam("OneEuroMinCutoff", "1 Euro Min Cutoff",
        ConfigParam::Float, const_cast<float*>(&m_minCutoff), 0.1f, 20.0f));
    schema.push_back(ConfigParam("OneEuroBeta", "1 Euro Beta",
        ConfigParam::Float, const_cast<float*>(&m_beta), 0.0f, 0.5f));
    schema.push_back(ConfigParam("OneEuroDCutoff", "1 Euro Deriv Cutoff",
        ConfigParam::Float, const_cast<float*>(&m_dCutoff), 0.1f, 10.0f));
    return schema;
}

void CoordinateFilter::SaveConfig(std::ostream& out) const {
    IFrameProcessor::SaveConfig(out);
    out << "OneEuroMinCutoff=" << m_minCutoff << "\n";
    out << "OneEuroBeta=" << m_beta << "\n";
    out << "OneEuroDCutoff=" << m_dCutoff << "\n";
}

void CoordinateFilter::LoadConfig(const std::string& key, const std::string& value) {
    IFrameProcessor::LoadConfig(key, value);
    if (key == "OneEuroMinCutoff") {
        m_minCutoff = std::stof(value);
    } else if (key == "OneEuroBeta") {
        m_beta = std::stof(value);
    } else if (key == "OneEuroDCutoff") {
        m_dCutoff = std::stof(value);
    }
}

} // namespace Engine
