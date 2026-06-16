#pragma once

#include "DvrTypes.h"
#include "SolverTypes.h"
#include <cstdint>
#include <string>
#include <vector>

namespace App {

// Lightweight mirror of Service-side Pen channel status (no PenBridge.h dependency)
struct PenBridgeStatus {
    bool     evtRunning   = false;  // col00 事件通道运行中
    bool     pressRunning = false;  // col01 压力通道运行中
    uint8_t  reportType   = 0;
    uint8_t  freq1        = 0;
    uint8_t  freq2        = 0;
    uint16_t press[4]     = {0,0,0,0};
    uint16_t rawPress[4]  = {0,0,0,0};
    uint8_t  pressureMode = 1;
    uint16_t pressureMax  = 4095;
};

enum class PenProtocolHint : uint8_t {
    Auto = 0,
    Hpp2,
    Hpp3,
};

struct PenIdentityStatus {
    bool hasConnectionState = false;
    bool connected = false;
    bool hasStylusId = false;
    uint8_t stylusId = 0;
    bool hasPenModuleModelId = false;
    uint32_t penModuleModelId = 0;
    bool hasProtocolHint = false;
    PenProtocolHint protocolHint = PenProtocolHint::Auto;
    bool protocolHintFromPenModule = false;
    uint16_t factoryStatusFlags = 0;
    bool hasSerialNumber = false;
    std::string serialNumber;
    bool hasHardwareVersion = false;
    std::string hardwareVersion;
    bool hasFirmwareVersion = false;
    std::string firmwareVersion;
};

enum class FrameSourceMode {
    Live = 0,
    Playback = 1,
};

enum class PlaybackTimingMode {
    HostReceiveEpochUs = 0,
    LegacyServiceTimestamp = 1,
    SyntheticFrameIndex = 2,
};

struct DvrPlaybackFrame {
    Solvers::HeatmapFrame frame{};
    uint64_t recordingTimeUs = 0;
    uint64_t sourceTimeUs = 0;
    uint64_t hostReceiveUnixTimeUs = 0;
    uint64_t sequence = 0;
    DvrDynamicDebugFrame dynamicDebug{};
};

struct DvrPlaybackDataset {
    std::vector<DvrPlaybackFrame> frames;
    int formatVersion = 0;
    uint32_t flags = 0;
    PlaybackTimingMode timingMode = PlaybackTimingMode::SyntheticFrameIndex;
    DvrDynamicDebugSchema dynamicDebugSchema{};
    DvrRuntimeConfigSnapshot runtimeConfig{};

    bool Empty() const { return frames.empty(); }
    size_t Size() const { return frames.size(); }
};

} // namespace App
