#pragma once

#include <array>
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

struct AsaCoorResult {
    int32_t dim1 = 0;
    int32_t dim2 = 0;
    bool valid = false;
};

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
    out.tx1.valid = (out.tx1.anchorRow != kAnchorInvalid) ||
                    (out.tx1.anchorCol != kAnchorInvalid);

    const uint16_t* tx2 = words + kBlockWords;
    out.tx2.anchorRow = tx2[0];
    out.tx2.anchorCol = tx2[1];
    for (int r = 0; r < kGridDim; ++r) {
        for (int c = 0; c < kGridDim; ++c) {
            out.tx2.grid[r][c] = static_cast<int16_t>(tx2[2 + r * kGridDim + c]);
        }
    }
    out.tx2.valid = out.tx1.valid;

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
