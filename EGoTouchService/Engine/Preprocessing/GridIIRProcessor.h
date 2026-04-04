#pragma once

#include "FramePipeline.h"
#include <string>
#include <vector>

namespace Engine {

// Grid IIR Processor v2
// Dynamic threshold gated IIR with aggressive noise floor decay.
// - High signal (>= dynamicThreshold) bypasses IIR entirely
// - Low signal gets fast-decay IIR to wash non-touch areas to zero
class GridIIRProcessor : public IFrameProcessor {
public:
    GridIIRProcessor();
    ~GridIIRProcessor() override = default;

    bool Process(HeatmapFrame& frame) override;
    std::string GetName() const override { return "Grid IIR Processor"; }

    std::vector<ConfigParam> GetConfigSchema() const override;
    
    void SaveConfig(std::ostream& out) const override;
    void LoadConfig(const std::string& key, const std::string& value) override;

private:
    // Dynamic Touch Gate
    float m_gateRatio = 0.10f;       // dynamicThreshold = frameMax * ratio
    int m_gateStaticFloor = 200;     // Static lower bound for threshold

    // Low-signal decay IIR
    int m_decayWeight = 200;         // IIR weight for low-signal pixels (0-256)
    int m_decayStep = 80;            // Extra per-frame subtract for fast zeroing
    int m_noiseFloorCutoff = 5;      // Hard cutoff: below this → 0

    // Residual correction (temporal)
    bool  m_residualEnabled = false;  // 残留补偿开关
    float m_residualAlpha   = 0.3f;   // 残留衰减系数 (0=不修正, 1=完全扣除)

    // History buffer
    bool m_historyInitialized = false;
    int16_t m_historyBuffer[40][60];

    int16_t ApplyIIR(int16_t current, int16_t history, int16_t dynThreshold);
};

} // namespace Engine
