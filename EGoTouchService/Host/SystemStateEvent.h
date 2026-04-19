#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

namespace Host {

enum class SystemStateEventType : uint8_t {
    Unknown = 0,
    DisplayOn,
    DisplayOff,
    LidOn,
    LidOff,
    Shutdown,
    ResumeAutomatic,
};

enum class SystemStateNamedEventId : uint8_t {
    MonitorPowerOn = 0,
    MonitorPowerOff,
    MonitorConsoleDisplayOn,
    MonitorConsoleDisplayOff,
    MonitorLidOn,
    MonitorLidOff,
    MonitorShutDown,
    PbtApmResumeAutomatic,
    Count,
};

struct SystemStateNamedEventSpec {
    SystemStateNamedEventId id = SystemStateNamedEventId::MonitorPowerOn;
    const wchar_t* name = L"";
    SystemStateEventType type = SystemStateEventType::Unknown;
};

inline constexpr size_t kSystemStateNamedEventCount =
    static_cast<size_t>(SystemStateNamedEventId::Count);

inline constexpr SystemStateNamedEventSpec kSystemStateNamedEventSpecs[kSystemStateNamedEventCount] = {
    {SystemStateNamedEventId::MonitorPowerOn, L"Global\\MonitorPowerOnEvent", SystemStateEventType::DisplayOn},
    {SystemStateNamedEventId::MonitorPowerOff, L"Global\\MonitorPowerOffEvent", SystemStateEventType::DisplayOff},
    {SystemStateNamedEventId::MonitorConsoleDisplayOn, L"Global\\MonitorConsoleDisplayOnEvent", SystemStateEventType::DisplayOn},
    {SystemStateNamedEventId::MonitorConsoleDisplayOff, L"Global\\MonitorConsoleDisplayOffEvent", SystemStateEventType::DisplayOff},
    {SystemStateNamedEventId::MonitorLidOn, L"Global\\MonitorLidOnEvent", SystemStateEventType::LidOn},
    {SystemStateNamedEventId::MonitorLidOff, L"Global\\MonitorLidOffEvent", SystemStateEventType::LidOff},
    {SystemStateNamedEventId::MonitorShutDown, L"Global\\MonitorShutDownEvent", SystemStateEventType::Shutdown},
    {SystemStateNamedEventId::PbtApmResumeAutomatic, L"Global\\PBT_APMRESUMEAUTOMATIC", SystemStateEventType::ResumeAutomatic},
};

constexpr size_t ToIndex(SystemStateNamedEventId id) noexcept {
    return static_cast<size_t>(id);
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

