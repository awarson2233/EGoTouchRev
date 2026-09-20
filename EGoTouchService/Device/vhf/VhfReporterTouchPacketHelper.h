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

// 手指管线 Stage 8（PitchRemapAndDisplay）交付的 TouchContact::x/y 是**显示物理
// 像素**，量程由面板整定表给定：2560 x 1600。上报口径只认这一个单位。
constexpr float kTouchPixelMaxX = 2560.0f;
constexpr float kTouchPixelMaxY = 1600.0f;
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

// 像素 → HID 逻辑单位。曾经这里按 `pixelValue > gridMax` 猜单位（大于 60/40 当
// 像素、否则当节点坐标），于是最外一列/行映射出来的小像素值（X ≤ 60、Y ≤ 40）会
// 被当成节点坐标除以 60/40，报点瞬间跳到屏幕另一侧再逐步走回中央 —— 长轴的那
// 60 px 死区比一列（2560/60 ≈ 42.7 px）还宽，所以整列受害；短轴的 40 px 死区恰
// 好等于一行（1600/40 = 40 px），重心一旦贴边就直接落到 0，跳不进死区，因此只有
// 长轴暴露出回绕。单位由调用方显式给定，不再推断。
inline uint16_t ToVhf(float pixelValue, float pixelMax,
                      float logicalMax, bool invert) {
    const float norm = std::clamp(pixelValue / pixelMax, 0.0f, 1.0f);
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
               ToVhf(contact.y, kTouchPixelMaxY,
                     kTouchLogicalMaxY, invertY));
    WriteU16Le(bytes, base + 4,
               ToVhf(contact.x, kTouchPixelMaxX,
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
