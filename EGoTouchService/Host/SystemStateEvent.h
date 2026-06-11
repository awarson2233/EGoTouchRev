#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

namespace Host {

// Authoritative normalized runtime-facing semantics.
// Transport-level named events may alias to the same normalized event.
enum class SystemStateEventType : uint8_t {
    Unknown = 0,
    DisplayOn,
    DisplayOff,
    LidOn,
    LidOff,
    Suspend,
    Shutdown,
    ResumeAutomatic,
};

inline constexpr std::size_t kSystemStateEventTypeCount =
    static_cast<std::size_t>(SystemStateEventType::ResumeAutomatic) + 1;

// Named events are a transport compatibility surface.
// Multiple named events may map to the same normalized SystemStateEventType.
enum class SystemStateNamedEventId : uint8_t {
    MonitorConsoleDisplayOn = 0,
    MonitorConsoleDisplayOff,
    MonitorLidOn,
    MonitorLidOff,
    MonitorShutDown,
    PbtApmSuspend,
    PbtApmResumeAutomatic,
    PbtApmResumeSuspend,
    Count,
};

struct SystemStateNamedEventSpec {
    SystemStateNamedEventId id = SystemStateNamedEventId::MonitorConsoleDisplayOn;
    const wchar_t* name = L"";
    SystemStateEventType type = SystemStateEventType::Unknown;
};

inline constexpr size_t kSystemStateNamedEventCount =
    static_cast<size_t>(SystemStateNamedEventId::Count);

inline constexpr SystemStateNamedEventSpec kSystemStateNamedEventSpecs[kSystemStateNamedEventCount] = {
    {SystemStateNamedEventId::MonitorConsoleDisplayOn, L"Global\\MonitorConsoleDisplayOnEvent", SystemStateEventType::DisplayOn},
    {SystemStateNamedEventId::MonitorConsoleDisplayOff, L"Global\\MonitorConsoleDisplayOffEvent", SystemStateEventType::DisplayOff},
    {SystemStateNamedEventId::MonitorLidOn, L"Global\\MonitorLidOnEvent", SystemStateEventType::LidOn},
    {SystemStateNamedEventId::MonitorLidOff, L"Global\\MonitorLidOffEvent", SystemStateEventType::LidOff},
    {SystemStateNamedEventId::MonitorShutDown, L"Global\\MonitorShutDownEvent", SystemStateEventType::Shutdown},
    {SystemStateNamedEventId::PbtApmSuspend, L"Global\\PBT_APMSUSPEND", SystemStateEventType::Suspend},
    {SystemStateNamedEventId::PbtApmResumeAutomatic, L"Global\\PBT_APMRESUMEAUTOMATIC", SystemStateEventType::ResumeAutomatic},
    {SystemStateNamedEventId::PbtApmResumeSuspend, L"Global\\PBT_APMRESUMESUSPEND", SystemStateEventType::ResumeAutomatic},
};

constexpr size_t ToIndex(SystemStateNamedEventId id) noexcept {
    return static_cast<size_t>(id);
}

constexpr std::size_t ToIndex(SystemStateEventType type) noexcept {
    return static_cast<std::size_t>(type);
}

constexpr const SystemStateNamedEventSpec* TryGetNamedEventSpec(SystemStateNamedEventId id) noexcept {
    const size_t index = ToIndex(id);
    if (index >= kSystemStateNamedEventCount) {
        return nullptr;
    }
    return &kSystemStateNamedEventSpecs[index];
}

constexpr const SystemStateNamedEventSpec* TryGetNamedEventSpec(size_t index) noexcept {
    if (index >= kSystemStateNamedEventCount) {
        return nullptr;
    }
    return &kSystemStateNamedEventSpecs[index];
}

enum class SystemStateEventSource : uint8_t {
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
