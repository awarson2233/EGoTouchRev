#pragma once

#include "IFrameProcessor.h"
#include <unordered_map>

namespace Engine {

class CoordinateFilter : public IFrameProcessor {
public:
    CoordinateFilter();
    ~CoordinateFilter() override = default;

    bool Process(HeatmapFrame& frame) override;
    std::string GetName() const override { return "Coordinate Filter (1 Euro)"; }

    std::vector<ConfigParam> GetConfigSchema() const override;
    void SaveConfig(std::ostream& out) const override;
    void LoadConfig(const std::string& key, const std::string& value) override;

private:
    struct FilterState {
        float x = 0.0f;
        float y = 0.0f;
        float dx = 0.0f;
        float dy = 0.0f;
        uint64_t lastTimestamp = 0;
        bool initialized = false;
    };

    float Alpha(float rate, float cutoff) const;

    std::unordered_map<int, FilterState> m_states;

    // 1 Euro Filter Parameters
    // minCutoff: 低速极限截止频率 — 越大越跟手（越少滞后），越小越平滑
    // beta:      速度自适应斜率   — 越大高速时截止频率提升越快（减少高速滞后）
    float m_minCutoff = 5.0f;   // Hz  (原 1.0: α≈0.05 过滤过强，改为 5.0: α≈0.21)
    float m_beta = 0.05f;       //     (原 0.007，改为 0.05 加快高速响应)
    float m_dCutoff = 1.0f;     // Hz  (导数截止，保持不变)
};

} // namespace Engine
