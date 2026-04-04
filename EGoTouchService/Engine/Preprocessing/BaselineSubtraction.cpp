#include "BaselineSubtraction.h"
#include <cstring>

namespace Engine {

bool BaselineSubtraction::Process(HeatmapFrame& frame) {
    if (!m_enabled) return true;

    int16_t* ptr = reinterpret_cast<int16_t*>(frame.heatmapMatrix);

    int16_t base = static_cast<int16_t>(m_baseline);

    // 使用标准 C++ 循环以解决潜在的 SSE/NEON 地址未对齐异常
    for(int i = 0; i < 2400; ++i) {
        ptr[i] = ptr[i] - base;
    }

    return true;
}

std::vector<ConfigParam> BaselineSubtraction::GetConfigSchema() const {
    std::vector<ConfigParam> schema = IFrameProcessor::GetConfigSchema();
    schema.push_back(ConfigParam("BaselineValue", "Baseline Value",
        ConfigParam::Int, const_cast<int*>(&m_baseline), 0, 65535));
    return schema;
}

void BaselineSubtraction::LoadConfig(const std::string& key, const std::string& value) {
    if (key == "BaselineValue") {
        try {
            m_baseline = std::stoi(value);
        } catch (...) {
            m_baseline = 0x7FFE;
        }
    }
}

void BaselineSubtraction::SaveConfig(std::ostream& out) const {
    out << "BaselineValue=" << m_baseline << "\n";
}

} // namespace Engine
