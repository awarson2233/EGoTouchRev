#pragma once

#include "FramePipeline.h"
#include <string>

namespace Engine {

// CMF Processor (Common Mode Filter)
// Replicates TSACore's original CMF_ProcessDim logic to remove global row/column 
// shift noise induced by chargers or LCD coupling.
class CMFProcessor : public IFrameProcessor {
public:
    enum class DimensionMode {
        None = 0,
        RowWise = 1,     // Processes each row independently
        ColumnWise = 2,  // Processes each column independently
        DualDim = 3      // Processes both Row and Column (2D-like approximation)
    };

    CMFProcessor() = default;
    ~CMFProcessor() override = default;

    bool Process(HeatmapFrame& frame) override;
    std::string GetName() const override { return "CMF Processor"; }

    std::vector<ConfigParam> GetConfigSchema() const override;
    
    void SaveConfig(std::ostream& out) const override {
        IFrameProcessor::SaveConfig(out);
        out << "DimensionMode=" << static_cast<int>(m_mode) << "\n";
        out << "ExclusionThreshold=" << m_exclusionThreshold << "\n";
        out << "MaxCorrection=" << m_maxCorrection << "\n";
    }
    
    void LoadConfig(const std::string& key, const std::string& value) override {
        IFrameProcessor::LoadConfig(key, value);
        if (key == "DimensionMode") m_mode = static_cast<DimensionMode>(std::stoi(value));
        else if (key == "ExclusionThreshold") m_exclusionThreshold = static_cast<int16_t>(std::stoi(value));
        else if (key == "MaxCorrection") m_maxCorrection = static_cast<int16_t>(std::stoi(value));
    }

private:
    void ProcessRowWise(HeatmapFrame& frame);
    void ProcessColumnWise(HeatmapFrame& frame);

    DimensionMode m_mode = DimensionMode::RowWise; 
    
    // Threshold above which a pixel is considered part of a finger touch,
    // and thus excluded from the common-mode noise calculation.
    int m_exclusionThreshold = 250; 
    int m_maxCorrection = 500;
};

} // namespace Engine
