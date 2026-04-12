#pragma once
// ── TouchPipeline Module: GridIIRProcessor ──
// Header-only. Converted from Preprocessing/GridIIRProcessor.{h,cpp}.
// Dynamic threshold gated IIR with aggressive noise floor decay.

#include "EngineTypes.h"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <cstdint>

namespace Engine { namespace Touch {

class GridIIRProcessor {
public:
    bool m_enabled = true;

    // Dynamic Touch Gate
    float m_gateRatio = 0.10f;
    int m_gateStaticFloor = 200;

    // Low-signal decay IIR
    int m_decayWeight = 200;
    int m_decayStep = 80;
    int m_noiseFloorCutoff = 5;

    // Residual correction (temporal)
    bool  m_residualEnabled = false;
    float m_residualAlpha   = 0.3f;

    GridIIRProcessor() {
        std::memset(m_historyBuffer, 0, sizeof(m_historyBuffer));
    }

    inline bool Process(HeatmapFrame& frame) {
        if (!m_enabled) return true;

        if (!m_historyInitialized) {
            std::memcpy(m_historyBuffer, frame.heatmapMatrix,
                        sizeof(m_historyBuffer));
            m_historyInitialized = true;
            return true;
        }

        // Compute per-frame dynamic threshold
        int16_t frameMax = 0;
        for (int y = 0; y < 40; ++y)
            for (int x = 0; x < 60; ++x)
                if (frame.heatmapMatrix[y][x] > frameMax)
                    frameMax = frame.heatmapMatrix[y][x];

        const int16_t dynThreshold = static_cast<int16_t>(std::max(
            static_cast<int>(std::lround(frameMax * m_gateRatio)),
            m_gateStaticFloor));

        // Apply per-pixel
        for (int y = 0; y < 40; ++y) {
            for (int x = 0; x < 60; ++x) {
                int16_t current = frame.heatmapMatrix[y][x];
                int16_t history = m_historyBuffer[y][x];

                // Residual correction
                if (m_residualEnabled && history > current) {
                    int16_t residual = static_cast<int16_t>(
                        (history - current) * m_residualAlpha);
                    current = std::max<int16_t>(0, current - residual);
                }

                int16_t filtered = ApplyIIR(current, history, dynThreshold);
                frame.heatmapMatrix[y][x] = filtered;
                m_historyBuffer[y][x] = filtered;
            }
        }
        return true;
    }

private:
    bool m_historyInitialized = false;
    int16_t m_historyBuffer[40][60];

    inline int16_t ApplyIIR(int16_t current, int16_t history,
                            int16_t dynThreshold) {
        if (current >= dynThreshold) return current;

        int32_t val = (static_cast<int32_t>(m_decayWeight) * current
                     + (256 - static_cast<int32_t>(m_decayWeight)) * history) / 256;
        val = std::max(static_cast<int32_t>(0), val - m_decayStep);

        if (val < m_noiseFloorCutoff) return 0;
        return static_cast<int16_t>(val);
    }
};

}} // namespace Engine::Touch
