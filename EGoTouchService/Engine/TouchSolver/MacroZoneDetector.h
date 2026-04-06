#pragma once

#include "EngineTypes.h"
#include <vector>

namespace Engine {

class MacroZoneDetector {
public:
    static constexpr int kRows = 40;
    static constexpr int kCols = 60;

    void Process(const HeatmapFrame& frame, int threshold);

    const std::vector<MacroZone>& GetMacroZones() const { return m_macroZones; }
    std::vector<MacroZone>& GetMutableMacroZones() { return m_macroZones; }

private:
    std::vector<MacroZone> m_macroZones;
    bool m_visited[kRows * kCols];
};

} // namespace Engine
