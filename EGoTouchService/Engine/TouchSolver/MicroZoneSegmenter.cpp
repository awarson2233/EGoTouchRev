#include "MicroZoneSegmenter.h"
#include <queue>
#include <cstring>

namespace Engine {

void MicroZoneSegmenter::Process(const HeatmapFrame& frame, const std::vector<MacroZone>& macroZones, const std::vector<Peak>& peaks) {
    m_peakZones.fill(0);

    // Create a boolean map of all valid grid cells that belong to ANY MacroZone.
    bool validPixels[2400] = {false};
    for (const auto& mz : macroZones) {
        for (int idx : mz.pixels) {
            validPixels[idx] = true;
        }
    }

    // Priority queue to traverse the highest signals first (Watershed)
    struct Node {
         int idx;
         int16_t sig;
         uint8_t zoneId;
         bool operator<(const Node& o) const { return sig < o.sig; }
    };
    std::priority_queue<Node> q;

    // Seed the queue with all peaks
    for (size_t i = 0; i < peaks.size(); ++i) {
        int idx = peaks[i].r * kCols + peaks[i].c;
        uint8_t zoneId = static_cast<uint8_t>(i + 1); // 1-indexed

        // Only seed if the peak is actually inside a valid macro zone 
        // (which it should be based on our new pipeline)
        if (validPixels[idx]) {
            m_peakZones[idx] = zoneId;
            q.push({idx, frame.heatmapMatrix[peaks[i].r][peaks[i].c], zoneId});
        }
    }

    int dr[] = {-1, 1, 0, 0, -1, -1, 1, 1};
    int dc[] = {0, 0, -1, 1, -1, 1, -1, 1};

    // Run steepest ascent / priority flood fill
    while (!q.empty()) {
        Node curr = q.top();
        q.pop();

        int r = curr.idx / kCols;
        int c = curr.idx % kCols;

        for (int d = 0; d < 8; ++d) {
            int nr = r + dr[d];
            int nc = c + dc[d];
            
            if (nr >= 0 && nr < kRows && nc >= 0 && nc < kCols) {
                int nIdx = nr * kCols + nc;
                
                // If it's part of a MacroZone and not yet claimed by any peak
                if (validPixels[nIdx] && m_peakZones[nIdx] == 0) {
                    m_peakZones[nIdx] = curr.zoneId;
                    q.push({nIdx, frame.heatmapMatrix[nr][nc], curr.zoneId});
                }
            }
        }
    }
}

} // namespace Engine
