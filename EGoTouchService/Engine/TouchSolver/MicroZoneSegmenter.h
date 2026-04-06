#pragma once

#include "EngineTypes.h"
#include "PeakDetector.h"
#include <vector>
#include <array>

namespace Engine {

class MicroZoneSegmenter {
public:
    static constexpr int kRows = 40;
    static constexpr int kCols = 60;

    void Process(const HeatmapFrame& frame, const std::vector<MacroZone>& macroZones, const std::vector<Peak>& peaks);

    const std::array<uint8_t, 2400>& GetPeakZones() const { return m_peakZones; }

private:
    std::array<uint8_t, 2400> m_peakZones{};
};

} // namespace Engine
