#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include "SolverBuildConfig.h"

namespace Solvers {

enum TouchContactState : int {
    TouchStateDown = 0,
    TouchStateMove = 1,
    TouchStateUp = 2,
};

enum TouchLifeFlagBits : uint32_t {
    TouchLifeMapped = 1u << 0,
    TouchLifeNew = 1u << 1,
    TouchLifeLiftOff = 1u << 2,
    TouchLifeEdge = 1u << 3,
    TouchLifeDebounced = 1u << 4,
    TouchLifeAlwaysMatch = 1u << 5,
    TouchLifeSilentGap = 1u << 6,
};

enum TouchReportEventCode : int {
    TouchReportIdle = 1,
    TouchReportDown = 2,
    TouchReportMove = 4,
    TouchReportUp = 0x20,
};

// 触摸点结构体 (用于 Stage 2 连通域计算)
struct TouchContact {
    int id = 0;
    float x = 0.0f;
    float y = 0.0f;
    int state = 0; // 0=Down, 1=Update, 2=Up
    int area = 0;  // 连通域大小或强度
    int signalSum = 0; // 区域信号总和(对齐 TS 的 SigSum 语义)

    // Extended fields for TS/TE/TouchReport-aligned processing.
    float sizeMm = 0.0f;
    bool isEdge = false;
    bool isReported = true;
    int prevIndex = -1;
    int debugFlags = 0;

    // TS/TE/TouchReport-aligned state mirrors
    uint32_t lifeFlags = 0;
    uint32_t reportFlags = 0;
    int reportEvent = 0;
};

struct TouchPacket {
    bool valid = false;
    uint8_t reportId = 0x01;
    uint8_t length = 0x20;
    std::array<uint8_t, 32> bytes{};
};

struct TouchPeak {
    int r = 0;
    int c = 0;
    int16_t z = 0;
    uint8_t id = 0;
};

// Represents a connected component in the heatmap greater than a global threshold
struct MacroZone {
    std::span<const int> pixels{}; // 1D indices (r * cols + c), owned by MacroZoneDetector arena
    int area = 0;
    int signalSum = 0;
    int minR = 39;
    int maxR = 0;
    int minC = 59;
    int maxC = 0;
};

} // namespace Solvers
