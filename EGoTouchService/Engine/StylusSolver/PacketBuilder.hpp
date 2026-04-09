#pragma once
#include "AsaTypes.hpp"
#include "EngineTypes.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace Engine {

/// PacketBuilder — Builds HID pen reports from pipeline output.
///
/// Converts internal StylusFrameData into a 13-byte HID report
/// matching the hidinjector.sys descriptor layout.
///
/// HID Pen Report layout:
///   b[0]      : Report ID (0x08)
///   b[1]      : Status bits (TipSwitch:0, Barrel:1, Invert:2, Eraser:3, pad:4, InRange:5)
///   b[2]      : Contact Identifier
///   b[3..4]   : X position (uint16 LE, 0..16000)
///   b[5..6]   : Y position (uint16 LE, 0..25600)
///   b[7..8]   : Tip Pressure (uint16 LE, 0..4095)
///   b[9..10]  : X Tilt (int16 LE, -9000..+9000 centidegrees)
///   b[11..12] : Y Tilt (int16 LE, -9000..+9000 centidegrees)
class PacketBuilder {
public:
    /// Build a StylusPacket from frame data.
    /// @param result    Current frame results
    /// @param bleBtn    BLE button state byte (bit0=barrel, bit1=eraser)
    /// @param emitWhenInvalid  If true, emit a zero-state packet even for invalid coords
    /// @param[out] pkt  Output packet
    inline void Build(const StylusFrameData& result, uint8_t bleBtn,
                      bool emitWhenInvalid, StylusPacket& pkt) const {
        pkt = StylusPacket{};
        pkt.reportId = 0x08;
        pkt.length = 13;
        if (!result.point.valid && !emitWhenInvalid) {
            pkt.valid = false;
            return;
        }
        pkt.valid = true;
        auto& b = pkt.bytes;
        b.fill(0);
        b[0] = 0x08;

        // Status byte
        {
            uint8_t penState = 0;
            if (result.point.valid) penState |= (1u << 5);   // InRange
            if (result.pressure > 0) penState |= (1u << 0);  // TipSwitch
            if (bleBtn & 0x01) penState |= (1u << 1);        // BarrelSwitch
            b[1] = penState;
        }

        // Contact ID
        b[2] = 0x00;

        // X/Y coordinates
        if (result.point.valid) {
            const float offsetRow = static_cast<float>(screenOffsetY);
            const float offsetCol = static_cast<float>(screenOffsetX);
            const float sensorRangeRow =
                static_cast<float>(sensorRows * Asa::kCoorUnit);
            const float sensorRangeCol =
                static_cast<float>(sensorCols * Asa::kCoorUnit);
            const float activeRow = sensorRangeRow - offsetRow -
                static_cast<float>(screenEndMarginY);
            const float activeCol = sensorRangeCol - offsetCol -
                static_cast<float>(screenEndMarginX);

            float gy = std::clamp(result.point.y - offsetRow, 0.0f,
                                  std::max(1.0f, activeRow));
            float gx = std::clamp(result.point.x - offsetCol, 0.0f,
                                  std::max(1.0f, activeCol));

            // Row → HID X (16000): no inversion
            const float normHidX = activeRow > 0.0f ? (gy / activeRow) : 0.5f;
            // Col → HID Y (25600): invert
            const float normHidY = activeCol > 0.0f ? (1.0f - gx / activeCol) : 0.5f;

            uint16_t vx = static_cast<uint16_t>(std::clamp(
                static_cast<int32_t>(std::lround(normHidX * kHidMaxX)),
                0, static_cast<int32_t>(kHidMaxX)));
            uint16_t vy = static_cast<uint16_t>(std::clamp(
                static_cast<int32_t>(std::lround(normHidY * kHidMaxY)),
                0, static_cast<int32_t>(kHidMaxY)));

            WriteU16Le(b, 3, vx);
            WriteU16Le(b, 5, vy);
        }

        // Pressure (0..4095)
        uint16_t press = static_cast<uint16_t>(
            std::min(static_cast<uint32_t>(result.pressure), 4095u));
        WriteU16Le(b, 7, press);

        // Tilt (centidegrees)
        int16_t tiltXCdeg = static_cast<int16_t>(std::clamp(
            static_cast<int32_t>(result.point.tiltX) * 100,
            static_cast<int32_t>(-kTiltMax),
            static_cast<int32_t>(kTiltMax)));
        int16_t tiltYCdeg = static_cast<int16_t>(std::clamp(
            static_cast<int32_t>(result.point.tiltY) * 100,
            static_cast<int32_t>(-kTiltMax),
            static_cast<int32_t>(kTiltMax)));
        WriteU16Le(b, 9,  static_cast<uint16_t>(tiltXCdeg));
        WriteU16Le(b, 11, static_cast<uint16_t>(tiltYCdeg));
    }

    // ── Configuration ──
    int sensorRows = 40;
    int sensorCols = 60;
    int screenOffsetX = 0;
    int screenOffsetY = 0;
    int screenEndMarginX = 0;
    int screenEndMarginY = 0;

private:
    static constexpr float kHidMaxX = 16000.0f;
    static constexpr float kHidMaxY = 25600.0f;
    static constexpr int16_t kTiltMax = 9000;

    static inline void WriteU16Le(std::array<uint8_t, 17>& b,
                                   size_t off, uint16_t v) {
        b[off]     = static_cast<uint8_t>(v & 0xFF);
        b[off + 1] = static_cast<uint8_t>((v >> 8) & 0xFF);
    }
};

} // namespace Engine
