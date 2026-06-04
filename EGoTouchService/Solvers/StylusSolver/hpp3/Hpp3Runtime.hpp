#pragma once

#include "StylusSolver/AsaTypes.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace Solvers {
struct HeatmapFrame;
}

namespace Solvers::Stylus::Hpp3 {

// Slave/grid frame constants.
static constexpr int kSlaveHeaderBytes = 7;
static constexpr int kBlockWords = 83;
static constexpr int kGridDim = 9;
static constexpr int kGridSize = kGridDim * kGridDim;
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

struct GridData {
    FreqBlock tx1;
    FreqBlock tx2;

    void Clear() {
        tx1.Clear();
        tx2.Clear();
    }
};

struct Projection {
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

struct GridFeature {
    int16_t grid[kGridDim][kGridDim]{};
    GridPeakUnit peak{};
    GridPeakTable peakTable{};
    Projection projection{};
    Asa::CoorResult refinedLocalCoor{};
    uint16_t peakSignal = 0;
    uint16_t dim1SelectedPeakNetSignal = 0;
    uint16_t dim2SelectedPeakNetSignal = 0;
    bool dim1SelectedPeakOnEdge = false;
    bool dim2SelectedPeakOnEdge = false;
};

struct RawGridRuntime {
    GridData grid{};
    std::array<uint8_t, kSlaveHeaderBytes> rawSlaveHdr{};
};

struct TxGridRuntime {
    GridFeature feature{};
#if EGOTOUCH_DIAG
    uint16_t triLeft = 0;
    uint16_t triCenter = 0;
    uint16_t triRight = 0;
    int16_t pitchComp = 0;
#endif
};

struct Settings {
    bool enabled = true;
};

struct State {
};

struct Runtime : Asa::Runtime {
    RawGridRuntime rawGrid{};
    TxGridRuntime tx1Grid{};
    TxGridRuntime tx2Grid{};

#if EGOTOUCH_DIAG
    void ResetDiagnosticFields() {
        Asa::Runtime::ResetDiagnosticFields();
        tx1Grid = {};
        tx2Grid = {};
    }
#endif
};

struct Context {
    HeatmapFrame& frame;
    Runtime& runtime;
    const Settings& settings;
    State& state;
};

inline bool IsAnchorValid(uint16_t anchorRow, uint16_t anchorCol) {
    return !(((anchorRow & 0xFFu) == kAnchorInvalid) &&
             ((anchorCol & 0xFFu) == kAnchorInvalid));
}

inline GridData ExtractGridFromSlavePayloadBytes(const uint8_t* bytes, std::size_t byteCount) {
    GridData out;
    if (!bytes || byteCount < static_cast<std::size_t>(kBlockWords * 2 * sizeof(uint16_t))) {
        return out;
    }

    out.tx1.anchorRow = Asa::ReadLe16(bytes);
    out.tx1.anchorCol = Asa::ReadLe16(bytes + sizeof(uint16_t));
    for (int r = 0; r < kGridDim; ++r) {
        for (int c = 0; c < kGridDim; ++c) {
            const std::size_t wordIndex = static_cast<std::size_t>(2 + r * kGridDim + c);
            out.tx1.grid[r][c] = static_cast<int16_t>(Asa::ReadLe16(bytes + wordIndex * sizeof(uint16_t)));
        }
    }
    out.tx1.valid = IsAnchorValid(out.tx1.anchorRow, out.tx1.anchorCol);

    const uint8_t* tx2 = bytes + static_cast<std::size_t>(kBlockWords * sizeof(uint16_t));
    out.tx2.anchorRow = Asa::ReadLe16(tx2);
    out.tx2.anchorCol = Asa::ReadLe16(tx2 + sizeof(uint16_t));
    for (int r = 0; r < kGridDim; ++r) {
        for (int c = 0; c < kGridDim; ++c) {
            const std::size_t wordIndex = static_cast<std::size_t>(2 + r * kGridDim + c);
            out.tx2.grid[r][c] = static_cast<int16_t>(Asa::ReadLe16(tx2 + wordIndex * sizeof(uint16_t)));
        }
    }
    out.tx2.valid = IsAnchorValid(out.tx2.anchorRow, out.tx2.anchorCol);

    return out;
}

inline GridData ExtractGridFromSlaveWords(const uint16_t* words, int wordCount) {
    GridData out;
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

} // namespace Solvers::Stylus::Hpp3
