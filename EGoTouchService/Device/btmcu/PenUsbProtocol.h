#pragma once

#include "btmcu/PenUsbTypes.h"

#include <cstdint>
#include <optional>
#include <span>

namespace Himax::Pen {

struct ParsedPenUsbEventFrame {
    uint8_t eventCode = 0;
    std::span<const uint8_t> payload{};
};

inline std::optional<ParsedPenUsbEventFrame> TryParsePenUsbEventFrame(
        std::span<const uint8_t> packet) noexcept {
    if (packet.size() < 9) {
        return std::nullopt;
    }
    if (packet[2] != 0x07 || packet[4] != 0x01) {
        return std::nullopt;
    }
    return ParsedPenUsbEventFrame{
        packet[5],
        packet.subspan(8),
    };
}

constexpr int GetFactoryBtMcuAckCode(uint8_t eventCode) noexcept {
    switch (eventCode) {
    case 0x2F: return 0x0B;
    case 0x70: return 0x00;
    case 0x71: return 0x01;
    case 0x72: return 0x02;
    case 0x73: return 0x0D;
    case 0x74: return 0x03;
    case 0x75: return 0x04;
    case 0x76: return 0x05;
    case 0x77: return 0x06;
    case 0x78: return 0x07;
    case 0x79: return 0x08;
    case 0x7B: return 0x0A;
    case 0x7C: return 0x0C;
    case 0x7F: return 0x09;
    default: return -1;
    }
}

constexpr bool FactoryStatusFlagsAffected(PenUsbEventCode code) noexcept {
    switch (code) {
    case PenUsbEventCode::PenAcStatus:
    case PenUsbEventCode::PenConnStatus:
    case PenUsbEventCode::PenCurStatus:
    case PenUsbEventCode::PenTypeInfo:
    case PenUsbEventCode::PenRotateAngle:
    case PenUsbEventCode::PenTouchMode:
    case PenUsbEventCode::PenGlobalPreventMode:
    case PenUsbEventCode::PenHolster:
        return true;
    default:
        return false;
    }
}

constexpr uint16_t SetFactoryFlagField(uint16_t flags,
                                       uint16_t mask,
                                       uint16_t value) noexcept {
    return static_cast<uint16_t>((flags & ~mask) | (value & mask));
}

constexpr uint16_t ApplyFactoryStatusFlagUpdate(uint16_t flags,
                                                PenUsbEventCode code,
                                                uint8_t payload) noexcept {
    switch (code) {
    case PenUsbEventCode::PenAcStatus:
        return SetFactoryFlagField(flags, 0x0001u, payload & 0x01u);
    case PenUsbEventCode::PenConnStatus:
        return SetFactoryFlagField(flags, 0x0002u, static_cast<uint16_t>((payload & 0x01u) << 1));
    case PenUsbEventCode::PenCurStatus:
        if (payload == 1) return SetFactoryFlagField(flags, 0x000Cu, 0);
        if (payload == 2) return SetFactoryFlagField(flags, 0x000Cu, 0x0004u);
        if (payload == 3) return SetFactoryFlagField(flags, 0x000Cu, 0x0008u);
        return flags;
    case PenUsbEventCode::PenTypeInfo:
        return SetFactoryFlagField(flags, 0x0030u, static_cast<uint16_t>((payload & 0x03u) << 4));
    case PenUsbEventCode::PenRotateAngle:
        if (payload == 2) return SetFactoryFlagField(flags, 0x00C0u, 0);
        if (payload == 4) return SetFactoryFlagField(flags, 0x00C0u, 0x0080u);
        return SetFactoryFlagField(flags, 0x00C0u, static_cast<uint16_t>((payload << 6) & 0x00C0u));
    case PenUsbEventCode::PenTouchMode:
        return SetFactoryFlagField(flags, 0x0100u, static_cast<uint16_t>((payload & 0x01u) << 8));
    case PenUsbEventCode::PenGlobalPreventMode:
        return SetFactoryFlagField(flags, 0x0200u, static_cast<uint16_t>((payload & 0x01u) << 9));
    case PenUsbEventCode::PenHolster:
        return SetFactoryFlagField(flags, 0x0800u, static_cast<uint16_t>((payload & 0x01u) << 11));
    default:
        return flags;
    }
}

} // namespace Himax::Pen
