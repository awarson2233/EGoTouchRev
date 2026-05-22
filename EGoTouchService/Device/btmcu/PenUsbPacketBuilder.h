#pragma once

#include "btmcu/PenUsbTypes.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <span>
#include <string_view>
#include <vector>

namespace Himax::Pen {

inline uint8_t ParsePenUsbType3Byte(std::string_view text) noexcept {
    char buf[5] = {'0', 'x', '\0', '\0', '\0'};
    if (!text.empty()) {
        buf[2] = text[0];
    }
    if (text.size() > 1) {
        buf[3] = text[1];
    }
    return static_cast<uint8_t>(std::strtoul(buf, nullptr, 0));
}

inline std::size_t EncodePenUsbType3Token(std::string_view token,
                                          std::span<uint8_t> out) noexcept {
    if (token.empty()) {
        return 0;
    }
    if (token.size() <= 1) {
        out[0] = ParsePenUsbType3Byte(token.substr(0, 1));
        return 1;
    }
    if (token.size() == 2) {
        out[0] = ParsePenUsbType3Byte(token.substr(1, 1));
        out[1] = ParsePenUsbType3Byte(token.substr(0, 1));
        return 2;
    }
    if (token.size() == 3) {
        out[0] = ParsePenUsbType3Byte(token.substr(1, 2));
        out[1] = ParsePenUsbType3Byte(token.substr(0, 1));
        return 2;
    }
    out[0] = ParsePenUsbType3Byte(token.substr(2, 2));
    out[1] = ParsePenUsbType3Byte(token.substr(0, 2));
    return 2;
}

inline std::vector<uint8_t> BuildPenUsbCommand(PenUsbCommandId commandId) {
    const auto id = static_cast<uint16_t>(commandId);
    return {
        0x07, 0x00, 0x02, 0x00,
        static_cast<uint8_t>(id & 0xFFu),
        static_cast<uint8_t>((id >> 8) & 0xFFu),
        0x11, 0x00,
    };
}

inline std::vector<uint8_t> BuildPenUsbPayloadCommand(PenUsbCommandId commandId,
                                                      std::span<const uint8_t> payload) {
    const auto id = static_cast<uint16_t>(commandId);
    std::vector<uint8_t> packet{
        0x07, 0x01, 0x02, 0x00,
        static_cast<uint8_t>(id & 0xFFu),
        static_cast<uint8_t>((id >> 8) & 0xFFu),
        0x11, 0x20,
    };
    packet.insert(packet.end(), payload.begin(), payload.end());
    return packet;
}

inline std::vector<uint8_t> BuildPenUsbEventAck(uint8_t ackCode) {
    const std::array<uint8_t, 1> payload{ackCode};
    return BuildPenUsbPayloadCommand(PenUsbCommandId::EventAck, payload);
}

inline std::array<uint8_t, 0x20> BuildScanModePayload(uint8_t freq1,
                                                       uint8_t freq2,
                                                       uint8_t mode) noexcept {
    char freq1Text[8]{};
    char freq2Text[8]{};
    char modeText[8]{};
    std::snprintf(freq1Text, sizeof(freq1Text), "%u", freq1);
    std::snprintf(freq2Text, sizeof(freq2Text), "%u", freq2);
    std::snprintf(modeText, sizeof(modeText), "%u", mode ? 3u : 0u);

    std::array<uint8_t, 0x20> payload{};
    std::size_t offset = 0;
    offset += EncodePenUsbType3Token(freq1Text, std::span<uint8_t>(payload).subspan(offset));
    offset += EncodePenUsbType3Token(freq2Text, std::span<uint8_t>(payload).subspan(offset));
    (void)EncodePenUsbType3Token(modeText, std::span<uint8_t>(payload).subspan(offset));
    return payload;
}

inline std::vector<uint8_t> BuildScanModeCommand(uint8_t freq1, uint8_t freq2, uint8_t mode) {
    const auto payload = BuildScanModePayload(freq1, freq2, mode);
    return BuildPenUsbPayloadCommand(PenUsbCommandId::InitParamSet, payload);
}

inline std::vector<uint8_t> BuildFactoryInitProtocolParamsCommand() {
    const std::array<uint8_t, 0x20> payload{
        0x33, 0x33, 0x33, 0x33, 0xE7, 0x02, 0x12, 0x04,
        0x58, 0x02, 0x8D, 0x20, 0x0F, 0x01, 0x02, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    return BuildPenUsbPayloadCommand(PenUsbCommandId::InitParamSet, payload);
}

} // namespace Himax::Pen
