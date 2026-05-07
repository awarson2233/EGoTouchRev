#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

#include "SolverBuildConfig.h"
#include "FrameLayout.h"
#include "TouchFrameTypes.h"
#include "StylusFrameTypes.h"

namespace Solvers {

struct HeatmapFrame {
#if EGOTOUCH_DIAG
    std::vector<uint8_t> rawData;
#endif
    const uint8_t* rawPtr = nullptr;
    size_t rawLen = 0;

    Frame::MasterSuffixView masterSuffix{};
    Frame::SlaveSuffixView slaveSuffix{};
    bool masterSuffixValid = false;
    bool slaveSuffixValid = false;

    int16_t heatmapMatrix[40][60];

    std::vector<TouchContact> contacts;
    std::array<TouchPacket, 2> touchPackets{};

#if EGOTOUCH_DIAG
    std::vector<TouchPeak> peaks;
    std::array<uint8_t, 2400> touchZones{};
    std::array<uint8_t, 2400> peakZones{};
#endif

    StylusFrameData stylus;

    uint64_t timestamp;
    uint64_t receiveSystemEpochUs = 0;
    bool masterWasRead = true;

    HeatmapFrame() : timestamp(0) {
        std::memset(heatmapMatrix, 0, sizeof(heatmapMatrix));
    }
};

} // namespace Solvers
