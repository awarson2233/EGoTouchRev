#pragma once
#include "IFrameProcessor.h"

namespace Engine {

class BaselineSubtraction : public IFrameProcessor {
public:
    bool Process(HeatmapFrame& frame) override;
    std::string GetName() const override { return "Baseline Subtraction"; }
    std::vector<ConfigParam> GetConfigSchema() const override;
    void LoadConfig(const std::string& key, const std::string& value) override;
    void SaveConfig(std::ostream& out) const override;

private:
    int m_baseline = 0x7FFE; // Default baseline (32766)
};

} // namespace Engine
