#include "penpress/PenPressureReader.h"
#include "Logger.h"

#include <Windows.h>
#include <SetupAPI.h>
#include <hidsdi.h>
#include <algorithm>
#include <cwctype>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")

namespace Himax::Pen {

// ── 回调设置 ───────────────────────────────────────────────────────────────
void PenPressureReader::SetPressureCallback(PressureCallback cb) {
    std::lock_guard<std::mutex> lk(m_cbMutex);
    m_pressureCallback = std::move(cb);
}

PenPressureStats PenPressureReader::GetPressureStats() const {
    std::lock_guard<std::mutex> lk(m_statsMutex);
    return m_stats;
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
void PenPressureReader::OnPacketReceived(const std::vector<uint8_t>& packet) {
    // 解析 'U' 报文: [0x55][freq1][freq2][p0L][p0H][p1L][p1H][p2L][p2H][p3L][p3H]...
    if (packet.size() >= 11 && packet[0] == 0x55) {
        PenPressureStats s;
        s.reportType = packet[0];
        s.freq1      = packet[1];
        s.freq2      = packet[2];
        for (int k = 0; k < 4; ++k) {
            s.press[k] = static_cast<uint16_t>(packet[3 + k * 2]) |
                         (static_cast<uint16_t>(packet[4 + k * 2]) << 8);
        }
        {
            std::lock_guard<std::mutex> lk(m_statsMutex);
            m_stats = s;
        }
        // 压感回调
        {
            std::lock_guard<std::mutex> lk(m_cbMutex);
            if (m_pressureCallback) m_pressureCallback(s.press[0]);
        }
        if (m_notifyEvent) {
            SetEvent(m_notifyEvent);
        }
    }
}

} // namespace Himax::Pen
