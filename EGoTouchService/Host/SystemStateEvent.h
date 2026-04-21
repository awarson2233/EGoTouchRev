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
    Shutdown,
    ResumeAutomatic,
};

inline constexpr std::size_t kSystemStateEventTypeCount =
    static_cast<std::size_t>(SystemStateEventType::ResumeAutomatic) + 1;

// Named events are a transport compatibility surface.
// Multiple named events may map to the same normalized SystemStateEventType.
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

enum class SystemStateTransportRole : uint8_t {
    Canonical = 0,
    LegacyAlias = 1,
};

struct SystemStateNamedEventSpec {
    SystemStateNamedEventId id = SystemStateNamedEventId::MonitorPowerOn;
    const wchar_t* name = L"";
    SystemStateEventType type = SystemStateEventType::Unknown;
    SystemStateTransportRole transportRole = SystemStateTransportRole::Canonical;
};

inline constexpr size_t kSystemStateNamedEventCount =
    static_cast<size_t>(SystemStateNamedEventId::Count);

inline constexpr SystemStateNamedEventSpec kSystemStateNamedEventSpecs[kSystemStateNamedEventCount] = {
    {SystemStateNamedEventId::MonitorPowerOn, L"Global\\MonitorPowerOnEvent", SystemStateEventType::DisplayOn, SystemStateTransportRole::LegacyAlias},
    {SystemStateNamedEventId::MonitorPowerOff, L"Global\\MonitorPowerOffEvent", SystemStateEventType::DisplayOff, SystemStateTransportRole::LegacyAlias},
    {SystemStateNamedEventId::MonitorConsoleDisplayOn, L"Global\\MonitorConsoleDisplayOnEvent", SystemStateEventType::DisplayOn, SystemStateTransportRole::Canonical},
    {SystemStateNamedEventId::MonitorConsoleDisplayOff, L"Global\\MonitorConsoleDisplayOffEvent", SystemStateEventType::DisplayOff, SystemStateTransportRole::Canonical},
    {SystemStateNamedEventId::MonitorLidOn, L"Global\\MonitorLidOnEvent", SystemStateEventType::LidOn, SystemStateTransportRole::Canonical},
    {SystemStateNamedEventId::MonitorLidOff, L"Global\\MonitorLidOffEvent", SystemStateEventType::LidOff, SystemStateTransportRole::Canonical},
    {SystemStateNamedEventId::MonitorShutDown, L"Global\\MonitorShutDownEvent", SystemStateEventType::Shutdown, SystemStateTransportRole::Canonical},
    {SystemStateNamedEventId::PbtApmResumeAutomatic, L"Global\\PBT_APMRESUMEAUTOMATIC", SystemStateEventType::ResumeAutomatic, SystemStateTransportRole::Canonical},
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

constexpr bool IsCanonicalTransportEvent(SystemStateNamedEventId id) noexcept {
    const auto* spec = TryGetNamedEventSpec(id);
    return spec != nullptr && spec->transportRole == SystemStateTransportRole::Canonical;
}

constexpr bool IsLegacyTransportAlias(SystemStateNamedEventId id) noexcept {
    const auto* spec = TryGetNamedEventSpec(id);
    return spec != nullptr && spec->transportRole == SystemStateTransportRole::LegacyAlias;
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
