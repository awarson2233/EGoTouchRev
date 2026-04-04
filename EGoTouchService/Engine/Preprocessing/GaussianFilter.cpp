#include "GaussianFilter.h"
#include <algorithm>

namespace Engine {

GaussianFilter::GaussianFilter() {
    m_temp.resize(40 * 60);
}

GaussianFilter::~GaussianFilter() {}

bool GaussianFilter::Process(HeatmapFrame& frame) {
    if (!m_enabled) return true;

    const int numRows = 40;
    const int numCols = 60;

    // Copy original data to temp buffer
    for (int y = 0; y < numRows; ++y) {
        for (int x = 0; x < numCols; ++x) {
            m_temp[y * numCols + x] = frame.heatmapMatrix[y][x];
        }
    }

    // 可变中心权重的高斯核模型：
    // 1        2        1
    // 2 m_centerWeight  2
    // 1        2        1
    // Sum = 12 + m_centerWeight
    int32_t kernelSum = 12 + m_centerWeight;

    for (int y = 1; y < numRows - 1; ++y) {
        for (int x = 1; x < numCols - 1; ++x) {
            int32_t sum = 0;
            
            // Top row
            sum += m_temp[(y - 1) * numCols + (x - 1)] * 1;
            sum += m_temp[(y - 1) * numCols + (x    )] * 2;
            sum += m_temp[(y - 1) * numCols + (x + 1)] * 1;
            
            // Middle row
            sum += m_temp[(y    ) * numCols + (x - 1)] * 2;
            sum += m_temp[(y    ) * numCols + (x    )] * m_centerWeight;
            sum += m_temp[(y    ) * numCols + (x + 1)] * 2;
            
            // Bottom row
            sum += m_temp[(y + 1) * numCols + (x - 1)] * 1;
            sum += m_temp[(y + 1) * numCols + (x    )] * 2;
            sum += m_temp[(y + 1) * numCols + (x + 1)] * 1;

            frame.heatmapMatrix[y][x] = static_cast<int16_t>(sum / kernelSum);
        }
    }

    return true;
}

std::vector<ConfigParam> GaussianFilter::GetConfigSchema() const {
    std::vector<ConfigParam> schema = IFrameProcessor::GetConfigSchema();
    schema.push_back(ConfigParam("CenterWeight", "Center Kernel Weight",
        ConfigParam::Int, const_cast<int*>(&m_centerWeight), 1, 30));
    return schema;
}

} // namespace Engine
