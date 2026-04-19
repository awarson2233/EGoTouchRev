#pragma once

#include <cstdint>

namespace Himax::detail {

inline void ParseAddressLittleEndian(uint32_t addr, uint8_t* out, int len) {
    switch (len) {
    case 1:
        out[0] = static_cast<uint8_t>(addr & 0xFFu);
        break;
    case 2:
        out[0] = static_cast<uint8_t>(addr & 0xFFu);
        out[1] = static_cast<uint8_t>((addr >> 8) & 0xFFu);
        break;
    case 4:
        out[0] = static_cast<uint8_t>(addr & 0xFFu);
        out[1] = static_cast<uint8_t>((addr >> 8) & 0xFFu);
        out[2] = static_cast<uint8_t>((addr >> 16) & 0xFFu);
        out[3] = static_cast<uint8_t>((addr >> 24) & 0xFFu);
        break;
    default:
        break;
    }
}

} // namespace Himax::detail
