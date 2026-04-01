#pragma once

#include <chrono>
#include <cstdint>

namespace Host {

enum class SystemStateEventType : std::uint8_t {
    Unknown = 0,
    DisplayOn,
    DisplayOff,
    LidOn,
    LidOff,
    Shutdown,
    ResumeAutomatic,
};

enum class SystemStateEventSource : std::uint8_t {
    ThpServiceNamedEvent = 0,
};

struct SystemStateEvent {
    SystemStateEventType type = SystemStateEventType::Unknown;
    SystemStateEventSource source = SystemStateEventSource::ThpServiceNamedEvent;
    std::chrono::system_clock::time_point timestamp{};
    std::uint32_t raw_index = 0;
    const wchar_t* raw_name = L"";
};

const char* ToString(SystemStateEventType type) noexcept;

} // namespace Host

