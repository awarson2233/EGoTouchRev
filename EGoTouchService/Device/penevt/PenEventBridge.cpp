#include "penevt/PenEventBridge.h"
#include "Logger.h"

#include <Windows.h>
#include <SetupAPI.h>
#include <hidsdi.h>
#include <algorithm>
#include <chrono>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")

namespace Himax::Pen {

// ── 静态常量 ──────────────────────────────────────────────────────────────
const GUID PenEventBridge::kEventDeviceGuid =
    {0xdd0ebedb, 0xf1d6, 0x4cfa, {0xac, 0xca, 0x71, 0xe6, 0x6d, 0x31, 0x78, 0xca}};

// ── 回调设置 ───────────────────────────────────────────────────────────────
void PenEventBridge::SetEventCallback(PenEventCallback cb) {
    std::lock_guard<std::mutex> lk(m_cbMutex);
    m_eventCallback = std::move(cb);
}

// ── 设备路径发现 ───────────────────────────────────────────────────────────
std::optional<std::wstring> PenEventBridge::FindDevicePath() {
    HDEVINFO devInfo = SetupDiGetClassDevsW(
        &kEventDeviceGuid, nullptr, nullptr,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devInfo == INVALID_HANDLE_VALUE) return std::nullopt;

    std::optional<std::wstring> result;
    for (DWORD i = 0; ; ++i) {
        SP_DEVICE_INTERFACE_DATA ifData{};
        ifData.cbSize = sizeof(ifData);
        if (!SetupDiEnumDeviceInterfaces(devInfo, nullptr,
                                         &kEventDeviceGuid, i, &ifData)) {
            if (GetLastError() == ERROR_NO_MORE_ITEMS) break;
            continue;
        }
        DWORD reqSize = 0;
        SetupDiGetDeviceInterfaceDetailW(devInfo, &ifData, nullptr, 0, &reqSize, nullptr);
        if (reqSize < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)) continue;
        std::vector<uint8_t> buf(reqSize, 0);
        auto* det = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(buf.data());
        det->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (SetupDiGetDeviceInterfaceDetailW(devInfo, &ifData, det, reqSize,
                                             nullptr, nullptr)) {
            result = det->DevicePath;
            break;
        }
    }
    SetupDiDestroyDeviceInfoList(devInfo);
    return result;
}

// ── 协议辅助 ───────────────────────────────────────────────────────────────
int PenEventBridge::GetAckCode(uint8_t eventCode) {
    switch (eventCode) {
    case 0x6F: return 11;   case 0x70: return 0;    case 0x71: return 1;
    case 0x72: return 2;    case 0x73: return 0xD;  case 0x74: return 3;
    case 0x75: return 4;    case 0x76: return 5;    case 0x77: return 6;
    case 0x78: return 7;    case 0x79: return 8;    case 0x7B: return 0xA;
    case 0x7C: return 0xC;  case 0x7F: return 9;    case 0x2F: return 0xB;
    default:   return -1;
    }
}

void PenEventBridge::SendRawPacket(const std::vector<uint8_t>& pkt) {
    if (!IsTransportOpen()) return;
    (void)GetTransport()->WritePacket(pkt);
}

void PenEventBridge::SendAck(uint8_t ackCode) {
    const std::vector<uint8_t> pkt = {
        0x07, 0x01, 0x02, 0x00,
        0x01, 0x80, 0x11, 0x00, ackCode
    };
    SendRawPacket(pkt);
    LOG_INFO("PenEventBridge", "SendAck", "MCU",
             "ACK sent: 0x{:02X}", ackCode);
}

void PenEventBridge::SendInitParamEcho(const uint8_t* data, size_t len) {
    size_t payloadLen = std::min(len, size_t(0x20));
    std::vector<uint8_t> pkt(8 + payloadLen, 0);
    pkt[0] = 0x07; pkt[1] = 0x20; pkt[2] = 0x01; pkt[3] = 0x00;
    pkt[4] = 0x01; pkt[5] = 0x7D;
    pkt[6] = 0x11; pkt[7] = 0x20;
    std::copy(data, data + payloadLen, pkt.begin() + 8);
    SendRawPacket(pkt);
    LOG_INFO("PenEventBridge", "SendInitParamEcho", "MCU",
             "0x7D01 InitParam echo sent ({} bytes payload).", payloadLen);
}

// ── BtHidChannel hooks ────────────────────────────────────────────────────
void PenEventBridge::OnConnected() {
    RunHandshake();  // 同步执行握手，避免 detach 的生命周期风险
}

void PenEventBridge::OnPacketReceived(const std::vector<uint8_t>& packet) {
    if (packet.size() < 7) return;

    const uint8_t eventCode = packet[5];

    // 1. 自动 ACK
    int ackCode = GetAckCode(eventCode);
    if (ackCode >= 0) {
        SendAck(static_cast<uint8_t>(ackCode));
    }

    // 2. PEN_REP_PARAM (0x7B) → 额外发送 0x7D01 InitParam 回显
    if (eventCode == 0x7B && packet.size() > 8) {
        SendInitParamEcho(packet.data() + 8, packet.size() - 8);
    }

    // 3. NewPenConnectRequest (0x15) → 发送 0x2E01 确认配对握手
    if (eventCode == 0x15) {
        const std::vector<uint8_t> pkt2e01 = {
            0x07, 0x00, 0x02, 0x00, 
            0x01, 0x2E, 0x11, 0x00
        };
        SendRawPacket(pkt2e01);
        LOG_INFO("PenEventBridge", "OnPacketReceived", "MCU", "Sent 0x2E01 pairing confirmation.");
    }

    // 4. 触发上层回调
    PenEventCallback cbCopy;
    {
        std::lock_guard<std::mutex> lk(m_cbMutex);
        cbCopy = m_eventCallback;
    }
    if (cbCopy) {
        PenEvent ev;
        ev.code     = static_cast<PenUsbEventCode>(eventCode);
        ev.payload  = std::vector<uint8_t>(packet.begin() + 8, packet.end());
        ev.receivedAt = std::chrono::steady_clock::now();
        cbCopy(ev);
    }

    if (m_notifyEvent) {
        SetEvent(m_notifyEvent);
    }
}

// ── 握手 ──────────────────────────────────────────────────────────────────
void PenEventBridge::RunHandshake() {
    if (!IsTransportOpen()) return;
    LOG_INFO("PenEventBridge", "RunHandshake", "MCU", "Sending 0x7101 + 0x7701 handshake.");

    const std::vector<uint8_t> q1 = {0x07,0x00,0x02,0x00, 0x01,0x71, 0x11,0x00};
    SendRawPacket(q1);

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    const std::vector<uint8_t> q2 = {0x07,0x00,0x02,0x00, 0x01,0x77, 0x11,0x00};
    SendRawPacket(q2);

    LOG_INFO("PenEventBridge", "RunHandshake", "MCU", "Handshake complete.");
}

} // namespace Himax::Pen
