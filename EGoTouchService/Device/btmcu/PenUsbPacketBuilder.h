#pragma once

#include "btmcu/PenUsbTypes.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <span>
#include <string>
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

inline bool IsAllowedPenUsbUtf8Scalar(uint32_t scalar) noexcept {
    if (scalar == 0x09u) return true; // tab is harmless in logs/UI.
    if (scalar < 0x20u || (scalar >= 0x7Fu && scalar <= 0x9Fu)) return false;
    if (scalar >= 0xD800u && scalar <= 0xDFFFu) return false;
    return scalar <= 0x10FFFFu;
}

inline bool IsValidPenUsbUtf8Payload(std::span<const uint8_t> payload) noexcept {
    std::size_t i = 0;
    while (i < payload.size()) {
        const uint8_t lead = payload[i];
        uint32_t scalar = 0;
        std::size_t width = 0;
        if ((lead & 0x80u) == 0) {
            scalar = lead;
            width = 1;
        } else if (lead >= 0xC2u && lead <= 0xDFu) {
            scalar = lead & 0x1Fu;
            width = 2;
        } else if (lead >= 0xE0u && lead <= 0xEFu) {
            scalar = lead & 0x0Fu;
            width = 3;
        } else if (lead >= 0xF0u && lead <= 0xF4u) {
            scalar = lead & 0x07u;
            width = 4;
        } else {
            return false;
        }

        if (i + width > payload.size()) return false;
        for (std::size_t j = 1; j < width; ++j) {
            const uint8_t cont = payload[i + j];
            if ((cont & 0xC0u) != 0x80u) return false;
            scalar = (scalar << 6) | (cont & 0x3Fu);
        }

        if ((width == 3 && lead == 0xE0u && payload[i + 1] < 0xA0u) ||
            (width == 3 && lead == 0xEDu && payload[i + 1] >= 0xA0u) ||
            (width == 4 && lead == 0xF0u && payload[i + 1] < 0x90u) ||
            (width == 4 && lead == 0xF4u && payload[i + 1] >= 0x90u)) {
            return false;
        }
        if (!IsAllowedPenUsbUtf8Scalar(scalar)) return false;
        i += width;
    }
    return true;
}

inline std::string DecodePenUsbUtf8Payload(std::span<const uint8_t> payload) {
    std::size_t end = 0;
    while (end < payload.size() && payload[end] != 0x00) {
        ++end;
    }
    if (end == 0) {
        return {};
    }

    const auto bytes = payload.subspan(0, end);
    if (!IsValidPenUsbUtf8Payload(bytes)) {
        return {};
    }
    return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

inline std::string FormatPenUsbAsciiPayload(std::span<const uint8_t> payload) {
    return DecodePenUsbUtf8Payload(payload);
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

inline std::vector<uint8_t> BuildPenUsbFixedSizeCommand(PenUsbCommandId commandId) {
    auto packet = BuildPenUsbCommand(commandId);
    packet.resize(0x40, 0x00);
    return packet;
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
        0x58, 0x02, 0x1A, 0x41, 0x0F, 0x01, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    return BuildPenUsbPayloadCommand(PenUsbCommandId::InitParamSet, payload);
}

} // namespace Himax::Pen
