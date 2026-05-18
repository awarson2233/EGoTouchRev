#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "SolverTypes.h"

namespace VhfStylusPacket {

struct Config {
    int sensorRows = 40;
    int sensorCols = 60;
    bool emitWhenInvalid = true;
};

namespace detail {

constexpr float kHidMaxX = 16000.0f;
constexpr float kHidMaxY = 25600.0f;
constexpr int16_t kTiltMax = 9000;
constexpr float kCoorUnit = 1024.0f;

inline void WriteU16Le(std::array<uint8_t, 17>& bytes,
                       size_t offset, uint16_t value) {
    bytes[offset] = static_cast<uint8_t>(value & 0xFFu);
    bytes[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
}

inline uint16_t ClampToU16(int32_t value) {
    return static_cast<uint16_t>(std::clamp(value, 0, 0xFFFF));
}

inline uint16_t MapOutputYToHidX(float sensorY, int sensorRows) {
    const float activeRows =
        std::max(1.0f, static_cast<float>(std::max(sensorRows, 1)) * kCoorUnit);
    const float clampedY = std::clamp(sensorY, 0.0f, activeRows);
    const auto hidX = static_cast<int32_t>(std::lround((clampedY / activeRows) * kHidMaxX));
    return ClampToU16(hidX);
}

inline uint16_t MapOutputXToHidY(float sensorX, int sensorCols) {
    const float activeCols =
        std::max(1.0f, static_cast<float>(std::max(sensorCols, 1)) * kCoorUnit);
    const float clampedX = std::clamp(sensorX, 0.0f, activeCols);
    const auto hidY = static_cast<int32_t>(std::lround((1.0f - (clampedX / activeCols)) * kHidMaxY));
    return ClampToU16(hidY);
}

inline bool HasReportableOutput(const Solvers::StylusOutputState& output) {
    return output.valid || output.point.valid || output.inRange;
}

} // namespace detail

inline Solvers::StylusPacket Build(const Solvers::StylusFrameData& stylus,
                                   const Config& config) {
    Solvers::StylusPacket packet{};
    packet.reportId = 0x08;
    packet.length = 13;

    if (!detail::HasReportableOutput(stylus.output)) {
        if (!config.emitWhenInvalid) {
            return packet;
        }

        packet.valid = true;
        packet.bytes.fill(0);
        packet.bytes[0] = packet.reportId;
        return packet;
    }

    packet.valid = true;
    packet.bytes.fill(0);
    packet.bytes[0] = packet.reportId;

    uint8_t penState = 0;
    if (stylus.output.tipDown) {
        penState |= 0x01u;
    }
    if (stylus.output.inRange) {
        penState |= 0x20u;
    }
    packet.bytes[1] = penState;
    packet.bytes[2] = 0x00u;

    detail::WriteU16Le(packet.bytes, 3,
                       detail::MapOutputYToHidX(stylus.output.point.y,
                                                config.sensorRows));
    detail::WriteU16Le(packet.bytes, 5,
                       detail::MapOutputXToHidY(stylus.output.point.x,
                                                config.sensorCols));
    detail::WriteU16Le(packet.bytes, 7,
                       static_cast<uint16_t>(std::min<uint32_t>(stylus.output.pressure, 4095u)));

    const int32_t tiltX = std::clamp(static_cast<int32_t>(stylus.output.point.tiltX),
                                     static_cast<int32_t>(-detail::kTiltMax),
                                     static_cast<int32_t>(detail::kTiltMax));
    const int32_t tiltY = std::clamp(static_cast<int32_t>(stylus.output.point.tiltY),
                                     static_cast<int32_t>(-detail::kTiltMax),
                                     static_cast<int32_t>(detail::kTiltMax));
    detail::WriteU16Le(packet.bytes, 9, static_cast<uint16_t>(static_cast<int16_t>(tiltX)));
    detail::WriteU16Le(packet.bytes, 11, static_cast<uint16_t>(static_cast<int16_t>(tiltY)));
    return packet;
}

inline uint8_t ExtractPenState(const Solvers::StylusPacket& packet) {
    return packet.valid ? packet.bytes[1] : 0;
}

} // namespace VhfStylusPacket
