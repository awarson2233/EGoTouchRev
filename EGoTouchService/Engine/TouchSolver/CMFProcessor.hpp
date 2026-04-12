#pragma once
// ── TouchPipeline Module: CMFProcessor ──
// Header-only. Converted from Preprocessing/CMFProcessor.{h,cpp}.
// Common Mode Filter: removes global row/column shift noise.

#include "EngineTypes.h"
#include <algorithm>
#include <cstdint>

namespace Engine { namespace Touch {

class CMFProcessor {
public:
    enum class DimensionMode {
        None = 0, RowWise = 1, ColumnWise = 2, DualDim = 3
    };

    bool m_enabled = true;
    DimensionMode m_mode = DimensionMode::RowWise;
    int m_exclusionThreshold = 250;
    int m_maxCorrection = 500;

    inline bool Process(HeatmapFrame& frame) {
        if (!m_enabled || m_mode == DimensionMode::None) return true;
        if (m_mode == DimensionMode::RowWise || m_mode == DimensionMode::DualDim)
            ProcessRowWise(frame);
        if (m_mode == DimensionMode::ColumnWise || m_mode == DimensionMode::DualDim)
            ProcessColumnWise(frame);
        return true;
    }

private:
    inline void ProcessRowWise(HeatmapFrame& frame) {
        for (int y = 0; y < 40; ++y) {
            int64_t rowSum = 0;
            int validCount = 0;
            for (int x = 0; x < 60; ++x) {
                int16_t val = frame.heatmapMatrix[y][x];
                if (val < m_exclusionThreshold && val > -m_exclusionThreshold) {
                    rowSum += val;
                    validCount++;
                }
            }
            if (validCount > 0) {
                int16_t rowOffset = static_cast<int16_t>(rowSum / validCount);
                rowOffset = std::clamp<int16_t>(rowOffset,
                    static_cast<int16_t>(-m_maxCorrection),
                    static_cast<int16_t>(m_maxCorrection));
                for (int x = 0; x < 60; ++x)
                    frame.heatmapMatrix[y][x] -= rowOffset;
            }
        }
    }

    inline void ProcessColumnWise(HeatmapFrame& frame) {
        for (int x = 0; x < 60; ++x) {
            int64_t colSum = 0;
            int validCount = 0;
            for (int y = 0; y < 40; ++y) {
                int16_t val = frame.heatmapMatrix[y][x];
                if (val < m_exclusionThreshold && val > -m_exclusionThreshold) {
                    colSum += val;
                    validCount++;
                }
            }
            if (validCount > 0) {
                int16_t colOffset = static_cast<int16_t>(colSum / validCount);
                colOffset = std::clamp<int16_t>(colOffset,
                    static_cast<int16_t>(-m_maxCorrection),
                    static_cast<int16_t>(m_maxCorrection));
                for (int y = 0; y < 40; ++y)
                    frame.heatmapMatrix[y][x] -= colOffset;
            }
        }
    }
};

}} // namespace Engine::Touch
