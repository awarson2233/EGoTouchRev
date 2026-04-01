#pragma once
#include <expected>
#include <cstdint>

// ── 设备层统一错误模型 ──────────────────────────────────────────────────────
enum class ChipError {
    Success = 0,
    CommunicationError,  // 底层驱动/总线通讯故障 (Handle, IOCTL, Read/Write)
    Timeout,             // 操作超时 (Interrupt, Reloading, Status polling)
    VerificationFailed,  // 逻辑校验失败 (Verify failed, FW Status error)
    InvalidOperation,    // 调用时机或参数非法 (Invalid param, State error, Not ready)
    InternalError        // 内部逻辑异常
};

// 错误处理别名
template <typename T = void>
using ChipResult = std::expected<T, ChipError>;
