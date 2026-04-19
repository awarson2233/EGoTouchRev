#include "VhfReporter.h"
#include "Logger.h"

#include <Windows.h>
#include <SetupAPI.h>

#include <algorithm>
#include <cmath>
#include <thread>

// ── VHF HID Injector GUID ──
const GUID VhfReporter::kVhfGuid =
    {0x59819b74, 0xf102, 0x469a,
     {0x90, 0x09, 0x3c, 0xaf, 0x35, 0xfd, 0x46, 0x86}};

// ── Helpers ──

namespace {

static inline void WriteU16Le(std::array<uint8_t, 32>& bytes,
                              size_t offset, uint16_t value) {
    bytes[offset] = static_cast<uint8_t>(value & 0xFFu);
    bytes[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
}

static inline uint16_t ToVhf(float gridValue, float gridMax,
                             float logicalMax, bool invert) {
    const float norm = std::clamp(gridValue / gridMax, 0.0f, 1.0f);
    const int vhf = std::clamp(
        static_cast<int>(std::lround(norm * logicalMax)),
        0, static_cast<int>(logicalMax));
    return static_cast<uint16_t>(
        invert ? (static_cast<int>(logicalMax) - vhf) : vhf);
}

static inline uint8_t EncodeContactState(const Solvers::TouchContact& c) {
    if (c.reportEvent == Solvers::TouchReportUp)
        return 0x02; // TipSwitch=0, Confidence=1
    return 0x03;     // TipSwitch=1, Confidence=1
}

static void BuildTouchReports(Solvers::HeatmapFrame& frame,
                              bool transposeEnabled) {
    for (auto& packet : frame.touchPackets) {
        packet = Solvers::TouchPacket{};
        packet.bytes.fill(0);
        packet.bytes[0] = 0x01;
    }

    std::vector<const Solvers::TouchContact*> reportable;
    reportable.reserve(frame.contacts.size());
    for (const auto& c : frame.contacts) {
        if (c.id <= 0 || !c.isReported) continue;
        reportable.push_back(&c);
    }

    const size_t count = std::min<size_t>(10, reportable.size());
    frame.touchPackets[0].bytes[31] = static_cast<uint8_t>(count);

    for (size_t i = 0; i < count; ++i) {
        const auto& c = *reportable[i];
        const size_t pi = (i < 5) ? 0 : 1;
        const size_t slot = (i < 5) ? i : (i - 5);
        const size_t base = 1 + slot * 6;
        auto& bytes = frame.touchPackets[pi].bytes;

        bytes[base] = EncodeContactState(c);
        bytes[base + 1] = static_cast<uint8_t>(std::clamp(c.id, 0, 255));

        const bool invertX = !transposeEnabled;
        const bool invertY = transposeEnabled;
        WriteU16Le(bytes, base + 2,
                   ToVhf(c.y, 40.0f, 16000.0f, invertY));
        WriteU16Le(bytes, base + 4,
                   ToVhf(c.x, 60.0f, 25600.0f, invertX));
    }

    frame.touchPackets[0].valid = (count > 0);
    frame.touchPackets[1].valid = (count > 5);
}

static void ApplyStylusPostTransform(std::array<uint8_t, 17>& bytes,
                                     uint8_t eraserState) {
    if (eraserState == 1u)
        bytes[1] = static_cast<uint8_t>((bytes[1] & 0xFEu) | 0x0Cu);
    else
        bytes[1] = static_cast<uint8_t>(bytes[1] & 0xF3u);
}

} // namespace

// ── Lifecycle ──

VhfReporter::VhfReporter() = default;
VhfReporter::~VhfReporter() { Close(); }

void VhfReporter::Close() {
    std::lock_guard<std::mutex> lk(m_mu);
    CloseDevice();
}

bool VhfReporter::IsDeviceOpen() const {
    return m_handle != INVALID_HANDLE_VALUE;
}

// ── 主入口 (legacy) ──

void VhfReporter::Dispatch(Solvers::HeatmapFrame& frame) {
    if (!m_enabled.load()) return;

    BuildTouchReports(frame, m_transpose.load());

    const bool hasTouch =
        frame.touchPackets[0].valid || frame.touchPackets[1].valid;
    const bool hasStylus = frame.stylus.packet.valid;

    if (!hasTouch && !hasStylus) {
        if (m_hadTouchLastFrame.exchange(false)) {
            std::lock_guard<std::mutex> lk(m_mu);
            WriteTouchAllUpLocked();
        }
        return;
    }
    m_hadTouchLastFrame.store(true);

    std::lock_guard<std::mutex> lk(m_mu);
    if (hasTouch) {
        WriteTouchPacketsLocked(frame.touchPackets);
    }
    if (hasStylus) {
        ApplyStylusPostTransform(frame.stylus.packet.bytes,
                                 m_eraserState.load());
        WriteStylusPacketLocked(frame.stylus.packet.bytes.data(),
                                frame.stylus.packet.length);
    }
}

// ── 独立手写笔写入 ──

void VhfReporter::DispatchStylus(const Solvers::StylusPacket& packet) {
    if (!m_enabled.load()) return;
    if (!packet.valid) return;

    auto bytes = packet.bytes;
    ApplyStylusPostTransform(bytes, m_eraserState.load());

    std::lock_guard<std::mutex> lk(m_mu);
    WriteStylusPacketLocked(bytes.data(), packet.length);
}

// ── 独立手指写入 ──

void VhfReporter::DispatchTouch(Solvers::HeatmapFrame& frame) {
    if (!m_enabled.load()) return;

    BuildTouchReports(frame, m_transpose.load());

    const bool hasTouch =
        frame.touchPackets[0].valid || frame.touchPackets[1].valid;

    if (!hasTouch) {
        if (m_hadTouchLastFrame.exchange(false)) {
            std::lock_guard<std::mutex> lk(m_mu);
            WriteTouchAllUpLocked();
        }
        return;
    }
    m_hadTouchLastFrame.store(true);

    std::lock_guard<std::mutex> lk(m_mu);
    WriteTouchPacketsLocked(frame.touchPackets);
}

// ── 传输写入职责 (requires m_mu held) ──

void VhfReporter::WriteTouchPacketsLocked(
        const std::array<Solvers::TouchPacket, 2>& packets) {
    if (!EnsureDeviceOpen()) return;

    if (packets[0].valid) {
        WritePacket(packets[0].bytes.data(), packets[0].length, "touch-0");
    }
    if (packets[1].valid) {
        WritePacket(packets[1].bytes.data(), packets[1].length, "touch-1");
    }
}

void VhfReporter::WriteTouchAllUpLocked() {
    if (!EnsureDeviceOpen()) return;

    Solvers::TouchPacket allUp{};
    allUp.bytes.fill(0);
    allUp.bytes[0] = 0x01;
    WritePacket(allUp.bytes.data(), allUp.length, "touch-all-up");
}

void VhfReporter::WriteStylusPacketLocked(const uint8_t* data, size_t len) {
    if (!EnsureDeviceOpen()) return;
    WritePacket(data, len, "stylus");
}

// ── 设备 I/O ──

bool VhfReporter::EnsureDeviceOpen() {
    if (m_handle != INVALID_HANDLE_VALUE) return true;

    HDEVINFO devInfo = SetupDiGetClassDevsW(
        &kVhfGuid, nullptr, nullptr,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devInfo == INVALID_HANDLE_VALUE) return false;

    bool opened = false;
    for (DWORD idx = 0;; ++idx) {
        SP_DEVICE_INTERFACE_DATA ifData{};
        ifData.cbSize = sizeof(ifData);
        if (!SetupDiEnumDeviceInterfaces(
                devInfo, nullptr, &kVhfGuid, idx, &ifData)) {
            if (GetLastError() == ERROR_NO_MORE_ITEMS) break;
            continue;
        }
        DWORD reqSize = 0;
        SetupDiGetDeviceInterfaceDetailW(
            devInfo, &ifData, nullptr, 0, &reqSize, nullptr);
        if (reqSize < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W))
            continue;

        std::vector<uint8_t> buf(reqSize, 0);
        auto* detail = reinterpret_cast<
            SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(buf.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (!SetupDiGetDeviceInterfaceDetailW(
                devInfo, &ifData, detail, reqSize, nullptr, nullptr))
            continue;

        HANDLE h = CreateFileW(
            detail->DevicePath,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr, OPEN_EXISTING, 0, nullptr);
        if (h != INVALID_HANDLE_VALUE) {
            m_handle = h;
            opened = true;
            LOG_INFO("VhfReporter", __func__, "VHF", "VHF device opened.");
            break;
        }
    }
    SetupDiDestroyDeviceInfoList(devInfo);
    return opened;
}

void VhfReporter::CloseDevice() {
    if (m_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
}

void VhfReporter::ReopenDevice() {
    CloseDevice();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EnsureDeviceOpen();
}

bool VhfReporter::WritePacket(const uint8_t* data, size_t len,
                               const char* tag) {
    if (!data || len == 0) return false;
    if (!EnsureDeviceOpen()) return false;

    DWORD written = 0;
    BOOL ok = WriteFile(m_handle, data,
                        static_cast<DWORD>(len),
                        &written, nullptr);
    if (!ok || written != len) {
        DWORD err = GetLastError();
        LOG_WARN("VhfReporter", __func__, "VHF", "Write {} failed (len={}, written={}, err={}), trying reopen.", tag, (unsigned)len, (unsigned)written, (unsigned)err);
        ReopenDevice();
        return false;
    }
    return true;
}
