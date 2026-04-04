#include "CMFProcessor.h"
#include <algorithm>

namespace Engine {

bool CMFProcessor::Process(HeatmapFrame& frame) {
    if (!m_enabled || m_mode == DimensionMode::None) return true;

    if (m_mode == DimensionMode::RowWise || m_mode == DimensionMode::DualDim) {
        ProcessRowWise(frame);
    }
    
    if (m_mode == DimensionMode::ColumnWise || m_mode == DimensionMode::DualDim) {
        ProcessColumnWise(frame);
    }

    return true;
}

void CMFProcessor::ProcessRowWise(HeatmapFrame& frame) {
    for (int y = 0; y < 40; ++y) {
        int64_t rowSum = 0;
        int validCount = 0;

        // Step 1: Calculate the average of this row, excluding potential finger touches
        for (int x = 0; x < 60; ++x) {
            int16_t val = frame.heatmapMatrix[y][x];
            // If the signal is below the finger threshold, it's considered background "noise"
            if (val < m_exclusionThreshold && val > -m_exclusionThreshold) { 
                rowSum += val;
                validCount++;
            }
        }

        // Step 2: CMF filter this row if we have enough valid background nodes
        if (validCount > 0) {
            int16_t rowOffset = static_cast<int16_t>(rowSum / validCount);

            // Cap the correction offset to prevent aggressive overshooting
            rowOffset = std::clamp<int16_t>(rowOffset, -m_maxCorrection, m_maxCorrection);

            // Subtract the row-wide common mode offset
            for (int x = 0; x < 60; ++x) {
                // Ensure we don't underflow below standard short sizes, 
                // typically floor at 0 for touch systems, but we allow neg for diff matrices
                frame.heatmapMatrix[y][x] = frame.heatmapMatrix[y][x] - rowOffset;
            }
        }
    }
}

void CMFProcessor::ProcessColumnWise(HeatmapFrame& frame) {
    for (int x = 0; x < 60; ++x) {
        int64_t colSum = 0;
        int validCount = 0;

        // Step 1: Calculate the average of this column, excluding potential finger touches
        for (int y = 0; y < 40; ++y) {
            int16_t val = frame.heatmapMatrix[y][x];
            if (val < m_exclusionThreshold && val > -m_exclusionThreshold) { 
                colSum += val;
                validCount++;
            }
        }

        // Step 2: CMF filter this column
        if (validCount > 0) {
            int16_t colOffset = static_cast<int16_t>(colSum / validCount);
            colOffset = std::clamp<int16_t>(colOffset, -m_maxCorrection, m_maxCorrection);

            for (int y = 0; y < 40; ++y) {
                frame.heatmapMatrix[y][x] = frame.heatmapMatrix[y][x] - colOffset;
            }
        }
    }
}

std::vector<ConfigParam> CMFProcessor::GetConfigSchema() const {
    std::vector<ConfigParam> schema = IFrameProcessor::GetConfigSchema();
    schema.push_back(ConfigParam("ExclusionThreshold", "Exclusion Threshold",
        ConfigParam::Int, const_cast<int*>(&m_exclusionThreshold), 50, 2000));
    schema.push_back(ConfigParam("MaxCorrection", "Max Correction",
        ConfigParam::Int, const_cast<int*>(&m_maxCorrection), 10, 2000));
    return schema;
}

} // namespace Engine
