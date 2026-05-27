#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>

#include "SolverTypes.h"

namespace VhfTouchPacket {

using TouchPackets = std::array<Solvers::TouchPacket, 2>;

constexpr float kTouchGridHeight = 40.0f;
constexpr float kTouchGridWidth = 60.0f;
constexpr float kTouchLogicalMaxY = 16000.0f;
constexpr float kTouchLogicalMaxX = 25600.0f;
constexpr size_t kContactsPerPacket = 5;
constexpr size_t kMaxTouchPackets = 2;
constexpr size_t kMaxReportedContacts = kContactsPerPacket * kMaxTouchPackets;
constexpr size_t kTouchPayloadOffset = 1;
constexpr size_t kTouchContactStride = 6;
constexpr size_t kTouchContactCountOffset = 31;

namespace detail {

inline void WriteU16Le(std::array<uint8_t, 32>& bytes,
                       size_t offset,
                       uint16_t value) {
    bytes[offset] = static_cast<uint8_t>(value & 0xFFu);
    bytes[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
}

inline uint16_t ToVhf(float gridValue, float gridMax,
                      float logicalMax, bool invert) {
    const float norm = std::clamp(gridValue / gridMax, 0.0f, 1.0f);
    const int vhf = std::clamp(
        static_cast<int>(std::lround(norm * logicalMax)),
        0, static_cast<int>(logicalMax));
    return static_cast<uint16_t>(
        invert ? (static_cast<int>(logicalMax) - vhf) : vhf);
}

inline uint8_t EncodeContactState(const Solvers::TouchContact& contact) {
    if (contact.reportEvent == Solvers::TouchReportUp) {
        return 0x02;
    }
    return 0x03;
}

inline bool ShouldReportTouchContact(
        const Solvers::TouchContact& contact) noexcept {
    return contact.id > 0 && contact.isReported;
}

inline void AppendContact(TouchPackets& packets,
                          size_t& count,
                          const Solvers::TouchContact& contact,
                          bool invertX,
                          bool invertY) {
    if (count >= kMaxReportedContacts || !ShouldReportTouchContact(contact)) {
        return;
    }

    auto& packet = packets[count / kContactsPerPacket];
    const size_t slot = count % kContactsPerPacket;
    const size_t base = kTouchPayloadOffset + slot * kTouchContactStride;
    auto& bytes = packet.bytes;

    bytes[base] = EncodeContactState(contact);
    bytes[base + 1] = static_cast<uint8_t>(std::clamp(contact.id, 0, 255));
    WriteU16Le(bytes, base + 2,
               ToVhf(contact.y, kTouchGridHeight,
                     kTouchLogicalMaxY, invertY));
    WriteU16Le(bytes, base + 4,
               ToVhf(contact.x, kTouchGridWidth,
                     kTouchLogicalMaxX, invertX));
    ++count;
}

} // namespace detail

inline TouchPackets Build(std::span<const Solvers::TouchContact> contacts,
                          bool transposeEnabled) {
    TouchPackets packets{};
    for (auto& packet : packets) {
        packet.bytes.fill(0);
        packet.bytes[0] = packet.reportId;
    }

    const bool invertX = !transposeEnabled;
    const bool invertY = transposeEnabled;
    size_t count = 0;

    for (const auto& contact : contacts) {
        if (count == kMaxReportedContacts) break;
        if (contact.reportEvent == Solvers::TouchReportUp) {
            detail::AppendContact(packets, count, contact, invertX, invertY);
        }
    }

    for (const auto& contact : contacts) {
        if (count == kMaxReportedContacts) break;
        if (contact.reportEvent != Solvers::TouchReportUp) {
            detail::AppendContact(packets, count, contact, invertX, invertY);
        }
    }

    packets[0].bytes[kTouchContactCountOffset] =
        static_cast<uint8_t>(count);
    packets[0].valid = count > 0;
    packets[1].valid = count > kContactsPerPacket;
    return packets;
}

} // namespace VhfTouchPacket
