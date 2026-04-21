#include "VhfReporter.h"
#include "Logger.h"

#include <Windows.h>
#include <SetupAPI.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <span>
#include <vector>

// ── VHF HID Injector GUID ──
const GUID VhfReporter::kVhfGuid =
    {0x59819b74, 0xf102, 0x469a,
     {0x90, 0x09, 0x3c, 0xaf, 0x35, 0xfd, 0x46, 0x86}};

// ── Helpers ──

namespace {

using TouchPackets = std::array<Solvers::TouchPacket, 2>;

constexpr float kTouchGridHeight = 40.0f;
constexpr float kTouchGridWidth = 60.0f;
constexpr float kTouchLogicalMaxY = 16000.0f;
constexpr float kTouchLogicalMaxX = 25600.0f;
constexpr size_t kContactsPerPacket = 5;
constexpr size_t kMaxTouchPackets = 2;
constexpr size_t kMaxReportedContacts =
    kContactsPerPacket * kMaxTouchPackets;
constexpr size_t kTouchPayloadOffset = 1;
constexpr size_t kTouchContactStride = 6;
constexpr size_t kTouchContactCountOffset = 31;

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

void WriteU16Le(std::array<uint8_t, 32>& bytes,
                size_t offset, uint16_t value) {
    bytes[offset] = static_cast<uint8_t>(value & 0xFFu);
    bytes[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
}

[[nodiscard]] uint16_t ToVhf(float gridValue, float gridMax,
                             float logicalMax, bool invert) {
    const float norm = std::clamp(gridValue / gridMax, 0.0f, 1.0f);
    const int vhf = std::clamp(
        static_cast<int>(std::lround(norm * logicalMax)),
        0, static_cast<int>(logicalMax));
    return static_cast<uint16_t>(
        invert ? (static_cast<int>(logicalMax) - vhf) : vhf);
}

[[nodiscard]] uint8_t EncodeContactState(const Solvers::TouchContact& c) {
    if (c.reportEvent == Solvers::TouchReportUp) {
        return 0x02; // TipSwitch=0, Confidence=1
    }
    return 0x03;     // TipSwitch=1, Confidence=1
}

[[nodiscard]] bool ShouldReportTouchContact(
        const Solvers::TouchContact& contact) noexcept {
    return contact.id > 0 && contact.isReported;
}

[[nodiscard]] bool HasTouchReports(const TouchPackets& packets) noexcept {
    return packets[0].valid || packets[1].valid;
}

[[nodiscard]] TouchPackets BuildTouchReports(
        std::span<const Solvers::TouchContact> contacts,
        bool transposeEnabled) {
    TouchPackets packets{};
    for (auto& packet : packets) {
        packet.bytes.fill(0);
        packet.bytes[0] = packet.reportId;
    }

    const bool invertX = !transposeEnabled;
    const bool invertY = transposeEnabled;
    size_t count = 0;
    for (const auto& contact : contacts) {
        if (count == kMaxReportedContacts) {
            break;
        }
        if (!ShouldReportTouchContact(contact)) {
            continue;
        }

        auto& packet = packets[count / kContactsPerPacket];
        const size_t slot = count % kContactsPerPacket;
        const size_t base = kTouchPayloadOffset + slot * kTouchContactStride;
        auto& bytes = packet.bytes;

        bytes[base] = EncodeContactState(contact);
        bytes[base + 1] =
            static_cast<uint8_t>(std::clamp(contact.id, 0, 255));
        WriteU16Le(bytes, base + 2,
                   ToVhf(contact.y, kTouchGridHeight,
                         kTouchLogicalMaxY, invertY));
        WriteU16Le(bytes, base + 4,
                   ToVhf(contact.x, kTouchGridWidth,
                         kTouchLogicalMaxX, invertX));
        ++count;
    }

    packets[0].bytes[kTouchContactCountOffset] =
        static_cast<uint8_t>(count);
    packets[0].valid = count > 0;
    packets[1].valid = count > kContactsPerPacket;
    return packets;
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

} // namespace

// ── Lifecycle ──

VhfReporter::VhfReporter() = default;
VhfReporter::~VhfReporter() { Close(); }

void VhfReporter::Close() {
    std::lock_guard<std::mutex> lk(m_mu);
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

// ── 主入口 (legacy) ──

void VhfReporter::Dispatch(Solvers::HeatmapFrame& frame) {
    if (!m_enabled.load(std::memory_order_relaxed)) {
        return;
    }

    frame.touchPackets = BuildTouchReports(
        frame.contacts,
        m_transpose.load(std::memory_order_relaxed));

    const bool hasTouch = HasTouchReports(frame.touchPackets);
    const bool sendTouchAllUp = UpdateTouchState(hasTouch);
    const bool hasStylus = frame.stylus.packet.valid;

    if (!hasTouch && !sendTouchAllUp && !hasStylus) {
        return;
    }

    std::array<uint8_t, 17> stylusBytes{};
    if (hasStylus) {
        stylusBytes = MakeStylusBytes(
            frame.stylus.packet,
            m_eraserState.load(std::memory_order_relaxed));
    }

    std::lock_guard<std::mutex> lk(m_mu);
    if (sendTouchAllUp) {
        WriteTouchAllUpLocked();
    }
    if (hasTouch) {
        WriteTouchPacketsLocked(frame.touchPackets);
    }
    if (hasStylus) {
        WriteStylusPacketLocked(stylusBytes.data(),
                                frame.stylus.packet.length);
    }
}

// ── 独立手写笔写入 ──

void VhfReporter::DispatchStylus(const Solvers::StylusPacket& packet) {
    if (!m_enabled.load(std::memory_order_relaxed) || !packet.valid) {
        return;
    }

    const auto bytes = MakeStylusBytes(
        packet,
        m_eraserState.load(std::memory_order_relaxed));

    std::lock_guard<std::mutex> lk(m_mu);
    WriteStylusPacketLocked(bytes.data(), packet.length);
}

// ── 独立手指写入 ──

void VhfReporter::DispatchTouch(Solvers::HeatmapFrame& frame) {
    if (!m_enabled.load(std::memory_order_relaxed)) {
        return;
    }

    frame.touchPackets = BuildTouchReports(
        frame.contacts,
        m_transpose.load(std::memory_order_relaxed));

    const bool hasTouch = HasTouchReports(frame.touchPackets);
    const bool sendTouchAllUp = UpdateTouchState(hasTouch);
    if (!hasTouch && !sendTouchAllUp) {
        return;
    }

    std::lock_guard<std::mutex> lk(m_mu);
    if (sendTouchAllUp) {
        WriteTouchAllUpLocked();
    }
    if (hasTouch) {
        WriteTouchPacketsLocked(frame.touchPackets);
    }
}

// ── 传输写入职责 (requires m_mu held) ──

void VhfReporter::WriteTouchPacketsLocked(
        const std::array<Solvers::TouchPacket, 2>& packets) {
    if (!EnsureDeviceOpenLocked()) {
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

void VhfReporter::WriteStylusPacketLocked(const uint8_t* data, size_t len) {
    if (!EnsureDeviceOpenLocked()) {
        return;
    }
    WritePacketLocked(data, len, "stylus");
}

// ── 设备 I/O ──

bool VhfReporter::EnsureDeviceOpenLocked() {
    if (m_handle != INVALID_HANDLE_VALUE) {
        return true;
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
