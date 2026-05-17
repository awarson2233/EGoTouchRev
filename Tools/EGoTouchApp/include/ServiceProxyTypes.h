#pragma once

#include "IpcProtocol.h"
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

struct DynamicDebugField {
    uint16_t fieldId = 0;
    Ipc::DebugValueType valueType = Ipc::DebugValueType::UInt32;
    Ipc::DebugSourceKind sourceKind = Ipc::DebugSourceKind::DerivedField;
    int16_t sourceIndex = -1;
    uint8_t uiOrder = 0;
    Ipc::DebugDvrTarget dvrTarget = Ipc::DebugDvrTarget::None;
    Ipc::DebugDvrPositionMode dvrPositionMode = Ipc::DebugDvrPositionMode::Append;
    int16_t dvrIndex = -1;
    std::string key;
    std::string displayName;
    std::string unit;
    std::string uiGroup;
    std::string dvrColumnName;
    std::string dvrAnchor;
};

struct DynamicDebugValue {
    Ipc::DebugValueType valueType = Ipc::DebugValueType::UInt32;
    bool valid = false;
    uint64_t rawValue = 0;
};

struct DvrDynamicDebugSample {
    uint16_t fieldId = 0;
    DynamicDebugValue value{};
};

struct DvrDynamicDebugSchema {
    std::vector<DynamicDebugField> fields;
    uint16_t schemaVersion = 0;
    uint32_t schemaHash = 0;

    bool Empty() const { return fields.empty(); }
};

struct DvrDynamicDebugFrame {
    std::vector<DvrDynamicDebugSample> samples;
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

    bool Empty() const { return frames.empty(); }
    size_t Size() const { return frames.size(); }
};

} // namespace App
