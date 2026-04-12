#pragma once

#include "SolverTypes.h"
#include <cstdint>
#include <algorithm>

namespace Solvers {

// TSACore edge boundary indices (sensor grid limits)
// TSACore EC boundary values (physical sensor edges in grid units).
// These extend half a cell beyond the last grid node on each side.
// E.g. a 40×60 grid has nodes at [0,39]×[0,59],
// but the physical sensor edge is at [0,40]×[0,60].
struct EdgeBounds {
    float colMin = 0.0f;     // Left physical edge
    float colMax = 60.0f;    // Right physical edge (kCols)
    float rowMin = 0.0f;     // Top physical edge
    float rowMax = 40.0f;    // Bottom physical edge (kRows)
};

// Per-zone edge info collected by TZ_UpdateEdgeInfo during BFS
struct ZoneEdgeInfo {
    int outerColSigSum = 0;  // Signal sum at col==min/max
    int innerColSigSum = 0;  // Signal sum at col==min+1/max-1
    int outerRowSigSum = 0;  // Signal sum at row==min/max
    int innerRowSigSum = 0;  // Signal sum at row==min+1/max-1
    int16_t outerColMax = 0; // Max signal at outer col edge
    int16_t innerColMax = 0; // Max signal at inner col edge
    int16_t outerRowMax = 0; // Max signal at outer row edge
    int16_t innerRowMax = 0; // Max signal at inner row edge

    // Zone bounding box (set by flagMask & 4 path)
    uint8_t minCol = 255, maxCol = 0;
    uint8_t minRow = 255, maxRow = 0;

    // TZ_GetEdgeTouchedFlag result
    uint32_t edgeFlags = 0;  // 0x20=touches boundary, 0x80000=within 2px
};

// BFS-level grid limits (node indices, not physical edges)
static constexpr int kGridColMin = 0, kGridColMax = 59;
static constexpr int kGridRowMin = 0, kGridRowMax = 39;

// TZ_UpdateEdgeInfo — called per pixel during BFS
inline void TZ_UpdateEdgeInfo(ZoneEdgeInfo& ei,
                              int16_t signal, int col, int row,
                              uint8_t flagMask) {
    // X-axis (column) boundaries
    if (col == kGridColMin || col == kGridColMax) {
        ei.outerColSigSum += signal;
        if (flagMask & 1)
            ei.outerColMax = std::max(ei.outerColMax, signal);
    } else if (col == kGridColMin + 1 || col == kGridColMax - 1) {
        ei.innerColSigSum += signal;
        if (flagMask & 2)
            ei.innerColMax = std::max(ei.innerColMax, signal);
    }
    // Y-axis (row) boundaries
    if (row == kGridRowMin || row == kGridRowMax) {
        ei.outerRowSigSum += signal;
        if (flagMask & 1)
            ei.outerRowMax = std::max(ei.outerRowMax, signal);
    } else if (row == kGridRowMin + 1 || row == kGridRowMax - 1) {
        ei.innerRowSigSum += signal;
        if (flagMask & 2)
            ei.innerRowMax = std::max(ei.innerRowMax, signal);
    }
    // Core pixels: update bounding box
    if (flagMask & 4) {
        ei.minCol = std::min(ei.minCol, (uint8_t)col);
        ei.maxCol = std::max(ei.maxCol, (uint8_t)col);
        ei.minRow = std::min(ei.minRow, (uint8_t)row);
        ei.maxRow = std::max(ei.maxRow, (uint8_t)row);
    }
}

// TZ_GetEdgeTouchedFlag — call after BFS completes
inline void TZ_GetEdgeTouchedFlag(ZoneEdgeInfo& ei) {
    ei.edgeFlags = 0;
    if (ei.minCol <= kGridColMin || ei.maxCol >= kGridColMax ||
        ei.minRow <= kGridRowMin || ei.maxRow >= kGridRowMax) {
        ei.edgeFlags |= 0x20;   // touches boundary
    }
    if (ei.minCol < kGridColMin + 2 || ei.maxCol > kGridColMax - 2 ||
        ei.minRow < kGridRowMin + 2 || ei.maxRow > kGridRowMax - 2) {
        ei.edgeFlags |= 0x80000; // within 2px of boundary
    }
}

// ── CTD_EC LUT and helpers (from firmware) ──

struct ECSegment {
    uint8_t edgeWidthThreshold;
    uint8_t lutIdxLow;
    uint8_t lutIdxHigh;
};
struct ECProfile {
    uint8_t numSegments;
    ECSegment segments[4];
};

// CTD_ECProcess: apply edge compensation to contacts
class EdgeCompensator {
public:
    void Process(std::vector<TouchContact>& contacts,
                 const std::vector<ZoneEdgeInfo>& edgeInfos,
                 const EdgeBounds& bounds);

    bool m_enabled = true;
    EdgeBounds m_bounds;
    // How far (in grid units) from the 1-grid-unit boundary the EC
    // blending zone extends.  Original firmware uses 0.25; we default
    // to 2.0 so centroids at ~58 still get pushed toward 60.
    float m_ecBlendRange = 2.0f;

private:
    void ProcessDim(float& coord, float boundNear, float boundFar,
                    uint32_t edgeFlags, const ZoneEdgeInfo& ei,
                    bool isDimX);
};

} // namespace Solvers
