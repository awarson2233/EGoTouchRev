#pragma once
#include "IFrameProcessor.h"
#include <vector>

namespace Engine {

class GaussianFilter : public IFrameProcessor {
public:
    GaussianFilter();
    ~GaussianFilter() override;

    bool Process(HeatmapFrame& frame) override;
    std::string GetName() const override { return "3x3 Gaussian Filter"; }

    std::vector<ConfigParam> GetConfigSchema() const override;

private:
    std::vector<int16_t> m_temp;
    
    // 中心权重。标准的高斯是 4 (周边是 2 和 1)。
    // 调大这个值，会使得模糊效果减弱，保留更多原信号的尖锐度。
    int m_centerWeight = 20;
};

} // namespace Engine
