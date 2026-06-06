#include "SystemStateMonitor.h"
#include "Logger.h"

#include <array>
#include <atomic>
#include <condition_variable>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <sddl.h>

namespace Host {

struct SystemStateMonitor::Impl {
    std::array<std::wstring, kEventCount> eventNames{};
    HANDLE events[kEventCount]{};
    HANDLE stopEvent = nullptr;
    EventCallback callback;
    std::thread worker;
    std::atomic<bool> running{false};
    std::mutex lifecycleMu;
    std::condition_variable lifecycleCv;
    bool joining = false;
    bool selfStopCleanupPending = false;
    std::thread::id workerThreadId{};
    SystemStateEventType lastDisplayType = SystemStateEventType::Unknown;
    SystemStateEventType lastLidType = SystemStateEventType::Unknown;
    bool shutdownObserved = false;
};

namespace {

struct NamedEventNameListHolder {
    const wchar_t* names[SystemStateMonitor::kEventCount]{};

    constexpr NamedEventNameListHolder() {
        for (std::size_t i = 0; i < SystemStateMonitor::kEventCount; ++i) {
            names[i] = kSystemStateNamedEventSpecs[i].name;
        }
    }
};

inline constexpr NamedEventNameListHolder kNamedEventNameListHolder{};

std::array<std::wstring, SystemStateMonitor::kEventCount> CopyEventNames(
    const wchar_t* const (&eventNames)[SystemStateMonitor::kEventCount]) {
    std::array<std::wstring, SystemStateMonitor::kEventCount> copied{};
    for (std::size_t i = 0; i < SystemStateMonitor::kEventCount; ++i) {
        copied[i] = eventNames[i] != nullptr ? eventNames[i] : L"";
    }
    return copied;
}

HANDLE OpenOrCreateNamedEvent(const wchar_t* name) {
    HANDLE handle = OpenEventW(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, name);
    if (handle != nullptr) {
        return handle;
    }

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = FALSE;
    sa.lpSecurityDescriptor = nullptr;

    // Secure SDDL:
    // D: DACL
    // (A;;GA;;;SY) -> Allow Generic All for SYSTEM
    // (A;;GA;;;BA) -> Allow Generic All for Built-in Administrators
    // (A;;GRGW;;;BU) -> Allow Generic Read/Write for Built-in Users
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            L"D:(A;;GA;;;SY)(A;;GA;;;BA)(A;;GRGW;;;BU)",
            SDDL_REVISION_1,
            &sa.lpSecurityDescriptor,
            nullptr)) {
        // Fallback to default security descriptor if conversion fails
        sa.lpSecurityDescriptor = nullptr;
    }

    HANDLE new_handle = CreateEventW(sa.lpSecurityDescriptor ? &sa : nullptr, TRUE, FALSE, name);

    if (sa.lpSecurityDescriptor != nullptr) {
        LocalFree(sa.lpSecurityDescriptor);
    }

    return new_handle;
}

} // namespace

const char* ToString(SystemStateEventType type) noexcept {
    switch (type) {
    case SystemStateEventType::DisplayOn:
        return "DisplayOn";
    case SystemStateEventType::DisplayOff:
        return "DisplayOff";
    case SystemStateEventType::LidOn:
        return "LidOn";
    case SystemStateEventType::LidOff:
        return "LidOff";
    case SystemStateEventType::Suspend:
        return "Suspend";
    case SystemStateEventType::Shutdown:
        return "Shutdown";
    case SystemStateEventType::ResumeAutomatic:
        return "ResumeAutomatic";
    default:
        return "Unknown";
    }
}

SystemStateMonitor::SystemStateMonitor()
    : SystemStateMonitor(kNamedEventNameListHolder.names) {}

SystemStateMonitor::SystemStateMonitor(const wchar_t* const (&eventNames)[kEventCount])
    : m_impl(std::make_shared<Impl>()) {
    m_impl->eventNames = CopyEventNames(eventNames);
}

SystemStateMonitor::~SystemStateMonitor() {
    Stop();
}

namespace {

constexpr std::size_t kInvalidBatchPosition = static_cast<std::size_t>(-1);

const char* ToString(SystemStateTransportRole role) noexcept {
    switch (role) {
    case SystemStateTransportRole::Canonical:
        return "canonical";
    case SystemStateTransportRole::LegacyAlias:
        return "legacy_alias";
    default:
        return "unknown";
    }
}

bool IsDisplayStateEvent(SystemStateEventType type) noexcept {
    return type == SystemStateEventType::DisplayOn ||
           type == SystemStateEventType::DisplayOff;
}

bool IsLidStateEvent(SystemStateEventType type) noexcept {
    return type == SystemStateEventType::LidOn ||
           type == SystemStateEventType::LidOff;
}

template <typename ImplT>
void ResetNormalizationState(ImplT& impl) noexcept {
    impl.lastDisplayType = SystemStateEventType::Unknown;
    impl.lastLidType = SystemStateEventType::Unknown;
    impl.shutdownObserved = false;
}

template <typename ImplT>
bool ShouldSuppressNormalizedEvent(const ImplT& impl, SystemStateEventType type) noexcept {
    if (IsDisplayStateEvent(type)) {
        return impl.lastDisplayType == type;
    }
    if (IsLidStateEvent(type)) {
        return impl.lastLidType == type;
    }
    if (type == SystemStateEventType::Shutdown) {
        return impl.shutdownObserved;
    }
    return false;
}

template <typename ImplT>
void RememberNormalizedEvent(ImplT& impl, SystemStateEventType type) noexcept {
    if (IsDisplayStateEvent(type)) {
        impl.lastDisplayType = type;
        return;
    }
    if (IsLidStateEvent(type)) {
        impl.lastLidType = type;
        return;
    }
    if (type == SystemStateEventType::Shutdown) {
        impl.shutdownObserved = true;
    }
}

SystemStateEvent BuildEvent(std::size_t index, const wchar_t* rawName) {
    SystemStateEvent event{};
    event.source = SystemStateEventSource::ThpServiceNamedEvent;
    event.timestamp = std::chrono::system_clock::now();
    event.raw_index = static_cast<std::uint32_t>(index);
    event.raw_name = rawName != nullptr ? rawName : L"";

    const SystemStateNamedEventSpec* spec = TryGetNamedEventSpec(index);
    event.type = spec != nullptr ? spec->type : SystemStateEventType::Unknown;

    return event;
}

struct NormalizedBatchEntry {
    bool present = false;
    std::size_t firstPosition = kInvalidBatchPosition;
    SystemStateEvent event{};
    SystemStateTransportRole transportRole = SystemStateTransportRole::Canonical;
};

bool ShouldPreferTransportRole(SystemStateTransportRole current, SystemStateTransportRole candidate) noexcept {
    return current != SystemStateTransportRole::Canonical &&
           candidate == SystemStateTransportRole::Canonical;
}

template <typename ImplT>
void ResetNamedEvent(ImplT& impl, std::size_t index) noexcept {
    HANDLE eventHandle = impl.events[index];
    if (eventHandle != nullptr && eventHandle != INVALID_HANDLE_VALUE) {
        ResetEvent(eventHandle);
    }
}

template <typename ImplT>
std::size_t CollectAndResetSignaledEventBatch(
    ImplT& impl,
    std::size_t firstEventIndex,
    std::array<std::size_t, SystemStateMonitor::kEventCount>& batchIndices) noexcept {
    std::size_t batchCount = 0;

    // These named events are Win32 manual-reset events, so they encode a
    // coalesced level ("this state needs attention"), not an edge count. Reset
    // each observed level before callbacks run so a SetEvent issued during a
    // callback remains signaled for the next drain pass. A SetEvent racing with
    // this reset while the event is already signaled can still coalesce by API
    // design; exact per-Set delivery requires an out-of-band counter/queue.
    batchIndices[batchCount++] = firstEventIndex;
    ResetNamedEvent(impl, firstEventIndex);

    for (std::size_t i = 0; i < SystemStateMonitor::kEventCount; ++i) {
        if (i == firstEventIndex) {
            continue;
        }

        HANDLE eventHandle = impl.events[i];
        if (eventHandle == nullptr || eventHandle == INVALID_HANDLE_VALUE) {
            continue;
        }

        if (WaitForSingleObject(eventHandle, 0) == WAIT_OBJECT_0) {
            batchIndices[batchCount++] = i;
            ResetNamedEvent(impl, i);
        }
    }

    return batchCount;
}

template <typename ImplT>
void DispatchNormalizedBatch(
    ImplT& impl,
    const std::array<std::size_t, SystemStateMonitor::kEventCount>& batchIndices,
    std::size_t batchCount) {
    std::array<NormalizedBatchEntry, kSystemStateEventTypeCount> entries{};

    for (std::size_t position = 0; position < batchCount; ++position) {
        const std::size_t eventIndex = batchIndices[position];
        const SystemStateNamedEventSpec* spec = TryGetNamedEventSpec(eventIndex);
        SystemStateEvent event = BuildEvent(eventIndex, impl.eventNames[eventIndex].c_str());
        NormalizedBatchEntry& entry = entries[ToIndex(event.type)];

        if (!entry.present) {
            entry.present = true;
            entry.firstPosition = position;
            entry.event = event;
            if (spec != nullptr) {
                entry.transportRole = spec->transportRole;
            }
            continue;
        }

        if (spec != nullptr && ShouldPreferTransportRole(entry.transportRole, spec->transportRole)) {
            const auto firstTimestamp = entry.event.timestamp;
            entry.event = event;
            entry.event.timestamp = firstTimestamp;
            entry.transportRole = spec->transportRole;
        }
    }

    const bool displayBurstHasOnAndOff =
        entries[ToIndex(SystemStateEventType::DisplayOn)].present &&
        entries[ToIndex(SystemStateEventType::DisplayOff)].present;

    for (std::size_t position = 0; position < batchCount; ++position) {
        if (!impl.running.load(std::memory_order_acquire)) {
            break;
        }

        const std::size_t eventIndex = batchIndices[position];
        const SystemStateNamedEventSpec* spec = TryGetNamedEventSpec(eventIndex);
        const SystemStateEventType type = spec != nullptr ? spec->type : SystemStateEventType::Unknown;
        const NormalizedBatchEntry& entry = entries[ToIndex(type)];

        if (!entry.present) {
            continue;
        }

        if (entry.firstPosition != position) {
            LOG_INFO(
                "Host",
                __func__,
                "Normalize",
                "Suppressing named event[{}] as duplicate transport for type={}.",
                eventIndex,
                ToString(type));
            continue;
        }

        if (displayBurstHasOnAndOff && entry.event.type == SystemStateEventType::DisplayOff) {
            LOG_INFO(
                "Host",
                __func__,
                "Normalize",
                "Suppressing DisplayOff from mixed display burst; DisplayOn wins for transient display changes.");
            continue;
        }

        if (ShouldSuppressNormalizedEvent(impl, entry.event.type)) {
            LOG_INFO(
                "Host",
                __func__,
                "Normalize",
                "Suppressing normalized duplicate type={} from named event[{}] (role={}).",
                ToString(entry.event.type),
                entry.event.raw_index,
                ToString(entry.transportRole));
            continue;
        }

        LOG_INFO(
            "Host",
            __func__,
            "Signal",
            "Named event[{}] normalized -> type={} (role={}).",
            entry.event.raw_index,
            ToString(entry.event.type),
            ToString(entry.transportRole));

        RememberNormalizedEvent(impl, entry.event.type);

        SystemStateMonitor::EventCallback callback;
        {
            std::lock_guard<std::mutex> lock(impl.lifecycleMu);
            callback = impl.callback;
        }

        if (callback) {
            try {
                callback(entry.event);
            } catch (const std::exception& ex) {
                LOG_ERROR("Host", __func__, "Callback", "SystemStateMonitor callback threw std::exception: {}", ex.what());
            } catch (...) {
                LOG_ERROR("Host", __func__, "Callback", "SystemStateMonitor callback threw unknown exception.");
            }
        }
    }
}

template <typename ImplT>
bool OpenOrCreateEvents(ImplT& impl) {
    for (std::size_t i = 0; i < SystemStateMonitor::kEventCount; ++i) {
        const std::wstring& eventName = impl.eventNames[i];
        if (eventName.empty()) {
            return false;
        }

        impl.events[i] = OpenOrCreateNamedEvent(eventName.c_str());
        if (impl.events[i] == nullptr || impl.events[i] == INVALID_HANDLE_VALUE) {
            return false;
        }
    }

    return true;
}

template <typename ImplT>
void CloseEvents(ImplT& impl) noexcept {
    for (HANDLE& event_handle : impl.events) {
        if (event_handle != nullptr && event_handle != INVALID_HANDLE_VALUE) {
            CloseHandle(event_handle);
        }
        event_handle = nullptr;
    }

    if (impl.stopEvent != nullptr && impl.stopEvent != INVALID_HANDLE_VALUE) {
        CloseHandle(impl.stopEvent);
        impl.stopEvent = nullptr;
    }
}

template <typename ImplT>
void WorkerLoop(ImplT& impl) {
    {
        std::lock_guard<std::mutex> lock(impl.lifecycleMu);
        impl.workerThreadId = std::this_thread::get_id();
    }

    std::array<HANDLE, SystemStateMonitor::kEventCount + 1> wait_handles{};
    wait_handles[0] = impl.stopEvent;
    for (std::size_t i = 0; i < SystemStateMonitor::kEventCount; ++i) {
        wait_handles[i + 1] = impl.events[i];
    }

    while (impl.running.load(std::memory_order_acquire)) {
        const DWORD wait_result = WaitForMultipleObjects(
            static_cast<DWORD>(wait_handles.size()),
            wait_handles.data(),
            FALSE,
            INFINITE);

        if (wait_result == WAIT_OBJECT_0) {
            LOG_INFO("Host", __func__, "Stop", "Stop event signaled, exiting monitor loop.");
            break;
        }

        if (wait_result >= WAIT_OBJECT_0 + 1 &&
            wait_result < WAIT_OBJECT_0 + 1 + SystemStateMonitor::kEventCount) {
            const std::size_t firstEventIndex = static_cast<std::size_t>(wait_result - WAIT_OBJECT_0 - 1);
            std::array<std::size_t, SystemStateMonitor::kEventCount> batchIndices{};
            const std::size_t batchCount = CollectAndResetSignaledEventBatch(impl, firstEventIndex, batchIndices);
            DispatchNormalizedBatch(impl, batchIndices, batchCount);
            continue;
        }

        // WAIT_FAILED/WAIT_ABANDONED are treated as hard stop for this monitor instance.
        LOG_WARN("Host", __func__, "Error", "WaitForMultipleObjects returned unexpected result: {}", wait_result);
        break;
    }

    impl.running.store(false, std::memory_order_release);

    std::lock_guard<std::mutex> lock(impl.lifecycleMu);
    impl.workerThreadId = std::thread::id{};
    if (impl.selfStopCleanupPending) {
        CloseEvents(impl);
        impl.callback = nullptr;
        impl.selfStopCleanupPending = false;
        impl.lifecycleCv.notify_all();
    }
}

} // namespace

bool SystemStateMonitor::Start(EventCallback callback) {
    Impl& impl = *m_impl;
    std::unique_lock<std::mutex> lifecycleLock(impl.lifecycleMu);
    impl.lifecycleCv.wait(lifecycleLock, [&impl]() {
        return !impl.joining && !impl.selfStopCleanupPending;
    });

    if (impl.running.load(std::memory_order_acquire)) {
        return false;
    }

    if (impl.worker.joinable()) {
        if (impl.worker.get_id() == std::this_thread::get_id()) {
            LOG_WARN("Host", __func__, "Thread", "Start() called from worker thread while previous worker is joinable.");
            return false;
        }

        impl.joining = true;
        lifecycleLock.unlock();
        impl.worker.join();
        lifecycleLock.lock();
        impl.joining = false;
        impl.lifecycleCv.notify_all();
        CloseEvents(impl);
        impl.callback = nullptr;
    }

    if (impl.running.exchange(true, std::memory_order_acq_rel)) {
        return false;
    }

    ResetNormalizationState(impl);
    impl.callback = std::move(callback);
    impl.stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (impl.stopEvent == nullptr) {
        impl.running.store(false, std::memory_order_release);
        return false;
    }

    if (!OpenOrCreateEvents(impl)) {
        lifecycleLock.unlock();
        Stop();
        return false;
    }

    auto workerImpl = m_impl;
    impl.worker = std::thread([workerImpl]() {
        WorkerLoop(*workerImpl);
    });
    return true;
}

void SystemStateMonitor::Stop() {
    if (!m_impl) {
        return;
    }

    Impl& impl = *m_impl;
    std::unique_lock<std::mutex> lifecycleLock(impl.lifecycleMu);
    impl.running.store(false, std::memory_order_release);

    if (impl.stopEvent != nullptr) {
        SetEvent(impl.stopEvent);
    }

    if (!impl.worker.joinable()) {
        if (impl.workerThreadId == std::this_thread::get_id() && impl.selfStopCleanupPending) {
            return;
        }
        impl.lifecycleCv.wait(lifecycleLock, [&impl]() { return !impl.selfStopCleanupPending; });
        CloseEvents(impl);
        impl.callback = nullptr;
        return;
    }

    if (impl.worker.get_id() == std::this_thread::get_id()) {
        impl.selfStopCleanupPending = true;
        impl.worker.detach();
        LOG_INFO("SystemStateMonitor", __func__, "Monitor",
                 "Stop() called from callback; worker detached and cleanup deferred to worker exit.");
        return;
    }

    while (impl.joining) {
        impl.lifecycleCv.wait(lifecycleLock);
        if (!impl.worker.joinable()) {
            CloseEvents(impl);
            impl.callback = nullptr;
            return;
        }
    }

    impl.joining = true;
    lifecycleLock.unlock();
    impl.worker.join();
    lifecycleLock.lock();
    impl.joining = false;
    impl.lifecycleCv.notify_all();

    CloseEvents(impl);
    impl.callback = nullptr;
}

bool SystemStateMonitor::IsRunning() const noexcept {
    return m_impl && m_impl->running.load(std::memory_order_acquire);
}

const SystemStateNamedEventSpec (&SystemStateMonitor::NamedEventSpecs() noexcept)[SystemStateMonitor::kEventCount] {
    return kSystemStateNamedEventSpecs;
}

const wchar_t* const (&SystemStateMonitor::NamedEventList() noexcept)[SystemStateMonitor::kEventCount] {
    return kNamedEventNameListHolder.names;
}

bool SystemStateMonitor::SignalNamedEvent(SystemStateNamedEventId id) noexcept {
    return SignalNamedEvent(id, kNamedEventNameListHolder.names);
}

bool SystemStateMonitor::SignalNamedEvent(SystemStateNamedEventId id, const wchar_t* const (&eventNames)[kEventCount]) noexcept {
    const std::size_t index = ToIndex(id);
    if (index >= kEventCount || eventNames[index] == nullptr || eventNames[index][0] == L'\0') {
        return false;
    }

    HANDLE event_handle = OpenEventW(EVENT_MODIFY_STATE, FALSE, eventNames[index]);
    if (event_handle == nullptr || event_handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    const BOOL set_result = SetEvent(event_handle);
    CloseHandle(event_handle);
    return set_result != FALSE;
}

} // namespace Host
