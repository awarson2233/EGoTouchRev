#include "penpress/PenPressureReader.h"
#include "btmcu/PenPressurePacketParser.h"

#include <Windows.h>
#include <SetupAPI.h>
#include <hidsdi.h>
#include <algorithm>
#include <cwctype>
#include <utility>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")

namespace Himax::Pen {

PenPressureReader::~PenPressureReader() {
    Stop();
}

// ── 回调设置 ───────────────────────────────────────────────────────────────
void PenPressureReader::SetPressureCallback(PressureCallback cb) {
    auto callback = cb ? std::make_shared<const PressureCallback>(std::move(cb)) : nullptr;
    std::lock_guard<std::mutex> lk(m_cbMutex);
    m_pressureCallback = std::move(callback);
}

PenPressureStats PenPressureReader::GetPressureStats() const {
    std::lock_guard<std::mutex> lk(m_statsMutex);
    return m_stats;
}

void PenPressureReader::SetPressureRangeMode(PenPressureRangeMode mode) {
    std::lock_guard<std::mutex> lk(m_statsMutex);
    ApplyPressureModeLocked(mode);
}

PenPressureRangeMode PenPressureReader::GetPressureRangeMode() const {
    std::lock_guard<std::mutex> lk(m_statsMutex);
    return m_stats.pressureMode;
}

void PenPressureReader::ApplyPressureModeLocked(PenPressureRangeMode mode) {
    m_stats.pressureMode = mode;
    m_stats.pressureMax = PenPressureMax(mode);
    for (int k = 0; k < 4; ++k) {
        m_stats.press[k] = ScalePenPressure(m_stats.rawPress[k], mode);
    }
}

// ── 设备路径发现 ───────────────────────────────────────────────────────────
std::optional<std::wstring> PenPressureReader::FindDevicePath() {
    GUID hidGuid{};
    HidD_GetHidGuid(&hidGuid);
    HDEVINFO devInfo = SetupDiGetClassDevsW(
        &hidGuid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devInfo == INVALID_HANDLE_VALUE) return std::nullopt;

    auto containsCI = [](const std::wstring& hay, const wchar_t* needle) {
        std::wstring lo = hay;
        std::transform(lo.begin(), lo.end(), lo.begin(), ::towlower);
        std::wstring nl = needle;
        std::transform(nl.begin(), nl.end(), nl.begin(), ::towlower);
        return lo.find(nl) != std::wstring::npos;
    };

    std::optional<std::wstring> result;
    for (DWORD i = 0; ; ++i) {
        SP_DEVICE_INTERFACE_DATA ifData{};
        ifData.cbSize = sizeof(ifData);
        if (!SetupDiEnumDeviceInterfaces(devInfo, nullptr, &hidGuid, i, &ifData)) {
            if (GetLastError() == ERROR_NO_MORE_ITEMS) break;
            continue;
        }
        DWORD reqSize = 0;
        SetupDiGetDeviceInterfaceDetailW(devInfo, &ifData, nullptr, 0, &reqSize, nullptr);
        if (reqSize < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)) continue;
        std::vector<uint8_t> buf(reqSize, 0);
        auto* det = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(buf.data());
        det->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (!SetupDiGetDeviceInterfaceDetailW(devInfo, &ifData, det, reqSize,
                                              nullptr, nullptr)) continue;
        std::wstring path = det->DevicePath;
        if (containsCI(path, kPressureHidMatch)) {
            result = path;
            break;
        }
    }
    SetupDiDestroyDeviceInfoList(devInfo);
    return result;
}

// ── BtHidChannel hook ─────────────────────────────────────────────────────
void PenPressureReader::OnPacketReceived(std::span<const uint8_t> packet) {
    PenPressureStats stats;
    {
        std::lock_guard<std::mutex> lk(m_statsMutex);
        auto parsed = TryParsePenPressurePacket(packet, m_stats.pressureMode);
        if (!parsed) {
            return;
        }
        stats = *parsed;
        m_stats = stats;
    }

    std::shared_ptr<const PressureCallback> callback;
    {
        std::lock_guard<std::mutex> lk(m_cbMutex);
        callback = m_pressureCallback;
    }
    if (callback) {
        (*callback)(stats);
    }
    if (m_notifyEvent) {
        SetEvent(static_cast<HANDLE>(m_notifyEvent));
    }
}

} // namespace Himax::Pen
