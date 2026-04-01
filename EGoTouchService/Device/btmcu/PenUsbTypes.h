#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <vector>

namespace Himax::Pen {

enum class PenUsbCommandId : uint16_t {
    QueryPenStatus = 0x7101,
    QueryPenInfo = 0x7701,
    InitParamSet = 0x7D01,
    PairInfoSet = 0x7E01,
    EventAck = 0x8001,
};

enum class PenUsbEventCode : uint8_t {
    PenCurrentFunc = 0x2F,
    PenUnknown6F = 0x6F,            // 未命名事件 (ACK=11)
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

struct PenUsbHeader {
    uint8_t reportId = 0x07;
    uint8_t direction = 0x00;
    uint8_t protocol = 0x02;
    uint8_t reserved0 = 0x00;
    uint16_t commandId = 0x0000;
    uint8_t transportTag = 0x11;
    uint8_t option = 0x00;
};

struct PenUsbPacket {
    PenUsbHeader header{};
    std::vector<uint8_t> payload{};
};

struct PenEvent {
    PenUsbEventCode code = PenUsbEventCode::Unknown;
    std::vector<uint8_t> payload{};
    std::chrono::steady_clock::time_point receivedAt{};
};

using PenEventCallback = std::function<void(const PenEvent&)>;

} // namespace Himax::Pen
