#pragma once
// ── TouchPipeline Module: MacroZoneDetector ──
// Header-only. Faithful replica of TouchSolver/MacroZoneDetector.{h,cpp}.
// BFS 8-connected component labeling on the heatmap.

#include "EngineTypes.h"
#include <vector>
#include <queue>
#include <cstdint>
#include <cstring>

namespace Engine { namespace Touch {

class MacroZoneDetector {
public:
    static constexpr int kRows = 40;
    static constexpr int kCols = 60;

    inline void Process(const HeatmapFrame& frame, int threshold) {
        m_macroZones.clear();
        std::memset(m_visited, 0, sizeof(m_visited));

        int dr[] = {-1, 1, 0, 0, -1, -1, 1, 1};
        int dc[] = {0, 0, -1, 1, -1, 1, -1, 1};

        for (int r = 0; r < kRows; ++r) {
            for (int c = 0; c < kCols; ++c) {
                int idx = r * kCols + c;
                if (m_visited[idx]) continue;

                if (frame.heatmapMatrix[r][c] >= threshold) {
                    MacroZone zone;
                    std::queue<int> q;

                    q.push(idx);
                    m_visited[idx] = true;

                    while (!q.empty()) {
                        int currIdx = q.front();
                        q.pop();

                        zone.pixels.push_back(currIdx);
                        zone.area++;

                        int currR = currIdx / kCols;
                        int currC = currIdx % kCols;

                        for (int d = 0; d < 8; ++d) {
                            int nr = currR + dr[d];
                            int nc = currC + dc[d];

                            if (nr >= 0 && nr < kRows && nc >= 0 && nc < kCols) {
                                int nIdx = nr * kCols + nc;
                                if (!m_visited[nIdx] && frame.heatmapMatrix[nr][nc] >= threshold) {
                                    m_visited[nIdx] = true;
                                    q.push(nIdx);
                                }
                            }
                        }
                    }

                    if (zone.area > 0) {
                        m_macroZones.push_back(std::move(zone));
                    }
                }
            }
        }
    }

    const std::vector<MacroZone>& GetMacroZones() const { return m_macroZones; }
    std::vector<MacroZone>& GetMutableMacroZones() { return m_macroZones; }

private:
    std::vector<MacroZone> m_macroZones;
    bool m_visited[kRows * kCols] = {};
};

}} // namespace Engine::Touch
