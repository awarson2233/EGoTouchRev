#include "VhfReporter.h"
#include "VhfReporterStylusPacketHelper.h"
#include "VhfReporterTouchPacketHelper.h"
#include "Logger.h"

#include <Windows.h>
#include <SetupAPI.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <vector>

// ── VHF HID Injector GUID ──
const GUID VhfReporter::kVhfGuid =
    {0x59819b74, 0xf102, 0x469a,
     {0x90, 0x09, 0x3c, 0xaf, 0x35, 0xfd, 0x46, 0x86}};

// ── Helpers ──

namespace {

class DeviceInfoList {
public:
    explicit DeviceInfoList(const GUID& guid)
        : m_handle(SetupDiGetClassDevsW(
              &guid, nullptr, nullptr,
              DIGCF_PRESENT | DIGCF_DEVICEINTERFACE)) {}

    ~DeviceInfoList() {
        if (m_handle != INVALID_HANDLE_VALUE) {
            SetupDiDestroyDeviceInfoList(m_handle);
        }
    }

    DeviceInfoList(const DeviceInfoList&) = delete;
    DeviceInfoList& operator=(const DeviceInfoList&) = delete;

    [[nodiscard]] bool IsValid() const {
        return m_handle != INVALID_HANDLE_VALUE;
    }

    [[nodiscard]] HDEVINFO Get() const {
        return m_handle;
    }

private:
    HDEVINFO m_handle = INVALID_HANDLE_VALUE;
};

[[nodiscard]] bool HasTouchReports(
        const VhfTouchPacket::TouchPackets& packets) noexcept {
    return packets[0].valid || packets[1].valid;
}

void ApplyStylusPostTransform(std::array<uint8_t, 17>& bytes,
                              uint8_t eraserState) {
    if (eraserState == 1u) {
        bytes[1] = static_cast<uint8_t>((bytes[1] & 0xFEu) | 0x0Cu);
    } else {
        bytes[1] = static_cast<uint8_t>(bytes[1] & 0xF3u);
    }
}

[[nodiscard]] std::array<uint8_t, 17> MakeStylusBytes(
        const Solvers::StylusPacket& packet,
        uint8_t eraserState) {
    auto bytes = packet.bytes;
    ApplyStylusPostTransform(bytes, eraserState);
    return bytes;
}

[[nodiscard]] bool IsStylusActive(const std::array<uint8_t, 17>& bytes) noexcept {
    return (bytes[1] & 0x2Fu) != 0;
}

} // namespace

// ── Lifecycle ──

VhfReporter::VhfReporter() = default;
VhfReporter::~VhfReporter() { Close(); }

void VhfReporter::SetStylusPacketSensorRows(int rows) {
    std::lock_guard<std::mutex> lk(m_mu);
    m_stylusSensorRows = std::max(rows, 1);
}

void VhfReporter::SetStylusPacketSensorCols(int cols) {
    std::lock_guard<std::mutex> lk(m_mu);
    m_stylusSensorCols = std::max(cols, 1);
}

void VhfReporter::SetStylusPacketEmitWhenInvalid(bool v) {
    std::lock_guard<std::mutex> lk(m_mu);
    m_emitStylusPacketWhenInvalid = v;
}

void VhfReporter::SetEnabled(bool v) {
    std::lock_guard<std::mutex> lk(m_mu);
    if (m_closed) {
        m_enabled.store(false, std::memory_order_release);
        return;
    }

    const bool wasEnabled = m_enabled.exchange(v, std::memory_order_acq_rel);
    if (!(wasEnabled && !v)) {
        return;
    }

    const bool hadTouch =
        m_hadTouchLastFrame.exchange(false, std::memory_order_acq_rel);
    const bool hadStylus =
        m_hadStylusActiveLastFrame.exchange(false, std::memory_order_acq_rel);
    if (hadTouch) {
        WriteTouchAllUpLocked();
    }
    if (hadStylus) {
        WriteStylusNeutralLocked();
    }
}

void VhfReporter::FlushTouchAllUp() {
    if (!m_hadTouchLastFrame.exchange(false, std::memory_order_acq_rel)) {
        return;
    }

    std::lock_guard<std::mutex> lk(m_mu);
    if (m_closed) {
        return;
    }
    WriteTouchAllUpLocked();
}

void VhfReporter::Close() {
    std::lock_guard<std::mutex> lk(m_mu);
    m_closed = true;
    m_enabled.store(false, std::memory_order_release);
    const bool hadTouch =
        m_hadTouchLastFrame.exchange(false, std::memory_order_acq_rel);
    const bool hadStylus =
        m_hadStylusActiveLastFrame.exchange(false, std::memory_order_acq_rel);
    if (m_handle != INVALID_HANDLE_VALUE) {
        if (hadTouch) {
            WriteTouchAllUpLocked();
        }
        if (hadStylus && m_handle != INVALID_HANDLE_VALUE) {
            WriteStylusNeutralLocked();
        }
    }
    CloseDeviceLocked();
    m_nextOpenAttempt = std::chrono::steady_clock::time_point{};
}

bool VhfReporter::IsDeviceOpen() const {
    std::lock_guard<std::mutex> lk(m_mu);
    return m_handle != INVALID_HANDLE_VALUE;
}

bool VhfReporter::UpdateTouchState(bool hasTouch) {
    if (hasTouch) {
        m_hadTouchLastFrame.store(true, std::memory_order_relaxed);
        return false;
    }
    return m_hadTouchLastFrame.exchange(false,
                                         std::memory_order_relaxed);
}

VhfReporter::StylusDispatchPacket VhfReporter::BuildStylusPacket(
        const Solvers::StylusFrameData& stylus) {
    VhfStylusPacket::Config config;
    {
        std::lock_guard<std::mutex> lk(m_mu);
        config.sensorRows = m_stylusSensorRows;
        config.sensorCols = m_stylusSensorCols;
        config.emitWhenInvalid = m_emitStylusPacketWhenInvalid;
    }
    config.barrelButton = m_barrelButtonState.exchange(
        false, std::memory_order_relaxed);

    StylusDispatchPacket built{};
    built.packet = VhfStylusPacket::Build(stylus, config);
    built.penState = VhfStylusPacket::ExtractPenState(built.packet);
    return built;
}

void VhfReporter::MirrorLegacyStylusPacket(
        Solvers::HeatmapFrame& frame,
        const StylusDispatchPacket& built) {
    frame.stylus.output.packet = built.packet;
#if EGOTOUCH_DIAG
    frame.stylus.debug.coord.vhfPenState = built.penState;
#endif
}

// ── 主入口 (legacy) ──

void VhfReporter::Dispatch(Solvers::HeatmapFrame& frame) {
    const auto stylusPacket = BuildStylusPacket(frame.stylus);
    MirrorLegacyStylusPacket(frame, stylusPacket);
    if (!m_enabled.load(std::memory_order_relaxed)) {
        return;
    }

    frame.touch.output.touchPackets = VhfTouchPacket::Build(
        frame.touch.output.contacts,
        m_transpose.load(std::memory_order_relaxed));

    const bool hasTouch = HasTouchReports(frame.touch.output.touchPackets);
    const bool sendTouchAllUp = UpdateTouchState(hasTouch);
    const bool hasStylus = stylusPacket.packet.valid;

    if (!hasTouch && !sendTouchAllUp && !hasStylus) {
        return;
    }

    std::array<uint8_t, 17> stylusBytes{};
    if (hasStylus) {
        stylusBytes = MakeStylusBytes(
            stylusPacket.packet,
            m_eraserState.load(std::memory_order_relaxed));
        m_hadStylusActiveLastFrame.store(
            IsStylusActive(stylusBytes), std::memory_order_release);
    }

    std::lock_guard<std::mutex> lk(m_mu);
    if (!CanDispatchWriteLocked()) {
        return;
    }
    if (sendTouchAllUp) {
        WriteTouchAllUpLocked();
    }
    if (hasTouch) {
        WriteTouchPacketsLocked(frame.touch.output.touchPackets);
    }
    if (hasStylus) {
        WriteStylusPacketLocked(stylusBytes.data(),
                                stylusPacket.packet.length);
    }
}

// ── 独立手写笔写入 ──

void VhfReporter::DispatchStylus(Solvers::HeatmapFrame& frame,
                                 bool writeEnabled) {
    const auto stylusPacket = BuildStylusPacket(frame.stylus);
    MirrorLegacyStylusPacket(frame, stylusPacket);
    if (!writeEnabled ||
        !m_enabled.load(std::memory_order_relaxed) ||
        !stylusPacket.packet.valid) {
        if (!writeEnabled) {
            const bool hadStylus = m_hadStylusActiveLastFrame.exchange(
                false, std::memory_order_acq_rel);
            if (hadStylus) {
                std::lock_guard<std::mutex> lk(m_mu);
                if (!m_closed && m_handle != INVALID_HANDLE_VALUE) {
                    WriteStylusNeutralLocked();
                }
            }
        }
        return;
    }

    const auto bytes = MakeStylusBytes(
        stylusPacket.packet,
        m_eraserState.load(std::memory_order_relaxed));
    m_hadStylusActiveLastFrame.store(
        IsStylusActive(bytes), std::memory_order_release);

    std::lock_guard<std::mutex> lk(m_mu);
    if (!CanDispatchWriteLocked()) {
        return;
    }
    WriteStylusPacketLocked(bytes.data(), stylusPacket.packet.length);
}

// ── 独立手指写入 ──

void VhfReporter::DispatchTouch(Solvers::HeatmapFrame& frame) {
    if (!m_enabled.load(std::memory_order_relaxed)) {
        return;
    }

    frame.touch.output.touchPackets = VhfTouchPacket::Build(
        frame.touch.output.contacts,
        m_transpose.load(std::memory_order_relaxed));

    const bool hasTouch = HasTouchReports(frame.touch.output.touchPackets);
    const bool sendTouchAllUp = UpdateTouchState(hasTouch);
    if (!hasTouch && !sendTouchAllUp) {
        return;
    }

    std::lock_guard<std::mutex> lk(m_mu);
    if (!CanDispatchWriteLocked()) {
        return;
    }
    if (sendTouchAllUp) {
        WriteTouchAllUpLocked();
    }
    if (hasTouch) {
        WriteTouchPacketsLocked(frame.touch.output.touchPackets);
    }
}

// ── 传输写入职责 (requires m_mu held) ──

void VhfReporter::WriteTouchPacketsLocked(
        const std::array<Solvers::TouchPacket, 2>& packets) {
    if (!CanDispatchWriteLocked() || !EnsureDeviceOpenLocked()) {
        return;
    }

    if (packets[0].valid &&
        !WritePacketLocked(packets[0].bytes.data(),
                           packets[0].length, "touch-0")) {
        return;
    }
    if (packets[1].valid) {
        WritePacketLocked(packets[1].bytes.data(),
                          packets[1].length, "touch-1");
    }
}

void VhfReporter::WriteTouchAllUpLocked() {
    if (!EnsureDeviceOpenLocked()) {
        return;
    }

    Solvers::TouchPacket allUp{};
    allUp.bytes.fill(0);
    allUp.bytes[0] = allUp.reportId;
    WritePacketLocked(allUp.bytes.data(), allUp.length, "touch-all-up");
}

void VhfReporter::WriteStylusNeutralLocked() {
    if (!EnsureDeviceOpenLocked()) {
        return;
    }

    Solvers::StylusPacket neutral{};
    neutral.bytes.fill(0);
    neutral.bytes[0] = neutral.reportId;
    neutral.length = 13;
    WritePacketLocked(neutral.bytes.data(), neutral.length, "stylus-neutral");
}

void VhfReporter::WriteStylusPacketLocked(const uint8_t* data, size_t len) {
    if (!CanDispatchWriteLocked() || !EnsureDeviceOpenLocked()) {
        return;
    }
    WritePacketLocked(data, len, "stylus");
}

// ── 设备 I/O ──

bool VhfReporter::CanDispatchWriteLocked() const {
    return m_enabled.load(std::memory_order_relaxed) && !m_closed;
}

bool VhfReporter::EnsureDeviceOpenLocked() {
    if (m_handle != INVALID_HANDLE_VALUE) {
        return true;
    }
    if (m_closed) {
        return false;
    }

    const auto now = std::chrono::steady_clock::now();
    if (now < m_nextOpenAttempt) {
        return false;
    }

    DeviceInfoList devInfo(kVhfGuid);
    if (!devInfo.IsValid()) {
        m_nextOpenAttempt = now + kReopenBackoff;
        return false;
    }

    for (DWORD idx = 0;; ++idx) {
        SP_DEVICE_INTERFACE_DATA ifData{};
        ifData.cbSize = sizeof(ifData);
        if (!SetupDiEnumDeviceInterfaces(
                devInfo.Get(), nullptr, &kVhfGuid, idx, &ifData)) {
            if (GetLastError() == ERROR_NO_MORE_ITEMS) {
                break;
            }
            continue;
        }

        DWORD reqSize = 0;
        SetupDiGetDeviceInterfaceDetailW(
            devInfo.Get(), &ifData, nullptr, 0, &reqSize, nullptr);
        if (reqSize < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)) {
            continue;
        }

        std::vector<uint8_t> buf(reqSize, 0);
        auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(
            buf.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (!SetupDiGetDeviceInterfaceDetailW(
                devInfo.Get(), &ifData, detail, reqSize,
                nullptr, nullptr)) {
            continue;
        }

        HANDLE h = CreateFileW(
            detail->DevicePath,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr, OPEN_EXISTING, 0, nullptr);
        if (h != INVALID_HANDLE_VALUE) {
            m_handle = h;
            m_nextOpenAttempt = std::chrono::steady_clock::time_point{};
            LOG_INFO("VhfReporter", __func__, "VHF", "VHF device opened.");
            return true;
        }
    }

    m_nextOpenAttempt = now + kReopenBackoff;
    return false;
}

void VhfReporter::CloseDeviceLocked() {
    if (m_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
}

void VhfReporter::ScheduleReopenLocked() {
    CloseDeviceLocked();
    m_nextOpenAttempt = std::chrono::steady_clock::now() +
                        kReopenBackoff;
}

bool VhfReporter::WritePacketLocked(const uint8_t* data, size_t len,
                                    const char* tag) {
    if (!data || len == 0 || m_handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD written = 0;
    const BOOL ok = WriteFile(m_handle, data,
                              static_cast<DWORD>(len),
                              &written, nullptr);
    if (ok && written == len) {
        return true;
    }

    const DWORD err = ok ? ERROR_WRITE_FAULT : GetLastError();
    LOG_WARN("VhfReporter", __func__, "VHF", "Write {} failed (len={}, written={}, err={}), scheduling reopen.", tag, (unsigned)len, (unsigned)written, (unsigned)err);
    ScheduleReopenLocked();
    return false;
}
