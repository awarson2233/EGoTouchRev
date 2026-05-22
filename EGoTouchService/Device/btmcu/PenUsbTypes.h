#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace Himax::Pen {

using NativeEventHandle = void*;

enum class PenUsbCommandId : uint16_t {
    QueryPenStatus = 0x7101,
    QueryPenInfo = 0x7701,
    InitParamSet = 0x7D01,
    PairInfoSet = 0x7E01,
    EventAck = 0x8001,
};

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

enum class PenUsbEventCode : uint8_t {
    PenCurrentFunc = 0x2F,
    PenUnknown6F = 0x6F,            // 未确认事件，不自动 ACK
    PenAcStatus = 0x70,
    PenConnStatus = 0x71,
    PenCurStatus = 0x72,            // 笔工作模式（书写/悬停/橡皮擦）
    PenTypeInfo = 0x73,             // 笔类型信息 → set_stylus_id
    PenRotateAngle = 0x74,          // 屏幕旋转角度
    PenTouchMode = 0x75,
    PenGlobalPreventMode = 0x76,
    PenScreenStatus = 0x77,         // 屏幕状态（非 PEN_READY！）
    PenHolster = 0x78,
    PenFreqJump = 0x79,
    PenRepParam = 0x7B,             // 初始化参数 (CSV) → 0x7D01 回显
    PenGlobalAnnotation = 0x7C,
    EraserToggle = 0x7F,
    Unknown = 0x00,
};

enum class PenSessionState : uint8_t {
    Stopped = 0,
    Starting,
    Running,
    Error,
};

enum class PenCurrentMode : uint8_t {
    Unknown = 0,
    Writing = 1,
    Hovering = 2,
    Eraser = 3,
};

enum class PenPressureRangeMode : uint8_t {
    Raw12Bit4096 = 0,
    Raw14Bit16382 = 1,
};

struct PenPressureStats {
    uint16_t press[4] = {0, 0, 0, 0};
    uint16_t rawPress[4] = {0, 0, 0, 0};
    uint8_t freq1 = 0;
    uint8_t freq2 = 0;
    uint8_t reportType = 0;
    PenPressureRangeMode pressureMode = PenPressureRangeMode::Raw12Bit4096;
    uint16_t pressureMax = 4095;
};

constexpr PenCurrentMode PenCurrentModeFromRaw(uint8_t raw) noexcept {
    switch (raw) {
    case 1: return PenCurrentMode::Writing;
    case 2: return PenCurrentMode::Hovering;
    case 3: return PenCurrentMode::Eraser;
    default: return PenCurrentMode::Unknown;
    }
}

constexpr const char* ToString(PenCurrentMode mode) noexcept {
    switch (mode) {
    case PenCurrentMode::Writing: return "writing";
    case PenCurrentMode::Hovering: return "hovering";
    case PenCurrentMode::Eraser: return "eraser";
    default: return "unknown";
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

struct PenSemanticState {
    bool hasConnection = false;
    bool connected = false;

    bool hasStylusId = false;
    uint8_t stylusId = 0;

    bool hasCurrentMode = false;
    PenCurrentMode currentMode = PenCurrentMode::Unknown;
    uint8_t currentModeRaw = 0;

    bool hasEraserToggle = false;
    uint8_t eraserToggle = 0;

    bool hasCurrentFunc = false;
    uint8_t currentFunc = 0;
};

struct PenUsbHeader {
    uint8_t reportId     = 0x07;  // byte[0]: HID report type (always 0x07)
    uint8_t hasPayload   = 0x00;  // byte[1]: 0x00=no payload, 0x01=has payload
    uint8_t protocol     = 0x02;  // byte[2]: sub-protocol (always 0x02)
    uint8_t reserved0    = 0x00;  // byte[3]: reserved
    uint16_t commandId   = 0x0000;// byte[4,5]: command ID (little-endian)
    uint8_t transportTag = 0x11;  // byte[6]: MCU transport marker (forced by SendPacket)
    uint8_t payloadTag   = 0x00;  // byte[7]: 0x00=no payload, 0x20=has payload
};

struct PenUsbPacket {
    PenUsbHeader header{};
    std::vector<uint8_t> payload{};
};

struct PenEvent {
    PenUsbEventCode code = PenUsbEventCode::Unknown;
    std::vector<uint8_t> payload{};
    PenSemanticState semantic{};
    std::chrono::steady_clock::time_point receivedAt{};
};

using PenEventCallback = std::function<void(const PenEvent&)>;

} // namespace Himax::Pen
