#pragma once

#include "btmcu/PenUsbTypes.h"

#include <cstdint>
#include <optional>
#include <span>

namespace Himax::Pen {

inline uint16_t PenPressureMax(PenPressureRangeMode) noexcept {
    return 4095;
}

inline uint16_t ScalePenPressure(uint16_t raw, PenPressureRangeMode mode) noexcept {
    uint16_t scaled = raw;
    if (mode == PenPressureRangeMode::Raw14Bit16382) {
        scaled = static_cast<uint16_t>(raw / 4u);
    }
    const uint16_t maxPressure = PenPressureMax(mode);
    return scaled > maxPressure ? maxPressure : scaled;
}

inline std::optional<PenPressureStats> TryParsePenPressurePacket(
        std::span<const uint8_t> packet,
        PenPressureRangeMode mode) noexcept {
    if (packet.size() < 11 || packet[0] != 0x55) {
        return std::nullopt;
    }

    PenPressureStats stats;
    stats.reportType = packet[0];
    stats.freq1 = packet[1];
    stats.freq2 = packet[2];
    stats.pressureMode = mode;
    stats.pressureMax = PenPressureMax(mode);

    for (int k = 0; k < 4; ++k) {
        stats.rawPress[k] = static_cast<uint16_t>(packet[3 + k * 2]) |
                            static_cast<uint16_t>(packet[4 + k * 2] << 8);
        stats.press[k] = ScalePenPressure(stats.rawPress[k], mode);
    }

    return stats;
}

} // namespace Himax::Pen
