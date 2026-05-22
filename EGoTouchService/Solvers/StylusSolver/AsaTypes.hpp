#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace Asa {

enum class StylusFrameClass : uint8_t {
    Valid,
    ShortFrame,
    NoSignal,
    ParseFail,
    Tx1Missing,
};

// Slave frame constants.
static constexpr int kSlaveHeaderBytes = 7;
static constexpr int kBlockWords = 83;
static constexpr int kGridDim = 9;
static constexpr int kGridSize = kGridDim * kGridDim;
static constexpr int kCoorUnit = 0x400;
static constexpr uint16_t kAnchorInvalid = 0x00FF;

struct FreqBlock {
    uint16_t anchorRow = kAnchorInvalid;
    uint16_t anchorCol = kAnchorInvalid;
    int16_t grid[kGridDim][kGridDim]{};
    bool valid = false;

    void Clear() {
        anchorRow = kAnchorInvalid;
        anchorCol = kAnchorInvalid;
        std::memset(grid, 0, sizeof(grid));
        valid = false;
    }
};

struct AsaGridData {
    FreqBlock tx1;
    FreqBlock tx2;

    void Clear() {
        tx1.Clear();
        tx2.Clear();
    }
};

struct AsaProjection {
    int32_t dim1[kGridDim]{};
    int32_t dim2[kGridDim]{};
    int peakIdxDim1 = -1;
    int peakIdxDim2 = -1;
    int spanDim1 = 0;
    int spanDim2 = 0;

    void Clear() {
        std::memset(dim1, 0, sizeof(dim1));
        std::memset(dim2, 0, sizeof(dim2));
        peakIdxDim1 = -1;
        peakIdxDim2 = -1;
        spanDim1 = 0;
        spanDim2 = 0;
    }
};

struct GridPeakUnit {
    int peakRow = -1;
    int peakCol = -1;
    int32_t peakValue = 0;
    int32_t neighborSum3x3 = 0;
    int connectedPixels = 0;
    bool valid = false;
};

struct GridPeakRegion {
    int peakRow = -1;
    int peakCol = -1;
    int32_t peakValue = 0;
    int32_t regionSum = 0;
    int32_t sum3x3 = 0;
    int minRow = 0;
    int maxRow = 0;
    int minCol = 0;
    int maxCol = 0;
    int32_t refinedDim1 = 0;
    int32_t refinedDim2 = 0;
    int connectedPixels = 0;
    int regionId = -1;
    bool valid = false;
};

struct GridPeakTable {
    std::array<GridPeakRegion, 4> regions{};
    int count = 0;
    int strongestSlot = -1;
    int weakestSlot = -1;
    int strongestRegionId = -1;
    int32_t selectedPeak3x3Sum = 0;
};

struct AsaCoorResult {
    int32_t dim1 = 0;
    int32_t dim2 = 0;
    bool valid = false;
};

inline uint16_t ReadLe16(const uint8_t* ptr) {
    return static_cast<uint16_t>(
        static_cast<uint16_t>(ptr[0]) |
        (static_cast<uint16_t>(ptr[1]) << 8));
}

inline bool IsAnchorValid(uint16_t anchorRow, uint16_t anchorCol) {
    return !(((anchorRow & 0xFFu) == kAnchorInvalid) &&
             ((anchorCol & 0xFFu) == kAnchorInvalid));
}

inline AsaGridData ExtractGridFromSlavePayloadBytes(const uint8_t* bytes, std::size_t byteCount) {
    AsaGridData out;
    if (!bytes || byteCount < static_cast<std::size_t>(kBlockWords * 2 * sizeof(uint16_t))) {
        return out;
    }

    out.tx1.anchorRow = ReadLe16(bytes);
    out.tx1.anchorCol = ReadLe16(bytes + sizeof(uint16_t));
    for (int r = 0; r < kGridDim; ++r) {
        for (int c = 0; c < kGridDim; ++c) {
            const std::size_t wordIndex = static_cast<std::size_t>(2 + r * kGridDim + c);
            out.tx1.grid[r][c] = static_cast<int16_t>(ReadLe16(bytes + wordIndex * sizeof(uint16_t)));
        }
    }
    out.tx1.valid = IsAnchorValid(out.tx1.anchorRow, out.tx1.anchorCol);

    const uint8_t* tx2 = bytes + static_cast<std::size_t>(kBlockWords * sizeof(uint16_t));
    out.tx2.anchorRow = ReadLe16(tx2);
    out.tx2.anchorCol = ReadLe16(tx2 + sizeof(uint16_t));
    for (int r = 0; r < kGridDim; ++r) {
        for (int c = 0; c < kGridDim; ++c) {
            const std::size_t wordIndex = static_cast<std::size_t>(2 + r * kGridDim + c);
            out.tx2.grid[r][c] = static_cast<int16_t>(ReadLe16(tx2 + wordIndex * sizeof(uint16_t)));
        }
    }
    out.tx2.valid = IsAnchorValid(out.tx2.anchorRow, out.tx2.anchorCol);

    return out;
}

inline AsaGridData ExtractGridFromSlaveWords(const uint16_t* words, int wordCount) {
    AsaGridData out;
    out.Clear();
    if (!words || wordCount < kBlockWords * 2) {
        return out;
    }

    out.tx1.anchorRow = words[0];
    out.tx1.anchorCol = words[1];
    for (int r = 0; r < kGridDim; ++r) {
        for (int c = 0; c < kGridDim; ++c) {
            out.tx1.grid[r][c] = static_cast<int16_t>(words[2 + r * kGridDim + c]);
        }
    }
    out.tx1.valid = IsAnchorValid(out.tx1.anchorRow, out.tx1.anchorCol);

    const uint16_t* tx2 = words + kBlockWords;
    out.tx2.anchorRow = tx2[0];
    out.tx2.anchorCol = tx2[1];
    for (int r = 0; r < kGridDim; ++r) {
        for (int c = 0; c < kGridDim; ++c) {
            out.tx2.grid[r][c] = static_cast<int16_t>(tx2[2 + r * kGridDim + c]);
        }
    }
    out.tx2.valid = IsAnchorValid(out.tx2.anchorRow, out.tx2.anchorCol);

    return out;
}

static constexpr int kMaxSensorDim = 80;

inline int32_t SensorPitchSizeMap(int32_t localCoor,
                                  const double* pitchTable,
                                  int coorUnit = kCoorUnit) {
    if (pitchTable[0] == 100.0) {
        return localCoor;
    }
    if (coorUnit == 0) {
        return 0;
    }

    const int cellIdx = localCoor / coorUnit;
    const int frac = localCoor % coorUnit;
    if (cellIdx < 0 || cellIdx >= kMaxSensorDim - 1) {
        return 0;
    }

    const double result =
        pitchTable[cellIdx + 1] * static_cast<double>(frac) +
        pitchTable[cellIdx] * static_cast<double>(coorUnit - frac);
    return static_cast<int32_t>(result);
}

} // namespace Asa
