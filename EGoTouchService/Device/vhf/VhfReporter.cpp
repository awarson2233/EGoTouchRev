#include "VhfReporter.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>
#include <thread>

// ── VHF HID Injector GUID ──
const GUID VhfReporter::kVhfGuid =
    {0x59819b74, 0xf102, 0x469a,
     {0x90, 0x09, 0x3c, 0xaf, 0x35, 0xfd, 0x46, 0x86}};

// ── Helpers ──

static inline void WriteU16Le(std::array<uint8_t, 32>& bytes,
                               size_t offset, uint16_t value) {
    bytes[offset]     = static_cast<uint8_t>(value & 0xFFu);
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

static inline uint8_t EncodeContactState(
        const Engine::TouchContact& c) {
    if (c.reportEvent == Engine::TouchReportUp)
        return 0x02; // TipSwitch=0, Confidence=1
    return 0x03;     // TipSwitch=1, Confidence=1
}

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

void VhfReporter::Dispatch(Engine::HeatmapFrame& frame) {
    if (!m_enabled.load()) return;

    BuildTouchReports(frame);

    const bool hasTouch =
        frame.touchPackets[0].valid || frame.touchPackets[1].valid;
    const bool hasStylus = frame.stylus.packet.valid;

    if (!hasTouch && !hasStylus) {
        if (m_hadTouchLastFrame.exchange(false)) {
            std::lock_guard<std::mutex> lk(m_mu);
            if (EnsureDeviceOpen()) {
                Engine::TouchPacket allUp{};
                allUp.bytes.fill(0);
                allUp.bytes[0] = 0x01;
                WritePacket(allUp.bytes.data(), allUp.length,
                            "touch-all-up");
            }
        }
        return;
    }
    m_hadTouchLastFrame.store(true);

    std::lock_guard<std::mutex> lk(m_mu);
    if (!EnsureDeviceOpen()) return;

    if (frame.touchPackets[0].valid)
        WritePacket(frame.touchPackets[0].bytes.data(),
                    frame.touchPackets[0].length, "touch-0");
    if (frame.touchPackets[1].valid)
        WritePacket(frame.touchPackets[1].bytes.data(),
                    frame.touchPackets[1].length, "touch-1");
    if (frame.stylus.packet.valid) {
        ApplyStylusPostTransform(frame.stylus.packet.bytes);
        WritePacket(frame.stylus.packet.bytes.data(),
                    frame.stylus.packet.length, "stylus");
    }
}

// ── 独立手写笔写入 ──

void VhfReporter::DispatchStylus(const Engine::StylusPacket& packet) {
    if (!m_enabled.load()) return;
    if (!packet.valid) return;

    std::lock_guard<std::mutex> lk(m_mu);
    if (!EnsureDeviceOpen()) return;

    // Copy and apply post-transform
    auto bytes = packet.bytes;
    ApplyStylusPostTransform(bytes);
    WritePacket(bytes.data(), packet.length, "stylus");
}

// ── 独立手指写入 ──

void VhfReporter::DispatchTouch(Engine::HeatmapFrame& frame) {
    if (!m_enabled.load()) return;

    BuildTouchReports(frame);

    const bool hasTouch =
        frame.touchPackets[0].valid || frame.touchPackets[1].valid;

    if (!hasTouch) {
        if (m_hadTouchLastFrame.exchange(false)) {
            std::lock_guard<std::mutex> lk(m_mu);
            if (EnsureDeviceOpen()) {
                Engine::TouchPacket allUp{};
                allUp.bytes.fill(0);
                allUp.bytes[0] = 0x01;
                WritePacket(allUp.bytes.data(), allUp.length,
                            "touch-all-up");
            }
        }
        return;
    }
    m_hadTouchLastFrame.store(true);

    std::lock_guard<std::mutex> lk(m_mu);
    if (!EnsureDeviceOpen()) return;

    if (frame.touchPackets[0].valid)
        WritePacket(frame.touchPackets[0].bytes.data(),
                    frame.touchPackets[0].length, "touch-0");
    if (frame.touchPackets[1].valid)
        WritePacket(frame.touchPackets[1].bytes.data(),
                    frame.touchPackets[1].length, "touch-1");
}

// ── Touch 报告构建 ──

void VhfReporter::BuildTouchReports(Engine::HeatmapFrame& frame) {
    frame.touchPackets[0] = Engine::TouchPacket{};
    frame.touchPackets[1] = Engine::TouchPacket{};
    frame.touchPackets[0].bytes.fill(0);
    frame.touchPackets[1].bytes.fill(0);
    frame.touchPackets[0].bytes[0] = 0x01;
    frame.touchPackets[1].bytes[0] = 0x01;

    std::vector<const Engine::TouchContact*> reportable;
    reportable.reserve(frame.contacts.size());
    for (const auto& c : frame.contacts) {
        if (c.id <= 0 || !c.isReported) continue;
        reportable.push_back(&c);
    }

    const size_t count = std::min<size_t>(10, reportable.size());
    frame.touchPackets[0].bytes[31] = static_cast<uint8_t>(count);

    const bool flip = m_transpose.load();
    for (size_t i = 0; i < count; ++i) {
        const auto& c = *reportable[i];
        const size_t pi = (i < 5) ? 0 : 1;
        const size_t slot = (i < 5) ? i : (i - 5);
        const size_t base = 1 + slot * 6;
        auto& bytes = frame.touchPackets[pi].bytes;

        bytes[base]     = EncodeContactState(c);
        bytes[base + 1] = static_cast<uint8_t>(
            std::clamp(c.id, 0, 255));
        const bool invertX = !flip;
        const bool invertY = flip;
        WriteU16Le(bytes, base + 2,
                   ToVhf(c.y, 40.0f, 16000.0f, invertY));
        WriteU16Le(bytes, base + 4,
                   ToVhf(c.x, 60.0f, 25600.0f, invertX));
    }

    frame.touchPackets[0].valid = (count > 0);
    frame.touchPackets[1].valid = (count > 5);
}

// ── Stylus 后处理 ──

void VhfReporter::ApplyStylusPostTransform(
        std::array<uint8_t, 17>& b) {
    if (m_eraserState.load() == 1u)
        b[1] = static_cast<uint8_t>((b[1] & 0xFEu) | 0x0Cu);
    else
        b[1] = static_cast<uint8_t>(b[1] & 0xF3u);
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
            LOG_INFO("VhfReporter", "EnsureDeviceOpen", "VHF",
                     "VHF device opened.");
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
        LOG_WARN("VhfReporter", "WritePacket", "VHF",
                 "Write {} failed (len={}, written={}, err={}), "
                 "trying reopen.",
                 tag, (unsigned)len, (unsigned)written, (unsigned)err);
        ReopenDevice();
        return false;
    }
    return true;
}
