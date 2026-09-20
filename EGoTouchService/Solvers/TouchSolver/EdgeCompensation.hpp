#pragma once

#include "SolverTypes.h"
#include "TouchFrameTypes.h"
#include "TouchSharedTypes.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace Solvers {

// EdgeBounds 和 ZoneEdgeInfo 已迁移到 TouchSharedTypes.h

// BFS-level grid limits (node indices, not physical edges)
static constexpr int kGridColMin = 0, kGridColMax = 59;
static constexpr int kGridRowMin = 0, kGridRowMax = 39;

// TZ_UpdateEdgeInfo — called per pixel during BFS
inline void TZ_UpdateEdgeInfo(ZoneEdgeInfo& ei,
                              int16_t signal, int col, int row,
                              uint8_t flagMask) {
    if (col == kGridColMin || col == kGridColMax) {
        ei.outerColSigSum += signal;
        if (flagMask & 1)
            ei.outerColMax = std::max(ei.outerColMax, signal);
        if (flagMask & 4) {
            const bool firstOuterCol = ei.minRowOnOuterCol == 255;
            if (firstOuterCol || row < ei.minRowOnOuterCol) {
                ei.minRowOnOuterCol = static_cast<uint8_t>(row);
                ei.colAtMinOuterRow = static_cast<uint8_t>(col);
            }
            if (firstOuterCol || row > ei.maxRowOnOuterCol) {
                ei.maxRowOnOuterCol = static_cast<uint8_t>(row);
                ei.colAtMaxOuterRow = static_cast<uint8_t>(col);
            }
        }
    } else if (col == kGridColMin + 1 || col == kGridColMax - 1) {
        ei.innerColSigSum += signal;
        if (flagMask & 2)
            ei.innerColMax = std::max(ei.innerColMax, signal);
    }
    if (row == kGridRowMin || row == kGridRowMax) {
        ei.outerRowSigSum += signal;
        if (flagMask & 1)
            ei.outerRowMax = std::max(ei.outerRowMax, signal);
        if (flagMask & 4) {
            const bool firstOuterRow = ei.minColOnOuterRow == 255;
            if (firstOuterRow || col < ei.minColOnOuterRow) {
                ei.minColOnOuterRow = static_cast<uint8_t>(col);
                ei.rowAtMinOuterCol = static_cast<uint8_t>(row);
            }
            if (firstOuterRow || col > ei.maxColOnOuterRow) {
                ei.maxColOnOuterRow = static_cast<uint8_t>(col);
                ei.rowAtMaxOuterCol = static_cast<uint8_t>(row);
            }
        }
    } else if (row == kGridRowMin + 1 || row == kGridRowMax - 1) {
        ei.innerRowSigSum += signal;
        if (flagMask & 2)
            ei.innerRowMax = std::max(ei.innerRowMax, signal);
    }
    if (flagMask & 4) {
        ei.minCol = std::min(ei.minCol, static_cast<uint8_t>(col));
        ei.maxCol = std::max(ei.maxCol, static_cast<uint8_t>(col));
        ei.minRow = std::min(ei.minRow, static_cast<uint8_t>(row));
        ei.maxRow = std::max(ei.maxRow, static_cast<uint8_t>(row));
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

inline uint8_t TZ_GetCentroidEdgeFlags(const ZoneEdgeInfo& ei, float col, float row) {
    uint8_t flags = 0;
    if (ei.minCol <= kGridColMin && col < static_cast<float>(kGridColMin + 1)) flags |= 0x01;
    if (ei.maxCol >= kGridColMax && col > static_cast<float>(kGridColMax)) flags |= 0x02;
    if (ei.minRow <= kGridRowMin && row < static_cast<float>(kGridRowMin + 1)) flags |= 0x04;
    if (ei.maxRow >= kGridRowMax && row > static_cast<float>(kGridRowMax)) flags |= 0x08;
    return flags;
}

template <size_t Rows, size_t Cols>
inline void TZ_GetEdgeWidth(ZoneEdgeInfo& ei,
                            const int16_t (&heatmap)[Rows][Cols],
                            int16_t threshold) {
    if (ei.minColOnOuterRow <= ei.maxColOnOuterRow) {
        int left = ei.minColOnOuterRow;
        int right = ei.maxColOnOuterRow;
        int scannedLeft = left;
        int scannedRight = right;
        while (right < static_cast<int>(Cols) && heatmap[ei.rowAtMaxOuterCol][right] >= threshold) {
            scannedRight = right;
            ++right;
        }
        while (left >= 0 && heatmap[ei.rowAtMinOuterCol][left] >= threshold) {
            scannedLeft = left;
            --left;
        }
        ei.rowEdgeWidth = static_cast<uint8_t>(std::clamp(scannedRight - scannedLeft + 1, 0, 255));
    } else {
        ei.rowEdgeWidth = 0;
    }

    if (ei.minRowOnOuterCol <= ei.maxRowOnOuterCol) {
        int top = ei.minRowOnOuterCol;
        int bottom = ei.maxRowOnOuterCol;
        int scannedTop = top;
        int scannedBottom = bottom;
        while (bottom < static_cast<int>(Rows) && heatmap[bottom][ei.colAtMaxOuterRow] >= threshold) {
            scannedBottom = bottom;
            ++bottom;
        }
        while (top >= 0 && heatmap[top][ei.colAtMinOuterRow] >= threshold) {
            scannedTop = top;
            --top;
        }
        ei.colEdgeWidth = static_cast<uint8_t>(std::clamp(scannedBottom - scannedTop + 1, 0, 255));
    } else {
        ei.colEdgeWidth = 0;
    }
}

} // namespace Solvers
